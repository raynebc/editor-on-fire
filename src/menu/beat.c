#include <allegro.h>
#include <float.h>
#include "../agup/agup.h"
#include "../main.h"
#include "../dialog.h"
#include "../beat.h"
#include "../mix.h"
#include "../event.h"
#include "../utility.h"
#include "../undo.h"
#include "../midi.h"
#include "../dialog/proc.h"
#include "../midi_data_import.h"	//For eof_events_overridden_by_stored_MIDI_track()
#include "../rs.h"
#include "../bpm.h"	//For eof_estimate_bpm()
#include "../song.h"
#include "beat.h"
#include "edit.h"
#include "song.h"
#include "main.h"

#ifdef USEMEMWATCH
#include "../memwatch.h"
#endif

char eof_ts_menu_off_text[32] = {0};
char eof_ks_menu_off_text[32] = {0};

MENU eof_beat_time_signature_menu[] =
{
	{"&4/4", eof_menu_beat_ts_4_4, NULL, D_USER, NULL},
	{"&2/4", eof_menu_beat_ts_2_4, NULL, D_USER, NULL},
	{"&3/4", eof_menu_beat_ts_3_4, NULL, D_USER, NULL},
	{"&5/4", eof_menu_beat_ts_5_4, NULL, D_USER, NULL},
	{"&6/4", eof_menu_beat_ts_6_4, NULL, D_USER, NULL},
	{"&Custom\tShift+I", eof_menu_beat_ts_custom, NULL, D_USER, NULL},
	{eof_ts_menu_off_text, eof_menu_beat_ts_off, NULL, D_USER, NULL},
	{"Clear all", eof_menu_beat_remove_ts, NULL, 0, NULL},
	{"Con&Vert", eof_menu_beat_ts_convert, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_beat_key_signature_menu[] =
{
	{eof_ks_menu_off_text, eof_menu_beat_ks_off, NULL, D_USER, NULL},
	{"B , g# (7 flats)", eof_menu_beat_ks_7_flats, NULL, D_USER, NULL},
	{"Gb , eb (6 flats)", eof_menu_beat_ks_6_flats, NULL, D_USER, NULL},
	{"Db , bb (5 flats)", eof_menu_beat_ks_5_flats, NULL, D_USER, NULL},
	{"Ab , f (4 flats)", eof_menu_beat_ks_4_flats, NULL, D_USER, NULL},
	{"Eb , c (3 flats)", eof_menu_beat_ks_3_flats, NULL, D_USER, NULL},
	{"Bb , g (2 flats)", eof_menu_beat_ks_2_flats, NULL, D_USER, NULL},
	{"F , d (1 flat)", eof_menu_beat_ks_1_flat, NULL, D_USER, NULL},
	{"C , a (0 flats/sharps)", eof_menu_beat_ks_0_flats, NULL, D_USER, NULL},
	{"G , e (1 sharp)", eof_menu_beat_ks_1_sharp, NULL, D_USER, NULL},
	{"D , b (2 sharps)", eof_menu_beat_ks_2_sharps, NULL, D_USER, NULL},
	{"A , f# (3 sharps)", eof_menu_beat_ks_3_sharps, NULL, D_USER, NULL},
	{"E , c# (4 sharps)", eof_menu_beat_ks_4_sharps, NULL, D_USER, NULL},
	{"B , g# (5 sharps)", eof_menu_beat_ks_5_sharps, NULL, D_USER, NULL},
	{"Gb , eb (6 sharps)", eof_menu_beat_ks_6_sharps, NULL, D_USER, NULL},
	{"Db , bb (7 sharps)", eof_menu_beat_ks_7_sharps, NULL, D_USER, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_beat_rocksmith_menu[] =
{
	{"Place RS &Phrase\tShift+P", eof_rocksmith_phrase_dialog_add, NULL, 0, NULL},
	{"Place RS &Section\tShift+S", eof_rocksmith_section_dialog_add, NULL, 0, NULL},
	{"Place RS &Event\tShift+E", eof_rocksmith_event_dialog_add, NULL, 0, NULL},
	{"&Copy phrase/section", eof_menu_beat_copy_rs_events, NULL, 0, NULL},
	{"Clear non RS events", eof_menu_beat_clear_non_rs_events, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_beat_double_bpm_menu[] =
{
	{"&Selected\t" CTRL_NAME "+Shift+D", eof_menu_beat_double_tempo, NULL, 0, NULL},
	{"&All", eof_menu_beat_double_tempo_all, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_beat_halve_bpm_menu[] =
{
	{"&Selected\t" CTRL_NAME "+Shift+X", eof_menu_beat_halve_tempo, NULL, 0, NULL},
	{"&All", eof_menu_beat_halve_tempo_all, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

char *eof_beat_events_menu_place_section_text_non_pg = "Place &Section\tShift+E";
char *eof_beat_events_menu_place_section_text_pg = "Place &Section";
MENU eof_beat_events_menu[] =
{
	{"All E&vents", eof_menu_beat_all_events, NULL, 0, NULL},
	{"&Events", eof_menu_beat_events, NULL, 0, NULL},
	{"Clear all events", eof_menu_beat_clear_events, NULL, 0, NULL},
	{"Place &Section", eof_menu_beat_add_section, NULL, 0, NULL},
	{"&Copy events\t" CTRL_NAME "+Shift+C", eof_menu_beat_copy_events, NULL, 0, NULL},
	{"&Paste events\t" CTRL_NAME "+Shift+V", eof_menu_beat_paste_events, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_beat_bpm_menu[] =
{
	{"&BPM Change", eof_menu_beat_bpm_change, NULL, 0, NULL},
	{"&Reset BPM", eof_menu_beat_reset_bpm, NULL, 0, NULL},
	{"&Calculate BPM", eof_menu_beat_calculate_bpm, NULL, 0, NULL},
	{"&Estimate BPM", eof_menu_beat_estimate_bpm, NULL, 0, NULL},
	{"&Fix tempo for RBN", eof_menu_beat_set_RBN_tempos, NULL, 0, NULL},
	{"&Double BPM", NULL, eof_beat_double_bpm_menu, 0, NULL},
	{"&Halve BPM", NULL, eof_beat_halve_bpm_menu, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_beat_menu[] =
{
	{"&BPM", NULL, eof_beat_bpm_menu, 0, NULL},
	{"Time &Signature", NULL, eof_beat_time_signature_menu, 0, NULL},
	{"&Key Signature", NULL, eof_beat_key_signature_menu, 0, NULL},
	{"", NULL, NULL, 0, NULL},
	{"&Add\t" CTRL_NAME "+Ins", eof_menu_beat_add, NULL, 0, NULL},
	{"Delete\t" CTRL_NAME "+Del", eof_menu_beat_delete, NULL, 0, NULL},
	{"Push Offset Back", eof_menu_beat_push_offset_back_menu, NULL, 0, NULL},
	{"Push Offset Up", eof_menu_beat_push_offset_up, NULL, 0, NULL},
	{"Reset Offset to Zero", eof_menu_beat_reset_offset, NULL, 0, NULL},
	{"Anchor Beat\tShift+A", eof_menu_beat_anchor, NULL, 0, NULL},
	{"Toggle Anchor\tA", eof_menu_beat_toggle_anchor, NULL, 0, NULL},
	{"De&Lete Anchor", eof_menu_beat_delete_anchor, NULL, 0, NULL},
	{"Anchor measures", eof_menu_beat_anchor_measures, NULL, 0, NULL},
	{"&Copy tempo map", eof_menu_beat_copy_tempo_map, NULL, 0, NULL},
	{"&Paste tempo map", eof_menu_beat_paste_tempo_map, NULL, 0, NULL},
	{"&Validate tempo map", eof_menu_beat_validate_tempo_map, NULL, 0, NULL},
	{"Lock tempo map", eof_menu_beat_lock_tempo_map, NULL, 0, NULL},
	{"Remove mid-beat status", eof_menu_beat_remove_mid_beat_status, NULL, 0, NULL},
	{"&Move to seek pos", eof_menu_beat_move_to_seek_pos, NULL, 0, NULL},
	{"Export beat timings", eof_menu_beat_export_beat_timings, NULL, 0, NULL},
	{"", NULL, NULL, 0, NULL},
	{"&Events", NULL, eof_beat_events_menu, 0, NULL},
	{"&Rocksmith", NULL, eof_beat_rocksmith_menu, 0, NULL},
	{"Place &Trainer event", eof_menu_beat_trainer_event, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

MENU eof_filtered_beat_menu[EOF_SCRATCH_MENU_SIZE];	//This will be built by eof_create_filtered_menu() to contain the contents of eof_song_menu[] minus the hidden items
MENU *eof_effective_beat_menu = eof_beat_menu;	//This is invoked by right clicking in the beat marker area in Feedback or Rex Mundi input modes

char stored_event_track_notice[] = "Warning:  A stored MIDI track will override chart-wide text events";
char no_notice[] = "";
char eof_events_dialog_string[15] = {0};	//The title string for the Events dialog
DIALOG eof_events_dialog[] =
{
	/* (proc)            (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)            (dp2) (dp3) */
	{ eof_window_proc,0,   48,  500, 237, 2,   23,  0,    0,      0,   0,   eof_events_dialog_string,NULL, NULL },
	{ d_agup_list_proc,  12,  84,  400, 138, 2,   23,  0,    0,      0,   0,   (void *)eof_events_list,NULL, NULL },
	{ d_agup_push_proc,  416, 84,  76,  28,  2,   23,  'a',  D_EXIT, 0,   0,   "&Add",         NULL, (void *)eof_events_dialog_add },
	{ d_agup_push_proc,  416, 124, 76,  28,  2,   23,  'e',  D_EXIT, 0,   0,   "&Edit",        NULL, (void *)eof_events_dialog_edit },
	{ d_agup_push_proc,  416, 164, 76,  28,  2,   23,  'l',  D_EXIT, 0,   0,   "De&lete",      NULL, (void *)eof_events_dialog_delete },
	{ d_agup_text_proc,  12,  225, 64,  8,   2,   23,  0,    0,      0,   0,   ""      ,       NULL, NULL },
	{ d_agup_button_proc,12,  245, 240, 28,  2,   23,  '\r', D_EXIT, 0,   0,   "Done",         NULL, NULL },
	{ d_agup_push_proc,  416, 204, 76,  28,  2,   23,  'u',  D_EXIT, 0,   0,   "Move &Up",     NULL, (void *)eof_events_dialog_move_up },
	{ d_agup_push_proc,  416, 244, 76,  28,  2,   23,  'd',  D_EXIT, 0,   0,   "Move &Down",   NULL, (void *)eof_events_dialog_move_down },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

///The dp2 values below must be edited appropriately whenever the radio buttons' object numbers in the dialog are changed
char eof_all_events_dialog_string[20] = {0};	//The title string for the All Events dialog
DIALOG eof_all_events_dialog[] =
{
	/* (proc)                    (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                     (dp2) (dp3) */
	{ eof_window_proc,         0,   48,  500, 266, 2,   23,  0,    0,      0,   0,   eof_all_events_dialog_string, NULL, NULL },
	{ d_agup_list_proc,           12,  84,  475, 140, 2,   23,  0,    0,      0,   0,   (void *)eof_events_list_all,  NULL, NULL },
	{ d_agup_button_proc,         12,  275, 70,  28,  2,   23,  'f',  D_EXIT, 0,   0,   "&Find",                NULL, NULL },
	{ d_agup_button_proc,         95,  275, 70,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "Done",                 NULL, NULL },
	{ d_agup_button_proc,         182, 275, 150, 28,  2,   23,  0,    D_EXIT, 0,   0,   "Copy to selected beat",NULL, NULL },
	{ eof_all_events_check_proc,  182, 259, 142, 15,  2,   23,  0,    0,      0,   0,   "This Track's Events",  (void *)5,    NULL },
	{ eof_all_events_radio_proc,  340, 243, 85,  15,  2,   23,  0, D_SELECTED,0,   0,   "All Events",           (void *)6,    NULL },	//Use dp2 to store the object number, for use in eof_all_events_radio_proc()
	{ eof_all_events_radio_proc,  340, 259, 152, 15,  2,   23,  0,    0,      0,   0,   "Sections (RS phrases)",(void *)7,    NULL },
	{ eof_all_events_radio_proc,  340, 275, 152, 15,  2,   23,  0,    0,      0,   0,   "RS sections",          (void *)8,    NULL },
	{ eof_all_events_radio_proc,  340, 291, 152, 15,  2,   23,  0,    0,      0,   0,   "RS events",            (void *)9,    NULL },
	{ d_agup_text_proc,           12,  228, 64,  8,   2,   23,  0,    0,      0,   0,   ""      ,               NULL, NULL },	//Used to display "A stored MIDI track will override chart-wide text events" warning if applicable
	{ d_agup_button_proc,         12,  243, 70,  28,  2,   23,  'e',  D_EXIT, 0,   0,   "&Edit",                NULL, NULL },
	{ d_agup_button_proc,         95,  243, 70,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Delete",               NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

char eof_events_add_dialog_string[100] = {0};
char eof_events_add_dialog_string1[] = "Add event";
char eof_events_add_dialog_string2[] = "Edit event";
DIALOG eof_events_add_dialog[] =
{
	/* (proc)                    (x) (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)           (dp2) (dp3) */
	{ eof_window_proc,        0,  48,  314, 206, 2,   23,  0,    0,      0,   0,   eof_events_add_dialog_string1,  NULL, NULL },
	{ d_agup_text_proc,          12, 84,  64,  8,   2,   23,  0,    0,      0,   0,   "Text:",       NULL, NULL },
	{ eof_edit_proc,          48, 80,  254, 20,  2,   23,  0,    0,      255, 0,   eof_etext,     NULL, NULL },
	{ d_agup_check_proc,         12, 110, 250, 16,  0,   0,   0,    0,      1,   0,   eof_events_add_dialog_string, NULL, NULL },
	{ eof_events_add_check_proc, 12, 130, 174, 16,  0,   0,   0,    0,      1,   0,   "Rocksmith phrase marker", NULL, NULL },
	{ d_agup_check_proc,         12, 150, 166, 16,  0,   0,   0,    0,      1,   0,   "Rocksmith solo phrase", NULL, NULL },
	{ d_agup_check_proc,         12, 170, 182, 16,  0,   0,   0,    0,      1,   0,   "Rocksmith section marker", NULL, NULL },
	{ d_agup_check_proc,         12, 190, 182, 16,  0,   0,   0,    0,      1,   0,   "Rocksmith event marker", NULL, NULL },
	{ d_agup_button_proc,        67, 214, 84,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",          NULL, NULL },
	{ d_agup_button_proc,        163,214, 78,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",      NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

char eof_events_add_floating_dialog_string1[] = "Add floating event";
char eof_events_add_floating_dialog_string2[] = "Edit floating event";
DIALOG eof_floating_events_add_dialog[] =
{
	/* (proc)                    (x) (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)           (dp2) (dp3) */
	{ eof_window_proc,        0,  48,  314, 186, 2,   23,  0,    0,      0,   0,   eof_events_add_floating_dialog_string1,  NULL, NULL },
	{ d_agup_text_proc,          12, 84,  64,  8,   2,   23,  0,    0,      0,   0,   "Text:",       NULL, NULL },
	{ eof_edit_proc,          48, 80,  254, 20,  2,   23,  0,    0,      255, 0,   eof_etext,     NULL, NULL },
	{ d_agup_text_proc,          12, 110, 60,  12,  0,   0,   0,    0,      0,   0,   "Position (ms)",NULL, NULL },
	{ eof_verified_edit_proc,    12, 130, 50,  20,  0,   0,   0,    0,      7,   0,   eof_etext2,     "0123456789", NULL },
	{ d_agup_check_proc,         12, 160, 250, 16,  0,   0,   0,    0,      1,   0,   eof_events_add_dialog_string, NULL, NULL },
	{ d_agup_button_proc,        67, 194, 84,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",          NULL, NULL },
	{ d_agup_button_proc,        163,194, 78,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",      NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

char eof_section_add_dialog_string1[] = "Add section";
char eof_section_add_dialog_string2[] = "Edit section";
DIALOG eof_section_add_dialog[] =
{
	/* (proc)            (x) (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)           (dp2) (dp3) */
	{ eof_window_proc,0,  48,  314, 100, 2,   23,  0,    0,      0,   0,   eof_section_add_dialog_string1,  NULL, NULL },
	{ d_agup_text_proc,  12, 84,  64,  8,   2,   23,  0,    0,      0,   0,   "Text:",       NULL, NULL },
	{ eof_edit_proc,  48, 80,  254, 20,  2,   23,  0,    0,      255, 0,   eof_etext,     NULL, NULL },
	{ d_agup_button_proc,67, 108, 84,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",          NULL, NULL },
	{ d_agup_button_proc,163,108, 78,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",      NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_bpm_change_dialog[] =
{
	/* (proc)                (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)             (dp2) (dp3) */
	{ eof_shadow_box_proc,32,  68,  170, 124, 2,   23,  0,    0,      0,   0,   NULL,            NULL, NULL },
	{ d_agup_text_proc,      56,  84,  64,  8,   2,   23,  0,    0,      0,   0,   "BPM:",          NULL, NULL },
	{ eof_verified_edit_proc,112, 80,  66,  20,  2,   23,  0,    0,      25,  0,   eof_etext,       "1234567890.", NULL },
	{ d_agup_check_proc,     56,  108, 128, 16,  2,   23,  0,    0,      1,   0,   "This Beat Only",NULL, NULL },
	{ d_agup_check_proc,     56,  128, 128, 16,  2,   23,  0,    0,      1,   0,   "Adjust Notes",  NULL, NULL },
	{ d_agup_button_proc,    42,  152, 68,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",            NULL, NULL },
	{ d_agup_button_proc,    120, 152, 68,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",        NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_anchor_dialog[] =
{
	/* (proc)                (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)         (dp2) (dp3) */
	{ eof_shadow_box_proc,32,  68,  170, 100,  2,   23,  0,    0,      0,   0,   NULL,        NULL, NULL },
	{ d_agup_text_proc,      56,  84,  64,  8,   2,   23,  0,    0,      0,   0,   "Position:", NULL, NULL },
	{ eof_verified_edit_proc,112, 80,  66,  20,  2,   23,  0,    0,      9,   0,   eof_etext2,  "0123456789:", NULL },
	{ d_agup_check_proc,     56,  108, 128, 16,  2,   23,  0,    0,      1,   0,   "Adjust Notes", NULL, NULL },
	{ d_agup_button_proc,    42,  128, 68,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",        NULL, NULL },
	{ d_agup_button_proc,    120, 128, 68,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",    NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

char eof_trainer_string[5] = "";
DIALOG eof_place_trainer_dialog[] =
{
	/* (proc)                (x) (y) (w)  (h)   (fg) (bg) (key) (flags)    (d1) (d2) (dp)                  (dp2) (dp3) */
	{ eof_window_proc,    0,  20, 260, 160,  2,   23,  0,    0,         0,   0,   "Place Trainer Event",NULL, NULL },
	{ d_agup_text_proc,      12, 56, 64,  8,    2,   23,  0,    0,         0,   0,   "Trainer #:",         NULL, NULL },
	{ eof_edit_trainer_proc, 80, 52, 40,  20,   2,   23,  0,    0,         3,   0,   eof_trainer_string,   "1234567890", NULL },
	{ d_agup_check_proc,     12, 78, 12,  16,   2,   23,  0,    D_DISABLED,0,   0,   "",                   NULL, NULL },
	{ d_agup_radio_proc,     34, 78, 220, 16,   2,   23,  0,    0,         1,   0,   eof_etext2,           NULL, NULL },
	{ d_agup_check_proc,     12, 98, 12,  16,   2,   23,  0,    D_DISABLED,0,   0,   "",                   NULL, NULL },
	{ d_agup_radio_proc,     34, 98, 220, 16,   2,   23,  0,    0,         1,   0,   eof_etext3,           NULL, NULL },
	{ d_agup_check_proc,     12, 118,12,  16,   2,   23,  0,    D_DISABLED,0,   0,   "",                   NULL, NULL },
	{ d_agup_radio_proc,     34, 118,220, 16,   2,   23,  0,    0,         1,   0,   eof_etext4,           NULL, NULL },
	{ d_agup_button_proc,    10, 144, 68, 28,   2,   23,  '\r', D_EXIT,    0,   0,   "OK",                 NULL, NULL },
	{ d_agup_button_proc,    88, 144, 68, 28,   2,   23,  0,    D_EXIT,    0,   0,   "Cancel",             NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

char eof_rocksmith_section_dialog_string1[] = "Add Rocksmith section";
char eof_rocksmith_section_dialog_string2[] = "Edit Rocksmith section";
DIALOG eof_rocksmith_section_dialog[] =
{
	/* (proc)             (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                     (dp2) (dp3) */
	{ eof_window_proc, 0,   0,   290, 450, 2,   23,  0,    0,      0,   0,   eof_rocksmith_section_dialog_string1, NULL, NULL },
	{ d_agup_list_proc,   12,  35,  260, 320, 2,   23,  0,    0,      0,   0,   (void *)eof_rs_section_add_list, NULL, NULL },
	{ d_agup_check_proc,  12,  364, 164, 16,  0,   0,   0,    0,      1,   0,   "Also add as RS phrase",  NULL, NULL },
	{ d_agup_check_proc,  12,  384, 250, 16,  0,   0,   0,    0,      1,   0,   eof_events_add_dialog_string,  NULL, NULL },
	{ d_agup_button_proc, 12,  410, 68,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",                    NULL, NULL },
	{ d_agup_button_proc, 206, 410, 68,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",                NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_rocksmith_event_dialog[] =
{
	/* (proc)             (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                     (dp2) (dp3) */
	{ eof_window_proc, 0,   0,   286, 235, 2,   23,  0,    0,      0,   0,   "Add Rocksmith event", NULL, NULL },
	{ d_agup_list_proc,   12,  35,  260, 125, 2,   23,  0,    0,      0,   0,   (void *)eof_rs_event_add_list, NULL, NULL },
	{ d_agup_check_proc,  12,  167, 250, 16,  0,   0,   0,    0,      1,   0,   eof_events_add_dialog_string,  NULL, NULL },
	{ d_agup_button_proc, 12,  195, 68,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",                    NULL, NULL },
	{ d_agup_button_proc, 206, 195, 68,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",                NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

int eof_export_beat_timings_edit_proc(int msg, DIALOG *d, int c);
DIALOG eof_export_beat_timings_dialog[]=
{
	/* (proc)                (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags)  (d1) (d2) (dp)         (dp2) (dp3) */
	{ eof_shadow_box_proc,32,  68,  174, 95,  2,    23,  0,    0,      0,   0,   NULL,        NULL, NULL },
	{ d_agup_text_proc,		 56,  84,  64,  8,   2,    23,  0,    0,      0,   0,   "Intervals:",NULL, NULL },
	{ eof_export_beat_timings_edit_proc,112, 80,  66,  20,  2,   23,   0,      0,   2,   0,    eof_etext2,  "0123456789", NULL },
	{ d_agup_radio_proc,     42,  105, 68,  15,  2,    23,  0,    0,      0,   0,   "&Millis",      NULL, NULL },
	{ d_agup_radio_proc,     120, 105, 84,  15,  2,    23,  0,    D_SELECTED,      0,   0,   "&Int/sec",   NULL, NULL },
	{ d_agup_button_proc,    42,  125, 68,  28,  2,    23,  '\r', D_EXIT, 0,   0,   "OK",        NULL, NULL },
	{ d_agup_button_proc,    120, 125, 68,  28,  2,    23,  0,    D_EXIT, 0,   0,   "Cancel",    NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

void eof_prepare_beat_menu(void)
{
	unsigned long i;

	if(eof_song && eof_song_loaded)
	{	//If a song is loaded
		//Several beat menu items are disabled below if the tempo map is locked.  Clear those items' flags in case the lock was removed
		eof_beat_menu[4].flags = 0;		//Add
		eof_beat_menu[5].flags = 0;		//Delete
		eof_beat_menu[6].flags = 0;		//Push offset back
		eof_beat_menu[7].flags = 0;		//Push offset up
		eof_beat_menu[8].flags = 0;		//Reset offset to zero
		eof_beat_menu[9].flags = 0;		//Anchor
		eof_beat_menu[10].flags = 0;	//Toggle anchor
		eof_beat_menu[11].flags = 0;	//Delete anchor
		eof_beat_menu[12].flags = 0;	//Anchor measures
		eof_beat_menu[14].flags = 0;	//Paste tempo map
		eof_beat_menu[18].flags = 0;	//Move to seek pos

		eof_beat_bpm_menu[0].flags = 0;	//BPM>BPM change
		eof_beat_bpm_menu[1].flags = 0;	//BPM>Reset BPM
		eof_beat_bpm_menu[2].flags = 0;	//BPM>Calculate BPM
		eof_beat_bpm_menu[3].flags = 0;	//BPM>Estimate BPM
		eof_beat_bpm_menu[5].flags = 0;	//BPM>Double BPM
		eof_beat_bpm_menu[6].flags = 0;	//BPM>Halve BPM
		eof_beat_bpm_menu[4].flags = 0;	//BPM>Fix tempo for RBN

		//Ditto for time signature sub menu items
		eof_beat_time_signature_menu[0].flags = 0;	//4/4
		eof_beat_time_signature_menu[1].flags = 0;	//2/4
		eof_beat_time_signature_menu[2].flags = 0;	//3/4
		eof_beat_time_signature_menu[3].flags = 0;	//5/4
		eof_beat_time_signature_menu[4].flags = 0;	//6/4
		eof_beat_time_signature_menu[5].flags = 0;	//Custom
		eof_beat_time_signature_menu[6].flags = 0;	//Off
		eof_beat_time_signature_menu[7].flags = 0;	//Clear all

//Beat>Add and Delete validation
		if(!eof_beat_num_valid(eof_song, eof_find_next_anchor(eof_song, eof_selected_beat)))
		{	//If there are no anchors after the selected beat, disable Beat>Add and Delete, as they'd have no effect
			eof_beat_menu[4].flags = D_DISABLED;
			eof_beat_menu[5].flags = D_DISABLED;
		}
		else
		{
			eof_beat_menu[4].flags = 0;	//Add
			eof_beat_menu[5].flags = 0;	//Delete
		}
		if(eof_selected_beat == 0)
		{	//If the first beat marker is selected, disable Beat>Delete, as this beat is not allowed to be deleted
			eof_beat_menu[5].flags = D_DISABLED;
		}
//Beat>Push Offset Up and Push Offset Back validation
		if(eof_song->beat[0]->pos >= eof_song->beat[1]->pos - eof_song->beat[0]->pos)
		{	//If the current MIDI delay is at least as long as the first beat's length, enable Beat>Push Offset Back
			eof_beat_menu[6].flags = 0;	//Push offset back
		}
		else
		{
			eof_beat_menu[6].flags = D_DISABLED;
		}
		if(eof_song->beats > 1)
		{	//If the chart has at least two beat markers, enable Beat>Push Offset Up
			eof_beat_menu[7].flags = 0;	//Push offset up
		}
		else
		{
			eof_beat_menu[7].flags = D_DISABLED;
		}
//Beat>Reset offset to zero validation
		if(eof_song->beat[0]->pos > 0)
		{	//If the current MIDI delay is not zero, enable Beat>Reset offset to zero
			eof_beat_menu[8].flags = 0;	//Reset offset to zero
		}
		else
		{
			eof_beat_menu[8].flags = D_DISABLED;
		}
//Beat>Anchor Beat and Toggle Anchor validation
		if(eof_beat_is_mandatory_anchor(eof_song, eof_selected_beat))
		{	//If the selected beat is required to be an anchor
			eof_beat_menu[10].flags = D_DISABLED;	//Toggle anchor
		}
		else
		{
			eof_beat_menu[10].flags = 0;
		}
		if(eof_selected_beat != 0)
		{	//If the first beat marker is not selected, enable Beat>Anchor Beat and Toggle Anchor
			eof_beat_menu[9].flags = 0;		//Anchor beat
		}
		else
		{
			eof_beat_menu[9].flags = D_DISABLED;
		}
//Beat>Delete Anchor validation
		if((eof_song->beat[eof_selected_beat]->flags & EOF_BEAT_FLAG_ANCHOR) && (eof_selected_beat != 0))
		{	//If the selected beat is an anchor, and the first beat marker is not selected, enable Beat>Delete Anchor
			eof_beat_menu[11].flags = 0;	//Delete anchor
		}
		else
		{
			eof_beat_menu[11].flags = D_DISABLED;
		}
//Beat>Reset BPM validation
		for(i = 1; i < eof_song->beats; i++)
		{
			if(eof_song->beat[i]->ppqn != eof_song->beat[0]->ppqn)
			{
				break;
			}
		}
		if(i == eof_song->beats)
		{	//If there are no tempo changes throughout the entire chart, disable Beat>BPM>Reset BPM, as it would have no effect
			eof_beat_bpm_menu[1].flags = D_DISABLED;
		}
		else
		{
			eof_beat_bpm_menu[1].flags = 0;	//BPM>Reset BPM
		}
//Beat>Estimate validation
		if(eof_silence_loaded || !eof_music_track)
		{	//If no chart audio is loaded
			eof_beat_bpm_menu[3].flags = D_DISABLED;
		}
		else
		{
			eof_beat_bpm_menu[3].flags = 0;	//BPM>Estimate BPM
		}

//BPM>Estimate BPM
#ifdef EOF_NO_MINIBPM
		eof_beat_bpm_menu[3].flags = D_DISABLED;
#endif // EOF_NO_MINIBPM

//Beat>All Events and Clear Events validation
		if(eof_song->text_events > 0)
		{	//If there is at least one defined text event, enable Beat>All Events and Clear Events
			eof_beat_events_menu[0].flags = 0;	//All events
			eof_beat_events_menu[2].flags = 0;	//Clear all events
		}
		else
		{
			eof_beat_events_menu[0].flags = D_DISABLED;
			eof_beat_events_menu[2].flags = D_DISABLED;
		}

		/* lock tempo map */
		if(eof_song->tags->tempo_map_locked)
		{
			eof_beat_menu[16].flags = D_SELECTED;	//Beat>Lock tempo map
		}
		else
		{
			eof_beat_menu[16].flags = 0;
		}

		/* remove mid beat status */
		if(eof_song->beat[eof_selected_beat]->flags & EOF_BEAT_FLAG_MIDBEAT)
		{	//If the selected beat has the mid beat flag set
			eof_beat_menu[17].flags = 0;	//Remove mid-beat status
		}
		else
		{
			eof_beat_menu[17].flags = D_DISABLED;
		}

		if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If a pro guitar/bass track is active
			eof_beat_menu[22].flags = 0;	//Beat>Rocksmith>
			eof_beat_events_menu[3].text = eof_beat_events_menu_place_section_text_pg;	//Show this menu function without the SHIFT+E shortcut, since it's used for placing Rocksmith Events if a pro guitar track is active
			if(eof_selected_track == EOF_TRACK_PRO_GUITAR_B)
			{	//The trainer event system is not compatible with the bonus track
				eof_beat_menu[23].flags = D_DISABLED;
			}
			else
			{
				eof_beat_menu[23].flags = 0;	//Place Trainer Event
			}
		}
		else
		{
			eof_beat_events_menu[3].text = eof_beat_events_menu_place_section_text_non_pg;	//In a non pro guitar track, SHIFT+E will invoke the "Place section" function
			eof_beat_menu[22].flags = D_DISABLED | D_HIDDEN;
			eof_beat_menu[23].flags = D_DISABLED | D_HIDDEN;
		}
//Re-flag the active Time Signature for the selected beat
		if(eof_song->beat[eof_selected_beat]->flags & EOF_BEAT_FLAG_START_4_4)
		{
			eof_beat_time_signature_menu[0].flags = D_SELECTED;
		}
		else if(eof_song->beat[eof_selected_beat]->flags & EOF_BEAT_FLAG_START_2_4)
		{
			eof_beat_time_signature_menu[1].flags = D_SELECTED;
		}
		else if(eof_song->beat[eof_selected_beat]->flags & EOF_BEAT_FLAG_START_3_4)
		{
			eof_beat_time_signature_menu[2].flags = D_SELECTED;
		}
		else if(eof_song->beat[eof_selected_beat]->flags & EOF_BEAT_FLAG_START_5_4)
		{
			eof_beat_time_signature_menu[3].flags = D_SELECTED;
		}
		else if(eof_song->beat[eof_selected_beat]->flags & EOF_BEAT_FLAG_START_6_4)
		{
			eof_beat_time_signature_menu[4].flags = D_SELECTED;
		}
		else if(eof_song->beat[eof_selected_beat]->flags & EOF_BEAT_FLAG_CUSTOM_TS)
		{
			eof_beat_time_signature_menu[5].flags = D_SELECTED;
		}
		else
		{
			eof_beat_time_signature_menu[6].flags = D_SELECTED;
		}
//If any beat before the selected beat has a defined Time Signature, change the menu's "Off" option to "No Change"
		for(i = 0; i < eof_selected_beat; i++)
		{
			if((eof_song->beat[i]->flags & EOF_BEAT_FLAG_START_4_4) || (eof_song->beat[i]->flags & EOF_BEAT_FLAG_START_3_4) || (eof_song->beat[i]->flags & EOF_BEAT_FLAG_START_5_4) || (eof_song->beat[i]->flags & EOF_BEAT_FLAG_START_6_4) || (eof_song->beat[i]->flags & EOF_BEAT_FLAG_CUSTOM_TS))
			{
				(void) ustrcpy(eof_ts_menu_off_text, "No Change");
				break;
			}
		}
		if(i == eof_selected_beat)
		{
			(void) ustrcpy(eof_ts_menu_off_text, "&Off");
		}
//Re-flag the active Key Signature for the selected beat
		for(i = 0; i < 16; i++)
		{
			eof_beat_key_signature_menu[i].flags = 0;
		}
		if(eof_song->beat[eof_selected_beat]->flags & EOF_BEAT_FLAG_KEY_SIG)
		{	//If the selected beat has a key signature defined
			if((eof_song->beat[eof_selected_beat]->key >= -7) && (eof_song->beat[eof_selected_beat]->key <= 7))
			{	//If this beat has a valid key
				eof_beat_key_signature_menu[eof_song->beat[eof_selected_beat]->key + 8].flags = D_SELECTED;	//Map the key to the appropriate menu item (-7 is menu index 1)
			}
			else
			{	//Otherwise remove the key signature
				eof_song->beat[eof_selected_beat]->flags &= ~EOF_BEAT_FLAG_KEY_SIG;	//Clear this flag
			}
		}
		if(!(eof_song->beat[eof_selected_beat]->flags & EOF_BEAT_FLAG_KEY_SIG))
		{	//If the selected beat has no key signature defined (or if it had an invalid signature removed above)
			eof_beat_key_signature_menu[0].flags = D_SELECTED;
		}
//If any beat before the selected beat has a defined Key Signature, change the menu's "Off" option to "No Change"
		for(i = 0; i < eof_selected_beat; i++)
		{
			if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_KEY_SIG)
			{
				(void) ustrcpy(eof_ks_menu_off_text, "No Change");
				break;
			}
		}
		if(i == eof_selected_beat)
		{
			(void) ustrcpy(eof_ks_menu_off_text, "&Off");
		}

		if((eof_selected_beat + 1 < eof_song->beats) && (eof_song->beat[eof_selected_beat + 1]->ppqn != eof_song->beat[eof_selected_beat]->ppqn))
		{	//If there is a beat after the current beat, and it has a different tempo
			eof_beat_halve_bpm_menu[0].flags = D_DISABLED;	//Disable Beat>Halve BPM>Selected
		}
		else
		{
			eof_beat_halve_bpm_menu[0].flags = 0;
		}

		if(eof_check_for_anchors_between_selected_beat_and_seek_pos() || (eof_song->beat[eof_selected_beat]->pos == eof_music_pos.value - eof_av_delay))
		{	//If there are anchors between the selected beat and seek position, or the selected beat is at the seek position already
			eof_beat_menu[18].flags = D_DISABLED;	//Move to seek pos
		}

		if(eof_song->tags->tempo_map_locked)
		{	//If the chart's tempo map is locked, disable various beat operations
			eof_beat_menu[4].flags = D_DISABLED;	//Add
			eof_beat_menu[5].flags = D_DISABLED;	//Delete
			eof_beat_menu[6].flags = D_DISABLED;	//Push offset back
			eof_beat_menu[7].flags = D_DISABLED;	//Push offset up
			eof_beat_menu[8].flags = D_DISABLED;	//Reset offset to zero
			eof_beat_menu[9].flags = D_DISABLED;	//Anchor
			eof_beat_menu[10].flags = D_DISABLED;	//Toggle anchor
			eof_beat_menu[11].flags = D_DISABLED;	//Delete anchor
			eof_beat_menu[12].flags = D_DISABLED;	//Anchor measures
			eof_beat_menu[14].flags = D_DISABLED;	//Paste tempo map
			eof_beat_menu[18].flags = D_DISABLED;	//Move to seek pos

			eof_beat_bpm_menu[0].flags = D_DISABLED;	//BPM>BPM change
			eof_beat_bpm_menu[1].flags = D_DISABLED;	//BPM>Reset BPM
			eof_beat_bpm_menu[2].flags = D_DISABLED;	//BPM>Calculate BPM
			eof_beat_bpm_menu[4].flags = D_DISABLED;	//BPM>Fix tempo for RBN
			eof_beat_bpm_menu[5].flags = D_DISABLED;	//BPM>Double BPM
			eof_beat_bpm_menu[6].flags = D_DISABLED;	//BPM>Halve BPM

			//Also disable time signature functions that can change beat positions
			eof_beat_time_signature_menu[0].flags = D_DISABLED;	//4/4
			eof_beat_time_signature_menu[1].flags = D_DISABLED;	//2/4
			eof_beat_time_signature_menu[2].flags = D_DISABLED;	//3/4
			eof_beat_time_signature_menu[3].flags = D_DISABLED;	//5/4
			eof_beat_time_signature_menu[4].flags = D_DISABLED;	//6/4
			eof_beat_time_signature_menu[5].flags = D_DISABLED;	//Custom
			eof_beat_time_signature_menu[6].flags = D_DISABLED;	//Off
			eof_beat_time_signature_menu[7].flags = D_DISABLED;	//Clear all
		}

		if(eof_song->tags->accurate_ts)
		{	//If the chart's accurate TS option is enabled
			eof_beat_time_signature_menu[8].flags = 0;	//Enable the TS Convert function
		}
		else
		{	//Otherwise disable it
			eof_beat_time_signature_menu[8].flags = D_DISABLED;
		}

		if(eof_create_filtered_menu(eof_beat_menu, eof_filtered_beat_menu, EOF_SCRATCH_MENU_SIZE))
		{	//If the Beat menu was recreated to filter out hidden items
			eof_main_menu[5].child = eof_filtered_beat_menu;	//Use this in the main menu
			eof_effective_beat_menu = eof_filtered_beat_menu;	//And in the context menu
		}
		else
		{	//Otherwise use the unabridged Beat menu
			eof_main_menu[5].child = eof_beat_menu;
			eof_effective_beat_menu = eof_beat_menu;
		}
	}//If a song is loaded
}

int eof_menu_beat_bpm_change(void)
{
	unsigned long cppqn, newppqn, i;
	int old_flags;
	double oldbpm;
	static int first_launch = 1;

	if(eof_song->tags->tempo_map_locked || (eof_selected_beat >= eof_song->beats))	//If the chart's tempo map is locked, or an invalid beat is selected
		return 1;							//Return without making changes

	if(first_launch)
	{	//On first launch of this dialog in an EOF session
		if(eof_note_auto_adjust)
		{	//If the note auto-adjust preference is enabled
			eof_bpm_change_dialog[4].flags = D_SELECTED;	//Check the "adjust notes" checkbox automatically
		}
		first_launch = 0;
	}

	cppqn = eof_song->beat[eof_selected_beat]->ppqn;
	old_flags = eof_song->beat[eof_selected_beat]->flags;
	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_bpm_change_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_bpm_change_dialog);
	oldbpm = 60000000.0 / (double)eof_song->beat[eof_selected_beat]->ppqn;
	(void) snprintf(eof_etext, sizeof(eof_etext) - 1, "%3.2f", oldbpm);
	if(eof_popup_dialog(eof_bpm_change_dialog, 2) == 5)
	{	//If the user activated the "OK" button
		double bpm = atof(eof_etext);
		(void) snprintf(eof_etext2, sizeof(eof_etext2) - 1, "%3.2f", oldbpm);
		if(!ustricmp(eof_etext, eof_etext2))
		{
			eof_cursor_visible = 1;
			eof_pen_visible = 1;
			eof_show_mouse(NULL);
			return 1;
		}
		if((bpm < 0.1) || (bpm > 2000.0))
		{
			eof_render();
			allegro_message("BPM must be between 0.1 and 2000.");
			eof_cursor_visible = 1;
			eof_pen_visible = 1;
			eof_show_mouse(NULL);
			return 1;
		}
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		if(eof_bpm_change_dialog[4].flags == D_SELECTED)
		{	//If the "Adjust Notes" option was selected
			eof_song->beat[eof_selected_beat]->flags |= EOF_BEAT_FLAG_ANCHOR;	//Ensure the selected beat is anchored before calling the auto-adjust logic
			(void) eof_menu_edit_cut(0, 1);	//Save auto-adjust data for the entire chart
			eof_song->beat[eof_selected_beat]->flags = old_flags;
		}
		newppqn = 60000000.0 / bpm;
		if(eof_bpm_change_dialog[3].flags == D_SELECTED)
		{	//If the "This Beat Only" option was selected
			eof_song->beat[eof_selected_beat]->ppqn = newppqn;	//Update this beat's tempo
			eof_song->beat[eof_selected_beat + 1]->flags |= EOF_BEAT_FLAG_ANCHOR;	//And anchor the next beat
		}
		else
		{	//Otherwise update the tempo on all beats up to the next anchor
			for(i = eof_selected_beat; i < eof_song->beats; i++)
			{	//For each beat from the selected beat onward
				if(eof_song->beat[i]->ppqn == cppqn)
				{	//If this beat isn't a tempo change
					eof_song->beat[i]->ppqn = newppqn;	//Update its tempo
				}

				/* break when we reach the end of the portion to change */
				else
				{
					break;	//Otherwise exit the loop
				}
			}
		}
		eof_song->beat[eof_selected_beat]->flags |= EOF_BEAT_FLAG_ANCHOR;
		eof_calculate_beats(eof_song);

		if(eof_bpm_change_dialog[4].flags == D_SELECTED)
		{	//If the "Adjust Notes" option was selected
			(void) eof_menu_edit_cut_paste(0, 1);	//Apply auto-adjust data for the entire chart
		}
		eof_truncate_chart(eof_song);	//Update number of beats and the chart length as appropriate, but only after the notes are optionally auto-adjusted
		eof_beat_stats_cached = 0;		//Mark the cached beat stats as not current
		(void) eof_detect_difficulties(eof_song, eof_selected_track);	//Update note highlighting
	}//If the user activated the "OK" button
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(NULL);
	return 1;
}

int eof_menu_beat_ts_4_4(void)
{
	(void) eof_apply_ts(4, 4, eof_selected_beat,eof_song, 1);
	eof_calculate_beats(eof_song);
	eof_truncate_chart(eof_song);
	eof_select_beat(eof_selected_beat);
	return 1;
}

int eof_menu_beat_ts_2_4(void)
{
	(void) eof_apply_ts(2, 4, eof_selected_beat,eof_song, 1);
	eof_calculate_beats(eof_song);
	eof_truncate_chart(eof_song);
	eof_select_beat(eof_selected_beat);
	return 1;
}

int eof_menu_beat_ts_3_4(void)
{
	(void) eof_apply_ts(3, 4, eof_selected_beat,eof_song, 1);
	eof_calculate_beats(eof_song);
	eof_truncate_chart(eof_song);
	eof_select_beat(eof_selected_beat);
	return 1;
}

int eof_menu_beat_ts_5_4(void)
{
	(void) eof_apply_ts(5, 4, eof_selected_beat,eof_song, 1);
	eof_calculate_beats(eof_song);
	eof_truncate_chart(eof_song);
	eof_select_beat(eof_selected_beat);
	return 1;
}

int eof_menu_beat_ts_6_4(void)
{
	(void) eof_apply_ts(6, 4, eof_selected_beat,eof_song, 1);
	eof_calculate_beats(eof_song);
	eof_truncate_chart(eof_song);
	eof_select_beat(eof_selected_beat);
	return 1;
}

DIALOG eof_custom_ts_dialog[] =
{
	/* (proc)				  (x)  (y)  (w)  (h) (fg) (bg) (key) (flags) (d1) (d2) (dp)                 (dp2) (dp3) */
	{ eof_shadow_box_proc, 32,  68,  175, 95, 2,   23,  0,    0,      0,   0,   NULL,                NULL, NULL },
	{ d_agup_text_proc,		  42,  84,  35,  8,  2,   23,  0,    0,      0,   0,   "Beats per measure:",NULL, NULL },
	{ eof_verified_edit_proc, 160, 80,  35,  20, 2,   23,  0,    0,      3,   0,   eof_etext,           "0123456789", NULL },
	{ d_agup_text_proc,		  42,  105, 35,  8,  2,   23,  0,    0,      0,   0,   "Beat unit:",        NULL, NULL },
	{ eof_verified_edit_proc, 160, 101, 35,  20, 2,   23,  0,    0,      3,   0,   eof_etext2,          "0123456789", NULL },
	{ d_agup_button_proc,	  42,  125, 68,  28, 2,   23,  '\r', D_EXIT, 0,   0,   "OK",                NULL, NULL },
	{ d_agup_button_proc,	  125, 125, 68,  28, 2,   23,  0,    D_EXIT, 0,   0,   "Cancel",            NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

int eof_menu_beat_ts_custom_dialog(unsigned start)
{
	unsigned num = 4, den = 4;
	int retval = 0;

//Prompt the user for the custom time signature
	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_custom_ts_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_custom_ts_dialog);
	(void) eof_get_effective_ts(eof_song, &num, &den, eof_selected_beat, 0);
	(void) snprintf(eof_etext, sizeof(eof_etext) - 1, "%u", num);
	(void) snprintf(eof_etext2, sizeof(eof_etext2) - 1, "%u", den);
	if(start)
	{	//If the calling function wanted the numerator field to be given initial focus
		start = 2;	//Set this to the dialog object index of that field
	}
	else
	{	//Otherwise set it to make the denominator have default focus
		start = 4;
	}
	if(eof_popup_dialog(eof_custom_ts_dialog, start) == 5)
	{	//User clicked OK
		num = atoi(eof_etext);
		den = atoi(eof_etext2);

		if((num > 256) || (num < 1) || (den > 256) || (den < 1))
		{	//These values must fit within an 8 bit number (where all bits zero represents 1 and all bits set represents 256)
			eof_render();
			allegro_message("Time signature numerator and denominator must be between 1 and 256");
		}
		else
		{
			if((den != 1) && (den != 2) && (den != 4) && (den != 8) && (den != 16) && (den != 32) && (den != 64) && (den != 128) && (den != 256))
			{
				eof_render();
				allegro_message("Time signature denominator must be a power of two");
			}
			else
			{	//User provided a valid time signature
				(void) eof_apply_ts(num,den,eof_selected_beat,eof_song,1);
				retval = 1;
			}
		}
	}

	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(NULL);
	return retval;
}

int eof_menu_beat_ts_custom(void)
{
	if(eof_menu_beat_ts_custom_dialog(1))
	{	//If a valid time signature was chosen and applied
		eof_calculate_beats(eof_song);
		eof_truncate_chart(eof_song);
		eof_select_beat(eof_selected_beat);
	}

	return 1;
}

int eof_menu_beat_ts_convert(void)
{
	unsigned long ctr;
	unsigned int num = 4, den = 4, prevden = 4;

	if(!eof_song->tags->accurate_ts)
		return 1;	//Don't allow this function to run if the accurate TS chart option is not enabled
	if(!eof_menu_beat_ts_custom_dialog(0))
		return 1;	//If a valid time signature was not chosen and applied, return immediately

	(void) eof_get_ts(eof_song, &num, &den, eof_selected_beat);	//Obtain the new TS
	for(ctr = eof_selected_beat; ctr + 1 < eof_song->beats;)
	{	//For each beat that has a beat that follows it, starting with the selected one
		eof_song->beat[ctr]->ppqn = 1000 * (eof_song->beat[ctr + 1]->pos - eof_song->beat[ctr]->pos);	//Calculate the tempo of the beat by getting its length (this is the formula "beat_length = 60000 / BPM" rewritten to solve for ppqn)
		if(den != 4)
		{	//If the time signature needs to be taken into account
			eof_song->beat[ctr]->ppqn *= (double)den / 4.0;	//Adjust for it
		}
		ctr++;	//Iterate to next beat
		if(eof_get_ts(eof_song, &num, &den, ctr) == 1)
		{	//If the next beat has a time signature change
			eof_song->beat[ctr]->flags |= EOF_BEAT_FLAG_ANCHOR;	//Anchor it to be safe
			break;
		}
	}
	if(eof_selected_beat > 0)
	{	//If a beat other than the first one is selected, determine if it now needs to be anchored
		(void) eof_get_effective_ts(eof_song, NULL, &prevden, eof_selected_beat - 1, 0);	//Determine what TS is in effect at the previous beat
		if((prevden != den) || (eof_song->beat[eof_selected_beat]->ppqn != eof_song->beat[eof_selected_beat - 1]->ppqn))
		{	//If the time signature denominator or the tempo is different from the previous beat
			eof_song->beat[eof_selected_beat]->flags |= EOF_BEAT_FLAG_ANCHOR;	//Set the anchor flag
		}
	}

	return 1;
}

int eof_menu_beat_ts_off(void)
{
//Clear the beat's status except for its anchor and event flags
	unsigned long flags = eof_song->beat[eof_selected_beat]->flags;

	flags &= ~EOF_BEAT_FLAG_START_4_4;	//Clear this TS flag
	flags &= ~EOF_BEAT_FLAG_START_2_4;	//Clear this TS flag
	flags &= ~EOF_BEAT_FLAG_START_3_4;	//Clear this TS flag
	flags &= ~EOF_BEAT_FLAG_START_5_4;	//Clear this TS flag
	flags &= ~EOF_BEAT_FLAG_START_6_4;	//Clear this TS flag
	flags &= ~EOF_BEAT_FLAG_CUSTOM_TS;	//Clear this TS flag
	flags &= ~0xFF000000;	//Clear any custom TS numerator
	flags &= ~0x00FF0000;	//Clear any custom TS denominator
	if(flags != eof_song->beat[eof_selected_beat]->flags)
	{	//If the user has changed the time signature status of this beat
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		eof_song->beat[eof_selected_beat]->flags = flags;
		eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current
		eof_calculate_beats(eof_song);
		eof_truncate_chart(eof_song);
		eof_select_beat(eof_selected_beat);
	}
	return 1;
}

void eof_menu_beat_delete_logic(unsigned long beat)
{
	int flags;

	if(!eof_song || !beat || (beat >= eof_song->beats))
		return;	//Invalid parameters

	if(eof_song->tags->tempo_map_locked)	//If the chart's tempo map is locked
		return;								//Return without making changes

	flags = eof_song->beat[beat]->flags;
	eof_song_delete_beat(eof_song, beat);
	if((eof_song->beat[beat - 1]->flags & EOF_BEAT_FLAG_ANCHOR) && (eof_song->beat[beat]->flags & EOF_BEAT_FLAG_ANCHOR))
	{	//If the beat after the newly deleted beat is an anchor, and the previous beat is also an anchor
		double beats_length = eof_song->beat[beat]->fpos - eof_song->beat[beat - 1]->fpos;
		double newbpm = 60000.0 / beats_length;
		double newppqn = 60000000.0 / newbpm;
		eof_song->beat[beat - 1]->ppqn = newppqn;	//Change the tempo on the previous beat
	}
	else if(eof_song->beat[beat - 1]->flags & EOF_BEAT_FLAG_ANCHOR)
	{
		eof_realign_beats(eof_song, beat);
	}
	else
	{
		eof_realign_beats(eof_song, beat - 1);
	}
	eof_move_text_events(eof_song, beat, 1, -1);
	flags &= (~EOF_BEAT_FLAG_ANCHOR);	//Clear the anchor flag
	flags &= (~EOF_BEAT_FLAG_MIDBEAT);	//Clear the mid-beat flag
	eof_song->beat[beat - 1]->flags |= flags;
	eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current
}

int eof_menu_beat_delete(void)
{
	if(!eof_song)
		return 1;
	if(eof_song->tags->tempo_map_locked)	//If the chart's tempo map is locked
		return 1;							//Return without making changes

	if((eof_selected_beat > 0) && (eof_beat_num_valid(eof_song, eof_find_next_anchor(eof_song, eof_selected_beat))))
	{	//Only process this function if a beat other than beat 0 is selected, and there is at least one anchor after the selected beat
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		eof_menu_beat_delete_logic(eof_selected_beat);
		eof_fixup_notes(eof_song);
	}
	return 1;
}

int eof_menu_beat_push_offset_back(char *undo_made)
{
	unsigned long i;
	double backamount;

	if(!eof_song)
		return 1;
	if(eof_song->tags->tempo_map_locked)	//If the chart's tempo map is locked
		return 0;							//Return without making changes
	if(!undo_made)
		return 0;	//Invalid parameter
	if(eof_song->beats < 2)
		return 0;	//Error condition

	backamount = eof_song->beat[1]->fpos - eof_song->beat[0]->fpos;
	if(eof_song->beat[0]->fpos < backamount)
		return 1;	//If the first beat isn't at least one beat length away from 0ms, don't change anything

	if(*undo_made == 0)
	{	//If an undo state needs to be made
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		*undo_made = 1;
	}
	if(eof_song_resize_beats(eof_song, eof_song->beats + 1))
	{	//If the beats array was successfully resized
		for(i = eof_song->beats - 1; i > 0; i--)
		{	//For each beat, in reverse order
			memcpy(eof_song->beat[i], eof_song->beat[i - 1], sizeof(EOF_BEAT_MARKER));	//The beat becomes a copy of the beat before it
		}
		//Re-initialize the first beat
		eof_song->beat[0]->fpos = eof_song->beat[1]->fpos - backamount;
		eof_song->beat[0]->pos = eof_song->beat[0]->fpos + 0.5;	//Round to nearest ms
		eof_song->beat[0]->flags = EOF_BEAT_FLAG_ANCHOR;
		eof_song->tags->ogg[0].midi_offset = eof_song->beat[0]->pos;
		eof_move_text_events(eof_song, 0, 1, 1);
		eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current
	}
	else
		return 0;	//Return failure

	return 1;	//Return success
}

int eof_menu_beat_push_offset_back_menu(void)
{
	char undo_made = 0;

	return eof_menu_beat_push_offset_back(&undo_made);
}

int eof_menu_beat_push_offset_up(void)
{
	unsigned long i;

	if(!eof_song)
		return 1;
	if(eof_song->tags->tempo_map_locked)	//If the chart's tempo map is locked
		return 1;							//Return without making changes

	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	for(i = 0; i < eof_song->beats - 1; i++)
	{
		memcpy(eof_song->beat[i], eof_song->beat[i + 1], sizeof(EOF_BEAT_MARKER));
	}
	eof_song_delete_beat(eof_song, eof_song->beats - 1);
	eof_song->tags->ogg[0].midi_offset = eof_song->beat[0]->pos;
	eof_move_text_events(eof_song, 0, 1, -1);
	eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current
	eof_fixup_notes(eof_song);
	return 1;
}

int eof_menu_beat_reset_offset(void)
{
	int i;
	double newbpm;
	char undo_made = 0;
	unsigned long last_pos;

	if(!eof_song)
		return 1;
	if(eof_song->beat[0]->pos == 0)
		return 1;	//If the MIDI delay is already zero, return without changing anything

	if(eof_song->tags->tempo_map_locked)	//If the chart's tempo map is locked
	{
		eof_clear_input();
		if(alert("Cannot perform this operation while the tempo map is locked.", NULL, "Would you like to unlock the tempo map?", "&Yes", "&No", 'y', 'n') != 1)
		{	//If user does not opt to unlock the tempo map to carry out the operation
			return 1;	//Return without making changes
		}
		eof_song->tags->tempo_map_locked = 0;	//Unlock the tempo map
	}

	if(eof_song->beats < 2)
		return 1;	//Chart must be at least 2 beats

	if(eof_song->beat[0]->pos >= eof_song->beat[1]->pos - eof_song->beat[0]->pos)
	{	//If the MIDI delay is at least one beat length long, offer to insert as many evenly spaced beats as possible
		eof_clear_input();
		if(alert(NULL, "Insert evenly spaced beats at the beginning of the chart?", NULL, "&Yes", "&No", 'y', 'n') == 1)
		{	//If user opts to insert evenly spaced beats
			while(eof_song->beat[0]->pos >= eof_song->beat[1]->pos - eof_song->beat[0]->pos)
			{	//While the MIDI delay is still large enough to be moved back one beat
				last_pos = eof_song->beat[0]->pos;	//Track this in case floating point beat positional rounding up would cause this loop to get stuck indefinitely
				if(!eof_menu_beat_push_offset_back(&undo_made))
				{	//If pushing back the chart by one beat failed
					break;	//Break from this loop and attempt to finish the normal reset offset logic
				}
				if(last_pos == eof_song->beat[0]->pos)
				{	//If the first beat's position did not move from the last call to eof_menu_beat_push_offset_back()
					break;
				}
			}
		}
	}

	if(eof_song->beat[0]->pos == 0)
	{	//If the beat push back completely filled the offset and the first beat marker is now at 0ms
		return 1;	//No more changes are needed
	}

	if(!undo_made)
	{	//If an undo state hasn't been made yet
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	}
	if(eof_song_resize_beats(eof_song, eof_song->beats + 1))
	{	//If the beats array was successfully resized
		for(i = eof_song->beats - 1; i > 0; i--)
		{
			memcpy(eof_song->beat[i], eof_song->beat[i - 1], sizeof(EOF_BEAT_MARKER));
		}
		eof_song->beat[0]->pos = eof_song->beat[0]->fpos = 0;
		eof_song->beat[0]->flags = eof_song->beat[1]->flags;	//Copy the flags (ie. Time Signature) of the original first beat marker
		eof_song->tags->ogg[0].midi_offset = 0;
		newbpm = 60000.0 / eof_song->beat[1]->fpos;	//60000ms / length of new beat (the MIDI delay) = Tempo
		eof_song->beat[0]->ppqn = 60000000.0 / newbpm;	//60000000usec_per_minute / tempo = PPQN
		eof_move_text_events(eof_song, 0, 1, 1);
		if(eof_song->beat[1]->ppqn != eof_song->beat[0]->ppqn)
		{	//If this operation caused the first and second beat markers to have different tempos,
			eof_song->beat[1]->flags = EOF_BEAT_FLAG_ANCHOR;		//Set the second beat marker's anchor flag
		}
		eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current
	}
	else
		return 0;	//Return failure

	return 1;	//Return success
}

int eof_menu_beat_anchor(void)
{
	int mm, ss, ms;
	int oldmm, oldss, oldms;
	unsigned long oldpos;
	unsigned long newpos = 0;
	int revert = 0;
	char ttext[4] = {0};

	if(!eof_song)
		return 1;							//No project loaded
	if(eof_song->tags->tempo_map_locked)	//If the chart's tempo map is locked
		return 1;							//Return without making changes
	if(eof_selected_beat == 0)
		return 1;							//This function is not allowed on the first beat marker
	if(eof_song->beats < 2)
		return 1;							//This function expects the project to have at least two beats

	oldpos = eof_song->beat[eof_selected_beat]->pos;
	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_anchor_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_anchor_dialog);
	oldmm = (eof_song->beat[eof_selected_beat]->pos / 1000) / 60;
	oldss = (eof_song->beat[eof_selected_beat]->pos / 1000) % 60;
	oldms = (eof_song->beat[eof_selected_beat]->pos) % 1000;
	(void) snprintf(eof_etext2, sizeof(eof_etext2) - 1, "%02d:%02d.%03d", oldmm, oldss, oldms);
	if(eof_popup_dialog(eof_anchor_dialog, 2) == 4)
	{	//If the user clicked OK
		ttext[0] = eof_etext2[0];
		ttext[1] = eof_etext2[1];
		mm = atoi(ttext);
		ttext[0] = eof_etext2[3];
		ttext[1] = eof_etext2[4];
		ss = atoi(ttext);
		ttext[0] = eof_etext2[6];
		ttext[1] = eof_etext2[7];
		ttext[2] = eof_etext2[8];
		ms = atoi(ttext);

		/* time wasn't modified so get out without changing anything */
		if((mm == oldmm) && (ss == oldss) && (ms == oldms))
		{
			eof_cursor_visible = 1;
			eof_pen_visible = 1;
			eof_show_mouse(NULL);
			return 1;
		}
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		if(eof_anchor_dialog[3].flags == D_SELECTED)
		{	//If the "Adjust Notes" option was selected
			(void) eof_menu_edit_cut(eof_selected_beat, 1);
		}
		newpos = (double)mm * 60.0 * 1000.0 + (double)ss * 1000.0 + (double)ms;
///It's unclear why this function was designed to move the affected beat 1ms at a time or why that would be necessary
		if(newpos > oldpos)
		{
			while(eof_song->beat[eof_selected_beat]->pos < newpos)
			{
				eof_song->beat[eof_selected_beat]->fpos++;
				eof_song->beat[eof_selected_beat]->pos = eof_song->beat[eof_selected_beat]->fpos + 0.5;	//Round up to nearest ms
				eof_mickeys_x = 1;
				eof_recalculate_beats(eof_song, eof_selected_beat);
				if((eof_selected_beat < eof_song->beats - 1) && (eof_song->beat[eof_selected_beat]->pos > eof_song->beat[eof_selected_beat + 1]->pos - 100))
				{	//If there is another beat, and the selected beat was moved to within 99ms of it, undo the operation
					(void) eof_undo_apply();
					revert = 1;
					break;
				}
			}
		}
		else if(newpos < oldpos)
		{
			while(eof_song->beat[eof_selected_beat]->pos > newpos)
			{
				eof_song->beat[eof_selected_beat]->fpos--;
				eof_song->beat[eof_selected_beat]->pos = eof_song->beat[eof_selected_beat]->fpos + 0.5;	//Round up to nearest ms
				eof_mickeys_x = 1;
				eof_recalculate_beats(eof_song, eof_selected_beat);
				if(eof_song->beat[eof_selected_beat]->pos < eof_song->beat[eof_selected_beat - 1]->pos + 100)
				{
					(void) eof_undo_apply();
					revert = 1;
					break;
				}
			}
		}
		if(!revert)
		{
			eof_song->beat[eof_selected_beat]->flags |= EOF_BEAT_FLAG_ANCHOR;
			eof_calculate_beats(eof_song);
			eof_truncate_chart(eof_song);	//Update number of beats and the chart length as appropriate
			if(eof_anchor_dialog[3].flags == D_SELECTED)
			{	//If the "Adjust Notes" option was selected
				(void) eof_menu_edit_cut_paste(eof_selected_beat, 1);
			}
		}
	}
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(NULL);
	eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current
	return 1;
}

int eof_menu_beat_toggle_anchor(void)
{
	if(!eof_song)
		return 1;
	if(eof_song->tags->tempo_map_locked)	//If the chart's tempo map is locked
		return 1;							//Return without making changes
	if(eof_selected_beat == 0)
		return 1;	//Don't attempt to remove the anchor from the first beat
	if(eof_beat_is_mandatory_anchor(eof_song, eof_selected_beat))
		return 1;	//Don't attempt to remove the anchor if the selected beat has a TS or tempo change that forces it to be an anchor

	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	eof_song->beat[eof_selected_beat]->flags ^= EOF_BEAT_FLAG_ANCHOR;
	return 1;
}

int eof_menu_beat_move_to_seek_pos(void)
{
	if(!eof_song)
		return 1;							//No project loaded
	if(eof_song->tags->tempo_map_locked)	//If the chart's tempo map is locked
		return 1;							//Return without making changes
	if(!eof_beat_num_valid(eof_song, eof_selected_beat))
		return 1;							//Logic error
	if(eof_song->beat[eof_selected_beat]->pos == eof_music_pos.value - eof_av_delay)
		return 1;							//The seek position is at the selected beat's position already
	if(eof_check_for_anchors_between_selected_beat_and_seek_pos())
		return 1;							//Don't allow this function to move the selected beat through an anchor

	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	eof_song->beat[eof_selected_beat]->pos = eof_song->beat[eof_selected_beat]->fpos = eof_music_pos.value - eof_av_delay;	//Update the selected beat's position
	eof_recalculate_beats(eof_song, eof_selected_beat);	//Update beat timings
	eof_song->beat[eof_selected_beat]->flags |= EOF_BEAT_FLAG_ANCHOR;
	eof_fixup_notes(eof_song);										//Update note highlighting
	(void) eof_detect_difficulties(eof_song, eof_selected_track);	//Update tab highlighting

	return 1;
}

int eof_menu_beat_delete_anchor_logic(char *undo_made)
{
	unsigned long cppqn, i;

	if(!eof_song)
		return 1;	//Invalid parameter
	if(eof_song->tags->tempo_map_locked)	//If the chart's tempo map is locked
		return 1;							//Return without making changes
	if(!undo_made)
		return 0;	//Invalid parameter

	cppqn = eof_song->beat[eof_selected_beat]->ppqn;
	if((eof_selected_beat > 0) && eof_beat_is_anchor(eof_song, eof_selected_beat))
	{
		if(*undo_made == 0)
		{	//If an undo state needs to be made
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			*undo_made = 1;
		}
		for(i = eof_selected_beat; i < eof_song->beats; i++)
		{
			if(eof_song->beat[i]->ppqn == cppqn)
			{
				eof_song->beat[i]->ppqn = eof_song->beat[eof_selected_beat - 1]->ppqn;
			}
			else
			{
				break;
			}
		}
		eof_song->beat[eof_selected_beat]->flags &= ~EOF_BEAT_FLAG_ANCHOR;	//Clear the anchor flag, retain all other flags
		if(!eof_beat_num_valid(eof_song, eof_find_next_anchor(eof_song, eof_selected_beat)))
		{	//If there's not an anchor after the selected beat
			(void) eof_song_resize_beats(eof_song, eof_selected_beat);
			eof_calculate_beats(eof_song);
			eof_truncate_chart(eof_song);	//Update number of beats and the chart length as appropriate
		}
		else
		{
			eof_realign_beats(eof_song, eof_selected_beat);
		}
	}
	eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current
	return 1;
}

int eof_menu_beat_delete_anchor(void)
{
	char undo_made = 0;
	int retval;

	if(eof_note_auto_adjust)
	{	//If the user has enabled the Note Auto-Adjust preference
		(void) eof_menu_edit_cut(0, 1);	//Save auto-adjust data for the entire chart
	}
	retval = eof_menu_beat_delete_anchor_logic(&undo_made);
	eof_fixup_notes(eof_song);
	if(eof_note_auto_adjust)
	{	//If the user has enabled the Note Auto-Adjust preference
		(void) eof_menu_edit_cut_paste(0, 1);	//Apply auto-adjust data for the entire chart
	}

	return retval;
}

int eof_menu_beat_anchor_measures(void)
{
	char undo_made = 0;
	unsigned long ctr;

	if(!eof_song)	//If no chart is loaded, abort
		return 1;

	if(eof_note_auto_adjust)
	{	//If the user has enabled the Note Auto-Adjust preference
		(void) eof_menu_edit_cut(0, 1);	//Save auto-adjust data for the entire chart
	}

	//Anchor all first beats in a measure
	for(ctr = 1; ctr < eof_song->beats; ctr++)
	{	//For each beat after the first (which is always an anchor)
		if(eof_song->beat[ctr]->has_ts)
		{	//If a time signature is in effect at this beat
			if(!eof_song->beat[ctr]->beat_within_measure)
			{	//And this is the first beat in a measure
				if(!(eof_song->beat[ctr]->flags & EOF_BEAT_FLAG_ANCHOR))
				{	//If this beat isn't already an anchor
					if(undo_made == 0)
					{	//If an undo state needs to be made
						eof_prepare_undo(EOF_UNDO_TYPE_NONE);
						undo_made = 1;
					}
					eof_song->beat[ctr]->flags |= EOF_BEAT_FLAG_ANCHOR;	//Turn it into one
				}
			}
		}
	}

	//Delete anchors from all other beats in a measure
	for(ctr = 1; ctr < eof_song->beats; ctr++)
	{	//For each beat after the first (which is always an anchor)
		if(eof_song->beat[ctr]->has_ts)
		{	//If a time signature is in effect at this beat
			if(eof_song->beat[ctr]->beat_within_measure)
			{	//And this is NOT the first beat in a measure
				eof_selected_beat = ctr;	//Select the beat
				(void) eof_menu_beat_delete_anchor_logic(&undo_made);	//Perform "Beat>Delete Anchor" on the beat, creating an undo state if necessary
			}
		}
	}

	if(eof_note_auto_adjust)
	{	//If the user has enabled the Note Auto-Adjust preference
		(void) eof_menu_edit_cut_paste(0, 1);	//Apply auto-adjust data for the entire chart
	}

	(void) eof_detect_tempo_map_corruption(eof_song, 0);
	eof_close_menu = 1;			//Force the main menu to close, as this function had a tendency to get hung in the menu logic when activated by keyboard

	return 1;
}

int eof_menu_beat_reset_bpm(void)
{
	int reset = 0, adjust = 0;
	unsigned long startbeat = 0, i;

	if(!eof_song || eof_song->tags->tempo_map_locked)	//If no chart is loaded or the chart's tempo map is locked
		return 1;										//Return without making changes

	for(i = 1; i < eof_song->tracks; i++)
	{	//For each track
		if(eof_get_track_size_all(eof_song, i))
		{	//If the track is populated with normal notes or tech notes
			eof_clear_input();
			if(alert(NULL, "Adjust notes?", NULL, "&Yes", "&No", 'y', 'n') == 1)
			{	//If the user opts to adjust the notes
				adjust = 1;
			}
			break;
		}
	}

	if(eof_selected_beat != 0)
	{	//If a beat besides the first is selected
		if(alert(NULL, "Erase all BPM changes after the first beat, or all BPM changes after the selected beat?", NULL, "First", "Selected", 0, 0) == 2)
		{	//If the user opted to only erase BPM changes after the selected beat
			startbeat = eof_selected_beat;
		}
	}

	for(i = startbeat + 1; i < eof_song->beats; i++)
	{
		if(eof_song->beat[i]->ppqn != eof_song->beat[startbeat]->ppqn)
		{
			reset = 1;
			break;
		}
	}
	if(reset)
	{
		if(alert(NULL, "Erase specified BPM changes?", NULL, "OK", "Cancel", 0, 0) == 1)
		{
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			if(adjust)
			{	//If the user opted to auto adjust the notes
				(void) eof_menu_edit_cut(startbeat + 1, 1);
			}
			for(i = startbeat + 1; i < eof_song->beats; i++)
			{
				eof_song->beat[i]->ppqn = eof_song->beat[startbeat]->ppqn;
				eof_song->beat[i]->flags &= ~EOF_BEAT_FLAG_ANCHOR;	//Clear the anchor flag
			}
			eof_calculate_beats(eof_song);	//Rebuild the beat timings
			if(adjust)
			{	//If the user opted to auto adjust the notes
				(void) eof_menu_edit_cut_paste(startbeat + 1, 1);
			}
			eof_truncate_chart(eof_song);	//Update number of beats and the chart length as appropriate
			eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current
		}
	}
	else
	{
		allegro_message("No BPM changes to erase!");
	}
	eof_clear_input();
	return 1;
}

int eof_menu_beat_remove_ts(void)
{
	unsigned junk1, junk2;
	unsigned long i, flags;
	char reset = 0;

	for(i = 0; i < eof_song->beats; i++)
	{	//For each beat
		if(eof_get_ts(eof_song, &junk1, &junk2, i) == 1)
		{	//If this beat has a time signature defined
			reset = 1;
			break;
		}
	}
	if(reset)
	{	//If there was at least one time signature change
		if(alert(NULL, "Erase all time signature changes?", NULL, "OK", "Cancel", 0, 0) == 1)
		{	//If the user opted to erase the changes
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			for(i = 0; i < eof_song->beats; i++)
			{
				flags = eof_song->beat[i]->flags;
				flags &= ~EOF_BEAT_FLAG_START_4_4;	//Clear this TS flag
				flags &= ~EOF_BEAT_FLAG_START_2_4;	//Clear this TS flag
				flags &= ~EOF_BEAT_FLAG_START_3_4;	//Clear this TS flag
				flags &= ~EOF_BEAT_FLAG_START_5_4;	//Clear this TS flag
				flags &= ~EOF_BEAT_FLAG_START_6_4;	//Clear this TS flag
				flags &= ~EOF_BEAT_FLAG_CUSTOM_TS;	//Clear this TS flag
				flags &= ~0xFF000000;				//Clear any custom TS numerator
				flags &= ~0x00FF0000;				//Clear any custom TS denominator
				eof_song->beat[i]->flags = flags;
			}
			eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current
			eof_calculate_beats(eof_song);
			eof_truncate_chart(eof_song);
		}
	}
	else
	{
		allegro_message("No time signature changes to erase!");
	}
	eof_clear_input();
	return 1;
}

int eof_menu_beat_calculate_bpm(void)
{
	unsigned long i;
	int first = 1;
	unsigned long curpos = 0;
	unsigned long delta;
	unsigned long cppqn;
	double bpm = 0.0;
	unsigned long bpm_count = 0;

	if(!eof_song)
		return 1;
	if(eof_song->tags->tempo_map_locked)	//If the chart's tempo map is locked
		return 1;							//Return without making changes

	cppqn = eof_song->beat[eof_selected_beat]->ppqn;
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If the note is selected
			if(first)
			{
				curpos = eof_get_note_pos(eof_song, eof_selected_track, i);
				first = 0;
			}
			else
			{
				delta = eof_get_note_pos(eof_song, eof_selected_track, i) - curpos;
				curpos = eof_get_note_pos(eof_song, eof_selected_track, i);
				bpm += 60000.0 / (double)delta;
				bpm_count++;
			}
		}
	}
	if(bpm_count >= 1)
	{
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		bpm = bpm / (double)bpm_count;
		for(i = eof_selected_beat; i < eof_song->beats; i++)
		{
			if(eof_song->beat[i]->ppqn == cppqn)
			{
				eof_song->beat[i]->ppqn = 60000000.0 / bpm;
			}
			else
			{
				break;
			}
		}
	}
	eof_calculate_beats(eof_song);
	eof_truncate_chart(eof_song);	//Update number of beats and the chart length as appropriate
	eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current

	return 1;
}

int eof_menu_beat_all_events(void)
{
	unsigned long track, realindex, flags, ctr, count;
	int retval;
	char undo_made = 0;

	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_all_events_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_all_events_dialog);

	//Pre-select the last non-filtered event that is at/before the seek position
	eof_all_events_dialog[1].d1 = 0;
	for(ctr = 0, count = 0; ctr < eof_song->text_events; ctr++)
	{	//For each text event in the project
		if(eof_get_text_event_pos(eof_song, ctr) > eof_music_pos.value - eof_av_delay)
			break;	//If this text event and all subsequent ones occur after the seek position, stop processing text events

		if(eof_event_is_not_filtered_from_listing(ctr))
		{	//If this text event is to be displayed with the current filter settings
			eof_all_events_dialog[1].d1 = count;
			count++;
		}
	}

	if(eof_events_overridden_by_stored_MIDI_track(eof_song))
	{	//If there is a stored events track
		eof_all_events_dialog[10].dp = stored_event_track_notice;	//Add a warning to the dialog
	}
	else
	{
		eof_all_events_dialog[10].dp = no_notice;	//Otherwise remove the warning
	}
	while(1)
	{	//Until the user closes the dialog
		retval = eof_popup_dialog(eof_all_events_dialog, 0);
		realindex = eof_retrieve_text_event(eof_all_events_dialog[1].d1);	//Find the actual event, taking the display filter into account
		if(retval == 2)
		{	//User clicked Find
			if(realindex < eof_song->text_events)
			{	//If a valid text event is selected
				unsigned long eventpos = eof_get_text_event_pos(eof_song, realindex);

				eof_set_seek_position(eventpos + eof_av_delay);
				if(!(eof_song->text_event[realindex]->flags & EOF_EVENT_FLAG_FLOATING_POS))
				{	//If this text event is assigned to a beat marker
					eof_selected_beat = eof_song->text_event[realindex]->pos;
				}
				track = eof_song->text_event[realindex]->track;
				if((track != 0) && (track < eof_song->tracks))
				{	//If this is a track-specific event
					(void) eof_menu_track_selected_track_number(track, 1);	//Change to that track
				}
			}
			break;	//Break from loop
		}
		else if(retval == 4)
		{	//User clicked Copy to selected beat
			if(realindex < eof_song->text_events)
			{	//If a valid text event is selected
				flags = eof_song->text_event[realindex]->flags;
				track = eof_song->text_event[realindex]->track;
				if((track != 0) && (track < eof_song->tracks))
				{	//If the event being copied is a track-specific event
					track = eof_selected_track;	//Have the newly created event assigned to the active track
				}
				if(!undo_made)
				{	//If an undo state hasn't been made yet
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					undo_made = 1;
				}
				(void) eof_song_add_text_event(eof_song, eof_selected_beat, eof_song->text_event[realindex]->text, track, flags, 0);
				eof_sort_events(eof_song);
				eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current
				eof_render();
			}
		}
		else if(retval == 11)
		{	//User clicked Edit
			if(realindex < eof_song->text_events)
			{	//If a valid text event is selected
				eof_add_or_edit_text_event(eof_song->text_event[realindex], 0, &undo_made);	//Run logic to edit an existing event
				eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current
				eof_render();
			}
		}
		else if(retval == 12)
		{	//User clicked Delete
			if(realindex < eof_song->text_events)
			{	//If a valid text event is selected
				unsigned long c = eof_events_dialog_delete_events_count();
				if(!undo_made)
				{	//If an undo state hasn't been made yet
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					undo_made = 1;
				}
				eof_song_delete_text_event(eof_song, realindex);
				eof_sort_events(eof_song);
				if((realindex >= c) && (c > 0))
				{
					eof_all_events_dialog[1].d1--;
				}
				eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current
				eof_render();
			}
		}
		else
		{	//User clicked Done or hit escape to cancel
			break;
		}
	}
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(NULL);
	return 1;
}

int eof_menu_beat_events(void)
{
	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_events_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_events_dialog);
	if(eof_events_overridden_by_stored_MIDI_track(eof_song))
	{	//If there is a stored events track
		eof_events_dialog[5].dp = stored_event_track_notice;	//Add a warning to the dialog
	}
	else
	{
		eof_events_dialog[5].dp = no_notice;	//Otherwise remove the warning
	}
	if(eof_popup_dialog(eof_events_dialog, 0) == 4)
	{
	}
	eof_clear_input();
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(NULL);
	return 1;
}

int eof_menu_beat_clear_events(void)
{
	unsigned long i;
	if(eof_song->text_events == 0)
	{
		allegro_message("No events to clear!");
		return 1;
	}
	eof_clear_input();
	if(alert(NULL, "Erase all events?", NULL, "&Yes", "&No", 'y', 'n') == 1)
	{
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		(void) eof_song_resize_text_events(eof_song, 0);
		for(i = 0; i < eof_song->beats; i++)
		{	//For each beat
			eof_song->beat[i]->flags &= (~EOF_BEAT_FLAG_EVENTS);	//Clear the event flag
		}
	}
	eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current
	eof_clear_input();
	return 1;
}

int eof_menu_beat_clear_non_rs_events(void)
{
	unsigned long i;
	char undo_made = 0;

	if(eof_song->text_events == 0)
	{
		allegro_message("No events to clear!");
		return 1;
	}
	eof_clear_input();
	if(alert(NULL, "Erase all non Rocksmith events?", NULL, "&Yes", "&No", 'y', 'n') == 1)
	{
		for(i = eof_song->text_events; i > 0; i--)
		{	//For each text event, in reverse
			if(!(eof_song->text_event[i - 1]->flags & EOF_EVENT_FLAG_RS_PHRASE) && !(eof_song->text_event[i - 1]->flags & EOF_EVENT_FLAG_RS_SECTION) && !(eof_song->text_event[i - 1]->flags & EOF_EVENT_FLAG_RS_EVENT))
			{	//If this event isn't any of the Rocksmith text event types
				if(!undo_made)
				{	//If an undo state hasn't been made yet
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					undo_made = 1;
				}
				eof_song_delete_text_event(eof_song, i - 1);
			}
		}
	}
	eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current
	eof_sort_events(eof_song);
	return 1;
}

char * eof_events_list(int index, int * size)
{
	unsigned long ecount = 0, i;
	char trackname[26] = {0};
	char eventflags[30] = {0};

	for(i = 0; i < eof_song->text_events; i++)
	{
		if((eof_song->text_event[i]->flags & EOF_EVENT_FLAG_FLOATING_POS) || (eof_song->text_event[i]->pos != eof_selected_beat))
			continue;	//If this text event has a floating position or isn't on the selected beat, skip it
		if(ecount >= EOF_MAX_TEXT_EVENTS)
			continue;	//If an invalid number of events have been counted, skip counting any more

		if((eof_song->text_event[i]->track != 0) || (eof_song->text_event[i]->flags != 0))
		{	//If this event is track specific or has any flags set
			if((eof_song->text_event[i]->track != 0) && (eof_song->text_event[i]->track < eof_song->tracks))
			{	//If this is a track specific event
				(void) snprintf(trackname, sizeof(trackname) - 1, "%s", eof_song->track[eof_song->text_event[i]->track]->name);
				if(eof_song->text_event[i]->flags != 0)
				{	//If the event has any other flags set
					(void) strncat(trackname, ", ", sizeof(trackname) - strlen(trackname) - 1);	//Append a comma
				}
			}
			else
			{
				trackname[0] = '\0';	//Empty the string
			}
			(void) snprintf(eventflags, sizeof(eventflags) - 1, "(%s", trackname);	//Start building the full flags string
			if(eof_song->text_event[i]->flags & EOF_EVENT_FLAG_RS_PHRASE)
			{	//If the event is an RS phrase
				if(eof_song->text_event[i]->flags & EOF_EVENT_FLAG_RS_SOLO_PHRASE)
				{	//If the event is an RS solo phrase
					(void) strncat(eventflags, "So", sizeof(trackname) - strlen(trackname) - 1);	//Append So
				}
				(void) strncat(eventflags, "P", sizeof(trackname) - strlen(trackname) - 1);	//Append a P
			}
			if(eof_song->text_event[i]->flags & EOF_EVENT_FLAG_RS_SECTION)
			{
				(void) strncat(eventflags, "S", sizeof(trackname) - strlen(trackname) - 1);	//Append an S
			}
			if(eof_song->text_event[i]->flags & EOF_EVENT_FLAG_RS_EVENT)
			{
				(void) strncat(eventflags, "E", sizeof(trackname) - strlen(trackname) - 1);	//Append an E
			}
			(void) strncat(eventflags, ") ", sizeof(trackname) - strlen(trackname) - 1);
		}
		else
		{
			eventflags[0] = '\0';	//Empty the string
		}
		(void) snprintf(eof_event_list_text[ecount], sizeof(eof_event_list_text[ecount]) - 1, "%s%s", eventflags, eof_song->text_event[i]->text);
		ecount++;
	}
	switch(index)
	{
		case -1:
		{
			if(!size)
				return NULL;

			*size = ecount;
			(void) snprintf(eof_events_dialog_string, sizeof(eof_events_dialog_string) - 1, "Events (%lu)", ecount);
			if(ecount > 0)
			{	//If there is at least one event at this beat
				eof_events_dialog[3].flags = 0;	//Enable the Edit button
				eof_events_dialog[4].flags = 0;	//Enable the Delete button
			}
			else
			{
				eof_events_dialog[3].flags = D_DISABLED;
				eof_events_dialog[4].flags = D_DISABLED;
			}
			break;
		}
		default:
		{
			return eof_event_list_text[index];
		}
	}
	return NULL;
}

int eof_event_is_not_filtered_from_listing(unsigned long index)
{
	if(!eof_song || (index >= eof_song->text_events))
		return 0;	//Invalid parameter

	if(eof_all_events_dialog[5].flags & D_SELECTED)
	{	//"This track's events filter
		if(eof_song->text_event[index]->track != eof_selected_track)
		{	//If the event is not specific to the currently active track
			return 0;	//Event is filtered
		}
	}

	if(eof_all_events_dialog[6].flags & D_SELECTED)
	{	//"All events" filter
		return 1;	//Event is not filtered
	}

	if(eof_all_events_dialog[7].flags & D_SELECTED)
	{	//Display section events
		if(eof_is_section_marker(eof_song->text_event[index], 0))
		{	//If the text event's string or flags indicate a section marker (regardless of the event's associated track
			return 1;	//Event is not filtered
		}
	}
	else if(eof_all_events_dialog[8].flags & D_SELECTED)
	{	//Display Rocksmith sections
		if(eof_song->text_event[index]->flags & EOF_EVENT_FLAG_RS_SECTION)
		{	//If the event is marked as a Rocksmith section
			return 1;	//Event is not filtered
		}
	}
	else
	{	//Display Rocksmith events
		if(eof_song->text_event[index]->flags & EOF_EVENT_FLAG_RS_EVENT)
		{	//If the event is marked as a Rocksmith event
			return 1;	//Event is not filtered
		}
	}

	return 0;	//Event is filtered
}

char * eof_events_list_all(int index, int * size)
{
	char trackname[26] = {0};
	char eventflags[30] = {0};
	unsigned long x, count = 0, realindex;

	if(index < 0)
	{	//Signal to return the list count
		if(!size)
			return NULL;

		for(x = 0; x < eof_song->text_events; x++)
		{	//For each event
			if(eof_event_is_not_filtered_from_listing(x))
			{	//If this text event is to be displayed with the current filter settings
				count++;
			}
		}

		*size = count;
		(void) snprintf(eof_all_events_dialog_string, sizeof(eof_all_events_dialog_string) - 1, "All Events (%lu)", count);
	}
	else
	{	//Return the specified list item
		unsigned long eventpos;

		realindex = eof_retrieve_text_event(index);	//Get the actual event based on the current filter
		if(!(eof_song->text_event[realindex]->flags & EOF_EVENT_FLAG_FLOATING_POS))
		{	//If this text event is assigned to a beat marker
			if(eof_song->text_event[realindex]->pos >= eof_song->beats)
			{	//Something bad happened, repair the event
				eof_song->text_event[realindex]->pos = eof_song->beats - 1;	//Reset the text event to be at the last beat marker
			}
		}
		if((eof_song->text_event[realindex]->track != 0) || (eof_song->text_event[realindex]->flags != 0))
		{	//If this event is track specific or has any flags (other than just the floating position flag) set
			if((eof_song->text_event[realindex]->track != 0) && (eof_song->text_event[realindex]->track < eof_song->tracks))
			{	//If this is a track specific event
				(void) snprintf(trackname, sizeof(trackname) - 1, "%s", eof_song->track[eof_song->text_event[realindex]->track]->name);
				if(eof_song->text_event[realindex]->flags != 0)
				{	//If the event has any other flags set
					(void) strncat(trackname, ", ", sizeof(trackname) - strlen(trackname) - 1);	//Append a comma
				}
			}
			else
			{
				trackname[0] = '\0';	//Empty the string
			}
			(void) snprintf(eventflags, sizeof(eventflags) - 1, " %s", trackname);	//Start building the full flags string
			if(eof_song->text_event[realindex]->flags & EOF_EVENT_FLAG_RS_PHRASE)
			{	//If the event is an RS phrase
				if(eof_song->text_event[realindex]->flags & EOF_EVENT_FLAG_RS_SOLO_PHRASE)
				{	//If the event is an RS solo phrase
					(void) strncat(eventflags, "So", sizeof(trackname) - strlen(trackname) - 1);	//Append So
				}
				(void) strncat(eventflags, "P", sizeof(trackname) - strlen(trackname) - 1);	//Append a P
			}
			if(eof_song->text_event[realindex]->flags & EOF_EVENT_FLAG_RS_SECTION)
			{
				(void) strncat(eventflags, "S", sizeof(trackname) - strlen(trackname) - 1);	//Append an S
			}
			if(eof_song->text_event[realindex]->flags & EOF_EVENT_FLAG_RS_EVENT)
			{
				(void) strncat(eventflags, "E", sizeof(trackname) - strlen(trackname) - 1);	//Append an E
			}
			if(eof_song->text_event[realindex]->flags & EOF_EVENT_FLAG_FLOATING_POS)
			{
				(void) strncat(eventflags, "f", sizeof(trackname) - strlen(trackname) - 1);	//Append an f
			}
		}
		else
		{
			eventflags[0] = '\0';	//Empty the string
		}

		eventpos = eof_get_text_event_pos(eof_song, realindex);
		(void) snprintf(eof_event_list_text[index], sizeof(eof_event_list_text[index]) - 1, "(%02lu:%02lu.%03lu%s) %s", eventpos / 60000, (eventpos / 1000) % 60, eventpos % 1000, eventflags, eof_song->text_event[realindex]->text);
		return eof_event_list_text[index];
	}
	return NULL;
}

int eof_events_dialog_add(DIALOG * d)
{
	if(!d)
	{	//Satisfy Splint by checking value of d
		return D_O_K;
	}
	return eof_events_dialog_add_function(0);	//Call the add text event dialog, don't automatically check the RS phrase marker option unless the user left it checked from before
}

int eof_rocksmith_phrase_dialog_add(void)
{
	char undo_made = 0;
	int contained_section_event = eof_song->beat[eof_selected_beat]->contained_section_event;

	if(contained_section_event >= 0)
	{	//If the selected beat already has an RS phrase defined
		eof_add_or_edit_text_event(eof_song->text_event[contained_section_event], 0, &undo_made);	//Run logic to edit an existing event
		return D_O_K;
	}

	//Launch the add text event dialog to add a new event, automatically checking the RS phrase marker option
	return eof_events_dialog_add_function(EOF_EVENT_FLAG_RS_PHRASE);
}

int eof_menu_beat_add_section(void)
{
	unsigned long ctr;
	char *ptr = NULL;
	EOF_TEXT_EVENT *ep = NULL;
	char text[EOF_TEXT_EVENT_LENGTH + 1] = {0};

	if(!eof_song)
		return D_O_K;

	//Determine if the selected beat already has an existing section marker
	for(ctr = 0; ctr < eof_song->text_events; ctr++)
	{	//For each text event in the project
		if(!(eof_song->text_event[ctr]->flags & EOF_EVENT_FLAG_FLOATING_POS))
		{	//If this text event is assigned to a beat marker
			if(eof_song->text_event[ctr]->pos == eof_selected_beat)
			{	//If the text event is assigned to the selected beat
				if(!eof_song->text_event[ctr]->track || (eof_song->text_event[ctr]->track == eof_selected_track))
				{	//If the text event has no associated track or is specific to the active track
					ptr = strcasestr_spec(eof_song->text_event[ctr]->text, "section ");	//Find this substring within the text event
					if(ptr)
					{	//If it exists
						ep = eof_song->text_event[ctr];	//Store this event's pointer
						break;
					}
				}
			}
		}
	}

	//Prepare the dialog accordingly
	if(ep && ptr)
	{	//If the selected beat already had a section event in scope
		eof_section_add_dialog[0].dp = eof_section_add_dialog_string2;	//Change the dialog title to "Edit section"
		strncpy(eof_etext, ptr, sizeof(eof_etext) - 1);		//Copy the portion of the text event that came after "section "
		if(eof_etext[strlen(eof_etext) - 1] == ']')
		{	//If that included a trailing closing bracket
			eof_etext[strlen(eof_etext) - 1] = '\0';	//Truncate it from the end of the string
		}
	}
	else
	{
		eof_section_add_dialog[0].dp = eof_section_add_dialog_string1;	//Change the dialog title to "Add section"
		eof_etext[0] = '\0';	//Empty the input field
	}

	//Process user input
	eof_color_dialog(eof_section_add_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_section_add_dialog);
	if(eof_popup_dialog(eof_section_add_dialog, 2) != 3)
		return D_O_K;	//If the user didn't click OK, return immediately

	if(strchr(eof_etext, '[') || strchr(eof_etext, ']'))
	{
		allegro_message("Section names cannot include opening or closing brackets [ ]");
		return D_O_K;
	}
	(void) snprintf(text, sizeof(text) - 1, "[section %s]", eof_etext);	//Format the section string
	if(!ep || strcmp(ep->text, text))
	{	//If there wasn't already a section event on this beat, or if the user altered its name
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	}
	if(!ep)
	{	//If there wasn't already a section event on this beat, add one now
		(void) eof_song_add_text_event(eof_song, eof_selected_beat, text, 0, 0, 0);	//Add it as a global text event
		eof_sort_events(eof_song);
	}
	else
	{	//Otherwise edit the existing event
		(void) strncpy(ep->text, text, sizeof(text) - 1);
	}
	eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current

	return D_O_K;
}

int eof_events_dialog_add_function(unsigned long function)
{
	int i;
	char undo_made = 0;

	eof_add_or_edit_text_event(NULL, function, &undo_made);	//Run logic to add a new event
	eof_render();
	if(!function)
	{	//If this function was launched from within Beat>Events
		(void) dialog_message(eof_events_dialog, MSG_DRAW, 0, &i);	//Redraw that menu
		return D_REDRAW;
	}
	return D_O_K;
}

int eof_events_dialog_edit(DIALOG * d)
{
	unsigned long i, event = 0;
	int ecount = 0, junk = 0;
	char found = 0;
	char undo_made = 0;

	if(!d)
	{	//Satisfy Splint by checking value of d
		return D_O_K;
	}

	/* find the event */
	for(i = 0; i < eof_song->text_events; i++)
	{
		if(!(eof_song->text_event[i]->flags & EOF_EVENT_FLAG_FLOATING_POS))
		{	//If this text event is assigned to a beat marker
			if(eof_song->text_event[i]->pos != eof_selected_beat)
				continue;	//If this beat isn't the selected beat, skip it

			/* if we've reached the item that is selected, edit it */
			if(eof_events_dialog[1].d1 == ecount)
			{
				event = i;
				found = 1;
				break;
			}

			/* go to next event */
			else
			{
				ecount++;
			}
		}
	}
	if(found && (event < eof_song->text_events))
	{	//If the selected event was found
		eof_add_or_edit_text_event(eof_song->text_event[event], 0, &undo_made);	//Run logic to edit an existing event
		eof_render();
		(void) dialog_message(eof_events_dialog, MSG_DRAW, 0, &junk);
	}
	return D_O_K;
}

int eof_events_add_check_proc(int msg, DIALOG *d, int c)
{
	int junk;
	int old_rs_phrase_state, new_rs_phrase_state;

	if(msg != MSG_CLICK)
		return d_agup_check_proc(msg, d, c);	//If this isn't a click message, allow the input to be processed

	old_rs_phrase_state = eof_events_add_dialog[4].flags & D_SELECTED;	//Record this checkbox's status
	(void) d_agup_check_proc(msg, d, c);	//Allow the input to be processed
	new_rs_phrase_state = eof_events_add_dialog[4].flags & D_SELECTED;	//Record this checkbox's status
	if(new_rs_phrase_state != old_rs_phrase_state)
	{	//If the user changed the Rocksmith phrase marker checkbox
		if(new_rs_phrase_state == D_SELECTED)
		{	//If the checkbox has been checked
			eof_events_add_dialog[5].flags = 0;	//Enable the RS solo phrase checkbox
		}
		else
		{	//The checkbox has been cleared
			eof_events_add_dialog[5].flags = D_DISABLED;	//Disable the RS solo phrase checkbox
		}
		(void) dialog_message(eof_events_add_dialog, MSG_START, 0, &junk);	//Re-initialize the dialog
		(void) dialog_message(eof_events_add_dialog, MSG_DRAW, 0, &junk);	//Redraw dialog
	}
	return D_REDRAW;
}

void eof_add_or_edit_text_event(EOF_TEXT_EVENT *ptr, unsigned long flags, char *undo_made)
{
	EOF_TEXT_EVENT temp = {{0}, 0, 0, 0, 0, 0};
	unsigned long newflags = 0;
	unsigned long newtrack = 0;

	if(!ptr)
	{	//If a new event is to be added
		ptr = &temp;
		ptr->flags = flags;
		eof_events_add_dialog[0].dp = eof_events_add_dialog_string1;	//Update the dialog window title to reflect that a new event is being added
	}
	else
	{	//An existing event is being edited
		if(ptr->flags & EOF_EVENT_FLAG_FLOATING_POS)
		{	//If this is a floating text event
			eof_add_or_edit_floating_text_event(ptr, flags, undo_made);	//Invoke the floating text event add/edit dialog instead
			return;
		}
		newflags = ptr->flags;
		eof_events_add_dialog[0].dp = eof_events_add_dialog_string2;	//Update the dialog window title to reflect that an event is being edited
		if(ptr->track == eof_selected_track)
		{	//If this event is specific to this track
			eof_events_add_dialog[3].flags = D_SELECTED;	//Set the checkbox specifying the event is track specific
		}
		else
		{	//Otherwise clear the checkbox
			eof_events_add_dialog[3].flags = 0;
		}
	}

	if(!undo_made)
	{
		return;	//Invalid parameter
	}
	if((ptr->track != 0) && (ptr->track != eof_selected_track))
	{	//If this is a track specific event, and it doesn't belong to the active track
		return;	//Don't allow it to be edited here
	}

	//Initialize the dialog
	(void) snprintf(eof_events_add_dialog_string, sizeof(eof_events_add_dialog_string) - 1, "Specific to %s", eof_song->track[eof_selected_track]->name);

	if(ptr->flags & EOF_EVENT_FLAG_RS_PHRASE)
	{	//If the calling function wanted to automatically enable the "Rocksmith phrase marker" checkbox, or the event being edited already has this flag
		eof_events_add_dialog[4].flags = D_SELECTED;
		if(ptr->flags & EOF_EVENT_FLAG_RS_SOLO_PHRASE)
		{	//If the calling function wanted to automatically enable the "Rocksmith solo phrase" checkbox, or the event being edited already has this flag
			eof_events_add_dialog[5].flags = D_SELECTED;
		}
		else
		{	//Otherwise clear the RS solo phrase checkbox
			eof_events_add_dialog[5].flags = 0;
		}
	}
	else
	{	//Otherwise clear the RS phrase flags
		eof_events_add_dialog[4].flags = 0;
		eof_events_add_dialog[5].flags = D_DISABLED;
	}
	if(ptr->flags & EOF_EVENT_FLAG_RS_SECTION)
	{	//If the calling function wanted to automatically enable the "Rocksmith section marker" checkbox, or the event being edited already has this flag
		eof_events_add_dialog[6].flags = D_SELECTED;
	}
	else
	{	//Otherwise clear it
		eof_events_add_dialog[6].flags = 0;
	}
	if(ptr->flags & EOF_EVENT_FLAG_RS_EVENT)
	{	//If the calling function wanted to automatically enable the "Rocksmith event marker" checkbox, or the event being edited already has this flag
		eof_events_add_dialog[7].flags = D_SELECTED;
	}
	else
	{	//Otherwise clear it
		eof_events_add_dialog[7].flags = 0;
	}
	(void) ustrcpy(eof_etext, ptr->text);

	//Run and process the dialog results
	eof_color_dialog(eof_events_add_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_events_add_dialog);
	if(eof_popup_dialog(eof_events_add_dialog, 2) == 8)
	{	//User clicked OK
		char *effective_text = eof_etext;	//By default, use the user-input string
		char *rssectionname;

		if(eof_events_add_dialog[4].flags & D_SELECTED)
		{	//User opted to make this a Rocksmith phrase marker
			newflags |= EOF_EVENT_FLAG_RS_PHRASE;
			if(eof_events_add_dialog[5].flags & D_SELECTED)
			{	//User opted to make this a Rocksmith solo phrase
				newflags |= EOF_EVENT_FLAG_RS_SOLO_PHRASE;
			}
		}
		if(eof_events_add_dialog[7].flags & D_SELECTED)
		{	//User opted to make this a Rocksmith event marker
			if(!eof_rs_event_text_valid(eof_etext))
			{	//If this isn't a valid Rocksmith event name
				allegro_message("Warning:  This is not a valid Rocksmith event.  Please edit it appropriately or remove it and re-add it using Beat>Place Rocksmith Event");
			}
			newflags |= EOF_EVENT_FLAG_RS_EVENT;
		}
		if(eof_events_add_dialog[6].flags & D_SELECTED)
		{	//User opted to make this a Rocksmith section marker
			newflags |= EOF_EVENT_FLAG_RS_SECTION;
			rssectionname = eof_rs_section_text_valid(eof_etext);	//Determine whether this is a valid Rocksmith section name
			if(rssectionname && ustrcmp(rssectionname, eof_etext) && !(newflags & EOF_EVENT_FLAG_RS_PHRASE) && !(newflags & EOF_EVENT_FLAG_RS_EVENT))
			{	//If this doesn't match a valid RS native name (case sensitive), but matches a valid Rocksmith section native/display name (case insensitive) and isn't marked as a RS phrase/event
				effective_text = rssectionname;	//Use the matching RS section's native name instead of the user-supplied text
			}
			else if(!rssectionname || ustrcmp(rssectionname, eof_etext))
			{	//Otherwise if this isn't a valid Rocksmith native section name (case sensitive), the user will have to resolve the conflict manually
				allegro_message("Warning:  This is not a valid Rocksmith section.\nPlease edit it appropriately or remove it and re-add it using Beat>Rocksmith>Place Rocksmith Section");
			}
		}
		if(eof_events_add_dialog[3].flags & D_SELECTED)
		{	//User opted to make this a track specific event
			if((eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT) && (newflags & EOF_EVENT_FLAG_RS_PHRASE))
			{	//If the user is tried to add a track specific RS phrase marker in a non pro guitar/bass track
				allegro_message("You cannot add a track-specific Rocksmith phrase marker in a non pro guitar/bass track");
				return;
			}
			newtrack = eof_selected_track;
		}

		if(eof_check_string(eof_etext) && (ustrcmp(ptr->text, eof_etext) || (newtrack != ptr->track) || (newflags != ptr->flags)))
		{	//If the user entered any nonspace characters, and (in the case of editing an existing event) any of the fields were altered
			if(*undo_made == 0)
			{	//If an undo state hasn't been made yet
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				*undo_made = 1;
			}
			if(ptr == &temp)
			{	//If a new event is to be added
				(void) eof_song_add_text_event(eof_song, eof_selected_beat, effective_text, newtrack, newflags, 0);	//Add it
				eof_sort_events(eof_song);
			}
			else
			{	//Otherwise edit the existing event
				(void) ustrcpy(ptr->text, effective_text);
				ptr->track = newtrack;
				ptr->flags = newflags;
			}
			eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current
		}

		if(newflags && EOF_EVENT_FLAG_RS_PHRASE)
		{	//If the event that was created/edited is an RS phrase, set all matching RS phrases of matching name and scope to have the same RS solo status
			int status = (newflags & EOF_EVENT_FLAG_RS_SOLO_PHRASE);

			if(eof_events_set_rs_solo_phrase_status(eof_etext, newtrack, status, undo_made))
			{	//If any such events were edited to match
				allegro_message("All instances of a Rocksmith phrase use the same solo status.  One or more matching phrases in this event's scope had solo status %s to match.", (status ? "added" : "removed"));
			}
		}
	}//User clicked OK
}

void eof_add_or_edit_floating_text_event_at_timestamp(EOF_TEXT_EVENT *ptr, unsigned long timestamp, unsigned long flags, char *undo_made)
{
	EOF_TEXT_EVENT temp = {{0}, 0, 0, 0, 0, 0};
	unsigned long newtrack = 0, newpos = 0;

	if(!ptr)
	{	//If a new event is to be added
		ptr = &temp;
		ptr->flags = flags | EOF_EVENT_FLAG_FLOATING_POS;	//Use any defined flags (ie. track specific flag) and add the flag to define it as a floating text event
		eof_floating_events_add_dialog[0].dp = eof_events_add_floating_dialog_string1;	//Update the dialog window title to reflect that a new event is being added
	}
	else
	{	//An existing event is being edited
		if(!(ptr->flags & EOF_EVENT_FLAG_FLOATING_POS))		//If the event being edited is not a floating text event
			return;											//Cancel

		eof_floating_events_add_dialog[0].dp = eof_events_add_floating_dialog_string2;	//Update the dialog window title to reflect that an event is being edited
		if(ptr->track == eof_selected_track)
		{	//If this event is specific to this track
			eof_floating_events_add_dialog[5].flags = D_SELECTED;	//Set the checkbox specifying the event is track specific
		}
		else
		{	//Otherwise clear the checkbox
			eof_floating_events_add_dialog[5].flags = 0;
		}
	}

	if(!undo_made)
	{
		return;	//Invalid parameter
	}
	if((ptr->track != 0) && (ptr->track != eof_selected_track))
	{	//If this is a track specific event, and it doesn't belong to the active track
		return;	//Don't allow it to be edited here
	}

	//Initialize the dialog
	(void) snprintf(eof_events_add_dialog_string, sizeof(eof_events_add_dialog_string) - 1, "Specific to %s", eof_song->track[eof_selected_track]->name);
	if(ptr != &temp)
	{	//If an existing floating text event is being edited
		(void) snprintf(eof_etext2, sizeof(eof_etext2) - 1, "%lu", ptr->pos);	//Initialize the event's time field with the event's position
	}
	else
	{
		(void) snprintf(eof_etext2, sizeof(eof_etext2) - 1, "%lu", timestamp);	//Initialize the event's time field with the specified position
	}
	(void) ustrcpy(eof_etext, ptr->text);

	//Run and process the dialog results
	eof_color_dialog(eof_floating_events_add_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_floating_events_add_dialog);
	if(eof_popup_dialog(eof_floating_events_add_dialog, 2) == 6)
	{	//User clicked OK
		char *effective_text = eof_etext;	//By default, use the user-input string

		if(eof_floating_events_add_dialog[5].flags & D_SELECTED)
		{	//User opted to make this a track specific event
			newtrack = eof_selected_track;
		}
		newpos = atol(eof_etext2);

		if(eof_check_string(eof_etext) && (ustrcmp(ptr->text, eof_etext) || (newtrack != ptr->track) || (newpos != ptr->pos)))
		{	//If the user entered any nonspace characters, and (in the case of editing an existing event) any of the fields were altered
			if(*undo_made == 0)
			{	//If an undo state hasn't been made yet
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				*undo_made = 1;
			}
			if(ptr == &temp)
			{	//If a new floating event is to be added
				(void) eof_song_add_text_event(eof_song, newpos, effective_text, newtrack, EOF_EVENT_FLAG_FLOATING_POS, 0);	//Add it
				eof_sort_events(eof_song);
			}
			else
			{	//Otherwise edit the existing event
				(void) ustrcpy(ptr->text, effective_text);
				ptr->pos = newpos;
				ptr->track = newtrack;
			}
			eof_beat_stats_cached = 0;		//Mark the cached beat stats as not current
		}
	}//User clicked OK
}

void eof_add_or_edit_floating_text_event(EOF_TEXT_EVENT *ptr, unsigned long flags, char *undo_made)
{
	eof_add_or_edit_floating_text_event_at_timestamp(ptr, eof_music_pos.value - eof_av_delay, flags, undo_made);
}

unsigned long eof_events_dialog_delete_events_count(void)
{
	unsigned long i, count = 0;

	for(i = 0; i < eof_song->text_events; i++)
	{
		if(!(eof_song->text_event[i]->flags & EOF_EVENT_FLAG_FLOATING_POS))
		{	//If this text event is assigned to a beat marker
			if(eof_song->text_event[i]->pos == eof_selected_beat)
			{
				count++;
			}
		}
	}
	return count;
}

int eof_events_dialog_delete(DIALOG * d)
{
	unsigned long c, i;
	int ecount = 0, junk = 0;

	if(!d)
	{	//Satisfy Splint by checking value of d
		return D_O_K;
	}
	if((eof_song->text_events == 0) || (eof_events_dialog[1].d1 < 0))
	{
		return D_O_K;
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	for(i = 0; i < eof_song->text_events; i++)
	{
		if(!(eof_song->text_event[i]->flags & EOF_EVENT_FLAG_FLOATING_POS))
		{	//If this text event is assigned to a beat marker
			if(eof_song->text_event[i]->pos != eof_selected_beat)
				continue;	//If this beat isn't the selected beat, skip it

			/* if we've reached the item that is selected, delete it */
			if(eof_events_dialog[1].d1 == ecount)
			{
				/* remove the text event and exit */
				eof_song_delete_text_event(eof_song, i);
				eof_sort_events(eof_song);

				/* remove flag if no more events tied to this beat */
				c = eof_events_dialog_delete_events_count();
				if(((unsigned long)eof_events_dialog[1].d1 >= c) && (c > 0))
				{
					eof_events_dialog[1].d1--;
				}
				(void) dialog_message(eof_events_dialog, MSG_DRAW, 0, &junk);
				eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current
				return D_REDRAW;
			}

			/* go to next event */
			ecount++;
		}
	}
	return D_O_K;
}

int eof_menu_beat_add(void)
{
	unsigned long i;

	if(!eof_song)
		return 1;
	if(eof_song->tags->tempo_map_locked)	//If the chart's tempo map is locked
		return 1;							//Return without making changes
	if(!eof_beat_num_valid(eof_song, eof_find_next_anchor(eof_song, eof_selected_beat)))
	{	//If there are no anchors after the selected beat
		return 1;	//Return without making changes
	}

	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	if(eof_song_add_beat(eof_song) != NULL)
	{	//Only if the new beat structure was successfully created
		for(i = eof_song->beats - 1; i > eof_selected_beat; i--)
		{	//For each beat after the selected beat, in reverse order
			memcpy(eof_song->beat[i], eof_song->beat[i - 1], sizeof(EOF_BEAT_MARKER));
		}
		eof_song->beat[eof_selected_beat + 1]->flags = 0;
		eof_realign_beats(eof_song, eof_selected_beat + 1);
		eof_move_text_events(eof_song, eof_selected_beat + 1, 1, 1);
		eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current
		return 1;
	}

	return -1;	//Otherwise return error
}

void eof_rebuild_trainer_strings(void)
{
	int relevant_track = eof_selected_track;	//In RB3, pro guitar/bass training events are only stored in the 17 fret track

	if(!eof_song)
		return;	//Return without rebuilding string tunings if there is an error
	if(eof_selected_track == EOF_TRACK_PRO_GUITAR_B)
		return;	//The bonus pro guitar track is not compatible with RB3

	if(eof_selected_track == EOF_TRACK_PRO_GUITAR_22)
	{	//Ensure that training events get written to the 17 fret version track
		relevant_track = EOF_TRACK_PRO_GUITAR;
	}
	else if(eof_selected_track == EOF_TRACK_PRO_BASS_22)
	{
		relevant_track = EOF_TRACK_PRO_BASS;
	}
	//Build the trainer strings
	if((eof_selected_track == EOF_TRACK_PRO_GUITAR) || (eof_selected_track == EOF_TRACK_PRO_GUITAR_22))
	{	//A pro guitar track is active
		(void) snprintf(eof_etext2, sizeof(eof_etext2) - 1, "[begin_pg song_trainer_pg_%s]", eof_trainer_string);
		(void) snprintf(eof_etext3, sizeof(eof_etext3) - 1, "[pg_norm song_trainer_pg_%s]", eof_trainer_string);
		(void) snprintf(eof_etext4, sizeof(eof_etext4) - 1, "[end_pg song_trainer_pg_%s]", eof_trainer_string);
	}
	else if((eof_selected_track == EOF_TRACK_PRO_BASS) || (eof_selected_track == EOF_TRACK_PRO_BASS_22))
	{	//A pro bass track is active
		(void) snprintf(eof_etext2, sizeof(eof_etext2) - 1, "[begin_pb song_trainer_pb_%s]", eof_trainer_string);
		(void) snprintf(eof_etext3, sizeof(eof_etext3) - 1, "[pb_norm song_trainer_pb_%s]", eof_trainer_string);
		(void) snprintf(eof_etext4, sizeof(eof_etext4) - 1, "[end_pb song_trainer_pb_%s]", eof_trainer_string);
	}

	//Update the checkboxes to indicate which trainer strings are already defined
	if(eof_song_contains_event(eof_song, eof_etext2, relevant_track, 0xFFFF, 1))
	{	//If this training event is already defined in the active track
		eof_place_trainer_dialog[3].flags = D_SELECTED | D_DISABLED;	//Check this box
	}
	else
	{
		eof_place_trainer_dialog[3].flags = D_DISABLED;		//Otherwise clear this box
	}
	if(eof_song_contains_event(eof_song, eof_etext3, relevant_track, 0xFFFF, 1))
	{	//If this training event is already defined in the active track
		eof_place_trainer_dialog[5].flags = D_SELECTED | D_DISABLED;	//Check this box
	}
	else
	{
		eof_place_trainer_dialog[5].flags = D_DISABLED;		//Otherwise clear this box
	}
	if(eof_song_contains_event(eof_song, eof_etext4, relevant_track, 0xFFFF, 1))
	{	//If this training event is already defined in the active track
		eof_place_trainer_dialog[7].flags = D_SELECTED | D_DISABLED;	//Check this box
	}
	else
	{
		eof_place_trainer_dialog[7].flags = D_DISABLED;		//Otherwise clear this box
	}
}

int eof_menu_beat_trainer_event(void)
{
	int relevant_track = eof_selected_track;	//In RB3, pro guitar/bass training events are only stored in the 17 fret track
	char *selected_string = NULL;

	if(eof_selected_track == EOF_TRACK_PRO_GUITAR_B)
		return 1;	//The bonus pro guitar track is not compatible with RB3

	eof_color_dialog(eof_place_trainer_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_place_trainer_dialog);
	eof_place_trainer_dialog[3].flags = D_DISABLED;	//Clear and disable the checkboxes
	eof_place_trainer_dialog[5].flags = D_DISABLED;
	eof_place_trainer_dialog[7].flags = D_DISABLED;
	if(eof_trainer_string[0] == '\0')
	{	//If the trainer number field is empty
		eof_place_trainer_dialog[9].flags = D_DISABLED;	//Disabled the OK button
	}
	eof_place_trainer_dialog[4].flags = D_SELECTED;	//Set the first radio button and clear the others
	eof_place_trainer_dialog[6].flags = 0;
	eof_place_trainer_dialog[8].flags = 0;

	if(eof_selected_track == EOF_TRACK_PRO_GUITAR_22)
	{	//Ensure that training events get written to the 17 fret version track
		relevant_track = EOF_TRACK_PRO_GUITAR;
	}
	else if(eof_selected_track == EOF_TRACK_PRO_BASS_22)
	{
		relevant_track = EOF_TRACK_PRO_BASS;
	}
	eof_rebuild_trainer_strings();

	if(eof_popup_dialog(eof_place_trainer_dialog, 2) != 9)
		return 1;	//If the user didn't click OK, return immediately

	if(eof_place_trainer_dialog[4].flags == D_SELECTED)
	{	//User selected the begin song trainer string
		selected_string = eof_etext2;
	}
	else if(eof_place_trainer_dialog[6].flags == D_SELECTED)
	{	//User selected the norm song trainer string
		selected_string = eof_etext3;
	}
	else
	{	//User selected the end song trainer string
		selected_string = eof_etext4;
	}

	if(eof_song_contains_event(eof_song, selected_string, relevant_track, 0xFFFF, 1))
	{	//If this training event is already defined in the active track
		eof_clear_input();
		if(alert(NULL, "Warning:  This text event already exists in this track.  Continue?", NULL, "&Yes", "&No", 'y', 'n') != 1)
		{	//If the user does not opt to place the duplicate text event
			return 0;
		}
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	(void) eof_song_add_text_event(eof_song, eof_selected_beat, selected_string, relevant_track, 0, 0);	//Add the chosen text event to the selected beat
	eof_sort_events(eof_song);

	return 1;
}

int eof_edit_trainer_proc(int msg, DIALOG *d, int c)
{
	int i;
	char * string = NULL;
	unsigned key_list[32] = {KEY_BACKSPACE, KEY_DEL, KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN, KEY_ESC, KEY_ENTER};
	int match = 0;
	int retval;
	unsigned c2 = (unsigned)c;	//Force cast this to unsigned because Splint is incapable of avoiding a false positive detecting it as negative despite assertions proving otherwise

	if(!d)	//If this pointer is NULL for any reason
		return d_agup_edit_proc(msg, d, c);	//Allow the input character to be returned
	if((msg != MSG_CHAR) && (msg != MSG_UCHAR))
		return d_agup_edit_proc(msg, d, c);	//If this isn't a character input message, allow the input character to be returned

	for(i = 0; i < 8; i++)
	{	//See if any default accepted input characters were given
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

	retval = d_agup_edit_proc(msg, d, c);	//Allow the input character to be processed
	if(!eof_song || (eof_selected_track >= eof_song->tracks))
		return retval;	//Return without redrawing string tunings if there is an error

	//Update various dialog objects
	eof_rebuild_trainer_strings();

	if(eof_trainer_string[0] == '\0')
	{	//If the trainer number field is empty
		eof_place_trainer_dialog[9].flags = D_DISABLED;	//Disable the OK button
	}
	else
	{
		eof_place_trainer_dialog[9].flags = D_EXIT;	//Enable the OK button and allow it to close the dialog menu
	}

	return D_REDRAW;	//Have Allegro redraw the entire dialog menu, because it won't update the radio button strings as needed otherwise
}

int eof_all_events_radio_proc(int msg, DIALOG *d, int c)
{
	static uintptr_t previous_option = 6;	//By default, eof_all_events_dialog[6] (all events) is selected
	uintptr_t selected_option;
	int junk;

	if(msg != MSG_CLICK)
		return d_agup_radio_proc(msg, d, c);	//If this isn't a click message, allow the input to be processed

	if(!d)	//If this pointer is NULL for any reason
		return d_agup_radio_proc(msg, d, c);	//Allow the input to be processed

	selected_option = (uintptr_t)d->dp2;	//Find out which radio button was clicked on

	if(selected_option != previous_option)
	{	//If the event display filter changed, have the event list redrawn
		eof_all_events_dialog[6].flags = eof_all_events_dialog[7].flags = eof_all_events_dialog[8].flags = eof_all_events_dialog[9].flags = 0;	//Clear all radio buttons
		eof_all_events_dialog[selected_option].flags = D_SELECTED;			//Re-select the radio button that was just clicked on
		eof_all_events_dialog[1].d2 = 0;									//Select first list item, since if it's too high, it will prevent the newly-selected filtered list from displaying
		previous_option = selected_option;
		(void) d_agup_radio_proc(msg, d, c);	//Allow the input to be processed
		(void) dialog_message(eof_all_events_dialog, MSG_START, 0, &junk);	//Re-initialize the dialog
		(void) dialog_message(eof_all_events_dialog, MSG_DRAW, 0, &junk);	//Redraw dialog
		return D_REDRAW;
	}

	return d_agup_radio_proc(msg, d, c);	//Allow the input to be processed
}

int eof_all_events_check_proc(int msg, DIALOG *d, int c)
{
	int junk;

	if(msg != MSG_CLICK)
		return d_agup_check_proc(msg, d, c);	//If this isn't a click message, allow the input to be processed

	eof_all_events_dialog[1].d2 = 0;		//Select first list item, since if it's too high, it will prevent the newly-selected filtered list from displaying
	(void) d_agup_check_proc(msg, d, c);	//Allow the input to be processed
	(void) dialog_message(eof_all_events_dialog, MSG_START, 0, &junk);	//Re-initialize the dialog
	(void) dialog_message(eof_all_events_dialog, MSG_DRAW, 0, &junk);	//Redraw dialog
	return D_REDRAW;
}

unsigned long eof_retrieve_text_event(unsigned long index)
{
	unsigned long ctr, count;

	for(ctr = 0, count = 0; ctr < eof_song->text_events; ctr++)
	{	//For each text event in the active project
		if(eof_event_is_not_filtered_from_listing(ctr))
		{	//If this text event is to be displayed with the current filter settings
			if(count == index)
			{	//If the requested entry has been found
				return ctr;
			}
			count++;
		}
	}

	return 0;	//The requested entry was not retrievable
}

int eof_menu_beat_double_tempo(void)
{
	if(!eof_song)
		return 1;
	if(eof_song->tags->tempo_map_locked)	//If the chart's tempo map is locked
		return 1;							//Return without making changes

	eof_double_tempo(eof_song, eof_selected_beat, NULL);
	return 1;
}

int eof_menu_beat_halve_tempo(void)
{
	if(!eof_song)
		return 1;
	if(eof_song->tags->tempo_map_locked)	//If the chart's tempo map is locked
		return 1;							//Return without making changes

	(void) eof_halve_tempo(eof_song, eof_selected_beat, NULL);
	return 1;
}

int eof_menu_beat_double_tempo_all(void)
{
	char undo_made = 0;
	unsigned long i;

	if(!eof_song)
		return 1;
	if(eof_song->tags->tempo_map_locked)	//If the chart's tempo map is locked
		return 1;							//Return without making changes

	for(i = 0; i < eof_song->beats; i++)
	{	//For each beat
		if(!i || (eof_song->beat[i]->ppqn != eof_song->beat[i - 1]->ppqn))
		{	//If this is the first beat or this beat is a tempo change
			eof_double_tempo(eof_song, i, &undo_made);	//Double its tempo
		}
	}

	return 1;
}

int eof_menu_beat_halve_tempo_all(void)
{
	char undo_made = 0;
	unsigned long i;

	if(!eof_song)
		return 1;
	if(eof_song->tags->tempo_map_locked)	//If the chart's tempo map is locked
		return 1;							//Return without making changes

	for(i = 0; i < eof_song->beats; i++)
	{	//For each beat
		if(!i || (eof_song->beat[i]->ppqn != eof_song->beat[i - 1]->ppqn))
		{	//If this is the first beat or this beat is a tempo change
			(void) eof_halve_tempo(eof_song, i, &undo_made);	//Halve its tempo
		}
	}

	return 1;
}

int eof_menu_beat_set_RBN_tempos(void)
{
	char undo_made = 0, changed;
	unsigned long loop_ctr = 0;
	unsigned long i;

	eof_log("eof_menu_beat_set_RBN_tempos() entered", 1);

	if(!eof_song)
		return 1;
	if(eof_song->tags->tempo_map_locked)	//If the chart's tempo map is locked
		return 1;							//Return without making changes

	for(loop_ctr = 0; loop_ctr < 5; loop_ctr++)
	{	//Only allow this loop to run 5 times in a row, to ensure it cannot run endlessly
		changed = 0;	//Reset this condition
		for(i = 0; i < eof_song->beats; i++)
		{	//For each beat
			if(eof_song->beat[i]->ppqn < 200000)
			{	//If this beat is > 300BPM
				(void) eof_halve_tempo(eof_song, i, &undo_made);
				changed = 1;		//Track that a change was made during this loop
			}
			else if(eof_song->beat[i]->ppqn > 1500000)
			{	//If this beat's tempo is < 40BPM
				eof_double_tempo(eof_song, i, &undo_made);
				changed = 1;		//Track that a change was made during this loop
			}
		}
		if(changed == 0)
		{	//If there were no changes necessary during this loop
			break;
		}
	}
	if(undo_made)
	{	//If any changes were made
		allegro_message("Tempos adjusted");
	}
	for(i = 0; i < eof_song->beats; i++)
	{	//For each beat
		if((eof_song->beat[i]->ppqn < 200000) || (eof_song->beat[i]->ppqn > 1500000))
		{	//If this beat's tempo is > 300BPM or < 40BPM
			eof_selected_beat = i;
			eof_seek_and_render_position(eof_selected_track, eof_note_type, eof_song->beat[i]->pos);
			allegro_message("Warning:  This beat has a tempo that must be manually corrected.");
			return 1;
		}
	}
	if(!undo_made)
	{	//If no changes were made
		allegro_message("No tempo adjustments necessary");
	}
	return 1;
}

int eof_menu_beat_adjust_bpm(double amount)
{
	unsigned long targetbeat;
	double newbpm;
	unsigned long newppqn, oldppqn, ctr;

	if(!eof_song)
		return 1;
	if(eof_song->tags->tempo_map_locked)
		return 1;	//Return without changing anything if the chart's tempo map is locked

	targetbeat = eof_get_beat(eof_song, eof_music_pos.value - eof_av_delay);	//Identify the beat at or before the seek position
	if(!eof_beat_num_valid(eof_song, targetbeat))
	{	//If the seek position is outside the scope of the chart
		targetbeat = 0;	//Identify the first beat as the target
	}
	oldppqn = eof_song->beat[targetbeat]->ppqn;
	if(eof_input_mode != EOF_INPUT_FEEDBACK)
	{	//If Feedback input method is not in effect, identify the anchor at or before the seek position
		while((targetbeat > 0) && (eof_song->beat[targetbeat - 1]->ppqn == oldppqn) && !(eof_song->beat[targetbeat]->flags & EOF_BEAT_FLAG_ANCHOR))
		{	//While the target is not a tempo change and is not an anchor
			targetbeat--;	//Move the target back one beat
		}
	}
	newbpm = (60000000.0 / eof_song->beat[targetbeat]->ppqn) + amount;	//Determine the new tempo for the target
	newppqn = (60000000.0 / newbpm) + 0.5;	//Determine the new ppqn for the tempo, rounded to nearest integer value
	if(newbpm < 0.1)
		return 1;	//A tempo < 0.1BPM is not allowed
	eof_prepare_undo(EOF_UNDO_TYPE_TEMPO_ADJUST);
	eof_song->beat[targetbeat]->flags |= EOF_BEAT_FLAG_ANCHOR;
	if(eof_note_auto_adjust)
	{	//If the user has enabled the Note Auto-Adjust preference
		(void) eof_menu_edit_cut(targetbeat + 1, 1);	//Save auto-adjust data
	}
	for(ctr = targetbeat; ctr < eof_song->beats; ctr++)
	{	//For each beat from the target beat onward
		if((eof_song->beat[ctr]->ppqn != oldppqn) || ((eof_song->beat[ctr]->flags & EOF_BEAT_FLAG_ANCHOR) && (ctr != targetbeat)))
		{	//If this beat is a tempo change, or is an anchor (except for the first beat modified by this function, which is allowed)
			break;	//Exit the loop
		}

		eof_song->beat[ctr]->ppqn = newppqn;	//Update the beat's tempo
	}
	eof_calculate_beats(eof_song);	//Update beat timestamps
	eof_truncate_chart(eof_song);	//Update number of beats and the chart length as appropriate
	if(eof_note_auto_adjust)
	{	//If the user has enabled the Note Auto-Adjust preference
		(void) eof_menu_edit_cut_paste(targetbeat + 1, 1);
	}
	eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current
	return 1;
}

int eof_apply_key_signature(int signature, unsigned long beatnum, EOF_SONG *sp, char *undo_made)
{
	if((signature < -7) || (signature > 7) || !sp || (beatnum >= sp->beats) || !undo_made)
		return 0;	//Return error;

	if((sp->beat[beatnum]->flags & EOF_BEAT_FLAG_KEY_SIG) && (sp->beat[beatnum]->key == signature))
		return 0;	//Don't apply a key signature if the selected beat already has that specific key signature

	if(*undo_made == 0)
	{
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make an undo state
		*undo_made = 1;
	}
	sp->beat[beatnum]->flags |= EOF_BEAT_FLAG_KEY_SIG;	//Set this flag
	sp->beat[beatnum]->key = signature;
	return 1;
}

int eof_menu_beat_ks_7_flats(void)
{
	char undo_made = 0;
	return eof_apply_key_signature(-7, eof_selected_beat, eof_song, &undo_made);
}

int eof_menu_beat_ks_6_flats(void)
{
	char undo_made = 0;
	return eof_apply_key_signature(-6, eof_selected_beat, eof_song, &undo_made);
}

int eof_menu_beat_ks_5_flats(void)
{
	char undo_made = 0;
	return eof_apply_key_signature(-5, eof_selected_beat, eof_song, &undo_made);
}

int eof_menu_beat_ks_4_flats(void)
{
	char undo_made = 0;
	return eof_apply_key_signature(-4, eof_selected_beat, eof_song, &undo_made);
}

int eof_menu_beat_ks_3_flats(void)
{
	char undo_made = 0;
	return eof_apply_key_signature(-3, eof_selected_beat, eof_song, &undo_made);
}

int eof_menu_beat_ks_2_flats(void)
{
	char undo_made = 0;
	return eof_apply_key_signature(-2, eof_selected_beat, eof_song, &undo_made);
}

int eof_menu_beat_ks_1_flat(void)
{
	char undo_made = 0;
	return eof_apply_key_signature(-1, eof_selected_beat, eof_song, &undo_made);
}

int eof_menu_beat_ks_0_flats(void)
{
	char undo_made = 0;
	return eof_apply_key_signature(0, eof_selected_beat, eof_song, &undo_made);
}

int eof_menu_beat_ks_1_sharp(void)
{
	char undo_made = 0;
	return eof_apply_key_signature(1, eof_selected_beat, eof_song, &undo_made);
}

int eof_menu_beat_ks_2_sharps(void)
{
	char undo_made = 0;
	return eof_apply_key_signature(2, eof_selected_beat, eof_song, &undo_made);
}

int eof_menu_beat_ks_3_sharps(void)
{
	char undo_made = 0;
	return eof_apply_key_signature(3, eof_selected_beat, eof_song, &undo_made);
}

int eof_menu_beat_ks_4_sharps(void)
{
	char undo_made = 0;
	return eof_apply_key_signature(4, eof_selected_beat, eof_song, &undo_made);
}

int eof_menu_beat_ks_5_sharps(void)
{
	char undo_made = 0;
	return eof_apply_key_signature(5, eof_selected_beat, eof_song, &undo_made);
}

int eof_menu_beat_ks_6_sharps(void)
{
	char undo_made = 0;
	return eof_apply_key_signature(6, eof_selected_beat, eof_song, &undo_made);
}

int eof_menu_beat_ks_7_sharps(void)
{
	char undo_made = 0;
	return eof_apply_key_signature(7, eof_selected_beat, eof_song, &undo_made);
}

int eof_menu_beat_ks_off(void)
{
	if(eof_song && (eof_selected_beat < eof_song->beats))
	{	//If a valid beat is selected
		if(eof_song->beat[eof_selected_beat]->flags & EOF_BEAT_FLAG_KEY_SIG)
		{	//If the selected beat has a key signature defined
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make an undo state
			eof_song->beat[eof_selected_beat]->flags &= ~EOF_BEAT_FLAG_KEY_SIG;	//Clear this flag
			eof_song->beat[eof_selected_beat]->key = 0;
		}
	}
	return 1;
}

char * eof_rs_section_add_list(int index, int * size)
{
	if(index < 0)
	{	//Signal to return the list count
		if(size)
			*size = EOF_NUM_RS_PREDEFINED_SECTIONS;
		return NULL;
	}
	else if(index < EOF_NUM_RS_PREDEFINED_SECTIONS)
	{	//Return the specified list item
		return eof_rs_predefined_sections[index].displayname;	//Return the display name
	}
	return NULL;
}

char * eof_rs_event_add_list(int index, int * size)
{
	if(index < 0)
	{	//Signal to return the list count
		if(size)
			*size = EOF_NUM_RS_PREDEFINED_EVENTS;
		return NULL;
	}
	else if(index < EOF_NUM_RS_PREDEFINED_EVENTS)
	{	//Return the specified list item
		return eof_rs_predefined_events[index].displayname;	//Return the display name
	}
	return NULL;
}

int eof_rocksmith_section_dialog_add(void)
{
	unsigned long flags = 0, newflags = 0, ctr;
	unsigned char track = 0, newtrack = 0;	//By default, new sections won't be track specific
	char *emptystring = "";
	char *string = emptystring, *newstring;
	EOF_TEXT_EVENT *ptr = NULL;

	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_rocksmith_section_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_rocksmith_section_dialog);

	(void) snprintf(eof_events_add_dialog_string, sizeof(eof_events_add_dialog_string) - 1, "Specific to %s", eof_song->track[eof_selected_track]->name);

	if(eof_song->beat[eof_selected_beat]->contained_rs_section_event >= 0)
	{	//If the selected beat already has an RS section defined
		ptr = eof_song->text_event[eof_song->beat[eof_selected_beat]->contained_rs_section_event];
		flags = ptr->flags;	//Store these values
		track = ptr->track;
		string = ptr->text;
		for(ctr = 0; ctr < EOF_NUM_RS_PREDEFINED_SECTIONS; ctr++)
		{	//For each of the valid RS section names
			if(!ustrcmp(eof_rs_predefined_sections[ctr].string, ptr->text))
			{	//If the name matches the RS section on the selected beat
				eof_rocksmith_section_dialog[1].d1 = ctr;	//Select it in the dialog's list
				break;
			}
		}
		if(ptr->track != 0)
		{	//If the RS section is specific to this track
			eof_rocksmith_section_dialog[3].flags = D_SELECTED;	//Check that option in the dialog
		}
		else
		{
			eof_rocksmith_section_dialog[3].flags = 0;	//Otherwise clear it
		}
		if(ptr->flags & EOF_EVENT_FLAG_RS_PHRASE)
		{	//If the RS section is also an RS phrase
			eof_rocksmith_section_dialog[2].flags = D_SELECTED;	//Check that option in the dialog
		}
		else
		{
			eof_rocksmith_section_dialog[2].flags = 0;	//Otherwise clear it
		}
		eof_rocksmith_section_dialog[0].dp = eof_rocksmith_section_dialog_string2;	//Update the dialog window title to reflect that a section is being edited
	}
	else
	{	//Otherwise update the dialog window title to reflect that a new section is being added
		eof_rocksmith_section_dialog[0].dp = eof_rocksmith_section_dialog_string1;
	}

	if(eof_popup_dialog(eof_rocksmith_section_dialog, 0) == 4)
	{	//User clicked OK
		newstring = eof_rs_predefined_sections[eof_rocksmith_section_dialog[1].d1].string;
		newflags = EOF_EVENT_FLAG_RS_SECTION;	//By default, the event will only be a RS section
		if(eof_rocksmith_section_dialog[2].flags == D_SELECTED)
		{	//If the user also opted to add it as a RS phrase
			newflags |= EOF_EVENT_FLAG_RS_PHRASE;	//The event will be both a RS phrase and a RS section
		}
		if(eof_rocksmith_section_dialog[3].flags == D_SELECTED)
		{	//If the user also opted to make it specific to the active track
			newtrack = eof_selected_track;
		}

		if(ustrcmp(string, newstring) || (flags != newflags) || (track != newtrack))
		{	//If a new RS section was added, or changes were made to an existing one
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make an undo state
			if(ptr)
			{	//If an existing RS section is being edited
				(void) ustrcpy(ptr->text, newstring);
				ptr->flags = newflags;
				ptr->track = newtrack;
			}
			else
			{	//If a new RS section is being added
				(void) eof_song_add_text_event(eof_song, eof_selected_beat, newstring, newtrack, newflags, 0);
				eof_sort_events(eof_song);
			}
		}
	}

	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(screen);
	eof_render();
	eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current

	return D_O_K;
}

int eof_rocksmith_event_dialog_add(void)
{
	unsigned char track = 0;	//By default, new events won't be track specific

	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_rocksmith_event_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_rocksmith_event_dialog);

	(void) snprintf(eof_events_add_dialog_string, sizeof(eof_events_add_dialog_string) - 1, "Specific to %s", eof_song->track[eof_selected_track]->name);

	if(eof_popup_dialog(eof_rocksmith_event_dialog, 0) == 3)
	{	//User clicked OK
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make an undo state
		if(eof_rocksmith_event_dialog[2].flags == D_SELECTED)
		{	//If the user also opted to make it specific to the active track
			track = eof_selected_track;
		}
		(void) eof_song_add_text_event(eof_song, eof_selected_beat, eof_rs_predefined_events[eof_rocksmith_event_dialog[1].d1].string, track, EOF_EVENT_FLAG_RS_EVENT, 0);
		eof_sort_events(eof_song);
	}

	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(screen);
	eof_render();
	eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current

	return D_O_K;
}

int eof_menu_beat_copy_rs_events(void)
{
	PACKFILE * fp;
	int contained_rs_phrase_event, contained_rs_section_event;
	char write_rs_phrase = 0, write_rs_section = 0;
	unsigned long num = 0;
	char eof_events_clipboard_path[50];

	if(!eof_song || (eof_selected_track >= eof_song->tracks) || (eof_selected_beat >= eof_song->beats))
		return 1;	//Error

	//Determine which events are to be written
	eof_process_beat_statistics(eof_song, eof_selected_track);	//Cache RS phrase and section information into the beat structures (from the perspective of the active track)
	contained_rs_phrase_event = eof_song->beat[eof_selected_beat]->contained_section_event;
	contained_rs_section_event = eof_song->beat[eof_selected_beat]->contained_rs_section_event;
	if(contained_rs_phrase_event >= 0)
	{	//If this beat has a valid RS phrase in effect
		num++;
		write_rs_phrase = 1;
	}
	if((contained_rs_section_event >= 0) && (contained_rs_section_event != contained_rs_phrase_event))
	{	//If this beat has a RS section in effect, and it's not the same text event as the above RS phrase
		num++;
		write_rs_section = 1;
	}
	if(!num)
	{
		allegro_message("This beat has no applicable Rocksmith phrases or sections");
		return 1;
	}

	//Create clipboard file
	if(eof_validate_temp_folder())
	{	//Ensure the correct working directory and presence of the temporary folder
		eof_log("\tCould not validate working directory and temp folder", 1);
		return 1;
	}

	(void) snprintf(eof_events_clipboard_path, sizeof(eof_events_clipboard_path) - 1, "%seof.events.clipboard", eof_temp_path_s);
	fp = pack_fopen(eof_events_clipboard_path, "w");
	if(!fp)
	{
		allegro_message("Clipboard error!");
		return 1;
	}
	(void) pack_iputl(num, fp);	//Write the number of events this clipboard file will contain
	if(write_rs_phrase)
	{	//If the RS phrase is to be written
		(void) eof_save_song_string_pf(eof_song->text_event[contained_rs_phrase_event]->text, fp);	//Write the event's name
		(void) pack_iputl(eof_song->text_event[contained_rs_phrase_event]->track, fp);				//Write the event's track
		(void) pack_iputl(eof_song->text_event[contained_rs_phrase_event]->flags, fp);				//Write the event's flags
	}
	if(write_rs_section)
	{	//If the RS section is to be written
		(void) eof_save_song_string_pf(eof_song->text_event[contained_rs_section_event]->text, fp);	//Write the event's name
		(void) pack_iputl(eof_song->text_event[contained_rs_section_event]->track, fp);				//Write the event's track
		(void) pack_iputl(eof_song->text_event[contained_rs_section_event]->flags, fp);				//Write the event's flags
	}
	(void) pack_fclose(fp);

	return 1;
}

int eof_menu_beat_copy_events(void)
{
	unsigned long ctr, ctr2, count = 0;
	PACKFILE * fp = NULL;
	char eof_events_clipboard_path[50];

	for(ctr = 0; ctr < 2; ctr++)
	{
		for(ctr2 = 0; ctr2 < eof_song->text_events; ctr2++)
		{	//For each text event in the project
			if(!(eof_song->text_event[ctr2]->flags & EOF_EVENT_FLAG_FLOATING_POS))
			{	//If this text event is assigned to a beat marker
				if(eof_song->text_event[ctr2]->pos != eof_selected_beat)
					continue;	//If the text event is not assigned to this beat, skip it
				if(eof_song->text_event[ctr2]->track && (eof_song->text_event[ctr2]->track != eof_selected_track))
					continue;	//If the text event is assigned to a specific track but it is not the active track, skip it

				if(!ctr)
				{	//On the first pass, count the events that will be copied
					count++;
				}
				else
				{	//On the second pass, write the events to the clipboard file
					(void) eof_save_song_string_pf(eof_song->text_event[ctr2]->text, fp);	//Write the event's text
					(void) pack_iputl(eof_song->text_event[ctr2]->track, fp);				//Write the event's track
					(void) pack_iputl(eof_song->text_event[ctr2]->flags, fp);				//Write the event's flags
				}
			}
		}
		if(ctr)
			continue;	//If this isn't the first pass, skip opening the clipboard file below

		if(eof_validate_temp_folder())
		{	//Ensure the correct working directory and presence of the temporary folder
			eof_log("\tCould not validate working directory and temp folder", 1);
			return 1;
		}

		(void) snprintf(eof_events_clipboard_path, sizeof(eof_events_clipboard_path) - 1, "%seof.events.clipboard", eof_temp_path_s);
		fp = pack_fopen(eof_events_clipboard_path, "w");
		if(!fp)
		{
			allegro_message("Clipboard error!");
			return 1;
		}
		(void) pack_iputl(count, fp);	//Write the number of events this clipboard file will contain
	}
	(void) pack_fclose(fp);

	return 1;
}

int eof_menu_beat_paste_events(void)
{
	PACKFILE * fp;
	char text[256];
	unsigned long ctr, num, flags, track;
	char eof_events_clipboard_path[50];
	char rs_warn = 0;

	if(!eof_song || (eof_selected_track >= eof_song->tracks) || (eof_selected_beat >= eof_song->beats))
		return 1;	//Error

	//Open the clipboard
	(void) snprintf(eof_events_clipboard_path, sizeof(eof_events_clipboard_path) - 1, "%seof.events.clipboard", eof_temp_path_s);
	fp = pack_fopen(eof_events_clipboard_path, "r");
	if(!fp)
	{
		allegro_message("Clipboard error!\nNothing to paste!");
		return 1;
	}

	//Load contents of clipboard into the chart
	num = pack_igetl(fp);	//Read the number of events in the clipboard
	if(num)
	{	//If there's at least one text event
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		for(ctr = 0; ctr < num; ctr++)
		{	//For each event in the clipboard
			(void) eof_load_song_string_pf(text, fp, sizeof(text));	//Read the event's name (this function guarantees buffer overflow protection and NULL termination)
			track = pack_igetl(fp);	//Read the track number
			flags = pack_igetl(fp);	//Read the flags
			if(track)
			{	//If the event is track specific, it will be applied to the active track
				if((flags & EOF_EVENT_FLAG_RS_PHRASE) || (flags & EOF_EVENT_FLAG_RS_SECTION) || (flags & EOF_EVENT_FLAG_RS_EVENT))
				{	//If the event is Rocksmith-specific
					if((eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT) && !rs_warn)
					{	//If the destination track is not a pro guitar track, and the user hasn't been warned of this yet
						allegro_message("Warning:  You cannot paste a track-specific Rocksmith event into a non pro guitar/bass track.");
						rs_warn = 1;
						continue;	//Skip pasting this event
					}
				}
				track = eof_selected_track;
			}
			(void) eof_song_add_text_event(eof_song, eof_selected_beat, text, track, flags, 0);	//Add the event to the track
		}
		eof_sort_events(eof_song);	//Sort the events
		eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current
	}
	(void) pack_fclose(fp);

	return 1;
}

int eof_events_dialog_move(char direction)
{
	long previous = -1, selected = -1, next = -1;	//Stores the indexes of the selected text event, as well as the previous and next ones if applicable
	unsigned long ecount = 0, i;
	int junk;
	EOF_TEXT_EVENT *ptr;

	if(eof_events_dialog[1].d1 < 0)
		return D_O_K;

	/* find the relevant event indexes */
	for(i = 0; i < eof_song->text_events; i++)
	{	//For each text event
		if(!(eof_song->text_event[i]->flags & EOF_EVENT_FLAG_FLOATING_POS))
		{	//If this text event is assigned to a beat marker
			if(eof_song->text_event[i]->pos != eof_selected_beat)
				continue;	//If the text event is not applied to the selected beat, skip it

			if((unsigned long)eof_events_dialog[1].d1 == ecount)
			{	//If the text event is the one selected in the Events dialog
				selected = i;
			}
			else
			{	//The text event is one of the non selected events in the dialog
				if(selected == -1)
				{	//If the selected one hasn't been reached yet
					previous = i;	//It's one of the previous entries
				}
				else
				{	//Otherwise it's the one immediately after it
					next = i;
					break;	//Stop parsing, since the selected entry has been found, and any previous and next event has been identified
				}
			}
			ecount++;	//Count the number of events at the selected beat that have been reached
		}
	}

	if(selected < 0)
		return D_O_K;	//If the selected event was not found, return immediately

	if(direction < 0)
	{	//If the selected event is being moved up in the list of events at the selected beat
		if(previous >= 0)
		{	//If there was an event listed before the selected event
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			ptr = eof_song->text_event[previous];	//Store this pointer
			eof_song->text_event[previous] = eof_song->text_event[selected];	//Swap the selected event with the one before it
			eof_song->text_event[selected] = ptr;
			eof_events_dialog[1].d1--;	//Move the selected index to compensate
			eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current, since event order matters for Rocksmith phrases and sections
			eof_render();
			(void) dialog_message(eof_events_dialog, MSG_DRAW, 0, &junk);
		}
	}
	else
	{	//Otherwise move it down in the list
		if(next >= 0)
		{	//If there was an event listed after the selected event
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			ptr = eof_song->text_event[next];	//Store this pointer
			eof_song->text_event[next] = eof_song->text_event[selected];	//Swap the selected event with the one after it
			eof_song->text_event[selected] = ptr;
			eof_events_dialog[1].d1++;	//Move the selected index to compensate
			eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current, since event order matters for Rocksmith phrases and sections
			eof_render();
			(void) dialog_message(eof_events_dialog, MSG_DRAW, 0, &junk);
		}
	}

	return D_O_K;
}

int eof_events_dialog_move_up(DIALOG * d)
{
	if(!d)
	{	//Satisfy Splint by checking value of d
		return D_O_K;
	}

	return eof_events_dialog_move(-1);
}

int eof_events_dialog_move_down(DIALOG * d)
{
	if(!d)
	{	//Satisfy Splint by checking value of d
		return D_O_K;
	}

	return eof_events_dialog_move(1);
}

#ifdef EOF_NO_MINIBPM

int eof_menu_beat_estimate_bpm(void)
{
	return D_O_K;
}

#else

int eof_menu_beat_estimate_bpm(void)
{
	unsigned long ctr, ppqn;
	double result;
	unsigned long startbeat;	//The first beat to which the detected tempo will be applied if user accepts the offer to do so
	int portion = 0;

	if(!eof_song || !eof_music_track)
		return D_O_K;

	eof_bpm_estimator_sample_count = 0;	//Reset this counter for debugging purposes
	set_window_title("Estimating tempo...");

	if((eof_song->tags->start_point != ULONG_MAX) && (eof_song->tags->end_point != ULONG_MAX) && (eof_song->tags->start_point != eof_song->tags->end_point))
	{	//If start/end markers are defined
		eof_clear_input();
		if(alert(NULL, "Estimate tempo of the entire song or just the portion between the start and end markers?", NULL, "&All", "&Portion", 'a', 'p') == 2)
		{	//If the user opts to estimate just between the start and end markers
			portion = 1;
		}
	}

	if(portion)
	{	//If just estimating between the start and end markers
		double start, end;

		start = (double)eof_song->tags->start_point / 1000.0;
		end = (double)eof_song->tags->end_point / 1000.0;
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Estimating tempo from %f seconds to %f seconds", start, end);
		eof_log(eof_log_string, 1);

		result = eof_estimate_bpm(eof_music_track, start, end);
		eof_fix_window_title();

		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tMiniBPM processed %lu samples and determined a tempo of %fBPM", eof_bpm_estimator_sample_count, result);
		eof_log(eof_log_string, 1);
		allegro_message("Estimated tempo of the selection is %fBPM", result);

		startbeat = eof_get_beat(eof_song, eof_song->tags->start_point);
		if(eof_beat_num_valid(eof_song, startbeat))
		{	//If the beat in which the selection start begins was found
			if(startbeat == ULONG_MAX)	//Redundant check to satisfy Coverity
				return D_O_K;

			if(eof_song->beat[startbeat]->pos == eof_song->tags->start_point)
			{	//If the selection starts on that beat marker exactly
				eof_clear_input();
				if(eof_song->tags->tempo_map_locked || (alert(NULL, "Would you like to apply this tempo to the beat at which the selection starts?", NULL, "&Yes", "&No", 'y', 'n') != 1))
					return D_O_K;	//If the tempo map is locked or the user does not opt to apply the estimated tempo, return immediately
			}
			else
				return D_O_K;	//Don't perform any more processing
		}
		else
		{
			startbeat = 0;
		}
	}
	else
	{
		eof_log("Estimating tempo of entire song", 1);

		result = eof_estimate_bpm(eof_music_track, 0, 0);
		eof_fix_window_title();

		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tMiniBPM processed %lu samples and determined a tempo of %fBPM", eof_bpm_estimator_sample_count, result);
		eof_log(eof_log_string, 1);
		allegro_message("Estimated tempo of the song is %fBPM", result);

		eof_clear_input();
		if(eof_song->tags->tempo_map_locked || (alert(NULL, "Would you like to apply this tempo to the first beat?", NULL, "&Yes", "&No", 'y', 'n') != 1))
			return D_O_K;	//If the tempo map is locked or the user does not opt to apply the estimated tempo, return immediately

		startbeat = 0;		//The tempo will apply starting with the first beat marker
	}

	//Apply the detected tempo to the applicable beat and all subsequent beats until the next anchor is found
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	if(result < DBL_EPSILON)
		result = DBL_EPSILON;	//Bounds check result to be a positive, nonzero double float value
	ppqn = 60000000.0 / result;
	eof_song->beat[startbeat]->ppqn = ppqn;	//Apply the tempo to the first beat in the applicable range of beats
	for(ctr = startbeat + 1; ctr < eof_song->beats; ctr++)
	{	//For the remaining beats in the project
		if(eof_song->beat[ctr]->flags & EOF_BEAT_FLAG_ANCHOR)
		{	//If this beat is an anchor
			break;	//Stop updating beat tempos
		}
		eof_song->beat[ctr]->ppqn = ppqn;	//Apply the new tempo
	}
	eof_calculate_beats(eof_song);
	eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current

	return D_O_K;
}

#endif

int eof_menu_beat_copy_tempo_map(void)
{
	unsigned long ctr, start_beat, end_beat;
	unsigned num = 0, den = 0;
	char *instructions = "Use \"Set start point\" and \"Set end point\" from the Edit menu or click on a beat and then use SHIFT+click on another beat to select the range of beats for this copy operation.";
	PACKFILE * fp;
	char clipboard_path[50];

	if(!eof_song)
		return D_O_K;

	eof_log("eof_menu_beat_copy_tempo_map() entered", 1);

	//Validate start and end points
	if((eof_song->tags->start_point == ULONG_MAX) || (eof_song->tags->end_point == ULONG_MAX))
	{	//If either the start point or the end point aren't defined
		allegro_message("No selection defined.\n%s", instructions);
		return D_O_K;
	}

	start_beat = eof_get_beat(eof_song, eof_song->tags->start_point);
	end_beat = eof_get_beat(eof_song, eof_song->tags->end_point);
	if((start_beat >= eof_song->beats) || (end_beat >= eof_song->beats))
	{
		allegro_message("Invalid selection.\n%s", instructions);
		return D_O_K;
	}

	if((start_beat == end_beat) && (eof_song->tags->start_point != eof_song->beat[start_beat]->pos))
	{	//If there is a selection, but it doesn't overlap any beat markers
		allegro_message("No beat markers are selected.\n%s", instructions);
		return D_O_K;
	}

	//Open clipboard file for writing
	if(eof_validate_temp_folder())
	{	//Ensure the correct working directory and presence of the temporary folder
		eof_log("\tCould not validate working directory and temp folder", 1);
		return D_O_K;
	}

	(void) snprintf(clipboard_path, sizeof(clipboard_path) - 1, "%seof.tempo.clipboard", eof_temp_path_s);
	fp = pack_fopen(clipboard_path, "w");
	if(!fp)
	{
		allegro_message("Clipboard error!");
		return D_O_K;
	}

	//Write tempo data to clipboard file
	(void) pack_iputl(end_beat - start_beat + 1, fp);	//Write the number of beats of data there will be
	(void) eof_get_effective_ts(eof_song, &num, &den, start_beat, 1);	//The first beat's written data should include any explicit time signature in effect
	for(ctr = start_beat; ctr <= end_beat; ctr++)
	{	//For each beat in the selection
		(void) eof_get_ts(eof_song, &num, &den, ctr);	//Update num and den to reflect any time signature change at this beat, if any
		(void) pack_iputl(eof_song->beat[ctr]->ppqn, fp);	//Write the beat's native tempo value (PPQN)
		(void) pack_iputl(num, fp);				//Write the time signature in effect
		(void) pack_iputl(den, fp);
	}
	(void) pack_fclose(fp);

	eof_close_menu = 1;			//Force the main menu to close, as this function had a tendency to get hung in the menu logic when activated by keyboard
	return D_O_K;
}

int eof_menu_beat_paste_tempo_map(void)
{
	unsigned long ctr, numbeats, ppqn;
	unsigned num = 0, den = 0;
	PACKFILE * fp;
	char clipboard_path[50];
	unsigned long remove_flags = EOF_BEAT_FLAG_ANCHOR | EOF_BEAT_FLAG_START_4_4 | EOF_BEAT_FLAG_START_3_4 | EOF_BEAT_FLAG_START_5_4 | EOF_BEAT_FLAG_START_6_4 | EOF_BEAT_FLAG_CUSTOM_TS | EOF_BEAT_FLAG_MIDBEAT | EOF_BEAT_FLAG_START_2_4;

	if(!eof_song || (eof_selected_beat >= eof_song->beats))
		return D_O_K;	//Error

	eof_log("eof_menu_beat_copy_tempo_map() entered", 1);

	//Open clipboard file for reading
	if(eof_validate_temp_folder())
	{	//Ensure the correct working directory and presence of the temporary folder
		eof_log("\tCould not validate working directory and temp folder", 1);
		return D_O_K;
	}

	(void) snprintf(clipboard_path, sizeof(clipboard_path) - 1, "%seof.tempo.clipboard", eof_temp_path_s);
	fp = pack_fopen(clipboard_path, "r");
	if(!fp)
	{
		allegro_message("Clipboard error!");
		return D_O_K;
	}

	//Apply clipboard data to beats
	numbeats = pack_igetl(fp);		//Read the number of beats' worth of tempo data this is in the clipboard file
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	for(ctr = 0; ctr < numbeats; ctr++)
	{	//For each beat of data in the clipboard fle
		//Read the data
		ppqn = pack_igetl(fp);	//Read the beat's tempo (PPQN)
		num = pack_igetl(fp);	//Read the beat's time signature change, if any
		den = pack_igetl(fp);

		//Update the selected beat
		eof_song->beat[eof_selected_beat]->flags &= ~remove_flags;	//Clear these flags
		eof_song->beat[eof_selected_beat]->ppqn = ppqn;	//Set the tempo
		if(!eof_selected_beat || (eof_song->beat[eof_selected_beat - 1]->ppqn != ppqn))
		{	//If the selected beat is the first beat, or if the previous beat has a different tempo
			eof_song->beat[eof_selected_beat]->flags |= EOF_BEAT_FLAG_ANCHOR;	//Make it an anchor
		}
		if(num && den)
		{	//If a time signature is to be set
			if(eof_selected_beat > 0)
			{	//If a beat after the first beat is selected
				unsigned eff_num = 0, eff_den = 0;

				(void) eof_get_effective_ts(eof_song, &eff_num, &eff_den, eof_selected_beat - 1, 1);	//Get any explicitly defined time signature in effect
				if(!eff_num || !eff_den || (eff_num != num) || (eff_den != den))
				{	//If there is no defined time signature in effect before the selected beat, or it is defined and is different from the signature from the clipboard
					(void) eof_apply_ts(num, den, eof_selected_beat, eof_song, 0);	//Set it
				}
			}
			else
			{	//The first beat will get any time signature change from the clipboard
				(void) eof_apply_ts(num, den, eof_selected_beat, eof_song, 0);
			}
		}

		//Update the next beat's position
		if(eof_selected_beat + 1 < eof_song->beats)
		{	//If there is a next beat
			eof_song->beat[eof_selected_beat + 1]->fpos = eof_song->beat[eof_selected_beat]->fpos + eof_calc_beat_length(eof_song, eof_selected_beat);	//Set its position
			eof_song->beat[eof_selected_beat + 1]->pos = eof_song->beat[eof_selected_beat + 1]->fpos + 0.5;

			eof_selected_beat++;	//Select the next beat
		}
		else
			break;	//There are no more beats to have tempo/TS changes applied to
	}
	(void) pack_fclose(fp);

	//Update the position of any remaining beats
	for(ctr = eof_selected_beat; ctr + 1 < eof_song->beats; ctr++)
	{	//For all beats from the now selected beat up until the penultimate one
		eof_song->beat[ctr + 1]->fpos = eof_song->beat[ctr]->fpos + eof_calc_beat_length(eof_song, ctr);	//Set the next beat's position
		eof_song->beat[ctr + 1]->pos = eof_song->beat[ctr + 1]->fpos + 0.5;
	}

	eof_set_seek_position(eof_song->beat[eof_selected_beat]->pos + eof_av_delay);	//Seek to the now selected beat
	eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current
	eof_close_menu = 1;			//Force the main menu to close, as this function had a tendency to get hung in the menu logic when activated by keyboard
	eof_render();

	return D_O_K;
}

int eof_menu_beat_validate_tempo_map(void)
{
	(void) eof_detect_tempo_map_corruption(eof_song, 1);
	eof_close_menu = 1;			//Force the main menu to close, as this function had a tendency to get hung in the menu logic when activated by keyboard

	return D_O_K;
}

int eof_menu_beat_lock_tempo_map(void)
{
	if(eof_song)
		eof_song->tags->tempo_map_locked ^= 1;	//Toggle this boolean variable
	eof_fix_window_title();
	return 1;
}

int eof_menu_beat_remove_mid_beat_status(void)
{
	if(!eof_song || (eof_selected_beat >= eof_song->beats))
		return D_O_K;	//Error

	if(!(eof_song->beat[eof_selected_beat]->flags & EOF_BEAT_FLAG_MIDBEAT))
		return D_O_K;	//Selected beat does not have this status

	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	eof_song->beat[eof_selected_beat]->flags &= ~EOF_BEAT_FLAG_MIDBEAT;	//Clear this status

	return D_O_K;
}

int eof_export_beat_timings_edit_proc(int msg, DIALOG *d, int c)
{
	//Check ASCII code input
	if(msg == MSG_CHAR)
	{
		unsigned c2 = (c & 255);	//The lower 8 bits is the scan code of the key press

		if((c2 == 'm') || (c2 == 'M'))
		{	//The user pressed m
			eof_export_beat_timings_dialog[3].flags = D_SELECTED;	//Select Millis beat radio button
			eof_export_beat_timings_dialog[4].flags = 0;			//Clear the measure radio button
			(void) object_message(&eof_export_beat_timings_dialog[3], MSG_DRAW, 0);		//Have Allegro redraw the radio buttons
			(void) object_message(&eof_export_beat_timings_dialog[4], MSG_DRAW, 0);
			return D_USED_CHAR;	//Input processed
		}
		else if((c2 == 'i') || (c2 == 'I'))
		{	//The user pressed i
			eof_export_beat_timings_dialog[4].flags = D_SELECTED;	//Select the Intervals radio button
			eof_export_beat_timings_dialog[3].flags = 0;			//Clear the beat radio button
			(void) object_message(&eof_export_beat_timings_dialog[3], MSG_DRAW, 0);		//Have Allegro redraw the radio buttons
			(void) object_message(&eof_export_beat_timings_dialog[4], MSG_DRAW, 0);
			return D_USED_CHAR;	//Input processed
		}
	}

	return eof_verified_edit_proc(msg, d, c);	//Allow the rest of the input to be filtered normally
}

int eof_menu_beat_export_beat_timings(void)
{
	unsigned long ctr, input = 0, output;
	PACKFILE * fp;
	char buffer[1024] = {0};

	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_export_beat_timings_dialog, gui_fg_color, gui_bg_color);
	eof_conditionally_center_dialog(eof_export_beat_timings_dialog);
	eof_etext2[0] = '\0';

	if(eof_popup_dialog(eof_export_beat_timings_dialog, 2) == 5)
	{	//User clicked OK
		(void) replace_filename(buffer, eof_song_path, "timings.txt", 1024);	//Obtain the destination filename
		fp = pack_fopen(buffer, "w");
		if(!fp)
		{
			eof_log("\tError saving:  Cannot open file for writing", 1);
			return 0;	//Return failure
		}

		input = atoi(eof_etext2);
		if(input < 1)
			input = 1;     //Values less than 1 are not valid for this function

		for(ctr = 0; ctr < eof_song->beats; ctr++)
		{	//For each beat
			if(eof_export_beat_timings_dialog[3].flags & D_SELECTED)
			{	//Export milliseconds
				output = eof_song->beat[ctr]->pos;
			}
			else
			{	//Export intervals per second
				output = ((eof_song->beat[ctr]->fpos * (double)input) / 1000.0) + 0.5;
			}
			(void) snprintf(buffer, sizeof(buffer) - 1, "beat(%lu)\n", output);
			(void) pack_fputs(buffer, fp);
		}

		(void) pack_fclose(fp);
	}

	//Cleanup
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(NULL);
	return 1;
}
