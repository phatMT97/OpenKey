# Sciter.JS Integration Guide - OpenKey AboutDialog

## Tổng quan

Document này ghi lại toàn bộ quá trình tích hợp Sciter.JS vào OpenKey để tạo AboutDialog với UI hiện đại, bao gồm tất cả các lỗi gặp phải và cách khắc phục.

## Kiến trúc

### Cấu trúc thư mục Production
```
OpenKey/
├── OpenKey.exe
├── sciter.dll                    # Sciter runtime (bắt buộc)
└── Resources/
    └── Sciter/
        └── about/
            ├── about.html        # UI structure
            ├── about.css         # Styling
            └── about.js          # Logic & C++ binding
```

### Standalone Pattern
AboutDialog được implement như một **standalone Sciter window**, không kế thừa từ `BaseDialog`:
- `AboutDialog` inherits từ `sciter::window`
- `AppDelegate` quản lý riêng qua `closeAboutDialog()`
- Không dùng chung dialog management với Win32 dialogs

---

## Các lỗi gặp phải & Cách fix

### 1. Windows Header Conflicts

**Lỗi:**
```
error C2059: syntax error: 'constant' at sciter-x-behavior.h:226
```

**Nguyên nhân:** 
Macros `KEY_DOWN`, `KEY_UP` từ Windows headers conflict với Sciter enums.

**Fix:**
```cpp
// AboutDialog.h - TRƯỚC khi include Sciter headers
#ifdef KEY_DOWN
#undef KEY_DOWN
#endif
#ifdef KEY_UP
#undef KEY_UP
#endif

#include "sciter-x-window.hpp"
```

---

### 2. Linker Error - Missing Sciter API

**Lỗi:**
```
error LNK2019: unresolved external symbol "sciter::application::hinstance(void)"
```

**Nguyên nhân:**
Sciter.JS SDK không có `.lib` file, chỉ có DLL. Function `sciter::application::hinstance()` cần được implement.

**Fix:**
```cpp
// AboutDialog.cpp - Implement missing function
namespace sciter {
    namespace application {
        HINSTANCE hinstance() {
            return GetModuleHandle(NULL);
        }
    }
}
```

---

### 3. Assertion Failed - Sciter DLL Not Found

**Lỗi:**
```
Assertion failed!
File: sciter-x-api.h
Line: 351
Expression: _api
```

**Nguyên nhân:**
`sciter.dll` không nằm cùng thư mục với executable.

**Fix:**
Copy `sciter.dll` vào thư mục chứa `.exe`:
```powershell
Copy-Item "sciter-js-sdk/bin/windows/x64/sciter.dll" `
          -Destination "path/to/exe/directory/"
```

**Lưu ý:** 
- Development: Copy vào `x64/Debug/` và `x64/Release/`
- Production: Đóng gói cùng installer

---

### 4. Failed to Load HTML

**Lỗi:**
```
Failed to load about.html
```

**Nguyên nhân:**
Đường dẫn HTML file không đúng.

**Sai:**
```cpp
// ❌ Dùng GetCurrentDirectory() - không reliable
GetCurrentDirectoryW(MAX_PATH, htmlPath);
wcscat_s(htmlPath, L"\\Resources\\...");
```

**Đúng:**
```cpp
// ✅ Dùng GetModuleFileNameW() - lấy đường dẫn executable
WCHAR exePath[MAX_PATH];
GetModuleFileNameW(NULL, exePath, MAX_PATH);

// Remove executable name
WCHAR* lastSlash = wcsrchr(exePath, L'\\');
if (lastSlash) *lastSlash = L'\0';

