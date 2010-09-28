/* HOPO note: if there is less than 170 mseconds between notes, they are created as HOPO */

#include <allegro.h>
#ifdef ALLEGRO_WINDOWS
	#include <winalleg.h>
#endif
#include <math.h>
#include <sys/stat.h>
#include "alogg/include/alogg.h"
#include "agup/agup.h"
#include "modules/wfsel.h"
#include "modules/ocd3d.h"
#include "modules/gametime.h"
#include "modules/g-idle.h"
#include "dialog/main.h"
#include "menu/main.h"
#include "menu/file.h"
#include "menu/edit.h"
#include "menu/song.h"
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
#include "window.h"
#include "note.h"
#include "beat.h"
#include "event.h"
#include "editor.h"
#include "dialog.h"
#include "undo.h"
#include "mix.h"
#include "control.h"

char      * eof_track_name[EOF_MAX_TRACKS + 1] = {"PART GUITAR", "PART BASS", "PART GUITAR COOP", "PART RHYTHM", "PART DRUMS", "PART VOCALS"};
char        eof_note_type_name[5][32] = {" Supaeasy", " Easy", " Medium", " Amazing", " BRE"};
char        eof_vocal_tab_name[5][32] = {" Lyrics", " ", " ", " ", " "};
char      * eof_snap_name[9] = {"Off", "1/4", "1/8", "1/12", "1/16", "1/24", "1/32", "1/48", "Custom"};
char      * eof_input_name[EOF_INPUT_NAME_NUM] = {"Classic", "Piano Roll", "Hold", "RexMundi", "Guitar Tap", "Guitar Strum", "Feedback"};

NCDFS_FILTER_LIST * eof_filter_music_files = NULL;
NCDFS_FILTER_LIST * eof_filter_ogg_files = NULL;
NCDFS_FILTER_LIST * eof_filter_midi_files = NULL;
NCDFS_FILTER_LIST * eof_filter_eof_files = NULL;
NCDFS_FILTER_LIST * eof_filter_exe_files = NULL;
NCDFS_FILTER_LIST * eof_filter_lyrics_files = NULL;
NCDFS_FILTER_LIST * eof_filter_dB_files = NULL;

PALETTE     eof_palette;
BITMAP *    eof_image[EOF_MAX_IMAGES] = {NULL};
FONT *      eof_font;
FONT *      eof_mono_font;

EOF_WINDOW * eof_window_editor = NULL;
EOF_WINDOW * eof_window_note = NULL;
EOF_WINDOW * eof_window_3d = NULL;

/* configuration */
char        eof_fof_executable_path[1024] = {0};
char        eof_fof_executable_name[1024] = {0};
char        eof_fof_songs_path[1024] = {0};
char        eof_last_frettist[256] = {0};
char        eof_temp_filename[1024] = {0};
char        eof_soft_cursor = 0;
char        eof_desktop = 1;
int         eof_av_delay = 200;
int         eof_buffer_size = 4096;
int         eof_audio_fine_tune = 0;
int         eof_inverted_notes = 0;
int         eof_lefty_mode = 0;
int         eof_note_auto_adjust = 1;
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
int         eof_new_idle_system = 0;
char        eof_just_played = 0;

int         eof_undo_toggle = 0;
int         eof_redo_toggle = 0;
int         eof_change_count = 0;
int         eof_zoom = 10;			//The width of one pixel in the editor window in ms (smaller = higher zoom)
int         eof_zoom_3d = 5;
char        eof_changes = 0;
ALOGG_OGG * eof_music_track = NULL;
SAMPLE    * eof_music_wave = NULL;
BITMAP    * eof_music_image = NULL;
void      * eof_music_data = NULL;
int         eof_music_data_size = 0;
int         eof_music_length = 0;
int         eof_music_actual_length = 0;
int         eof_music_pos;
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
char        eof_filename[1024] = {0};	//The full path of the EOF file that is loaded
char        eof_song_path[1024] = {0};
char        eof_songs_path[1024] = {0};
char        eof_last_ogg_path[1024] = {0};
char        eof_last_eof_path[1024] = {0};
char        eof_loaded_song_name[1024] = {0};
char        eof_loaded_ogg_name[1024] = {0};
char        eof_window_title[4096] = {0};
int         eof_quit = 0;
int         eof_note_type = EOF_NOTE_AMAZING;
int         eof_note_difficulties[5] = {0};
int         eof_note_types = 0;
int         eof_selected_track = EOF_TRACK_GUITAR;
int         eof_vocals_selected = 0;
int         eof_vocals_tab = 0;
int         eof_vocals_offset = 60; // Start at "middle C"
int         eof_song_loaded = 0;	//The boolean condition that a chart and its audio are successfully loaded
//int         eof_first_note = 0;
int         eof_last_note = 0;
int         eof_last_midi_offset = 0;

/* mouse control data */
int         eof_selected_control = -1;
int         eof_cselected_control = -1;
int         eof_selected_catalog_entry = 0;
int         eof_selected_beat = 0;
int         eof_selected_measure = 0;
int         eof_beat_in_measure = 0;
int         eof_beats_in_measure = 1;
int         eof_pegged_note = -1;
int         eof_hover_note = -1;
int         eof_hover_note_2 = -1;
int         eof_hover_beat = -1;
int         eof_hover_beat_2 = -1;
int         eof_hover_piece = -1;
int         eof_hover_key = -1;
int         eof_hover_lyric = -1;
int         eof_last_tone = -1;
int         eof_mouse_z;
int         eof_mickey_z;
int         eof_mickeys_x;
int         eof_mickeys_y;
int         eof_lclick_released = 1;
int         eof_blclick_released = 1;
int         eof_rclick_released = 1;
int         eof_click_x;
int         eof_click_y;
int         eof_peg_x;
int         eof_peg_y;
int         eof_last_pen_pos = 0;
int         eof_cursor_visible = 1;
int         eof_pen_visible = 1;
int         eof_hover_type = -1;
int         eof_mouse_drug = 0;
int         eof_mouse_drug_mickeys = 0;
int         eof_notes_moved = 0;

int         eof_menu_key_waiting = 0;

/* grid snap data */
char          eof_snap_mode = EOF_SNAP_OFF;
char          eof_last_snap_mode = EOF_SNAP_OFF;
int           eof_snap_interval = 1;
char eof_custom_snap_measure = 0;	//Boolean: The user set a custom grid snap interval defined in measures instead of beats

char          eof_hopo_view = EOF_HOPO_RF;

/* controller info */
EOF_CONTROLLER eof_guitar;
EOF_CONTROLLER eof_drums;

/* colors */
int eof_color_black;
int eof_color_white;
int eof_color_gray;
int eof_color_red;
int eof_color_green;
int eof_color_blue;
int eof_color_yellow;
int eof_color_purple;
int eof_info_color;

EOF_SCREEN_LAYOUT eof_screen_layout;
BITMAP * eof_screen = NULL;
BITMAP * eof_screen_3d = NULL;

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

struct MIDIentry *MIDIqueue=NULL;		//Linked list of queued MIDI notes
struct MIDIentry *MIDIqueuetail=NULL;	//Points to the tail of the list
char eof_midi_initialized=0;			//Specifies whether Allegro was able to set up a MIDI device
char eof_display_waveform=0;			//Specifies whether the waveform display is enabled
struct wavestruct *eof_waveform=NULL;	//Stores the waveform data

void eof_debug_message(char * text)
{
	if(eof_debug_mode)
	{
		allegro_message("%s", text);
		gametime_reset();
	}
}

void eof_show_mouse(BITMAP * bp)
{
	if(eof_soft_cursor)
	{
		show_mouse(bp);
	}
}

float eof_get_porpos(unsigned long pos)
{
	float porpos = 0.0;
	int beat = 0;
	int blength;
	unsigned long rpos;

	beat = eof_get_beat(eof_song, pos);
	if(beat < eof_song->beats - 1)
	{
		blength = eof_song->beat[beat + 1]->pos - eof_song->beat[beat]->pos;
	}
	else
	{
		blength = eof_song->beat[eof_song->beats - 1]->pos - eof_song->beat[eof_song->beats - 2]->pos;
	}
	rpos = pos - eof_song->beat[beat]->pos;
	porpos = ((float)rpos / (float)blength) * 100.0;
	return porpos;
}

unsigned long eof_put_porpos(int beat, float porpos, float offset)
{
	float fporpos = porpos + offset;
	int cbeat = beat;
	if(fporpos <= -1.0)
	{
//		allegro_message("a - %f", fporpos);
		while(fporpos < 0.0)
		{
			fporpos += 100.0;
			cbeat--;
		}
		if(cbeat >= 0)
		{
			return eof_song->beat[cbeat]->pos + ((float)eof_get_beat_length(eof_song, cbeat) * fporpos) / 100.0;
		}
		return -1;
	}
	else if(fporpos >= 100.0)
	{
//		allegro_message("b - %f", fporpos);
		while(fporpos >= 1.0)
		{
			fporpos -= 100.0;
			cbeat++;
		}
		if(cbeat < eof_song->beats)
		{
			return eof_song->beat[cbeat]->pos + ((float)eof_get_beat_length(eof_song, cbeat) * fporpos) / 100.0;
		}
		return -1;
	}
//	allegro_message("c - %f", fporpos);
	return eof_song->beat[cbeat]->pos + ((float)eof_get_beat_length(eof_song, cbeat) * fporpos) / 100.0;
}

void eof_reset_lyric_preview_lines(void)
{
	eof_preview_line[0] = 0;
	eof_preview_line[1] = 0;
	eof_preview_line_lyric[0] = 0;
	eof_preview_line_lyric[1] = 0;
	eof_preview_line_end_lyric[0] = 0;
	eof_preview_line_end_lyric[1] = 0;
}

