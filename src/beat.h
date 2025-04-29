#ifndef EOF_BEAT_H
#define EOF_BEAT_H

#include "song.h"

extern int eof_beat_stats_cached;
extern int eof_skip_mid_beats_in_measure_numbering;

unsigned long eof_get_beat(EOF_SONG * sp, unsigned long pos);
	//Returns the beat number at or immediately before the specified position, or ULONG_MAX if the timestamp does not occur within the chart (before the first beat or after the last beat)
unsigned long eof_get_nearest_beat(EOF_SONG * sp, unsigned long pos);
	//Returns the beat number that is closest to the specified position, or ULONG_MAX no error
	//If pos is exactly half-way between two beats, the earlier beat is returned
double eof_get_beat_length(EOF_SONG * sp, unsigned long beat);
	//Returns the difference in position between the specified beat marker and the next, or the difference between the last two beat markers if the beat marker specified is invalid
unsigned long eof_find_previous_anchor(EOF_SONG * sp, unsigned long cbeat);
	//Returns the beat number of the last anchor that occurs before the specified beat
unsigned long eof_find_next_anchor(EOF_SONG * sp, unsigned long cbeat);
	//Returns the beat number of the first anchor that occurs after the specified beat, or ULONG_MAX if there is no such anchor
int eof_beat_is_anchor(EOF_SONG * sp, unsigned long cbeat);
	//Returns nonzero if the specified beat number is an anchor based on its flag or a change in tempo from the prior beat
unsigned long eof_calculate_beats_logic(EOF_SONG * sp, int addbeats);
	//Sets the timestamp and anchor status of each beat existing for the duration of the chart by using the tempo map
	//If addbeats is nonzero, creates beats if there aren't enough to extend to one beat past the end of the chart as defined by eof_chart_length
	//Updates eof_chart_length to reflect the last beat's timestamp if the latter is larger
	//Returns nonzero to indicate the number of beats added to the project
unsigned long eof_calculate_beats(EOF_SONG * sp);
	//Calls eof_calculate_beats_logic with the option to add beats as deemed necessary
void eof_calculate_tempo_map(EOF_SONG * sp);
	//Sets the tempo and anchor status of each beat in the EOF_SONG structure by using the configured FLOATING POINT time stamp of each beat
double eof_calculate_beat_pos_by_prev_beat_tempo(EOF_SONG *sp, unsigned long beat);
	//Determines the specified beat's expected position based on the previous beat's defined tempo
	//If the specified beat number is 0, the position returned is the current OGG profile's MIDI delay
int eof_detect_tempo_map_corruption(EOF_SONG *sp, int report);
	//Uses eof_calculate_beat_pos_by_prev_beat_tempo() on each beat to determine if any have an unexpected timestamp
	//If any do, the user is prompted whether to recreeate the tempo changes to reflect the beat timings
	//If the user opts to do so, eof_calculate_tempo_map() is used to carry this out
	//If report is nonzero, and no tempo issues are found, a message is displayed to this effect
void eof_change_accurate_ts(EOF_SONG * sp, char function);
	//Recalculates the tempo on beats using a time signature with a denominator other than 4 depending on the value of function,
	// allowing beats to retain the same real time length
	//If function is nonzero, such beats are calculated without regard to time signature the way EOF originally did
	//If function is zero, such beats are calculated with regard to time signature (ie. beats in 4/8 are shorter than 4/4 if they use the same tempo)
void eof_realign_beats(EOF_SONG * sp, unsigned long cbeat);
	//Recalculates and applies the tempo of the anchors before and after the specified beat, updating beat timestamps, etc.
void eof_recalculate_beats(EOF_SONG * sp, unsigned long cbeat);
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
double eof_get_distance_in_beats(EOF_SONG *sp, unsigned long pos1, unsigned long pos2);
	//Uses eof_get_beat() and eof_get_porpos_sp() to calculate the number of beats between the two specified positions, which could be in either order
	//Returns 0.0 on error
void eof_detect_mid_measure_ts_changes(void);
	//Checks for time signature changes that occur part-way into a measure and warns the user
unsigned long eof_get_text_event_pos_ptr(EOF_SONG *sp, EOF_TEXT_EVENT *ptr);
	//Returns the realtime position of the specified text event, taking into account whether it has a floating position instead of being assigned to a beat marker
	//Returns ULONG_MAX on error
unsigned long eof_get_text_event_pos(EOF_SONG *sp, unsigned long event);
	//Accepts a song structure and event index, returning the event's realtime position determined by eof_get_text_event_pos_ptr()
double eof_get_text_event_fpos_ptr(EOF_SONG *sp, EOF_TEXT_EVENT *ptr);
double eof_get_text_event_fpos(EOF_SONG *sp, unsigned long event);
	//Floating point versions of eof_get_text_event_pos() and eof_get_text_event_fpos()
	//Return 0.0 on error, since DBL_MAX doesn't seem standardized in MinGW

void eof_apply_tempo(unsigned long ppqn, unsigned long beatnum, char adjust);
	//Applies the specified tempo to the specified beat and all subsequent beats up until the next anchor
	//If adjust is nonzero, auto-adjust is performed to suit the tempo change
void eof_remove_ts(unsigned long beatnum);
	//Removes the time signature defined on the specified beat

int eof_check_for_anchors_between_selected_beat_and_seek_pos(void);
	//Returns zero if there are no anchors (excluding the selected beat) between the selected beat and the seek position
	//Returns nonzero if there are anchors or upon error

#endif
