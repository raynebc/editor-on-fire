#ifndef EOF_EDITOR_H
#define EOF_EDITOR_H

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

#endif
