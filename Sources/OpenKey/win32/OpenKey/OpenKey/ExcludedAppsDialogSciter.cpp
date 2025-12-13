/*----------------------------------------------------------
OpenKey - The Cross platform Open source Vietnamese Keyboard application.

Copyright (C) 2019 Mai Vu Tuyen
Contact: maivutuyen.91@gmail.com
Github: https://github.com/tuyenvm/OpenKey
Fanpage: https://www.facebook.com/OpenKeyVN

This file is belong to the OpenKey project, Win32 version
which is released under GPL license.
You can fork, modify, improve this program. If you
redistribute your new version, it MUST be open source.
-----------------------------------------------------------*/
#include "ExcludedAppsDialogSciter.h"
#include "stdafx.h"
#include "OpenKeyHelper.h"
#include <dwmapi.h>
#include <CommCtrl.h>
#include <windowsx.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "comctl32.lib")

// OCR_NORMAL is the ID for the standard arrow cursor
#ifndef OCR_NORMAL
#define OCR_NORMAL 32512
#endif

// External engine functions
extern void getAllEnglishOnlyApps(std::vector<std::string>& apps);
extern void addEnglishOnlyApp(const std::string& bundleId);
extern void removeEnglishOnlyApp(const std::string& bundleId);
extern bool isEnglishOnlyApp(const std::string& bundleId);
extern void initEnglishOnlyApps(const Byte* pData, const int& size);
extern void saveEnglishOnlyAppsData();

// Helper function to convert UTF-8 to wide string
extern std::wstring utf8ToWideString(const std::string& utf8str);
// Helper function to convert wide string to UTF-8
extern std::string wideStringToUtf8(const std::wstring& wstr);

// Helper function to force window to foreground (works in subprocess)
static void forceForegroundWindow(HWND hwnd) {
    HWND hForeground = GetForegroundWindow();
    if (hForeground == hwnd) return;  // Already foreground
    
    DWORD dwCurrentThread = GetCurrentThreadId();
    DWORD dwForegroundThread = GetWindowThreadProcessId(hForeground, NULL);
    
    // Attach to foreground thread to bypass Windows restriction
    if (dwCurrentThread != dwForegroundThread) {
        AttachThreadInput(dwCurrentThread, dwForegroundThread, TRUE);
    }
    
    // Now we can set foreground
    SetForegroundWindow(hwnd);
    BringWindowToTop(hwnd);
    SetActiveWindow(hwnd);
    
    // Detach
    if (dwCurrentThread != dwForegroundThread) {
        AttachThreadInput(dwCurrentThread, dwForegroundThread, FALSE);
    }
    
    // Force repaint
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);
}

// Acrylic blur structures
struct ACCENT_POLICY_EXCL {
    int AccentState;
    int AccentFlags;
    int GradientColor;
    int AnimationId;
};

struct WINDOWCOMPOSITIONATTRIBDATA_EXCL {
    int Attrib;
    void* pvData;
    size_t cbData;
};

enum ACCENT_STATE_EXCL {
    ACCENT_DISABLED_EXCL = 0,
    ACCENT_ENABLE_GRADIENT_EXCL = 1,
    ACCENT_ENABLE_TRANSPARENTGRADIENT_EXCL = 2,
    ACCENT_ENABLE_BLURBEHIND_EXCL = 3,
    ACCENT_ENABLE_ACRYLICBLURBEHIND_EXCL = 4,
    ACCENT_ENABLE_HOSTBACKDROP_EXCL = 5
};

