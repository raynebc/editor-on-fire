#ifndef EOF_MAIN_H
#define EOF_MAIN_H

#include <allegro.h>
#include "alogg/include/alogg.h"
#include "modules/wfsel.h"
#include "window.h"
#include "note.h"
#include "control.h"
#include "editor.h"

#define EOF_VERSION_STRING "EOF v1.8beta38"
#define EOF_COPYRIGHT_STRING "(c)2008-2010 T^3 Software."

#define KEY_EITHER_ALT (key[KEY_ALT] || key[KEY_ALTGR])
#ifdef ALLEGRO_MACOSX
	#define KEY_EITHER_CTRL (key[106])
#else
	#define KEY_EITHER_CTRL (key[KEY_LCONTROL] || key[KEY_RCONTROL])
#endif
#define KEY_EITHER_SHIFT (key[KEY_LSHIFT] || key[KEY_RSHIFT])

#ifdef ALLEGRO_MACOSX
	#define CTRL_NAME "Cmd"
#else
	#define CTRL_NAME "Ctrl"
#endif

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
#define EOF_HOPO_MANUAL        3
#define EOF_NUM_HOPO_MODES     4

#define EOF_INPUT_CLASSIC      0
#define EOF_INPUT_PIANO_ROLL   1
#define EOF_INPUT_HOLD         2
#define EOF_INPUT_REX          3
#define EOF_INPUT_GUITAR_TAP   4
#define EOF_INPUT_GUITAR_STRUM 5
#define EOF_INPUT_FEEDBACK     6

#define EOF_COLORS_DEFAULT     0
#define EOF_COLORS_RB          1
#define EOF_COLORS_GH          2

#define EOF_MAX_IMAGES 80

