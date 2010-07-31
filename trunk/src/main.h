#ifndef EOF_MAIN_H
#define EOF_MAIN_H

#include <allegro.h>
#include "alogg/include/alogg.h"
#include "modules/wfsel.h"
#include "window.h"
#include "note.h"
#include "control.h"
#include "editor.h"

#define EOF_VERSION_STRING "EOF v1.65"
#define EOF_COPYRIGHT_STRING "(c)2008-2010 T^3 Software."

#define KEY_EITHER_ALT (key[KEY_ALT] || key[KEY_ALTGR])
#define KEY_EITHER_CTRL (key[KEY_LCONTROL] || key[KEY_RCONTROL])
#define KEY_EITHER_SHIFT (key[KEY_LSHIFT] || key[KEY_RSHIFT])

#define EOF_SNAP_OFF           0
#define EOF_SNAP_QUARTER       1
#define EOF_SNAP_EIGHTH        2
#define EOF_SNAP_TWELFTH       3
#define EOF_SNAP_SIXTEENTH     4
#define EOF_SNAP_TWENTY_FOURTH 5
#define EOF_SNAP_THIRTY_SECOND 6
#define EOF_SNAP_FORTY_EIGHTH  7
#define EOF_SNAP_CUSTOM        8

#define EOF_HOPO_RF            0
#define EOF_HOPO_FOF           1
#define EOF_HOPO_OFF           2

#define EOF_INPUT_CLASSIC      0
#define EOF_INPUT_PIANO_ROLL   1
#define EOF_INPUT_HOLD         2
#define EOF_INPUT_REX          3
#define EOF_INPUT_GUITAR_TAP   4
#define EOF_INPUT_GUITAR_STRUM 5
#define EOF_INPUT_FEEDBACK     6

#define EOF_MAX_IMAGES 64

#define EOF_IMAGE_WAVE              0
#define EOF_IMAGE_TAB0              1
#define EOF_IMAGE_TAB1              2
#define EOF_IMAGE_TAB2              3
#define EOF_IMAGE_TAB3              4
#define EOF_IMAGE_TAB4              5
#define EOF_IMAGE_TABE              6
#define EOF_IMAGE_NOTE_GREEN        7
#define EOF_IMAGE_NOTE_RED          8
#define EOF_IMAGE_NOTE_YELLOW       9
#define EOF_IMAGE_NOTE_BLUE        10
#define EOF_IMAGE_NOTE_PURPLE      11
#define EOF_IMAGE_NOTE_GREEN_HIT   12
#define EOF_IMAGE_NOTE_RED_HIT     13
#define EOF_IMAGE_NOTE_YELLOW_HIT  14
#define EOF_IMAGE_NOTE_BLUE_HIT    15
#define EOF_IMAGE_NOTE_PURPLE_HIT  16
#define EOF_IMAGE_CONTROLS_BASE    17
#define EOF_IMAGE_CONTROLS_0       18
#define EOF_IMAGE_CONTROLS_1       19
#define EOF_IMAGE_CONTROLS_2       20
#define EOF_IMAGE_CONTROLS_3       21
#define EOF_IMAGE_CONTROLS_4       22
#define EOF_IMAGE_MENU             23
#define EOF_IMAGE_MENU_FULL        24
#define EOF_IMAGE_NOTE_HGREEN      25
#define EOF_IMAGE_NOTE_HRED        26
#define EOF_IMAGE_NOTE_HYELLOW     27
#define EOF_IMAGE_NOTE_HBLUE       28
#define EOF_IMAGE_NOTE_HPURPLE     29
#define EOF_IMAGE_NOTE_HGREEN_HIT  30
#define EOF_IMAGE_NOTE_HRED_HIT    31
#define EOF_IMAGE_NOTE_HYELLOW_HIT 32
#define EOF_IMAGE_NOTE_HBLUE_HIT   33
#define EOF_IMAGE_NOTE_HPURPLE_HIT 34
#define EOF_IMAGE_SCROLL_BAR       35
#define EOF_IMAGE_SCROLL_HANDLE    36
#define EOF_IMAGE_CCONTROLS_BASE   37
#define EOF_IMAGE_CCONTROLS_0      38
#define EOF_IMAGE_CCONTROLS_1      39
#define EOF_IMAGE_CCONTROLS_2      40
#define EOF_IMAGE_NOTE_WHITE       41
#define EOF_IMAGE_NOTE_WHITE_HIT   42
#define EOF_IMAGE_NOTE_HWHITE      43
#define EOF_IMAGE_NOTE_HWHITE_HIT  44
#define EOF_IMAGE_MENU_NO_NOTE     45
#define EOF_IMAGE_VTAB0            46
#define EOF_IMAGE_LYRIC_SCRATCH    47

#define EOF_DISPLAY_640             0
#define EOF_DISPLAY_800             1
#define EOF_DISPLAY_1024            2

#define EOF_INPUT_NAME_NUM			7
	//The number of defined input methods

