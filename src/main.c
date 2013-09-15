/* HOPO note: if there is less than 170 mseconds between notes, they are created as HOPO */

#include <allegro.h>
#ifdef ALLEGRO_WINDOWS
	#include <winalleg.h>
#endif
#include <math.h>
#include <sys/stat.h>
#include <time.h>
#include "alogg/include/alogg.h"
#include "agup/agup.h"
#include "modules/wfsel.h"
#include "modules/ocd3d.h"
#include "modules/gametime.h"
#include "modules/g-idle.h"
#include "dialog/main.h"
#include "menu/beat.h"
#include "menu/main.h"
#include "menu/file.h"
#include "menu/edit.h"
#include "menu/song.h"
#include "menu/track.h"
#include "menu/help.h"
#include "menu/note.h"	//For pitch macros
#include "foflc/Lyric_storage.h"
#include "main.h"
#include "utility.h"
#include "player.h"
#include "config.h"
#include "midi.h"
#include "midi_import.h"
#include "chart_import.h"
#include "gh_import.h"
#include "window.h"
#include "note.h"
#include "beat.h"
#include "event.h"
#include "editor.h"
#include "dialog.h"
#include "undo.h"
#include "mix.h"
#include "control.h"
#include "waveform.h"
#include "spectrogram.h"
#include "silence.h"
#include "tuning.h"
#include "ini_import.h"
#include "midi_data_import.h"	//For eof_track_overridden_by_stored_MIDI_track()
#include "rs.h"	//for eof_pro_guitar_track_find_effective_fret_hand_position()

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

char        eof_note_type_name[5][32] = {" Supaeasy", " Easy", " Medium", " Amazing", " BRE"};
char        eof_vocal_tab_name[5][32] = {" Lyrics", " ", " ", " ", " "};
char        eof_dance_tab_name[5][32] = {" Beginner", " Easy", " Medium", " Hard", " Challenge"};
char        eof_track_diff_populated_status[256] = {0};
char      * eof_snap_name[9] = {"Off", "1/4", "1/8", "1/12", "1/16", "1/24", "1/32", "1/48", "Custom"};
char      * eof_input_name[EOF_INPUT_NAME_NUM + 1] = {"Classic", "Piano Roll", "Hold", "RexMundi", "Guitar Tap", "Guitar Strum", "Feedback"};

NCDFS_FILTER_LIST * eof_filter_music_files = NULL;
NCDFS_FILTER_LIST * eof_filter_ogg_files = NULL;
NCDFS_FILTER_LIST * eof_filter_midi_files = NULL;
NCDFS_FILTER_LIST * eof_filter_eof_files = NULL;
NCDFS_FILTER_LIST * eof_filter_exe_files = NULL;
NCDFS_FILTER_LIST * eof_filter_lyrics_files = NULL;
NCDFS_FILTER_LIST * eof_filter_dB_files = NULL;
NCDFS_FILTER_LIST * eof_filter_gh_files = NULL;
NCDFS_FILTER_LIST * eof_filter_gp_files = NULL;
NCDFS_FILTER_LIST * eof_filter_text_files = NULL;
NCDFS_FILTER_LIST * eof_filter_rs_files = NULL;

PALETTE     eof_palette;
BITMAP *    eof_image[EOF_MAX_IMAGES] = {NULL};
FONT *      eof_font;
FONT *      eof_mono_font;
FONT *      eof_symbol_font;
int         eof_global_volume = 255;

EOF_WINDOW * eof_window_editor = NULL;
EOF_WINDOW * eof_window_editor2 = NULL;
EOF_WINDOW * eof_window_note = NULL;
EOF_WINDOW * eof_window_note_lower_left = NULL;
EOF_WINDOW * eof_window_note_upper_left = NULL;
EOF_WINDOW * eof_window_3d = NULL;

/* configuration */
char        eof_fof_executable_path[1024] = {0};
char        eof_fof_executable_name[1024] = {0};
char        eof_fof_songs_path[1024] = {0};
char        eof_ps_executable_path[1024] = {0};
char        eof_ps_executable_name[1024] = {0};
char        eof_ps_songs_path[1024] = {0};
char        eof_last_frettist[256] = {0};
char        eof_temp_filename[1024] = {0};
char        eof_soft_cursor = 0;
char        eof_desktop = 1;
int         eof_av_delay = 200;
int         eof_midi_tone_delay = 0;
int         eof_midi_synth_instrument_guitar = 28;	//Electric Guitar (clean) MIDI instrument
int         eof_midi_synth_instrument_bass = 34;	//Electric bass (fingered) MIDI instrument
int         eof_buffer_size = 4096;
int         eof_audio_fine_tune = 0;
int         eof_inverted_notes = 0;
int         eof_lefty_mode = 0;
int         eof_note_auto_adjust = 1;
int         eof_use_ts = 0;	//By default, do not import/export TS events in MIDI/Feedback files
int			eof_hide_drum_tails = 0;
int         eof_hide_note_names = 0;
int         eof_disable_sound_processing = 0;
int         eof_disable_3d_rendering = 0;
int         eof_disable_2d_rendering = 0;
int         eof_disable_info_panel = 0;
int         eof_paste_erase_overlap = 0;
int         eof_write_rb_files = 0;				//If nonzero, extra files are written during save that are used for authoring Rock Band customs
int         eof_write_rs_files = 0;				//If nonzero, extra files are written during save that are used for authoring Rocksmith customs
int         eof_add_new_notes_to_selection = 0;	//If nonzero, newly added gems cause notes to be added to the selection instead of the selection being cleared first
int         eof_drum_modifiers_affect_all_difficulties = 1;	//If nonzero, a drum modifier (ie. open/pedal hi hat or rim shot apply to any notes at the same position in non active difficulties)
int         eof_fb_seek_controls = 0;			//If nonzero, the page up/dn keys have their seek directions reversed, and up/down seek forward/backward
int         eof_new_note_length_1ms;			//If nonzero, newly created notes are initialized to a length of 1ms instead of the regular grid snap based logic
int         eof_gp_import_preference_1 = 0;		//If nonzero during GP import, beat texts with qualifying strings are imported as RS sections, section markers as RS phrases.
int         eof_gp_import_truncate_short_notes = 1;	//If nonzero during GP import, notes shorter than one quarter note are set to the minimum length of 1ms
int         eof_gp_import_replaces_track = 1;	//If nonzero during GP import, the imported track replaces the active track instead of just the active track difficulty
int         eof_gp_import_nat_harmonics_only = 0;	//If nonzero during GP import, only natural harmonics are imported as notes with harmonic status
int         eof_min_note_length = 0;			//Specifies the user-configured minimum length for all non-drum notes (for making Guitar Hero customs, is set to 0 if undefined)
int         eof_min_note_distance = 3;			//Specifies the user-configured minimum distance between notes (to avoid problems with timing conversion leading to precision loss that can cause notes to combine/drop)
int         eof_render_bass_drum_in_lane = 0;	//If nonzero, the 3D rendering will draw bass drum gems in a lane instead of as a bar spanning all lanes
int         eof_inverted_chords_slash = 0;
int         eof_render_3d_rs_chords = 0;	//If nonzero, the 3D rendering will draw a rectangle to represent chords that will export to XML as repeats (Rocksmith), and 3D chord tails will not be rendered
int         eof_imports_recall_last_path = 0;	//If nonzero, various import dialogs will initialize the dialog to the path containing the last chosen import, instead of initializing to the project's folder
int         eof_rewind_at_end = 1;				//If nonzero, chart rewinds when the end of chart is reached during playback
int         eof_smooth_pos = 1;
int         eof_input_mode = EOF_INPUT_PIANO_ROLL;
int         eof_windowed = 1;
int         eof_anchor_all_beats = 1;
int         eof_disable_windows = 0;
int         eof_disable_vsync = 0;
int         eof_playback_speed = 1000;
char        eof_ogg_setting = 1;
char      * eof_ogg_quality[6] = {"1.0", "2.0", "4.0", "5.0", "6.0", "8.0"};
unsigned long eof_frame = 0;
int         eof_debug_mode = 0;
char        eof_cpu_saver = 0;
char        eof_has_focus = 1;
char        eof_supports_mp3 = 0;
char        eof_supports_oggcat = 0;		//Set to nonzero if EOF determines oggCat is usable
int         eof_new_idle_system = 0;
char        eof_just_played = 0;
char        eof_mark_drums_as_cymbal = 0;		//Allows the user to specify whether Y/B/G drum notes will be placed with cymbal notation by default
char        eof_mark_drums_as_double_bass = 0;	//Allows the user to specify whether expert bass drum notes will be placed with expert+ notation by default
unsigned long eof_mark_drums_as_hi_hat = 0;		//Allows the user to specify whether Y drum notes in the PS drum track will be placed with one of the hi hat statuses by default (this variable holds the note flag of the desired status)
unsigned long eof_pro_guitar_fret_bitmask = 63;	//Defines which lanes are affected by these shortcuts:  CTRL+# to set fret numbers, CTRL+(plus/minus) increment/decrement fret values, CTRL+G to toggle ghost status
char		eof_legacy_view = 0;				//Specifies whether pro guitar notes will render as legacy notes
unsigned char eof_2d_render_top_option = 32;	//Specifies what item displays at the top of the 2D panel (defaults to note names)

int         eof_undo_toggle = 0;
int         eof_redo_toggle = 0;
int         eof_window_title_dirty = 0;	//This is set to true when an undo state is made and is cleared when the window title is recreated with eof_fix_window_title()
int         eof_change_count = 0;
int         eof_zoom = 10;			//The width of one pixel in the editor window in ms (smaller = higher zoom)
int         eof_zoom_3d = 5;
char        eof_changes = 0;
ALOGG_OGG * eof_music_track = NULL;
void      * eof_music_data = NULL;
int         eof_silence_loaded = 0;
int         eof_music_data_size = 0;
unsigned long eof_chart_length = 0;
unsigned long eof_music_length = 0;
int         eof_music_pos;
int         eof_music_pos2 = -1;		//The position to display in the secondary piano roll (-1 means it will initialize to the current track when it is enabled)
int         eof_sync_piano_rolls = 1;	//If nonzero, the secondary piano roll will render with the current chart position instead of its own
int         eof_music_actual_pos;
int         eof_music_rewind_pos;
int         eof_music_catalog_pos;
int         eof_music_end_pos;
int         eof_music_paused = 1;
char        eof_music_catalog_playback = 0;
char        eof_play_selection = 0;
int         eof_selected_ogg = 0;
EOF_SONG    * eof_song = NULL;
EOF_NOTE    eof_pen_note;
EOF_LYRIC   eof_pen_lyric;
char        eof_filename[1024] = {0};			//The full path of the EOF file that is loaded
char        eof_song_path[1024] = {0};			//The path to active project's parent folder
char        eof_songs_path[1024] = {0};			//The path to the user's song folder
char        eof_last_ogg_path[1024] = {0};		//The path to the folder containing the last loaded OGG file
char        eof_last_eof_path[1024] = {0};		//The path to the folder containing the last loaded project
char        eof_last_midi_path[1024] = {0};		//The path to the folder containing the last imported MIDI
char        eof_last_db_path[1024] = {0};		//The path to the folder containing the last imported Feedback chart file
char        eof_last_gh_path[1024] = {0};		//The path to the folder containing the last imported Guitar Hero chart file
char        eof_last_lyric_path[1024] = {0};	//The path to the folder containing the last imported lyric file
char        eof_last_gp_path[1024] = {0};		//The path to the folder containing the last imported Guitar Pro file
char        eof_last_rs_path[1024] = {0};		//The path to the folder containing the last imported Rocksmith XML file
char        eof_loaded_song_name[1024] = {0};	//The file name (minus the folder path) of the active project
char        eof_loaded_ogg_name[1024] = {0};	//The full path of the loaded OGG file
char        eof_window_title[4096] = {0};
int         eof_quit = 0;
int         eof_note_type = EOF_NOTE_AMAZING;	//The active difficulty
int         eof_selected_track = EOF_TRACK_GUITAR;
int         eof_display_second_piano_roll = 0;	//If nonzero, a second piano roll will render in place of the info panel and 3D preview
int         eof_note_type2 = -1;	//The difficulty to display in the secondary piano roll (-1 means it will initialize to the current difficulty when it is enabled)
int         eof_selected_track2 = -1;	//The track to display in the secondary piano roll (-1 means it will initialize to the current track when it is enabled)
int         eof_vocals_selected = 0;	//Is set to nonzero if the active track is a vocal track
int         eof_vocals_tab = 0;
int         eof_vocals_offset = 60; // Start at "middle C"
int         eof_song_loaded = 0;	//The boolean condition that a chart and its audio are successfully loaded
int         eof_last_note = 0;
int         eof_last_midi_offset = 0;
PACKFILE *  eof_recovery = NULL;
unsigned long eof_seek_selection_start = 0, eof_seek_selection_end = 0;	//Used to track the keyboard driven note selection system in Feedback input mode
int         eof_shift_released = 1;	//Tracks the press/release of the SHIFT keys for the Feedback input mode seek selection system
int         eof_shift_used = 0;	//Tracks whether the SHIFT key was used for a keyboard shortcut while SHIFT was held

/* mouse control data */
int         eof_selected_control = -1;
int         eof_cselected_control = -1;
unsigned long eof_selected_catalog_entry = 0;
unsigned long eof_selected_beat = 0;
int         eof_selected_measure = 0;
int         eof_beat_in_measure = 0;
int         eof_beats_in_measure = 1;
int         eof_pegged_note = -1;
int         eof_hover_note = -1;
int         eof_seek_hover_note = -1;
int         eof_hover_note_2 = -1;
long        eof_hover_beat = -1;
long        eof_hover_beat_2 = -1;
int         eof_hover_piece = -1;
int         eof_hover_key = -1;
int         eof_hover_lyric = -1;
int         eof_last_tone = -1;
int         eof_mouse_z;
int         eof_mickey_z;
int         eof_mickeys_x;
int         eof_mickeys_y;
int         eof_lclick_released = 1;	//Tracks the handling of the left mouse button in general
int         eof_blclick_released = 1;	//Tracks the handling of the left mouse button when used on beat markers
int         eof_rclick_released = 1;	//Tracks the handling of the right mouse button in general
int         eof_click_x;
int         eof_click_y;
int         eof_peg_x;
int         eof_last_pen_pos = 0;
int         eof_cursor_visible = 1;
int         eof_pen_visible = 1;
int         eof_hover_type = -1;
int         eof_mouse_drug = 0;
int         eof_notes_moved = 0;

/* grid snap data */
char          eof_snap_mode = EOF_SNAP_OFF;
char          eof_last_snap_mode = EOF_SNAP_OFF;
int           eof_snap_interval = 1;
char          eof_custom_snap_measure = 0;	//Boolean: The user set a custom grid snap interval defined in measures instead of beats

char          eof_hopo_view = EOF_HOPO_RF;

/* controller info */
EOF_CONTROLLER eof_guitar;
EOF_CONTROLLER eof_drums;

/* colors */
int eof_color_black;
int eof_color_white;
int eof_color_gray;
int eof_color_gray2;
int eof_color_gray3;
int eof_color_light_gray;
int eof_color_red;
int eof_color_green;
int eof_color_dark_green;
int eof_color_blue;
int eof_color_dark_blue;
int eof_color_light_blue;
int eof_color_turquoise;
int eof_color_yellow;
int eof_color_purple;
int eof_color_dark_purple;
int eof_color_orange;
int eof_color_silver;
int eof_color_dark_silver;
int eof_color_cyan;
int eof_color_dark_cyan;
int eof_info_color;
int eof_color_waveform_trough;
int eof_color_waveform_peak;
int eof_color_waveform_rms;

int eof_color_set = EOF_COLORS_DEFAULT;
eof_color eof_colors[6];	//Contain the color definitions for each lane
eof_color eof_color_green_struct, eof_color_red_struct, eof_color_yellow_struct, eof_color_blue_struct, eof_color_orange_struct, eof_color_purple_struct;
	//Color data

EOF_SCREEN_LAYOUT eof_screen_layout;
BITMAP * eof_screen = NULL;
unsigned long eof_screen_width, eof_screen_height;	//Used to track the EOF window size, for when the 3D projection is altered
unsigned long eof_screen_width_default;				//Stores the default width based on the current window height
int eof_vanish_x = 0, eof_vanish_y = 0;				//Used to allow the user to control the vanishing point for the 3D preview
char eof_full_screen_3d = 0;						//If nonzero, directs the render logic to scale the 3D window to fit the entire program window
char eof_3d_fretboard_coordinates_cached = 0;		//Tracks the validity of the 3D window rendering's cache of the fretboard's 2D coordinates
char eof_screen_zoom = 0;							//Tracks whether EOF will perform a x2 zoom rendering

EOF_SELECTION_DATA eof_selection;

/* lyric preview data */
int eof_preview_line[2] = {0};
int eof_preview_line_lyric[2] = {0};
int eof_preview_line_end_lyric[2] = {0};

/* Windows-only data */
#ifdef ALLEGRO_WINDOWS
int eof_windows_argc = 0;
wchar_t ** eof_windows_internal_argv;
char ** eof_windows_argv;
#endif

/* fret/tuning editing arrays */
char eof_string_lane_1[4] = {0};	//Use a fourth byte to guarantee proper truncation
char eof_string_lane_2[4] = {0};
char eof_string_lane_3[4] = {0};
char eof_string_lane_4[4] = {0};
char eof_string_lane_5[4] = {0};
char eof_string_lane_6[4] = {0};
char *eof_fret_strings[6] = {eof_string_lane_1, eof_string_lane_2, eof_string_lane_3, eof_string_lane_4, eof_string_lane_5, eof_string_lane_6};
char eof_string_lane_1_number[] = "String 6:";
char eof_string_lane_2_number[] = "String 5:";
char eof_string_lane_3_number[] = "String 4:";
char eof_string_lane_4_number[] = "String 3:";
char eof_string_lane_5_number[] = "String 2:";
char eof_string_lane_6_number[] = "String 1:";
char *eof_fret_string_numbers[6] = {eof_string_lane_1_number, eof_string_lane_2_number, eof_string_lane_3_number, eof_string_lane_4_number, eof_string_lane_5_number ,eof_string_lane_6_number};

struct MIDIentry *MIDIqueue=NULL;		//Linked list of queued MIDI notes
struct MIDIentry *MIDIqueuetail=NULL;	//Points to the tail of the list
char eof_midi_initialized = 0;			//Specifies whether Allegro was able to set up a MIDI device

FILE *eof_log_fp = NULL;	//Is set to NULL if logging is disabled
char eof_log_level = 0;		//Is set to 0 if logging is disabled
char enable_logging = 1;	//Is set to 0 if logging is disabled

int eof_custom_zoom_level = 0;	//Tracks any user-defined custom zoom level
char eof_display_flats = 0;		//Used to allow eof_get_tone_name() to return note names containing flats.  By default, display as sharps instead

void eof_debug_message(char * text)
{
	eof_log("eof_debug_message() entered", 1);

	if(text && eof_debug_mode)
	{
		allegro_message("%s", text);
		gametime_reset();
	}
}

void eof_show_mouse(BITMAP * bp)
{
	eof_log("eof_show_mouse() entered", 2);

	if(bp && eof_soft_cursor)
	{
		show_mouse(bp);
	}
}

float eof_get_porpos(unsigned long pos)
{
	float porpos = 0.0;
	long beat;
	int blength;
	unsigned long rpos;

	eof_log("eof_get_porpos() entered", 2);

	beat = eof_get_beat(eof_song, pos);
	if(beat < eof_song->beats - 1)
	{
		blength = eof_song->beat[beat + 1]->pos - eof_song->beat[beat]->pos;
	}
	else
	{
		blength = eof_song->beat[eof_song->beats - 1]->pos - eof_song->beat[eof_song->beats - 2]->pos;
	}
	if(beat < 0)
	{	//If eof_get_beat() returned error
		beat = eof_song->beats - 1;	//Assume the note position is relative to the last beat marker
	}
	rpos = pos - eof_song->beat[beat]->pos;
	porpos = ((float)rpos / (float)blength) * 100.0;
	return porpos;
}

long eof_put_porpos(unsigned long beat, float porpos, float offset)
{
	float fporpos = porpos + offset;
	unsigned long cbeat = beat;

	eof_log("eof_put_porpos() entered", 2);

	if(fporpos <= -1.0)
	{
//		allegro_message("a - %f", fporpos);	//Debug
		while(fporpos < 0.0)
		{	//If the position within the beat to find is negative, scale up to a positive number
			fporpos += 100.0;
			if(cbeat == 0)
			{	//If this position couldn't be found
				return -1;	//Return error instead of decrementing below 0
			}
			cbeat--;
		}
		return ((eof_song->beat[cbeat]->fpos + ((float)eof_get_beat_length(eof_song, cbeat) * fporpos) / 100.0) + 0.5);	//Round up to nearest millisecond
	}
	else if(fporpos >= 100.0)
	{
//		allegro_message("b - %f", fporpos);	//Debug
		while(fporpos >= 1.0)
		{
			fporpos -= 100.0;
			cbeat++;
		}
		if(cbeat < eof_song->beats)
		{
			return ((eof_song->beat[cbeat]->fpos + ((float)eof_get_beat_length(eof_song, cbeat) * fporpos) / 100.0) + 0.5);	//Round up to nearest millisecond
		}
		return -1;
	}
//	allegro_message("c - %f", fporpos);	//Debug

	if(cbeat < eof_song->beats)
	{	//If cbeat is valid
		return ((eof_song->beat[cbeat]->fpos + ((float)eof_get_beat_length(eof_song, cbeat) * fporpos) / 100.0) + 0.5);	//Round up to nearest millisecond
	}
	return -1;	//Error
}

void eof_reset_lyric_preview_lines(void)
{
	eof_log("eof_reset_lyric_preview_lines() entered", 2);

	eof_preview_line[0] = 0;
	eof_preview_line[1] = 0;
	eof_preview_line_lyric[0] = 0;
	eof_preview_line_lyric[1] = 0;
	eof_preview_line_end_lyric[0] = 0;
	eof_preview_line_end_lyric[1] = 0;
}

void eof_find_lyric_preview_lines(void)
{
//	eof_log("eof_find_lyric_preview_lines() entered");

	unsigned long i, j;
	int current_line = -1;
	int next_line = -1;
	int dist = -1;
	int beyond = 1;
	int adj_eof_music_pos=eof_music_pos - eof_av_delay;	//The current seek position of the chart, adjusted for AV delay

	for(i = 0; i < eof_song->vocal_track[0]->lines; i++)
	{
		if((adj_eof_music_pos >= eof_song->vocal_track[0]->line[i].start_pos) && (adj_eof_music_pos <= eof_song->vocal_track[0]->line[i].end_pos))
		{
			current_line = i;
			break;
		}
	}
	for(i = 0; i < eof_song->vocal_track[0]->lines; i++)
	{
		if(adj_eof_music_pos <= eof_song->vocal_track[0]->line[i].end_pos)
		{
			beyond = 0;
			break;
		}
	}
	if(beyond)
	{
		eof_preview_line[0] = -1;
		eof_preview_line_lyric[0] = 0;
		eof_preview_line_end_lyric[0] = 0;
		return;
	}
	if(current_line >= 0)
	{
		eof_preview_line[0] = current_line;
		for(i = 0; i < eof_song->vocal_track[0]->lyrics; i++)
		{
			if((eof_song->vocal_track[0]->lyric[i]->pos >= eof_song->vocal_track[0]->line[eof_preview_line[0]].start_pos) && (eof_song->vocal_track[0]->lyric[i]->pos <= eof_song->vocal_track[0]->line[eof_preview_line[0]].end_pos))
			{
				eof_preview_line_lyric[0] = i;
				for(j = i; j < eof_song->vocal_track[0]->lyrics; j++)
				{
					if(eof_song->vocal_track[0]->lyric[j]->pos > eof_song->vocal_track[0]->line[eof_preview_line[0]].end_pos)
					{
						break;
					}
				}
				eof_preview_line_end_lyric[0] = j;
				break;
			}
		}
		for(i = 0; i < eof_song->vocal_track[0]->lines; i++)
		{
			if((eof_song->vocal_track[0]->line[current_line].start_pos < eof_song->vocal_track[0]->line[i].start_pos) && (eof_song->vocal_track[0]->line[i].start_pos - eof_song->vocal_track[0]->line[current_line].start_pos < dist))
			{
				next_line = i;
				dist = eof_song->vocal_track[0]->line[i].start_pos - eof_song->vocal_track[0]->line[current_line].start_pos;
			}
		}
		if(next_line >= 0)
		{
			eof_preview_line[1] = next_line;
			for(i = 0; i < eof_song->vocal_track[0]->lyrics; i++)
			{
				if((eof_song->vocal_track[0]->lyric[i]->pos >= eof_song->vocal_track[0]->line[eof_preview_line[1]].start_pos) && (eof_song->vocal_track[0]->lyric[i]->pos <= eof_song->vocal_track[0]->line[eof_preview_line[1]].end_pos))
				{
					eof_preview_line_lyric[1] = i;
					for(j = i; j < eof_song->vocal_track[0]->lyrics; j++)
					{
						if(eof_song->vocal_track[0]->lyric[j]->pos > eof_song->vocal_track[0]->line[eof_preview_line[1]].end_pos)
						{
							break;
						}
					}
					eof_preview_line_end_lyric[1] = j;
					break;
				}
			}
		}
		else
		{
			eof_preview_line[1] = -1;
			eof_preview_line_lyric[1] = 0;
			eof_preview_line_end_lyric[1] = 0;
		}
	}
	else
	{
		eof_preview_line[0] = -1;
	}
}

void eof_emergency_stop_music(void)
{
	eof_log("eof_emergency_stop_music() entered", 2);

	if(eof_song_loaded)
	{
		if(!eof_music_paused)
		{
			eof_log("\tStopping playback", 1);
			eof_music_paused = 1;
			eof_stop_midi();
			alogg_stop_ogg(eof_music_track);
		}
		else if(eof_music_catalog_playback)
		{
			eof_log("\tStopping playback", 1);
			eof_music_catalog_playback = 0;
			eof_music_catalog_pos = eof_song->catalog->entry[eof_selected_catalog_entry].start_pos + eof_av_delay;
			eof_stop_midi();
			alogg_stop_ogg(eof_music_track);
		}
	}
}

