#ifndef EOF_PLAYER_H
#define EOF_PLAYER_H

void eof_music_play(void);	//Plays/pauses the chart audio
void eof_catalog_play(void);	//Plays the current catalog entry, stops playback if the chart is playing
void eof_music_seek(unsigned long pos);
	//Seeks the chart audio to the specified position (also offsetting by eof_av_delay) and updates EOF variables
	//This is similar to eof_set_seek_position(), but this function automatically adds eof_av_delay to the passed position
void eof_music_rewind(void);	//Seeks backward
void eof_music_forward(void);	//Seeks forward

#endif