void eof_find_lyric_preview_lines(void)
{
	int i, j;
	int current_line = -1;
	int next_line = -1;
	unsigned long dist = -1;
	int beyond = 1;
	int adj_eof_music_pos=eof_music_pos - eof_av_delay;	//The current seek position of the chart, adjusted for AV delay

	for(i = 0; i < eof_song->vocal_track->lines; i++)
	{
		if((adj_eof_music_pos >= eof_song->vocal_track->line[i].start_pos) && (adj_eof_music_pos <= eof_song->vocal_track->line[i].end_pos))
		{
			current_line = i;
			break;
		}
	}
	for(i = 0; i < eof_song->vocal_track->lines; i++)
	{
		if(adj_eof_music_pos <= eof_song->vocal_track->line[i].end_pos)
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
		for(i = 0; i < eof_song->vocal_track->lyrics; i++)
		{
			if((eof_song->vocal_track->lyric[i]->pos >= eof_song->vocal_track->line[eof_preview_line[0]].start_pos) && (eof_song->vocal_track->lyric[i]->pos <= eof_song->vocal_track->line[eof_preview_line[0]].end_pos))
			{
				eof_preview_line_lyric[0] = i;
				for(j = i; j < eof_song->vocal_track->lyrics; j++)
				{
					if(eof_song->vocal_track->lyric[j]->pos > eof_song->vocal_track->line[eof_preview_line[0]].end_pos)
					{
						break;
					}
				}
				eof_preview_line_end_lyric[0] = j;
				break;
			}
		}
		for(i = 0; i < eof_song->vocal_track->lines; i++)
		{
			if((eof_song->vocal_track->line[current_line].start_pos < eof_song->vocal_track->line[i].start_pos) && (eof_song->vocal_track->line[i].start_pos - eof_song->vocal_track->line[current_line].start_pos < dist))
			{
				next_line = i;
				dist = eof_song->vocal_track->line[i].start_pos - eof_song->vocal_track->line[current_line].start_pos;
			}
		}
		if(next_line >= 0)
		{
			eof_preview_line[1] = next_line;
			for(i = 0; i < eof_song->vocal_track->lyrics; i++)
			{
				if((eof_song->vocal_track->lyric[i]->pos >= eof_song->vocal_track->line[eof_preview_line[1]].start_pos) && (eof_song->vocal_track->lyric[i]->pos <= eof_song->vocal_track->line[eof_preview_line[1]].end_pos))
				{
					eof_preview_line_lyric[1] = i;
					for(j = i; j < eof_song->vocal_track->lyrics; j++)
					{
						if(eof_song->vocal_track->lyric[j]->pos > eof_song->vocal_track->line[eof_preview_line[1]].end_pos)
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
	if(eof_song_loaded)
	{
		if(!eof_music_paused)
		{
			eof_music_paused = 1;
			alogg_stop_ogg(eof_music_track);
		}
		else if(eof_music_catalog_playback)
		{
			eof_music_catalog_playback = 0;
			eof_music_catalog_pos = eof_song->catalog->entry[eof_selected_catalog_entry].start_pos + eof_av_delay;
			alogg_stop_ogg(eof_music_track);
		}
	}
}

void eof_switch_out_callback(void)
{
	eof_emergency_stop_music();
	key[KEY_TAB] = 0;
	eof_has_focus = 0;
}

void eof_switch_in_callback(void)
{
	key[KEY_TAB] = 0;
	eof_has_focus = 1;
	gametime_reset();
}

void eof_fix_catalog_selection(void)
{
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

int eof_set_display_mode(int mode)
{

	/* destroy windows first */
	if(eof_window_editor)
	{
		eof_window_destroy(eof_window_editor);
		eof_window_editor = NULL;
	}
	if(eof_window_note)
	{
		eof_window_destroy(eof_window_note);
		eof_window_note = NULL;
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
	if(eof_screen_3d)
	{
		destroy_bitmap(eof_screen_3d);
		eof_screen_3d = NULL;
	}

	switch(mode)
	{
		case EOF_DISPLAY_640:
		{
			if(set_gfx_mode(GFX_AUTODETECT_WINDOWED, 640, 480, 0, 0))
			{
				if(set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0))
				{
					if(set_gfx_mode(GFX_SAFE, 640, 480, 0, 0))
					{
						allegro_message("Can't set up screen!  Error: %s",allegro_error);
						return 0;
					}
					else
					{
						allegro_message("EOF is in safe mode!\nThings may not work as expected.");
					}
				}
			}
			eof_screen = create_bitmap(640, 480);
			if(!eof_screen)
			{
				return 0;
			}
/*			eof_screen_3d = create_bitmap(320, 240);
			if(!eof_screen_3d)
			{
				return 0;
			} */
			eof_window_editor = eof_window_create(0, 20, 640, 220, eof_screen);
			if(!eof_window_editor)
			{
				allegro_message("Unable to create editor window!");
				return 0;
			}
			eof_window_note = eof_window_create(0, 240, 320, 240, eof_screen);
			if(!eof_window_note)
			{
				allegro_message("Unable to create information window!");
				return 0;
			}
			eof_window_3d = eof_window_create(320, 240, 320, 240, eof_screen);
			if(!eof_window_3d)
			{
				allegro_message("Unable to create 3D preview window!");
				return 0;
			}
			eof_screen_layout.scrollbar_y = 203;
			eof_screen_layout.string_space = 20;
			eof_screen_layout.note_y[0] = 20;
			eof_screen_layout.note_y[1] = 40;
			eof_screen_layout.note_y[2] = 60;
			eof_screen_layout.note_y[3] = 80;
			eof_screen_layout.note_y[4] = 100;
			eof_screen_layout.lyric_y = 20;
			eof_screen_layout.vocal_y = 96;
			eof_screen_layout.vocal_view_size = 13;
			eof_screen_layout.vocal_tail_size = 4;
			eof_screen_layout.lyric_view_key_width = (eof_window_3d->screen->w - eof_window_3d->screen->w % 29) / 29;
			eof_screen_layout.lyric_view_key_height = eof_screen_layout.lyric_view_key_width * 4;
			eof_screen_layout.lyric_view_bkey_width = 3;
			eof_screen_layout.lyric_view_bkey_height = eof_screen_layout.lyric_view_key_width * 3;
			eof_screen_layout.note_size = 6;
			eof_screen_layout.hopo_note_size = 4;
			eof_screen_layout.anti_hopo_note_size = 8;
			eof_screen_layout.note_dot_size = 2;
			eof_screen_layout.hopo_note_dot_size = 1;
			eof_screen_layout.anti_hopo_note_dot_size = 4;
			eof_screen_layout.note_marker_size = 10;
			eof_screen_layout.note_tail_size = 3;
			eof_screen_layout.note_kick_size = 2;
			eof_screen_layout.fretboard_h = 125;
			eof_screen_layout.buffered_preview = 0;
			eof_screen_layout.controls_x = 443;
			eof_screen_layout.mode = mode;
			eof_menu_edit_zoom_10();
			ocd3d_set_projection(1.0, 1.0, 160.0, 0.0, 320.0, 320.0);
			break;
		}
		case EOF_DISPLAY_800:
		{
			if(set_gfx_mode(GFX_AUTODETECT_WINDOWED, 800, 600, 0, 0))
			{
				if(set_gfx_mode(GFX_AUTODETECT, 800, 600, 0, 0))
				{
					if(set_gfx_mode(GFX_SAFE, 800, 600, 0, 0))
					{
						allegro_message("Can't set up screen!  Error: %s",allegro_error);
						return 0;
					}
					else
					{
						allegro_message("EOF is in safe mode!\nThings may not work as expected.");
					}
				}
			}
			eof_screen = create_bitmap(800, 600);
			if(!eof_screen)
			{
				return 0;
			}
/*			eof_screen_3d = create_bitmap(320, 240);
			if(!eof_screen_3d)
			{
				return 0;
			} */
			eof_window_editor = eof_window_create(0, 20, 800, 280, eof_screen);
			if(!eof_window_editor)
			{
				allegro_message("Unable to create editor window!");
				return 0;
			}
			eof_window_note = eof_window_create(0, 300, 400, 300, eof_screen);
			if(!eof_window_note)
			{
				allegro_message("Unable to create information window!");
				return 0;
			}
			eof_window_3d = eof_window_create(400, 300, 400, 300, eof_screen);
//			eof_window_3d = eof_window_create(0, 0, 320, 240, eof_screen_3d);
			if(!eof_window_3d)
			{
				allegro_message("Unable to create 3D preview window!");
				return 0;
			}
			eof_screen_layout.scrollbar_y = 263;
			eof_screen_layout.string_space = 30;
			eof_screen_layout.note_y[0] = 20;
			eof_screen_layout.note_y[1] = 50;
			eof_screen_layout.note_y[2] = 80;
			eof_screen_layout.note_y[3] = 110;
			eof_screen_layout.note_y[4] = 140;
			eof_screen_layout.lyric_y = 20;
			eof_screen_layout.vocal_y = 128;
			eof_screen_layout.vocal_view_size = 13;
			eof_screen_layout.vocal_tail_size = 6;
			eof_screen_layout.lyric_view_key_width = eof_window_3d->screen->w / 29;
			eof_screen_layout.lyric_view_key_height = eof_screen_layout.lyric_view_key_width * 4;
			eof_screen_layout.lyric_view_bkey_width = 4;
			eof_screen_layout.lyric_view_bkey_height = eof_screen_layout.lyric_view_key_width * 3;
			eof_screen_layout.note_size = 8;
			eof_screen_layout.hopo_note_size = 5;
			eof_screen_layout.anti_hopo_note_size = 11;
			eof_screen_layout.note_dot_size = 3;
			eof_screen_layout.hopo_note_dot_size = 2;
			eof_screen_layout.anti_hopo_note_dot_size = 5;
			eof_screen_layout.note_marker_size = 12;
			eof_screen_layout.note_tail_size = 4;
			eof_screen_layout.note_kick_size = 3;
			eof_screen_layout.fretboard_h = 165;
			eof_screen_layout.buffered_preview = 0;
			eof_screen_layout.controls_x = 800 - 197;
			eof_screen_layout.mode = mode;
			eof_menu_edit_zoom_8();
			ocd3d_set_projection(800.0 / 640.0, 600.0 / 480.0, 160.0, 0.0, 320.0, 320.0);
			break;
		}
		case EOF_DISPLAY_1024:
		{
			if(set_gfx_mode(GFX_AUTODETECT_WINDOWED, 1024, 768, 0, 0))
			{
				if(set_gfx_mode(GFX_AUTODETECT, 1024, 768, 0, 0))
				{
					if(set_gfx_mode(GFX_SAFE, 1024, 768, 0, 0))
					{
						allegro_message("Can't set up screen!  Error: %s",allegro_error);
						return 0;
					}
					else
					{
						allegro_message("EOF is in safe mode!\nThings may not work as expected.");
					}
				}
			}
			eof_screen = create_bitmap(1024, 768);
			if(!eof_screen)
			{
				return 0;
			}
/*			eof_screen_3d = create_bitmap(320, 240);
			if(!eof_screen_3d)
			{
				return 0;
			} */
			eof_window_editor = eof_window_create(0, 20, 1024, 364, eof_screen);
			if(!eof_window_editor)
			{
				allegro_message("Unable to create editor window!");
				return 0;
			}
			eof_window_note = eof_window_create(0, 384, 512, 384, eof_screen);
			if(!eof_window_note)
			{
				allegro_message("Unable to create information window!");
				return 0;
			}
			eof_window_3d = eof_window_create(512, 384, 512, 384, eof_screen);
			if(!eof_window_3d)
			{
				allegro_message("Unable to create 3D preview window!");
				return 0;
			}
			eof_screen_layout.scrollbar_y = 347;
			eof_screen_layout.string_space = 48;
			eof_screen_layout.note_y[0] = 20;
			eof_screen_layout.note_y[1] = 20 + 48 * 1;
			eof_screen_layout.note_y[2] = 20 + 48 * 2;
			eof_screen_layout.note_y[3] = 20 + 48 * 3;
			eof_screen_layout.note_y[4] = 20 + 48 * 4;
			eof_screen_layout.lyric_y = 20;
			eof_screen_layout.vocal_y = 197;
			eof_screen_layout.vocal_view_size = 13;
			eof_screen_layout.vocal_tail_size = 11;
			eof_screen_layout.lyric_view_key_width = eof_window_3d->screen->w / 29;
			eof_screen_layout.lyric_view_key_height = eof_screen_layout.lyric_view_key_width * 4;
			eof_screen_layout.lyric_view_bkey_width = 5;
			eof_screen_layout.lyric_view_bkey_height = eof_screen_layout.lyric_view_key_width * 3;
			eof_screen_layout.note_size = 10;
			eof_screen_layout.hopo_note_size = 7;
			eof_screen_layout.anti_hopo_note_size = 14;
			eof_screen_layout.note_dot_size = 4;
			eof_screen_layout.hopo_note_dot_size = 3;
			eof_screen_layout.anti_hopo_note_dot_size = 7;
			eof_screen_layout.note_marker_size = 15;
			eof_screen_layout.note_tail_size = 5;
			eof_screen_layout.note_kick_size = 4;
			eof_screen_layout.fretboard_h = 20 + 48 * 4 + 25;
			eof_screen_layout.buffered_preview = 0;
			eof_screen_layout.controls_x = 1024 - 197;
			eof_screen_layout.mode = mode;
			eof_menu_edit_zoom_5();
			ocd3d_set_projection(1024.0 / 640.0, 768.0 / 480.0, 160.0, 0.0, 320.0, 320.0);
			break;
		}
	}
	set_display_switch_mode(SWITCH_BACKGROUND);
	set_display_switch_callback(SWITCH_OUT, eof_switch_out_callback);
	set_display_switch_callback(SWITCH_IN, eof_switch_in_callback);
	PrepVSyncIdle();
	return 1;
}

void eof_fix_window_title(void)
{
	if(eof_changes)
	{
		ustrcpy(eof_window_title, "*EOF - ");
	}
	else
	{
		ustrcpy(eof_window_title, "EOF - ");
	}
	if(eof_song && eof_song_loaded)
	{
		ustrcat(eof_window_title, eof_song->tags->title);
		ustrcat(eof_window_title, " (");
		if(eof_vocals_selected)
		{
			ustrcat(eof_window_title, "PART VOCALS");
		}
		else
		{
			ustrcat(eof_window_title, eof_track_name[eof_selected_track]);
		}
		ustrcat(eof_window_title, ")");
	}
	else
	{
		ustrcat(eof_window_title, "No Song");
	}
	set_window_title(eof_window_title);
}

void eof_clear_input(void)
{
	int i;

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
	if(eof_undo_add(type))
	{
		eof_change_count++;
		eof_changes = 1;
	}
	eof_redo_toggle = 0;
	eof_undo_toggle = 1;
	eof_fix_window_title();
	if(eof_change_count % 10 == 0)
	{
		replace_extension(eof_temp_filename, eof_filename, "backup.eof", 1024);
//		append_filename(eof_temp_filename, eof_song_path, "notes.backup.eof", 1024);
		if(!eof_save_song(eof_song, eof_temp_filename))
		{
			allegro_message("Undo state error!");
		}
//		allegro_message("backup saved\n%s", eof_temp_filename);
	}
}

int eof_get_previous_note(int cnote)
{
	int i;

	for(i = cnote - 1; i >= 0; i--)
	{
		if(eof_song->track[eof_selected_track]->note[i]->type == eof_song->track[eof_selected_track]->note[cnote]->type)
		{
			return i;
		}
	}
	return -1;
}

int eof_note_is_hopo(int cnote)
{
	double delta;
	float hopo_delta = eof_song->tags->eighth_note_hopo ? 250.0 : 170.0;
	int i;
	int pnote;
	int beat = -1;
	double bpm;
	double scale;

	if(eof_song->track[eof_selected_track]->note[cnote]->flags & EOF_NOTE_FLAG_NO_HOPO)
	{
		return 0;
	}
	if(eof_song->track[eof_selected_track]->note[cnote]->flags & EOF_NOTE_FLAG_F_HOPO)
	{
		return 1;
	}
	for(i = 0; i < eof_song->beats - 1; i++)
	{
		if((eof_song->beat[i]->pos <= eof_song->track[eof_selected_track]->note[cnote]->pos) && eof_song->beat[i + 1]->pos > eof_song->track[eof_selected_track]->note[cnote]->pos)
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
			delta = eof_song->track[eof_selected_track]->note[cnote]->pos - eof_song->track[eof_selected_track]->note[pnote]->pos;
			if((delta <= (hopo_delta * scale)) && (eof_note_count_colors(eof_song->track[eof_selected_track]->note[pnote]) == 1) && (eof_note_count_colors(eof_song->track[eof_selected_track]->note[cnote]) == 1) && (eof_song->track[eof_selected_track]->note[pnote]->note != eof_song->track[eof_selected_track]->note[cnote]->note))
			{
				return 1;
			}
		}
		else if(eof_hopo_view == EOF_HOPO_FOF)
		{
			delta = eof_song->track[eof_selected_track]->note[cnote]->pos - (eof_song->track[eof_selected_track]->note[pnote]->pos + eof_song->track[eof_selected_track]->note[pnote]->length);
			if((delta <= hopo_delta * scale) && !(eof_song->track[eof_selected_track]->note[pnote]->note & eof_song->track[eof_selected_track]->note[cnote]->note))
			{
				return 1;
			}
		}
//		allegro_message("bpm = %f\nscale = %f\ndelta = %f\npnote = %d(%d), cnote = %d(%d)", bpm, scale, delta, pnote, eof_song->track[eof_selected_track]->note[pnote].pos, cnote, eof_song->track[eof_selected_track]->note[cnote].pos);
	}
	return 0;
}

void eof_determine_hopos(void)
{
	int i, j;
	char sp[EOF_MAX_STAR_POWER] = {0};
	char so[EOF_MAX_STAR_POWER] = {0};

	for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
	{

		/* clear the flags */
		if(eof_song->track[eof_selected_track]->note[i]->flags & EOF_NOTE_FLAG_HOPO)
		{
			eof_song->track[eof_selected_track]->note[i]->flags ^= EOF_NOTE_FLAG_HOPO;
		}
		if(eof_song->track[eof_selected_track]->note[i]->flags & EOF_NOTE_FLAG_SP)
		{
			eof_song->track[eof_selected_track]->note[i]->flags ^= EOF_NOTE_FLAG_SP;
		}
//		eof_song->track[eof_selected_track]->note[i]->flags = (eof_song->track[eof_selected_track]->note[i]->flags & EOF_NOTE_FLAG_CRAZY);

		/* mark HOPO notes */
		switch(eof_hopo_view)
		{
			case EOF_HOPO_OFF:
			{
				break;
			}
			case EOF_HOPO_RF:
			case EOF_HOPO_FOF:
			case EOF_HOPO_MANUAL:
			{
				if(eof_song->track[eof_selected_track]->note[i]->type == eof_note_type)
				{
					if(eof_note_is_hopo(i))
					{
						eof_song->track[eof_selected_track]->note[i]->flags |= EOF_NOTE_FLAG_HOPO;
					}
				}
				break;
			}
		}

		/* mark star power notes */
		for(j = 0; j < eof_song->track[eof_selected_track]->star_power_paths; j++)
		{
			if((eof_song->track[eof_selected_track]->note[i]->pos >= eof_song->track[eof_selected_track]->star_power_path[j].start_pos) && (eof_song->track[eof_selected_track]->note[i]->pos <= eof_song->track[eof_selected_track]->star_power_path[j].end_pos))
			{
				eof_song->track[eof_selected_track]->note[i]->flags |= EOF_NOTE_FLAG_SP;
				sp[j] = 1;
			}
		}

		/* check solos */
		for(j = 0; j < eof_song->track[eof_selected_track]->solos; j++)
		{
			if((eof_song->track[eof_selected_track]->note[i]->pos >= eof_song->track[eof_selected_track]->solo[j].start_pos) && (eof_song->track[eof_selected_track]->note[i]->pos <= eof_song->track[eof_selected_track]->solo[j].end_pos))
			{
				so[j] = 1;
			}
		}
	}

	/* delete star power phrases with no notes */
	for(j = eof_song->track[eof_selected_track]->star_power_paths - 1; j >= 0; j--)
	{
		if(!sp[j])
		{
			eof_track_delete_star_power(eof_song->track[eof_selected_track], j);
		}
	}

	/* delete solos with no notes */
	for(j = eof_song->track[eof_selected_track]->solos - 1; j >= 0; j--)
	{
		if(!so[j])
		{
			eof_track_delete_solo(eof_song->track[eof_selected_track], j);
		}
	}
}

int eof_figure_difficulty(void)
{
	int i;
	int nt[5] = {0};
	int dt[5] = {-1, -1, -1, -1};
	int count = 0;

	/* check for lyrics if PART VOCALS is selected */
	if(eof_vocals_selected)
	{

		/* see if there are any pitched lyrics */
		for(i = 0; i < eof_song->vocal_track->lyrics; i++)
		{
			if(eof_song->vocal_track->lyric[i]->note != 0)
			{
				break;
			}
		}

		/* if we have pitched lyrics and line definitions, allow test */
		if((i < eof_song->vocal_track->lyrics) && (eof_song->vocal_track->lines > 0))
		{
			return 0;
		}
		return -1;
	}

	/* see which difficulties are populated with notes */
	for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
	{
		nt[(int)eof_song->track[eof_selected_track]->note[i]->type] = 1;
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

int eof_count_notes(void)
{
	return eof_song->track[eof_selected_track]->notes;
}

int eof_count_selected_notes_vocal(int * total, char v)
{
	int i;
	int count = 0;
	int last = -1;

	for(i = 0; i < eof_song->vocal_track->lyrics; i++)
	{
		if(eof_selection.track == EOF_TRACK_VOCALS)
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

/* total is used to determine the total number of notes including unselected notes */
int eof_count_selected_notes(int * total, char v)
{
	if(eof_vocals_selected)
	{
		return eof_count_selected_notes_vocal(total, v);
	}
	int i;
	int count = 0;
	int last = -1;

	for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
	{
		if(eof_song->track[eof_selected_track]->note[i]->type == eof_note_type)
		{
			if(eof_selection.track == eof_selected_track)
			{
				if(eof_selection.multi[i])
				{
					if((!v) || (v && eof_song->track[eof_selected_track]->note[i]->note))
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
	int part[6] = {0};

	if(eof_vocals_selected)
	{
		if(eof_song->vocal_track->lyrics <= 0)
		{
			return -1;
		}
	}
	else if(eof_song->track[eof_selected_track]->notes <= 0)
	{
		return -1;
	}

	part[EOF_TRACK_GUITAR] = 0;
	part[EOF_TRACK_RHYTHM] = 1;
	part[EOF_TRACK_BASS] = 2;
	part[EOF_TRACK_GUITAR_COOP] = 3;
	part[EOF_TRACK_DRUM] = 4;
	part[EOF_TRACK_VOCALS] = 5;
	return part[eof_selected_track];
}

int eof_load_ogg_quick(char * filename)
{
	int loaded = 0;

	eof_destroy_ogg();
	eof_music_data = (void *)eof_buffer_file(filename);
	eof_music_data_size = file_size_ex(filename);
	if(eof_music_data)
	{
		eof_music_track = alogg_create_ogg_from_buffer(eof_music_data, eof_music_data_size);
		if(eof_music_track)
		{
			loaded = 1;
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
	if(loaded)
	{
		eof_music_actual_length = alogg_get_length_msecs_ogg(eof_music_track);
	}
	else
	{
		if(eof_music_data)
		{
			free(eof_music_data);
		}
	}
	ustrncpy(eof_loaded_ogg_name,filename,1024);	//Store the loaded OGG filename
	eof_loaded_ogg_name[1023] = '\0';
	return loaded;
}

int eof_load_ogg(char * filename)
{
	char * returnedfn = NULL;
	char directory[1024] = {0};
	int loaded = 0;

	eof_destroy_ogg();
	eof_music_data = (void *)eof_buffer_file(filename);
	eof_music_data_size = file_size_ex(filename);
	if(eof_music_data)
	{
		eof_music_track = alogg_create_ogg_from_buffer(eof_music_data, eof_music_data_size);
		if(eof_music_track)
		{
			loaded = 1;
			if(alogg_get_wave_freq_ogg(eof_music_track) != 44100)
			{
				allegro_message("OGG sampling rate is not 44.1khz.\nSong may not play back at the\ncorrect speed in FOF.");
			}
			if(!alogg_get_wave_is_stereo_ogg(eof_music_track))
			{
				allegro_message("OGG is not stereo.\nSong may not play back\ncorrectly in FOF.");
			}
		}
		else
		{
			allegro_message("Unable to load OGG file.\n%s\nMake sure your file is a valid OGG file.", filename);
		}
	}
	else
	{
		returnedfn = ncd_file_select(0, eof_last_ogg_path, "Select Music File", eof_filter_music_files);
		eof_clear_input();
		if(returnedfn)
		{	//User selected an OGG or MP3 file, write guitar.ogg into the chart's destination folder accordingly
			replace_filename(directory, filename, "", 1024);	//Store the path of the file's parent folder
			if(!eof_mp3_to_ogg(returnedfn,directory))				//Create guitar.ogg in the folder
			{	//If the copy or conversion to create guitar.ogg succeeded
				replace_filename(returnedfn, filename, "guitar.ogg", 1024);	//guitar.ogg is the expected file
				eof_music_data = (void *)eof_buffer_file(returnedfn);
				eof_music_data_size = file_size_ex(returnedfn);
				if(eof_music_data)
				{
					eof_music_track = alogg_create_ogg_from_buffer(eof_music_data, eof_music_data_size);
					if(eof_music_track)
					{
						loaded = 1;
						if(alogg_get_wave_freq_ogg(eof_music_track) != 44100)
						{
							allegro_message("OGG sampling rate is not 44.1khz.\nSong may not play back at the\ncorrect speed in FOF.");
						}
						if(!alogg_get_wave_is_stereo_ogg(eof_music_track))
						{
							allegro_message("OGG is not stereo.\nSong may not play back\ncorrectly in FOF.");
						}
					}
					else
					{
						allegro_message("Unable to load OGG file.\n%s\nMake sure your file is a valid OGG file.", returnedfn);
					}
				}
				else
				{
					allegro_message("Unable to load OGG file!\n%s", returnedfn);
				}
			}
		}
	}
	if(loaded)
	{
		eof_music_actual_length = alogg_get_length_msecs_ogg(eof_music_track);
	}
	else
	{
		if(eof_music_data)
		{
			free(eof_music_data);
		}

		if(eof_song)
		{
			eof_destroy_song(eof_song);
			eof_song = NULL;
			eof_song_loaded = 0;
		}
	}
/*	if(eof_song && !loaded)
	{
		if(eof_music_data)
		{
			free(eof_music_data);
		}
		eof_destroy_song(eof_song);
		eof_song = NULL;
		eof_song_loaded = 0;
	}
	else if(!loaded)
	{
		if(eof_music_data)
		{
			free(eof_music_data);
		}
		eof_song_loaded = 0;
	}
	else
	{
		eof_music_actual_length = alogg_get_length_msecs_ogg(eof_music_track);
	} */

	ustrncpy(eof_loaded_ogg_name,filename,1024);	//Store the loaded OGG filename
	eof_loaded_ogg_name[1023] = '\0';
	return loaded;
}

int eof_destroy_ogg(void)
{
	int failed = 1;

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
	if(eof_music_data)
	{
		fp = pack_fopen(fn, "w");
		if(!fp)
		{
			return 0;
		}
		pack_fwrite(eof_music_data, eof_music_data_size, fp);
		pack_fclose(fp);
		return 1;
	}
	return 0;
}

int eof_notes_selected(void)
{
	int count = 0;
	int i;

	for(i = 0; i < EOF_MAX_NOTES; i++)
	{
		if(eof_selection.multi[i])
		{
			count++;
		}
	}
	return count;
}

int eof_first_selected_note(void)
{
	int i;

	for(i = 0; i < EOF_MAX_NOTES; i++)
	{
		if(eof_selection.multi[i] && (eof_song->track[eof_selected_track]->note[i]->type == eof_note_type))
		{
			return i;
		}
	}
	return -1;
}

/* read keys that are universally usable */
void eof_read_global_keys(void)
{

	/* exit program */
	if(key[KEY_ESC])
	{
		eof_menu_file_exit();
		key[KEY_ESC] = 0;
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
		eof_popup_dialog(eof_main_dialog, 0);
		eof_cursor_visible = 1;
		eof_pen_visible = 1;
		eof_show_mouse(NULL);
	}

	/* switch between window and full screen mode */
	if(KEY_EITHER_ALT && key[KEY_ENTER])
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

	/* show help */
	if(key[KEY_F1])
	{
		clear_keybuf();
		eof_menu_help_keys();
		key[KEY_ESC] = 0;
	}

	/* stuff you can only do when a chart is loaded */
	if(eof_song_loaded && eof_song)
	{

		/* undo */
		if(KEY_EITHER_CTRL && key[KEY_Z] && (eof_undo_count > 0))
		{
			eof_menu_edit_undo();
			key[KEY_Z] = 0;
			eof_reset_lyric_preview_lines();	//Rebuild the preview lines
		}

		/* redo */
		if(KEY_EITHER_CTRL && key[KEY_Y] && eof_redo_toggle)
		{
			eof_menu_edit_redo();
			key[KEY_Y] = 0;
			eof_reset_lyric_preview_lines();	//Rebuild the preview lines
		}

		/* switch between inverted and normal editor view */
		if(KEY_EITHER_CTRL && key[KEY_I])
		{
			eof_inverted_notes = 1 - eof_inverted_notes;
			key[KEY_I] = 0;
		}

		/* switch between normal and lefty 3D view */
		if(KEY_EITHER_CTRL && key[KEY_R])
		{
			eof_lefty_mode = 1 - eof_lefty_mode;
			key[KEY_R] = 0;
		}

		if(key[KEY_MINUS])
		{
			if(eof_av_delay > 0)
			{
				eof_av_delay--;
			}
			key[KEY_MINUS] = 0;
		}
		if(key[KEY_EQUALS])
		{
			eof_av_delay++;
			key[KEY_EQUALS] = 0;
		}
	}

	if(key[KEY_F4] || (KEY_EITHER_CTRL && key[KEY_N]))
	{
		clear_keybuf();
		eof_menu_file_new_wizard();
		key[KEY_F4] = 0;
	}
	if(key[KEY_F3] || (KEY_EITHER_CTRL && key[KEY_O]))
	{	//File>Load
		clear_keybuf();
		eof_menu_file_load();
		key[KEY_F3] = 0;
		key[KEY_O] = 0;
	}
	if(key[KEY_F10])
	{
		clear_keybuf();
		eof_menu_file_settings();
		key[KEY_F10] = 0;
	}
	if(key[KEY_F11])
	{
		clear_keybuf();
		eof_menu_file_preferences();
		key[KEY_F11] = 0;
	}
	if(key[KEY_F8])
	{
		clear_keybuf();
		eof_menu_file_lyrics_import();
		key[KEY_F8] = 0;
	}
	if(key[KEY_F7])
	{	//Launch Feedback chart import
		clear_keybuf();
		eof_menu_file_feedback_import();
		key[KEY_F7] = 0;
	}
	if(key[KEY_F6])
	{	//Launch Feedback chart import
		clear_keybuf();
		eof_menu_file_midi_import();
		key[KEY_F6] = 0;
	}
}

void eof_lyric_logic(void)
{
//	int kw = (eof_window_3d->screen->w - eof_window_3d->screen->w % 29) / 29;
//	int kh = kw * 4;
//	int bkh = kw * 3;
	int note[7] = {0, 2, 4, 5, 7, 9, 11};
	int bnote[7] = {1, 3, 0, 6, 8, 10, 0};
	int i, k;
	eof_hover_key = -1;

	if(eof_song == NULL)	//Do not allow lyric processing to occur if no song is loaded
		return;

	if(eof_music_paused)
	{
		if((mouse_x >= eof_window_3d->x) && (mouse_x < eof_window_3d->x + eof_window_3d->w) && (mouse_y >= eof_window_3d->y + eof_window_3d->h - eof_screen_layout.lyric_view_key_height))
		{
			/* check for black key */
			if(mouse_y < eof_window_3d->y + eof_window_3d->h - eof_screen_layout.lyric_view_key_height + eof_screen_layout.lyric_view_bkey_height)
			{
				for(i = 0; i < 28; i++)
				{
					k = i % 7;
					if((k == 0) || (k == 1) || (k == 3) || (k == 4) || (k == 5))
					{
						if((mouse_x - eof_window_3d->x >= i * eof_screen_layout.lyric_view_key_width + eof_screen_layout.lyric_view_key_width / 2 + eof_screen_layout.lyric_view_bkey_width) && (mouse_x - eof_window_3d->x <= (i + 1) * eof_screen_layout.lyric_view_key_width + eof_screen_layout.lyric_view_key_width / 2 - eof_screen_layout.lyric_view_bkey_width + 1))
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
					if((mouse_x - eof_window_3d->x >= i * eof_screen_layout.lyric_view_key_width) && (mouse_x - eof_window_3d->x < i * eof_screen_layout.lyric_view_key_width + eof_screen_layout.lyric_view_key_width - 1))
					{
						eof_hover_key = MINPITCH + (i / 7) * 12 + note[k];
						break;
					}
				}
//				eof_hover_key = (((mouse_x - eof_window_3d->x) / 11) / 7) * 12 + note[((mouse_x - eof_window_3d->x) / 11) % 7] + 36;
			}
			if(eof_hover_key >= 0)
			{
				if(mouse_b & 1)
				{
					if(KEY_EITHER_CTRL && (eof_selection.current < eof_song->vocal_track->lyrics))
					{
						if(eof_count_selected_notes(NULL, 0) == 1)
						{
							eof_prepare_undo(EOF_UNDO_TYPE_LYRIC_NOTE);
							eof_song->vocal_track->lyric[eof_selection.current]->note = eof_hover_key;
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
				if((mouse_b & 2) || key[KEY_INSERT])
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
			if(eof_selection.current < eof_song->vocal_track->lyrics)
			{
				eof_hover_key = eof_song->vocal_track->lyric[eof_selection.current]->note;
			}
		}
	}
	else
	{
		if(eof_hover_note >= 0)
		{
			eof_hover_key = eof_song->vocal_track->lyric[eof_hover_note]->note;
		}
	}
}

void eof_note_logic(void)
{
	if((eof_catalog_menu[0].flags & D_SELECTED) && (mouse_x >= 0) && (mouse_x < 90) && (mouse_y > 40 + eof_window_note->y) && (mouse_y < 40 + 18 + eof_window_note->y))
	{
		eof_cselected_control = mouse_x / 30;
		if(mouse_b & 1)
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
					eof_menu_catalog_previous();
				}
				else if(eof_cselected_control == 1)
				{
					eof_catalog_play();
				}
				else if(eof_cselected_control == 2)
				{
					eof_menu_catalog_next();
				}
			}
		}
	}
	else
	{
		eof_cselected_control = -1;
	}
	if((mouse_y >= eof_window_note->y) && (mouse_y < eof_window_note->y + 12) && (mouse_x >= 0) && (mouse_x < ((eof_catalog_menu[0].flags & D_SELECTED) ? text_length(font, "Fret Catalog") : text_length(font, "Information Panel"))))
	{
		if(mouse_b & 1)
		{
			eof_blclick_released = 0;
		}
		if(!mouse_b & 1)
		{
			if(!eof_blclick_released)
			{
				eof_blclick_released = 1;
				eof_menu_catalog_show();
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
	eof_read_global_keys();

	/* see if we need to activate the menu */
	if((mouse_y < eof_image[EOF_IMAGE_MENU]->h) && (mouse_b & 1))
	{
		eof_cursor_visible = 0;
		eof_emergency_stop_music();
		eof_render();
		clear_keybuf();
		eof_popup_dialog(eof_main_dialog, 0);
		eof_cursor_visible = 1;
		eof_pen_visible = 1;
		eof_show_mouse(NULL);
	}

	eof_last_note = EOF_MAX_NOTES;

	get_mouse_mickeys(&eof_mickeys_x, &eof_mickeys_y);
	if(eof_song_loaded && (eof_input_mode != EOF_INPUT_FEEDBACK))
	{
		eof_read_editor_keys();
	}
	if(eof_music_catalog_playback)
	{
		eof_music_catalog_pos += 10;
		if(eof_music_catalog_pos - eof_av_delay > eof_song->catalog->entry[eof_selected_catalog_entry].end_pos)
		{
			eof_music_catalog_playback = 0;
			eof_music_catalog_pos = eof_song->catalog->entry[eof_selected_catalog_entry].start_pos + eof_av_delay;
			alogg_stop_ogg(eof_music_track);
			alogg_seek_abs_msecs_ogg(eof_music_track, eof_music_pos);
		}
	}
	else if(!eof_music_paused)
	{
		if(eof_smooth_pos)
		{
			if(eof_playback_speed != eof_mix_speed)
			{
				if(eof_mix_speed == 1000)		//Force full speed playback
					eof_music_pos += 10;
				else if(eof_mix_speed == 500)	//Force half speed playback
					eof_music_pos += 5;
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
						{
							eof_music_pos += 7;
						}
						else
						{
							eof_music_pos += 8;
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
						{
							eof_music_pos += 3;
						}
						else
						{
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
			alogg_stop_ogg(eof_music_track);
			alogg_seek_abs_msecs_ogg(eof_music_track, eof_music_pos);
		}
	}
	if(eof_input_mode == EOF_INPUT_FEEDBACK)
	{
		if(eof_song_loaded)
		{
//			eof_editor_logic_feedback();
		}
	}
	else
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
	char * note_name[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
	char * note_name_flats[12] = {"C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"};
	char ** array_to_use=note_name;	//Default to using the regular note name array

	if(eof_display_flats)				//If user enabled the feature to display flat notes
		array_to_use=note_name_flats;	//Use the other array

	sprintf(eof_tone_name_buffer, "%s%d", array_to_use[tone % 12], tone / 12 - 1);
	return eof_tone_name_buffer;
}

void eof_render_note_window(void)
{
	int i;
	int pos;
	int lpos, npos, ypos;

	clear_to_color(eof_window_note->screen, makecol(64, 64, 64));

	if(eof_catalog_menu[0].flags & D_SELECTED)
	{
		textprintf_ex(eof_window_note->screen, font, 2, 0, eof_info_color, -1, "Fret Catalog");
		textprintf_ex(eof_window_note->screen, font, 2, 12, makecol(255, 255, 255), -1, "-------------------");
		textprintf_ex(eof_window_note->screen, font, 2, 24,  makecol(255, 255, 255), -1, "Entry: %d of %d", eof_song->catalog->entries ? eof_selected_catalog_entry + 1 : 0, eof_song->catalog->entries);

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
			if(eof_song->catalog->entry[eof_selected_catalog_entry].track == EOF_TRACK_VOCALS)
			{
				/* draw the starting position */
				pos = eof_music_catalog_pos / eof_zoom;
				if(pos < 140)
				{
					lpos = 20;
				}
				else
				{
					lpos = 20 - (pos - 140);
				}

				/* fill in window background color */
//				rectfill(eof_window_note->screen, 0, 25 + 8, eof_window_editor->w - 1, eof_window_editor->h - 1, eof_color_gray);

				/* draw fretboard area */
				rectfill(eof_window_note->screen, 0, EOF_EDITOR_RENDER_OFFSET + 25, eof_window_editor->w - 1, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 1, eof_color_black);

				for(i = 0; i < 5; i += 4)
				{
					hline(eof_window_note->screen, lpos, EOF_EDITOR_RENDER_OFFSET + 35 + i * eof_screen_layout.string_space, lpos + (eof_music_length) / eof_zoom, eof_color_white);
				}
				vline(eof_window_note->screen, lpos + (eof_music_length) / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 35, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 11, eof_color_white);

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
					vline(eof_window_note->screen, npos + eof_song->beat[i]->pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 35, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 10, eof_color_white);
				}

				/* clear lyric text area */
				rectfill(eof_window_note->screen, 0, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.lyric_y + 1, eof_window_editor->w - 1, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.lyric_y + 1 + 16, eof_color_black);
				hline(eof_window_note->screen, lpos, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.lyric_y + 1 + 16, lpos + (eof_music_length) / eof_zoom, eof_color_white);

				for(i = 0; i < eof_song->vocal_track->lyrics; i++)
				{
					if((eof_song->vocal_track->lyric[i]->pos >= eof_song->catalog->entry[eof_selected_catalog_entry].start_pos) && (eof_song->vocal_track->lyric[i]->pos <= eof_song->catalog->entry[eof_selected_catalog_entry].end_pos))
					{
						eof_lyric_draw_catalog(eof_song->vocal_track->lyric[i], i == eof_hover_note_2 ? 2 : 0);
					}
				}

				/* draw the current position */
				if(pos > eof_av_delay / eof_zoom)
				{
					if(pos < 140)
					{
						vline(eof_window_note->screen, 20 + pos - eof_av_delay / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 25, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 1, eof_color_green);
					}
					else
					{
						vline(eof_window_note->screen, 20 + 140 - eof_av_delay / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 25, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 1, eof_color_green);
					}
				}
			}
			else
			{
				/* draw the starting position */
				pos = eof_music_catalog_pos / eof_zoom;
				if(pos < 140)
				{
					lpos = 20;
				}
				else
				{
					lpos = 20 - (pos - 140);
				}

				rectfill(eof_window_note->screen, 0, EOF_EDITOR_RENDER_OFFSET + 25, eof_window_note->w - 1, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 1, eof_color_black);
				for(i = 0; i < 5; i++)
				{
					hline(eof_window_note->screen, lpos, EOF_EDITOR_RENDER_OFFSET + 35 + i * eof_screen_layout.string_space, lpos + (eof_music_length) / eof_zoom, eof_color_white);
				}
				vline(eof_window_note->screen, lpos + (eof_music_length) / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 35, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 11, eof_color_white);

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
					vline(eof_window_note->screen, npos + eof_song->beat[i]->pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 35, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 10, eof_color_white);
				}

				for(i = 0; i < eof_song->track[(int)eof_song->catalog->entry[eof_selected_catalog_entry].track]->notes; i++)
				{
					if((eof_song->catalog->entry[eof_selected_catalog_entry].type == eof_song->track[(int)eof_song->catalog->entry[eof_selected_catalog_entry].track]->note[i]->type) && (eof_song->track[(int)eof_song->catalog->entry[eof_selected_catalog_entry].track]->note[i]->pos >= eof_song->catalog->entry[eof_selected_catalog_entry].start_pos) && (eof_song->track[(int)eof_song->catalog->entry[eof_selected_catalog_entry].track]->note[i]->pos <= eof_song->catalog->entry[eof_selected_catalog_entry].end_pos))
					{
						eof_note_draw_catalog(eof_song->track[(int)eof_song->catalog->entry[eof_selected_catalog_entry].track]->note[i], i == eof_hover_note_2 ? 2 : 0);
					}
				}

				/* draw the current position */
				if(pos > eof_av_delay / eof_zoom)
				{
					if(pos < 140)
					{
						vline(eof_window_note->screen, 20 + pos - eof_av_delay / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 25, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 1, eof_color_green);
					}
					else
					{
						vline(eof_window_note->screen, 20 + 140 - eof_av_delay / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 25, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 1, eof_color_green);
					}
				}
			}
		}
	}
	else
	{
		textprintf_ex(eof_window_note->screen, font, 2, 0, eof_info_color, -1, "Information Panel");
		textprintf_ex(eof_window_note->screen, font, 2, 12, makecol(255, 255, 255), -1, "----------------------------");
		ypos = 24;
		if(eof_hover_beat >= 0)
		{
			textprintf_ex(eof_window_note->screen, font, 2, ypos, makecol(255, 255, 255), -1, "Beat = %d : BPM = %f : Hover = %d", eof_selected_beat, (double)60000000.0 / (double)eof_song->beat[eof_selected_beat]->ppqn, eof_hover_beat);
		}
		else
		{
			textprintf_ex(eof_window_note->screen, font, 2, ypos, makecol(255, 255, 255), -1, "Beat = %d : BPM = %f", eof_selected_beat, (double)60000000.0 / (double)eof_song->beat[eof_selected_beat]->ppqn);
		}
		ypos += 12;
		textprintf_ex(eof_window_note->screen, font, 2, ypos, makecol(255, 255, 255), -1, "Measure = %d (Beat %d/%d)", eof_selected_measure, eof_beat_in_measure + 1, eof_beats_in_measure);
		ypos += 12;
		if(eof_vocals_selected)
		{
			if(eof_selection.current < eof_song->vocal_track->lyrics)
			{
				textprintf_ex(eof_window_note->screen, font, 2, ypos, makecol(255, 255, 255), -1, "Lyric = %d : Pos = %lu : Length = %lu", eof_selection.current, eof_song->vocal_track->lyric[eof_selection.current]->pos, eof_song->vocal_track->lyric[eof_selection.current]->length);
				ypos += 12;
				textprintf_ex(eof_window_note->screen, font, 2, ypos, makecol(255, 255, 255), -1, "Lyric Text = \"%s\" : Tone = %d (%s)", eof_song->vocal_track->lyric[eof_selection.current]->text, eof_song->vocal_track->lyric[eof_selection.current]->note, (eof_song->vocal_track->lyric[eof_selection.current]->note != 0) ? eof_get_tone_name(eof_song->vocal_track->lyric[eof_selection.current]->note) : "none");
			}
			else
			{
				textprintf_ex(eof_window_note->screen, font, 2, ypos, makecol(255, 255, 255), -1, "Lyric = None");
			}
			ypos += 12;
			if(eof_hover_note >= 0)
			{
				textprintf_ex(eof_window_note->screen, font, 2, ypos, makecol(255, 255, 255), -1, "Hover Lyric = %d", eof_hover_note);
			}
			else
			{
				textprintf_ex(eof_window_note->screen, font, 2, ypos, makecol(255, 255, 255), -1, "Hover Lyric = None");
			}
		}
		else
		{
			if(eof_selection.current < eof_song->track[eof_selected_track]->notes)
			{
				textprintf_ex(eof_window_note->screen, font, 2, 48, makecol(255, 255, 255), -1, "Note = %d : Pos = %lu : Length = %lu", eof_selection.current, eof_song->track[eof_selected_track]->note[eof_selection.current]->pos, eof_song->track[eof_selected_track]->note[eof_selection.current]->length);
			}
			else
			{
				textprintf_ex(eof_window_note->screen, font, 2, 48, makecol(255, 255, 255), -1, "Note = None");
			}
			ypos += 12;
			if(eof_hover_note >= 0)
			{
				textprintf_ex(eof_window_note->screen, font, 2, 60, makecol(255, 255, 255), -1, "Hover Note = %d", eof_hover_note);
			}
			else
			{
				textprintf_ex(eof_window_note->screen, font, 2, 60, makecol(255, 255, 255), -1, "Hover Note = None");
			}
		}
		int ism = ((eof_music_pos - eof_av_delay) / 1000) / 60;
		int iss = ((eof_music_pos - eof_av_delay) / 1000) % 60;
		int isms = ((eof_music_pos - eof_av_delay) % 1000);
		int itn = 0;
		int isn = eof_count_selected_notes(&itn, 0);
		ypos += 12;
		textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "Seek Position = %02d:%02d:%03d", ism, iss, isms >= 0 ? isms : 0);
		ypos += 12;
		textprintf_ex(eof_window_note->screen, font, 2, ypos, eof_color_white, -1, "%s Selected = %d/%d", eof_vocals_selected ? "Lyrics" : "Notes", isn, itn);

		ypos += 24;
		textprintf_ex(eof_window_note->screen, font, 2, ypos,  makecol(255, 255, 255), -1, "Input Mode: %s", eof_input_name[eof_input_mode]);
		ypos += 12;

		if(eof_snap_mode != EOF_SNAP_CUSTOM)
			textprintf_ex(eof_window_note->screen, font, 2, ypos,  makecol(255, 255, 255), -1, "Grid Snap: %s", eof_snap_name[(int)eof_snap_mode]);
		else
		{
			if(eof_custom_snap_measure == 0)
				textprintf_ex(eof_window_note->screen, font, 2, ypos,  makecol(255, 255, 255), -1, "Grid Snap: %s (1/%d beat)", eof_snap_name[(int)eof_snap_mode],eof_snap_interval);
			else
				textprintf_ex(eof_window_note->screen, font, 2, ypos,  makecol(255, 255, 255), -1, "Grid Snap: %s (1/%d measure)", eof_snap_name[(int)eof_snap_mode],eof_snap_interval);
		}

		ypos += 12;
		textprintf_ex(eof_window_note->screen, font, 2, ypos,  makecol(255, 255, 255), -1, "Metronome: %s", eof_mix_metronome_enabled ? "On" : "Off");
		ypos += 12;
		textprintf_ex(eof_window_note->screen, font, 2, ypos,  makecol(255, 255, 255), -1, "Claps: %s", eof_mix_claps_enabled ? "On" : "Off");
		ypos += 12;
		textprintf_ex(eof_window_note->screen, font, 2, ypos,  makecol(255, 255, 255), -1, "Vocal Tones: %s", eof_mix_vocal_tones_enabled ? "On" : "Off");
		ypos += 12;
		textprintf_ex(eof_window_note->screen, font, 2, ypos,  makecol(255, 255, 255), -1, "Playback Speed: %d%%", eof_playback_speed / 10);
		ypos += 12;
		textprintf_ex(eof_window_note->screen, font, 2, ypos,  makecol(255, 255, 255), -1, "Catalog: %d of %d", eof_song->catalog->entries ? eof_selected_catalog_entry + 1 : 0, eof_song->catalog->entries);
		ypos += 12;
		textprintf_ex(eof_window_note->screen, font, 2, ypos,  makecol(255, 255, 255), -1, "OGG File: \"%s\"", eof_song->tags->ogg[eof_selected_ogg].filename);
	}

	rect(eof_window_note->screen, 0, 0, eof_window_note->w - 1, eof_window_note->h - 1, makecol(160, 160, 160));
	rect(eof_window_note->screen, 1, 1, eof_window_note->w - 2, eof_window_note->h - 2, eof_color_black);
	hline(eof_window_note->screen, 1, eof_window_note->h - 2, eof_window_note->w - 2, makecol(255, 255, 255));
	vline(eof_window_note->screen, eof_window_note->w - 2, 1, eof_window_note->h - 2, makecol(255, 255, 255));
}

void eof_render_lyric_preview(BITMAP * bp)
{
	#define MAX_LYRIC_PREVIEW_LENGTH 255
	unsigned long currentlength=0;	//Used to track the length of the preview line being built
	unsigned long lyriclength=0;	//The length of the lyric being added
	char *tempstring=NULL;			//The code to render in green needs special handling to suppress the / character
	EOF_LYRIC_LINE *linenum=NULL;	//Used to find the background color to render the lyric lines in (green for overdrive, otherwise transparent)
	int bgcol1=-1,bgcol2=-1;

	char lline[2][MAX_LYRIC_PREVIEW_LENGTH+1] = {{0}};
	int i,x;
	int offset = -1;
	int space;	//Track the spacing for lyric preview, taking pitch shift and grouping logic into account

//Build both lyric preview lines
	for(x = 0; x < 2; x++)
	{	//For each of the two lyric preview lines to build
		space = 0;			//The first lyric will have no space inserted in front of it
		currentlength = 0;	//Reset preview line length counter

		linenum=FindLyricLine(eof_preview_line_lyric[x]);	//Find the line structure representing this lyric preview
		if(linenum && (linenum->flags & EOF_LYRIC_LINE_FLAG_OVERDRIVE))	//If this line is overdrive
		{
			if(x == 0)	//This is the first preview line
				bgcol1=makecol(64, 128, 64);	//Render the line's text with an overdrive green background
			else		//This is the second preview line
				bgcol2=makecol(64, 128, 64);	//Render the line's text with an overdrive green background
		}

		for(i = eof_preview_line_lyric[x]; i < eof_preview_line_end_lyric[x]; i++)
		{	//For each lyric in the preview line
			lyriclength=ustrlen(eof_song->vocal_track->lyric[i]->text);	//This value will be used multiple times

		//Perform grouping logic
			if((eof_song->vocal_track->lyric[i]->text[0] != '+'))
			{	//If the lyric is not a pitch shift
				if(space)
				{	//If space is nonzero, append a space if possible
					if(currentlength+1 <= MAX_LYRIC_PREVIEW_LENGTH)
					{	//If appending a space would NOT cause an overflow
						ustrcat(lline[x], " ");
						currentlength++;	//Track the length of this preview line
					}
					else
						break;				//Stop building this line's preview
				}

				if(ustrchr(eof_song->vocal_track->lyric[i]->text,'-'))
				{	//If the lyric contains a hyphen
					space = 0;	//The next syllable will group with this one
				}
				else
				{
					space = 1;	//The next syllable will not group with this one, and will be separated by space
				}
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
			ustrcat(lline[x], eof_song->vocal_track->lyric[i]->text);
			currentlength+=lyriclength;									//Track the length of this preview line

		//Truncate a '/' character off the end of the lyric line if it exists (TB:RB notation for a forced line break)
			if(lline[x][ustrlen(lline[x])-1] == '/')
				lline[x][ustrlen(lline[x])-1]='\0';
		}
	}

	textout_centre_ex(bp, font, lline[0], bp->w / 2, 20, makecol(255, 255, 255), bgcol1);
	textout_centre_ex(bp, font, lline[1], bp->w / 2, 36, makecol(255, 255, 255), bgcol2);
	if((offset >= 0) && (eof_hover_lyric >= 0))
	{
		if(eof_song->vocal_track->lyric[eof_hover_lyric]->text[strlen(eof_song->vocal_track->lyric[eof_hover_lyric]->text)-1] == '/')
		{	//If the at-playback position lyric ends in a forward slash, make a copy with the slash removed and display it instead
			tempstring=malloc(ustrlen(eof_song->vocal_track->lyric[eof_hover_lyric]->text)+1);
			if(tempstring==NULL)	//If there wasn't enough memory to copy this string...
				return;
			ustrcpy(tempstring,eof_song->vocal_track->lyric[eof_hover_lyric]->text);
			tempstring[ustrlen(tempstring)-1]='\0';
			textout_ex(bp, font, tempstring, bp->w / 2 - text_length(font, lline[0]) / 2 + offset, 20, makecol(0, 255, 0), -1);
			free(tempstring);
		}
		else	//Otherwise, display the regular string instead
			textout_ex(bp, font, eof_song->vocal_track->lyric[eof_hover_lyric]->text, bp->w / 2 - text_length(font, lline[0]) / 2 + offset, 20, makecol(0, 255, 0), -1);
	}
}

void eof_render_lyric_window(void)
{
	int i;
//	int kw = (eof_window_3d->screen->w - eof_window_3d->screen->w % 29) / 29;
//	int kh = kw * 4;
//	int bkh = kw * 3;
	int k, n;
	int kcol;
	int kcol2;
	int note[7] = {0, 2, 4, 5, 7, 9, 11};
	int bnote[7] = {1, 3, 0, 6, 8, 10, 0};

	clear_to_color(eof_window_3d->screen, makecol(64, 64, 64));

	/* render the 29 white keys */
	for(i = 0; i < 29; i++)
	{
		n = (i / 7) * 12 + note[i % 7] + MINPITCH;
		if(n == eof_hover_key)
		{
			if((n >= eof_vocals_offset) && (n < eof_vocals_offset + eof_screen_layout.vocal_view_size))
			{
				kcol = makecol(0, 255, 0);
				kcol2 = makecol(0, 192, 0);
			}
			else
			{
				kcol = makecol(0, 160, 0);
				kcol2 = makecol(0, 96, 0);
			}
			textprintf_centre_ex(eof_window_3d->screen, font, i * eof_screen_layout.lyric_view_key_width + eof_screen_layout.lyric_view_key_width / 2, eof_window_3d->h - eof_screen_layout.lyric_view_key_height - text_height(font), eof_color_white, eof_color_black, "%s", eof_get_tone_name(eof_hover_key));
		}
		else
		{
			if((n >= eof_vocals_offset) && (n < eof_vocals_offset + eof_screen_layout.vocal_view_size))
			{
				kcol = makecol(255, 255, 255);
				kcol2 = makecol(192, 192, 192);
			}
			else
			{
				kcol = makecol(160, 160, 160);
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
					kcol = makecol(0, 255, 0);
					kcol2 = makecol(0, 192, 0);
				}
				else
				{
					kcol = makecol(0, 160, 0);
					kcol2 = makecol(0, 96, 0);
				}
				textprintf_centre_ex(eof_window_3d->screen, font, i * eof_screen_layout.lyric_view_key_width + eof_screen_layout.lyric_view_key_width / 2 + eof_screen_layout.lyric_view_bkey_width + eof_screen_layout.lyric_view_bkey_width / 2, eof_window_3d->h - eof_screen_layout.lyric_view_key_height - text_height(font), eof_color_white, eof_color_black, "%s", eof_get_tone_name(eof_hover_key));
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
					kcol2 = makecol(0, 0, 0);
				}
			}
			rectfill(eof_window_3d->screen, i * eof_screen_layout.lyric_view_key_width + eof_screen_layout.lyric_view_key_width / 2 + eof_screen_layout.lyric_view_bkey_width, eof_window_3d->screen->h - eof_screen_layout.lyric_view_key_height, (i + 1) * eof_screen_layout.lyric_view_key_width + eof_screen_layout.lyric_view_key_width / 2 - eof_screen_layout.lyric_view_bkey_width + 1, eof_window_3d->screen->h - eof_screen_layout.lyric_view_key_height + eof_screen_layout.lyric_view_bkey_height, kcol);
			vline(eof_window_3d->screen, (i + 1) * eof_screen_layout.lyric_view_key_width + eof_screen_layout.lyric_view_key_width / 2 - eof_screen_layout.lyric_view_bkey_width + 1, eof_window_3d->screen->h - eof_screen_layout.lyric_view_key_height, eof_window_3d->screen->h - eof_screen_layout.lyric_view_key_height + eof_screen_layout.lyric_view_bkey_height, kcol2);
		}
	}
	eof_render_lyric_preview(eof_window_3d->screen);

	rect(eof_window_3d->screen, 0, 0, eof_window_3d->w - 1, eof_window_3d->h - 1, makecol(160, 160, 160));
	rect(eof_window_3d->screen, 1, 1, eof_window_3d->w - 2, eof_window_3d->h - 2, eof_color_black);
	hline(eof_window_3d->screen, 1, eof_window_3d->h - 2, eof_window_3d->w - 2, makecol(255, 255, 255));
	vline(eof_window_3d->screen, eof_window_3d->w - 2, 1, eof_window_3d->h - 2, makecol(255, 255, 255));
}

void eof_render_3d_window(void)
{
	int point[8];
	int i;

	clear_to_color(eof_window_3d->screen, makecol(64, 64, 64));

	point[0] = ocd3d_project_x(20, 600);
	point[1] = ocd3d_project_y(200, 600);
	point[2] = ocd3d_project_x(300, 600);
	point[3] = ocd3d_project_y(200, 600);
	point[4] = ocd3d_project_x(300, -100);
	point[5] = ocd3d_project_y(200, -100);
	point[6] = ocd3d_project_x(20, -100);
	point[7] = ocd3d_project_y(200, -100);
	polygon(eof_window_3d->screen, 4, point, eof_color_black);

	/* render solo sections */
	int sz, sez;
	int spz, spez;
	for(i = 0; i < eof_song->track[eof_selected_track]->solos; i++)
	{
		sz = -eof_music_pos / eof_zoom_3d + eof_song->track[eof_selected_track]->solo[i].start_pos / eof_zoom_3d + eof_av_delay / eof_zoom_3d;
		sez = -eof_music_pos / eof_zoom_3d + eof_song->track[eof_selected_track]->solo[i].end_pos / eof_zoom_3d + eof_av_delay / eof_zoom_3d;
		if((-100 <= sez) && (600 >= sz))
		{
			spz = sz < -100 ? -100 : sz;
			spez = sez > 600 ? 600 : sez;
			point[0] = ocd3d_project_x(20, spez);
			point[1] = ocd3d_project_y(200, spez);
			point[2] = ocd3d_project_x(300, spez);
			point[3] = ocd3d_project_y(200, spez);
			point[4] = ocd3d_project_x(300, spz);
			point[5] = ocd3d_project_y(200, spz);
			point[6] = ocd3d_project_x(20, spz);
			point[7] = ocd3d_project_y(200, spz);
			polygon(eof_window_3d->screen, 4, point, makecol(0, 0, 64));
		}
	}

	/* draw the 'strings' */
	int obx, oby, oex, oey;
	int px, py, pw;

	px = eof_window_3d->w / 2;
	py = 0;
	pw = 320;

	/* draw the beat markers */
	int bz;
	int beat_counter = 0;
	int beats_per_measure = 0;
	for(i = 0; i < eof_song->beats; i++)
	{
		if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_START_4_4)
		{
			beats_per_measure = 4;
			beat_counter = 0;
		}
		else if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_START_3_4)
		{
			beats_per_measure = 3;
			beat_counter = 0;
		}
		else if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_START_5_4)
		{
			beats_per_measure = 5;
			beat_counter = 0;
		}
		else if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_START_6_4)
		{
			beats_per_measure = 6;
			beat_counter = 0;
		}
		bz = -eof_music_pos / eof_zoom_3d + eof_song->beat[i]->pos / eof_zoom_3d + eof_av_delay / eof_zoom_3d;
		if((bz >= -100) && (bz <= 600))
		{
			line(eof_window_3d->screen, ocd3d_project_x(48, bz), ocd3d_project_y(200, bz), ocd3d_project_x(48 + 4 * 56, bz), ocd3d_project_y(200, bz), beat_counter == 0 ? eof_color_white : makecol(160, 160, 160));
		}
		beat_counter++;
		if(beat_counter >= beats_per_measure)
		{
			beat_counter = 0;
		}
	}

	for(i = 0; i < 5; i++)
	{
		obx = ocd3d_project_x(20 + i * 56 + 28, -100);
		oex = ocd3d_project_x(20 + i * 56 + 28, 600);
		oby = ocd3d_project_y(200, -100);
		oey = ocd3d_project_y(200, 600);

		line(eof_window_3d->screen, obx, oby, oex, oey, eof_color_white);
	}

	/* draw the position marker */
	line(eof_window_3d->screen, ocd3d_project_x(48, 0), ocd3d_project_y(200, 0), ocd3d_project_x(48 + 4 * 56, 0), ocd3d_project_y(200, 0), eof_color_green);

//	int first_note = -1;	//Used for debugging
//	int last_note = 0;
	int tr;
	/* draw the note tails and notes */
	for(i = eof_song->track[eof_selected_track]->notes - 1; i >= 0; i--)
	{	//Render 3D notes from last to first so that the earlier notes are in front
		if(eof_note_type == eof_song->track[eof_selected_track]->note[i]->type)
		{
			tr = eof_note_tail_draw_3d(eof_song->track[eof_selected_track]->note[i], (eof_selection.multi[i] && eof_music_paused) ? 1 : i == eof_hover_note ? 2 : 0);
			eof_note_draw_3d(eof_song->track[eof_selected_track]->note[i], (eof_selection.track == eof_selected_track && eof_selection.multi[i] && eof_music_paused) ? 1 : i == eof_hover_note ? 2 : 0);

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
//	allegro_message("first = %d\nlast = %d", first_note, last_note);

	eof_render_lyric_preview(eof_window_3d->screen);

	rect(eof_window_3d->screen, 0, 0, eof_window_3d->w - 1, eof_window_3d->h - 1, makecol(160, 160, 160));
	rect(eof_window_3d->screen, 1, 1, eof_window_3d->w - 2, eof_window_3d->h - 2, eof_color_black);
	hline(eof_window_3d->screen, 1, eof_window_3d->h - 2, eof_window_3d->w - 2, makecol(255, 255, 255));
	vline(eof_window_3d->screen, eof_window_3d->w - 2, 1, eof_window_3d->h - 2, makecol(255, 255, 255));
}

void eof_render(void)
{
	/* don't draw if window is out of focus */
	if(!eof_has_focus)
	{
		return;
	}
	if(eof_song_loaded)
	{
		clear_to_color(eof_screen, makecol(224, 224, 224));
		if(eof_count_selected_notes(NULL, 0) > 0)
		{
			blit(eof_image[EOF_IMAGE_MENU_FULL], eof_screen, 0, 0, 0, 0, eof_screen->w, eof_screen->h);
		}
		else
		{
			blit(eof_image[EOF_IMAGE_MENU_NO_NOTE], eof_screen, 0, 0, 0, 0, eof_screen->w, eof_screen->h);
		}
		if(eof_vocals_selected)
		{
 			eof_render_vocal_editor_window();
			eof_render_lyric_window();
		}
		else
		{
 			eof_render_editor_window();
			eof_render_3d_window();
		}
		eof_render_note_window();
	}
	else
	{
		clear_to_color(eof_screen, eof_color_gray);
		blit(eof_image[EOF_IMAGE_MENU], eof_screen, 0, 0, 0, 0, eof_screen->w, eof_screen->h);
	}

	if(eof_cursor_visible && eof_soft_cursor)
	{
		draw_sprite(eof_screen, mouse_sprite, mouse_x - 1, mouse_y - 1);
	}

	if(!eof_disable_vsync)
	{
		IdleUntilVSync();
		vsync();
		DoneVSync();
	}
	blit(eof_screen, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
}

void eof_reset_song(void)
{
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

/*
int eof_check_arg_desktop(int argc, char * argv[])
{
	int i;

	for(i = 0; i < argc; i++)
	{
		if(!ustricmp(argv[i], "-desktop"))
		{
			set_color_depth(desktop_color_depth() != 0 ? desktop_color_depth() : 8);
		}
	}
	return 1;
}


int eof_check_arg_softmouse(int argc, char * argv[])
{
	int i;

	for(i = 0; i < argc; i++)
	{
		if(!ustricmp(argv[i], "-softmouse"))
		{
			eof_soft_cursor = 1;
		}
	}
	return 1;
}
*/

int eof_load_data(void)
{
	int i;

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
	for(i = 1; i <= EOF_IMAGE_MENU_NO_NOTE; i++)
	{
		if(!eof_image[i])
		{
			allegro_message("Could not load all images!\nFailed on %d", i);
			return 0;
		}
	}
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
	eof_image[EOF_IMAGE_LYRIC_SCRATCH] = create_bitmap(320, text_height(eof_font) - 1);
	font = eof_font;
	set_palette(eof_palette);
	set_mouse_sprite(NULL);

	eof_color_black = makecol(0, 0, 0);
	eof_color_white = makecol(255, 255, 255);
	eof_color_gray = makecol(64, 64, 64);
	eof_color_red = makecol(255, 0, 0);
	eof_color_green = makecol(0, 255, 0);
	eof_color_blue = makecol(0, 0, 255);
	eof_color_yellow = makecol(255, 255, 0);
	eof_color_purple = makecol(255, 0, 255);

    gui_fg_color = agup_fg_color;
    gui_bg_color = agup_bg_color;
    gui_mg_color = agup_mg_color;
	agup_init(awin95_theme);

	return 1;
}

void eof_destroy_data(void)
{
	int i;

	agup_shutdown();
	for(i = 0; i < EOF_MAX_IMAGES; i++)
	{
		if(eof_image[i])
		{
			destroy_bitmap(eof_image[i]);
		}
	}
	destroy_font(eof_font);
	destroy_font(eof_mono_font);
}

int eof_initialize(int argc, char * argv[])
{
	int i;
	char temp_filename[1024] = {0};

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
		eof_midi_initialized=0;	//Couldn't set up MIDI
	}
	else
	{
		install_timer();	//Needed to use midi_out()
		eof_midi_initialized=1;
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

	InitIdleSystem();

	show_mouse(NULL);
	eof_reset_song();

	/* make sure we are in the proper directory before loading external data */
	if(argc > 1)
	{
		if(!file_exists("eof.dat", 0, NULL))
		{
			get_executable_name(temp_filename, 1024);
			replace_filename(temp_filename, temp_filename, "", 1024);
			if(eof_chdir(temp_filename))
			{
				allegro_message("Could not load program data!\n%s", temp_filename);
				return 1;
			}
		}
	}

	/* reset songs path */
	get_executable_name(eof_songs_path, 1024);
	replace_filename(eof_songs_path, eof_songs_path, "", 1024);

	eof_load_config("eof.cfg");
	if(eof_desktop)
	{
		set_color_depth(desktop_color_depth() != 0 ? desktop_color_depth() : 8);
	}
	if(!eof_set_display_mode(eof_screen_layout.mode))
	{
		allegro_message("Unable to set display mode!");
		return 0;
	}

	if(!eof_load_data())
	{
		return 0;
	}
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

	/* check availability of MP3 conversion tools */
	if(!eof_supports_mp3)
	{
	#ifdef ALLEGRO_WINDOWS
		if((eof_system("lame -S check.wav >foo") == 0) && (eof_system("oggenc2 -h >foo") == 0))
		{
			eof_supports_mp3 = 1;
			delete_file("check.wav.mp3");
			delete_file("foo");
		}
		else
		{
			eof_supports_mp3 = 0;
			delete_file("foo");
			allegro_message("MP3 support disabled.\n\nPlease install LAME and Vorbis Tools if you want MP3 support.");
		}
	#elif defined(ALLEGRO_MACOSX)
		if((eof_system("./lame -S check.wav >foo") == 0) && (eof_system("./oggenc -h >foo") == 0))
		{
			eof_supports_mp3 = 1;
			delete_file("check.wav.mp3");
			delete_file("foo");
		}
		else
		{
			eof_supports_mp3 = 0;
			delete_file("foo");
			allegro_message("MP3 support disabled.\n\nPlease install LAME and Vorbis Tools if you want MP3 support.");
		}
	#else
		if((eof_system("lame -S check.wav >foo") == 0) && (eof_system("oggenc -h >foo") == 0))
		{
			eof_supports_mp3 = 1;
			delete_file("check.wav.mp3");
			delete_file("foo");
		}
		else
		{
			eof_supports_mp3 = 0;
			delete_file("foo");
			allegro_message("MP3 support disabled.\n\nPlease install LAME and Vorbis Tools if you want MP3 support.");
		}
	#endif
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
		allegro_message("Could not create file list filter (*.txt, *.mid, *.rmi, *.rba, *.lrc, *.vl, *.kar, *.mp3)!");
		return 0;
	}
	ncdfs_filter_list_add(eof_filter_lyrics_files, "txt;mid;rmi;rba;lrc;vl;kar;mp3", "Lyrics (*.txt,*.mid,*.rmi,*.rba,*.lrc,*.vl,*.kar,*.mp3)", 1);

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

	/* see if we are opening a file on launch */
	for(i = 1; i < argc; i++)
	{
		if(!ustricmp(argv[i], "-debug"))
		{
			eof_debug_mode = 1;
		}
		else if(!ustricmp(argv[i], "-newidle"))
		{
			eof_new_idle_system = 1;
		}
		else if(!eof_song_loaded)
		{
			if(!ustricmp(get_extension(argv[i]), "eof"))
			{
				ustrcpy(eof_song_path, argv[i]);
				ustrcpy(eof_filename, argv[i]);
				replace_filename(eof_last_eof_path, eof_filename, "", 1024);
				ustrcpy(eof_loaded_song_name, get_filename(eof_filename));
				eof_song = eof_load_song(eof_filename);
				if(!eof_song)
				{
					allegro_message("Unable to load project. File could be corrupt!");
					return 0;
				}
				replace_filename(eof_song_path, eof_filename, "", 1024);
				append_filename(temp_filename, eof_song_path, eof_song->tags->ogg[eof_selected_ogg].filename, 1024);
				if(!eof_load_ogg(temp_filename))
				{
					allegro_message("Failed to load OGG!");
					return 0;
				}
				eof_song_loaded = 1;
				eof_music_length = alogg_get_length_msecs_ogg(eof_music_track);
//				break;
			}
			else if(!ustricmp(get_extension(argv[i]), "mid"))
			{
				ustrcpy(eof_song_path, argv[i]);
				ustrcpy(eof_filename, argv[i]);
				replace_filename(eof_last_eof_path, eof_filename, "", 1024);
				ustrcpy(eof_loaded_song_name, get_filename(eof_filename));
				replace_extension(eof_loaded_song_name, eof_loaded_song_name, "eof", 1024);
				eof_song = eof_import_midi(eof_filename);
				if(!eof_song)
				{
					allegro_message("Could not import song!");
					return 0;
				}
				else
				{
					eof_vocal_track_fixup_lyrics(eof_song->vocal_track, 0);
				}
//				break;
			}
			else if(!ustricmp(get_extension(argv[i]), "rba"))
			{
				ustrcpy(eof_song_path, argv[i]);
				ustrcpy(eof_filename, argv[i]);
				replace_filename(eof_last_eof_path, eof_filename, "", 1024);
				ustrcpy(eof_loaded_song_name, get_filename(eof_filename));
				replace_extension(eof_loaded_song_name, eof_loaded_song_name, "eof", 1024);
				replace_filename(temp_filename, eof_filename, "eof_rba_import.tmp", 1024);

				if(eof_extract_rba_midi(eof_filename,temp_filename) == 0)
				{	//If this was an RBA file and the MIDI was extracted successfully
					eof_song = eof_import_midi(temp_filename);
					delete_file(temp_filename);	//Delete temporary file
				}
				if(!eof_song)
				{
					allegro_message("Could not import song!");
					return 0;
				}
				else
				{
					eof_vocal_track_fixup_lyrics(eof_song->vocal_track, 0);
				}
			}
			else if(!ustricmp(get_extension(argv[i]), "chart"))
			{	//Import a Feedback chart via command line
				ustrcpy(eof_song_path, argv[i]);
				ustrcpy(eof_filename, argv[i]);
				replace_filename(eof_last_eof_path, eof_filename, "", 1024);
				ustrcpy(eof_loaded_song_name, get_filename(eof_filename));
				replace_extension(eof_loaded_song_name, eof_loaded_song_name, "eof", 1024);
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

			if(eof_song_loaded)
			{	//The command line load succeeded, perform some common initialization
				eof_init_after_load();	//Initialize variables
				eof_cursor_visible = 1;
				eof_pen_visible = 1;
				show_mouse(NULL);
			}
		}
	}
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

	return 1;
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
			uconvert((char *)eof_windows_internal_argv[i], U_UNICODE, eof_windows_argv[i], U_UTF8, 4096);
		}
		return eof_initialize(eof_windows_argc, eof_windows_argv);
	}
#endif

void eof_exit(void)
{
	int i;

	StopIdleSystem();
	eof_mix_exit();
	eof_save_config("eof.cfg");
	delete_file("eof.redo");
	delete_file("eof.undo0");
	delete_file("eof.undo1");
	delete_file("eof.undo2");
	delete_file("eof.undo3");
	delete_file("eof.undo4");
	delete_file("eof.undo5");
	delete_file("eof.undo6");
	delete_file("eof.undo7");
	delete_file("eof.autoadjust");
	agup_shutdown();
	eof_destroy_ogg();
	eof_window_destroy(eof_window_editor);
	eof_window_destroy(eof_window_note);
	eof_window_destroy(eof_window_3d);
	for(i = 0; i < EOF_MAX_IMAGES; i++)
	{
		if(eof_image[i])
		{
			destroy_bitmap(eof_image[i]);
		}
	}
	destroy_font(eof_font);
	destroy_font(eof_mono_font);
}

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
			if(ret == ALOGG_POLL_PLAYJUSTFINISHED)
			{
				eof_music_actual_pos = alogg_get_pos_msecs_ogg(eof_music_track);
				eof_music_pos = eof_music_actual_pos + eof_av_delay;
				eof_music_paused = 1;
			}
			else if((ret == ALOGG_POLL_NOTPLAYING) || (ret == ALOGG_POLL_FRAMECORRUPT) || (ret == ALOGG_POLL_INTERNALERROR))
			{
				eof_music_actual_pos = alogg_get_pos_msecs_ogg(eof_music_track);
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
//				if(eof_just_played)
//				{
//					eof_music_pos = eof_music_actual_pos - eof_av_delay;
//					eof_just_played = 0;
//				}
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
					if(eof_disable_vsync)
					{
						Idle(eof_cpu_saver);
					}
					else
					{
						#ifndef ALLEGRO_WINDOWS
							Idle(eof_cpu_saver);
						#endif
					}
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

void eof_process_midi_queue(int currentpos)
{	//Process the MIDI queue based on the current chart timestamp (eof_music_pos)
	unsigned char NOTE_ON_DATA[3]={0x91,0x0,127};	//Data sequence for a Note On, channel 1, Note 0
	unsigned char NOTE_OFF_DATA[3]={0x81,0x0,127};	//Data sequence for a Note Off, channel 1, Note 0

	struct MIDIentry *ptr=MIDIqueue;	//Points to the head of the list
	struct MIDIentry *temp;

	for(ptr=MIDIqueue;ptr != NULL;)
	{	//Process all queue entries
		if(ptr->status == 0)	//If this queued note has not been played yet
		{
			if(eof_music_pos >= ptr->startpos)	//If the current chart position is at or past this note's start position
			{
				NOTE_ON_DATA[1]=ptr->note;	//Alter the data sequence to be the appropriate note number
				midi_out(NOTE_ON_DATA,3);	//Turn on this MIDI note
				ptr->status=1;			//Track that it is playing
			}
		}
		else			//This queued note has already been played
		{
			if(eof_music_pos >= ptr->endpos)	//If the current chart position is at or past this note's stop position
			{
				NOTE_OFF_DATA[1]=ptr->note;	//Alter the data sequence to be the appropriate note number
				midi_out(NOTE_OFF_DATA,3);	//Turn off this MIDI note

			//Remove this link from the list
				if(ptr->prev)	//If there's a link that precedes this link
					ptr->prev->next=ptr->next;	//It now points forward to the next link (if there is one)
				else
					MIDIqueue=ptr->next;		//Update head of list

				if(ptr->next)	//If there's a link that follows this link
					ptr->next->prev=ptr->prev;	//It now points back to the previous link (if there is one)
				else
					MIDIqueuetail=ptr->prev;	//Update tail of list

				temp=ptr->next;	//Save the address of the next link (if there is one)
				free(ptr);
				ptr=temp;	//Iterate to the link that followed the deleted link
				continue;	//Restart at top of loop without iterating to the next link
			}
		}
		ptr=ptr->next;	//Point to the next link
	}
}

int eof_midi_queue_add(unsigned char note,int startpos,int endpos)
{
	struct MIDIentry *ptr=NULL;	//Stores the newly allocated link

//Validate input
	if(note > 127)
		return -1;	//return error:  Invalid MIDI note
	if(startpos >= endpos)
		return -1;	//return error:  Duration must be > 0

//Allocate and initialize MIDI queue entry
	ptr=malloc(sizeof(struct MIDIentry));
	if(ptr == NULL)
		return -1;	//return error:  Unable to allocate memory
	ptr->status=0;
	ptr->note=note;
	ptr->startpos=startpos;
	ptr->endpos=endpos;
	ptr->next=NULL;		//When appended to the end of the list, it will never point forward to anything

//Append to list
	if(MIDIqueuetail)	//If the list isn't empty
	{
		MIDIqueuetail->next=ptr;	//Tail points forward to new link
		ptr->prev=MIDIqueuetail;	//New link points backward to tail
		MIDIqueuetail=ptr;		//Update tail
	}
	else
	{
		MIDIqueue=MIDIqueuetail=ptr;	//This new link is both the head and the tail of the list
		ptr->prev=NULL;			//New link points backward to nothing
	}

	return 0;	//return success
}

void eof_midi_queue_destroy(void)
{
	struct MIDIentry *ptr=MIDIqueue,*temp=NULL;

	while(ptr != NULL)	//For all links in the list
	{
		temp=ptr->next;	//Save address of next link
		free(ptr);	//Destroy this link
		ptr=temp;	//Point to next link
	}

	MIDIqueue=MIDIqueuetail=NULL;
}

void eof_init_after_load(void)
{
	eof_changes = 0;
	eof_music_pos = eof_av_delay;
	eof_music_paused = 1;
	eof_selected_track = EOF_TRACK_GUITAR;
	eof_vocals_selected = 0;
	eof_undo_last_type = 0;
	eof_change_count = 0;
	eof_selected_catalog_entry = 0;
	eof_calculate_beats(eof_song);
	eof_determine_hopos();
	eof_detect_difficulties(eof_song);
	eof_reset_lyric_preview_lines();
	eof_select_beat(0);
	eof_prepare_menus();
	eof_undo_reset();
	eof_sort_notes();
	eof_fixup_notes();
	eof_fix_window_title();
	eof_destroy_waveform(eof_waveform);	//If a waveform had already been created, destroy it
	eof_waveform = NULL;
	eof_display_waveform = 0;
}

END_OF_MAIN()
