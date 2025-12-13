# Sciter Dialog Creation Checklist

> **Purpose**: Complete guide to create a new Sciter dialog with blur effect, drag, close button, and proper layout.

## Quick Checklist

- [ ] C++: Create `DialogName.h/.cpp` inheriting `sciter::window`
- [ ] C++: Add `--dialogname` router in `main.cpp` (with single-instance mutex)
- [ ] C++: Add spawn code in `AppDelegate.cpp` (store process handle for cleanup)
- [ ] C++: If spawning from another subprocess, use IPC message to main process
- [ ] C++: Window title uses Unicode escape sequences for Vietnamese
- [ ] HTML/CSS/JS: Create `Resources/Sciter/dialogname/`
- [ ] CSS: Use fixed height + `overflow-y: auto` for scrollable lists
- [ ] C++: SubclassProc excludes close button (40px right) from drag zone
- [ ] Copy files to `x64/Debug/Resources/Sciter/dialogname/` for testing

---

## 1. C++ Header File

```cpp
// DialogName.h
#pragma once
#include "sciter-x.h"
#include "sciter-x-window.hpp"
#include <string>

class DialogName : public sciter::window {
public:
    DialogName();
    virtual ~DialogName() {}
    
    virtual bool handle_event(HELEMENT he, BEHAVIOR_EVENT_PARAMS& params) override;
    void show();
    
    static LRESULT CALLBACK SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, 
                                         LPARAM lParam, UINT_PTR uIdSubclass, 
                                         DWORD_PTR dwRefData);
private:
    void enableAcrylicEffect();
};
```

---

## 2. C++ Implementation

### Constructor Pattern

```cpp
DialogName::DialogName() 
    : sciter::window(SW_POPUP | SW_ALPHA | SW_ENABLE_DEBUG, RECT{ 0, 0, 400, 500 }) {
    
    // CRITICAL: Subprocess starts fresh - must load settings from registry!
    // Without this, all toggles will show default values
    APP_GET_DATA(vLanguage, 1);          // Default: Vietnamese
    APP_GET_DATA(vInputType, 0);         // Default: Telex
    APP_GET_DATA(vMyToggleSetting, 0);   // Each toggle needs its own APP_GET_DATA
    // ... add all settings that UI toggles use
    
    // Load HTML relative to exe path
    wchar_t exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);
    PathRemoveFileSpec(exePath);
    wcscat_s(exePath, L"\\Resources\\Sciter\\dialogname\\dialog.html");
    
    if (!load(exePath)) {
        MessageBox(NULL, L"Failed to load HTML", L"Error", MB_OK);
        return;
    }
    
    expand();  // Show window
    
    // Set title for FindWindow() single-instance check
    SetWindowTextW(get_hwnd(), L"Dialog Title");
    
    // Fixed layout: Set exact window size
    SetWindowPos(get_hwnd(), NULL, 0, 0, 380, 450, SWP_NOMOVE | SWP_NOZORDER);
    
    // OR Auto-fit: Measure content
    // sciter::dom::element container = root().find_first(".container");
    // if (container) {
    //     RECT r = container.get_location(CONTENT_BOX);
    //     SetWindowPos(get_hwnd(), NULL, 0, 0, r.right-r.left, r.bottom-r.top, SWP_NOMOVE | SWP_NOZORDER);
    // }
    
    // Center on screen
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    RECT rc; GetWindowRect(get_hwnd(), &rc);
    int x = (screenW - (rc.right - rc.left)) / 2;
    int y = (screenH - (rc.bottom - rc.top)) / 2;
    SetWindowPos(get_hwnd(), HWND_NOTOPMOST, x, y, 0, 0, SWP_NOSIZE);
    
    // Enable blur effect
    enableAcrylicEffect();
    
    // Subclass for drag + close
    SetWindowSubclass(get_hwnd(), SubclassProc, 1, (DWORD_PTR)this);
}
```

> [!IMPORTANT]
> **Subprocesses start with empty global variables!** You MUST call `APP_GET_DATA` for every registry setting that your UI toggles display. Without this, all toggles will show default values instead of the actual saved settings.

### Blur Effect

