#pragma once

#include <FastLED.h>

/**
 * @brief   LED light manager for visual feedback on key press or instrument events.
 * 
 * Provides:
 *  - non-blocking brightness boost triggered by external events (e.g., key press),
 *  - gradual decay of brightness back to idle,
 *  - simple global instance `light` for easy access.
 */
class LightClass {
public:
    /**
     * @brief   Initialize the LED strip.
     * 
     * Configures FastLED with WS2812B LEDs, sets initial fill color to white,
     * disables dithering, applies initial brightness, and updates the strip.
     * Should be called once during setup.
     */
    void begin() {
        FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
        fill_solid(leds, NUM_LEDS, CRGB::White);
        FastLED.setDither(false);
        FastLED.setBrightness(brightness);
        FastLED.show();
    }

    /**
     * @brief   Trigger a progressive brightness boost.
     * 
     * Stores the current brightness and marks the boost as active.
     * Intended to be called when an external event occurs (e.g., a button press).
     * 
     * @param velocity  Value representing intensity of the event (currently unused in calculation).
     */
    void boost(uint8_t velocity) {
        boosting = true;
        time = millis();                      // Memorize the start time of the boost
        boostStartBrightness = brightness;    // Memorize the current brightness
    }

    /**
     * @brief   Update loop for managing LED brightness.
     * 
     * Should be called repeatedly in the main loop.
     * Handles non-blocking interpolation of brightness:
     *  - If boosting: gradually increases brightness up to `boostBrightness`.
     *  - Otherwise: gradually decays brightness back toward zero.
     */
    void idle() {

        unsigned long elapsed = millis() - time;

        if (boosting && elapsed < duration) {
            // Linear interpolation from current brightness to boost target
            brightness = boostStartBrightness + (boostBrightness - boostStartBrightness) * elapsed / duration;
        } else {
            boosting = false;
            // Gradual decay when not boosting
            brightness = (brightness * 7.5) / 10;
        }

        FastLED.setBrightness(brightness);
        FastLED.show();
    }

private:
    static constexpr uint16_t NUM_LEDS = 194;       ///< Total number of LEDs
    static constexpr uint8_t LED_PIN = 24;         ///< Data pin connected to the LEDs

    CRGB leds[NUM_LEDS];                           ///< LED buffer for FastLED

    uint8_t brightness = 0;                         ///< Current LED brightness
    uint8_t boostStartBrightness = 0;              ///< Brightness at start of boost
    static constexpr uint8_t boostBrightness = 128; ///< Maximum brightness during boost

    bool boosting = false;                          ///< True if a boost is in progress
    unsigned long time = 0;                         ///< Timestamp when boost started
    static constexpr unsigned long duration = 100; ///< Duration of the boost in milliseconds
};

/**
 * @brief   Global singleton instance of LightClass.
 * 
 * Provides convenient access to methods: `light.begin()`, `light.boost()`, `light.idle()`.
 */
inline LightClass light;