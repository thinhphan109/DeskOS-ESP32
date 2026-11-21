# mimi.desk · các vấn đề cần thảo luận và đợt update tiếp theo

Cập nhật ngày 21/11/2026, sau mốc **2.15.0-rc.1** (`01b4261`). Đây là danh sách để chốt hướng phát triển; chưa cho phép deploy hoặc xác định mọi đề xuất đều nằm trong đợt triển khai tiếp. Trạng thái source/artifact hiện tại nằm trong [bàn giao RC](RELEASE_CANDIDATE_V215.md).

## 1. Những quyết định đã chốt

- Tên sản phẩm **mimi.desk**; giao diện ấm áp, gọn và hiện đại, giữ nhận diện mèo.
- Web và app dùng trạng thái thiết bị chung; cấu hình cũ được giữ khi nâng cấp.
- Firmware OTA có hai chế độ chỉnh từ Web/app: **kiểm tra rồi bấm cập nhật** và **tự cập nhật khi có bản mới**.
- Một lượt ghi Flash đang chạy, một bản đích mới nhất đang chờ; không xếp hàng mọi release hoặc ngắt lượt đang ghi để chen bản khác.
- Push Git sau mỗi phiên bản hoàn thành. Kiểm thử tập trung vào phần thay đổi và các đường ảnh hưởng đến cập nhật/phục hồi; không mặc định chạy lại toàn bộ bộ kiểm thử.
- Chưa nạp firmware mới trong đợt vừa qua, chưa chọn hosting thật hoặc bật Auto trên bo. Hardening bảo mật nâng cao để đợt sau theo yêu cầu hiện tại.

## 2. Các vấn đề cần thảo luận trước deploy

| ID | Cần chốt | Tình hình hiện tại | Hướng đề xuất để thảo luận |
|---|---|---|---|
| D01 | Nơi đặt firmware và manifest | Có publisher, staging cục bộ, mẫu cấu hình HTTPS; URL mẫu dùng miền `.invalid` | Chọn hosting đang có hoặc GitHub Releases kết hợp manifest HTTPS tĩnh. Giữ cùng hợp đồng manifest để đổi nơi host sau này |
| D02 | Lượt nạp cầu nối đầu tiên | Bo được đọc lần cuối ở 2.4-dev; chưa xác minh lại hiện trạng. Có build cầu nối riêng và build public RC | Đọc lại bo, sao lưu cấu hình và chọn một lượt OTA Wi-Fi có kiểm soát. Không đổi bootloader/phân vùng trong lượt đầu |
| D03 | Cách nghiệm thu pull OTA | Cầu nối mới nhất và public đều là 2.15.0-rc.1; reader không tự ghi lại cùng version | Chọn cầu nối mới nhất rồi đợi release cao hơn, hoặc dùng cầu nối v2.13 đã giữ để nghiệm thu tải public RC mới hơn. Không sửa version trong manifest để giả firmware mới |
| D04 | Thời điểm bật Auto | Mặc định là thông báo; manifest URL trống trên firmware chưa cấu hình | Giữ thông báo cho lượt nghiệm thu đầu. Chỉ bật Auto sau khi đã xác nhận đường cập nhật và phục hồi trên bo |
| D05 | Điều kiện cho phép cập nhật | Đã có hoãn Focus, đếm ngược 10 giây trước tải, tối đa ba lần tự thử một release và khoảng chờ khi lỗi | Chốt có cần khung giờ yên tĩnh, chỉ cập nhật khi rảnh và nút Hoãn lần này. Các tuỳ chọn bổ sung này chưa có |
| D06 | Kênh và quyền phát hành | Có preview/stable và sequence; push Git chưa tự phát hành OTA. Workflow chỉ tạo artifact để review | Chốt ai được đưa bản vào kênh, khi nào nâng preview lên stable, và cách rút một release lỗi. Ưu tiên thao tác phát hành có chủ đích ở giai đoạn đầu |
| D07 | Mức phục hồi cần đạt trước stable | Có A/B, hash/image verification và health check; rollback vật lý chưa nghiệm thu | Chốt kịch bản lỗi và đường USB/UART cứu hộ cần thử. Không coi kết quả mô phỏng hoặc cờ bootloader là bằng chứng phục hồi trên bo |
| D08 | Cách phân phối Windows | Có ZIP portable, script cài/nâng cấp theo người dùng và startup tuỳ chọn. Chưa có tự nâng cấp EXE | Chốt portable là hình thức chính hay cần installer. Nếu thêm updater Windows, ưu tiên kiểm tra/thông báo rồi người dùng bấm nâng cấp |

Các hướng ở cột cuối là đề xuất, chưa phải lựa chọn mới đã được phê duyệt. Điều kiện hiện tại của Auto có thể được điều chỉnh sau thảo luận; không tự thay chính sách trên thiết bị đang dùng.

