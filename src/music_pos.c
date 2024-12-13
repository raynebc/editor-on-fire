#include <allegro.h>
#include "music_pos.h"

void eof_initialize_music_pos(EOF_MUSIC_POS * mpp, int rate)
{
	memset(mpp, 0, sizeof(EOF_MUSIC_POS));
	mpp->rate = rate;
	eof_set_music_pos(mpp, 0);
}

void eof_set_music_pos(EOF_MUSIC_POS * mpp, int pos)
{
	mpp->value = pos;
	mpp->accumulator = 0;
}

static void update_speed(EOF_MUSIC_POS * mpp, int speed)
{
	mpp->velocity = speed / mpp->rate;
	mpp->accumulate = speed % mpp->rate;
}

void eof_update_music_pos(EOF_MUSIC_POS * mpp, int speed)
{
	if(speed != mpp->last_speed)
	{
		update_speed(mpp, speed);
		mpp->last_speed = speed;
	}
	mpp->value += mpp->velocity;
	mpp->accumulator += mpp->accumulate;
	if(mpp->accumulator >= mpp->rate)
	{
		mpp->value += 1;
		mpp->accumulator -= mpp->rate;
	}
}
