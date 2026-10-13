#pragma once
#include <Arduino.h>
#include "qmi8658.h"
#include "my_config.h"

class HeadTracker
{
public:
    HeadTracker();

    bool begin();
    void update();

    // Re-center all axes to current head position
    void center();

    // Re-calibrate gyro bias (keep still for ~1.5s)
    void calibrate();

    // Current relative angles in degrees
    float getPan()  const { return _relPan; }
    float getTilt() const { return _relTilt; }
    float getRoll() const { return _relRoll; }

    // Current CRSF 11-bit channel values (172 - 1811)
    uint16_t getCrsfPan()  const { return _crsfPan; }
    uint16_t getCrsfTilt() const { return _crsfTilt; }
    uint16_t getCrsfRoll() const { return _crsfRoll; }

    // Fill all 16 trainer channels (Pan=CH0/TR1, Tilt=CH1/TR2, Roll=CH2/TR3)
    void getChannels16(uint16_t* channels);

    bool isSensorConnected() const;
    const char* getSensorName() const;

private:
    QMI8658 _qmi;
    bool _usingBno;

    uint32_t _lastUpdateMicros;

    // Fused absolute orientation (degrees)
    float _pitch;
    float _roll;
    float _yaw;

    // Zero / Center reference
    float _zeroPitch;
    float _zeroRoll;
    float _zeroYaw;

    // Relative angles
    float _relPan;
    float _relTilt;
    float _relRoll;

    // Output CRSF values
    uint16_t _crsfPan;
    uint16_t _crsfTilt;
    uint16_t _crsfRoll;

    uint16_t angleToCrsf(float angle, float maxAngle, bool invert);
};
