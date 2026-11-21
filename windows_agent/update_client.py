"""Server-update API and a secret-free journal for interrupted desktop sessions."""
import json
from pathlib import Path
import time
from urllib.error import HTTPError
import uuid
from companion_core import json_request

MESSAGES = {
    "idle": "Chưa có lượt cập nhật đang chạy.", "checking": "Đang kiểm tra phiên bản mới…",
    "up_to_date": "Thiết bị đang dùng phiên bản mới nhất trong kênh đã chọn.",
    "release_available": "Có phiên bản mới sẵn sàng.", "waiting_for_focus": "Chờ kết thúc phiên tập trung.",
    "waiting_for_device": "Chờ thiết bị sẵn sàng và hoàn tất tự kiểm tra.",
    "reboot_notice": "Sắp cập nhật. Thiết bị sẽ khởi động lại sau khi xác minh firmware.",
    "downloading": "Thiết bị đang tải firmware…", "verified_rebooting": "Đã xác minh. Đang khởi động lại…",
    "update_confirmed": "Đã xác nhận firmware mới hoạt động.",
    "returned_previous_firmware": "Thiết bị đã trở về bản trước. Bản vừa thử đã được tạm ngừng tự cập nhật.",
    "policy_conflict": "Chính sách vừa được đổi ở giao diện khác. Đọc lại trước khi lưu.",
    "release_changed": "Bản mới đã thay đổi. Kiểm tra lại trước khi cập nhật.",
    "server_not_configured": "Nhập địa chỉ manifest rồi lưu để kiểm tra bản mới.",
    "update_busy": "Thiết bị đang xử lý một lượt cập nhật khác. Hãy đợi lượt đó hoàn tất.",
    "invalid_policy": "Kiểm tra địa chỉ server, kênh và chu kỳ kiểm tra.",
    "incompatible_manifest": "Bản phát hành không phù hợp với thiết bị hoặc kênh đã chọn.",
    "incompatible_release": "Phiên bản hoặc dung lượng không phù hợp. Kiểm tra bản firmware cầu nối.",
    "release_mutated": "Nội dung của release đã bị thay đổi. Cần phát hành với mã release mới.",
    "stale_manifest": "Server đang trả manifest cũ. Thiết bị sẽ không hạ phiên bản.",
    "http_connect_failed": "Chưa kết nối được server. Kiểm tra Wi-Fi, địa chỉ và giờ hệ thống.",
    "http_status_failed": "Server chưa cung cấp file cần tải. Kiểm tra đường dẫn phát hành.",
    "content_size_mismatch": "Dung lượng tải về khác manifest. Thiết bị đã giữ firmware hiện tại.",
    "image_verification_failed": "Firmware không qua xác minh. Thiết bị đã giữ bản hiện tại.",
    "download_interrupted": "Tải firmware bị gián đoạn. Thiết bị sẽ thử lại theo lịch.",
    "download_incomplete": "Chưa tải đủ firmware. Thiết bị đã giữ bản hiện tại.",
    "unauthorized": "Kiểm tra lại mã ghép nối trong Thiết bị.",
}


class UpdateError(OSError):
    def __init__(self, code, status=0):
        self.code, self.status = code, status
        super().__init__(MESSAGES.get(code, "Chưa thực hiện được cập nhật. Kiểm tra kết nối rồi thử lại."))


def request(host, key, operation="load", payload=None):
    endpoints = {"load": "/api/v1/update-policy", "configure": "/api/v1/update-policy",
                 "check": "/api/v1/update/check", "start": "/api/v1/update/start"}
    if operation not in endpoints:
        raise ValueError("Unknown update operation")
    body = None if operation == "load" else dict(payload or {})
    if operation in ("check", "start"):
        body.setdefault("command_id", uuid.uuid4().hex)
    try:
        result = json_request(host, endpoints[operation], key, body, timeout=3.0)
    except HTTPError as error:
        if error.code == 404:
            return {"schema": 1, "supported": False}
        code = "request_failed"
        try:
            data = json.loads(error.read(4096))
            code = data.get("error") or code
        except (ValueError, OSError):
            pass
        raise UpdateError("unauthorized" if error.code==401 else code, error.code) from error
    if result.get("schema") != 1 or not isinstance(result.get("policy"), dict) or not isinstance(result.get("update"), dict):
        raise UpdateError("invalid_response")
    return result


class UpdateJournal:
    def __init__(self, path):
        self.path = Path(path)

    def read(self):
        try:
            if self.path.stat().st_size > 4096:
                return None
            value = json.loads(self.path.read_text(encoding="utf8"))
            if not isinstance(value, dict) or value.get("schema") != 1:
                return None
            if (not isinstance(value.get("host"),str) or value.get("target_slot") not in ("app0","app1")
                    or value.get("phase") not in ("uploading","uploaded","unconfirmed","confirmed")):
                return None
            return value
        except (OSError, ValueError):
            return None

    def begin(self, host, before, info):
        value = {"schema": 1, "job_id": uuid.uuid4().hex, "host": host, "phase": "uploading",
                 "device_id": before.get("device_id", ""), "previous_boot_id": before.get("state_boot_id", ""),
                 "target_slot": before.get("inactive_slot", ""), "version": info.get("firmware_version", ""),
                 "sha256": info["sha256"], "started_at": int(time.time())}
        self.write(value)
        return value

    def write(self, value):
        # Whitelist fields: never persist a pairing key, Wi-Fi password or request headers.
        fields = ("schema", "job_id", "host", "phase", "device_id", "previous_boot_id", "target_slot", "version", "sha256", "started_at")
        clean = {key: value[key] for key in fields if key in value}
        self.path.parent.mkdir(parents=True, exist_ok=True)
        temporary = self.path.with_suffix(".tmp")
        temporary.write_text(json.dumps(clean, ensure_ascii=False, indent=2), encoding="utf8")
        temporary.replace(self.path)
