# Kế hoạch hoàn thiện ứng dụng desktop và OTA

Cập nhật ngày 21/11/2026. Mục tiêu của đợt kế tiếp là hoàn thiện trải nghiệm sử dụng, kết nối và cập nhật. Các mục đã làm được ghi rõ theo phiên bản. Chưa thay bootloader hoặc triển khai firmware lên thiết bị.

## Tên sản phẩm đã chốt

Người dùng đã chốt **mimi.desk** cho Web và app Python. Bản v2.11-dev áp dụng tên hiển thị mới, giao diện ấm áp/gọn/hiện đại, điều hướng theo tác vụ và khu vực Chẩn đoán thu gọn.

Giữ đường dẫn cấu hình, namespace NVS, hostname, khoá Registry và định danh giao thức cũ để tương thích thiết bị đã cài. Bản này chưa có reader tự tải firmware từ server; [đề xuất OTA server](SERVER_OTA_PROPOSAL.md) ghi hai chế độ đã chọn và cách xử lý nhiều release.

## Ứng dụng desktop

Giữ PySide6 và phần giao tiếp hiện có để tập trung vào chất lượng vận hành. Tách lớp `MainWindow` thành các màn hình, model trạng thái và service kết nối/cập nhật; tránh đổi framework trong cùng lượt chuẩn hoá sản phẩm.

| Ưu tiên | Công việc | Điều kiện nghiệm thu |
|---|---|---|
| P0 | Kiểm kê thao tác và thông báo; thống nhất tên sản phẩm, tiếng Việt, trạng thái | Không còn tiêu đề đánh số, khẩu hiệu thử nghiệm hoặc thông tin giao thức trên màn hình chính; DEMO luôn được ghi rõ khi dùng dữ liệu mô phỏng |
| P0 | Điều hướng: Tổng quan, Âm nhạc, Tập trung, Studio, Thiết bị; Cài đặt riêng | Các thao tác thường dùng tìm được trong một màn hình; Firmware nằm trong Thiết bị → Cập nhật |
| P0 | Kết nối theo vòng đời, kiểm tra thiết bị, reconnect có backoff, phát hiện lại IP | Không hiển thị Đã kết nối chỉ vì socket mở; dữ liệu hết hạn chuyển trạng thái rõ ràng; không phát lại lệnh không an toàn sau reconnect |
| P0 | Một phiên ứng dụng; tray, mở lại cửa sổ, thoát hoàn toàn, dừng worker sạch | Không có hai agent cùng gửi dữ liệu; đóng cửa sổ không gây hiểu nhầm ứng dụng đã thoát; không ghi cấu hình hoặc gửi request sau khi thoát |
| P1 | Tách DeviceService, AgentService, UpdateService và model trạng thái khỏi view | Test service không cần GUI/thiết bị thật; GUI test không đọc cấu hình cá nhân; kết quả worker có thể huỷ khi đóng view |
| P1 | Đóng gói bản phát hành và nâng cấp Windows | Cài/chạy/gỡ được trên Windows sạch; có icon, thông tin phiên bản, tài nguyên và log; nâng cấp giữ cấu hình cũ; kiểm tra mức scale 100%, 125%, 150% |
| P1 | Hoàn thiện Studio, Music, Focus, luồng không có dữ liệu và lỗi | Không tràn khung với tên bài tiếng Việt dài; giữ bản nháp khi thiết bị đổi trạng thái; chức năng phụ thuộc provider có hướng dẫn cụ thể |

Thông tin FPS, heap, PSRAM, HTTP/WebSocket, slot Flash, SHA-256 và log đặt trong **Hỗ trợ → Chẩn đoán**. Phiên bản, kênh beta/stable và commit nằm trong **Giới thiệu**. Không xoá thông tin cần debug; đưa chúng vào đúng nơi.

## Quy ước thông báo

Mỗi trạng thái dùng một mã ổn định và nội dung riêng cho người dùng. Thông báo lỗi nêu việc đã xảy ra và hành động có thể thực hiện. Log kỹ thuật giữ mã lỗi, thời điểm và ngữ cảnh nhưng không ghi pairing key hoặc mật khẩu Wi-Fi.

