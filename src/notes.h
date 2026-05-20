// src/notes.h — Full note LUT for STM32 TIM1 (PSC=99, 1MHz timer clock)
// ARR = (1,000,000 / (freq * 2)) - 1
// Range: B1 (61Hz) → B7 (3951Hz) — practical motor audio range

#ifndef NOTES_H
#define NOTES_H

#include <stdint.h>

typedef enum {
    NOTE_C0,  NOTE_CS0, NOTE_D0,  NOTE_DS0, NOTE_E0,
    NOTE_F0,  NOTE_FS0, NOTE_G0,  NOTE_GS0, NOTE_A0,  NOTE_AS0, NOTE_B0,

    // Octave 1
    NOTE_C1,  NOTE_CS1, NOTE_D1,  NOTE_DS1, NOTE_E1,
    NOTE_F1,  NOTE_FS1, NOTE_G1,  NOTE_GS1, NOTE_A1,  NOTE_AS1, NOTE_B1,

    // Octave 2
    NOTE_C2,  NOTE_CS2, NOTE_D2,  NOTE_DS2, NOTE_E2,
    NOTE_F2,  NOTE_FS2, NOTE_G2,  NOTE_GS2, NOTE_A2,  NOTE_AS2, NOTE_B2,

    // Octave 3
    NOTE_C3,  NOTE_CS3, NOTE_D3,  NOTE_DS3, NOTE_E3,
    NOTE_F3,  NOTE_FS3, NOTE_G3,  NOTE_GS3, NOTE_A3,  NOTE_AS3, NOTE_B3,

    // Octave 4 (middle octave)
    NOTE_C4,  NOTE_CS4, NOTE_D4,  NOTE_DS4, NOTE_E4,
    NOTE_F4,  NOTE_FS4, NOTE_G4,  NOTE_GS4, NOTE_A4,  NOTE_AS4, NOTE_B4,

    // Octave 5
    NOTE_C5,  NOTE_CS5, NOTE_D5,  NOTE_DS5, NOTE_E5,
    NOTE_F5,  NOTE_FS5, NOTE_G5,  NOTE_GS5, NOTE_A5,  NOTE_AS5, NOTE_B5,

    // Octave 6
    NOTE_C6,  NOTE_CS6, NOTE_D6,  NOTE_DS6, NOTE_E6,
    NOTE_F6,  NOTE_FS6, NOTE_G6,  NOTE_GS6, NOTE_A6,  NOTE_AS6, NOTE_B6,

    // Octave 7
    NOTE_C7,  NOTE_CS7, NOTE_D7,  NOTE_DS7, NOTE_E7,
    NOTE_F7,  NOTE_FS7, NOTE_G7,  NOTE_GS7, NOTE_A7,  NOTE_AS7, NOTE_B7,

    NOTE_REST,
    NOTE_COUNT
} NoteIndex;

