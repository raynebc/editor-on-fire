#include <allegro.h>
#include <ctype.h>
#include "../agup/agup.h"
#include "../foflc/Lyric_storage.h"
#include "../foflc/ID3_parse.h"
#include "../lc_import.h"
#include "../midi_import.h"
#include "../chart_import.h"
#include "../gh_import.h"
#include "../gp_import.h"
#include "../main.h"
#include "../player.h"
#include "../dialog.h"
#include "../undo.h"
#include "../utility.h"
#include "../midi.h"
#include "../ini.h"
#include "../ini_import.h"
#include "../feedback.h"
#include "../dialog/proc.h"
#include "../mix.h"
#include "../tuning.h"
#include "../rs.h"
#include "../silence.h"	//For save_wav_with_silence_appended
#include "beat.h"	//For eof_menu_beat_reset_offset()
#include "file.h"
#include "song.h"
#include "edit.h"	//For eof_menu_edit_undo()
#include "note.h"	//For eof_correct_chord_fingerings()

#ifdef USEMEMWATCH
#include "../memwatch.h"
#endif

MENU eof_file_menu[] =
{
    {"&New\t" CTRL_NAME "+N / F4", eof_menu_file_new_wizard, NULL, 0, NULL},
    {"&Load\t" CTRL_NAME "+O", eof_menu_file_load, NULL, 0, NULL},
    {"&Save\tF2 / "CTRL_NAME "+S", eof_menu_file_save, NULL, D_DISABLED, NULL},
    {"Save &As", eof_menu_file_save_as, NULL, D_DISABLED, NULL},
    {"", NULL, NULL, 0, NULL},
    {"Load &OGG", eof_menu_file_load_ogg, NULL, D_DISABLED, NULL},
    {"", NULL, NULL, 0, NULL},
    {"&MIDI Import\tF6", eof_menu_file_midi_import, NULL, 0, NULL},
    {"&Feedback Import\tF7", eof_menu_file_feedback_import, NULL, 0, NULL},
    {"&GH Import", eof_menu_file_gh_import, NULL, 0, NULL},
    {"Lyric Import\tF8", eof_menu_file_lyrics_import, NULL, 0, NULL},
    {"Guitar Pro Import", eof_menu_file_gp_import, NULL, 0, NULL},
    {"", NULL, NULL, 0, NULL},
    {"Settings\tF10", eof_menu_file_settings, NULL, 0, NULL},
    {"&Preferences\tF11", eof_menu_file_preferences, NULL, 0, NULL},
    {"&Display", eof_menu_file_display, NULL, 0, NULL},
    {"Set display width", eof_set_display_width, NULL, EOF_LINUX_DISABLE, NULL},
    {"&Controllers", eof_menu_file_controllers, NULL, 0, NULL},
    {"", NULL, NULL, 0, NULL},
    {"Song Folder", eof_menu_file_song_folder, NULL, 0, NULL},
    {"Link to FOF", eof_menu_file_link_fof, NULL, EOF_LINUX_DISABLE, NULL},
    {"Link to Phase Shift", eof_menu_file_link_ps, NULL, EOF_LINUX_DISABLE, NULL},
    {"", NULL, NULL, 0, NULL},
    {"E&xit\tEsc", eof_menu_file_exit, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

DIALOG eof_settings_dialog[] =
{
   /* (proc)         (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)           (dp2) (dp3) */
   { d_agup_window_proc,    0,  48,  200, 140 + 64 + 16, 2,   23,  0,    0,      0,   0,   "Settings",               NULL, NULL },
   { d_agup_text_proc,   16,  84,  64,  8,  2,   23,  0,    0,      0,   0,   "AV Delay (ms):",         NULL, NULL },
   { eof_verified_edit_proc,   128, 80,  64,  20,  2,   23,  0,    0,      5,   0,   eof_etext,           "0123456789", NULL },
   { d_agup_text_proc,   16,  108,  64,  8,  2,   23,  0,    0,      0,   0,   "Buffer Size:",         NULL, NULL },
   { eof_verified_edit_proc,   128, 104,  64,  20,  2,   23,  0,    0,      5,   0,   eof_etext2,           "0123456789", NULL },
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
   /* (proc)            (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                   (dp2) (dp3) */
   { d_agup_window_proc,0,   48,  480, 365, 2,   23,  0,    0,      0,   0,   "Preferences",         NULL, NULL },
   { d_agup_check_proc, 16,  75,  110, 16,  2,   23,  0,    0,      1,   0,   "Inverted Notes",      NULL, NULL },
   { d_agup_check_proc, 150, 75,  92 , 16,  2,   23,  0,    0,      1,   0,   "Lefty Mode",          NULL, NULL },
   { d_agup_check_proc, 306, 75,  128, 16,  2,   23,  0,    0,      1,   0,   "Note Auto-Adjust",    NULL, NULL },
   { d_agup_check_proc, 16,  90,  124, 16,  2,   23,  0,    0,      1,   0,   "Import/Export TS",    NULL, NULL },
   { d_agup_check_proc, 150, 90,  148, 16,  2,   23,  0,    0,      1,   0,   "Hide drum note tails",NULL, NULL },
   { d_agup_check_proc, 306, 90,  122, 16,  2,   23,  0,    0,      1,   0,   "Hide note names",     NULL, NULL },
   { d_agup_check_proc, 16,  105, 130, 16,  2,   23,  0,    0,      1,   0,   "Disable sound FX",NULL, NULL },
   { d_agup_check_proc, 150, 105, 148, 16,  2,   23,  0,    0,      1,   0,   "Disable 3D rendering",NULL, NULL },
   { d_agup_check_proc, 306, 105, 148, 16,  2,   23,  0,    0,      1,   0,   "Disable 2D rendering",NULL, NULL },
   { d_agup_check_proc, 16,  120, 116, 16,  2,   23,  0,    0,      1,   0,   "Hide info panel",NULL, NULL },
   { d_agup_check_proc, 150, 120, 206, 16,  2,   23,  0,    0,      1,   0,   "Erase overlapped pasted notes",NULL, NULL },
   { d_agup_check_proc, 16,  136, 208, 16,  2,   23,  0,    0,      1,   0,   "Save separate Rock Band files",NULL, NULL },
   { d_agup_check_proc, 248, 136, 206, 16,  2,   23,  0,    0,      1,   0,   "Save separate Rocksmith files",NULL, NULL },
   { d_agup_check_proc, 16,  152, 210, 16,  2,   23,  0,    0,      1,   0,   "Treat inverted chords as slash",NULL, NULL },
   { d_agup_check_proc, 248, 152, 182, 16,  2,   23,  0,    0,      1,   0,   "Enable logging on launch",NULL, NULL },
   { d_agup_check_proc, 16,  168, 190, 16,  2,   23,  0,    0,      1,   0,   "Add new notes to selection",NULL, NULL },
   { d_agup_check_proc, 248, 168, 216, 16,  2,   23,  0,    0,      1,   0,   "Drum modifiers affect all diff's",NULL, NULL },
   { d_agup_text_proc,  16,  185, 144, 12,  0,   0,   0,    0,      0,   0,   "Min. note distance (ms):",NULL,NULL },
   { eof_verified_edit_proc,170,185,30,20,  0,   0,   0,    0,      3,   0,   eof_etext2,     "0123456789", NULL },
   { d_agup_text_proc,  248, 185, 144, 12,  0,   0,   0,    0,      0,   0,   "Min. note length (ms):",NULL,NULL },
   { eof_verified_edit_proc,392,185,30,20,  0,   0,   0,    0,      3,   0,   eof_etext,     "0123456789", NULL },
   { d_agup_check_proc, 248, 237, 214, 16,  2,   23,  0,    0,      1,   0,   "3D render bass drum in a lane",NULL, NULL },
   { d_agup_check_proc, 248, 253, 184, 16,  2,   23,  0,    0,      1,   0,   "Use dB style seek controls",NULL, NULL },
   { d_agup_text_proc,  24,  237, 48,  8,   2,   23,  0,    0,      0,   0,   "Input Method",        NULL, NULL },
   { d_agup_list_proc,  16,  255, 100, 110, 2,   23,  0,    0,      0,   0,   eof_input_list,        NULL, NULL },
   { d_agup_text_proc,  150, 255, 48,  8,   2,   23,  0,    0,      0,   0,   "Color set",           NULL, NULL },
   { d_agup_list_proc,  129, 270, 100, 95,  2,   23,  0,    0,      0,   0,   eof_colors_list,       NULL, NULL },
   { d_agup_button_proc,12,  370, 68,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",                  NULL, NULL },
   { d_agup_button_proc,86,  370, 68,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Default",             NULL, NULL },
   { d_agup_button_proc,160, 370, 68,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",              NULL, NULL },
   { d_agup_text_proc,  16,  206, 120, 12,  0,   0,   0,    0,      0,   0,   "Top of 2D pane shows:",NULL,NULL },
   { d_agup_radio_proc, 161, 206, 60,  16,  2,   23,  0,    0,      1,   0,   "Names",               NULL, NULL },
   { d_agup_radio_proc, 224, 206, 72,  16,  2,   23,  0,    0,      1,   0,   "Sections",            NULL, NULL },
   { d_agup_radio_proc, 301, 206, 78,  16,  2,   23,  0,    0,      1,   0,   "Hand pos",            NULL, NULL },
   { d_agup_radio_proc, 384, 206, 92,  16,  2,   23,  0,    0,      1,   0,   "RS sections",         NULL, NULL },
   { d_agup_check_proc, 248, 269, 206, 16,  2,   23,  0,    0,      1,   0,   "New notes are made 1ms long",NULL, NULL },
   { d_agup_check_proc, 16,  221, 340, 16,  2,   23,  0,    0,      1,   0,   "GP import beat text as sections, markers as phrases",NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_display_dialog[] =
{
   /* (proc)         (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)           (dp2) (dp3) */
   { d_agup_window_proc,    0,  48,  200, 196, 2,   23,  0,    0,      0,   0,   "Display Settings",               NULL, NULL },

   { d_agup_check_proc, 16,  80, 160,  16, 2,   23,  0,    0, 1,   0,   "Use Software Cursor",               NULL, NULL },
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
   { eof_verified_edit_proc,   104, 236,  64,  20,  2,   23,  0,    0,      5,   0,   eof_etext,           "0123456789", NULL },
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
   { eof_verified_edit_proc,   104, 236,  64,  20,  2,   23,  0,    0,      5,   0,   eof_etext,           "0123456789", NULL },
   { d_agup_list_proc,   13, 266,  170 + 32 + 85 + 28,  79,  2,   23,  0,    0,      0,   0,   eof_drum_list, NULL, NULL },
   { d_agup_push_proc, 13,  176 + 64 + 22 + 16 + 24 + 24 + 24 + 36 - 30, 170 + 32 + 85 + 28,  28, 2,   23,  0,    D_EXIT, 0,   0,   "Redefine",               NULL, eof_drum_controller_redefine },
   { d_agup_button_proc, 13, 176 + 64 + 22 + 16 + 24 + 24 + 24 + 36 + 32 + 8 - 30, 170 + 32 + 85 + 28,  28, 2,   23,  0,    D_EXIT, 0,   0,   "OK",               NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_controller_settings_dialog[] =
{
   /* (proc)             (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                   (dp2) (dp3) */
   { d_agup_window_proc, 0,   200, 222, 144, 2,   23,  0,    0,      0,   0,   "Controller Settings", NULL, NULL },
   { d_agup_push_proc,   12,  236, 198, 28,  2,   23,  'g',  D_EXIT, 0,   0,   "&Guitar",             NULL, eof_controller_settings_guitar },
   { d_agup_push_proc,   12,  266, 198, 28,  2,   23,  'd',  D_EXIT, 0,   0,   "&Drums",              NULL, eof_controller_settings_drums },
   { d_agup_button_proc, 12,  304, 198, 28,  2,   23,  '\r', D_EXIT, 0,   0,   "Done",                NULL, NULL },
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
char lyricdetectionstring[1024] = {0};		//The display name given to the detection when displayed in the list box
static int redefine_index = -1;

void eof_prepare_file_menu(void)
{
	if(eof_song && eof_song_loaded)
	{	//If a chart is loaded
		eof_file_menu[2].flags = 0; // Save
		eof_file_menu[3].flags = 0; // Save As
		eof_file_menu[5].flags = 0; // Load OGG
		eof_file_menu[10].flags = 0; // Lyric Import
		if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{
			eof_file_menu[11].flags = 0; // Guitar Pro Import
		}
		else
		{
			eof_file_menu[11].flags = D_DISABLED;
		}
	}
	else
	{
		eof_file_menu[2].flags = D_DISABLED; // Save
		eof_file_menu[3].flags = D_DISABLED; // Save As
		eof_file_menu[5].flags = D_DISABLED; // Load OGG
		eof_file_menu[10].flags = D_DISABLED; // Lyric Import
		eof_file_menu[11].flags = D_DISABLED; // Guitar Pro Import
	}
}

int eof_menu_file_new_supplement(char *directory, char check)
{
	char syscommand[1024] = {0};
	int err;

	(void) ustrcpy(syscommand, directory);

	/* remove slash from folder name so we can check if it exists */
	if((syscommand[uoffset(syscommand, ustrlen(syscommand) - 1)] == '\\') || (syscommand[uoffset(syscommand, ustrlen(syscommand) - 1)] == '/'))
	{
		syscommand[uoffset(syscommand, ustrlen(syscommand) - 1)] = '\0';
	}
	if(!file_exists(syscommand, FA_DIREC | FA_HIDDEN, NULL))
	{	//If the folder doesn't exist,
		err = eof_mkdir(syscommand);	//Try to create it
		if(err)
		{	//If it couldn't be created
			eof_render();
			allegro_message("Could not create folder!\n%s\nEnsure that the specified folder name is valid and\nthe Song Folder is configured to a non write-restricted area.\n(File->Song Folder)", syscommand);
			return 0;
		}
	}

	eof_render();
	err = 0;
	put_backslash(directory);
	if(check == 1)
	{	//If checking for the presence of guitar.ogg
		(void) replace_filename(eof_temp_filename, directory, "guitar.ogg", 1024);
		if(exists(eof_temp_filename))
		{
			if(alert(NULL, "Existing guitar.ogg will be overwritten. Proceed?", NULL, "&Yes", "&No", 'y', 'n') == 2)
			{
				return 0;
			}
		}
	}
	else if(check == 2)
	{	//If checking for the presence of original.mp3
		(void) replace_filename(eof_temp_filename, directory, "original.mp3", 1024);
		if(exists(eof_temp_filename))
		{
			if(alert(NULL, "Existing original.mp3 will be overwritten. Proceed?", NULL, "&Yes", "&No", 'y', 'n') == 2)
			{
				return 0;
			}
		}
	}
	else
	{	//If checking for the presence of any of the default chart file names
		(void) replace_filename(eof_temp_filename, directory, "guitar.ogg", 1024);
		if(exists(eof_temp_filename))
		{
			err = 1;
		}
		(void) replace_filename(eof_temp_filename, directory, "original.mp3", 1024);
		if(exists(eof_temp_filename))
		{
			err = 1;
		}
		(void) replace_filename(eof_temp_filename, directory, "notes.eof", 1024);
		if(exists(eof_temp_filename))
		{
			err = 1;
		}
		(void) replace_filename(eof_temp_filename, directory, "song.ini", 1024);
		if(exists(eof_temp_filename))
		{
			err = 1;
		}
		(void) replace_filename(eof_temp_filename, directory, "notes.mid", 1024);
		if(exists(eof_temp_filename))
		{
			err = 1;
		}
	}
	if(err)
	{
		if(alert(NULL, "Some existing chart files will be overwritten. Proceed?", NULL, "&Yes", "&No", 'y', 'n') == 2)
		{
			return 0;
		}
	}
	return 1;
}

int eof_menu_file_new_wizard(void)
{
	char * returnedfn = NULL;

	if(eof_menu_prompt_save_changes() == 3)
	{	//If user canceled closing the current, modified chart
		return 1;
	}
	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	returnedfn = ncd_file_select(0, eof_last_ogg_path, "Select Music File", eof_filter_music_files);
	eof_clear_input();
	if(!returnedfn)
	{
		eof_cursor_visible = 1;
		eof_pen_visible = 1;
		eof_show_mouse(NULL);
		return 1;
	}

	return eof_new_chart(returnedfn);
}

int eof_menu_file_load(void)
{
	char temp_filename[1024] = {0};
	char * returnedfn = NULL;

	if(eof_menu_prompt_save_changes() == 3)
	{	//If user canceled closing the current, modified chart
		return 1;
	}
	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	returnedfn = ncd_file_select(0, eof_last_eof_path, "Load Song", eof_filter_eof_files);
	eof_clear_input();
	if(returnedfn)
	{	//If the user selected a file
		(void) ustrcpy(eof_filename, returnedfn);
		(void) replace_filename(eof_last_eof_path, eof_filename, "", 1024);
		(void) ustrcpy(eof_loaded_song_name, get_filename(eof_filename));

		/* free the current project */
		if(eof_song)
		{
			eof_destroy_song(eof_song);
			eof_song = NULL;
			eof_song_loaded = 0;
		}
		eof_destroy_ogg();

		/* load the specified project */
		eof_song = eof_load_song(eof_filename);
		if(!eof_song)
		{
			eof_song_loaded = 0;
			allegro_message("Error loading song!");
			eof_fix_window_title();
			return 1;
		}
		(void) replace_filename(eof_song_path, eof_filename, "", 1024);

		/* check song.ini and prompt user to load any external edits */
		(void) replace_filename(temp_filename, eof_song_path, "song.ini", 1024);
		(void) eof_import_ini(eof_song, temp_filename, 1);	//Read song.ini and prompt to replace values of existing settings in the project if they are different

		/* attempt to load the OGG profile OGG */
		(void) replace_filename(temp_filename, eof_song_path, eof_song->tags->ogg[eof_selected_ogg].filename, 1024);
		if(!eof_load_ogg_quick(temp_filename))
		{
			/* upon fail, fall back to "guitar.ogg" */
			(void) append_filename(temp_filename, eof_song_path, "guitar.ogg", 1024);
			if(!eof_load_ogg(temp_filename, 1))	//If user does not provide audio, fail over to using silent audio
			{
				eof_destroy_song(eof_song);
				eof_song = NULL;
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
					(void) eof_adjust_notes(eof_song->tags->ogg[0].midi_offset - eof_song->tags->ogg[eof_selected_ogg].midi_offset);
				}
			}
			eof_selected_ogg = 0;
		}
		eof_song_loaded = 1;
		eof_chart_length = alogg_get_length_msecs_ogg(eof_music_track);
		eof_init_after_load(0);
		eof_track_fixup_notes(eof_song, EOF_TRACK_VOCALS, 0);
	}//If the user selected a file
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	return 1;
}

int eof_menu_file_save_as(void)
{
	char new_foldername[1024] = {0};
	char * returnedfn = NULL;
	EOF_OGG_INFO  temp_ogg[8];
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

	returnedfn = ncd_file_select(1, eof_last_eof_path, "Save Song As", eof_filter_eof_files);
	eof_clear_input();
	if(returnedfn)
	{
		eof_log("\tPerforming \"Save as\"", 1);

		(void) replace_filename(new_foldername, returnedfn, "", 1024);		//Obtain the chosen destination folder path
		if(eof_menu_file_new_supplement(new_foldername, 3) == 0)	//If the folder doesn't exist, or the user has declined to overwrite any existing files
			return 1;	//Return failure

		if(!eof_silence_loaded)
		{	//Only do audio checking if chart audio is loaded
			(void) append_filename(eof_temp_filename, new_foldername, "guitar.ogg", 1024);
			swap = ustricmp(new_foldername, eof_song_path);

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
					memcpy(&eof_song->tags->ogg[0], &eof_song->tags->ogg[eof_selected_ogg], sizeof(EOF_OGG_INFO));
					(void) ustrcpy(eof_song->tags->ogg[0].filename, "guitar.ogg");
					eof_song->tags->oggs = 1;
					eof_selected_ogg = 0;
				}
			}
		}

		i=eof_save_helper(returnedfn);	//Perform "Save As" operation to the selected path
		if(i == 0)
		{	//If the "Save as" operation succeeded, update folder path strings
			(void) ustrcpy(eof_song_path,new_foldername);
			(void) ustrcpy(eof_last_eof_path,new_foldername);
		}
		return i;
	}
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;

	return 1;	//Return canceled
}

int eof_menu_file_load_ogg(void)
{
	char * returnedfn;
	char checkfn[1024] = {0};
	char checkfn2[1024] = {0};
	unsigned long new_length;
	char * fn = NULL;
	int i, j;

	returnedfn = ncd_file_select(0, eof_last_ogg_path, "Select OGG File", eof_filter_ogg_files);
	eof_clear_input();
	if(returnedfn)
	{	//If the user selected an OGG file
		eof_get_rocksmith_wav_path(checkfn, eof_song_path, sizeof(checkfn));	//Build the path to the WAV file written for Rocksmith during save
		(void) delete_file(checkfn);	//Delete it, if it exists, since changing the chart's OGG will necessitate rewriting the WAV file during save

		/* make sure selected file is in the same path as the current chart */
		(void) replace_filename(checkfn, returnedfn, "", 1024);
		(void) replace_filename(checkfn2, eof_song_path, "", 1024);
		if(ustricmp(checkfn, checkfn2))
		{
			allegro_message("OGGs can only be loaded from the current song folder!\n");
			return 1;
		}

		/* failed to load new OGG so reload old one */
		if(!eof_load_ogg(returnedfn, 0))
		{	//If eof_load_ogg() failed, eof_loaded_ogg_name contains the name of the file that was loaded before
			returnedfn = eof_loaded_ogg_name;
			if(!eof_load_ogg(eof_loaded_ogg_name, 0))
			{
				eof_show_mouse(NULL);
				eof_cursor_visible = 1;
				eof_pen_visible = 1;
				(void) append_filename(eof_temp_filename, eof_song_path, "notes.lostoggbackup.eof", 1024);
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
						(void) eof_adjust_notes(eof_song->tags->ogg[i].midi_offset - eof_song->tags->ogg[eof_selected_ogg].midi_offset);
					}
				}
				eof_selected_ogg = i;
				break;
			}
		}
		eof_fixup_notes(eof_song);
		if(i == eof_song->tags->oggs)
		{
			if(eof_song->tags->oggs < 8)
			{	//Create a new OGG profile
				(void) ustrcpy(eof_song->tags->ogg[eof_song->tags->oggs].filename, fn);
				eof_song->tags->ogg[eof_song->tags->oggs].description[0] = '\0';
				eof_song->tags->ogg[eof_song->tags->oggs].midi_offset = eof_song->tags->ogg[eof_selected_ogg].midi_offset;
				eof_song->tags->ogg[eof_song->tags->oggs].modified = 0;
				eof_selected_ogg = eof_song->tags->oggs;
				eof_song->tags->oggs++;
			}
			else
			{	//Replace the last OGG profile
				(void) ustrcpy(eof_song->tags->ogg[eof_song->tags->oggs - 1].filename, fn);
				eof_song->tags->ogg[eof_song->tags->oggs - 1].description[0] = '\0';
				eof_song->tags->ogg[eof_song->tags->oggs - 1].midi_offset = eof_song->tags->ogg[eof_selected_ogg].midi_offset;
				eof_song->tags->ogg[eof_song->tags->oggs - 1].modified = 0;
				eof_selected_ogg = eof_song->tags->oggs - 1;
			}
		}

		new_length = alogg_get_length_msecs_ogg(eof_music_track);
		if(new_length > eof_chart_length)
		{
			eof_chart_length = new_length;
			eof_calculate_beats(eof_song);
		}
		if(eof_music_pos > eof_music_length)
		{
			(void) eof_menu_song_seek_end();
		}
		eof_music_seek(eof_music_pos - eof_av_delay);
		(void) replace_filename(eof_last_ogg_path, returnedfn, "", 1024);
	}

	return 1;
}

int eof_menu_file_save(void)
{
	int err;

	if(!eof_song_loaded || !eof_song)
		return 1;	//Do not allow this function to run if no chart is loaded

	if(!eof_music_paused)
	{
		eof_music_play();
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
	(void) ustrcpy(eof_temp_filename, eof_song_path);
	if((eof_temp_filename[uoffset(eof_temp_filename, ustrlen(eof_temp_filename) - 1)] == '\\') || (eof_temp_filename[uoffset(eof_temp_filename, ustrlen(eof_temp_filename) - 1)] == '/'))
	{	//If the path ends in a separator
		eof_temp_filename[uoffset(eof_temp_filename, ustrlen(eof_temp_filename) - 1)] = '\0';	//Remove it
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

	eof_log("\tSaving chart", 1);

	return eof_save_helper(NULL);	//Perform "Save" operation to the chart's current path
}

int eof_menu_file_quick_save(void)
{
	char temp_filename[1024] = {0};

	(void) append_filename(temp_filename, eof_song_path, eof_loaded_song_name, 1024);

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
	int returncode = 1;	//Stores the return value of EOF_IMPORT_VIA_LC() to check for error

	if(eof_song == NULL)	//Do not import lyrics if no chart is open
		return 0;

	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	returnedfn = ncd_file_select(0, eof_filename, "Import Lyrics", eof_filter_lyrics_files);
	eof_clear_input();
	if(returnedfn)
	{
		(void) ustrcpy(eof_filename, returnedfn);
		jumpcode=setjmp(jumpbuffer); //Store environment/stack/etc. info in the jmp_buf array
		if(jumpcode!=0) //if program control returned to the setjmp() call above returning any nonzero value
		{
			(void) puts("Assert() handled sucessfully!");
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
				(void) alert("Error", NULL, "No lyrics detected", "OK", NULL, 0, KEY_ENTER);
				return 0;	//return error
			}

		//Import lyrics
			if(lyricdetectionlist->next == NULL)	//If this file had only one detected lyric format
			{
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make a generic undo state
				returncode = EOF_IMPORT_VIA_LC(eof_song->vocal_track[0], NULL, lyricdetectionlist->format, returnedfn, lyricdetectionlist->track);
					//Import the format
			}
			else
			{	//Prompt user to select from the multiple possible imports
				eof_lyric_import_prompt(&selectedformat,&selectedtrack);
				if(selectedformat)	//If a selection was made
				{
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make a generic undo state
					returncode = EOF_IMPORT_VIA_LC(eof_song->vocal_track[0], NULL, selectedformat, returnedfn, selectedtrack);
				}
			}
			if(returncode == 0)
			{	//This was initialized to nonzero, so if it's zero now, the import above failed, undo the import by loading the last undo state
				(void) alert("Error", NULL, "Invalid lyric file", "OK", NULL, 0, KEY_ENTER);
				eof_menu_edit_undo();
			}
		}
	}
	eof_track_fixup_notes(eof_song, EOF_TRACK_VOCALS, 0);
	eof_reset_lyric_preview_lines();
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	DestroyLyricFormatList(lyricdetectionlist);	//Free the detection list
	return 1;
}

int eof_menu_file_midi_import(void)
{
	char * returnedfn = NULL;
	char tempfilename[1024] = {0};

	if(eof_menu_prompt_save_changes() == 3)
	{	//If user canceled closing the current, modified chart
		return 1;
	}
	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	returnedfn = ncd_file_select(0, eof_last_eof_path, "Import MIDI", eof_filter_midi_files);
	eof_clear_input();
	if(returnedfn)
	{
		eof_log("\tImporting MIDI", 1);

		if(eof_song)
		{
			eof_destroy_song(eof_song);
			eof_song = NULL;
			eof_song_loaded = 0;
		}
		eof_destroy_ogg();
		(void) ustrcpy(eof_filename, returnedfn);
		(void) ustrcpy(eof_loaded_song_name, get_filename(eof_filename));
		(void) replace_extension(eof_loaded_song_name, eof_loaded_song_name, "eof", 1024);
		(void) replace_filename(eof_last_eof_path, eof_filename, "", 1024);
		if(!ustricmp(get_extension(eof_filename), "rba"))
		{
			(void) replace_filename(tempfilename, eof_filename, "eof_rba_import.tmp", 1024);
			if(eof_extract_rba_midi(eof_filename,tempfilename) == 0)
			{	//If this was an RBA file and the MIDI was extracted successfully
				eof_song = eof_import_midi(tempfilename);
				(void) delete_file(tempfilename);	//Delete temporary file
			}
		}
		else
		{	//Perform regular MIDI import
			eof_song = eof_import_midi(eof_filename);
		}
		if(eof_song)
		{
			eof_song_loaded = 1;
			eof_init_after_load(0);
			eof_track_fixup_notes(eof_song, EOF_TRACK_VOCALS, 0);
		}
		else
		{
			eof_song_loaded = 0;
			allegro_message("Could not import song!");
			eof_fix_window_title();
			eof_changes = 0;
			eof_undo_last_type = 0;
			eof_change_count = 0;
		}
	}
	eof_reset_lyric_preview_lines();
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;

	eof_log("\tMIDI loaded", 1);

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
	(void) snprintf(eof_etext, sizeof(eof_etext) - 1, "%d", eof_av_delay);
	(void) snprintf(eof_etext2, sizeof(eof_etext2) - 1, "%d", eof_buffer_size);
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
	int retval, original_input_mode;

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
	//Use the currently configured settings to populate the dialog selections
	eof_preferences_dialog[1].flags = eof_inverted_notes ? D_SELECTED : 0;				//Inverted notes
	eof_preferences_dialog[2].flags = eof_lefty_mode ? D_SELECTED : 0;					//Lefty mode
	eof_preferences_dialog[3].flags = eof_note_auto_adjust ? D_SELECTED : 0;			//Note auto adjust
	eof_preferences_dialog[4].flags = eof_use_ts ? D_SELECTED : 0;						//Import/Export TS
	eof_preferences_dialog[5].flags = eof_hide_drum_tails ? D_SELECTED : 0;				//Hide drum note tails
	eof_preferences_dialog[6].flags = eof_hide_note_names ? D_SELECTED : 0;				//Hide note names
	eof_preferences_dialog[7].flags = eof_disable_sound_processing ? D_SELECTED : 0;	//Disable sound effects
	eof_preferences_dialog[8].flags = eof_disable_3d_rendering ? D_SELECTED : 0;		//Disable 3D rendering
	eof_preferences_dialog[9].flags = eof_disable_2d_rendering ? D_SELECTED : 0;		//Disable 2D rendering
	eof_preferences_dialog[10].flags = eof_disable_info_panel ? D_SELECTED : 0;			//Disable info panel
	eof_preferences_dialog[11].flags = eof_paste_erase_overlap ? D_SELECTED : 0;		//Erase overlapped pasted notes
	eof_preferences_dialog[12].flags = eof_write_rb_files ? D_SELECTED : 0;				//Save separate Rock Band files
	eof_preferences_dialog[13].flags = eof_write_rs_files ? D_SELECTED : 0;				//Save separate Rocksmith files
	eof_preferences_dialog[14].flags = eof_inverted_chords_slash ? D_SELECTED : 0;		//Treat inverted chords as slash
	eof_preferences_dialog[15].flags = enable_logging ? D_SELECTED : 0;					//Enable logging on launch
	eof_preferences_dialog[16].flags = eof_add_new_notes_to_selection ? D_SELECTED : 0;	//Add new notes to selection
	eof_preferences_dialog[17].flags = eof_drum_modifiers_affect_all_difficulties ? D_SELECTED : 0;	//Drum modifiers affect all diff's
	eof_preferences_dialog[23].flags = eof_fb_seek_controls ? D_SELECTED : 0;			//Use dB style seek controls
	eof_preferences_dialog[32].flags = eof_preferences_dialog[33].flags = eof_preferences_dialog[34].flags = eof_preferences_dialog[35].flags = 0;	//Options for what to display at top of 2D panel
	eof_preferences_dialog[eof_2d_render_top_option].flags = D_SELECTED;
	eof_preferences_dialog[36].flags = eof_new_note_length_1ms ? D_SELECTED : 0;		//New notes are made 1ms long
	eof_preferences_dialog[37].flags = eof_gp_import_preference_1 ? D_SELECTED : 0;		//GP import beat text as sections, markers as phrases
	if(eof_min_note_length)
	{	//If the user has defined a minimum note length
		(void) snprintf(eof_etext, sizeof(eof_etext) - 1, "%d", eof_min_note_length);	//Populate the field's string with it
	}
	else
	{
		eof_etext[0] = '\0';	//Otherwise empty the string
	}
	(void) snprintf(eof_etext2, sizeof(eof_etext2) - 1, "%d", eof_min_note_distance);	//Populate the min. note distance field's string
	eof_preferences_dialog[22].flags = eof_render_bass_drum_in_lane ? D_SELECTED : 0;	//3D render bass drum in a lane
	eof_preferences_dialog[25].d1 = eof_input_mode;										//Input method
	original_input_mode = eof_input_mode;												//Store this value
	eof_preferences_dialog[27].d1 = eof_color_set;										//Color set

	do
	{	//Run the dialog
		retval = eof_popup_dialog(eof_preferences_dialog, 0);	//Run the dialog
		if(retval == 28)
		{	//If the user clicked OK, update EOF's configured settings from the dialog selections
			eof_inverted_notes = (eof_preferences_dialog[1].flags == D_SELECTED ? 1 : 0);
			eof_lefty_mode = (eof_preferences_dialog[2].flags == D_SELECTED ? 1 : 0);
			eof_note_auto_adjust = (eof_preferences_dialog[3].flags == D_SELECTED ? 1 : 0);
			eof_use_ts = (eof_preferences_dialog[4].flags == D_SELECTED ? 1 : 0);
			eof_hide_drum_tails = (eof_preferences_dialog[5].flags == D_SELECTED ? 1 : 0);
			eof_hide_note_names = (eof_preferences_dialog[6].flags == D_SELECTED ? 1 : 0);
			eof_disable_sound_processing = (eof_preferences_dialog[7].flags == D_SELECTED ? 1 : 0);
			eof_disable_3d_rendering = (eof_preferences_dialog[8].flags == D_SELECTED ? 1 : 0);
			eof_disable_2d_rendering = (eof_preferences_dialog[9].flags == D_SELECTED ? 1 : 0);
			eof_disable_info_panel = (eof_preferences_dialog[10].flags == D_SELECTED ? 1 : 0);
			eof_paste_erase_overlap = (eof_preferences_dialog[11].flags == D_SELECTED ? 1 : 0);
			eof_write_rb_files = (eof_preferences_dialog[12].flags == D_SELECTED ? 1 : 0);
			eof_write_rs_files = (eof_preferences_dialog[13].flags == D_SELECTED ? 1 : 0);
			eof_inverted_chords_slash = (eof_preferences_dialog[14].flags == D_SELECTED ? 1 : 0);
			enable_logging = (eof_preferences_dialog[15].flags == D_SELECTED ? 1 : 0);
			eof_add_new_notes_to_selection = (eof_preferences_dialog[16].flags == D_SELECTED ? 1 : 0);
			eof_drum_modifiers_affect_all_difficulties = (eof_preferences_dialog[17].flags == D_SELECTED ? 1 : 0);
			if(eof_etext[0] != '\0')
			{	//If the minimum note length field is populated
				eof_min_note_length = atol(eof_etext);
			}
			else
			{
				eof_min_note_length = 0;
			}
			if(eof_etext2[0] != '\0')
			{	//If the minimum note distance field is populated
				eof_min_note_distance = atol(eof_etext2);
				if(eof_min_note_distance < 1)
				{	//Validate this value
					eof_min_note_distance = 3;
				}
			}
			else
			{
				eof_min_note_distance = 3;
			}
			eof_render_bass_drum_in_lane = (eof_preferences_dialog[22].flags == D_SELECTED ? 1 : 0);
			eof_fb_seek_controls = (eof_preferences_dialog[23].flags == D_SELECTED ? 1 : 0);
			eof_input_mode = eof_preferences_dialog[25].d1;
			eof_set_2D_lane_positions(0);	//Update ychart[] by force just in case eof_inverted_notes was changed
			eof_set_3D_lane_positions(0);	//Update xchart[] by force just in case eof_lefty_mode was changed
			eof_color_set = eof_preferences_dialog[27].d1;
			if(eof_preferences_dialog[32].flags == D_SELECTED)
			{	//User opted to display note names at top of 2D panel
				eof_2d_render_top_option = 32;
			}
			else if(eof_preferences_dialog[33].flags == D_SELECTED)
			{	//User opted to display section names at top of 2D panel
				eof_2d_render_top_option = 33;
			}
			else if(eof_preferences_dialog[34].flags == D_SELECTED)
			{	//User opted to display fret hand positions at top of 2D panel
				eof_2d_render_top_option = 34;
			}
			else if(eof_preferences_dialog[35].flags == D_SELECTED)
			{	//User opted to display Rocksmith sections at top of 2D panel
				eof_2d_render_top_option = 35;
			}
			eof_new_note_length_1ms = (eof_preferences_dialog[36].flags == D_SELECTED ? 1 : 0);
			eof_gp_import_preference_1 = (eof_preferences_dialog[37].flags == D_SELECTED ? 1 : 0);
		}//If the user clicked OK
		else if(retval == 29)
		{	//If the user clicked "Default, change all selections to EOF's default settings
			eof_preferences_dialog[1].flags = 0;					//Inverted notes
			eof_preferences_dialog[2].flags = 0;					//Lefty mode
			eof_preferences_dialog[3].flags = D_SELECTED;			//Note auto adjust
			eof_preferences_dialog[4].flags = D_SELECTED;			//Import/Export TS
			eof_preferences_dialog[5].flags = 0;					//Hide drum note tails
			eof_preferences_dialog[6].flags = 0;					//Hide note names
			eof_preferences_dialog[7].flags = 0;					//Disable sound effects
			eof_preferences_dialog[8].flags = 0;					//Disable 3D rendering
			eof_preferences_dialog[9].flags = 0;					//Disable 2D rendering
			eof_preferences_dialog[10].flags = 0;					//Disable info panel
			eof_preferences_dialog[11].flags = 0;					//Erase overlapped pasted notes
			eof_preferences_dialog[12].flags = 0;					//Save separate RBN MIDI files

			eof_preferences_dialog[14].flags = 0;					//Treat inverted chords as slash
			eof_preferences_dialog[15].flags = D_SELECTED;			//Enable logging on launch
			eof_preferences_dialog[16].flags = 0;					//Add new notes to selection
			eof_preferences_dialog[17].flags = D_SELECTED;			//Drum modifiers affect all diff's
			eof_etext2[0] = '3';									//Min. note distance
			eof_etext2[1] = '\0';
			eof_etext[0] = '\0';									//Min. note length
			eof_preferences_dialog[22].flags = 0;					//3D render bass drum in a lane
			eof_preferences_dialog[23].flags = 0;					//Use dB style seek controls
			eof_preferences_dialog[25].d1 = EOF_INPUT_PIANO_ROLL;	//Input method
			eof_preferences_dialog[27].d1 = EOF_COLORS_DEFAULT;		//Color set
			eof_preferences_dialog[32].flags = D_SELECTED;			//Display note names at top of 2D panel
			eof_preferences_dialog[33].flags = eof_preferences_dialog[34].flags = eof_preferences_dialog[35].flags = 0;	//Display sections/fret hand positions at top of 2D panel
			eof_preferences_dialog[36].flags = 0;					//New notes are made 1ms long
			eof_preferences_dialog[37].flags = 0;					//GP import beat text as sections, markers as phrases
		}//If the user clicked "Default
	}while(retval == 29);	//Keep re-running the dialog until the user closes it with anything besides "Default"
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_set_color_set();
	if(original_input_mode != eof_input_mode)
	{	//If the input mode was changed
		eof_seek_selection_start = eof_seek_selection_end = 0;	//Clear the seek selection
	}
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
		eof_soft_cursor = (eof_display_dialog[1].flags & D_SELECTED) ? 1 : 0;
		eof_desktop = (eof_display_dialog[2].flags & D_SELECTED) ? 0 : 1;
		eof_apply_display_settings(eof_display_dialog[4].d1);
	}
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;

	return 1;
}

void eof_apply_display_settings(int mode)
{
	eof_destroy_data();
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
	eof_set_display_mode_preset(mode);
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

	//Update coordinate related items
	eof_scale_fretboard(0);			//Recalculate the 2D screen positioning based on the current track
	eof_set_2D_lane_positions(0);	//Update ychart[] by force just in case the display window size was changed
	eof_set_3D_lane_positions(0);	//Update xchart[] by force just in case the display window size was changed
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
	char * returnfolder;

	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	returnfolder = ncd_folder_select("Set EOF Song Folder");
	if(returnfolder)
	{
		(void) ustrcpy(eof_songs_path, returnfolder);
	}
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	return 1;
}

int eof_menu_file_link(char application)
{
	char *fofdisplayname = "FoF";
	char *psdisplayname = "Phase Shift";
	char titlebartext[100] = {0};
	char *appdisplayname = NULL;
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
	if(application == 1)
	{	//User is linking to FoF
		appdisplayname = fofdisplayname;
	}
	else
	{	//User is linking to Phase Shift
		appdisplayname = psdisplayname;
	}
	(void) snprintf(titlebartext, sizeof(titlebartext) - 1, "Locate %s Executable", appdisplayname);
	returnedfn = ncd_file_select(0, NULL, titlebartext, eof_filter_exe_files);
	eof_clear_input();
	if(returnedfn)
	{
		(void) snprintf(titlebartext, sizeof(titlebartext) - 1, "Locate %s Songs Folder", appdisplayname);
		returnfolder = ncd_folder_select(titlebartext);
		if(returnfolder)
		{
			if(application == 1)
			{	//User is linking to FoF
				(void) ustrcpy(eof_fof_executable_name, get_filename(returnedfn));
				(void) ustrcpy(eof_fof_executable_path, returnedfn);
				(void) ustrcpy(eof_fof_songs_path, returnfolder);
			}
			else
			{	//User is linking to Phase Shift
				(void) ustrcpy(eof_ps_executable_name, get_filename(returnedfn));
				(void) ustrcpy(eof_ps_executable_path, returnedfn);
				(void) ustrcpy(eof_ps_songs_path, returnfolder);
			}
		}
	}
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	return 1;
}

int eof_menu_file_link_fof(void)
{
	return eof_menu_file_link(1);	//Link to FoF
}

int eof_menu_file_link_ps(void)
{
	return eof_menu_file_link(2);	//Link to Phase Shift
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
			eof_stop_midi();
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
				eof_restore_oggs_helper();
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
			*size = EOF_INPUT_NAME_NUM;
			break;
		}
		case 0:
		{
			return "Classic";
		}
		case 1:
		{
			return "Piano Roll";
		}
		case 2:
		{
			return "Hold";
		}
		case 3:
		{
			return "RexMundi";
		}
		case 4:
		{
			return "Guitar Tap";
		}
		case 5:
		{
			return "Guitar Strum";
		}
		case 6:
		{
			return "Feedback";
		}
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
		}
		case 1:
		{
			return "96kbps";
		}
		case 2:
		{
			return "128kbps";
		}
		case 3:
		{
			return "160kbps";
		}
		case 4:
		{
			return "192kbps";
		}
		case 5:
		{
			return "256kbps";
		}
	}
	return NULL;
}

char * eof_guitar_list(int index, int * size)
{
	(void) snprintf(eof_ctext[0], sizeof(eof_ctext[0]) - 1, "Strum 1 (%s)", redefine_index == 0 ? "Press new key" : eof_guitar.button[0].name);
	(void) snprintf(eof_ctext[1], sizeof(eof_ctext[1]) - 1, "Strum 2 (%s)", redefine_index == 1 ? "Press new key" : eof_guitar.button[1].name);
	(void) snprintf(eof_ctext[2], sizeof(eof_ctext[2]) - 1, "Fret 1 (%s)", redefine_index == 2 ? "Press new key" : eof_guitar.button[2].name);
	(void) snprintf(eof_ctext[3], sizeof(eof_ctext[3]) - 1, "Fret 2 (%s)", redefine_index == 3 ? "Press new key" : eof_guitar.button[3].name);
	(void) snprintf(eof_ctext[4], sizeof(eof_ctext[4]) - 1, "Fret 3 (%s)", redefine_index == 4 ? "Press new key" : eof_guitar.button[4].name);
	(void) snprintf(eof_ctext[5], sizeof(eof_ctext[5]) - 1, "Fret 4 (%s)", redefine_index == 5 ? "Press new key" : eof_guitar.button[5].name);
	(void) snprintf(eof_ctext[6], sizeof(eof_ctext[6]) - 1, "Fret 5 (%s)", redefine_index == 6 ? "Press new key" : eof_guitar.button[6].name);
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
		}
		case 1:
		{
			return eof_ctext[1];
		}
		case 2:
		{
			return eof_ctext[2];
		}
		case 3:
		{
			return eof_ctext[3];
		}
		case 4:
		{
			return eof_ctext[4];
		}
		case 5:
		{
			return eof_ctext[5];
		}
		case 6:
		{
			return eof_ctext[6];
		}
	}
	return NULL;
}

