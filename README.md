# 🎯 Wireless FPV HeadTracker for ExpressLRS Backpack (ESP-NOW)

[![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP32--C3%20%7C%20ESP32--S3-orange)](platformio.ini)
[![Sensor](https://img.shields.io/badge/IMU-BNO085%209--DOF%20%7C%20QMI8658-blue)](src/headtracker.cpp)
[![Protocol](https://img.shields.io/badge/Protocol-ELRS%20Backpack%20ESP--NOW%20%2F%20CRSF-green)](src/fake_vrx_fake_trainer.cpp)

Jump to: **[🇺🇸 English Documentation](#-english-documentation)** | **[🇩🇪 Deutsche Dokumentation](#-deutsche-dokumentation)**

---

# 🇺🇸 English Documentation

## 📖 Overview

This project turns an **ESP32** and an **IMU sensor** into a **completely wireless, low-latency 3-axis FPV HeadTracker (Pan / Tilt / Roll)** that transmits directly to your **ExpressLRS TX Backpack** over **ESP-NOW** (`MSP_ELRS_BACKPACK_SET_PTR` / 11-bit CRSF Trainer channels `TR1..TR3` at 50 Hz) — **no trainer cable between your FPV goggles and RC radio required!**

It evolved from an ExpressLRS Backpack ESP-NOW example into a full-featured, ready-to-build HeadTracker system including a custom 3D-printable goggle case, 9-DOF quaternion math, an interactive USB Serial ASCII dashboard, and non-volatile settings storage.

### 🙏 Credits & Acknowledgments

A huge thank you to **[druckgott](https://github.com/druckgott)** and his original repository **[ELRS-Backpack-Example-ESPNOW](https://github.com/druckgott/ELRS-Backpack-Example-ESPNOW)**, which provided the foundation for bidirectional ESP-NOW communication with the ExpressLRS Backpack, CRSF telemetry decoding, MSP packet framing, and the raw 802.11 TX bypass on ESP32.

---

## ✨ What Was Done / Key Features

* 🧭 **9-DOF Sensor Fusion (BNO085) with Zero Yaw Drift:**
  * Primary support for the **Waveshare ESP32-C3-Zero (Mini)** paired with an external **Adafruit / Hillcrest BNO085 9-DOF IMU** over 400 kHz I2C.
  * Three live-selectable fusion modes:
    * `Mode 0`: **9-DOF Rotation Vector** (Accelerometer + Gyroscope + Magnetometer, anchored to magnetic North for **0% long-term yaw drift** with automatic background Dynamic Calibration).
    * `Mode 1`: **9-DOF AR/VR Stabilized Rotation Vector** (headset-tuned 9-DOF fusion that suppresses micro-jitter and magnetic swimming).
    * `Mode 2`: **6-DOF Game Rotation Vector** (Accelerometer + Gyroscope only, 100% immune to magnetic interference from headphone speakers or fans).
* 📐 **Gravity-Aligned 3D Quaternion Math (Zero Axis Cross-Talk):**
  * Uses full 3D quaternion transformations:
    $$q_{\text{rel}} = q_{\text{yaw},0}^{-1} \otimes q_{\text{current}} \otimes q_{\text{tilt},0}^{-1}$$
  * Separates **World-Z azimuth zeroing** (around true Gravity Up) from **local goggle mounting tilt/roll** compensation. Even if the headtracker is mounted at a steep angle on curved FPV goggles, panning your head left/right produces **0% roll or pitch bleed**.
  * Includes an $\text{SO}(3)$ quaternion space $90^\circ$ axis swap (`HT_SWAP_PITCH_ROLL`) prior to Intrinsic $Z\text{-}Y'\text{-}X''$ (`Pan -> Tilt -> Roll`) Euler extraction to support rotated board mounting.
* 🖥️ **Interactive USB Serial Console & Live ASCII Dashboard:**
  * Built-in real-time ASCII horizon/level meters over USB CDC (`115200` baud) for headless boards like the ESP32-C3 — view live angles, CRSF channel values (`TR1..TR3`), packet counters, and toggle axis inversions, fusion modes, or WiFi TX power on the fly.
  * All runtime adjustments (`'f'`, `'p'`, `'y'`, `'o'`, `'s'`, `'t'`) are automatically saved to **ESP32 NVS (`Preferences`)** so they survive power cycles.

![HeadTracker Serial Debug Dashboard](img/Headtracker_debug_screen.png)

* 🔘 **Single-Button Operation & WS2812 RGB Status LED:**
  * **Short Click (< 1.5s)** on the onboard BOOT switch (`GPIO 9`): Center / Zero head position (`Cyan` LED flash).
  * **Long Press (> 1.5s)** on the onboard BOOT switch (`GPIO 9`): Hardware Tare / Calibrate IMU (`Yellow` LED flash).
  * **Heartbeat LED (`GPIO 10`):** Solid `Green` = ESP-NOW TX OK, `Red` = TX Error, `Dim Cyan` = Initializing.
* ❄️ **Cool RF Operation (Adjustable WiFi TX Power):**
  * Defaults to `10.0 dBm` (`10 mW`) instead of `19.5 dBm` (`89 mW`), reducing board heat by ~89% while easily covering the 1–2 meter distance between goggles and transmitter. Adjustable live via Serial (`'t'`).
* 💻 **Dual-Hardware Support:**
  * **Target 1 (Default):** **Waveshare ESP32-C3-Zero (Mini)** + external **BNO085** (`env:esp32c3`).
  * **Target 2 (Alternative):** **WEMOS LOLIN S3 Mini Pro** (`env:lolin_s3_mini_pro`) with onboard **QMI8658 6-DOF IMU** and **0.85" 128x128 TFT LCD** (ST7789 / GC9A01 auto-detection).
* 📦 **Custom 3D-Printable Enclosure (`case/`):**
  * Includes a ready-to-print 3MF file ([`case/Headtracker-Case_V6.3mf`](case/Headtracker-Case_V6.3mf)) and parametric OpenSCAD source ([`case/Headtracker-Case_V6.scad`](case/Headtracker-Case_V6.scad)) designed to house both the **Waveshare ESP32-C3-Zero** and **Adafruit BNO085 STEMMA QT** board in a ultra-compact stacked enclosure with integrated BOOT/RESET button plungers, LED viewport, internal PCB retention clip, L-shaped snap lid, and goggle strap / Velcro mount.

---

## 🛠️ How to Build Your Own HeadTracker

### 1. Required Hardware

#### Option A: Primary Build (Recommended — Zero Yaw Drift 9-DOF)
* **Microcontroller:** Waveshare ESP32-C3-Zero (ESP32-C3 Mini with onboard WS2812 RGB LED and USB-C)
* **IMU Sensor:** Adafruit 9-DOF Orientation IMU Fusion Breakout - BNO085 (STEMMA QT / Qwiic) or compatible GY-BNO085 module
* **Case:** 3D-printed enclosure from [`case/Headtracker-Case_V6.3mf`](case/Headtracker-Case_V6.3mf)
* **Wires:** 6 short silicone wires (30 AWG) for I2C + RST/INT

#### Option B: All-in-One Build (6-DOF Onboard IMU + TFT Screen)
* **Board:** WEMOS LOLIN S3 Mini Pro (includes onboard QMI8658 6-axis IMU, 0.85" TFT display, RGB LED, and 3 buttons — no external wiring needed)

---

### 2. Wiring Diagram (Waveshare ESP32-C3 + BNO085)

| BNO085 Pin | Waveshare ESP32-C3 Pin | Function / Notes |
| :--- | :--- | :--- |
| **VIN / 3V3** | **3V3** | 3.3V Power Supply |
| **GND** | **GND** | Common Ground |
| **SDA** | **GPIO 4** | I2C Data (400 kHz) |
| **SCL** | **GPIO 5** | I2C Clock (400 kHz) |
| **INT** | **GPIO 6** | Active-LOW Data Ready Interrupt (internal pull-up enabled) |
| **RST** | **GPIO 7** | Active-LOW Hardware Reset (50ms boot pulse) |

*(Built-in peripherals on the Waveshare ESP32-C3-Zero: **GPIO 9** = BOOT Button, **GPIO 10** = WS2812 RGB LED).*

---

### 3. 3D Printing the Hardware Case (`case/`)

The [`case/`](case) folder contains the custom enclosure specifically engineered for the **Waveshare ESP32-C3-Zero + Adafruit BNO085 STEMMA QT** boards:
* **[`case/Headtracker-Case_V6.3mf`](case/Headtracker-Case_V6.3mf):** Ready-to-slice 3MF project file.
* **[`case/Headtracker-Case_V6.scad`](case/Headtracker-Case_V6.scad):** Parametric OpenSCAD file (uses the [BOSL2 library](https://github.com/BelfrySCAD/BOSL2)) allowing you to customize layout (`stacked` vs. `flat`), goggle strap slots vs. flat Velcro backplate, tolerances, and button plunger length (`1.70 mm`).

**Printing & Assembly Tips:**
* Recommended material: **PETG** or **PLA+** with `0.16 mm` or `0.20 mm` layer height, no supports required when printing `base` and `lid` flat on the bed.
* Slide the BNO085 into the lower rail slot first, then snap the ESP32-C3-Zero under the rear retention clip on the upper deck with the USB-C port facing the front opening, and close the L-shaped snap lid.

---

### 4. Configure Your ExpressLRS Binding UID

The ESP32 communicates with your radio's ExpressLRS Backpack using a 6-byte MAC address (`UID`) derived from your **ELRS Binding Phrase**.

1. Open the **[ExpressLRS Web Flasher](https://expresslrs.github.io/web-flasher/)** (or check your ELRS TX Backpack Web UI) and type in your binding phrase to get your 6-byte `UID`:

![ELRS Config](img/config.png)
![ELRS UID](img/uid.png)

2. Open [`src/my_config.h`](src/my_config.h) (you can also use [`src/my_config.h.example`](src/my_config.h.example) as a reference) and enter your 6 UID numbers into `MY_CUSTOM_UID`:

```cpp
// 1. Your ExpressLRS Binding UID
#define MY_CUSTOM_UID {0, 0, 0, 0, 0, 0} // <-- Replace with your 6 numbers!

// 2. BNO085 Sensor Fusion Mode (0 = 9-DOF Mag, 1 = 9-DOF AR/VR, 2 = 6-DOF Game RV)
#define BNO085_FUSION_MODE  0

// 3. Axis Inversion & 90-Degree Mounting Swap
#define HT_INVERT_PAN       true
#define HT_INVERT_TILT      false
#define HT_INVERT_ROLL      true
#define HT_SWAP_PITCH_ROLL  true   // Set true if sensor is mounted rotated by 90 degrees

// 4. Max Physical Head Deflection Angles (mapped to -100% .. +100% CRSF 172..1811)
#define HT_MAX_ANGLE_PAN    90.0f  // +/- 90 deg (TR1)
#define HT_MAX_ANGLE_TILT   60.0f  // +/- 60 deg (TR2)
#define HT_MAX_ANGLE_ROLL   45.0f  // +/- 45 deg (TR3)
```

---

### 5. Build, Flash & Test via Serial Monitor

1. Install [VS Code](https://code.visualstudio.com/) with the **PlatformIO IDE** extension.
2. Connect your **Waveshare ESP32-C3** via USB-C (adjust `upload_port` / `monitor_port` in [`platformio.ini`](platformio.ini) if needed) and flash:
   ```bash
   pio run -e esp32c3 --target upload
   ```
   *(For the WEMOS LOLIN S3 Mini Pro, use `pio run -e lolin_s3_mini_pro --target upload`).*
3. Open the PlatformIO Serial Monitor (`115200` baud):
   ```bash
   pio device monitor -e esp32c3
   ```

#### Interactive Serial Commands

| Key | Action |
| :---: | :--- |
| **`c`** or **`SPACE`** | **Center / Zero** head position to current orientation |
| **`r`** | **Calibrate / Tare** IMU sensor |
| **`f`** | **Cycle IMU Fusion Mode** (`9-DOF Mag` $\leftrightarrow$ `AR/VR 9-DOF` $\leftrightarrow$ `6-DOF Game RV`) *(saved to NVS)* |
| **`p`** | **Toggle Invert Pitch / Tilt** *(saved to NVS)* |
| **`y`** | **Toggle Invert Pan / Yaw** *(saved to NVS)* |
| **`o`** | **Toggle Invert Roll** *(saved to NVS)* |
| **`s`** | **Toggle Swap Pitch & Roll** for $90^\circ$ rotated mounting *(saved to NVS)* |
| **`t`** | **Cycle WiFi TX Power** (`10dBm` $\to$ `8.5dBm` $\to$ `5dBm` $\to$ `2dBm` $\to$ `19.5dBm` $\to$ `15dBm`) *(saved to NVS)* |
| **`m`** | **Toggle Serial Display Mode** (`Compact Single-Line` $\leftrightarrow$ `Visual ASCII Dashboard`) |
| **`w`** | **Toggle Test Wave Mode** (`LIVE HeadTracker` $\leftrightarrow$ `TEST WAVE` sine generator) |
| **`d`** | **Mute / Unmute** periodic Serial output |
| **`1` .. `3`** | **Select Radio Mode** (`1` = RX Only, `2` = TX Only, `3` = Bidirectional BOTH) |
| **`?`** or **`h`** | Display help menu |

---

### 6. ExpressLRS Backpack & EdgeTX Transmitter Setup

Official ExpressLRS Trainer Input documentation: https://www.expresslrs.org/software/trainer-input/

#### A. ExpressLRS TX Backpack Settings
1. Ensure your TX Backpack runs **ExpressLRS Backpack firmware v1.5.1+** with the **same Binding Phrase / UID**.
2. In the **ExpressLRS Lua Script** (or Backpack Web UI), configure:
   * **Backpack:** `On`
   * **Telemetry:** `ESPNOW` (enables ESP-NOW link)
   * **HT Enable:** `On`
   * **HT Start Ch:** `EdgeTX` (outputs headtracking on `TR1`, `TR2`, `TR3`)

<img width="475" height="312" alt="ELRS Backpack Web UI" src="https://github.com/user-attachments/assets/ac6c4348-ead1-44a8-ac6f-fea9a810f884" />

![ELRS Config 1](img/screen-2026-06-15-173347.bmp)
![ELRS Config 2](img/screen-2026-06-14-174124.bmp)
![ELRS Config 3](img/screen-2026-06-15-171914.bmp)

#### B. EdgeTX Model Settings
1. Open **Model Setup** $\to$ **Trainer** and set **Mode** to **`Master / CRSF`**.
2. Map **`TR1` (Pan)**, **`TR2` (Tilt)**, and **`TR3` (Roll)** in your **Inputs / Mixes** to your camera gimbal servo channels:

![Trainer Step 1](img/screen-2026-03-13-172119.bmp)
![Trainer Step 2](img/screen-2026-03-13-172123.bmp)
![Trainer Step 3](img/screen-2026-03-13-172126.bmp)
![Trainer Step 4](img/screen-2026-03-13-172129.bmp)
![Trainer Step 5](img/screen-2026-03-13-172138.bmp)
![Trainer Step 6](img/screen-2026-03-13-172142.bmp)

> [!TIP]
> **Troubleshooting Trainer Detection in EdgeTX:**
> Sometimes EdgeTX receives telemetry via ESP-NOW, but does not immediately lock onto the `Master / CRSF` trainer stream. Re-selecting `Master / CRSF` in the Model Setup or toggling `HT Enable` (`Off` $\to$ `On`) in the ExpressLRS Backpack Lua script resolves this immediately.

---

# 🇩🇪 Deutsche Dokumentation

## 📖 Übersicht

Dieses Projekt verwandelt einen **ESP32-C3** und einen **9-DOF IMU-Sensor (BNO085)** in einen **komplett kabellosen 3-Achsen FPV-HeadTracker (Pan / Tilt / Roll)**, der seine Kanaldaten direkt per **ESP-NOW** (`MSP_ELRS_BACKPACK_SET_PTR` / 11-Bit CRSF Trainer-Kanäle `TR1..TR3` mit 50 Hz) an das **ExpressLRS TX-Backpack** der Fernsteuerung überträgt.

Das Projekt basiert auf dem ESP-NOW Bridge-Beispiel von **[druckgott](https://github.com/druckgott/ELRS-Backpack-Example-ESPNOW)** (vielen Dank für die großartige Vorarbeit bei der ESP-NOW / CRSF / Raw-802.11-Implementierung!) und wurde zu einem vollwertigen HeadTracker-System inkl. 3D-druckbarem Gehäuse, gravitationsausgerichteter 3D-Quaternionen-Mathematik und interaktivem Serial-Dashboard ausgebaut.

---

## ✨ Neuerungen & Features

* 🧭 **Driftfreies 9-DOF Tracking mit dem BNO085:**
  * Unterstützt den kompakten **Waveshare ESP32-C3-Zero (Mini)** mit externem **Adafruit / Hillcrest BNO085 9-DOF Sensor** über 400 kHz I2C.
  * 3 umschaltbare Fusions-Modi (`9-DOF Rotation Vector` mit Magnetometer für **0% Yaw-Drift**, `9-DOF AR/VR Stabilized` für Videobrillen, sowie `6-DOF Game Rotation Vector` ohne Magnetometer).
* 📐 **Gravitationsausgerichtete 3D-Quaternionen-Berechnung (0% Achsen-Übersprechen):**
  * Berechnet die relative Kopfbewegung über:
    $$q_{\text{rel}} = q_{\text{yaw},0}^{-1} \otimes q_{\text{current}} \otimes q_{\text{tilt},0}^{-1}$$
  * Dadurch bleibt die horizontale Drehachse (Pan) immer exakt an der echten Schwerkraft-Hochachse ausgerichtet, während die Einbauneigung an gewölbten FPV-Brillen automatisch herausgerechnet wird — **kein Übersprechen (Cross-Talk) zwischen Pan, Tilt und Roll**, selbst bei schräger Montage!
  * Unterstützt $90^\circ$ gedrehten Einbau (`HT_SWAP_PITCH_ROLL`) direkt im Quaternionen-Raum.
* 🖥️ **Interaktives USB-Serial ASCII-Dashboard & NVS-Speicherung:**
  * Echtzeit-Wasserwaage / Balkenanzeige über USB-Serial (`115200` Baud) für Boards ohne eigenes Display (siehe [`img/Headtracker_debug_screen.png`](img/Headtracker_debug_screen.png)).
  * Alle per Tastendruck im Serial-Monitor geänderten Einstellungen (Achsen-Invertierung, $90^\circ$-Tausch, Fusions-Modus, WLAN-Sendeleistung) werden automatisch dauerhaft im **ESP32 Flash (NVS)** gespeichert.
* 🔘 **Ein-Tasten-Bedienung & RGB-Status-LED:**
  * **Kurzer Klick (< 1.5s)** auf den BOOT-Taster (`GPIO 9`): HeadTracker zentrieren / nullen (`Cyan` LED-Blinken).
  * **Langer Druck (> 1.5s)** auf den BOOT-Taster (`GPIO 9`): Sensor kalibrieren / Tare (`Gelb` LED-Blinken).
  * **Status-LED (`GPIO 10`):** `Grün` = ESP-NOW Senden OK, `Rot` = Sendefehler.
* 📦 **3D-Druck-Gehäuse im Ordner [`case/`](case):**
  * Druckfertige 3MF-Datei ([`case/Headtracker-Case_V6.3mf`](case/Headtracker-Case_V6.3mf)) sowie parametrisches OpenSCAD-Modell ([`case/Headtracker-Case_V6.scad`](case/Headtracker-Case_V6.scad)) für **Waveshare ESP32-C3-Zero + Adafruit BNO085 STEMMA QT** mit integrierten Taster-Stößeln, LED-Sichtfenster, Platinen-Schnappclip, L-Deckel und Klett-/Kopfband-Halterung.

---

## 🛠️ Eigenen HeadTracker nachbauen (Schritt-für-Schritt)

### 1. Verkabelung (Waveshare ESP32-C3-Zero + BNO085)

| BNO085 Pin | Waveshare ESP32-C3 Pin | Funktion |
| :--- | :--- | :--- |
| **VIN / 3V3** | **3V3** | 3.3V Stromversorgung |
| **GND** | **GND** | Masse (GND) |
| **SDA** | **GPIO 4** | I2C Datenleitung (400 kHz) |
| **SCL** | **GPIO 5** | I2C Taktleitung (400 kHz) |
| **INT** | **GPIO 6** | Interrupt-Leitung (Active-LOW, interner Pull-Up aktiv) |
| **RST** | **GPIO 7** | Hardware-Reset (Active-LOW) |

*(Alternativ wird auch das **WEMOS LOLIN S3 Mini Pro** mit integriertem QMI8658 6-Achsen-Sensor und 0.85" TFT-Display über das PlatformIO-Environment `lolin_s3_mini_pro` unterstützt).*

### 2. Gehäuse drucken (`case/`)
* Drucke [`case/Headtracker-Case_V6.3mf`](case/Headtracker-Case_V6.3mf) aus PETG oder PLA+ (Schichthöhe `0.16 mm`–`0.20 mm`, kein Support nötig).
* Zuerst den BNO085 in die untere Führungsschiene einschieben, danach den ESP32-C3-Zero oben unter den hinteren Halteclip drücken und den L-Deckel aufclipsen.

### 3. ELRS UID eintragen & Firmware flashen
1. Eigene 6-Byte `UID` über den **[ExpressLRS Web Flasher](https://expresslrs.github.io/web-flasher/)** aus der eigenen Binding-Phrase generieren.
2. In [`src/my_config.h`](src/my_config.h) unter `#define MY_CUSTOM_UID { ... }` eintragen.
3. Mit PlatformIO kompilieren und flashen:
   ```bash
   pio run -e esp32c3 --target upload
   ```
4. Im Serial Monitor (`pio device monitor -e esp32c3`) mit `'c'` zentrieren und bei Bedarf mit `'p'`, `'y'`, `'o'`, `'s'` die Wirkrichtungen an das eigene Kamera-Gimbal anpassen.

### 4. EdgeTX & ELRS Backpack einrichten
1. Im **ExpressLRS Backpack Lua-Script** (bzw. Web-UI):
   * **Telemetry:** `ESPNOW`
   * **HT Enable:** `On`
   * **HT Start Ch:** `EdgeTX`
2. In **EdgeTX Model Setup**:
   * **Trainer Mode:** `Master / CRSF`
   * In den **Inputs / Mischern** die Quellen **`TR1` (Pan)**, **`TR2` (Tilt)** und **`TR3` (Roll)** den jeweiligen Servo-Kanälen zuweisen.

Weitere technische Details zur Architektur und den 9-DOF Quaternionen-Berechnungen befinden sich in **[HANDOFF.md](HANDOFF.md)**.
