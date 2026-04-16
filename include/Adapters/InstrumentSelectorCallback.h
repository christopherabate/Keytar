#pragma once

#include <EEPROM.h>

/**
 * @brief   Callback class for a Control Surface selector that persists the selection to EEPROM.
 * 
 * This class can be attached to a GenericIncrementDecrementSelector or similar Selectable element.
 * It initializes the selector with a value stored in EEPROM and automatically saves any changes.
 * 
 * @tparam N  Number of options in the Selectable.
 */
template <setting_t N>
class InstrumentSelectorCallback {
public:
    /**
     * @brief   Construct a new InstrumentSelectorCallback object.
     * 
     * @param   selectable
     *          Reference to the Selectable object this callback monitors.
     * @param   EEPROMAddress
     *          EEPROM address to read/write the selector value.
     */
    InstrumentSelectorCallback(cs::Selectable<N> &selectable, int EEPROMAddress) 
        : selectable(selectable), EEPROMAddress(EEPROMAddress) {}

    /**
     * @brief   Called once by Control Surface during initialization.
     * 
     * Reads the initial selection from EEPROM and applies it to the Selectable.
     */
    void begin() {
        uint8_t initialSelection = EEPROM.read(EEPROMAddress);
        if (initialSelection < N) {
            selectable.setInitialSelection(initialSelection);
        }
    }

    /**
     * @brief   Called continuously by Control Surface in the main loop.
     * 
     * Can be used to implement continuous effects like fading or blinking.
     * Not used in this example.
     */
    void update() {}

    /**
     * @brief   Called whenever the selection changes.
     * 
     * @param   previousSelection  The previously selected value.
     * @param   currentSelection   The new selected value.
     * 
     * Updates the EEPROM with the new selection and prints it to Serial.
     */
    void update(setting_t previousSelection, setting_t currentSelection) {
        EEPROM.update(EEPROMAddress, currentSelection);
        display.main();
    }

private:
    cs::Selectable<N> &selectable; ///< Reference to the associated Selectable object.
    int EEPROMAddress;         ///< EEPROM address used to store the current selection.
};