```cpp
void DialogName::enableAcrylicEffect() {
    HWND hwnd = get_hwnd();
    SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
    
    // Acrylic blur on Windows 10/11
    typedef BOOL(WINAPI* pSetWindowCompositionAttribute)(HWND, void*);
    HMODULE hUser32 = LoadLibrary(L"user32.dll");
    if (hUser32) {
        auto SetWindowCompositionAttribute = 
            (pSetWindowCompositionAttribute)GetProcAddress(hUser32, "SetWindowCompositionAttribute");
        
        if (SetWindowCompositionAttribute) {
            struct { int state; DWORD color; int flags; } policy = { 4, 0x80FFFFFF, 0 };
            struct { int attrib; void* data; size_t size; } data = { 19, &policy, sizeof(policy) };
            SetWindowCompositionAttribute(hwnd, &data);
        }
    }
    
    // Round corners on Windows 11
    int preference = 2;  // DWMWCP_ROUND
    DwmSetWindowAttribute(hwnd, 33, &preference, sizeof(preference));
}
```

### Subclass for Drag and Close

> [!IMPORTANT]
> **Close button must be excluded from drag zone!** The 40px on the right side of title bar is reserved for the close button. Without this exclusion, clicking close button will start dragging instead.

```cpp
LRESULT CALLBACK DialogName::SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, 
                                           LPARAM lParam, UINT_PTR uIdSubclass, 
                                           DWORD_PTR dwRefData) {
    if (msg == WM_CLOSE) {
        ExitProcess(0);  // Force exit subprocess
        return 0;
    }
    
    if (msg == WM_NCHITTEST) {
        LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
        if (result == HTCLIENT) {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(hwnd, &pt);
            
            RECT winRect; GetClientRect(hwnd, &winRect);
            int closeButtonZone = winRect.right - 40;  // CRITICAL: Exclude close button area
            
            if (pt.y < 40 && pt.x < closeButtonZone) {
                return HTCAPTION;  // Enable drag in title bar (except close button)
            }
        }
        return result;
    }
    
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}
```

### Handle Events (Close Button)

```cpp
bool DialogName::handle_event(HELEMENT he, BEHAVIOR_EVENT_PARAMS& params) {
    if (params.cmd == DOCUMENT_READY) {
        // Initialize UI here
        return true;
    }
    
    if (params.cmd == BUTTON_CLICK) {
        sciter::dom::element el(params.heTarget);
        auto id = el.get_attribute("id");
        
        if (id == L"btn-close") {
            PostMessage(get_hwnd(), WM_CLOSE, 0, 0);
            return true;
        }
    }
    
    return false;
}
```

---

## 3. main.cpp Router (with Template Helper Functions)

**Use template functions to avoid code duplication:**

```cpp
// Helper: Run a sciter dialog with single-instance mutex protection
template<typename DialogType>
int runSingleInstanceDialog(const wchar_t* mutexName, const wchar_t* windowTitle) {
    HANDLE hMutex = CreateMutexW(NULL, TRUE, mutexName);
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND existingWnd = FindWindowW(NULL, windowTitle);
        if (existingWnd) SetForegroundWindow(existingWnd);
        CloseHandle(hMutex);
        return 0;
    }
    
    SciterSetOption(NULL, SCITER_SET_DEBUG_MODE, TRUE);
    SciterSetOption(NULL, SCITER_SET_SCRIPT_RUNTIME_FEATURES,
        ALLOW_FILE_IO | ALLOW_SOCKET_IO | ALLOW_EVAL | ALLOW_SYSINFO);
    
    DialogType dialog;
    dialog.show();
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

// Helper: Run simple dialog without mutex
template<typename DialogType>
int runSimpleDialog() {
    DialogType dialog;
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
```

**Usage in wWinMain:**
```cpp
// Simple dialogs (no mutex needed)
if (lpCmdLine && wcsstr(lpCmdLine, L"--about")) {
    return runSimpleDialog<AboutDialog>();
}

// Dialogs with single-instance protection
if (lpCmdLine && wcsstr(lpCmdLine, L"--macro")) {
    return runSingleInstanceDialog<MacroDialogSciter>(
        L"OpenKeyMacroDialogMutex", 
        L"Bảng gõ tắt"
    );
}
```