char * eof_drum_list(int index, int * size)
{
	(void) snprintf(eof_ctext[0], sizeof(eof_ctext[0]) - 1, "Lane 1 (%s)", redefine_index == 0 ? "Press new key" : eof_drums.button[0].name);
	(void) snprintf(eof_ctext[1], sizeof(eof_ctext[1]) - 1, "Lane 2 (%s)", redefine_index == 1 ? "Press new key" : eof_drums.button[1].name);
	(void) snprintf(eof_ctext[2], sizeof(eof_ctext[2]) - 1, "Lane 3 (%s)", redefine_index == 2 ? "Press new key" : eof_drums.button[2].name);
	(void) snprintf(eof_ctext[3], sizeof(eof_ctext[3]) - 1, "Lane 4 (%s)", redefine_index == 3 ? "Press new key" : eof_drums.button[3].name);
	(void) snprintf(eof_ctext[4], sizeof(eof_ctext[4]) - 1, "Lane 5 (%s)", redefine_index == 4 ? "Press new key" : eof_drums.button[4].name);
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
		}
		case 1:
		{
			return eof_ctext[1];
		}
		case 2:
		{
			return eof_ctext[2];
		}
		case 3:
		{
			return eof_ctext[3];
		}
		case 4:
		{
			return eof_ctext[4];
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
		}
		case EOF_DISPLAY_800:
		{
			return "800x600";
		}
		case EOF_DISPLAY_1024:
		{
			return "1024x768";
		}
	}
	return NULL;
}

