#include <allegro.h>
#include <math.h>	//For sqrt()
#include <string.h>	//For memcpy()
#include "../agup/agup.h"
#include "../main.h"
#include "../dialog.h"
#include "../mix.h"
#include "../beat.h"
#include "../utility.h"
#include "../midi.h"
#include "../midi_data_import.h"
#include "../ini.h"
#include "../dialog/proc.h"
#include "../undo.h"
#include "../player.h"
#include "../waveform.h"
#include "../silence.h"
#include "../song.h"
#include "../tuning.h"
#include "note.h"	//For eof_feedback_mode_update_note_selection()
#include "song.h"
#include "file.h"	//For eof_menu_prompt_save_changes()

#ifdef USEMEMWATCH
#include "../memwatch.h"
#endif

char eof_track_selected_menu_text[EOF_TRACKS_MAX][EOF_TRACK_NAME_SIZE+1] = {{0}};
	//Allow an extra leading space due to the checkmark erasing the first character in each string
char eof_raw_midi_track_error[] = "(error)";

MENU eof_song_seek_bookmark_menu[] =
{
    {"&0\tNum 0", eof_menu_song_seek_bookmark_0, NULL, 0, NULL},
    {"&1\tNum 1", eof_menu_song_seek_bookmark_1, NULL, 0, NULL},
    {"&2\tNum 2", eof_menu_song_seek_bookmark_2, NULL, 0, NULL},
    {"&3\tNum 3", eof_menu_song_seek_bookmark_3, NULL, 0, NULL},
    {"&4\tNum 4", eof_menu_song_seek_bookmark_4, NULL, 0, NULL},
    {"&5\tNum 5", eof_menu_song_seek_bookmark_5, NULL, 0, NULL},
    {"&6\tNum 6", eof_menu_song_seek_bookmark_6, NULL, 0, NULL},
    {"&7\tNum 7", eof_menu_song_seek_bookmark_7, NULL, 0, NULL},
    {"&8\tNum 8", eof_menu_song_seek_bookmark_8, NULL, 0, NULL},
    {"&9\tNum 9", eof_menu_song_seek_bookmark_9, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_song_seek_menu[] =
{
    {"Start\tHome", eof_menu_song_seek_start, NULL, 0, NULL},
    {"End\tEnd", eof_menu_song_seek_end, NULL, 0, NULL},
    {"Rewind\tR", eof_menu_song_seek_rewind, NULL, 0, NULL},
    {"", NULL, NULL, 0, NULL},
    {"First Note\t" CTRL_NAME "+Home", eof_menu_song_seek_first_note, NULL, 0, NULL},
    {"Last Note\t" CTRL_NAME "+End", eof_menu_song_seek_last_note, NULL, 0, NULL},
    {"Previous Note\tShift+PGUP", eof_menu_song_seek_previous_note, NULL, 0, NULL},
    {"Next Note\tShift+PGDN", eof_menu_song_seek_next_note, NULL, 0, NULL},
    {"", NULL, NULL, 0, NULL},
    {"Previous Screen\t" CTRL_NAME "+PGUP", eof_menu_song_seek_previous_screen, NULL, 0, NULL},
    {"Next Screen\t" CTRL_NAME "+PGDN", eof_menu_song_seek_next_screen, NULL, 0, NULL},
    {"Previous Grid Snap\t" CTRL_NAME "+Shift+PGUP", eof_menu_song_seek_previous_grid_snap, NULL, 0, NULL},
    {"Next Grid Snap\t" CTRL_NAME "+Shift+PGDN", eof_menu_song_seek_next_grid_snap, NULL, 0, NULL},
    {"", NULL, NULL, 0, NULL},
    {"Previous Beat\tPGUP", eof_menu_song_seek_previous_beat, NULL, 0, NULL},
    {"Next Beat\tPGDN", eof_menu_song_seek_next_beat, NULL, 0, NULL},
    {"Previous Anchor\t" CTRL_NAME "+Shift+PGUP", eof_menu_song_seek_previous_anchor, NULL, 0, NULL},
    {"Next Anchor\t" CTRL_NAME "+Shift+PGDN", eof_menu_song_seek_next_anchor, NULL, 0, NULL},
    {"Beat/&Measure", eof_menu_song_seek_beat_measure, NULL, 0, NULL},
    {"", NULL, NULL, 0, NULL},
    {"&Bookmark", NULL, eof_song_seek_bookmark_menu, 0, NULL},
    {"&Catalog entry", eof_menu_song_seek_catalog_entry, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_track_selected_menu[EOF_TRACKS_MAX] =
{
    {eof_track_selected_menu_text[0], eof_menu_track_selected_1, NULL, D_SELECTED, NULL},
    {eof_track_selected_menu_text[1], eof_menu_track_selected_2, NULL, 0, NULL},
    {eof_track_selected_menu_text[2], eof_menu_track_selected_3, NULL, 0, NULL},
    {eof_track_selected_menu_text[3], eof_menu_track_selected_4, NULL, 0, NULL},
    {eof_track_selected_menu_text[4], eof_menu_track_selected_5, NULL, 0, NULL},
    {eof_track_selected_menu_text[5], eof_menu_track_selected_6, NULL, 0, NULL},
    {eof_track_selected_menu_text[6], eof_menu_track_selected_7, NULL, 0, NULL},
    {eof_track_selected_menu_text[7], eof_menu_track_selected_8, NULL, 0, NULL},
    {eof_track_selected_menu_text[8], eof_menu_track_selected_9, NULL, 0, NULL},
    {eof_track_selected_menu_text[9], eof_menu_track_selected_10, NULL, 0, NULL},
    {eof_track_selected_menu_text[10], eof_menu_track_selected_11, NULL, 0, NULL},
    {eof_track_selected_menu_text[11], eof_menu_track_selected_12, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_catalog_menu[] =
{
    {"&Show\tQ", eof_menu_catalog_show, NULL, 0, NULL},
    {"Double &Width\tSHIFT+Q", eof_menu_catalog_toggle_full_width, NULL, 0, NULL},
    {"&Edit Name", eof_menu_catalog_edit_name, NULL, 0, NULL},
    {"", NULL, NULL, 0, NULL},
    {"&Add", eof_menu_catalog_add, NULL, 0, NULL},
    {"&Delete", eof_menu_catalog_delete, NULL, 0, NULL},
    {"", NULL, NULL, 0, NULL},
    {"&Previous\tW", eof_menu_catalog_previous, NULL, 0, NULL},
    {"&Next\tE", eof_menu_catalog_next, NULL, 0, NULL},
    {"", NULL, NULL, 0, NULL},
    {"Find Previous\t" CTRL_NAME "+SHIFT+F3", eof_menu_catalog_find_prev, NULL, 0, NULL},
    {"Find Next\tF3", eof_menu_catalog_find_next, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_waveform_menu[] =
{
	{"&Show\tF5", eof_menu_song_waveform, NULL, 0, NULL},
	{"&Configure", eof_menu_song_waveform_settings, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

DIALOG eof_note_set_num_frets_strings_dialog[] =
{
   /* (proc)                  (x)  (y) (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)  (            dp2) (dp3) */
//   { d_agup_shadow_box_proc,  32,  68, 170, 120, 2,   23,   0,    0,     0,   0,   NULL,              NULL, NULL },
   { d_agup_window_proc,      32,  68, 170, 136, 0,   0,    0,    0,     0,   0,   "Edit fret/string count", NULL, NULL },
   { d_agup_text_proc,        44,  100,110, 8,   2,   23,   0,    0,     0,   0,   "Max fret value:", NULL, NULL },
   { eof_verified_edit_proc,  158, 96, 26,  20,  2,   23,   0,    0,     2,   0,   eof_etext2,        "0123456789", NULL },
   { d_agup_text_proc,		  44,  120,64,	8,   2,   23,   0,    0,     0,   0,   "Number of strings:",NULL, NULL },
   { d_agup_radio_proc,	      44,  140,36,  15,  2,   23,   0,    0,     0,   0,   "4",               NULL, NULL },
   { d_agup_radio_proc,	      80,  140,36,  15,  2,   23,   0,    0,     0,   0,   "5",               NULL, NULL },
   { d_agup_radio_proc,	      116, 140,36,  15,  2,   23,   0,    0,     0,   0,   "6",               NULL, NULL },
   { d_agup_button_proc,      42,  164,68,  28,  2,   23,   '\r', D_EXIT,0,   0,   "OK",              NULL, NULL },
   { d_agup_button_proc,      120, 164,68,  28,  2,   23,   0,    D_EXIT,0,   0,   "Cancel",          NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

MENU eof_song_proguitar_menu[] =
{
    {"Set track &Tuning", eof_menu_song_track_tuning, NULL, 0, NULL},
    {"Set number of &Frets/strings", eof_menu_set_num_frets_strings, NULL, 0, NULL},
    {"Enable &Legacy view\tShift+L", eof_menu_song_legacy_view, NULL, 0, NULL},
    {"&Previous chord name\tShift+W", eof_menu_previous_chord_result, NULL, 0, NULL},
    {"&Next chord name\tShift+E", eof_menu_next_chord_result, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_song_menu[] =
{
    {"&Seek", NULL, eof_song_seek_menu, 0, NULL},
    {"&Track", NULL, eof_track_selected_menu, 0, NULL},
    {"&File Info", eof_menu_song_file_info, NULL, 0, NULL},
    {"&Audio cues", eof_menu_audio_cues, NULL, 0, NULL},
    {"Display semitones as flat", eof_display_flats_menu, NULL, 0, NULL},
    {"Create image sequence", eof_create_image_sequence, NULL, 0, NULL},
    {"&Waveform Graph", NULL, eof_waveform_menu, 0, NULL},
    {"", NULL, NULL, 0, NULL},
    {"&Catalog", NULL, eof_catalog_menu, 0, NULL},
    {"&INI Settings", eof_menu_song_ini_settings, NULL, 0, NULL},
    {"&Properties\tF9", eof_menu_song_properties, NULL, 0, NULL},
    {"Set track &Difficulty", eof_song_track_difficulty_dialog, NULL, 0, NULL},
    {"&Leading Silence", eof_menu_song_add_silence, NULL, 0, NULL},
    {"Enable open strum bass", eof_menu_song_open_bass, NULL, 0, NULL},
    {"Enable five lane drums", eof_menu_song_five_lane_drums, NULL, 0, NULL},
    {"Lock tempo map", eof_menu_song_lock_tempo_map, NULL, 0, NULL},
    {"Disable expert+ bass drum", eof_menu_song_disable_double_bass_drums, NULL, 0, NULL},
    {"Pro &Guitar", NULL, eof_song_proguitar_menu, 0, NULL},
    {"Manage raw MIDI tracks", eof_menu_song_raw_MIDI_tracks, NULL, 0, NULL},
    {"", NULL, NULL, 0, NULL},
    {"T&est In FOF\tF12", eof_menu_song_test_fof, NULL, EOF_LINUX_DISABLE, NULL},
    {"Test I&n Phase Shift", eof_menu_song_test_ps, NULL, EOF_LINUX_DISABLE, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

DIALOG eof_ini_dialog[] =
{
   /* (proc)         (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)           (dp2) (dp3) */
   { d_agup_window_proc,    0,  48,  216 + 110 + 20, 160 + 72, 2,   23,  0,    0,      0,   0,   "INI Settings",               NULL, NULL },
   { d_agup_list_proc,   12, 84,  110 * 2 + 20,  69 * 2,  2,   23,  0,    0,      0,   0,   eof_ini_list, NULL, NULL },
   { d_agup_push_proc, 134 + 130,  84, 68,  28, 2,   23,  0,    D_EXIT, 0,   0,   "Add",               NULL, eof_ini_dialog_add },
   { d_agup_push_proc, 134 + 130,  124, 68,  28, 2,   23,  0,    D_EXIT, 0,   0,   "Delete",               NULL, eof_ini_dialog_delete },
   { d_agup_button_proc, 12,  166 + 69, 190 + 30 + 20,  28, 2,   23,  0,    D_EXIT, 0,   0,   "Done",               NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_ini_add_dialog[] =
{
   /* (proc)         (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)           (dp2) (dp3) */
   { d_agup_window_proc,    0,  48,  204 + 110, 106, 2,   23,  0,    0,      0,   0,   "Add INI Setting",               NULL, NULL },
   { d_agup_text_proc,   12,  84,  64,  8,  2,   23,  0,    0,      0,   0,   "Text:",         NULL, NULL },
   { d_agup_edit_proc,   48, 80,  144 + 110,  20,  2,   23,  0,    0,      255,   0,   eof_etext,           NULL, NULL },
   { d_agup_button_proc, 12 + 55,  112, 84,  28, 2,   23,  '\r',    D_EXIT, 0,   0,   "OK",               NULL, NULL },
   { d_agup_button_proc, 108 + 55, 112, 78,  28, 2,   23,  0,    D_EXIT, 0,   0,   "Cancel",           NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_song_properties_dialog[] =
{
   /* (proc)              (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                    (dp2) (dp3) */
   { d_agup_window_proc,  0,   0,   480, 244, 0,   0,   0,    0,      0,   0,   "Song Properties",      NULL, NULL },
   { d_agup_text_proc,    12,  40,  80,  12,  0,   0,   0,    0,      0,   0,   "Song Title",           NULL, NULL },
   { d_agup_edit_proc,    12,  56,  184, 20,  0,   0,   0,    0,      255, 0,   eof_etext,              NULL, NULL },
   { d_agup_text_proc,    12,  88,  96,  12,  0,   0,   0,    0,      0,   0,   "Artist",               NULL, NULL },
   { d_agup_edit_proc,    12,  104, 184, 20,  0,   0,   0,    0,      255, 0,   eof_etext2,             NULL, NULL },
   { d_agup_text_proc,    12,  136, 108, 12,  0,   0,   0,    0,      0,   0,   "Frettist",             NULL, NULL },
   { d_agup_edit_proc,    12,  152, 184, 20,  0,   0,   0,    0,      255, 0,   eof_etext3,             NULL, NULL },
   { d_agup_text_proc,    12,  188, 60,  12,  0,   0,   0,    0,      0,   0,   "Delay",                NULL, NULL },
   { eof_verified_edit_proc,12,204, 50,  20,  0,   0,   0,    0,      6,   0,   eof_etext4,     "0123456789", NULL },
   { d_agup_text_proc,    74,  188, 48,  12,  0,   0,   0,    0,      0,   0,   "Year",                 NULL, NULL },
   { eof_verified_edit_proc,74,204, 38,  20,  0,   0,   0,    0,      4,   0,   eof_etext5,     "0123456789", NULL },
   { d_agup_text_proc,    124, 188, 60,  12,  0,   0,   0,    0,      0,   0,   "Diff.",                NULL, NULL },
   { eof_verified_edit_proc,124,204,26,  20,  0,   0,   0,    0,      1,   0,   eof_etext7,        "0123456", NULL },
   { d_agup_text_proc,    208, 40,  96,  12,  0,   0,   0,    0,      0,   0,   "Loading Text",         NULL, NULL },
   { d_agup_edit_proc,    208, 56,  256, 20,  0,   0,   0,    0,      255, 0,   eof_etext6,             NULL, NULL },
   { d_agup_text_proc,    208, 88,  96,  12,  0,   0,   0,    0,      0,   0,   "Loading Text Preview", NULL, NULL },
   { d_agup_textbox_proc, 208, 104, 256, 68,  0,   0,   0,    0,      0,   0,   eof_etext6,             NULL, NULL },
   { d_agup_check_proc,   160, 175, 136, 16,  0,   0,   0,    0,      1,   0,   "Lyrics",               NULL, NULL },
   { d_agup_check_proc,   160, 190, 136, 16,  0,   0,   0,    0,      1,   0,   "8th Note HO/PO",       NULL, NULL },
   { d_agup_check_proc,   160, 205, 215, 16,  0,   0,   0,    0,      1,   0,   "Use fret hand pos of 1 (pro g)", NULL, NULL },
   { d_agup_check_proc,   160, 220, 215, 16,  0,   0,   0,    0,      1,   0,   "Use fret hand pos of 1 (pro b)", NULL, NULL },
   { d_agup_button_proc,  380, 204, 84,  24,  0,   0,   '\r', D_EXIT, 0,   0,   "OK",                   NULL, NULL },
   { NULL,                0,   0,   0,   0,   0,   0,   0,    0,      0,   0,   NULL,                   NULL, NULL }
};

void eof_prepare_song_menu(void)
{
	unsigned long i, tracknum;
	long firstnote = -1;
	long lastnote = -1;
	long noted[4] = {0};
	long seekp = 0;
	long seekn = 0;

	if(eof_song && eof_song_loaded)
	{//If a chart is loaded
		tracknum = eof_song->track[eof_selected_track]->tracknum;
		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{
			if(eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type)
			{
				if(firstnote < 0)
				{
					firstnote = i;
				}
				lastnote = i;
			}
			if((eof_get_note_type(eof_song, eof_selected_track, i) >= 0) && (eof_get_note_type(eof_song, eof_selected_track, i) < 4))
			{
				noted[(int)eof_get_note_type(eof_song, eof_selected_track, i)] = 1;	//Type cast to avoid a nag warning about indexing with a char type
			}
			if((eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type) && (eof_get_note_pos(eof_song, eof_selected_track, i) < ((eof_music_pos - eof_av_delay >= 0) ? eof_music_pos - eof_av_delay : 0)))
			{
				seekp = 1;
			}
			if((eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type) && (eof_get_note_pos(eof_song, eof_selected_track, i) > ((eof_music_pos - eof_av_delay >= 0) ? eof_music_pos - eof_av_delay : 0)))
			{
				seekn = 1;
			}
		}

		/* track */
		for(i = 0; i < EOF_TRACKS_MAX; i++)
		{
			char *ptr = eof_track_selected_menu_text[i];	//Do this to work around false alarm warnings
			eof_track_selected_menu[i].flags = 0;
			if((i + 1 < eof_song->tracks) && (eof_song->track[i+1] != NULL))
			{	//If the track exists, copy its name into the string used by the track menu
				ptr[0] = ' ';	//Add a leading space
				ustrncpy(&(ptr[1]),eof_song->track[i+1]->name,EOF_TRACK_NAME_SIZE-1);
					//Append the track name to the menu string, starting at index 1
			}
			else
			{	//Write a blank string for the track name
				ustrcpy(eof_track_selected_menu_text[i],"");
			}
		}
		eof_track_selected_menu[eof_selected_track-1].flags = D_SELECTED;	//Track numbering begins at 1 instead of 0

		/* seek start */
		if(eof_music_pos == eof_av_delay)
		{	//If the seek position is already at the start of the chart
			eof_song_seek_menu[0].flags = D_DISABLED;	//Seek start
			eof_song_seek_menu[14].flags = D_DISABLED;	//Previous beat
		}
		else
		{
			eof_song_seek_menu[0].flags = 0;
			eof_song_seek_menu[14].flags = 0;
		}

		/* seek end */
		if(eof_music_pos >= eof_music_actual_length - 1)
		{	//If the seek position is already at the end of the chart
			eof_song_seek_menu[1].flags = D_DISABLED;	//Seek end
			eof_song_seek_menu[15].flags = D_DISABLED;	//Next beat
		}
		else
		{
			eof_song_seek_menu[1].flags = 0;
			eof_song_seek_menu[15].flags = 0;
		}

		/* rewind */
		if(eof_music_pos == eof_music_rewind_pos)
		{
			eof_song_seek_menu[2].flags = D_DISABLED;	//Rewind
		}
		else
		{
			eof_song_seek_menu[2].flags = 0;
		}
		if(eof_vocals_selected)
		{
			if(eof_song->vocal_track[tracknum]->lyrics)
			{
				/* seek first note */
				if(eof_song->vocal_track[tracknum]->lyric[0]->pos == eof_music_pos - eof_av_delay)
				{
					eof_song_seek_menu[4].flags = D_DISABLED;	//First note
				}
				else
				{
					eof_song_seek_menu[4].flags = 0;
				}

				/* seek last note */
				if(eof_song->vocal_track[tracknum]->lyric[eof_song->vocal_track[tracknum]->lyrics - 1]->pos == eof_music_pos - eof_av_delay)
				{
					eof_song_seek_menu[5].flags = D_DISABLED;	//Last note
				}
				else
				{
					eof_song_seek_menu[5].flags = 0;
				}
			}
			else
			{
				eof_song_seek_menu[4].flags = D_DISABLED;
				eof_song_seek_menu[5].flags = D_DISABLED;
			}
		}
		else
		{
			if(noted[eof_note_type])
			{
				/* seek first note */
				if((firstnote >= 0) && (eof_get_note_pos(eof_song, eof_selected_track, firstnote) == eof_music_pos - eof_av_delay))
				{
					eof_song_seek_menu[4].flags = D_DISABLED;	//First note
				}
				else
				{
					eof_song_seek_menu[4].flags = 0;
				}

				/* seek last note */
				if((lastnote >= 0) && (eof_get_note_pos(eof_song, eof_selected_track, lastnote) == eof_music_pos - eof_av_delay))
				{
					eof_song_seek_menu[5].flags = D_DISABLED;	//Last note
				}
				else
				{
					eof_song_seek_menu[5].flags = 0;
				}
			}
			else
			{
				eof_song_seek_menu[4].flags = D_DISABLED;
				eof_song_seek_menu[5].flags = D_DISABLED;
			}
		}

		/* seek previous note */
		if(seekp)
		{
			eof_song_seek_menu[6].flags = 0;	//Previous note
		}
		else
		{
			eof_song_seek_menu[6].flags = D_DISABLED;
		}

		/* seek next note */
		if(seekn)
		{
			eof_song_seek_menu[7].flags = 0;	//Next note
		}
		else
		{
			eof_song_seek_menu[7].flags = D_DISABLED;
		}

		/* seek previous screen */
		if(eof_music_pos <= eof_av_delay)
		{
			eof_song_seek_menu[9].flags = D_DISABLED;	//Previous screen
		}
		else
		{
			eof_song_seek_menu[9].flags = 0;
		}

		/* seek next screen */
		if(eof_music_pos >= eof_music_actual_length - 1)
		{
			eof_song_seek_menu[10].flags = D_DISABLED;	//Next screen
		}
		else
		{
			eof_song_seek_menu[10].flags = 0;
		}

		/* previous/next grid snap/anchor */
		if(eof_snap_mode == EOF_SNAP_OFF)
		{	//If grid snap is disabled
			eof_song_seek_menu[11].flags = D_DISABLED;	//Previous grid snap
			eof_song_seek_menu[12].flags = D_DISABLED;	//Next grid snap
			eof_song_seek_menu[16].flags = 0;			//Previous anchor
			eof_song_seek_menu[17].flags = 0;			//Next anchor
		}
		else
		{
			eof_song_seek_menu[11].flags = 0;			//Previous grid snap
			eof_song_seek_menu[12].flags = 0;			//Next grid snap
			eof_song_seek_menu[16].flags = D_DISABLED;	//Previous anchor
			eof_song_seek_menu[17].flags = D_DISABLED;	//Next anchor
		}

		/* seek bookmark # */
		char bmcount = 0;
		for(i = 0; i < EOF_MAX_BOOKMARK_ENTRIES; i++)
		{
			if(eof_song->bookmark_pos[i] != 0)
			{
				eof_song_seek_bookmark_menu[i].flags = 0;
				bmcount++;
			}
			else
			{
				eof_song_seek_bookmark_menu[i].flags = D_DISABLED;
			}
		}

		/* seek bookmark */
		if(bmcount == 0)
		{
			eof_song_seek_menu[20].flags = D_DISABLED;	//Bookmark
		}
		else
		{
			eof_song_seek_menu[20].flags = 0;
		}

		/* show catalog */
		/* edit name */
		/* seek catalog entry */
		if(eof_song->catalog->entries > 0)
		{
			eof_catalog_menu[0].flags = eof_catalog_menu[0].flags & D_SELECTED;	//Enable "Show Catalog" and check it if it's already checked
			eof_catalog_menu[2].flags = 0;		//Enable "Edit name"
			eof_song_seek_menu[21].flags = 0;	//Enable Seek>Catalog entry
		}
		else
		{
			eof_catalog_menu[0].flags = D_DISABLED;	//Disable "Show catalog"
			eof_catalog_menu[2].flags = D_DISABLED;	//Disable "Edit name"
			eof_song_seek_menu[21].flags = D_DISABLED;	//Disable Seek>Catalog entry
		}

		/* add catalog entry */
		if(eof_count_selected_notes(NULL,0))	//If there are notes selected
		{
			eof_catalog_menu[4].flags = 0;
		}
		else
		{
			eof_catalog_menu[4].flags = D_DISABLED;
		}

		/* remove catalog entry */
		/* find previous */
		/* find next */
		if(eof_selected_catalog_entry < eof_song->catalog->entries)
		{
			eof_catalog_menu[5].flags = 0;
			eof_catalog_menu[10].flags = 0;
			eof_catalog_menu[11].flags = 0;
		}
		else
		{
			eof_catalog_menu[5].flags = D_DISABLED;
			eof_catalog_menu[10].flags = D_DISABLED;
			eof_catalog_menu[11].flags = D_DISABLED;
		}

		/* previous/next catalog entry */
		if(eof_song->catalog->entries > 1)
		{
			eof_catalog_menu[7].flags = 0;
			eof_catalog_menu[8].flags = 0;
		}
		else
		{
			eof_catalog_menu[7].flags = D_DISABLED;
			eof_catalog_menu[8].flags = D_DISABLED;
		}

		/* catalog */
		if(!eof_song->catalog->entries && (eof_catalog_menu[4].flags == D_DISABLED))
		{	//If there are no catalog entries and no notes selected (in which case Song>Catalog>Add would have been disabled earlier)
			eof_song_menu[8].flags = D_DISABLED;	//Song>Catalog> submenu
		}
		else
		{
			eof_song_menu[8].flags = 0;
		}

		/* track */
		for(i = 0; i < EOF_TRACKS_MAX - 1; i++)
		{	//Access this array reflecting track numbering starting from 0 instead of 1
			if(eof_get_track_size(eof_song,i+1))
			{	//If the track exists and is populated, draw the track populated indicator
				eof_track_selected_menu[i].text[0] = '*';
			}
			else
			{	//Otherwise clear the indicator from the menu
				eof_track_selected_menu[i].text[0] = ' ';
			}
		}

		/* enable open strum bass */
		if(eof_open_bass_enabled())
		{
			eof_song_menu[13].flags = D_SELECTED;	//Song>Enable open strum bass
		}
		else
		{
			eof_song_menu[13].flags = 0;
		}

		/* enable five lane drums */
		if(eof_five_lane_drums_enabled())
		{
			eof_song_menu[14].flags = D_SELECTED;	//Song>Enable five lane drums
		}
		else
		{
			eof_song_menu[14].flags = 0;
		}

		/* lock tempo map */
		if(eof_song->tags->tempo_map_locked)
		{
			eof_song_menu[15].flags = D_SELECTED;	//Song>Lock tempo map
		}
		else
		{
			eof_song_menu[15].flags = 0;
		}

		/* Disable expert+ bass drum */
		if(eof_song->tags->double_bass_drum_disabled)
		{
			eof_song_menu[16].flags = D_SELECTED;	//Song>Disable expert+ bass drum
		}
		else
		{
			eof_song_menu[16].flags = 0;
		}

		/* enable pro guitar submenu */
		if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If a pro guitar track is active
			eof_song_menu[17].flags = 0;			//Song>Pro Guitar> submenu

			if(eof_enable_chord_cache && (eof_chord_lookup_count > 1))
			{	//If an un-named note is selected and it has at least two chord matches
				eof_song_proguitar_menu[3].flags = 0;	//Previous chord name
				eof_song_proguitar_menu[4].flags = 0;	//Next chord name
			}
			else
			{
				eof_song_proguitar_menu[3].flags = D_DISABLED;
				eof_song_proguitar_menu[4].flags = D_DISABLED;
			}
		}
		else
		{	//Otherwise disable this menu item
			eof_song_menu[17].flags = D_DISABLED;
		}
	}//If a chart is loaded
}

int eof_menu_song_seek_start(void)
{
	char wasplaying = 0;
	if(!eof_music_catalog_playback)
	{
		if(!eof_music_paused)	//If the chart is already playing
		{
			eof_music_play();	//stop it
			wasplaying = 1;
		}
		if(eof_input_mode == EOF_INPUT_FEEDBACK)
		{	//If the feedback input method is in effect
			eof_set_seek_position(eof_song->beat[0]->pos + eof_av_delay);	//Seek to the first beat marker, because the Feedback seek controls aren't designed to work if the seek position is earlier than this
		}
		else
		{
			eof_set_seek_position(eof_av_delay);	//Seek to beginning of piano roll (adjusted for AV delay)
		}
		if(wasplaying)
		{	//If the playback was stopped to rewind the seek position
			eof_music_play();	//Resume playing
		}
	}
	return 1;
}

int eof_menu_song_seek_end(void)
{
	if(!eof_music_catalog_playback)
	{
		eof_set_seek_position(eof_music_actual_length - 1);
	}
	return 1;
}

int eof_menu_song_seek_rewind(void)
{
	if(!eof_music_catalog_playback)
	{
		eof_set_seek_position(eof_music_rewind_pos);
	}
	return 1;
}

int eof_menu_song_seek_first_note(void)
{
	unsigned long i;
	unsigned long first_pos = 0;
	char firstfound = 0;

	if(!eof_music_catalog_playback)
	{
		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//For each note in the active track
			if(eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type)
			{
				if(!firstfound || (eof_get_note_pos(eof_song, eof_selected_track, i) < first_pos))
				{
					first_pos = eof_get_note_pos(eof_song, eof_selected_track, i);
					firstfound = 1;
				}
			}
		}
		eof_set_seek_position(first_pos + eof_av_delay);
	}
	return 1;
}

int eof_menu_song_seek_last_note(void)
{
	unsigned long i;
	unsigned long last_pos = 0;
	char firstfound = 0;

	if(!eof_music_catalog_playback)
	{
		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//For each note in the active track
			if((eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type) && (eof_get_note_pos(eof_song, eof_selected_track, i) < eof_music_length))
			{
				if(!firstfound || (eof_get_note_pos(eof_song, eof_selected_track, i) > last_pos))
				{
					last_pos = eof_get_note_pos(eof_song, eof_selected_track, i);
					firstfound = 1;
				}
			}
		}
		eof_set_seek_position(last_pos + eof_av_delay);
	}
	return 1;
}

int eof_menu_song_seek_previous_note(void)
{
	unsigned long i;

	for(i = eof_get_track_size(eof_song, eof_selected_track); i > 0; i--)
	{	//For each note in the active track
		if((eof_get_note_type(eof_song, eof_selected_track, i-1) == eof_note_type) && (eof_get_note_pos(eof_song, eof_selected_track, i-1) < ((eof_music_pos - eof_av_delay >= 0) ? eof_music_pos - eof_av_delay : 0)))
		{
			eof_set_seek_position(eof_get_note_pos(eof_song, eof_selected_track, i-1) + eof_av_delay);
			break;
		}
	}
	return 1;
}

int eof_menu_song_seek_next_note(void)
{
	unsigned long i;

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type) && (eof_get_note_pos(eof_song, eof_selected_track, i) < eof_music_length) && (eof_get_note_pos(eof_song, eof_selected_track, i) > ((eof_music_pos - eof_av_delay >= 0) ? eof_music_pos - eof_av_delay : 0)))
		{
			eof_set_seek_position(eof_get_note_pos(eof_song, eof_selected_track, i) + eof_av_delay);
			break;
		}
	}
	return 1;
}

int eof_menu_song_seek_previous_screen(void)
{
	if(!eof_music_catalog_playback)
	{
		if(eof_music_pos - SCREEN_W * eof_zoom < 0)
		{
			eof_menu_song_seek_start();
		}
		else
		{
			eof_set_seek_position(eof_music_pos - SCREEN_W * eof_zoom);
		}
	}
	return 1;
}

int eof_menu_song_seek_next_screen(void)
{
	if(!eof_music_catalog_playback)
	{
		if(eof_music_pos + SCREEN_W * eof_zoom > eof_music_length)
		{
			eof_menu_song_seek_end();
		}
		else
		{
			eof_set_seek_position(eof_music_pos + SCREEN_W * eof_zoom);
		}
	}
	return 1;
}

int eof_menu_song_seek_bookmark_help(int b)
{
	if(!eof_music_catalog_playback && (eof_song->bookmark_pos[b] != 0))
	{
		eof_set_seek_position(eof_song->bookmark_pos[b] + eof_av_delay);
		return 1;
	}
	return 0;
}

int eof_menu_song_seek_bookmark_0(void)
{
	eof_menu_song_seek_bookmark_help(0);
	return 1;
}

int eof_menu_song_seek_bookmark_1(void)
{
	eof_menu_song_seek_bookmark_help(1);
	return 1;
}

int eof_menu_song_seek_bookmark_2(void)
{
	eof_menu_song_seek_bookmark_help(2);
	return 1;
}

int eof_menu_song_seek_bookmark_3(void)
{
	eof_menu_song_seek_bookmark_help(3);
	return 1;
}

int eof_menu_song_seek_bookmark_4(void)
{
	eof_menu_song_seek_bookmark_help(4);
	return 1;
}

int eof_menu_song_seek_bookmark_5(void)
{
	eof_menu_song_seek_bookmark_help(5);
	return 1;
}

int eof_menu_song_seek_bookmark_6(void)
{
	eof_menu_song_seek_bookmark_help(6);
	return 1;
}

int eof_menu_song_seek_bookmark_7(void)
{
	eof_menu_song_seek_bookmark_help(7);
	return 1;
}

int eof_menu_song_seek_bookmark_8(void)
{
	eof_menu_song_seek_bookmark_help(8);
	return 1;
}

int eof_menu_song_seek_bookmark_9(void)
{
	eof_menu_song_seek_bookmark_help(9);
	return 1;
}

int eof_menu_song_file_info(void)
{
	allegro_message("Song Folder:\n  %s\n\nEOF Filename:\n  %s\n\nRevision %lu", eof_song_path, eof_loaded_song_name, eof_song->tags->revision);
	return 1;
}

int eof_menu_song_ini_settings(void)
{
	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_ini_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_ini_dialog);
	if(eof_popup_dialog(eof_ini_dialog, 0) == 4)
	{
	}
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(NULL);
	return 1;
}

int eof_is_number(char * buffer)
{
	unsigned long i;

	for(i = 0; i < ustrlen(buffer); i++)
	{
		if((buffer[i] < '0') || (buffer[i] > '9'))
		{
			return 0;
		}
	}
	return 1;
}

int eof_menu_song_properties(void)
{
	int old_offset = 0;
	int i, invalid = 0;
	unsigned long difficulty, undo_made = 0;
	char newlyrics, neweighth_note_hopo, neweof_fret_hand_pos_1_pg, neweof_fret_hand_pos_1_pb;

	if(!eof_song_loaded || !eof_song)
		return 1;	//Do not allow this function to run if a chart is not loaded

	old_offset = eof_song->tags->ogg[eof_selected_ogg].midi_offset;
	eof_cursor_visible = 0;
	if(eof_song_loaded)
	{
		eof_music_paused = 1;
		eof_stop_midi();
		alogg_stop_ogg(eof_music_track);
	}
	eof_render();
	eof_color_dialog(eof_song_properties_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_song_properties_dialog);
	ustrcpy(eof_etext, eof_song->tags->title);
	ustrcpy(eof_etext2, eof_song->tags->artist);
	ustrcpy(eof_etext3, eof_song->tags->frettist);
	sprintf(eof_etext4, "%ld", eof_song->tags->ogg[eof_selected_ogg].midi_offset);
	ustrcpy(eof_etext5, eof_song->tags->year);
	ustrcpy(eof_etext6, eof_song->tags->loading_text);
	eof_song_properties_dialog[17].flags = eof_song->tags->lyrics ? D_SELECTED : 0;
	eof_song_properties_dialog[18].flags = eof_song->tags->eighth_note_hopo ? D_SELECTED : 0;
	eof_song_properties_dialog[19].flags = eof_song->tags->eof_fret_hand_pos_1_pg ? D_SELECTED : 0;
	eof_song_properties_dialog[20].flags = eof_song->tags->eof_fret_hand_pos_1_pb ? D_SELECTED : 0;
	if(eof_song->tags->difficulty != 0xFF)
	{	//If there is a band difficulty defined, populate the band difficulty field
		sprintf(eof_etext7, "%lu", eof_song->tags->difficulty);
	}
	else
	{	//Othewise leave the field blank
		eof_etext7[0] = '\0';
	}
	if(eof_popup_dialog(eof_song_properties_dialog, 2) == 21)
	{	//User clicked OK
		newlyrics = (eof_song_properties_dialog[17].flags & D_SELECTED) ? 1 : 0;
		neweighth_note_hopo = (eof_song_properties_dialog[18].flags & D_SELECTED) ? 1 : 0;
		neweof_fret_hand_pos_1_pg = (eof_song_properties_dialog[19].flags & D_SELECTED) ? 1 : 0;
		neweof_fret_hand_pos_1_pb = (eof_song_properties_dialog[20].flags & D_SELECTED) ? 1 : 0;
		//Ensure the boolean values use 1 to indicate true to simplify comparison below
		if(eof_song->tags->lyrics)
			eof_song->tags->lyrics = 1;
		if(eof_song->tags->eighth_note_hopo)
			eof_song->tags->eighth_note_hopo = 1;
		if(eof_song->tags->eof_fret_hand_pos_1_pg)
			eof_song->tags->eof_fret_hand_pos_1_pg = 1;
		if(eof_song->tags->eof_fret_hand_pos_1_pb)
			eof_song->tags->eof_fret_hand_pos_1_pb = 1;
		if(ustricmp(eof_song->tags->title, eof_etext) || ustricmp(eof_song->tags->artist, eof_etext2) || ustricmp(eof_song->tags->frettist, eof_etext3) || ustricmp(eof_song->tags->year, eof_etext5) || ustricmp(eof_song->tags->loading_text, eof_etext6))
		{	//If any of the text fields were changed
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			undo_made = 1;
		}
		else if(eof_is_number(eof_etext4) && (eof_song->tags->ogg[eof_selected_ogg].midi_offset != atol(eof_etext4)))
		{	//If the delay was changed
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			undo_made = 1;
		}
		else if((eof_song->tags->lyrics != newlyrics) || (eof_song->tags->eighth_note_hopo != neweighth_note_hopo) || (eof_song->tags->eof_fret_hand_pos_1_pg != neweof_fret_hand_pos_1_pg) || (eof_song->tags->eof_fret_hand_pos_1_pb != neweof_fret_hand_pos_1_pb))
		{	//If any of the checkboxes were changed
			undo_made = 1;
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		}
		if((ustrlen(eof_etext) > 255) || (ustrlen(eof_etext2) > 255) || (ustrlen(eof_etext3) > 255) || (ustrlen(eof_etext5) > 31) || (ustrlen(eof_etext6) > 511))
		{
			allegro_message("Text too large for allocated buffer!");
			return 1;
		}
		ustrcpy(eof_song->tags->title, eof_etext);
		ustrcpy(eof_song->tags->artist, eof_etext2);
		ustrcpy(eof_song->tags->frettist, eof_etext3);
		ustrcpy(eof_song->tags->year, eof_etext5);
		ustrcpy(eof_song->tags->loading_text, eof_etext6);
		eof_song->tags->lyrics = newlyrics;
		eof_song->tags->eighth_note_hopo = neweighth_note_hopo;
		eof_song->tags->eof_fret_hand_pos_1_pg = neweof_fret_hand_pos_1_pg;
		eof_song->tags->eof_fret_hand_pos_1_pb = neweof_fret_hand_pos_1_pb;
		ustrcpy(eof_last_frettist, eof_etext3);
		if(!eof_is_number(eof_etext4))
		{
			invalid = 1;
		}
		if(!invalid)
		{
			eof_song->tags->ogg[eof_selected_ogg].midi_offset = atol(eof_etext4);
			if(eof_song->tags->ogg[eof_selected_ogg].midi_offset < 0)
			{
				invalid = 1;
			}
		}
		if(invalid)
		{
			allegro_message("Invalid MIDI offset.");
		}
		if(eof_song->beat[0]->pos != eof_song->tags->ogg[eof_selected_ogg].midi_offset)
		{
			for(i = 0; i < eof_song->beats; i++)
			{
				eof_song->beat[i]->fpos += (double)(eof_song->tags->ogg[eof_selected_ogg].midi_offset - old_offset);
				eof_song->beat[i]->pos = eof_song->beat[i]->fpos + 0.5;	//Round up
			}
		}
		if((eof_song->tags->ogg[eof_selected_ogg].midi_offset != old_offset) && (eof_get_chart_size(eof_song) > 0))
		{	//If the MIDI offset was changed and there is at least one note/lyric in the chart, prompt to adjust the notes/lyrics
			if(alert(NULL, "Adjust notes to new offset?", NULL, "&Yes", "&No", 'y', 'n') == 1)
			{
				eof_adjust_notes(eof_song->tags->ogg[eof_selected_ogg].midi_offset - old_offset);
			}
			eof_clear_input();
		}
		if(eof_etext7[0] != '\0')
		{	//If the band difficulty field is populated
			difficulty = eof_etext7[0] - '0';	//Convert the ASCII character to its numerical value
		}
		else
		{
			difficulty = 0xFF;
		}
		if(!undo_made && (difficulty != eof_song->tags->difficulty))
		{	//If the band difficulty field was edited, make an undo state if one hasn't been made already
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			undo_made = 1;
		}
		eof_song->tags->difficulty = difficulty;	//Save the value reflected by the band difficulty field
		eof_fixup_notes(eof_song);
		eof_calculate_beats(eof_song);
		eof_fix_window_title();
	}
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(NULL);
	return 1;
}

int eof_menu_song_test(char application)
{
	char *songs_path = NULL;
	char *executablepath = NULL;
	char *executablename = NULL;
	char *fofdisplayname = "FoF";
	char *psdisplayname = "Phase Shift";
	char *appdisplayname = NULL;
	char syscommand[1024] = {0};
	char temppath[1024] = {0};
	char temppath2[1024] = {0};
	int difficulty = 0;
	int part = 0;

	if(!eof_song_loaded || !eof_song)
		return 1;	//Do not allow this function to run if a chart is not loaded

	if(application == 1)
	{	//If the user wants to test the chart in FoF
		/* check difficulty before allowing test */
		difficulty = eof_figure_difficulty();
		if(difficulty < 0)
		{
			alert("Error", NULL, "No Notes!", "OK", NULL, 0, KEY_ENTER);
			eof_clear_input();
			return 1;
		}

		/* which part are we going to play */
		part = eof_figure_part();
		if(part < 0)
		{
			alert("Error", NULL, "No Notes!", "OK", NULL, 0, KEY_ENTER);
			eof_clear_input();
			return 1;
		}

		appdisplayname = fofdisplayname;
		songs_path = eof_fof_songs_path;
		executablepath = eof_fof_executable_path;
		executablename = eof_fof_executable_name;
	}
	else
	{	//The user wants to test the chart in Phase Shift
		appdisplayname = psdisplayname;
		songs_path = eof_ps_songs_path;
		executablepath = eof_ps_executable_path;
		executablename = eof_ps_executable_name;
	}

	/* switch to songs folder */
	if(eof_chdir(songs_path))
	{
		allegro_message("Song could not be tested!\nMake sure you set the %s song folder correctly (\"Link To %s\")!", appdisplayname, appdisplayname);
		return 1;
	}

	/* create temporary song folder in library */
	eof_mkdir("EOFTemp");

	/* save temporary song */
	ustrcpy(temppath, songs_path);
	ustrcat(temppath, "EOFTemp\\");
	append_filename(temppath2, temppath, "notes.eof", 1024);
	eof_sort_notes(eof_song);
	eof_fixup_notes(eof_song);
	if(!eof_save_song(eof_song, temppath2))
	{
		allegro_message("Song could not be tested!\nMake sure you set the %s song folder correctly (\"Link To %s\")!", appdisplayname, appdisplayname);
		get_executable_name(temppath, 1024);
		replace_filename(temppath, temppath, "", 1024);
		eof_chdir(temppath);
		eof_show_mouse(NULL);
		eof_cursor_visible = 1;
		eof_pen_visible = 1;
		return 1;
	}
	append_filename(temppath2, temppath, "notes.mid", 1024);
	eof_export_midi(eof_song, temppath2, 0, 1, 1);	//Export the temporary MIDI, make pitchless/phraseless lyric corrections automatically
	append_filename(temppath2, temppath, "song.ini", 1024);
	eof_save_ini(eof_song, temppath2);
	sprintf(syscommand, "%sguitar.ogg", eof_song_path);
	sprintf(temppath2, "%sEOFTemp\\guitar.ogg", songs_path);
	eof_copy_file(syscommand, temppath2);

	/* switch to application folder */
	replace_filename(temppath, executablepath, "", 1024);
	if(eof_chdir(temppath))
	{
		allegro_message("Song could not be tested!\nMake sure you set the %s song folder correctly (\"Link To %s\")!", appdisplayname, appdisplayname);
		return 1;
	}

	/* execute appropriate application to launch chart */
	if(application == 1)
	{	//If the user wants to test the chart in FoF
		ustrcpy(syscommand, executablename);
		ustrcat(syscommand, " -p \"EOFTemp\" -D ");
		sprintf(temppath, "%d", difficulty);
		ustrcat(syscommand, temppath);
		ustrcat(syscommand, " -P ");
		sprintf(temppath, "%d", part);
		ustrcat(syscommand, temppath);
		eof_system(syscommand);
	}
	else
	{	//The user wants to test the chart in Phase Shift
		FILE *phaseshiftfp = fopen("launch_ps.bat", "wt");	//Write a batch file to launch Phase Shift

		if(phaseshiftfp)
		{
			fprintf(phaseshiftfp, "cd %s\n", temppath);
			replace_filename(temppath, temppath2, "", 1024);	//Get the path to the temporary chart's folder
			if(temppath[strlen(temppath)-1] == '\\')
			{	//Remove the trailing backslash because Phase Shift doesn't handle it correctly
				temppath[strlen(temppath)-1] = '\0';
			}
			sprintf(syscommand, "\"%s\" \"%s\" /p", executablepath, temppath);
			fprintf(phaseshiftfp, "%s", syscommand);
			fclose(phaseshiftfp);
			eof_system("launch_ps.bat");
			delete_file("launch_ps.bat");
		}
	}

	/* switch to songs folder */
	if(eof_chdir(songs_path))
	{
		allegro_message("Cleanup failed!");
		return 1;
	}

	/* delete temporary song folder */
	delete_file("EOFTemp\\guitar.ogg");
	delete_file("EOFTemp\\notes.mid");
	delete_file("EOFTemp\\song.ini");
	delete_file("EOFTemp\\notes.eof");
	eof_system("rd /s /q EOFTemp");

	/* switch back to EOF folder */
	get_executable_name(temppath, 1024);
	replace_filename(temppath, temppath, "", 1024);
	eof_chdir(temppath);

	return 1;
}

int eof_menu_song_test_fof(void)
{
	return eof_menu_song_test(1);	//Launch the chart in FoF
}

int eof_menu_song_test_ps(void)
{
	return eof_menu_song_test(2);	//Launch the chart in Phase Shift
}

int eof_menu_track_selected_1(void)
{
	return eof_menu_track_selected_track_number(1);
}

int eof_menu_track_selected_2(void)
{
	return eof_menu_track_selected_track_number(2);
}

int eof_menu_track_selected_3(void)
{
	return eof_menu_track_selected_track_number(3);
}

int eof_menu_track_selected_4(void)
{
	return eof_menu_track_selected_track_number(4);
}

int eof_menu_track_selected_5(void)
{
	return eof_menu_track_selected_track_number(5);
}

int eof_menu_track_selected_6(void)
{
	return eof_menu_track_selected_track_number(6);
}

int eof_menu_track_selected_7(void)
{
	return eof_menu_track_selected_track_number(7);
}

int eof_menu_track_selected_8(void)
{
	return eof_menu_track_selected_track_number(8);
}

int eof_menu_track_selected_9(void)
{
	return eof_menu_track_selected_track_number(9);
}

int eof_menu_track_selected_10(void)
{
	return eof_menu_track_selected_track_number(10);
}

int eof_menu_track_selected_11(void)
{
	return eof_menu_track_selected_track_number(11);
}

int eof_menu_track_selected_12(void)
{
	return eof_menu_track_selected_track_number(12);
}

int eof_menu_track_selected_track_number(int tracknum)
{
	eof_log("\tChanging active track", 1);
	eof_log("eof_menu_track_selected_track_number() entered", 1);

	unsigned long i;

	if((tracknum > 0) && (tracknum < eof_song->tracks))
	{
		for(i = 0; i < EOF_TRACKS_MAX; i++)
		{
			eof_track_selected_menu[i].flags = 0;
		}

		if(eof_song->track[tracknum]->track_format == EOF_VOCAL_TRACK_FORMAT)
		{
			eof_vocals_selected = 1;
			eof_note_type = 0;	//For now, the default lyric set (PART VOCALS) will be selected when changing to PART VOCALS
		}
		else
			eof_vocals_selected = 0;

		//Track numbering begins at one instead of zero
		eof_track_selected_menu[tracknum-1].flags = D_SELECTED;
		eof_selected_track = tracknum;
		eof_detect_difficulties(eof_song);
		eof_fix_window_title();
		eof_scale_fretboard(0);			//Recalculate the 2D screen positioning based on the current track
		eof_set_3D_lane_positions(0);
		eof_set_2D_lane_positions(0);
		eof_determine_phrase_status(eof_selected_track);
		eof_chord_lookup_note = 0;	//Reset the cached chord lookup count
	}
	eof_set_color_set();
	return 1;
}

char * eof_ini_list(int index, int * size)
{
	short i;
	int ecount = 0;
	char * etextpointer[EOF_MAX_INI_SETTINGS] = {NULL};

	if(eof_song->tags->ini_settings >= EOF_MAX_INI_SETTINGS)	//If the maximum number of settings has been met or exceeded
		eof_ini_dialog[2].flags = D_DISABLED;	//Disable the "Add" Song INI dialog button
	else
		eof_ini_dialog[2].flags = 0;	//Enable the "Add" Song INI dialog button

	for(i = 0; i < eof_song->tags->ini_settings; i++)
	{
		if(ecount < EOF_MAX_INI_SETTINGS)
		{
			etextpointer[ecount] = eof_song->tags->ini_setting[i];
			ecount++;
		}
	}
	switch(index)
	{
		case -1:
		{
			*size = ecount;
			if(ecount > 0)
			{
				eof_ini_dialog[3].flags = 0;
			}
			else
			{
				eof_ini_dialog[3].flags = D_DISABLED;	//Disable the "Delete" Song INI dialog button if there are no settings
			}
			break;
		}
		default:
		{
			return etextpointer[index];
		}
	}
	return NULL;
}

int eof_menu_catalog_show(void)
{
	if(eof_song->catalog->entries > 0)
	{
		if(eof_catalog_menu[0].flags & D_SELECTED)
		{
			eof_catalog_menu[0].flags = 0;
		}
		else
		{
			eof_catalog_menu[0].flags = D_SELECTED;
			if((eof_song->catalog->entries > 0) && !eof_music_catalog_playback)
			{
				eof_music_catalog_pos = eof_song->catalog->entry[eof_selected_catalog_entry].start_pos + eof_av_delay;
			}
		}
	}
	else
	{
		eof_catalog_menu[0].flags = 0;
	}
	return 1;
}

int eof_menu_catalog_add(void)
{
	long first_pos = -1;
	long last_pos = -1;
	unsigned long i;
	long next;

	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{
			if(first_pos == -1)
			{
				first_pos = eof_get_note_pos(eof_song, eof_selected_track, i);
			}
			if(eof_get_note_length(eof_song, eof_selected_track, i) < 100)
			{
				last_pos = eof_get_note_pos(eof_song, eof_selected_track, i) + 100;
				next = eof_track_fixup_next_note(eof_song, eof_selected_track, i);
				if(next >= 0)
				{
					if(last_pos >= eof_get_note_pos(eof_song, eof_selected_track, next))
					{
						last_pos = eof_get_note_pos(eof_song, eof_selected_track, next) - 1;
					}
				}
			}
			else
			{
				last_pos = eof_get_note_pos(eof_song, eof_selected_track, i) + eof_get_note_length(eof_song, eof_selected_track, i);
			}
		}
	}
	if((eof_song->catalog->entry[eof_song->catalog->entries].start_pos != -1) && (eof_song->catalog->entry[eof_song->catalog->entries].end_pos != -1))
	{
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		eof_track_add_section(eof_song, 0, EOF_FRET_CATALOG_SECTION, eof_note_type, first_pos, last_pos, eof_selected_track, NULL);
		eof_music_catalog_pos = eof_song->catalog->entry[eof_selected_catalog_entry].start_pos + eof_av_delay;
	}

	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_catalog_delete(void)
{
	unsigned long i;

	if(eof_song->catalog->entries > 0)
	{
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		for(i = eof_selected_catalog_entry; i < eof_song->catalog->entries - 1; i++)
		{
			memcpy(&eof_song->catalog->entry[i], &eof_song->catalog->entry[i + 1], sizeof(EOF_CATALOG_ENTRY));
		}
		eof_song->catalog->entries--;
		if((eof_selected_catalog_entry >= eof_song->catalog->entries) && (eof_selected_catalog_entry > 0))
		{
			eof_selected_catalog_entry--;
		}
		eof_music_catalog_pos = eof_song->catalog->entry[eof_selected_catalog_entry].start_pos + eof_av_delay;
		if(eof_song->catalog->entries <= 0)
		{
			eof_catalog_menu[0].flags = 0;
		}
	}
	return 1;
}

int eof_menu_catalog_previous(void)
{
	if((eof_song->catalog->entries > 0) && !eof_music_catalog_playback)
	{
		if(eof_selected_catalog_entry == 0)
		{	//Wrap around
			eof_selected_catalog_entry = eof_song->catalog->entries - 1;
		}
		else
		{
			eof_selected_catalog_entry--;
		}
		eof_music_catalog_pos = eof_song->catalog->entry[eof_selected_catalog_entry].start_pos + eof_av_delay;
	}
	return 1;
}

int eof_menu_catalog_next(void)
{
	if((eof_song->catalog->entries > 0) && !eof_music_catalog_playback)
	{
		eof_selected_catalog_entry++;
		if(eof_selected_catalog_entry >= eof_song->catalog->entries)
		{
			eof_selected_catalog_entry = 0;
		}
		eof_music_catalog_pos = eof_song->catalog->entry[eof_selected_catalog_entry].start_pos + eof_av_delay;
	}
	return 1;
}

int eof_ini_dialog_add(DIALOG * d)
{
	int i;

	if(eof_song->tags->ini_settings >= EOF_MAX_INI_SETTINGS)	//If the maximum number of INI settings is already defined
		return D_O_K;	//Return without adding anything

	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_ini_add_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_ini_add_dialog);
	ustrcpy(eof_etext, "");

	if(eof_popup_dialog(eof_ini_add_dialog, 2) == 3)
	{
		if((ustrlen(eof_etext) > 0) && eof_check_string(eof_etext))
		{
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			ustrcpy(eof_song->tags->ini_setting[eof_song->tags->ini_settings], eof_etext);
			eof_song->tags->ini_settings++;
		}
	}

	dialog_message(eof_ini_dialog, MSG_DRAW, 0, &i);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(screen);
	return D_O_K;
}

int eof_ini_dialog_delete(DIALOG * d)
{
	int i;

	if(eof_song->tags->ini_settings > 0)
	{
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		for(i = eof_ini_dialog[1].d1; i < eof_song->tags->ini_settings - 1; i++)
		{
			memcpy(eof_song->tags->ini_setting[i], eof_song->tags->ini_setting[i + 1], 512);
		}
		eof_song->tags->ini_settings--;
		if(eof_ini_dialog[1].d1 >= eof_song->tags->ini_settings)
		{
			eof_ini_dialog[1].d1--;
		}
	}
	dialog_message(eof_ini_dialog, MSG_DRAW, 0, &i);
	return D_O_K;
}

int eof_menu_song_waveform(void)
{
	if(!eof_song_loaded)
		return 1;	//Return error

	if(eof_display_waveform == 0)
	{
		if(eof_music_paused)
		{	//Don't try to generate the waveform data if the chart is playing
			if(eof_waveform == NULL)
			{
				eof_waveform = eof_create_waveform(eof_loaded_ogg_name,1);	//Generate 1ms waveform data from the current audio file
			}
			else if(ustricmp(eof_waveform->oggfilename,eof_loaded_ogg_name) != 0)
			{	//If the user opened a different OGG file since the waveform data was generated
				eof_destroy_waveform(eof_waveform);
				eof_waveform = eof_create_waveform(eof_loaded_ogg_name,1);	//Generate 1ms waveform data from the current audio file
			}
		}

		if(eof_waveform != NULL)
		{
			eof_display_waveform = 1;
			eof_waveform_menu[0].flags = D_SELECTED;	//Check the Show item in the Song>Waveform graph menu
		}
	}
	else
	{
		eof_display_waveform = 0;
		eof_waveform_menu[0].flags = 0;	//Clear the Show item in the Song>Waveform graph menu
	}

	if(eof_music_paused)
		eof_render();
	eof_fix_window_title();

	return 0;	//Return success
}

DIALOG eof_leading_silence_dialog[] =
{
   /* (proc) 		        (x)	(y)	(w)	(h)	(fg) (bg) (key) (flags)	(d1)(d2)(dp)						(dp2) (dp3) */
   { d_agup_window_proc,  	  0,	 48,200,288,2,   23,  0,    0,      0,	0,	"Leading Silence",          NULL, NULL },
   { d_agup_text_proc      , 16,     80,110,20, 2,   23,  0,    0,      0,  0,  "Add:",                      NULL, NULL },
   { d_agup_radio_proc,		 16,	100,110,15,	2,   23,  0,    0,      0,	0,	"Milliseconds",             NULL, NULL },
   { d_agup_radio_proc,		 16,	120,110,15,	2,   23,  0,    0,      0,	0,	"Beats",                    NULL, NULL },
   { d_agup_text_proc      , 16,    140,110,20, 2,   23,  0,    0,      0,  0,  "Pad:",                      NULL, NULL },
   { d_agup_radio_proc,		 16,	160,110,15,	2,   23,  0,    0,      0,	0,	"Milliseconds",	            NULL, NULL },
   { d_agup_radio_proc,		 16,	180,110,15,	2,   23,  0,    0,      0,	0,	"Beats",                    NULL, NULL },
   { eof_verified_edit_proc, 16,    200,110,20, 2,   23,  0,    0,    255,  0,   eof_etext,         "1234567890", NULL },
   { d_agup_check_proc,		  16,	226,180,16,	2,   23,  0,    D_SELECTED,		1,	0,	"Adjust Notes/Beats",		NULL, NULL },
   { d_agup_radio_proc,		 16,	246,160,15,	2,   23,  0,    0,      1,	0,	"Stream copy (oggCat)",     NULL, NULL },
   { d_agup_radio_proc,		 16,	266, 90,15,	2,   23,  0,    0,      1,	0,	"Re-encode",                NULL, NULL },
   { d_agup_button_proc,	  16,	292, 68,28,	2,   23,  '\r',	D_EXIT, 0,	0,	"OK",             			NULL, NULL },
   { d_agup_button_proc,	 116,   292,68, 28,	2,   23,  0,	D_EXIT, 0,	0,	"Cancel",         			NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

static long get_ogg_length(const char * fn)
{
	ALOGG_OGG * ogg;
	unsigned long length = 0;
	FILE * fp = fopen(fn, "rb");
	if(fp)
	{
		ogg = alogg_create_ogg_from_file(fp);
		length = alogg_get_length_msecs_ogg(ogg);
		alogg_destroy_ogg(ogg);
	}
	return length;
}

int eof_menu_song_add_silence(void)
{
	double beat_length;
	unsigned long silence_length = 0;
	long current_length = 0;
	long after_silence_length = 0;
	long adjust = 0;
	unsigned long i, x;
	char fn[1024] = {0};
	char mp3fn[1024] = {0};
	static int creationmethod = 9;	//Stores the user's last selected leading silence creation method (default to oggCat, which is menu item 9 in eof_leading_silence_dialog[])

	eof_leading_silence_dialog[9].flags = 0;
	eof_leading_silence_dialog[10].flags = 0;
	if(eof_supports_oggcat == 0)
	{	//If EOF has not found oggCat to be available, disable it in the menu and select re-encode
		creationmethod = 10;	//eof_leading_silence_dialog[10] is the re-encode option
		eof_leading_silence_dialog[9].flags = D_DISABLED;	//Disable the stream copy option
	}

	eof_leading_silence_dialog[creationmethod].flags = D_SELECTED;	//Select the last selected creation method

	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	eof_color_dialog(eof_leading_silence_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_leading_silence_dialog);

	for(x = 1; x <= 4; x++)
	{
		eof_leading_silence_dialog[x].flags = 0;
	}
	eof_leading_silence_dialog[2].flags = D_SELECTED;
	eof_leading_silence_dialog[3].flags = 0;
	eof_leading_silence_dialog[5].flags = 0;
	eof_leading_silence_dialog[6].flags = 0;
	sprintf(eof_etext, "0");

	if(eof_popup_dialog(eof_leading_silence_dialog, 0) == 11)			//User clicked OK
	{
		sprintf(fn, "%s.backup", eof_loaded_ogg_name);
		current_length = get_ogg_length(eof_loaded_ogg_name);
		/* revert to original file */
		if(atoi(eof_etext) <= 0)
		{
			if(eof_file_compare(fn, eof_loaded_ogg_name) == 0)
			{	//If the backup audio is already loaded (the loaded file is an exact copy of the backup)
				return 1;	//Return without making any changes
			}
			if(exists(fn))
			{	//Only attempt to restore the original audio if the backup exists
				eof_prepare_undo(EOF_UNDO_TYPE_SILENCE);
				eof_copy_file(fn, eof_loaded_ogg_name);
				if(eof_load_ogg(eof_loaded_ogg_name))
				{
					eof_fix_waveform_graph();
					eof_fix_window_title();
				}
			}
			else
			{
				return 1;	//Return without making any changes
			}
		}
		else
		{
			eof_prepare_undo(EOF_UNDO_TYPE_SILENCE);
			if(eof_leading_silence_dialog[2].flags & D_SELECTED)
			{
				silence_length = atoi(eof_etext);
			}
			else if(eof_leading_silence_dialog[3].flags & D_SELECTED)
			{
				beat_length = (double)60000 / ((double)60000000.0 / (double)eof_song->beat[0]->ppqn);
				silence_length = beat_length * (double)atoi(eof_etext);
			}
			else if(eof_leading_silence_dialog[5].flags & D_SELECTED)
			{
				silence_length = atoi(eof_etext);
				if(silence_length > eof_song->beat[0]->pos)
				{
					silence_length -= eof_song->beat[0]->pos;
				}
			}
			else if(eof_leading_silence_dialog[6].flags & D_SELECTED)
			{
				beat_length = (double)60000 / ((double)60000000.0 / (double)eof_song->beat[0]->ppqn);
				silence_length = beat_length * (double)atoi(eof_etext);
				printf("%lu\n", silence_length);
				if(silence_length > eof_song->beat[0]->pos)
				{
					silence_length -= eof_song->beat[0]->pos;
				}
			}

			if((eof_leading_silence_dialog[9].flags == D_SELECTED) && eof_supports_oggcat)
			{	//User opted to use oggCat
				creationmethod = 9;		//Remember this as the default next time
				eof_add_silence(eof_loaded_ogg_name, silence_length);
			}
			else
			{	//User opted to re-encode
				replace_filename(mp3fn, eof_song_path, "original.mp3", 1024);
				if(exists(mp3fn))
				{
					creationmethod = 10;	//Remember this as the default next time
					eof_add_silence_recode_mp3(eof_loaded_ogg_name, silence_length);
				}
				else
				{
					creationmethod = 10;	//Remember this as the default next time
					eof_add_silence_recode(eof_loaded_ogg_name, silence_length);
				}
			}
			after_silence_length = get_ogg_length(eof_loaded_ogg_name);
			eof_song->tags->ogg[eof_selected_ogg].modified = 1;
		}

		/* adjust notes/beats */
		if(eof_leading_silence_dialog[8].flags & D_SELECTED)
		{
			if(after_silence_length != 0)
			{
				adjust = after_silence_length - current_length;
			}
			else
			{
				adjust = get_ogg_length(fn) - current_length;
			}
			if(eof_song->tags->ogg[eof_selected_ogg].midi_offset + adjust < 0)
			{
				adjust = 0;
			}
			eof_song->tags->ogg[eof_selected_ogg].midi_offset += adjust;
			if(eof_song->beat[0]->pos != eof_song->tags->ogg[eof_selected_ogg].midi_offset)
			{
				for(i = 0; i < eof_song->beats; i++)
				{
					eof_song->beat[i]->fpos += (double)adjust;
					eof_song->beat[i]->pos = eof_song->beat[i]->fpos + 0.5;	//Round up
				}
			}
			eof_adjust_notes(adjust);
		}
		eof_fixup_notes(eof_song);
		eof_calculate_beats(eof_song);
		eof_fix_window_title();
	}
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	return 1;
}

char eof_chart_volume_string[10] = "100%";
char eof_clap_volume_string[10] = "100%";
char eof_metronome_volume_string[10] = "100%";
char eof_tone_volume_string[10] = "100%";
char eof_percussion_volume_string[10] = "100%";
DIALOG eof_audio_cues_dialog[] =
{
/*	(proc)					(x)	(y)				(w)	(h)		(fg)	(bg)	(key)	(flags)	(d1)	(d2)	(dp)							(dp2)	(dp3) */
	{ d_agup_window_proc,	0,	48,				310,310,	2,		23,		0,		0,		0,		0,		"Audio cues",					NULL,	NULL },
	{ d_agup_text_proc,		16,	88,				64,	8,		2,		23,		0,		0,		0,		0,		"Chart volume",					NULL,	NULL },
	{ d_agup_slider_proc,	176,88,				96,	16,		2,		23,		0,		0,		100,	0,		NULL,							eof_set_cue_volume,	eof_chart_volume_string },
	{ d_agup_text_proc,		275,88,				30,	16,		2,		23,		0,		0,		100,	0,		eof_chart_volume_string,		NULL,	NULL },
	{ d_agup_text_proc,		16,	108,			64,	8,		2,		23,		0,		0,		0,		0,		"Clap volume",					NULL,	NULL },
	{ d_agup_slider_proc,	176,108,			96,	16,		2,		23,		0,		0,		100,	0,		NULL,							eof_set_cue_volume,	eof_clap_volume_string },
	{ d_agup_text_proc,		275,108,			30,	16,		2,		23,		0,		0,		100,	0,		eof_clap_volume_string,			NULL,	NULL },
	{ d_agup_text_proc,		16, 128,			64,	8,		2,		23,		0,		0,		0,		0,		"Metronome volume",				NULL,	NULL },
	{ d_agup_slider_proc,	176,128,			96,	16,		2,		23,		0,		0,		100,	0,		NULL,							eof_set_cue_volume,	eof_metronome_volume_string },
	{ d_agup_text_proc,		275,128,			30,	16,		2,		23,		0,		0,		100,	0,		eof_metronome_volume_string,	NULL,	NULL },
	{ d_agup_text_proc,		16, 148,			64,	8,		2,		23,		0,		0,		0,		0,		"Tone volume",					NULL,	NULL },
	{ d_agup_slider_proc,	176,148,			96,	16,		2,		23,		0,		0,		100,	0,		NULL,							eof_set_cue_volume,	eof_tone_volume_string },
	{ d_agup_text_proc,		275,148,			30,	16,		2,		23,		0,		0,		100,	0,		eof_tone_volume_string,			NULL,	NULL },
	{ d_agup_text_proc,		16, 168,			64,	8,		2,		23,		0,		0,		0,		0,		"Vocal Percussion volume",		NULL,	NULL },
	{ d_agup_slider_proc,	176,168,			96,	16,		2,		23,		0,		0,		100,	0,		NULL,							eof_set_cue_volume,	eof_percussion_volume_string },
	{ d_agup_text_proc,		275,168,			30,	16,		2,		23,		0,		0,		100,	0,		eof_percussion_volume_string,	NULL,	NULL },
	{ d_agup_text_proc,		16, 188,			64,	8,		2,		23,		0,		0,		0,		0,		"Vocal Percussion sound:",		NULL,	NULL },
	{ d_agup_radio_proc,	16,208,				68,	15,		2,		23,		0,		0,		0,		0,		"Cowbell",						NULL,	NULL },
	{ d_agup_radio_proc,	124,208,			84,	15,		2,		23,		0,		0,		0,		0,		"Triangle 1",					NULL,	NULL },
	{ d_agup_radio_proc,	210,208,			30,	15,		2,		23,		0,		0,		0,		0,		"2",							NULL,	NULL },
	{ d_agup_radio_proc,	16,228,				102,15,		2,		23,		0,		0,		0,		0,		"Tambourine 1",					NULL,	NULL },
	{ d_agup_radio_proc,	124,228,			30,	15,		2,		23,		0,		0,		0,		0,		"2",							NULL,	NULL },
	{ d_agup_radio_proc,	160,228,			30,	15,		2,		23,		0,		0,		0,		0,		"3",							NULL,	NULL },
	{ d_agup_radio_proc,	16,248,				110,15,		2,		23,		0,		0,		0,		0,		"Wood Block 1",					NULL,	NULL },
	{ d_agup_radio_proc,	124,248,			30,	15,		2,		23,		0,		0,		0,		0,		"2",							NULL,	NULL },
	{ d_agup_radio_proc,	160,248,			30,	15,		2,		23,		0,		0,		0,		0,		"3",							NULL,	NULL },
	{ d_agup_radio_proc,	196,248,			30,	15,		2,		23,		0,		0,		0,		0,		"4",							NULL,	NULL },
	{ d_agup_radio_proc,	16,268,				30,	15,		2,		23,		0,		0,		0,		0,		"5",							NULL,	NULL },
	{ d_agup_radio_proc,	52,268,				30,	15,		2,		23,		0,		0,		0,		0,		"6",							NULL,	NULL },
	{ d_agup_radio_proc,	88,268,				30,	15,		2,		23,		0,		0,		0,		0,		"7",							NULL,	NULL },
	{ d_agup_radio_proc,	124,268,			30,	15,		2,		23,		0,		0,		0,		0,		"8",							NULL,	NULL },
	{ d_agup_radio_proc,	160,268,			30,	15,		2,		23,		0,		0,		0,		0,		"9",							NULL,	NULL },
	{ d_agup_radio_proc,	196,268,			40,	15,		2,		23,		0,		0,		0,		0,		"10",							NULL,	NULL },
	{ d_agup_radio_proc,	16,288,				68,15,		2,		23,		0,		0,		0,		0,		"Clap 1",						NULL,	NULL },
	{ d_agup_radio_proc,	88,288,				30,15,		2,		23,		0,		0,		0,		0,		"2",							NULL,	NULL },
	{ d_agup_radio_proc,	124,288,			30,15,		2,		23,		0,		0,		0,		0,		"3",							NULL,	NULL },
	{ d_agup_radio_proc,	160,288,			30,15,		2,		23,		0,		0,		0,		0,		"4",							NULL,	NULL },
	{ d_agup_button_proc,	44,314,				68,	28,		2,		23,		'\r',	D_EXIT,	0,		0,		"OK",							NULL,	NULL },
	{ d_agup_button_proc,	172,314,			68,	28,		2,		23,		0,		D_EXIT,	0,		0,		"Cancel",						NULL,	NULL },
	{ NULL,					0,	0,				0,	0,		0,		0,		0,		0,		0,		0,		NULL,							NULL,	NULL }
};

int eof_set_cue_volume(void *dp3, int d2)
{
	//Validate input
	if(dp3 == NULL)
		return 1;
	if((d2 < 0) || (d2 > 100))
		return 1;

	sprintf((char *)dp3,"%3d%%",d2);	//Rewrite the specified volume slider string
	object_message(&eof_audio_cues_dialog[3], MSG_DRAW, 0);	//Have Allegro redraw the volume slider strings
	object_message(&eof_audio_cues_dialog[6], MSG_DRAW, 0);
	object_message(&eof_audio_cues_dialog[9], MSG_DRAW, 0);
	object_message(&eof_audio_cues_dialog[12], MSG_DRAW, 0);
	object_message(&eof_audio_cues_dialog[15], MSG_DRAW, 0);
	return 0;
}

int eof_menu_audio_cues(void)
{
	unsigned long x;

	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	eof_color_dialog(eof_audio_cues_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_audio_cues_dialog);
	eof_audio_cues_dialog[2].d2 = eof_chart_volume;
	eof_audio_cues_dialog[5].d2 = eof_clap_volume;
	eof_audio_cues_dialog[8].d2 = eof_tick_volume;
	eof_audio_cues_dialog[11].d2 = eof_tone_volume;
	eof_audio_cues_dialog[14].d2 = eof_percussion_volume;

	for(x = 17; x <= 36; x++)
	{	//Deselect all vocal percussion radio buttons
		eof_audio_cues_dialog[x].flags = 0;
	}
	eof_audio_cues_dialog[eof_selected_percussion_cue].flags = D_SELECTED;	//Activate the radio button for the current vocal percussion cue

	//Rebuild the volume slider strings, as they are not guaranteed to be all 100% on launch of EOF since EOF stores the user's last-configured volumes in the config file
	eof_set_cue_volume(eof_chart_volume_string, eof_chart_volume);
	eof_set_cue_volume(eof_clap_volume_string, eof_clap_volume);
	eof_set_cue_volume(eof_metronome_volume_string, eof_tick_volume);
	eof_set_cue_volume(eof_tone_volume_string, eof_tone_volume);
	eof_set_cue_volume(eof_percussion_volume_string, eof_percussion_volume);

	if(eof_popup_dialog(eof_audio_cues_dialog, 0) == 37)			//User clicked OK
	{
		eof_chart_volume = eof_audio_cues_dialog[2].d2;				//Store the volume set by the chart volume slider
		eof_chart_volume_multiplier = sqrt(eof_chart_volume/100.0);	//Store this math so it only needs to be performed once
		eof_clap_volume = eof_audio_cues_dialog[5].d2;				//Store the volume set by the clap cue volume slider
		eof_tick_volume = eof_audio_cues_dialog[8].d2;				//Store the volume set by the tick cue volume slider
		eof_tone_volume = eof_audio_cues_dialog[11].d2;				//Store the volume set by the tone cue volume slider
		eof_percussion_volume = eof_audio_cues_dialog[14].d2;		//Store the volume set by the vocal percussion cue volume slider

		for(x = 17; x <= 36; x++)
		{	//Search for the selected vocal percussion cue
			if(eof_audio_cues_dialog[x].flags == D_SELECTED)
			{
				eof_set_percussion_cue(x);
				break;
			}
		}
	}
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	return 1;
}

DIALOG eof_waveform_settings_dialog[] =
{
   /* (proc) 		        (x)	(y)	(w)	(h)	(fg) (bg) (key) (flags)	(d1)(d2)(dp)						(dp2) (dp3) */
   { d_agup_window_proc,  	0,	48,	200,200,2,   23,  0,    0,      0,	0,	"Configure Waveform Graph",	NULL, NULL },
   { d_agup_text_proc,		16,	80,	64,	8,	2,   23,  0,    0,      0,	0,	"Fit into:",				NULL, NULL },
   { d_agup_radio_proc,		16,	100,110,15,	2,   23,  0,    0,      0,	0,	"Fretboard area",			NULL, NULL },
   { d_agup_radio_proc,		16,	120,110,15,	2,   23,  0,    0,      0,	0,	"Editor window",			NULL, NULL },
   { d_agup_text_proc,		16,	140,80,16,	2,   23,  0,    0,		1,	0,	"Display channels:",		NULL, NULL },
   { d_agup_check_proc,		16,	160,45,16,	2,   23,  0,    0,		1,	0,	"Left",						NULL, NULL },
   { d_agup_check_proc,		16,	180,55,16,	2,   23,  0,    0,		1,	0,	"Right",					NULL, NULL },
   { d_agup_button_proc,	16,	208,68,	28,	2,   23,  '\r',	D_EXIT, 0,	0,	"OK",             			NULL, NULL },
   { d_agup_button_proc,	116,208,68,	28,	2,   23,  0,	D_EXIT, 0,	0,	"Cancel",         			NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

int eof_menu_song_waveform_settings(void)
{
	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	eof_color_dialog(eof_waveform_settings_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_waveform_settings_dialog);

	eof_waveform_settings_dialog[2].flags = eof_waveform_settings_dialog[3].flags = 0;
	if(eof_waveform_renderlocation == 0)
	{	//If fit into fretboard is active
		eof_waveform_settings_dialog[2].flags = D_SELECTED;
	}
	else
	{	//If fit into editor window is active
		eof_waveform_settings_dialog[3].flags = D_SELECTED;
	}

	eof_waveform_settings_dialog[5].flags = eof_waveform_settings_dialog[6].flags = 0;
	if(eof_waveform_renderleftchannel)
	{	//If the left channel is selected to be rendered
		eof_waveform_settings_dialog[5].flags = D_SELECTED;
	}
	if(eof_waveform_renderrightchannel)
	{	//If the right channel is selected to be rendered
		eof_waveform_settings_dialog[6].flags = D_SELECTED;
	}

	if(eof_popup_dialog(eof_waveform_settings_dialog, 0) == 7)		//User clicked OK
	{
		if(eof_waveform_settings_dialog[2].flags == D_SELECTED)
		{	//User selected to render into fretboard area
			eof_waveform_renderlocation = 0;
		}
		else
		{	//User selected to render into editor window
			eof_waveform_renderlocation = 1;
		}
		if(eof_waveform_settings_dialog[5].flags == D_SELECTED)
		{	//User selected to render the left channel
			eof_waveform_renderleftchannel = 1;
		}
		else
		{
			eof_waveform_renderleftchannel = 0;
		}
		if(eof_waveform_settings_dialog[6].flags == D_SELECTED)
		{	//User selected to render the right channel
			eof_waveform_renderrightchannel = 1;
		}
		else
		{
			eof_waveform_renderrightchannel = 0;
		}
	}
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	return 1;
}

int eof_menu_song_open_bass(void)
{
	unsigned long tracknum = eof_song->track[EOF_TRACK_BASS]->tracknum;
	unsigned long ctr;
	char undo_made = 0;	//Set to nonzero if an undo state was saved

	if(eof_open_bass_enabled())
	{	//Turn off open bass notes
		eof_song->track[EOF_TRACK_BASS]->flags &= ~(EOF_TRACK_FLAG_SIX_LANES);	//Clear the flag
		eof_song->legacy_track[tracknum]->numlanes = 5;
	}
	else
	{	//Turn on open bass notes
		//Examine existing notes to ensure that lanes don't have to be erased for notes that use open bass strumming
		for(ctr = 0; ctr < eof_song->legacy_track[tracknum]->notes; ctr++)
		{	//For each note in PART BASS
			if((eof_song->legacy_track[tracknum]->note[ctr]->note & 32) && (eof_song->legacy_track[tracknum]->note[ctr]->note & ~32))
			{	//If this note uses lane 6 (open bass) and at least one other lane
				eof_cursor_visible = 0;
				eof_pen_visible = 0;
				eof_show_mouse(screen);
				if(alert(NULL, "Warning: Open bass strum notes must have other lanes cleared to enable open bass.  Continue?", NULL, "&Yes", "&No", 'y', 'n') == 2)
				{	//If user opts cancel the save
					eof_show_mouse(NULL);
					eof_cursor_visible = 1;
					eof_pen_visible = 1;
					return 1;	//Return cancellation
				}
				break;
			}
		}
		eof_show_mouse(NULL);
		eof_cursor_visible = 1;
		eof_pen_visible = 1;

		for(ctr = 0; ctr < eof_song->legacy_track[tracknum]->notes; ctr++)
		{	//For each note in PART BASS
			if((eof_song->legacy_track[tracknum]->note[ctr]->note & 32) && (eof_song->legacy_track[tracknum]->note[ctr]->note & ~32))
			{	//If this note uses lane 6 (open bass) and at least one other lane
				if(!undo_made)
				{
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Create an undo state before making the first change
					undo_made = 1;
				}
				eof_song->legacy_track[tracknum]->note[ctr]->note = 32;	//Clear all lanes for this note except for lane 6 (open bass)
			}
		}
		eof_song->track[EOF_TRACK_BASS]->flags |= EOF_TRACK_FLAG_SIX_LANES;	//Set the flag
		eof_song->legacy_track[tracknum]->numlanes = 6;
	}
	eof_scale_fretboard(0);	//Recalculate the 2D screen positioning based on the current track
	return 1;
}

DIALOG eof_catalog_entry_name_dialog[] =
{
   /* (proc)         (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)           (dp2) (dp3) */
   { d_agup_window_proc,    0,  48,  204 + 110, 106, 2,   23,  0,    0,      0,   0,   "Edit catalog entry name",               NULL, NULL },
   { d_agup_text_proc,   12,  84,  64,  8,  2,   23,  0,    0,      0,   0,   "Text:",         NULL, NULL },
   { d_agup_edit_proc,   48, 80,  144 + 110,  20,  2,   23,  0,    0,      EOF_NAME_LENGTH,   0,   eof_etext,           NULL, NULL },
   { d_agup_button_proc, 12 + 55,  112, 84,  28, 2,   23,  '\r',    D_EXIT, 0,   0,   "OK",               NULL, NULL },
   { d_agup_button_proc, 108 + 55, 112, 78,  28, 2,   23,  0,    D_EXIT, 0,   0,   "Cancel",           NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

int eof_menu_catalog_edit_name(void)
{
	if((eof_song->catalog->entries > 0) && (eof_selected_catalog_entry < eof_song->catalog->entries) && !eof_music_catalog_playback)
	{
		eof_cursor_visible = 0;
		eof_render();
		eof_color_dialog(eof_catalog_entry_name_dialog, gui_fg_color, gui_bg_color);
		centre_dialog(eof_catalog_entry_name_dialog);
		ustrcpy(eof_etext, eof_song->catalog->entry[eof_selected_catalog_entry].name);
		if(eof_popup_dialog(eof_catalog_entry_name_dialog, 2) == 3)	//User hit OK
		{
			if(ustrcmp(eof_song->catalog->entry[eof_selected_catalog_entry].name, eof_etext))	//If the updated string (eof_etext) is different
			{
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				ustrcpy(eof_song->catalog->entry[eof_selected_catalog_entry].name, eof_etext);
			}
		}
	}
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(screen);
	return D_O_K;
}

int eof_menu_song_legacy_view(void)
{
	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run unless a pro guitar track is active

	if(eof_legacy_view)
	{
		eof_legacy_view = 0;
		eof_song_proguitar_menu[2].flags = 0;	//Song>Pro Guitar>Enable legacy view
		eof_scale_fretboard(0);	//Recalculate the 2D screen positioning based on the current track
	}
	else
	{
		eof_legacy_view = 1;
		eof_song_proguitar_menu[2].flags = D_SELECTED;
		eof_scale_fretboard(5);	//Recalculate the 2D screen positioning based on a 5 lane track
	}
	eof_fix_window_title();
	return 1;
}

#define EOF_SONG_TRACK_DIFFICULTY_MENU_X 0
#define EOF_SONG_TRACK_DIFFICULTY_MENU_Y 48
DIALOG eof_song_track_difficulty_menu[] =
{
   /* (proc)                (x) (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)           (dp2)    (dp3) */
   { d_agup_window_proc,    EOF_SONG_TRACK_DIFFICULTY_MENU_X,  EOF_SONG_TRACK_DIFFICULTY_MENU_Y,  232, 146, 2,   23,  0,    0,      0,   0,   "Set track difficulty", NULL, NULL },
   { d_agup_text_proc,      12, 84,  64,  8,   2,   23,  0,    0,      0,   0,   "Difficulty (0-6):",    NULL, NULL },
   { eof_verified_edit_proc,111,80,  20,  20,  2,   23,  0,    0,      1,   0,   eof_etext,              "0123456", NULL },
   { d_agup_button_proc,    8,  152, 84,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",                   NULL, NULL },
   { d_agup_button_proc,    111,152, 78,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",               NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_song_track_difficulty_menu_pro_drum[] =
{
   { d_agup_text_proc,      12, 104, 114,   8,   2,   23,  0,    0,      0,   0,   "Pro Drum Difficulty (0-6):",    NULL, NULL },
   { eof_verified_edit_proc,174,100,  20,   20,  2,   23,  0,    0,      1,   0,   eof_etext2,              "0123456", NULL },
   { d_agup_text_proc,      12, 124, 144,   8,   2,   23,  0,    0,      0,   0,   "PS Real Drum Difficulty (0-6):",    NULL, NULL },
   { eof_verified_edit_proc,200,120,  20,   20,  2,   23,  0,    0,      1,   0,   eof_etext3,              "0123456", NULL },
};

DIALOG eof_song_track_difficulty_menu_harmony[] =
{
   { d_agup_text_proc,      12, 104, 114,   8,   2,   23,  0,    0,      0,   0,   "Harmony Difficulty (0-6):",    NULL, NULL },
   { eof_verified_edit_proc,170,100,  20,   20,  2,   23,  0,    0,      1,   0,   eof_etext2,              "0123456", NULL },
};

DIALOG eof_song_track_difficulty_menu_normal[] =
{
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

int eof_song_track_difficulty_dialog(void)
{
	int difficulty, undo_made = 0;
	int difficulty2 = 0xF, newdifficulty2 = 0xF;	//For pro drums and harmony vocals, a half byte (instead of a full byte) is used to store each's difficulty
	int difficulty3 = 0xF0, newdifficulty3 = 0xF0;	//The PS Real Drums difficulty is also stored in a half byte

	if(!eof_song || !eof_song_loaded)
		return 1;
	eof_song_track_difficulty_menu[5] = eof_song_track_difficulty_menu_normal[0];
	eof_song_track_difficulty_menu[6] = eof_song_track_difficulty_menu_normal[0];
	eof_song_track_difficulty_menu[7] = eof_song_track_difficulty_menu_normal[0];
	eof_song_track_difficulty_menu[8] = eof_song_track_difficulty_menu_normal[0];

	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_song_track_difficulty_menu, gui_fg_color, gui_bg_color);
	centre_dialog(eof_song_track_difficulty_menu);

	if(eof_selected_track == EOF_TRACK_DRUM)
	{	//Insert the pro drum dialog menu items
		eof_song_track_difficulty_menu[5] = eof_song_track_difficulty_menu_pro_drum[0];
		eof_song_track_difficulty_menu[6] = eof_song_track_difficulty_menu_pro_drum[1];
		difficulty2 = (eof_song->track[EOF_TRACK_DRUM]->flags & 0x0F000000) >> 24;		//Mask out the low nibble of the high order byte of the drum track's flags (pro drum difficulty)
		eof_song_track_difficulty_menu[7] = eof_song_track_difficulty_menu_pro_drum[2];
		eof_song_track_difficulty_menu[8] = eof_song_track_difficulty_menu_pro_drum[3];
		difficulty3 = (eof_song->track[EOF_TRACK_DRUM]->flags & 0xF0000000) >> 24;		//Mask out the high nibble of the high order byte of the drum track's flags (PS Real Drums difficulty)
	}
	else if(eof_selected_track == EOF_TRACK_VOCALS)
	{	//Insert the harmony dialog menu items
		eof_song_track_difficulty_menu[5] = eof_song_track_difficulty_menu_harmony[0];
		eof_song_track_difficulty_menu[6] = eof_song_track_difficulty_menu_harmony[1];
		difficulty2 = (eof_song->track[EOF_TRACK_VOCALS]->flags & 0x0F000000) >> 24;	//Mask out the low nibble of the high order byte of the vocal track's flags (harmony difficulty)
	}
	if(difficulty2 != 0x0F)
	{	//If the secondary difficulty (pro drum or vocal harmony) is to be displayed
		sprintf(eof_etext2, "%d", difficulty2);
	}
	else
	{
		eof_etext2[0] = '\0';
	}
	if(difficulty3 != 0xF0)
	{	//If the tertiary difficulty (PS Real Drums) is to be displayed
		sprintf(eof_etext3, "%d", difficulty3 >> 4);	//Shift down from the high nibble
	}
	else
	{
		eof_etext3[0] = '\0';
	}

	//Manually re-center these elements, because they are not altered by centre_dialog()
	eof_song_track_difficulty_menu[5].x += eof_song_track_difficulty_menu[0].x - EOF_SONG_TRACK_DIFFICULTY_MENU_X;	//Add the X amount offset by centre_dialog()
	eof_song_track_difficulty_menu[5].y += eof_song_track_difficulty_menu[0].y - EOF_SONG_TRACK_DIFFICULTY_MENU_Y;	//Add the Y amount offset by centre_dialog()
	eof_song_track_difficulty_menu[6].x += eof_song_track_difficulty_menu[0].x - EOF_SONG_TRACK_DIFFICULTY_MENU_X;	//Add the X amount offset by centre_dialog()
	eof_song_track_difficulty_menu[6].y += eof_song_track_difficulty_menu[0].y - EOF_SONG_TRACK_DIFFICULTY_MENU_Y;	//Add the Y amount offset by centre_dialog()
	eof_song_track_difficulty_menu[7].x += eof_song_track_difficulty_menu[0].x - EOF_SONG_TRACK_DIFFICULTY_MENU_X;	//Add the X amount offset by centre_dialog()
	eof_song_track_difficulty_menu[7].y += eof_song_track_difficulty_menu[0].y - EOF_SONG_TRACK_DIFFICULTY_MENU_Y;	//Add the Y amount offset by centre_dialog()
	eof_song_track_difficulty_menu[8].x += eof_song_track_difficulty_menu[0].x - EOF_SONG_TRACK_DIFFICULTY_MENU_X;	//Add the X amount offset by centre_dialog()
	eof_song_track_difficulty_menu[8].y += eof_song_track_difficulty_menu[0].y - EOF_SONG_TRACK_DIFFICULTY_MENU_Y;	//Add the Y amount offset by centre_dialog()

	if(eof_song->track[eof_selected_track]->difficulty != 0xFF)
	{	//If the track difficulty is defined, write it in text format
		sprintf(eof_etext, "%d", eof_song->track[eof_selected_track]->difficulty);
	}
	else
	{	//Otherwise prepare an empty string
		eof_etext[0] = '\0';
	}
	if(eof_popup_dialog(eof_song_track_difficulty_menu, 2) == 3)	//User hit OK
	{
		if(eof_etext[0] != '\0')
		{	//If a track difficulty was specified
			difficulty = atol(eof_etext);
		}
		else
		{
			difficulty = 0xFF;
		}
		if(difficulty != eof_song->track[eof_selected_track]->difficulty)	//If the updated track difficulty is different
		{
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			eof_song->track[eof_selected_track]->difficulty = difficulty;
			undo_made = 1;
		}
		if((eof_selected_track == EOF_TRACK_DRUM) || (eof_selected_track == EOF_TRACK_VOCALS))
		{	//If a secondary difficulty needs to be checked
			if(eof_etext2[0] != '\0')
			{	//If a secondary track difficulty was specified
				newdifficulty2 = atol(eof_etext2);
			}
			else
			{
				newdifficulty2 = 0x0F;
			}

			if(eof_selected_track == EOF_TRACK_DRUM)
			{	//If a tertiary difficulty needs to be checked
				if(eof_etext3[0] != '\0')
				{	//If a secondary track difficulty was specified
					newdifficulty3 = atol(eof_etext3) << 4;	//Shift up into the high nibble of the byte
				}
				else
				{
					newdifficulty3 = 0xF0;
				}
			}

			if(((newdifficulty2 != difficulty2) || (newdifficulty3 != difficulty3)) && !undo_made)
			{	//If a secondary/tertiary difficulty has changed, make an undo state if one hasn't been made already
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			}

			if(eof_selected_track == EOF_TRACK_DRUM)
			{
				eof_song->track[EOF_TRACK_DRUM]->flags &= ~(0xFF << 24);			//Clear the drum track's flag's most significant byte
				eof_song->track[EOF_TRACK_DRUM]->flags |= (newdifficulty2 << 24);	//Store the pro drum difficulty in the drum track's flag's most significant byte
				eof_song->track[EOF_TRACK_DRUM]->flags |= (newdifficulty3 << 24);	//Store the PS Real Drums difficulty in the drum track's flag's MSB
			}
			else if(eof_selected_track == EOF_TRACK_VOCALS)
			{
				eof_song->track[EOF_TRACK_VOCALS]->flags &= ~(0xFF << 24);			//Clear the vocal track's flag's most significant byte
				eof_song->track[EOF_TRACK_VOCALS]->flags |= (newdifficulty2 << 24);	//Store the harmony difficulty in the vocal track's flag's most significant byte
			}
		}
	}
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(screen);
	return 1;
}

void eof_rebuild_tuning_strings(char *tuningarray)
{
	unsigned long tracknum, ctr;
	int tuning, halfsteps;

	if(!eof_song || (eof_selected_track >= eof_song->tracks) || (eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return;	//Return without rebuilding string tunings if there is an error
	tracknum = eof_song->track[eof_selected_track]->tracknum;

	for(ctr = 0; ctr < EOF_TUNING_LENGTH; ctr++)
	{	//For each usable string in the track
		if((ctr < eof_song->pro_guitar_track[tracknum]->numstrings) && (eof_fret_strings[ctr][0] != '\0'))
		{	//If this string is used by the track and its tuning field is populated
			halfsteps = atol(eof_fret_strings[ctr]);
			if(!halfsteps && (eof_fret_strings[ctr][0] != '0'))
			{	//If there was some kind of error converting this character to a number
				strncpy(tuning_list[ctr], "   ", sizeof(tuning_list[0])-1);	//Empty the tuning string
			}
			else
			{	//Otherwise look up the tuning
				tuning = eof_lookup_tuned_note(eof_song->pro_guitar_track[tracknum], eof_selected_track, ctr, halfsteps);
				if(tuning < 0)
				{	//If there was an error determining the tuning
					strncpy(tuning_list[ctr], "   ", sizeof(tuning_list[0])-1);	//Empty the tuning string
				}
				else
				{	//Otherwise update the tuning string
					tuning %= 12;	//Guarantee this value is in the range of [0,11]
					strncpy(tuning_list[ctr], eof_note_names[tuning], sizeof(tuning_list[0]));
					strncat(tuning_list[ctr], "  ", sizeof(tuning_list[0])-1);	//Pad with a space to ensure old string is overwritten
					tuning_list[ctr][sizeof(tuning_list[0])-1] = '\0';	//Guarantee this string is truncated
				}
			}
		}
		else
		{	//Otherwise empty the string
			strncpy(tuning_list[ctr], "   ", sizeof(tuning_list[0])-1);	//Empty the tuning string
		}
	}

	//Rebuild the tuning name string
	strncpy(eof_tuning_name, eof_lookup_tuning_name(eof_song, eof_selected_track, tuningarray), sizeof(eof_tuning_name)-1);
	strncat(eof_tuning_name, "        ", sizeof(eof_tuning_name) - strlen(eof_tuning_name));	//Pad the end of the string with several spaces
}

int eof_edit_tuning_proc(int msg, DIALOG *d, int c)
{
	int i;
	char * string = NULL;
	int key_list[32] = {KEY_BACKSPACE, KEY_DEL, KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN, KEY_ESC, KEY_ENTER};
	int match = 0;
	int retval;
	char tuning[EOF_TUNING_LENGTH];

	if((msg == MSG_CHAR) || (msg == MSG_UCHAR))
	{	//ASCII is not handled until the MSG_UCHAR event is sent
		for(i = 0; i < 8; i++)
		{
			if((msg == MSG_UCHAR) && (c == 27))
			{	//If the Escape ASCII character was trapped
				return d_agup_edit_proc(msg, d, c);	//Immediately allow the input character to be returned (so the user can escape to cancel the dialog)
			}
			if((msg == MSG_CHAR) && ((c >> 8 == KEY_BACKSPACE) || (c >> 8 == KEY_DEL)))
			{	//If the backspace or delete keys are trapped
				match = 1;	//Ensure the full function runs, so that the strings are rebuilt
				break;
			}
			if(c >> 8 == key_list[i])			//If the input is permanently allowed
			{
				return d_agup_edit_proc(msg, d, c);	//Immediately allow the input character to be returned
			}
		}

		/* see if key is an allowed key */
		if(!match)
		{
			string = (char *)(d->dp2);
			if(string == NULL)	//If the accepted characters list is NULL for some reason
				match = 1;	//Implicitly accept the input character instead of allowing a crash
			else
			{
				for(i = 0; string[i] != '\0'; i++)	//Search all characters of the accepted characters list
				{
					if(string[i] == (c & 0xff))
					{
						match = 1;
						break;
					}
				}
			}
		}

		if(!match)			//If there was no match
			return D_USED_CHAR;	//Drop the character

		retval = d_agup_edit_proc(msg, d, c);	//Allow the input character to be returned
		if(!eof_song || (eof_selected_track >= eof_song->tracks) || (eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
			return retval;	//Return without redrawing string tunings if there is an error

		//Build an integer type tuning array from the current input
		for(i = 0; i < EOF_TUNING_LENGTH; i++)
		{
			tuning[i] = atol(eof_fret_strings[i]) % 12;	//Convert the text input to integer value
		}
		eof_rebuild_tuning_strings(tuning);
		object_message(&eof_pro_guitar_tuning_dialog[2], MSG_DRAW, 0);	//Have Allegro redraw the tuning name
		object_message(&eof_pro_guitar_tuning_dialog[6], MSG_DRAW, 0);	//Have Allegro redraw the string tuning strings
		object_message(&eof_pro_guitar_tuning_dialog[9], MSG_DRAW, 0);	//Have Allegro redraw the string tuning strings
		object_message(&eof_pro_guitar_tuning_dialog[12], MSG_DRAW, 0);	//Have Allegro redraw the string tuning strings
		object_message(&eof_pro_guitar_tuning_dialog[15], MSG_DRAW, 0);	//Have Allegro redraw the string tuning strings
		object_message(&eof_pro_guitar_tuning_dialog[18], MSG_DRAW, 0);	//Have Allegro redraw the string tuning strings
		object_message(&eof_pro_guitar_tuning_dialog[21], MSG_DRAW, 0);	//Have Allegro redraw the string tuning strings
		return retval;
	}

	return d_agup_edit_proc(msg, d, c);	//Allow the input character to be returned
}

DIALOG eof_pro_guitar_tuning_dialog[] =
{
/*	(proc)					(x)  (y)  (w)  (h) (fg) (bg) (key) (flags) (d1)       (d2) (dp)          		(dp2) (dp3) */
	{d_agup_window_proc,	0,   48,  230, 272,0,   0,   0,    0,      0,         0,	"Edit guitar tuning",NULL, NULL },
	{d_agup_text_proc,  	16,  80,  44,  8,  0,   0,   0,    0,      0,         0,	"Tuning:",      	NULL, NULL },
	{d_agup_text_proc,		74,  80,  154, 8,  0,   0,   0,    0, EOF_NAME_LENGTH,0,	eof_tuning_name,    NULL, NULL },

	//Note:  In guitar theory, string 1 refers to high e
	{d_agup_text_proc,      16,  108, 64,  12,  0,   0,   0,    0,      0,         0,   "Half steps above/below standard",NULL,NULL },
	{d_agup_text_proc,      16,  132, 64,  12,  0,   0,   0,    0,      0,         0,   eof_string_lane_6_number,  NULL,          NULL },
	{eof_edit_tuning_proc,	74,  128, 28,  20,  0,   0,   0,    0,      3,         0,   eof_string_lane_6,  "0123456789-",NULL },
	{d_agup_text_proc,      110, 132, 28,  12,  0,   0,   0,    0,      0,         0,   string_6_name,NULL,          NULL },
	{d_agup_text_proc,      16,  156, 64,  12,  0,   0,   0,    0,      0,         0,   eof_string_lane_5_number,  NULL,          NULL },
	{eof_edit_tuning_proc,	74,  152, 28,  20,  0,   0,   0,    0,      3,         0,   eof_string_lane_5,  "0123456789-",NULL },
	{d_agup_text_proc,      110, 156, 28,  12,  0,   0,   0,    0,      0,         0,   string_5_name,NULL,          NULL },
	{d_agup_text_proc,      16,  180, 64,  12,  0,   0,   0,    0,      0,         0,   eof_string_lane_4_number,  NULL,          NULL },
	{eof_edit_tuning_proc,	74,  176, 28,  20,  0,   0,   0,    0,      3,         0,   eof_string_lane_4,  "0123456789-",NULL },
	{d_agup_text_proc,      110, 180, 28,  12,  0,   0,   0,    0,      0,         0,   string_4_name,NULL,          NULL },
	{d_agup_text_proc,      16,  204, 64,  12,  0,   0,   0,    0,      0,         0,   eof_string_lane_3_number,  NULL,          NULL },
	{eof_edit_tuning_proc,	74,  200, 28,  20,  0,   0,   0,    0,      3,         0,   eof_string_lane_3,  "0123456789-",NULL },
	{d_agup_text_proc,      110, 204, 28,  12,  0,   0,   0,    0,      0,         0,   string_3_name,NULL,          NULL },
	{d_agup_text_proc,      16,  228, 64,  12,  0,   0,   0,    0,      0,         0,   eof_string_lane_2_number,  NULL,          NULL },
	{eof_edit_tuning_proc,	74,  224, 28,  20,  0,   0,   0,    0,      3,         0,   eof_string_lane_2,  "0123456789-",NULL },
	{d_agup_text_proc,      110, 228, 28,  12,  0,   0,   0,    0,      0,         0,   string_2_name,NULL,          NULL },
	{d_agup_text_proc,      16,  252, 64,  12,  0,   0,   0,    0,      0,         0,   eof_string_lane_1_number,  NULL,          NULL },
	{eof_edit_tuning_proc,	74,  248, 28,  20,  0,   0,   0,    0,      3,         0,   eof_string_lane_1,  "0123456789-",NULL },
	{d_agup_text_proc,      110, 252, 28,  12,  0,   0,   0,    0,      0,         0,   string_1_name,NULL,          NULL },

	{d_agup_button_proc,    20,  280, 68,  28, 0,   0,   '\r', D_EXIT, 0,         0,   "OK",         NULL,          NULL },
	{d_agup_button_proc,    140, 280, 68,  28, 0,   0,   0,    D_EXIT, 0,         0,   "Cancel",     NULL,          NULL },
	{NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

int eof_menu_song_track_tuning(void)
{
	unsigned long ctr, tracknum, stringcount;
	char undo_made = 0;

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run unless the pro guitar track is active
	tracknum = eof_song->track[eof_selected_track]->tracknum;

	if(!eof_music_paused)
	{
		eof_music_play();
	}

	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	eof_color_dialog(eof_pro_guitar_tuning_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_pro_guitar_tuning_dialog);

//Update the strings to reflect the currently set tuning
	stringcount = eof_song->pro_guitar_track[tracknum]->numstrings;
	for(ctr = 0; ctr < 6; ctr++)
	{	//For each of the 6 supported strings
		if(ctr < stringcount)
		{	//If this track uses this string, convert the tuning to its string representation
			snprintf(eof_fret_strings[ctr], sizeof(eof_string_lane_1), "%d", eof_song->pro_guitar_track[tracknum]->tuning[ctr]);
			eof_pro_guitar_tuning_dialog[19 - (3 * ctr)].flags = 0;		//Enable the input field's label
			eof_pro_guitar_tuning_dialog[20 - (3 * ctr)].flags = 0;		//Enable the input field for the string
			eof_fret_string_numbers[ctr][7] = '0' + (stringcount - ctr);	//Correct the string number for this label
		}
		else
		{	//Otherwise hide this textbox and its label
			eof_pro_guitar_tuning_dialog[19 - (3 * ctr)].flags = D_HIDDEN;	//Hide the input field's label
			eof_pro_guitar_tuning_dialog[20 - (3 * ctr)].flags = D_HIDDEN;	//Hide the input field for the string
		}
	}
	eof_rebuild_tuning_strings(eof_song->pro_guitar_track[tracknum]->tuning);

	if(eof_popup_dialog(eof_pro_guitar_tuning_dialog, 0) == 22)
	{	//If user clicked OK
		//Validate and store the input
		for(ctr = 0; ctr < eof_song->pro_guitar_track[tracknum]->numstrings; ctr++)
		{	//For each string in the track, ensure the user entered a tuning
			if(eof_fret_strings[ctr][0] == '\0')
			{	//Ensure the user entered a tuning
				allegro_message("Error:  Each of the track's strings must have a defined tuning");
				return 1;
			}
			if(!atol(eof_fret_strings[ctr]) && (eof_fret_strings[ctr][0] != '0'))
			{	//Ensure the tuning is valid (ie. not "-")
				allegro_message("Error:  Invalid tuning value");
				return 1;
			}
			if((atol(eof_fret_strings[ctr]) > 11) || (atol(eof_fret_strings[ctr]) < -11))
			{	//Ensure the tuning is valid (ie. not "-")
				allegro_message("Error:  Invalid tuning value (must be between -11 and 11)");
				return 1;
			}
		}
		for(ctr = 0; ctr < eof_song->pro_guitar_track[tracknum]->numstrings; ctr++)
		{	//For each string in the track, store the numerical value into the track's tuning array
			if(!undo_made && (eof_song->pro_guitar_track[tracknum]->tuning[ctr] != atol(eof_fret_strings[ctr]) % 12))
			{	//If a tuning was changed
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				undo_made = 1;
				eof_chord_lookup_note = 0;	//Reset the cached chord lookup count
			}
			eof_song->pro_guitar_track[tracknum]->tuning[ctr] = atol(eof_fret_strings[ctr]) % 12;
		}
	}//If user clicked OK

	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	return 1;
}

int eof_menu_set_num_frets_strings(void)
{
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	unsigned char newnumfrets = 0, newnumstrings = 0;
	unsigned long highestfret = 0;
	char undo_made = 0, cancel = 0;

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run if a pro guitar track isn't active

	//Update dialog fields
	snprintf(eof_etext2, sizeof(eof_etext2), "%d", eof_song->pro_guitar_track[tracknum]->numfrets);
	eof_note_set_num_frets_strings_dialog[4].flags = eof_note_set_num_frets_strings_dialog[5].flags = eof_note_set_num_frets_strings_dialog[6].flags = 0;
	switch(eof_song->pro_guitar_track[tracknum]->numstrings)
	{
		case 4:
			eof_note_set_num_frets_strings_dialog[4].flags = D_SELECTED;
		break;
		case 5:
			eof_note_set_num_frets_strings_dialog[5].flags = D_SELECTED;
		break;
		case 6:
			eof_note_set_num_frets_strings_dialog[6].flags = D_SELECTED;
		break;
	}

	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_note_set_num_frets_strings_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_note_set_num_frets_strings_dialog);
	if(eof_popup_dialog(eof_note_set_num_frets_strings_dialog, 2) == 7)
	{	//If the user clicked OK
		//Update max fret number
		newnumfrets = atol(eof_etext2);
		if(newnumfrets && (newnumfrets != eof_song->pro_guitar_track[tracknum]->numfrets))
		{	//If the specified number of frets was changed
			highestfret = eof_get_highest_fret(eof_selected_track, 0);	//Get the highest used fret value in this track
			if(highestfret > newnumfrets)
			{	//If any notes in this track use fret values that would exceed the new fret limit
				char message[120];
				snprintf(message, sizeof(message), "Warning:  This track uses frets as high as %lu, exceeding the proposed limit.  Continue?", highestfret);
				if(alert(NULL, message, NULL, "&Yes", "&No", 'y', 'n') != 1)
				{	//If user does not opt to continue after being alerted of this fret limit issue
					return 1;
				}
			}
			if(!undo_made)
			{
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				undo_made = 1;
			}
			eof_song->pro_guitar_track[tracknum]->numfrets = newnumfrets;
		}
		//Update number of strings
		newnumstrings = eof_song->pro_guitar_track[tracknum]->numstrings;
		if(eof_note_set_num_frets_strings_dialog[4].flags == D_SELECTED)
		{
			newnumstrings = 4;
		}
		else if(eof_note_set_num_frets_strings_dialog[5].flags == D_SELECTED)
		{
			newnumstrings = 5;
		}
		else if(eof_note_set_num_frets_strings_dialog[6].flags == D_SELECTED)
		{
			newnumstrings = 6;
		}
		if(newnumstrings != eof_song->pro_guitar_track[tracknum]->numstrings)
		{	//If the specified number of strings was changed
			if(eof_detect_string_gem_conflicts(eof_song->pro_guitar_track[tracknum], newnumstrings))
			{
				if(alert(NULL, "Warning:  Changing the # of strings will cause one or more gems to be deleted.  Continue?", NULL, "&Yes", "&No", 'y', 'n') != 1)
				{	//If user opts to cancel
					cancel = 1;
				}
			}

			if(!cancel)
			{
				if(!undo_made)
				{
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					undo_made = 1;
				}
				eof_song->pro_guitar_track[tracknum]->numstrings = newnumstrings;
				eof_scale_fretboard(0);	//Recalculate the 2D screen positioning based on the current track
			}
		}
		//Perform cleanup
		if(undo_made)
		{
			eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Fix fret/string conflicts
		}
	}

	return 1;
}

int eof_menu_song_five_lane_drums(void)
{
	unsigned long tracknum = eof_song->track[EOF_TRACK_DRUM]->tracknum;

	if(eof_five_lane_drums_enabled())
	{	//Turn off five lane drums
		eof_song->track[EOF_TRACK_DRUM]->flags &= ~(EOF_TRACK_FLAG_SIX_LANES);	//Clear the flag
		eof_song->legacy_track[tracknum]->numlanes = 5;
	}
	else
	{	//Turn on five lane drums
		eof_song->track[EOF_TRACK_DRUM]->flags |= EOF_TRACK_FLAG_SIX_LANES;	//Set the flag
		eof_song->legacy_track[tracknum]->numlanes = 6;
	}

	eof_set_3D_lane_positions(0);	//Update xchart[] by force
	eof_scale_fretboard(0);			//Recalculate the 2D screen positioning based on the current track
	return 1;
}

void eof_set_percussion_cue(int cue_number)
{
	if((cue_number < 17) || (cue_number > 36))
	{	//If the cue number is out of bounds
		cue_number = 17;	//Reset to cowbell
	}

	switch(cue_number)
	{
		case 17:
			eof_sound_chosen_percussion = eof_sound_cowbell;
		break;
		case 18:
			eof_sound_chosen_percussion = eof_sound_triangle1;
		break;
		case 19:
			eof_sound_chosen_percussion = eof_sound_triangle2;
		break;
		case 20:
			eof_sound_chosen_percussion = eof_sound_tambourine1;
		break;
		case 21:
			eof_sound_chosen_percussion = eof_sound_tambourine2;
		break;
		case 22:
			eof_sound_chosen_percussion = eof_sound_tambourine3;
		break;
		case 23:
			eof_sound_chosen_percussion = eof_sound_woodblock1;
		break;
		case 24:
			eof_sound_chosen_percussion = eof_sound_woodblock2;
		break;
		case 25:
			eof_sound_chosen_percussion = eof_sound_woodblock3;
		break;
		case 26:
			eof_sound_chosen_percussion = eof_sound_woodblock4;
		break;
		case 27:
			eof_sound_chosen_percussion = eof_sound_woodblock5;
		break;
		case 28:
			eof_sound_chosen_percussion = eof_sound_woodblock6;
		break;
		case 29:
			eof_sound_chosen_percussion = eof_sound_woodblock7;
		break;
		case 30:
			eof_sound_chosen_percussion = eof_sound_woodblock8;
		break;
		case 31:
			eof_sound_chosen_percussion = eof_sound_woodblock9;
		break;
		case 32:
			eof_sound_chosen_percussion = eof_sound_woodblock10;
		break;
		case 33:
			eof_sound_chosen_percussion = eof_sound_clap1;
		break;
		case 34:
			eof_sound_chosen_percussion = eof_sound_clap2;
		break;
		case 35:
			eof_sound_chosen_percussion = eof_sound_clap3;
		break;
		case 36:
			eof_sound_chosen_percussion = eof_sound_clap4;
		break;
	}
	eof_selected_percussion_cue = cue_number;
}

void eof_seek_by_grid_snap(int dir)
{
	long beat;
	unsigned long adjustedpos = eof_music_pos - eof_av_delay;	//Find the actual chart position
	unsigned long originalpos = adjustedpos;

	if(!eof_song || (eof_snap_mode == EOF_SNAP_OFF))
		return;

	beat = eof_get_beat(eof_song, adjustedpos);	//Find which beat the current seek position is in
	if(beat < 0)	//If the seek position is outside the chart
		return;

	if(dir < 0)
	{	//If seeking backward
		if(adjustedpos <= eof_song->beat[0]->pos)
			return;	//Do not allow this operation to seek before the first beat marker
		if(adjustedpos == eof_song->beat[beat]->pos)
			beat--;	//If the current position is on a beat marker, seeking back will take the previous beat into account instead
	}
	else
	{	//If seeking forward
		if(adjustedpos >= eof_song->beat[eof_song->beats - 1]->pos)
			return;	//Do not allow this operation to seek after the last beat marker
	}

	if(dir < 0)
	{	//If seeking backward
		eof_snap_logic(&eof_tail_snap, eof_song->beat[beat]->pos);	//Find beat/measure length
		eof_snap_length_logic(&eof_tail_snap);	//Find length of one grid snap in the target beat
		if(eof_tail_snap.length > adjustedpos)
		{	//Special case:  Current position is less than one grid snap from the beginning of the chart
			eof_set_seek_position(eof_av_delay);	//Seek to the beginning of the chart
		}
		else
		{
			eof_snap_logic(&eof_tail_snap, adjustedpos - eof_tail_snap.length);	//Find the grid snapped position of the new seek position
			eof_set_seek_position(eof_tail_snap.pos + eof_av_delay);	//Seek to the new seek position
		}
	}
	else
	{	//If seeking forward
		eof_snap_logic(&eof_tail_snap, adjustedpos);					//Find the grid snapped position of the new seek position
		eof_set_seek_position(eof_tail_snap.next_snap + eof_av_delay);	//Seek to the next calculated grid snap position
	}

	if((eof_input_mode == EOF_INPUT_FEEDBACK) && (KEY_EITHER_SHIFT))
	{
		eof_shift_used = 1;	//Track that the SHIFT key was used
		if(eof_seek_selection_start == eof_seek_selection_end)
		{	//If this begins a seek selection
			eof_update_seek_selection(originalpos, eof_music_pos - eof_av_delay);
		}
		else
		{
			eof_update_seek_selection(eof_seek_selection_start, eof_music_pos - eof_av_delay);
		}
	}
	eof_feedback_input_mode_update_selected_beat();	//Update the selected beat and measure if Feedback input mode is in use
}

int eof_menu_song_seek_previous_grid_snap(void)
{
	eof_seek_by_grid_snap(-1);
	return 1;
}

int eof_menu_song_seek_next_grid_snap(void)
{
	eof_seek_by_grid_snap(1);
	return 1;
}

int eof_menu_song_seek_previous_anchor(void)
{
	if(!eof_song)
		return 1;

	long b = eof_get_beat(eof_song, eof_music_pos - eof_av_delay);

	if(eof_song->beat[b]->pos == eof_music_pos - eof_av_delay)
	{	//If the seek position was on the beat marker
		b--;	//Go to the previous beat
	}
	while((b > 0) && ((eof_song->beat[b]->flags & EOF_BEAT_FLAG_ANCHOR) == 0))
	{	//Starting at/before the beat at the current seek position, until an anchor is found,
		b--;	//Go back one beat
	}
	if(b < 0)
	{	//If no other suitable anchor was found
		b = 0;	//Seek to the first beat marker
	}
	eof_set_seek_position(eof_song->beat[b]->pos + eof_av_delay);	//Seek to the anchor

	return 1;
}

int eof_menu_song_seek_next_anchor(void)
{
	if(!eof_song)
		return 1;

      	long b = eof_get_beat(eof_song, eof_music_pos - eof_av_delay);
      	long b2 = b;

      	if(eof_song->beat[b]->pos == eof_music_pos - eof_av_delay)
      	{	//If the seek position was on the beat marker
      		b++;	//Go to the next beat
      	}
      	while((b > 0) && (b < eof_song->beats) && ((eof_song->beat[b]->flags & EOF_BEAT_FLAG_ANCHOR) == 0))
      	{	//Starting at/before the beat at the current seek position, until an anchor is found,
      		b++;	//Go forward one beat
      	}
      	if((b < eof_song->beats) && (b != b2))
      	{	//Don't perform a seek if there was no anchor ahead of the current seek position
      		if(b < 0)
      		{	//If no other suitable anchor was found
      			b = 0;	//Seek to the first beat marker
      		}
      		eof_set_seek_position(eof_song->beat[b]->pos + eof_av_delay);	//Seek to the anchor
      	}

	return 1;
}

int eof_menu_song_seek_previous_beat(void)
{
	if(!eof_song)
		return 1;

	long b = eof_get_beat(eof_song, eof_music_pos - eof_av_delay);

	if(b > 0)
	{
		if(eof_song->beat[b]->pos == eof_music_pos - eof_av_delay)
		{
			eof_set_seek_position(eof_song->beat[b - 1]->pos + eof_av_delay);
		}
		else
		{
			eof_set_seek_position(eof_song->beat[b]->pos + eof_av_delay);
		}
	}
	else
	{
		eof_set_seek_position(eof_song->beat[0]->pos + eof_av_delay);
	}

	return 1;
}

int eof_menu_song_seek_next_beat(void)
{
	if(!eof_song)
		return 1;

      	long b = eof_get_beat(eof_song, eof_music_pos - eof_av_delay);
      	if(eof_music_pos - eof_av_delay < 0)
      	{
      		b = -1;
      	}

      	if(((b < 0) || (b < eof_song->beats - 1)) && (eof_song->beat[b + 1]->pos < eof_music_actual_length))
      	{
      		eof_set_seek_position(eof_song->beat[b + 1]->pos + eof_av_delay);
      	}

	return 1;
}

int eof_menu_song_seek_previous_measure(void)
{
	if(!eof_song)
		return 1;

	long b = eof_get_beat(eof_song, eof_music_pos - eof_av_delay);
	unsigned num, ctr;
	unsigned long originalpos = eof_music_pos - eof_av_delay;
	while(b >= 0)
	{	//For each beat at or before the current seek position
		if(eof_get_ts(eof_song, &num, NULL, b) == 1)
		{	//If this beat is a time signature
			break;	//Break from the loop
		}
		b--;	//Check previous beat on next loop
	}
	if(b < 0)
	{	//If no time signature was found
		num = 4;	//Assume 4/4
	}
	for(ctr = 0; ctr < num; ctr++)
	{	//Seek backward by a number of beats equal to the TS numerator
		eof_menu_song_seek_previous_beat();
	}
	if(eof_input_mode == EOF_INPUT_FEEDBACK)
	{	//If Feedback input method is in effect
		if(KEY_EITHER_SHIFT)
		{	//If the user held the SHIFT key down, update the seek selection
			eof_shift_used = 1;	//Track that the SHIFT key was used
			if(eof_seek_selection_start == eof_seek_selection_end)
			{	//If this begins a seek selection
				eof_update_seek_selection(originalpos, eof_music_pos - eof_av_delay);
			}
			else
			{
				eof_update_seek_selection(eof_seek_selection_start, eof_music_pos - eof_av_delay);
			}
		}
	}
	eof_feedback_input_mode_update_selected_beat();	//Update the selected beat and measure if Feedback input mode is in use
	return 1;
}

int eof_menu_song_seek_next_measure(void)
{
	if(!eof_song)
		return 1;

	long b = eof_get_beat(eof_song, eof_music_pos - eof_av_delay);
	unsigned num = 0, ctr;
	unsigned long originalpos = eof_music_pos - eof_av_delay;
	while(b >= 0)
	{	//For each beat at or before the current seek position
		if(eof_get_ts(eof_song, &num, NULL, b) == 1)
		{	//If this beat is a time signature
			break;	//Break from the loop
		}
		b--;	//Check previous beat on next loop
	}
	if(b < 0)
	{	//If no time signature was found
		num = 4;	//Assume 4/4
	}
	for(ctr = 0; ctr < num; ctr++)
	{	//Seek backward by a number of beats equal to the TS numerator
		eof_menu_song_seek_next_beat();
	}
	if(eof_input_mode == EOF_INPUT_FEEDBACK)
	{	//If Feedback input method is in effect
		if(KEY_EITHER_SHIFT)
		{	//If the user held the SHIFT key down, update the seek selection
			eof_shift_used = 1;	//Track that the SHIFT key was used
			if(eof_seek_selection_start == eof_seek_selection_end)
			{	//If this begins a seek selection
				eof_update_seek_selection(originalpos, eof_music_pos - eof_av_delay);
			}
			else
			{
				eof_update_seek_selection(eof_seek_selection_start, eof_music_pos - eof_av_delay);
			}
		}
	}
	eof_feedback_input_mode_update_selected_beat();	//Update the selected beat and measure if Feedback input mode is in use
	return 1;
}

int eof_menu_previous_chord_result(void)
{
	if(eof_enable_chord_cache && (eof_chord_lookup_count > 1))
	{	//If an un-named note is selected and it has at least two chord matches
		if(eof_selected_chord_lookup)
		{	//If any except the first lookup is selected for display
			eof_selected_chord_lookup--;
		}
		else
		{	//Otherwise cycle to the last lookup
			eof_selected_chord_lookup = eof_chord_lookup_count - 1;
		}
	}
	return 1;
}

int eof_menu_next_chord_result(void)
{
	if(eof_enable_chord_cache && (eof_chord_lookup_count > 1))
	{	//If an un-named note is selected and it has at least two chord matches
		eof_selected_chord_lookup++;
		if(eof_selected_chord_lookup >= eof_chord_lookup_count)
		{	//If the selected chord number exceeded the number of chord matches
			eof_selected_chord_lookup = 0;	//Revert to showing the first match
		}
	}
	return 1;
}

int eof_menu_song_raw_MIDI_tracks(void)
{
	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_raw_midi_tracks_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_raw_midi_tracks_dialog);
	eof_MIDI_track_list_to_enumerate = eof_song->midi_data_head;	//This is the list that the dialog should enumerate
	if(eof_popup_dialog(eof_raw_midi_tracks_dialog, 0) == 5)
	{	//User clicked done
	}
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(NULL);
	return 1;
}

DIALOG eof_raw_midi_tracks_dialog[] =
{
   /* (proc)            (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)            (dp2) (dp3) */
   { d_agup_window_proc,0,   48,  500, 232, 2,   23,  0,    0,      0,   0,   "Stored MIDI tracks",    NULL, NULL },
   { d_agup_list_proc,  12,  84,  400, 138, 2,   23,  0,    0,      0,   0,   eof_raw_midi_tracks_list,NULL, NULL },
   { d_agup_push_proc,  425, 84,  68,  28,  2,   23,  'a',  D_EXIT, 0,   0,   "&Add",         NULL, eof_raw_midi_dialog_add },
//   { d_agup_push_proc,  425, 124, 68,  28,  2,   23,  'd',  D_EXIT, 0,   0,   "&Desc",        NULL, eof_raw_midi_dialog_desc },
   { d_agup_push_proc,  425, 164, 68,  28,  2,   23,  'l',  D_EXIT, 0,   0,   "De&lete",      NULL, eof_raw_midi_dialog_delete },
   { d_agup_button_proc,12,  235, 240, 28,  2,   23,  '\r', D_EXIT, 0,   0,   "Done",         NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

struct eof_MIDI_data_track * eof_MIDI_track_list_to_enumerate;
char * eof_raw_midi_tracks_list(int index, int * size)
{
	struct eof_MIDI_data_track *trackptr;
	unsigned long ctr;

	switch(index)
	{
		case -1:
		{	//Return a count of the number of tracks
			if(!eof_MIDI_track_list_to_enumerate)
			{
				*size = 0;
				return NULL;
			}

			for(ctr = 0, trackptr = eof_MIDI_track_list_to_enumerate; trackptr != NULL; ctr++, trackptr = trackptr->next);	//Count the number of tracks in this list
			if(ctr > 0)
			{	//If there is at least stored MIDI track
				eof_raw_midi_tracks_dialog[3].flags = 0;	//Enable the Delete button
			}
			else
			{
				eof_raw_midi_tracks_dialog[3].flags = D_DISABLED;
			}
			*size = ctr;
			break;
		}
		default:
		{
			if(!eof_MIDI_track_list_to_enumerate)
				return eof_raw_midi_track_error;

			for(ctr = 0, trackptr = eof_MIDI_track_list_to_enumerate; (trackptr != NULL) && (ctr < index); ctr++, trackptr = trackptr->next);	//Seek to the appropriate track link
			if(!trackptr)
				return eof_raw_midi_track_error;

			return trackptr->trackname;
		}
	}
	return NULL;
}

int eof_raw_midi_dialog_delete(DIALOG * d)
{
	struct eof_MIDI_data_track *trackptr, *prevptr = NULL, *tempptr;
	unsigned long ctr;

	if(!eof_song->midi_data_head)
	{	//If there are no stored MIDI tracks
		return D_O_K;
	}
	for(ctr = 0, trackptr = eof_song->midi_data_head; trackptr != NULL; ctr++, trackptr = trackptr->next)
	{	//For each stored MIDI track
		if(eof_raw_midi_tracks_dialog[1].d1 == ctr)
		{	//If this is the track to be removed
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			tempptr = trackptr->next;
			if(trackptr->trackname)
				free(trackptr->trackname);
			if(trackptr->description)
				free(trackptr->description);
			eof_MIDI_empty_event_list(trackptr->events);
			free(trackptr);

			if(prevptr)
			{	//If there was a previous pointer
				prevptr->next = tempptr;	//It will point to the next link
			}
			else
			{	//The head link was just deleted
				eof_song->midi_data_head = tempptr;	//The new head link is the one that follows the deleted link
				eof_MIDI_track_list_to_enumerate = tempptr;	//Update the head pointer used by eof_raw_midi_tracks_list()
			}
			if(!tempptr)
			{	//If the last list entry was deleted
				eof_raw_midi_tracks_dialog[1].d1--;	//Re-select the new last entry in the list
			}
			object_message(&eof_raw_midi_tracks_dialog[1], MSG_DRAW, 0);	//Redraw the tracks list
			return D_O_K;
		}
		prevptr = trackptr;	//Keep track of this so the next link's previous link is known
	}
	return D_O_K;
}

DIALOG eof_raw_midi_add_track_dialog[] =
{
   /* (proc)            (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)            (dp2) (dp3) */
   { d_agup_window_proc,0,   48,  500, 232, 2,   23,  0,    0,      0,   0,   "Select MIDI track to import",    NULL, NULL },
   { d_agup_list_proc,  12,  84,  400, 138, 2,   23,  0,    0,      0,   0,   eof_raw_midi_tracks_list,NULL, NULL },
   { d_agup_button_proc, 42,  235, 68,  28, 2,   23,  '\r', D_EXIT, 0,   0,   "OK",               NULL, NULL },
   { d_agup_button_proc, 120, 235, 68,  28, 2,   23,  0,    D_EXIT, 0,   0,   "Cancel",           NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

int eof_raw_midi_dialog_add(DIALOG * d)
{
	char * returnedfn = NULL;
	char tempfilename[1024] = {0};
	MIDI * eof_work_midi = NULL;
	struct eof_MIDI_data_track *head = NULL, *tail = NULL, *ptr, *selected = NULL, *prev = NULL, *next, *prevenumeration = eof_MIDI_track_list_to_enumerate;
	unsigned long ctr;
	char undo_made = 0, canceled = 0, updateenumlist = 0;
	int junk;

	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	returnedfn = ncd_file_select(0, eof_last_eof_path, "Import raw MIDI track", eof_filter_midi_files);
	eof_clear_input();
	if(returnedfn)
	{	//If the user selected a file
		eof_log("\tImporting raw MIDI track", 1);

//Have user browse for a MIDI/RBA file
		if(!ustricmp(get_extension(returnedfn), "rba"))
		{	//If the selected file has a .rba extension, extract the MIDI file
			replace_filename(tempfilename, returnedfn, "eof_rba_import.tmp", 1024);
			if(eof_extract_rba_midi(returnedfn,tempfilename) == 0)
			{	//If this was an RBA file and the MIDI was extracted successfully
				eof_work_midi = load_midi(tempfilename);	//Buffer extracted MIDI file
				delete_file(tempfilename);	//Delete temporary file
			}
		}
		else
		{	//Buffer MIDI file
			eof_work_midi = load_midi(returnedfn);
		}
		if(!eof_work_midi)
		{
			return 0;
		}

//Parse the MIDI delay from song.ini if it exists in the same folder as the MIDI
		int delay = 0;
		replace_filename(tempfilename, returnedfn, "song.ini", 1024);
		set_config_file(tempfilename);
		delay = get_config_int("song", "delay", 0);
		if(delay)
		{	//If a nonzero delay was read from song.ini
			snprintf(tempfilename, sizeof(tempfilename), "%d", delay);
			if(alert("Import this MIDI delay from song.ini?", tempfilename, NULL, "&Yes", "&No", 'y', 'n') != 1)
			{	//If the user declined using the MIDI delay from the MIDI's song.ini file
				delay = 0;	//Set the delay back to 0
			}
		}

//Build a linked list of track information
		for(ctr = 0; ctr < MIDI_TRACKS; ctr++)
		{
			if(eof_work_midi->track[ctr].data)
			{	//For each populated track in the buffered MIDI
				ptr = eof_get_raw_MIDI_data(eof_work_midi, ctr, delay);
				if(!ptr)
				{	//If the track was not read
					dialog_message(eof_raw_midi_tracks_dialog, MSG_DRAW, 0, &junk);	//Redraw the Manage raw MIDI tracks dialog
					return 0;
				}
				if(!ptr->trackname)
				{	//If the track has no name
					eof_MIDI_empty_track_list(ptr);	//Delete it
				}
				else
				{	//Add the track to the list
					if(head == NULL)
					{	//If the list is empty
						head = ptr;	//The new link is now the first link in the list
					}
					else if(tail != NULL)
					{	//If there is already a link at the end of the list
						tail->next = ptr;	//Point it forward to the new link
					}
					tail = ptr;	//The new link is the new tail of the list
				}
			}
		}

//Have the user select a track to import
		eof_MIDI_track_list_to_enumerate = head;	//eof_raw_midi_tracks_list() will enumerate this list
		eof_color_dialog(eof_raw_midi_add_track_dialog, gui_fg_color, gui_bg_color);
		centre_dialog(eof_raw_midi_add_track_dialog);
		if(eof_popup_dialog(eof_raw_midi_add_track_dialog, 0) == 2)
		{	//If the user selected a track to import and clicked OK
			for(ctr = 0, ptr = head; ptr != NULL; ctr++, ptr = ptr->next)
			{	//For each track in the user-selected MIDI
				if(eof_raw_midi_add_track_dialog[1].d1 == ctr)
				{	//If this is the track to import
					selected = ptr;	//Keep track of the link that is to be kept
					break;
				}
			}

//Validate the user's selection
			if(!selected->trackname)
			{
				selected = NULL;
				canceled = 1;
			}
			else if((selected == head) || !ustricmp(selected->trackname, "BEAT") || !ustricmp(selected->trackname, "EVENTS"))
			{	//If the user selected track 0 or the beat or events tracks
				allegro_message("Import of this track is not allowed because it is written during save");
				selected = NULL;
				canceled = 1;
			}
			else
			{
				for(ctr = 0; ctr < EOF_TRACKS_MAX; ctr++)
				{	//For each of EOF's supported tracks
					if(!ustricmp(selected->trackname, eof_midi_tracks[ctr].name))
					{	//If the user selected a track with a name that EOF natively supports
						allegro_message("\"%s\" is a track supported by EOF.  To load it into the current project, please import it in another EOF instance and copy+paste it into this project.", selected->trackname);
						selected = NULL;
						canceled = 1;
						break;
					}
				}
			}
			if(!canceled)
			{
				for(ptr = eof_song->midi_data_head; ptr != NULL; ptr = ptr->next)
				{	//For each MIDI track already being stored in the project
					next = ptr->next;
					if(!ustricmp(selected->trackname, ptr->trackname))
					{	//If the user selected a track with a name matching a track already being stored in the project
						if(alert("A track with this name is already being stored:", selected->trackname, "Replace it?", "&Yes", "&No", 'y', 'n') != 1)
						{	//If the user declined to replace the previously-stored track with the newly-selected track
							selected = NULL;
							canceled = 1;
						}
						else
						{	//Remove the existing track
							eof_prepare_undo(EOF_UNDO_TYPE_NONE);
							undo_made = 1;
							if(ptr->trackname)
								free(ptr->trackname);
							if(ptr->description)
								free(ptr->description);
							eof_MIDI_empty_event_list(ptr->events);
								free(ptr);
							if(prev)
							{	//If there was a previous pointer
								prev->next = next;	//It will point to the next link
							}
							else
							{	//The head link was just deleted
								eof_song->midi_data_head = next;	//The new head link is the one that follows the deleted link
							}
						}
						break;
					}
					prev = ptr;	//Keep track of this so the next link's previous link is known
				}
			}

//Add the selected track, remove the others from memory
			if(!canceled)
			{
				if(!undo_made)
				{	//If any undo state wasn't already made
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					undo_made = 1;
				}
				if(eof_song->midi_data_head == prevenumeration)
				{	//If the calling dialog was enumerating the list in the loaded project
					updateenumlist = 1;	//Remember to update the pointer just in case the head link is replaced
				}
				eof_MIDI_add_track(eof_song, selected);
				if(updateenumlist)
				{	//If the calling dialog was enumerating the list in the loaded project
					prevenumeration = eof_song->midi_data_head;	//Update this as well so the updated list is enumerated
				}
			}
		}//If the user selected a track to import and clicked OK
		else
		{
			canceled = 1;
		}

//Remove all tracks (that weren't stored) from memory
		for(ptr = head; ptr != NULL; ptr = next)
		{	//For each track stored from the user-selected MIDI
			next = ptr->next;
			if(ptr != selected)
			{	//If this isn't the track that is being kept
				if(ptr->trackname)
					free(ptr->trackname);
				if(ptr->description)
					free(ptr->description);
				eof_MIDI_empty_event_list(ptr->events);
				free(ptr);
			}
		}
		if(!canceled)
		{	//Perform this at the end so that the unused portion of the list is able to be destroyed via the loop above
			selected->next = NULL;	//This is the new tail of the list
		}
	}//If the user selected a file
	eof_MIDI_track_list_to_enumerate = prevenumeration;
	dialog_message(eof_raw_midi_tracks_dialog, MSG_DRAW, 0, &junk);	//Redraw the Manage raw MIDI tracks dialog
	return 0;
}

DIALOG eof_seek_beat_measure_dialog[] =
{
   /* (proc) 		        (x)	(y)	(w)	(h)	(fg) (bg) (key) (flags)	(d1)(d2)(dp)						(dp2) (dp3) */
   { d_agup_window_proc,  	0,	48,	180,157,2,   23,  0,    0,      0,	0,	"Seek to beat/measure",	NULL, NULL },
   { d_agup_text_proc,		16,	80,	64,	8,	2,   23,  0,    0,      0,	0,	"Seek to:",				NULL, NULL },
   { d_agup_radio_proc,		16,	100,110,15,	2,   23,  0,    0,      0,	0,	eof_etext,				NULL, NULL },
   { d_agup_radio_proc,		16,	120,160,15,	2,   23,  0,    0,      0,	0,	eof_etext2,				NULL, NULL },
   { d_agup_text_proc,		16,	140,80,16,	2,   23,  0,    0,		1,	0,	"Number:",				NULL, NULL },
   { eof_verified_edit_proc,70, 140,66,20,  2,   23,  0,    0,      4,  0,  eof_etext3,           	"1234567890", NULL },
   { d_agup_button_proc,	16,	164,68,	28,	2,   23,  '\r',	D_EXIT, 0,	0,	"OK",             		NULL, NULL },
   { d_agup_button_proc,	96, 164,68,	28,	2,   23,  0,	D_EXIT, 0,	0,	"Cancel",         		NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

int eof_menu_song_seek_beat_measure(void)
{
	unsigned long measurecount;
	int retval, done = 0;
	static unsigned long lastselected = 2;

	if(!eof_song)
		return 1;
	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	eof_color_dialog(eof_seek_beat_measure_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_seek_beat_measure_dialog);

	//Create the beat and measure strings
	eof_seek_beat_measure_dialog[2].flags = 0;	//Clear these so they can be selected/disabled below as applicable
	eof_seek_beat_measure_dialog[3].flags = 0;
	measurecount = eof_get_measure(0, 1);	//Count the number of measures
	snprintf(eof_etext, sizeof(eof_etext), "Beat [0 - %lu]", eof_song->beats - 1);
	if(measurecount)
	{	//If there is at least one measure
		snprintf(eof_etext2, sizeof(eof_etext2), "Measure [1 - %lu]", measurecount);
		eof_seek_beat_measure_dialog[3].flags = 0;	//Enable the measure radio button
	}
	else
	{
		snprintf(eof_etext2, sizeof(eof_etext2), "Measure (no TS)");
		lastselected = 2;	//Have the beat option selected instead
		eof_seek_beat_measure_dialog[3].flags = D_DISABLED;	//Disable the measure radio button
	}
	if(lastselected == 2)
	{	//If the "beat" radio button was selected last time the menu was open
		eof_seek_beat_measure_dialog[2].flags = D_SELECTED;	//Ensure it's selected now
	}
	else
	{	//If the "measure" radio button was selected last time the menu was open
		eof_seek_beat_measure_dialog[3].flags = D_SELECTED;	//Ensure it's selected now
	}
	while(!done)
	{
		retval = eof_popup_dialog(eof_seek_beat_measure_dialog, 0);
		if(retval == 6)
		{	//User clicked OK
			unsigned long input = atol(eof_etext3);
			if(eof_seek_beat_measure_dialog[2].flags & D_SELECTED)
			{	//User opted to seek to a beat
				lastselected = 2;
				if(input < eof_song->beats)
				{	//The specified beat is valid
					eof_set_seek_position(eof_song->beat[input]->pos + eof_av_delay);
					done = 1;
				}
			}
			else
			{	//User opted to seek to a measure
				lastselected = 3;
				if(input && (input <= measurecount))
				{	//The specified measure is valid
					measurecount = eof_get_measure(input, 0);	//Get the beat number for the start of this measure
					eof_set_seek_position(eof_song->beat[measurecount]->pos + eof_av_delay);
					done = 1;
				}
			}
		}
		else if((retval == 7) || (retval == -1))
		{	//User clicked cancel (or pressed escape)
			done = 1;
		}
	}
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	return 1;
}

int eof_menu_song_lock_tempo_map(void)
{
	if(eof_song)
		eof_song->tags->tempo_map_locked ^= 1;	//Toggle this boolean variable
	eof_fix_window_title();
	return 1;
}

int eof_menu_song_disable_double_bass_drums(void)
{
	if(eof_song)
		eof_song->tags->double_bass_drum_disabled ^= 1;	//Toggle this boolean variable
	eof_fix_window_title();
	return 1;
}

int eof_menu_song_seek_catalog_entry(void)
{
	if(eof_song && eof_song->catalog->entries && (eof_selected_catalog_entry < eof_song->catalog->entries))
	{	//If a catalog entry is selected
		eof_set_seek_position(eof_song->catalog->entry[eof_selected_catalog_entry].start_pos + eof_av_delay);
	}
	return 1;
}

int eof_find_note_sequence(EOF_SONG *sp, unsigned long target_track, unsigned long target_start, unsigned long target_size, unsigned long input_track, unsigned long input_diff, unsigned long start_pos, char direction, unsigned long *hit_pos)
{
	long input_note, next_note, match_count = 0, next_target_note = target_start, match_pos = 0;
	char start_found = 0, match;

	if(!sp || !hit_pos || (target_track >= sp->tracks) || (input_track >= sp->tracks))
		return 0;	//Return error

	//Find the first input note before/after the starting timestamp, depending on the search direction
	for(input_note = 0; input_note < eof_get_track_size(sp, input_track); input_note++)
	{	//For each note in the input track
		if(eof_get_note_type(sp, input_track, input_note) == input_diff)
		{	//If the note is in the target difficulty
			if(direction < 0)
			{	//If searching for the previous match
				if(eof_get_note_pos(sp, input_track, input_note) < start_pos)
				{	//And this note is before the starting timestamp
					next_note = eof_track_fixup_next_note(sp, input_track, input_note);
					if((next_note < 0) || (eof_get_note_pos(sp, input_track, next_note) >= start_pos))
					{	//And there is no next note, or the next note is at or after the starting timestamp, this is the first note before the timestamp
						start_found = 1;
						break;
					}
				}
			}
			else
			{	//If searching for the next match
				if(eof_get_note_pos(sp, input_track, input_note) > start_pos)
				{	//And this is the first note after the starting timestamp
					start_found = 1;
					break;
				}
			}
		}
	}

	//Search until the first/last note for a match, depending on the search direction
	while(start_found && (input_note < eof_get_track_size(sp, input_track)))
	{
		match = 0;
		if(!eof_note_compare(input_track, input_note, target_track, next_target_note))
		{	//If the next note of the target has been found, next pass of the loop will look for the next note in the target
			match = 1;
			if(!match_count)
			{	//The match was the first note in the target
				match_pos = input_note;	//Store the note number of the match
			}
			match_count++;	//Track the number of notes matched
			if(match_count == target_size)
			{	//If all notes in the target have been matched
				*hit_pos = eof_get_note_pos(sp, input_track, match_pos);	//Store the timestamp of the first note in the match
				return 1;	//Return match found
			}
			input_note = eof_track_fixup_next_note(sp, input_track, input_note);	//Next pass of the loop will examine the next note in the input
			next_target_note = eof_track_fixup_next_note(sp, target_track, next_target_note);	//Next pass of the loop will examine the next note in the target
			if((input_note < 0) || (next_target_note < 0))
			{	//If the input/target notes were exhausted during a partial match
				match = 0;	//This is not a match
			}
		}

		if(!match)
		{	//The note did not match, resume search starting from where the partial match began
			if(match_count != 0)
			{	//If a partial match had been found
				input_note = match_pos;	//Reset the input position to the start of the partial match
			}
			match_count = 0;
			if(direction < 0)
			{	//If searching for the previous match
				input_note = eof_track_fixup_previous_note(sp, input_track, input_note);	//Adjust the input position to the previous note
			}
			else
			{	//If searching for the next match
				input_note = eof_track_fixup_next_note(sp, input_track, input_note);		//Adjust the input position to the next note
			}
			next_target_note = target_start;	//Next pass of the loop will look for the first note in the target
		}
		if(input_note < 0)
		{	//If the input notes were exhausted
			return 0;	//Return no match found
		}
	}
	return 0;	//Return no match found
}

int eof_find_note_sequence_time_range(EOF_SONG *sp, unsigned long target_track, unsigned long target_diff, unsigned long target_start_pos, unsigned long target_end_pos, unsigned long input_track, unsigned long input_diff, unsigned long start_pos, char direction, unsigned long *hit_pos)
{
	unsigned long ctr, target_start = 0, target_size = 0, notepos;

	if(!sp || !hit_pos || (target_track >= sp->tracks))
		return 0;	//Return error

	//Find the first note and count the total number of notes within the specified time span
	for(ctr = 0; ctr < eof_get_track_size(sp, target_track); ctr++)
	{	//For each note in the target track
		if(eof_get_note_type(sp, target_track, ctr) == target_diff)
		{	//If the note is in the target difficulty
			notepos = eof_get_note_pos(sp, target_track, ctr);
			if((notepos >= target_start_pos) && (notepos <= target_end_pos))
			{	//If the note is within the target time span
				if(!target_size)
				{	//If this is the first note found in the time span
					target_start = ctr;	//Remember which note it was
				}
				target_size++;	//Count the number of notes found within the time span
			}
		}
	}

	if(!target_size)	//If there were no notes within the target time span
		return 0;	//Return no match found

	//Perform the search
	return eof_find_note_sequence(sp, target_track, target_start, target_size, input_track, input_diff, start_pos, direction, hit_pos);
}

int eof_menu_catalog_find(char direction)
{
	EOF_CATALOG_ENTRY *entry;
	unsigned long hit_pos;

	if(!eof_song || !eof_song->catalog->entries || (eof_selected_catalog_entry >= eof_song->catalog->entries))
		return 1;

	entry = &eof_song->catalog->entry[eof_selected_catalog_entry];	//Simplify the code below
	if(eof_find_note_sequence_time_range(eof_song, entry->track, entry->type, entry->start_pos, entry->end_pos, eof_selected_track, eof_note_type, eof_music_pos - eof_av_delay, direction, &hit_pos))
	{	//If a match was found
		eof_set_seek_position(hit_pos + eof_av_delay);	//Seek to the match position
	}
	return 1;
}

int eof_menu_catalog_find_prev(void)
{
	return eof_menu_catalog_find(-1);
}

int eof_menu_catalog_find_next(void)
{
	return eof_menu_catalog_find(1);
}

int eof_menu_catalog_toggle_full_width(void)
{
	if(eof_catalog_menu[1].flags == D_SELECTED)
	{	//If it was already enabled
		eof_catalog_menu[1].flags = 0;
	}
	else
	{
		eof_catalog_menu[1].flags = D_SELECTED;
	}
	return 1;
}
