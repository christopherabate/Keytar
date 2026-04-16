#pragma once

#include "DoubleBanksMIDIButtons.h"

BEGIN_CS_NAMESPACE

namespace Bankable {

/**
 * @brief   Manages one or more momentary buttons that send MIDI **Note On / Note Off** events,
 *          with support for two banks:
 *          - one for the MIDI channel
 *          - one for the note (address)
 * 
 * Each button is **software debounced**, and each press/release triggers a MIDI message.  
 * 
 * This version is **bankable**, meaning that the MIDI addresses can change depending
 * on the active bank.
 * 
 * @tparam  NumButtons   Number of physical buttons connected.
 *
 * @ingroup BankableMIDIOutputElements
 */
template <uint8_t NumButtons>
class DoubleBanksNoteButtons 
    : public DoubleBanksMIDIButtons<SingleAddress, DigitalNoteSender, NumButtons> 
{
public:
    /**
     * @brief   Constructs a double-bank MIDI "Note" button set.
     *
     * When a button is pressed, a **Note On** message is sent.  
     * When released, a **Note Off** message is sent.  
     *
     * @param   channel_config
     *          Bank configuration applied to the MIDI channel.
     *          Allows changing the channel or MIDI port depending on the active bank.
     *
     * @param   note_config
     *          Bank configuration applied to the MIDI note.
     *          Allows transposing or shifting notes according to the active bank.
     *
     * @param   buttons
     *          Array containing the physical buttons (objects `Button`).
     *          Each button triggers a MIDI message depending on its state.
     *
     * @param   baseAddress
     *          Base MIDI address (note, channel, port).  
     *          Example: `MIDIAddress(note, Channel_1)`.
     *
     * @param   incrementAddress
     *          Offset applied between successive buttons.
     *          Automatically increments notes for multiple buttons.
     *
     * @param   velocity
     *          Velocity used for MIDI "Note On" messages.
     *          Default: `0x7F` (127, maximum velocity).
     */
    DoubleBanksNoteButtons(
        OutputBankConfig<> channel_config,
        OutputBankConfig<> note_config,
        const Array<Button, NumButtons> &buttons,
        MIDIAddress baseAddress,
        RelativeMIDIAddress incrementAddress,
        uint8_t velocity = 0x7F
    ) : DoubleBanksMIDIButtons<SingleAddress, DigitalNoteSender, NumButtons> {
        {channel_config, baseAddress},    // MIDI channel address
        {note_config, baseAddress},       // MIDI note address
        buttons,                          // physical buttons array
        incrementAddress,                 // offset for each button
        {velocity},                       // velocity for Note On
    } {}

    /**
     * @brief   Sets the velocity used for MIDI Note events.
     * @param   velocity  New velocity (0–127)
     */
    void setVelocity(uint8_t velocity) { 
        this->sender.setVelocity(velocity); 
    }

    /**
     * @brief   Retrieves the currently used velocity for MIDI Note events.
     * @return  Velocity value (0–127)
     */
    uint8_t getVelocity() const { 
        return this->sender.getVelocity(); 
    }
};

} // namespace Bankable

END_CS_NAMESPACE
