#pragma once
#include <Arduino.h>
#include <SPI.h>
#include "my_config.h"

enum DisplayDriverType
{
    DRIVER_ST7789 = 0,
    DRIVER_GC9A01 = 1
};

class DisplayManager
{
public:
    DisplayManager();

    bool begin();

    // Toggle between ST7789 and GC9A01 drivers at runtime
    void toggleDriver();

    const char* getDriverName() const;

    // Fast drawing into internal 128x128 framebuffer
    void clear(uint16_t color = 0x0000);
    void drawPixel(int16_t x, int16_t y, uint16_t color);
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
    void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);

    void drawChar(int16_t x, int16_t y, char c, uint16_t color, uint16_t bg, uint8_t size = 1);
    void drawString(int16_t x, int16_t y, const char* str, uint16_t color, uint16_t bg = 0x0000, uint8_t size = 1);

    // Render the complete HeadTracker status screen
    void renderUI(float pan, float tilt, float roll,
                  uint16_t crsfPan, uint16_t crsfTilt, uint16_t crsfRoll,
                  uint32_t txCount, bool txOk, uint32_t rxCount, uint32_t lastRxTime,
                  const char* imuName, const uint8_t* uid);

    // Push framebuffer to TFT display over SPI
    void flush();

private:
    DisplayDriverType _driver;
    SPIClass _spi;
    uint16_t* _framebuffer; // 128x128 = 16384 pixels = 32KB

    void writeCommand(uint8_t cmd);
    void writeData(uint8_t data);
    void writeData16(uint16_t data);
    void setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

    void initST7789();
    void initGC9A01();
    DisplayDriverType detectDriver();

    void drawProgressBar(int16_t x, int16_t y, int16_t w, int16_t h,
                         float value, float minVal, float maxVal,
                         uint16_t fillColor, uint16_t borderColor);
};
