#include "button_manager.h"
#include "logging.h"
#include <math.h>

static volatile bool s_bootBtnLatched = false;
static volatile uint32_t s_bootPinInitMs = 0;

static void IRAM_ATTR onBootBtnIsr()
{
    if (millis() - s_bootPinInitMs >= 20) {
        s_bootBtnLatched = true;
    }
}

ButtonManager::ButtonManager()
    : _btnCenterPressStart(0), _btnCenterWasDown(false),
      _btnCenterYellowShown(false), _btnCenterWebFired(false),
      _btnRightWasDown(false), _btnLeftWasDown(false), _flashUntil(0)
{
}

void ButtonManager::begin()
{
    s_bootBtnLatched = false;

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

    delay(5);
    s_bootPinInitMs = millis();

    // Attach hardware interrupt on BOOT/Center button so any click during IMU init or boot window is latched
    if (PIN_BTN_CENTER >= 0) {
        attachInterrupt(digitalPinToInterrupt(PIN_BTN_CENTER), onBootBtnIsr, FALLING);
    }
    if (PIN_BTN_LEFT >= 0) {
        attachInterrupt(digitalPinToInterrupt(PIN_BTN_LEFT), onBootBtnIsr, FALLING);
    }

    // Initialize RGB LED Power if configured
    if (PIN_RGB_PWR >= 0) {
        pinMode(PIN_RGB_PWR, OUTPUT);
        digitalWrite(PIN_RGB_PWR, HIGH); // Power ON WS2812
    }

    // Start with Blue during boot window
    setLed(0, 0, 60);
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

void ButtonManager::updateApModeLed()
{
    uint32_t now = millis();
    if (now < _flashUntil) {
        return; // Preserve temporary Cyan (Center) or Yellow (Tare) flash
    }

    // Smooth breathing Magenta (#7A00B8 -> R=122, G=0, B=184)
    float phase = (float)now * 0.0032f; // ~0.5 Hz full breathing cycle
    float wave = 0.5f * (1.0f + sinf(phase));
    float intensity = 0.08f + 0.92f * (wave * wave); // Quadratic easing for organic breathing
    uint8_t r = (uint8_t)lroundf(122.0f * intensity); // 0x7A
    uint8_t g = 0;                                    // 0x00
    uint8_t b = (uint8_t)lroundf(184.0f * intensity); // 0xB8
    setLed(r, g, b);
}

void ButtonManager::updateBootPulseLed()
{
    uint32_t now = millis();
    if (now < _flashUntil) {
        return;
    }

    // Smooth pulsing Blue during the 5-second power-up window (~2 Hz pulse)
    float phase = (float)now * 0.0125f;
    float wave = 0.5f * (1.0f + sinf(phase));
    uint8_t b = (uint8_t)lroundf(8.0f + 112.0f * wave);
    setLed(0, 0, b);
}

bool ButtonManager::isBootButtonPressed() const
{
    if (PIN_BTN_CENTER >= 0 && digitalRead(PIN_BTN_CENTER) == LOW) {
        return true;
    }
    if (PIN_BTN_LEFT >= 0 && digitalRead(PIN_BTN_LEFT) == LOW) {
        return true;
    }
    return false;
}

bool ButtonManager::wasBootButtonLatched() const
{
    return s_bootBtnLatched || isBootButtonPressed();
}

void ButtonManager::clearBootButtonLatch()
{
    s_bootBtnLatched = false;
}

ButtonEvent ButtonManager::update()
{
    ButtonEvent event = BTN_NONE;
    uint32_t now = millis();

    // 1. Center / BOOT Button (GPIO 9 on ESP32-C3 / GPIO 47 on S3 Mini Pro)
    //    - Short Click (40ms .. 1.2s)   -> BTN_CENTER_CLICK (Center / Zero, Cyan LED)
    //    - Medium Hold (1.2s .. 5.0s)   -> Lights Yellow at 1.2s; fires BTN_CENTER_LONG (Tare IMU) on release
    //    - Long Hold   (>= 5.0s)        -> Lights Magenta and immediately fires BTN_ENTER_WEB_CONFIG
    if (PIN_BTN_CENTER >= 0) {
        bool centerDown = (digitalRead(PIN_BTN_CENTER) == LOW);
        if (centerDown) {
            if (!_btnCenterWasDown) {
                _btnCenterWasDown = true;
                _btnCenterPressStart = now;
                _btnCenterYellowShown = false;
                _btnCenterWebFired = false;
            } else {
                uint32_t heldMs = now - _btnCenterPressStart;
                if (!_btnCenterYellowShown && heldMs >= 1200) {
                    _btnCenterYellowShown = true;
                    // Indicate Tare threshold reached with Yellow LED while continuing to hold (up to 5s)
                    flashLed(80, 80, 0, 3800);
                }
                if (!_btnCenterWebFired && heldMs >= 5000) {
                    _btnCenterWebFired = true;
                    // Indicate Web-IF AP Mode entry with Magenta LED
                    flashLed(122, 0, 184, 1000);
                    event = BTN_ENTER_WEB_CONFIG;
                }
            }
        } else {
            if (_btnCenterWasDown) {
                _btnCenterWasDown = false;
                uint32_t heldMs = now - _btnCenterPressStart;
                if (!_btnCenterWebFired) {
                    if (_btnCenterYellowShown || heldMs >= 1200) {
                        event = BTN_CENTER_LONG;
                    } else if (heldMs > 40) { // Debounced short click
                        event = BTN_CENTER_CLICK;
                    }
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
