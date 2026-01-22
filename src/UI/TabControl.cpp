// Console3 - TabControl.cpp
// Tab bar control implementation

#include "UI/TabControl.h"
#include <algorithm>

namespace Console3::UI {

TabControl::TabControl() = default;

TabControl::~TabControl() = default;

void TabControl::SetEventCallback(TabEventCallback callback) {
    m_eventCallback = std::move(callback);
}

// ============================================================================
// Tab Management
// ============================================================================

int TabControl::AddTab(const std::wstring& title, void* userData) {
    TabItem tab;
    tab.id = m_nextTabId++;
    tab.title = title;
    tab.userData = userData;
    
    m_tabs.push_back(tab);
    RecalculateLayout();
    
    // Auto-select if first tab
    if (m_tabs.size() == 1) {
        SelectTab(0);
    }
    
    Invalidate();
    return tab.id;
}

bool TabControl::RemoveTab(int tabId) {
    int index = FindTabIndex(tabId);
    if (index < 0) return false;
    
    m_tabs.erase(m_tabs.begin() + index);
    
    // Adjust active index
    if (m_activeIndex >= static_cast<int>(m_tabs.size())) {
        m_activeIndex = static_cast<int>(m_tabs.size()) - 1;
    }
    
    // Update active state
    if (m_activeIndex >= 0 && m_activeIndex < static_cast<int>(m_tabs.size())) {
        m_tabs[m_activeIndex].isActive = true;
        if (m_eventCallback) {
            m_eventCallback(TabEvent::Selected, m_tabs[m_activeIndex].id);
        }
    }
    
    RecalculateLayout();
    Invalidate();
    return true;
}

int TabControl::GetTabCount() const {
    return static_cast<int>(m_tabs.size());
}

TabItem* TabControl::GetTab(int index) {
    if (index >= 0 && index < static_cast<int>(m_tabs.size())) {
        return &m_tabs[index];
    }
    return nullptr;
}

const TabItem* TabControl::GetTab(int index) const {
    if (index >= 0 && index < static_cast<int>(m_tabs.size())) {
        return &m_tabs[index];
    }
    return nullptr;
}

TabItem* TabControl::GetTabById(int tabId) {
    int index = FindTabIndex(tabId);
    return GetTab(index);
}

int TabControl::FindTabIndex(int tabId) const {
    for (size_t i = 0; i < m_tabs.size(); ++i) {
        if (m_tabs[i].id == tabId) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void TabControl::SelectTab(int index) {
    if (index < 0 || index >= static_cast<int>(m_tabs.size())) {
        return;
    }
    
    // Deselect current
    if (m_activeIndex >= 0 && m_activeIndex < static_cast<int>(m_tabs.size())) {
        m_tabs[m_activeIndex].isActive = false;
    }
    
    // Select new
    m_activeIndex = index;
    m_tabs[index].isActive = true;
    
    if (m_eventCallback) {
        m_eventCallback(TabEvent::Selected, m_tabs[index].id);
    }
    
    Invalidate();
}

void TabControl::SelectTabById(int tabId) {
    int index = FindTabIndex(tabId);
    SelectTab(index);
}

int TabControl::GetActiveTabIndex() const {
    return m_activeIndex;
}

int TabControl::GetActiveTabId() const {
    if (m_activeIndex >= 0 && m_activeIndex < static_cast<int>(m_tabs.size())) {
        return m_tabs[m_activeIndex].id;
    }
    return -1;
}

void TabControl::SetTabTitle(int tabId, const std::wstring& title) {
    if (auto* tab = GetTabById(tabId)) {
        tab->title = title;
        Invalidate();
    }
}

void TabControl::SetTabDirty(int tabId, bool dirty) {
    if (auto* tab = GetTabById(tabId)) {
        tab->isDirty = dirty;
        Invalidate();
    }
}

void TabControl::SetHeight(int height) {
    m_height = height;
    RecalculateLayout();
    Invalidate();
}

// ============================================================================
// Message Handlers
// ============================================================================

int TabControl::OnCreate(LPCREATESTRUCT /*lpCreateStruct*/) {
    // Create font
    m_font.CreateFont(
        14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI"
    );
    
    RecalculateLayout();
    return 0;
}

void TabControl::OnDestroy() {
    m_font.DeleteObject();
}

void TabControl::OnSize(UINT nType, CSize /*size*/) {
    if (nType != SIZE_MINIMIZED) {
        RecalculateLayout();
        Invalidate();
    }
}

void TabControl::OnPaint(CDCHandle /*dc*/) {
    CPaintDC paintDc(m_hWnd);
    CDCHandle dc = paintDc.m_hDC;
    
    CRect clientRect;
    GetClientRect(&clientRect);
    
    // Create memory DC for double buffering
    CDC memDC;
    memDC.CreateCompatibleDC(dc);
    CBitmap memBitmap;
    memBitmap.CreateCompatibleBitmap(dc, clientRect.Width(), clientRect.Height());
    HBITMAP oldBitmap = memDC.SelectBitmap(memBitmap);
    
    // Fill background
    memDC.FillSolidRect(&clientRect, m_bgColor);
    
    // Draw tabs
    HFONT oldFont = memDC.SelectFont(m_font);
    memDC.SetBkMode(TRANSPARENT);
    
    for (size_t i = 0; i < m_tabs.size(); ++i) {
        PaintTab(memDC.m_hDC, static_cast<int>(i), m_tabRects[i]);
    }
    
    // Draw new tab button
    PaintNewTabButton(memDC.m_hDC, m_newTabRect);
    
    memDC.SelectFont(oldFont);
    
    // Blit to screen
    dc.BitBlt(0, 0, clientRect.Width(), clientRect.Height(), memDC, 0, 0, SRCCOPY);
    
    memDC.SelectBitmap(oldBitmap);
}

LRESULT TabControl::OnEraseBkgnd(CDCHandle /*dc*/) {
    return 1;  // We handle background in OnPaint
}

void TabControl::OnLButtonDown(UINT /*nFlags*/, CPoint point) {
    SetCapture();
    
    // Check new tab button
    if (HitTestNewTabButton(point)) {
        m_pressedIndex = -2;  // Special value for new tab
        return;
    }
    
    // Check close buttons
    for (size_t i = 0; i < m_tabs.size(); ++i) {
        if (HitTestCloseButton(static_cast<int>(i), point)) {
            m_pressedIndex = static_cast<int>(i);
            m_hoverCloseButton = true;
            return;
        }
    }
    
    // Check tab selection
    int tabIndex = HitTestTab(point);
    if (tabIndex >= 0) {
        m_pressedIndex = tabIndex;
        m_dragStart = point;
        SelectTab(tabIndex);
    }
}

void TabControl::OnLButtonUp(UINT /*nFlags*/, CPoint point) {
    ReleaseCapture();
    
    // New tab button
    if (m_pressedIndex == -2 && HitTestNewTabButton(point)) {
        if (m_eventCallback) {
            m_eventCallback(TabEvent::NewTab, -1);
        }
    }
    // Close button
    else if (m_pressedIndex >= 0 && m_hoverCloseButton && HitTestCloseButton(m_pressedIndex, point)) {
        int tabId = m_tabs[m_pressedIndex].id;
        if (m_eventCallback) {
            m_eventCallback(TabEvent::Closed, tabId);
        }
    }
    // Drag end
    else if (m_isDragging) {
        m_isDragging = false;
        if (m_eventCallback && m_dragIndex != m_pressedIndex) {
            m_eventCallback(TabEvent::Reordered, m_tabs[m_activeIndex].id);
        }
    }
    
    m_pressedIndex = -1;
    m_hoverCloseButton = false;
    Invalidate();
}

void TabControl::OnLButtonDblClk(UINT nFlags, CPoint point) {
    // Double click on empty area = new tab
    if (HitTestTab(point) < 0 && !HitTestNewTabButton(point)) {
        if (m_eventCallback) {
            m_eventCallback(TabEvent::NewTab, -1);
        }
    } else {
        OnLButtonDown(nFlags, point);
    }
}

void TabControl::OnRButtonDown(UINT /*nFlags*/, CPoint point) {
    int tabIndex = HitTestTab(point);
    if (tabIndex >= 0) {
        SelectTab(tabIndex);
        if (m_eventCallback) {
            m_eventCallback(TabEvent::ContextMenu, m_tabs[tabIndex].id);
        }
    }
}

void TabControl::OnMouseMove(UINT nFlags, CPoint point) {
    // Track mouse leave
    if (!m_trackingMouse) {
        TRACKMOUSEEVENT tme{};
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = m_hWnd;
        TrackMouseEvent(&tme);
        m_trackingMouse = true;
    }
    
    // Handle dragging
    if (m_pressedIndex >= 0 && (nFlags & MK_LBUTTON) && !m_hoverCloseButton) {
        int dx = abs(point.x - m_dragStart.x);
        if (dx > 5 && !m_isDragging) {
            m_isDragging = true;
            m_dragIndex = m_pressedIndex;
        }
        
        if (m_isDragging) {
            // Find new position
            int newIndex = -1;
            for (size_t i = 0; i < m_tabRects.size(); ++i) {
                if (m_tabRects[i].PtInRect(point)) {
                    newIndex = static_cast<int>(i);
                    break;
                }
            }
            
            if (newIndex >= 0 && newIndex != m_dragIndex) {
                // Swap tabs
                std::swap(m_tabs[m_dragIndex], m_tabs[newIndex]);
                std::swap(m_tabRects[m_dragIndex], m_tabRects[newIndex]);
                m_dragIndex = newIndex;
                m_activeIndex = newIndex;
                Invalidate();
            }
        }
        return;
    }
    
    // Update hover state
    int oldHover = m_hoverIndex;
    m_hoverIndex = HitTestTab(point);
    m_hoverNewTabButton = HitTestNewTabButton(point);
    
    bool wasHoverClose = m_hoverCloseButton;
    m_hoverCloseButton = false;
    for (size_t i = 0; i < m_tabs.size(); ++i) {
        if (HitTestCloseButton(static_cast<int>(i), point)) {
            m_hoverCloseButton = true;
            m_hoverIndex = static_cast<int>(i);
            break;
        }
    }
    
    if (oldHover != m_hoverIndex || wasHoverClose != m_hoverCloseButton) {
        Invalidate();
    }
}

void TabControl::OnMouseLeave() {
    m_trackingMouse = false;
    m_hoverIndex = -1;
    m_hoverCloseButton = false;
    m_hoverNewTabButton = false;
    Invalidate();
}

// ============================================================================
// Painting
// ============================================================================

void TabControl::PaintTab(CDCHandle dc, int index, const CRect& rect) {
    const TabItem& tab = m_tabs[index];
    
    // Background
    COLORREF bgColor = m_inactiveTabColor;
    if (tab.isActive) {
        bgColor = m_activeTabColor;
    } else if (index == m_hoverIndex) {
        bgColor = m_hoverTabColor;
    }
    
    dc.FillSolidRect(&rect, bgColor);
    
    // Title
    dc.SetTextColor(m_textColor);
    CRect textRect = rect;
    textRect.left += 10;
    textRect.right -= m_closeButtonSize + 10;
    
    std::wstring title = tab.title;
    if (tab.isDirty) {
        title = L"● " + title;
    }
    
    dc.DrawText(title.c_str(), static_cast<int>(title.length()), &textRect,
                DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    
    // Close button
    CRect closeRect = GetCloseButtonRect(index);
    bool hoverClose = (m_hoverIndex == index && m_hoverCloseButton);
    PaintCloseButton(dc, closeRect, hoverClose);
}

void TabControl::PaintNewTabButton(CDCHandle dc, const CRect& rect) {
    // Background on hover
    if (m_hoverNewTabButton) {
        dc.FillSolidRect(&rect, m_hoverTabColor);
    }
    
    // Plus sign
    dc.SetTextColor(m_textColor);
    dc.DrawText(L"+", 1, const_cast<CRect*>(&rect), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void TabControl::PaintCloseButton(CDCHandle dc, const CRect& rect, bool hover) {
    COLORREF color = hover ? m_closeButtonHoverColor : m_closeButtonColor;
    
    if (hover) {
        dc.FillSolidRect(&rect, m_closeButtonHoverColor);
        color = RGB(255, 255, 255);
    }
    
    // Draw X
    dc.SetTextColor(color);
    dc.DrawText(L"×", 1, const_cast<CRect*>(&rect), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// ============================================================================
// Hit Testing
// ============================================================================

int TabControl::HitTestTab(CPoint point) const {
    for (size_t i = 0; i < m_tabRects.size(); ++i) {
        if (m_tabRects[i].PtInRect(point)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool TabControl::HitTestCloseButton(int tabIndex, CPoint point) const {
    if (tabIndex < 0 || tabIndex >= static_cast<int>(m_tabs.size())) {
        return false;
    }
    CRect closeRect = GetCloseButtonRect(tabIndex);
    return closeRect.PtInRect(point) != FALSE;
}

bool TabControl::HitTestNewTabButton(CPoint point) const {
    return m_newTabRect.PtInRect(point) != FALSE;
}

// ============================================================================
// Layout
// ============================================================================

void TabControl::RecalculateLayout() {
    CRect clientRect;
    GetClientRect(&clientRect);
    
    m_tabRects.clear();
    m_tabRects.resize(m_tabs.size());
    
    // Calculate tab width
    int totalTabsWidth = clientRect.Width() - m_newTabButtonWidth;
    int tabCount = static_cast<int>(m_tabs.size());
    int tabWidth = tabCount > 0 ? std::min(m_tabWidth, totalTabsWidth / tabCount) : m_tabWidth;
    tabWidth = std::max(tabWidth, m_minTabWidth);
    
    // Position tabs
    int x = 0;
    for (size_t i = 0; i < m_tabs.size(); ++i) {
        m_tabRects[i] = CRect(x, 0, x + tabWidth, m_height);
        x += tabWidth;
    }
    
    // Position new tab button
    m_newTabRect = CRect(x, 0, x + m_newTabButtonWidth, m_height);
}

CRect TabControl::GetTabRect(int index) const {
    if (index >= 0 && index < static_cast<int>(m_tabRects.size())) {
        return m_tabRects[index];
    }
    return CRect();
}

CRect TabControl::GetNewTabButtonRect() const {
    return m_newTabRect;
}

CRect TabControl::GetCloseButtonRect(int tabIndex) const {
    if (tabIndex < 0 || tabIndex >= static_cast<int>(m_tabRects.size())) {
        return CRect();
    }
    
    CRect tabRect = m_tabRects[tabIndex];
    int padding = (m_height - m_closeButtonSize) / 2;
    return CRect(
        tabRect.right - m_closeButtonSize - 8,
        padding,
        tabRect.right - 8,
        padding + m_closeButtonSize
    );
}

} // namespace Console3::UI
