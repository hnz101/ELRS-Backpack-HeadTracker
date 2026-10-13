# Project Handoff: FPV HeadTracker via ELRS Backpack (ESP-NOW)

**Target Hardware:** WEMOS LOLIN S3 Mini Pro (ESP32-S3FH4R2, 4MB Flash, 2MB PSRAM)  
**Base Repository:** [druckgott/ELRS-Backpack-Example-ESPNOW](https://github.com/druckgott/ELRS-Backpack-Example-ESPNOW.git)  
**Communication Protocol:** ExpressLRS Backpack ESP-NOW (`MSP_ELRS_BACKPACK_SET_PTR` / CRSF 11-bit)  
**RC Transmitter Mode:** EdgeTX Trainer (`Master / CRSF`, TR1..TR3)

---

## 1. System Architecture & File Overview

```
ELRS-Backpack-Example-ESPNOW/
├── platformio.ini              # Configured for lolin_s3_mini_pro with USB CDC & COM19
├── src/
│   ├── my_config.h            # Central configuration (UID, pinout, channels, angles, inversion)
│   ├── main.cpp               # Setup, ESP-NOW init, 50Hz main loop, display & button dispatch
│   ├── headtracker.h / .cpp   # Sensor fusion (Complementary filter), relative zeroing, CRSF scaling
│   ├── qmi8658.h / .cpp       # Fast I2C driver for onboard 6-axis IMU with gyro bias calibration
│   ├── display_manager.h/.cpp # 0.85" LCD driver (ST7789 / GC9A01 auto-detect), 32KB SRAM framebuffer
│   ├── button_manager.h/.cpp  # Debounced buttons (GPIO 0, 47, 48) and WS2812 RGB LED controller
│   └── fake_vrx_fake_trainer.cpp # Raw 802.11 ESP-NOW packet generator & CRSF frame transmitter
```

---

## 2. Hardware Pinout & Peripherals

| Peripheral | Component | Pins / Config | Notes |
| :--- | :--- | :--- | :--- |
| **Microcontroller** | ESP32-S3FH4R2 | Native USB OTG (COM19) | Native CDC enabled (`ARDUINO_USB_CDC_ON_BOOT=1`) |
| **Display** | 0.85" 128x128 TFT | MOSI=38, MISO=39, SCLK=40, CS=35, DC=36, RST=34, BL=33 | Auto-detects ST7789 / GC9A01. Button 0 toggles driver. |
| **Onboard IMU** | QMI8658C (6-axis) | SDA=12, SCL=11, Addr=`0x6B` (fallback `0x6A`) | **Soldered on bottom side of PCB!** (See Gotcha #1) |
| **Future IMU** | BNO085 (9-axis) | SDA=12, SCL=11, Addr=`0x4A` | Auto-detected in `HeadTracker::begin()`. |
| **Buttons** | Tactile switches | BTN_LEFT=0, BTN_CENTER=47, BTN_RIGHT=48 | Center: Short=Zero, Long=Calibrate. Right: Live/Wave. |
| **RGB LED** | WS2812 Neopixel | PWR=GPIO 7, DATA=GPIO 8 | Green=TX OK, Red=TX Error, Cyan=Zeroed, Yellow=Calibrating |

---

## 3. Critical Special Cases & Gotchas (Lessons Learned)

### Gotcha 1: The Inverted IMU Mounting (The 172 $\leftrightarrow$ 1811 Clamp Jump)
* **The Problem:** When holding the board with the display facing UP, both Pitch and Roll sat at extreme limits and violently jumped between **172** and **1811** with nothing in between. But holding it upside down (display to the floor) worked smoothly.
* **Root Cause:** The 0.85" TFT display is mounted on the **top** face of the PCB, but WEMOS soldered the QMI8658 IMU on the **bottom** face. When the display points UP, the IMU's Z-axis points **DOWN** ($az \approx -1.0g$). In `atan2f(..., az)`, having a negative denominator places the resting vector at $\pm 180^\circ$. Microscopic sensor noise flipped the angle between $+179^\circ$ and $-179^\circ$, which clamped directly to `CRSF_CHANNEL_MIN` (172) and `CRSF_CHANNEL_MAX` (1811).
* **The Solution:** In [`src/headtracker.cpp`](file:///C:/Users/patri/.gemini/antigravity/scratch/ELRS-Backpack-Example-ESPNOW/src/headtracker.cpp), use the upward normal reference **`-az`** ($+1.0g$ when display faces up):
  ```cpp
  // Normal reference: -az is +1.0g when display faces UP
  float pitchAcc = atan2f(-ax, -az) * (180.0f / M_PI);
  float rollAcc  = atan2f(ay, -az) * (180.0f / M_PI);

  _pitch = alpha * (_pitch + gy * dt) + (1.0f - alpha) * pitchAcc;
  _roll  = alpha * (_roll  + gx * dt) + (1.0f - alpha) * rollAcc;
  _yaw  += (-gz) * dt; // -gz because the vertical axis is reversed
  ```

### Gotcha 2: The "Sniffer" / Promiscuous Mode Freeze
* **The Problem:** S3 display showed `RX: WAIT...` indefinitely, even with the radio backpack turned on and telemetry active.
* **Root Cause:** In [`src/fake_vrx_fake_trainer.cpp`](file:///C:/Users/patri/.gemini/antigravity/scratch/ELRS-Backpack-Example-ESPNOW/src/fake_vrx_fake_trainer.cpp), `custom_esp_now_send()` called `esp_wifi_set_promiscuous(true)` before each transmission and `false` after. In ESP-IDF, promiscuous mode is literally the Wi-Fi packet sniffer. Toggling it at 50 Hz continuously resets the Wi-Fi baseband receiver state machine, breaking incoming ESP-NOW reception (`OnDataRecv`).
* **The Solution:** `esp_wifi_80211_tx()` does **not** need promiscuous mode. Promiscuous mode calls were removed completely.

### Gotcha 3: ExpressLRS Binding UID & IEEE 802 Unicast MAC Rule
* **Binding Phrase:** `"YOUR_BINDING_PHRASE"`.
* **Verified UID:** `{0, 0, 0, 0, 0, 0}` (`0x00, 0x00, 0x00, 0x00, 0x00, 0x00`) obtained directly from the [ELRS Web Flasher](https://expresslrs.github.io/web-flasher/) and the TX Backpack Web UI. Standard MD5 hashing strings in external scripts produces a different hash than the official ELRS generator.
* **Unicast Rule (`UID[0] &= ~0x01`):** In IEEE 802 / Wi-Fi, bit 0 of the first octet denotes multicast/broadcast. ESP-NOW drops packets with multicast source/peer MAC addresses. In `initWiFi()`, `UID[0] &= ~0x01` clears bit 0, turning `0x15` (21) into `0x14` (20). The effective MAC is `14:77:28:A0:8F:F0`.

### Gotcha 4: Windows Python UTF-8 Progress Bar Crash
* **The Problem:** PlatformIO `pio run` crashes on Windows with `UnicodeEncodeError: 'charmap' codec can't encode characters in position ...`.
* **Root Cause:** Windows PowerShell defaults Python `sys.stdout` to Windows-1252 (`cp1252`), which cannot render PlatformIO's Unicode progress bar characters (`░`, `█`).
* **The Solution:** Always prepend environment variables before invoking PlatformIO:
  ```powershell
  $env:PYTHONIOENCODING = "utf-8"; $env:PYTHONUTF8 = "1"; & "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e lolin_s3_mini_pro --target upload
  ```

### Gotcha 5: RF Output Power Throttling
* In original code, `esp_wifi_set_max_tx_power(8)` was present, limiting RF power to only $2\text{ dBm}$ (1.5 mW).
* This was replaced with `esp_wifi_set_max_tx_power(78)` (the maximum $19.5\text{ dBm}$), providing full wireless range between headtracker and radio.

---

## 4. Configuration Reference ([`src/my_config.h`](file:///C:/Users/patri/.gemini/antigravity/scratch/ELRS-Backpack-Example-ESPNOW/src/my_config.h))

```cpp
// 1. ELRS Binding UID
#define MY_CUSTOM_UID {0, 0, 0, 0, 0, 0}

// 2. Axis Inversion (Tilt and Yaw inverted per user preference)
#define HT_INVERT_PAN       true   // Inverted (Yaw)
#define HT_INVERT_TILT      true   // Inverted (Tilt)
#define HT_INVERT_ROLL      false  // Non-inverted (Roll)

// 3. EdgeTX Trainer Channel Mapping (0-indexed: 0=TR1, 1=TR2, 2=TR3)
#define HT_CH_PAN           0   // TR1 = Pan
#define HT_CH_TILT          1   // TR2 = Tilt
#define HT_CH_ROLL          2   // TR3 = Roll

// 4. Max Angle Throws (maps to 172..1811 CRSF)
#define HT_MAX_ANGLE_PAN    90.0f  // +/- 90 degrees
#define HT_MAX_ANGLE_TILT   60.0f  // +/- 60 degrees
#define HT_MAX_ANGLE_ROLL   45.0f  // +/- 45 degrees
```

---

## 5. Radio Controller (EdgeTX) Setup Guide

1. **Model Settings:**
   * **Trainer Mode:** Set to `Master / CRSF`.
2. **Radio Tools $\to$ ExpressLRS Backpack Lua Script:**
   * **HT Enable:** `On`
   * **HT Start Ch:** `EdgeTX` (starts assigning from `TR1`)
   * **Telemetry:** `ESPNOW` (leaves bidirectional telemetry active for `RX: OK` on S3 screen).
3. **Model Inputs / Mixer:**
   * Pan camera servo $\to$ Source: `TR1`
   * Tilt camera servo $\to$ Source: `TR2`
   * Roll camera servo $\to$ Source: `TR3`

---

## 6. Future Upgrade Path: BNO085 Sensor Integration

When your BNO085 arrives:
1. Connect to I2C: `SDA -> GPIO 12`, `SCL -> GPIO 11`, `3V3`, `GND`.
2. Address: Default is `0x4A`.
3. Auto-detection is already implemented in [`HeadTracker::begin()`](file:///C:/Users/patri/.gemini/antigravity/scratch/ELRS-Backpack-Example-ESPNOW/src/headtracker.cpp#L20-L28).
4. Simply initialize the BNO085 driver in the `_usingBno = true` branch to read hardware rotation vector quaternions (eliminating all gyro drift entirely).