int eof_controller_settings_guitar(DIALOG * d)
{
	eof_color_dialog(eof_guitar_settings_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_guitar_settings_dialog);
	eof_guitar_settings_dialog[1].d1 = 0;
	(void) snprintf(eof_etext, sizeof(eof_etext) - 1, "%d", eof_guitar.delay);
	if(eof_popup_dialog(eof_guitar_settings_dialog, 0) == 5)
	{
		eof_guitar.delay = atoi(eof_etext);
	}
	eof_render();
	(void) dialog_message(eof_controller_settings_dialog, MSG_DRAW, 0, NULL);
	return 0;
}

int eof_controller_settings_drums(DIALOG * d)
{
	eof_color_dialog(eof_drum_settings_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_drum_settings_dialog);
	eof_drum_settings_dialog[1].d1 = 0;
	(void) snprintf(eof_etext, sizeof(eof_etext) - 1, "%d", eof_drums.delay);
	if(eof_popup_dialog(eof_drum_settings_dialog, 0) == 5)
	{
		eof_drums.delay = atoi(eof_etext);
	}
	eof_render();
	(void) dialog_message(eof_controller_settings_dialog, MSG_DRAW, 0, NULL);
	return 0;
}

int eof_guitar_controller_redefine(DIALOG * d)
{
	int i;
	redefine_index = eof_guitar_settings_dialog[3].d1;
	(void) dialog_message(eof_guitar_settings_dialog, MSG_DRAW, 0, &i);
	eof_controller_set_button(&eof_guitar.button[eof_guitar_settings_dialog[3].d1]);
	eof_clear_input();
	redefine_index = -1;

	//Wait for user to release button 0 or 1 on the first controller to prevent Allegro from hijacking the controller input
	while((joy[0].button[0].b) || (joy[0].button[1].b))
	{
		poll_joystick();
	}

	(void) dialog_message(eof_guitar_settings_dialog, MSG_DRAW, 0, &i);

	if(eof_test_controller_conflict(&eof_guitar,0,6))
		(void) alert("Warning", NULL, "There is a key conflict for this controller", "OK", NULL, 0, KEY_ENTER);

	return 0;
}

