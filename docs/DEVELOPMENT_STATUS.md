# Tình hình phát triển mimi.desk

Cập nhật ngày 21/11/2026. Firmware và Windows Companion đã đến **2.15.0-rc.1**, sẵn để xem xét trước deploy. Các bản nguồn được kiểm thử và push theo từng mốc; việc nghiệm thu trên thiết bị tách riêng với kết quả chạy trên máy tính. Xem [bàn giao RC](RELEASE_CANDIDATE_V215.md) và [tracker](COMPLETION_TRACKER.md).

| Mốc | Đã có trong mã nguồn | Giới hạn còn lại |
|---|---|---|
| v1.8 | Performance Lab, lịch vẽ đồng hồ theo giây, Studio gọn theo tab, trạng thái chung qua WebSocket | Cần đo TFT thực tế; DMA wait chưa phải toàn bộ thời gian chiếm bus SPI |
| v1.9 | Manifest/hash/phiên bản OTA, lịch sử cập nhật, sao lưu cấu hình, Focus tuỳ chỉnh | Cần thử OTA và phục hồi cấu hình trên bo; chưa chứng minh rollback vật lý |
| v2.0 | Hợp đồng widget, vùng vẽ độc lập, scene tuỳ chỉnh ID 14 | Giữ renderer hiện tại; chưa thay toàn bộ các trang cũ thành component |
| v2.1 | Kéo widget, chỉnh kích thước/toạ độ, nhập/xuất JSON trên Web và Windows | Xem trước là bố cục; cần kiểm tra chữ và màu trên TFT |
| v2.2 | Ngủ/thức theo idle thật, chào buổi sáng, ưu tiên Focus/cảnh báo | Dùng bộ ảnh hiện có; chưa có bộ animation nhiều frame mới |
| v2.3 | Cozy, Midnight, Auto, Cyberpunk, Sakura, Minimal | Là các palette trên bố cục hiện tại; chưa có theme pack độc lập |
| v2.4 | Metadata/album art/điều khiển nhạc hiện có, timeline đồng nhất, dừng nội suy khi mất kết nối | Cần nghiệm thu với các trình phát thật; chưa phân tích beat âm thanh |
| v2.5 | Biểu đồ PC 60 giây trên Windows, khoảng trống dữ liệu và tách DEMO | Nhiệt độ/quạt phụ thuộc cảm biến và provider; không dựng số liệu thay thế |
| v2.6 | Nhật ký Focus quan sát được, nhắc nghỉ tuỳ chọn, khởi động cùng Windows, EXE từ CI | Nhật ký nằm ở Windows; chưa đồng bộ lịch cá nhân hoặc thống kê toàn bộ thời gian sử dụng PC |
| v2.7 | Thông báo qua API, ẩn nội dung mặc định, TTL/ưu tiên/chế độ yên tĩnh, dòng sự kiện trên TFT | Chưa tự đọc thông báo của các ứng dụng Windows khác |
| v2.8 | Sáu quy tắc có thời gian giữ/chờ/chống lặp, sao lưu nguyên bộ, editor JSON đồng bộ | Editor nâng cao dùng JSON; chưa có trình dựng quy tắc trực quan |
| v2.9 | Ba kênh dữ liệu ngoài có TTL, widget Data 1–3, SDK Python/CLI | Chưa tự kết nối lịch, thời tiết, Home Assistant hay tài khoản bên ngoài |
| v2.10 | Music TFT rộng 304×160, ảnh bìa 120px, font tiếng Việt và cắt UTF-8; Music Web/Windows lớn hơn | Nguồn ảnh vẫn 64×64; cần nghiệm thu TFT và bản Windows cuối trên máy thật |
| v2.11 | Tên hiển thị mimi.desk, Web/Python cùng hướng ấm áp/gọn, điều hướng theo tác vụ, Chẩn đoán thu gọn và chiều cao theo trang | OTA server mới ở mức đề xuất; chưa tự tải firmware hoặc cập nhật EXE |
| Windows v2.12 | Một phiên GUI, nhạc cục bộ độc lập với ESP32, tìm phiên media, backoff và tìm lại đúng device ID | Đã đọc Spotify/ảnh bìa thật; còn nghiệm thu đổi IP bo và EXE trên Windows sạch. Firmware vẫn v2.11-dev |
| Firmware v2.13 | Reader OTA server, chính sách revision, manifest/tương thích, lease OTA/CatPack, NVS cầu nối và publisher | Hai build đã qua; chưa deploy, thử mạng/TLS hoặc rollback trên bo |
| v2.14 | Bảng update Web/Qt, journal OTA, xác minh device/boot, AP pairing và revision Studio/rules | Kiểm tra phần mềm và render DEMO đã qua; chưa nạp bo |
| v2.15 RC | EXE/ZIP Windows, icon/version, cài/nâng cấp chủ động, chẩn đoán, staging và receipt | EXE đọc Spotify thật, 21 tình huống layout và hai firmware build đã qua; Windows sạch/phần cứng còn chờ |

