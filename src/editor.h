#ifndef EOF_EDITOR_H
#define EOF_EDITOR_H

#include "song.h"

typedef struct
{

	/* which track the selection pertains to */
	int track;

	/* the current selection */
	int current;
	unsigned long current_pos;

	/* the previous selection */
	int last;
	unsigned long last_pos;

	/* which notes are selected */
	char multi[EOF_MAX_NOTES];

	/* range data */
	unsigned long range_pos_1;
	unsigned long range_pos_2;

} EOF_SELECTION_DATA;

typedef struct
{

	int beat;
	int beat_length;
	int measure_beat;
	int measure_length;
	int length;
	int numerator, denominator;
	float grid_pos[EOF_MAX_GRID_SNAP_INTERVALS];
	float grid_distance[EOF_MAX_GRID_SNAP_INTERVALS];
	unsigned long pos;

} EOF_SNAP_DATA;

extern EOF_SELECTION_DATA eof_selection;
extern EOF_SNAP_DATA eof_snap;
extern EOF_SNAP_DATA eof_tail_snap;

void eof_select_beat(int beat);
void eof_set_tail_pos(int note, unsigned long pos);
void eof_snap_logic(EOF_SNAP_DATA * sp, unsigned long p);
void eof_snap_length_logic(EOF_SNAP_DATA * sp);
void eof_read_editor_keys(void);
void eof_editor_logic(void);
void eof_editor_drum_logic(void);	//The drum record mode logic
void eof_vocal_editor_logic(void);
void eof_render_editor_window(void);
void eof_render_vocal_editor_window(void);

unsigned long eof_determine_piano_roll_left_edge(void);
	//calculates the timestamp of the left visible edge of the piano roll
unsigned long eof_determine_piano_roll_right_edge(void);
	//calculates the timestamp of the right visible edge of the piano roll

void eof_render_editor_window_common(void);
	//Performs rendering common to both the normal and vocal editor windows, before the notes are rendered
void eof_render_editor_window_common2(void);
	//Performs rendering common to both the nromal and vocal editor windows, after the notes are rendered

void eof_mark_new_note_as_cymbal(EOF_SONG *sp, unsigned long tracknum, unsigned long notenum);
	//Checks if the newly-created note is in PART DRUMS, if so, and the new note contains
	//yellow, blue or green pad, cymbal status is applied to this note position accordingly
void eof_mark_edited_note_as_cymbal(EOF_SONG *sp, unsigned long tracknum, unsigned long notenum, unsigned long bitmask);
	//Performs similar logic to eof_mark_new_note_as_cymbal(), but only applies cymbal status to
	//the frets based on the specified bitmask, ie. only applying the status of the cymbal that
	//was added to the drum note

#endif