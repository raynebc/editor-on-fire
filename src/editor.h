#ifndef EOF_EDITOR_H
#define EOF_EDITOR_H

#include "song.h"

typedef struct
{

	/* which track the selection pertains to */
	unsigned long track;

	/* the current selection */
	long current;	//-1 is used to represent no notes selected?
	unsigned long current_pos;

	/* the previous selection */
	long last;
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
void eof_snap_logic(EOF_SNAP_DATA * sp, unsigned long p);
void eof_snap_length_logic(EOF_SNAP_DATA * sp);	//Calculates the grid snap interval length for sp->beat
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

void eof_mark_new_note_as_cymbal(EOF_SONG *sp, unsigned long track, unsigned long notenum);
	//Checks if the newly-created note is in PART DRUMS, if so, and the new note contains
	//yellow, blue or green pad, cymbal status is applied to this note position accordingly
void eof_mark_edited_note_as_cymbal(EOF_SONG *sp, unsigned long track, unsigned long notenum, unsigned long bitmask);
	//Performs similar logic to eof_mark_new_note_as_cymbal(), but only applies cymbal status to
	//the frets based on the specified bitmask, ie. only applying the status of the cymbal that
	//was added to the drum note
void eof_mark_new_note_as_double_bass(EOF_SONG *sp, unsigned long track, unsigned long notenum);
	//Checks if the newly-created note is in PART DRUMS, if so, and the new note is an expert
	//bass note, expert+ status is applied to this note

unsigned char eof_find_pen_note_mask(void);
	//For the current track, sets eof_hover_piece appropriately and returns the pen note mask,
	//based on the value of eof_hover_note, eof_inverted_notes and the number of lanes in the active track

void eof_editor_logic_common(void);
	//Performs editor logic common to both the editor and vocal editor windows (some variable initialization, handling beat marker, seek bar and playback controls)


#endif
