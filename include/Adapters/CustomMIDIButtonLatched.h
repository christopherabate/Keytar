#pragma once

#include <AH/Hardware/Button.hpp>
#include <Def/Def.hpp>
#include <MIDI_Outputs/Abstract/MIDIOutputElement.hpp>

BEGIN_CS_NAMESPACE

/**
 * @brief   Custom latched MIDI button class.
 *
 * Manages a physical button connected to a pin and sends MIDI messages
 * via a `Sender`. Each press toggles the button state (latched) and sends
 * the corresponding MIDI message (On/Off).
 *
 * @tparam  Sender  Type of the MIDI sender (e.g., DigitalCCSender)
 */
template <class Sender>
class CustomMIDIButtonLatched : public MIDIOutputElement {
protected:
    /**
     * @brief   Protected constructor to initialize the button and sender.
     * @param   pin       Physical button pin
     * @param   address   MIDI address associated with the button
     * @param   sender    MIDI sender object
     */
    CustomMIDIButtonLatched(pin_t pin, MIDIAddress address, const Sender &sender)
        : button(pin), address(address), sender(sender) {}

public:
    /**
     * @brief   Initialize the physical button.
     */
    void begin() override {
        button.begin();
    }

    /**
     * @brief   Update the button state.
     *
     * If the button is pressed (Falling), toggle the latched state
     * and send the corresponding MIDI message.
     */
    void update() override {
        Button::State state = button.update();
        if (state == Button::Falling) {
            toggleState();
        }
    }

    /**
     * @brief   Toggle the internal latched state.
     * @return  New state (true = active, false = inactive)
     */
    bool toggleState() {
        setState(!getState());
        return getState();
    }

    /**
     * @brief   Get the current latched state of the button.
     * @return  true if active, false otherwise
     */
    bool getState() const {
        return state;
    }

    /**
     * @brief   Set the latched state and send the corresponding MIDI message.
     * @param   state  New state (true = active, false = inactive)
     */
    void setState(bool state) {
        this->state = state;
        if (state) {
            sender.sendOn(address);
        } else {
            sender.sendOff(address);
        }
    }

    /**
     * @brief   Invert the logic of the physical button (active-high / active-low)
     */
    void invert() {
        button.invert();
    }

    /**
     * @brief   Get the current physical button state.
     * @return  Button state (Pressed, Released, Falling, Rising)
     */
    Button::State getButtonState() const {
        return button.getState();
    }

    /**
     * @brief   Get the MIDI address associated with this button.
     * @return  MIDI address
     */
    MIDIAddress getAddress() const {
        return address;
    }

    /**
     * @brief   Change the MIDI address of the button (internal use only).
     * @param   address  New MIDI address
     */
    void setAddressUnsafe(MIDIAddress address) {
        this->address = address;
    }

protected:
    Button button;           ///< Physical button object

private:
    MIDIAddress address;     ///< MIDI address associated
    bool state = false;      ///< Current latched state

public:
    Sender sender;           ///< MIDI sender used
};

END_CS_NAMESPACE
