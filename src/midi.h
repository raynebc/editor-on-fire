#ifndef EOF_MIDI_H
#define EOF_MIDI_H

#include "song.h"

typedef struct
{

	unsigned long pos;
	int type;
	int note;
	char * dp;

} EOF_MIDI_EVENT;

#define EOF_MAX_MIDI_EVENTS 65536
#define EOF_TIME_DIVISION 120.0

int eof_figure_beat(double pos);				//Returns the beat marker immediately before the specified timestamp, or -1 on failure
double eof_calculate_bpm_absolute(double pos);	//Returns the tempo defined by the beat marker immediately before the specified timestamp, or 0.0 on failure
double eof_calculate_delta(double start, double end);	//Finds the number of delta ticks within the specified time span?

int eof_export_midi(EOF_SONG * sp, char * fn);	//Writes the specified chart's contents to the specified file

#endif
