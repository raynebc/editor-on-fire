#ifndef EOF_EXPORT_H
#define EOF_EXPORT_H

#include "song.h"

char *eof_difficulty_ini_tags[EOF_TRACKS_MAX + 1];
	//Stores the song.ini difficulty tag strings for each track, including one extra element for track 0 (band difficulty)
	//"PART GUITAR COOP" and "PART RHYTHM" are not currently handled

int eof_save_ini(EOF_SONG * sp, char * fn);

#endif
