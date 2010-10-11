#ifndef EOF_MIDI_IMPORT_H
#define EOF_MIDI_IMOPRT_H

#include "song.h"
#include "midi.h"
#include "foflc/Midi_parse.h"

#define EOF_MAX_IMPORT_MIDI_TRACKS 16
#define EOF_MAX_MIDI_TEXT_SIZE 255
#define EOF_IMPORT_MAX_EVENTS 100000
EOF_SONG * eof_import_midi(const char * fn);
double eof_ConvertToRealTime(unsigned long absolutedelta,struct Tempo_change *anchorlist,EOF_MIDI_TS_LIST *tslist,unsigned long timedivision,unsigned long offset);
	//Converts and returns the converted realtime of the specified delta time based on the anchors, time signatures and the MIDI's time division
	//The specified offset should represent a chart delay, and is added to the returned realtime
inline unsigned long eof_ConvertToRealTimeInt(unsigned long absolutedelta,struct Tempo_change *anchorlist,EOF_MIDI_TS_LIST *tslist,unsigned long timedivision,unsigned long offset);
	//Returns the value as if eof_ConvertToRealTime() was called, and the result was rounded up to the nearest unsigned long

#endif
