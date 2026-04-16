#pragma once

#include <SparkFun_BNO080_Arduino_Library.h>

/**
 * @brief   IMU sensor manager (BNO085 - SparkFun).
 *
 * Provides:
 *  - non-blocking update at fixed interval (50ms),
 *  - pitch (angle X relative to ground),
 *  - acceleration on X axis.
 *
 * Designed as a singleton (`sensor`). Must call `begin()` once in setup.
 */
class SensorClass {
public:

    /**
     * @brief   Initialize the IMU sensor.
     *
     * Uses default I2C address (0x4B). Can block if sensor not found.
     */
    void begin(uint8_t address = BNO080_DEFAULT_ADDRESS) {
        Wire.begin();

        // Attempt begin with address
        if (!imu.begin(address, Wire)) {
            Serial.println("Erreur : BNO085 non trouvé !");
            while (1);
        }

        // Set I2C high speed
        Wire.setClock(400000);

        // Enable rotation vector (Euler angles + quaternion)
        imu.enableRotationVector(50);

        // Also enable accelerometer for raw accel data
        imu.enableAccelerometer(10);

        lastUpdate = millis();
        started = true;
    }

    /**
     * @brief   Non-blocking update.
     *
     * Must be called repeatedly in loop().
     */
    void update() {
        if (!started) return;

        uint32_t now = millis();
        if (now - lastUpdate >= interval) {
            lastUpdate = now;

            if (imu.dataAvailable()) {
                pitch  = imu.getPitch() * 180.0f / PI;
                accelX = imu.getAccelX();
                accelY = imu.getAccelY();
                accelZ = imu.getAccelZ();
            }
        }
    }

    /**
     * @brief   Get current pitch (deg)
     */
    float getPitch() const { return pitch; }

    /**
     * @brief   Get current acceleration X Y Z (m/s²)
     */
    float getAccelX() const { return accelX; }
    float getAccelY() const { return accelY; }
    float getAccelZ() const { return accelZ; }

    /**
     * @brief   Check if sensor initialized
     */
    bool ready() const { return started; }

private:
    BNO080 imu;

    float pitch  = 0.0f;
    float accelX  = 0.0f;
    float accelY  = 0.0f;
    float accelZ  = 0.0f;

    uint32_t lastUpdate = 0;
    const uint32_t interval = 10;
    bool started = false;
};

/**
 * @brief   Global singleton instance
 */
inline SensorClass sensor;