#define EOF_IMAGE_WAVE                    0
#define EOF_IMAGE_TAB0                    1
#define EOF_IMAGE_TAB1                    2
#define EOF_IMAGE_TAB2                    3
#define EOF_IMAGE_TAB3                    4
#define EOF_IMAGE_TAB4                    5
#define EOF_IMAGE_TABE                    6
#define EOF_IMAGE_NOTE_GREEN              7
#define EOF_IMAGE_NOTE_RED                8
#define EOF_IMAGE_NOTE_YELLOW             9
#define EOF_IMAGE_NOTE_BLUE              10
#define EOF_IMAGE_NOTE_PURPLE            11
#define EOF_IMAGE_NOTE_GREEN_HIT         12
#define EOF_IMAGE_NOTE_RED_HIT           13
#define EOF_IMAGE_NOTE_YELLOW_HIT        14
#define EOF_IMAGE_NOTE_BLUE_HIT          15
#define EOF_IMAGE_NOTE_PURPLE_HIT        16
#define EOF_IMAGE_CONTROLS_BASE          17
#define EOF_IMAGE_CONTROLS_0             18
#define EOF_IMAGE_CONTROLS_1             19
#define EOF_IMAGE_CONTROLS_2             20
#define EOF_IMAGE_CONTROLS_3             21
#define EOF_IMAGE_CONTROLS_4             22
#define EOF_IMAGE_MENU                   23
#define EOF_IMAGE_MENU_FULL              24
#define EOF_IMAGE_NOTE_HGREEN            25
#define EOF_IMAGE_NOTE_HRED              26
#define EOF_IMAGE_NOTE_HYELLOW           27
#define EOF_IMAGE_NOTE_HBLUE             28
#define EOF_IMAGE_NOTE_HPURPLE           29
#define EOF_IMAGE_NOTE_HGREEN_HIT        30
#define EOF_IMAGE_NOTE_HRED_HIT          31
#define EOF_IMAGE_NOTE_HYELLOW_HIT       32
#define EOF_IMAGE_NOTE_HBLUE_HIT         33
#define EOF_IMAGE_NOTE_HPURPLE_HIT       34
#define EOF_IMAGE_SCROLL_BAR             35
#define EOF_IMAGE_SCROLL_HANDLE          36
#define EOF_IMAGE_CCONTROLS_BASE         37
#define EOF_IMAGE_CCONTROLS_0            38
#define EOF_IMAGE_CCONTROLS_1            39
#define EOF_IMAGE_CCONTROLS_2            40
#define EOF_IMAGE_NOTE_WHITE             41
#define EOF_IMAGE_NOTE_WHITE_HIT         42
#define EOF_IMAGE_NOTE_HWHITE            43
#define EOF_IMAGE_NOTE_HWHITE_HIT        44
#define EOF_IMAGE_MENU_NO_NOTE           45
#define EOF_IMAGE_VTAB0                  46
#define EOF_IMAGE_LYRIC_SCRATCH          47
#define EOF_IMAGE_NOTE_YELLOW_CYMBAL     48
#define EOF_IMAGE_NOTE_YELLOW_CYMBAL_HIT 49
#define EOF_IMAGE_NOTE_BLUE_CYMBAL       50
#define EOF_IMAGE_NOTE_BLUE_CYMBAL_HIT   51
#define EOF_IMAGE_NOTE_PURPLE_CYMBAL     52
#define EOF_IMAGE_NOTE_PURPLE_CYMBAL_HIT 53
#define EOF_IMAGE_NOTE_WHITE_CYMBAL      54
#define EOF_IMAGE_NOTE_WHITE_CYMBAL_HIT  55
#define EOF_IMAGE_NOTE_ORANGE            56
#define EOF_IMAGE_NOTE_ORANGE_HIT        57
#define EOF_IMAGE_NOTE_HORANGE           58
#define EOF_IMAGE_NOTE_HORANGE_HIT       59
#define EOF_IMAGE_NOTE_GREEN_ARROW       60
#define EOF_IMAGE_NOTE_RED_ARROW         61
#define EOF_IMAGE_NOTE_YELLOW_ARROW      62
#define EOF_IMAGE_NOTE_BLUE_ARROW        63
#define EOF_IMAGE_NOTE_GREEN_ARROW_HIT   64
#define EOF_IMAGE_NOTE_RED_ARROW_HIT     65
#define EOF_IMAGE_NOTE_YELLOW_ARROW_HIT  66
#define EOF_IMAGE_NOTE_BLUE_ARROW_HIT    67
#define EOF_IMAGE_NOTE_GREEN_CYMBAL      68
#define EOF_IMAGE_NOTE_GREEN_CYMBAL_HIT  69
#define EOF_IMAGE_NOTE_ORANGE_CYMBAL     70
#define EOF_IMAGE_NOTE_ORANGE_CYMBAL_HIT 71

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
	int string_space, string_space_unscaled;	//string_space_unscaled is set by the display size, string_space is set by eof_scale_fretboard()
	int note_y[EOF_MAX_FRETS];
	int lyric_y;
	int vocal_y;
	int vocal_tail_size;
	int vocal_view_size;
	int lyric_view_key_width;
	int lyric_view_key_height;
	int lyric_view_bkey_width;
	int lyric_view_bkey_height;
	int note_size;
	int hopo_note_size;			//The size of a note that has HOPO forced on
	int anti_hopo_note_size;	//The size of a note that has HOPO forced off
	int note_dot_size;
	int hopo_note_dot_size;
	int anti_hopo_note_dot_size;
	int note_marker_size;
	int note_tail_size;
	int note_kick_size;
	int fretboard_h;
	int buffered_preview;
	int controls_x;

	/* editor mouse click areas */


} EOF_SCREEN_LAYOUT;

#define EOF_EDITOR_RENDER_OFFSET  56

typedef struct
{
	int color;					//RGB color for the normal note
	int hit;					//RGB color for when the note is highlighted (3D tail rendering)
	int border;					//Contrasting RGB color for when the note is highlighted (2D rendering)
	int lightcolor;				//RGB color for a lighter version of the note (for rendering trill/tremolo markers)
	unsigned int note3d;		//The index into eof_image[] storing the 3D note image for this color
	unsigned int notehit3d;		//The index into eof_image[] storing the hit 3D note image for this color
	unsigned int hoponote3d;	//The index into eof_image[] storing the hopo 3D note image for this color
	unsigned int hoponotehit3d;	//The index into eof_image[] storing the hopo hit 3D note image for this color
	unsigned int cymbal3d;		//The index into eof_image[] storing the 3D cymbal image for this color
	unsigned int cymbalhit3d;	//The index into eof_image[] storing the hit 3D cymbal image for this color
	unsigned int arrow3d;		//The index into eof_image[] storing the 3D arrow image for this color (dance track)
	unsigned int arrowhit3d;	//The index into eof_image[] storing the hit 3D arrow image for this color (dance track)
	char *colorname;			//The display name of this color, ie "red"
}eof_color;

extern int eof_color_set;
extern char eof_note_toggle_menu_string_1[20];
extern char eof_note_toggle_menu_string_2[20];
extern char eof_note_toggle_menu_string_3[20];
extern char eof_note_toggle_menu_string_4[20];
extern char eof_note_toggle_menu_string_5[20];
extern char eof_note_toggle_menu_string_6[20];
extern eof_color eof_colors[6];

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
extern NCDFS_FILTER_LIST * eof_filter_gh_files;

