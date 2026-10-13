#include "headtracker.h"
#include "logging.h"
#include <math.h>

HeadTracker::HeadTracker()
    : _usingBno(false), _lastUpdateMicros(0),
      _pitch(0), _roll(0), _yaw(0),
      _zeroPitch(0), _zeroRoll(0), _zeroYaw(0),
      _relPan(0), _relTilt(0), _relRoll(0),
      _crsfPan(CRSF_CHANNEL_CENTER),
      _crsfTilt(CRSF_CHANNEL_CENTER),
      _crsfRoll(CRSF_CHANNEL_CENTER)
{
}

bool HeadTracker::begin()
{
    LOG_INFO("Initializing HeadTracker IMU...");

    // 1. Try BNO085 first at I2C address 0x4A (if connected)
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);
    Wire.beginTransmission(0x4A);
    if (Wire.endTransmission() == 0) {
        LOG_INFO("BNO085 detected at 0x4A!");
        _usingBno = true;
        // Hook for BNO085 initialization when sensor is plugged in
        return true;
    }

    // 2. Fall back to onboard QMI8658 at 0x6B
    if (_qmi.begin(Wire, PIN_I2C_SDA, PIN_I2C_SCL, 0x6B)) {
        LOG_INFO("Using onboard QMI8658 IMU");
        _usingBno = false;

        // Read initial static orientation from gravity vector (display-up orientation)
        delay(20);
        if (_qmi.update()) {
            float ax = _qmi.getAccelX();
            float ay = _qmi.getAccelY();
            float az = _qmi.getAccelZ();
            _pitch = atan2f(-ax, -az) * (180.0f / M_PI);
            _roll  = atan2f(ay, -az) * (180.0f / M_PI);
            _yaw   = 0.0f;
        }

        center();
        _lastUpdateMicros = micros();
        return true;
    }

    LOG_ERROR("No IMU sensor found!");
    return false;
}

void HeadTracker::center()
{
    _zeroPitch = _pitch;
    _zeroRoll  = _roll;
    _zeroYaw   = _yaw;
    LOG_INFO("HeadTracker Centered: Zero Yaw=%.1f, Pitch=%.1f, Roll=%.1f", _zeroYaw, _zeroPitch, _zeroRoll);
}

void HeadTracker::calibrate()
{
    if (!_usingBno) {
        _qmi.calibrateGyro(150);
        if (_qmi.update()) {
            float ax = _qmi.getAccelX();
            float ay = _qmi.getAccelY();
            float az = _qmi.getAccelZ();
            _pitch = atan2f(-ax, -az) * (180.0f / M_PI);
            _roll  = atan2f(ay, -az) * (180.0f / M_PI);
        }
        _yaw = 0;
        center();
    }
}

bool HeadTracker::isSensorConnected() const
{
    return _usingBno || _qmi.isConnected();
}

const char* HeadTracker::getSensorName() const
{
    if (_usingBno) return "BNO085";
    if (_qmi.isConnected()) return "QMI8658";
    return "NONE";
}

uint16_t HeadTracker::angleToCrsf(float angle, float maxAngle, bool invert)
{
    if (invert) angle = -angle;

    // Constrain to configured maximum deflection angle
    if (angle > maxAngle)  angle = maxAngle;
    if (angle < -maxAngle) angle = -maxAngle;

    // Map -maxAngle..+maxAngle to CRSF_CHANNEL_MIN..CRSF_CHANNEL_MAX
    float normalized = angle / maxAngle; // -1.0 .. +1.0
    int32_t crsf = (int32_t)(CRSF_CHANNEL_CENTER + normalized * (CRSF_CHANNEL_MAX - CRSF_CHANNEL_CENTER));

    if (crsf < CRSF_CHANNEL_MIN) crsf = CRSF_CHANNEL_MIN;
    if (crsf > CRSF_CHANNEL_MAX) crsf = CRSF_CHANNEL_MAX;

    return (uint16_t)crsf;
}

void HeadTracker::update()
{
    uint32_t now = micros();
    float dt = (now - _lastUpdateMicros) * 1e-6f;
    _lastUpdateMicros = now;

    if (dt <= 0.0f || dt > 0.5f) {
        dt = 0.02f; // Fallback for timer wrap / startup
    }

    if (!_usingBno && _qmi.isConnected()) {
        if (_qmi.update()) {
            float ax = _qmi.getAccelX();
            float ay = _qmi.getAccelY();
            float az = _qmi.getAccelZ();

            float gx = _qmi.getGyroX();
            float gy = _qmi.getGyroY();
            float gz = _qmi.getGyroZ();

            // On LOLIN S3 Mini Pro, the QMI8658 is on the bottom side of the PCB.
            // When the display is facing UP, gravity points in the negative Z direction (az ~ -1.0g).
            // Therefore, we use -az as the upward normal reference (+1.0g).
            // Pitch (Tilt) - rotation around lateral Y axis (front/back nod)
            float pitchAcc = atan2f(-ax, -az) * (180.0f / M_PI);
            // Roll - rotation around longitudinal X axis (ear to shoulder tilt)
            float rollAcc = atan2f(ay, -az) * (180.0f / M_PI);

            // Complementary filter: 98% Gyro integration + 2% Accel reference
            constexpr float alpha = 0.98f;
            _pitch = alpha * (_pitch + gy * dt) + (1.0f - alpha) * pitchAcc;
            _roll  = alpha * (_roll  + gx * dt) + (1.0f - alpha) * rollAcc;

            // Pan (Yaw): Integrate Z-axis gyro (inverted because Z axis points downward)
            _yaw += (-gz) * dt;
            if (_yaw > 180.0f)  _yaw -= 360.0f;
            if (_yaw < -180.0f) _yaw += 360.0f;
        }
    }

    // Relative angles against center reference
    _relTilt = _pitch - _zeroPitch;
    _relRoll = _roll  - _zeroRoll;

    _relPan = _yaw - _zeroYaw;
    while (_relPan > 180.0f)  _relPan -= 360.0f;
    while (_relPan < -180.0f) _relPan += 360.0f;

    #if HT_INVERT_PAN
    _relPan = -_relPan;
    #endif
    #if HT_INVERT_TILT
    _relTilt = -_relTilt;
    #endif
    #if HT_INVERT_ROLL
    _relRoll = -_relRoll;
    #endif

    // Convert relative angles to CRSF values
    _crsfPan  = angleToCrsf(_relPan,  HT_MAX_ANGLE_PAN,  false);
    _crsfTilt = angleToCrsf(_relTilt, HT_MAX_ANGLE_TILT, false);
    _crsfRoll = angleToCrsf(_relRoll, HT_MAX_ANGLE_ROLL, false);
}

void HeadTracker::getChannels16(uint16_t* channels)
{
    // Initialize all channels to neutral center (1500 us / 992)
    for (int i = 0; i < 16; i++) {
        channels[i] = CRSF_CHANNEL_CENTER;
    }

    // Assign configured HeadTracker channels (defaults: TR1=Pan, TR2=Tilt, TR3=Roll)
    channels[HT_CH_PAN]  = _crsfPan;
    channels[HT_CH_TILT] = _crsfTilt;
    channels[HT_CH_ROLL] = _crsfRoll;
}
