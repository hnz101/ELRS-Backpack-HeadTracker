#include "serial_display.h"
#include <math.h>

SerialDisplay::SerialDisplay()
    : _lastRenderTime(0), _mode(SERIAL_MODE_COMPACT), _enabled(true)
{
}

void SerialDisplay::begin()
{
    _lastRenderTime = millis();
    printHelp();
}

void SerialDisplay::toggleMode()
{
    if (_mode == SERIAL_MODE_COMPACT) {
        _mode = SERIAL_MODE_DASHBOARD;
        Serial.println("\n>>> Serial Display Mode: VISUAL DASHBOARD <<<\n");
    } else {
        _mode = SERIAL_MODE_COMPACT;
        Serial.println("\n>>> Serial Display Mode: COMPACT STREAM <<<\n");
    }
}

void SerialDisplay::toggleEnabled()
{
    _enabled = !_enabled;
    if (_enabled) {
        Serial.println("\n>>> Serial Display: ENABLED <<<\n");
    } else {
        Serial.println("\n>>> Serial Display: MUTED (Press 'd' to re-enable) <<<\n");
    }
}

void SerialDisplay::printHelp()
{
    Serial.println();
    Serial.println("==========================================================");
    Serial.println("   ExpressLRS Backpack HeadTracker - Waveshare ESP32-C3   ");
    Serial.println("==========================================================");
    Serial.println(" Interactive Serial Commands:");
    Serial.println("   'c' or SPACE : Center / Zero head position");
    Serial.println("   'r'         : Calibrate / Tare IMU");
    Serial.println("   'f'         : Cycle IMU Fusion (9-DOF Mag <-> AR/VR <-> 6-DOF Game RV)");
    Serial.println("   't'         : Cycle WiFi TX Power (10dBm -> 8.5dBm -> 5dBm -> 2dBm -> 19.5dBm)");
    Serial.println("   'p'         : Toggle Invert Pitch (Up <-> Down)");
    Serial.println("   's'         : Toggle Swap Pitch & Roll (for 90 deg rotated mounting)");
    Serial.println("   'y'         : Toggle Invert Pan / Yaw (Left <-> Right)");
    Serial.println("   'o'         : Toggle Invert Roll");
    Serial.println("   'w'         : Toggle LIVE HeadTracker <-> TEST WAVE");
    Serial.println("   'm'         : Toggle ASCII Display Mode (Compact/Dashboard)");
    Serial.println("   'd'         : Toggle Serial Output Mute / Unmute");
    Serial.println("   '1'..'3'    : Set Radio Mode (1=RX, 2=TX, 3=BOTH)");
    Serial.println("   '?' or 'h'  : Show this help menu");
    Serial.println(" Physical Controls:");
    Serial.println("   BOOT Switch (GPIO 9) : Short Click = Center | Long Press = Calibrate");
    Serial.println("   RGB LED (GPIO 10)    : Green=TX OK | Red=TX Err | Cyan=Zero | Yellow=Cal");
    Serial.println("==========================================================");
    Serial.println();
}

void SerialDisplay::makeBar(char* buf, size_t bufSize, float value, float maxVal, int halfWidth)
{
    if (bufSize < (size_t)(halfWidth * 2 + 4)) return;

    if (maxVal <= 0.001f) maxVal = 1.0f;
    int len = (int)roundf((fabsf(value) / maxVal) * halfWidth);
    if (len > halfWidth) len = halfWidth;

    buf[0] = '[';
    int idx = 1;

    if (value < -0.5f) {
        // Negative deflection: fill on left side of center
        int pad = halfWidth - len;
        for (int i = 0; i < pad; i++) buf[idx++] = '-';
        for (int i = 0; i < len; i++) buf[idx++] = '=';
        buf[idx++] = '|';
        for (int i = 0; i < halfWidth; i++) buf[idx++] = '-';
    } else if (value > 0.5f) {
        // Positive deflection: fill on right side of center
        for (int i = 0; i < halfWidth; i++) buf[idx++] = '-';
        buf[idx++] = '|';
        for (int i = 0; i < len; i++) buf[idx++] = '=';
        int pad = halfWidth - len;
        for (int i = 0; i < pad; i++) buf[idx++] = '-';
    } else {
        // Neutral center
        for (int i = 0; i < halfWidth; i++) buf[idx++] = '-';
        buf[idx++] = '|';
        for (int i = 0; i < halfWidth; i++) buf[idx++] = '-';
    }

    buf[idx++] = ']';
    buf[idx] = '\0';
}