typedef struct
{

	int mode;

	/* rendering offsets */
	int scrollbar_y;
	int string_space;
	int note_y[5];
	int lyric_y;
	int vocal_y;
	int vocal_tail_size;
	int vocal_view_size;
	int lyric_view_key_width;
	int lyric_view_key_height;
	int lyric_view_bkey_width;
	int lyric_view_bkey_height;
	int note_size;
	int note_dot_size;
	int note_marker_size;
	int note_tail_size;
	int note_kick_size;
	int fretboard_h;
	int buffered_preview;
	int controls_x;

	/* editor mouse click areas */


} EOF_SCREEN_LAYOUT;

#define EOF_EDITOR_RENDER_OFFSET  56

struct MIDIentry
{
	unsigned char status;	//The current entry status (0=note not playing, 1=note playing)
	unsigned char note;		//The MIDI note to be played
	int startpos;			//The time at which the MIDI tone is to be started
	int endpos;				//The time at which the MIDI tone is to be stopped
	struct MIDIentry *prev;	//The previous link in the list
	struct MIDIentry *next;	//The next link in the list
};
extern struct MIDIentry *MIDIqueue;		//Linked list of queued MIDI notes
extern struct MIDIentry *MIDIqueuetail;	//Points to the tail of the list

extern NCDFS_FILTER_LIST * eof_filter_music_files;
extern NCDFS_FILTER_LIST * eof_filter_ogg_files;
extern NCDFS_FILTER_LIST * eof_filter_midi_files;
extern NCDFS_FILTER_LIST * eof_filter_eof_files;
extern NCDFS_FILTER_LIST * eof_filter_exe_files;
extern NCDFS_FILTER_LIST * eof_filter_lyrics_files;
extern NCDFS_FILTER_LIST * eof_filter_dB_files;

extern PALETTE     eof_palette;
extern BITMAP *    eof_image[EOF_MAX_IMAGES];

extern EOF_WINDOW * eof_window_editor;
extern EOF_WINDOW * eof_window_note;
extern EOF_WINDOW * eof_window_3d;

extern char        eof_last_frettist[256];
extern char      * eof_track_name[EOF_MAX_TRACKS + 1];
extern int         eof_zoom;
extern int         eof_zoom_3d;
extern char        eof_changes;
extern ALOGG_OGG * eof_music_track;
extern void      * eof_music_data;
extern int         eof_music_data_size;
//extern ALOGG_OGG * eof_bgmusic_track;
//extern void      * eof_bgmusic_data;
//extern int         eof_bgmusic_data_size;
extern int         eof_music_length;
extern int         eof_music_actual_length;
extern int         eof_music_pos;
extern int         eof_music_actual_pos;
extern int         eof_music_rewind_pos;
extern int         eof_music_catalog_pos;
extern int         eof_music_end_pos;
extern int         eof_music_paused;
extern char        eof_music_catalog_playback;
extern char        eof_play_selection;
extern int         eof_selected_ogg;
extern EOF_SONG    * eof_song;
//extern EOF_EXTENDED_NOTE eof_note_clipboard[EOF_MAX_NOTES];
//extern char        eof_note_selected[EOF_MAX_NOTES];
//extern char        eof_note_clipboard_type;
extern EOF_NOTE    eof_pen_note;
extern EOF_LYRIC   eof_pen_lyric;
extern char        eof_filename[1024];
extern char        eof_song_path[1024];
extern char        eof_songs_path[1024];
extern char        eof_window_title[4096];
extern char        eof_last_ogg_path[1024];
extern char        eof_last_eof_path[1024];
extern char        eof_loaded_song_name[1024];
extern int         eof_quit;
extern EOF_SCREEN_LAYOUT eof_screen_layout;
extern BITMAP *    eof_screen;
extern int         eof_av_delay;
extern int         eof_buffer_size;
extern int         eof_audio_fine_tune;
extern int         eof_inverted_notes;
extern int         eof_lefty_mode;
extern int         eof_note_auto_adjust;
extern int         eof_smooth_pos;
extern int         eof_windowed;
extern int         eof_anchor_all_beats;
extern int         eof_disable_windows;
extern int         eof_disable_vsync;
extern int         eof_playback_speed;
extern char        eof_ogg_setting;
extern char      * eof_ogg_quality[6];
extern char        eof_cpu_saver;
extern int         eof_note_type;
extern int         eof_note_difficulties[5];
extern int         eof_selected_track;
extern int         eof_vocals_selected;
extern int         eof_vocals_offset;
extern int         eof_vocals_tab;
extern int         eof_song_loaded;
extern char        eof_fof_executable_path[1024];
extern char        eof_fof_executable_name[1024];
extern char        eof_fof_songs_path[1024];
extern char        eof_temp_filename[1024];
extern char        eof_soft_cursor;
extern char        eof_desktop;
extern int         eof_input_mode;
extern int         eof_undo_toggle;
extern int         eof_redo_toggle;
extern int         eof_change_count;
extern int         eof_anchor_diff[EOF_MAX_TRACKS + 1];
extern int         eof_last_note;
extern int         eof_last_midi_offset;
//extern int         eof_default_length;
extern int         eof_notes_moved;
extern EOF_NOTE *  eof_entering_note_note;
extern EOF_LYRIC *  eof_entering_note_lyric;
extern int         eof_entering_note;
extern int         eof_snote;

