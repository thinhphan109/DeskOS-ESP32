# Đợt hoàn thiện mimi.desk

Tiếp tục liên tục theo yêu cầu, push Git sau mỗi mốc. Chỉ kiểm tra phần thay đổi và các đường có rủi ro thực tế; không chạy lại toàn bộ bộ kiểm thử cho mỗi chỉnh sửa nhỏ.

| Mốc | Phạm vi | Trạng thái |
|---|---|---|
| Windows v2.12 | Nhạc độc lập, provider Spotify, một phiên GUI, reconnect và nhận diện thiết bị | Đã push `2a451e1` |
| v2.13 | OTA pull, chính sách hai chế độ, tương thích release, điều phối Flash, NVS cầu nối và publisher | Đã push `3bdd7a6`, hai build và kiểm tra liên quan đã qua |
| v2.14 | Web/Windows quản lý OTA, phản hồi lưu/xung đột, thiết lập ban đầu, xác nhận và tiếp tục lượt cập nhật | Đã triển khai và kiểm tra liên quan; đang chốt artifact |
| v2.15 RC | Đóng gói Windows, firmware riêng/chung, staging server, hướng dẫn vận hành và kiểm tra tập trung | Kế tiếp |

Điểm dừng của đợt này là bộ source/artifact sẵn để thảo luận và deploy. Không tự nạp bo, chọn hosting thật, bật Auto hoặc gắn nhãn stable. Nghiệm thu TFT, đổi IP/router, mất nguồn/rollback và Windows sạch được ghi rõ là chờ phần cứng/môi trường, không thay bằng kết quả mô phỏng.