void eof_switch_out_callback(void)
{
	eof_log("eof_switch_out_callback() entered", 2);

	eof_emergency_stop_music();
	key[KEY_TAB] = 0;

	#ifndef ALLEGRO_MACOSX
		eof_has_focus = 0;
	#endif
}

void eof_switch_in_callback(void)
{
	eof_log("eof_switch_in_callback() entered", 2);

	key[KEY_TAB] = 0;
	eof_has_focus = 1;
	gametime_reset();
}

void eof_fix_catalog_selection(void)
{
	eof_log("eof_fix_catalog_selection() entered", 1);

	if(eof_song->catalog->entries > 0)
	{
		if(eof_selected_catalog_entry >= eof_song->catalog->entries)
		{
			eof_selected_catalog_entry = eof_song->catalog->entries - 1;
		}
	}
	else
	{
		eof_selected_catalog_entry = 0;
	}
}

int eof_set_display_mode_preset(int mode)
{
	switch(mode)
	{
		case EOF_DISPLAY_640:
			eof_set_display_mode(640, 480);
		break;

		case EOF_DISPLAY_800:
			eof_set_display_mode(800, 600);
		break;

		case EOF_DISPLAY_1024:
			eof_set_display_mode(1024, 768);
		break;

		default:
		return 0;	//Invalid display mode
	}

	return 1;
}

int eof_set_display_mode_preset_custom_width(int mode, unsigned long width)
{
	switch(mode)
	{
		case EOF_DISPLAY_640:
			eof_set_display_mode(width, 480);
		break;

		case EOF_DISPLAY_800:
			eof_set_display_mode(width, 600);
		break;

		case EOF_DISPLAY_1024:
			eof_set_display_mode(width, 768);
		break;

		default:
		return 0;	//Invalid display mode
	}

	return 1;
}

int eof_set_display_mode(unsigned long width, unsigned long height)
{
	int mode;
	unsigned long effectiveheight = height, effectivewidth = width;

	eof_log("eof_set_display_mode() entered", 1);

	/* destroy windows first */
	if(eof_window_editor)
	{
		eof_window_destroy(eof_window_editor);
		eof_window_editor = NULL;
	}
	if(eof_window_editor2)
	{
		eof_window_destroy(eof_window_editor2);
		eof_window_editor2 = NULL;
	}
	if(eof_window_note_lower_left)
	{
		eof_window_destroy(eof_window_note_lower_left);
		eof_window_note_lower_left = NULL;
	}
	if(eof_window_note_upper_left)
	{
		eof_window_destroy(eof_window_note_upper_left);
		eof_window_note_upper_left = NULL;
	}
	if(eof_window_3d)
	{
		eof_window_destroy(eof_window_3d);
		eof_window_3d = NULL;
	}
	if(eof_screen)
	{
		destroy_bitmap(eof_screen);
		eof_screen = NULL;
	}

	if(eof_screen_zoom)
	{	//If x2 zoom is in effect
		effectiveheight = height * 2;
		effectivewidth = width * 2;
	}

	eof_3d_fretboard_coordinates_cached = 0;	//The 3D rendering logic will need to rebuild the fretboard's 2D coordinate projections
	switch(height)
	{
		case 480:
			mode = EOF_DISPLAY_640;
			eof_screen_width_default = 640;
			eof_screen_width = width;
			eof_screen_height = 480;
			eof_screen_layout.string_space_unscaled = 20;
			eof_screen_layout.vocal_y = 96;
			eof_screen_layout.vocal_tail_size = 4;
			eof_screen_layout.lyric_view_bkey_width = 3;
			eof_screen_layout.note_size = 6;
			eof_screen_layout.hopo_note_size = 4;
			eof_screen_layout.anti_hopo_note_size = 8;
			eof_screen_layout.note_dot_size = 2;
			eof_screen_layout.hopo_note_dot_size = 1;
			eof_screen_layout.anti_hopo_note_dot_size = 4;
			eof_screen_layout.note_marker_size = 10;
			eof_screen_layout.note_tail_size = 3;
			eof_screen_layout.note_kick_size = 2;
		break;

		case 600:
			mode = EOF_DISPLAY_800;
			eof_screen_width_default = 800;
			eof_screen_width = width;
			eof_screen_height = 600;
			eof_screen_layout.string_space_unscaled = 30;
			eof_screen_layout.vocal_y = 128;
			eof_screen_layout.vocal_tail_size = 6;
			eof_screen_layout.lyric_view_bkey_width = 4;
			eof_screen_layout.note_size = 8;
			eof_screen_layout.hopo_note_size = 5;
			eof_screen_layout.anti_hopo_note_size = 11;
			eof_screen_layout.note_dot_size = 3;
			eof_screen_layout.hopo_note_dot_size = 2;
			eof_screen_layout.anti_hopo_note_dot_size = 5;
			eof_screen_layout.note_marker_size = 12;
			eof_screen_layout.note_tail_size = 4;
			eof_screen_layout.note_kick_size = 3;
		break;

		case 768:
			mode = EOF_DISPLAY_1024;
			eof_screen_width_default = 1024;
			eof_screen_width = width;
			eof_screen_height = 768;
			eof_screen_layout.string_space_unscaled = 48;
			eof_screen_layout.vocal_y = 197;
			eof_screen_layout.vocal_tail_size = 11;
			eof_screen_layout.lyric_view_bkey_width = 5;
			eof_screen_layout.note_size = 10;
			eof_screen_layout.hopo_note_size = 7;
			eof_screen_layout.anti_hopo_note_size = 14;
			eof_screen_layout.note_dot_size = 4;
			eof_screen_layout.hopo_note_dot_size = 3;
			eof_screen_layout.anti_hopo_note_dot_size = 7;
			eof_screen_layout.note_marker_size = 15;
			eof_screen_layout.note_tail_size = 5;
			eof_screen_layout.note_kick_size = 4;
		break;

		default:
		return 0;	//Invalid display mode
	}
	if(eof_screen_width < eof_screen_width_default)
	{	//If the specified width is invalid
		eof_screen_width = eof_screen_width_default;
	}

	if(set_gfx_mode(GFX_AUTODETECT_WINDOWED, effectivewidth, effectiveheight, 0, 0))
	{	//If the specified window size could not be set, try again
		if(set_gfx_mode(GFX_AUTODETECT, effectivewidth, effectiveheight, 0, 0))
		{	//If it failed again
			if(eof_screen_zoom)
			{	//If x2 zoom display is enabled
				eof_screen_zoom = 0;	//Disable x2 zoom
				eof_set_display_mode(width, height);
				allegro_message("Warning:  Failed to enable x2 zoom.\nTry setting a smaller program window size first.");
				return 1;
			}
			if(width != eof_screen_width_default)
			{	//If the custom width failed to be applied, revert to the default width for this window height
				eof_set_display_mode(eof_screen_width_default, height);
				allegro_message("Warning:  Failed to set custom display width, reverted to default.\nTry setting a different width.");
				return 1;
			}
			if(set_gfx_mode(GFX_SAFE, width, eof_screen_height, 0, 0))
			{	//If the window size could not be set at all
				allegro_message("Can't set up screen!  Error: %s",allegro_error);
				return 0;
			}
			else
			{
				allegro_message("EOF is in safe mode!\nThings may not work as expected.");
			}
		}
	}
	eof_screen = create_bitmap(eof_screen_width, eof_screen_height);
	if(!eof_screen)
	{
		return 0;
	}
	eof_window_editor = eof_window_create(0, 20, eof_screen_width, (eof_screen_height / 2) - 20, eof_screen);
	if(!eof_window_editor)
	{
		allegro_message("Unable to create editor window!");
		return 0;
	}
	eof_window_editor2 = eof_window_create(0, (eof_screen_height / 2) + 1, eof_screen_width, eof_screen_height, eof_screen);
	if(!eof_window_editor2)
	{
		allegro_message("Unable to create second editor window!");
		return 0;
	}
	eof_window_note_lower_left = eof_window_create(0, eof_screen_height / 2, eof_screen_width, eof_screen_height / 2, eof_screen);	//Make the window full width
	eof_window_note_upper_left = eof_window_create(0, 20, eof_screen_width, eof_screen_height / 2, eof_screen);	//Make the window full width
	if(!eof_window_note_lower_left || !eof_window_note_upper_left)
	{
		allegro_message("Unable to create information windows!");
		return 0;
	}
	eof_window_3d = eof_window_create(eof_screen_width - (eof_screen_width_default / 2), eof_screen_height / 2, eof_screen_width_default / 2, eof_screen_height / 2, eof_screen);
	if(!eof_window_3d)
	{
		allegro_message("Unable to create 3D preview window!");
		return 0;
	}
	eof_screen_layout.scrollbar_y = (eof_screen_height / 2) - 37;
	eof_scale_fretboard(5);	//Set the eof_screen_layout.note_y[] array based on a 5 lane track, for setting the fretboard height below
	eof_screen_layout.lyric_y = 20;
	eof_screen_layout.vocal_view_size = 13;
	eof_screen_layout.lyric_view_key_width = eof_window_3d->screen->w / 29;
	eof_screen_layout.lyric_view_key_height = eof_screen_layout.lyric_view_key_width * 4;
	eof_screen_layout.lyric_view_bkey_height = eof_screen_layout.lyric_view_key_width * 3;
	eof_screen_layout.fretboard_h = eof_screen_layout.note_y[4] + 25;
	eof_screen_layout.buffered_preview = 0;
	eof_screen_layout.controls_x = eof_screen_width - 197;
	eof_screen_layout.mode = mode;
	eof_vanish_x = 160;
	eof_set_3d_projection();
	set_display_switch_mode(SWITCH_BACKGROUND);
	set_display_switch_callback(SWITCH_OUT, eof_switch_out_callback);
	set_display_switch_callback(SWITCH_IN, eof_switch_in_callback);
	PrepVSyncIdle();
	return 1;
}

void eof_cat_track_difficulty_string(char *str)
{
	if(eof_song->track[eof_selected_track]->flags & EOF_TRACK_FLAG_ALT_NAME)
	{	//If this track has an alternate name, append the track's alternate name
		(void) ustrcat(str, eof_song->track[eof_selected_track]->altname);
	}
	else
	{	//Otherwise append the track's native name
		(void) ustrcat(str, eof_song->track[eof_selected_track]->name);
	}
	if(!eof_vocals_selected)
	{	//If the vocal track isn't active, append other information such as the current difficulty
		(void) ustrcat(str, "  ");
		char *ptr;
		if(eof_song->track[eof_selected_track]->flags & EOF_TRACK_FLAG_UNLIMITED_DIFFS)
		{	//If this track is not limited to 5 difficulties
			char diff_string[15] = {0};			//Used to generate the string for a numbered difficulty
			(void) snprintf(diff_string, sizeof(diff_string) - 1, " Diff: %d", eof_note_type);
			if(eof_track_diff_populated_status[eof_note_type])
			{	//If this difficulty is populated
				diff_string[0] = '*';
			}
			(void) ustrcat(str, diff_string);
		}
		else if(eof_selected_track == EOF_TRACK_DANCE)
		{	//If the dance track is active
			ptr = eof_dance_tab_name[eof_note_type];
			(void) ustrcat(str, ptr);					//Append the active dance difficulty name
		}
		else
		{
			ptr = eof_note_type_name[eof_note_type];
			(void) ustrcat(str, ptr);					//Append the active instrument difficulty name
		}
	}

	if(eof_song->track[eof_selected_track]->flags & EOF_TRACK_FLAG_ALT_NAME)
	{	//If this track has an alternate name, append the track's native name
		(void) ustrcat(str, " (");
		(void) ustrcat(str, eof_song->track[eof_selected_track]->name);
		(void) ustrcat(str, ")");
	}
}

void eof_fix_window_title(void)
{
	eof_log("eof_fix_window_title() entered", 2);

	if(eof_changes)
	{
		(void) ustrcpy(eof_window_title, "*EOF - ");
	}
	else
	{
		(void) ustrcpy(eof_window_title, "EOF - ");
	}
	if(eof_song && eof_song_loaded)
	{
		eof_cat_track_difficulty_string(eof_window_title);	//Append the track difficulty's title to the window title

		if((eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT) && eof_legacy_view)
		{	//If this is a pro guitar track being displayed with the legacy view enabled
			(void) ustrcat(eof_window_title, "(Legacy view)");
		}

		if(eof_song->tags->tempo_map_locked)
		{	//If the tempo map is locked
			(void) ustrcat(eof_window_title, "(Tempo map locked)");
		}
		if(eof_song->tags->click_drag_disabled)
		{	//If click and drag is disabled
			(void) ustrcat(eof_window_title, "(Click+drag disabled)");
		}
		if(eof_song->tags->double_bass_drum_disabled)
		{	//If expert+ bass drum is disabled
			(void) ustrcat(eof_window_title, "(Expert+ drums off)");
		}
		if(eof_silence_loaded)
		{	//If no chart audio is actually loaded
			(void) ustrcat(eof_window_title, "(No audio loaded)");
		}
	}
	else
	{
		(void) ustrcat(eof_window_title, "No Song");
	}
	set_window_title(eof_window_title);
	eof_window_title_dirty = 0;
}

void eof_clear_input(void)
{
	int i;

	eof_log("eof_clear_input() entered", 2);

	clear_keybuf();
	for(i = 0; i < KEY_MAX; i++)
	{
		if((i != KEY_ALTGR) && (i != KEY_ALT) && (i != KEY_LCONTROL) && (i != KEY_RCONTROL) && (i != KEY_LSHIFT) && (i != KEY_RSHIFT))
		{
			key[i] = 0;
		}
	}
	eof_mouse_z = mouse_z;
}

void eof_prepare_undo(int type)
{
	eof_log("eof_prepare_undo() entered", 1);

	if(eof_undo_add(type))
	{
		eof_change_count++;
		eof_changes = 1;
	}
	eof_redo_toggle = 0;
	eof_undo_toggle = 1;
	eof_fix_window_title();	//Redraw the window title to reflect the chart is modified
	eof_window_title_dirty = 1;	//Indicate that the window title will need to be redrawn during the next normal render, to reflect whatever change is to follow this undo state
	if(eof_change_count % 10 == 0)
	{
		(void) replace_extension(eof_temp_filename, eof_filename, "backup.eof", 1024);
		eof_log("\tSaving periodic backup", 1);
		if(!eof_save_song(eof_song, eof_temp_filename))
		{
			allegro_message("Undo state error!");
		}
//		allegro_message("backup saved\n%s", eof_temp_filename);
	}
}

long eof_get_previous_note(long cnote)
{
	long i;

	eof_log("eof_get_previous_note() entered", 2);

	for(i = cnote - 1; i >= 0; i--)
	{
		if(eof_get_note_type(eof_song, eof_selected_track, i) == eof_get_note_type(eof_song, eof_selected_track, cnote))
		{
			return i;
		}
	}
	return -1;
}

int eof_note_is_hopo(unsigned long cnote)
{
	double delta;
	float hopo_delta = eof_song->tags->eighth_note_hopo ? 250.0 : 170.0;	//The proximity threshold to determine HOPO eligibility, when the tempo is 120BPM (within 1/3 beat, or 1/2 beat if 8th note HOPO option is enabled)
	unsigned long i, cnotepos;
	long pnote;
	long beat = -1;
	double bpm;
	double scale;	//Scales the proximity threshold for the given beat's tempo

	eof_log("eof_note_is_hopo() entered", 2);

	if((eof_song->track[eof_selected_track]->track_behavior != EOF_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_behavior != EOF_PRO_GUITAR_TRACK_BEHAVIOR))
		return 0;	//Only guitar/bass tracks can have HOPO notes

	if(eof_hopo_view == EOF_HOPO_MANUAL)
	{
		if(eof_get_note_flags(eof_song, eof_selected_track, cnote) & EOF_NOTE_FLAG_F_HOPO)
		{	//In manual HOPO preview mode, a note only shows as a HOPO if it is explicitly marked as one
			return 1;
		}
		return 0;
	}
	cnotepos = eof_get_note_pos(eof_song, eof_selected_track, cnote);
	for(i = 0; i < eof_song->beats - 1; i++)
	{
		if((eof_song->beat[i]->pos <= cnotepos) && (eof_song->beat[i + 1]->pos > cnotepos))
		{
			beat = i;
			break;
		}
	}
	if(beat < 0)
	{
		return 0;
	}
	bpm = (double)60000000.0 / (double)eof_song->beat[beat]->ppqn;
	scale = 120.0 / bpm;
	if(cnote > 0)
	{
		pnote = eof_get_previous_note(cnote);
		if(pnote < 0)
		{
			return 0;
		}
		if(eof_hopo_view == EOF_HOPO_RF)
		{
			delta = cnotepos - eof_get_note_pos(eof_song, eof_selected_track, pnote);
			if((delta <= (hopo_delta * scale)) && (eof_note_count_colors(eof_song, eof_selected_track, pnote) == 1) && (eof_note_count_colors(eof_song, eof_selected_track, cnote) == 1) && (eof_get_note_note(eof_song, eof_selected_track, pnote) != eof_get_note_note(eof_song, eof_selected_track, cnote)))
			{	//If this note is close enough to the previous note, both notes are not chords, and both notes are a different gem
				return 1;
			}
		}
		else if(eof_hopo_view == EOF_HOPO_FOF)
		{
			delta = cnotepos - (eof_get_note_pos(eof_song, eof_selected_track, pnote) + eof_get_note_length(eof_song, eof_selected_track, pnote));
			if((delta <= hopo_delta * scale) && !(eof_get_note_note(eof_song, eof_selected_track, pnote) & eof_get_note_note(eof_song, eof_selected_track, cnote)))
			{	//If this note is close enough to the previous note and both notes have no gems in common
				return 1;
			}
		}
//		allegro_message("bpm = %f\nscale = %f\ndelta = %f\npnote = %d(%d), cnote = %d(%d)", bpm, scale, delta, pnote, eof_song->legacy_track[tracknum]->note[pnote].pos, cnote, eof_song->legacy_track[tracknum]->note[cnote].pos);	//Debug
	}
	return 0;
}

