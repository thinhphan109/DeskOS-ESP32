# mimi.desk

<!-- impeccable:product-schema 1 -->

## Platform

web

## Users

Người dùng Windows sở hữu thiết bị ESP32-S3 để theo dõi PC, nhạc và Focus ngay trên bàn.

## Product Purpose

Kết nối trạng thái máy tính và cảnh cá nhân hoá trên thiết bị. Web và app Python/PySide6 điều khiển cùng một trạng thái thiết bị.

## Operating Context

Thiết bị trong mạng Wi-Fi nội bộ; sử dụng hàng ngày, thao tác ngắn, đọc từ xa trên TFT. Có DEMO để xem trước không kết nối phần cứng.

## Capabilities and Constraints

Có telemetry, nhạc SMTC, Focus, cảnh/widget, theme, rules, CatPack, OTA A/B thủ công và reader OTA server. Giữ định danh giao thức, NVS và cấu hình cũ; không đổi bootloader/phân vùng. OTA server có hai chế độ đã chọn: kiểm tra rồi bấm cập nhật, hoặc tự cập nhật bản tương thích mới nhất, với một lượt đang chạy và một bản chờ mới nhất. Bản 2.15.0-rc.1 đã có artifact và staging cục bộ; hosting thật, nạp bo và nghiệm thu phần cứng còn chờ. Không tự bật Auto khi chưa cấu hình server.

## Brand Commitments

Tên sản phẩm được người dùng chốt là mimi.desk. Người dùng chọn cảm giác ấm áp, gọn và hiện đại, giữ mèo làm nhận diện. Nội dung sử dụng tiếng Việt rõ ràng, phù hợp sản phẩm dùng hàng ngày.

## Evidence on Hand

Mã nguồn, assets mèo, bitmap font tiếng Việt, ảnh mô phỏng TFT và bản Web/Qt có thể chạy ở chế độ preview. Kết quả preview không thay cho nghiệm thu phần cứng.

## Product Principles

- Thao tác thường dùng luôn dễ tìm; chẩn đoán kỹ thuật nằm trong phần mở rộng.
- Trạng thái stale/offline/DEMO phải trung thực.
- Giữ cấu hình và tương thích thiết bị hiện có khi đổi tên hiển thị.
- Kiểm tra tập trung theo phần thay đổi; không chạy lại toàn bộ test hoặc chờ CI mỗi lượt dev.
