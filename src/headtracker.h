#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <SparkFun_BNO08x_Arduino_Library.h>
#include "qmi8658.h"
#include "my_config.h"

struct Quat {
    float w, x, y, z;
};

class HeadTracker
{
public:
    HeadTracker();

    bool begin();
    void update();

    // Re-center all axes to current head position using 3D quaternion transformation
    void center();

    // Re-calibrate / tare
    void calibrate();

    // IMU fusion mode (0=9DOF RV, 1=ARVR 9DOF, 2=Game 6DOF)
    void setFusionMode(uint8_t mode);
    uint8_t getFusionMode() const { return _fusionMode; }

    // Runtime axis inversion and swap toggles & setters
    void toggleInvertTilt();
    void toggleInvertPan();
    void toggleInvertRoll();
    void toggleSwapPitchRoll();

    void setInvertTilt(bool inv);
    void setInvertPan(bool inv);
    void setInvertRoll(bool inv);
    void setSwapPitchRoll(bool swap);

    bool getInvertTilt() const { return _invertTilt; }
    bool getInvertPan()  const { return _invertPan; }
    bool getInvertRoll() const { return _invertRoll; }
    bool getSwapPitchRoll() const { return _swapPitchRoll; }

    // Configurable max deflection angles (degrees)
    void setMaxAngles(float maxPan, float maxTilt, float maxRoll);
    float getMaxAnglePan()  const { return _maxAnglePan; }
    float getMaxAngleTilt() const { return _maxAngleTilt; }
    float getMaxAngleRoll() const { return _maxAngleRoll; }

    // Configurable EdgeTX Trainer channel mapping (0..15 = TR1..TR16)
    void setChannelMapping(uint8_t chPan, uint8_t chTilt, uint8_t chRoll);
    uint8_t getChPan()  const { return _chPan; }
    uint8_t getChTilt() const { return _chTilt; }
    uint8_t getChRoll() const { return _chRoll; }

    // Current relative angles in degrees
    float getPan()  const { return _finalPan; }
    float getTilt() const { return _finalTilt; }
    float getRoll() const { return _finalRoll; }

    // Absolute orientation in degrees (world frame)
    float getAbsPitch() const { return _pitch; }
    float getAbsRoll()  const { return _roll; }
    float getAbsYaw()   const { return _yaw; }

    // Current CRSF 11-bit channel values (172 - 1811)
    uint16_t getCrsfPan()  const { return _crsfPan; }
    uint16_t getCrsfTilt() const { return _crsfTilt; }
    uint16_t getCrsfRoll() const { return _crsfRoll; }

    // Fill all 16 trainer channels (Pan=TR1, Tilt=TR2, Roll=TR3 by default)
    void getChannels16(uint16_t* channels);

    bool isSensorConnected() const;
    const char* getSensorName() const;

    void loadSettings();
    void saveSettings();

    // ExpressLRS Binding Phrase MD5 -> 6-byte UID helper & NVS network config persistence
    static void computeElrsUid(const char* phrase, uint8_t outUid[6]);
    static bool isUidNonZero(const uint8_t uid[6]);
    static bool loadNetworkConfig(uint8_t outUid[6], String &outPhrase, int8_t &outTxPower);
    static void saveNetworkConfig(const uint8_t uid[6], const char* phrase, int8_t txPower);

private:
    BNO08x   _bno;
    QMI8658  _qmi;
    bool     _usingBno;
    uint8_t  _bnoAddress;
    uint8_t  _fusionMode;

    // Runtime orientation options
    bool     _swapPitchRoll;
    bool     _invertPan;
    bool     _invertTilt;
    bool     _invertRoll;

    // Max deflection angles (degrees)
    float    _maxAnglePan;
    float    _maxAngleTilt;
    float    _maxAngleRoll;

    // Trainer channel assignment (0..15)
    uint8_t  _chPan;
    uint8_t  _chTilt;
    uint8_t  _chRoll;

    uint32_t _lastUpdateMicros;
    uint32_t _lastPollMicros;

    // 3D Quaternions for gravity-aligned, cross-talk free relative orientation:
    // q_rel = _zeroYawInv * _currentQuat * _zeroTiltInv
    Quat     _currentQuat;
    Quat     _zeroQuat;
    Quat     _zeroYawInv;
    Quat     _zeroTiltInv;

    // Async tare / mode-switch centering state
    bool     _pendingCenter;
    uint8_t  _pendingSkipSamples;
    bool     _anglesDirty;

    // Absolute orientation (degrees, world frame)
    float _pitch;
    float _roll;
    float _yaw;

    // Relative angles (head frame)
    float _relPan;
    float _relTilt;
    float _relRoll;

    // Final conditioned angles after inversion and axis swap
    float _finalPan;
    float _finalTilt;
    float _finalRoll;

    // Output CRSF values
    uint16_t _crsfPan;
    uint16_t _crsfTilt;
    uint16_t _crsfRoll;

    uint16_t angleToCrsf(float angle, float maxAngle, bool invert);
    void applyFusionMode();
    void recomputeAngles();
    uint8_t getActiveReportId() const;

    static Quat quatNormalize(const Quat& q);
    static Quat quatMultiply(const Quat& q1, const Quat& q2);
    static Quat quatInverse(const Quat& q);
    static Quat eulerToQuat(float yawDeg, float pitchDeg, float rollDeg);
    static void quatToEuler(const Quat& q, float &yaw, float &pitch, float &roll);
};
