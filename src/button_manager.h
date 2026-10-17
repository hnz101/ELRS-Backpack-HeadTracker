#pragma once
#include <Arduino.h>
#include "my_config.h"

enum ButtonEvent
{
    BTN_NONE = 0,
    BTN_CENTER_CLICK,
    BTN_CENTER_LONG,
    BTN_RIGHT_CLICK,
    BTN_LEFT_CLICK
};

class ButtonManager
{
public:
    ButtonManager();

    void begin();
    ButtonEvent update();

    // Set RGB LED color directly
    void setLed(uint8_t r, uint8_t g, uint8_t b);

    // Flash RGB LED for a temporary duration (ms), protecting it from heartbeat overwrite
    void flashLed(uint8_t r, uint8_t g, uint8_t b, uint32_t durationMs = 500);

    // Set heartbeat LED (only updates if no temporary flash is currently active)
    void setHeartbeatLed(uint8_t r, uint8_t g, uint8_t b);

private:
    uint32_t _btnCenterPressStart;
    bool     _btnCenterWasDown;
    bool     _btnCenterLongFired;

    bool     _btnRightWasDown;
    bool     _btnLeftWasDown;

    uint32_t _flashUntil;
};
