#ifndef EOF_DRUMBEATS_H
#define EOF_DRUMBEATS_H

#include "song.h"

int eof_export_drumbeats_midi(EOF_SONG *sp, unsigned long track, unsigned char diff, char *fn);
	//Exports the specified drum track difficulty to a MIDI file suited for use in the game DrumBeats VR
	//Returns zero on error

#endif