## Bằng chứng kiểm thử

- Bộ kiểm thử Python có 115 bài, gồm GUI, đồng bộ, SDK HTTP, dữ liệu hết hạn, khôi phục cấu hình lớn, chuẩn hoá tiếng Việt và kiểm tra glyph trong font firmware. Các test GUI đã được cô lập khỏi cấu hình thiết bị thật.
- Bộ kiểm thử C++ kiểm tra lịch vẽ, motion, profile, widget, theme, timeline, thông báo, automation và feed; JavaScript kiểm tra hợp đồng dữ liệu và trạng thái Web.
- GitHub CI kiểm tra nguồn, build firmware và đóng gói EXE Windows. EXE v2.6 từ CI đã được mở bằng `--preview`, tải được ảnh và đổi theme cục bộ trên máy phát triển. Đây chưa phải nghiệm thu trên máy Windows sạch.
- Các ảnh xem trước và tài liệu từng mốc nằm trong thư mục này. Firmware chứa cấu hình riêng được giữ ở máy, không push file nhị phân lên repository công khai.

## Thiết bị và OTA

Lần đọc trạng thái ngày 21/11/2026 từ `192.168.0.107` cho biết firmware **2.4.0-dev**, `boot_health=passed`, đang chạy `app0`, OTA sẵn sàng sang `app1` dung lượng **6 MiB**. Đường cập nhật qua Wi-Fi hiện có sẵn. Trong lượt phát triển này chưa nạp các bản mới lên bo.

## Phần cần chốt trước khi deploy rộng

1. Kiểm tra đồng hồ Midnight, FPS, thời gian vẽ và đồng bộ trên TFT với bản mới; đối chiếu các phép đo cùng điều kiện.
2. Thử scene tuỳ chỉnh, feed hết TTL, quy tắc, thông báo và lưu/khôi phục sau khởi động trên bo.
3. Thực hiện một lượt OTA có kiểm soát, xác nhận phiên bản/slot/health và đường phục hồi thực tế.
4. Thử EXE mới trên Windows sạch, provider nhạc/cảm biến, nhắc nghỉ và startup khi người dùng bật.

Phần v3.0 còn lại gồm điều khiển nhiều thiết bị đồng thời, driver cho màn hình khác, theme/animation pack độc lập và các adapter tích hợp cụ thể. `.meocat` v3 có nén, bộ ảnh mới, cảm ứng, cảm biến và âm thanh cần một lượt thiết kế/nghiệm thu riêng; chưa thay phân vùng Flash hoặc bootloader cho các mục này. Các bước hardening bảo mật tiếp tục để sau theo yêu cầu hiện tại.

## Đợt chuẩn hoá sản phẩm tiếp theo

[Đợt desktop v2.12](DESKTOP_STABILITY_V212.md) đã hoàn thành phần vận hành app/nhạc với 37 kiểm thử tập trung và chạy provider trong Qt thật. Môi trường phát triển đã có WinRT; thiếu dependency này là nguyên nhân bổ sung khiến bản thật trước đó không đọc Spotify. Chưa dựng OTA server hoặc deploy firmware.

[Kế hoạch desktop và OTA](PRODUCTION_PLAN.md) ghi hướng nội dung, điều hướng và vòng đời kết nối/cập nhật. Tên mimi.desk đã được chốt và áp dụng. Định danh kỹ thuật cũ giữ để tương thích; RC chưa được gắn nhãn stable. OTA server đã có reader/API/UI/publisher, nhưng hosting và deploy thực tế vẫn chờ thảo luận. Xem [bàn giao RC](RELEASE_CANDIDATE_V215.md).
