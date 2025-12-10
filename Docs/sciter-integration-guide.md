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

## Quick Reference

### DO ✅

| Task | Code |
|------|------|
| **Find element** | `root.find_first("#id")` with `char*` |
| **Get value** | `el.get_value().get<int>()` |
| **Set value** | `el.set_value(sciter::value(intVal))` |
| **Get element size** | `RECT r = el.get_location(CONTENT_BOX)` |
| **Set class** | `el.set_attribute("class", L"toggle checked")` |
| **Handle events** | Override `handle_event(HELEMENT, BEHAVIOR_EVENT_PARAMS&)` |
| **Exit subprocess** | `ExitProcess(0)` in WM_CLOSE |
| **Notify main process** | `PostMessage(mainWnd, WM_USER+101, 0, 0)` |

### DON'T ❌

| Don't | Why | Instead |
|-------|-----|---------|
| `delete sciterWindow` | Assertion error | Use subprocess, call `ExitProcess(0)` |
| `find_first(L"#id")` | Wrong type | Use `char*`: `find_first("#id")` |
| `<label>` around toggles | Blocks clicks | Use `<div>` |
| `<input type="checkbox">` | ::before/::after broken | Use div-based toggles |
| `height: 100%` on container | Breaks auto-fit | Use `height: auto` |
| `SW_TITLELESS` | Undeclared in some versions | Use `SW_POPUP \| SW_ALPHA` |
| `el.child(0).destroy()` | Chained call on temporary fails | Store in variable: `auto c = el.child(0); c.destroy();` |

---

## Settings Synchronization

### UI → Registry → Main Process (Save)

```cpp
// In handle_event VALUE_CHANGED handler:
if (id == L"val-toggle-language") {
    sciter::value val = el.get_value();
    std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
    bool checked = (strVal == L"1");
    
    vLanguage = checked ? 0 : 1;
    APP_SET_DATA(vLanguage, vLanguage);
    notifyMainProcess();  // Send WM_USER+101 to main
    return true;
}
```

### Main Process → Registry Reload (WM_USER+101)

```cpp
// In SystemTrayHelper.cpp WndProc:
case WM_USER+101:
    APP_GET_DATA(vLanguage, 1);
    APP_GET_DATA(vInputType, 0);
    APP_GET_DATA(vCodeTable, 0);
    APP_GET_DATA(vSwitchKeyStatus, 0);
    SystemTrayHelper::updateData();  // Refresh tray icon
    break;
```

### Bidirectional Sync: Main → Settings UI (WM_USER+102)

When tray menu changes settings while Settings UI is open:

```cpp
// In SystemTrayHelper.cpp after handling tray menu:
HWND settingsWnd = FindWindow(NULL, _T("OpenKey Settings"));
if (settingsWnd) {
    PostMessage(settingsWnd, WM_USER + 102, 0, 0);
}
```

Handle in SettingsDialog's SubclassProc:

```cpp
if (msg == WM_USER + 102) {
    APP_GET_DATA(vLanguage, 1);
    // ... reload other settings
    
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
> Pass `this` pointer to SubclassProc: `SetWindowSubclass(hwnd, SubclassProc, 1, (DWORD_PTR)this);`

---

## Toggle Switch Pattern

Sciter doesn't support `::before`/`::after` on inputs. Use **div-based toggles with hidden inputs**:

### HTML
```html
<div class="toggle-switch" id="my-toggle">
    <div class="toggle-thumb"></div>
</div>
<input type="hidden" id="val-my-toggle" value="0">
```

### CSS
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
    transition: left 0.3s;
}
.toggle-switch.checked .toggle-thumb { left: 20px; }
```

### JavaScript
```javascript
toggle.onclick = function() {
    this.classList.toggle("checked");
    const hidden = document.getElementById("val-" + this.id);
    if (hidden) {
        hidden.value = this.classList.contains("checked") ? "1" : "0";
        hidden.dispatchEvent(new Event("change", { bubbles: true }));
    }
};
```

---

## Auto-Fit Window to Content

Don't hardcode window height. Measure content after loading:

```cpp
// After expand() in constructor:
sciter::dom::element container = this->root().find_first(".container");
if (container) {
    RECT r = container.get_location(CONTENT_BOX);
    int w = max(r.right - r.left, 350);
    int h = max(r.bottom - r.top, 200);
    SetWindowPos(get_hwnd(), NULL, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER);
}
```

**CSS requirement**:
```css
.container { width: 350px; height: auto; }
```

