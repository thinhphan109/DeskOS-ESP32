# mimi.desk Windows Companion v2.12.0-dev

Đợt này hoàn thiện phần vận hành desktop và nhạc. Firmware vẫn ở mốc nguồn v2.11-dev; không build lại hoặc OTA firmware cho thay đổi Python này.

## Đã triển khai

- Mỗi người dùng chỉ có một phiên GUI chạy thật. Mở lần hai đưa cửa sổ đang chạy lên trước, kể cả từ khay hệ thống. DEMO có phiên riêng nên có thể mở cạnh bản thật. Khoá tiến trình và IPC cục bộ xử lý trường hợp mở đồng thời hoặc tiến trình cũ thoát bất thường.
- `MediaMonitor` đọc Windows Media Controls trên luồng riêng, có cache metadata và ảnh bìa. Không cần cấu hình ESP32 hoặc gửi telemetry thành công mới đọc được nhạc. Worker mạng lấy cache để chuyển tiếp; không đọc provider lần thứ hai.
- Provider tìm trong các phiên media đã tồn tại khi mở app. Giữ trình phát hiện tại nếu đang phát; khi phiên hiện tại trống/lỗi hoặc đã dừng, tìm phiên đang phát khác. Spotify được ưu tiên khi các ứng viên còn lại tương đương. Một phiên đã đóng không làm mất các phiên còn lại.
- Phân biệt thiếu provider, lỗi đọc provider, chưa có phiên nhạc và người dùng đã tắt đọc nhạc. Giới hạn thời gian chờ bất đồng bộ cho metadata, ảnh bìa và điều khiển nhạc. Ảnh bìa xuất hiện sau metadata được gửi ở heartbeat tiếp theo, không phải chờ backoff của một lượt upload lỗi.
- App dùng nhạc cục bộ khi provider đang bật; trạng thái nhạc cũ từ ESP32 không ghi đè mẫu nhạc mới trên PC. Callback cũ sau khi đổi cài đặt hoặc thoát được bỏ qua.
- Telemetry, HTTP trạng thái và WebSocket có backoff tăng dần, tối đa 30 giây. Kết nối thành công đặt lại lịch thử. Sai mã ghép nối có thông báo riêng và không giữ badge xanh Đã kết nối chỉ nhờ đọc được trạng thái công khai.
- Ghi nhận `device_id` từ thiết bị đã chọn. Sau mất liên lạc, tìm lại qua địa chỉ đã lưu, mDNS và AP dự phòng; chỉ chuyển địa chỉ khi ID trùng. Không tự quét toàn mạng LAN; nút quét thủ công vẫn còn. Giữ trạng thái tạm dừng của agent khi tìm lại thiết bị.
- Worker xác minh ID đã lưu trước khi bắt đầu gửi tới thiết bị và sau lỗi kết nối. Dừng xử lý lệnh đến muộn khi thoát. Phiên app mới không tự chạy lại thao tác cập nhật firmware.

## Dependency nhạc Windows

Môi trường Python đang dùng ban đầu thiếu WinRT. Đã cài các dependency trong môi trường phát triển; provider sau đó nhận được `Spotify.exe` ở trạng thái tạm dừng và đọc được ảnh bìa RGB565 8.192 byte.

`requirements-gui.txt` chỉ khai báo các namespace thực sự dùng: Media.Control, Media và Storage.Streams. Bỏ extra `[all]` vì extra này kéo theo nhiều API Windows không dùng. Đã tạo môi trường Python tối thiểu riêng và xác nhận đọc được Spotify cùng ảnh bìa với bộ dependency này. Hướng dẫn của PyWinRT: [cài namespace cần dùng](https://pywinrt.readthedocs.io/en/latest/installing/), [đóng gói ứng dụng](https://pywinrt.readthedocs.io/en/latest/freezing/).

## Kiểm tra tập trung

37 kiểm thử trong ba module `test_companion`, `test_desktop_services` và `test_gui_smoke` đã qua. Phạm vi gồm khoá/đánh thức giữa hai tiến trình thật, phục hồi khoá sau tiến trình thoát không dọn dẹp, chạy song song DEMO/bản thật, chọn phiên media, đọc nhạc khi mạng lỗi, cache ảnh bìa, backoff, nhận diện sai thiết bị, đổi địa chỉ giữ tạm dừng và bỏ callback muộn.

Đã chạy app Qt thật với provider nhạc thật, không cấu hình thiết bị: hiển thị đúng tên bài và ảnh bìa Spotify, không tạo worker thiết bị hoặc gửi request tới ESP32. Luồng nhạc dừng sạch khi thoát. Ảnh có nội dung nhạc cá nhân chỉ được giữ tạm trên máy, không commit lên Git.

Không chạy lại toàn bộ bộ kiểm thử, không kiểm tra TFT hoặc thay đổi firmware. Chưa nghiệm thu DHCP/mDNS bằng cách đổi IP bo thật và chưa chạy bản EXE mới trên Windows sạch.

## Áp dụng và phần tiếp theo

Thoát hẳn các phiên cũ từ khay hệ thống rồi mở lại source/app mới. Các tiến trình cũ chưa có cơ chế một phiên nên không tham gia khoá mới. Cấu hình cũ và mã ghép nối vẫn dùng đường dẫn/kho lưu hiện tại. Các script CLI/tray đời cũ là công cụ dự phòng độc lập; không chạy đồng thời với GUI đang gửi dữ liệu.

OTA server ở đợt tiếp theo: giữ hai chế độ đã chốt, một lượt đang chạy và một bản mới nhất đang chờ. Chưa dựng hosting hoặc kích hoạt tự cập nhật. Đóng gói EXE, thiết lập lần đầu, phản hồi lưu/xung đột trên Studio và nghiệm thu phần cứng vẫn là các mục còn lại trước bản RC.
