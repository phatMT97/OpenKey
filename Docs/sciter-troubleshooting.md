# Sciter Troubleshooting Guide

> **Purpose**: Common issues, their fixes, and how to enable Sciter Inspector for debugging.

---

## Sciter Inspector

Inspector is the essential tool for debugging Sciter UI. It allows live DOM inspection, CSS editing, and JavaScript console.

### Enable Inspector

In `main.cpp` before creating dialog:

```cpp
// Enable debug mode for Inspector connection
SciterSetOption(NULL, SCITER_SET_DEBUG_MODE, TRUE);

// Enable required runtime features (socket I/O is required for Inspector)
SciterSetOption(NULL, SCITER_SET_SCRIPT_RUNTIME_FEATURES,
    ALLOW_FILE_IO |
    ALLOW_SOCKET_IO |    // REQUIRED for Inspector
    ALLOW_EVAL |
    ALLOW_SYSINFO);
```

OR add `SW_ENABLE_DEBUG` flag in window creation:
```cpp
: sciter::window(SW_POPUP | SW_ALPHA | SW_ENABLE_DEBUG, RECT{0, 0, 380, 450})
```

### Connect Inspector

1. Run `inspector.exe` from Sciter SDK folder first
2. Open your dialog
3. Press **Ctrl+Shift+I** in the dialog window
4. Inspector will show: DOM tree, CSS styles, Console

> [!TIP]
> **Ctrl+Shift+Click** on any element to select it directly in Inspector.

### Inspector Not Connecting?

| Issue | Solution |
|-------|----------|
| Nothing happens on Ctrl+Shift+I | Add `ALLOW_SOCKET_IO` in `SciterSetOption` |
| Port conflict | Check if another Inspector is already running |
| Wrong SDK version | Use Inspector from the same SDK as sciter.dll |

---

## Common Issues & Fixes

### Layout Issues

| Issue | Cause | Fix |
|-------|-------|-----|
| Content overflows window | No height limit | Set fixed `height` + `overflow: hidden` |
| Footer buttons cut off | List too tall | Reduce list `height`, calculate height budget |
| Background overflows | Container too large | Match container height to window height |
| Scrollbar appears | Content overflow | Add `overflow: hidden` to containers |
| Extra space at bottom | Window too large | Use auto-fit or correct fixed height |

### Element Issues

| Issue | Cause | Fix |
|-------|-------|-----|
| Toggle not clickable | `<label>` wrapping | Use `<div>` not `<label>` |
| Toggle ::before/::after not working | Sciter limitation | Use div-based toggles |
| Button click not working | SVG captures click | Add `pointer-events: none` to SVG |
| Close button not responding | Wrong event handler | Handle in C++ `BUTTON_CLICK` |
| Hidden input not triggering | No change event | Call `dispatchEvent(new Event("change", { bubbles: true }))` |

### Styling Issues

| Issue | Cause | Fix |
|-------|-------|-----|
| Gray corners on blur | border-radius too large | Use `8px` max |
| Blur not visible | CSS background too opaque | Use `rgba(255,255,255,0.65)` |
| Jagged borders | Sciter anti-aliasing | Use `box-shadow: inset` instead of `border` |
| Text jumping in input | Sciter's std-edit uses `height: 1.4em` | Use `!important` on `height`, `line-height: height-2px`, `overflow: hidden` |
| Layout shift on toggle | Inline-block whitespace | Use `font-size: 0` on container |

### C++ / Integration Issues

| Issue | Cause | Fix |
|-------|-------|-----|
| Assertion on delete | Sciter cleanup issue | Use subprocess pattern, call `ExitProcess(0)` |
| Multiple dialogs open | No single-instance check | Use `FindWindow()` before spawn |
| find_first returns null | Wrong string type | Use `char*` not `wchar_t*`: `find_first("#id")` |
| Settings not saving | Registry not synced | Call `notifyMainProcess()` with WM_USER+101 |
| Main process not updating | Not reloading registry | Add `APP_GET_DATA` in WM_USER+101 handler |
| Heap corruption | getRegBinary memory issue | Don't `delete[]` returned pointer (it's static) |

