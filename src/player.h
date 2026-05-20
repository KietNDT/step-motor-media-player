// src/player.h
#ifndef PLAYER_H
#define PLAYER_H

#include <stdint.h>
#include "songs.h"   // ← ADD THIS - defines Step and Song types

extern uint8_t volume;   // 0–100

// Motor channels
typedef enum {
    MOTOR_MELODY = 0,
    MOTOR_HARMONY,
    MOTOR_BASS,
    MOTOR_COUNT
} MotorChannel;

void player_init(void);
void player_play(uint8_t song_index);
void player_play_3ch(const Step* melody, const Step* harmony, const Step* bass, 
                     uint16_t mel_len, uint16_t har_len, uint16_t bas_len);
void player_stop(void);
void player_stop_motor(MotorChannel motor);
void player_set_volume(uint8_t vol);
void player_next_song(void);

// For backwards compatibility
void player_play_single(uint8_t song_index);  // old single-motor mode

#endif // PLAYER_H