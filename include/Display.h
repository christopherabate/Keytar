#pragma once

#include <U8g2lib.h>
#include "Images.h"

/**
 * @brief   OLED display manager for instruments, controls, and temporary popups.
 * 
 * Provides:
 *  - a non-blocking startup sequence with an animated image followed by a splash screen,
 *  - main display for instruments and controls,
 *  - temporary popups for control values or effect states.
 * 
 * Designed as a singleton (`display`). Display functions do nothing until the startup
 * sequence is finished. Locks are used to prevent overwriting temporary popups.
 */
class DisplayClass {
public:
    /**
     * @brief   Execute the non-blocking startup sequence.
     * 
     * Sequence:
     *  1. Animate `keytar_img` moving upward from bottom of screen.
     *  2. Display `splashscreen_img` for 2 seconds.
     * 
     * After the splash screen duration, the startup is considered finished (`started = true`).
     * Can be called repeatedly in `loop()` without blocking execution.
     */
    void startup() {
        uint32_t start = millis();

        // Initial display setup at first call
        if (!started && timer == 0) {
            timer = start;
            u8g2.begin();
            u8g2.setFont(u8g2_font_6x10_tf);
            SCREEN_W = u8g2.getDisplayWidth();
            SCREEN_H = u8g2.getDisplayHeight();
        }

        if (!started) {
            u8g2.clearBuffer();

            static int OFFSET = SCREEN_H; // Vertical position for animation

            if (OFFSET > -SCREEN_H) {
                // Draw moving image for startup animation
                u8g2.drawXBMP(0, OFFSET, SCREEN_W, SCREEN_H, keytar_img);
                OFFSET -= 5; // fixed vertical step per frame
            } else {
                // Draw splash screen for 2 seconds after animation completes
                u8g2.drawXBMP(0, 0, SCREEN_W, SCREEN_H, splashscreen_img);
                if (start - timer > 3000) started = true; // mark startup as finished
            }

            u8g2.sendBuffer();
        }
    }

    /**
     * @brief   Attach an instrument for display on the main page.
     * 
     * @tparam Instrument  Type of instrument object
     * @param instrument   Reference to the instrument
     * 
     * Stores callbacks to query the instrument's channel and octave dynamically.
     */
    template <typename Instrument>
    void attachInstrument(Instrument &instrument) {
        InstrumentDisplay instrumentDisplay;
        instrumentDisplay.octave  = [&instrument]() -> int { return instrument.transposer.getSelection(); };
        instrumentDisplay.channel = [&instrument]() -> int { return instrument.channel.getSelection(); };
        instruments.push_back(std::move(instrumentDisplay));
    }

    /**
     * @brief   Attach a control for display on the main page.
     * 
     * @tparam Control  Type of control object
     * @param control   Reference to the control
     * 
     * Stores callbacks to query the control value and static label/info strings.
     */
    template <typename Control>
    void attachControl(Control &control) {
        ControlDisplay controlDisplay;
        controlDisplay.value = [&control]() -> int { return control.getValue(); };
        controlDisplay.label = control.label;
        controlDisplay.info  = control.info;
        controls.push_back(std::move(controlDisplay));
    }

    /**
     * @brief   Draw the main page showing all attached instruments and controls.
     * 
     * Does nothing if the startup sequence is not finished or a lock is active.
     * Instruments show channel and octave info. Controls show value bars with labels.
     */
    void main() {
        if (!started || locked()) return;

        u8g2.clearBuffer();

        int MARGIN_X = 4;
        int MARGIN_Y = 6;
        int CONTROL_H = 14;
        int CONTROL_Y = 0;

        // Draw all controls
        for (const auto& control : controls) {
            u8g2.drawStr(0, CONTROL_Y + 8, String(control.label).c_str());
            u8g2.drawRFrame(0, CONTROL_Y + 8 + (MARGIN_Y / 2), SCREEN_W, CONTROL_H - 8, 1);
            u8g2.drawBox(2, CONTROL_Y + 8 + (MARGIN_Y / 2) + 2, map(control.value(), 0, 127, 0, SCREEN_W - 4), CONTROL_H - 8 - 4);
            CONTROL_Y += CONTROL_H + MARGIN_Y;
        }

        int INSTRUMENT_W = 36;
        int INSTRUMENT_H = 18;
        int OCTAVE = 4;
        int OCTAVER_H = 8;
        int OCTAVER_W = OCTAVE * 5 + 2;

        int INSTRUMENT_Y = SCREEN_H - (INSTRUMENT_H * 2) - MARGIN_Y;

        // Draw all instruments
        for (const auto& instrument : instruments) {
            u8g2.drawXBMP(0, INSTRUMENT_Y, INSTRUMENT_W, INSTRUMENT_H, instrument_imgs[instrument.channel()]);
            u8g2.drawStr(INSTRUMENT_W + MARGIN_X, INSTRUMENT_Y + 7, (String("Ch") + (String(instrument.channel() + 1).length() < 2 ? " " : "") + String(instrument.channel() + 1)).c_str());
            u8g2.drawRFrame(INSTRUMENT_W + MARGIN_X, INSTRUMENT_Y + 10, OCTAVER_W, OCTAVER_H, 1);
            u8g2.drawRBox(INSTRUMENT_W + MARGIN_X + (instrument.octave() * 4) + 1, INSTRUMENT_Y + 10 + 2, OCTAVE, OCTAVER_H - 4, 1);
            INSTRUMENT_Y += INSTRUMENT_H + MARGIN_Y;
        }

        u8g2.sendBuffer();
    }

