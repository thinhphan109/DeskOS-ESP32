# mimi.desk Web và desktop

Mốc nguồn v2.11.0-dev đổi tên hiển thị thành **mimi.desk** và áp dụng hướng ấm áp, gọn, hiện đại đã được người dùng chọn.

## Đã thay đổi

- Web có sidebar trên desktop và thanh điều hướng cuộn ngang trên màn hẹp. Âm nhạc và Tập trung tách riêng; Studio và Thiết bị giữ các công cụ hiện có.
- Tổng quan gọn hơn, thống nhất màu biểu đồ, bỏ vòng CPU trang trí và nội dung đánh số. Trạng thái mô phỏng vẫn được nêu rõ.
- Chẩn đoán kết nối/hiệu năng và các công cụ CatPack/JSON nâng cao được thu gọn. Các endpoint, file và thao tác lưu vẫn dùng hợp đồng cũ.
- App Python có sidebar cùng tên/biểu tượng, khung Music dễ đọc và chỉ lấy chiều cao trang đang mở. Cấu hình, credentials và namespace cũ vẫn được đọc ở vị trí cũ.
- Badge kết nối trên app dựa vào phản hồi thiết bị gần nhất; sau sáu giây không có phản hồi thì không giữ trạng thái Đã kết nối.
- Các trường firmware vẫn cập nhật bằng luồng chọn file hiện có. Hai chế độ OTA server nằm trong [đề xuất riêng](SERVER_OTA_PROPOSAL.md), chưa được bật trong app hoặc bo.

## Kiểm tra tập trung

Chạy nhóm smoke GUI liên quan tới các tab/preview/đồng bộ; kiểm tra cú pháp JS nhúng và trạng thái WebSocket; build firmware chứa giao diện mới. Không chạy lại toàn bộ Python/C++/JS hoặc chờ CI đầy đủ trong lượt redesign này theo yêu cầu tăng tốc dev.

Web đã quan sát ở desktop và 390px: không tràn ngang, canvas Studio co theo vùng chứa. Đã quan sát Overview và Music của app trong DEMO. Người dùng đang tương tác với cửa sổ preview; không tự khởi động lại cửa sổ đó để thay các chỉnh sửa cuối. Các ảnh xem trước là dữ liệu mô phỏng, không phải đo từ phần cứng.

![Web mimi.desk](mimi-web-v211.png)

## Chưa triển khai

Chưa OTA lên thiết bị, chưa phát hành binary lên server, chưa tự cập nhật EXE Windows và chưa đổi bootloader/phân vùng. `MeoDesk` trong định danh giao thức, đường dẫn dữ liệu và tên package cũ là tương thích kỹ thuật, không phải tên hiển thị bị bỏ sót.
