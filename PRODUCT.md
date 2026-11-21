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

Có telemetry, nhạc SMTC, Focus, cảnh/widget, theme, rules, CatPack và OTA A/B thủ công. Giữ định danh giao thức, NVS và cấu hình cũ. Chưa triển khai pull OTA/server hoặc đổi bootloader. OTA server cần hai chế độ do người dùng chọn: kiểm tra rồi bấm cập nhật, hoặc tự cập nhật bản tương thích mới nhất; cần xử lý nhiều release nối tiếp.

## Brand Commitments

Tên sản phẩm được người dùng chốt là mimi.desk. Người dùng chọn cảm giác ấm áp, gọn và hiện đại, giữ mèo làm nhận diện. Nội dung sử dụng tiếng Việt rõ ràng, phù hợp sản phẩm dùng hàng ngày.

## Evidence on Hand

Mã nguồn, assets mèo, bitmap font tiếng Việt, ảnh mô phỏng TFT và bản Web/Qt có thể chạy ở chế độ preview. Kết quả preview không thay cho nghiệm thu phần cứng.

## Product Principles

- Thao tác thường dùng luôn dễ tìm; chẩn đoán kỹ thuật nằm trong phần mở rộng.
- Trạng thái stale/offline/DEMO phải trung thực.
- Giữ cấu hình và tương thích thiết bị hiện có khi đổi tên hiển thị.
- Kiểm tra tập trung theo phần thay đổi; không chạy lại toàn bộ test hoặc chờ CI mỗi lượt dev.
