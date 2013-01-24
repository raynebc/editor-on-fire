#include <allegro.h>
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
#include "beat.h"
#include "edit.h"
#include "song.h"

#ifdef USEMEMWATCH
#include "../memwatch.h"
#endif

char eof_ts_menu_off_text[32] = {0};
char eof_ks_menu_off_text[32] = {0};

MENU eof_beat_time_signature_menu[] =
{
    {"&4/4", eof_menu_beat_ts_4_4, NULL, 0, NULL},
    {"&3/4", eof_menu_beat_ts_3_4, NULL, 0, NULL},
    {"&5/4", eof_menu_beat_ts_5_4, NULL, 0, NULL},
    {"&6/4", eof_menu_beat_ts_6_4, NULL, 0, NULL},
    {"&Custom", eof_menu_beat_ts_custom, NULL, 0, NULL},
    {eof_ts_menu_off_text, eof_menu_beat_ts_off, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_beat_key_signature_menu[] =
{
    {eof_ks_menu_off_text, eof_menu_beat_ks_off, NULL, 0, NULL},
    {"B , g# (7 flats)", eof_menu_beat_ks_7_flats, NULL, 0, NULL},
    {"Gb , eb (6 flats)", eof_menu_beat_ks_6_flats, NULL, 0, NULL},
    {"Db , bb (5 flats)", eof_menu_beat_ks_5_flats, NULL, 0, NULL},
    {"Ab , f (4 flats)", eof_menu_beat_ks_4_flats, NULL, 0, NULL},
    {"Eb , c (3 flats)", eof_menu_beat_ks_3_flats, NULL, 0, NULL},
    {"Bb , g (2 flats)", eof_menu_beat_ks_2_flats, NULL, 0, NULL},
    {"F , d (1 flat)", eof_menu_beat_ks_1_flat, NULL, 0, NULL},
    {"C , a (0 flats/sharps)", eof_menu_beat_ks_0_flats, NULL, 0, NULL},
    {"G , e (1 sharp)", eof_menu_beat_ks_1_sharp, NULL, 0, NULL},
    {"D , b (2 sharps)", eof_menu_beat_ks_2_sharps, NULL, 0, NULL},
    {"A , f# (3 sharps)", eof_menu_beat_ks_3_sharps, NULL, 0, NULL},
    {"E , c# (4 sharps)", eof_menu_beat_ks_4_sharps, NULL, 0, NULL},
    {"B , g# (5 sharps)", eof_menu_beat_ks_5_sharps, NULL, 0, NULL},
    {"Gb , eb (6 sharps)", eof_menu_beat_ks_6_sharps, NULL, 0, NULL},
    {"Db , bb (7 sharps)", eof_menu_beat_ks_7_sharps, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_beat_menu[] =
{
    {"&BPM Change", eof_menu_beat_bpm_change, NULL, 0, NULL},
    {"Time &Signature", NULL, eof_beat_time_signature_menu, 0, NULL},
    {"&Key Signature", NULL, eof_beat_key_signature_menu, 0, NULL},
    {"", NULL, NULL, 0, NULL},
    {"Add", eof_menu_beat_add, NULL, 0, NULL},
    {"Delete\t" CTRL_NAME "+Del", eof_menu_beat_delete, NULL, 0, NULL},
    {"Push Offset Back", eof_menu_beat_push_offset_back_menu, NULL, 0, NULL},
    {"Push Offset Up", eof_menu_beat_push_offset_up, NULL, 0, NULL},
    {"Reset Offset to Zero", eof_menu_beat_reset_offset, NULL, 0, NULL},
    {"&Anchor Beat\tShift+A", eof_menu_beat_anchor, NULL, 0, NULL},
    {"Toggle Anchor\tA", eof_menu_beat_toggle_anchor, NULL, 0, NULL},
    {"&Delete Anchor", eof_menu_beat_delete_anchor, NULL, 0, NULL},
    {"&Reset BPM", eof_menu_beat_reset_bpm, NULL, 0, NULL},
    {"&Calculate BPM", eof_menu_beat_calculate_bpm, NULL, 0, NULL},
    {"Double BPM", eof_menu_beat_double_tempo, NULL, 0, NULL},
    {"Halve BPM", eof_menu_beat_halve_tempo, NULL, 0, NULL},
    {"Adjust tempo for RBN", eof_menu_beat_set_RBN_tempos, NULL, 0, NULL},
    {"", NULL, NULL, 0, NULL},
    {"All E&vents", eof_menu_beat_all_events, NULL, 0, NULL},
    {"&Events", eof_menu_beat_events, NULL, 0, NULL},
    {"Clear Events", eof_menu_beat_clear_events, NULL, 0, NULL},
    {"Place &Trainer Event", eof_menu_beat_trainer_event, NULL, 0, NULL},
    {"Place RS &Phrase\tShift+P", eof_rocksmith_phrase_dialog_add, NULL, 0, NULL},
    {"Place RS section\tShift+S", eof_rocksmith_section_dialog_add, NULL, 0, NULL},
    {"Place RS event\tShift+E", eof_rocksmith_event_dialog_add, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

char stored_event_track_notice[] = "Warning:  A stored MIDI track will override chart-wide text events";
char no_notice[] = "";
DIALOG eof_events_dialog[] =
{
   /* (proc)            (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)            (dp2) (dp3) */
   { d_agup_window_proc,0,   48,  500, 237, 2,   23,  0,    0,      0,   0,   "Events",       NULL, NULL },
   { d_agup_list_proc,  12,  84,  400, 138, 2,   23,  0,    0,      0,   0,   (void *)eof_events_list,NULL, NULL },
   { d_agup_push_proc,  425, 84,  68,  28,  2,   23,  'a',  D_EXIT, 0,   0,   "&Add",         NULL, (void *)eof_events_dialog_add },
   { d_agup_push_proc,  425, 124, 68,  28,  2,   23,  'e',  D_EXIT, 0,   0,   "&Edit",        NULL, (void *)eof_events_dialog_edit },
   { d_agup_push_proc,  425, 164, 68,  28,  2,   23,  'l',  D_EXIT, 0,   0,   "De&lete",      NULL, (void *)eof_events_dialog_delete },
   { d_agup_text_proc,  12,  225, 64,  8,   2,   23,  0,    0,      0,   0,   ""      ,       NULL, NULL },
   { d_agup_button_proc,12,  245, 240, 28,  2,   23,  '\r', D_EXIT, 0,   0,   "Done",         NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

///The dp2 values below must be edited appropriately whenever the radio buttons' object numbers in the dialog are changed
DIALOG eof_all_events_dialog[] =
{
   /* (proc)                    (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                   (dp2) (dp3) */
   { d_agup_window_proc,         0,   48,  500, 282, 2,   23,  0,    0,      0,   0,   "All Events",           NULL, NULL },
   { d_agup_list_proc,           12,  84,  475, 140, 2,   23,  0,    0,      0,   0,   (void *)eof_events_list_all,  NULL, NULL },
   { d_agup_button_proc,         12,  273, 70,  28,  2,   23,  'f',  D_EXIT, 0,   0,   "&Find",                NULL, NULL },
   { d_agup_button_proc,         95,  273, 70,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "Done",                 NULL, NULL },
   { d_agup_button_proc,         182, 273, 150, 28,  2,   23,  0,    D_EXIT, 0,   0,   "Copy to selected beat",NULL, NULL },
   { eof_all_events_radio_proc,	 340, 243, 85,  15,  2,   23,  0, D_SELECTED,0,   0,   "All Events",           (void *)5,    NULL },	//Use dp2 to store the object number, for use in eof_all_events_radio_proc()
   { eof_all_events_radio_proc,	 340, 259, 142, 15,  2,   23,  0,    0,      0,   0,   "This Track's Events",  (void *)6,    NULL },
   { eof_all_events_radio_proc,	 340, 275, 152, 15,  2,   23,  0,    0,      0,   0,   "Sections (RS phrases)",(void *)7,    NULL },
   { eof_all_events_radio_proc,	 340, 291, 152, 15,  2,   23,  0,    0,      0,   0,   "RS sections",          (void *)8,    NULL },
   { eof_all_events_radio_proc,	 340, 307, 152, 15,  2,   23,  0,    0,      0,   0,   "RS events",            (void *)9,    NULL },
   { d_agup_text_proc,           12,  228, 64,  8,   2,   23,  0,    0,      0,   0,   ""      ,                NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

char eof_events_add_dialog_string[100] = {0};
DIALOG eof_events_add_dialog[] =
{
   /* (proc)            (x) (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)           (dp2) (dp3) */
   { d_agup_window_proc,0,  48,  314, 186, 2,   23,  0,    0,      0,   0,   "Event Name",  NULL, NULL },
   { d_agup_text_proc,  12, 84,  64,  8,   2,   23,  0,    0,      0,   0,   "Text:",       NULL, NULL },
   { d_agup_edit_proc,  48, 80,  254, 20,  2,   23,  0,    0,      255, 0,   eof_etext,     NULL, NULL },
   { d_agup_check_proc, 12, 110, 250, 16,  0,   0,   0,    0,      1,   0,   eof_events_add_dialog_string, NULL, NULL },
   { d_agup_check_proc, 12, 130, 174, 16,  0,   0,   0,    0,      1,   0,   "Rocksmith phrase marker", NULL, NULL },
   { d_agup_check_proc, 12, 150, 182, 16,  0,   0,   0,    0,      1,   0,   "Rocksmith section marker", NULL, NULL },
   { d_agup_check_proc, 12, 170, 182, 16,  0,   0,   0,    0,      1,   0,   "Rocksmith event marker", NULL, NULL },
   { d_agup_button_proc,67, 194, 84,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",          NULL, NULL },
   { d_agup_button_proc,163,194, 78,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",      NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_bpm_change_dialog[] =
{
   /* (proc)         (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)           (dp2) (dp3) */
   { d_agup_shadow_box_proc,    32,  68,  170, 96 + 8 + 20, 2,   23,  0,    0,      0,   0,   NULL,               NULL, NULL },
   { d_agup_text_proc,   56,  84,  64,  8,  2,   23,  0,    0,      0,   0,   "BPM:",         NULL, NULL },
   { eof_verified_edit_proc,   112, 80,  66,  20,  2,   23,  0,    0,      255,   0,   eof_etext,           "1234567890.", NULL },
   { d_agup_check_proc, 56,  108, 128,  16, 2,   23,  0,    0, 1,   0,   "This Beat Only",               NULL, NULL },
   { d_agup_check_proc, 56,  128, 128,  16, 2,   23,  0,    0, 1,   0,   "Adjust Notes",               NULL, NULL },
   { d_agup_button_proc, 42,  152, 68,  28, 2,   23,  '\r',    D_EXIT, 0,   0,   "OK",               NULL, NULL },
   { d_agup_button_proc, 120, 152, 68,  28, 2,   23,  0,    D_EXIT, 0,   0,   "Cancel",           NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_anchor_dialog[] =
{
   /* (proc)         (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)           (dp2) (dp3) */
   { d_agup_shadow_box_proc,    32,  68,  170, 72 + 8, 2,   23,  0,    0,      0,   0,   NULL,               NULL, NULL },
   { d_agup_text_proc,   56,  84,  64,  8,  2,   23,  0,    0,      0,   0,   "Position:",         NULL, NULL },
   { eof_verified_edit_proc,   112, 80,  66,  20,  2,   23,  0,    0,      9,   0,   eof_etext2,           "0123456789:", NULL },
   { d_agup_button_proc, 42,  108, 68,  28, 2,   23,  '\r',    D_EXIT, 0,   0,   "OK",               NULL, NULL },
   { d_agup_button_proc, 120, 108, 68,  28, 2,   23,  0,    D_EXIT, 0,   0,   "Cancel",           NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

char eof_trainer_string[5] = "";
DIALOG eof_place_trainer_dialog[] =
{
   /* (proc)                (x) (y) (w)  (h)   (fg) (bg) (key) (flags)    (d1) (d2) (dp)                  (dp2) (dp3) */
   { d_agup_window_proc,    0,  20, 260, 160,  2,   23,  0,    0,         0,   0,   "Place Trainer Event",NULL, NULL },
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

DIALOG eof_rocksmith_section_dialog[] =
{
   /* (proc)             (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                     (dp2) (dp3) */
   { d_agup_window_proc, 0,   0,   290, 450, 2,   23,  0,    0,      0,   0,   "Add Rocksmith section", NULL, NULL },
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
   { d_agup_window_proc, 0,   0,   286, 180, 2,   23,  0,    0,      0,   0,   "Add Rocksmith event", NULL, NULL },
   { d_agup_list_proc,   12,  35,  260, 70,  2,   23,  0,    0,      0,   0,   (void *)eof_rs_event_add_list, NULL, NULL },
   { d_agup_check_proc,  12,  112, 250, 16,  0,   0,   0,    0,      1,   0,   eof_events_add_dialog_string,  NULL, NULL },
   { d_agup_button_proc, 12,  140, 68,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",                    NULL, NULL },
   { d_agup_button_proc, 206, 140, 68,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",                NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

void eof_prepare_beat_menu(void)
{
	unsigned long i;

	if(eof_song && eof_song_loaded)
	{	//If a song is loaded
		//Several beat menu items are disabled below if the tempo map is locked.  Clear those items' flags in case the lock was removed
		eof_beat_menu[0].flags = 0;		//BPM change
		eof_beat_menu[4].flags = 0;		//Add
		eof_beat_menu[5].flags = 0;		//Delete
		eof_beat_menu[6].flags = 0;		//Push offset back
		eof_beat_menu[7].flags = 0;		//Push offset up
		eof_beat_menu[8].flags = 0;		//Reset offset to zero
		eof_beat_menu[9].flags = 0;		//Anchor
		eof_beat_menu[10].flags = 0;	//Toggle anchor
		eof_beat_menu[11].flags = 0;	//Delete anchor
		eof_beat_menu[12].flags = 0;	//Reset BPM
		eof_beat_menu[13].flags = 0;	//Calculate BPM
		eof_beat_menu[14].flags = 0;	//Double BPM
		eof_beat_menu[15].flags = 0;	//Halve BPM
		eof_beat_menu[16].flags = 0;	//Adjust tempo for RBN

//Beat>Add and Delete validation
		if(eof_find_next_anchor(eof_song, eof_selected_beat) < 0)
		{	//If there are no anchors after the selected beat, disable Beat>Add and Delete, as they'd have no effect
			eof_beat_menu[4].flags = D_DISABLED;
			eof_beat_menu[5].flags = D_DISABLED;
		}
		else
		{
			eof_beat_menu[4].flags = 0;
			eof_beat_menu[5].flags = 0;
		}
		if(eof_selected_beat == 0)
		{	//If the first beat marker is selected, disable Beat>Delete, as this beat is not allowed to be deleted
			eof_beat_menu[5].flags = D_DISABLED;
		}
//Beat>Push Offset Up and Push Offset Back validation
		if(eof_song->beat[0]->pos >= eof_song->beat[1]->pos - eof_song->beat[0]->pos)
		{	//If the current MIDI delay is at least as long as the first beat's length, enable Beat>Push Offset Back
			eof_beat_menu[6].flags = 0;
		}
		else
		{
			eof_beat_menu[6].flags = D_DISABLED;
		}
		if(eof_song->beats > 1)
		{	//If the chart has at least two beat markers, enable Beat>Push Offset Up
			eof_beat_menu[7].flags = 0;
		}
		else
		{
			eof_beat_menu[7].flags = D_DISABLED;
		}
//Beat>Reset offset to zero validation
		if(eof_song->beat[0]->pos > 0)
		{	//If the current MIDI delay is not zero, enable Beat>Reset offset to zero
			eof_beat_menu[8].flags = 0;
		}
		else
		{
			eof_beat_menu[8].flags = D_DISABLED;
		}
//Beat>Anchor Beat and Toggle Anchor validation
		if(eof_selected_beat != 0)
		{	//If the first beat marker is not selected, enable Beat>Anchor Beat and Toggle Anchor
			eof_beat_menu[9].flags = 0;
			eof_beat_menu[10].flags = 0;
		}
		else
		{
			eof_beat_menu[9].flags = D_DISABLED;
			eof_beat_menu[10].flags = D_DISABLED;
		}
//Beat>Delete Anchor validation
		if((eof_song->beat[eof_selected_beat]->flags & EOF_BEAT_FLAG_ANCHOR) && (eof_selected_beat != 0))
		{	//If the selected beat is an anchor, and the first beat marker is not selected, enable Beat>Delete Anchor
			eof_beat_menu[11].flags = 0;
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
		{	//If there are no tempo changes throughout the entire chart, disable Beat>Reset BPM, as it would have no effect
			eof_beat_menu[12].flags = D_DISABLED;
		}
		else
		{
			eof_beat_menu[12].flags = 0;
		}
//Beat>All Events and Clear Events validation
		if(eof_song->text_events > 0)
		{	//If there is at least one defined text event, enable Beat>All Events and Clear Events
			eof_beat_menu[18].flags = 0;
			eof_beat_menu[20].flags = 0;
		}
		else
		{
			eof_beat_menu[18].flags = D_DISABLED;
			eof_beat_menu[20].flags = D_DISABLED;
		}

		if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If a pro guitar/bass track is active
			eof_beat_menu[21].flags = 0;	//Place Trainer Event
			eof_beat_menu[22].flags = 0;	//Place RS phrase
			eof_beat_menu[23].flags = 0;	//Place RS section
			eof_beat_menu[24].flags = 0;	//Place RS event
		}
		else
		{
			eof_beat_menu[21].flags = D_DISABLED;
			eof_beat_menu[22].flags = D_DISABLED;
			eof_beat_menu[23].flags = D_DISABLED;
			eof_beat_menu[24].flags = D_DISABLED;
		}
//Re-flag the active Time Signature for the selected beat
		for(i = 0; i < 6; i++)
		{
			eof_beat_time_signature_menu[i].flags = 0;
		}
		if(eof_song->beat[eof_selected_beat]->flags & EOF_BEAT_FLAG_START_4_4)
		{
			eof_beat_time_signature_menu[0].flags = D_SELECTED;
		}
		else if(eof_song->beat[eof_selected_beat]->flags & EOF_BEAT_FLAG_START_3_4)
		{
			eof_beat_time_signature_menu[1].flags = D_SELECTED;
		}
		else if(eof_song->beat[eof_selected_beat]->flags & EOF_BEAT_FLAG_START_5_4)
		{
			eof_beat_time_signature_menu[2].flags = D_SELECTED;
		}
		else if(eof_song->beat[eof_selected_beat]->flags & EOF_BEAT_FLAG_START_6_4)
		{
			eof_beat_time_signature_menu[3].flags = D_SELECTED;
		}
		else if(eof_song->beat[eof_selected_beat]->flags & EOF_BEAT_FLAG_CUSTOM_TS)
		{
			eof_beat_time_signature_menu[4].flags = D_SELECTED;
		}
		else
		{
			eof_beat_time_signature_menu[5].flags = D_SELECTED;
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
			eof_beat_menu[15].flags = D_DISABLED;	//Disable Beat>Halve BPM
		}
		else
		{
			eof_beat_menu[15].flags = 0;
		}

		if(eof_song->tags->tempo_map_locked)
		{	//If the chart's tempo map is locked, disable various beat operations
			eof_beat_menu[0].flags = D_DISABLED;	//BPM change
			eof_beat_menu[4].flags = D_DISABLED;	//Add
			eof_beat_menu[5].flags = D_DISABLED;	//Delete
			eof_beat_menu[6].flags = D_DISABLED;	//Push offset back
			eof_beat_menu[7].flags = D_DISABLED;	//Push offset up
			eof_beat_menu[8].flags = D_DISABLED;	//Reset offset to zero
			eof_beat_menu[9].flags = D_DISABLED;	//Anchor
			eof_beat_menu[10].flags = D_DISABLED;	//Toggle anchor
			eof_beat_menu[11].flags = D_DISABLED;	//Delete anchor
			eof_beat_menu[12].flags = D_DISABLED;	//Reset BPM
			eof_beat_menu[13].flags = D_DISABLED;	//Calculate BPM
			eof_beat_menu[14].flags = D_DISABLED;	//Double BPM
			eof_beat_menu[15].flags = D_DISABLED;	//Halve BPM
			eof_beat_menu[16].flags = D_DISABLED;	//Adjust tempo for RBN
		}
	}//If a song is loaded
}

int eof_menu_beat_bpm_change(void)
{
	int i;
	unsigned long cppqn, newppqn;
	int old_flags;
	double oldbpm;

	if(eof_song->tags->tempo_map_locked || (eof_selected_beat >= eof_song->beats))	//If the chart's tempo map is locked, or an invalid beat is selected
		return 1;							//Return without making changes

	cppqn = eof_song->beat[eof_selected_beat]->ppqn;
	old_flags = eof_song->beat[eof_selected_beat]->flags;
	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_bpm_change_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_bpm_change_dialog);
	oldbpm = 60000000.0 / (double)eof_song->beat[eof_selected_beat]->ppqn;
	(void) snprintf(eof_etext, sizeof(eof_etext) - 1, "%3.2f", oldbpm);
	eof_bpm_change_dialog[3].flags = 0;
	eof_bpm_change_dialog[4].flags = 0;
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
		if(bpm < 0.1)
		{
			eof_render();
			allegro_message("BPM must be at least 0.1.");
			eof_cursor_visible = 1;
			eof_pen_visible = 1;
			eof_show_mouse(NULL);
			return 1;
		}
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		if(eof_bpm_change_dialog[4].flags == D_SELECTED)
		{	//If the "Adjust Notes" option was selected
			eof_song->beat[eof_selected_beat]->flags |= EOF_BEAT_FLAG_ANCHOR;	//Ensure the selected beat is anchored before calling the auto-adjust logic
			(void) eof_menu_edit_cut(eof_selected_beat + 1, 1);
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
			(void) eof_menu_edit_cut_paste(eof_selected_beat + 1, 1);
		}
		eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current
	}
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(NULL);
	return 1;
}

int eof_menu_beat_ts_4_4(void)
{
	(void) eof_apply_ts(4,4,eof_selected_beat,eof_song,1);
	eof_select_beat(eof_selected_beat);
	return 1;
}

int eof_menu_beat_ts_3_4(void)
{
	(void) eof_apply_ts(3,4,eof_selected_beat,eof_song,1);
	eof_select_beat(eof_selected_beat);
	return 1;
}

int eof_menu_beat_ts_5_4(void)
{
	(void) eof_apply_ts(5,4,eof_selected_beat,eof_song,1);
	eof_select_beat(eof_selected_beat);
	return 1;
}

int eof_menu_beat_ts_6_4(void)
{
	(void) eof_apply_ts(6,4,eof_selected_beat,eof_song,1);
	eof_select_beat(eof_selected_beat);
	return 1;
}

DIALOG eof_custom_ts_dialog[] =
{
   /* (proc)				(x)		(y)		(w)		(h)  		(fg)	(bg) (key) (flags)	(d1) (d2) (dp)			(dp2) (dp3) */
   { d_agup_shadow_box_proc,32,		68,		175, 	72 + 8 +15,	2,		23,  0,    0,		0,   0,   NULL,			NULL, NULL },
   { d_agup_text_proc,		42,		84,		35,		8,			2,		23,  0,    0,		0,   0,   "Beats per measure:",	NULL, NULL },
   { eof_verified_edit_proc,160,	80,		35,		20,			2,		23,  0,    0,		8,   0,   eof_etext,	"0123456789", NULL },
   { d_agup_text_proc,		42,		105,	35,		8,			2,		23,  0,    0,		0,   0,   "Beat unit:",	NULL, NULL },
   { eof_verified_edit_proc,160,	101,	35,		20,			2,		23,  0,    0,		8,   0,   eof_etext2,	"0123456789", NULL },
   { d_agup_button_proc,	42,		125,	68,		28,			2,		23,  '\r', D_EXIT,	0,   0,   "OK",			NULL, NULL },
   { d_agup_button_proc,	125,	125,	68,		28,			2,		23,  0,    D_EXIT,	0,   0,   "Cancel",		NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

int eof_menu_beat_ts_custom(void)
{
	unsigned num=0,den=0;

//Prompt the user for the custom time signature
	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_custom_ts_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_custom_ts_dialog);
	(void) snprintf(eof_etext, sizeof(eof_etext) - 1, "%lu", ((eof_song->beat[eof_selected_beat]->flags & 0xFF000000)>>24) + 1);
	(void) snprintf(eof_etext2, sizeof(eof_etext2) - 1, "%lu", ((eof_song->beat[eof_selected_beat]->flags & 0x00FF0000)>>16) + 1);
	if(eof_popup_dialog(eof_custom_ts_dialog, 2) == 5)
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
				eof_select_beat(eof_selected_beat);
			}
		}
	}

	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(NULL);
	return 1;
}

int eof_menu_beat_ts_off(void)
{
//Clear the beat's status except for its anchor and event flags
	int flags = eof_song->beat[eof_selected_beat]->flags & EOF_BEAT_FLAG_ANCHOR;
	flags |= eof_song->beat[eof_selected_beat]->flags & EOF_BEAT_FLAG_EVENTS;
	if(flags != eof_song->beat[eof_selected_beat]->flags)
	{	//If the user has changed the time signature status of this beat
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		eof_song->beat[eof_selected_beat]->flags = flags;
		eof_select_beat(eof_selected_beat);
		eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current
	}
	return 1;
}

int eof_menu_beat_delete(void)
{
	int flags = eof_song->beat[eof_selected_beat]->flags;

	if(eof_song->tags->tempo_map_locked)	//If the chart's tempo map is locked
		return 1;							//Return without making changes

	if((eof_selected_beat > 0) && (eof_find_next_anchor(eof_song, eof_selected_beat) >= 0))
	{	//Only process this function if a beat other than beat 0 is selected, and there is at least one anchor after the selected beat
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		eof_song_delete_beat(eof_song, eof_selected_beat);
		if((eof_song->beat[eof_selected_beat - 1]->flags & EOF_BEAT_FLAG_ANCHOR) && (eof_song->beat[eof_selected_beat]->flags & EOF_BEAT_FLAG_ANCHOR))
		{
			double beats_length = eof_song->beat[eof_selected_beat]->pos - eof_song->beat[eof_selected_beat - 1]->pos;
			double newbpm = (double)60000.0 / beats_length;
			double newppqn = (double)60000000.0 / newbpm;
			eof_song->beat[eof_selected_beat - 1]->ppqn = newppqn;
		}
		else if(eof_song->beat[eof_selected_beat - 1]->flags & EOF_BEAT_FLAG_ANCHOR)
		{
			eof_realign_beats(eof_song, eof_selected_beat);
		}
		else
		{
			eof_realign_beats(eof_song, eof_selected_beat - 1);
		}
		eof_move_text_events(eof_song, eof_selected_beat, 1, -1);
		flags &= (~EOF_BEAT_FLAG_ANCHOR);	//Clear the anchor flag
		eof_song->beat[eof_selected_beat - 1]->flags |= flags;
		eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current
	}
	return 1;
}

int eof_menu_beat_push_offset_back(char *undo_made)
{
	unsigned long i;
	unsigned long backamount = eof_song->beat[1]->pos - eof_song->beat[0]->pos;

	if(eof_song->tags->tempo_map_locked)	//If the chart's tempo map is locked
		return 0;							//Return without making changes
	if(!undo_made)
		return 0;	//Invalid parameter

	if(eof_song->beat[0]->pos >= backamount)
	{
		if(*undo_made == 0)
		{	//If an undo state needs to be made
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			*undo_made = 1;
		}
		if(eof_song_resize_beats(eof_song, eof_song->beats + 1))
		{	//If the beats array was successfully resized
			for(i = eof_song->beats - 1; i > 0; i--)
			{
				memcpy(eof_song->beat[i], eof_song->beat[i - 1], sizeof(EOF_BEAT_MARKER));
			}
			eof_song->beat[0]->pos = eof_song->beat[1]->pos - backamount;
			eof_song->beat[0]->fpos = eof_song->beat[0]->pos;
			eof_song->beat[1]->flags = 0;
			eof_song->tags->ogg[eof_selected_ogg].midi_offset = eof_song->beat[0]->pos;
			eof_move_text_events(eof_song, 0, 1, 1);
			eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current
		}
		else
			return 0;	//Return failure
	}
	return 1;	//Return success
}

int eof_menu_beat_push_offset_back_menu(void)
{
	char undo_made = 0;

	return eof_menu_beat_push_offset_back(&undo_made);
}

int eof_menu_beat_push_offset_up(void)
{
	int i;

	if(eof_song->tags->tempo_map_locked)	//If the chart's tempo map is locked
		return 1;							//Return without making changes

	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	for(i = 0; i < eof_song->beats - 1; i++)
	{
		memcpy(eof_song->beat[i], eof_song->beat[i + 1], sizeof(EOF_BEAT_MARKER));
	}
	eof_song_delete_beat(eof_song, eof_song->beats - 1);
	eof_song->tags->ogg[eof_selected_ogg].midi_offset = eof_song->beat[0]->pos;
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

	if(eof_song->tags->tempo_map_locked)	//If the chart's tempo map is locked
	{
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
		if(alert(NULL, "Insert evenly spaced beats at the beginning of the chart?", NULL, "&Yes", "&No", 'y', 'n') == 1)
		{	//If user opts to insert evenly spaced beats
			while(eof_song->beat[0]->pos >= eof_song->beat[1]->pos - eof_song->beat[0]->pos)
			{	//While the MIDI delay is still large enough to be moved back one beat
				if(!eof_menu_beat_push_offset_back(&undo_made))
				{	//If pushing back the chart by one beat failed
					break;	//Break from this loop and attempt to finish the normal reset offset logic
				}
			}
		}
	}

	if(eof_song->beat[0]->pos > 0)
	{	//Only allow this function to run if the current MIDI delay is above zero
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
			eof_song->tags->ogg[eof_selected_ogg].midi_offset = 0;
			newbpm = 60000.0 / eof_song->beat[1]->pos;	//60000ms / length of new beat (the MIDI delay) = Tempo
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
	}
	return 1;	//Return success
}

int eof_menu_beat_anchor(void)
{
	int mm, ss, ms;
	int oldmm, oldss, oldms;
	int oldpos = eof_song->beat[eof_selected_beat]->pos;
	int newpos = 0;
	int revert = 0;
	char ttext[4] = {0};

	if(eof_song->tags->tempo_map_locked)	//If the chart's tempo map is locked
		return 1;							//Return without making changes

	if(eof_selected_beat == 0)
	{	//This function is not allowed on the first beat marker
		return 1;
	}
	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_anchor_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_anchor_dialog);
	oldmm = (eof_song->beat[eof_selected_beat]->pos / 1000) / 60;
	oldss = (eof_song->beat[eof_selected_beat]->pos / 1000) % 60;
	oldms = (eof_song->beat[eof_selected_beat]->pos) % 1000;
	(void) snprintf(eof_etext2, sizeof(eof_etext2) - 1, "%02d:%02d.%03d", oldmm, oldss, oldms);
	if(eof_popup_dialog(eof_anchor_dialog, 2) == 3)
	{
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
		newpos = (double)mm * 60.0 * 1000.0 + (double)ss * 1000.0 + (double)ms;
		if(newpos > oldpos)
		{
			while(eof_song->beat[eof_selected_beat]->pos < newpos)
			{
				eof_song->beat[eof_selected_beat]->fpos++;
				eof_song->beat[eof_selected_beat]->pos = eof_song->beat[eof_selected_beat]->fpos + 0.5;	//Round up to nearest ms
				eof_mickeys_x = 1;
				eof_recalculate_beats(eof_song, eof_selected_beat);
				if(eof_song->beat[eof_selected_beat]->pos > eof_song->beat[eof_selected_beat + 1]->pos - 100)
				{
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
	if(eof_song->tags->tempo_map_locked)	//If the chart's tempo map is locked
		return 1;							//Return without making changes

	if(eof_selected_beat > 0)
	{
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		eof_song->beat[eof_selected_beat]->flags ^= EOF_BEAT_FLAG_ANCHOR;
	}
	return 1;
}

int eof_menu_beat_delete_anchor(void)
{
	int i;
	unsigned long cppqn = eof_song->beat[eof_selected_beat]->ppqn;

	if(eof_song->tags->tempo_map_locked)	//If the chart's tempo map is locked
		return 1;							//Return without making changes

	if((eof_selected_beat > 0) && eof_beat_is_anchor(eof_song, eof_selected_beat))
	{
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
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
		eof_song->beat[eof_selected_beat]->flags = 0;
		if(eof_find_next_anchor(eof_song, eof_selected_beat) < 0)
		{
			(void) eof_song_resize_beats(eof_song, eof_selected_beat);
			eof_calculate_beats(eof_song);
		}
		else
		{
			eof_realign_beats(eof_song, eof_selected_beat);
		}
	}
	eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current
	return 1;
}

int eof_menu_beat_reset_bpm(void)
{
	int i;
	int reset = 0;

	if(eof_song->tags->tempo_map_locked)	//If the chart's tempo map is locked
		return 1;							//Return without making changes

	for(i = 1; i < eof_song->beats; i++)
	{
		if(eof_song->beat[i]->ppqn != eof_song->beat[0]->ppqn)
		{
			reset = 1;
			break;
		}
	}
	if(reset)
	{
		if(alert(NULL, "Erase all BPM changes?", NULL, "OK", "Cancel", 0, 0) == 1)
		{
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			for(i = 1; i < eof_song->beats; i++)
			{
				eof_song->beat[i]->ppqn = eof_song->beat[0]->ppqn;
				eof_song->beat[i]->flags = eof_song->beat[i]->flags & EOF_BEAT_FLAG_EVENTS;
			}
			eof_calculate_beats(eof_song);
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

int eof_menu_beat_calculate_bpm(void)
{
	unsigned long i;
	int first = 1;
	unsigned long curpos = 0;
	unsigned long delta;
	unsigned long cppqn = eof_song->beat[eof_selected_beat]->ppqn;
	double bpm = 0.0;
	unsigned long bpm_count = 0;

	if(eof_song->tags->tempo_map_locked)	//If the chart's tempo map is locked
		return 1;							//Return without making changes

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if(eof_selection.multi[i])
		{
			if(first)
			{
				curpos = eof_get_note_pos(eof_song, eof_selected_track, i);
				first = 0;
			}
			else
			{
				delta = eof_get_note_pos(eof_song, eof_selected_track, i) - curpos;
				curpos = eof_get_note_pos(eof_song, eof_selected_track, i);
				bpm += (double)60000 / (double)delta;
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
				eof_song->beat[i]->ppqn = (double)60000000.0 / bpm;
			}
			else
			{
				break;
			}
		}
	}
	eof_calculate_beats(eof_song);
	eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current
	return 1;
}

int eof_menu_beat_all_events(void)
{
	unsigned long track, realindex, flags;
	int retval;
	char undo_made = 0;

	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_all_events_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_all_events_dialog);
	eof_all_events_dialog[1].d1 = 0;
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
		if(retval == 2)
		{	//User clicked Find
			realindex = eof_retrieve_text_event(eof_all_events_dialog[1].d1);	//Find the actual event, taking the display filter into account
			if(realindex < eof_song->text_events)
			{
				eof_set_seek_position(eof_song->beat[eof_song->text_event[realindex]->beat]->pos + eof_av_delay);
				eof_selected_beat = eof_song->text_event[realindex]->beat;
				track = eof_song->text_event[realindex]->track;
				if((track != 0) && (track < eof_song->tracks))
				{	//If this is a track-specific event
					(void) eof_menu_track_selected_track_number(track);	//Change to that track
				}
			}
			break;	//Break from loop
		}
		else if(retval == 4)
		{	//User clicked Copy to selected beat
			realindex = eof_retrieve_text_event(eof_all_events_dialog[1].d1);	//Find the actual event, taking the display filter into account
			if(realindex < eof_song->text_events)
			{
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
	centre_dialog(eof_events_dialog);
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
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(NULL);
	return 1;
}

int eof_menu_beat_clear_events(void)
{
	int i;
	if(eof_song->text_events == 0)
	{
		allegro_message("No events to clear!");
		return 1;
	}
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

char * eof_events_list(int index, int * size)
{
	int i;
	int ecount = 0;
	char trackname[25] = {0};

	for(i = 0; i < eof_song->text_events; i++)
	{
		if(eof_song->text_event[i]->beat == eof_selected_beat)
		{
			if(ecount < EOF_MAX_TEXT_EVENTS)
			{
				if((eof_song->text_event[i]->track != 0) && (eof_song->text_event[i]->track < eof_song->tracks))
				{	//If this is a track specific event
					(void) snprintf(trackname, sizeof(trackname) - 1, "(%s) ", eof_song->track[eof_song->text_event[i]->track]->name);
				}
				else
				{
					trackname[0] = '\0';	//Empty the string
				}
				(void) snprintf(eof_event_list_text[ecount], sizeof(eof_event_list_text[ecount]) - 1, "%s%s", trackname, eof_song->text_event[i]->text);
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

char * eof_events_list_all(int index, int * size)
{
	char trackname[22] = {0};
	unsigned long x, count = 0, realindex;

	if(index < 0)
	{	//Signal to return the list count
		if(eof_all_events_dialog[5].flags & D_SELECTED)
		{	//Display all events
			count = eof_song->text_events;
		}
		else if(eof_all_events_dialog[6].flags & D_SELECTED)
		{	//Display this track's events
			for(x = 0; x < eof_song->text_events; x++)
			{	//For each event
				if(eof_song->text_event[x]->track == eof_selected_track)
				{	//If the event is specific to the currently active track
					count++;
				}
			}
		}
		else if(eof_all_events_dialog[7].flags & D_SELECTED)
		{	//Display section events
			for(x = 0; x < eof_song->text_events; x++)
			{	//For each event
				if(eof_is_section_marker(eof_song->text_event[x], 0))
				{	//If the text event's string or flags indicate a section marker (regardless of the event's associated track
					count++;
				}
			}
		}
		else if(eof_all_events_dialog[8].flags & D_SELECTED)
		{	//Display Rocksmith sections
			for(x = 0; x < eof_song->text_events; x++)
			{	//For each event
				if(eof_song->text_event[x]->flags & EOF_EVENT_FLAG_RS_SECTION)
				{	//If the event is marked as a Rocksmith section
					count++;
				}
			}
		}
		else
		{	//Display Rocksmith events
			for(x = 0; x < eof_song->text_events; x++)
			{	//For each event
				if(eof_song->text_event[x]->flags & EOF_EVENT_FLAG_RS_EVENT)
				{	//If the event is marked as a Rocksmith event
					count++;
				}
			}
		}
		*size = count;
	}
	else
	{	//Return the specified list item
		realindex = eof_retrieve_text_event(index);	//Get the actual event based on the current filter
		if(eof_song->text_event[realindex]->beat >= eof_song->beats)
		{	//Something bad happened, repair the event
			eof_song->text_event[realindex]->beat = eof_song->beats - 1;	//Reset the text event to be at the last beat marker
		}
		if((eof_song->text_event[realindex]->track != 0) && (eof_song->text_event[realindex]->track < eof_song->tracks))
		{	//If this is a track specific event
			(void) snprintf(trackname, sizeof(trackname) - 1, " %s", eof_song->track[eof_song->text_event[realindex]->track]->name);
		}
		else
		{
			trackname[0] = '\0';	//Empty the string
		}
		(void) snprintf(eof_event_list_text[index], sizeof(eof_event_list_text[index]) - 1, "(%02lu:%02lu.%03lu%s) %s", eof_song->beat[eof_song->text_event[realindex]->beat]->pos / 60000, (eof_song->beat[eof_song->text_event[realindex]->beat]->pos / 1000) % 60, eof_song->beat[eof_song->text_event[realindex]->beat]->pos % 1000, trackname, eof_song->text_event[realindex]->text);
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
	return eof_events_dialog_add_function(EOF_EVENT_FLAG_RS_PHRASE);	//Call the add text event dialog, automatically checking the RS phrase marker option
}

int eof_events_dialog_add_function(char function)
{
	int i;
	unsigned long track = 0, flags = 0;
	char *rssectionname;

	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_events_add_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_events_add_dialog);
	(void) ustrcpy(eof_etext, "");
	(void) snprintf(eof_events_add_dialog_string, sizeof(eof_events_add_dialog_string) - 1, "Specific to %s", eof_song->track[eof_selected_track]->name);
	eof_events_add_dialog[3].flags = 0;	//By default, this is not a track specific event
	if(function & EOF_EVENT_FLAG_RS_PHRASE)
	{	//If the calling function wanted to automatically enable the "Rocksmith phrase marker" checkbox
		eof_events_add_dialog[4].flags = D_SELECTED;
	}
	else
	{	//Otherwise clear it
		eof_events_add_dialog[4].flags = 0;
	}
	if(function & EOF_EVENT_FLAG_RS_SECTION)
	{	//If the calling function wanted to automatically enable the "Rocksmith section marker" checkbox
		eof_events_add_dialog[5].flags = D_SELECTED;
	}
	else
	{	//Otherwise clear it
		eof_events_add_dialog[5].flags = 0;
	}
	if(function & EOF_EVENT_FLAG_RS_EVENT)
	{	//If the calling function wanted to automatically enable the "Rocksmith event marker" checkbox
		eof_events_add_dialog[6].flags = D_SELECTED;
	}
	else
	{	//Otherwise clear it
		eof_events_add_dialog[6].flags = 0;
	}
	if(eof_popup_dialog(eof_events_add_dialog, 2) == 7)
	{	//User clicked OK
		char *effective_text = eof_etext;	//By default, use the user-input string

		if((ustrlen(eof_etext) > 0) && eof_check_string(eof_etext))
		{	//User entered text that isn't all space characters
			if((eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT) && (eof_events_add_dialog[3].flags & D_SELECTED) && (eof_events_add_dialog[4].flags & D_SELECTED))
			{	//If the user is tried to add a track specific phrase marker in a non pro guitar/bass track
				allegro_message("You cannot add a track-specific Rocksmith phrase marker in a non pro guitar/bass track");
				return D_REDRAW;
			}
			if(eof_events_add_dialog[3].flags & D_SELECTED)
			{	//User opted to make this a track specific event
				track = eof_selected_track;
			}
			if(eof_events_add_dialog[4].flags & D_SELECTED)
			{	//User opted to make this a Rocksmith phrase marker
				flags |= EOF_EVENT_FLAG_RS_PHRASE;
			}
			if(eof_events_add_dialog[5].flags & D_SELECTED)
			{	//User opted to make this a Rocksmith section marker
				rssectionname = eof_rs_section_text_valid(eof_etext);	//Determine whether this is a valid Rocksmith section name
				if(rssectionname && ustrcmp(rssectionname, eof_etext) && !(eof_events_add_dialog[4].flags & D_SELECTED) && !(eof_events_add_dialog[6].flags & D_SELECTED))
				{	//If this doesn't match a valid RS native name (case sensitive), but matches a valid Rocksmith section native/display name (case insensitive) and isn't marked as a RS phrase/event
					effective_text = rssectionname;	//Use the matching RS section's native name instead of the user-supplied text
				}
				else if(!rssectionname || ustrcmp(rssectionname, eof_etext))
				{	//Otherwise if this isn't a valid Rocksmith native section name (case sensitive), the user will have to resolve the conflict manually
					allegro_message("Warning:  This is not a valid Rocksmith section.  Please edit it appropriately or remove it and re-add it using Beat>Place Rocksmith Section");
				}
				flags |= EOF_EVENT_FLAG_RS_SECTION;
			}
			if(eof_events_add_dialog[6].flags & D_SELECTED)
			{	//User opted to make this a Rocksmith event marker
				if(!eof_rs_event_text_valid(eof_etext))
				{	//If this isn't a valid Rocksmith event name
					allegro_message("Warning:  This is not a valid Rocksmith event.  Please edit it appropriately or remove it and re-add it using Beat>Place Rocksmith Event");
				}
				flags |= EOF_EVENT_FLAG_RS_EVENT;
			}
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			(void) eof_song_add_text_event(eof_song, eof_selected_beat, effective_text, track, flags, 0);
			eof_sort_events(eof_song);
			eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current
		}
	}
	eof_render();
	(void) dialog_message(eof_events_dialog, MSG_DRAW, 0, &i);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(screen);
	return D_O_K;
}

int eof_events_dialog_edit(DIALOG * d)
{
	int i, trackflag, phraseflag, sectionflag, eventflag;
	short ecount = 0;
	short event = -1;
	unsigned long track = 0, flags = 0;
	char *rssectionname;

	if(!d)
	{	//Satisfy Splint by checking value of d
		return D_O_K;
	}

	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_events_add_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_events_add_dialog);

	/* find the event */
	for(i = 0; i < eof_song->text_events; i++)
	{
		if(eof_song->text_event[i]->beat == eof_selected_beat)
		{
			/* if we've reached the item that is selected, edit it */
			if(eof_events_dialog[1].d1 == ecount)
			{
				event = i;
				break;
			}

			/* go to next event */
			else
			{
				ecount++;
			}
		}
	}

	if((eof_song->text_event[event]->track != 0) && (eof_song->text_event[event]->track != eof_selected_track))
	{	//If this is a track specific event, and it doesn't belong to the active track
		(void) dialog_message(eof_events_dialog, MSG_DRAW, 0, &i);
		eof_cursor_visible = 1;
		eof_pen_visible = 1;
		eof_show_mouse(screen);
		return D_O_K;	//Don't allow it to be edited here
	}
	(void) snprintf(eof_events_add_dialog_string, sizeof(eof_events_add_dialog_string) - 1, "Specific to %s", eof_song->track[eof_selected_track]->name);
	if(eof_song->text_event[event]->track == eof_selected_track)
	{	//If this event is specific to this track
		eof_events_add_dialog[3].flags = D_SELECTED;	//Set the checkbox specifying the event is track specific
	}
	else
	{	//Otherwise clear the checkbox
		eof_events_add_dialog[3].flags = 0;
	}
	if(eof_song->text_event[event]->flags & EOF_EVENT_FLAG_RS_PHRASE)
	{	//If this event is flagged as a Rocksmith phrase marker
		eof_events_add_dialog[4].flags = D_SELECTED;	//Set the checkbox specifying the event is a Rocksmith phrase marker
	}
	else
	{
		eof_events_add_dialog[4].flags = 0;
	}
	if(eof_song->text_event[event]->flags & EOF_EVENT_FLAG_RS_SECTION)
	{	//If this event is flagged as a Rocksmith section marker
		eof_events_add_dialog[5].flags = D_SELECTED;	//Set the checkbox specifying the event is a Rocksmith section marker
	}
	else
	{
		eof_events_add_dialog[5].flags = 0;
	}
	if(eof_song->text_event[event]->flags & EOF_EVENT_FLAG_RS_EVENT)
	{	//If this event is flagged as a Rocksmith event marker
		eof_events_add_dialog[6].flags = D_SELECTED;	//Set the checkbox specifying the event is a Rocksmith event marker
	}
	else
	{
		eof_events_add_dialog[6].flags = 0;
	}

	(void) ustrcpy(eof_etext, eof_song->text_event[event]->text);	//Save the original event text
	trackflag = eof_events_add_dialog[3].flags;				//Save the track specifier flag
	phraseflag = eof_events_add_dialog[4].flags;			//Save the RS phrase flag
	sectionflag = eof_events_add_dialog[5].flags;			//Save the RS section flag
	eventflag = eof_events_add_dialog[6].flags;				//Save the RS event flag
	if(eof_popup_dialog(eof_events_add_dialog, 2) == 7)
	{	//User clicked OK
		char *effective_text = eof_etext;	//By default, use the user-input string

		if((ustrlen(eof_etext) > 0) && eof_check_string(eof_etext) && (ustrcmp(eof_song->text_event[event]->text, eof_etext) || (eof_events_add_dialog[3].flags != trackflag) || (eof_events_add_dialog[4].flags != phraseflag) || (eof_events_add_dialog[5].flags != sectionflag)|| (eof_events_add_dialog[6].flags != eventflag)))
		{	//User entered text that isn't all space characters, and either the event's text was changed or one of it's flags were
			if((eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT) && (eof_events_add_dialog[3].flags & D_SELECTED) && (eof_events_add_dialog[4].flags & D_SELECTED))
			{	//If the user is tried to add a track specific phrase marker in a non pro guitar/bass track
				allegro_message("You cannot add a track-specific Rocksmith phrase marker in a non pro guitar/bass track");
				return D_REDRAW;
			}
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			if(eof_events_add_dialog[3].flags & D_SELECTED)
			{	//User opted to make this a track specific event
				track = eof_selected_track;
			}
			if(eof_events_add_dialog[4].flags & D_SELECTED)
			{	//User opted to make this a Rocksmith phrase marker
				flags |= EOF_EVENT_FLAG_RS_PHRASE;
			}
			if(eof_events_add_dialog[5].flags & D_SELECTED)
			{	//User opted to make this a Rocksmith section marker
				rssectionname = eof_rs_section_text_valid(eof_etext);	//Determine whether this is a valid Rocksmith section name
				if(rssectionname && ustrcmp(rssectionname, eof_etext) && !(eof_events_add_dialog[4].flags & D_SELECTED) && !(eof_events_add_dialog[6].flags & D_SELECTED))
				{	//If this doesn't match a valid RS native name (case sensitive), but matches a valid Rocksmith section native/display name (case insensitive) and isn't marked as a RS phrase/event
					effective_text = rssectionname;	//Use the matching RS section's native name instead of the user-supplied text
				}
				else if(!rssectionname || ustrcmp(rssectionname, eof_etext))
				{	//Otherwise if this isn't a valid Rocksmith native section name (case sensitive), the user will have to resolve the conflict manually
					allegro_message("Warning:  This is not a valid Rocksmith section.  Please edit it appropriately or remove it and re-add it using Beat>Place Rocksmith Section");
				}
				flags |= EOF_EVENT_FLAG_RS_SECTION;
			}
			if(eof_events_add_dialog[6].flags & D_SELECTED)
			{	//User opted to make this a Rocksmith event marker
				if(!eof_rs_event_text_valid(eof_etext))
				{	//If this isn't a valid Rocksmith event name
					allegro_message("Warning:  This is not a valid Rocksmith event.  Please edit it appropriately or remove it and re-add it using Beat>Place Rocksmith Event");
				}
				flags |= EOF_EVENT_FLAG_RS_EVENT;
			}
			(void) ustrcpy(eof_song->text_event[event]->text, effective_text);
			eof_song->text_event[event]->track = track;
			eof_song->text_event[event]->flags = flags;
			eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current
		}
	}//User clicked OK
	eof_render();
	(void) dialog_message(eof_events_dialog, MSG_DRAW, 0, &i);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(screen);
	return D_O_K;
}

int eof_events_dialog_delete_events_count(void)
{
	int i;
	int count = 0;

	for(i = 0; i < eof_song->text_events; i++)
	{
		if(eof_song->text_event[i]->beat == eof_selected_beat)
		{
			count++;
		}
	}
	return count;
}

int eof_events_dialog_delete(DIALOG * d)
{
	int i, c;
	int ecount = 0;

	if(!d)
	{	//Satisfy Splint by checking value of d
		return D_O_K;
	}
	if(eof_song->text_events == 0)
	{
		return D_O_K;
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	for(i = 0; i < eof_song->text_events; i++)
	{
		if(eof_song->text_event[i]->beat == eof_selected_beat)
		{
			/* if we've reached the item that is selected, delete it */
			if(eof_events_dialog[1].d1 == ecount)
			{
				/* remove the text event and exit */
				eof_song_delete_text_event(eof_song, i);
				eof_sort_events(eof_song);

				/* remove flag if no more events tied to this beat */
				c = eof_events_dialog_delete_events_count();
				if((eof_events_dialog[1].d1 >= c) && (c > 0))
				{
					eof_events_dialog[1].d1--;
				}
				(void) dialog_message(eof_events_dialog, MSG_DRAW, 0, &i);
				eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current
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

int eof_menu_beat_add(void)
{
	int i;

	if(eof_song->tags->tempo_map_locked)	//If the chart's tempo map is locked
		return 1;							//Return without making changes

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
	else
		return -1;	//Otherwise return error
}

void eof_rebuild_trainer_strings(void)
{
	int relevant_track = eof_selected_track;	//In RB3, pro guitar/bass training events are only stored in the 17 fret track

	if(!eof_song)
		return;	//Return without rebuilding string tunings if there is an error

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
	if(eof_song_contains_event(eof_song, eof_etext2, relevant_track, 0xFFFF))
	{	//If this training event is already defined in the active track
		eof_place_trainer_dialog[3].flags = D_SELECTED | D_DISABLED;	//Check this box
	}
	else
	{
		eof_place_trainer_dialog[3].flags = 0 | D_DISABLED;		//Otherwise clear this box
	}
	if(eof_song_contains_event(eof_song, eof_etext3, relevant_track, 0xFFFF))
	{	//If this training event is already defined in the active track
		eof_place_trainer_dialog[5].flags = D_SELECTED | D_DISABLED;	//Check this box
	}
	else
	{
		eof_place_trainer_dialog[5].flags = 0 | D_DISABLED;		//Otherwise clear this box
	}
	if(eof_song_contains_event(eof_song, eof_etext4, relevant_track, 0xFFFF))
	{	//If this training event is already defined in the active track
		eof_place_trainer_dialog[7].flags = D_SELECTED | D_DISABLED;	//Check this box
	}
	else
	{
		eof_place_trainer_dialog[7].flags = 0 | D_DISABLED;		//Otherwise clear this box
	}
}

int eof_menu_beat_trainer_event(void)
{
	int relevant_track = eof_selected_track;	//In RB3, pro guitar/bass training events are only stored in the 17 fret track
	char *selected_string = NULL;

	eof_color_dialog(eof_place_trainer_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_place_trainer_dialog);
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
	if(eof_popup_dialog(eof_place_trainer_dialog, 2) == 9)
	{	//If user clicked OK button
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

		if(eof_song_contains_event(eof_song, selected_string, relevant_track, 0xFFFF))
		{	//If this training event is already defined in the active track
			if(alert(NULL, "Warning:  This text event already exists in this track.  Continue?", NULL, "&Yes", "&No", 'y', 'n') != 1)
			{	//If the user does not opt to place the duplicate text event
				return 0;
			}
		}
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		(void) eof_song_add_text_event(eof_song, eof_selected_beat, selected_string, relevant_track, 0, 0);	//Add the chosen text event to the selected beat
		eof_sort_events(eof_song);
	}
	return 1;
}

int eof_edit_trainer_proc(int msg, DIALOG *d, int c)
{
	int i;
	char * string = NULL;
	int key_list[32] = {KEY_BACKSPACE, KEY_DEL, KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN, KEY_ESC, KEY_ENTER};
	int match = 0;
	int retval;

	if((msg == MSG_CHAR) || (msg == MSG_UCHAR))
	{	//ASCII is not handled until the MSG_UCHAR event is sent
		for(i = 0; i < 8; i++)
		{	//See if any default accepted input characters were given
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

	return d_agup_edit_proc(msg, d, c);	//Allow the input character to be returned
}

int eof_all_events_radio_proc(int msg, DIALOG *d, int c)
{
	static int previous_option = 5;	//By default, eof_all_events_dialog[5] (all events) is selected
	int selected_option;

	if(msg == MSG_CLICK)
	{
		selected_option = (int)d->dp2;	//Find out which radio button was clicked on

		if(selected_option != previous_option)
		{	//If the event display filter changed, have the event list redrawn
			eof_all_events_dialog[5].flags = eof_all_events_dialog[6].flags = eof_all_events_dialog[7].flags = eof_all_events_dialog[8].flags = eof_all_events_dialog[9].flags = 0;	//Clear all radio buttons
			eof_all_events_dialog[selected_option].flags = D_SELECTED;			//Re-select the radio button that was just clicked on
			eof_all_events_dialog[1].d2 = 0;									//Select first list item, since if it's too high, it will prevent the newly-selected filtered list from displaying
			(void) object_message(&eof_all_events_dialog[1], MSG_DRAW, 0);		//Have Allegro redraw the list of events
			(void) object_message(&eof_all_events_dialog[5], MSG_DRAW, 0);		//Have Allegro redraw the radio buttons
			(void) object_message(&eof_all_events_dialog[6], MSG_DRAW, 0);
			(void) object_message(&eof_all_events_dialog[7], MSG_DRAW, 0);
			(void) object_message(&eof_all_events_dialog[8], MSG_DRAW, 0);
			(void) object_message(&eof_all_events_dialog[9], MSG_DRAW, 0);
			(void) object_message(&eof_all_events_dialog[10], MSG_DRAW, 0);		//Have Allegro redraw the event override string
			previous_option = selected_option;
		}
	}

	return d_agup_radio_proc(msg, d, c);	//Allow the input to be processed
}

unsigned long eof_retrieve_text_event(unsigned long index)
{
	unsigned long x, count = 0;
	if(eof_all_events_dialog[5].flags & D_SELECTED)
	{	//Display all events
		return index;
	}
	else if(eof_all_events_dialog[6].flags & D_SELECTED)
	{	//Display this track's events
		for(x = 0; x < eof_song->text_events; x++)
		{	//For each event
			if(eof_song->text_event[x]->track == eof_selected_track)
			{	//If the event is specific to the currently active track
				if(count == index)
				{	//If the requested event was found
					return x;
				}
				count++;
			}
		}
	}
	else if(eof_all_events_dialog[7].flags & D_SELECTED)
	{	//Display section events
		for(x = 0; x < eof_song->text_events; x++)
		{	//For each event
			if(eof_is_section_marker(eof_song->text_event[x], 0))
			{	//If the text event's string or flags indicate a section marker (regardless of the event's associated track)
				if(count == index)
				{	//If the requested event was found
					return x;
				}
				count++;
			}
		}
	}
	else if(eof_all_events_dialog[8].flags & D_SELECTED)
	{	//Display Rocksmith sections
		for(x = 0; x < eof_song->text_events; x++)
		{	//For each event
			if(eof_song->text_event[x]->flags & EOF_EVENT_FLAG_RS_SECTION)
			{	//If the event is marked as a Rocksmith section
				if(count == index)
				{	//If the requested event was found
					return x;
				}
				count++;
			}
		}
	}
	else
	{	//Display Rocksmith events
		for(x = 0; x < eof_song->text_events; x++)
		{	//For each event
			if(eof_song->text_event[x]->flags & EOF_EVENT_FLAG_RS_EVENT)
			{	//If the event is marked as a Rocksmith section
				if(count == index)
				{	//If the requested event was found
					return x;
				}
				count++;
			}
		}
	}
	return 0;	//If for some reason the requested event was not retrievable, return 0
}

int eof_menu_beat_double_tempo(void)
{
	if(eof_song->tags->tempo_map_locked)	//If the chart's tempo map is locked
		return 1;							//Return without making changes

	eof_double_tempo(eof_song, eof_selected_beat, NULL);
	return 1;
}

int eof_menu_beat_halve_tempo(void)
{
	if(eof_song->tags->tempo_map_locked)	//If the chart's tempo map is locked
		return 1;							//Return without making changes

	(void) eof_halve_tempo(eof_song, eof_selected_beat, NULL);
	return 1;
}

int eof_menu_beat_set_RBN_tempos(void)
{
	char undo_made = 0, changed;
	unsigned long loop_ctr = 0;
	unsigned long i;

	eof_log("eof_move_text_events() entered", 1);

	if(eof_song->tags->tempo_map_locked)	//If the chart's tempo map is locked
		return 1;							//Return without making changes
	if(!eof_song)
	{
		return 1;
	}
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
			alogg_seek_abs_msecs_ogg(eof_music_track, eof_song->beat[i]->pos + eof_av_delay);	//Seek to the offending beat
			eof_music_pos = alogg_get_pos_msecs_ogg(eof_music_track);
			eof_music_actual_pos = eof_music_pos;
			eof_mix_seek(eof_music_pos);
			eof_selected_beat = i;
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
	long targetbeat;
	double newbpm;
	unsigned long newppqn, oldppqn, ctr;

	if(!eof_song)
		return 1;
	if(eof_song->tags->tempo_map_locked)
		return 1;	//Return without changing anything if the chart's tempo map is locked

	targetbeat = eof_get_beat(eof_song, eof_music_pos - eof_av_delay);	//Identify the beat at or before the seek position
	if(targetbeat < 0)
	{	//If the seek position is outside the scope of the chart
		targetbeat = 0;	//Identify the first beat as the target
	}
	oldppqn = eof_song->beat[targetbeat]->ppqn;
	if(eof_input_mode != EOF_INPUT_FEEDBACK)
	{	//If Feedback input method is not in effect, identify the anchor at or before the seek position
		while((targetbeat > 0) && (eof_song->beat[targetbeat - 1]->ppqn == oldppqn))
		{	//While the target is not a tempo change
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
		if(eof_song->beat[ctr]->ppqn == oldppqn)
		{	//If this beat isn't a tempo change
			eof_song->beat[ctr]->ppqn = newppqn;	//Update its tempo
		}

		/* break when we reach the end of the portion to change */
		else
		{
			break;	//Otherwise exit the loop
		}
	}
	eof_calculate_beats(eof_song);
	if(eof_note_auto_adjust)
	{	//If the user has enabled the Note Auto-Adjust preference
		(void) eof_menu_edit_cut_paste(targetbeat + 1, 1);
	}
	eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current
	return 1;
}

int eof_apply_key_signature(int signature, unsigned long beatnum, EOF_SONG *sp)
{
	if((signature < -7) || (signature > 7) || !sp || (beatnum >= sp->beats))
		return 0;	//Return error;

	if((sp->beat[beatnum]->flags & EOF_BEAT_FLAG_KEY_SIG) && (sp->beat[beatnum]->key == signature))
		return 0;	//Don't apply a key signature if the selected beat already has that specific key signature

	eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make an undo state
	sp->beat[beatnum]->flags |= EOF_BEAT_FLAG_KEY_SIG;	//Set this flag
	sp->beat[beatnum]->key = signature;
	return 1;
}

int eof_menu_beat_ks_7_flats(void)
{
	return eof_apply_key_signature(-7, eof_selected_beat, eof_song);
}

int eof_menu_beat_ks_6_flats(void)
{
	return eof_apply_key_signature(-6, eof_selected_beat, eof_song);
}

int eof_menu_beat_ks_5_flats(void)
{
	return eof_apply_key_signature(-5, eof_selected_beat, eof_song);
}

int eof_menu_beat_ks_4_flats(void)
{
	return eof_apply_key_signature(-4, eof_selected_beat, eof_song);
}

int eof_menu_beat_ks_3_flats(void)
{
	return eof_apply_key_signature(-3, eof_selected_beat, eof_song);
}

int eof_menu_beat_ks_2_flats(void)
{
	return eof_apply_key_signature(-2, eof_selected_beat, eof_song);
}

int eof_menu_beat_ks_1_flat(void)
{
	return eof_apply_key_signature(-1, eof_selected_beat, eof_song);
}

int eof_menu_beat_ks_0_flats(void)
{
	return eof_apply_key_signature(0, eof_selected_beat, eof_song);
}

int eof_menu_beat_ks_1_sharp(void)
{
	return eof_apply_key_signature(1, eof_selected_beat, eof_song);
}

int eof_menu_beat_ks_2_sharps(void)
{
	return eof_apply_key_signature(2, eof_selected_beat, eof_song);
}

int eof_menu_beat_ks_3_sharps(void)
{
	return eof_apply_key_signature(3, eof_selected_beat, eof_song);
}

int eof_menu_beat_ks_4_sharps(void)
{
	return eof_apply_key_signature(4, eof_selected_beat, eof_song);
}

int eof_menu_beat_ks_5_sharps(void)
{
	return eof_apply_key_signature(5, eof_selected_beat, eof_song);
}

int eof_menu_beat_ks_6_sharps(void)
{
	return eof_apply_key_signature(6, eof_selected_beat, eof_song);
}

int eof_menu_beat_ks_7_sharps(void)
{
	return eof_apply_key_signature(7, eof_selected_beat, eof_song);
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
	unsigned long flags;
	unsigned char track = 0;	//By default, new sections won't be track specific

	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_rocksmith_section_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_rocksmith_section_dialog);

	(void) snprintf(eof_events_add_dialog_string, sizeof(eof_events_add_dialog_string) - 1, "Specific to %s", eof_song->track[eof_selected_track]->name);

	if(eof_popup_dialog(eof_rocksmith_section_dialog, 0) == 4)
	{	//User clicked OK
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make an undo state
		flags = EOF_EVENT_FLAG_RS_SECTION;	//By default, the event will only be a RS section
		if(eof_rocksmith_section_dialog[2].flags == D_SELECTED)
		{	//If the user also opted to add it as a RS phrase
			flags |= EOF_EVENT_FLAG_RS_PHRASE;	//The event will be both a RS phrase and a RS section
		}
		if(eof_rocksmith_section_dialog[3].flags == D_SELECTED)
		{	//If the user also opted to make it specific to the active track
			track = eof_selected_track;
		}
		(void) eof_song_add_text_event(eof_song, eof_selected_beat, eof_rs_predefined_sections[eof_rocksmith_section_dialog[1].d1].string, track, flags, 0);
		eof_sort_events(eof_song);
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
	centre_dialog(eof_rocksmith_event_dialog);

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
