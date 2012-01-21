#ifndef EOF_BEAT_H
#define EOF_BEAT_H

//extern EOF_BEAT_MARKER eof_beat[EOF_MAX_BEATS];
//extern int eof_beats;

long eof_get_beat(EOF_SONG * sp, unsigned long pos);
	//Returns the beat number immediately before the specified position, or -1 if the timestamp does not occur within the chart
long eof_get_beat_length(EOF_SONG * sp, int beat);
	//Returns the difference in position between the specified beat marker and the next, or the difference between the last two beat markers if the beat marker specified is invalid
unsigned long eof_find_previous_anchor(EOF_SONG * sp, unsigned long cbeat);
	//Returns the beat number of the last anchor that occurs before the specified beat
long eof_find_next_anchor(EOF_SONG * sp, unsigned long cbeat);
	//Returns the beat number of the first anchor that occurs after the specified beat, or -1 if there is no such anchor
int eof_beat_is_anchor(EOF_SONG * sp, int cbeat);
	//Returns nonzero if the specified beat number is an anchor based on its flag or a change in tempo from the prior beat, or if the specified beat number is negative
void eof_calculate_beats(EOF_SONG * sp);
	//Sets the timestamp of each beat existing for the duration of the chart audio
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
void eof_double_tempo(EOF_SONG * sp, unsigned long beat, char make_undo);
	//Doubles the tempo on the specified beat, effectively doubling the number of beats up to the next anchor
	//If make_undo is nonzero, this function will create an undo state before modifying the chart
int eof_halve_tempo(EOF_SONG * sp, unsigned long beat, char make_undo);
	//Halves the tempo on the specified beat, effectively halving the number of beats up to the next anchor.
	//If the number of beats until the next anchor is odd, the beat preceding the anchor will be anchored and
	//will keep its existing tempo in order to keep the tempo map accurate, and -1 will be returned.  If only
	//an even number of beats is altered, 0 is returned
	//If make_undo is nonzero, this function will create an undo state before modifying the chart

#endif