ExcludedAppsDialogSciter::ExcludedAppsDialogSciter() 
    : sciter::window(SW_POPUP | SW_ALPHA | SW_ENABLE_DEBUG, RECT{ 0, 0, 400, 500 }) {
    
    // Load English-only apps data from registry (subprocess starts fresh)
    DWORD dataSize = 0;
    BYTE* data = OpenKeyHelper::getRegBinary(_T("englishOnlyApps"), dataSize);
    if (data && dataSize > 0) {
        initEnglishOnlyApps(data, (int)dataSize);
        // Do NOT delete[] data - it's managed by OpenKeyHelper
    }
    
    // Load HTML
#ifdef NDEBUG
    // Release: load from embedded resources (packed by packfolder.exe)
    if (!load(WSTR("this://app/excludedapps/excludedapps.html"))) {
        MessageBoxW(NULL, L"Failed to load excludedapps.html from resources", L"Error", MB_OK | MB_ICONERROR);
        return;
    }
#else
    // Debug: load from file (allows hot-reload during development)
    WCHAR exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    WCHAR* lastSlash = wcsrchr(exePath, L'\\');
    if (lastSlash) *lastSlash = L'\0';
    
    WCHAR htmlPath[MAX_PATH];
    swprintf_s(htmlPath, MAX_PATH, L"%s\\Resources\\Sciter\\excludedapps\\excludedapps.html", exePath);
    
    if (!load(htmlPath)) {
        MessageBoxW(NULL, htmlPath, L"Failed to load excludedapps.html", MB_OK | MB_ICONERROR);
        return;
    }
#endif
    
    // Show the window
    expand();
    
    // Set window title (Unicode escape for Vietnamese)
    // "Loại trừ ứng dụng" = "Lo\u1EA1i tr\u1EEB \u1EE9ng d\u1EE5ng"
    SetWindowTextW(get_hwnd(), L"Lo\u1EA1i tr\u1EEB \u1EE9ng d\u1EE5ng");
    
    // Fixed window size (matches CSS container: 380x450)
    SetWindowPos(get_hwnd(), NULL, 0, 0, 380, 450, SWP_NOMOVE | SWP_NOZORDER);
    
    // Center window on screen
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    RECT rc;
    GetWindowRect(get_hwnd(), &rc);
    int winWidth = rc.right - rc.left;
    int winHeight = rc.bottom - rc.top;
    int x = (screenWidth - winWidth) / 2;
    int y = (screenHeight - winHeight) / 2;
    SetWindowPos(get_hwnd(), HWND_NOTOPMOST, x, y, 0, 0, SWP_NOSIZE);
    
    // Enable blur effect
    enableAcrylicEffect();
    
    // Subclass for WM_NCHITTEST (drag) and WM_CLOSE
    SetWindowSubclass(get_hwnd(), ExcludedAppsDialogSciter::SubclassProc, 1, (DWORD_PTR)this);
}

ExcludedAppsDialogSciter::~ExcludedAppsDialogSciter() {
}

void ExcludedAppsDialogSciter::show() {
    ShowWindow(get_hwnd(), SW_SHOW);
    SetForegroundWindow(get_hwnd());
}

void ExcludedAppsDialogSciter::enableAcrylicEffect() {
    HWND hwnd = get_hwnd();

    // 1. CRITICAL: Set WS_EX_LAYERED style first
    SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);

    // 2. Try Acrylic (Windows 10 1803+)
    HMODULE hUser = GetModuleHandle(L"user32.dll");
    if (hUser) {
        typedef BOOL(WINAPI* pSetWindowCompositionAttribute)(HWND, WINDOWCOMPOSITIONATTRIBDATA_EXCL*);
        auto SetWindowCompositionAttribute = 
            (pSetWindowCompositionAttribute)GetProcAddress(hUser, "SetWindowCompositionAttribute");

        if (SetWindowCompositionAttribute) {
            ACCENT_POLICY_EXCL policy = { 0 };
            policy.AccentState = ACCENT_ENABLE_ACRYLICBLURBEHIND_EXCL;
            policy.AccentFlags = 0;
            policy.GradientColor = 0x80FFFFFF;  // ABGR: 50% white tint for visible blur
            policy.AnimationId = 0;

            WINDOWCOMPOSITIONATTRIBDATA_EXCL data = { 0 };
            data.Attrib = 19;  // WCA_ACCENT_POLICY
            data.pvData = &policy;
            data.cbData = sizeof(policy);

            SetWindowCompositionAttribute(hwnd, &data);
        }
        else {
            // Fallback to DWM Blur
            DWM_BLURBEHIND bb = { 0 };
            bb.dwFlags = DWM_BB_ENABLE;
            bb.fEnable = TRUE;
            bb.hRgnBlur = NULL;
            DwmEnableBlurBehindWindow(hwnd, &bb);
        }
    }

    // 3. Fix corners on Windows 11
    typedef enum {
        DWMWCP_DEFAULT_EXCL = 0,
        DWMWCP_DONOTROUND_EXCL = 1,
        DWMWCP_ROUND_EXCL = 2,
        DWMWCP_ROUNDSMALL_EXCL = 3
    } DWM_WINDOW_CORNER_PREFERENCE_EXCL;

    DWM_WINDOW_CORNER_PREFERENCE_EXCL preference = DWMWCP_ROUND_EXCL;
    DwmSetWindowAttribute(hwnd, 33, &preference, sizeof(preference));
}

