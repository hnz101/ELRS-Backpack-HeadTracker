#include "display_manager.h"
#include "logging.h"

#if !FEATURE_DISPLAY

DisplayManager::DisplayManager() : _driver(DRIVER_ST7789), _framebuffer(nullptr) {}
bool DisplayManager::begin() { return false; }
void DisplayManager::toggleDriver() {}
const char* DisplayManager::getDriverName() const { return "NONE"; }
void DisplayManager::clear(uint16_t color) {}
void DisplayManager::drawPixel(int16_t x, int16_t y, uint16_t color) {}
void DisplayManager::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {}
void DisplayManager::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {}
void DisplayManager::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {}
void DisplayManager::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {}
void DisplayManager::drawChar(int16_t x, int16_t y, char c, uint16_t color, uint16_t bg, uint8_t size) {}
void DisplayManager::drawString(int16_t x, int16_t y, const char* str, uint16_t color, uint16_t bg, uint8_t size) {}
void DisplayManager::renderUI(float pan, float tilt, float roll,
                              uint16_t crsfPan, uint16_t crsfTilt, uint16_t crsfRoll,
                              uint32_t txCount, bool txOk, uint32_t rxCount, uint32_t lastRxTime,
                              const char* imuName, const uint8_t* uid) {}
void DisplayManager::flush() {}

#else

