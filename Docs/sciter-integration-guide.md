# Sciter Integration Guide for OpenKey

> **Purpose**: AI-readable reference for Sciter dialog development in OpenKey.

## Architecture

OpenKey uses **subprocess pattern** for Sciter dialogs:

```
OpenKey.exe              → Engine (2MB, no Sciter)
OpenKey.exe --about      → AboutDialog subprocess (30MB)
OpenKey.exe --settings   → SettingsDialog subprocess (30MB)
```

**Why subprocess?** Sciter objects cannot be cleanly deleted (assertion errors). Subprocess exits = OS frees all memory.

## File Structure

```
Sources/OpenKey/win32/OpenKey/OpenKey/
├── AboutDialog.h/.cpp      # About dialog
├── SettingsDialog.h/.cpp   # Settings dialog
└── main.cpp                # Router (--about, --settings flags)

Resources/Sciter/
├── about/                  # AboutDialog UI
│   ├── about.html
│   └── about.css
└── settings/               # SettingsDialog UI
    ├── settings.html
    └── settings.css

lib/sciter/                 # Sciter SDK
bin/                        # sciter.dll (must be next to exe)
```

## Quick Reference

### DO ✅

| Task | Code |
|------|------|
| **Include Sciter** | `#include "sciter-x-window.hpp"` |
| **Find element** | `root.find_first("#id")` with `char*` |
| **Get value** | `el.get_value().get<int>()` |
| **Set value** | `el.set_value(sciter::value(intVal))` |
| **Handle events** | Override `handle_event(HELEMENT, BEHAVIOR_EVENT_PARAMS&)` |
| **Exit subprocess** | `ExitProcess(0)` in WM_CLOSE |
| **Notify main process** | `PostMessage(mainWnd, WM_USER+101, 0, 0)` |

### DON'T ❌

| Don't | Why | Instead |
|-------|-----|---------|
| `delete sciterWindow` | Assertion error | Use subprocess, call `ExitProcess(0)` |
| `call_function()` | Often fails with assertion | Use `DOCUMENT_READY` + `set_value()` |
| `find_first(L"#id")` | Wrong type | Use `char*`: `find_first("#id")` |
| `wcscmp(tagName, ...)` | `tagName` is `astring` | Use `strcmp(tagName.c_str(), ...)` |
| SOM bindings on window | Doesn't work reliably | Use `handle_event()` directly |

## Common Errors & Fixes

### Header Conflicts
```cpp
// BEFORE including Sciter
#ifdef KEY_DOWN
#undef KEY_DOWN
#endif
#ifdef KEY_UP
#undef KEY_UP
#endif
```

### Missing hinstance()
```cpp
namespace sciter {
    namespace application {
        HINSTANCE hinstance() { return GetModuleHandle(NULL); }
    }
}
```

### sciter.dll Not Found
```
Assertion failed: _api (sciter-x-api.h:351)
```
→ Copy `sciter.dll` next to `.exe`

### Assertion on Close
```
Assertion failed: _ref_cntr == 0
```
→ Use `ExitProcess(0)` instead of deleting window

## Subprocess Implementation

### 1. Router (main.cpp)
```cpp
if (lpCmdLine && wcsstr(lpCmdLine, L"--settings")) {
    SettingsDialog dialog;
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
```

### 2. Spawn from Main Process
```cpp
void AppDelegate::createMainDialog() {
    HWND existing = FindWindowW(NULL, L"OpenKey Settings");
    if (existing) { SetForegroundWindow(existing); return; }
    
    WCHAR exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    wchar_t cmdLine[MAX_PATH + 20];
    swprintf_s(cmdLine, L"\"%s\" --settings", exePath);
    CreateProcessW(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}
```

### 3. Force Exit on Close
```cpp
if (msg == WM_CLOSE) {
    ExitProcess(0);
    return 0;
}
```

## Settings Dialog Pattern