extern int         eof_global_volume;

extern EOF_WINDOW * eof_window_editor;
extern EOF_WINDOW * eof_window_note;
extern EOF_WINDOW * eof_window_3d;

extern char        eof_last_frettist[256];
extern int         eof_zoom;
extern int         eof_zoom_3d;
extern char        eof_changes;
extern ALOGG_OGG * eof_music_track;
extern void      * eof_music_data;
extern int         eof_music_data_size;
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
extern EOF_NOTE    eof_pen_note;
extern EOF_LYRIC   eof_pen_lyric;
extern char        eof_filename[1024];
extern char        eof_song_path[1024];
extern char        eof_songs_path[1024];
extern char        eof_window_title[4096];
extern char        eof_last_ogg_path[1024];
extern char        eof_last_eof_path[1024];
extern char        eof_loaded_song_name[1024];
extern char        eof_loaded_ogg_name[1024];
extern int         eof_quit;
extern EOF_SCREEN_LAYOUT eof_screen_layout;
extern BITMAP *    eof_screen;
extern unsigned long eof_screen_width, eof_screen_height;
extern int         eof_vanish_x, eof_vanish_y;
extern char        eof_full_screen_3d;
extern int         eof_av_delay;
extern int         eof_buffer_size;
extern int         eof_audio_fine_tune;
extern int         eof_inverted_notes;
extern int         eof_lefty_mode;
extern int         eof_note_auto_adjust;
extern int         eof_use_ts;	//Determines whether Time Signature events will be handled for MIDI/Feedback import/export
extern int         eof_hide_drum_tails;
extern int         eof_hide_note_names;
extern int         eof_disable_sound_processing;
extern int         eof_disable_3d_rendering;
extern int         eof_disable_2d_rendering;
extern int         eof_disable_info_panel;
extern int         eof_paste_erase_overlap;
extern int         eof_write_rbn_midis;
extern int         eof_add_new_notes_to_selection;
extern int         eof_drum_modifiers_affect_all_difficulties;
extern int         eof_fb_seek_controls;
extern int         eof_min_note_length;
extern int         eof_render_bass_drum_in_lane;
extern int         eof_inverted_chords_slash;
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
extern char        eof_ps_executable_path[1024];
extern char        eof_ps_executable_name[1024];
extern char        eof_ps_songs_path[1024];
extern char        eof_temp_filename[1024];
extern char        eof_soft_cursor;
extern char        eof_desktop;
extern int         eof_input_mode;
extern int         eof_undo_toggle;
extern int         eof_redo_toggle;
extern int         eof_change_count;
extern long        eof_anchor_diff[EOF_TRACKS_MAX];
extern int         eof_last_note;
extern int         eof_last_midi_offset;
extern int         eof_notes_moved;
extern EOF_NOTE *  eof_entering_note_note;
extern EOF_LYRIC *  eof_entering_note_lyric;
extern int         eof_entering_note;
extern int         eof_snote;
extern PACKFILE *  eof_recovery;
extern unsigned long eof_seek_selection_start, eof_seek_selection_end;
extern int         eof_shift_released;
extern int         eof_shift_used;

/* mouse control data */
extern int         eof_selected_control;
extern int         eof_cselected_control;
extern unsigned long eof_selected_catalog_entry;
extern unsigned long eof_selected_beat;
extern int         eof_selected_measure;
extern int         eof_beat_in_measure;
extern int         eof_beats_in_measure;
extern int         eof_pegged_note;
extern int         eof_hover_note;		//The hover note that is tracked using the mouse position
extern int         eof_seek_hover_note;	//The hover note that is tracked using the seek position (for Feedback input method)
extern int         eof_hover_note_2;	//The hover note that is tracked for fret catalog playback
extern long        eof_hover_beat;
extern long        eof_hover_beat_2;
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
extern int         eof_last_pen_pos;
extern int         eof_cursor_visible;
extern int         eof_pen_visible;
extern int         eof_hover_type;
extern int         eof_mouse_drug;

extern char          eof_snap_mode;
extern char          eof_last_snap_mode;
extern int           eof_snap_interval;
extern char			eof_custom_snap_measure;

extern char          eof_hopo_view;

