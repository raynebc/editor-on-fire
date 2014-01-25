#ifndef EOF_EDITOR_H
#define EOF_EDITOR_H

#include "song.h"

typedef struct
{

	/* which track the selection pertains to */
	unsigned long track;

	/* the current selection */
	long current;	//EOF_MAX_NOTES - 1 is used to represent no notes selected
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
	unsigned long pos, previous_snap, next_snap;
	int intervals;	//The number of grid snaps defined in the above arrays

} EOF_SNAP_DATA;

extern EOF_SNAP_DATA eof_snap;
extern EOF_SNAP_DATA eof_tail_snap;

void eof_select_beat(unsigned long beat);	//Updates eof_selected_measure, eof_beat_in_measure, eof_beats_in_measure and eof_selected_beat to reflect the specified beat number
void eof_snap_logic(EOF_SNAP_DATA * sp, unsigned long p);
	//Accepts a timestamp, and updates sp->grid_pos[] with the positions of each grid snap immediately at/before the timestamp and all remaining grid snaps for the beat/measure (depending on which of the two the grid snap setting is set to use)
	//The position of the grid snap nearest to the given timestamp is stored in sp->pos
	//sp->previous_snap and sp->next_snap are populated with the grid snap positions before/after sp->pos, with the limitation that previous_snap is no earlier than the beat's position itself (it won't go into the previous beat)
void eof_snap_length_logic(EOF_SNAP_DATA * sp);	//Calculates the grid snap interval length for sp->beat
void eof_read_editor_keys(void);
void eof_editor_logic(void);
void eof_editor_drum_logic(void);	//The drum record mode logic
void eof_vocal_editor_logic(void);

unsigned long eof_determine_piano_roll_left_edge(void);
	//calculates the timestamp of the left visible edge of the piano roll
unsigned long eof_determine_piano_roll_right_edge(void);
	//calculates the timestamp of the right visible edge of the piano roll

void eof_render_editor_window(EOF_WINDOW *window, unsigned char windownum);
	//Renders to the specified editor window
	//Calls eof_render_vocal_editor_window() instead if a vocal track is to be rendered
	//windownum specifies whether the first or the second piano roll is being drawn.  If it's the first one, and tech view is in effect, the tech notes are drawn on top of the regular notes
void eof_render_vocal_editor_window(EOF_WINDOW *window);
	//Renders to the specified editor window
void eof_render_editor_window_2(void);
	//If the user opted to display the secondary piano roll, temporarily swaps the active track difficulty with that of the secondary piano roll and renders it
unsigned long eof_get_number_displayed_tabs(void);
	//Determines how many tabs are to be rendered for the active track, based on the program window width and whether the difficulty limit has been removed from the track
void eof_render_editor_window_common(EOF_WINDOW *window);
	//Performs rendering common to both the normal and vocal editor windows, before the notes are rendered
void eof_render_editor_window_common2(EOF_WINDOW *window);
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
void eof_mark_edited_note_as_double_bass(EOF_SONG *sp, unsigned long track, unsigned long notenum, unsigned long bitmask);
	//Performs similar logic to eof_mark_new_note_as_double_bass(), but only applies double bass status
	//to the frets based on the specified bitmask, ie. only applying the status to a bass drum note
	//that was added to an existing drum note
void eof_mark_new_note_as_special_hi_hat(EOF_SONG *sp, unsigned long track, unsigned long notenum);
	//Checks if the newly-created note is in the PS drum track, if so, and the new note contains
	//yellow, the status stored in eof_mark_drums_as_hi_hat is applied to this note
	//if eof_drum_modifiers_affect_all_difficulties is true, existing yellow gems at identical positions
	//in lower difficulties are likewise turned into the specified hi hat type
void eof_mark_edited_note_as_special_hi_hat(EOF_SONG *sp, unsigned long track, unsigned long notenum, unsigned long bitmask);
	//Performs similar logic to eof_mark_new_note_as_special_hi_hat(), but only applies the hi hat status
	//to the frets based on the specified bitmask, ie. only applying the status to a yellow gem
	//that was added to an existing drum note

unsigned char eof_find_pen_note_mask(void);
	//For the current track, sets eof_hover_piece appropriately and returns the pen note mask,
	//based on the value of eof_hover_note, eof_inverted_notes and the number of lanes in the active track

void eof_editor_logic_common(void);
	//Performs editor logic common to both the editor and vocal editor windows (some variable initialization, handling beat marker, seek bar and playback controls)
float eof_pos_distance(float p1, float p2);
	//Returns the difference between the two floating point numbers as a positive value
void eof_get_snap_ts(EOF_SNAP_DATA * sp, int beat);
	//Finds the time signature in effect for the specified beat and stores the numerator and denominator into sp
int eof_get_ts_text(int beat, char * buffer);
	//Writes a string into the provided buffer that represents the time signature on the specified beat (string will be empty if the beat has no time signature defined explicitly on it)
	//Returns zero on error or if the beat does not contain a time signature change, otherwise nonzero is returned
int eof_get_tempo_text(int beat, char * buffer);
	//Writes a string into the provided buffer that represents the tempo of the specified beat, followed by a trailing space
	//Returns 0 on error

void eof_seek_to_nearest_grid_snap(void);
	//Used in Feedback input method to seek to the nearest grid snap position when playback is stopped, provided there's a grid snap selected and the current seek position is valid.
int eof_find_hover_note(int targetpos, int x_tolerance, char snaplogic);
	//Finds and returns an appropriate hover note based on the specified position and the given x_tolerance
	//If snaplogic is nonzero, also checks snap position data (should only be needed for tracking by mouse position)
	//Returns -1 if no note was close enough to the specified position in the current instrument difficulty
void eof_update_seek_selection(unsigned long start, unsigned long stop, char deselect);
	//Updates the seek selection variables and the note selection array to reflect which range of notes are selected via keyboard seek controls
	//If deselect is nonzero, the note selection is cleared and the function returns without selecting notes between the start and stop positions
void eof_feedback_input_mode_update_selected_beat(void);
	//If Feedback input mode is in use, update eof_selected_beat and measure to correspond with the beat at or immediately before the current seek position

unsigned char eof_set_active_difficulty(unsigned char diff);
	//Changes the active difficulty of the active track, if the given difficulty is valid
	//Returns the maximum number of difficulty levels supported by the active track

#endif