// Basic 5x7 ASCII font data
static const uint8_t font5x7[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, // (space)
    0x00, 0x00, 0x5F, 0x00, 0x00, // !
    0x00, 0x07, 0x00, 0x07, 0x00, // "
    0x14, 0x7F, 0x14, 0x7F, 0x14, // #
    0x24, 0x2A, 0x7F, 0x2A, 0x12, // $
    0x23, 0x13, 0x08, 0x64, 0x62, // %
    0x36, 0x49, 0x55, 0x22, 0x50, // &
    0x00, 0x05, 0x03, 0x00, 0x00, // '
    0x00, 0x1C, 0x22, 0x41, 0x00, // (
    0x00, 0x41, 0x22, 0x1C, 0x00, // )
    0x08, 0x2A, 0x1C, 0x2A, 0x08, // *
    0x08, 0x08, 0x3E, 0x08, 0x08, // +
    0x00, 0x50, 0x30, 0x00, 0x00, // ,
    0x08, 0x08, 0x08, 0x08, 0x08, // -
    0x00, 0x60, 0x60, 0x00, 0x00, // .
    0x20, 0x10, 0x08, 0x04, 0x02, // /
    0x3E, 0x51, 0x49, 0x45, 0x3E, // 0
    0x00, 0x42, 0x7F, 0x40, 0x00, // 1
    0x42, 0x61, 0x51, 0x49, 0x46, // 2
    0x21, 0x41, 0x45, 0x4B, 0x31, // 3
    0x18, 0x14, 0x12, 0x7F, 0x10, // 4
    0x27, 0x45, 0x45, 0x45, 0x39, // 5
    0x3C, 0x4A, 0x49, 0x49, 0x30, // 6
    0x01, 0x71, 0x09, 0x05, 0x03, // 7
    0x36, 0x49, 0x49, 0x49, 0x36, // 8
    0x06, 0x49, 0x49, 0x29, 0x1E, // 9
    0x00, 0x36, 0x36, 0x00, 0x00, // :
    0x00, 0x56, 0x36, 0x00, 0x00, // ;
    0x00, 0x08, 0x14, 0x22, 0x41, // <
    0x14, 0x14, 0x14, 0x14, 0x14, // =
    0x41, 0x22, 0x14, 0x08, 0x00, // >
    0x02, 0x01, 0x51, 0x09, 0x06, // ?
    0x32, 0x49, 0x79, 0x41, 0x3E, // @
    0x7E, 0x11, 0x11, 0x11, 0x7E, // A
    0x7F, 0x49, 0x49, 0x49, 0x36, // B
    0x3E, 0x41, 0x41, 0x41, 0x22, // C
    0x7F, 0x41, 0x41, 0x22, 0x1C, // D
    0x7F, 0x49, 0x49, 0x49, 0x41, // E
    0x7F, 0x09, 0x09, 0x01, 0x01, // F
    0x3E, 0x41, 0x41, 0x51, 0x32, // G
    0x7F, 0x08, 0x08, 0x08, 0x7F, // H
    0x00, 0x41, 0x7F, 0x41, 0x00, // I
    0x20, 0x40, 0x41, 0x3F, 0x01, // J
    0x7F, 0x08, 0x14, 0x22, 0x41, // K
    0x7F, 0x40, 0x40, 0x40, 0x40, // L
    0x7F, 0x02, 0x04, 0x02, 0x7F, // M
    0x7F, 0x04, 0x08, 0x10, 0x7F, // N
    0x3E, 0x41, 0x41, 0x41, 0x3E, // O
    0x7F, 0x09, 0x09, 0x09, 0x06, // P
    0x3E, 0x41, 0x51, 0x21, 0x5E, // Q
    0x7F, 0x09, 0x19, 0x29, 0x46, // R
    0x46, 0x49, 0x49, 0x49, 0x31, // S
    0x01, 0x01, 0x7F, 0x01, 0x01, // T
    0x3F, 0x40, 0x40, 0x40, 0x3F, // U
    0x1F, 0x20, 0x40, 0x20, 0x1F, // V
    0x7F, 0x20, 0x18, 0x20, 0x7F, // W
    0x63, 0x14, 0x08, 0x14, 0x63, // X
    0x03, 0x04, 0x78, 0x04, 0x03, // Y
    0x61, 0x51, 0x49, 0x45, 0x43, // Z
    0x00, 0x7F, 0x41, 0x41, 0x00, // [
    0x02, 0x04, 0x08, 0x10, 0x20, // backslash
    0x00, 0x41, 0x41, 0x7F, 0x00, // ]
    0x04, 0x02, 0x01, 0x02, 0x04, // ^
    0x40, 0x40, 0x40, 0x40, 0x40, // _
    0x00, 0x01, 0x02, 0x04, 0x00, // `
    0x20, 0x54, 0x54, 0x54, 0x78, // a
    0x7F, 0x48, 0x44, 0x44, 0x38, // b
    0x38, 0x44, 0x44, 0x44, 0x20, // c
    0x38, 0x44, 0x44, 0x48, 0x7F, // d
    0x38, 0x54, 0x54, 0x54, 0x18, // e
    0x08, 0x7E, 0x09, 0x01, 0x02, // f
    0x08, 0x14, 0x54, 0x54, 0x3C, // g
    0x7F, 0x08, 0x04, 0x04, 0x78, // h
    0x00, 0x44, 0x7D, 0x40, 0x00, // i
    0x20, 0x40, 0x44, 0x3D, 0x00, // j
    0x7F, 0x10, 0x28, 0x44, 0x00, // k
    0x00, 0x41, 0x7F, 0x40, 0x00, // l
    0x7C, 0x04, 0x18, 0x04, 0x78, // m
    0x7C, 0x08, 0x04, 0x04, 0x78, // n
    0x38, 0x44, 0x44, 0x44, 0x38, // o
    0x7C, 0x14, 0x14, 0x14, 0x08, // p
    0x08, 0x14, 0x14, 0x18, 0x7C, // q
    0x7C, 0x08, 0x04, 0x04, 0x08, // r
    0x48, 0x54, 0x54, 0x54, 0x20, // s
    0x04, 0x3F, 0x44, 0x40, 0x20, // t
    0x3C, 0x40, 0x40, 0x20, 0x7C, // u
    0x1C, 0x20, 0x40, 0x20, 0x1C, // v
    0x3C, 0x40, 0x30, 0x40, 0x3C, // w
    0x44, 0x28, 0x10, 0x28, 0x44, // x
    0x0C, 0x50, 0x50, 0x50, 0x3C, // y
    0x44, 0x64, 0x54, 0x4C, 0x44, // z
    0x00, 0x08, 0x36, 0x41, 0x00, // {
    0x00, 0x00, 0x7F, 0x00, 0x00, // |
    0x00, 0x41, 0x36, 0x08, 0x00, // }
    0x08, 0x08, 0x2A, 0x1C, 0x08  // ~
};

