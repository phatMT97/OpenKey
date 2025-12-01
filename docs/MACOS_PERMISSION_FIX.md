# 🔧 Hướng Dẫn Sửa Lỗi Quyền OpenKey trên macOS

> **Dành cho**: Người dùng gặp lỗi OpenKey liên tục hỏi quyền Accessibility hoặc không hoạt động dù đã cấp quyền.

---

## 🎯 Triệu Chứng

Bạn đang gặp một trong các vấn đề sau:

- ✅ Đã cấp quyền Accessibility cho OpenKey trong System Settings
- ❌ Nhưng OpenKey vẫn không gõ được tiếng Việt
- ❌ Hoặc liên tục hiện popup yêu cầu cấp quyền lại
- ❌ Hoặc bảng điều khiển hiện lên rồi biến mất sau vài giây

---

## 🔍 Nguyên Nhân

Đây là lỗi **App Translocation** - tính năng bảo mật của macOS:

- macOS phát hiện OpenKey "tải từ internet"
- Tạo đường dẫn ngẫu nhiên để chạy app (bảo mật)
- Đường dẫn thay đổi mỗi lần chạy
- Quyền Accessibility bị mất vì đường dẫn khác

---

## ✅ Giải Pháp (Làm Theo Từng Bước)

### Bước 1: Thoát OpenKey Hoàn Toàn

```bash
# Mở Terminal (Cmd+Space → gõ "Terminal")
# Copy và paste lệnh này:
killall OpenKey
```

Hoặc:
- Mở Activity Monitor
- Tìm "OpenKey"
- Click "X" để thoát

---

### Bước 2: Kiểm Tra OpenKey Đã Ở Đúng Chỗ Chưa

OpenKey **BẮT BUỘC** phải ở thư mục `/Applications`

**Cách kiểm tra**:
1. Mở Finder
2. Vào thư mục **Applications** (Ứng dụng)
3. Tìm **OpenKey.app**