> [!IMPORTANT]
> **Use Named Mutex, not just FindWindow!** FindWindow can fail due to timing - user may click faster than window creation. Mutex is created instantly.

---

## 4. AppDelegate.cpp - Spawn Subprocess with Process Handle Management

> [!IMPORTANT]
> **Store process handles for cleanup!** Instead of using `FindWindow` by title (requires Unicode escapes, error-prone), store process handles when spawning and use `TerminateProcess` when main app exits.

### Add to AppDelegate.h:

```cpp
#include <vector>

class AppDelegate {
private:
    std::vector<HANDLE> m_childProcesses;  // Track subprocess handles
    
public:
    void trackChildProcess(HANDLE hProcess);
    void terminateAllChildren();  // Call in onOpenKeyExit()
};
```

### Add helper methods to AppDelegate.cpp:

```cpp
void AppDelegate::trackChildProcess(HANDLE hProcess) {
    if (hProcess) {
        m_childProcesses.push_back(hProcess);
    }
}

void AppDelegate::terminateAllChildren() {
    for (HANDLE h : m_childProcesses) {
        if (h) {
            TerminateProcess(h, 0);
            CloseHandle(h);
        }
    }
    m_childProcesses.clear();
}
```

### Spawn subprocess (store handle):

```cpp
void AppDelegate::openDialogName() {
    // Optional: Anti-spam with FindWindow
    HWND existingWnd = FindWindowW(NULL, L"Dialog Title");
    if (existingWnd) {
        SetForegroundWindow(existingWnd);
        return;
    }
    
    wchar_t exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);
    
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    
    wchar_t cmdLine[MAX_PATH + 20];
    swprintf_s(cmdLine, L"\"%s\" --dialogname", exePath);
    
    if (CreateProcess(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hThread);
        trackChildProcess(pi.hProcess);  // Store handle for cleanup
    }
}
```

### Cleanup on exit:

```cpp
void AppDelegate::onOpenKeyExit() {
    // Close all subprocess dialogs at once - no FindWindow needed!
    terminateAllChildren();
    
    OpenKeyManager::freeEngine();
    SystemTrayHelper::removeSystemTray();
    PostQuitMessage(0);
}
```

> [!TIP]
> With this pattern, adding a new dialog only requires calling `trackChildProcess()` when spawning. No need to update `onOpenKeyExit()` with new window titles!

### IPC Message Routing (Subprocess → Main Process)

> [!IMPORTANT]
> **Subprocesses cannot track handles!** If a subprocess (e.g., Settings dialog) spawns another subprocess (e.g., Macro dialog), it cannot track the handle because it runs in a different process. Always route spawn requests to main process via IPC messages.

**Problem:** Settings dialog button spawns Macro dialog directly:
```cpp
// BAD - subprocess loses handle, cannot be terminated on exit
if (id == L"btn-macro") {
    if (CreateProcessW(..., &si, &pi)) {
        CloseHandle(pi.hProcess);  // LOST - can't terminate later!
        CloseHandle(pi.hThread);
    }
}
```

**Solution:** Send message to main process, let it spawn and track:

1. **Define message in SystemTrayHelper.cpp:**
```cpp
// In WndProc switch statement
case WM_USER+103:  // Macro table
    AppDelegate::getInstance()->onMacroTable();
    break;
case WM_USER+104:  // Excluded apps
    AppDelegate::getInstance()->onSpawnExcludedAppsSciter();
    break;
```

2. **Send message from subprocess (SettingsDialog.cpp):**
```cpp
if (id == L"btn-macro-table") {
    // Route to main process - it will track the handle
    HWND mainWnd = FindWindow(_T("OpenKeyVietnameseInputMethod"), NULL);
    if (mainWnd) {
        PostMessage(mainWnd, WM_USER + 103, 0, 0);
    }
    return true;
}
```

3. **Main process spawns with handle tracking (AppDelegate.cpp):**
```cpp
void AppDelegate::onMacroTable() {
    // ... anti-spam check ...
    if (CreateProcessW(..., &si, &pi)) {
        CloseHandle(pi.hThread);
        trackChildProcess(pi.hProcess);  // ✓ Tracked!
    }
}
```

