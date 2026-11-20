# MèoDesk v1.3–v1.7 — OTA A/B migration and recovery (ESP32-S3 WROOM-1 N16R8)

**READ BEFORE FLASHING.** This changes the partition table of the existing
single-app `huge_app.csv` firmware. **The first deployment requires USB.**
Do not attempt to OTA directly from v1.2: there is no inactive application
partition. No one has performed the final hardware migration in this archive.
**If v1.3–v1.6 was already installed with this partition layout, v1.7
does NOT require another USB partition migration: OTA can update the inactive
firmware slot.**

## What is implemented

- Custom 16 MiB Flash layout with independent **6 MiB `ota_0` and `ota_1`**
  partitions, plus 3.875 MiB asset partition and 64 KiB crash log.
- Preserve `nvs` at `0x9000/0x5000`, `otadata` at `0xE000/0x2000`, and
  bootable `ota_0` at `0x10000` to simplify migration from `huge_app`.
- Authenticated local multipart upload on `/api/v1/ota/upload`. Image bytes are
  **streamed** to the inactive slot, not buffered in PSRAM. An exact byte
  length and 64-character SHA-256 are **mandatory**, and are checked before
  choosing the next boot partition. The ESP-IDF image check runs on finish.
- Web Studio Firmware Studio page and Windows Companion Firmware tab.
- A static OTA screen pauses normal TFT animation during flash writes.
- Device reports whether the **built bootloader** was compiled for rollback.
  Application-controlled confirmation after 25 s of healthy operation only
  applies if that bootloader feature is actually present.

## Partition map (proposal compiled into v1.3 source)

| Partition | Offset | Bytes | Note |
|---|---:|---:|---|
| `nvs` | `0x9000` | `0x5000` | Existing private Wi-Fi/settings NVS |
| `otadata` | `0xE000` | `0x2000` | Existing OTA selection metadata |
| `app0` (`ota_0`) | `0x10000` | `0x600000` | 6 MiB, USB/bootstrap app |
| `app1` (`ota_1`) | `0x610000` | `0x600000` | 6 MiB inactive update slot |
| `assets` (`spiffs`) | `0xC10000` | `0x3E0000` | **Not mounted**; v1.6 keeps two raw Cat/Motion Pack banks |
| `coredump` | `0xFF0000` | `0x10000` | Reserved; crash logging not enabled |

The new image must be **under 6 MiB**, include the expected ESP32-S3 target,
and use WROOM-1 **qio_opi**, not WROOM-2 N16R8V `opi_opi`.
This partition map is source-reviewed and checked by unit tests, but has not
been flashed on a physical N16R8 board.

## One-time USB transition (do not skip backup)

1. Confirm the physical board says **ESP32-S3 WROOM-1 N16R8** and that the
   serial boot log reports about **16 MB Flash + 8 MB PSRAM**.
2. Keep a private copy of the **known-good v1.2 source and compiled binary**.
   Archive the existing Wi-Fi/AP/pairing settings separately. The `baseline/`
   binaries are older historical releases, not a verified v1.2 rollback.
3. Find the COM port in Device Manager. Close Serial Monitor. The following
   **read-only** example uses COM17; replace it with your actual port:

   ```powershell
   py -m pip install esptool
   py -m esptool --chip esp32s3 --port COM17 flash_id
   py -m esptool --chip esp32s3 --port COM17 read_flash 0x9000 0x5000 meodesk-nvs-private-backup.bin
   ```

   Keep the backup private: NVS may contain Wi-Fi credentials and pairing keys.
   On newer `esptool` CLI versions, use `flash-id` and `read-flash` aliases if
   the older names are not recognized. Do **not** run `erase_flash`.
4. Extract the v1.7 ZIP. Open **`meodesk_v17/firmware/`**, not its parent,
   in VS Code. Confirm `platformio.ini` refers to `meodesk_ota_16mb.csv`.
5. Run **PlatformIO Build** first. If it fails, **do not upload**. Only after
   a successful build, run **Upload via the proven USB/COM port**. Uploading
   this new table can invalidate old filesystem data: the old SPIFFS region
   overlaps the enlarged `app0` range. The current MèoDesk source does not
   mount SPIFFS, but still back up any external user data if you added code.
6. Monitor Serial at 115200. Verify Flash 16 MB, PSRAM around 8 MB, DMA on,
   Web Studio, old PC/Focus/music pages and Wi-Fi pairing. Open:

   `http://<YOUR-ESP32-IP>/api/v1/ota/status`

   Verify `ready:true`, `inactive_slot_bytes:6291456`, and different
   `running_slot`/`inactive_slot` values.
7. **Only now** is the device ready for the first LAN OTA test using a freshly
   compiled `.pio/build/esp32-s3-devkitc-1/firmware.bin`.

