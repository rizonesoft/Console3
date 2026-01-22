#pragma once
// Console3 - TabControl.h
// Tab bar control for managing terminal sessions
//
// Custom-drawn tab bar with support for drag-and-drop reordering,
// close buttons, and context menus.

#ifndef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_WIN10_RS5
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_WIN10
#endif

#define STRICT
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>

// ATL/WTL headers
#include <atlbase.h>
#include <atlapp.h>

extern CAppModule _Module;

#include <atlwin.h>
#include <atlcrack.h>
#include <atlgdi.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Console3::UI {

/// Tab item data
struct TabItem {
    int id = 0;                     ///< Unique tab ID
    std::wstring title = L"Tab";    ///< Tab title
    std::wstring icon;              ///< Icon path (optional)
    bool isDirty = false;           ///< Has unsaved changes
    bool isActive = false;          ///< Is currently active tab
    void* userData = nullptr;       ///< User-defined data (Session pointer)
};

/// Tab event types
enum class TabEvent {
    Selected,       ///< Tab was selected
    Closed,         ///< Tab close button clicked
    NewTab,         ///< New tab button clicked
    Reordered,      ///< Tab was reordered via drag
    ContextMenu     ///< Right-click on tab
};

/// Tab event callback
using TabEventCallback = std::function<void(TabEvent event, int tabId)>;

/// Custom-drawn tab bar control
class TabControl : public CWindowImpl<TabControl> {
public:
    DECLARE_WND_CLASS(L"Console3TabControl")

    TabControl();
    ~TabControl();

    /// Set the tab event callback
    void SetEventCallback(TabEventCallback callback);

    // ========================================================================
    // Tab Management
    // ========================================================================

    /// Add a new tab
    /// @param title Tab title
    /// @param userData User data to associate with tab
    /// @return Tab ID
    int AddTab(const std::wstring& title, void* userData = nullptr);

    /// Remove a tab by ID
    /// @return true if tab was removed
    bool RemoveTab(int tabId);

    /// Get the number of tabs
    [[nodiscard]] int GetTabCount() const;

    /// Get tab by index
    [[nodiscard]] TabItem* GetTab(int index);
    [[nodiscard]] const TabItem* GetTab(int index) const;

    /// Get tab by ID
    [[nodiscard]] TabItem* GetTabById(int tabId);

    /// Find tab index by ID
    [[nodiscard]] int FindTabIndex(int tabId) const;

    /// Select a tab by index
    void SelectTab(int index);

    /// Select a tab by ID
    void SelectTabById(int tabId);

    /// Get the active tab index
    [[nodiscard]] int GetActiveTabIndex() const;

    /// Get the active tab ID
    [[nodiscard]] int GetActiveTabId() const;

    /// Set tab title
    void SetTabTitle(int tabId, const std::wstring& title);

    /// Set tab dirty state
    void SetTabDirty(int tabId, bool dirty);

    // ========================================================================
    // Appearance
    // ========================================================================

    /// Set tab bar height
    void SetHeight(int height);

    /// Get tab bar height
    [[nodiscard]] int GetHeight() const { return m_height; }

    /// Set colors
    void SetBackgroundColor(COLORREF color) { m_bgColor = color; }
    void SetActiveTabColor(COLORREF color) { m_activeTabColor = color; }
    void SetInactiveTabColor(COLORREF color) { m_inactiveTabColor = color; }
    void SetTextColor(COLORREF color) { m_textColor = color; }

    // Message map
    BEGIN_MSG_MAP(TabControl)
        MSG_WM_CREATE(OnCreate)
        MSG_WM_DESTROY(OnDestroy)
        MSG_WM_SIZE(OnSize)
        MSG_WM_PAINT(OnPaint)
        MSG_WM_ERASEBKGND(OnEraseBkgnd)
        MSG_WM_LBUTTONDOWN(OnLButtonDown)
        MSG_WM_LBUTTONUP(OnLButtonUp)
        MSG_WM_LBUTTONDBLCLK(OnLButtonDblClk)
        MSG_WM_RBUTTONDOWN(OnRButtonDown)
        MSG_WM_MOUSEMOVE(OnMouseMove)
        MSG_WM_MOUSELEAVE(OnMouseLeave)
    END_MSG_MAP()

private:
    // Message handlers
    int OnCreate(LPCREATESTRUCT lpCreateStruct);
    void OnDestroy();
    void OnSize(UINT nType, CSize size);
    void OnPaint(CDCHandle dc);
    LRESULT OnEraseBkgnd(CDCHandle dc);
    void OnLButtonDown(UINT nFlags, CPoint point);
    void OnLButtonUp(UINT nFlags, CPoint point);
    void OnLButtonDblClk(UINT nFlags, CPoint point);
    void OnRButtonDown(UINT nFlags, CPoint point);
    void OnMouseMove(UINT nFlags, CPoint point);
    void OnMouseLeave();

    // Painting
    void PaintTab(CDCHandle dc, int index, const CRect& rect);
    void PaintNewTabButton(CDCHandle dc, const CRect& rect);
    void PaintCloseButton(CDCHandle dc, const CRect& rect, bool hover);

    // Hit testing
    int HitTestTab(CPoint point) const;
    bool HitTestCloseButton(int tabIndex, CPoint point) const;
    bool HitTestNewTabButton(CPoint point) const;

    // Layout
    void RecalculateLayout();
    CRect GetTabRect(int index) const;
    CRect GetNewTabButtonRect() const;
    CRect GetCloseButtonRect(int tabIndex) const;

private:
    // Tab data
    std::vector<TabItem> m_tabs;
    int m_nextTabId = 1;
    int m_activeIndex = -1;

    // Callback
    TabEventCallback m_eventCallback;

    // Layout
    int m_height = 32;
    int m_tabWidth = 200;
    int m_minTabWidth = 100;
    int m_newTabButtonWidth = 32;
    int m_closeButtonSize = 16;
    std::vector<CRect> m_tabRects;
    CRect m_newTabRect;

    // Interaction state
    int m_hoverIndex = -1;
    int m_pressedIndex = -1;
    bool m_hoverCloseButton = false;
    bool m_hoverNewTabButton = false;
    bool m_isDragging = false;
    int m_dragIndex = -1;
    CPoint m_dragStart;
    bool m_trackingMouse = false;

    // Colors
    COLORREF m_bgColor = RGB(45, 45, 45);
    COLORREF m_activeTabColor = RGB(30, 30, 30);
    COLORREF m_inactiveTabColor = RGB(60, 60, 60);
    COLORREF m_hoverTabColor = RGB(70, 70, 70);
    COLORREF m_textColor = RGB(255, 255, 255);
    COLORREF m_closeButtonColor = RGB(150, 150, 150);
    COLORREF m_closeButtonHoverColor = RGB(232, 17, 35);

    // Font
    CFont m_font;
};

} // namespace Console3::UI
