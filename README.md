# ESP32C3 Multi-Sketch OTA

- `dashboard` → Web dashboard with OTA update capability
- `blink_tiny` → Example blink LED sketch
- OTA binaries are generated automatically in `docs/` by GitHub Actions
- `version.json` contains all sketches and OTA URLs

---

### Usage

1. Open `dashboard/dashboard.ino` in Arduino IDE.
2. Set your Wi-Fi SSID and password.
3. Upload to ESP32-C3.
4. Push new sketches or changes to GitHub → GitHub Actions builds OTA files.
5. Dashboard automatically updates itself (or other sketches) via OTA.
