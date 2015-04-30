#ifndef EOF_BEAT_H
#define EOF_BEAT_H

#include "song.h"

extern int eof_beat_stats_cached;

long eof_get_beat(EOF_SONG * sp, unsigned long pos);
	//Returns the beat number at or immediately before the specified position, or -1 if the timestamp does not occur within the chart
double eof_get_beat_length(EOF_SONG * sp, unsigned long beat);
	//Returns the difference in position between the specified beat marker and the next, or the difference between the last two beat markers if the beat marker specified is invalid
unsigned long eof_find_previous_anchor(EOF_SONG * sp, unsigned long cbeat);
	//Returns the beat number of the last anchor that occurs before the specified beat
long eof_find_next_anchor(EOF_SONG * sp, unsigned long cbeat);
	//Returns the beat number of the first anchor that occurs after the specified beat, or -1 if there is no such anchor
int eof_beat_is_anchor(EOF_SONG * sp, int cbeat);
	//Returns nonzero if the specified beat number is an anchor based on its flag or a change in tempo from the prior beat, or if the specified beat number is negative
void eof_calculate_beats(EOF_SONG * sp);
	//Sets the timestamp and anchor status of each beat existing for the duration of the chart by using the tempo map
	//Creates beats if there aren't enough to extend to one beat past the end of the chart as defined by eof_chart_length
void eof_calculate_tempo_map(EOF_SONG * sp);
	//Sets the tempo and anchor status of each beat in the EOF_SONG structure by using the configured time stamp of each beat
void eof_realign_beats(EOF_SONG * sp, int cbeat);
	//Recalculates and applies the tempo of the anchors before and after the specified beat, updating beat timestamps, etc.
void eof_recalculate_beats(EOF_SONG * sp, int cbeat);
	//Recalculates and applies the tempo and timings on both sides of cbeat based on the previous and next anchors, such as when dragging a beat marker to define cbeat as an anchor
EOF_BEAT_MARKER * eof_song_add_beat(EOF_SONG * sp);
	//Allocates, initializes and stores a new EOF_BEAT_MARKER structure into the beats array.  Returns the newly allocated structure or NULL upon error
void eof_song_delete_beat(EOF_SONG * sp, unsigned long beat);
	//Removes and frees the specified beat from the beats array.  All beats after the deleted beat are moved back in the array one position
int eof_song_resize_beats(EOF_SONG * sp, unsigned long beats);
	//Grows or shrinks the beats array to fit the specified number of beats by allocating/freeing EOF_BEAT_MARKER structures.  Returns zero on error
int eof_song_append_beats(EOF_SONG * sp, unsigned long beats);
	//Adds the specified number of beats to the chart, initializing each to be the same tempo and length as the chart's current last beat.
	//If there are no beats already, the first beat added is set to 500ms long (ie. 120BPM).  Returns zero on error.
void eof_double_tempo(EOF_SONG * sp, unsigned long beat, char *undo_made);
	//Doubles the tempo on the specified beat, effectively doubling the number of beats up to the next anchor
	//If *undo_made is zero, this function will create an undo state before modifying the chart and will set the referenced variable to nonzero
	//If undo_made is NULL, the undo state will be made
int eof_halve_tempo(EOF_SONG * sp, unsigned long beat, char *undo_made);
	//Halves the tempo on the specified beat, effectively halving the number of beats up to the next anchor.
	//If the number of beats until the next anchor is odd, the beat preceding the anchor will be anchored and
	//will keep its existing tempo in order to keep the tempo map accurate, and -1 will be returned.  If only
	//an even number of beats is altered, 0 is returned
	//If *undo_made is zero, this function will create an undo state before modifying the chart and will set the referenced variable to nonzero
	//If undo_made is NULL, the undo state will be made
unsigned long eof_get_measure(unsigned long measure, unsigned char count_only);
	//If count_only is nonzero, the number of measures present in the currently open chart is returned (0 if no time signatures are defined)
	//If count_only is zero, the beat number that is at the start of the specified measure is returned (or 0 if no such measure is present)
void eof_process_beat_statistics(EOF_SONG * sp, unsigned long track);
	//Parses the beat[] array of the specified chart and stores various information for each beat:
	//The beat's measure number (or 0 if no TS is in effect), beat within measure, total number of beats in current measure are stored,
	//The section text event assigned to the beat is stored (from the perspective of the specified track, or -1 if no section event),
	//a boolean status for whether the beat contains an "[end]" event and boolean statuses for whether the beat contains tempo or TS changes

#endif