## 3. Phần chưa nghiệm thu thực tế

| Ưu tiên | Cần kiểm tra | Bằng chứng cần có để chốt |
|---|---|---|
| P0 | OTA cầu nối → firmware public | Đúng device ID, boot ID mới, version, slot đích và health; Wi-Fi, mã ghép nối, cảnh và cấu hình còn nguyên |
| P0 | Mạng đứt, file/hash sai và phục hồi | Bản đang chạy vẫn truy cập/khởi động được; không hai lượt ghi đồng thời; không tự thử vô hạn. Mất nguồn/rollback chỉ thử trong lượt đã thống nhất |
| P0 | TLS chạy cùng renderer | Đo heap trong, PSRAM, thời gian vẽ/FPS và khả năng phản hồi Web/app lúc kiểm tra hoặc tải; chưa có số đo trên bo cho reader mới |
| P0 | Music/font tiếng Việt trên TFT | Tên bài dài, dấu chồng, nghệ sĩ, ảnh bìa, timeline và chuyển bài không tràn/nhấp nháy. Đã có source/font và Spotify thật trên Windows, chưa có nghiệm thu TFT bản mới |
| P0 | Đồng hồ Midnight | Theo dõi nhiều phút để xác nhận cập nhật từng giây trong điều kiện nhạc/mạng khác nhau. Source đã sửa lịch vẽ; chưa chứng minh hết hiện tượng nhảy hai giây trên bo |
| P1 | Đổi IP và đồng bộ hai giao diện | Tìm lại đúng device ID, không nối nhầm bo khác; reconnect không gửi lại lệnh điều khiển cũ. Web/app cùng thao tác và cùng thấy kết quả |
| P1 | Windows sạch | Chạy khi không cài Python, kiểm tra WinRT, tray/startup, nâng cấp/gỡ, giữ cấu hình và dependency. EXE mới chỉ được nghiệm thu trên máy phát triển |
| P1 | Studio/rules qua reboot | Giữ bản nháp khi xung đột; revision/boot ID mới không cho ghi đè bản cũ ngoài ý muốn. Dùng Web firmware và app cùng thế hệ có hỗ trợ revision |

Các mục này là thiếu bằng chứng nghiệm thu, không đồng nghĩa tất cả đang có bug. Các lỗi DEMO gây nhầm Spotify, thiếu WinRT ở môi trường phát triển, cuộn chuột đổi option và khoảng trống do chiều cao Studio đã có bản sửa.

## 4. Các cải tiến muốn làm tiếp

### Ưu tiên gần: hoàn thiện cập nhật và thao tác hàng ngày

| ID | Cải tiến đề xuất | Giá trị sử dụng | Phạm vi cần thống nhất |
|---|---|---|---|
| U01 | Hoãn/cancel cập nhật trước khi ghi | Có thể dời lượt Auto khi đang dùng thiết bị | Chỉ huỷ ở giai đoạn chờ/đếm ngược; không chen thao tác huỷ nguy hiểm vào lúc đang ghi Flash |
| U02 | Ghi chú và lịch sử cập nhật dễ đọc | Biết bản mới thay đổi gì, vì sao lượt trước chưa xác nhận | Hiển thị bản trước/sau, thời điểm, nguồn cập nhật và kết quả thực tế; giữ mã kỹ thuật trong Chẩn đoán |
| U03 | Trung tâm phát hành từ CI | Giảm thao tác build, upload và đổi con trỏ kênh | Build public → staging → kiểm tra tải/hash → phát hành có chủ đích. Có cách ngừng phát hành bản lỗi; không tự hạ firmware trên bo |
| U04 | Chọn nguồn nhạc | Tránh app nhận nhầm trình duyệt khi muốn theo Spotify | Tự chọn như hiện tại, ưu tiên Spotify hoặc chọn một trình phát. Metadata, ảnh bìa và lệnh điều khiển phải cùng nguồn |
| U05 | Làm mượt chuyển bài và trạng thái stale | Đổi bài nhanh, ít xuất hiện ảnh cũ hoặc trạng thái khó hiểu | Đánh giá sự kiện SMTC so với polling hiện tại; giữ phương án dự phòng và không dựa vào mạng ESP32 để hiển thị trên PC |
| U06 | Profile cá nhân | Đổi nhanh giữa làm việc, nghe nhạc và nghỉ ngơi | Tên profile, cảnh/theme, độ sáng hoặc lịch Focus khi firmware có hỗ trợ; xem trước thay đổi và phân biệt lưu trên PC với lưu trên bo |
| U07 | Studio có preset và thao tác hoàn tác | Dễ thử bố cục mà không mất cấu hình đang ổn | Bộ preset nhỏ, Undo/Redo trong bản nháp, xem thay đổi và trạng thái đã lưu. Không tự ghi đè khi thiết bị vừa được sửa từ giao diện khác |
| U08 | Tăng chất lượng chữ/ảnh trên màn nhỏ | Đọc tên bài và thông tin từ xa rõ hơn | Tuỳ chọn cỡ chữ, xử lý tên dài; đánh giá ảnh bìa độ phân giải cao hơn so với RAM/băng thông. Nguồn ảnh hiện vẫn 64×64 dù ô hiển thị lớn hơn |
| U09 | Hướng dẫn thiết lập lần đầu rõ hơn | Mở app mới biết ngay bước tiếp theo | Hoàn thiện các bước tìm thiết bị, Wi-Fi/AP, ghép nối và kiểm tra xong; phân biệt DEMO, app cục bộ và kết nối bo |
| U10 | Cập nhật app Windows có hướng dẫn | Không cần tự tìm và thay cả thư mục ứng dụng | Kiểm tra bản app mới, tải gói, đề nghị thoát agent trước khi thay file, mở lại giữ cấu hình. Đây là updater Windows riêng, không dùng engine OTA ESP32 |

