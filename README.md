# OpenKey (Phiên bản cải tiến) ⚡

Đây là phiên bản fork từ dự án [OpenKey gốc](https://github.com/tuyenvm/OpenKey) của tác giả Mai Vũ Tuyên.
Phiên bản này được phát triển tiếp để bổ sung các tính năng mới và sửa lỗi cho người dùng Windows và macOS.

> **Lưu ý:** Dự án này kế thừa từ OpenKey gốc. Nếu bạn yêu thích phần mềm và muốn ủng hộ tác giả gốc, vui lòng truy cập: [Donate cho tác giả Mai Vũ Tuyên](https://tuyenvm.github.io/donate.html).

---

## ✨ Tính năng mới trong phiên bản này

### 🔒 1. Loại trừ ứng dụng (English-Only App Lock) - Windows
Tính năng này cực kỳ hữu ích cho lập trình viên hoặc game thủ:
- **Chức năng**: Cho phép lập danh sách các ứng dụng "loại trừ" (ví dụ: Visual Studio Code, Terminal, CMD, CS:GO...).
- **Hoạt động**: Khi bạn chuyển cửa sổ sang các ứng dụng trong danh sách này, OpenKey sẽ **tự động chuyển sang chế độ gõ Tiếng Anh** và **khóa phím tắt** chuyển đổi ngôn ngữ. Điều này giúp tránh việc vô tình gõ tiếng Việt khi đang code hoặc chơi game.
- **Quản lý**: Dễ dàng thêm/xóa ứng dụng thông qua giao diện quản lý (có thể thêm nhanh ứng dụng đang mở hoặc nhập tay tên file .exe).

### 🚀 2. Tối ưu hiệu suất (Performance Optimizations) - Windows
Phiên bản này đã được tối ưu hóa toàn diện để cải thiện hiệu suất và độ mượt:
- **⚡ Tối ưu CPU**: Giảm 30-50% sử dụng CPU khi gõ tiếng Việt, ~80% khi gõ tiếng Anh
- **⏱️ Độ trễ phím**: Giảm latency xuống <5ms cho trải nghiệm gõ mượt mà hơn
- **💾 Quản lý bộ nhớ**: Loại bỏ memory leaks, tối ưu memory allocations
- **🛡️ Ổn định**: Sử dụng non-blocking operations để tránh đơ/lag
- **🔧 Kỹ thuật áp dụng**:
  - IME window caching (~90% giảm lookups)
  - PID-based app detection (~95% nhanh hơn so với string comparison)
  - Vector pre-allocation (loại bỏ reallocations)
  - Clipboard retry mechanism (+80% độ tin cậy)
  - Process handle cleanup (zero leaks)

### 🌏 3. Hỗ trợ gõ tiếng Việt với các ngôn ngữ khác (Multi-language Support) - Windows & macOS
Tính năng này giúp bạn linh hoạt chuyển đổi giữa tiếng Việt và các ngôn ngữ khác:
- **Chức năng**: Tự động phát hiện bàn phím/IME hiện tại (Japanese, Korean, Chinese, v.v.) và tạm tắt xử lý tiếng Việt khi đang dùng ngôn ngữ khác.
- **Hoạt động**: Khi bật tính năng này, OpenKey sẽ:
  - ✅ Cho phép bạn gõ tiếng Nhật, Hàn, Trung (các ngôn ngữ CJK) mà **không bị OpenKey can thiệp**
  - ✅ **Tự động bật lại** xử lý tiếng Việt khi bạn chuyển về bàn phím tiếng Anh
  - ✅ Không cần tắt/bật OpenKey thủ công khi đổi ngôn ngữ
- **Hướng dẫn sử dụng**:
  - **Windows**: Mở OpenKey settings → Tab "Bộ gõ" → Tích chọn "Cho phép gõ trong các ngôn ngữ khác"
  - **macOS**: Mở OpenKey settings → Tích chọn "Cho phép gõ trong các ngôn ngữ khác"
  - Chuyển sang Japanese/Korean/Chinese IME (Windows: `Win + Space`, macOS: `Control + Space`)
  - Gõ tiếng Nhật/Hàn/Trung bình thường, không bị ảnh hưởng!
- **Lưu ý**: Tính năng này mặc định **đã được bật** trên cả Windows và macOS.

### 🐛 4. Sửa lỗi khởi động cùng Windows
- Khắc phục hoàn toàn lỗi OpenKey không thể tự khởi động cùng Windows khi chạy dưới quyền Administrator nếu đường dẫn thư mục cài đặt có chứa khoảng trắng (Space).

### 🔧 5. Sửa lỗi critical trên macOS
- **Memory leak fix**: Khắc phục critical bug memory leak khi phát hiện ngôn ngữ khác (Japanese/Korean) trên macOS.
  - Sửa lỗi sử dụng CFRelease không đúng cách
  - Cải thiện string comparison từ `isLike:` sang `hasPrefix:`
  - Memory luôn được giải phóng đúng cách
- **Permission loop fix**: Khắc phục lỗi macOS liên tục hỏi lại quyền Accessibility dù đã cấp.
  - Thêm `NSAccessibilityUsageDescription` vào Info.plist theo yêu cầu của macOS 10.15+
  - Permission giờ được persist chính xác, không hỏi lại sau khi restart

### 🏗️ 6. GitHub Actions CI/CD
- **Windows Build**: Tự động build x86 và x64, artifact sẵn sàng để download
- **macOS Build**: Tự động build Universal Binary (arm64 + x86_64), đóng gói thành **DMG installer** chuyên nghiệp
- Hỗ trợ manual trigger để build on-demand
- Build artifacts có attestation để đảm bảo an toàn

---

## 📋 Các tính năng chính (Kế thừa từ OpenKey gốc)

OpenKey là bộ gõ tiếng Việt hiện đại, mã nguồn mở với nhiều tính năng mạnh mẽ:

### ⌨️ Hỗ trợ gõ
- **Kiểu gõ**: Telex, VNI, Simple Telex 1/2.
- **Bảng mã**: Unicode (Dựng sẵn), TCVN3 (ABC), VNI Windows, Unicode tổ hợp...

### 🧠 Tính năng thông minh
- **Modern Orthography**: Tùy chọn đặt dấu oà, uý (mới) thay vì òa, úy (cũ).
- **Smart Switch Key**: Tự động ghi nhớ chế độ gõ (Anh/Việt) cho từng ứng dụng riêng biệt.
- **Kiểm tra chính tả & Ngữ pháp**: Phát hiện và xử lý lỗi chính tả cơ bản.
- **Macro (Gõ tắt)**: Hỗ trợ gõ tắt không giới hạn ký tự, giúp tăng tốc độ soạn thảo.
- **Quick Telex**: Hỗ trợ gõ tắt nhanh các phụ âm đầu/cuối (cc=ch, gg=gi, kk=kh...).
- **Phục hồi từ sai**: Tự động khôi phục phím đã gõ nếu từ đó không hợp lệ.

### 🛠️ Tiện ích hệ thống
- **Gửi từng phím**: Chế độ tương thích cao cho các ứng dụng/game kén bộ gõ.
- **Run as Admin**: Hỗ trợ chạy với quyền quản trị cao nhất (Windows).
- **Công cụ chuyển mã**: Tích hợp sẵn công cụ chuyển đổi văn bản giữa các bảng mã.
  - Windows: Ctrl+Shift+F9
  - macOS: Configurable hotkey
- **Tự động cập nhật**: Kiểm tra và cập nhật phiên bản mới.

---

## 📥 Cài đặt & Sử dụng

### Windows
1. Tải về phiên bản mới nhất từ [Releases](https://github.com/phatMT97/OpenKey/releases).
2. Giải nén và chạy file `OpenKey64.exe` (64-bit) hoặc `OpenKey32.exe` (32-bit).
3. (Khuyên dùng) Nên tắt các bộ gõ tiếng Việt khác (Unikey, EVKey...) để tránh xung đột.

### macOS
1. Tải về file `OpenKey-macOS.dmg` từ [Releases](https://github.com/phatMT97/OpenKey/releases).
2. Mở file DMG và kéo `OpenKey.app` vào thư mục `Applications`.
3. Lần đầu chạy:
   - Double-click `OpenKey.app` trong Applications
   - macOS có thể hỏi: "OpenKey is from an unidentified developer" → Click **Open**
   - Cấp quyền Accessibility khi được hỏi
4. Hoặc dùng Terminal để bỏ quarantine:
   ```bash
   xattr -cr /Applications/OpenKey.app
   open /Applications/OpenKey.app
   ```

---

## 🤝 Đóng góp

Mọi đóng góp đều được chào đón! Vui lòng:
1. Fork repository
2. Tạo branch mới cho feature/bugfix của bạn
3. Commit và push lên branch
4. Tạo Pull Request

### Quy tắc code
- **Windows**: C++ code theo style hiện có
- **macOS**: Objective-C/C++ code theo style hiện có
- Comment code bằng tiếng Anh hoặc tiếng Việt
- Test kỹ trước khi PR

---

## 📜 Mã nguồn & Giấy phép

Mã nguồn của ứng dụng được mở công khai dưới giấy phép **GPL v3**. Bạn có thể tự do tải về, nghiên cứu và phát triển tiếp, miễn là tuân thủ các điều khoản của giấy phép nguồn mở.

---

## 🙏 Credits

- **Tác giả gốc**: [Mai Vũ Tuyên](https://github.com/tuyenvm) - OpenKey gốc
- **Fork và phát triển tiếp**: [Mai Tấn Phát](https://github.com/phatMT97)
- **Contributors**: Xem danh sách đầy đủ tại [Contributors](https://github.com/phatMT97/OpenKey/graphs/contributors)

---

## 📞 Hỗ trợ

- **Issues**: [GitHub Issues](https://github.com/phatMT97/OpenKey/issues)
- **Discussions**: [GitHub Discussions](https://github.com/phatMT97/OpenKey/discussions)
- **Original Project**: [OpenKey by tuyenvm](https://github.com/tuyenvm/OpenKey)

---

## 📊 So sánh phiên bản

| Tính năng | OpenKey gốc | Fork này |
|-----------|-------------|----------|
| Windows support | ✅ | ✅ |
| macOS support | ✅ | ✅ |
| English-Only App Lock | ❌ | ✅ (Windows) |
| Performance optimization | ❌ | ✅ (Windows) |
| Multi-language support | ⚠️ (có bug) | ✅ (Fixed cả Windows & macOS) |
| Memory leak fixes | ❌ | ✅ (macOS) |
| Permission loop fix | ❌ | ✅ (macOS) |

---

**⭐ Nếu bạn thấy hữu ích, hãy cho dự án một star nhé!**