extern EOF_CONTROLLER eof_guitar;
extern EOF_CONTROLLER eof_drums;
extern int eof_color_black;
extern int eof_color_white;
extern int eof_color_gray;
extern int eof_color_red;
extern int eof_color_green;
extern int eof_color_blue;
extern int eof_color_dark_blue;
extern int eof_color_yellow;
extern int eof_color_purple;
extern int eof_color_dark_purple;
extern int eof_color_orange;
extern int eof_color_silver;
extern int eof_color_dark_silver;
extern int eof_info_color;
extern int eof_color_waveform_trough;
extern int eof_color_waveform_peak;
extern int eof_color_waveform_rms;

extern char eof_string_lane_1[4];
extern char eof_string_lane_2[4];
extern char eof_string_lane_3[4];
extern char eof_string_lane_4[4];
extern char eof_string_lane_5[4];
extern char eof_string_lane_6[4];
extern char *eof_fret_strings[6];
extern char eof_string_lane_1_number[];
extern char eof_string_lane_2_number[];
extern char eof_string_lane_3_number[];
extern char eof_string_lane_4_number[];
extern char eof_string_lane_5_number[];
extern char eof_string_lane_6_number[];
extern char *eof_fret_string_numbers[6];

extern PALETTE     eof_palette;
extern BITMAP *    eof_image[EOF_MAX_IMAGES];
extern FONT *      eof_font;
extern FONT *      eof_mono_font;
extern char        eof_note_type_name[5][32];
extern char        eof_vocal_tab_name[5][32];
extern char        eof_dance_tab_name[5][32];

extern char        eof_supports_mp3;
extern char        eof_supports_oggcat;
extern char        eof_just_played;
extern char        eof_mark_drums_as_cymbal;
extern char        eof_mark_drums_as_double_bass;
extern unsigned long eof_mark_drums_as_hi_hat;
extern unsigned long eof_pro_guitar_fret_bitmask;
extern char        eof_legacy_view;

extern char eof_midi_initialized;			//Specifies whether Allegro was able to set up a MIDI device
extern EOF_SELECTION_DATA eof_selection;

extern FILE *eof_log_fp;
extern char eof_log_level;
extern char enable_logging;

extern long xchart[EOF_MAX_FRETS];	//Stores coordinate values used for 3D rendering, updated by eof_set_3D_lane_positions()
extern long ychart[EOF_MAX_FRETS];	//Stores coordinate values used for 3D rendering, updated by eof_set_2D_lane_positions()

void eof_show_mouse(BITMAP * bp);	//Shows the software mouse if it is being used
float eof_get_porpos(unsigned long pos);	//Returns the timestamp position within a beat (percentage)?
long eof_put_porpos(unsigned long beat, float porpos, float offset);	//Returns the timestamp of the specified position within a beat, or -1 on error
void eof_reset_lyric_preview_lines(void);	//Resets the preview line variables to 0
void eof_find_lyric_preview_lines(void);	//Sets the first and second preview line variables
void eof_emergency_stop_music(void);	//Stops audio playback
void eof_fix_catalog_selection(void);	//Ensures that a valid catalog entry is active, if any
unsigned long eof_count_selected_notes(unsigned long * total, char v);
	//Returns the number of notes selected in the active instrument difficulty
	//If total is not NULL, its value is incremented once for each note in the active difficulty
void eof_fix_waveform_graph(void);	//Rebuild the waveform graph data if it exists
void eof_clear_input(void);
void eof_prepare_undo(int type);
int eof_figure_difficulty(void);
int eof_figure_part(void);	//Returns the active track number in terms of FoF's command line play functionality, or -1 on error
int d_hackish_edit_proc (int msg, DIALOG *d, int c);
int eof_set_display_mode(int mode);
void eof_debug_message(char * text);
void eof_determine_phrase_status(unsigned long track);
	//Re-applies the HOPO, SP, trill and tremolo status of each note in the specified track, as well as deleting empty SP, Solo, trill, tremolo and arpeggio phrases