**Message ID Convention:**
| Message | Purpose |
|---------|---------|
| `WM_USER+101` | Reload settings from registry |
| `WM_USER+102` | Update UI from hotkey change |
| `WM_USER+103` | Spawn Macro dialog |
| `WM_USER+104` | Spawn Excluded Apps dialog |

> [!CAUTION]
> **WM_USER+101 must reload ALL settings that UI can change!** When subprocess sends `notifyMainProcess()`, the main process handler in `SystemTrayHelper.cpp` must call `APP_GET_DATA` for EVERY setting that any toggle or dropdown can modify. Missing settings = toggle changes have no effect!

**Example WM_USER+101 handler (SystemTrayHelper.cpp):**
```cpp
case WM_USER+101:
    // Reload ALL toggle settings from registry
    APP_GET_DATA(vLanguage, 1);
    APP_GET_DATA(vInputType, 0);
    APP_GET_DATA(vUseSmartSwitchKey, 0);
    APP_GET_DATA(vExcludeApps, 1);  // ← CRITICAL: Don't forget new toggles!
    // ... every other toggle setting ...
    
    // Reload data structures (macro, excluded apps list)
    {
        DWORD macroDataSize = 0;
        BYTE* macroData = OpenKeyHelper::getRegBinary(_T("macroData"), macroDataSize);
        if (macroData && macroDataSize > 0) {
            initMacroMap(macroData, (int)macroDataSize);
        }
    }
    
    SystemTrayHelper::updateData();
    break;
```

---

## Custom Tooltips in Sciter

> [!IMPORTANT]
> **Use `data-tooltip` attribute, NOT `title`!** The `title` attribute triggers both native tooltip AND CSS custom tooltip (double tooltip bug).

**CSS Pattern (in theme.css):**
```css
[data-tooltip] {
    position: relative;
}

[data-tooltip]:hover::after {
    content: attr(data-tooltip);
    position: absolute;
    top: calc(100% + 8px);  /* Below element, not above (avoids clipping) */
    right: 0;               /* Right-aligned to fit in narrow window */
    max-width: 200px;       /* Keep within window boundary */
    padding: 8px 12px;
    background: rgba(30, 30, 40, 0.92);
    color: #fff;
    font-size: 12px;
    border-radius: 8px;
    white-space: normal;
    z-index: 9999;
    pointer-events: none;
}
```

**HTML Usage:**
```html
<div class="toggle-switch-small" id="my-toggle" 
     data-tooltip="Mô tả chức năng của toggle này">
    ...
</div>
```

> [!CAUTION]  
> **Tooltip clipping issues:** Sciter windows clip content at window boundary. Solutions:
> - Use `right: 0` (not `left: 50%`) to prevent overflow on right side
> - Use `top: calc(100% + 8px)` (not `bottom:`) to show below element
> - Keep `max-width: 200px` or less for narrow dialogs (350px width)
> - Keep `.container { overflow: hidden }` (visible causes text blur issues)

---

## Text Clarity on Glass Background

> [!IMPORTANT]
> **Keep `overflow: hidden`!** Using `overflow: visible` on container causes text to appear blurry on acrylic/glass backgrounds.

**Best practices for sharp text:**
```css
* {
    font-family: var(--font-family);
    font-weight: 500;  /* Slightly bolder for glass backgrounds */
}
```

**AVOID these CSS properties on acrylic backgrounds:**
- `-webkit-font-smoothing: antialiased` - causes blur in Sciter
- `text-rendering: optimizeLegibility` - causes blur
- `text-shadow` on glass backgrounds - adds unwanted haze

---

## 5. HTML Template