### Để sau khi lõi ổn định

- Thống kê Focus theo tuần, xuất dữ liệu và mục tiêu cá nhân; tránh gọi nhật ký quan sát được là toàn bộ thời gian sử dụng PC.
- Adapter cụ thể cho thời tiết, lịch hoặc Home Assistant; tận dụng feed/TTL đã có, xác định nguồn dữ liệu và hành vi khi offline trước khi làm từng adapter.
- Bộ animation mèo mới và theme/pack độc lập; đo giới hạn Flash/PSRAM trước khi thêm frame hoặc thay định dạng `.meocat`.
- Quản lý nhiều thiết bị: đặt tên, chọn bo và tránh gửi thao tác nhầm. Chưa mở rộng thành nền tảng quản lý đội thiết bị trong đợt gần.
- Driver cho màn hình khác, cảm ứng, cảm biến và âm thanh là các nhánh phần cứng riêng. Beat detection cần nguồn audio thật; trạng thái `playing` không phải dữ liệu beat.
- Ký gói phát hành, transport và hardening bootloader là đợt riêng trước khi phân phối rộng; chưa tự bật trong phạm vi hiện tại.

## 5. Thứ tự update đề xuất

Tên/version bên dưới chỉ là mốc dự kiến, chưa tạo tag hoặc coi là công việc đã triển khai.

| Đợt dự kiến | Trọng tâm | Điều kiện kết thúc |
|---|---|---|
| v2.16 · nghiệm thu và sửa RC | Chốt D01–D07; kiểm tra bo, TFT/nhạc/Midnight, OTA và Windows sạch; sửa lỗi có bằng chứng | Có kết quả thực tế và đường phục hồi rõ ràng; danh sách lỗi còn lại được phân loại |
| v2.17 · Music và Studio | Chốt U04–U09 theo phản hồi sử dụng; làm ít tính năng nhưng hoàn thiện luồng | Đổi nguồn nhạc nhất quán, profile/preset không làm mất bản nháp, chữ/ảnh đáp ứng màn thật |
| v2.18 · phát hành và nâng cấp | U01–U03, U10 và quyết định phân phối Windows | Có quy trình phát hành lặp lại được, theo dõi đúng kết quả và nâng cấp app giữ cấu hình |
| Sau đó | Tích hợp, animation, nhiều thiết bị hoặc nhánh phần cứng | Chọn từng nhu cầu có mục tiêu cụ thể, không gom tất cả vào một bản lớn |

Stable là kết quả nghiệm thu, không tự phát sinh chỉ vì tăng số version. Khi đã chốt hướng, tiếp tục push mỗi mốc hoàn thành và dùng kiểm tra phù hợp với thay đổi.

## 6. Sổ quyết định cho lần thảo luận tiếp

| Nhóm quyết định | Trạng thái | Nơi ghi kết quả |
|---|---|---|
| Hosting, kênh, quyền phát hành | Chờ thảo luận | D01, D06 và hướng dẫn deploy |
| Lượt nạp cầu nối, thử OTA/phục hồi, Auto | Chờ thảo luận | D02–D05, D07 và biên bản nghiệm thu |
| Phân phối/nâng cấp Windows | Chờ thảo luận | D08, U10 |
| Cải tiến Music/Studio/profile cần làm trước | Chờ chọn ưu tiên | U04–U09 |

Trước khi triển khai đợt mới, cập nhật hàng tương ứng thành quyết định cụ thể, phạm vi và điều kiện nghiệm thu. Chưa cần chốt mọi cải tiến dài hạn cùng lúc.
