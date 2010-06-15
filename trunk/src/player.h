#ifndef EOF_PLAYER_H
#define EOF_PLAYER_H

void eof_music_play(void);	//Plays/pauses the chart audio
void eof_catalog_play(void);	//Plays the current catalog entry
void eof_music_seek(unsigned long pos);	//Seeks the chart audio to the specified position and updates EOF variables
void eof_music_rewind(void);	//Seeks backward
void eof_music_forward(void);	//Seeks forward

#endif