    /**
     * @brief   Display a temporary popup for a control value (0–127).
     * 
     * @param label  Control label
     * @param info   Optional descriptive text
     * @param value  Current value of the control
     * 
     * Activates an internal lock to prevent other displays from overwriting it immediately.
     */
    void control(const char* label, const char* info, uint8_t value) {
        if (!started) return;

        u8g2.clearBuffer();

        int CONTROL_H = 30;
        int CONTROL_W = 8;

        u8g2.drawStr((SCREEN_W - u8g2.getStrWidth(String(label).c_str())) / 2, 26, String(label).c_str());
        u8g2.drawStr((SCREEN_W - u8g2.getStrWidth((String(map(value, 0, 127, 0, 100)) + "%").c_str())) / 2, SCREEN_H / 2 + 4, (String(map(value, 0, 127, 0, 100)) + "%").c_str());
        u8g2.drawStr((SCREEN_W - u8g2.getStrWidth(String(info).c_str())) / 2, SCREEN_H - 18, String(info).c_str());
        u8g2.drawCircle(CONTROL_H, SCREEN_H / 2, CONTROL_H - CONTROL_W);

        for(int radius = CONTROL_H - CONTROL_W; radius <= CONTROL_H; radius++) {
            if(value > 0) u8g2.drawArc(CONTROL_H, SCREEN_H / 2, radius, 192 - (uint8_t)(map(value, 0, 127, 0, 360) * 255L / 360L), 192);
        }

        u8g2.sendBuffer();
        lock(1000);
    }

    /**
     * @brief   Display a temporary popup for an effect state (ON/OFF).
     * 
     * @param label  Effect label
     * @param info   Optional descriptive text
     * @param value  Current effect state (true = ON, false = OFF)
     * 
     * Activates an internal lock to prevent other displays from overwriting it immediately.
     */
    void effect(const char* label, const char* info, bool value) {
        if (!started) return;

        u8g2.clearBuffer();

        int EFFECT_H = 48;
        int EFFECT_W = 60;

        u8g2.drawStr((SCREEN_W - u8g2.getStrWidth(String(label).c_str())) / 2, 26, String(label).c_str());
        if(value) u8g2.drawRBox((SCREEN_W - EFFECT_W) / 2, (SCREEN_H - EFFECT_H) / 2, EFFECT_W, EFFECT_H, 7);
        else u8g2.drawRFrame((SCREEN_W - EFFECT_W) / 2, (SCREEN_H - EFFECT_H) / 2, EFFECT_W, EFFECT_H, 7);

        u8g2.setDrawColor(!value);
        u8g2.drawStr(SCREEN_W / 2 - u8g2.getStrWidth(String(value ? "ON" : "OFF").c_str()) / 2, SCREEN_H / 2 + 4, String(value ? "ON" : "OFF").c_str());
        u8g2.setDrawColor(1);
        u8g2.drawStr((SCREEN_W - u8g2.getStrWidth(String(info).c_str())) / 2, SCREEN_H - 18, String(info).c_str());

        u8g2.sendBuffer();
        lock(1000);
    }

private:
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2{U8G2_R3, U8X8_PIN_NONE};

    int SCREEN_W;
    int SCREEN_H;

    struct InstrumentDisplay {
        std::function<int()> channel;   ///< Callback to get instrument channel
        std::function<int()> octave;    ///< Callback to get instrument octave
    };

    struct ControlDisplay {
        std::function<int()> value;     ///< Callback to get control value
        const char* label;              ///< Label for control
        const char* info;               ///< Optional info text
    };

    std::vector<InstrumentDisplay> instruments;
    std::vector<ControlDisplay> controls;

    uint32_t timer = 0;                 ///< General-purpose timer for locks and splash timing
    uint32_t duration = 1000;           ///< Duration of current lock
    bool started = false;    ///< Flag indicating whether startup is complete

    /// Check if display is currently locked
    bool locked() const { return (millis() - timer) < duration; }
    /// Lock the display for a given duration (ms)
    void lock(uint32_t ms = 1000) { timer = millis(); duration = ms; }
};

/**
 * @brief   Global singleton instance of DisplayClass.
 */
inline DisplayClass display;