### Load from Registry (constructor)
```cpp
SettingsDialog::SettingsDialog() : sciter::window(...) {
    APP_GET_DATA(vLanguage, 1);
    APP_GET_DATA(vInputType, 0);
    // ... load HTML
}
```

### Set UI on DOCUMENT_READY
```cpp
if (params.cmd == DOCUMENT_READY) {
    sciter::dom::element root = this->root();
    root.find_first("#kieu-go").set_value(sciter::value(vInputType));
    return true;
}
```

### Handle VALUE_CHANGED
```cpp
if (params.cmd == VALUE_CHANGED) {
    auto id = el.get_attribute("id");
    if (id == L"kieu-go") {
        int value = el.get_value().get<int>();
        vInputType = value;
        APP_SET_DATA(vInputType, value);
        notifyMainProcess();
        return true;
    }
}
```

### IPC to Main Process
```cpp
static void notifyMainProcess() {
    HWND mainWnd = FindWindow(_T("OpenKeyVietnameseInputMethod"), NULL);
    if (mainWnd) PostMessage(mainWnd, WM_USER + 101, 0, 0);
}

// In SystemTrayHelper.cpp WndProc:
case WM_USER+101:
    APP_GET_DATA(vLanguage, 1);
    APP_GET_DATA(vInputType, 0);
    APP_GET_DATA(vCodeTable, 0);
    APP_GET_DATA(vSwitchKeyStatus, 0);
    APP_GET_DATA(vUseSmartSwitchKey, 0);
    SystemTrayHelper::updateData();  // Refresh tray icon
    break;
```

### Bidirectional Sync (Tray → Settings UI)

When settings change from tray menu while Settings UI is open, notify subprocess:

```cpp
// In SystemTrayHelper.cpp after handling tray menu:
SystemTrayHelper::updateData();

// Notify settings subprocess to update UI if it's open
HWND settingsWnd = FindWindow(NULL, _T("OpenKey Settings"));
if (settingsWnd) {
    PostMessage(settingsWnd, WM_USER + 102, 0, 0);
}
```

Handle in SettingsDialog's SubclassProc:

```cpp
if (msg == WM_USER + 102) {
    // Reload settings from registry
    APP_GET_DATA(vLanguage, 1);
    APP_GET_DATA(vInputType, 0);
    APP_GET_DATA(vCodeTable, 0);
    
    // Get dialog instance (passed via SetWindowSubclass dwRefData)
    SettingsDialog* dialog = reinterpret_cast<SettingsDialog*>(dwRefData);
    if (dialog) {
        sciter::dom::element root = dialog->root();
        
        // Update toggle classes
        sciter::dom::element toggle = root.find_first("#toggle-language");
        if (toggle) {
            if (vLanguage == 0)
                toggle.set_attribute("class", L"toggle-switch checked");
            else
                toggle.set_attribute("class", L"toggle-switch");
        }
        
        // Update dropdowns
        sciter::dom::element dropdown = root.find_first("#input-type");
        if (dropdown) dropdown.set_value(sciter::value(vInputType));
    }
    return 0;
}
```

> [!IMPORTANT]
> Pass `this` pointer to `SetWindowSubclass`: `SetWindowSubclass(hwnd, SubclassProc, 1, (DWORD_PTR)this);`

## Blur Effect (Windows 10/11)

### Acrylic Blur Implementation

```cpp
void enableAcrylicEffect() {
    HWND hwnd = get_hwnd();
    
    // 1. CRITICAL: Set WS_EX_LAYERED style first
    SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
    
    // 2. Apply Acrylic effect
    ACCENT_POLICY policy = { 0 };
    policy.AccentState = ACCENT_ENABLE_ACRYLICBLURBEHIND;  // 4
    policy.GradientColor = 0x80FFFFFF;  // ABGR: 50% white tint for visible blur
    
    WINDOWCOMPOSITIONATTRIBDATA data = { 19, &policy, sizeof(policy) };
    SetWindowCompositionAttribute(hwnd, &data);
    
    // 3. Fix corners on Windows 11
    DWM_WINDOW_CORNER_PREFERENCE preference = DWMWCP_ROUND;  // 2
    DwmSetWindowAttribute(hwnd, 33, &preference, sizeof(preference));
}
```