void eof_determine_phrase_status(EOF_SONG *sp, unsigned long track)
{
	unsigned long i, j, tracknum;
	char st[EOF_MAX_PHRASES] = {0};
	char so[EOF_MAX_PHRASES] = {0};
	char trills[EOF_MAX_PHRASES] = {0};
	char tremolos[EOF_MAX_PHRASES] = {0};
	char arpeggios[EOF_MAX_PHRASES] = {0};
	char sliders[EOF_MAX_PHRASES] = {0};
	unsigned long notepos, flags, numphrases, numnotes;
	unsigned char notetype;
	EOF_PHRASE_SECTION *sectionptr = NULL;

	eof_log("eof_determine_phrase_status() entered", 2);

	if(!sp || (track >= sp->tracks))
		return;	//Invalid parameters
	if(!eof_music_paused)
		return;	//Do not allow this to run during playback because it causes too much lag when switching to a track with a large number of notes

	tracknum = sp->track[track]->tracknum;
	numnotes = eof_get_track_size(sp, track);
	for(i = 0; i < numnotes; i++)
	{	//For each note in the active track
		/* clear the flags */
		notepos = eof_get_note_pos(sp, track, i);
		notetype = eof_get_note_type(sp, track, i);
		flags = eof_get_note_flags(sp, track, i);
		flags &= (~EOF_NOTE_FLAG_HOPO);
		flags &= (~EOF_NOTE_FLAG_SP);
		flags &= (~EOF_NOTE_FLAG_IS_TRILL);
		flags &= (~EOF_NOTE_FLAG_IS_TREMOLO);
		if((sp->track[track]->track_behavior == EOF_GUITAR_TRACK_BEHAVIOR) && (sp->track[track]->track_format == EOF_LEGACY_TRACK_FORMAT))
		{	//Only clear the is slider flag if this is a legacy guitar track
			flags &= (~EOF_GUITAR_NOTE_FLAG_IS_SLIDER);
		}

		/* mark HOPO */
		if(eof_note_is_hopo(i))
		{
			flags |= EOF_NOTE_FLAG_HOPO;
		}

		/* mark and check star power notes */
		numphrases = eof_get_num_star_power_paths(sp, track);
		for(j = 0; j < numphrases; j++)
		{	//For each star power path in the active track
			sectionptr = eof_get_star_power_path(sp, track, j);
			if((notepos >= sectionptr->start_pos) && (notepos <= sectionptr->end_pos))
			{	//If the note is in this star power section
				flags |= EOF_NOTE_FLAG_SP;
				st[j] = 1;
			}
		}

		/* check solos */
		numphrases = eof_get_num_solos(sp, track);
		for(j = 0; j < numphrases; j++)
		{	//For each solo section in the active track
			sectionptr = eof_get_solo(sp, track, j);
			if((notepos >= sectionptr->start_pos) && (notepos <= sectionptr->end_pos))
			{	//If the note is in this solo section
				so[j] = 1;
			}
		}

		/* mark and check trills */
		numphrases = eof_get_num_trills(sp, track);
		for(j = 0; j < numphrases; j++)
		{	//For each trill section in the active track
			sectionptr = eof_get_trill(sp, track, j);
			if((notepos >= sectionptr->start_pos) && (notepos <= sectionptr->end_pos))
			{	//If the note is in this trill section
				flags |= EOF_NOTE_FLAG_IS_TRILL;
				trills[j] = 1;
			}
		}

		/* mark and check tremolos */
		numphrases = eof_get_num_tremolos(sp, track);
		for(j = 0; j < numphrases; j++)
		{	//For each tremolo section in the active track
			sectionptr = eof_get_tremolo(sp, track, j);
			if((notepos >= sectionptr->start_pos) && (notepos <= sectionptr->end_pos))
			{	//If the note is in this tremolo section
				if(eof_song->track[track]->flags & EOF_TRACK_FLAG_UNLIMITED_DIFFS)
				{	//If the track's difficulty limit has been removed
					if(sectionptr->difficulty == notetype)	//And the tremolo section applies to this note's track difficulty
						flags |= EOF_NOTE_FLAG_IS_TREMOLO;
				}
				else
				{
					if(sectionptr->difficulty == 0xFF)	//Otherwise if the tremolo section applies to all track difficulties
						flags |= EOF_NOTE_FLAG_IS_TREMOLO;
				}

				tremolos[j] = 1;
			}
		}

		/* check arpeggios */
		if(sp->track[track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If this is a pro guitar track
			for(j = 0; j < sp->pro_guitar_track[tracknum]->arpeggios; j++)
			{	//For each arpeggio section in the active track
				sectionptr = &sp->pro_guitar_track[tracknum]->arpeggio[j];
				if((notepos >= sectionptr->start_pos) && (notepos <= sectionptr->end_pos))
				{	//If the note is in this arpeggio section
					if((sectionptr->difficulty == 0xFF) || (sectionptr->difficulty == notetype))
					{	//If the arpeggio section applies to all difficulties or if it applies to this note's track difficulty
						arpeggios[j] = 1;
					}
				}
			}
		}

		/* mark and check sliders */
		if((sp->track[track]->track_behavior == EOF_GUITAR_TRACK_BEHAVIOR) && (sp->track[track]->track_format == EOF_LEGACY_TRACK_FORMAT))
		{	//Only check the is slider flag if this is a legacy guitar track
			numphrases = eof_get_num_sliders(sp, track);
			for(j = 0; j < numphrases; j++)
			{	//For each slider section in the active track
				sectionptr = eof_get_slider(sp, track, j);
				if((notepos >= sectionptr->start_pos) && (notepos <= sectionptr->end_pos))
				{	//If the note is in this slider section
					flags |= EOF_GUITAR_NOTE_FLAG_IS_SLIDER;
					sliders[j] = 1;
				}
			}
		}

		eof_set_note_flags(sp, track, i, flags);	//Update the note's flags variable
	}//For each note in the active track

	/* delete star power phrases with no notes */
	numphrases = eof_get_num_star_power_paths(sp, track);
	for(j = numphrases; j > 0; j--)
	{	//For each star power section in the active track (in reverse order)
		if(!st[j - 1])
		{	//If the section's note count taken above was 0
			eof_track_delete_star_power_path(sp, track, j - 1);
		}
	}

	/* delete solos with no notes */
	numphrases = eof_get_num_solos(sp, track);
	for(j = numphrases; j > 0; j--)
	{	//For each solo section in the active track (in reverse order)
		if(!so[j - 1])
		{	//If the section's note count taken above was 0
			eof_track_delete_solo(sp, track, j - 1);
		}
	}

	/* delete trills with no notes */
	numphrases = eof_get_num_trills(sp, track);
	for(j = numphrases; j > 0; j--)
	{	//For each trill section in the active track (in reverse order)
		if(!trills[j - 1])
		{	//If the section's note count taken above was 0
			eof_track_delete_trill(sp, track, j - 1);
		}
	}

	/* delete tremolos with no notes */
	numphrases = eof_get_num_tremolos(sp, track);
	for(j = numphrases; j > 0; j--)
	{	//For each tremolo section in the active track (in reverse order)
		if(!tremolos[j - 1])
		{	//If the section's note count taken above was 0
			eof_track_delete_tremolo(sp, track, j - 1);
		}
	}

	/* delete arpeggios with no notes */
	if(sp->track[track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
	{	//If this is a pro guitar track
		for(j = sp->pro_guitar_track[tracknum]->arpeggios; j > 0; j--)
		{	//For each arpeggio section in the active track (in reverse order)
			if(!arpeggios[j - 1])
			{	//If the section's note count taken above was 0
				eof_pro_guitar_track_delete_arpeggio(sp->pro_guitar_track[tracknum], j - 1);
			}
		}
	}

	/* delete sliders with no notes */
	numphrases = eof_get_num_sliders(sp, track);
	for(j = numphrases; j > 0; j--)
	{	//For each slider section in the active track (in reverse order)
		if(!sliders[j - 1])
		{	//If the section's note count taken above was 0
			eof_track_delete_slider(sp, track, j - 1);
		}
	}
}

int eof_figure_difficulty(void)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	int nt[5] = {0};
	int dt[5] = {-1, -1, -1, -1};
	int count = 0;

	eof_log("eof_figure_difficulty() entered", 1);

	if(eof_selected_track > EOF_TRACK_VOCALS)
		return -1;	//FoFiX does not support any custom, keys or pro tracks

	/* check for lyrics if PART VOCALS is selected */
	if(eof_selected_track == EOF_TRACK_VOCALS)
	{
		/* see if there are any pitched lyrics */
		for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
		{
			if(eof_song->vocal_track[tracknum]->lyric[i]->note != 0)
			{
				break;
			}
		}

		/* if we have pitched lyrics and line definitions, allow test */
		if((i < eof_song->vocal_track[tracknum]->lyrics) && (eof_song->vocal_track[tracknum]->lines > 0))
		{
			return 0;
		}
		return -1;
	}

	/* see which difficulties are populated with notes */
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		nt[(int)eof_get_note_type(eof_song, eof_selected_track, i)] = 1;
	}

	/* no notes in this difficulty so don't allow test */
	if(!nt[eof_note_type])
	{
		return -1;
	}
	for(i = 0; i < 5; i++)
	{
		if(nt[i])
		{
			dt[i] = count;
			count++;
		}
	}
	return dt[eof_note_type];
}

/* total is used to determine the total number of notes including unselected notes */
unsigned long eof_count_selected_notes(unsigned long * total, char v)
{
//	eof_log("eof_count_selected_notes() entered");

	unsigned long tracknum;
	unsigned long count=0, i;
	long last = -1;

	if(!eof_song || (eof_selected_track >= eof_song->tracks))
		return 0;	//Return error

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	switch(eof_song->track[eof_selected_track]->track_format)
	{
		case EOF_LEGACY_TRACK_FORMAT:
			for(i = 0; i < eof_song->legacy_track[tracknum]->notes; i++)
			{
				if(eof_song->legacy_track[tracknum]->note[i]->type == eof_note_type)
				{
					if(eof_selection.track == eof_selected_track)
					{
						if(eof_selection.multi[i])
						{
							if((!v) || (v && eof_song->legacy_track[tracknum]->note[i]->note))
							{
								last = i;
								count++;
							}
						}
					}
					if(total)
					{
						(*total)++;
					}
				}
			}
		break;

		case EOF_VOCAL_TRACK_FORMAT:
			for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
			{
				if(eof_selection.track == eof_selected_track)
				{
					if(eof_selection.multi[i])
					{
						last = i;
						count++;
					}
				}
				if(total)
				{
					(*total)++;
				}
			}
		break;

		case EOF_PRO_GUITAR_TRACK_FORMAT:
			for(i = 0; i < eof_song->pro_guitar_track[tracknum]->notes; i++)
			{
				if(eof_song->pro_guitar_track[tracknum]->note[i]->type == eof_note_type)
				{
					if(eof_selection.track == eof_selected_track)
					{
						if(eof_selection.multi[i])
						{
							if((!v) || (v && eof_song->pro_guitar_track[tracknum]->note[i]->note))
							{
								last = i;
								count++;
							}
						}
					}
					if(total)
					{
						(*total)++;
					}
				}
			}
		break;
	}

	/* if no notes remain selected, current note should be unset */
	if(count == 0)
	{
		eof_selection.current = EOF_MAX_NOTES - 1;
		eof_selection.current_pos = 0;
	}

	/* if only one note is selected, make sure current note is set to that note */
	else if(count == 1)
	{
		eof_selection.current = last;
	}
	return count;
}

int eof_figure_part(void)
{
	int part[EOF_TRACKS_MAX] = {-1};	//Ensure that any tracks FoF don't support reflect -1 being returned

	eof_log("eof_figure_part() entered", 1);

	if(eof_get_track_size(eof_song,eof_selected_track) == 0)
		return -1;

	part[EOF_TRACK_GUITAR] = 0;
	part[EOF_TRACK_RHYTHM] = 1;
	part[EOF_TRACK_BASS] = 2;
	part[EOF_TRACK_GUITAR_COOP] = 3;
	part[EOF_TRACK_DRUM] = 4;
	part[EOF_TRACK_VOCALS] = 5;
	part[EOF_TRACK_KEYS] = 6;
	return part[eof_selected_track];
}

int eof_load_ogg_quick(char * filename)
{
	int loaded = 0;

	eof_log("eof_load_ogg_quick() entered", 1);

	if(!filename)
	{
		return 0;
	}
	eof_destroy_ogg();
	eof_music_data = (void *)eof_buffer_file(filename, 0);
	eof_music_data_size = file_size_ex(filename);
	if(eof_music_data)
	{
		eof_music_track = alogg_create_ogg_from_buffer(eof_music_data, eof_music_data_size);
		if(eof_music_track)
		{
			loaded = 1;
			if(!eof_silence_loaded)
			{	//Only display song channel and sample rate warnings if chart audio is loaded
				if(!eof_write_rs_files)
				{	//If not authoring for Rocksmith
					if(alogg_get_wave_freq_ogg(eof_music_track) != 44100)
					{
						allegro_message("OGG sampling rate is not 44.1khz.\nSong may not play back at the\ncorrect speed in FOF.");
					}
					if(!alogg_get_wave_is_stereo_ogg(eof_music_track))
					{
						allegro_message("OGG is not stereo.\nSong may not play back\ncorrectly in FOF.");
					}
				}
			}
		}
	}
	if(loaded)
	{
		eof_music_length = alogg_get_length_msecs_ogg(eof_music_track);
		eof_truncate_chart(eof_song);	//Remove excess beat markers and update the eof_chart_length variable
	}
	else
	{
		if(eof_music_data)
		{
			free(eof_music_data);
		}
	}
	(void) ustrncpy(eof_loaded_ogg_name,filename,1024 - 1);	//Store the loaded OGG filename
	eof_loaded_ogg_name[1023] = '\0';	//Guarantee NULL termination
	return loaded;
}

int eof_load_ogg(char * filename, char silence_failover)
{
	char * returnedfn = NULL;
	char * ptr = filename;	//Used to refer to the OGG file that was processed from memory buffer
	char directory[1024] = {0};
	int loaded = 0;
	char load_silence = 0;
	char * emptystring = "";

	eof_log("eof_load_ogg() entered", 1);

	if(!filename)
	{
		return 0;
	}
	eof_destroy_ogg();
	eof_music_data = (void *)eof_buffer_file(filename, 0);
	eof_music_data_size = file_size_ex(filename);
	if(!eof_music_data)
	{	//If the referenced file couldn't be buffered to memory, have the user browse for another file
		(void) replace_filename(directory, filename, "", 1024);	//Get the path of the target file's parent directory
		returnedfn = ncd_file_select(0, directory, "Select Music File", eof_filter_music_files);
		eof_clear_input();
		if(returnedfn)
		{	//User selected an OGG or MP3 file, write guitar.ogg into the chart's destination folder accordingly
			ptr = returnedfn;
			(void) replace_filename(directory, filename, "", 1024);	//Store the path of the file's parent folder
			if(!eof_mp3_to_ogg(returnedfn, directory))			//Create guitar.ogg in the folder
			{	//If the copy or conversion to create guitar.ogg succeeded
				(void) replace_filename(returnedfn, filename, "guitar.ogg", 1024);	//guitar.ogg is the expected file
				eof_music_data = (void *)eof_buffer_file(returnedfn, 0);
				eof_music_data_size = file_size_ex(returnedfn);
			}
		}
		else if(silence_failover)
		{	//If the user canceled loading audio, and the calling function allows defaulting to second_of_silence.ogg
			load_silence = 1;
			ptr = emptystring;
			get_executable_name(directory, 1024);	//Get EOF's executable path
			(void) replace_filename(directory, directory, "second_of_silence.ogg", 1024);
			eof_music_data = (void *)eof_buffer_file(directory, 0);
			eof_music_data_size = file_size_ex(directory);
		}
	}
	if(eof_music_data)
	{	//If the OGG file was able to buffer to memory
		if(load_silence)
		{	//If EOF failed over to silent audio
			eof_silence_loaded = 1;	//Track this condition
		}
		else
		{
			eof_silence_loaded = 0;
		}
		eof_music_track = alogg_create_ogg_from_buffer(eof_music_data, eof_music_data_size);
		if(eof_music_track)
		{
			loaded = 1;
			if(!eof_silence_loaded)
			{	//Only display song channel and sample rate warnings if chart audio is loaded
				if(!eof_write_rs_files)
				{	//If not authoring for Rocksmith
					if(alogg_get_wave_freq_ogg(eof_music_track) != 44100)
					{
						allegro_message("OGG sampling rate is not 44.1khz.\nSong may not play back at the\ncorrect speed in FOF.");
					}
					if(!alogg_get_wave_is_stereo_ogg(eof_music_track))
					{
						allegro_message("OGG is not stereo.\nSong may not play back\ncorrectly in FOF.");
					}
				}
			}
			eof_music_length = alogg_get_length_msecs_ogg(eof_music_track);
			eof_truncate_chart(eof_song);	//Remove excess beat markers and update the eof_chart_length variable
			(void) ustrncpy(eof_loaded_ogg_name,filename,1024 - 1);	//Store the loaded OGG filename
			eof_loaded_ogg_name[1023] = '\0';	//Guarantee NULL termination
		}
	}
	if(!loaded)
	{
		allegro_message("Unable to load OGG file.\n%s\nMake sure your file is a valid OGG file.", ptr);
		if(eof_music_data)
		{
			free(eof_music_data);
		}
		eof_music_data = NULL;
	}

	eof_fix_window_title();
	return loaded;
}

int eof_destroy_ogg(void)
{
	int failed = 1;

	eof_log("eof_destroy_ogg() entered", 1);

	if(eof_music_track)
	{
		alogg_destroy_ogg(eof_music_track);
		free(eof_music_data);
		eof_music_track = NULL;
		eof_music_data = NULL;
		failed = 0;
	}
	return !failed;
}

int eof_save_ogg(char * fn)
{
	PACKFILE * fp;

	eof_log("eof_save_ogg() entered", 1);

	if(fn && eof_music_data)
	{
		fp = pack_fopen(fn, "w");
		if(!fp)
		{
			return 0;
		}
		(void) pack_fwrite(eof_music_data, eof_music_data_size, fp);
		(void) pack_fclose(fp);
		return 1;
	}
	return 0;
}

/* used for debugging */
int eof_notes_selected(void)
{
	int count = 0;
	int i;

	eof_log("eof_notes_selected() entered", 1);

	for(i = 0; i < EOF_MAX_NOTES; i++)
	{
		if(eof_selection.multi[i])
		{
			count++;
		}
	}
	return count;
}

void eof_fix_waveform_graph(void)
{
	eof_log("eof_fix_waveform_graph() entered", 1);

	if(eof_music_paused && eof_waveform)
	{
		eof_destroy_waveform(eof_waveform);
		eof_waveform = eof_create_waveform(eof_loaded_ogg_name,1);	//Generate 1ms waveform data from the current audio file
		if(eof_waveform)
		{
			eof_waveform_menu[0].flags = D_SELECTED;	//Check the Show item in the Song>Waveform graph menu
		}
		else
		{
			eof_display_waveform = 0;
			eof_waveform_menu[0].flags = 0;	//Clear the Show item in the Song>Waveform graph menu
		}
	}
}

void eof_fix_spectrogram(void)
{
	eof_log("eof_fix_spectrogram() entered", 1);

	if(eof_music_paused && eof_spectrogram)
	{
		eof_destroy_spectrogram(eof_spectrogram);
		eof_spectrogram = eof_create_spectrogram(eof_loaded_ogg_name);	//Generate 1ms spectrogram data from the current audio file
		if(eof_spectrogram)
		{
			eof_spectrogram_menu[0].flags = D_SELECTED;	//Check the Show item in the Song>Waveform graph menu
		}
		else
		{
			eof_display_spectrogram = 0;
			eof_spectrogram_menu[0].flags = 0;	//Clear the Show item in the Song>Waveform graph menu
		}
	}
}

/* read keys that are universally usable */
void eof_read_global_keys(void)
{
//	eof_log("eof_read_global_keys() entered");

	/* exit program (Esc) */
	if(key[KEY_ESC] && !KEY_EITHER_SHIFT)
	{
		eof_menu_file_exit();
		key[KEY_ESC] = 0;	//If user cancelled quitting, make sure these keys are cleared
		key[KEY_N] = 0;
	}

	if(!eof_music_paused)
	{
		return;
	}

	/* activate the menu when ALT is pressed */
	if(KEY_EITHER_ALT)
	{
		clear_keybuf();
		eof_cursor_visible = 0;
		eof_emergency_stop_music();
		eof_render();
		(void) eof_popup_dialog(eof_main_dialog, 0);
		eof_cursor_visible = 1;
		eof_pen_visible = 1;
		eof_show_mouse(NULL);
	}

	/* switch between window and full screen mode */
/*	if(KEY_EITHER_ALT && key[KEY_ENTER])
	{
		if(eof_windowed == 1)
		{
			eof_windowed = 0;
			if(set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0))
			{
				allegro_message("Can't set up screen!  Error: %s",allegro_error);
				exit(1);
			}
			set_palette(eof_palette);
			ncdfs_use_allegro = 1;
		}
		else
		{
			eof_windowed = 1;
			if(set_gfx_mode(GFX_AUTODETECT_WINDOWED, 640, 480, 0, 0))
			{
				allegro_message("Can't set up screen!  Error: %s",allegro_error);
				exit(1);
			}
			set_palette(eof_palette);
			ncdfs_use_allegro = 0;
		}
	}
*/

	/* stuff you can only do when a chart is loaded */
	if(eof_song_loaded && eof_song)
	{
		/* undo (CTRL+Z) */
		if(KEY_EITHER_CTRL && key[KEY_Z] && (eof_undo_count > 0))
		{
			eof_menu_edit_undo();
			key[KEY_Z] = 0;
			eof_reset_lyric_preview_lines();	//Rebuild the preview lines
		}

		/* redo (CTRL+R) */
		if(KEY_EITHER_CTRL && !KEY_EITHER_SHIFT && key[KEY_R] && eof_redo_toggle)
		{
			eof_menu_edit_redo();
			key[KEY_R] = 0;
			eof_reset_lyric_preview_lines();	//Rebuild the preview lines
		}

		/* decrease tempo by 1BPM (-) */
		/* decrease tempo by .1BPM (SHIFT+-) */
		/* decrease tempo by .01BPM (SHIFT+CTRL+-) */
		double tempochange;
		if(key[KEY_MINUS])
		{
			if(KEY_EITHER_SHIFT)
			{	//If SHIFT is held
				eof_shift_used = 1;	//Track that the SHIFT key was used
				if(KEY_EITHER_CTRL)
				{	//If both SHIFT and CTRL are held
					tempochange = -0.01;
				}
				else
				{	//If only SHIFT is held
					tempochange = -0.1;
				}
				eof_menu_beat_adjust_bpm(tempochange);
				key[KEY_MINUS] = 0;
			}
			else
			{	//If SHIFT is not held
				if(!KEY_EITHER_CTRL)
				{	//If the CTRL key is not held (CTRL+- is reserved for decrement pro guitar fret value
					tempochange = -1.0;
					eof_menu_beat_adjust_bpm(tempochange);
					key[KEY_MINUS] = 0;
				}
			}
		}

		/* increase tempo by 1BPM (+) */
		/* increase tempo by .1BPM (SHIFT+(plus)) */
		/* increase tempo by .01BPM (SHIFT+CTRL+(plus)) */
		if(key[KEY_EQUALS])
		{
			if(KEY_EITHER_SHIFT)
			{	//If SHIFT is held
				eof_shift_used = 1;	//Track that the SHIFT key was used
				if(KEY_EITHER_CTRL)
				{	//If both SHIFT and CTRL are held
					tempochange = 0.01;
				}
				else
				{	//If only the SHIFT key is held
					tempochange = 0.1;
				}
				eof_menu_beat_adjust_bpm(tempochange);
				key[KEY_EQUALS] = 0;
			}
			else
			{	//The SHIFT key is not held
				if(!KEY_EITHER_CTRL)
				{	//If the CTRL key is not held (CTRL++ is reserved for increment pro guitar fret value)
					tempochange = 1.0;
					eof_menu_beat_adjust_bpm(tempochange);
					key[KEY_EQUALS] = 0;
				}
			}
		}
	}//If a song is loaded

	/* save (F2 or CTRL+S) */
	if((key[KEY_F2] && !KEY_EITHER_CTRL && !KEY_EITHER_SHIFT) || (key[KEY_S] && KEY_EITHER_CTRL && !KEY_EITHER_SHIFT))
	{
		eof_menu_file_save();
		key[KEY_F2] = 0;
		key[KEY_S] = 0;
	}

	/* find next catalog match (F3) */
	if(key[KEY_F3])
	{
		if(!KEY_EITHER_CTRL && !KEY_EITHER_SHIFT)
		{	//Neither CTRL nor SHIFT are held
			eof_menu_catalog_find_next();
			key[KEY_F3] = 0;
		}
	}

	/* find previous catalog match (CTRL+SHIFT+G) */
	if(key[KEY_G] && KEY_EITHER_SHIFT && KEY_EITHER_CTRL)
	{
		eof_shift_used = 1;	//Track that the SHIFT key was used
		eof_menu_catalog_find_prev();
		key[KEY_G] = 0;
	}

	/* load chart (CTRL+O) */
	if(KEY_EITHER_CTRL && key[KEY_O])
	{	//File>Load
		clear_keybuf();
		eof_menu_file_load();
		key[KEY_O] = 0;
	}

	/* new chart (F4 or CTRL+N) */
	if((key[KEY_F4] && !KEY_EITHER_CTRL && !KEY_EITHER_SHIFT) || (KEY_EITHER_CTRL && key[KEY_N]))
	{
		clear_keybuf();
		eof_menu_file_new_wizard();
		key[KEY_F4] = 0;
	}

	if(!KEY_EITHER_CTRL && !KEY_EITHER_SHIFT)
	{	//If neither CTRL nor SHIFT are held
	/* show help */
		if(key[KEY_F1])
		{
			clear_keybuf();
			eof_menu_help_keys();
			key[KEY_ESC] = 0;
			key[KEY_F1] = 0;
		}

	/* show waveform graph (F5) */
		else if(key[KEY_F5])
		{
			clear_keybuf();
			eof_menu_song_waveform();
			key[KEY_F5] = 0;
		}

	/* midi import (F6) */
		else if(key[KEY_F6])
		{	//Launch MIDI import
			clear_keybuf();
			eof_menu_file_midi_import();
			key[KEY_F6] = 0;
		}

	/* feedback import (F7) */
		else if(key[KEY_F7])
		{	//Launch Feedback chart import
			clear_keybuf();
			eof_menu_file_feedback_import();
			key[KEY_F7] = 0;
		}

	/* lyric import (F8) */
		else if(key[KEY_F8])
		{
			clear_keybuf();
			eof_menu_file_lyrics_import();
			key[KEY_F8] = 0;
		}

	/* song properties (F9) */
		else if(key[KEY_F9])
		{
			clear_keybuf();
			eof_menu_song_properties();
			key[KEY_F9] = 0;
		}

	/* settings (F10) */
		else if(key[KEY_F10])
		{
			clear_keybuf();
			eof_menu_file_settings();
			key[KEY_F10] = 0;
		}

	/* preferences (F11) */
		else if(key[KEY_F11])
		{
			clear_keybuf();
			eof_menu_file_preferences();
			key[KEY_F11] = 0;
		}

	/* test in FoF (F12) */
		else if(key[KEY_F12])
		{
			clear_keybuf();
			eof_menu_song_test_fof();
			key[KEY_F12] = 0;
		}
	}//If neither CTRL nor SHIFT are held
}

void eof_lyric_logic(void)
{
//	eof_log("eof_lyric_logic() entered");

	int note[7] = {0, 2, 4, 5, 7, 9, 11};
	int bnote[7] = {1, 3, 0, 6, 8, 10, 0};
	unsigned long i, k;
	unsigned long tracknum;
	eof_hover_key = -1;
	int eof_mouse_x = mouse_x, eof_mouse_y = mouse_y;	//Rescaled mouse coordinates that account for the x2 zoom display feature

	if(eof_song == NULL)	//Do not allow lyric processing to occur if no song is loaded
		return;

	if(!eof_vocals_selected)
		return;

	if(eof_screen_zoom)
	{	//If x2 zoom is in effect, take that into account for the mouse position
		eof_mouse_x = mouse_x / 2;
		eof_mouse_y = mouse_y / 2;
	}

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	if(eof_music_paused)
	{
		if((eof_mouse_x >= eof_window_3d->x) && (eof_mouse_x < eof_window_3d->x + eof_window_3d->w) && (eof_mouse_y >= eof_window_3d->y + eof_window_3d->h - eof_screen_layout.lyric_view_key_height))
		{
			/* check for black key */
			if(eof_mouse_y < eof_window_3d->y + eof_window_3d->h - eof_screen_layout.lyric_view_key_height + eof_screen_layout.lyric_view_bkey_height)
			{
				for(i = 0; i < 28; i++)
				{
					k = i % 7;
					if((k == 0) || (k == 1) || (k == 3) || (k == 4) || (k == 5))
					{
						if((eof_mouse_x - eof_window_3d->x >= i * eof_screen_layout.lyric_view_key_width + eof_screen_layout.lyric_view_key_width / 2 + eof_screen_layout.lyric_view_bkey_width) && (eof_mouse_x - eof_window_3d->x <= (i + 1) * eof_screen_layout.lyric_view_key_width + eof_screen_layout.lyric_view_key_width / 2 - eof_screen_layout.lyric_view_bkey_width + 1))
						{
							eof_hover_key = MINPITCH + (i / 7) * 12 + bnote[k];
							break;
						}
					}
				}
			}

			/* otherwise, white key */
			if(eof_hover_key < 0)
			{
				for(i = 0; i < 29; i++)
				{
					k = i % 7;
					if((eof_mouse_x - eof_window_3d->x >= i * eof_screen_layout.lyric_view_key_width) && (eof_mouse_x - eof_window_3d->x < i * eof_screen_layout.lyric_view_key_width + eof_screen_layout.lyric_view_key_width - 1))
					{
						eof_hover_key = MINPITCH + (i / 7) * 12 + note[k];
						break;
					}
				}
			}
			if(eof_hover_key >= 0)
			{
				if(!eof_full_screen_3d && (mouse_b & 1))
				{
					if(KEY_EITHER_CTRL && (eof_selection.current < eof_song->vocal_track[tracknum]->lyrics))
					{
						if(eof_count_selected_notes(NULL, 0) == 1)
						{
							eof_prepare_undo(EOF_UNDO_TYPE_LYRIC_NOTE);
							eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->note = eof_hover_key;
						}
					}
					if(eof_hover_key != eof_last_tone)
					{
						eof_mix_play_note(eof_hover_key);
						eof_last_tone = eof_hover_key;
					}
				}
				else
				{
					eof_last_tone = -1;
				}
				if(!eof_full_screen_3d && ((mouse_b & 2) || key[KEY_INSERT]))
				{
					eof_vocals_offset = eof_hover_key - eof_screen_layout.vocal_view_size / 2;
					if(KEY_EITHER_CTRL)
					{
						eof_vocals_offset = (eof_hover_key / 12) * 12;
					}
					if(eof_vocals_offset < MINPITCH)
					{
						eof_vocals_offset = MINPITCH;
					}
					else if(eof_vocals_offset > MAXPITCH - eof_screen_layout.vocal_view_size + 1)
					{
						eof_vocals_offset = MAXPITCH - eof_screen_layout.vocal_view_size + 1;
					}
				}
			}
		}
		else
		{
			if(eof_selection.current < eof_song->vocal_track[tracknum]->lyrics)
			{
				eof_hover_key = eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->note;
			}
		}
	}

	if(eof_music_catalog_playback || !eof_music_paused)
	{	//If the chart or fret catalog is playing
		if(eof_hover_note >= 0)
		{	//If there is a hover note, define the hover key
			eof_hover_key = eof_song->vocal_track[tracknum]->lyric[eof_hover_note]->note;
		}
	}
}

void eof_note_logic(void)
{
	int eof_mouse_x = mouse_x, eof_mouse_y = mouse_y;	//Rescaled mouse coordinates that account for the x2 zoom display feature
//	eof_log("eof_note_logic() entered");

	if(eof_screen_zoom)
	{	//If x2 zoom is in effect, take that into account for the mouse position
		eof_mouse_x = mouse_x / 2;
		eof_mouse_y = mouse_y / 2;
	}

	if((eof_catalog_menu[0].flags & D_SELECTED) && (eof_mouse_x >= 0) && (eof_mouse_x < 90) && (eof_mouse_y > 40 + eof_window_note->y) && (eof_mouse_y < 40 + 18 + eof_window_note->y))
	{
		eof_cselected_control = eof_mouse_x / 30;
		if(!eof_full_screen_3d && (mouse_b & 1))
		{
			eof_blclick_released = 0;
		}
		if(!(mouse_b & 1))
		{
			if(!eof_blclick_released)
			{
				eof_blclick_released = 1;
				if(eof_cselected_control == 0)
				{
					(void) eof_menu_catalog_previous();
				}
				else if(eof_cselected_control == 1)
				{
					eof_catalog_play();
				}
				else if(eof_cselected_control == 2)
				{
					(void) eof_menu_catalog_next();
				}
			}
		}
	}
	else
	{
		eof_cselected_control = -1;
	}
	if((eof_mouse_y >= eof_window_note->y) && (eof_mouse_y < eof_window_note->y + 12) && (eof_mouse_x >= 0) && (eof_mouse_x < ((eof_catalog_menu[0].flags & D_SELECTED) ? text_length(font, "Fret Catalog") : text_length(font, "Information Panel"))))
	{
		if(!eof_full_screen_3d && (mouse_b & 1))
		{
			eof_blclick_released = 0;
		}
		if(!(mouse_b & 1))
		{
			if(!eof_blclick_released)
			{
				eof_blclick_released = 1;
				(void) eof_menu_catalog_show();
			}
		}
		eof_info_color = eof_color_green;
	}
	else
	{
		eof_info_color = eof_color_white;
	}
}

void eof_logic(void)
{
//	eof_log("eof_logic() entered");

	eof_read_global_keys();

	/* see if we need to activate the menu */
	if((mouse_y < eof_image[EOF_IMAGE_MENU]->h) && (mouse_b & 1))
	{
		eof_cursor_visible = 0;
		eof_emergency_stop_music();
		eof_render();
		clear_keybuf();
		(void) eof_popup_dialog(eof_main_dialog, 0);
		eof_cursor_visible = 1;
		eof_pen_visible = 1;
		eof_show_mouse(NULL);
	}

	eof_last_note = EOF_MAX_NOTES;

	get_mouse_mickeys(&eof_mickeys_x, &eof_mickeys_y);
	if(eof_song_loaded)
	{
		eof_read_editor_keys();
	}
	if(eof_music_catalog_playback)
	{	//If the fret catalog is playing
		eof_music_catalog_pos += 10;
		if(eof_music_catalog_pos - eof_av_delay > eof_song->catalog->entry[eof_selected_catalog_entry].end_pos)
		{
			eof_music_catalog_playback = 0;
			eof_music_catalog_pos = eof_song->catalog->entry[eof_selected_catalog_entry].start_pos + eof_av_delay;
			eof_stop_midi();
			alogg_stop_ogg(eof_music_track);
			alogg_seek_abs_msecs_ogg(eof_music_track, eof_music_pos);
		}
	}
	else if(!eof_music_paused)
	{	//If the chart is playing
		if(eof_smooth_pos)
		{
			if(eof_playback_speed != eof_mix_speed)
			{
				if(eof_mix_speed == 1000)		//Force full speed playback
					eof_music_pos += 10;
				else if(eof_mix_speed == 500)	//Force half speed playback
					eof_music_pos += 5;
				else if(eof_mix_speed == 250)	//Force quarter speed playback
				{
					if(eof_frame % 2 == 0)
					{	//Round up every even frame
						eof_music_pos += 3;
					}
					else
					{	//Round down every odd frame
						eof_music_pos += 2;
					}
				}
				else							//Something unexpected is going on
					eof_playback_speed = eof_mix_speed;
			}
			else
			{
				switch(eof_playback_speed)
				{
					case 1000:
					{
						eof_music_pos += 10;
						break;
					}
					case 750:
					{
						if(eof_frame % 2 == 0)
						{	//Round up every even frame
							eof_music_pos += 8;
						}
						else
						{	//Round down every odd frame
							eof_music_pos += 7;
						}
						break;
					}
					case 500:
					{
						eof_music_pos += 5;
						break;
					}
					case 250:
					{
						if(eof_frame % 2 == 0)
						{	//Round up every even frame
							eof_music_pos += 3;
						}
						else
						{	//Round down every odd frame
							eof_music_pos += 2;
						}
						break;
					}
					default:	//For custom playback rate
					{
						if(eof_frame % 2 == 0)	//If eof_frame is even
						{
							eof_music_pos += (eof_playback_speed / 100.0 + 0.5);	//Round up
						}
						else
						{
							eof_music_pos += eof_playback_speed / 100;	//Round down
						}
						break;
					}
				}
			}
		}
		else
		{
			eof_music_pos = eof_music_actual_pos;
		}
		if(eof_play_selection && (eof_music_pos - eof_av_delay > eof_music_end_pos))
		{
			eof_music_paused = 1;
			eof_music_pos = eof_music_rewind_pos;
			eof_stop_midi();
			alogg_stop_ogg(eof_music_track);
			alogg_seek_abs_msecs_ogg(eof_music_track, eof_music_pos);
			if(key[KEY_S])
			{	//If S is still being held down, replay the note selection
				eof_music_play();
			}
		}
	}//If the chart is playing
	if(eof_song_loaded)
	{
		if(eof_vocals_selected)
		{
			eof_vocal_editor_logic();
		}
		else
		{
			eof_editor_logic();
		}
	}
	eof_note_logic();
	if(eof_vocals_selected)
	{
		eof_lyric_logic();
	}
	if(eof_song_loaded)
	{
		eof_find_lyric_preview_lines();
	}
	eof_frame++;
}

static char eof_tone_name_buffer[16] = {0};
char * eof_get_tone_name(int tone)
{
//	eof_log("eof_get_tone_name() entered");

	char * note_name[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
	char * note_name_flats[12] = {"C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"};
	char ** array_to_use=note_name;	//Default to using the regular note name array

	if(eof_display_flats)				//If user enabled the feature to display flat notes
		array_to_use=note_name_flats;	//Use the other array

	(void) snprintf(eof_tone_name_buffer, sizeof(eof_tone_name_buffer) - 1, "%s%d", array_to_use[tone % 12], tone / 12 - 1);
	return eof_tone_name_buffer;
}

void eof_render_note_window(void)
{
//	eof_log("eof_render_note_window() entered");

	unsigned long i, bitmask, index;
	int pos;
	int lpos, npos, ypos;
	unsigned long tracknum;
	int xcoord;
	unsigned long numlanes;				//The number of fretboard lanes that will be rendered
	char temp[1024] = {0};
	unsigned long notepos;
	char pro_guitar_string[30] = {0};
	char difficulty1[20] = {0}, difficulty2[50] = {0}, difficulty3[50] = {0};
	int scale, chord, isslash, bassnote;	//Used when looking up the chord name (if the last selected note is not already named)

	if(eof_disable_info_panel)	//If the user disabled the info panel's rendering
		return;					//Return immediately without rendering anything

	if(eof_display_second_piano_roll)	//If a second piano roll is being rendered instead of the 3D preview
		return;							//Return immediately

	numlanes = eof_count_track_lanes(eof_song, eof_selected_track);
	if(eof_full_screen_3d)
	{	//If full screen 3D view is in effect
		eof_window_note = eof_window_note_upper_left;	//Render info panel at the top left of EOF's program window
	}
	else
	{
		eof_window_note = eof_window_note_lower_left;	//Render info panel at the lower left of EOF's program window
		clear_to_color(eof_window_note->screen, eof_color_gray);
	}

	if((eof_catalog_menu[0].flags & D_SELECTED) && eof_song->catalog->entries)
	{//If show catalog is selected and there's at least one entry
		eof_set_2D_lane_positions(eof_selected_track);	//Update the ychart[] array
		if(eof_song->track[eof_song->catalog->entry[eof_selected_catalog_entry].track] == NULL)
			return;	//If this is NULL for some reason (broken track array or corrupt catalog entry, abort rendering it

		tracknum = eof_song->track[eof_song->catalog->entry[eof_selected_catalog_entry].track]->tracknum;	//Information about the active fret catalog entry is going to be displayed
		textprintf_ex(eof_window_note->screen, font, 2, 0, eof_info_color, -1, "Fret Catalog");
		textprintf_ex(eof_window_note->screen, font, 2, 12, eof_color_white, -1, "-------------------");
		if(eof_song->catalog->entry[eof_selected_catalog_entry].name[0] != '\0')
		{	//If the active fret catalog has a defined name
			textprintf_ex(eof_window_note->screen, font, 2, 24,  eof_color_white, -1, "Entry: %lu of %lu: %s", eof_song->catalog->entries ? eof_selected_catalog_entry + 1 : 0, eof_song->catalog->entries, eof_song->catalog->entry[eof_selected_catalog_entry].name);
		}
		else
		{
			textprintf_ex(eof_window_note->screen, font, 2, 24,  eof_color_white, -1, "Entry: %lu of %lu", eof_song->catalog->entries ? eof_selected_catalog_entry + 1 : 0, eof_song->catalog->entries);
		}
		if((eof_song->track[eof_song->catalog->entry[eof_selected_catalog_entry].track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT) && (eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		{	//If the catalog entry is a pro guitar note and the active track is a legacy track
			(void) snprintf(temp, sizeof(temp) - 1, "Would paste from \"%s\" as:",eof_song->track[eof_song->catalog->entry[eof_selected_catalog_entry].track]->name);
			textout_ex(eof_window_note->screen, font, temp, 2, 53, eof_color_white, -1);
		}

		if(eof_cselected_control < 0)
		{
			draw_sprite(eof_window_note->screen, eof_image[EOF_IMAGE_CCONTROLS_BASE], 0, 40);
		}
		else
		{
			draw_sprite(eof_window_note->screen, eof_image[EOF_IMAGE_CCONTROLS_0 + eof_cselected_control], 0, 40);
		}

		/* render catalog entry */
		if(eof_song->catalog->entries > 0)
		{
			pos = eof_music_catalog_pos / eof_zoom;
			if(pos < 140)
			{
				lpos = 20;
			}
			else
			{
				lpos = 20 - (pos - 140);
			}
			/* draw fretboard area */
			rectfill(eof_window_note->screen, 0, EOF_EDITOR_RENDER_OFFSET + 25, eof_window_note->w - 1, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 1, eof_color_black);
			/* draw fretboard area */
			for(i = 0; i < numlanes; i++)
			{
				if(!i || (i + 1 >= numlanes))
				{	//Ensure the top and bottom lines extend to the left of the piano roll
					hline(eof_window_note->screen, lpos, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[i], lpos + (eof_chart_length) / eof_zoom, eof_color_white);
				}
				else if(eof_song->catalog->entry[eof_selected_catalog_entry].track != EOF_TRACK_VOCALS)
				{	//Otherwise, if not drawing the vocal editor, draw the other fret lines from the first beat marker to the end of the chart
					hline(eof_window_note->screen, lpos, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[i], lpos + (eof_chart_length) / eof_zoom, eof_color_white);
				}
			}
			vline(eof_window_note->screen, lpos + (eof_chart_length) / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 35, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 11, eof_color_white);

			/* render information about the entry */
			char emptystring[2] = "", *difficulty_name = emptystring;	//The empty string will be 2 characters, so that the first can (used to define populated status of each track) be skipped
			char diff_string[12] = {0};				//Used to generate the string for a numbered difficulty (prefix with an extra space, since the string rendering skips the populated status character used for normal tracks

			if(eof_song->track[eof_song->catalog->entry[eof_selected_catalog_entry].track]->track_format != EOF_VOCAL_TRACK_FORMAT)
			{	//If the catalog entry is not from a vocal track, determine the name of the active difficulty
				if(eof_song->track[eof_song->catalog->entry[eof_selected_catalog_entry].track]->flags & EOF_TRACK_FLAG_UNLIMITED_DIFFS)
				{	//If this track is not limited to 5 difficulties
					snprintf(diff_string, sizeof(diff_string) - 1, " Diff: %d", eof_song->catalog->entry[eof_selected_catalog_entry].type);
					difficulty_name = diff_string;
				}
				else if(eof_song->catalog->entry[eof_selected_catalog_entry].track == EOF_TRACK_DANCE)
				{	//The dance track has different difficulty names
					difficulty_name = eof_dance_tab_name[(int)eof_song->catalog->entry[eof_selected_catalog_entry].type];
				}
				else
				{	//All other tracks use the same difficulty names
					difficulty_name = eof_note_type_name[(int)eof_song->catalog->entry[eof_selected_catalog_entry].type];
				}
			}
			textprintf_ex(eof_window_note->screen, font, 2, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h, eof_color_white, -1, "%s , %s , %lums - %lums", eof_song->track[eof_song->catalog->entry[eof_selected_catalog_entry].track]->name, difficulty_name + 1, eof_song->catalog->entry[eof_selected_catalog_entry].start_pos, eof_song->catalog->entry[eof_selected_catalog_entry].end_pos);
			/* draw beat lines */
			if(pos < 140)
			{
				npos = 20;
			}
			else
			{
				npos = 20 - ((pos - 140));
			}
			for(i = 0; i < eof_song->beats; i++)
			{
				xcoord = npos + eof_song->beat[i]->pos / eof_zoom;
				if(xcoord >= eof_window_note->screen->w)
				{	//If this beat line would render off the edge of the screen
					break;	//Stop rendering them
				}
				if(xcoord >= 0)
				{	//If this beat line would render visibly, render it
					vline(eof_window_note->screen, xcoord, EOF_EDITOR_RENDER_OFFSET + 35, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 10, eof_color_white);
				}
			}
			if(eof_song->catalog->entry[eof_selected_catalog_entry].track == EOF_TRACK_VOCALS)
			{	//If drawing a vocal catalog entry
				/* clear lyric text area */
				rectfill(eof_window_note->screen, 0, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.lyric_y + 1, eof_window_note->w - 1, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.lyric_y + 1 + 16, eof_color_black);
				hline(eof_window_note->screen, lpos, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.lyric_y + 1 + 16, lpos + (eof_chart_length) / eof_zoom, eof_color_white);

				for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
				{	//For each lyric
					if(eof_song->vocal_track[tracknum]->lyric[i]->pos > eof_song->catalog->entry[eof_selected_catalog_entry].end_pos)
					{	//If this lyric is after the end of the catalog entry
						break;	//Stop processing lyrics
					}
					if(eof_song->vocal_track[tracknum]->lyric[i]->pos >= eof_song->catalog->entry[eof_selected_catalog_entry].start_pos)
					{	//If this lyric is in the catalog entry, render it
						if(eof_lyric_draw(eof_song->vocal_track[tracknum]->lyric[i], i == eof_hover_note_2 ? 2 : 0, eof_window_note) > 0)
							break;	//Break if the function indicated that the lyric was rendered beyond the clip window
					}
				}
			}//If drawing a vocal catalog entry
			else
			{	//If drawing a non vocal catalog entry
				for(i = 0; i < eof_get_track_size(eof_song, eof_song->catalog->entry[eof_selected_catalog_entry].track); i++)
				{	//For each note in the entry's track
					notepos = eof_get_note_pos(eof_song, eof_song->catalog->entry[eof_selected_catalog_entry].track, i);	//Get the note's position
					if(notepos > eof_song->catalog->entry[eof_selected_catalog_entry].end_pos)
					{	//If this note is after the end of the catalog entry
						break;	//Stop processing notes
					}
					if((eof_song->catalog->entry[eof_selected_catalog_entry].type == eof_get_note_type(eof_song, eof_song->catalog->entry[eof_selected_catalog_entry].track, i)) &&
					  (notepos >= eof_song->catalog->entry[eof_selected_catalog_entry].start_pos))
					{	//If this note is the same difficulty as that from where the catalog entry was taken, and is in the catalog entry
						(void) eof_note_draw(eof_song->catalog->entry[eof_selected_catalog_entry].track, i, i == eof_hover_note_2 ? 2 : 0, eof_window_note);
					}
				}
			}//If drawing a non vocal catalog entry
			/* draw the current position */
			int zoom = eof_av_delay / eof_zoom;	//AV delay compensated for zoom level
			if(pos > zoom)
			{
				if(pos < 140)
				{
					vline(eof_window_note->screen, 20 + pos - zoom, EOF_EDITOR_RENDER_OFFSET + 25, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 1, eof_color_green);
				}
				else
				{
					vline(eof_window_note->screen, 20 + 140 - zoom, EOF_EDITOR_RENDER_OFFSET + 25, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 1, eof_color_green);
				}
			}
		}//if(eof_song->catalog->entries > 0)
	}//If show catalog is selected
	else
	{	//If show catalog is disabled
		tracknum = eof_song->track[eof_selected_track]->tracknum;	//Information about the active track is going to be displayed
		textprintf_ex(eof_window_note->screen, font, 2, 0, eof_info_color, -1, "Information Panel");
		textprintf_ex(eof_window_note->screen, font, 2, 6, eof_color_white, -1, "----------------------------");
		ypos = 16;

		//Display the difficulties associated with the active track
		if(eof_song->track[eof_selected_track]->difficulty != 0xFF)
		{	//If the active track has a defined difficulty
			(void) snprintf(difficulty1, sizeof(difficulty1) - 1, "%d", eof_song->track[eof_selected_track]->difficulty);
		}
		else
		{
			(void) snprintf(difficulty1, sizeof(difficulty1) - 1, "(Undefined)");
		}
		difficulty2[0] = '\0';
		difficulty3[0] = '\0';
		if(eof_selected_track == EOF_TRACK_DRUM)
		{	//Write the difficulty string to display for pro drums
			if(((eof_song->track[EOF_TRACK_DRUM]->flags & 0x0F000000) >> 24) != 0x0F)
			{	//If the pro drum difficulty is defined
				(void) snprintf(difficulty2, sizeof(difficulty2) - 1, "(Pro: %lu)", (eof_song->track[EOF_TRACK_DRUM]->flags & 0x0F000000) >> 24);	//Mask out the low nibble of the high order byte of the drum track's flags (pro drum difficulty)
			}
			else
			{
				(void) snprintf(difficulty2, sizeof(difficulty2) - 1, "(Pro: Undefined)");
			}
		}
		else if(eof_selected_track == EOF_TRACK_VOCALS)
		{	//Write the difficulty string to display for vocal harmony
			if(((eof_song->track[EOF_TRACK_VOCALS]->flags & 0x0F000000) >> 24) != 0x0F)
			{	//If the harmony difficulty is defined
				(void) snprintf(difficulty2, sizeof(difficulty2) - 1, "(Harmony: %lu)", (eof_song->track[EOF_TRACK_VOCALS]->flags & 0x0F000000) >> 24);	//Mask out the high order byte of the vocal track's flags (harmony difficulty)
			}
			else
			{
				(void) snprintf(difficulty2, sizeof(difficulty2) - 1, "(Harmony: Undefined)");
			}
			difficulty3[0] = '\0';	//Unused for vocals
		}
		else
		{	//Otherwise truncate the extra difficulty strings
			difficulty2[0] = '\0';
			difficulty3[0] = '\0';
		}
		textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "Track difficulty: %s %s %s", difficulty1, difficulty2, difficulty3);
		ypos += 12;

		if(!eof_disable_sound_processing)
		{	//If the user didn't disable sound processing, display the sound cue statuses
			textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "Metronome: %s Claps: %s Tones: %s", eof_mix_metronome_enabled ? "On" : "Off", eof_mix_claps_enabled ? "On" : "Off", eof_mix_vocal_tones_enabled ? "On" : "Off");
		}
		else
		{	//Otherwise indicate they are disabled
			textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "(Sound cues are currently disabled)");
		}
		ypos += 12;
		if(eof_hover_beat >= 0)
		{
			textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "Beat = %lu : BPM = %f : Hover = %ld", eof_selected_beat, (double)60000000.0 / (double)eof_song->beat[eof_selected_beat]->ppqn, eof_hover_beat);
		}
		else
		{
			textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "Beat = %lu : BPM = %f", eof_selected_beat, (double)60000000.0 / (double)eof_song->beat[eof_selected_beat]->ppqn);
		}
///Keep for debugging
//#ifdef EOF_DEBUG
//		ypos += 12;
//		textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "#Beat pos = %lu : fpos = %f", eof_song->beat[eof_selected_beat]->pos, eof_song->beat[eof_selected_beat]->fpos);
//#endif
		ypos += 12;
		textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "Key : %s maj (%s min)", eof_get_key_signature(eof_song, eof_selected_beat, 1, 0), eof_get_key_signature(eof_song, eof_selected_beat, 1, 1));
		ypos += 12;
		if(eof_selected_measure >= 0)
		{
			textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "Measure = %d (Beat %d/%d)", eof_selected_measure, eof_beat_in_measure + 1, eof_beats_in_measure);
		}
		else
		{
			textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "Measure = (Time Sig. not defined)");
		}
		ypos += 12;
		if(eof_vocals_selected)
		{	//If the vocal track is active
			if(eof_selection.current < eof_song->vocal_track[tracknum]->lyrics)
			{
				textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "Lyric = %ld : Pos = %lu : Length = %lu", eof_selection.current, eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->pos, eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->length);
				ypos += 12;
				textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "Lyric Text = \"%s\" : Tone = %d (%s)", eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->text, eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->note, (eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->note != 0) ? eof_get_tone_name(eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->note) : "none");
			}
			else
			{
				textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "Lyric = None");
			}
			ypos += 12;
			if(eof_hover_note >= 0)
			{
				if(eof_seek_hover_note >= 0)
				{
					textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "Lyric: Hover = %d : Seek = %d", eof_hover_note, eof_seek_hover_note);
				}
				else
				{
					textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "Lyric Hover = %d : Seek = None", eof_hover_note);
				}
			}
			else
			{
				if(eof_seek_hover_note >= 0)
				{
					textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "Lyric: Hover = None : Seek = %d", eof_seek_hover_note);
				}
				else
				{
					textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "Lyric: Hover = None : Seek = None");
				}
			}
		}//If the vocal track is active
		else
		{	//If a non vocal track is active
			if(eof_selection.current < eof_get_track_size(eof_song, eof_selected_track))
			{	//If a note is selected
				textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "Note = %ld : Pos = %lu : Length = %lu", eof_selection.current, eof_get_note_pos(eof_song, eof_selected_track, eof_selection.current), eof_get_note_length(eof_song, eof_selected_track, eof_selection.current));
///Keep for debugging
//#ifdef EOF_DEBUG
//				ypos += 12;
//				textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "#Mask = %ld : Flags = %lu", eof_get_note_note(eof_song, eof_selected_track, eof_selection.current), eof_get_note_flags(eof_song, eof_selected_track, eof_selection.current));
//#endif
			}
			else
			{
				textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "Note = None");
			}
			ypos += 12;
			if(eof_seek_selection_start != eof_seek_selection_end)
			{	//If there is a seek selection
				textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "Seek selection start = %lu : stop = %lu", eof_seek_selection_start, eof_seek_selection_end);
				ypos += 12;
			}
			if(eof_hover_note >= 0)
			{
				if(eof_seek_hover_note >= 0)
				{
					textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "Note: Hover = %d : Seek = %d", eof_hover_note, eof_seek_hover_note);
				}
				else
				{
					textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "Note: Hover = %d : Seek = None", eof_hover_note);
				}
			}
			else
			{
				if(eof_seek_hover_note >= 0)
				{
					textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "Note: Hover = None : Seek = %d", eof_seek_hover_note);
				}
				else
				{
					textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "Note: Hover = None : Seek = None");
				}
			}
		}//If a non vocal track is active
		int ism = ((eof_music_pos - eof_av_delay) / 1000) / 60;
		int iss = ((eof_music_pos - eof_av_delay) / 1000) % 60;
		int isms = ((eof_music_pos - eof_av_delay) % 1000);
		unsigned long itn = 0;
		int isn = eof_count_selected_notes(&itn, 0);
		ypos += 12;
		textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "Seek Position = %02d:%02d.%03d", ism, iss, isms >= 0 ? isms : 0);
		ypos += 12;
		textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "%s Selected = %d/%lu", eof_vocals_selected ? "Lyrics" : "Notes", isn, itn);
		ypos += 12;
		textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "Input Mode: %s", eof_input_name[eof_input_mode]);
		ypos += 12;

		if(eof_snap_mode != EOF_SNAP_CUSTOM)
			textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "Grid Snap: %s", eof_snap_name[(int)eof_snap_mode]);
		else
		{
			if(eof_custom_snap_measure == 0)
				textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "Grid Snap: %s (1/%d beat)", eof_snap_name[(int)eof_snap_mode],eof_snap_interval);
			else
				textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "Grid Snap: %s (1/%d measure)", eof_snap_name[(int)eof_snap_mode],eof_snap_interval);
		}

		ypos += 12;
		textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "Playback Speed: %d%%", eof_playback_speed / 10);
		ypos += 12;
		if((eof_selected_catalog_entry < eof_song->catalog->entries) && (eof_song->catalog->entry[eof_selected_catalog_entry].name[0] != '\0'))
		{	//If the active fret catalog has a defined name
			textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "Catalog: %lu of %lu: %s", eof_song->catalog->entries ? eof_selected_catalog_entry + 1 : 0, eof_song->catalog->entries, eof_song->catalog->entry[eof_selected_catalog_entry].name);
		}
		else
		{
			textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "Catalog: %lu of %lu", eof_song->catalog->entries ? eof_selected_catalog_entry + 1 : 0, eof_song->catalog->entries);
		}
		ypos += 12;
		textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "OGG File: %s", eof_silence_loaded ? "(None)" : eof_song->tags->ogg[eof_selected_ogg].filename);

		if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//Display information specific to pro guitar tracks
			ypos += 12;
			if(!eof_pro_guitar_fret_bitmask || (eof_pro_guitar_fret_bitmask == 63))
			{	//If the fret shortcut bitmask is set to no strings or all 6 strings
				textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "Fret value shortcuts apply to %s strings", (eof_pro_guitar_fret_bitmask == 0) ? "no" : "all");
			}
			else
			{	//Build a string to indicate which strings the bitmask pertains to
				for(i = 6, bitmask = 32, index = 0; i > 0; i--, bitmask>>=1)
				{	//For each of the 6 usable strings, starting with the highest pitch string
					if(eof_pro_guitar_fret_bitmask & bitmask)
					{	//If the bitmask applies to this string
						if(index != 0)
						{	//If another string number was already written to this string
							pro_guitar_string[index++] = ',';	//Insert a comma
							pro_guitar_string[index++] = ' ';	//And a space
						}
						pro_guitar_string[index++] = '7' - i;	//'0' + # is converts a number of value # to a text character representation
					}
				}
				pro_guitar_string[index] = '\0';	//Terminate the string
				if(pro_guitar_string[1] != '\0')
				{	//If there are at least two strings denoted
					textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "Fret value shortcuts apply to strings %s", pro_guitar_string);
				}
				else
				{	//There's only one string denoted, use a shortcut to just display the one character
					textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "Fret value shortcuts apply to string %c", pro_guitar_string[0]);
				}
			}

			tracknum = eof_song->track[eof_selected_track]->tracknum;
			if((eof_selection.current < eof_song->pro_guitar_track[tracknum]->notes) && (eof_selection.track == eof_selected_track))
			{	//If a note in the active track is selected, display a line with its fretting information
				ypos += 12;
				if(eof_get_pro_guitar_note_fret_string(eof_song->pro_guitar_track[tracknum], eof_selection.current, pro_guitar_string))
				{	//If the note's frets can be represented in string format
					if(eof_song->pro_guitar_track[tracknum]->note[eof_selection.current]->name[0] != '\0')
					{	//If this note was manually given a name, display it in addition to the fretting
						textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "%s: %s", pro_guitar_string, eof_song->pro_guitar_track[tracknum]->note[eof_selection.current]->name);
						eof_enable_chord_cache = 0;	//When a manually-named note is selected, reset this variable so that previous/next chord name cannot be used
					}
					else
					{
						unsigned long matchcount;
						char chord_match_string[30] = {0};

						matchcount = eof_count_chord_lookup_matches(eof_song->pro_guitar_track[tracknum], eof_selected_track, eof_selection.current);
						if(matchcount)
						{	//If there's at least one chord lookup match, obtain the user's selected match
							eof_lookup_chord(eof_song->pro_guitar_track[tracknum], eof_selected_track, eof_selection.current, &scale, &chord, &isslash, &bassnote, eof_selected_chord_lookup, 1);	//Run a cache-able lookup
							scale %= 12;	//Ensure this is a value from 0 to 11
							bassnote %= 12;
							if(matchcount > 1)
							{	//If there's more than one match
								(void) snprintf(chord_match_string, sizeof(chord_match_string) - 1, " (match %lu/%lu)", eof_selected_chord_lookup + 1, matchcount);
							}
							if(!isslash)
							{	//If it's a normal chord
								textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "%s: [%s%s]%s", pro_guitar_string, eof_note_names[scale], eof_chord_names[chord].chordname, chord_match_string);
							}
							else
							{	//If it's a slash chord
								textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "%s: [%s%s%s]%s", pro_guitar_string, eof_note_names[scale], eof_chord_names[chord].chordname, eof_slash_note_names[bassnote], chord_match_string);
							}
						}
						else
						{	//Otherwise just display the fretting
							textout_ex(eof_window_note->screen, font, pro_guitar_string, 2, ypos, eof_color_white, -1);
							eof_chord_lookup_note = eof_get_pro_guitar_note_note(eof_song->pro_guitar_track[tracknum], eof_selection.current);		//Cache the failed, looked up note's details
							memcpy(eof_chord_lookup_frets, eof_song->pro_guitar_track[tracknum]->note[eof_selection.current]->frets, 6);
							eof_cached_chord_lookup_retval = 0;	//Cache a failed lookup result
						}
					}
				}
				ypos += 2;	//Lower the virtual "cursor" because underscores for the fretting string are rendered low enough to touch text 12 pixels below the y position of the glyph
			}//If a note in the active track is selected, display a line with its information

			EOF_PRO_GUITAR_TRACK *tp = eof_song->pro_guitar_track[tracknum];
			unsigned char position = eof_pro_guitar_track_find_effective_fret_hand_position(tp, eof_note_type, eof_music_pos - eof_av_delay);	//Find if there's a fret hand position in effect
			ypos += 12;
			if(position)
			{	//If a fret hand position is in effect
				textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "Effective fret hand position:  %u", position);
			}
			else
			{
				textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "Effective fret hand position:  None");
			}
		}//Display information specific to pro guitar tracks
	}//If show catalog is disabled

	if(!eof_full_screen_3d)
	{	//If full screen 3D view is not in effect, render a border around the info panel
		rect(eof_window_note->screen, 0, 0, eof_window_note->w - 1, eof_window_note->h - 1, eof_color_dark_silver);
		rect(eof_window_note->screen, 1, 1, eof_window_note->w - 2, eof_window_note->h - 2, eof_color_black);
		hline(eof_window_note->screen, 1, eof_window_note->h - 2, eof_window_note->w - 2, eof_color_white);
		vline(eof_window_note->screen, eof_window_note->w - 2, 1, eof_window_note->h - 2, eof_color_white);
	}
}

