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
#include "../ini_import.h"
#include "../dialog/proc.h"
#include "../undo.h"
#include "../player.h"
#include "../waveform.h"
#include "../spectrogram.h"
#include "../notefunc.h"
#include "../silence.h"
#include "../song.h"
#include "../tuning.h"
#include "../rs.h"	//For hand position generation logic
#include "edit.h"	//For eof_menu_edit_paste_from_difficulty()
#include "note.h"	//For eof_feedback_mode_update_note_selection()
#include "song.h"
#include "file.h"	//For eof_menu_prompt_save_changes()
#include "track.h"	//For tech view functions

#ifdef USEMEMWATCH
#include "../memwatch.h"
#endif

char eof_track_selected_menu_text[EOF_TRACKS_MAX][EOF_TRACK_NAME_SIZE+1] = {{0}};
	//Allow an extra leading space due to the checkmark erasing the first character in each string

char eof_raw_midi_track_error[] = "(error)";
struct eof_MIDI_data_track *eof_parsed_MIDI;
char eof_raw_midi_track_undo_made;

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

char eof_seek_menu_prev_anchor_text1[] = "Previous Anchor\t" CTRL_NAME "+Shift+PGUP";
char eof_seek_menu_next_anchor_text1[] = "Next Anchor\t" CTRL_NAME "+Shift+PGDN";
char eof_seek_menu_prev_anchor_text2[] = "Previous Anchor";
char eof_seek_menu_next_anchor_text2[] = "Next Anchor";
MENU eof_song_seek_menu[] =
{
	{"&Bookmark", NULL, eof_song_seek_bookmark_menu, 0, NULL},
	{"Start\tHome", eof_menu_song_seek_start, NULL, 0, NULL},
	{"End of audio\tEnd", eof_menu_song_seek_end, NULL, 0, NULL},
	{"End of chart\t" CTRL_NAME "+Shift+End", eof_menu_song_seek_chart_end, NULL, 0, NULL},
	{"&Catalog entry", eof_menu_song_seek_catalog_entry, NULL, 0, NULL},
	{"Rewind\tR", eof_menu_song_seek_rewind, NULL, 0, NULL},
	{"", NULL, NULL, 0, NULL},
	{"First Note\t" CTRL_NAME "+Home", eof_menu_song_seek_first_note, NULL, 0, NULL},
	{"Last Note\t" CTRL_NAME "+End", eof_menu_song_seek_last_note, NULL, 0, NULL},
	{"Previous Note\tShift+PGUP", eof_menu_song_seek_previous_note, NULL, 0, NULL},
	{"Next Note\tShift+PGDN", eof_menu_song_seek_next_note, NULL, 0, NULL},
	{"Previous h.l. note\t" CTRL_NAME "+Shift+Y", eof_menu_song_seek_previous_highlighted_note, NULL, 0, NULL},
	{"Next h.l. note\tShift+Y", eof_menu_song_seek_next_highlighted_note, NULL, 0, NULL},
	{"", NULL, NULL, 0, NULL},
	{"Previous Screen\t" CTRL_NAME "+PGUP", eof_menu_song_seek_previous_screen, NULL, 0, NULL},
	{"Next Screen\t" CTRL_NAME "+PGDN", eof_menu_song_seek_next_screen, NULL, 0, NULL},
	{"Previous Grid Snap\t" CTRL_NAME "+Shift+PGUP", eof_menu_song_seek_previous_grid_snap, NULL, 0, NULL},
	{"Next Grid Snap\t" CTRL_NAME "+Shift+PGDN", eof_menu_song_seek_next_grid_snap, NULL, 0, NULL},
	{"", NULL, NULL, 0, NULL},
	{"First Beat\t"  CTRL_NAME "+Shift+Home", eof_menu_song_seek_first_beat, NULL, 0, NULL},
	{"Previous Beat\tPGUP", eof_menu_song_seek_previous_beat, NULL, 0, NULL},
	{"Next Beat\tPGDN", eof_menu_song_seek_next_beat, NULL, 0, NULL},
	{eof_seek_menu_prev_anchor_text1, eof_menu_song_seek_previous_anchor, NULL, 0, NULL},
	{eof_seek_menu_next_anchor_text1, eof_menu_song_seek_next_anchor, NULL, 0, NULL},
	{"Beat/&Measure\t" CTRL_NAME "+Shift+B", eof_menu_song_seek_beat_measure, NULL, 0, NULL},
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
	{eof_track_selected_menu_text[12], eof_menu_track_selected_13, NULL, 0, NULL},
	{eof_track_selected_menu_text[13], eof_menu_track_selected_14, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_catalog_menu[] =
{
	{"&Show\tQ", eof_menu_catalog_show, NULL, 0, NULL},
	{"Double &Width\tSHIFT+Q", eof_menu_catalog_toggle_full_width, NULL, 0, NULL},
	{"&Edit Name", eof_menu_catalog_edit_name, NULL, 0, NULL},
	{"Edit &Timing", eof_menu_song_catalog_edit, NULL, 0, NULL},
	{"", NULL, NULL, 0, NULL},
	{"&Add\tSHIFT+W", eof_menu_catalog_add, NULL, 0, NULL},
	{"&Delete", eof_menu_catalog_delete, NULL, 0, NULL},
	{"", NULL, NULL, 0, NULL},
	{"&Previous\tW", eof_menu_catalog_previous, NULL, 0, NULL},
	{"&Next\tE", eof_menu_catalog_next, NULL, 0, NULL},
	{"", NULL, NULL, 0, NULL},
	{"Find Previous\tShift+F3", eof_menu_catalog_find_prev, NULL, 0, NULL},
	{"Find Next\tF3", eof_menu_catalog_find_next, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_waveform_menu[] =
{
	{"&Show\tF5", eof_menu_song_waveform, NULL, 0, NULL},
	{"&Configure", eof_menu_song_waveform_settings, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_spectrogram_menu[] =
{
	{"&Show", eof_menu_song_spectrogram, NULL, 0, NULL},
	{"&Configure", eof_menu_song_spectrogram_settings, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_song_proguitar_menu[] =
{
	{"Enable &Legacy view", eof_menu_song_legacy_view, NULL, 0, NULL},
	{"&Previous chord name\t" CTRL_NAME "+Shift+W", eof_menu_previous_chord_result, NULL, 0, NULL},
	{"&Next chord name\t" CTRL_NAME "+Shift+E", eof_menu_next_chord_result, NULL, 0, NULL},
	{"&Highlight notes in arpeggios", eof_menu_song_highlight_arpeggios, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_song_rocksmith_menu[] =
{
	{"&Correct chord fingerings", eof_correct_chord_fingerings_menu, NULL, 0, NULL},
	{"Check &Fret hand positions", eof_check_fret_hand_positions_menu, NULL, 0, NULL},
	{"", NULL, NULL, 0, NULL},
	{"Export chord techniques (RS1)", eof_menu_song_rocksmith_export_chord_techniques, NULL, 0, NULL},
	{"Suppress DD warnings", eof_menu_song_rocksmith_suppress_dynamic_difficulty_warnings, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_song_piano_roll_menu[] =
{
	{"&Display\tShift+Enter", eof_menu_song_toggle_second_piano_roll, NULL, 0, NULL},
	{"S&wap with main piano roll\t" CTRL_NAME "+Enter", eof_menu_song_swap_piano_rolls, NULL, 0, NULL},
	{"&Sync with main piano roll", eof_menu_song_toggle_piano_roll_sync, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_song_menu[] =
{
	{"&Seek", NULL, eof_song_seek_menu, 0, NULL},
	{"&Track", NULL, eof_track_selected_menu, 0, NULL},
	{"&File Info", eof_menu_song_file_info, NULL, 0, NULL},
	{"&Audio cues", eof_menu_audio_cues, NULL, 0, NULL},
	{"Display semitones as flat", eof_display_flats_menu, NULL, 0, NULL},
	{"Create image sequence", eof_write_image_sequence, NULL, 0, NULL},
	{"&Waveform Graph", NULL, eof_waveform_menu, 0, NULL},
	{"Spectrogra&m", NULL, eof_spectrogram_menu, 0, NULL},
	{"Highlight non grid snapped notes", eof_menu_song_highlight_non_grid_snapped_notes, NULL, 0, NULL},
	{"", NULL, NULL, 0, NULL},
	{"&Catalog", NULL, eof_catalog_menu, 0, NULL},
	{"&INI Settings", eof_menu_song_ini_settings, NULL, 0, NULL},
	{"Properties\tF9", eof_menu_song_properties, NULL, 0, NULL},
	{"&Leading Silence", eof_menu_song_add_silence, NULL, 0, NULL},
	{"Lock tempo map", eof_menu_song_lock_tempo_map, NULL, 0, NULL},
	{"&Disable click and drag", eof_menu_song_disable_click_drag, NULL, 0, NULL},
	{"Pro &Guitar", NULL, eof_song_proguitar_menu, 0, NULL},
	{"&Rocksmith", NULL, eof_song_rocksmith_menu, 0, NULL},
	{"Second &Piano roll", NULL, eof_song_piano_roll_menu, 0, NULL},
	{"Manage raw MIDI tracks", eof_menu_song_raw_MIDI_tracks, NULL, 0, NULL},
	{"Create pre&View audio", eof_menu_song_export_song_preview, NULL, 0, NULL},
	{"", NULL, NULL, 0, NULL},
	{"T&est in FOF", eof_menu_song_test_fof, NULL, EOF_LINUX_DISABLE, NULL},
	{"Test i&N Phase Shift", eof_menu_song_test_ps, NULL, EOF_LINUX_DISABLE, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

DIALOG eof_ini_dialog[] =
{
	/* (proc)             (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                 (dp2) (dp3) */
	{ d_agup_window_proc, 0,   48,  346, 232, 2,   23,  0,    0,      0,   0,   "INI Settings",      NULL, NULL },
	{ d_agup_list_proc,   12,  84,  240, 138, 2,   23,  0,    0,      0,   0,   (void *)eof_ini_list,NULL, NULL },
	{ d_agup_push_proc,   264, 84,  68,  28,  2,   23,  'a',  D_EXIT, 0,   0,   "&Add",              NULL, (void *)eof_ini_dialog_add },
	{ d_agup_push_proc,   264, 124, 68,  28,  2,   23,  'l',  D_EXIT, 0,   0,   "De&lete",           NULL, (void *)eof_ini_dialog_delete },
	{ d_agup_button_proc, 12,  235, 240, 28,  2,   23,  '\r', D_EXIT, 0,   0,   "Done",              NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_ini_add_dialog[] =
{
	/* (proc)             (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)              (dp2) (dp3) */
	{ d_agup_window_proc, 0,   48,  314, 106, 2,   23,  0,    0,      0,   0,   "Add INI Setting",NULL, NULL },
	{ d_agup_text_proc,   12,  84,  64,  8,   2,   23,  0,    0,      0,   0,   "Text:",          NULL, NULL },
	{ d_agup_edit_proc,   48,  80,  254, 20,  2,   23,  0,    0,      255, 0,   eof_etext,        NULL, NULL },
	{ d_agup_button_proc, 67,  112, 84,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",             NULL, NULL },
	{ d_agup_button_proc, 163, 112, 78,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",         NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

///When inserting items into this dialog menu that are before the loading text, update eof_popup_dialog() with the new indexes for the loading text related fields
DIALOG eof_song_properties_dialog[] =
{
	/* (proc)              (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                    (dp2) (dp3) */
	{ d_agup_window_proc,  0,   0,   480, 307, 0,   0,   0,    0,      0,   0,   "Song Properties",      NULL, NULL },
	{ d_agup_text_proc,    12,  40,  80,  12,  0,   0,   0,    0,      0,   0,   "Song Title",           NULL, NULL },
	{ d_agup_edit_proc,    12,  56,  184, 20,  0,   0,   0,    0,      255, 0,   eof_etext,              NULL, NULL },
	{ d_agup_text_proc,    12,  88,  96,  12,  0,   0,   0,    0,      0,   0,   "Artist",               NULL, NULL },
	{ d_agup_edit_proc,    12,  104, 184, 20,  0,   0,   0,    0,      255, 0,   eof_etext2,             NULL, NULL },
	{ d_agup_text_proc,    12,  136, 108, 12,  0,   0,   0,    0,      0,   0,   "Frettist",             NULL, NULL },
	{ d_agup_edit_proc,    12,  152, 184, 20,  0,   0,   0,    0,      255, 0,   eof_etext3,             NULL, NULL },
	{ d_agup_text_proc,    12,  184, 108, 12,  0,   0,   0,    0,      0,   0,   "Album",                NULL, NULL },
	{ d_agup_edit_proc,    12,  200, 184, 20,  0,   0,   0,    0,      255, 0,   eof_etext8,             NULL, NULL },
	{ d_agup_text_proc,    12,  232, 60,  12,  0,   0,   0,    0,      0,   0,   "Delay",                NULL, NULL },
	{ eof_verified_edit_proc,12,248, 50,  20,  0,   0,   0,    0,      6,   0,   eof_etext4,     "0123456789", NULL },
	{ d_agup_text_proc,    74,  232, 48,  12,  0,   0,   0,    0,      0,   0,   "Year",                 NULL, NULL },
	{ eof_verified_edit_proc,74,248, 38,  20,  0,   0,   0,    0,      4,   0,   eof_etext5,     "0123456789", NULL },
	{ d_agup_text_proc,    124, 232, 60,  12,  0,   0,   0,    0,      0,   0,   "Diff.",                NULL, NULL },
	{ eof_verified_edit_proc,124,248,26,  20,  0,   0,   0,    0,      1,   0,   eof_etext7,        "0123456", NULL },
	{ d_agup_text_proc,    208, 40,  96,  12,  0,   0,   0,    0,      0,   0,   "Loading Text",         NULL, NULL },
	{ d_agup_edit_proc,    208, 56,  256, 20,  0,   0,   0,    0,      255, 0,   eof_etext6,             NULL, NULL },
	{ d_agup_text_proc,    208, 88,  96,  12,  0,   0,   0,    0,      0,   0,   "Loading Text Preview", NULL, NULL },
	{ d_agup_textbox_proc, 208, 104, 256,116,  0,   0,   0,    0,      0,   0,   eof_etext6,             NULL, NULL },
	{ d_agup_check_proc,   160, 223, 136, 16,  0,   0,   0,    0,      1,   0,   "Lyrics",               NULL, NULL },
	{ d_agup_check_proc,   160, 238, 136, 16,  0,   0,   0,    0,      1,   0,   "8th Note HO/PO",       NULL, NULL },
	{ d_agup_check_proc,   160, 253, 215, 16,  0,   0,   0,    0,      1,   0,   "Use fret hand pos of 1 (pro g)", NULL, NULL },
	{ d_agup_check_proc,   160, 268, 215, 16,  0,   0,   0,    0,      1,   0,   "Use fret hand pos of 1 (pro b)", NULL, NULL },
	{ d_agup_check_proc,   160, 283, 200, 16,  0,   0,   0,    0,      1,   0,   "Use accurate time signatures", NULL, NULL },
	{ d_agup_button_proc,  380, 267, 84,  24,  0,   0,   '\r', D_EXIT, 0,   0,   "OK",                   NULL, NULL },
	{ NULL,                0,   0,   0,   0,   0,   0,   0,    0,      0,   0,   NULL,                   NULL, NULL }
};

void eof_prepare_song_menu(void)
{
	unsigned long i, tracknum;
	long firstnote = -1;
	long lastnote = -1;
	long noted[4] = {0};
	int seekp = 0, seekph = 0, seekn = 0, seeknh = 0;

	if(eof_song && eof_song_loaded)
	{//If a chart is loaded
		char bmcount = 0;

		tracknum = eof_song->track[eof_selected_track]->tracknum;
		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//For each note in the active track
			if(eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type)
			{
				if(firstnote < 0)
				{
					firstnote = i;
				}
				lastnote = i;
			}
			if(eof_get_note_type(eof_song, eof_selected_track, i) < 4)
			{
				noted[(unsigned)eof_get_note_type(eof_song, eof_selected_track, i)] = 1;	//Type cast to avoid a nag warning about indexing with a char type
			}
			if((eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type) && (eof_get_note_pos(eof_song, eof_selected_track, i) < ((eof_music_pos >= eof_av_delay) ? eof_music_pos - eof_av_delay : 0)))
			{	//If the note is earlier than the seek position
				seekp = 1;
				if(eof_note_is_highlighted(eof_song, eof_selected_track, i))
				{	//If either the static or dynamic highlight flag of this note is set
					seekph = 1;
				}
			}
			if((eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type) && (eof_get_note_pos(eof_song, eof_selected_track, i) > ((eof_music_pos >= eof_av_delay) ? eof_music_pos - eof_av_delay : 0)))
			{	//If the note is later than the seek position
				seekn = 1;
				if(eof_note_is_highlighted(eof_song, eof_selected_track, i))
				{	//If either the static or dynamic highlight flag of this note is set
					seeknh = 1;
				}
			}
		}

		/* track */
		for(i = 0; i < EOF_TRACKS_MAX; i++)
		{
			char *ptr = eof_track_selected_menu_text[i];	//Do this to work around false alarm warnings
			eof_track_selected_menu[i].flags = 0;
			if((i + 1 < EOF_TRACKS_MAX) && (i + 1 < eof_song->tracks) && (eof_song->track[i + 1] != NULL))
			{	//If the track exists, copy its name into the string used by the track menu
				ptr[0] = ' ';	//Add a leading space
				(void) ustrcpy(&(ptr[1]),eof_song->track[i+1]->name);
					//Append the track name to the menu string, starting at index 1
			}
			else
			{	//Write a blank string for the track name
				(void) ustrcpy(eof_track_selected_menu_text[i],"");
			}
		}
		eof_track_selected_menu[eof_selected_track-1].flags = D_SELECTED;	//Track numbering begins at 1 instead of 0

		/* seek start */
		if(eof_music_pos == eof_av_delay)
		{	//If the seek position is already at the start of the chart
			eof_song_seek_menu[1].flags = D_DISABLED;	//Seek start
			eof_song_seek_menu[20].flags = D_DISABLED;	//Previous beat
		}
		else
		{
			eof_song_seek_menu[1].flags = 0;
			eof_song_seek_menu[20].flags = 0;
		}

		/* seek end */
		if(eof_music_pos >= eof_music_length - 1)
		{	//If the seek position is already at the end of the chart
			eof_song_seek_menu[2].flags = D_DISABLED;	//Seek end
			eof_song_seek_menu[21].flags = D_DISABLED;	//Next beat
		}
		else
		{
			eof_song_seek_menu[2].flags = 0;
			eof_song_seek_menu[21].flags = 0;
		}

		/* show catalog */
		/* edit name */
		/* seek catalog entry */
		if(eof_song->catalog->entries > 0)
		{	//If there are any fret catalog entries
			eof_catalog_menu[0].flags = eof_catalog_menu[0].flags & D_SELECTED;	//Enable "Show Catalog" and check it if it's already checked
			eof_catalog_menu[2].flags = 0;		//Enable "Edit name"
			eof_catalog_menu[3].flags = 0;		//Enable "Edit timing"
			eof_song_seek_menu[4].flags = 0;	//Enable Seek>Catalog entry
		}
		else
		{
			eof_catalog_menu[0].flags = D_DISABLED;	//Disable "Show catalog"
			eof_catalog_menu[2].flags = D_DISABLED;	//Disable "Edit name"
			eof_catalog_menu[3].flags = D_DISABLED;	//Disable "Edit timing"
			eof_song_seek_menu[4].flags = D_DISABLED;	//Disable Seek>Catalog entry
		}

		/* rewind */
		if(eof_music_pos == eof_music_rewind_pos)
		{
			eof_song_seek_menu[5].flags = D_DISABLED;	//Rewind
		}
		else
		{
			eof_song_seek_menu[5].flags = 0;
		}
		if(eof_vocals_selected)
		{
			if(eof_song->vocal_track[tracknum]->lyrics)
			{
				/* seek first note */
				if(eof_song->vocal_track[tracknum]->lyric[0]->pos == eof_music_pos - eof_av_delay)
				{
					eof_song_seek_menu[7].flags = D_DISABLED;	//First note
				}
				else
				{
					eof_song_seek_menu[7].flags = 0;
				}

				/* seek last note */
				if(eof_song->vocal_track[tracknum]->lyric[eof_song->vocal_track[tracknum]->lyrics - 1]->pos == eof_music_pos - eof_av_delay)
				{
					eof_song_seek_menu[8].flags = D_DISABLED;	//Last note
				}
				else
				{
					eof_song_seek_menu[8].flags = 0;
				}
			}
			else
			{
				eof_song_seek_menu[7].flags = D_DISABLED;
				eof_song_seek_menu[8].flags = D_DISABLED;
			}
		}
		else
		{
			if(noted[eof_note_type])
			{
				/* seek first note */
				if((firstnote >= 0) && (eof_get_note_pos(eof_song, eof_selected_track, firstnote) == eof_music_pos - eof_av_delay))
				{
					eof_song_seek_menu[7].flags = D_DISABLED;	//First note
				}
				else
				{
					eof_song_seek_menu[7].flags = 0;
				}

				/* seek last note */
				if((lastnote >= 0) && (eof_get_note_pos(eof_song, eof_selected_track, lastnote) == eof_music_pos - eof_av_delay))
				{
					eof_song_seek_menu[8].flags = D_DISABLED;	//Last note
				}
				else
				{
					eof_song_seek_menu[8].flags = 0;
				}
			}
			else
			{
				eof_song_seek_menu[7].flags = D_DISABLED;
				eof_song_seek_menu[8].flags = D_DISABLED;
			}
		}

		/* seek previous note */
		if(seekp)
		{
			eof_song_seek_menu[9].flags = 0;	//Previous note
		}
		else
		{
			eof_song_seek_menu[9].flags = D_DISABLED;
		}

		/* seek next note */
		if(seekn)
		{
			eof_song_seek_menu[10].flags = 0;	//Next note
		}
		else
		{
			eof_song_seek_menu[10].flags = D_DISABLED;
		}

		/* seek previous highlighed note */
		if(seekph)
		{
			eof_song_seek_menu[11].flags = 0;	//Previous note
		}
		else
		{
			eof_song_seek_menu[11].flags = D_DISABLED;
		}

		/* seek next highlighted note */
		if(seeknh)
		{
			eof_song_seek_menu[12].flags = 0;	//Next note
		}
		else
		{
			eof_song_seek_menu[12].flags = D_DISABLED;
		}

		/* seek previous screen */
		if(eof_music_pos <= eof_av_delay)
		{
			eof_song_seek_menu[14].flags = D_DISABLED;	//Previous screen
		}
		else
		{
			eof_song_seek_menu[14].flags = 0;
		}

		/* seek next screen */
		if(eof_music_pos >= eof_music_length - 1)
		{
			eof_song_seek_menu[15].flags = D_DISABLED;	//Next screen
		}
		else
		{
			eof_song_seek_menu[15].flags = 0;
		}

		/* previous/next grid snap/anchor */
		if(eof_snap_mode == EOF_SNAP_OFF)
		{	//If grid snap is disabled
			eof_song_seek_menu[16].flags = D_DISABLED;	//Previous grid snap
			eof_song_seek_menu[17].flags = D_DISABLED;	//Next grid snap
			eof_song_seek_menu[22].text = eof_seek_menu_prev_anchor_text1;	//Display the previous anchor menu item with the shortcut
			eof_song_seek_menu[23].text = eof_seek_menu_next_anchor_text1;	//Display the next anchor menu item with the shortcut
		}
		else
		{
			eof_song_seek_menu[16].flags = 0;			//Previous grid snap
			eof_song_seek_menu[17].flags = 0;			//Next grid snap
			eof_song_seek_menu[22].text = eof_seek_menu_prev_anchor_text2;	//Display the previous anchor menu item with no shortcut
			eof_song_seek_menu[23].text = eof_seek_menu_next_anchor_text2;	//Display the next anchor menu item with no shortcut
		}

		/* seek bookmark # */
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
			eof_song_seek_menu[0].flags = D_DISABLED;	//Bookmark
		}
		else
		{
			eof_song_seek_menu[0].flags = 0;
		}

		/* display semitones as flat */
		if(eof_display_flats)
		{
			eof_song_menu[4].flags = D_SELECTED;	//Song>Display semitones as flat
		}
		else
		{
			eof_song_menu[4].flags = 0;
		}

		/* add catalog entry */
		if(eof_count_selected_notes(NULL))	//If there are notes selected
		{
			eof_catalog_menu[5].flags = 0;
		}
		else
		{
			eof_catalog_menu[5].flags = D_DISABLED;
		}

		/* remove catalog entry */
		/* find previous */
		/* find next */
		if(eof_selected_catalog_entry < eof_song->catalog->entries)
		{
			eof_catalog_menu[6].flags = 0;
			eof_catalog_menu[11].flags = 0;
			eof_catalog_menu[12].flags = 0;
		}
		else
		{
			eof_catalog_menu[6].flags = D_DISABLED;
			eof_catalog_menu[11].flags = D_DISABLED;
			eof_catalog_menu[12].flags = D_DISABLED;
		}

		/* previous/next catalog entry */
		if(eof_song->catalog->entries > 1)
		{
			eof_catalog_menu[8].flags = 0;
			eof_catalog_menu[9].flags = 0;
		}
		else
		{
			eof_catalog_menu[8].flags = D_DISABLED;
			eof_catalog_menu[9].flags = D_DISABLED;
		}

		/* catalog */
		if(!eof_song->catalog->entries && (eof_catalog_menu[5].flags == D_DISABLED))
		{	//If there are no catalog entries and no notes selected (in which case Song>Catalog>Add would have been disabled earlier)
			eof_song_menu[10].flags = D_DISABLED;	//Song>Catalog> submenu
		}
		else
		{
			eof_song_menu[10].flags = 0;
		}

		/* track */
		for(i = 0; i < EOF_TRACKS_MAX - 1; i++)
		{	//Access this array reflecting track numbering starting from 0 instead of 1
			if(eof_get_track_size(eof_song, i + 1) || eof_get_track_tech_note_size(eof_song, i + 1))
			{	//If the track exists and is populated with normal notes or tech notes, draw the track populated indicator
				eof_track_selected_menu[i].text[0] = '*';
			}
			else
			{	//Otherwise clear the indicator from the menu
				eof_track_selected_menu[i].text[0] = ' ';
			}
		}

		/* Highlight non grid snapped notes */
		if(eof_song->tags->highlight_unsnapped_notes)
		{	//If the user has enabled the dynamic highlighting of non grid snapped notes for this track
			eof_song_menu[8].flags = D_SELECTED;
		}
		else
		{
			eof_song_menu[8].flags = 0;
		}

		/* leading silence */
		if(eof_silence_loaded)
		{	//If no chart audio is loaded
			eof_song_menu[13].flags = D_DISABLED;	//Song>Leading silence
		}
		else
		{
			eof_song_menu[13].flags = 0;
		}

		/* lock tempo map */
		if(eof_song->tags->tempo_map_locked)
		{
			eof_song_menu[14].flags = D_SELECTED;	//Song>Lock tempo map
		}
		else
		{
			eof_song_menu[14].flags = 0;
		}

		/* disable click and drag */
		if(eof_song->tags->click_drag_disabled)
		{
			eof_song_menu[15].flags = D_SELECTED;	//Song>Disable click and drag
		}
		else
		{
			eof_song_menu[15].flags = 0;
		}

		/* enable pro guitar and rocksmith submenus */
		if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If a pro guitar track is active
			eof_song_menu[16].flags = 0;			//Song>Pro Guitar> submenu
			eof_song_menu[17].flags = 0;			//Song>Rocksmith> submenu

			if(eof_enable_chord_cache && (eof_chord_lookup_count > 1))
			{	//If an un-named note is selected and it has at least two chord matches
				eof_song_proguitar_menu[1].flags = 0;	//Previous chord name
				eof_song_proguitar_menu[2].flags = 0;	//Next chord name
			}
			else
			{
				eof_song_proguitar_menu[1].flags = D_DISABLED;
				eof_song_proguitar_menu[2].flags = D_DISABLED;
			}

			if(eof_song->tags->highlight_arpeggios)
			{
				eof_song_proguitar_menu[3].flags = D_SELECTED;	//Song>Pro Guitar>Highlight notes in arpeggios
			}
			else
			{
				eof_song_proguitar_menu[3].flags = 0;
			}
		}
		else
		{	//Otherwise disable these menu items
			eof_song_menu[16].flags = D_DISABLED;
			eof_song_menu[17].flags = D_DISABLED;
		}

		/* Second piano roll>Sync with main piano roll */
		if(eof_sync_piano_rolls)
		{
			eof_song_piano_roll_menu[2].flags = D_SELECTED;
		}
		else
		{
			eof_song_piano_roll_menu[2].flags = 0;
		}

		/* Export chord techniques */
		if(eof_song->tags->rs_chord_technique_export)
		{
			eof_song_rocksmith_menu[3].flags = D_SELECTED;
		}
		else
		{
			eof_song_rocksmith_menu[3].flags = 0;
		}

		/* Suppress DD warnings */
		if(eof_song->tags->rs_export_suppress_dd_warnings)
		{
			eof_song_rocksmith_menu[4].flags = D_SELECTED;
		}
		else
		{
			eof_song_rocksmith_menu[4].flags = 0;
		}
	}//If a chart is loaded
}

int eof_menu_song_seek_start(void)
{
	char wasplaying = 0;

	if(eof_music_catalog_playback)
		return 1;	//If the catalog is playing, return immediately

	if(!eof_music_paused)	//If the chart is already playing
	{
		eof_music_play(0);	//stop it
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
		eof_music_play(1);	//Resume playing using whatever playback rate was previously in use
	}

	return 1;
}

int eof_menu_song_seek_end(void)
{
	if(!eof_music_catalog_playback)
	{
		eof_set_seek_position(eof_music_length + eof_av_delay);
	}
	return 1;
}

int eof_menu_song_seek_chart_end(void)
{
	if(!eof_music_catalog_playback)
	{
		eof_set_seek_position(eof_chart_length + eof_av_delay);
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
			if((eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type) && (eof_get_note_pos(eof_song, eof_selected_track, i) < eof_chart_length))
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
		if((eof_get_note_type(eof_song, eof_selected_track, i - 1) == eof_note_type) && (eof_get_note_pos(eof_song, eof_selected_track, i - 1) < ((eof_music_pos >= eof_av_delay) ? eof_music_pos - eof_av_delay : 0)))
		{
			eof_set_seek_position(eof_get_note_pos(eof_song, eof_selected_track, i - 1) + eof_av_delay);
			break;
		}
	}
	return 1;
}

int eof_menu_song_seek_previous_highlighted_note(void)
{
	unsigned long i;

	for(i = eof_get_track_size(eof_song, eof_selected_track); i > 0; i--)
	{	//For each note in the active track
		if((eof_get_note_type(eof_song, eof_selected_track, i - 1) == eof_note_type) && (eof_get_note_pos(eof_song, eof_selected_track, i - 1) < ((eof_music_pos >= eof_av_delay) ? eof_music_pos - eof_av_delay : 0)))
		{
			if(eof_note_is_highlighted(eof_song, eof_selected_track, i - 1))
			{	//If either the static or dynamic highlight flag of this note is set
				eof_set_seek_position(eof_get_note_pos(eof_song, eof_selected_track, i - 1) + eof_av_delay);
				break;
			}
		}
	}
	return 1;
}

int eof_menu_song_seek_next_note(void)
{
	unsigned long i;

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type) && (eof_get_note_pos(eof_song, eof_selected_track, i) < eof_chart_length) && (eof_get_note_pos(eof_song, eof_selected_track, i) > ((eof_music_pos >= eof_av_delay) ? eof_music_pos - eof_av_delay : 0)))
		{
			eof_set_seek_position(eof_get_note_pos(eof_song, eof_selected_track, i) + eof_av_delay);
			break;
		}
	}
	return 1;
}

int eof_menu_song_seek_next_highlighted_note(void)
{
	unsigned long i;

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type) && (eof_get_note_pos(eof_song, eof_selected_track, i) < eof_chart_length) && (eof_get_note_pos(eof_song, eof_selected_track, i) > ((eof_music_pos >= eof_av_delay) ? eof_music_pos - eof_av_delay : 0)))
		{
			if(eof_note_is_highlighted(eof_song, eof_selected_track, i))
			{	//If either the static or dynamic highlight flag of this note is set
				eof_set_seek_position(eof_get_note_pos(eof_song, eof_selected_track, i) + eof_av_delay);
				break;
			}
		}
	}
	return 1;
}

int eof_menu_song_seek_previous_screen(void)
{
	if(!eof_music_catalog_playback)
	{
		if(eof_music_pos < SCREEN_W * eof_zoom)
		{	//Cannot seek left one full screen
			(void) eof_menu_song_seek_start();
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
		if(eof_music_pos + SCREEN_W * eof_zoom > eof_chart_length)
		{	//If the seek position is closer to the end of the chart than one more screen
			eof_set_seek_position(eof_chart_length + eof_av_delay);	//Seek to the end of the chart
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
	if(b >= EOF_MAX_BOOKMARK_ENTRIES)
		return 0;	//Invalid parameter

	if(!eof_music_catalog_playback && (eof_song->bookmark_pos[b] != 0))
	{
		eof_set_seek_position(eof_song->bookmark_pos[b] + eof_av_delay);
		return 1;
	}
	return 0;
}

int eof_menu_song_seek_bookmark_0(void)
{
	return eof_menu_song_seek_bookmark_help(0);
}

int eof_menu_song_seek_bookmark_1(void)
{
	return eof_menu_song_seek_bookmark_help(1);
}

int eof_menu_song_seek_bookmark_2(void)
{
	return eof_menu_song_seek_bookmark_help(2);
}

int eof_menu_song_seek_bookmark_3(void)
{
	return eof_menu_song_seek_bookmark_help(3);
}

int eof_menu_song_seek_bookmark_4(void)
{
	return eof_menu_song_seek_bookmark_help(4);
}

int eof_menu_song_seek_bookmark_5(void)
{
	return eof_menu_song_seek_bookmark_help(5);
}

int eof_menu_song_seek_bookmark_6(void)
{
	return eof_menu_song_seek_bookmark_help(6);
}

int eof_menu_song_seek_bookmark_7(void)
{
	return eof_menu_song_seek_bookmark_help(7);
}

int eof_menu_song_seek_bookmark_8(void)
{
	return eof_menu_song_seek_bookmark_help(8);
}

int eof_menu_song_seek_bookmark_9(void)
{
	return eof_menu_song_seek_bookmark_help(9);
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
	int i;

	if(!buffer)
		return 0;	//Invalid parameter

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
	unsigned long i, invalid = 0;
	unsigned long difficulty, undo_made = 0;
	char newlyrics, neweighth_note_hopo, neweof_fret_hand_pos_1_pg, neweof_fret_hand_pos_1_pb, oldaccurate_ts, newaccurate_ts;

	if(!eof_song_loaded || !eof_song)
		return 1;	//Do not allow this function to run if a chart is not loaded

	old_offset = eof_song->tags->ogg[eof_selected_ogg].midi_offset;
	oldaccurate_ts = eof_song->tags->accurate_ts;
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
	(void) ustrcpy(eof_etext, eof_song->tags->title);
	(void) ustrcpy(eof_etext2, eof_song->tags->artist);
	(void) ustrcpy(eof_etext3, eof_song->tags->frettist);
	(void) snprintf(eof_etext4, sizeof(eof_etext4) - 1, "%ld", eof_song->tags->ogg[eof_selected_ogg].midi_offset);
	(void) ustrcpy(eof_etext5, eof_song->tags->year);
	(void) ustrcpy(eof_etext6, eof_song->tags->loading_text);
	(void) ustrcpy(eof_etext8, eof_song->tags->album);
	eof_song_properties_dialog[19].flags = eof_song->tags->lyrics ? D_SELECTED : 0;
	eof_song_properties_dialog[20].flags = eof_song->tags->eighth_note_hopo ? D_SELECTED : 0;
	eof_song_properties_dialog[21].flags = eof_song->tags->eof_fret_hand_pos_1_pg ? D_SELECTED : 0;
	eof_song_properties_dialog[22].flags = eof_song->tags->eof_fret_hand_pos_1_pb ? D_SELECTED : 0;
	eof_song_properties_dialog[23].flags = eof_song->tags->accurate_ts ? D_SELECTED : 0;
	if(eof_song->tags->difficulty != 0xFF)
	{	//If there is a band difficulty defined, populate the band difficulty field
		(void) snprintf(eof_etext7, sizeof(eof_etext7) - 1, "%lu", eof_song->tags->difficulty);
	}
	else
	{	//Othewise leave the field blank
		eof_etext7[0] = '\0';
	}
	if(eof_popup_dialog(eof_song_properties_dialog, 2) == 24)
	{	//User clicked OK
		newlyrics = (eof_song_properties_dialog[19].flags & D_SELECTED) ? 1 : 0;
		neweighth_note_hopo = (eof_song_properties_dialog[20].flags & D_SELECTED) ? 1 : 0;
		neweof_fret_hand_pos_1_pg = (eof_song_properties_dialog[21].flags & D_SELECTED) ? 1 : 0;
		neweof_fret_hand_pos_1_pb = (eof_song_properties_dialog[22].flags & D_SELECTED) ? 1 : 0;
		newaccurate_ts = (eof_song_properties_dialog[23].flags & D_SELECTED) ? 1 : 0;
		if(ustricmp(eof_song->tags->title, eof_etext) || ustricmp(eof_song->tags->artist, eof_etext2) || ustricmp(eof_song->tags->frettist, eof_etext3) || ustricmp(eof_song->tags->year, eof_etext5) || ustricmp(eof_song->tags->loading_text, eof_etext6) || ustricmp(eof_song->tags->album, eof_etext8))
		{	//If any of the text fields were changed
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			undo_made = 1;
			if(ustricmp(eof_song->tags->title, eof_etext))
			{	//If the song title field was changed, delete the Rocksmith WAV file, since changing the title will cause a new WAV file to be written
				eof_delete_rocksmith_wav();
			}
		}
		else if(eof_is_number(eof_etext4) && (eof_song->tags->ogg[eof_selected_ogg].midi_offset != atol(eof_etext4)))
		{	//If the delay was changed
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			undo_made = 1;
		}
		else if((eof_song->tags->lyrics != newlyrics) || (eof_song->tags->eighth_note_hopo != neweighth_note_hopo) || (eof_song->tags->eof_fret_hand_pos_1_pg != neweof_fret_hand_pos_1_pg) || (eof_song->tags->eof_fret_hand_pos_1_pb != neweof_fret_hand_pos_1_pb) || (eof_song->tags->accurate_ts != newaccurate_ts))
		{	//If any of the checkboxes were changed
			undo_made = 1;
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		}
		if((ustrlen(eof_etext) > 255) || (ustrlen(eof_etext2) > 255) || (ustrlen(eof_etext3) > 255) || (ustrlen(eof_etext5) > 31) || (ustrlen(eof_etext6) > 511))
		{
			allegro_message("Text too large for allocated buffer!");
			return 1;
		}
		(void) ustrcpy(eof_song->tags->title, eof_etext);
		(void) ustrcpy(eof_song->tags->artist, eof_etext2);
		(void) ustrcpy(eof_song->tags->frettist, eof_etext3);
		(void) ustrcpy(eof_song->tags->year, eof_etext5);
		(void) ustrcpy(eof_song->tags->loading_text, eof_etext6);
		(void) ustrcpy(eof_song->tags->album, eof_etext8);
		eof_song->tags->lyrics = newlyrics;
		eof_song->tags->eighth_note_hopo = neweighth_note_hopo;
		eof_song->tags->eof_fret_hand_pos_1_pg = neweof_fret_hand_pos_1_pg;
		eof_song->tags->eof_fret_hand_pos_1_pb = neweof_fret_hand_pos_1_pb;
		eof_song->tags->accurate_ts = newaccurate_ts;
		(void) ustrcpy(eof_last_frettist, eof_etext3);
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
			eof_clear_input();
			if(alert(NULL, "Adjust notes to new offset?", NULL, "&Yes", "&No", 'y', 'n') == 1)
			{
				(void) eof_adjust_notes(eof_song->tags->ogg[eof_selected_ogg].midi_offset - old_offset);
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
		}
		eof_song->tags->difficulty = difficulty;	//Save the value reflected by the band difficulty field
		if(eof_song->tags->accurate_ts != oldaccurate_ts)
		{	//If the time signature handling was changed
			if(eof_song->beats >= 3)
			{	//Only check this if there are more than 2 beats
				unsigned num = 4, den = 4;
				for(i = 0; i < eof_song->beats - 1; i++)
				{	//For each beat in the chart
					(void) eof_get_ts(eof_song, &num, &den, i);	//Lookup any time signature defined at the beat
					if(den != 4)
					{	//If a denominator other than 4 is in effect
						break;
					}
				}
				if(i < eof_song->beats - 1)
				{	//If this condition is met, one of the time signatures used a TS with a denominator other than 4
					eof_clear_input();
					if(alert(NULL, "Alter tempos to keep beat positions intact?", NULL, "Yes", "No", 'y', 'n') == 1)
					{	//If the user opted to keep them intact
						eof_change_accurate_ts(eof_song, newaccurate_ts);
					}
				}
			}
		}
		eof_calculate_beats(eof_song);
		eof_truncate_chart(eof_song);
		eof_fixup_notes(eof_song);
		eof_fix_window_title();
	}//User clicked OK
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

	eof_log("eof_menu_song_test() entered", 1);

	if(!eof_song_loaded || !eof_song)
		return 1;	//Do not allow this function to run if a chart is not loaded

	if(application == 1)
	{	//If the user wants to test the chart in FoF
		/* check difficulty before allowing test */
		difficulty = eof_figure_difficulty();
		if(difficulty < 0)
		{
			(void) alert("Error", NULL, "No Notes!", "OK", NULL, 0, KEY_ENTER);
			eof_clear_input();
			return 1;
		}

		/* which part are we going to play */
		part = eof_figure_part();
		if(part < 0)
		{
			(void) alert("Error", NULL, "No Notes!", "OK", NULL, 0, KEY_ENTER);
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
	(void) eof_mkdir("EOFTemp");

	/* save temporary song */
	(void) ustrcpy(temppath, songs_path);
	(void) ustrcat(temppath, "EOFTemp\\");
	(void) append_filename(temppath2, temppath, "notes.eof", 1024);
	eof_sort_notes(eof_song);
	eof_fixup_notes(eof_song);
	if(!eof_save_song(eof_song, temppath2))
	{
		allegro_message("Song could not be tested!\nMake sure you set the %s song folder correctly (\"Link To %s\")!", appdisplayname, appdisplayname);
		get_executable_name(temppath, 1024);
		(void) replace_filename(temppath, temppath, "", 1024);
		(void) eof_chdir(temppath);
		eof_show_mouse(NULL);
		eof_cursor_visible = 1;
		eof_pen_visible = 1;
		return 1;
	}
	(void) append_filename(temppath2, temppath, "notes.mid", 1024);
	(void) eof_export_midi(eof_song, temppath2, 0, 1, 1, 0);	//Export the temporary MIDI, make pitchless/phraseless lyric corrections automatically
	(void) append_filename(temppath2, temppath, "song.ini", 1024);
	(void) eof_save_ini(eof_song, temppath2);
	(void) snprintf(syscommand, sizeof(syscommand) - 1, "%sguitar.ogg", eof_song_path);
	(void) snprintf(temppath2, sizeof(temppath2) - 1, "%sEOFTemp\\guitar.ogg", songs_path);
	(void) eof_copy_file(syscommand, temppath2);

	/* switch to the applicable rhythm game's program folder */
	(void) replace_filename(temppath, executablepath, "", 1024);
	if(eof_chdir(temppath))
	{
		allegro_message("Song could not be tested!\nMake sure you set the %s song folder correctly (\"Link To %s\")!", appdisplayname, appdisplayname);
		return 1;
	}

	/* execute appropriate application to launch chart */
	if(application == 1)
	{	//If the user wants to test the chart in FoF
		(void) ustrcpy(syscommand, executablename);
		(void) ustrcat(syscommand, " -p \"EOFTemp\" -D ");
		(void) uszprintf(temppath, (int) sizeof(temppath) - 1, "%d", difficulty);
		(void) ustrcat(syscommand, temppath);
		(void) ustrcat(syscommand, " -P ");
		(void) uszprintf(temppath, (int) sizeof(temppath) - 1, "%d", part);
		(void) ustrcat(syscommand, temppath);
		(void) eof_system(syscommand);
	}
	else
	{	//The user wants to test the chart in Phase Shift
		FILE *phaseshiftfp = fopen("launch_ps.bat", "wt");	//Write a batch file to launch Phase Shift

		if(phaseshiftfp)
		{
			(void) fprintf(phaseshiftfp, "cd %s\n", temppath);
			(void) replace_filename(temppath, temppath2, "", 1024);	//Get the path to the temporary chart's folder
			if(temppath[strlen(temppath)-1] == '\\')
			{	//Remove the trailing backslash because Phase Shift doesn't handle it correctly
				temppath[strlen(temppath)-1] = '\0';
			}
			(void) snprintf(syscommand, sizeof(syscommand) - 1, "\"%s\" \"%s\" /p", executablepath, temppath);
			(void) fprintf(phaseshiftfp, "%s", syscommand);
			(void) fclose(phaseshiftfp);
			(void) eof_system("launch_ps.bat");
			(void) delete_file("launch_ps.bat");
		}
	}

	/* switch to songs folder */
	if(eof_chdir(songs_path))
	{
		allegro_message("Cleanup failed!");
		return 1;
	}

	/* delete temporary song folder */
	(void) delete_file("EOFTemp\\guitar.ogg");
	(void) delete_file("EOFTemp\\notes.mid");
	(void) delete_file("EOFTemp\\song.ini");
	(void) delete_file("EOFTemp\\notes.eof");
	(void) eof_system("rd /s /q EOFTemp");

	/* switch back to EOF folder */
	get_executable_name(temppath, 1024);
	(void) replace_filename(temppath, temppath, "", 1024);
	(void) eof_chdir(temppath);

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
	return eof_menu_track_selected_track_number(1, 1);
}

int eof_menu_track_selected_2(void)
{
	return eof_menu_track_selected_track_number(2, 1);
}

int eof_menu_track_selected_3(void)
{
	return eof_menu_track_selected_track_number(3, 1);
}

int eof_menu_track_selected_4(void)
{
	return eof_menu_track_selected_track_number(4, 1);
}

int eof_menu_track_selected_5(void)
{
	return eof_menu_track_selected_track_number(5, 1);
}

int eof_menu_track_selected_6(void)
{
	return eof_menu_track_selected_track_number(6, 1);
}

int eof_menu_track_selected_7(void)
{
	return eof_menu_track_selected_track_number(7, 1);
}

int eof_menu_track_selected_8(void)
{
	return eof_menu_track_selected_track_number(8, 1);
}

int eof_menu_track_selected_9(void)
{
	return eof_menu_track_selected_track_number(9, 1);
}

int eof_menu_track_selected_10(void)
{
	return eof_menu_track_selected_track_number(10, 1);
}

int eof_menu_track_selected_11(void)
{
	return eof_menu_track_selected_track_number(11, 1);
}

int eof_menu_track_selected_12(void)
{
	return eof_menu_track_selected_track_number(12, 1);
}

int eof_menu_track_selected_13(void)
{
	return eof_menu_track_selected_track_number(13, 1);
}

int eof_menu_track_selected_14(void)
{
	return eof_menu_track_selected_track_number(14, 1);
}

int eof_menu_track_selected_track_number(unsigned long tracknum, int updatetitle)
{
	unsigned long i;
	unsigned char maxdiff = 4;	//By default, each track supports 5 difficulties

	eof_log("\tChanging active track", 2);
	eof_log("eof_menu_track_selected_track_number() entered", 2);

	if(!eof_song)
		return 0;	//Error

	//Store the active difficulty number into the appropriate variable
	if(eof_song->track[eof_selected_track]->track_format == EOF_VOCAL_TRACK_FORMAT)
	{	//If a vocal track is active
		eof_note_type_v = eof_note_type;	//Store the active difficulty number into the vocal difficulty variable
	}
	else
	{
		eof_note_type_i = eof_note_type;	//Store the active difficulty number into the instrument difficulty variable
	}

	if((tracknum > 0) && (tracknum < eof_song->tracks))
	{	//If the specified track number is valid
		for(i = 0; i < EOF_TRACKS_MAX; i++)
		{
			eof_track_selected_menu[i].flags = 0;
		}

		if(eof_song->track[tracknum]->track_format == EOF_VOCAL_TRACK_FORMAT)
		{
			eof_vocals_selected = 1;
			eof_note_type = eof_note_type_v;	//Retrieve the difficulty number last in effect when the vocal track was last active
			maxdiff = 0;			//For now, the default lyric set (PART VOCALS) will be selected when changing to PART VOCALS
		}
		else
		{
			eof_vocals_selected = 0;
			eof_note_type = eof_note_type_i;	//Retrieve the difficulty number last in effect when an instrument track was last active
			if(eof_song->track[tracknum]->flags & EOF_TRACK_FLAG_UNLIMITED_DIFFS)
			{	//If the track being changed to has its difficulty limit removed
				maxdiff = eof_song->track[tracknum]->numdiffs - 1;
			}
		}

		//Ensure the active difficulty is valid for the selected track
		if(eof_note_type > maxdiff)
		{
			eof_note_type = maxdiff;
		}

		//Track numbering begins at one instead of zero
		eof_track_selected_menu[tracknum-1].flags = D_SELECTED;
		eof_selected_track = tracknum;
		(void) eof_detect_difficulties(eof_song, eof_selected_track);
		if(updatetitle)
		{	//If the program window title is to be updated
			eof_fix_window_title();
		}
		eof_scale_fretboard(0);			//Recalculate the 2D screen positioning based on the current track
		eof_set_3D_lane_positions(0);
		eof_set_2D_lane_positions(0);
		eof_determine_phrase_status(eof_song, eof_selected_track);
		eof_chord_lookup_note = 0;	//Reset the cached chord lookup count
		eof_beat_stats_cached = 0;	//Have the beat statistics rebuilt
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
			if(size)
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
	unsigned long first_pos = 0, last_pos = 0, i;
	long next;
	char first = 1;

	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(!eof_count_selected_notes(NULL))	//If there are still no notes selected
		return 1;

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i] || (eof_get_note_type(eof_song, eof_selected_track, i) != eof_note_type))
			continue;	//If this note is not selected, skip it

		if(first)
		{	//If this was the first selected note found
			first_pos = eof_get_note_pos(eof_song, eof_selected_track, i);
			first = 0;
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
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	(void) eof_track_add_section(eof_song, 0, EOF_FRET_CATALOG_SECTION, eof_note_type, first_pos, last_pos, eof_selected_track, NULL);
	eof_music_catalog_pos = eof_song->catalog->entry[eof_selected_catalog_entry].start_pos + eof_av_delay;

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

	if(eof_song->catalog->entries == 0)
		return 1;	//If there are no catalog entries, return immediately

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
	if(eof_song->catalog->entries == 0)
	{
		eof_catalog_menu[0].flags = 0;
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

	if(!d)
	{	//Satisfy Splint by checking value of d
		return D_O_K;
	}
	if(eof_song->tags->ini_settings >= EOF_MAX_INI_SETTINGS)	//If the maximum number of INI settings is already defined
		return D_O_K;	//Return without adding anything

	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_ini_add_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_ini_add_dialog);
	(void) ustrcpy(eof_etext, "");

	if(eof_popup_dialog(eof_ini_add_dialog, 2) == 3)
	{
		if((ustrlen(eof_etext) > 0) && eof_check_string(eof_etext))
		{
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			(void) ustrncpy(eof_song->tags->ini_setting[eof_song->tags->ini_settings], eof_etext, EOF_INI_LENGTH - 1);
			eof_song->tags->ini_settings++;
		}
	}

	(void) dialog_message(eof_ini_dialog, MSG_DRAW, 0, &i);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(screen);
	return D_O_K;
}

void eof_ini_delete(unsigned long index)
{
	unsigned short i;

	if(!eof_song || (index >= eof_song->tags->ini_settings))
		return;	//Invalid parameter

	for(i = eof_ini_dialog[1].d1; i < eof_song->tags->ini_settings - 1; i++)
	{
		memcpy(eof_song->tags->ini_setting[i], eof_song->tags->ini_setting[i + 1], EOF_INI_LENGTH);
	}
	eof_song->tags->ini_settings--;
}

int eof_ini_dialog_delete(DIALOG * d)
{
	int i;

	if(!d)
	{	//Satisfy Splint by checking value of d
		return D_O_K;
	}
	if(eof_song->tags->ini_settings > 0)
	{
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		eof_ini_delete(eof_ini_dialog[1].d1);
		if(eof_ini_dialog[1].d1 >= eof_song->tags->ini_settings)
		{
			eof_ini_dialog[1].d1--;
		}
	}
	(void) dialog_message(eof_ini_dialog, MSG_DRAW, 0, &i);
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

int eof_menu_song_spectrogram(void)
{
	if(!eof_song_loaded)
		return 1;	//Return error

	if(eof_display_spectrogram == 0)
	{
		if(eof_music_paused)
		{	//Don't try to generate the spectrogram data if the chart is playing
			if(eof_spectrogram == NULL)
			{
				eof_spectrogram = eof_create_spectrogram(eof_loaded_ogg_name);	//Generate 1ms spectrogram data from the current audio file
			}
			else if(ustricmp(eof_spectrogram->oggfilename,eof_loaded_ogg_name) != 0)
			{	//If the user opened a different OGG file since the spectrogram data was generated
				eof_destroy_spectrogram(eof_spectrogram);
				eof_spectrogram = eof_create_spectrogram(eof_loaded_ogg_name);	//Generate 1ms spectrogram data from the current audio file
			}
		}

		if(eof_spectrogram != NULL)
		{
			eof_display_spectrogram = 1;
			eof_spectrogram_menu[0].flags = D_SELECTED;	//Check the Show item in the Song>Waveform graph menu
		}
	}
	else
	{
		eof_display_spectrogram = 0;
		eof_spectrogram_menu[0].flags = 0;	//Clear the Show item in the Song>Waveform graph menu
	}

	if(eof_music_paused)
		eof_render();
	eof_fix_window_title();

	return 0;	//Return success
}

DIALOG eof_leading_silence_dialog[] =
{
	/* (proc)                 (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags)    (d1)  (d2) (dp)                     (dp2) (dp3) */
	{ d_agup_window_proc,  	  0,   48,  200, 288, 2,   23,  0,    0,          0,   0,   "Leading Silence",       NULL, NULL },
	{ d_agup_text_proc,       16,  80,  110, 20,  2,   23,  0,    0,          0,   0,   "Add:",                  NULL, NULL },
	{ d_agup_radio_proc,      16,  100, 110, 15,  2,   23,  0,    0,          0,   0,   "Milliseconds",          NULL, NULL },
	{ d_agup_radio_proc,      16,  120, 110, 15,  2,   23,  0,    0,          0,   0,   "Beats",                 NULL, NULL },
	{ d_agup_text_proc,       16,  140, 110, 20,  2,   23,  0,    0,          0,   0,   "Pad:",                  NULL, NULL },
	{ d_agup_radio_proc,      16,  160, 110, 15,  2,   23,  0,    0,          0,   0,   "Milliseconds",          NULL, NULL },
	{ d_agup_radio_proc,      16,  180, 110, 15,  2,   23,  0,    0,          0,   0,   "Beats",                 NULL, NULL },
	{ eof_verified_edit_proc, 16,  200, 110, 20,  2,   23,  0,    0,          10,  0,   eof_etext,       "1234567890", NULL },
	{ d_agup_check_proc,      16,  226, 180, 16,  2,   23,  0,    D_SELECTED, 1,   0,   "Adjust Notes/Beats",    NULL, NULL },
	{ d_agup_radio_proc,      16,  246, 160, 15,  2,   23,  0,    0,          1,   0,   "Stream copy (oggCat)",  NULL, NULL },
	{ d_agup_radio_proc,      16,  266, 90,  15,  2,   23,  0,    0,          1,   0,   "Re-encode",             NULL, NULL },
	{ d_agup_button_proc,     16,  292, 68,  28,  2,   23,  '\r', D_EXIT,     0,   0,   "OK",                    NULL, NULL },
	{ d_agup_button_proc,     116, 292, 68,  28,  2,   23,  0,    D_EXIT,     0,   0,   "Cancel",                NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

static long get_ogg_length(const char * fn)
{
	ALOGG_OGG * ogg;
	unsigned long length = 0;
	void * oggbuffer;

	oggbuffer = eof_buffer_file(fn, 0);	//Decode the OGG from buffer instead of from file because the latter cannot support special characters in the file path due to limitations with fopen()
	if(!oggbuffer)
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading OGG:  \"%s\"", strerror(errno));	//Get the Operating System's reason for the failure
		eof_log(eof_log_string, 1);
		return 0;	//Return failure
	}
	ogg = alogg_create_ogg_from_buffer(oggbuffer, (int)file_size_ex(fn));
	if(ogg == NULL)
	{
		eof_log("ALOGG failed to open input audio file", 1);
		free(oggbuffer);
		return 0;	//Return failure
	}
	length = alogg_get_length_msecs_ogg_ul(ogg);
	alogg_destroy_ogg(ogg);
	free(oggbuffer);

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
	int retval;
	unsigned long old_eof_music_length = eof_music_length;	//Keep track of the current chart audio's length to compare with after silence was added

	eof_log("eof_menu_song_add_silence() entered", 1);

	if(eof_silence_loaded)
	{	//Do not allow this function to run when no audio is loaded
		return 1;
	}

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
	(void) snprintf(eof_etext, sizeof(eof_etext) - 1, "0");

	if(eof_popup_dialog(eof_leading_silence_dialog, 7) == 11)
	{	//User clicked OK
		eof_delete_rocksmith_wav();		//Delete the Rocksmith WAV file since changing silence will require a new WAV file to be written

		(void) snprintf(fn, sizeof(fn) - 1, "%s.backup", eof_loaded_ogg_name);
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
				(void) eof_copy_file(fn, eof_loaded_ogg_name);
				if(eof_load_ogg(eof_loaded_ogg_name, 0))
				{	//If the audio was loaded
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
		{	//Add silence
			eof_prepare_undo(EOF_UNDO_TYPE_SILENCE);
			if(eof_leading_silence_dialog[2].flags & D_SELECTED)
			{	//Add milliseconds
				silence_length = atoi(eof_etext);
			}
			else if(eof_leading_silence_dialog[3].flags & D_SELECTED)
			{	//Add beats
				beat_length = eof_calc_beat_length(eof_song, 0);
				silence_length = beat_length * (double)atoi(eof_etext);
			}
			else if(eof_leading_silence_dialog[5].flags & D_SELECTED)
			{	//Pad milliseconds
				silence_length = atoi(eof_etext);
				if(silence_length > eof_song->beat[0]->pos)
				{
					silence_length -= eof_song->beat[0]->pos;
				}
			}
			else if(eof_leading_silence_dialog[6].flags & D_SELECTED)
			{	//Pad beats
				beat_length = eof_calc_beat_length(eof_song, 0);
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
				retval = eof_add_silence(eof_loaded_ogg_name, silence_length);
			}
			else
			{	//User opted to re-encode
				(void) replace_filename(mp3fn, eof_song_path, "original.mp3", 1024);
				if(exists(mp3fn))
				{
					creationmethod = 10;	//Remember this as the default next time
					retval = eof_add_silence_recode_mp3(eof_loaded_ogg_name, silence_length);
				}
				else
				{
					creationmethod = 10;	//Remember this as the default next time
					retval = eof_add_silence_recode(eof_loaded_ogg_name, silence_length);
				}
			}
			if(retval)
			{	//If silence could not be inserted
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError code %d when adding leading silence", retval);
				eof_log(eof_log_string, 1);
				allegro_message("Could not add leading silence (error %d)", retval);
			}
			after_silence_length = get_ogg_length(eof_loaded_ogg_name);
			eof_song->tags->ogg[eof_selected_ogg].modified = 1;
		}//Add silence

		if(eof_music_length < old_eof_music_length)
		{	//If the operation malfunctioned (usually due to an incompatibility between OggCat and the chart audio)
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tLeading silence failure detected.  Chart audio went from %lums to %lums in length", old_eof_music_length, eof_music_length);
			eof_log(eof_log_string, 1);
			if(alert("Warning:  The leading silence operation seemed to have failed.", NULL, "Undo?", "&Yes", "&No", 'y', 'n') == 1)
			{	//If the user opts to undo the operation
				(void) eof_undo_apply();
			}
		}
		else
		{	//Operation succeeded, adjust notes/beats
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
				(void) eof_adjust_notes(adjust);
			}
			eof_fixup_notes(eof_song);
			eof_calculate_beats(eof_song);
			eof_fix_window_title();
			eof_truncate_chart(eof_song);	//Update number of beats and the chart length as appropriate
		}
	}//User clicked OK
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	return 1;
}

#define EOF_CUE_VOLUME_STRING_LEN 10
char eof_chart_volume_string[EOF_CUE_VOLUME_STRING_LEN] = "100%";
char eof_clap_volume_string[EOF_CUE_VOLUME_STRING_LEN] = "100%";
char eof_metronome_volume_string[EOF_CUE_VOLUME_STRING_LEN] = "100%";
char eof_tone_volume_string[EOF_CUE_VOLUME_STRING_LEN] = "100%";
char eof_percussion_volume_string[EOF_CUE_VOLUME_STRING_LEN] = "100%";
DIALOG eof_audio_cues_dialog[] =
{
/*	(proc)					(x)   (y)  (w)	(h) (fg) (bg) (key) (flags) (d1) (d2) (dp)                         (dp2) (dp3) */
	{ d_agup_window_proc,	0,	  48, 310, 350,   2,  23,    0,      0,   0,   0, "Audio cues",                 NULL, NULL },
	{ d_agup_text_proc,		16,	  88,  64,   8,   2,  23,    0,      0,   0,   0, "Chart volume",               NULL, NULL },
	{ d_agup_slider_proc,	176,  88,  96,  16,   2,  23,    0,      0, 100,   0, NULL,                         (void *)eof_set_cue_volume,	eof_chart_volume_string },
	{ d_agup_text_proc,		275,  88,  30,  16,   2,  23,    0,      0, 100,   0, eof_chart_volume_string,      NULL, NULL },
	{ d_agup_text_proc,		16,	 108,  64,   8,   2,  23,    0,      0,   0,   0, "Clap volume",                NULL, NULL },
	{ d_agup_slider_proc,	176, 108,  96,  16,   2,  23,    0,      0, 100,   0, NULL,                         (void *)eof_set_cue_volume,	eof_clap_volume_string },
	{ d_agup_text_proc,		275, 108,  30,  16,   2,  23,    0,      0, 100,   0, eof_clap_volume_string,       NULL, NULL },
	{ d_agup_text_proc,		16,  128,  64,   8,   2,  23,    0,      0,   0,   0, "Metronome volume",           NULL, NULL },
	{ d_agup_slider_proc,	176, 128,  96,  16,   2,  23,    0,      0, 100,   0, NULL,                         (void *)eof_set_cue_volume,	eof_metronome_volume_string },
	{ d_agup_text_proc,		275, 128,  30,  16,   2,  23,    0,      0, 100,   0, eof_metronome_volume_string,  NULL, NULL },
	{ d_agup_text_proc,		16,  148,  64,   8,   2,  23,    0,      0,   0,   0, "Tone volume",                NULL, NULL },
	{ d_agup_slider_proc,	176, 148,  96,  16,   2,  23,    0,      0, 100,   0, NULL,                         (void *)eof_set_cue_volume,	eof_tone_volume_string },
	{ d_agup_text_proc,		275, 148,  30,  16,   2,  23,    0,      0, 100,   0, eof_tone_volume_string,       NULL, NULL },
	{ d_agup_text_proc,		16,  168,  64,   8,   2,  23,    0,      0,   0,   0, "Vocal Percussion volume",    NULL, NULL },
	{ d_agup_slider_proc,	176, 168,  96,  16,   2,  23,    0,      0, 100,   0, NULL,                         (void *)eof_set_cue_volume,	eof_percussion_volume_string },
	{ d_agup_text_proc,		275, 168,  30,  16,   2,  23,    0,      0, 100,   0, eof_percussion_volume_string, NULL, NULL },
	{ d_agup_text_proc,		16,  188,  64,   8,   2,  23,    0,      0,   0,   0, "Vocal Percussion sound:",    NULL, NULL },
	{ d_agup_radio_proc,	16,  208,  68,  15,   2,  23,    0,      0,   0,   0, "Cowbell",                    NULL, NULL },
	{ d_agup_radio_proc,	124, 208,  84,  15,   2,  23,    0,      0,   0,   0, "Triangle 1",                 NULL, NULL },
	{ d_agup_radio_proc,	210, 208,  30,  15,   2,  23,    0,      0,   0,   0, "2",                          NULL, NULL },
	{ d_agup_radio_proc,	16,  228, 102,  15,   2,  23,    0,      0,   0,   0, "Tambourine 1",               NULL, NULL },
	{ d_agup_radio_proc,	124, 228,  30,  15,   2,  23,    0,      0,   0,   0, "2",                          NULL, NULL },
	{ d_agup_radio_proc,	160, 228,  30,  15,   2,  23,    0,      0,   0,   0, "3",                          NULL, NULL },
	{ d_agup_radio_proc,	16,  248, 110,  15,   2,  23,    0,      0,   0,   0, "Wood Block 1",               NULL, NULL },
	{ d_agup_radio_proc,	124, 248,  30,  15,   2,  23,    0,      0,   0,   0, "2",                          NULL, NULL },
	{ d_agup_radio_proc,	160, 248,  30,  15,   2,  23,    0,      0,   0,   0, "3",                          NULL, NULL },
	{ d_agup_radio_proc,	196, 248,  30,  15,   2,  23,    0,      0,   0,   0, "4",                          NULL, NULL },
	{ d_agup_radio_proc,	16,  268,  30,  15,   2,  23,    0,      0,   0,   0, "5",                          NULL, NULL },
	{ d_agup_radio_proc,	52,  268,  30,  15,   2,  23,    0,      0,   0,   0, "6",                          NULL, NULL },
	{ d_agup_radio_proc,	88,  268,  30,  15,   2,  23,    0,      0,   0,   0, "7",                          NULL, NULL },
	{ d_agup_radio_proc,	124, 268,  30,  15,   2,  23,    0,      0,   0,   0, "8",                          NULL, NULL },
	{ d_agup_radio_proc,	160, 268,  30,  15,   2,  23,    0,      0,   0,   0, "9",                          NULL, NULL },
	{ d_agup_radio_proc,	196, 268,  40,  15,   2,  23,    0,      0,   0,   0, "10",                         NULL, NULL },
	{ d_agup_radio_proc,	16,  288,  68,  15,   2,  23,    0,      0,   0,   0, "Clap 1",                     NULL, NULL },
	{ d_agup_radio_proc,	88,  288,  30,  15,   2,  23,    0,      0,   0,   0, "2",                          NULL, NULL },
	{ d_agup_radio_proc,	124, 288,  30,  15,   2,  23,    0,      0,   0,   0, "3",                          NULL, NULL },
	{ d_agup_radio_proc,	160, 288,  30,  15,   2,  23,    0,      0,   0,   0, "4",                          NULL, NULL },
	{ d_agup_check_proc,	16,  308, 178,  16,   2,  23,    0,      0,   1,   0, "String mutes trigger clap",  NULL, NULL },
	{ d_agup_check_proc,	16,  328, 178,  16,   2,  23,    0,      0,   2,   0, "Ghost notes trigger clap",   NULL, NULL },
	{ d_agup_button_proc,	44,  354,  68,  28,   2,  23, '\r', D_EXIT,   0,   0, "OK",                         NULL, NULL },
	{ d_agup_button_proc,	172, 354,  68,  28,   2,  23,    0, D_EXIT,   0,   0, "Cancel",                     NULL, NULL },
	{ NULL,                   0,   0,   0,   0,   0,   0,    0,      0,   0,   0, NULL,                         NULL, NULL }
};

int eof_set_cue_volume(void *dp3, int d2)
{
	//Validate input
	if(dp3 == NULL)
		return 1;
	if((d2 < 0) || (d2 > 100))
		return 1;

	(void) snprintf((char *)dp3, EOF_CUE_VOLUME_STRING_LEN - 1, "%3d%%", d2);	//Rewrite the specified volume slider string
	(void) object_message(&eof_audio_cues_dialog[3], MSG_DRAW, 0);			//Have Allegro redraw the volume slider strings
	(void) object_message(&eof_audio_cues_dialog[6], MSG_DRAW, 0);
	(void) object_message(&eof_audio_cues_dialog[9], MSG_DRAW, 0);
	(void) object_message(&eof_audio_cues_dialog[12], MSG_DRAW, 0);
	(void) object_message(&eof_audio_cues_dialog[15], MSG_DRAW, 0);
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

	eof_audio_cues_dialog[37].flags = eof_clap_for_mutes ? D_SELECTED : 0;	//Update the "String mutes trigger clap" option
	eof_audio_cues_dialog[38].flags = eof_clap_for_ghosts ? D_SELECTED : 0;	//Update the "Ghost notes trigger clap" option

	//Rebuild the volume slider strings, as they are not guaranteed to be all 100% on launch of EOF since EOF stores the user's last-configured volumes in the config file
	(void) eof_set_cue_volume(eof_chart_volume_string, eof_chart_volume);
	(void) eof_set_cue_volume(eof_clap_volume_string, eof_clap_volume);
	(void) eof_set_cue_volume(eof_metronome_volume_string, eof_tick_volume);
	(void) eof_set_cue_volume(eof_tone_volume_string, eof_tone_volume);
	(void) eof_set_cue_volume(eof_percussion_volume_string, eof_percussion_volume);

	if(eof_popup_dialog(eof_audio_cues_dialog, 0) == 39)			//User clicked OK
	{
		eof_chart_volume = eof_audio_cues_dialog[2].d2;				//Store the volume set by the chart volume slider
		eof_chart_volume_multiplier = sqrt(eof_chart_volume/100.0);	//Store this math so it only needs to be performed once
		eof_clap_volume = eof_audio_cues_dialog[5].d2;				//Store the volume set by the clap cue volume slider
		eof_tick_volume = eof_audio_cues_dialog[8].d2;				//Store the volume set by the tick cue volume slider
		eof_tone_volume = eof_audio_cues_dialog[11].d2;				//Store the volume set by the tone cue volume slider
		eof_percussion_volume = eof_audio_cues_dialog[14].d2;		//Store the volume set by the vocal percussion cue volume slider
		eof_clap_for_mutes = eof_audio_cues_dialog[37].flags == D_SELECTED ? 1 : 0;	//Store the "String mutes trigger clap" option
		eof_clap_for_ghosts = eof_audio_cues_dialog[38].flags == D_SELECTED ? 1 : 0;	//Store the "Ghost notes trigger clap" option

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

int eof_display_flats_menu(void)
{
	eof_log("eof_display_flats_menu() entered", 1);

	if(eof_display_flats)
	{
		eof_display_flats = 0;
		eof_note_names = eof_note_names_sharp;	//Switch to displaying guitar chords with sharps
		eof_slash_note_names = eof_slash_note_names_sharp;
	}
	else
	{
		eof_display_flats = 1;
		eof_note_names = eof_note_names_flat;	//Switch to displaying guitar chords with flats
		eof_slash_note_names = eof_slash_note_names_flat;
	}
	return 1;
}

DIALOG eof_waveform_settings_dialog[] =
{
	/* (proc)             (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                        (dp2) (dp3) */
	{ d_agup_window_proc, 0,   48,  200, 200, 2,   23,  0,    0,      0,   0,   "Configure Waveform Graph", NULL, NULL },
	{ d_agup_text_proc,   16,  80,  64,  8,	  2,   23,  0,    0,      0,   0,   "Fit into:",                NULL, NULL },
	{ d_agup_radio_proc,  16,  100, 110, 15,  2,   23,  0,    0,      0,   0,   "Fretboard area",           NULL, NULL },
	{ d_agup_radio_proc,  16,  120, 110, 15,  2,   23,  0,    0,      0,   0,   "Editor window",            NULL, NULL },
	{ d_agup_text_proc,   16,  140, 80,  16,  2,   23,  0,    0,	  1,   0,   "Display channels:",        NULL, NULL },
	{ d_agup_check_proc,  16,  160, 45,  16,  2,   23,  0,    0,	  1,   0,   "Left",                     NULL, NULL },
	{ d_agup_check_proc,  16,  180, 55,  16,  2,   23,  0,    0,	  1,   0,   "Right",                    NULL, NULL },
	{ d_agup_button_proc, 16,  208, 68,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",                       NULL, NULL },
	{ d_agup_button_proc, 116, 208, 68,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",                   NULL, NULL },
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

int eof_spectrogram_settings_wsize[] = {32,128,512,1024,2048};
DIALOG eof_spectrogram_settings_dialog[] =
{
	/* (proc)				(x)	(y)		(w)	(h)		(fg)(bg)(key)	(flags)	(d1)(d2)(dp)						(dp2)(dp3) */
	{ d_agup_window_proc,	0,	48,		260,290,	2,	23,	0,		0,		0,	0,	"Configure Spectrogram",	NULL,NULL },
	{ d_agup_text_proc,		16,	80,		64,	8,		2,	23,	0,		0,		0,	0,	"Fit into:",				NULL,NULL },
	{ d_agup_radio_proc,	16,	100,	110,15,		2,	23,	0,		0,		0,	0,	"Fretboard area",			NULL,NULL },
	{ d_agup_radio_proc,	16,	120,	110,15,		2,	23,	0,		0,		0,	0,	"Editor window",			NULL,NULL },
	{ d_agup_text_proc,		16,	140,	80,	16,		2,	23,	0,		0,		1,	0,	"Display channels:",		NULL,NULL },
	{ d_agup_check_proc,	16,	160,	45,	16,		2,	23,	0,		0,		1,	0,	"Left",						NULL,NULL },
	{ d_agup_check_proc,	16,	180,	55,	16,		2,	23,	0,		0,		1,	0,	"Right",					NULL,NULL },
	{ d_agup_text_proc,		136,80,		64,	8,		2,	23,	0,		0,		0,	0,	"Window size:",				NULL,NULL },
	{ d_agup_radio_proc,	136,100,	45,	15,		2,	23,	0,		0,		1,	0,	"128",						NULL,NULL },
	{ d_agup_radio_proc,	136,120,	45,	15,		2,	23,	0,		0,		1,	0,	"512",						NULL,NULL },
	{ d_agup_radio_proc,	136,140,	45,	15,		2,	23,	0,		0,		1,	0,	"1024",						NULL,NULL },
	{ d_agup_radio_proc,	136,160,	45,	15,		2,	23,	0,		0,		1,	0,	"2048",						NULL,NULL },
	{ d_agup_radio_proc,	136,180,	69,	15,		2,	23,	0,		0,		1,	0,	"Custom",					NULL,NULL },
	{ d_agup_edit_proc,		205,177,	50,	20,		0,	0,	0,		0,		255,0,	eof_etext,					NULL,NULL },
	{ d_agup_text_proc,		16,	205,	100,8,		2,	23,	0,		0,		0,	0,	"Color scheme:",			NULL,NULL },
	{ d_agup_radio_proc,	16,	225,	100,15,		2,	23,	0,		0,		2,	0,	"Grayscale",				NULL,NULL },
	{ d_agup_radio_proc,	16,	245,	100,15,		2,	23,	0,		0,		2,	0,	"Color",					NULL,NULL },
	{ d_agup_check_proc,	136,205,	100,16,		2,	23,	0,		0,		1,	0,	"Note range:",				NULL,NULL },
	{ d_agup_edit_proc,		136,225,	40,	15,		2,	23,	0,		0,		3,	0,	eof_etext2,					NULL,NULL },
	{ d_agup_text_proc,		180,230,	15,	8,		2,	23,	0,		0,		0,	0,	"to",						NULL,NULL },
	{ d_agup_edit_proc,		196,225,	40,	15,		2,	23,	0,		0,		3,	0,	eof_etext3,					NULL,NULL },
	{ d_agup_check_proc,	136,270,	120,16,		2,	23,	0,		0,		1,	0,	"Avg freq (slow)",			NULL,NULL },
	{ d_agup_check_proc,	16,	270,	100,16,		2,	23,	0,		0,		1,	0,	"Log plot",					NULL,NULL },

	{ d_agup_button_proc,	16,	298,	68,	28,		2,	23,	'\r',	D_EXIT,	0,	0,	"OK",						NULL,NULL },
	{ d_agup_button_proc,	116,298,	68,	28,		2,	23,	0,		D_EXIT,	0,	0,	"Cancel",					NULL,NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

int eof_menu_song_spectrogram_settings(void)
{
	int i;
	int prev_windowsize;
	char custom_windowsize;
	char is_dirty = 0;
	double newfreq;
	char prevchk;

	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	eof_color_dialog(eof_spectrogram_settings_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_spectrogram_settings_dialog);

	eof_spectrogram_settings_dialog[2].flags = eof_spectrogram_settings_dialog[3].flags = 0;
	if(eof_spectrogram_renderlocation == 0)
	{	//If fit into fretboard is active
		eof_spectrogram_settings_dialog[2].flags = D_SELECTED;
	}
	else
	{	//If fit into editor window is active
		eof_spectrogram_settings_dialog[3].flags = D_SELECTED;
	}

	eof_spectrogram_settings_dialog[5].flags = eof_spectrogram_settings_dialog[6].flags = 0;
	if(eof_spectrogram_renderleftchannel)
	{	//If the left channel is selected to be rendered
		eof_spectrogram_settings_dialog[5].flags = D_SELECTED;
	}
	if(eof_spectrogram_renderrightchannel)
	{	//If the left channel is selected to be rendered
		eof_spectrogram_settings_dialog[6].flags = D_SELECTED;
	}

	eof_spectrogram_settings_dialog[8].flags
		= eof_spectrogram_settings_dialog[9].flags
		= eof_spectrogram_settings_dialog[10].flags
		= eof_spectrogram_settings_dialog[11].flags
		= eof_spectrogram_settings_dialog[12].flags
		= 0;
	eof_etext[0] = '\0';

	custom_windowsize = 1;
	for(i=8;i<12;i++)
	{
		if(eof_spectrogram_windowsize == eof_spectrogram_settings_wsize[i-8])
		{
			eof_spectrogram_settings_dialog[i].flags = D_SELECTED;
			custom_windowsize = 0;
		}
	}
	if(custom_windowsize)
	{
		eof_spectrogram_settings_dialog[12].flags = D_SELECTED;
		(void) snprintf(eof_etext, sizeof(eof_etext) - 1, "%d", eof_spectrogram_windowsize);
	}

	//Set up the color scheme
	eof_spectrogram_settings_dialog[15].flags = eof_spectrogram_settings_dialog[16].flags = 0;
	eof_spectrogram_settings_dialog[15 + eof_spectrogram_colorscheme].flags = D_SELECTED;

	//Set up the note range
	strncpy(eof_etext2, notefunc_freq_to_note(eof_spectrogram_startfreq), 10);
	strncpy(eof_etext3, notefunc_freq_to_note(eof_spectrogram_endfreq), 10);

	if(eof_spectrogram_userange)
		eof_spectrogram_settings_dialog[17].flags = D_SELECTED;

	if(eof_spectrogram_avgbins)
		eof_spectrogram_settings_dialog[21].flags = D_SELECTED;

	if(eof_spectrogram_logplot)
		eof_spectrogram_settings_dialog[22].flags = D_SELECTED;

	if(eof_popup_dialog(eof_spectrogram_settings_dialog, 0) == 23)
	{ //User clicked OK
		if(eof_spectrogram_settings_dialog[2].flags == D_SELECTED)
		{	//User selected to render into fretboard area
			eof_spectrogram_renderlocation = 0;
		}
		else
		{	//User selected to render into editor window
			eof_spectrogram_renderlocation = 1;
		}
		if(eof_spectrogram_settings_dialog[5].flags == D_SELECTED)
		{	//User selected to render the left channel
			eof_spectrogram_renderleftchannel = 1;
		}
		else
		{
			eof_spectrogram_renderleftchannel = 0;
		}
		if(eof_spectrogram_settings_dialog[6].flags == D_SELECTED)
		{	//User selected to render the right channel
			eof_spectrogram_renderrightchannel = 1;
		}
		else
		{
			eof_spectrogram_renderrightchannel = 0;
		}

		prev_windowsize = eof_spectrogram_windowsize;
		//Run through the window sizes
		for(i=8;i<12;i++)
		{
			if(eof_spectrogram_settings_dialog[i].flags == D_SELECTED)
			{
				eof_spectrogram_windowsize = eof_spectrogram_settings_wsize[i-8];
			}
		}
		//Check for the Other box
		if(eof_spectrogram_settings_dialog[12].flags == D_SELECTED)
		{
			eof_spectrogram_windowsize = atoi(eof_etext);
			if(eof_spectrogram_windowsize == 0)
			{
				eof_spectrogram_windowsize = 1024;
			}
		}
		eof_half_spectrogram_windowsize = (double)eof_spectrogram_windowsize / 2.0;	//Cache this value so it isn't calculated for every rendered column of the spectrogram

		//Recreate the spectrogram if we changed the window size
		if((eof_spectrogram_windowsize != prev_windowsize) && (eof_spectrogram != NULL))
		{
			eof_destroy_spectrogram(eof_spectrogram);
			eof_spectrogram = eof_create_spectrogram(eof_loaded_ogg_name);
		}

		//Run through the color options
		for(i=15;i<17;i++)
		{
			if(eof_spectrogram_settings_dialog[i].flags == D_SELECTED)
			{
				eof_spectrogram_colorscheme = i-15;
			}
		}

		//Parse the note names
		newfreq = notefunc_note_to_freq(eof_etext2);
		if((eof_spectrogram_startfreq < newfreq) || (eof_spectrogram_startfreq > newfreq))
		{	//If the returned value is different than the old value (floating point == and != comparisons are not considered reliable)
			is_dirty = 1;
		}
		eof_spectrogram_startfreq = newfreq;

		newfreq = notefunc_note_to_freq(eof_etext3);
		if((eof_spectrogram_endfreq < newfreq) || (eof_spectrogram_endfreq > newfreq))
		{	//If the returned value is different than the old value (floating point == and != comparisons are not considered reliable)
			is_dirty = 1;
		}
		eof_spectrogram_endfreq = newfreq;

		//Set the various checkboxes
		prevchk	= eof_spectrogram_userange;
		eof_spectrogram_userange = (eof_spectrogram_settings_dialog[17].flags == D_SELECTED);
		if(eof_spectrogram_userange != prevchk) { is_dirty = 1; }

		eof_spectrogram_avgbins = (eof_spectrogram_settings_dialog[21].flags == D_SELECTED);

		prevchk = eof_spectrogram_logplot;
		eof_spectrogram_logplot = (eof_spectrogram_settings_dialog[22].flags == D_SELECTED);
		if(eof_spectrogram_logplot != prevchk) { is_dirty = 1; }

		//Check if the various things that require a re-gen of the
		//px_to_freq table, and if so set the dirty flag
		if(is_dirty && (eof_spectrogram != NULL))
		{
			eof_spectrogram->px_to_freq.dirty = 1;
		}
	}
	eof_fix_window_title();
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	return 1;
}

DIALOG eof_catalog_entry_name_dialog[] =
{
	/* (proc)         (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)           (dp2) (dp3) */
	{ d_agup_window_proc,  0,   48,  314, 106, 2,   23,   0,      0,      0,   0,   "Edit catalog entry name",               NULL, NULL },
	{ d_agup_text_proc,    12,  84,  64,  8,   2,   23,   0,      0,      0,   0,   "Text:",         NULL, NULL },
	{ d_agup_edit_proc,    48,  80,  254, 20,  2,   23,   0,      0,      EOF_NAME_LENGTH,   0,   eof_etext,           NULL, NULL },
	{ d_agup_button_proc,  67,  112, 84,  28,  2,   23,   '\r',   D_EXIT, 0,   0,   "OK",               NULL, NULL },
	{ d_agup_button_proc,  163, 112, 78,  28,  2,   23,   0,      D_EXIT, 0,   0,   "Cancel",           NULL, NULL },
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
		(void) ustrcpy(eof_etext, eof_song->catalog->entry[eof_selected_catalog_entry].name);
		if(eof_popup_dialog(eof_catalog_entry_name_dialog, 2) == 3)	//User hit OK
		{
			if(ustrcmp(eof_song->catalog->entry[eof_selected_catalog_entry].name, eof_etext))	//If the updated string (eof_etext) is different
			{
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				(void) ustrcpy(eof_song->catalog->entry[eof_selected_catalog_entry].name, eof_etext);
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
		eof_song_proguitar_menu[0].flags = 0;	//Song>Pro Guitar>Enable legacy view
		eof_scale_fretboard(0);	//Recalculate the 2D screen positioning based on the current track
	}
	else
	{
		eof_legacy_view = 1;
		eof_song_proguitar_menu[0].flags = D_SELECTED;
		eof_scale_fretboard(5);	//Recalculate the 2D screen positioning based on a 5 lane track
	}
	eof_fix_window_title();
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

		default:
		break;
	}
	eof_selected_percussion_cue = cue_number;
}

void eof_seek_by_grid_snap(int dir)
{
	unsigned long adjustedpos = eof_music_pos - eof_av_delay;	//Find the actual chart position
	unsigned long originalpos = adjustedpos;
	unsigned long target = 0;

	if(!eof_find_grid_snap(adjustedpos, dir, &target))
	{	//If the seek position couldn't be found
		return;
	}
	eof_set_seek_position(target + eof_av_delay);	//Seek to the position that was determined to be the previous/next grid snap position

	if((eof_input_mode == EOF_INPUT_FEEDBACK) && (KEY_EITHER_SHIFT))
	{
		eof_shift_used = 1;	//Track that the SHIFT key was used
		if(eof_seek_selection_start == eof_seek_selection_end)
		{	//If this begins a seek selection
			eof_update_seek_selection(originalpos, eof_music_pos - eof_av_delay, 0);
		}
		else
		{
			eof_update_seek_selection(eof_seek_selection_start, eof_music_pos - eof_av_delay, 0);
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
	unsigned long b;

	if(!eof_song)
		return 1;

	b = eof_get_beat(eof_song, eof_music_pos - eof_av_delay);
	if(!eof_beat_num_valid(eof_song, b))
		return 1;	//If the beta containing the seek position was not identified, return immediately

	if((eof_song->beat[b]->pos < eof_music_pos - eof_av_delay) && (eof_song->beat[b]->flags & EOF_BEAT_FLAG_ANCHOR))
	{	//If the seek position is within (but not at the start of) a beat that is an anchor
		eof_set_seek_position(eof_song->beat[b]->pos + eof_av_delay);	//Seek to that beat
	}
	else
	{
		while(b > 0)
		{	//While there's a previous beat
			b--;	//Iterate to that beat
			if(eof_song->beat[b]->flags & EOF_BEAT_FLAG_ANCHOR)
			{	//If this beat is an anchor
				eof_set_seek_position(eof_song->beat[b]->pos + eof_av_delay);	//Seek to that beat
				break;	//Break from loop
			}
		}
	}

	return 1;
}

int eof_menu_song_seek_next_anchor(void)
{
	unsigned long b;

	if(!eof_song)
		return 1;

	if(eof_music_pos - eof_av_delay < eof_song->beat[0]->pos)
	{	//If the seek position is before the first beat
		eof_set_seek_position(eof_song->beat[0]->pos + eof_av_delay);	//Seek to the first beat
	}
	else
	{
		b = eof_get_beat(eof_song, eof_music_pos - eof_av_delay);

		if(eof_beat_num_valid(eof_song, b))
		{	//If the beat containing the seek position was identified
			b++;	//Iterate to the next beat
			while(eof_beat_num_valid(eof_song, b))
			{	//While that next beat was found
				if(eof_song->beat[b]->flags & EOF_BEAT_FLAG_ANCHOR)
				{	//If this beat is an anchor
					eof_set_seek_position(eof_song->beat[b]->pos + eof_av_delay);	//Seek to that beat
					break;	//Break from loop
				}
				b++;	//Iterate to the next beat
			}
		}
	}

	return 1;
}

int eof_menu_song_seek_first_beat(void)
{
	if(!eof_song)
		return 1;

	eof_set_seek_position(eof_song->beat[0]->pos + eof_av_delay);

	return 1;
}

int eof_menu_song_seek_previous_beat(void)
{
	unsigned long b = eof_get_beat(eof_song, eof_music_pos - eof_av_delay);

	if(!eof_song)
		return 1;

	if(eof_beat_num_valid(eof_song, b) && (b > 0))
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
	unsigned long b = eof_get_beat(eof_song, eof_music_pos - eof_av_delay);

	if(!eof_song)
		return 1;

	if(eof_music_pos - eof_av_delay < eof_song->beat[0]->pos)
	{	//If the seek position is before the first beat marker
		eof_set_seek_position(eof_song->beat[0]->pos + eof_av_delay);	//Seek to the first beat's position
	}
	else if(b < eof_song->beats - 1)
	{	//If the seek position is before the last beat marker
		eof_set_seek_position(eof_song->beat[b + 1]->pos + eof_av_delay);	//Seek to the next beat's position
	}

	return 1;
}

int eof_menu_song_seek_previous_measure(void)
{
	unsigned long b = eof_get_beat(eof_song, eof_music_pos - eof_av_delay);
	unsigned num = 0, ctr;
	unsigned long originalpos = eof_music_pos - eof_av_delay;

	if(!eof_song)
		return 1;

	while(eof_beat_num_valid(eof_song, b))
	{	//For each beat at or before the current seek position
		if(eof_get_ts(eof_song, &num, NULL, b) == 1)
		{	//If this beat is a time signature
			break;	//Break from the loop
		}
		b--;	//Check previous beat on next loop
	}
	if(!eof_beat_num_valid(eof_song, b))
	{	//If no time signature was found
		num = 4;	//Assume 4/4
	}
	for(ctr = 0; ctr < num; ctr++)
	{	//Seek backward by a number of beats equal to the TS numerator
		(void) eof_menu_song_seek_previous_beat();
	}
	if(eof_input_mode == EOF_INPUT_FEEDBACK)
	{	//If Feedback input method is in effect
		if(KEY_EITHER_SHIFT)
		{	//If the user held the SHIFT key down, update the seek selection
			eof_shift_used = 1;	//Track that the SHIFT key was used
			if(eof_seek_selection_start == eof_seek_selection_end)
			{	//If this begins a seek selection
				eof_update_seek_selection(originalpos, eof_music_pos - eof_av_delay, 0);
			}
			else
			{
				eof_update_seek_selection(eof_seek_selection_start, eof_music_pos - eof_av_delay, 0);
			}
		}
	}
	eof_feedback_input_mode_update_selected_beat();	//Update the selected beat and measure if Feedback input mode is in use
	return 1;
}

int eof_menu_song_seek_next_measure(void)
{
	unsigned long b = eof_get_beat(eof_song, eof_music_pos - eof_av_delay);
	unsigned num = 0, ctr;
	unsigned long originalpos = eof_music_pos - eof_av_delay;

	if(!eof_song)
		return 1;

	while(eof_beat_num_valid(eof_song, b))
	{	//For each beat at or before the current seek position
		if(eof_get_ts(eof_song, &num, NULL, b) == 1)
		{	//If this beat is a time signature
			break;	//Break from the loop
		}
		b--;	//Check previous beat on next loop
	}
	if(!eof_beat_num_valid(eof_song, b))
	{	//If no time signature was found
		num = 4;	//Assume 4/4
	}
	for(ctr = 0; ctr < num; ctr++)
	{	//Seek backward by a number of beats equal to the TS numerator
		(void) eof_menu_song_seek_next_beat();
	}
	if(eof_input_mode == EOF_INPUT_FEEDBACK)
	{	//If Feedback input method is in effect
		if(KEY_EITHER_SHIFT)
		{	//If the user held the SHIFT key down, update the seek selection
			eof_shift_used = 1;	//Track that the SHIFT key was used
			if(eof_seek_selection_start == eof_seek_selection_end)
			{	//If this begins a seek selection
				eof_update_seek_selection(originalpos, eof_music_pos - eof_av_delay, 0);
			}
			else
			{
				eof_update_seek_selection(eof_seek_selection_start, eof_music_pos - eof_av_delay, 0);
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
	{ d_agup_list_proc,  12,  84,  400, 138, 2,   23,  0,    0,      0,   0,   (void *)eof_raw_midi_tracks_list,NULL, NULL },
	{ d_agup_push_proc,  425, 84,  68,  28,  2,   23,  'a',  D_EXIT, 0,   0,   "&Add",         NULL, (void *)eof_raw_midi_dialog_add },
//	{ d_agup_push_proc,  425, 124, 68,  28,  2,   23,  'd',  D_EXIT, 0,   0,   "&Desc",        NULL, eof_raw_midi_dialog_desc },
	{ d_agup_push_proc,  425, 164, 68,  28,  2,   23,  'l',  D_EXIT, 0,   0,   "De&lete",      NULL, (void *)eof_raw_midi_dialog_delete },
	{ d_agup_button_proc,12,  235, 240, 28,  2,   23,  '\r', D_EXIT, 0,   0,   "Done",         NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

struct eof_MIDI_data_track * eof_MIDI_track_list_to_enumerate;
char * eof_raw_midi_tracks_list(int index, int * size)
{
	struct eof_MIDI_data_track *trackptr;
	unsigned long ctr;

	if(index < 0)
	{	//Return a count of the number of tracks
		if(!size)
			return NULL;	//Invalid parameter

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
	}
	else
	{
		if(!eof_MIDI_track_list_to_enumerate)
			return eof_raw_midi_track_error;

		for(ctr = 0, trackptr = eof_MIDI_track_list_to_enumerate; (trackptr != NULL) && (ctr < (unsigned long)index); ctr++, trackptr = trackptr->next);	//Seek to the appropriate track link
		if(!trackptr)
			return eof_raw_midi_track_error;

		return trackptr->trackname;
	}
	return NULL;
}

int eof_raw_midi_dialog_delete(DIALOG * d)
{
	struct eof_MIDI_data_track *trackptr, *prevptr = NULL, *tempptr;
	unsigned long ctr;

	if(!d)
	{	//Satisfy Splint by checking value of d
		return D_O_K;
	}
	if(!eof_song->midi_data_head)
	{	//If there are no stored MIDI tracks
		return D_O_K;
	}
	if(eof_raw_midi_tracks_dialog[1].d1 < 0)
	{	//If there isn't a valid selection
		return D_O_K;
	}
	for(ctr = 0, trackptr = eof_song->midi_data_head; trackptr != NULL; ctr++, trackptr = trackptr->next)
	{	//For each stored MIDI track
		if((unsigned long)eof_raw_midi_tracks_dialog[1].d1 == ctr)
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
			(void) object_message(&eof_raw_midi_tracks_dialog[1], MSG_DRAW, 0);	//Redraw the tracks list
			return D_O_K;
		}
		prevptr = trackptr;	//Keep track of this so the next link's previous link is known
	}
	return D_O_K;
}

DIALOG eof_raw_midi_add_track_dialog[] =
{
	/* (proc)            (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)            (dp2) (dp3) */
	{ d_agup_window_proc, 0,  48,  500, 232, 2,   23,  0,    0,      0,   0,   "Select MIDI track to import",    NULL, NULL },
	{ d_agup_list_proc,  12,  84,  400, 138, 2,   23,  0,    0,      0,   0,   (void *)eof_raw_midi_tracks_list,NULL, NULL },
	{ d_agup_push_proc,  12,  235, 68,  28,  2,   23,  'i',  D_EXIT, 0,   0,   "&Import",      NULL, (void *)eof_raw_midi_track_import },
	{ d_agup_button_proc,120, 235, 68,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "Done",         NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

int eof_raw_midi_track_import(DIALOG * d)
{
	unsigned long ctr;
	struct eof_MIDI_data_track *ptr, *selected = NULL, *prev = NULL;
	char canceled = 0;
	int junk;

	if(!eof_parsed_MIDI || !eof_song || !d || (eof_raw_midi_add_track_dialog[1].d1 < 0))
		return D_O_K;

	//Find the selected track in the linked list
	for(ctr = 0, ptr = eof_parsed_MIDI; ptr != NULL; ctr++, ptr = ptr->next)
	{	//For each track in the user-selected MIDI
		if((eof_raw_midi_add_track_dialog[1].d1 > 0) && ((unsigned long)eof_raw_midi_add_track_dialog[1].d1 - 1 == ctr))
		{	//If the next track is the one to import
			prev = ptr;	//Keep track of the link prior to the one that is kept (for modifying the linked list)
		}
		else if((unsigned long)eof_raw_midi_add_track_dialog[1].d1 == ctr)
		{	//If this is the track to import
			selected = ptr;	//Keep track of the link that is to be kept
			break;
		}
	}
	if(!selected)
	{	//If the link wasn't found for some reason
		return D_O_K;	//Error
	}

	//Validate the user's selection
	if(!selected->trackname)
	{	//If this track has no name, don't allow it to be imported
		selected = NULL;
		canceled = 1;
	}
	else if((selected->timedivision) || !ustricmp(selected->trackname, "BEAT"))
	{	//If the user selected the tempo track or the beat track
		allegro_message("Import of this track is not allowed because it is written during save");
		selected = NULL;
		canceled = 1;
	}
	else if(!ustricmp(selected->trackname, "EVENTS"))
	{
		eof_clear_input();
		if(alert("Warning:  This track contains the chart's text events.", "Importing it will cause it to replace the project's events track", "in the MIDI created during save.  Continue?", "&Yes", "&No", 'y', 'n') != 1)
		{	//If user did not opt to override the project's events track with that of the external MIDI
			selected = NULL;
			canceled = 1;
		}
	}
	else
	{
		for(ctr = 0; ctr < EOF_TRACKS_MAX; ctr++)
		{	//For each of EOF's supported tracks
			if(!ustricmp(selected->trackname, eof_midi_tracks[ctr].name))
			{	//If the user selected a track with a name that EOF natively supports
				eof_clear_input();
				if(alert("Warning:  This is a track supported natively by EOF.", "Importing it will cause it to replace the project's related track", "in the MIDI created during save.  Continue?", "&Yes", "&No", 'y', 'n') != 1)
				{	//If user did not opt to override the project's native track with that of the external MIDI
					selected = NULL;
					canceled = 1;
				}
				break;
			}
		}
	}
	if(!canceled)
	{
		struct eof_MIDI_data_track *prevptr = NULL, *next = NULL;
		for(ptr = eof_song->midi_data_head; ptr != NULL; ptr = ptr->next)
		{	//For each MIDI track already being stored in the project
			next = ptr->next;
			if(!ustricmp(selected->trackname, ptr->trackname))
			{	//If the user selected a track with a name matching a track already being stored in the project
				eof_clear_input();
				if(alert("A track with this name is already being stored:", selected->trackname, "Replace it?", "&Yes", "&No", 'y', 'n') != 1)
				{	//If the user declined to replace the previously-stored track with the newly-selected track
					selected = NULL;
					canceled = 1;
				}
				else
				{	//Remove the existing track
					if(!eof_raw_midi_track_undo_made)
					{	//If an undo state hasn't already been made
						eof_prepare_undo(EOF_UNDO_TYPE_NONE);
						eof_raw_midi_track_undo_made = 1;
					}
					if(ptr->trackname)
						free(ptr->trackname);
					if(ptr->description)
						free(ptr->description);
					eof_MIDI_empty_event_list(ptr->events);
						free(ptr);
					if(prevptr)
					{	//If there was a previous pointer
						prevptr->next = next;	//It will point to the next link
					}
					else
					{	//The head link was just deleted
						if(eof_MIDI_track_list_to_enumerate == eof_song->midi_data_head)
						{	//If the calling function was enumerating this list
							eof_MIDI_track_list_to_enumerate = next;	//It needs to have the updated head pointer
						}
						eof_song->midi_data_head = next;	//The new head link is the one that follows the deleted link
					}
				}
				break;
			}
			prevptr = ptr;	//Keep track of this so the next link's previous link is known
		}
	}

//Remove the selected track from the linked list, add it to the project and redraw eof_raw_midi_add_track_dialog
	if(canceled || !selected)
		return D_O_K;	//If the user canceled or didn't select a track, return immediately

	if(!eof_raw_midi_track_undo_made)
	{	//If any undo state wasn't already made
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		eof_raw_midi_track_undo_made = 1;
	}
	if(prev)
	{	//If there was a link before the one that was imported into the project
		prev->next = selected->next;	//It points forward to what this link points forward to
	}
	else
	{	//Otherwise the next link becomes the new head of the list
		if(eof_MIDI_track_list_to_enumerate == eof_parsed_MIDI)
		{	//If the calling dialog was enumerating the list of tracks parsed in the MIDI
			eof_MIDI_track_list_to_enumerate = selected->next;	//Update the list to reflect the new head
		}
		eof_parsed_MIDI = selected->next;
	}
	selected->next = NULL;
	eof_MIDI_add_track(eof_song, selected);
	(void) dialog_message(eof_raw_midi_add_track_dialog, MSG_DRAW, 0, &junk);	//Redraw the dialog since the list's contents have changed

	return D_O_K;
}

int eof_raw_midi_dialog_add(DIALOG * d)
{
	char * returnedfn;
	char tempfilename[1024] = {0};
	MIDI * eof_work_midi = NULL;
	struct eof_MIDI_data_track *tail = NULL, *ptr, *next;
	unsigned long ctr;
	int junk;

	if(!d)
	{	//Satisfy Splint by checking value of d
		return D_O_K;
	}
	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	returnedfn = ncd_file_select(0, eof_last_eof_path, "Import raw MIDI track", eof_filter_midi_files);
	eof_clear_input();
	if(returnedfn)
	{	//If the user selected a file
		int delay;

		eof_log("\tImporting raw MIDI track", 1);

//Have user browse for a MIDI/RBA file
		if(!ustricmp(get_extension(returnedfn), "rba"))
		{	//If the selected file has a .rba extension, extract the MIDI file
			(void) replace_filename(tempfilename, returnedfn, "eof_rba_import.tmp", 1024);
			if(eof_extract_rba_midi(returnedfn,tempfilename) == 0)
			{	//If this was an RBA file and the MIDI was extracted successfully
				eof_work_midi = load_midi(tempfilename);	//Buffer extracted MIDI file
				(void) delete_file(tempfilename);	//Delete temporary file
			}
		}
		else
		{	//Buffer MIDI file
			eof_work_midi = load_midi(returnedfn);
		}
		if(!eof_work_midi)
		{
			allegro_message("Error:  Invalid MIDI file");
			(void) dialog_message(eof_raw_midi_tracks_dialog, MSG_DRAW, 0, &junk);	//Redraw the Manage raw MIDI tracks dialog
			return 0;
		}

//Parse the MIDI delay from song.ini if it exists in the same folder as the MIDI
		(void) replace_filename(tempfilename, returnedfn, "song.ini", 1024);
		set_config_file(tempfilename);
		delay = get_config_int("song", "delay", 0);
		if(delay)
		{	//If a nonzero delay was read from song.ini
			(void) snprintf(tempfilename, sizeof(tempfilename) - 1, "%d", delay);
			eof_clear_input();
			if(alert("Import this MIDI delay from song.ini?", tempfilename, NULL, "&Yes", "&No", 'y', 'n') != 1)
			{	//If the user declined using the MIDI delay from the MIDI's song.ini file
				delay = 0;	//Set the delay back to 0
			}
		}

//Build a linked list of track information
		eof_parsed_MIDI = NULL;
		for(ctr = 0; ctr < MIDI_TRACKS; ctr++)
		{
			if(!eof_work_midi->track[ctr].data)
				continue;	//If this buffered MIDI track is not populated, skip it

			ptr = eof_get_raw_MIDI_data(eof_work_midi, ctr, delay);
			if(!ptr)
			{	//If the track was not read
				allegro_message("Error parsing MIDI track");
				(void) dialog_message(eof_raw_midi_tracks_dialog, MSG_DRAW, 0, &junk);	//Redraw the Manage raw MIDI tracks dialog
				return 0;
			}
			if(!ptr->trackname)
			{	//If the track has no name
				eof_MIDI_empty_track_list(ptr);	//Delete it
			}
			else
			{	//Add the track to the list
				if(eof_parsed_MIDI == NULL)
				{	//If the list is empty
					eof_parsed_MIDI = ptr;	//The new link is now the first link in the list
				}
				else if(tail != NULL)
				{	//If there is already a link at the end of the list
					tail->next = ptr;	//Point it forward to the new link
				}
				tail = ptr;	//The new link is the new tail of the list
			}
		}

//Launch dialog to allow the user to import one or more tracks
		eof_raw_midi_track_undo_made = 0;
		eof_MIDI_track_list_to_enumerate = eof_parsed_MIDI;	//eof_raw_midi_tracks_list() will enumerate this list
		eof_clear_input();
		eof_cursor_visible = 0;
		eof_render();
		eof_color_dialog(eof_raw_midi_add_track_dialog, gui_fg_color, gui_bg_color);
		centre_dialog(eof_raw_midi_add_track_dialog);
		(void) eof_popup_dialog(eof_raw_midi_add_track_dialog, 0);
		eof_cursor_visible = 1;
		eof_pen_visible = 1;
		eof_show_mouse(NULL);

//Remove all tracks (that weren't stored) from memory
		for(ptr = eof_parsed_MIDI; ptr != NULL; ptr = next)
		{	//For each track stored from the user-selected MIDI
			next = ptr->next;
			if(ptr->trackname)
				free(ptr->trackname);
			if(ptr->description)
				free(ptr->description);
			eof_MIDI_empty_event_list(ptr->events);
			free(ptr);
		}
	}//If the user selected a file
	eof_MIDI_track_list_to_enumerate = eof_song->midi_data_head;	//This is the list that the dialog should enumerate
	(void) dialog_message(eof_raw_midi_tracks_dialog, MSG_DRAW, 0, &junk);	//Redraw the Manage raw MIDI tracks dialog
	return 0;
}

DIALOG eof_seek_beat_measure_dialog[] =
{
	/* (proc) 		        (x)	(y)	(w)	(h)	(fg) (bg) (key) (flags)	(d1)(d2)(dp)						(dp2) (dp3) */
	{ d_agup_window_proc,  	0,	48,	180,157,2,   23,  0,    0,      0,	0,	"Seek to beat/measure",	NULL, NULL },
	{ d_agup_text_proc,		16,	80,	64,	8,	2,   23,  0,    0,      0,	0,	"Seek to:",				NULL, NULL },
	{ d_agup_radio_proc,	16,	100,110,15,	2,   23,  0,    0,      0,	0,	eof_etext,				NULL, NULL },
	{ d_agup_radio_proc,	16,	120,160,15,	2,   23,  0,    0,      0,	0,	eof_etext2,				NULL, NULL },
	{ d_agup_text_proc,		16,	140,80,16,	2,   23,  0,    0,		1,	0,	"Number:",				NULL, NULL },
	{ eof_verified_edit_proc,70, 140,66,20,  2,   23,  0, D_GOTFOCUS,4,  0,  eof_etext3,           	"1234567890", NULL },
	{ d_agup_button_proc,	16,	164,68,	28,	2,   23,  '\r',	D_EXIT, 0,	0,	"OK",             		NULL, NULL },
	{ d_agup_button_proc,	96, 164,68,	28,	2,   23,  0,	D_EXIT, 0,	0,	"Cancel",         		NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

int eof_menu_song_seek_beat_measure(void)
{
	unsigned long measurecount;
	int retval, done = 0;
	static unsigned long lastselected = 3;	//By default, select the seek to measure option, which is probably going to be used more when using musical composition as a charting aid

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
	eof_etext3[0] = '\0';	//Empty the input field
	(void) snprintf(eof_etext, sizeof(eof_etext) - 1, "Beat [0 - %lu]", eof_song->beats - 1);
	if(measurecount)
	{	//If there is at least one measure
		(void) snprintf(eof_etext2, sizeof(eof_etext2) - 1, "Measure [1 - %lu]", measurecount);
		eof_seek_beat_measure_dialog[3].flags = 0;	//Enable the measure radio button
	}
	else
	{
		(void) snprintf(eof_etext2, sizeof(eof_etext2) - 1, "Measure (no TS)");
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
		retval = eof_popup_dialog(eof_seek_beat_measure_dialog, 5);
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
		else if((retval == 7) || (retval == -1) || (retval == 5))
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

int eof_menu_song_disable_click_drag(void)
{
	if(eof_song)
		eof_song->tags->click_drag_disabled ^= 1;	//Toggle this boolean variable
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
	long input_note, next_note, next_target_note = target_start, match_pos = 0;
	unsigned long match_count = 0;
	char start_found = 0, match;

	if(!sp || !hit_pos || (target_track >= sp->tracks) || (input_track >= sp->tracks))
		return 0;	//Return error

	//Find the first input note before/after the starting timestamp, depending on the search direction
	for(input_note = 0; input_note < eof_get_track_size(sp, input_track); input_note++)
	{	//For each note in the input track
		if(eof_get_note_type(sp, input_track, input_note) != input_diff)
			continue;	//If the note is not in the target difficulty, skip it

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

	//Search until the first/last note for a match, depending on the search direction
	while(start_found && (input_note < eof_get_track_size(sp, input_track)))
	{
		match = 0;
		if(!eof_note_compare(sp, input_track, input_note, target_track, next_target_note, 0))
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

	if(!sp || !hit_pos || (target_track >= sp->tracks) || (input_track >= sp->tracks))
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
	unsigned long hit_pos = 0;

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

DIALOG eof_song_catalog_edit_dialog[] =
{
	/* (proc)                (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                    (dp2) (dp3) */
	{ d_agup_window_proc,    0,   0,   200, 180, 0,   0,   0,    0,      0,   0,   "Edit catalog entry",      NULL, NULL },
	{ d_agup_text_proc,      12,  40,  60,  12,  0,   0,   0,    0,      0,   0,   "Start position (ms)",                NULL, NULL },
	{ eof_verified_edit_proc,12,  56,  50,  20,  0,   0,   0,    0,      7,   0,   eof_etext,     "0123456789", NULL },
	{ d_agup_text_proc,      12,  88,  60,  12,  0,   0,   0,    0,      0,   0,   "End position (ms)",                NULL, NULL },
	{ eof_verified_edit_proc,12,  104, 50,  20,  0,   0,   0,    0,      7,   0,   eof_etext2,     "0123456789", NULL },
	{ d_agup_button_proc,    12,  140, 84,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",               NULL, NULL },
	{ d_agup_button_proc,    110, 140, 78,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",           NULL, NULL },
	{ NULL,                  0,   0,   0,   0,   0,   0,   0,    0,      0,   0,   NULL,               NULL, NULL }
};

int eof_menu_song_catalog_edit(void)
{
	unsigned long newstart, newend;

	if(!eof_song_loaded || !eof_song || (eof_selected_catalog_entry >= eof_song->catalog->entries))
		return 1;	//Do not allow this function to run if a chart is not loaded or if an invalid catalog entry is selected

	eof_render();
	eof_color_dialog(eof_song_catalog_edit_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_song_catalog_edit_dialog);
	(void) snprintf(eof_etext, sizeof(eof_etext) - 1, "%lu", eof_song->catalog->entry[eof_selected_catalog_entry].start_pos);
	(void) snprintf(eof_etext2, sizeof(eof_etext2) - 1, "%lu", eof_song->catalog->entry[eof_selected_catalog_entry].end_pos);

	if(eof_popup_dialog(eof_song_catalog_edit_dialog, 2) == 5)
	{	//User clicked OK
		newstart = atol(eof_etext);
		newend = atol(eof_etext2);

		if(newstart >= newend)
		{	//If the given timing is not valid
			allegro_message("The entry must end after it begins");
		}
		else if((eof_song->catalog->entry[eof_selected_catalog_entry].start_pos != newstart) || (eof_song->catalog->entry[eof_selected_catalog_entry].end_pos != newend))
		{	//If the timing was changed
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			eof_song->catalog->entry[eof_selected_catalog_entry].start_pos = newstart;
			eof_song->catalog->entry[eof_selected_catalog_entry].end_pos = newend;
		}
	}

	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(NULL);
	return 1;
}

int eof_menu_song_toggle_second_piano_roll(void)
{
	eof_display_second_piano_roll ^= 1;	//Toggle this setting
	return 1;
}

int eof_menu_song_swap_piano_rolls(void)
{
	int temp_track, temp_type, temp_pos;

	if(eof_note_type2 > 255)
	{	//If the difficulty hasn't been initialized
		eof_note_type2 = eof_note_type;
	}
	if(eof_selected_track2 == 0)
	{	//If the track hasn't been initialized
		eof_selected_track2 = eof_selected_track;
	}
	if(eof_music_pos2 < 0)
	{	//If the position hasn't been initialized
		eof_music_pos2 = eof_music_pos;
	}
	eof_display_second_piano_roll = 1;	//Enable the display of the secondary piano roll
	temp_track = eof_selected_track;	//Remember the active track difficulty
	temp_type = eof_note_type;
	temp_pos = eof_music_pos;			//And remember the position
	eof_note_type = eof_note_type2;		//Update the active difficulty before updating the title bar
	(void) eof_menu_track_selected_track_number(eof_selected_track2, 1);	//Change to the track difficulty of the secondary piano roll and update title bar
	if(!eof_sync_piano_rolls)
	{	//If the secondary piano roll is tracking its own position
		eof_set_seek_position(eof_music_pos2);	//Seek to the secondary piano roll's position
	}
	eof_selected_track2 = temp_track;		//Set the secondary piano roll's track difficulty
	eof_note_type2 = temp_type;
	eof_music_pos2 = temp_pos;				//And set the secondary piano roll's position
	eof_beat_stats_cached = 0;				//Mark the cached beat stats as not current
	return 1;
}

int eof_menu_song_toggle_piano_roll_sync(void)
{
	if(eof_sync_piano_rolls)
	{	//If this setting is being turned off
		eof_music_pos2 = eof_music_pos;	//Synchronize the piano rolls
	}
	eof_sync_piano_rolls ^= 1;	//Toggle this setting
	return 1;
}

int eof_menu_song_rocksmith_export_chord_techniques(void)
{
	if(eof_song)
		eof_song->tags->rs_chord_technique_export ^= 1;	//Toggle this boolean variable
	return 1;
}

int eof_menu_song_rocksmith_suppress_dynamic_difficulty_warnings(void)
{
	if(eof_song)
		eof_song->tags->rs_export_suppress_dd_warnings ^= 1;	//Toggle this boolean variable
	return 1;
}

int eof_check_fret_hand_positions(void)
{
	char undo_made = 0;

	return eof_check_fret_hand_positions_option(0, &undo_made);	//Correct fret hand positions, don't report to user if no corrections were needed
}

int eof_check_fret_hand_positions_menu(void)
{
	char undo_made = 0;

	return eof_check_fret_hand_positions_option(1, &undo_made);	//Correct fret hand positions, report to user if no corrections were needed
}

int eof_check_fret_hand_positions_option(char report, char *undo_made)
{
	EOF_PRO_GUITAR_TRACK *tp;
	unsigned long tracknum, ctr, ctr2, ctr3, bitmask, startpos = 0, endpos;
	unsigned char position, lowest, highest;
	char width_warning = 0, problem_found = 0, started = 0, phrase_warning, unset_warning = 0;
	unsigned long ignorewarning = 0;	//Tracks which warnings the user has suppressed by answering "Ignore" to a prompt
	int ret = 0;
	unsigned limit = 21;	//Rocksmith 2's fret hand position limit is 21
	char restore_tech_view = 0;		//If tech view is in effect, it is temporarily disabled until after the secondary piano roll has been rendered

	if(!eof_song || !undo_made)
		return 0;	//Invalid parameter

	if(eof_write_rs_files || eof_write_rb_files)
	{	//If Rocksmith 1 or Rock Band export is enabled
		limit = 19;	//The highest fret hand position change is 19 instead
	}
	eof_log("eof_check_fret_hand_positions_option() entered", 1);

	for(ctr = 1; ctr < eof_song->tracks; ctr++)
	{	//For each track in the project
		if(eof_song->track[ctr]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
			continue;	//If this is not a pro guitar/bass track, skip it

		tracknum = eof_song->track[ctr]->tracknum;
		tp = eof_song->pro_guitar_track[tracknum];
		unset_warning = 0;

		restore_tech_view = eof_menu_track_get_tech_view_state(eof_song, ctr);
		eof_menu_track_set_tech_view_state(eof_song, ctr, 0);	//Disable tech view if applicable

		//Check to ensure that defined fret hand positions don't conflict with defined notes
		if(tp->handpositions)
		{	//If this track has any manually defined fret hand positions
			for(ctr2 = 0; ctr2 < tp->notes; ctr2++)
			{	//For each note in the track
				position = eof_pro_guitar_track_find_effective_fret_hand_position(tp, tp->note[ctr2]->type, tp->note[ctr2]->pos);	//Get the fret hand position in effect
				lowest = eof_pro_guitar_note_lowest_fret(tp, ctr2);		//Determine the lowest fret value in the note
				highest = eof_pro_guitar_note_highest_fret(tp, ctr2);	//And the highest fret value
				if(!position)
				{	//If no fret hand position is in effect yet
					if(!unset_warning)
					{	//If the user hasn't been warned about this issue yet
						if(report)
						{	//If the calling function wanted to prompt the user about each issue found
							unsigned char original_eof_2d_render_top_option = eof_2d_render_top_option;	//Back up the user's preference

							eof_2d_render_top_option = 7;			//Display fret hand positions at the top of the piano roll
							eof_seek_and_render_position(ctr, tp->note[ctr2]->type, tp->note[ctr2]->pos);

							if(alert("Warning:  A fret hand position was not defined at/before the first note in this track difficulty.", "You should correct this by setting the fret hand position at or before this note,", "or by deleting/regenerating the fret hand positions.  Continue?", "&Yes", "&No", 'y', 'n') != 1)
							{	//If the user does not opt to continue looking for errors
								eof_process_beat_statistics(eof_song, eof_selected_track);	//Cache section name information into the beat structures (from the perspective of the active track)
								(void) eof_detect_difficulties(eof_song, eof_selected_track);	//Update eof_track_diff_populated_status[] to reflect all populated difficulties for the active track
								eof_2d_render_top_option = original_eof_2d_render_top_option;	//Restore the user's preference
								eof_menu_track_set_tech_view_state(eof_song, ctr, restore_tech_view);	//Re-enable tech view if applicable
								return 1;	//Return user cancellation
							}
							eof_2d_render_top_option = original_eof_2d_render_top_option;	//Restore the user's preference
						}
						unset_warning = 1;
						problem_found = 1;
					}
				}
				else if(position + tp->capo > limit)
				{	//If an invalid fret hand position is in effect
					if(!report)
					{	//If the calling function wanted to just check for presence of errors
						problem_found = 1;
					}
					else if(!(ignorewarning & 1))
					{	//If the calling function wanted to prompt the user about each issue found, and this warning message hasn't been suppressed
						unsigned char original_eof_2d_render_top_option = eof_2d_render_top_option;	//Back up the user's preference
						char triggered = 0;

						eof_2d_render_top_option = 7;			//Display fret hand positions at the top of the piano roll
						eof_seek_and_render_position(ctr, tp->note[ctr2]->type, tp->note[ctr2]->pos);

						if(eof_write_rs_files)
						{	//If Rocksmith 1 files are to be exported
							ret = alert3("Warning (RS1):  Fret hand positions higher than 19 (considering any capo in use) are invalid.", "You should change this to a lower number or delete/regenerate the fret hand positions.", "Continue?", "&Yes", "&No", "&Ignore", 'y', 'n', 'i');
							problem_found = 1;
							triggered = 1;
						}
						else if(eof_write_rs2_files)
						{	//If Rocksmith 2 files are to be exported
							if(position + tp->capo > 21)
							{	//If the effective position is above fret 21
								ret = alert3("Warning (RS2):  Fret hand positions higher than 21 (considering any capo in use) are invalid.", "You should change this to a lower number or delete/regenerate the fret hand positions.", "Continue?", "&Yes", "&No", "&Ignore", 'y', 'n', 'i');
								problem_found = 1;
								triggered = 1;
							}
						}
						if(triggered)
						{	//If the prompt was triggered
							if(ret == 2)
							{	//If the user does not opt to continue looking for errors
								eof_process_beat_statistics(eof_song, eof_selected_track);		//Cache section name information into the beat structures (from the perspective of the active track)
								(void) eof_detect_difficulties(eof_song, eof_selected_track);	//Update eof_track_diff_populated_status[] to reflect all populated difficulties for the active track
								eof_2d_render_top_option = original_eof_2d_render_top_option;	//Restore the user's preference
								eof_menu_track_set_tech_view_state(eof_song, ctr, restore_tech_view);	//Re-enable tech view if applicable
								return 1;	//Return user cancellation
							}
							if(ret == 3)
							{	//If the user chose to ignore instances of this issue
								ignorewarning |= 1;
							}
							eof_2d_render_top_option = original_eof_2d_render_top_option;	//Restore the user's preference
						}
					}
				}
				else
				{	//A valid fret hand position is in effect, run other checks
					if((lowest != 0) && ((eof_write_rs_files && (highest > position + 3)) || (lowest < position)))
					{	//If the note is not open, and its fret value goes outside of the highlighted fret range based on the current fret hand position in place (too far above current FHP is only an error in Rocksmith 1)
						unsigned char original_eof_2d_render_top_option = eof_2d_render_top_option;	//Back up the user's preference

						if(report)
						{	//If the calling function wanted to prompt the user about each issue found
							eof_2d_render_top_option = 7;			//Display fret hand positions at the top of the piano roll
							eof_selection.current = ctr2;			//Select the note in question
							eof_selection.track = ctr;
							eof_selection.multi[ctr2] = 1;
							eof_seek_and_render_position(ctr, tp->note[ctr2]->type, tp->note[ctr2]->pos);
						}
						if(highest - lowest + 1 > 4)
						{	//If this note is a chord, and its fret range is too large to fit into Rocksmith's chord box
							if(!width_warning && report)
							{	//If the user wasn't warned about this yet, and this validation function was manually invoked through the menu
								if(alert("Warning (RS1):  At least one chord is too wide to fit into a chord box.", "It may be a good idea to change its fretting.", "Continue?", "&Yes", "&No", 'y', 'n') != 1)
								{	//If the user does not opt to continue looking for errors
									eof_process_beat_statistics(eof_song, eof_selected_track);	//Cache section name information into the beat structures (from the perspective of the active track)
									(void) eof_detect_difficulties(eof_song, eof_selected_track);	//Update eof_track_diff_populated_status[] to reflect all populated difficulties for the active track
									eof_2d_render_top_option = original_eof_2d_render_top_option;	//Restore the user's preference
									eof_menu_track_set_tech_view_state(eof_song, ctr, restore_tech_view);	//Re-enable tech view if applicable
									return 1;	//Return user cancellation
								}
								eof_2d_render_top_option = original_eof_2d_render_top_option;	//Restore the user's preference
								problem_found = 1;
							}
							width_warning = 1;
						}
						if(report)
						{	//If the calling function wanted to prompt the user about each issue found
							char *warning = NULL;
							char warning1[] = "Warning:  This note is lower than the highlighted range of the fretboard.";
							char warning2[] = "Warning (RS1):  This note is higher than the highlighted range of the fretboard.";

							if(lowest < position)
							{	//If there is a fretted note lower than the fret hand position
								if(!(ignorewarning & 2))
								{	//If this warning message hasn't been suppressed
									warning = warning1;	//Select the appropriate warning message
								}
							}
							else if(eof_write_rs_files)
							{	//There is a fretted note more than 3 frets higher than the fret hand position and Rocksmith 1 files are to be exported
								if(!(ignorewarning & 4))
								{	//If this warning message hasn't been suppressed
									warning = warning2;	//Select the appropriate warning message
								}
							}
							if(warning)
							{	//If a warning is being displayed instead of suppressed
								ret = alert3(warning, "You should correct this by setting the fret hand position at or before this note,", "or by deleting/regenerating the fret hand positions.  Continue?", "&Yes", "&No", "&Ignore", 'y', 'n', 'i');
								if(ret == 2)
								{	//If the user does not opt to continue looking for errors
									eof_process_beat_statistics(eof_song, eof_selected_track);	//Cache section name information into the beat structures (from the perspective of the active track)
									(void) eof_detect_difficulties(eof_song, eof_selected_track);	//Update eof_track_diff_populated_status[] to reflect all populated difficulties for the active track
									eof_menu_track_set_tech_view_state(eof_song, ctr, restore_tech_view);	//Re-enable tech view if applicable
									return 1;	//Return user cancellation
								}
								if(ret == 3)
								{	//If the user chose to ignore instances of this issue
									if(warning == warning1)
									{
										ignorewarning |= 2;
									}
									else
									{
										ignorewarning |= 4;
									}
								}
							}
						}//If the calling function wanted to prompt the user about each issue found
						problem_found = 1;
					}//If the note is not open, and its fret value goes outside of the highlighted fret range based on the current fret hand position in place
					else
					{	//The note stays within the highlighted fret range
						for(ctr3 = 0, bitmask = 1; ctr3 < 6; ctr3++, bitmask <<= 1)
						{	//For each of the 6 usable strings
							if(!(tp->note[ctr2]->note & bitmask) || (tp->note[ctr2]->finger[ctr3] != 1) || ((tp->note[ctr2]->frets[ctr3] & 0x7F) == position))
								continue;	//If this string isn't defined as being fretted by the index finger, or the fret hand position in effect is already at that note's fret, skip it

							if(report && !(ignorewarning & 8))
							{	//If the calling function wanted to prompt the user about each issue found, and this warning message hasn't been suppressed
								unsigned char original_eof_2d_render_top_option = eof_2d_render_top_option;	//Back up the user's preference

								eof_2d_render_top_option = 7;			//Display fret hand positions at the top of the piano roll
								eof_selection.current = ctr2;			//Select the note in question
								eof_selection.track = ctr;
								eof_selection.multi[ctr2] = 1;
								eof_seek_and_render_position(ctr, tp->note[ctr2]->type, tp->note[ctr2]->pos);
								ret = alert3("Warning:  This note is specified as using the index finger,", "but the fret hand position is on a different fret.", "It's recommended to change the fret hand position to match.  Continue?", "&Yes", "&No", "&Ignore", 'y', 'n', 'i');
								if(ret == 2)
								{	//If the user does not opt to continue looking for errors
									eof_process_beat_statistics(eof_song, eof_selected_track);	//Cache section name information into the beat structures (from the perspective of the active track)
									(void) eof_detect_difficulties(eof_song, eof_selected_track);	//Update eof_track_diff_populated_status[] to reflect all populated difficulties for the active track
									eof_2d_render_top_option = original_eof_2d_render_top_option;	//Restore the user's preference
									eof_menu_track_set_tech_view_state(eof_song, ctr, restore_tech_view);	//Re-enable tech view if applicable
									return 1;	//Return user cancellation
								}
								if(ret == 3)
								{	//If the user chose to ignore instances of this issue
									ignorewarning |= 8;
								}
								eof_2d_render_top_option = original_eof_2d_render_top_option;	//Restore the user's preference
							}
							problem_found = 1;
							break;
						}
					}
				}//A fret hand position is in effect
				eof_selection.multi[ctr2] = 0;	//Deselect the note
			}//For each note in the track

			//Check to ensure that each difficulty of an RS phrase defines a fret hand position
			phrase_warning = 0;
			eof_process_beat_statistics(eof_song, ctr);	//Cache section name information into the beat structures (from the perspective of the specified track)
			(void) eof_detect_difficulties(eof_song, ctr);	//Update eof_track_diff_populated_status[] to reflect all populated difficulties for this track
			for(ctr2 = 0; ctr2 < eof_song->beats; ctr2++)
			{	//For each beat in the project
				if((eof_song->beat[ctr2]->contained_section_event >= 0) || ((ctr2 + 1 >= eof_song->beats) && started))
				{	//If this beat has a section event (RS phrase) or a phrase is in progress and this is the last beat, it marks the end of any current phrase and the potential start of another
					if(started)
					{	//If the first phrase marker has been encountered, this beat marks the end of a phrase
						endpos = eof_song->beat[ctr2]->pos - 1;	//Track this as the end position of the phrase
						for(ctr3 = 0; ctr3 < 256; ctr3++)
						{	//For each of the 255 possible difficulties
							if(!eof_track_diff_populated_status[ctr3])
								continue;	//If this difficulty is not populated, skip it
							if(!eof_time_range_is_populated(eof_song, ctr, startpos, endpos, ctr3))
								continue;	//If there isn't at least one note in this RS phrase in this difficulty, skip it

							if(!phrase_warning && !eof_enforce_rs_phrase_begin_with_fret_hand_position(eof_song, ctr, ctr3, startpos, endpos, undo_made, 1))
							{	//If the user hasn't been warned about this issue yet for this track, check to see if a fret hand position was NOT defined at or before the first note in the phrase
								problem_found = 1;
								if(report)
								{	//If the calling function wanted to prompt the user about each issue found
									unsigned char original_eof_2d_render_top_option = eof_2d_render_top_option;	//Back up the user's preference

									eof_2d_render_top_option = 7;			//Display fret hand positions at the top of the piano roll
									eof_selected_beat = ctr2;				//Change the selected beat
									eof_seek_and_render_position(ctr, ctr3, startpos);
									eof_clear_input();
									phrase_warning = alert3("The fret hand position is not redefined at each difficulty of each phrase.", "This can prevent the positions from working in dynamic difficulty charts.", "Correct this issue?", "&Yes", "&No", "Cancel", 'y', 'n', 0);
									if(phrase_warning == 3)
									{	//If the user does not opt to continue looking for errors
										eof_process_beat_statistics(eof_song, eof_selected_track);	//Cache section name information into the beat structures (from the perspective of the active track)
										(void) eof_detect_difficulties(eof_song, eof_selected_track);	//Update eof_track_diff_populated_status[] to reflect all populated difficulties for the active track
										eof_2d_render_top_option = original_eof_2d_render_top_option;	//Restore the user's preference
										eof_menu_track_set_tech_view_state(eof_song, ctr, restore_tech_view);	//Re-enable tech view if applicable
										return 1;	//Return user cancellation
									}
									eof_2d_render_top_option = original_eof_2d_render_top_option;	//Restore the user's preference
									if(phrase_warning == 2)
									{	//If the user declined to fix this issue
										break;
									}
								}
							}
							if(report && (phrase_warning == 1))
							{	//If the user opted to correct this issue, ensure each difficulty of each phrase defines a fret hand position
								(void) eof_enforce_rs_phrase_begin_with_fret_hand_position(eof_song, ctr, ctr3, startpos, endpos, undo_made, 0);	//Add a fret hand position to this phrase if one is not defined already
							}
						}//For each of the 255 possible difficulties
					}//If the first phrase marker has been encountered, this beat marks the end of a phrase
					if(phrase_warning == 2)
					{	//If the user declined to fix this issue
						break;
					}

					started = 1;	//Track that a phrase has been encountered
					startpos = eof_song->beat[ctr2]->pos;	//Track the starting position of the phrase
				}//If this beat has a section event (RS phrase) or a phrase is in progress and this is the last beat, it marks the end of any current phrase and the potential start of another
			}//For each beat in the project
		}//If this track has any manually defined fret hand positions

		eof_menu_track_set_tech_view_state(eof_song, ctr, restore_tech_view);	//Re-enable tech view if applicable
	}//For each track in the project

	if(report && !problem_found)
	{	//If the calling function wanted to report if no errors were found, and none were
		allegro_message("No fret hand position errors were found");
	}

	eof_process_beat_statistics(eof_song, eof_selected_track);	//Cache section name information into the beat structures (from the perspective of the active track)
	(void) eof_detect_difficulties(eof_song, eof_selected_track);	//Update eof_track_diff_populated_status[] to reflect all populated difficulties for the active track
	if(!report)
	{	//If the calling function was only checking for errors
		return problem_found;
	}

	return 0;	//Return completion
}

DIALOG eof_menu_song_time_range_dialog[] =
{
	/* (proc)                (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                   (dp2) (dp3) */
	{ d_agup_window_proc,    0,   0,   200, 190, 0,   0,   0,    0,      0,   0,   eof_etext,             NULL, NULL },
	{ d_agup_text_proc,      12,  56,  60,  12,  0,   0,   0,    0,      0,   0,   "Start position (ms)", NULL, NULL },
	{ eof_verified_edit_proc,12,  72,  50,  20,  0,   0,   0,    0,      7,   0,   eof_etext2,     "0123456789", NULL },
	{ d_agup_text_proc,      12,  100, 60,  12,  0,   0,   0,    0,      0,   0,   "Stop position (ms)",  NULL, NULL },
	{ eof_verified_edit_proc,12,  120, 50,  20,  0,   0,   0,    0,      7,   0,   eof_etext3,     "0123456789", NULL },
	{ d_agup_button_proc,    12,  150, 84,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",                  NULL, NULL },
	{ d_agup_button_proc,    110, 150, 78,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",              NULL, NULL },
	{ NULL,                  0,   0,   0,   0,   0,   0,   0,    0,      0,   0,   NULL,                  NULL, NULL }
};

int eof_run_time_range_dialog(unsigned long *start, unsigned long *end)
{
	unsigned long newstart, newend;

	if(!start || !end)
		return 0;	//Invalid parameters

	(void) snprintf(eof_etext2, sizeof(eof_etext2) - 1, "%lu", *start);	//Initialize the start time string
	(void) snprintf(eof_etext3, sizeof(eof_etext3) - 1, "%lu", *end);	//Initialize the end time string

	eof_render();
	eof_color_dialog(eof_menu_song_time_range_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_menu_song_time_range_dialog);
	if(eof_popup_dialog(eof_menu_song_time_range_dialog, 2) == 5)
	{	//User clicked OK
		if(!eof_check_string(eof_etext2) || !eof_check_string(eof_etext3))
		{	//If either the start or stop fields had no non-space characters
			return 0;	//Invalid input
		}
		newstart = atol(eof_etext2);
		newend = atol(eof_etext3);
		if(newstart == newend)
			return 0;	//This isn't a valid preview time range

		*start = newstart;
		*end = newend;
		return 1;
	}

	eof_render();
	return 0;	//User cancelation
}

int eof_menu_song_export_song_preview(void)
{
	unsigned long start = 0, stop = 0;
	char targetpath[1024] = {0};
	char syscommand[1024] = {0};
	char wavname[270] = {0};
	unsigned long oldstarttag = 0, oldendtag = 0;
	char *oldstartstring, *oldendstring;

	eof_log("eof_menu_song_export_song_preview() entered", 1);
	eof_log("\tCreating preview audio", 1);

	if(!eof_song)
		return 0;

	//Determine if the preview start and end timestamps are already stored in the project
	oldstartstring = eof_find_ini_setting_tag(eof_song, &oldstarttag, "preview_start_time");
	oldendstring = eof_find_ini_setting_tag(eof_song, &oldendtag, "preview_end_time");
	if(oldstartstring && oldendstring)
	{	//If both timestamp tags were found in the INI settings
		char *temp;
		for(temp = oldstartstring; *temp == ' '; temp++);	//Skip past any leading whitespace in the value portion of the INI setting
		start = strtoul(temp, NULL, 10);
		for(temp = oldendstring; *temp == ' '; temp++);	//Skip past any leading whitespace in the value portion of the INI setting
		stop = strtoul(temp, NULL, 10);
	}

	//Initialize the start and end positions as appropriate
	if(!(oldstartstring && oldendstring && (start != stop)))
	{	//If the positions were NOT read from the INI settings
		if(eof_seek_selection_start != eof_seek_selection_end)
		{	//If there is a seek selection
			start = eof_seek_selection_start;
			stop = eof_seek_selection_end;
		}
		else if((eof_song->tags->start_point != ULONG_MAX) && (eof_song->tags->end_point != ULONG_MAX) && (eof_song->tags->start_point != eof_song->tags->end_point))
		{	//If both the start and end points are defined with different timestamps
			start = eof_song->tags->start_point;
			stop = eof_song->tags->end_point;
		}
		else
		{	//Default the start time to the current seek position and the stop time 30 seconds later
			start = eof_music_pos - eof_av_delay;
			stop = start + 30000; //30,000 ms later
		}
	}

	strncpy(eof_etext, "Set preview timings", sizeof(eof_etext) - 1);	//Set the title of the eof_menu_song_time_range_dialog dialog
	if(!eof_run_time_range_dialog(&start, &stop))	//If a valid time range isn't provided by the user
		return 0;									//Cancel

	if(!oldstartstring || !oldendstring || (start != strtoul(oldstartstring, NULL, 10)) || (stop != strtoul(oldendstring, NULL, 10)))
	{	//If the project didn't already have the start/stop preview tags stored, or if the times just entered are different from those already stored
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		if(oldstartstring)
			eof_ini_delete(oldstarttag);	//Remove any existing preview start tag
		oldendstring = eof_find_ini_setting_tag(eof_song, &oldendtag, "preview_end_time");	//Re-lookup the index for the end tag, since the index may have just changed due to the above deletion
		if(oldendstring)
			eof_ini_delete(oldendtag);	//Remove any existing preview start tag
		if(eof_song->tags->ini_settings + 2 < EOF_MAX_INI_SETTINGS)
		{	//If the start and end INI tags can be stored into the project
			(void) snprintf(eof_song->tags->ini_setting[eof_song->tags->ini_settings], EOF_INI_LENGTH - 1, "preview_start_time = %lu", start);
			eof_song->tags->ini_settings++;
			(void) snprintf(eof_song->tags->ini_setting[eof_song->tags->ini_settings], EOF_INI_LENGTH - 1, "preview_end_time = %lu", stop);
			eof_song->tags->ini_settings++;
		}
	}

	if(eof_silence_loaded)
	{	//If no chart audio is loaded
		return 1;
	}
	if(alert(NULL, "Generate preview audio files?", NULL, "&Yes", "&No", 'y', 'n') != 1)
	{	//If the user declined to generate audio files
		return 1;
	}

	if(eof_ogg_settings())
	{	//If the user selected an OGG encoding quality
		//Determine the name to save the WAV file to
		if(eof_song->tags->title[0] != '\0')
		{	//If the chart has a defined song title
			(void) snprintf(wavname, sizeof(wavname), "%s_preview.wav", eof_song->tags->title);
		}
		else
		{	//Otherwise default to "guitar"
			(void) snprintf(wavname, sizeof(wavname), "guitar_preview.wav");
		}
		(void) replace_filename(targetpath, eof_song_path, wavname, 1024);	//Build the target path for the preview WAV file
		(void) delete_file(targetpath);	//Delete the preview WAV file if it already exists
		eof_export_audio_time_range(eof_music_track, start / 1000.0, stop / 1000.0, targetpath);	//Build the preview WAV file
		if(!exists(targetpath))
		{	//If the preview WAV file was not created, retry using a known acceptable file name
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError writing file \"%s\":  \"%s\", retrying with a target name of guitar_preview.wav", targetpath, strerror(errno));	//Get the Operating System's reason for the failure
			eof_log(eof_log_string, 1);
			(void) snprintf(wavname, sizeof(wavname), "guitar_preview.wav");
			(void) replace_filename(targetpath, eof_song_path, wavname, 1024);	//Build the target path for the preview WAV file
			(void) delete_file(targetpath);	//Delete the preview WAV file if it already exists
			eof_export_audio_time_range(eof_music_track, start / 1000.0, stop / 1000.0, targetpath);	//Build the preview WAV file
		}
		if(exists(targetpath))
		{	//If the preview WAV file was created, convert it to OGG
			(void) replace_filename(targetpath, eof_song_path, "preview.ogg", 1024);	//Build the target for the preview OGG file
			(void) delete_file(targetpath);	//Delete the preview OGG file if it already exists
			(void) replace_filename(targetpath, eof_song_path, "", 1024);	//Build the path for the preview files' parent folder
			#ifdef ALLEGRO_WINDOWS
				(void) uszprintf(syscommand, (int) sizeof(syscommand), "oggenc2 --quiet -q %s --resample 44100 -s 0 \"%s%s\" -o \"%spreview.ogg\"", eof_ogg_quality[(int)eof_ogg_setting], targetpath, wavname, targetpath);
			#else
				(void) uszprintf(syscommand, (int) sizeof(syscommand), "oggenc --quiet -q %s --resample 44100 -s 0 \"%s%s\" -o \"%spreview.ogg\"", eof_ogg_quality[(int)eof_ogg_setting], targetpath, wavname, targetpath);
			#endif
			(void) eof_system(syscommand);
			if(!exists(targetpath))
			{
				eof_log("\tError creating preview OGG file.", 1);
			}
		}
		else
		{
			eof_log("\tError creating preview WAV file, aborting.", 1);
		}
	}
	else
	{
		eof_log("\tUser cancellation.", 1);
	}

	return 1;
}

int eof_menu_song_highlight_non_grid_snapped_notes(void)
{
	unsigned long ctr;

	if(!eof_song || (eof_selected_track >= eof_song->tracks))
		return 1;	//Invalid parameters

	eof_song->tags->highlight_unsnapped_notes ^= 1;	//Toggle this setting
	for(ctr = 1; ctr < eof_song->tracks; ctr++)
	{	//For each track
		eof_track_remove_highlighting(eof_song, ctr, 1);	//Remove existing temporary highlighting from the track
		if(eof_song->tags->highlight_unsnapped_notes)
			eof_song_highlight_non_grid_snapped_notes(eof_song, ctr);	//Re-create the non grid snapped highlighting as appropriate
		if(eof_song->tags->highlight_arpeggios)
			eof_song_highlight_arpeggios(eof_song, ctr);		//Re-create the arpeggio highlighting as appropriate
	}
	(void) eof_detect_difficulties(eof_song, eof_selected_track);	//Update arrays for note set population and highlighting to reflect the active track
	return 1;
}

void eof_song_highlight_non_grid_snapped_notes(EOF_SONG *sp, unsigned long track)
{
	unsigned long ctr, ctr2, loopcount = 1, tflags, thispos, lastpos = 0;
	int thisisgridsnapped, lastisgridsnapped = 0;

	if(!sp || (track >= sp->tracks) || !track)
		return;	//Invalid parameters

	if(sp->track[track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
	{	//If this is a pro guitar track
		loopcount = 2;	//The main loop will run a second time to process the inactive note set
	}

	for(ctr2 = 0; ctr2 < loopcount; ctr2++)
	{	//For each note set in the specified track
		for(ctr = 0; ctr < eof_get_track_size(sp, track); ctr++)
		{	//For each note in the specified track
			thispos = eof_get_note_pos(sp, track, ctr);
			if(ctr && (thispos == lastpos))
			{	//If this isn't the first note, but it is at the same timestamp as the last examined note
				thisisgridsnapped = lastisgridsnapped;	//Skip the number crunching and automatically assume the same highlighting status
			}
			else
			{	//Otherwise do it the hard way
				thisisgridsnapped = eof_is_any_grid_snap_position(thispos, NULL, NULL, NULL);
			}
			if(!thisisgridsnapped)
			{	//If this note position does not match that of any grid snap
				tflags = eof_get_note_tflags(sp, track, ctr);
				tflags |= EOF_NOTE_TFLAG_HIGHLIGHT;	//Highlight the note with the temporary flag
				eof_set_note_tflags(sp, track, ctr, tflags);
			}
			lastpos = thispos;						//Track the timestamp of the last examined note
			lastisgridsnapped = thisisgridsnapped;	//And whether it was grid snapped
		}
		eof_menu_track_toggle_tech_view_state(sp, track);	//Toggle to the other note set as applicable
	}
}

int eof_menu_song_highlight_arpeggios(void)
{
	unsigned long ctr;

	if(!eof_song || (eof_selected_track >= eof_song->tracks))
		return 1;	//Invalid parameters

	eof_song->tags->highlight_arpeggios ^= 1;	//Toggle this setting
	for(ctr = 1; ctr < eof_song->tracks; ctr++)
	{	//For each track
		eof_track_remove_highlighting(eof_song, ctr, 1);	//Remove existing temporary highlighting from the track
		if(eof_song->tags->highlight_unsnapped_notes)
			eof_song_highlight_non_grid_snapped_notes(eof_song, ctr);	//Re-create the non grid snapped highlighting as appropriate
		if(eof_song->tags->highlight_arpeggios)
			eof_song_highlight_arpeggios(eof_song, ctr);		//Re-create the arpeggio highlighting as appropriate
	}
	return 1;
}

void eof_song_highlight_arpeggios(EOF_SONG *sp, unsigned long track)
{
	unsigned long ctr, ctr2, ctr3, tracknum, loopcount = 2;
	EOF_PRO_GUITAR_TRACK *tp;

	if(!sp || (track >= sp->tracks) || !track)
		return;	//Invalid parameters
	if(sp->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return;	//Do not allow this function to run on a non pro guitar track

	tracknum = sp->track[track]->tracknum;
	tp = sp->pro_guitar_track[tracknum];
	if(!tp->arpeggios)
		return;	//Do not proceed further if there aren't even any arpeggios
	for(ctr2 = 0; ctr2 < loopcount; ctr2++)
	{	//For each note set in the specified track
		for(ctr = 0; ctr < tp->notes; ctr++)
		{	//For each note in the active pro guitar track
			for(ctr3 = 0; ctr3 < tp->arpeggios; ctr3++)
			{	//For each arpeggio section in the track
				if((tp->note[ctr]->pos >= tp->arpeggio[ctr3].start_pos) && (tp->note[ctr]->pos <= tp->arpeggio[ctr3].end_pos) && (tp->note[ctr]->type == tp->arpeggio[ctr3].difficulty))
				{	//If the note is within the arpeggio phrase
					tp->note[ctr]->tflags |= EOF_NOTE_TFLAG_HIGHLIGHT;	//Highlight it with the temporary flag
				}
			}
		}
		eof_menu_track_toggle_tech_view_state(sp, track);	//Toggle to the other note set as applicable
	}
}