### Blur Not Visible?

| Issue | Cause | Fix |
|-------|-------|-----|
| Blur weak | GradientColor opacity too low | Increase alpha: `0x80FFFFFF` (50%) instead of `0x01FFFFFF` (1%) |
| Blur not showing | CSS container opacity too high | Use `rgba(255,255,255,0.65)` not `0.85` |
| Gray corners | Container border-radius larger than window | Reduce `border-radius: 8px` or match window DWM corners |

### CSS for Blur Visibility

```css
html, body {
    background: transparent;
    overflow: hidden;
    width: 100%;
    height: 100%;
}

.container {
    width: 100%;
    height: 100%;
    background: rgba(255, 255, 255, 0.65);  /* Semi-transparent for blur */
    border-radius: 8px;  /* Match DWM corners */
    overflow: hidden;
}
```


## Window Dragging

```cpp
if (msg == WM_NCHITTEST) {
    LRESULT r = DefSubclassProc(hwnd, msg, wParam, lParam);
    if (r == HTCLIENT) {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        ScreenToClient(hwnd, &pt);
        if (pt.y < 50) return HTCAPTION;  // Drag zone
    }
    return r;
}
```

## Checklist for New Dialog

- [ ] Create `DialogName.h/.cpp` inheriting `sciter::window`
- [ ] Add `#undef KEY_DOWN/KEY_UP` before includes
- [ ] Add `sciter::application::hinstance()` implementation
- [ ] Create `Resources/Sciter/dialogname/` with HTML/CSS
- [ ] Add `--dialogname` router in `main.cpp`
- [ ] Add spawn code in `AppDelegate.cpp`
- [ ] Handle `WM_CLOSE` with `ExitProcess(0)`
- [ ] For settings: Add IPC (`WM_USER+101`) to reload in main process
- [ ] Copy `sciter.dll` to output directory

## Sciter CSS/JS Patterns

### ⚠️ CSS Limitations

Sciter's CSS engine differs from standard browsers:

| Feature | Browser | Sciter | Workaround |
|---------|---------|--------|------------|
| `::before` on inputs | ✅ Works | ❌ Limited | Use nested `<div>` elements |
| `::after` on inputs | ✅ Works | ❌ Limited | Use nested `<div>` elements |
| `inset: 0` shorthand | ✅ Works | ❌ Not supported | Use `top/left/right/bottom: 0` |
| `gap` in flexbox | ✅ Works | ⚠️ May not work | Use margins between items |
| Native `<widget type="toggle">` | N/A | ⚠️ Limited styling | Use div-based toggle |

### Toggle Switch Pattern (Recommended)

Since Sciter doesn't support `::before`/`::after` pseudo-elements for creating custom toggles, use **div-based toggles** with JavaScript click handlers.

#### HTML Structure
```html
<!-- Toggle container with inner thumb -->
<div class="toggle-switch" id="my-toggle">
    <div class="toggle-thumb"></div>
</div>
```

#### CSS Styling
```css
.toggle-switch {
    display: inline-block;
    width: 40px;
    height: 22px;
    border-radius: 11px;
    background-color: #e0e0e0;
    position: relative;
    cursor: pointer;
    vertical-align: middle;  /* Align with text */
}

.toggle-switch.checked {
    background-color: #007AFF;
}

.toggle-thumb {
    width: 18px;
    height: 18px;
    border-radius: 50%;
    background-color: white;
    box-shadow: 0 1px 2px rgba(0, 0, 0, 0.3);
    position: absolute;
    top: 2px;
    left: 2px;
    transition: left 0.2s ease;
}

.toggle-switch.checked .toggle-thumb {
    left: 20px;  /* Move thumb to right when checked */
}
```

