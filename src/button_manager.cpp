#include "button_manager.h"
#include "logging.h"

ButtonManager::ButtonManager()
    : _btnCenterPressStart(0), _btnCenterWasDown(false), _btnCenterLongFired(false),
      _btnRightWasDown(false), _btnLeftWasDown(false)
{
}

void ButtonManager::begin()
{
    // Initialize Buttons (Active LOW with internal pullups)
    pinMode(PIN_BTN_LEFT, INPUT_PULLUP);
    pinMode(PIN_BTN_CENTER, INPUT_PULLUP);
    pinMode(PIN_BTN_RIGHT, INPUT_PULLUP);

    // Initialize RGB LED
    pinMode(PIN_RGB_PWR, OUTPUT);
    digitalWrite(PIN_RGB_PWR, HIGH); // Power ON WS2812
    setLed(0, 50, 0); // Start with dim Green
}

void ButtonManager::setLed(uint8_t r, uint8_t g, uint8_t b)
{
    rgbLedWrite(PIN_RGB_DATA, r, g, b);
}

ButtonEvent ButtonManager::update()
{
    ButtonEvent event = BTN_NONE;
    uint32_t now = millis();

    // 1. Center Button (GPIO 47) - Supports Click and Long Press (> 1.5s)
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

    // 2. Right Button (GPIO 48) - Click
    bool rightDown = (digitalRead(PIN_BTN_RIGHT) == LOW);
    if (rightDown && !_btnRightWasDown) {
        _btnRightWasDown = true;
    } else if (!rightDown && _btnRightWasDown) {
        _btnRightWasDown = false;
        event = BTN_RIGHT_CLICK;
    }

    // 3. Left / Boot Button (GPIO 0) - Click
    bool leftDown = (digitalRead(PIN_BTN_LEFT) == LOW);
    if (leftDown && !_btnLeftWasDown) {
        _btnLeftWasDown = true;
    } else if (!leftDown && _btnLeftWasDown) {
        _btnLeftWasDown = false;
        event = BTN_LEFT_CLICK;
    }

    return event;
}
