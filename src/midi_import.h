#ifndef EOF_MIDI_IMPORT_H
#define EOF_MIDI_IMOPRT_H

#include "song.h"
#include "midi.h"
#include "foflc/Midi_parse.h"

#define EOF_MAX_IMPORT_MIDI_TRACKS 16

EOF_SONG * eof_import_midi(const char * fn);
EOF_SONG * eof_import_midi2(const char * fn);
double eof_ConvertToRealTime(unsigned long absolutedelta,double starttime,struct Tempo_change *anchorlist,unsigned long timedivision);
	//Converts and returns the converted realtime of the specified delta time based on the list of anchors and MIDI's time division

#endif