#### JavaScript Click Handler
```javascript
// Setup on document ready
document.on("ready", function() {
    const toggles = document.$$(".toggle-switch");
    for (const toggle of toggles) {
        toggle.on("click", function(evt) {
            const isChecked = this.classList.contains("checked");
            if (isChecked) {
                this.classList.remove("checked");
            } else {
                this.classList.add("checked");
            }
            // Call C++ handler
            handleToggleChange(this.id, !isChecked);
        });
    }
});
```

### Flexbox Alignment Tips

```css
/* Use explicit flex-flow for reliable layout */
.setting-row {
    display: flex;
    flex-flow: row nowrap;
    align-items: center;
    justify-content: space-between;
}

/* Prevent toggle from shrinking */
.toggle-switch {
    flex-shrink: 0;
    min-width: 40px;
}

/* Let label take remaining space */
.setting-label {
    flex: 1;
}
```

### Event Handling Patterns

```javascript
// Dropdown changes
document.on("change", "select", function(evt, select) {
    const id = select.id;
    const value = parseInt(select.value);
    Window.this.onValueChange(id, value);
});

// Button clicks
document.on("click", "button", function(evt, button) {
    const id = button.id;
    Window.this.onButtonClick(id);
});

// Custom div toggle clicks
document.on("click", ".toggle-switch", function(evt) {
    // Toggle .checked class and notify
});
```

### Setting Initial Values from C++

```javascript
// Called from C++ to set initial UI state
function setSettings(language, inputType, freeAccent) {
    // Set toggle state via classList
    const toggle = document.getElementById("my-toggle");
    if (toggle) {
        if (freeAccent) {
            toggle.classList.add("checked");
        } else {
            toggle.classList.remove("checked");
        }
    }
    
    // Set dropdown value
    const dropdown = document.getElementById("kieu-go");
    if (dropdown) dropdown.value = inputType.toString();
}
```

## Troubleshooting

### ⚠️ Elements Not Clickable in Header Area

**Symptom**: Toggle switches, dropdowns, or buttons in the header area cannot be clicked.

**Cause**: The `WM_NCHITTEST` drag zone in C++ is capturing mouse events.

**Solution**: Reduce the drag zone height in the SubclassProc:

```cpp
// In SettingsDialog.cpp
if (msg == WM_NCHITTEST) {
    LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
    if (result == HTCLIENT) {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        ScreenToClient(hwnd, &pt);
        
        // ❌ Too large - blocks header element clicks
        // if (pt.y < 50) return HTCAPTION;
        
        // ✅ Better - only top edge for dragging
        if (pt.y < 10) return HTCAPTION;
    }
    return result;
}
```

> [!IMPORTANT]
> If interactive elements in the header area don't respond to clicks, always check the drag zone height first.

### Flexbox Vertical Alignment Issues

**Symptom**: Elements in a flex row are not vertically aligned.

**Solution**: Use consistent height and line-height:

```css
.header {
    display: flex;
    flex-flow: row nowrap;
    align-items: center;
    height: 32px;  /* Fixed height */
}

.header-element {
    line-height: 32px;  /* Match parent height */
    height: 32px;
    vertical-align: middle;
    flex-shrink: 0;  /* Prevent shrinking */
}
```

### Toggle Clicks Not Working (JS)

**Symptom**: Toggle clicks don't trigger in JavaScript.

**Possible causes**:
1. **Drag zone** - See "Elements Not Clickable" above
2. **Event delegation not working** - Use direct onclick instead:

```javascript
// ❌ May not work in Sciter
document.on("click", ".toggle-switch", function() { ... });

// ✅ Use querySelectorAll + direct onclick
document.querySelectorAll(".toggle-switch").forEach(function(toggle) {
    toggle.onclick = function(evt) {
        // Handle click
    };
});
```

