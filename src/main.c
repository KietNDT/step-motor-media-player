#include "player.h"
#include "songs/mozart-symphony40-1-piano-solo.h"

int main(void) {
    player_init();
    player_set_volume(80);
    player_play_3ch(
        song_mozart_symphony40_1_piano_solo_melody,
        song_mozart_symphony40_1_piano_solo_harmony,
        song_mozart_symphony40_1_piano_solo_bass,
        SONG_MOZART_SYMPHONY40_1_PIANO_SOLO_MELODY_LEN,
        SONG_MOZART_SYMPHONY40_1_PIANO_SOLO_HARMONY_LEN,
        SONG_MOZART_SYMPHONY40_1_PIANO_SOLO_BASS_LEN
    );
    while(1);
}