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

MENU eof_track_menu[] =
{
	{"Pro &Guitar", NULL, eof_track_proguitar_menu, 0, NULL},
	{"&Rocksmith", NULL, eof_track_rocksmith_menu, 0, NULL},
	{"Set &Difficulty", eof_track_difficulty_dialog, NULL, 0, NULL},
	{"Re&Name", eof_track_rename, NULL, 0, NULL},
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
	unsigned long i, tracknum;

	if(eof_song && eof_song_loaded)
	{//If a chart is loaded
		tracknum = eof_song->track[eof_selected_track]->tracknum;

		/* enable pro guitar and rocksmith submenus */
		if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If a pro guitar track is active
			eof_track_menu[0].flags = 0;			//Track>Pro Guitar> submenu

			if(eof_song->pro_guitar_track[tracknum]->ignore_tuning)
			{
				eof_track_proguitar_menu[2].flags = D_SELECTED;	//Track>Pro Guitar>Ignore this track's tuning
			}
			else
			{
				eof_track_proguitar_menu[2].flags = 0;
			}

			if(eof_song->track[eof_selected_track]->flags & EOF_TRACK_FLAG_UNLIMITED_DIFFS)
			{	//If the active track has already had the difficulty limit removed
				eof_track_rocksmith_menu[3].flags = D_SELECTED;	//Track>Rocksmith>Remove difficulty limit
			}
			else
			{
				eof_track_rocksmith_menu[3].flags = 0;
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
		(void) snprintf(eof_etext, sizeof(eof_etext) - 1, "%d", eof_song->track[eof_selected_track]->difficulty);
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
				eof_song->track[EOF_TRACK_DRUM]->flags |= (newdifficulty2 << 24);	//Store the pro drum difficulty in the drum track's flag's most significant byte
				eof_song->track[EOF_TRACK_DRUM]->flags |= 0xF0 << 24;				//Set the top nibble to 0xF for backwards compatibility from when this stored the PS drum track difficulty
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
	if(!eof_song_loaded || !eof_song)
		return 1;	//Do not allow this function to run if a chart is not loaded

	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_track_rename_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_track_rename_dialog);

	(void) ustrncpy(eof_etext, eof_song->track[eof_selected_track]->altname, EOF_NAME_LENGTH);	//Update the input field
	if(eof_popup_dialog(eof_track_rename_dialog, 2) == 3)
	{	//If user clicked OK
		if(ustrncmp(eof_etext, eof_song->track[eof_selected_track]->altname, EOF_NAME_LENGTH))
		{	//If the user provided a different alternate track name
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			(void) ustrncpy(eof_song->track[eof_selected_track]->altname, eof_etext, EOF_NAME_LENGTH);	//Update the track entry
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
					strncpy(tuning_list[ctr], eof_note_names[tuning], sizeof(tuning_list[0]) - 1);
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
		(void) object_message(&eof_pro_guitar_tuning_dialog[2], MSG_DRAW, 0);	//Have Allegro redraw the tuning name
		(void) object_message(&eof_pro_guitar_tuning_dialog[6], MSG_DRAW, 0);	//Have Allegro redraw the string tuning strings
		(void) object_message(&eof_pro_guitar_tuning_dialog[9], MSG_DRAW, 0);	//Have Allegro redraw the string tuning strings
		(void) object_message(&eof_pro_guitar_tuning_dialog[12], MSG_DRAW, 0);	//Have Allegro redraw the string tuning strings
		(void) object_message(&eof_pro_guitar_tuning_dialog[15], MSG_DRAW, 0);	//Have Allegro redraw the string tuning strings
		(void) object_message(&eof_pro_guitar_tuning_dialog[18], MSG_DRAW, 0);	//Have Allegro redraw the string tuning strings
		(void) object_message(&eof_pro_guitar_tuning_dialog[21], MSG_DRAW, 0);	//Have Allegro redraw the string tuning strings
		return retval;
	}

	return d_agup_edit_proc(msg, d, c);	//Allow the input character to be returned
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
			(void) snprintf(eof_fret_strings[ctr], sizeof(eof_string_lane_1) - 1, "%d", eof_song->pro_guitar_track[tracknum]->tuning[ctr]);
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
		memset(eof_song->pro_guitar_track[tracknum]->tuning, 0, sizeof(eof_song->pro_guitar_track[tracknum]->tuning));	//Clear the tuning array to ensure that 4 string bass tracks have the 2 unused strings cleared
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

int eof_track_proguitar_toggle_ignore_tuning(void)
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

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run if a pro guitar track isn't active

	//Update dialog fields
	(void) snprintf(eof_etext2, sizeof(eof_etext2) - 1, "%d", eof_song->pro_guitar_track[tracknum]->numfrets);
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
			highestfret = eof_get_highest_fret(eof_song, eof_selected_track, 0);	//Get the highest used fret value in this track
			if(highestfret > newnumfrets)
			{	//If any notes in this track use fret values that would exceed the new fret limit
				char message[120] = {0};
				(void) snprintf(message, sizeof(message) - 1, "Warning:  This track uses frets as high as %lu, exceeding the proposed limit.  Continue?", highestfret);
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

MENU eof_song_proguitar_fret_hand_menu[] =
{
	{"&Set\tShift+F", eof_track_proguitar_set_fret_hand_position, NULL, 0, NULL},
	{"&List\t" CTRL_NAME "+Shift+F", eof_track_fret_hand_positions, NULL, 0, NULL},
	{"&Copy", eof_track_fret_hand_positions_copy_from, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

char eof_track_proguitar_set_fret_hand_position_dialog_string1[] = "Set fret hand position";
char eof_track_proguitar_set_fret_hand_position_dialog_string2[] = "Edit fret hand position";
DIALOG eof_track_proguitar_set_fret_hand_position_dialog[] =
{
	/* (proc)                (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                    (dp2) (dp3) */
	{ d_agup_window_proc,    0,   0,   200, 132, 0,   0,   0,    0,      0,   0,   eof_track_proguitar_set_fret_hand_position_dialog_string1,      NULL, NULL },
	{ d_agup_text_proc,      12,  40,  60,  12,  0,   0,   0,    0,      0,   0,   "At fret #",                NULL, NULL },
	{ eof_verified_edit_proc,12,  56,  50,  20,  0,   0,   0,    0,      2,   0,   eof_etext,     "0123456789", NULL },
	{ d_agup_button_proc,    12,  92,  84,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",               NULL, NULL },
	{ d_agup_button_proc,    110, 92,  78,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",           NULL, NULL },
	{ NULL,                  0,   0,   0,   0,   0,   0,   0,    0,      0,   0,   NULL,               NULL, NULL }
};

int eof_track_proguitar_set_fret_hand_position(void)
{
	unsigned long position, tracknum;
	EOF_PHRASE_SECTION *ptr = NULL;	//If the seek position has a fret hand position defined, this will reference it
	unsigned long index;			//Will store the index number of the existing fret hand position being edited

	if(!eof_song_loaded || !eof_song)
		return 1;	//Do not allow this function to run if a chart is not loaded
	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	eof_render();
	eof_color_dialog(eof_track_proguitar_set_fret_hand_position_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_track_proguitar_set_fret_hand_position_dialog);

	//Find the pointer to the fret hand position at the current seek position in this difficulty, if there is one
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	ptr = eof_pro_guitar_track_find_effective_fret_hand_position_definition(eof_song->pro_guitar_track[tracknum], eof_note_type, eof_music_pos - eof_av_delay, &index, NULL, 1);
	if(ptr)
	{	//If an existing fret hand position is to be edited
		(void) snprintf(eof_etext, 5, "%lu", ptr->end_pos);	//Populate the input box with it
		eof_track_proguitar_set_fret_hand_position_dialog[0].dp = eof_track_proguitar_set_fret_hand_position_dialog_string2;	//Update the dialog window title to reflect that a hand position is being edited
	}
	else
	{
		eof_track_proguitar_set_fret_hand_position_dialog[0].dp = eof_track_proguitar_set_fret_hand_position_dialog_string1;	//Update the dialog window title to reflect that a new hand position is being added
		eof_etext[0] = '\0';	//Empty this string
	}
	if(eof_popup_dialog(eof_track_proguitar_set_fret_hand_position_dialog, 2) == 3)
	{	//User clicked OK
		if(eof_etext[0] != '\0')
		{	//If the user provided a number
			position = atol(eof_etext);
			if(position > eof_song->pro_guitar_track[tracknum]->numfrets)
			{	//If the given fret position is higher than this track's supported number of frets
				allegro_message("You cannot specify a fret hand position that is higher than this track's number of frets (%u).", eof_song->pro_guitar_track[tracknum]->numfrets);
				return 1;
			}
			else if(position > 19)
			{	//19 is the highest valid anchor because fret 22 is the highest supported fret in both Rock Band and Rocksmith
				allegro_message("You cannot specify a fret hand position higher than 19 (it will cause Rocksmith to crash).");
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
				(void) eof_track_add_section(eof_song, eof_selected_track, EOF_FRET_HAND_POS_SECTION, eof_note_type, eof_music_pos - eof_av_delay, position, 0, NULL);
				eof_pro_guitar_track_sort_fret_hand_positions(eof_song->pro_guitar_track[tracknum]);	//Sort the positions, since they must be in order for displaying to the user
			}
		}
		else if(ptr)
		{	//If the user left the input box empty and was editing an existing hand position
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			eof_pro_guitar_track_delete_hand_position(eof_song->pro_guitar_track[tracknum], index);	//Delete the existing fret hand position
			eof_pro_guitar_track_sort_fret_hand_positions(eof_song->pro_guitar_track[tracknum]);	//Sort the positions, since they must be in order for displaying to the user
		}
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

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	if(eof_song->pro_guitar_track[tracknum]->handpositions == 0)
	{
		return D_O_K;
	}
	for(i = 0; i < eof_song->pro_guitar_track[tracknum]->handpositions; i++)
	{	//For each fret hand position
		if(eof_song->pro_guitar_track[tracknum]->handposition[i].difficulty == eof_note_type)
		{	//If the fret hand position is in the active difficulty
			/* if we've reached the item that is selected, delete it */
			if(eof_fret_hand_position_list_dialog[1].d1 == ecount)
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
				if((eof_fret_hand_position_list_dialog[1].d1 >= ecount) && (ecount > 0))
				{	//If the last list item was deleted and others remain
					eof_fret_hand_position_list_dialog[1].d1--;	//Select the one before the one that was deleted, or the last event, whichever one remains
				}
				(void) dialog_message(eof_fret_hand_position_list_dialog, MSG_DRAW, 0, &junk);	//Redraw dialog
				return D_REDRAW;
			}

			/* go to next event */
			else
			{
				ecount++;
			}
		}
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
	(void) dialog_message(eof_fret_hand_position_list_dialog, MSG_DRAW, 0, &junk);	//Redraw dialog
	return D_REDRAW;
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
			if(eof_fret_hand_position_list_dialog[1].d1 == ecount)
			{
				eof_set_seek_position(eof_song->pro_guitar_track[tracknum]->handposition[i].start_pos + eof_av_delay);	//Seek to the hand position's timestamp
				eof_render();	//Redraw screen
				(void) dialog_message(eof_fret_hand_position_list_dialog, MSG_DRAW, 0, &junk);	//Redraw dialog
				return D_O_K;
			}

			/* go to next event */
			else
			{
				ecount++;
			}
		}
	}
	return D_O_K;
}

char eof_fret_hand_position_list_text[EOF_MAX_NOTES][25] = {{0}};

char * eof_fret_hand_position_list(int index, int * size)
{
	int i;
	int ecount = 0;
	unsigned long tracknum;
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
			*size = ecount;
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
			(void) snprintf(eof_fret_hand_position_list_dialog_title_string, sizeof(eof_fret_hand_position_list_dialog_title_string) - 1, "Fret hand positions (%d)", ecount);	//Redraw the dialog's title bar to reflect the number of hand positions
			break;
		}
		default:
		{
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
	unsigned long start, duration, i;
	unsigned long sel_start = 0, sel_end = 0;	//Will track the range of selected notes if any
	char failed = 0;

	if(!eof_song_loaded || !eof_song)
		return 1;	//Do not allow this function to run if a chart is not loaded or if an invalid catalog entry is selected

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
		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//For each note in the active track
			if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
			{
				if(!sel_end || (eof_get_note_pos(eof_song, eof_selected_track, i) < sel_start))
				{	//If this is the first selected note or it otherwise starts before the other selected notes
					sel_start = eof_get_note_pos(eof_song, eof_selected_track, i);
				}
				if(eof_get_note_pos(eof_song, eof_selected_track, i) + eof_get_note_length(eof_song, eof_selected_track, i) > sel_end)
				{
					sel_end = eof_get_note_pos(eof_song, eof_selected_track, i) + eof_get_note_length(eof_song, eof_selected_track, i);
				}
			}
		}
	}

	(void) ustrcpy(eof_etext, "");
	if(sel_start != sel_end)
	{	//If notes in this track difficulty are selected
		(void) snprintf(eof_etext2, sizeof(eof_etext2) - 1, "%lu", sel_start);				//Initialize the start time with the start of the selection
		(void) snprintf(eof_etext3, sizeof(eof_etext3) - 1, "%lu", sel_end - sel_start);	//Initialize the duration based on the end of the selection
	}
	else
	{
		(void) snprintf(eof_etext2, sizeof(eof_etext2) - 1, "%d", eof_music_pos - eof_av_delay);	//Otherwise initialize the start time with the current seek position
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
			for(i = 0; i < ustrlen(eof_etext); i ++)
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
	switch(index)
	{
		case -1:
		{
			*size = eof_song->pro_guitar_track[eof_song->track[eof_selected_track]->tracknum]->popupmessages;
			(void) snprintf(eof_rs_popup_messages_dialog_string, sizeof(eof_rs_popup_messages_dialog_string) - 1, "RS popup messages (%lu)", eof_song->pro_guitar_track[eof_song->track[eof_selected_track]->tracknum]->popupmessages);
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
	unsigned long tracknum, ctr, popupmessage;

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

int eof_song_qsort_popup_messages(const void * e1, const void * e2)
{
	EOF_PHRASE_SECTION * thing1 = (EOF_PHRASE_SECTION *)e1;
	EOF_PHRASE_SECTION * thing2 = (EOF_PHRASE_SECTION *)e2;

	//Sort by timestamp
	if(thing1->start_pos < thing2->start_pos)
	{
		return -1;
	}
	else if(thing1->start_pos > thing2->start_pos)
	{
		return 1;
	}

	//They are equal
	return 0;
}

void eof_track_pro_guitar_sort_popup_messages(EOF_PRO_GUITAR_TRACK* tp)
{
 	eof_log("eof_track_pro_guitar_sort_popup_messages() entered", 1);

	if(tp)
	{
		qsort(tp->popupmessage, (size_t)tp->popupmessages, sizeof(EOF_PHRASE_SECTION), eof_song_qsort_popup_messages);
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

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	tp = eof_song->pro_guitar_track[tracknum];
	if(tp->popupmessages == 0)
		return D_O_K;

	for(ctr = 0; ctr < tp->popupmessages; ctr++)
	{	//For each popup message
		if(ctr == eof_rs_popup_messages_dialog[1].d1)
		{	//If this is the popup message selected from the list
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
			eof_track_pro_guitar_delete_popup_message(eof_song->pro_guitar_track[tracknum], ctr);
			eof_track_pro_guitar_sort_popup_messages(eof_song->pro_guitar_track[tracknum]);	//Re-sort the remaining popup messages
			if((eof_fret_hand_position_list_dialog[1].d1 >= tp->popupmessages) && (tp->popupmessages > 0))
			{	//If the last list item was deleted and others remain
				eof_rs_popup_messages_dialog[1].d1--;	//Select the one before the one that was deleted, or the last message, whichever one remains
			}
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
	if(eof_rs_popup_messages_dialog[1].d1 >= tp->popupmessages)
		return D_O_K;	//Invalid popup message selected in list
	ptr = &(tp->popupmessage[eof_rs_popup_messages_dialog[1].d1]);
	(void) ustrcpy(eof_etext, ptr->name);
	(void) snprintf(eof_etext2, sizeof(eof_etext2) - 1, "%lu", ptr->start_pos);
	(void) snprintf(eof_etext3, sizeof(eof_etext3) - 1, "%lu", ptr->end_pos - ptr->start_pos);

	eof_clear_input();
	if(eof_popup_dialog(eof_song_rs_popup_add_dialog, 1) == 6)
	{	//User clicked OK
		start = atol(eof_etext2);
		duration = atol(eof_etext3);

		if(!duration)
		{	//If the given timing is not valid
			allegro_message("The popup message must have a duration");
		}
		else
		{
			for(i = 0; i < ustrlen(eof_etext); i ++)
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
				(void) ustrncpy(ptr->name, eof_etext, EOF_SECTION_NAME_LENGTH);	//Update the popup message string
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
	if(eof_rs_popup_messages_dialog[1].d1 >= tp->popupmessages)
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
	{"Ignore tuning", eof_track_proguitar_toggle_ignore_tuning, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_track_rocksmith_popup_menu[] =
{
	{"&Add", eof_track_rs_popup_add, NULL, 0, NULL},
	{"&List", eof_track_rs_popup_messages, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_track_rocksmith_menu[] =
{
	{"Fret &Hand positions", NULL, eof_song_proguitar_fret_hand_menu, 0, NULL},
	{"&Popup messages", NULL, eof_track_rocksmith_popup_menu, 0, NULL},
	{"&Arrangement type", NULL, eof_track_rocksmith_arrangement_menu, 0, NULL},
	{"Remove difficulty limit", eof_track_rocksmith_toggle_difficulty_limit, NULL, 0, NULL},
	{"Insert new difficulty", eof_track_rocksmith_insert_difficulty, NULL, 0, NULL},
	{"Delete active difficulty", eof_track_rocksmith_delete_difficulty, NULL, 0, NULL},
	{"&Manage RS phrases\t" CTRL_NAME "+Shift+M", eof_track_manage_rs_phrases, NULL, 0, NULL},
	{"Flatten this difficulty", eof_track_flatten_difficulties, NULL, 0, NULL},
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
	{	//If the active track already had the difficulty limit removed, toggle this flag off
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

	if(alert(NULL, "Insert the new difficulty above or below the active difficulty?", NULL, "&Above", "&Below", 'a', 'b') == 1)
	{	//If the user chooses to insert the difficulty above the active difficulty
		newdiff = eof_note_type + 1;	//The difficulty number of all content at/above the difficulty immediately above the active difficulty will be incremented
	}
	else
	{	//The user chose to insert the difficulty below the active difficulty
		newdiff = eof_note_type;
	}

	//Update note difficulties
	for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
	{	//For each note in the track
		thistype = eof_get_note_type(eof_song, eof_selected_track, ctr);	//Get this note's difficulty
		if(thistype >= newdiff)
		{	//If this note's difficulty needs to be updated
			eof_set_note_type(eof_song, eof_selected_track, ctr, thistype + 1);
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
		if(alert(NULL, "Would you like to copy the lower difficulty's contents?", NULL, "&Yes", "&No", 'y', 'n') == 1)
		{	//If user opted to copy the lower difficulty
			(void) eof_menu_edit_paste_from_difficulty(newdiff - 1, &undo_made);
		}
	}
	else if(!lower && upper)
	{	//If only the upper difficulty is populated, offer to copy it into the new difficulty
		if(alert(NULL, "Would you like to copy the upper difficulty's contents?", NULL, "&Yes", "&No", 'y', 'n') == 1)
		{	//If user opted to copy the upper difficulty
			(void) eof_menu_edit_paste_from_difficulty(newdiff + 1, &undo_made);
		}
	}
	else if(lower && upper)
	{	//If both the upper and lower difficulties are populated, prompt whether to copy either into the new difficulty
		if(alert(NULL, "Would you like to copy an adjacent difficulty's contents?", NULL, "&Yes", "&No", 'y', 'n') == 1)
		{	//If user opted to copy either the upper or lower difficulty
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

	eof_song->track[eof_selected_track]->flags |= EOF_TRACK_FLAG_UNLIMITED_DIFFS;	//Remove the difficulty limit for this track
	eof_pro_guitar_track_sort_fret_hand_positions(tp);
	eof_song->track[eof_selected_track]->numdiffs++;	//Increment the track's difficulty counter
	(void) eof_detect_difficulties(eof_song, eof_selected_track);
	eof_fix_window_title();	//Redraw the window title in case the active difficulty was incremented to compensate for inserting a difficulty below the active difficulty
	return 1;
}

int eof_track_rocksmith_delete_difficulty(void)
{
	unsigned long ctr, tracknum;
	unsigned char thistype;
	EOF_PRO_GUITAR_TRACK *tp;

	if(!eof_song || eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	if(eof_track_diff_populated_status[eof_note_type])
	{	//If the active track has any notes
		if(alert(NULL, "Warning:  This difficulty contains at least one note.  Delete the difficulty anyway?", NULL, "&Yes", "&No", 'y', 'n') != 1)
		{	//If the user doesn't opt to delete the populated track difficulty
			return 1;
		}
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	tp = eof_song->pro_guitar_track[tracknum];

	//Update note difficulties
	for(ctr = eof_get_track_size(eof_song, eof_selected_track); ctr > 0; ctr--)
	{	//For each note in the track (in reverse order)
		thistype = eof_get_note_type(eof_song, eof_selected_track, ctr - 1);	//Get this note's difficulty
		if(thistype == eof_note_type)
		{	//If this note needs to be deleted
			eof_track_delete_note(eof_song, eof_selected_track, ctr - 1);
		}
		else if(thistype >= eof_note_type)
		{	//If this note's difficulty needs to be updated
			eof_set_note_type(eof_song, eof_selected_track, ctr - 1, thistype - 1);
		}
	}

	//Update arpeggio difficulties
	for(ctr = tp->arpeggios; ctr > 0; ctr--)
	{	//For each arpeggio phrase in the track (in reverse order)
		if(tp->arpeggio[ctr - 1].difficulty == eof_note_type)
		{	//If this arpeggio phrase needs to be deleted
			eof_track_delete_arpeggio(eof_song, eof_selected_track, ctr - 1);
		}
		else if(tp->arpeggio[ctr - 1].difficulty >= eof_note_type)
		{	//If this arpeggio phrase's difficulty needs to be updated
			tp->arpeggio[ctr - 1].difficulty--;
		}
	}

	//Update fret hand positions
	for(ctr = 0; ctr < tp->handpositions; ctr++)
	{	//For each fret hand position
		if(tp->handposition[ctr - 1].difficulty == eof_note_type)
		{	//If this fret hand position needs to be deleted
			eof_pro_guitar_track_delete_hand_position(tp, ctr - 1);
		}
		else if(tp->handposition[ctr - 1].difficulty >= eof_note_type)
		{	//If this fret hand position's difficulty needs to be updated
			tp->handposition[ctr - 1].difficulty--;
		}
	}

	eof_pro_guitar_track_sort_fret_hand_positions(tp);
	if(eof_song->track[eof_selected_track]->numdiffs > 5)
	{	//If there are more than 5 difficulties in the active track
		eof_song->track[eof_selected_track]->numdiffs--;	//Decrement the track's difficulty counter
	}
	(void) eof_detect_difficulties(eof_song, eof_selected_track);
	(void) eof_menu_track_selected_track_number(eof_note_type - 1);
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

char * eof_magage_rs_phrases_list(int index, int * size)
{
	unsigned long ctr, numphrases;

	switch(index)
	{
		case -1:
		{
			for(ctr = 0, numphrases = 0; ctr < eof_song->beats; ctr++)
			{	//For each beat in the chart
				if(eof_song->beat[ctr]->contained_section_event >= 0)
				{	//If this beat has a section event (RS phrase)
					numphrases++;	//Update counter
				}
			}
			(void) snprintf(eof_track_manage_rs_phrases_dialog_string, sizeof(eof_track_manage_rs_phrases_dialog_string) - 1, "Manage RS phrases (%lu)", numphrases);
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
	{ d_agup_list_proc,  12,  84,  300, 144, 2,   23,  0,    0,      0,   0,   (void *)eof_magage_rs_phrases_list,NULL, NULL },
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

	//Count the number of phrases in the active track
	eof_process_beat_statistics(eof_song, eof_selected_track);	//Cache section name information into the beat structures (from the perspective of the active track)
	for(ctr = 0, numphrases = 0; ctr < eof_song->beats; ctr++)
	{	//For each beat in the chart
		if(eof_song->beat[ctr]->contained_section_event >= 0)
		{	//If this beat has a section event (RS phrase)
			numphrases++;	//Update counter
		}
	}

	eof_track_manage_rs_phrases_strings = malloc(sizeof(char *) * numphrases);	//Allocate enough pointers to have one for each phrase
	if(!eof_track_manage_rs_phrases_strings)
	{
		allegro_message("Error allocating memory");
		return;
	}
	for(ctr = 0, index = 0; ctr < eof_song->beats; ctr++)
	{	//For each beat in the chart
		if((eof_song->beat[ctr]->contained_section_event >= 0) || ((ctr + 1 >= eof_song->beats) && started))
		{	//If this beat has a section event (RS phrase) or a phrase is in progress and this is the last beat, it marks the end of any current phrase and the potential start of another
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

	for(ctr = 0, numphrases = 0; ctr < eof_song->beats; ctr++)
	{	//For each beat in the chart
		if(eof_song->beat[ctr]->contained_section_event >= 0)
		{	//If this beat has a section event (RS phrase)
			if(eof_track_manage_rs_phrases_dialog[1].d1 == numphrases)
			{	//If we've reached the item that is selected, seek to it
				eof_set_seek_position(eof_song->beat[ctr]->pos + eof_av_delay);	//Seek to the beat containing the phrase
				eof_render();	//Redraw screen
				(void) dialog_message(eof_track_manage_rs_phrases_dialog, MSG_DRAW, 0, &junk);	//Redraw dialog
				return D_O_K;
			}
			else
			{
				numphrases++;	//Update counter
			}
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
	unsigned long ctr, ctr2, ctr3, numphrases, targetbeat = 0, instancectr, tracknum;
	unsigned long startpos, endpos;		//Track the start and end position of the each instance of the phrase
	char *phrasename = NULL, undo_made = 0;
	EOF_PRO_GUITAR_TRACK *tp;
	EOF_PHRASE_SECTION *ptr;
	char prompt = 0;
	char started = 0;

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return D_O_K;

	//Identify the phrase that was selected
	for(ctr = 0, numphrases = 0; ctr < eof_song->beats; ctr++)
	{	//For each beat in the chart
		if(eof_song->beat[ctr]->contained_section_event >= 0)
		{	//If this beat has a section event (RS phrase)
			if(eof_track_manage_rs_phrases_dialog[1].d1 == numphrases)
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
	startpos = endpos = 0;	//Reset these to indicate that a phrase is being looked for
	for(; ctr < eof_song->beats; ctr++)
	{	//For each beat in the chart (starting from the one ctr is referring to)
		if((eof_song->beat[ctr]->contained_section_event >= 0) || ((ctr + 1 >= eof_song->beats) && started))
		{	//If this beat has a section event (RS phrase) or a phrase is in progress and this is the last beat, it marks the end of any current phrase and the potential start of another
			if(started)
			{	//If this beat marks the end of a phrase instance that needs to be modified
				started = 0;
				endpos = eof_song->beat[ctr]->pos - 1;	//Track this as the end position of the previous phrase marker

				//Increment the difficulty level of all notes and phrases that fall within the phrase that are in the active difficulty or higher
				//Parse in reverse order so that notes can be appended to or deleted from the track in the same loop
				for(ctr2 = eof_get_track_size(eof_song, eof_selected_track); ctr2 > 0; ctr2--)
				{	//For each note in the track (in reverse order)
					unsigned long notepos = eof_get_note_pos(eof_song, eof_selected_track, ctr2 - 1);
					unsigned char notetype = eof_get_note_type(eof_song, eof_selected_track, ctr2 - 1);
					if((notetype >= eof_note_type) && (notepos < startpos) && (notepos + 10 >= startpos))
					{	//If this note is 1 to 10 milliseconds before the beginning of the phrase
						if(!prompt)
						{	//If the user wasn't prompted about how to handle this condition yet, seek to the note in question and prompt the user whether to take action
							eof_set_seek_position(notepos + eof_av_delay);	//Seek to the note's position
							eof_render();
							if(alert("At least one note is between 1 and 10 ms before the phrase.", NULL, "Move such notes to the start of the phrase?", "&Yes", "&No", 'y', 'n') == 1)
							{	//If the user opts to correct the note positions
								prompt = 1;	//Store a "yes" response
							}
							else
							{
								prompt = 2;	//Store a "no" response
							}
						}
						if(prompt == 1)
						{	//If the user opted to correct the note positions
							if(!undo_made)
							{	//If an undo state hasn't been made yet
								eof_prepare_undo(EOF_UNDO_TYPE_NONE);
								undo_made = 1;
							}
							for(ctr3 = eof_get_track_size(eof_song, eof_selected_track); ctr3 > 0; ctr3--)
							{	//For each note in the track (in reverse order)
								unsigned long notepos2 = eof_get_note_pos(eof_song, eof_selected_track, ctr3 - 1);
								if((notepos2 < startpos) && (notepos2 + 10 >= startpos))
								{	//If this note is 1 to 10 milliseconds before the beginning of the phrase
									eof_set_note_pos(eof_song, eof_selected_track, ctr3 - 1, startpos);	//Re-position the note to the start of the phrase
								}
							}
							notepos = startpos;	//Update the stored position for the note
						}
					}
					if((notetype >= eof_note_type) && (notepos >= startpos) && (notepos <= endpos))
					{	//If the note meets the criteria to be altered
						if(!undo_made)
						{	//If an undo state hasn't been made yet
							eof_prepare_undo(EOF_UNDO_TYPE_NONE);
							undo_made = 1;
						}
						if(notetype == eof_note_type)
						{	//If this note is in the active difficulty
							if(function < 0)
							{	//If the delete level function is being performed, this note will be deleted
								eof_track_delete_note(eof_song, eof_selected_track, ctr2 - 1);
							}
							else
							{	//The add level function is being performed, this note will be duplicated into the next higher difficulty instead of just having its difficulty incremented
								long length = eof_get_note_length(eof_song, eof_selected_track, ctr2 - 1);
								(void) eof_copy_note(eof_song, eof_selected_track, ctr2 - 1, eof_selected_track, notepos, length, eof_note_type + 1);
							}
						}
						else
						{
							if(function < 0)
							{	//If the delete level function is being performed, this note will have its difficulty decremented
								eof_set_note_type(eof_song, eof_selected_track, ctr2 - 1, notetype - 1);	//Decrement the note's difficulty
							}
							else
							{	//The add level function is being performed, this note will have its difficulty incremented
								eof_set_note_type(eof_song, eof_selected_track, ctr2 - 1, notetype + 1);	//Increment the note's difficulty
							}
						}
					}
				}//For each note in the track (in reverse order)

				//Update arpeggios
				tracknum = eof_song->track[eof_selected_track]->tracknum;
				tp = eof_song->pro_guitar_track[tracknum];
				for(ctr2 = eof_get_num_arpeggios(eof_song, eof_selected_track); ctr2 > 0; ctr2--)
				{	//For each arpeggio section in the track (in reverse order)
					ptr = &eof_song->pro_guitar_track[tracknum]->arpeggio[ctr2 - 1];	//Simplify
					if((ptr->difficulty >= eof_note_type) && (ptr->start_pos <= endpos) && (ptr->end_pos >= startpos))
					{	//If this arpeggio overlaps at all with the phrase being manipulated and is in a difficulty level that this function affects
						if(ptr->difficulty == eof_note_type)
						{	//If this arpeggio is in the active difficulty
							if(function < 0)
							{	//If the delete level function is being performed, this arpeggio will be deleted
								eof_track_delete_arpeggio(eof_song, eof_selected_track, ctr2 - 1);
							}
							else
							{	//The add level function is being performed, this arpeggio will be duplicated into the next higher difficulty instead of just having its difficulty incremented
								(void) eof_track_add_section(eof_song, eof_selected_track, EOF_ARPEGGIO_SECTION, eof_note_type + 1, ptr->start_pos, ptr->end_pos, 0, NULL);
							}
						}
						else
						{
							if(function < 0)
							{	//If the delete level function is being performed, this arpeggio will have its difficulty decremented
								ptr->difficulty--;	//Decrement the arpeggio's difficulty
							}
							else
							{	//The add level function is being performed, this arpeggio will have its difficulty incremented
								ptr->difficulty++;	//Increment the arpeggio's difficulty
							}
						}
					}
				}
				//Update fret hand positions
				for(ctr2 = tp->handpositions; ctr2 > 0; ctr2--)
				{	//For each fret hand position in the track (in reverse order)
					ptr = &tp->handposition[ctr2 - 1];	//Simplify
					if((ptr->difficulty >= eof_note_type) && (ptr->start_pos >= startpos) && (ptr->start_pos <= endpos))
					{	//If this fret hand position is within the phrase being manipulated and is in a difficulty level that this function affects
						if(ptr->difficulty == eof_note_type)
						{	//If this fret hand position is in the active difficulty, it will be duplicated into the next higher difficulty instead of just having its difficulty incremented
							if(function < 0)
							{	//If the delete level function is being performed, this fret hand position will be deleted
								eof_pro_guitar_track_delete_hand_position(tp, ctr2 - 1);
							}
							else
							{	//The add level function is being performed, this fret hand position will be duplicated into the next higher difficulty instead of just having its difficulty incremented
								(void) eof_track_add_section(eof_song, eof_selected_track, EOF_FRET_HAND_POS_SECTION, eof_note_type + 1, ptr->start_pos, ptr->end_pos, 0, NULL);
							}
						}
						else
						{
							if(function < 0)
							{	//If the delete level function is being performed, this fret hand position will have its difficulty decremented
								ptr->difficulty--;	//Decrement the fret hand position's difficulty
							}
							else
							{	//The add level function is being performed, this fret hand position will have its difficulty incremented
								ptr->difficulty++;	//Increment the fret hand position's difficulty
							}
						}
					}
				}
				//Update tremolos
				for(ctr2 = eof_get_num_tremolos(eof_song, eof_selected_track); ctr2 > 0; ctr2--)
				{	//For each tremolo section in the track (in reverse order)
					ptr = &eof_song->pro_guitar_track[tracknum]->tremolo[ctr2 - 1];	//Simplify
					if((ptr->difficulty >= eof_note_type) && (ptr->start_pos <= endpos) && (ptr->end_pos >= startpos))
					{	//If this tremolo overlaps at all with the phrase being manipulated and is in a difficulty level that this function affects
						if(ptr->difficulty == 0xFF)
						{	//If this tremolo is not difficulty specific
							continue;	//Don't manipulate it in this function
						}
						if(ptr->difficulty == eof_note_type)
						{	//If this tremolo is in the active difficulty
							if(function < 0)
							{	//If the delete level function is being performed, this tremolo will be deleted
								eof_track_delete_tremolo(eof_song, eof_selected_track, ctr2 - 1);
							}
							else
							{	//The add level function is being performed, this tremolo will be duplicated into the next higher difficulty instead of just having its difficulty incremented
								(void) eof_track_add_section(eof_song, eof_selected_track, EOF_TREMOLO_SECTION, eof_note_type + 1, ptr->start_pos, ptr->end_pos, 0, NULL);
							}
						}
						else
						{
							if(function < 0)
							{	//If the delete level function is being performed, this tremolo will have its difficulty decremented
								ptr->difficulty--;	//Decrement the tremolo's difficulty
							}
							else
							{	//The add level function is being performed, this tremolo will have its difficulty incremented
								ptr->difficulty++;	//Increment the tremolo's difficulty
							}
						}
					}
				}

				if(!instancectr)
				{	//If only the selected phrase instance was to be modified
					break;	//Exit loop
				}
				startpos = endpos = 0;	//Reset these to indicate that a phrase is being looked for
			}

			if(eof_song->beat[ctr]->contained_section_event >= 0)
			{	//If this beat has a phrase assigned to it
				if(!ustrcmp(eof_song->text_event[eof_song->beat[ctr]->contained_section_event]->text, phrasename))
				{	//If the phrase matched the one that was selected to be leveled up
					started = 1;
					startpos = eof_song->beat[ctr]->pos;	//Track the starting position of the phrase
				}
			}
		}//If this beat has a section event (RS phrase) or a phrase is in progress and this is the last beat, it marks the end of any current phrase and the potential start of another
	}//For each beat in the chart (starting from the one ctr is referring to)

	if(!undo_made)
	{	//If no notes were within the selected phrase instance(s)
		allegro_message("The selected phrase instance(s) had no notes at or above the current difficulty, so no alterations were performed.");
	}
	else
	{	//Otherwise remove the difficulty limit since this operation has modified the chart
		eof_track_sort_notes(eof_song, eof_selected_track);
		eof_pro_guitar_track_sort_fret_hand_positions(eof_song->pro_guitar_track[tracknum]);	//Sort the positions, since they must be in order for displaying to the user
		eof_song->track[eof_selected_track]->flags |= EOF_TRACK_FLAG_UNLIMITED_DIFFS;	//Remove the difficulty limit for this track
		eof_song->track[eof_selected_track]->numdiffs = eof_detect_difficulties(eof_song, eof_selected_track);	//Update the number of difficulties used in this track
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
