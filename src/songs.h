#ifndef SONGS_H
#define SONGS_H

#include "notes.h"
#include <stdint.h>

// Duration macros (same as before)
#define MS_64  47
#define MS_32  94
#define MS_16 187
#define MS_Q  375
#define MS_DQ 562
#define MS_H  750
#define MS_DH 1125
#define MS_W 1500

#define STEPS(arr) (sizeof(arr)/sizeof(arr[0]))

typedef struct { uint8_t note; uint16_t dur_ms; } Step;
typedef struct { const char* name; const Step* steps; uint16_t length; } Song;

// Only include single‑motor songs here (if any)
// #include "songs/tetris.h"

static const Song song_list[] = {
    // { "Tetris", song_tetris, STEPS(song_tetris) },
};

#define SONG_COUNT (sizeof(song_list)/sizeof(song_list[0]))

#endif