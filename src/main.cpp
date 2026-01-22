// Console3 - main.cpp
// Application entry point
//
// Initializes WTL, COM, and the main application window.

// Target Windows 10 RS5 (1809) or later
#ifndef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_WIN10_RS5
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_WIN10
#endif
#ifndef WINVER
#define WINVER _WIN32_WINNT_WIN10
#endif

// ATL/WTL configuration
#define STRICT
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>
#include <shellapi.h>
#include <commctrl.h>

// ATL headers
#include <atlbase.h>
#include <atlapp.h>

// WTL module declaration
CAppModule _Module;

#include <atlwin.h>
#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>

// WTL headers
#include <atlcrack.h>
#include <atlmisc.h>

// Direct2D/DirectWrite for rendering
#include <d2d1_1.h>
#include <dwrite_1.h>
#include <wrl/client.h>

// Application headers
#include "UI/MainFrame.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

using Microsoft::WRL::ComPtr;

/// Initialize common controls
bool InitializeCommonControls() {
    INITCOMMONCONTROLSEX icc{};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_BAR_CLASSES | ICC_COOL_CLASSES | ICC_TAB_CLASSES;
    return InitCommonControlsEx(&icc) != FALSE;
}

/// Initialize Direct2D factory
ComPtr<ID2D1Factory1> InitializeDirect2D() {
    ComPtr<ID2D1Factory1> factory;
    
    D2D1_FACTORY_OPTIONS options{};
#ifdef _DEBUG
    options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#else
    options.debugLevel = D2D1_DEBUG_LEVEL_NONE;
#endif

    HRESULT hr = D2D1CreateFactory(
        D2D1_FACTORY_TYPE_SINGLE_THREADED,
        __uuidof(ID2D1Factory1),
        &options,
        reinterpret_cast<void**>(factory.GetAddressOf())
    );

    if (FAILED(hr)) {
        MessageBoxW(nullptr, L"Failed to initialize Direct2D.", L"Console3", MB_ICONERROR);
        return nullptr;
    }

    return factory;
}

/// Initialize DirectWrite factory
ComPtr<IDWriteFactory1> InitializeDirectWrite() {
    ComPtr<IDWriteFactory1> factory;
    
    HRESULT hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory1),
        reinterpret_cast<IUnknown**>(factory.GetAddressOf())
    );

    if (FAILED(hr)) {
        MessageBoxW(nullptr, L"Failed to initialize DirectWrite.", L"Console3", MB_ICONERROR);
        return nullptr;
    }

    return factory;
}

/// Application message loop
int RunMessageLoop() {
    CMessageLoop theLoop;
    _Module.AddMessageLoop(&theLoop);

    int nRet = theLoop.Run();

    _Module.RemoveMessageLoop();
    return nRet;
}

/// Application entry point
int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE /*hPrevInstance*/,
    _In_ LPWSTR /*lpCmdLine*/,
    _In_ int nShowCmd
) {
    // Initialize COM (required for Direct2D/DirectWrite)
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        MessageBoxW(nullptr, L"Failed to initialize COM.", L"Console3", MB_ICONERROR);
        return 1;
    }

    // Initialize ATL module
    hr = _Module.Init(nullptr, hInstance);
    if (FAILED(hr)) {
        CoUninitialize();
        return 1;
    }

    // Initialize common controls
    if (!InitializeCommonControls()) {
        _Module.Term();
        CoUninitialize();
        return 1;
    }

    // Initialize Direct2D
    auto d2dFactory = InitializeDirect2D();
    if (!d2dFactory) {
        _Module.Term();
        CoUninitialize();
        return 1;
    }

    // Initialize DirectWrite
    auto dwriteFactory = InitializeDirectWrite();
    if (!dwriteFactory) {
        _Module.Term();
        CoUninitialize();
        return 1;
    }

    int nRet = 0;
    {
        // Create and show the main window
        Console3::UI::MainFrame mainFrame;
        
        // Pass factories to the main frame
        mainFrame.SetD2DFactory(d2dFactory.Get());
        mainFrame.SetDWriteFactory(dwriteFactory.Get());

        // Create the window
        if (mainFrame.CreateEx() == nullptr) {
            MessageBoxW(nullptr, L"Failed to create main window.", L"Console3", MB_ICONERROR);
            nRet = 1;
        } else {
            // Show the window
            mainFrame.ShowWindow(nShowCmd);
            mainFrame.UpdateWindow();

            // Run the message loop
            nRet = RunMessageLoop();
        }
    }

    // Cleanup
    _Module.Term();
    CoUninitialize();

    return nRet;
}