## Local update from Web Studio

1. On a PC on the same trusted private LAN, visit `http://<ESP32-IP>/`.
2. Enter the **firmware pairing key** (NOT the Wi-Fi password).
3. Open the **Firmware Studio** area near the bottom.
4. Confirm the inactive slot is available. Select the PlatformIO `firmware.bin`.
5. Click **UPLOAD & VERIFY** and explicitly approve the operation.
6. Keep the browser page open for up to 85 seconds. v1.4 automatically checks
   that the device boots into the selected slot and reports `boot_health:passed`
   after its startup test (~25 s). If the check times out, use USB Serial to
   diagnose; **100% upload is not a confirmed successful update**.

Web Studio computes the SHA-256 in the browser, including a JavaScript fallback
for plain HTTP LAN origins where `crypto.subtle` is unavailable.

## Local update from Windows Companion

1. Start the existing Windows Companion (`start_here.pyw`), select **Thiết bị**
   and enter/scan the IP plus pairing key.
2. Open **Firmware** tab → **KIỂM TRA OTA**.
3. Select `firmware.bin` → **XÁC NHẬN VÀ CẬP NHẬT QUA WI-FI**.
4. The app streams the file in 16 KiB chunks and shows upload progress.
5. It then waits for the target slot to boot and pass the startup check. It
   reports rollback/timeout as *not confirmed*. Review the active version and
   Serial log after successful installation as an additional check.

## OTA protocol v1

`GET /api/v1/ota/status` is read-only. It returns `board`, `version`,
`ready`, `phase`, `running_slot`, `inactive_slot`,
`inactive_slot_bytes`, `bootloader_rollback_configured`, `written`, `expected`.
v1.4 additionally returns `uptime_ms` and `boot_health`; the latter is an
**application** self-test result, not proof of automatic bootloader rollback.

`POST /api/v1/ota/upload` requires multipart form-data with field `firmware`
and these HTTP headers:

| Header | Meaning |
|---|---|
| `X-MEO-KEY` | Existing private pairing key |
| `X-MEO-OTA-BOARD` | Exact `meodesk-esp32s3-n16r8` |
| `X-MEO-OTA-SIZE` | Exact length in bytes of the raw `.bin` |
| `X-MEO-OTA-SHA256` | 64 hexadecimal characters, hash of raw `.bin` |

The server rejects wrong slot, size, hash, board header or ESP-IDF image.
When successful, it returns `{ "ok": true, "verified": true, "rebooting": true }`
and schedules reboot after the HTTP response is sent.

## Critical limitations: developer OTA, not secure production OTA

1. **Unsigned firmware.** SHA-256 prevents accidental corruption but does not
   prove the publisher's identity. Anyone who learns the HTTP pairing key
   could send a malicious image. This build has NO digital signature check.
2. **HTTP transport.** Pairing key travels over plain HTTP on the LAN. Keep
   the device on a trusted private network; do **not** forward port 80 or put
   it directly on the Internet. Future secure release needs signed firmware
   and preferably TLS/certificate validation.
3. **Rollback depends on the bootloader.** The precompiled Arduino bootloader
   may have rollback disabled. The firmware reports its compile-time setting;
   `bootloader_rollback_configured:false` means an automatic rollback cannot
   be promised. Changing this requires a separate bootloader build/validation,
   **not** merely calling the OTA API.
4. A wrong or faulty new application may require USB recovery if the
   bootloader cannot roll back. Preserve the old `.bin` and USB access.
5. NVS and asset **partition-table updates themselves are not transactional**.
   Never attempt a second partition migration using this firmware OTA API.
6. Avoid simultaneous Web Studio OTA and Windows Companion OTA submissions.

## Recovery

- **Upload fails before confirmation:** the active app should remain selected.
  Retry when the network is stable; a partial inactive image is not booted.
- **New firmware boots but fails tests:** when a rollback-enabled bootloader is
  in use, confirmation is withheld and rollback requested.
- **Device is offline after update:** try `http://192.168.4.1/` from the
  fallback AP, then inspect USB Serial 115200. If the bootloader cannot
  recover, flash the saved known-good image over USB.
- **Lost Wi-Fi configuration after USB migration:** recover from your private
  NVS backup only after confirming partition geometry and encryption state.
  Do not write the backup blindly at a different offset.

## Future hardening milestones

- Signed firmware bundles verified against a pinned device public key.
- Independently built and tested rollback-enabled bootloader (ESP-IDF config).
- HTTPS/device-pull updates with signed manifests and explicit user approval.
- Signed firmware and Cat Pack bundles (v1.5 separates image packs from firmware,
  but neither currently has a publisher signature).
- Power-loss, wrong-image, wrong-key and 30-minute post-update endurance tests
  on real N16R8 hardware.