// Colors (16-bit RGB565)
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_CYAN    0x07FF
#define COLOR_YELLOW  0xFFE0
#define COLOR_ORANGE  0xFD20
#define COLOR_GRAY    0x39E7
#define COLOR_DARKGRAY 0x18E3

DisplayManager::DisplayManager()
    : _driver(DRIVER_ST7789), _spi(FSPI), _framebuffer(nullptr)
{
}

bool DisplayManager::begin()
{
    LOG_INFO("Initializing TFT Display (0.85\" 128x128)...");

    // Allocate 128x128 framebuffer (16-bit color = 32768 bytes)
    if (!_framebuffer) {
        _framebuffer = (uint16_t*)heap_caps_malloc(128 * 128 * sizeof(uint16_t), MALLOC_CAP_8BIT);
        if (!_framebuffer) {
            _framebuffer = (uint16_t*)malloc(128 * 128 * sizeof(uint16_t));
        }
    }

    if (_framebuffer) {
        memset(_framebuffer, 0, 128 * 128 * sizeof(uint16_t));
    }

    // Configure GPIOs
    pinMode(PIN_TFT_BL, OUTPUT);
    digitalWrite(PIN_TFT_BL, LOW); // Off during reset

    pinMode(PIN_TFT_CS, OUTPUT);
    digitalWrite(PIN_TFT_CS, HIGH);

    pinMode(PIN_TFT_DC, OUTPUT);
    digitalWrite(PIN_TFT_DC, HIGH);

    pinMode(PIN_TFT_RST, OUTPUT);
    digitalWrite(PIN_TFT_RST, HIGH);

    // Hardware Reset
    delay(20);
    digitalWrite(PIN_TFT_RST, LOW);
    delay(50);
    digitalWrite(PIN_TFT_RST, HIGH);
    delay(120);

    // Start SPI bus at 40MHz
    _spi.begin(PIN_TFT_SCLK, PIN_TFT_MISO, PIN_TFT_MOSI, PIN_TFT_CS);

    // Auto-detect driver
    _driver = detectDriver();

    if (_driver == DRIVER_ST7789) {
        initST7789();
    } else {
        initGC9A01();
    }

    // Turn Backlight ON
    digitalWrite(PIN_TFT_BL, HIGH);

    // Clear display
    clear(COLOR_BLACK);
    flush();

    LOG_INFO("TFT Display initialized successfully (Driver: %s)", getDriverName());
    return true;
}

DisplayDriverType DisplayManager::detectDriver()
{
    // Probe via command 0x04 (Read Display ID)
    digitalWrite(PIN_TFT_CS, LOW);
    digitalWrite(PIN_TFT_DC, LOW);
    _spi.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));
    _spi.transfer(0x04);
    digitalWrite(PIN_TFT_DC, HIGH);
    
    // Read dummy byte then 3 ID bytes
    _spi.transfer(0x00);
    uint8_t id1 = _spi.transfer(0x00);
    uint8_t id2 = _spi.transfer(0x00);
    uint8_t id3 = _spi.transfer(0x00);
    _spi.endTransaction();
    digitalWrite(PIN_TFT_CS, HIGH);

    LOG_INFO("TFT Probe RDDID: 0x%02X 0x%02X 0x%02X", id1, id2, id3);

    // ST7789 typically reports ID2 = 0x85
    if (id2 == 0x85) {
        LOG_INFO("Detected ST7789 Controller");
        return DRIVER_ST7789;
    }
    // Default to ST7789 (most common in recent batches) or GC9A01
    return DRIVER_ST7789;
}

