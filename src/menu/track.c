#include <allegro.h>
#include <ctype.h>
#include "../agup/agup.h"
#include "../dialog/proc.h"
#include "../foflc/Lyric_storage.h"
#include "../dialog.h"
#include "../ir.h"
#include "../main.h"
#include "../midi.h"
#include "../midi_import.h"
#include "../mix.h"
#include "../pathing.h"
#include "../player.h"
#include "../rs.h"
#include "../song.h"
#include "../tuning.h"
#include "../undo.h"
#include "edit.h"
#include "main.h"
#include "note.h"
#include "song.h"
#include "track.h"

#ifdef USEMEMWATCH
#include "../memwatch.h"
#endif

MENU eof_track_phaseshift_menu[] =
{
	{"Enable &Open strum", eof_menu_track_open_strum, NULL, 0, NULL},
	{"Enable &Five lane drums", eof_menu_track_five_lane_drums, NULL, 0, NULL},
	{"Unshare drum phrasing", eof_menu_track_unshare_drum_phrasing, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};


MENU eof_menu_thin_diff_menu[EOF_TRACKS_MAX] =
{
	{eof_menu_track_names[0], eof_menu_thin_notes_track_1, NULL, D_SELECTED, NULL},
	{eof_menu_track_names[1], eof_menu_thin_notes_track_2, NULL, 0, NULL},
	{eof_menu_track_names[2], eof_menu_thin_notes_track_3, NULL, 0, NULL},
	{eof_menu_track_names[3], eof_menu_thin_notes_track_4, NULL, 0, NULL},
	{eof_menu_track_names[4], eof_menu_thin_notes_track_5, NULL, 0, NULL},
	{eof_menu_track_names[5], eof_menu_thin_notes_track_6, NULL, 0, NULL},
	{eof_menu_track_names[6], eof_menu_thin_notes_track_7, NULL, 0, NULL},
	{eof_menu_track_names[7], eof_menu_thin_notes_track_8, NULL, 0, NULL},
	{eof_menu_track_names[8], eof_menu_thin_notes_track_9, NULL, 0, NULL},
	{eof_menu_track_names[9], eof_menu_thin_notes_track_10, NULL, 0, NULL},
	{eof_menu_track_names[10], eof_menu_thin_notes_track_11, NULL, 0, NULL},
	{eof_menu_track_names[11], eof_menu_thin_notes_track_12, NULL, 0, NULL},
	{eof_menu_track_names[12], eof_menu_thin_notes_track_13, NULL, 0, NULL},
	{eof_menu_track_names[13], eof_menu_thin_notes_track_14, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_menu_track_clone_from_menu[EOF_TRACKS_MAX] =
{
	{eof_menu_track_names[0], eof_menu_track_clone_track_1, NULL, D_SELECTED, NULL},
	{eof_menu_track_names[1], eof_menu_track_clone_track_2, NULL, 0, NULL},
	{eof_menu_track_names[2], eof_menu_track_clone_track_3, NULL, 0, NULL},
	{eof_menu_track_names[3], eof_menu_track_clone_track_4, NULL, 0, NULL},
	{eof_menu_track_names[4], eof_menu_track_clone_track_5, NULL, 0, NULL},
	{eof_menu_track_names[5], eof_menu_track_clone_track_6, NULL, 0, NULL},
	{eof_menu_track_names[6], eof_menu_track_clone_track_7, NULL, D_DISABLED, NULL},
	{eof_menu_track_names[7], eof_menu_track_clone_track_8, NULL, 0, NULL},
	{eof_menu_track_names[8], eof_menu_track_clone_track_9, NULL, 0, NULL},
	{eof_menu_track_names[9], eof_menu_track_clone_track_10, NULL, 0, NULL},
	{eof_menu_track_names[10], eof_menu_track_clone_track_11, NULL, 0, NULL},
	{eof_menu_track_names[11], eof_menu_track_clone_track_12, NULL, 0, NULL},
	{eof_menu_track_names[12], eof_menu_track_clone_track_13, NULL, 0, NULL},
	{eof_menu_track_names[13], eof_menu_track_clone_track_14, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_track_menu_set_difficulty[] =
{
	{"&0", eof_menu_track_set_difficulty_0, NULL, 0, NULL},
	{"&1", eof_menu_track_set_difficulty_1, NULL, 0, NULL},
	{"&2", eof_menu_track_set_difficulty_2, NULL, 0, NULL},
	{"&3", eof_menu_track_set_difficulty_3, NULL, 0, NULL},
	{"&4", eof_menu_track_set_difficulty_4, NULL, 0, NULL},
	{"&5", eof_menu_track_set_difficulty_5, NULL, 0, NULL},
	{"&6", eof_menu_track_set_difficulty_6, NULL, 0, NULL},
	{"&Undefined", eof_menu_track_set_difficulty_none, NULL, 0, NULL},
	{"&Manually set", eof_track_difficulty_dialog, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_track_clone_menu[] =
{
	{"From &Track", NULL, eof_menu_track_clone_from_menu, 0, NULL},
	{"To &Clipboard", eof_menu_track_clone_track_to_clipboard, NULL, 0, NULL},
	{"&From clipboard", eof_menu_track_clone_track_from_clipboard, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_track_drumsrock_menu[] =
{
	{"&Enable Drums Rock", eof_menu_track_drumsrock_enable_drumsrock_export, NULL, 0, NULL},
	{"Drums Rock &Remap notes", eof_menu_track_drumsrock_enable_remap, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_track_beatable_menu[] =
{
	{"&Enable BEATABLE", eof_menu_track_beatable_enable_beatable_export, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_track_immerrock_hand_mode_menu[] =
{
	{"&Chord", eof_track_set_chord_hand_mode_change, NULL, 0, NULL},
	{"&String", eof_track_set_string_hand_mode_change, NULL, 0, NULL},
	{"&Delete effective", eof_track_delete_effective_hand_mode_change, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_track_immerrock_menu[] =
{
	{"&Hand mode", NULL, eof_track_immerrock_hand_mode_menu, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

char eof_note_name_find_next_menu_name[20] = {0};
MENU eof_track_search_menu[] =
{
	{eof_note_name_find_next_menu_name, eof_note_name_find_next, NULL, 0, NULL},
	{"&Search and replace", eof_name_search_replace, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_track_erase_menu[] =
{
	{"Erase &Track", eof_track_erase_track, NULL, 0, NULL},
	{"Erase track &Difficulty", eof_track_erase_track_difficulty, NULL, 0, NULL},
	{"Erase &Highlighting", eof_menu_track_remove_highlighting, NULL, 0, NULL},
	{"Erase note &Names", eof_menu_track_erase_note_names, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_track_menu[] =
{
	{"Pro &Guitar", NULL, eof_track_proguitar_menu, 0, NULL},
	{"&Rocksmith", NULL, eof_track_rocksmith_menu, 0, NULL},
	{"&IMMERROCK", NULL, eof_track_immerrock_menu, 0, NULL},
	{"&Phase Shift", NULL, eof_track_phaseshift_menu, 0, NULL},
	{"&Drums Rock", NULL, eof_track_drumsrock_menu, 0, NULL},
	{"&BEATABLE", NULL, eof_track_beatable_menu, 0, NULL},
	{"Set di&Fficulty", NULL, eof_track_menu_set_difficulty, 0, NULL},
	{"Re&name", eof_track_rename, NULL, 0, NULL},
	{"Disable expert+ bass drum", eof_menu_track_disable_double_bass_drums, NULL, 0, NULL},
	{"&Erase", NULL, eof_track_erase_menu, 0, NULL},
	{"&Thin diff. to match", NULL, eof_menu_thin_diff_menu, 0, NULL},
	{"Delete active difficulty", eof_track_delete_difficulty, NULL, 0, NULL},
	{"Repair grid snap", eof_menu_track_repair_grid_snap, NULL, 0, NULL},
	{"&Clone", NULL, eof_track_clone_menu, 0, NULL},
	{"Enable GHL mode", eof_track_menu_enable_ghl_mode, NULL, 0, NULL},
	{"Find optimal CH star power path", eof_menu_track_find_ch_sp_path, NULL, 0, NULL},
	{"Evaluate CH star power path", eof_menu_track_evaluate_user_ch_sp_path, NULL, 0, NULL},
	{"&Offset", eof_menu_track_offset, NULL, 0, NULL},
	{"Chord snap", eof_menu_track_check_chord_snapping, NULL, 0, NULL},
	{"&Search", NULL, eof_track_search_menu, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_filtered_track_menu[EOF_SCRATCH_MENU_SIZE];	//This will be built by eof_create_filtered_menu() to contain the contents of eof_track_menu[] minus the hidden items

MENU eof_track_rocksmith_arrangement_menu[] =
{
	{"&Undefined", eof_track_rocksmith_arrangement_undefined, NULL, 0, NULL},
	{"&Combo", eof_track_rocksmith_arrangement_combo, NULL, 0, NULL},
	{"&Rhythm", eof_track_rocksmith_arrangement_rhythm, NULL, 0, NULL},
	{"&Lead", eof_track_rocksmith_arrangement_lead, NULL, 0, NULL},
	{"&Bass", eof_track_rocksmith_arrangement_bass, NULL, 0, NULL},
	{"", NULL, NULL, 0, NULL},
	{"&Normal", eof_menu_track_rs_normal_arrangement, NULL, 0, NULL},
	{"B&Onus", eof_menu_track_rs_bonus_arrangement, NULL, 0, NULL},
	{"&Alternate", eof_menu_track_rs_alternate_arrangement, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

void eof_prepare_track_menu(void)
{
	unsigned long i, j, tracknum;
	char clipboard_path[50];
	unsigned diff1, diff2 = 0xFF, multidiff = 0;

	if(eof_song && eof_song_loaded)
	{//If a chart is loaded
		tracknum = eof_song->track[eof_selected_track]->tracknum;

		/* enable pro guitar and rocksmith submenus */
		if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If a pro guitar track is active
			eof_track_menu[0].flags = 0;			//Track>Pro Guitar> submenu
			eof_track_menu[1].flags = 0;			//Track>Rocksmith> submenu
			eof_track_menu[2].flags = 0;			//Track>IMMERROCK> submenu

			if(eof_song->pro_guitar_track[tracknum]->ignore_tuning)
			{
				eof_track_proguitar_menu[3].flags = D_SELECTED;	//Track>Pro Guitar>Ignore tuning/capo
			}
			else
			{
				eof_track_proguitar_menu[3].flags = 0;
			}

			if(eof_song->pro_guitar_track[tracknum]->note == eof_song->pro_guitar_track[tracknum]->technote)
			{	//If tech view is in effect for the active track
				eof_track_rocksmith_menu[0].flags = D_SELECTED;	//Track>Rocksmith>Enable tech view
			}
			else
			{
				eof_track_rocksmith_menu[0].flags = 0;
			}

			if(eof_song->track[eof_selected_track]->flags & EOF_TRACK_FLAG_UNLIMITED_DIFFS)
			{	//If the active track has already had the difficulty limit removed
				eof_track_rocksmith_menu[5].flags = D_SELECTED;	//Track>Rocksmith>Remove difficulty limit
			}
			else
			{
				eof_track_rocksmith_menu[5].flags = 0;
			}

			if(eof_song->pro_guitar_track[tracknum]->arrangement == EOF_BASS_ARRANGEMENT)
			{	//If this track's arrangement type is bass
				if(eof_song->track[eof_selected_track]->flags & EOF_TRACK_FLAG_RS_PICKED_BASS)
				{	//If the track is defined as a picked bass track
					eof_track_rocksmith_menu[11].flags = D_SELECTED;
				}
				else
				{
					eof_track_rocksmith_menu[11].flags = 0;
				}
			}
			else
			{	//Otherwise disable the picked bass option
				eof_track_rocksmith_menu[11].flags = D_DISABLED;
			}

			//Update checkmarks on the arrangement type submenu
			for(i = 0; i < 5; i++)
			{	//For each of the arrangement types
				if(eof_song->pro_guitar_track[tracknum]->arrangement == i)
				{	//If the active track is set to this arrangement type
					eof_track_rocksmith_arrangement_menu[i].flags = D_SELECTED;	//Check it
				}
				else
				{	//Otherwise uncheck it
					eof_track_rocksmith_arrangement_menu[i].flags = 0;
				}
			}

			eof_track_rocksmith_arrangement_menu[6].flags = eof_track_rocksmith_arrangement_menu[7].flags = eof_track_rocksmith_arrangement_menu[8].flags = 0;
			if(eof_song->track[eof_selected_track]->flags & EOF_TRACK_FLAG_RS_BONUS_ARR)
			{	//If the active track has the RS bonus arrangement flag
				eof_track_rocksmith_arrangement_menu[7].flags = D_SELECTED;	//Track>Rocksmith>Arrangement type>Bonus
			}
			else if(eof_song->track[eof_selected_track]->flags & EOF_TRACK_FLAG_RS_ALT_ARR)
			{	//If the active track has the RS alternate arrangement flag
				eof_track_rocksmith_arrangement_menu[8].flags = D_SELECTED;	//Track>Rocksmith>Arrangement type>Alternate
			}
			else
			{	//The active track is a normal RS arrangement
				eof_track_rocksmith_arrangement_menu[6].flags = D_SELECTED;	//Track>Rocksmith>Arrangement type>Normal
			}
		}
		else
		{	//Otherwise disable and hide these menu items
			eof_track_menu[0].flags = D_DISABLED | D_HIDDEN;
			eof_track_menu[1].flags = D_DISABLED | D_HIDDEN;
			eof_track_menu[2].flags = D_DISABLED | D_HIDDEN;
		}

		if(eof_song->track[eof_selected_track]->track_format == EOF_LEGACY_TRACK_FORMAT)
		{	//If a legacy track is active
			eof_track_menu[5].flags = 0;	//Track>BEATABLE>
			if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
				eof_track_menu[5].flags = D_DISABLED | D_HIDDEN;	//Drum tracks aren't supported for BEATABLE export
			else
			{
				if(eof_track_is_beatable_mode(eof_song, eof_selected_track))
					eof_track_beatable_menu[0].flags = D_SELECTED;		//Track>BEATABLE>Enable BEATABLE
				else
					eof_track_beatable_menu[0].flags = 0;
			}
			eof_track_menu[3].flags = 0;	//Track>Phase Shift
		}
		else
		{	//Otherwise hide and disable this menu
			eof_track_menu[3].flags = D_DISABLED | D_HIDDEN;
			eof_track_menu[5].flags = D_DISABLED | D_HIDDEN;
		}

		if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
		{	//If a drum track is active
			eof_track_phaseshift_menu[1].flags = 0;	//Track>Phase Shift>Enable five lane drums
			if(eof_five_lane_drums_enabled())
				eof_track_phaseshift_menu[1].flags = D_SELECTED;	//Track>Phase Shift>Enable five lane drums
			eof_track_phaseshift_menu[2].flags = 0;	//Track>Phase Shift>Unshare drum phrasing
			if(eof_song->tags->unshare_drum_phrasing)
				eof_track_phaseshift_menu[2].flags = D_SELECTED;	//Track>Phase Shift>Unshare drum phrasing
			eof_track_menu[4].flags = 0;	//Track>Drums Rock> submenu
			if(eof_song->track[eof_selected_track]->flags & EOF_TRACK_FLAG_DRUMS_ROCK)
			{	//If Drums Rock export is enabled for this track
				eof_track_drumsrock_menu[0].flags = D_SELECTED;
			}
			else
			{
				eof_track_drumsrock_menu[0].flags = 0;
			}
			if(eof_song->track[eof_selected_track]->flags & EOF_TRACK_FLAG_DRUMS_ROCK_REMAP)
			{	//If Drums Rock remapping export is enabled for this track
				eof_track_drumsrock_menu[1].flags = D_SELECTED;
			}
			else
			{
				eof_track_drumsrock_menu[1].flags = 0;
			}
		}
		else
		{
			eof_track_phaseshift_menu[1].flags = D_DISABLED;
			eof_track_phaseshift_menu[2].flags = D_DISABLED;
			eof_track_menu[4].flags = D_DISABLED | D_HIDDEN;
		}

		/* enable open strum */
		if(eof_open_strum_enabled(eof_selected_track))
		{
			eof_track_phaseshift_menu[0].flags = D_SELECTED;	//Track>Phase Shift>Enable open strum
		}
		else
		{
			if(!eof_track_is_legacy_guitar(eof_song, eof_selected_track))
			{	//If a legacy guitar track isn't active
				eof_track_phaseshift_menu[0].flags = D_DISABLED;
			}
			else
			{
				eof_track_phaseshift_menu[0].flags = 0;
			}
		}

		/* enable GHL mode */
		if(eof_track_is_legacy_guitar(eof_song, eof_selected_track))
		{	//If the active track is a legacy guitar/bass track
			if(eof_track_is_ghl_mode(eof_song, eof_selected_track))
			{	//If GHL mode is enabled for the active track
				eof_track_phaseshift_menu[0].flags |= D_DISABLED;	//Prevent user from disabling the open strum option
				eof_track_menu[14].flags = D_SELECTED;				//Track>Enable GHL mode
			}
			else
			{
				eof_track_menu[14].flags = 0;
			}
		}
		else
		{	//Otherwise disable and hide this item
			eof_track_menu[14].flags = D_DISABLED | D_HIDDEN;			//Track>Enable GHL mode
		}

		/* (Clone Hero pathing functions) */
		if(eof_track_is_legacy_guitar(eof_song, eof_selected_track) || (eof_selected_track == EOF_TRACK_KEYS) || (eof_selected_track == EOF_TRACK_DRUM))
		{	//If the active track is a legacy guitar/bass track or the keys track or the normal drum track
			eof_track_menu[15].flags = 0;					//Track>Find optimal CH star power path
			eof_track_menu[16].flags = 0;					//Track>Evaluate CH star power path
		}
		else
		{	//Otherwise disable and hide these items
			eof_track_menu[15].flags = D_DISABLED | D_HIDDEN;			//Track>Find optimal CH star power path
			eof_track_menu[16].flags = D_DISABLED | D_HIDDEN;			//Track>Evaluate CH star power path
		}

		/* Disable expert+ bass drum */
		if(eof_song->tags->double_bass_drum_disabled)
		{
			eof_track_menu[8].flags = D_SELECTED;	//Track>Disable expert+ bass drum
		}
		else
		{
			eof_track_menu[8].flags = 0;
		}

		/* popup messages copy from */
		for(i = 0; i < EOF_TRACKS_MAX; i++)
		{	//For each track supported by EOF
			eof_menu_track_rocksmith_popup_copy_menu[i].flags = 0;

			if(!eof_get_num_popup_messages(eof_song, i + 1) || (i + 1 == eof_selected_track))
			{	//If the track has no popup messages or is the active track
				eof_menu_track_rocksmith_popup_copy_menu[i].flags = D_DISABLED;	//Disable the track from the submenu
			}
		}

		/* tone changes copy from */
		for(i = 0; i < EOF_TRACKS_MAX; i++)
		{	//For each track supported by EOF
			eof_menu_track_rocksmith_tone_change_copy_menu[i].flags = 0;

			if(!eof_get_num_tone_changes(eof_song, i + 1) || (i + 1 == eof_selected_track))
			{	//If the track has no popup messages or is the active track
				eof_menu_track_rocksmith_tone_change_copy_menu[i].flags = D_DISABLED;	//Disable the track from the submenu
			}
		}

		/* fret hand positions copy from */
		for(i = 0; i < EOF_TRACKS_MAX; i++)
		{	//For each track supported by EOF
			eof_track_proguitar_fret_hand_copy_menu[i].flags = 0;

			if(!eof_get_num_fret_hand_positions(eof_song, i + 1) || (i + 1 == eof_selected_track))
			{	//If the track has nofret hand positions or is the active track
				eof_track_proguitar_fret_hand_copy_menu[i].flags = D_DISABLED;	//Disable the track from the submenu
			}
		}

		/* Thin difficulty to match */
		for(i = 0; i < EOF_TRACKS_MAX; i++)
		{	//For each track supported by EOF
			eof_menu_thin_diff_menu[i].flags = D_DISABLED;

			if(i + 1 != eof_selected_track)
			{	//If the track isn't the active track
				for(j = 0; j < eof_get_track_size(eof_song, i + 1); j++)
				{	//For each note in the track
					if(eof_get_note_type(eof_song, i + 1, j) == eof_note_type)
					{	//If the note is in the active track's difficulty
						eof_menu_thin_diff_menu[i].flags = 0;	//Enable the track from the submenu
						break;
					}
				}
			}
		}

		/* Clone from track */
		for(i = 0; i < EOF_TRACKS_MAX - 1; i++)
		{	//For each track supported by EOF
			eof_menu_track_clone_from_menu[i].flags = D_DISABLED;

			if(i + 1 == EOF_TRACK_VOCALS)
				continue;	//This function cannot be used with the vocal track since there isn't a second such track
			if(i + 1 == EOF_TRACK_DANCE)
				continue;	//This function cannot be used with the dance track since there isn't a second such track
			if(i + 1 == eof_selected_track)
				continue;	//This function cannot clone the active track from itself
			if(eof_song->track[eof_selected_track]->track_format != eof_song->track[i + 1]->track_format)
				continue;	//This function can only be used to clone a track of the same format as the active track

			eof_menu_track_clone_from_menu[i].flags = 0;	//Enable the track from the submenu
		}

		/* Clone from clipboard */
		(void) snprintf(clipboard_path, sizeof(clipboard_path) - 1, "%seof.clone.clipboard", eof_temp_path_s);
		if(exists(clipboard_path))
		{	//If the clone clipboard file was found
			eof_track_clone_menu[2].flags = 0;	//Track>Clone>From clipboard
		}
		else
		{
			eof_track_clone_menu[2].flags = D_DISABLED;
		}

		/* Set difficulty */
		for(i = 0; i < 9; i++)
		{	//For each submenu item
			eof_track_menu_set_difficulty[i].flags = 0;	//Clear checkmark
		}
		diff1 = eof_song->track[eof_selected_track]->difficulty;
		if(diff1 > 6)		//Bounds check
			diff1 = 0xFF;
		if((eof_selected_track == EOF_TRACK_DRUM) || (eof_selected_track == EOF_TRACK_VOCALS))
		{	//These tracks have a secondary difficulty level
			diff2 = (eof_song->track[eof_selected_track]->flags & 0x0F000000) >> 24;
			if(diff2 > 6)		//Bounds check
				diff2 = 0xFF;
			multidiff = 1;
		}
		if(multidiff && (diff1 != diff2))
		{	//If the track has two difficulty levels, but they are different values
			eof_track_menu_set_difficulty[8].flags = D_SELECTED;	//Check the manually defined submenu item
		}
		else
		{	//There is only one effective difficulty value
			if(diff1 <= 6)
			{	//If the difficulty level is defined
				eof_track_menu_set_difficulty[diff1].flags = D_SELECTED;	//Check the numbered submenu item
			}
			else
			{	//The difficulty level is undefined
				eof_track_menu_set_difficulty[7].flags = D_SELECTED;	//Check the undefined submenu item
			}
		}

		if(eof_track_is_beatable_mode(eof_song, eof_selected_track))
		{	//If a BEATABLE track is active, several menu items are not applicable
			eof_track_menu[3].flags = D_DISABLED | D_HIDDEN;	//Track>Phase Shift
			eof_track_menu[14].flags = D_DISABLED | D_HIDDEN;	//Track>Enable GHL mode
			eof_track_menu[15].flags = D_DISABLED | D_HIDDEN;	//Track>Find optimal CH star power path
			eof_track_menu[16].flags = D_DISABLED | D_HIDDEN;	//Track>Evaluate CH star power path
		}

		if(eof_create_filtered_menu(eof_track_menu, eof_filtered_track_menu, EOF_SCRATCH_MENU_SIZE))
		{	//If the Track menu was recreated to filter out hidden items
			eof_main_menu[3].child = eof_filtered_track_menu;	//Use this in the main menu
		}
		else
		{	//Otherwise use the unabridged Track menu
			eof_main_menu[3].child = eof_track_menu;
		}

		/* Track>Search>Note name */
		if(eof_song->track[eof_selected_track]->track_format == EOF_VOCAL_TRACK_FORMAT)
		{	//If a vocal track is active
			snprintf(eof_note_name_find_next_menu_name, sizeof(eof_note_name_find_next_menu_name) - 1, "&Lyric text");
			snprintf(eof_note_name_search_dialog_title, sizeof(eof_note_name_search_dialog_title) - 1, "Find next lyric containing this text");
		}
		else
		{
			snprintf(eof_note_name_find_next_menu_name, sizeof(eof_note_name_find_next_menu_name) - 1, "&Note name");
			snprintf(eof_note_name_search_dialog_title, sizeof(eof_note_name_search_dialog_title) - 1, "Find next note containing this name");
		}
	}//If a chart is loaded
}

#define eof_track_difficulty_menu_X 0
#define eof_track_difficulty_menu_Y 48
DIALOG eof_track_difficulty_menu[] =
{
	/* (proc)                (x) (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)           (dp2)    (dp3) */
	{ eof_window_proc,    eof_track_difficulty_menu_X,  eof_track_difficulty_menu_Y,  232, 146, 2,   23,  0,    0,      0,   0,   "Set track difficulty", NULL, NULL },
	{ d_agup_text_proc,      12, 84,  64,  8,   2,   23,  0,    0,      0,   0,   "Difficulty (0-6):",    NULL, NULL },
	{ eof_verified_edit_proc,111,80,  20,  20,  2,   23,  0,    0,      1,   0,   eof_etext,              "0123456", NULL },
	{ d_agup_button_proc,    8,  152, 84,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",                   NULL, NULL },
	{ d_agup_button_proc,    111,152, 78,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",               NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_track_difficulty_menu_pro_drum[] =
{
   { d_agup_text_proc,      12, 104, 114,   8,   2,   23,  0,    0,      0,   0,   "Pro Drum Difficulty (0-6):",    NULL, NULL },
   { eof_verified_edit_proc,174,100,  20,   20,  2,   23,  0,    0,      1,   0,   eof_etext2,              "0123456", NULL },
};

DIALOG eof_track_difficulty_menu_harmony[] =
{
   { d_agup_text_proc,      12, 104, 114,   8,   2,   23,  0,    0,      0,   0,   "Harmony Difficulty (0-6):",    NULL, NULL },
   { eof_verified_edit_proc,170,100,  20,   20,  2,   23,  0,    0,      1,   0,   eof_etext2,              "0123456", NULL },
};

DIALOG eof_track_difficulty_menu_normal[] =
{
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

int eof_menu_track_set_difficulty_0(void)
{
	return eof_menu_track_set_difficulty(0);
}

int eof_menu_track_set_difficulty_1(void)
{
	return eof_menu_track_set_difficulty(1);
}

int eof_menu_track_set_difficulty_2(void)
{
	return eof_menu_track_set_difficulty(2);
}

int eof_menu_track_set_difficulty_3(void)
{
	return eof_menu_track_set_difficulty(3);
}

int eof_menu_track_set_difficulty_4(void)
{
	return eof_menu_track_set_difficulty(4);
}

int eof_menu_track_set_difficulty_5(void)
{
	return eof_menu_track_set_difficulty(5);
}

int eof_menu_track_set_difficulty_6(void)
{
	return eof_menu_track_set_difficulty(6);
}

int eof_menu_track_set_difficulty_none(void)
{
	return eof_menu_track_set_difficulty(0xFF);
}

int eof_menu_track_set_difficulty(unsigned difficulty)
{
	int undo_made = 0;
	unsigned olddiff;
	unsigned diff2 = difficulty & 0xF;	//Secondary difficulties are only stored as a half byte, so undefined is 0xF instead of 0xFF

	if(!eof_song || !eof_song_loaded || !eof_selected_track || (eof_selected_track >= eof_song->tracks))
		return 1;

	if(difficulty != eof_song->track[eof_selected_track]->difficulty)
	{
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		eof_song->track[eof_selected_track]->difficulty = difficulty;
		undo_made = 1;
	}

	//Drum tracks also define a pro drum difficulty and vocal tracks also define a harmony difficulty
	//Set those values if applicable
	if(eof_selected_track == EOF_TRACK_DRUM)
	{
		olddiff = (eof_song->track[EOF_TRACK_DRUM]->flags & 0x0F000000) >> 24;		//Mask out the low nibble of the high order byte of the drum track's flags (pro drum difficulty)
		if(diff2 != olddiff)
		{
			if(!undo_made)
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);

			eof_song->track[EOF_TRACK_DRUM]->flags &= ~(0xFF << 24);	//Clear the drum track's flag's most significant byte
			eof_song->track[EOF_TRACK_DRUM]->flags |= (difficulty << 24);	//Store the pro drum difficulty in the drum track's flag's most significant byte
			eof_song->track[EOF_TRACK_DRUM]->flags |= 0xF0 << 24;		//Set the top nibble to 0xF for backwards compatibility from when this stored the PS drum track difficulty
		}
	}
	else if(eof_selected_track == EOF_TRACK_VOCALS)
	{
		olddiff = (eof_song->track[EOF_TRACK_VOCALS]->flags & 0x0F000000) >> 24;	//Mask out the low nibble of the high order byte of the vocal track's flags (harmony difficulty)
		if(diff2 != olddiff)
		{
			if(!undo_made)
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);

			eof_song->track[EOF_TRACK_VOCALS]->flags &= ~(0xFF << 24);	//Clear the vocal track's flag's most significant byte
			eof_song->track[EOF_TRACK_VOCALS]->flags |= (difficulty << 24);	//Store the harmony difficulty in the vocal track's flag's most significant byte
		}
	}

	return 1;
}

int eof_track_difficulty_dialog(void)
{
	int difficulty, undo_made = 0;
	int difficulty2 = 0xF, newdifficulty2 = 0xF;	//For pro drums and harmony vocals, a half byte (instead of a full byte) is used to store each's difficulty

	if(!eof_song || !eof_song_loaded)
		return 1;
	eof_track_difficulty_menu[5] = eof_track_difficulty_menu_normal[0];
	eof_track_difficulty_menu[6] = eof_track_difficulty_menu_normal[0];

	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_track_difficulty_menu, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_track_difficulty_menu);

	if(eof_selected_track == EOF_TRACK_DRUM)
	{	//Insert the pro drum dialog menu items
		eof_track_difficulty_menu[5] = eof_track_difficulty_menu_pro_drum[0];
		eof_track_difficulty_menu[6] = eof_track_difficulty_menu_pro_drum[1];
		difficulty2 = (eof_song->track[EOF_TRACK_DRUM]->flags & 0x0F000000) >> 24;		//Mask out the low nibble of the high order byte of the drum track's flags (pro drum difficulty)
	}
	else if(eof_selected_track == EOF_TRACK_VOCALS)
	{	//Insert the harmony dialog menu items
		eof_track_difficulty_menu[5] = eof_track_difficulty_menu_harmony[0];
		eof_track_difficulty_menu[6] = eof_track_difficulty_menu_harmony[1];
		difficulty2 = (eof_song->track[EOF_TRACK_VOCALS]->flags & 0x0F000000) >> 24;	//Mask out the low nibble of the high order byte of the vocal track's flags (harmony difficulty)
	}
	if(difficulty2 != 0x0F)
	{	//If the secondary difficulty (pro drum or vocal harmony) is to be displayed
		(void) snprintf(eof_etext2, sizeof(eof_etext2) - 1, "%d", difficulty2);
	}
	else
	{
		eof_etext2[0] = '\0';
	}

	//Manually re-center these elements, because they are not altered by eof_conditionally_center_dialog()
	eof_track_difficulty_menu[5].x += eof_track_difficulty_menu[0].x - eof_track_difficulty_menu_X;	//Add the X amount offset by eof_conditionally_center_dialog()
	eof_track_difficulty_menu[5].y += eof_track_difficulty_menu[0].y - eof_track_difficulty_menu_Y;	//Add the Y amount offset by eof_conditionally_center_dialog()
	eof_track_difficulty_menu[6].x += eof_track_difficulty_menu[0].x - eof_track_difficulty_menu_X;	//Add the X amount offset by eof_conditionally_center_dialog()
	eof_track_difficulty_menu[6].y += eof_track_difficulty_menu[0].y - eof_track_difficulty_menu_Y;	//Add the Y amount offset by eof_conditionally_center_dialog()

	if(eof_song->track[eof_selected_track]->difficulty != 0xFF)
	{	//If the track difficulty is defined, write it in text format
		(void) snprintf(eof_etext, sizeof(eof_etext) - 1, "%u", eof_song->track[eof_selected_track]->difficulty);
	}
	else
	{	//Otherwise prepare an empty string
		eof_etext[0] = '\0';
	}
	if(eof_popup_dialog(eof_track_difficulty_menu, 2) == 3)	//User hit OK
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

			if((newdifficulty2 != difficulty2) && !undo_made)
			{	//If a secondary difficulty has changed, make an undo state if one hasn't been made already
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			}

			if(eof_selected_track == EOF_TRACK_DRUM)
			{
				eof_song->track[EOF_TRACK_DRUM]->flags &= ~(0xFF << 24);			//Clear the drum track's flag's most significant byte
				eof_song->track[EOF_TRACK_DRUM]->flags |= ((unsigned)newdifficulty2 << 24);	//Store the pro drum difficulty in the drum track's flag's most significant byte
				eof_song->track[EOF_TRACK_DRUM]->flags |= 0xF0 << 24;				//Set the top nibble to 0xF for backwards compatibility from when this stored the PS drum track difficulty
			}
			else if(eof_selected_track == EOF_TRACK_VOCALS)
			{
				eof_song->track[EOF_TRACK_VOCALS]->flags &= ~(0xFF << 24);			//Clear the vocal track's flag's most significant byte
				eof_song->track[EOF_TRACK_VOCALS]->flags |= ((unsigned)newdifficulty2 << 24);	//Store the harmony difficulty in the vocal track's flag's most significant byte
			}
		}
	}
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(screen);
	return 1;
}

DIALOG eof_track_rename_dialog[] =
{
	/* (proc)            (x) (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)           (dp2) (dp3) */
	{ eof_window_proc,0,  48,  314, 112, 2,   23,  0,    0,      0,   0,   "Rename track",NULL, NULL },
	{ d_agup_text_proc,  12, 84,  64,  8,   2,   23,  0,    0,      0,   0,   "New name:",   NULL, NULL },
	{ eof_edit_proc,  90, 80,  212, 20,  2,   23,  0, 0,EOF_NAME_LENGTH,0, eof_etext,     NULL, NULL },
	{ d_agup_button_proc,67, 120, 84,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",          NULL, NULL },
	{ d_agup_button_proc,163,120, 78,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",      NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

int eof_track_rename(void)
{
	unsigned long ctr;

	if(!eof_song_loaded || !eof_song)
		return 1;	//Do not allow this function to run if a chart is not loaded

	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_track_rename_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_track_rename_dialog);

	if(eof_song->track[eof_selected_track]->flags & EOF_TRACK_FLAG_ALT_NAME)
	{	//If the track already has an alternate name
		(void) ustrcpy(eof_etext, eof_song->track[eof_selected_track]->altname);	//Copy it into the input field
	}
	else
	{
		eof_etext[0] = '\0';	//Otherwise empty the input field
	}
	if(eof_popup_dialog(eof_track_rename_dialog, 2) == 3)
	{	//If user clicked OK
		if(ustrncmp(eof_etext, eof_song->track[eof_selected_track]->altname, EOF_NAME_LENGTH))
		{	//If the user provided a different alternate track name than what was already defined
			if(eof_etext[0] != '\0')
			{	//If the specified string isn't empty
				//Verify that the provided name doesn't match the existing native or display name of any track in the project
				for(ctr = 1; ctr < eof_song->tracks; ctr++)
				{	//For each track in the project
					if(!ustrnicmp(eof_etext, eof_song->track[ctr]->name, EOF_NAME_LENGTH) || !ustrnicmp(eof_etext, eof_song->track[ctr]->altname, EOF_NAME_LENGTH))
					{	//If the provided name matches the track's native or display name
						allegro_message("Error:  The provided name is already in use by this track:  %s", eof_song->track[ctr]->name);
						return 1;
					}
				}
			}

			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			(void) ustrcpy(eof_song->track[eof_selected_track]->altname, eof_etext);	//Update the track entry
			if(eof_etext[0] != '\0')
			{	//If the alternate name string is not empty
				eof_song->track[eof_selected_track]->flags |= EOF_TRACK_FLAG_ALT_NAME;	//Set this flag
			}
			else
			{	//Otherwise clear the alternate track name flag
				eof_song->track[eof_selected_track]->flags &= ~EOF_TRACK_FLAG_ALT_NAME;	//Clear this flag
			}
		}
	}

	eof_fix_window_title();	//Update EOF's window title
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(NULL);
	return 1;
}

void eof_rebuild_tuning_strings(char *tuningarray)
{
	unsigned long tracknum, ctr, numspaces;
	int tuning, halfsteps;
	char error;

	if(!eof_song || (eof_selected_track >= eof_song->tracks) || (eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT) || !tuningarray)
		return;	//Invalid parameters
	tracknum = eof_song->track[eof_selected_track]->tracknum;

	for(ctr = 0; ctr < EOF_TUNING_LENGTH; ctr++)
	{	//For each usable string in the track
		error = 0;
		if((ctr < eof_song->pro_guitar_track[tracknum]->numstrings) && (eof_fret_strings[ctr][0] != '\0'))
		{	//If this string is used by the track and its tuning field is populated
			halfsteps = atol(eof_fret_strings[ctr]);
			if(!halfsteps && (eof_fret_strings[ctr][0] != '0'))
			{	//If there was some kind of error converting this character to a number
				error = 1;
			}
			else
			{	//Otherwise look up the tuning
				tuning = eof_lookup_tuned_note(eof_song->pro_guitar_track[tracknum], eof_selected_track, ctr, halfsteps);
				if(tuning < 0)
				{	//If there was an error determining the tuning
					error = 1;
				}
				else
				{	//Otherwise update the tuning string
					tuning %= 12;	//Guarantee this value is in the range of [0,11]
					strncpy(tuning_list[ctr], eof_note_names[tuning], sizeof(tuning_list[0]) - 1);
					strncat(tuning_list[ctr], "  ", sizeof(tuning_list[0])-1);	//Pad with a space to ensure old string is overwritten
					tuning_list[ctr][sizeof(tuning_list[0])-1] = '\0';	//Guarantee this string is truncated
				}
			}
		}
		else
		{	//Otherwise empty the string
			error = 1;
		}
		if(error)
		{	//If the tuning string couldn't be built, empty it
			tuning_list[ctr][0] = '\0';
		}
	}

	//Rebuild the tuning name string
	strncpy(eof_tuning_name, eof_lookup_tuning_name(eof_song, eof_selected_track, tuningarray), sizeof(eof_tuning_name)-1);
	numspaces = (unsigned long) (sizeof(eof_tuning_name) - 1 - strlen(eof_tuning_name));	//Determine how many unused characters are in the tuning name array
	for(ctr = 0; ctr < numspaces; ctr++)
	{
		(void) strncat(eof_tuning_name, " ", sizeof(eof_tuning_name) - strlen(eof_tuning_name) - 1);		//Fill in the tuning name array with spaces so the old name is completely overwritten in the dialog
	}
}

void eof_redraw_tuning_dialog(void)
{
	int i;
	char tuning[EOF_TUNING_LENGTH] = {0};

	//Build an integer type tuning array from the current input
	for(i = 0; i < EOF_TUNING_LENGTH; i++)
	{
		tuning[i] = atol(eof_fret_strings[i]) % 12;	//Convert the text input to integer value
	}
	eof_rebuild_tuning_strings(tuning);
	(void) object_message(&eof_pro_guitar_tuning_dialog[2], MSG_DRAW, 0);	//Have Allegro redraw the tuning name
	(void) object_message(&eof_pro_guitar_tuning_dialog[6], MSG_DRAW, 0);	//Have Allegro redraw the string tuning strings
	(void) object_message(&eof_pro_guitar_tuning_dialog[9], MSG_DRAW, 0);	//Have Allegro redraw the string tuning strings
	(void) object_message(&eof_pro_guitar_tuning_dialog[12], MSG_DRAW, 0);	//Have Allegro redraw the string tuning strings
	(void) object_message(&eof_pro_guitar_tuning_dialog[15], MSG_DRAW, 0);	//Have Allegro redraw the string tuning strings
	(void) object_message(&eof_pro_guitar_tuning_dialog[18], MSG_DRAW, 0);	//Have Allegro redraw the string tuning strings
	(void) object_message(&eof_pro_guitar_tuning_dialog[21], MSG_DRAW, 0);	//Have Allegro redraw the string tuning strings
}

int eof_edit_tuning_proc(int msg, DIALOG *d, int c)
{
	int i;
	char * string = NULL;
	unsigned key_list[32] = {KEY_BACKSPACE, KEY_DEL, KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN, KEY_ESC, KEY_ENTER, KEY_TAB};	//A list of scan codes to match against MSG_CHAR events
	int match = 0;
	int retval;
	char tuning, tuningchange = 0;
	unsigned c2 = (unsigned)c;	//Force cast this to unsigned because Splint is incapable of avoiding a false positive detecting it as negative despite assertions proving otherwise

	if(!d)	//If this pointer is NULL for any reason
		return d_agup_edit_proc(msg, d, c);	//Allow the input character to be returned

	if((msg != MSG_CHAR) && (msg != MSG_UCHAR))
	{	//If this isn't a character input message, allow the input character to be returned
		return d_agup_edit_proc(msg, d, c);
	}

	//Check scan code input
	if(msg == MSG_CHAR)
	{
		if(tolower(c2 & 0xFF) == 'p')
		{	//User pressed the P key
///			return d_agup_edit_proc(msg, d, c);
//			int junk = 0;
//			(void) object_message(&eof_pro_guitar_tuning_dialog[23], MSG_CLICK, 0);	//Send click event to the Preset button
//			(void) object_message(&eof_pro_guitar_tuning_dialog[23], D_EXIT, 0);	//Send click event to the Preset button
//			(void) dialog_message(eof_pro_guitar_tuning_dialog, D_EXIT, 0, &junk);
//			return D_EXIT;
		}
		if((c2 >> 8 == KEY_BACKSPACE) || (c2 >> 8 == KEY_DEL))
		{	//If the backspace or delete keys are trapped
			match = 1;	//Ensure the full function runs, so that the strings are rebuilt
		}
		if(c2 >> 8 == KEY_DOWN)
		{	//If the down key is trapped
			tuningchange = -1;	//The tuning value for this field will be decremented
		}
		if(c2 >> 8 == KEY_UP)
		{	//If the up key is trapped
			tuningchange = 1;	//The tuning value for this field will be incremented
		}
		if(tuningchange)
		{	//If the tuning value is to be incremented/decremented
			string = (char *)d->dp;	//Obtain the string of whichever tuning field has focus
			tuning = atol(string);	//Convert the text input to integer value
			tuning = (tuning + tuningchange) % 12;	//Adjust the tuning value accordingly and constrain to a range of 0 through +- 11
			(void) snprintf(string, sizeof(eof_string_lane_1) - 1, "%d", tuning);	//Repopulate this input field with the updated string
			eof_redraw_tuning_dialog();
			return D_REDRAWME;
		}
	}

	//Check ASCII code input
	if(msg == MSG_UCHAR)
	{
		if(c2 == 27)
		{	//If the Escape ASCII character was trapped
			return d_agup_edit_proc(msg, d, c2);	//Immediately allow the input character to be returned (so the user can escape to cancel the dialog)
		}
		if(c2 == 9)
		{	//If the Tab ASCII character was trapped
			return d_agup_edit_proc(msg, d, c2);	//Immediately allow the input character to be returned (so the user can tab to the next input field)
		}
	}

	for(i = 0; !match && (i < 9); i++)
	{	//Check each of the pre-defined allowable keys, or unless above code preempts this checking
		if(c2 >> 8 == key_list[i])			//If the input is permanently allowed
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

	eof_redraw_tuning_dialog();
	return retval;
}

char * eof_tunings_list(int index, int * size)
{
	int ctr, tuning_count = 0;
	EOF_PRO_GUITAR_TRACK *tp;
	int bass_active_track = 0;	//Tracks whether the active track is determined to be a bass arrangement

	if(!eof_song || (eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
	{
		if(index < 0)
			*size = 0;	//Indicate no list entries

		return NULL;	//Active track is not a pro guitar track
	}

	tp = eof_song->pro_guitar_track[eof_song->track[eof_selected_track]->tracknum];
	bass_active_track = eof_track_is_bass_arrangement(tp, eof_selected_track);	//Track whether the active track is a bass arrangement

	switch(index)
	{
		case -1:
		{	//Count the number of tunings applicable to the active track
			if(size)
			{
				for(ctr = 0; ctr < EOF_NUM_TUNING_DEFINITIONS; ctr++)
				{	//For each defined tuning
					if(!eof_tuning_definition_is_applicable(bass_active_track, tp->numstrings, ctr))
					{	//If the tuning definition doesn't apply to the active track
						continue;	//Skip it
					}

					tuning_count++;
				}
				*size = tuning_count;
			}
			break;
		}

		default:
			for(ctr = 0; ctr < EOF_NUM_TUNING_DEFINITIONS; ctr++)
			{	//For each defined tuning
				if(!eof_tuning_definition_is_applicable(bass_active_track, tp->numstrings, ctr))
				{	//If the tuning definition doesn't apply to the active track
					continue;	//Skip it
				}

				if(tuning_count == index)
				{	//If this is the tuning definition whose string was requested
					return eof_tuning_definitions[ctr].descriptive_name;
				}

				tuning_count++;	//The next match will be one index higher
			}
		break;
	}

	return NULL;
}

int eof_tuning_definition_is_applicable(char track_is_bass, unsigned char numstrings, unsigned definition_num)
{
	if(track_is_bass)
	{	//If the active track is a bass arrangement
		if(!eof_tuning_definitions[definition_num].is_bass)
		{	//If the tuning doesn't reflect a bass arrangement
			return 0;	//Not applicable
		}
	}
	else
	{	//The active track is a guitar arrangement
		if(eof_tuning_definitions[definition_num].is_bass)
		{	//If the tuning reflects a bass arrangement instead
			return 0;	//Not applicable
		}
	}
	if(numstrings != eof_tuning_definitions[definition_num].numstrings)
	{	//If the tuning does not reflect the active track's string count
		return 0;	//Not applicable
	}

	return 1;	//This tuning definition applies to the track with the input criteria
}

DIALOG eof_tuning_preset_dialog[] =
{
	/* (proc)                 (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)              (dp2) (dp3) */
	{ eof_shadow_box_proc, 4,   200, 340, 350, 2,   23,  0,    0,      0,   0,   NULL,             NULL, NULL },
	{ d_agup_text_proc,       123, 208, 128, 8,   2,   23,  0,    0,      0,   0,   "Select tuning",   NULL, NULL },
	{ d_agup_list_proc,       16,  228, 306, 276, 2,   23,  0,    0,      0,   0,   (void *)eof_tunings_list, NULL, NULL },
	{ d_agup_button_proc,     16,  510, 68,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",             NULL, NULL },
	{ d_agup_button_proc,     164, 510, 138, 28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",         NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

int eof_tuning_preset(void)
{
	if(!eof_song || (eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
	{	//Don't run this dialog unless a pro guitar track is active
		return 1;	//Return cancellation
	}

	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	eof_color_dialog(eof_tuning_preset_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_tuning_preset_dialog);
	if(eof_popup_dialog(eof_tuning_preset_dialog, 0) == 3)
	{	//User clicked OK
		EOF_PRO_GUITAR_TRACK *tp;
		int bass_active_track;
		unsigned ctr, stringnum, tuning_count = 0;

		tp = eof_song->pro_guitar_track[eof_song->track[eof_selected_track]->tracknum];
		bass_active_track = eof_track_is_bass_arrangement(tp, eof_selected_track);	//Track whether the active track is a bass arrangement

		//Find the selected tuning definition
		for(ctr = 0; ctr < EOF_NUM_TUNING_DEFINITIONS; ctr++)
		{	//For each defined tuning
			if(!eof_tuning_definition_is_applicable(bass_active_track, tp->numstrings, ctr))
			{	//If the tuning definition doesn't apply to the active track
				continue;	//Skip it
			}

			if(tuning_count == eof_tuning_preset_dialog[2].d1)
			{	//If this is the tuning definition that was selected
				for(stringnum = 0; stringnum < tp->numstrings; stringnum++)
				{	//For each string
					(void) snprintf(eof_fret_strings[stringnum], sizeof(eof_string_lane_1) - 1, "%d", eof_tuning_definitions[ctr].tuning[stringnum]);	//Apply the user selected tuning to the edit guitar tuning dialog
				}
				break;
			}

			tuning_count++;	//The next match will be one index higher
		}
	}
	else
	{
		return 1;	//Return cancellation
	}
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;

	return 0;	//Return success
}

DIALOG eof_pro_guitar_tuning_dialog[] =
{
	/*	(proc)				(x)  (y)  (w)  (h) (fg) (bg) (key) (flags) (d1)       (d2) (dp)          		(dp2) (dp3) */
	{ eof_window_proc,	0,   48,  252, 272,0,   0,   0,    0,      0,         0,	"Edit guitar tuning",NULL, NULL },
	{ d_agup_text_proc,  	16,  80,  44,  8,  0,   0,   0,    0,      0,         0,	"Tuning:",      	NULL, NULL },
	{ d_agup_text_proc,		74,  80,  154, 8,  0,   0,   0,    0,      21,        0,	eof_tuning_name,    NULL, NULL },

	//Note:  In guitar theory, string 1 refers to high e
	{ d_agup_text_proc,      16,  108, 64,  12,  0,   0,   0,    0,      0,         0,   "Half steps above/below standard",NULL,NULL },
	{ d_agup_text_proc,      16,  132, 64,  12,  0,   0,   0,    0,      0,         0,   eof_string_lane_6_number,  NULL,          NULL },
	{ eof_edit_tuning_proc,	74,  128, 28,  20,  0,   0,   0,    0,      3,         0,   eof_string_lane_6,  "0123456789-",NULL },
	{ d_agup_text_proc,      110, 132, 28,  12,  0,   0,   0,    0,      0,         0,   string_6_name,NULL,          NULL },
	{ d_agup_text_proc,      16,  156, 64,  12,  0,   0,   0,    0,      0,         0,   eof_string_lane_5_number,  NULL,          NULL },
	{ eof_edit_tuning_proc,	74,  152, 28,  20,  0,   0,   0,    0,      3,         0,   eof_string_lane_5,  "0123456789-",NULL },
	{ d_agup_text_proc,      110, 156, 28,  12,  0,   0,   0,    0,      0,         0,   string_5_name,NULL,          NULL },
	{ d_agup_text_proc,      16,  180, 64,  12,  0,   0,   0,    0,      0,         0,   eof_string_lane_4_number,  NULL,          NULL },
	{ eof_edit_tuning_proc,	74,  176, 28,  20,  0,   0,   0,    0,      3,         0,   eof_string_lane_4,  "0123456789-",NULL },
	{ d_agup_text_proc,      110, 180, 28,  12,  0,   0,   0,    0,      0,         0,   string_4_name,NULL,          NULL },
	{ d_agup_text_proc,      16,  204, 64,  12,  0,   0,   0,    0,      0,         0,   eof_string_lane_3_number,  NULL,          NULL },
	{ eof_edit_tuning_proc,	74,  200, 28,  20,  0,   0,   0,    0,      3,         0,   eof_string_lane_3,  "0123456789-",NULL },
	{ d_agup_text_proc,      110, 204, 28,  12,  0,   0,   0,    0,      0,         0,   string_3_name,NULL,          NULL },
	{ d_agup_text_proc,      16,  228, 64,  12,  0,   0,   0,    0,      0,         0,   eof_string_lane_2_number,  NULL,          NULL },
	{ eof_edit_tuning_proc,	74,  224, 28,  20,  0,   0,   0,    0,      3,         0,   eof_string_lane_2,  "0123456789-",NULL },
	{ d_agup_text_proc,      110, 228, 28,  12,  0,   0,   0,    0,      0,         0,   string_2_name,NULL,          NULL },
	{ d_agup_text_proc,      16,  252, 64,  12,  0,   0,   0,    0,      0,         0,   eof_string_lane_1_number,  NULL,          NULL },
	{ eof_edit_tuning_proc,	74,  248, 28,  20,  0,   0,   0,    0,      3,         0,   eof_string_lane_1,  "0123456789-",NULL },
	{ d_agup_text_proc,      110, 252, 28,  12,  0,   0,   0,    0,      0,         0,   string_1_name,NULL,          NULL },

	{ d_agup_button_proc,    12,  280, 68,  28, 0,   0,   '\r', D_EXIT, 0,         0,   "OK",         NULL,          NULL },
	{ d_agup_button_proc,    92,  280, 68,  28, 0,   0,   'p',    D_EXIT, 0,         0,   "Preset",     NULL,          NULL },
	{ d_agup_button_proc,    172, 280, 68,  28, 0,   0,   0,    D_EXIT, 0,         0,   "Cancel",     NULL,          NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

int eof_track_tuning(void)
{
	unsigned long ctr, tracknum, stringcount;
	char undo_made = 0, newtuning[6] = {0};
	EOF_PRO_GUITAR_TRACK *tp;
	int focus = 5;	//By default, start dialog focus in the tuning box for string 1 (top-most box for 6 string track)
	int retval, cancelled = 0;
	long value;

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run unless the pro guitar track is active
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	tp = eof_song->pro_guitar_track[tracknum];

	if(!eof_music_paused)
	{
		eof_music_play(0);
	}

	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	eof_color_dialog(eof_pro_guitar_tuning_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_pro_guitar_tuning_dialog);

//Update the strings to reflect the currently set tuning
	stringcount = tp->numstrings;
	for(ctr = 0; ctr < 6; ctr++)
	{	//For each of the 6 supported strings
		if(ctr < stringcount)
		{	//If this track uses this string, convert the tuning to its string representation
			(void) snprintf(eof_fret_strings[ctr], sizeof(eof_string_lane_1) - 1, "%d", tp->tuning[ctr]);
			eof_pro_guitar_tuning_dialog[19 - (3 * ctr)].flags = 0;			//Enable the input field's label
			eof_pro_guitar_tuning_dialog[20 - (3 * ctr)].flags = 0;			//Enable the input field for the string
			eof_fret_string_numbers[ctr][7] = '0' + (stringcount - ctr);	//Correct the string number for this label
		}
		else
		{	//Otherwise hide this textbox and its label
			eof_pro_guitar_tuning_dialog[19 - (3 * ctr)].flags = D_HIDDEN;	//Hide the input field's label
			eof_pro_guitar_tuning_dialog[20 - (3 * ctr)].flags = D_HIDDEN;	//Hide the input field for the string
		}
	}
	eof_rebuild_tuning_strings(tp->tuning);

	//Adjust initial focus to account for whichever the top-most string is
	if(stringcount == 5)
		focus = 8;
	else if(stringcount == 4)
		focus = 11;

	retval = eof_popup_dialog(eof_pro_guitar_tuning_dialog, focus);
	if(retval == 22)
	{	//If user clicked OK
		//Validate and store the input
		for(ctr = 0; ctr < tp->numstrings; ctr++)
		{	//For each string in the track, ensure the user entered a tuning
			value = atol(eof_fret_strings[ctr]);

			if(eof_fret_strings[ctr][0] == '\0')
			{	//Ensure the user entered a tuning
				allegro_message("Error:  Each of the track's strings must have a defined tuning");
				return 1;
			}
			if(!value && (eof_fret_strings[ctr][0] != '0'))
			{	//Ensure the tuning is valid (ie. not "-")
				allegro_message("Error:  Invalid tuning value");
				return 1;
			}
			if((value > 24) || (value < -24))
			{	//Ensure the tuning is within range (allow up to two octaves above or below standard)
				allegro_message("Error:  Invalid tuning value (must be between -11 and 11)");
				return 1;
			}
		}
	}
	if(retval == 23)
	{	//If user opted to select a preset
		cancelled = eof_tuning_preset();
	}

	if(!cancelled && ((retval == 22) || (retval == 23)))
	{	//If the user manually edited the tuning or selected a preset (and didn't cancel the dialog)
		for(ctr = 0; ctr < 6; ctr++)
		{	//For each of the 6 supported strings
			value = atol(eof_fret_strings[ctr]);
			if(ctr < tp->numstrings)
			{	//If this string is used in the track, store the numerical value into the track's tuning array
				if(!undo_made && (tp->tuning[ctr] != value % 12))
				{	//If a tuning was changed
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					undo_made = 1;
					eof_chord_lookup_note = 0;	//Reset the cached chord lookup count
				}
				newtuning[ctr] = value - tp->tuning[ctr];	//Find this string's tuning change (in half steps)
				tp->tuning[ctr] = value;
			}
			else
			{	//This string is not used in the track
				tp->tuning[ctr] = 0;
			}
		}
		if(eof_track_is_drop_tuned(tp))
		{	//If the user has given a drop tuning
			if(tp->ignore_tuning)
			{	//If the option to disregard the tuning for chord name lookups is in effect
				if(alert("This appears to be a drop tuning.", "Disable the option to ignore the tuning", "for chord names? (recommended)", "&Yes", "&No", 'y', 'n') == 1)
				{	//If user opts to take the drop tuning into account for chord name lookups
					tp->ignore_tuning = 0;
				}
			}
		}
		(void) eof_track_transpose_tuning(tp, newtuning);	//Offer to transpose any existing notes in the track to the new tuning
	}//If the user manually edited the tuning or selected a preset

	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	return 1;
}

int eof_track_transpose_tuning(EOF_PRO_GUITAR_TRACK* tp, char *tuningdiff)
{
	unsigned long ctr, ctr2, ctr3, bitmask, noteset;
	unsigned char prompt = 0, skiptranspose, warning = 0, first, diffmask = 0;
	char val = 0;
	char restore_tech_view = 0;		//If tech view is in effect, it is temporarily disabled until after the secondary piano roll has been rendered

	if(!tp || !tuningdiff)
		return 0;	//Error

	restore_tech_view = eof_menu_pro_guitar_track_get_tech_view_state(tp);	//Track which note set is in use

	for(noteset = 0; noteset < 2; noteset++)
	{	//For each the normal and the tech note set
		eof_menu_pro_guitar_track_set_tech_view_state(tp, noteset);	//Activate the appropriate note set
		for(ctr = 0; ctr < tp->notes; ctr++)
		{	//For each note in the track
			skiptranspose = 0;	//This will be set to nonzero if the note won't be transposed automatically (in which case it will be highlighted for user action)
			for(ctr2 = 0; ctr2 < 2; ctr2++)
			{	//On the first pass, validate whether the note can be transposed automatically, on second pass, alter the note
				if(!noteset)
				{	//Only check fret values for normal notes, because tech notes do not have any
					for(ctr3 = 0, bitmask = 1; ctr3 < 6; ctr3++, bitmask <<= 1)
					{	//For each of the 6 supported strings
						val = tuningdiff[ctr3];	//Simplify
						if(!tuningdiff[ctr3])
							continue;	//If this string's tuning was not changed, skip it
						if(!(tp->note[ctr]->note & bitmask))
							continue;	//If this string isn't used by this note, skip it

						diffmask |= bitmask;	//Build a bitmask defining which strings are being transposed
						if(!prompt)
						{	//If the user hasn't been prompted yet
							if(alert("This track contains notes that would be affected by the tuning change.", "Would you like to transpose the notes to keep the same pitches where possible?", NULL, "&Yes", "&No", 'y', 'n') != 1)
							{	//If the user doesn't opt to transpose the track
								eof_menu_pro_guitar_track_set_tech_view_state(tp, restore_tech_view);	//Re-enable the original note set in use
								return 1;
							}
							prompt = 1;
						}
						if(tuningdiff[ctr3] < 0)
						{	//If the tuning for this string is being lowered, existing notes need to be moved up the fretboard
							if(!ctr2)
							{	//First pass of the note, just validate the tranpsose
								if((tp->note[ctr]->frets[ctr3] & 0x7F) - val > tp->numfrets)
								{	//If the note's fret value (masking out the muting flag) can't be raised an equivalent number of frets
									skiptranspose = 1;	//Track that the note will be highlighted instead
								}
							}
							else
							{	//Second pass of the note
								if(!skiptranspose)
								{	//If the note is to be transposed automatically
									tp->note[ctr]->frets[ctr3] -= val;
								}
								else if(skiptranspose & 1)
								{	//Otherwise highlight it and warn about the note fret value
									tp->note[ctr]->flags |= EOF_NOTE_FLAG_HIGHLIGHT;	//Highlight it with the permanent flag
									if(!(warning & 1))
									{	//If the user hasn't been warned about this problem yet
										eof_seek_and_render_position(eof_selected_track, tp->note[ctr]->type, tp->note[ctr]->pos);	//Show the offending note
										allegro_message("Warning:  At least one note will have to be manually transposed to another string or octave.\nThese notes will be highlighted.");
										warning |= 1;
									}
									break;	//Skip checking the rest of the strings
								}
							}
						}
						else
						{	//The tuning for this string is being raised, existing notes need to be moved down the fretboard
							if(!ctr2)
							{	//First pass of the note, just validate the tranpsose
								if((tp->note[ctr]->frets[ctr3] & 0x7f) < val)
								{	//If the note's fret value (masking out the muting flag) can't be lowered an equivalent number of frets
									skiptranspose = 1;	//Track that the note will be highlighted instead
								}
							}
							else
							{	//Second pass of the note
								if(!skiptranspose)
								{	//If the note is to be transposed automatically
									tp->note[ctr]->frets[ctr3] -= val;
								}
								else if(skiptranspose & 1)
								{	//Otherwise highlight it and warn about the note fret value
									tp->note[ctr]->flags |= EOF_NOTE_FLAG_HIGHLIGHT;	//Highlight it with the permanent flag
									if(!(warning & 1))
									{	//If the user hasn't been warned about this problem yet
										eof_seek_and_render_position(eof_selected_track, tp->note[ctr]->type, tp->note[ctr]->pos);	//Show the offending note
										allegro_message("Warning:  At least one note will have to be manually transposed to another string or octave.\nThese notes will be highlighted.");
										warning |= 1;
									}
									break;	//Skip checking the rest of the strings
								}
							}
						}
					}//For each of the 6 supported strings
				}//Only check fret values for normal notes, because tech notes do not have any

				//Check slide end positions
				if((!tp->note[ctr]->slideend && !tp->note[ctr]->unpitchend) || !(tp->note[ctr]->note & diffmask))
					continue;	//If this note does not slide or is not altered by the tranpose, skip the remainder of the for loop

				//Determine whether the transpose would affect some but not all of the strings in a note, and set val to the number of halfsteps the transposition is
				for(ctr3 = 0, bitmask = 1, first = 1; ctr3 < 6; ctr3++, bitmask <<= 1)
				{	//For each of the 6 supported strings
					if(tp->note[ctr]->note & bitmask)
					{	//If this note uses this string
						if(!first)
						{	//If this isn't the first string being examined in this note
							if(val != tuningdiff[ctr3])
							{	//If this string is being transposed a different amount than the previous string
								skiptranspose = 2;	//Cancel transposing this chord's end slide position because the author will need to manually define it
								break;				//Stop checking this chord's other strings
							}
						}
						val = tuningdiff[ctr3];
						first = 0;
					}
				}
				if(!ctr2)
				{	//First pass of the note, validate whether the slide end position can be transposed automatically
					if(!first && !skiptranspose)
					{	//If the note uses at least one string that is being transposed and transposing the note hasn't already been ruled out (ie. any used strings transpose different amounts)
						if(val < 0)
						{	//If the tuning for this string is being lowered, the slide end position needs to be moved up the fretboard
							if(tp->note[ctr]->slideend && (tp->note[ctr]->slideend - val > tp->numfrets))
							{	//If the note's slide end position can't be raised an equivalent number of frets
								skiptranspose = 4;	//Track that the note will be highlighted instead
							}
							if(tp->note[ctr]->unpitchend && (tp->note[ctr]->unpitchend - val > tp->numfrets))
							{	//If the note's unpitched slide end position can't be raised an equivalent number of frets
								skiptranspose = 4;	//Track that the note will be highlighted instead
							}
						}
						else
						{	//The tuning for this string is being raised, the slide end position needs to be moved down the fretboard
							if(eof_pro_guitar_note_lowest_fret(tp, ctr) == val)
							{	//If the transpose would cause the start fret of any fretted string in the note to be 0
								skiptranspose = 8;	//Track that the note will be highlighted instead
							}
							else
							{
								if(tp->note[ctr]->slideend && (tp->note[ctr]->slideend <= val))
								{	//If the note's slide end position can't be lowered an equivalent number of frets and still be greater than 0 (not a valid end of slide position)
									skiptranspose = 4;	//Track that the note will be highlighted instead
								}
								if(tp->note[ctr]->unpitchend && (tp->note[ctr]->unpitchend <= val))
								{	//If the note's unpitched slide end position can't be lowered an equivalent number of frets and still be greater than 0 (not a valid end of slide position)
									skiptranspose = 4;	//Track that the note will be highlighted instead
								}
							}
						}
					}
				}
				else
				{	//Second pass of the note, alter the slide end position if applicable
					if(!skiptranspose)
					{	//This end of slide position is to be transposed automatically
						if(tp->note[ctr]->slideend)
							tp->note[ctr]->slideend -= val;
						if(tp->note[ctr]->unpitchend)
							tp->note[ctr]->unpitchend -= val;
					}
					else if(skiptranspose & (2 | 4 | 8))
					{	//Otherwise highlight it and warn about the slide
						tp->note[ctr]->flags |= EOF_NOTE_FLAG_HIGHLIGHT;	//Highlight it with the permanent flag
						if((skiptranspose & 2) && !(warning & 2))
						{	//If the user hasn't been warned about this problem yet
							eof_seek_and_render_position(eof_selected_track, tp->note[ctr]->type, tp->note[ctr]->pos);	//Show the offending note
							allegro_message("Warning:  At least one sliding chord will have to be manually transposed due to only some of its strings being affected.\nThese notes will be highlighted.");
							warning |= 2;
						}
						if((skiptranspose & 4) && !(warning & 4))
						{	//If the user hasn't been warned about this problem yet
							eof_seek_and_render_position(eof_selected_track, tp->note[ctr]->type, tp->note[ctr]->pos);	//Show the offending note
							allegro_message("Warning:  At least one sliding chord will have to be manually transposed due to its end of slide position.\nThese notes will be highlighted.");
							warning |= 4;
						}
						if((skiptranspose & 8) && !(warning & 8))
						{	//If the user hasn't been warned about this problem yet
							eof_seek_and_render_position(eof_selected_track, tp->note[ctr]->type, tp->note[ctr]->pos);	//Show the offending note
							allegro_message("Warning:  At least one sliding chord will have to be manually transposed due to its start of slide position.\nThese notes will be highlighted.");
							warning |= 8;
						}
					}
				}
			}//On the first pass, validate whether the note can be transposed automatically, on second pass, alter the note
		}//For each note in the track
	}//For each the normal and the tech note set

	eof_menu_pro_guitar_track_set_tech_view_state(tp, restore_tech_view);	//Re-enable the original note set in use
	return 1;
}

int eof_track_pro_guitar_toggle_ignore_tuning(void)
{
	unsigned long tracknum;
	EOF_PRO_GUITAR_TRACK *tp;

	if(!eof_song || (eof_selected_track >= eof_song->tracks))
		return 1;	//Invalid parameters
	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	tp = eof_song->pro_guitar_track[tracknum];
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	if(tp->ignore_tuning)
	{
		tp->ignore_tuning = 0;
	}
	else
	{
		tp->ignore_tuning = 1;
	}
	eof_chord_lookup_note = 0;	//Reset the cached chord lookup count

	return 1;
}

DIALOG eof_note_set_num_frets_strings_dialog[] =
{
	/* (proc)                 (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                      (dp2) (dp3) */
	{ eof_window_proc,     32,  68,  170, 136, 0,   0,    0,    0,     0,   0,   "Edit fret/string count", NULL, NULL },
	{ d_agup_text_proc,       44,  100, 110, 8,   2,   23,   0,    0,     0,   0,   "Max fret value:",        NULL, NULL },
	{ eof_verified_edit_proc, 158, 96,  26,  20,  2,   23,   0,    0,     2,   0,   eof_etext2,       "0123456789", NULL },
	{ d_agup_text_proc,		  44,  120, 64,	 8,   2,   23,   0,    0,     0,   0,   "Number of strings:",     NULL, NULL },
	{ d_agup_radio_proc,	  44,  140, 36,  15,  2,   23,   0,    0,     0,   0,   "4",                      NULL, NULL },
	{ d_agup_radio_proc,	  80,  140, 36,  15,  2,   23,   0,    0,     0,   0,   "5",                      NULL, NULL },
	{ d_agup_radio_proc,	  116, 140, 36,  15,  2,   23,   0,    0,     0,   0,   "6",                      NULL, NULL },
	{ d_agup_button_proc,     42,  164, 68,  28,  2,   23,   '\r', D_EXIT,0,   0,   "OK",                     NULL, NULL },
	{ d_agup_button_proc,     120, 164, 68,  28,  2,   23,   0,    D_EXIT,0,   0,   "Cancel",                 NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

int eof_track_set_num_frets_strings(void)
{
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	unsigned char newnumfrets = 0, newnumstrings = 0;
	unsigned long highestfret = 0;
	char undo_made = 0, cancel = 0;
	int retval;

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run if a pro guitar track isn't active

	//Update dialog fields
	(void) snprintf(eof_etext2, sizeof(eof_etext2) - 1, "%u", eof_song->pro_guitar_track[tracknum]->numfrets);
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
		default:
		break;
	}

	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_note_set_num_frets_strings_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_note_set_num_frets_strings_dialog);
	if(eof_popup_dialog(eof_note_set_num_frets_strings_dialog, 2) != 7)
		return 1;	//If the user did not click OK, return immediately

	//Update max fret number
	newnumfrets = atol(eof_etext2);
	if(newnumfrets && (newnumfrets != eof_song->pro_guitar_track[tracknum]->numfrets))
	{	//If the specified number of frets was changed
		highestfret = eof_get_highest_fret(eof_song, eof_selected_track);	//Get the highest used fret value in this track
		if(highestfret > newnumfrets)
		{	//If any notes in this track use fret values that would exceed the new fret limit
			char message[120] = {0};
			(void) snprintf(message, sizeof(message) - 1, "Warning:  This track uses frets as high as %lu, exceeding the proposed limit.", highestfret);
			eof_clear_input();
			retval = alert3(NULL, message, "Continue?", "&Yes", "&No", "Highlight conflicts", 'y', 'n', 0);
			if(retval != 1)
			{	//If user does not opt to continue after being alerted of this fret limit issue
				if(retval == 3)
				{	//If the user opted to highlight the notes that conflict with the new fret count
					eof_hightlight_all_notes_above_fret_number(eof_song, eof_selected_track, newnumfrets);
				}
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
			eof_clear_input();
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

	return 1;
}

DIALOG eof_track_pro_guitar_set_capo_position_dialog[] =
{
	/* (proc)                (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                    (dp2) (dp3) */
	{ eof_window_proc,    0,   0,   200, 132, 0,   0,   0,    0,      0,   0,   "Set capo position",    NULL, NULL },
	{ d_agup_text_proc,      12,  40,  60,  12,  0,   0,   0,    0,      0,   0,   "At fret #",            NULL, NULL },
	{ eof_verified_edit_proc,12,  56,  50,  20,  0,   0,   0,    0,      2,   0,   eof_etext, "0123456789", NULL },
	{ d_agup_button_proc,    12,  92,  84,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",                   NULL, NULL },
	{ d_agup_button_proc,    110, 92,  78,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",               NULL, NULL },
	{ NULL,                  0,   0,   0,   0,   0,   0,   0,    0,      0,   0,   NULL,                   NULL, NULL }
};

int eof_track_pro_guitar_set_capo_position(void)
{
	unsigned long position, tracknum;
	EOF_PRO_GUITAR_TRACK *tp;

	if(!eof_song_loaded || !eof_song)
		return 1;	//Do not allow this function to run if a chart is not loaded
	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	eof_render();
	eof_color_dialog(eof_track_pro_guitar_set_capo_position_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_track_pro_guitar_set_capo_position_dialog);

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	tp = eof_song->pro_guitar_track[tracknum];
	(void) snprintf(eof_etext, 3, "%u", tp->capo);
	if(eof_popup_dialog(eof_track_pro_guitar_set_capo_position_dialog, 2) != 3)
		return 0;	//If the user did not click OK, return immediately

	if(eof_etext[0] != '\0')
	{	//If the user provided a number
		position = atol(eof_etext);
	}
	else
	{	//User left the field empty
		position = 0;
	}
	if(position > eof_song->pro_guitar_track[tracknum]->numfrets)
	{	//If the given capo position is higher than this track's supported number of frets
		allegro_message("You cannot specify a capo position that is higher than this track's number of frets (%u).", eof_song->pro_guitar_track[tracknum]->numfrets);
	}
	else if(position != tp->capo)
	{	//If the user gave a valid position and it is different from the capo position that was already in use
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		tp->capo = position;
	}
	eof_chord_lookup_note = 0;	//Reset the cached chord lookup count

	return 0;
}

MENU eof_track_proguitar_fret_hand_copy_menu[EOF_TRACKS_MAX] =
{
	{eof_menu_track_names[0], eof_unused_menu_function, NULL, D_SELECTED, NULL},
	{eof_menu_track_names[1], eof_unused_menu_function, NULL, 0, NULL},
	{eof_menu_track_names[2], eof_unused_menu_function, NULL, 0, NULL},
	{eof_menu_track_names[3], eof_unused_menu_function, NULL, 0, NULL},
	{eof_menu_track_names[4], eof_unused_menu_function, NULL, 0, NULL},
	{eof_menu_track_names[5], eof_unused_menu_function, NULL, 0, NULL},
	{eof_menu_track_names[6], eof_unused_menu_function, NULL, 0, NULL},
	{eof_menu_track_names[7], eof_menu_track_copy_fret_hand_track_8, NULL, 0, NULL},
	{eof_menu_track_names[8], eof_menu_track_copy_fret_hand_track_9, NULL, 0, NULL},
	{eof_menu_track_names[9], eof_unused_menu_function, NULL, 0, NULL},
	{eof_menu_track_names[10], eof_menu_track_copy_fret_hand_track_11, NULL, 0, NULL},
	{eof_menu_track_names[11], eof_menu_track_copy_fret_hand_track_12, NULL, 0, NULL},
	{eof_menu_track_names[12], eof_unused_menu_function, NULL, 0, NULL},
	{eof_menu_track_names[13], eof_menu_track_copy_fret_hand_track_14, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

char eof_track_pro_guitar_set_fret_hand_position_dialog_string1[] = "Set fret hand position";
char eof_track_pro_guitar_set_fret_hand_position_dialog_string2[] = "Edit fret hand position";
DIALOG eof_track_pro_guitar_set_fret_hand_position_dialog[] =
{
	/* (proc)                (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                    (dp2) (dp3) */
	{ eof_window_proc,    0,   0,   200, 132, 0,   0,   0,    0,      0,   0,   eof_track_pro_guitar_set_fret_hand_position_dialog_string1,      NULL, NULL },
	{ d_agup_text_proc,      12,  40,  60,  12,  0,   0,   0,    0,      0,   0,   "At fret #",                NULL, NULL },
	{ eof_verified_edit_proc,12,  56,  50,  20,  0,   0,   0,    0,      2,   0,   eof_etext,     "0123456789", NULL },
	{ d_agup_button_proc,    12,  92,  84,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",               NULL, NULL },
	{ d_agup_button_proc,    110, 92,  78,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",           NULL, NULL },
	{ NULL,                  0,   0,   0,   0,   0,   0,   0,    0,      0,   0,   NULL,               NULL, NULL }
};

int eof_track_pro_guitar_set_fret_hand_position_at_timestamp(unsigned long timestamp)
{
	unsigned long position, tracknum;
	EOF_PHRASE_SECTION *ptr = NULL;	//If the seek position has a fret hand position defined, this will reference it
	unsigned long index = 0;		//Will store the index number of the existing fret hand position being edited
	EOF_PRO_GUITAR_TRACK *tp;
	unsigned long limit = 21;		//Unless Rocksmith 1 or Rock Band export are enabled, assume Rocksmith 2's limit

	if(!eof_song_loaded || !eof_song)
		return 1;	//Do not allow this function to run if a chart is not loaded
	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active
	if(eof_write_rs_files || eof_write_rb_files)
		limit = 19;	//Rocksmith 1 and Rock Band only support fret hand positions as high as fret 19

	eof_render();
	eof_color_dialog(eof_track_pro_guitar_set_fret_hand_position_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_track_pro_guitar_set_fret_hand_position_dialog);

	//Find the pointer to the fret hand position at the current seek position in this difficulty, if there is one
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	tp = eof_song->pro_guitar_track[tracknum];
	ptr = eof_pro_guitar_track_find_effective_fret_hand_position_definition(tp, eof_note_type, timestamp, &index, NULL, 1);
	if(ptr)
	{	//If an existing fret hand position is to be edited
		(void) snprintf(eof_etext, 5, "%lu", ptr->end_pos);	//Populate the input box with it
		eof_track_pro_guitar_set_fret_hand_position_dialog[0].dp = eof_track_pro_guitar_set_fret_hand_position_dialog_string2;	//Update the dialog window title to reflect that a hand position is being edited
	}
	else
	{
		eof_track_pro_guitar_set_fret_hand_position_dialog[0].dp = eof_track_pro_guitar_set_fret_hand_position_dialog_string1;	//Update the dialog window title to reflect that a new hand position is being added
		eof_etext[0] = '\0';	//Empty this string
	}
	if(eof_popup_dialog(eof_track_pro_guitar_set_fret_hand_position_dialog, 2) != 3)
		return 0;	//If the user did not click OK, return immediately

	if(eof_etext[0] != '\0')
	{	//If the user provided a number
		position = atol(eof_etext);
		if(position > tp->numfrets)
		{	//If the given fret position is higher than this track's supported number of frets
			allegro_message("You cannot specify a fret hand position that is higher than this track's number of frets (%u).", tp->numfrets);
			return 1;
		}
		else if(position + tp->capo > limit)
		{	//If the fret hand position (taking the capo into account) is higher than fret 19
			if(eof_write_rs_files || eof_write_rb_files)
			{	//Fret 22 is the highest supported fret in both Rock Band and Rocksmith 1
				if(tp->capo)
				{	//If there is a capo in use
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "You cannot specify a fret hand position higher than %u when a capo is at fret %u (it will cause Rocksmith 1 to crash).", 19U - tp->capo, tp->capo);
					allegro_message("%s", eof_log_string);
				}
				else
				{
					allegro_message("You cannot specify a fret hand position higher than 19 (it will cause Rocksmith 1 to crash).");
				}
				return 1;
			}
			//If this line is reached, it's because the limit was exceeded and it wasn't that of Rocksmith 1 or Rock Band (is Rocksmith 2's limit)
			if(eof_write_rs2_files)
			{	//Fret 24 is the highest supported fret in Rocksmith 2
				allegro_message("You cannot specify a fret hand position higher than 21 (it will cause Rocksmith 2 to crash).");
				return 1;
			}
			//Currently none of the supported games can use a FHP higher than 21
			allegro_message("You cannot specify a fret hand position higher than 21 (none of the supported games allow it).");
			return 1;
		}
		else if(!position)
		{	//If the user gave a fret position of 0
			allegro_message("You cannot specify a fret hand position of 0.");
			return 1;
		}
		else
		{	//If the user gave a valid position
			if(ptr)
			{	//If an existing fret hand position was being edited
				if(ptr->end_pos != position)
				{	//And it defines a different fret hand position than the user just gave
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					ptr->end_pos = position;	//Update the existing fret hand position entry
				}
				return 0;
			}

			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			(void) eof_track_add_section(eof_song, eof_selected_track, EOF_FRET_HAND_POS_SECTION, eof_note_type, timestamp, position, 0, NULL);
			eof_pro_guitar_track_sort_fret_hand_positions(tp);	//Sort the positions, since they must be in order for displaying to the user
		}
	}//If the user provided a number
	else if(ptr)
	{	//If the user left the input box empty and was editing an existing hand position
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		eof_pro_guitar_track_delete_hand_position(tp, index);	//Delete the existing fret hand position
		eof_pro_guitar_track_sort_fret_hand_positions(tp);	//Sort the positions, since they must be in order for displaying to the user
	}

	return 0;
}

int eof_track_pro_guitar_set_fret_hand_position(void)
{
	return eof_track_pro_guitar_set_fret_hand_position_at_timestamp(eof_music_pos.value - eof_av_delay);
}

int eof_track_pro_guitar_set_fret_hand_position_at_mouse(void)
{
	if(eof_pen_note.pos < eof_chart_length)
	{	//If the pen note is at a valid position
		return eof_track_pro_guitar_set_fret_hand_position_at_timestamp(eof_pen_note.pos);
	}
	return 0;
}

int eof_fret_hand_position_delete(DIALOG * d)
{
	unsigned long tracknum, ecount = 0, i;
	int junk;

	if(!d)
	{	//Satisfy Splint by checking value of d
		return D_O_K;
	}
	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return D_O_K;
	if(eof_fret_hand_position_list_dialog[1].d1 < 0)
		return D_O_K;

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	if(eof_song->pro_guitar_track[tracknum]->handpositions == 0)
	{
		return D_O_K;
	}
	for(i = 0; i < eof_song->pro_guitar_track[tracknum]->handpositions; i++)
	{	//For each fret hand position
		if(eof_song->pro_guitar_track[tracknum]->handposition[i].difficulty != eof_note_type)
			continue;	//If this fret hand position is not in the active difficulty, skip it

		/* if we've reached the item that is selected, delete it */
		if((unsigned long)eof_fret_hand_position_list_dialog[1].d1 == ecount)
		{
			if(!eof_fret_hand_position_list_dialog_undo_made)
			{	//If an undo state hasn't been made yet since launching this dialog
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				eof_fret_hand_position_list_dialog_undo_made = 1;
			}

			/* remove the hand position, update the selection in the list box and exit */
			eof_pro_guitar_track_delete_hand_position(eof_song->pro_guitar_track[tracknum], i);
			eof_pro_guitar_track_sort_fret_hand_positions(eof_song->pro_guitar_track[tracknum]);	//Re-sort the remaining hand positions
			eof_beat_stats_cached = 0;	//Have the beat statistics rebuilt
			for(i = 0, ecount = 0; i < eof_song->pro_guitar_track[tracknum]->handpositions; i++)
			{	//For each remaining fret hand position
				if(eof_song->pro_guitar_track[tracknum]->handposition[i].difficulty == eof_note_type)
				{	//If the fret hand position is in the active difficulty
					ecount++;
				}
			}
			if(((unsigned long)eof_fret_hand_position_list_dialog[1].d1 >= ecount) && (ecount > 0))
			{	//If the last list item was deleted and others remain
				eof_fret_hand_position_list_dialog[1].d1--;	//Select the one before the one that was deleted, or the last event, whichever one remains
			}
			eof_render();	//Redraw the screen
			(void) dialog_message(eof_fret_hand_position_list_dialog, MSG_START, 0, &junk);	//Re-initialize the dialog
			(void) dialog_message(eof_fret_hand_position_list_dialog, MSG_DRAW, 0, &junk);	//Redraw dialog
			return D_REDRAW;
		}

		/* go to next event */
		ecount++;
	}
	return D_O_K;
}

int eof_fret_hand_position_delete_all(DIALOG * d)
{
	unsigned long i, tracknum;
	int junk;

	if(!d)
	{	//Satisfy Splint by checking value of d
		return D_O_K;
	}
	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return D_O_K;

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	if(eof_song->pro_guitar_track[tracknum]->handpositions == 0)
	{
		return D_O_K;
	}
	for(i = eof_song->pro_guitar_track[tracknum]->handpositions; i > 0; i--)
	{	//For each fret hand position (in reverse order)
		if(eof_song->pro_guitar_track[tracknum]->handposition[i - 1].difficulty == eof_note_type)
		{	//If the fret hand position is in the active difficulty, delete it
			if(!eof_fret_hand_position_list_dialog_undo_made)
			{	//If an undo state hasn't been made yet since launching this dialog
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				eof_fret_hand_position_list_dialog_undo_made = 1;
			}
			eof_pro_guitar_track_delete_hand_position(eof_song->pro_guitar_track[tracknum], i - 1);
		}
	}

	eof_beat_stats_cached = 0;	//Have the beat statistics rebuilt
	eof_pro_guitar_track_sort_fret_hand_positions(eof_song->pro_guitar_track[tracknum]);	//Re-sort the remaining hand positions
	eof_render();	//Redraw the screen
	(void) dialog_message(eof_fret_hand_position_list_dialog, MSG_START, 0, &junk);	//Re-initialize the dialog
	(void) dialog_message(eof_fret_hand_position_list_dialog, MSG_DRAW, 0, &junk);	//Redraw dialog
	return D_REDRAW;
}

int eof_track_delete_effective_fret_hand_position(void)
{
	unsigned long index = 0;
	EOF_PRO_GUITAR_TRACK *tp;

	if(!eof_song_loaded || !eof_song)
		return 1;	//Do not allow this function to run if a chart is not loaded
	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	tp = eof_song->pro_guitar_track[eof_song->track[eof_selected_track]->tracknum];
	if(eof_pro_guitar_track_find_effective_fret_hand_position_definition(tp, eof_note_type, eof_music_pos.value - eof_av_delay, &index, NULL, 0))
	{	//If there is a fret hand position in effect at the current seek position in the active track difficulty
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		eof_pro_guitar_track_delete_hand_position(tp, index);	//Delete the hand position
		eof_pro_guitar_track_sort_fret_hand_positions(tp);		//Re-sort the remaining hand positions
	}
	return 1;
}

int eof_track_delete_fret_hand_position_at_pos(unsigned long track, unsigned char diff, unsigned long pos)
{
	unsigned long index = 0;
	EOF_PRO_GUITAR_TRACK *tp;

	if(!eof_song_loaded || !eof_song)
		return 1;	//Do not allow this function to run if a chart is not loaded
	if(eof_song->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	tp = eof_song->pro_guitar_track[eof_song->track[track]->tracknum];
	if(eof_pro_guitar_track_find_effective_fret_hand_position_definition(tp, diff, pos, &index, NULL, 1))
	{	//If there is a fret hand position defined at exactly the specified position of the specified track difficulty
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		eof_pro_guitar_track_delete_hand_position(tp, index);	//Delete the hand position
		eof_pro_guitar_track_sort_fret_hand_positions(tp);		//Re-sort the remaining hand positions
	}
	return 1;
}

int eof_fret_hand_position_seek(DIALOG * d)
{
	unsigned long tracknum, ecount = 0, i;
	int junk;

	if(!d)
	{	//Satisfy Splint by checking value of d
		return D_O_K;
	}
	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return D_O_K;
	if(eof_fret_hand_position_list_dialog[1].d1 < 0)
		return D_O_K;

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	if(eof_song->pro_guitar_track[tracknum]->handpositions == 0)
	{
		return D_O_K;
	}
	for(i = 0; i < eof_song->pro_guitar_track[tracknum]->handpositions; i++)
	{	//For each fret hand position
		if(eof_song->pro_guitar_track[tracknum]->handposition[i].difficulty == eof_note_type)
		{	//If the fret hand position is in the active difficulty
			/* if we've reached the item that is selected, seek to it */
			if((unsigned long)eof_fret_hand_position_list_dialog[1].d1 == ecount)
			{
				eof_set_seek_position(eof_song->pro_guitar_track[tracknum]->handposition[i].start_pos + eof_av_delay);	//Seek to the hand position's timestamp
				eof_render();	//Redraw screen
				(void) dialog_message(eof_fret_hand_position_list_dialog, MSG_DRAW, 0, &junk);	//Redraw dialog
				return D_O_K;
			}

			/* go to next event */
			ecount++;
		}
	}
	return D_O_K;
}

int eof_fret_hand_position_seek_prev(DIALOG * d)
{
	if(!d)
	{	//Satisfy Splint by checking value of d
		return D_O_K;
	}
	if(eof_fret_hand_position_list_dialog[1].d1 > 0)
	{	//If there is a previous list entry
		eof_fret_hand_position_list_dialog[1].d1--;	//Select the previous entry in the FHP list
		return eof_fret_hand_position_seek(d);		//Call the dialog function to seek to it
	}

	return D_O_K;
}

unsigned long eof_fret_hand_position_list_size = 0;

int eof_fret_hand_position_seek_next(DIALOG * d)
{
	if(!d)
	{	//Satisfy Splint by checking value of d
		return D_O_K;
	}
	if(eof_fret_hand_position_list_dialog[1].d1 + 1 < eof_fret_hand_position_list_size)
	{	//If there is a next list entry
		eof_fret_hand_position_list_dialog[1].d1++;	//Select the next entry in the FHP list
		return eof_fret_hand_position_seek(d);		//Call the dialog function to seek to it
	}

	return D_O_K;
}

char eof_fret_hand_position_list_text[EOF_MAX_NOTES][25] = {{0}};

char * eof_fret_hand_position_list(int index, int * size)
{
	unsigned long i, tracknum, ecount = 0;
	int ism, iss, isms;

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return NULL;

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	eof_pro_guitar_track_sort_fret_hand_positions(eof_song->pro_guitar_track[tracknum]);
	for(i = 0; i < eof_song->pro_guitar_track[tracknum]->handpositions; i++)
	{	//For each fret hand position
		if(eof_song->pro_guitar_track[tracknum]->handposition[i].difficulty == eof_note_type)
		{	//If the fret hand position is in the active difficulty
			if(ecount < EOF_MAX_TEXT_EVENTS)
			{
				ism = (eof_song->pro_guitar_track[tracknum]->handposition[i].start_pos / 1000) / 60;
				iss = (eof_song->pro_guitar_track[tracknum]->handposition[i].start_pos / 1000) % 60;
				isms = (eof_song->pro_guitar_track[tracknum]->handposition[i].start_pos % 1000);
				(void) snprintf(eof_fret_hand_position_list_text[ecount], sizeof(eof_fret_hand_position_list_text[ecount]) - 1, "%02d:%02d.%03d: Fret %lu", ism, iss, isms, eof_song->pro_guitar_track[tracknum]->handposition[i].end_pos);
				ecount++;
			}
		}
	}
	switch(index)
	{
		case -1:
		{
			if(size)
			{
				if(ecount <= INT_MAX)
				{
					*size = ecount;
					eof_fret_hand_position_list_size = ecount;	//Store this for the seek to next FHP function
				}
				else
					*size = INT_MAX;
			}
			if(ecount > 0)
			{	//If there is at least one fret hand position in the active difficulty
				eof_fret_hand_position_list_dialog[2].flags = 0;	//Enable the Delete button
				eof_fret_hand_position_list_dialog[3].flags = 0;	//Enable the Delete all button
			}
			else
			{
				eof_fret_hand_position_list_dialog[2].flags = D_DISABLED;
				eof_fret_hand_position_list_dialog[3].flags = D_DISABLED;
			}
			(void) snprintf(eof_fret_hand_position_list_dialog_title_string, sizeof(eof_fret_hand_position_list_dialog_title_string) - 1, "Fret hand positions (%lu)", ecount);	//Redraw the dialog's title bar to reflect the number of hand positions
			break;
		}
		default:
		{
			if(index < EOF_MAX_NOTES)
				return eof_fret_hand_position_list_text[index];
		}
	}
	return NULL;
}

char eof_fret_hand_position_list_dialog_undo_made = 0;	//Used to track whether an undo state was made in this dialog
char eof_fret_hand_position_list_dialog_title_string[30] = "Fret hand positions";	//This will be rebuilt by the list box count function to display the number of positions present
DIALOG eof_fret_hand_position_list_dialog[] =
{
	/* (proc)            (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)            (dp2) (dp3) */
	{ eof_window_proc,        0,   48,  250, 237, 2,   23,  0,    0,      0,   0,   "Fret hand positions",       NULL, NULL },
	{ d_agup_list_proc,        12,  84,  150, 138, 2,   23,  0,    0,      0,   0,   (void *)eof_fret_hand_position_list,NULL, NULL },
	{ d_agup_push_proc,  170, 84,  68,  28,  2,   23,  'l',  D_EXIT, 0,   0,   "De&lete",      NULL, (void *)eof_fret_hand_position_delete },
	{ d_agup_push_proc,  170, 124, 68,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Delete all",   NULL, (void *)eof_fret_hand_position_delete_all },
	{ d_agup_push_proc,  170, 164, 68,  28,  2,   23,  's',  D_EXIT, 0,   0,   "&Seek to",     NULL, (void *)eof_fret_hand_position_seek },
	{ d_agup_push_proc,  170, 204, 68,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Generate",     NULL, (void *)eof_generate_hand_positions_current_track_difficulty },
	{ d_agup_push_proc,  170, 244, 34,  28,  2,   23,  0,    D_EXIT, 0,   0,   "<",     NULL, (void *)eof_fret_hand_position_seek_prev },
	{ d_agup_push_proc,  204, 244, 34,  28,  2,   23,  0,    D_EXIT, 0,   0,   ">",     NULL, (void *)eof_fret_hand_position_seek_next },
	{ d_agup_button_proc,12,  244, 90,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "Done",         NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

int eof_track_fret_hand_positions(void)
{
	unsigned long tracknum, diffindex = 0;

	if(!eof_song_loaded || !eof_song)
		return 1;	//Do not allow this function to run if a chart is not loaded
	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_fret_hand_position_list_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_fret_hand_position_list_dialog);

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	(void) eof_pro_guitar_track_find_effective_fret_hand_position_definition(eof_song->pro_guitar_track[tracknum], eof_note_type, eof_music_pos.value - eof_av_delay, NULL, &diffindex, 0);	//Determine if a hand position exists at the current seek position
	eof_fret_hand_position_list_dialog[1].d1 = diffindex;	//Pre-select the hand position in effect (if one exists) at the current seek position
	eof_fret_hand_position_list_dialog[0].dp = eof_fret_hand_position_list_dialog_title_string;	//Replace the string used for the title bar with a dynamic one
	eof_fret_hand_position_list_dialog_undo_made = 0;			//Reset this condition
	(void) eof_popup_dialog(eof_fret_hand_position_list_dialog, 1);	//Launch the dialog

	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(NULL);
	return 1;
}

DIALOG eof_song_rs_popup_add_dialog[] =
{
	/* (proc)                (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                    (dp2) (dp3) */
	{ eof_window_proc,    0,   0,   200, 190, 0,   0,   0,    0,      0,   0,   "Rocksmith popup message",      NULL, NULL },
	{ eof_edit_proc,      12,  30,  176, 20,  2,   23,  0,    0,      EOF_SECTION_NAME_LENGTH,   0,   eof_etext,           NULL, NULL },
	{ d_agup_text_proc,      12,  56,  60,  12,  0,   0,   0,    0,      0,   0,   "Start position (ms)",                NULL, NULL },
	{ eof_verified_edit_proc,12,  72,  50,  20,  0,   0,   0,    0,      7,   0,   eof_etext2,     "0123456789", NULL },
	{ d_agup_text_proc,      12,  100, 60,  12,  0,   0,   0,    0,      0,   0,   "Duration (ms)",                NULL, NULL },
	{ eof_verified_edit_proc,12,  120, 50,  20,  0,   0,   0,    0,      7,   0,   eof_etext3,     "0123456789", NULL },
	{ d_agup_button_proc,    12,  150, 84,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",               NULL, NULL },
	{ d_agup_button_proc,    110, 150, 78,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",           NULL, NULL },
	{ NULL,                  0,   0,   0,   0,   0,   0,   0,    0,      0,   0,   NULL,               NULL, NULL }
};

int eof_track_rs_popup_add(void)
{
	unsigned long start, duration, i, sel_start = 0, sel_end = 0;
	char failed = 0;

	if(!eof_song_loaded || !eof_song)
		return 1;	//Do not allow this function to run if a chart is not loaded

	eof_render();
	eof_color_dialog(eof_song_rs_popup_add_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_song_rs_popup_add_dialog);

	//Initialize the start and end positions as appropriate
	if(eof_seek_selection_start != eof_seek_selection_end)
	{	//If there is a seek selection
		sel_start = eof_seek_selection_start;
		sel_end = eof_seek_selection_end;
	}
	else
	{	//Find the start and stop positions of any notes selected in the active track difficulty
		(void) eof_get_selected_note_range(&sel_start, &sel_end, 1);
	}

	(void) ustrcpy(eof_etext, "");
	if(sel_start != sel_end)
	{	//If notes in this track difficulty are selected
		(void) snprintf(eof_etext2, sizeof(eof_etext2) - 1, "%lu", sel_start);				//Initialize the start time with the start of the selection
		(void) snprintf(eof_etext3, sizeof(eof_etext3) - 1, "%lu", sel_end - sel_start);	//Initialize the duration based on the end of the selection
	}
	else
	{
		(void) snprintf(eof_etext2, sizeof(eof_etext2) - 1, "%lu", eof_music_pos.value - eof_av_delay);	//Otherwise initialize the start time with the current seek position
		(void) ustrcpy(eof_etext3, "");
	}
	if(eof_popup_dialog(eof_song_rs_popup_add_dialog, 1) == 6)
	{	//User clicked OK
		start = atol(eof_etext2);
		duration = atol(eof_etext3);

		if(!duration)
		{	//If the given duration is not valid
			allegro_message("The popup message must have a duration.");
		}
		else
		{
			for(i = 0; i < (unsigned long)ustrlen(eof_etext); i ++)
			{	//For each character in the user-specified string
				if((ugetat(eof_etext, i) == '(') || (ugetat(eof_etext, i) == ')'))
				{	//If the character is an open or close parenthesis
					allegro_message("Rocksmith does not allow parentheses () in popup messages.");
					failed = 1;
					break;
				}
			}
			if(!failed)
			{	//If no parentheses were found in the string
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				(void) eof_track_add_section(eof_song, eof_selected_track, EOF_RS_POPUP_MESSAGE, 0, start, start + duration, 0, eof_etext);
			}
		}
	}

	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(NULL);
	return 1;
}

char **eof_track_rs_popup_messages_list_strings = NULL;	//Stores allocated strings for eof_track_rs_popup_messages()
char eof_rs_popup_messages_dialog_undo_made = 0;	//Used to track whether an undo state was made in this dialog
char eof_rs_popup_messages_dialog_string[25] = {0};	//The title string for the RS popup messages dialog

DIALOG eof_rs_popup_messages_dialog[] =
{
	/* (proc)            (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)            (dp2) (dp3) */
	{ eof_window_proc,0,   48,  400, 237, 2,   23,  0,    0,      0,   0,   eof_rs_popup_messages_dialog_string,NULL, NULL },
	{ d_agup_list_proc,  12,  84,  300, 138, 2,   23,  0,    0,      0,   0,   (void *)eof_track_rs_popup_messages_list,NULL, NULL },
	{ d_agup_push_proc,  320, 84,  68,  28,  2,   23,  'l',  D_EXIT, 0,   0,   "De&lete",      NULL, (void *)eof_track_rs_popup_messages_delete },
	{ d_agup_push_proc,  320, 124, 68,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Delete all",   NULL, (void *)eof_track_rs_popup_messages_delete_all },
	{ d_agup_push_proc,  320, 164, 68,  28,  2,   23,  's',  D_EXIT, 0,   0,   "&Seek to",     NULL, (void *)eof_track_rs_popup_messages_seek },
	{ d_agup_push_proc,  320, 204, 68,  28,  2,   23,  'e',  D_EXIT, 0,   0,   "&Edit",        NULL, (void *)eof_track_rs_popup_messages_edit },
	{ d_agup_button_proc,12,  245, 90,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "Done",         NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

char * eof_track_rs_popup_messages_list(int index, int * size)
{
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return NULL;	//Incorrect track active

	switch(index)
	{
		case -1:
		{
			if(size)
				*size = eof_song->pro_guitar_track[tracknum]->popupmessages;
			(void) snprintf(eof_rs_popup_messages_dialog_string, sizeof(eof_rs_popup_messages_dialog_string) - 1, "RS popup messages (%lu)", eof_song->pro_guitar_track[tracknum]->popupmessages);
			break;
		}
		default:
		{
			return eof_track_rs_popup_messages_list_strings[index];
		}
	}
	return NULL;
}

void eof_rebuild_rs_popup_messages_list_strings(void)
{
	EOF_PRO_GUITAR_TRACK *tp;
	unsigned long tracknum, ctr;
	size_t stringlen;

	if(!eof_song_loaded || !eof_song || (eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return;	//Do not allow this function to run if a chart is not loaded or a pro guitar/bass track is not active

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	tp = eof_song->pro_guitar_track[tracknum];
	eof_track_rs_popup_messages_list_strings = malloc(sizeof(char *) * tp->popupmessages);	//Allocate enough pointers to have one for each popup message

	for(ctr = 0; ctr < tp->popupmessages; ctr++)
	{	//For each popup message
		stringlen = (size_t)snprintf(NULL, 0, "%lu-%lums : %s", tp->popupmessage[ctr].start_pos, tp->popupmessage[ctr].end_pos, tp->popupmessage[ctr].name) + 1;	//Find the number of characters needed to snprintf this string
		eof_track_rs_popup_messages_list_strings[ctr] = malloc(stringlen + 1);	//Allocate memory to build the string
		if(!eof_track_rs_popup_messages_list_strings[ctr])
		{
			allegro_message("Error allocating memory");
			while(ctr > 0)
			{	//Free previously allocated strings
				free(eof_track_rs_popup_messages_list_strings[ctr - 1]);
				ctr--;
			}
			free(eof_track_rs_popup_messages_list_strings);
			eof_track_rs_popup_messages_list_strings = NULL;
			return;
		}
		(void) snprintf(eof_track_rs_popup_messages_list_strings[ctr], stringlen, "%lu-%lums : %s", tp->popupmessage[ctr].start_pos, tp->popupmessage[ctr].end_pos, tp->popupmessage[ctr].name);
	}
}

int eof_find_effective_rs_popup_message(unsigned long pos, unsigned long *popupnum)
{
	EOF_PRO_GUITAR_TRACK *tp;
	unsigned long tracknum, ctr;

	if(!eof_song || (eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT) || !popupnum)
		return 0;	//Return false if a pro guitar track isn't active or parameters are invalid

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	tp = eof_song->pro_guitar_track[tracknum];

	for(ctr = 0; ctr < tp->popupmessages; ctr++)
	{	//For each popup message
		if((pos >= tp->popupmessage[ctr].start_pos) && (pos <= tp->popupmessage[ctr].end_pos))
		{	//If the specified position is within this popup message's time range
			*popupnum = ctr;	//Store the result
			return 1;	//Return found
		}
	}

	return 0;	//Return not found
}

int eof_track_rs_popup_messages(void)
{
	EOF_PRO_GUITAR_TRACK *tp;
	unsigned long tracknum, ctr, popupmessage = 0;

	if(!eof_song || (eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return 1;	//Do not allow this function to run if a pro guitar track isn't active

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	tp = eof_song->pro_guitar_track[tracknum];
	eof_track_pro_guitar_sort_popup_messages(tp);	//Re-sort the popup messages

	//Allocate and build the strings for the phrases
	eof_rebuild_rs_popup_messages_list_strings();
	if(eof_find_effective_rs_popup_message(eof_music_pos.value - eof_av_delay, &popupmessage))
	{	//If a popup message is in effect at the current seek position
		eof_rs_popup_messages_dialog[1].d1 = popupmessage;	//Pre-select the popup message from the list
	}

	//Call the dialog
	eof_clear_input();
	eof_rs_popup_messages_dialog_undo_made = 0;	//Reset this condition
	eof_color_dialog(eof_rs_popup_messages_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_rs_popup_messages_dialog);
	(void) eof_popup_dialog(eof_rs_popup_messages_dialog, 0);

	//Cleanup
	for(ctr = 0; ctr < tp->popupmessages; ctr++)
	{	//Free previously allocated strings
		free(eof_track_rs_popup_messages_list_strings[ctr]);
	}
	free(eof_track_rs_popup_messages_list_strings);
	eof_track_rs_popup_messages_list_strings = NULL;

	return 1;
}

void eof_track_pro_guitar_sort_popup_messages(EOF_PRO_GUITAR_TRACK* tp)
{
 	eof_log("eof_track_pro_guitar_sort_popup_messages() entered", 1);

	if(tp)
	{
		qsort(tp->popupmessage, (size_t)tp->popupmessages, sizeof(EOF_PHRASE_SECTION), eof_song_qsort_phrase_sections);
	}
}

void eof_track_pro_guitar_delete_popup_message(EOF_PRO_GUITAR_TRACK *tp, unsigned long index)
{
	unsigned long ctr;
 	eof_log("eof_track_pro_guitar_delete_popup_message() entered", 1);

	if(tp && (index < tp->popupmessages))
	{
		tp->popupmessage[index].name[0] = '\0';	//Empty the name string
		for(ctr = index; ctr < tp->popupmessages; ctr++)
		{
			memcpy(&tp->popupmessage[ctr], &tp->popupmessage[ctr + 1], sizeof(EOF_PHRASE_SECTION));
		}
		tp->popupmessages--;
	}
}

int eof_track_rs_popup_messages_delete(DIALOG * d)
{
	EOF_PRO_GUITAR_TRACK *tp;
	unsigned long tracknum, ctr, ctr2;

	if(!d)
	{	//Satisfy Splint by checking value of d
		return D_O_K;
	}
	if(!eof_song || (eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return D_O_K;	//Do not allow this function to run if a pro guitar track isn't active
	if(eof_rs_popup_messages_dialog[1].d1 < 0)
		return D_O_K;

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	tp = eof_song->pro_guitar_track[tracknum];
	if(tp->popupmessages == 0)
		return D_O_K;

	for(ctr = 0; ctr < tp->popupmessages; ctr++)
	{	//For each popup message
		if(ctr != (unsigned long)eof_rs_popup_messages_dialog[1].d1)
			continue;	//If this isn't the selected popup message, skip it

		if(!eof_rs_popup_messages_dialog_undo_made)
		{	//If an undo state hasn't been made yet since launching this dialog
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			eof_rs_popup_messages_dialog_undo_made = 1;
		}

		//Release strings
		for(ctr2 = 0; ctr2 < tp->popupmessages; ctr2++)
		{	//Free previously allocated strings
			free(eof_track_rs_popup_messages_list_strings[ctr2]);
		}
		free(eof_track_rs_popup_messages_list_strings);
		eof_track_rs_popup_messages_list_strings = NULL;

		/* remove the popup message, update the selection in the list box and exit */
		eof_track_pro_guitar_delete_popup_message(tp, ctr);
		eof_track_pro_guitar_sort_popup_messages(tp);	//Re-sort the remaining popup messages
		if(((unsigned long)eof_rs_popup_messages_dialog[1].d1 >= tp->popupmessages) && (tp->popupmessages > 0))
		{	//If the last list item was deleted and others remain
			eof_rs_popup_messages_dialog[1].d1--;	//Select the one before the one that was deleted, or the last message, whichever one remains
		}
	}

	//Rebuild the strings for the dialog menu
	eof_rebuild_rs_popup_messages_list_strings();

	return D_REDRAW;	//Have Allegro redraw the dialog
}

int eof_track_rs_popup_messages_edit(DIALOG * d)
{
	unsigned long start, duration, i, tracknum;
	EOF_PHRASE_SECTION *ptr;
	EOF_PRO_GUITAR_TRACK *tp;
	char failed = 0;

	if(!d)
	{	//Satisfy Splint by checking value of d
		return D_O_K;
	}

	if(!eof_song_loaded || !eof_song || (eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return D_O_K;	//Do not allow this function to run if a chart is not loaded or a pro guitar/bass track is not active

	eof_color_dialog(eof_song_rs_popup_add_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_song_rs_popup_add_dialog);

	//Initialize the dialog fields
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	tp = eof_song->pro_guitar_track[tracknum];
	if((eof_rs_popup_messages_dialog[1].d1 < 0) || ((unsigned long)eof_rs_popup_messages_dialog[1].d1 >= tp->popupmessages))
		return D_O_K;	//Invalid popup message selected in list
	ptr = &(tp->popupmessage[eof_rs_popup_messages_dialog[1].d1]);
	(void) ustrcpy(eof_etext, ptr->name);
	(void) snprintf(eof_etext2, sizeof(eof_etext2) - 1, "%lu", ptr->start_pos);
	(void) snprintf(eof_etext3, sizeof(eof_etext3) - 1, "%lu", ptr->end_pos - ptr->start_pos);

	eof_clear_input();
	if(eof_popup_dialog(eof_song_rs_popup_add_dialog, 1) == 6)
	{	//User clicked OK
		start = strtoul(eof_etext2, NULL, 10);
		duration = strtoul(eof_etext3, NULL, 10);

		if(!duration)
		{	//If the given timing is not valid
			allegro_message("The popup message must have a duration");
		}
		else
		{
			for(i = 0; i < (unsigned long)ustrlen(eof_etext); i ++)
			{	//For each character in the user-specified string
				if((ugetat(eof_etext, i) == '(') || (ugetat(eof_etext, i) == ')'))
				{	//If the character is an open or close parenthesis
					allegro_message("Rocksmith does not allow parentheses () in popup messages.");
					failed = 1;
					break;
				}
			}
			if(!failed && (ustrcmp(eof_etext, ptr->name) || (start != ptr->start_pos) || (duration != ptr->end_pos - ptr->start_pos)))
			{	//If no parentheses were found and any of the popup's fields were edited
				if(!eof_rs_popup_messages_dialog_undo_made)
				{	//If an undo state hasn't been made yet since launching this dialog
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					eof_rs_popup_messages_dialog_undo_made = 1;
				}
				(void) ustrcpy(ptr->name, eof_etext);	//Update the popup message string
				ptr->name[EOF_SECTION_NAME_LENGTH] = '\0';	//Guarantee NULL termination
				ptr->start_pos = start;				//Update start timestamp
				ptr->end_pos = start + duration;	//Update end timestamp
			}
		}
	}

	eof_track_pro_guitar_sort_popup_messages(tp);	//Re-sort the popup messages

	//Release strings
	for(i = 0; i < tp->popupmessages; i++)
	{	//Free previously allocated strings
		free(eof_track_rs_popup_messages_list_strings[i]);
	}
	free(eof_track_rs_popup_messages_list_strings);
	eof_track_rs_popup_messages_list_strings = NULL;

	//Rebuild the strings for the dialog menu
	eof_rebuild_rs_popup_messages_list_strings();

	return D_REDRAW;	//Have Allegro redraw the dialog
}

int eof_track_rs_popup_messages_delete_all(DIALOG * d)
{
	unsigned long i, tracknum;
	int junk;

	if(!d)
	{	//Satisfy Splint by checking value of d
		return D_O_K;
	}
	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return D_O_K;

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	if(eof_song->pro_guitar_track[tracknum]->popupmessages == 0)
	{
		return D_O_K;
	}
	for(i = eof_song->pro_guitar_track[tracknum]->popupmessages; i > 0; i--)
	{	//For each popup message (in reverse order)
		if(!eof_rs_popup_messages_dialog_undo_made)
		{	//If an undo state hasn't been made yet since launching this dialog
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			eof_rs_popup_messages_dialog_undo_made = 1;
		}
		eof_track_pro_guitar_delete_popup_message(eof_song->pro_guitar_track[tracknum], i - 1);
	}

	(void) dialog_message(eof_rs_popup_messages_dialog, MSG_START, 0, &junk);	//Re-initialize the dialog
	(void) dialog_message(eof_rs_popup_messages_dialog, MSG_DRAW, 0, &junk);	//Redraw dialog
	return D_REDRAW;
}

int eof_track_rs_popup_messages_seek(DIALOG * d)
{
	unsigned long tracknum;
	EOF_PRO_GUITAR_TRACK *tp;
	int junk;

	if(!d)
	{	//Satisfy Splint by checking value of d
		return D_O_K;
	}
	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return D_O_K;

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	tp = eof_song->pro_guitar_track[tracknum];
	if((eof_rs_popup_messages_dialog[1].d1 < 0) || ((unsigned long)eof_rs_popup_messages_dialog[1].d1 >= tp->popupmessages))
	{	//Invalid popup selected
		return D_O_K;
	}
	eof_set_seek_position(tp->popupmessage[eof_rs_popup_messages_dialog[1].d1].start_pos + eof_av_delay);	//Seek to the pop message's timestamp
	eof_render();	//Redraw screen
	(void) dialog_message(eof_rs_popup_messages_dialog, MSG_DRAW, 0, &junk);	//Redraw dialog

	return D_O_K;
}

MENU eof_track_proguitar_menu[] =
{
	{"Set &Tuning", eof_track_tuning, NULL, 0, NULL},
	{"Set number of &Frets/strings", eof_track_set_num_frets_strings, NULL, 0, NULL},
	{"Set &Capo", eof_track_pro_guitar_set_capo_position, NULL, 0, NULL},
	{"&Ignore tuning/capo", eof_track_pro_guitar_toggle_ignore_tuning, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_menu_track_rocksmith_tone_change_copy_menu[EOF_TRACKS_MAX] =
{
	{eof_menu_track_names[0], eof_unused_menu_function, NULL, D_SELECTED, NULL},
	{eof_menu_track_names[1], eof_unused_menu_function, NULL, 0, NULL},
	{eof_menu_track_names[2], eof_unused_menu_function, NULL, 0, NULL},
	{eof_menu_track_names[3], eof_unused_menu_function, NULL, 0, NULL},
	{eof_menu_track_names[4], eof_unused_menu_function, NULL, 0, NULL},
	{eof_menu_track_names[5], eof_unused_menu_function, NULL, 0, NULL},
	{eof_menu_track_names[6], eof_unused_menu_function, NULL, 0, NULL},
	{eof_menu_track_names[7], eof_menu_track_copy_tone_changes_track_8, NULL, 0, NULL},
	{eof_menu_track_names[8], eof_menu_track_copy_tone_changes_track_9, NULL, 0, NULL},
	{eof_menu_track_names[9], eof_unused_menu_function, NULL, 0, NULL},
	{eof_menu_track_names[10], eof_menu_track_copy_tone_changes_track_11, NULL, 0, NULL},
	{eof_menu_track_names[11], eof_menu_track_copy_tone_changes_track_12, NULL, 0, NULL},
	{eof_menu_track_names[12], eof_unused_menu_function, NULL, 0, NULL},
	{eof_menu_track_names[13], eof_menu_track_copy_tone_changes_track_14, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_track_rocksmith_tone_change_menu[] =
{
	{"&Add\t" CTRL_NAME "+Shift+T", eof_track_rs_tone_change_add, NULL, 0, NULL},
	{"&List", eof_track_rs_tone_changes, NULL, 0, NULL},
	{"&Names", eof_track_rs_tone_names, NULL, 0, NULL},
	{"&Copy From", NULL, eof_menu_track_rocksmith_tone_change_copy_menu, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_menu_track_rocksmith_popup_copy_menu[EOF_TRACKS_MAX] =
{
	{eof_menu_track_names[0], eof_unused_menu_function, NULL, D_SELECTED, NULL},
	{eof_menu_track_names[1], eof_unused_menu_function, NULL, 0, NULL},
	{eof_menu_track_names[2], eof_unused_menu_function, NULL, 0, NULL},
	{eof_menu_track_names[3], eof_unused_menu_function, NULL, 0, NULL},
	{eof_menu_track_names[4], eof_unused_menu_function, NULL, 0, NULL},
	{eof_menu_track_names[5], eof_unused_menu_function, NULL, 0, NULL},
	{eof_menu_track_names[6], eof_unused_menu_function, NULL, 0, NULL},
	{eof_menu_track_names[7], eof_menu_track_copy_popups_track_8, NULL, 0, NULL},
	{eof_menu_track_names[8], eof_menu_track_copy_popups_track_9, NULL, 0, NULL},
	{eof_menu_track_names[9], eof_unused_menu_function, NULL, 0, NULL},
	{eof_menu_track_names[10], eof_menu_track_copy_popups_track_11, NULL, 0, NULL},
	{eof_menu_track_names[11], eof_menu_track_copy_popups_track_12, NULL, 0, NULL},
	{eof_menu_track_names[12], eof_unused_menu_function, NULL, 0, NULL},
	{eof_menu_track_names[13], eof_menu_track_copy_popups_track_14, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_track_rocksmith_popup_menu[] =
{
	{"&Add", eof_track_rs_popup_add, NULL, 0, NULL},
	{"&List", eof_track_rs_popup_messages, NULL, 0, NULL},
	{"&Copy From", NULL, eof_menu_track_rocksmith_popup_copy_menu, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_track_rocksmith_fret_hand_menu[] =
{
	{"&Set\tShift+F", eof_track_pro_guitar_set_fret_hand_position, NULL, 0, NULL},
	{"&List\t" CTRL_NAME "+Shift+F", eof_track_fret_hand_positions, NULL, 0, NULL},
	{"&Copy", eof_track_fret_hand_positions_copy_from, NULL, 0, NULL},
	{"Generate all diffs", eof_track_fret_hand_positions_generate_all, NULL, 0, NULL},
	{"Delete effective\tShift+Del", eof_track_delete_effective_fret_hand_position, NULL, 0, NULL},
	{"Copy &From", NULL, eof_track_proguitar_fret_hand_copy_menu, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_track_rocksmith_menu[] =
{
	{"Enable tech &View\tF4", eof_menu_track_toggle_tech_view, NULL, 0, NULL},
	{"Fret &Hand positions", NULL, eof_track_rocksmith_fret_hand_menu, 0, NULL},
	{"&Popup messages", NULL, eof_track_rocksmith_popup_menu, 0, NULL},
	{"&Arrangement type", NULL, eof_track_rocksmith_arrangement_menu, 0, NULL},
	{"&Tone change", NULL, eof_track_rocksmith_tone_change_menu, 0, NULL},
	{"Remove difficulty limit", eof_track_rocksmith_toggle_difficulty_limit, NULL, 0, NULL},
	{"Insert new difficulty", eof_track_rocksmith_insert_difficulty, NULL, 0, NULL},
	{"Dynamic difficulty &List", eof_rocksmith_dynamic_difficulty_list, NULL, 0, NULL},
	{"&Manage RS phrases\t" CTRL_NAME "+Shift+M", eof_track_manage_rs_phrases, NULL, 0, NULL},
	{"Flatten this difficulty", eof_track_flatten_difficulties, NULL, 0, NULL},
	{"Un-flatten track", eof_track_unflatten_difficulties, NULL, 0, NULL},
	{"Picked &Bass", eof_menu_track_rs_picked_bass_arrangement, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

int eof_track_rocksmith_arrangement_set(unsigned char num)
{
	unsigned long tracknum;

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	if(num > 4)
	{	//If the specified arrangement number is invalid
		num = 0;	//Make it undefined
	}
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	eof_song->pro_guitar_track[tracknum]->arrangement = num;
	if(num != 4)
	{	//If the arrangement type was changed to anything other than bass
		eof_song->track[eof_selected_track]->flags &= ~EOF_TRACK_FLAG_RS_PICKED_BASS;	//Clear the picked bass flag
	}
	return 1;
}

int eof_track_rocksmith_arrangement_undefined(void)
{
	return eof_track_rocksmith_arrangement_set(0);
}

int eof_track_rocksmith_arrangement_combo(void)
{
	return eof_track_rocksmith_arrangement_set(1);
}

int eof_track_rocksmith_arrangement_rhythm(void)
{
	return eof_track_rocksmith_arrangement_set(2);
}

int eof_track_rocksmith_arrangement_lead(void)
{
	return eof_track_rocksmith_arrangement_set(3);
}

int eof_track_rocksmith_arrangement_bass(void)
{
	return eof_track_rocksmith_arrangement_set(4);
}

int eof_track_rocksmith_toggle_difficulty_limit(void)
{
	unsigned long ctr;

	if(!eof_song || (eof_selected_track >= eof_song->tracks))
		return 1;	//Invalid parameters
	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	(void) eof_detect_difficulties(eof_song, eof_selected_track);	//Determine which difficulties are populated for the active track
	if(eof_song->track[eof_selected_track]->flags & EOF_TRACK_FLAG_UNLIMITED_DIFFS)
	{	//If the active track already had the difficulty limit removed
		for(ctr = 5; ctr < 256; ctr++)
		{	//For each possible difficulty, starting after the first 5
			if(eof_track_diff_populated_status[ctr])
			{	//If this difficulty is populated
				allegro_message("Warning:  There is at least one populated difficulty beyond the first 5 difficulties.  Only the first 5 will export to MIDI.");
				break;
			}
		}
		if(eof_track_diff_populated_status[EOF_NOTE_SPECIAL])
		{	//If there are any notes in the BRE difficulty
			allegro_message("Warning:  There are notes in the BRE difficulty.  Ensure this difficulty's contents are valid by Rock Band standards if you plan to use the exported MIDI.");
		}
	}
	else
	{	//The track currently has the difficulty limit in effect, toggle this flag on
		if(eof_track_diff_populated_status[EOF_NOTE_SPECIAL])
		{	//If there are any notes in the BRE difficulty
			allegro_message("Warning:  There are notes in the BRE difficulty.  BRE notes from Rock Band charts are not compatible with Rocksmith and will need to be removed.");
		}
	}

	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	eof_song->track[eof_selected_track]->flags ^= EOF_TRACK_FLAG_UNLIMITED_DIFFS;	//Toggle this flag
	if((eof_song->track[eof_selected_track]->flags & EOF_TRACK_FLAG_UNLIMITED_DIFFS) == 0)
	{	//If the difficulty limit is in place
		if(eof_note_type > 4)
		{	//Ensure one of the first 5 difficulties is active
			eof_note_type = 4;
		}
	}
	eof_fix_window_title();
	eof_determine_phrase_status(eof_song, eof_selected_track);	//Update note flags because tremolo phrases behave different depending on the difficulty numbering in effect
	return 1;
}

int eof_track_rocksmith_insert_difficulty(void)
{
	unsigned long ctr, tracknum;
	unsigned char thistype, newdiff, upper = 0, lower = 0;
	EOF_PRO_GUITAR_TRACK *tp;
	char undo_made = 0;
	char restore_tech_view = 0;		//If tech view is in effect, it is temporarily disabled until after the note difficulties are updated

	if(!eof_song || eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	if(eof_song->track[eof_selected_track]->numdiffs == 255)
	{
		allegro_message("No more difficulties can be added.");
		return 1;
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	undo_made = 1;
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	tp = eof_song->pro_guitar_track[tracknum];

	eof_clear_input();
	if(alert(NULL, "Insert the new difficulty above or below the active difficulty?", NULL, "&Above", "&Below", 'a', 'b') == 1)
	{	//If the user chooses to insert the difficulty above the active difficulty
		newdiff = eof_note_type + 1;	//The difficulty number of all content at/above the difficulty immediately above the active difficulty will be incremented
	}
	else
	{	//The user chose to insert the difficulty below the active difficulty
		newdiff = eof_note_type;
	}

	restore_tech_view = eof_menu_track_get_tech_view_state(eof_song, eof_selected_track);
	eof_menu_track_set_tech_view_state(eof_song, eof_selected_track, 0);	//Disable tech view if applicable

	//Update note difficulties
	for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
	{	//For each note in the track
		thistype = eof_get_note_type(eof_song, eof_selected_track, ctr);	//Get this note's difficulty
		if(thistype >= newdiff)
		{	//If this note's difficulty needs to be updated
			eof_set_note_type(eof_song, eof_selected_track, ctr, thistype + 1);
		}
	}

	//Update tech note difficulties
	for(ctr = 0; ctr < tp->technotes; ctr++)
	{	//For each tech note in the track
		if(tp->technote[ctr]->type >= newdiff)
		{	//If this tech note's difficulty needs to be updated
			tp->technote[ctr]->type++;
		}
	}

	//Update arpeggio difficulties
	for(ctr = 0; ctr < tp->arpeggios; ctr++)
	{	//For each arpeggio phrase in the track
		if(tp->arpeggio[ctr].difficulty >= newdiff)
		{	//If this arpeggio phrase's difficulty needs to be updated
			tp->arpeggio[ctr].difficulty++;
		}
	}

	//Update fret hand positions
	for(ctr = 0; ctr < tp->handpositions; ctr++)
	{	//For each fret hand position
		if(tp->handposition[ctr].difficulty >= newdiff)
		{	//If this fret hand position's difficulty needs to be updated
			tp->handposition[ctr].difficulty++;
		}
	}

	//Prompt whether to clone an adjacent difficulty if applicable
	(void) eof_detect_difficulties(eof_song, eof_selected_track);	//Find which difficulties are populated
	if((newdiff > 0) && (eof_track_diff_populated_status[newdiff - 1]))
	{	//If there's a populated difficulty below the newly inserted difficulty
		lower = 1;
	}
	if((newdiff < 255) && (eof_track_diff_populated_status[newdiff + 1]))
	{	//If there's a populated difficulty above the newly inserted difficulty
		upper = 1;
	}
	eof_note_type = newdiff;
	if(lower && !upper)
	{	//If only the lower difficulty is populated, offer to copy it into the new difficulty
		eof_clear_input();
		if(alert(NULL, "Would you like to copy the lower difficulty's contents?", NULL, "&Yes", "&No", 'y', 'n') == 1)
		{	//If user opted to copy the lower difficulty
			(void) eof_menu_edit_paste_from_difficulty(newdiff - 1, &undo_made);
		}
	}
	else if(!lower && upper)
	{	//If only the upper difficulty is populated, offer to copy it into the new difficulty
		eof_clear_input();
		if(alert(NULL, "Would you like to copy the upper difficulty's contents?", NULL, "&Yes", "&No", 'y', 'n') == 1)
		{	//If user opted to copy the upper difficulty
			(void) eof_menu_edit_paste_from_difficulty(newdiff + 1, &undo_made);
		}
	}
	else if(lower && upper)
	{	//If both the upper and lower difficulties are populated, prompt whether to copy either into the new difficulty
		eof_clear_input();
		if(alert(NULL, "Would you like to copy an adjacent difficulty's contents?", NULL, "&Yes", "&No", 'y', 'n') == 1)
		{	//If user opted to copy either the upper or lower difficulty
			eof_clear_input();
			if(alert(NULL, "Copy the upper difficulty or the lower difficulty?", NULL, "&Upper", "&Lower", 'u', 'l') == 1)
			{	//If user opted to copy the upper difficulty
				(void) eof_menu_edit_paste_from_difficulty(newdiff + 1, &undo_made);
			}
			else
			{	//The user opted to copy the lower difficulty
				(void) eof_menu_edit_paste_from_difficulty(newdiff - 1, &undo_made);
			}
		}
	}

	eof_menu_track_set_tech_view_state(eof_song, eof_selected_track, restore_tech_view);	//Re-enable tech view if applicable
	eof_song->track[eof_selected_track]->flags |= EOF_TRACK_FLAG_UNLIMITED_DIFFS;	//Remove the difficulty limit for this track
	eof_pro_guitar_track_sort_fret_hand_positions(tp);
	eof_song->track[eof_selected_track]->numdiffs++;	//Increment the track's difficulty counter
	(void) eof_detect_difficulties(eof_song, eof_selected_track);
	eof_fix_window_title();	//Redraw the window title in case the active difficulty was incremented to compensate for inserting a difficulty below the active difficulty
	return 1;
}

int eof_track_delete_difficulty(void)
{
	char undo_made = 0;

	if(!eof_song)
		return 1;

	if(eof_track_diff_populated_status[eof_note_type])
	{	//If the active track has any notes
		eof_clear_input();
		if(alert(NULL, "Warning:  This difficulty contains at least one note.  Delete the difficulty anyway?", NULL, "&Yes", "&No", 'y', 'n') != 1)
		{	//If the user doesn't opt to delete the populated track difficulty
			return 1;
		}
	}

	//Delete all content in the track difficulty, decrementing the difficulty of remaining notes, arpeggios, hand positions and tremolos
	eof_track_add_or_remove_track_difficulty_content_range(eof_song, eof_selected_track, eof_note_type, 0, ULONG_MAX, -1, 3, &undo_made);

	if(eof_song->track[eof_selected_track]->numdiffs > 5)
	{	//If there are more than 5 difficulties in the active track
		eof_song->track[eof_selected_track]->numdiffs--;	//Decrement the track's difficulty counter
	}
	(void) eof_detect_difficulties(eof_song, eof_selected_track);
	return 1;
}

char *eof_rocksmith_dynamic_difficulty_list_array[256] = {0};

DIALOG eof_rocksmith_dynamic_difficulty_list_dialog[] =
{
	/* (proc)                        (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                     (dp2) (dp3) */
	{ eof_window_proc, 0,   0,   500, 450, 2,   23,  0,    0,      0,   0,   "Dynamic difficulty list", NULL, NULL },
	{ d_agup_list_proc,        12,  35, 470, 360, 2,   23,  0,    0,      0,   0,   (void *)eof_rocksmith_dynamic_difficulty_list_proc, NULL, NULL },
	{ d_agup_button_proc, 12,  410, 68,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",                    NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

char * eof_rocksmith_dynamic_difficulty_list_proc(int index, int * size)
{
	if(!eof_song || (eof_selected_track >= eof_song->tracks))
		return NULL;

	if(index < 0)
	{	//Signal to return the list count
		if(size)
			*size = eof_song->track[eof_selected_track]->numdiffs;
		return  NULL;
	}
	else if(index < eof_song->track[eof_selected_track]->numdiffs)
	{	//Return the specified list item
		return eof_rocksmith_dynamic_difficulty_list_array[index];
	}
	return NULL;
}

int eof_rocksmith_dynamic_difficulty_list(void)
{
	unsigned long ctr;
	char temp_string[256];
	unsigned long last_flattened_size = 0;
	unsigned long this_flattened_size;
	unsigned long complete_flattened_size = eof_get_track_flattened_diff_size(eof_song, eof_selected_track, 0xFF);
	long delta;

	if(!eof_song_loaded || !eof_song)
		return 1;	//Do not allow this function to run if a chart is not loaded
	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	if(!eof_track_has_dynamic_difficulty(eof_song, eof_selected_track))
	{
		allegro_message("This track does not have dynamic difficulty enabled (Track>Rocksmith>Remove Difficulty Limit).");
		return 1;
	}
	if(!complete_flattened_size)
	{	//If no notes were counted for the active track
		allegro_message("This track or the active note set (normal notes or tech notes) is empty.");
		return 1;
	}

	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_rocksmith_dynamic_difficulty_list_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_rocksmith_dynamic_difficulty_list_dialog);

	//Build a string for each dynamic difficulty in the active track
	for(ctr = 0; ctr < eof_song->track[eof_selected_track]->numdiffs; ctr++)
	{
		this_flattened_size = eof_get_track_flattened_diff_size(eof_song, eof_selected_track, ctr);
		delta = this_flattened_size - last_flattened_size;
		(void) snprintf(temp_string, sizeof(temp_string) - 1, "#%lu : %lu notes, %lu notes flattened (%c%ld notes, %.2f%% of all notes)", ctr, eof_get_track_diff_size(eof_song, eof_selected_track, ctr), this_flattened_size, (delta < 0 ? '-' : '+'), labs(delta), (double)this_flattened_size * 100.0/complete_flattened_size);
		eof_rocksmith_dynamic_difficulty_list_array[ctr] = DuplicateString(temp_string);
		last_flattened_size = this_flattened_size;
	}

	(void) eof_popup_dialog(eof_rocksmith_dynamic_difficulty_list_dialog, 1);

	//Release strings from memory
	for(ctr = 0; ctr < eof_song->track[eof_selected_track]->numdiffs; ctr++)
	{
		if(eof_rocksmith_dynamic_difficulty_list_array[ctr])
		{
			free(eof_rocksmith_dynamic_difficulty_list_array[ctr]);
			eof_rocksmith_dynamic_difficulty_list_array[ctr] = NULL;
		}
	}

	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(screen);
	return D_O_K;
}

DIALOG eof_track_fret_hand_positions_copy_from_dialog[] =
{
	/* (proc)                       (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)            (dp2) (dp3) */
	{ eof_window_proc,0,  48,  250, 237, 2,   23,  0,    0,      0,   0,   "Copy fret hand positions from diff #", NULL, NULL },
	{ d_agup_list_proc,      12,  84,  226, 138, 2,   23,  0,    0,      0,   0,   (void *)eof_rocksmith_dynamic_difficulty_list_proc,NULL, NULL },
	{ d_agup_button_proc,12,  245, 90,  28,  2,   23,  'c', D_EXIT,  0,   0,   "&Copy",         NULL, NULL },
	{ d_agup_button_proc,148, 245, 90,  28,  2,   23,  0,   D_EXIT,  0,   0,   "Cancel",        NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

char eof_menu_song_difficulty_list_strings[256][4];

char * eof_track_fret_hand_positions_copy_from_list(int index, int * size)
{
	unsigned long ctr2, ctr3, diffcount = 0;
	unsigned long tracknum;
	EOF_PRO_GUITAR_TRACK *tp;

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return NULL;

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	tp = eof_song->pro_guitar_track[tracknum];
	if(tp->handpositions)
	{	//If the active difficulty has at least one fret hand position
		for(ctr2 = 0; ctr2 < 256; ctr2++)
		{	//For each possible difficulty
			for(ctr3 = 0; ctr3 < tp->handpositions; ctr3++)
			{	//For each hand position in the track
				if((tp->handposition[ctr3].difficulty == ctr2) && (eof_note_type != ctr2))
				{	//If this hand position is in the difficulty being checked, and it isn't in the active difficulty, increment counter
					diffcount++;	//Track the number of difficulties that contain any fret hand positions
					break;	//Break so that the remaining difficulties can be checked
				}
			}
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

int eof_track_fret_hand_positions_copy_from(void)
{
	unsigned long tracknum, ctr, ctr2, ctr3, target, diffcount = 0;
	EOF_PRO_GUITAR_TRACK *tp;
	char user_warned = 0;

	if(!eof_song_loaded || !eof_song)
		return 1;	//Do not allow this function to run if a chart is not loaded
	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	tp = eof_song->pro_guitar_track[tracknum];
	if(tp->handpositions)
	{	//If the active difficulty has at least one fret hand position
		for(ctr2 = 0; ctr2 < 256; ctr2++)
		{	//For each possible difficulty
			for(ctr3 = 0; ctr3 < tp->handpositions; ctr3++)
			{	//For each hand position in the track
				if((tp->handposition[ctr3].difficulty == ctr2) && (eof_note_type != ctr2))
				{	//If this hand position is in the difficulty being checked, and it isn't in the active difficulty, build its list box display string and increment counter
					(void) snprintf(eof_menu_song_difficulty_list_strings[diffcount], sizeof(eof_menu_song_difficulty_list_strings[0]) - 1, "%lu", ctr2);
					diffcount++;	//Track the number of difficulties that contain any fret hand positions
					break;	//Break so that the remaining difficulties can be checked
				}
			}
		}
	}
	if(diffcount == 0)
	{
		allegro_message("No other difficulties in this track contain fret hand positions.");
		return 1;
	}

	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_track_fret_hand_positions_copy_from_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_track_fret_hand_positions_copy_from_dialog);
	if(eof_popup_dialog(eof_track_fret_hand_positions_copy_from_dialog, 1) == 2)
	{	//User clicked Copy
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		target = atol(eof_menu_song_difficulty_list_strings[eof_track_fret_hand_positions_copy_from_dialog[1].d1]);

		//Delete the active track difficulty's fret hand positions (if there are any)
		for(ctr = tp->handpositions; ctr > 0; ctr--)
		{	//For each hand position in the track (in reverse)
			if(tp->handposition[ctr - 1].difficulty == eof_note_type)
			{	//If this hand position is in the active track difficulty
				eof_clear_input();
				if(!user_warned && alert("Warning:  This track difficulty's existing fret hand positions will be replaced.", NULL, "Continue?", "&Yes", "&No", 'y', 'n') != 1)
				{	//If the user doesn't opt to replace the existing fret hand positions
					eof_cursor_visible = 1;
					eof_pen_visible = 1;
					eof_show_mouse(NULL);
					return 1;	//Return user cancellation
				}
				user_warned = 1;
				eof_pro_guitar_track_delete_hand_position(tp, ctr - 1);	//Delete it
			}
		}

		//Copy the target difficulty's fret hand positions
		for(ctr = 0; ctr < tp->handpositions; ctr++)
		{	//For each hand position in the track
			if(tp->handposition[ctr].difficulty == target)
			{	//If this hand position is in the difficulty selected by the user
				(void) eof_track_add_section(eof_song, eof_selected_track, EOF_FRET_HAND_POS_SECTION, eof_note_type, tp->handposition[ctr].start_pos, tp->handposition[ctr].end_pos, 0, NULL);	//Create a duplicate of this hand position in the active difficulty
			}
		}
	}

	eof_pro_guitar_track_sort_fret_hand_positions(tp);	//Sort the positions, since they must be in order for displaying to the user
	eof_beat_stats_cached = 0;	//Have the beat statistics rebuilt
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(NULL);
	return 1;
}

char **eof_track_manage_rs_phrases_strings = NULL;	//Stores allocated strings for eof_track_manage_rs_phrases()
unsigned long eof_track_manage_rs_phrases_strings_size = 0;	//The number of strings stored in the above array
char eof_track_manage_rs_phrases_dialog_string[25] = {0};	//The title string for the manage RS phrases dialog

char * eof_manage_rs_phrases_list(int index, int * size)
{
	unsigned long ctr, numphrases;

	switch(index)
	{
		case -1:
		{
			for(ctr = 0, numphrases = 0; ctr < eof_song->beats; ctr++)
			{	//For each beat in the chart
				if(ctr != eof_song->beats - 1)
				{	//As long as this isn't the last beat in the chart (to avoid a crash when the lack of a following beat prevents the string being built)
					if(eof_song->beat[ctr]->contained_section_event >= 0)
					{	//If this beat has a section event (RS phrase)
						numphrases++;	//Update counter
					}
				}
			}
			(void) snprintf(eof_track_manage_rs_phrases_dialog_string, sizeof(eof_track_manage_rs_phrases_dialog_string) - 1, "Manage RS phrases (%lu)", numphrases);
			if(size)
				*size = numphrases;
			break;
		}
		default:
		{
			return eof_track_manage_rs_phrases_strings[index];
		}
	}
	return NULL;
}

DIALOG eof_track_manage_rs_phrases_dialog[] =
{
	/* (proc)            (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                 (dp2) (dp3) */
	{ eof_window_proc,0,   48,  400, 237, 2,   23,  0,    0,      0,   0,   eof_track_manage_rs_phrases_dialog_string, NULL, NULL },
	{ d_agup_list_proc,  12,  84,  300, 144, 2,   23,  0,    0,      0,   0,   (void *)eof_manage_rs_phrases_list,NULL, NULL },
	{ d_agup_push_proc,  325, 84,  68,  28,  2,   23,  'a',  D_EXIT, 0,   0,   "&Add level",        NULL, (void *)eof_track_manage_rs_phrases_add_level },
	{ d_agup_push_proc,  325, 124, 68,  28,  2,   23,  'd',  D_EXIT, 0,   0,   "&Del level",        NULL, (void *)eof_track_manage_rs_phrases_remove_level },
	{ d_agup_push_proc,  325, 164, 68,  28,  2,   23,  's',  D_EXIT, 0,   0,   "&Seek to",          NULL, (void *)eof_track_manage_rs_phrases_seek },
	{ d_agup_button_proc,12,  245, 240, 28,  2,   23,  '\r', D_EXIT, 0,   0,   "Done",              NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

void eof_rebuild_manage_rs_phrases_strings(void)
{
	unsigned long ctr, index, numphrases;
	size_t stringlen;
	unsigned long startpos = 0, endpos = 0;		//Track the start and end position of the each instance of the phrase
	unsigned char maxdiff;
	char *currentphrase = NULL;
	char started = 0;

	if(eof_song->beats < 2)
		return;

	//Count the number of phrases in the active track
	eof_process_beat_statistics(eof_song, eof_selected_track);	//Cache section name information into the beat structures (from the perspective of the active track)
	if(eof_song->beat[eof_song->beats - 1]->contained_section_event >= 0)
	{	//If the last beat in the project has a phrase
		eof_truncate_chart(eof_song);	//Append some beats to allow the logic below to function properly
		eof_process_beat_statistics(eof_song, eof_selected_track);	//Update beat stats
	}
	for(ctr = 0, numphrases = 0; ctr < eof_song->beats; ctr++)
	{	//For each beat in the chart
		if(eof_song->beat[ctr]->contained_section_event >= 0)
		{	//If this beat has a section event (RS phrase)
			numphrases++;	//Update counter
		}
	}

	if(numphrases)
	{	//If at least one RS phrase was present
		eof_track_manage_rs_phrases_strings = malloc(sizeof(char *) * numphrases);	//Allocate enough pointers to have one for each phrase
	}
	else
	{
		eof_track_manage_rs_phrases_strings = malloc(sizeof(char *));	//Otherwise allocate a non zero amount of bytes to satisfy Clang scan-build
	}
	if(!eof_track_manage_rs_phrases_strings)
	{
		allegro_message("Error allocating memory");
		return;
	}
	for(ctr = 0, index = 0; ctr < eof_song->beats; ctr++)
	{	//For each beat in the chart
		if((eof_song->beat[ctr]->contained_section_event < 0) && ((ctr + 1 < eof_song->beats) || !started))
			continue;	//If this beat has no section event (RS phrase) and no phrase is in progress or this isn't the last beat, skip it

		//Otherwise it marks the end of any current phrase and the potential start of another
		if(currentphrase)
		{	//If another phrase has been read
///			started = 0;   //Any ongoing phrase ends whenever the next starts, so setting this condition to 0 is meaningless
			endpos = eof_song->beat[ctr]->pos - 1;	//Track this as the end position of the previous phrase marker
			maxdiff = eof_find_fully_leveled_rs_difficulty_in_time_range(eof_song, eof_selected_track, startpos, endpos, 1);	//Find the maxdifficulty value for this phrase instance, converted to relative numbering
			stringlen = (size_t)snprintf(NULL, 0, "%s : maxDifficulty = %u", currentphrase, maxdiff) + 1;	//Find the number of characters needed to snprintf this string
			eof_track_manage_rs_phrases_strings[index] = malloc(stringlen + 1);	//Allocate memory to build the string
			if(!eof_track_manage_rs_phrases_strings[index])
			{
				allegro_message("Error allocating memory");
				for(ctr = 0; ctr < index; ctr++)
				{	//Free previously allocated strings
					free(eof_track_manage_rs_phrases_strings[ctr]);
				}
				free(eof_track_manage_rs_phrases_strings);
				eof_track_manage_rs_phrases_strings = NULL;
				return;
			}
			(void) snprintf(eof_track_manage_rs_phrases_strings[index], stringlen, "%s : maxDifficulty = %u", currentphrase, maxdiff);
			index++;
		}
		started = 1;
		startpos = eof_song->beat[ctr]->pos;	//Track the starting position of the phrase
		currentphrase = eof_song->text_event[eof_song->beat[ctr]->contained_section_event]->text;	//Track which phrase is being examined
	}
	eof_track_manage_rs_phrases_strings_size = index;
}

int eof_track_manage_rs_phrases(void)
{
	unsigned long ctr;
	if(!eof_song || (eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return 1;	//Do not allow this function to run if a pro guitar track isn't active

	//Allocate and build the strings for the phrases
	eof_rebuild_manage_rs_phrases_strings();
	eof_track_manage_rs_phrases_dialog[1].d1 = eof_find_effective_rs_phrase(eof_music_pos.value - eof_av_delay);	//Pre-select the phrase in effect at the current position

	//Call the dialog
	eof_color_dialog(eof_track_manage_rs_phrases_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_track_manage_rs_phrases_dialog);
	(void) eof_popup_dialog(eof_track_manage_rs_phrases_dialog, 0);

	//Cleanup
	for(ctr = 0; ctr < eof_track_manage_rs_phrases_strings_size; ctr++)
	{	//Free previously allocated strings
		free(eof_track_manage_rs_phrases_strings[ctr]);
	}
	free(eof_track_manage_rs_phrases_strings);
	eof_track_manage_rs_phrases_strings = NULL;

	return 1;
}

int eof_track_manage_rs_phrases_seek(DIALOG * d)
{
	unsigned long ctr, numphrases;
	int junk;

	if(!d)
	{	//Satisfy Splint by checking value of d
		return D_O_K;
	}
	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return D_O_K;
	if(eof_track_manage_rs_phrases_dialog[1].d1 < 0)
		return D_O_K;

	for(ctr = 0, numphrases = 0; ctr < eof_song->beats; ctr++)
	{	//For each beat in the chart
		if(eof_song->beat[ctr]->contained_section_event >= 0)
		{	//If this beat has a section event (RS phrase)
			if((unsigned long)eof_track_manage_rs_phrases_dialog[1].d1 == numphrases)
			{	//If we've reached the item that is selected, seek to it
				eof_set_seek_position(eof_song->beat[ctr]->pos + eof_av_delay);	//Seek to the beat containing the phrase
				eof_render();	//Redraw screen
				(void) dialog_message(eof_track_manage_rs_phrases_dialog, MSG_DRAW, 0, &junk);	//Redraw dialog
				return D_O_K;
			}

			numphrases++;	//Update counter
		}
	}
	return D_O_K;
}

int eof_track_manage_rs_phrases_add_level(DIALOG * d)
{
	if(!d)
	{	//Satisfy Splint by checking value of d
		return D_O_K;
	}

	return eof_track_manage_rs_phrases_add_or_remove_level(1);	//Call the add level logic
}

int eof_track_manage_rs_phrases_remove_level(DIALOG * d)
{
	if(!d)
	{	//Satisfy Splint by checking value of d
		return D_O_K;
	}

	return eof_track_manage_rs_phrases_add_or_remove_level(-1);	//Call the remove level logic
}

int eof_track_manage_rs_phrases_add_or_remove_level(int function)
{
	unsigned long ctr, numphrases, targetbeat = 0, instancectr, tracknum;
	unsigned long startpos, endpos;		//Track the start and end position of the each instance of the phrase
	char *phrasename = NULL, undo_made = 0;
	char started = 0;
	EOF_PRO_GUITAR_TRACK *tp;

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return D_O_K;
	if(eof_track_manage_rs_phrases_dialog[1].d1 < 0)
		return D_O_K;

	//Identify the phrase that was selected
	for(ctr = 0, numphrases = 0; ctr < eof_song->beats; ctr++)
	{	//For each beat in the chart
		if(eof_song->beat[ctr]->contained_section_event >= 0)
		{	//If this beat has a section event (RS phrase)
			if((unsigned long)eof_track_manage_rs_phrases_dialog[1].d1 == numphrases)
			{	//If we've reached the selected phrase
				targetbeat = ctr;			//Track the beat containing the selected phrase
				phrasename = eof_song->text_event[eof_song->beat[ctr]->contained_section_event]->text;	//Track the name of the phrase
				break;
			}
			else
			{
				numphrases++;	//Update counter
			}
		}
	}

	//Recheck to see if there are multiple instances of the selected phrase, and if so, prompt the user whether to alter all of them
	for(ctr = 0, instancectr = 0; ctr < eof_song->beats; ctr++)
	{	//For each beat in the chart
		if(eof_song->beat[ctr]->contained_section_event >= 0)
		{	//If this beat has a section event (RS phrase)
			if(!ustrcmp(eof_song->text_event[eof_song->beat[ctr]->contained_section_event]->text, phrasename))
			{	//If the section event matched the one that was selected
				instancectr++;	//Increment counter
			}
		}
	}
	ctr = targetbeat;	//Parse the beat starting from the one that had the selected phrase
	if(instancectr > 1)
	{
		eof_clear_input();
		if(alert(NULL, "Modify all instances of the selected phrase?", NULL, "&Yes", "&No", 'y', 'n') == 1)
		{	//If the user opts to increase the level of all instances of the selected phrase
			ctr = 0;	//Parse the beats below starting from the first
		}
		else
		{	//only modify the selected instance
			instancectr = 0;	//Reset this variable
		}
	}

	//Modify the selected phrase instance(s)
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	tp = eof_song->pro_guitar_track[tracknum];
	startpos = endpos = 0;	//Reset these to indicate that a phrase is being looked for
	for(; ctr < eof_song->beats; ctr++)
	{	//For each beat in the chart (starting from the one ctr is referring to)
		if((eof_song->beat[ctr]->contained_section_event < 0) && ((ctr + 1 < eof_song->beats) || !started))
			continue;	//If this beat has no section event (RS phrase) and no phrase is in progress or this isn't the last beat, skip it

		//Otherwise it marks the end of any current phrase and the potential start of another
		if(started)
		{	//If this beat marks the end of a phrase instance that needs to be modified
			started = 0;
			endpos = eof_song->beat[ctr]->pos - 1;	//Track this as the end position of the previous phrase marker

			(void) eof_enforce_rs_phrase_begin_with_fret_hand_position(eof_song, eof_selected_track, eof_note_type, startpos, endpos, &undo_made, 0);
				//Ensure there is a fret hand position defined in this phrase at or before its first note
			eof_track_add_or_remove_track_difficulty_content_range(eof_song, eof_selected_track, eof_note_type, startpos, endpos, function, 0, &undo_made);
				//Level up/down the content of this time range of the track difficulty

			if(!instancectr)
			{	//If only the selected phrase instance was to be modified
				break;	//Exit loop
			}
			startpos = endpos = 0;	//Reset these to indicate that a phrase is being looked for
		}//If this beat marks the end of a phrase instance that needs to be modified

		if(eof_song->beat[ctr]->contained_section_event >= 0)
		{	//If this beat has a phrase assigned to it
			if(!ustrcmp(eof_song->text_event[eof_song->beat[ctr]->contained_section_event]->text, phrasename))
			{	//If the phrase matched the one that was selected to be leveled up
				started = 1;
				startpos = eof_song->beat[ctr]->pos;	//Track the starting position of the phrase
			}
		}
	}//For each beat in the chart (starting from the one ctr is referring to)

	if(!undo_made)
	{	//If no notes were within the selected phrase instance(s)
		allegro_message("The selected phrase instance(s) had no notes at or above the current difficulty, so no alterations were performed.");
	}
	else
	{	//Otherwise remove the difficulty limit since this operation has modified the chart
		eof_track_sort_notes(eof_song, eof_selected_track);
		eof_pro_guitar_track_sort_fret_hand_positions(tp);	//Sort the positions, since they must be in order for displaying to the user
		eof_song->track[eof_selected_track]->flags |= EOF_TRACK_FLAG_UNLIMITED_DIFFS;	//Remove the difficulty limit for this track
		eof_song->track[eof_selected_track]->numdiffs = eof_detect_difficulties(eof_song, eof_selected_track);	//Update the number of difficulties used in this track
		if(eof_note_type >= eof_song->track[eof_selected_track]->numdiffs)
		{	//If the active track difficulty is now invalid
			eof_note_type = eof_song->track[eof_selected_track]->numdiffs - 1;	//Change to the current highest difficulty
		}
		eof_determine_phrase_status(eof_song, eof_selected_track);	//Update note flags, since notes in a new level may no longer be within tremolos
	}

	eof_render();

	//Release and rebuild the strings for the dialog menu
	for(ctr = 0; ctr < eof_track_manage_rs_phrases_strings_size; ctr++)
	{	//Free previously allocated strings
		free(eof_track_manage_rs_phrases_strings[ctr]);
	}
	free(eof_track_manage_rs_phrases_strings);
	eof_track_manage_rs_phrases_strings = NULL;
	eof_rebuild_manage_rs_phrases_strings();

	return D_REDRAW;	//Have Allegro redraw the dialog
}

char eof_track_flatten_difficulties_threshold[10] = "2";
DIALOG eof_track_flatten_difficulties_dialog[] =
{
	/* (proc)                (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                          (dp2) (dp3) */
	{ eof_window_proc,    0,   0,   200, 132, 0,   0,   0,    0,      0,   0,   "Merge notes that are within", NULL, NULL },
	{ d_agup_text_proc,      12,  40,  60,  12,  0,   0,   0,    0,      0,   0,   "This # of ms:",NULL, NULL },
	{ eof_verified_edit_proc,12,  56,  90,  20,  0,   0,   0,    0,      7,   0,   eof_track_flatten_difficulties_threshold,     "0123456789",  NULL },
	{ d_agup_button_proc,    12,  92,  84,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",                         NULL, NULL },
	{ d_agup_button_proc,    110, 92,  78,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",                     NULL, NULL },
	{ NULL,                  0,   0,   0,   0,   0,   0,   0,    0,      0,   0,   NULL,                         NULL, NULL }
};

int eof_track_flatten_difficulties(void)
{
	unsigned long threshold;

	if(!eof_song || !eof_song_loaded)
		return 1;

	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_track_flatten_difficulties_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_track_flatten_difficulties_dialog);

	if(eof_popup_dialog(eof_track_flatten_difficulties_dialog, 2) == 3)	//User hit OK
	{
		if(eof_track_flatten_difficulties_threshold[0] != '\0')
		{	//If a threshold was specified
			threshold = atol(eof_track_flatten_difficulties_threshold);
			eof_flatten_difficulties(eof_song, eof_selected_track, eof_note_type, eof_selected_track, eof_note_type, threshold);	//Flatten the difficulties in the active track into itself
		}
	}

	return 1;
}

int eof_track_unflatten_difficulties(void)
{
	if(!eof_song || !eof_song_loaded)
		return 1;

	eof_unflatten_difficulties(eof_song, eof_selected_track);	//Unflatten the difficulties in the active track

	return 1;
}

int eof_menu_track_open_strum(void)
{
	unsigned long tracknum;
	unsigned long ctr;
	char undo_made = 0;	//Set to nonzero if an undo state was saved

	if(!eof_track_is_legacy_guitar(eof_song, eof_selected_track))
		return 1;	//Don't allow this function to run unless a legacy guitar/bass track is active

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	if(eof_open_strum_enabled(eof_selected_track))
	{	//Turn off open strum notes
		eof_song->track[eof_selected_track]->flags &= ~(EOF_TRACK_FLAG_SIX_LANES);	//Clear the flag
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
				eof_clear_input();
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
		{	//For each note in the active track
			if((eof_song->legacy_track[tracknum]->note[ctr]->note & 32) && (eof_song->legacy_track[tracknum]->note[ctr]->note & ~32))
			{	//If this note uses lane 6 (open note) and at least one other lane
				if(!undo_made)
				{
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Create an undo state before making the first change
					undo_made = 1;
				}
				eof_song->legacy_track[tracknum]->note[ctr]->note = 32;	//Clear all lanes for this note except for lane 6 (open bass)
			}
		}
		eof_song->track[eof_selected_track]->flags |= EOF_TRACK_FLAG_SIX_LANES;	//Set the flag
		eof_song->legacy_track[tracknum]->numlanes = 6;
	}
	eof_scale_fretboard(0);	//Recalculate the 2D screen positioning based on the current track
	return 1;
}

int eof_menu_track_five_lane_drums(void)
{
	unsigned long tracknum;
	unsigned long tracknum2;

	if(!eof_song)
		return 1;

	if(eof_song->track[eof_selected_track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR)
		return 1;	//Do not allow this function to run when a drum track is not active

	tracknum = eof_song->track[EOF_TRACK_DRUM]->tracknum;
	tracknum2 = eof_song->track[EOF_TRACK_DRUM_PS]->tracknum;
	if(eof_five_lane_drums_enabled())
	{	//Turn off five lane drums
		eof_song->track[EOF_TRACK_DRUM]->flags &= ~(EOF_TRACK_FLAG_SIX_LANES);	//Clear the flag
		eof_song->legacy_track[tracknum]->numlanes = 5;
		eof_song->track[EOF_TRACK_DRUM_PS]->flags &= ~(EOF_TRACK_FLAG_SIX_LANES);	//Clear the flag
		eof_song->legacy_track[tracknum2]->numlanes = 5;
	}
	else
	{	//Turn on five lane drums
		eof_song->track[EOF_TRACK_DRUM]->flags |= EOF_TRACK_FLAG_SIX_LANES;	//Set the flag
		eof_song->legacy_track[tracknum]->numlanes = 6;
		eof_song->track[EOF_TRACK_DRUM_PS]->flags |= EOF_TRACK_FLAG_SIX_LANES;	//Set the flag
		eof_song->legacy_track[tracknum2]->numlanes = 6;
	}

	eof_set_3D_lane_positions(0);		//Update xchart[] by force
	eof_scale_fretboard(0);			//Recalculate the 2D screen positioning based on the current track
	return 1;
}

int eof_menu_track_unshare_drum_phrasing(void)
{
	if(!eof_song)
		return 1;

	if(eof_song->track[eof_selected_track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR)
		return 1;	//Do not allow this function to run when a drum track is not active

	eof_song->tags->unshare_drum_phrasing ^= 1;	//Toggle this boolean variable
	return 1;
}

int eof_menu_track_disable_double_bass_drums(void)
{
	if(eof_song)
		eof_song->tags->double_bass_drum_disabled ^= 1;	//Toggle this boolean variable
	eof_fix_window_title();
	return 1;
}

int eof_menu_track_drumsrock_enable_drumsrock_export(void)
{
	if(!eof_song || (eof_selected_track >= eof_song->tracks))
		return 1;	//Invalid parameters
	if(eof_song->track[eof_selected_track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR)
		return 1;	//Do not allow this function to run when a drum track is not active

	eof_song->track[eof_selected_track]->flags ^= EOF_TRACK_FLAG_DRUMS_ROCK;	//Toggle this flag
	if(eof_song->track[eof_selected_track]->flags & EOF_TRACK_FLAG_DRUMS_ROCK)
	{	//If it was just turned on
		unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;

		eof_song->track[eof_selected_track]->flags |= EOF_TRACK_FLAG_SIX_LANES;	//Set the "Enable five lane drums" feature
		eof_song->legacy_track[tracknum]->numlanes = 6;
		eof_set_3D_lane_positions(0);		//Update xchart[] by force
		eof_scale_fretboard(0);			//Recalculate the 2D screen positioning based on the current track
	}

	eof_fix_window_title();
	eof_set_color_set();		//This setting changes the color set

	return 1;
}

int eof_menu_track_drumsrock_enable_remap(void)
{
	if(!eof_song || (eof_selected_track >= eof_song->tracks))
		return 1;	//Invalid parameters
	if(eof_song->track[eof_selected_track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR)
		return 1;	//Do not allow this function to run when a drum track is not active

	eof_song->track[eof_selected_track]->flags ^= EOF_TRACK_FLAG_DRUMS_ROCK_REMAP;	//Toggle this flag
	if(eof_song->track[eof_selected_track]->flags & EOF_TRACK_FLAG_DRUMS_ROCK_REMAP)
	{	//If it was just turned on, enable Drums Rock export automatically
		unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;

		eof_song->track[eof_selected_track]->flags |= EOF_TRACK_FLAG_DRUMS_ROCK;
		eof_song->track[eof_selected_track]->flags |= EOF_TRACK_FLAG_SIX_LANES;	//Set the "Enable five lane drums" feature
		eof_song->legacy_track[tracknum]->numlanes = 6;
		eof_set_3D_lane_positions(0);		//Update xchart[] by force
		eof_scale_fretboard(0);			//Recalculate the 2D screen positioning based on the current track
	}

	eof_fix_window_title();
	eof_set_color_set();		//This setting changes the color set

	return 1;
}

int eof_menu_track_beatable_enable_beatable_export(void)
{
	if(!eof_song || (eof_selected_track >= eof_song->tracks))
		return 1;	//Invalid parameters
	if(eof_song->track[eof_selected_track]->track_format != EOF_LEGACY_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a legacy track is not active
	if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
		return 1;	//Do not allow this function to run when a drum track is active

	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	eof_song->track[eof_selected_track]->flags ^= EOF_TRACK_FLAG_BEATABLE;	//Toggle this flag
	if(eof_track_is_beatable_mode(eof_song, eof_selected_track))
	{	//If it was just turned on
		unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;

		eof_song->track[eof_selected_track]->flags |= EOF_TRACK_FLAG_SIX_LANES;	//Set the "Enable open strums" feature
		eof_song->legacy_track[tracknum]->numlanes = 6;
		eof_set_3D_lane_positions(0);		//Update xchart[] by force
		eof_scale_fretboard(0);			//Recalculate the 2D screen positioning based on the current track
		eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Ensure necessary statuses such as crazy and disjointed are applied immediately
	}

	eof_fix_window_title();
	eof_set_color_set();		//This setting changes the color set

	return 1;
}

int eof_track_erase_track_difficulty(void)
{
	if(!eof_song || (eof_selected_track >= eof_song->tracks))
		return 0;	//Error

	(void) eof_detect_difficulties(eof_song, eof_selected_track);
	if(!eof_track_diff_populated_status[eof_note_type])
	{	//If this track's difficulty isn't populated by the current note set (either normal or tech notes)
		(void) eof_menu_track_toggle_tech_view();	//Change the note set if applicable (automatically calls eof_detect_difficulties() )
		if(!eof_track_diff_populated_status[eof_note_type])
		{	//If this track's difficulty isn't populated by either normal or tech notes
			(void) eof_menu_track_toggle_tech_view();	//Change the note set back if applicable
			return 1;	//Cancel
		}
		(void) eof_menu_track_toggle_tech_view();	//Change the note set back if applicable
	}

	eof_clear_input();
	if(alert(NULL, "This operation will erase this track difficulty's contents.", "Continue?", "&Yes", "&No", 'y', 'n') != 1)
	{	//If the user does not opt to erase the track difficulty
		return 1;	//Cancel
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	eof_erase_track_difficulty(eof_song, eof_selected_track, eof_note_type);
	(void) eof_detect_difficulties(eof_song, eof_selected_track);
	eof_reset_lyric_preview_lines();
	return 1;
}

int eof_track_erase_track(void)
{
	if(!eof_song || (eof_selected_track >= eof_song->tracks))
		return 0;	//Error

	eof_clear_input();
	if(!eof_get_track_size_all(eof_song, eof_selected_track))
	{	//If this track isn't populated in either note set (either normal or tech notes)
		return 1;
	}

	eof_clear_input();
	if(alert(NULL, "This operation will erase this track's contents.", "Continue?", "&Yes", "&No", 'y', 'n') != 1)
	{	//If user does not opt to erase the track
		return 1;	//Cancel
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	eof_erase_track(eof_song, eof_selected_track, 1);
	(void) eof_detect_difficulties(eof_song, eof_selected_track);
	eof_reset_lyric_preview_lines();
	return 1;
}

int eof_menu_track_remove_highlighting(void)
{
	if(!eof_song || (eof_selected_track >= eof_song->tracks))
		return 0;	//Error

	if(eof_song->tags->highlight_unsnapped_notes)
	{	//If the feature to highlight unsnapped notes is enabled, disable it now
		eof_song->tags->highlight_unsnapped_notes = 0;
	}
	if(eof_song->tags->highlight_arpeggios)
	{	//If the feature to highlight notes in arpeggios is enabled, disable it now
		eof_song->tags->highlight_arpeggios = 0;
	}
	eof_track_remove_highlighting(eof_song, eof_selected_track, 2);	//Remove all highlighting
	return 1;
}

char eof_track_rs_tone_change_add_dialog_string[40] = {0};
DIALOG eof_track_rs_tone_change_add_dialog[] =
{
	/* (proc)                (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                     (dp2) (dp3) */
	{ eof_window_proc,    0,   0,   230, 161, 0,   0,   0,    0,      0,   0,   "Rocksmith tone change", NULL, NULL },
	{ d_agup_text_proc,      12,  30,  60,  12,  0,   0,   0,    0,      0,   0,   "Tone key name:",        NULL, NULL },
	{ eof_edit_proc,      12,  46,  176, 20,  2,   23,  0,    0,      EOF_SECTION_NAME_LENGTH,   0, eof_etext, NULL, NULL },
	{ d_agup_text_proc,      12,  70,  60,  12,  0,   0,   0,    0,      0,   0,   "Tone changes should not occur",        NULL, NULL },
	{ d_agup_text_proc,      50,  85,  60,  12,  0,   0,   0,    0,      0,   0,   "on top of a note.",        NULL, NULL },
	{ d_agup_text_proc,      12,  100,  60,  12,  0,   0,   0,    0,      0,   0,   eof_track_rs_tone_change_add_dialog_string,        NULL, NULL },
	{ d_agup_button_proc,    12,  121,  84,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",                    NULL, NULL },
	{ d_agup_button_proc,    140, 121,  78,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",                NULL, NULL },
	{ NULL,                  0,   0,   0,   0,   0,   0,   0,    0,      0,   0,   NULL,                    NULL, NULL }
};

int eof_track_rs_tone_change_add_at_timestamp(unsigned long timestamp)
{
	EOF_PRO_GUITAR_TRACK *tp;
	unsigned long tracknum, ctr;
	char defaulttone = 0;

	if(!eof_song_loaded || !eof_song || (eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return 1;	//Do not allow this function to run if a pro guitar track isn't active

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	tp = eof_song->pro_guitar_track[tracknum];
	eof_render();

	//Check to see if the given timestamp occurs on a note in the active track, but only if this logic isn't suppressed
	if(!eof_dont_restrict_tone_change_timing)
	{	//If the user did not enable the "Don't restrict tone change timing" pro guitar preference
		eof_track_rs_tone_change_add_dialog[3].flags = D_HIDDEN;		//The warning about a tone change occurring on top of a note is hidden unless this condition is found to be happening
		eof_track_rs_tone_change_add_dialog[4].flags = D_HIDDEN;
		eof_track_rs_tone_change_add_dialog[5].flags = D_HIDDEN;
		for(ctr = 0; ctr < tp->pgnotes; ctr++)
		{	//For each normal note
			if((timestamp >= tp->pgnote[ctr]->pos) && (timestamp < tp->pgnote[ctr]->pos + tp->pgnote[ctr]->length))
			{	//If the specified timestamp overlaps this note (except for the last 1ms of the note)
				eof_track_rs_tone_change_add_dialog[3].flags = 0;	//Show this warning
				eof_track_rs_tone_change_add_dialog[4].flags = 0;
				eof_track_rs_tone_change_add_dialog[5].flags = 0;
				if(tp->pgnote[ctr]->pos > 0)
				{	//If the overlapped note already begins at 0s, the tone change can't occur any earlier
					timestamp = tp->pgnote[ctr]->pos - 1;
					snprintf(eof_track_rs_tone_change_add_dialog_string, sizeof(eof_track_rs_tone_change_add_dialog_string) - 1, "Adjusting tone position to %lums", timestamp);
				}
			}
		}
	}
	else
	{	//Suppress the message related to altering the tone change timestamp
		eof_track_rs_tone_change_add_dialog[3].flags = D_HIDDEN;
		eof_track_rs_tone_change_add_dialog[4].flags = D_HIDDEN;
		eof_track_rs_tone_change_add_dialog[5].flags = D_HIDDEN;
	}

	//Find the tone change at the current seek position, if any
	for(ctr = 0; ctr < tp->tonechanges; ctr++)
	{	//For each tone change in the track
		if(tp->tonechange[ctr].start_pos != timestamp)
			continue;	//If this tone change is not at the current seek position, skip it

		//Otherwise edit it instead of adding a new tone change
		eof_color_dialog(eof_track_rs_tone_change_add_dialog, gui_fg_color, gui_bg_color);
		eof_conditionally_center_dialog(eof_track_rs_tone_change_add_dialog);
		(void) ustrcpy(eof_etext, tp->tonechange[ctr].name);
		eof_clear_input();
		if(eof_popup_dialog(eof_track_rs_tone_change_add_dialog, 2) == 6)
		{	//User clicked OK
			if(eof_etext[0] != '\0')
			{	//If a tone key name is specified
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				(void) ustrcpy(tp->tonechange[ctr].name, eof_etext);	//Update the tone name string
				tp->tonechange[ctr].name[EOF_SECTION_NAME_LENGTH] = '\0';	//Guarantee NULL termination
			}
		}
		return 1;
	}

	eof_color_dialog(eof_track_rs_tone_change_add_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_track_rs_tone_change_add_dialog);

	(void) ustrcpy(eof_etext, "");
	if(eof_popup_dialog(eof_track_rs_tone_change_add_dialog, 2) == 6)
	{	//User clicked OK
		if(eof_etext[0] != '\0')
		{	//If a tone key name is specified
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			if((tp->defaulttone[0] != '\0') && (!strcmp(tp->defaulttone, eof_etext)))
			{	//If the tone being changed to is the currently defined default tone
					defaulttone = 1;
			}
			(void) eof_track_add_section(eof_song, eof_selected_track, EOF_RS_TONE_CHANGE, 0, timestamp, defaulttone, 0, eof_etext);
		}
		eof_track_pro_guitar_sort_tone_changes(tp);	//Re-sort the tone changes
	}

	return 1;
}

int eof_track_rs_tone_change_add(void)
{
	return eof_track_rs_tone_change_add_at_timestamp(eof_music_pos.value - eof_av_delay);
}

int eof_track_rs_tone_change_add_at_mouse(void)
{
	if(eof_pen_note.pos < eof_chart_length)
	{	//If the pen note is at a valid position
		return eof_track_rs_tone_change_add_at_timestamp(eof_pen_note.pos);
	}

	return 0;
}

char **eof_track_rs_tone_changes_list_strings = NULL;	//Stores allocated strings for eof_track_rs_tone_changes()
char eof_track_rs_tone_changes_dialog_undo_made = 0;	//Used to track whether an undo state was made in this dialog
char eof_track_rs_tone_changes_dialog_string[35] = {0};	//The title string for the RS tone changes dialog

DIALOG eof_track_rs_tone_changes_dialog[] =
{
	/* (proc)            (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)            (dp2) (dp3) */
	{ eof_window_proc,0,   48,  400, 237, 2,   23,  0,    0,      0,   0,   eof_track_rs_tone_changes_dialog_string,NULL, NULL },
	{ d_agup_list_proc,  12,  84,  300, 138, 2,   23,  0,    0,      0,   0,   (void *)eof_track_rs_tone_changes_list,NULL, NULL },
	{ d_agup_push_proc,  320, 84,  68,  28,  2,   23,  'd',  D_EXIT, 0,   0,   "&Delete",      NULL, (void *)eof_track_rs_tone_changes_delete },
	{ d_agup_push_proc,  320, 124, 68,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Delete all",   NULL, (void *)eof_track_rs_tone_changes_delete_all },
	{ d_agup_button_proc,320, 164, 68,  28,  2,   23,  'n',  D_EXIT, 0,   0,   "&Names",       NULL, NULL },
	{ d_agup_push_proc,  320, 204, 68,  28,  2,   23,  's',  D_EXIT, 0,   0,   "&Seek to",     NULL, (void *)eof_track_rs_tone_changes_seek },
	{ d_agup_push_proc,  320, 244, 68,  28,  2,   23,  'e',  D_EXIT, 0,   0,   "&Edit",        NULL, (void *)eof_track_rs_tone_changes_edit },
	{ d_agup_button_proc,12,  244, 90,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "Done",         NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

char * eof_track_rs_tone_changes_list(int index, int * size)
{
	switch(index)
	{
		case -1:
		{
			unsigned long tonechanges = eof_song->pro_guitar_track[eof_song->track[eof_selected_track]->tracknum]->tonechanges;
			if(size)
				*size = tonechanges;
			(void) snprintf(eof_track_rs_tone_changes_dialog_string, sizeof(eof_track_rs_tone_changes_dialog_string) - 1, "Rocksmith tone changes (%lu)", tonechanges);
			break;
		}
		default:
		{
			return eof_track_rs_tone_changes_list_strings[index];
		}
	}
	return NULL;
}

void eof_track_rebuild_rs_tone_changes_list_strings(void)
{
	EOF_PRO_GUITAR_TRACK *tp;
	unsigned long tracknum, ctr;
	size_t stringlen;
	char *suffix, blank[] = "", def[] = " (D)", defaultfound = 0;

	if(!eof_song_loaded || !eof_song || (eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return;	//Do not allow this function to run if a chart is not loaded or a pro guitar/bass track is not active

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	tp = eof_song->pro_guitar_track[tracknum];
	eof_track_rs_tone_changes_list_strings = malloc(sizeof(char *) * tp->tonechanges);	//Allocate enough pointers to have one for each tone change

	for(ctr = 0; ctr < tp->tonechanges; ctr++)
	{	//For each tone change
		stringlen = (size_t)snprintf(NULL, 0, "%lums : %s", tp->tonechange[ctr].start_pos, tp->tonechange[ctr].name) + 1;	//Find the number of characters needed to snprintf this string
		suffix = blank;
		if(!strcmp(tp->tonechange[ctr].name, tp->defaulttone))
		{	//If this tone is the track's default tone
			suffix = def;	//Append the suffix denoting the default tone
			stringlen += strlen(def);
			defaultfound = 1;	//Track that at least one tone change still uses the default tone
		}
		eof_track_rs_tone_changes_list_strings[ctr] = malloc(stringlen);	//Allocate memory to build the string
		if(!eof_track_rs_tone_changes_list_strings[ctr])
		{
			allegro_message("Error allocating memory");
			while(ctr > 0)
			{	//Free previously allocated strings
				free(eof_track_rs_tone_changes_list_strings[ctr - 1]);
				ctr--;
			}
			free(eof_track_rs_tone_changes_list_strings);
			eof_track_rs_tone_changes_list_strings = NULL;
			return;
		}
		(void) snprintf(eof_track_rs_tone_changes_list_strings[ctr], stringlen, "%lums : %s%s", tp->tonechange[ctr].start_pos, tp->tonechange[ctr].name, suffix);
	}
	if(!defaultfound)
	{	//If no tone changes in the arrangement use the default tone anymore (ie. all the ones that did were deleted)
		tp->defaulttone[0] = '\0';	//Truncate the default tone string
	}
}

int eof_track_find_effective_rs_tone_change(unsigned long pos, unsigned long *changenum)
{
	EOF_PRO_GUITAR_TRACK *tp;
	unsigned long tracknum, ctr;
	int found = 0;

	if(!eof_song || (eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT) || !changenum)
		return 0;	//Return false if a pro guitar track isn't active or parameters are invalid

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	tp = eof_song->pro_guitar_track[tracknum];

	for(ctr = 0; ctr < tp->tonechanges; ctr++)
	{	//For each tone change
		if(pos >= tp->tonechange[ctr].start_pos)
		{	//If the specified position is at or after this tone change
			*changenum = ctr;	//Store the result
			found = 1;
		}
	}

	return found;	//Return found status
}

int eof_track_rs_tone_changes(void)
{
	EOF_PRO_GUITAR_TRACK *tp;
	unsigned long tracknum, ctr, tonechange = 0;
	int retval;

	if(!eof_song || (eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return 1;	//Do not allow this function to run if a pro guitar track isn't active

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	tp = eof_song->pro_guitar_track[tracknum];
	eof_track_pro_guitar_sort_tone_changes(tp);	//Re-sort the tone changes

	//Allocate and build the strings for the tone changes
	eof_track_rebuild_rs_tone_changes_list_strings();
	if(eof_track_find_effective_rs_tone_change(eof_music_pos.value - eof_av_delay, &tonechange))
	{	//If a tone change had been placed at or before the current seek position
		eof_track_rs_tone_changes_dialog[1].d1 = tonechange;	//Pre-select the tone change from the list
	}

	//Call the dialog
	eof_clear_input();
	eof_track_rs_tone_changes_dialog_undo_made = 0;	//Reset this condition
	eof_color_dialog(eof_track_rs_tone_changes_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_track_rs_tone_changes_dialog);
	retval = eof_popup_dialog(eof_track_rs_tone_changes_dialog, 1);

	//Cleanup
	for(ctr = 0; ctr < tp->tonechanges; ctr++)
	{	//Free previously allocated strings
		free(eof_track_rs_tone_changes_list_strings[ctr]);
	}
	free(eof_track_rs_tone_changes_list_strings);
	eof_track_rs_tone_changes_list_strings = NULL;

	//Switch to tone name dialog if necessary
	if(retval == 4)
	{	//If the user clicked the Names button
		return eof_track_rs_tone_names();	//Call the Rocksmith Tone Names dialog
	}

	return 1;
}

void eof_track_pro_guitar_sort_tone_changes(EOF_PRO_GUITAR_TRACK* tp)
{
 	eof_log("eof_track_pro_guitar_sort_tone_changes() entered", 1);

	if(tp)
	{
		qsort(tp->tonechange, (size_t)tp->tonechanges, sizeof(EOF_PHRASE_SECTION), eof_song_qsort_phrase_sections);
	}
}

void eof_track_pro_guitar_delete_tone_change(EOF_PRO_GUITAR_TRACK *tp, unsigned long index)
{
	unsigned long ctr;
 	eof_log("eof_track_pro_guitar_delete_tone_change() entered", 1);

	if(tp && (index < tp->tonechanges))
	{
		tp->tonechange[index].name[0] = '\0';	//Empty the name string
		for(ctr = index; ctr < tp->tonechanges; ctr++)
		{
			memcpy(&tp->tonechange[ctr], &tp->tonechange[ctr + 1], sizeof(EOF_PHRASE_SECTION));
		}
		tp->tonechanges--;
	}
}

int eof_track_rs_tone_changes_delete(DIALOG * d)
{
	EOF_PRO_GUITAR_TRACK *tp;
	unsigned long tracknum, ctr, ctr2;

	if(!d)
	{	//Satisfy Splint by checking value of d
		return D_O_K;
	}
	if(!eof_song || (eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return D_O_K;	//Do not allow this function to run if a pro guitar track isn't active
	if(eof_track_rs_tone_changes_dialog[1].d1 < 0)
		return D_O_K;

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	tp = eof_song->pro_guitar_track[tracknum];
	if(tp->tonechanges == 0)
		return D_O_K;

	for(ctr = 0; ctr < tp->tonechanges; ctr++)
	{	//For each tone change
		if(ctr != (unsigned long)eof_track_rs_tone_changes_dialog[1].d1)
			continue;	//If this is not the selected tone change, skip it

		if(!eof_track_rs_tone_changes_dialog_undo_made)
		{	//If an undo state hasn't been made yet since launching this dialog
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			eof_track_rs_tone_changes_dialog_undo_made = 1;
		}

		//Release strings
		for(ctr2 = 0; ctr2 < tp->tonechanges; ctr2++)
		{	//Free previously allocated strings
			free(eof_track_rs_tone_changes_list_strings[ctr2]);
		}
		free(eof_track_rs_tone_changes_list_strings);
		eof_track_rs_tone_changes_list_strings = NULL;

		/* remove the tone change, update the selection in the list box and exit */
		eof_track_pro_guitar_delete_tone_change(tp, ctr);
		eof_track_pro_guitar_sort_tone_changes(tp);	//Re-sort the remaining tone changes
		if(((unsigned long)eof_track_rs_tone_changes_dialog[1].d1 >= tp->tonechanges) && (tp->tonechanges > 0))
		{	//If the last list item was deleted and others remain
			eof_track_rs_tone_changes_dialog[1].d1--;	//Select the one before the one that was deleted, or the last message, whichever one remains
		}
	}

	eof_render();	//Redraw screen
	eof_track_rebuild_rs_tone_changes_list_strings();	//Rebuild the strings for the dialog menu

	return D_REDRAW;	//Have Allegro redraw the dialog
}

int eof_track_rs_tone_changes_edit(DIALOG * d)
{
	unsigned long i, tracknum;
	EOF_PHRASE_SECTION *ptr;
	EOF_PRO_GUITAR_TRACK *tp;

	if(!d)
	{	//Satisfy Splint by checking value of d
		return D_O_K;
	}

	if(!eof_song_loaded || !eof_song || (eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return D_O_K;	//Do not allow this function to run if a chart is not loaded or a pro guitar/bass track is not active
	if(eof_track_rs_tone_changes_dialog[1].d1 < 0)
		return D_O_K;

	eof_color_dialog(eof_track_rs_tone_change_add_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_track_rs_tone_change_add_dialog);

	//Initialize the dialog fields
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	tp = eof_song->pro_guitar_track[tracknum];
	if((unsigned long)eof_track_rs_tone_changes_dialog[1].d1 >= tp->tonechanges)
		return D_O_K;	//Invalid tone change selected in list
	ptr = &(tp->tonechange[eof_track_rs_tone_changes_dialog[1].d1]);
	(void) ustrcpy(eof_etext, ptr->name);

	eof_clear_input();
	if(eof_popup_dialog(eof_track_rs_tone_change_add_dialog, 2) == 6)
	{	//User clicked OK
		if(eof_etext[0] != '\0')
		{	//If a tone key name is specified
			if(!eof_track_rs_tone_changes_dialog_undo_made)
			{	//If an undo state hasn't been made yet since launching this dialog
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				eof_track_rs_tone_changes_dialog_undo_made = 1;
			}
			(void) ustrcpy(ptr->name, eof_etext);	//Update the tone name string
			ptr->name[EOF_SECTION_NAME_LENGTH] = '\0';	//Guarantee NULL termination
		}
	}

	eof_track_pro_guitar_sort_tone_changes(tp);	//Re-sort the tone changes

	//Release strings
	for(i = 0; i < tp->tonechanges; i++)
	{	//Free previously allocated strings
		free(eof_track_rs_tone_changes_list_strings[i]);
	}
	free(eof_track_rs_tone_changes_list_strings);
	eof_track_rs_tone_changes_list_strings = NULL;

	eof_render();	//Redraw screen

	//Rebuild the strings for the dialog menu
	eof_track_rebuild_rs_tone_changes_list_strings();

	return D_REDRAW;	//Have Allegro redraw the dialog
}

int eof_track_rs_tone_changes_delete_all(DIALOG * d)
{
	EOF_PRO_GUITAR_TRACK *tp;
	unsigned long i, tracknum;
	int junk;

	if(!d)
	{	//Satisfy Splint by checking value of d
		return D_O_K;
	}
	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return D_O_K;

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	tp = eof_song->pro_guitar_track[tracknum];
	if(tp->tonechanges == 0)
	{
		return D_O_K;
	}

	//Release strings
	for(i = 0; i < tp->tonechanges; i++)
	{	//Free previously allocated strings
		free(eof_track_rs_tone_changes_list_strings[i]);
	}
	free(eof_track_rs_tone_changes_list_strings);
	eof_track_rs_tone_changes_list_strings = NULL;

	//Delete tone changes
	for(i = tp->tonechanges; i > 0; i--)
	{	//For each tone change (in reverse order)
		if(!eof_track_rs_tone_changes_dialog_undo_made)
		{	//If an undo state hasn't been made yet since launching this dialog
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			eof_track_rs_tone_changes_dialog_undo_made = 1;
		}
		eof_track_pro_guitar_delete_tone_change(tp, i - 1);
	}
	tp->defaulttone[0] = '\0';	//Truncate the default tone string since no tone changes exist anymore

	eof_render();	//Redraw screen
	(void) dialog_message(eof_track_rs_tone_changes_dialog, MSG_START, 0, &junk);	//Re-initialize the dialog
	(void) dialog_message(eof_track_rs_tone_changes_dialog, MSG_DRAW, 0, &junk);	//Redraw dialog
	return D_REDRAW;
}

int eof_track_rs_tone_changes_seek(DIALOG * d)
{
	unsigned long tracknum;
	EOF_PRO_GUITAR_TRACK *tp;
	int junk;

	if(!d)
	{	//Satisfy Splint by checking value of d
		return D_O_K;
	}
	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return D_O_K;
	if(eof_track_rs_tone_changes_dialog[1].d1 < 0)
		return D_O_K;

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	tp = eof_song->pro_guitar_track[tracknum];
	if((unsigned long)eof_track_rs_tone_changes_dialog[1].d1 >= tp->tonechanges)
	{	//Invalid tone change selected
		return D_O_K;
	}
	eof_set_seek_position(tp->tonechange[eof_track_rs_tone_changes_dialog[1].d1].start_pos + eof_av_delay);	//Seek to the tone change's timestamp
	eof_render();	//Redraw screen
	(void) dialog_message(eof_track_rs_tone_changes_dialog, MSG_DRAW, 0, &junk);	//Redraw dialog

	return D_O_K;
}

char **eof_track_rs_tone_names_list_strings = NULL;			//Stores allocated strings for eof_track_rs_tone_names()
unsigned long eof_track_rs_tone_names_list_strings_num = 0;	//Tracks the number of strings stored in the above array
char eof_track_rs_tone_names_dialog_undo_made = 0;			//Used to track whether an undo state was made in this dialog
char eof_track_rs_tone_names_dialog_string[35] = {0};		//The title string for the RS tone names dialog

DIALOG eof_track_rs_tone_names_dialog[] =
{
	/* (proc)            (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)            (dp2) (dp3) */
	{ eof_window_proc,0,   48,  400, 237, 2,   23,  0,    0,      0,   0,   eof_track_rs_tone_names_dialog_string,NULL, NULL },
	{ d_agup_list_proc,  12,  84,  300, 138, 2,   23,  0,    0,      0,   0,   (void *)eof_track_rs_tone_names_list,NULL, NULL },
	{ d_agup_push_proc,  320, 84,  68,  28,  2,   23,  'd',  D_EXIT, 0,   0,   "&Default",     NULL, (void *)eof_track_rs_tone_names_default },
	{ d_agup_push_proc,  320, 124, 68,  28,  2,   23,  'r',  D_EXIT, 0,   0,   "&Rename",      NULL, (void *)eof_track_rs_tone_names_rename },
	{ d_agup_button_proc,320, 164, 68,  28,  2,   23,  'l',  D_EXIT, 0,   0,   "&List",        NULL, NULL },
	{ d_agup_button_proc,12,  244, 90,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "Done",         NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

char * eof_track_rs_tone_names_list(int index, int * size)
{
	switch(index)
	{
		case -1:
		{
			EOF_PRO_GUITAR_TRACK *tp;
			unsigned long tracknum, ctr, ctr2, namecount = 0;
			char unique;

			tracknum = eof_song->track[eof_selected_track]->tracknum;
			tp = eof_song->pro_guitar_track[tracknum];
			for(ctr = 0; ctr < tp->tonechanges; ctr++)
			{	//For each tone change in the active track
				unique = 1;		//Consider this tone change's name to be unique unless any of the previous tone changes use the same name
				if(ctr > 0)
				{	//If this isn't the first tone change, compare this tone change's name to the previous changes
					for(ctr2 = ctr; ctr2 > 0; ctr2--)
					{	//For each previous tone change
						if(!strcmp(tp->tonechange[ctr].name, tp->tonechange[ctr2 - 1].name))
						{	//If the tone change's name matches the name of any of the previous tone changes
							unique = 0;
							break;
						}
					}
				}
				if(unique)
				{	//If this is the first tone change of this name encountered for the track
					namecount++;
				}
			}
			if(size)
				*size = namecount;
			eof_track_rs_tone_names_list_strings_num = namecount;
			(void) snprintf(eof_track_rs_tone_names_dialog_string, sizeof(eof_track_rs_tone_names_dialog_string) - 1, "Rocksmith tone names (%lu)", namecount);
			break;
		}
		default:
		{
			return eof_track_rs_tone_names_list_strings[index];
		}
	}
	return NULL;
}

void eof_track_rebuild_rs_tone_names_list_strings(unsigned long track, char allowsuffix)
{
	EOF_PRO_GUITAR_TRACK *tp;
	unsigned long tracknum, ctr, ctr2, index = 0, temp;
	size_t stringlen;
	char unique, *suffix, blank[] = "", def[] = " (D)", defaultfound = 0;
	int junk;

	if(!eof_song_loaded || !eof_song || !track || (eof_song->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return;	//Do not allow this function to run if a chart is not loaded or a pro guitar/bass track is not active

	tracknum = eof_song->track[track]->tracknum;
	tp = eof_song->pro_guitar_track[tracknum];
	temp = eof_selected_track;	//Store the current track since eof_selected_track has to be changed in order for eof_track_rs_tone_names_list() to count tones in the correct track
	eof_selected_track = track;
	(void) eof_track_rs_tone_names_list(-1, &junk);	//Update eof_track_rs_tone_names_list_strings_num
	eof_selected_track = temp;	//Restore the original active track
	eof_track_rs_tone_names_list_strings = malloc(sizeof(char *) * eof_track_rs_tone_names_list_strings_num);	//Allocate enough pointers to have one for each tone name

	for(ctr = 0; ctr < tp->tonechanges; ctr++)
	{	//For each tone change in the active track
		unique = 1;		//Consider this tone change's name to be unique unless any of the previous tone changes use the same name
		if(ctr > 0)
		{	//If this isn't the first tone change, compare this tone change's name to the previous changes
			for(ctr2 = ctr; ctr2 > 0; ctr2--)
			{	//For each previous tone change
				if(!strcmp(tp->tonechange[ctr].name, tp->tonechange[ctr2 - 1].name))
				{	//If the tone change's name matches the name of any of the previous tone changes
					unique = 0;
					break;
				}
			}
		}
		if(!unique)
			continue;	//If this isn't the first tone change of this name encountered for the track, skip it

		//Otherwise copy it into a list
		stringlen = strlen(tp->tonechange[ctr].name) + 1;	//Unless this tone name is found to be the default, the string will just be the tone's name
		suffix = blank;
		if(!strcmp(tp->tonechange[ctr].name, tp->defaulttone))
		{	//If this tone is the track's default tone
			defaultfound = 1;	//Track that at least one tone change still uses the default tone
			if(allowsuffix)
			{	//If the calling function is allowing the default tone suffix to be appended to the string
				suffix = def;	//Append the suffix denoting the default tone
				stringlen += strlen(def);
			}
		}
		eof_track_rs_tone_names_list_strings[index] = malloc(stringlen);
		if(!eof_track_rs_tone_names_list_strings[index])
		{
			allegro_message("Error allocating memory");
			while(index > 0)
			{	//Free previously allocated strings
				free(eof_track_rs_tone_names_list_strings[index - 1]);
				index--;
			}
			free(eof_track_rs_tone_names_list_strings);
			eof_track_rs_tone_names_list_strings = NULL;
			eof_track_rs_tone_names_list_strings_num = 0;
			return;
		}
		(void) snprintf(eof_track_rs_tone_names_list_strings[index], stringlen, "%s%s", tp->tonechange[ctr].name, suffix);
		index++;
	}
	if(!defaultfound)
	{	//If no tone changes in the arrangement use the default tone anymore (ie. all the ones that did were deleted)
		tp->defaulttone[0] = '\0';	//Truncate the default tone string
	}
}

void eof_track_destroy_rs_tone_names_list_strings(void)
{
	unsigned long ctr;

	for(ctr = 0; ctr < eof_track_rs_tone_names_list_strings_num; ctr++)
	{	//Free previously allocated strings
		free(eof_track_rs_tone_names_list_strings[ctr]);
	}
	free(eof_track_rs_tone_names_list_strings);
	eof_track_rs_tone_names_list_strings = NULL;
	eof_track_rs_tone_names_list_strings_num = 0;
}

int eof_track_rs_tone_names(void)
{
	EOF_PRO_GUITAR_TRACK *tp;
	unsigned long tracknum;
	int retval;

	if(!eof_song || (eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return 1;	//Do not allow this function to run if a pro guitar track isn't active

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	tp = eof_song->pro_guitar_track[tracknum];
	eof_track_pro_guitar_sort_tone_changes(tp);	//Re-sort the tone changes

	//Allocate and build the strings for the tone names
	eof_track_rebuild_rs_tone_names_list_strings(eof_selected_track, 1);

	//Call the dialog
	eof_clear_input();
	eof_track_rs_tone_names_dialog_undo_made = 0;	//Reset this condition
	eof_color_dialog(eof_track_rs_tone_names_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_track_rs_tone_names_dialog);
	retval = eof_popup_dialog(eof_track_rs_tone_names_dialog, 1);

	//Cleanup
	eof_track_destroy_rs_tone_names_list_strings();

	//Switch to tone list dialog if necessary
	if(retval == 4)
	{	//If the user clicked the List button
		return eof_track_rs_tone_changes();	//Call the Rocksmith Tone Changes dialog
	}

	return 1;
}

int eof_track_rs_tone_names_default(DIALOG * d)
{
	EOF_PRO_GUITAR_TRACK *tp;
	unsigned long tracknum, ctr, namenum;

	if(!d)
	{	//Satisfy Splint by checking value of d
		return D_O_K;
	}
	if(!eof_song || (eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return D_O_K;	//Do not allow this function to run if a pro guitar track isn't active

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	tp = eof_song->pro_guitar_track[tracknum];
	namenum = eof_track_rs_tone_names_dialog[1].d1;
	if((tp->tonechanges == 0) || (namenum >= eof_track_rs_tone_names_list_strings_num) || (eof_track_rs_tone_names_list_strings[namenum][0] == '\0'))
		return D_O_K;

	//Rebuild the tone name strings omitting the (D) suffix, so the default tone name can be compared with other tone names
	eof_track_destroy_rs_tone_names_list_strings();
	eof_track_rebuild_rs_tone_names_list_strings(eof_selected_track, 0);
	if(strcmp(tp->defaulttone, eof_track_rs_tone_names_list_strings[namenum]))
	{	//If the default tone is being changed
		if(!eof_track_rs_tone_names_dialog_undo_made)
		{	//If an undo state hasn't been made yet since launching this dialog
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			eof_track_rs_tone_names_dialog_undo_made = 1;
		}
		strncpy(tp->defaulttone, eof_track_rs_tone_names_list_strings[namenum], EOF_SECTION_NAME_LENGTH);	//Update the defaulttone string
		for(ctr = 0; ctr < tp->tonechanges; ctr++)
		{	//For each tone change in the track
			if(!strcmp(tp->defaulttone, tp->tonechange[ctr].name))
			{	//If this tone change applies the default tone
				tp->tonechange[ctr].end_pos = 1;	//Track this condition
			}
			else
			{
				tp->tonechange[ctr].end_pos = 0;	//Reset this condition
			}
		}
	}

	//Release the tone name strings showing the (D) suffix for the default tone
	eof_track_destroy_rs_tone_names_list_strings();
	eof_track_rebuild_rs_tone_names_list_strings(eof_selected_track, 1);

	return D_REDRAW;	//Have Allegro redraw the dialog
}

int eof_track_rs_tone_names_rename(DIALOG * d)
{
	EOF_PRO_GUITAR_TRACK *tp;
	unsigned long tracknum, ctr, namenum;

	if(!d)
	{	//Satisfy Splint by checking value of d
		return D_O_K;
	}
	if(!eof_song || (eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return D_O_K;	//Do not allow this function to run if a pro guitar track isn't active

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	tp = eof_song->pro_guitar_track[tracknum];
	namenum = eof_track_rs_tone_names_dialog[1].d1;
	if((tp->tonechanges == 0) || (namenum >= eof_track_rs_tone_names_list_strings_num) || (eof_track_rs_tone_names_list_strings[namenum][0] == '\0'))
		return D_O_K;

	//Rebuild the tone name strings omitting the (D) suffix, so the correct tone name can be matched against tone changes for renaming
	eof_track_destroy_rs_tone_names_list_strings();
	eof_track_rebuild_rs_tone_names_list_strings(eof_selected_track, 0);

	//Prepare the tone change dialog so the user can edit the name
	eof_color_dialog(eof_track_rs_tone_change_add_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_track_rs_tone_change_add_dialog);
	strncpy(eof_etext, eof_track_rs_tone_names_list_strings[namenum], EOF_SECTION_NAME_LENGTH);
	if(eof_popup_dialog(eof_track_rs_tone_change_add_dialog, 2) == 6)
	{	//User clicked OK
		if((eof_etext[0] != '\0') && strcmp(eof_track_rs_tone_names_list_strings[namenum], eof_etext))
		{	//If a tone key name was specified and it is different from the key name it already had
			if(!eof_track_rs_tone_names_dialog_undo_made)
			{	//If an undo state hasn't been made yet since launching this dialog
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				eof_track_rs_tone_names_dialog_undo_made = 1;
			}
			if(!strcmp(tp->defaulttone, eof_track_rs_tone_names_list_strings[namenum]))
			{	//If the default tone is being renamed
				strncpy(tp->defaulttone, eof_etext, EOF_SECTION_NAME_LENGTH);	//Update the default tone string
			}
			for(ctr = 0; ctr < tp->tonechanges; ctr++)
			{	//For each tone change in the track
				if(!strcmp(tp->tonechange[ctr].name, eof_track_rs_tone_names_list_strings[namenum]))
				{	//If this is a tone change whose name is being changed
					strncpy(tp->tonechange[ctr].name, eof_etext, EOF_SECTION_NAME_LENGTH);	//Update the tone name
					tp->tonechange[ctr].name[sizeof(tp->tonechange[ctr].name) - 1] = '\0';	//Redundant NULL termination to satisfy Coverity
				}
			}
		}
	}

	eof_render();	//Redraw screen

	//Release the tone name strings showing the (D) suffix for the default tone
	eof_track_destroy_rs_tone_names_list_strings();
	eof_track_rebuild_rs_tone_names_list_strings(eof_selected_track, 1);

	return D_REDRAW;	//Have Allegro redraw the dialog
}

int eof_menu_track_copy_tone_changes_track_8(void)
{
	return eof_menu_track_copy_tone_changes_track_number(eof_song, 8, eof_selected_track);
}

int eof_menu_track_copy_tone_changes_track_9(void)
{
	return eof_menu_track_copy_tone_changes_track_number(eof_song, 9, eof_selected_track);
}

int eof_menu_track_copy_tone_changes_track_11(void)
{
	return eof_menu_track_copy_tone_changes_track_number(eof_song, 11, eof_selected_track);
}

int eof_menu_track_copy_tone_changes_track_12(void)
{
	return eof_menu_track_copy_tone_changes_track_number(eof_song, 12, eof_selected_track);
}

int eof_menu_track_copy_tone_changes_track_14(void)
{
	return eof_menu_track_copy_tone_changes_track_number(eof_song, 14, eof_selected_track);
}

int eof_menu_track_copy_tone_changes_track_number(EOF_SONG *sp, unsigned long sourcetrack, unsigned long desttrack)
{
	EOF_PRO_GUITAR_TRACK *stp, *dtp;

	if(!sp || (sourcetrack >= sp->tracks) || (desttrack >= sp->tracks) || (sourcetrack == desttrack))
		return 0;	//Invalid parameters
	if((sp->track[sourcetrack]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT) || (sp->track[desttrack]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return 0;	//Invalid parameters

	stp = sp->pro_guitar_track[sp->track[sourcetrack]->tracknum];
	dtp = sp->pro_guitar_track[sp->track[desttrack]->tracknum];
	if(dtp->tonechanges)
	{	//If there are already tone changes in the destination track
		eof_clear_input();
		if(alert(NULL, "Warning:  Existing tone changes in this track will be lost.  Continue?", NULL, "&Yes", "&No", 'y', 'n') != 1)
		{	//If the user does not opt to continue
			return 0;
		}
	}

	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	dtp->tonechanges = stp->tonechanges;
	memcpy(dtp->tonechange, stp->tonechange, sizeof(stp->tonechange));
	memcpy(dtp->defaulttone, stp->defaulttone, sizeof(stp->defaulttone));

	return 1;	//Return completion
}

int eof_menu_track_copy_popups_track_8(void)
{
	return eof_menu_track_copy_popups_track_number(eof_song, 8, eof_selected_track);
}

int eof_menu_track_copy_popups_track_9(void)
{
	return eof_menu_track_copy_popups_track_number(eof_song, 9, eof_selected_track);
}

int eof_menu_track_copy_popups_track_11(void)
{
	return eof_menu_track_copy_popups_track_number(eof_song, 11, eof_selected_track);
}

int eof_menu_track_copy_popups_track_12(void)
{
	return eof_menu_track_copy_popups_track_number(eof_song, 12, eof_selected_track);
}

int eof_menu_track_copy_popups_track_14(void)
{
	return eof_menu_track_copy_popups_track_number(eof_song, 14, eof_selected_track);
}

int eof_menu_track_copy_popups_track_number(EOF_SONG *sp, unsigned long sourcetrack, unsigned long desttrack)
{
	unsigned long ctr;
	EOF_PRO_GUITAR_TRACK *stp, *dtp;
	EOF_PHRASE_SECTION *ptr;

	if(!sp || (sourcetrack >= sp->tracks) || (desttrack >= sp->tracks) || (sourcetrack == desttrack))
		return 0;	//Invalid parameters
	if((sp->track[sourcetrack]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT) || (sp->track[desttrack]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return 0;	//Invalid parameters

	stp = sp->pro_guitar_track[sp->track[sourcetrack]->tracknum];
	dtp = sp->pro_guitar_track[sp->track[desttrack]->tracknum];
	if(dtp->popupmessages)
	{	//If there are already popup messages in the destination track
		eof_clear_input();
		if(alert(NULL, "Warning:  Existing popup messages in this track will be lost.  Continue?", NULL, "&Yes", "&No", 'y', 'n') != 1)
		{	//If the user does not opt to continue
			return 0;
		}
	}

	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	dtp->popupmessages = 0;
	for(ctr = 0; ctr < stp->popupmessages; ctr++)
	{	//For each popup message in the source track
		ptr = &stp->popupmessage[ctr];	//Simplify
		(void) eof_track_add_section(sp, desttrack, EOF_RS_POPUP_MESSAGE, 0, ptr->start_pos, ptr->end_pos, 0, ptr->name);	//Duplicate the message
	}

	return 1;	//Return completion
}

int eof_menu_track_copy_fret_hand_track_8(void)
{
	return eof_menu_track_copy_fret_hand_track_number(eof_song, 8, eof_selected_track);
}

int eof_menu_track_copy_fret_hand_track_9(void)
{
	return eof_menu_track_copy_fret_hand_track_number(eof_song, 9, eof_selected_track);
}

int eof_menu_track_copy_fret_hand_track_11(void)
{
	return eof_menu_track_copy_fret_hand_track_number(eof_song, 11, eof_selected_track);
}

int eof_menu_track_copy_fret_hand_track_12(void)
{
	return eof_menu_track_copy_fret_hand_track_number(eof_song, 12, eof_selected_track);
}

int eof_menu_track_copy_fret_hand_track_14(void)
{
	return eof_menu_track_copy_fret_hand_track_number(eof_song, 14, eof_selected_track);
}

int eof_menu_track_copy_fret_hand_track_number(EOF_SONG *sp, unsigned long sourcetrack, unsigned long desttrack)
{
	EOF_PRO_GUITAR_TRACK *stp, *dtp;

	if(!sp || (sourcetrack >= sp->tracks) || (desttrack >= sp->tracks) || (sourcetrack == desttrack))
		return 0;	//Invalid parameters
	if((sp->track[sourcetrack]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT) || (sp->track[desttrack]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return 0;	//Invalid parameters

	stp = sp->pro_guitar_track[sp->track[sourcetrack]->tracknum];
	dtp = sp->pro_guitar_track[sp->track[desttrack]->tracknum];
	if(dtp->handpositions)
	{	//If there are already fret hand positions in the destination track
		eof_clear_input();
		if(alert(NULL, "Warning:  Existing fret hand positions in this track will be lost.  Continue?", NULL, "&Yes", "&No", 'y', 'n') != 1)
		{	//If the user does not opt to continue
			return 0;
		}
	}

	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	dtp->handpositions = stp->handpositions;
	memcpy(dtp->handposition, stp->handposition, sizeof(stp->handposition));

	return 1;	//Return completion
}

int eof_menu_track_clone_track_1(void)
{
	return eof_menu_track_clone_track_number(eof_song, 1, eof_selected_track);
}

int eof_menu_track_clone_track_2(void)
{
	return eof_menu_track_clone_track_number(eof_song, 2, eof_selected_track);
}

int eof_menu_track_clone_track_3(void)
{
	return eof_menu_track_clone_track_number(eof_song, 3, eof_selected_track);
}

int eof_menu_track_clone_track_4(void)
{
	return eof_menu_track_clone_track_number(eof_song, 4, eof_selected_track);
}

int eof_menu_track_clone_track_5(void)
{
	return eof_menu_track_clone_track_number(eof_song, 5, eof_selected_track);
}

int eof_menu_track_clone_track_6(void)
{
	return 1;	//This function is not valid for the vocal track
}

int eof_menu_track_clone_track_7(void)
{
	return eof_menu_track_clone_track_number(eof_song, 7, eof_selected_track);
}

int eof_menu_track_clone_track_8(void)
{
	return eof_menu_track_clone_track_number(eof_song, 8, eof_selected_track);
}

int eof_menu_track_clone_track_9(void)
{
	return eof_menu_track_clone_track_number(eof_song, 9, eof_selected_track);
}

int eof_menu_track_clone_track_10(void)
{
	return eof_menu_track_clone_track_number(eof_song, 10, eof_selected_track);
}

int eof_menu_track_clone_track_11(void)
{
	return eof_menu_track_clone_track_number(eof_song, 11, eof_selected_track);
}

int eof_menu_track_clone_track_12(void)
{
	return eof_menu_track_clone_track_number(eof_song, 12, eof_selected_track);
}

int eof_menu_track_clone_track_13(void)
{
	return eof_menu_track_clone_track_number(eof_song, 13, eof_selected_track);
}

int eof_menu_track_clone_track_14(void)
{
	return eof_menu_track_clone_track_number(eof_song, 14, eof_selected_track);
}

int eof_menu_track_clone_track_number(EOF_SONG *sp, unsigned long sourcetrack, unsigned long desttrack)
{
	unsigned long stracknum, dtracknum, noteset, notesetcount = 1, ctr, tracknameflag;
	EOF_TRACK_ENTRY *parent;
	EOF_PRO_GUITAR_NOTE **setptr;
	int populated = 1;	//Set to nonzero if the destination track is found to contain no notes
	char s_restore_tech_view = 0, d_restore_tech_view = 0;	//Store the original tech view states of the source/destination tracks if applicable

	if(!sp || (sourcetrack >= sp->tracks) || (desttrack >= sp->tracks) || (sourcetrack == desttrack))
		return 0;	//Invalid parameters
	if(sp->track[sourcetrack]->track_format != sp->track[desttrack]->track_format)
		return 0;	//Invalid parameters

	if((sourcetrack == EOF_TRACK_DANCE) || (desttrack == EOF_TRACK_DANCE))
	{
		allegro_message("Cannot clone between dance and non dance tracks");
		return 0;	//Invalid parameters
	}

	//Warn if the destination track is populated
	eof_clear_input();
	if(!eof_get_track_size_all(sp, desttrack))
	{	//If the destination track isn't populated in either note set (either normal or tech notes)
		populated = 0;
	}
	if(populated && alert(NULL, "This operation will replace this track's contents.", "Continue?", "&Yes", "&No", 'y', 'n') != 1)
	{	//If user does not opt to replace the track
		return 1;	//Cancel
	}

	//Erase the destination track
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	eof_erase_track(sp, desttrack, 1);

	//Clone the source track
	sp->track[desttrack]->difficulty = sp->track[sourcetrack]->difficulty;
	sp->track[desttrack]->numdiffs = sp->track[sourcetrack]->numdiffs;
	tracknameflag = sp->track[desttrack]->flags & EOF_TRACK_FLAG_ALT_NAME;	//Remember the destination track's alternate name flag status
	sp->track[desttrack]->flags = (sp->track[sourcetrack]->flags & ~EOF_TRACK_FLAG_ALT_NAME);	//Copy the flags, with the exception of the source track's alternate name flag
	sp->track[desttrack]->flags |= tracknameflag;	//Restore the destination track's original alternate name flag value
	stracknum = sp->track[sourcetrack]->tracknum;
	dtracknum = sp->track[desttrack]->tracknum;
	switch(sp->track[sourcetrack]->track_format)
	{
		case EOF_LEGACY_TRACK_FORMAT:
			parent = sp->legacy_track[dtracknum]->parent;	//Back up this pointer
			memcpy(sp->legacy_track[dtracknum], sp->legacy_track[stracknum], sizeof(EOF_LEGACY_TRACK));
			sp->legacy_track[dtracknum]->parent = parent;	//Restore this pointer
			sp->legacy_track[dtracknum]->notes = 0;			//Clear the note count
			if(desttrack == EOF_TRACK_KEYS)
			{	//The keys track must be forced to remain at 5 lanes
				sp->legacy_track[dtracknum]->numlanes = 5;
			}
		break;

		case EOF_PRO_GUITAR_TRACK_FORMAT:
			parent = sp->pro_guitar_track[dtracknum]->parent;	//Back up these pointers
			setptr = sp->pro_guitar_track[dtracknum]->note;
			s_restore_tech_view = eof_menu_track_get_tech_view_state(sp, sourcetrack);	//Track which note set each track was in
			d_restore_tech_view = eof_menu_track_get_tech_view_state(sp, desttrack);
			memcpy(sp->pro_guitar_track[dtracknum], sp->pro_guitar_track[stracknum], sizeof(EOF_PRO_GUITAR_TRACK));
			sp->pro_guitar_track[dtracknum]->parent = parent;	//Restore these pointers
			sp->pro_guitar_track[dtracknum]->note = setptr;
			sp->pro_guitar_track[dtracknum]->notes = sp->pro_guitar_track[dtracknum]->pgnotes = sp->pro_guitar_track[dtracknum]->technotes = 0;	//Clear the note counts
			notesetcount = 2;	//Pro guitar tracks have the normal notes AND tech notes that need to be cloned
		break;

		default:
		return 1;	//Unexpected track format
	}
	for(noteset = 0; noteset < notesetcount; noteset++)
	{	//For each note set in the source track
		eof_menu_track_set_tech_view_state(sp, sourcetrack, noteset);
		eof_menu_track_set_tech_view_state(sp, desttrack, noteset);	//Activate the appropriate note set
		for(ctr = 0; ctr < eof_get_track_size(sp, sourcetrack); ctr++)
		{	//For each note in the source track
			unsigned long pos;
			long length;
			unsigned type;

			//Clone the note into the destination track
			pos = eof_get_note_pos(sp, sourcetrack, ctr);
			length = eof_get_note_length(sp, sourcetrack, ctr);
			type = eof_get_note_type(sp, sourcetrack, ctr);
			(void) eof_copy_note_simple(sp, sourcetrack, ctr, desttrack, pos, length, type);
		}
	}
	eof_menu_track_set_tech_view_state(sp, sourcetrack, s_restore_tech_view);	//Re-enable tech view if applicable
	eof_menu_track_set_tech_view_state(sp, desttrack, d_restore_tech_view);
	eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current

	//Clone the source track's events
	for(ctr = 0; ctr < sp->text_events; ctr++)
	{	//For each event in the project
		if(sp->text_event[ctr]->track == sourcetrack)
		{	//If the event is specific to the source track, copy it to the destination track
			(void) eof_song_add_text_event(sp, sp->text_event[ctr]->pos, sp->text_event[ctr]->text, desttrack, sp->text_event[ctr]->flags, 0);
		}
	}

	eof_sort_events(sp);
	eof_scale_fretboard(0);	//Recalculate the 2D screen positioning based on the current track
	eof_set_3D_lane_positions(eof_selected_track);	//Update xchart[] to reflect a different number of lanes being represented in the 3D preview window
	eof_set_color_set();
	(void) eof_detect_difficulties(sp, eof_selected_track);
	eof_fixup_notes(sp);		//Run fixup logic (ie. to apply crazy status to notes cloned into the keys track)

	return 1;
}

int eof_menu_thin_notes_track_1(void)
{
	return eof_thin_notes_to_match_target_difficulty(eof_song, 1, eof_selected_track, 2, eof_note_type);
}

int eof_menu_thin_notes_track_2(void)
{
	return eof_thin_notes_to_match_target_difficulty(eof_song, 2, eof_selected_track, 2, eof_note_type);
}

int eof_menu_thin_notes_track_3(void)
{
	return eof_thin_notes_to_match_target_difficulty(eof_song, 3, eof_selected_track, 2, eof_note_type);
}

int eof_menu_thin_notes_track_4(void)
{
	return eof_thin_notes_to_match_target_difficulty(eof_song, 4, eof_selected_track, 2, eof_note_type);
}

int eof_menu_thin_notes_track_5(void)
{
	return eof_thin_notes_to_match_target_difficulty(eof_song, 5, eof_selected_track, 2, eof_note_type);
}

int eof_menu_thin_notes_track_6(void)
{
	return eof_thin_notes_to_match_target_difficulty(eof_song, 6, eof_selected_track, 2, eof_note_type);
}

int eof_menu_thin_notes_track_7(void)
{
	return eof_thin_notes_to_match_target_difficulty(eof_song, 7, eof_selected_track, 2, eof_note_type);
}

int eof_menu_thin_notes_track_8(void)
{
	return eof_thin_notes_to_match_target_difficulty(eof_song, 8, eof_selected_track, 2, eof_note_type);
}

int eof_menu_thin_notes_track_9(void)
{
	return eof_thin_notes_to_match_target_difficulty(eof_song, 9, eof_selected_track, 2, eof_note_type);
}

int eof_menu_thin_notes_track_10(void)
{
	return eof_thin_notes_to_match_target_difficulty(eof_song, 10, eof_selected_track, 2, eof_note_type);
}

int eof_menu_thin_notes_track_11(void)
{
	return eof_thin_notes_to_match_target_difficulty(eof_song, 11, eof_selected_track, 2, eof_note_type);
}

int eof_menu_thin_notes_track_12(void)
{
	return eof_thin_notes_to_match_target_difficulty(eof_song, 12, eof_selected_track, 2, eof_note_type);
}

int eof_menu_thin_notes_track_13(void)
{
	return eof_thin_notes_to_match_target_difficulty(eof_song, 13, eof_selected_track, 2, eof_note_type);
}

int eof_menu_thin_notes_track_14(void)
{
	return eof_thin_notes_to_match_target_difficulty(eof_song, 14, eof_selected_track, 2, eof_note_type);
}

int eof_track_fret_hand_positions_generate_all(void)
{
	unsigned long tracknum, ctr;
	EOF_PRO_GUITAR_TRACK *tp;
	char restore_tech_view = 0;		//If tech view is in effect, it is temporarily disabled until after the populated difficulties are determined

	if(!eof_song || (eof_selected_track >= eof_song->tracks) || (eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return 1;	//Error

	eof_fret_hand_position_list_dialog_undo_made = 0;	//Reset this condition

	//Remove any existing fret hand positions defined for this track
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	tp = eof_song->pro_guitar_track[tracknum];
	if(tp->handpositions)
	{	//If the active track has at least one fret hand position already
		eof_clear_input();
		if(alert(NULL, "Existing fret hand positions for the active track will be removed.", "Continue?", "&Yes", "&No", 'y', 'n') != 1)
		{	//If the user does not opt to remove the existing hand positions
			return 1;
		}
		if(!eof_fret_hand_position_list_dialog_undo_made)
		{
			eof_fret_hand_position_list_dialog_undo_made = 1;
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		}
		tp->handpositions = 0;
	}

	restore_tech_view = eof_menu_track_get_tech_view_state(eof_song, eof_selected_track);
	eof_menu_track_set_tech_view_state(eof_song, eof_selected_track, 0);	//Disable tech view for the active pro guitar track if applicable
	(void) eof_detect_difficulties(eof_song, eof_selected_track);	//Update eof_track_diff_populated_status[] to reflect all populated difficulties for the active track
	for(ctr = 0; ctr < 256; ctr++)
	{	//For each of the 256 possible difficulties
		if(eof_track_diff_populated_status[ctr])
		{	//If this difficulty is populated
			if(!eof_fret_hand_position_list_dialog_undo_made)
			{
				eof_fret_hand_position_list_dialog_undo_made = 1;
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			}
			eof_generate_efficient_hand_positions(eof_song, eof_selected_track, ctr, 1, 0);	//Generate fret hand positions for it
		}
	}
	eof_menu_track_set_tech_view_state(eof_song, eof_selected_track, restore_tech_view);	//Re-enable tech view for the second piano roll's track if applicable

	return 1;
}

int eof_menu_track_toggle_tech_view(void)
{
	EOF_PRO_GUITAR_TRACK *tp;

	if(!eof_song_loaded)
		return 1;	//Return error

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run unless a pro guitar track is active

	tp = eof_song->pro_guitar_track[eof_song->track[eof_selected_track]->tracknum];
	if(tp->note == tp->technote)
	{	//If tech view is already in effect for the active track
		eof_log("Disabling tech view.", 2);
		eof_menu_pro_guitar_track_disable_tech_view(tp);
	}
	else
	{	//Otherwise put the tech note array into effect
		eof_log("Enabling tech view.", 2);
		eof_menu_pro_guitar_track_enable_tech_view(tp);
	}
	(void) eof_menu_edit_deselect_all();	//Clear the note selection
	(void) eof_detect_difficulties(eof_song, eof_selected_track);	//Re-count the number of notes in the currently active array
	eof_fix_window_title();
	return 1;
}

char eof_menu_pro_guitar_track_get_tech_view_state(EOF_PRO_GUITAR_TRACK *tp)
{
	if(!tp)
		return 0;	//Invalid parameter

	if(tp->note == tp->technote)
	{	//If tech view is in effect for the specified track
		return 1;
	}

	return 0;
}

void eof_menu_pro_guitar_track_disable_tech_view(EOF_PRO_GUITAR_TRACK *tp)
{
	if(!tp)
		return;	//Invalid parameter

	if(tp->note == tp->technote)
	{	//If tech view is in effect for the specified track
		tp->technotes = tp->notes;	//Ensure that the size of the tech note array is backed up
		tp->note = tp->pgnote;		//Put the regular pro guitar note array into effect
		tp->notes = tp->pgnotes;
	}
}

void eof_menu_pro_guitar_track_enable_tech_view(EOF_PRO_GUITAR_TRACK *tp)
{
	if(!tp)
		return;	//Invalid parameter

	if(tp->note != tp->technote)
	{	//If tech view is not in effect for the specified track
		tp->pgnotes = tp->notes;	//Ensure that the size of the regular note array is backed up
		tp->note = tp->technote;	//Put the tech note array into effect
		tp->notes = tp->technotes;
	}
}

void eof_menu_pro_guitar_track_set_tech_view_state(EOF_PRO_GUITAR_TRACK *tp, char state)
{
	if(!tp)
		return;	//Invalid parameter

	if(state)
	{	//If the calling function specified to enable tech view
		eof_menu_pro_guitar_track_enable_tech_view(tp);
	}
	else
	{	//The calling function specified to disable tech view
		eof_menu_pro_guitar_track_disable_tech_view(tp);
	}
}

char eof_menu_track_get_tech_view_state(EOF_SONG *sp, unsigned long track)
{
	EOF_PRO_GUITAR_TRACK *tp;

	if(!sp || !track || (track >= sp->tracks) || (sp->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return 0;	//Invalid parameters

	tp = sp->pro_guitar_track[sp->track[track]->tracknum];
	if(tp->note == tp->technote)
	{	//If tech view is in effect for the specified track
		return 1;
	}

	return 0;	//Return tech view not in effect
}

void eof_menu_track_set_tech_view_state(EOF_SONG *sp, unsigned long track, char state)
{
	EOF_PRO_GUITAR_TRACK *tp;

	if(!sp || !track || (track >= sp->tracks) || (sp->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return;	//Invalid parameters

	tp = sp->pro_guitar_track[sp->track[track]->tracknum];
	if(state)
	{	//If the calling function specified to enable tech view
		eof_menu_pro_guitar_track_enable_tech_view(tp);
	}
	else
	{	//The calling function specified to disable tech view
		eof_menu_pro_guitar_track_disable_tech_view(tp);
	}
}

void eof_menu_track_toggle_tech_view_state(EOF_SONG *sp, unsigned long track)
{
	EOF_PRO_GUITAR_TRACK *tp;

	if(!sp || !track || (track >= sp->tracks) || (sp->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return;	//Invalid parameters

	tp = sp->pro_guitar_track[sp->track[track]->tracknum];
	if(tp->note == tp->technote)
	{	//If tech view is already in effect for the active track
		eof_menu_pro_guitar_track_disable_tech_view(tp);
	}
	else
	{	//Otherwise put the tech note array into effect
		eof_menu_pro_guitar_track_enable_tech_view(tp);
	}
}

void eof_menu_pro_guitar_track_update_note_counter(EOF_PRO_GUITAR_TRACK *tp)
{
	if(!tp)
		return;	//Invalid parameter

	if(tp->note == tp->technote)
	{	//If tech view is in effect for the specified track
		tp->technotes = tp->notes;	//Update the tech note counter
	}
	else
	{
		tp->pgnotes = tp->notes;	//Otherwise update the normal note counter
	}
}

DIALOG eof_menu_track_repair_grid_snap_dialog[] =
{
	/* (proc)                (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                          (dp2) (dp3) */
	{ eof_window_proc,    0,   0,   325, 148, 0,   0,   0,    0,      0,   0,   "Resnap notes to closest of any grid snap within", NULL, NULL },
	{ d_agup_text_proc,      12,  40,  60,  12,  0,   0,   0,    0,      0,   0,   "This # of ms:",NULL, NULL },
	{ eof_verified_edit_proc,12,  56,  90,  20,  0,   0,   0,    0,      7,   0,   eof_etext2, "0123456789",  NULL },
	{ d_agup_text_proc,      12,  82,  60,  12,  0,   0,   0,    0,      0,   0,   eof_etext   ,NULL, NULL },
	{ d_agup_button_proc,    12,  108, 84,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",       NULL, NULL },
	{ d_agup_button_proc,    110, 108, 78,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",   NULL, NULL },
	{ NULL,                  0,   0,   0,   0,   0,   0,   0,    0,      0,   0,   NULL,       NULL, NULL }
};

int eof_menu_track_repair_grid_snap(void)
{
	unsigned long ctr, closestpos = 0, count = 0, tncount = 0;
	unsigned long oldnotecount, newnotecount;
	long threshold = 0;
	char undo_made = 0;
	unsigned long offset;

	//Prompt user for a threshold distance
	eof_etext2[0] = '\0';	//Empty the dialog's input string
	eof_color_dialog(eof_menu_track_repair_grid_snap_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_menu_track_repair_grid_snap_dialog);

	//Find how far out of grid snap the track's notes are
	(void) eof_menu_edit_deselect_all();		//Clear the selection variables if necessary
	eof_selection.track = eof_selected_track;
	for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
	{	//For each note in the active track
		unsigned long notepos;

		notepos = eof_get_note_pos(eof_song, eof_selected_track, ctr);

		if(!eof_is_any_beat_interval_position(notepos, NULL, NULL, NULL, &closestpos, eof_prefer_midi_friendly_grid_snapping))
		{	//If this note is not beat interval snapped
			if(closestpos != ULONG_MAX)
			{	//If the nearest beat interval position was determined
				if(closestpos < notepos)
				{	//If the closest beat interval is earlier than the note
					offset = notepos - closestpos;
				}
				else
				{	//The closest beat interval is later than the note
					offset = closestpos - notepos;
				}

				if(offset > threshold)
				{	//If this is the most out of grid-snap note found so far
					threshold = offset;
				}
			}
		}
	}

	//Prompt the user
	(void) snprintf(eof_etext, sizeof(eof_etext) - 1, "This track's notes are <= %ldms out of snap", threshold);
	if(eof_popup_dialog(eof_menu_track_repair_grid_snap_dialog, 2) != 4)
		return 1;	//If the user did not click OK, return immediately
	if(eof_etext2[0] == '\0')
		return 1;	//If the user did not enter a threshold, return immediately
	threshold = atol(eof_etext2);
	if(threshold <= 0)
		return 1;	//If the specified value is not valid, return immediately

	oldnotecount = eof_get_track_size_all(eof_song, eof_selected_track);	//Store the number of notes (and tech notes if applicable) for later comparison

	eof_auto_adjust_sections(eof_song, eof_selected_track, 0, 0, 1, &undo_made);			//Move sections to nearest grid snap of ANY grid size
	(void) eof_auto_adjust_tech_notes(eof_song, eof_selected_track, 0, 0, 1, &undo_made);	//Move tech notes to nearest grid snap of ANY grid size

	//Process notes
	for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
	{	//For each note in the active track
		unsigned long notepos;

		notepos = eof_get_note_pos(eof_song, eof_selected_track, ctr);

		if(!eof_is_any_beat_interval_position(notepos, NULL, NULL, NULL, &closestpos, eof_prefer_midi_friendly_grid_snapping))
		{	//If this note is not beat interval snapped
			if(closestpos != ULONG_MAX)
			{	//If the nearest grid snap position was determined
				char direction;

				if(closestpos < notepos)
				{	//If the closest grid snap is earlier than the note
					direction = -1;
					offset = notepos - closestpos;
				}
				else
				{	//The closest grid snap is later than the note
					direction = 1;
					offset = closestpos - notepos;
				}

				if(offset <= threshold)
				{	//If the note is within the specified threshold distance from the closest grid snap
					if(!undo_made)
					{	//If an undo state hasn't been made yet
						eof_prepare_undo(EOF_UNDO_TYPE_NONE);
						undo_made = 1;
					}

					eof_selection.multi[ctr] = 1;		//Update the selection array to indicate this note is selected, for use with the tech note auto adjust logic
					tncount += eof_auto_adjust_tech_notes(eof_song, eof_selected_track, offset, direction, 0, &undo_made);	//Move this note's tech notes accordingly by the same number of ms as the normal note
					eof_set_note_pos(eof_song, eof_selected_track, ctr, closestpos);
					eof_selection.multi[ctr] = 0;		//Deselect this note
					count++;
				}
			}
		}
	}

	if(undo_made)
	{	//If any notes were moved
		char *plural = "s";
		char *singular = "";
		char *verbplural = "were";
		char *verbsingular = "was";
		char *countplurality = plural;
		char *tncountplurality = plural;
		char *verbplurality = verbplural;

		if(count == 1)
		{
			countplurality = singular;
			verbplurality = verbsingular;
		}
		if(tncount == 1)
			tncountplurality = singular;

		eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Update highlighting
		newnotecount = eof_get_track_size_all(eof_song, eof_selected_track);	//Read the number of note (and tech notes if applicable)
		if(oldnotecount > newnotecount)
		{	//If one or more notes were lost (ie. merged) since the changes
			allegro_message("Warning:  %lu notes were lost (ie. merged with other notes) during the repair.  Please undo if this is unwanted.", oldnotecount - newnotecount);
		}
		if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If a pro guitar track is active
			if(eof_technote_auto_adjust)
			{	//If the "Auto-adjust tech notes" preference is enabled
				allegro_message("%lu note%s and %lu tech note%s were moved.", count, countplurality, tncount, tncountplurality);
			}
			else
			{
				allegro_message("%lu note%s %s moved.  Auto-adjustment of tech notes (in File>Preferences>Preferences) is not currently enabled.", count, countplurality, verbplurality);
			}
		}
		else
		{
			allegro_message("%lu note%s %s moved.", count, countplurality, verbplurality);
		}
	}

	return 1;
}

int eof_menu_track_clone_track_to_clipboard(void)
{
	unsigned notesetcount = 1;
	char restore_tech_view = 0, noteset;
	char content_found = 0;	//Set to nonzero as long as at least one note/section was written to file
	unsigned long ctr, ctr2, sectiontype, first_pos = 0, tracknum;
	PACKFILE *fp;
	char clipboard_path[50];
	unsigned long notes = 0, technotes = 0, sections = 0, events = 0;

	eof_log("eof_menu_track_clone_track_to_clipboard() entered", 1);

	if(!eof_song)
		return 1;	//No chart open

	if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
	{
		notesetcount = 2;
		restore_tech_view = eof_menu_track_get_tech_view_state(eof_song, eof_selected_track);
	}

	//Create clipboard file
	if(eof_validate_temp_folder())
	{	//Ensure the correct working directory and presence of the temporary folder
		eof_log("\tCould not validate working directory and temp folder", 1);
		return 2;	//Temp folder error
	}
	(void) snprintf(clipboard_path, sizeof(clipboard_path) - 1, "%seof.clone.clipboard", eof_temp_path_s);
	fp = pack_fopen(clipboard_path, "w");
	if(!fp)
	{
		allegro_message("Clipboard error!");
		return 3;	//Clipboard write error
	}
	eof_log("\tCloning to clipboard", 1);

	//Write various track details
	(void) pack_putc(eof_selected_track, fp);	//Write the source track number
	(void) eof_save_song_string_pf(eof_song->track[eof_selected_track]->altname, fp);	//Write the track's alternate name
	(void) pack_putc(eof_song->track[eof_selected_track]->difficulty, fp);	//Write the difficulty rating
	(void) pack_putc(eof_song->track[eof_selected_track]->numdiffs, fp);	//Write the difficulty count
	(void) pack_iputl(eof_song->track[eof_selected_track]->flags, fp);		//Write the track flags
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
	{	//Write pro guitar track specific data
		EOF_PRO_GUITAR_TRACK *tp = eof_song->pro_guitar_track[tracknum];
		(void) pack_putc(tp->numstrings, fp);			//Write the string count
		(void) pack_putc(tp->numfrets, fp);				//Write the fret count
		(void) pack_putc(tp->arrangement, fp);			//Write the arrangement type
		(void) pack_putc(tp->ignore_tuning, fp);		//Write the ignore tuning boolean setting
		(void) pack_putc(tp->capo, fp);					//Write the capo placement
		for(ctr = 0; ctr < EOF_TUNING_LENGTH; ctr++)
			(void) pack_putc(tp->tuning[ctr], fp);		//Write the tuning of each string
		(void) eof_save_song_string_pf(tp->defaulttone, fp);	//Write the default tone name
	}
	else if(eof_song->track[eof_selected_track]->track_format == EOF_LEGACY_TRACK_FORMAT)
		(void) pack_putc(eof_song->legacy_track[tracknum]->numlanes, fp);	//Write the track lane count

	//Process notes
	for(noteset = 0; noteset < notesetcount; noteset++)
	{	//For each note set in the source track
		eof_menu_track_set_tech_view_state(eof_song, eof_selected_track, noteset);	//Activate the note set being examined
		(void) pack_iputl(eof_get_track_size(eof_song, eof_selected_track), fp);	//Store the number of notes that will be stored to clipboard

		if(eof_get_track_size(eof_song, eof_selected_track))
		{	//If at least one note is being written for this note set
			first_pos = eof_get_note_pos(eof_song, eof_selected_track, 0);
			content_found = 1;

			for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
			{	//For each note in the source track
				eof_write_clipboard_note(fp, eof_song, eof_selected_track, ctr, first_pos);				//Write the note data
				eof_write_clipboard_position_snap_data(fp, eof_get_note_pos(eof_song, eof_selected_track, ctr));	//Write the grid snap position data for this note's position
				if(noteset == 0)
					notes++;		//Track how many normal notes are cloned to the clipboard
				else
					technotes++;	//Track how many tech notes are cloned to the clipboard
			}
		}
	}

	//Process sections
	for(sectiontype = 1; sectiontype <= EOF_NUM_SECTION_TYPES; sectiontype++)
	{	//For each type of section that exists
		unsigned long sectionnum, *sectioncount = NULL;
		EOF_PHRASE_SECTION *phrase = NULL;
		double tfloat;
		int skip = 0;

		switch(sectiontype)
		{
			case EOF_BOOKMARK_SECTION:
			case EOF_FRET_CATALOG_SECTION:
			case EOF_PREVIEW_SECTION:
				skip = 1;	//These section types are not recorded by the track clipboard functions
			break;

			default:
			break;
		}

		if(skip || !eof_lookup_track_section_type(eof_song, eof_selected_track, sectiontype, &sectioncount, &phrase) || !phrase)
		{	//If this section type is to be skipped for the cloning, or if the array of this section type couldn't be found
			(void) pack_iputl(0, fp);	//Write 0 as the number of instances of this section type
		}
		else
		{
			(void) pack_iputl(*sectioncount, fp);	//Write the number of instances of this section type
			if(*sectioncount)
			{
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tProcessing %lu instances of section type %lu", *sectioncount, sectiontype);
				eof_log(eof_log_string, 2);
			}

			for(sectionnum = 0; sectionnum < *sectioncount; sectionnum++)
			{	//For each instance of this type of section in the track
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tCloning section instance %lu", sectionnum + 1);
				eof_log(eof_log_string, 2);

				content_found = 1;
				(void) pack_iputl(eof_get_beat(eof_song, phrase[sectionnum].start_pos), fp);	//Write the beat number in which this section starts
				tfloat = eof_get_porpos(phrase[sectionnum].start_pos);
				(void) pack_fwrite(&tfloat, (long)sizeof(double), fp);				//Write the percentage into the beat at which this section starts

				if(sectiontype == EOF_FRET_HAND_POS_SECTION)
				{	//Fret hand positions store the fret number as the end position
					(void) pack_iputl(phrase[sectionnum].end_pos, fp);
				}
				else if(sectiontype == EOF_RS_TONE_CHANGE)
				{	//Tone changes don't use the end position variable
					(void) pack_iputl(0, fp);	//Write dummy data
				}
				else
				{	//Other sections have an end position variable to adjust
					(void) pack_iputl(eof_get_beat(eof_song, phrase[sectionnum].end_pos), fp);	//Write the beat number in which this section ends
					tfloat = eof_get_porpos(phrase[sectionnum].end_pos);
					(void) pack_fwrite(&tfloat, (long)sizeof(double), fp);				//Write the percentage into the beat at which this section ends
				}

				(void) pack_putc(phrase[sectionnum].difficulty, fp);			//Write section difficulty
				(void) eof_save_song_string_pf(phrase[sectionnum].name, fp);	//Write section name
				(void) pack_putc(phrase[sectionnum].flags, fp);					//Write section flags

				sections++;		//Track how many sections are cloned to the clipboard
			}
		}
	}//For each type of section that exists

	//Process events
	for(ctr = 0; ctr < 2; ctr++)
	{
		if(ctr)
		{	//On second pass
			(void) pack_iputl(events, fp);	//Write the number of events specific to this track
		}
		for(ctr2 = 0; ctr2 < eof_song->text_events; ctr2++)
		{	//For each event in the track
			if(eof_song->text_event[ctr2]->track == eof_selected_track)
			{	//If this text event is specific to this track
				if(!ctr)
				{	//On first pass, count the events
					events++;
				}
				else
				{	//On second pass, write the events to the clipboard
					(void) eof_save_song_string_pf(eof_song->text_event[ctr2]->text, fp);	//Write event text
					(void) pack_iputl(eof_song->text_event[ctr2]->pos, fp);				//Write event's assigned beat number
					(void) pack_putc(eof_song->text_event[ctr2]->flags, fp);				//Write event flags
				}
			}
		}
	}

	//Cleanup
	(void) pack_fclose(fp);
	eof_menu_track_set_tech_view_state(eof_song, eof_selected_track, restore_tech_view);	//Re-enable tech view if applicable

	if(!content_found)
	{
		allegro_message("There was no content found in the track.");
		(void) delete_file(clipboard_path);
	}

	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tClone to clipboard succeeded.  Recorded %lu notes, %lu tech notes, %lu sections and %lu events.", notes, technotes, sections, events);
	eof_log(eof_log_string, 1);
	eof_close_menu = 1;				//Force the main menu to close, as this function had a tendency to get hung in the menu logic when activated by keyboard
	return 0;	//Success
}

int eof_menu_track_clone_track_from_clipboard(void)
{
	PACKFILE *fp;
	char clipboard_path[50];
	char source_vocal = 0, dest_vocal = 0;
	char source_dance = 0, dest_dance = 0;
	unsigned char sourcetrack, difficulty = 0xFF, numdiffs = 5, lanecount = 0, numfrets = 17, arrangement = 0, ignore_tuning = 0, capo = 0;
	unsigned long ctr, flags, notecount, sectiontype, eventcount;
	char tuning[EOF_TUNING_LENGTH] = {0};
	char defaulttone[EOF_SECTION_NAME_LENGTH + 1] = {0};
	EOF_PRO_GUITAR_TRACK *tp = NULL;
	char s_track_format, d_track_format;
	EOF_EXTENDED_NOTE temp_note = {{0}, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.0, 0.0, 0, 0, 0, {0}, {0}, 0, 0, 0, 0, 0, 0};
	unsigned char notesetcount = 1, noteset;
	EOF_PRO_GUITAR_NOTE *np;
	unsigned long beats = 0, notes = 0, technotes = 0, sections = 0, events = 0;
	char altname[EOF_NAME_LENGTH + 1];

	//Beat interval variables used to automatically re-snap auto-adjusted timestamps
	unsigned long intervalbeat = 0, intervalpos = 0;
	unsigned char intervalvalue = 0, intervalnum = 0;

	eof_log("eof_menu_track_clone_track_from_clipboard() entered", 1);

	if(!eof_song)
		return 1;	//No chart open

	//Open and validate clipboard file
	(void) snprintf(clipboard_path, sizeof(clipboard_path) - 1, "%seof.clone.clipboard", eof_temp_path_s);
	fp = pack_fopen(clipboard_path, "r");
	if(!fp)
	{
		allegro_message("Cannot access clipboard.");
		return 2;	//Clipboard read error
	}
	sourcetrack = pack_getc(fp);		//Read the source track of the clipboard data
	if(sourcetrack >= eof_song->tracks)
	{
		(void) pack_fclose(fp);
		return 3;	//Invalid track number
	}
	s_track_format = eof_song->track[sourcetrack]->track_format;
	d_track_format = eof_song->track[eof_selected_track]->track_format;
	if(s_track_format == EOF_VOCAL_TRACK_FORMAT)
		source_vocal = 1;
	if(d_track_format == EOF_VOCAL_TRACK_FORMAT)
		dest_vocal = 1;
	else if(d_track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		tp = eof_song->pro_guitar_track[eof_song->track[eof_selected_track]->tracknum];
	if(source_vocal != dest_vocal)
	{
		allegro_message("Cannot clone between vocal and non vocal tracks");
		(void) pack_fclose(fp);
		return 4;	//Invalid clone operation
	}

	if(sourcetrack == EOF_TRACK_DANCE)
		source_dance = 1;
	if(eof_selected_track == EOF_TRACK_DANCE)
		dest_dance = 1;
	if(source_dance != dest_dance)
	{
		allegro_message("Cannot clone between dance and non dance tracks");
		(void) pack_fclose(fp);
		return 4;	//Invalid clone operation
	}

	eof_log("\tCloning from clipboard", 1);

	//Prompt user
	if(eof_get_track_size_all(eof_song, eof_selected_track) && alert("This track already has notes", "Cloning from the clipboard will overwrite this track's contents", "Continue?", "&Yes", "&No", 'y', 'n') != 1)
	{	//If the active track is already populated (with normal or tech notes) and the user doesn't opt to overwrite it
		(void) pack_fclose(fp);
		return 5;	//User cancellation
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	eof_erase_track(eof_song, eof_selected_track, 1);

	//Read various track details
	(void) eof_load_song_string_pf(altname, fp, sizeof(altname));	//Read the track's alternate name
	for(ctr = 1; ctr < eof_song->tracks; ctr++)
	{	//For each track in the project
		if(ctr == eof_selected_track)
			continue;	//It doesn't matter if the active track already has the name it would inherit from the clipboard

		if(!ustrnicmp(altname, eof_song->track[ctr]->name, sizeof(altname)) || !ustrnicmp(altname, eof_song->track[ctr]->altname, sizeof(altname)))
		{	//If the provided name matches the track's native or display name
			altname[0] = '\0';	//The clipboard track's alternate name is in use, don't apply it to the active track
			break;
		}
	}
	if(altname[0] != '\0')
	{	//If the alternate name was not invalidated, apply it to the active track
		strncpy(eof_song->track[eof_selected_track]->altname, altname, sizeof(eof_song->track[eof_selected_track]->altname) - 1);
	}
	difficulty = pack_getc(fp);
	numdiffs = pack_getc(fp);
	flags = pack_igetl(fp);
	if(altname[0] == '\0')
	{	//If the alternate name on the active track is not being altered, ensure the relevant track flag is cleared
		flags &= ~EOF_TRACK_FLAG_ALT_NAME;
	}
	if(s_track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
	{	//Read pro guitar track specific data
		lanecount = pack_getc(fp);
		numfrets = pack_getc(fp);
		arrangement = pack_getc(fp);
		ignore_tuning = pack_getc(fp);
		capo = pack_getc(fp);
		for(ctr = 0; ctr < EOF_TUNING_LENGTH; ctr++)
			tuning[ctr] = pack_getc(fp);
		(void) eof_load_song_string_pf(defaulttone, fp, sizeof(defaulttone));
		notesetcount = 2;
	}
	else if(s_track_format == EOF_LEGACY_TRACK_FORMAT)
	{
		lanecount = pack_getc(fp);
	}

	//Update various track details
	eof_erase_track(eof_song, eof_selected_track, 1);
	eof_song->track[eof_selected_track]->difficulty = difficulty;
	eof_song->track[eof_selected_track]->numdiffs = numdiffs;
	eof_song->track[eof_selected_track]->flags = flags;
	if(d_track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
	{	//If cloning to a pro guitar track
		tp->numstrings = lanecount;

		if(s_track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//And the source track was a pro guitar track
			tp->numfrets = numfrets;
			tp->arrangement = arrangement;
			tp->ignore_tuning = ignore_tuning;
			tp->capo = capo;
			for(ctr = 0; ctr < EOF_TUNING_LENGTH; ctr++)
				tp->tuning[ctr] = tuning[ctr];
			memcpy(tp->defaulttone, defaulttone, sizeof(defaulttone));
		}
	}
	else if(d_track_format == EOF_LEGACY_TRACK_FORMAT)
	{	//If cloning to a legacy track
		if(eof_selected_track != EOF_TRACK_KEYS)
		{	//Except for when cloning to the keys track, which doesn't support a sixth lane
			eof_song->legacy_track[eof_song->track[eof_selected_track]->tracknum]->numlanes = lanecount;	//Apply the source track's lane count
		}
	}

	//Process notes
	for(noteset = 0; noteset < notesetcount; noteset++)
	{	//For each note set in the clipboard file
		eof_menu_track_set_tech_view_state(eof_song, eof_selected_track, noteset);	//Activate the note set being examined
		notecount = pack_igetl(fp);

		for(ctr = 0; ctr < notecount; ctr++)
		{	//For each note for this note set in the clipboard file
			eof_read_clipboard_note(fp, &temp_note, EOF_NAME_LENGTH + 1);	//Read the note
			eof_read_clipboard_position_beat_interval_data(fp, &intervalbeat, &intervalvalue, &intervalnum);	//Read its beat interval data

			if((noteset > 0) && (d_track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
				continue;	//Don't add tech notes to a legacy track

			//Add enough beats to encompass the note if necessary
			while(eof_song->beats <= temp_note.endbeat + 1)
			{	//Until there are enough beats to accommodate the end position of this note
				if(!eof_song_append_beats(eof_song, 1))
				{	//If there was an error adding a beat
					eof_log("\tError adding beat (notes).  Aborting", 1);
					allegro_message("Error adding beat (notes).  Aborting");
					eof_erase_track(eof_song, eof_selected_track, 1);
					(void) pack_fclose(fp);
					return 6;
				}
				eof_chart_length = eof_song->beat[eof_song->beats - 1]->pos;	//Alter the chart length so that the full transcription will display
				beats++;					//Track how many beats were appended to the project
			}

			//Make adjustments to the note and flag bitmasks if necessary
			if((d_track_format == EOF_LEGACY_TRACK_FORMAT) && (temp_note.legacymask != 0))
			{	//If the cloned note indicated that the legacy bitmask this overrides the original bitmask (cloning a pro guitar into a legacy track)
				temp_note.note = temp_note.legacymask;
			}
			eof_sanitize_note_flags(&temp_note.flags, sourcetrack, eof_selected_track);	//Ensure the note flags are validated for the track being cloned into

			//Update the note position and length
			temp_note.pos = eof_put_porpos(temp_note.beat, temp_note.porpos, 0.0);
			temp_note.length = eof_put_porpos(temp_note.endbeat, temp_note.porendpos, 0.0) - temp_note.pos;
			if(temp_note.length <= 0)
				temp_note.length = 1;
			if(eof_find_beat_interval_position(intervalbeat, intervalvalue, intervalnum, &intervalpos))
			{	//If the destination beat interval position can be calculated
				temp_note.pos = intervalpos;	//Update the adjusted position for the note
			}

			//Create the note
			np = eof_track_add_create_note(eof_song, eof_selected_track, temp_note.note, temp_note.pos, temp_note.length, temp_note.type, temp_note.name);
			if(!np)
			{	//If the note was not created
				eof_log("\tError adding note.  Aborting", 1);
				allegro_message("Error adding note.  Aborting");
				eof_erase_track(eof_song, eof_selected_track, 1);
				(void) pack_fclose(fp);
				return 7;
			}
			eof_set_note_flags(eof_song, eof_selected_track, eof_get_track_size(eof_song, eof_selected_track) - 1, temp_note.flags);
			eof_set_note_eflags(eof_song, eof_selected_track, eof_get_track_size(eof_song, eof_selected_track) - 1, temp_note.eflags);
			eof_set_note_accent(eof_song, eof_selected_track, eof_get_track_size(eof_song, eof_selected_track) - 1, temp_note.accent);
			eof_set_note_ghost(eof_song, eof_selected_track, eof_get_track_size(eof_song, eof_selected_track) - 1, temp_note.ghost);

			if(noteset == 0)
				notes++;		//Track how many normal notes are cloned from the clipboard
			else
				technotes++;	//Track how many tech notes are cloned from the clipboard
			if(d_track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
				continue;	//If the track isn't being cloned into a pro guitar track, skip the logic below

			//Update pro guitar content if applicable
			np->legacymask = temp_note.legacymask;							//Copy the legacy bitmask to the last created pro guitar note
			memcpy(np->frets, temp_note.frets, sizeof(temp_note.frets));	//Copy the fret array to the last created pro guitar note
			memcpy(np->finger, temp_note.finger, sizeof(temp_note.finger));	//Copy the finger array to the last created pro guitar note
			np->bendstrength = temp_note.bendstrength;						//Copy the bend height to the last created pro guitar note
			np->slideend = temp_note.slideend;								//Copy the slide end position to the last created pro guitar note
			np->unpitchend = temp_note.unpitchend;							//Copy the slide end position to the last created pro guitar note
		}//For each note for this note set in the clipboard file
	}//For each note set in the clipboard file
	eof_menu_track_set_tech_view_state(eof_song, eof_selected_track, 0);	//Disable tech view for the active track
	eof_track_fixup_notes(eof_song, eof_selected_track, 0);

	//Process sections
	for(sectiontype = 1; sectiontype <= EOF_NUM_SECTION_TYPES; sectiontype++)
	{	//For each type of section that exists
		unsigned long sectionnum, sectioncount, *junk = NULL, start, end;
		long endbeat;
		EOF_PHRASE_SECTION *phrase = NULL;
		char name[EOF_SECTION_NAME_LENGTH + 1];
		unsigned char sectiondiff, sectionflags;
		double pos = 0.0;
		long beat;

		sectioncount = pack_igetl(fp);	//Read the number of instances of this section type
		if(sectioncount)
		{
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tProcessing %lu instances of section type %lu", sectioncount, sectiontype);
			eof_log(eof_log_string, 2);
		}
		for(sectionnum = 0; sectionnum < sectioncount; sectionnum++)
		{	//For each instance of this type of section in the track
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tCloning section instance %lu", sectionnum + 1);
			eof_log(eof_log_string, 2);

			beat = pack_igetl(fp);	//Read the beat number in which this section starts
			(void) pack_fread(&pos, (long)sizeof(double), fp);	//Read the percentage into the beat at which this section starts
			start = eof_put_porpos(beat, pos, 0.0);

			endbeat = pack_igetl(fp);
			if((beat < 0) || (endbeat < 0))
			{	//If an invalid value was read
				eof_log("\tError determining phrase timing.  Aborting", 1);
				allegro_message("Error determining phrase timing.  Aborting");
				(void) pack_fclose(fp);
				return 8;	//Return error
			}
			if(sectiontype == EOF_FRET_HAND_POS_SECTION)
			{	//Fret hand positions store the fret number as the end position
				end = endbeat;
			}
			else if(sectiontype == EOF_RS_TONE_CHANGE)
			{	//Tone changes don't use the end position variable
				end = 0;
			}
			else
			{	//Other sections have an end position variable to adjust
				beat = endbeat;	//This is he beat number in which this section ends
				(void) pack_fread(&pos, (long)sizeof(double), fp);	//Read the percentage into the beat at which this section ends
				end = eof_put_porpos(beat, pos, 0.0);
			}

			sectiondiff = pack_getc(fp);	//Read section difficulty
			(void) eof_load_song_string_pf(name, fp, sizeof(name));	//Read section name
			sectionflags = pack_getc(fp);	//Read section flags

			(void) eof_lookup_track_section_type(eof_song, eof_selected_track, sectiontype, &junk, &phrase);
			if(!phrase)
			{	//If this section type isn't compatible with the destination track
				continue;	//Don't add it
			}

			//Add enough beats to encompass the section if necessary
			while(eof_song->beats <= beat + 1)
			{	//Until there are enough beats to accommodate the end position of this section
				if(!eof_song_append_beats(eof_song, 1))
				{	//If there was an error adding a beat
					eof_log("\tError adding beat (phrase).  Aborting", 1);
					allegro_message("Error adding beat (phrase).  Aborting");
					eof_erase_track(eof_song, eof_selected_track, 1);
					(void) pack_fclose(fp);
					return 9;
				}
				eof_chart_length = eof_song->beat[eof_song->beats - 1]->pos;	//Alter the chart length so that the full transcription will display
				beats++;					//Track how many beats were appended to the project
			}

			if(!eof_track_add_section(eof_song, eof_selected_track, sectiontype, sectiondiff, start, end, sectionflags, name))
			{	//If the section couldn't be added
				eof_log("\tError adding section.  Aborting", 1);
				allegro_message("Error adding section.  Aborting");
				eof_erase_track(eof_song, eof_selected_track, 1);
				(void) pack_fclose(fp);
				return 10;
			}
			sections++;		//Track how many sections are cloned from the clipboard
		}
	}//For each type of section that exists

	//Process events
	eventcount = pack_igetl(fp);	//Read the number of events
	for(ctr = 0; ctr < eventcount; ctr++)
	{	//For each event in the clipboard file
		char text[EOF_TEXT_EVENT_LENGTH + 1];
		unsigned long beatnum;
		unsigned char eventflags;
		int needbeat = 0;

		(void) eof_load_song_string_pf(text, fp, sizeof(text));	//Read event text
		beatnum = pack_igetl(fp);								//Read event's assigned beat number
		eventflags = pack_getc(fp);								//Read event flags

		//Add enough beats to encompass the event if necessary
		while(1)
		{	//Until there are enough beats to accommodate the beat number this event is assigned to
			needbeat = 0;
			if(eventflags & EOF_EVENT_FLAG_FLOATING_POS)
			{	//If this is a floating text event
				if((eof_song->beats < 2) || (eof_song->beat[eof_song->beats - 1]->pos <= beatnum))
				{	//If the last beat doesn't exceed the millisecond position needed for the event
					needbeat = 1;
				}
			}
			else
			{	//This text event is assigned to a beat marker
				if(eof_song->beats <= beatnum + 1)
				{	//If the number of beats in the project doesn't exceed the beat number needed for the event
					needbeat = 1;	//Add another
				}
			}
			if(!needbeat)	//If the current number of beats is sufficient
				break;		//Exit loop

			if(!eof_song_append_beats(eof_song, 1))
			{	//If there was an error adding a beat
				eof_log("\tError adding beat (event).  Aborting", 1);
				allegro_message("Error adding beat (event).  Aborting");
				eof_erase_track(eof_song, eof_selected_track, 1);
				(void) pack_fclose(fp);
				return 11;
			}
			eof_chart_length = eof_song->beat[eof_song->beats - 1]->pos;	//Alter the chart length so that the full transcription will display
			beats++;					//Track how many beats were appended to the project
		}

		if(!eof_song_add_text_event(eof_song, beatnum, text, eof_selected_track, eventflags, 0))
		{	//If the event couldn't be added
			eof_log("\tError adding event.  Aborting", 1);
			allegro_message("Error adding event.  Aborting");
			eof_erase_track(eof_song, eof_selected_track, 1);
			(void) pack_fclose(fp);
			return 12;
		}
		events++;
	}

	//Cleanup
	eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current
	(void) eof_detect_difficulties(eof_song, eof_selected_track);	//Update note/technote populated identifiers
	eof_scale_fretboard(0);	//Recalculate the 2D screen positioning based on the current track
	eof_set_2D_lane_positions(0);					//Update ychart[] to reflect a different number of lanes being represented in the editor window
	eof_set_3D_lane_positions(eof_selected_track);	//Update xchart[] to reflect a different number of lanes being represented in the 3D preview window
	eof_set_color_set();
	eof_sort_events(eof_song);
	(void) pack_fclose(fp);

	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tClone from clipboard succeeded.  Added %lu beats.  Cloned %lu notes, %lu tech notes, %lu sections and %lu events.", beats, notes, technotes, sections, events);
	eof_log(eof_log_string, 1);
	eof_render();
	eof_close_menu = 1;				//Force the main menu to close, as this function had a tendency to get hung in the menu logic when activated by keyboard
	return 0;	//Success
}

int eof_track_menu_enable_ghl_mode(void)
{
	EOF_TRACK_ENTRY *ep;
	unsigned long ctr;
	int warning = 0, lane_6_gem = 0;
	char undo_made = 0;

	if(!eof_song_loaded || !eof_song)
		return 1;	//Do not allow this function to run if a chart is not loaded
	if(!eof_track_is_legacy_guitar(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run when a legacy guitar behavior track is not active

	//Check if any authoring would be lost by disabling GHL mode and converting notes to non GHL
	if(eof_track_is_ghl_mode(eof_song, eof_selected_track))
	{	//If the track is having GHL mode disabled
		for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
		{	//For each note in the active track
			unsigned long note = eof_get_note_note(eof_song, eof_selected_track, ctr);

			if((note != 32) || !(eof_get_note_flags(eof_song, eof_selected_track, ctr) & EOF_GUITAR_NOTE_FLAG_GHL_OPEN))
			{	//If this note is not a GHL open note
				if(note & 32)
				{	//If this note contains a gem on lane 6
					if(note != 32)
					{	//If this contains a gem on another lane as well
						if(!undo_made)
						{	//If an undo state hasn't been made yet
							eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//The conversion of this note below will be lossy, make an undo state
							undo_made = 1;
						}
					}
					else
					{	//This is a W3 single note
						lane_6_gem = 1;
					}
					if(lane_6_gem && undo_made)
					{	//If both conditions (W3 single note and chord with W3) were found
						break;
					}
				}
			}
		}
	}

	if(lane_6_gem)
	{
		allegro_message("Warning:  Lane 3 white GHL gems will be converted to 5 note chords.");
	}
	ep = eof_song->track[eof_selected_track];		//Simplify
	ep->flags ^= EOF_TRACK_FLAG_GHL_MODE;			//Toggle this flag
	if(ep->flags & EOF_TRACK_FLAG_GHL_MODE)
	{	//If the GHL mode flag is now enabled
		eof_song->legacy_track[ep->tracknum]->numlanes = 6;
		ep->flags |= EOF_TRACK_FLAG_SIX_LANES;		//Set the open strum flag
		ep->flags |= EOF_TRACK_FLAG_GHL_MODE_MS;	//Denote that the new GHL lane ordering is in effect
	}
	else
	{	//GHL mode was just disabled
		ep->flags &= ~EOF_TRACK_FLAG_GHL_MODE_MS;	//Clear this flag since it's only applicable for GHL tracks
	}
	for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
	{	//For each note in the active track
		if(eof_note_convert_ghl_authoring(eof_song, eof_selected_track, ctr))	//Convert lane 3 white GHL gem and open string notation accordingly
		{	//If the conversion of a chord containing a lane 3 white gem caused a loss of original authoring
			if(!warning)
			{	//If the user wasn't already warned about this
				allegro_message("Warning:  Chords containing lane 3 white GHL gems can't be authored in a non GHL track.  These were converted to 5 note chords and highlighted.");
				warning = 1;
			}
		}
	}

	//Cleanup
	eof_scale_fretboard(0);	//Recalculate the 2D screen positioning based on the current track
	eof_set_3D_lane_positions(eof_selected_track);	//Update xchart[] to reflect a different number of lanes being represented in the 3D preview window
	eof_set_color_set();
	eof_fix_window_title();
	eof_prepare_track_menu();	///For some reason, it seems activating this menu function by keyboard and then immediately opening the track menu again by keyboard doesn't reflect the correct
								///GHL status because the menu prepare logic doesn't get re-run before the menu launches
	eof_render();
	return 0;	//Success
}

int eof_menu_track_rs_bonus_arrangement(void)
{
	if(!eof_song)
		return 1;
	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Only allow this function to run for pro guitar/bass tracks

	if(!(eof_song->track[eof_selected_track]->flags & EOF_TRACK_FLAG_RS_BONUS_ARR))
	{	//If this track doesn't already have the RS bonus arrangement flag
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		eof_song->track[eof_selected_track]->flags |= EOF_TRACK_FLAG_RS_BONUS_ARR;	//Set the bonus flag
		eof_song->track[eof_selected_track]->flags &= ~EOF_TRACK_FLAG_RS_ALT_ARR;	//And clear the alternate flag
	}

	return 1;
}

int eof_menu_track_rs_alternate_arrangement(void)
{
	if(!eof_song)
		return 1;
	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Only allow this function to run for pro guitar/bass tracks

	if(!(eof_song->track[eof_selected_track]->flags & EOF_TRACK_FLAG_RS_ALT_ARR))
	{	//If this track doesn't already have the RS alternate arrangement flag
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		eof_song->track[eof_selected_track]->flags |= EOF_TRACK_FLAG_RS_ALT_ARR;	//Set the alternate flag
		eof_song->track[eof_selected_track]->flags &= ~EOF_TRACK_FLAG_RS_BONUS_ARR;	//And clear the bonus flag
	}

	return 1;
}

int eof_menu_track_rs_normal_arrangement(void)
{
	if(!eof_song)
		return 1;
	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Only allow this function to run for pro guitar/bass tracks

	if((eof_song->track[eof_selected_track]->flags & EOF_TRACK_FLAG_RS_BONUS_ARR) || (eof_song->track[eof_selected_track]->flags & EOF_TRACK_FLAG_RS_ALT_ARR))
	{	//If this track has either the RS bonus or alternate arrangement flag
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		eof_song->track[eof_selected_track]->flags &= ~EOF_TRACK_FLAG_RS_ALT_ARR;	//Clear the alternate flag
		eof_song->track[eof_selected_track]->flags &= ~EOF_TRACK_FLAG_RS_BONUS_ARR;	//And clear the bonus flag
	}

	return 1;
}

int eof_menu_track_rs_picked_bass_arrangement(void)
{
	if(!eof_song)
		return 1;
	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Only allow this function to run for pro guitar/bass tracks

	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	if(eof_song->track[eof_selected_track]->flags & EOF_TRACK_FLAG_RS_PICKED_BASS)
	{	//If this track already has the RS picked bass arrangement flag
		eof_song->track[eof_selected_track]->flags &= ~EOF_TRACK_FLAG_RS_PICKED_BASS;	//Clear the flag
	}
	else
	{
		eof_song->track[eof_selected_track]->flags |= EOF_TRACK_FLAG_RS_PICKED_BASS;	//Otherwise set the flag
	}

	return 1;
}

DIALOG eof_menu_track_offset_dialog[] =
{
	/* (proc)                (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                      (dp2) (dp3) */
	{ eof_window_proc,    0,   0,   300, 148, 0,   0,   0,    0,      0,   0,   "Move all track content", NULL, NULL },
	{ d_agup_text_proc,      12,  40,  60,  12,  0,   0,   0,    0,      0,   0,   "This # of ms:",          NULL, NULL },
	{ eof_verified_edit_proc,12,  74,  90,  20,  0,   0,   0,    0,      7,   0,   eof_etext,                "-0123456789",  NULL },
	{ d_agup_button_proc,    12,  108, 84,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",                     NULL, NULL },
	{ d_agup_button_proc,    110, 108, 78,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",                 NULL, NULL },
	{ NULL,                  0,   0,   0,   0,   0,   0,   0,    0,      0,   0,   NULL,                     NULL, NULL }
};

int eof_menu_track_offset(void)
{
	long offset;

	eof_etext[0] = '\0';	//Empty the dialog's input string
	eof_color_dialog(eof_menu_track_offset_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_menu_track_offset_dialog);

	if(eof_popup_dialog(eof_menu_track_offset_dialog, 2) != 3)
		return 1;	//If the user did not click OK, return immediately
	if(eof_etext[0] == '\0')
		return 1;	//If the user did not enter an offset, return immediately

	offset = atol(eof_etext);
	if(!offset)
		return 1;	//If there was an error converting the number of the user entered an offset of 0, don't do anything

	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	if(eof_adjust_notes(eof_selected_track, offset) == 0)
	{	//The operation failed
		allegro_message("Cannot offset content to a position earlier than 0ms.  The operation will be canceled.");
		(void) eof_undo_apply();	//Undo this failed operation
	}

	eof_reset_lyric_preview_lines();
	eof_truncate_chart(eof_song);	//Update number of beats and the chart length as appropriate
	return 1;
}

DIALOG eof_menu_track_check_chord_snapping_dialog[] =
{
	/* (proc)                (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags)  (d1) (d2) (dp)                      (dp2) (dp3) */
	{ eof_window_proc,    0,   0,   170, 135, 2,    23,  0,    0,      0,   0,   "Chord snap gems within", NULL, NULL },
	{ eof_verified_edit_proc,12,  30,  30,  20,  0,    0,   0,    0,      3,   0,   eof_etext2,               "0123456789",  NULL },
	{ d_agup_radio_proc,     12,  55,  86,  15,  2,    23,  0,    0,      0,   0,   "Delta ticks",            NULL, NULL },
	{ d_agup_radio_proc,     12,  75,  96,  15,  2,    23,  0,    D_SELECTED,0,0,   "Milliseconds",           NULL, NULL },
	{ d_agup_button_proc,    12,  100, 68,  28,  2,    23,  '\r', D_EXIT, 0,   0,   "OK",                     NULL, NULL },
	{ d_agup_button_proc,    90,  100, 68,  28,  2,    23,  0,    D_EXIT, 0,   0,   "Cancel",                 NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

int eof_menu_track_check_chord_snapping(void)
{
	long threshold;
	int timing = 0;

	eof_etext2[0] = '\0';	//Empty the dialog's input string
	eof_color_dialog(eof_menu_track_check_chord_snapping_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_menu_track_check_chord_snapping_dialog);

	if(eof_popup_dialog(eof_menu_track_check_chord_snapping_dialog, 1) != 4)
		return 1;	//If the user did not click OK, return immediately
	if(eof_etext2[0] == '\0')
		return 1;	//If the user did not enter a threshold, return immediately

	threshold = atol(eof_etext2);
	if(threshold < 0)
		return 1;	//If the specified value is not valid, return immediately
	if(eof_menu_track_check_chord_snapping_dialog[3].flags == D_SELECTED)
		timing = 1;	//User selected millisecond timing

	if(eof_song_check_unsnapped_chords(eof_song, eof_selected_track, 0, timing, threshold))
	{	//If there are notes that should be snapped into chords
		eof_render();
		eof_clear_input();
		if(alert("At least one note would chord snap with the specified threshold", "These have been highlighted", "Re-align the gems to make them proper chords?", "&Yes", "&No", 'y', 'n') == 1)
		{	//If the user opts to automatically fix the unsnapped chords
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			eof_song_check_unsnapped_chords(eof_song, eof_selected_track, 1, timing, threshold);
		}
	}

	return 1;
}

int eof_menu_track_erase_note_names(void)
{
	unsigned long i;
	char *notename = NULL, undo_made = 0;

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the track
		notename = eof_get_note_name(eof_song, eof_selected_track, i);	//Get the note's name
		if(!notename)
			continue;	//If the note's name couldn't be accessed, skip it
		if(notename[0] == '\0')
			continue;	//If the note has no name, skip it

		if(!undo_made)
		{
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			undo_made = 1;
		}
		notename[0] = '\0';	//Empty the note's name string
	}

	return D_O_K;
}

int eof_track_set_chord_hand_mode_change(void)
{
	return eof_pro_guitar_track_set_hand_mode_change_at_timestamp(eof_music_pos.value - eof_av_delay, 0);
}

int eof_track_set_string_hand_mode_change(void)
{
	return eof_pro_guitar_track_set_hand_mode_change_at_timestamp(eof_music_pos.value - eof_av_delay, 1);
}

int eof_track_delete_effective_hand_mode_change(void)
{
	unsigned long index = ULONG_MAX;
	EOF_PRO_GUITAR_TRACK *tp;

	if(!eof_song_loaded || !eof_song)
		return 1;	//Do not allow this function to run if a chart is not loaded
	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	tp = eof_song->pro_guitar_track[eof_song->track[eof_selected_track]->tracknum];
	(void) eof_pro_guitar_track_find_effective_hand_mode_change(tp, eof_music_pos.value - eof_av_delay, NULL, &index);
	if(index <= tp->handmodechanges)
	{	//If there is a hand mode change in effect at the current seek position in the active track difficulty
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		eof_pro_guitar_track_delete_hand_mode_change(tp, index);	//Delete the hand mode change
	}
	return 1;
}

void eof_track_pro_guitar_sort_hand_mode_changes(EOF_PRO_GUITAR_TRACK* tp)
{
 	eof_log("eof_track_pro_guitar_sort_hand_mode_changes() entered", 1);

	if(tp)
	{
		qsort(tp->handmodechange, (size_t)tp->handmodechanges, sizeof(EOF_PHRASE_SECTION), eof_song_qsort_phrase_sections);
	}
}

char eof_note_name_search_string[256] = {0};
char eof_note_name_search_dialog_title[50] = {0};
DIALOG eof_note_name_search_dialog[] =
{
	/* (proc)             (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)          (dp2) (dp3) */
	{ eof_window_proc, 0,   48,  280, 106, 2,   23,  0,    0,      0,   0,   eof_note_name_search_dialog_title, NULL, NULL },
	{ eof_edit_proc,   12,  80,  254, 20,  2,   23,  0,    0,      255, 0,   eof_note_name_search_string,    NULL, NULL },
	{ d_agup_button_proc, 40,  112, 84,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",         NULL, NULL },
	{ d_agup_button_proc, 155, 112, 78,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",     NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

int eof_note_name_find_next(void)
{
	unsigned long ctr;

	eof_cursor_visible = 0;
	eof_render();
	eof_track_sort_notes(eof_song, eof_selected_track);
	eof_color_dialog(eof_note_name_search_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_note_name_search_dialog);
	if(eof_popup_dialog(eof_note_name_search_dialog, 1) == 2)	//User hit OK on "Find next lyric/note with this text/name" dialog instead of canceling
	{
		for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
		{	//For each note in this track
			if(eof_get_note_pos(eof_song, eof_selected_track, ctr) <= eof_music_pos.value - eof_av_delay)
				continue;	//If this note isn't after the current seek position, skip it

			if(strcasestr_spec(eof_get_note_name(eof_song, eof_selected_track, ctr), eof_note_name_search_string))
			{	//If the specified text is included in the note name (case insensitive)
				eof_set_seek_position(eof_get_note_pos(eof_song, eof_selected_track, ctr) + eof_av_delay);	//Seek to the match position
				break;
			}
		}
	}
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(screen);

	return D_O_K;
}

DIALOG eof_name_search_replace_dialog[] =
{
	/* (proc)             (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)          (dp2) (dp3) */
	{ eof_window_proc, 0,   48,  314, 162, 2,   23,  0,    0,      0,   0,   "Search and replace", NULL, NULL },
	{ d_agup_text_proc,   12,  84,  64,  8,   2,   23,  0,    0,      0,   0,   "Replace:",   NULL, NULL },
	{ eof_edit_proc,   70,  80,  230, 20,  2,   23,  0,    0,      255, 0,   eof_etext,    NULL, NULL },
	{ d_agup_text_proc,   12,  110, 64,  8,   2,   23,  0,    0,      0,   0,   "With:",      NULL, NULL },
	{ eof_edit_proc,   70,  106, 230, 20,  2,   23,  0,    0,      255, 0,   eof_etext2,   NULL, NULL },
	{ d_agup_check_proc,  12,  130, 90,  16,  2,   23,  0,    0,      0,   0,   "Match case", NULL, NULL },
	{ d_agup_check_proc,  12,  146, 220, 16,  2,   23,  0,    D_SELECTED,0,0,   "Retain first letter capitalization", NULL, NULL },
	{ d_agup_button_proc, 67,  168, 84,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",         NULL, NULL },
	{ d_agup_button_proc, 163, 168, 78,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",     NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

int eof_name_search_replace(void)
{
	unsigned long ctr, count = 0;
	int note_selection_updated, focus;
	char undo_made = 0, *ptr;
	int result;

	//Initialize the dialog
	note_selection_updated = eof_update_implied_note_selection();	//If no notes are selected, take start/end selection and Feedback input mode into account
	if(eof_selection.current < eof_get_track_size(eof_song, eof_selected_track))
	{	//If a specific note/lyric is selected (ie. via click)
		strncpy(eof_etext, eof_get_note_name(eof_song, eof_selected_track, eof_selection.current), sizeof(eof_etext) - 1);	//Populate the "Replace" field with the note's/lyric's text
		focus = 4;	//And set initial focus to the "With" field
	}
	else
	{	//Otherwise empty the "Replace" field and set initial focus to it
		eof_etext[0] = '\0';
		focus = 2;
	}
	eof_etext2[0] = '\0';	//Empty the "With" field
	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_lyric_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_name_search_replace_dialog);

	if(eof_popup_dialog(eof_name_search_replace_dialog, focus) == 7)
	{	//If the user clicked OK
		if((eof_etext[0] != '\0') && (eof_etext2[0] != '\0') && strcmp(eof_etext, eof_etext2))
		{	//If the "Replace" and "With" fields are both populated and aren't the same
			for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
			{	//For each note in the active track
				if(eof_name_search_replace_dialog[5].flags == D_SELECTED)
				{	//The user specified a case-sensitive search and replace
					result = strcmp(eof_get_note_name(eof_song, eof_selected_track, ctr), eof_etext);
				}
				else
				{
					result = ustricmp(eof_get_note_name(eof_song, eof_selected_track, ctr), eof_etext);
				}
				if(!result)
				{	//If the lyric matches the specified text (with the specified case sensitivity)
					if(!undo_made)
					{	//If an undo state hasn't been made yet
						eof_prepare_undo(EOF_UNDO_TYPE_NONE);
						undo_made = 1;
					}
					if(eof_name_search_replace_dialog[6].flags == D_SELECTED)
					{	//The user specified to apply the replaced word's first letter capitalization
						if(isalpha(eof_etext[0]) && isalpha(eof_etext2[0]))
						{	//If the first letters of the search and replace terms are both alphabetical
							ptr = eof_get_note_name(eof_song, eof_selected_track, ctr);
							if(isupper(ptr[0]))
							{	//If the first letter of the lyric instance being replaced is upper case
								eof_etext2[0] = toupper(eof_etext2[0]);	//Force the first letter of the replace string to upper case
							}
							else
							{	//Otherwise force the first letter of the replace string to lower case
								eof_etext2[0] = tolower(eof_etext2[0]);
							}
						}
					}
					eof_set_note_name(eof_song, eof_selected_track, ctr, eof_etext2);	//Apply the "With" text to the lyric
					count++;
				}
			}

			allegro_message("%lu %s made.", count, ((count == 1) ? "replacement" : "replacements"));
		}
	}
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(screen);
	if(note_selection_updated)
	{	//If the note selection was originally empty and was dynamically updated
		(void) eof_menu_edit_deselect_all();	//Clear the note selection
	}
	return D_O_K;
}
