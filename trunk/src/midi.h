#ifndef EOF_MIDI_H
#define EOF_MIDI_H

#include "song.h"
#include "foflc/Midi_parse.h"

#define EOF_MAX_TS 100
#define EOF_MAX_MIDI_EVENTS 65536

typedef struct
{

	unsigned long pos;
	int type;
	int note;
	char * dp;

} EOF_MIDI_EVENT;


typedef struct
{
	unsigned long pos;		//Absolute delta time of this TS change
	double realtime;		//Realtime of this TS change
	unsigned int num;		//The numerator of the TS change (number of beats per measure)
	unsigned int den;		//The denominator of the TS change (note value representing one beat)
	unsigned long track;	//The track number containing this change
} EOF_MIDI_TS_CHANGE;

typedef struct
{
	EOF_MIDI_TS_CHANGE * change[EOF_MAX_TS];	//The list of time signatures changes
	unsigned long changes;					//The number of time signatures changes found in this list
} EOF_MIDI_TS_LIST;

int eof_figure_beat(double pos);				//Returns the beat marker immediately before the specified timestamp, or -1 on failure
double eof_calculate_bpm_absolute(double pos);	//Returns the tempo defined by the beat marker immediately before the specified timestamp, or 0.0 on failure
double eof_calculate_delta(double start, double end);	//Finds the number of delta ticks within the specified time span?
	//Warning: This function does not take the time signature into account

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

unsigned long eof_ConvertToDeltaTime(double realtime,struct Tempo_change *anchorlist,EOF_MIDI_TS_LIST *tslist,unsigned long timedivision);
	//An adaptation of the ConvertToDeltaTime() function from the FoFLC source
	//Parses a linked list of anchors and returns the delta time of the specified realtime

int eof_extract_rba_midi(const char * source, const char * dest);
	//Extracts the MIDI file embedded in the source RBA file and writes it to a regular MIDI file called dest
	//Returns nonzero on error

EOF_MIDI_TS_LIST * eof_create_ts_list(void);
	//Allocates and returns a TS change list
void eof_destroy_ts_list(EOF_MIDI_TS_LIST *ptr);
	//Deallocates the specified TS change list
void eof_midi_add_ts_deltas(EOF_MIDI_TS_LIST * changes, unsigned long pos, unsigned long num, unsigned long den, unsigned long track);
	//Adds the time signature information to the specified list of time signature changes, providing the absolute delta time of the TS change
void eof_midi_add_ts_realtime(EOF_MIDI_TS_LIST * changes, double pos, unsigned long num, unsigned long den, unsigned long track);
	//Adds the time signature information to the specified list of time signature changes, providing the absolute real time of the TS change
EOF_MIDI_TS_LIST *eof_build_ts_list(struct Tempo_change *anchorlist);
	//Parses eof_song->beat[], returning a linked list of time signature changes, or NULL on error
int eof_get_ts(unsigned *num,unsigned *den,int beatnum);
	//If the specified beat number has a defined TS, return the num and den through the passed pointers if they are not NULL
	//Returns 1 if a time signature was returned
	//Returns 0 if the specified beat was not a time signature
	//Returns -1 on error
int eof_apply_ts(unsigned num,unsigned den,int beatnum,EOF_SONG *sp,char undo);
	//Validates and applies the specified time signature to the specified beat
	//If undo is nonzero, then an undo state is made before any changes are made

#endif
