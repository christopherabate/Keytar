#pragma once

#include <AH/Hardware/Button.hpp>
#include <MIDI_Senders/DigitalCCSender.hpp>
#include "Adapters/CustomMIDIButtonLatched.h"

BEGIN_CS_NAMESPACE

/**
 * @brief   Represents a latched MIDI On/Off effect with automatic display and labels.
 * 
 * This class inherits from `CustomMIDIButtonLatched<DigitalCCSender>` and provides:
 *  - toggle handling on each button press,
 *  - sending MIDI CC On/Off messages,
 *  - automatic popup display via `display`.
 * 
 * Additionally, it adds:
 *  - a readable label (`label`),
 *  - a descriptive text (`info`).
 */
class Effect : public CustomMIDIButtonLatched<DigitalCCSender> {
public:
    /**
     * @brief   Constructs a latched MIDI CC effect.
     * 
     * @param   pin
     *          Pin number of the physical button.
     * @param   led
     *          Pin number of the led.
     * @param   address
     *          MIDI CC address to send messages.
     * @param   label
     *          Displayable name of the effect (e.g., "Chorus").
     * @param   info
     *          Optional descriptive text for the effect. Default is empty string.
     * @param   sender
     *          Optional MIDI sender object. Default is empty.
     */
    Effect(pin_t pin, pin_t led, MIDIAddress address,
           const char* label,
           const char* info = "",
           const DigitalCCSender &sender = {})
        : CustomMIDIButtonLatched<DigitalCCSender>(pin, address, sender),
          led(led),
          label(label),
          info(info)
        {
            pinMode(led, OUTPUT);
            digitalWrite(led, LOW);
        }

    /**
     * @brief   Called continuously to update the button and display the effect state.
     * 
     * Overrides the parent `update()` method to handle:
     *  - toggling the latched state on button press,
     *  - automatically displaying the current state via `display.effect()`.
     */

    void update() override {
        // Update the physical button state
        Button::State state = button.update();

        // On button press (Falling edge), toggle state and display popup
        if (state == Button::Falling) {
            bool current = toggleState();           // Toggle the latched state
            analogWrite(led, current ? 20 : 0);     // Toggle the led
            display.effect(label, info, current);   // Automatic display
        }
    }

public:
    const pin_t led; ///< Led pin
    const char* label; ///< Displayable name of the effect
    const char* info;  ///< Optional descriptive text
};

END_CS_NAMESPACE
