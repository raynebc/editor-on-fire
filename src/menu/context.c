#include <allegro.h>
#include "../main.h"
#include "context.h"
#include "edit.h"
#include "note.h"
#include "song.h"
#include "track.h"

#ifdef USEMEMWATCH
#include "../memwatch.h"
#endif

MENU eof_right_click_menu_normal[] =
{
	{"&Copy\t" CTRL_NAME "+C", eof_menu_edit_copy, NULL, 0, NULL},
	{"Paste\t" CTRL_NAME "+V", eof_menu_edit_paste, NULL, 0, NULL},
	{"&Grid Snap", NULL, eof_edit_snap_menu, 0, NULL},
	{"&Zoom", NULL, eof_edit_zoom_menu, 0, NULL},
	{"Full screen &3D view", eof_enable_full_screen_3d, NULL, 0, NULL},
	{"Second piano &Roll", NULL, eof_song_piano_roll_menu, 0, NULL},
	{"", NULL, NULL, 0, NULL},
	{"&Paste at mouse\tShift+Ins", eof_menu_edit_paste_at_mouse, NULL, 0, NULL},
	{"Place fl&Oating event at mouse", eof_menu_song_add_floating_text_event_at_mouse, NULL, 0, NULL},
	{"Set &Start point at mouse", eof_menu_edit_set_start_point_at_mouse, NULL, 0, NULL},
	{"Set &End point at mouse", eof_menu_edit_set_end_point_at_mouse, NULL, 0, NULL},
	{"Set &Fret hand position at mouse", eof_track_pro_guitar_set_fret_hand_position_at_mouse, NULL, 0, NULL},
	{"Place &Tone change at mouse", eof_track_rs_tone_change_add_at_mouse, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_right_click_menu_note[] =
{
	{"&Note", NULL, eof_note_menu, 0, NULL},
	{"", NULL, NULL, 0, NULL},
	{"&Copy\t" CTRL_NAME "+C", eof_menu_edit_copy, NULL, 0, NULL},
	{"Paste\t" CTRL_NAME "+V", eof_menu_edit_paste, NULL, 0, NULL},
	{"&Grid Snap", NULL, eof_edit_snap_menu, 0, NULL},
	{"&Zoom", NULL, eof_edit_zoom_menu, 0, NULL},
	{"Full screen &3D view", eof_enable_full_screen_3d, NULL, 0, NULL},
	{"", NULL, NULL, 0, NULL},
	{"&Paste at mouse\tShift+Ins", eof_menu_edit_paste_at_mouse, NULL, 0, NULL},
	{"Place fl&Oating event at mouse", eof_menu_song_add_floating_text_event_at_mouse, NULL, 0, NULL},
	{"Set &Start point at mouse", eof_menu_edit_set_start_point_at_mouse, NULL, 0, NULL},
	{"Set &End point at mouse", eof_menu_edit_set_end_point_at_mouse, NULL, 0, NULL},
	{"Set &Fret hand position at mouse", eof_track_pro_guitar_set_fret_hand_position_at_mouse, NULL, 0, NULL},
	{"Place &Tone change at mouse", eof_track_rs_tone_change_add_at_mouse, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_right_click_menu_full_screen_3d_view[] =
{
	{"Exit full screen 3D preview", eof_disable_full_screen_3d, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

int eof_disable_full_screen_3d(void)
{
	eof_full_screen_3d = 0;
	return 0;
}

int eof_enable_full_screen_3d(void)
{
	eof_full_screen_3d = 1;
	return 0;
}

void eof_prepare_context_menu(void)
{
	int vselected = 0;
	char clipboard_path[50];

	if(eof_song && eof_song_loaded)
	{	//If a chart is loaded
		vselected = eof_count_selected_and_unselected_notes(NULL);
		if(vselected)
		{	//If at least one note is selected, enable the Copy function
			eof_right_click_menu_normal[0].flags = 0;	//Copy
			eof_right_click_menu_note[2].flags = 0;		//Copy
		}
		else
		{
			eof_right_click_menu_normal[0].flags = D_DISABLED;
			eof_right_click_menu_note[2].flags = D_DISABLED;
		}
		if(eof_vocals_selected)
		{
			(void) snprintf(clipboard_path, sizeof(clipboard_path) - 1, "%seof.vocals.clipboard", eof_temp_path_s);
		}
		else
		{
			(void) snprintf(clipboard_path, sizeof(clipboard_path) - 1, "%seof.clipboard", eof_temp_path_s);
		}
		if(exists(clipboard_path))
		{	//If there is a note clipboard file, enable the Paste function
			eof_right_click_menu_normal[1].flags = 0;	//Paste
			eof_right_click_menu_note[3].flags = 0;		//Paste
		}
		else
		{
			eof_right_click_menu_normal[1].flags = D_DISABLED;
			eof_right_click_menu_note[3].flags = D_DISABLED;
		}
		if(eof_pen_note.pos < eof_chart_length)
		{	//If the pen note is at a valid position
			eof_right_click_menu_normal[7].flags = 0;	//Paste at mouse
			eof_right_click_menu_note[8].flags = 0;		//Paste at mouse
			eof_right_click_menu_normal[8].flags = 0;	//Place floating event
			eof_right_click_menu_note[9].flags = 0;		//Place floating event
			eof_right_click_menu_normal[11].flags = 0;	//Set fret hand position
			eof_right_click_menu_note[12].flags = 0;	//Set fret hand position
			eof_right_click_menu_normal[12].flags = 0;	//Place tone change
			eof_right_click_menu_note[13].flags = 0;	//Place tone change
		}
		else
		{
			eof_right_click_menu_normal[7].flags = D_DISABLED;
			eof_right_click_menu_note[8].flags = D_DISABLED;
			eof_right_click_menu_normal[8].flags = D_DISABLED;
			eof_right_click_menu_note[9].flags = D_DISABLED;
			eof_right_click_menu_normal[11].flags = D_DISABLED;
			eof_right_click_menu_note[12].flags = D_DISABLED;
			eof_right_click_menu_normal[12].flags = D_DISABLED;
			eof_right_click_menu_note[13].flags = D_DISABLED;
		}
		if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If a pro guitar track is not active, disable the fret hand position and tone change context menu items
			eof_right_click_menu_normal[11].flags = D_DISABLED;
			eof_right_click_menu_note[12].flags = D_DISABLED;
			eof_right_click_menu_normal[12].flags = D_DISABLED;
			eof_right_click_menu_note[13].flags = D_DISABLED;
		}
	}//If a chart is loaded
}