void DisplayManager::toggleDriver()
{
    if (_driver == DRIVER_ST7789) {
        _driver = DRIVER_GC9A01;
        initGC9A01();
    } else {
        _driver = DRIVER_ST7789;
        initST7789();
    }
    LOG_INFO("Switched Display Driver to: %s", getDriverName());
    flush();
}

const char* DisplayManager::getDriverName() const
{
    return (_driver == DRIVER_ST7789) ? "ST7789" : "GC9A01";
}

void DisplayManager::writeCommand(uint8_t cmd)
{
    digitalWrite(PIN_TFT_DC, LOW);
    digitalWrite(PIN_TFT_CS, LOW);
    _spi.beginTransaction(SPISettings(40000000, MSBFIRST, SPI_MODE0));
    _spi.transfer(cmd);
    _spi.endTransaction();
    digitalWrite(PIN_TFT_CS, HIGH);
}

void DisplayManager::writeData(uint8_t data)
{
    digitalWrite(PIN_TFT_DC, HIGH);
    digitalWrite(PIN_TFT_CS, LOW);
    _spi.beginTransaction(SPISettings(40000000, MSBFIRST, SPI_MODE0));
    _spi.transfer(data);
    _spi.endTransaction();
    digitalWrite(PIN_TFT_CS, HIGH);
}

void DisplayManager::initST7789()
{
    writeCommand(0x01); // Software Reset
    delay(150);

    writeCommand(0x11); // Sleep Out
    delay(120);

    writeCommand(0x3A); // Color mode
    writeData(0x55);    // 16-bit color (RGB565)

    writeCommand(0x36); // MADCTL
    writeData(0x60);    // BGR mode + orientation

    writeCommand(0x21); // Display Inversion On
    delay(10);

    writeCommand(0x13); // Normal Display Mode On
    delay(10);

    writeCommand(0x29); // Display ON
    delay(100);
}

void DisplayManager::initGC9A01()
{
    writeCommand(0xEF);
    writeCommand(0xEB); writeData(0x14);
    writeCommand(0xFE); // Inter register enable 1
    writeCommand(0xEF); // Inter register enable 2

    writeCommand(0xEB); writeData(0x14);
    writeCommand(0x84); writeData(0x40);
    writeCommand(0x85); writeData(0xFF);
    writeCommand(0x86); writeData(0xFF);
    writeCommand(0x87); writeData(0xFF);
    writeCommand(0x8E); writeData(0xFF);
    writeCommand(0x8F); writeData(0xFF);

    writeCommand(0x3A); writeData(0x05); // 16-bit format

    writeCommand(0x36); writeData(0x08); // MADCTL BGR

    writeCommand(0x21); // Display Inversion ON
    writeCommand(0x11); // Sleep OUT
    delay(120);
    writeCommand(0x29); // Display ON
    delay(100);
}

void DisplayManager::setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    // Center offset for 128x128 panel inside 240x240 memory
    uint16_t colOffset = (_driver == DRIVER_ST7789) ? 0 : 0;
    uint16_t rowOffset = (_driver == DRIVER_ST7789) ? 0 : 0;

    writeCommand(0x2A); // CASET
    writeData((x0 + colOffset) >> 8);
    writeData((x0 + colOffset) & 0xFF);
    writeData((x1 + colOffset) >> 8);
    writeData((x1 + colOffset) & 0xFF);

    writeCommand(0x2B); // RASET
    writeData((y0 + rowOffset) >> 8);
    writeData((y0 + rowOffset) & 0xFF);
    writeData((y1 + rowOffset) >> 8);
    writeData((y1 + rowOffset) & 0xFF);

    writeCommand(0x2C); // RAMWR
}

