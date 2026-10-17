#include "headtracker.h"
#include "logging.h"
#include <math.h>

HeadTracker::HeadTracker()
    : _usingBno(false), _bnoAddress(0), _fusionMode(BNO085_FUSION_MODE),
      _swapPitchRoll(HT_SWAP_PITCH_ROLL),
      _invertPan(HT_INVERT_PAN),
      _invertTilt(HT_INVERT_TILT),
      _invertRoll(HT_INVERT_ROLL),
      _lastUpdateMicros(0),
      _pitch(0), _roll(0), _yaw(0),
      _zeroPitch(0), _zeroRoll(0), _zeroYaw(0),
      _relPan(0), _relTilt(0), _relRoll(0),
      _finalPan(0), _finalTilt(0), _finalRoll(0),
      _crsfPan(CRSF_CHANNEL_CENTER),
      _crsfTilt(CRSF_CHANNEL_CENTER),
      _crsfRoll(CRSF_CHANNEL_CENTER)
{
    _currentQuat = {1.0f, 0.0f, 0.0f, 0.0f};
    _zeroQuat    = {1.0f, 0.0f, 0.0f, 0.0f};
}

Quat HeadTracker::quatMultiply(const Quat& q1, const Quat& q2)
{
    Quat q;
    q.w = q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z;
    q.x = q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y;
    q.y = q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x;
    q.z = q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w;
    return q;
}

Quat HeadTracker::quatInverse(const Quat& q)
{
    Quat inv;
    inv.w =  q.w;
    inv.x = -q.x;
    inv.y = -q.y;
    inv.z = -q.z;
    return inv;
}

