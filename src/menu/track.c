#include <allegro.h>
#include "../agup/agup.h"
#include "../dialog/proc.h"
#include "../dialog.h"
#include "./edit.h"
#include "../main.h"
#include "../mix.h"
#include "../player.h"
#include "../rs.h"
#include "../song.h"
#include "../tuning.h"
#include "../undo.h"
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


char eof_menu_thin_diff_menu_text[EOF_TRACKS_MAX][EOF_TRACK_NAME_SIZE] = {{0}};
MENU eof_menu_thin_diff_menu[EOF_TRACKS_MAX] =
{
	{eof_menu_thin_diff_menu_text[0], eof_menu_thin_notes_track_1, NULL, D_SELECTED, NULL},
	{eof_menu_thin_diff_menu_text[1], eof_menu_thin_notes_track_2, NULL, 0, NULL},
	{eof_menu_thin_diff_menu_text[2], eof_menu_thin_notes_track_3, NULL, 0, NULL},
	{eof_menu_thin_diff_menu_text[3], eof_menu_thin_notes_track_4, NULL, 0, NULL},
	{eof_menu_thin_diff_menu_text[4], eof_menu_thin_notes_track_5, NULL, 0, NULL},
	{eof_menu_thin_diff_menu_text[5], eof_menu_thin_notes_track_6, NULL, 0, NULL},
	{eof_menu_thin_diff_menu_text[6], eof_menu_thin_notes_track_7, NULL, 0, NULL},
	{eof_menu_thin_diff_menu_text[7], eof_menu_thin_notes_track_8, NULL, 0, NULL},
	{eof_menu_thin_diff_menu_text[8], eof_menu_thin_notes_track_9, NULL, 0, NULL},
	{eof_menu_thin_diff_menu_text[9], eof_menu_thin_notes_track_10, NULL, 0, NULL},
	{eof_menu_thin_diff_menu_text[10], eof_menu_thin_notes_track_11, NULL, 0, NULL},
	{eof_menu_thin_diff_menu_text[11], eof_menu_thin_notes_track_12, NULL, 0, NULL},
	{eof_menu_thin_diff_menu_text[12], eof_menu_thin_notes_track_13, NULL, 0, NULL},
	{eof_menu_thin_diff_menu_text[13], eof_menu_thin_notes_track_14, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

char eof_menu_track_clone_menu_text[EOF_TRACKS_MAX][EOF_TRACK_NAME_SIZE] = {{0}};
MENU eof_menu_track_clone_menu[EOF_TRACKS_MAX] =
{
	{eof_menu_track_clone_menu_text[0], eof_menu_track_clone_track_1, NULL, D_SELECTED, NULL},
	{eof_menu_track_clone_menu_text[1], eof_menu_track_clone_track_2, NULL, 0, NULL},
	{eof_menu_track_clone_menu_text[2], eof_menu_track_clone_track_3, NULL, 0, NULL},
	{eof_menu_track_clone_menu_text[3], eof_menu_track_clone_track_4, NULL, 0, NULL},
	{eof_menu_track_clone_menu_text[4], eof_menu_track_clone_track_5, NULL, 0, NULL},
	{eof_menu_track_clone_menu_text[5], eof_menu_track_clone_track_6, NULL, 0, NULL},
	{eof_menu_track_clone_menu_text[6], eof_menu_track_clone_track_7, NULL, D_DISABLED, NULL},
	{eof_menu_track_clone_menu_text[7], eof_menu_track_clone_track_8, NULL, 0, NULL},
	{eof_menu_track_clone_menu_text[8], eof_menu_track_clone_track_9, NULL, 0, NULL},
	{eof_menu_track_clone_menu_text[9], eof_menu_track_clone_track_10, NULL, 0, NULL},
	{eof_menu_track_clone_menu_text[10], eof_menu_track_clone_track_11, NULL, 0, NULL},
	{eof_menu_track_clone_menu_text[11], eof_menu_track_clone_track_12, NULL, 0, NULL},
	{eof_menu_track_clone_menu_text[12], eof_menu_track_clone_track_13, NULL, 0, NULL},
	{eof_menu_track_clone_menu_text[13], eof_menu_track_clone_track_14, NULL, 0, NULL},
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

MENU eof_track_menu[] =
{
	{"Pro &Guitar", NULL, eof_track_proguitar_menu, 0, NULL},
	{"&Rocksmith", NULL, eof_track_rocksmith_menu, 0, NULL},
	{"&Phase Shift", NULL, eof_track_phaseshift_menu, 0, NULL},
	{"Set &Difficulty", NULL, eof_track_menu_set_difficulty, 0, NULL},
	{"Re&name", eof_track_rename, NULL, 0, NULL},
	{"Disable expert+ bass drum", eof_menu_track_disable_double_bass_drums, NULL, 0, NULL},
	{"Erase track", eof_track_erase_track, NULL, 0, NULL},
	{"Erase track difficulty", eof_track_erase_track_difficulty, NULL, 0, NULL},
	{"Erase &Highlighting", eof_menu_track_remove_highlighting, NULL, 0, NULL},
	{"&Thin diff. to match", NULL, eof_menu_thin_diff_menu, 0, NULL},
	{"Delete active difficulty", eof_track_delete_difficulty, NULL, 0, NULL},
	{"&Clone from", NULL, eof_menu_track_clone_menu, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_track_rocksmith_arrangement_menu[] =
{
	{"&Undefined", eof_track_rocksmith_arrangement_undefined, NULL, 0, NULL},
	{"&Combo", eof_track_rocksmith_arrangement_combo, NULL, 0, NULL},
	{"&Rhythm", eof_track_rocksmith_arrangement_rhythm, NULL, 0, NULL},
	{"&Lead", eof_track_rocksmith_arrangement_lead, NULL, 0, NULL},
	{"&Bass", eof_track_rocksmith_arrangement_bass, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

void eof_prepare_track_menu(void)
{
	unsigned long i, j, tracknum;

	if(eof_song && eof_song_loaded)
	{//If a chart is loaded
		tracknum = eof_song->track[eof_selected_track]->tracknum;

		/* enable pro guitar and rocksmith submenus */
		if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If a pro guitar track is active
			eof_track_menu[0].flags = 0;			//Track>Pro Guitar> submenu
			eof_track_menu[1].flags = 0;			//Track>Rocksmith> submenu

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
		}
		else
		{	//Otherwise disable these menu items
			eof_track_menu[0].flags = D_DISABLED;
			eof_track_menu[1].flags = D_DISABLED;
		}

		/* enable open strum */
		if(eof_open_strum_enabled(eof_selected_track))
		{
			eof_track_phaseshift_menu[0].flags = D_SELECTED;	//Track>Phase Shift>Enable open strum
		}
		else
		{
			eof_track_phaseshift_menu[0].flags = 0;
		}

		/* enable five lane drums */
		if(eof_five_lane_drums_enabled())
		{
			eof_track_phaseshift_menu[1].flags = D_SELECTED;	//Track>Phase Shift>Enable five lane drums
		}
		else
		{
			eof_track_phaseshift_menu[1].flags = 0;
		}

		/* Disable expert+ bass drum */
		if(eof_song->tags->double_bass_drum_disabled)
		{
			eof_track_menu[5].flags = D_SELECTED;	//Track>Disable expert+ bass drum
		}
		else
		{
			eof_track_menu[5].flags = 0;
		}

		/* Unshare drum phrasing */
		if(eof_song->tags->unshare_drum_phrasing)
		{
			eof_track_phaseshift_menu[2].flags = D_SELECTED;	//Track>Phase Shift>Unshare drum phrasing
		}
		else
		{
			eof_track_phaseshift_menu[2].flags = 0;
		}

		/* popup messages copy from */
		for(i = 0; i < EOF_TRACKS_MAX; i++)
		{	//For each track supported by EOF
			eof_menu_track_rocksmith_popup_copy_menu[i].flags = 0;
			if((i + 1 < EOF_TRACKS_MAX) && (i + 1 < eof_song->tracks) && (eof_song->track[i + 1] != NULL))
			{	//If the track exists, copy its name into the string used by the track menu
				(void) ustrcpy(eof_menu_track_rocksmith_popup_copy_menu_text[i], eof_song->track[i + 1]->name);
					//Copy the track name to the menu string
			}
			else
			{	//Write a blank string for the track name
				(void) ustrcpy(eof_menu_track_rocksmith_popup_copy_menu_text[i],"");
			}
			if(!eof_get_num_popup_messages(eof_song, i + 1) || (i + 1 == eof_selected_track))
			{	//If the track has no popup messages or is the active track
				eof_menu_track_rocksmith_popup_copy_menu[i].flags = D_DISABLED;	//Disable the track from the submenu
			}
		}

		/* Thin difficulty to match */
		for(i = 0; i < EOF_TRACKS_MAX; i++)
		{	//For each track supported by EOF
			eof_menu_thin_diff_menu[i].flags = D_DISABLED;
			if((i + 1 < EOF_TRACKS_MAX) && (i + 1 < eof_song->tracks) && (eof_song->track[i + 1] != NULL))
			{	//If the track exists, copy its name into the string used by the track menu
				(void) ustrcpy(eof_menu_thin_diff_menu_text[i], eof_song->track[i + 1]->name);
					//Copy the track name to the menu string
			}
			else
			{	//Write a blank string for the track name
				(void) ustrcpy(eof_menu_thin_diff_menu_text[i],"");
			}
			for(j = 0; j < eof_get_track_size(eof_song, i + 1); j++)
			{	//For each note in the track
				if(eof_get_note_type(eof_song,i + 1, j) == eof_note_type)
				{	//If the note is in the active track's difficulty
					eof_menu_thin_diff_menu[i].flags = 0;	//Enable the track from the submenu
					break;
				}
			}
		}

		/* Clone from */
		for(i = 0; i < EOF_TRACKS_MAX - 1; i++)
		{	//For each track supported by EOF
			eof_menu_track_clone_menu[i].flags = D_DISABLED;
			if((i + 1 < EOF_TRACKS_MAX) && (i + 1 < eof_song->tracks) && (eof_song->track[i + 1] != NULL))
			{	//If the track exists, copy its name into the string used by the track menu
				(void) ustrcpy(eof_menu_track_clone_menu_text[i], eof_song->track[i + 1]->name);
					//Copy the track name to the menu string
			}
			else
			{	//Write a blank string for the track name
				(void) ustrcpy(eof_menu_track_clone_menu_text[i],"");
			}

			if(i + 1 == EOF_TRACK_VOCALS)
				continue;	//This function cannot be used with the vocal track since there isn't a second such track
			if(i + 1 == eof_selected_track)
				continue;	//This function cannot clone the active track from itself
			if(eof_song->track[eof_selected_track]->track_format != eof_song->track[i + 1]->track_format)
				continue;	//This function can only be used to clone a track of the same format as the active track

			eof_menu_track_clone_menu[i].flags = 0;	//Enable the track from the submenu
		}
	}//If a chart is loaded
}

#define eof_track_difficulty_menu_X 0
#define eof_track_difficulty_menu_Y 48
DIALOG eof_track_difficulty_menu[] =
{
	/* (proc)                (x) (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)           (dp2)    (dp3) */
	{ d_agup_window_proc,    eof_track_difficulty_menu_X,  eof_track_difficulty_menu_Y,  232, 146, 2,   23,  0,    0,      0,   0,   "Set track difficulty", NULL, NULL },
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

	if(!eof_song || !eof_song_loaded || !eof_selected_track || (eof_selected_track >= eof_song->tracks))
		return 1;

	if(difficulty != eof_song->track[eof_selected_track]->difficulty)
	{
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		eof_song->track[eof_selected_track]->difficulty = difficulty;
		undo_made = 1;
	}

	//Drum tracks also define a pro drum difficulty and vocal tracks also efine a harmony difficulty
	//Set those values if applicable
	if(eof_selected_track == EOF_TRACK_DRUM)
	{
		olddiff = (eof_song->track[EOF_TRACK_DRUM]->flags & 0x0F000000) >> 24;		//Mask out the low nibble of the high order byte of the drum track's flags (pro drum difficulty)
		if(difficulty != olddiff)
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
		if(difficulty != olddiff)
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
	centre_dialog(eof_track_difficulty_menu);

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

	//Manually re-center these elements, because they are not altered by centre_dialog()
	eof_track_difficulty_menu[5].x += eof_track_difficulty_menu[0].x - eof_track_difficulty_menu_X;	//Add the X amount offset by centre_dialog()
	eof_track_difficulty_menu[5].y += eof_track_difficulty_menu[0].y - eof_track_difficulty_menu_Y;	//Add the Y amount offset by centre_dialog()
	eof_track_difficulty_menu[6].x += eof_track_difficulty_menu[0].x - eof_track_difficulty_menu_X;	//Add the X amount offset by centre_dialog()
	eof_track_difficulty_menu[6].y += eof_track_difficulty_menu[0].y - eof_track_difficulty_menu_Y;	//Add the Y amount offset by centre_dialog()

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
	{ d_agup_window_proc,0,  48,  314, 112, 2,   23,  0,    0,      0,   0,   "Rename track",NULL, NULL },
	{ d_agup_text_proc,  12, 84,  64,  8,   2,   23,  0,    0,      0,   0,   "New name:",   NULL, NULL },
	{ d_agup_edit_proc,  90, 80,  212, 20,  2,   23,  0, 0,EOF_NAME_LENGTH,0, eof_etext,     NULL, NULL },
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
	centre_dialog(eof_track_rename_dialog);

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
					if(!ustrncmp(eof_etext, eof_song->track[ctr]->name, EOF_NAME_LENGTH) || !ustrncmp(eof_etext, eof_song->track[ctr]->altname, EOF_NAME_LENGTH))
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
	unsigned long tracknum, ctr;
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
	strncat(eof_tuning_name, "        ", sizeof(eof_tuning_name) - 1 - strlen(eof_tuning_name));	//Pad the end of the string with several spaces
}

int eof_edit_tuning_proc(int msg, DIALOG *d, int c)
{
	int i;
	char * string = NULL;
	unsigned key_list[32] = {KEY_BACKSPACE, KEY_DEL, KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN, KEY_ESC, KEY_ENTER};
	int match = 0;
	int retval;
	char tuning[EOF_TUNING_LENGTH] = {0};
	unsigned c2 = (unsigned)c;	//Force cast this to unsigned because Splint is incapable of avoiding a false positive detecting it as negative despite assertions proving otherwise

	if(!d)	//If this pointer is NULL for any reason
		return d_agup_edit_proc(msg, d, c);	//Allow the input character to be returned

	if((msg != MSG_CHAR) && (msg != MSG_UCHAR))
		return d_agup_edit_proc(msg, d, c);		//If this isn't a character input message, allow the input character to be returned

	for(i = 0; i < 8; i++)
	{	//Check each of the pre-defined allowable keys
		if((msg == MSG_UCHAR) && (c2 == 27))
		{	//If the Escape ASCII character was trapped
			return d_agup_edit_proc(msg, d, c2);	//Immediately allow the input character to be returned (so the user can escape to cancel the dialog)
		}
		if((msg == MSG_CHAR) && ((c2 >> 8 == KEY_BACKSPACE) || (c2 >> 8 == KEY_DEL)))
		{	//If the backspace or delete keys are trapped
			match = 1;	//Ensure the full function runs, so that the strings are rebuilt
			break;
		}
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

	return retval;
}

DIALOG eof_pro_guitar_tuning_dialog[] =
{
	/*	(proc)				(x)  (y)  (w)  (h) (fg) (bg) (key) (flags) (d1)       (d2) (dp)          		(dp2) (dp3) */
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

int eof_track_tuning(void)
{
	unsigned long ctr, tracknum, stringcount;
	long newval;
	char undo_made = 0, newtuning[6] = {0};
	EOF_PRO_GUITAR_TRACK *tp;
	int focus = 5;	//By default, start dialog focus in the tuning box for string 1 (top-most box for 6 string track)

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
	centre_dialog(eof_pro_guitar_tuning_dialog);

//Update the strings to reflect the currently set tuning
	stringcount = tp->numstrings;
	for(ctr = 0; ctr < 6; ctr++)
	{	//For each of the 6 supported strings
		if(ctr < stringcount)
		{	//If this track uses this string, convert the tuning to its string representation
			(void) snprintf(eof_fret_strings[ctr], sizeof(eof_string_lane_1) - 1, "%d", tp->tuning[ctr]);
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
	eof_rebuild_tuning_strings(tp->tuning);

	//Adjust initial focus to account for whichever the top-most string is
	if(stringcount == 5)
		focus = 8;
	else if(stringcount == 4)
		focus = 11;

	if(eof_popup_dialog(eof_pro_guitar_tuning_dialog, focus) == 22)
	{	//If user clicked OK
		//Validate and store the input
		for(ctr = 0; ctr < tp->numstrings; ctr++)
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
		for(ctr = 0; ctr < 6; ctr++)
		{	//For each of the 6 supported strings
			if(ctr < tp->numstrings)
			{	//If this string is used in the track, store the numerical value into the track's tuning array
				if(!undo_made && (tp->tuning[ctr] != atol(eof_fret_strings[ctr]) % 12))
				{	//If a tuning was changed
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					undo_made = 1;
					eof_chord_lookup_note = 0;	//Reset the cached chord lookup count
				}
				newval = atol(eof_fret_strings[ctr]) % 12;
				newtuning[ctr] = newval - tp->tuning[ctr];	//Find this string's tuning change (in half steps)
				tp->tuning[ctr] = newval;
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
	}//If user clicked OK

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
	{ d_agup_window_proc,     32,  68,  170, 136, 0,   0,    0,    0,     0,   0,   "Edit fret/string count", NULL, NULL },
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
	centre_dialog(eof_note_set_num_frets_strings_dialog);
	if(eof_popup_dialog(eof_note_set_num_frets_strings_dialog, 2) != 7)
		return 1;	//If the user did not click OK, return immediately

	//Update max fret number
	newnumfrets = atol(eof_etext2);
	if(newnumfrets && (newnumfrets != eof_song->pro_guitar_track[tracknum]->numfrets))
	{	//If the specified number of frets was changed
		highestfret = eof_get_highest_fret(eof_song, eof_selected_track, 0);	//Get the highest used fret value in this track
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
	{ d_agup_window_proc,    0,   0,   200, 132, 0,   0,   0,    0,      0,   0,   "Set capo position",    NULL, NULL },
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
	centre_dialog(eof_track_pro_guitar_set_capo_position_dialog);

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

MENU eof_track_proguitar_fret_hand_menu[] =
{
	{"&Set\tShift+F", eof_track_pro_guitar_set_fret_hand_position, NULL, 0, NULL},
	{"&List\t" CTRL_NAME "+Shift+F", eof_track_fret_hand_positions, NULL, 0, NULL},
	{"&Copy", eof_track_fret_hand_positions_copy_from, NULL, 0, NULL},
	{"Generate all diffs", eof_track_fret_hand_positions_generate_all, NULL, 0, NULL},
	{"Delete effective\tShift+Del", eof_track_delete_effective_fret_hand_position, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

char eof_track_pro_guitar_set_fret_hand_position_dialog_string1[] = "Set fret hand position";
char eof_track_pro_guitar_set_fret_hand_position_dialog_string2[] = "Edit fret hand position";
DIALOG eof_track_pro_guitar_set_fret_hand_position_dialog[] =
{
	/* (proc)                (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                    (dp2) (dp3) */
	{ d_agup_window_proc,    0,   0,   200, 132, 0,   0,   0,    0,      0,   0,   eof_track_pro_guitar_set_fret_hand_position_dialog_string1,      NULL, NULL },
	{ d_agup_text_proc,      12,  40,  60,  12,  0,   0,   0,    0,      0,   0,   "At fret #",                NULL, NULL },
	{ eof_verified_edit_proc,12,  56,  50,  20,  0,   0,   0,    0,      2,   0,   eof_etext,     "0123456789", NULL },
	{ d_agup_button_proc,    12,  92,  84,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",               NULL, NULL },
	{ d_agup_button_proc,    110, 92,  78,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",           NULL, NULL },
	{ NULL,                  0,   0,   0,   0,   0,   0,   0,    0,      0,   0,   NULL,               NULL, NULL }
};

int eof_track_pro_guitar_set_fret_hand_position(void)
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
	centre_dialog(eof_track_pro_guitar_set_fret_hand_position_dialog);

	//Find the pointer to the fret hand position at the current seek position in this difficulty, if there is one
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	tp = eof_song->pro_guitar_track[tracknum];
	ptr = eof_pro_guitar_track_find_effective_fret_hand_position_definition(tp, eof_note_type, eof_music_pos - eof_av_delay, &index, NULL, 1);
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
			(void) eof_track_add_section(eof_song, eof_selected_track, EOF_FRET_HAND_POS_SECTION, eof_note_type, eof_music_pos - eof_av_delay, position, 0, NULL);
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
	if(eof_pro_guitar_track_find_effective_fret_hand_position_definition(tp, eof_note_type, eof_music_pos - eof_av_delay, &index, NULL, 0))
	{	//If there is a fret hand position in effect at the current seek position in the active track difficulty
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
					*size = ecount;
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
	{ d_agup_window_proc,0,   48,  250, 237, 2,   23,  0,    0,      0,   0,   "Fret hand positions",       NULL, NULL },
	{ d_agup_list_proc,  12,  84,  150, 138, 2,   23,  0,    0,      0,   0,   (void *)eof_fret_hand_position_list,NULL, NULL },
	{ d_agup_push_proc,  170, 84,  68,  28,  2,   23,  'l',  D_EXIT, 0,   0,   "De&lete",      NULL, (void *)eof_fret_hand_position_delete },
	{ d_agup_push_proc,  170, 124, 68,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Delete all",   NULL, (void *)eof_fret_hand_position_delete_all },
	{ d_agup_push_proc,  170, 164, 68,  28,  2,   23,  's',  D_EXIT, 0,   0,   "&Seek to",     NULL, (void *)eof_fret_hand_position_seek },
	{ d_agup_push_proc,  170, 204, 68,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Generate",     NULL, (void *)eof_generate_hand_positions_current_track_difficulty },
	{ d_agup_button_proc,12,  245, 90,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "Done",         NULL, NULL },
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
	centre_dialog(eof_fret_hand_position_list_dialog);

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	(void) eof_pro_guitar_track_find_effective_fret_hand_position_definition(eof_song->pro_guitar_track[tracknum], eof_note_type, eof_music_pos - eof_av_delay, NULL, &diffindex, 0);	//Determine if a hand position exists at the current seek position
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
	{ d_agup_window_proc,    0,   0,   200, 190, 0,   0,   0,    0,      0,   0,   "Rocksmith popup message",      NULL, NULL },
	{ d_agup_edit_proc,      12,  30,  176, 20,  2,   23,  0,    0,      EOF_SECTION_NAME_LENGTH,   0,   eof_etext,           NULL, NULL },
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
	centre_dialog(eof_song_rs_popup_add_dialog);

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
		(void) snprintf(eof_etext2, sizeof(eof_etext2) - 1, "%lu", eof_music_pos - eof_av_delay);	//Otherwise initialize the start time with the current seek position
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
	{ d_agup_window_proc,0,   48,  400, 237, 2,   23,  0,    0,      0,   0,   eof_rs_popup_messages_dialog_string,NULL, NULL },
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
	if(eof_find_effective_rs_popup_message(eof_music_pos - eof_av_delay, &popupmessage))
	{	//If a popup message is in effect at the current seek position
		eof_rs_popup_messages_dialog[1].d1 = popupmessage;	//Pre-select the popup message from the list
	}

	//Call the dialog
	eof_clear_input();
	eof_rs_popup_messages_dialog_undo_made = 0;	//Reset this condition
	eof_color_dialog(eof_rs_popup_messages_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_rs_popup_messages_dialog);
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
	centre_dialog(eof_song_rs_popup_add_dialog);

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

MENU eof_track_rocksmith_tone_change_menu[] =
{
	{"&Add\t" CTRL_NAME "+Shift+T", eof_track_rs_tone_change_add, NULL, 0, NULL},
	{"&List", eof_track_rs_tone_changes, NULL, 0, NULL},
	{"&Names", eof_track_rs_tone_names, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

char eof_menu_track_rocksmith_popup_copy_menu_text[EOF_TRACKS_MAX][EOF_TRACK_NAME_SIZE] = {{0}};
MENU eof_menu_track_rocksmith_popup_copy_menu[EOF_TRACKS_MAX] =
{
	{eof_menu_track_rocksmith_popup_copy_menu_text[0], eof_menu_track_copy_popups_track_1, NULL, D_SELECTED, NULL},
	{eof_menu_track_rocksmith_popup_copy_menu_text[1], eof_menu_track_copy_popups_track_2, NULL, 0, NULL},
	{eof_menu_track_rocksmith_popup_copy_menu_text[2], eof_menu_track_copy_popups_track_3, NULL, 0, NULL},
	{eof_menu_track_rocksmith_popup_copy_menu_text[3], eof_menu_track_copy_popups_track_4, NULL, 0, NULL},
	{eof_menu_track_rocksmith_popup_copy_menu_text[4], eof_menu_track_copy_popups_track_5, NULL, 0, NULL},
	{eof_menu_track_rocksmith_popup_copy_menu_text[5], eof_menu_track_copy_popups_track_6, NULL, 0, NULL},
	{eof_menu_track_rocksmith_popup_copy_menu_text[6], eof_menu_track_copy_popups_track_7, NULL, 0, NULL},
	{eof_menu_track_rocksmith_popup_copy_menu_text[7], eof_menu_track_copy_popups_track_8, NULL, 0, NULL},
	{eof_menu_track_rocksmith_popup_copy_menu_text[8], eof_menu_track_copy_popups_track_9, NULL, 0, NULL},
	{eof_menu_track_rocksmith_popup_copy_menu_text[9], eof_menu_track_copy_popups_track_10, NULL, 0, NULL},
	{eof_menu_track_rocksmith_popup_copy_menu_text[10], eof_menu_track_copy_popups_track_11, NULL, 0, NULL},
	{eof_menu_track_rocksmith_popup_copy_menu_text[11], eof_menu_track_copy_popups_track_12, NULL, 0, NULL},
	{eof_menu_track_rocksmith_popup_copy_menu_text[12], eof_menu_track_copy_popups_track_13, NULL, 0, NULL},
	{eof_menu_track_rocksmith_popup_copy_menu_text[13], eof_menu_track_copy_popups_track_14, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_track_rocksmith_popup_menu[] =
{
	{"&Add", eof_track_rs_popup_add, NULL, 0, NULL},
	{"&List", eof_track_rs_popup_messages, NULL, 0, NULL},
	{"&Copy From", NULL, eof_menu_track_rocksmith_popup_copy_menu, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_track_rocksmith_menu[] =
{
	{"Enable tech &View\tF4", eof_menu_track_toggle_tech_view, NULL, 0, NULL},
	{"Fret &Hand positions", NULL, eof_track_proguitar_fret_hand_menu, 0, NULL},
	{"&Popup messages", NULL, eof_track_rocksmith_popup_menu, 0, NULL},
	{"&Arrangement type", NULL, eof_track_rocksmith_arrangement_menu, 0, NULL},
	{"&Tone change", NULL, eof_track_rocksmith_tone_change_menu, 0, NULL},
	{"Remove difficulty limit", eof_track_rocksmith_toggle_difficulty_limit, NULL, 0, NULL},
	{"Insert new difficulty", eof_track_rocksmith_insert_difficulty, NULL, 0, NULL},
	{"&Manage RS phrases\t" CTRL_NAME "+Shift+M", eof_track_manage_rs_phrases, NULL, 0, NULL},
	{"Flatten this difficulty", eof_track_flatten_difficulties, NULL, 0, NULL},
	{"Un-flatten track", eof_track_unflatten_difficulties, NULL, 0, NULL},
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

DIALOG eof_track_fret_hand_positions_copy_from_dialog[] =
{
	/* (proc)            (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)            (dp2) (dp3) */
	{ d_agup_window_proc,0,   48,  250, 237, 2,   23,  0,    0,      0,   0,   "Copy fret hand positions from diff #", NULL, NULL },
	{ d_agup_list_proc,  12,  84,  226, 138, 2,   23,  0,    0,      0,   0,   (void *)eof_track_fret_hand_positions_copy_from_list,NULL, NULL },
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
	centre_dialog(eof_track_fret_hand_positions_copy_from_dialog);
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
	{ d_agup_window_proc,0,   48,  400, 237, 2,   23,  0,    0,      0,   0,   eof_track_manage_rs_phrases_dialog_string, NULL, NULL },
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
			started = 0;
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
	eof_track_manage_rs_phrases_dialog[1].d1 = eof_find_effective_rs_phrase(eof_music_pos - eof_av_delay);	//Pre-select the phrase in effect at the current position

	//Call the dialog
	eof_color_dialog(eof_track_manage_rs_phrases_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_track_manage_rs_phrases_dialog);
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
	{ d_agup_window_proc,    0,   0,   200, 132, 0,   0,   0,    0,      0,   0,   "Merge notes that are within", NULL, NULL },
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
	centre_dialog(eof_track_flatten_difficulties_dialog);

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

	if(!eof_song || (eof_selected_track >= eof_song->tracks) || (eof_song->track[eof_selected_track]->track_format != EOF_LEGACY_TRACK_FORMAT) || (eof_song->track[eof_selected_track]->track_behavior != EOF_GUITAR_TRACK_BEHAVIOR))
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
		eof_song->track[eof_selected_track]->flags |= EOF_TRACK_FLAG_SIX_LANES;	//Set the flag
		eof_song->legacy_track[tracknum]->numlanes = 6;
	}
	eof_scale_fretboard(0);	//Recalculate the 2D screen positioning based on the current track
	return 1;
}

int eof_menu_track_five_lane_drums(void)
{
	unsigned long tracknum = eof_song->track[EOF_TRACK_DRUM]->tracknum;
	unsigned long tracknum2 = eof_song->track[EOF_TRACK_DRUM_PS]->tracknum;

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

	eof_set_3D_lane_positions(0);	//Update xchart[] by force
	eof_scale_fretboard(0);			//Recalculate the 2D screen positioning based on the current track
	return 1;
}

int eof_menu_track_unshare_drum_phrasing(void)
{
	if(eof_song)
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
	eof_erase_track(eof_song, eof_selected_track);
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

DIALOG eof_track_rs_tone_change_add_dialog[] =
{
	/* (proc)                (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                     (dp2) (dp3) */
	{ d_agup_window_proc,    0,   0,   200, 116, 0,   0,   0,    0,      0,   0,   "Rocksmith tone change", NULL, NULL },
	{ d_agup_text_proc,      12,  30,  60,  12,  0,   0,   0,    0,      0,   0,   "Tone key name:",        NULL, NULL },
	{ d_agup_edit_proc,      12,  46,  176, 20,  2,   23,  0,    0,      EOF_SECTION_NAME_LENGTH,   0, eof_etext, NULL, NULL },
	{ d_agup_button_proc,    12,  76,  84,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",                    NULL, NULL },
	{ d_agup_button_proc,    110, 76,  78,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",                NULL, NULL },
	{ NULL,                  0,   0,   0,   0,   0,   0,   0,    0,      0,   0,   NULL,                    NULL, NULL }
};

int eof_track_rs_tone_change_add(void)
{
	EOF_PRO_GUITAR_TRACK *tp;
	unsigned long tracknum, ctr;
	char defaulttone = 0;

	if(!eof_song_loaded || !eof_song || (eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return 1;	//Do not allow this function to run if a pro guitar track isn't active

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	tp = eof_song->pro_guitar_track[tracknum];
	eof_render();

	//Find the tone change at the current seek position, if any
	for(ctr = 0; ctr < tp->tonechanges; ctr++)
	{	//For each tone change in the track
		if(tp->tonechange[ctr].start_pos != eof_music_pos - eof_av_delay)
			continue;	//If this tone change is not at the current seek position, skip it

		//Otherwise edit it instead of adding a new tone change
		eof_color_dialog(eof_track_rs_tone_change_add_dialog, gui_fg_color, gui_bg_color);
		centre_dialog(eof_track_rs_tone_change_add_dialog);
		(void) ustrcpy(eof_etext, tp->tonechange[ctr].name);
		eof_clear_input();
		if(eof_popup_dialog(eof_track_rs_tone_change_add_dialog, 2) == 3)
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
	centre_dialog(eof_track_rs_tone_change_add_dialog);

	(void) ustrcpy(eof_etext, "");
	if(eof_popup_dialog(eof_track_rs_tone_change_add_dialog, 2) == 3)
	{	//User clicked OK
		if(eof_etext[0] != '\0')
		{	//If a tone key name is specified
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			if((tp->defaulttone[0] != '\0') && (!strcmp(tp->defaulttone, eof_etext)))
			{	//If the tone being changed to is the currently defined default tone
					defaulttone = 1;
			}
			(void) eof_track_add_section(eof_song, eof_selected_track, EOF_RS_TONE_CHANGE, 0, eof_music_pos - eof_av_delay, defaulttone, 0, eof_etext);
		}
        eof_track_pro_guitar_sort_tone_changes(tp);	//Re-sort the tone changes
	}

	return 1;
}

char **eof_track_rs_tone_changes_list_strings = NULL;	//Stores allocated strings for eof_track_rs_tone_changes()
char eof_track_rs_tone_changes_dialog_undo_made = 0;	//Used to track whether an undo state was made in this dialog
char eof_track_rs_tone_changes_dialog_string[35] = {0};	//The title string for the RS tone changes dialog

DIALOG eof_track_rs_tone_changes_dialog[] =
{
	/* (proc)            (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)            (dp2) (dp3) */
	{ d_agup_window_proc,0,   48,  400, 237, 2,   23,  0,    0,      0,   0,   eof_track_rs_tone_changes_dialog_string,NULL, NULL },
	{ d_agup_list_proc,  12,  84,  300, 138, 2,   23,  0,    0,      0,   0,   (void *)eof_track_rs_tone_changes_list,NULL, NULL },
	{ d_agup_push_proc,  320, 84,  68,  28,  2,   23,  'l',  D_EXIT, 0,   0,   "De&lete",      NULL, (void *)eof_track_rs_tone_changes_delete },
	{ d_agup_push_proc,  320, 124, 68,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Delete all",   NULL, (void *)eof_track_rs_tone_changes_delete_all },
	{ d_agup_push_proc,  320, 164, 68,  28,  2,   23,  's',  D_EXIT, 0,   0,   "&Seek to",     NULL, (void *)eof_track_rs_tone_changes_seek },
	{ d_agup_push_proc,  320, 204, 68,  28,  2,   23,  'e',  D_EXIT, 0,   0,   "&Edit",        NULL, (void *)eof_track_rs_tone_changes_edit },
	{ d_agup_button_proc,12,  245, 90,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "Done",         NULL, NULL },
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

	if(!eof_song || (eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return 1;	//Do not allow this function to run if a pro guitar track isn't active

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	tp = eof_song->pro_guitar_track[tracknum];
	eof_track_pro_guitar_sort_tone_changes(tp);	//Re-sort the tone changes

	//Allocate and build the strings for the tone changes
	eof_track_rebuild_rs_tone_changes_list_strings();
	if(eof_track_find_effective_rs_tone_change(eof_music_pos - eof_av_delay, &tonechange))
	{	//If a tone change had been placed at or before the current seek position
		eof_track_rs_tone_changes_dialog[1].d1 = tonechange;	//Pre-select the tone change from the list
	}

	//Call the dialog
	eof_clear_input();
	eof_track_rs_tone_changes_dialog_undo_made = 0;	//Reset this condition
	eof_color_dialog(eof_track_rs_tone_changes_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_track_rs_tone_changes_dialog);
	(void) eof_popup_dialog(eof_track_rs_tone_changes_dialog, 1);

	//Cleanup
	for(ctr = 0; ctr < tp->tonechanges; ctr++)
	{	//Free previously allocated strings
		free(eof_track_rs_tone_changes_list_strings[ctr]);
	}
	free(eof_track_rs_tone_changes_list_strings);
	eof_track_rs_tone_changes_list_strings = NULL;

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

	//Rebuild the strings for the dialog menu
	eof_track_rebuild_rs_tone_changes_list_strings();

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
	centre_dialog(eof_track_rs_tone_change_add_dialog);

	//Initialize the dialog fields
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	tp = eof_song->pro_guitar_track[tracknum];
	if((unsigned long)eof_track_rs_tone_changes_dialog[1].d1 >= tp->tonechanges)
		return D_O_K;	//Invalid tone change selected in list
	ptr = &(tp->tonechange[eof_track_rs_tone_changes_dialog[1].d1]);
	(void) ustrcpy(eof_etext, ptr->name);

	eof_clear_input();
	if(eof_popup_dialog(eof_track_rs_tone_change_add_dialog, 2) == 3)
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
	{ d_agup_window_proc,0,   48,  400, 237, 2,   23,  0,    0,      0,   0,   eof_track_rs_tone_names_dialog_string,NULL, NULL },
	{ d_agup_list_proc,  12,  84,  300, 138, 2,   23,  0,    0,      0,   0,   (void *)eof_track_rs_tone_names_list,NULL, NULL },
	{ d_agup_push_proc,  320, 84,  68,  28,  2,   23,  'd',  D_EXIT, 0,   0,   "&Default",     NULL, (void *)eof_track_rs_tone_names_default },
	{ d_agup_push_proc,  320, 124, 68,  28,  2,   23,  'r',  D_EXIT, 0,   0,   "&Rename",      NULL, (void *)eof_track_rs_tone_names_rename },
	{ d_agup_button_proc,12,  245, 90,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "Done",         NULL, NULL },
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
	centre_dialog(eof_track_rs_tone_names_dialog);
	(void) eof_popup_dialog(eof_track_rs_tone_names_dialog, 1);

	//Cleanup
	eof_track_destroy_rs_tone_names_list_strings();

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
	centre_dialog(eof_track_rs_tone_change_add_dialog);
	strncpy(eof_etext, eof_track_rs_tone_names_list_strings[namenum], EOF_SECTION_NAME_LENGTH);
	if(eof_popup_dialog(eof_track_rs_tone_change_add_dialog, 2) == 3)
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
				}
			}
		}
	}

	//Release the tone name strings showing the (D) suffix for the default tone
	eof_track_destroy_rs_tone_names_list_strings();
	eof_track_rebuild_rs_tone_names_list_strings(eof_selected_track, 1);

	return D_REDRAW;	//Have Allegro redraw the dialog
}

int eof_menu_track_copy_popups_track_1(void)
{
	return eof_menu_track_copy_popups_track_number(eof_song, 1, eof_selected_track);
}

int eof_menu_track_copy_popups_track_2(void)
{
	return eof_menu_track_copy_popups_track_number(eof_song, 2, eof_selected_track);
}

int eof_menu_track_copy_popups_track_3(void)
{
	return eof_menu_track_copy_popups_track_number(eof_song, 3, eof_selected_track);
}

int eof_menu_track_copy_popups_track_4(void)
{
	return eof_menu_track_copy_popups_track_number(eof_song, 4, eof_selected_track);
}

int eof_menu_track_copy_popups_track_5(void)
{
	return eof_menu_track_copy_popups_track_number(eof_song, 5, eof_selected_track);
}

int eof_menu_track_copy_popups_track_6(void)
{
	return eof_menu_track_copy_popups_track_number(eof_song, 6, eof_selected_track);
}

int eof_menu_track_copy_popups_track_7(void)
{
	return eof_menu_track_copy_popups_track_number(eof_song, 7, eof_selected_track);
}

int eof_menu_track_copy_popups_track_8(void)
{
	return eof_menu_track_copy_popups_track_number(eof_song, 8, eof_selected_track);
}

int eof_menu_track_copy_popups_track_9(void)
{
	return eof_menu_track_copy_popups_track_number(eof_song, 9, eof_selected_track);
}

int eof_menu_track_copy_popups_track_10(void)
{
	return eof_menu_track_copy_popups_track_number(eof_song, 10, eof_selected_track);
}

int eof_menu_track_copy_popups_track_11(void)
{
	return eof_menu_track_copy_popups_track_number(eof_song, 11, eof_selected_track);
}

int eof_menu_track_copy_popups_track_12(void)
{
	return eof_menu_track_copy_popups_track_number(eof_song, 12, eof_selected_track);
}

int eof_menu_track_copy_popups_track_13(void)
{
	return eof_menu_track_copy_popups_track_number(eof_song, 13, eof_selected_track);
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
	{	//If there are already hand positions in the destination track
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
	unsigned long stracknum, dtracknum, noteset, notesetcount = 1, ctr;
	EOF_TRACK_ENTRY *parent;
	EOF_PRO_GUITAR_NOTE **setptr;
	int populated = 1;	//Set to nonzero if the destination track is found to contain no notes
	char s_restore_tech_view = 0, d_restore_tech_view = 0;	//Store the original tech view states of the source/destination tracks if applicable

	if(!sp || (sourcetrack >= sp->tracks) || (desttrack >= sp->tracks) || (sourcetrack == desttrack))
		return 0;	//Invalid parameters
	if(sp->track[sourcetrack]->track_format != sp->track[desttrack]->track_format)
		return 0;	//Invalid parameters

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
	eof_erase_track(sp, desttrack);

	//Clone the source track
	sp->track[desttrack]->difficulty = sp->track[sourcetrack]->difficulty;
	sp->track[desttrack]->numdiffs = sp->track[sourcetrack]->numdiffs;
	sp->track[desttrack]->flags = sp->track[sourcetrack]->flags;
	stracknum = sp->track[sourcetrack]->tracknum;
	dtracknum = sp->track[desttrack]->tracknum;
	switch(sp->track[sourcetrack]->track_format)
	{
		case EOF_LEGACY_TRACK_FORMAT:
			parent = sp->legacy_track[dtracknum]->parent;	//Back up this pointer
			memcpy(sp->legacy_track[dtracknum], sp->legacy_track[stracknum], sizeof(EOF_LEGACY_TRACK));
			sp->legacy_track[dtracknum]->parent = parent;	//Restore this pointer
			sp->legacy_track[dtracknum]->notes = 0;			//Clear the note count
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

	eof_scale_fretboard(0);	//Recalculate the 2D screen positioning based on the current track
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
		eof_menu_pro_guitar_track_disable_tech_view(tp);
	}
	else
	{	//Otherwise put the tech note array into effect
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
