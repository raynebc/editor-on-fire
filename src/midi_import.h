#ifndef EOF_MIDI_IMPORT_H
#define EOF_MIDI_IMPORT_H

#include "song.h"
#include "midi.h"
#include "foflc/Midi_parse.h"

#define EOF_MAX_IMPORT_MIDI_TRACKS 32
#define EOF_MAX_MIDI_TEXT_SIZE 255
#define EOF_IMPORT_MAX_EVENTS 100000
EOF_SONG * eof_import_midi(const char * fn);
double eof_ConvertToRealTime(unsigned long absolutedelta, struct Tempo_change *anchorlist, EOF_MIDI_TS_LIST *tslist, unsigned long timedivision, unsigned long offset);
	//Converts and returns the converted realtime of the specified delta time based on the anchors, time signatures and the MIDI's time division
	//The specified offset should represent a chart delay, and is added to the returned realtime
unsigned long eof_parse_var_len(unsigned char * data, unsigned long pos, unsigned long * bytes_used);
	//Parses a variable length value from data[pos], returning it and adding the size (in bytes) of the variable length value to bytes_used
	//bytes_used is NOT initialized to zero within this function, the calling function must set it appropriately

#endif
