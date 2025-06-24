#include <allegro.h>
#include <ctype.h>
#include <time.h>
#include "../agup/agup.h"
#include "../main.h"
#include "../foflc/Lyric_storage.h"
#include "../foflc/ID3_parse.h"
#include "../foflc/RS_parse.h"	//For XML parsing functions
#include "../gh_import.h"
#include "../gp_import.h"
#include "../dialog/proc.h"
#include "../bf.h"
#include "../bf_import.h"
#include "../chart_import.h"
#include "../config.h"	//For eof_lookup_drum_mapping()
#include "../dialog.h"
#include "../dr.h"
#include "../ini.h"
#include "../ini_import.h"
#include "../ir.h"
#include "../lc_import.h"
#include "../midi_import.h"
#include "../main.h"	//For eof_import_to_track_dialog[] declaration
#include "../midi.h"
#include "../mix.h"
#include "../player.h"
#include "../rs.h"
#include "../rs_import.h"
#include "../silence.h"	//For save_wav_with_silence_appended
#include "../song.h"
#include "../tuning.h"
#include "../undo.h"
#include "../utility.h"
#include "beat.h"	//For eof_menu_beat_reset_offset()
#include "edit.h"	//For eof_menu_edit_undo()
#include "file.h"
#include "note.h"	//For eof_correct_chord_fingerings()
#include "song.h"
#include "track.h"	//For tone name functions
#include "help.h"	//For eof_reset_display()

#ifdef USEMEMWATCH
#include "../memwatch.h"
#endif

struct Lyric_Format *lyricdetectionlist;	//Dialog windows cannot be passed local variables, requiring the use of this global variable for the lyric track prompt dialog
char lyricdetectionstring[1024] = {0};		//The display name given to the detection when displayed in the list box
static int redefine_index = -1;

char eof_file_notes_panel_menu_string[1024];
MENU eof_file_notes_panel_menu[] =
{
	{"&Notes", eof_menu_file_notes_panel_notes, NULL, 0, NULL},
	{"N&Ote controls", eof_menu_file_notes_panel_note_controls, NULL, 0, NULL},
	{"In&Formation panel", eof_menu_file_notes_panel_information, NULL, 0, NULL},
	{"Note &Counts", eof_menu_file_notes_panel_note_counts, NULL, 0, NULL},
	{"&Rocksmith", eof_menu_file_notes_panel_rocksmith, NULL, 0, NULL},
	{"&IMMERROCK", eof_menu_file_notes_panel_immerrock, NULL, 0, NULL},
	{"Clone &Hero", eof_menu_file_notes_panel_clone_hero, NULL, 0, NULL},
	{eof_file_notes_panel_menu_string, eof_menu_file_notes_panel_user, NULL, 0, NULL},
	{"&Browse", eof_menu_file_notes_panel_browse, NULL, 0, NULL},
	{"", NULL, NULL, 0, NULL},
	{"&Enable\t" CTRL_NAME "+P", eof_display_notes_panel, NULL, D_USER, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_file_3d_preview_menu[] =
{
	{"Set &HOPO image scale size", eof_set_3d_hopo_scale_size, NULL, 0, NULL},
	{"Set &Camera angle", eof_set_3d_camera_angle, NULL, 0, NULL},
	{"&Full height", eof_3d_preview_toggle_full_height, NULL, D_USER, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_file_display_menu[] =
{
	{"&Notes panel", NULL, eof_file_notes_panel_menu, 0, NULL},
	{"&3D preview", NULL, eof_file_3d_preview_menu, 0, NULL},
	{"&Display", eof_menu_file_display, NULL, 0, NULL},
	{"Set display &Width", eof_set_display_width, NULL, 0, NULL},
	{"x2 &Zoom", eof_toggle_display_zoom, NULL, D_USER, NULL},
	{"Benchmark image sequence", eof_benchmark_image_sequence, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_file_import_menu[] =
{
	{"&Sonic visualiser", eof_menu_file_sonic_visualiser_import, NULL, 0, NULL},
	{"&MIDI\tF6", eof_menu_file_midi_import, NULL, 0, NULL},
	{"&Feedback (.chart)", eof_menu_file_feedback_import, NULL, 0, NULL},
	{"Guitar &Hero", eof_menu_file_gh_import, NULL, 0, NULL},
	{"&Lyric\tShift+F8", eof_menu_file_lyrics_import, NULL, 0, NULL},
	{"&Guitar Pro\tF12", eof_menu_file_gp_import, NULL, 0, NULL},
	{"&Rocksmith\tF7", eof_menu_file_rs_import, NULL, 0, NULL},
	{"&Bandfuse", eof_menu_file_bf_import, NULL, 0, NULL},
	{"&Queen Bee", eof_menu_file_array_txt_import, NULL, 0, NULL},
	{"Queen Bee (multi)", eof_menu_file_multiple_array_txt_import, NULL, 0, NULL},
	{"Guitar Hero sections", eof_menu_file_gh3_section_import, NULL, 0, NULL},
	{"Guitar Hero Live", eof_menu_file_ghl_import, NULL, 0, NULL},
	{"&Drums Rock", eof_menu_file_drums_rock_import, NULL, 0, NULL},
	{"GP style lyric text", eof_note_menu_read_gp_lyric_texts, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_file_preferences_menu[] =
{
	{"&Preferences\tF11", eof_menu_file_preferences, NULL, 0, NULL},
	{"&Import/Export", eof_menu_file_import_export_preferences, NULL, 0, NULL},
	{"Pro &Guitar", eof_menu_file_gp_preferences, NULL, 0, NULL},
	{"Drum MIDI &Velocities", eof_menu_file_drum_midi_velocities, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_file_export_menu[] =
{
	{"&Chart range", eof_menu_file_export_chart_range, NULL, D_DISABLED, NULL},
	{"&Audio range", eof_menu_file_export_audio_range, NULL, D_DISABLED, NULL},
	{"&Guitar pro", eof_menu_file_export_guitar_pro, NULL, D_DISABLED, NULL},
	{"Image &Sequence", eof_write_image_sequence, NULL, 0, NULL},
	{"&Preview audio", eof_menu_file_export_song_preview, NULL, 0, NULL},
	{"&IMMERROCK", eof_menu_file_export_immerrock_track_diff, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_file_menu_link_to[] =
{
	{"&FOF", eof_menu_file_link_fof, NULL, EOF_LINUX_DISABLE, NULL},
	{"&Phase Shift", eof_menu_file_link_ps, NULL, EOF_LINUX_DISABLE, NULL},
	{"&RocksmithToTab", eof_menu_file_link_rs_to_tab, NULL, EOF_LINUX_DISABLE, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_file_menu[] =
{
	{"&New\t" CTRL_NAME "+N", eof_menu_file_new_wizard, NULL, 0, NULL},
	{"&Load\t" CTRL_NAME "+O", eof_menu_file_load, NULL, 0, NULL},
	{"&Save\tF2 / " CTRL_NAME "+S", eof_menu_file_save, NULL, D_DISABLED, NULL},
	{"Save &As", eof_menu_file_save_as, NULL, D_DISABLED, NULL},
	{"&Quick save\t" CTRL_NAME "+Q", eof_menu_file_quick_save, NULL, D_DISABLED, NULL},
	{"Load &OGG", eof_menu_file_load_ogg, NULL, D_DISABLED, NULL},
	{"&Import", NULL, eof_file_import_menu, 0, NULL},
	{"&Export", NULL, eof_file_export_menu, 0, NULL},
	{"", NULL, NULL, 0, NULL},
	{"Settings\tF10", eof_menu_file_settings, NULL, 0, NULL},
	{"Default INI settings", eof_menu_file_default_ini_settings, NULL, 0, NULL},
	{"&Preferences", NULL, eof_file_preferences_menu, 0, NULL},
	{"&Display", NULL, eof_file_display_menu, 0, NULL},
	{"&Controllers", eof_menu_file_controllers, NULL, 0, NULL},
	{"Song Folder", eof_menu_file_song_folder, NULL, 0, NULL},
	{"Lin&K to", NULL, eof_file_menu_link_to, 0, NULL},
	{"", NULL, NULL, 0, NULL},
	{"E&xit\tEsc", eof_menu_file_exit, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

DIALOG eof_settings_dialog[] =
{
	/* (proc)                (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)              (dp2) (dp3) */
	{ eof_window_proc,    0,   48,  230, 284, 2,   23,  0,    0,      0,   0,   "Settings",       NULL, NULL },
	{ d_agup_text_proc,      16,  84,  64,  8,   2,   23,  0,    0,      0,   0,   "AV Delay (ms):", NULL, NULL },
	{ eof_verified_edit_proc,158, 80,  64,  20,  2,   23,  0,    0,      5,   0,   eof_etext,        "0123456789", NULL },
	{ d_agup_text_proc,      16,  108, 64,  8,   2,   23,  0,    0,      0,   0,   "MIDI Tone Delay (ms):",NULL, NULL },
	{ eof_verified_edit_proc,158, 104, 64,  20,  2,   23,  0,    0,      5,   0,   eof_etext3,       "0123456789", NULL },
	{ d_agup_text_proc,      16,  132, 64,  8,   2,   23,  0,    0,      0,   0,   "Buffer Size:",   NULL, NULL },
	{ eof_verified_edit_proc,158, 128, 64,  20,  2,   23,  0,    0,      5,   0,   eof_etext2,       "0123456789", NULL },
	{ d_agup_text_proc,      16,  156, 64,  8,   2,   23,  0,    0,      0,   0,   "CPU Saver",      NULL, NULL },
	{ d_agup_slider_proc,    126, 156, 96,  16,  2,   23,  0,    0,      10,  0,   NULL,             NULL, NULL },
	{ d_agup_check_proc,     16,  184, 160, 16,  2,   23,  0,    0,      1,   0,   "Smooth Playback",NULL, NULL },
	{ d_agup_check_proc,     16,  204, 160, 16,  2,   23,  0,    0,      1,   0,   "Disable Windows UI",NULL, NULL },
	{ d_agup_check_proc,     16,  224, 160, 16,  2,   23,  0,    0,      1,   0,   "Disable VSync",  NULL, NULL },
	{ d_agup_check_proc,     16,  244, 160, 16,  2,   23,  0,    0,      1,   0,   "Phase Cancellation",NULL, NULL },
	{ d_agup_check_proc,     16,  264, 160, 16,  2,   23,  0,    0,      1,   0,   "Center Isolation",NULL, NULL },
	{ d_agup_button_proc,    16,  292, 68,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",             NULL, NULL },
	{ d_agup_button_proc,    146, 292, 68,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",         NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_preferences_dialog[] =
{
	/* (proc)            (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                   (dp2) (dp3) */
	{ eof_window_proc,0,   48,  482, 476, 2,   23,  0,    0,      0,   0,   "Preferences",         NULL, NULL },
	{ d_agup_button_proc,12,  484, 68,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",                  NULL, NULL },
	{ d_agup_button_proc,86,  484, 68,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Default",             NULL, NULL },
	{ d_agup_button_proc,160, 484, 68,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",              NULL, NULL },
	{ d_agup_text_proc,  16,  207, 120, 12,  0,   0,   0,    0,      0,   0,   "Top of 2D pane shows:",NULL,NULL },
	{ d_agup_radio_proc, 161, 207, 60,  16,  2,   23,  0,    0,      1,   0,   "Names",               NULL, NULL },
	{ d_agup_radio_proc, 224, 207, 72,  16,  2,   23,  0,    0,      1,   0,   "Sections",            NULL, NULL },
	{ d_agup_radio_proc, 301, 207, 78,  16,  2,   23,  0,    0,      1,   0,   "Hand pos",            NULL, NULL },
	{ d_agup_radio_proc, 384, 207, 92,  16,  2,   23,  0,    0,      1,   0,   "RS sections",         NULL, NULL },
	{ d_agup_radio_proc, 161, 223, 143, 16,  2,   23,  0,    0,      1,   0,   "RS sections+Phrases", NULL, NULL },
	{ d_agup_radio_proc, 301, 223,  60, 16,  2,   23,  0,    0,      1,   0,   "Tones",               NULL, NULL },
	{ d_agup_check_proc, 16,  75,  110, 16,  2,   23,  0,    0,      1,   0,   "Inverted Notes",      NULL, NULL },
	{ d_agup_check_proc, 150, 75,  92 , 16,  2,   23,  0,    0,      1,   0,   "Lefty Mode",          NULL, NULL },
	{ d_agup_check_proc, 306, 75,  128, 16,  2,   23,  0,    0,      1,   0,   "Note Auto-Adjust",    NULL, NULL },
	{ d_agup_check_proc, 16,  91,  115, 16,  2,   23,  0,    0,      1,   0,   "Enable logging",      NULL, NULL },
	{ d_agup_check_proc, 150, 91,  148, 16,  2,   23,  0,    0,      1,   0,   "Hide drum note tails",NULL, NULL },
	{ d_agup_check_proc, 306, 91,  122, 16,  2,   23,  0,    0,      1,   0,   "Hide note names",     NULL, NULL },
	{ d_agup_check_proc, 16,  107, 130, 16,  2,   23,  0,    0,      1,   0,   "Disable sound FX",NULL, NULL },
	{ d_agup_check_proc, 150, 107, 148, 16,  2,   23,  0,    0,      1,   0,   "Disable 3D rendering",NULL, NULL },
	{ d_agup_check_proc, 306, 107, 148, 16,  2,   23,  0,    0,      1,   0,   "Disable 2D rendering",NULL, NULL },
	{ d_agup_check_proc, 16,  123, 116, 16,  2,   23,  0,    0,      1,   0,   "Hide info panel",NULL, NULL },
	{ d_agup_check_proc, 150, 123, 148, 16,  2,   23,  0,    0,      1,   0,   "Paste erases overlap",NULL, NULL },
	{ d_agup_check_proc, 306, 123, 174, 16,  2,   23,  0,    0,      1,   0,   "Make note tails clickable",NULL, NULL },
	{ d_agup_check_proc, 16,  303, 216, 16,  2,   23,  0,    0,      1,   0,   "Drum modifiers affect all diff's",NULL, NULL },
	{ d_agup_text_proc,  16,  144, 144, 12,  0,   0,   0,    0,      0,   0,   "Min. note distance:",NULL,NULL },
	{ eof_verified_edit_proc,160,144,30,20,  0,   0,   0,    0,      3,   0,   eof_etext2,     "0123456789", NULL },
	{ d_agup_text_proc,  16,  165, 144, 12,  0,   0,   0,    0,      0,   0,   "Min. note length (ms):",NULL,NULL },
	{ eof_verified_edit_proc,160,165,30,20,  0,   0,   0,    0,      3,   0,   eof_etext,     "0123456789", NULL },
	{ d_agup_check_proc, 248, 335, 214, 16,  2,   23,  0,    0,      1,   0,   "3D render bass drum in a lane",NULL, NULL },
	{ d_agup_check_proc, 248, 255, 156, 16,  2,   23,  0,    0,      1,   0,   "dB style seek controls",NULL, NULL },
	{ d_agup_text_proc,  24,  351, 48,  8,   2,   23,  0,    0,      0,   0,   "Input Method",        NULL, NULL },
	{ d_agup_list_proc,  16,  368, 100, 110, 2,   23,  0,    0,      0,   0,   (void *)eof_input_list,        NULL, NULL },
	{ d_agup_text_proc,  150, 368, 48,  8,   2,   23,  0,    0,      0,   0,   "Color set",           NULL, NULL },
	{ d_agup_list_proc,  129, 383, 100, 95,  2,   23,  0,    0,      0,   0,   (void *)eof_colors_list,       NULL, NULL },
	{ d_agup_check_proc, 248, 191, 206, 16,  2,   23,  0,    0,      1,   0,   "New notes are made 1ms long",NULL, NULL },
	{ d_agup_check_proc, 248, 271, 190, 16,  2,   23,  0,    0,      1,   0,   "Don't redraw on exit prompt",NULL, NULL },
	{ d_agup_check_proc, 248, 287, 224, 16,  2,   23,  0,    0,      1,   0,   "Rewind when playback is at end",NULL, NULL },
	{ d_agup_check_proc, 248, 303, 196, 16,  2,   23,  0,    0,      1,   0,   "Display seek pos. in seconds",NULL, NULL },
	{ d_agup_check_proc, 16,  271, 190, 16,  2,   23,  0,    0,      1,   0,   "Add new notes to selection",NULL, NULL },
	{ d_agup_check_proc, 16,  287, 210, 16,  2,   23,  0,    0,      1,   0,   "Don't auto edit new lyrics",NULL, NULL },
	{ d_agup_check_proc, 16,  255, 200, 16,  2,   23,  0,    0,      1,   0,   "Click to change dialog focus",NULL, NULL },
	{ d_agup_check_proc, 248, 319, 230, 16,  2,   23,  0,    0,      1,   0,   "EOF leaving focus stops playback",NULL, NULL },
	{ d_agup_text_proc,  16,  186, 200, 12,  0,   0,   0,    0,      0,   0,   "Chord density threshold (ms):",NULL,NULL },
	{ eof_verified_edit_proc,204,186,40,20,  0,   0,   0,    0,      5,   0,   eof_etext3,     "0123456789", NULL },
	{ d_agup_check_proc, 16,  239, 340, 16,  2,   23,  0,    0,      1,   0,   "Prefer MIDI friendly grid snaps",NULL, NULL },
	{ d_agup_check_proc, 248, 239, 230, 16,  2,   23,  0,    0,      1,   0,   "GHL conversion swaps B/W gems",NULL, NULL },
	{ d_agup_check_proc, 248, 351, 224, 16,  2,   23,  0,    0,      1,   0,   "Enable open strum by default",NULL, NULL },
	{ d_agup_check_proc, 248, 367, 206, 16,  2,   23,  0,    0,      1,   0,   "Disable automatic backups",NULL, NULL },
	{ d_agup_check_proc, 248, 383, 184, 16,  2,   23,  0,    0,      1,   0,   "Auto-Adjust sections/FHPs",NULL, NULL },
	{ d_agup_radio_proc, 190, 144, 40,  16,  2,   23,  0,    0,      2,   0,   "ms",          NULL, NULL },
	{ d_agup_radio_proc, 230, 144, 92,  16,  2,   23,  0,    0,      2,   0,   "1/# measure", NULL, NULL },
	{ d_agup_radio_proc, 322, 144, 68,  16,  2,   23,  0,    0,      2,   0,   "1/# beat",    NULL, NULL },
	{ d_agup_check_proc, 248, 175, 206, 16,  2,   23,  0,    0,      1,   0,   "New notes are force strum",NULL, NULL },
	{ d_agup_check_proc, 248, 159, 206, 16,  2,   23,  0,    0,      1,   0,   "Use FoF difficulty naming",NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_import_export_preferences_dialog[] =
{
	/* (proc)            (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                   (dp2) (dp3) */
	{ eof_window_proc,0,   48,  500, 345, 2,   23,  0,    0,      0,   0,   "Import/Export preferences",  NULL, NULL },
	{ d_agup_button_proc,12,  350, 68,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",                  NULL, NULL },
	{ d_agup_button_proc,86,  350, 68,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Default",             NULL, NULL },
	{ d_agup_button_proc,160, 350, 68,  28,  2,   23,  0,    D_EXIT, 0,   0,  "Cancel",              NULL, NULL },
	{ d_agup_check_proc, 16,    75,  208, 16,  2,   23,  0,    0,      1,   0,   "Save separate Rock Band files",NULL, NULL },
	{ d_agup_check_proc, 248,  75,  224, 16,  2,   23,  0,    0,      1,   0,   "Save separate musical MIDI files",NULL, NULL },
	{ d_agup_check_proc, 16,    90,  216, 16,  2,   23,  0,    0,      1,   0,   "Save separate Rocksmith 1 files",NULL, NULL },
	{ d_agup_check_proc, 248,  90,  216, 16,  2,   23,  0,    0,      1,   0,   "Save separate Rocksmith 2 files",NULL, NULL },
	{ d_agup_check_proc, 16,   105, 200, 16,  2,   23,  0,    0,      1,   0,   "Save separate Bandfuse files",NULL, NULL },
	{ d_agup_check_proc, 248, 105, 208, 16,  2,   23,  0,    0,      1,   0,   "Save FoF/CH/Phase Shift files",NULL, NULL },
	{ d_agup_check_proc, 16,   120, 212, 16,  2,   23,  0,    0,      1,   0,   "Save separate Guitar Hero files",NULL, NULL },
	{ d_agup_check_proc, 248, 120, 200, 16,  2,   23,  0,    0,      1,   0,   "Save separate IMMERROCK files",NULL, NULL },
	{ d_agup_check_proc, 16,   135, 222, 16,  2,   23,  0,    0,      1,   0,   "Save LRC, ELRC, QRC lyric files",NULL, NULL },
	{ d_agup_check_proc, 248, 135, 222, 16,  2,   23,  0,    0,      1,   0,   "Force pro drum MIDI notation",NULL, NULL },
	{ d_agup_check_proc, 16,   150, 218, 16,  2,   23,  0,    0,      1,   0,   "GP import truncates short notes",NULL, NULL },
	{ d_agup_check_proc, 248, 150, 186, 16,  2,   23,  0,    0,      1,   0,   "RBN export slider as HOPO",NULL, NULL },
	{ d_agup_check_proc, 16,   165, 226, 16,  2,   23,  0,    0,      1,   0,   "GP import truncates short chords",NULL, NULL },
	{ d_agup_check_proc, 248, 165, 218, 16,  2,   23,  0,    0,      1,   0,   "GP import replaces active track",NULL, NULL },
	{ d_agup_check_proc, 16,   180, 226, 16,  2,   23,  0,    0,      1,   0,   "Imports drop mid beat tempos",NULL, NULL },
	{ d_agup_check_proc, 248, 180, 218, 16,  2,   23,  0,    0,      1,   0,   "GP import nat. harmonics only",NULL, NULL },
	{ d_agup_check_proc, 16,   195, 210, 16,  2,   23,  0,    0,      1,   0,   "Import dialogs recall last path",NULL, NULL },
	{ d_agup_check_proc, 248, 195, 124, 16,  2,   23,  0,    0,      1,   0,   "Import/Export TS",NULL, NULL },
	{ d_agup_check_proc, 16,   210, 214, 16,  2,   23,  0,    0,      1,   0,   "dB import skips 5nc conversion",NULL, NULL },
	{ d_agup_check_proc, 248, 210, 214, 16,  2,   23,  0,    0,      1,   0,   "Warn about missing bass FHPs",NULL, NULL },
	{ d_agup_check_proc, 16,   225, 202, 16,  2,   23,  0,    0,      1,   0,   "Abridged Rocksmith 2 export",NULL, NULL },
	{ d_agup_check_proc, 248, 225, 220, 16,  2,   23,  0,    0,      1,   0,   "Allow RS2 extended ASCII lyrics",NULL, NULL },
	{ d_agup_check_proc, 16,   240, 224, 16,  2,   23,  0,    0,      1,   0,   "Don't warn about INI differences",NULL, NULL },
	{ d_agup_check_proc, 248, 240, 202, 16,  2,   23,  0,    0,      1,   0,   "Render mid beat tempos blue",NULL, NULL },
	{ d_agup_check_proc, 16,   255, 340, 16,  2,   23,  0,    0,      1,   0,   "GP import beat text as sections, markers as phrases",NULL, NULL },
	{ d_agup_check_proc, 16,   270, 310, 16,  2,   23,  0,    0,      1,   0,   "RS2 export version 8 (DLC Builder) style XML",NULL, NULL },
	{ d_agup_check_proc, 16,   285, 218, 16,  2,   23,  0,    0,      1,   0,   "RS import loads all handshapes",NULL, NULL },
	{ d_agup_check_proc, 248, 285, 202, 16,  2,   23,  0,    0,      1,   0,   "Don't write INI file",NULL, NULL },
	{ d_agup_check_proc, 16,   300, 315, 16,  2,   23,  0,    0,      1,   0,   "MIDI export CH/YARG open note/chord markers",NULL, NULL },
	{ d_agup_check_proc, 16,   315, 300, 16,  2,   23,  0,    0,      1,   0,   "GP import remove accent from staccato notes",NULL, NULL },
	{ d_agup_check_proc, 16,   330, 300, 16,  2,   23,  0,    0,      1,   0,   "Offer to derive finger placements from FHPs",NULL, NULL },
	{ d_agup_check_proc, 248, 345, 244, 16,  2,   23,  0,    0,      1,   0,   "GH import sustain threshold prompt",NULL, NULL },
	{ d_agup_check_proc, 248, 360, 220, 16,  2,   23,  0,    0,      1,   0,   "Don't write Rocksmith WAV file",NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_pg_preferences_dialog[] =
{
	/* (proc)            (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                   (dp2) (dp3) */
	{ eof_window_proc,0,   48,  482, 226, 2,   23,  0,    0,      0,   0,   "Pro Guitar Preferences", NULL, NULL },
	{ d_agup_button_proc,12,  234, 68,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",                  NULL, NULL },
	{ d_agup_button_proc,86,  234, 68,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Default",             NULL, NULL },
	{ d_agup_button_proc,160, 234, 68,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",              NULL, NULL },
	{ d_agup_check_proc, 16,  75, 340, 16,  2,   23,  0,    0,      1,   0,   "Apply crazy to repeated chords separated by a rest",NULL, NULL },
	{ d_agup_check_proc, 16,  91, 210, 16,  2,   23,  0,    0,      1,   0,   "Treat inverted chords as slash",NULL, NULL },
	{ d_agup_check_proc, 248, 91, 190, 16,  2,   23,  0,    0,      1,   0,   "3D render RS style chords",NULL, NULL },
	{ d_agup_check_proc, 16, 107, 224, 16,  2,   23,  0,    0,      1,   0,   "Offer to auto complete fingering",NULL, NULL },
	{ d_agup_check_proc, 248, 107, 206, 16,  2,   23,  0,    0,      1,   0,   "Don't auto-name double stops",NULL, NULL },
	{ d_agup_check_proc, 16, 123, 168, 16,  2,   23,  0,    0,      1,   0,   "Auto-Adjust tech notes",NULL, NULL },
	{ d_agup_check_proc, 248, 123, 216, 16,  2,   23,  0,    0,      1,   0,   "Fingering checks include mutes",NULL, NULL },
	{ d_agup_check_proc, 16, 139, 168, 16,  2,   23,  0,    0,      1,   0,   "2D render RS piano roll",NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_display_dialog[] =
{
	/* (proc)             (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                  (dp2) (dp3) */
	{ eof_window_proc, 0,   48,  200, 212, 2,   23,  0,    0,      0,   0,   "Display Settings",   NULL, NULL },
	{ d_agup_check_proc,  16,  80,  160, 16,  2,   23,  0,    0,      1,   0,   "Use Software Cursor",NULL, NULL },
	{ d_agup_check_proc,  16,  100, 128, 16,  2,   23,  0,    0,      1,   0,   "Force 8-Bit Color",  NULL, NULL },
	{ d_agup_text_proc,   56,  124, 48,  8,   2,   23,  0,    0,      0,   0,   "Window Size",        NULL, NULL },
	{ d_agup_list_proc,   12,  140, 174, 65,  2,   23,  0,    0,      0,   0,   (void *)eof_display_list, NULL, NULL },
	{ d_agup_button_proc, 12,  218, 174, 28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",                 NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_custom_display_size_dialog[] =
{
	/* (proc)                (x) (y)  (w)  (h)  (fg) (bg) (key) (flags)  (d1) (d2) (dp)                   (dp2) (dp3) */
	{ eof_window_proc,    0,   48,  200, 166, 2,   23,  0,   0,       0,   0,   "Custom display size", NULL, NULL },
	{ d_agup_text_proc,      12,  76,  64,  8,   2,   23,  0,   0,       0,   0,   eof_etext,             NULL, NULL },
	{ d_agup_text_proc,      12,  96,  64,  8,   2,   23,  0,   0,       0,   0,   eof_etext2,             NULL, NULL },
	{ d_agup_text_proc,      12,  118, 64,  8,   2,   23,  0,   0,       0,   0,   "Width:",              NULL, NULL },
	{ eof_verified_edit_proc,60,  114, 100, 20,  2,   23,  0,   0,       4,   0,   eof_etext3,            "0123456789", NULL },
	{ d_agup_text_proc,      12,  148, 64,  8,   2,   23,  0,   0,       0,   0,   "Height:",             NULL, NULL },
	{ eof_verified_edit_proc,60,  144, 100, 20,  2,   23,  0,   0,       4,   0,   eof_etext4,            "0123456789", NULL },
	{ d_agup_button_proc,    12,  174, 174, 28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",                  NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_guitar_settings_dialog[] =
{
	/* (proc)                (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)              (dp2) (dp3) */
	{ eof_window_proc,    4,   200, 336, 284, 2,   23,  0,    0,      0,   0,   "Guitar Settings",NULL, NULL },
	{ d_agup_text_proc,      16,  240, 64,  8,   2,   23,  0,    0,      0,   0,   "Delay (ms):",    NULL, NULL },
	{ eof_verified_edit_proc,104, 236, 64,  20,  2,   23,  0,    0,      5,   0,   eof_etext,        "0123456789", NULL },
	{ d_agup_list_proc,      13,  266, 315, 125, 2,   23,  0,    0,      0,   0,   (void *)eof_guitar_list, NULL, NULL },
	{ d_agup_push_proc,      13,  402, 315, 28,  2,   23,  0,    D_EXIT, 0,   0,   "Redefine",       NULL, (void *)eof_guitar_controller_redefine },
	{ d_agup_button_proc,    13,  442, 315, 28,  2,   23,  0,    D_EXIT, 0,   0,   "OK",             NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_drum_settings_dialog[] =
{
	/* (proc)                (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)            (dp2) (dp3) */
	{ eof_window_proc,    4,   200, 336, 238, 2,   23,  0,    0,      0,   0,   "Drum Settings",NULL, NULL },
	{ d_agup_text_proc,      16,  240, 64,  8,   2,   23,  0,    0,      0,   0,   "Delay (ms):",  NULL, NULL },
	{ eof_verified_edit_proc,104, 236, 64,  20,  2,   23,  0,    0,      5,   0,   eof_etext,      "0123456789", NULL },
	{ d_agup_list_proc,      13,  266, 315, 79,  2,   23,  0,    0,      0,   0,   (void *)eof_drum_list, NULL, NULL },
	{ d_agup_push_proc,      13,  356, 315, 28,  2,   23,  0,    D_EXIT, 0,   0,   "Redefine",     NULL, (void *)eof_drum_controller_redefine },
	{ d_agup_button_proc,    13,  396, 315, 28,  2,   23,  0,    D_EXIT, 0,   0,   "OK",           NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_controller_settings_dialog[] =
{
	/* (proc)             (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                   (dp2) (dp3) */
	{ eof_window_proc, 0,   200, 222, 144, 2,   23,  0,    0,      0,   0,   "Controller Settings", NULL, NULL },
	{ d_agup_push_proc,   12,  236, 198, 28,  2,   23,  'g',  D_EXIT, 0,   0,   "&Guitar",             NULL, (void *)eof_controller_settings_guitar },
	{ d_agup_push_proc,   12,  266, 198, 28,  2,   23,  'd',  D_EXIT, 0,   0,   "&Drums",              NULL, (void *)eof_controller_settings_drums },
	{ d_agup_button_proc, 12,  304, 198, 28,  2,   23,  '\r', D_EXIT, 0,   0,   "Done",                NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_file_new_dialog[] =
{
	/* (proc)             (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                   (dp2) (dp3) */
	{ eof_window_proc, 0,   48,  320, 120, 2,   23,  0,    0,      0,   0,   "New Song Information",NULL, NULL },
	{ d_agup_text_proc,   112, 56,  128, 8,   2,   23,  0,    0,      0,   0,   "",                    NULL, NULL },
	{ d_agup_text_proc,   16,  84,  64,  8,   2,   23,  0,    0,      0,   0,   "Artist:",             NULL, NULL },
	{ eof_edit_proc,   80,  80,  224, 20,  2,   23,  0,    0,      255, 0,   eof_etext,             NULL, NULL },
	{ d_agup_text_proc,   16,  108, 64,  8,   2,   23,  0,    0,      0,   0,   "Title:",              NULL, NULL },
	{ eof_edit_proc,   80,  104, 224, 20,  2,   23,  0,    0,      255, 0,   eof_etext2,            NULL, NULL },
	{ d_agup_button_proc, 80,  132, 68,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",                  NULL, NULL },
	{ d_agup_button_proc, 160, 132, 68,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",              NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_file_new_windows_dialog[] =
{
	/* (proc)             (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                        (dp2) (dp3) */
	{ eof_window_proc, 0,   48,  320, 168, 2,   23,  0,    0,      0,   0,   "Location for New Song",    NULL, NULL },
	{ d_agup_radio_proc,  16,  84,  256, 16,  2,   23,  0,    0,      0,   0,   "Use Existing Folder",      NULL, NULL },
	{ d_agup_radio_proc,  16,  108, 256, 16,  2,   23,  0,    0,      0,   0,   "Use Source Audio's Folder",NULL, NULL },
	{ d_agup_radio_proc,  16,  132, 256, 16,  2,   23,  0,    0,      0,   0,   "Create New Folder",        NULL, NULL },
	{ eof_edit_proc,   34,  150, 252, 20,  2,   23,  0,    0,      255, 0,   eof_etext4,                 NULL, NULL },
	{ d_agup_button_proc, 80,  180, 68,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",                       NULL, NULL },
	{ d_agup_button_proc, 160, 180, 68,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",                   NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_ogg_settings_dialog[] =
{
	/* (proc)                 (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)              (dp2) (dp3) */
	{ d_agup_shadow_box_proc, 4,   200, 192, 206, 2,   23,  0,    0,      0,   0,   NULL,             NULL, NULL },
	{ d_agup_text_proc,       58,  208, 128, 8,   2,   23,  0,    0,      0,   0,   "OGG Settings",   NULL, NULL },
	{ d_agup_text_proc,       49,  228, 48,  8,   2,   23,  0,    0,      0,   0,   "Encoder Quality",NULL, NULL },
	{ d_agup_list_proc,       43,  244, 110, 112, 2,   23,  0,    0,      0,   0,   (void *)eof_ogg_list, NULL, NULL },
	{ d_agup_button_proc,     16,  366, 68,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",             NULL, NULL },
	{ d_agup_button_proc,     116, 366, 68,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",         NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_lyric_detections_dialog[]=
{
	/*(proc)             (x)  (y)  (w)  (h)  (fg) (bg)  (key) (flags) (d1) (d2) (dp)                      (dp2) (dp3)*/
	{ eof_window_proc, 0,   48,  346, 234, 2,   23,   0,    0,      0,   0,   "Select track to import", NULL,NULL},
	{ d_agup_list_proc,   12,  84,  320, 140, 2,   23,   0,    0,      0,   0,   (void *)eof_lyric_detections_list_all,	NULL,NULL},
	{ d_agup_button_proc, 12,  237, 154, 28,  2,   23,   0,    D_EXIT, 0,   0,   "Import",                 NULL,NULL},
	{ d_agup_button_proc, 178, 237, 154, 28,  2,   23,   0,    D_EXIT, 0,   0,   "Cancel",                 NULL,NULL},
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL}
};

void eof_prepare_file_menu(void)
{
	if(eof_song && eof_song_loaded)
	{	//If a chart is loaded
		#ifdef ALLEGRO_WINDOWS
		int has_pg_notes = eof_song_has_pro_guitar_content(eof_song);
		#endif

		eof_file_menu[2].flags = 0;	//Save
		eof_file_menu[3].flags = 0;	//Save As
		eof_file_menu[4].flags = 0;	//Quick save
		eof_file_menu[5].flags = 0;	//Load OGG
		eof_file_menu[7].flags = 0;	//Export>
		if(eof_silence_loaded)
		{	//If no chart audio is loaded
			eof_file_export_menu[1].flags = D_DISABLED;	//Export>Audio range
		}
		else
		{
			eof_file_export_menu[1].flags = 0;	//Export>Audio range
		}
		#ifdef ALLEGRO_WINDOWS
			if(has_pg_notes)
			{	//If there are any notes in any of the pro guitar tracks
				eof_file_export_menu[2].flags = 0;	//Export>Guitar pro
			}
			else
			{
				eof_file_export_menu[2].flags = D_DISABLED;
			}
		#else
			eof_file_export_menu[2].flags = D_DISABLED;	//Export>Guitar pro
		#endif
		eof_file_import_menu[0].flags = 0;		//Import>Sonic Visualiser
		eof_file_import_menu[4].flags = 0;		//Import>Lyric
		eof_file_import_menu[8].flags = 0;		//Import>Queen Bee
		eof_file_import_menu[9].flags = 0;		//Import>Queen Bee (multi)
		eof_file_import_menu[10].flags = 0;	//Import>Guitar Hero sections
		eof_file_import_menu[11].flags = 0;	//Import>Guitar Hero Live

		if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
			eof_file_import_menu[12].flags = 0;	//Import>Drums Rock
		else
			eof_file_import_menu[12].flags = D_DISABLED;

		eof_file_import_menu[13].flags = 0;	//Import>GP style lyric text
		if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{
			eof_file_import_menu[5].flags = 0; // Import>Guitar Pro
			eof_file_import_menu[6].flags = 0; // Import>Rocksmith
			eof_file_export_menu[5].flags = eof_get_track_diff_size_normal(eof_song, eof_selected_track, eof_note_type) ? 0 : D_DISABLED;	//Export>IMMERROCK
		}
		else
		{
			eof_file_import_menu[5].flags = D_DISABLED;
			eof_file_import_menu[6].flags = D_DISABLED;
			eof_file_export_menu[5].flags = D_DISABLED;
		}
		eof_file_display_menu[5].flags = 0;	//Benchmark image sequence
	}
	else
	{
		eof_file_menu[2].flags = D_DISABLED;	//Save
		eof_file_menu[3].flags = D_DISABLED;	//Save As
		eof_file_menu[4].flags = D_DISABLED;	//Quick save
		eof_file_menu[5].flags = D_DISABLED;	//Load OGG
		eof_file_menu[7].flags = D_DISABLED;	//Export>
		eof_file_import_menu[0].flags = D_DISABLED;	//Import>Sonic Visualiser
		eof_file_import_menu[4].flags = D_DISABLED;	//Import>Lyric
		eof_file_import_menu[8].flags = D_DISABLED;	//Import>Queen Bee
		eof_file_import_menu[9].flags = D_DISABLED;	//Import>Queen Bee (multi)
		eof_file_import_menu[10].flags = D_DISABLED;	//Import>Guitar Hero sections
		eof_file_import_menu[11].flags = D_DISABLED;	//Import>Guitar Hero Live
		eof_file_import_menu[12].flags = D_DISABLED;	//Import>Drums Rock
		eof_file_import_menu[13].flags = D_DISABLED;	//Import>GP style lyric text
		eof_file_display_menu[5].flags = D_DISABLED;	//Benchmark image sequence
	}

	eof_file_display_menu[4].flags = eof_screen_zoom ? D_SELECTED : 0;	//File>Display>x2 zoom
	eof_file_3d_preview_menu[2].flags = eof_full_height_3d_preview ? D_SELECTED : 0;	//Display>3D Preview>Full height

	//Notes panel stuff
	eof_file_notes_panel_menu[10].flags = eof_enable_notes_panel ? D_SELECTED : 0;		//File>Display>Notes Panel>Enable
	if(exists(eof_last_browsed_notes_panel_path))
	{	//If the last browsed Notes panel file path is valid, display the relative file name for the user's reference
		(void) snprintf(eof_file_notes_panel_menu_string, sizeof(eof_file_notes_panel_menu_string) - 1, "&User defined (%s)", get_filename(eof_last_browsed_notes_panel_path));
	}
	else
	{
		(void) snprintf(eof_file_notes_panel_menu_string, sizeof(eof_file_notes_panel_menu_string) - 1, "&User defined");
	}
}

int eof_menu_file_new_supplement(char *directory, char *filename, char check)
{
	char syscommand[1024] = {0};
	int err;

	if(!directory)
		return 0;	//Return error

	(void) ustrcpy(syscommand, directory);

	/* remove slash from folder name so we can check if it exists */
	if((syscommand[uoffset(syscommand, ustrlen(syscommand) - 1)] == '\\') || (syscommand[uoffset(syscommand, ustrlen(syscommand) - 1)] == '/'))
	{	//If the path ends in a separator
		syscommand[uoffset(syscommand, ustrlen(syscommand) - 1)] = '\0';	//Remove it
	}
	if(!eof_folder_exists(syscommand))
	{	//If the folder doesn't exist,
		err = eof_mkdir(syscommand);	//Try to create it
		if(err && !eof_folder_exists(syscommand))
		{	//If it couldn't be created and is still not found to exist (in case the previous check was a false negative)
			eof_render();
			allegro_message("Could not create folder!\n%s\nEnsure that the specified folder name is valid and\nthe Song Folder is configured to a non write-restricted area.\n(File->Song Folder)", syscommand);
			return 0;
		}
	}

	eof_render();
	err = 0;
	put_backslash(directory);
	if(check & 1)
	{	//If checking for the presence of guitar.ogg
		(void) replace_filename(eof_temp_filename, directory, "guitar.ogg", 1024);
		if(exists(eof_temp_filename))
		{
			eof_clear_input();
			if(alert(NULL, "Existing guitar.ogg will be overwritten. Proceed?", NULL, "&Yes", "&No", 'y', 'n') == 2)
			{
				return 0;
			}
		}
	}
	if(check & 2)
	{	//If checking for the presence of original.mp3
		(void) replace_filename(eof_temp_filename, directory, "original.mp3", 1024);
		if(exists(eof_temp_filename))
		{
			eof_clear_input();
			if(alert(NULL, "Existing original.mp3 will be overwritten. Proceed?", NULL, "&Yes", "&No", 'y', 'n') == 2)
			{
				return 0;
			}
		}
	}
	if(check & 4)
	{	//If checking for the presence of [filename].ogg and [filename].eof
		if(!filename)	//If a filename was not specified
			return 0;	//Invalid parameters
		(void) replace_filename(eof_temp_filename, directory, filename, 1024);
		(void) replace_extension(eof_temp_filename, eof_temp_filename, "ogg", 1024);
		if(exists(eof_temp_filename))
		{
			eof_clear_input();
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Existing OGG file will be overwritten.  Proceed?");
			if(alert(NULL, eof_log_string, NULL, "&Yes", "&No", 'y', 'n') == 2)
			{
				return 0;
			}
		}
		(void) replace_extension(eof_temp_filename, eof_temp_filename, "wav", 1024);
		if(exists(eof_temp_filename))
		{
			eof_clear_input();
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Existing WAV file will be overwritten.  Proceed?");
			if(alert(NULL, eof_log_string, NULL, "&Yes", "&No", 'y', 'n') == 2)
			{
				return 0;
			}
		}
		(void) replace_filename(eof_temp_filename, directory, filename, 1024);
		if(exists(eof_temp_filename))
		{
			eof_clear_input();
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Existing EOF project \"%s\" will be overwritten.  Proceed?", filename);
			if(alert(NULL, eof_log_string, NULL, "&Yes", "&No", 'y', 'n') == 2)
			{
				return 0;
			}
		}
	}
	if(check & ~(1 | 2 | 4))
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
		if(filename)
		{	//If the calling function specified a project file name to check for
			(void) replace_filename(eof_temp_filename, directory, filename, 1024);
		}
		else
		{	//Otherwise check for an existing project file named "notes.eof"
			(void) replace_filename(eof_temp_filename, directory, "notes.eof", 1024);
		}
		if(exists(eof_temp_filename))
		{
			err = 1;
		}
		if(eof_write_fof_files)
		{	//If the user opted to save Frets on Fire and Phase Shift files
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
	}
	if(err)
	{
		eof_clear_input();
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

	eof_log("eof_menu_file_new_wizard() entered", 1);
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

	eof_log("eof_menu_file_load() entered", 1);

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
		int warn = 1;	//By default, warn about differences in the INI file

		if(eof_disable_ini_difference_warnings)
			warn = 0;	//Unless the user enabled the preference to disable the warnings

		/* free the current project */
		if(eof_song)
		{
			eof_destroy_song(eof_song);
			eof_song = NULL;
			eof_song_loaded = 0;
		}
		(void) eof_destroy_ogg();

		/* load the specified project */
		eof_song = eof_load_song(returnedfn);
		if(!eof_song)
		{
			eof_song_loaded = 0;
			allegro_message("Error loading song!");
			eof_fix_window_title();
			return 1;
		}
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tSet eof_song_path to \"%s\"", eof_song_path);
		eof_log(eof_log_string, 1);

		/* check song.ini and prompt user to load any external edits */
		(void) replace_filename(temp_filename, eof_song_path, "song.ini", 1024);
		(void) eof_import_ini(eof_song, temp_filename, warn);	//Read song.ini and prompt to replace values of existing settings in the project if they are different (unless user preference suppresses the prompts)

		/* attempt to load the first OGG profile's OGG */
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tAttempting to load first OGG profile \"%s\"", eof_song->tags->ogg[0].filename);
		eof_log(eof_log_string, 1);
		(void) replace_filename(temp_filename, eof_song_path, eof_song->tags->ogg[0].filename, 1024);
		if(!eof_load_ogg_quick(temp_filename))
		{
			eof_log("\tCouldn't load OGG, trying guitar.ogg", 1);
			/* upon fail, fall back to "guitar.ogg" */
			(void) append_filename(temp_filename, eof_song_path, "guitar.ogg", 1024);
			if(!eof_load_ogg(temp_filename, 1))	//If user does not provide audio, fail over to using silent audio
			{
				eof_log("\tCouldn't load guitar.ogg, continuing with no audio", 1);
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
		}

		eof_song_loaded = 1;
		eof_chart_length = alogg_get_length_msecs_ogg_ul(eof_music_track);
		eof_init_after_load(0);
		eof_fixup_notes(eof_song);
		(void) eof_detect_difficulties(eof_song, eof_selected_track);	//Update arrays for note set population and highlighting
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
	int retval;

	if(eof_song_loaded)
	{
		if(!eof_music_paused)
		{
			eof_music_play(0);
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
		if(eof_menu_file_new_supplement(new_foldername, get_filename(returnedfn), 8) == 0)	//If the folder doesn't exist, or the user has declined to overwrite any existing files
			return 1;	//Return failure

		retval = eof_save_helper(returnedfn, 0);	//Perform "Save As" operation to the selected path
		if(retval == 0)
		{	//If the "Save as" operation succeeded, update folder path strings
			(void) ustrcpy(eof_song_path, new_foldername);	//Set the project folder path
			(void) ustrcpy(eof_last_eof_path, new_foldername);
		}
		else if(retval == 1)
		{
			eof_log("\tSave canceled", 1);
		}
		else
		{	//If save was not successful
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError code %d during \"Save as\"", retval);
			eof_log(eof_log_string, 1);
			allegro_message("Could not save project (error %d)", retval);
		}
		return retval;
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

	eof_log("eof_menu_file_load_ogg() entered", 1);

	returnedfn = ncd_file_select(0, eof_song_path, "Select OGG File", eof_filter_ogg_files);	//Init the dialog to the project path instead of eof_last_ogg_path since the current behavior is to only allow loading one from the active project's folder
	eof_clear_input();
	if(!returnedfn)
	{
		eof_log("\tFile browse canceled.", 1);
		return 1;	//If the user did not select a file, return immediately
	}

	eof_delete_rocksmith_wav();		//Delete the Rocksmith WAV file since loading different audio will require a new WAV file to be written

	/* make sure selected file is in the same path as the current chart */
	(void) replace_filename(checkfn, returnedfn, "", 1024);
	(void) replace_filename(checkfn2, eof_song_path, "", 1024);
	if(ustricmp(checkfn, checkfn2))
	{
		allegro_message("OGGs can only be loaded from the current song folder!\n");
		return 1;
	}

	/* try to load the specified OGG, or fail back to the previous audio */
	ogg_profile_name = eof_song->tags->ogg[0].filename;	//Allow eof_load_ogg() to update the name of the file in the OGG profile
	if(!eof_load_ogg(returnedfn, 0))
	{	//If eof_load_ogg() failed, eof_loaded_ogg_name contains the name of the file that was loaded before
		eof_log("\tFailed to load specified OGG.", 1);
		returnedfn = eof_loaded_ogg_name;
		if(!eof_load_ogg(eof_loaded_ogg_name, 0))
		{
			eof_log("\tFailed to re-load the OGG that was in use when this function was invoked.", 1);
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

	new_length = alogg_get_length_msecs_ogg_ul(eof_music_track);
	if(new_length > eof_chart_length)
	{
		eof_chart_length = new_length;
		eof_calculate_beats(eof_song);
	}
	if(eof_music_pos.value > eof_music_length)
	{
		(void) eof_menu_song_seek_end();
	}
	eof_music_seek(eof_music_pos.value - eof_av_delay);
	(void) replace_filename(eof_last_ogg_path, returnedfn, "", 1024);
	eof_log("\t\t\tOGG loaded", 1);

	return 1;
}

int eof_menu_file_save(void)
{
	eof_log("File>Save selected", 1);
	return eof_menu_file_save_logic(0);
}

int eof_menu_file_quick_save(void)
{
	eof_log("\"File>Quick save\" selected", 1);
	return eof_menu_file_save_logic(1);
}

int eof_menu_file_save_logic(char silent)
{
	int err, retval;

	if(!eof_song_loaded || !eof_song)
		return 1;	//Do not allow this function to run if no chart is loaded

	if(!eof_music_paused)
	{
		eof_music_play(0);
	}
	eof_cursor_visible = 0;
	eof_pen_visible = 0;

	/* no changes so ask for save */
	if(!silent && (eof_changes <= 0))
	{
		eof_show_mouse(screen);
		eof_clear_input();
		if(alert(NULL, "No Changes, save anyway?", NULL, "&Yes", "&No", 'y', 'n') == 2)
		{
			eof_show_mouse(NULL);
			eof_cursor_visible = 1;
			eof_pen_visible = 1;
			return 1;
		}
	}

	/* check to see if the project file still exists */
	if((eof_song_path[0] == '\0') || (eof_loaded_song_name[0] == '\0'))
	{
		eof_log("Song and project paths not initialized.  Save failed.", 1);
		allegro_message("Save failed, please try \"Save as\".");
		return 2;	//Project path strings are invalid
	}
	(void) append_filename(eof_temp_filename, eof_song_path, eof_loaded_song_name, (int) sizeof(eof_temp_filename));	//Get full project path
	if(!exists(eof_temp_filename))
	{	//If the project file doesn't exist, check to see if song folder still exists
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tProject file \"%s\" does not exist.  Checking existence of project folder", eof_temp_filename);
		eof_log(eof_log_string, 1);

		(void) ustrcpy(eof_temp_filename, eof_song_path);
		if((eof_temp_filename[uoffset(eof_temp_filename, ustrlen(eof_temp_filename) - 1)] == '\\') || (eof_temp_filename[uoffset(eof_temp_filename, ustrlen(eof_temp_filename) - 1)] == '/'))
		{	//If the path ends in a separator
			eof_temp_filename[uoffset(eof_temp_filename, ustrlen(eof_temp_filename) - 1)] = '\0';	//Remove it
		}
		if(!eof_folder_exists(eof_temp_filename))
		{
			eof_clear_input();
			if(alert("Song folder no longer exists.", "Recreate folder?", NULL, "&Yes", "&No", 'y', 'n') == 2)
			{
				eof_show_mouse(NULL);
				eof_cursor_visible = 1;
				eof_pen_visible = 1;
				return 2;
			}
			err = eof_mkdir(eof_temp_filename);
			if(err && !eof_folder_exists(eof_temp_filename))
			{	//If it couldn't be created and is still not found to exist (in case the previous check was a false negative)
				eof_render();
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError creating project folder:  \"%s\"", strerror(errno));	//Get the Operating System's reason for the failure
				eof_log(eof_log_string, 1);
				allegro_message("Could not create folder!\n%s", eof_temp_filename);
				eof_show_mouse(NULL);
				eof_cursor_visible = 1;
				eof_pen_visible = 1;
				return 2;
			}
		}
	}

	if(!silent)
	{	//Normal save
		eof_log("\tSaving chart", 1);
	}
	else
	{
		eof_log("\tQuick saving chart", 1);
	}

	retval = eof_save_helper(NULL, silent);	//Perform "Save" operation to the chart's current path
	if(retval == 1)
	{
		eof_log("\tSave canceled", 1);
	}
	else if(retval > 1)
	{	//If save was not successful
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError code %d during \"Save\"", retval);
		eof_log(eof_log_string, 1);
		allegro_message("Could not save project (error %d)", retval);
	}
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_render();

	eof_close_menu = 1;			//Force the main menu to close, as this function had a tendency to get hung in the menu logic when activated by keyboard
	return retval;
}

int eof_menu_file_lyrics_import(void)
{
	char * returnedfn = NULL, *initial;
	int jumpcode = 0;
	int selectedformat=0;
	char *selectedtrack = NULL;
	int returncode = 1;			//Stores the return value of EOF_IMPORT_VIA_LC() to check for error
	static char tempfile = 0;	//Is set to nonzero if the selected lyric file's path contains any extended ASCII or Unicode characters, as a temporary file is created as a workaround
	char templyricfile[30];		//The name of the temporary file created as per the above condition

	eof_log("\tImporting lyrics", 1);

	if(eof_song == NULL)	//Do not import lyrics if no chart is open
		return 0;

	if(eof_validate_temp_folder())
	{	//Ensure the correct working directory and presence of the temporary folder
		eof_log("\tCould not validate working directory and temp folder", 1);
		return 1;
	}

	(void) snprintf(templyricfile, sizeof(templyricfile) - 1, "%slyricfile.tmp", eof_temp_path_s);
	tempfile = 0;	//Keep tempfile a static variable because if setjmp's fail condition is reached, the value may be re-initialized, clobbering its value
	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	if((eof_last_lyric_path[uoffset(eof_last_lyric_path, ustrlen(eof_last_lyric_path) - 1)] == '\\') || (eof_last_lyric_path[uoffset(eof_last_lyric_path, ustrlen(eof_last_lyric_path) - 1)] == '/'))
	{	//If the path ends in a separator
		eof_last_lyric_path[uoffset(eof_last_lyric_path, ustrlen(eof_last_lyric_path) - 1)] = '\0';	//Remove it
	}
	if(eof_imports_recall_last_path && eof_folder_exists(eof_last_lyric_path))
	{	//If the user chose for the lyric import dialog to start at the path of the last imported lyric file and that path is valid
		initial = eof_last_lyric_path;	//Use it
	}
	else
	{	//Otherwise start at the project's path
		initial = eof_last_eof_path;
	}
	returnedfn = ncd_file_select(0, initial, "Import Lyrics", eof_filter_lyrics_files);
	eof_clear_input();
	if(returnedfn)
	{	//If the user selected a file
		if(eof_string_has_non_ascii(returnedfn))
		{	//If the string has any non ASCII characters
			eof_log("\t\tUnicode or extended ASCII file path detected.  Creating temporarily copy to use for import.", 1);
			tempfile = 1;	//Track that EOF will create a temporary file to pass to FoFLC, since FoFLC uses standard C file I/O and cannot open file paths that aren't normal ASCII
			(void) eof_copy_file(returnedfn, templyricfile);	//Copy this to a temporary file containing normal ASCII characters
			returnedfn = templyricfile;	//Change the target file path to the temporary file
		}
		(void) ustrcpy(eof_filename, returnedfn);	//Store another copy of the lyric file name, since if it's a vocal rhythm file, the user will have to select a MIDI file, whose path will be stored in returnedfn
		jumpcode=setjmp(jumpbuffer); //Store environment/stack/etc. info in the jmp_buf array
		if(jumpcode!=0) //if program control returned to the setjmp() call above returning any nonzero value
		{	//If the import failed
			(void) puts("Assert() handled successfully!");
			eof_show_mouse(NULL);
			eof_cursor_visible = 1;
			eof_pen_visible = 1;
			if(tempfile)
			{	//If a temporary file was created
				(void) delete_file(templyricfile);	//Delete it
			}
			eof_log("\t\tError:  Could not import lyrics.  Undetermined error.", 1);
			(void) alert("Error", NULL, "Could not import lyrics.  Undetermined error.", "OK", NULL, 0, KEY_ENTER);
			return 1;
		}

		//Detect if the selected file has lyrics
		eof_log("\t\tDetecting lyric format.", 1);
		lyricdetectionlist=DetectLyricFormat(returnedfn);	//Auto detect the lyric format of the chosen file
		ReleaseMemory(1);	//Release memory allocated during lyric import
		if(lyricdetectionlist == NULL)
		{
			if(tempfile)
			{	//If a temporary file was created
				(void) delete_file(templyricfile);	//Delete it
			}
			(void) alert("Error", NULL, "No lyrics detected", "OK", NULL, 0, KEY_ENTER);
			return 0;	//return error
		}

	//Import lyrics
		eof_log("\t\tParsing lyrics.", 1);
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
			eof_log("\t\tError:  Invalid lyric file", 1);
			(void) alert("Error", NULL, "Invalid lyric file", "OK", NULL, 0, KEY_ENTER);
			(void) eof_menu_edit_undo();
		}
		else
		{	//Import succeeded
			eof_log("\tLyric import successful", 1);
			eof_truncate_chart(eof_song);	//Add beats to the chart if necessary to encompass the imported lyrics
			eof_track_fixup_notes(eof_song, EOF_TRACK_VOCALS, 0);
			eof_reset_lyric_preview_lines();
			(void) replace_filename(eof_last_lyric_path, returnedfn, "", 1024);	//Set the last loaded lyric file path
		}

		if(tempfile)
		{	//If a temporary file was created
			(void) delete_file(templyricfile);	//Delete it
		}
	}//If the user selected a file
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	DestroyLyricFormatList(lyricdetectionlist);	//Free the detection list
	(void) eof_detect_difficulties(eof_song, eof_selected_track);	//Refresh track populated status in case the vocal track is active
	return 1;
}

int eof_menu_file_midi_import(void)
{
	char returnedfn_path[1024] = {0}, *returnedfn = NULL;
	char tempfilename[1024] = {0};
	char *initial;

	eof_log("eof_menu_file_midi_import() entered", 1);

	if(eof_menu_prompt_save_changes() == 3)
	{	//If user canceled closing the current, modified chart
		return 1;
	}
	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	if((eof_last_midi_path[uoffset(eof_last_midi_path, ustrlen(eof_last_midi_path) - 1)] == '\\') || (eof_last_midi_path[uoffset(eof_last_midi_path, ustrlen(eof_last_midi_path) - 1)] == '/'))
	{	//If the path ends in a separator
		eof_last_midi_path[uoffset(eof_last_midi_path, ustrlen(eof_last_midi_path) - 1)] = '\0';	//Remove it
	}
	if(eof_imports_recall_last_path && eof_folder_exists(eof_last_midi_path))
	{	//If the user chose for the MIDI import dialog to start at the path of the last imported MIDI file and that path is valid
		initial = eof_last_midi_path;	//Use it
	}
	else
	{	//Otherwise start at the project's path
		initial = eof_last_eof_path;
	}
	returnedfn = ncd_file_select(0, initial, "Import MIDI", eof_filter_midi_files);
	eof_clear_input();
	if(returnedfn)
	{
		eof_log("\tImporting MIDI", 1);
		strncpy(returnedfn_path, returnedfn, sizeof(returnedfn_path) - 1);	//Back up this path for later use

		if(eof_song)
		{
			eof_destroy_song(eof_song);
			eof_song = NULL;
			eof_song_loaded = 0;
		}
		(void) eof_destroy_ogg();
		if(!ustricmp(get_extension(returnedfn), "rba"))
		{
			(void) replace_filename(tempfilename, returnedfn, "eof_rba_import.tmp", 1024);
			if(eof_extract_rba_midi(returnedfn,tempfilename) == 0)
			{	//If this was an RBA file and the MIDI was extracted successfully
				eof_song = eof_import_midi(tempfilename);
				(void) delete_file(tempfilename);	//Delete temporary file
			}
		}
		else
		{	//Perform regular MIDI import
			eof_song = eof_import_midi(returnedfn);
		}
		if(eof_song)
		{
			eof_song_loaded = 1;
			eof_init_after_load(0);
			if(!eof_repair_midi_import_grid_snap())
			{
				eof_log("\tGrid snap correction failed.", 1);
			}
			eof_track_fixup_notes(eof_song, EOF_TRACK_VOCALS, 0);
			eof_song_enforce_mid_beat_tempo_change_removal();	//Remove mid beat tempo changes if applicable
			(void) eof_detect_difficulties(eof_song, eof_selected_track);
			(void) replace_filename(eof_last_midi_path, returnedfn_path, "", 1024);	//Set the last loaded MIDI file path
			eof_determine_phrase_status(eof_song, eof_selected_track);	//Update HOPO statuses
			eof_skip_mid_beats_in_measure_numbering = 0;	//Disable this measure numbering alteration so that any relevant warnings can be given by eof_detect_mid_measure_ts_changes() below
			eof_beat_stats_cached = 0;
			eof_detect_mid_measure_ts_changes();
///			eof_skip_mid_beats_in_measure_numbering = 1;	//Enable mid beat tempo changes to be ignored in the measure numbering now that any applicable warnings were given
			(void) eof_check_for_notes_preceding_sections(0);	//Warn if there are notes that precede the first section event, if there are any sections
			eof_beat_stats_cached = 0;
		}
		else
		{
			eof_song_loaded = 0;
			allegro_message("Could not import MIDI!");
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

int eof_menu_file_drums_rock_import(void)
{
	char returnedfn_path[1024] = {0}, *returnedfn = NULL;
	char *initial;
	unsigned long ctr;
	int retval = 1;

	eof_log("eof_menu_file_drums_rock_import() entered", 1);

	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();

	if(!eof_song || !eof_song_loaded)
		return 1;	//Don't do anything unless a project is loaded
	if(eof_song->track[eof_selected_track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR)
		return 1;	//Don't do anything unless the active track is a drum track

	if((eof_last_dr_path[uoffset(eof_last_dr_path, ustrlen(eof_last_dr_path) - 1)] == '\\') || (eof_last_dr_path[uoffset(eof_last_dr_path, ustrlen(eof_last_dr_path) - 1)] == '/'))
	{	//If the path ends in a separator
		eof_last_dr_path[uoffset(eof_last_dr_path, ustrlen(eof_last_dr_path) - 1)] = '\0';	//Remove it
	}
	if(eof_imports_recall_last_path && eof_folder_exists(eof_last_dr_path))
	{	//If the user chose for the DR import dialog to start at the path of the last imported DR file and that path is valid
		initial = eof_last_dr_path;	//Use it
	}
	else
	{	//Otherwise start at the project's path
		initial = eof_last_eof_path;
	}
	returnedfn = ncd_file_select(0, initial, "Import Drums Rock", eof_filter_dr_files);
	eof_clear_input();
	if(returnedfn)
	{
		eof_log("\tImporting Drums Rock", 1);
		strncpy(returnedfn_path, returnedfn, sizeof(returnedfn_path) - 1);	//Back up this path for later use

		retval = eof_import_drums_rock_track_diff(returnedfn);
		if(retval == 1)
		{
			allegro_message("Could not import Drums Rock file!");
		}
		else if(retval == 0)
		{
			eof_track_fixup_notes(eof_song, eof_selected_track, 0);
			(void) eof_detect_difficulties(eof_song, eof_selected_track);
			(void) replace_filename(eof_last_dr_path, returnedfn_path, "", 1024);	//Set the last loaded DR file path
			eof_log("\tDrums Rock file loaded", 1);
		}
	}
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;

	if(retval == 0)
	{	//If the file was imported
		if(!(eof_song->track[eof_selected_track]->flags & EOF_TRACK_FLAG_DRUMS_ROCK))
			(void) eof_menu_track_drumsrock_enable_drumsrock_export();	//If drums rock mode isn't already enabled for the active track, enable it now

		if(eof_song->beats > 1)
		{	//Bounds check
			for(ctr = 0; ctr < eof_song->beats - 1; ctr++)
			{
				if(eof_song->beat[ctr]->ppqn != eof_song->beat[ctr + 1]->ppqn)
				{	//If there's at least one tempo change in this project
					if(alert("The project has a tempo map, but Drums Rock timing has lower accuracy.", "It's suggested to repair the grid snap of imported notes", "and correct by the detected skew.  Perform this?", "&Yes", "&No", 'y', 'n') == 1)
					{	//If the user opts to repair the grid snapping
						(void) eof_menu_track_repair_grid_snap();
					}
					break;
				}
			}
		}
	}

	return 1;
}

int eof_menu_file_settings(void)
{
	if(eof_song_loaded)
	{
		if(!eof_music_paused)
		{
			eof_music_play(0);
		}
	}
	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	eof_color_dialog(eof_settings_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_settings_dialog);
	(void) snprintf(eof_etext, sizeof(eof_etext) - 1, "%lu", eof_av_delay);
	(void) snprintf(eof_etext2, sizeof(eof_etext2) - 1, "%d", eof_buffer_size);
	(void) snprintf(eof_etext3, sizeof(eof_etext2) - 1, "%d", eof_midi_tone_delay);
	eof_settings_dialog[8].d2 = eof_cpu_saver;
	eof_settings_dialog[9].flags = eof_smooth_pos ? D_SELECTED : 0;
	eof_settings_dialog[10].flags = eof_disable_windows ? D_SELECTED : 0;
	eof_settings_dialog[11].flags = eof_disable_vsync ? D_SELECTED : 0;
	eof_settings_dialog[12].flags = eof_phase_cancellation ? D_SELECTED : 0;
	eof_settings_dialog[13].flags = eof_center_isolation ? D_SELECTED : 0;
	if(eof_popup_dialog(eof_settings_dialog, 0) == 14)
	{	//User clicked OK
		eof_av_delay = strtoul(eof_etext, NULL, 10);
		eof_buffer_size = atol(eof_etext2);
		if(eof_buffer_size < 1024)
		{
			allegro_message("Buffer size must be at least 1024. It has been set to 1024.");
			eof_buffer_size = 1024;
		}
		eof_midi_tone_delay = atol(eof_etext3);
		if(eof_midi_tone_delay < 0)
		{
			allegro_message("MIDI Delay must be at least 0.\nIt has been set to 0 for now.");
			eof_midi_tone_delay = 0;
		}
		eof_cpu_saver = eof_settings_dialog[8].d2;
		eof_smooth_pos = (eof_settings_dialog[9].flags == D_SELECTED ? 1 : 0);
		eof_disable_windows = (eof_settings_dialog[10].flags == D_SELECTED ? 1 : 0);
		eof_disable_vsync = (eof_settings_dialog[11].flags == D_SELECTED ? 1 : 0);
		eof_phase_cancellation  = (eof_settings_dialog[12].flags == D_SELECTED ? 1 : 0);
		eof_center_isolation  = (eof_settings_dialog[13].flags == D_SELECTED ? 1 : 0);
		ncdfs_use_allegro = eof_disable_windows;

		if(eof_phase_cancellation)
			eof_center_isolation = 0;	//These settings can't both be enabled simultaneously

		eof_fix_window_title();
	}
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	return 1;
}

int eof_menu_file_default_ini_settings(void)
{
	eof_cursor_visible = 0;
	eof_render();

	(void) snprintf(eof_etext2, sizeof(eof_etext2) - 1, "Default INI Settings");
	eof_ini_dialog_array = eof_default_ini_setting;	//The dialog will alter the EOF instance's default INI settings list
	eof_ini_dialog_count = &eof_default_ini_settings;

	eof_color_dialog(eof_ini_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_ini_dialog);
	if(eof_popup_dialog(eof_ini_dialog, 0) == 4)
	{
	}
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(NULL);
	return 1;
}

int eof_menu_file_preferences(void)
{
	int retval, original_input_mode, original_eof_disable_info_panel = eof_disable_info_panel, original_eof_prefer_midi_friendly_grid_snapping = eof_prefer_midi_friendly_grid_snapping;
	unsigned original_eof_min_note_distance = eof_min_note_distance, original_eof_chord_density_threshold = eof_chord_density_threshold;
	unsigned long ctr;

	eof_log("Preferences logic starting", 2);

	if(eof_song_loaded)
	{
		if(!eof_music_paused)
		{
			eof_music_play(0);
		}
	}
	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	eof_color_dialog(eof_preferences_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_preferences_dialog);
	//Use the currently configured settings to populate the dialog selections
	eof_preferences_dialog[5].flags = eof_preferences_dialog[6].flags = eof_preferences_dialog[7].flags = eof_preferences_dialog[8].flags = eof_preferences_dialog[9].flags = eof_preferences_dialog[10].flags = 0;	//Options for what to display at top of 2D panel
	eof_preferences_dialog[eof_2d_render_top_option].flags = D_SELECTED;
	eof_preferences_dialog[11].flags = eof_inverted_notes ? D_SELECTED : 0;					//Inverted notes
	eof_preferences_dialog[12].flags = eof_lefty_mode ? D_SELECTED : 0;						//Lefty mode
	eof_preferences_dialog[13].flags = eof_note_auto_adjust ? D_SELECTED : 0;					//Note auto adjust
	eof_preferences_dialog[14].flags = eof_enable_logging ? D_SELECTED : 0;					//Enable logging
	eof_preferences_dialog[15].flags = eof_hide_drum_tails ? D_SELECTED : 0;					//Hide drum note tails
	eof_preferences_dialog[16].flags = eof_hide_note_names ? D_SELECTED : 0;				//Hide note names
	eof_preferences_dialog[17].flags = eof_disable_sound_processing ? D_SELECTED : 0;			//Disable sound effects
	eof_preferences_dialog[18].flags = eof_disable_3d_rendering ? D_SELECTED : 0;				//Disable 3D rendering
	eof_preferences_dialog[19].flags = eof_disable_2d_rendering ? D_SELECTED : 0;				//Disable 2D rendering
	eof_preferences_dialog[20].flags = eof_disable_info_panel ? D_SELECTED : 0;				//Hide info panel
	eof_preferences_dialog[21].flags = eof_paste_erase_overlap ? D_SELECTED : 0;				//Paste erases overlap
	eof_preferences_dialog[22].flags = eof_note_tails_clickable ? D_SELECTED : 0;				//Make note tails clickable
	eof_preferences_dialog[23].flags = eof_drum_modifiers_affect_all_difficulties ? D_SELECTED : 0;	//Drum modifiers affect all diff's
	eof_preferences_dialog[28].flags = eof_render_bass_drum_in_lane ? D_SELECTED : 0;			//3D render bass drum in a lane
	eof_preferences_dialog[29].flags = eof_fb_seek_controls ? D_SELECTED : 0;					//dB style seek controls
	eof_preferences_dialog[31].d1 = eof_input_mode;										//Input method
	original_input_mode = eof_input_mode;											//Store this value
	eof_preferences_dialog[33].d1 = eof_color_set;										//Color set
	eof_preferences_dialog[34].flags = eof_new_note_length_1ms ? D_SELECTED : 0;				//New notes are made 1ms long
	eof_preferences_dialog[35].flags = eof_dont_redraw_on_exit_prompt ? D_SELECTED : 0;		//Don't redraw on exit prompt
	eof_preferences_dialog[36].flags = eof_rewind_at_end ? D_SELECTED : 0;					//Rewind when playback is at end
	eof_preferences_dialog[37].flags = eof_display_seek_pos_in_seconds ? D_SELECTED : 0;		//Display seek pos. in seconds
	eof_preferences_dialog[38].flags = eof_add_new_notes_to_selection ? D_SELECTED : 0;		//Add new notes to selection
	eof_preferences_dialog[39].flags = eof_dont_auto_edit_new_lyrics ? D_SELECTED : 0;			//Don't auto edit new lyrics
	eof_preferences_dialog[40].flags = eof_click_changes_dialog_focus ? D_SELECTED : 0;			//Click to change dialog focus
	eof_preferences_dialog[41].flags = eof_stop_playback_leave_focus ? D_SELECTED : 0;			//EOF leaving focus stops playback
	eof_preferences_dialog[44].flags = eof_prefer_midi_friendly_grid_snapping ? D_SELECTED : 0;	//Prefer MIDI friendly grid snaps
	eof_preferences_dialog[45].flags = eof_ghl_conversion_swaps_bw_gems ? D_SELECTED : 0;	//GHL conversion swaps B/W gems
	eof_preferences_dialog[46].flags = eof_enable_open_strums_by_default ? D_SELECTED : 0;		//Enable open strum by default
	eof_preferences_dialog[47].flags = eof_disable_backups ? D_SELECTED : 0;					//Disable automatic backups
	eof_preferences_dialog[48].flags = eof_section_auto_adjust ? D_SELECTED : 0;				//Auto-Adjust sections/FHPs
	eof_preferences_dialog[49].flags = eof_preferences_dialog[50].flags = eof_preferences_dialog[51].flags = 0;	//Clear the min. note distance radio buttons
	if(!eof_min_note_distance_intervals)
	{	//If the min. note distance is ms
		eof_preferences_dialog[49].flags = D_SELECTED;	//Select the "ms" radio button
	}
	else if(eof_min_note_distance_intervals == 1)
	{
		eof_preferences_dialog[50].flags = D_SELECTED;	//Select the "1/# measure" radio button
	}
	else
	{
		eof_preferences_dialog[51].flags = D_SELECTED;	//Select the "1/# beat" radio button
	}
	if(eof_min_note_length)
	{	//If the user has defined a minimum note length
		(void) snprintf(eof_etext, sizeof(eof_etext) - 1, "%d", eof_min_note_length);	//Populate the field's string with it
	}
	else
	{
		eof_etext[0] = '\0';	//Otherwise empty the string
	}
	(void) snprintf(eof_etext2, sizeof(eof_etext2) - 1, "%u", eof_min_note_distance);	//Populate the min. note distance field's string
	if(eof_chord_density_threshold)
	{	//If the user has defined a chord density threshold
		(void) snprintf(eof_etext3, sizeof(eof_etext3) - 1, "%u", eof_chord_density_threshold);	//Populate the field's string with it
	}
	else
	{
		eof_etext3[0] = '\0';	//Otherwise empty the string
	}
	eof_preferences_dialog[52].flags = eof_new_note_forced_strum ? D_SELECTED : 0;			//New notes are force strum
	eof_preferences_dialog[53].flags = eof_use_fof_difficulty_naming ? D_SELECTED : 0;			//Use FoF difficulty naming

	eof_log("\tLaunching preferences dialog", 2);

	do
	{	//Run the dialog
		retval = eof_popup_dialog(eof_preferences_dialog, 0);	//Run the dialog
		if(retval == 1)
		{	//If the user clicked OK, update EOF's configured settings from the dialog selections
			eof_log("\t\"OK\" clicked.  Applying changes if any.", 2);

			if(eof_preferences_dialog[5].flags == D_SELECTED)
			{	//User opted to display note names at top of 2D panel
				eof_2d_render_top_option = 5;
			}
			else if(eof_preferences_dialog[6].flags == D_SELECTED)
			{	//User opted to display section names at top of 2D panel
				eof_2d_render_top_option = 6;
			}
			else if(eof_preferences_dialog[7].flags == D_SELECTED)
			{	//User opted to display fret hand positions at top of 2D panel
				eof_2d_render_top_option = 7;
			}
			else if(eof_preferences_dialog[8].flags == D_SELECTED)
			{	//User opted to display Rocksmith sections at top of 2D panel
				eof_2d_render_top_option = 8;
			}
			else if(eof_preferences_dialog[9].flags == D_SELECTED)
			{	//User opted to display both Rocksmith sections and Rocksmith phrases at top of 2D panel
				eof_2d_render_top_option = 9;
			}
			else if(eof_preferences_dialog[10].flags == D_SELECTED)
			{	//User opted to display tone changes at top of 2D panel
				eof_2d_render_top_option = 10;
			}
			eof_inverted_notes = (eof_preferences_dialog[11].flags == D_SELECTED ? 1 : 0);
			eof_lefty_mode = (eof_preferences_dialog[12].flags == D_SELECTED ? 1 : 0);
			eof_note_auto_adjust = (eof_preferences_dialog[13].flags == D_SELECTED ? 1 : 0);
			eof_enable_logging = (eof_preferences_dialog[14].flags == D_SELECTED ? 1 : 0);
			eof_hide_drum_tails = (eof_preferences_dialog[15].flags == D_SELECTED ? 1 : 0);
			eof_hide_note_names = (eof_preferences_dialog[16].flags == D_SELECTED ? 1 : 0);
			eof_disable_sound_processing = (eof_preferences_dialog[17].flags == D_SELECTED ? 1 : 0);
			eof_disable_3d_rendering = (eof_preferences_dialog[18].flags == D_SELECTED ? 1 : 0);
			eof_disable_2d_rendering = (eof_preferences_dialog[19].flags == D_SELECTED ? 1 : 0);
			eof_disable_info_panel = (eof_preferences_dialog[20].flags == D_SELECTED ? 1 : 0);
			eof_paste_erase_overlap = (eof_preferences_dialog[21].flags == D_SELECTED ? 1 : 0);
			eof_note_tails_clickable = (eof_preferences_dialog[22].flags == D_SELECTED ? 1 : 0);
			eof_drum_modifiers_affect_all_difficulties = (eof_preferences_dialog[23].flags == D_SELECTED ? 1 : 0);
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
				if(eof_min_note_distance > 999)
				{	//Validate this value (no more than 3 digits)
					eof_min_note_distance = 3;
				}
			}
			else
			{
				eof_min_note_distance = 3;
			}
			eof_render_bass_drum_in_lane = (eof_preferences_dialog[28].flags == D_SELECTED ? 1 : 0);
			eof_fb_seek_controls = (eof_preferences_dialog[29].flags == D_SELECTED ? 1 : 0);
			eof_input_mode = eof_preferences_dialog[31].d1;
			eof_color_set = eof_preferences_dialog[33].d1;
			eof_new_note_length_1ms = (eof_preferences_dialog[34].flags == D_SELECTED ? 1 : 0);
			eof_dont_redraw_on_exit_prompt = (eof_preferences_dialog[35].flags == D_SELECTED ? 1 : 0);
			eof_rewind_at_end = (eof_preferences_dialog[36].flags == D_SELECTED ? 1 : 0);
			eof_display_seek_pos_in_seconds = (eof_preferences_dialog[37].flags == D_SELECTED ? 1 : 0);
			eof_add_new_notes_to_selection = (eof_preferences_dialog[38].flags == D_SELECTED ? 1 : 0);
			eof_dont_auto_edit_new_lyrics = (eof_preferences_dialog[39].flags == D_SELECTED ? 1 : 0);
			eof_click_changes_dialog_focus = (eof_preferences_dialog[40].flags == D_SELECTED ? 1 : 0);
			if(eof_click_changes_dialog_focus)
				gui_mouse_focus = 0;
			else
				gui_mouse_focus = 1;
			eof_stop_playback_leave_focus = (eof_preferences_dialog[41].flags == D_SELECTED ? 1 : 0);
			if(eof_etext3[0] != '\0')
			{	//If the chord density threshold field is populated
				eof_chord_density_threshold = atol(eof_etext3);
				if(eof_chord_density_threshold > 99999)
				{	//Validate this value (not expected to be more than 5 digits)
					eof_chord_density_threshold = 10000;
				}
			}
			else
			{
				eof_chord_density_threshold = 0;
			}
			if(!eof_min_note_distance_intervals && ((original_eof_min_note_distance != eof_min_note_distance) || (original_eof_chord_density_threshold != eof_chord_density_threshold)))
			{	//If the minimum note distance or chord density threshold values were changed, and the minimum note distance setting is in ms
				if(eof_chord_density_threshold < eof_min_note_distance)
				{	//If EOF is configured to truncate note tails to enforce a distance longer than the chord density threshold
					allegro_message("Warning:  Configuring the minimum note distance to be longer than the chord density\nthreshold will eliminate chord repeat lines for all chords that aren't close enough to\neach others' start positions.");
				}
			}
			eof_prefer_midi_friendly_grid_snapping = (eof_preferences_dialog[44].flags == D_SELECTED ? 1 : 0);
			eof_ghl_conversion_swaps_bw_gems = (eof_preferences_dialog[45].flags == D_SELECTED ? 1 : 0);
			eof_enable_open_strums_by_default = (eof_preferences_dialog[46].flags == D_SELECTED ? 1 : 0);
			eof_disable_backups = (eof_preferences_dialog[47].flags == D_SELECTED ? 1 : 0);
			eof_section_auto_adjust = (eof_preferences_dialog[48].flags == D_SELECTED ? 1 : 0);
			if(eof_preferences_dialog[49].flags == D_SELECTED)
			{	//min. note distance is in ms
				eof_min_note_distance_intervals = 0;
			}
			else if(eof_preferences_dialog[50].flags == D_SELECTED)
			{	//min. note distance is 1/# measure
				eof_min_note_distance_intervals = 1;
			}
			else
			{	//min. note distance is 1/# beat
				eof_min_note_distance_intervals = 2;
			}
			eof_new_note_forced_strum = (eof_preferences_dialog[52].flags == D_SELECTED ? 1 : 0);
			eof_use_fof_difficulty_naming = (eof_preferences_dialog[53].flags == D_SELECTED ? 1 : 0);
			if(eof_use_fof_difficulty_naming)
			{
				eof_note_type_name = eof_note_type_name_fof;
			}
			else
			{
				eof_note_type_name = eof_note_type_name_rb;
			}
			eof_set_2D_lane_positions(0);	//Update ychart[] by force just in case eof_inverted_notes was changed
			eof_set_3D_lane_positions(0);	//Update xchart[] by force just in case eof_lefty_mode was changed
		}//If the user clicked OK
		else if(retval == 2)
		{	//If the user clicked "Default, change all selections to EOF's default settings
			eof_log("\t\"Default\" clicked.  Applying default settings.", 2);

			eof_preferences_dialog[5].flags = D_SELECTED;			//Display note names at top of 2D panel
			eof_preferences_dialog[6].flags = eof_preferences_dialog[7].flags = eof_preferences_dialog[8].flags = eof_preferences_dialog[9].flags = eof_preferences_dialog[10].flags = 0;
			eof_preferences_dialog[11].flags = 0;					//Inverted notes
			eof_preferences_dialog[12].flags = 0;					//Lefty mode
			eof_preferences_dialog[13].flags = D_SELECTED;			//Note auto adjust
			eof_preferences_dialog[14].flags = D_SELECTED;			//Enable logging
			eof_preferences_dialog[15].flags = 0;					//Hide drum note tails
			eof_preferences_dialog[16].flags = 0;					//Hide note names
			eof_preferences_dialog[17].flags = 0;					//Disable sound effects
			eof_preferences_dialog[18].flags = 0;					//Disable 3D rendering
			eof_preferences_dialog[19].flags = 0;					//Disable 2D rendering
			eof_preferences_dialog[20].flags = 0;					//Disable info panel
			eof_preferences_dialog[21].flags = 0;					//Paste erases overlap
			eof_preferences_dialog[22].flags = 0;					//Make note tails clickable
			eof_preferences_dialog[23].flags = D_SELECTED;			//Drum modifiers affect all diff's
			eof_etext2[0] = '3';									//Min. note distance
			eof_etext2[1] = '\0';
			eof_etext[0] = '\0';									//Min. note length
			eof_preferences_dialog[28].flags = 0;					//3D render bass drum in a lane
			eof_preferences_dialog[29].flags = 0;					//dB style seek controls
			eof_preferences_dialog[31].d1 = EOF_INPUT_REX;			//Input method
			eof_preferences_dialog[33].d1 = EOF_COLORS_DEFAULT;	//Color set
			eof_preferences_dialog[34].flags = 0;					//New notes are made 1ms long
			eof_preferences_dialog[35].flags = 0;					//Don't redraw on exit prompt
			eof_preferences_dialog[36].flags = D_SELECTED;			//Rewind when playback is at end
			eof_preferences_dialog[37].flags = 0;					//Display seek pos. in seconds
			eof_preferences_dialog[38].flags = 0;					//Add new notes to selection
			eof_preferences_dialog[39].flags = 0;					//Don't auto edit new lyrics
			eof_preferences_dialog[40].flags = D_SELECTED;			//Click to change dialog focus
			eof_preferences_dialog[41].flags = D_SELECTED;			//EOF leaving focus stops playback
			(void) snprintf(eof_etext3, sizeof(eof_etext3) - 1, "10000");	//Chord density threshold
			eof_preferences_dialog[44].flags = D_SELECTED;			//Prefer MIDI friendly grid snaps
			eof_preferences_dialog[45].flags = 0;					//GHL conversion swaps B/W gems
			eof_preferences_dialog[46].flags = 0;					//Enable open strum by default
			eof_preferences_dialog[47].flags = 0;					//Disable automatic backups
			eof_preferences_dialog[48].flags = D_SELECTED;			//Auto-Adjust sections/FHPs
			eof_preferences_dialog[49].flags = D_SELECTED;			//min. note distance is in ms
			eof_preferences_dialog[50].flags = 0;					//min. note distance is 1/# measure
			eof_preferences_dialog[51].flags = 0;					//min. note distance is 1/# beat
			eof_preferences_dialog[52].flags = 0;					//New notes are force strum
			eof_preferences_dialog[53].flags = 0;					//Use FoF difficulty naming
		}//If the user clicked "Default
	}while(retval == 2);	//Keep re-running the dialog until the user closes it with anything besides "Default"

	eof_log("\tPreferences dialog closed.  Cleaning up.", 2);

	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_set_color_set();
	if(original_input_mode != eof_input_mode)
	{	//If the input mode was changed
		eof_seek_selection_start = eof_seek_selection_end = 0;	//Clear the seek selection
	}
	if(original_eof_disable_info_panel != eof_disable_info_panel)
	{	//If the hiding of the information panel was changed
		eof_log("\tToggling info panel", 2);
		eof_disable_info_panel = 1 - eof_disable_info_panel;	//Toggle this because the function call below will toggle it again
		eof_display_info_panel();	//Create/destroy the information panel instance accordingly
	}
	if((original_eof_prefer_midi_friendly_grid_snapping != eof_prefer_midi_friendly_grid_snapping) && eof_song->tags->highlight_unsnapped_notes)
	{	//If the "Prefer MIDI friendly grid snaps" preference changed, and unsnapped notes are to be highlighted
		for(ctr = 1; ctr < eof_song->tracks; ctr++)
		{	//For each track
			eof_track_remove_highlighting(eof_song, ctr, 1);	//Remove existing temporary highlighting from the track
			eof_song_highlight_non_grid_snapped_notes(eof_song, ctr);	//Re-create the non grid snapped highlighting as appropriate
		}
	}
	(void) eof_increase_display_width_to_panel_count(1);	//Prompt to resize the program window if necessary, disable notes panel if resulting width is insufficient
	eof_rebuild_notes_window();					//Recreate the notes panel window to fill all available space in the bottom half of the program window

	eof_log("Preferences logic exiting.", 2);
	return 1;
}

int eof_menu_file_import_export_preferences(void)
{
	int retval, original_rs1_export_setting;

	if(eof_song_loaded)
	{
		if(!eof_music_paused)
		{
			eof_music_play(0);
		}
	}
	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	eof_color_dialog(eof_import_export_preferences_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_import_export_preferences_dialog);
	//Use the currently configured settings to populate the dialog selections
	eof_import_export_preferences_dialog[4].flags = eof_write_rb_files ? D_SELECTED : 0;						//Save separate Rock Band files
	eof_import_export_preferences_dialog[5].flags = eof_write_music_midi ? D_SELECTED : 0;					//Save separate musical MIDI files
	eof_import_export_preferences_dialog[6].flags = eof_write_rs_files ? D_SELECTED : 0;						//Save separate Rocksmith 1 files
	original_rs1_export_setting = eof_write_rs_files;	//Back up this setting to track whether it is changed in the dialog
	eof_import_export_preferences_dialog[7].flags = eof_write_rs2_files ? D_SELECTED : 0;					//Save separate Rocksmith 2 files
	eof_import_export_preferences_dialog[8].flags = eof_write_bf_files ? D_SELECTED : 0;						//Save separate Bandfuse files
	eof_import_export_preferences_dialog[9].flags = eof_write_fof_files ? D_SELECTED : 0;					//Save FoF/CH/Phase Shift files
	eof_import_export_preferences_dialog[10].flags = eof_write_gh_files ? D_SELECTED : 0;					//Save separate Guitar Hero files
	eof_import_export_preferences_dialog[11].flags = eof_write_immerrock_files ? D_SELECTED : 0;				//Save separate IMMERROCK files
	eof_import_export_preferences_dialog[12].flags = eof_write_lrc_files ? D_SELECTED : 0;					//Save LRC, ELRC, QRC lyric files
	eof_import_export_preferences_dialog[13].flags = eof_force_pro_drum_midi_notation ? D_SELECTED : 0;		//Force pro drum MIDI notation
	eof_import_export_preferences_dialog[14].flags = eof_gp_import_truncate_short_notes ? D_SELECTED : 0;	//GP import truncates short notes
	eof_import_export_preferences_dialog[15].flags = eof_rbn_export_slider_hopo ? D_SELECTED : 0;			//RBN export slider as HOPO
	eof_import_export_preferences_dialog[16].flags = eof_gp_import_truncate_short_chords ? D_SELECTED : 0;	//GP import truncates short chords
	eof_import_export_preferences_dialog[17].flags = eof_gp_import_replaces_track ? D_SELECTED : 0;			//GP import replaces active track
	eof_import_export_preferences_dialog[18].flags = eof_imports_drop_mid_beat_tempos ? D_SELECTED : 0;	//Imports drop mid beat tempos
	eof_import_export_preferences_dialog[19].flags = eof_gp_import_nat_harmonics_only ? D_SELECTED : 0;		//GP import nat. harmonics only
	eof_import_export_preferences_dialog[20].flags = eof_imports_recall_last_path ? D_SELECTED : 0;			//Import dialogs recall last path
	eof_import_export_preferences_dialog[21].flags = eof_use_ts ? D_SELECTED : 0;							//Import/Export TS
	eof_import_export_preferences_dialog[22].flags = eof_db_import_suppress_5nc_conversion ? D_SELECTED : 0;//dB import skips 5nc conversion
	eof_import_export_preferences_dialog[23].flags = eof_warn_missing_bass_fhps ? D_SELECTED : 0;			//Warn about missing bass FHPs
	eof_import_export_preferences_dialog[24].flags = eof_abridged_rs2_export ? D_SELECTED : 0;				//Abridged Rocksmith 2 export
	eof_import_export_preferences_dialog[25].flags = eof_rs2_export_extended_ascii_lyrics ? D_SELECTED : 0;		//Allow RS2 extended ASCII lyrics
	eof_import_export_preferences_dialog[26].flags = eof_disable_ini_difference_warnings ? D_SELECTED : 0;		//Don't warn about INI differences
	eof_import_export_preferences_dialog[27].flags = eof_render_mid_beat_tempos_blue ? D_SELECTED : 0;		//Render mid beat tempos blue
	eof_import_export_preferences_dialog[28].flags = eof_gp_import_preference_1 ? D_SELECTED : 0;			//GP import beat text as sections, markers as phrases
	eof_import_export_preferences_dialog[29].flags = eof_rs2_export_version_8 ? D_SELECTED : 0;				//RS2 export version 8 (DLC Builder) style XML
	eof_import_export_preferences_dialog[30].flags = eof_rs_import_all_handshapes ? D_SELECTED : 0;			//RS import loads all handshapes
	eof_import_export_preferences_dialog[31].flags = eof_disable_ini_export ? D_SELECTED : 0;				//Don't write INI file
	eof_import_export_preferences_dialog[32].flags = eof_midi_export_enhanced_open_marker ? D_SELECTED : 0;//MIDI export CH/YARG open note/chord markers
	eof_import_export_preferences_dialog[33].flags = eof_gp_import_remove_accent_from_staccato ? D_SELECTED : 0;//GP import remove accent from staccato notes
	eof_import_export_preferences_dialog[34].flags = eof_offer_fhp_derived_finger_placements ? D_SELECTED : 0;//Offer to derive finger placements from FHPs
	eof_import_export_preferences_dialog[35].flags = eof_gh_import_sustain_threshold_prompt ? D_SELECTED : 0;//GH import sustain threshold prompt
	eof_import_export_preferences_dialog[36].flags = eof_disable_rs_wav ? D_SELECTED : 0;					//Don't write Rocksmith WAV file

	do
	{	//Run the dialog
		retval = eof_popup_dialog(eof_import_export_preferences_dialog, 0);	//Run the dialog
		if(retval == 1)
		{	//If the user clicked OK, update EOF's configured settings from the dialog selections
			eof_write_rb_files = (eof_import_export_preferences_dialog[4].flags == D_SELECTED ? 1 : 0);
			eof_write_music_midi = (eof_import_export_preferences_dialog[5].flags == D_SELECTED ? 1 : 0);
			eof_write_rs_files = (eof_import_export_preferences_dialog[6].flags == D_SELECTED ? 1 : 0);
			eof_write_rs2_files = (eof_import_export_preferences_dialog[7].flags == D_SELECTED ? 1 : 0);
			eof_write_bf_files = (eof_import_export_preferences_dialog[8].flags == D_SELECTED ? 1 : 0);
			eof_write_fof_files = (eof_import_export_preferences_dialog[9].flags == D_SELECTED ? 1 : 0);
			eof_write_gh_files = (eof_import_export_preferences_dialog[10].flags == D_SELECTED ? 1 : 0);
			eof_write_immerrock_files = (eof_import_export_preferences_dialog[11].flags == D_SELECTED ? 1 : 0);
			eof_write_lrc_files = (eof_import_export_preferences_dialog[12].flags == D_SELECTED ? 1 : 0);
			eof_force_pro_drum_midi_notation = (eof_import_export_preferences_dialog[13].flags == D_SELECTED ? 1 : 0);
			eof_gp_import_truncate_short_notes = (eof_import_export_preferences_dialog[14].flags == D_SELECTED ? 1 : 0);
			eof_rbn_export_slider_hopo = (eof_import_export_preferences_dialog[15].flags == D_SELECTED ? 1 : 0);
			eof_gp_import_truncate_short_chords = (eof_import_export_preferences_dialog[16].flags == D_SELECTED ? 1 : 0);
			eof_gp_import_replaces_track = (eof_import_export_preferences_dialog[17].flags == D_SELECTED ? 1 : 0);
			eof_imports_drop_mid_beat_tempos = (eof_import_export_preferences_dialog[18].flags == D_SELECTED ? 1 : 0);
			eof_gp_import_nat_harmonics_only = (eof_import_export_preferences_dialog[19].flags == D_SELECTED ? 1 : 0);
			eof_imports_recall_last_path = (eof_import_export_preferences_dialog[20].flags == D_SELECTED ? 1 : 0);
			eof_use_ts = (eof_import_export_preferences_dialog[21].flags == D_SELECTED ? 1 : 0);
			eof_db_import_suppress_5nc_conversion = (eof_import_export_preferences_dialog[22].flags == D_SELECTED ? 1 : 0);
			eof_warn_missing_bass_fhps = (eof_import_export_preferences_dialog[23].flags == D_SELECTED ? 1 : 0);
			eof_abridged_rs2_export = (eof_import_export_preferences_dialog[24].flags == D_SELECTED ? 1 : 0);
			eof_rs2_export_extended_ascii_lyrics = (eof_import_export_preferences_dialog[25].flags == D_SELECTED ? 1 : 0);
			eof_disable_ini_difference_warnings = (eof_import_export_preferences_dialog[26].flags == D_SELECTED ? 1 : 0);
			eof_render_mid_beat_tempos_blue = (eof_import_export_preferences_dialog[27].flags == D_SELECTED ? 1 : 0);
			eof_gp_import_preference_1 = (eof_import_export_preferences_dialog[28].flags == D_SELECTED ? 1 : 0);
			eof_rs2_export_version_8 = (eof_import_export_preferences_dialog[29].flags == D_SELECTED ? 1 : 0);
			eof_rs_import_all_handshapes = (eof_import_export_preferences_dialog[30].flags == D_SELECTED ? 1 : 0);
			eof_disable_ini_export = (eof_import_export_preferences_dialog[31].flags == D_SELECTED ? 1 : 0);
			eof_midi_export_enhanced_open_marker = (eof_import_export_preferences_dialog[32].flags == D_SELECTED ? 1 : 0);
			eof_gp_import_remove_accent_from_staccato = (eof_import_export_preferences_dialog[33].flags == D_SELECTED ? 1 : 0);
			eof_offer_fhp_derived_finger_placements = (eof_import_export_preferences_dialog[34].flags == D_SELECTED ? 1 : 0);
			eof_gh_import_sustain_threshold_prompt = (eof_import_export_preferences_dialog[35].flags == D_SELECTED ? 1 : 0);
			eof_disable_rs_wav = (eof_import_export_preferences_dialog[36].flags == D_SELECTED ? 1 : 0);
		}//If the user clicked OK
		else if(retval == 2)
		{	//If the user clicked "Default, change all selections to EOF's default settings
			eof_import_export_preferences_dialog[4].flags = 0;				//Save separate Rock Band files
			eof_import_export_preferences_dialog[5].flags = 0;				//Save separate musical MIDI files
			eof_import_export_preferences_dialog[6].flags = 0;				//Save separate Rocksmith 1 files
			eof_import_export_preferences_dialog[7].flags = D_SELECTED;		//Save separate Rocksmith 2 files
			eof_import_export_preferences_dialog[8].flags = 0;				//Save separate Bandfuse files
			eof_import_export_preferences_dialog[9].flags = D_SELECTED;		//Save FoF/CH/Phase Shift files
			eof_import_export_preferences_dialog[10].flags = 0;				//Save separate Guitar Hero files
			eof_import_export_preferences_dialog[11].flags = 0;				//Save separate IMMERROCK files
			eof_import_export_preferences_dialog[12].flags = 0;				//Save LRC, ELRC, QRC lyric files
			eof_import_export_preferences_dialog[13].flags = D_SELECTED;		//Force pro drum MIDI notation
			eof_import_export_preferences_dialog[14].flags = D_SELECTED;		//GP import truncates short notes
			eof_import_export_preferences_dialog[15].flags = 0;				//RBN export slider as HOPO
			eof_import_export_preferences_dialog[16].flags = D_SELECTED;		//GP import truncates short chords
			eof_import_export_preferences_dialog[17].flags = D_SELECTED;		//GP import replaces active track
			eof_import_export_preferences_dialog[18].flags = 0;				//Imports drop mid beat tempos
			eof_import_export_preferences_dialog[19].flags = 0;				//GP import nat. harmonics only
			eof_import_export_preferences_dialog[20].flags = 0;				//Import dialogs recall last path
			eof_import_export_preferences_dialog[21].flags = D_SELECTED;		//Import/Export TS
			eof_import_export_preferences_dialog[22].flags = 0;				//dB import skips 5nc conversion
			eof_import_export_preferences_dialog[23].flags = D_SELECTED;		//Warn about missing bass FHPs
			eof_import_export_preferences_dialog[24].flags = D_SELECTED;		//Abridged Rocksmith 2 export
			eof_import_export_preferences_dialog[25].flags = D_SELECTED;		//Allow RS2 extended ASCII lyrics
			eof_import_export_preferences_dialog[26].flags = 0;				//Don't warn about INI differences
			eof_import_export_preferences_dialog[27].flags = D_SELECTED;		//Render mid beat tempos blue
			eof_import_export_preferences_dialog[28].flags = 0;				//GP import beat text as sections, markers as phrases
			eof_import_export_preferences_dialog[29].flags = 0;				//RS2 export version 8 (DLC Builder) style XML
			eof_import_export_preferences_dialog[30].flags = 0;				//RS import loads all handshapes
			eof_import_export_preferences_dialog[31].flags = 0;				//Don't write INI file
			eof_import_export_preferences_dialog[32].flags = 0;				//MIDI export CH/YARG open note/chord markers
			eof_import_export_preferences_dialog[33].flags = 0;				//GP import remove accent from staccato notes
			eof_import_export_preferences_dialog[34].flags = 0;				//Offer to derive finger placements from FHPs
			eof_import_export_preferences_dialog[35].flags = 0;				//GH import sustain threshold prompt
			eof_import_export_preferences_dialog[36].flags = 0;				//Don't write Rocksmith WAV file
		}//If the user clicked "Default
	}while(retval == 2);	//Keep re-running the dialog until the user closes it with anything besides "Default"
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_set_color_set();
	if(original_rs1_export_setting != eof_write_rs_files)
	{	//If the preference for exporting RS1 file was changed
		eof_delete_rocksmith_wav();	//Delete the Rocksmith WAV file since the amount of silence appended to it will differ
	}
	return 1;
}

int eof_menu_file_gp_preferences(void)
{
	int retval;

	eof_log("Guitar Pro Preferences logic starting", 2);

	if(eof_song_loaded)
	{
		if(!eof_music_paused)
		{
			eof_music_play(0);
		}
	}
	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	eof_color_dialog(eof_pg_preferences_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_pg_preferences_dialog);

	//Use the currently configured settings to populate the dialog selections
	eof_pg_preferences_dialog[4].flags = eof_enforce_chord_density ? D_SELECTED : 0;			//Apply crazy to repeated chords separated by a rest
	eof_pg_preferences_dialog[5].flags = eof_inverted_chords_slash ? D_SELECTED : 0;			//Treat inverted chords as slash
	eof_pg_preferences_dialog[6].flags = eof_render_3d_rs_chords ? D_SELECTED : 0;			//3D render RS style chords
	eof_pg_preferences_dialog[7].flags = eof_auto_complete_fingering ? D_SELECTED : 0;		//Offer to auto complete fingering
	eof_pg_preferences_dialog[8].flags = eof_dont_auto_name_double_stops ? D_SELECTED : 0;	//Don't auto-name double stops
	eof_pg_preferences_dialog[9].flags = eof_technote_auto_adjust ? D_SELECTED : 0;			//Auto-Adjust tech notes
	eof_pg_preferences_dialog[10].flags = eof_fingering_checks_include_mutes ? D_SELECTED : 0;	//Fingering checks include mutes
	eof_pg_preferences_dialog[11].flags = eof_render_2d_rs_piano_roll ? D_SELECTED : 0;			//2D render RS piano roll

	eof_log("\tLaunching Guitar Pro Preferences dialog", 2);

	do
	{	//Run the dialog
		retval = eof_popup_dialog(eof_pg_preferences_dialog, 0);	//Run the dialog
		if(retval == 1)
		{	//If the user clicked OK, update EOF's configured settings from the dialog selections
			eof_log("\t\"OK\" clicked.  Applying changes if any.", 2);

			eof_enforce_chord_density = (eof_pg_preferences_dialog[4].flags == D_SELECTED ? 1 : 0);
			eof_inverted_chords_slash = (eof_pg_preferences_dialog[5].flags == D_SELECTED ? 1 : 0);
			eof_render_3d_rs_chords = (eof_pg_preferences_dialog[6].flags == D_SELECTED ? 1 : 0);
			eof_auto_complete_fingering = (eof_pg_preferences_dialog[7].flags == D_SELECTED ? 1 : 0);
			eof_dont_auto_name_double_stops = (eof_pg_preferences_dialog[8].flags == D_SELECTED ? 1 : 0);
			eof_technote_auto_adjust = (eof_pg_preferences_dialog[9].flags == D_SELECTED ? 1 : 0);
			eof_fingering_checks_include_mutes = (eof_pg_preferences_dialog[10].flags == D_SELECTED ? 1 : 0);
			eof_render_2d_rs_piano_roll = (eof_pg_preferences_dialog[11].flags == D_SELECTED ? 1 : 0);
		}//If the user clicked OK
		else if(retval == 2)
		{	//If the user clicked "Default, change all selections to EOF's default settings
			eof_log("\t\"Default\" clicked.  Applying default settings.", 2);

			eof_pg_preferences_dialog[4].flags = 0;					//Apply crazy to repeated chords separated by a rest
			eof_pg_preferences_dialog[5].flags = 0;					//Treat inverted chords as slash
			eof_pg_preferences_dialog[6].flags = 0;					//3D render RS style chords
			eof_pg_preferences_dialog[7].flags = D_SELECTED;		//Offer to auto complete fingering
			eof_pg_preferences_dialog[8].flags = 0;					//Don't auto-name double stops
			eof_pg_preferences_dialog[9].flags = D_SELECTED;		//Auto-Adjust tech notes
			eof_pg_preferences_dialog[10].flags = 0;				//Fingering checks include mutes
			eof_pg_preferences_dialog[11].flags = 0;				//2D render RS piano roll
		}//If the user clicked "Default
	}while(retval == 2);	//Keep re-running the dialog until the user closes it with anything besides "Default"

	eof_log("\tGuitar Pro Preferences dialog closed.  Cleaning up.", 2);

	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_set_color_set();

	eof_log("Guitar Pro Preferences logic exiting.", 2);
	return 1;
}

int eof_menu_file_display(void)
{
	int maxwidth = 0, maxheight = 0;
	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	eof_color_dialog(eof_display_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_display_dialog);
	eof_display_dialog[4].d1 = eof_screen_layout.mode;
	eof_display_dialog[1].flags = eof_soft_cursor ? D_SELECTED : 0;
	eof_display_dialog[2].flags = (eof_desktop == 0) ? D_SELECTED : 0;
	if(eof_popup_dialog(eof_display_dialog, 4) == 5)
	{	//User clicked OK
		eof_soft_cursor = (eof_display_dialog[1].flags & D_SELECTED) ? 1 : 0;
		eof_desktop = (eof_display_dialog[2].flags & D_SELECTED) ? 0 : 1;
		if(eof_display_dialog[4].d1 == EOF_DISPLAY_CUSTOM)
		{	//User selected "Custom" window size
			(void) snprintf(eof_etext, sizeof(eof_etext) - 1, "Currently set to %lux%lu", eof_screen_width, eof_screen_height);
			if(!get_desktop_resolution(&maxwidth, &maxheight))
			{	//If the desktop resolution could be read
				(void) snprintf(eof_etext2, sizeof(eof_etext) - 1, "Screen resolution is %dx%d", maxwidth, maxheight);
			}
			else
			{
				eof_etext2[0] = '\0';	//Empty this string
				maxwidth = maxheight = INT_MAX;	//Set these to be high enough that they won't limit the user's input
			}
			eof_etext3[0] = '\0';	//Empty these input fields
			eof_etext4[0] = '\0';
			eof_color_dialog(eof_custom_display_size_dialog, gui_fg_color, gui_bg_color);
			eof_conditionally_center_dialog(eof_custom_display_size_dialog);
			if(eof_popup_dialog(eof_custom_display_size_dialog, 4) == 7)
			{	//User clicked OK
				unsigned long width, height, oldwidth, oldheight;
				int cancelled = 0;

				width = atol(eof_etext3);
				height = atol(eof_etext4);
				oldwidth = eof_screen_width;
				oldheight = eof_screen_height;
				if((width >= 640) && (height >= 480))
				{	//If a sufficiently high width and height were specified
					if((width >= maxwidth) || (height >= maxheight))
					{	//User specified a resolution that won't fit on screen
						allegro_message("Must specify a width and height smaller than the screen resolution (%d x %d)", maxwidth, maxheight);
						cancelled = 1;
					}
					else if(width % 4 != 0)
					{	//Allegro requires the window width to be a multiple of 4
						unsigned long suggestion1, suggestion2;
						char string1[10], string2[10];
						int ret;

						suggestion1 = width - (width % 4);	//The nearest multiple of 4 before the user specified width
						suggestion2 = suggestion1 + 4;		//The next multiple of 4 after that
						(void) snprintf(string1, sizeof(string1) - 1, "%lu", suggestion1);
						(void) snprintf(string2, sizeof(string1) - 1, "%lu", suggestion2);
						ret = alert3(NULL, "The window width must be a multiple of 4.", "Try one of these suggested widths.", string1, string2, "Cancel", 0, 0, 0);
						if(ret == 1)
							width = suggestion1;
						else if(ret == 2)
							width = suggestion2;
						else
							cancelled = 1;
					}

					if(!cancelled)
					{	//If the user didn't cancel the operation
						if(!eof_set_display_mode(width, height) || (eof_screen_width != width) || (eof_screen_height != height))
						{	//If the user specified display size could not be set
							eof_apply_display_settings(EOF_DISPLAY_640);		//Formally reload this resolution
							(void) eof_set_display_mode(oldwidth, oldheight);	//Revert to the previous resolution in use
						}
					}
					eof_scale_fretboard(0);			//Recalculate the 2D screen positioning based on the current track
					eof_set_2D_lane_positions(0);	//Update ychart[]
					eof_set_3D_lane_positions(0);	//Update xchart[]
				}
			}
		}
		else
		{	//Use a pre-set window size
			eof_apply_display_settings(eof_display_dialog[4].d1);
		}
		if(eof_screen_width < eof_get_display_panel_count() * EOF_SCREEN_PANEL_WIDTH)
		{	//If the current screen width isn't sufficient to display all enabled panels
			eof_enable_notes_panel = 0;	//Disable the notes panel
		}
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
	if(!eof_set_display_mode_preset(mode))
	{
		allegro_message("Unable to set display mode, reverting!");
		(void) eof_set_display_mode_preset(eof_screen_layout.mode);
	}
	set_palette(eof_palette);
	if(!eof_soft_cursor)
	{	//If not using the software mouse
		if(show_os_cursor(2) < 0)
		{	//If the OS mouse cannot be displayed
			eof_soft_cursor = 1;	//Revert to the software mouse
		}
	}
	else
	{
		(void) show_os_cursor(0);
	}
	if(!eof_load_data())
	{	//Load images AFTER the graphics mode has been set, to avoid performance issues in Allegro
		allegro_message("Could not reload program data!");
		exit(0);
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
	eof_conditionally_center_dialog(eof_ogg_settings_dialog);
	eof_ogg_settings_dialog[3].d1 = eof_ogg_setting;
	if(eof_popup_dialog(eof_ogg_settings_dialog, 0) == 4)
	{	//User clicked OK
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
	eof_conditionally_center_dialog(eof_controller_settings_dialog);
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
	int retval = 0;

	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	returnfolder = ncd_folder_select("Set EOF Song Folder");
	if(returnfolder)
	{
		(void) ustrcpy(eof_songs_path, returnfolder);
		retval = 1;
	}
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;

	return retval;
}

int eof_menu_file_link(unsigned char application)
{
	char *fofdisplayname = "FoF";
	char *psdisplayname = "Phase Shift";
	char *rstotabdisplayname = "RocksmithToTab.exe";
	char titlebartext[100] = {0};
	char *appdisplayname = NULL;
	char * returnfolder = NULL;
	char * returnedfn = NULL;

	if(eof_song_loaded)
	{
		if(!eof_music_paused)
		{
			eof_music_play(0);
		}
	}
	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	if(application == 0)
	{	//User is linking to FoF
		appdisplayname = fofdisplayname;
	}
	else if(application == 1)
	{	//User is linking to Phase Shift
		appdisplayname = psdisplayname;
	}
	else
	{	//User is linking to RocksmithToTab
		appdisplayname = rstotabdisplayname;
	}
	(void) snprintf(titlebartext, sizeof(titlebartext) - 1, "Locate %s Executable", appdisplayname);
	returnedfn = ncd_file_select(0, NULL, titlebartext, eof_filter_exe_files);
	eof_clear_input();
	if(returnedfn)
	{
		if(application < 2)
		{	//If linking to either FoF or Phase Shift
			(void) snprintf(titlebartext, sizeof(titlebartext) - 1, "Locate %s Songs Folder", appdisplayname);
			returnfolder = ncd_folder_select(titlebartext);
			if(returnfolder)
			{
				if(application == 0)
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
		else
		{	//If linking to RocksmithToTab
			(void) ustrcpy(eof_rs_to_tab_executable_path, returnedfn);
			if(strcasestr_spec(returnedfn, "RocksmithToTabGUI.exe"))
			{	//If the selected file name indicates that the user picked the GUI program
				allegro_message("It appears you picked the GUI version of RocksmithToTab instead of the command line one (RocksmithToTab.exe).  Consider re-linking to the correct program if problems are encountered.");
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
	return eof_menu_file_link(0);	//Link to FoF
}

int eof_menu_file_link_ps(void)
{
	return eof_menu_file_link(1);	//Link to Phase Shift
}

int eof_menu_file_link_rs_to_tab(void)
{
	return eof_menu_file_link(2);	//Link to RocksmithToTab
}

int eof_menu_file_exit(void)
{
	int ret = 0;
	int ret2 = 1;

	if(eof_song_loaded)
	{
		if(!eof_music_paused)
		{
			eof_music_play(0);
		}
		if(eof_music_catalog_playback)
		{
			eof_music_catalog_playback = 0;
			eof_music_catalog_pos = eof_song->catalog->entry[eof_selected_catalog_entry].start_pos + eof_av_delay;
			eof_stop_midi();
			alogg_stop_ogg(eof_music_track);
			alogg_seek_abs_msecs_ogg_ul(eof_music_track, eof_music_pos.value);
		}
	}
	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	eof_clear_input();
	if(!eof_dont_redraw_on_exit_prompt)
	{	//If the user didn't disable the redrawing of the program window on exit prompt
		(void) eof_reset_display();
	}

	//Re-initialize the notes panel if it was destroyed when the program window was redrawn
	if(eof_enable_notes_panel)
	{
		eof_enable_notes_panel = 0;	//Toggle this because the function call below will toggle it back to on
		(void) eof_display_notes_panel();
		eof_render();
	}
	if(alert(NULL, "Want to Quit?", NULL, "&Yes", "&No", 'y', 'n') == 1)
	{
		if(eof_changes)
		{
			eof_clear_input();
			ret = alert3(NULL, "Quick save changes before quitting?", NULL, "&Yes", "&No", "Cancel", 'y', 'n', 0);
			if(ret == 1)
			{
				if(eof_menu_file_quick_save() > 1)
				{	//If the result was anything other than success or user cancellation
					eof_clear_input();
					ret2 = alert3(NULL, "Quick save failed! Exit without saving?", NULL, "&Yes", "&No", NULL, 'y', 'n', 0);
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

	eof_close_button_clicked = 0;	//User either canceled the close or eof_quit has been set
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
			if(size)
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

		default:
		break;
	}

	return NULL;
}

char * eof_ogg_list(int index, int * size)
{
	switch(index)
	{
		case -1:
		{
			if(size)
				*size = 7;
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
		case 6:
		{
			return "320kbps";
		}

		default:
		break;
	}

	return NULL;
}

int eof_ogg_list_bitrate(int index)
{
	switch(index)
	{
		case 0:
		{
			return 80;
		}
		case 1:
		{
			return 96;
		}
		case 2:
		{
			return 128;
		}
		case 3:
		{
			return 160;
		}
		case 4:
		{
			return 192;
		}
		case 5:
		{
			return 256;
		}
		case 6:
		{
			return 320;
		}

		default:
		break;
	}

	return 320;
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
	(void) snprintf(eof_ctext[7], sizeof(eof_ctext[7]) - 1, "Fret 6 (%s)", redefine_index == 7 ? "Press new key" : eof_guitar.button[7].name);
	switch(index)
	{
		case -1:
		{
			if(size)
				*size = 8;
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
		case 7:
		{
			return eof_ctext[7];
		}

		default:
		break;
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
			if(size)
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

		default:
		break;
	}

	return NULL;
}

char * eof_display_list(int index, int * size)
{
	static char custom[20] = {0};

	switch(index)
	{
		case -1:
		{
			if(size)
				*size = 4;
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
		case EOF_DISPLAY_CUSTOM:
		{
			(void) snprintf(custom, sizeof(custom) - 1, "Custom (%lux%lu)", eof_screen_width, eof_screen_height);
			return custom;
		}

		default:
		break;
	}

	return NULL;
}

int eof_controller_settings_guitar(DIALOG * d)
{
	if(!d)
	{	//Satisfy Splint by checking value of d
		return D_O_K;
	}
	eof_color_dialog(eof_guitar_settings_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_guitar_settings_dialog);
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
	if(!d)
	{	//Satisfy Splint by checking value of d
		return D_O_K;
	}
	eof_color_dialog(eof_drum_settings_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_drum_settings_dialog);
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

	if(!d)
	{	//Satisfy Splint by checking value of d
		return D_O_K;
	}
	redefine_index = eof_guitar_settings_dialog[3].d1;
	(void) dialog_message(eof_guitar_settings_dialog, MSG_DRAW, 0, &i);
	(void) eof_controller_set_button(&eof_guitar.button[eof_guitar_settings_dialog[3].d1]);
	eof_clear_input();
	redefine_index = -1;

	//Wait for user to release button 0 or 1 on the first controller to prevent Allegro from hijacking the controller input
	while((joy[0].button[0].b) || (joy[0].button[1].b))
	{
		(void) poll_joystick();
	}

	(void) dialog_message(eof_guitar_settings_dialog, MSG_DRAW, 0, &i);

	if(eof_test_controller_conflict(&eof_guitar,0,7))
		(void) alert("Warning", NULL, "There is a key conflict for this controller", "OK", NULL, 0, KEY_ENTER);

	return 0;
}

int eof_drum_controller_redefine(DIALOG * d)
{
	int i;

	if(!d)
	{	//Satisfy Splint by checking value of d
		return D_O_K;
	}
	redefine_index = eof_drum_settings_dialog[3].d1;
	(void) dialog_message(eof_drum_settings_dialog, MSG_DRAW, 0, &i);
	(void) eof_controller_set_button(&eof_drums.button[eof_drum_settings_dialog[3].d1]);
	eof_clear_input();
	redefine_index = -1;

	//Wait for user to release button 0 or 1 on the first controller to prevent Allegro from hijacking the controller input
	while((joy[0].button[0].b) || (joy[0].button[1].b))
	{
		(void) poll_joystick();
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
		ptr = GetDetectionNumber(lyricdetectionlist, index);
		if(ptr == NULL)
			return NULL;

		if(printf("%s\t->\t%lu Lyrics", ptr->track, ptr->count) + 1 > 1024)	//If for some abnormal reason, the lyric info is too long to display in 1024 characters,
			return ptr->track;												//return just the track name instead of allowing an overflow

		(void) snprintf(lyricdetectionstring, sizeof(lyricdetectionstring) - 1, "%s -> %lu Lyrics", ptr->track, ptr->count);	//Write a bounds-checked formatted string to the global lyricdetectionstring array
		return lyricdetectionstring;	//and return it to be displayed in the list box
	}

	//Otherwise return NULL and set *size to the number of items to display in the list
	for(ctr = 1, ptr = lyricdetectionlist; ptr->next != NULL; ptr = ptr->next, ctr++);	//Count the number of lyric formats detected
	if(size)
		*size = ctr;
	return NULL;
}

struct Lyric_Format *GetDetectionNumber(struct Lyric_Format *detectionlist, unsigned long number)
{
	unsigned long ctr;
	struct Lyric_Format *ptr;	//Linked list conductor

	if(!detectionlist)
		return NULL;

	ptr = detectionlist;	//Point at front of list
	for(ctr = 0; ctr < number; ctr++)
	{
		if(ptr->next == NULL)
			return NULL;

		ptr = ptr->next;
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
	eof_conditionally_center_dialog(eof_lyric_detections_dialog);				//Center it
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

int eof_test_controller_conflict(EOF_CONTROLLER *controller, int start, int stop)
{
	int ctr1;	//The counter for the key being tested
	int ctr2;	//The counter for the keys being tested against

	if(!controller)
		return 2;	//Return error

	for(ctr1 = start; ctr1 <= stop; ctr1++)	//For each key to test
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
	char returnedfn_path[1024] = {0}, *returnedfn = NULL;
	char *initial;
	int jumpcode = 0;

	eof_log("eof_menu_file_feedback_import() entered", 1);

	if(eof_menu_prompt_save_changes() == 3)
	{	//If user canceled closing the current, modified chart
		return 1;
	}
	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	if((eof_last_db_path[uoffset(eof_last_db_path, ustrlen(eof_last_db_path) - 1)] == '\\') || (eof_last_db_path[uoffset(eof_last_db_path, ustrlen(eof_last_db_path) - 1)] == '/'))
	{	//If the path ends in a separator
		eof_last_db_path[uoffset(eof_last_db_path, ustrlen(eof_last_db_path) - 1)] = '\0';	//Remove it
	}
	if(eof_imports_recall_last_path && eof_folder_exists(eof_last_db_path))
	{	//If the user chose for the Feedback import dialog to start at the path of the last imported Feedback file and that path is valid
		initial = eof_last_db_path;	//Use it
	}
	else
	{	//Otherwise start at the project's path
		initial = eof_last_eof_path;
	}
	returnedfn = ncd_file_select(0, initial, "Import Feedback Chart", eof_filter_dB_files);
	eof_clear_input();
	if(returnedfn)
	{
		strncpy(returnedfn_path, returnedfn, sizeof(returnedfn_path) - 1);	//Back up this path for later use
		(void) ustrcpy(eof_filename, returnedfn);
		jumpcode=setjmp(jumpbuffer); //Store environment/stack/etc. info in the jmp_buf array
		if(jumpcode!=0) //if program control returned to the setjmp() call above returning any nonzero value
		{	//Import failed
			(void) puts("Assert() handled successfully!");
			allegro_message("dB Chart import failed");
			eof_show_mouse(NULL);
			eof_cursor_visible = 1;
			eof_pen_visible = 1;
			return 1;
		}

		/* destroy loaded song */
		if(eof_song)
		{
			eof_destroy_song(eof_song);
			eof_song = NULL;
			eof_song_loaded = 0;
		}
		(void) eof_destroy_ogg();

		/* import chart */
		eof_song = eof_import_chart(returnedfn);
		if(eof_song)
		{
			eof_song_loaded = 1;
			eof_init_after_load(0);
			(void) replace_filename(eof_last_db_path, returnedfn_path, "", 1024);	//Set the last loaded Feedback file path
			eof_cleanup_beat_flags(eof_song);	//Update anchor flags as necessary for any time signature changes
			eof_song_enforce_mid_beat_tempo_change_removal();	//Remove mid beat tempo changes if applicable
			(void) eof_check_for_notes_preceding_sections(0);	//Warn if there are notes that precede the first section event, if there are any sections
		}
		else
		{
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

void EnumeratedBChartInfo(struct FeedbackChart *chart)
{
	int printflen;	//Used to store the length of a printed format string
	char *tempstr;	//Used to store a formatted print string
	char *chartinfo = NULL;

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
		if(tempstr)
		{
			(void) snprintf(tempstr, sizeof(tempstr) - 1, "\nOffset: %.3f",chart->offset);
			chartinfo=ResizedAppend(chartinfo,tempstr,1);
			free(tempstr);
		}
	}
	printflen=snprintf(NULL,0,"\nResolution: %lu",chart->resolution)+1;	//Find the number of characters needed to snprintf this string
	if(printflen > 0)
	{
		tempstr=malloc_err((size_t)printflen);
		if(tempstr)
		{
			(void) snprintf(tempstr, sizeof(tempstr) - 1,"\nResolution: %lu",chart->resolution);
			chartinfo=ResizedAppend(chartinfo,tempstr,1);
			free(tempstr);
		}
	}
	printflen=snprintf(NULL,0,"\n%lu Instrument tracks loaded",chart->tracksloaded)+1;	//Find the number of characters needed to snprintf this string
	if(printflen > 0)
	{
		tempstr=malloc_err((size_t)printflen);
		if(tempstr)
		{
			(void) snprintf(tempstr, sizeof(tempstr) - 1,"\n%lu Instrument tracks loaded",chart->tracksloaded);
			chartinfo=ResizedAppend(chartinfo,tempstr,1);
			free(tempstr);
		}
	}

	allegro_message("%s",chartinfo);
}

int eof_audio_to_ogg(char *file, char *directory, char *dest_name, char function)
{
	char syscommand[1024] = {0};
	char cfn[1024] = {0};
	char *src_name;

	eof_log("eof_audio_to_ogg() entered", 1);

	if((file == NULL) || (directory == NULL) || (dest_name == NULL))
		return 3;	//Return invalid filename

	src_name = get_filename(file);

	//Determine the name of the file to be created
	if(!function)
	{	//Name the file as guitar.ogg
		(void) snprintf(dest_name, 15, "guitar.ogg");
	}
	else
	{	//Allow song.*, drums.*, rhythm.* and vocals.* to retain that part of their name
		if(!ustricmp(src_name, "song.ogg") || !ustricmp(src_name, "song.mp3") || !ustricmp(src_name, "song.wav"))
			(void) snprintf(dest_name, 15, "song.ogg");
		else if(!ustricmp(src_name, "drums.ogg") || !ustricmp(src_name, "drums.mp3") || !ustricmp(src_name, "drums.wav"))
			(void) snprintf(dest_name, 15, "drums.ogg");
		else if(!ustricmp(src_name, "rhythm.ogg") || !ustricmp(src_name, "rhythm.mp3") || !ustricmp(src_name, "rhythm.wav"))
			(void) snprintf(dest_name, 15, "rhythm.ogg");
		else if(!ustricmp(src_name, "vocals.ogg") || !ustricmp(src_name, "vocals.mp3") || !ustricmp(src_name, "vocals.wav"))
			(void) snprintf(dest_name, 15, "vocals.ogg");
		else if(!ustricmp(src_name, "bass.ogg") || !ustricmp(src_name, "bass.mp3") || !ustricmp(src_name, "bass.wav"))
			(void) snprintf(dest_name, 15, "bass.ogg");
		else if(!ustricmp(src_name, "keys.ogg") || !ustricmp(src_name, "keys.mp3") || !ustricmp(src_name, "keys.wav"))
			(void) snprintf(dest_name, 15, "keys.ogg");
		else if(!ustricmp(src_name, "crowd.ogg") || !ustricmp(src_name, "crowd.mp3") || !ustricmp(src_name, "crowd.wav"))
			(void) snprintf(dest_name, 15, "crowd.ogg");
		else
			(void) snprintf(dest_name, 15, "guitar.ogg");	//If it doesn't match any of those names, it will be renamed as guitar.ogg
	}

	if(!ustricmp(src_name, dest_name))
	{	//If the input file's name is the same as the OGG file that is to be created
		(void) replace_filename(syscommand, file, "", 1024);	//Obtain the parent directory of the input file
		if(!ustricmp(syscommand,directory))				//Special case:  The input and output file are the same
		{
			return 0;									//Return success without altering the existing OGG file
		}
	}

	if(!ustricmp(get_extension(file), "mp3"))
	{	//Convert from MP3
		eof_log("\tConverting source MP3 file to OGG format", 1);

		//If an MP3 is to be encoded to OGG, store a copy of the MP3 as "original.mp3"
		if(ustricmp(syscommand,directory))
		{	//If the user did not select a file named original.mp3 in the chart's folder, check to see if a such-named file will be overwritten
			if(!eof_menu_file_new_supplement(directory, NULL, 2))
			{	//If the user declined to overwrite an existing "original.mp3" file at the destination path
				eof_cursor_visible = 1;
				eof_pen_visible = 1;
				eof_show_mouse(NULL);
				return 1;	//Return user declined to overwrite existing files
			}
		}

		if(!eof_soft_cursor)
		{	//If not using the software mouse
			if(show_os_cursor(2) < 0)
			{	//If the OS mouse cannot be displayed
				eof_soft_cursor = 1;	//Revert to the software mouse
			}
		}

		if(eof_ogg_settings())
		{
			put_backslash(directory);												//Ensure that the directory string ends in a folder separator
			(void) snprintf(cfn, sizeof(cfn) - 1, "%soriginal.mp3", directory);		//Get the destination path of the original.mp3 to be created
			#ifdef ALLEGRO_WINDOWS
				(void) eof_copy_file(file, "eoftemp.mp3");
				(void) uszprintf(syscommand, (int) sizeof(syscommand), "mp3toogg \"eoftemp.mp3\" %s \"%s%s\" \"%s\"", eof_ogg_quality[(int)eof_ogg_setting], directory, dest_name, file);
			#else
				(void) uszprintf(syscommand, (int) sizeof(syscommand), "lame --decode \"%s\" - | oggenc --quiet -q %s --resample 44100 -s 0 - -o \"%s%s\"", file, eof_ogg_quality[(int)eof_ogg_setting], directory, dest_name);
			#endif
			(void) eof_system(syscommand);
			#ifdef ALLEGRO_WINDOWS
				(void) delete_file("eoftemp.mp3");
			#endif
			(void) eof_copy_file(file, cfn);	//Copy the selected MP3 file to a file named original.mp3 in the chart folder
		}
		else
		{
			return 2;	//Return user canceled conversion
		}
	}//Convert from MP3
	else if(!ustricmp(get_extension(file), "wav"))
	{	//Convert from WAV
		eof_log("\tConverting source WAV file to OGG format", 1);

		if(eof_ogg_settings())
		{
			put_backslash(directory);												//Ensure that the directory string ends in a folder separator
			#ifdef ALLEGRO_WINDOWS
				(void) uszprintf(syscommand, (int) sizeof(syscommand), "wavtoogg \"%s\" %s \"%s%s\"", file, eof_ogg_quality[(int)eof_ogg_setting], directory, dest_name);
			#else
				(void) uszprintf(syscommand, (int) sizeof(syscommand), "oggenc --quiet -q %s --resample 44100 -s 0 \"%s\" -o \"%s%s\"", eof_ogg_quality[(int)eof_ogg_setting], file, directory, dest_name);
			#endif
			(void) eof_system(syscommand);
		}
		else
		{
			return 2;	//Return user canceled conversion
		}
	}//Convert from WAV
	else
	{	//Copy as-is (assume OGG file)
		eof_log("\tUsing source file in its original format (OGG assumed)", 1);

		if(!eof_menu_file_new_supplement(directory, NULL, 1))
		{	//If the user declined to overwrite an existing "guitar.ogg" file at the destination path
			eof_cursor_visible = 1;
			eof_pen_visible = 1;
			eof_changes = 0;
			eof_show_mouse(NULL);
			return 1;	//Return user declined to overwrite existing files
		}
		(void) ustrcpy(syscommand, directory);
		put_backslash(syscommand);
		(void) ustrcat(syscommand, dest_name);
		if(ustricmp(file, syscommand))
		{	//If the source and destination file are not the same, copy the file
			(void) eof_copy_file(file, syscommand);
		}
	}

	snprintf(cfn, sizeof(cfn) - 1, "%s%s", directory, dest_name);	//Build the target file name
	if(exists(cfn))
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tFile \"%s\" created", cfn);
		eof_log(eof_log_string, 1);
		return 0;	//Return success
	}
	else
	{	//The file does not exist, test write permission
		PACKFILE *fp;

		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tFile \"%s\" was not created", cfn);
		eof_log(eof_log_string, 1);

		fp = pack_fopen(cfn, "wb");	//Attempt to create file
		if(!fp)
		{
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Could not manually create file:  \"%s\"", strerror(errno));	//Get the Operating System's reason for the failure
			eof_log(eof_log_string, 1);
		}
		else
		{
			pack_fclose(fp);
			eof_log("Test file was able to be created manually, but the conversion utility could not do so", 1);
			(void) delete_file(cfn);
		}
		return 4;	//Error
	}
}

int eof_new_chart(char * filename)
{
	char year[32] = {0};
	char album[256] = {0};
	char genre[256] = {0};
	char tracknumber[32] = {0};
	char oggfilename[1024] = {0};
	char dest_name[15] = {0};
	char * returnedfolder = NULL;
	int ret = 0;
	ALOGG_OGG * temp_ogg = NULL;
	char * temp_buffer = NULL;
	int temp_buffer_size = 0;
	struct ID3Tag tag={NULL,0,0,0,0,0,0.0,NULL,0,NULL,NULL,NULL,NULL};
	unsigned long ctr = 0;

	eof_log("eof_new_chart() entered", 1);

	if(filename == NULL)
		return 1;	//Return failure

	if(!eof_soft_cursor)
	{	//If not using the software mouse
		if(show_os_cursor(2) < 0)
		{	//If the OS mouse cannot be displayed
			eof_soft_cursor = 1;	//Revert to the software mouse
		}
	}

	(void) ustrcpy(oggfilename, filename);
	(void) ustrcpy(eof_last_ogg_path, filename);

	eof_color_dialog(eof_file_new_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_file_new_dialog);
	(void) ustrcpy(eof_etext, "");		//Used to store the Artist tag
	(void) ustrcpy(eof_etext2, "");		//Used to store the Title tag
	(void) ustrcpy(eof_etext3, "");
	(void) ustrcpy(eof_etext4, "");		//Used to store the filename created with %Artist% - %Title%
	eof_render();
	if(!ustricmp("ogg", get_extension(oggfilename)))
	{
		eof_log("\tOGG file selected.", 2);
		temp_buffer = eof_buffer_file(oggfilename, 0, 0);
		temp_buffer_size = (int) file_size_ex(oggfilename);
		if(temp_buffer)
		{
			temp_ogg = alogg_create_ogg_from_buffer(temp_buffer, temp_buffer_size);
			if(temp_ogg)
			{
				(void) alogg_get_ogg_comment(temp_ogg, "ARTIST", eof_etext);
				(void) alogg_get_ogg_comment(temp_ogg, "TITLE", eof_etext2);
				(void) alogg_get_ogg_comment(temp_ogg, "ALBUM", album);
				(void) alogg_get_ogg_comment(temp_ogg, "DATE", eof_etext3);
				(void) alogg_get_ogg_comment(temp_ogg, "GENRE", genre);
				(void) alogg_get_ogg_comment(temp_ogg, "TRACKNUMBER", tracknumber);
				strncpy(year, eof_etext3, sizeof(year) - 1);	//Truncate the string to fit
			}
		}
	}
	else if(!ustricmp("mp3", get_extension(oggfilename)))
	{
		#ifdef ALLEGRO_WINDOWS
			//Windows has its own function to open files that have non ASCII characters in the file path
			wchar_t widepath[1024] = {0};

			(void) uconvert(oggfilename, U_UTF8, (char *)(&widepath[0]), U_UNICODE, 2048);
			tag.fp=_wfopen(widepath, L"rb");
		#else
			tag.fp=fopen(oggfilename,"rb");	//Open user-specified file for reading
		#endif

		if(tag.fp != NULL)
		{	//If the file was able to be opened
			eof_allocate_ucode_table();		//Prepare for extended ASCII to Unicode conversion
			year[0]='\0';	//Empty the year string

			if(ID3FrameProcessor(&tag))		//If ID3v2 frames are found
			{
				(void) GrabID3TextFrame(&tag,"TPE1",eof_etext,(unsigned long)(sizeof(eof_etext)/sizeof(char)));		//Store the Artist info in eof_etext[]
				eof_convert_from_extended_ascii(eof_etext, 1024);
				(void) GrabID3TextFrame(&tag,"TIT2",eof_etext2,(unsigned long)(sizeof(eof_etext2)/sizeof(char)));	//Store the Title info in eof_etext2[]
				eof_convert_from_extended_ascii(eof_etext2, 1024);
				(void) GrabID3TextFrame(&tag,"TYER",year,(unsigned long)(sizeof(year)/sizeof(char)));				//Store the Year info in year[]
				eof_sanitize_string(year);			//Filter out unprintable and extended ASCII
				eof_convert_from_extended_ascii(eof_etext2, 1024);
				(void) GrabID3TextFrame(&tag,"TALB",album,(unsigned long)(sizeof(album)/sizeof(char)));				//Store the Album info in album[]
				eof_convert_from_extended_ascii(album, 256);
				(void) GrabID3TextFrame(&tag,"TCON",genre,(unsigned long)(sizeof(genre)/sizeof(char)));				//Store the Genre info in genre[]
				eof_sanitize_string(album);			//Filter out unprintable and extended ASCII
				if((genre[0] != '\0') && (genre[1] == '\0'))
				{	//If the genre string is a single byte long, it is a genre number instead of a string
					genre[0] = '\0';	//Ignore it
				}
				else
				{	//Otherwise
					eof_sanitize_string(album);			//Filter out unprintable and extended ASCII
				}
				(void) GrabID3TextFrame(&tag,"TRCK",tracknumber,(unsigned long)(sizeof(tracknumber)/sizeof(char)));	//Store the track number info in tracknumber[]
				eof_sanitize_string(album);			//Filter out unprintable and extended ASCII
			}

			//If any of the information was not found in the ID3v2 tag, check for it from an ID3v1 tag
			//ID3v1 fields are 30 characters long maximum (31 bytes as a string), while the year field is 4 characters long (5 bytes as a string)
			if(tag.id3v1present > 1)	//If there were fields defined in an ID3v1 tag
			{
				char buffer[1024] = {0};
				if((eof_etext[0]=='\0') && (tag.id3v1artist != NULL))
				{
					(void) strncpy(buffer, tag.id3v1artist, 1023);
					eof_convert_from_extended_ascii(buffer, 1024);
					(void) ustrcpy(eof_etext, buffer);
				}
				if((eof_etext2[0]=='\0') && (tag.id3v1title != NULL))
				{
					(void) strncpy(buffer, tag.id3v1title, 1023);
					eof_convert_from_extended_ascii(buffer, 1024);
					(void) ustrcpy(eof_etext2, buffer);
				}
				if((year[0]=='\0') && (tag.id3v1year != NULL))
				{
					(void) ustrcpy(year, tag.id3v1year);
				}
				if((album[0]=='\0') && (tag.id3v1album != NULL))
				{
					(void) strncpy(buffer, tag.id3v1album, 1023);
					eof_convert_from_extended_ascii(buffer, 1024);
					buffer[255] = '\0';	//Ensure this will fit in the album string
					(void) ustrcpy(album, buffer);
				}
			}

			DestroyID3(&tag);	//Release the list of ID3 frames

			(void) fclose(tag.fp);	//Close file
			tag.fp=NULL;
			eof_free_ucode_table();
		}
	}

	/* user fills in song information */
	if(eof_popup_dialog(eof_file_new_dialog, 3) != 6)
	{
		eof_cursor_visible = 1;
		eof_pen_visible = 1;
		eof_show_mouse(NULL);
		alogg_destroy_ogg(temp_ogg);
		free(temp_buffer);
		return 1;	//Return failure
	}

	eof_log("\tSong information confirmed", 1);

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
	eof_conditionally_center_dialog(eof_file_new_windows_dialog);
	eof_file_new_windows_dialog[1].flags = 0;
	eof_file_new_windows_dialog[2].flags = 0;
	eof_file_new_windows_dialog[3].flags = D_SELECTED;
	if(eof_popup_dialog(eof_file_new_windows_dialog, 4) == 5)
	{	//If the user clicked OK
		if(eof_file_new_windows_dialog[1].flags & D_SELECTED)
		{	//Use Existing Folder
			eof_log("\tUse existing folder selected", 1);
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
			eof_log("\tUse source audio's folder selected", 1);
			eof_render();
			(void) replace_filename(eof_etext3, filename, "", 1024);
		}
		else
		{	//Create New Folder
#ifdef ALLEGRO_WINDOWS
			unsigned long ctr2;

			for(ctr2 = ustrlen(eof_etext4); ctr2 > 0; ctr2--)
			{	//For each character in the user defined song folder, in reverse order
				if(isspace(ugetat(eof_etext4, ctr2 - 1)))
				{	//If the end of the string is a space character
					(void) usetat(eof_etext4, ctr2 - 1, '\0');	//Truncate it from the string
				}
				else
				{
					break;	//Otherwise break from the loop
				}
			}
#endif
			eof_log("\tCreate new folder selected", 1);
			(void) ustrcpy(eof_etext3, eof_songs_path);
			(void) ustrcat(eof_etext3, eof_etext4);
		}
	}
	else
	{
		eof_cursor_visible = 1;
		eof_pen_visible = 1;
		eof_show_mouse(NULL);
		alogg_destroy_ogg(temp_ogg);
		free(temp_buffer);
		return 1;	//Return failure
	}

	/* if music file is MP3/WAV, convert it */
	ret = eof_audio_to_ogg(oggfilename, eof_etext3, dest_name, 1);
	if(ret != 0)	//If a suitably named OGG was not created successfully
		return ret;	//Return failure

	/* destroy old song */
	if(eof_song)
	{
		eof_destroy_song(eof_song);
		eof_song = NULL;
		eof_song_loaded = 0;
	}
	(void) eof_destroy_ogg();

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

	//Validate year string
	if(strlen(year) != 4)		//If the year string isn't exactly 4 digits
		year[0]='\0';			//Nullify it
	else
		for(ctr=0;ctr<4;ctr++)		//Otherwise check all digits to ensure they're numerical
			if(!isdigit(year[ctr]))	//If it contains a non numerical character
				year[0]='\0';		//Empty the year array

	/* fill in information */
	(void) ustrcpy(eof_song->tags->artist, eof_etext);	//Prevent buffer overflow
	(void) ustrcpy(eof_song->tags->title, eof_etext2);
	(void) ustrcpy(eof_song->tags->frettist, eof_last_frettist);
	(void) ustrcpy(eof_song->tags->year, year);	//The year tag that was read from an MP3 (if applicable)
	(void) ustrcpy(eof_song->tags->album, album);
	(void) ustrcpy(eof_song->tags->genre, genre);
	(void) ustrcpy(eof_song->tags->tracknumber, tracknumber);
	(void) ustrcpy(oggfilename, filename);
	(void) replace_filename(eof_last_ogg_path, oggfilename, "", 1024);

	/* load OGG */
	(void) ustrcpy(eof_song_path, eof_etext3);
	put_backslash(eof_song_path);
	(void) ustrcpy(eof_last_eof_path, eof_song_path);
	if(temp_ogg)
	{	//If the user-specified audio file was an OGG file and was successfully loaded earlier in this function
		eof_music_track = temp_ogg;
		eof_music_data = temp_buffer;
		eof_music_data_size = temp_buffer_size;
		if(exists(oggfilename))
		{	//As long as oggfilename still reflects a valid path to the OGG file
			(void) ustrncpy(eof_loaded_ogg_name, oggfilename, sizeof(eof_loaded_ogg_name) - 1);
			(void) ustrcpy(eof_song->tags->ogg[0].filename, get_filename(eof_loaded_ogg_name));	//Apply this file name to the OGG profile
		}
		else
		{	//Due to some error, oggfilename isn't valid and should be reset to reflect the default filename of guitar.ogg (which is already applied to the OGG profile's file name)
			(void) ustrcpy(eof_loaded_ogg_name, eof_etext3);
			put_backslash(eof_loaded_ogg_name);
			(void) ustrcat(eof_loaded_ogg_name, "guitar.ogg");
		}
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
	eof_music_length = alogg_get_length_msecs_ogg_ul(eof_music_track);
	(void) ustrcpy(eof_loaded_song_name, "notes.eof");
	(void) append_filename(eof_filename, eof_song_path, eof_loaded_song_name, (int) sizeof(eof_filename));	//Build the full path to the project file

	eof_song_loaded = 1;

	/* get ready to edit */
	eof_init_after_load(0);	//Initialize variables
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(NULL);

	/* enable open strum for legacy tracks if the relevant preference is enabled */
	if(eof_enable_open_strums_by_default)
	{	//If the user enabled this preference
		for(ctr = 1; ctr < eof_song->tracks; ctr++)
		{	//For each track in the project
			if(eof_track_is_legacy_guitar(eof_song, ctr))
			{	//If it is a legacy guitar track
				if(!eof_open_strum_enabled(ctr))
				{	//If open strum isn't already enabled for it (the expected case)
					unsigned long tracknum = eof_song->track[ctr]->tracknum;

					eof_song->track[ctr]->flags |= EOF_TRACK_FLAG_SIX_LANES;	//Set the 6 lane flag
					eof_song->legacy_track[tracknum]->numlanes = 6;				//Set the lane count
				}
			}
		}
		eof_scale_fretboard(0);	//Recalculate the 2D screen positioning based on the current track
	}

	/* selectively disable the difficulty limit for pro guitar tracks */
	if(!eof_write_rb_files && !eof_write_fof_files && (eof_write_rs_files || eof_write_rs2_files))
	{	//If neither Rock Band nor FoF export are enabled, but either RS1 or RS2 export is enabled
		eof_song->track[EOF_TRACK_PRO_BASS]->flags |= EOF_TRACK_FLAG_UNLIMITED_DIFFS;	//Enable unlimited difficulties
		eof_song->track[EOF_TRACK_PRO_GUITAR]->flags |= EOF_TRACK_FLAG_UNLIMITED_DIFFS;
		eof_song->track[EOF_TRACK_PRO_BASS_22]->flags |= EOF_TRACK_FLAG_UNLIMITED_DIFFS;
		eof_song->track[EOF_TRACK_PRO_GUITAR_22]->flags |= EOF_TRACK_FLAG_UNLIMITED_DIFFS;
		eof_song->track[EOF_TRACK_PRO_GUITAR_B]->flags |= EOF_TRACK_FLAG_UNLIMITED_DIFFS;
		eof_note_type = 0;	//And start in the first difficulty tab
	}

	/* add any default INI entries to the project's INI entries list */
	for(ctr = 0; ctr < eof_default_ini_settings; ctr++)
	{	//For each of the default INI settings
		if(ctr < EOF_MAX_INI_SETTINGS)
		{	//Bounds check
			(void) ustrncpy(eof_song->tags->ini_setting[eof_song->tags->ini_settings], eof_default_ini_setting[ctr], EOF_INI_LENGTH - 1);
			eof_song->tags->ini_settings++;
		}
	}

	(void) eof_menu_file_quick_save();

	return 0;	//Return success
}

int eof_save_helper_checks(void)
{
	EOF_PRO_GUITAR_TRACK *tp;
	unsigned long tracknum, notes_after_chart_audio, ctr, ctr2, ctr3;
	int suggested = 0, ret;
	char oggfn[1024] = {0};
	char newfolderpath[1024] = {0};
	char note_length_warned = 0, note_distance_warned = 0, arpeggio_warned = 0, slide_warned = 0, bend_warned = 0, slide_error = 0, note_skew_warned = 0;
	int pg_notes_present;

	/* check if there are any notes beyond the chart audio */
	notes_after_chart_audio = eof_check_if_notes_exist_beyond_audio_end(eof_song);
	if(notes_after_chart_audio && !eof_silence_loaded)
	{	//Only display this warning if there is chart audio loaded
		(void) snprintf(oggfn, sizeof(oggfn) - 1, "Warning:  Track \"%s\" contains notes/lyrics extending beyond the chart's audio.", eof_song->track[notes_after_chart_audio]->name);
		eof_clear_input();
		if(alert(oggfn, NULL, "This chart may not work properly.  Continue?", "&Yes", "&No", 'y', 'n') != 1)
		{	//If the user doesn't opt to continue due to this error condition
			return 1;	//Return cancellation
		}
	}


	/* check lyrics for lyrics outside of phrases or invalid characters */
	if(eof_song->vocal_track[0]->lyrics > 0)
	{
		/* sort and validate lyrics */
		eof_track_sort_notes(eof_song, EOF_TRACK_VOCALS);
		eof_track_fixup_notes(eof_song, EOF_TRACK_VOCALS, 0);

		/* pre-parse the lyrics to determine if any of them are not contained within a lyric phrase */
		if(eof_song->tags->lyrics)
		{	//If user enabled the Lyrics checkbox in song properties
			ret = 0;
			for(ctr = 0; ctr < eof_song->vocal_track[0]->lyrics; ctr++)
			{	//For each lyric
				if((eof_song->vocal_track[0]->lyric[ctr]->note == EOF_LYRIC_PERCUSSION) || (eof_find_lyric_line(ctr) != NULL))
					continue;	//If this lyric is vocal percussion or is within a line, skip it

				if(!ret)
				{	//If the lyric is not in a lyric line and the user wasn't prompted about how to handle this
					eof_cursor_visible = 0;
					eof_pen_visible = 0;
					eof_show_mouse(screen);
					eof_clear_input();
					eof_seek_and_render_position(EOF_TRACK_VOCALS, eof_get_note_type(eof_song, EOF_TRACK_VOCALS, ctr), eof_get_note_pos(eof_song, EOF_TRACK_VOCALS, ctr));

					ret = alert3("Warning: One or more lyrics aren't within lyric phrases.", "These lyrics won't export to FoF script or simple text formats.", "Continue?", "&Yes", "&No", "&Highlight", 'y', 'n', 'h');
					eof_show_mouse(NULL);
					eof_cursor_visible = 1;
					eof_pen_visible = 1;
					if(ret == 1)
					{	//User wants to continue normally
						break;
					}
					if(ret == 2)
					{	//User canceled
						return 1;	//Return cancellation
					}
				}
				if(ret == 3)
				{	//User wanted to hightlight lyrics outside of lyric lines
					eof_set_note_flags(eof_song, EOF_TRACK_VOCALS, ctr, eof_get_note_flags(eof_song, EOF_TRACK_VOCALS, ctr) | EOF_NOTE_FLAG_HIGHLIGHT);	//Add highlighting
				}
			}
			if(ret == 3)
			{	//If the user wanted to cancel after highlighting lyrics outside of lyric lines
				return 1;	//Return cancellation
			}
		}//If user enabled the Lyrics checkbox in song properties
		if(!eof_rs2_export_extended_ascii_lyrics || eof_write_fof_files || eof_write_rb_files || eof_write_bf_files || eof_write_gh_files)
		{	//If extended ASCII export isn't enabled, or if a non Rocksmith export is enabled
			for(ctr = 0; ctr < eof_song->vocal_track[0]->lyrics; ctr++)
			{	//For each lyric
				if((eof_song->vocal_track[0]->lyric[ctr]->text[0] != '\0') && (eof_string_has_non_ascii(eof_song->vocal_track[0]->lyric[ctr]->text)))
				{	//If any of the lyrics that contain text have non ASCII characters
					eof_clear_input();
					if(alert("Warning: One or more lyrics have non ASCII characters.", "These lyrics may not work correctly for some rhythm games.", "Cancel and seek to first offending lyric?", "&Yes", "&No", 'y', 'n') == 1)
					{	//If user opts to cancel the save
						eof_seek_and_render_position(EOF_TRACK_VOCALS, eof_get_note_type(eof_song, EOF_TRACK_VOCALS, ctr), eof_get_note_pos(eof_song, EOF_TRACK_VOCALS, ctr));
						return 1;	//Return cancellation
					}
					break;
				}
			}
		}
	}


	/* check 5 lane guitar note lengths */
	for(ctr = 1; !note_length_warned && eof_min_note_length && (ctr < eof_song->tracks); ctr++)
	{	//For each track (only check if the user defined a minimum length, and only if the user didn't already decline to cancel when an offending note was found)
		if(!eof_track_is_legacy_guitar(eof_song, ctr))
			continue;	//If this isn't a legacy guitar track, skip it

		for(ctr2 = 0; ctr2 < eof_get_track_size(eof_song, ctr); ctr2++)
		{	//For each note in the track
			if(eof_get_note_length(eof_song, ctr, ctr2) >= eof_min_note_length)
				continue;	//If this note's length is not shorter than the minimum length, skip it

			eof_clear_input();
			if(alert("Warning:  At least one note was truncated shorter", "than your defined minimum length.", "Cancel save and seek to the first such note?", "&Yes", "&No", 'y', 'n') == 1)
			{	//If the user opted to seek to the first offending note (only prompt once per call)
				eof_seek_and_render_position(ctr, eof_get_note_type(eof_song, ctr, ctr2), eof_get_note_pos(eof_song, ctr, ctr2));
				return 1;	//Return cancellation
			}
			note_length_warned = 1;
			break;	//Stop checking after the first offending note is found
		}
	}


	/* check note distances */
	for(ctr = 1; !note_distance_warned && (ctr < eof_song->tracks); ctr++)
	{	//For each track (and only if the user didn't already decline to cancel when an offending note was found)
		for(ctr2 = 0; ctr2 < eof_get_track_size(eof_song, ctr); ctr2++)
		{	//For each note in the track
			long next;
			unsigned long effective_note_distance, thisnotepos, nextnotepos;

			next = eof_track_fixup_next_note(eof_song, ctr, ctr2);	//Get the next note, if it exists
			if(next <= 0)
				continue;	//If this note doesn't have a note that follows, skip it

			thisnotepos = eof_get_note_pos(eof_song, ctr, ctr2);
			nextnotepos = eof_get_note_pos(eof_song, ctr, next);
			effective_note_distance = eof_get_effective_minimum_note_distance(eof_song, ctr, ctr2);

			if(effective_note_distance == ULONG_MAX)
				continue;	//If the effective minimum note distance for this note couldn't be determined, skip it
			if(nextnotepos - thisnotepos >= effective_note_distance)
				continue;	//If this note and the next are placed far away enough to allow enforcing the minimum note distance, skip it
			if((thisnotepos == nextnotepos) && (eof_get_note_eflags(eof_song, ctr, ctr2) & EOF_NOTE_EFLAG_DISJOINTED))
				continue;	//If this note and the next note are at the same position and have disjointed status, skip it

			eof_clear_input();
			if(alert("Warning:  At least one note is too close to another", "to enforce the minimum note distance.", "Cancel save and seek to the first such note?", "&Yes", "&No", 'y', 'n') == 1)
			{	//If the user opted to seek to the first offending note (only prompt once per call)
				eof_seek_and_render_position(ctr, eof_get_note_type(eof_song, ctr, ctr2), thisnotepos);
				return 1;	//Return cancellation
			}
			note_distance_warned = 1;
			break;	//Stop checking after the first offending note is found
		}
	}


	/* check for any incomplete measures, where a time signature changed is placed mid-measure */
	eof_process_beat_statistics(eof_song, eof_selected_track);
	for(ctr = 1; ctr < eof_song->beats; ctr++)
	{	//For each beat after the first
		if(!eof_song->beat[ctr]->contains_ts_change)
			continue;	//If this beat does not have a time signature change, skip it

		if(!eof_song->beat[ctr - 1]->has_ts || (eof_song->beat[ctr - 1]->beat_within_measure == eof_song->beat[ctr - 1]->num_beats_in_measure - 1))
			continue;	//If the previous beat does not have a time signature, or if it was the last beat in its measure (the beat_within_measure stat is numbered starting with 0), skip it

		suggested = eof_song->beat[ctr - 1]->beat_within_measure + 1;	//Track the last beat number in the measure and account for the zero numbering
		for(ctr = ctr - 1; ctr > 0; ctr--)
		{	//For each of the previous beats
			if(eof_song->beat[ctr]->beat_within_measure == 0)
			{	//If this is the first beat in the affected measure
				break;
			}
		}
		(void) snprintf(newfolderpath, sizeof(newfolderpath) - 1, "(Suggested T/S is %d/%u).  Cancel save?", suggested, eof_song->beat[ctr]->beat_unit);	//Determine a suitable time signature to suggest
		eof_selected_beat = ctr;	//Select the affected beat marker
		eof_beat_stats_cached = 0;
		eof_seek_and_render_position(eof_selected_track, eof_note_type, eof_song->beat[ctr]->pos);	//seek to the beat in question and render
		eof_clear_input();
		if(alert("At least one measure is interrupted by a time signature.", "This can cause problems in some rhythm games.", newfolderpath, "&Yes", "&No", 'y', 'n') == 1)
		{	//If the user opts to correct the issue
			return 1;	//Return cancellation
		}
		break;
	}


	/* perform checks for chord fingerings and fret hand positions */
	if(eof_write_rs_files || eof_write_rs2_files || eof_write_bf_files)
	{	//If the user wants to save Rocksmith or Bandfuse capable files
		(void) eof_correct_chord_fingerings();			//Ensure all chords in each pro guitar track have valid finger arrays, prompt user to provide any that are missing
		if(eof_check_fret_hand_positions())
		{	//If any fret hand position errors were found
			if(alert("One or more problems with defined fret hand positions were found.", NULL, "Cancel save and review them now?", "&Yes", "&No", 'y', 'n') == 1)
			{	//If the user opts to see the problems
				(void) eof_check_fret_hand_positions_menu();
				return 1;	//Return cancellation
			}
		}
	}


	pg_notes_present = eof_song_has_pro_guitar_content(eof_song);	//Determine whether there are no pro guitar/bass notes and therefore no need to provide certain RS/BF warnings


	/* check if any Rocksmith sections don't have a Rocksmith phrase at the same position */
	if(pg_notes_present && (eof_write_rs_files || eof_write_rs2_files || eof_write_bf_files))
	{	//If there are any pro guitar/bass notes and the user wants to save Rocksmith or Bandfuse capable files
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


	/* check if any tracks use 2 or more tone names but doesn't define the default or uses more than 4 tone names */
	if(eof_write_rs2_files)
	{	//If the user wants to save Rocksmith 2 files
		char warning1 = 0, warning2 = 0, warning3 = 0;

		for(ctr = 1; ctr < eof_song->tracks; ctr++)
		{	//For each track
			if(eof_song->track[ctr]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
				continue;	//If this is not a pro guitar/bass track, skip it

			tracknum = eof_song->track[ctr]->tracknum;
			tp = eof_song->pro_guitar_track[tracknum];
			//Build and count the size of the list of unique tone names used, and empty the default tone string if it is not valid
			eof_track_rebuild_rs_tone_names_list_strings(ctr, 1);
			if(eof_track_rs_tone_names_list_strings_num == 1)
			{	//If only one tone name is used in tone changes (changing from an unspecified tone to another one)
				eof_clear_input();
				if(!warning3 && alert("Warning:  At least one track uses only one tone name.  You must use at least", "two different tone names and set one as default for them to work in Rocksmith 2014.", "Cancel save and update tone definitions?", "&Yes", "&No", 'y', 'n') == 1)
				{
					eof_track_destroy_rs_tone_names_list_strings();
					(void) eof_menu_track_selected_track_number(ctr, 1);	//Set the active instrument track
					eof_render();
					(void) eof_track_rs_tone_names();	//Call up the tone names dialog
					return 1;	//Return cancellation
				}
				warning3 = 1;
			}
			else if(eof_track_rs_tone_names_list_strings_num > 1)
			{	//If at least 2 unique tone names are used
				if((tp->defaulttone[0] == '\0') && !warning1)
				{	//If the default tone is not set, and the user wasn't warned about this yet
					eof_clear_input();
					if(alert("Warning:  At least one track with tone changes has no default tone set.", NULL, "Cancel save and update tone definitions?", "&Yes", "&No", 'y', 'n') == 1)
					{
						eof_track_destroy_rs_tone_names_list_strings();
						(void) eof_menu_track_selected_track_number(ctr, 1);	//Set the active instrument track
						eof_render();
						(void) eof_track_rs_tone_names();	//Call up the tone names dialog
						return 1;	//Return cancellation
					}
					warning1 = 1;
				}
				if((eof_track_rs_tone_names_list_strings_num > 4) && !warning2)
				{	//If there are more than 4 unique tone names used, and the user wasn't warned about this yet
					if(alert("Warning:  At least one arrangement uses more than 4 different tones.", "Rocksmith doesn't support more than 4 so EOF will only export changes for 4 tone names.", "Cancel save and update tone definitions?", "&Yes", "&No", 'y', 'n') == 1)
					{
						eof_track_destroy_rs_tone_names_list_strings();
						(void) eof_menu_track_selected_track_number(ctr, 1);	//Set the active instrument track
						eof_render();
						(void) eof_track_rs_tone_names();	//Call up the tone names dialog
						return 1;	//Return cancellation
					}
					warning2 = 1;
				}
			}
			eof_track_destroy_rs_tone_names_list_strings();
		}
	}


	/* check if any arpeggio phrases only have one note in them (handshapes do not trigger this warning) */
	for(ctr = 1; (ctr < eof_song->tracks) && !arpeggio_warned; ctr++)
	{	//For each track, or until the user is warned about an offending arpeggio
		unsigned long notectr;
		char restore_tech_view = 0;

		if(eof_song->track[ctr]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
			continue;	//If this is not a pro guitar/bass track, skip it

		tracknum = eof_song->track[ctr]->tracknum;
		tp = eof_song->pro_guitar_track[tracknum];
		restore_tech_view = eof_menu_pro_guitar_track_get_tech_view_state(tp);	//Track which note set is in use
		eof_menu_pro_guitar_track_set_tech_view_state(tp, 0);	//Activate the normal note set

		for(ctr2 = 0; ctr2 < tp->arpeggios; ctr2++)
		{	//For each arpeggio/handshape phrase in the track
			if(tp->arpeggio[ctr2].flags & EOF_RS_ARP_HANDSHAPE)
				continue;	//If this is a handshape phrase instead of a normal arpeggio phrase, skip it

			notectr = 0;
			for(ctr3 = 0; ctr3 < tp->notes; ctr3++)
			{	//For each note in the track
				if((tp->note[ctr3]->pos >= tp->arpeggio[ctr2].start_pos) && (tp->note[ctr3]->pos <= tp->arpeggio[ctr2].end_pos) && (tp->note[ctr3]->type == tp->arpeggio[ctr2].difficulty))
				{	//If the note is within the arpeggio phrase
					notectr++;	//Increment counter
				}
			}
			if(notectr < 2)
			{
				eof_clear_input();
				eof_seek_and_render_position(ctr, tp->arpeggio[ctr2].difficulty, tp->arpeggio[ctr2].start_pos);
				if(alert("Warning:  At least one arpeggio phrase doesn't contain at least two notes.", "You should remove the arpeggio phrase or add additional notes into it.", "Cancel save?", "&Yes", "&No", 'y', 'n') == 1)
				{	//If the user opts to cancel
					return 1;	//Return cancellation
				}
				arpeggio_warned = 1;	//Set a condition to exit outer for loop
				break;	//Break from inner for loop
			}
		}//For each arpeggio phrase in the track
		eof_menu_pro_guitar_track_set_tech_view_state(tp, restore_tech_view);	//Activate whichever note set was active for the track
	}//For each track, or until the user is warned about an offending arpeggio


	/* check if any slide notes don't validly define their end position or bend notes don't define their bend strength */
	if(eof_write_rs_files || eof_write_rs2_files || eof_write_bf_files)
	{	//If the user wants to save Rocksmith or Bandfuse capable files
		for(ctr = 1; ctr < eof_song->tracks; ctr++)
		{	//For each track
			unsigned long flags, noteset;
			char restore_tech_view = 0;

			if(eof_song->track[ctr]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
				continue;	//If this is not a pro guitar/bass track, skip it

			tracknum = eof_song->track[ctr]->tracknum;
			tp = eof_song->pro_guitar_track[tracknum];
			restore_tech_view = eof_menu_pro_guitar_track_get_tech_view_state(tp);	//Track which note set is in use
			for(noteset = 0; noteset < 2; noteset++)
			{	//For each note set
				eof_menu_pro_guitar_track_set_tech_view_state(tp, noteset);	//Activate the appropriate note set
				for(ctr2 = 0; ctr2 < tp->notes; ctr2++)
				{	//For each note in the track
					flags = tp->note[ctr2]->flags;
					if(!(flags & EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION))
					{	//If the note contains no bend strength or slide end position
						if(((flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP) || (flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN)) && !slide_warned)
						{	//If the note has slide technique and is missing the end position, and the user hasn't been warned about this yet
							eof_clear_input();
							eof_seek_and_render_position(ctr, tp->note[ctr2]->type, tp->note[ctr2]->pos);
							if(alert("Warning:  At least one slide note doesn't define its ending position.", "Unless you define this information they will export as 1 fret slides.", "Cancel save?", "&Yes", "&No", 'y', 'n') == 1)
							{	//If the user opts to cancel
								return 1;	//Return cancellation
							}
							slide_warned = 1;
						}
						if(flags & EOF_PRO_GUITAR_NOTE_FLAG_BEND && !bend_warned)
						{	//If the note has bend technique and is missing the bend strength, and the user hasn't been warned about this yet
							eof_clear_input();
							eof_seek_and_render_position(ctr, tp->note[ctr2]->type, tp->note[ctr2]->pos);
							if(alert("Warning:  At least one bend note doesn't define its bend strength.", "Unless you define this information they will export as bending 1 half step.", "Cancel save?", "&Yes", "&No", 'y', 'n') == 1)
							{	//If the user opts to cancel
								return 1;	//Return cancellation
							}
							bend_warned = 1;
						}
					}

					if(!slide_error && !noteset)
					{	//If the user hasn't been warned about any slide related errors yet, and this is the normal note set
						unsigned char lowestfret = eof_pro_guitar_note_lowest_fret(tp, ctr2);	//Determine the note's lowest used fret value

						if(flags & EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION)
						{	//If the note has pitched slide end position data
							if(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP)
							{	//If the note slides up
								if(tp->note[ctr2]->slideend <= lowestfret)
								{	//The slide doesn't go higher than the current note
									slide_error = 1;
								}
							}
							if(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN)
							{	//If the note slides down
								if(tp->note[ctr2]->slideend >= lowestfret)
								{	//The slide doesn't go lower than the current note
									slide_error = 1;
								}
							}
						}
						if(flags & EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE)
						{	//If the note has an unpitched slide
							if(tp->note[ctr2]->unpitchend == lowestfret)
							{	//The slide doesn't move from the current note
								slide_error = 1;
							}
						}
						if(slide_error)
						{	//If one of the above checks failed
							eof_seek_and_render_position(ctr, tp->note[ctr2]->type, tp->note[ctr2]->pos);
							if(alert("Warning:  At least one slide note has an error in its end position.", "Unless corrected, it will not export to XML as a slide.", "Cancel save?", "&Yes", "&No", 'y', 'n') == 1)
							{	//If the user opts to cancel
								return 1;	//Return cancellation
							}
						}
					}
					if(slide_warned && bend_warned && slide_error)
						break;	//Exit for loop if all warnings/errors were issued and declined by the user
				}//For each note in the track
				if(slide_warned && bend_warned && slide_error)
					break;	//Exit for loop if all warnings/errors were issued and declined by the user
			}//For each note set
			eof_menu_pro_guitar_track_set_tech_view_state(tp, restore_tech_view);	//Restore the note set that was in use for the track
			if(slide_warned && bend_warned && slide_error)
				break;	//Exit for loop if all warnings/errors were issued and declined by the user
		}//For each track
	}//If the user wants to save Rocksmith capable files


	/* check if any chords have manually defined names with certain characters such as parentheses, which will cause Rocksmith to malfunction */
	if(eof_write_rs_files || eof_write_rs2_files)
	{	//If the user wants to save Rocksmith capable files
		char target = 1;	//Unless eof_write_rs2_files is enabled, only notes that are valid for RS1 export are checked
		char *name, user_prompted = 0;
		unsigned char original_eof_2d_render_top_option = eof_2d_render_top_option;	//Back up the user's preference

		if(eof_write_rs2_files)
			target = 2;
		for(ctr = 1; !user_prompted && (ctr < eof_song->tracks); ctr++)
		{	//For each track (until the user is warned about any offending chord names)
			char restore_tech_view = 0;

			if(eof_song->track[ctr]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
				continue;	//If this is not a pro guitar/bass track, skip it

			tracknum = eof_song->track[ctr]->tracknum;
			tp = eof_song->pro_guitar_track[tracknum];
			restore_tech_view = eof_menu_pro_guitar_track_get_tech_view_state(tp);	//Track which note set is in use
			eof_menu_pro_guitar_track_set_tech_view_state(tp, 0);	//Activate the normal note set
			for(ctr2 = 0; ctr2 < tp->notes; ctr2++)
			{	//For each note in the track
				if(eof_note_count_rs_lanes(eof_song, ctr, ctr2, target) <= 1)
					continue;	//If the note will not export as a chord, skip it
				name = eof_get_note_name(eof_song, ctr, ctr2);	//Get pointer to the chord's name
				if(!name)
					continue;	//If this note's name was not retrievable, skip it
				if(!rs_filter_string(name, 1))
					continue;	//If this note's name doesn't include any invalid characters (forward slash is allowed for denoting slash chords), skip it

				eof_2d_render_top_option = 5;					//Change the user preference to render note names at the top of the piano roll
				eof_seek_and_render_position(ctr, tp->note[ctr2]->type, tp->note[ctr2]->pos);	//Render the track so the user can see where the correction needs to be made
				eof_clear_input();
				if(!user_prompted && alert("At least one chord has an unaccepted character: } , \\ : { \"", "This can cause Rocksmith to crash or hang and will be removed.", "Cancel save?", "&Yes", "&No", 'y', 'n') == 1)
				{	//If the user hasn't already answered this prompt, and opts to correct the issue
					eof_2d_render_top_option = original_eof_2d_render_top_option;	//Restore the user's preference
					return 1;	//Return cancellation
				}
				eof_2d_render_top_option = original_eof_2d_render_top_option;	//Restore the user's preference
				user_prompted = 1;	//Set the condition to exit outer for loop
				break;	//Break from inner for loop
			}//For each note in the track
			eof_menu_pro_guitar_track_set_tech_view_state(tp, restore_tech_view);	//Restore the note set that was in use for the track
		}//For each track (until the user is warned about any offending chord names)
	}//If the user wants to save Rocksmith capable files


	/* check for the use of "maj" in chord names, which will be overridden by Rocksmith to reflect that it is a minor chord instead of a major chord */
	if(eof_write_rs_files || eof_write_rs2_files)
	{	//If the user wants to save Rocksmith capable files
		char restore_tech_view = 0;
		int user_prompted = 0;

		for(ctr = 1; !user_prompted && (ctr < eof_song->tracks); ctr++)
		{	//For each track (until the user is warned about any offending handshape phrases)
			if(eof_song->track[ctr]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
				continue;	//If this is not a pro guitar/bass track, skip it

			tracknum = eof_song->track[ctr]->tracknum;
			tp = eof_song->pro_guitar_track[tracknum];
			restore_tech_view = eof_menu_pro_guitar_track_get_tech_view_state(tp);	//Track which note set is in use
			eof_menu_pro_guitar_track_set_tech_view_state(tp, 0);	//Disable tech view

			for(ctr2 = 0; ctr2 < tp->notes; ctr2++)
			{	//For each note in the track
				user_prompted = eof_rs_check_chord_name(eof_song, eof_selected_track, eof_selection.current, 1);
				if(user_prompted)
				{	//If there is lowercase "maj" in the note's name
					if(user_prompted == 2)
					{	//If the user opted to cancel the save
						return 1;	//Return cancellation
					}
					break;	//Otherwise just stop checking notes in this track
				}
			}

			eof_menu_pro_guitar_track_set_tech_view_state(tp, restore_tech_view);	//Re-enable tech view if it was in effect before save
		}
	}


	/* check for arpeggio/handshape phrases that cross from one RS phrase into another, which may malfunction on charts with dynamic difficulty */
	if(!eof_song->tags->rs_export_suppress_dd_warnings)
	{	//If dynamic difficulty warnings haven't been suppressed for this chart
		if(eof_write_rs_files || eof_write_rs2_files)
		{	//If the user wants to save Rocksmith capable files
			char user_prompted = 0;
			unsigned char original_eof_2d_render_top_option = eof_2d_render_top_option;	//Back up the user's preference

			for(ctr = 1; !user_prompted && (ctr < eof_song->tracks); ctr++)
			{	//For each track (until the user is warned about any offending handshape phrases)
				if(eof_song->track[ctr]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
					continue;	//If this is not a pro guitar/bass track, skip it

				eof_process_beat_statistics(eof_song, ctr);	//Rebuild beat stats from this track's perspective
				tracknum = eof_song->track[ctr]->tracknum;
				tp = eof_song->pro_guitar_track[tracknum];
				eof_pro_guitar_track_sort_arpeggios(tp);
				for(ctr2 = 0; ctr2 < tp->arpeggios; ctr2++)
				{	//For each arpeggio/handshape
					unsigned long start, end;

					start = eof_get_beat(eof_song, tp->arpeggio[ctr2].start_pos);
					end = eof_get_beat(eof_song, tp->arpeggio[ctr2].end_pos);
					if(!eof_beat_num_valid(eof_song, start) || !eof_beat_num_valid(eof_song, end) || (end < start))
						continue;	//If the effective beat numbers for the start and end position of the arpeggio/handshape were not properly identified, skip it

					for(ctr3 = start + 1; !user_prompted && (ctr3 <= end); ctr3++)
					{	//For each beat between them, after the first (which will always be at/before the beginning of the arpeggio, when the condition being checked can only happen to a beat AFTER the start of the arpeggio)
						if(eof_song->beat[ctr3]->contained_section_event < 0)
							continue;	//If this beat does not have an RS phrase defined, skip it

						//Otherwise it marks a phrase change
						eof_2d_render_top_option = 9;					//Change the user preference to render RS phrases and sections at the top of the piano roll
						if(tp->arpeggio[ctr2].difficulty != 0xFF)
						{	//If this is a difficulty specific arpeggio
							eof_note_type = tp->arpeggio[ctr2].difficulty;	//Change to the relevant difficulty
						}
						eof_seek_and_render_position(ctr, eof_note_type, tp->arpeggio[ctr2].start_pos);	//Render the track so the user can see where the correction needs to be made
						eof_clear_input();
						if(!user_prompted && alert("At least one arpeggio/handshape crosses over into another RS phrase", "This can behave strangely in Rocksmith if the chart has dynamic difficulty.", "Cancel save?", "&Yes", "&No", 'y', 'n') == 1)
						{	//If the user hasn't already answered this prompt, and opts to correct the issue
							eof_2d_render_top_option = original_eof_2d_render_top_option;	//Restore the user's preference
							return 1;	//Return cancellation
						}
						eof_2d_render_top_option = original_eof_2d_render_top_option;	//Restore the user's preference
						user_prompted = 1;	//Set the condition to exit outer for loops
						break;	//Break from inner for loop
					}
				}
			}
		}
	}//If dynamic difficulty warnings haven't been suppressed for this chart


	/* check for any bend strengths higher than 3 half steps */
	if(eof_write_rs_files || eof_write_rs2_files)
	{	//If the user wants to save Rocksmith capable files
		char user_prompted = 0;
		char restore_tech_view = 0;

		for(ctr = 1; !user_prompted && (ctr < eof_song->tracks); ctr++)
		{	//For each track (until the user is warned about any offending handshape phrases)
			if(eof_song->track[ctr]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
				continue;	//If this is not a pro guitar/bass track, skip it

			tracknum = eof_song->track[ctr]->tracknum;
			tp = eof_song->pro_guitar_track[tracknum];
			restore_tech_view = eof_menu_pro_guitar_track_get_tech_view_state(tp);	//Track which note set is in use
			for(ctr2 = 0; !user_prompted && (ctr2 < 2); ctr2++)
			{	//For each note set
				eof_menu_pro_guitar_track_set_tech_view_state(tp, ctr2);	//Activate the appropriate note set
				for(ctr3 = 0; ctr3 < tp->notes; ctr3++)
				{	//For each note in the note set
					unsigned long flags = tp->note[ctr3]->flags;
					unsigned char strength;

					if(!(flags & EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION) || !(flags & EOF_PRO_GUITAR_NOTE_FLAG_BEND))
						continue;	//If the note does not contain a bend strength, skip it

					strength = tp->note[ctr3]->bendstrength;
					if(!(strength & 0x80))
					{	//If the MSB of this value isn't set, it is defined in half steps
						strength = (strength & 0x7F) * 2;	//Convert to quarter steps
					}
					else
					{
						strength = strength & 0x7F;	//Mask out the MSB
					}
					if(strength > 6)
					{	//If the bend strength is greater than 3 half steps (6 quarter steps)
						eof_seek_and_render_position(ctr, tp->note[ctr3]->type, tp->note[ctr3]->pos);	//Render the track so the user can see where the correction needs to be made
						eof_clear_input();
						if(!user_prompted && alert("At least one bend note has a strength higher than 3 half steps.", "3 half steps is the strongest bend that Rocksmith supports.", "Cancel save?", "&Yes", "&No", 'y', 'n') == 1)
						{	//If the user hasn't already answered this prompt, and opts to correct the issue
							return 1;	//Return cancellation
						}
						user_prompted = 1;	//Set the condition to exit outer for loops
						break;	//Break from inner for loop
					}
				}
			}
			eof_menu_pro_guitar_track_set_tech_view_state(tp, restore_tech_view);	//Restore the note set that was in use for the track
		}
	}

	/* check for notes that are out of alignment between difficulties */
	for(ctr = 1; (note_skew_warned != 1) && (ctr < eof_song->tracks); ctr++)
	{	//For each track (or until the user declines the warning about an offending note)
		for(ctr2 = 0; ctr2 < eof_get_track_size(eof_song, ctr); ctr2++)
		{	//For each note in the track
			char match = 0;	//Set to nonzero if a note in a higher difficulty is found to be at this note's exact position
			char unmatch = 0;	//Set to nonzero if a note in a higher difficulty is found to be within this note's exact position, but not at the exact position itself
			unsigned long notepos = eof_get_note_pos(eof_song, ctr, ctr2), notepos2 = 0;
			int note1snapped, note2snapped;
			unsigned long offender = 0;
			char *warning1 = "Warning: At least one note is out of sync with a note in another difficulty.";
			char *warning2 = "Warning: At least one note is out of sync with a grid snapped note.";
			char *warning = warning1;

			note1snapped = eof_is_any_beat_interval_position(notepos, NULL, NULL, NULL, NULL, 0);	//Check if the outer loop's note is beat interval snapped
			note2snapped = note1snapped;
			for(ctr3 = 0; ctr3 < eof_get_track_size(eof_song, ctr); ctr3++)
			{	//For each note in the track
				notepos2 = eof_get_note_pos(eof_song, ctr, ctr3);

				if(notepos2 + 3 < notepos)
					continue;	//If this note is more than 3ms earlier, skip it
				if(notepos2 > notepos + 3)
					break;		//If this note is more than 3ms later, stop comparing notes to the one in the outer loop
				if(ctr2 == ctr3)
					continue;	//Don't compare the note with itself
				if(eof_get_note_type(eof_song, ctr, ctr2) >= eof_get_note_type(eof_song, ctr, ctr3))
					continue;	//Don't compare the note with others in the same or lower difficulties

				if(notepos == notepos2)
					match = 1;	//If a note was found at the same exact position, track this
				else
					unmatch = 1;	//Otherwise it was a note within 3ms, track this

				note2snapped = eof_is_any_beat_interval_position(notepos2, NULL, NULL, NULL, NULL, 0);	//Check if the inner loop's note is beat interval snapped
				if(note1snapped != note2snapped)
					break;	//If one note is snapped and the other isn't, the snapped note is considered correct
			}

			if(note1snapped != note2snapped)
			{	//If one note is snapped and the other isn't, consider the non snapped note the offender
				if(!note1snapped)
					offender = ctr2;	//The outer loop's note is not snapped
				else
					offender = ctr3;	//The inner loop's note is not snapped

				warning = warning2;
			}
			else
			{	//Otherwise consider the outer loop's note to be the offender
				offender = ctr2;
				warning = warning1;
			}
			if(match || !unmatch)
				continue;	//If any notes in other difficulties were at a matching position, or if there weren't any 1-3ms away, skip this note

			//Otherwise the notes likely need to be synchronized
			if(!note_skew_warned)
			{	//If the user was not prompted yet
				ret = alert3(warning, NULL, "Cancel and seek to first offending note?", "Yes", "No", "Highlight all and cancel", 0, 0, 0);
				if(ret == 2)
				{	//User declined
					note_skew_warned = 1;	//Set a condition to exit the outer for loop
					break;
				}

				eof_seek_and_render_position(ctr, eof_get_note_type(eof_song, ctr, offender), eof_get_note_pos(eof_song, ctr, offender));	//Seek to the first offending note
				if(ret == 1)
				{	//If the user opted to cancel the save
					return 1;	//Return cancellation
				}

				note_skew_warned = 3;
			}
			if(note_skew_warned == 3)
			{	//User opted to highlight all offending notes
				unsigned long flags = eof_get_note_flags(eof_song, ctr, offender);
				eof_set_note_flags(eof_song, ctr, offender, flags | EOF_NOTE_FLAG_HIGHLIGHT);
			}
		}
	}
	if(note_skew_warned == 3)
		return 1;	//If the user opted to cancel after highlighting offending notes, return cancellation


	/* check for RS phrases and RS sections that have non alphanumeric characters in their name */
	if(pg_notes_present && (eof_write_rs_files || eof_write_rs2_files))
	{	//If there are any pro guitar/bass notes and the user wants to save Rocksmith capable files
		char warn = 0;
		unsigned char original_eof_2d_render_top_option = eof_2d_render_top_option;	//Back up the user's preference

		for(ctr = 0; ctr < eof_song->beats; ctr++)
		{	//For each beat in the chart
			if(eof_song->beat[ctr]->contained_rs_section_event >= 0)
			{	//If this beat has a Rocksmith section
				if(eof_string_has_non_alphanumeric(eof_song->text_event[eof_song->beat[ctr]->contained_rs_section_event]->text))
				{	//If this section name has non alphanumeric characters
					warn = 1;
				}
			}
			if(eof_song->beat[ctr]->contained_section_event >= 0)
			{	//If this beat has a section event (RS phrase)
				if(eof_string_has_non_alphanumeric(eof_song->text_event[eof_song->beat[ctr]->contained_section_event]->text))
				{	//If this phrase name has non alphanumeric characters
					warn = 1;
				}
			}

			if(warn)
			{
				eof_2d_render_top_option = 9;					//Change the user preference to render RS phrases and sections at the top of the piano roll
				eof_seek_and_render_position(ctr, eof_note_type, eof_song->beat[ctr]->pos);	//Render the track so the user can see where the correction needs to be made
				eof_clear_input();

				if(alert("Warning (RS):  At least one RS section/phrase has a non alphanumeric character in its name (letters and numbers only).", "Some characters may cause the game to malfunction.", "Cancel save?", "&Yes", "&No", 'y', 'n') == 1)
				{	//If the user opted to cancel the save
					return 1;	//Return cancellation
				}

				eof_2d_render_top_option = original_eof_2d_render_top_option;	//Restore the user's preference
				break;
			}
		}
	}

	/* check for notes with more than 32 bend points, which will cause Rocksmith 2014 to crash */
	if(eof_write_rs2_files)
	{	//If the user wants to save Rocksmith 2014 capable files
		char user_prompted = 0;

		for(ctr = 1; !user_prompted && (ctr < eof_song->tracks); ctr++)
		{	//For each track (until the user is warned about any offending bend notes)
			char restore_tech_view = 0;

			if(eof_song->track[ctr]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
				continue;	//If this is not a pro guitar/bass track, skip it

			tracknum = eof_song->track[ctr]->tracknum;
			tp = eof_song->pro_guitar_track[tracknum];
			restore_tech_view = eof_menu_pro_guitar_track_get_tech_view_state(tp);	//Track which note set is in use
			eof_menu_pro_guitar_track_set_tech_view_state(tp, 0);	//Activate the normal note set
			for(ctr2 = 0; ctr2 < tp->notes; ctr2++)
			{	//For each note in the track
				if(eof_pro_guitar_note_bitmask_has_bend_tech_note(tp, ctr2, 63, NULL) < 33)
					continue;	//If this note has fewer than 33 bend tech notes, skip it

				eof_menu_pro_guitar_track_set_tech_view_state(tp, 1);	//Activate the tech note set
				eof_seek_and_render_position(ctr, tp->pgnote[ctr2]->type, tp->pgnote[ctr2]->pos);	//Render the track so the user can see where the correction needs to be made
				eof_clear_input();
				if(!user_prompted && alert("At least one note has 33 or more bend tech notes", "This would cause Rocksmith 2014 to crash so only the first 32 bend points will export.", "Cancel save?", "&Yes", "&No", 'y', 'n') == 1)
				{	//If the user hasn't already answered this prompt, and opts to correct the issue
					return 1;	//Return cancellation
				}
				user_prompted = 1;	//Set the condition to exit outer for loop
				break;	//Break from inner for loop
			}//For each note in the track
			eof_menu_pro_guitar_track_set_tech_view_state(tp, restore_tech_view);	//Restore the note set that was in use for the track
		}//For each track (until the user is warned about any offending bend notes)
	}

	/* check for notes occurring before the first defined section, if there are any sections */
	if(eof_check_for_notes_preceding_sections(1))
	{	//If there are notes before the first section and the user opts to cancel save
		return 1;	//Return cancellation
	}


	///Dynamic difficulty related checks below
	if(eof_song->tags->rs_export_suppress_dd_warnings)
		return 0;	//If dynamic difficulty warnings have been suppressed for this chart, skip the remainder of this function's logic which performs that check

	/* check for any notes that extend into a different RS phrase or section */
	if(eof_write_rs_files || eof_write_rs2_files)
	{	//If the user wants to save Rocksmith capable files
		char user_prompted = 0;
		unsigned char original_eof_2d_render_top_option = eof_2d_render_top_option;	//Back up the user's preference

		for(ctr = 1; !user_prompted && (ctr < eof_song->tracks); ctr++)
		{	//For each track (until the user is warned about any offending notes)
			EOF_RS_TECHNIQUES tech = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
			unsigned long bitmask;
			char restore_tech_view = 0;
			unsigned long start, stop;
			int sectionchange = 0, phrasechange = 0;

			if(eof_song->track[ctr]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
				continue;	//If this is not a pro guitar/bass track, skip it

			eof_process_beat_statistics(eof_song, ctr);	//Rebuild beat statistics from the perspective of this track
			tracknum = eof_song->track[ctr]->tracknum;
			tp = eof_song->pro_guitar_track[tracknum];
			restore_tech_view = eof_menu_pro_guitar_track_get_tech_view_state(tp);	//Track which note set is in use
			eof_menu_pro_guitar_track_set_tech_view_state(tp, 0);	//Activate the normal note set

			for(ctr2 = 0; ctr2 < tp->notes; ctr2++)
			{	//For each note in the track
				unsigned long startbeat, stopbeat;

				start = tp->note[ctr2]->pos;			//Record its start and stop position
				stop = start + tp->note[ctr2]->length;

				for(ctr3 = 0, bitmask = 1; ctr3 < 6; ctr3++, bitmask <<= 1)
				{	//For each of the 6 usable strings
					if(tp->note[ctr2]->note & bitmask)
					{	//If the note uses this string
						(void) eof_get_rs_techniques(eof_song, ctr, ctr2, ctr3, &tech, 2, 1);	//Check to see if the gem on this string has linknext status applied
						if(tech.linknext)
						{	//If it does
							long nextnote = eof_fixup_next_pro_guitar_note(tp, ctr2);	//Look for another note that follows in this track difficulty

							if(nextnote > 0)
							{	//If a next note is identified
								stop = tp->note[nextnote]->pos + tp->note[nextnote]->length;	//Its ending is the effective end position to consider
							}
						}
					}
				}

				startbeat = eof_get_beat(eof_song, start);	//Find the beat in which this note starts
				stopbeat = eof_get_beat(eof_song, stop);	//And the beat in which it ends
				if(eof_beat_num_valid(eof_song, stopbeat) && (stop == eof_song->beat[stopbeat]->pos) && (stopbeat > startbeat))
				{	//If the note extends up to and ends on the beat
					stopbeat--;	//Interpret it as ending at the previous beat instead of surpassing it
				}
				if(!eof_beat_num_valid(eof_song, startbeat) || !eof_beat_num_valid(eof_song, stopbeat) || (startbeat == stopbeat))
					continue;	//If either of those beats weren't successfully identified or they are the same beat, skip the logic below

				for(ctr3 = startbeat + 1; (ctr3 <= stopbeat) && (ctr3 < eof_song->beats); ctr3++)
				{	//For each beat after the start beat up to and including the stop beat
					if(eof_song->beat[ctr3]->contained_rs_section_event >= 0)
						sectionchange = 1;	//This beat indicates a section change
					if(eof_song->beat[ctr3]->contained_section_event >= 0)
						phrasechange = 1;	//This beat indicates a phrase change
				}
				if(sectionchange || phrasechange)
				{	//If the section or phrase changes during the course of the note
					eof_2d_render_top_option = 9;					//Change the user preference to render RS phrases and sections at the top of the piano roll
					eof_seek_and_render_position(ctr, tp->note[ctr2]->type, tp->note[ctr2]->pos);	//Render the track so the user can see where the correction needs to be made
					eof_clear_input();
					if(!user_prompted && alert("Warning (RS):  At least one note crosses an RS phrase or section boundary.", "This can behave strangely in Rocksmith if the chart has dynamic difficulty.", "Cancel save?", "&Yes", "&No", 'y', 'n') == 1)
					{	//If the user hasn't already answered this prompt, and opts to correct the issue
						return 1;	//Return cancellation
					}
					eof_2d_render_top_option = original_eof_2d_render_top_option;	//Restore the user's preference
					user_prompted = 1;	//Set the condition to exit outer for loop
					break;	//Break from inner for loop
				}
			}
			eof_menu_pro_guitar_track_set_tech_view_state(tp, restore_tech_view);	//Activate whichever note set was active for the track
		}//For each track (until the user is warned about any offending notes)
	}//If the user wants to save Rocksmith capable files

	//Drums Rock related checks
	if(eof_track_is_drums_rock_mode(eof_song, EOF_TRACK_DRUM) || eof_track_is_drums_rock_mode(eof_song, EOF_TRACK_DRUM_PS))
	{	//If a Drums Rock track is to be exported
		(void) replace_filename(oggfn, eof_song_path, "preview.ogg", 1024);
		if(!exists(oggfn))
		{	//If the project has no preview audio defined
			eof_clear_input();
			allegro_message("Warning:  There is no preview audio defined.  Please use \"File>Export preview audio\" and save the project again to save it to each Drums Rock chart difficulty.");
		}
		if(eof_check_drums_rock_track(eof_song, EOF_TRACK_DRUM) || eof_check_drums_rock_track(eof_song, EOF_TRACK_DRUM_PS))
		{	//If user opted to cancel the save due to any prompted issues
			return 1;	//Return cancellation
		}
	}

	return 0;
}

int eof_save_helper(char *destfilename, char silent)
{
	unsigned long ctr;
	char newfolderpath[1024] = {0};
	char tempfilename2[1024] = {0};
	char oggfn[1024] = {0};
	char function;		//Will be set to 1 for "Save" or 2 for "Save as"
	int jumpcode = 0;
	char fixvoxpitches = 0, fixvoxphrases = 0;
	time_t seconds;		//Will store the current time in seconds
	struct tm *caltime;	//Will store the current time in calendar format
	unsigned short user_warned = 0;	//Tracks whether the user was warned about hand positions being undefined and auto-generated during Rocksmith and Bandfuse exports
	int err;
	int midi_export_success = 1;

	eof_log("eof_save_helper() entered", 1);

	if(!eof_song_loaded || !eof_song)
		return 2;	//Return failure:  Invalid parameters

	/* sort notes so they are in order of position */
	eof_sort_notes(eof_song);
	eof_fixup_notes(eof_song);

	if(!silent)
	{	//If warning messages aren't suppressed
		unsigned long seektrack = eof_selected_track;	//Used to store the active track difficulty and seek position to be restored after prompts to cancel save are declined
		unsigned long seekdiff = eof_note_type;
		unsigned long seekpos = eof_music_pos.value - eof_av_delay;

		eof_skip_mid_beats_in_measure_numbering = 0;	//Disable this measure numbering alteration so that any relevant warnings can be given by eof_save_helper_checks() below
		eof_beat_stats_cached = 0;

		if(eof_save_helper_checks())	//If the user cancels the save via one of the prompts
			return 1;	//Return cancellation

///		eof_skip_mid_beats_in_measure_numbering = 1;	//Enable mid beat tempo changes to be ignored in the measure numbering now that any applicable warnings were given
		eof_beat_stats_cached = 0;

		eof_seek_and_render_position(seektrack, seekdiff, seekpos);	//Restore the active track and seek position from before the checks
	}

	/* build the target file name */
	if(destfilename == NULL)
	{	//Perform save
		function = 1;
		if((eof_song_path[0] == '\0') || (eof_loaded_song_name[0] == '\0'))
		{
			if(eof_song_path[0] == '\0')
				eof_log("\t\teof_song_path string is empty", 1);
			if(eof_loaded_song_name[0] == '\0')
				eof_log("\t\teof_loaded_song_name string is empty", 1);
			return 3;	//Return failure:  Invalid paths
		}
		(void) append_filename(eof_temp_filename, eof_song_path, eof_loaded_song_name, (int) sizeof(eof_temp_filename));
		(void) replace_filename(newfolderpath, eof_song_path, "", 1024);	//Obtain the destination path
	}
	else
	{	//Perform save as
		function = 2;
		(void) replace_extension(destfilename, destfilename, "eof", 1024);	//Ensure the chart is saved with a .eof extension
		(void) ustrcpy(eof_temp_filename, destfilename);
		if(eof_temp_filename[1022] != '\0')	//If the source filename was too long to store in the array
			return 4;			//Return failure:  Destination path too long
		(void) replace_filename(newfolderpath, destfilename, "", 1024);	//Obtain the destination path
	}

	/* rotate out the last save file (filename).previous_save.eof.bak */
	if(!eof_disable_backups)
	{	//Only do this if the user did not disable automatic backups
		(void) replace_extension(eof_temp_filename, eof_temp_filename, "eof", (int) sizeof(eof_temp_filename));	//Ensure the chart's file path has a .eof extension
		(void) replace_extension(tempfilename2, eof_temp_filename, "previous_save.eof.bak", 1024);	//(filename).previous_save.eof.bak will be store the last save operation
		if(exists(tempfilename2))
		{	//If the lastsave file exists
			(void) delete_file(tempfilename2);	//Delete it
			if(exists(tempfilename2))
			{	//Make sure it's gone
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Warning:  Could not delete the previous_save backup file.  Aborting save.  Please use \"Save as\" to save to a new location.");
				eof_log(eof_log_string, 1);
				allegro_message("%s", eof_log_string);
				return 5;	//Return failure:  Error deleting lastsave file
			}
		}
		if(exists(eof_temp_filename))
		{	//If the target of the save operation already exists
			(void) eof_copy_file(eof_temp_filename, tempfilename2);	//Back it up as (filename).previous_save.eof.bak
			if(!exists(tempfilename2))
			{	//Make sure it was created
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Warning:  Could not create the previous_save backup file.  Aborting save.  Please use \"Save as\" to save to a new location.");
				eof_log(eof_log_string, 1);
				allegro_message("%s", eof_log_string);
				return 6;	//Return failure:  Could not create previous_save backup file
			}
			(void) delete_file(eof_temp_filename);	//Delete the target file name
			if(exists(eof_temp_filename))
			{	//Make sure it was deleted
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Warning:  Could not delete the last save file.  Aborting save.  Please use \"Save as\" to save to a new location.");
				eof_log(eof_log_string, 1);
				allegro_message("%s", eof_log_string);
				return 7;	//Return failure:  Could not delete the last save file
			}
		}
	}

	/* save the chart */
	eof_song->tags->revision++;
 	eof_log("\tSaving project", 1);
	if(!eof_save_song(eof_song, eof_temp_filename))
	{
		allegro_message("Could not save song!");
		eof_show_mouse(NULL);
		eof_cursor_visible = 1;
		eof_pen_visible = 1;
		return 8;	//Return failure:  Could not save song
	}
	seconds = time(NULL);
	caltime = localtime(&seconds);
	(void) strftime(eof_log_string, sizeof(eof_log_string) - 1, "\tProject saved at %c", caltime);
	eof_log(eof_log_string, 1);
	if(!exists(eof_temp_filename))
	{	//Make sure the target file was created
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Warning:  Save operation did not create project file.  Please use \"Save as\" to save to a new location.");
		eof_log(eof_log_string, 1);
		allegro_message("%s", eof_log_string);
		return 9;	//Return failure:  Could not create project file
	}
	(void) ustrcpy(eof_filename, eof_temp_filename);	//Update the absolute project filename
	(void) ustrcpy(eof_loaded_song_name, get_filename(eof_temp_filename));	//Update the relative project filename

	/* save the MIDI, INI and other files*/
	if(!silent && (eof_write_fof_files || eof_write_rb_files))
	{	//If prompts aren't being suppressed, and FoF or RB MIDIs are to be exported
		eof_check_vocals(eof_song, &fixvoxpitches, &fixvoxphrases);
	}
	else
	{
		fixvoxpitches = fixvoxphrases = 0;	//Answer no to the prompts on behalf of the user
	}
	if(eof_song->tags->lyrics && eof_song->vocal_track[0]->lyrics)
	{	//If user enabled the Lyrics checkbox in song properties and there are lyrics defined
		//Save plain script format lyrics
		(void) append_filename(eof_temp_filename, newfolderpath, "lyrics.txt", (int) sizeof(eof_temp_filename));
		jumpcode=setjmp(jumpbuffer); //Store environment/stack/etc. info in the jmp_buf array
		if(jumpcode!=0) //if program control returned to the setjmp() call above returning any nonzero value
		{	//Lyric export failed
			(void) puts("Assert() handled successfully!");
			allegro_message("Plain script lyric export failed.\nMake sure there are no Unicode or extended ASCII characters in EOF's folder path,\nbecause EOF's lyric export doesn't support them.");
		}
		else
		{
			(void) EOF_EXPORT_TO_LC(eof_song,eof_temp_filename,NULL,PLAIN_FORMAT);	//Import lyrics into FLC lyrics structure and export to plain script format
		}
	}

	if(eof_write_fof_files)
	{	//If the user opted to save FoF related files
		/* Save MIDI file */
		(void) append_filename(eof_temp_filename, newfolderpath, "notes.mid", (int) sizeof(eof_temp_filename));
		midi_export_success &= eof_export_midi(eof_song, eof_temp_filename, 0, fixvoxpitches, fixvoxphrases, 0);

		/* Save INI file */
		if(!eof_disable_ini_export)
		{	//But only if the user did not disable this
			(void) append_filename(eof_temp_filename, newfolderpath, "song.ini", (int) sizeof(eof_temp_filename));
			(void) eof_save_ini(eof_song, eof_temp_filename, silent);
		}

		/* save script lyrics if applicable) */
		if(eof_song->tags->lyrics && eof_song->vocal_track[0]->lyrics)							//If user enabled the Lyrics checkbox in song properties and there are lyrics defined
		{
			(void) append_filename(eof_temp_filename, newfolderpath, "script.txt", (int) sizeof(eof_temp_filename));
			jumpcode=setjmp(jumpbuffer); //Store environment/stack/etc. info in the jmp_buf array
			if(jumpcode!=0) //if program control returned to the setjmp() call above returning any nonzero value
			{	//Lyric export failed
				(void) puts("Assert() handled successfully!");
				allegro_message("Script lyric export failed.\nMake sure there are no Unicode or extended ASCII characters in EOF's folder path,\nbecause EOF's lyric export doesn't support them.");
			}
			else
			{
				(void) EOF_EXPORT_TO_LC(eof_song,eof_temp_filename,NULL,SCRIPT_FORMAT);	//Import lyrics into FLC lyrics structure and export to script format
			}
		}
	}

	if(eof_write_lrc_files && eof_song->vocal_track[0]->lyrics && eof_song->vocal_track[0]->lines)
	{	//If the user opted to save LRC lyric files and there are lyrics and lyric lines defined
		//Export normal LRC
		(void) append_filename(eof_temp_filename, newfolderpath, "lyrics.lrc", (int) sizeof(eof_temp_filename));
		jumpcode=setjmp(jumpbuffer); //Store environment/stack/etc. info in the jmp_buf array
		if(jumpcode!=0) //if program control returned to the setjmp() call above returning any nonzero value
		{	//Lyric export failed
			(void) puts("Assert() handled successfully!");
			allegro_message("LRC lyric export failed.\nMake sure there are no Unicode or extended ASCII characters in EOF's folder path,\nbecause EOF's lyric export doesn't support them.");
		}
		else
		{
			(void) EOF_EXPORT_TO_LC(eof_song,eof_temp_filename,NULL,LRC_FORMAT);	//Import lyrics into FLC lyrics structure and export to LRC format
		}

		//Export extended LRC
		(void) append_filename(eof_temp_filename, newfolderpath, "lyrics.elrc", (int) sizeof(eof_temp_filename));
		jumpcode=setjmp(jumpbuffer); //Store environment/stack/etc. info in the jmp_buf array
		if(jumpcode!=0) //if program control returned to the setjmp() call above returning any nonzero value
		{	//Lyric export failed
			(void) puts("Assert() handled successfully!");
			allegro_message("ELRC lyric export failed.\nMake sure there are no Unicode or extended ASCII characters in EOF's folder path,\nbecause EOF's lyric export doesn't support them.");
		}
		else
		{
			(void) EOF_EXPORT_TO_LC(eof_song,eof_temp_filename,NULL,ELRC_FORMAT);	//Import lyrics into FLC lyrics structure and export to ELRC format
		}

		//Export QRC
		(void) append_filename(eof_temp_filename, newfolderpath, "lyrics.qrc", (int) sizeof(eof_temp_filename));
		jumpcode=setjmp(jumpbuffer); //Store environment/stack/etc. info in the jmp_buf array
		if(jumpcode!=0) //if program control returned to the setjmp() call above returning any nonzero value
		{	//Lyric export failed
			(void) puts("Assert() handled successfully!");
			allegro_message("QRC lyric export failed.\nMake sure there are no Unicode or extended ASCII characters in EOF's folder path,\nbecause EOF's lyric export doesn't support them.");
		}
		else
		{
			(void) EOF_EXPORT_TO_LC(eof_song,eof_temp_filename,NULL,QRC_FORMAT);	//Import lyrics into FLC lyrics structure and export to QRC format
		}
	}

	if(eof_write_gh_files)
	{	//If the user opted to save Guitar Hero related files
		/* Save GHWT drum codes, for use in Queen Bee */
		(void) append_filename(eof_temp_filename, newfolderpath, "ghwt_expert_drums.array.txt", (int) sizeof(eof_temp_filename));
		eof_write_ghwt_drum_array_txt(eof_song, eof_temp_filename);

		/* Save GHWT MIDI variant */
		(void) append_filename(eof_temp_filename, newfolderpath, "notes_ghwt.mid", (int) sizeof(eof_temp_filename));
		midi_export_success &= eof_export_midi(eof_song, eof_temp_filename, 0, fixvoxpitches, fixvoxphrases, 1);

		/* Save GH3 MIDI variant */
		(void) append_filename(eof_temp_filename, newfolderpath, "notes_gh3.mid", (int) sizeof(eof_temp_filename));
		midi_export_success &= eof_export_midi(eof_song, eof_temp_filename, 0, fixvoxpitches, fixvoxphrases, 2);

		/* Save GHL charts */
		(void) append_filename(eof_temp_filename, newfolderpath, "guitar_3x2.xmk", (int) sizeof(eof_temp_filename));
		(void) eof_export_ghl(eof_song, EOF_TRACK_GUITAR, eof_temp_filename);
		(void) append_filename(eof_temp_filename, newfolderpath, "vocals.xmk", (int) sizeof(eof_temp_filename));
		(void) eof_export_ghl(eof_song, EOF_TRACK_VOCALS, eof_temp_filename);
	}

	if(eof_write_rb_files)
	{	//If the user opted to also save RBN2 and RB3 pro guitar upgrade compliant MIDIs
		(void) append_filename(eof_temp_filename, newfolderpath, "notes_rbn.mid", (int) sizeof(eof_temp_filename));
		midi_export_success &= eof_export_midi(eof_song, eof_temp_filename, 1, fixvoxpitches, fixvoxphrases, 0);	//Write a RBN2 compliant MIDI
		if(eof_get_track_size_normal(eof_song, EOF_TRACK_PRO_BASS) || eof_get_track_size_normal(eof_song, EOF_TRACK_PRO_BASS_22) || eof_get_track_size_normal(eof_song, EOF_TRACK_PRO_GUITAR) || eof_get_track_size_normal(eof_song, EOF_TRACK_PRO_GUITAR_22))
		{	//If any of the pro guitar tracks' normal note sets are populated
			//Write the pro guitar upgrade MIDI
			(void) append_filename(eof_temp_filename, newfolderpath, "notes_pro.mid", (int) sizeof(eof_temp_filename));
			midi_export_success &= eof_export_midi(eof_song, eof_temp_filename, 2, fixvoxpitches, fixvoxphrases, 0);	//Write a RB3 pro guitar upgrade compliant MIDI

			//Write the Rock Band MIDI that can be built by the C3 release of Magma
			(void) append_filename(eof_temp_filename, newfolderpath, "notes_c3.mid", (int) sizeof(eof_temp_filename));
			midi_export_success &= eof_export_midi(eof_song, eof_temp_filename, 3, fixvoxpitches, fixvoxphrases, 0);	//Write a MIDI containing the RBN and pro guitar content

			//Write a DTA file for the pro guitar upgrade
			(void) ustrcpy(eof_temp_filename, newfolderpath);
			put_backslash(eof_temp_filename);
			(void) ustrcat(eof_temp_filename, "songs_upgrades");
			if(!eof_folder_exists(eof_temp_filename))
			{	//If the songs_upgrades folder doesn't already exist
				err = eof_mkdir(eof_temp_filename);
				if(err && !eof_folder_exists(eof_temp_filename))
				{	//If it couldn't be created and is still not found to exist (in case the previous check was a false negative)
					allegro_message("Could not create folder!\n%s", eof_temp_filename);
					return 10;	//Return failure:  Could not recreate project folder
				}
			}
			put_backslash(eof_temp_filename);
			(void) replace_filename(eof_temp_filename, eof_temp_filename, "upgrades.dta", (int) sizeof(eof_temp_filename));
			(void) eof_save_upgrades_dta(eof_song, eof_temp_filename);		//Create the upgrades.dta file in the songs_upgrades folder if it does not already exist
		}
	}

	if(!midi_export_success)
	{	//If any of the calls to eof_export_midi() returned 0 (failed)
		allegro_message("Error:  At least one MIDI file failed to export.  Check the logging for details.");
	}

	if(eof_write_music_midi)
	{	//If the user opted to also save musical MIDIs
		(void) append_filename(eof_temp_filename, newfolderpath, "notes_music.mid", (int) sizeof(eof_temp_filename));
		(void) eof_export_music_midi(eof_song, eof_temp_filename, 0);	//Write a Synthesia formatted MIDI
		(void) append_filename(eof_temp_filename, newfolderpath, "notes_fretlight.mid", (int) sizeof(eof_temp_filename));
		(void) eof_export_music_midi(eof_song, eof_temp_filename, 1);	//Write a Fretlight formatted MIDI
	}

	if(eof_write_rs_files || eof_write_rs2_files)
	{	//If the user wants to save Rocksmith capable files
		eof_log("Exporting Rocksmith XML files", 1);
		(void) append_filename(eof_temp_filename, newfolderpath, "xmlpath.xml", (int) sizeof(eof_temp_filename));	//Re-acquire the save's target folder

		if(silent)
		{	//If warnings are being suppressed
			user_warned = 0xFFFF;	//Mark all warnings as already having been given so they won't be displayed again
		}

		if(eof_write_rs_files)
		{	//If the user wants to save Rocksmith 1 files
			(void) eof_export_rocksmith_1_track(eof_song, eof_temp_filename, EOF_TRACK_PRO_BASS, &user_warned);
			(void) eof_export_rocksmith_1_track(eof_song, eof_temp_filename, EOF_TRACK_PRO_BASS_22, &user_warned);
			(void) eof_export_rocksmith_1_track(eof_song, eof_temp_filename, EOF_TRACK_PRO_GUITAR, &user_warned);
			(void) eof_export_rocksmith_1_track(eof_song, eof_temp_filename, EOF_TRACK_PRO_GUITAR_22, &user_warned);
			(void) eof_export_rocksmith_1_track(eof_song, eof_temp_filename, EOF_TRACK_PRO_GUITAR_B, &user_warned);
		}
		if(eof_write_rs2_files)
		{	//If the user wants to save Rocksmith 2 files
			(void) eof_export_rocksmith_2_track(eof_song, eof_temp_filename, EOF_TRACK_PRO_BASS, &user_warned);
			(void) eof_export_rocksmith_2_track(eof_song, eof_temp_filename, EOF_TRACK_PRO_BASS_22, &user_warned);
			(void) eof_export_rocksmith_2_track(eof_song, eof_temp_filename, EOF_TRACK_PRO_GUITAR, &user_warned);
			(void) eof_export_rocksmith_2_track(eof_song, eof_temp_filename, EOF_TRACK_PRO_GUITAR_22, &user_warned);
			(void) eof_export_rocksmith_2_track(eof_song, eof_temp_filename, EOF_TRACK_PRO_GUITAR_B, &user_warned);
		}
		if(eof_song->vocal_track[0]->lyrics)
		{	//If there are lyrics, export them in Rocksmith format as well
			char *arrangement_name;	//This will point to the track's native name unless it has an alternate name defined
			unsigned long numlines;
			EOF_PHRASE_SECTION temp;

			if((eof_song->track[EOF_TRACK_VOCALS]->flags & EOF_TRACK_FLAG_ALT_NAME) && (eof_song->track[EOF_TRACK_VOCALS]->altname[0] != '\0'))
			{	//If the vocal track has an alternate name
				arrangement_name = eof_song->track[EOF_TRACK_VOCALS]->altname;
			}
			else
			{	//Otherwise use the track's native name
				arrangement_name = eof_song->track[EOF_TRACK_VOCALS]->name;
			}

			//Sort lyrics and back up the lyric line data that may be altered by lyric export
			qsort(eof_song->vocal_track[0]->line, (size_t)eof_song->vocal_track[0]->lines, sizeof(EOF_PHRASE_SECTION), eof_song_qsort_phrase_sections);	//Sort the lyric lines
			temp = eof_song->vocal_track[0]->line[0];	//Retain the original lyric line information, which would be lost during a failed RS lyric export
			numlines = eof_song->vocal_track[0]->lines;		//Retain the original line count, which would be lost during a failed RS lyric export

			//Export in RS1 format
			if(eof_write_rs_files)
			{	//If the user wants to save Rocksmith 1 files
				(void) snprintf(tempfilename2, sizeof(tempfilename2), "%s.xml", arrangement_name);	//Build the filename
				(void) append_filename(eof_temp_filename, newfolderpath, tempfilename2, (int) sizeof(eof_temp_filename));	//Build the full file name
				jumpcode=setjmp(jumpbuffer); //Store environment/stack/etc. info in the jmp_buf array
				if(jumpcode!=0) //if program control returned to the setjmp() call above returning any nonzero value
				{	//Lyric export failed
					(void) puts("Assert() handled successfully!");
					allegro_message("Rocksmith lyric export failed.\nMake sure there are no Unicode or extended ASCII characters in EOF's folder path,\nbecause EOF's lyric export doesn't support them.");
				}
				else
				{
					eof_log("Exporting Rocksmith 1 lyrics", 1);
					(void) EOF_EXPORT_TO_LC(eof_song,eof_temp_filename,NULL,RS_FORMAT);	//Import lyrics into FLC lyrics structure and export to Rocksmith format
				}
			}
			//Export in RS2 format
			if(eof_write_rs2_files)
			{	//If the user wants to save Rocksmith 2 files
				int exascii = 0;	//Set to nonzero if non ASCII characters are detected

				for(ctr = 0; ctr < eof_song->vocal_track[0]->lyrics; ctr++)
				{	//For each lyric
					if(eof_string_has_non_ascii(eof_song->vocal_track[0]->lyric[ctr]->text))
					{	//If the lyric has any characters of value higher than 127
						exascii = 1;
						break;
					}
				}

				//Export the lyrics with FoFLC
				if(eof_rs2_export_extended_ascii_lyrics && exascii)
				{	//If extended ASCII output is enabled and such characters are present in the lyrics
					(void) snprintf(tempfilename2, sizeof(tempfilename2), "%s_RS2_EXT.xml", arrangement_name);					//Export a Win-1252 encoded version with the "RS2_EXT" name suffix
					(void) append_filename(eof_temp_filename, newfolderpath, tempfilename2, (int) sizeof(eof_temp_filename));	//Build the full file name
				}
				else
				{
					(void) snprintf(tempfilename2, sizeof(tempfilename2), "%s_RS2.xml", arrangement_name);						//Build the filename
					(void) append_filename(eof_temp_filename, newfolderpath, tempfilename2, (int) sizeof(eof_temp_filename));	//Build the full file name
				}
				jumpcode = setjmp(jumpbuffer); //Store environment/stack/etc. info in the jmp_buf array
				if(jumpcode != 0) //if program control returned to the setjmp() call above returning any nonzero value
				{	//Lyric export failed
					(void) puts("Assert() handled successfully!");
					allegro_message("Rocksmith lyric export failed.\nMake sure there are no Unicode or extended ASCII characters in EOF's folder path,\nbecause EOF's lyric export doesn't support them.");
				}
				else
				{
					eof_log("Exporting Rocksmith 2 lyrics", 1);
					(void) EOF_EXPORT_TO_LC(eof_song, eof_temp_filename, NULL, RS2_FORMAT);	//Import lyrics into FLC lyrics structure and export to Rocksmith 2014 format
				}

				//Export a UTF-8 version of the lyrics if applicable
				if(eof_rs2_export_extended_ascii_lyrics && exascii)
				{	//If extended ASCII output is enabled and such characters were exported in Win-1252 format by FoFLC, export a UTF-8 version of the lyrics
					(void) snprintf(tempfilename2, sizeof(tempfilename2), "%s_RS2.xml", arrangement_name);	//Build the filename
					(void) append_filename(eof_temp_filename, newfolderpath, tempfilename2, (int) sizeof(eof_temp_filename));	//Build the full file name
					eof_export_rocksmith_lyrics(eof_song, eof_temp_filename, 2);
				}
			}
			eof_song->vocal_track[0]->lines = numlines;	//Restore the number of lyric lines present before lyric export was attempted
			eof_song->vocal_track[0]->line[0] = temp;	//Restore the first lyric line present before lyric export was attempted
		}
		(void) eof_detect_difficulties(eof_song, eof_selected_track);		//Update eof_track_diff_populated_status[] to reflect the currently selected difficulty
		eof_process_beat_statistics(eof_song, eof_selected_track);	//Cache section name information into the beat structures (from the perspective of the active track)

		//Determine if the Rocksmith WAV file exists, if not, export the chart audio in WAV format
		if(!eof_silence_loaded && eof_song_has_pro_guitar_content(eof_song))
		{	//If chart audio is loaded and there is at least one normal or tech note in any pro guitar track
			eof_get_rocksmith_wav_path(eof_temp_filename, newfolderpath, sizeof(eof_temp_filename));	//Build the path to the target WAV file
			if(!exists(eof_temp_filename))
			{	//If the WAV file does not exist
				(void) replace_filename(eof_temp_filename, newfolderpath, "guitar.wav", (int) sizeof(eof_temp_filename));
				if(!eof_disable_rs_wav)
				{	//If the user hasn't disabled the creation of the Rocksmith WAV file
					if(!exists(eof_temp_filename))
					{	//If "guitar.wav" also does not exist
						unsigned long silence = 0;	//The amount of silence to append to the RS WAV file (8000ms if RS1 export is enabled)
						SAMPLE *decoded = alogg_create_sample_from_ogg(eof_music_track);	//Create PCM data from the loaded chart audio

						eof_get_rocksmith_wav_path(eof_temp_filename, newfolderpath, sizeof(eof_temp_filename));	//Rebuild the target path based on the song title
						eof_log("Saving Rocksmith WAV file", 1);
						set_window_title("Saving WAV file for use with Wwise.  Please wait.");
						if(eof_write_rs_files)
						{	//If the user wants to save Rocksmith 1 files
							silence = 8000;	//Create a WAV with 8 seconds of silence appended, so that the song will end properly in-game
						}
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Writing RS WAV file (%s) with %lums of silence", eof_temp_filename, silence);
						eof_log(eof_log_string, 1);
						//Write a WAV file with it, appending either 0 or 8 seconds of silence to it
						if(!save_wav_with_silence_appended(eof_temp_filename, decoded, silence))
						{	//If it didn't save, try saving again as "guitar.wav", just in case the user put invalid characters in the song title
							eof_log("Retrying as \"guitar.wav\"", 1);
							(void) replace_filename(eof_temp_filename, newfolderpath, "guitar.wav", (int) sizeof(eof_temp_filename));
							if(!save_wav_with_silence_appended(eof_temp_filename, decoded, 8000))
							{	//If it didn't save again
								allegro_message("Error saving WAV file, check the log for the OS' reason");
							}
						}
						destroy_sample(decoded);	//Release buffered chart audio
						eof_fix_window_title();
					}
				}
			}
		}
	}//If the user wants to save Rocksmith capable files

	if(eof_write_bf_files)
	{	//If the user wants to save Bandfuse capable files
		eof_log("Exporting Bandfuse XML file", 1);

		if(silent)
		{	//If warnings are being suppressed
			user_warned = 0xFFFF;	//Mark all warnings as already having been given so they won't be displayed again
		}

		//Build the target file name
		(void) append_filename(eof_temp_filename, newfolderpath, "xmlpath.xml", (int) sizeof(eof_temp_filename));	//Re-acquire the save's target folder
		if(eof_song->tags->title[0] != '\0')
		{	//If the song title is defined
			if(eof_song->tags->artist[0] != '\0')
			{	//If the artist name is also defined
				(void) snprintf(tempfilename2, sizeof(tempfilename2), "%s- %s_BF.xml", eof_song->tags->artist, eof_song->tags->title);	//Build the relative file name
			}
			else
			{	//Only the song title is defined
				(void) snprintf(tempfilename2, sizeof(tempfilename2), "%s_BF.xml", eof_song->tags->title);
			}
		}
		else
		{
			(void) snprintf(tempfilename2, sizeof(tempfilename2), "UNTITLED_BF.xml");
		}
		(void) append_filename(eof_temp_filename, newfolderpath, tempfilename2, (int) sizeof(eof_temp_filename));	//Build the full file name

		(void) eof_export_bandfuse(eof_song, eof_temp_filename, &user_warned);
	}//If the user wants to save Bandfuse capable files

	/* export Drums Rock files for one drum track if enabled */
	if(eof_track_is_drums_rock_mode(eof_song, EOF_TRACK_DRUM))
	{
		(void) eof_export_drums_rock_track_diff(eof_song, EOF_TRACK_DRUM, 0, newfolderpath);
		(void) eof_export_drums_rock_track_diff(eof_song, EOF_TRACK_DRUM, 1, newfolderpath);
		(void) eof_export_drums_rock_track_diff(eof_song, EOF_TRACK_DRUM, 2, newfolderpath);
		(void) eof_export_drums_rock_track_diff(eof_song, EOF_TRACK_DRUM, 3, newfolderpath);
//		(void) eof_export_drums_rock_track_diff(eof_song, EOF_TRACK_DRUM, 4, newfolderpath);
	}
	else if(eof_track_is_drums_rock_mode(eof_song, EOF_TRACK_DRUM_PS))
	{
		(void) eof_export_drums_rock_track_diff(eof_song, EOF_TRACK_DRUM_PS, 0, newfolderpath);
		(void) eof_export_drums_rock_track_diff(eof_song, EOF_TRACK_DRUM_PS, 1, newfolderpath);
		(void) eof_export_drums_rock_track_diff(eof_song, EOF_TRACK_DRUM_PS, 2, newfolderpath);
		(void) eof_export_drums_rock_track_diff(eof_song, EOF_TRACK_DRUM_PS, 3, newfolderpath);
//		(void) eof_export_drums_rock_track_diff(eof_song, EOF_TRACK_DRUM_PS, 4, newfolderpath);
	}

	/* export IMMERROCK files for all populated difficulties if enabled */
	if(eof_write_immerrock_files)
	{
		eof_export_immerrock(silent);
	}

	/* save OGG file if necessary*/
	if(!eof_silence_loaded)
	{	//Only try to save an audio file if one is loaded
		(void) append_filename(eof_temp_filename, newfolderpath, get_filename(eof_loaded_ogg_name), (int) sizeof(eof_temp_filename));	//Rebuild the file path based on the currently loaded OGG filename
		if(function == 1)
		{	//If performing "Save" function, only write guitar.ogg if it is missing
			if(!exists(eof_temp_filename))
				(void) eof_save_ogg(eof_temp_filename);
		}
		else	//"Save as" requires the guitar.ogg to be overwritten to ensure it's the correct audio
			(void) eof_save_ogg(eof_temp_filename);
	}

	/* finish up */
	eof_changes = 0;
	for(ctr = 0; ctr < eof_song->tags->oggs; ctr++)
	{
		if(eof_song->tags->ogg[ctr].modified)
		{
			(void) replace_filename(eof_temp_filename, eof_temp_filename, eof_song->tags->ogg[ctr].filename, (int) sizeof(eof_temp_filename));
			(void) snprintf(oggfn, sizeof(oggfn) - 1, "%s.lastsaved", eof_temp_filename);
			(void) eof_copy_file(eof_temp_filename, oggfn);
			eof_song->tags->ogg[ctr].modified = 0;
		}
	}
	eof_undo_last_type = 0;
	eof_change_count = 0;
	eof_fix_window_title();
	(void) eof_detect_difficulties(eof_song, eof_selected_track);		//Update eof_track_diff_populated_status[] to reflect the currently selected difficulty
	eof_process_beat_statistics(eof_song, eof_selected_track);		//Re-cache beat information (from the perspective of the active track), since temporary MIDI events may have been added/removed and can cause cached section event numbers to be invalid

	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;

	eof_log("\tSave completed", 1);

	return 0;	//Return success
}

void eof_restore_oggs_helper(void)
{
	char oggfn[1024] = {0};
	char coggfn[1024] = {0};
	int i;

	eof_log("eof_restore_oggs_helper() entered", 1);

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
					eof_log("restoring from .lastsaved\n", 1);
					(void) eof_copy_file(oggfn, coggfn);
					(void) delete_file(oggfn);
				}
				else
				{
					(void) snprintf(oggfn, sizeof(oggfn) - 1, "%s.backup", coggfn);
					if(exists(oggfn))
					{
						eof_log("restoring from .backup\n", 1);
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
		eof_music_play(0);
	}
	ret = alert3(NULL, "You have unsaved changes.", NULL, "Save", "Discard", "Cancel", 0, 0, 0);
	if(ret == 1)
	{
		(void) eof_menu_file_save();
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
	char returnedfn_path[1024] = {0}, *returnedfn = NULL;
	char *initial;

	eof_log("eof_menu_file_gh_import() entered", 1);

	if(eof_menu_prompt_save_changes() == 3)
	{	//If user canceled closing the current, modified chart
		return 1;
	}
	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	if((eof_last_gh_path[uoffset(eof_last_gh_path, ustrlen(eof_last_gh_path) - 1)] == '\\') || (eof_last_gh_path[uoffset(eof_last_gh_path, ustrlen(eof_last_gh_path) - 1)] == '/'))
	{	//If the path ends in a separator
		eof_last_gh_path[uoffset(eof_last_gh_path, ustrlen(eof_last_gh_path) - 1)] = '\0';	//Remove it
	}
	if(eof_imports_recall_last_path && eof_folder_exists(eof_last_gh_path))
	{	//If the user chose for the GH import dialog to start at the path of the last imported GH file and that path is valid
		initial = eof_last_gh_path;	//Use it
	}
	else
	{	//Otherwise start at the project's path
		initial = eof_last_eof_path;
	}
	returnedfn = ncd_file_select(0, initial, "Import GH Chart", eof_filter_gh_files);
	eof_clear_input();
	if(returnedfn)
	{
		strncpy(returnedfn_path, returnedfn, sizeof(returnedfn_path) - 1);	//Back up this path for later use

		/* destroy loaded song */
		if(eof_song)
		{
			eof_destroy_song(eof_song);
			eof_song = NULL;
			eof_song_loaded = 0;
		}
		(void) eof_destroy_ogg();

		/* import chart */
		eof_song = eof_import_gh(returnedfn);
		if(eof_song)
		{
			eof_song_loaded = 1;
			eof_init_after_load(0);
			(void) replace_filename(eof_last_gh_path, returnedfn_path, "", 1024);	//Set the last loaded GH file path
			eof_cleanup_beat_flags(eof_song);	//Update anchor flags as necessary for any time signature changes
			eof_skip_mid_beats_in_measure_numbering = 0;	//Disable this measure numbering alteration so that any relevant warnings can be given by eof_detect_mid_measure_ts_changes() below
			eof_beat_stats_cached = 0;
			eof_detect_mid_measure_ts_changes();
///			eof_skip_mid_beats_in_measure_numbering = 1;	//Enable mid beat tempo changes to be ignored in the measure numbering now that any applicable warnings were given
			(void) eof_check_for_notes_preceding_sections(0);	//Warn if there are notes that precede the first section event, if there are any sections
			eof_beat_stats_cached = 0;
		}
		else
		{
			allegro_message("Could not import GH file!");
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

int eof_menu_file_ghl_import(void)
{
	char returnedfn_path[1024] = {0}, *returnedfn = NULL;
	char *initial;
	char newchart = 0;	//Is set to nonzero if a new chart is created to store the imported GHL file

	if(!eof_song || !eof_song_loaded)
	{	//If no project is loaded
		newchart = 1;
	}

	eof_log("eof_menu_file_ghl_import() entered", 1);

	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	if((eof_last_ghl_path[uoffset(eof_last_ghl_path, ustrlen(eof_last_ghl_path) - 1)] == '\\') || (eof_last_ghl_path[uoffset(eof_last_ghl_path, ustrlen(eof_last_ghl_path) - 1)] == '/'))
	{	//If the path ends in a separator
		eof_last_ghl_path[uoffset(eof_last_ghl_path, ustrlen(eof_last_ghl_path) - 1)] = '\0';	//Remove it
	}
	if(eof_imports_recall_last_path && eof_folder_exists(eof_last_ghl_path))
	{	//If the user chose for the GHL import dialog to start at the path of the last imported GHL file and that path is valid
		initial = eof_last_ghl_path;	//Use it
	}
	else
	{	//Otherwise start at the project's path
		initial = eof_last_eof_path;
	}
	returnedfn = ncd_file_select(0, initial, "Import GHL Chart", eof_filter_ghl_files);
	eof_clear_input();
	if(returnedfn)
	{
		strncpy(returnedfn_path, returnedfn, sizeof(returnedfn_path) - 1);	//Back up this path for later use

		/* import chart */
		if(newchart)
		{	//If a project wasn't already opened when the import was started
/*			if(!eof_command_line_gp_import(returnedfn))
			{	//If the file was imported
				eof_init_after_load(0);
			}
			else
			{	//Import failed
				eof_destroy_song(eof_song);
				eof_song = NULL;
				eof_song_loaded = 0;
				eof_changes = 0;
			}
*/
		}
		else
		{	//A project was already open
			if(!eof_ghl_import_common(returnedfn))
			{	//If the file imported
				(void) replace_filename(eof_last_ghl_path, returnedfn_path, "", 1024);	//Set the last loaded GHL file path
				eof_cleanup_beat_flags(eof_song);				//Update anchor flags as necessary for any time signature changes
				eof_skip_mid_beats_in_measure_numbering = 0;	//Disable this measure numbering alteration so that any relevant warnings can be given by eof_detect_mid_measure_ts_changes() below
				eof_beat_stats_cached = 0;
				eof_detect_mid_measure_ts_changes();
				eof_beat_stats_cached = 0;
			}
			else
			{
				allegro_message("Could not import GHL file!");
			}
		}
		(void) replace_filename(eof_last_gp_path, returnedfn_path, "", 1024);	//Set the last loaded GP file path
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
			if(size)
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
		case 4:
		{
			return "Bandfuse";
		}
		case 5:
		{
			return "IMMERROCK";
		}

		default:
		break;
	}

	return NULL;
}

struct eof_guitar_pro_struct *eof_parsed_gp_file;

DIALOG eof_gp_import_dialog[] =
{
	/* (proc)            (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)            (dp2) (dp3) */
	{ eof_window_proc,0,   48,  500, 232, 2,   23,  0,    0,      0,   0,   "Import which GP track into the project's active track?",       NULL, NULL },
	{ d_agup_list_proc,  12,  84,  400, 138, 2,   23,  0,    0,      0,   0,   (void *)eof_gp_tracks_list, NULL, NULL },
	{ d_agup_push_proc,  425, 84,  68,  28,  2,   23,  'i',  D_EXIT, 0,   0,   "&Import",      NULL, (void *)eof_gp_import_track },
	{ d_agup_button_proc,12,  235, 240, 28,  2,   23,  '\r', D_EXIT, 0,   0,   "Cancel",       NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

int eof_gp_import_drum_track(int importvoice, int function)
{
	unsigned long populated = 0, populated2 = 0, bitmask, ctr, ctr2, selected;
	char fivelane = 0;		//Set to nonzero if a lane 6 drum note (based on mappings in gp_drum_import_lane_6[] ) causes the destination drum track(s) to be converted to 5 lane
	int combo = 0;			//Track whether any cymbal+tom notes were imported
	int gracenote_prompt = 0;
	int this_is_gracenote = 0, previous_was_gracenote = 0;	//Tracks whether the current and previous notes were before the beat grace notes, optionally used to signal flam drum notation

	eof_log("\tImporting as a drum track", 1);
	selected = eof_gp_import_dialog[1].d1;
	if(function & 1)
	{	//Import into normal drum track
		populated = eof_get_track_size(eof_song, EOF_TRACK_DRUM);
		eof_selected_track = EOF_TRACK_DRUM;
	}
	if(function & 2)
	{	//Import into Phase Shift drum track
		populated2 = eof_get_track_size(eof_song, EOF_TRACK_DRUM_PS);
		eof_selected_track = EOF_TRACK_DRUM_PS;
	}

	//Prompt about overwriting the active track or track difficulty as appropriate
	eof_clear_input();
	if(populated || populated2)
	{	//If the destination track(s) have notes in them
		eof_clear_input();
		if(alert("The destination drum track(s) have notes", "Importing this GP track will overwrite their contents", "Continue?", "&Yes", "&No", 'y', 'n') != 1)
		{	//If the drum track(s) are already populated and the user doesn't opt to overwrite them
			eof_log("\t\tImport canceled", 1);
			return 0;
		}
	}
	if(!gp_import_undo_made)
	{	//If an undo state wasn't already made
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make one now
		gp_import_undo_made = 1;
	}
	if(function & 1)
	{	//Import into normal drum track
		eof_erase_track(eof_song, EOF_TRACK_DRUM, 1);
	}
	if(function & 2)
	{	//Import into Phase Shift drum track
		eof_erase_track(eof_song, EOF_TRACK_DRUM_PS, 1);
	}

	//Copy notes
	eof_pro_guitar_track_sort_notes(eof_parsed_gp_file->track[selected]);
	for(ctr = 0; ctr < eof_parsed_gp_file->track[selected]->notes; ctr++)
	{	//For each note in the GP track
		EOF_PRO_GUITAR_NOTE *gnp;
		unsigned long lastflags = 0;
		unsigned char lastnote = 0;

		if(!(importvoice & (eof_parsed_gp_file->track[selected]->note[ctr]->type + 1)))
			continue;	//If this voice was not chosen for import, skip it

		this_is_gracenote = 0;	//Reset this status
		gnp = eof_parsed_gp_file->track[selected]->note[ctr];
		for(ctr2 = 0, bitmask = 1; ctr2 < 6; ctr2++, bitmask <<= 1)
		{	//For each of the 6 usable strings
			EOF_NOTE *np;
			unsigned char note = 0;
			unsigned long flags = 0, psflags = 0;
			unsigned char fret;

			if(!(gnp->note & bitmask))
				continue;	//If this string does not have a gem, skip it

			if(gnp->tflags & EOF_NOTE_TFLAG_GRACE)
			{	//If this note is a before the beat grace note
				this_is_gracenote = 1;	//Track this
			}
			fret = gnp->frets[ctr2] & 0x7F;
			if(eof_lookup_drum_mapping(gp_drum_import_lane_1, fret))
			{	//Maps to the bass drum
				note = 1;
			}
			else if(eof_lookup_drum_mapping(gp_drum_import_lane_2, fret))
			{	//Maps to snare
				note = 2;
			}
			else if(eof_lookup_drum_mapping(gp_drum_import_lane_2_rimshot, fret))
			{	//Maps to rim shot
				note = 2;
				psflags |= EOF_DRUM_NOTE_FLAG_R_RIMSHOT;
			}
			else if(eof_lookup_drum_mapping(gp_drum_import_lane_3, fret))
			{	//Maps to yellow tom
				if((lastnote & 4) && (lastflags & EOF_DRUM_NOTE_FLAG_Y_CYMBAL))
				{	//If one of the other strings for this note already defined a yellow cymbal
					flags |= EOF_DRUM_NOTE_FLAG_Y_COMBO;	//This yellow gem will be treated as a yellow cymbal+tom
					combo = 1;
				}
				note = 4;
			}
			else if(eof_lookup_drum_mapping(gp_drum_import_lane_3_cymbal, fret))
			{	//Maps to yellow cymbal
				if((lastnote & 4) && !(lastflags & EOF_DRUM_NOTE_FLAG_Y_CYMBAL))
				{	//If one of the other strings for this note already defined a yellow tom
					flags |= EOF_DRUM_NOTE_FLAG_Y_COMBO;	//This yellow gem will be treated as a yellow cymbal+tom
					combo = 1;
				}
				note = 4;
				flags |= EOF_DRUM_NOTE_FLAG_Y_CYMBAL;
			}
			else if(eof_lookup_drum_mapping(gp_drum_import_lane_3_hi_hat_pedal, fret))
			{	//Maps to pedal hi hat
				if((lastnote & 4) && !(lastflags & EOF_DRUM_NOTE_FLAG_Y_CYMBAL))
				{	//If one of the other strings for this note already defined a yellow tom
					flags |= EOF_DRUM_NOTE_FLAG_Y_COMBO;	//This yellow gem will be treated as a yellow cymbal+tom
					combo = 1;
				}
				note = 4;
				flags |= EOF_DRUM_NOTE_FLAG_Y_CYMBAL;
				psflags |= EOF_DRUM_NOTE_FLAG_Y_HI_HAT_PEDAL;
			}
			else if(eof_lookup_drum_mapping(gp_drum_import_lane_3_hi_hat_open, fret))
			{	//Maps to open hi hat
				if((lastnote & 4) && !(lastflags & EOF_DRUM_NOTE_FLAG_Y_CYMBAL))
				{	//If one of the other strings for this note already defined a yellow tom
					flags |= EOF_DRUM_NOTE_FLAG_Y_COMBO;	//This yellow gem will be treated as a yellow cymbal+tom
					combo = 1;
				}
				note = 4;
				flags |= EOF_DRUM_NOTE_FLAG_Y_CYMBAL;
				psflags |= EOF_DRUM_NOTE_FLAG_Y_HI_HAT_OPEN;
			}
			else if(eof_lookup_drum_mapping(gp_drum_import_lane_4, fret))
			{	//Maps to blue tom
				if((lastnote & 8) && (lastflags & EOF_DRUM_NOTE_FLAG_B_CYMBAL))
				{	//If one of the other strings for this note already defined a blue cymbal
					flags |= EOF_DRUM_NOTE_FLAG_B_COMBO;	//This blue gem will be treated as a blue cymbal+tom
					combo = 1;
				}
				note = 8;
			}
			else if(eof_lookup_drum_mapping(gp_drum_import_lane_4_cymbal, fret))
			{	//Maps to blue cymbal
				if((lastnote & 8) && !(lastflags & EOF_DRUM_NOTE_FLAG_B_CYMBAL))
				{	//If one of the other strings for this note already defined a blue tom
					flags |= EOF_DRUM_NOTE_FLAG_B_COMBO;	//This blue gem will be treated as a blue cymbal+tom
					combo = 1;
				}
				note = 8;
				flags |= EOF_DRUM_NOTE_FLAG_B_CYMBAL;
			}
			else if(eof_lookup_drum_mapping(gp_drum_import_lane_5, fret))
			{	//Maps to green tom
				if((lastnote & 16) && (lastflags & EOF_DRUM_NOTE_FLAG_G_CYMBAL))
				{	//If one of the other strings for this note already defined a green cymbal
					flags |= EOF_DRUM_NOTE_FLAG_G_COMBO;	//This green gem will be treated as a green cymbal+tom
					combo = 1;
				}
				note = 16;
			}
			else if(eof_lookup_drum_mapping(gp_drum_import_lane_5_cymbal, fret))
			{	//Maps to green cymbal
				if((lastnote & 16) && !(lastflags & EOF_DRUM_NOTE_FLAG_G_CYMBAL))
				{	//If one of the other strings for this note already defined a green tom
					flags |= EOF_DRUM_NOTE_FLAG_G_COMBO;	//This green gem will be treated as a green cymbal+tom
					combo = 1;
				}
				note = 16;
				flags |= EOF_DRUM_NOTE_FLAG_G_CYMBAL;
			}
			else if(eof_lookup_drum_mapping(gp_drum_import_lane_6, fret))
			{
				if(!fivelane)
				{	//If this is the first lane 6 drum note parsed
					fivelane = 1;
				}
				note = 32;
			}

			if(fivelane == 1)
			{	//If the sixth lane hasn't been enabled for the destination track(s) yet
				if(function & 1)
				{	//Import into normal drum track
					eof_song->track[EOF_TRACK_DRUM]->flags |= EOF_TRACK_FLAG_SIX_LANES;					//Set the six lane flag
					eof_song->legacy_track[eof_song->track[EOF_TRACK_DRUM]->tracknum]->numlanes = 6;	//Set the lane count
				}
				if(function & 2)
				{	//Import into Phase Shift drum track
					eof_song->track[EOF_TRACK_DRUM_PS]->flags |= EOF_TRACK_FLAG_SIX_LANES;				//Set the six lane flag
					eof_song->legacy_track[eof_song->track[EOF_TRACK_DRUM_PS]->tracknum]->numlanes = 6;	//Set the lane count
				}
				fivelane = 2;
			}

			if(previous_was_gracenote && !this_is_gracenote)
			{	//If a previously parsed note was a grace note, and this one isn't
				if(!gracenote_prompt)
				{	//If the user wasn't prompted yet
					eof_clear_input();
					if(alert("There is at least one before-the-beat grace note", NULL, "Import the notes that follow these as flams?", "&Yes", "&No", 'y', 'n') == 1)
					{	//If the user opts to treat this as flam drum notation
						gracenote_prompt = 1;
					}
					else
					{
						gracenote_prompt = 2;
					}
				}
				if(gracenote_prompt == 1)
				{	//If the user opted to treat this as a flam note
					flags |= EOF_DRUM_NOTE_FLAG_FLAM;
				}
			}

			if(function & 1)
			{	//Import into normal drum track
				np = eof_track_add_create_note(eof_song, EOF_TRACK_DRUM, note, gnp->pos, 1, EOF_NOTE_AMAZING, NULL);
				if(!np)
				{	//If the memory couldn't be allocated
					allegro_message("Error allocating memory.  Aborting");
					eof_log("\t\tImport failed", 1);
					return 0;
				}
				np->flags = flags;
			}
			if(function & 2)
			{	//Import into Phase Shift drum track
				np = eof_track_add_create_note(eof_song, EOF_TRACK_DRUM_PS, note, gnp->pos, 1, EOF_NOTE_AMAZING, NULL);
				if(!np)
				{	//If the memory couldn't be allocated
					allegro_message("Error allocating memory.  Aborting");
					eof_log("\t\tImport failed", 1);
					return 0;
				}
				np->flags = flags | psflags;
			}
			lastflags = flags | psflags;
			lastnote = note;
		}//For each of the 6 usable strings
		previous_was_gracenote = this_is_gracenote;	//The next iteration will remember if this note was a grace note
	}//For each note in the GP track

	if(combo)
	{	//If one or more cymbal+tom notes were imported
		if(function == 3)
		{	//If the drum notes were imported into both the normal and the Phase Shift drum tracks
			allegro_message("Note:  The imported drum track contains cymbal+tom notes, but these notes are only compatible with the Phase Shift drum track.  This status was retained for the notes imported into the Phase Shift drum track, but not the ones imported into the normal drum track.");
		}
		else if(!(function & 2))
		{	//If the drum notes were only imported into the normal drum track
			allegro_message("Note:  The imported drum track contains cymbal+tom notes, but these notes are only compatible with the Phase Shift drum track.  Re-import the Guitar Pro file into that drum track is this notation is wanted.");
		}
	}
	eof_fixup_notes(eof_song);
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tImport complete.  %lu drum notes created", eof_get_track_size(eof_song, eof_selected_track));
	eof_log(eof_log_string, 1);
	return D_CLOSE;
}

int eof_gp_import_guitar_track(int importvoice)
{
	unsigned long ctr, ctr2, selected;
	char overlaps, tuning_prompted = 0, is_bass = 0;
	char still_populated = 0;	//Will be set to nonzero if the track still contains notes after the track/difficulty is cleared before importing the GP track
	EOF_PHRASE_SECTION *ptr, *ptr2;

	if(eof_song && eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
	{	//Only continue if a pro guitar/bass track is active
		unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;

		selected = eof_gp_import_dialog[1].d1;
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tImporting track #%lu (%s) from GP file", selected + 1, eof_parsed_gp_file->names[selected]);
		eof_log(eof_log_string, 1);

		eof_menu_track_set_tech_view_state(eof_song, eof_selected_track, 0);	//Disable tech view if applicable

		//Prompt about overwriting the active track or track difficulty as appropriate
		eof_clear_input();
		if(eof_gp_import_replaces_track)
		{	//If the user preference to replace the entire active track with the imported track is enabled
			if(eof_get_track_size_all(eof_song, eof_selected_track) && alert("This track already has notes", "Importing this GP track will overwrite this track's contents", "Continue?", "&Yes", "&No", 'y', 'n') != 1)
			{	//If the active track is already populated (with normal or tech notes) and the user doesn't opt to overwrite it
				eof_log("\t\tImport canceled", 1);
				return 0;
			}
		}
		else
		{	//If the imported track will only replace the active track difficulty
			if(eof_track_diff_populated_status[eof_note_type] && alert("This track difficulty already has notes", "Importing this GP track will overwrite this difficulty's contents", "Continue?", "&Yes", "&No", 'y', 'n') != 1)
			{	//If the active track difficulty is already populated and the user doesn't opt to overwrite it
				eof_log("\t\tImport canceled", 1);
				return 0;
			}
		}
		if(!gp_import_undo_made)
		{	//If an undo state wasn't already made
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make one now
			gp_import_undo_made = 1;
		}
		if(eof_gp_import_replaces_track)
		{	//If the user preference to replace the entire active track with the imported track is enabled
			eof_erase_track(eof_song, eof_selected_track, 1);	//Erase the active track
		}
		else
		{
			eof_erase_track_difficulty(eof_song, eof_selected_track, eof_note_type);	//Otherwise erase the active track difficulty
		}
		if(eof_get_track_size_all(eof_song, eof_selected_track))
		{	//If the track still has notes in it after the removal of the track or track difficulty's notes
				still_populated = 1;
		}

		//Copy the content from the selected GP track into the active track
		//Copy notes
		eof_pro_guitar_track_sort_notes(eof_parsed_gp_file->track[selected]);
		for(ctr = 0; ctr < eof_parsed_gp_file->track[selected]->notes; ctr++)
		{	//For each note in the GP track
			if(importvoice & (eof_parsed_gp_file->track[selected]->note[ctr]->type + 1))
			{	//If this voice is to be imported
				EOF_PRO_GUITAR_NOTE *np = eof_pro_guitar_track_add_note(eof_song->pro_guitar_track[tracknum]);	//Allocate a new note
				if(!np)
				{	//If the memory couldn't be allocated
					allegro_message("Error allocating memory.  Aborting");
					eof_log("\t\tImport failed", 1);
					return 0;
				}
				memcpy(np, eof_parsed_gp_file->track[selected]->note[ctr], sizeof(EOF_PRO_GUITAR_NOTE));	//Clone the note from the GP track
				np->type = eof_note_type;	//Update the note difficulty
			}
		}
		//Copy tech notes
		for(ctr = 0; ctr < eof_parsed_gp_file->track[selected]->technotes; ctr++)
		{	//For each tech note in the GP track
			if(importvoice & (eof_parsed_gp_file->track[selected]->technote[ctr]->type + 1))
			{	//If this voice is to be imported
				EOF_PRO_GUITAR_NOTE *np = eof_pro_guitar_track_add_tech_note(eof_song->pro_guitar_track[tracknum]);	//Allocate a new note
				if(!np)
				{	//If the memory couldn't be allocated
					allegro_message("Error allocating memory.  Aborting");
					eof_log("\t\tImport failed", 1);
					return 0;
				}
				memcpy(np, eof_parsed_gp_file->track[selected]->technote[ctr], sizeof(EOF_PRO_GUITAR_NOTE));	//Clone the tech note from the GP track
				np->type = eof_note_type;	//Update the note difficulty
			}
		}
		//Copy trill phrases
		for(ctr = 0, overlaps = 0; ctr < eof_parsed_gp_file->track[selected]->trills; ctr++)
		{	//For each trill phrase in the GP track
			ptr = &eof_parsed_gp_file->track[selected]->trill[ctr];
			for(ctr2 = 0; ctr2 < eof_get_num_trills(eof_song, eof_selected_track); ctr2++)
			{	//For each trill section already in the active track
				ptr2 = eof_get_trill(eof_song, eof_selected_track, ctr2);
				if((ptr->end_pos >= ptr2->start_pos) && (ptr->start_pos <= ptr2->end_pos))
				{	//If the trill phrase in the GP track overlaps with any trill phrase already in the active track
					overlaps = 1;	//Make a note
					break;
				}
			}
			if(!overlaps)
			{	//If this trill phrase doesn't overlap with any existing trill phrases in the active track
				(void) eof_track_add_section(eof_song, eof_selected_track, EOF_TRILL_SECTION, 0, ptr->start_pos, ptr->end_pos, 0, NULL);	//Copy it to the active track
			}
		}
		//Copy tremolo phrases
		for(ctr = 0; ctr < eof_parsed_gp_file->track[selected]->tremolos; ctr++)
		{	//For each tremolo phrase in the GP track
			overlaps = 0;	//Reset this status
			ptr = &eof_parsed_gp_file->track[selected]->tremolo[ctr];
			for(ctr2 = 0; ctr2 < eof_get_num_tremolos(eof_song, eof_selected_track); ctr2++)
			{	//For each tremolo section already in the active track
				ptr2 = eof_get_tremolo(eof_song, eof_selected_track, ctr2);
				if((ptr->end_pos >= ptr2->start_pos) && (ptr->start_pos <= ptr2->end_pos) && (ptr->difficulty == eof_note_type))
				{	//If the tremolo phrase in the GP track overlaps with any tremolo phrase already in the active track difficulty
					overlaps = 1;	//Make a note
					break;
				}
			}
			if(!overlaps)
			{	//If this tremolo phrase doesn't overlap with any existing tremolo phrases in the active track
				(void) eof_track_add_section(eof_song, eof_selected_track, EOF_TREMOLO_SECTION, eof_note_type, ptr->start_pos, ptr->end_pos, 0, NULL);	//Copy it to the active track difficulty
			}
		}

		//Copy the imported track's tuning, string count, fret count and capo position into the active track
		if(eof_detect_string_gem_conflicts(eof_song->pro_guitar_track[tracknum], eof_parsed_gp_file->track[selected]->numstrings))
		{	//If the track being imported has a different string count that would cause notes to be removed from the track
			if(alert("Warning:  The imported track's string count is lower than the active track.", "Applying the string count will alter/delete existing notes in the track.", "Apply the imported track's string count?", "&Yes", "&No", 'y', 'n') == 1)
			{	//If user opts to change the string count even though notes will be altered/deleted
				eof_song->pro_guitar_track[tracknum]->numstrings = eof_parsed_gp_file->track[selected]->numstrings;
			}
		}
		else
		{	//If the imported track's string count doesn't conflict with any notes that were already in the active track
			eof_song->pro_guitar_track[tracknum]->numstrings = eof_parsed_gp_file->track[selected]->numstrings;
		}
		if(eof_get_highest_fret(eof_song, eof_selected_track) > eof_song->pro_guitar_track[tracknum]->numfrets)
		{	//If the track being imported uses a fret value that is higher than what the active track's fret limit
			eof_song->pro_guitar_track[tracknum]->numfrets = eof_get_highest_fret(eof_song, eof_selected_track);	//Update the fret limit
		}
		if(eof_track_is_bass_arrangement(eof_song->pro_guitar_track[tracknum], eof_selected_track))
		{	//If the track receiving the Guitar Pro import is configured as a bass guitar track
			is_bass = 1;
		}
		if(is_bass && (eof_parsed_gp_file->instrument_types[selected] == 1))
		{	//If the imported GP track is a guitar track and the user is importing it into an EOF track that's configured as a bass arrangement
			if(alert("You are importing a guitar arrangement into a bass track.", NULL, "Update the recipient track's type to a non-bass arrangement type?", "&Yes", "&No", 'y', 'n') == 1)
			{	//If the user opts to alter the arrangement type
				eof_song->pro_guitar_track[tracknum]->arrangement = 0;	//Set it to undefined guitar arrangement
			}
		}
		else if(!is_bass && (eof_parsed_gp_file->instrument_types[selected] == 2))
		{	//If the imported GP track is a bass track and the user is importing it into an EOF track that's configured as a guitar arrangement
			if(alert("You are importing a bass arrangement into a guitar track.", NULL, "Update the recipient track's type to a bass arrangement type?", "&Yes", "&No", 'y', 'n') == 1)
			{	//If the user opts to alter the arrangement type
				eof_song->pro_guitar_track[tracknum]->arrangement = EOF_BASS_ARRANGEMENT;	//Set it to bass arrangement
			}
		}
		for(ctr = 0; ctr < 6; ctr++)
		{	//For each of the 6 supported strings
			int new_tuning;
			int default_tuning = eof_lookup_default_string_tuning_absolute(eof_song->pro_guitar_track[tracknum], eof_selected_track, ctr);	//Get the default absolute tuning for this string
			if(default_tuning < 0)
				continue;	//If the default tuning was not found, skip the string

			new_tuning = eof_parsed_gp_file->track[selected]->tuning[ctr] - default_tuning;	//Convert the tuning to relative
			new_tuning %= 12;	//Ensure the stored value is bounded to [-11,11]
			if(!tuning_prompted && still_populated && (new_tuning != eof_song->pro_guitar_track[tracknum]->tuning[ctr]))
			{	//If applying the imported track's tuning would alter the tuning for other notes that are already in the track, prompt the user
				if(alert("Warning:  The imported track's tuning is different than the track's current tuning.", "Applying the tuning will affect the existing notes in the track.", "Apply the imported track's tuning?", "&Yes", "&No", 'y', 'n') == 1)
				{	//If the user opts to set the imported track's tuning even though the active track still has notes in it
					tuning_prompted = 1;	//Note that the user answered yes
				}
				else
				{
					tuning_prompted = 2;	//Note that the user answered no
				}
			}
			if(tuning_prompted != 2)
			{	//If the user didn't decline to apply the imported track's tuning
				eof_song->pro_guitar_track[tracknum]->tuning[ctr] = new_tuning;	//Apply the tuning to this string
			}
		}
		eof_song->pro_guitar_track[tracknum]->capo = eof_parsed_gp_file->track[selected]->capo;	//Apply the capo position
		eof_song->pro_guitar_track[tracknum]->ignore_tuning = eof_parsed_gp_file->track[selected]->ignore_tuning;	//Apply the option whether or not to ignore the tuning for chord name lookups
		eof_track_sort_notes(eof_song, eof_selected_track);	//Sort notes so tech notes display with the correct status
	}//Only perform this action if a pro guitar/bass track is active

	eof_log("\t\tImport complete", 1);
	return D_CLOSE;
}

int eof_gp_import_track(DIALOG * d)
{
	unsigned long ctr, selected;
	int voicespresent = 0, importvoice = 0;

	if(!d || (eof_parsed_gp_file->numtracks > INT_MAX) || (d->d1 >= (int)eof_parsed_gp_file->numtracks))
		return 0;

	selected = eof_gp_import_dialog[1].d1;

	//Parse the track to see which voices were populated, and if more than one is, prompt user which voice(s) to import
	for(ctr = 0; ctr < eof_parsed_gp_file->track[selected]->notes; ctr++)
	{	//For each note in the GP track
		EOF_PRO_GUITAR_NOTE *np, *pnp;

		if(eof_parsed_gp_file->track[selected]->note[ctr]->type == 0)
		{	//This is a note in the lead voice
			voicespresent |= 1;
		}
		else
		{	//This is a note in the bass voice
			voicespresent |= 2;
		}
		if((voicespresent == 3) && !importvoice)
		{	//If notes in both voices have been encountered and the user hasn't been prompted about which to import
			int ret = alert3(NULL, "Import which voices from this track?", NULL, "&Lead", "&Bass", "B&Oth", 'l', 'b', 'o');
			if(ret == 1)
			{	//Import lead voice only
				importvoice = 1;
			}
			else if(ret == 2)
			{	//Import bass voice only
				importvoice = 2;
			}
			else
			{	//Import both voices
				importvoice = 3;
			}
		}
		if(importvoice != 3)
			continue;	//If the user has not opted to import both voices, skip the logic below

		//In order for importvoice to be 3, at least two notes will have been parsed so ctr is guaranteed to be greater than 0
		np = eof_parsed_gp_file->track[selected]->note[ctr];	//Simplify
		pnp = eof_parsed_gp_file->track[selected]->note[ctr - 1];
		if(np->pos == pnp->pos)
		{	//If it starts at the same time stamp, ensure both notes have the same length so that eof_track_find_crazy_notes() works as intended
			if(np->length > pnp->length)
			{	//This note is longer
				pnp->length = np->length;
			}
			else
			{	//The other note is longer or of equal length
				np->length = pnp->length;
			}
		}
	}
	if(!importvoice)
	{	//If only one voice was in the Guitar Pro file had notes
		if(voicespresent == 1)
		{	//Import the lead voice if it was the only voice with notes
			importvoice = 1;
		}
		else
		{	//Import the bass voice if it was the only voice with notes
			importvoice = 2;
		}
	}

	if(eof_parsed_gp_file->instrument_types[selected] == 3)
	{	//If this is a drum track
		eof_clear_input();
		if(alert(NULL, "Import the selected track as a drum track?", NULL, "&Yes", "&No", 'y', 'n') == 1)
		{	//If the user opts to import this drum track as a drum track
			int ret = alert3(NULL, "Import into which drum track(s)?", NULL, "&Normal", "&Phase Shift", "&Both", 'n', 'p', 'b');

			return eof_gp_import_drum_track(importvoice, ret);
		}
	}

	return eof_gp_import_guitar_track(importvoice);	//Import into the active pro guitar track
}

char gp_import_undo_made;
int eof_gp_import_common(const char *fn)
{
	unsigned long ctr, ctr2;

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Return failure
	if(!fn)
		return 1;	//If no file name is specified, return failure

	eof_parsed_gp_file = eof_load_gp(fn, &gp_import_undo_made);	//Parse the GP file, make an undo state if time signatures are imported

	if(eof_parsed_gp_file)
	{	//The file was successfully parsed, allow the user to import a track into the active project
		eof_clear_input();
		eof_cursor_visible = 0;
		eof_render();

//If the GP file contained section markers, offer to import them now
		if(eof_parsed_gp_file->text_events)
		{	//If there were text events imported
			eof_clear_input();
			if(alert(NULL, "Import Guitar Pro file's section markers/beat text as Rocksmith phrases/sections?", NULL, "&Yes", "&No", 'y', 'n') == 1)
			{	//If the user opts to import RS phrases and sections from GP files
				if(!gp_import_undo_made)
				{	//If an undo state hasn't been made yet
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					gp_import_undo_made = 1;
				}
				for(ctr = 0; ctr < eof_parsed_gp_file->text_events; ctr++)
				{	//For each of the text events
					(void) eof_song_add_text_event(eof_song, eof_parsed_gp_file->text_event[ctr]->pos, eof_parsed_gp_file->text_event[ctr]->text, 0, eof_parsed_gp_file->text_event[ctr]->flags, 0);	//Add the event to the active project
				}
			}
			for(ctr = 0; ctr < eof_parsed_gp_file->text_events; ctr++)
			{	//For each text event parsed from the Guitar Pro file
				free(eof_parsed_gp_file->text_event[ctr]);	//Free the event
			}
			eof_parsed_gp_file->text_events = 0;
			eof_sort_events(eof_song);
		}

		eof_color_dialog(eof_gp_import_dialog, gui_fg_color, gui_bg_color);
		eof_conditionally_center_dialog(eof_gp_import_dialog);
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
			for(ctr2 = 0; ctr2 < eof_parsed_gp_file->track[ctr]->technotes; ctr2++)
			{	//For each tech note in the track
				free(eof_parsed_gp_file->track[ctr]->technote[ctr2]);	//Free its memory
			}
			free(eof_parsed_gp_file->track[ctr]);	//Free the pro guitar track
		}
		free(eof_parsed_gp_file->names);
		free(eof_parsed_gp_file->track);
		free(eof_parsed_gp_file->instrument_types);
		free(eof_parsed_gp_file);

		if(!(eof_song->track[eof_selected_track]->flags & EOF_TRACK_FLAG_UNLIMITED_DIFFS))
		{	//If the track's difficulty limit is in place
			for(ctr2 = 0; ctr2 < eof_get_num_tremolos(eof_song, eof_selected_track); ctr2++)
			{	//For each tremolo phrase in the track
				EOF_PHRASE_SECTION *ptr = eof_get_tremolo(eof_song, eof_selected_track, ctr2);
				if(ptr)
				{	//If the tremolo was successfully found
					if(ptr->difficulty == eof_note_type)
					{	//If the tremolo phrase is specific to the active track, it was newly imported
						if(alert(NULL, "Remove the track difficulty limit to show imported tremolo phrases?", NULL, "&Yes", "&No", 'y', 'n') == 1)
						{	//If the user opts to remove the track difficulty limit
							(void) eof_track_rocksmith_toggle_difficulty_limit();
						}
						break;
					}
				}
			}
		}
	}//The file was successfully parsed...
	else
	{
		allegro_message("Failure.  Check log for details.");
		return 1;	//Return failure
	}

	if(!eof_music_length)
	{	//If eof_truncate_chart() is run before audio is loaded, eof_music_length will be 0 and imported beat timings will be lost as beats are dropped from an empty project
		eof_log("Skipping beat cleanup while no audio is loaded, to avoid losing tempo mapping", 1);
	}
	else
	{
		eof_log("Cleaning up beats", 1);
		eof_truncate_chart(eof_song);	//Remove excess beat markers and update the eof_chart_length variable
	}
	eof_beat_stats_cached = 0;		//Mark the cached beat stats as not current
	eof_log("Cleaning up imported notes", 1);
	eof_track_find_crazy_notes(eof_song, eof_selected_track, 1);	//Mark notes that overlap others as crazy, if they don't begin at the same timestamp (ie. should become a normal chord)
	eof_track_fixup_notes(eof_song, eof_selected_track, 1);			//Run fixup logic to clean up the track
	eof_track_fixup_notes(eof_song, eof_selected_track, 1);			//Run fixup logic again to ensure that notes that were combined into chords (on the previous call) during a multi-voice import truncate as appropriate
	(void) eof_menu_track_selected_track_number(eof_selected_track, 1);	//Re-select the active track to allow for a change in string count

	return 0;	//Return success
}

int eof_menu_file_gp_import(void)
{
	char returnedfn_path[1024] = {0}, *returnedfn = NULL;
	char *initial;
	char newchart = 0;	//Is set to nonzero if a new chart is created to store the imported RS file

	if(!eof_song || !eof_song_loaded)
	{	//If no project is loaded
		newchart = 1;
	}
	else
	{
		if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
			return 1;	//Don't do anything unless the active track is a pro guitar/bass track
	}

	gp_import_undo_made = 0;	//Will ensure only one undo state is created during this import
	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	if((eof_last_gp_path[uoffset(eof_last_gp_path, ustrlen(eof_last_gp_path) - 1)] == '\\') || (eof_last_gp_path[uoffset(eof_last_gp_path, ustrlen(eof_last_gp_path) - 1)] == '/'))
	{	//If the path ends in a separator
		eof_last_gp_path[uoffset(eof_last_gp_path, ustrlen(eof_last_gp_path) - 1)] = '\0';	//Remove it
	}
	if(eof_imports_recall_last_path && eof_folder_exists(eof_last_gp_path))
	{	//If the user chose for the GP import dialog to start at the path of the last imported GP file and that path is valid
		initial = eof_last_gp_path;	//Use it
	}
	else
	{	//Otherwise start at the project's path
		initial = eof_last_eof_path;
	}
	returnedfn = ncd_file_select(0, initial, "Import Guitar Pro", eof_filter_gp_files);
	eof_clear_input();
	if(returnedfn)
	{	//If a file was selected for import
		strncpy(returnedfn_path, returnedfn, sizeof(returnedfn_path) - 1);	//Back up this path for later use

		if(newchart)
		{	//If a project wasn't already opened when the import was started
			if(!eof_command_line_gp_import(returnedfn))
			{	//If the file was imported
				eof_init_after_load(0);
				(void) ustrcpy(eof_song->tags->frettist, eof_last_frettist);
			}
			else
			{	//Import failed
				eof_destroy_song(eof_song);
				eof_song = NULL;
				eof_song_loaded = 0;
				eof_changes = 0;
			}
		}
		else
		{	//A project was already open
			(void) eof_gp_import_common(returnedfn);
		}
		(void) replace_filename(eof_last_gp_path, returnedfn_path, "", 1024);	//Set the last loaded GP file path
	}
	eof_render();

	return 1;
}

int eof_command_line_gp_import(char *fn)
{
	char nfn[1024] = {0};

	eof_log("eof_command_line_gp_import() entered", 1);

	if(!fn)
		return 1;	//Return error

	//Create a new project and have user select a target pro guitar/bass track
	(void) snprintf(eof_etext, sizeof(eof_etext) - 1, "Import Guitar Pro (guitar) arrangement to:");
	if(!eof_create_new_project_select_pro_guitar())
		return 2;	//New project couldn't be created

	if(eof_gp_import_common(fn))
	{	//if there was an error importing the file
		return 3;	//Return error
	}

//Update path variables
	(void) ustrcpy(eof_filename, fn);
	(void) replace_filename(eof_song_path, fn, "", 1024);	//Set the project folder path
	(void) replace_filename(eof_last_eof_path, eof_filename, "", 1024);
	(void) ustrcpy(eof_loaded_song_name, get_filename(eof_filename));
	(void) replace_extension(eof_loaded_song_name, eof_loaded_song_name, "eof", 1024);

//Load guitar.ogg automatically if it's present, otherwise prompt user to browse for audio
	(void) append_filename(nfn, eof_song_path, "guitar.ogg", 1024);
	if(!eof_load_ogg(nfn, 1))	//If user does not provide audio, fail over to using silent audio
	{
		eof_destroy_song(eof_song);
		eof_song = NULL;
		eof_song_loaded = 0;
		return 4;	//Return error
	}

//Run beat cleanup now that chart audio is loaded
	eof_log("Cleaning up beats", 1);
	eof_truncate_chart(eof_song);	//Remove excess beat markers and update the eof_chart_length variable

	return 0;	//Return success
}

char * eof_gp_tracks_list(int index, int * size)
{
	if(index < 0)
	{	//Signal to return the list count
		if(size)
		{
			if(eof_parsed_gp_file)
			{
				*size = eof_parsed_gp_file->numtracks;
			}
			else
			{	//eof_parsed_gp_file pointer is not valid
				*size = 0;
			}
		}
	}
	else
	{	//Return the specified list item
		if(eof_parsed_gp_file && (eof_parsed_gp_file->numtracks <= INT_MAX) && (index < (int)eof_parsed_gp_file->numtracks))
		{	//eof_parsed_gp_file and the referenced track index are valid
			return eof_parsed_gp_file->names[index];
		}
	}
	return NULL;
}

DIALOG eof_set_display_width_dialog[] =
{
	/* (proc)                (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags)   (d1) (d2) (dp)                 (dp2) (dp3) */
	{ eof_window_proc,    0,   48,  214, 116, 2,   23,   0,      0,      0,   0,   "Set display width",NULL, NULL },
	{ d_agup_text_proc,      12,  76,  64,  8,   2,   23,   0,      0,      0,   0,   eof_etext3,         NULL, NULL },
	{ d_agup_text_proc,      12,  98,  64,  8,   2,   23,   0,      0,      0,   0,   "Width:",           NULL, NULL },
	{ eof_verified_edit_proc,60,  94,  100, 20,  2,   23,   0,      0,      4,   0,   eof_etext2,         "0123456789", NULL },
	{ d_agup_button_proc,    17,  122, 84,  28,  2,   23,   '\r',   D_EXIT, 0,   0,   "OK",               NULL, NULL },
	{ d_agup_button_proc,    113, 122, 78,  28,  2,   23,   0,      D_EXIT, 0,   0,   "Cancel",           NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

int eof_set_display_width(void)
{
	unsigned long width;

	(void) snprintf(eof_etext, sizeof(eof_etext) - 1, "Set display width (>= %lu)", eof_screen_width_default);
	eof_set_display_width_dialog[0].dp = eof_etext;	//Update the dialog window's title

	(void) snprintf(eof_etext3, sizeof(eof_etext3) - 1, "(Currently set to %lu)", eof_screen_width);
	eof_etext2[0] = '\0';
	eof_color_dialog(eof_set_display_width_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_set_display_width_dialog);
	if(eof_popup_dialog(eof_set_display_width_dialog, 3) == 4)	//User hit OK
	{
		if(eof_etext2[0] != '\0')
		{	//If a width was specified
			width = atol(eof_etext2);

			if(width >= eof_screen_width_default)
			{	//If the specified width is valid
				(void) eof_set_display_mode(width, eof_screen_height);	//Resize the program window to the specified width and the current height
			}
		}
	}
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(screen);

	if(eof_enable_notes_panel)
	{	//If the notes window must be rebuilt
		eof_enable_notes_panel = 0;	//Toggle this because the function call below will toggle it back to on
		(void) eof_display_notes_panel();
	}

	//Update coordinate related items
	eof_scale_fretboard(0);			//Recalculate the 2D screen positioning based on the current track
	eof_set_2D_lane_positions(0);	//Update ychart[] by force just in case the display window size was changed
	eof_set_3D_lane_positions(0);	//Update xchart[] by force just in case the display window size was changed

	if(eof_screen_width < eof_get_display_panel_count() * EOF_SCREEN_PANEL_WIDTH)
	{	//If the current screen width isn't sufficient to display all enabled panels
		eof_enable_notes_panel = 0;	//Disable the notes panel
	}

	return D_O_K;
}

DIALOG eof_set_3d_hopo_scale_size_dialog[] =
{
	/* (proc)                (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags)   (d1) (d2) (dp)                     (dp2) (dp3) */
	{ eof_window_proc,    0,   48,  224, 124, 2,   23,   0,      0,      0,   0,   "Set 3D HOPO image scale size",NULL, NULL },
	{ d_agup_text_proc,      12,  76,  64,  8,   2,   23,   0,      0,      0,   0,   "To this % of normal sized gems:", NULL, NULL },
	{ d_agup_text_proc,      12,  92,  64,  8,   2,   23,   0,      0,      0,   0,   "(Must be between 10 and 200)", NULL, NULL },
	{ eof_verified_edit_proc,12,  110, 100, 20,  2,   23,   0,      0,      4,   0,   eof_etext2,         "0123456789", NULL },
	{ d_agup_button_proc,    17,  136, 84,  28,  2,   23,   '\r',   D_EXIT, 0,   0,   "OK",               NULL, NULL },
	{ d_agup_button_proc,    113, 136, 78,  28,  2,   23,   0,      D_EXIT, 0,   0,   "Cancel",           NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

int eof_set_3d_hopo_scale_size(void)
{
	unsigned long percent;

	(void) snprintf(eof_etext2, sizeof(eof_etext2) - 1, "%d", eof_3d_hopo_scale_size);
	eof_color_dialog(eof_set_3d_hopo_scale_size_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_set_3d_hopo_scale_size_dialog);
	if(eof_popup_dialog(eof_set_3d_hopo_scale_size_dialog, 3) == 4)	//User hit OK
	{
		if(eof_etext2[0] != '\0')
		{	//If a percentage was specified
			eof_log("Changing 3D HOPO scale size.", 1);
			percent = atol(eof_etext2);

			if((percent < 10) || (percent > 200))
			{
				allegro_message("Invalid scale size.");
				return 0;
			}

			eof_3d_hopo_scale_size = percent;
			if(!eof_load_and_scale_hopo_images((double)eof_3d_hopo_scale_size / 100.0))
			{
				allegro_message("Error reloading HOPO images.  EOF must exit, please save if necessary.");
				(void) eof_menu_file_exit();
			}
		}
	}
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(screen);
	eof_render();

	return D_O_K;
}

DIALOG eof_set_3d_camera_angle_dialog[] =
{
	/* (proc)                (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags)   (d1) (d2) (dp)                                 (dp2) (dp3) */
	{ eof_window_proc,    0,   48,  230, 132, 2,   23,   0,      0,      0,   0,   "Set 3D camera angle",              NULL, NULL },
	{ d_agup_text_proc,      12,  76,  64,  8,   2,   23,   0,      0,      0,   0,   "Specify a value from -500 (max)",  NULL, NULL },
	{ d_agup_text_proc,      12,  92,  64,  8,   2,   23,   0,      0,      0,   0,   "to 260 (min) or 0 for default",    NULL, NULL },
	{ d_agup_text_proc,      12,  114, 64,  8,   2,   23,   0,      0,      0,   0,   "Angle:",                           NULL, NULL },
	{ eof_verified_edit_proc,60,  110, 100, 20,  2,   23,   0,      0,      4,   0,   eof_etext2,                         "-0123456789", NULL },
	{ d_agup_button_proc,    17,  138, 84,  28,  2,   23,   '\r',   D_EXIT, 0,   0,   "OK",                               NULL, NULL },
	{ d_agup_button_proc,    129, 138, 84,  28,  2,   23,   0,      D_EXIT, 0,   0,   "Cancel",                           NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

int eof_set_3d_camera_angle(void)
{
	long angle;

	eof_etext2[0] = '\0';
	eof_color_dialog(eof_set_3d_camera_angle_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_set_3d_camera_angle_dialog);
	if(eof_popup_dialog(eof_set_3d_camera_angle_dialog, 4) == 5)	//User hit OK
	{
		if(eof_etext2[0] != '\0')
		{	//If a width was specified
			angle = atol(eof_etext2);

			if((angle >= -500) && (angle <= 260))
			{	//If the specified angle is valid
				eof_vanish_y = angle;
				eof_3d_fretboard_coordinates_cached = 0;	//The 3D rendering logic will need to rebuild the fretboard's 2D coordinate projections
				eof_set_3d_projection();
			}
		}
	}
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(screen);

	return D_O_K;
}

int eof_toggle_display_zoom(void)
{
	eof_log("\tToggling x2 zoom", 1);
	eof_screen_zoom ^= 1;	//Toggle this value
	(void) eof_set_display_mode(eof_screen_width, eof_screen_height);	//Redefine graphic primitive sizes to fit 480 height display mode

	//Update coordinate related items
	eof_scale_fretboard(0);			//Recalculate the 2D screen positioning based on the current track
	eof_set_2D_lane_positions(0);	//Update ychart[] by force just in case the display window size was changed
	eof_set_3D_lane_positions(0);	//Update xchart[] by force just in case the display window size was changed

	//Re-initialize the notes panel if it was destroyed when the program window was redrawn
	if(eof_enable_notes_panel)
	{
		eof_enable_notes_panel = 0;	//Toggle this because the function call below will toggle it back to on
		(void) eof_display_notes_panel();
	}

	eof_render();
	eof_prepare_file_menu();		//The checkmark indicating the status of this feature doesn't reliably update when activating/de-activating by keyboard, force it to update
	eof_close_menu = 1;				//Force the main menu to close, as this function had a tendency to get hung in the menu logic when activated by keyboard
	return D_O_K;
}

int eof_rs_import_common(char *fn)
{
	EOF_PRO_GUITAR_TRACK *tp = NULL;
	unsigned long tracknum;

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	if(fn && (tracknum < EOF_PRO_GUITAR_TRACKS_MAX))
	{	//If the file name and active track are valid
		tp = eof_load_rs(fn);

		if(tp)
		{	//If the track was imported, replace the active track
			eof_erase_track(eof_song, eof_selected_track, 0);	//Delete all notes, tech notes, etc, PRESERVING text events which were added during import
			tp->parent = eof_song->pro_guitar_track[tracknum]->parent;
			tp->parent->flags |= EOF_TRACK_FLAG_UNLIMITED_DIFFS;	//Remove the difficulty limit for this track
			free(eof_song->pro_guitar_track[tracknum]);	//Free the active track
			eof_song->pro_guitar_track[tracknum] = tp;	//Replace it with the track imported from the Rocksmith file

			eof_log("Cleaning up beats", 1);
			eof_truncate_chart(eof_song);	//Remove excess beat markers and update the eof_chart_length variable
			eof_beat_stats_cached = 0;		//Mark the cached beat stats as not current
			eof_log("Cleaning up imported notes", 1);
			eof_song->track[eof_selected_track]->numdiffs = eof_detect_difficulties(eof_song, eof_selected_track);	//Update the number of difficulties used in this track
			eof_track_sort_notes(eof_song, eof_selected_track);	//Sort the notes
			eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Run fixup logic to clean up the track
			eof_menu_track_set_tech_view_state(eof_song, eof_selected_track, 1);	//Activate the tech note set
			eof_track_sort_notes(eof_song, eof_selected_track);	//Sort the tech notes
			eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Run fixup logic to clean up the tech notes
			eof_menu_track_set_tech_view_state(eof_song, eof_selected_track, 0);	//Activate the normal note set
			(void) eof_menu_track_selected_track_number(eof_selected_track, 1);	//Re-select the active track to allow for a change in string count
			eof_sort_events(eof_song);
			eof_delete_blank_events(eof_song);
			eof_cleanup_beat_flags(eof_song);	//Make corrections to beat statuses if necessary
		}
		else
		{
			allegro_message("Failure.  Check log for details.");
			return 1;	//Return failure
		}
	}
	else
	{
		return 1;	//Return failure
	}

	return 0;	//Return success
}

int eof_menu_file_rs_import(void)
{
	char returnedfn_path[1024] = {0}, *returnedfn = NULL;
	char *initial;
	char newchart = 0;	//Is set to nonzero if a new chart is created to store the imported RS file

	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();

	if(!eof_song || !eof_song_loaded)
	{	//If no project is loaded
		newchart = 1;
	}
	else
	{
		if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
			return 1;	//Don't do anything unless the active track is a pro guitar/bass track

		eof_clear_input();
		if(eof_get_track_size_all(eof_song, eof_selected_track) && alert("This track already has notes", "Importing this Rocksmith track will overwrite this track's contents", "Continue?", "&Yes", "&No", 'y', 'n') != 1)
		{	//If the active track is already populated (by either normal or tech notes) and the user doesn't opt to overwrite it
			return 0;
		}
	}

	if((eof_last_rs_path[uoffset(eof_last_rs_path, ustrlen(eof_last_rs_path) - 1)] == '\\') || (eof_last_rs_path[uoffset(eof_last_rs_path, ustrlen(eof_last_rs_path) - 1)] == '/'))
	{	//If the path ends in a separator
		eof_last_rs_path[uoffset(eof_last_rs_path, ustrlen(eof_last_rs_path) - 1)] = '\0';	//Remove it
	}
	if(eof_imports_recall_last_path && eof_folder_exists(eof_last_rs_path))
	{	//If the user chose for the Rocksmith import dialog to start at the path of the last imported Rocksmith file and that path is valid
		initial = eof_last_rs_path;	//Use it
	}
	else
	{	//Otherwise start at the project's path
		initial = eof_last_eof_path;
	}
	returnedfn = ncd_file_select(0, initial, "Import Rocksmith", eof_filter_rs_files);
	eof_clear_input();
	if(returnedfn)
	{	//If a file was selected for import
		strncpy(returnedfn_path, returnedfn, sizeof(returnedfn_path) - 1);	//Back up this path for later use

		if(newchart)
		{	//If a project wasn't already opened when the import was started
			if(!eof_command_line_rs_import(returnedfn))
			{	//If the file was imported
				eof_init_after_load(0);
			}
			else
			{	//Import failed
				eof_destroy_song(eof_song);
				eof_song = NULL;
				eof_song_loaded = 0;
				eof_changes = 0;
			}
		}
		else
		{	//A project was already open
			(void) eof_rs_import_common(returnedfn);	//Import the specified Rocksmith XML file into the active pro guitar/bass track
		}
		(void) replace_filename(eof_last_rs_path, returnedfn_path, "", 1024);	//Set the last loaded Rocksmith file path
	}
	eof_render();

	return D_O_K;
}

int eof_command_line_rs_import(char *fn)
{
	char nfn[1024] = {0};

	eof_log("eof_command_line_rs_import() entered", 1);

	if(!fn)
		return 1;	//Return error

	//Create a new project and have user select a target pro guitar/bass track
	(void) snprintf(eof_etext, sizeof(eof_etext) - 1, "Import Rocksmith arrangement to:");
	if(strcasestr_spec(fn, "_bonus"))
	{	//If the name of the specified XML file implies it was originally a bonus track
		eof_import_to_track_dialog[3].flags = 0;			//Deselect PART REAL_GUITAR as the default destination track for the import
		eof_import_to_track_dialog[6].flags = D_SELECTED;	//Select PART REAL_GUITAR_BONUS instead
	}
	else if(strcasestr_spec(fn, "_bass"))
	{	//If the name of the specified XML file implies bass guitar
		eof_import_to_track_dialog[3].flags = 0;			//Deselect PART REAL_GUITAR as the default destination track for the import
		if(strstr(fn, "_22"))
		{	//If the name of the file implies it was originally a 22 fret track
			eof_import_to_track_dialog[4].flags = D_SELECTED;	//Select PART REAL_BASS_22 instead
		}
		else
		{
			eof_import_to_track_dialog[2].flags = D_SELECTED;	//Select PART REAL_BASS instead
		}
	}
	else if(strstr(fn, "_22"))
	{	//If the name of the file implies it was originally a 22 fret track
		eof_import_to_track_dialog[3].flags = 0;			//Deselect PART REAL_GUITAR as the default destination track for the import
		eof_import_to_track_dialog[5].flags = D_SELECTED;	//Select PART REAL_GUITAR_22 instead
	}
	if(!eof_create_new_project_select_pro_guitar())
		return 2;	//New project couldn't be created

	if(eof_rs_import_common(fn))
	{	//if there was an error importing the file
		return 3;	//Return error
	}

//Update path variables
	(void) ustrcpy(eof_filename, fn);
	(void) replace_filename(eof_song_path, fn, "", 1024);	//Set the project folder path
	(void) replace_filename(eof_last_eof_path, eof_filename, "", 1024);
	(void) ustrcpy(eof_loaded_song_name, get_filename(eof_filename));
	(void) replace_extension(eof_loaded_song_name, eof_loaded_song_name, "eof", 1024);

//Load guitar.ogg automatically if it's present, otherwise prompt user to browse for audio
	(void) append_filename(nfn, eof_song_path, "guitar.ogg", 1024);
	if(!eof_load_ogg(nfn, 1))	//If user does not provide audio, fail over to using silent audio
	{
		eof_destroy_song(eof_song);
		eof_song = NULL;
		eof_song_loaded = 0;
		return 4;	//Return error
	}

	return 0;	//Return success
}

int eof_menu_file_sonic_visualiser_import(void)
{
	char * returnedfn = NULL, *initial, *ptr;
	PACKFILE *inf = NULL;
	size_t maxlinelength;
	char *buffer = NULL, error = 0, tempo_s[15] = {0}, undo_made = 0, done = 0;
	unsigned long linectr = 1, ctr, beatctr = 0, pointctr = 0;
	long samplerate = 0, frame = 0;
	double frametime, tempo_f, beatlen = 500.0, timectr = 0.0, lastbeatlen = 500.0;

	eof_log("eof_menu_file_sonic_visualiser_import() entered", 1);

	if(!eof_song || !eof_song_loaded)
		return 1;	//For now, don't do anything unless a project is active

	if(eof_song->tags->tempo_map_locked)
	{	//If the user has locked the tempo map
		eof_clear_input();
		if(alert(NULL, "The tempo map must be unlocked in order to import a Sonic Visualiser file.  Continue?", NULL, "&Yes", "&No", 'y', 'n') != 1)
		{	//If the user does not opt to unlock the tempo map
			eof_log("\tUser cancellation.  Aborting", 1);
			return 1;
		}
		eof_song->tags->tempo_map_locked = 0;	//Unlock the tempo map
	}

	if((eof_last_sonic_visualiser_path[uoffset(eof_last_sonic_visualiser_path, ustrlen(eof_last_sonic_visualiser_path) - 1)] == '\\') || (eof_last_sonic_visualiser_path[uoffset(eof_last_sonic_visualiser_path, ustrlen(eof_last_sonic_visualiser_path) - 1)] == '/'))
	{	//If the path ends in a separator
		eof_last_sonic_visualiser_path[uoffset(eof_last_sonic_visualiser_path, ustrlen(eof_last_sonic_visualiser_path) - 1)] = '\0';	//Remove it
	}
	if(eof_imports_recall_last_path && eof_folder_exists(eof_last_sonic_visualiser_path))
	{	//If the user chose for the Sonic Visualiser import dialog to start at the path of the last imported file and that path is valid
		initial = eof_last_sonic_visualiser_path;	//Use it
	}
	else
	{	//Otherwise start at the project's path
		initial = eof_last_eof_path;
	}
	returnedfn = ncd_file_select(0, initial, "Import Sonic Visualiser", eof_filter_sonic_visualiser_files);
	eof_clear_input();
	if(!returnedfn)
		return 1;	//If the user did not select a file, return immediately

	inf = pack_fopen(returnedfn, "rt");	//Open file in text mode
	if(!inf)
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError loading:  Cannot open input SVL file:  \"%s\"", strerror(errno));	//Get the Operating System's reason for the failure
		eof_log(eof_log_string, 1);
		return 1;
	}

	//Allocate memory buffers large enough to hold any line in this file
	maxlinelength = (size_t)FindLongestLineLength_ALLEGRO(returnedfn, 0);
	if(!maxlinelength)
	{
		eof_log("\tError finding the largest line in the file.  Aborting", 1);
		(void) pack_fclose(inf);
		return 1;
	}
	buffer = (char *)malloc(maxlinelength);
	if(!buffer)
	{
		eof_log("\tError allocating memory.  Aborting", 1);
		(void) pack_fclose(inf);
		return 1;
	}

	//Read first line of text, capping it to prevent buffer overflow
	if(!pack_fgets(buffer, (int)maxlinelength, inf))
	{	//I/O error
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tSonic Visualiser import failed on line #%lu:  Unable to read from file:  \"%s\"", linectr, strerror(errno));
		eof_log(eof_log_string, 1);
		error = 1;
	}

	//Parse the contents of the file
	while(!error && !done && !pack_feof(inf))
	{	//Until there was an error reading from the file or end of file is reached
		#ifdef RS_IMPORT_DEBUG
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tProcessing line #%lu", linectr);
			eof_log(eof_log_string, 3);
		#endif

		//Separate the line into the opening XML tag (buffer) and the content between the opening and closing tag (buffer2)
		ptr = strcasestr_spec(buffer, ">");
		if(!ptr)
		{	//This line had no XML, skip it
			(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text, so the EOF condition can be checked
			linectr++;
			continue;
		}

		if(strcasestr_spec(buffer, "<model"))
		{	//If this is a model tag
			if(parse_xml_attribute_number("sampleRate", buffer, &samplerate) != 1)
			{	//If the sample rate couldn't be read
				eof_log("\tError reading sample rate.  Aborting", 1);
				error = 1;
				break;
			}
		}
		else if(strcasestr_spec(buffer, "<point"))
		{	//If this is a point tag
			if(!samplerate)
			{	//If the sample rate hadn't been read yet
				eof_log("\tError:  Sample rate not defined.  Aborting", 1);
				error = 1;
				break;
			}
			if(parse_xml_attribute_number("frame", buffer, &frame) != 1)
			{	//If the frame number couldn't be read
				eof_log("\tError reading frame number.  Aborting", 1);
				error = 1;
				break;
			}
			frametime = (double)frame / ((double)samplerate / 1000.0);	//Convert point timing to realtime in milliseconds

			//Read either the value or label attribute (prefer the value attribute as it is higher precision), convert to floating point and determine the beat length
			if(parse_xml_attribute_text(tempo_s, sizeof(tempo_s), "value", buffer) != 1)
			{	//If the tempo value isn't defined (only available when the tempo estimation function is used in Sonic Visualiser instead of the beat estimation function)
				if(parse_xml_attribute_text(tempo_s, sizeof(tempo_s), "label", buffer) != 1)
				{	//If the tempo label isn't defined either
					eof_log("\tError:  Tempo not defined.  Aborting", 1);
					error = 1;
					break;
				}
				if(!strcasestr_spec(tempo_s, "BPM"))
				{	//If this isn't a tempo definition
					tempo_s[0] = '\0';	//Empty the string
				}
				else
				{
					for(ctr = 0; ctr < (unsigned long)strlen(tempo_s) && (tempo_s[ctr] != '\0'); ctr++)
					{	//For each character in the label string
						if(!isdigit(tempo_s[ctr]) && (tempo_s[ctr] != '.'))
						{	//If this character isn't a number or decimal point
							tempo_s[ctr] = '\0';	//Truncate string
							break;
						}
					}
				}
			}
			if(tempo_s[0] == '\0')
			{	//If the tempo string is empty (the last point tag in a beat estimation defines an empty tag attribute), keep the last beat length in effect
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tFrame = %ld\ttime = %fms", frame, frametime);
				eof_log(eof_log_string, 1);
				beatlen = 0;
			}
			else
			{	//Otherwise convert the string to floating point
				tempo_f = atof(tempo_s);
				beatlen = 60000.0 / tempo_f;	//Get the length (in milliseconds) of one beat using this tempo (for now, don't take time signature into consideration because I don't believe Sonic Visualizer does)
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tFrame = %ld\ttime = %fms\ttempo = %.3fBPM\tbeat length = %f", frame, frametime, tempo_f, beatlen);
				eof_log(eof_log_string, 1);
			}

			if(!lastbeatlen && beatctr)
			{	//If the previous beat was defined with a timestamp but without a tempo change
				lastbeatlen = frametime - eof_song->beat[beatctr - 1]->fpos;	//Define that beat's length as the time difference between it and the current beat
			}

			pointctr++;

			//Apply the beat timing
			if(!undo_made)
			{	//If an undo hasn't been made
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				undo_made = 1;
			}
			if(pointctr > 1)
			{	//If this isn't the first point tag, apply timings for all beats between this point tag and the previous (if any)
				while(timectr + lastbeatlen + 1 < frametime)
				{	//While another beat fits before this frame's timestamp with at least an extra millisecond of room
					if(beatctr >= eof_song->beats)
					{	//If another beat needs to be added to the project
						if(!eof_song_append_beats(eof_song, 1))
						{	//If a beat couldn't be added
							eof_log("\tError allocating memory to add a beat.  Aborting", 1);
							error = 1;
							break;
						}
					}
					timectr += lastbeatlen;	//Advance the time counter by one beat length
					eof_song->beat[beatctr]->fpos = timectr;
					eof_song->beat[beatctr]->pos = eof_song->beat[beatctr]->fpos + 0.5;	//Round up to nearest ms
					beatctr++;
				}
				if(error)
				{	//If an error was reached
					break;	//Exit outer loop
				}
			}
			else
			{	//This is the first point tag, update the chart's MIDI delay
				eof_song->tags->ogg[0].midi_offset = frametime + 0.5;
			}
			if(beatctr >= eof_song->beats)
			{	//If another beat needs to be added to the project
				if(!eof_song_append_beats(eof_song, 1))
				{	//If a beat couldn't be added
					eof_log("\tError allocating memory to add a beat.  Aborting", 1);
					error = 1;
					break;
				}
				if(eof_chart_length < eof_song->beat[eof_song->beats - 1]->pos)
				{	//If the chart length needs to be updated to reflect the Sonic Visualizer file making the chart longer
					eof_chart_length = eof_song->beat[eof_song->beats - 1]->pos;
				}
			}
			eof_song->beat[beatctr]->fpos = frametime;
			eof_song->beat[beatctr]->pos = eof_song->beat[beatctr]->fpos + 0.5;	//Round up to nearest ms
			beatctr++;
			timectr = frametime;	//Track the position of the last processed beat
			lastbeatlen = beatlen;	//Track the beat length of the last processed point tag
		}//If this is a point tag
		else if(strcasestr_spec(buffer, "</dataset>"))
		{	//If this is the end of the dataset tag containing the point tags
			done = 1;
		}

		(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
		linectr++;	//Increment line counter
	}//Until there was an error reading from the file or end of file is reached

	if(!lastbeatlen)
	{	//If the beats were defined as timestamps and not with tempo changes
		if(beatctr > 2)
		{	//Bounds check
			lastbeatlen = eof_song->beat[beatctr - 1]->fpos - eof_song->beat[beatctr - 2]->fpos;
		}
		else
		{	//Something unusual is going on, use a generic 120BPM beat length
			lastbeatlen = 500;
		}
	}

	if(error)
	{
		if(undo_made)
		{	//If an undo state was made
			(void) eof_undo_apply();	//Load it
		}
		allegro_message("Sonic Visualiser import failed, see log for details.");
	}
	else
	{
		while((beatctr > 0) && (beatctr < eof_song->beats))
		{	//While there are beats whose timings weren't covered by the imported file
			eof_song->beat[beatctr]->fpos = eof_song->beat[beatctr - 1]->fpos + lastbeatlen;	//Apply the beat length defined by the last point frame
			eof_song->beat[beatctr]->pos = eof_song->beat[beatctr]->fpos + 0.5;				//Round up to nearest ms
			beatctr++;
		}
	}

	//Cleanup
	eof_calculate_tempo_map(eof_song);	//Determine all tempo changes based on the beats' timestamps
	eof_truncate_chart(eof_song);		//Remove excess beat markers and update the eof_chart_length variable
	(void) replace_filename(eof_last_sonic_visualiser_path, returnedfn, "", 1024);	//Set the last loaded Sonic Visualiser file path
	free(buffer);
	(void) pack_fclose(inf);
	eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current

	return 1;
}

int eof_menu_file_bf_import(void)
{
	char returnedfn_path[1024] = {0}, *returnedfn = NULL;
	char *initial;

	eof_log("eof_menu_file_bf_import() entered", 1);

	if(eof_menu_prompt_save_changes() == 3)
	{	//If user canceled closing the current, modified chart
		return 1;
	}
	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	if((eof_last_bf_path[uoffset(eof_last_bf_path, ustrlen(eof_last_bf_path) - 1)] == '\\') || (eof_last_bf_path[uoffset(eof_last_bf_path, ustrlen(eof_last_bf_path) - 1)] == '/'))
	{	//If the path ends in a separator
		eof_last_bf_path[uoffset(eof_last_bf_path, ustrlen(eof_last_bf_path) - 1)] = '\0';	//Remove it
	}
	if(eof_imports_recall_last_path && eof_folder_exists(eof_last_bf_path))
	{	//If the user chose for the Rocksmith import dialog to start at the path of the last imported Rocksmith file and that path is valid
		initial = eof_last_bf_path;	//Use it
	}
	else
	{	//Otherwise start at the project's path
		initial = eof_last_eof_path;
	}
	returnedfn = ncd_file_select(0, initial, "Import Bandfuse", eof_filter_bf_files);
	eof_clear_input();
	if(returnedfn)
	{
		eof_log("\tImporting Bandfuse", 1);
		strncpy(returnedfn_path, returnedfn, sizeof(returnedfn_path) - 1);	//Back up this path for later use

		if(eof_song)
		{
			eof_destroy_song(eof_song);
			eof_song = NULL;
			eof_song_loaded = 0;
			eof_changes = 0;
			eof_undo_last_type = 0;
			eof_change_count = 0;
		}
		(void) eof_destroy_ogg();
		(void) replace_filename(eof_last_bf_path, eof_filename, "", 1024);

		eof_song = eof_load_bf(returnedfn);
		if(eof_song)
		{
			eof_song_loaded = 1;
			eof_init_after_load(0);
			eof_sort_notes(eof_song);
			eof_fixup_notes(eof_song);
			(void) eof_detect_difficulties(eof_song, eof_selected_track);
			(void) replace_filename(eof_last_bf_path, returnedfn_path, "", 1024);	//Set the last loaded Bandfuse file path
			eof_cleanup_beat_flags(eof_song);	//Update anchor flags as necessary for any time signature changes
			eof_log("\tImport complete", 1);
		}
		else
		{
			allegro_message("Failure.  Check log for details.");
			eof_log("\tImport failed", 1);
		}
	}
	eof_reset_lyric_preview_lines();
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_render();

	return D_O_K;
}

int eof_menu_file_export_chart_range(void)
{
	char oggpath[1024] = {0};
	char temppath[1024] = {0};
	char syscommand[1024] = {0};
	char new_foldername[1024] = {0};
	char * returnedfn = NULL;
	unsigned long start = 0, end;
	EOF_SONG *csp;

	if(!eof_song)
		return 0;	//No chart open

	if(eof_validate_temp_folder())
	{	//Ensure the correct working directory and presence of the temporary folder
		eof_log("\tCould not validate working directory and temp folder", 1);
		return 1;
	}

	eof_log("eof_menu_file_export_chart_range() entered", 1);

	//Initialize start and end timestamps
	end = eof_determine_chart_length(eof_song);	//By default, start and end encompass all chart content
	if(eof_seek_selection_start != eof_seek_selection_end)
	{	//If there is a seek selection
		start = eof_seek_selection_start;
		end = eof_seek_selection_end;
	}
	else if((eof_song->tags->start_point != ULONG_MAX) && (eof_song->tags->end_point != ULONG_MAX) && (eof_song->tags->start_point != eof_song->tags->end_point))
	{	//If both the start and end points are defined with different timestamps
		start = eof_song->tags->start_point;
		end = eof_song->tags->end_point;
	}

	strncpy(eof_etext, "Export chart range", sizeof(eof_etext) - 1);	//Set the title of the eof_menu_song_time_range_dialog dialog
	if(!eof_run_time_range_dialog(&start, &end))	//If a valid time range isn't provided by the user
		return 0;									//Cancel

	eof_log("\tCloning chart", 1);
	csp = eof_clone_chart_time_range(eof_song, start, end);	//Build a new EOF project containing the specified time range's content from the active project
	if(!csp)
	{
		eof_log("Failed to clone project.", 1);
		return 0;
	}

	returnedfn = ncd_file_select(1, eof_last_eof_path, "Export Song As", eof_filter_eof_files);
	eof_clear_input();
	if(returnedfn)
	{
		eof_log("\tSaving cloned chart", 1);

		(void) replace_extension(temppath, returnedfn, "eof", 1024);		//Ensure the chart is saved with a .eof extension
		(void) replace_filename(new_foldername, temppath, "", 1024);		//Obtain the chosen destination folder path
		if(eof_menu_file_new_supplement(new_foldername, get_filename(temppath), 4) == 0)	//If the folder doesn't exist, or the user has declined to overwrite any existing files
		{
			eof_destroy_song(csp);
			return 0;	//Return failure
		}

		(void) replace_extension(oggpath, temppath, "ogg", 1024);			//Build the path to the target OGG file to create
		(void) ustrcpy(csp->tags->ogg[0].filename, get_filename(oggpath));	//Create a default OGG profile using this name
		if(!eof_save_song(csp, temppath))
		{
			eof_log("Failed to save clone project.", 1);
			eof_destroy_song(csp);
			return 0;
		}

		if(!eof_silence_loaded)
		{	//If chart audio is loaded
			eof_log("\tSaving audio excerpt.", 1);
			(void) replace_extension(temppath, temppath, "wav", 1024);
			eof_export_audio_time_range(eof_music_track, start / 1000.0, end / 1000.0, temppath);	//Build the WAV file
			if(exists(temppath))
			{	//If the WAV file was created, convert it to OGG
				(void) delete_file(oggpath);	//Delete any existing OGG file with the same name
				#ifdef ALLEGRO_WINDOWS
					(void) uszprintf(syscommand, (int) sizeof(syscommand), "oggenc2 --quiet -q %s --resample 44100 -s 0 \"%s\" -o \"%s\"", eof_ogg_quality[(int)eof_ogg_setting], temppath, oggpath);
				#else
					(void) uszprintf(syscommand, (int) sizeof(syscommand), "oggenc --quiet -q %s --resample 44100 -s 0 \"%s\" -o \"%s\"", eof_ogg_quality[(int)eof_ogg_setting], temppath, oggpath);
				#endif
				(void) eof_system(syscommand);
				(void) delete_file(temppath);	//Delete the temporary WAV file
			}
		}
	}

	eof_destroy_song(csp);
	return 1;
}

int eof_menu_file_export_audio_range(void)
{
	char oggpath[1024] = {0};
	char wavpath[1024] = {0};
	char syscommand[1024] = {0};
	char * returnedfn = NULL;
	unsigned long start = 0, end;

	if(!eof_song)
		return 0;	//No chart open
	if(eof_silence_loaded)
		return 0;	//No audio loaded

	if(eof_validate_temp_folder())
	{	//Ensure the correct working directory and presence of the temporary folder
		eof_log("\tCould not validate working directory and temp folder", 1);
		return 1;
	}

	eof_log("eof_menu_file_export_audio_range() entered", 1);

	//Initialize start and end timestamps
	end = eof_determine_chart_length(eof_song);	//By default, start and end encompass all chart content
	if(eof_seek_selection_start != eof_seek_selection_end)
	{	//If there is a seek selection
		start = eof_seek_selection_start;
		end = eof_seek_selection_end;
	}
	else if((eof_song->tags->start_point != ULONG_MAX) && (eof_song->tags->end_point != ULONG_MAX) && (eof_song->tags->start_point != eof_song->tags->end_point))
	{	//If both the start and end points are defined with different timestamps
		start = eof_song->tags->start_point;
		end = eof_song->tags->end_point;
	}

	strncpy(eof_etext, "Export audio range", sizeof(eof_etext) - 1);	//Set the title of the eof_menu_song_time_range_dialog dialog
	if(!eof_run_time_range_dialog(&start, &end))	//If a valid time range isn't provided by the user
		return 0;									//Cancel

	returnedfn = ncd_file_select(1, eof_last_eof_path, "Export Audio As", eof_filter_ogg_files);
	eof_clear_input();
	if(returnedfn)
	{
		(void) replace_extension(oggpath, returnedfn, "ogg", 1024);		//Ensure the file is saved with a .ogg extension
		if(exists(oggpath))
		{	//If the specified file path already exists
			eof_clear_input();
			if(alert(NULL, "The specified audio file already exists.  Overwrite?", NULL, "&Yes", "&No", 'y', 'n') != 1)
			{	//If the user does not opt to overwrite the file
				return 0;	//Cancel
			}
		}

		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tSaving audio excerpt to \"%s\"", oggpath);
		eof_log(eof_log_string, 1);
		(void) replace_extension(wavpath, oggpath, "wav", 1024);
		eof_export_audio_time_range(eof_music_track, start / 1000.0, end / 1000.0, wavpath);	//Build the WAV file
		if(exists(wavpath))
		{	//If the WAV file was created, convert it to OGG
			(void) delete_file(oggpath);	//Delete any existing OGG file with the same name
			#ifdef ALLEGRO_WINDOWS
				(void) uszprintf(syscommand, (int) sizeof(syscommand), "oggenc2 --quiet -q %s --resample 44100 -s 0 \"%s\" -o \"%s\"", eof_ogg_quality[(int)eof_ogg_setting], wavpath, oggpath);
			#else
				(void) uszprintf(syscommand, (int) sizeof(syscommand), "oggenc --quiet -q %s --resample 44100 -s 0 \"%s\" -o \"%s\"", eof_ogg_quality[(int)eof_ogg_setting], wavpath, oggpath);
			#endif
			(void) eof_system(syscommand);
			(void) delete_file(wavpath);	//Delete the temporary WAV file
		}
	}

	return 1;
}

int eof_menu_file_export_guitar_pro(void)
{
	int retval;
	char temppath1[1024] = {0};
	char temppath2[1024] = {0};
	char temppath3[1024] = {0};
	char temppath4[1024] = {0};
	char temppath5[1024] = {0};
	char tempstr[1024] = {0};
	char tempstr2[1024] = {0};
	char syscommand[1024] = {0};
	unsigned short user_warned = 0xFFFF;	//Mark all warnings as already having been given so the RS exports are silent
	int original_eof_abridged_rs2_export;

	//Ensure RocksmithToTab is linked
	if(!exists(eof_rs_to_tab_executable_path))
	{	//If the path to RocksmithToTab is not defined or otherwise doesn't refer to a valid file
		retval = alert3("RocksmithToTab is not properly linked", "First download and extract the program", "Then use \"File>Link to RocksmithToTab\" to browse to the program", "OK", "Download", "Link", 0, 0, 0);
		if(retval == 2)
		{	//If the user opted to download the program
			allegro_message("Note:   cabalistic's original release has not been updated in a long time as of 2025.  spontiroli forked the project and added some fixes.");
			(void) eof_system("start https://sourceforge.net/projects/rocksmithtotab/");
			(void) eof_system("start https://github.com/spontiroli/rocksmith2tab");
		}
		else if(retval == 3)
		{	//If the user opted to link the program
			return eof_menu_file_link_rs_to_tab();
		}
		return 1;
	}

	//Create temporary XML files
	(void) eof_validate_temp_folder();	//Attempt to set the current working directory if it isn't EOF's executable/resource folder
	original_eof_abridged_rs2_export = eof_abridged_rs2_export;	//Back up the original setting of this user preference
	eof_abridged_rs2_export = 0;	//Force unabridged RS2 export, as RocksmithToTab doesn't currently support that XML variation
	strncpy(temppath1, eof_temp_path_s, sizeof(temppath1) - 1);
	strncpy(temppath2, eof_temp_path_s, sizeof(temppath2) - 1);
	strncpy(temppath3, eof_temp_path_s, sizeof(temppath3) - 1);
	strncpy(temppath4, eof_temp_path_s, sizeof(temppath4) - 1);
	strncpy(temppath5, eof_temp_path_s, sizeof(temppath5) - 1);
	if(eof_export_rocksmith_2_track(eof_song, temppath1, EOF_TRACK_PRO_BASS, &user_warned) != 1)
		temppath1[0] = '\0';	//This arrangement failed to export
	if(eof_export_rocksmith_2_track(eof_song, temppath2, EOF_TRACK_PRO_BASS_22, &user_warned) != 1)
		temppath2[0] = '\0';	//This arrangement failed to export
	if(eof_export_rocksmith_2_track(eof_song, temppath3, EOF_TRACK_PRO_GUITAR, &user_warned) != 1)
		temppath3[0] = '\0';	//This arrangement failed to export
	if(eof_export_rocksmith_2_track(eof_song, temppath4, EOF_TRACK_PRO_GUITAR_22, &user_warned) != 1)
		temppath4[0] = '\0';	//This arrangement failed to export
	if(eof_export_rocksmith_2_track(eof_song, temppath5, EOF_TRACK_PRO_GUITAR_B, &user_warned) != 1)
		temppath5[0] = '\0';	//This arrangement failed to export
	eof_abridged_rs2_export = original_eof_abridged_rs2_export;	//Restore this user preference

	//Call program
	(void) ustrcpy(syscommand, eof_rs_to_tab_executable_path);
	(void) ustrcpy(tempstr2, eof_song_path);
	if(ustrlen(tempstr2))
	{	//As long as a valid project path was obtained
		if(ugetat(tempstr2, ustrlen(tempstr2) - 1) == '\\')
		{	//If the path ends in a folder separator
			(void) uremove(tempstr2, -1);	//Remove the last character in the string, as RocksmithToTab will crash if the output directory path ends in a separator
		}
	}
	(void) uszprintf(tempstr, (int) sizeof(tempstr) - 1, " --xml \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" -o \"%s\"", temppath1, temppath2, temppath3, temppath4, temppath5, tempstr2);
	(void) ustrcat(syscommand, tempstr);
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tCalling RocksmithToTab as follows:  %s", syscommand);
	eof_log(eof_log_string, 1);
	(void) eof_system(syscommand);

	//Cleanup
	(void) delete_file(temppath1);
	(void) delete_file(temppath2);
	(void) delete_file(temppath3);
	(void) delete_file(temppath4);
	return 1;
}

int eof_menu_file_export_song_preview(void)
{
	unsigned long start = 0, stop = 0;
	char targetpath[1024] = {0};
	char syscommand[1024] = {0};
	char wavname[270] = {0};
	unsigned long oldstarttag = 0, oldendtag = 0;
	char *oldstartstring, *oldendstring;

	eof_log("eof_menu_file_export_song_preview() entered", 1);
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
			start = eof_music_pos.value - eof_av_delay;
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
				(void) uszprintf(syscommand, (int) sizeof(syscommand), "oggenc --quiet -q %s --resample 44100 -s 0 \"%s%s\" -o \"%spreview.ogg\"", eof_ogg_quality[eof_ogg_setting], targetpath, wavname, targetpath);
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

int eof_menu_file_export_immerrock_track_diff(void)
{
	unsigned long gglead = 0, ggrhythm = 0, ggbass = 0, tracknum;

	eof_log("eof_menu_file_export_immerrock_track_diff() entered", 1);
	eof_log("\tExporting active track difficulty", 1);

	if(!eof_song || (eof_selected_track >= eof_song->tracks) || (eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return 0;

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	switch(eof_song->pro_guitar_track[tracknum]->arrangement)
	{
		case 2:	//Rhythm arrangement
			ggrhythm = eof_selected_track;
		break;

		case 4:	//Bass arrangement
			ggbass = eof_selected_track;
		break;

		case 3:	//Lead arrrangement (the default type if the active track doesn't have a specified arrangement type)
		default:
			gglead = eof_selected_track;
	}
	(void) replace_filename(eof_temp_filename, eof_song_path, "", 1024);	//Obtain the destination path
	eof_export_immerrock_diff(eof_song, gglead, ggrhythm, ggbass, eof_note_type, eof_temp_filename, 1);

	return 1;
}

void eof_rebuild_notes_window(void)
{
	unsigned long available_width = eof_screen_width;	//Allocate all width that isn't taken up by the info panel or 3D preview to the notes panel

	if(eof_window_notes)
		eof_window_destroy(eof_window_notes);
	eof_window_notes = NULL;

	if(!eof_disable_info_panel)
		available_width -= EOF_SCREEN_PANEL_WIDTH;	//This amount of space is unavailable
	if(!eof_disable_3d_rendering)
		available_width -= EOF_SCREEN_PANEL_WIDTH;	//This amount of space is unavailable
	if(eof_disable_info_panel)
	{	//If the info panel is disabled, the notes panel will render in its place
		eof_window_notes = eof_window_create(0, eof_screen_height / 2, available_width, eof_screen_height / 2, eof_screen);
	}
	else
	{	//Otherwise it will render one panel to the right of it
		eof_window_notes = eof_window_create(EOF_SCREEN_PANEL_WIDTH, eof_screen_height / 2, available_width, eof_screen_height / 2, eof_screen);
	}
	if(!eof_window_notes)
	{
		allegro_message("Unable to create notes window!");
		eof_enable_notes_panel = 0;
	}
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tNew notes panel window instance:  0x%p", eof_window_notes);
	eof_log(eof_log_string, 1);
	if(eof_notes_panel)
	{	//If the Notes panel instance exists
		eof_notes_panel->window = eof_window_notes;	//Update the Notes panel instance with the new window
	}
}

int eof_display_notes_panel_logic(int builtin)
{
	char *default_target = "notes.panel.txt";	//Unless eof_current_notes_panel_path is defined, the default notes panel file will be displayed
	char *effective_target = eof_current_notes_panel_path;	//Unless the target note panel is missing, it will be the file opened

	eof_enable_notes_panel = 1 - eof_enable_notes_panel;	//Toggle the notes panel on/off
	eof_log("\tLoading Notes panel file.", 2);

	if(eof_notes_panel)
	{	//If a notes panel instance already exists
		eof_destroy_text_panel(eof_notes_panel);
		eof_notes_panel = NULL;
	}

	if(eof_enable_notes_panel)
	{	//If the notes panel was just enabled, see if the program window needs to resize
		(void) eof_increase_display_width_to_panel_count(0);	//Resize the program window if necessary, disable notes panel on failure

		if(!builtin)
		{	//If the panel being loaded is NOT a built-in one
			if((eof_current_notes_panel_path[0] == '\0') || !exists(eof_current_notes_panel_path))
			{	//If the Notes panel file path is undefined, or the path does not exist
				effective_target = default_target;	//Resort to loading the standard Notes panel file
				builtin = 1;						//And recovered that file from eof.dat if necessary
			}
		}
		if(eof_validate_temp_folder())
		{	//Ensure the correct working directory and presence of the temporary folder
			eof_log("\tCould not validate working directory and temp folder", 1);
			eof_log_cwd();
			eof_enable_notes_panel = 0;

			return 1;
		}
		eof_notes_panel = eof_create_text_panel(effective_target, builtin);	//Reload the active Notes panel file, recover panel file from eof.dat if necessary
		if(eof_notes_panel == NULL)
		{	//Could not buffer file
			allegro_message("Error opening Notes panel file");
			eof_enable_notes_panel = 0;
			return 1;
		}
		if(eof_enable_notes_panel)
		{	//If the program window is wide enough
			eof_log("\tRebuilding notes window.", 2);
			eof_rebuild_notes_window();
		}
	}

	return 1;
}

int eof_display_notes_panel(void)
{
	return eof_display_notes_panel_logic(0);	//Display the currently configured Notes panel file
}

int eof_3d_preview_toggle_full_height(void)
{
	eof_full_height_3d_preview ^= 1;	//Toggle this feature on/off
	if(!eof_set_display_mode(eof_screen_width, eof_screen_height))
	{	//If the windows couldn't be rebuilt
		eof_full_height_3d_preview = 0;	//Disable the feature
		(void) eof_set_display_mode(eof_screen_width, eof_screen_height);	//Retry rebuilding the windows
	}

	if(eof_enable_notes_panel)
	{	//If the notes window must be rebuilt
		eof_enable_notes_panel = 0;	//Toggle this because the function call below will toggle it back to on
		(void) eof_display_notes_panel();
	}
	eof_scale_fretboard(0);			//Recalculate the 2D screen positioning based on the current track

	//Change the 3D preview depth
	if(eof_full_height_3d_preview)
	{	//If full height 3D preview is in effect
		eof_3d_max_depth = 1200;
	}
	else
	{
		eof_3d_max_depth = 600;
	}
	return 1;
}

int eof_menu_file_notes_panel_notes(void)
{
	eof_log("Switching Notes Panel to default Notes.", 1);

	if(eof_validate_temp_folder())
	{	//Ensure the correct working directory and presence of the temporary folder
		eof_log("\tCould not validate working directory and temp folder", 1);
		eof_log_cwd();
		eof_enable_notes_panel = 0;

		return 1;
	}

	if(eof_enable_notes_panel)
	{	//If the notes window was already open
		eof_enable_notes_panel = 0;	//Toggle this because the function call below will toggle it back to on
	}
	(void) ustrcpy(eof_current_notes_panel_path, "notes.panel.txt");
	return eof_display_notes_panel_logic(1);	//Load the default notes.panel.txt file, recover panel file from eof.dat if necessary
}

int eof_menu_file_notes_panel_note_controls(void)
{
	eof_log("Switching Notes Panel to Note Controls.", 1);

	if(eof_validate_temp_folder())
	{	//Ensure the correct working directory and presence of the temporary folder
		eof_log("\tCould not validate working directory and temp folder", 1);
		eof_log_cwd();
		eof_enable_notes_panel = 0;

		return 1;
	}

	if(eof_enable_notes_panel)
	{	//If the notes window was already open
		eof_enable_notes_panel = 0;	//Toggle this because the function call below will toggle it back to on
	}
	(void) ustrcpy(eof_current_notes_panel_path, "note_controls.panel.txt");
	return eof_display_notes_panel_logic(1);	//Load the note_controls.panel.txt file, recover panel file from eof.dat if necessary
}

int eof_menu_file_notes_panel_information(void)
{
	eof_log("Switching Notes Panel to Information panel.", 1);

	if(eof_validate_temp_folder())
	{	//Ensure the correct working directory and presence of the temporary folder
		eof_log("\tCould not validate working directory and temp folder", 1);
		eof_log_cwd();
		eof_enable_notes_panel = 0;

		return 1;
	}

	if(eof_enable_notes_panel)
	{	//If the notes window was already open
		eof_enable_notes_panel = 0;	//Toggle this because the function call below will toggle it back to on
	}
	(void) ustrcpy(eof_current_notes_panel_path, "info.panel.txt");
	return eof_display_notes_panel_logic(1);	//Load the info.panel.txt file, recover panel file from eof.dat if necessary
}

int eof_menu_file_notes_panel_note_counts(void)
{
	eof_log("Switching Notes Panel to Note Counts.", 1);

	if(eof_validate_temp_folder())
	{	//Ensure the correct working directory and presence of the temporary folder
		eof_log("\tCould not validate working directory and temp folder", 1);
		eof_log_cwd();
		eof_enable_notes_panel = 0;

		return 1;
	}

	if(eof_enable_notes_panel)
	{	//If the notes window was already open
		eof_enable_notes_panel = 0;	//Toggle this because the function call below will toggle it back to on
	}
	(void) ustrcpy(eof_current_notes_panel_path, "note_counts.panel.txt");
	return eof_display_notes_panel_logic(1);	//Load the note_counts.panel.txt, recover panel file from eof.dat if necessary
}

int eof_menu_file_notes_panel_rocksmith(void)
{
	eof_log("Switching Notes Panel to Rocksmith.", 1);

	if(eof_validate_temp_folder())
	{	//Ensure the correct working directory and presence of the temporary folder
		eof_log("\tCould not validate working directory and temp folder", 1);
		eof_log_cwd();
		eof_enable_notes_panel = 0;

		return 1;
	}

	if(eof_enable_notes_panel)
	{	//If the notes window was already open
		eof_enable_notes_panel = 0;	//Toggle this because the function call below will toggle it back to on
	}
	(void) ustrcpy(eof_current_notes_panel_path, "rocksmith.panel.txt");
	return eof_display_notes_panel_logic(1);	//Load the rocksmith.panel.txt, recover panel file from eof.dat if necessary
}

int eof_menu_file_notes_panel_immerrock(void)
{
	eof_log("Switching Notes Panel to IMMERROCK.", 1);

	if(eof_validate_temp_folder())
	{	//Ensure the correct working directory and presence of the temporary folder
		eof_log("\tCould not validate working directory and temp folder", 1);
		eof_log_cwd();
		eof_enable_notes_panel = 0;

		return 1;
	}

	if(eof_enable_notes_panel)
	{	//If the notes window was already open
		eof_enable_notes_panel = 0;	//Toggle this because the function call below will toggle it back to on
	}
	(void) ustrcpy(eof_current_notes_panel_path, "immerrock.panel.txt");
	return eof_display_notes_panel_logic(1);	//Load the immerrock.panel.txt, recover panel file from eof.dat if necessary
}

int eof_menu_file_notes_panel_clone_hero(void)
{
	eof_log("Switching Notes Panel to Clone Hero.", 1);

	if(eof_validate_temp_folder())
	{	//Ensure the correct working directory and presence of the temporary folder
		eof_log("\tCould not validate working directory and temp folder", 1);
		eof_log_cwd();
		eof_enable_notes_panel = 0;

		return 1;
	}

	if(eof_enable_notes_panel)
	{	//If the notes window was already open
		eof_enable_notes_panel = 0;	//Toggle this because the function call below will toggle it back to on
	}
	(void) ustrcpy(eof_current_notes_panel_path, "clone_hero.panel.txt");
	return eof_display_notes_panel_logic(1);	//Load the clone_hero.panel.txt, recover panel file from eof.dat if necessary
}

int eof_menu_file_notes_panel_user(void)
{
	eof_log("Switching Notes Panel to user-defined file.", 1);

	if((eof_last_browsed_notes_panel_path[0] == '\0') || !exists(eof_last_browsed_notes_panel_path))
		return eof_menu_file_notes_panel_browse();	//If the last browsed panel file path is undefined or is no longer valid, have the user browse for a file

	(void) ustrcpy(eof_current_notes_panel_path, eof_last_browsed_notes_panel_path);	//This is the file that eof_display_notes_panel() will load
	if(eof_enable_notes_panel)
	{	//If the notes window was already open
		eof_enable_notes_panel = 0;	//Toggle this because the function call below will toggle it back to on
	}
	(void) eof_display_notes_panel();
	eof_render();
	eof_close_menu = 1;		//Force the main menu to close, as this function had a tendency to get hung in the menu logic when activated by keyboard

	return D_O_K;
}

int eof_menu_file_notes_panel_browse(void)
{
	char *returnedfn = NULL, initial[1024] = {0};

	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();

	if(!eof_song || !eof_song_loaded)
	{	//If no project is loaded
		return 1;
	}

	eof_log("Browsing for Notes panel file", 1);
	(void) replace_filename(initial, eof_last_browsed_notes_panel_path, "", 1024);	//Get the initial folder path for the browse dialog
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tInitializing browse dialog to \"%s\"", initial);
	eof_log(eof_log_string, 1);

	returnedfn = ncd_file_select(0, initial, "Load Notes panel file", eof_filter_note_panel_files);
	eof_clear_input();
	if(returnedfn)
	{	//If a file was selected for load
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tLoading Notes panel file:  %s", returnedfn);
		eof_log(eof_log_string, 1);
		(void) ustrcpy(eof_current_notes_panel_path, returnedfn);	//This is the file that eof_display_notes_panel() will load
		(void) ustrcpy(eof_last_browsed_notes_panel_path, returnedfn);		//Track this as the last notes panel file the user browsed to
		if(eof_enable_notes_panel)
		{	//If the notes window was already open
			eof_enable_notes_panel = 0;	//Toggle this because the function call below will toggle it back to on
		}
		(void) eof_display_notes_panel();
	}
	eof_render();

	return D_O_K;
}

int eof_menu_file_array_txt_import(void)
{
	char *returnedfn = NULL;
	int retval;
	int prompt1 = 0, prompt2 = 0, prompt3 = 0;

	if(!eof_song)
		return 0;

	eof_log("eof_menu_file_array_txt_import() entered", 1);

	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	returnedfn = ncd_file_select(0, eof_song_path, "Import Queen Bee array.txt", eof_filter_array_txt_files);
	eof_clear_input();
	if(returnedfn)
	{
		eof_log("\tImporting array.txt data", 1);

		retval = eof_import_array_txt(returnedfn, NULL, &prompt1, &prompt2, &prompt3);
		if(retval)
		{
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError %d", retval);
			eof_log(eof_log_string, 1);
			allegro_message("Import failed (error %u).", retval);
		}
		else
		{
			if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
			{	//If the file imported into a drum track
				(void) eof_gh_import_sustained_bass_drum_check(eof_song, eof_selected_track, 0);	//Warn about and highlight any sustained bass drum notes
			}
			eof_log("\tData loaded", 1);
		}
	}
	eof_reset_lyric_preview_lines();
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;

	return 1;
}

int eof_menu_file_multiple_array_txt_import(void)
{
	char *returnedfn = NULL;
	unsigned long count = 0, ctr;
	char foldername[100] = {0};
	char rootfolder[1024] = {0};
	char targetfolder[1024] = {0};
	char undo_made = 0;
	char ghot_import = 0;	//Set to nonzero if contents of multiple folders are targeted for import
	int last_track_type = 0;
	int prompt1 = 0, prompt2 = 0, prompt3 = 0;

	if(!eof_song)
		return 0;

	eof_log("eof_menu_file_multiple_array_txt_import() entered", 1);

	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	returnedfn = ncd_file_select(0, eof_song_path, "Import all Queen Bee array.txt files in the same folder as", eof_filter_array_txt_files);
	eof_clear_input();
	if(returnedfn)
	{
		if(eof_parse_last_folder_name(returnedfn, foldername, sizeof(foldername) / 2))
		{	//If the name of the folder containing the specified file was identified
			for(ctr = 0; ctr < EOF_GHOT_ARRAY_TXT_IMPORT_TRACK_COUNT; ctr++)
			{	//For each of the folder names known to be exported by the Queen Bee variant
				if(!ustricmp(foldername, eof_array_txt_tracks[ctr].name))
				{	//If the user selected file is in a folder name that matches
					eof_clear_input();
					if(alert(NULL, "Perform GHOT import of multiple folders of array.txt files?", NULL, "&Yes", "&No", 'y', 'n') == 1)
					{	//If user opts to import multiple folders of files
						ghot_import = 1;
					}
					if(ghot_import)
					{
						eof_log("\tUser opted to perform GHOT import", 1);
					}
					else
					{
						eof_log("\tUser declined to perform GHOT import", 1);
					}
					break;
				}
			}
		}

		if(!ghot_import)
		{	//If just importing one folder's files
			count += eof_import_array_txt_folder(returnedfn, &undo_made, 0, &prompt1, &prompt2, &prompt3);
		}
		else
		{	//If importing multiple folders' files
			char *ptr;

			//Build the path to folder that would contain these (two folder levels higher than the selected array.txt file)
			(void) replace_filename(rootfolder, returnedfn, "", 1024);
			ptr = ustrrchr(rootfolder, '/');	//Find the last folder separator in the path
			if(!ptr)
				ptr = ustrrchr(rootfolder, '\\');
			if(!ptr)
			{	//If there was no folder separator
				eof_log("\tCould not parse parent folder", 1);
				return 0;
			}
			*ptr = '\0';	//Truncate this from the file path

			ptr = ustrrchr(rootfolder, '/');	//Find the last folder separator in the path
			if(!ptr)
				ptr = ustrrchr(rootfolder, '\\');
			if(!ptr)
			{	//If there was no folder separator
				eof_log("\tCould not parse parent of parent folder", 1);
				return 0;
			}
			(void) replace_filename(rootfolder, rootfolder, "", 1024);	//Strip the folder name from the path
			put_backslash(rootfolder);	//Ensure it ends in a folder separator

			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tGHOT root identified as \"%s\"", rootfolder);
			eof_log(eof_log_string, 1);

			//Build the path to each applicable folder and import all array.txt files from each
			for(ctr = 0; ctr < EOF_GHOT_ARRAY_TXT_IMPORT_TRACK_COUNT; ctr++)
			{	//For each entry in eof_array_txt_tracks[]
				(void) snprintf(targetfolder, sizeof(targetfolder) - 1, "%s%s", rootfolder, eof_array_txt_tracks[ctr].name);	//Build the path to this folder
				put_backslash(targetfolder);
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tImporting contents of \"%s\"", targetfolder);
				eof_log(eof_log_string, 1);
				if(eof_array_txt_tracks[ctr].track_type)
				{	//If this folder is specific to an instrument track
					eof_set_active_difficulty(eof_array_txt_tracks[ctr].difficulty);	//Change to the applicable track difficulty as well
					(void) eof_menu_track_selected_track_number(eof_array_txt_tracks[ctr].track_type, 1);	//Change to the applicable track and update the tile bar
					eof_render();	//Redraw the screen so the user can see which track prompts (ie. for HOPO threshold selection) are for

					if(eof_array_txt_tracks[ctr].track_type != last_track_type)
					{	//If this is the first track-specific content being imported, or it's for a different track than the previous folder of files
						prompt1 = prompt2 = prompt3 = 0;	//Reset these statuses so that the user is prompted for how this track's contents are to import
					}
					last_track_type = eof_array_txt_tracks[ctr].track_type;
				}
				count += eof_import_array_txt_folder(targetfolder, &undo_made, 1, &prompt1, &prompt2, &prompt3);
			}
		}

		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tImported %lu files", count);
		eof_log(eof_log_string, 1);

		if(!count)
			allegro_message("No files were imported.");
	}
	eof_reset_lyric_preview_lines();
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;

	return 1;
}

int eof_menu_file_gh3_section_import(void)
{
	filebuffer *fb;
	char * sectionfn;

	eof_log("eof_menu_file_gh3_section_import() entered", 1);

	if(!eof_song)
		return 1;

	//Prompt user to browse for file, and buffer it to memory
	sectionfn = ncd_file_select(0, eof_last_eof_path, "Import GH3 section name file", eof_filter_gh_files);
	if(!sectionfn)
		return 1;
	fb = eof_filebuffer_load(sectionfn);
	if(fb == NULL)
	{	//Section names file failed to buffer
		allegro_message("Error:  Failed to buffer section name file");
	}

	//Import sections
	eof_gh_import_gh3_style_sections = 0;	//Reset this status
	(void) eof_gh_read_sections_qb(fb, eof_song, 1);

	//Cleanup
	eof_filebuffer_close(fb);	//Close that file buffer
	return 1;
}

/* velocity editing arrays */
char eof_drum_lane_1_velocity[5] = {0};	//Use a fifth byte to guarantee proper truncation
char eof_drum_lane_2_velocity[5] = {0};
char eof_drum_lane_3_velocity[5] = {0};
char eof_drum_lane_4_velocity[5] = {0};
char eof_drum_lane_5_velocity[5] = {0};
char eof_drum_lane_6_velocity[5] = {0};
char *eof_drum_velocity_strings[6] = {eof_drum_lane_1_velocity, eof_drum_lane_2_velocity, eof_drum_lane_3_velocity, eof_drum_lane_4_velocity, eof_drum_lane_5_velocity, eof_drum_lane_6_velocity};
unsigned char eof_drum_velocities[6] = {100, 100, 100, 100, 100, 100};	//The velocity written for each normal gem, for Phase Shift
char eof_drum_lane_1_ghost_velocity[5] = {0};	//Use a fifth byte to guarantee proper truncation
char eof_drum_lane_2_ghost_velocity[5] = {0};
char eof_drum_lane_3_ghost_velocity[5] = {0};
char eof_drum_lane_4_ghost_velocity[5] = {0};
char eof_drum_lane_5_ghost_velocity[5] = {0};
char eof_drum_lane_6_ghost_velocity[5] = {0};
char *eof_drum_ghost_velocity_strings[6] = {eof_drum_lane_1_ghost_velocity, eof_drum_lane_2_ghost_velocity, eof_drum_lane_3_ghost_velocity, eof_drum_lane_4_ghost_velocity, eof_drum_lane_5_ghost_velocity, eof_drum_lane_6_ghost_velocity};
unsigned char eof_drum_ghost_velocities[6] = {1, 1, 1, 1, 1, 1};	//The velocity written for each ghost gem, for Phase Shift
char eof_drum_lane_1_accent_velocity[5] = {0};	//Use a fifth byte to guarantee proper truncation
char eof_drum_lane_2_accent_velocity[5] = {0};
char eof_drum_lane_3_accent_velocity[5] = {0};
char eof_drum_lane_4_accent_velocity[5] = {0};
char eof_drum_lane_5_accent_velocity[5] = {0};
char eof_drum_lane_6_accent_velocity[5] = {0};
char *eof_drum_accent_velocity_strings[6] = {eof_drum_lane_1_accent_velocity, eof_drum_lane_2_accent_velocity, eof_drum_lane_3_accent_velocity, eof_drum_lane_4_accent_velocity, eof_drum_lane_5_accent_velocity, eof_drum_lane_6_accent_velocity};
unsigned char eof_drum_accent_velocities[6] = {127, 127, 127, 127, 127, 127};	//The velocity written for each accent gem, for Phase Shift

DIALOG eof_menu_file_drum_midi_velocities_dialog[] =
{
	/*	(proc)				(x)  (y)  (w)  (h) (fg) (bg) (key) (flags) (d1)       (d2) (dp)          (dp2)          (dp3) */
	{ eof_window_proc,    0,   48,  240, 258,2,   23,  0,    0,      0,         0,   "Edit drum MIDI velocities",NULL, NULL },

	{ d_agup_text_proc,      63,  80,  64,  8,  2,   23,  0,    0,      0,         0,   "Normal",     NULL,          NULL },
	{ d_agup_text_proc,      16,  108, 64,  8,  2,   23,  0,    0,      0,         0,   "Lane 1", NULL, NULL },
	{ eof_verified_edit_proc,70,  104, 28,  20, 2,   23,  0,    0,      3,         0,   eof_drum_lane_1_velocity, "0123456789",NULL },
	{ d_agup_text_proc,      16,  132, 64,  8,  2,   23,  0,    0,      0,         0,   "Lane 2", NULL, NULL },
	{ eof_verified_edit_proc,70,  128, 28,  20, 2,   23,  0,    0,      3,         0,   eof_drum_lane_2_velocity, "0123456789",NULL },
	{ d_agup_text_proc,      16,  156, 64,  8,  2,   23,  0,    0,      0,         0,   "Lane 3", NULL, NULL },
	{ eof_verified_edit_proc,70,  152, 28,  20, 2,   23,  0,    0,      3,         0,   eof_drum_lane_3_velocity, "0123456789",NULL },
	{ d_agup_text_proc,      16,  180, 64,  8,  2,   23,  0,    0,      0,         0,   "Lane 4", NULL, NULL },
	{ eof_verified_edit_proc,70,  176, 28,  20, 2,   23,  0,    0,      3,         0,   eof_drum_lane_4_velocity, "0123456789",NULL },
	{ d_agup_text_proc,      16,  204, 64,  8,  2,   23,  0,    0,      0,         0,   "Lane 5", NULL, NULL },
	{ eof_verified_edit_proc,70,  200, 28,  20, 2,   23,  0,    0,      3,         0,   eof_drum_lane_5_velocity, "0123456789",NULL },
	{ d_agup_text_proc,      16,  228, 64,  8,  2,   23,  0,    0,      0,         0,   "Lane 6", NULL, NULL },
	{ eof_verified_edit_proc,70,  224, 28,  20, 2,   23,  0,    0,      3,         0,   eof_drum_lane_6_velocity, "0123456789",NULL },

	{ d_agup_text_proc,      127, 80,  64,  8,  2,   23,  0,    0,      0,         0,   "Ghost",     NULL,          NULL },
	{ eof_verified_edit_proc,130, 104, 28,  20, 2,   23,  0,    0,      3,         0,   eof_drum_lane_1_ghost_velocity, "0123456789",NULL },
	{ eof_verified_edit_proc,130, 128, 28,  20, 2,   23,  0,    0,      3,         0,   eof_drum_lane_2_ghost_velocity, "0123456789",NULL },
	{ eof_verified_edit_proc,130, 152, 28,  20, 2,   23,  0,    0,      3,         0,   eof_drum_lane_3_ghost_velocity, "0123456789",NULL },
	{ eof_verified_edit_proc,130, 176, 28,  20, 2,   23,  0,    0,      3,         0,   eof_drum_lane_4_ghost_velocity, "0123456789",NULL },
	{ eof_verified_edit_proc,130, 200, 28,  20, 2,   23,  0,    0,      3,         0,   eof_drum_lane_5_ghost_velocity, "0123456789",NULL },
	{ eof_verified_edit_proc,130, 224, 28,  20, 2,   23,  0,    0,      3,         0,   eof_drum_lane_6_ghost_velocity, "0123456789",NULL },

	{ d_agup_text_proc,      185, 80,  64,  8,  2,   23,  0,    0,      0,         0,   "Accent",     NULL,          NULL },
	{ eof_verified_edit_proc,190, 104, 28,  20, 2,   23,  0,    0,      3,         0,   eof_drum_lane_1_accent_velocity, "0123456789",NULL },
	{ eof_verified_edit_proc,190, 128, 28,  20, 2,   23,  0,    0,      3,         0,   eof_drum_lane_2_accent_velocity, "0123456789",NULL },
	{ eof_verified_edit_proc,190, 152, 28,  20, 2,   23,  0,    0,      3,         0,   eof_drum_lane_3_accent_velocity, "0123456789",NULL },
	{ eof_verified_edit_proc,190, 176, 28,  20, 2,   23,  0,    0,      3,         0,   eof_drum_lane_4_accent_velocity, "0123456789",NULL },
	{ eof_verified_edit_proc,190, 200, 28,  20, 2,   23,  0,    0,      3,         0,   eof_drum_lane_5_accent_velocity, "0123456789",NULL },
	{ eof_verified_edit_proc,190, 224, 28,  20, 2,   23,  0,    0,      3,         0,   eof_drum_lane_6_accent_velocity, "0123456789",NULL },

	{ d_agup_button_proc,    50,   260, 35,  28, 2,   23,  '\r',  D_EXIT, 0,       0,   "OK",         NULL,          NULL },
	{ d_agup_button_proc,    135, 260, 55,  28, 2,   23,  0,   D_EXIT, 0,       0,   "Cancel",     NULL,          NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

int eof_menu_file_drum_midi_velocities(void)
{
	unsigned long ctr;
	int value;

	if(eof_song_loaded)
	{
		if(!eof_music_paused)
		{
			eof_music_play(0);
		}
	}
	eof_cursor_visible = 0;
	eof_pen_visible = 0;

	//Update dialog
	for(ctr = 0; ctr < 6; ctr++)
	{	//For each of the 6 lanes
		snprintf(eof_drum_velocity_strings[ctr], sizeof(eof_drum_velocity_strings[0]) - 1, "%u", eof_drum_velocities[ctr]);	//Populate this lane's normal velocity
		snprintf(eof_drum_ghost_velocity_strings[ctr], sizeof(eof_drum_ghost_velocity_strings[0]) - 1, "%u", eof_drum_ghost_velocities[ctr]);	//Populate this lane's ghost velocity
		snprintf(eof_drum_accent_velocity_strings[ctr], sizeof(eof_drum_accent_velocity_strings[0]) - 1, "%u", eof_drum_accent_velocities[ctr]);	//Populate this lane's accent velocity
	}

	//Run dialog
	eof_render();
	eof_color_dialog(eof_menu_file_drum_midi_velocities_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_menu_file_drum_midi_velocities_dialog);
	if(eof_popup_dialog(eof_menu_file_drum_midi_velocities_dialog, 0) == 28)
	{	//User clicked OK
		for(ctr = 0; ctr < 6; ctr++)
		{	//For each of the 6 lanes
			value = atoi(eof_drum_velocity_strings[ctr]);
			eof_drum_velocities[ctr] = value ? (value > 127 ? 127 : value) : 1;			//Force the velocities to be within the range of 1 through 127
			value = atoi(eof_drum_ghost_velocity_strings[ctr]);
			eof_drum_ghost_velocities[ctr] = value ? (value > 127 ? 127 : value) : 1;	//Force the velocities to be within the range of 1 through 127
			value = atoi(eof_drum_accent_velocity_strings[ctr]);
			eof_drum_accent_velocities[ctr] = value ? (value > 127 ? 127 : value) : 1;	//Force the velocities to be within the range of 1 through 127
		}
	}
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	return 1;
}