#define MAX_LYRIC_PREVIEW_LENGTH 255
void eof_render_lyric_preview(BITMAP * bp)
{
//	eof_log("eof_render_lyric_preview() entered");

	unsigned long currentlength=0;		//Used to track the length of the preview line being built
	unsigned long lyriclength=0;		//The length of the lyric being added
	char *tempstring=NULL;				//The code to render in green needs special handling to suppress the / character
	EOF_PHRASE_SECTION *linenum=NULL;	//Used to find the background color to render the lyric lines in (green for overdrive, otherwise transparent)
	int bgcol1=-1,bgcol2=-1;

	char lline[2][MAX_LYRIC_PREVIEW_LENGTH+1] = {{0}};
	unsigned long i,x;
	int offset = -1;
	int space;	//Track the spacing for lyric preview, taking pitch shift and grouping logic into account

	if(!bp)
	{
		return;
	}

//If the active track is being overridden by a stored MIDI track
	if(eof_track_overridden_by_stored_MIDI_track(eof_song, eof_selected_track))
	{
		textout_centre_ex(bp, font, "A stored MIDI track overrides this track", bp->w / 2, 20, eof_color_red, bgcol1);
		textout_centre_ex(bp, font, "View Song>Manage Stored MIDI tracks", bp->w / 2, 36, eof_color_red, bgcol2);
		return;
	}

//Build both lyric preview lines
	for(x = 0; x < 2; x++)
	{	//For each of the two lyric preview lines to build
		space = 0;			//The first lyric will have no space inserted in front of it
		currentlength = 0;	//Reset preview line length counter

		linenum=eof_find_lyric_line(eof_preview_line_lyric[x]);	//Find the line structure representing this lyric preview
		if(linenum && (linenum->flags & EOF_LYRIC_LINE_FLAG_OVERDRIVE))	//If this line is overdrive
		{
			if(x == 0)	//This is the first preview line
				bgcol1=makecol(64, 128, 64);	//Render the line's text with an overdrive green background
			else		//This is the second preview line
				bgcol2=makecol(64, 128, 64);	//Render the line's text with an overdrive green background
		}

		for(i = eof_preview_line_lyric[x]; i < eof_preview_line_end_lyric[x]; i++)
		{	//For each lyric in the preview line
			lyriclength=ustrlen(eof_song->vocal_track[0]->lyric[i]->text);	//This value will be used multiple times

		//Perform grouping logic
			if((eof_song->vocal_track[0]->lyric[i]->text[0] != '+'))
			{	//If the lyric is not a pitch shift
				if(space)
				{	//If space is nonzero, append a space if possible
					if(currentlength+1 <= MAX_LYRIC_PREVIEW_LENGTH)
					{	//If appending a space would NOT cause an overflow
						(void) ustrcat(lline[x], " ");
						currentlength++;	//Track the length of this preview line
					}
					else
						break;				//Stop building this line's preview
				}
			}

			if(ustrchr(eof_song->vocal_track[0]->lyric[i]->text,'-') || ustrchr(eof_song->vocal_track[0]->lyric[i]->text,'='))
			{	//If the lyric contains a hyphen or an equal sign
				space = 0;	//The next syllable will group with this one
			}
			else
			{
				space = 1;	//The next syllable will not group with this one, and will be separated by space
			}

			if(!x && !eof_music_paused)	//Only perform this logic for the first lyric preview line
			{
				if(i == eof_hover_lyric)
				{
					offset = text_length(font, lline[x]);
				}
			}

		//Perform bounds checking before appending string
			if(currentlength+lyriclength > MAX_LYRIC_PREVIEW_LENGTH)	//If appending this lyric would cause an overflow
				break;													//Stop building this line's preview

		//Append string
			(void) ustrcat(lline[x], eof_song->vocal_track[0]->lyric[i]->text);
			currentlength+=lyriclength;									//Track the length of this preview line

		//Truncate a '/' character off the end of the lyric line if it exists (TB:RB notation for a forced line break)
			if(lline[x][ustrlen(lline[x])-1] == '/')
				lline[x][ustrlen(lline[x])-1]='\0';
		}
	}

	textout_centre_ex(bp, font, lline[0], bp->w / 2, 20, eof_color_white, bgcol1);
	textout_centre_ex(bp, font, lline[1], bp->w / 2, 36, eof_color_white, bgcol2);
	if((offset >= 0) && (eof_hover_lyric >= 0))
	{
		if(eof_song->vocal_track[0]->lyric[eof_hover_lyric]->text[strlen(eof_song->vocal_track[0]->lyric[eof_hover_lyric]->text)-1] == '/')
		{	//If the at-playback position lyric ends in a forward slash, make a copy with the slash removed and display it instead
			tempstring=malloc(ustrlen(eof_song->vocal_track[0]->lyric[eof_hover_lyric]->text)+1);
			if(tempstring==NULL)	//If there wasn't enough memory to copy this string...
				return;
			(void) ustrcpy(tempstring,eof_song->vocal_track[0]->lyric[eof_hover_lyric]->text);
			tempstring[ustrlen(tempstring)-1]='\0';
			textout_ex(bp, font, tempstring, bp->w / 2 - text_length(font, lline[0]) / 2 + offset, 20, eof_color_green, -1);
			free(tempstring);
		}
		else	//Otherwise, display the regular string instead
			textout_ex(bp, font, eof_song->vocal_track[0]->lyric[eof_hover_lyric]->text, bp->w / 2 - text_length(font, lline[0]) / 2 + offset, 20, eof_color_green, -1);
	}
}

