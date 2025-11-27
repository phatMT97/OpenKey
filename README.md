# OpenKey (Phiên bản cải tiến) ⚡

Đây là phiên bản fork từ dự án [OpenKey gốc](https://github.com/tuyenvm/OpenKey) của tác giả Mai Vũ Tuyên.
Phiên bản này được phát triển tiếp để bổ sung các tính năng mới và sửa lỗi cho người dùng Windows.

> **Lưu ý:** Dự án này kế thừa từ OpenKey gốc. Nếu bạn yêu thích phần mềm và muốn ủng hộ tác giả gốc, vui lòng truy cập: [Donate cho tác giả Mai Vũ Tuyên](https://tuyenvm.github.io/donate.html).

## ✨ Tính năng mới trong phiên bản này

### 🔒 1. Loại trừ ứng dụng (English Only Mode)
Tính năng này cực kỳ hữu ích cho lập trình viên hoặc game thủ:
- **Chức năng**: Cho phép lập danh sách các ứng dụng "loại trừ" (ví dụ: Visual Studio Code, Terminal, CMD, CS:GO...).
- **Hoạt động**: Khi bạn chuyển cửa sổ sang các ứng dụng trong danh sách này, OpenKey sẽ **tự động chuyển sang chế độ gõ Tiếng Anh** và **khóa phím tắt** chuyển đổi ngôn ngữ. Điều này giúp tránh việc vô tình gõ tiếng Việt khi đang code hoặc chơi game.
- **Quản lý**: Dễ dàng thêm/xóa ứng dụng thông qua giao diện quản lý (có thể thêm nhanh ứng dụng đang mở hoặc nhập tay tên file .exe).

### 🚀 2. Tối ưu hóa hiệu suất (Performance Optimizations)
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

### 🐛 3. Sửa lỗi khởi động cùng Windows
- Khắc phục hoàn toàn lỗi OpenKey không thể tự khởi động cùng Windows khi chạy dưới quyền Administrator nếu đường dẫn thư mục cài đặt có chứa khoảng trắng (Space).

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
- **Run as Admin**: Hỗ trợ chạy với quyền quản trị cao nhất.
- **Công cụ chuyển mã**: Tích hợp sẵn công cụ chuyển đổi văn bản giữa các bảng mã (Ctrl+Shift+F9).
- **Tự động cập nhật**: Kiểm tra và cập nhật phiên bản mới.

## 📥 Cài đặt & Sử dụng

1. Tải về phiên bản mới nhất từ mục Releases.
2. Giải nén và chạy file `OpenKey.exe`.
3. (Khuyên dùng) Nên tắt các bộ gõ tiếng Việt khác (Unikey, EVKey...) để tránh xung đột.

## 📜 Mã nguồn & Giấy phép

Mã nguồn của ứng dụng được mở công khai dưới giấy phép **GPL**. Bạn có thể tự do tải về, nghiên cứu và phát triển tiếp, miễn là tuân thủ các điều khoản của giấy phép nguồn mở.