### CSS `::before` / `::after` Not Working

**Symptom**: Custom toggle styling using pseudo-elements doesn't render.

**Cause**: Sciter has limited support for `::before`/`::after` on form elements.

**Solution**: Use real child `<div>` elements instead:

```html
<!-- ❌ Won't work in Sciter -->
<label class="toggle"><input type="checkbox"><span class="slider"></span></label>

<!-- ✅ Works in Sciter -->
<div class="toggle-switch" id="my-toggle">
    <div class="toggle-thumb"></div>
</div>
```

### Toggle Settings Not Saving to Registry

**Symptom**: Toggle visual state changes but settings are not persisted.

**Cause**: Div-based toggles don't fire `VALUE_CHANGED` events that C++ can catch.

**Solution**: Use hidden input pattern - JS toggles class AND updates hidden input:

```html
<div class="toggle-switch" id="my-toggle">
    <div class="toggle-thumb"></div>
</div>
<input type="hidden" id="val-my-toggle" value="0">
```

```javascript
toggle.onclick = function(evt) {
    const isChecked = this.classList.contains("checked");
    this.classList.toggle("checked");
    
    // Update hidden input to fire VALUE_CHANGED
    const hidden = document.getElementById("val-" + this.id);
    if (hidden) {
        hidden.value = !isChecked ? "1" : "0";
        hidden.dispatchEvent(new Event("change", { bubbles: true }));
    }
};
```

Handle in C++:

```cpp
if (params.cmd == VALUE_CHANGED) {
    std::wstring id = el.get_attribute("id");
    
    if (id == L"val-my-toggle") {
        sciter::value val = el.get_value();
        std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
        bool checked = (strVal == L"1");
        
        vMySetting = checked ? 1 : 0;
        APP_SET_DATA(vMySetting, vMySetting);
        notifyMainProcess();
        return true;
    }
}
```

### Window Size Issues (Content Clipped or Extra Space)

**Symptom**: Content is cut off at bottom, or there's extra gray space around edges.

**Causes & Fixes**:

| Issue | Cause | Fix |
|-------|-------|-----|
| Content cut off | Window height too small | Increase height in constructor: `RECT{0, 0, 350, 410}` |
| Extra space at bottom | Window height too large | Reduce height to match content |
| Gray corners | Container `border-radius` larger than DWM corners | Reduce `border-radius: 8px` |
| Scrollbars appear | Container/body overflow | Add `overflow: hidden` to html, body, .container |

### `<label>` Elements Blocking Toggle Clicks

**Symptom**: Toggles inside `<label>` elements don't respond to clicks.

**Cause**: Sciter's `<label>` behavior may interfere with click handling.

**Solution**: Use `<div>` instead of `<label>` for toggle containers:

```html
<!-- ❌ May block clicks in Sciter -->
<label class="switch-key-item">
    <div class="toggle-switch-small" id="key-ctrl">...</div>
</label>

<!-- ✅ Works -->
<div class="switch-key-item">
    <div class="toggle-switch-small" id="key-ctrl">...</div>
</div>
```

### Window Always on Top (Topmost)

**Symptom**: Settings window always stays above other windows.

**Cause**: Sciter's `SW_POPUP` flag creates a topmost window by default.

**Solution**: Explicitly remove topmost after centering:

```cpp
// Instead of:
SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

// Use:
SetWindowPos(hwnd, HWND_NOTOPMOST, x, y, 0, 0, SWP_NOSIZE);
```

### Recommended CSS Values

| Property | Recommended Value | Why |
|----------|-------------------|-----|
| Container `border-radius` | `8px` | Matches DWM corners, prevents gray clipping |
| Container `background` | `rgba(255,255,255,0.65)` | Semi-transparent for blur visibility |
| Container `overflow` | `hidden` | Prevents scrollbars |
| Body `overflow` | `hidden` | Prevents scrollbars |