// Resources folder cùng cấp với exe
WCHAR htmlPath[MAX_PATH];
swprintf_s(htmlPath, MAX_PATH, L"%s\\Resources\\Sciter\\about\\about.html", exePath);
```

---

### 5. Comment Line Continuation Error

**Lỗi:**
```
warning C4010: single-line comment contains line-continuation character
error C2065: 'htmlPath': undeclared identifier
```

**Nguyên nhân:**
Dấu `\` ở cuối comment line khiến line tiếp theo bị treat như part of comment.

**Sai:**
```cpp
// That's 5 levels up: ..\..\..\..\..\
WCHAR htmlPath[MAX_PATH];  // ❌ Bị comment out!
```

**Đúng:**
```cpp
// That's 5 levels up
WCHAR htmlPath[MAX_PATH];  // ✅ OK
```

---

### 6. Window Size & Style Issues

**Vấn đề:**
Window hiển thị quá nhỏ hoặc không có titlebar.

**Fix:**
```cpp
// Specify window size in constructor
AboutDialog::AboutDialog()
    : sciter::window(SW_ENABLE_DEBUG, RECT{0, 0, 500, 600}) {
```cpp
// Use GetModuleFileNameW for reliable paths
WCHAR exePath[MAX_PATH];
GetModuleFileNameW(NULL, exePath, MAX_PATH);
```

**❌ DON'T:**
```cpp
// Don't use GetCurrentDirectory - unreliable
GetCurrentDirectoryW(MAX_PATH, path);
```

### 2. String Functions

**✅ DO:**
```cpp
// Always specify buffer size
swprintf_s(buffer, MAX_PATH, L"format %s", arg);
```

**❌ DON'T:**
```cpp
// Missing size parameter - compilation error
swprintf_s(buffer, L"format %s", arg);
```

### 3. Comments

**✅ DO:**
```cpp
// Avoid backslashes at end of comments
// Use forward slashes for paths in comments: ../../../
```

**❌ DON'T:**
```cpp
// Don't end comments with backslash: ..\..\..\
```

### 4. Header Include Order

**✅ DO:**
```cpp
#pragma once

// 1. Undefine conflicting macros
#ifdef KEY_DOWN
#undef KEY_DOWN
#endif

// 2. Include Sciter headers
#include "sciter-x-window.hpp"

// 3. Include other headers
#include <string>
```

---

## C++ ↔ JavaScript Communication

### Setup (Tạm thời disabled)

Hiện tại các JavaScript calls đã được comment out để tránh assertion errors. Để enable:

1. **Get root element:**
```cpp
sciter::dom::element root = sciter::dom::element::root_element(get_hwnd());
```

2. **Call JavaScript function:**
```cpp
sciter::value result = root.call_function("functionName", arg1, arg2);
```

3. **Expose C++ functions via SOM_PASSPORT:**
```cpp
// AboutDialog.h
SOM_PASSPORT_BEGIN(AboutDialog)
    SOM_FUNCS(
        SOM_FUNC(openUrl),
        SOM_FUNC(checkUpdate)
    )
SOM_PASSPORT_END
```

---

## Customizing UI

### HTML Structure
File: `Resources/Sciter/about/about.html`

```html
<!DOCTYPE html>
<html lang="vi">
<head>
    <link rel="stylesheet" href="about.css">
    <script type="module" src="about.js"></script>
</head>
<body>
    <!-- Your UI here -->
</body>
</html>
```

### CSS Styling
File: `Resources/Sciter/about/about.css`

**Glassmorphism effect:**
```css
.container {
    background: rgba(255, 255, 255, 0.1);
    backdrop-filter: blur(10px);
    border: 1px solid rgba(255, 255, 255, 0.2);
    border-radius: 16px;
}
```

**Note:** `backdrop-filter` có thể không work trong Sciter. Cần test và có thể dùng alternative effects.

### JavaScript Logic
File: `Resources/Sciter/about/about.js`

```javascript
// Handle external links
document.addEventListener('click', (e) => {
    if (e.target.tagName === 'A') {
        e.preventDefault();
        Window.this.openUrl(e.target.href);
    }
});

// Call C++ functions
function checkForUpdates() {
    Window.this.checkUpdate();
}
```

---

## Deployment Checklist

### Development
- [ ] Copy `sciter.dll` to `x64/Debug/` and `x64/Release/`
- [ ] Copy `Resources/` folder to executable directory
- [ ] Test with Sciter Inspector (`SW_ENABLE_DEBUG` flag)

### Production
- [ ] Include `sciter.dll` in installer
- [ ] Package `Resources/` folder
- [ ] Remove `SW_ENABLE_DEBUG` flag
- [ ] Test on clean Windows installation

---

## Troubleshooting

### Window không hiển thị
1. Check `sciter.dll` có trong thư mục exe không
2. Check HTML path trong error message
3. Verify `Resources/` folder structure

### CSS không apply
1. Check relative path trong HTML: `<link rel="stylesheet" href="about.css">`
2. Verify CSS file tồn tại cùng thư mục với HTML
3. Open Sciter Inspector để xem console errors

### JavaScript không chạy
1. Check `<script type="module" src="about.js"></script>`
2. Verify JS file tồn tại
3. Check browser console trong Sciter Inspector

### Assertion errors
1. Ensure `sciter.dll` loaded successfully
2. Don't call JavaScript functions before window fully loaded
3. Check `SOM_PASSPORT` bindings are correct

---

## Known Limitations

1. **Glassmorphism/Backdrop-filter:** Có thể không work trong Sciter, cần dùng alternative effects
2. **JavaScript calls:** Hiện tại disabled, cần implement proper timing
3. **Window management:** Standalone pattern - không integrate với BaseDialog system

---

## Next Steps

1. **Enable JavaScript communication:** Implement proper `call_function()` với timing checks
2. **Add glassmorphism:** Test và implement working blur effects
3. **Implement version display:** Call `setVersionInfo()` sau khi window loaded
4. **Add update check:** Implement `checkUpdate()` callback flow
5. **Polish UI:** Refine animations, colors, spacing

---

## References

- Sciter.JS SDK: https://github.com/c-smile/sciter-js-sdk
- Sciter Documentation: https://sciter.com/docs/
- SOM (Sciter Object Model): https://sciter.com/docs/content/sciter/SOM.htm

---

## Windows DWM Acrylic/Blur Effect Implementation

### Problem
Need true translucent blur effect (see-through to desktop) with rounded corners, without black artifacts.

### Solution Attempts

####  Attempt 1: Native Sciter (FAILED)

**Code:**
```cpp
// C++
AboutDialog::AboutDialog()
    : sciter::window(SW_POPUP | SW_ALPHA | SW_ENABLE_DEBUG, RECT{0, 0, 500, 600}) {
```

```css
/* CSS */
html {
    window-blur: acrylic;
    background: transparent;
}
```

**Result:** Black corners, no blur effect.

**Reason:** Sciter.JS doesn't fully support `window-blur` property like Sciter C++ SDK.

---

####  Attempt 2: DWM C++ API (SUCCESS)

**Full Implementation:**

1. **Add includes** (`AboutDialog.cpp`):
```cpp
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
```

2. **Add method** (`AboutDialog.h`):
```cpp
private:
    void enableAcrylicEffect();
```

3. **Implement** (`AboutDialog.cpp`):
```cpp
// Undocumented Windows API structures
struct ACCENT_POLICY {
    int AccentState;
    int AccentFlags;
    int GradientColor;  // ABGR format
    int AnimationId;
};

struct WINDOWCOMPOSITIONATTRIBDATA {
    int Attrib;
    void* pvData;
    size_t cbData;
};

enum ACCENT_STATE {
    ACCENT_DISABLED = 0,
    ACCENT_ENABLE_GRADIENT = 1,
    ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
    ACCENT_ENABLE_BLURBEHIND = 3,
    ACCENT_ENABLE_ACRYLICBLURBEHIND = 4,  // Windows 10 1803+
    ACCENT_ENABLE_HOSTBACKDROP = 5         // Windows 11 (Mica)
};

void AboutDialog::enableAcrylicEffect() {
    HWND hwnd = get_hwnd();

    // 1. CRITICAL: Set WS_EX_LAYERED style
    SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);

    // 2. Try Acrylic (Windows 10+)
    HMODULE hUser = GetModuleHandle(L"user32.dll");
    if (hUser) {
        typedef BOOL(WINAPI* pSetWindowCompositionAttribute)(HWND, WINDOWCOMPOSITIONATTRIBDATA*);
        auto SetWindowCompositionAttribute = 
            (pSetWindowCompositionAttribute)GetProcAddress(hUser, "SetWindowCompositionAttribute");

        if (SetWindowCompositionAttribute) {
            ACCENT_POLICY policy = { 0 };
            policy.AccentState = ACCENT_ENABLE_ACRYLICBLURBEHIND;
            policy.GradientColor = 0x01FFFFFF;  // ABGR: 1% white tint
            
            WINDOWCOMPOSITIONATTRIBDATA data = { 0 };
            data.Attrib = 19;  // WCA_ACCENT_POLICY
            data.pvData = &policy;
            data.cbData = sizeof(policy);

            SetWindowCompositionAttribute(hwnd, &data);
        }
        else {
            // Fallback: DWM Blur (Windows 7/8)
            DWM_BLURBEHIND bb = { 0 };
            bb.dwFlags = DWM_BB_ENABLE;
            bb.fEnable = TRUE;
            DwmEnableBlurBehindWindow(hwnd, &bb);
        }
    }

    // 3. Fix black corners (Windows 11)
    typedef enum {
        DWMWCP_ROUND = 2
    } DWM_WINDOW_CORNER_PREFERENCE;
    
    DWM_WINDOW_CORNER_PREFERENCE preference = DWMWCP_ROUND;
    DwmSetWindowAttribute(hwnd, 33, &preference, sizeof(preference));
}
```

4. **Call after window creation**:
```cpp
AboutDialog::AboutDialog()
    : sciter::window(SW_POPUP | SW_ALPHA | SW_ENABLE_DEBUG, RECT{0, 0, 500, 600}) {
    
    // Load HTML...
    expand();
    
    // Enable Acrylic AFTER expand()
    enableAcrylicEffect();
}
```

5. **CSS**:
```css
html {
    background: transparent;  /* Essential! */
}

body {
    background: rgba(240, 240, 240, 0.6);  /* Tint layer */
    border-radius: 12px;
}
```

### Key Points

-  **WS_EX_LAYERED** - Must set BEFORE calling SetWindowCompositionAttribute
-  **GradientColor: 0x01FFFFFF** - ABGR format (1% white = nearly transparent)
-  **DWMWCP_ROUND** - Fixes black corners on Windows 11
-  **Fallback to DWM** - For Windows 7/8 compatibility

### Tint Color Guide (ABGR Format)

- `0x01FFFFFF` - 1% white (nearly transparent)
- `0x99000000` - 60% black (dark mode)
- `0xAABBGGRR` - Alpha-Blue-Green-Red

### Common Issues

**Black corners:**
- Ensure `html { background: transparent }` in CSS
- Ensure `SW_ALPHA` flag in C++ constructor
- Call `DwmSetWindowAttribute` with `DWMWCP_ROUND`

**No blur effect:**
- Check `WS_EX_LAYERED` is set
- Verify `SetWindowCompositionAttribute` succeeded
- Test on Windows 10 1803+ or Windows 11


---

## Window Dragging Issue

**Problem:** Sciter.JS doesn't support window dragging well with SW_ALPHA + SW_POPUP layered windows.

**Attempted Solutions:**
-  ehavior: window-move - Not supported
-  low: caption - Not supported  
-  -moz-window-dragging: drag - Not supported
-  JavaScript mousedown/mousemove - Complex, unreliable with Acrylic

**Current Status:** Window dragging is **not implemented**. User must use Alt+Space  Move or taskbar to reposition window.

**Alternative:** Could add a custom title bar with drag handle, but this would interfere with the glassmorphism design.

**Recommendation:** Accept this limitation for the Acrylic effect. The visual quality is worth the trade-off.

