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
    {"Delete\tCtrl+Del", eof_menu_beat_delete, NULL, 0, NULL},
    {"", NULL, NULL, 0, NULL},
    {"Push Offset Back", eof_menu_beat_push_offset_back, NULL, 0, NULL},
    {"Push Offset Up", eof_menu_beat_push_offset_up, NULL, 0, NULL},
    {"", NULL, NULL, 0, NULL},
    {"&Anchor Beat\tShift+A", eof_menu_beat_anchor, NULL, 0, NULL},
    {"&Toggle Anchor\tA", eof_menu_beat_toggle_anchor, NULL, 0, NULL},
    {"&Delete Anchor", eof_menu_beat_delete_anchor, NULL, 0, NULL},
    {"", NULL, NULL, 0, NULL},
    {"&Reset BPM", eof_menu_beat_reset_bpm, NULL, 0, NULL},
    {"&Calculate BPM", eof_menu_beat_calculate_bpm, NULL, 0, NULL},
    {"", NULL, NULL, 0, NULL},
    {"All E&vents", eof_menu_beat_all_events, NULL, 0, NULL},
    {"&Events", eof_menu_beat_events, NULL, 0, NULL},
    {"Clear Events", eof_menu_beat_clear_events, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

DIALOG eof_events_dialog[] =
{
   /* (proc)         (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)           (dp2) (dp3) */
   { d_agup_window_proc,    0,  48,  216 + 110 + 20, 160 + 72, 2,   23,  0,    0,      0,   0,   "Events",               NULL, NULL },
   { d_agup_list_proc,   12, 84,  110 * 2 + 20,  69 * 2,  2,   23,  0,    0,      0,   0,   eof_events_list, NULL, NULL },
   { d_agup_push_proc, 134 + 130,  84, 68,  28, 2,   23,  0,    D_EXIT, 0,   0,   "Add",               NULL, eof_events_dialog_add },
   { d_agup_push_proc, 134 + 130,  124, 68,  28, 2,   23,  0,    D_EXIT, 0,   0,   "Edit",               NULL, eof_events_dialog_edit },
   { d_agup_push_proc, 134 + 130,  164, 68,  28, 2,   23,  0,    D_EXIT, 0,   0,   "Delete",               NULL, eof_events_dialog_delete },
   { d_agup_button_proc, 12,  166 + 69, 190 + 30 + 20,  28, 2,   23,  0,    D_EXIT, 0,   0,   "Done",               NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_all_events_dialog[] =
{
   /* (proc)         (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)           (dp2) (dp3) */
   { d_agup_window_proc,    0,  48,  216 + 110 + 20, 160 + 72 + 2, 2,   23,  0,    0,      0,   0,   "All Events",               NULL, NULL },
   { d_agup_list_proc,   12, 84,  110 * 2 + 20 + 80,  69 * 2 + 2,  2,   23,  0,    0,      0,   0,   eof_events_list_all, NULL, NULL },
   { d_agup_button_proc, 12,  166 + 69 + 2, 160 - 6,  28, 2,   23,  0,    D_EXIT, 0,   0,   "Find",               NULL, NULL },
   { d_agup_button_proc, 12 + 160 + 6,  166 + 69 + 2, 160 - 6,  28, 2,   23,  0,    D_EXIT, 0,   0,   "Done",               NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_events_add_dialog[] =
{
   /* (proc)         (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)           (dp2) (dp3) */
   { d_agup_window_proc,    0,  48,  204 + 110, 106, 2,   23,  0,    0,      0,   0,   "Event Name",               NULL, NULL },
   { d_agup_text_proc,   12,  84,  64,  8,  2,   23,  0,    0,      0,   0,   "Text:",         NULL, NULL },
   { d_agup_edit_proc,   48, 80,  144 + 110,  20,  2,   23,  0,    0,      255,   0,   eof_etext,           NULL, NULL },
   { d_agup_button_proc, 12 + 55,  112, 84,  28, 2,   23,  '\r',    D_EXIT, 0,   0,   "OK",               NULL, NULL },
   { d_agup_button_proc, 108 + 55, 112, 78,  28, 2,   23,  0,    D_EXIT, 0,   0,   "Cancel",           NULL, NULL },
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

void eof_prepare_beat_menu(void)
{
	int i;
//	int selected = 0;	//This variable is not being used for anything meaningful

	if(eof_song && eof_song_loaded)
	{
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
			eof_beat_menu[13].flags = D_DISABLED;
		}
		else
		{
			eof_beat_menu[13].flags = 0;
		}
//The condition (selected > 1) was always false, since it was initialized to 0 and never modified afterward
/*
//Beat>Calculate BPM validation
		if(selected > 1)
		{
			eof_beat_menu[14].flags = 0;
		}
		else
		{
			eof_beat_menu[14].flags = D_DISABLED;
		}
*/
//Beat>All Events and Clear Events validation
		if(eof_song->text_events > 0)
		{	//If there is at least one defined text event, enable Beat>Events and Clear Events
			eof_beat_menu[16].flags = 0;
			eof_beat_menu[18].flags = 0;
		}
		else
		{
			eof_beat_menu[16].flags = D_DISABLED;
			eof_beat_menu[18].flags = D_DISABLED;
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
			ustrcpy(eof_ts_menu_off_text, "Off");
		}
	}
}

int eof_menu_beat_bpm_change(void)
{
	int i;
	unsigned long cppqn = eof_song->beat[eof_selected_beat]->ppqn;
	int old_flags = eof_song->beat[eof_selected_beat]->flags;

	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_bpm_change_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_bpm_change_dialog);
	sprintf(eof_etext, "%3.2f", (double)60000000.0 / (double)eof_song->beat[eof_selected_beat]->ppqn);
//	sprintf(eof_etext2, "%02lu:%02lu:%02lu", (eof_song->beat[eof_selected_beat]->pos / 1000) / 60, (eof_song->beat[eof_selected_beat]->pos / 1000) % 60, (eof_song->beat[eof_selected_beat]->pos / 10) % 100);
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
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	eof_song->beat[eof_selected_beat]->flags = flags;
	eof_select_beat(eof_selected_beat);
	return 1;
}

int eof_menu_beat_delete(void)
{
	int flags = eof_song->beat[eof_selected_beat]->flags;
	if((eof_selected_beat > 0) && (eof_find_next_anchor(eof_song, eof_selected_beat) >= 0))
	{	//Only process this function if a beat other than beat 0 is selected, and there is at least one anchor after the selected beat
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		eof_song_delete_beat(eof_song, eof_selected_beat);
		if((eof_song->beat[eof_selected_beat - 1]->flags & EOF_BEAT_FLAG_ANCHOR) && (eof_song->beat[eof_selected_beat]->flags & EOF_BEAT_FLAG_ANCHOR))
		{
			double beats_length = eof_song->beat[eof_selected_beat]->pos - eof_song->beat[eof_selected_beat - 1]->pos;
			double newbpm = (double)60000 / (beats_length / (double)1);
			double newppqn = (double)60000000 / newbpm;
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
		eof_move_text_events(eof_song, eof_selected_beat, -1);
		if(flags & EOF_BEAT_FLAG_ANCHOR)
		{
			flags ^= EOF_BEAT_FLAG_ANCHOR;
		}
		eof_song->beat[eof_selected_beat - 1]->flags |= flags;
	}
	return 1;
}

int eof_menu_beat_add(void)
{
	int i;

	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	if(eof_song_add_beat(eof_song) != NULL)
	{	//Only if the new beat structure was successfully created
		for(i = eof_song->beats - 1; i > eof_selected_beat; i--)
		{
			memcpy(eof_song->beat[i], eof_song->beat[i - 1], sizeof(EOF_BEAT_MARKER));
		}
		eof_song->beat[eof_selected_beat + 1]->flags = 0;
		eof_realign_beats(eof_song, eof_selected_beat + 1);
		eof_move_text_events(eof_song, eof_selected_beat + 1, 1);
		return 1;
	}
	else
		return -1;	//Otherwise return error
}

int eof_menu_beat_push_offset_back(void)
{
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
//			eof_song->beats++;
			eof_song->beat[0]->pos = eof_song->beat[1]->pos - backamount;
			eof_song->beat[0]->fpos = eof_song->beat[0]->pos;
			eof_song->beat[1]->flags = 0;
			eof_song->tags->ogg[eof_selected_ogg].midi_offset = eof_song->beat[0]->pos;
			eof_move_text_events(eof_song, 0, 1);
		}
		else
			return 0;	//Return failure
	}
	return 1;	//Return success
}

int eof_menu_beat_push_offset_up(void)
{
	int i;

	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	for(i = 0; i < eof_song->beats - 1; i++)
	{
		memcpy(eof_song->beat[i], eof_song->beat[i + 1], sizeof(EOF_BEAT_MARKER));
	}
	eof_song_delete_beat(eof_song, eof_song->beats - 1);
//	eof_song->beats--;
	eof_song->tags->ogg[eof_selected_ogg].midi_offset = eof_song->beat[0]->pos;
	eof_move_text_events(eof_song, 0, -1);
	eof_fixup_notes();
	return 1;
}

int eof_menu_beat_anchor(void)
{
	int mm, ss, hs;
	int oldmm, oldss, oldhs;
	int oldpos = eof_song->beat[eof_selected_beat]->pos;
	int newpos = 0;
	int revert = 0;
	char ttext[3] = {0};

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
				eof_song->beat[eof_selected_beat]->pos++;
				eof_song->beat[eof_selected_beat]->fpos = eof_song->beat[eof_selected_beat]->pos;
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
				eof_song->beat[eof_selected_beat]->pos--;
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
//	double blength = 0;
//	double bpos = 0;
//	int bbeat = 0;

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
	int i;
	int first = 1;
	unsigned long curpos = 0;
	unsigned long delta;
	unsigned long cppqn = eof_song->beat[eof_selected_beat]->ppqn;
	double bpm = 0.0;
	int bpm_count = 0;

	for(i = 0; i < eof_song->legacy_track[eof_song->track[eof_selected_track]->tracknum]->notes; i++)
	{
		if(eof_selection.multi[i])
		{
			if(first)
			{
				curpos = eof_song->legacy_track[eof_song->track[eof_selected_track]->tracknum]->note[i]->pos;
				first = 0;
			}
			else
			{
				delta = eof_song->legacy_track[eof_song->track[eof_selected_track]->tracknum]->note[i]->pos - curpos;
				curpos = eof_song->legacy_track[eof_song->track[eof_selected_track]->tracknum]->note[i]->pos;
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
	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_all_events_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_all_events_dialog);
	eof_all_events_dialog[1].d1 = 0;
	if(eof_popup_dialog(eof_all_events_dialog, 0) == 2)
	{
		if(eof_all_events_dialog[1].d1 < eof_song->text_events)
		{
			alogg_seek_abs_msecs_ogg(eof_music_track, eof_song->beat[eof_song->text_event[eof_all_events_dialog[1].d1]->beat]->pos + eof_av_delay);
			eof_music_pos = alogg_get_pos_msecs_ogg(eof_music_track);
			eof_music_actual_pos = eof_music_pos;
			eof_mix_seek(eof_music_pos);
			eof_selected_beat = eof_song->text_event[eof_all_events_dialog[1].d1]->beat;
			eof_reset_lyric_preview_lines();
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
		{
			if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_EVENTS)
			{
				eof_song->beat[i]->flags ^= EOF_BEAT_FLAG_EVENTS;
			}
		}
	}
	eof_clear_input();
	return 1;
}

char * eof_events_list(int index, int * size)
{
	int i;
	int ecount = 0;
	char * etextpointer[32] = {NULL};

	for(i = 0; i < eof_song->text_events; i++)
	{
		if(eof_song->text_event[i]->beat == eof_selected_beat)
		{
			if(ecount < 32)
			{
				etextpointer[ecount] = eof_song->text_event[i]->text;
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
			{
				eof_events_dialog[3].flags = 0;
				eof_events_dialog[4].flags = 0;
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
			return etextpointer[index];
		}
	}
	return NULL;
}

char * eof_events_list_all(int index, int * size)
{
	switch(index)
	{
		case -1:
		{
			*size = eof_song->text_events;
			break;
		}
		default:
		{
			sprintf(eof_event_list_text[index], "(%02lu:%02lu.%02lu) %s", eof_song->beat[eof_song->text_event[index]->beat]->pos / 60000, (eof_song->beat[eof_song->text_event[index]->beat]->pos / 1000) % 60, (eof_song->beat[eof_song->text_event[index]->beat]->pos / 10) % 100, eof_song->text_event[index]->text);
			return eof_event_list_text[index];
		}
	}
	return NULL;
}

int eof_events_dialog_add(DIALOG * d)
{
	int i;

	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_events_add_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_events_add_dialog);
	ustrcpy(eof_etext, "");
	if(eof_popup_dialog(eof_events_add_dialog, 2) == 3)
	{
		if((ustrlen(eof_etext) > 0) && eof_check_string(eof_etext))
		{
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			eof_add_text_event(eof_song, eof_selected_beat, eof_etext);
			eof_sort_events();
			eof_song->beat[eof_selected_beat]->flags |= EOF_BEAT_FLAG_EVENTS;
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
	int i;
	short ecount = 0;
	short event = -1;

	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_events_add_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_events_add_dialog);

	/* find the event */
	for(i = 0; i < eof_song->text_events; i++)
	{
		if(eof_song->text_event[i]->beat == eof_selected_beat)
		{

			/* if we've reached the item that is selected, delete it */
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


	ustrcpy(eof_etext, eof_song->text_event[event]->text);
	if(eof_popup_dialog(eof_events_add_dialog, 2) == 3)
	{
		if((ustrlen(eof_etext) > 0) && eof_check_string(eof_etext))
		{
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			ustrcpy(eof_song->text_event[event]->text, eof_etext);
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
				eof_sort_events();

				/* remove flag if no more events tied to this beat */
				c = eof_events_dialog_delete_events_count();
				if((c <= 0) && (eof_song->beat[eof_selected_beat]->flags & EOF_BEAT_FLAG_EVENTS))
				{
					eof_song->beat[eof_selected_beat]->flags ^= 2;
				}
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
