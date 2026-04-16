#pragma once

#include <MIDI_Outputs/Abstract/MIDIAbsoluteEncoder.hpp>
#include <MIDI_Senders/ContinuousCCSender.hpp>

BEGIN_CS_NAMESPACE

/**
 * @brief   Represents a MIDI control based on an absolute rotary encoder.
 * 
 * This class inherits from `MIDIAbsoluteEncoder<ContinuousCCSender>` and adds:
 *  - a label (`label`) to identify the controlled parameter or effect;
 *  - an optional descriptive text (`info`);
 *  - a mandatory physical button used to modify encoder speed.
 * 
 * When the button is pressed, the encoder speed is multiplied to allow faster adjustments.
 */
class Control : public MIDIAbsoluteEncoder<ContinuousCCSender> {
public:
    /**
     * @brief   Construct a new Control object with an absolute encoder and a button.
     * 
     * @param   encoder
     *          The AHEncoder object already constructed, passed as rvalue.
     *          Example: `AHEncoder({9, 10})`
     * @param   buttonPin
     *          Pin number of the associated button (mandatory).
     * @param   address
     *          MIDI address associated with this control (CC number, channel, etc.)
     * @param   label
     *          Displayable name of the control (e.g., "Reverb", "Filter").
     * @param   info
     *          Optional descriptive text for the control. Default is empty string.
     */
    Control(
        AHEncoder &&encoder,
        pin_t buttonPin,
        MIDIAddress address,
        const char *label,
        const char *info = ""
    )
        : MIDIAbsoluteEncoder<ContinuousCCSender>(std::move(encoder), address, 1, 4, {}),
          button(buttonPin),
          label(label),
          info(info)
    {
        button.begin();
    }

    /**
     * @brief   Called continuously by the main loop to update the control state.
     * 
     * - Checks the button state: if pressed, multiplies encoder speed (x10).  
     * - Otherwise, normal speed (x1).  
     * - Updates the encoder and displays the current value if it has changed.
     */
    void update() {
        if (button.update() == Button::Pressed) {
            setSpeedMultiply(10);
        } else {
            setSpeedMultiply(1);
        }

        MIDIAbsoluteEncoder<ContinuousCCSender>::update();

        int current = getValue();
        if (current != lastValue) {
            lastValue = current;
            display.control(label, info, current);
        }
    }

private:
    int lastValue = -1;       ///< Last displayed or sent encoder value

public:
    Button button;            ///< Associated button (mandatory)
    const char *label;        ///< Displayable name of the control
    const char *info;         ///< Optional descriptive text
};

END_CS_NAMESPACE
