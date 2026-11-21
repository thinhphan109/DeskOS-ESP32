# OTA server v2.13.0-dev

Đã triển khai reader trên ESP32, API chính sách và bộ tạo kênh phát hành cục bộ. Chưa cấu hình hosting, phát hành binary lên Internet hoặc nạp firmware vào thiết bị. Mặc định manifest URL trống, chế độ thông báo, hoãn khi Focus đang chạy.

## Luồng đã có

ESP32 tải manifest ở luồng FreeRTOS riêng, theo HTTPS với certificate bundle của ESP-IDF. HTTP chỉ dành cho địa chỉ LAN/mDNS khi debug. Manifest được giới hạn 8 KiB; firmware được chuyển qua hàng đợi bốn khối 4 KiB, rồi engine A/B hiện có ghi vào slot không chạy. Không giữ toàn bộ firmware trong RAM. Sau xác minh hash/image, thiết bị lên lịch reboot và đối chiếu phiên bản/health khi lên lại.

Chính sách nằm trong NVS: mode notify/auto, channel preview/stable, URL, chu kỳ 60–86.400 giây, hoãn Focus và revision. Chỉ ghi khi thay đổi. API yêu cầu revision khi lưu; command ID chống nhận lặp thao tác kiểm tra/bắt đầu. Manifest kiểm tra board, layout, SemVer, phiên bản cầu nối tối thiểu, kích thước, SHA-256 và sequence.

Chỉ giữ một bản mới nhất đang chờ. Bản đang ghi giữ manifest đã chọn; không nhận bản khác chen ngang. Yêu cầu thủ công đã được nhận giữ bản đích kể cả khi manifest kênh đổi lúc đang chờ Focus. Các lượt OTA/CatPack dùng chung lease; lease giữ tới reboot hoặc giải phóng khi thất bại. Auto có báo trước 10 giây, chờ kiểm tra sức khoẻ khởi động, backoff lỗi và tối đa ba lần thử trên một release. Trở về bản cũ sau reboot sẽ tạm ngừng tự thử release đó.

## API

- `GET /api/v1/update-policy`: policy và update state, có pairing key.
- `POST /api/v1/update-policy`: policy đầy đủ và `expected_revision`.
- `POST /api/v1/update/check`: `command_id`.
- `POST /api/v1/update/start`: `release_id`, `expected_revision`, `command_id`.
- Status chung có `update_policy` và `update_state`; URL không đưa vào status công khai.
- `GET /api/v1/pairing`: chỉ trả mã khi máy khách ở mạng AP thiết bị và thiết bị chưa kết nối router; dùng cho thiết lập lần đầu của firmware chung.

## Firmware cầu nối và build chung

Build `esp32-s3-devkitc-1` vẫn đọc cấu hình riêng ở máy, phục vụ lượt cầu nối. Khi chạy, nó lưu Wi-Fi, mã ghép nối và cấu hình AP vào NVS nếu chưa có. Build `mimi-public` đọc header công khai rỗng, sử dụng cấu hình NVS đã chuyển; thiết bị mới sinh mã riêng và dùng AP thiết lập. Không thay bootloader hoặc bảng phân vùng.

`firmware/tools/publish_update.py` chỉ nhận binary có marker build công khai đúng phiên bản. Nó tạo `releases/<id>/firmware.bin`, manifest bất biến và `channels/<channel>.json`, kiểm tra kênh không lùi phiên bản/sequence, khoá publisher và đổi file con trỏ nguyên tử sau cùng. Công cụ không upload hoặc tự tạo GitHub Release. Không dùng binary cầu nối riêng để phát hành công khai.

## Kiểm tra và giới hạn

Đã chạy kiểm thử native SemVer/lease và hai engine OTA/CatPack thật với API Flash mô phỏng; kiểm thử publisher kiểm tra sai board/layout/slot, downgrade, thay nội dung release và từ chối build riêng. Firmware được build với toolchain Arduino hiện tại. Giao diện quản lý và quy trình đóng gói RC được hoàn thiện ở các đợt tiếp theo.

Các kiểm tra này chưa chứng minh mạng TLS/DHCP, mức heap khi TLS cùng renderer, tốc độ ghi, mất nguồn hoặc rollback trên bo. Chưa bật Auto ở thiết bị đang dùng. Cơ chế rollback vẫn phụ thuộc bootloader đã cài; không coi cờ cấu hình biên dịch là bằng chứng đã nghiệm thu.