void eof_render_lyric_window(void)
{
//	eof_log("eof_render_lyric_window() entered");

	int i;
	int k, n;
	int kcol;
	int kcol2;
	int note[7] = {0, 2, 4, 5, 7, 9, 11};
	int bnote[7] = {1, 3, 0, 6, 8, 10, 0};

	if((eof_catalog_menu[0].flags & D_SELECTED) && (eof_catalog_menu[1].flags == D_SELECTED))
	{	//If the fret catalog is visible and it's configured to display at full width
		return;	//Don't draw the lyric window
	}

	if(eof_display_second_piano_roll)	//If a second piano roll is being rendered instead of the 3D preview
		return;							//Return immediately

	clear_to_color(eof_window_3d->screen, eof_color_gray);

	/* render the 29 white keys */
	for(i = 0; i < 29; i++)
	{
		n = (i / 7) * 12 + note[i % 7] + MINPITCH;
		if(n == eof_hover_key)
		{
			if((n >= eof_vocals_offset) && (n < eof_vocals_offset + eof_screen_layout.vocal_view_size))
			{
				kcol = eof_color_green;
				kcol2 = makecol(0, 192, 0);
			}
			else
			{
				kcol = makecol(0, 160, 0);
				kcol2 = makecol(0, 96, 0);
			}
			textout_centre_ex(eof_window_3d->screen, font, eof_get_tone_name(eof_hover_key), i * eof_screen_layout.lyric_view_key_width + eof_screen_layout.lyric_view_key_width / 2, eof_window_3d->h - eof_screen_layout.lyric_view_key_height - text_height(font), eof_color_white, eof_color_black);
		}
		else
		{
			if((n >= eof_vocals_offset) && (n < eof_vocals_offset + eof_screen_layout.vocal_view_size))
			{
				kcol = eof_color_white;
				kcol2 = eof_color_silver;
			}
			else
			{
				kcol = eof_color_dark_silver;
				kcol2 = makecol(96, 96, 96);
			}
		}
		rectfill(eof_window_3d->screen, i * eof_screen_layout.lyric_view_key_width, eof_window_3d->screen->h - eof_screen_layout.lyric_view_key_height, i * eof_screen_layout.lyric_view_key_width + eof_screen_layout.lyric_view_key_width - 1, eof_window_3d->screen->h, kcol);
		vline(eof_window_3d->screen, i * eof_screen_layout.lyric_view_key_width, eof_window_3d->screen->h - eof_screen_layout.lyric_view_key_height, eof_window_3d->screen->h, kcol2);
	}

	/* render black keys over white keys */
	for(i = 0; i < 28; i++)
	{
		n = (i / 7) * 12 + bnote[i % 7] + MINPITCH;
		k = n % 12;
		if((k == 1) || (k == 3) || (k == 6) || (k == 8) || (k == 10))
		{
			if(n == eof_hover_key)
			{
				if((n >= eof_vocals_offset) && (n < eof_vocals_offset + eof_screen_layout.vocal_view_size))
				{
					kcol = eof_color_green;
					kcol2 = makecol(0, 192, 0);
				}
				else
				{
					kcol = makecol(0, 160, 0);
					kcol2 = makecol(0, 96, 0);
				}
				textout_centre_ex(eof_window_3d->screen, font, eof_get_tone_name(eof_hover_key), i * eof_screen_layout.lyric_view_key_width + eof_screen_layout.lyric_view_key_width / 2 + eof_screen_layout.lyric_view_bkey_width + eof_screen_layout.lyric_view_bkey_width / 2, eof_window_3d->h - eof_screen_layout.lyric_view_key_height - text_height(font), eof_color_white, eof_color_black);
			}
			else
			{
				if((n >= eof_vocals_offset) && (n < eof_vocals_offset + eof_screen_layout.vocal_view_size))
				{
					kcol = makecol(48, 48, 48);
					kcol2 = makecol(24, 24, 24);
				}
				else
				{
					kcol = makecol(16, 16, 16);
					kcol2 = eof_color_black;
				}
			}
			rectfill(eof_window_3d->screen, i * eof_screen_layout.lyric_view_key_width + eof_screen_layout.lyric_view_key_width / 2 + eof_screen_layout.lyric_view_bkey_width, eof_window_3d->screen->h - eof_screen_layout.lyric_view_key_height, (i + 1) * eof_screen_layout.lyric_view_key_width + eof_screen_layout.lyric_view_key_width / 2 - eof_screen_layout.lyric_view_bkey_width + 1, eof_window_3d->screen->h - eof_screen_layout.lyric_view_key_height + eof_screen_layout.lyric_view_bkey_height, kcol);
			vline(eof_window_3d->screen, (i + 1) * eof_screen_layout.lyric_view_key_width + eof_screen_layout.lyric_view_key_width / 2 - eof_screen_layout.lyric_view_bkey_width + 1, eof_window_3d->screen->h - eof_screen_layout.lyric_view_key_height, eof_window_3d->screen->h - eof_screen_layout.lyric_view_key_height + eof_screen_layout.lyric_view_bkey_height, kcol2);
		}
	}
	eof_render_lyric_preview(eof_window_3d->screen);

	rect(eof_window_3d->screen, 0, 0, eof_window_3d->w - 1, eof_window_3d->h - 1, eof_color_dark_silver);
	rect(eof_window_3d->screen, 1, 1, eof_window_3d->w - 2, eof_window_3d->h - 2, eof_color_black);
	hline(eof_window_3d->screen, 1, eof_window_3d->h - 2, eof_window_3d->w - 2, eof_color_white);
	vline(eof_window_3d->screen, eof_window_3d->w - 2, 1, eof_window_3d->h - 2, eof_color_white);
}

void eof_render_3d_window(void)
{
//	eof_log("eof_render_3d_window() entered");

	static int fretboardpoint[8];		//Used to cache the 3D->2D coordinate projections for the 3D fretboard
	int point[8];
	unsigned long i;
	short numsolos = 0;					//Used to abstract the solo sections
	unsigned long numnotes;				//Used to abstract the notes
	unsigned long numlanes;				//The number of fretboard lanes that will be rendered
	unsigned long tracknum;
	unsigned long firstlane = 0, lastlane;	//Used to track the first and last lanes that get track specific rendering (ie. drums don't render markers for lane 1, bass doesn't render markers for lane 6)

	//Used to draw trill and tremolo sections:
	unsigned long j, ctr, usedlanes, bitmask, numsections;
	EOF_PHRASE_SECTION *sectionptr = NULL;	//Used to abstract sections

	if(!eof_full_screen_3d)
	{	//If full screen 3D view isn't in effect, 3D rendering can be disabled
		if(eof_disable_3d_rendering)	//If the user wanted to disable the rendering of the 3D window to improve performance
			return;						//Return immediately

		if(eof_display_second_piano_roll)	//If a second piano roll is being rendered instead of the 3D preview
			return;							//Return immediately
	}

	if((eof_catalog_menu[0].flags & D_SELECTED) && (eof_catalog_menu[1].flags == D_SELECTED))
	{	//If the fret catalog is visible and it's configured to display at full width
		return;	//Don't draw the 3D window
	}

	if(eof_song->track[eof_selected_track]->track_format == EOF_VOCAL_TRACK_FORMAT)
	{	//If this is a vocal track
		eof_render_lyric_window();
		return;
	}

	clear_to_color(eof_window_3d->screen, eof_color_gray);
	numlanes = eof_count_track_lanes(eof_song, eof_selected_track);
	lastlane = numlanes - 1;	//This variable begins lane numbering at 0 instead of 1
	eof_set_3D_lane_positions(eof_selected_track);	//Update the xchart[] array
	if(eof_selected_track == EOF_TRACK_BASS)
	{	//Special case:  The bass track can use a sixth lane but its 3D representation still only draws 5 lanes
		numlanes = 5;
		lastlane = 4;	//Don't render trill/tremolo markers for the 6th lane (render for lanes 0 through 4)
	}
	if((eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR) & !eof_render_bass_drum_in_lane)
	{	//If a drum track is active and the user hasn't enabled the preference to render the bass drum in its own lane
		firstlane = 1;		//Don't render drum roll/special drum roll markers for the first lane, 0 (unless user enabled the preference to render bass drum in its own lane)
	}

	if(!eof_3d_fretboard_coordinates_cached)
	{	//If the appropriate 3D->2D coordinate projections for the 3D fretboard aren't cached yet
		fretboardpoint[0] = ocd3d_project_x(20, 600);
		fretboardpoint[1] = ocd3d_project_y(200, 600);
		fretboardpoint[2] = ocd3d_project_x(300, 600);
		fretboardpoint[3] = fretboardpoint[1];
		fretboardpoint[4] = ocd3d_project_x(300, -100);
		fretboardpoint[5] = ocd3d_project_y(200, -100);
		fretboardpoint[6] = ocd3d_project_x(20, -100);
		fretboardpoint[7] = fretboardpoint[5];
	}
	polygon(eof_window_3d->screen, 4, fretboardpoint, eof_color_black);

	/* render solo sections */
	long sz, sez;
	long spz, spez;
	numsolos = eof_get_num_solos(eof_song, eof_selected_track);
	for(i = 0; i < numsolos; i++)
	{
		sectionptr = eof_get_solo(eof_song, eof_selected_track, i);	//Obtain the information for this legacy/pro guitar solo
		if(sectionptr != NULL)
		{
			sz = (long)(sectionptr->start_pos + eof_av_delay - eof_music_pos) / eof_zoom_3d;
			sez = (long)(sectionptr->end_pos + eof_av_delay - eof_music_pos) / eof_zoom_3d;
			if((-100 <= sez) && (600 >= sz))
			{
				spz = sz < -100 ? -100 : sz;
				spez = sez > 600 ? 600 : sez;
				point[0] = ocd3d_project_x(20, spez);
				point[1] = ocd3d_project_y(200, spez);
				point[2] = ocd3d_project_x(300, spez);
				point[3] = point[1];
				point[4] = ocd3d_project_x(300, spz);
				point[5] = ocd3d_project_y(200, spz);
				point[6] = ocd3d_project_x(20, spz);
				point[7] = point[5];
				polygon(eof_window_3d->screen, 4, point, eof_color_dark_blue);
			}
		}
	}

	/* render arpeggio sections */
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
	{
		for(i = 0; i < eof_song->pro_guitar_track[tracknum]->arpeggios; i++)
		{	//For each arpeggio section in the track
			sectionptr = &eof_song->pro_guitar_track[tracknum]->arpeggio[i];
			if(sectionptr->difficulty == eof_note_type)
			{	//If this arpeggio is assigned to the active difficulty
				sz = (long)(sectionptr->start_pos + eof_av_delay - eof_music_pos) / eof_zoom_3d;
				sez = (long)(sectionptr->end_pos + eof_av_delay - eof_music_pos) / eof_zoom_3d;
				if((-100 <= sez) && (600 >= sz))
				{	//If the arpeggio section would render visibly, fill the topmost lane with turquoise
					spz = sz < -100 ? -100 : sz;
					spez = sez > 600 ? 600 : sez;
					point[0] = ocd3d_project_x(20, spez);
					point[1] = ocd3d_project_y(200, spez);
					point[2] = ocd3d_project_x(300, spez);
					point[3] = point[1];
					point[4] = ocd3d_project_x(300, spz);
					point[5] = ocd3d_project_y(200, spz);
					point[6] = ocd3d_project_x(20, spz);
					point[7] = point[5];
					polygon(eof_window_3d->screen, 4, point, eof_color_turquoise);	//Fill with a turquoise color
				}
			}
		}
	}

	/* render seek selection */
	long halflanewidth = (56.0 * (4.0 / (numlanes-1))) / 2;
	if(eof_seek_selection_start != eof_seek_selection_end)
	{	//If there is a seek selection
		sz = (long)(eof_seek_selection_start + eof_av_delay - eof_music_pos) / eof_zoom_3d;
		sez = (long)(eof_seek_selection_end + eof_av_delay - eof_music_pos) / eof_zoom_3d;
		if((-100 <= sez) && (600 >= sz))
		{
			spz = sz < -100 ? -100 : sz;
			spez = sez > 600 ? 600 : sez;
			point[0] = ocd3d_project_x(xchart[0], spez);
			point[1] = ocd3d_project_y(200, spez);
			point[2] = ocd3d_project_x(xchart[lastlane], spez);
			point[3] = point[1];
			point[4] = ocd3d_project_x(xchart[lastlane], spz);
			point[5] = ocd3d_project_y(200, spz);
			point[6] = ocd3d_project_x(xchart[0], spz);
			point[7] = point[5];
			polygon(eof_window_3d->screen, 4, point, eof_color_red);
		}
	}

	/* render trill and tremolo sections */
	if(eof_get_num_trills(eof_song, eof_selected_track) || eof_get_num_tremolos(eof_song, eof_selected_track))
	{	//If this track has any trill or tremolo sections
		long xoffset = 0;	//This will be used to offset the trill/tremolo lane fill as necessary to center the fill over that lane's gem
		if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
		{	//If a drum track is active
			xoffset = halflanewidth;	//Drum gems render half a lane width further right (in between fret lines instead of centered over the lines)
		}

		//Build the lane X coordinate array
		for(j = 0; j < 2; j++)
		{	//For each of the two phrase types (trills and tremolos)
			if(j == 0)
			{	//On the first pass, render trill sections
				numsections = eof_get_num_trills(eof_song, eof_selected_track);
			}
			else
			{	//On the second pass, render tremolo sections
				numsections = eof_get_num_tremolos(eof_song, eof_selected_track);
			}
			for(i = 0; i < numsections; i++)
			{	//For each trill or tremolo section in the track
				if(j == 0)
				{	//On the first pass, render trill sections
					sectionptr = eof_get_trill(eof_song, eof_selected_track, i);
				}
				else
				{	//On the second pass, render tremolo sections
					sectionptr = eof_get_tremolo(eof_song, eof_selected_track, i);
				}
				if(sectionptr != NULL)
				{	//If the section exists
					if(j)
					{	//If tremolo sections are being rendered
						if(eof_song->track[eof_selected_track]->flags & EOF_TRACK_FLAG_UNLIMITED_DIFFS)
						{	//If the track's difficulty limit has been removed
							if(sectionptr->difficulty != eof_note_type)	//And the tremolo section does not apply to the active track difficulty
								continue;	//Skip rendering it
						}
						else
						{
							if(sectionptr->difficulty != 0xFF)	//Otherwise if the tremolo section does not apply to all track difficulties
								continue;	//Skip rendering it
						}
					}
					sz = (long)(sectionptr->start_pos + eof_av_delay - eof_music_pos) / eof_zoom_3d;
					sez = (long)(sectionptr->end_pos + eof_av_delay - eof_music_pos) / eof_zoom_3d;
					spz = sz < -100 ? -100 : sz;
					spez = sez > 600 ? 600 : sez;
					if((-100 <= sez) && (600 >= sz))
					{	//If the section would render to the visible portion of the screen
						usedlanes = eof_get_used_lanes(eof_selected_track, sectionptr->start_pos, sectionptr->end_pos, eof_note_type);	//Determine which lane(s) use this phrase
						for(ctr = firstlane, bitmask = (1 << firstlane); ctr <= lastlane; ctr++, bitmask <<= 1)
						{	//For each of the usable lanes (that are allowed to have lane specific marker rendering)
							if(usedlanes & bitmask)
							{	//If this lane is used in the phrase and the lane is active
								point[0] = ocd3d_project_x(xchart[ctr] - halflanewidth - xoffset, spez);	//Offset drum lanes by drawing them one lane further left than other tracks
								point[1] = ocd3d_project_y(200, spez);
								point[2] = ocd3d_project_x(xchart[ctr] + halflanewidth - xoffset, spez);
								point[3] = point[1];
								point[4] = ocd3d_project_x(xchart[ctr] + halflanewidth - xoffset, spz);
								point[5] = ocd3d_project_y(200, spz);
								point[6] = ocd3d_project_x(xchart[ctr] - halflanewidth - xoffset, spz);
								point[7] = point[5];
								polygon(eof_window_3d->screen, 4, point, eof_colors[ctr].lightcolor);
							}
						}
					}
				}//If the section exists
			}//For each trill or tremolo section in the track
		}//For each of the two phrase types (trills and tremolos)
	}//If this track has any trill or tremolo sections

	/* draw the 'strings' */
	long obx, oby, oex, oey;

	/* draw the beat markers */
	long bz;
	float y_projection;
	char ts_text[16] = {0}, tempo_text[16] = {0};
	for(i = 0; i < eof_song->beats; i++)
	{	//For each beat
		bz = (long)(eof_song->beat[i]->pos + eof_av_delay - eof_music_pos) / eof_zoom_3d;
		if((bz >= -100) && (bz <= 600))
		{	//If the beat is visible
			y_projection = ocd3d_project_y(200, bz);
			hline(eof_window_3d->screen, ocd3d_project_x(48, bz), y_projection, ocd3d_project_x(48 + 4 * 56, bz), eof_song->beat[i]->beat_within_measure == 0 ? eof_color_white : eof_color_dark_silver);
			if(eof_song->beat[i]->contains_tempo_change || eof_song->beat[i]->contains_ts_change)
			{	//If this beat contains either a tempo or TS change
				if(eof_song->beat[i]->contains_tempo_change)
				{	//If there is a tempo change
					eof_get_tempo_text(i, tempo_text);
				}
				else
				{
					tempo_text[0] = '\0';	//Otherwise empty out this string
				}
				eof_get_ts_text(i, ts_text);
				textprintf_ex(eof_window_3d->screen, eof_font, ocd3d_project_x(48 + 4 * 56 + 4, bz), y_projection - 12, eof_color_white, -1, "%s%s", tempo_text, ts_text);	//Render the tempo and time signature to the right of the beat marker
			}
		}
		else if(bz > 600)
		{	//If this beat wasn't visible
			break;	//None of the remaining ones will be either, so stop rendering them
		}
	}//For each beat

	//Draw fret lanes
	for(i = 0; i < numlanes; i++)
	{
		obx = ocd3d_project_x(xchart[i], -100);
		oex = ocd3d_project_x(xchart[i], 600);
		oby = ocd3d_project_y(200, -100);
		oey = ocd3d_project_y(200, 600);

		line(eof_window_3d->screen, obx, oby, oex, oey, eof_color_white);
	}

	/* draw the position marker */
	y_projection = ocd3d_project_y(200, 0);
	line(eof_window_3d->screen, ocd3d_project_x(48, 0), y_projection, ocd3d_project_x(48 + 4 * 56, 0), y_projection, eof_color_green);

//	int first_note = -1;	//Used for debugging
//	int last_note = 0;		//Used for debugging
	long tr;
	/* draw the note tails and notes */
	numnotes = eof_get_track_size(eof_song, eof_selected_track);	//Get the number of notes in this legacy/pro guitar track
	for(i = numnotes; i > 0; i--)
	{	//Render 3D notes from last to first so that the earlier notes are in front
		if(eof_note_type == eof_get_note_type(eof_song, eof_selected_track, i-1))
		{
			tr = eof_note_tail_draw_3d(eof_selected_track, i-1, (eof_selection.multi[i-1] && eof_music_paused) ? 1 : (i-1) == eof_hover_note ? 2 : 0);
			(void) eof_note_draw_3d(eof_selected_track, i-1, (eof_selection.track == eof_selected_track && eof_selection.multi[i-1] && eof_music_paused) ? 1 : (i-1) == eof_hover_note ? 2 : 0);

			if(tr < 0)	//if eof_note_tail_draw_3d skipped rendering the tail because it renders before the visible area
				break;	//Stop rendering 3d notes
/*	Used for debugging
			if(tr == 0)
			{
				if(first_note < 0)
				{
					first_note = i;
				}
			}
			else if(tr < 0)
			{
				if(first_note >= 0)
				{
					last_note = i;
					break;
				}
			}
*/
		}
	}
//	allegro_message("first = %d\nlast = %d", first_note, last_note);	//Debug

	eof_render_lyric_preview(eof_window_3d->screen);

	if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
	{	//If a pro guitar/bass track is active
		unsigned long popupmessage;
		if(eof_find_effective_rs_popup_message(eof_music_pos - eof_av_delay, &popupmessage))
		{	//If there is a popup message in effect at the current position
			textout_centre_ex(eof_window_3d->screen, font, eof_song->pro_guitar_track[tracknum]->popupmessage[popupmessage].name, eof_window_3d->screen->w / 2, 4, eof_color_white, eof_color_black);
		}
	}

	rect(eof_window_3d->screen, 0, 0, eof_window_3d->w - 1, eof_window_3d->h - 1, eof_color_dark_silver);
	rect(eof_window_3d->screen, 1, 1, eof_window_3d->w - 2, eof_window_3d->h - 2, eof_color_black);
	hline(eof_window_3d->screen, 1, eof_window_3d->h - 2, eof_window_3d->w - 2, eof_color_white);
	vline(eof_window_3d->screen, eof_window_3d->w - 2, 1, eof_window_3d->h - 2, eof_color_white);
}

