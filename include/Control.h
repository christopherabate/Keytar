#pragma once

#include "Address.h"
#include <EEPROM.h>
#include <MIDI_Outputs/Abstract/MIDIAbsoluteEncoder.hpp>
#include <MIDI_Senders/ContinuousCCSender.hpp>
#include <AH/Hardware/MultiPurposeButton.hpp>

BEGIN_CS_NAMESPACE

/**
 * @brief   Represents a MIDI control based on an absolute rotary encoder.
 * 
 * This class inherits from `MIDIAbsoluteEncoder<ContinuousCCSender>` and adds:
 *  - a label (`label`) to identify the controlled parameter or effect;
 *  - an optional descriptive text (`info`);
 *  - a mandatory physical button used to modify encoder speed and latch value.
 *  - EEPROM persistence for the current control value.
 * 
 * Button behavior:
 *  - Pressed: encoder speed is multiplied by 10
 *  - Released: encoder speed returns to normal (x1)
 *  - Double click: toggles latch (0 <-> previous value)
 *
 * The current value is saved to EEPROM after the encoder has been idle
 * for a short period, in order to reduce EEPROM write cycles.
 */
class Control : public MIDIAbsoluteEncoder<ContinuousCCSender> {
public:
    /**
     * @brief   Construct a new Control object with an absolute encoder and a button.
     *
     * Each control automatically reserves one EEPROM byte from the global EEPROM
     * address allocator.
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
          EEPROMAddress(eeprom++),
          button(buttonPin),
          label(label),
          info(info)
    {
        setSpeedMultiply(4);
        button.setLongPressDelay(0);
        button.setMultiPressDelay(150);
        button.begin();
    }

    /**
     * @brief   Initialize the control.
     *
     * Restores the last saved value from EEPROM, then synchronizes the internal
     * encoder position with the current physical encoder position.
     */
    void begin() override {
        MIDIAbsoluteEncoder<ContinuousCCSender>::begin();

        uint8_t storedValue = EEPROM.read(EEPROMAddress);

        // EEPROM erased/uninitialized = 0xFF, so accept only MIDI values 0..127
        if (storedValue <= MIDIAbsoluteEncoder<ContinuousCCSender>::getMaxValue()) {
            setValue(storedValue);
            forcedUpdate();
        }

        // Synchronize the internal encoder position without changing the restored value
        resetPositionOffset();

        // Avoid triggering a display popup immediately after startup
        //lastValue = getValue();
    }

    /**
     * @brief   Called continuously by the main loop to update the control state.
     * 
     * - Handles button events (press / release / double click)
     * - Adjusts encoder speed multiplier
     * - Applies latch logic on double click
     * - Updates MIDI output and display
     */
    void update() {
        // -------------------------------------------------------------
        // Button behavior
        // -------------------------------------------------------------
        switch (button.update()) {
            
            case button.LongPressRelease:
                setSpeedMultiply(4);
                break;
            
            case button.LongPress:
                setSpeedMultiply(16);
                break;

            case button.MultiPress:
            {
                int current = getValue();

                if (current > 0) {
                    savedValue = current;
                    setValue(0);
                } else {
                    if (savedValue == 0) setValue(127);
                    else setValue(savedValue);
                }

                forcedUpdate();

                break;
            }

            default: break;
        }

        // -------------------------------------------------------------
        // Encoder update
        // -------------------------------------------------------------
        MIDIAbsoluteEncoder<ContinuousCCSender>::update();

        int current = getValue();
        if (current != lastValue) {
            lastValue = current;

            // Schedule EEPROM save after the encoder has stopped moving
            EEPROMTimer = millis();
            EEPROMPending = true;

            if (current != 0) display.control(label, info, current);
            else display.effect(label, info, current);
        }

        // -------------------------------------------------------------
        // Deferred EEPROM save
        // -------------------------------------------------------------
        if (EEPROMPending &&
            millis() - EEPROMTimer >= EEPROM_SAVE_DELAY) {

            EEPROM.update(EEPROMAddress, static_cast<uint8_t>(getValue()));

            EEPROMPending = false;
        }
    }

private:
    int lastValue = -1;         ///< Last displayed or sent encoder value
    int savedValue = 0;         ///< Stored value for latch restore

    int EEPROMAddress;          ///< EEPROM address used to store the current value
    uint32_t EEPROMTimer = 0;   ///< Timestamp of the last value change
    bool EEPROMPending = false; ///< True when an EEPROM save is pending

    static constexpr uint32_t EEPROM_SAVE_DELAY = 250;  ///< Delay before saving (ms)

public:
    MultiPurposeButton button;  ///< Button handling press / release / double click
    const char *label;          ///< Displayable name of the control
    const char *info;           ///< Optional descriptive text
};

END_CS_NAMESPACE