void DisplayManager::flush()
{
    if (!_framebuffer) return;

    setWindow(0, 0, 127, 127);

    digitalWrite(PIN_TFT_DC, HIGH);
    digitalWrite(PIN_TFT_CS, LOW);
    _spi.beginTransaction(SPISettings(40000000, MSBFIRST, SPI_MODE0));

    // Fast 16-bit transfer of entire 128x128 buffer
    _spi.transferBytes((uint8_t*)_framebuffer, nullptr, 128 * 128 * sizeof(uint16_t));

    _spi.endTransaction();
    digitalWrite(PIN_TFT_CS, HIGH);
}

void DisplayManager::clear(uint16_t color)
{
    if (!_framebuffer) return;
    for (int i = 0; i < 128 * 128; i++) {
        _framebuffer[i] = color;
    }
}

void DisplayManager::drawPixel(int16_t x, int16_t y, uint16_t color)
{
    if (x < 0 || x >= 128 || y < 0 || y >= 128 || !_framebuffer) return;
    // Swap bytes for little-endian SPI
    uint16_t swapped = (color >> 8) | (color << 8);
    _framebuffer[y * 128 + x] = swapped;
}

void DisplayManager::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
{
    if (y < 0 || y >= 128 || w <= 0) return;
    if (x < 0) { w += x; x = 0; }
    if (x + w > 128) w = 128 - x;
    uint16_t swapped = (color >> 8) | (color << 8);
    for (int16_t i = 0; i < w; i++) {
        _framebuffer[y * 128 + x + i] = swapped;
    }
}

void DisplayManager::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color)
{
    if (x < 0 || x >= 128 || h <= 0) return;
    if (y < 0) { h += y; y = 0; }
    if (y + h > 128) h = 128 - y;
    uint16_t swapped = (color >> 8) | (color << 8);
    for (int16_t i = 0; i < h; i++) {
        _framebuffer[(y + i) * 128 + x] = swapped;
    }
}

void DisplayManager::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    for (int16_t i = 0; i < h; i++) {
        drawFastHLine(x, y + i, w, color);
    }
}

void DisplayManager::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    drawFastHLine(x, y, w, color);
    drawFastHLine(x, y + h - 1, w, color);
    drawFastVLine(x, y, h, color);
    drawFastVLine(x + w - 1, y, h, color);
}

void DisplayManager::drawChar(int16_t x, int16_t y, char c, uint16_t color, uint16_t bg, uint8_t size)
{
    if (c < 32 || c > 126) c = '?';
    const uint8_t* charData = &font5x7[(c - 32) * 5];

    for (int8_t i = 0; i < 5; i++) {
        uint8_t line = charData[i];
        for (int8_t j = 0; j < 8; j++) {
            if (line & 0x1) {
                if (size == 1) drawPixel(x + i, y + j, color);
                else fillRect(x + i * size, y + j * size, size, size, color);
            } else if (bg != color) {
                if (size == 1) drawPixel(x + i, y + j, bg);
                else fillRect(x + i * size, y + j * size, size, size, bg);
            }
            line >>= 1;
        }
    }
}

void DisplayManager::drawString(int16_t x, int16_t y, const char* str, uint16_t color, uint16_t bg, uint8_t size)
{
    int16_t cursorX = x;
    while (*str) {
        if (*str == '\n') {
            cursorX = x;
            y += 8 * size;
        } else {
            drawChar(cursorX, y, *str, color, bg, size);
            cursorX += 6 * size;
        }
        str++;
    }
}

void DisplayManager::drawProgressBar(int16_t x, int16_t y, int16_t w, int16_t h,
                                    float value, float minVal, float maxVal,
                                    uint16_t fillColor, uint16_t borderColor)
{
    drawRect(x, y, w, h, borderColor);
    int16_t midX = x + (w / 2);
    drawFastVLine(midX, y, h, COLOR_GRAY); // Center mark

    float clamped = constrain(value, minVal, maxVal);
    float norm = (clamped - minVal) / (maxVal - minVal); // 0.0 .. 1.0
    int16_t targetX = x + (int16_t)(norm * (w - 2));

    if (targetX >= midX) {
        fillRect(midX, y + 1, targetX - midX + 1, h - 2, fillColor);
    } else {
        fillRect(targetX, y + 1, midX - targetX + 1, h - 2, fillColor);
    }
}

