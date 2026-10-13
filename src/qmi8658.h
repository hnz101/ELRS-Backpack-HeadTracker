#pragma once
#include <Arduino.h>
#include <Wire.h>

class QMI8658
{
public:
    QMI8658();

    // Initialize sensor on given I2C pins, returns true if sensor detected
    bool begin(TwoWire &wire = Wire, uint8_t sda = 12, uint8_t scl = 11, uint8_t address = 0x6B);

    // Read raw data and update sensor readings
    bool update();

    // Calibrate gyroscope bias (device must be held still for ~1.5s)
    void calibrateGyro(uint16_t samples = 150);

    // Get accelerometer in g's
    float getAccelX() const { return _ax; }
    float getAccelY() const { return _ay; }
    float getAccelZ() const { return _az; }

    // Get gyro in degrees/second (bias-compensated)
    float getGyroX() const { return _gx; }
    float getGyroY() const { return _gy; }
    float getGyroZ() const { return _gz; }

    bool isConnected() const { return _connected; }
    bool isCalibrated() const { return _calibrated; }

private:
    TwoWire* _wire;
    uint8_t  _address;
    bool     _connected;
    bool     _calibrated;

    float _ax, _ay, _az;
    float _gx, _gy, _gz;

    float _gyroBiasX;
    float _gyroBiasY;
    float _gyroBiasZ;

    uint8_t readRegister(uint8_t reg);
    bool    writeRegister(uint8_t reg, uint8_t value);
    bool    readBytes(uint8_t reg, uint8_t *buffer, uint8_t length);
};