void eof_render(void)
{
//	eof_log("eof_render() entered");

	/* don't draw if window is out of focus */
	if(!eof_has_focus)
	{
		return;
	}
	if(eof_song_loaded)
	{	//If a project is loaded
		if(eof_window_title_dirty)
		{	//If the window title needs to be redrawn
			eof_fix_window_title();
		}
		clear_to_color(eof_screen, eof_color_light_gray);
		if(!eof_full_screen_3d && !eof_screen_zoom)
		{	//Only blit the menu bar now if neither full screen 3D view nor x2 zoom is in effect, otherwise it will be blitted later
			if((eof_count_selected_notes(NULL, 0) > 0) || ((eof_input_mode == EOF_INPUT_FEEDBACK) && (eof_seek_hover_note >= 0)))
			{	//If notes are selected, or the seek position is at a note position when Feedback input mode is in use
				blit(eof_image[EOF_IMAGE_MENU_FULL], eof_screen, 0, 0, 0, 0, eof_screen->w, eof_screen->h);
			}
			else
			{
				blit(eof_image[EOF_IMAGE_MENU_NO_NOTE], eof_screen, 0, 0, 0, 0, eof_screen->w, eof_screen->h);
			}
		}
		if(!eof_beat_stats_cached)
		{	//If the cached beat statistics are not current
			eof_process_beat_statistics(eof_song, eof_selected_track);	//Rebuild them (from the perspective of the specified track)
		}
		if(!eof_full_screen_3d)
		{	//In full screen 3D view, don't render the note window yet, it will just be overwritten by the 3D window
			eof_render_note_window();	//Otherwise render the note window first, so if the user didn't opt to display its full width, it won't draw over the 3D window
		}
		eof_render_editor_window(eof_window_editor);
		eof_render_editor_window_2();	//Render the secondary piano roll if applicable
		eof_render_3d_window();
	}
	else
	{	//If no project is loaded, just draw a blank screen and the menu
		clear_to_color(eof_screen, eof_color_gray);
		if(eof_screen_zoom)
		{	//If x2 zoom is enabled, stretch blit the menu
			stretch_blit(eof_image[EOF_IMAGE_MENU], eof_screen, 0, 0, eof_image[EOF_IMAGE_MENU]->w, eof_image[EOF_IMAGE_MENU]->h, 0, 0, eof_image[EOF_IMAGE_MENU]->w / 2, eof_image[EOF_IMAGE_MENU]->h / 2);
		}
		else
		{	//Otherwise render it normally
			blit(eof_image[EOF_IMAGE_MENU], eof_screen, 0, 0, 0, 0, eof_screen->w, eof_screen->h);
		}
	}

	if(eof_cursor_visible && eof_soft_cursor)
	{	//If rendering the software mouse cursor, do so at the actual screen coordinates
		draw_sprite(eof_screen, mouse_sprite, mouse_x - 1, mouse_y - 1);
	}

	if(eof_full_screen_3d && eof_song_loaded)
	{	//If the user enabled full screen 3D view, scale it to fill the program window
		stretch_blit(eof_window_3d->screen, eof_screen, 0, 0, eof_screen_width_default / 2, eof_screen_height / 2, 0, 0, eof_screen_width_default, eof_screen_height);
		eof_window_note->y = 0;	//Re-position the note window to the top left corner of EOF's program window
		eof_render_note_window();
		if(!eof_screen_zoom)
		{	//If x2 zoom is not enabled, render the menu now
			if((eof_count_selected_notes(NULL, 0) > 0) || ((eof_input_mode == EOF_INPUT_FEEDBACK) && (eof_seek_hover_note >= 0)))
			{	//If notes are selected, or the seek position is at a note position when Feedback input mode is in use
				blit(eof_image[EOF_IMAGE_MENU_FULL], eof_screen, 0, 0, 0, 0, eof_screen->w, eof_screen->h);
			}
			else
			{
				blit(eof_image[EOF_IMAGE_MENU_NO_NOTE], eof_screen, 0, 0, 0, 0, eof_screen->w, eof_screen->h);
			}
		}
		eof_window_note->y = eof_screen_height / 2;	//Re-position the note window to the bottom left corner of EOF's program window
	}

	if(!eof_disable_vsync)
	{	//Wait for vsync unless this was disabled
		IdleUntilVSync();
		vsync();
		DoneVSync();
	}
	if(eof_screen_zoom)
	{	//If x2 zoom is enabled, stretch blit the menu to eof_screen, and then stretch blit that to screen
		//Blitting straight to screen causes flickery menus
		//Drawing the menu half size and then stretching it to full size makes it unreadable, but that may be better than not rendering them at all
		//The highest quality (and most memory wasteful) solution would require another large bitmap to render the x2 program window and then the x1 menus on top, which would then be blit to screen
		if((eof_count_selected_notes(NULL, 0) > 0) || ((eof_input_mode == EOF_INPUT_FEEDBACK) && (eof_seek_hover_note >= 0)))
		{	//If notes are selected, or the seek position is at a note position when Feedback input mode is in use
			stretch_blit(eof_image[EOF_IMAGE_MENU_FULL], eof_screen, 0, 0, eof_image[EOF_IMAGE_MENU_FULL]->w, eof_image[EOF_IMAGE_MENU_FULL]->h, 0, 0, eof_image[EOF_IMAGE_MENU_FULL]->w / 2, eof_image[EOF_IMAGE_MENU_FULL]->h / 2);
		}
		else
		{
			stretch_blit(eof_image[EOF_IMAGE_MENU_NO_NOTE], eof_screen, 0, 0, eof_image[EOF_IMAGE_MENU_NO_NOTE]->w, eof_image[EOF_IMAGE_MENU_NO_NOTE]->h, 0, 0, eof_image[EOF_IMAGE_MENU_NO_NOTE]->w / 2, eof_image[EOF_IMAGE_MENU_NO_NOTE]->h / 2);
		}
		stretch_blit(eof_screen, screen, 0, 0, eof_screen_width, eof_screen_height, 0, 0, SCREEN_W, SCREEN_H);
	}
	else
	{
		blit(eof_screen, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);	//Render the screen normally
	}
}

static int work_around_fsel_bug = 0;
/* The AGUP d_agup_edit_proc isn't exactly compatible to Allegro's stock
 * d_edit_proc, which has no border at all. Therefore we resort to this little
 * hack.
 */
int d_hackish_edit_proc (int msg, DIALOG *d, int c)
{
	if ((msg == MSG_START) && !work_around_fsel_bug)
	{
		/* Adjust position/dimension so it is the same as AGUP's. */
		d->y -= 3;
		d->h += 6;
		/* The Allegro GUI has a bug where it repeatedely sends MSG_START to a
		 * custom GUI procedure. We need to work around that.
		 */
		work_around_fsel_bug = 1;
	}
	if ((msg == MSG_END) && work_around_fsel_bug)
	{
		d->y += 3;
		d->h -= 6;
		work_around_fsel_bug = 0;
	}
	return d_agup_edit_proc (msg, d, c);
}

