#pragma once
#include <SparkFun_BNO080_Arduino_Library.h>

/**
 * @brief   IMU sensor manager (BNO085 - SparkFun).
 *
 * Provides:
 *  - non-blocking update at fixed interval (10ms for vibrato precision),
 *  - pitch (angle X relative to ground),
 *  - acceleration on X/Y/Z axis (configurable),
 *  - vibrato detection via RMS windowing → Pitch Bend value.
 *
 * Designed as a singleton (`sensor`). Must call `begin()` once in setup.
 */
class SensorClass {
public:

    // ─── Axe de détection vibrato ─────────────────────────────────────────────
    enum class Axis { X, Y, Z };

    // ─── Configuration vibrato ────────────────────────────────────────────────
    Axis  vibratoAxis      = Axis::X;  ///< Axe utilisé pour la détection — à calibrer
    float vibratoThreshold = 1.5f;    ///< RMS min pour activer (m/s²) — à calibrer
    float vibratoMaxDepth  = 0.5f;    ///< Profondeur max en demi-tons
    float vibratoSmooth    = 0.15f;   ///< Lissage retour au centre [0..1]

    // ─── Init ─────────────────────────────────────────────────────────────────

    /**
     * @brief   Initialize the IMU sensor.
     *
     * Uses default I2C address (0x4B). Blocks if sensor not found.
     */
    void begin(uint8_t address = BNO080_DEFAULT_ADDRESS) {
        Wire.begin();
        if (!imu.begin(address, Wire)) {
            Serial.println("Erreur : BNO085 non trouvé !");
            while (1);
        }
        Wire.setClock(400000);
        imu.enableRotationVector(50);
        imu.enableAccelerometer(10);  // 10ms pour meilleure résolution vibrato
        lastUpdate = millis();
        started = true;
    }

    // ─── Update ───────────────────────────────────────────────────────────────

    /**
     * @brief   Non-blocking update. Must be called repeatedly in loop().
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
                updateVibrato(getAxisValue());
            }
        }
    }

    // ─── Getters ──────────────────────────────────────────────────────────────

    float getPitch()  const { return pitch; }
    float getAccelX() const { return accelX; }
    float getAccelY() const { return accelY; }
    float getAccelZ() const { return accelZ; }
    bool  ready()     const { return started; }

    /**
     * @brief   Valeur de l'axe actuellement sélectionné pour le vibrato.
     */
    float getAxisValue() const {
        switch (vibratoAxis) {
            case Axis::Y: return accelY;
            case Axis::Z: return accelZ;
            default:      return accelX;
        }
    }

    /**
     * @brief   Vibrato actif (RMS au-dessus du seuil) ?
     */
    bool isVibratoActive() const { return vibratoActive; }

    /**
     * @brief   Valeur Pitch Bend calculée [0..16383], centre = 8192.
     *          Retourne progressivement 8192 si vibrato inactif.
     */
    int16_t getVibratoPitchBend() const {
        return (int16_t)constrain(_smoothedPB, 0.0f, 16383.0f);
    }

    /**
     * @brief   Affiche les 3 axes + RMS + état vibrato sur Serial.
     *          Appeler depuis loop() pendant la calibration, commenter ensuite.
     */
    void debugAxes() const {
        Serial.print("X: ");     Serial.print(accelX, 2);
        Serial.print(" | Y: ");  Serial.print(accelY, 2);
        Serial.print(" | Z: ");  Serial.print(accelZ, 2);
        Serial.print(" | RMS("); Serial.print(axisName()); Serial.print("): ");
        Serial.print(lastRMS, 2);
        Serial.print(" | Vibrato: "); Serial.println(vibratoActive ? "ON" : "off");
    }

private:

    // ─── Capteur ──────────────────────────────────────────────────────────────
    BNO080      imu;
    float       pitch   = 0.0f;
    float       accelX  = 0.0f;
    float       accelY  = 0.0f;
    float       accelZ  = 0.0f;
    uint32_t    lastUpdate = 0;
    const uint32_t interval = 10;   // ms
    bool        started = false;

    // ─── Vibrato ──────────────────────────────────────────────────────────────
    static constexpr uint8_t RMS_WINDOW = 50; ///< ~500ms à 10ms/sample
    float   _rmsBuffer[RMS_WINDOW] = {};
    uint8_t _rmsIdx       = 0;
    float   _smoothedPB   = 8192.0f;
    float   lastRMS       = 0.0f;
    bool    vibratoActive = false;

    const char* axisName() const {
        switch (vibratoAxis) {
            case Axis::Y: return "Y";
            case Axis::Z: return "Z";
            default:      return "X";
        }
    }

    /**
     * @brief   Calcule le RMS sur fenêtre glissante et met à jour le Pitch Bend.
     * @param   a   Accélération instantanée sur l'axe sélectionné (m/s²)
     */
    void updateVibrato(float a) {
        // 1. Fenêtre RMS circulaire
        _rmsBuffer[_rmsIdx % RMS_WINDOW] = a * a;
        _rmsIdx++;
        float sum = 0.0f;
        for (auto v : _rmsBuffer) sum += v;
        lastRMS = sqrtf(sum / RMS_WINDOW);

        // 2. Seuil d'activation
        vibratoActive = (lastRMS > vibratoThreshold);

        float targetPB;
        if (vibratoActive) {
            // 3. Profondeur : RMS [threshold..threshold*3] → [0..maxDepth semitones]
            float depth = constrain(
                (lastRMS - vibratoThreshold) / (vibratoThreshold * 2.0f) * vibratoMaxDepth,
                0.0f, vibratoMaxDepth
            );
            // 4. Direction : signe de a → montée ou descente du pitch
            float sign = (a >= 0.0f) ? 1.0f : -1.0f;
            // 5. Conversion : 1 semitone = 4096 unités PB (range ±2 semitones = ±8192)
            targetPB = 8192.0f + sign * depth * 4096.0f;
        } else {
            // Retour progressif au centre
            targetPB = 8192.0f;
        }

        // 6. Lissage exponentiel pour éviter les sauts brutaux
        _smoothedPB += vibratoSmooth * (targetPB - _smoothedPB);
    }
};

/**
 * @brief   Global singleton instance
 */
inline SensorClass sensor;