**Nếu KHÔNG thấy**:
- Kéo OpenKey.app từ Downloads/Desktop vào Applications
- Hoặc tải lại từ [Releases](https://github.com/phatMT97/OpenKey/releases)

---

### Bước 3: Xóa "Quarantine Flag" (QUAN TRỌNG!)

Đây là bước **QUAN TRỌNG NHẤT**!

```bash
# Mở Terminal
# Copy và paste lệnh này:
xattr -cr /Applications/OpenKey.app
```

**Lệnh này làm gì?**
- Xóa cờ "tải từ internet"
- Ngăn macOS tạo đường dẫn ngẫu nhiên
- Cho phép OpenKey chạy ổn định

**Verify**:
```bash
# Kiểm tra xem đã xóa chưa:
xattr -l /Applications/OpenKey.app

# Nếu KHÔNG hiện gì = Thành công ✅
# Nếu vẫn thấy "com.apple.quarantine" = Chưa xóa được
```

---

### Bước 4: Reset Quyền TCC

```bash
# Mở Terminal
# Copy và paste lệnh này:
tccutil reset Accessibility com.tuyenmai.openkey
```

**Kết quả mong đợi**:
```
Successfully reset Accessibility approval status for com.tuyenmai.openkey
```

**Nếu báo lỗi**:
- Bỏ qua bước này
- Chuyển sang Bước 5

---

### Bước 5: Mở OpenKey Từ /Applications

**QUAN TRỌNG**: Phải mở từ `/Applications`, KHÔNG dùng Spotlight!

**Cách 1: Dùng Finder**
1. Mở Finder
2. Vào thư mục **Applications**
3. Double-click **OpenKey.app**

**Cách 2: Dùng Terminal**
```bash
open /Applications/OpenKey.app
```

---

### Bước 6: Cấp Quyền Accessibility

1. **Popup sẽ hiện ra**: "OpenKey cần bạn cấp quyền..."
2. Click **"Cấp quyền"**
3. **System Settings mở ra**
4. Tìm **OpenKey** trong danh sách
5. **Toggle ON** (bật)
6. **Đợi 2-5 giây** (QUAN TRỌNG!)
7. OpenKey sẽ **tự động khởi động** - KHÔNG cần mở lại!

---

## 🎉 Xong!

Bây giờ OpenKey sẽ:
- ✅ Gõ tiếng Việt bình thường
- ✅ Không hỏi quyền lại
- ✅ Hoạt động ổn định

---

## 🔄 Khi Nào Cần Làm Lại?

**Phải làm lại** khi:
- ❌ Update OpenKey lên phiên bản mới
- ❌ Tải lại OpenKey từ internet
- ❌ Di chuyển OpenKey sang máy khác

**KHÔNG cần làm lại** khi:
- ✅ Restart máy
- ✅ Sleep/Wake máy
- ✅ Chỉ dùng bình thường

---

## ⚠️ Lưu Ý Đặc Biệt

### Về Bản GitHub Actions

Nếu bạn dùng bản build từ GitHub Actions (không phải bản gốc):

**Đặc điểm**:
- Dùng "ad-hoc signing" (không có Apple Certificate)
- **Mỗi lần update** phải làm lại các bước trên
- Đây là hạn chế kỹ thuật, KHÔNG phải bug

**Nếu muốn UX tốt hơn**:
- Dùng bản gốc từ [tuyenvm/OpenKey](https://github.com/tuyenvm/OpenKey)
- Bản gốc có Developer ID Certificate
- Không cần reset quyền khi update

---

## 🆘 Troubleshooting

### Vấn đề 1: Lệnh `xattr -cr` báo lỗi

**Lỗi**: `Operation not permitted`

**Fix**:
```bash
# Thử với sudo:
sudo xattr -cr /Applications/OpenKey.app
# Nhập password máy khi được hỏi
```

---

### Vấn đề 2: Lệnh `tccutil reset` báo lỗi

**Lỗi**: `Failed to reset`

**Fix**: Dùng GUI thay vì Terminal
1. Mở  → System Settings
2. Privacy & Security → Accessibility
3. Tìm OpenKey
4. Click nút **"-"** để xóa
5. Confirm xóa
6. Chuyển sang Bước 5 (mở OpenKey lại)

---

### Vấn đề 3: Sau khi cấp quyền vẫn không hoạt động

**Nguyên nhân**: Chưa đợi đủ lâu

**Fix**:
- Đợi **5-10 giây** sau khi toggle ON
- OpenKey sẽ tự động khởi động
- KHÔNG click mở OpenKey lại!

---

### Vấn đề 4: Vẫn không được sau khi làm hết các bước

**Kiểm tra lại**:

```bash
# 1. Kiểm tra OpenKey đang chạy từ đâu:
ps aux | grep OpenKey

# Phải thấy: /Applications/OpenKey.app
# KHÔNG được thấy: /private/var/folders/.../AppTranslocation/...
```

**Nếu vẫn thấy "AppTranslocation"**:
```bash
# Xóa hẳn và cài lại:
rm -rf /Applications/OpenKey.app
# Tải lại từ Releases
# Kéo vào /Applications
# Làm lại từ Bước 3
```

---

## 📞 Cần Hỗ Trợ?

Nếu vẫn không được sau khi làm hết các bước:

1. **GitHub Issues**: [Tạo issue mới](https://github.com/phatMT97/OpenKey/issues)
2. **Discussions**: [Hỏi đáp](https://github.com/phatMT97/OpenKey/discussions)
3. **Cung cấp thông tin**:
   ```bash
   # Chạy các lệnh này và paste kết quả vào issue:
   
   # 1. Kiểm tra OpenKey path:
   ps aux | grep OpenKey
   
   # 2. Kiểm tra quarantine:
   xattr -l /Applications/OpenKey.app
   
   # 3. Kiểm tra macOS version:
   sw_vers
   ```

---

## 📚 Tài Liệu Liên Quan

- [README.md](../README.md) - Hướng dẫn cài đặt
- [App Translocation Diagnosis](./APP_TRANSLOCATION_DIAGNOSIS.md) - Giải thích kỹ thuật
- [Developer ID vs Ad-hoc](./DEVELOPER_ID_VS_ADHOC.md) - So sánh signing methods

---

**Chúc bạn sử dụng OpenKey vui vẻ!** 🎉
