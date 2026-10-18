import gzip
import os

HTML_SOURCE = r"""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1.0,maximum-scale=1.0,user-scalable=no">
<title>ELRS Backpack HeadTracker — Web Configuration & 3D Telemetry</title>
<style>
:root {
  --bg: #080b12;
  --bg-card: #101624;
  --bg-card-alt: #151d30;
  --border: #22304d;
  --text: #e6edf7;
  --text-dim: #8b9bb8;
  --cyan: #00f0ff;
  --orange: #ff8a00;
  --magenta: #d946ef;
  --purple-ap: #7a00b8;
  --green: #10b981;
  --yellow: #f59e0b;
  --red: #ef4444;
  --radius: 12px;
}
* { box-sizing: border-box; margin: 0; padding: 0; }
body {
  font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
  background: radial-gradient(circle at 50% 0%, #161130 0%, var(--bg) 55%);
  color: var(--text);
  min-height: 100vh;
  line-height: 1.45;
  padding-bottom: 40px;
}
header {
  background: rgba(16, 22, 36, 0.88);
  backdrop-filter: blur(10px);
  border-bottom: 1px solid var(--border);
  position: sticky;
  top: 0;
  z-index: 100;
  padding: 12px 18px;
}
.header-inner {
  max-width: 1240px;
  margin: 0 auto;
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}
.brand {
  display: flex;
  align-items: center;
  gap: 12px;
}
.ap-led {
  width: 13px;
  height: 13px;
  border-radius: 50%;
  background: #a820f0;
  box-shadow: 0 0 12px #a820f0, 0 0 22px var(--purple-ap);
  animation: breatheMagenta 2s infinite ease-in-out;
}
@keyframes breatheMagenta {
  0%, 100% { opacity: 0.28; transform: scale(0.88); box-shadow: 0 0 4px var(--purple-ap); }
  50% { opacity: 1; transform: scale(1.12); box-shadow: 0 0 14px #d946ef, 0 0 26px var(--purple-ap); }
}
.brand-title {
  font-size: 1.12rem;
  font-weight: 700;
  letter-spacing: 0.3px;
  background: linear-gradient(90deg, #fff, var(--cyan), #d946ef);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
}
.brand-sub {
  font-size: 0.75rem;
  color: var(--text-dim);
}
.status-pills {
  display: flex;
  align-items: center;
  gap: 8px;
  flex-wrap: wrap;
}
.pill {
  font-size: 0.74rem;
  font-weight: 600;
  padding: 4px 10px;
  border-radius: 999px;
  background: var(--bg-card-alt);
  border: 1px solid var(--border);
  color: var(--text-dim);
  display: inline-flex;
  align-items: center;
  gap: 6px;
}
.pill.ok { color: var(--green); border-color: rgba(16,185,129,0.4); }
.pill.warn { color: var(--yellow); border-color: rgba(245,158,11,0.4); }
.pill.ap { color: #e879f9; border-color: rgba(217,70,239,0.45); background: rgba(122,0,184,0.18); }

.tabs-bar {
  max-width: 1240px;
  margin: 16px auto 0;
  padding: 0 16px;
  display: flex;
  gap: 8px;
  flex-wrap: wrap;
}
.tab-btn {
  background: var(--bg-card);
  color: var(--text-dim);
  border: 1px solid var(--border);
  padding: 9px 18px;
  border-radius: 10px;
  font-size: 0.88rem;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.18s;
}
.tab-btn:hover { color: var(--text); border-color: #364d7a; }
.tab-btn.active {
  background: linear-gradient(135deg, rgba(0,240,255,0.15), rgba(122,0,184,0.28));
  color: #fff;
  border-color: var(--cyan);
  box-shadow: 0 0 14px rgba(0,240,255,0.16);
}

.container {
  max-width: 1240px;
  margin: 16px auto;
  padding: 0 16px;
}
.tab-panel { display: none; }
.tab-panel.active { display: block; }

.grid-main {
  display: grid;
  grid-template-columns: 1.05fr 0.95fr;
  gap: 16px;
}
@media (max-width: 920px) {
  .grid-main { grid-template-columns: 1fr; }
}

.card {
  background: var(--bg-card);
  border: 1px solid var(--border);
  border-radius: var(--radius);
  padding: 16px;
  box-shadow: 0 8px 24px rgba(0,0,0,0.35);
  margin-bottom: 16px;
}
.card-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 12px;
  padding-bottom: 8px;
  border-bottom: 1px solid rgba(255,255,255,0.06);
}
.card-title {
  font-size: 0.95rem;
  font-weight: 700;
  color: #fff;
  display: flex;
  align-items: center;
  gap: 8px;
}
.card-badge {
  font-size: 0.72rem;
  padding: 2px 8px;
  border-radius: 6px;
  background: var(--bg-card-alt);
  color: var(--cyan);
  border: 1px solid rgba(0,240,255,0.25);
}

/* 3D Viewport */
.viewport-wrap {
  position: relative;
  width: 100%;
  height: 310px;
  background: radial-gradient(circle at 50% 42%, #192540 0%, #090d16 80%);
  border: 1px solid #243456;
  border-radius: 10px;
  overflow: hidden;
}
#canvas3d {
  width: 100%;
  height: 100%;
  display: block;
}
.hud-overlay {
  position: absolute;
  top: 10px;
  left: 10px;
  right: 10px;
  display: flex;
  justify-content: space-between;
  pointer-events: none;
  gap: 6px;
  flex-wrap: wrap;
}
.hud-chip {
  background: rgba(8, 11, 18, 0.78);
  border: 1px solid rgba(255,255,255,0.12);
  padding: 4px 9px;
  border-radius: 6px;
  font-family: "SFMono-Regular", Consolas, monospace;
  font-size: 0.78rem;
  font-weight: 600;
}
.hud-pan { color: var(--cyan); border-color: rgba(0,240,255,0.35); }
.hud-tilt { color: var(--orange); border-color: rgba(255,138,0,0.35); }
.hud-roll { color: var(--magenta); border-color: rgba(217,70,239,0.35); }

/* Action Buttons */
.action-row {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 10px;
  margin-top: 12px;
}
.btn {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
  padding: 10px 14px;
  border-radius: 9px;
  font-size: 0.86rem;
  font-weight: 700;
  border: 1px solid transparent;
  cursor: pointer;
  transition: all 0.15s;
  color: #fff;
}
.btn:active { transform: translateY(1px); }
.btn-cyan {
  background: rgba(0, 240, 255, 0.14);
  border-color: var(--cyan);
  color: var(--cyan);
}
.btn-cyan:hover { background: rgba(0, 240, 255, 0.25); box-shadow: 0 0 14px rgba(0,240,255,0.28); }
.btn-yellow {
  background: rgba(245, 158, 11, 0.15);
  border-color: var(--yellow);
  color: #fbbf24;
}
.btn-yellow:hover { background: rgba(245, 158, 11, 0.26); box-shadow: 0 0 14px rgba(245,158,11,0.28); }
.btn-primary {
  background: linear-gradient(135deg, #0284c7, #7a00b8);
  border-color: #38bdf8;
  color: #fff;
  width: 100%;
  padding: 12px;
  font-size: 0.94rem;
}
.btn-primary:hover { filter: brightness(1.12); box-shadow: 0 0 18px rgba(0,240,255,0.3); }
.btn-secondary {
  background: var(--bg-card-alt);
  border-color: var(--border);
  color: var(--text);
}
.btn-secondary:hover { border-color: var(--cyan); }

/* Rolling Graph & CRSF Bars */
.graph-wrap {
  width: 100%;
  height: 175px;
  background: #090d16;
  border: 1px solid #243456;
  border-radius: 10px;
  overflow: hidden;
  position: relative;
}
#canvasGraph {
  width: 100%;
  height: 100%;
  display: block;
}
.crsf-bars {
  margin-top: 12px;
  display: flex;
  flex-direction: column;
  gap: 8px;
}
.crsf-row {
  display: grid;
  grid-template-columns: 82px 1fr 105px;
  align-items: center;
  gap: 8px;
  font-size: 0.78rem;
}
.crsf-label { font-weight: 700; }
.crsf-track {
  height: 10px;
  background: #090d16;
  border: 1px solid #22304d;
  border-radius: 999px;
  position: relative;
  overflow: hidden;
}
.crsf-center-mark {
  position: absolute;
  left: 50%;
  top: 0;
  bottom: 0;
  width: 2px;
  background: rgba(255,255,255,0.3);
  z-index: 2;
}
.crsf-fill {
  position: absolute;
  top: 0;
  bottom: 0;
  left: 50%;
  width: 0%;
  transition: left 0.04s linear, width 0.04s linear;
}
.crsf-val {
  font-family: "SFMono-Regular", Consolas, monospace;
  text-align: right;
  color: var(--text-dim);
}

/* Forms & Controls */
.form-group {
  margin-bottom: 13px;
}
.form-label {
  display: flex;
  justify-content: space-between;
  font-size: 0.8rem;
  font-weight: 600;
  color: var(--text-dim);
  margin-bottom: 5px;
}
.form-label span.val {
  color: var(--cyan);
  font-family: "SFMono-Regular", Consolas, monospace;
}
input[type="text"], input[type="number"], select {
  width: 100%;
  background: #0a0f1a;
  border: 1px solid var(--border);
  color: var(--text);
  padding: 9px 11px;
  border-radius: 8px;
  font-size: 0.86rem;
  outline: none;
}
input[type="text"]:focus, input[type="number"]:focus, select:focus {
  border-color: var(--cyan);
  box-shadow: 0 0 0 2px rgba(0,240,255,0.15);
}
input[type="range"] {
  width: 100%;
  accent-color: var(--cyan);
  cursor: pointer;
}
.uid-grid {
  display: grid;
  grid-template-columns: repeat(6, 1fr);
  gap: 6px;
  margin-top: 6px;
}
.uid-grid input {
  text-align: center;
  font-family: "SFMono-Regular", Consolas, monospace;
  padding: 7px 4px;
  font-weight: 700;
}
.uid-preview-box {
  background: #090d16;
  border: 1px solid #243456;
  border-radius: 8px;
  padding: 9px 11px;
  margin-top: 8px;
  font-family: "SFMono-Regular", Consolas, monospace;
  font-size: 0.78rem;
  color: var(--text-dim);
}
.uid-preview-box strong { color: var(--cyan); }

.toggle-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 8px;
  margin-bottom: 12px;
}
.toggle-item {
  background: var(--bg-card-alt);
  border: 1px solid var(--border);
  border-radius: 8px;
  padding: 8px 10px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  font-size: 0.8rem;
  font-weight: 600;
  cursor: pointer;
}
.toggle-item input {
  width: 17px;
  height: 17px;
  accent-color: var(--cyan);
  cursor: pointer;
}
.ch-grid {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 8px;
}

/* OTA Section */
.ota-dropzone {
  border: 2px dashed #344973;
  border-radius: 12px;
  padding: 28px 18px;
  text-align: center;
  background: rgba(15, 22, 38, 0.6);
  transition: all 0.2s;
  cursor: pointer;
}
.ota-dropzone:hover, .ota-dropzone.dragover {
  border-color: var(--cyan);
  background: rgba(0, 240, 255, 0.06);
}
.progress-wrap {
  margin-top: 16px;
  display: none;
}
.progress-bar-bg {
  width: 100%;
  height: 14px;
  background: #090d16;
  border: 1px solid var(--border);
  border-radius: 999px;
  overflow: hidden;
}
.progress-bar-fill {
  height: 100%;
  width: 0%;
  background: linear-gradient(90deg, var(--cyan), var(--magenta));
  transition: width 0.15s;
}

#toast {
  position: fixed;
  bottom: 20px;
  right: 20px;
  background: #141e33;
  color: #fff;
  border: 1px solid var(--cyan);
  padding: 11px 18px;
  border-radius: 10px;
  font-size: 0.85rem;
  font-weight: 600;
  box-shadow: 0 8px 24px rgba(0,0,0,0.5);
  transform: translateY(100px);
  opacity: 0;
  transition: all 0.25s;
  z-index: 999;
}
#toast.show { transform: translateY(0); opacity: 1; }
</style>
</head>
<body>

<header>
  <div class="header-inner">
    <div class="brand">
      <div class="ap-led" title="AP Configuration Mode Active (Breathing Magenta #7A00B8)"></div>
      <div>
        <div class="brand-title">ELRS Backpack HeadTracker</div>
        <div class="brand-sub">Wireless 9-DOF FPV HeadTracker &bull; Web Configuration &amp; 3D Telemetry</div>
      </div>
    </div>
    <div class="status-pills">
      <span class="pill ap">AP MODE: 192.168.4.1</span>
      <span class="pill" id="pillWs">WS: Connecting...</span>
      <span class="pill" id="pillSensor">IMU: --</span>
    </div>
  </div>
</header>

<div class="tabs-bar">
  <button class="tab-btn active" onclick="switchTab('dashboard')">3D Telemetry &amp; Tuning</button>
  <button class="tab-btn" onclick="switchTab('ota')">Wireless OTA Update</button>
</div>

<div class="container">
  <!-- TAB 1: DASHBOARD & CONFIG -->
  <div id="tab-dashboard" class="tab-panel active">
    <div class="grid-main">
      <!-- LEFT COLUMN: 3D MODEL + ROLLING GRAPH -->
      <div>
        <div class="card">
          <div class="card-header">
            <div class="card-title">3D FPV Goggles &amp; Pilot Head Orientation</div>
            <span class="card-badge" id="fpsBadge">60 FPS 3D</span>
          </div>
          <div class="viewport-wrap">
            <canvas id="canvas3d"></canvas>
            <div class="hud-overlay">
              <div style="display:flex;gap:6px;flex-wrap:wrap;">
                <span class="hud-chip hud-pan" id="hudPan">PAN: +0.0&deg;</span>
                <span class="hud-chip hud-tilt" id="hudTilt">TILT: +0.0&deg;</span>
                <span class="hud-chip hud-roll" id="hudRoll">ROLL: +0.0&deg;</span>
              </div>
              <span class="hud-chip" id="hudMode">9-DOF RV</span>
            </div>
          </div>
          <div class="action-row">
            <button class="btn btn-cyan" onclick="sendCenter()">&#9678; Center / Zero Head (Cyan)</button>
            <button class="btn btn-yellow" onclick="sendTare()">&#9881; Tare / Calibrate IMU (Yellow)</button>
          </div>
        </div>

        <div class="card">
          <div class="card-header">
            <div class="card-title">Real-Time 3-Axis Telemetry (~3s Rolling Window)</div>
            <span class="card-badge">CRSF 11-Bit (172..1811)</span>
          </div>
          <div class="graph-wrap">
            <canvas id="canvasGraph"></canvas>
          </div>
          <div class="crsf-bars">
            <div class="crsf-row">
              <span class="crsf-label" style="color:var(--cyan)" id="lblTrPan">PAN (TR1)</span>
              <div class="crsf-track"><div class="crsf-center-mark"></div><div class="crsf-fill" id="barPan" style="background:var(--cyan)"></div></div>
              <span class="crsf-val" id="valCrsfPan">992 (1500us)</span>
            </div>
            <div class="crsf-row">
              <span class="crsf-label" style="color:var(--orange)" id="lblTrTilt">TILT (TR2)</span>
              <div class="crsf-track"><div class="crsf-center-mark"></div><div class="crsf-fill" id="barTilt" style="background:var(--orange)"></div></div>
              <span class="crsf-val" id="valCrsfTilt">992 (1500us)</span>
            </div>
            <div class="crsf-row">
              <span class="crsf-label" style="color:var(--magenta)" id="lblTrRoll">ROLL (TR3)</span>
              <div class="crsf-track"><div class="crsf-center-mark"></div><div class="crsf-fill" id="barRoll" style="background:var(--magenta)"></div></div>
              <span class="crsf-val" id="valCrsfRoll">992 (1500us)</span>
            </div>
          </div>
        </div>
      </div>

      <!-- RIGHT COLUMN: BINDING PHRASE + HEADTRACKER SETTINGS -->
      <div>
        <div class="card">
          <div class="card-header">
            <div class="card-title">ExpressLRS Binding Phrase &amp; 6-Byte UID</div>
            <span class="card-badge">MD5 Auto-Hash</span>
          </div>
          <div class="form-group">
            <div class="form-label">
              <span>ExpressLRS Binding Phrase</span>
              <span class="val" id="phraseStatus">Live MD5</span>
            </div>
            <input type="text" id="inpPhrase" placeholder="Enter ELRS binding phrase (leave blank to keep manual UID)..." oninput="onPhraseInput()">
          </div>

          <label class="toggle-item" style="margin-bottom:8px;">
            <span>Manual Raw 6-Byte UID Override</span>
            <input type="checkbox" id="chkManualUid" onchange="onToggleManualUid()">
          </label>

          <div class="uid-grid">
            <input type="number" min="0" max="255" id="uid0" oninput="onManualUidChange()">
            <input type="number" min="0" max="255" id="uid1" oninput="onManualUidChange()">
            <input type="number" min="0" max="255" id="uid2" oninput="onManualUidChange()">
            <input type="number" min="0" max="255" id="uid3" oninput="onManualUidChange()">
            <input type="number" min="0" max="255" id="uid4" oninput="onManualUidChange()">
            <input type="number" min="0" max="255" id="uid5" oninput="onManualUidChange()">
          </div>

          <div class="uid-preview-box">
            <div>Unicast MAC / UID Hex: <strong id="uidHexPreview">--:--:--:--:--:--</strong></div>
            <div>C-Array (UID[0]&amp;=~1): <strong id="uidArrPreview">{0, 0, 0, 0, 0, 0}</strong></div>
          </div>
        </div>

        <div class="card">
          <div class="card-header">
            <div class="card-title">IMU Sensor Fusion &amp; Axis Configuration</div>
            <span class="card-badge">Real-Time Preview</span>
          </div>

          <div class="form-group">
            <div class="form-label"><span>BNO085 Sensor Fusion Mode</span></div>
            <select id="selFusion" onchange="pushLiveSettings()">
              <option value="0">Mode 0: 9-DOF Rotation Vector (Mag + Gyro + Accel — Zero Yaw Drift)</option>
              <option value="1">Mode 1: 9-DOF AR/VR Stabilized Rotation Vector (HMD Tuned)</option>
              <option value="2">Mode 2: 6-DOF Game Rotation Vector (Gyro + Accel Only — No Mag)</option>
            </select>
          </div>

          <div class="form-group">
            <div class="form-label"><span>Max Pan Deflection Angle (Yaw)</span><span class="val" id="lblMaxPan">&plusmn;90&deg;</span></div>
            <input type="range" id="rngMaxPan" min="15" max="180" step="5" value="90" oninput="updateAngleLabels();pushLiveSettings()">
          </div>
          <div class="form-group">
            <div class="form-label"><span>Max Tilt Deflection Angle (Pitch)</span><span class="val" id="lblMaxTilt">&plusmn;60&deg;</span></div>
            <input type="range" id="rngMaxTilt" min="15" max="120" step="5" value="60" oninput="updateAngleLabels();pushLiveSettings()">
          </div>
          <div class="form-group">
            <div class="form-label"><span>Max Roll Deflection Angle</span><span class="val" id="lblMaxRoll">&plusmn;45&deg;</span></div>
            <input type="range" id="rngMaxRoll" min="15" max="90" step="5" value="45" oninput="updateAngleLabels();pushLiveSettings()">
          </div>

          <div class="form-label"><span>Axis Inversion &amp; Mounting Orientation</span></div>
          <div class="toggle-grid">
            <label class="toggle-item"><span>Invert Pan (Yaw)</span><input type="checkbox" id="chkInvPan" onchange="pushLiveSettings()"></label>
            <label class="toggle-item"><span>Invert Tilt (Pitch)</span><input type="checkbox" id="chkInvTilt" onchange="pushLiveSettings()"></label>
            <label class="toggle-item"><span>Invert Roll</span><input type="checkbox" id="chkInvRoll" onchange="pushLiveSettings()"></label>
            <label class="toggle-item"><span>Swap Pitch/Roll (90&deg;)</span><input type="checkbox" id="chkSwapPR" onchange="pushLiveSettings()"></label>
          </div>

          <div class="form-group">
            <div class="form-label"><span>EdgeTX Trainer Channel Mapping (TR1..TR16)</span></div>
            <div class="ch-grid">
              <div>
                <div style="font-size:0.72rem;color:var(--cyan);margin-bottom:3px;">Pan Channel</div>
                <select id="selChPan" onchange="pushLiveSettings()"></select>
              </div>
              <div>
                <div style="font-size:0.72rem;color:var(--orange);margin-bottom:3px;">Tilt Channel</div>
                <select id="selChTilt" onchange="pushLiveSettings()"></select>
              </div>
              <div>
                <div style="font-size:0.72rem;color:var(--magenta);margin-bottom:3px;">Roll Channel</div>
                <select id="selChRoll" onchange="pushLiveSettings()"></select>
              </div>
            </div>
          </div>

          <div class="form-group">
            <div class="form-label"><span>WiFi / ESP-NOW RF Output Power</span></div>
            <select id="selTxPower" onchange="pushLiveSettings()">
              <option value="8">2.0 dBm (1.6 mW — Ultra Low Power)</option>
              <option value="20">5.0 dBm (3.2 mW — Low Power, 1-2m Range)</option>
              <option value="34">8.5 dBm (7.1 mW — Cool Operation)</option>
              <option value="40">10.0 dBm (10 mW — Recommended Balanced &amp; Cool)</option>
              <option value="44">11.0 dBm (13 mW — Medium-High)</option>
              <option value="60">15.0 dBm (32 mW — High Power)</option>
              <option value="78">19.5 dBm (89 mW — Maximum Power)</option>
            </select>
          </div>

          <div style="display:flex;gap:10px;margin-top:14px;">
            <button class="btn btn-secondary" style="flex:0.45;" onclick="saveConfig(false)">&#128190; Save to NVS</button>
            <button class="btn btn-primary" style="flex:1;" onclick="saveConfig(true)">&#128640; Save &amp; Reboot to Normal Mode</button>
          </div>
        </div>
      </div>
    </div>
  </div>

  <!-- TAB 2: WIRELESS OTA UPDATE -->
  <div id="tab-ota" class="tab-panel">
    <div class="card" style="max-width:640px;margin:0 auto;">
      <div class="card-header">
        <div class="card-title">Wireless Over-The-Air (OTA) Firmware Update</div>
        <span class="card-badge" id="boardBadge">ESP32-C3</span>
      </div>
      <p style="font-size:0.84rem;color:var(--text-dim);margin-bottom:16px;">
        Upload a compiled PlatformIO binary (<code style="color:var(--cyan)">.pio/build/esp32c3/firmware.bin</code>) directly over Wi-Fi.
        The device will flash the new image into the OTA partition and reboot automatically when complete.
      </p>

      <input type="file" id="otaFile" accept=".bin" style="display:none" onchange="onOtaFileSelected()">
      <div class="ota-dropzone" id="otaDropzone" onclick="document.getElementById('otaFile').click()">
        <div style="font-size:2.1rem;margin-bottom:8px;">&#128228;</div>
        <div style="font-weight:700;font-size:0.95rem;" id="otaFileName">Click or drag &amp; drop <span style="color:var(--cyan)">firmware.bin</span> here</div>
        <div style="font-size:0.78rem;color:var(--text-dim);margin-top:4px;" id="otaFileSize">Supports ESP32-C3 / ESP32-S3 Arduino OTA binary images</div>
      </div>

      <div class="progress-wrap" id="otaProgressWrap">
        <div class="form-label">
          <span id="otaStatusText">Uploading firmware...</span>
          <span class="val" id="otaPctText">0%</span>
        </div>
        <div class="progress-bar-bg">
          <div class="progress-bar-fill" id="otaProgressBar"></div>
        </div>
      </div>

      <button class="btn btn-primary" style="margin-top:16px;" id="btnStartOta" onclick="uploadOtaFirmware()" disabled>
        &#9889; Flash Firmware &amp; Reboot
      </button>
    </div>
  </div>
</div>

<div id="toast">Saved!</div>

<script>
// ============================================================================
// 1. Pure Client-Side MD5 Implementation for ExpressLRS Binding Phrase Hashing
//    Matches: MD5('-DMY_BINDING_PHRASE="' + phrase + '"')[0..5]
// ============================================================================
function md5Bytes(str) {
  function utf8Bytes(s) {
    var out = [];
    for (var i = 0; i < s.length; i++) {
      var c = s.charCodeAt(i);
      if (c < 0x80) out.push(c);
      else if (c < 0x800) { out.push(0xc0 | (c >> 6), 0x80 | (c & 0x3f)); }
      else { out.push(0xe0 | (c >> 12), 0x80 | ((c >> 6) & 0x3f), 0x80 | (c & 0x3f)); }
    }
    return out;
  }
  function lrot(x, c) { return (x << c) | (x >>> (32 - c)); }
  var bytes = utf8Bytes(str);
  var origBitLen = bytes.length * 8;
  bytes.push(0x80);
  while ((bytes.length % 64) !== 56) bytes.push(0);
  for (var i = 0; i < 4; i++) bytes.push((origBitLen >>> (i * 8)) & 0xff);
  for (var i = 0; i < 4; i++) bytes.push(0);

  var K = [], S = [
    7,12,17,22,7,12,17,22,7,12,17,22,7,12,17,22,
    5,9,14,20,5,9,14,20,5,9,14,20,5,9,14,20,
    4,11,16,23,4,11,16,23,4,11,16,23,4,11,16,23,
    6,10,15,21,6,10,15,21,6,10,15,21,6,10,15,21
  ];
  for (var i = 0; i < 64; i++) K[i] = Math.floor(Math.abs(Math.sin(i + 1)) * 4294967296) >>> 0;

  var a0 = 0x67452301, b0 = 0xefcdab89, c0 = 0x98badcfe, d0 = 0x10325476;
  for (var off = 0; off < bytes.length; off += 64) {
    var M = [];
    for (var j = 0; j < 16; j++) {
      var idx = off + j * 4;
      M[j] = (bytes[idx]) | (bytes[idx+1] << 8) | (bytes[idx+2] << 16) | (bytes[idx+3] << 24);
    }
    var A = a0, B = b0, C = c0, D = d0;
    for (var i = 0; i < 64; i++) {
      var F, g;
      if (i < 16) { F = (B & C) | ((~B) & D); g = i; }
      else if (i < 32) { F = (D & B) | ((~D) & C); g = (5 * i + 1) % 16; }
      else if (i < 48) { F = B ^ C ^ D; g = (3 * i + 5) % 16; }
      else { F = C ^ (B | (~D)); g = (7 * i) % 16; }
      F = (F + A + K[i] + M[g]) >>> 0;
      A = D; D = C; C = B;
      B = (B + lrot(F, S[i])) >>> 0;
    }
    a0 = (a0 + A) >>> 0; b0 = (b0 + B) >>> 0;
    c0 = (c0 + C) >>> 0; d0 = (d0 + D) >>> 0;
  }
  var words = [a0, b0, c0, d0], digest = [];
  for (var w = 0; w < 4; w++) {
    for (var b = 0; b < 4; b++) digest.push((words[w] >>> (b * 8)) & 0xff);
  }
  return digest;
}

function computeElrsUidFromPhrase(phrase) {
  var input = phrase.indexOf('-DMY_BINDING_PHRASE=') === 0
    ? phrase
    : '-DMY_BINDING_PHRASE="' + phrase + '"';
  var d = md5Bytes(input).slice(0, 6);
  d[0] = d[0] & (~0x01); // Unicast bit fix
  return d;
}

// ============================================================================
// 2. UI State & Configuration Helpers
// ============================================================================
var liveTarget = { pan: 0, tilt: 0, roll: 0, cPan: 992, cTilt: 992, cRoll: 992, chPan: 0, chTilt: 1, chRoll: 2 };
var animState = { pan: 0, tilt: 0, roll: 0 };
var graphHistory = [];
var MAX_GRAPH_POINTS = 180; // ~3 seconds at 60 FPS
var ws = null;
var wsConnected = false;
var livePushTimer = null;

function showToast(msg, color) {
  var t = document.getElementById('toast');
  t.textContent = msg;
  t.style.borderColor = color || 'var(--cyan)';
  t.classList.add('show');
  setTimeout(function() { t.classList.remove('show'); }, 2200);
}

function switchTab(name) {
  var btns = document.querySelectorAll('.tab-btn');
  btns[0].classList.toggle('active', name === 'dashboard');
  btns[1].classList.toggle('active', name === 'ota');
  document.getElementById('tab-dashboard').classList.toggle('active', name === 'dashboard');
  document.getElementById('tab-ota').classList.toggle('active', name === 'ota');
}

function populateChannelSelects() {
  var ids = ['selChPan', 'selChTilt', 'selChRoll'];
  for (var k = 0; k < ids.length; k++) {
    var sel = document.getElementById(ids[k]);
    sel.innerHTML = '';
    for (var c = 0; c < 16; c++) {
      var opt = document.createElement('option');
      opt.value = c;
      opt.textContent = 'TR' + (c + 1) + ' (CH' + (c + 1) + ')';
      sel.appendChild(opt);
    }
  }
  document.getElementById('selChPan').value = 0;
  document.getElementById('selChTilt').value = 1;
  document.getElementById('selChRoll').value = 2;
}

function getUidArrayFromInputs() {
  var arr = [];
  for (var i = 0; i < 6; i++) {
    var v = parseInt(document.getElementById('uid' + i).value, 10);
    if (isNaN(v) || v < 0) v = 0;
    if (v > 255) v = 255;
    arr.push(v);
  }
  arr[0] = arr[0] & (~0x01); // Enforce IEEE 802 unicast bit
  return arr;
}

function setUidInputs(arr) {
  for (var i = 0; i < 6; i++) {
    document.getElementById('uid' + i).value = (arr[i] !== undefined ? arr[i] : 0);
  }
  updateUidPreviewDisplay();
}

function updateUidPreviewDisplay() {
  var arr = getUidArrayFromInputs();
  var hex = arr.map(function(b) { return ('0' + b.toString(16).toUpperCase()).slice(-2); }).join(':');
  document.getElementById('uidHexPreview').textContent = hex;
  document.getElementById('uidArrPreview').textContent = '{' + arr.join(', ') + '}';
}

function onPhraseInput() {
  var phrase = document.getElementById('inpPhrase').value;
  var manual = document.getElementById('chkManualUid').checked;
  if (phrase.length > 0 && !manual) {
    var uid = computeElrsUidFromPhrase(phrase);
    setUidInputs(uid);
  }
}

function onToggleManualUid() {
  var manual = document.getElementById('chkManualUid').checked;
  for (var i = 0; i < 6; i++) {
    document.getElementById('uid' + i).readOnly = !manual && (document.getElementById('inpPhrase').value.length > 0);
  }
  if (!manual && document.getElementById('inpPhrase').value.length > 0) {
    onPhraseInput();
  }
}

function onManualUidChange() {
  document.getElementById('chkManualUid').checked = true;
  updateUidPreviewDisplay();
}

function updateAngleLabels() {
  document.getElementById('lblMaxPan').innerHTML  = '&plusmn;' + document.getElementById('rngMaxPan').value + '&deg;';
  document.getElementById('lblMaxTilt').innerHTML = '&plusmn;' + document.getElementById('rngMaxTilt').value + '&deg;';
  document.getElementById('lblMaxRoll').innerHTML = '&plusmn;' + document.getElementById('rngMaxRoll').value + '&deg;';
}

// ============================================================================
// 3. REST API & WebSocket Telemetry Client
// ============================================================================
function loadInitialConfig() {
  fetch('/api/config')
    .then(function(r) { return r.json(); })
    .then(function(cfg) {
      document.getElementById('inpPhrase').value = cfg.phrase || '';
      if (cfg.uid && cfg.uid.length === 6) {
        setUidInputs(cfg.uid);
      }
      document.getElementById('selFusion').value = cfg.fusionMode || 0;
      document.getElementById('rngMaxPan').value = cfg.maxPan || 90;
      document.getElementById('rngMaxTilt').value = cfg.maxTilt || 60;
      document.getElementById('rngMaxRoll').value = cfg.maxRoll || 45;
      updateAngleLabels();

      document.getElementById('chkInvPan').checked = !!cfg.invPan;
      document.getElementById('chkInvTilt').checked = !!cfg.invTilt;
      document.getElementById('chkInvRoll').checked = !!cfg.invRoll;
      document.getElementById('chkSwapPR').checked = !!cfg.swapPitchRoll;

      document.getElementById('selChPan').value = (cfg.chPan !== undefined) ? cfg.chPan : 0;
      document.getElementById('selChTilt').value = (cfg.chTilt !== undefined) ? cfg.chTilt : 1;
      document.getElementById('selChRoll').value = (cfg.chRoll !== undefined) ? cfg.chRoll : 2;
      document.getElementById('selTxPower').value = cfg.txPower || 40;

      if (cfg.sensor) {
        document.getElementById('pillSensor').textContent = 'IMU: ' + cfg.sensor;
        document.getElementById('hudMode').textContent = cfg.sensor;
      }
      if (cfg.board) {
        document.getElementById('boardBadge').textContent = cfg.board;
      }
      updateChannelLabels();
    })
    .catch(function() {});
}

function updateChannelLabels() {
  var cp = parseInt(document.getElementById('selChPan').value, 10) + 1;
  var ct = parseInt(document.getElementById('selChTilt').value, 10) + 1;
  var cr = parseInt(document.getElementById('selChRoll').value, 10) + 1;
  document.getElementById('lblTrPan').textContent  = 'PAN (TR' + cp + ')';
  document.getElementById('lblTrTilt').textContent = 'TILT (TR' + ct + ')';
  document.getElementById('lblTrRoll').textContent = 'ROLL (TR' + cr + ')';
}

function pushLiveSettings() {
  updateChannelLabels();
  if (livePushTimer) clearTimeout(livePushTimer);
  livePushTimer = setTimeout(function() {
    var payload = gatherSettingsPayload(false);
    fetch('/api/live', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    }).catch(function() {});
  }, 90);
}

function gatherSettingsPayload(rebootFlag) {
  var uid = getUidArrayFromInputs();
  return {
    phrase: document.getElementById('inpPhrase').value.trim(),
    manualUid: document.getElementById('chkManualUid').checked,
    uid: uid,
    fusionMode: parseInt(document.getElementById('selFusion').value, 10),
    maxPan: parseFloat(document.getElementById('rngMaxPan').value),
    maxTilt: parseFloat(document.getElementById('rngMaxTilt').value),
    maxRoll: parseFloat(document.getElementById('rngMaxRoll').value),
    invPan: document.getElementById('chkInvPan').checked,
    invTilt: document.getElementById('chkInvTilt').checked,
    invRoll: document.getElementById('chkInvRoll').checked,
    swapPitchRoll: document.getElementById('chkSwapPR').checked,
    chPan: parseInt(document.getElementById('selChPan').value, 10),
    chTilt: parseInt(document.getElementById('selChTilt').value, 10),
    chRoll: parseInt(document.getElementById('selChRoll').value, 10),
    txPower: parseInt(document.getElementById('selTxPower').value, 10),
    reboot: !!rebootFlag
  };
}

function saveConfig(reboot) {
  var payload = gatherSettingsPayload(reboot);
  fetch('/api/save', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(payload)
  })
  .then(function(r) { return r.json(); })
  .then(function(res) {
    if (res.uid) setUidInputs(res.uid);
    if (reboot) {
      showToast('Settings saved! Rebooting into 50Hz Normal Mode...', 'var(--green)');
      document.getElementById('pillWs').textContent = 'Rebooting to Normal Mode...';
    } else {
      showToast('Settings saved to ESP32 NVS!', 'var(--cyan)');
    }
  })
  .catch(function() {
    showToast('Failed to save settings', 'var(--red)');
  });
}

function sendCenter() {
  if (ws && wsConnected) {
    ws.send('{"cmd":"center"}');
  } else {
    fetch('/api/center', { method: 'POST' }).catch(function() {});
  }
  showToast('HeadTracker Centered / Zeroed (Cyan LED)', 'var(--cyan)');
}

function sendTare() {
  if (ws && wsConnected) {
    ws.send('{"cmd":"tare"}');
  } else {
    fetch('/api/tare', { method: 'POST' }).catch(function() {});
  }
  showToast('IMU Tare / Calibration Initiated (Yellow LED)', 'var(--yellow)');
}

function handleTelemetryPacket(d) {
  if (d.pan !== undefined)  liveTarget.pan  = d.pan;
  if (d.tilt !== undefined) liveTarget.tilt = d.tilt;
  if (d.roll !== undefined) liveTarget.roll = d.roll;
  if (d.cPan !== undefined)  liveTarget.cPan  = d.cPan;
  if (d.cTilt !== undefined) liveTarget.cTilt = d.cTilt;
  if (d.cRoll !== undefined) liveTarget.cRoll = d.cRoll;

  var fmt = function(v) { return (v >= 0 ? '+' : '') + v.toFixed(1) + '\u00B0'; };
  document.getElementById('hudPan').textContent  = 'PAN: ' + fmt(liveTarget.pan);
  document.getElementById('hudTilt').textContent = 'TILT: ' + fmt(liveTarget.tilt);
  document.getElementById('hudRoll').textContent = 'ROLL: ' + fmt(liveTarget.roll);

  if (d.sensor) {
    document.getElementById('pillSensor').textContent = 'IMU: ' + d.sensor;
    document.getElementById('pillSensor').className = 'pill ok';
    document.getElementById('hudMode').textContent = d.sensor;
  }

  updateCrsfBar('barPan',  'valCrsfPan',  liveTarget.cPan);
  updateCrsfBar('barTilt', 'valCrsfTilt', liveTarget.cTilt);
  updateCrsfBar('barRoll', 'valCrsfRoll', liveTarget.cRoll);
}

function updateCrsfBar(barId, valId, crsf) {
  var us = Math.round(988 + (crsf - 172) * (1024 / (1811 - 172)));
  document.getElementById(valId).textContent = crsf + ' (' + us + 'us)';
  var norm = (crsf - 992) / (1811 - 992);
  if (norm > 1) norm = 1;
  if (norm < -1) norm = -1;
  var el = document.getElementById(barId);
  if (norm >= 0) {
    el.style.left = '50%';
    el.style.width = (norm * 50).toFixed(1) + '%';
  } else {
    var w = (-norm * 50);
    el.style.left = (50 - w).toFixed(1) + '%';
    el.style.width = w.toFixed(1) + '%';
  }
}

function connectWebSocket() {
  var host = window.location.host || '192.168.4.1';
  var url = 'ws://' + host + '/ws';
  try {
    ws = new WebSocket(url);
    ws.onopen = function() {
      wsConnected = true;
      var p = document.getElementById('pillWs');
      p.textContent = 'WS: Live 30Hz';
      p.className = 'pill ok';
    };
    ws.onmessage = function(evt) {
      try {
        var d = JSON.parse(evt.data);
        handleTelemetryPacket(d);
      } catch (e) {}
    };
    ws.onclose = function() {
      wsConnected = false;
      var p = document.getElementById('pillWs');
      p.textContent = 'WS: Reconnecting...';
      p.className = 'pill warn';
      setTimeout(connectWebSocket, 1500);
    };
    ws.onerror = function() {
      ws.close();
    };
  } catch (e) {
    setTimeout(connectWebSocket, 2000);
  }
}

// Fallback HTTP polling if WebSocket is not yet connected
setInterval(function() {
  if (!wsConnected) {
    fetch('/api/telemetry')
      .then(function(r) { return r.json(); })
      .then(function(d) { handleTelemetryPacket(d); })
      .catch(function() {});
  }
}, 200);

// ============================================================================
// 4. 3D Low-Poly Pilot Head & FPV Goggles Renderer (60 FPS Pure HTML5 Canvas)
// ============================================================================
var modelFaces = [];
function buildPilotHeadAndGogglesMesh() {
  modelFaces = [];
  function addQuad(p1, p2, p3, p4, color, emissive) {
    modelFaces.push({ verts: [p1, p2, p3, p4], color: color, emissive: !!emissive });
  }
  function addBox(cx, cy, cz, rx, ry, rz, colTop, colSide, colFront, emissive) {
    var x0=cx-rx, x1=cx+rx, y0=cy-ry, y1=cy+ry, z0=cz-rz, z1=cz+rz;
    var v000=[x0,y0,z0], v100=[x1,y0,z0], v110=[x1,y1,z0], v010=[x0,y1,z0];
    var v001=[x0,y0,z1], v101=[x1,y0,z1], v111=[x1,y1,z1], v011=[x0,y1,z1];
    addQuad(v010, v110, v111, v011, colTop, emissive);   // Top (+Y)
    addQuad(v001, v101, v100, v000, colSide, emissive);  // Bottom (-Y)
    addQuad(v001, v011, v111, v101, colFront, emissive); // Front (+Z)
    addQuad(v100, v110, v010, v000, colSide, emissive);  // Back (-Z)
    addQuad(v101, v111, v110, v100, colSide, emissive);  // Right (+X)
    addQuad(v000, v010, v011, v001, colSide, emissive);  // Left (-X)
  }

  // 1. Sculpted Low-Poly Pilot Head (tapered cranium + jawline + neck)
  // Neck column & shoulders base
  addBox(0, -1.38, -0.12, 0.78, 0.14, 0.46, '#1e293b', '#151d2a', '#1a2436', false);
  addBox(0, -1.10, -0.10, 0.42, 0.32, 0.42, '#28354e', '#1f293d', '#243048', false);
  // Main Head Cranium
  addBox(0, 0.05, -0.05, 0.78, 0.86, 0.82, '#3b4d71', '#2c3a57', '#344566', false);
  // Upper Crown bevel
  addBox(0, 0.96, -0.05, 0.62, 0.14, 0.66, '#465b85', '#344566', '#3b4d71', false);
  // Lower Chin / Jaw taper (front of head)
  addBox(0, -0.76, 0.14, 0.54, 0.18, 0.60, '#2c3a57', '#25314a', '#2f3e5c', false);
  // Left & Right Ears
  addBox(-0.82, -0.02, -0.05, 0.06, 0.22, 0.14, '#344566', '#28354e', '#2c3a57', false);
  addBox( 0.82, -0.02, -0.05, 0.06, 0.22, 0.14, '#344566', '#28354e', '#2c3a57', false);

  // 2. Elastic Goggle Headband Strap (horizontal wrap + overhead crown strap + rear battery/pad)
  addBox(0, 0.20, -0.04, 0.84, 0.20, 0.86, '#141923', '#10141d', '#181f2c', false);
  // Overhead crown strap running front-to-back over top of head
  addBox(0, 1.05, 0.08, 0.16, 0.06, 0.82, '#181f2c', '#10141d', '#141923', false);
  // Rear Goggle Headstrap Cushion & Battery Pack on back of head (facing viewer in rear view)
  addBox(0, 0.20, -0.92, 0.46, 0.26, 0.08, '#1e2638', '#151b29', '#232d42', false);
  // Rear Battery Pack LED indicator strip (subtle cyan/green power bar on back of strap)
  addBox(0, 0.20, -1.01, 0.24, 0.05, 0.02, '#00f0ff', '#00f0ff', '#00f0ff', true);

  // 3. FPV Goggles Visor Housing (mounted on front face of pilot head, facing away into -Z)
  addBox(0, 0.20, 0.94, 0.94, 0.42, 0.28, '#1e2638', '#151b29', '#0f1420', false);
  // Goggles Front Carbon Bezel / Visor
  addBox(0, 0.20, 1.24, 0.86, 0.34, 0.06, '#26324a', '#1b2436', '#111722', false);

  // 4. Glowing Cyan / Magenta FPV Goggle LED Lightbar & Optics Accents (on front visor)
  addBox(-0.42, 0.20, 1.31, 0.26, 0.12, 0.02, '#00f0ff', '#00f0ff', '#00f0ff', true);
  addBox( 0.42, 0.20, 1.31, 0.26, 0.12, 0.02, '#00f0ff', '#00f0ff', '#00f0ff', true);
  addBox( 0.00, 0.20, 1.31, 0.08, 0.22, 0.02, '#d946ef', '#d946ef', '#d946ef', true);

  // 5. HeadTracker Module Case mounted on right side of Goggles Strap (Magenta AP LED)
  addBox(-0.88, 0.24, -0.15, 0.09, 0.25, 0.34, '#25324c', '#1c263b', '#2d3d5c', false);
  addBox(-0.98, 0.24, -0.22, 0.02, 0.06, 0.06, '#d946ef', '#d946ef', '#d946ef', true);

  // 6. Dual 5.8GHz FPV Goggle Antennas (visible rising above the goggles from behind)
  // Left antenna mast + red pagoda top
  addBox( 0.58, 0.84, 0.95, 0.04, 0.28, 0.04, '#64748b', '#475569', '#64748b', false);
  addBox( 0.58, 1.15, 0.95, 0.15, 0.07, 0.15, '#ef4444', '#dc2626', '#ef4444', false);
  // Right directional patch antenna
  addBox(-0.55, 0.72, 1.02, 0.20, 0.18, 0.05, '#334155', '#1e293b', '#00f0ff', false);
}

function rotatePointEuler(pt, panDeg, tiltDeg, rollDeg) {
  // Keep the rear-facing visual head orientation ([-x3, y3, -z3] at rest)
  // Note: 180-deg Y-rotation commutes with Y (yaw) and inverts X (pitch) and Z (roll)
  var yaw   = -panDeg  * Math.PI / 180.0;
  var pitch =  tiltDeg * Math.PI / 180.0;
  var roll  =  rollDeg * Math.PI / 180.0;

  var x = pt[0], y = pt[1], z = pt[2];

  // 1. Roll around Z (head forward axis)
  var cr = Math.cos(roll), sr = Math.sin(roll);
  var x1 = x * cr - y * sr;
  var y1 = x * sr + y * cr;
  var z1 = z;

  // 2. Pitch (Tilt) around X (head right axis)
  var cp = Math.cos(pitch), sp = Math.sin(pitch);
  var x2 = x1;
  var y2 = y1 * cp - z1 * sp;
  var z2 = y1 * sp + z1 * cp;

  // 3. Yaw (Pan) around Y (world vertical up axis)
  var cy = Math.cos(yaw), sy = Math.sin(yaw);
  var x3 =  x2 * cy + z2 * sy;
  var y3 =  y2;
  var z3 = -x2 * sy + z2 * cy;

  // 4. Rotate 180 deg around Y so the pilot faces away from the camera (-Z)
  return [-x3, y3, -z3];
}

function shadeHexColor(hex, factor) {
  var r = parseInt(hex.slice(1,3), 16);
  var g = parseInt(hex.slice(3,5), 16);
  var b = parseInt(hex.slice(5,7), 16);
  r = Math.min(255, Math.max(0, Math.round(r * factor)));
  g = Math.min(255, Math.max(0, Math.round(g * factor)));
  b = Math.min(255, Math.max(0, Math.round(b * factor)));
  return 'rgb(' + r + ',' + g + ',' + b + ')';
}

function render3DScene(ctx, w, h, pan, tilt, roll) {
  ctx.clearRect(0, 0, w, h);
  var cx = w * 0.5;
  var cy = h * 0.56;
  var fov = Math.min(w, h) * 0.92;
  var camDist = 4.6;

  // Camera elevated 18 deg above horizon for a clear over-the-shoulder rear 3D view
  var camPitch = 18 * Math.PI / 180.0;
  var ccp = Math.cos(camPitch), csp = Math.sin(camPitch);

  function project(pt) {
    var x = pt[0];
    var y = pt[1] * ccp - pt[2] * csp;
    var z = pt[1] * csp + pt[2] * ccp;
    var depth = camDist - z;
    if (depth < 0.5) depth = 0.5;
    var scale = fov / depth;
    return [cx + x * scale, cy - y * scale, depth];
  }

  // Draw Reference Compass / Horizon Ring around the pilot head
  ctx.save();
  ctx.beginPath();
  for (var a = 0; a <= 360; a += 10) {
    var rad = a * Math.PI / 180.0;
    var p = project([Math.sin(rad) * 1.75, -0.95, Math.cos(rad) * 1.75]);
    if (a === 0) ctx.moveTo(p[0], p[1]);
    else ctx.lineTo(p[0], p[1]);
  }
  ctx.strokeStyle = 'rgba(0, 240, 255, 0.18)';
  ctx.lineWidth = 1.5;
  ctx.stroke();
  ctx.restore();

  // Draw Forward Sight Vector Beam from front of FPV Goggles (drawn before head when pointing away into -Z)
  var pStart = project(rotatePointEuler([0, 0.20, 1.30], pan, tilt, roll));
  var pEnd   = project(rotatePointEuler([0, 0.20, 2.55], pan, tilt, roll));
  ctx.save();
  ctx.beginPath();
  ctx.moveTo(pStart[0], pStart[1]);
  ctx.lineTo(pEnd[0], pEnd[1]);
  ctx.strokeStyle = 'rgba(0, 240, 255, 0.85)';
  ctx.lineWidth = 2.6;
  ctx.stroke();
  ctx.beginPath();
  ctx.arc(pEnd[0], pEnd[1], 4.2, 0, Math.PI * 2);
  ctx.fillStyle = '#00f0ff';
  ctx.fill();
  ctx.restore();

  // Transform & depth-sort all mesh faces
  var drawList = [];
  var light = [0.38, 0.65, 0.65];
  for (var i = 0; i < modelFaces.length; i++) {
    var f = modelFaces[i];
    var rv = [];
    var pv = [];
    var avgZ = 0;
    for (var j = 0; j < 4; j++) {
      var r = rotatePointEuler(f.verts[j], pan, tilt, roll);
      var p = project(r);
      rv.push(r);
      pv.push(p);
      avgZ += p[2];
    }
    avgZ /= 4.0;

    // Face normal for lighting
    var ux = rv[1][0] - rv[0][0], uy = rv[1][1] - rv[0][1], uz = rv[1][2] - rv[0][2];
    var vx = rv[2][0] - rv[0][0], vy = rv[2][1] - rv[0][1], vz = rv[2][2] - rv[0][2];
    var nx = uy * vz - uz * vy;
    var ny = uz * vx - ux * vz;
    var nz = ux * vy - uy * vx;
    var nLen = Math.sqrt(nx*nx + ny*ny + nz*nz) || 1;
    nx /= nLen; ny /= nLen; nz /= nLen;

    var dot = nx * light[0] + ny * light[1] + nz * light[2];
    var shade = f.emissive ? 1.15 : (0.55 + 0.55 * Math.max(0, dot));
    drawList.push({ pv: pv, z: avgZ, fill: shadeHexColor(f.color, shade), emissive: f.emissive });
  }

  // Painter's algorithm (back to front)
  drawList.sort(function(a, b) { return b.z - a.z; });

  for (var i = 0; i < drawList.length; i++) {
    var item = drawList[i];
    ctx.beginPath();
    ctx.moveTo(item.pv[0][0], item.pv[0][1]);
    ctx.lineTo(item.pv[1][0], item.pv[1][1]);
    ctx.lineTo(item.pv[2][0], item.pv[2][1]);
    ctx.lineTo(item.pv[3][0], item.pv[3][1]);
    ctx.closePath();
    ctx.fillStyle = item.fill;
    ctx.fill();
    ctx.strokeStyle = item.emissive ? item.fill : 'rgba(10, 15, 28, 0.55)';
    ctx.lineWidth = item.emissive ? 1.2 : 0.8;
    ctx.stroke();
  }
}

// ============================================================================
// 5. Real-Time 3-Axis Rolling Strip Chart Renderer (~3 Seconds History)
// ============================================================================
function renderRollingGraph(ctx, w, h) {
  ctx.clearRect(0, 0, w, h);

  // Horizontal grid lines (-90, -45, 0, +45, +90 deg)
  var levels = [-60, -30, 0, 30, 60];
  ctx.font = '10px SFMono-Regular, Consolas, monospace';
  for (var i = 0; i < levels.length; i++) {
    var deg = levels[i];
    var y = h * 0.5 - (deg / 90.0) * (h * 0.44);
    ctx.beginPath();
    ctx.moveTo(0, y);
    ctx.lineTo(w, y);
    ctx.strokeStyle = (deg === 0) ? 'rgba(255,255,255,0.22)' : 'rgba(255,255,255,0.06)';
    ctx.lineWidth = (deg === 0) ? 1.2 : 1.0;
    ctx.stroke();
    ctx.fillStyle = 'rgba(139, 155, 184, 0.55)';
    ctx.fillText((deg > 0 ? '+' : '') + deg + '\u00B0', 6, y - 3);
  }

  if (graphHistory.length < 2) return;

  function drawTrace(key, color) {
    ctx.beginPath();
    for (var i = 0; i < graphHistory.length; i++) {
      var x = (i / (MAX_GRAPH_POINTS - 1)) * w;
      var val = Math.max(-90, Math.min(90, graphHistory[i][key]));
      var y = h * 0.5 - (val / 90.0) * (h * 0.44);
      if (i === 0) ctx.moveTo(x, y);
      else ctx.lineTo(x, y);
    }
    ctx.strokeStyle = color;
    ctx.lineWidth = 2.0;
    ctx.stroke();
  }

  drawTrace('pan',  '#00f0ff'); // Cyan
  drawTrace('tilt', '#ff8a00'); // Orange
  drawTrace('roll', '#d946ef'); // Magenta
}

// ============================================================================
// 6. 60 FPS Animation Loop
// ============================================================================
var c3d = document.getElementById('canvas3d');
var ctx3d = c3d.getContext('2d');
var cGraph = document.getElementById('canvasGraph');
var ctxGraph = cGraph.getContext('2d');

function resizeCanvases() {
  var dpr = window.devicePixelRatio || 1;
  var r3 = c3d.parentElement.getBoundingClientRect();
  c3d.width = r3.width * dpr;
  c3d.height = r3.height * dpr;
  ctx3d.setTransform(dpr, 0, 0, dpr, 0, 0);

  var rg = cGraph.parentElement.getBoundingClientRect();
  cGraph.width = rg.width * dpr;
  cGraph.height = rg.height * dpr;
  ctxGraph.setTransform(dpr, 0, 0, dpr, 0, 0);
}
window.addEventListener('resize', resizeCanvases);

function animateLoop() {
  // Smooth 60 FPS interpolation toward 30Hz telemetry target
  var alpha = 0.28;
  animState.pan  += (liveTarget.pan  - animState.pan)  * alpha;
  animState.tilt += (liveTarget.tilt - animState.tilt) * alpha;
  animState.roll += (liveTarget.roll - animState.roll) * alpha;

  graphHistory.push({ pan: animState.pan, tilt: animState.tilt, roll: animState.roll });
  if (graphHistory.length > MAX_GRAPH_POINTS) graphHistory.shift();

  var r3 = c3d.parentElement.getBoundingClientRect();
  if (r3.width > 0) {
    render3DScene(ctx3d, r3.width, r3.height, animState.pan, animState.tilt, animState.roll);
  }
  var rg = cGraph.parentElement.getBoundingClientRect();
  if (rg.width > 0) {
    renderRollingGraph(ctxGraph, rg.width, rg.height);
  }

  requestAnimationFrame(animateLoop);
}

// ============================================================================
// 7. Wireless OTA Firmware Upload Handler
// ============================================================================
var selectedOtaFile = null;
var dropzone = document.getElementById('otaDropzone');
dropzone.addEventListener('dragover', function(e) { e.preventDefault(); dropzone.classList.add('dragover'); });
dropzone.addEventListener('dragleave', function() { dropzone.classList.remove('dragover'); });
dropzone.addEventListener('drop', function(e) {
  e.preventDefault();
  dropzone.classList.remove('dragover');
  if (e.dataTransfer.files && e.dataTransfer.files.length > 0) {
    setOtaFile(e.dataTransfer.files[0]);
  }
});

function onOtaFileSelected() {
  var f = document.getElementById('otaFile').files[0];
  if (f) setOtaFile(f);
}

function setOtaFile(file) {
  selectedOtaFile = file;
  document.getElementById('otaFileName').innerHTML = 'Selected: <span style="color:var(--cyan)">' + file.name + '</span>';
  document.getElementById('otaFileSize').textContent = (file.size / 1024).toFixed(1) + ' KB ready for Wireless OTA';
  document.getElementById('btnStartOta').disabled = false;
}

function uploadOtaFirmware() {
  if (!selectedOtaFile) return;
  var btn = document.getElementById('btnStartOta');
  btn.disabled = true;
  document.getElementById('otaProgressWrap').style.display = 'block';

  var formData = new FormData();
  formData.append('update', selectedOtaFile, selectedOtaFile.name);

  var xhr = new XMLHttpRequest();
  xhr.open('POST', '/update', true);
  xhr.upload.onprogress = function(e) {
    if (e.lengthComputable) {
      var pct = Math.round((e.loaded / e.total) * 100);
      document.getElementById('otaProgressBar').style.width = pct + '%';
      document.getElementById('otaPctText').textContent = pct + '%';
      document.getElementById('otaStatusText').textContent =
        'Uploading: ' + (e.loaded / 1024).toFixed(0) + ' / ' + (e.total / 1024).toFixed(0) + ' KB';
    }
  };
  xhr.onload = function() {
    if (xhr.status === 200) {
      document.getElementById('otaProgressBar').style.width = '100%';
      document.getElementById('otaPctText').textContent = '100%';
      document.getElementById('otaStatusText').textContent = 'OTA Complete! Rebooting HeadTracker...';
      showToast('OTA Flash Successful! Rebooting...', 'var(--green)');
    } else {
      document.getElementById('otaStatusText').textContent = 'OTA Failed: ' + xhr.responseText;
      showToast('OTA Update Failed', 'var(--red)');
      btn.disabled = false;
    }
  };
  xhr.onerror = function() {
    document.getElementById('otaStatusText').textContent = 'Connection error during OTA upload.';
    showToast('OTA Upload Error', 'var(--red)');
    btn.disabled = false;
  };
  xhr.send(formData);
}

// Initialize Everything on DOM Load
window.addEventListener('DOMContentLoaded', function() {
  populateChannelSelects();
  buildPilotHeadAndGogglesMesh();
  resizeCanvases();
  loadInitialConfig();
  connectWebSocket();
  requestAnimationFrame(animateLoop);
});
</script>
</body>
</html>
"""

