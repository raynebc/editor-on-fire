#ifndef EOF_BEAT_H
#define EOF_BEAT_H

//extern EOF_BEAT_MARKER eof_beat[EOF_MAX_BEATS];
//extern int eof_beats;

long eof_get_beat(EOF_SONG * sp, unsigned long pos);		//Returns the beat number immediately before the specified position, or -1 if the timestamp does not occur within the chart
int eof_get_beat_length(EOF_SONG * sp, int beat);		//Returns the difference in position between the specified beat marker and the next, or the difference between the last two beat markers if the beat marker specified is invalid
unsigned long eof_find_previous_anchor(EOF_SONG * sp, unsigned long cbeat);	//Returns the beat number of the last anchor that occurs before the specified beat
long eof_find_next_anchor(EOF_SONG * sp, unsigned long cbeat);		//Returns the beat number of the first anchor that occurs after the specified beat, or -1 if there is no such anchor
int eof_beat_is_anchor(EOF_SONG * sp, int cbeat);		//Returns nonzero if the specified beat number is an anchor based on its flag or a change in tempo from the prior beat, or if the specified beat number is negative
void eof_calculate_beats(EOF_SONG * sp);				//Sets the timestamp of each beat existing for the duration of the chart audio
void eof_realign_beats(EOF_SONG * sp, int cbeat);		//Recalculates and applies the tempo of the anchors before and after the specified beat, updating beat timestamps, etc.
void eof_recalculate_beats(EOF_SONG * sp, int cbeat);	//Recalculates and applies the tempo and timings on both sides of cbeat based on the previous and next anchors, such as when dragging a beat marker to define cbeat as an anchor

#endif
