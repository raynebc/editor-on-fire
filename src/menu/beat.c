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
#include "beat.h"
#include "song.h"

#ifdef USEMEMWATCH
#include "../memwatch.h"
#endif

char eof_ts_menu_off_text[32] = {0};

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

MENU eof_beat_menu[] =
{
    {"&BPM Change", eof_menu_beat_bpm_change, NULL, 0, NULL},
    {"Time &Signature", NULL, eof_beat_time_signature_menu, 0, NULL},
    {"", NULL, NULL, 0, NULL},
    {"Add", eof_menu_beat_add, NULL, 0, NULL},
    {"Delete\t" CTRL_NAME "+Del", eof_menu_beat_delete, NULL, 0, NULL},
    {"", NULL, NULL, 0, NULL},
    {"Push Offset Back", eof_menu_beat_push_offset_back, NULL, 0, NULL},
    {"Push Offset Up", eof_menu_beat_push_offset_up, NULL, 0, NULL},
    {"Reset Offset to Zero", eof_menu_beat_reset_offset, NULL, 0, NULL},
    {"", NULL, NULL, 0, NULL},
    {"&Anchor Beat\tShift+A", eof_menu_beat_anchor, NULL, 0, NULL},
    {"Toggle Anchor\tA", eof_menu_beat_toggle_anchor, NULL, 0, NULL},
    {"&Delete Anchor", eof_menu_beat_delete_anchor, NULL, 0, NULL},
    {"", NULL, NULL, 0, NULL},
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
    {NULL, NULL, NULL, 0, NULL}
};