def resolve_project_dir():
    try:
        from SCons.Script import DefaultEnvironment
        env = DefaultEnvironment()
        return env.subst("$PROJECT_DIR")
    except Exception:
        pass
    if "__file__" in globals():
        return os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    return os.getcwd()


def main():
    raw_bytes = HTML_SOURCE.strip().encode("utf-8")
    # Use mtime=0 for 100% deterministic, reproducible gzip output
    gz_bytes = gzip.compress(raw_bytes, compresslevel=9, mtime=0)
    project_dir = resolve_project_dir()
    out_path = os.path.join(project_dir, "src", "web_assets.h")

    lines = [
        "#pragma once",
        "#include <Arduino.h>",
        "#include <pgmspace.h>",
        "",
        "// Auto-generated by scripts/generate_web_assets.py — 100% Offline Single-File Gzipped Web-IF Frontend",
        f"// Uncompressed size: {len(raw_bytes)} bytes | Gzipped PROGMEM size: {len(gz_bytes)} bytes",
        f"const size_t index_html_gz_len = {len(gz_bytes)};",
        "const uint8_t index_html_gz[] PROGMEM = {",
    ]

    chunk_size = 16
    for i in range(0, len(gz_bytes), chunk_size):
        chunk = gz_bytes[i : i + chunk_size]
        hex_str = ", ".join(f"0x{b:02X}" for b in chunk)
        lines.append(f"    {hex_str},")

    lines.append("};")
    lines.append("")
    new_content = "\n".join(lines)

    existing_content = None
    if os.path.exists(out_path):
        with open(out_path, "r", encoding="utf-8") as f:
            existing_content = f.read()

    if existing_content != new_content or __name__ == "__main__":
        with open(out_path, "w", encoding="utf-8") as f:
            f.write(new_content)
        print(f"[web_assets] Generated {out_path}: {len(raw_bytes)} bytes -> {len(gz_bytes)} bytes gzipped.")
    else:
        print(f"[web_assets] Up-to-date ({len(gz_bytes)} bytes gzipped).")


main()