/* mouse control data */
extern int         eof_selected_control;
extern int         eof_cselected_control;
//extern int         eof_selected_note;
extern int         eof_selected_catalog_entry;
//extern int         eof_selected_note_pos;
extern int         eof_selected_beat;
extern int         eof_selected_measure;
extern int         eof_beat_in_measure;
extern int         eof_beats_in_measure;
//extern int         eof_last_selected_note;
//extern int         eof_last_selected_note_pos;
extern int         eof_pegged_note;
extern int         eof_hover_note;
extern int         eof_hover_note_2;
extern int         eof_hover_beat;
extern int         eof_hover_beat_2;
extern int         eof_hover_piece;
extern int         eof_hover_key;
extern int         eof_hover_lyric;
extern int         eof_last_tone;
extern int         eof_mouse_z;
extern int         eof_mickey_z;
extern int         eof_mickeys_x;
extern int         eof_mickeys_y;
extern int         eof_lclick_released;
extern int         eof_blclick_released;
extern int         eof_rclick_released;
extern int         eof_click_x;
extern int         eof_click_y;
extern int         eof_peg_x;
extern int         eof_peg_y;
extern int         eof_last_pen_pos;
extern int         eof_cursor_visible;
extern int         eof_pen_visible;
extern int         eof_hover_type;
extern int         eof_mouse_drug;
extern int         eof_menu_key_waiting;

extern char          eof_snap_mode;
extern char          eof_last_snap_mode;
extern int           eof_snap_interval;
extern char			eof_custom_snap_measure;
//extern int           eof_snap_beat;
//extern float         eof_snap_pos[32];
//extern float         eof_snap_distance[32];
//extern int           eof_snap_length;

extern char          eof_hopo_view;

extern EOF_CONTROLLER eof_guitar;
extern EOF_CONTROLLER eof_drums;
extern int eof_color_black;
extern int eof_color_white;
extern int eof_color_gray;
extern int eof_color_red;
extern int eof_color_green;
extern int eof_color_blue;
extern int eof_color_yellow;
extern int eof_color_purple;
extern int eof_info_color;

extern PALETTE     eof_palette;
extern BITMAP *    eof_image[EOF_MAX_IMAGES];
extern FONT *      eof_font;
extern FONT *      eof_mono_font;
extern char        eof_note_type_name[5][32];
extern char        eof_vocal_tab_name[5][32];

extern char        eof_supports_mp3;
extern char        eof_just_played;

extern char eof_midi_initialized;	//Specifies whether Allegro was able to set up a MIDI device

extern EOF_SELECTION_DATA eof_selection;

void eof_show_mouse(BITMAP * bp);	//Shows the software mouse if it is being used
float eof_get_porpos(unsigned long pos);	//Returns the timestamps position within a beat (percentage)?
unsigned long eof_put_porpos(int beat, float porpos, float offset);
void eof_reset_lyric_preview_lines(void);	//Resets the preview line variables to 0
void eof_find_lyric_preview_lines(void);	//Sets the first and second preview line variables
void eof_emergency_stop_music(void);	//Stops audio playback
void eof_fix_catalog_selection(void);	//Ensures that a valid catalog entry is active, if any
int eof_count_selected_notes(int * total, char v);
	//Returns the number of notes selected in the active difficulty
	//If total is not NULL, its value is incremeneted once for each note in the active difficulty
void eof_clear_input(void);
void eof_prepare_undo(int type);
int eof_count_notes(void);
int eof_figure_difficulty(void);
int eof_figure_part(void);	//Returns the active track number, or -1 if the active track has no notes/lyrics
int d_hackish_edit_proc (int msg, DIALOG *d, int c);
int eof_set_display_mode(int mode);
void eof_debug_message(char * text);
void eof_determine_hopos(void);	//Re-applies the HOPO and SP status of each note in the selected track, as well as deleting empty SP and Solo phrases
void eof_fix_window_title(void);
int eof_load_ogg_quick(char * filename);
int eof_load_ogg(char * filename);
int eof_load_complete_song(char * filename);
int eof_destroy_ogg(void);
int eof_save_ogg(char * fn);
void eof_render(void);
void eof_reset_song(void);
int eof_load_data(void);	//Loads graphics and fonts from eof.dat
void eof_destroy_data(void);	//Frees graphics and fonts from memory
char * eof_get_tone_name(int tone);	//Returns the name of the given note number (ie. C# or Db) based on the value of eof_display_flats
void eof_process_midi_queue(int currentpos);	//Process the MIDI queue based on the current chart timestamp (eof_music_pos)
int eof_midi_queue_add(unsigned char note,int startpos,int endpos);	//Appends the Note On/Off data to the MIDI queue
void eof_midi_queue_destroy(void);	//Destroys the MIDI queue
void eof_exit(void);

#endif
