#include "Light.h"
#include <Control_Surface.h>
#include "Display.h"
#include "Instrument.h"
#include "Control.h"
#include "Effect.h"
#include "Sensor.h"

USBMIDI_Interface midi;

CD74HC4067 mux_0 {2, {35, 36, 38, 37}};
CD74HC4067 mux_1 {3, {35, 36, 38, 37}};
CD74HC4067 mux_2 {0, {35, 36, 38, 37}};
CD74HC4067 mux_3 {1, {35, 36, 38, 37}};

Instrument<Row<8>, Row<8>> head {
    {31, 32},
    {29, 30},
    Row<8>{{mux_0.pin(15), mux_0.pin(14), mux_0.pin(13), mux_0.pin(12), mux_0.pin(11), mux_0.pin(10), mux_0.pin(9), mux_0.pin(8)}, MIDI_Notes::E[3]},
    Row<8>{{mux_0.pin(0), mux_0.pin(1), mux_0.pin(2), mux_0.pin(3), mux_0.pin(4), mux_0.pin(5), mux_0.pin(6), mux_0.pin(7)}, MIDI_Notes::A[3]}
};

Instrument<Row<1>, Row<1>, Row<1>, Row<13>, Row<13>, Row<9>> body {
    {27,28},
    {25,26},
    Row<1>{{mux_1.pin(3)}, MIDI_Notes::Gb[2]},
    Row<1>{{mux_2.pin(13)}, MIDI_Notes::Ab[2]},
    Row<1>{{mux_2.pin(1)}, MIDI_Notes::Bb[2]},
    Row<13>{{mux_1.pin(12), mux_1.pin(11), mux_1.pin(9), mux_1.pin(10), mux_1.pin(5), mux_1.pin(4), mux_2.pin(11), mux_2.pin(12), mux_2.pin(3), mux_2.pin(2), mux_3.pin(13), mux_3.pin(14), mux_3.pin(0)}, MIDI_Notes::C[3]},
    Row<13>{{mux_1.pin(8), mux_1.pin(7), mux_1.pin(6), mux_2.pin(9), mux_2.pin(10), mux_2.pin(5), mux_2.pin(4), mux_3.pin(11), mux_3.pin(12), mux_3.pin(4), mux_3.pin(2), mux_3.pin(3), mux_3.pin(1)}, MIDI_Notes::Db[4]},
    Row<9>{{mux_2.pin(8), mux_2.pin(7), mux_2.pin(6), mux_3.pin(9), mux_3.pin(10), mux_3.pin(8), mux_3.pin(6), mux_3.pin(7), mux_3.pin(5)}, MIDI_Notes::Eb[5]}
};

Control controls[] = {
    {{8, 9}, mux_1.pin(14), MIDI_CC::Sound_Controller_5, "Filter"},
    {{6, 7}, mux_1.pin(15), MIDI_CC::Effects_1, "Reverb"},
    {{10, 11}, mux_1.pin(13), MIDI_CC::Effects_4, "Delay"},
    {{4, 5}, mux_1.pin(0), MIDI_CC::Effects_3, "Chorus"},
};

Effect effects[] = {
    {34, 33, MIDI_CC::General_Purpose_Controller_7, "Octaver"},
};

PBPotentiometer whammys[16] = {
    {15, Channel(1)},
    {15, Channel(2)},
    {15, Channel(3)},
    {15, Channel(4)},
    {15, Channel(5)},
    {15, Channel(6)},
    {15, Channel(7)},
    {15, Channel(8)},
    {15, Channel(9)},
    {15, Channel(10)},
    {15, Channel(11)},
    {15, Channel(12)},
    {15, Channel(13)},
    {15, Channel(14)},
    {15, Channel(15)},
    {15, Channel(16)}
};

PBPotentiometer softpots[16] = {
    {14, Channel(1)},
    {14, Channel(2)},
    {14, Channel(3)},
    {14, Channel(4)},
    {14, Channel(5)},
    {14, Channel(6)},
    {14, Channel(7)},
    {14, Channel(8)},
    {14, Channel(9)},
    {14, Channel(10)},
    {14, Channel(11)},
    {14, Channel(12)},
    {14, Channel(13)},
    {14, Channel(14)},
    {14, Channel(15)},
    {14, Channel(16)}
};

template<auto MIN, auto MAX, auto START, auto END> 
analog_t boundaries(analog_t raw){ return map(constrain(raw, MIN, MAX), MIN, MAX, START, END); }

void setup() {
    Serial.begin(115200);
    Control_Surface.begin();
    // Init light
    light.begin();
    // Sensor
    sensor.begin();
    // Whammy and softpot
    for (auto &whammy : whammys) whammy.map(boundaries<500, 700, 8192, 0>);
    for (auto &softpot : softpots) softpot.map(boundaries<5350, 10000, 8192, 16384>);
    // Set velocity
    std::apply([](auto&... key){ (key.setVelocity(100), ...); }, head.keys);
    std::apply([](auto&... key){ (key.setVelocity(127), ...); }, body.keys);
    // Init display
    display.attachInstrument(head);
    display.attachInstrument(body);
    for (auto &control : controls) display.attachControl(control);
}

void loop() {
    Control_Surface.loop();
    display.startup();
    display.main();
    light.idle();

    // Sensor
    sensor.update();
    Serial.print("Pitch (X) : ");
    Serial.print(sensor.getPitch(), 2);
    Serial.print(" deg | Accel X : ");
    Serial.print(sensor.getAccelX(), 2);
    Serial.println(" m/s²");
    Serial.print(" deg | Accel Y : ");
    Serial.print(sensor.getAccelY(), 2);
    Serial.println(" m/s²");
    Serial.print(" deg | Accel Z : ");
    Serial.print(sensor.getAccelZ(), 2);
    Serial.println(" m/s²");
}
