#include "player.h"
#include "songs.h"   // contains song_list

int main(void) {
    player_init();
    player_set_volume_channel(MOTOR_MELODY, 80);
    player_set_volume_channel(MOTOR_HARMONY, 50);
    player_set_volume_channel(MOTOR_BASS, 40);
    
    // Start with the first song
    player_play_song(0);
    
    while (1) {}
}