DIALOG eof_events_dialog[] =
{
   /* (proc)            (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)            (dp2) (dp3) */
   { d_agup_window_proc,0,   48,  500, 232, 2,   23,  0,    0,      0,   0,   "Events",       NULL, NULL },
   { d_agup_list_proc,  12,  84,  400, 138, 2,   23,  0,    0,      0,   0,   eof_events_list,NULL, NULL },
   { d_agup_push_proc,  425, 84,  68,  28,  2,   23,  'a',  D_EXIT, 0,   0,   "&Add",         NULL, eof_events_dialog_add },
   { d_agup_push_proc,  425, 124, 68,  28,  2,   23,  'e',  D_EXIT, 0,   0,   "&Edit",        NULL, eof_events_dialog_edit },
   { d_agup_push_proc,  425, 164, 68,  28,  2,   23,  'l',  D_EXIT, 0,   0,   "De&lete",      NULL, eof_events_dialog_delete },
   { d_agup_button_proc,12,  235, 240, 28,  2,   23,  '\r', D_EXIT, 0,   0,   "Done",         NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_all_events_dialog[] =
{
   /* (proc)                    (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                   (dp2) (dp3) */
   { d_agup_window_proc,         0,   48,  500, 234, 2,   23,  0,    0,      0,   0,   "All Events",         NULL, NULL },
   { d_agup_list_proc,           12,  84,  475, 140, 2,   23,  0,    0,      0,   0,   eof_events_list_all,  NULL, NULL },
   { d_agup_button_proc,         12,  237, 154, 28,  2,   23,  'f',  D_EXIT, 0,   0,   "&Find",              NULL, NULL },
   { d_agup_button_proc,         178, 237, 154, 28,  2,   23,  '\r', D_EXIT, 0,   0,   "Done",               NULL, NULL },
   { eof_all_events_radio_proc,	 344, 228, 100, 15,  2,   23,  0, D_SELECTED,0,   0,   "All Events",         (void *)4,    NULL },
   { eof_all_events_radio_proc,	 344, 244, 150, 15,  2,   23,  0,    0,      0,   0,   "This Track's Events",(void *)5,    NULL },
   { eof_all_events_radio_proc,	 344, 260, 140, 15,  2,   23,  0,    0,      0,   0,   "Section Events",     (void *)6,    NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

char eof_events_add_dialog_string[100] = {0};
DIALOG eof_events_add_dialog[] =
{
   /* (proc)            (x) (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)           (dp2) (dp3) */
   { d_agup_window_proc,0,  48,  314, 126, 2,   23,  0,    0,      0,   0,   "Event Name",  NULL, NULL },
   { d_agup_text_proc,  12, 84,  64,  8,   2,   23,  0,    0,      0,   0,   "Text:",       NULL, NULL },
   { d_agup_edit_proc,  48, 80,  254, 20,  2,   23,  0,    0,      255, 0,   eof_etext,     NULL, NULL },
   { d_agup_check_proc, 12, 108, 225, 16,  0,   0,   0,    0,      1,   0,   eof_events_add_dialog_string, NULL, NULL },
   { d_agup_button_proc,67, 132, 84,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",          NULL, NULL },
   { d_agup_button_proc,163,132, 78,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",      NULL, NULL },
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
   { eof_verified_edit_proc,   112, 80,  66,  20,  2,   23,  0,    0,      8,   0,   eof_etext2,           "0123456789:", NULL },
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

void eof_prepare_beat_menu(void)
{
	unsigned long i;

	if(eof_song && eof_song_loaded)
	{	//If a song is loaded
		//Several beat menu items are disabled below if the tempo map is locked.  Clear those items' flags in case the lock was removed
		eof_beat_menu[0].flags = 0;	//BPM change
		eof_beat_menu[3].flags = 0;	//Add
		eof_beat_menu[4].flags = 0;	//Delete
		eof_beat_menu[6].flags = 0;	//Push offset back
		eof_beat_menu[7].flags = 0;	//Push offset up
		eof_beat_menu[8].flags = 0;	//Reset offset to zero
		eof_beat_menu[10].flags = 0;	//Anchor
		eof_beat_menu[11].flags = 0;	//Toggle anchor
		eof_beat_menu[12].flags = 0;	//Delete anchor
		eof_beat_menu[14].flags = 0;	//Reset BPM
		eof_beat_menu[15].flags = 0;	//Calculate BPM
		eof_beat_menu[16].flags = 0;	//Double BPM
		eof_beat_menu[17].flags = 0;	//Halve BPM
		eof_beat_menu[18].flags = 0;	//Adjust tempo for RBN

//Beat>Add and Delete validation
		if(eof_find_next_anchor(eof_song, eof_selected_beat) < 0)
		{	//If there are no anchors after the selected beat, disable Beat>Add and Delete, as they'd have no effect
			eof_beat_menu[3].flags = D_DISABLED;
			eof_beat_menu[4].flags = D_DISABLED;
		}
		else
		{
			eof_beat_menu[3].flags = 0;
			eof_beat_menu[4].flags = 0;
		}
		if(eof_selected_beat == 0)
		{	//If the first beat marker is selected, disable Beat>Delete, as this beat is not allowed to be deleted
			eof_beat_menu[4].flags = D_DISABLED;
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
			eof_beat_menu[10].flags = 0;
			eof_beat_menu[11].flags = 0;
		}
		else
		{
			eof_beat_menu[10].flags = D_DISABLED;
			eof_beat_menu[11].flags = D_DISABLED;
		}
//Beat>Delete Anchor validation
		if((eof_song->beat[eof_selected_beat]->flags & EOF_BEAT_FLAG_ANCHOR) && (eof_selected_beat != 0))
		{	//If the selected beat is an anchor, and the first beat marker is not selected, enable Beat>Delete Anchor
			eof_beat_menu[12].flags = 0;
		}
		else
		{
			eof_beat_menu[12].flags = D_DISABLED;
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
			eof_beat_menu[14].flags = D_DISABLED;
		}
		else
		{
			eof_beat_menu[14].flags = 0;
		}
//Beat>All Events and Clear Events validation
		if(eof_song->text_events > 0)
		{	//If there is at least one defined text event, enable Beat>All Events and Clear Events
			eof_beat_menu[20].flags = 0;
			eof_beat_menu[22].flags = 0;
		}
		else
		{
			eof_beat_menu[20].flags = D_DISABLED;
			eof_beat_menu[22].flags = D_DISABLED;
		}
//Beat>Place Trainer Event
		if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If a pro guitar/bass track is active
			eof_beat_menu[23].flags = 0;
		}
		else
		{
			eof_beat_menu[23].flags = D_DISABLED;
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
				ustrcpy(eof_ts_menu_off_text, "No Change");
				break;
			}
		}
		if(i == eof_selected_beat)
		{
			ustrcpy(eof_ts_menu_off_text, "&Off");
		}
		if((eof_selected_beat + 1 < eof_song->beats) && (eof_song->beat[eof_selected_beat + 1]->ppqn != eof_song->beat[eof_selected_beat]->ppqn))
		{	//If there is a beat after the current beat, and it has a different tempo
			eof_beat_menu[17].flags = D_DISABLED;	//Disable Beat>Halve BPM
		}
		else
		{
			eof_beat_menu[17].flags = 0;
		}

		if(eof_song->tags->tempo_map_locked)
		{	//If the chart's tempo map is locked, disable various beat operations
			eof_beat_menu[0].flags = D_DISABLED;	//BPM change
			eof_beat_menu[3].flags = D_DISABLED;	//Add
			eof_beat_menu[4].flags = D_DISABLED;	//Delete
			eof_beat_menu[6].flags = D_DISABLED;	//Push offset back
			eof_beat_menu[7].flags = D_DISABLED;	//Push offset up
			eof_beat_menu[8].flags = D_DISABLED;	//Reset offset to zero
			eof_beat_menu[10].flags = D_DISABLED;	//Anchor
			eof_beat_menu[11].flags = D_DISABLED;	//Toggle anchor
			eof_beat_menu[12].flags = D_DISABLED;	//Delete anchor
			eof_beat_menu[14].flags = D_DISABLED;	//Reset BPM
			eof_beat_menu[15].flags = D_DISABLED;	//Calculate BPM
			eof_beat_menu[16].flags = D_DISABLED;	//Double BPM
			eof_beat_menu[17].flags = D_DISABLED;	//Halve BPM
			eof_beat_menu[18].flags = D_DISABLED;	//Adjust tempo for RBN
		}
	}//If a song is loaded
}

int eof_menu_beat_bpm_change(void)
{
	int i;
	unsigned long cppqn = eof_song->beat[eof_selected_beat]->ppqn;
	int old_flags = eof_song->beat[eof_selected_beat]->flags;

	if(eof_song->tags->tempo_map_locked)	//If the chart's tempo map is locked
		return 1;							//Return without making changes

	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_bpm_change_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_bpm_change_dialog);
	sprintf(eof_etext, "%3.2f", (double)60000000.0 / (double)eof_song->beat[eof_selected_beat]->ppqn);
	eof_bpm_change_dialog[3].flags = 0;
	eof_bpm_change_dialog[4].flags = 0;
	if(eof_popup_dialog(eof_bpm_change_dialog, 2) == 5)
	{	//If the user activated the "OK" button
		double bpm = atof(eof_etext);
		sprintf(eof_etext2, "%3.2f", (double)60000000.0 / (double)eof_song->beat[eof_selected_beat]->ppqn);
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
		{
			if(eof_selected_beat > 0)
			{
				eof_song->beat[eof_selected_beat]->flags |= EOF_BEAT_FLAG_ANCHOR;
			}
			eof_menu_edit_cut(eof_selected_beat + 1, 1, 0.0);
			eof_song->beat[eof_selected_beat]->flags = old_flags;
		}
		if(eof_bpm_change_dialog[3].flags == D_SELECTED)
		{
			eof_song->beat[eof_selected_beat]->ppqn = (double)60000000.0 / bpm;
			eof_song->beat[eof_selected_beat + 1]->flags |= EOF_BEAT_FLAG_ANCHOR;
		}
		else
		{
			for(i = eof_selected_beat; i < eof_song->beats; i++)
			{
				if(eof_song->beat[i]->ppqn == cppqn)
				{
					eof_song->beat[i]->ppqn = (double)60000000.0 / bpm;
				}

				/* break when we reach the end of the portion to change */
				else
				{
					break;
				}
			}
		}
		if(eof_selected_beat > 0)
		{
			eof_song->beat[eof_selected_beat]->flags |= EOF_BEAT_FLAG_ANCHOR;
		}
		eof_calculate_beats(eof_song);
		if(eof_bpm_change_dialog[4].flags == D_SELECTED)
		{
			eof_menu_edit_cut_paste(eof_selected_beat + 1, 1, 0.0);
		}
	}
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(NULL);
	return 1;
}

int eof_menu_beat_ts_4_4(void)
{
	eof_apply_ts(4,4,eof_selected_beat,eof_song,1);
	eof_select_beat(eof_selected_beat);
	return 1;
}

int eof_menu_beat_ts_3_4(void)
{
	eof_apply_ts(3,4,eof_selected_beat,eof_song,1);
	eof_select_beat(eof_selected_beat);
	return 1;
}

int eof_menu_beat_ts_5_4(void)
{
	eof_apply_ts(5,4,eof_selected_beat,eof_song,1);
	eof_select_beat(eof_selected_beat);
	return 1;
}

int eof_menu_beat_ts_6_4(void)
{
	eof_apply_ts(6,4,eof_selected_beat,eof_song,1);
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
	sprintf(eof_etext, "%lu", ((eof_song->beat[eof_selected_beat]->flags & 0xFF000000)>>24) + 1);
	sprintf(eof_etext2, "%lu", ((eof_song->beat[eof_selected_beat]->flags & 0x00FF0000)>>16) + 1);
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
				eof_apply_ts(num,den,eof_selected_beat,eof_song,1);
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
	}
	return 1;
}

int eof_menu_beat_delete(void)
{
	if(eof_song->tags->tempo_map_locked)	//If the chart's tempo map is locked
		return 1;							//Return without making changes

	int flags = eof_song->beat[eof_selected_beat]->flags;
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
	}
	return 1;
}

int eof_menu_beat_push_offset_back(void)
{
	if(eof_song->tags->tempo_map_locked)	//If the chart's tempo map is locked
		return 1;							//Return without making changes

	int i;
	int backamount = eof_song->beat[1]->pos - eof_song->beat[0]->pos;

	if(eof_song->beat[0]->pos - backamount >= 0)
	{
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
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
		}
		else
			return 0;	//Return failure
	}
	return 1;	//Return success
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
	eof_fixup_notes(eof_song);
	return 1;
}

int eof_menu_beat_reset_offset(void)
{
	int i;
	double newbpm;

	if(eof_song->tags->tempo_map_locked)	//If the chart's tempo map is locked
		return 1;							//Return without making changes

	if(eof_song->beat[0]->pos > 0)
	{	//Only allow this function to run if the current MIDI delay is above zero
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		if(eof_song_resize_beats(eof_song, eof_song->beats + 1))
		{	//If the beats array was successfully resized
			for(i = eof_song->beats - 1; i > 0; i--)
			{
				memcpy(eof_song->beat[i], eof_song->beat[i - 1], sizeof(EOF_BEAT_MARKER));
			}
			eof_song->beat[0]->pos = 0;
			eof_song->beat[0]->fpos = 0;
			eof_song->beat[0]->flags = eof_song->beat[1]->flags;	//Copy the flags (ie. Time Signature) of the original first beat marker
			eof_song->tags->ogg[eof_selected_ogg].midi_offset = 0;
			newbpm = 60000.0 / eof_song->beat[1]->pos;	//60000ms / length of new beat (the MIDI delay) = Tempo
			eof_song->beat[0]->ppqn = 60000000.0 / newbpm;	//60000000usec_per_minute / tempo = PPQN
			eof_move_text_events(eof_song, 0, 1, 1);
			if(eof_song->beat[1]->ppqn != eof_song->beat[0]->ppqn)
			{	//If this operation caused the first and second beat markers to have different tempos,
				eof_song->beat[1]->flags = EOF_BEAT_FLAG_ANCHOR;		//Set the second beat marker's anchor flag
			}
		}
		else
			return 0;	//Return failure
	}
	return 1;	//Return success
}

int eof_menu_beat_anchor(void)
{
	int mm, ss, hs;
	int oldmm, oldss, oldhs;
	int oldpos = eof_song->beat[eof_selected_beat]->pos;
	int newpos = 0;
	int revert = 0;
	char ttext[3] = {0};

	if(eof_song->tags->tempo_map_locked)	//If the chart's tempo map is locked
		return 1;							//Return without making changes

	if(eof_selected_beat == 0)
	{
		return 1;
	}
	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_anchor_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_anchor_dialog);
	oldmm = (eof_song->beat[eof_selected_beat]->pos / 1000) / 60;
	oldss = (eof_song->beat[eof_selected_beat]->pos / 1000) % 60;
	oldhs = (eof_song->beat[eof_selected_beat]->pos / 10) % 100;
	sprintf(eof_etext2, "%02d:%02d:%02d", oldmm, oldss, oldhs);
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
		hs = atoi(ttext);

		/* time wasn't modified so get out without changing anything */
		if((mm == oldmm) && (ss == oldss) && (hs == oldhs))
		{
			eof_cursor_visible = 1;
			eof_pen_visible = 1;
			eof_show_mouse(NULL);
			return 1;
		}
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		newpos = (double)mm * 60.0 * 1000.0 + (double)ss * 1000.0 + (double)hs * 10.0;
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
					eof_undo_apply();
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
					eof_undo_apply();
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
			eof_song_resize_beats(eof_song, eof_selected_beat);
			eof_calculate_beats(eof_song);
		}
		else
		{
			eof_realign_beats(eof_song, eof_selected_beat);
		}
	}
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
	return 1;
}