| Trạng thái | Nội dung trên giao diện | Hành động |
|---|---|---|
| Đang tìm | Đang tìm thiết bị… | Chọn thiết bị / Nhập địa chỉ |
| Đã kết nối | Đã kết nối với mimi.desk | Xem thiết bị |
| Dữ liệu quá hạn | Đang kết nối lại… Dữ liệu hiển thị là lần nhận gần nhất. | Kiểm tra kết nối |
| Không liên lạc được | Chưa liên lạc được với thiết bị. Kiểm tra nguồn và Wi-Fi, rồi thử lại. | Thử lại |
| Chưa có nhạc | Chưa phát nhạc. Mở Spotify hoặc trình phát trên PC. | Không cần thông báo lỗi |
| Cảm biến không cung cấp | Chưa có dữ liệu từ cảm biến này. | Xem hướng dẫn |
| Đã lưu cấu hình | Đã lưu thay đổi. | Không lặp lại theo mỗi lần đồng bộ |
| Firmware gửi xong | Đã gửi firmware. Đang chờ thiết bị khởi động lại… | Theo dõi tự động |
| Khởi động đã xác nhận | Đã cập nhật lên phiên bản {version}. | Hoàn tất |
| Hết thời gian xác nhận | Chưa xác nhận được cập nhật. Thiết bị có thể vẫn đang khởi động. | Tìm lại thiết bị / Kiểm tra lại |

Chỉ báo thành công sau khi có phản hồi tương ứng từ thiết bị. Với thao tác thay đổi cấu hình, chờ ACK và đọc lại trạng thái; với OTA, chờ phiên bản/slot/health. Chế độ xem trước vẫn hiển thị nhãn dữ liệu mô phỏng, kể cả trong bản phát hành.

## Luồng OTA hiện tại trong mã nguồn

OTA hiện cập nhật **ứng dụng trên ESP32**. Bản EXE Windows được build riêng từ CI; chưa có cơ chế tự cập nhật ứng dụng Windows. Firmware và CatPack cũng là hai lượt cập nhật riêng.

1. Windows/Web đọc `GET /api/v1/ota/status`: kiểm tra board, Flash, trạng thái và slot chưa sử dụng. Thiết bị dùng hai slot app0/app1, mỗi slot 6 MiB.
2. Người dùng chọn `.bin`. Windows kiểm tra header ESP32-S3, kích thước và SHA-256. Nếu có manifest cùng tên, kiểm tra thêm board, tên file, kích thước, hash và phiên bản. Hiện vẫn chấp nhận `.bin` không có manifest.
3. Người dùng bắt đầu cập nhật. Client gửi multipart qua HTTP nội bộ tới `POST /api/v1/ota/upload`, kèm pairing key, board, số byte và SHA-256 trong header. Windows gửi từng khối 16 KiB; ESP32 ghi trực tiếp vào slot không chạy, không giữ toàn bộ firmware trong RAM.
4. ESP32 kiểm tra header đúng chip trước khi bắt đầu xoá/ghi slot đích. Khi nhận đủ, kiểm tra số byte, SHA-256 và image qua ESP-IDF; chỉ chọn slot khởi động mới sau khi các bước này thành công.
5. Thiết bị lên lịch khởi động lại sau khoảng 1,8 giây. TFT tạm dừng cảnh thường và hiện trạng thái cập nhật.
6. Sau khoảng 25 giây uptime, firmware kiểm tra Flash tối thiểu 16 MiB, PSRAM, buffer DMA và bộ nhớ trong còn trên 24 KiB. Kiểm tra hiện tại chưa chứng minh các tính năng Music, Wi-Fi hoặc từng màn hình đều hoạt động đúng.
7. Windows/Web chờ tối đa khoảng 85 giây để xác nhận slot đích và `boot_health=passed`. Windows đối chiếu phiên bản nếu manifest cung cấp phiên bản. Mất kết nối trong lúc reboot là trạng thái bình thường. Gửi đủ byte chưa được coi là hoàn tất cập nhật.

