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

    // Runtime axis inversion and swap toggles
    void toggleInvertTilt();
    void toggleInvertPan();
    void toggleInvertRoll();
    void toggleSwapPitchRoll();

    bool getInvertTilt() const { return _invertTilt; }
    bool getInvertPan()  const { return _invertPan; }
    bool getInvertRoll() const { return _invertRoll; }
    bool getSwapPitchRoll() const { return _swapPitchRoll; }

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

    // Fill all 16 trainer channels (Pan=CH0/TR1, Tilt=CH1/TR2, Roll=CH2/TR3)
    void getChannels16(uint16_t* channels);

    bool isSensorConnected() const;
    const char* getSensorName() const;

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

    uint32_t _lastUpdateMicros;

    // 3D Quaternions for cross-talk free relative orientation
    Quat     _currentQuat;
    Quat     _zeroQuat;

    // Absolute orientation (degrees)
    float _pitch;
    float _roll;
    float _yaw;

    // Zero / Center reference for QMI fallback
    float _zeroPitch;
    float _zeroRoll;
    float _zeroYaw;

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

    static Quat quatMultiply(const Quat& q1, const Quat& q2);
    static Quat quatInverse(const Quat& q);
    static void quatToEuler(const Quat& q, float &yaw, float &pitch, float &roll);
};
