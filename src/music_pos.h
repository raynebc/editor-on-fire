#ifndef EOF_MUSIC_POS_H
#define EOF_MUSIC_POS_H

typedef struct
{

	int value;       // the current value
	int accumulator; // keep track of how far we are into the next position
	int velocity;    // how much to change value each frame
	int accumulate;
	int rate;
	int last_speed;

} EOF_MUSIC_POS;

void eof_initialize_music_pos(EOF_MUSIC_POS * mpp, int rate);
void eof_set_music_pos(EOF_MUSIC_POS * mpp, int pos);
void eof_update_music_pos(EOF_MUSIC_POS * mpp, int speed);

#endif
