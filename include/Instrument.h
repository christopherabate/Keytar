#pragma once

#include "Adapters/DoubleBanksNoteButtons.h"
#include "Adapters/InstrumentSelectorCallback.h"

inline int eeprom = 0;

/**
 * @brief   Represents a single row of MIDI note buttons with a starting note.
 * 
 * Each row contains a fixed number of physical buttons and a base MIDI note.
 * This allows constructing instruments with multiple rows of keys, potentially
 * of different lengths.
 * 
 * @tparam  Buttons
 *          Number of physical buttons in this row.
 */
template <uint8_t Buttons>
struct Row {
    /** @brief   Array of physical buttons corresponding to this row. */
    const Array<Button, Buttons> &buttons;

    /** @brief   Base MIDI note for the first button in this row. */
    MIDIAddress baseNote;

    /** @brief   Compile-time accessor for the number of buttons. */
    static constexpr uint8_t size() { return Buttons; }
};

/**
 * @brief   Represents a complete MIDI instrument composed of multiple rows of buttons.
 * 
 * This class bundles:
 *  - a transposer to shift notes across rows,
 *  - a 16-channel MIDI bank with selector,
 *  - a set of configurable note rows.
 * 
 * The instrument can have rows of different lengths by using a variadic template.
 * Each row is represented by a `Row<Buttons>` type.
 * 
 * @tparam  Rows
 *          Variadic template parameters representing the individual `Row` types.
 */
template <typename... Rows>
struct Instrument {

    /**
     * @brief   Transposer managing note offsets from -2 to +2 semitones.
     * 
     * Allows dynamic transposition of played notes across all rows.
     */
    Transposer<-2, +2> transposer;

    /**
     * @brief   Increment/decrement selector associated with the transposer.
     * 
     * Allows the user to change the transposition using physical buttons.
     */
    GenericIncrementDecrementSelector<5, InstrumentSelectorCallback<5>> transposerSelector;

    /**
     * @brief   MIDI channel bank (16 channels available).
     * 
     * Enables dynamically changing the channel on which MIDI messages are sent.
     */
    Bank<16> channel;

    /**
     * @brief   Increment/decrement selector for active MIDI channel.
     */
    GenericIncrementDecrementSelector<16, InstrumentSelectorCallback<16>> channelSelector;

    /**
     * @brief   Tuple containing one `DoubleBanksNoteButtons` object per row.
     * 
     * Each element automatically manages:
     *  - the corresponding MIDI channel bank,
     *  - the row's base note with applied transposition.
     */
    std::tuple<Bankable::DoubleBanksNoteButtons<Rows::size()>...> keys;

    /**
     * @brief   Constructs a complete instrument with transposer, channel selectors, and note rows.
     * 
     * @param   transposerButtons
     *          Physical buttons used to adjust the transposer.
     * @param   channelButtons
     *          Physical buttons used to select the active MIDI channel.
     * @param   rows
     *          Variadic list of `Row` objects representing each row of the instrument.
     *          Rows can have different numbers of buttons.
     */
    Instrument(
        const IncrementDecrementButtons &transposerButtons,
        const IncrementDecrementButtons &channelButtons,
        Rows... rows
    )
    : transposer(12), // Base MIDI: one octave (12 semitones)
      transposerSelector(
          transposer,
          InstrumentSelectorCallback<5>(transposer, eeprom),
          transposerButtons,
          Wrap::Clamp
      ),
      channel(1),
      channelSelector(
          channel,
          InstrumentSelectorCallback<16>(channel, eeprom + 1), // EEPROM address for channel
          channelButtons,
          Wrap::Clamp
      ),
      keys{ Bankable::DoubleBanksNoteButtons<Rows::size()>(
          {channel, BankType::ChangeChannel},
          {transposer, BankType::ChangeAddress},
          rows.buttons,
          rows.baseNote,
          1
      )... }
    {
        // Increment EEPROM base address for next instrument
        eeprom += 10;
    }
};