Rollback phụ thuộc bootloader đã cài, theo [cơ chế App Rollback của ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/system/ota.html#app-rollback). Trường `bootloader_rollback_configured` hiện lấy từ cấu hình biên dịch ứng dụng; không phải bằng chứng đọc trực tiếp từ bootloader hoặc một lần thử phục hồi trên phần cứng. Mã nguồn có yêu cầu đánh dấu image hợp lệ/rollback khi image ở trạng thái pending verify, nhưng chưa nghiệm thu khả năng tự phục hồi vật lý.

Thiết bị đã có phân vùng A/B có thể cập nhật ứng dụng thông thường qua Wi-Fi, không cần UART. Cài lần đầu/chuyển từ phân vùng một ứng dụng, đổi bootloader/phân vùng hoặc phục hồi khi không còn firmware truy cập được có thể cần USB/UART. Không thay các phần này trong đợt Music.

Mã tham chiếu: [client OTA](../windows_agent/ota_client.py), [engine OTA](../firmware/src/ota_manager.cpp), [HTTP bridge](../firmware/src/pc_bridge.cpp), [startup check](../firmware/src/main.cpp), [hướng dẫn migration](OTA_MIGRATION.md).

## Cải tiến OTA theo thứ tự

| Đợt | Cải tiến | Kết quả mong đợi |
|---|---|---|
| 1 | Một màn hình cập nhật với các bước Kiểm tra → Gửi firmware → Xác minh → Khởi động lại → Hoàn tất | Tách tiến độ gửi file khỏi trạng thái khởi động; dùng trạng thái chờ cho bước không đo được phần trăm |
| 1 | Lưu mã lượt cập nhật, device_id, boot_id trước update, phiên bản và slot đích trên Windows | Mở lại app có thể tiếp tục kiểm tra; không tự gửi lại firmware chỉ vì cửa sổ đóng hoặc timeout |
| 1 | Xác nhận đúng thiết bị, boot_id mới, phiên bản và slot; phát hiện lại IP qua discovery | Tránh xác nhận nhầm thiết bị sau DHCP đổi IP hoặc chỉ vì endpoint sống trở lại |
| 1 | Phân loại lỗi và nút phục hồi phù hợp | Phân biệt Chưa gửi / Gửi bị gián đoạn / Chưa xác nhận / Đã quay lại bản cũ; chỉ hiện hướng dẫn USB khi cần |
| 2 | Manifest phát hành có model, revision phần cứng, layout phân vùng, phiên bản tối thiểu của app/firmware, kênh và changelog | Chọn đúng bản trước khi upload; giữ luồng chọn file thủ công cho debug |
| 2 | Kiểm tra bản mới từ release feed, tải về rồi cập nhật bằng một thao tác chủ động | Không phải tìm file trong CI; không tự flash trong nền; build phát hành không chứa Wi-Fi hoặc key của máy phát triển |
| 2 | Tách lịch sử cập nhật dự kiến khỏi kết quả thực tế | Có thể đối chiếu lỗi, timeout và lần xác nhận sau; không dùng journal làm bằng chứng rollback |
| 3 | Nghiệm thu lỗi ghi, mất mạng, mất nguồn và rollback trên bo thử nghiệm | Xác định đường phục hồi trước khi gọi bản phát hành ổn định; giữ đường USB cứu hộ |

Ký gói cập nhật, hardening transport và bootloader là đợt riêng trước khi phân phối rộng; hiện vẫn giữ luồng LAN dễ debug theo yêu cầu. Không thay eFuse hoặc bật Secure Boot trong kế hoạch cải thiện giao diện.

## Thứ tự thực hiện và mốc phát hành

1. Chốt v2.10-dev Music rộng và font tiếng Việt bằng kiểm thử nguồn/build, sau đó kiểm tra trên TFT khi đến lượt deploy.
2. Chốt tên hiển thị và bộ nội dung; thiết kế lại điều hướng/các trạng thái desktop với cùng service hiện tại.
3. Hoàn thiện vòng đời kết nối, một phiên ứng dụng, tray và service độc lập; chạy bộ kiểm thử hồi quy.
4. Làm OTA đợt 1; sau đó mới thêm release feed và chọn bản tự động. Việc nâng cấp EXE Windows có luồng và cơ chế cài đặt riêng.
5. Nghiệm thu Windows sạch và phần cứng, chạy đủ một lượt cập nhật thành công và các tình huống thất bại đã xác định. Chỉ gắn nhãn stable khi đạt các điều kiện này; tên phiên bản dev hiện tại không tự trở thành bản production.

## Tiêu chí trước khi phát hành ổn định

- Giao diện tiếng Việt nhất quán, dấu đầy đủ, không tràn ở các mức scale đã hỗ trợ; đường đi từ mở app đến kết nối thiết bị rõ ràng.
- Không có vòng reconnect hoặc thông báo lặp vô hạn; stale, offline và DEMO hiển thị đúng bản chất dữ liệu.
- Giữ được cấu hình sau nâng cấp/đổi tên; thao tác thoát dừng worker và không làm mất bản nháp ngoài ý muốn.
- Windows mới cài có thể chạy bản đóng gói và provider thiết yếu; thiếu provider có hướng dẫn thay vì crash.
- OTA chỉ báo thành công sau khi xác nhận, có thể tìm lại thiết bị và có đường phục hồi đã thử trên bo.
- Log hỗ trợ đủ thông tin để debug, không lộ bí mật; artifact public không chứa cấu hình riêng.
