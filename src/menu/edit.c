#include <allegro.h>
#include <rubberband/rubberband-c.h>	//For rubberband benchmarking
#include "../alogg/include/alogg.h"		//For rubberband benchmarking
#include "vorbis/vorbisfile.h"			//For rubberband benchmarking
#include "../agup/agup.h"
#include "../undo.h"
#include "../dialog.h"
#include "../mix.h"
#include "../main.h"	//Inclusion for eof_custom_snap_measure
#include "../song.h"
#include "../dialog/proc.h"
#include "../utility.h"
#include "edit.h"
#include "note.h"	//For eof_feedback_mode_update_note_selection()
#include "song.h"
#include "track.h"	//For tech view functions

#ifdef USEMEMWATCH
#include "../memwatch.h"
#endif

static float *eof_benchmark_rubberband_buffer[2];
unsigned long eof_benchmark_rubberband_sample_count = 0;
RubberBandState eof_benchmark_rubberband_state;
int eof_benchmark_rubberband_cancel = 0;
#define EOF_BENCHMARK_RUBBERBAND_BUFFSIZE 2048

MENU eof_edit_paste_from_menu[] =
{
	{"&Supaeasy", eof_menu_edit_paste_from_supaeasy, NULL, 0, NULL},
	{"&Easy", eof_menu_edit_paste_from_easy, NULL, 0, NULL},
	{"&Medium", eof_menu_edit_paste_from_medium, NULL, 0, NULL},
	{"&Amazing", eof_menu_edit_paste_from_amazing, NULL, 0, NULL},
	{"", NULL, NULL, 0, NULL},
	{"&Catalog\tShift+C", eof_menu_edit_paste_from_catalog, NULL, 0, NULL},
	{"&Difficulty", eof_menu_song_paste_from_difficulty, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_edit_paste_from_menu_dance[] =
{
	{"&Beginner", eof_menu_edit_paste_from_supaeasy, NULL, 0, NULL},
	{"&Easy", eof_menu_edit_paste_from_easy, NULL, 0, NULL},
	{"&Medium", eof_menu_edit_paste_from_medium, NULL, 0, NULL},
	{"H&ard", eof_menu_edit_paste_from_amazing, NULL, 0, NULL},
	{"C&hallenge", eof_menu_edit_paste_from_challenge, NULL, 0, NULL},
	{"&Catalog\t" CTRL_NAME "+Shift+C", eof_menu_edit_paste_from_catalog, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU * eof_active_edit_paste_from_menu;

MENU eof_edit_snap_menu[] =
{
	{"1/4", eof_menu_edit_snap_quarter, NULL, 0, NULL},
	{"1/8", eof_menu_edit_snap_eighth, NULL, 0, NULL},
	{"1/16", eof_menu_edit_snap_sixteenth, NULL, 0, NULL},
	{"1/32", eof_menu_edit_snap_thirty_second, NULL, 0, NULL},
	{"1/64", eof_menu_edit_snap_sixty_forth, NULL, 0, NULL},
	{"", NULL, NULL, 0, NULL},
	{"1/12", eof_menu_edit_snap_twelfth, NULL, 0, NULL},
	{"1/24", eof_menu_edit_snap_twenty_fourth, NULL, 0, NULL},
	{"1/48", eof_menu_edit_snap_forty_eighth, NULL, 0, NULL},
	{"1/96", eof_menu_edit_snap_ninty_sixth, NULL, 0, NULL},
	{"", NULL, NULL, 0, NULL},
	{"&Custom\tG", eof_menu_edit_snap_custom, NULL, 0, NULL},
	{"", NULL, NULL, 0, NULL},
	{"&Off", eof_menu_edit_snap_off, NULL, D_SELECTED, NULL},
	{"", NULL, NULL, 0, NULL},
	{"&Display grid lines\tShift+G", eof_menu_edit_toggle_grid_lines, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_edit_claps_menu[] =
{
	{"&All", eof_menu_edit_claps_all, NULL, D_SELECTED, NULL},
	{"&Green", eof_menu_edit_claps_green, NULL, 0, NULL},
	{"&Red", eof_menu_edit_claps_red, NULL, 0, NULL},
	{"&Yellow", eof_menu_edit_claps_yellow, NULL, 0, NULL},
	{"&Blue", eof_menu_edit_claps_blue, NULL, 0, NULL},
	{"&Purple", eof_menu_edit_claps_purple, NULL, 0, NULL},
	{"&Orange", eof_menu_edit_claps_orange, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_edit_hopo_menu[] =
{
	{"&RF", eof_menu_edit_hopo_rf, NULL, D_SELECTED, NULL},
	{"&FOF", eof_menu_edit_hopo_fof, NULL, 0, NULL},
	{"&Off", eof_menu_edit_hopo_off, NULL, 0, NULL},
	{"&Manual", eof_menu_edit_hopo_manual, NULL, 0, NULL},
	{"&GH3", eof_menu_edit_hopo_gh3, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

char eof_edit_zoom_menu_string[20] = "&Custom";
MENU eof_edit_zoom_menu[] =
{
	{"1/1&0", eof_menu_edit_zoom_10, NULL, D_SELECTED, NULL},
	{"1/&9", eof_menu_edit_zoom_9, NULL, 0, NULL},
	{"1/&8", eof_menu_edit_zoom_8, NULL, 0, NULL},
	{"1/&7", eof_menu_edit_zoom_7, NULL, 0, NULL},
	{"1/&6", eof_menu_edit_zoom_6, NULL, 0, NULL},
	{"1/&5", eof_menu_edit_zoom_5, NULL, 0, NULL},
	{"1/&4", eof_menu_edit_zoom_4, NULL, 0, NULL},
	{"1/&3", eof_menu_edit_zoom_3, NULL, 0, NULL},
	{"1/&2", eof_menu_edit_zoom_2, NULL, 0, NULL},
	{"1/&1", eof_menu_edit_zoom_1, NULL, 0, NULL},
	{eof_edit_zoom_menu_string, eof_menu_edit_zoom_custom, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_edit_playback_menu[] =
{
	{"&Time Stretch", eof_menu_edit_playback_time_stretch, NULL, D_SELECTED, NULL},
	{"&100%", eof_menu_edit_playback_100, NULL, D_SELECTED, NULL},
	{"&75%", eof_menu_edit_playback_75, NULL, 0, NULL},
	{"&50%", eof_menu_edit_playback_50, NULL, 0, NULL},
	{"&25%", eof_menu_edit_playback_25, NULL, 0, NULL},
	{"&Custom", eof_menu_edit_playback_custom, NULL, 0, NULL},
	{"Benchmark rubberband", eof_menu_edit_benchmark_rubberband, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_edit_speed_menu[] =
{
	{"&Slow", eof_menu_edit_speed_slow, NULL, D_SELECTED, NULL},
	{"&Medium", eof_menu_edit_speed_medium, NULL, 0, NULL},
	{"&Fast", eof_menu_edit_speed_fast, NULL, 0, NULL},
	{"&Hyper", eof_menu_edit_speed_hyper, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_edit_bookmark_menu[] =
{
	{"&0\t" CTRL_NAME "+Num 0", eof_menu_edit_bookmark_0, NULL, 0, NULL},
	{"&1\t" CTRL_NAME "+Num 1", eof_menu_edit_bookmark_1, NULL, 0, NULL},
	{"&2\t" CTRL_NAME "+Num 2", eof_menu_edit_bookmark_2, NULL, 0, NULL},
	{"&3\t" CTRL_NAME "+Num 3", eof_menu_edit_bookmark_3, NULL, 0, NULL},
	{"&4\t" CTRL_NAME "+Num 4", eof_menu_edit_bookmark_4, NULL, 0, NULL},
	{"&5\t" CTRL_NAME "+Num 5", eof_menu_edit_bookmark_5, NULL, 0, NULL},
	{"&6\t" CTRL_NAME "+Num 6", eof_menu_edit_bookmark_6, NULL, 0, NULL},
	{"&7\t" CTRL_NAME "+Num 7", eof_menu_edit_bookmark_7, NULL, 0, NULL},
	{"&8\t" CTRL_NAME "+Num 8", eof_menu_edit_bookmark_8, NULL, 0, NULL},
	{"&9\t" CTRL_NAME "+Num 9", eof_menu_edit_bookmark_9, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_edit_selection_select_menu[] =
{
	{"&Chords", eof_menu_edit_select_chords, NULL, 0, NULL},
	{"Single &Notes", eof_menu_edit_select_single_notes, NULL, 0, NULL},
	{"&Toms", eof_menu_edit_select_toms, NULL, 0, NULL},
	{"c&Ymbals", eof_menu_edit_select_cymbals, NULL, 0, NULL},
	{"&Grid snapped", eof_menu_edit_select_grid_snapped_notes, NULL, 0, NULL},
	{"Non g&Rid snapped", eof_menu_edit_select_non_grid_snapped_notes, NULL, 0, NULL},
	{"&Shorter than", eof_menu_edit_select_all_shorter_than, NULL, 0, NULL},
	{"&Longer than", eof_menu_edit_select_all_longer_than, NULL, 0, NULL},
	{"&Of length", eof_menu_edit_select_all_of_length, NULL, 0, NULL},
	{"On &Beat notes", eof_menu_edit_select_on_beat_notes, NULL, 0, NULL},
	{"O&Ff Beat notes", eof_menu_edit_select_off_beat_notes, NULL, 0, NULL},
	{"&Highlighted", eof_menu_edit_select_highlighted_notes, NULL, 0, NULL},
	{"Not h&Ighlighted", eof_menu_edit_select_non_highlighted_notes, NULL, 0, NULL},
	{"Open notes", eof_menu_edit_select_open_notes, NULL, 0, NULL},
	{"Non open notes", eof_menu_edit_select_non_open_notes, NULL, 0, NULL},
	{"One in &Every", eof_menu_edit_select_note_number_in_sequence, NULL, 0, NULL},
	{"Neighbor &Proximity", eof_menu_edit_select_note_within_threshhold_of_next_note, NULL, 0, NULL},
	{"Gem count", eof_menu_edit_select_gem_count, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_edit_selection_deselect_menu[] =
{
	{"&Chords", eof_menu_edit_deselect_chords, NULL, 0, NULL},
	{"Single &Notes", eof_menu_edit_deselect_single_notes, NULL, 0, NULL},
	{"&Toms", eof_menu_edit_deselect_toms, NULL, 0, NULL},
	{"c&Ymbals", eof_menu_edit_deselect_cymbals, NULL, 0, NULL},
	{"&Grid snapped", eof_menu_edit_deselect_grid_snapped_notes, NULL, 0, NULL},
	{"Non g&Rid snapped", eof_menu_edit_deselect_non_grid_snapped_notes, NULL, 0, NULL},
	{"&Shorter than", eof_menu_edit_deselect_all_shorter_than, NULL, 0, NULL},
	{"&Longer than", eof_menu_edit_deselect_all_longer_than, NULL, 0, NULL},
	{"&Of length", eof_menu_edit_deselect_all_of_length, NULL, 0, NULL},
	{"On &Beat notes", eof_menu_edit_deselect_on_beat_notes, NULL, 0, NULL},
	{"O&Ff Beat notes", eof_menu_edit_deselect_off_beat_notes, NULL, 0, NULL},
	{"&Highlighted", eof_menu_edit_deselect_highlighted_notes, NULL, 0, NULL},
	{"Not h&Ighlighted", eof_menu_edit_deselect_non_highlighted_notes, NULL, 0, NULL},
	{"Open notes", eof_menu_edit_deselect_open_notes, NULL, 0, NULL},
	{"Non open notes", eof_menu_edit_deselect_non_open_notes, NULL, 0, NULL},
	{"One in &Every", eof_menu_edit_deselect_note_number_in_sequence, NULL, 0, NULL},
	{"Neighbor &Proximity", eof_menu_edit_deselect_note_within_threshhold_of_next_note, NULL, 0, NULL},
	{"Gem count", eof_menu_edit_deselect_gem_count, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_edit_selection_menu[] =
{
	{"&Select", NULL, eof_edit_selection_select_menu, 0, NULL},
	{"Select &All\t" CTRL_NAME "+A", eof_menu_edit_select_all, NULL, 0, NULL},
	{"&Conditional select\tALT+C", eof_menu_edit_select_conditional, NULL, 0, NULL},
	{"Select like\t" CTRL_NAME "+L", eof_menu_edit_select_like, NULL, 0, NULL},
	{"&Precise select like\tShift+L", eof_menu_edit_precise_select_like, NULL, 0, NULL},
	{"Select &Rest\tShift+End", eof_menu_edit_select_rest, NULL, 0, NULL},
	{"Select previous\tShift+Home", eof_menu_edit_select_previous, NULL, 0, NULL},
	{"", NULL, NULL, 0, NULL},
	{"&Deselect", NULL, eof_edit_selection_deselect_menu, 0, NULL},
	{"Deselect All\t" CTRL_NAME "+D", eof_menu_edit_deselect_all, NULL, 0, NULL},
	{"Conditional d&Eselect\tALT+D", eof_menu_edit_deselect_conditional, NULL, 0, NULL},
	{"Invert selection", eof_menu_edit_invert_selection, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_edit_menu[] =
{
	{"&Undo\t" CTRL_NAME "+Z", eof_menu_edit_undo, NULL, D_DISABLED, NULL},
	{"&Redo\t" CTRL_NAME "+R", eof_menu_edit_redo, NULL, D_DISABLED, NULL},
	{"", NULL, NULL, 0, NULL},
	{"&Copy\t" CTRL_NAME "+C", eof_menu_edit_copy, NULL, 0, NULL},
	{"Paste\t" CTRL_NAME "+V", eof_menu_edit_paste, NULL, 0, NULL},
	{"Old &Paste", eof_menu_edit_old_paste, NULL, 0, NULL},
	{"Paste &From", NULL, eof_edit_paste_from_menu, 0, NULL},
	{"", NULL, NULL, 0, NULL},
	{"&Grid Snap", NULL, eof_edit_snap_menu, 0, NULL},
	{"&Zoom", NULL, eof_edit_zoom_menu, 0, NULL},
	{"Preview Sp&eed", NULL, eof_edit_speed_menu, 0, NULL},
	{"Playback R&ate", NULL, eof_edit_playback_menu, 0, NULL},
	{"Preview &HOPO", NULL, eof_edit_hopo_menu, 0, NULL},
	{"", NULL, NULL, 0, NULL},
	{"&Metronome\tM", eof_menu_edit_metronome, NULL, 0, NULL},
	{"Claps\tC", eof_menu_edit_claps, NULL, 0, NULL},
	{"Clap &Notes", NULL, eof_edit_claps_menu, 0, NULL},
	{"&Vocal Tones\tV", eof_menu_edit_vocal_tones, NULL, 0, NULL},
	{"MIDI Tones\tShift+T", eof_menu_edit_midi_tones, NULL, 0, NULL},
	{"", NULL, NULL, 0, NULL},
	{"&Bookmark", NULL, eof_edit_bookmark_menu, 0, NULL},
	{"&Selection", NULL, eof_edit_selection_menu, 0, NULL},
	{"Set start point", eof_menu_edit_set_start_point, NULL, 0, NULL},
	{"Set end point", eof_menu_edit_set_end_point, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

DIALOG eof_custom_snap_dialog[] =
{
	/* (proc)                (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags)  (d1) (d2) (dp)         (dp2) (dp3) */
	{ d_agup_shadow_box_proc,32,  68,  170, 95,  2,    23,  0,    0,      0,   0,   NULL,        NULL, NULL },
	{ d_agup_text_proc,		 56,  84,  64,  8,   2,    23,  0,    0,      0,   0,   "Intervals:",NULL, NULL },
	{ eof_custom_grid_snap_edit_proc,112, 80, 66, 20,  2,   23,   0,      0,   2,   0,    eof_etext2,  "0123456789", NULL },
	{ d_agup_radio_proc,     42,  105, 60,  15,  2,    23,  0,    0,      0,   0,   "&Beat",      NULL, NULL },
	{ d_agup_radio_proc,     120, 105, 80,  15,  2,    23,  0,    0,      0,   0,   "&Measure",   NULL, NULL },
	{ d_agup_button_proc,    42,  125, 68,  28,  2,    23,  '\r', D_EXIT, 0,   0,   "OK",        NULL, NULL },
	{ d_agup_button_proc,    120, 125, 68,  28,  2,    23,  0,    D_EXIT, 0,   0,   "Cancel",    NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_custom_speed_dialog[] =
{
	/* (proc)                (x)  (y)  (w)  (h)  (fg)  (bg) (key) (flags) (d1) (d2) (dp)         (dp2)         (dp3) */
	{ d_agup_shadow_box_proc,32,  68,  170, 95,  2,    23,  0,    0,       0,   0,   NULL,       NULL,         NULL },
	{ d_agup_text_proc,      56,  84,  64,  8,   2,    23,  0,    0,       0,   0,   "Percent:", NULL,         NULL },
	{ eof_verified_edit_proc,112, 80,  66,  20,  2,    23,  0,    0,       3,   0,   eof_etext2, "0123456789", NULL },
	{ d_agup_button_proc,    42,  125, 68,  28,  2,    23,  '\r', D_EXIT,  0,   0,   "OK",       NULL,         NULL },
	{ d_agup_button_proc,    120, 125, 68,  28,  2,    23,  0,    D_EXIT,  0,   0,   "Cancel",   NULL,         NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_custom_zoom_dialog[] =
{
	/* (proc)                (x)  (y)  (w)  (h)  (fg)  (bg) (key) (flags) (d1) (d2) (dp)         (dp2)         (dp3) */
	{ d_agup_shadow_box_proc,32,  68,  170, 95,  2,    23,  0,    0,       0,   0,   NULL,		 NULL,         NULL },
	{ d_agup_text_proc,      56,  84,  64,  8,   2,    23,  0,    0,       0,   0,   "1 / ",	 NULL,         NULL },
	{ eof_verified_edit_proc,112, 80,  66,  20,  2,    23,  0,    0,       2,   0,   eof_etext2, "0123456789", NULL },
	{ d_agup_button_proc,    42,  125, 68,  28,  2,    23,  '\r', D_EXIT,  0,   0,   "OK",       NULL,         NULL },
	{ d_agup_button_proc,    120, 125, 68,  28,  2,    23,  0,    D_EXIT,  0,   0,   "Cancel",   NULL,         NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

void eof_prepare_edit_menu(void)
{
	unsigned long i, diffcount = 0;
	unsigned long tracknum;
	int vselected = 0;
	char clipboard_path[50];

	if(eof_song && eof_song_loaded)
	{	//If a chart is loaded
		tracknum = eof_song->track[eof_selected_track]->tracknum;

		/* undo */
		if(eof_undo_count > 0)
		{
			eof_edit_menu[0].flags = 0;
		}
		else
		{
			eof_edit_menu[0].flags = D_DISABLED;
		}

		/* redo */
		if(eof_redo_count)
		{
			eof_edit_menu[1].flags = 0;
		}
		else
		{
			eof_edit_menu[1].flags = D_DISABLED;
		}

		/* copy */
		vselected = eof_count_selected_notes(NULL);
		if(vselected)
		{	//If any notes in the active track difficulty are selected
			eof_edit_menu[3].flags = 0;		//copy
			eof_edit_selection_menu[3].flags = 0;	//select like
			eof_edit_selection_menu[4].flags = 0;	//precise select like

			/* select rest */
			if(eof_selection.current != (eof_get_track_size(eof_song, eof_selected_track) - 1))
			{	//If the selected note isn't the last in the track
				eof_edit_selection_menu[5].flags = 0;
			}
			else
			{
				eof_edit_selection_menu[5].flags = D_DISABLED;
			}

			if(eof_selection.current != 0)
			{
				eof_edit_selection_menu[6].flags = 0;	//select previous
			}
			else
			{
				eof_edit_selection_menu[6].flags = D_DISABLED;	//Select previous cannot be used when the first note/lyric was just selected
			}

			eof_edit_selection_menu[8].flags = 0;	//deselect>
			eof_edit_selection_menu[9].flags = 0;	//deselect all
			eof_edit_selection_menu[10].flags = 0;	//conditional deselect
			eof_edit_selection_menu[11].flags = 0;	//invert selection
			if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
			{	//If a drum track is active
				eof_edit_selection_deselect_menu[2].flags = 0;	//deselect>Toms
				eof_edit_selection_deselect_menu[3].flags = 0;	//deselect>Cymbals
			}
			else
			{
				eof_edit_selection_deselect_menu[2].flags = D_DISABLED;
				eof_edit_selection_deselect_menu[3].flags = D_DISABLED;
			}
		}
		else
		{	//If no notes in the active track difficulty are selected
			eof_edit_menu[3].flags = D_DISABLED;			//copy
			eof_edit_selection_menu[3].flags = D_DISABLED;	//select like
			eof_edit_selection_menu[4].flags = D_DISABLED;	//precise select like
			eof_edit_selection_menu[5].flags = D_DISABLED;	//select rest
			eof_edit_selection_menu[6].flags = D_DISABLED;	//select previous
			eof_edit_selection_menu[8].flags = D_DISABLED;	//deselect>
			eof_edit_selection_menu[9].flags = D_DISABLED;	//deselect all
			eof_edit_selection_menu[10].flags = D_DISABLED;	//conditional deselect
			eof_edit_selection_menu[11].flags = D_DISABLED;	//invert selection
		}

		/* paste, old paste */
		if(eof_vocals_selected)
		{
			(void) snprintf(clipboard_path, sizeof(clipboard_path) - 1, "%seof.vocals.clipboard", eof_temp_path_s);
		}
		else
		{
			(void) snprintf(clipboard_path, sizeof(clipboard_path) - 1, "%seof.clipboard", eof_temp_path_s);
		}
		if(exists(clipboard_path))
		{
			eof_edit_menu[4].flags = 0;
			eof_edit_menu[5].flags = 0;
		}
		else
		{
			eof_edit_menu[4].flags = D_DISABLED;
			eof_edit_menu[5].flags = D_DISABLED;
		}

		/* select all, selection>select */
		if(eof_vocals_selected)
		{
			if(eof_song->vocal_track[tracknum]->lyrics > 0)
			{
				eof_edit_selection_menu[0].flags = 0;
				eof_edit_selection_menu[1].flags = 0;
			}
			else
			{
				eof_edit_selection_menu[0].flags = D_DISABLED;
				eof_edit_selection_menu[1].flags = D_DISABLED;
			}
		}
		else
		{
			if(eof_track_diff_populated_status[eof_note_type])
			{	//If the active track has one or more notes
				eof_edit_selection_menu[0].flags = 0;	//Select
				eof_edit_selection_menu[1].flags = 0;	//Select all
				eof_edit_selection_menu[2].flags = 0;	//Conditional select
			}
			else
			{
				eof_edit_selection_menu[0].flags = D_DISABLED;
				eof_edit_selection_menu[1].flags = D_DISABLED;
				eof_edit_selection_menu[2].flags = D_DISABLED;
			}
		}

		/* selection>select>toms and ...cymbals */
		if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
		{	//If a drum track is active
			eof_edit_selection_select_menu[2].flags = 0;	//select>Toms
			eof_edit_selection_select_menu[3].flags = 0;	//select>Cymbals
		}
		else
		{
			eof_edit_selection_select_menu[2].flags = D_DISABLED;
			eof_edit_selection_select_menu[3].flags = D_DISABLED;
		}

		/* zoom */
		for(i = 0; i < EOF_NUM_ZOOM_LEVELS; i++)
		{
			eof_edit_zoom_menu[i].flags = 0;
		}
		eof_edit_zoom_menu[EOF_NUM_ZOOM_LEVELS].flags = 0;	//Clear the flags from the Edit>Zoom>Custom menu item
		if((eof_zoom > 0) && (eof_zoom <= EOF_NUM_ZOOM_LEVELS))
		{	//If a preset zoom level is in use
			eof_edit_zoom_menu[EOF_NUM_ZOOM_LEVELS - eof_zoom].flags = D_SELECTED;
		}
		else
		{	//If a custom zoom level is in use
			eof_edit_zoom_menu[EOF_NUM_ZOOM_LEVELS].flags = D_SELECTED;
		}
		if(eof_custom_zoom_level)
		{	//If the user defined a custom zoom level
			(void) snprintf(eof_edit_zoom_menu_string, sizeof(eof_edit_zoom_menu_string), "&Custom (1/%d)", eof_custom_zoom_level);	//Build the menu string
		}
		else
		{
			(void) snprintf(eof_edit_zoom_menu_string, sizeof(eof_edit_zoom_menu_string), "&Custom");
		}

		/* hopo */
		for(i = 0; i < EOF_NUM_HOPO_MODES; i++)
		{
			eof_edit_hopo_menu[i].flags = 0;
		}
		eof_edit_hopo_menu[(unsigned)eof_hopo_view].flags = D_SELECTED;

		/* speed */
		eof_edit_playback_menu[0].flags = 0;
		if(eof_playback_time_stretch)
		{
			eof_edit_playback_menu[0].flags = D_SELECTED;
		}
		for(i = 0; i < 4; i++)
		{
			eof_edit_speed_menu[i].flags = 0;	//Erase checkmarks from all 3D preview speeds
		}
		switch(eof_zoom_3d)
		{
			case 5:
			{
				eof_edit_speed_menu[0].flags = D_SELECTED;
				break;
			}
			case 3:
			{
				eof_edit_speed_menu[1].flags = D_SELECTED;
				break;
			}
			case 2:
			{
				eof_edit_speed_menu[2].flags = D_SELECTED;
				break;
			}
			case 1:
			{
				eof_edit_speed_menu[3].flags = D_SELECTED;
				break;
			}
			default:
			break;
		}

		if(eof_selected_track == EOF_TRACK_DANCE)
		{	//If this is the dance track, insert the dance specific paste from menu
			eof_edit_menu[6].child = eof_edit_paste_from_menu_dance;
			eof_active_edit_paste_from_menu = eof_edit_paste_from_menu_dance;
		}
		else
		{	//Otherwise insert the generic paste from menu
			eof_edit_menu[6].child = eof_edit_paste_from_menu;
			eof_active_edit_paste_from_menu = eof_edit_paste_from_menu;
		}

		/* paste from difficulty */
		(void) eof_detect_difficulties(eof_song, eof_selected_track);	//Determine which track difficulties are populated
		for(i = 0; i < 256; i++)
		{	//For each possible difficulty
			if(eof_track_diff_populated_status[i] && (i != eof_note_type))
			{	//If this difficulty is populated and isn't the active difficulty
				diffcount++;	//Increment counter
			}
		}
		if(diffcount == 0)
		{	//No other difficulties are populated
			eof_edit_menu[6].flags = D_DISABLED;	//Disable the paste from submenu
		}
		else
		{
			eof_edit_menu[6].flags = 0;				//Enable the Paste from submenu
		}
		for(i = 0; i < EOF_MAX_DIFFICULTIES; i++)	//For each of the natively supported difficulties
		{
			if((i == EOF_NOTE_CHALLENGE) && (eof_selected_track != EOF_TRACK_DANCE))
				break;	//Don't check the BRE difficulty of non dance tracks

			if((i != eof_note_type) && eof_track_diff_populated_status[i] && !eof_vocals_selected)
			{		//If the difficulty is populated, isn't the active difficulty and PART VOCALS isn't active
				eof_active_edit_paste_from_menu[i].flags = 0;	//Enable paste from the difficulty
			}
			else
			{
				eof_active_edit_paste_from_menu[i].flags = D_DISABLED;	//(Paste from difficulty isn't supposed to be usable in PART VOCALS)
			}
		}

		/* paste from catalog */
		if(eof_selected_catalog_entry < eof_song->catalog->entries)
		{
			if((eof_music_pos >= eof_song->catalog->entry[eof_selected_catalog_entry].start_pos) && (eof_music_pos <= eof_song->catalog->entry[eof_selected_catalog_entry].end_pos) && (eof_song->catalog->entry[eof_selected_catalog_entry].track == eof_selected_track) && (eof_song->catalog->entry[eof_selected_catalog_entry].type == eof_note_type))
			{
				eof_active_edit_paste_from_menu[5].flags = D_DISABLED;
			}
			else if((eof_song->catalog->entry[eof_selected_catalog_entry].track == EOF_TRACK_VOCALS) && !eof_vocals_selected)
			{
				eof_active_edit_paste_from_menu[5].flags = D_DISABLED;
			}
			else if((eof_song->catalog->entry[eof_selected_catalog_entry].track != EOF_TRACK_VOCALS) && eof_vocals_selected)
			{
				eof_active_edit_paste_from_menu[5].flags = D_DISABLED;
			}
			else
			{
				eof_active_edit_paste_from_menu[5].flags = 0;	//Enable Paste from catalog
				eof_edit_menu[6].flags = 0;						//Enable the Paste from menu
			}
		}
		else
		{
			eof_active_edit_paste_from_menu[5].flags = D_DISABLED;
		}

		/* selection */
		for(i = 0; i < 16; i++)
		{
			eof_edit_snap_menu[i].flags = 0;
		}
		switch(eof_snap_mode)
		{
			case EOF_SNAP_QUARTER:
			{
				eof_edit_snap_menu[0].flags = D_SELECTED;
				break;
			}
			case EOF_SNAP_EIGHTH:
			{
				eof_edit_snap_menu[1].flags = D_SELECTED;
				break;
			}
			case EOF_SNAP_SIXTEENTH:
			{
				eof_edit_snap_menu[2].flags = D_SELECTED;
				break;
			}
			case EOF_SNAP_THIRTY_SECOND:
			{
				eof_edit_snap_menu[3].flags = D_SELECTED;
				break;
			}
			case EOF_SNAP_SIXTY_FORTH:
			{
				eof_edit_snap_menu[4].flags = D_SELECTED;
				break;
			}
			case EOF_SNAP_TWELFTH:
			{
				eof_edit_snap_menu[6].flags = D_SELECTED;
				break;
			}
			case EOF_SNAP_TWENTY_FOURTH:
			{
				eof_edit_snap_menu[7].flags = D_SELECTED;
				break;
			}
			case EOF_SNAP_FORTY_EIGHTH:
			{
				eof_edit_snap_menu[8].flags = D_SELECTED;
				break;
			}
			case EOF_SNAP_NINTY_SIXTH:
			{
				eof_edit_snap_menu[9].flags = D_SELECTED;
				break;
			}
			case EOF_SNAP_CUSTOM:
			{
				eof_edit_snap_menu[11].flags = D_SELECTED;
				break;
			}
			case EOF_SNAP_OFF:
			{
				eof_edit_snap_menu[13].flags = D_SELECTED;
				break;
			}
			default:
			break;
		}
		if(eof_render_grid_lines)
		{
			eof_edit_snap_menu[15].flags = D_SELECTED;
		}

		/* MIDI tones */
		if(!eof_midi_initialized)
			eof_edit_menu[18].flags = D_DISABLED;

		/* Update "Paste From" difficulty labeling */
		if(eof_use_fof_difficulty_naming)
		{	//If the user preference is to use Frets on Fire difficulty labeling
			eof_edit_paste_from_menu[0].text = "&Supaeasy";
			eof_edit_paste_from_menu[1].text = "&Easy";
			eof_edit_paste_from_menu[2].text = "&Medium";
			eof_edit_paste_from_menu[3].text = "&Amazing";
		}
		else
		{
			eof_edit_paste_from_menu[0].text = "&Easy";
			eof_edit_paste_from_menu[1].text = "&Medium";
			eof_edit_paste_from_menu[2].text = "&Hard";
			eof_edit_paste_from_menu[3].text = "E&Xpert";
		}
	}//If a chart is loaded
}

int eof_menu_edit_undo(void)
{
	(void) eof_undo_apply();
	eof_redo_toggle = 1;
	if((eof_catalog_menu[0].flags & D_SELECTED) && (eof_song->catalog->entries > 0))
	{
		eof_music_catalog_pos = eof_song->catalog->entry[eof_selected_catalog_entry].start_pos + eof_av_delay;
	}
	return 1;
}

int eof_menu_edit_redo(void)
{
	eof_redo_apply();
	eof_redo_toggle = 0;
	if((eof_catalog_menu[0].flags & D_SELECTED) && (eof_song->catalog->entries > 0))
	{
		eof_music_catalog_pos = eof_song->catalog->entry[eof_selected_catalog_entry].start_pos + eof_av_delay;
	}
	return 1;
}

int eof_menu_edit_copy_vocal(void)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	unsigned long first_pos = -1;
	unsigned long first_beat = ULONG_MAX;
	char note_check = 0;
	int copy_notes = 0;
	PACKFILE * fp;
	int note_selection_updated;
	char clipboard_path[50];
	int first_selected = 1;

	if(!eof_vocals_selected)
		return 1;	//Return error

	note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect
	/* first, scan for selected notes */
	for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
	{
		if((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i])
		{
			copy_notes++;
			if(first_selected || (eof_song->vocal_track[tracknum]->lyric[i]->pos < first_pos))
			{	//If this is the first selected lyric in the active track, or if this lyric is the earliest selected one found so far
				first_pos = eof_song->vocal_track[tracknum]->lyric[i]->pos;
			}
			if(first_selected)
			{	//Track the beat number containing the first selected lyric
				first_beat = eof_get_beat(eof_song, eof_song->vocal_track[tracknum]->lyric[i]->pos);
			}
			first_selected = 0;
		}
	}
	if(copy_notes <= 0)
	{
		return 1;
	}

	/* get ready to write clipboard to disk */
	if(eof_validate_temp_folder())
	{	//Ensure the correct working directory and presence of the temporary folder
		eof_log("\tCould not validate working directory and temp folder", 1);
		return 1;
	}

	(void) snprintf(clipboard_path, sizeof(clipboard_path) - 1, "%seof.vocals.clipboard", eof_temp_path_s);
	fp = pack_fopen(clipboard_path, "w");
	if(!fp)
	{
		allegro_message("Clipboard error!");
		return 1;
	}
	(void) pack_iputl(eof_log_id, fp);		//Store the source EOF instance number
	(void) pack_iputl(copy_notes, fp);
	(void) pack_iputl(first_beat, fp);

	for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
	{	//For each lyric
		if((eof_selection.track != EOF_TRACK_VOCALS) || !eof_selection.multi[i])
			continue;	//If the lyric isn't selected, skip it

		/* check for accidentally moved lyric */
		if(!note_check)
		{
			unsigned long beatnum = eof_get_beat(eof_song, eof_get_note_pos(eof_song, eof_selected_track, i));	//Find which beat that the first selected lyric is currently in

			if(eof_beat_num_valid(eof_song, beatnum))
			{	//If that beat was identified
				if(eof_beat_num_valid(eof_song, beatnum + 1) && (beatnum < eof_song->beats - 1) && (eof_song->beat[beatnum + 1]->pos - eof_get_note_pos(eof_song, eof_selected_track, i) <= 10))
				{	//If there is a next beat and the first selected lyric is within 10 ms of it
					eof_clear_input();
					if(alert(NULL, "First lyric appears to be off.", "Adjust?", "&Yes", "&No", 'y', 'n') == 1)
					{
						eof_set_note_pos(eof_song, eof_selected_track, i, eof_song->beat[beatnum + 1]->pos);	//Move the first selected lyric to the next beat
					}
					eof_clear_input();
				}
				note_check = 1;
			}
		}

		/* write lyric data to disk */
		eof_write_clipboard_note(fp, eof_song, EOF_TRACK_VOCALS, i, first_pos);
		eof_write_clipboard_position_snap_data(fp, eof_get_note_pos(eof_song, EOF_TRACK_VOCALS, i));	//Write the grid snap position data for this lyric's position
	}//For each lyric
	(void) pack_fclose(fp);
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_edit_paste_vocal_logic(int oldpaste)
{
	unsigned long i, j;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	unsigned long paste_pos[EOF_MAX_NOTES] = {0};
	unsigned long paste_count = 0;
	unsigned long first_beat = 0;
	unsigned long this_beat = eof_get_beat(eof_song, eof_music_pos - eof_av_delay);
	unsigned long copy_notes;
	long new_pos = -1;
	long new_end_pos = -1;
	long last_pos = -1;
	EOF_EXTENDED_NOTE temp_lyric = {{0}, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.0, 0.0, 0, 0, 0, {0}, {0}, 0, 0, 0, 0, 0};
	EOF_LYRIC * new_lyric = NULL;
	PACKFILE * fp;
	double newpasteoffset = 0.0;	//This will be used to allow new paste to paste lyrics starting at the seek position instead of the original in-beat positions
	char clipboard_path[50];
	unsigned long source_id = 0;
	unsigned long lastlinenum = 0xFFFFFFFF, linestart = 0, lineend = 0;	//Used to create lyric lines

	//Grid snap variables used to automatically re-snap auto-adjusted timestamps
	int gridsnapbeat = 0;
	char gridsnapvalue = 0;
	unsigned char gridsnapnum = 0;

	if(!eof_vocals_selected)
		return 1;	//Return error
	if(!eof_beat_num_valid(eof_song, this_beat))
		return 1;	//Return error if the seek position isn't within the chart
	if(!oldpaste && (first_beat + this_beat >= eof_song->beats - 1))
	{	//If new paste logic is being used, return from function if the first lyric would paste after the last beat
		return 1;
	}

	/* open the file */
	(void) snprintf(clipboard_path, sizeof(clipboard_path) - 1, "%seof.vocals.clipboard", eof_temp_path_s);
	fp = pack_fopen(clipboard_path, "r");
	if(!fp)
	{
		allegro_message("Clipboard error!\nNothing to paste!");
		return 1;
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NOTE_SEL);
	source_id = pack_igetl(fp);			//Read the source EOF instance number
	copy_notes = pack_igetl(fp);
	first_beat = pack_igetl(fp);
	memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
	eof_selection.current = EOF_MAX_NOTES - 1;
	eof_selection.current_pos = 0;

	if(!oldpaste)
	{	//If using new paste, find the seek position's percentage within the current beat
		newpasteoffset = eof_get_porpos(eof_music_pos - eof_av_delay);
	}
	for(i = 0; i < copy_notes; i++)
	{	//For each lyric in the clipboard file
		/* read the note */
		eof_read_clipboard_note(fp, &temp_lyric, EOF_MAX_LYRIC_LENGTH + 1);
		eof_read_clipboard_position_snap_data(fp, &gridsnapbeat, &gridsnapvalue, &gridsnapnum);	//Read its grid snap data

		if(eof_music_pos + temp_lyric.pos - eof_av_delay >= eof_chart_length)
			continue;	//If this lyric doesn't fit within the chart, skip it

		if(last_pos >= 0)
		{
			last_pos = new_end_pos + 1;
		}
		if(!oldpaste)
		{	//If new paste logic is being used, this lyric pastes into a position relative to the start and end of a beat marker
			if(i == 0)
			{	//If this is the first lyric being pasted
				newpasteoffset = newpasteoffset - temp_lyric.porpos;	//Find the percentage offset that needs to be applied to all start/stop timestamps
			}
			new_pos = eof_put_porpos(temp_lyric.beat - first_beat + this_beat, temp_lyric.porpos, newpasteoffset);
			if(gridsnapvalue)
			{	//If the source lyric was grid snapped
				unsigned long closestpos = new_pos;

				(void) eof_is_any_grid_snap_position(new_pos, NULL, NULL, NULL, &closestpos);	//Get the grid snap position nearest the destination position
				if(closestpos != new_pos)
				{	//If they aren't the same position
					if(closestpos != ULONG_MAX)
					{	//If the nearest grid snap position was determined
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Correcting lyric paste position from %ldms to %lums", new_pos, closestpos);
						eof_log(eof_log_string, 1);
						new_pos = closestpos;	//Update the destination timestamp for the note;
					}
				}
			}
			new_end_pos = eof_put_porpos(temp_lyric.endbeat - first_beat + this_beat, temp_lyric.porendpos, newpasteoffset);
		}
		else
		{	//If old paste logic is being used, this lyric pastes into a position relative to the previous pasted note
			new_pos = eof_music_pos + temp_lyric.pos - eof_av_delay;
			new_end_pos = new_pos + temp_lyric.length;
		}
		if(last_pos < 0)
		{
			last_pos = new_pos;
		}
		eof_menu_edit_paste_clear_range(eof_selected_track, eof_note_type, last_pos, new_end_pos);

		if(!oldpaste)
		{	//If new paste logic is being used, this lyric pastes into a position relative to the start and end of a beat marker
			new_lyric = eof_track_add_create_note(eof_song, eof_selected_track, temp_lyric.note, new_pos, new_end_pos - new_pos, 0, temp_lyric.name);
		}
		else
		{	//If old paste logic is being used, this lyric pastes into a position relative to the previous pasted note
			new_lyric = eof_track_add_create_note(eof_song, eof_selected_track, temp_lyric.note, new_pos, temp_lyric.length, 0, temp_lyric.name);
		}
		if(new_lyric)
		{	//If the lyric was successfully created
			paste_pos[paste_count] = new_lyric->pos;
			paste_count++;

			if(source_id == eof_log_id)
			{	//If the copy/paste is being performed within the same EOF instance
				if(temp_lyric.phrasenum != lastlinenum)
				{	//If this pasted lyric's source line number is different from that of the previous pasted lyric
					if(lastlinenum != 0xFFFFFFFF)
					{	//If the previous pasted lyric's source lyric was in a line
						if(lastlinenum < eof_song->vocal_track[0]->lines)
						{	//Bounds check
							(void) eof_track_add_section(eof_song, eof_selected_track, EOF_LYRIC_PHRASE_SECTION, 0xFF, linestart, lineend, eof_song->vocal_track[0]->line[lastlinenum].flags, NULL);
						}
					}
					linestart = new_lyric->pos;	//Track start of new line
				}
				if(temp_lyric.phrasenum != 0xFFFFFFFF)
				{	//If this pasted lyric's source lyric is in a line
					lineend = new_lyric->pos + new_lyric->length;	//Track end position of line
				}
				lastlinenum = temp_lyric.phrasenum;
			}
		}//If the lyric was successfully created
	}//For each lyric in the clipboard file
	if(source_id == eof_log_id)
	{	//If the copy/paste is being performed within the same EOF instance
		if(lastlinenum != 0xFFFFFFFF)
		{	//If a lyric line was in progress when the last of the lyrics were pasted, add the line now
			if(lastlinenum < eof_song->vocal_track[0]->lines)
			{	//Bounds check
				(void) eof_track_add_section(eof_song, eof_selected_track, EOF_LYRIC_PHRASE_SECTION, 0xFF, linestart, lineend, eof_song->vocal_track[0]->line[lastlinenum].flags, NULL);
			}
		}
	}
	(void) pack_fclose(fp);
	eof_track_sort_notes(eof_song, eof_selected_track);
	eof_track_fixup_notes(eof_song, eof_selected_track, 0);
	if((paste_count > 0) && (eof_selection.track != EOF_TRACK_VOCALS))
	{
		eof_selection.track = EOF_TRACK_VOCALS;
		memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
	}
	for(i = 0; i < paste_count; i++)
	{
		for(j = 0; j < eof_song->vocal_track[tracknum]->lyrics; j++)
		{
			if(eof_song->vocal_track[tracknum]->lyric[j]->pos == paste_pos[i])
			{
				eof_selection.multi[j] = 1;
				break;
			}
		}
	}
	return 1;
}

int eof_menu_edit_cut(unsigned long anchor, int option)
{
	unsigned long i, j, sectiontype;
	char first_pos_found[EOF_TRACKS_MAX] = {0};
	unsigned long first_pos[EOF_TRACKS_MAX] = {0};
	char first_beat_found[EOF_TRACKS_MAX] = {0};
	unsigned long first_beat[EOF_TRACKS_MAX] = {0};	//The beat containing the first note stored for each track
	unsigned long start_pos, end_pos;
	unsigned long last_anchor, next_anchor;
	unsigned long copy_notes[EOF_TRACKS_MAX] = {0};	//The number of notes to store for each track
	double tfloat;
	PACKFILE * fp;
	unsigned long notepos=0;
	long notelength;
	char eof_autoadjust_path[50];

	if(eof_song->beats < 1)
		return 1;	//Invalid beat count

	/* set boundary */
	for(i = 0; i < EOF_TRACKS_MAX; i++)
	{
		eof_anchor_diff[i] = 0;
		eof_tech_anchor_diff[i] = 0;
	}
	last_anchor = eof_find_previous_anchor(eof_song, anchor);
	next_anchor = eof_find_next_anchor(eof_song, anchor);
	start_pos = eof_song->beat[last_anchor]->pos;
	if(!eof_beat_num_valid(eof_song, next_anchor) || (option == 1))
	{
		end_pos = eof_song->beat[eof_song->beats - 1]->pos - 1;
	}
	else
	{
		end_pos = eof_song->beat[next_anchor]->pos;
	}

	/* get ready to write clipboard to disk */
	if(eof_validate_temp_folder())
	{	//Ensure the correct working directory and presence of the temporary folder
		eof_log("\tCould not validate working directory and temp folder", 1);
		return 1;
	}

	(void) snprintf(eof_autoadjust_path, sizeof(eof_autoadjust_path) - 1, "%seof.autoadjust", eof_temp_path_s);
	fp = pack_fopen(eof_autoadjust_path, "w");
	if(!fp)
	{
		allegro_message("Clipboard error!");
		return 1;
	}

	/* copy all tracks */
	for(j = 1; j < eof_song->tracks; j++)
	{	//For each track
		EOF_PRO_GUITAR_TRACK *tp = NULL;
		char restore_tech_view = 0;		//If tech view is in effect, it is temporarily disabled until after the notes have been stored

		if(eof_song->track[j]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If the track being stored is a pro guitar track
			tp = eof_song->pro_guitar_track[eof_song->track[j]->tracknum];
			restore_tech_view = eof_menu_track_get_tech_view_state(eof_song, j);
			eof_menu_track_set_tech_view_state(eof_song, j, 0);	//Disable tech view if applicable
		}

		/* Process notes */
		for(i = 0; i < eof_get_track_size(eof_song, j); i++)
		{	//For each note in the track
			notepos = eof_get_note_pos(eof_song, j, i);
			notelength = eof_get_note_length(eof_song, j, i);
			if((notepos + notelength < start_pos) || (notepos >= end_pos))
				continue;	//If this note is outside the scope of the beat being altered, skip it

			copy_notes[j]++;
			if(!first_pos_found[j])
			{
				first_pos[j] = notepos;
				first_pos_found[j] = 1;
				eof_anchor_diff[j] = eof_get_beat(eof_song, notepos) - last_anchor;
			}
			if(notepos < first_pos[j])
			{
				first_pos[j] = notepos;
				eof_anchor_diff[j] = eof_get_beat(eof_song, notepos) - last_anchor;
			}
			if(!first_beat_found[j])
			{
				first_beat[j] = eof_get_beat(eof_song, notepos);
				first_beat_found[j] = 1;
			}
		}

		/* notes */
		(void) pack_iputl(copy_notes[j], fp);
		(void) pack_iputl(first_beat[j], fp);
		for(i = 0; i < eof_get_track_size(eof_song, j); i++)
		{	//For each note in this track
			notepos = eof_get_note_pos(eof_song, j, i);
			notelength = eof_get_note_length(eof_song, j, i);
			if((notepos + notelength >= start_pos) && (notepos < end_pos))
			{	//If this note falls within the start->end time range
				eof_write_clipboard_note(fp, eof_song, j, i, first_pos[j]);	//Write note data to disk
				eof_write_clipboard_position_snap_data(fp, notepos);		//Write the grid snap position data for this note's position
			}//If this note falls within the start->end time range
		}//For each note in this track

		/* tech notes */
		if(tp)
		{	//If the track being exported is a pro guitar track
			/* Process tech notes */
			eof_menu_pro_guitar_track_enable_tech_view(tp);	//Enable tech view just until the tech notes are stored
			copy_notes[j] = 0;
			first_pos_found[j] = 0;
			first_pos[j] = 0;
			first_beat[j] = 0;
			first_beat_found[j] = 0;
			for(i = 0; i < eof_get_track_size(eof_song, j); i++)
			{	//For each note in the track
				notepos = eof_get_note_pos(eof_song, j, i);
				notelength = eof_get_note_length(eof_song, j, i);
				if((notepos + notelength < start_pos) || (notepos >= end_pos))
					continue;	//If this note is outside the scope of the beat being altered, skip it

				copy_notes[j]++;
				if(!first_pos_found[j])
				{
					first_pos[j] = notepos;
					first_pos_found[j] = 1;
					eof_tech_anchor_diff[j] = eof_get_beat(eof_song, notepos) - last_anchor;
				}
				if(notepos < first_pos[j])
				{
					first_pos[j] = notepos;
					eof_tech_anchor_diff[j] = eof_get_beat(eof_song, notepos) - last_anchor;
				}
				if(!first_beat_found[j])
				{
					first_beat[j] = eof_get_beat(eof_song, notepos);
					first_beat_found[j] = 1;
				}
			}
			(void) pack_iputl(copy_notes[j], fp);
			(void) pack_iputl(first_beat[j], fp);
			for(i = 0; i < eof_get_track_size(eof_song, j); i++)
			{	//For each note in this track
				notepos = eof_get_note_pos(eof_song, j, i);
				notelength = eof_get_note_length(eof_song, j, i);
				if((notepos + notelength >= start_pos) && (notepos < end_pos))
				{	//If this note falls within the start->end time range
					eof_write_clipboard_note(fp, eof_song, j, i, first_pos[j]);	//Write note data to disk
					eof_write_clipboard_position_snap_data(fp, notepos);		//Write the grid snap position data for this tech note's position
				}//If this note falls within the start->end time range
			}//For each note in this track
			eof_menu_pro_guitar_track_disable_tech_view(tp);
		}

		for(sectiontype = 1; sectiontype <= EOF_NUM_SECTION_TYPES; sectiontype++)
		{	//For each type of section that exists
			unsigned long sectionnum, sectioncount = 0;
			EOF_PHRASE_SECTION *phrase = NULL;
			int skip = 0;

			switch(sectiontype)
			{
				case EOF_BOOKMARK_SECTION:
				case EOF_FRET_CATALOG_SECTION:
				case EOF_TRAINER_SECTION:
				case EOF_PREVIEW_SECTION:
				case EOF_RS_POPUP_MESSAGE:
				case EOF_RS_TONE_CHANGE:
					skip = 1;	//These section types are not modified by the auto adjust logic
				break;

				default:
				break;
			}

			if(skip || !eof_lookup_track_section_type(eof_song, j, sectiontype, &sectioncount, &phrase) || !phrase)
				continue;	//If this type of section doesn't get updated by the auto adjust logic or this track doesn't have any instances of this type of section, skip it

			for(sectionnum = 0; sectionnum < sectioncount; sectionnum++)
			{	//For each instance of this type of section in the track
				/* which beat */
				(void) pack_iputl(eof_get_beat(eof_song, phrase[sectionnum].start_pos), fp);
				tfloat = eof_get_porpos(phrase[sectionnum].start_pos);
				(void) pack_fwrite(&tfloat, (long)sizeof(double), fp);
				eof_write_clipboard_position_snap_data(fp, phrase[sectionnum].start_pos);		//Write the grid snap position data for this section's start position

				if(sectiontype != EOF_FRET_HAND_POS_SECTION)
				{	//Each of the auto adjusted phrase types have an end position variable to adjust, except for fret hand positions, which instead store the fret number with that value
					(void) pack_iputl(eof_get_beat(eof_song, phrase[sectionnum].end_pos), fp);
					tfloat = eof_get_porpos(phrase[sectionnum].end_pos);
					(void) pack_fwrite(&tfloat, (long)sizeof(double), fp);
					eof_write_clipboard_position_snap_data(fp, phrase[sectionnum].end_pos);		//Write the grid snap position data for this section's end position
				}
			}
		}

		eof_menu_track_set_tech_view_state(eof_song, j, restore_tech_view);	//Re-enable tech view if applicable
	}//For each track
	(void) pack_fclose(fp);
	return 1;
}

int eof_menu_edit_cut_paste(unsigned long anchor, int option)
{
	unsigned long i, j, b, notenum, sectiontype;
	unsigned long first_beat[EOF_TRACKS_MAX] = {0};	//The first beat of each track that contains a note being adjusted
	unsigned long this_beat[EOF_TRACKS_MAX] = {0};
	unsigned long this_tech_beat[EOF_TRACKS_MAX] = {0};
	unsigned long start_pos, end_pos = 0;
	unsigned long last_anchor, next_anchor;
	PACKFILE * fp;
	unsigned long copy_notes[EOF_TRACKS_MAX];	//The number of notes to adjust for each track
	EOF_EXTENDED_NOTE temp_note = {{0}, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.0, 0.0, 0, 0, 0, {0}, {0}, 0, 0, 0, 0, 0};
	EOF_NOTE * new_note = NULL;
	double tfloat = 0.0;
	unsigned long notepos = 0;
	long notelength = 0;
	char affect_until_end = 0;	//Is set to nonzero if all notes until the end of the project are affected by this operation
	char eof_autoadjust_path[50];

	//Grid snap variables used to automatically re-snap auto-adjusted timestamps
	int beat = 0;
	char gridsnapvalue = 0;
	unsigned char gridsnapnum = 0;
	unsigned long gridpos = 0;

	for(i = 0; i < EOF_TRACKS_MAX; i++)
	{
		this_beat[i] = eof_find_previous_anchor(eof_song, anchor) + eof_anchor_diff[i];
		this_tech_beat[i] = eof_find_previous_anchor(eof_song, anchor) + eof_tech_anchor_diff[i];
	}

	/* set boundary */
	last_anchor = eof_find_previous_anchor(eof_song, anchor);
	next_anchor = eof_find_next_anchor(eof_song, anchor);
	start_pos = eof_song->beat[last_anchor]->pos;
	if(!eof_beat_num_valid(eof_song, next_anchor) || (option == 1))
	{	//If there was no following anchor, or the notes are being auto-adjusted
		affect_until_end = 1;	//Track this condition, as the last beat's position may now be before some notes and may no longer be a suitable end point
	}
	else
	{
		end_pos = eof_song->beat[next_anchor]->pos;
	}

	if(eof_validate_temp_folder())
	{	//Ensure the correct working directory and presence of the temporary folder
		eof_log("\tCould not validate working directory and temp folder", 1);
		return 1;
	}

	(void) snprintf(eof_autoadjust_path, sizeof(eof_autoadjust_path) - 1, "%seof.autoadjust", eof_temp_path_s);
	fp = pack_fopen(eof_autoadjust_path, "r");
	if(!fp)
	{
		allegro_message("Clipboard error!");
		return 1;
	}

///Unclear why this function call would be needed
///	eof_calculate_beats(eof_song);	//Call this before placing notes so that the auto re-snap logic can avoid any loss of grid snap due to beat position rounding errors

	memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
	for(j = 1; j < eof_song->tracks; j++)
	{	//For each track
		EOF_PRO_GUITAR_TRACK *tp = NULL;
		char restore_tech_view = 0;		//If tech view is in effect, it is temporarily disabled until after the notes have been stored

		if(eof_song->track[j]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If the track being written is a pro guitar track
			tp = eof_song->pro_guitar_track[eof_song->track[j]->tracknum];
			restore_tech_view = eof_menu_track_get_tech_view_state(eof_song, j);
			eof_menu_track_set_tech_view_state(eof_song, j, 0);	//Disable tech view if applicable
		}

		//Empty the notes from the track
		for(i = eof_get_track_size(eof_song, j); i > 0; i--)
		{	//For each note in the track, in reverse order
			notepos = eof_get_note_pos(eof_song, j, i - 1);
			if((notepos + eof_get_note_length(eof_song, j, i - 1) >= start_pos) && (affect_until_end || (notepos < end_pos)))
			{	//If the note is in the affected range of the chart (between affected anchors or between an anchor and the end of the project)
				eof_track_delete_note(eof_song, j, i - 1);	//Delete the note
			}
		}

		//Re-populate the notes
		copy_notes[j] = pack_igetl(fp);
		first_beat[j] = pack_igetl(fp);

		for(i = 0; i < copy_notes[j]; i++)
		{
		/* read the note */
			eof_read_clipboard_note(fp, &temp_note, EOF_MAX_LYRIC_LENGTH + 1);
			eof_read_clipboard_position_snap_data(fp, &beat, &gridsnapvalue, &gridsnapnum);

			if(temp_note.pos + temp_note.length < eof_chart_length)
			{
				notepos = eof_put_porpos(temp_note.beat - first_beat[j] + this_beat[j], temp_note.porpos, 0.0);

				if(eof_find_grid_snap_position(beat, gridsnapvalue, gridsnapnum, &gridpos))
				{	//If the adjusted grid snap position can be calculated
					notepos = gridpos;	//Update the adjusted position for the note
				}
				notelength = eof_put_porpos(temp_note.endbeat - first_beat[j] + this_beat[j], temp_note.porendpos, 0.0) - notepos;
				new_note = eof_track_add_create_note(eof_song, j, temp_note.note, notepos, notelength, temp_note.type, temp_note.name);

				if(new_note)
				{	//If the note was successfully created
					notenum = eof_get_track_size(eof_song, j) - 1;	//Get the index of the note that was just created
					eof_set_note_flags(eof_song, j, notenum, temp_note.flags);		//Set the last created note's flags
					eof_set_note_eflags(eof_song, j, notenum, temp_note.eflags);	//Set the last created note's extended flags
					eof_set_note_accent(eof_song, j, notenum, temp_note.accent);	//Set the last created note's accent bitmask

					if(eof_song->track[j]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
					{	//If this is a pro guitar track
						EOF_PRO_GUITAR_NOTE *np = eof_song->pro_guitar_track[eof_song->track[j]->tracknum]->note[notenum];	//Simplify

						np->legacymask = temp_note.legacymask;							//Copy the legacy bitmask to the last created pro guitar note
						memcpy(np->frets, temp_note.frets, sizeof(temp_note.frets));	//Copy the fret array to the last created pro guitar note
						memcpy(np->finger, temp_note.finger, sizeof(temp_note.finger));	//Copy the finger array to the last created pro guitar note
						np->ghost = temp_note.ghost;									//Copy the ghost bitmask to the last created pro guitar note
						np->bendstrength = temp_note.bendstrength;						//Copy the bend height to the last created pro guitar note
						np->slideend = temp_note.slideend;								//Copy the slide end position to the last created pro guitar note
						np->unpitchend = temp_note.unpitchend;							//Copy the slide end position to the last created pro guitar note
					}
				}
			}
			else
			{
				eof_log("Note dropped during cut paste", 2);
			}
		}
		eof_track_sort_notes(eof_song, j);

		//Adjust the tech notes, if any
		if(tp)
		{	//If the track being exported is a pro guitar track
			eof_menu_pro_guitar_track_enable_tech_view(tp);	//Enable tech view just until the tech notes are restored

			//Empty the notes from the track
			for(i = eof_get_track_size(eof_song, j); i > 0; i--)
			{	//For each note in the track, in reverse order
				notepos = eof_get_note_pos(eof_song, j, i - 1);
				if((notepos + eof_get_note_length(eof_song, j, i - 1) >= start_pos) && (affect_until_end || (notepos < end_pos)))
				{	//If the note is in the affected range of the chart (between affected anchors or between an anchor and the end of the project)
					eof_track_delete_note(eof_song, j, i - 1);	//Delete the note
				}
			}

			//Re-populate the tech notes
			copy_notes[j] = pack_igetl(fp);
			first_beat[j] = pack_igetl(fp);
			for(i = 0; i < copy_notes[j]; i++)
			{
				/* read the note */
				eof_read_clipboard_note(fp, &temp_note, EOF_MAX_LYRIC_LENGTH + 1);
				eof_read_clipboard_position_snap_data(fp, &beat, &gridsnapvalue, &gridsnapnum);

				if(temp_note.pos + temp_note.length >= eof_chart_length)
					continue;	//If this note doesn't fit within the chart, skip it

				notepos = eof_put_porpos(temp_note.beat - first_beat[j] + this_tech_beat[j], temp_note.porpos, 0.0);
				if(eof_find_grid_snap_position(beat, gridsnapvalue, gridsnapnum, &gridpos))
				{	//If the adjusted grid snap position can be calculated
					notepos = gridpos;	//Update the adjusted position for the note
				}
				notelength = eof_put_porpos(temp_note.endbeat - first_beat[j] + this_tech_beat[j], temp_note.porendpos, 0.0) - notepos;
				new_note = eof_track_add_create_note(eof_song, j, temp_note.note, notepos, notelength, temp_note.type, temp_note.name);

				if(!new_note)
					continue;	//If the note was not successfully created, skip the logic below

				notenum = eof_get_track_size(eof_song, j) - 1;	//Get the index of the note that was just created
				eof_set_note_flags(eof_song, j, notenum, temp_note.flags);	//Set the last created note's flags

				if(eof_song->track[j]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
				{	//If this is a pro guitar track
					EOF_PRO_GUITAR_NOTE *np = eof_song->pro_guitar_track[eof_song->track[j]->tracknum]->note[notenum];	//Simplify

					np->legacymask = temp_note.legacymask;							//Copy the legacy bitmask to the last created pro guitar note
					memcpy(np->frets, temp_note.frets, sizeof(temp_note.frets));	//Copy the fret array to the last created pro guitar note
					memcpy(np->finger, temp_note.finger, sizeof(temp_note.finger));	//Copy the finger array to the last created pro guitar note
					np->ghost = temp_note.ghost;									//Copy the ghost bitmask to the last created pro guitar note
					np->bendstrength = temp_note.bendstrength;						//Copy the bend height to the last created pro guitar note
					np->slideend = temp_note.slideend;								//Copy the slide end position to the last created pro guitar note
					np->unpitchend = temp_note.unpitchend;							//Copy the slide end position to the last created pro guitar note
					np->eflags = temp_note.eflags;									//Copy the extended track flags
				}
			}
			eof_track_sort_notes(eof_song, j);
			eof_menu_pro_guitar_track_disable_tech_view(tp);
		}//If the track being exported is a pro guitar track

		for(sectiontype = 1; sectiontype <= EOF_NUM_SECTION_TYPES; sectiontype++)
		{	//For each type of section that exists
			unsigned long sectionnum, sectioncount = 0;
			EOF_PHRASE_SECTION *phrase = NULL;
			int skip = 0;

			switch(sectiontype)
			{
				case EOF_BOOKMARK_SECTION:
				case EOF_FRET_CATALOG_SECTION:
				case EOF_TRAINER_SECTION:
				case EOF_PREVIEW_SECTION:
				case EOF_RS_POPUP_MESSAGE:
				case EOF_RS_TONE_CHANGE:
					skip = 1;	//These section types are not modified by the auto adjust logic
				break;

				default:
				break;
			}

			if(skip || !eof_lookup_track_section_type(eof_song, j, sectiontype, &sectioncount, &phrase) || !phrase)
				continue;	//If this type of section doesn't get updated by the auto adjust logic or there are no instances of this type of section, skip it

			for(sectionnum = 0; sectionnum < sectioncount; sectionnum++)
			{	//For each instance of this type of section in the track
				/* which beat */
				b = pack_igetl(fp);
				(void) pack_fread(&tfloat, (long)sizeof(double), fp);								//Read the floating point position of the section's start position
				eof_read_clipboard_position_snap_data(fp, &beat, &gridsnapvalue, &gridsnapnum);		//Read the grid snap position data for the section's start position

				phrase[sectionnum].start_pos = eof_put_porpos(b, tfloat, 0.0);
				if(eof_find_grid_snap_position(beat, gridsnapvalue, gridsnapnum, &gridpos))
				{	//If the adjusted grid snap position can be calculated
					phrase[sectionnum].start_pos = gridpos;	//Update the adjusted position for the section
				}

				if(sectiontype != EOF_FRET_HAND_POS_SECTION)
				{	//Each of the auto adjusted phrase types have an end position variable to adjust, except for fret hand positions, which instead store the fret number with that value
					b = pack_igetl(fp);
					(void) pack_fread(&tfloat, (long)sizeof(double), fp);								//Read the floating point position of the section's end position
					eof_read_clipboard_position_snap_data(fp, &beat, &gridsnapvalue, &gridsnapnum);		//Read the grid snap position data for the section's end position
					phrase[sectionnum].end_pos = eof_put_porpos(b, tfloat, 0.0);
					if(eof_find_grid_snap_position(beat, gridsnapvalue, gridsnapnum, &gridpos))
					{	//If the adjusted grid snap position can be calculated
						phrase[sectionnum].end_pos = gridpos;	//Update the adjusted position for the section
					}
				}
			}
		}

		eof_menu_track_set_tech_view_state(eof_song, j, restore_tech_view);	//Re-enable tech view if applicable
	}//For each track
	(void) pack_fclose(fp);
///	eof_calculate_beats(eof_song);	//It's not clear why this is being called at all, this function isn't meant to change beat positions, just perform auto-adjust logic
	eof_truncate_chart(eof_song);	//Add or remove beat markers as necessary and update the eof_chart_length variable
	eof_fixup_notes(eof_song);
	eof_determine_phrase_status(eof_song, eof_selected_track);
	return 1;
}

int eof_menu_edit_copy(void)
{
	unsigned long i;
	unsigned long first_pos = 0, note_pos;
	char first_pos_read = 0;
	unsigned long first_beat = 0;
	char first_beat_read = 0;
	char note_check = 0;
	unsigned long copy_notes = 0;
	PACKFILE * fp;
	int note_selection_updated;
	char clipboard_path[50];

	if(eof_vocals_selected)
	{
		return eof_menu_edit_copy_vocal();
	}

	note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect
	/* first, scan for selected notes */
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type) && (eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the active difficulty, is in the active track and is selected
			copy_notes++;
			note_pos = eof_get_note_pos(eof_song, eof_selected_track, i);
			if(!first_pos_read || (note_pos < first_pos))
			{	//Track the position of the first note in the selection
				first_pos = note_pos;
				first_pos_read = 1;
			}
			if(!first_beat_read)
			{
				first_beat = eof_get_beat(eof_song, eof_get_note_pos(eof_song, eof_selected_track, i));
				first_beat_read = 1;
			}
		}
	}
	if(copy_notes == 0)
	{
		return 1;
	}

	/* get ready to write clipboard to disk */
	if(eof_validate_temp_folder())
	{	//Ensure the correct working directory and presence of the temporary folder
		eof_log("\tCould not validate working directory and temp folder", 1);
		return 1;
	}

	(void) snprintf(clipboard_path, sizeof(clipboard_path) - 1, "%seof.clipboard", eof_temp_path_s);
	fp = pack_fopen(clipboard_path, "w");
	if(!fp)
	{
		allegro_message("Clipboard error!");
		return 1;
	}
	(void) pack_iputl(eof_log_id, fp);			//Store the source EOF instance number
	(void) pack_iputl(eof_selected_track, fp);	//Store the source track number
	(void) pack_putc(eof_track_is_ghl_mode(eof_song, eof_selected_track), fp);	//Store the GHL mode status
	(void) pack_iputl(copy_notes, fp);			//Store the number of notes that will be stored to clipboard
	(void) pack_iputl(first_beat, fp);			//Store the beat number of the first note that will be stored to clipboard

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_get_note_type(eof_song, eof_selected_track, i) != eof_note_type) || (eof_selection.track != eof_selected_track) || !eof_selection.multi[i])
			continue;	//If the note isn't selected or in the active track difficulty, skip it

		/* check for accidentally moved note */
		if(!note_check)
		{
			unsigned long beatnum = eof_get_beat(eof_song, eof_get_note_pos(eof_song, eof_selected_track, i));	//Find which beat that the first selected note is currently in

			if(eof_beat_num_valid(eof_song, beatnum))
			{	//If that beat was identified
				if(eof_beat_num_valid(eof_song, beatnum + 1) && (beatnum < eof_song->beats - 1) && (eof_song->beat[beatnum + 1]->pos - eof_get_note_pos(eof_song, eof_selected_track, i) <= 10))
				{	//If there is a next beat and the first selected note is within 10 ms of it
					eof_clear_input();
					if(alert(NULL, "First note appears to be off.", "Adjust?", "&Yes", "&No", 'y', 'n') == 1)
					{
						eof_set_note_pos(eof_song, eof_selected_track, i, eof_song->beat[beatnum + 1]->pos);	//Move the first selected note to the next beat
					}
					eof_clear_input();
				}
				note_check = 1;
			}
		}

		/* write note data to disk */
		eof_write_clipboard_note(fp, eof_song, eof_selected_track, i, first_pos);
		eof_write_clipboard_position_snap_data(fp, eof_get_note_pos(eof_song, eof_selected_track, i));	//Write the grid snap position data for this note's position
	}
	(void) pack_fclose(fp);
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_edit_paste_logic(int oldpaste)
{
	unsigned long i, j;
	unsigned long paste_pos[EOF_MAX_NOTES] = {0};
	unsigned long paste_count = 0;
	unsigned long first_beat = 0;
	unsigned long this_beat = eof_get_beat(eof_song, eof_music_pos - eof_av_delay);
	unsigned long copy_notes;
	PACKFILE * fp;
	EOF_EXTENDED_NOTE temp_note = {{0}, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.0, 0.0, 0, 0, 0, {0}, {0}, 0, 0, 0, 0, 0}, first_note = {{0}, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.0, 0.0, 0, 0, 0, {0}, {0}, 0, 0, 0, 0, 0};
	EOF_EXTENDED_NOTE last_note;
	EOF_NOTE * new_note = NULL;
	unsigned long sourcetrack = 0;	//Will store the track that this clipboard data was from
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	unsigned long srctracknum;
	unsigned long highestfret, highestlane;
	unsigned long numlanes = eof_count_track_lanes(eof_song, eof_selected_track);
	unsigned long maxbitmask = (1 << numlanes) - 1;	//A bitmask representing the highest valid note bitmask (a gem on all used lanes in the destination track)
	double newpasteoffset = 0.0;	//This will be used to allow new paste to paste notes starting at the seek position instead of the original in-beat positions
	unsigned long lastarpeggnum = 0xFFFFFFFF, arpeggstart = 0, arpeggend = 0;	//Used to create arpeggio/handshape phrases
	char clipboard_path[50];
	int warning = 0;
	char isghl;	//Set to nonzero if the clipboard's source track had GHL mode enabled, which changes the interpretation of lane 6 gems

	//Grid snap variables used to automatically re-snap auto-adjusted timestamps
	int gridsnapbeat = 0;
	char gridsnapvalue = 0;
	unsigned char gridsnapnum = 0;
	unsigned long source_id = 0;

	if(eof_vocals_selected)
	{	//The vocal track uses its own clipboard logic
		return eof_menu_edit_paste_vocal_logic(oldpaste);	//Call the old or new vocal paste logic accordingly
	}

	/* open the file */
	(void) snprintf(clipboard_path, sizeof(clipboard_path) - 1, "%seof.clipboard", eof_temp_path_s);
	fp = pack_fopen(clipboard_path, "r");
	if(!fp)
	{
		allegro_message("Clipboard error!\nNothing to paste!");
		return 1;
	}
	if(!oldpaste && (first_beat + this_beat >= eof_song->beats - 1))
	{	//If new paste logic is being used, return from function if the first note would paste after the last beat
		return 1;
	}
	source_id = pack_igetl(fp);			//Read the source EOF instance number
	sourcetrack = pack_igetl(fp);		//Read the source track of the clipboard data
	srctracknum = eof_song->track[sourcetrack]->tracknum;
	isghl = pack_getc(fp);				//Read the GHL mode status
	copy_notes = pack_igetl(fp);		//Read the number of notes on the clipboard
	first_beat = pack_igetl(fp);		//Read the original beat number of the first note that was copied
	if(!copy_notes)
	{	//If there are 0 notes on the clipboard, return without making an undo
		return 1;
	}
	if((eof_song->track[sourcetrack]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT) && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
	{	//If the source and destination track are both pro guitar format, pre-check to ensure that the pasted notes won't go above the current track's fret limit
		highestfret = eof_get_highest_clipboard_fret(clipboard_path);
		if(highestfret > eof_song->pro_guitar_track[tracknum]->numfrets)
		{	//If any notes on the clipboard would exceed the active track's fret limit
			char message[120] = {0};
			(void) snprintf(message, sizeof(message) - 1, "Warning:  This track's fret limit is exceeded by a pasted note's fret value of %lu.  Continue?", highestfret);
			eof_clear_input();
			if(alert(NULL, message, NULL, "&Yes", "&No", 'y', 'n') != 1)
			{	//If user does not opt to continue after being alerted of this fret limit issue
				(void) pack_fclose(fp);
				return 0;
			}
		}
	}
	highestlane = eof_get_highest_clipboard_lane(clipboard_path);
	if(highestlane > numlanes)
	{	//If any notes on the clipboard exceed the active track's lane limit
		char message[120] = {0};
		(void) snprintf(message, sizeof(message) - 1, "Warning:  This track's highest lane number is exceeded by a pasted note with a gem on lane %lu.", highestlane);
		eof_clear_input();
		if(alert(NULL, message, "Gems will either be dropped, or added to form all-lane chords for such notes.  Continue?", "&Yes", "&No", 'y', 'n') != 1)
		{	//If user does not opt to continue after being alerted of this lane limit issue
			(void) pack_fclose(fp);
			return 0;
		}
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NOTE_SEL);

	if(!oldpaste)
	{	//If using new paste, find the seek position's percentage within the current beat
		newpasteoffset = eof_get_porpos(eof_music_pos - eof_av_delay);
	}

	memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
	eof_selection.current = EOF_MAX_NOTES - 1;
	eof_selection.current_pos = 0;

	if(eof_paste_erase_overlap)
	{	//If the user decided to delete existing notes that are between the start and end of the pasted notes
		unsigned long clear_start, clear_end;

		eof_read_clipboard_note(fp, &first_note, EOF_NAME_LENGTH + 1);	//Read the first note on the clipboard
		eof_read_clipboard_position_snap_data(fp, &gridsnapbeat, &gridsnapvalue, &gridsnapnum);	//Read its grid snap data
		memcpy(&last_note, &first_note, sizeof(first_note));	//Clone the first clipboard note into last_note in case there aren't any other notes on the clipboard
		for(i = 1; i < copy_notes; i++)
		{	//For each remaining note on the clipboard
			eof_read_clipboard_note(fp, &last_note, EOF_NAME_LENGTH + 1);	//Read the note
			eof_read_clipboard_position_snap_data(fp, &gridsnapbeat, &gridsnapvalue, &gridsnapnum);	//Read its grid snap data
		}
		//At this point, last_note contains the data for the last note on the clipboard.  Determine the time span of notes that would need to be cleared
		if(!oldpaste)
		{	//If new paste logic is being used, this note pastes into a position relative to the start and end of a beat marker
			newpasteoffset = newpasteoffset - first_note.porpos;	//Find the percentage offset that needs to be applied to all start/stop timestamps
			clear_start = eof_put_porpos(first_note.beat - first_beat + this_beat, first_note.porpos, newpasteoffset);	//The position that "new paste" would paste the first note at
			clear_end = eof_put_porpos(last_note.beat - first_beat + this_beat, last_note.porpos, newpasteoffset);		//The position where "new paste" would paste the last note at
		}
		else
		{	//If old paste logic is being used, this note pastes into a position relative to the previous pasted note
			clear_start = eof_music_pos + first_note.pos - eof_av_delay;	//The position that "old paste" would paste the first note at
			clear_end = eof_music_pos + last_note.pos - eof_av_delay;	//The position where "old paste" would paste the last note at
		}
		eof_menu_edit_paste_clear_range(eof_selected_track, eof_note_type, clear_start, clear_end);
		//The packfile functions have no seek routine, so the file has to be closed, re-opened and repositioned to the first clipboard note for the actual paste logic
		(void) pack_fclose(fp);
		fp = pack_fopen(clipboard_path, "r");
		if(!fp)
		{
			allegro_message("Error re-opening clipboard");
			return 1;
		}
		source_id = pack_igetl(fp);			//Read the source EOF instance number
		sourcetrack = pack_igetl(fp);		//Read the source track of the clipboard data
		isghl = pack_getc(fp);				//Read the GHL mode status
		copy_notes = pack_igetl(fp);		//Read the number of notes on the clipboard
		first_beat = pack_igetl(fp);		//Read the original beat number of the first note that was copied
	}

	if(!oldpaste)
	{	//If using new paste, find the seek position's percentage within the current beat
		newpasteoffset = eof_get_porpos(eof_music_pos - eof_av_delay);
	}
	for(i = 0; i < copy_notes; i++)
	{	//For each note in the clipboard file
		unsigned long newnotepos, match = 0, flags, eflags;
		long newnotelength;
		EOF_PRO_GUITAR_NOTE *np;

		eof_read_clipboard_note(fp, &temp_note, EOF_NAME_LENGTH + 1);	//Read the note
		eof_read_clipboard_position_snap_data(fp, &gridsnapbeat, &gridsnapvalue, &gridsnapnum);	//Read its grid snap data

		//Add enough beats to encompass the pasted note if necessary
		while(1)
		{
			if(oldpaste)
			{	//If old paste is being performed, the chart simply has to be long enough to accommodate the pasted note's original length
				if(eof_music_pos + temp_note.pos + temp_note.length - eof_av_delay < eof_song->beat[eof_song->beats - 1]->pos)
				{	//If there are enough beats already
					break;
				}
			}
			else
			{	//If new paste is being performed, the paste re-aligns the pasted note to the beat map
				if(temp_note.endbeat - first_beat + this_beat < eof_song->beats - 1)
				{	//If there are enough beats already
					break;
				}
			}
			if(!eof_song_append_beats(eof_song, 1))
			{	//If there was an error adding a beat
				eof_log("\tError adding beat.  Aborting", 1);
				return 1;
			}
			eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current
		}
		if((eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT) && (temp_note.legacymask != 0))
		{	//If the copied note indicated that this overrides the original bitmask (pasting pro guitar into a legacy track)
			temp_note.note = temp_note.legacymask;
		}
		if((temp_note.note > maxbitmask) && ((temp_note.note & maxbitmask) == 0))
		{	//If this note only uses lanes higher than the active track allows
			temp_note.note = maxbitmask;	//Alter this note to be an all-lane chord
		}
		eof_sanitize_note_flags(&temp_note.flags, sourcetrack, eof_selected_track);	//Ensure the note flags are validated for the track being pasted into

		/* create/merge the note */
		if(!oldpaste)
		{	//If new paste logic is being used, this note pastes into a position relative to the start and end of a beat marker
			if(i == 0)
			{	//If this is the first note being pasted
				newpasteoffset = newpasteoffset - temp_note.porpos;	//Find the percentage offset that needs to be applied to all start/stop timestamps
			}
			newnotepos = eof_put_porpos(temp_note.beat - first_beat + this_beat, temp_note.porpos, newpasteoffset);
			if(gridsnapvalue)
			{	//If the source note was grid snapped
				unsigned long closestpos = newnotepos;

				(void) eof_is_any_grid_snap_position(newnotepos, NULL, NULL, NULL, &closestpos);	//Get the grid snap position nearest the destination position
				if(closestpos != newnotepos)
				{	//If they aren't the same position
					if(closestpos != ULONG_MAX)
					{	//If the nearest grid snap position was determined
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Correcting note paste position from %lums to %lums", newnotepos, closestpos);
						eof_log(eof_log_string, 1);
						newnotepos = closestpos;	//Update the destination timestamp for the note;
					}
				}
			}
			newnotelength = eof_put_porpos(temp_note.endbeat - first_beat + this_beat, temp_note.porendpos, newpasteoffset) - newnotepos;
		}
		else
		{	//If old paste logic is being used, this note pastes into a position relative to the previous pasted note
			newnotepos = eof_music_pos + temp_note.pos - eof_av_delay;
			newnotelength = temp_note.length;
		}
		if(!eof_paste_erase_overlap && eof_search_for_note_near(eof_song, eof_selected_track, newnotepos, 2, eof_note_type, &match))
		{	//If using the default paste behavior (a note merges with a note that it overlaps), and this pasted note would overlap another
			//Erase any lane specific flags in the matching note that correspond with used lanes in the note being pasted
			flags = eof_prepare_note_flag_merge(eof_get_note_flags(eof_song, eof_selected_track, match), eof_song->track[eof_selected_track]->track_behavior, temp_note.note);
				//Get the flags of the overlapped note as they would be if all applicable lane-specific flags are cleared to inherit the flags of the note to merge
			flags |= temp_note.flags;	//Merge the pasted note's flags
			eof_set_note_flags(eof_song, eof_selected_track, match, flags);	//Apply the updated flags to the overlapped note

			eflags = eof_get_note_eflags(eof_song, eof_selected_track, match);
			eflags |= temp_note.eflags;
			eof_set_note_eflags(eof_song, eof_selected_track, match, eflags);	//Marge the extended flags

			eof_set_note_note(eof_song, eof_selected_track, match, eof_get_note_note(eof_song, eof_selected_track, match) | temp_note.note);	//Merge the note bitmask
			eof_set_note_accent(eof_song, eof_selected_track, match, eof_get_note_accent(eof_song, eof_selected_track, match) | temp_note.accent);	//Merge the accent bitmask
			//Erase ghost and legacy flags
			if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
			{
				eof_song->pro_guitar_track[tracknum]->note[match]->legacymask = 0;	//Clear the legacy bit mask
				eof_song->pro_guitar_track[tracknum]->note[match]->ghost = 0;	//Clear the ghost bit mask
			}
		}
		else
		{	//This will paste as a new note
			new_note = eof_track_add_create_note(eof_song, eof_selected_track, temp_note.note, newnotepos, newnotelength, eof_note_type, temp_note.name);
			if(new_note)
			{
				eof_set_note_flags(eof_song, eof_selected_track, eof_get_track_size(eof_song, eof_selected_track) - 1, temp_note.flags);
				eof_set_note_eflags(eof_song, eof_selected_track, eof_get_track_size(eof_song, eof_selected_track) - 1, temp_note.eflags);
				eof_set_note_accent(eof_song, eof_selected_track, eof_get_track_size(eof_song, eof_selected_track) - 1, temp_note.accent);
				if(isghl != eof_track_is_ghl_mode(eof_song, eof_selected_track))
				{	//If the source and destination tracks for this paste didn't have matching GHL mode status, convert the note
					if(eof_note_convert_ghl_authoring(eof_song, eof_selected_track, eof_get_track_size(eof_song, eof_selected_track) - 1))
					{	//If this paste had a lossy conversion from GHL to non GHL format
						if(!warning)
						{	//If the user wasn't already warned about this during this paste operation
							allegro_message("Chords containing lane 3 white GHL gems can't be authored in a non GHL track");
							warning = 1;
						}
					}
				}
				paste_pos[paste_count] = eof_get_note_pos(eof_song, eof_selected_track, eof_get_track_size(eof_song, eof_selected_track) - 1);
				paste_count++;
			}
		}

		/* process pro guitar data */
		if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
			continue;	//If the track being pasted into isn't a pro guitar track, skip the remainder of the logic below

		np = eof_song->pro_guitar_track[tracknum]->note[eof_song->pro_guitar_track[tracknum]->notes - 1];	//Simplify
		np->legacymask = temp_note.legacymask;							//Copy the legacy bitmask to the last created pro guitar note
		memcpy(np->frets, temp_note.frets, sizeof(temp_note.frets));	//Copy the fret array to the last created pro guitar note
		memcpy(np->finger, temp_note.finger, sizeof(temp_note.finger));	//Copy the finger array to the last created pro guitar note
		np->ghost = temp_note.ghost;									//Copy the ghost bitmask to the last created pro guitar note
		np->bendstrength = temp_note.bendstrength;						//Copy the bend height to the last created pro guitar note
		np->slideend = temp_note.slideend;								//Copy the slide end position to the last created pro guitar note
		np->unpitchend = temp_note.unpitchend;							//Copy the slide end position to the last created pro guitar note
		if(!eof_menu_track_get_tech_view_state(eof_song, eof_selected_track))
		{	//If the active track does not have tech view enabled
			temp_note.eflags &= ~EOF_PRO_GUITAR_NOTE_EFLAG_STOP;		//Clear these tech note only statuses
			temp_note.eflags &= ~EOF_PRO_GUITAR_NOTE_EFLAG_PRE_BEND;
		}
		if(eof_song->track[sourcetrack]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If a non pro guitar note is being pasted into a pro guitar track
			unsigned char legacymask = temp_note.note & 31;	//Determine the appropriate legacy mask to apply (drop lane 6)
			if(!legacymask)
			{	//If the note only contained lane 6
				legacymask = 31;	//Make it chord on all 5 lanes
			}
			np->legacymask = legacymask;
		}

		if(eof_song->track[sourcetrack]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
			continue;	//If the source track isn't a pro guitar track, skip the logic below

		//Otherwise paste arpeggio/handshape phrasing
		if(source_id == eof_log_id)
		{	//If the copy/paste is being performed within the same EOF instance
			if(temp_note.phrasenum != lastarpeggnum)
			{	//If this pasted note's source arpeggio is different from that of the previous pasted note
				if(lastarpeggnum != 0xFFFFFFFF)
				{	//If the previous pasted note's source note was in an arpeggio/handshape
					if(lastarpeggnum < eof_song->pro_guitar_track[srctracknum]->arpeggios)
					{	//Bounds check
						(void) eof_track_add_section(eof_song, eof_selected_track, EOF_ARPEGGIO_SECTION, eof_note_type, arpeggstart, arpeggend, eof_song->pro_guitar_track[srctracknum]->arpeggio[lastarpeggnum].flags, NULL);
					}
				}
				arpeggstart = np->pos;	//Track start of new arpeggio
			}
			if(temp_note.phrasenum != 0xFFFFFFFF)
			{	//If this pasted note's source note is in an arpeggio/handshape
				arpeggend = np->pos + np->length;	//Track end position of arpeggio
			}
			lastarpeggnum = temp_note.phrasenum;
		}
	}//For each note in the clipboard file
	if(source_id == eof_log_id)
	{	//If the copy/paste is being performed within the same EOF instance
		if((eof_song->track[sourcetrack]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT) && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
		{	//If copying from a pro guitar track and pasting into a pro guitar track
			if(lastarpeggnum != 0xFFFFFFFF)
			{	//If an arpeggio phrase was in progress when the last of the notes were pasted, add the phrase now
				if(lastarpeggnum < eof_song->pro_guitar_track[srctracknum]->arpeggios)
				{	//Bounds check
					(void) eof_track_add_section(eof_song, eof_selected_track, EOF_ARPEGGIO_SECTION, eof_note_type, arpeggstart, arpeggend, eof_song->pro_guitar_track[srctracknum]->arpeggio[lastarpeggnum].flags, NULL);
				}
			}
		}
	}
	(void) pack_fclose(fp);
	eof_truncate_chart(eof_song);	//Update chart variables for any beats that were added to accommodate the paste
	eof_track_sort_notes(eof_song, eof_selected_track);
	eof_fixup_notes(eof_song);
	eof_determine_phrase_status(eof_song, eof_selected_track);
	(void) eof_detect_difficulties(eof_song, eof_selected_track);
	if((paste_count > 0) && (eof_selection.track != eof_selected_track))
	{
		eof_selection.track = eof_selected_track;
		memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
	}
	for(i = 0; i < paste_count; i++)
	{
		for(j = 0; j < eof_get_track_size(eof_song, eof_selected_track); j++)
		{	//For each note in the active track
			if((eof_get_note_type(eof_song, eof_selected_track, j) == eof_note_type) && (eof_get_note_pos(eof_song, eof_selected_track, j) == paste_pos[i]))
			{
				eof_selection.multi[j] = 1;	//Add the pasted note to the selection
				break;
			}
		}
	}
	return 1;
}

int eof_menu_edit_paste(void)
{
	return eof_menu_edit_paste_logic(0);	//Use new paste logic
}

int eof_menu_edit_old_paste(void)
{
	return eof_menu_edit_paste_logic(1);	//Use old paste logic
}

int eof_menu_edit_snap_quarter(void)
{
	eof_snap_mode = EOF_SNAP_QUARTER;
	return 1;
}

int eof_menu_edit_snap_eighth(void)
{
	eof_snap_mode = EOF_SNAP_EIGHTH;
	return 1;
}

int eof_menu_edit_snap_sixteenth(void)
{
	eof_snap_mode = EOF_SNAP_SIXTEENTH;
	return 1;
}

int eof_menu_edit_snap_thirty_second(void)
{
	eof_snap_mode = EOF_SNAP_THIRTY_SECOND;
	return 1;
}

int eof_menu_edit_snap_twelfth(void)
{
	eof_snap_mode = EOF_SNAP_TWELFTH;
	return 1;
}

int eof_menu_edit_snap_twenty_fourth(void)
{
	eof_snap_mode = EOF_SNAP_TWENTY_FOURTH;
	return 1;
}

int eof_menu_edit_snap_forty_eighth(void)
{
	eof_snap_mode = EOF_SNAP_FORTY_EIGHTH;
	return 1;
}

int eof_menu_edit_snap_sixty_forth(void)
{
	eof_snap_mode = EOF_SNAP_SIXTY_FORTH;
	return 1;
}

int eof_menu_edit_snap_ninty_sixth(void)
{
	eof_snap_mode = EOF_SNAP_NINTY_SIXTH;
	return 1;
}

int eof_custom_grid_snap_edit_proc(int msg, DIALOG *d, int c)
{
	//Check ASCII code input
	if(msg == MSG_CHAR)
	{
		unsigned c2 = (c & 255);	//The lower 8 bits is the scan code of the key press

		if((c2 == 'b') || (c2 == 'B'))
		{	//The user pressed b
			eof_custom_snap_dialog[3].flags = D_SELECTED;	//Select the beat radio button
			eof_custom_snap_dialog[4].flags = 0;			//Clear the measure radio button
			(void) object_message(&eof_custom_snap_dialog[3], MSG_DRAW, 0);		//Have Allegro redraw the radio buttons
			(void) object_message(&eof_custom_snap_dialog[4], MSG_DRAW, 0);
			return D_USED_CHAR;	//Input processed
		}
		else if((c2 == 'm') || (c2 == 'M'))
		{	//The user pressed m
			eof_custom_snap_dialog[4].flags = D_SELECTED;	//Select the measure radio button
			eof_custom_snap_dialog[3].flags = 0;			//Clear the beat radio button
			(void) object_message(&eof_custom_snap_dialog[3], MSG_DRAW, 0);		//Have Allegro redraw the radio buttons
			(void) object_message(&eof_custom_snap_dialog[4], MSG_DRAW, 0);
			return D_USED_CHAR;	//Input processed
		}
	}

	return eof_verified_edit_proc(msg, d, c);	//Allow the rest of the input to be filtered normally
}

int eof_menu_edit_snap_custom(void)
{
	int last_interval = eof_snap_interval;
	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_custom_snap_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_custom_snap_dialog);
	(void) snprintf(eof_etext2, sizeof(eof_etext2) - 1, "%d", eof_snap_interval);
	if(eof_custom_snap_measure == 0)
	{	//If the custom grid snap is per beats
		eof_custom_snap_dialog[3].flags = D_SELECTED;	//Activate "per beat" radio button by default
		eof_custom_snap_dialog[4].flags = 0;
	}
	else
	{
		eof_custom_snap_dialog[3].flags = 0;
		eof_custom_snap_dialog[4].flags = D_SELECTED;	//Activate "per measure" radio button by default
	}
	if(eof_popup_dialog(eof_custom_snap_dialog, 2) == 5)
	{	//User clicked OK
		eof_snap_interval = atoi(eof_etext2);

		if(eof_custom_snap_dialog[4].flags & D_SELECTED)	//If user selected per measure instead of per beat
		{
			eof_custom_snap_measure = 1;
		}
		else
		{
			eof_custom_snap_measure = 0;
		}

		if((eof_snap_interval >= EOF_MAX_GRID_SNAP_INTERVALS) || (eof_snap_interval < 1))
		{
			eof_snap_interval = last_interval;
			eof_render();
			allegro_message("Invalid snap setting, must be between 1 and %d", EOF_MAX_GRID_SNAP_INTERVALS - 1);
		}
		else
		{
			eof_snap_mode = EOF_SNAP_CUSTOM;
		}
		eof_fixup_notes(eof_song);	//Run the fixup logic for all tracks, so that if the "Highlight non grid snapped notes" feature is in use, the highlighting can take the custom grid snap into account
	}
	printf("%d\n", eof_snap_interval);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(NULL);
	return 1;
}

int eof_menu_edit_toggle_grid_lines(void)
{
	eof_render_grid_lines ^= 1;	//Toggle this boolean variable
	return 1;
}

int eof_menu_edit_zoom_helper_in(void)
{
	if(eof_zoom > EOF_NUM_ZOOM_LEVELS)
	{	//If the current zoom level is user defined
		return eof_menu_edit_zoom_level(EOF_NUM_ZOOM_LEVELS);	//Zoom in to the highest preset zoom level
	}

	//Otherwise zoom in normally
	return eof_menu_edit_zoom_level(eof_zoom - 1);
}

int eof_menu_edit_zoom_helper_out(void)
{
	return eof_menu_edit_zoom_level(eof_zoom + 1);
}

int eof_menu_edit_zoom_10(void)
{
	return eof_menu_edit_zoom_level(10);
}

int eof_menu_edit_zoom_9(void)
{
	return eof_menu_edit_zoom_level(9);
}

int eof_menu_edit_zoom_8(void)
{
	return eof_menu_edit_zoom_level(8);
}

int eof_menu_edit_zoom_7(void)
{
	return eof_menu_edit_zoom_level(7);
}

int eof_menu_edit_zoom_6(void)
{
	return eof_menu_edit_zoom_level(6);
}

int eof_menu_edit_zoom_5(void)
{
	return eof_menu_edit_zoom_level(5);
}

int eof_menu_edit_zoom_4(void)
{
	return eof_menu_edit_zoom_level(4);
}

int eof_menu_edit_zoom_3(void)
{
	return eof_menu_edit_zoom_level(3);
}

int eof_menu_edit_zoom_2(void)
{
	return eof_menu_edit_zoom_level(2);
}

int eof_menu_edit_zoom_1(void)
{
	return eof_menu_edit_zoom_level(1);
}

int eof_menu_edit_zoom_level(int zoom)
{
	if(zoom > 0)
	{	//If the zoom level is valid
		if(zoom <= EOF_NUM_ZOOM_LEVELS)
		{	//If the zoom level is one of the presets
			eof_zoom = zoom;
		}
		else if(eof_custom_zoom_level)
		{	//If the user had defined a custom zoom level, allow zoom out to change to this zoom level
			eof_zoom = eof_custom_zoom_level;
		}
	}

	return 1;
}

int eof_menu_edit_zoom_custom(void)
{
	int userinput;

	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_custom_zoom_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_custom_zoom_dialog);
	(void) ustrcpy(eof_etext2, "");
	if(eof_popup_dialog(eof_custom_zoom_dialog, 2) == 3)
	{	//User clicked OK
		userinput = atoi(eof_etext2);

		if(userinput > 0)
		{	//If it's a valid number
			eof_custom_zoom_level = userinput;	//Store this so user can cycle through the presets and the user-defined zoom levels
			eof_zoom = userinput;
		}
	}
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(NULL);

	return 1;
}

int eof_menu_edit_playback_speed_helper_faster(void)
{
	int i, amount;

	if(eof_music_catalog_playback)
		return 1;	//Don't allow speed changes during catalog playback

	for(i = 0; i < 5; i++)
	{
		eof_edit_playback_menu[i + 1].flags = 0;
	}
	if(KEY_EITHER_CTRL)
	{	//If CTRL is held, change the playback speed by 1%
		amount = 10;
	}
	else
	{
		if((eof_input_mode == EOF_INPUT_FEEDBACK) || (!eof_music_paused))
		{	//In Feedback input mode, or the chart is playing back, cycle playback speed in intervals of 10%
			amount = 100;
		}
		else
		{
			amount = 250;	//Otherwise do so in intervals of 25%
		}
	}
	eof_playback_speed = (eof_playback_speed /amount)*amount;	//Account for custom playback rate (force to round down to a multiple of the change interval)
	eof_playback_speed += amount;
	if(eof_playback_speed > 1000)
	{
		if(eof_input_mode == EOF_INPUT_FEEDBACK)
		{	//In Feedback input mode
			eof_playback_speed = amount;	//Wrap around
		}
		else
		{
			eof_playback_speed = 1000;
		}
	}
	if(eof_playback_speed % 250 == 0)
	{	//If one of the 25% preset intervals is set
		eof_edit_playback_menu[(1000 - eof_playback_speed) / 250 + 1].flags = D_SELECTED;	//Check the appropriate speed menu item
	}
	else
	{
		eof_edit_playback_menu[5].flags = D_SELECTED;	//Check the "custom" playback speed menu item
	}
	return 1;
}

int eof_menu_edit_playback_speed_helper_slower(void)
{
	int i, amount;

	if(eof_music_catalog_playback)
		return 1;	//Don't allow speed changes during catalog playback

	for(i = 0; i < 5; i++)
	{
		eof_edit_playback_menu[i + 1].flags = 0;
	}
	if(KEY_EITHER_CTRL)
	{	//If CTRL is held, change the playback speed by 1%
		amount = 10;
	}
	else
	{
		if((eof_input_mode == EOF_INPUT_FEEDBACK) || (!eof_music_paused))
		{	//In Feedback input mode, or the chart is playing back, cycle playback speed in intervals of 10%
			amount = 100;
		}
		else
		{
			amount = 250;	//Otherwise do so in intervals of 25%
		}
	}
	eof_playback_speed = (eof_playback_speed /amount)*amount;	//Account for custom playback rate (force to round down to a multiple of the change interval)
	eof_playback_speed -= amount;
	if(eof_playback_speed < amount)
	{
		if(eof_input_mode == EOF_INPUT_FEEDBACK)
		{	//In Feedback input mode
			eof_playback_speed = 1000;	//Wrap around
		}
		else
		{
			eof_playback_speed = 250;
		}
	}
	if(eof_playback_speed % 250 == 0)
	{	//If one of the 25% preset intervals is set
		eof_edit_playback_menu[(1000 - eof_playback_speed) / 250 + 1].flags = D_SELECTED;	//Check the appropriate speed menu item
	}
	else
	{
		eof_edit_playback_menu[5].flags = D_SELECTED;	//Check the "custom" playback speed menu item
	}
	return 1;
}

int eof_menu_edit_playback_time_stretch(void)
{
	eof_playback_time_stretch = 1 - eof_playback_time_stretch;
	return 1;
}

int eof_menu_edit_playback_100(void)
{
	int i;

	for(i = 0; i < 5; i++)
	{
		eof_edit_playback_menu[i + 1].flags = 0;
	}
	eof_edit_playback_menu[1].flags = D_SELECTED;
	eof_playback_speed = 1000;
	return 1;
}

int eof_menu_edit_playback_75(void)
{
	int i;

	for(i = 0; i < 5; i++)
	{
		eof_edit_playback_menu[i + 1].flags = 0;
	}
	eof_edit_playback_menu[2].flags = D_SELECTED;
	eof_playback_speed = 750;
	return 1;
}

int eof_menu_edit_playback_50(void)
{
	int i;

	for(i = 0; i < 5; i++)
	{
		eof_edit_playback_menu[i + 1].flags = 0;
	}
	eof_edit_playback_menu[3].flags = D_SELECTED;
	eof_playback_speed = 500;
	return 1;
}

int eof_menu_edit_playback_25(void)
{
	int i;

	for(i = 0; i < 5; i++)
	{
		eof_edit_playback_menu[i + 1].flags = 0;
	}
	eof_edit_playback_menu[4].flags = D_SELECTED;
	eof_playback_speed = 250;
	return 1;
}

int eof_menu_edit_playback_custom(void)
{
	int i;
	int userinput=0;

	for(i = 0; i < 5; i++)
	{
		eof_edit_playback_menu[i + 1].flags = 0;
	}
	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_custom_speed_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_custom_speed_dialog);
	(void) snprintf(eof_etext2, sizeof(eof_etext2) - 1, "%d", eof_playback_speed/10);		//Load the current playback speed into a string
	if(eof_popup_dialog(eof_custom_speed_dialog, 2) == 3)		//If user activated "OK" from the custom speed dialog
	{
		userinput = atoi(eof_etext2);
//		if((userinput < 1) || (userinput > 99))			//User cannot specify to play at any speed not between 1% and 99%
//			return 1;

		eof_playback_speed = userinput * 10;
	}
	printf("%d\n", eof_playback_speed);
	eof_edit_playback_menu[5].flags = D_SELECTED;
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(NULL);
	return 1;
}

int eof_menu_edit_speed_number(int value)
{
	if((value < EOF_ZOOM_3D_MIN) || (value > EOF_ZOOM_3D_MAX))
		return 0;	//Invalid parameter

	if(value == 4)	//EOF isn't currently using a zoom 3d value of 4
		value = 3;

	eof_zoom_3d = value;
	return 1;
}

int eof_menu_edit_speed_slow(void)
{
	return eof_menu_edit_speed_number(5);
}

int eof_menu_edit_speed_medium(void)
{
	return eof_menu_edit_speed_number(3);
}

int eof_menu_edit_speed_fast(void)
{
	return eof_menu_edit_speed_number(2);
}

int eof_menu_edit_speed_hyper(void)
{
	return eof_menu_edit_speed_number(1);
}

int eof_menu_edit_snap_off(void)
{
	if(eof_snap_mode == EOF_SNAP_OFF)
	{	//Toggle grid snap on and restore the last snap setting in use
		eof_snap_mode = eof_last_snap_mode;
	}
	else
	{	//Store the grid snap setting in use and toggle grid snap off
		eof_last_snap_mode = eof_snap_mode;
		eof_snap_mode = EOF_SNAP_OFF;
	}
	return 1;
}

int eof_menu_edit_hopo_rf(void)
{
	return eof_menu_edit_hopo_helper(EOF_HOPO_RF);
}

int eof_menu_edit_hopo_fof(void)
{
	return eof_menu_edit_hopo_helper(EOF_HOPO_FOF);
}

int eof_menu_edit_hopo_off(void)
{
	return eof_menu_edit_hopo_helper(EOF_HOPO_OFF);
}

int eof_menu_edit_hopo_manual(void)
{
	return eof_menu_edit_hopo_helper(EOF_HOPO_MANUAL);
}

int eof_menu_edit_hopo_gh3(void)
{
	return eof_menu_edit_hopo_helper(EOF_HOPO_GH3);
}

int eof_menu_edit_hopo_helper(int hopo_view)
{
	int i;
	if(hopo_view < EOF_NUM_HOPO_MODES)
	{
		for(i = 0; i < EOF_NUM_HOPO_MODES; i++)
		{
			eof_edit_hopo_menu[i].flags = 0;
		}
		eof_edit_hopo_menu[hopo_view].flags = D_SELECTED;
		eof_hopo_view = hopo_view;
		eof_determine_phrase_status(eof_song, eof_selected_track);
	}
	return 1;
}

int eof_menu_edit_metronome(void)
{
	if(eof_mix_metronome_enabled)
	{
		eof_mix_metronome_enabled = 0;
		eof_edit_menu[14].flags = 0;
	}
	else
	{
		eof_mix_metronome_enabled = 1;
		eof_edit_menu[14].flags = D_SELECTED;
	}
	return 1;
}

int eof_menu_edit_claps_all(void)
{
	return eof_menu_edit_claps_helper(0,63);
}

int eof_menu_edit_claps_green(void)
{
	return eof_menu_edit_claps_helper(1,1);
}

int eof_menu_edit_claps_red(void)
{
	return eof_menu_edit_claps_helper(2,2);
}

int eof_menu_edit_claps_yellow(void)
{
	return eof_menu_edit_claps_helper(3,4);
}

int eof_menu_edit_claps_blue(void)
{
	return eof_menu_edit_claps_helper(4,8);
}

int eof_menu_edit_claps_purple(void)
{
	return eof_menu_edit_claps_helper(5,16);
}

int eof_menu_edit_claps_orange(void)
{
	return eof_menu_edit_claps_helper(6,32);
}

int eof_menu_edit_claps_helper(unsigned long menu_item, char claps_flag)
{
	int i;
	for(i = 0; i < 7; i++)
	{
		eof_edit_claps_menu[i].flags = 0;
	}
	eof_edit_claps_menu[menu_item].flags = D_SELECTED;
	eof_mix_claps_note = claps_flag;
	return 1;
}

int eof_menu_edit_claps(void)
{
	if(eof_mix_claps_enabled)
	{
		eof_mix_claps_enabled = 0;
		eof_edit_menu[15].flags = 0;
	}
	else
	{
		eof_mix_claps_enabled = 1;
		eof_edit_menu[15].flags = D_SELECTED;
	}
	return 1;
}

int eof_menu_edit_vocal_tones(void)
{
	if(eof_mix_vocal_tones_enabled)
	{
		eof_mix_vocal_tones_enabled = 0;
		eof_mix_percussion_enabled = 0;
		eof_edit_menu[17].flags = 0;
	}
	else
	{
		eof_mix_vocal_tones_enabled = 1;
		eof_mix_percussion_enabled = 1;
		eof_edit_menu[17].flags = D_SELECTED;
	}
	return 1;
}

int eof_menu_edit_midi_tones(void)
{
	if(eof_mix_midi_tones_enabled)
	{
		eof_mix_midi_tones_enabled = 0;
		eof_edit_menu[18].flags = 0;
	}
	else
	{
		eof_mix_midi_tones_enabled = 1;
		eof_edit_menu[18].flags = D_SELECTED;
	}
	return 1;
}

int eof_menu_edit_bookmark_helper(int b)
{
	if(b >= EOF_MAX_BOOKMARK_ENTRIES)
		return 0;	//Invalid parameter

	if(eof_music_pos <= eof_av_delay)
		return 1;	//Do not place a bookmark at a negative or zero chart position

	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	if(eof_song->bookmark_pos[b] != eof_music_pos - eof_av_delay)
	{
		eof_song->bookmark_pos[b] = eof_music_pos - eof_av_delay;
	}
	else
	{
		eof_song->bookmark_pos[b] = 0;
	}
	return 1;
}

int eof_menu_edit_bookmark_0(void)
{
	return eof_menu_edit_bookmark_helper(0);
}

int eof_menu_edit_bookmark_1(void)
{
	return eof_menu_edit_bookmark_helper(1);
}

int eof_menu_edit_bookmark_2(void)
{
	return eof_menu_edit_bookmark_helper(2);
}

int eof_menu_edit_bookmark_3(void)
{
	return eof_menu_edit_bookmark_helper(3);
}

int eof_menu_edit_bookmark_4(void)
{
	return eof_menu_edit_bookmark_helper(4);
}

int eof_menu_edit_bookmark_5(void)
{
	return eof_menu_edit_bookmark_helper(5);
}

int eof_menu_edit_bookmark_6(void)
{
	return eof_menu_edit_bookmark_helper(6);
}

int eof_menu_edit_bookmark_7(void)
{
	return eof_menu_edit_bookmark_helper(7);
}

int eof_menu_edit_bookmark_8(void)
{
	return eof_menu_edit_bookmark_helper(8);
}

int eof_menu_edit_bookmark_9(void)
{
	return eof_menu_edit_bookmark_helper(9);
}

int eof_menu_edit_select_all(void)
{
	unsigned long i;

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{
		if(eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type)
		{
			eof_selection.track = eof_selected_track;
			eof_selection.multi[i] = 1;
		}
		else
		{
			eof_selection.multi[i] = 0;
		}
	}
	return 1;
}

int eof_menu_edit_select_like_function(char thorough)
{
	unsigned long i, j, ntypes = 0;
	unsigned long ntype[100] = {0};	//This tracks each unique selected note to allow multiple dislike notes to be selected during a "select like" operation
	int note_selection_updated = eof_feedback_mode_update_note_selection();	//If no notes are selected, select the seek hover note if Feedback input mode is in effect

	if(eof_selection.track != eof_selected_track)
	{
		return 1;
	}
	if(eof_selection.current >= eof_get_track_size(eof_song, eof_selected_track))
	{
		return 1;
	}
	//Make a list of all the unique selected notes
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i] || (eof_get_note_type(eof_song, eof_selected_track, i) != eof_note_type))
			continue;	//If the note isn't selected or in the active track difficulty, skip it

		for(j = 0; j < ntypes; j++)
		{	//For each unique note number in the ntype array
			if(eof_note_compare_simple(eof_song, eof_selected_track, ntype[j], i) == 0)
			{	//If the stored unique note mask is the same as this note mask
				if(!thorough || (eof_get_note_ghost(eof_song, eof_selected_track, ntype[j]) == eof_get_note_ghost(eof_song, eof_selected_track, i)))
				{	//If the option to compare ghost flags was not chosen, or the two notes' ghost bitmasks also match
					break;	//Break loop to reject this notes from the unique note list
				}
			}
		}
		if(j == ntypes)
		{	//If no match was found
			if(ntypes < 100)
			{	//If the limit hasn't been reached
				ntype[ntypes] = i;	//Append this note's number to the ntype array
				ntypes++;
			}
		}
	}
	memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		for(j = 0; j < ntypes; j++)
		{	//For each note bitmask in the ntype array
			if((eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type) && (eof_note_compare_simple(eof_song, eof_selected_track, ntype[j], i) == 0))
			{	//If the note is in the active difficulty and matches one of the unique notes that are selected
				if(!thorough || ((eof_get_note_flags(eof_song, eof_selected_track, ntype[j]) == eof_get_note_flags(eof_song, eof_selected_track, i)) && (eof_get_note_eflags(eof_song, eof_selected_track, ntype[j]) == eof_get_note_eflags(eof_song, eof_selected_track, i)) && (eof_get_note_ghost(eof_song, eof_selected_track, ntype[j]) == eof_get_note_ghost(eof_song, eof_selected_track, i))))
				{	//If the option to compare note flags was not chosen, or if the extended flags, common flags and ghost bitmasks do match
					eof_selection.track = eof_selected_track;	//Change the selection's track to the active track
					eof_selection.multi[i] = 1;					//Mark the note as selected
				}
			}
		}
	}
	if(note_selection_updated)
	{	//If the only note modified was the seek hover note
		eof_selection.multi[eof_seek_hover_note] = 0;	//Deselect it to restore the note selection's original condition
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	return 1;
}

int eof_menu_edit_select_like(void)
{
	return eof_menu_edit_select_like_function(0);	//Perform select like logic, without comparing note flags
}

int eof_menu_edit_precise_select_like(void)
{
	return eof_menu_edit_select_like_function(1);	//Perform select like logic, comparing note flags
}

int eof_menu_edit_deselect_all(void)
{
	eof_update_seek_selection(0, 0, 1);	//Clear the seek selection and the selected notes array, do not reselect a note at position 0
	eof_selection.current = EOF_MAX_NOTES - 1;
	eof_selection.current_pos = 0;
	eof_selection.range_pos_1 = 0;
	eof_selection.range_pos_2 = 0;
	return 1;
}

int eof_menu_edit_select_rest(void)
{
	unsigned long i;

	if(eof_count_selected_notes(NULL) == 0)
	{
		return 1;
	}
	if(eof_selection.current == EOF_MAX_NOTES - 1)	//No notes selected?
		return 1;	//Don't perform this operation

	for(i = eof_selection.current; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if(eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type)
		{
			eof_selection.multi[i] = 1;
		}
	}

	return 1;
}

DIALOG eof_menu_edit_select_by_note_length_dialog[] =
{
	/* (proc)                (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                          (dp2) (dp3) */
	{ d_agup_window_proc,    0,   0,   300, 148, 0,   0,   0,    0,      0,   0,   eof_etext, NULL, NULL },
	{ d_agup_text_proc,      12,  40,  60,  12,  0,   0,   0,    0,      0,   0,   "This # of ms:",NULL, NULL },
	{ d_agup_text_proc,      12,  56,  60,  12,  0,   0,   0,    0,      0,   0,   "(specify 0 for one grid snap length)",NULL, NULL },
	{ eof_verified_edit_proc,12,  74,  90,  20,  0,   0,   0,    0,      7,   0,   eof_etext2,     "0123456789",  NULL },
	{ d_agup_button_proc,    12,  108, 84,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",                         NULL, NULL },
	{ d_agup_button_proc,    110, 108, 78,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",                     NULL, NULL },
	{ NULL,                  0,   0,   0,   0,   0,   0,   0,    0,      0,   0,   NULL,                         NULL, NULL }
};

int eof_menu_edit_select_by_note_length_logic(int (*check)(long, long), int function)
{
	unsigned long i;
	long threshold;

	if(!function && (eof_selection.track != eof_selected_track))
		return 1;	//No notes in the active track are selected so none can become deselected

	if(eof_snap_mode == EOF_SNAP_OFF)
	{	//If no grid snap is active
		eof_menu_edit_select_by_note_length_dialog[2].flags = D_HIDDEN;	//Hide the text regarding selecting by grid snap
	}
	else
	{
		eof_menu_edit_select_by_note_length_dialog[2].flags = 0;	//Show the text regarding selecting by grid snap
	}
	eof_etext2[0] = '\0';	//Empty the dialog's input string
	eof_color_dialog(eof_menu_edit_select_by_note_length_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_menu_edit_select_by_note_length_dialog);

	if(eof_popup_dialog(eof_menu_edit_select_by_note_length_dialog, 3) != 4)
		return 1;	//If the user did not click OK, return immediately
	if(eof_etext2[0] == '\0')
		return 1;	//If the user did not enter a threshold, return immediately

	threshold = atol(eof_etext2);
	if(!threshold && (eof_snap_mode == EOF_SNAP_OFF))
	{	//If the user specified a threshold of one grid snap, but no grid snap length is active
		allegro_message("Grid snap must be enabled in order to select/deselect notes longer/shorter than one grid snap length.");
		return 1;
	}
	if(threshold < 0)
		return 1;	//If the specified value is not valid, return immediately

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		long effective_threshold = threshold;

		if(!threshold)
		{	//The threshold is one grid snap length
			EOF_SNAP_DATA temp = {0, 0.0, 0, 0.0, 0, 0, 0, 0.0, {0.0}, {0.0}, 0, 0, 0, 0};

			eof_snap_logic(&temp, eof_get_note_pos(eof_song, eof_selected_track, i));	//Calculate grid snap data about the note's position
			eof_snap_length_logic(&temp);
       		if(temp.length <= 0)
				continue;	//If the grid snap length could not be determined, skip this note
			effective_threshold = temp.length;	//One grid snap is the length to compare against the note's length
		}
		if((eof_get_note_type(eof_song, eof_selected_track, i) != eof_note_type) || !(check(eof_get_note_length(eof_song, eof_selected_track, i), effective_threshold)))
			continue;	//If the note isn't in the active difficulty or doesn't match the conditions the user specified, skip it

		if(!function)
		{	//Perform deselection
			eof_selection.multi[i] = 0;
		}
		else
		{	//Perform selection
			if(eof_selection.track != eof_selected_track)
			{	//If no notes in the current track are currently selected
				(void) eof_menu_edit_deselect_all();		//Clear the selection variables if necessary
				eof_selection.track = eof_selected_track;
			}
			eof_selection.multi[i] = 1;
		}
	}
	if(eof_selection.current != EOF_MAX_NOTES - 1)
	{	//If there was a last selected note
		if(eof_selection.multi[eof_selection.current] == 0)
		{	//And it's not selected anymore
			eof_selection.current = EOF_MAX_NOTES - 1;	//Clear the selected note
		}
	}

	return 1;
}

int eof_menu_edit_select_all_shorter_than(void)
{
	strncpy(eof_etext, "Select all notes shorter than", sizeof(eof_etext) - 1);
	eof_etext2[0] = '\0';	//Empty the input field
	return eof_menu_edit_select_by_note_length_logic(eof_length_is_shorter_than, 1);
}

int eof_menu_edit_select_all_longer_than(void)
{
	strncpy(eof_etext, "Select all notes longer than", sizeof(eof_etext) - 1);
	eof_etext2[0] = '\0';	//Empty the input field
	return eof_menu_edit_select_by_note_length_logic(eof_length_is_longer_than, 1);
}

int eof_menu_edit_select_all_of_length(void)
{
	strncpy(eof_etext, "Select notes with a length of", sizeof(eof_etext) - 1);
	eof_etext2[0] = '\0';	//Empty the input field
	return eof_menu_edit_select_by_note_length_logic(eof_length_is_equal_to, 1);
}

int eof_menu_edit_deselect_all_shorter_than(void)
{
	strncpy(eof_etext, "Deselect all notes shorter than", sizeof(eof_etext) - 1);
	eof_etext2[0] = '\0';	//Empty the input field
	return eof_menu_edit_select_by_note_length_logic(eof_length_is_shorter_than, 0);
}

int eof_menu_edit_deselect_all_longer_than(void)
{
	strncpy(eof_etext, "Deselect all notes longer than", sizeof(eof_etext) - 1);
	eof_etext2[0] = '\0';	//Empty the input field
	return eof_menu_edit_select_by_note_length_logic(eof_length_is_longer_than, 0);
}

int eof_menu_edit_deselect_all_of_length(void)
{
	strncpy(eof_etext, "Deselect notes with a length of", sizeof(eof_etext) - 1);
	eof_etext2[0] = '\0';	//Empty the input field
	return eof_menu_edit_select_by_note_length_logic(eof_length_is_equal_to, 0);
}

DIALOG eof_menu_edit_select_less_than_threshold_dialog[] =
{
	/* (proc)                (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                          (dp2) (dp3) */
	{ d_agup_window_proc,    0,   0,   350, 148, 0,   0,   0,    0,      0,   0,   eof_etext, NULL, NULL },
	{ d_agup_text_proc,      12,  40,  60,  12,  0,   0,   0,    0,      0,   0,   "This # of ms:",NULL, NULL },
	{ d_agup_text_proc,      12,  56,  60,  12,  0,   0,   0,    0,      0,   0,   "(specify 0 for one grid snap length)",NULL, NULL },
	{ eof_verified_edit_proc,12,  76,  90,  20,  0,   0,   0,    0,      7,   0,   eof_etext2,     "0123456789",  NULL },
	{ d_agup_button_proc,    12,  108, 84,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",                         NULL, NULL },
	{ d_agup_button_proc,    110, 108, 78,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",                     NULL, NULL },
	{ NULL,                  0,   0,   0,   0,   0,   0,   0,    0,      0,   0,   NULL,                         NULL, NULL }
};

int eof_menu_edit_select_note_within_threshhold_of_next_note_logic(int function)
{
	unsigned long i;
	long threshold;

	if(!function && (eof_selection.track != eof_selected_track))
		return 1;	//No notes in the active track are selected so none can become deselected

	if(eof_snap_mode == EOF_SNAP_OFF)
	{	//If no grid snap is active
		eof_menu_edit_select_less_than_threshold_dialog[2].flags = D_HIDDEN;	//Hide the text regarding selecting by grid snap
	}
	else
	{
		eof_menu_edit_select_less_than_threshold_dialog[2].flags = 0;			//Show the text regarding selecting by grid snap
	}
	eof_etext2[0] = '\0';	//Empty the dialog's input string
	eof_color_dialog(eof_menu_edit_select_less_than_threshold_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_menu_edit_select_less_than_threshold_dialog);

	if(eof_popup_dialog(eof_menu_edit_select_less_than_threshold_dialog, 3) != 4)
		return 1;	//If the user did not click OK, return immediately
	if(eof_etext2[0] == '\0')
		return 1;	//If the user did not enter a threshold, return immediately

	threshold = atol(eof_etext2);
	if(threshold < 0)
		return 1;	//If the specified value is not valid, return immediately
	if(!threshold && (eof_snap_mode == EOF_SNAP_OFF))
	{	//If the user specified a threshold of one grid snap, but no grid snap length is active
		allegro_message("Grid snap must be enabled in order to select/deselect notes less than one grid snap length away from their next neighbor.");
		return 1;
	}

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		long nextnote;
		unsigned long notepos, nextpos;

		if(eof_get_note_type(eof_song, eof_selected_track, i) != eof_note_type)
			continue;	//If the note isn't in the active difficulty, skip it
		nextnote = eof_track_fixup_next_note(eof_song, eof_selected_track, i);	//Find the next note in the track difficulty
		if(nextnote < 0)
			break;		//If there is no note that follows this one, stop checking notes for this track difficulty

		notepos = eof_get_note_pos(eof_song, eof_selected_track, i);
		nextpos = eof_get_note_pos(eof_song, eof_selected_track, nextnote);
		if(threshold)
		{	//The threshold is a static number of milliseconds
			if(nextpos - notepos >= threshold)
				continue;	//If the next note's position exceeds the threshold proximity, skip this note
		}
		else
		{	//The threshold is one grid snap length
			EOF_SNAP_DATA temp = {0, 0.0, 0, 0.0, 0, 0, 0, 0.0, {0.0}, {0.0}, 0, 0, 0, 0};

			eof_snap_logic(&temp, notepos);	//Calculate grid snap data about the note's position
			eof_snap_length_logic(&temp);
            if(temp.length <= 0)
				continue;	//If the grid snap length could not be determined, skip this note
			if(notepos + temp.length <= nextpos)
				continue;	//If the next note's position exceeds the threshold proximity, skip this note
		}

		if(!function)
		{	//Perform deselection
			eof_selection.multi[i] = 0;
		}
		else
		{	//Perform selection
			if(eof_selection.track != eof_selected_track)
			{	//If no notes in the current track are currently selected
				(void) eof_menu_edit_deselect_all();		//Clear the selection variables if necessary
				eof_selection.track = eof_selected_track;
			}
			eof_selection.multi[i] = 1;
		}
	}
	if(eof_selection.current != EOF_MAX_NOTES - 1)
	{	//If there was a last selected note
		if(eof_selection.multi[eof_selection.current] == 0)
		{	//And it's not selected anymore
			eof_selection.current = EOF_MAX_NOTES - 1;	//Clear the selected note
		}
	}

	return 1;
}

int eof_menu_edit_select_note_within_threshhold_of_next_note(void)
{
	strncpy(eof_etext, "Select all notes preceding their neighbor by less than", sizeof(eof_etext) - 1);
	eof_etext2[0] = '\0';	//Empty the input field
	return eof_menu_edit_select_note_within_threshhold_of_next_note_logic(1);
}

int eof_menu_edit_deselect_note_within_threshhold_of_next_note(void)
{

	strncpy(eof_etext, "Deselect all notes preceding their neighbor by less than", sizeof(eof_etext) - 1);
	eof_etext2[0] = '\0';	//Empty the input field
	return eof_menu_edit_select_note_within_threshhold_of_next_note_logic(0);
}

char eof_menu_edit_conditional_selection_dialog_string[25];
DIALOG eof_menu_edit_conditional_selection_dialog[] =
{
	/*	(proc)           (x)  (y)  (w)  (h) (fg) (bg) (key) (flags)     (d1) (d2) (dp)                   (dp2) (dp3) */
	{d_agup_window_proc, 0,   0,   376, 222,2,   23,  0,    0,          0,   0,   eof_menu_edit_conditional_selection_dialog_string, NULL, NULL },
	{d_agup_radio_proc,	 16,  32,  38,  16, 2,   23,  0,    D_SELECTED, 1,   0,   "Do",                   NULL, NULL },
	{d_agup_radio_proc,	 72,  32,  62,  16, 2,   23,  0,    0,          1,   0,   "Do not",               NULL, NULL },
	{d_agup_radio_proc,	 16,  52,  116, 16, 2,   23,  0,    D_SELECTED, 2,   0,   "Contain any of",       NULL, NULL },
	{d_agup_radio_proc,	 134, 52,  108, 16, 2,   23,  0,    0,          2,   0,   "Contain all of",       NULL, NULL },
	{d_agup_radio_proc,	 244, 52,  116, 16, 2,   23,  0,    0,          2,   0,   "Contain exactly",      NULL, NULL },
	{d_agup_text_proc,   16,  72,  64,  8,  2,   23,  0,    0,          0,   0,   "These gems:",          NULL, NULL },
	{d_agup_check_proc,	 16,  92,  64,  16, 2,   23,  0,    0,          0,   0,   "Lane 1",               NULL, NULL },
	{d_agup_check_proc,	 80,  92,  64,  16, 2,   23,  0,    0,          0,   0,   "Lane 2",               NULL, NULL },
	{d_agup_check_proc,	 144, 92,  64,  16, 2,   23,  0,    0,          0,   0,   "Lane 3",               NULL, NULL },
	{d_agup_check_proc,	 16,  112, 64,  16, 2,   23,  0,    0,          0,   0,   "Lane 4",               NULL, NULL },
	{d_agup_check_proc,	 80,  112, 64,  16, 2,   23,  0,    0,          0,   0,   "Lane 5",               NULL, NULL },
	{d_agup_check_proc,	 144, 112, 64,  16, 2,   23,  0,    0,          0,   0,   "Lane 6",               NULL, NULL },
	{d_agup_text_proc,   16,  132, 44,  8,  2,   23,  0,    0,          0,   0,   "These cymbals:",       NULL, NULL },
	{d_agup_check_proc,	 110, 132, 64,  16, 2,   23,  0,    0,          3,   0,   "Lane 3",               NULL, NULL },
	{d_agup_check_proc,	 174, 132, 64,  16, 2,   23,  0,    0,          3,   0,   "Lane 4",               NULL, NULL },
	{d_agup_check_proc,	 238, 132, 64,  16, 2,   23,  0,    0,          3,   0,   "Lane 5",               NULL, NULL },
	{d_agup_text_proc,   16,  152, 44,  8,  2,   23,  0,    0,          0,   0,   "Of type:",             NULL, NULL },
	{d_agup_check_proc,	 70,  152, 70,  16, 2,   23,  0,    D_SELECTED, 0,   0,   "Normal",               NULL, NULL },
	{d_agup_check_proc,	 140, 152, 54,  16, 2,   23,  0,    D_SELECTED, 0,   0,   "Mute",                 NULL, NULL },
	{d_agup_check_proc,	 200, 152, 58,  16, 2,   23,  0,    D_SELECTED, 0,   0,   "Ghost",                NULL, NULL },
	{d_agup_button_proc, 12,  182, 68,  28, 2,   23,  '\r', D_EXIT,     0,   0,   "OK",                   NULL, NULL },
	{d_agup_button_proc, 296, 182, 68,  28, 2,   23,  0,    D_EXIT,     0,   0,   "Cancel",               NULL, NULL },
	{NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

int eof_check_note_conditional_selection(EOF_SONG *sp, unsigned long track, unsigned long notenum, unsigned long match_bitmask, unsigned long cymbal_match_bitmask)
{
	int match = 0;	//This will be set to nonzero if the user's criteria apply to the note, and will ultimately be checked against the "Do" or "Do not" criterion
	unsigned long ctr, bitmask;
	unsigned char note;			//The non cymbal gems contained by this note
	unsigned char cymbal = 0;	//The cymbal gems contained by this note

	if(!sp || (track >= sp->tracks) || !track)
		return 0;	//Invalid parameters

	note = eof_get_note_note(sp, track, notenum);
	if(sp->track[track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
	{	//If a drum track is active
		(void) eof_get_drum_note_masks(sp, track, notenum, &note, &cymbal);	//Get a bitmask each to define which cymbal gems and non-cymbal gems the note contains
	}
	else if(sp->track[track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
	{	//If the specified track is a pro guitar track
		EOF_PRO_GUITAR_NOTE *np = sp->pro_guitar_track[sp->track[track]->tracknum]->note[notenum];

		for(ctr = 0, bitmask = 1; ctr < 6; ctr++, bitmask <<= 1)
		{	//For each of the 6 usable strings
			int matched = 0;

			if(!(match_bitmask & bitmask) || !(note & bitmask))
				continue;	//If this string is not one of those the user included in the criteria, or isn't a string used in this note, skip it

			if(eof_menu_edit_conditional_selection_dialog[18].flags == D_SELECTED)
			{	//Criteria include normal note gems and this note has a gem on that string
				if(!(np->frets[ctr] & 0x80) && !(np->ghost & bitmask))
				{	//If this gem is not muted or ghosted
					matched = 1;
				}
			}
			if(eof_menu_edit_conditional_selection_dialog[19].flags == D_SELECTED)
			{	//Criteria include muted note gems
				if(np->frets[ctr] & 0x80)
				{	//If this gem is muted
					matched = 1;
				}
			}
			if(eof_menu_edit_conditional_selection_dialog[20].flags == D_SELECTED)
			{	//Criteria include ghosted note gems
				if(np->ghost & bitmask)
				{	//If this gem is ghosted
					matched = 1;
				}
			}

			if(!matched)
			{	//If this gem did not meet any of those 3 criteria
				note &= ~bitmask;	//Clear it from the effective note bitmask to indicate this gem isn't a match
			}
		}
	}

	if(eof_menu_edit_conditional_selection_dialog[3].flags == D_SELECTED)
	{	//Contain any of
		if((note & match_bitmask) || (cymbal & cymbal_match_bitmask))
		{	//If the selected note contains any of the specified gems (or cymbals)
			match = 1;
		}
	}
	else if(eof_menu_edit_conditional_selection_dialog[4].flags == D_SELECTED)
	{	//Contain all of
		if(((note & match_bitmask) == match_bitmask) && ((cymbal & cymbal_match_bitmask) == match_bitmask))
		{	//If the selected note contains all specified gems (and cymbals)
			match = 1;
		}
	}
	else
	{	//Contain exactly
		if((note == match_bitmask) && (cymbal == cymbal_match_bitmask))
		{	//If the selected note contains exactly the specified gems (and cymbals)
			match = 1;
		}
	}

	if(eof_menu_edit_conditional_selection_dialog[1].flags == D_SELECTED)
	{	//"Do" is selected, return nonzero if a match has been made
		if(match)
		{
			return 1;
		}
	}
	else
	{	//"Do not" is selected, return nonzero if a match has not been made
		if(!match)
		{
			return 1;
		}
	}

	return 0;
}

int eof_menu_edit_conditional_selection_logic(int function)
{
	unsigned long ctr, bitmask, match_bitmask = 0, stringcount, cymbal_match_bitmask = 0;
	static unsigned last_selected_pg_normal = D_SELECTED, last_selected_pg_mute = D_SELECTED, last_selected_pg_ghost = D_SELECTED;	//Track the last specified status for these pro guitar gem types

	if(!eof_song || (eof_selected_track >= eof_song->tracks))
		return 0;	//Return error
	if(!function && (eof_selection.track != eof_selected_track))
		return 1;	//No notes in the active track are selected so none can become deselected

	if(!function)
	{	//Perform conditional deselection
		strncpy(eof_menu_edit_conditional_selection_dialog_string, "Deselect notes that", sizeof(eof_menu_edit_conditional_selection_dialog_string) - 1);
	}
	else
	{	//Perform conditional selection
		strncpy(eof_menu_edit_conditional_selection_dialog_string, "Select notes that", sizeof(eof_menu_edit_conditional_selection_dialog_string) - 1);
	}
	eof_color_dialog(eof_menu_edit_conditional_selection_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_menu_edit_conditional_selection_dialog);

	//Prepare the dialog
	stringcount = eof_count_track_lanes(eof_song, eof_selected_track);
	for(ctr = 0; ctr < 6; ctr++)
	{	//For each of the 6 supported strings
		if(ctr < stringcount)
		{	//If this track uses this string
			eof_menu_edit_conditional_selection_dialog[7 + ctr].flags = 0;			//Enable this lane's checkbox
		}
		else
		{
			eof_menu_edit_conditional_selection_dialog[7 + ctr].flags = D_HIDDEN;	//Hide this lane's checkbox
		}
	}
	if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
	{	//If a drum track is active
		eof_menu_edit_conditional_selection_dialog[13].flags = eof_menu_edit_conditional_selection_dialog[14].flags =
		eof_menu_edit_conditional_selection_dialog[15].flags = eof_menu_edit_conditional_selection_dialog[16].flags = 0;	//Enable the cymbal checkboxes
	}
	else
	{
		eof_menu_edit_conditional_selection_dialog[13].flags = eof_menu_edit_conditional_selection_dialog[14].flags =
		eof_menu_edit_conditional_selection_dialog[15].flags = eof_menu_edit_conditional_selection_dialog[16].flags = D_HIDDEN;	//Hide the cymbal checkboxes
	}
	if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
	{	//If a pro guitar track is active
		eof_menu_edit_conditional_selection_dialog[17].flags = 0;
		eof_menu_edit_conditional_selection_dialog[18].flags = last_selected_pg_normal;
		eof_menu_edit_conditional_selection_dialog[19].flags = last_selected_pg_mute;
		eof_menu_edit_conditional_selection_dialog[20].flags = last_selected_pg_ghost;
	}
	else
	{
		eof_menu_edit_conditional_selection_dialog[17].flags = eof_menu_edit_conditional_selection_dialog[18].flags =
		eof_menu_edit_conditional_selection_dialog[19].flags = eof_menu_edit_conditional_selection_dialog[20].flags = D_HIDDEN;	//Hide the normal/mute/ghost checkboxes
	}

	//Process the dialog
	if(eof_popup_dialog(eof_menu_edit_conditional_selection_dialog, 0) != 21)
		return 1;	//If the user did not click OK, return immediately

	if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
	{
		last_selected_pg_normal = eof_menu_edit_conditional_selection_dialog[18].flags;
		last_selected_pg_mute = eof_menu_edit_conditional_selection_dialog[19].flags;
		last_selected_pg_ghost = eof_menu_edit_conditional_selection_dialog[20].flags;
	}
	for(ctr = 0, bitmask = 1; ctr < stringcount; ctr++, bitmask <<= 1)
	{	//For each lane in use for this track
		if(eof_menu_edit_conditional_selection_dialog[7 + ctr].flags == D_SELECTED)
		{	//If the user checked this lane's checkbox
			match_bitmask |= bitmask;	//Set the appropriate bit in the match bitmask
		}
	}
	if(eof_menu_edit_conditional_selection_dialog[14].flags == D_SELECTED)
	{	//Lane 3 cymbal cyecked
		cymbal_match_bitmask |= 4;
	}
	if(eof_menu_edit_conditional_selection_dialog[15].flags == D_SELECTED)
	{	//Lane 4 cymbal cyecked
		cymbal_match_bitmask |= 8;
	}
	if(eof_menu_edit_conditional_selection_dialog[16].flags == D_SELECTED)
	{	//Lane 5 cymbal cyecked
		cymbal_match_bitmask |= 16;
	}

	for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
	{	//For each note in the track
		if(eof_get_note_type(eof_song, eof_selected_track, ctr) != eof_note_type)
			continue;	//If the note is not in the active track ddifficulty, skip it

		if(!function)
		{	//Perform conditional deselection
			if(eof_selection.multi[ctr] && eof_check_note_conditional_selection(eof_song, eof_selected_track, ctr, match_bitmask, cymbal_match_bitmask))
			{	//If this note is selected and matches the conditions the user specified
				eof_selection.multi[ctr] = 0;	//Deselect the note
			}
		}
		else
		{	//Perform conditional selection
			if(eof_check_note_conditional_selection(eof_song, eof_selected_track, ctr, match_bitmask, cymbal_match_bitmask))
			{	//If this note matches the conditions the user specified
				if(eof_selection.track != eof_selected_track)
				{	//If no notes in the current track are currently selected
					(void) eof_menu_edit_deselect_all();		//Clear the selection variables if necessary
					eof_selection.track = eof_selected_track;	//Indicate that the active track is the one with notes selected
				}
				eof_selection.multi[ctr] = 1;	//Select the note
			}
		}
	}//For each note in the track
	if(eof_selection.current != EOF_MAX_NOTES - 1)
	{	//If there was a last selected note
		if(eof_selection.multi[eof_selection.current] == 0)
		{	//And it's not selected anymore
			eof_selection.current = EOF_MAX_NOTES - 1;	//Clear the selected note
		}
	}

	return 1;
}

int eof_menu_edit_deselect_conditional(void)
{
	return eof_menu_edit_conditional_selection_logic(0);
}

int eof_menu_edit_select_conditional(void)
{
	return eof_menu_edit_conditional_selection_logic(1);
}

int eof_menu_edit_select_logic(int (*check)(EOF_SONG *, unsigned long, unsigned long))
{
	unsigned long ctr;

	if(!eof_song || (eof_selected_track >= eof_song->tracks) || !check)
		return 0;	//Return error

	for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
	{	//For each note in the track
		if(eof_get_note_type(eof_song, eof_selected_track, ctr) == eof_note_type)
		{	//If the note is in the active track difficulty
			if(check(eof_song, eof_selected_track, ctr))
			{	//If this note meets the criterion for being selected
				if(eof_selection.track != eof_selected_track)
				{	//If no notes in the current track are currently selected
					(void) eof_menu_edit_deselect_all();		//Clear the selection variables if necessary
					eof_selection.track = eof_selected_track;	//Indicate that the active track is the one with notes selected
				}
				eof_selection.multi[ctr] = 1;	//Select the note
			}
		}
	}

	return 1;
}

int eof_menu_edit_select_chords(void)
{
	return eof_menu_edit_select_logic(eof_note_is_chord);
}

int eof_menu_edit_select_single_notes(void)
{
	return eof_menu_edit_select_logic(eof_note_is_single_note);
}

int eof_menu_edit_select_toms(void)
{
	return eof_menu_edit_select_logic(eof_note_is_tom);
}

int eof_menu_edit_select_cymbals(void)
{
	return eof_menu_edit_select_logic(eof_note_is_cymbal);
}

int eof_menu_edit_select_grid_snapped_notes(void)
{
	return eof_menu_edit_select_logic(eof_note_is_grid_snapped);
}

int eof_menu_edit_select_non_grid_snapped_notes(void)
{
	return eof_menu_edit_select_logic(eof_note_is_not_grid_snapped);
}

int eof_menu_edit_select_highlighted_notes(void)
{
	return eof_menu_edit_select_logic(eof_note_is_highlighted);
}

int eof_menu_edit_select_non_highlighted_notes(void)
{
	return eof_menu_edit_select_logic(eof_note_is_not_highlighted);
}

int eof_menu_edit_select_open_notes(void)
{
	return eof_menu_edit_select_logic(eof_note_is_open_note);
}

int eof_menu_edit_select_non_open_notes(void)
{
	return eof_menu_edit_select_logic(eof_note_is_not_open_note);
}

int eof_menu_edit_deselect_logic(int (*check)(EOF_SONG *, unsigned long, unsigned long))
{
	unsigned long ctr;

	if(!eof_song || (eof_selected_track >= eof_song->tracks) || !check)
		return 0;	//Return error
	if(eof_selection.track != eof_selected_track)
		return 1;	//No notes in the active track are selected so none can become deselected

	for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
	{	//For each note in the track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[ctr] && (eof_get_note_type(eof_song, eof_selected_track, ctr) == eof_note_type))
		{	//If the note is in the active instrument difficulty and is selected
			if(check(eof_song, eof_selected_track, ctr))
			{	//If this note meets the criterion for being deselected
				eof_selection.multi[ctr] = 0;	//Deselect it
			}
		}
	}
	if(eof_selection.current != EOF_MAX_NOTES - 1)
	{	//If there was a last selected note
		if(eof_selection.multi[eof_selection.current] == 0)
		{	//And it's not selected anymore
			eof_selection.current = EOF_MAX_NOTES - 1;	//Clear the selected note
		}
	}

	return 1;
}

int eof_menu_edit_deselect_chords(void)
{
	return eof_menu_edit_deselect_logic(eof_note_is_chord);
}

int eof_menu_edit_deselect_single_notes(void)
{
	return eof_menu_edit_deselect_logic(eof_note_is_single_note);
}

int eof_menu_edit_deselect_toms(void)
{
	return eof_menu_edit_deselect_logic(eof_note_is_tom);
}

int eof_menu_edit_deselect_cymbals(void)
{
	return eof_menu_edit_deselect_logic(eof_note_is_cymbal);
}

int eof_menu_edit_deselect_grid_snapped_notes(void)
{
	return eof_menu_edit_deselect_logic(eof_note_is_grid_snapped);
}

int eof_menu_edit_deselect_non_grid_snapped_notes(void)
{
	return eof_menu_edit_deselect_logic(eof_note_is_not_grid_snapped);
}

int eof_menu_edit_deselect_highlighted_notes(void)
{
	return eof_menu_edit_deselect_logic(eof_note_is_highlighted);
}

int eof_menu_edit_deselect_non_highlighted_notes(void)
{
	return eof_menu_edit_deselect_logic(eof_note_is_not_highlighted);
}

int eof_menu_edit_deselect_open_notes(void)
{
	return eof_menu_edit_deselect_logic(eof_note_is_open_note);
}

int eof_menu_edit_deselect_non_open_notes(void)
{
	return eof_menu_edit_deselect_logic(eof_note_is_not_open_note);
}

char eof_menu_edit_select_gem_count_dialog_string[25];
DIALOG eof_menu_edit_select_gem_count_dialog[] =
{
	/*	(proc)              (x)  (y)  (w)  (h) (fg) (bg) (key) (flags)     (d1) (d2) (dp)                   (dp2) (dp3) */
	{d_agup_window_proc,    0,   0,   216, 140,2,   23,  0,    0,          0,   0,   eof_menu_edit_select_gem_count_dialog_string, NULL, NULL },
	{d_agup_radio_proc,	    16,  32,  38,  16, 2,   23,  0,    D_SELECTED, 1,   0,   "Do",                   NULL, NULL },
	{d_agup_radio_proc,	    72,  32,  62,  16, 2,   23,  0,    0,          1,   0,   "Do not",               NULL, NULL },
	{d_agup_text_proc,      16,  52,  44,  8,  2,   23,  0,    0,          0,   0,   "Have exactly this many gems:",       NULL, NULL },
	{eof_verified_edit_proc,16,  72,  28,  20, 2,   23,  0,    0,          1,   0,   eof_etext, "123456", NULL },
	{d_agup_button_proc,    12,  100, 68,  28, 2,   23,  '\r', D_EXIT,     0,   0,   "OK",                   NULL, NULL },
	{d_agup_button_proc,    130, 100, 68,  28, 2,   23,  0,    D_EXIT,     0,   0,   "Cancel",               NULL, NULL },
	{NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

int eof_menu_edit_select_gem_count_logic(int function)
{
	unsigned long targetcount, ctr, gemcount;

	if(!eof_song || (eof_selected_track >= eof_song->tracks))
		return 0;	//Return error
	if(!function && (eof_selection.track != eof_selected_track))
		return 1;	//No notes in the active track are selected so none can become deselected

	//Set up and run dialog
	if(!function)
	{	//Perform conditional deselection
		strncpy(eof_menu_edit_select_gem_count_dialog_string, "Deselect notes that", sizeof(eof_menu_edit_select_gem_count_dialog_string) - 1);
	}
	else
	{	//Perform conditional selection
		strncpy(eof_menu_edit_select_gem_count_dialog_string, "Select notes that", sizeof(eof_menu_edit_select_gem_count_dialog_string) - 1);
	}
	eof_etext[0] = '\0';
	eof_color_dialog(eof_menu_edit_select_gem_count_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_menu_edit_select_gem_count_dialog);
	if(eof_popup_dialog(eof_menu_edit_select_gem_count_dialog, 4) != 5)
		return 1;	//If the user did not click OK, return immediately
	targetcount = atol(eof_etext);
	if(!targetcount)
		return 0;	//Couldn't convert the string to a number

	//Process notes
	for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
	{	//For each note in the track
		if(eof_get_note_type(eof_song, eof_selected_track, ctr) == eof_note_type)
		{	//If the note is in the active track difficulty
			gemcount = eof_note_count_colors(eof_song, eof_selected_track, ctr);	//Determine how many gems this note has

			if(eof_menu_edit_select_gem_count_dialog[1].flags == D_SELECTED)
			{	//User specified to select/deselect notes that have a specified gem count
				if(!function)
				{	//User specified to deselect applicable notes
					if(gemcount == targetcount)
					{	//If this note meets the deselection criteria
						eof_selection.multi[ctr] = 0;	//Deselect it
					}
				}
				else
				{	//user specified to select applicable notes
					if(gemcount == targetcount)
					{	//If this note meets the selection criteria
						if(eof_selection.track != eof_selected_track)
						{	//If no notes in the current track are currently selected
							(void) eof_menu_edit_deselect_all();		//Clear the selection variables if necessary
							eof_selection.track = eof_selected_track;	//Indicate that the active track is the one with notes selected
						}
						eof_selection.multi[ctr] = 1;	//Select the note
					}
				}
			}
			else
			{	//User specified to select/deselect notes that DO NOT have a specified gem count
				if(!function)
				{	//User specified to deselect applicable notes
					if(gemcount != targetcount)
					{	//If this note meets the deselection criteria
						eof_selection.multi[ctr] = 0;	//Deselect it
					}
				}
				else
				{	//user specified to select applicable notes
					if(gemcount != targetcount)
					{	//If this note meets the selection criteria
						if(eof_selection.track != eof_selected_track)
						{	//If no notes in the current track are currently selected
							(void) eof_menu_edit_deselect_all();		//Clear the selection variables if necessary
							eof_selection.track = eof_selected_track;	//Indicate that the active track is the one with notes selected
						}
						eof_selection.multi[ctr] = 1;	//Select the note
					}
				}
			}
		}
	}

	//Clean up
	if(!function && (eof_selection.current != EOF_MAX_NOTES - 1))
	{	//If there was a last selected note
		if(eof_selection.multi[eof_selection.current] == 0)
		{	//And it's not selected anymore
			eof_selection.current = EOF_MAX_NOTES - 1;	//Clear the selected note
		}
	}

	return 1;
}

int eof_menu_edit_select_gem_count(void)
{
	return eof_menu_edit_select_gem_count_logic(1);
}

int eof_menu_edit_deselect_gem_count(void)
{
	return eof_menu_edit_select_gem_count_logic(0);
}

int eof_menu_edit_invert_selection(void)
{
	unsigned long i;

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the track
		if(eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type)
		{	//If the note is in the active track difficulty
			if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
			{	//If the note is selected
				eof_selection.multi[i] = 0;	//Deselect the note
			}
			else
			{
				eof_selection.multi[i] = 1;	//Otherwise select the note
				eof_selection.track = eof_selected_track;
			}
		}
		else
		{
			eof_selection.multi[i] = 0;	//Otherwise deselect the note
		}
	}
	return 1;
}

int eof_menu_edit_paste_from_supaeasy(void)
{
	char undo_made = 0;
	return eof_menu_edit_paste_from_difficulty(EOF_NOTE_SUPAEASY, &undo_made);
}

int eof_menu_edit_paste_from_easy(void)
{
	char undo_made = 0;
	return eof_menu_edit_paste_from_difficulty(EOF_NOTE_EASY, &undo_made);
}

int eof_menu_edit_paste_from_medium(void)
{
	char undo_made = 0;
	return eof_menu_edit_paste_from_difficulty(EOF_NOTE_MEDIUM, &undo_made);
}

int eof_menu_edit_paste_from_amazing(void)
{
	char undo_made = 0;
	return eof_menu_edit_paste_from_difficulty(EOF_NOTE_AMAZING, &undo_made);
}

int eof_menu_edit_paste_from_challenge(void)
{
	char undo_made = 0;
	return eof_menu_edit_paste_from_difficulty(EOF_NOTE_CHALLENGE, &undo_made);
}

int eof_menu_edit_paste_from_difficulty(unsigned long source_difficulty, char *undo_made)
{
	unsigned long i, pos, tracknum;
	long length;
	EOF_PHRASE_SECTION *ptr;
	EOF_PRO_GUITAR_TRACK *tp = NULL;
	char restore_tech_view = 0;		//If tech view is in effect, it is temporarily disabled until after the secondary piano roll has been rendered

	if(!undo_made || (eof_note_type == source_difficulty))
		return 1;	//Invalid parameters

	if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
	{	//If a pro guitar track is active
		tracknum = eof_song->track[eof_selected_track]->tracknum;
		tp = eof_song->pro_guitar_track[tracknum];
		restore_tech_view = eof_menu_track_get_tech_view_state(eof_song, eof_selected_track);
		eof_menu_track_set_tech_view_state(eof_song, eof_selected_track, 0);	//Disable tech view if applicable
		(void) eof_detect_difficulties(eof_song, eof_selected_track);	//Update eof_track_diff_populated_status[] to reflect all populated difficulties for the active track
	}

	if(eof_track_diff_populated_status[eof_note_type])
	{	//If the current difficulty is populated
		eof_clear_input();
		if(alert(NULL, "This operation will replace this difficulty's contents.", "Continue?", "&Yes", "&No", 'y', 'n') != 1)
		{	//If user does not opt to overwrite this difficulty
			return 1;
		}
	}
	eof_clear_input();
	if(*undo_made == 0)
	{	//If an undo state hasn't been made
		eof_prepare_undo(EOF_UNDO_TYPE_NOTE_SEL);
		*undo_made = 1;
	}

	eof_erase_track_difficulty(eof_song, eof_selected_track, eof_note_type);	//Erase the contents of the active track difficulty

	//Copy notes from the source difficulty
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in this instrument track
		if(eof_get_note_type(eof_song, eof_selected_track, i) == source_difficulty)
		{	//If this note is in the source difficulty
			pos = eof_get_note_pos(eof_song, eof_selected_track, i);
			length = eof_get_note_length(eof_song, eof_selected_track, i);
			(void) eof_copy_note_simple(eof_song, eof_selected_track, i, eof_selected_track, pos, length, eof_note_type);
		}
	}

	if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
	{	//If this is a pro guitar track
		//Copy tech notes from the source difficulty
		eof_menu_track_set_tech_view_state(eof_song, eof_selected_track, 1);	//Enable tech view
		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//For each note in this instrument track
			if(eof_get_note_type(eof_song, eof_selected_track, i) == source_difficulty)
			{	//If this note is in the source difficulty
				pos = eof_get_note_pos(eof_song, eof_selected_track, i);
				length = eof_get_note_length(eof_song, eof_selected_track, i);
				(void) eof_copy_note_simple(eof_song, eof_selected_track, i, eof_selected_track, pos, length, eof_note_type);
			}
		}
		eof_track_sort_notes(eof_song, eof_selected_track);	//Sort tech notes before switching back to the normal note array
		eof_menu_track_set_tech_view_state(eof_song, eof_selected_track, restore_tech_view);	//Restore original tech view state
		(void) eof_detect_difficulties(eof_song, eof_selected_track);	//Update eof_track_diff_populated_status[] to reflect all populated difficulties for the active track

		//Copy arpeggios from the source difficulty
		for(i = 0; i < eof_get_num_arpeggios(eof_song, eof_selected_track); i++)
		{	//For each arpeggio phrase in the source track
			ptr = eof_get_arpeggio(eof_song, eof_selected_track, i);
			if(ptr)
			{	//If this phrase could be found
				if(ptr->difficulty == source_difficulty)
				{	//If this is an arpeggio section defined in the source difficulty
					EOF_PHRASE_SECTION *ptr2;
					(void) eof_track_add_section(eof_song, eof_selected_track, EOF_ARPEGGIO_SECTION, eof_note_type, ptr->start_pos, ptr->end_pos, 0, NULL);	//Copy it to the active difficulty
					ptr2 = eof_get_arpeggio(eof_song, eof_selected_track, eof_get_num_arpeggios(eof_song, eof_selected_track) - 1);	//Get a pointer to the newly created arpeggio
					ptr2->flags = ptr->flags;	//Copy the arpeggio flags in case it was a handshape that was copied
				}
			}
		}

		//Copy hand positions from the source difficulty
		for(i = 0; i < tp->handpositions; i++)
		{	//For each hand position in the track
			if(tp->handposition[i].difficulty == source_difficulty)
			{	//If this is a hand position in the source difficulty
				(void) eof_track_add_section(eof_song, eof_selected_track, EOF_FRET_HAND_POS_SECTION, eof_note_type, tp->handposition[i].start_pos, tp->handposition[i].end_pos, 0, NULL);	//Create a duplicate of this hand position in the target difficulty
			}
		}
		eof_pro_guitar_track_sort_fret_hand_positions(tp);	//Sort the positions, since they must be in order for displaying to the user
	}//If this is a pro guitar track

	//Copy tremolos from the source difficulty
	for(i = 0; i < eof_get_num_tremolos(eof_song, eof_selected_track); i++)
	{	//For each tremolo phrase in the source track
		ptr = eof_get_tremolo(eof_song, eof_selected_track, i);
		if(ptr)
		{	//If this phrase could be found
			if(ptr->difficulty == source_difficulty)
			{	//If this is a tremolo section defined in the source difficulty
				(void) eof_track_add_section(eof_song, eof_selected_track, EOF_TREMOLO_SECTION, eof_note_type, ptr->start_pos, ptr->end_pos, 0, NULL);	//Copy it to the active difficulty
			}
		}
	}

	eof_track_sort_notes(eof_song, eof_selected_track);
	eof_determine_phrase_status(eof_song, eof_selected_track);	//Update note flags, since pasted notes may no longer be within tremolos
	(void) eof_detect_difficulties(eof_song, eof_selected_track);
	return 1;
}

static unsigned long notes_in_beat(unsigned long beat)
{
	unsigned long count = 0, i;

	if(!eof_song || (!eof_beat_num_valid(eof_song, beat)))
		return 0;	//Error

	if(beat > eof_song->beats - 2)
	{
		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//For each note in the active track
			if((eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type) && (eof_get_note_pos(eof_song, eof_selected_track, i) >= eof_song->beat[beat]->pos))
			{
				count++;
			}
		}
	}
	else
	{
		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//For each note in the active track
			if((eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type) && (eof_get_note_pos(eof_song, eof_selected_track, i) >= eof_song->beat[beat]->pos) && (eof_get_note_pos(eof_song, eof_selected_track, i) < eof_song->beat[beat + 1]->pos))
			{
				count++;
			}
		}
	}
	return count;
}

static unsigned long lyrics_in_beat(unsigned long beat)
{
	unsigned long count = 0;
	unsigned long i;

	if(!eof_song || (!eof_beat_num_valid(eof_song, beat)))
		return 0;	//Error

	if(beat > eof_song->beats - 2)
	{
		for(i = 0; i < eof_song->vocal_track[0]->lyrics; i++)
		{
			if(eof_song->vocal_track[0]->lyric[i]->pos >= eof_song->beat[beat]->pos)
			{
				count++;
			}
		}
	}
	else
	{
		for(i = 0; i < eof_song->vocal_track[0]->lyrics; i++)
		{
			if((eof_song->vocal_track[0]->lyric[i]->pos >= eof_song->beat[beat]->pos) && (eof_song->vocal_track[0]->lyric[i]->pos < eof_song->beat[beat + 1]->pos))
			{
				count++;
			}
		}
	}
	return count;
}

int eof_menu_edit_paste_from_catalog(void)
{
	unsigned long i, j, bitmask;
	unsigned long paste_pos[EOF_MAX_NOTES] = {0};
	unsigned long paste_count = 0;
	unsigned long note_count = 0;
	unsigned long first = ULONG_MAX;
	unsigned long first_beat = ULONG_MAX;
	unsigned long start_beat = eof_get_beat(eof_song, eof_music_pos - eof_av_delay);
	unsigned long this_beat = ULONG_MAX;
	unsigned long current_beat = eof_get_beat(eof_song, eof_music_pos - eof_av_delay);
	unsigned long last_current_beat = current_beat;
	unsigned long end_beat = ULONG_MAX;
	double nporpos, nporendpos;
	EOF_NOTE * new_note = NULL;
	unsigned long newnotenum, sourcetrack, highestfret = 0, highestlane = 0, currentfret;
	unsigned long numlanes = eof_count_track_lanes(eof_song, eof_selected_track);
	EOF_CATALOG_ENTRY *entry;

	if((eof_selected_catalog_entry >= eof_song->catalog->entries) || !eof_song->catalog->entries)
		return 1;	//If a valid catalog entry is not selected, return immediately

	entry = &eof_song->catalog->entry[eof_selected_catalog_entry];

	/* make sure we can paste */
	if(eof_music_pos - eof_av_delay < eof_song->beat[0]->pos)
	{
		return 1;
	}

	sourcetrack = entry->track;

	/* make sure we can't paste inside of the catalog entry */
	if((sourcetrack == eof_selected_track) && (entry->type == eof_note_type) && (eof_music_pos - eof_av_delay >= entry->start_pos) && (eof_music_pos - eof_av_delay <= entry->end_pos))
	{
		return 1;
	}

	//Don't allow copying instrument track notes to PART VOCALS and vice versa
	if(((sourcetrack == EOF_TRACK_VOCALS) && (eof_selected_track != EOF_TRACK_VOCALS)) || ((sourcetrack != EOF_TRACK_VOCALS) && (eof_selected_track == EOF_TRACK_VOCALS)))
		return 1;

	for(i = 0; i < eof_get_track_size(eof_song, sourcetrack); i++)
	{	//For each note in the active catalog entry's track
		if((eof_get_note_type(eof_song, sourcetrack, i) != entry->type) || (eof_get_note_pos(eof_song, sourcetrack, i) < entry->start_pos) || (eof_get_note_pos(eof_song, sourcetrack, i) + eof_get_note_length(eof_song, sourcetrack, i) > entry->end_pos))
			continue;	//If the note isn't in the scope of the current catalog entry, skip it

		note_count++;
		currentfret = eof_get_highest_fret_value(eof_song, sourcetrack, i);	//Get the highest used fret value in this note if applicable
		if(currentfret > highestfret)
		{	//Track the highest used fret value
			highestfret = currentfret;
		}
		for(j = 1, bitmask = 1; j < 9; j++, bitmask<<=1)
		{	//For each of the 8 bits in the bitmask
			if(bitmask & eof_get_note_note(eof_song, sourcetrack, i))
			{	//If this bit is in use
				if(j > highestlane)
				{	//If this lane is higher than the previously tracked highest lane
					highestlane = j;
				}
			}
		}
	}
	if(note_count == 0)
	{
		return 1;
	}
	if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
	{	//If the current track is pro guitar format, warn if pasted notes go above the current track's fret limit
		unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
		if(highestfret > eof_song->pro_guitar_track[tracknum]->numfrets)
		{	//If any notes in the catalog entry would exceed the active track's fret limit
			char message[120] = {0};
			(void) snprintf(message, sizeof(message) - 1, "Warning:  This track's fret limit is exceeded by a pasted note's fret value of %lu.  Continue?", highestfret);
			eof_clear_input();
			if(alert(NULL, message, NULL, "&Yes", "&No", 'y', 'n') != 1)
			{	//If user does not opt to continue after being alerted of this fret limit issue
				return 0;
			}
		}
	}
	if(highestlane > numlanes)
	{	//Warn if pasted notes go above the current track's lane limit
		char message[120] = {0};
		(void) snprintf(message, sizeof(message) - 1, "Warning:  This track's highest lane number is exceeded by a pasted note with a gem on lane %lu.", highestlane);
		eof_clear_input();
		if(alert(NULL, message, "Such notes will be omitted.  Continue?", "&Yes", "&No", 'y', 'n') != 1)
		{	//If user does not opt to continue after being alerted of this lane limit issue
			return 0;
		}
	}

	eof_prepare_undo(EOF_UNDO_TYPE_NOTE_SEL);
	if(eof_paste_erase_overlap)
	{	//If the user decided to delete existing notes that are between the start and end of the pasted notes
		unsigned long clear_start = 0, clear_end = 0;
		for(i = 0; i < eof_get_track_size(eof_song, sourcetrack); i++)
		{	//For each note in the active catalog entry's track
			unsigned long pos = eof_get_note_pos(eof_song, sourcetrack, i);

			/* this note needs to be copied */
			if((eof_get_note_type(eof_song, sourcetrack, i) != entry->type) || (pos < entry->start_pos) || (pos + eof_get_note_length(eof_song, sourcetrack, i) > entry->end_pos))
				continue;	//If this note doesn't start and end within the scope of the catalog entry, skip it

			if(first == ULONG_MAX)
			{
				first_beat = eof_get_beat(eof_song, pos);
			}
			this_beat = eof_get_beat(eof_song, pos);
			if(!eof_beat_num_valid(eof_song, this_beat))
			{
				break;
			}
			current_beat = eof_get_beat(eof_song, eof_music_pos - eof_av_delay) + (this_beat - first_beat);
			if(!eof_beat_num_valid(eof_song, current_beat) || (current_beat >= eof_song->beats - 1))
			{
				break;
			}

			nporpos = eof_get_porpos(pos);
			nporendpos = eof_get_porpos(pos + eof_get_note_length(eof_song, sourcetrack, i));
			end_beat = eof_get_beat(eof_song, pos + eof_get_note_length(eof_song, sourcetrack, i));
			if(!eof_beat_num_valid(eof_song, end_beat))
			{
				break;
			}

			if(first == ULONG_MAX)
			{	//Track the start of the range of notes to clear
				clear_start = eof_put_porpos(current_beat, nporpos, 0.0);
				first = 1;
			}
			clear_end = eof_put_porpos(end_beat - first_beat + start_beat, nporendpos, 0.0);	//Track the end of each note so the end of the pasted notes can be tracked
		}//For each note in the active catalog entry's track
		eof_menu_edit_paste_clear_range(eof_selected_track, eof_note_type, clear_start, clear_end);	//Erase the notes that would get in the way of the pasted catalog entry

		//Re-initialize some variables for the regular paste from catalog logic
		first = first_beat = this_beat = end_beat = ULONG_MAX;
		current_beat = eof_get_beat(eof_song, eof_music_pos - eof_av_delay);
		last_current_beat = current_beat;
	}
	for(i = 0; i < eof_get_track_size(eof_song, sourcetrack); i++)
	{	//For each note in the active catalog entry's track
		unsigned long pos = eof_get_note_pos(eof_song, sourcetrack, i);

		/* this note needs to be copied */
		if((eof_get_note_type(eof_song, sourcetrack, i) != entry->type) || (pos < entry->start_pos) || (pos + eof_get_note_length(eof_song, sourcetrack, i) > entry->end_pos))
			continue;	//If this note doesn't start and end within the scope of the catalog entry, skip it

		if(first == ULONG_MAX)
		{
			first_beat = eof_get_beat(eof_song, pos);
			first = 1;
		}
		this_beat = eof_get_beat(eof_song, pos);
		if(!eof_beat_num_valid(eof_song, this_beat))
		{
			break;
		}
		last_current_beat = current_beat;
		current_beat = eof_get_beat(eof_song, eof_music_pos - eof_av_delay) + (this_beat - first_beat);
		if(!eof_beat_num_valid(eof_song, current_beat) || (current_beat >= eof_song->beats - 1))
		{	//If the beat is at or after the last beat or otherwise not valid
			break;
		}

		/* if we run into notes, abort */
		if(!eof_paste_erase_overlap)
		{	//But only if the user hasn't already allowed any notes that would have been in the way to be deleted
			if((eof_vocals_selected) && (lyrics_in_beat(current_beat) && (last_current_beat != current_beat)))
			{
				break;
			}
			else if(notes_in_beat(current_beat) && (last_current_beat != current_beat))
			{
				break;
			}
		}
		nporpos = eof_get_porpos(pos);
		nporendpos = eof_get_porpos(pos + eof_get_note_length(eof_song, sourcetrack, i));
		end_beat = eof_get_beat(eof_song, pos + eof_get_note_length(eof_song, sourcetrack, i));
		if(end_beat == ULONG_MAX)
		{	//If a valid beat number wasn't found by eof_get_beat()
			break;
		}

		/* paste the note */
		if(eof_beat_num_valid(eof_song, end_beat - first_beat + start_beat))
		{
			new_note = eof_copy_note_simple(eof_song, sourcetrack, i, eof_selected_track, eof_put_porpos(current_beat, nporpos, 0.0), eof_put_porpos(end_beat - first_beat + start_beat, nporendpos, 0.0) - eof_put_porpos(current_beat, nporpos, 0.0), eof_note_type);
			if(new_note)
			{	//If the note was successfully created
				newnotenum = eof_get_track_size(eof_song, eof_selected_track) - 1;	//The index of the new note
				paste_pos[paste_count] = eof_get_note_pos(eof_song, eof_selected_track, newnotenum);
				paste_count++;
			}
		}
	}//For each note in the active catalog entry's track

	eof_track_sort_notes(eof_song, eof_selected_track);
	eof_track_fixup_notes(eof_song, eof_selected_track, 0);
	eof_determine_phrase_status(eof_song, eof_selected_track);
	(void) eof_detect_difficulties(eof_song, eof_selected_track);
	eof_selection.current_pos = 0;
	(void) eof_menu_edit_deselect_all();	//Clear the seek selection and notes array
	for(i = 0; i < paste_count; i++)
	{	//For each of the pasted notes
		for(j = 0; j < eof_get_track_size(eof_song, eof_selected_track); j++)
		{	//For each note in the destination track
			if((eof_get_note_pos(eof_song, eof_selected_track, j) == paste_pos[i]) && (eof_get_note_type(eof_song, eof_selected_track, j) == eof_note_type))
			{	//If this note is in the current difficulty and matches the position of one of the pasted notes
				eof_selection.track = eof_selected_track;	//Mark the note as selected
				eof_selection.multi[j] = 1;
				break;
			}
		}
	}
	return 1;
}

int eof_menu_edit_select_previous(void)
{
	unsigned long i;

	if(eof_count_selected_notes(NULL) == 0)	//If no notes are selected
	{
		return 1;
	}
	if(eof_selection.current == EOF_MAX_NOTES - 1)	//No notes selected?
		return 1;	//Don't perform this operation

	for(i = 0; (i < eof_selection.current) && (i < eof_get_track_size(eof_song, eof_selected_track)); i++)
	{
		if(eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type)
		{
			eof_selection.multi[i] = 1;
		}
	}

	return 1;
}

void eof_sanitize_note_flags(unsigned long *flags, unsigned long sourcetrack, unsigned long desttrack)
{
	if((flags == NULL) || (desttrack >= eof_song->tracks) || (sourcetrack >= eof_song->tracks))
		return;

	if((eof_song->track[sourcetrack]->track_format == EOF_LEGACY_TRACK_FORMAT) && (eof_song->track[desttrack]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
	{	//If the note is copying from a legacy track to a pro guitar track
		if(*flags & EOF_NOTE_FLAG_F_HOPO)
		{	//If the forced HOPO flag is set
			*flags &= ~EOF_NOTE_FLAG_F_HOPO;		//Clear that flag
			*flags |= EOF_PRO_GUITAR_NOTE_FLAG_HO;	//Set the pro guitar hammer on flag
		}
	}
	if(eof_song->track[sourcetrack]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
	{	//If the note is copying from a pro guitar track
		if(eof_song->track[desttrack]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If it is pasting into a non pro guitar track, erase all pro guitar flags as they are invalid
			*flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_ACCENT;			//Erase the pro guitar accent flag
			*flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_P_HARMONIC;		//Erase the pro guitar pinch harmonic flag
			*flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_HO;				//Erase the pro hammer on flag
			*flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_PO;				//Erase the pro hammer off flag
			*flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_TAP;			//Erase the pro tap flag
			*flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP;		//Erase the pro slide up flag
			*flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN;		//Erase the pro slide down flag
			*flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE;	//Erase the pro string mute flag
			*flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE;		//Erase the pro palm mute flag
			*flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_UP_STRUM;		//Erase the pro strum up flag
			*flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_DOWN_STRUM;		//Erase the pro strum down flag
			*flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_MID_STRUM;		//Erase the pro strum mid flag
			*flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_BEND;			//Erase the pro bend flag
			*flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC;		//Erase the pro harmonic flag
			*flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_REVERSE;	//Erase the pro guitar slide reverse flag
			*flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO;		//Erase the pro guitar vibrato flag
			*flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION;	//Erase the pro guitar Rocksmith status flag
			*flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_POP;			//Erase the pro guitar pop flag
			*flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_SLAP;			//Erase the pro guitar slap flag
			*flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_LINKNEXT;		//Erase the pro guitar linknext flag
			*flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_HD;				//Erase the pro guitar high density flag
			*flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_SPLIT;			//Erase the pro guitar split flag
		}
		else
		{	//If it is pasting into a pro guitar track
			if((*flags & EOF_PRO_GUITAR_NOTE_FLAG_HO) && (*flags & EOF_PRO_GUITAR_NOTE_FLAG_PO))
			{	//If both the hammer on AND the pull off flags are set, clear both
				*flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_HO;
				*flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_PO;
			}
			if(*flags & EOF_PRO_GUITAR_NOTE_FLAG_TAP)
			{	//If the tap flag is set
				if(*flags & EOF_PRO_GUITAR_NOTE_FLAG_HO)
				{	//If the hammer on flag is also set, clear both
					*flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_TAP;
					*flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_HO;
				}
				if(*flags & EOF_PRO_GUITAR_NOTE_FLAG_PO)
				{	//If the pull off flag is also set, clear both
					*flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_TAP;
					*flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_PO;
				}
			}
			if((*flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP) && (*flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN))
			{	//If both the slide up AND the slide down flags are set, clear both
				*flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP;
				*flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN;
			}
			if(!(*flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP) && !(*flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN))
			{	//If neither the slide up nor the slide down flags are set
				*flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_REVERSE;	//Clear the slide reverse flag
			}
			if((*flags & EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE) && (*flags & EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE))
			{	//If both the string mute AND the palm mute flags are set, clear both
				*flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE;
				*flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE;
			}
			if((*flags & EOF_PRO_GUITAR_NOTE_FLAG_SLAP) && (*flags & EOF_PRO_GUITAR_NOTE_FLAG_POP))
			{	//If the slap and pop flags are both set, clear both
				*flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_SLAP;
				*flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_POP;
			}
		}//If it is pasting into a pro guitar track
	}//If the note is copying from a pro guitar track

	if(	((eof_song->track[sourcetrack]->track_behavior == EOF_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[sourcetrack]->track_format == EOF_LEGACY_TRACK_FORMAT)) &&
		((eof_song->track[desttrack]->track_behavior != EOF_GUITAR_TRACK_BEHAVIOR) || (eof_song->track[desttrack]->track_format != EOF_LEGACY_TRACK_FORMAT)))
	{	//If copying from a legacy guitar track to a non legacy guitar track, erase conflicting flags
		*flags &= ~EOF_GUITAR_NOTE_FLAG_IS_SLIDER;
		*flags &= ~EOF_GUITAR_NOTE_FLAG_GHL_OPEN;
	}

	if((sourcetrack == EOF_TRACK_KEYS) && (desttrack != EOF_TRACK_KEYS))
	{	//If copying from a keys track to a non keys track
		*flags &= ~EOF_GUITAR_NOTE_FLAG_IS_SLIDER;
	}

	if((eof_song->track[sourcetrack]->track_behavior == EOF_DANCE_TRACK_BEHAVIOR) && (eof_song->track[desttrack]->track_behavior != EOF_DANCE_TRACK_BEHAVIOR))
	{	//If the note is copying from a dance track to a non dance track, erase conflicting flags
		*flags &= ~EOF_DANCE_FLAG_LANE_1_MINE;	//Erase the lane 1 mine flag
		*flags &= ~EOF_DANCE_FLAG_LANE_2_MINE;	//Erase the lane 2 mine flag
		*flags &= ~EOF_DANCE_FLAG_LANE_3_MINE;	//Erase the lane 3 mine flag
		*flags &= ~EOF_DANCE_FLAG_LANE_4_MINE;	//Erase the lane 4 mine flag
	}

	if(eof_song->track[desttrack]->track_behavior != EOF_GUITAR_TRACK_BEHAVIOR)
	{	//If the note is pasting into a non 5 lane guitar track, erase legacy HOPO flags
		*flags &= ~EOF_NOTE_FLAG_HOPO;		//Erase the temporary HOPO flag
		*flags &= ~EOF_NOTE_FLAG_F_HOPO;	//Erase the forced HOPO ON flag
		*flags &= ~EOF_NOTE_FLAG_NO_HOPO;	//Erase the forced HOPO OFF flag
	}

	if(eof_song->track[desttrack]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR)
	{	//If the note is pasting into a non drum track, erase drum flags
		if(eof_song->track[sourcetrack]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
		{	//If the note is copying from a drum track, erase all drum flags as they are invalid
			*flags &= ~EOF_DRUM_NOTE_FLAG_Y_HI_HAT_OPEN;	//Erase the open hi hat flag
			*flags &= ~EOF_DRUM_NOTE_FLAG_Y_HI_HAT_PEDAL;	//Erase the pedal controlled hi hat flag
			*flags &= ~EOF_DRUM_NOTE_FLAG_R_RIMSHOT;		//Erase the rim shot flag
			*flags &= ~EOF_DRUM_NOTE_FLAG_Y_SIZZLE;			//Erase the sizzle hi hat flag
			*flags &= ~EOF_DRUM_NOTE_FLAG_Y_CYMBAL;			//Erase the yellow cymbal flag
			*flags &= ~EOF_DRUM_NOTE_FLAG_B_CYMBAL;			//Erase the blue cymbal flag
			*flags &= ~EOF_DRUM_NOTE_FLAG_G_CYMBAL;			//Erase the green cymbal flag
			*flags &= ~EOF_DRUM_NOTE_FLAG_DBASS;			//Erase the double bass flag
			*flags &= ~EOF_DRUM_NOTE_FLAG_Y_COMBO;			//Erase the yellow tom/cymbal combo flag
			*flags &= ~EOF_DRUM_NOTE_FLAG_B_COMBO;			//Erase the blue tom/cymbal combo flag
			*flags &= ~EOF_DRUM_NOTE_FLAG_G_COMBO;			//Erase the green tom/cymbal combo flag
		}
	}
	else
	{	//If it is pasting into a drum track, erase flags that are invalid for drum notes
		*flags &= (~EOF_NOTE_FLAG_CRAZY);	//Erase the "crazy" note flag
	}

	if((eof_song->track[sourcetrack]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR) && (desttrack != EOF_TRACK_DRUM_PS))
	{	//If notes are being copied from a drum track and not being pasted into the PS drum track, erase flags exclusive to that track
		*flags &= ~EOF_DRUM_NOTE_FLAG_Y_HI_HAT_OPEN;	//Erase the open hi hat flag
		*flags &= ~EOF_DRUM_NOTE_FLAG_Y_HI_HAT_PEDAL;	//Erase the pedal controlled hi hat flag
		*flags &= ~EOF_DRUM_NOTE_FLAG_R_RIMSHOT;		//Erase the rim shot flag
		*flags &= ~EOF_DRUM_NOTE_FLAG_Y_SIZZLE;			//Erase the sizzle hi hat flag
		*flags &= ~EOF_DRUM_NOTE_FLAG_Y_COMBO;			//Erase the yellow tom/cymbal combo flag
		*flags &= ~EOF_DRUM_NOTE_FLAG_B_COMBO;			//Erase the blue tom/cymbal combo flag
		*flags &= ~EOF_DRUM_NOTE_FLAG_G_COMBO;			//Erase the green tom/cymbal combo flag
	}

	if(eof_song->track[desttrack]->track_behavior == EOF_KEYS_TRACK_BEHAVIOR)
	{	//If pasting into a keys behavior track,
		*flags |= EOF_NOTE_FLAG_CRAZY;	//Set the crazy flag
	}
}

void eof_menu_edit_paste_clear_range(unsigned long track, int note_type, unsigned long start, unsigned long end)
{
	unsigned long i, notepos;
	int type;

	for(i = eof_get_track_size(eof_song, track); i > 0; i--)
	{	//For each note in the specified track
		notepos = eof_get_note_pos(eof_song, track, i - 1);
		type = eof_get_note_type(eof_song, track, i - 1);

		if((type == note_type) && ((notepos <= end) && (notepos >= start)))
		{	//If the note is in the target difficulty, begins at or before the specified end position and the note ends at or after the specified start position
			eof_track_delete_note(eof_song, track, i - 1);	//Delete the note
		}
	}
}

void eof_read_clipboard_note(PACKFILE *fp, EOF_EXTENDED_NOTE *temp_note, unsigned long namelength)
{
	if(!fp || !temp_note)
		return;

	/* read the note */
	(void) eof_load_song_string_pf(temp_note->name, fp, (size_t)namelength);	//Read the note's name, up to the specified number of characters
	temp_note->type = pack_getc(fp);		//Read the note's difficulty
	temp_note->note = pack_getc(fp);		//Read the note's bitmask value
	temp_note->accent = pack_getc(fp);		//Read the note's accent bitmask value
	temp_note->beat = pack_igetl(fp);		//Read the beat the note starts in
	temp_note->endbeat = pack_igetl(fp);	//Read the beat the note ends in
	temp_note->pos = pack_igetl(fp);		//Read the note's position relative to within the selection
	temp_note->length = pack_igetl(fp);		//Read the note's length
	temp_note->midi_pos = temp_note->midi_length = 0;	//Initialize these unused variables
	(void) pack_fread(&temp_note->porpos, (long)sizeof(double), fp);	//Read the percent representing the note's start position within a beat
	(void) pack_fread(&temp_note->porendpos, (long)sizeof(double), fp);	//Read the percent representing the note's end position within a beat
	temp_note->flags = pack_igetl(fp);		//Read the note's flags
	temp_note->eflags = pack_igetl(fp);		//Read the note's extended track flags
	temp_note->legacymask = pack_getc(fp);		//Read the note's legacy bitmask
	(void) pack_fread(temp_note->frets, (long)sizeof(temp_note->frets), fp);	//Read the note's fret array
	(void) pack_fread(temp_note->finger, (long)sizeof(temp_note->finger), fp);	//Read the note's finger array
	temp_note->ghost = pack_getc(fp);			//Read the note's ghost bitmask
	temp_note->bendstrength = pack_getc(fp);	//Read the note's bend strength
	temp_note->slideend = pack_getc(fp);		//Read the note's slide end position
	temp_note->unpitchend = pack_getc(fp);		//Read the note's unpitched slide end position
	temp_note->phrasenum = pack_igetl(fp);		//Read the arpeggio/handshape/lyric phrase number the note is in
}

void eof_read_clipboard_position_snap_data(PACKFILE *fp, int *beat, char *gridsnapvalue, unsigned char *gridsnapnum)
{
	if(!fp || !beat || !gridsnapvalue || !gridsnapnum)
		return;	//Invalid parameters

	*beat = pack_igetl(fp);				//Read the beat number
	*gridsnapvalue = pack_getc(fp);		//Read the grid snap setting
	*gridsnapnum = pack_getc(fp);		//Read the grid snap number
}

void eof_write_clipboard_note(PACKFILE *fp, EOF_SONG *sp, unsigned long track, unsigned long note, unsigned long first_pos)
{
	long note_len;
	double tfloat;
	unsigned char frets[8] = {0};	//Used to store NULL fret data to support copying legacy notes to a pro guitar track
	unsigned char finger[8] = {0};	//Used to store NULL finger data to support copying legacy notes to a pro guitar track
	unsigned long tracknum, ctr;

	if(!fp || !sp || (track >= sp->tracks) || (note >= eof_get_track_size(sp, track)) || !track)
		return;	//Invalid parameters

	tracknum = sp->track[track]->tracknum;

	/* write the note */
	(void) eof_save_song_string_pf(eof_get_note_name(sp, track, note), fp);		//Write the note's name
	(void) pack_putc(eof_get_note_type(sp, track, note), fp);					//Write the note's difficulty
	(void) pack_putc(eof_get_note_note(sp, track, note), fp);					//Write the note bitmask value
	(void) pack_putc(eof_get_note_accent(sp, track, note), fp);					//Write the note accent bitmask value
	note_len = eof_get_note_length(sp, track, note);
	(void) pack_iputl(eof_get_beat(sp, eof_get_note_pos(sp, track, note)), fp);	//Write the beat the note starts in
	(void) pack_iputl(eof_get_beat(sp, eof_get_note_pos(sp, track, note) + note_len), fp);	//Write the beat the note ends in
	(void) pack_iputl(eof_get_note_pos(sp, track, note) - first_pos, fp);		//Write the note's position relative to within the selection
	(void) pack_iputl(note_len, fp);	//Write the note's length
	tfloat = eof_get_porpos(eof_get_note_pos(sp, track, note));
	(void) pack_fwrite(&tfloat, (long)sizeof(double), fp);	//Write the percent representing the note's start position within a beat
	tfloat = eof_get_porpos(eof_get_note_pos(sp, track, note) + note_len);
	(void) pack_fwrite(&tfloat, (long)sizeof(double), fp);	//Write the percent representing the note's end position within a beat
	(void) pack_iputl(eof_get_note_flags(sp, track, note), fp);		//Write the note's flags
	(void) pack_iputl(eof_get_note_eflags(sp, track, note), fp);	//Write the extended flags

	/* Write pro guitar specific data to disk */
	if(sp->track[track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
	{	//If this is a pro guitar note
		unsigned long arpeggnum = 0xFFFFFFFF;
		EOF_PRO_GUITAR_TRACK *tp = sp->pro_guitar_track[tracknum];
		EOF_PRO_GUITAR_NOTE *np = tp->note[note];	//Simplify

		for(ctr = 0; ctr < tp->arpeggios; ctr++)
		{	//For each apreggio/handshape phrase in this track
			if((np->pos >= tp->arpeggio[ctr].start_pos) && (np->pos <= tp->arpeggio[ctr].end_pos))
			{	//If the note being written to the clipboard is inside this phrase
				arpeggnum = ctr;	//Record this phrase number
				break;
			}
		}
		(void) pack_putc(np->legacymask, fp);					//Write the pro guitar note's legacy bitmask
		(void) pack_fwrite(np->frets, (long)sizeof(frets), fp);	//Write the note's fret array
		(void) pack_fwrite(np->finger, (long)sizeof(finger), fp);//Write the note's finger array
		(void) pack_putc(np->ghost, fp);						//Write the note's ghost bitmask
		(void) pack_putc(np->bendstrength, fp);					//Write the note's bend strength
		(void) pack_putc(np->slideend, fp);						//Write the note's slide end position
		(void) pack_putc(np->unpitchend, fp);					//Write the note's unpitched slide end position
		(void) pack_iputl(arpeggnum, fp);						//Write the arpeggio/handshape number the phrase is in
	}
	else
	{
		unsigned long linenum = 0xFFFFFFFF;	//If a lyric is being copied to the clipboard, and it is in a lyric line, this number will change to reflect that line number

		if(sp->track[track]->track_format == EOF_VOCAL_TRACK_FORMAT)
		{	//If this is a lyric, obtain lyric line data to be written to the clipboard
			EOF_VOCAL_TRACK *tp = eof_song->vocal_track[tracknum];
			unsigned long pos = eof_get_note_pos(eof_song, EOF_TRACK_VOCALS, note);

			for(ctr = 0; ctr < tp->lines; ctr++)
			{	//For each lyric line in this track
				if((pos >= tp->line[ctr].start_pos) && (pos <= tp->line[ctr].end_pos))
				{	//If the lyric being written to the clipboard is inside the lyric line
					linenum = ctr;	//Record this lyric line number
					break;
				}
			}
		}

		(void) pack_putc(0, fp);			//Write a legacy bitmask indicating that the original note bitmask is to be used
		(void) pack_fwrite(frets, (long)sizeof(frets), fp);	//Write 0 data for the note's fret array (legacy notes pasted into a pro guitar track will be played open by default)
		(void) pack_fwrite(finger, (long)sizeof(finger), fp);	//Write 0 data for the note's finger array (legacy notes pasted into a pro guitar track will have no fingering by default)
		(void) pack_putc(0, fp);			//Write a blank ghost bitmask (no strings are ghosted by default)
		(void) pack_putc(0, fp);			//Write a blank bend strength
		(void) pack_putc(0, fp);			//Write a blank slide end position
		(void) pack_putc(0, fp);			//Write a blank unpitched slide end position
		(void) pack_iputl(linenum, fp);		//Write the lyric line number associated with this lyric, if any
	}
}

void eof_write_clipboard_position_snap_data(PACKFILE *fp, unsigned long pos)
{
	char gridsnapvalue = 0;
	unsigned char gridsnapnum = 0;
	long beat = 0;

	if(!fp)
		return;	//Invalid parameters

	(void) eof_is_any_grid_snap_position(pos, &beat, &gridsnapvalue, &gridsnapnum, NULL);	//Determine grid snap position, if any, of the specified position
	(void) pack_iputl(beat, fp);			//Write the beat number
	(void) pack_putc(gridsnapvalue, fp);	//Write the grid snap setting
	(void) pack_putc(gridsnapnum, fp);		//Write the grid snap number
}

unsigned long eof_prepare_note_flag_merge(unsigned long flags, unsigned long track_behavior, unsigned long notemask)
{
	if(notemask & 1)
	{	//If the note being pasted uses lane 1, erase lane 1 flags from the overlapped note
		if(track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
		{	//Erase drum specific flags
			flags &= ~EOF_DRUM_NOTE_FLAG_DBASS;
		}
		else if(track_behavior == EOF_DANCE_TRACK_BEHAVIOR)
		{
			flags &= ~EOF_DANCE_FLAG_LANE_1_MINE;
		}
	}
	if(notemask & 2)
	{	//If the note being pasted uses lane 2, erase lane 2 flags from the overlapped note
		if(track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
		{	//Erase drum specific flags
			flags &= ~EOF_DRUM_NOTE_FLAG_R_RIMSHOT;
		}
		else if(track_behavior == EOF_DANCE_TRACK_BEHAVIOR)
		{
			flags &= ~EOF_DANCE_FLAG_LANE_2_MINE;
		}
	}
	if(notemask & 4)
	{	//If the note being pasted uses lane 3, erase lane 3 flags from the overlapped note (remove yellow hi hat statuses if a red gem is present)
		if(track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
		{	//Erase drum specific flags
			flags &= ~EOF_DRUM_NOTE_FLAG_Y_CYMBAL;
			flags &= ~EOF_DRUM_NOTE_FLAG_Y_HI_HAT_OPEN;
			flags &= ~EOF_DRUM_NOTE_FLAG_Y_HI_HAT_PEDAL;
			flags &= ~EOF_DRUM_NOTE_FLAG_Y_SIZZLE;
			flags &= ~EOF_DRUM_NOTE_FLAG_Y_COMBO;
		}
		else if(track_behavior == EOF_DANCE_TRACK_BEHAVIOR)
		{
			flags &= ~EOF_DANCE_FLAG_LANE_3_MINE;
		}
	}
	if(notemask & 8)
	{	//If the note being pasted uses lane 4, erase lane 4 flags from the overlapped note
		if(track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
		{	//Erase drum specific flags
			flags &= ~EOF_DRUM_NOTE_FLAG_B_CYMBAL;
			flags &= ~EOF_DRUM_NOTE_FLAG_B_COMBO;
		}
		else if(track_behavior == EOF_DANCE_TRACK_BEHAVIOR)
		{
			flags &= ~EOF_DANCE_FLAG_LANE_4_MINE;
		}
	}
	if(notemask & 16)
	{	//If the note being pasted uses lane 5, erase lane 5 flags from the overlapped note
		if(track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
		{	//Erase drum specific flags
			flags &= ~EOF_DRUM_NOTE_FLAG_G_CYMBAL;
			flags &= ~EOF_DRUM_NOTE_FLAG_G_COMBO;
		}
	}
	return flags;
}

DIALOG eof_menu_song_paste_from_difficulty_dialog[] =
{
	/* (proc)            (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)            (dp2) (dp3) */
	{ d_agup_window_proc,0,   48,  250, 237, 2,   23,  0,    0,      0,   0,   "Copy content from diff #", NULL, NULL },
	{ d_agup_list_proc,  12,  84,  226, 138, 2,   23,  0,    0,      0,   0,   (void *)eof_menu_song_paste_from_difficulty_list,NULL, NULL },
	{ d_agup_button_proc,12,  245, 90,  28,  2,   23,  'c', D_EXIT,  0,   0,   "&Copy",         NULL, NULL },
	{ d_agup_button_proc,148, 245, 90,  28,  2,   23,  0,   D_EXIT,  0,   0,   "Cancel",        NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

char * eof_menu_song_paste_from_difficulty_list(int index, int * size)
{
	unsigned long ctr, diffcount = 0;

	(void) eof_detect_difficulties(eof_song, eof_selected_track);
	for(ctr = 0; ctr < 256; ctr++)
	{	//For each possible difficulty
		if(eof_track_diff_populated_status[ctr] && (ctr != eof_note_type))
		{	//If this difficulty is populated and isn't the active difficulty
			diffcount++;	//Increment counter
		}
	}

	switch(index)
	{
		case -1:
		{
			if(size)
				*size = diffcount;
			break;
		}
		default:
		{
			return eof_menu_song_difficulty_list_strings[index];
		}
	}
	return NULL;
}

int eof_menu_song_paste_from_difficulty(void)
{
	unsigned long ctr, target, diffcount = 0;
	char undo_made = 0;

	if(!eof_song_loaded || !eof_song)
		return 1;	//Do not allow this function to run if a chart is not loaded

	(void) eof_detect_difficulties(eof_song, eof_selected_track);
	for(ctr = 0; ctr < 256; ctr++)
	{	//For each possible difficulty
		if(eof_track_diff_populated_status[ctr] && (ctr != eof_note_type))
		{	//If this difficulty is populated and isn't the active difficulty, build its list box display string
			(void) snprintf(eof_menu_song_difficulty_list_strings[diffcount], sizeof(eof_menu_song_difficulty_list_strings[0]) - 1, "%lu", ctr);
			diffcount++;	//Increment counter
		}
	}
	if(diffcount == 0)
	{
		allegro_message("No other difficulties in this track contain notes.");
		return 1;
	}

	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_menu_song_paste_from_difficulty_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_menu_song_paste_from_difficulty_dialog);
	if(eof_popup_dialog(eof_menu_song_paste_from_difficulty_dialog, 1) == 2)
	{	//User clicked Copy
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		target = atol(eof_menu_song_difficulty_list_strings[eof_menu_song_paste_from_difficulty_dialog[1].d1]);

		(void) eof_menu_edit_paste_from_difficulty(target, &undo_made);
	}

	eof_beat_stats_cached = 0;	//Have the beat statistics rebuilt
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(NULL);
	return 1;
}

DIALOG eof_menu_edit_select_note_number_in_sequence_logic_dialog[] =
{
	/* (proc)                (x)  (y)  (w)  (h)  (fg)  (bg) (key) (flags) (d1) (d2) (dp)         (dp2)         (dp3) */
	{ d_agup_shadow_box_proc,32,  68,  188, 130, 2,    23,  0,    0,       0,   0,   NULL,       NULL,         NULL },
	{ d_agup_text_proc,      56,  84,  64,  8,   2,    23,  0,    0,       0,   0,   eof_etext, NULL,         NULL },
	{ eof_verified_edit_proc,160, 80,  28,  20,  2,    23,  0,    0,       3,   0,   eof_etext2, "0123456789", NULL },
	{ d_agup_text_proc,      56,  108, 64,  8,   2,    23,  0,    0,       0,   0,   "Out of every:", NULL,         NULL },
	{ eof_verified_edit_proc,160, 104, 28,  20,  2,    23,  0,    0,       3,   0,   eof_etext3, "0123456789", NULL },
	{ d_agup_text_proc,      56,  132, 64,  8,   2,    23,  0,    0,       0,   0,   eof_etext4, NULL,         NULL },
	{ d_agup_button_proc,    48,  156, 68,  28,  2,    23,  '\r', D_EXIT,  0,   0,   "OK",       NULL,         NULL },
	{ d_agup_button_proc,    136, 156, 68,  28,  2,    23,  0,    D_EXIT,  0,   0,   "Cancel",   NULL,         NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

int eof_menu_edit_select_note_number_in_sequence_logic(int function)
{
	int val1, val2, ctr;
	unsigned long i;

	if(!eof_song_loaded || !eof_song)
		return 1;	//Do not allow this function to run if a chart is not loaded
	if(!function && (eof_selection.track != eof_selected_track))
		return 1;	//No notes in the active track are selected so none can become deselected

	eof_color_dialog(eof_menu_edit_select_note_number_in_sequence_logic_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_menu_edit_select_note_number_in_sequence_logic_dialog);

	if(!function)
	{
		strncpy(eof_etext, "Deselect note #:", sizeof(eof_etext) - 1);
		strncpy(eof_etext4, "selected notes", sizeof(eof_etext4) - 1);
	}
	else
	{
		strncpy(eof_etext, "Select note #:", sizeof(eof_etext) - 1);
		strncpy(eof_etext4, "notes", sizeof(eof_etext4) - 1);
	}
	eof_etext2[0] = '\0';	//Empty this field
	eof_etext3[0] = '\0';	//Empty this field

	if(eof_popup_dialog(eof_menu_edit_select_note_number_in_sequence_logic_dialog, 2) != 6)
		return 1;	//If the user did not click OK, return immediately

	val1 = atoi(eof_etext2);
	val2 = atoi(eof_etext3);
	if((val1 < 1) || (val2 < 1))
		return 1;	//If the user did not enter valid values, return immediately

	for(i = 0, ctr = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if(!function)
		{	//Perform deselection
			if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
			{	//If the note is selected
				ctr++;	//Keep track of which note number in the sequence this is
				if(ctr == val1)
				{	//If this is the note the user wanted to deselect
					eof_selection.multi[i] = 0;	//Deselect it
				}
				if(ctr == val2)
				{	//If the counter resets after this note
					ctr = 0;
				}
			}
		}
		else
		{	//Perform selection
				ctr++;	//Keep track of which note number in the sequence this is
				if(ctr == val1)
				{	//If this is the note the user wanted to deselect
					if(eof_selection.track != eof_selected_track)
					{	//If no notes in the current track are currently selected
						(void) eof_menu_edit_deselect_all();		//Clear the selection variables if necessary
						eof_selection.track = eof_selected_track;
					}
					eof_selection.multi[i] = 1;	//Select it
				}
				if(ctr == val2)
				{	//If the counter resets after this note
					ctr = 0;
				}
		}
	}//For each note in the active track

	if(eof_selection.current != EOF_MAX_NOTES - 1)
	{	//If there was a last selected note
		if(eof_selection.multi[eof_selection.current] == 0)
		{	//And it's not selected anymore
			eof_selection.current = EOF_MAX_NOTES - 1;	//Clear the selected note
		}
	}

	return 1;
}

int eof_menu_edit_select_note_number_in_sequence(void)
{
	return eof_menu_edit_select_note_number_in_sequence_logic(1);
}

int eof_menu_edit_deselect_note_number_in_sequence(void)
{
	return eof_menu_edit_select_note_number_in_sequence_logic(0);
}

int eof_menu_edit_select_on_or_off_beat_note_logic(int function, int position)
{
	unsigned long ctr, ctr2, notepos;
	char match;

	if(!eof_song_loaded || !eof_song)
		return 1;	//Do not allow this function to run if a chart is not loaded
	if(!function && (eof_selection.track != eof_selected_track))
		return 1;	//No notes in the active track are selected so none can become deselected

	for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
	{	//For each note in the track
		if(eof_get_note_type(eof_song, eof_selected_track, ctr) != eof_note_type)
			continue;	//If the note is not in the active track difficulty, skip it

		match = 0;	//Reset this condition
		for(ctr2 = 0; ctr2 < eof_song->beats; ctr2++)
		{	//For each beat in the project
			notepos = eof_get_note_pos(eof_song, eof_selected_track, ctr);
			if(notepos < eof_song->beat[ctr2]->pos)
			{	//If this beat (and all remaining beats) are past the note
				break;
			}
			if(notepos == eof_song->beat[ctr2]->pos)
			{	//If this note is on a beat marker
				match = 1;
				break;
			}
		}
		if(match)
		{	//If the note was on a beat marker
			if(position)
			{	//If on-beat notes are being examined
				if(!function)
				{	//If the calling function wanted to deselect on-beat notes
					eof_selection.multi[ctr] = 0;	//Deselect it
				}
				else
				{	//If the calling function wanted to select on-beat notes
					if(eof_selection.track != eof_selected_track)
					{	//If no notes in the current track are currently selected
						(void) eof_menu_edit_deselect_all();		//Clear the selection variables if necessary
						eof_selection.track = eof_selected_track;
					}
					eof_selection.multi[ctr] = 1;
				}
			}
		}
		else
		{	//The note was not on a beat marker
			if(!position)
			{	//If off-beat notes are being examined
				if(!function)
				{	//If the calling function wanted to deselect off-beat notes
					eof_selection.multi[ctr] = 0;	//Deselect it
				}
				else
				{	//If the calling function wanted to select off-beat notes
					if(eof_selection.track != eof_selected_track)
					{	//If no notes in the current track are currently selected
						(void) eof_menu_edit_deselect_all();		//Clear the selection variables if necessary
						eof_selection.track = eof_selected_track;
					}
					eof_selection.multi[ctr] = 1;
				}
			}
		}
	}//For each note in the track
	if(eof_selection.current != EOF_MAX_NOTES - 1)
	{	//If there was a last selected note
		if(eof_selection.multi[eof_selection.current] == 0)
		{	//And it's not selected anymore
			eof_selection.current = EOF_MAX_NOTES - 1;	//Clear the selected note
		}
	}

	return 1;
}

int eof_menu_edit_select_on_beat_notes(void)
{
	return eof_menu_edit_select_on_or_off_beat_note_logic(1, 1);
}

int eof_menu_edit_select_off_beat_notes(void)
{
	return eof_menu_edit_select_on_or_off_beat_note_logic(1, 0);
}

int eof_menu_edit_deselect_on_beat_notes(void)
{
	return eof_menu_edit_select_on_or_off_beat_note_logic(0, 1);
}

int eof_menu_edit_deselect_off_beat_notes(void)
{
	return eof_menu_edit_select_on_or_off_beat_note_logic(0, 0);
}

int eof_menu_edit_set_start_point(void)
{
	unsigned long temp;

	if(!eof_song)
		return 0;

	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	if(eof_song->tags->start_point == eof_music_pos - eof_av_delay)
	{	//If the start point is already set to the current seek position
		eof_song->tags->start_point = ULONG_MAX;	//Clear the start point
	}
	else
	{	//Otherwise update it to the current seek position
		eof_song->tags->start_point = eof_music_pos - eof_av_delay;
		if((eof_song->tags->end_point != ULONG_MAX) && (eof_song->tags->start_point > eof_song->tags->end_point))
		{	//If the start and end points are defined out of order, swap them
			temp =eof_song->tags-> end_point;
			eof_song->tags->end_point = eof_song->tags->start_point;
			eof_song->tags->start_point = temp;
		}
	}
	return 1;
}

int eof_menu_edit_set_end_point(void)
{
	unsigned long temp;

	if(!eof_song)
		return 0;

	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	if(eof_song->tags->end_point == eof_music_pos - eof_av_delay)
	{	//If the end point is already set to the current seek position
		eof_song->tags->end_point = ULONG_MAX;	//Clear the start point
	}
	else
	{	//Otherwise update it to the current seek position
		eof_song->tags->end_point = eof_music_pos - eof_av_delay;
		if((eof_song->tags->start_point != ULONG_MAX) && (eof_song->tags->start_point > eof_song->tags->end_point))
		{	//If the start and end points are defined out of order, swap them
			temp = eof_song->tags->end_point;
			eof_song->tags->end_point = eof_song->tags->start_point;
			eof_song->tags->start_point = temp;
		}
	}
	return 1;
}

static void eof_benchmark_rubberband_callback(void *buffer, int nsamples, int stereo)
{
	int i;
	unsigned short * buffer_sp = (unsigned short *)buffer;
	static unsigned long progress = 0, samples_processed = 0;
	unsigned new_progress;
	char windowtitle[101];

	//If calling function passed a NULL pointer, it is a signal to reset the internal counters
	if(!buffer)
	{
		progress = samples_processed = 0;
		return;
	}

	//Stop processing if user cancels
	if(key[KEY_ESC])
		eof_benchmark_rubberband_cancel = 1;
	if(eof_benchmark_rubberband_cancel)
		return;

	//Convert samples to signed floating point format
	if(stereo)
	{
		int halfnsamples = nsamples / 2;
		for(i = 0; i < halfnsamples; i++)
		{
			eof_benchmark_rubberband_buffer[0][i] = (float)((long)buffer_sp[i * 2] - 0x8000) / (float)0x8000;
			eof_benchmark_rubberband_buffer[1][i] = (float)((long)buffer_sp[i * 2 + 1] - 0x8000) / (float)0x8000;
		}
	}
	else
	{
		for(i = 0; i < nsamples; i++)
		{
			eof_benchmark_rubberband_buffer[0][i] = (float)((long)buffer_sp[i] - 0x8000) / (float)0x8000;
		}
	}

	//Process with rubberband library
	rubberband_process(eof_benchmark_rubberband_state, (const float **)eof_benchmark_rubberband_buffer, nsamples, 0);

	while(rubberband_available(eof_benchmark_rubberband_state) >= EOF_BENCHMARK_RUBBERBAND_BUFFSIZE)
	{
		rubberband_retrieve(eof_benchmark_rubberband_state, eof_benchmark_rubberband_buffer, EOF_BENCHMARK_RUBBERBAND_BUFFSIZE);
	}

	//Report progress
	samples_processed += nsamples;
	new_progress = samples_processed * 100 / eof_benchmark_rubberband_sample_count;
	if(new_progress > progress)
	{	//At least 1% closer to completion since last window title update
		snprintf(windowtitle, sizeof(windowtitle) - 1, "Benchmarking rubberband: %u%% - Press Esc to cancel", new_progress);
		set_window_title(windowtitle);
		progress = new_progress;
	}
}

clock_t eof_bechmark_rubberband(ALOGG_OGG *ogg)
{
	clock_t start = 0, end = 0;
	int isstereo;

	if(!ogg)
		return 0;	//Invalid parameter

	isstereo = alogg_get_wave_is_stereo_ogg(ogg);
	eof_benchmark_rubberband_state = rubberband_new(alogg_get_wave_freq_ogg(ogg), (isstereo ? 2 : 1), RubberBandOptionProcessRealTime |  RubberBandOptionThreadingNever, 2.0, 1.0);	//Process half speed playback
	rubberband_set_max_process_size(eof_benchmark_rubberband_state, EOF_BENCHMARK_RUBBERBAND_BUFFSIZE);

	eof_benchmark_rubberband_buffer[0] = malloc(sizeof(float) * EOF_BENCHMARK_RUBBERBAND_BUFFSIZE);
	if(!eof_benchmark_rubberband_buffer[0])
		return 0;	//Memory error

	if(isstereo)
	{
		eof_benchmark_rubberband_buffer[1] = malloc(sizeof(float) * EOF_BENCHMARK_RUBBERBAND_BUFFSIZE);
		if(!eof_benchmark_rubberband_buffer[1])
		{
			free(eof_benchmark_rubberband_buffer[0]);
			return 0;	//Memory error
		}
	}

	start = clock();
	eof_benchmark_rubberband_callback(NULL, 0, 0);	//Reset the callback's progress reporting counters
	(void) alogg_process_ogg(ogg, eof_benchmark_rubberband_callback, EOF_BENCHMARK_RUBBERBAND_BUFFSIZE, 0.0, 0.0);
	end = clock();
	free(eof_benchmark_rubberband_buffer[0]);
	if(isstereo)
		free(eof_benchmark_rubberband_buffer[1]);

	return end - start;
}

int eof_menu_edit_benchmark_rubberband(void)
{
	clock_t duration;
	unsigned long channels;

	if(!eof_music_track || eof_silence_loaded)
		return 1;	//No audio loaded

	eof_log("eof_menu_edit_benchmark_rubberband() entered", 1);
	eof_benchmark_rubberband_cancel = 0;
	channels = alogg_get_wave_is_stereo_ogg(eof_music_track) ? 2 : 1;
	eof_benchmark_rubberband_sample_count = (double)alogg_get_length_msecs_ogg_ul(eof_music_track) * alogg_get_wave_freq_ogg(eof_music_track) * channels / 1000.0;
	set_window_title("Benchmarking rubberband: 0% - Press Esc to cancel");
	duration = eof_bechmark_rubberband(eof_music_track);
	if(!eof_benchmark_rubberband_cancel && duration)
	{	//If the user didn't cancel the benchmark and the benchmark didn't fail
		set_window_title("Benchmarking complete");
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tBenchmarking complete in %.2f seconds:  ~%lu samples in %lu clocks (%f samples per clock)", (double)duration / CLOCKS_PER_SEC, eof_benchmark_rubberband_sample_count, (unsigned long) duration, (double)eof_benchmark_rubberband_sample_count / duration);
		eof_log(eof_log_string, 1);
		allegro_message("%s", eof_log_string);
	}
	else
	{
		eof_log("\tBenchmarking canceled.", 1);
	}

	eof_fix_window_title();

	return 1;
}