int eof_load_data(void)
{
	int i;

	eof_log("eof_load_data() entered", 1);

	eof_image[EOF_IMAGE_TAB0] = load_pcx("eof.dat#tab0.pcx", eof_palette);
	eof_image[EOF_IMAGE_TAB1] = load_pcx("eof.dat#tab1.pcx", NULL);
	eof_image[EOF_IMAGE_TAB2] = load_pcx("eof.dat#tab2.pcx", NULL);
	eof_image[EOF_IMAGE_TAB3] = load_pcx("eof.dat#tab3.pcx", NULL);
	eof_image[EOF_IMAGE_TAB4] = load_pcx("eof.dat#tab4.pcx", NULL);
	eof_image[EOF_IMAGE_TABE] = load_pcx("eof.dat#tabempty.pcx", NULL);
	eof_image[EOF_IMAGE_VTAB0] = load_pcx("eof.dat#vtab0.pcx", NULL);
	eof_image[EOF_IMAGE_SCROLL_BAR] = load_pcx("eof.dat#scrollbar.pcx", NULL);
	eof_image[EOF_IMAGE_SCROLL_HANDLE] = load_pcx("eof.dat#scrollhandle.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_GREEN] = load_pcx("eof.dat#note_green.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_RED] = load_pcx("eof.dat#note_red.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_YELLOW] = load_pcx("eof.dat#note_yellow.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_BLUE] = load_pcx("eof.dat#note_blue.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_PURPLE] = load_pcx("eof.dat#note_purple.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_WHITE] = load_pcx("eof.dat#note_white.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_GREEN_HIT] = load_pcx("eof.dat#note_green_hit.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_RED_HIT] = load_pcx("eof.dat#note_red_hit.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_YELLOW_HIT] = load_pcx("eof.dat#note_yellow_hit.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_BLUE_HIT] = load_pcx("eof.dat#note_blue_hit.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_PURPLE_HIT] = load_pcx("eof.dat#note_purple_hit.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_WHITE_HIT] = load_pcx("eof.dat#note_white_hit.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_HGREEN] = load_pcx("eof.dat#note_green_hopo.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_HRED] = load_pcx("eof.dat#note_red_hopo.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_HYELLOW] = load_pcx("eof.dat#note_yellow_hopo.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_HBLUE] = load_pcx("eof.dat#note_blue_hopo.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_HPURPLE] = load_pcx("eof.dat#note_purple_hopo.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_HWHITE] = load_pcx("eof.dat#note_white_hopo.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_HGREEN_HIT] = load_pcx("eof.dat#note_green_hopo_hit.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_HRED_HIT] = load_pcx("eof.dat#note_red_hopo_hit.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_HYELLOW_HIT] = load_pcx("eof.dat#note_yellow_hopo_hit.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_HBLUE_HIT] = load_pcx("eof.dat#note_blue_hopo_hit.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_HPURPLE_HIT] = load_pcx("eof.dat#note_purple_hopo_hit.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_HWHITE_HIT] = load_pcx("eof.dat#note_white_hopo_hit.pcx", NULL);
	eof_image[EOF_IMAGE_CONTROLS_BASE] = load_pcx("eof.dat#controls0.pcx", NULL);
	eof_image[EOF_IMAGE_CONTROLS_0] = load_pcx("eof.dat#controls1.pcx", NULL);
	eof_image[EOF_IMAGE_CONTROLS_1] = load_pcx("eof.dat#controls2.pcx", NULL);
	eof_image[EOF_IMAGE_CONTROLS_2] = load_pcx("eof.dat#controls3.pcx", NULL);
	eof_image[EOF_IMAGE_CONTROLS_3] = load_pcx("eof.dat#controls4.pcx", NULL);
	eof_image[EOF_IMAGE_CONTROLS_4] = load_pcx("eof.dat#controls5.pcx", NULL);
	eof_image[EOF_IMAGE_MENU] = load_pcx("eof.dat#menu.pcx", NULL);
	eof_image[EOF_IMAGE_MENU_FULL] = load_pcx("eof.dat#menufull.pcx", NULL);
	eof_image[EOF_IMAGE_CCONTROLS_BASE] = load_pcx("eof.dat#ccontrols0.pcx", NULL);
	eof_image[EOF_IMAGE_CCONTROLS_0] = load_pcx("eof.dat#ccontrols1.pcx", NULL);
	eof_image[EOF_IMAGE_CCONTROLS_1] = load_pcx("eof.dat#ccontrols2.pcx", NULL);
	eof_image[EOF_IMAGE_CCONTROLS_2] = load_pcx("eof.dat#ccontrols3.pcx", NULL);
	eof_image[EOF_IMAGE_MENU_NO_NOTE] = load_pcx("eof.dat#menunonote.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_YELLOW_CYMBAL] = load_pcx("eof.dat#note_yellow_cymbal.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_YELLOW_CYMBAL_HIT] = load_pcx("eof.dat#note_yellow_hit_cymbal.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_BLUE_CYMBAL] = load_pcx("eof.dat#note_blue_cymbal.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_BLUE_CYMBAL_HIT] = load_pcx("eof.dat#note_blue_hit_cymbal.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_PURPLE_CYMBAL] = load_pcx("eof.dat#note_purple_cymbal.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_PURPLE_CYMBAL_HIT] = load_pcx("eof.dat#note_purple_hit_cymbal.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_WHITE_CYMBAL] = load_pcx("eof.dat#note_white_cymbal.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_WHITE_CYMBAL_HIT] = load_pcx("eof.dat#note_white_hit_cymbal.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_ORANGE] = load_pcx("eof.dat#note_orange.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_ORANGE_HIT] = load_pcx("eof.dat#note_orange_hit.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_HORANGE] = load_pcx("eof.dat#note_orange_hopo.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_HORANGE_HIT] = load_pcx("eof.dat#note_orange_hopo_hit.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_GREEN_ARROW] = load_pcx("eof.dat#note_green_arrow.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_RED_ARROW] = load_pcx("eof.dat#note_red_arrow.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_YELLOW_ARROW] = load_pcx("eof.dat#note_yellow_arrow.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_BLUE_ARROW] = load_pcx("eof.dat#note_blue_arrow.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_GREEN_ARROW_HIT] = load_pcx("eof.dat#note_green_hit_arrow.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_RED_ARROW_HIT] = load_pcx("eof.dat#note_red_hit_arrow.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_YELLOW_ARROW_HIT] = load_pcx("eof.dat#note_yellow_hit_arrow.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_BLUE_ARROW_HIT] = load_pcx("eof.dat#note_blue_hit_arrow.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_GREEN_CYMBAL] = load_pcx("eof.dat#note_green_cymbal.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_GREEN_CYMBAL_HIT] = load_pcx("eof.dat#note_green_hit_cymbal.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_ORANGE_CYMBAL] = load_pcx("eof.dat#note_orange_cymbal.pcx", NULL);
	eof_image[EOF_IMAGE_NOTE_ORANGE_CYMBAL_HIT] = load_pcx("eof.dat#note_orange_hit_cymbal.pcx", NULL);
	eof_image[EOF_IMAGE_TAB_FG] = load_pcx("eof.dat#tabfg.pcx", NULL);
	eof_image[EOF_IMAGE_TAB_BG] = load_pcx("eof.dat#tabbg.pcx", NULL);

	eof_font = load_bitmap_font("eof.dat#font_times_new_roman.pcx", NULL, NULL);
	if(!eof_font)
	{
		allegro_message("Could not load font!");
		return 0;
	}
	eof_mono_font = load_bitmap_font("eof.dat#font_courier_new.pcx", NULL, NULL);
	if(!eof_mono_font)
	{
		allegro_message("Could not load mono font!");
		return 0;
	}
	eof_symbol_font = load_bitmap_font("eof.dat#font_symbols.pcx", NULL, NULL);
	if(!eof_symbol_font)
	{
		allegro_message("Could not load symbol font!");
		return 0;
	}
	eof_image[EOF_IMAGE_LYRIC_SCRATCH] = create_bitmap(320, text_height(eof_font) - 1);
	font = eof_font;
	set_palette(eof_palette);
	set_mouse_sprite(NULL);

	for(i = 1; i <= EOF_IMAGE_NOTE_ORANGE_CYMBAL_HIT; i++)
	{
		if(!eof_image[i])
		{
			allegro_message("Could not load all images!\nFailed on %d", i);
			return 0;
		}
	}

	eof_color_black = makecol(0, 0, 0);
	eof_color_white = makecol(255, 255, 255);
	eof_color_gray = makecol(64, 64, 64);
	eof_color_gray2 = makecol(147, 147, 147);
	eof_color_gray3 = makecol(163, 163, 163);
	eof_color_light_gray = makecol(224, 224, 224);
	eof_color_red = makecol(255, 0, 0);
	eof_color_green = makecol(0, 255, 0);
	eof_color_dark_green = makecol(0, 128, 0);
	eof_color_blue = makecol(0, 0, 255);
	eof_color_dark_blue = makecol(0, 0, 96);
	eof_color_light_blue = makecol(96, 96, 255);
	eof_color_turquoise = makecol(51,166,153);	//(use (68,221,204) for light turquoise)
	eof_color_yellow = makecol(255, 255, 0);
	eof_color_purple = makecol(255, 0, 255);
	eof_color_dark_purple = makecol(128, 0, 128);
	eof_color_orange = makecol(255, 127, 0);
	eof_color_silver = makecol(192, 192, 192);
	eof_color_dark_silver = makecol(160, 160, 160);
	eof_color_cyan = makecol(0, 255, 255);
	eof_color_dark_cyan = makecol(0, 160, 160);

	gui_fg_color = agup_fg_color;
	gui_bg_color = agup_bg_color;
	gui_mg_color = agup_mg_color;
	agup_init(awin95_theme);

	return 1;
}

void eof_destroy_data(void)
{
	int i;

	eof_log("eof_destroy_data() entered", 1);

	agup_shutdown();
	for(i = 0; i < EOF_MAX_IMAGES; i++)
	{
		if(eof_image[i])
		{
			destroy_bitmap(eof_image[i]);
			eof_image[i] = NULL;
		}
	}
	destroy_font(eof_font);
	eof_font = NULL;
	destroy_font(eof_mono_font);
	eof_mono_font = NULL;
	destroy_font(eof_symbol_font);
	eof_symbol_font = NULL;
}

int eof_initialize(int argc, char * argv[])
{
	int i, eof_zoom_backup;
	char temp_filename[1024] = {0}, recovered = 0;
	time_t seconds;		//Will store the current time in seconds
	struct tm *caltime;	//Will store the current time in calendar format

	eof_log("eof_initialize() entered", 1);

	if(!argv)
	{
		return 0;
	}
	allegro_init();

	set_window_title("EOF - No Song");
	if(install_sound(DIGI_AUTODETECT, MIDI_AUTODETECT, NULL))
	{	//If Allegro failed to initialize the sound AND midi
//		allegro_message("Can't set up MIDI!  Error: %s\nAttempting to init audio only",allegro_error);
		if(install_sound(DIGI_AUTODETECT, MIDI_NONE, NULL))
		{
			allegro_message("Can't set up sound!  Error: %s",allegro_error);
			return 0;
		}
		eof_midi_initialized = 0;	//Couldn't set up MIDI
	}
	else
	{
		install_timer();	//Needed to use midi_out()
		eof_midi_initialized = 1;
	}

	if(install_keyboard())
	{
		allegro_message("Can't set up keyboard!  Error: %s",allegro_error);
		return 0;
	}
	if(install_mouse() < 0)
	{
		allegro_message("Can't set up mouse!  Error: %s",allegro_error);
		return 0;
	}
	install_joystick(JOY_TYPE_AUTODETECT);
	alogg_detect_endianess(); // make sure OGG player works for PPC

	if(!file_exists("eof.dat", 0, NULL))
	{	//If eof.dat doesn't exist in the current working directory (ie. opening a file with EOF over command line)
		get_executable_name(temp_filename, 1024);
		(void) replace_filename(temp_filename, temp_filename, "", 1024);
		if(eof_chdir(temp_filename))
		{
			allegro_message("Could not load program data!\n%s", temp_filename);
			return 1;
		}
	}

	//Start the logging system (unless the user disabled it via preferences)
	eof_start_logging();
	seconds = time(NULL);
	caltime = localtime(&seconds);
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Logging started during program initialization at %s", asctime(caltime));
	eof_log(eof_log_string, 1);
	eof_log(EOF_VERSION_STRING, 1);

	InitIdleSystem();
	show_mouse(NULL);
	eof_load_config("eof.cfg");

	/* reset songs path */
	/* remove slash from folder name so we can check if it exists */
	if((eof_songs_path[uoffset(eof_songs_path, ustrlen(eof_songs_path) - 1)] == '\\') || (eof_songs_path[uoffset(eof_songs_path, ustrlen(eof_songs_path) - 1)] == '/'))
	{	//If the path ends in a separator
		eof_songs_path[uoffset(eof_songs_path, ustrlen(eof_songs_path) - 1)] = '\0';	//Remove it
	}
	if((eof_songs_path[0] == '\0') || !file_exists(eof_songs_path, FA_RDONLY | FA_HIDDEN | FA_DIREC, NULL))
	{	//If the user-specified song folder couldn't be loaded from the config file above, or if the folder does not exist
		get_executable_name(eof_songs_path, 1024);	//Set it to EOF's program file folder
		(void) replace_filename(eof_songs_path, eof_songs_path, "", 1024);
	}
	put_backslash(eof_songs_path);	//Append a file separator if necessary

	eof_zoom_backup = eof_zoom;	//Save this because it will be over-written in eof_set_display_mode()
	if(eof_zoom_backup <= 0)
	{	//Validate eof_zoom_backup, to ensure a valid zoom level was loaded from the config file
		eof_zoom_backup = 10;
	}
	if(eof_zoom > EOF_NUM_ZOOM_LEVELS)
	{	//If the zoom level is higher than the highest preset, track this as the custom zoom level
		eof_custom_zoom_level = eof_zoom;
	}
	if(eof_desktop)
	{
		set_color_depth(desktop_color_depth() != 0 ? desktop_color_depth() : 8);
	}
	if(!eof_set_display_mode_preset_custom_width(eof_screen_layout.mode, eof_screen_width))
	{
		allegro_message("Unable to set display mode, reverting to default width!");
		if(!eof_set_display_mode_preset(eof_screen_layout.mode))
		{
			allegro_message("Unable to set display mode!");
		}
		return 0;
	}
	eof_window_note = eof_window_note_lower_left;	//By default, the info panel is at the lower left corner
	eof_menu_edit_zoom_level(eof_zoom_backup);	//Apply the zoom level loaded from the config file

	if(!eof_load_data())
	{
		return 0;
	}
	eof_init_colors();
	gui_shadow_box_proc = d_agup_shadow_box_proc;
	gui_button_proc = d_agup_button_proc;
	gui_ctext_proc = d_agup_ctext_proc;
	gui_text_list_proc = d_agup_text_list_proc;
	gui_edit_proc = d_hackish_edit_proc;

	/* create file filters */
	eof_filter_eof_files = ncdfs_filter_list_create();
	if(!eof_filter_eof_files)
	{
		allegro_message("Could not create file list filter (*.eof)!");
		return 0;
	}
	ncdfs_filter_list_add(eof_filter_eof_files, "eof", "EOF Files (*.eof)", 1);

	eof_filter_midi_files = ncdfs_filter_list_create();
	if(!eof_filter_midi_files)
	{
		allegro_message("Could not create file list filter (*.mid)!");
		return 0;
	}
	ncdfs_filter_list_add(eof_filter_midi_files, "mid;rba", "MIDI Files (*.mid, *.rba)", 1);

	eof_filter_dB_files = ncdfs_filter_list_create();
	if(!eof_filter_dB_files)
	{
		allegro_message("Could not create file list filter (*.chart)!");
		return 0;
	}
	ncdfs_filter_list_add(eof_filter_dB_files, "chart", "dB Chart Files (*.chart)", 1);

	eof_filter_gh_files = ncdfs_filter_list_create();
	if(!eof_filter_gh_files)
	{
		allegro_message("Could not create file list filter (*.*)!");
		return 0;
	}
	ncdfs_filter_list_add(eof_filter_gh_files, "*", "GH Chart Files (*.*)", 1);

	eof_filter_gp_files = ncdfs_filter_list_create();
	if(!eof_filter_gp_files)
	{
		allegro_message("Could not create file list filter (*.gp5;gp4;gp3;xml)!");
		return 0;
	}
	ncdfs_filter_list_add(eof_filter_gp_files, "gp5;gp4;gp3;xml", "Guitar Pro (*.gp?), Go PlayAlong (*.xml)", 1);

	eof_filter_text_files = ncdfs_filter_list_create();
	if(!eof_filter_text_files)
	{
		allegro_message("Could not create file list filter (*.txt)!");
		return 0;
	}
	ncdfs_filter_list_add(eof_filter_text_files, "txt", "Guitar Pro lyric text files (*.txt)", 1);

	eof_filter_rs_files = ncdfs_filter_list_create();
	if(!eof_filter_rs_files)
	{
		allegro_message("Could not create file list filter (*.xml)!");
		return 0;
	}
	ncdfs_filter_list_add(eof_filter_rs_files, "xml", "Rocksmith chart files (*.xml)", 1);

	/* check availability of MP3 conversion tools */
	if(!eof_supports_mp3)
	{
	#ifdef ALLEGRO_WINDOWS
		if((eof_system("lame -S check.wav >foo") == 0) && (eof_system("oggenc2 -h >foo") == 0))
		{
			eof_supports_mp3 = 1;
			(void) delete_file("check.wav.mp3");
			(void) delete_file("foo");
		}
		else
		{
			eof_supports_mp3 = 0;
			(void) delete_file("foo");
			allegro_message("MP3 support disabled.\n\nPlease install LAME and Vorbis Tools if you want MP3 support.");
		}
	#elif defined(ALLEGRO_MACOSX)
		if((eof_system("lame -S check.wav >foo") == 0) && (eof_system("oggenc -h >foo") == 0))
		{
			eof_supports_mp3 = 1;
			(void) delete_file("check.mp3");
			(void) delete_file("foo");
		}
		else
		{
			eof_supports_mp3 = 0;
			(void) delete_file("foo");
			allegro_message("MP3 support disabled.\n\nPlease install LAME and Vorbis Tools if you want MP3 support.");
		}
	#else
		if((eof_system("lame -S check.wav >foo") == 0) && (eof_system("oggenc -h >foo") == 0))
		{
			eof_supports_mp3 = 1;
			(void) delete_file("check.wav.mp3");
			(void) delete_file("foo");
		}
		else
		{
			eof_supports_mp3 = 0;
			(void) delete_file("foo");
			allegro_message("MP3 support disabled.\n\nPlease install LAME and Vorbis Tools if you want MP3 support.");
		}
	#endif
	}

	/* check availability of silence generating tools */
	if(!eof_supports_oggcat)
	{
		SAMPLE *silentaudio = create_silence_sample(1);	//Create 1ms worth of silence
		if(silentaudio != NULL)
		{	//If the silence was successfully created
			(void) delete_file("silence.wav");	//Delete these temp files if they exist
			(void) delete_file("silence.ogg");
			(void) delete_file("silence2.ogg");
			(void) delete_file("silence3.ogg");
			if(save_wav("silence.wav", silentaudio) && exists("silence.wav"))
			{	//If the wave file was successfully created
				#ifdef ALLEGRO_WINDOWS
				if((eof_system("oggenc2 -o silence.ogg -b 128 silence.wav") == 0) &&
				#else
				if((eof_system("oggenc -o silence.ogg -b 128 silence.wav") == 0) &&
				#endif
					exists("silence.ogg"))
				{	//If the OGG file was succesfully created
					if(eof_copy_file("silence.ogg","silence2.ogg") && exists("silence2.ogg"))
					{	//If the OGG file was successfully duplicated
						if(!eof_system("oggCat silence3.ogg silence.ogg silence2.ogg") && exists("silence3.ogg"))
						{	//If oggCat successfully concatenated the two files
							eof_supports_oggcat = 1;
							(void) delete_file("silence3.ogg");
						}
						(void) delete_file("silence2.ogg");
					}
					(void) delete_file("silence.ogg");
				}
				(void) delete_file("silence.wav");
			}
		}
	}

	/* make music filter */
	eof_filter_music_files = ncdfs_filter_list_create();
	if(!eof_filter_music_files)
	{
		allegro_message("Could not create file list filter (*.mp3, *.ogg)!");
		return 0;
	}
	if(eof_supports_mp3)
	{
		ncdfs_filter_list_add(eof_filter_music_files, "ogg;mp3", "Music Files (*.ogg, *.mp3)", 1);
	}
	else
	{
		ncdfs_filter_list_add(eof_filter_music_files, "ogg", "Music Files (*.ogg)", 1);
	}
	eof_filter_ogg_files = ncdfs_filter_list_create();
	if(!eof_filter_ogg_files)
	{
		allegro_message("Could not create file list filter (*.ogg)!");
		return 0;
	}
	ncdfs_filter_list_add(eof_filter_ogg_files, "ogg", "Music Files (*.ogg)", 1);

	eof_filter_lyrics_files = ncdfs_filter_list_create();
	if(!eof_filter_lyrics_files)
	{
		allegro_message("Could not create file list filter (*.txt, *.mid, *.rmi, *.rba, *.lrc, *.vl, *.kar, *.mp3, *.srt, *.xml)!");
		return 0;
	}
	ncdfs_filter_list_add(eof_filter_lyrics_files, "txt;mid;rmi;rba;lrc;vl;kar;mp3;srt;xml;c9c", "Lyrics (txt,mid,rmi,rba,lrc,vl,kar,mp3,srt,xml,c9c)", 1);

	eof_filter_exe_files = ncdfs_filter_list_create();
	#ifdef ALLEGRO_WINDOWS
		if(!eof_filter_exe_files)
		{
			allegro_message("Could not create file list filter (*.exe)!");
			return 0;
		}
		ncdfs_filter_list_add(eof_filter_exe_files, "exe", "Executable Files (*.exe)", 1);
	#else
		if(!eof_filter_exe_files)
		{
			allegro_message("Could not create file list filter (*.*)!");
			return 0;
		}
		ncdfs_filter_list_add(eof_filter_exe_files, "*", "Executable Files (*.*)", 1);
	#endif

	eof_undo_reset(); // restart undo system
	memset(&eof_selection, 0, sizeof(EOF_SELECTION_DATA));
	eof_selection.current = EOF_MAX_NOTES - 1;

	eof_log("\tInitializing audio", 1);
	eof_mix_init();
	if(!eof_soft_cursor)
	{
		if(show_os_cursor(2) < 0)
		{
			eof_soft_cursor = 1;
		}
	}
	gametime_init(100); // 100hz timer

	MIDIqueue=MIDIqueuetail=NULL;	//Initialize the MIDI queue as empty
	set_volume_per_voice(0);		//By default, Allegro halves the volume of each voice so that it won't clip if played fully panned to either the left or right channels.  EOF doesn't use panning, so force full volume.
	set_volume(eof_global_volume, eof_global_volume);

	/* check for a previous crash condition of EOF */
	eof_log("\tChecking for crash recovery files", 1);
	if(exists("eof.recover.on"))
	{	//If the recovery status file is present
		(void) delete_file("eof.recover.on");	//Try to delete the file
		if(!exists("eof.recover.on"))
		{	//If the file no longer exists, it is not open by another EOF instance
			if(exists("eof.recover"))
			{	//If the recovery file exists
				char *buffer = eof_buffer_file("eof.recover", 1);	//Read its contents into a NULL terminated string buffer
				char *ptr = NULL;
				if(buffer)
				{	//If the file could buffer
					(void) ustrtok(buffer, "\r\n");	//Split each line into NULL separated strings
					ptr = ustrtok(NULL, "\r\n[]");	//Get the second line (the project file path)
					if(exists(buffer) && ptr && exists(ptr))
					{	//If the recovery file contained the names of an undo file and a project file that each exist
						eof_clear_input();
						key[KEY_Y] = 0;
						key[KEY_N] = 0;
						if(alert(NULL, "Recover crashed project from last undo state?", NULL, "&Yes", "&No", 'y', 'n') == 1)
						{	//If user opts to recover from a crashed EOF instance
							eof_log("\t\tLoading last undo state", 1);
							eof_song = eof_load_song(buffer);
							if(!eof_song)
							{
								allegro_message("Unable to load last undo state. File could be corrupt!");
								return 0;
							}
							(void) ustrcpy(eof_filename, ptr);		//Set the full project path
							(void) replace_filename(eof_last_eof_path, eof_filename, "", 1024);	//Set the last loaded song path
							(void) ustrcpy(eof_loaded_song_name, get_filename(eof_filename));	//Set the project filename
							(void) replace_filename(eof_song_path, eof_filename, "", 1024);	//Set the project folder path
							(void) append_filename(temp_filename, eof_song_path, eof_song->tags->ogg[eof_selected_ogg].filename, 1024);	//Construct the full OGG path
							if(!eof_load_ogg(temp_filename, 1))	//If user does not provide audio, fail over to using silent audio
							{
								allegro_message("Failed to load OGG!");
								return 0;
							}
							eof_song_loaded = 1;
							eof_chart_length = alogg_get_length_msecs_ogg(eof_music_track);
							recovered = 1;	//Remember that a file was recovered so an undo state can be made after the call to eof_init_after_load()
						}

						(void) delete_file("eof.recover");
					}
					free(buffer);
				}
			}
			eof_recovery = pack_fopen("eof.recover.on", "w");	//Open the recovery active file for writing
		}
	}
	else
	{
		eof_recovery = pack_fopen("eof.recover.on", "w");	//Open the recovery active file for writing
	}

	/* see if we are opening a file on launch */
	for(i = 1; i < argc; i++)
	{	//For each command line argument
		if(!ustricmp(argv[i], "-debug"))
		{
			eof_debug_mode = 1;
		}
		else if(!ustricmp(argv[i], "-newidle"))
		{
			eof_new_idle_system = 1;
		}
		else if(!eof_song_loaded)
		{	//If the argument is not one of EOF's native command line parameters and no file is loaded yet
			if(!ustricmp(get_extension(argv[i]), "eof"))
			{
				/* load the specified project */
				(void) ustrcpy(eof_song_path, argv[i]);
				(void) ustrcpy(eof_filename, argv[i]);
				(void) replace_filename(eof_last_eof_path, eof_filename, "", 1024);
				(void) ustrcpy(eof_loaded_song_name, get_filename(eof_filename));
				eof_song = eof_load_song(eof_filename);
				if(!eof_song)
				{
					allegro_message("Unable to load project. File could be corrupt!");
					return 0;
				}
				(void) replace_filename(eof_song_path, eof_filename, "", 1024);

				/* check song.ini and prompt user to load any external edits */
				(void) replace_filename(temp_filename, eof_song_path, "song.ini", 1024);
				(void) eof_import_ini(eof_song, temp_filename, 1);	//Read song.ini and prompt to replace values of existing settings in the project if they are different

				/* attempt to load the OGG profile OGG */
				(void) append_filename(temp_filename, eof_song_path, eof_song->tags->ogg[eof_selected_ogg].filename, 1024);
				if(!eof_load_ogg(temp_filename, 1))	//If user does not provide audio, fail over to using silent audio
				{
					allegro_message("Failed to load OGG!");
					return 0;
				}
				eof_song_loaded = 1;
				eof_chart_length = alogg_get_length_msecs_ogg(eof_music_track);
			}
			else if(!ustricmp(get_extension(argv[i]), "mid"))
			{
				(void) ustrcpy(eof_song_path, argv[i]);
				(void) ustrcpy(eof_filename, argv[i]);
				(void) replace_filename(eof_last_eof_path, eof_filename, "", 1024);
				(void) ustrcpy(eof_loaded_song_name, get_filename(eof_filename));
				(void) replace_extension(eof_loaded_song_name, eof_loaded_song_name, "eof", 1024);
				eof_song = eof_import_midi(eof_filename);
				if(!eof_song)
				{
					allegro_message("Could not import song!");
					return 0;
				}
				else
				{	//!This will need to be updated to scan for lyric tracks and fix them
					eof_track_fixup_notes(eof_song, EOF_TRACK_VOCALS, 0);
				}
			}
			else if(!ustricmp(get_extension(argv[i]), "rba"))
			{
				(void) ustrcpy(eof_song_path, argv[i]);
				(void) ustrcpy(eof_filename, argv[i]);
				(void) replace_filename(eof_last_eof_path, eof_filename, "", 1024);
				(void) ustrcpy(eof_loaded_song_name, get_filename(eof_filename));
				(void) replace_extension(eof_loaded_song_name, eof_loaded_song_name, "eof", 1024);
				(void) replace_filename(temp_filename, eof_filename, "eof_rba_import.tmp", 1024);

				if(eof_extract_rba_midi(eof_filename,temp_filename) == 0)
				{	//If this was an RBA file and the MIDI was extracted successfully
					eof_song = eof_import_midi(temp_filename);
					(void) delete_file(temp_filename);	//Delete temporary file
				}
				if(!eof_song)
				{
					allegro_message("Could not import song!");
					return 0;
				}
				else
				{
					eof_track_fixup_notes(eof_song, EOF_TRACK_VOCALS, 0);
				}
			}
			else if(!ustricmp(get_extension(argv[i]), "chart"))
			{	//Import a Feedback chart via command line
				(void) ustrcpy(eof_song_path, argv[i]);
				(void) ustrcpy(eof_filename, argv[i]);
				(void) replace_filename(eof_last_eof_path, eof_filename, "", 1024);
				(void) ustrcpy(eof_loaded_song_name, get_filename(eof_filename));
				(void) replace_extension(eof_loaded_song_name, eof_loaded_song_name, "eof", 1024);
				eof_song = eof_import_chart(eof_filename);
				if(!eof_song)
				{
					allegro_message("Could not import song!");
					return 0;
				}
				eof_song_loaded = 1;
			}
			else if(!ustricmp(get_extension(argv[i]), "ogg") || !ustricmp(get_extension(argv[i]), "mp3"))
			{	//Launch new chart wizard via command line
				eof_new_chart(argv[i]);
			}
			else if(strcasestr_spec(argv[i], ".pak."))
			{	//Import a Guitar Hero file via command line
				(void) ustrcpy(eof_song_path, argv[i]);
				(void) ustrcpy(eof_filename, argv[i]);
				(void) replace_filename(eof_last_eof_path, eof_filename, "", 1024);
				(void) ustrcpy(eof_loaded_song_name, get_filename(eof_filename));
				(void) replace_extension(eof_loaded_song_name, eof_loaded_song_name, "eof", 1024);
				eof_song = eof_import_gh(eof_filename);
				if(!eof_song)
				{
					allegro_message("Could not import song!");
					return 0;
				}
				eof_song_loaded = 1;
			}
		}//If the argument is not one of EOF's native command line parameters and no file is loaded yet
	}//For each command line argument

	if(eof_song_loaded)
	{	//The command line load succeeded (or a project was recovered), perform some common initialization
		eof_init_after_load(0);	//Initialize variables
		eof_cursor_visible = 1;
		eof_pen_visible = 1;
		show_mouse(NULL);
		if(recovered)
		{	//If a project was recovered
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make an undo state
		}
	}

	return 1;
}

void eof_exit(void)
{
	unsigned long i;
	char fn[1024] = {0};
	time_t seconds;		//Will store the current time in seconds
	struct tm *caltime;	//Will store the current time in calendar format

	eof_log("eof_exit() entered", 1);

	StopIdleSystem();

	//Delete the undo/redo related files
	eof_save_config("eof.cfg");
	(void) snprintf(fn, sizeof(fn) - 1, "eof%03u.redo", eof_log_id);	//Get the name of this EOF instance's redo file
	(void) delete_file(fn);	//And delete it if it exists
	(void) snprintf(fn, sizeof(fn) - 1, "eof%03u.redo.ogg", eof_log_id);	//Get the name of this EOF instance's redo OGG
	(void) delete_file(fn);	//And delete it if it exists
	if(eof_undo_states_initialized > 0)
	{
		for(i = 0; i < EOF_MAX_UNDO; i++)
		{	//For each undo slot
			if(eof_undo_filename[i])
			{
				(void) delete_file(eof_undo_filename[i]);	//Delete the undo file
				(void) snprintf(fn, sizeof(fn) - 1, "%s.ogg", eof_undo_filename[i]);	//Get the filename of any associated undo OGG
				(void) delete_file(fn);	//And delete it if it exists
			}
		}
	}
	(void) delete_file("eof.autoadjust");
	eof_destroy_undo();

	//Free the file filters
	free(eof_filter_music_files);
	eof_filter_music_files = NULL;
	free(eof_filter_ogg_files);
	eof_filter_ogg_files = NULL;
	free(eof_filter_midi_files);
	eof_filter_midi_files = NULL;
	free(eof_filter_eof_files);
	eof_filter_eof_files = NULL;
	free(eof_filter_exe_files);
	eof_filter_exe_files = NULL;
	free(eof_filter_lyrics_files);
	eof_filter_lyrics_files = NULL;
	free(eof_filter_dB_files);
	eof_filter_dB_files = NULL;
	free(eof_filter_gh_files);
	eof_filter_gh_files = NULL;
	free(eof_filter_gp_files);
	eof_filter_gp_files = NULL;
	free(eof_filter_text_files);
	eof_filter_text_files = NULL;
	free(eof_filter_rs_files);
	eof_filter_rs_files = NULL;
	//Free command line storage variables (for Windows build)
	#ifdef ALLEGRO_WINDOWS
	for(i = 0; i < eof_windows_argc; i++)
	{	//For each stored command line parameter
		free(eof_windows_argv[i]);
		eof_windows_argv[i] = NULL;
	}
	free(eof_windows_argv);
	eof_windows_argv = NULL;
	#endif

	agup_shutdown();
	eof_mix_exit();		//Frees memory used by audio cues
	eof_destroy_data();	//Frees graphics and fonts from memory
	eof_destroy_ogg();	//Frees chart audio
	eof_destroy_song(eof_song);	//Frees memory used by any currently loaded chart
	eof_destroy_waveform(eof_waveform);	//Frees memory used by any currently loaded waveform data
	eof_waveform = NULL;
	eof_destroy_spectrogram(eof_spectrogram);	//Frees memory used by any currently loaded spectrogram data
	eof_spectrogram = NULL;
	eof_window_destroy(eof_window_editor);
	eof_window_destroy(eof_window_editor2);
	eof_window_destroy(eof_window_note_lower_left);
	eof_window_destroy(eof_window_note_upper_left);
	eof_window_destroy(eof_window_3d);

	//Stop the logging system
	seconds = time(NULL);
	caltime = localtime(&seconds);
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Logging stopped during program completion at %s", asctime(caltime));
	eof_log(eof_log_string, 1);
	eof_stop_logging();

	//Close the autorecover file if it is open, and delete the auto-recovery status file
	if(eof_recovery)
	{	//If this EOF instance is maintaining auto-recovery files
		(void) pack_fclose(eof_recovery);
		eof_recovery = NULL;
		(void) delete_file("eof.recover.on");
	}
}

void eof_process_midi_queue(int pos)
{	//Process the MIDI queue based on the current chart timestamp (eof_music_pos)
	unsigned char NOTE_ON_DATA[3] = {0x91,0x0,127};		//Data sequence for a Note On, channel 1, Note 0
	unsigned char NOTE_OFF_DATA[3] = {0x81,0x0,127};	//Data sequence for a Note Off, channel 1, Note 0

	struct MIDIentry *ptr = MIDIqueue;	//Points to the head of the list
	struct MIDIentry *temp = NULL;

	eof_log("eof_process_midi_queue() entered", 1);

	if(midi_driver == NULL)
		return;

	while(ptr != NULL)
	{	//Process all queue entries
		if(ptr->status == 0)
		{	//If this queued note has not been played yet
			if(pos >= ptr->startpos)	//If the current chart position is at or past this note's start position
			{
				NOTE_ON_DATA[1] = ptr->note;	//Alter the data sequence to be the appropriate note number
				midi_driver->raw_midi(NOTE_ON_DATA[0]);
				midi_driver->raw_midi(NOTE_ON_DATA[1]);
				midi_driver->raw_midi(NOTE_ON_DATA[2]);
				ptr->status = 1;			//Track that it is playing
			}
		}
		else
		{	//This queued note has already been played
			if(pos >= ptr->endpos)	//If the current chart position is at or past this note's stop position
			{
				NOTE_OFF_DATA[1] = ptr->note;	//Alter the data sequence to be the appropriate note number
				midi_driver->raw_midi(NOTE_OFF_DATA[0]);
				midi_driver->raw_midi(NOTE_OFF_DATA[1]);
				midi_driver->raw_midi(NOTE_OFF_DATA[2]);

			//Remove this link from the list
				if(ptr->prev)	//If there's a link that precedes this link
					ptr->prev->next = ptr->next;	//It now points forward to the next link (if there is one)
				else
					MIDIqueue = ptr->next;		//Update head of list

				if(ptr->next)	//If there's a link that follows this link
					ptr->next->prev = ptr->prev;	//It now points back to the previous link (if there is one)
				else
					MIDIqueuetail = ptr->prev;	//Update tail of list

				temp = ptr->next;	//Save the address of the next link (if there is one)
				free(ptr);
				ptr = temp;	//Iterate to the link that followed the deleted link
				continue;	//Restart at top of loop without iterating to the next link
			}
		}
		ptr = ptr->next;	//Point to the next link
	}
}

int eof_midi_queue_add(unsigned char note, int startpos, int endpos)
{
	struct MIDIentry *ptr = NULL;	//Stores the newly allocated link

	eof_log("eof_midi_queue_add() entered", 1);

//Validate input
	if(note > 127)
		return -1;	//return error:  Invalid MIDI note
	if(startpos >= endpos)
		return -1;	//return error:  Duration must be > 0

//Allocate and initialize MIDI queue entry
	ptr = malloc(sizeof(struct MIDIentry));
	if(ptr == NULL)
		return -1;	//return error:  Unable to allocate memory
	ptr->status = 0;
	ptr->note = note;
	ptr->startpos = startpos;
	ptr->endpos = endpos;
	ptr->next = NULL;		//When appended to the end of the list, it will never point forward to anything

//Append to list
	if(MIDIqueuetail)	//If the list isn't empty
	{
		MIDIqueuetail->next = ptr;	//Tail points forward to new link
		ptr->prev = MIDIqueuetail;	//New link points backward to tail
		MIDIqueuetail = ptr;		//Update tail
	}
	else
	{
		MIDIqueue = MIDIqueuetail = ptr;	//This new link is both the head and the tail of the list
		ptr->prev = NULL;				//New link points backward to nothing
	}

	return 0;	//return success
}

void eof_midi_queue_destroy(void)
{
	struct MIDIentry *ptr = MIDIqueue,*temp = NULL;

	eof_log("eof_midi_queue_destroy() entered", 2);

	while(ptr != NULL)	//For all links in the list
	{
		temp = ptr->next;	//Save address of next link
		free(ptr);	//Destroy this link
		ptr = temp;	//Point to next link
	}

	MIDIqueue = MIDIqueuetail = NULL;
}

void eof_all_midi_notes_off(void)
{
	eof_log("eof_all_midi_notes_off() entered", 2);

	if(midi_driver)
	{
		unsigned char ALL_NOTES_OFF[3] = {0xB1,123,0};	//Data sequence for a Control Change, controller 123, value 0 (All notes off)

		midi_driver->raw_midi(ALL_NOTES_OFF[0]);
		midi_driver->raw_midi(ALL_NOTES_OFF[1]);
		midi_driver->raw_midi(ALL_NOTES_OFF[2]);
	}
}

void eof_stop_midi(void)
{
	if(eof_midi_initialized)
	{
		eof_log("eof_stop_midi() entered", 2);

		eof_all_midi_notes_off();
		eof_midi_queue_destroy();
	}
}

void eof_init_after_load(char initaftersavestate)
{
	unsigned long tracknum, tracknum2;

	eof_log("\tInitializing after load", 1);
	eof_log("eof_init_after_load() entered", 1);

	eof_music_paused = 1;
	if((eof_selected_track <= 0) || (eof_selected_track >= eof_song->tracks))
	{	//Validate eof_selected_track, to ensure a valid track was loaded from the config file
		eof_selected_track = EOF_TRACK_GUITAR;
	}
	(void) eof_menu_track_selected_track_number(eof_selected_track, 1);
	if(eof_zoom <= 0)
	{	//Validate eof_zoom, to ensure a valid zoom level was loaded from the config file
		eof_zoom = 10;
	}
	if((eof_note_type < EOF_NOTE_SUPAEASY) || (eof_note_type >= eof_song->track[eof_selected_track]->numdiffs))
	{	//Validate eof_note_type, to ensure a valid active difficulty was loaded from the config file
		eof_note_type = EOF_NOTE_AMAZING;
	}
	eof_menu_edit_zoom_level(eof_zoom);
	eof_calculate_beats(eof_song);
	eof_truncate_chart(eof_song);	//Add or remove beat markers as necessary and update the eof_chart_length variable
	if(eof_selected_beat >= eof_song->beats)
	{	//If the selected beat is no longer valid
		eof_selected_beat = 0;	//Select the first beat
	}
	if(!initaftersavestate)
	{	//If this wasn't cleanup after an undo/redo state, reset more variables
		eof_music_pos = eof_av_delay;
		eof_changes = 0;
		eof_undo_last_type = 0;
		eof_change_count = 0;
		eof_selected_catalog_entry = 0;
		eof_display_waveform = 0;
		eof_display_spectrogram = 0;
		eof_catalog_menu[0].flags = 0;	//Hide the fret catalog by default
		eof_select_beat(0);
		eof_undo_reset();
		if(eof_song->beats > 0)
			eof_set_seek_position(eof_song->beat[0]->pos + eof_av_delay);	//Seek to the first beat marker
		eof_seek_selection_start = eof_seek_selection_end = 0;	//Clear the seek selection
	}
	tracknum = eof_song->track[EOF_TRACK_DRUM]->tracknum;
	if((eof_song->track[EOF_TRACK_DRUM]->flags & EOF_TRACK_FLAG_SIX_LANES) || (eof_song->legacy_track[tracknum]->numlanes == 6))
	{	//If the normal drum track uses 6 lanes, update the PS drum track's settings to match
		tracknum2 = eof_song->track[EOF_TRACK_DRUM_PS]->tracknum;
		eof_song->track[EOF_TRACK_DRUM_PS]->flags |= eof_song->track[EOF_TRACK_DRUM]->flags;
		eof_song->legacy_track[tracknum2]->numlanes = eof_song->legacy_track[tracknum]->numlanes;
	}
	if(((eof_song->track[EOF_TRACK_DRUM]->flags & 0xF0000000) >> 24) != 0xF0)
	{	//If the PS deal drums difficulty is defined in the high nibble of the normal drum track's flags (deprecated)
		eof_song->track[EOF_TRACK_DRUM_PS]->difficulty = (eof_song->track[EOF_TRACK_DRUM]->flags & 0xF0000000) >> 28;	//Use it to set the PS drum track's difficulty
		eof_song->track[EOF_TRACK_DRUM]->flags &= ~0xF0000000;	//Clear the high nibble of the normal drum track's flags
		eof_song->track[EOF_TRACK_DRUM]->flags |= 0xF0000000;	//Set the high nibble of the normal drum track's flags of 0xF
	}
	(void) eof_detect_difficulties(eof_song, eof_selected_track);
	eof_reset_lyric_preview_lines();
	eof_prepare_menus();
	eof_sort_notes(eof_song);
	eof_fixup_notes(eof_song);
	eof_fix_window_title();
	eof_cleanup_beat_flags(eof_song);	//Make corrections to beat statuses if necessary
	eof_sort_events(eof_song);
	eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current

	eof_log("\tInitialization after load complete", 1);
}

void eof_scale_fretboard(unsigned long numlanes)
{
	unsigned long ctr;
	float lanewidth;

	eof_log("eof_scale_fretboard() entered", 1);

	eof_screen_layout.string_space = eof_screen_layout.string_space_unscaled;

	if(!numlanes)	//If 0 was passed, find the number of lanes in the active track
		numlanes = eof_count_track_lanes(eof_song, eof_selected_track);

	lanewidth = (float)eof_screen_layout.string_space * (4.0 / (numlanes-1));	//This is the correct lane width for either 5 or 6 lanes
	if(numlanes > 5)
	{	//If the active track has more than 5 lanes, scale the spacing between the fretboard lanes
		eof_screen_layout.string_space = (double)eof_screen_layout.string_space * 5.0 / (double)numlanes;
	}

	for(ctr = 0; ctr < EOF_MAX_FRETS; ctr++)
	{	//For each fretboard lane after the first is eof_screen_layout.string_space higher than the previous lane
		eof_screen_layout.note_y[ctr] = 20.0 + ((float)ctr * lanewidth);
	}
}

long xchart[EOF_MAX_FRETS] = {0};

void eof_set_3D_lane_positions(unsigned long track)
{
//	eof_log("eof_set_3D_lane_positions() entered");

	static unsigned long numlanes = 0;	//This remembers the number of lanes handled by the previous call
	unsigned long newnumlanes = eof_count_track_lanes(eof_song, track);	//This is the number of lanes in the specified track
	unsigned long numlaneswidth = 5 - 1;	//By default, the lane width will be based on a 5 lane track
	unsigned long ctr;
	float lanewidth = 0.0;

	if(!eof_song)	//If a song isn't loaded, ie. the user changed the lefty mode option with no song loaded
		return;		//Return immediately

	if(eof_selected_track == EOF_TRACK_BASS)
	{	//Special case:  The bass track can use a sixth lane but its 3D representation still only draws 5 lanes
		newnumlanes = 5;
	}
	else if(track && (eof_song->track[track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR) && !eof_render_bass_drum_in_lane)
	{	//Special case:  The drum track renders with only 4 lanes (unless the user enabled the preference to render the bass drum in a dedicated lane)
		if(eof_five_lane_drums_enabled())
		{	//The exception is if the fifth drum lane has been enabled
			newnumlanes = 6;
			numlaneswidth = 5;	//Draw six lines on the 3D fretboard like with pro guitar
		}
		else
		{
			newnumlanes = 4;
		}
	}
	else
	{
		numlaneswidth = newnumlanes - 1;	//Otherwise, base the lane width on the number of lanes used in the track
	}

	if(track && (newnumlanes == numlanes))	//If the xchart[] array doesn't need to be updated
		return;			//Return immediately

	numlanes = newnumlanes;		//Permanently store this as the number of lanes the xchart[] array represents
	lanewidth = 56.0 * (4.0 / numlaneswidth);
	if(eof_lefty_mode)
	{
		for(ctr = 0; ctr < numlanes; ctr++)
		{	//Store the fretboard lane positions in reverse order, with respect to the number of lanes in use
			xchart[ctr] = 48 + (lanewidth * (numlanes - 1 - ctr));
		}
	}
	else
	{
		for(ctr = 0; ctr < numlanes; ctr++)
		{	//Store the fretboard lane positions in normal order
			xchart[ctr] = 48 + (lanewidth * ctr);
		}
	}
}

long ychart[EOF_MAX_FRETS] = {0};

void eof_set_2D_lane_positions(unsigned long track)
{
//	eof_log("eof_set_2D_lane_positions() entered");

	static unsigned long numlanes = 0;		//This remembers the number of lanes handled by the previous call
	unsigned long newnumlanes, newnumlanes2;
	unsigned long ctr, ctr2;

	if(!eof_song)	//If a song isn't loaded, ie. the user changed the inverted notes option with no song loaded
		return;		//Return immediately

	newnumlanes = eof_count_track_lanes(eof_song, eof_selected_track);	//Count the number of lanes in the active track
	newnumlanes2 = eof_count_track_lanes(eof_song, track);	//Count the number of lanes in that note's track
	if(newnumlanes > newnumlanes2)
	{	//Special case (ie. viewing an open bass guitar catalog entry when any other legacy track is active)
		newnumlanes = newnumlanes2;	//Use the number of lanes in the active track
	}

	if(track && (newnumlanes == numlanes))	//If the ychart[] array doesn't need to be updated
		return;			//Return immediately

	numlanes = newnumlanes;		//Permanently store this as the number of lanes the xchart[] array represents
	if(!track)
	{	//If track is 0, force a rebuild of the current track
		track = eof_selected_track;
	}
	if(eof_inverted_notes || (eof_song->track[track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
	{	//If the user selected the inverted notes option OR a pro guitar track is active (force inverted notes display)
		for(ctr = 0, ctr2 = 0; ctr < EOF_MAX_FRETS; ctr++)
		{	//Store the fretboard lane positions in reverse order, with respect to the number of lanes in use
			if(EOF_MAX_FRETS - ctr <= numlanes)
			{	//If this lane is used in the chart, store it in ychart[] in reverse order
				ychart[ctr2++] = eof_screen_layout.note_y[EOF_MAX_FRETS - 1 - ctr];
			}
		}
	}
	else
	{
		for(ctr = 0; ctr < EOF_MAX_FRETS; ctr++)
		{	//Store the fretboard lane positions in normal order
			ychart[ctr] = eof_screen_layout.note_y[ctr];
		}
	}
}

unsigned int eof_log_id = 0;
void eof_start_logging(void)
{
	char log_filename[1024] = {0};

	if((eof_log_fp == NULL) && enable_logging)
	{	//If logging isn't alredy running, and logging isn't disabled
		srand(time(NULL));	//Seed the random number generator with the current time
		eof_log_id = ((unsigned int) rand()) % 1000;	//Create a 3 digit random number to represent this EOF instance
		get_executable_name(log_filename, 1024);	//Get the path of the EOF binary that is running
		(void) replace_filename(log_filename, log_filename, "eof_log.txt", 1024);
		eof_log_fp = fopen(log_filename, "w");
		eof_log_level |= 1;	//Set the logging enabled bit

		if(eof_log_fp == NULL)
		{
			allegro_message("Error opening log file for writing");
		}
	}
}

void eof_stop_logging(void)
{
	if(eof_log_fp)
	{
		(void) fclose(eof_log_fp);
		eof_log_fp = NULL;
		eof_log_level &= ~1;	//Disable the logging enabled bit
	}
}

char eof_log_string[1024] = {0};
void eof_log(const char *text, int level)
{
	if(eof_log_fp && (eof_log_level & 1) && (eof_log_level >= level))
	{	//If the log file is open, logging is enabled and the current logging level is high enough
		fprintf(eof_log_fp, "%03u: %s\n", eof_log_id, text);	//Prefix the log text with this EOF instance's logging ID
		fflush(eof_log_fp);
	}
}

#ifdef ALLEGRO_WINDOWS
	int eof_initialize_windows(void)
	{
		int i;
		wchar_t * cl = GetCommandLineW();

		eof_windows_internal_argv = CommandLineToArgvW(cl, &eof_windows_argc);
		eof_windows_argv = malloc(eof_windows_argc * sizeof(char *));
		for(i = 0; i < eof_windows_argc; i++)
		{
			eof_windows_argv[i] = malloc(1024 * sizeof(char));
			memset(eof_windows_argv[i], 0, 1024);
			(void) uconvert((char *)eof_windows_internal_argv[i], U_UNICODE, eof_windows_argv[i], U_UTF8, 4096);
		}
		return eof_initialize(eof_windows_argc, eof_windows_argv);
	}
#endif

int main(int argc, char * argv[])
{
	int updated = 0;

	#ifdef ALLEGRO_WINDOWS
		if(!eof_initialize_windows())
		{
			return 1;
		}
	#else
		if(!eof_initialize(argc, argv))
		{
			return 1;
		}
	#endif

	eof_log("\tEntering main program loop", 1);

	while(!eof_quit)
	{
		/* frame skip mode */
		while(gametime_get_frames() - gametime_tick > 0)
		{
			eof_logic();
			updated = 0;
			++gametime_tick;
		}

		/* update and draw the screen */
		if(!updated)
		{
			eof_render();
			updated = 1;
		}

		/* update the music */
		if(!eof_music_paused)
		{
			int ret = alogg_poll_ogg(eof_music_track);
			eof_music_actual_pos = alogg_get_pos_msecs_ogg(eof_music_track);
			eof_play_queued_midi_tones();	//Played cued MIDI tones for pro guitar/bass notes
			if((ret == ALOGG_POLL_PLAYJUSTFINISHED) && !eof_rewind_at_end)
			{	//If the end of the chart has been reached during playback, and the user did not want the chart to automatically rewind
				(void) eof_menu_song_seek_end();	//Re-seek to end of the audio
				eof_music_paused = 1;
			}
			else if((ret == ALOGG_POLL_PLAYJUSTFINISHED) || (ret == ALOGG_POLL_NOTPLAYING) || (ret == ALOGG_POLL_FRAMECORRUPT) || (ret == ALOGG_POLL_INTERNALERROR) || (eof_music_actual_pos > alogg_get_length_msecs_ogg(eof_music_track)))
			{	//Otherwise if ALOGG reported a completed/error condition or if the reported position is greater than the length of the audio
				eof_music_pos = eof_music_actual_pos + eof_av_delay;
				eof_music_paused = 1;
			}
			else
			{
				eof_music_actual_pos = alogg_get_pos_msecs_ogg(eof_music_track);
				if(eof_smooth_pos)
				{
					if((eof_music_actual_pos > eof_music_pos) || eof_music_paused)
					{
						eof_music_pos = eof_music_actual_pos;
					}
				}
			}
		}

		/* update the music for playing a portion from catalog */
		else if(eof_music_catalog_playback)
		{
			int ret = alogg_poll_ogg(eof_music_track);
			if(ret == ALOGG_POLL_PLAYJUSTFINISHED)
			{
				eof_music_catalog_pos = eof_song->catalog->entry[eof_selected_catalog_entry].start_pos + eof_av_delay;
				eof_music_catalog_playback = 0;
			}
			else if((ret == ALOGG_POLL_NOTPLAYING) || (ret == ALOGG_POLL_FRAMECORRUPT) || (ret == ALOGG_POLL_INTERNALERROR))
			{
				eof_music_catalog_pos = eof_song->catalog->entry[eof_selected_catalog_entry].start_pos + eof_av_delay;
				eof_music_catalog_playback = 0;
			}
			else
			{
				int ap = alogg_get_pos_msecs_ogg(eof_music_track);
				if((ap > eof_music_catalog_pos) || !eof_music_catalog_playback)
				{
					eof_music_catalog_pos = ap;
				}
			}
		}
		else
		{
			if(eof_new_idle_system)
			{
				/* rest to save CPU */
				if(eof_has_focus)
				{
					#ifndef ALLEGRO_WINDOWS
						Idle(eof_cpu_saver);
					#else
						if(eof_disable_vsync)
						{
							Idle(eof_cpu_saver);
						}
					#endif
				}

				/* make program "sleep" until it is back in focus */
				else
				{
					Idle(500);
					gametime_reset();
				}
			}
			else
			{
				/* rest to save CPU */
				if(eof_has_focus)
				{
					rest(eof_cpu_saver);
				}

				/* make program "sleep" until it is back in focus */
				else
				{
					rest(500);
					gametime_reset();
				}
			}
		}
	}
	eof_exit();
	return 0;
}

void eof_cleanup_beat_flags(EOF_SONG *sp)
{
	unsigned long ctr;

	eof_log("eof_cleanup_beat_flags() entered", 1);

	if(sp == NULL)
		return;

	for(ctr = 0; ctr < sp->beats; ctr++)
	{	//For each beat
		sp->beat[ctr]->flags &= ~(EOF_BEAT_FLAG_EVENTS);	//Clear the events flag
	}

	for(ctr = 0; ctr < sp->text_events; ctr++)
	{	//For each text event
		if(sp->text_event[ctr]->beat >= sp->beats)
		{	//If the text event is assigned to an invalid beat number
			sp->text_event[ctr]->beat = sp->beats - 1;	//Re-assign to the last beat marker
		}
		sp->beat[sp->text_event[ctr]->beat]->flags |= EOF_BEAT_FLAG_EVENTS;	//Ensure the text event status flag is set
	}

	if(sp->beats < 1)
		return;

	sp->beat[0]->flags |= EOF_BEAT_FLAG_ANCHOR;	//The first beat marker is required to be an anchor
	for(ctr = 1; ctr < sp->beats; ctr++)
	{	//For each beat after the first
		if(sp->beat[ctr]->ppqn != sp->beat[ctr - 1]->ppqn)
		{	//If the beat has a different tempo than the previous beat
			sp->beat[ctr]->flags |= EOF_BEAT_FLAG_ANCHOR;	//Ensure the anchor status flag is set
		}
	}
}

char eof_color_green_name[] = "&Green";
char eof_color_red_name[] = "&Red";
char eof_color_yellow_name[] = "&Yellow";
char eof_color_blue_name[] = "&Blue";
char eof_color_orange_name[] = "&Orange";
char eof_color_purple_name[] = "&Purple";

char eof_note_toggle_menu_string_1[20] = "";	//These strings are used in the eof_note_toggle_menu[] array
char eof_note_toggle_menu_string_2[20] = "";
char eof_note_toggle_menu_string_3[20] = "";
char eof_note_toggle_menu_string_4[20] = "";
char eof_note_toggle_menu_string_5[20] = "";
char eof_note_toggle_menu_string_6[20] = "";
char *eof_note_toggle_menu_strings[] = {eof_note_toggle_menu_string_1, eof_note_toggle_menu_string_2, eof_note_toggle_menu_string_3, eof_note_toggle_menu_string_4, eof_note_toggle_menu_string_5, eof_note_toggle_menu_string_6};

char eof_note_clear_menu_string_1[20] = "";	//These strings are used in the eof_note_clear_menu[] array
char eof_note_clear_menu_string_2[20] = "";
char eof_note_clear_menu_string_3[20] = "";
char eof_note_clear_menu_string_4[20] = "";
char eof_note_clear_menu_string_5[20] = "";
char eof_note_clear_menu_string_6[20] = "";
char *eof_note_clear_menu_strings[] = {eof_note_clear_menu_string_1, eof_note_clear_menu_string_2, eof_note_clear_menu_string_3, eof_note_clear_menu_string_4, eof_note_clear_menu_string_5, eof_note_clear_menu_string_6};

void eof_init_colors(void)
{
	eof_log("eof_init_colors() entered", 1);

	//Init green
	eof_color_green_struct.color = eof_color_green;
	eof_color_green_struct.hit = makecol(192, 255, 192);
	eof_color_green_struct.lightcolor = makecol(170,255,170);
	eof_color_green_struct.border = eof_color_red;
	eof_color_green_struct.note3d = EOF_IMAGE_NOTE_GREEN;
	eof_color_green_struct.notehit3d = EOF_IMAGE_NOTE_GREEN_HIT;
	eof_color_green_struct.hoponote3d = EOF_IMAGE_NOTE_HGREEN;
	eof_color_green_struct.hoponotehit3d = EOF_IMAGE_NOTE_HGREEN_HIT;
	eof_color_green_struct.cymbal3d = EOF_IMAGE_NOTE_GREEN_CYMBAL;
	eof_color_green_struct.cymbalhit3d = EOF_IMAGE_NOTE_GREEN_CYMBAL_HIT;
	eof_color_green_struct.arrow3d = EOF_IMAGE_NOTE_GREEN_ARROW;
	eof_color_green_struct.arrowhit3d = EOF_IMAGE_NOTE_GREEN_ARROW_HIT;
	eof_color_green_struct.colorname = eof_color_green_name;
	//Init red
	eof_color_red_struct.color = eof_color_red;
	eof_color_red_struct.hit = makecol(255, 192, 192);
	eof_color_red_struct.lightcolor = makecol(255,156,156);
	eof_color_red_struct.border = eof_color_white;
	eof_color_red_struct.note3d = EOF_IMAGE_NOTE_RED;
	eof_color_red_struct.notehit3d = EOF_IMAGE_NOTE_RED_HIT;
	eof_color_red_struct.hoponote3d = EOF_IMAGE_NOTE_HRED;
	eof_color_red_struct.hoponotehit3d = EOF_IMAGE_NOTE_HRED_HIT;
	eof_color_red_struct.cymbal3d = EOF_IMAGE_NOTE_RED;
	eof_color_red_struct.cymbalhit3d = EOF_IMAGE_NOTE_RED_HIT;
	eof_color_red_struct.arrow3d = EOF_IMAGE_NOTE_RED_ARROW;
	eof_color_red_struct.arrowhit3d = EOF_IMAGE_NOTE_RED_ARROW_HIT;
	eof_color_red_struct.colorname = eof_color_red_name;
	//Init yellow
	eof_color_yellow_struct.color = eof_color_yellow;
	eof_color_yellow_struct.hit = makecol(255, 255, 192);
	eof_color_yellow_struct.lightcolor = makecol(255,255,224);
	eof_color_yellow_struct.border = eof_color_red;
	eof_color_yellow_struct.note3d = EOF_IMAGE_NOTE_YELLOW;
	eof_color_yellow_struct.notehit3d = EOF_IMAGE_NOTE_YELLOW_HIT;
	eof_color_yellow_struct.hoponote3d = EOF_IMAGE_NOTE_HYELLOW;
	eof_color_yellow_struct.hoponotehit3d = EOF_IMAGE_NOTE_HYELLOW_HIT;
	eof_color_yellow_struct.cymbal3d = EOF_IMAGE_NOTE_YELLOW_CYMBAL;
	eof_color_yellow_struct.cymbalhit3d = EOF_IMAGE_NOTE_YELLOW_CYMBAL_HIT;
	eof_color_yellow_struct.arrow3d = EOF_IMAGE_NOTE_YELLOW_ARROW;
	eof_color_yellow_struct.arrowhit3d = EOF_IMAGE_NOTE_YELLOW_ARROW_HIT;
	eof_color_yellow_struct.colorname = eof_color_yellow_name;
	//Init blue
	eof_color_blue_struct.color = eof_color_blue;
	eof_color_blue_struct.hit = makecol(192, 192, 255);
	eof_color_blue_struct.lightcolor = makecol(156,156,255);
	eof_color_blue_struct.border = eof_color_white;
	eof_color_blue_struct.note3d = EOF_IMAGE_NOTE_BLUE;
	eof_color_blue_struct.notehit3d = EOF_IMAGE_NOTE_BLUE_HIT;
	eof_color_blue_struct.hoponote3d = EOF_IMAGE_NOTE_HBLUE;
	eof_color_blue_struct.hoponotehit3d = EOF_IMAGE_NOTE_HBLUE_HIT;
	eof_color_blue_struct.cymbal3d = EOF_IMAGE_NOTE_BLUE_CYMBAL;
	eof_color_blue_struct.cymbalhit3d = EOF_IMAGE_NOTE_BLUE_CYMBAL_HIT;
	eof_color_blue_struct.arrow3d = EOF_IMAGE_NOTE_BLUE_ARROW;
	eof_color_blue_struct.arrowhit3d = EOF_IMAGE_NOTE_BLUE_ARROW_HIT;
	eof_color_blue_struct.colorname = eof_color_blue_name;
	//Init orange
	eof_color_orange_struct.color = eof_color_orange;
	eof_color_orange_struct.hit = makecol(255, 192, 0);
	eof_color_orange_struct.lightcolor = makecol(255,170,128);
	eof_color_orange_struct.border = eof_color_white;
	eof_color_orange_struct.note3d = EOF_IMAGE_NOTE_ORANGE;
	eof_color_orange_struct.notehit3d = EOF_IMAGE_NOTE_ORANGE_HIT;
	eof_color_orange_struct.hoponote3d = EOF_IMAGE_NOTE_HORANGE;
	eof_color_orange_struct.hoponotehit3d = EOF_IMAGE_NOTE_HORANGE_HIT;
	eof_color_orange_struct.cymbal3d = EOF_IMAGE_NOTE_ORANGE_CYMBAL;
	eof_color_orange_struct.cymbalhit3d = EOF_IMAGE_NOTE_ORANGE_CYMBAL_HIT;
	eof_color_orange_struct.arrow3d = EOF_IMAGE_NOTE_ORANGE;
	eof_color_orange_struct.arrowhit3d = EOF_IMAGE_NOTE_ORANGE_HIT;
	eof_color_orange_struct.colorname = eof_color_orange_name;
	//Init purple
	eof_color_purple_struct.color = eof_color_purple;
	eof_color_purple_struct.hit = makecol(255, 192, 255);
	eof_color_purple_struct.lightcolor = makecol(255,156,255);
	eof_color_purple_struct.border = eof_color_white;
	eof_color_purple_struct.note3d = EOF_IMAGE_NOTE_PURPLE;
	eof_color_purple_struct.notehit3d = EOF_IMAGE_NOTE_PURPLE_HIT;
	eof_color_purple_struct.hoponote3d = EOF_IMAGE_NOTE_HPURPLE;
	eof_color_purple_struct.hoponotehit3d = EOF_IMAGE_NOTE_HPURPLE_HIT;
	eof_color_purple_struct.cymbal3d = EOF_IMAGE_NOTE_PURPLE_CYMBAL;
	eof_color_purple_struct.cymbalhit3d = EOF_IMAGE_NOTE_PURPLE_CYMBAL_HIT;
	eof_color_purple_struct.arrow3d = EOF_IMAGE_NOTE_PURPLE;
	eof_color_purple_struct.arrowhit3d = EOF_IMAGE_NOTE_PURPLE_HIT;
	eof_color_purple_struct.colorname = eof_color_purple_name;
}

void eof_set_color_set(void)
{
	int x;

	eof_log("eof_set_color_set() entered", 1);

	if(!eof_song)
		return;

	if(eof_color_set == EOF_COLORS_DEFAULT)
	{	//If user is using the original EOF color set
		eof_colors[0] = eof_color_green_struct;
		eof_colors[1] = eof_color_red_struct;
		eof_colors[2] = eof_color_yellow_struct;
		eof_colors[3] = eof_color_blue_struct;
		eof_colors[4] = eof_color_purple_struct;
		eof_colors[5] = eof_color_orange_struct;
	}
	else if((eof_color_set == EOF_COLORS_RB) || (eof_color_set == EOF_COLORS_RS))
	{	//If user is using the Rock Band or Rocksmith color set
		if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
		{	//If a drum track is active
			eof_colors[0] = eof_color_orange_struct;
			eof_colors[1] = eof_color_red_struct;
			eof_colors[2] = eof_color_yellow_struct;
			eof_colors[3] = eof_color_blue_struct;
			eof_colors[4] = eof_color_green_struct;
			eof_colors[5] = eof_color_purple_struct;
		}
		else if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If a pro guitar/bass track is active
			if(eof_color_set == EOF_COLORS_RB)
			{	//If the user is using the Rock Band color set
				eof_colors[0] = eof_color_red_struct;
				eof_colors[1] = eof_color_green_struct;
				eof_colors[2] = eof_color_orange_struct;
				eof_colors[3] = eof_color_blue_struct;
				eof_colors[4] = eof_color_yellow_struct;
				eof_colors[5] = eof_color_purple_struct;
			}
			else
			{	//The user is using the Rocksmith color set
				eof_colors[0] = eof_color_red_struct;
				eof_colors[1] = eof_color_yellow_struct;
				eof_colors[2] = eof_color_blue_struct;
				eof_colors[3] = eof_color_orange_struct;
				eof_colors[4] = eof_color_green_struct;
				eof_colors[5] = eof_color_purple_struct;
			}
		}
		else
		{	//All other tracks use the generic Rock Band color set
			eof_colors[0] = eof_color_green_struct;
			eof_colors[1] = eof_color_red_struct;
			eof_colors[2] = eof_color_yellow_struct;
			eof_colors[3] = eof_color_blue_struct;
			eof_colors[4] = eof_color_orange_struct;
			eof_colors[5] = eof_color_purple_struct;
		}
	}
	else if(eof_color_set == EOF_COLORS_GH)
	{	//If user is using the Guitar Hero color set
		if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
		{	//If a drum track is active
			eof_colors[0] = eof_color_purple_struct;
			eof_colors[1] = eof_color_red_struct;
			eof_colors[2] = eof_color_yellow_struct;
			eof_colors[3] = eof_color_blue_struct;
			eof_colors[4] = eof_color_orange_struct;
			eof_colors[5] = eof_color_green_struct;
		}
		else
		{	//All other tracks use the generic Guitar Hero color set
			eof_colors[0] = eof_color_green_struct;
			eof_colors[1] = eof_color_red_struct;
			eof_colors[2] = eof_color_yellow_struct;
			eof_colors[3] = eof_color_blue_struct;
			eof_colors[4] = eof_color_orange_struct;
			eof_colors[5] = eof_color_purple_struct;
		}
	}
	//Update the strings for the Note>Toggle and Note>Clear menus
	for(x = 0; x < 6; x++)
	{
		 snprintf(eof_note_toggle_menu_strings[x], sizeof(eof_note_toggle_menu_string_1) - 1, "%s\tShift+%d", eof_colors[x].colorname, x+1);
		 strncpy(eof_note_clear_menu_strings[x], eof_colors[x].colorname, sizeof(eof_note_clear_menu_string_1) - 1);
	}

	//Update the strings for the Edit>Clap Notes menu
	for(x = 0; x < 6; x++)
	{
		eof_edit_claps_menu[x + 1].text = eof_colors[x].colorname;
	}
}

void eof_set_3d_projection(void)
{
	ocd3d_set_projection((float)eof_screen_width_default / 640.0, (float)eof_screen_height / 480.0, (float)eof_vanish_x, (float)eof_vanish_y, 320.0, 320.0);
}

END_OF_MAIN()