void DisplayManager::renderUI(float pan, float tilt, float roll,
                              uint16_t crsfPan, uint16_t crsfTilt, uint16_t crsfRoll,
                              uint32_t txCount, bool txOk, uint32_t rxCount, uint32_t lastRxTime,
                              const char* imuName, const uint8_t* uid)
{
    clear(COLOR_BLACK);

    // Header banner
    fillRect(0, 0, 128, 14, COLOR_BLUE);
    drawString(8, 3, "HEAD TRACKER", COLOR_WHITE, COLOR_BLUE, 1);

    // Status line: IMU & Driver
    char buf[32];
    snprintf(buf, sizeof(buf), "%s | %s", imuName, getDriverName());
    drawString(4, 18, buf, COLOR_CYAN, COLOR_BLACK, 1);

    // Pan Axis (TR1)
    snprintf(buf, sizeof(buf), "PAN  %+.1f*", pan);
    drawString(4, 29, buf, COLOR_YELLOW, COLOR_BLACK, 1);
    snprintf(buf, sizeof(buf), "%4u", crsfPan);
    drawString(98, 29, buf, COLOR_WHITE, COLOR_BLACK, 1);
    drawProgressBar(4, 38, 120, 6, pan, -HT_MAX_ANGLE_PAN, HT_MAX_ANGLE_PAN, COLOR_YELLOW, COLOR_DARKGRAY);

    // Tilt Axis (TR2)
    snprintf(buf, sizeof(buf), "TILT %+.1f*", tilt);
    drawString(4, 48, buf, COLOR_GREEN, COLOR_BLACK, 1);
    snprintf(buf, sizeof(buf), "%4u", crsfTilt);
    drawString(98, 48, buf, COLOR_WHITE, COLOR_BLACK, 1);
    drawProgressBar(4, 57, 120, 6, tilt, -HT_MAX_ANGLE_TILT, HT_MAX_ANGLE_TILT, COLOR_GREEN, COLOR_DARKGRAY);

    // Roll Axis (TR3)
    snprintf(buf, sizeof(buf), "ROLL %+.1f*", roll);
    drawString(4, 67, buf, COLOR_ORANGE, COLOR_BLACK, 1);
    snprintf(buf, sizeof(buf), "%4u", crsfRoll);
    drawString(98, 67, buf, COLOR_WHITE, COLOR_BLACK, 1);
    drawProgressBar(4, 76, 120, 6, roll, -HT_MAX_ANGLE_ROLL, HT_MAX_ANGLE_ROLL, COLOR_ORANGE, COLOR_DARKGRAY);

    // Divider
    drawFastHLine(0, 85, 128, COLOR_DARKGRAY);

    // TX Send and RX telemetry status
    snprintf(buf, sizeof(buf), "TX:%lu", txCount);
    drawString(4, 88, buf, txOk ? COLOR_GREEN : COLOR_RED, COLOR_BLACK, 1);

    if (rxCount > 0) {
        snprintf(buf, sizeof(buf), "RX:OK(%lu)", rxCount);
        drawString(64, 88, buf, COLOR_GREEN, COLOR_BLACK, 1);
    } else {
        drawString(64, 88, "RX:WAIT...", COLOR_GRAY, COLOR_BLACK, 1);
    }

    // ELRS Backpack UID
    snprintf(buf, sizeof(buf), "UID:%02X:%02X:%02X:%02X:%02X:%02X",
             uid[0], uid[1], uid[2], uid[3], uid[4], uid[5]);
    drawString(4, 100, buf, COLOR_CYAN, COLOR_BLACK, 1);

    // Footer
    drawString(4, 114, "C:Zero | R:Mode/Wave", COLOR_GRAY, COLOR_BLACK, 1);

    // Push to LCD
    flush();
}

#endif