```html
<!DOCTYPE html>
<html lang="vi">
<head>
    <meta charset="UTF-8">
    <title>Dialog Title</title>
    <link rel="stylesheet" href="dialog.css">
    <script src="dialog.js"></script>
</head>
<body>
    <div class="container" id="main-container">
        <!-- Title Bar with centered text -->
        <div class="title-bar">
            <span class="title-text">Dialog Title</span>
            <button class="btn-close" id="btn-close" title="Đóng">
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" 
                     width="24" height="24" fill="rgba(240,23,23,1)">
                    <path d="M12 22C6.47715 22 2 17.5228 2 12C2 6.47715 6.47715 2 12 2C17.5228 2 22 6.47715 22 12C22 17.5228 17.5228 22 12 22ZM12 10.5858L9.17157 7.75736L7.75736 9.17157L10.5858 12L7.75736 14.8284L9.17157 16.2426L12 13.4142L14.8284 16.2426L16.2426 14.8284L13.4142 12L16.2426 9.17157L14.8284 7.75736L12 10.5858Z"/>
                </svg>
            </button>
        </div>

        <!-- Content Section -->
        <div class="content-section">
            <!-- Your content here -->
        </div>
    </div>
</body>
</html>
```

---

## 6. CSS Template (Fixed Layout)

```css
* {
    margin: 0;
    padding: 0;
    box-sizing: border-box;
    font-family: 'Segoe UI', system-ui, sans-serif;
}

html, body {
    background: transparent;
    overflow: hidden;
    width: 100%;
    height: 100%;
}

/* Container - Fixed size, blur-friendly background */
.container {
    width: 380px;
    height: 450px;
    background: rgba(255, 255, 255, 0.75);  /* Semi-transparent for blur */
    border-radius: 8px;
    overflow: hidden;
}

/* Title Bar - Centered text, 36px height */
.title-bar {
    display: block;
    position: relative;
    width: 100%;
    height: 36px;
    line-height: 36px;
    text-align: center;   /* Center title text */
    background: rgba(0, 0, 0, 0.05);
    border-bottom: 1px solid rgba(0, 0, 0, 0.08);
}

.title-text {
    font-size: 16px;
    font-weight: 700;
    color: #007AFF;
}

/* Close Button - Absolute right */
.btn-close {
    position: absolute;
    right: 8px;
    top: 6px;
    width: 24px;
    height: 24px;
    border: none;
    background: none;
    cursor: pointer;
    border-radius: 50%;
}

.btn-close:hover {
    background: rgba(235, 33, 33, 0.15);
}

.btn-close svg {
    pointer-events: none;  /* Prevent SVG from capturing clicks */
}

/* Content Section - Height = container - title bar */
.content-section {
    height: 414px;  /* 450 - 36 */
    padding: 16px;
    overflow: hidden;
}

/* Scrollable List (if needed) */
.item-list {
    display: block;
    height: 150px;
    overflow-y: auto;
}

/* Input Field - Fixed height, NO TEXT JUMPING */
/* CRITICAL: !important overrides Sciter's std-edit from master-base.css (height: 1.4em) */
.setting-input {
    display: inline-block;
    box-sizing: border-box !important;
    height: 28px !important;
    min-height: 28px !important;
    max-height: 28px !important;
    padding: 0 8px !important;
    font-size: 13px;
    line-height: 26px !important;    /* height - 2px border */
    border: 1px solid rgba(0, 0, 0, 0.15);
    border-radius: 4px;
    outline: none !important;
    background: #fff;
    margin: 0;
    font-rendering-mode: snap-pixel;  /* Sciter: snap glyphs to pixel boundary */
    vertical-align: middle;
    overflow: hidden !important;
}

/* Focus: keep same border WIDTH to prevent layout shift */
.setting-input:focus {
    border: 1px solid #007AFF;
    outline: none !important;
}
```

---

## 7. UI Elements

### Toggle Switch (Div-based)

```html
<div class="toggle-switch" id="my-toggle">
    <div class="toggle-thumb"></div>
</div>
<input type="hidden" id="val-my-toggle" value="0">
```

```css
.toggle-switch {
    width: 40px; height: 22px;
    border-radius: 11px;
    background-color: #c0c0c0;
    position: relative;
    cursor: pointer;
}
.toggle-switch.checked { background-color: #007AFF; }
.toggle-thumb {
    width: 18px; height: 18px;
    border-radius: 50%;
    background: white;
    position: absolute;
    top: 2px; left: 2px;
    transition: left 0.2s;
}
.toggle-switch.checked .toggle-thumb { left: 20px; }
```

