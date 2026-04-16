#pragma once

BEGIN_CS_NAMESPACE

namespace Bankable {

/**
 * @brief   Abstract class representing a set of momentary buttons that send MIDI events,
 *          with double-bank support (channel and note).
 * 
 * Each button is **software debounced** and triggers MIDI events on press and release.  
 *
 * @tparam  BankAddress  Type of bankable address (channel, note, etc.)
 * @tparam  Sender       Type of MIDI sender used (e.g., DigitalNoteSender)
 * @tparam  NumButtons   Number of physical buttons managed
 *
 * @see     Button
 */
template <class BankAddress, class Sender, uint8_t NumButtons>
class DoubleBanksMIDIButtons : public MIDIOutputElement {
public:
    /**
     * @brief   Constructs a double-bank MIDI button set.
     * 
     * @param   channel_bankAddress
     *          Bankable address associated with the MIDI channel.
     * @param   note_bankAddress
     *          Bankable address associated with the MIDI note.
     * @param   buttons
     *          Array of physical buttons. Each button triggers a MIDI event depending on its state.
     * @param   incrementAddress
     *          Address offset applied between successive buttons.
     * @param   sender
     *          Object responsible for sending MIDI messages.
     */
    DoubleBanksMIDIButtons(
        BankAddress channel_bankAddress,
        BankAddress note_bankAddress,
        const Array<Button, NumButtons>& buttons,
        RelativeMIDIAddress incrementAddress,
        const Sender &sender
    ) : channel_address(channel_bankAddress),
        note_address(note_bankAddress),
        buttons(buttons),
        incrementAddress(incrementAddress),
        sender(sender) 
    {}

    /**
     * @brief   Initializes the buttons (enables internal pull-ups and configures input pins).
     */
    void begin() override { 
        for (auto &button : buttons) 
            button.begin(); 
    }

    /**
     * @brief   Updates the button states and sends corresponding MIDI events.
     * 
     * - Press (Falling) → sends **Note On**
     * - Release (Rising) → sends **Note Off**
     * - Addresses remain locked while at least one button is pressed
     * - Each successive button receives an address offset according to `incrementAddress`
     */
    void update() override {
        RelativeMIDIAddress offset = {0, 0, 0};

        for (auto &button : buttons) {
            Button::State state = button.update();

            if (state == Button::Falling) {
                if (!activeButtons) {
                    channel_address.lock();
                    note_address.lock();
                }

                sender.sendOn(
                    channel_address.getActiveAddress()
                    + RelativeMIDIAddress(note_address.getActiveAddress().getAddress() - channel_address.getActiveAddress().getAddress())
                    + offset
                );

                activeButtons++;

            } else if (state == Button::Rising) {
                sender.sendOff(
                    channel_address.getActiveAddress()
                    + RelativeMIDIAddress(note_address.getActiveAddress().getAddress() - channel_address.getActiveAddress().getAddress())
                    + offset
                );

                activeButtons--;

                if (!activeButtons) {
                    channel_address.unlock();
                    note_address.unlock();
                }
            }

            if (state == Button::Pressed) light.boost(sender.getVelocity());

            offset += incrementAddress;
        }
    }

    /**
     * @brief   Inverts the logic of the buttons (active-low / active-high).
     */
    void invert() { 
        for (auto &button : buttons) 
            button.invert(); 
    }

    /**
     * @brief   Retrieves the current state of a specific button.
     * @param   index  Index of the button in the array
     * @return  Current state of the button (Pressed, Released, Falling, Rising)
     */
    Button::State getButtonState(size_t index) const { 
        return buttons[index].getState(); 
    }

protected:
    BankAddress channel_address;           ///< MIDI channel bank address
    BankAddress note_address;              ///< MIDI note bank address
    Array<Button, NumButtons> buttons;     ///< Array of physical buttons
    RelativeMIDIAddress incrementAddress;  ///< Address offset for each successive button
    uint8_t activeButtons = 0;             ///< Number of currently pressed buttons

public:
    Sender sender; ///< Object responsible for sending MIDI events
};

} // namespace Bankable

END_CS_NAMESPACE
