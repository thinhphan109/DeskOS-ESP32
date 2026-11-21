# Đợt hoàn thiện mimi.desk

Tiếp tục liên tục theo yêu cầu, push Git sau mỗi mốc. Chỉ kiểm tra phần thay đổi và các đường có rủi ro thực tế; không chạy lại toàn bộ bộ kiểm thử cho mỗi chỉnh sửa nhỏ.

| Mốc | Phạm vi | Trạng thái |
|---|---|---|
| Windows v2.12 | Nhạc độc lập, provider Spotify, một phiên GUI, reconnect và nhận diện thiết bị | Đã push `2a451e1` |
| v2.13 | OTA pull, chính sách hai chế độ, tương thích release, điều phối Flash, NVS cầu nối và publisher | Đã push `3bdd7a6`, hai build và kiểm tra liên quan đã qua |
| v2.14 | Web/Windows quản lý OTA, phản hồi lưu/xung đột, thiết lập ban đầu, xác nhận và tiếp tục lượt cập nhật | Đã push `1b15562`; 29 kiểm thử Python, các kiểm tra JS/build/render đã qua |
| v2.15 RC | Đóng gói Windows, firmware riêng/chung, staging server, hướng dẫn vận hành và kiểm tra tập trung | Đã build/kiểm tra, sẵn bàn giao trước deploy; xem [RC v2.15](RELEASE_CANDIDATE_V215.md) |

Điểm dừng của đợt này là bộ source/artifact sẵn để thảo luận và deploy. Không tự nạp bo, chọn hosting thật, bật Auto hoặc gắn nhãn stable. Nghiệm thu TFT, đổi IP/router, mất nguồn/rollback và Windows sạch được ghi rõ là chờ phần cứng/môi trường, không thay bằng kết quả mô phỏng.

Đã đạt điểm dừng phần mềm ở 2.15.0-rc.1. Các quyết định cần trao đổi tiếp nằm trong [bàn giao RC](RELEASE_CANDIDATE_V215.md); không còn bước build/source bắt buộc trong bốn mốc này.

Danh sách để trao đổi tiếp: [các quyết định và cải tiến update](NEXT_DISCUSSION_AND_UPDATES.md). Các mốc v2.16–v2.18 trong tài liệu đó là đề xuất mới, chưa được triển khai hoặc gắn tag.