> [!IMPORTANT]
> **NEVER DELETE THIS CODE!** The auto-fit pattern is critical for proper window sizing.
> - CSS MUST use `height: auto`, never hardcode height values
> - C++ MUST measure container via `get_location(CONTENT_BOX)` for window sizing
> - When resizing dynamically (e.g., expand/collapse), reuse this same pattern

---

## Fixed Layout with Scrollable Lists

When a dialog contains a list with variable item count, use **fixed container + fixed list height + internal scroll**:

### Use Case
Dialogs like Macro Table where:
- Window size should NOT change based on content
- List area should scroll when items exceed available space
- All UI elements (header, inputs, buttons, footer) remain visible

### Pattern

**C++ - Fixed Window Size:**
```cpp
// In constructor after expand():
SetWindowPos(get_hwnd(), NULL, 0, 0, 380, 450, SWP_NOMOVE | SWP_NOZORDER);
// Do NOT use recalcWindowSize() for fixed layouts
```

**CSS - Height Calculation:**
```css
/* Total window: 450px */

.container {
    width: 380px;
    height: 450px;          /* Fixed */
    overflow: hidden;
}

/* Title bar */
.title-bar {
    height: 36px;           /* = 36px */
}

/* Content area: 450 - 36 = 414px */
.content-section {
    height: 414px;
    padding: 16px;          /* = 32px vertical */
    overflow: hidden;
}

/* List with fixed height and scroll */
.macro-list {
    display: block;
    height: 150px;          /* Fixed - adjust to fit remaining space */
    min-height: 150px;
    max-height: 150px;
    overflow-y: auto;       /* Standard scroll (not scroll-indicator) */
}
```

### Height Budget Calculation

Calculate heights explicitly to prevent overflow:

| Element | Height |
|---------|--------|
| Container | 450px (window) |
| Title bar | 36px |
| Content padding | 32px (16px × 2) |
| Input card | ~120px |
| List card header | ~30px |
| **List area** | **150px** |
| Footer buttons | ~40px |
| Margins | ~24px |
| **Total** | ~432px ✅ (fits in 414px content + 36px title) |

> [!TIP]
> Always calculate height budget before implementation. Leave ~10-20px margin for safety.

### Common Fixes

| Issue | Fix |
|-------|-----|
| Footer buttons cut off | Reduce list height |
| `scroll-indicator` not working | Use `overflow-y: auto` instead |
| Content overflows container | Add `overflow: hidden` to content-section |
| Items not visible | Ensure `display: block` on list |

---

## Blur Effect (Windows 10/11)

```cpp
void enableAcrylicEffect() {
    HWND hwnd = get_hwnd();
    SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
    
    ACCENT_POLICY policy = { 0 };
    policy.AccentState = 4;  // ACCENT_ENABLE_ACRYLICBLURBEHIND
    policy.GradientColor = 0x80FFFFFF;  // 50% white (ABGR)
    
    WINDOWCOMPOSITIONATTRIBDATA data = { 19, &policy, sizeof(policy) };
    SetWindowCompositionAttribute(hwnd, &data);
    
    // Round corners on Windows 11
    int preference = 2;  // DWMWCP_ROUND
    DwmSetWindowAttribute(hwnd, 33, &preference, sizeof(preference));
}
```

**CSS for blur visibility**:
```css
.container {
    background: rgba(255, 255, 255, 0.65);  /* Semi-transparent */
    border-radius: 8px;  /* Match DWM corners */
}
```

---

## Button Click Handling

```cpp
if (params.cmd == BUTTON_CLICK) {
    sciter::dom::element el(params.heTarget);
    auto id = el.get_attribute("id");
    
    if (id == L"btn-close") {
        PostMessage(get_hwnd(), WM_CLOSE, 0, 0);
        return true;
    }
    
    if (id == L"btn-advanced") {
        openAdvancedSettings();
        return true;
    }
}
```

---

## Window Dragging (Reduced Zone)

```cpp
if (msg == WM_NCHITTEST) {
    LRESULT r = DefSubclassProc(hwnd, msg, wParam, lParam);
    if (r == HTCLIENT) {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        ScreenToClient(hwnd, &pt);
        if (pt.y < 10) return HTCAPTION;  // Only top 10px for dragging
    }
    return r;
}
```

---

## Common Issues & Fixes