void eof_fix_window_title(void);
int eof_load_ogg_quick(char * filename);
int eof_load_ogg(char * filename);	//Loads the specified OGG file.  Upon success, eof_loaded_ogg_name is updated and nonzero is returned.
int eof_load_complete_song(char * filename);
int eof_destroy_ogg(void);	//Frees chart audio
int eof_save_ogg(char * fn);
void eof_render(void);
void eof_render_lyric_window(void);
void eof_render_3d_window(void);
void eof_render_note_window(void);
int eof_load_data(void);	//Loads graphics and fonts from eof.dat
void eof_destroy_data(void);	//Frees graphics and fonts from memory
char * eof_get_tone_name(int tone);	//Returns the name of the given note number (ie. C# or Db) based on the value of eof_display_flats
void eof_process_midi_queue(int pos);	//Process the MIDI queue based on the given timestamp
int eof_midi_queue_add(unsigned char note,int startpos,int endpos);	//Appends the Note On/Off data to the MIDI queue
void eof_midi_queue_destroy(void);	//Destroys the MIDI queue
void eof_all_midi_notes_off(void);	//Sends a channel mode message to turn off all active notes, as per MIDI specification
void eof_stop_midi(void);	//To be called whenever playback stops, turning of all playing MIDI notes and destroying the MIDI queue
void eof_exit(void);		//Formally exits the program, releasing all data structures allocated

void eof_init_after_load(char initaftersavestate);
	//Initializes variables and cleans up notes, should be used after loading or creating a chart
	//If initaftersavestate is zero (as should be used when loading/importing a chart/MIDI/etc), more variables are forcibly initialized
void eof_cleanup_beat_flags(EOF_SONG *sp);	//Repairs the text event and anchor statuses for beats in the specified chart

void eof_scale_fretboard(unsigned long numlanes);
	//Prepares screen layout offsets, etc. based on EOF's window display size and a given number of lanes (depending on the passed numlanes parameter)
	//If numlanes is 0, the number of lanes of the active track is used, otherwise the parameter itself is used
	//If the number of lanes is less than 5, the original 5 lane spacing will be used
void eof_set_3D_lane_positions(unsigned long track);
	//Sets the 3D coordinate values in the global xchart[] array based on the needs of the specified track
	//If the array doesn't need to be changed, the function returns without recalculating the array's values
	//If a track value of 0 is passed, xchart[] is forcibly updated, such as to be used after changing window size, lefty mode, etc.
void eof_set_2D_lane_positions(unsigned long track);
	//Sets the 2D coordinate values in the global ychart[] array based on the needs of the specified track
	//If the array doesn't need to be changed, the function returns without recalculating the array's values
	//If a track value of 0 is passed, ychart[] is forcibly updated, such as to be used after changing window size, inverted note mode, etc.

void eof_start_logging(void);	//Opens the log file for writing if it isn't already open
void eof_stop_logging(void);	//Closes the log file if it is open
void eof_log(const char *text, char level);
	//If the log is open, writes the string to the log file, followed by a newline character, and flushes the I/O stream
	//Level indicates the minimum level of logging that must be in effect to log the message (ie. 1 = on, 2 = verbose)
	//Verbose logging should be disabled during chart creation/deletion due to the large amount of note creations/deletions
	//The logging verbosity can be altered by toggling bit 1, as bit 0 must be also set in order to log
extern char eof_log_string[1024];	//A string reserved for use with eof_log()
extern unsigned int eof_log_id;
	//This will be set to a random value when logging is started, so if multiple instances of EOF are writing to the same
	//log file (Windows does not prevent this), each instance's log entries can be identified separately from each other.

void eof_init_colors(void);		//Initializes the color structures, to be called after eof_load_data()
void eof_set_color_set(void);	//Updates the eof_colors[] array with color data based on the current track and the user color set preference
void eof_switch_out_callback(void);	//Performs some logic that needs to occur when EOF loses foreground focus
void eof_switch_in_callback(void);	//Performs some logic that needs to occur when EOF gains foreground focus
long eof_get_previous_note(long cnote);	//Returns the note that exists immediately before the specified note in the active track difficulty, or -1 if no such note exists
int eof_note_is_hopo(unsigned long cnote);	//Returns nonzero if the specified note in the active track difficulty should be rendered as a HOPO, based on the current HOPO preview setting
int eof_notes_selected(void);	//Debugging:  Returns the number of notes that are selected by scanning the eof_selection.multi[] array (is deprecated by eof_count_selected_notes())
void eof_read_global_keys(void);	//Reads and acts on various keyboard combinations
void eof_lyric_logic(void);	//Performs various vocal editor logic
void eof_note_logic(void);	//Performs various note catalog logic
void eof_logic(void);		//Performs various editor logic
void eof_render_lyric_preview(BITMAP * bp);	//Renders lyric preview lines (found by eof_find_lyric_preview_lines()) to the specified bitmap
int eof_initialize(int argc, char * argv[]);	//Initializes various values, checks for crash recovery, parses command line parameters, etc.
#ifdef ALLEGRO_WINDOWS
	int eof_initialize_windows(void);	//Performs additional initialization for the Windows platform
#endif

#endif
