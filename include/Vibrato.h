
#include <Control_Surface.h>

BEGIN_CS_NAMESPACE

/*
 */
class VibratoOutput : public MIDIOutputElement {
public:
    void begin() override {}

    void update() override {
        int16_t pb = sensor.getVibratoPitchBend();
        if (pb == lastPB) return; // n'envoie que si changement
        for (uint8_t ch = 1; ch <= 16; ch++)
            MIDI_Interface::getDefault().sendPitchBend(Channel(ch), pb);
        lastPB = pb;
    }

private:
    int16_t lastPB = 8192;
};

inline VibratoOutput vibrato;

END_CS_NAMESPACE