#include <allegro.h>
#include "../main.h"
#include "edit.h"
#include "note.h"
#include "context.h"
#include "song.h"

#ifdef USEMEMWATCH
#include "../memwatch.h"
#endif

MENU eof_right_click_menu_normal[] =
{
	{"&Copy\t" CTRL_NAME "+C", eof_menu_edit_copy, NULL, 0, NULL},
	{"&Paste\t" CTRL_NAME "+V", eof_menu_edit_paste, NULL, 0, NULL},
	{"&Grid Snap", NULL, eof_edit_snap_menu, 0, NULL},
	{"&Zoom", NULL, eof_edit_zoom_menu, 0, NULL},
	{"&Full screen 3D view", eof_enable_full_screen_3d, NULL, 0, NULL},
	{"Second piano &Roll", NULL, eof_song_piano_roll_menu, 0, NULL},
	{"", NULL, NULL, 0, NULL},
	{"&Selection", NULL, eof_edit_selection_menu, 0, NULL},
	{"Paste at mouse\tShift+Ins", eof_menu_edit_paste_at_mouse, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_right_click_menu_note[] =
{
	{"&Note", NULL, eof_note_menu, 0, NULL},
	{"", NULL, NULL, 0, NULL},
	{"&Copy\t" CTRL_NAME "+C", eof_menu_edit_copy, NULL, 0, NULL},
	{"&Paste\t" CTRL_NAME "+V", eof_menu_edit_paste, NULL, 0, NULL},
	{"&Grid Snap", NULL, eof_edit_snap_menu, 0, NULL},
	{"&Zoom", NULL, eof_edit_zoom_menu, 0, NULL},
	{"&Full screen 3D view", eof_enable_full_screen_3d, NULL, 0, NULL},
	{"", NULL, NULL, 0, NULL},
	{"&Selection", NULL, eof_edit_selection_menu, 0, NULL},
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
	{
		vselected = eof_count_selected_and_unselected_notes(NULL);
		if(vselected)
		{	//If at least one note is selected, enable the Copy function
			eof_right_click_menu_normal[0].flags = 0;
			eof_right_click_menu_note[2].flags = 0;
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
		if(eof_track_diff_populated_status[eof_note_type])
		{	//If the active instrument difficulty is populated, enable the Selection submenu
			eof_right_click_menu_normal[7].flags = 0;
			eof_right_click_menu_note[8].flags = 0;
		}
		else
		{
			eof_right_click_menu_normal[7].flags = D_DISABLED;
			eof_right_click_menu_note[7].flags = D_DISABLED;
		}
	}
}
