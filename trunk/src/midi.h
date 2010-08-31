#ifndef EOF_MIDI_H
#define EOF_MIDI_H

#include "song.h"
#include "foflc/Midi_parse.h"

typedef struct
{

	unsigned long pos;
	int type;
	int note;
	char * dp;

} EOF_MIDI_EVENT;

#define EOF_MAX_MIDI_EVENTS 65536

int eof_figure_beat(double pos);				//Returns the beat marker immediately before the specified timestamp, or -1 on failure
double eof_calculate_bpm_absolute(double pos);	//Returns the tempo defined by the beat marker immediately before the specified timestamp, or 0.0 on failure
double eof_calculate_delta(double start, double end);	//Finds the number of delta ticks within the specified time span?

int eof_export_midi(EOF_SONG * sp, char * fn);	//Writes the specified chart's contents to the specified file

struct Tempo_change *eof_build_tempo_list(void);
	//Parses eof_song->beat[], returning a linked list of anchors (tempo changes), or NULL on error
	//Each link in the list gives the absolute deltatime and absolute realtime of the anchor

struct Tempo_change *eof_add_to_tempo_list(unsigned long delta,double realtime,double BPM,struct Tempo_change *ptr);
	//Creates and adds a new link EOF's tempo linked list referred to by ptr, and ptr is returned
	//If ptr is NULL, a new list is created and the new head pointer is returned
	//NULL is returned upon error

void eof_destroy_tempo_list(struct Tempo_change *ptr);
	//If ptr is not NULL, de-allocates all memory consumed by the tempo linked list

unsigned long eof_ConvertToDeltaTime(double realtime,struct Tempo_change *anchorlist,unsigned long timedivision);
	//An adaptation of the ConvertToDeltaTime() function from the FoFLC source
	//Parses a linked list of anchors and returns the delta time of the specified realtime

#endif