LRESULT CALLBACK ExcludedAppsDialogSciter::SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    ExcludedAppsDialogSciter* dialog = reinterpret_cast<ExcludedAppsDialogSciter*>(dwRefData);
    
    if (msg == WM_CLOSE) {
        ExitProcess(0);  // Force exit subprocess
        return 0;
    }
    
    // Window Picker: Handle mouse click to capture window
    if (msg == WM_LBUTTONUP && dialog && dialog->m_isPickingWindow) {
        POINT pt;
        GetCursorPos(&pt);
        HWND targetWnd = WindowFromPoint(pt);
        
        // Get root window (not child control)
        if (targetWnd) {
            targetWnd = GetAncestor(targetWnd, GA_ROOT);
        }
        
        if (targetWnd && targetWnd != hwnd) {
            std::string exeName = dialog->getExeNameFromWindow(targetWnd);
            if (!exeName.empty()) {
                dialog->stopWindowPicking();
                dialog->onAddPickedApp(exeName);
                return 0;
            }
        }
        dialog->stopWindowPicking();
        return 0;
    }
    
    // Window Picker: ESC to cancel
    if (msg == WM_KEYDOWN && wParam == VK_ESCAPE && dialog && dialog->m_isPickingWindow) {
        dialog->stopWindowPicking();
        return 0;
    }
    
    if (msg == WM_NCHITTEST) {
        LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
        if (result == HTCLIENT) {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(hwnd, &pt);
            
            // Drag zone: title bar height (40px) for easy dragging
            // CRITICAL: Exclude close button area (last 40px on right side)
            RECT winRect;
            GetClientRect(hwnd, &winRect);
            int closeButtonZone = winRect.right - 40;
            
            if (pt.y < 40 && pt.x < closeButtonZone) {
                return HTCAPTION;
            }
        }
        return result;
    }
    
    // Window Picker: Show crosshair cursor continuously during picking
    if (msg == WM_SETCURSOR && dialog && dialog->m_isPickingWindow) {
        SetCursor(LoadCursor(NULL, IDC_CROSS));
        return TRUE;  // Prevent default cursor
    }
    
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

bool ExcludedAppsDialogSciter::handle_event(HELEMENT he, BEHAVIOR_EVENT_PARAMS& params) {
    // Handle DOCUMENT_READY to populate apps list
    if (params.cmd == DOCUMENT_READY) {
        fillAppsList();
        return true;
    }
    
    // Handle VALUE_CHANGED for hidden inputs (JS sets these, C++ reads them)
    if (params.cmd == VALUE_CHANGED) {
        sciter::dom::element el(params.heTarget);
        std::wstring id = el.get_attribute("id");
        
        if (id == L"val-action") {
            sciter::dom::element root = this->root();
            sciter::value actionVal = el.get_value();
            std::wstring action = actionVal.is_string() ? actionVal.get<std::wstring>() : L"";
            
            if (action == L"add-manual") {
                sciter::dom::element nameEl = root.find_first("#val-app-name");
                
                if (nameEl.is_valid()) {
                    sciter::value nameVal = nameEl.get_value();
                    std::wstring name = nameVal.is_string() ? nameVal.get<std::wstring>() : L"";
                    
                    if (!name.empty()) {
                        onAddManual(name);
                    }
                }
                // Clear action
                el.set_value(sciter::value(L""));
                return true;
            }
            
            if (action == L"add-current") {
                startWindowPicking();  // Start window picker mode
                el.set_value(sciter::value(L""));
                return true;
            }
            
            if (action == L"delete") {
                sciter::dom::element nameEl = root.find_first("#val-app-name");
                if (nameEl.is_valid()) {
                    sciter::value nameVal = nameEl.get_value();
                    std::wstring name = nameVal.is_string() ? nameVal.get<std::wstring>() : L"";
                    
                    if (!name.empty()) {
                        onDeleteApp(name);
                    }
                }
                el.set_value(sciter::value(L""));
                return true;
            }
            
            if (action == L"close") {
                PostMessage(get_hwnd(), WM_CLOSE, 0, 0);
                return true;
            }
        }
    }
    
    // Handle button clicks - only for btn-close (direct click without JS)
    if (params.cmd == BUTTON_CLICK) {
        sciter::dom::element el(params.heTarget);
        std::wstring id = el.get_attribute("id");
        
        if (id == L"btn-close") {
            PostMessage(get_hwnd(), WM_CLOSE, 0, 0);
            return true;
        }
    }
    
    // Handle app item click
    if (params.cmd == HYPERLINK_CLICK || params.cmd == BUTTON_CLICK) {
        sciter::dom::element el(params.heTarget);
        std::wstring className = el.get_attribute("class");
        
        if (className.find(L"app-item") != std::wstring::npos) {
            // Get app data from attributes
            std::wstring name = el.get_attribute("data-name");
            
            // Fill input field
            sciter::dom::element root = this->root();
            sciter::dom::element nameEl = root.find_first("#app-name");
            
            if (nameEl.is_valid()) nameEl.set_value(sciter::value(name));
            
            // Update selection
            sciter::dom::element list = root.find_first("#app-list");
            if (list.is_valid()) {
                for (int i = 0; i < (int)list.children_count(); i++) {
                    sciter::dom::element item = list.child(i);
                    item.set_attribute("class", L"app-item");
                }
                el.set_attribute("class", L"app-item selected");
            }
            return true;
        }
    }
    
    return false;
}