| Issue | Cause | Fix |
|-------|-------|-----|
| Toggles not saving | Div doesn't fire VALUE_CHANGED | Use hidden input pattern |
| Click not working | `<label>` intercepting | Use `<div>` instead |
| Gray corners | border-radius too large | Use `8px` |
| Blur not visible | GradientColor opacity low / CSS too opaque | Use `0x80FFFFFF` / `rgba(255,255,255,0.65)` |
| Content clipped | Window height too small | Use auto-fit |
| Flex children collapse | `display:flex` + `height:auto` with children having `height:100%` | Don't use flex for initial layout, OR use fixed height |
| Button click not working | C++ `handle_event` intercepts before JS | Handle button in C++ directly |
| SVG captures button clicks | Click target is SVG/path inside button, not button itself | Add `pointer-events: none` to SVG and its children |
| Extra space at bottom | Window height too large | Use auto-fit |
| Always on top | SW_POPUP default | Use `HWND_NOTOPMOST` |
| Scrollbars | Overflow | Add `overflow: hidden` |
| Header clicks blocked | Drag zone too large | Reduce to 10px |
| Toggles disappear on layout change | Sciter doesn't repaint absolute positioned elements | Use synchronous force reflow (via offsetHeight) |
| Clickable div not receiving clicks | Button element has issues with child SVG | Use `<div>` with `setting-row-clickable` class |
| JS click handler not firing | C++ HYPERLINK_CLICK intercepting | Let JS handle UI, use hidden input VALUE_CHANGED for C++ communication |
| Layout shift on expand/collapse | Inline-block whitespace + inconsistent padding | Use `font-size: 0` on container, reset `font-size` in sections. Keep padding on sections (not container) for consistency across states |
| Blur ghost on window resize | Windows DWM blur doesn't update when window shrinks | Resize immediately on collapse (10ms timer), use CSS fade/slide animation on content. Remove CSS `transition` on container width. Call `enableAcrylicEffect()` after resize. |
| Jagged/aliased borders on rounded corners | Sciter doesn't anti-alias borders well | Use `box-shadow: inset 0 0 0 1px color` instead of `border`. Reduce `border-radius` to 6px or less. |
| Blurry/jagged SVG icons | SVG scaled from different size | Use SVG with exact display size in `viewBox`, `width`, `height` attributes (e.g., 24x24). Avoid scaling down large SVGs. |
| Settings saved but core doesn't receive | Main process not reloading from registry | Add `APP_GET_DATA` for new settings in `WM_USER+101` handler in `SystemTrayHelper.cpp`. Settings subprocess saves to registry, main process must reload. |
| Heap corruption with getRegBinary | `new BYTE[0]` when registry key empty | Check `size > 0` before allocating in `getRegBinary()`. Return NULL when size is 0. Don't `delete[]` the returned pointer (it's static). |
| Macro changes not applied immediately | Main process doesn't reload macro data | Add `initMacroMap()` call in `WM_USER+101` handler. MacroDialog sends `PostMessage(mainWnd, WM_USER+101)` after saving. |
| Input text jumps when typing | `line-height` doesn't match `height` | Use `height: 32px; line-height: 32px; padding: 0 10px;` for inputs. Keep line-height equal to height for vertical centering. |
| Call JS functions from C++ | `element.eval()` doesn't exist | Use `call_function("funcName", arg1, arg2)` inherited from `sciter::window`. Pass UTF-8 strings. |

---

## Expandable Panels

### Architecture

For expandable settings panels, use this pattern:

1. **HTML**: Clickable row + hidden input for C++ communication
2. **CSS**: Toggle visibility with class selector
3. **JS**: Handle click, toggle class, dispatch VALUE_CHANGED
4. **C++**: Only resize window when notified

### HTML Structure

```html
<!-- Clickable row (not button to avoid SVG click issues) -->
<div class="setting-row setting-row-clickable" id="btn-advanced">
    <span class="setting-label">Cài đặt nâng cao</span>
    <svg class="arrow-icon" viewBox="0 0 24 24">
        <path d="M9 18l6-6-6-6"></path>
    </svg>
</div>
<!-- Hidden input for C++ VALUE_CHANGED -->
<input type="hidden" id="val-expand-state" value="0">
```