void SerialDisplay::render(float pan, float tilt, float roll,
                           uint16_t crsfPan, uint16_t crsfTilt, uint16_t crsfRoll,
                           uint32_t txCount, bool txOk,
                           uint32_t rxCount, uint32_t lastRxTime,
                           const char* sensorName,
                           bool waveMode,
                           uint8_t radioMode,
                           int8_t txPower)
{
    if (!_enabled) return;

    uint32_t now = millis();
    if (now - _lastRenderTime < SERIAL_DEBUG_INTERVAL_MS) {
        return;
    }
    _lastRenderTime = now;

    // Generate ASCII bar meters
    char barPan[24];
    char barTilt[24];
    char barRoll[24];

    float pwrDbm = (float)txPower * 0.25f;

    if (_mode == SERIAL_MODE_COMPACT) {
        // 5 chars on each side (total width 13)
        makeBar(barPan,  sizeof(barPan),  pan,  HT_MAX_ANGLE_PAN,  5);
        makeBar(barTilt, sizeof(barTilt), tilt, HT_MAX_ANGLE_TILT, 5);
        makeBar(barRoll, sizeof(barRoll), roll, HT_MAX_ANGLE_ROLL, 5);

        Serial.printf("[HT %s|%.1fdBm%s] P:%+5.1f\xC2\xB0%s T:%+5.1f\xC2\xB0%s R:%+5.1f\xC2\xB0%s | CRSF:[%4d,%4d,%4d] | TX:%s#%lu RX:%lu\n",
                      waveMode ? "WAVE" : sensorName,
                      pwrDbm,
                      radioMode == 0 ? " RX" : (radioMode == 1 ? " TX" : ""),
                      pan, barPan,
                      tilt, barTilt,
                      roll, barRoll,
                      crsfPan, crsfTilt, crsfRoll,
                      txOk ? "OK" : "ERR",
                      (unsigned long)txCount,
                      (unsigned long)rxCount);
    } else {
        // Dashboard mode: 8 chars on each side (total width 19)
        makeBar(barPan,  sizeof(barPan),  pan,  HT_MAX_ANGLE_PAN,  8);
        makeBar(barTilt, sizeof(barTilt), tilt, HT_MAX_ANGLE_TILT, 8);
        makeBar(barRoll, sizeof(barRoll), roll, HT_MAX_ANGLE_ROLL, 8);

        Serial.println("--------------------------------------------------------------------------------");
        Serial.printf("  SENSOR: %-14s | PWR: %4.1f dBm | MODE: %-9s | TX: %s (#%lu) | RX: %lu\n",
                      sensorName,
                      pwrDbm,
                      waveMode ? "TEST-WAVE" : "LIVE-HT",
                      txOk ? "OK " : "ERR",
                      (unsigned long)txCount,
                      (unsigned long)rxCount);
        Serial.printf("  PAN  (Yaw)  : %+6.1f deg  [-90] %s [+90]  CRSF: %4d (TR1)\n", pan,  barPan,  crsfPan);
        Serial.printf("  TILT (Pitch): %+6.1f deg  [-60] %s [+60]  CRSF: %4d (TR2)\n", tilt, barTilt, crsfTilt);
        Serial.printf("  ROLL        : %+6.1f deg  [-45] %s [+45]  CRSF: %4d (TR3)\n", roll, barRoll, crsfRoll);
        Serial.println("--------------------------------------------------------------------------------");
    }
}
