# 🔓 Chi tiết kỹ thuật: Sửa lỗi phím tắt sau Lock/Unlock Windows

## 📋 Vấn đề

Sau khi khóa màn hình Windows (Win+L) và đăng nhập lại, phím tắt chuyển đổi Anh-Việt của OpenKey ngưng hoạt động. Người dùng phải restart ứng dụng để khôi phục chức năng.

### Nguyên nhân

1. **Desktop Isolation**: Windows lock screen chạy trên "Secure Desktop" (Winlogon Desktop), trong khi OpenKey hooks được cài đặt trên "User Desktop"
2. **Hook Lifetime**: Windows hooks không tự động chuyển đổi giữa các desktop contexts
3. **No Auto-Reconnection**: Sau khi unlock, hooks không tự động kết nối lại

## ✅ Giải pháp

### Phương pháp: Session Notification + Timer-based Hook Reinstallation

#### 1. Phát hiện Lock/Unlock Events
```cpp
// Đăng ký nhận thông báo session changes
WTSRegisterSessionNotification(hWnd, NOTIFY_FOR_THIS_SESSION);

// Xử lý sự kiện
case WM_WTSSESSION_CHANGE:
    if (wParam == WTS_SESSION_UNLOCK) {
        // Lên lịch reinstall hooks sau 500ms
        SetTimer(hWnd, TIMER_REINSTALL_HOOKS, 500, NULL);
    }
    break;
```

#### 2. Reinstall Hooks từ Main Thread
```cpp
// Timer fires trong main thread (có message loop)
case WM_TIMER:
    if (wParam == TIMER_REINSTALL_HOOKS) {
        KillTimer(hWnd, TIMER_REINSTALL_HOOKS);
        OpenKeyManager::reinstallHooks();
    }
    break;
```

**Tại sao dùng Timer thay vì Thread?**
- Low-level hooks YÊU CẦU được cài từ thread có message loop
- Worker thread KHÔNG có message loop → hooks không nhận events
- Main thread (WndProc) CÓ message loop → hooks hoạt động bình thường

#### 3. Resync Keyboard State
```cpp
void ReinstallHooks() {
    // 1. Unhook old hooks
    UnhookWindowsHookEx(hKeyboardHook);
    UnhookWindowsHookEx(hMouseHook);
    
    // 2. Reset state variables
    _lastFlag = 0;
    _keycode = 0;
    _hasJustUsedHotKey = false;
    
    // 3. CRITICAL: Resync với trạng thái bàn phím hiện tại
    _flag = 0;
    if (GetKeyState(VK_LSHIFT) < 0 || GetKeyState(VK_RSHIFT) < 0) 
        _flag |= MASK_SHIFT;
    if (GetKeyState(VK_LCONTROL) < 0 || GetKeyState(VK_RCONTROL) < 0) 
        _flag |= MASK_CONTROL;
    // ... tất cả modifier keys
    
    // 4. Reinstall hooks
    hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboardHookProcess, hInstance, 0);
    hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, mouseHookProcess, hInstance, 0);
}
```

**Tại sao cần resync `_flag`?**
- Khi lock, user có thể đang giữ phím (Ctrl, Shift...)
- Sau unlock, các phím đã được thả nhưng `_flag` vẫn giữ state cũ
- Hotkey detection so sánh `_flag` vs `_lastFlag` → sai lệch → fail
- Resync với `GetKeyState()` đảm bảo state chính xác

## 🔧 Implementation Details

### Files Modified
- `SystemTrayHelper.cpp`: Session notification + timer handler
- `OpenKey.cpp`: ReinstallHooks function với state resync
- `OpenKeyManager.h/.cpp`: Wrapper function

### Key Components
1. **Debouncing**: Chỉ xử lý unlock events cách nhau ≥2 giây
2. **Delay**: 500ms sau unlock để desktop ổn định
3. **Thread Safety**: Mutex protection trong ReinstallHooks
4. **Cleanup**: KillTimer + Unregister trong WM_DESTROY

## 📊 Kết quả

- ✅ Phím tắt hoạt động ngay sau unlock
- ✅ Gõ tiếng Việt hoạt động bình thường
- ✅ Không cần restart ứng dụng
- ✅ Ổn định, không crash
- ✅ Performance impact tối thiểu

## 🎓 Bài học

1. **Message Loop is Critical**: Low-level hooks phải từ thread có message loop
2. **State Synchronization**: Không chỉ reset về 0, phải resync với actual state
3. **Desktop Isolation**: Windows security model ảnh hưởng hook lifetime
4. **Timer > Thread**: Cho UI apps, timer approach đơn giản và reliable hơn
