#include "button_manager.h"
#include "logging.h"

ButtonManager::ButtonManager()
    : _btnCenterPressStart(0), _btnCenterWasDown(false), _btnCenterLongFired(false),
      _btnRightWasDown(false), _btnLeftWasDown(false), _flashUntil(0)
{
}

void ButtonManager::begin()
{
    // Initialize Buttons (Active LOW with internal pullups)
    if (PIN_BTN_LEFT >= 0) {
        pinMode(PIN_BTN_LEFT, INPUT_PULLUP);
    }
    if (PIN_BTN_CENTER >= 0) {
        pinMode(PIN_BTN_CENTER, INPUT_PULLUP);
    }
    if (PIN_BTN_RIGHT >= 0) {
        pinMode(PIN_BTN_RIGHT, INPUT_PULLUP);
    }

    // Initialize RGB LED Power if configured
    if (PIN_RGB_PWR >= 0) {
        pinMode(PIN_RGB_PWR, OUTPUT);
        digitalWrite(PIN_RGB_PWR, HIGH); // Power ON WS2812
    }

    // Start with dim Cyan / Blue
    setLed(0, 40, 40);
}

void ButtonManager::setLed(uint8_t r, uint8_t g, uint8_t b)
{
#if defined(PIN_RGB_DATA) && (PIN_RGB_DATA >= 0)
    rgbLedWrite(PIN_RGB_DATA, r, g, b);
#elif defined(RGB_BUILTIN)
    rgbLedWrite(RGB_BUILTIN, r, g, b);
#endif
}

void ButtonManager::flashLed(uint8_t r, uint8_t g, uint8_t b, uint32_t durationMs)
{
    setLed(r, g, b);
    _flashUntil = millis() + durationMs;
}

void ButtonManager::setHeartbeatLed(uint8_t r, uint8_t g, uint8_t b)
{
    if (millis() >= _flashUntil) {
        setLed(r, g, b);
    }
}

ButtonEvent ButtonManager::update()
{
    ButtonEvent event = BTN_NONE;
    uint32_t now = millis();

    // 1. Center Button (GPIO 9 on ESP32-C3 / GPIO 47 on S3 Mini Pro)
    // Supports Click (<1.5s) and Long Press (> 1.5s)
    if (PIN_BTN_CENTER >= 0) {
        bool centerDown = (digitalRead(PIN_BTN_CENTER) == LOW);
        if (centerDown) {
            if (!_btnCenterWasDown) {
                _btnCenterWasDown = true;
                _btnCenterPressStart = now;
                _btnCenterLongFired = false;
            } else {
                if (!_btnCenterLongFired && (now - _btnCenterPressStart > 1500)) {
                    _btnCenterLongFired = true;
                    event = BTN_CENTER_LONG;
                }
            }
        } else {
            if (_btnCenterWasDown) {
                _btnCenterWasDown = false;
                if (!_btnCenterLongFired && (now - _btnCenterPressStart > 40)) { // Debounced click
                    event = BTN_CENTER_CLICK;
                }
            }
        }
    }

    // 2. Right Button (if available) - Click
    if (PIN_BTN_RIGHT >= 0) {
        bool rightDown = (digitalRead(PIN_BTN_RIGHT) == LOW);
        if (rightDown && !_btnRightWasDown) {
            _btnRightWasDown = true;
        } else if (!rightDown && _btnRightWasDown) {
            _btnRightWasDown = false;
            event = BTN_RIGHT_CLICK;
        }
    }

    // 3. Left / Boot Button (if available) - Click
    if (PIN_BTN_LEFT >= 0) {
        bool leftDown = (digitalRead(PIN_BTN_LEFT) == LOW);
        if (leftDown && !_btnLeftWasDown) {
            _btnLeftWasDown = true;
        } else if (!leftDown && _btnLeftWasDown) {
            _btnLeftWasDown = false;
            event = BTN_LEFT_CLICK;
        }
    }

    return event;
}