### Window Issues

| Issue | Cause | Fix |
|-------|-------|-----|
| Always on top | SW_POPUP default | Use `HWND_NOTOPMOST` in SetWindowPos |
| Can't drag window | No hit-test handler | Implement `WM_NCHITTEST` in SubclassProc |
| Drag blocked by close button | Drag zone too large | Exclude close button area in `WM_NCHITTEST` |
| Window doesn't close | No WM_CLOSE handler | Handle in SubclassProc with `ExitProcess(0)` |

---

## Debugging with DebugView

Use `OutputDebugStringW` for logging, view with [DebugView](https://learn.microsoft.com/en-us/sysinternals/downloads/debugview):

```cpp
wchar_t msg[256];
swprintf_s(msg, L"Dialog: value = %d\n", someValue);
OutputDebugStringW(msg);
```

**DebugView setup:**
1. Run as Administrator
2. Enable **Capture → Capture Win32** (Ctrl+W)
3. Filter: add `OpenKey:*` or your prefix

---

## Height Budget Calculation

For fixed layouts, calculate all heights explicitly:

```
Container Height = Title + Content + Padding + Margins

Example (450px window):
- Title bar:        36px
- Content padding:  32px (16px × 2)
- Input card:      120px
- List header:      30px
- List area:       150px
- Footer buttons:   40px
- Margins:          24px
-----------------------
  Total:           432px ✅ (fits in 450px)
```

> [!TIP]
> Leave 10-20px safety margin. If footer is cut off, reduce list height.

---

## Sciter CSS Differences

Properties unique to Sciter:

| Property | Use |
|----------|-----|
| `font-rendering-mode: snap-pixel` | Fix text jumping in edit fields |
| `overflow-y: scroll-indicator` | Mobile-style scrollbar (may not work in all versions) |
| `flow: horizontal` | Horizontal layout (alternative to flexbox) |
| `size: 100px 200px` | Shorthand for width + height |

---

## Quick Reference

### DO ✅

| Task | Code |
|------|------|
| Find element | `root.find_first("#id")` (char*, not wchar_t*) |
| Get value | `el.get_value().get<int>()` |
| Set value | `el.set_value(sciter::value(val))` |
| Set class | `el.set_attribute("class", L"myclass")` |
| Get size | `el.get_location(CONTENT_BOX)` |
| Call JS function | `call_function("funcName", arg1, arg2)` |
| Exit dialog | `ExitProcess(0)` in WM_CLOSE |

### DON'T ❌

| Don't | Why | Instead |
|-------|-----|---------|
| `delete sciterWindow` | Assertion error | Use subprocess |
| `find_first(L"#id")` | Wrong type | Use `char*` |
| `<label>` around toggles | Blocks clicks | Use `<div>` |
| `<input type="checkbox">` | ::before broken | Use div toggles |
| `delete[] getRegBinary()` | Static pointer | Don't delete |
| `height: 100%` on container | Breaks layout | Use fixed height |

---

## Sample Debug Logging

Add to `handle_event` to see all events:

```cpp
bool DialogName::handle_event(HELEMENT he, BEHAVIOR_EVENT_PARAMS& params) {
    sciter::dom::element el(params.heTarget);
    std::wstring id = el.get_attribute("id") ? el.get_attribute("id") : L"";
    std::wstring cls = el.get_attribute("class") ? el.get_attribute("class") : L"";
    
    wchar_t msg[512];
    swprintf_s(msg, L"Event: cmd=%d target=[%s#%s.%s]\n",
        params.cmd,
        el.get_element_type(),
        id.c_str(),
        cls.c_str());
    OutputDebugStringW(msg);
    
    // Your event handling...
    return false;
}
```

**Common event codes:**
- `DOCUMENT_READY` (193) - HTML loaded
- `BUTTON_CLICK` (0) - Button clicked  
- `VALUE_CHANGED` (2) - Input value changed
- `HYPERLINK_CLICK` (128) - Link clicked
