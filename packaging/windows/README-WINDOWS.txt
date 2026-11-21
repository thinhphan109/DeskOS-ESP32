mimi.desk Windows Companion - RC

Giai nen toan bo ZIP. Mo mimi.desk.exe de chay portable; khong can Python.
De cai cho tai khoan hien tai: mo PowerShell trong thu muc nay, chay .\install.ps1
Neu Windows chan script chua ky, co the dung cho rieng lan nay:
  powershell -NoProfile -ExecutionPolicy Bypass -File .\install.ps1
Lenh nay khong thay doi execution policy luu trong may.
Tu khoi dong chi bat neu dung -EnableStartup, hoac truoc do da bat.
Thoat ung dung tu khay he thong truoc khi nang cap. Khong can nhap lai thiet bi.

Che do --preview dung du lieu mau, khong doc Spotify/ket noi ESP32.
Ban that doc nhac Windows ngay ca khi chua ket noi ESP32.
Lua chon firmware/update server nam trong Thiet bi -> Cap nhat.

Chan doan dependency ma khong doc cau hinh thiet bi:
  .\mimi.desk.exe --self-check .\diagnostics.json
Bao cao chi ghi trang thai provider, khong ghi ten bai, Wi-Fi hoac ma ghep noi.

Goi uninstall.ps1 chi go Start menu/startup. Sau khi thoat app, co the tu xoa
thu muc chuong trinh. Cau hinh LocalAppData\MeoDesk duoc giu lai.

RC chua nghiem thu tren Windows sach va ESP32. Khong tu nap firmware hoac
kich hoat OTA server chi bang viec mo ung dung nay.