void HeadTracker::quatToEuler(const Quat& q, float &yaw, float &pitch, float &roll)
{
    float norm = sqrtf(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
    if (norm < 1e-6f) return;
    float qw = q.w / norm;
    float qx = q.x / norm;
    float qy = q.y / norm;
    float qz = q.z / norm;

    // Roll (rotation around X-axis):
    float t0 = +2.0f * (qw * qx + qy * qz);
    float t1 = +1.0f - 2.0f * (qx * qx + qy * qy);
    roll = atan2f(t0, t1) * (180.0f / M_PI);

    // Pitch (rotation around Y-axis):
    float t2 = +2.0f * (qw * qy - qz * qx);
    if (t2 > 1.0f) t2 = 1.0f;
    if (t2 < -1.0f) t2 = -1.0f;
    pitch = asinf(t2) * (180.0f / M_PI);

    // Yaw (rotation around Z-axis):
    float t3 = +2.0f * (qw * qz + qx * qy);
    float t4 = +1.0f - 2.0f * (qy * qy + qz * qz);
    yaw = atan2f(t3, t4) * (180.0f / M_PI);
}

void HeadTracker::toggleInvertTilt()
{
    _invertTilt = !_invertTilt;
    LOG_INFO("Pitch Inversion: %s", _invertTilt ? "INVERTED" : "NORMAL");
}

void HeadTracker::toggleInvertPan()
{
    _invertPan = !_invertPan;
    LOG_INFO("Pan/Yaw Inversion: %s", _invertPan ? "INVERTED" : "NORMAL");
}

void HeadTracker::toggleInvertRoll()
{
    _invertRoll = !_invertRoll;
    LOG_INFO("Roll Inversion: %s", _invertRoll ? "INVERTED" : "NORMAL");
}

void HeadTracker::toggleSwapPitchRoll()
{
    _swapPitchRoll = !_swapPitchRoll;
    LOG_INFO("Pitch/Roll Swap: %s", _swapPitchRoll ? "SWAPPED (90 deg)" : "NORMAL");
}

void HeadTracker::applyFusionMode()
{
    if (!_usingBno) return;

    if (_fusionMode == 0) {
        LOG_INFO("Configuring BNO085: 9-DOF ROTATION VECTOR (Mag + Gyro + Accel -> ZERO YAW DRIFT)");
        _bno.enableRotationVector(10); // 10ms = 100Hz
    } else if (_fusionMode == 1) {
        LOG_INFO("Configuring BNO085: 9-DOF AR/VR STABILIZED ROTATION VECTOR (Headset-Tuned)");
        _bno.enableARVRStabilizedRotationVector(10);
    } else {
        LOG_INFO("Configuring BNO085: 6-DOF GAME ROTATION VECTOR (Accel + Gyro ONLY, no Magnetometer)");
        _bno.enableGameRotationVector(10);
    }
}

void HeadTracker::setFusionMode(uint8_t mode)
{
    _fusionMode = mode % 3;
    applyFusionMode();
    center();
}

bool HeadTracker::begin()
{
    LOG_INFO("Initializing HeadTracker IMU...");

    // 1. Hardware Reset BNO085 if RST pin is configured
    if (PIN_BNO_RST >= 0) {
        LOG_INFO("Hardware reset BNO085 via GPIO%d...", PIN_BNO_RST);
        pinMode(PIN_BNO_RST, OUTPUT);
        digitalWrite(PIN_BNO_RST, HIGH);
        delay(10);
        digitalWrite(PIN_BNO_RST, LOW);
        delay(20);
        digitalWrite(PIN_BNO_RST, HIGH);
        delay(300); // Critical: give BNO085 ARM Cortex-M0 time to boot firmware
    }

    // 2. Initialize I2C Bus
    LOG_INFO("Initializing I2C bus: SDA=GPIO%d, SCL=GPIO%d (400kHz)...", PIN_I2C_SDA, PIN_I2C_SCL);
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);

    // 3. Probe I2C Bus
    bool found0x4A = false;
    bool found0x4B = false;

    Wire.beginTransmission(0x4A);
    if (Wire.endTransmission() == 0) found0x4A = true;

    Wire.beginTransmission(0x4B);
    if (Wire.endTransmission() == 0) found0x4B = true;

    LOG_INFO("I2C Probe: 0x4A=%s, 0x4B=%s", 
             found0x4A ? "ACK (OK)" : "NACK", 
             found0x4B ? "ACK (OK)" : "NACK");

    // If neither ACKed, scan all I2C addresses for diagnostic help
    if (!found0x4A && !found0x4B) {
        LOG_INFO("Scanning all I2C addresses (1-127)...");
        for (uint8_t addr = 1; addr < 127; addr++) {
            Wire.beginTransmission(addr);
            if (Wire.endTransmission() == 0) {
                LOG_INFO("  Found I2C device at address 0x%02X", addr);
            }
        }
    }

    // 4. Connect to BNO085 using pure I2C (pass -1 for INT and RST to avoid SparkFun blocking timeout resets)
    uint8_t addrToUse = found0x4A ? 0x4A : (found0x4B ? 0x4B : 0x4A);
    LOG_INFO("Attempting BNO08x driver initialization at 0x%02X...", addrToUse);

    bool bnoOk = _bno.begin(addrToUse, Wire, -1, -1);
    if (!bnoOk && addrToUse == 0x4A) {
        LOG_INFO("Trying 0x4B fallback...");
        bnoOk = _bno.begin(0x4B, Wire, -1, -1);
        if (bnoOk) addrToUse = 0x4B;
    }

    if (bnoOk) {
        _bnoAddress = addrToUse;
        _usingBno = true;
        LOG_INFO("BNO085 connected successfully at I2C address 0x%02X!", _bnoAddress);

        applyFusionMode();

        delay(50);

        // Fetch initial readings
        for (int i = 0; i < 25; i++) {
            update();
            delay(5);
        }

        center();
        _lastUpdateMicros = micros();
        return true;
    }

    // 5. Fallback to onboard QMI8658 at 0x6B (if present, e.g. on S3 Mini Pro)
    LOG_INFO("BNO085 not detected. Checking for onboard QMI8658 fallback...");
    if (_qmi.begin(Wire, PIN_I2C_SDA, PIN_I2C_SCL, 0x6B)) {
        LOG_INFO("Using onboard QMI8658 IMU");
        _usingBno = false;

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

    LOG_ERROR("NO IMU SENSOR DETECTED! Check connections: SDA=GPIO%d, SCL=GPIO%d, RST=GPIO%d",
              PIN_I2C_SDA, PIN_I2C_SCL, PIN_BNO_RST);
    return false;
}

void HeadTracker::center()
{
    if (_usingBno) {
        // Lock zero quaternion to current absolute 3D orientation
        _zeroQuat = _currentQuat;
        LOG_INFO("HeadTracker Centered via 3D Quaternion transformation!");
    } else {
        _zeroPitch = _pitch;
        _zeroRoll  = _roll;
        _zeroYaw   = _yaw;
        LOG_INFO("HeadTracker Centered: Zero Yaw=%.1f deg, Pitch=%.1f deg, Roll=%.1f deg", _zeroYaw, _zeroPitch, _zeroRoll);
    }
}

void HeadTracker::calibrate()
{
    if (_usingBno) {
        LOG_INFO("BNO085 Tare / Recenter initiated...");
        if (_fusionMode == 0 || _fusionMode == 1) {
            _bno.tareNow(true, SH2_TARE_BASIS_ROTATION_VECTOR);
        } else {
            _bno.tareNow(true, SH2_TARE_BASIS_GAMING_ROTATION_VECTOR);
        }
        center();
    } else if (_qmi.isConnected()) {
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
    if (_usingBno) {
        if (_fusionMode == 0) return "BNO085 (9D-RV)";
        if (_fusionMode == 1) return "BNO085 (ARVR9D)";
        return "BNO085 (6D-GRV)";
    }
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
        dt = 0.02f;
    }

    if (_usingBno) {
        while (_bno.getSensorEvent()) {
            uint8_t id = _bno.sensorValue.sensorId;
            float qr = 1.0f, qi = 0.0f, qj = 0.0f, qk = 0.0f;
            bool gotQuat = false;

            if (id == SH2_ROTATION_VECTOR) {
                qr = _bno.sensorValue.un.rotationVector.real;
                qi = _bno.sensorValue.un.rotationVector.i;
                qj = _bno.sensorValue.un.rotationVector.j;
                qk = _bno.sensorValue.un.rotationVector.k;
                gotQuat = true;
            } else if (id == SH2_ARVR_STABILIZED_RV) {
                qr = _bno.sensorValue.un.arvrStabilizedRV.real;
                qi = _bno.sensorValue.un.arvrStabilizedRV.i;
                qj = _bno.sensorValue.un.arvrStabilizedRV.j;
                qk = _bno.sensorValue.un.arvrStabilizedRV.k;
                gotQuat = true;
            } else if (id == SH2_GAME_ROTATION_VECTOR) {
                qr = _bno.sensorValue.un.gameRotationVector.real;
                qi = _bno.sensorValue.un.gameRotationVector.i;
                qj = _bno.sensorValue.un.gameRotationVector.j;
                qk = _bno.sensorValue.un.gameRotationVector.k;
                gotQuat = true;
            } else if (id == SH2_ARVR_STABILIZED_GRV) {
                qr = _bno.sensorValue.un.arvrStabilizedGRV.real;
                qi = _bno.sensorValue.un.arvrStabilizedGRV.i;
                qj = _bno.sensorValue.un.arvrStabilizedGRV.j;
                qk = _bno.sensorValue.un.arvrStabilizedGRV.k;
                gotQuat = true;
            }

            if (gotQuat) {
                _currentQuat = {qr, qi, qj, qk};
            }
        }

        // Relative 3D rotation: q_rel = q_zero^(-1) * q_current
        // This mathematically eliminates all cross-talk between Pitch, Roll, and Yaw!
        Quat qRel = quatMultiply(quatInverse(_zeroQuat), _currentQuat);
        quatToEuler(qRel, _relPan, _relTilt, _relRoll);

        // Keep absolute orientation for reference
        quatToEuler(_currentQuat, _yaw, _pitch, _roll);

    } else if (_qmi.isConnected()) {
        if (_qmi.update()) {
            float ax = _qmi.getAccelX();
            float ay = _qmi.getAccelY();
            float az = _qmi.getAccelZ();

            float gx = _qmi.getGyroX();
            float gy = _qmi.getGyroY();
            float gz = _qmi.getGyroZ();

            // S3 Mini Pro inverted bottom-mount fix (-az upward normal reference)
            float pitchAcc = atan2f(-ax, -az) * (180.0f / M_PI);
            float rollAcc  = atan2f(ay, -az) * (180.0f / M_PI);

            constexpr float alpha = 0.98f;
            _pitch = alpha * (_pitch + gy * dt) + (1.0f - alpha) * pitchAcc;
            _roll  = alpha * (_roll  + gx * dt) + (1.0f - alpha) * rollAcc;

            _yaw += (-gz) * dt;
            if (_yaw > 180.0f)  _yaw -= 360.0f;
            if (_yaw < -180.0f) _yaw += 360.0f;
        }

        _relTilt = _pitch - _zeroPitch;
        _relRoll = _roll  - _zeroRoll;
        _relPan  = _yaw   - _zeroYaw;
        while (_relPan > 180.0f)  _relPan -= 360.0f;
        while (_relPan < -180.0f) _relPan += 360.0f;
    }

    // Apply axis swap if sensor is mounted rotated 90 degrees
    float pan  = _relPan;
    float tilt = _relTilt;
    float roll = _relRoll;

    if (_swapPitchRoll) {
        float tmp = tilt;
        tilt = roll;
        roll = tmp;
    }

    // Apply axis inversions
    if (_invertPan)  pan  = -pan;
    if (_invertTilt) tilt = -tilt;
    if (_invertRoll) roll = -roll;

    _finalPan  = pan;
    _finalTilt = tilt;
    _finalRoll = roll;

    // Convert relative angles to CRSF values
    _crsfPan  = angleToCrsf(_finalPan,  HT_MAX_ANGLE_PAN,  false);
    _crsfTilt = angleToCrsf(_finalTilt, HT_MAX_ANGLE_TILT, false);
    _crsfRoll = angleToCrsf(_finalRoll, HT_MAX_ANGLE_ROLL, false);
}

void HeadTracker::getChannels16(uint16_t* channels)
{
    // Initialize all channels to neutral center (1500 us / 992)
    for (int i = 0; i < 16; i++) {
        channels[i] = CRSF_CHANNEL_CENTER;
    }

    // Assign configured HeadTracker channels
    channels[HT_CH_PAN]  = _crsfPan;
    channels[HT_CH_TILT] = _crsfTilt;
    channels[HT_CH_ROLL] = _crsfRoll;
}
