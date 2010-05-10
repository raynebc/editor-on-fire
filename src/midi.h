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

int eof_figure_beat(double pos);
double eof_calculate_bpm_absolute(double pos);
double eof_calculate_delta(double start, double end);

EOF_SONG * eof_import_midi(const char * fn);
int eof_export_midi(EOF_SONG * sp, char * fn);

#endif
