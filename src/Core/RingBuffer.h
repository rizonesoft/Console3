#pragma once
// Console3 - RingBuffer.h
// Thread-safe lock-free ring buffer for PTY I/O
//
// This is a single-producer, single-consumer (SPSC) lock-free ring buffer
// optimized for the PTY I/O thread writing and the UI thread reading.

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace Console3::Core {

/// Thread-safe SPSC (Single Producer, Single Consumer) ring buffer
/// Producer: IoThread writes PTY output data
/// Consumer: UI/Emulation reads data for processing
template <typename T = char> class RingBuffer {
public:
  /// Create a ring buffer with the specified capacity
  /// @param capacity Buffer capacity in elements (will be rounded up to power
  /// of 2)
  explicit RingBuffer(size_t capacity = 65536)
      : m_capacity(NextPowerOfTwo(capacity)), m_mask(m_capacity - 1),
        m_buffer(m_capacity), m_head(0), m_tail(0) {}

  ~RingBuffer() = default;

  // Non-copyable, non-movable (atomic members)
  RingBuffer(const RingBuffer &) = delete;
  RingBuffer &operator=(const RingBuffer &) = delete;
  RingBuffer(RingBuffer &&) = delete;
  RingBuffer &operator=(RingBuffer &&) = delete;

  /// Write data to the buffer (producer side)
  /// @param data Pointer to data to write
  /// @param length Number of elements to write
  /// @return Number of elements actually written (may be less if buffer is
  /// full)
  size_t Write(const T *data, size_t length) noexcept {
    const size_t head = m_head.load(std::memory_order_relaxed);
    const size_t tail = m_tail.load(std::memory_order_acquire);

    const size_t available = AvailableToWrite(head, tail);
    const size_t toWrite = std::min(length, available);

    if (toWrite == 0) {
      return 0;
    }

    // Calculate positions with wrap-around
    const size_t headIndex = head & m_mask;
    const size_t firstChunk = std::min(toWrite, m_capacity - headIndex);
    const size_t secondChunk = toWrite - firstChunk;

    // Copy data in one or two chunks (handles wrap-around)
    std::memcpy(&m_buffer[headIndex], data, firstChunk * sizeof(T));
    if (secondChunk > 0) {
      std::memcpy(&m_buffer[0], data + firstChunk, secondChunk * sizeof(T));
    }

    // Publish the new head position
    m_head.store(head + toWrite, std::memory_order_release);
    return toWrite;
  }

  /// Read data from the buffer (consumer side)
  /// @param data Pointer to buffer to read into
  /// @param maxLength Maximum number of elements to read
  /// @return Number of elements actually read
  size_t Read(T *data, size_t maxLength) noexcept {
    const size_t tail = m_tail.load(std::memory_order_relaxed);
    const size_t head = m_head.load(std::memory_order_acquire);

    const size_t available = AvailableToRead(head, tail);
    const size_t toRead = std::min(maxLength, available);

    if (toRead == 0) {
      return 0;
    }

    // Calculate positions with wrap-around
    const size_t tailIndex = tail & m_mask;
    const size_t firstChunk = std::min(toRead, m_capacity - tailIndex);
    const size_t secondChunk = toRead - firstChunk;

    // Copy data in one or two chunks (handles wrap-around)
    std::memcpy(data, &m_buffer[tailIndex], firstChunk * sizeof(T));
    if (secondChunk > 0) {
      std::memcpy(data + firstChunk, &m_buffer[0], secondChunk * sizeof(T));
    }

    // Publish the new tail position
    m_tail.store(tail + toRead, std::memory_order_release);
    return toRead;
  }

  /// Peek at data without consuming it (consumer side)
  /// @param data Pointer to buffer to copy into
  /// @param maxLength Maximum number of elements to peek
  /// @return Number of elements actually peeked
  size_t Peek(T *data, size_t maxLength) const noexcept {
    const size_t tail = m_tail.load(std::memory_order_relaxed);
    const size_t head = m_head.load(std::memory_order_acquire);

    const size_t available = AvailableToRead(head, tail);
    const size_t toPeek = std::min(maxLength, available);

    if (toPeek == 0) {
      return 0;
    }

    const size_t tailIndex = tail & m_mask;
    const size_t firstChunk = std::min(toPeek, m_capacity - tailIndex);
    const size_t secondChunk = toPeek - firstChunk;

    std::memcpy(data, &m_buffer[tailIndex], firstChunk * sizeof(T));
    if (secondChunk > 0) {
      std::memcpy(data + firstChunk, &m_buffer[0], secondChunk * sizeof(T));
    }

    return toPeek;
  }

  /// Skip/discard elements from the buffer (consumer side)
  /// @param count Number of elements to skip
  /// @return Number of elements actually skipped
  size_t Skip(size_t count) noexcept {
    const size_t tail = m_tail.load(std::memory_order_relaxed);
    const size_t head = m_head.load(std::memory_order_acquire);

    const size_t available = AvailableToRead(head, tail);
    const size_t toSkip = std::min(count, available);

    if (toSkip > 0) {
      m_tail.store(tail + toSkip, std::memory_order_release);
    }
    return toSkip;
  }

  /// Get the number of elements available to read
  [[nodiscard]] size_t Size() const noexcept {
    const size_t head = m_head.load(std::memory_order_acquire);
    const size_t tail = m_tail.load(std::memory_order_acquire);
    return AvailableToRead(head, tail);
  }

  /// Get the number of elements that can be written
  [[nodiscard]] size_t Available() const noexcept {
    const size_t head = m_head.load(std::memory_order_acquire);
    const size_t tail = m_tail.load(std::memory_order_acquire);
    return AvailableToWrite(head, tail);
  }

  /// Check if the buffer is empty
  [[nodiscard]] bool IsEmpty() const noexcept { return Size() == 0; }

  /// Check if the buffer is full
  [[nodiscard]] bool IsFull() const noexcept { return Available() == 0; }

  /// Get the buffer capacity
  [[nodiscard]] size_t Capacity() const noexcept {
    return m_capacity -
           1; // One slot is always empty to distinguish full from empty
  }

  /// Clear all data from the buffer (not thread-safe - use only when no
  /// concurrent access)
  void Clear() noexcept {
    m_head.store(0, std::memory_order_relaxed);
    m_tail.store(0, std::memory_order_relaxed);
  }

private:
  /// Round up to the next power of two
  static constexpr size_t NextPowerOfTwo(size_t n) noexcept {
    if (n == 0)
      return 1;
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    return n + 1;
  }

  /// Calculate elements available to read
  [[nodiscard]] size_t AvailableToRead(size_t head,
                                       size_t tail) const noexcept {
    return head - tail;
  }

  /// Calculate elements available to write (leave one slot empty)
  [[nodiscard]] size_t AvailableToWrite(size_t head,
                                        size_t tail) const noexcept {
    return (m_capacity - 1) - (head - tail);
  }

private:
  const size_t m_capacity; ///< Buffer capacity (power of 2)
  const size_t m_mask;     ///< Bitmask for fast modulo (capacity - 1)
  std::vector<T> m_buffer; ///< Underlying storage

  // Cache-line padding to prevent false sharing between producer and consumer
  alignas(64) std::atomic<size_t> m_head; ///< Write position (producer)
  alignas(64) std::atomic<size_t> m_tail; ///< Read position (consumer)
};

/// Convenience alias for byte buffer (PTY I/O)
using ByteRingBuffer = RingBuffer<char>;

} // namespace Console3::Core
