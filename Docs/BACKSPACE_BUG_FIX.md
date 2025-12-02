# Sửa lỗi không gõ được tiếng Việt sau khi xóa ký tự (Backspace)

## Triệu chứng

Khi người dùng gõ nhầm và xóa (backspace) để sửa lại, OpenKey tự động chuyển sang chế độ tiếng Anh và không cho phép gõ tiếng Việt nữa.

**Ví dụ cụ thể**:
- Gõ "vieetj" (do bàn phím bị duplicate hoặc gõ nhầm)
- Xóa (backspace) 3 lần còn lại "vie" (hiển thị "viê")
- Tiếp tục gõ "ejt" để hoàn thành từ "việt"
- **Kết quả thực tế**: "vieejt" (tiếng Anh - không có dấu) ❌
- **Kết quả mong đợi**: "việt" (tiếng Việt - có dấu) ✅

## Nguyên nhân

OpenKey sử dụng 2 buffer song song để xử lý input:
- `TypingWord[]` + `_index`: Lưu các ký tự đã được xử lý (có dấu)
- `KeyStates[]` + `_stateIndex`: Lưu các phím gốc mà user nhấn (raw keys)

**Vấn đề xảy ra khi backspace**:

1. User gõ "vieetj" → `TypingWord[v,i,ê,ê,t,j]`, `KeyStates[v,i,e,e,t,j]`
2. User backspace 3 lần → `_index` giảm xuống 3, nhưng `KeyStates[]` **không được cập nhật**
3. User gõ "e" tiếp theo → `checkSpelling()` kiểm tra `KeyStates[0..3] = "viee"`
4. Spell checker phát hiện "viee" **không hợp lệ** → set `tempDisableKey = true`
5. Engine chuyển sang chế độ tiếng Anh → không xử lý dấu nữa

**Code gây lỗi** (dòng 321 trong `Engine.cpp`):
```cpp
tempDisableKey = !(_spellingOK && _spellingVowelOK);
```

Khi `tempDisableKey = true`, engine không xử lý các phím đặc biệt (s/f/r/x/j) như dấu tiếng Việt nữa.

## Giải pháp

Thêm 3 dòng code vào hàm xử lý backspace (dòng 1507-1522 trong `Engine.cpp`):

```cpp
if (_index > 0){
    _index--;
    // ... existing code ...
    
    // FIX 1: Đồng bộ state index với typing index
    _stateIndex = _index;
    
    // FIX 2: Xóa dữ liệu rác trong KeyStates buffer
    for (i = _index; i < MAX_BUFF; i++) {
        KeyStates[i] = 0;
    }
    
    // FIX 3: Reset flag tiếng Việt để cho phép kiểm tra lại
    tempDisableKey = false;
    
    if (vCheckSpelling)
        checkSpelling();
}
```

**Giải thích**:
- `_stateIndex = _index`: Đồng bộ 2 buffer
- Xóa `KeyStates[_index..]`: Loại bỏ dữ liệu cũ, tránh spell checking đọc nhầm
- `tempDisableKey = false`: Cho phép engine đánh giá lại chế độ tiếng Việt với dữ liệu mới

## Kết quả

Sau khi áp dụng fix:
- ✅ Gõ "vieetj" → xóa còn "vie" → gõ "ejt" → kết quả "việt"
- ✅ Mọi trường hợp xóa và gõ lại đều hoạt động bình thường
- ✅ Không ảnh hưởng đến các tính năng khác
