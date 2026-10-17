#include "headtracker.h"
#include "logging.h"
#include <math.h>
#if defined(ESP32)
#include <Preferences.h>
#endif

static constexpr uint32_t HT_PREFS_SIGNATURE =
    0x48543031UL ^
    ((uint32_t)(BNO085_FUSION_MODE & 0x0F)) ^
    ((uint32_t)(HT_INVERT_PAN ? 1 : 0) << 4) ^
    ((uint32_t)(HT_INVERT_TILT ? 1 : 0) << 5) ^
    ((uint32_t)(HT_INVERT_ROLL ? 1 : 0) << 6) ^
    ((uint32_t)(HT_SWAP_PITCH_ROLL ? 1 : 0) << 7);

HeadTracker::HeadTracker()
    : _usingBno(false), _bnoAddress(0), _fusionMode(BNO085_FUSION_MODE),
      _swapPitchRoll(HT_SWAP_PITCH_ROLL),
      _invertPan(HT_INVERT_PAN),
      _invertTilt(HT_INVERT_TILT),
      _invertRoll(HT_INVERT_ROLL),
      _lastUpdateMicros(0),
      _lastPollMicros(0),
      _pendingCenter(false),
      _pendingSkipSamples(0),
      _anglesDirty(true),
      _pitch(0), _roll(0), _yaw(0),
      _relPan(0), _relTilt(0), _relRoll(0),
      _finalPan(0), _finalTilt(0), _finalRoll(0),
      _crsfPan(CRSF_CHANNEL_CENTER),
      _crsfTilt(CRSF_CHANNEL_CENTER),
      _crsfRoll(CRSF_CHANNEL_CENTER)
{
    _currentQuat = {1.0f, 0.0f, 0.0f, 0.0f};
    _zeroQuat    = {1.0f, 0.0f, 0.0f, 0.0f};
    _zeroYawInv  = {1.0f, 0.0f, 0.0f, 0.0f};
    _zeroTiltInv = {1.0f, 0.0f, 0.0f, 0.0f};
}

void HeadTracker::loadSettings()
{
#if defined(ESP32)
    Preferences prefs;
    if (prefs.begin("elrs_ht", false)) {
        uint32_t sig = prefs.getUInt("def_sig", 0);
        if (sig == HT_PREFS_SIGNATURE) {
            _fusionMode    = prefs.getUChar("fusion", _fusionMode) % 3;
            _invertPan     = prefs.getBool("inv_pan", _invertPan);
            _invertTilt    = prefs.getBool("inv_tilt", _invertTilt);
            _invertRoll    = prefs.getBool("inv_roll", _invertRoll);
            _swapPitchRoll = prefs.getBool("swap_pr", _swapPitchRoll);
        } else {
            prefs.putUInt("def_sig", HT_PREFS_SIGNATURE);
            prefs.putUChar("fusion", _fusionMode);
            prefs.putBool("inv_pan", _invertPan);
            prefs.putBool("inv_tilt", _invertTilt);
            prefs.putBool("inv_roll", _invertRoll);
            prefs.putBool("swap_pr", _swapPitchRoll);
        }
        prefs.end();
    }
#endif
}

void HeadTracker::saveSettings()
{
#if defined(ESP32)
    Preferences prefs;
    if (prefs.begin("elrs_ht", false)) {
        prefs.putUInt("def_sig", HT_PREFS_SIGNATURE);
        prefs.putUChar("fusion", _fusionMode);
        prefs.putBool("inv_pan", _invertPan);
        prefs.putBool("inv_tilt", _invertTilt);
        prefs.putBool("inv_roll", _invertRoll);
        prefs.putBool("swap_pr", _swapPitchRoll);
        prefs.end();
    }
#endif
}

