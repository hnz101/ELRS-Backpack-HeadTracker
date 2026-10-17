#pragma once
#include <Arduino.h>
#include "my_config.h"

enum SerialDisplayMode
{
    SERIAL_MODE_COMPACT = 0,   // Single scrolling status line with mini-bars
    SERIAL_MODE_DASHBOARD = 1  // Multi-line visual ASCII dashboard
};

class SerialDisplay
{
public:
    SerialDisplay();

    void begin();

    // Periodic render (called inside loop)
    void render(float pan, float tilt, float roll,
                uint16_t crsfPan, uint16_t crsfTilt, uint16_t crsfRoll,
                uint32_t txCount, bool txOk,
                uint32_t rxCount, uint32_t lastRxTime,
                const char* sensorName,
                bool waveMode,
                uint8_t radioMode,
                int8_t txPower = WIFI_TX_POWER_DEFAULT);

    // Toggle between compact line and dashboard mode
    void toggleMode();

    // Toggle display output on / off
    void toggleEnabled();
    bool isEnabled() const { return _enabled; }

    // Print help menu of interactive commands
    void printHelp();

private:
    uint32_t _lastRenderTime;
    SerialDisplayMode _mode;
    bool _enabled;

    // Generates an ASCII bar meter, e.g. [-------|====---] or [---====|-------]
    void makeBar(char* buf, size_t bufSize, float value, float maxVal, int halfWidth);
};
