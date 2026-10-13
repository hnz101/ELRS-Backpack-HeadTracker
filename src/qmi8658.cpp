#include "qmi8658.h"
#include "logging.h"

// Register definitions for QMI8658
#define QMI8658_WHO_AM_I   0x00
#define QMI8658_REVISION   0x01
#define QMI8658_CTRL1      0x02
#define QMI8658_CTRL2      0x03
#define QMI8658_CTRL3      0x04
#define QMI8658_CTRL5      0x06
#define QMI8658_CTRL7      0x08
#define QMI8658_STATUS0    0x2E
#define QMI8658_STATUS1    0x2F
#define QMI8658_AX_L       0x35

// Scale factors:
// Accel +/- 8g: 4096 LSB/g -> (1.0f / 4096.0f) g
// Gyro +/- 1024 dps: 32 LSB/(deg/s) -> (1.0f / 32.0f) deg/s
static constexpr float ACCEL_SCALE_8G = 1.0f / 4096.0f;
static constexpr float GYRO_SCALE_1024DPS = 1.0f / 32.0f;

QMI8658::QMI8658()
    : _wire(nullptr), _address(0x6B), _connected(false), _calibrated(false),
      _ax(0), _ay(0), _az(1.0f), _gx(0), _gy(0), _gz(0),
      _gyroBiasX(0), _gyroBiasY(0), _gyroBiasZ(0)
{
}

bool QMI8658::begin(TwoWire &wire, uint8_t sda, uint8_t scl, uint8_t address)
{
    _wire = &wire;
    _address = address;

    _wire->begin(sda, scl, 400000); // 400 kHz Fast I2C

    // Probe sensor address (try default 0x6B, then fallback 0x6A)
    uint8_t chipId = readRegister(QMI8658_WHO_AM_I);
    if (chipId != 0x05) {
        _address = 0x6A;
        chipId = readRegister(QMI8658_WHO_AM_I);
    }

    if (chipId != 0x05) {
        LOG_WARN("QMI8658 not detected (WHO_AM_I=0x%02X, expected 0x05)", chipId);
        _connected = false;
        return false;
    }

    LOG_INFO("QMI8658 detected at 0x%02X! WHO_AM_I=0x%02X", _address, chipId);

    // CTRL1: Auto address increment, 50MHz SPI disable
    writeRegister(QMI8658_CTRL1, 0x60);

    // CTRL2: Accel +/- 8g, 100Hz ODR (0x33)
    writeRegister(QMI8658_CTRL2, 0x33);

    // CTRL3: Gyro +/- 1024 dps, 100Hz ODR (0x63)
    writeRegister(QMI8658_CTRL3, 0x63);

    // CTRL5: Low pass filter enable for Accel and Gyro
    writeRegister(QMI8658_CTRL5, 0x55);

    // CTRL7: Enable both Accelerometer and Gyroscope
    writeRegister(QMI8658_CTRL7, 0x03);

    _connected = true;
    delay(50);

    // Perform initial gyro bias calibration
    calibrateGyro(100);

    return true;
}

uint8_t QMI8658::readRegister(uint8_t reg)
{
    if (!_wire) return 0xFF;
    _wire->beginTransmission(_address);
    _wire->write(reg);
    if (_wire->endTransmission(false) != 0) return 0xFF;

    if (_wire->requestFrom((int)_address, 1) == 1) {
        return _wire->read();
    }
    return 0xFF;
}

bool QMI8658::writeRegister(uint8_t reg, uint8_t value)
{
    if (!_wire) return false;
    _wire->beginTransmission(_address);
    _wire->write(reg);
    _wire->write(value);
    return (_wire->endTransmission() == 0);
}

bool QMI8658::readBytes(uint8_t reg, uint8_t *buffer, uint8_t length)
{
    if (!_wire) return false;
    _wire->beginTransmission(_address);
    _wire->write(reg);
    if (_wire->endTransmission(false) != 0) return false;

    uint8_t count = _wire->requestFrom((int)_address, (int)length);
    if (count != length) return false;

    for (uint8_t i = 0; i < length; i++) {
        buffer[i] = _wire->read();
    }
    return true;
}

bool QMI8658::update()
{
    if (!_connected) return false;

    uint8_t data[12];
    // Read 12 bytes starting from QMI8658_AX_L (0x35)
    if (!readBytes(QMI8658_AX_L, data, 12)) {
        return false;
    }

    int16_t raw_ax = (int16_t)(data[0] | (data[1] << 8));
    int16_t raw_ay = (int16_t)(data[2] | (data[3] << 8));
    int16_t raw_az = (int16_t)(data[4] | (data[5] << 8));

    int16_t raw_gx = (int16_t)(data[6] | (data[7] << 8));
    int16_t raw_gy = (int16_t)(data[8] | (data[9] << 8));
    int16_t raw_gz = (int16_t)(data[10] | (data[11] << 8));

    _ax = (float)raw_ax * ACCEL_SCALE_8G;
    _ay = (float)raw_ay * ACCEL_SCALE_8G;
    _az = (float)raw_az * ACCEL_SCALE_8G;

    float raw_deg_gx = (float)raw_gx * GYRO_SCALE_1024DPS;
    float raw_deg_gy = (float)raw_gy * GYRO_SCALE_1024DPS;
    float raw_deg_gz = (float)raw_gz * GYRO_SCALE_1024DPS;

    _gx = raw_deg_gx - _gyroBiasX;
    _gy = raw_deg_gy - _gyroBiasY;
    _gz = raw_deg_gz - _gyroBiasZ;

    return true;
}

void QMI8658::calibrateGyro(uint16_t samples)
{
    if (!_connected) return;

    LOG_INFO("Calibrating Gyro bias (%d samples)... Keep sensor still!", samples);
    float sumX = 0.0f;
    float sumY = 0.0f;
    float sumZ = 0.0f;

    uint16_t validSamples = 0;
    for (uint16_t i = 0; i < samples; i++) {
        uint8_t data[6];
        if (readBytes(0x3B, data, 6)) { // Gyro registers
            int16_t raw_gx = (int16_t)(data[0] | (data[1] << 8));
            int16_t raw_gy = (int16_t)(data[2] | (data[3] << 8));
            int16_t raw_gz = (int16_t)(data[4] | (data[5] << 8));

            sumX += (float)raw_gx * GYRO_SCALE_1024DPS;
            sumY += (float)raw_gy * GYRO_SCALE_1024DPS;
            sumZ += (float)raw_gz * GYRO_SCALE_1024DPS;
            validSamples++;
        }
        delay(10);
    }

    if (validSamples > 0) {
        _gyroBiasX = sumX / (float)validSamples;
        _gyroBiasY = sumY / (float)validSamples;
        _gyroBiasZ = sumZ / (float)validSamples;
        _calibrated = true;
        LOG_INFO("Gyro calibrated! Bias: X=%.2f, Y=%.2f, Z=%.2f dps", _gyroBiasX, _gyroBiasY, _gyroBiasZ);
    }
}
