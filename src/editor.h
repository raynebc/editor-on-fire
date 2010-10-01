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
	float grid_pos[32];
	float grid_distance[32];
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
void eof_destroy_waveform(struct wavestruct *ptr);
	//frees memory used by the specified waveform structure
int eof_waveform_slice_mean(struct waveformslice *left,struct waveformslice *right,struct wavestruct *waveform,unsigned long slicestart, unsigned long num);
	//performs a mathematical mean on the specified waveform slice data, returning the results via left and right if they aren't NULL, which will hold the values for the left and right channels, respectively.
	//slice numbering begins with 0
	//returns nonzero on error
int eof_render_waveform(struct wavestruct *waveform);
	//Renders the left channel waveform into the editor window, taking the zoom level into account
	//Returns nonzero on failure
int eof_render_waveform2(struct wavestruct *waveform);
	//Similar to eof_render_waveform, but renders the waveform for both channels in the editor window
void eof_render_waveform_line(struct wavestruct *waveform,struct waveformchanneldata *channel,unsigned amp,unsigned long x,int color);
	//Given the amplitude and the channel and waveform to process, draws the vertical line for the channel's waveform originating from point x and the channel's defined y axis coordinate

void eof_render_editor_window_common(void);
	//Performs rendering common to both the normal and vocal editor windows, before the notes are rendered
void eof_render_editor_window_common2(void);
	//Performs rendering common to both the nromal and vocal editor windows, after the notes are rendered


#endif
