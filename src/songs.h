// src/songs.h
#ifndef SONGS_H
#define SONGS_H

#include "notes.h"
#include <stdint.h>

// Duration macros (as before)
#define MS_64  47
#define MS_32  94
#define MS_16 187
#define MS_Q  375
#define MS_DQ 562
#define MS_H  750
#define MS_DH 1125
#define MS_W 1500

typedef struct { uint8_t note; uint16_t dur_ms; } Step;

// Song structure for 3 motors
typedef struct {
    const char* name;
    const Step* melody;
    uint16_t melody_len;
    const Step* harmony;
    uint16_t harmony_len;
    const Step* bass;
    uint16_t bass_len;
} Song3;

// ---- Include your generated 3‑motor song headers ----
#include "songs/mozart-symphony40-1-piano-solo.h"
#include "songs/Tokyo Ghoul - Unravel.h"
#include "songs/Queen - Bohemian Rhapsody.h"
#include "songs/linkin_park-numb.h"   // provides _melody, _harmony, _bass arrays and length macros
#include "songs\Tetris - A Theme.h"             // future song

// ---- Define the song list ----
static const Song3 song_list[] = {



    {
        .name = "Mozart Symphony40",
        .melody = song_mozart_symphony40_1_piano_solo_melody,
        .melody_len = SONG_MOZART_SYMPHONY40_1_PIANO_SOLO_MELODY_LEN,
        .harmony = song_mozart_symphony40_1_piano_solo_harmony,
        .harmony_len = SONG_MOZART_SYMPHONY40_1_PIANO_SOLO_HARMONY_LEN,
        .bass = song_mozart_symphony40_1_piano_solo_bass,
        .bass_len = SONG_MOZART_SYMPHONY40_1_PIANO_SOLO_BASS_LEN,
    },
    {
        .name = "Tetris Theme",
        .melody = song_tetris___a_theme_melody,
        .melody_len = SONG_TETRIS___A_THEME_MELODY_LEN,
        .harmony = song_tetris___a_theme_harmony,
        .harmony_len = SONG_TETRIS___A_THEME_HARMONY_LEN,
        .bass = song_tetris___a_theme_bass,
        .bass_len = SONG_TETRIS___A_THEME_BASS_LEN,
    },
    {
        .name = "Tokyo Ghoul",
        .melody = song_tokyo_ghoul___unravel_melody,
        .melody_len = SONG_TOKYO_GHOUL___UNRAVEL_MELODY_LEN,
        .harmony = song_tokyo_ghoul___unravel_harmony,
        .harmony_len = SONG_TOKYO_GHOUL___UNRAVEL_HARMONY_LEN,
        .bass = song_tokyo_ghoul___unravel_bass,
        .bass_len = SONG_TOKYO_GHOUL___UNRAVEL_BASS_LEN,
    },
    {
        .name = "Queen   Bohemian Rhapsody",
        .melody = song_queen___bohemian_rhapsody_melody,
        .melody_len = SONG_QUEEN___BOHEMIAN_RHAPSODY_MELODY_LEN,
        .harmony = song_queen___bohemian_rhapsody_harmony,
        .harmony_len = SONG_QUEEN___BOHEMIAN_RHAPSODY_HARMONY_LEN,
        .bass = song_queen___bohemian_rhapsody_bass,
        .bass_len = SONG_QUEEN___BOHEMIAN_RHAPSODY_BASS_LEN,
    },
    {
        .name = "Linkin Park Numb",
        .melody = song_linkin_park_numb_melody,
        .melody_len = SONG_LINKIN_PARK_NUMB_MELODY_LEN,
        .harmony = song_linkin_park_numb_harmony,
        .harmony_len = SONG_LINKIN_PARK_NUMB_HARMONY_LEN,
        .bass = song_linkin_park_numb_bass,
        .bass_len = SONG_LINKIN_PARK_NUMB_BASS_LEN,
    },
};

#define SONG_COUNT (sizeof(song_list)/sizeof(song_list[0]))

#endif