int eof_drum_controller_redefine(DIALOG * d)
{
	int i;
	redefine_index = eof_drum_settings_dialog[3].d1;
	(void) dialog_message(eof_drum_settings_dialog, MSG_DRAW, 0, &i);
	eof_controller_set_button(&eof_drums.button[eof_drum_settings_dialog[3].d1]);
	eof_clear_input();
	redefine_index = -1;

	//Wait for user to release button 0 or 1 on the first controller to prevent Allegro from hijacking the controller input
	while((joy[0].button[0].b) || (joy[0].button[1].b))
	{
		poll_joystick();
	}

	(void) dialog_message(eof_drum_settings_dialog, MSG_DRAW, 0, &i);

	if(eof_test_controller_conflict(&eof_drums,0,4))
		(void) alert("Warning", NULL, "There is a key conflict for this controller", "OK", NULL, 0, KEY_ENTER);

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

		(void) snprintf(lyricdetectionstring, sizeof(lyricdetectionstring) - 1, "%s -> %lu Lyrics",ptr->track,ptr->count);	//Write a bounds-checked formatted string to the global lyricdetectionstring array
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

	if(eof_popup_dialog(eof_lyric_detections_dialog, 0) == 2)	//If a user selection was made
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
			if(controller->button[ctr1].key == 0)
				continue;	//Don't test the button if its value is undefined

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

	if(eof_menu_prompt_save_changes() == 3)
	{	//If user canceled closing the current, modified chart
		return 1;
	}
	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	returnedfn = ncd_file_select(0, eof_last_eof_path, "Import Feedback Chart", eof_filter_dB_files);
	eof_clear_input();
	if(returnedfn)
	{
		(void) ustrcpy(eof_filename, returnedfn);
		jumpcode=setjmp(jumpbuffer); //Store environment/stack/etc. info in the jmp_buf array
		if(jumpcode!=0) //if program control returned to the setjmp() call above returning any nonzero value
		{	//Import failed
			(void) puts("Assert() handled sucessfully!");
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
				eof_song = NULL;
				eof_song_loaded = 0;
			}
			eof_destroy_ogg();
			(void) ustrcpy(eof_loaded_song_name, get_filename(eof_filename));
			(void) replace_extension(eof_loaded_song_name, eof_loaded_song_name, "eof", 1024);
			(void) replace_filename(eof_last_eof_path, eof_filename, "", 1024);
			(void) ustrcpy(eof_song_path, eof_last_eof_path);

			/* import chart */
			eof_song = eof_import_chart(returnedfn);
			if(eof_song)
			{
				eof_song_loaded = 1;
				eof_init_after_load(0);
			}
			else
			{
				eof_song_loaded = 0;
				eof_changes = 0;
				eof_fix_window_title();
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
	printflen=snprintf(NULL,0,"\nOffset: %.3f",chart->offset)+1;	//Find the number of characters needed to snprintf this string
	if(printflen > 0)
	{
		tempstr=malloc_err((size_t)printflen);
		(void) snprintf(tempstr, sizeof(tempstr) - 1, "\nOffset: %.3f",chart->offset);
		chartinfo=ResizedAppend(chartinfo,tempstr,1);
		free(tempstr);
	}
	printflen=snprintf(NULL,0,"\nResolution: %lu",chart->resolution)+1;	//Find the number of characters needed to snprintf this string
	if(printflen > 0)
	{
		tempstr=malloc_err((size_t)printflen);
		(void) snprintf(tempstr, sizeof(tempstr) - 1,"\nResolution: %lu",chart->resolution);
		chartinfo=ResizedAppend(chartinfo,tempstr,1);
		free(tempstr);
	}
	printflen=snprintf(NULL,0,"\n%lu Instrument tracks loaded",chart->tracksloaded)+1;	//Find the number of characters needed to snprintf this string
	if(printflen > 0)
	{
		tempstr=malloc_err((size_t)printflen);
		(void) snprintf(tempstr, sizeof(tempstr) - 1,"\n%lu Instrument tracks loaded",chart->tracksloaded);
		chartinfo=ResizedAppend(chartinfo,tempstr,1);
		free(tempstr);
	}

	allegro_message("%s",chartinfo);
}

int eof_mp3_to_ogg(char *file, char *directory)
{
	char syscommand[1024] = {0};
	char cfn[1024] = {0};

	if((file == NULL) || (directory == NULL))
		return 3;	//Return invalid filename

	if(!ustricmp(get_filename(file),"guitar.ogg"))
	{	//If the input file's name is guitar.ogg
		(void) replace_filename(syscommand, file, "", 1024);	//Obtain the parent directory of the input file
		if(!ustricmp(syscommand,directory))				//Special case:  The input and output file are the same
			return 0;									//Return success without altering the existing guitar.ogg
	}

	if(!ustricmp(get_extension(file), "mp3"))
	{
		(void) snprintf(cfn, sizeof(cfn) - 1, "%soriginal.mp3", directory);		//Get the destination path of the original.mp3 to be created

		//If an MP3 is to be encoded to OGG, store a copy of the MP3 as "original.mp3"
		if(ustricmp(syscommand,directory))
		{	//If the user did not select a file named original.mp3 in the chart's folder, check to see if a such-named file will be overwritten
			if(!eof_menu_file_new_supplement(directory, 2))
			{	//If the user declined to overwrite an existing "original.mp3" file at the destination path
				eof_cursor_visible = 1;
				eof_pen_visible = 1;
				eof_show_mouse(NULL);
				return 1;	//Return user declined to overwrite existing files
			}
		}

		if(!eof_soft_cursor)
		{
			if(show_os_cursor(2) < 0)
			{
				eof_soft_cursor = 1;
			}
		}

		if(eof_ogg_settings())
		{
			put_backslash(directory);
			#ifdef ALLEGRO_WINDOWS
				(void) eof_copy_file(file, "eoftemp.mp3");
				(void) uszprintf(syscommand, sizeof(syscommand), "mp3toogg \"eoftemp.mp3\" %s \"%sguitar.ogg\" \"%s\"", eof_ogg_quality[(int)eof_ogg_setting], directory, file);
			#elif defined(ALLEGRO_MACOSX)
				(void) uszprintf(syscommand, sizeof(syscommand), "./lame --decode \"%s\" - | ./oggenc --quiet -q %s --resample 44100 -s 0 - -o \"%sguitar.ogg\"", file, eof_ogg_quality[(int)eof_ogg_setting], directory);
			#else
				(void) uszprintf(syscommand, sizeof(syscommand), "lame --decode \"%s\" - | oggenc --quiet -q %s --resample 44100 -s 0 - -o \"%sguitar.ogg\"", file, eof_ogg_quality[(int)eof_ogg_setting], directory);
			#endif
			(void) eof_system(syscommand);
			#ifdef ALLEGRO_WINDOWS
				(void) delete_file("eoftemp.mp3");
			#endif
			(void) eof_copy_file(file, cfn);	//Copy the selected MP3 file to a file named original.mp3 in the chart folder
		}
		else
		{
			eof_cursor_visible = 1;
			eof_pen_visible = 1;
			eof_show_mouse(NULL);
			return 2;	//Return user canceled conversion
		}
	}

	/* otherwise copy it as-is (assume OGG file) */
	else
	{
		if(!eof_menu_file_new_supplement(directory, 1))
		{	//If the user declined to overwrite an existing "guitar.ogg" file at the destination path
			eof_cursor_visible = 1;
			eof_pen_visible = 1;
			eof_changes = 0;
			eof_show_mouse(NULL);
			return 1;	//Return user declined to overwrite existing files
		}
		(void) ustrcpy(syscommand, directory);
		put_backslash(syscommand);
		(void) ustrcat(syscommand, "guitar.ogg");
		if(ustricmp(file, syscommand))
		{	//If the source and destination file are not the same, copy the file
			(void) eof_copy_file(file, syscommand);
		}
	}

	return 0;	//Return success
}

int eof_new_chart(char * filename)
{
	char year[256] = {0};
	char album[1024] = {0};
	char oggfilename[1024] = {0};
	char * returnedfolder = NULL;
	int ret = 0;
	ALOGG_OGG * temp_ogg = NULL;
	char * temp_buffer = NULL;
	int temp_buffer_size = 0;
	struct ID3Tag tag={NULL,0,0,0,0,0,0.0,NULL,0,NULL,NULL,NULL,NULL};
	int ctr=0;

	if(filename == NULL)
		return 1;	//Return failure

	if(!eof_soft_cursor)
	{
		if(show_os_cursor(2) < 0)
		{
			eof_soft_cursor = 1;
		}
	}

	(void) ustrcpy(oggfilename, filename);
	(void) ustrcpy(eof_last_ogg_path, filename);

	eof_color_dialog(eof_file_new_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_file_new_dialog);
	(void) ustrcpy(eof_etext, "");		//Used to store the Artist tag
	(void) ustrcpy(eof_etext2, "");	//Used to store the Title tag
	(void) ustrcpy(eof_etext3, "");
	(void) ustrcpy(eof_etext4, "");	//Used to store the filename created with %Artist% - %Title%
	eof_render();
	if(!ustricmp("ogg", get_extension(oggfilename)))
	{
		temp_buffer = eof_buffer_file(oggfilename, 0);
		temp_buffer_size = file_size_ex(oggfilename);
		if(temp_buffer)
		{
			temp_ogg = alogg_create_ogg_from_buffer(temp_buffer, temp_buffer_size);
			if(temp_ogg)
			{
				alogg_get_ogg_comment(temp_ogg, "ARTIST", eof_etext);
				alogg_get_ogg_comment(temp_ogg, "TITLE", eof_etext2);
				alogg_get_ogg_comment(temp_ogg, "ALBUM", album);
			}
		}
	}
	else if(!ustricmp("mp3", get_extension(oggfilename)))
	{
		tag.fp=fopen(oggfilename,"rb");	//Open user-specified file for reading
		if(tag.fp != NULL)
		{	//If the file was able to be opened
			year[0]='\0';	//Empty the year string

			if(ID3FrameProcessor(&tag))		//If ID3v2 frames are found
			{
				GrabID3TextFrame(&tag,"TPE1",eof_etext,sizeof(eof_etext)/sizeof(char));		//Store the Artist info in eof_etext[]
				GrabID3TextFrame(&tag,"TIT2",eof_etext2,sizeof(eof_etext2)/sizeof(char));	//Store the Title info in eof_etext2[]
				GrabID3TextFrame(&tag,"TYER",year,sizeof(year)/sizeof(char));				//Store the Year info in year[]
				GrabID3TextFrame(&tag,"TALB",album,sizeof(album)/sizeof(char));			//Store the Album info in album[]
			}

			//If any of the information was not found in the ID3v2 tag, check for it from an ID3v1 tag
			//ID3v1 fields are 30 characters long maximum (31 bytes as a string), while the year field is 4 characters long (5 bytes as a string)
			if(tag.id3v1present > 1)	//If there were fields defined in an ID3v1 tag
			{
				if((eof_etext[0]=='\0') && (tag.id3v1artist != NULL))
					strncpy(eof_etext,tag.id3v1artist,sizeof(eof_etext)/sizeof(char) - 1);
				if((eof_etext2[0]=='\0') && (tag.id3v1title != NULL))
					strncpy(eof_etext2,tag.id3v1title,sizeof(eof_etext2)/sizeof(char) - 1);
				if((year[0]=='\0') && (tag.id3v1year != NULL))
					strncpy(year,tag.id3v1year,sizeof(year)/sizeof(char) - 1);
				if((album[0]=='\0') && (tag.id3v1album != NULL))
					strncpy(album,tag.id3v1album,sizeof(album)/sizeof(char) - 1);
			}

			//Validate year string
			if(strlen(year) != 4)		//If the year string isn't exactly 4 digits
				year[0]='\0';			//Nullify it
			else
				for(ctr=0;ctr<4;ctr++)		//Otherwise check all digits to ensure they're numerical
					if(!isdigit(year[ctr]))	//If it contains a non numerical character
						year[0]='\0';		//Empty the year array

			DestroyID3(&tag);	//Release the list of ID3 frames

			(void) fclose(tag.fp);	//Close file
			tag.fp=NULL;
		}
	}

	/* user fills in song information */
	if(eof_popup_dialog(eof_file_new_dialog, 3) != 6)
	{
		eof_cursor_visible = 1;
		eof_pen_visible = 1;
		eof_show_mouse(NULL);
		return 1;	//Return failure
	}

	if((ustrlen(eof_etext) > 0) && (ustrlen(eof_etext2) > 0))
	{	//If both the artist and song name are provided, use both in the suggested folder name
		(void) snprintf(eof_etext4, sizeof(eof_etext4) - 1, "%s - %s", eof_etext, eof_etext2);
	}
	else if(ustrlen(eof_etext) > 0)
	{
		(void) snprintf(eof_etext4, sizeof(eof_etext4) - 1, "%s", eof_etext);
	}
	else if(ustrlen(eof_etext2) > 0)
	{
		(void) snprintf(eof_etext4, sizeof(eof_etext4) - 1, "%s", eof_etext2);
	}

	/* user selects location for new song */
	eof_color_dialog(eof_file_new_windows_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_file_new_windows_dialog);
	eof_file_new_windows_dialog[1].flags = 0;
	eof_file_new_windows_dialog[2].flags = 0;
	eof_file_new_windows_dialog[3].flags = D_SELECTED;
	if(eof_popup_dialog(eof_file_new_windows_dialog, 4) == 5)
	{	//If the user clicked OK
		if(eof_file_new_windows_dialog[1].flags & D_SELECTED)
		{	//Use Existing Folder
			eof_render();
			returnedfolder = ncd_folder_select("Select Folder for Song");
			if(!returnedfolder)
			{
				eof_cursor_visible = 1;
				eof_pen_visible = 1;
				eof_show_mouse(NULL);
				return 1;	//Return failure
			}
			(void) ustrcpy(eof_etext3, returnedfolder);
		}
		else if(eof_file_new_windows_dialog[2].flags & D_SELECTED)
		{	//Use Source Audio's Folder
			eof_render();
			(void) replace_filename(eof_etext3, filename, "", 1024);
		}
		else
		{	//Create New Folder
			(void) ustrcpy(eof_etext3, eof_songs_path);
			(void) ustrcat(eof_etext3, eof_etext4);
		}
	}
	else
	{
		eof_cursor_visible = 1;
		eof_pen_visible = 1;
		eof_show_mouse(NULL);
		return 1;	//Return failure
	}

	/* if music file is MP3, convert it */
	ret = eof_mp3_to_ogg(oggfilename,eof_etext3);
	if(ret != 0)	//If guitar.ogg was not created successfully
		return ret;	//Return failure

	/* destroy old song */
	if(eof_song)
	{
		eof_destroy_song(eof_song);
		eof_song = NULL;
		eof_song_loaded = 0;
	}
	eof_destroy_ogg();

	/* create new song */
	eof_song = eof_create_song_populated();
	if(!eof_song)
	{
		allegro_message("Error creating new song!");
		eof_cursor_visible = 1;
		eof_pen_visible = 1;
		eof_show_mouse(NULL);
		return 1;	//Return failure
	}

	/* fill in information */
	(void) ustrncpy(eof_song->tags->artist, eof_etext, sizeof(eof_song->tags->artist) - 1);	//Prevent buffer overflow
	(void) ustrncpy(eof_song->tags->title, eof_etext2, sizeof(eof_song->tags->title) - 1);
	(void) ustrncpy(eof_song->tags->frettist, eof_last_frettist, sizeof(eof_song->tags->frettist) - 1);
	(void) ustrncpy(eof_song->tags->year, year, sizeof(eof_song->tags->year) - 1);	//The year tag that was read from an MP3 (if applicable)
	(void) ustrncpy(eof_song->tags->album, album, sizeof(eof_song->tags->album) - 1);
	(void) ustrcpy(oggfilename, filename);
	(void) replace_filename(eof_last_ogg_path, oggfilename, "", 1024);

	/* load OGG */
	(void) ustrcpy(eof_song_path, eof_etext3);
	put_backslash(eof_song_path);
	(void) ustrcpy(eof_last_eof_path, eof_song_path);
	if(temp_ogg)
	{
		eof_music_track = temp_ogg;
		eof_music_data = temp_buffer;
		eof_music_data_size = temp_buffer_size;
		(void) ustrcpy(eof_loaded_ogg_name, eof_etext3);
		put_backslash(eof_loaded_ogg_name);
		(void) ustrcat(eof_loaded_ogg_name, "guitar.ogg");
	}
	else
	{
		(void) ustrcpy(oggfilename, eof_etext3);
		put_backslash(oggfilename);
		(void) ustrcat(oggfilename, "guitar.ogg");
		if(!eof_load_ogg(oggfilename, 0))
		{
			eof_cursor_visible = 1;
			eof_pen_visible = 1;
			eof_show_mouse(NULL);
			return 1;	//Return failure
		}
	}
	eof_music_length = alogg_get_length_msecs_ogg(eof_music_track);
	(void) ustrcpy(eof_loaded_song_name, "notes.eof");

	eof_song_loaded = 1;

	/* get ready to edit */
	eof_init_after_load(0);	//Initialize variables
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(NULL);

	eof_menu_file_quick_save();

	return 0;	//Return success
}

int eof_save_helper(char *destfilename)
{
	unsigned long ctr, ctr2, notes_after_chart_audio;
	char newfolderpath[1024] = {0};
	char oggfn[1024] = {0};
	char function;		//Will be set to 1 for "Save" or 2 for "Save as"
	int jumpcode = 0;
	char fixvoxpitches, fixvoxphrases;
	char note_length_warned = 0, note_distance_warned = 0;

	if(!eof_song_loaded || !eof_song)
		return 1;	//Return failure

//	eof_log_level &= ~2;	//Disable verbose logging

	if(destfilename == NULL)
	{	//Perform save
		function = 1;
		if((eof_song_path == NULL) || (eof_loaded_song_name == NULL))
			return 1;	//Return failure
		(void) append_filename(eof_temp_filename, eof_song_path, eof_loaded_song_name, 1024);
		(void) replace_filename(newfolderpath, eof_song_path, "", 1024);	//Obtain the destination path
	}
	else
	{	//Perform save as
		function = 2;
		(void) replace_extension(destfilename, destfilename, "eof", 1024);	//Ensure the chart is saved with a .eof extension
		(void) ustrncpy(eof_temp_filename, destfilename, 1024 - 1);
		if(eof_temp_filename[1022] != '\0')	//If the source filename was too long to store in the array
			return 1;			//Return failure
		(void) replace_filename(newfolderpath, destfilename, "", 1024);	//Obtain the destination path
	}

	/* sort notes so they are in order of position */
	eof_sort_notes(eof_song);
	eof_fixup_notes(eof_song);

	/* check if there are any notes beyond the chart audio */
	notes_after_chart_audio = eof_check_if_notes_exist_beyond_audio_end(eof_song);
	if(notes_after_chart_audio && !eof_silence_loaded)
	{	//Only display this warning if there is chart audio loaded
		snprintf(oggfn, sizeof(oggfn) - 1, "Warning:  Track \"%s\" contains notes/lyrics extending beyond the chart's audio.", eof_song->track[notes_after_chart_audio]->name);
		if(alert(oggfn, NULL, "This chart may not work properly.  Continue?", "&Yes", "&No", 'y', 'n') != 1)
		{	//If the user doesn't opt to continue due to this error condition
			return 2;	//Return cancellation
		}
	}

	/* prepare lyrics if applicable */
	if(eof_song->vocal_track[0]->lyrics > 0)
	{
		/* sort and validate lyrics */
		eof_track_sort_notes(eof_song, EOF_TRACK_VOCALS);
		eof_track_fixup_notes(eof_song, EOF_TRACK_VOCALS, 0);

		/* pre-parse the lyrics to determine if any of them are not contained within a lyric phrase */
		for(ctr = 0; ctr < eof_song->vocal_track[0]->lyrics; ctr++)
		{
			if((eof_song->vocal_track[0]->lyric[ctr]->note != EOF_LYRIC_PERCUSSION) && (eof_find_lyric_line(ctr) == NULL))
			{	//If any of the non vocal percussion lyrics are not within a line
				eof_cursor_visible = 0;
				eof_pen_visible = 0;
				eof_show_mouse(screen);
				if(alert("Warning: One or more lyrics aren't within lyric phrases.", "These lyrics won't export to FoF script format.", "Continue?", "&Yes", "&No", 'y', 'n') == 2)
				{	//If user opts cancel the save
					eof_show_mouse(NULL);
					eof_cursor_visible = 1;
					eof_pen_visible = 1;
					return 2;	//Return cancellation
				}
				break;
			}
		}
	}

	/* check 5 lane guitar note lengths */
	for(ctr = 1; !note_length_warned && eof_min_note_length && (ctr < eof_song->tracks); ctr++)
	{	//For each track (only check if the user defined a minimum length, and only if the user didn't already decline to cancel when an offending note was found)
		if((eof_song->track[ctr]->track_behavior == EOF_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[ctr]->track_format == EOF_LEGACY_TRACK_FORMAT))
		{	//If this is a 5 lane guitar track
			for(ctr2 = 0; ctr2 < eof_get_track_size(eof_song, ctr); ctr2++)
			{	//For each note in the track
				if(eof_get_note_length(eof_song, ctr, ctr2) < eof_min_note_length)
				{	//If this note's length is shorter than the minimum length
					if(alert("Warning:  At least one note was truncated shorter", "than your defined minimum length.", "Cancel save and seek to the first such note?", "&Yes", "&No", 'y', 'n') == 1)
					{	//If the user opted to seek to the first offending note (only prompt once per call)
						(void) eof_menu_track_selected_track_number(ctr);										//Set the active instrument track
						eof_note_type = eof_get_note_type(eof_song, ctr, ctr2);							//Set the active difficulty to match that of the note
						eof_set_seek_position(eof_get_note_pos(eof_song, ctr, ctr2) + eof_av_delay);	//Seek to the note's position
						return 2;	//Return cancellation
					}
					note_length_warned = 1;
					break;	//Stop checking after the first offending note is found
				}
			}
		}
	}

	/* check note distances */
	for(ctr = 1; !note_distance_warned && (ctr < eof_song->tracks); ctr++)
	{	//For each track (and only if the user didn't already decline to cancel when an offending note was found)
		for(ctr2 = 0; ctr2 < eof_get_track_size(eof_song, ctr); ctr2++)
		{	//For each note in the track
			long next = eof_fixup_next_note(eof_song, ctr, ctr2);	//Get the next note, if it exists
			unsigned long maxlength = eof_get_note_max_length(eof_song, ctr, ctr2);	//Get the maximum length of this note
			if(next > 0)
			{	//If there was a next note
				if(eof_get_note_length(eof_song, ctr, ctr2) > maxlength)
				{	//And this note is longer than its maximum length
					if(eof_get_note_pos(eof_song, ctr, next) - eof_get_note_pos(eof_song, ctr, ctr2) < eof_min_note_distance)
					{	//If the notes are too close to enforce the minimum note distance
						if(alert("Warning:  At least one note is too close to another", "to enforce the minimum note distance.", "Cancel save and seek to the first such note?", "&Yes", "&No", 'y', 'n') == 1)
						{	//If the user opted to seek to the first offending note (only prompt once per call)
							(void) eof_menu_track_selected_track_number(ctr);										//Set the active instrument track
							eof_note_type = eof_get_note_type(eof_song, ctr, ctr2);							//Set the active difficulty to match that of the note
							eof_set_seek_position(eof_get_note_pos(eof_song, ctr, ctr2) + eof_av_delay);	//Seek to the note's position
							return 2;	//Return cancellation
						}
						note_distance_warned = 1;
						break;	//Stop checking after the first offending note is found
					}
				}
			}
		}
	}

	/* check if any chords have undefined fingering */
	if(eof_write_rs_files)
	{	//If the user wants to save Rocksmith capable files
		eof_correct_chord_fingerings();			//Ensure all chords in each pro guitar track have valid finger arrays, prompt user to provide any that are missing
	}

	/* check if there is a MIDI delay, offer to use Reset offset to zero */
	if(eof_write_rs_files)
	{	//If the user wants to save Rocksmith capable files
		if(eof_song->beat[0]->pos > 0)
		{	//If there is a MIDI delay
			if(alert("Warning:  The first beat marker (the MIDI delay) is not positioned at 0 seconds.", "This might prevent the song from playing from the beginning in Rocksmith.", "Correct this condition with \"Reset offset to zero\"?", "&Yes", "&No", 'y', 'n') == 1)
			{	//If the user opts to correct the issue
				eof_menu_beat_reset_offset();	//Run the "Reset offset to zero" function.  If the tempo map is locked, the function will offer to unlock it before proceeding
			}
		}
	}

	/* check if any Rocksmith sections don't have a Rocksmith phrase at the same position */
	if(eof_write_rs_files)
	{
		for(ctr = 1; ctr < eof_song->tracks; ctr++)
		{	//For each track
			if(eof_song->track[ctr]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
			{	//If this is a pro guitar/bass track
				if(eof_check_rs_sections_have_phrases(eof_song, ctr))
				{	//If the user canceled adding missing phrases
					break;	//Stop fixing them and break from loop
				}
			}
		}
	}

	/* save the chart */
	(void) replace_extension(eof_temp_filename, eof_temp_filename, "eof", 1024);	//Ensure the chart is saved with a .eof extension
	eof_song->tags->revision++;
	if(!eof_save_song(eof_song, eof_temp_filename))
	{
		allegro_message("Could not save song!");
		eof_show_mouse(NULL);
		eof_cursor_visible = 1;
		eof_pen_visible = 1;
		return 1;	//Return failure
	}
	(void) ustrcpy(eof_loaded_song_name, get_filename(eof_temp_filename));

	/* save the MIDI, INI and other files*/
	eof_check_vocals(eof_song, &fixvoxpitches, &fixvoxphrases);
	(void) append_filename(eof_temp_filename, newfolderpath, "notes.mid", 1024);
	if(eof_export_midi(eof_song, eof_temp_filename, 0, fixvoxpitches, fixvoxphrases))
	{	//If saving the normal MIDI succeeded, proceed with saving song.ini and additional files if applicable
		if(eof_write_rb_files)
		{	//If the user opted to also save RBN2 and RB3 pro guitar upgrade compliant MIDIs
			(void) append_filename(eof_temp_filename, newfolderpath, "notes_rbn.mid", 1024);
			(void) eof_export_midi(eof_song, eof_temp_filename, 1, fixvoxpitches, fixvoxphrases);	//Write a RBN2 compliant MIDI
			if(eof_get_track_size(eof_song, EOF_TRACK_PRO_BASS) || eof_get_track_size(eof_song, EOF_TRACK_PRO_BASS_22) || eof_get_track_size(eof_song, EOF_TRACK_PRO_GUITAR) || eof_get_track_size(eof_song, EOF_TRACK_PRO_GUITAR_22))
			{	//If any of the pro guitar tracks are populated
				(void) append_filename(eof_temp_filename, newfolderpath, "notes_pro.mid", 1024);
				(void) eof_export_midi(eof_song, eof_temp_filename, 2, fixvoxpitches, fixvoxphrases);	//Write a RB3 pro guitar upgrade compliant MIDI
				(void) ustrcpy(eof_temp_filename, newfolderpath);
				put_backslash(eof_temp_filename);
				(void) ustrcat(eof_temp_filename, "songs_upgrades");
				if(!file_exists(eof_temp_filename, FA_DIREC | FA_HIDDEN, NULL))
				{	//If the songs_upgrades folder doesn't already exist
					if(eof_mkdir(eof_temp_filename))
					{	//And it couldn't be created
						allegro_message("Could not create folder!\n%s", eof_temp_filename);
						return 1;	//Return failure
					}
				}
				put_backslash(eof_temp_filename);
				(void) replace_filename(eof_temp_filename, eof_temp_filename, "upgrades.dta", 1024);
				eof_save_upgrades_dta(eof_song, eof_temp_filename);		//Create the upgrades.dta file in the songs_upgrades folder if it does not already exist
			}
		}

		if(eof_write_rs_files)
		{	//If the user wants to save Rocksmith capable files
			char user_warned = 0;	//Tracks whether the user was warned about hand positions being undefined and auto-generated during export
			(void) append_filename(eof_temp_filename, newfolderpath, "xmlpath.xml", 1024);	//Re-acquire the save's target folder

			eof_export_rocksmith_track(eof_song, eof_temp_filename, EOF_TRACK_PRO_BASS, &user_warned);
			eof_export_rocksmith_track(eof_song, eof_temp_filename, EOF_TRACK_PRO_BASS_22, &user_warned);
			eof_export_rocksmith_track(eof_song, eof_temp_filename, EOF_TRACK_PRO_GUITAR, &user_warned);
			eof_export_rocksmith_track(eof_song, eof_temp_filename, EOF_TRACK_PRO_GUITAR_22, &user_warned);
			if(eof_song->vocal_track[0]->lyrics)
			{	//If there are lyrics, export them in Rocksmith format as well
				char *arrangement_name;	//This will point to the track's native name unless it has an alternate name defined
				if((eof_song->track[EOF_TRACK_VOCALS]->flags & EOF_TRACK_FLAG_ALT_NAME) && (eof_song->track[EOF_TRACK_VOCALS]->altname[0] != '\0'))
				{	//If the vocal track has an alternate name
					arrangement_name = eof_song->track[EOF_TRACK_VOCALS]->altname;
				}
				else
				{	//Otherwise use the track's native name
					arrangement_name = eof_song->track[EOF_TRACK_VOCALS]->name;
				}
				(void) append_filename(eof_temp_filename, newfolderpath, arrangement_name, 1024);
				(void) replace_extension(eof_temp_filename, eof_temp_filename, "xml", 1024);
				jumpcode=setjmp(jumpbuffer); //Store environment/stack/etc. info in the jmp_buf array
				if(jumpcode!=0) //if program control returned to the setjmp() call above returning any nonzero value
				{	//Lyric export failed
					(void) puts("Assert() handled sucessfully!");
					allegro_message("Rocksmith lyric export failed");
				}
				else
				{
					EOF_EXPORT_TO_LC(eof_song->vocal_track[0],eof_temp_filename,NULL,RS_FORMAT);	//Import lyrics into FLC lyrics structure and export to script format
				}
			}
			eof_detect_difficulties(eof_song, eof_selected_track);		//Update eof_track_diff_populated_status[] to reflect the currently selected difficulty
			eof_process_beat_statistics(eof_song, eof_selected_track);	//Cache section name information into the beat structures (from the perspective of the active track)

			//Determine if "[song name].wav" exists, if not, export the chart audio in WAV format
			eof_get_rocksmith_wav_path(eof_temp_filename, newfolderpath, sizeof(eof_temp_filename));	//Build the path to the target WAV file
			if(!exists(eof_temp_filename) && !eof_silence_loaded)
			{	//If the WAV file does not exist, and chart audio is loaded
				set_window_title("Saving WAV file for use with Wwise.  Please wait.");
				SAMPLE *decoded = alogg_create_sample_from_ogg(eof_music_track);	//Create PCM data from the loaded chart audio
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Writing RS WAV file (%s)", eof_temp_filename);
				eof_log(eof_log_string, 1);
				save_wav_with_silence_appended(eof_temp_filename, decoded, 8000);	//Write a WAV file with it, appending 8 seconds of silence to it
				eof_fix_window_title();
			}
		}

		/* Save INI file */
		(void) append_filename(eof_temp_filename, newfolderpath, "song.ini", 1024);
		(void) eof_save_ini(eof_song, eof_temp_filename);

		/* save script lyrics if applicable) */
		if(eof_song->tags->lyrics && eof_song->vocal_track[0]->lyrics)							//If user enabled the Lyrics checkbox in song properties and there are lyrics defined
		{
			(void) append_filename(eof_temp_filename, newfolderpath, "script.txt", 1024);
			jumpcode=setjmp(jumpbuffer); //Store environment/stack/etc. info in the jmp_buf array
			if(jumpcode!=0) //if program control returned to the setjmp() call above returning any nonzero value
			{	//Lyric export failed
				(void) puts("Assert() handled sucessfully!");
				allegro_message("Lyric export failed");
			}
			else
			{
				EOF_EXPORT_TO_LC(eof_song->vocal_track[0],eof_temp_filename,NULL,SCRIPT_FORMAT);	//Import lyrics into FLC lyrics structure and export to script format
			}
		}
	}

	/* save OGG file if necessary*/
	if(!eof_silence_loaded)
	{	//Only try to save an audio file if one is loaded
		(void) append_filename(eof_temp_filename, newfolderpath, "guitar.ogg", 1024);
		if(function == 1)
		{	//If performing "Save" function, only write guitar.ogg if it is missing
			if(!exists(eof_temp_filename))
				eof_save_ogg(eof_temp_filename);
		}
		else	//"Save as" requires the guitar.ogg to be overwritten to ensure it's the correct audio
			eof_save_ogg(eof_temp_filename);
	}

	/* finish up */
	eof_changes = 0;
	for(ctr = 0; ctr < eof_song->tags->oggs; ctr++)
	{
		if(eof_song->tags->ogg[ctr].modified)
		{
			(void) replace_filename(eof_temp_filename, eof_temp_filename, eof_song->tags->ogg[ctr].filename, 1024);
			(void) snprintf(oggfn, sizeof(oggfn) - 1, "%s.lastsaved", eof_temp_filename);
			(void) eof_copy_file(eof_temp_filename, oggfn);
			eof_song->tags->ogg[ctr].modified = 0;
		}
	}
	eof_undo_last_type = 0;
	eof_change_count = 0;
	eof_fix_window_title();
	eof_process_beat_statistics(eof_song, eof_selected_track);	//Re-cache beat information (from the perspective of the active track), since temporary MIDI events may have been added/removed and can cause cached section event numbers to be invalid

	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;

//	eof_log_level |= 2;	//Enable verbose logging
	eof_log("\tSave completed", 1);

	return 0;	//Return success
}

void eof_restore_oggs_helper(void)
{
	char oggfn[1024] = {0};
	char coggfn[1024] = {0};
	int i;

	/* see if we need to restore any OGGs before quitting */
	if(eof_song == NULL)
		return;

	if(eof_changes)
	{
		for(i = 0; i < eof_song->tags->oggs; i++)
		{
			if(eof_song->tags->ogg[i].modified)
			{
				(void) snprintf(coggfn, sizeof(coggfn) - 1, "%s%s", eof_song_path, eof_song->tags->ogg[i].filename);
				(void) snprintf(oggfn, sizeof(oggfn) - 1, "%s.lastsaved", coggfn);
				if(exists(oggfn))
				{
					printf("restoring from .lastsaved\n");
					(void) eof_copy_file(oggfn, coggfn);
					(void) delete_file(oggfn);
				}
				else
				{
					(void) snprintf(oggfn, sizeof(oggfn) - 1, "%s.backup", coggfn);
					if(exists(oggfn))
					{
						printf("restoring from .backup\n");
						(void) eof_copy_file(oggfn, coggfn);
					}
				}
			}

			/* clean up copies */
			(void) snprintf(oggfn, sizeof(oggfn) - 1, "%s.lastsaved", coggfn);
			if(exists(oggfn))
			{
				(void) delete_file(oggfn);
			}
		}
	}
}

int eof_menu_prompt_save_changes(void)
{
	int ret = 0;

	if(!eof_song_loaded || !eof_changes)
	{	//There are no changes or a song isn't loaded
		return 0;
	}

	if(!eof_music_paused)
	{
		eof_music_play();
	}
	ret = alert3(NULL, "You have unsaved changes.", NULL, "Save", "Discard", "Cancel", 0, 0, 0);
	if(ret == 1)
	{
		eof_menu_file_save();
	}
	else
	{
		eof_restore_oggs_helper();
	}
	eof_clear_input();

	return ret;
}

int eof_menu_file_gh_import(void)
{
	char * returnedfn = NULL;

	if(eof_menu_prompt_save_changes() == 3)
	{	//If user canceled closing the current, modified chart
		return 1;
	}
	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	returnedfn = ncd_file_select(0, eof_last_eof_path, "Import GH Chart", eof_filter_gh_files);
	eof_clear_input();
	if(returnedfn)
	{
		(void) ustrcpy(eof_filename, returnedfn);
		/* destroy loaded song */
		if(eof_song)
		{
			eof_destroy_song(eof_song);
			eof_song = NULL;
			eof_song_loaded = 0;
		}
		eof_destroy_ogg();
		(void) ustrcpy(eof_loaded_song_name, get_filename(eof_filename));
		(void) replace_extension(eof_loaded_song_name, eof_loaded_song_name, "eof", 1024);
		(void) replace_filename(eof_last_eof_path, eof_filename, "", 1024);
		(void) ustrcpy(eof_song_path, eof_last_eof_path);

		/* import chart */
		eof_song = eof_import_gh(returnedfn);
		if(eof_song)
		{
			eof_song_loaded = 1;
			eof_init_after_load(0);
		}
		else
		{
			allegro_message("Could not import file!");
			eof_song_loaded = 0;
			eof_changes = 0;
			eof_fix_window_title();
		}
	}
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	return 1;
}

char * eof_colors_list(int index, int * size)
{
	switch(index)
	{
		case -1:
		{
			*size = EOF_NUM_COLOR_SETS;
			break;
		}
		case 0:
		{
			return "Default";
		}
		case 1:
		{
			return "Rock Band";
		}
		case 2:
		{
			return "Guitar Hero";
		}
		case 3:
		{
			return "Rocksmith";
		}
	}
	return NULL;
}

struct eof_guitar_pro_struct *eof_parsed_gp_file;

DIALOG eof_gp_import_dialog[] =
{
   /* (proc)            (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)            (dp2) (dp3) */
   { d_agup_window_proc,0,   48,  500, 232, 2,   23,  0,    0,      0,   0,   "Import which GP track into the project's active track?",       NULL, NULL },
   { d_agup_list_proc,  12,  84,  400, 138, 2,   23,  0,    0,      0,   0,   eof_gp_tracks_list, NULL, NULL },
   { d_agup_push_proc,  425, 84,  68,  28,  2,   23,  'i',  D_EXIT, 0,   0,   "&Import",      NULL, eof_gp_import_track },
   { d_agup_button_proc,12,  235, 240, 28,  2,   23,  '\r', D_EXIT, 0,   0,   "Done",         NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

int eof_gp_import_track(DIALOG * d)
{
	unsigned long ctr, selected;
	int junk;
	if(!d || d->d1 >= eof_parsed_gp_file->numtracks)
		return 0;

	if(eof_song && eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
	{	//Only perform this action if a pro guitar/bass track is active
		unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
		selected = eof_gp_import_dialog[1].d1;

		if(eof_get_track_size(eof_song, eof_selected_track) && alert("This track already has notes", "Importing this GP track will overwrite this track's contents", "Continue?", "&Yes", "&No", 'y', 'n') != 1)
		{	//If the active track is already populated and the user doesn't opt to overwrite it
			return 0;
		}

		if(!gp_import_undo_made)
		{	//If an undo state wasn't already made
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make one now
			gp_import_undo_made = 1;
		}
		eof_parsed_gp_file->track[selected]->parent = eof_song->pro_guitar_track[tracknum]->parent;
		for(ctr = 0; ctr < eof_song->pro_guitar_track[tracknum]->notes; ctr++)
		{	//For each note in the active track
			free(eof_song->pro_guitar_track[tracknum]->note[ctr]);	//Free its memory
		}
		free(eof_song->pro_guitar_track[tracknum]);	//Free the active track
		eof_song->pro_guitar_track[tracknum] = eof_parsed_gp_file->track[selected];	//Replace it with the track imported from the Guitar Pro file
		free(eof_parsed_gp_file->names[selected]);	//Free the imported track's name
		for(ctr = selected + 1; ctr < eof_parsed_gp_file->numtracks; ctr++)
		{	//For all remaining tracks imported from the Guitar Pro file
			eof_parsed_gp_file->track[ctr - 1] = eof_parsed_gp_file->track[ctr];	//Save this track into the previous track's index
			eof_parsed_gp_file->names[ctr - 1] = eof_parsed_gp_file->names[ctr];	//Save this track's name into the previous track name's index
		}
		eof_parsed_gp_file->numtracks--;
		(void) dialog_message(eof_gp_import_dialog, MSG_DRAW, 0, &junk);	//Redraw the dialog since the list's contents have changed

		//Correct the track's tuning array from absolute to relative
		for(ctr = 0; ctr < 6; ctr++)
		{	//For each of the 6 supported strings
			int default_tuning = eof_lookup_default_string_tuning_absolute(eof_song->pro_guitar_track[tracknum], eof_selected_track, ctr);	//Get the default absolute tuning for this string
			if(default_tuning >= 0)
			{	//If the default tuning was found
				eof_song->pro_guitar_track[tracknum]->tuning[ctr] -= default_tuning;	//Convert the tuning to relative
				eof_song->pro_guitar_track[tracknum]->tuning[ctr] %= 12;	//Ensure the stored value is bounded to [-11,11]
			}
		}
	}

	return D_O_K;
}

char gp_import_undo_made;
int eof_menu_file_gp_import(void)
{
	char * returnedfn = NULL;
	unsigned long ctr, ctr2;

	if(!eof_song || !eof_song_loaded)
		return 1;	//For now, don't do anything unless a project is active

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Don't do anything unless the active track is a pro guitar/bass track

	gp_import_undo_made = 0;	//Will ensure only one undo state is created during this import
	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	returnedfn = ncd_file_select(0, eof_last_eof_path, "Import Guitar Pro", eof_filter_gp_files);
	eof_clear_input();
	if(returnedfn)
	{
		eof_parsed_gp_file = eof_load_gp(returnedfn, &gp_import_undo_made);	//Parse the GP file, make an undo state if time signatures are imported

		if(eof_parsed_gp_file)
		{	//The file was successfully parsed, allow the user to import a track into the active project
			eof_clear_input();
			eof_cursor_visible = 0;
			eof_render();

//If the GP file contained section markers, offer to import them now
			if(eof_parsed_gp_file->text_events)
			{	//If there were text events imported
				if(alert(NULL, "Import Guitar Pro file's section markers/beat text as Rocksmith phrases/sections?", NULL, "&Yes", "&No", 'y', 'n') == 1)
				{	//If the user opts to import RS phrases and sections from GP files
					if(!gp_import_undo_made)
					{	//If an undo state hasn't been made yet
						eof_prepare_undo(EOF_UNDO_TYPE_NONE);
						gp_import_undo_made = 1;
					}
					for(ctr = 0; ctr < eof_parsed_gp_file->text_events; ctr++)
					{	//For each of the text events
						(void) eof_song_add_text_event(eof_song, eof_parsed_gp_file->text_event[ctr]->beat, eof_parsed_gp_file->text_event[ctr]->text, 0, eof_parsed_gp_file->text_event[ctr]->flags, 0);	//Add the event to the active project
					}
				}
				for(ctr = 0; ctr < eof_parsed_gp_file->text_events; ctr++)
				{	//For each text event parsed from the Guitar Pro file
					free(eof_parsed_gp_file->text_event[ctr]);	//Free the event
				}
				eof_parsed_gp_file->text_events = 0;
			}

			eof_color_dialog(eof_gp_import_dialog, gui_fg_color, gui_bg_color);
			centre_dialog(eof_gp_import_dialog);
			(void) eof_popup_dialog(eof_gp_import_dialog, 0);	//Launch the dialog to allow the user to import a track
			eof_cursor_visible = 1;
			eof_pen_visible = 1;
			eof_show_mouse(NULL);

//Release the Guitar Pro structure's memory
			for(ctr = 0; ctr < eof_parsed_gp_file->numtracks; ctr++)
			{	//For each track parsed from the Guitar Pro file
				free(eof_parsed_gp_file->names[ctr]);	//Free the track name string
				for(ctr2 = 0; ctr2 < eof_parsed_gp_file->track[ctr]->notes; ctr2++)
				{	//For each note in the track
					free(eof_parsed_gp_file->track[ctr]->note[ctr2]);	//Free its memory
				}
				free(eof_parsed_gp_file->track[ctr]);	//Free the pro guitar track
			}
			free(eof_parsed_gp_file->names);
			free(eof_parsed_gp_file->track);
			free(eof_parsed_gp_file);
		}
		else
		{
			allegro_message("Failure");
		}

		eof_truncate_chart(eof_song);	//Remove excess beat markers and update the eof_chart_length variable
		eof_beat_stats_cached = 0;		//Mark the cached beat stats as not current
		eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Run fixup logic to clean up the track
		(void) eof_menu_track_selected_track_number(eof_selected_track);	//Re-select the active track to allow for a change in string count
	}
	return 1;
}

char * eof_gp_tracks_list(int index, int * size)
{
	if(index < 0)
	{	//Signal to return the list count
		if(eof_parsed_gp_file)
		{
			*size = eof_parsed_gp_file->numtracks;
		}
		else
		{	//eof_parsed_gp_file pointer is not valid
			*size = 0;
		}
	}
	else
	{	//Return the specified list item
		if(eof_parsed_gp_file && index < eof_parsed_gp_file->numtracks)
		{	//eof_parsed_gp_file and the referenced track index are valid
			return eof_parsed_gp_file->names[index];
		}
	}
	return NULL;
}

DIALOG eof_set_display_width_dialog[] =
{
   /* (proc)                (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags)   (d1) (d2) (dp)                 (dp2) (dp3) */
   { d_agup_window_proc,    0,   48,  214, 106, 2,   23,   0,      0,      0,   0,   "Set display width",NULL, NULL },
   { d_agup_text_proc,      12,  84,  64,  8,   2,   23,   0,      0,      0,   0,   "Width:",           NULL, NULL },
   { eof_verified_edit_proc,60,  80,  100, 20,  2,   23,   0,      0,      4,   0,   eof_etext2,         "0123456789", NULL },
   { d_agup_button_proc,    17,  112, 84,  28,  2,   23,   '\r',   D_EXIT, 0,   0,   "OK",               NULL, NULL },
   { d_agup_button_proc,    113, 112, 78,  28,  2,   23,   0,      D_EXIT, 0,   0,   "Cancel",           NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

int eof_set_display_width(void)
{
	int width;

	snprintf(eof_etext, sizeof(eof_etext) - 1, "Set display width (>= %lu)", eof_screen_width_default);
	eof_set_display_width_dialog[0].dp = eof_etext;	//Update the dialog window's title

	eof_etext2[0] = '\0';
	eof_color_dialog(eof_set_display_width_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_set_display_width_dialog);
	if(eof_popup_dialog(eof_set_display_width_dialog, 2) == 3)	//User hit OK
	{
		if(eof_etext2[0] != '\0')
		{	//If a width was specified
			width = atol(eof_etext2);

			if(width >= eof_screen_width_default)
			{	//If the specified width is valid
				eof_set_display_mode(width, eof_screen_height);	//Resize the program window to the specified width and the current height
			}
		}
	}
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(screen);

	//Update coordinate related items
	eof_scale_fretboard(0);			//Recalculate the 2D screen positioning based on the current track
	eof_set_2D_lane_positions(0);	//Update ychart[] by force just in case the display window size was changed
	eof_set_3D_lane_positions(0);	//Update xchart[] by force just in case the display window size was changed

	return D_O_K;
}
