#include <allegro.h>
#include "../agup/agup.h"
#include "../foflc/Lyric_storage.h"
#include "../lc_import.h"
#include "../chart_import.h"
#include "../main.h"
#include "../player.h"
#include "../dialog.h"
#include "../undo.h"
#include "../utility.h"
#include "../midi.h"
#include "../ini.h"
#include "../feedback.h"
#include "file.h"
#include "song.h"

MENU eof_file_menu[] =
{
    {"&New\tF4", eof_menu_file_new_wizard, NULL, 0, NULL},
    {"&Load\tF3", eof_menu_file_load, NULL, 0, NULL},
    {"&Save\tF2", eof_menu_file_save, NULL, D_DISABLED, NULL},
    {"Save &As", eof_menu_file_save_as, NULL, D_DISABLED, NULL},
    {"", NULL, NULL, 0, NULL},
    {"Load &OGG", eof_menu_file_load_ogg, NULL, D_DISABLED, NULL},
    {"", NULL, NULL, 0, NULL},
    {"MIDI Import", eof_menu_file_midi_import, NULL, 0, NULL},
    {"Import Lyrics\tF8", eof_menu_file_lyrics_import, NULL, 0, NULL},
    {"", NULL, NULL, 0, NULL},
    {"Settings\tF10", eof_menu_file_settings, NULL, 0, NULL},
    {"Preferences\tF11", eof_menu_file_preferences, NULL, 0, NULL},
    {"Display", eof_menu_file_display, NULL, 0, NULL},
    {"Controllers", eof_menu_file_controllers, NULL, 0, NULL},
    {"", NULL, NULL, 0, NULL},
    {"Song Folder", eof_menu_file_song_folder, NULL, 0, NULL},
    {"Link to FOF", eof_menu_file_link, NULL, EOF_LINUX_DISABLE, NULL},
    {"", NULL, NULL, 0, NULL},
    {"E&xit\tEsc", eof_menu_file_exit, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

DIALOG eof_settings_dialog[] =
{
   /* (proc)         (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)           (dp2) (dp3) */
   { d_agup_window_proc,    0,  48,  200, 140 + 64 + 16, 2,   23,  0,    0,      0,   0,   "Settings",               NULL, NULL },
   { d_agup_text_proc,   16,  84,  64,  8,  2,   23,  0,    0,      0,   0,   "AV Delay (ms):",         NULL, NULL },
   { d_agup_edit_proc,   128, 80,  64,  20,  2,   23,  0,    0,      5,   0,   eof_etext,           NULL, NULL },
   { d_agup_text_proc,   16,  108,  64,  8,  2,   23,  0,    0,      0,   0,   "Buffer Size:",         NULL, NULL },
   { d_agup_edit_proc,   128, 104,  64,  20,  2,   23,  0,    0,      5,   0,   eof_etext2,           NULL, NULL },
   { d_agup_text_proc,   16,  132,  64,  8,  2,   23,  0,    0,      0,   0,   "CPU Saver",         NULL, NULL },
   { d_agup_slider_proc, 96, 132,  96,  16,  2,   23,  0,    0,      10,   0,   NULL,           NULL, NULL },
   { d_agup_check_proc, 16,  160, 160,  16, 2,   23,  0,    0, 1,   0,   "Smooth Playback",               NULL, NULL },
   { d_agup_check_proc, 16,  180, 160,  16, 2,   23,  0,    0, 1,   0,   "Disable Windows UI",               NULL, NULL },
   { d_agup_check_proc, 16,  200, 160,  16, 2,   23,  0,    0, 1,   0,   "Disable VSync",               NULL, NULL },

   { d_agup_button_proc, 16,  156 + 64 + 8, 68,  28, 2,   23,  '\r',    D_EXIT, 0,   0,   "OK",               NULL, NULL },
   { d_agup_button_proc, 116, 156 + 64 + 8, 68,  28, 2,   23,  0,    D_EXIT, 0,   0,   "Cancel",           NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_preferences_dialog[] =
{
   /* (proc)         (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)           (dp2) (dp3) */
   { d_agup_window_proc,    0,  48,  200, 140 + 64 + 32 + 20, 2,   23,  0,    0,      0,   0,   "Preferences",               NULL, NULL },
   { d_agup_check_proc, 16,  80, 128,  16, 2,   23,  0,    0, 1,   0,   "Inverted Notes",               NULL, NULL },
   { d_agup_check_proc, 16,  100, 128,  16, 2,   23,  0,    0, 1,   0,   "Lefty Mode",               NULL, NULL },
   { d_agup_check_proc, 16,  120, 128,  16, 2,   23,  0,    0, 1,   0,   "Note Auto-Adjust",               NULL, NULL },

   { d_agup_text_proc,   56, 104 + 40,  48,  8,   2,   23,  0,    0,      0,   0,   "Input Method",            NULL, NULL },
   { d_agup_list_proc,   43, 104 + 16 + 40,  110,  94,  2,   23,  0,    0,      0,   0,   eof_input_list, NULL, NULL },
   { d_agup_button_proc, 16,  58 + 16 + 64 + 22 + 16 + 24 + 24 + 40, 68,  28, 2,   23,  '\r',    D_EXIT, 0,   0,   "OK",               NULL, NULL },
   { d_agup_button_proc, 116, 58 + 16 + 64 + 22 + 16 + 24 + 24 + 40, 68,  28, 2,   23,  0,    D_EXIT, 0,   0,   "Cancel",           NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_display_dialog[] =
{
   /* (proc)         (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)           (dp2) (dp3) */
   { d_agup_window_proc,    0,  48,  200, 196, 2,   23,  0,    0,      0,   0,   "Display Settings",               NULL, NULL },

   { d_agup_check_proc, 16,  80, 128,  16, 2,   23,  0,    0, 1,   0,   "Use Software Cursor",               NULL, NULL },
   { d_agup_check_proc, 16,  100, 128,  16, 2,   23,  0,    0, 1,   0,   "Force 8-Bit Color",               NULL, NULL },
   { d_agup_text_proc,   56, 124,  48,  8,   2,   23,  0,    0,      0,   0,   "Window Size",            NULL, NULL },
   { d_agup_list_proc,   43, 140,  110,  94 - 45,  2,   23,  0,    0,      0,   0,   eof_display_list, NULL, NULL },
   { d_agup_button_proc, 12,  202, 174,  28, 2,   23,  '\r',    D_EXIT, 0,   0,   "OK",               NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_guitar_settings_dialog[] =
{
   /* (proc)         (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)           (dp2) (dp3) */
   { d_agup_window_proc,    4,  236 + 24 - 32 - 20 - 8,  192 + 32 + 96 + 16, 190 + 78, 2,   23,  0,    0,      0,   0,   "Guitar Settings",               NULL, NULL },

   { d_agup_text_proc,    16, 240,  64,  8,  2,   23,  0,    0,      0,   0,   "Delay (ms):",         NULL, NULL },
   { d_agup_edit_proc,   104, 236,  64,  20,  2,   23,  0,    0,      5,   0,   eof_etext,           NULL, NULL },
   { d_agup_list_proc,   13, 266,  170 + 32 + 85 + 28,  96 + 13,  2,   23,  0,    0,      0,   0,   eof_guitar_list, NULL, NULL },
   { d_agup_push_proc, 13,  176 + 64 + 22 + 16 + 24 + 24 + 24 + 36, 170 + 32 + 85 + 28,  28, 2,   23,  0,    D_EXIT, 0,   0,   "Redefine",               NULL, eof_guitar_controller_redefine },
   { d_agup_button_proc, 13, 176 + 64 + 22 + 16 + 24 + 24 + 24 + 36 + 32 + 8, 170 + 32 + 85 + 28,  28, 2,   23,  0,    D_EXIT, 0,   0,   "OK",               NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_drum_settings_dialog[] =
{
   /* (proc)         (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)           (dp2) (dp3) */
   { d_agup_window_proc,    4,  236 + 24 - 32 - 20 - 8,  192 + 32 + 96 + 16, 190 + 78 - 30, 2,   23,  0,    0,      0,   0,   "Drum Settings",               NULL, NULL },

   { d_agup_text_proc,    16, 240,  64,  8,  2,   23,  0,    0,      0,   0,   "Delay (ms):",         NULL, NULL },
   { d_agup_edit_proc,   104, 236,  64,  20,  2,   23,  0,    0,      5,   0,   eof_etext,           NULL, NULL },
   { d_agup_list_proc,   13, 266,  170 + 32 + 85 + 28,  79,  2,   23,  0,    0,      0,   0,   eof_drum_list, NULL, NULL },
   { d_agup_push_proc, 13,  176 + 64 + 22 + 16 + 24 + 24 + 24 + 36 - 30, 170 + 32 + 85 + 28,  28, 2,   23,  0,    D_EXIT, 0,   0,   "Redefine",               NULL, eof_drum_controller_redefine },
   { d_agup_button_proc, 13, 176 + 64 + 22 + 16 + 24 + 24 + 24 + 36 + 32 + 8 - 30, 170 + 32 + 85 + 28,  28, 2,   23,  0,    D_EXIT, 0,   0,   "OK",               NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_controller_settings_dialog[] =
{
   /* (proc)         (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)           (dp2) (dp3) */
   { d_agup_window_proc,    4,  236 + 24 - 32 - 20 - 8, 190 + 32, 144, 2,   23,  0,    0,      0,   0,   "Controller Settings",               NULL, NULL },

   { d_agup_push_proc, 13,  176 + 6 + 24 + 24 + 36 - 30, 170 + 32,  28, 2,   23,  0,    D_EXIT, 0,   0,   "Guitar",               NULL, eof_controller_settings_guitar },
   { d_agup_push_proc, 13,  176 + 6 + 24 + 24 + 36, 170 + 32,  28, 2,   23,  0,    D_EXIT, 0,   0,   "Drums",               NULL, eof_controller_settings_drums },
   { d_agup_button_proc, 13, 176 + 88 + 32 + 8, 170 + 32,  28, 2,   23,  0,    D_EXIT, 0,   0,   "Done",               NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_file_new_dialog[] =
{
   /* (proc)         (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)           (dp2) (dp3) */
   { d_agup_window_proc,    0,  48,  320, 112 + 8, 2,   23,  0,    0,      0,   0,   "New Song Information",               NULL, NULL },
   { d_agup_text_proc,   112,  56,  128, 8,  2,   23,  0,    0,      0,   0,   "", NULL, NULL },
   { d_agup_text_proc,   16,  84,  64,  8,  2,   23,  0,    0,      0,   0,   "Artist:",         NULL, NULL },
   { d_agup_edit_proc,   80, 80,  224,  20,  2,   23,  0,    0,      255,   0,   eof_etext,           NULL, NULL },
   { d_agup_text_proc,   16,  108,  64,  8,  2,   23,  0,    0,      0,   0,   "Title:",         NULL, NULL },
   { d_agup_edit_proc,   80, 104,  224,  20,  2,   23,  0,    0,      255,   0,   eof_etext2,           NULL, NULL },
   { d_agup_button_proc, 80,  132, 68,  28, 2,   23,  '\r',    D_EXIT, 0,   0,   "OK",               NULL, NULL },
   { d_agup_button_proc, 160, 132, 68,  28, 2,   23,  0,    D_EXIT, 0,   0,   "Cancel",           NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_file_new_windows_dialog[] =
{
   /* (proc)         (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)           (dp2) (dp3) */
   { d_agup_window_proc,    0,  48,  320, 112 + 32 + 24, 2,   23,  0,    0,      0,   0,   "Location for New Song",               NULL, NULL },
   { d_agup_radio_proc,   16,  84,  256,  16,  2,   23,  0,    0,      0,   0,   "Use Existing Folder",         NULL, NULL },
   { d_agup_radio_proc,   16,  108,  256,  16,  2,   23,  0,    0,      0,   0,   "Use Source Audio's Folder",         NULL, NULL },
   { d_agup_radio_proc,   16,  132,  256,  16,  2,   23,  0,    0,      0,   0,   "Create New Folder",         NULL, NULL },
   { d_agup_edit_proc,   34, 150,  252,  20,  2,   23,  0,    0,      255,   0,   eof_etext4,           NULL, NULL },
   { d_agup_button_proc, 80,  180, 68,  28, 2,   23,  '\r',    D_EXIT, 0,   0,   "OK",               NULL, NULL },
   { d_agup_button_proc, 160, 180, 68,  28, 2,   23,  0,    D_EXIT, 0,   0,   "Cancel",           NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_ogg_settings_dialog[] =
{
   /* (proc)         (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)           (dp2) (dp3) */
   { d_agup_shadow_box_proc,    4,  236 + 24 - 32 - 20 - 8,  192, 190, 2,   23,  0,    0,      0,   0,   NULL,               NULL, NULL },
   { d_agup_text_proc,   58,  236 + 24 - 32 - 20,  128, 8,  2,   23,  0,    0,      0,   0,   "OGG Settings", NULL, NULL },
   { d_agup_text_proc,   49, 236 + 24 - 32,  48,  8,   2,   23,  0,    0,      0,   0,   "Encoder Quality",            NULL, NULL },
   { d_agup_list_proc,   43, 252 + 24 - 32,  110,  96,  2,   23,  0,    0,      0,   0,   eof_ogg_list, NULL, NULL },
   { d_agup_button_proc, 16,  176 + 64 + 22 + 16 + 24 + 24 + 24, 68,  28, 2,   23,  '\r',    D_EXIT, 0,   0,   "OK",               NULL, NULL },
   { d_agup_button_proc, 116, 176 + 64 + 22 + 16 + 24 + 24 + 24, 68,  28, 2,   23,  0,    D_EXIT, 0,   0,   "Cancel",           NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_lyric_detections_dialog[]=
{
	/*(proc)				(x)			(y)			(w)			(h)			(fg)	(bg)	(key)	(flags)	(d1)(d2)	(dp)							(dp2)(dp3)*/
	{d_agup_window_proc,	0,			48,			216+110+20,	160+72+2,	2,		23,		0,		0,		0,	0,		"Select track to import",		NULL,NULL},
	{d_agup_list_proc,		12,			84,			110*2+20+80,69*2+2,		2,		23,		0,		0,		0,	0,		eof_lyric_detections_list_all,	NULL,NULL},
	{d_agup_button_proc,	12,			166+69+2,	160-6,		28,			2,		23,		0,		D_EXIT,	0,	0,		"Import",						NULL,NULL},
	{d_agup_button_proc,	12+160+6,	166+69+2,	160-6,		28,			2,		23,		0,		D_EXIT,	0,	0,		"Cancel",						NULL,NULL},
	{NULL,					0,			0,			0,			0,			0,		0,		0,		0,		0,	0,		NULL,							NULL,NULL}
};

struct Lyric_Format *lyricdetectionlist;	//Dialog windows cannot be passed local variables, requiring the use of this global variable for the lyric track prompt dialog
char lyricdetectionstring[1024];			//The display name given to the detection when displayed in the list box
static int redefine_index = -1;

void eof_prepare_file_menu(void)
{
	if(eof_song && eof_song_loaded)
	{
		eof_file_menu[2].flags = 0; // Save
		eof_file_menu[3].flags = 0; // Save As
		eof_file_menu[5].flags = 0; // Load OGG
		eof_file_menu[8].flags = 0; // Import Lyrics
	}
	else
	{
		eof_file_menu[2].flags = D_DISABLED; // Save
		eof_file_menu[3].flags = D_DISABLED; // Save As
		eof_file_menu[5].flags = D_DISABLED; // Load OGG
		eof_file_menu[8].flags = D_DISABLED; // Import Lyrics
	}
}

int eof_menu_file_new_supplement(int option)
{
	char syscommand[1024] = {0};
	int err;

	ustrcpy(syscommand, eof_etext3);

	/* remove slash from folder name so we can check if it exists */
	if(syscommand[uoffset(syscommand, ustrlen(syscommand) - 1)] == '\\' || syscommand[uoffset(syscommand, ustrlen(syscommand) - 1)] == '/')
	{
		syscommand[uoffset(syscommand, ustrlen(syscommand) - 1)] = '\0';
	}
	if(!file_exists(syscommand, FA_DIREC | FA_HIDDEN, NULL))
	{
		err = eof_mkdir(syscommand);
		if(err)
		{
			eof_render();
			allegro_message("Could not create folder!\n%s\nEnsure that the specified folder name is valid and\nthe Song Folder is configured to a non write-restricted area.\n(File->Song Folder)", syscommand);
			return 0;
		}
	}
	else
	{
		eof_render();
		if(!option)
		{
			err = 0;
			put_backslash(eof_etext3);
			replace_filename(eof_temp_filename, eof_etext3, "notes.eof", 1024);
			if(exists(eof_temp_filename))
			{
				err = 1;
			}
			replace_filename(eof_temp_filename, eof_etext3, "guitar.ogg", 1024);
			if(exists(eof_temp_filename))
			{
				err = 1;
			}
			replace_filename(eof_temp_filename, eof_etext3, "song.ini", 1024);
			if(exists(eof_temp_filename))
			{
				err = 1;
			}
			replace_filename(eof_temp_filename, eof_etext3, "notes.mid", 1024);
			if(exists(eof_temp_filename))
			{
				err = 1;
			}
			if(err)
			{
				if(alert(NULL, "Some files may be overwritten. Proceed?", NULL, "&Yes", "&No", 'y', 'n') == 2)
				{
					return 0;
				}
			}
		}
		else
		{
			err = 0;
			put_backslash(eof_etext3);
			replace_filename(eof_temp_filename, eof_etext3, "notes.eof", 1024);
			if(exists(eof_temp_filename))
			{
				err = 1;
			}
			replace_filename(eof_temp_filename, eof_etext3, "song.ini", 1024);
			if(exists(eof_temp_filename))
			{
				err = 1;
			}
			replace_filename(eof_temp_filename, eof_etext3, "notes.mid", 1024);
			if(exists(eof_temp_filename))
			{
				err = 1;
			}
			if(err)
			{
				if(alert(NULL, "Some files may be overwritten. Proceed?", NULL, "&Yes", "&No", 'y', 'n') == 2)
				{
					return 0;
				}
			}
		}
	}
	return 1;
}

int eof_menu_file_new_wizard(void)
{
	char syscommand[1024] = {0};
	char oggfilename[1024] = {0};
	char * returnedfn = NULL;
	char * returnedfolder = NULL;
	int newtype = 0;
	int ret = 0;
	ALOGG_OGG * temp_ogg = NULL;
	char * temp_buffer = NULL;
	int temp_buffer_size = 0;

	if(eof_changes)
	{
		ret = alert3(NULL, "You have unsaved changes.", NULL, "Save", "Discard", "Cancel", 0, 0, 0);
		if(ret == 1)
		{
			eof_menu_file_save();
		}
		eof_clear_input();
	}
	if(ret == 3)
	{
		return 1;
	}

	/* stop the music and get ready */
	eof_cursor_visible = 0;
	if(eof_song_loaded)
	{
		if(!eof_music_paused)
		{
			eof_music_play();
		}
	}
	eof_render();

	/* select a music file */
	returnedfn = ncd_file_select(0, eof_last_ogg_path, "Select Music File", eof_filter_music_files);
	eof_clear_input();
	if(!returnedfn)
	{
		eof_cursor_visible = 1;
		eof_pen_visible = 1;
		eof_show_mouse(NULL);
		return 1;
	}
	ustrcpy(oggfilename, returnedfn);
	ustrcpy(eof_last_ogg_path, returnedfn);

	eof_color_dialog(eof_file_new_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_file_new_dialog);
	ustrcpy(eof_etext, "");
	ustrcpy(eof_etext2, "");
	ustrcpy(eof_etext4, "");
	eof_render();
	if(!ustricmp("ogg", get_extension(oggfilename)))
	{
		temp_buffer = eof_buffer_file(oggfilename);
		temp_buffer_size = file_size_ex(oggfilename);
		if(temp_buffer)
		{
			temp_ogg = alogg_create_ogg_from_buffer(temp_buffer, temp_buffer_size);
			if(temp_ogg)
			{
				alogg_get_ogg_comment(temp_ogg, "ARTIST", eof_etext);
				alogg_get_ogg_comment(temp_ogg, "TITLE", eof_etext2);
			}
		}
	}

	/* user fills in song information */
	if(eof_popup_dialog(eof_file_new_dialog, 3) != 6)
	{
		eof_cursor_visible = 1;
		eof_pen_visible = 1;
		eof_show_mouse(NULL);
		return 1;
	}

	if(ustrlen(eof_etext) > 0 && ustrlen(eof_etext2) > 0)
	{
		sprintf(eof_etext4, "%s - %s", eof_etext, eof_etext2);
	}

	/* user selects location for new song */
	eof_color_dialog(eof_file_new_windows_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_file_new_windows_dialog);
	eof_file_new_windows_dialog[1].flags = 0;
	eof_file_new_windows_dialog[2].flags = 0;
	eof_file_new_windows_dialog[3].flags = D_SELECTED;
	if(eof_popup_dialog(eof_file_new_windows_dialog, 4) == 5)
	{
		if(eof_file_new_windows_dialog[1].flags & D_SELECTED)
		{
			eof_render();
			returnedfolder = ncd_folder_select("Select Folder for Song");
			if(!returnedfolder)
			{
				eof_cursor_visible = 1;
				eof_pen_visible = 1;
				eof_show_mouse(NULL);
				return 1;
			}
			ustrcpy(eof_etext3, returnedfolder);
			newtype = 0;
		}
		else if(eof_file_new_windows_dialog[2].flags & D_SELECTED)
		{
			eof_render();
			replace_filename(eof_etext3, returnedfn, "", 1024);
			newtype = 1;
		}
		else
		{
			ustrcpy(eof_etext3, eof_songs_path);
			ustrcat(eof_etext3, eof_etext4);
			newtype = 0;
		}
	}
	else
	{
		eof_cursor_visible = 1;
		eof_pen_visible = 1;
		eof_show_mouse(NULL);
		return 1;
	}

	/* if music file is MP3, convert it */
	if(!ustricmp(get_extension(oggfilename), "mp3"))
	{
		if(eof_ogg_settings())
		{
			if(!eof_menu_file_new_supplement(newtype))
			{
				eof_cursor_visible = 1;
				eof_pen_visible = 1;
				eof_show_mouse(NULL);
				return 1;
			}
			put_backslash(eof_etext3);
			#ifdef ALLEGRO_WINDOWS
				eof_copy_file(oggfilename, "eoftemp.mp3");
				usprintf(syscommand, "mp3toogg \"eoftemp.mp3\" %s \"%sguitar.ogg\" \"%s\"", eof_ogg_quality[(int)eof_ogg_setting], eof_etext3, oggfilename);
				delete_file("eoftemp.mp3");
			#elif defined(ALLEGRO_MACOSX)
				usprintf(syscommand, "./lame --decode \"%s\" - | ./oggenc --quiet -q %s --resample 44100 -s 0 - -o \"%sguitar.ogg\"", oggfilename, eof_ogg_quality[(int)eof_ogg_setting], eof_etext3);
			#else
				usprintf(syscommand, "lame --decode \"%s\" - | oggenc --quiet -q %s --resample 44100 -s 0 - -o \"%sguitar.ogg\"", oggfilename, eof_ogg_quality[(int)eof_ogg_setting], eof_etext3);
			#endif
			eof_system(syscommand);
		}
		else
		{
			eof_cursor_visible = 1;
			eof_pen_visible = 1;
			eof_show_mouse(NULL);
			return 2;
		}
	}

	/* otherwise copy it */
	else
	{
		if(!eof_menu_file_new_supplement(newtype))
		{
			eof_cursor_visible = 1;
			eof_pen_visible = 1;
			eof_changes = 0;
			eof_show_mouse(NULL);
			return 1;
		}
		ustrcpy(syscommand, eof_etext3);
		put_backslash(syscommand);
		ustrcat(syscommand, "guitar.ogg");
		if(ustricmp(oggfilename, syscommand))
		{
			eof_copy_file(oggfilename, syscommand);
		}
	}

	/* destroy old song */
	if(eof_song)
	{
		eof_destroy_song(eof_song);
	}
	eof_destroy_ogg();

	/* create new song */
	eof_song = eof_create_song();
	if(!eof_song)
	{
		allegro_message("Error creating new song!");
		eof_cursor_visible = 1;
		eof_pen_visible = 1;
		eof_show_mouse(NULL);
		return 1;
	}

	/* fill in information */
	ustrcpy(eof_song->tags->artist, eof_etext);
	ustrcpy(eof_song->tags->title, eof_etext2);
	ustrcpy(eof_song->tags->frettist, eof_last_frettist);
	ustrcpy(oggfilename, returnedfn);
	replace_filename(eof_last_ogg_path, oggfilename, "", 1024);


//	ustrcpy(outputfilename, eof_etext3);
//	put_backslash(outputfilename);
//	ustrcat(outputfilename, "song.ini");
//	eof_save_ini(eof_song, outputfilename);

	/* load OGG */
	ustrcpy(eof_song_path, eof_etext3);
	put_backslash(eof_song_path);
	ustrcpy(eof_last_eof_path, eof_song_path);
	if(temp_ogg)
	{
		eof_music_track = temp_ogg;
		eof_music_data = temp_buffer;
		eof_music_data_size = temp_buffer_size;
	}
	else
	{
		ustrcpy(oggfilename, eof_etext3);
		put_backslash(oggfilename);
		ustrcat(oggfilename, "guitar.ogg");
		if(!eof_load_ogg(oggfilename))
		{
			eof_cursor_visible = 1;
			eof_pen_visible = 1;
			eof_show_mouse(NULL);
			return 0;
		}
	}
	eof_music_length = alogg_get_length_msecs_ogg(eof_music_track);
	eof_music_actual_length = eof_music_length;
	ustrcpy(eof_loaded_song_name, "notes.eof");

	eof_song_loaded = 1;

	/* get ready to edit */
	eof_changes = 0;
	eof_music_pos = eof_av_delay;
	eof_undo_last_type = 0;
	eof_change_count = 0;
	eof_selected_catalog_entry = 0;
//	eof_setup_menus();
	eof_calculate_beats();
	eof_menu_file_quick_save();
	eof_detect_difficulties(eof_song);
	eof_reset_lyric_preview_lines();
	eof_prepare_menus();
	eof_undo_reset();
	eof_fix_window_title();
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(NULL);

	return 1;
}

int eof_menu_file_load(void)
{
	int ret = 0;
	char temp_filename[1024] = {0};
	char * returnedfn = NULL;

	if(eof_song_loaded)
	{
		if(!eof_music_paused)
		{
			eof_music_play();
		}
	}
	if(eof_changes)
	{
		ret = alert3(NULL, "You have unsaved changes.", NULL, "Save", "Discard", "Cancel", 0, 0, 0);
		if(ret == 1)
		{
			eof_menu_file_save();
		}
		eof_clear_input();
	}
	if(ret == 3)
	{
		return 1;
	}
	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	returnedfn = ncd_file_select(0, eof_last_eof_path, "Load Song", eof_filter_eof_files);
	eof_clear_input();
	if(returnedfn)
	{
		ustrcpy(eof_filename, returnedfn);
		replace_filename(eof_last_eof_path, eof_filename, "", 1024);
		ustrcpy(eof_loaded_song_name, get_filename(eof_filename));

		/* free the old song */
		if(eof_song)
		{
			eof_destroy_song(eof_song);
		}
		eof_destroy_ogg();
		eof_song = eof_load_song(eof_filename);
		if(!eof_song)
		{
			eof_song_loaded = 0;
			allegro_message("Error loading song!");
			eof_fix_window_title();
			return 1;
		}
		replace_filename(eof_song_path, eof_filename, "", 1024);

		/* attempt to load the OGG profile OGG */
		append_filename(temp_filename, eof_song_path, eof_song->tags->ogg[eof_selected_ogg].filename, 1024);
		if(!eof_load_ogg_quick(temp_filename))
		{

			/* upon fail, fall back to "guitar.ogg" */
			append_filename(temp_filename, eof_song_path, "guitar.ogg", 1024);
			if(!eof_load_ogg(temp_filename))
			{
				eof_destroy_song(eof_song);
				eof_song=NULL;
				eof_song_loaded = 0;
				eof_changes = 0;
				eof_change_count = 0;
				eof_show_mouse(NULL);
				eof_cursor_visible = 1;
				eof_pen_visible = 1;
				eof_fix_window_title();
				return 1;
			}
			/* adjust MIDI offset if it is different */
			if(eof_selected_ogg != 0)
			{
				int j;
				if(eof_song->tags->ogg[0].midi_offset != eof_song->tags->ogg[eof_selected_ogg].midi_offset)
				{
					for(j = 0; j < eof_song->beats; j++)
					{
						eof_song->beat[j]->pos += eof_song->tags->ogg[0].midi_offset - eof_song->tags->ogg[eof_selected_ogg].midi_offset;
					}
					eof_adjust_notes(eof_song->tags->ogg[0].midi_offset - eof_song->tags->ogg[eof_selected_ogg].midi_offset);
				}
			}
			eof_selected_ogg = 0;
		}
		eof_song_loaded = 1;
		eof_changes = 0;
		eof_select_beat(0);
		eof_undo_last_type = 0;
		eof_change_count = 0;
//		eof_setup_menus();
		eof_music_length = alogg_get_length_msecs_ogg(eof_music_track);
		eof_music_pos = eof_av_delay;
		eof_selected_track = 0;
		eof_selected_catalog_entry = 0;
		eof_calculate_beats();
		eof_determine_hopos();
		eof_detect_difficulties(eof_song);
		eof_reset_lyric_preview_lines();
		eof_prepare_menus();
		eof_undo_reset();
		eof_fix_window_title();
		eof_vocal_track_fixup_lyrics(eof_song->vocal_track, 0);
	}
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	return 1;
}

int eof_menu_file_save_as(void)
{
	char old_filename[1024] = {0};
	char * returnedfn = NULL;
	EOF_OGG_INFO  temp_ogg[8];
	short         temp_oggs;
	int i;
	int swap = 0;

	if(eof_song_loaded)
	{
		if(!eof_music_paused)
		{
			eof_music_play();
		}
	}
	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();

	/* sort notes so they are in order of position */
	eof_sort_notes();
	eof_fixup_notes();

	returnedfn = ncd_file_select(1, eof_last_eof_path, "Save Song As", eof_filter_eof_files);
	eof_clear_input();
	if(returnedfn)
	{

		ustrcpy(old_filename, eof_song_path);
		replace_filename(eof_song_path, returnedfn, "", 1024);
		replace_filename(eof_last_eof_path, returnedfn, "", 1024);
		append_filename(eof_temp_filename, eof_song_path, "guitar.ogg", 1024);
		swap = ustricmp(old_filename, eof_song_path);
		if(!exists(eof_temp_filename) || swap)
		{
			/* if we are writing to a different folder and writing
			   the currenty loaded OGG as "guitar.ogg," temporarily swap
			   the OGG profiles to compensate */
			if(eof_selected_ogg != 0)
			{
				for(i = 0; i < eof_song->tags->oggs; i++)
				{
					memcpy(&temp_ogg[i], &eof_song->tags->ogg[i], sizeof(EOF_OGG_INFO));
				}
				temp_oggs = eof_song->tags->oggs;
				memcpy(&eof_song->tags->ogg[0], &eof_song->tags->ogg[eof_selected_ogg], sizeof(EOF_OGG_INFO));
				ustrcpy(eof_song->tags->ogg[0].filename, "guitar.ogg");
				eof_song->tags->oggs = 1;
				eof_selected_ogg = 0;
			}

			/* "guitar.ogg" exists so confirm overwrite when writing into a different folder */
			if(swap && exists(eof_temp_filename))
			{
				if(alert(NULL, "File \"guitar.ogg\" exists. Replace?", NULL, "&Yes", "&No", 'y', 'n') == 1)
				{
					eof_save_ogg(eof_temp_filename);
				}
			}
			else
			{
				eof_save_ogg(eof_temp_filename);
			}
		}

		/* save the chart */
		replace_extension(eof_temp_filename, returnedfn, "eof", 1024);
		eof_song->tags->revision++;
		if(!eof_save_song(eof_song, eof_temp_filename))
		{
			allegro_message("Could not save song!");
			eof_show_mouse(NULL);
			eof_cursor_visible = 1;
			eof_pen_visible = 1;
			return 2;
		}
		ustrcpy(eof_loaded_song_name, get_filename(eof_temp_filename));

		/* save the MIDI */
		append_filename(eof_temp_filename, eof_song_path, "notes.mid", 1024);
		eof_export_midi(eof_song, eof_temp_filename);
		ustrcpy(eof_temp_filename, eof_song_path);
		ustrcat(eof_temp_filename, "song.ini");
		eof_save_ini(eof_song, eof_temp_filename);

		/* finish up */
		eof_changes = 0;
		eof_undo_last_type = 0;
		eof_change_count = 0;
		eof_fix_window_title();
	}
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	return 1;
}

int eof_menu_file_load_ogg(void)
{
	char * returnedfn = NULL;
	unsigned long new_length;
	char * fn = NULL;
	int i, j;

	returnedfn = ncd_file_select(0, eof_last_ogg_path, "Select OGG File", eof_filter_ogg_files);
	eof_clear_input();
	if(returnedfn)
	{

		/* failed to load new OGG so reload old one */
		if(!eof_load_ogg(returnedfn))
		{
			append_filename(eof_temp_filename, eof_song_path, "guitar.ogg", 1024);
			returnedfn = eof_temp_filename;
			if(!eof_load_ogg(eof_temp_filename))
			{
				eof_show_mouse(NULL);
				eof_cursor_visible = 1;
				eof_pen_visible = 1;
				append_filename(eof_temp_filename, eof_song_path, "notes.lostoggbackup.eof", 1024);
				allegro_message("%s", eof_temp_filename);
				if(!eof_save_song(eof_song, eof_temp_filename))
				{
					allegro_message("Couldn't save backup!");
				}
				if(eof_song)
				{
					eof_destroy_song(eof_song);
					eof_song = NULL;
					eof_song_loaded = 0;
				}
				return 1;
			}
		}

		/* see if this OGG is already associated with the current project */
		fn = get_filename(returnedfn);
		for(i = 0; i < eof_song->tags->oggs; i++)
		{
			if(!ustricmp(fn, eof_song->tags->ogg[i].filename))
			{
				/* adjust MIDI offset if it is different */
				if(eof_selected_ogg != i)
				{
					if(eof_song->tags->ogg[i].midi_offset != eof_song->tags->ogg[eof_selected_ogg].midi_offset)
					{
						for(j = 0; j < eof_song->beats; j++)
						{
							eof_song->beat[j]->pos += eof_song->tags->ogg[i].midi_offset - eof_song->tags->ogg[eof_selected_ogg].midi_offset;
						}
						eof_adjust_notes(eof_song->tags->ogg[i].midi_offset - eof_song->tags->ogg[eof_selected_ogg].midi_offset);
					}
				}
				eof_selected_ogg = i;
				break;
			}
		}
		eof_fixup_notes();
		if(i == eof_song->tags->oggs)
		{
			if(eof_song->tags->oggs < 8)
			{
				ustrcpy(eof_song->tags->ogg[eof_song->tags->oggs].filename, fn);
				eof_song->tags->ogg[eof_song->tags->oggs].midi_offset = eof_song->tags->ogg[eof_selected_ogg].midi_offset;
				eof_selected_ogg = eof_song->tags->oggs;
				eof_song->tags->oggs++;
			}
			else
			{
				ustrcpy(eof_song->tags->ogg[eof_song->tags->oggs - 1].filename, fn);
				eof_song->tags->ogg[eof_song->tags->oggs - 1].midi_offset = eof_song->tags->ogg[eof_selected_ogg].midi_offset;
				eof_selected_ogg = eof_song->tags->oggs - 1;
			}
		}

		new_length = alogg_get_length_msecs_ogg(eof_music_track);
		if(new_length > eof_music_length)
		{
			eof_music_length = new_length;
			eof_calculate_beats();
		}
		if(eof_music_pos > eof_music_actual_length)
		{
			eof_menu_song_seek_end();
		}
		eof_music_seek(eof_music_pos - eof_av_delay);
		replace_filename(eof_last_ogg_path, returnedfn, "", 1024);
	}

	return 1;
}

int eof_menu_file_save(void)
{
	char temp_filename[1024] = {0};
	int err;

	if(eof_song_loaded)
	{
		if(!eof_music_paused)
		{
			eof_music_play();
		}
	}
	eof_cursor_visible = 0;
	eof_pen_visible = 0;

	/* no changes so ask for save */
	if(eof_changes <= 0)
	{
		eof_show_mouse(screen);
		if(alert(NULL, "No Changes, save anyway?", NULL, "&Yes", "&No", 'y', 'n') == 2)
		{
			eof_show_mouse(NULL);
			eof_cursor_visible = 1;
			eof_pen_visible = 1;
			return 1;
		}
	}

	/* check to see if song folder still exists */
	ustrcpy(eof_temp_filename, eof_song_path);
	if(eof_temp_filename[uoffset(eof_temp_filename, ustrlen(eof_temp_filename) - 1)] == '\\' || eof_temp_filename[uoffset(eof_temp_filename, ustrlen(eof_temp_filename) - 1)] == '/')
	{
		eof_temp_filename[uoffset(eof_temp_filename, ustrlen(eof_temp_filename) - 1)] = '\0';
	}
	if(!file_exists(eof_temp_filename, FA_DIREC | FA_HIDDEN, NULL))
	{
		if(alert("Song folder no longer exists.", "Recreate folder?", NULL, "&Yes", "&No", 'y', 'n') == 2)
		{
			eof_show_mouse(NULL);
			eof_cursor_visible = 1;
			eof_pen_visible = 1;
			return 1;
		}
		err = eof_mkdir(eof_temp_filename);
		if(err)
		{
			eof_render();
			allegro_message("Could not create folder!\n%s", eof_temp_filename);
			eof_show_mouse(NULL);
			eof_cursor_visible = 1;
			eof_pen_visible = 1;
			return 1;
		}
	}


//	eof_render();
	append_filename(temp_filename, eof_song_path, eof_loaded_song_name, 1024);

	/* sort notes so they are in order of position */
	eof_sort_notes();
	eof_fixup_notes();

	/* sort lyrics */
	eof_vocal_track_sort_lyrics(eof_song->vocal_track);
	eof_vocal_track_fixup_lyrics(eof_song->vocal_track, 0);

	replace_extension(temp_filename, temp_filename, "eof", 1024);
	eof_song->tags->revision++;
	if(!eof_save_song(eof_song, temp_filename))
	{
		allegro_message("Could not save song!\n%s", temp_filename);
		eof_show_mouse(NULL);
		eof_cursor_visible = 1;
		eof_pen_visible = 1;
		return 2;
	}
	append_filename(temp_filename, eof_song_path, "notes.mid", 1024);
	eof_export_midi(eof_song, temp_filename);
	append_filename(temp_filename, eof_song_path, "song.ini", 1024);
	eof_save_ini(eof_song, temp_filename);
	append_filename(temp_filename, eof_song_path, "guitar.ogg", 1024);
	if(!exists(temp_filename))
	{
		eof_save_ogg(temp_filename);
	}
	eof_changes = 0;
	eof_undo_last_type = 0;
	eof_change_count = 0;
	eof_fix_window_title();
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	return 1;
}

int eof_menu_file_quick_save(void)
{
	char temp_filename[1024] = {0};

	append_filename(temp_filename, eof_song_path, eof_loaded_song_name, 1024);

	eof_song->tags->revision++;
	if(!eof_save_song(eof_song, temp_filename))
	{
		allegro_message("Could not save song!");
		eof_show_mouse(NULL);
		eof_cursor_visible = 1;
		eof_pen_visible = 1;
		return 1;
	}
	eof_changes = 0;
	eof_undo_last_type = 0;
	eof_change_count = 0;
	eof_fix_window_title();
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	return 1;
}

int eof_menu_file_lyrics_import(void)
{
	char * returnedfn = NULL;
	int jumpcode = 0;
	int selectedformat=0;
	char *selectedtrack;
//	struct Lyric_Format *detectionlist;	//This is the list populated by EOF_IMPORT_VIA_LC() in the event of multiple possible imports

	if(eof_song == NULL)	//Do not import lyrics if no chart is open
		return 0;

	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	returnedfn = ncd_file_select(0, eof_filename, "Import Lyrics", eof_filter_lyrics_files);
	eof_clear_input();
	if(returnedfn)
	{
		ustrcpy(eof_filename, returnedfn);
		jumpcode=setjmp(jumpbuffer); //Store environment/stack/etc. info in the jmp_buf array
		if(jumpcode!=0) //if program control returned to the setjmp() call above returning any nonzero value
		{
			puts("Assert() handled sucessfully!");
			eof_show_mouse(NULL);
			eof_cursor_visible = 1;
			eof_pen_visible = 1;
			return 1;
		}
		else
		{
		//Detect if the selected file has lyrics
			lyricdetectionlist=DetectLyricFormat(returnedfn);	//Auto detect the lyric format of the chosen file
			ReleaseMemory(1);	//Release memory allocated during lyric import
			if(lyricdetectionlist == NULL)
			{
				alert("Error", NULL, "No lyrics detected", "OK", NULL, 0, KEY_ENTER);
				return 0;	//return error
			}

		//Import lyrics
			if(lyricdetectionlist->next == NULL)	//If this file had only one detected lyric format
			{
				eof_prepare_undo(0);	//Make a generic undo state
				EOF_IMPORT_VIA_LC(eof_song->vocal_track, NULL, lyricdetectionlist->format, returnedfn, lyricdetectionlist->track);
					//Import the format
			}
			else
			{	//Prompt user to select from the multiple possible imports
				eof_lyric_import_prompt(&selectedformat,&selectedtrack);
				if(selectedformat)	//If a selection was made
				{
					eof_prepare_undo(0);	//Make a generic undo state
					EOF_IMPORT_VIA_LC(eof_song->vocal_track, NULL, selectedformat, returnedfn, selectedtrack);
				}
			}
		}
	}
	eof_vocal_track_fixup_lyrics(eof_song->vocal_track, 0);
	eof_reset_lyric_preview_lines();
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	return 1;
}

int eof_menu_file_midi_import(void)
{
	int ret = 0;
	char * returnedfn = NULL;

	if(eof_song_loaded)
	{
		eof_music_paused = 1;
		alogg_stop_ogg(eof_music_track);
	}
	if(eof_changes)
	{
		ret = alert3(NULL, "You have unsaved changes.", NULL, "Save", "Discard", "Cancel", 0, 0, 0);
		if(ret == 1)
		{
			eof_menu_file_save();
		}
		eof_clear_input();
	}
	if(ret == 3)
	{
		return 1;
	}
	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	returnedfn = ncd_file_select(0, eof_last_eof_path, "Import MIDI", eof_filter_midi_files);
	eof_clear_input();
	if(returnedfn)
	{
		if(eof_song)
		{
			eof_destroy_song(eof_song);
		}
		eof_destroy_ogg();
		ustrcpy(eof_filename, returnedfn);
		ustrcpy(eof_loaded_song_name, get_filename(eof_filename));
		replace_extension(eof_loaded_song_name, eof_loaded_song_name, "eof", 1024);
		replace_filename(eof_last_eof_path, eof_filename, "", 1024);
		eof_song = eof_import_midi(eof_filename);
		if(eof_song)
		{
			eof_selected_track = 0;
			eof_music_pos = eof_av_delay;
			eof_selected_catalog_entry = 0;
			eof_sort_notes();
			eof_fixup_notes();
			eof_calculate_beats();
			eof_determine_hopos();
			eof_detect_difficulties(eof_song);
			eof_prepare_menus();
			eof_undo_reset();
			eof_catalog_menu[0].flags = 0;
			eof_select_beat(0);
			eof_fix_window_title();
			eof_song_loaded = 1;
		}
		else
		{
			eof_song_loaded = 0;
			allegro_message("Could not import song!");
			eof_fix_window_title();
		}
		eof_changes = 0;
		eof_undo_last_type = 0;
		eof_change_count = 0;
	}
	eof_vocal_track_fixup_lyrics(eof_song->vocal_track, 0);
	eof_reset_lyric_preview_lines();
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	return 1;
}

int eof_menu_file_settings(void)
{
	if(eof_song_loaded)
	{
		if(!eof_music_paused)
		{
			eof_music_play();
		}
	}
	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	eof_color_dialog(eof_settings_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_settings_dialog);
	sprintf(eof_etext, "%d", eof_av_delay);
	sprintf(eof_etext2, "%d", eof_buffer_size);
	eof_settings_dialog[6].d2 = eof_cpu_saver;
	eof_settings_dialog[7].flags = eof_smooth_pos ? D_SELECTED : 0;
	eof_settings_dialog[8].flags = eof_disable_windows ? D_SELECTED : 0;
	eof_settings_dialog[9].flags = eof_disable_vsync ? D_SELECTED : 0;
	if(eof_popup_dialog(eof_settings_dialog, 0) == 10)
	{
		eof_av_delay = atol(eof_etext);
		if(eof_av_delay < 0)
		{
			allegro_message("AV Delay must be at least 0.\nIt has been set to 0 for now.");
			eof_av_delay = 0;
		}
		eof_buffer_size = atol(eof_etext2);
		if(eof_buffer_size < 1024)
		{
			allegro_message("Buffer size must be at least 1024. It has been set to 1024.");
			eof_buffer_size = 1024;
		}
		eof_cpu_saver = eof_settings_dialog[6].d2;
		eof_smooth_pos = (eof_settings_dialog[7].flags == D_SELECTED ? 1 : 0);
		eof_disable_windows = (eof_settings_dialog[8].flags == D_SELECTED ? 1 : 0);
		eof_disable_vsync = (eof_settings_dialog[9].flags == D_SELECTED ? 1 : 0);
		ncdfs_use_allegro = eof_disable_windows;
	}
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	return 1;
}

int eof_menu_file_preferences(void)
{
	if(eof_song_loaded)
	{
		if(!eof_music_paused)
		{
			eof_music_play();
		}
	}
	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	eof_color_dialog(eof_preferences_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_preferences_dialog);
	eof_preferences_dialog[1].flags = eof_inverted_notes ? D_SELECTED : 0;
	eof_preferences_dialog[2].flags = eof_lefty_mode ? D_SELECTED : 0;
	eof_preferences_dialog[3].flags = eof_note_auto_adjust ? D_SELECTED : 0;
	eof_preferences_dialog[5].d1 = eof_input_mode;
	if(eof_popup_dialog(eof_preferences_dialog, 0) == 6)
	{
		eof_inverted_notes = (eof_preferences_dialog[1].flags == D_SELECTED ? 1 : 0);
		eof_lefty_mode = (eof_preferences_dialog[2].flags == D_SELECTED ? 1 : 0);
		eof_note_auto_adjust = (eof_preferences_dialog[3].flags == D_SELECTED ? 1 : 0);
		eof_input_mode = eof_preferences_dialog[5].d1;
	}
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	return 1;
}

int eof_menu_file_display(void)
{
	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	eof_color_dialog(eof_display_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_display_dialog);
	eof_display_dialog[4].d1 = eof_screen_layout.mode;
	eof_display_dialog[1].flags = eof_soft_cursor ? D_SELECTED : 0;
	eof_display_dialog[2].flags = (eof_desktop == 0) ? D_SELECTED : 0;
	if(eof_popup_dialog(eof_display_dialog, 0) == 5)
	{
		eof_destroy_data();
		eof_soft_cursor = (eof_display_dialog[1].flags & D_SELECTED) ? 1 : 0;
		eof_desktop = (eof_display_dialog[2].flags & D_SELECTED) ? 0 : 1;
		if(eof_desktop)
		{
			set_color_depth(desktop_color_depth() != 0 ? desktop_color_depth() : 8);
		}
		else
		{
			set_color_depth(8);
		}
		if(!eof_load_data())
		{
			allegro_message("Could not reload program data!");
			exit(0);
		}
		eof_set_display_mode(eof_display_dialog[4].d1);
		set_palette(eof_palette);
		if(!eof_soft_cursor)
		{
			if(show_os_cursor(2) < 0)
			{
				eof_soft_cursor = 1;
			}
		}
		else
		{
			show_os_cursor(0);
		}
	}
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	return 1;
}

int eof_ogg_settings(void)
{
	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	eof_color_dialog(eof_ogg_settings_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_ogg_settings_dialog);
	eof_ogg_settings_dialog[3].d1 = eof_ogg_setting;
	if(eof_popup_dialog(eof_ogg_settings_dialog, 0) == 4)
	{
		eof_ogg_setting = eof_ogg_settings_dialog[3].d1;
	}
	else
	{
		return 0;
	}
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	return 1;
}

int eof_menu_file_controllers(void)
{
	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	eof_color_dialog(eof_controller_settings_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_controller_settings_dialog);
	if(eof_popup_dialog(eof_controller_settings_dialog, 0) == 3)
	{
	}
	else
	{
		return 0;
	}
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	return 1;
}

int eof_menu_file_song_folder(void)
{
	char * returnfolder = NULL;

	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	returnfolder = ncd_folder_select("Set EOF Song Folder");
	if(returnfolder)
	{
		ustrcpy(eof_songs_path, returnfolder);
	}
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	return 1;
}

int eof_menu_file_link(void)
{
	char * returnfolder = NULL;
	char * returnedfn = NULL;

	if(eof_song_loaded)
	{
		if(!eof_music_paused)
		{
			eof_music_play();
		}
	}
	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	returnedfn = ncd_file_select(0, NULL, "Locate FOF Executable", eof_filter_exe_files);
	eof_clear_input();
	if(returnedfn)
	{
		returnfolder = ncd_folder_select("Locate FOF Songs Folder");
		if(returnfolder)
		{
			ustrcpy(eof_fof_executable_name, get_filename(returnedfn));
			ustrcpy(eof_fof_executable_path, returnedfn);
			ustrcpy(eof_fof_songs_path, returnfolder);
		}
	}
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	return 1;
}

int eof_menu_file_exit(void)
{
	int ret = 0;
	int ret2 = 1;

	if(eof_song_loaded)
	{
		if(!eof_music_paused)
		{
			eof_music_play();
		}
		if(eof_music_catalog_playback)
		{
			eof_music_catalog_playback = 0;
			eof_music_catalog_pos = eof_song->catalog->entry[eof_selected_catalog_entry].start_pos + eof_av_delay;
			alogg_stop_ogg(eof_music_track);
			alogg_seek_abs_msecs_ogg(eof_music_track, eof_music_pos);
		}
	}
	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	if(alert(NULL, "Want to Quit?", NULL, "&Yes", "&No", 'y', 'n') == 1)
	{
		if(eof_changes)
		{
			ret = alert3(NULL, "Save changes before quitting?", NULL, "&Yes", "&No", "Cancel", 'y', 'n', 0);
			if(ret == 1)
			{
				if(eof_menu_file_save() == 2)
				{
					ret2 = alert3(NULL, "Save failed! Exit without saving?", NULL, "&Yes", "&No", NULL, 'y', 'n', 0);
				}
			}
		}
		if(ret != 3)
		{
			if(ret2 == 1)
			{
				eof_quit = 1;
			}
		}
		eof_clear_input();
	}
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	return 1;
}

char * eof_input_list(int index, int * size)
{
	switch(index)
	{
		case -1:
		{
			*size = EOF_INPUT_NAME_NUM-1;
			break;
		}
		case 0:
		{
			return "Classic";
			break;
		}
		case 1:
		{
			return "Piano Roll";
			break;
		}
		case 2:
		{
			return "Hold";
			break;
		}
		case 3:
		{
			return "RexMundi";
			break;
		}
		case 4:
		{
			return "Guitar Tap";
			break;
		}
		case 5:
		{
			return "Guitar Strum";
			break;
		}
//		case 6:
//		{
//			return "Feedback";
//			break;
//		}
	}
	return NULL;
}

char * eof_ogg_list(int index, int * size)
{
	switch(index)
	{
		case -1:
		{
			*size = 6;
			break;
		}
		case 0:
		{
			return "80kbps";
			break;
		}
		case 1:
		{
			return "96kbps";
			break;
		}
		case 2:
		{
			return "128kbps";
			break;
		}
		case 3:
		{
			return "160kbps";
			break;
		}
		case 4:
		{
			return "192kbps";
			break;
		}
		case 5:
		{
			return "256kbps";
			break;
		}
	}
	return NULL;
}

char * eof_guitar_list(int index, int * size)
{
	sprintf(eof_ctext[0], "Strum 1 (%s)", redefine_index == 0 ? "Press new key" : eof_guitar.button[0].name);
	sprintf(eof_ctext[1], "Strum 2 (%s)", redefine_index == 1 ? "Press new key" : eof_guitar.button[1].name);
	sprintf(eof_ctext[2], "Green Fret (%s)", redefine_index == 2 ? "Press new key" : eof_guitar.button[2].name);
	sprintf(eof_ctext[3], "Red Fret (%s)", redefine_index == 3 ? "Press new key" : eof_guitar.button[3].name);
	sprintf(eof_ctext[4], "Yellow Fret (%s)", redefine_index == 4 ? "Press new key" : eof_guitar.button[4].name);
	sprintf(eof_ctext[5], "Blue Fret (%s)", redefine_index == 5 ? "Press new key" : eof_guitar.button[5].name);
	sprintf(eof_ctext[6], "Purple Fret (%s)", redefine_index == 6 ? "Press new key" : eof_guitar.button[6].name);
	switch(index)
	{
		case -1:
		{
			*size = 7;
			break;
		}
		case 0:
		{
			return eof_ctext[0];
			break;
		}
		case 1:
		{
			return eof_ctext[1];
			break;
		}
		case 2:
		{
			return eof_ctext[2];
			break;
		}
		case 3:
		{
			return eof_ctext[3];
			break;
		}
		case 4:
		{
			return eof_ctext[4];
			break;
		}
		case 5:
		{
			return eof_ctext[5];
			break;
		}
		case 6:
		{
			return eof_ctext[6];
			break;
		}
	}
	return NULL;
}

char * eof_drum_list(int index, int * size)
{
	sprintf(eof_ctext[0], "Kick Pedal (%s)", redefine_index == 0 ? "Press new key" : eof_drums.button[0].name);
	sprintf(eof_ctext[1], "Red Drum (%s)", redefine_index == 1 ? "Press new key" : eof_drums.button[1].name);
	sprintf(eof_ctext[2], "Yellow Drum (%s)", redefine_index == 2 ? "Press new key" : eof_drums.button[2].name);
	sprintf(eof_ctext[3], "Blue Drum (%s)", redefine_index == 3 ? "Press new key" : eof_drums.button[3].name);
	sprintf(eof_ctext[4], "Green Drum (%s)", redefine_index == 4 ? "Press new key" : eof_drums.button[4].name);
	switch(index)
	{
		case -1:
		{
			*size = 5;
			break;
		}
		case 0:
		{
			return eof_ctext[0];
			break;
		}
		case 1:
		{
			return eof_ctext[1];
			break;
		}
		case 2:
		{
			return eof_ctext[2];
			break;
		}
		case 3:
		{
			return eof_ctext[3];
			break;
		}
		case 4:
		{
			return eof_ctext[4];
			break;
		}
	}
	return NULL;
}

char * eof_display_list(int index, int * size)
{
	switch(index)
	{
		case -1:
		{
			*size = 3;
			break;
		}
		case EOF_DISPLAY_640:
		{
			return "640x480";
			break;
		}
		case EOF_DISPLAY_800:
		{
			return "800x600";
			break;
		}
		case EOF_DISPLAY_1024:
		{
			return "1024x768";
			break;
		}
	}
	return NULL;
}

int eof_controller_settings_guitar(DIALOG * d)
{
	eof_color_dialog(eof_guitar_settings_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_guitar_settings_dialog);
	eof_guitar_settings_dialog[1].d1 = 0;
	sprintf(eof_etext, "%d", eof_guitar.delay);
	if(eof_popup_dialog(eof_guitar_settings_dialog, 0) == 5)
	{
		eof_guitar.delay = atoi(eof_etext);
	}
	eof_render();
	dialog_message(eof_controller_settings_dialog, MSG_DRAW, 0, NULL);
	return 0;
}

int eof_controller_settings_drums(DIALOG * d)
{
	eof_color_dialog(eof_drum_settings_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_drum_settings_dialog);
	eof_drum_settings_dialog[1].d1 = 0;
	sprintf(eof_etext, "%d", eof_drums.delay);
	if(eof_popup_dialog(eof_drum_settings_dialog, 0) == 5)
	{
		eof_drums.delay = atoi(eof_etext);
	}
	eof_render();
	dialog_message(eof_controller_settings_dialog, MSG_DRAW, 0, NULL);
	return 0;
}

int eof_guitar_controller_redefine(DIALOG * d)
{
	int i;
	redefine_index = eof_guitar_settings_dialog[3].d1;
	dialog_message(eof_guitar_settings_dialog, MSG_DRAW, 0, &i);
	eof_controller_set_button(&eof_guitar.button[eof_guitar_settings_dialog[3].d1]);
	eof_clear_input();
	redefine_index = -1;

	//Wait for user to release button 0 or 1 on the first controller to prevent Allegro from hijacking the controller input
	while((joy[0].button[0].b) || (joy[0].button[1].b))
	{
		poll_joystick();
	}

	dialog_message(eof_guitar_settings_dialog, MSG_DRAW, 0, &i);

	if(eof_test_controller_conflict(&eof_guitar,0,6))
		alert("Warning", NULL, "There is a key conflict for this controller", "OK", NULL, 0, KEY_ENTER);

	return 0;
}

int eof_drum_controller_redefine(DIALOG * d)
{
	int i;
	redefine_index = eof_drum_settings_dialog[3].d1;
	dialog_message(eof_drum_settings_dialog, MSG_DRAW, 0, &i);
	eof_controller_set_button(&eof_drums.button[eof_drum_settings_dialog[3].d1]);
	eof_clear_input();
	redefine_index = -1;

	//Wait for user to release button 0 or 1 on the first controller to prevent Allegro from hijacking the controller input
	while((joy[0].button[0].b) || (joy[0].button[1].b))
	{
		poll_joystick();
	}

	dialog_message(eof_drum_settings_dialog, MSG_DRAW, 0, &i);

	if(eof_test_controller_conflict(&eof_drums,0,4))
		alert("Warning", NULL, "There is a key conflict for this controller", "OK", NULL, 0, KEY_ENTER);

	return 0;
}

char *eof_lyric_detections_list_all(int index, int * size)
{
	unsigned long ctr;
	struct Lyric_Format *ptr;

	if(index >= 0)
	{	//Return a display string for item #index
		ptr=GetDetectionNumber(lyricdetectionlist,index);
		if(ptr == NULL)
			return NULL;

		if(printf("%s\t->\t%lu Lyrics",ptr->track,ptr->count) + 1 > 1024)	//If for some abnormal reason, the lyric info is too long to display in 1024 characters,
			return ptr->track;												//return just the track name instead of allowing an overflow

		sprintf(lyricdetectionstring,"%s -> %lu Lyrics",ptr->track,ptr->count);	//Write a bounds-checked formatted string to the global lyricdetectionstring array
		return lyricdetectionstring;	//and return it to be displayed in the list box
	}

	//Otherwise return NULL and set *size to the number of items to display in the list
	for(ctr=1,ptr=lyricdetectionlist;ptr->next != NULL;ptr=ptr->next,ctr++);	//Count the number of lyric formats detected
	*size=ctr;
	return NULL;
}

struct Lyric_Format *GetDetectionNumber(struct Lyric_Format *detectionlist, unsigned long number)
{
	unsigned long ctr;
	struct Lyric_Format *ptr;	//Linked list conductor

	ptr=detectionlist;	//Point at front of list
	for(ctr=0;ctr<number;ctr++)
	{
		if(ptr->next == NULL)
			return NULL;

		ptr=ptr->next;
	}

	return ptr;
}

void eof_lyric_import_prompt(int *selectedformat, char **selectedtrack)
{
	struct Lyric_Format *ptr;

	if(!selectedformat || !selectedtrack || !lyricdetectionlist)	//If any of these are NULL
		return;

	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_lyric_detections_dialog, gui_fg_color, gui_bg_color);	//Display the lyric detections dialog
	centre_dialog(eof_lyric_detections_dialog);				//Center it
	eof_lyric_detections_dialog[1].d1 = 0;				//Init the list procedure's selection variable to 0

	if(eof_popup_dialog(eof_lyric_detections_dialog, 0) == 2)	//If a user selection was made?
	{	//Seek to the selected link
		ptr=GetDetectionNumber(lyricdetectionlist,eof_lyric_detections_dialog[1].d1);
		*selectedformat=ptr->format;
		*selectedtrack=ptr->track;
	}
	else
		*selectedformat=0;	//Failure

	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(NULL);
}

int eof_test_controller_conflict(EOF_CONTROLLER *controller,int start,int stop)
{
	int ctr1;	//The counter for the key being tested
	int ctr2;	//The counter for the keys being tested against

	for(ctr1=start;ctr1<=stop;ctr1++)	//For each key to test
	{
		if(controller->button[ctr1].type == EOF_CONTROLLER_BUTTON_TYPE_KEY)	//Keyboard button
		{
			for(ctr2=start;ctr2<=stop;ctr2++)	//For each key to test against
			{
				if(ctr2 == ctr1)
					continue;	//Don't test the key against itself

				if(controller->button[ctr1].key == controller->button[ctr2].key)
					return 1;	//Return conflict
			}
		}
		else if(controller->button[ctr1].type == EOF_CONTROLLER_BUTTON_TYPE_JOYBUTTON)	//Controller button
		{
			for(ctr2=start;ctr2<=stop;ctr2++)	//For each key to test against
			{
				if(ctr2 == ctr1)
					continue;	//Don't test the key against itself

				if(	(controller->button[ctr1].key == controller->button[ctr2].key) &&
					(controller->button[ctr1].joy == controller->button[ctr2].joy))
					return 1;	//Return conflict
			}
		}
		else if(controller->button[ctr1].type == EOF_CONTROLLER_BUTTON_TYPE_JOYAXIS)		//Controller joystick
		{
			for(ctr2=start;ctr2<=stop;ctr2++)	//For each key to test against
			{
				if(ctr2 == ctr1)
					continue;	//Don't test the key against itself

				if(	(controller->button[ctr1].joy == controller->button[ctr2].joy) &&
					(controller->button[ctr1].index == controller->button[ctr2].index) &&
					(controller->button[ctr1].key == controller->button[ctr2].key))
					return 1;	//Return conflict
			}
		}
	}

	return 0;	//No conflict found
}

int eof_menu_file_feedback_import(void)
{
	char * returnedfn = NULL;
	int jumpcode = 0;

	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	returnedfn = ncd_file_select(0, eof_filename, "Import Feedback Chart", eof_filter_dB_files);
	eof_clear_input();
	if(returnedfn)
	{
		ustrcpy(eof_filename, returnedfn);
		jumpcode=setjmp(jumpbuffer); //Store environment/stack/etc. info in the jmp_buf array
		if(jumpcode!=0) //if program control returned to the setjmp() call above returning any nonzero value
		{	//Import failed
			puts("Assert() handled sucessfully!");
			allegro_message("dB Chart import failed");
			eof_show_mouse(NULL);
			eof_cursor_visible = 1;
			eof_pen_visible = 1;
			return 1;
		}
		else
		{
			/* destroy loaded song */
			if(eof_song)
			{
				eof_destroy_song(eof_song);
			}

			/* import chart */
			eof_song = eof_import_chart(returnedfn);
			if(eof_song)
			{
				eof_calculate_beats();
				eof_changes = 0;
				eof_song_loaded = 1;
				eof_fix_window_title();
				eof_detect_difficulties(eof_song);
			}
			else
			{
				eof_song_loaded = 0;
				eof_changes = 0;
			}
		}
	}
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	return 1;
}

void EnumeratedBChartInfo(struct FeedbackChart *chart)
{
	int printflen;	//Used to store the length of a printed format string
	char *tempstr;	//Used to store a formatted print string
	char *chartinfo=NULL;

	if(chart == NULL)
		return;

	chartinfo=DuplicateString("dB Chart info:");
	if(chart->name)
	{
		chartinfo=ResizedAppend(chartinfo,"\nName: ",1);
		chartinfo=ResizedAppend(chartinfo,chart->name,1);
	}
	if(chart->artist)
	{
		chartinfo=ResizedAppend(chartinfo,"\nArtist: ",1);
		chartinfo=ResizedAppend(chartinfo,chart->artist,1);
	}
	if(chart->charter)
	{
		chartinfo=ResizedAppend(chartinfo,"\nCharter: ",1);
		chartinfo=ResizedAppend(chartinfo,chart->charter,1);
	}
	if(chart->audiofile)
	{
		chartinfo=ResizedAppend(chartinfo,"\nMusicStream: ",1);
		chartinfo=ResizedAppend(chartinfo,chart->audiofile,1);
	}
	printflen=printf("\nOffset: %lu",chart->offset)+1;	//Find the number of characters needed to sprintf this string
	if(printflen > 0)
	{
		tempstr=malloc_err(printflen);
		sprintf(tempstr,"\nOffset: %lu",chart->offset);
		chartinfo=ResizedAppend(chartinfo,tempstr,1);
		free(tempstr);
	}
	printflen=printf("\nResolution: %lu",chart->resolution)+1;	//Find the number of characters needed to sprintf this string
	if(printflen > 0)
	{
		tempstr=malloc_err(printflen);
		sprintf(tempstr,"\nResolution: %lu",chart->resolution);
		chartinfo=ResizedAppend(chartinfo,tempstr,1);
		free(tempstr);
	}
	printflen=printf("\n%lu Instrument tracks loaded",chart->tracksloaded)+1;	//Find the number of characters needed to sprintf this string
	if(printflen > 0)
	{
		tempstr=malloc_err(printflen);
		sprintf(tempstr,"\n%lu Instrument tracks loaded",chart->tracksloaded);
		chartinfo=ResizedAppend(chartinfo,tempstr,1);
		free(tempstr);
	}

	allegro_message("%s",chartinfo);
}