Quat HeadTracker::quatNormalize(const Quat& q)
{
    float norm = sqrtf(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
    if (norm < 1e-6f) {
        return {1.0f, 0.0f, 0.0f, 0.0f};
    }
    float invNorm = 1.0f / norm;
    return {q.w * invNorm, q.x * invNorm, q.y * invNorm, q.z * invNorm};
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

Quat HeadTracker::eulerToQuat(float yawDeg, float pitchDeg, float rollDeg)
{
    float halfYaw   = (yawDeg   * (M_PI / 180.0f)) * 0.5f;
    float halfPitch = (pitchDeg * (M_PI / 180.0f)) * 0.5f;
    float halfRoll  = (rollDeg  * (M_PI / 180.0f)) * 0.5f;

    float cy = cosf(halfYaw);
    float sy = sinf(halfYaw);
    float cp = cosf(halfPitch);
    float sp = sinf(halfPitch);
    float cr = cosf(halfRoll);
    float sr = sinf(halfRoll);

    Quat q;
    q.w = cr * cp * cy + sr * sp * sy;
    q.x = sr * cp * cy - cr * sp * sy;
    q.y = cr * sp * cy + sr * cp * sy;
    q.z = cr * cp * sy - sr * sp * cy;
    return quatNormalize(q);
}

void HeadTracker::quatToEuler(const Quat& q, float &yaw, float &pitch, float &roll)
{
    Quat nq = quatNormalize(q);
    float qw = nq.w;
    float qx = nq.x;
    float qy = nq.y;
    float qz = nq.z;

    // Roll (intrinsic X''-axis rotation):
    float t0 = +2.0f * (qw * qx + qy * qz);
    float t1 = +1.0f - 2.0f * (qx * qx + qy * qy);
    roll = atan2f(t0, t1) * (180.0f / M_PI);

    // Pitch (intrinsic Y'-axis rotation):
    float t2 = +2.0f * (qw * qy - qz * qx);
    if (t2 > 1.0f) t2 = 1.0f;
    if (t2 < -1.0f) t2 = -1.0f;
    pitch = asinf(t2) * (180.0f / M_PI);

    // Yaw (intrinsic Z-axis rotation):
    float t3 = +2.0f * (qw * qz + qx * qy);
    float t4 = +1.0f - 2.0f * (qy * qy + qz * qz);
    yaw = atan2f(t3, t4) * (180.0f / M_PI);
}

void HeadTracker::toggleInvertTilt()
{
    _invertTilt = !_invertTilt;
    _anglesDirty = true;
    saveSettings();
    recomputeAngles();
    LOG_INFO("Pitch Inversion: %s", _invertTilt ? "INVERTED" : "NORMAL");
}

void HeadTracker::toggleInvertPan()
{
    _invertPan = !_invertPan;
    _anglesDirty = true;
    saveSettings();
    recomputeAngles();
    LOG_INFO("Pan/Yaw Inversion: %s", _invertPan ? "INVERTED" : "NORMAL");
}

void HeadTracker::toggleInvertRoll()
{
    _invertRoll = !_invertRoll;
    _anglesDirty = true;
    saveSettings();
    recomputeAngles();
    LOG_INFO("Roll Inversion: %s", _invertRoll ? "INVERTED" : "NORMAL");
}

void HeadTracker::toggleSwapPitchRoll()
{
    _swapPitchRoll = !_swapPitchRoll;
    _anglesDirty = true;
    saveSettings();
    recomputeAngles();
    LOG_INFO("Pitch/Roll Swap: %s", _swapPitchRoll ? "SWAPPED (90 deg)" : "NORMAL");
}

uint8_t HeadTracker::getActiveReportId() const
{
    if (_fusionMode == 0) return SH2_ROTATION_VECTOR;
    if (_fusionMode == 1) return SH2_ARVR_STABILIZED_RV;
    return SH2_GAME_ROTATION_VECTOR;
}

void HeadTracker::applyFusionMode()
{
    if (!_usingBno) return;

    // Disable inactive rotation vector reports first so multiple streams never collide
    if (_fusionMode != 0) _bno.enableReport(SH2_ROTATION_VECTOR, 0);
    if (_fusionMode != 1) _bno.enableReport(SENSOR_REPORTID_AR_VR_STABILIZED_ROTATION_VECTOR, 0);
    if (_fusionMode != 2) _bno.enableReport(SH2_GAME_ROTATION_VECTOR, 0);

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
    saveSettings();
    applyFusionMode();
    // Defer centering until the new fusion report delivers a fresh quaternion
    _pendingCenter = true;
    _pendingSkipSamples = 2;
}

bool HeadTracker::begin()
{
    loadSettings();
    LOG_INFO("Initializing HeadTracker IMU...");

    // Configure optional INT pin with internal pull-up to prevent floating
    if (PIN_BNO_INT >= 0) {
        pinMode(PIN_BNO_INT, INPUT_PULLUP);
    }

    // 1. Hardware Reset BNO085 if RST pin is configured
    if (PIN_BNO_RST >= 0) {
        LOG_INFO("Hardware reset BNO085 via GPIO%d...", PIN_BNO_RST);
        pinMode(PIN_BNO_RST, OUTPUT);
        digitalWrite(PIN_BNO_RST, HIGH);
        delay(10);
        digitalWrite(PIN_BNO_RST, LOW);
        delay(50);
        digitalWrite(PIN_BNO_RST, HIGH);
        delay(300); // Critical: give BNO085 ARM Cortex-M0+ time to boot firmware
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
            _lastPollMicros = 0; // Force poll during startup warmup
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
            float nz = -az;
            float signNz = (nz >= 0.0f) ? 1.0f : -1.0f;
            _pitch = atan2f(-ax, signNz * sqrtf(ay * ay + nz * nz)) * (180.0f / M_PI);
            _roll  = atan2f(ay, nz) * (180.0f / M_PI);
            _yaw   = 0.0f;
            _currentQuat = eulerToQuat(_yaw, _pitch, _roll);
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
    // Factor current orientation q0 into:
    //   1. Pure World-Z azimuth rotation q_yaw0 (aligned with true gravity up)
    //   2. Sensor/goggle mounting pitch & roll offset q_tilt0 = q_yaw0^(-1) * q0
    // This ensures horizontal head pan rotates strictly around World Gravity Up
    // with zero Roll/Tilt bleed even when the sensor is mounted at an angle on curved goggles.
    _zeroQuat = quatNormalize(_currentQuat);

    float t3 = +2.0f * (_zeroQuat.w * _zeroQuat.z + _zeroQuat.x * _zeroQuat.y);
    float t4 = +1.0f - 2.0f * (_zeroQuat.y * _zeroQuat.y + _zeroQuat.z * _zeroQuat.z);
    float yaw0Rad = atan2f(t3, t4);

    float halfYaw0 = yaw0Rad * 0.5f;
    Quat qYaw0 = {cosf(halfYaw0), 0.0f, 0.0f, sinf(halfYaw0)};
    _zeroYawInv = quatInverse(qYaw0);

    Quat qTilt0 = quatNormalize(quatMultiply(_zeroYawInv, _zeroQuat));
    _zeroTiltInv = quatInverse(qTilt0);

    _pendingCenter = false;
    _pendingSkipSamples = 0;
    _anglesDirty = true;
    recomputeAngles();

    LOG_INFO("HeadTracker Centered (Gravity-Aligned 3D Quaternion: Yaw0=%.1f deg)", yaw0Rad * (180.0f / M_PI));
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
        // Wait for fresh post-tare quaternion report before latching _zeroQuat
        _pendingCenter = true;
        _pendingSkipSamples = 2;
    } else if (_qmi.isConnected()) {
        _qmi.calibrateGyro(150);
        if (_qmi.update()) {
            float ax = _qmi.getAccelX();
            float ay = _qmi.getAccelY();
            float az = _qmi.getAccelZ();
            float nz = -az;
            float signNz = (nz >= 0.0f) ? 1.0f : -1.0f;
            _pitch = atan2f(-ax, signNz * sqrtf(ay * ay + nz * nz)) * (180.0f / M_PI);
            _roll  = atan2f(ay, nz) * (180.0f / M_PI);
        }
        _yaw = 0.0f;
        _currentQuat = eulerToQuat(_yaw, _pitch, _roll);
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

    // Map -maxAngle..+maxAngle symmetrically to CRSF_CHANNEL_MIN..CRSF_CHANNEL_CENTER..CRSF_CHANNEL_MAX
    float normalized = angle / maxAngle; // -1.0 .. +1.0
    float span = (normalized >= 0.0f)
        ? (float)(CRSF_CHANNEL_MAX - CRSF_CHANNEL_CENTER)
        : (float)(CRSF_CHANNEL_CENTER - CRSF_CHANNEL_MIN);
    int32_t crsf = (int32_t)lroundf((float)CRSF_CHANNEL_CENTER + normalized * span);

    if (crsf < CRSF_CHANNEL_MIN) crsf = CRSF_CHANNEL_MIN;
    if (crsf > CRSF_CHANNEL_MAX) crsf = CRSF_CHANNEL_MAX;

    return (uint16_t)crsf;
}

void HeadTracker::recomputeAngles()
{
    // Gravity-aligned 3D relative rotation:
    //   q_rel = q_yaw0^(-1) * q_current * q_tilt0^(-1)
    // Left-multiplying by q_yaw0^(-1) zeroes azimuth around true World Gravity Up (+Z),
    // while right-multiplying by q_tilt0^(-1) removes the sensor's mounting tilt/roll on the goggles.
    Quat qTemp = quatMultiply(_zeroYawInv, _currentQuat);
    Quat qRel  = quatMultiply(qTemp, _zeroTiltInv);

    // If sensor is mounted rotated 90 degrees around Z, perform SO(3) similarity transform
    // in quaternion space BEFORE Z-Y'-X'' Euler decomposition so gimbal order (Pan -> Tilt -> Roll)
    // stays physically aligned with the head axes.
    if (_swapPitchRoll) {
        Quat qSwapped = {qRel.w, -qRel.y, qRel.x, qRel.z};
        quatToEuler(qSwapped, _relPan, _relTilt, _relRoll);
        _relRoll = -_relRoll;
    } else {
        quatToEuler(qRel, _relPan, _relTilt, _relRoll);
    }

    // Keep absolute world orientation for diagnostics
    quatToEuler(_currentQuat, _yaw, _pitch, _roll);

    float pan  = _relPan;
    float tilt = _relTilt;
    float roll = _relRoll;

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

    _anglesDirty = false;
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
        // Poll immediately if hardware INT pin is asserted (active LOW),
        // or every 2ms (500Hz) for non-blocking polling without I2C bus saturation.
        bool intAsserted = (PIN_BNO_INT >= 0) && (digitalRead(PIN_BNO_INT) == LOW);
        if (!intAsserted && (now - _lastPollMicros < 2000)) {
            if (_anglesDirty) {
                recomputeAngles();
            }
            return;
        }
        _lastPollMicros = now;

        // Recover automatically if BNO085 experienced an internal reset or brown-out
        if (_bno.wasReset()) {
            LOG_WARN("BNO085 reset detected — re-enabling sensor reports");
            applyFusionMode();
        }

        uint8_t expectedId = getActiveReportId();
        bool gotQuat = false;

        while (_bno.getSensorEvent()) {
            uint8_t id = _bno.sensorValue.sensorId;
            if (id != expectedId) {
                continue;
            }

            float qr = 1.0f, qi = 0.0f, qj = 0.0f, qk = 0.0f;
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
            }

            if (gotQuat) {
                _currentQuat = {qr, qi, qj, qk};
            }
        }

        if (gotQuat) {
            if (_pendingCenter) {
                if (_pendingSkipSamples > 0) {
                    _pendingSkipSamples--;
                } else {
                    center();
                    return;
                }
            }
            recomputeAngles();
        } else if (_anglesDirty) {
            recomputeAngles();
        }

    } else if (_qmi.isConnected()) {
        if (_qmi.update()) {
            float ax = _qmi.getAccelX();
            float ay = _qmi.getAccelY();
            float az = _qmi.getAccelZ();

            float gx = _qmi.getGyroX();
            float gy = _qmi.getGyroY();
            float gz = _qmi.getGyroZ();

            // S3 Mini Pro inverted bottom-mount fix (-az upward normal reference)
            float nz = -az;
            float signNz = (nz >= 0.0f) ? 1.0f : -1.0f;
            float pitchAcc = atan2f(-ax, signNz * sqrtf(ay * ay + nz * nz)) * (180.0f / M_PI);
            float rollAcc  = atan2f(ay, nz) * (180.0f / M_PI);

            constexpr float alpha = 0.98f;
            _pitch = alpha * (_pitch + gy * dt) + (1.0f - alpha) * pitchAcc;
            _roll  = alpha * (_roll  + gx * dt) + (1.0f - alpha) * rollAcc;

            _yaw += (-gz) * dt;
            if (_yaw > 180.0f)  _yaw -= 360.0f;
            if (_yaw < -180.0f) _yaw += 360.0f;

            _currentQuat = eulerToQuat(_yaw, _pitch, _roll);
            recomputeAngles();
        } else if (_anglesDirty) {
            recomputeAngles();
        }
    }
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