```javascript
document.querySelectorAll(".toggle-switch").forEach(function(toggle) {
    toggle.onclick = function() {
        this.classList.toggle("checked");
        var hidden = document.getElementById("val-" + this.id);
        if (hidden) {
            hidden.value = this.classList.contains("checked") ? "1" : "0";
            hidden.dispatchEvent(new Event("change", { bubbles: true }));
        }
    };
});
```

**C++ VALUE_CHANGED Handler (REQUIRED):**

```cpp
// In handle_event, inside "else if (params.cmd == VALUE_CHANGED)"
else if (id == L"val-my-toggle") {
    sciter::value val = el.get_value();
    std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
    bool checked = (strVal == L"1");
    
    vMyToggleSetting = checked ? 1 : 0;
    APP_SET_DATA(vMyToggleSetting, vMyToggleSetting);
    notifyMainProcess();  // Tell main process to reload
    return true;
}
```

> [!IMPORTANT]
> **Every toggle needs a VALUE_CHANGED handler!** JavaScript updates the hidden input `val-xxx`, which fires VALUE_CHANGED. If you forget the C++ handler, the setting won't save.

> **DON'T use `<input type="checkbox">`** - Sciter doesn't support `::before`/`::after` on inputs.

---

## 8. Height Budget Calculation

For fixed layouts, calculate all heights:

| Element | Height |
|---------|--------|
| Container | 450px |
| Title bar | 36px |
| Content padding | 32px (16px × 2) |
| Your content | calculate remaining |
| **Safety margin** | **10-20px** |

---

## 9. Vietnamese Text in C++ (MessageBox, Window Titles)

Vietnamese characters in C++ source files may display as garbled text if the file encoding is incorrect.

**ALWAYS use Unicode escape sequences for Vietnamese in C++:**

```cpp
// BAD - may show garbled text:
MessageBoxW(hwnd, L"Bạn có muốn giữ lại dữ liệu?", L"Thông báo", MB_OK);

// GOOD - Unicode escape sequences:
// "Bạn có muốn giữ lại dữ liệu?" = "B\u1EA1n c\u00F3 mu\u1ED1n gi\u1EEF l\u1EA1i d\u1EEF li\u1EC7u?"
// "Thông báo" = "Th\u00F4ng b\u00E1o"
MessageBoxW(hwnd, 
    L"B\u1EA1n c\u00F3 mu\u1ED1n gi\u1EEF l\u1EA1i d\u1EEF li\u1EC7u?", 
    L"Th\u00F4ng b\u00E1o", 
    MB_OK);
```

**Common Vietnamese Unicode codes:**
| Character | Code | Character | Code |
|-----------|------|-----------|------|
| ạ | `\u1EA1` | ấ | `\u1EA5` |
| á | `\u00E1` | ắ | `\u1EAF` |
| ã | `\u00E3` | ặ | `\u1EB7` |
| ô | `\u00F4` | ố | `\u1ED1` |
| ơ | `\u01A1` | ờ | `\u1EDD` |
| ư | `\u01B0` | ứ | `\u1EE9` |
| đ | `\u0111` | Đ | `\u0110` |

> [!TIP]
> Use online tools to convert Vietnamese text to Unicode escape sequences.

---

## Summary

| Requirement | Solution |
|-------------|----------|
| Blur effect | `enableAcrylicEffect()` + semi-transparent CSS |
| Centered header | `text-align: center` on `.title-bar` |
| Close button | Absolute positioned, handle in C++ `BUTTON_CLICK` |
| Drag support | SubclassProc with `WM_NCHITTEST` → `HTCAPTION` |
| **Close button vs drag** | **Exclude 40px right side from drag zone** |
| No background overflow | Fixed heights + `overflow: hidden` |
| **Scrollable lists** | **`height: 150px; overflow-y: auto`** |
| Single instance | Named Mutex in subprocess + `FindWindow()` fallback |
| **Subprocess cleanup** | **Store process handles, call `terminateAllChildren()` on exit** |
| **IPC subprocess spawn** | **Subprocess sends `WM_USER+1xx` to main process, not `CreateProcess`** |
| Toggle switch | Div-based with hidden input for VALUE_CHANGED |
| Text input (no jumping) | `!important` heights + `line-height: height-2px` + `overflow: hidden` |
| Vietnamese in C++ | Use Unicode escape sequences `\uXXXX` |