// Frequencies in Hz × 10 (standard equal temperament, A4=440Hz)
// ARR = (1,000,000 / (freq * 2)) - 1
static const uint32_t NOTE_ARR[NOTE_COUNT] = {

    [NOTE_C0]  = 1000000/(  33*2)-1,   // ~32.7 Hz → 33 Hz
    [NOTE_CS0] = 1000000/(  35*2)-1,   // 34.6 Hz → 35
    [NOTE_D0]  = 1000000/(  37*2)-1,   // 36.7 Hz → 37
    [NOTE_DS0] = 1000000/(  39*2)-1,   // 38.9 Hz → 39
    [NOTE_E0]  = 1000000/(  41*2)-1,   // 41.2 Hz → 41
    [NOTE_F0]  = 1000000/(  44*2)-1,   // 43.7 Hz → 44
    [NOTE_FS0] = 1000000/(  46*2)-1,   // 46.2 Hz → 46
    [NOTE_G0]  = 1000000/(  49*2)-1,   // 49.0 Hz → 49
    [NOTE_GS0] = 1000000/(  52*2)-1,   // 51.9 Hz → 52
    [NOTE_A0]  = 1000000/(  55*2)-1,   // 55.0 Hz → 55
    [NOTE_AS0] = 1000000/(  58*2)-1,   // 58.3 Hz → 58
    [NOTE_B0]  = 1000000/(  62*2)-1,   //  61.7 Hz

    // Octave 1
    [NOTE_C1]  = 1000000/(  33*2)-1,   // ~32.7 Hz → 33 Hz
    [NOTE_CS1] = 1000000/(  35*2)-1,   // 34.6 Hz → 35
    [NOTE_D1]  = 1000000/(  37*2)-1,   // 36.7 Hz → 37
    [NOTE_DS1] = 1000000/(  39*2)-1,   // 38.9 Hz → 39
    [NOTE_E1]  = 1000000/(  41*2)-1,   // 41.2 Hz → 41
    [NOTE_F1]  = 1000000/(  44*2)-1,   // 43.7 Hz → 44
    [NOTE_FS1] = 1000000/(  46*2)-1,   // 46.2 Hz → 46
    [NOTE_G1]  = 1000000/(  49*2)-1,   // 49.0 Hz → 49
    [NOTE_GS1] = 1000000/(  52*2)-1,   // 51.9 Hz → 52
    [NOTE_A1]  = 1000000/(  55*2)-1,   // 55.0 Hz → 55
    [NOTE_AS1] = 1000000/(  58*2)-1,   // 58.3 Hz → 58
    [NOTE_B1]  = 1000000/(  62*2)-1,   //  61.7 Hz

    // Octave 2
    [NOTE_C2]  = 1000000/(  65*2)-1,   //  65.4 Hz
    [NOTE_CS2] = 1000000/(  69*2)-1,   //  69.3 Hz
    [NOTE_D2]  = 1000000/(  73*2)-1,   //  73.4 Hz
    [NOTE_DS2] = 1000000/(  78*2)-1,   //  77.8 Hz
    [NOTE_E2]  = 1000000/(  82*2)-1,   //  82.4 Hz
    [NOTE_F2]  = 1000000/(  87*2)-1,   //  87.3 Hz
    [NOTE_FS2] = 1000000/(  93*2)-1,   //  92.5 Hz
    [NOTE_G2]  = 1000000/(  98*2)-1,   //  98.0 Hz
    [NOTE_GS2] = 1000000/( 104*2)-1,   // 103.8 Hz
    [NOTE_A2]  = 1000000/( 110*2)-1,   // 110.0 Hz
    [NOTE_AS2] = 1000000/( 117*2)-1,   // 116.5 Hz
    [NOTE_B2]  = 1000000/( 123*2)-1,   // 123.5 Hz

    // Octave 3
    [NOTE_C3]  = 1000000/( 131*2)-1,   // 130.8 Hz
    [NOTE_CS3] = 1000000/( 139*2)-1,   // 138.6 Hz
    [NOTE_D3]  = 1000000/( 147*2)-1,   // 146.8 Hz
    [NOTE_DS3] = 1000000/( 156*2)-1,   // 155.6 Hz
    [NOTE_E3]  = 1000000/( 165*2)-1,   // 164.8 Hz
    [NOTE_F3]  = 1000000/( 175*2)-1,   // 174.6 Hz
    [NOTE_FS3] = 1000000/( 185*2)-1,   // 185.0 Hz
    [NOTE_G3]  = 1000000/( 196*2)-1,   // 196.0 Hz
    [NOTE_GS3] = 1000000/( 208*2)-1,   // 207.7 Hz
    [NOTE_A3]  = 1000000/( 220*2)-1,   // 220.0 Hz
    [NOTE_AS3] = 1000000/( 233*2)-1,   // 233.1 Hz
    [NOTE_B3]  = 1000000/( 247*2)-1,   // 246.9 Hz

    // Octave 4 (middle)
    [NOTE_C4]  = 1000000/( 262*2)-1,   // 261.6 Hz
    [NOTE_CS4] = 1000000/( 277*2)-1,   // 277.2 Hz
    [NOTE_D4]  = 1000000/( 294*2)-1,   // 293.7 Hz
    [NOTE_DS4] = 1000000/( 311*2)-1,   // 311.1 Hz
    [NOTE_E4]  = 1000000/( 330*2)-1,   // 329.6 Hz
    [NOTE_F4]  = 1000000/( 349*2)-1,   // 349.2 Hz
    [NOTE_FS4] = 1000000/( 370*2)-1,   // 370.0 Hz
    [NOTE_G4]  = 1000000/( 392*2)-1,   // 392.0 Hz
    [NOTE_GS4] = 1000000/( 415*2)-1,   // 415.3 Hz
    [NOTE_A4]  = 1000000/( 440*2)-1,   // 440.0 Hz
    [NOTE_AS4] = 1000000/( 466*2)-1,   // 466.2 Hz
    [NOTE_B4]  = 1000000/( 494*2)-1,   // 493.9 Hz

    // Octave 5
    [NOTE_C5]  = 1000000/( 523*2)-1,   // 523.3 Hz
    [NOTE_CS5] = 1000000/( 554*2)-1,   // 554.4 Hz
    [NOTE_D5]  = 1000000/( 587*2)-1,   // 587.3 Hz
    [NOTE_DS5] = 1000000/( 622*2)-1,   // 622.3 Hz
    [NOTE_E5]  = 1000000/( 659*2)-1,   // 659.3 Hz
    [NOTE_F5]  = 1000000/( 698*2)-1,   // 698.5 Hz
    [NOTE_FS5] = 1000000/( 740*2)-1,   // 740.0 Hz
    [NOTE_G5]  = 1000000/( 784*2)-1,   // 784.0 Hz
    [NOTE_GS5] = 1000000/( 831*2)-1,   // 830.6 Hz
    [NOTE_A5]  = 1000000/( 880*2)-1,   // 880.0 Hz
    [NOTE_AS5] = 1000000/( 932*2)-1,   // 932.3 Hz
    [NOTE_B5]  = 1000000/( 988*2)-1,   // 987.8 Hz

    // Octave 6
    [NOTE_C6]  = 1000000/(1047*2)-1,   // 1046.5 Hz
    [NOTE_CS6] = 1000000/(1109*2)-1,   // 1108.7 Hz
    [NOTE_D6]  = 1000000/(1175*2)-1,   // 1174.7 Hz
    [NOTE_DS6] = 1000000/(1245*2)-1,   // 1244.5 Hz
    [NOTE_E6]  = 1000000/(1319*2)-1,   // 1318.5 Hz
    [NOTE_F6]  = 1000000/(1397*2)-1,   // 1396.9 Hz
    [NOTE_FS6] = 1000000/(1480*2)-1,   // 1480.0 Hz
    [NOTE_G6]  = 1000000/(1568*2)-1,   // 1568.0 Hz
    [NOTE_GS6] = 1000000/(1661*2)-1,   // 1661.2 Hz
    [NOTE_A6]  = 1000000/(1760*2)-1,   // 1760.0 Hz
    [NOTE_AS6] = 1000000/(1865*2)-1,   // 1864.7 Hz
    [NOTE_B6]  = 1000000/(1976*2)-1,   // 1975.5 Hz

    // Octave 7
    [NOTE_C7]  = 1000000/(2093*2)-1,   // 2093.0 Hz
    [NOTE_CS7] = 1000000/(2217*2)-1,   // 2217.5 Hz
    [NOTE_D7]  = 1000000/(2349*2)-1,   // 2349.3 Hz
    [NOTE_DS7] = 1000000/(2489*2)-1,   // 2489.0 Hz
    [NOTE_E7]  = 1000000/(2637*2)-1,   // 2637.0 Hz
    [NOTE_F7]  = 1000000/(2794*2)-1,   // 2793.8 Hz
    [NOTE_FS7] = 1000000/(2960*2)-1,   // 2960.0 Hz
    [NOTE_G7]  = 1000000/(3136*2)-1,   // 3136.0 Hz
    [NOTE_GS7] = 1000000/(3322*2)-1,   // 3322.4 Hz
    [NOTE_A7]  = 1000000/(3520*2)-1,   // 3520.0 Hz
    [NOTE_AS7] = 1000000/(3729*2)-1,   // 3729.3 Hz
    [NOTE_B7]  = 1000000/(3951*2)-1,   // 3951.1 Hz

    [NOTE_REST] = 0,
};


#endif // NOTES_H