void ExcludedAppsDialogSciter::fillAppsList() {
    // Get all excluded apps from engine
    m_appsList.clear();
    getAllEnglishOnlyApps(m_appsList);
    
    // Call JS function to clear list
    call_function("clearAppList");
    
    // Add items via JS
    for (size_t i = 0; i < m_appsList.size(); i++) {
        // Convert UTF-8 to wide string for proper display in Sciter
        std::wstring wName = utf8ToWideString(m_appsList[i]);
        call_function("addAppToList", wName.c_str());
    }
    
    // Force Sciter layout recalculation
    call_function("forceRefresh");
    
    // Process pending messages to force Sciter to render immediately
    MSG msg;
    while (PeekMessage(&msg, get_hwnd(), 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // Force repaint
    InvalidateRect(get_hwnd(), NULL, TRUE);
    UpdateWindow(get_hwnd());
}

void ExcludedAppsDialogSciter::saveAndReload() {
    // Save to registry
    saveEnglishOnlyAppsData();
    
    // Notify main process to reload
    HWND mainWnd = FindWindow(_T("OpenKeyVietnameseInputMethod"), NULL);
    if (mainWnd) {
        PostMessage(mainWnd, WM_USER + 101, 0, 0);
    }
    
    // Reload list
    fillAppsList();
}

void ExcludedAppsDialogSciter::onAddManual(const std::wstring& appName) {
    std::string utf8Name = wideStringToUtf8(appName);
    
    if (isEnglishOnlyApp(utf8Name)) {
        // Already exists - use Unicode escape for Vietnamese
        // "Ứng dụng này đã có trong danh sách!"
        MessageBoxW(get_hwnd(), 
            L"\u1EE8ng d\u1EE5ng n\u00E0y \u0111\u00E3 c\u00F3 trong danh s\u00E1ch!", 
            L"OpenKey", 
            MB_OK | MB_ICONINFORMATION);
        return;
    }
    
    addEnglishOnlyApp(utf8Name);
    
    // Save to registry and notify main
    saveEnglishOnlyAppsData();
    HWND mainWnd = FindWindow(_T("OpenKeyVietnameseInputMethod"), NULL);
    if (mainWnd) {
        PostMessage(mainWnd, WM_USER + 101, 0, 0);
    }
    
    // Only add the new item (incremental update - much faster!)
    call_function("addAppToList", appName.c_str());
    call_function("forceRefresh");
    
    // Force window to repaint immediately (for side-by-side window scenario)
    forceForegroundWindow(get_hwnd());
}

void ExcludedAppsDialogSciter::onAddCurrentApp() {
    std::string& currentApp = OpenKeyHelper::getFrontMostAppExecuteName();
    
    if (currentApp.compare("OpenKey64.exe") == 0 || currentApp.compare("OpenKey32.exe") == 0) {
        // "Không thể thêm OpenKey vào danh sách loại trừ!"
        MessageBoxW(get_hwnd(), 
            L"Kh\u00F4ng th\u1EC3 th\u00EAm OpenKey v\u00E0o danh s\u00E1ch lo\u1EA1i tr\u1EEB!", 
            L"OpenKey", 
            MB_OK | MB_ICONWARNING);
        return;
    }
    
    if (isEnglishOnlyApp(currentApp)) {
        // "Ứng dụng này đã có trong danh sách!"
        MessageBoxW(get_hwnd(), 
            L"\u1EE8ng d\u1EE5ng n\u00E0y \u0111\u00E3 c\u00F3 trong danh s\u00E1ch!", 
            L"OpenKey", 
            MB_OK | MB_ICONINFORMATION);
        return;
    }
    
    addEnglishOnlyApp(currentApp);
    
    // Save to registry and notify main
    saveEnglishOnlyAppsData();
    HWND mainWnd = FindWindow(_T("OpenKeyVietnameseInputMethod"), NULL);
    if (mainWnd) {
        PostMessage(mainWnd, WM_USER + 101, 0, 0);
    }
    
    // Only add the new item (incremental update - much faster!)
    std::wstring wName = utf8ToWideString(currentApp);
    call_function("addAppToList", wName.c_str());
    call_function("forceRefresh");
    
    // Force window to repaint immediately (for side-by-side window scenario)
    forceForegroundWindow(get_hwnd());
}

void ExcludedAppsDialogSciter::onDeleteApp(const std::wstring& appName) {
    std::string utf8Name = wideStringToUtf8(appName);
    removeEnglishOnlyApp(utf8Name);
    
    // Save to registry and notify main
    saveEnglishOnlyAppsData();
    HWND mainWnd = FindWindow(_T("OpenKeyVietnameseInputMethod"), NULL);
    if (mainWnd) {
        PostMessage(mainWnd, WM_USER + 101, 0, 0);
    }
    
    // Only remove the deleted item (incremental update - much faster!)
    call_function("removeAppFromList", appName.c_str());
    call_function("forceRefresh");
}

// ===== Window Picker Implementation =====

void ExcludedAppsDialogSciter::startWindowPicking() {
    m_isPickingWindow = true;
    
    // Capture mouse to receive events outside our window
    SetCapture(get_hwnd());
    
    // IMPORTANT: Save original arrow cursor BEFORE replacing
    HCURSOR hOriginalArrow = LoadCursor(NULL, IDC_ARROW);
    m_hSavedArrowCursor = CopyCursor(hOriginalArrow);  // Make a copy to restore later
    
    // Replace arrow cursor with crosshair globally
    HCURSOR hCross = LoadCursor(NULL, IDC_CROSS);
    SetSystemCursor(CopyCursor(hCross), OCR_NORMAL);
}

void ExcludedAppsDialogSciter::stopWindowPicking() {
    m_isPickingWindow = false;
    
    // Release mouse capture
    ReleaseCapture();
    
    // Restore original arrow cursor directly (FAST - no registry reload!)
    if (m_hSavedArrowCursor) {
        SetSystemCursor(m_hSavedArrowCursor, OCR_NORMAL);
        m_hSavedArrowCursor = NULL;  // SetSystemCursor destroys the handle
    }
    
    // Bring our window back to front
    forceForegroundWindow(get_hwnd());
}

std::string ExcludedAppsDialogSciter::getExeNameFromWindow(HWND hwnd) {
    if (!hwnd) return "";
    
    DWORD processId = 0;
    GetWindowThreadProcessId(hwnd, &processId);
    if (processId == 0) return "";
    
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (!hProcess) return "";
    
    WCHAR exePath[MAX_PATH] = {0};
    GetProcessImageFileNameW(hProcess, exePath, MAX_PATH);
    CloseHandle(hProcess);
    
    if (wcslen(exePath) == 0) return "";
    
    // Extract filename from full path (path uses \Device\Harddisk... format)
    WCHAR* filename = wcsrchr(exePath, L'\\');
    if (filename) filename++;
    else filename = exePath;
    
    // Convert to UTF-8
    return wideStringToUtf8(filename);
}

void ExcludedAppsDialogSciter::onAddPickedApp(const std::string& exeName) {
    // Check if it's OpenKey itself
    if (exeName.compare("OpenKey64.exe") == 0 || exeName.compare("OpenKey32.exe") == 0) {
        // "Không thể thêm OpenKey vào danh sách loại trừ!"
        MessageBoxW(get_hwnd(), 
            L"Kh\u00F4ng th\u1EC3 th\u00EAm OpenKey v\u00E0o danh s\u00E1ch lo\u1EA1i tr\u1EEB!", 
            L"OpenKey", 
            MB_OK | MB_ICONWARNING);
        return;
    }
    
    // Check if already exists
    if (isEnglishOnlyApp(exeName)) {
        // "Ứng dụng này đã có trong danh sách!"
        MessageBoxW(get_hwnd(), 
            L"\u1EE8ng d\u1EE5ng n\u00E0y \u0111\u00E3 c\u00F3 trong danh s\u00E1ch!", 
            L"OpenKey", 
            MB_OK | MB_ICONINFORMATION);
        return;
    }
    
    addEnglishOnlyApp(exeName);
    
    // Save to registry and notify main
    saveEnglishOnlyAppsData();
    HWND mainWnd = FindWindow(_T("OpenKeyVietnameseInputMethod"), NULL);
    if (mainWnd) {
        PostMessage(mainWnd, WM_USER + 101, 0, 0);
    }
    
    // Only add the new item (incremental update - much faster!)
    std::wstring wName = utf8ToWideString(exeName);
    call_function("addAppToList", wName.c_str());
    call_function("clearInput");  // Clear textbox after adding
    call_function("forceRefresh");
    
    // Force window to repaint immediately (for side-by-side window scenario)
    forceForegroundWindow(get_hwnd());
}