int eof_menu_beat_all_events(void)
{
	unsigned long track, realindex;

	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_all_events_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_all_events_dialog);
	eof_all_events_dialog[1].d1 = 0;
	if(eof_popup_dialog(eof_all_events_dialog, 0) == 2)
	{	//User clicked Find
		realindex = eof_retrieve_text_event(eof_all_events_dialog[1].d1);	//Find the actual event, taking the display filter into account
		if(realindex < eof_song->text_events)
		{
			eof_set_seek_position(eof_song->beat[eof_song->text_event[realindex]->beat]->pos + eof_av_delay);
			eof_selected_beat = eof_song->text_event[realindex]->beat;
			track = eof_song->text_event[realindex]->track;
			if((track != 0) && (track < eof_song->tracks))
			{	//If this is a track-specific event
				eof_menu_track_selected_track_number(track);	//Change to that track
			}
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
	if(eof_song->text_events <= 0)
	{
		allegro_message("No events to clear!");
		return 1;
	}
	if(alert(NULL, "Erase all events?", NULL, "&Yes", "&No", 'y', 'n') == 1)
	{
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		eof_song_resize_text_events(eof_song, 0);
		for(i = 0; i < eof_song->beats; i++)
		{	//For each beat
			eof_song->beat[i]->flags &= (~EOF_BEAT_FLAG_EVENTS);	//Clear the event flag
		}
	}
	eof_clear_input();
	return 1;
}

char * eof_events_list(int index, int * size)
{
	int i;
	int ecount = 0;
	char trackname[22];

	for(i = 0; i < eof_song->text_events; i++)
	{
		if(eof_song->text_event[i]->beat == eof_selected_beat)
		{
			if(ecount < EOF_MAX_TEXT_EVENTS)
			{
				if((eof_song->text_event[i]->track != 0) && (eof_song->text_event[i]->track < eof_song->tracks))
				{	//If this is a track specific event
					snprintf(trackname, sizeof(trackname), "(%s) ", eof_song->track[eof_song->text_event[i]->track]->name);
				}
				else
				{
					trackname[0] = '\0';	//Empty the string
				}
				snprintf(eof_event_list_text[ecount], 256, "%s%s", trackname, eof_song->text_event[i]->text);
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
	char trackname[22];
	unsigned long x, count = 0, realindex;

	if(index < 0)
	{	//Signal to return the list count
		if(eof_all_events_dialog[4].flags && D_SELECTED)
		{	//Display all events
			count = eof_song->text_events;
		}
		else if(eof_all_events_dialog[5].flags && D_SELECTED)
		{	//Display this track's events
			for(x = 0; x < eof_song->text_events; x++)
			{	//For each event
				if(eof_song->text_event[x]->track == eof_selected_track)
				{	//If the event is specific to the currently active track
					count++;
				}
			}
		}
		else
		{	//Display section events
			for(x = 0; x < eof_song->text_events; x++)
			{	//For each event
				if(eof_is_section_marker(eof_song->text_event[x]->text))
				{	//If the string begins with "[section", "section" or "[prc_"
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
			snprintf(trackname, sizeof(trackname), " %s", eof_song->track[eof_song->text_event[realindex]->track]->name);
		}
		else
		{
			trackname[0] = '\0';	//Empty the string
		}
		snprintf(eof_event_list_text[index], 256, "(%02lu:%02lu.%03lu%s) %s", eof_song->beat[eof_song->text_event[realindex]->beat]->pos / 60000, (eof_song->beat[eof_song->text_event[realindex]->beat]->pos / 1000) % 60, eof_song->beat[eof_song->text_event[realindex]->beat]->pos % 1000, trackname, eof_song->text_event[realindex]->text);
		return eof_event_list_text[index];
	}
	return NULL;
}

int eof_events_dialog_add(DIALOG * d)
{
	int i;
	unsigned long track = 0;

	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_events_add_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_events_add_dialog);
	ustrcpy(eof_etext, "");
	snprintf(eof_events_add_dialog_string, sizeof(eof_events_add_dialog_string), "Specific to %s", eof_song->track[eof_selected_track]->name);
	eof_events_add_dialog[3].flags = 0;	//By default, this is not a track specific event
	if(eof_popup_dialog(eof_events_add_dialog, 2) == 4)
	{	//User clicked OK
		if((ustrlen(eof_etext) > 0) && eof_check_string(eof_etext))
		{	//User entered text that isn't all space characters
			if(eof_events_add_dialog[3].flags & D_SELECTED)
			{	//User opted to make this a track specific event
				track = eof_selected_track;
			}
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			eof_song_add_text_event(eof_song, eof_selected_beat, eof_etext, track, 0);
			eof_sort_events(eof_song);
		}
	}
	dialog_message(eof_events_dialog, MSG_DRAW, 0, &i);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(screen);
	return D_O_K;
}

int eof_events_dialog_edit(DIALOG * d)
{
	int i, trackflag;
	short ecount = 0;
	short event = -1;
	unsigned long track = 0;

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
		dialog_message(eof_events_dialog, MSG_DRAW, 0, &i);
		eof_cursor_visible = 1;
		eof_pen_visible = 1;
		eof_show_mouse(screen);
		return D_O_K;	//Don't allow it to be edited here
	}
	snprintf(eof_events_add_dialog_string, sizeof(eof_events_add_dialog_string), "Specific to %s", eof_song->track[eof_selected_track]->name);
	if(eof_song->text_event[event]->track == eof_selected_track)
	{	//If this event is specific to this track
		eof_events_add_dialog[3].flags = D_SELECTED;	//Set the checkbox specifying the event is track specific
	}
	else
	{	//Otherwise clear the checkbox
		eof_events_add_dialog[3].flags = 0;
	}

	ustrcpy(eof_etext, eof_song->text_event[event]->text);	//Save the original event text
	trackflag = eof_events_add_dialog[3].flags;				//Save the track specifier flag
	if(eof_popup_dialog(eof_events_add_dialog, 2) == 4)
	{	//User clicked OK
		if((ustrlen(eof_etext) > 0) && eof_check_string(eof_etext) && (ustrcmp(eof_song->text_event[event]->text, eof_etext) || (eof_events_add_dialog[3].flags != trackflag)))
		{	//User entered text that isn't all space characters, and either the event's text was changed or it's track specifier was
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			ustrcpy(eof_song->text_event[event]->text, eof_etext);
			if(eof_events_add_dialog[3].flags & D_SELECTED)
			{	//User opted to make this a track specific event
				track = eof_selected_track;
			}
			eof_song->text_event[event]->track = track;
		}
	}
	dialog_message(eof_events_dialog, MSG_DRAW, 0, &i);
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

	if(eof_song->text_events <= 0)
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
				dialog_message(eof_events_dialog, MSG_DRAW, 0, &i);
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
		sprintf(eof_etext2, "[begin_pg song_trainer_pg_%s]", eof_trainer_string);
		sprintf(eof_etext3, "[pg_norm song_trainer_pg_%s]", eof_trainer_string);
		sprintf(eof_etext4, "[end_pg song_trainer_pg_%s]", eof_trainer_string);
	}
	else if((eof_selected_track == EOF_TRACK_PRO_BASS) || (eof_selected_track == EOF_TRACK_PRO_BASS_22))
	{	//A pro bass track is active
		sprintf(eof_etext2, "[begin_pb song_trainer_pb_%s]", eof_trainer_string);
		sprintf(eof_etext3, "[pb_norm song_trainer_pb_%s]", eof_trainer_string);
		sprintf(eof_etext4, "[end_pb song_trainer_pb_%s]", eof_trainer_string);
	}

	//Update the checkboxes to indicate which trainer strings are already defined
	if(eof_song_contains_event(eof_song, eof_etext2, relevant_track))
	{	//If this training event is already defined in the active track
		eof_place_trainer_dialog[3].flags = D_SELECTED | D_DISABLED;	//Check this box
	}
	else
	{
		eof_place_trainer_dialog[3].flags = 0 | D_DISABLED;		//Otherwise clear this box
	}
	if(eof_song_contains_event(eof_song, eof_etext3, relevant_track))
	{	//If this training event is already defined in the active track
		eof_place_trainer_dialog[5].flags = D_SELECTED | D_DISABLED;	//Check this box
	}
	else
	{
		eof_place_trainer_dialog[5].flags = 0 | D_DISABLED;		//Otherwise clear this box
	}
	if(eof_song_contains_event(eof_song, eof_etext4, relevant_track))
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

		if(eof_song_contains_event(eof_song, selected_string, relevant_track))
		{	//If this training event is already defined in the active track
			if(alert(NULL, "Warning:  This text event already exists in this track.  Continue?", NULL, "&Yes", "&No", 'y', 'n') != 1)
			{	//If the user does not opt to place the duplicate text event
				return 0;
			}
		}
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		eof_song_add_text_event(eof_song, eof_selected_beat, selected_string, relevant_track, 0);	//Add the chosen text event to the selected beat
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
	static int previous_option = 4;	//By default, eof_all_events_dialog[4] (all events) is selected
	int selected_option;

	if(msg == MSG_CLICK)
	{
		selected_option = (int)d->dp2;	//Find out which radio button was clicked on

		if(selected_option != previous_option)
		{	//If the event display filter changed, have the event list redrawn
			eof_all_events_dialog[4].flags = eof_all_events_dialog[5].flags = eof_all_events_dialog[6].flags = 0;	//Clear all radio buttons
			eof_all_events_dialog[selected_option].flags = D_SELECTED;	//Reselect the radio button that was just clicked on
			object_message(&eof_all_events_dialog[1], MSG_DRAW, 0);		//Have Allegro redraw the list of events
			object_message(&eof_all_events_dialog[4], MSG_DRAW, 0);		//Have Allegro redraw the radio buttons
			object_message(&eof_all_events_dialog[5], MSG_DRAW, 0);
			object_message(&eof_all_events_dialog[6], MSG_DRAW, 0);
			previous_option = selected_option;
		}
	}

	return d_agup_radio_proc(msg, d, c);	//Allow the input to be processed
}

unsigned long eof_retrieve_text_event(unsigned long index)
{
	unsigned long x, count = 0;
	if(eof_all_events_dialog[4].flags && D_SELECTED)
	{	//Display all events
		return index;
	}
	else if(eof_all_events_dialog[5].flags && D_SELECTED)
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
	else
	{	//Display section events
		for(x = 0; x < eof_song->text_events; x++)
		{	//For each event
			if(eof_is_section_marker(eof_song->text_event[x]->text))
			{	//If the string begins with "[section", "section" or "[prc_"
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

	eof_double_tempo(eof_song, eof_selected_beat, 1);
	return 1;
}

int eof_menu_beat_halve_tempo(void)
{
	if(eof_song->tags->tempo_map_locked)	//If the chart's tempo map is locked
		return 1;							//Return without making changes

	eof_halve_tempo(eof_song, eof_selected_beat, 1);
	return 1;
}

int eof_menu_beat_set_RBN_tempos(void)
{
	if(eof_song->tags->tempo_map_locked)	//If the chart's tempo map is locked
		return 1;							//Return without making changes

	char undo_made = 0, first_change = 1, changed;
	unsigned long loop_ctr = 0;
	eof_log("eof_move_text_events() entered", 1);

	unsigned long i;

	if(!eof_song)
	{
		return 1;
	}
	for(loop_ctr = 0; loop_ctr < 25; loop_ctr++)
	{	//Only allow this loop to run 25 times in a row
		changed = 0;	//Reset this condition
		for(i = 0; i < eof_song->beats; i++)
		{	//For each beat
			if(eof_song->beat[i]->ppqn < 200000)
			{	//If this beat is > 300BPM
				if(!undo_made)
				{
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					undo_made = 1;
				}
				if(eof_halve_tempo(eof_song, i, 0) < 0)
				{	//If the function had to omit one beat from processing
					alogg_seek_abs_msecs_ogg(eof_music_track, eof_song->beat[i + 2]->pos + eof_av_delay);	//Seek to the offending beat (two beats ahead)
					eof_music_pos = alogg_get_pos_msecs_ogg(eof_music_track);
					eof_music_actual_pos = eof_music_pos;
					eof_mix_seek(eof_music_pos);
					eof_selected_beat = i + 2;
					allegro_message("Warning:  This beat has a tempo that must be manually corrected.");
					return 1;
				}
				changed = 1;		//Track that a change was made during this loop
				first_change = 0;	//Track the first successful alteration of the tempo map
			}
			else if(eof_song->beat[i]->ppqn > 1500000)
			{	//If this beat's tempo is < 40BPM
				if(!undo_made)
				{
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					undo_made = 1;
				}
				eof_double_tempo(eof_song, i, 0);
				changed = 1;		//Track that a change was made during this loop
				first_change = 0;	//Track the first successful alteration of the tempo map
			}
		}
		if(changed == 0)
		{	//If there were no changes made during this loop
			break;
		}
	}
	if(!loop_ctr)
	{	//If no changes were made
		allegro_message("No tempo adjustments necessary");
	}
	else
	{
		allegro_message("Tempos successfully adjusted");
	}
	return 1;
}