```css
/* Container: no padding, font-size:0 to eliminate inline-block whitespace */
.container {
    width: 350px;
    padding: 0;
    font-size: 0; /* Critical: eliminates inline-block whitespace */
}

/* Sections always have their own padding (prevents shift on expand) */
.compact-section {
    width: 350px;
    padding: 16px;
    font-size: 14px; /* Reset font-size from container */
    box-sizing: border-box;
}

.advanced-section {
    display: none;
    width: 350px;
    padding: 16px;
    font-size: 14px;
    box-sizing: border-box;
}

/* Expanded state - only change display, not padding/width */
.container.expanded {
    width: 700px;
    white-space: nowrap;
}

.container.expanded .compact-section {
    display: inline-block;
    vertical-align: top;
    white-space: normal;
}

.container.expanded .advanced-section {
    display: inline-block;
    vertical-align: top;
    white-space: normal;
}
```

### JavaScript (with Synchronous Force Reflow)

```javascript
function toggleAdvancedSettings() {
    const container = document.getElementById("main-container");
    if (container) {
        const isExpanded = container.classList.contains("expanded");
        
        if (isExpanded) {
            container.classList.remove("expanded");
        } else {
            container.classList.add("expanded");
        }
        
        // CRITICAL: Synchronous force reflow on toggle switches
        // Must be IMMEDIATE (no setTimeout) to minimize visible flash
        // Sciter doesn't repaint absolute positioned elements after layout change
        const toggles = document.querySelectorAll(".toggle-switch, .toggle-switch-small");
        toggles.forEach(function(toggle) {
            toggle.style.display = "none";
        });
        container.offsetHeight; // Force synchronous reflow
        toggles.forEach(function(toggle) {
            toggle.style.display = "";
        });
        
        // Notify C++ to resize window
        const hiddenInput = document.getElementById("val-expand-state");
        if (hiddenInput) {
            hiddenInput.value = !isExpanded ? "1" : "0";
            hiddenInput.dispatchEvent(new Event("change", { bubbles: true }));
        }
    }
}
```

> [!IMPORTANT]
> **Force reflow timing**: Use synchronous approach (no `setTimeout`) to minimize visible flash. Async approaches (visibility, transform, delayed display) cause noticeable flicker.

### C++ (VALUE_CHANGED handler)

```cpp
if (id == L"val-expand-state") {
    sciter::value val = el.get_value();
    std::wstring strVal = val.is_string() ? val.get<std::wstring>() : L"0";
    m_isExpanded = (strVal == L"1");
    SetTimer(get_hwnd(), TIMER_RESIZE_WINDOW, 100, NULL);
    return true;
}
```

---

## Debugging

### OutputDebugString + DebugView

Use `OutputDebugStringW` for logging, view with [DebugView](https://learn.microsoft.com/en-us/sysinternals/downloads/debugview):

```cpp
// Log all events with target info
wchar_t debugMsg[512];
swprintf_s(debugMsg, L"OpenKey: event cmd=%d target=[%S#%s.%s]\n", 
    params.cmd, 
    el.get_tag().c_str(),
    id.c_str(),
    className.c_str());
OutputDebugStringW(debugMsg);
```

**DebugView setup:**
1. Run as Administrator
2. Enable Capture → Capture Win32 (Ctrl+W)
3. Filter: `OpenKey:*`

### Sciter Inspector

Enable debug mode in window creation:

```cpp
: sciter::window(SW_POPUP | SW_ALPHA | SW_ENABLE_DEBUG, RECT{0, 0, 350, 380})
```

Then run `inspector.exe` from [Sciter SDK](https://sciter.com/download/) to inspect live DOM.

---

## SVG Icons

**Filled icons**: `fill="currentColor"` or `fill="#EB2121"`
**Stroke icons**: `fill="none" stroke="currentColor" stroke-width="1.5"`

```html
<!-- Close button with red X -->
<button class="btn-close" id="btn-close">
    <svg viewBox="0 0 24 24" fill="#EB2121">
        <path d="M12 22C6.47715 22 2 17.5228 2 12C2 6.47715..."/>
    </svg>
</button>
```

---

## Checklist for New Dialog

- [ ] Create `DialogName.h/.cpp` inheriting `sciter::window`
- [ ] Add `#undef KEY_DOWN/KEY_UP` before includes
- [ ] Add `sciter::application::hinstance()` implementation
- [ ] Create `Resources/Sciter/dialogname/` with HTML/CSS/JS
- [ ] Add `--dialogname` router in `main.cpp`
- [ ] Add spawn code in `AppDelegate.cpp`
- [ ] Handle `WM_CLOSE` with `ExitProcess(0)`
- [ ] Use auto-fit for window sizing
- [ ] For settings: Add IPC (`WM_USER+101/102`) for sync
- [ ] Copy `sciter.dll` to output directory
