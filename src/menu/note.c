#include <allegro.h>
#include "../agup/agup.h"
#include "../undo.h"
#include "../dialog.h"
#include "../utility.h"
#include "note.h"

char eof_solo_menu_mark_text[32] = "&Mark";
char eof_star_power_menu_mark_text[32] = "&Mark";
char eof_lyric_line_menu_mark_text[32] = "&Mark";

MENU eof_solo_menu[] =
{
    {eof_solo_menu_mark_text, eof_menu_solo_mark, NULL, 0, NULL},
    {"&Remove", eof_menu_solo_unmark, NULL, 0, NULL},
    {"&Erase All", eof_menu_solo_erase_all, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_star_power_menu[] =
{
    {eof_star_power_menu_mark_text, eof_menu_star_power_mark, NULL, 0, NULL},
    {"&Remove", eof_menu_star_power_unmark, NULL, 0, NULL},
    {"&Erase All", eof_menu_star_power_erase_all, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_lyric_line_menu[] =
{
    {eof_lyric_line_menu_mark_text, eof_menu_lyric_line_mark, NULL, 0, NULL},
    {"&Remove", eof_menu_lyric_line_unmark, NULL, 0, NULL},
    {"&Erase All", eof_menu_lyric_line_erase_all, NULL, 0, NULL},
    {"", NULL, NULL, 0, NULL},
    {"&Toggle Overdrive", eof_menu_lyric_line_toggle_overdrive, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_hopo_menu[] =
{
    {"&Auto", eof_menu_hopo_auto, NULL, 0, NULL},
    {"&Force On", eof_menu_hopo_force_on, NULL, 0, NULL},
    {"Fo&rce Off", eof_menu_hopo_force_off, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_toggle_menu[] =
{
    {"&Green\tCtrl+1", eof_menu_note_toggle_green, NULL, 0, NULL},
    {"&Red\tCtrl+2", eof_menu_note_toggle_red, NULL, 0, NULL},
    {"&Yellow\tCtrl+3", eof_menu_note_toggle_yellow, NULL, 0, NULL},
    {"&Blue\tCtrl+4", eof_menu_note_toggle_blue, NULL, 0, NULL},
    {"&Purple\tCtrl+5", eof_menu_note_toggle_purple, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_menu[] =
{
    {"&Toggle", NULL, eof_note_toggle_menu, 0, NULL},
    {"Transpose &Up\tUp", eof_menu_note_transpose_up, NULL, 0, NULL},
    {"Transpose &Down\tDown", eof_menu_note_transpose_down, NULL, 0, NULL},
    {"", NULL, NULL, 0, NULL},
    {"&Resnap", eof_menu_note_resnap, NULL, 0, NULL},
    {"", NULL, NULL, 0, NULL},
    {"Ed&it Lyric\tL", eof_edit_lyric_dialog, NULL, 0, NULL},
    {"Split Lyric\tShift+L", eof_menu_split_lyric, NULL, 0, NULL},
    {"", NULL, NULL, 0, NULL},
    {"Toggle &Crazy\tT", eof_menu_note_toggle_crazy, NULL, 0, NULL},
    {"", NULL, NULL, 0, NULL},
    {"&Solos", NULL, eof_solo_menu, 0, NULL},
    {"Star &Power", NULL, eof_star_power_menu, 0, NULL},
    {"&Lyric Lines", NULL, eof_lyric_line_menu, 0, NULL},
    {"", NULL, NULL, 0, NULL},
    {"&HOPO", NULL, eof_hopo_menu, 0, NULL},
    {"", NULL, NULL, 0, NULL},
    {"D&elete\tDel", eof_menu_note_delete, NULL, 0, NULL},
    {"Display semitones as &Flat", eof_display_flats_menu, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

DIALOG eof_lyric_dialog[] =
{
   /* (proc)         (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)           (dp2) (dp3) */
   { d_agup_window_proc,    0,  48,  204 + 110, 106, 2,   23,  0,    0,      0,   0,   "Edit Lyric",               NULL, NULL },
   { d_agup_text_proc,   12,  84,  64,  8,  2,   23,  0,    0,      0,   0,   "Text:",         NULL, NULL },
   { d_agup_edit_proc,   48, 80,  144 + 110,  20,  2,   23,  0,    0,      255,   0,   eof_etext,           NULL, NULL },
   { d_agup_button_proc, 12 + 55,  112, 84,  28, 2,   23,  '\r',    D_EXIT, 0,   0,   "OK",               NULL, NULL },
   { d_agup_button_proc, 108 + 55, 112, 78,  28, 2,   23,  0,    D_EXIT, 0,   0,   "Cancel",           NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_split_lyric_dialog[] =
{
   /* (proc)         (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)           (dp2) (dp3) */
   { d_agup_window_proc,    0,  48,  204 + 110, 106, 2,   23,  0,    0,      0,   0,   "Split Lyric",               NULL, NULL },
   { d_agup_text_proc,   12,  84,  64,  8,  2,   23,  0,    0,      0,   0,   "Text:",         NULL, NULL },
   { d_agup_edit_proc,   48, 80,  144 + 110,  20,  2,   23,  0,    0,      255,   0,   eof_etext,           NULL, NULL },
   { d_agup_button_proc, 12 + 55,  112, 84,  28, 2,   23,  '\r',    D_EXIT, 0,   0,   "OK",               NULL, NULL },
   { d_agup_button_proc, 108 + 55, 112, 78,  28, 2,   23,  0,    D_EXIT, 0,   0,   "Cancel",           NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

void eof_prepare_note_menu(void)
{
	int selected = 0;
	int vselected = 0;
	int insp = 0, insolo = 0, inll = 0;
	int spstart = -1;
	int spend = -1;
	int spp = 0;
	int ssstart = -1;
	int ssend = -1;
	int ssp = 0;
	int llstart = -1;
	int llend = -1;
	int llp = 0;
	int i, j;
	int sel_start = eof_music_length, sel_end = 0;
	int firstnote = 0, lastnote;

	if(eof_song && eof_song_loaded)
	{
		if(eof_vocals_selected)
		{
			for(i = 0; i < eof_song->vocal_track->lyrics; i++)
			{
				if(eof_selection.track == EOF_TRACK_VOCALS && eof_selection.multi[i])
				{
					if(eof_song->vocal_track->lyric[i]->pos < sel_start)
					{
						sel_start = eof_song->vocal_track->lyric[i]->pos;
					}
					if(eof_song->vocal_track->lyric[i]->pos > sel_end)
					{
						sel_end = eof_song->vocal_track->lyric[i]->pos + eof_song->vocal_track->lyric[i]->length;
					}
				}
				if(eof_selection.track == EOF_TRACK_VOCALS && eof_selection.multi[i])
				{
					selected++;
					vselected++;
				}
				if(firstnote < 0)
				{
					firstnote = i;
				}
				lastnote = i;
			}
			for(j = 0; j < eof_song->vocal_track->lines; j++)
			{
				if(sel_end >= eof_song->vocal_track->line[j].start_pos && sel_start <= eof_song->vocal_track->line[j].end_pos)
				{
					inll = 1;
					llstart = sel_start;
					llend = sel_end;
					llp = j;
				}
			}
		}
		else
		{
			for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
			{
				if(eof_selection.track == eof_selected_track && eof_selection.multi[i] && eof_song->track[eof_selected_track]->note[i]->type == eof_note_type)
				{
					if(eof_song->track[eof_selected_track]->note[i]->pos < sel_start)
					{
						sel_start = eof_song->track[eof_selected_track]->note[i]->pos;
					}
					if(eof_song->track[eof_selected_track]->note[i]->pos > sel_end)
					{
						sel_end = eof_song->track[eof_selected_track]->note[i]->pos + eof_song->track[eof_selected_track]->note[i]->length;
					}
				}
				if(eof_selection.track == eof_selected_track && eof_selection.multi[i] && eof_song->track[eof_selected_track]->note[i]->type == eof_note_type)
				{
					selected++;
					if(eof_song->track[eof_selected_track]->note[i]->note)
					{
						vselected++;
					}
				}
				if(eof_song->track[eof_selected_track]->note[i]->type == eof_note_type)
				{
					if(firstnote < 0)
					{
						firstnote = i;
					}
					lastnote = i;
				}
			}
			for(j = 0; j < eof_song->track[eof_selected_track]->star_power_paths; j++)
			{
				if(sel_end >= eof_song->track[eof_selected_track]->star_power_path[j].start_pos && sel_start <= eof_song->track[eof_selected_track]->star_power_path[j].end_pos)
				{
					insp = 1;
					spstart = sel_start;
					spend = sel_end;
					spp = j;
				}
			}
			for(j = 0; j < eof_song->track[eof_selected_track]->solos; j++)
			{
				if(sel_end >= eof_song->track[eof_selected_track]->solo[j].start_pos && sel_start <= eof_song->track[eof_selected_track]->solo[j].end_pos)
				{
					insolo = 1;
					ssstart = sel_start;
					ssend = sel_end;
					ssp = j;
				}
			}
		}
		vselected = eof_count_selected_notes(NULL, 1);
		if(vselected)
		{
			/* star power mark */
			if(spstart == eof_song->track[eof_selected_track]->star_power_path[spp].start_pos && spend == eof_song->track[eof_selected_track]->star_power_path[spp].end_pos)
			{
				eof_star_power_menu[0].flags = D_DISABLED;
			}
			else
			{
				eof_star_power_menu[0].flags = 0;
			}

			/* solo mark */
			if(ssstart == eof_song->track[eof_selected_track]->solo[ssp].start_pos && ssend == eof_song->track[eof_selected_track]->solo[ssp].end_pos)
			{
				eof_solo_menu[0].flags = D_DISABLED;
			}
			else
			{
				eof_solo_menu[0].flags = 0;
			}

			/* lyric line mark */
			if(llstart == eof_song->vocal_track->line[llp].start_pos && llend == eof_song->vocal_track->line[llp].end_pos)
			{
				eof_lyric_line_menu[0].flags = D_DISABLED;
			}
			else
			{
				eof_lyric_line_menu[0].flags = 0;
			}

			eof_note_menu[11].flags = 0; // solos
			eof_note_menu[12].flags = 0; // star power
			eof_note_menu[13].flags = 0; // lyric lines
		}
		else
		{
			eof_star_power_menu[0].flags = D_DISABLED; // star power mark
			eof_solo_menu[0].flags = D_DISABLED; // solo mark
			eof_lyric_line_menu[0].flags = D_DISABLED; // lyric line mark
			eof_note_menu[6].flags = D_DISABLED; // edit lyric
			eof_note_menu[11].flags = D_DISABLED; // solos
			eof_note_menu[12].flags = D_DISABLED; // star power
			eof_note_menu[13].flags = 0; // lyric lines
		}

		/* star power remove */
		if(insp)
		{
			eof_star_power_menu[1].flags = 0;
			ustrcpy(eof_star_power_menu_mark_text, "Re-&Mark");
		}
		else
		{
			eof_star_power_menu[1].flags = D_DISABLED;
			ustrcpy(eof_star_power_menu_mark_text, "&Mark");
		}

		/* solo remove */
		if(insolo)
		{
			eof_solo_menu[1].flags = 0;
			ustrcpy(eof_solo_menu_mark_text, "Re-&Mark");
		}
		else
		{
			eof_solo_menu[1].flags = D_DISABLED;
			ustrcpy(eof_solo_menu_mark_text, "&Mark");
		}

		/* lyric line */
		if(inll)
		{
			eof_lyric_line_menu[1].flags = 0; // remove
			eof_lyric_line_menu[4].flags = 0; // toggle overdrive
			ustrcpy(eof_lyric_line_menu_mark_text, "Re-&Mark");
		}
		else
		{
			eof_lyric_line_menu[1].flags = D_DISABLED; // remove
			eof_lyric_line_menu[4].flags = D_DISABLED; // toggle overdrive
			ustrcpy(eof_lyric_line_menu_mark_text, "&Mark");
		}

		/* star power erase all */
		if(eof_song->track[eof_selected_track]->star_power_paths > 0)
		{
			eof_star_power_menu[2].flags = 0;
		}
		else
		{
			eof_star_power_menu[2].flags = D_DISABLED;
		}

		/* solo erase all */
		if(eof_song->track[eof_selected_track]->solos > 0)
		{
			eof_solo_menu[2].flags = 0;
		}
		else
		{
			eof_solo_menu[2].flags = D_DISABLED;
		}

		/* lyric lines erase all */
		if(eof_song->vocal_track->lines > 0)
		{
			eof_lyric_line_menu[2].flags = 0;
		}
		else
		{
			eof_lyric_line_menu[2].flags = D_DISABLED;
		}

		/* resnap */
		if(eof_snap_mode == EOF_SNAP_OFF)
		{
			eof_note_menu[4].flags = D_DISABLED;
		}
		else
		{
			eof_note_menu[4].flags = 0;
		}

		if(eof_vocals_selected)
		{
			eof_note_menu[0].flags = D_DISABLED; // toggle
			eof_note_menu[1].flags = D_DISABLED; // transpose up
			eof_note_menu[2].flags = D_DISABLED; // transpose down

			if(eof_selection.current < eof_song->vocal_track->lyrics && vselected == 1)
			{
				eof_note_menu[6].flags = 0; // edit lyric
				eof_note_menu[7].flags = 0; // split lyric
			}
			else
			{
				eof_note_menu[6].flags = D_DISABLED; // edit lyric
				eof_note_menu[7].flags = D_DISABLED; // split lyric
			}
			eof_note_menu[9].flags = D_DISABLED; // toggle crazy
			eof_note_menu[11].flags = D_DISABLED; // solos
			eof_note_menu[12].flags = D_DISABLED; // star power

			/* lyric lines */
			if(eof_song->vocal_track->lines > 0)
			{
				eof_note_menu[13].flags = 0;
			}
//			else
//			{
//				eof_note_menu[13].flags = D_DISABLED;
//			}
			eof_note_menu[15].flags = D_DISABLED; // HOPO
		}
		else
		{
			eof_note_menu[0].flags = 0; // toggle

			/* transpose up */
			if(eof_transpose_possible(-1))
			{
				eof_note_menu[1].flags = 0;
			}
			else
			{
				eof_note_menu[1].flags = D_DISABLED;
			}

			/* transpose down */
			if(eof_transpose_possible(1))
			{
				eof_note_menu[2].flags = 0;
			}
			else
			{
				eof_note_menu[2].flags = D_DISABLED;
			}

			eof_note_menu[6].flags = D_DISABLED; // edit lyric
			eof_note_menu[7].flags = D_DISABLED; // edit lyric

			/* solos */
			if(selected)
			{
				eof_note_menu[11].flags = 0;
			}
			else
			{
				eof_note_menu[11].flags = D_DISABLED;
			}

			/* star power */
			if(selected)
			{
				eof_note_menu[12].flags = 0;
			}
			else
			{
				eof_note_menu[12].flags = D_DISABLED;
			}
			eof_note_menu[13].flags = D_DISABLED; // lyric lines
			eof_note_menu[15].flags = 0; // HOPO
		}
	}
}

int eof_menu_note_transpose_up(void)
{
	int i;

	if(!eof_transpose_possible(-1))
	{
		return 1;
	}
	if(eof_vocals_selected)
	{
		eof_prepare_undo(EOF_UNDO_TYPE_LYRIC_NOTE);	//Perform a cumulative undo for lyric pitch transpose operations
		for(i = 0; i < eof_song->vocal_track->lyrics; i++)
		{
			if(eof_selection.track == EOF_TRACK_VOCALS && eof_selection.multi[i])
			{
				eof_song->vocal_track->lyric[i]->note++;
			}
		}
	}
	else
	{
		eof_prepare_undo(0);
		for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
		{
			if(eof_selection.track == eof_selected_track && eof_selection.multi[i] && eof_song->track[eof_selected_track]->note[i]->type == eof_note_type)
			{
				eof_song->track[eof_selected_track]->note[i]->note = (eof_song->track[eof_selected_track]->note[i]->note << 1) & 31;
			}
		}
	}
	return 1;
}

int eof_menu_note_transpose_down(void)
{
	int i;

	if(!eof_transpose_possible(1))
	{
		return 1;
	}
	if(eof_vocals_selected)
	{
		eof_prepare_undo(EOF_UNDO_TYPE_LYRIC_NOTE);	//Perform a cumulative undo for lyric pitch transpose operations
		for(i = 0; i < eof_song->vocal_track->lyrics; i++)
		{
			if(eof_selection.track == EOF_TRACK_VOCALS && eof_selection.multi[i])
			{
				eof_song->vocal_track->lyric[i]->note--;
			}
		}
	}
	else
	{
		eof_prepare_undo(0);
		for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
		{
			if(eof_selection.track == eof_selected_track && eof_selection.multi[i] && eof_song->track[eof_selected_track]->note[i]->type == eof_note_type)
			{
				eof_song->track[eof_selected_track]->note[i]->note = (eof_song->track[eof_selected_track]->note[i]->note >> 1) & 31;
			}
		}
	}
	return 1;
}

int eof_menu_note_transpose_up_octave(void)
{
	int i;

	if(!eof_transpose_possible(-12))	//If lyric cannot move up one octave
		return 1;
	if(!eof_vocals_selected)			//If PART VOCALS is not active
		return 1;

	eof_prepare_undo(EOF_UNDO_TYPE_LYRIC_NOTE);	//Perform a cumulative undo for lyric pitch transpose operations

	for(i = 0; i < eof_song->vocal_track->lyrics; i++)
		if(eof_selection.track == EOF_TRACK_VOCALS && eof_selection.multi[i])
			eof_song->vocal_track->lyric[i]->note+=12;

	return 1;
}

int eof_menu_note_transpose_down_octave(void)
{
	int i;

	if(!eof_transpose_possible(12))		//If lyric cannot move down one octave
		return 1;
	if(!eof_vocals_selected)		//If PART VOCALS is not active
		return 1;


	eof_prepare_undo(EOF_UNDO_TYPE_LYRIC_NOTE);	//Perform a cumulative undo for lyric pitch transpose operations

	for(i = 0; i < eof_song->vocal_track->lyrics; i++)
		if(eof_selection.track == EOF_TRACK_VOCALS && eof_selection.multi[i])
			eof_song->vocal_track->lyric[i]->note-=12;

	return 1;
}

int eof_menu_note_resnap_vocal(void)
{
	int i;
	int oldnotes = eof_song->vocal_track->lyrics;

	if(eof_snap_mode == EOF_SNAP_OFF)
	{
		return 1;
	}
	eof_prepare_undo(0);
	for(i = 0; i < eof_song->vocal_track->lyrics; i++)
	{
		if(eof_selection.track == EOF_TRACK_VOCALS && eof_selection.multi[i])
		{

			/* snap the note itself */
			eof_snap_logic(&eof_tail_snap, eof_song->vocal_track->lyric[i]->pos);
			eof_song->vocal_track->lyric[i]->pos = eof_tail_snap.pos;
		}
	}
	eof_vocal_track_fixup_lyrics(eof_song->vocal_track, 1);
	if(oldnotes != eof_song->vocal_track->lyrics)
	{
		allegro_message("Warning! Some lyrics snapped to the same position and were automatically combined.");
	}
	return 1;
}

int eof_menu_note_resnap(void)
{
	if(eof_vocals_selected)
	{
		return eof_menu_note_resnap_vocal();
	}
	int i;
	int oldnotes = eof_song->track[eof_selected_track]->notes;

	if(eof_snap_mode == EOF_SNAP_OFF)
	{
		return 1;
	}
	eof_prepare_undo(0);
	for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
	{
		if(eof_selection.track == eof_selected_track && eof_selection.multi[i] && eof_song->track[eof_selected_track]->note[i]->type == eof_note_type)
		{

			/* snap the note itself */
			eof_snap_logic(&eof_tail_snap, eof_song->track[eof_selected_track]->note[i]->pos);
			eof_song->track[eof_selected_track]->note[i]->pos = eof_tail_snap.pos;

			/* snap the tail */
			eof_snap_logic(&eof_tail_snap, eof_song->track[eof_selected_track]->note[i]->pos + eof_song->track[eof_selected_track]->note[i]->length);
			eof_snap_length_logic(&eof_tail_snap);
			if(eof_song->track[eof_selected_track]->note[i]->length > 1)
			{
				eof_snap_logic(&eof_tail_snap, eof_song->track[eof_selected_track]->note[i]->pos + eof_song->track[eof_selected_track]->note[i]->length);
				eof_set_tail_pos(i, eof_tail_snap.pos);
			}
		}
	}
	eof_track_fixup_notes(eof_song->track[eof_selected_track], 1);
	if(oldnotes != eof_song->track[eof_selected_track]->notes)
	{
		allegro_message("Warning! Some notes snapped to the same position and were automatically combined.");
	}
	eof_determine_hopos();
	return 1;
}

int eof_menu_note_delete_vocal(void)
{
	int i, d = 0;

	for(i = 0; i < eof_song->vocal_track->lyrics; i++)
	{
		if(eof_selection.track == EOF_TRACK_VOCALS && eof_selection.multi[i])
		{
			d++;
		}
	}
	if(d)
	{
		eof_prepare_undo(EOF_UNDO_TYPE_NOTE_SEL);
		for(i = eof_song->vocal_track->lyrics - 1; i >= 0; i--)
		{
			if(eof_selection.track == EOF_TRACK_VOCALS && eof_selection.multi[i])
			{
				eof_vocal_track_delete_lyric(eof_song->vocal_track, i);
				eof_selection.multi[i] = 0;
			}
		}
		eof_selection.current = EOF_MAX_NOTES - 1;
		eof_vocal_track_fixup_lyrics(eof_song->vocal_track, 0);
		eof_detect_difficulties(eof_song);
		eof_reset_lyric_preview_lines();
	}
	return 1;
}

int eof_menu_note_delete(void)
{
	if(eof_vocals_selected)
	{
		return eof_menu_note_delete_vocal();
	}
	int i, d = 0;

	for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
	{
		if(eof_selection.track == eof_selected_track && eof_selection.multi[i] && eof_song->track[eof_selected_track]->note[i]->type == eof_note_type)
		{
			d++;
		}
	}
	if(d)
	{
		eof_prepare_undo(EOF_UNDO_TYPE_NOTE_SEL);
		for(i = eof_song->track[eof_selected_track]->notes - 1; i >= 0; i--)
		{
			if(eof_selection.track == eof_selected_track && eof_selection.multi[i] && eof_selection.track == eof_selected_track && eof_song->track[eof_selected_track]->note[i]->type == eof_note_type)
			{
				eof_track_delete_note(eof_song->track[eof_selected_track], i);
				eof_selection.multi[i] = 0;
			}
		}
		eof_selection.current = EOF_MAX_NOTES - 1;
		eof_detect_difficulties(eof_song);
		eof_determine_hopos();
	}
	return 1;
}

int eof_menu_note_toggle_green(void)
{
	int i;

	if(eof_count_selected_notes(NULL, 0) <= 0)
	{
		return 1;
	}
	eof_prepare_undo(0);
	for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
	{
		if(eof_selection.track == eof_selected_track && eof_selection.multi[i] && eof_song->track[eof_selected_track]->note[i]->type == eof_note_type)
		{
			eof_song->track[eof_selected_track]->note[i]->note ^= 1;
		}
	}
	return 1;
}

int eof_menu_note_toggle_red(void)
{
	int i;

	if(eof_count_selected_notes(NULL, 0) <= 0)
	{
		return 1;
	}
	eof_prepare_undo(0);
	for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
	{
		if(eof_selection.track == eof_selected_track && eof_selection.multi[i] && eof_song->track[eof_selected_track]->note[i]->type == eof_note_type)
		{
			eof_song->track[eof_selected_track]->note[i]->note ^= 2;
		}
	}
	return 1;
}

int eof_menu_note_toggle_yellow(void)
{
	int i;

	if(eof_count_selected_notes(NULL, 0) <= 0)
	{
		return 1;
	}
	eof_prepare_undo(0);
	for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
	{
		if(eof_selection.track == eof_selected_track && eof_selection.multi[i] && eof_song->track[eof_selected_track]->note[i]->type == eof_note_type)
		{
			eof_song->track[eof_selected_track]->note[i]->note ^= 4;
		}
	}
	return 1;
}

int eof_menu_note_toggle_blue(void)
{
	int i;

	if(eof_count_selected_notes(NULL, 0) <= 0)
	{
		return 1;
	}
	eof_prepare_undo(0);
	for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
	{
		if(eof_selection.track == eof_selected_track && eof_selection.multi[i] && eof_song->track[eof_selected_track]->note[i]->type == eof_note_type)
		{
			eof_song->track[eof_selected_track]->note[i]->note ^= 8;
		}
	}
	return 1;
}

int eof_menu_note_toggle_purple(void)
{
	int i;

	if(eof_count_selected_notes(NULL, 0) <= 0)
	{
		return 1;
	}
	eof_prepare_undo(0);
	for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
	{
		if(eof_selection.track == eof_selected_track && eof_selection.multi[i] && eof_song->track[eof_selected_track]->note[i]->type == eof_note_type)
		{
			eof_song->track[eof_selected_track]->note[i]->note ^= 16;
		}
	}
	return 1;
}

int eof_menu_note_toggle_crazy(void)
{
	int i;
	int u = 0;

	for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
	{
		if(eof_selection.track == eof_selected_track && eof_selection.multi[i] && eof_song->track[eof_selected_track]->note[i]->type == eof_note_type)
		{
			if(!u)
			{
				eof_prepare_undo(0);
				u = 1;
			}
			eof_song->track[eof_selected_track]->note[i]->flags ^= EOF_NOTE_FLAG_CRAZY;
		}
	}
	return 1;
}

float eof_menu_note_push_get_offset(void)
{
	switch(eof_snap_mode)
	{
		case EOF_SNAP_QUARTER:
		{
			return 100.0;
		}
		case EOF_SNAP_EIGHTH:
		{
			return 50.0;
		}
		case EOF_SNAP_TWELFTH:
		{
			return 100.0 / 3.0;
		}
		case EOF_SNAP_SIXTEENTH:
		{
			return 25.0;
		}
		case EOF_SNAP_TWENTY_FOURTH:
		{
			return 100.0 / 6.0;
		}
		case EOF_SNAP_THIRTY_SECOND:
		{
			return 12.5;
		}
		case EOF_SNAP_FORTY_EIGHTH:
		{
			return 100.0 / 12.0;
		}
		case EOF_SNAP_CUSTOM:
		{
			return 100.0 / (float)eof_snap_interval;
		}
	}
	return 0.0;
}

int eof_menu_note_push_back(void)
{
	int i;
	float porpos;
	int beat;

	if(eof_count_selected_notes(NULL, 0) > 0)
	{
		for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
		{
			if(eof_selection.multi[i])
			{
				beat = eof_get_beat(eof_song->track[eof_selected_track]->note[i]->pos);
				porpos = eof_get_porpos(eof_song->track[eof_selected_track]->note[i]->pos);
				eof_song->track[eof_selected_track]->note[i]->pos = eof_put_porpos(beat, porpos, eof_menu_note_push_get_offset());
			}
		}
	}
	else
	{
		for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
		{
			if(eof_song->track[eof_selected_track]->note[i]->pos >= eof_music_pos - eof_av_delay)
			{
				beat = eof_get_beat(eof_song->track[eof_selected_track]->note[i]->pos);
				porpos = eof_get_porpos(eof_song->track[eof_selected_track]->note[i]->pos);
				eof_song->track[eof_selected_track]->note[i]->pos = eof_put_porpos(beat, porpos, eof_menu_note_push_get_offset());
			}
		}
	}
	return 1;
}

int eof_menu_note_push_up(void)
{
	int i;
	float porpos;
	int beat;

	if(eof_count_selected_notes(NULL, 0) > 0)
	{
		for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
		{
			if(eof_selection.multi[i])
			{
				beat = eof_get_beat(eof_song->track[eof_selected_track]->note[i]->pos);
				porpos = eof_get_porpos(eof_song->track[eof_selected_track]->note[i]->pos);
				eof_song->track[eof_selected_track]->note[i]->pos = eof_put_porpos(beat, porpos, -eof_menu_note_push_get_offset());
			}
		}
	}
	else
	{
		for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
		{
			if(eof_song->track[eof_selected_track]->note[i]->pos >= eof_music_pos - eof_av_delay)
			{
				beat = eof_get_beat(eof_song->track[eof_selected_track]->note[i]->pos);
				porpos = eof_get_porpos(eof_song->track[eof_selected_track]->note[i]->pos);
				eof_song->track[eof_selected_track]->note[i]->pos = eof_put_porpos(beat, porpos, -eof_menu_note_push_get_offset());
			}
		}
	}
	return 1;
}

int eof_menu_note_create_bre(void)
{
	int i;
	int first_pos = 0;
	int last_pos = eof_music_length;
	EOF_NOTE * new_note = NULL;

	for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
	{
		if(eof_selection.track == eof_selected_track && eof_selection.multi[i] && eof_song->track[eof_selected_track]->note[i]->type == eof_note_type)
		{
			if(eof_song->track[eof_selected_track]->note[i]->pos > first_pos)
			{
				first_pos = eof_song->track[eof_selected_track]->note[i]->pos;
			}
			if(eof_song->track[eof_selected_track]->note[i]->pos < last_pos)
			{
				last_pos = eof_song->track[eof_selected_track]->note[i]->pos;
			}
		}
	}

	/* create the BRE marking note */
	if(first_pos != 0 && last_pos != eof_music_length)
	{
		new_note = eof_track_add_note(eof_song->track[eof_selected_track]);
		eof_note_create(new_note, 1, 1, 1, 1, 1, first_pos, last_pos - first_pos);
//		new_note->type = EOF_NOTE_SPECIAL;
//		new_note->flags = EOF_NOTE_FLAG_BRE;
	}
	return 1;
}

/* split a lyric into multiple pieces (look for ' ' characters) */
static void eof_split_lyric(int lyric)
{
	int i, l, c = 0, lastc;
	int first = 1;
	int piece = 1;
	int pieces = 1;
	char * token = NULL;
	EOF_LYRIC * new_lyric = NULL;

	/* see how many pieces there are */
	for(i = 0; i < strlen(eof_song->vocal_track->lyric[lyric]->text); i++)
	{
		lastc = c;
		c = eof_song->vocal_track->lyric[lyric]->text[i];
		if(c == ' ' && lastc != ' ')
		{
			pieces++;
		}
	}

	/* shorten the original note */
	if(eof_song->vocal_track->lyric[lyric]->note >= MINPITCH && eof_song->vocal_track->lyric[lyric]->note <= MAXPITCH)
	{
		l = eof_song->vocal_track->lyric[lyric]->length > 100 ? eof_song->vocal_track->lyric[lyric]->length : 250 * pieces;
	}
	else
	{
		l = 250 * pieces;
	}
	eof_song->vocal_track->lyric[lyric]->length = l / pieces - 20;
	if(eof_song->vocal_track->lyric[lyric]->length < 1)
	{
		eof_song->vocal_track->lyric[lyric]->length = 1;
	}

	/* split at spaces */
	strtok(eof_song->vocal_track->lyric[lyric]->text, " ");
	do
	{
		token = strtok(NULL, " ");
		if(token)
		{
//			if(!first)
			{
				new_lyric = eof_vocal_track_add_lyric(eof_song->vocal_track);
				new_lyric->pos = eof_song->vocal_track->lyric[lyric]->pos + (l / pieces) * piece;
				new_lyric->note = eof_song->vocal_track->lyric[lyric]->note;
				new_lyric->length = l / pieces - 20;
				if(new_lyric->length < 1)
				{
					new_lyric->length = 1;
				}
				ustrcpy(new_lyric->text, token);
				piece++;
			}
			first = 0;
		}
	} while(token != NULL);
	eof_vocal_track_sort_lyrics(eof_song->vocal_track);
	eof_vocal_track_fixup_lyrics(eof_song->vocal_track, 1);
}

int eof_menu_split_lyric(void)
{
	if(eof_count_selected_notes(NULL, 0) != 1)
	{
		return 1;
	}
	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_split_lyric_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_split_lyric_dialog);
	ustrcpy(eof_etext, eof_song->vocal_track->lyric[eof_selection.current]->text);
	if(eof_popup_dialog(eof_split_lyric_dialog, 2) == 3)
	{
		if(ustricmp(eof_song->vocal_track->lyric[eof_selection.current]->text, eof_etext))
		{
			eof_prepare_undo(0);
			ustrcpy(eof_song->vocal_track->lyric[eof_selection.current]->text, eof_etext);
			eof_split_lyric(eof_selection.current);
		}
	}
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(screen);
	return D_O_K;
}

int eof_menu_solo_mark(void)
{
	int i, j;
	int insp = -1;
	unsigned long sel_start = -1;
	unsigned long sel_end = 0;

	for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
	{
		if(eof_selection.track == eof_selected_track && eof_selection.multi[i] && eof_song->track[eof_selected_track]->note[i]->type == eof_note_type)
		{
			if(eof_song->track[eof_selected_track]->note[i]->pos < sel_start)
			{
				sel_start = eof_song->track[eof_selected_track]->note[i]->pos;
			}
			if(eof_song->track[eof_selected_track]->note[i]->pos > sel_end)
			{
				sel_end = eof_song->track[eof_selected_track]->note[i]->pos + 20;
			}
		}
	}
	for(j = 0; j < eof_song->track[eof_selected_track]->solos; j++)
	{
		if(sel_end >= eof_song->track[eof_selected_track]->solo[j].start_pos && sel_start <= eof_song->track[eof_selected_track]->solo[j].end_pos)
		{
			insp = j;
		}
	}
	eof_prepare_undo(0);
	if(insp < 0)
	{
		eof_track_add_solo(eof_song->track[eof_selected_track], sel_start, sel_end);
	}
	else
	{
		eof_song->track[eof_selected_track]->solo[insp].start_pos = sel_start;
		eof_song->track[eof_selected_track]->solo[insp].end_pos = sel_end;
	}
	return 1;
}

int eof_menu_solo_unmark(void)
{
	int i, j;

	eof_prepare_undo(0);
	for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
	{
		if(eof_selection.track == eof_selected_track && eof_selection.multi[i] && eof_song->track[eof_selected_track]->note[i]->type == eof_note_type)
		{
			for(j = 0; j < eof_song->track[eof_selected_track]->solos; j++)
			{
				if(eof_song->track[eof_selected_track]->note[i]->pos >= eof_song->track[eof_selected_track]->solo[j].start_pos && eof_song->track[eof_selected_track]->note[i]->pos <= eof_song->track[eof_selected_track]->solo[j].end_pos)
				{
					eof_track_delete_solo(eof_song->track[eof_selected_track], j);
					break;
				}
			}
		}
	}
	return 1;
}

int eof_menu_solo_erase_all(void)
{
	if(alert(NULL, "Erase all solos from this track?", NULL, "&Yes", "&No", 'y', 'n') == 1)
	{
		eof_prepare_undo(0);
		eof_song->track[eof_selected_track]->solos = 0;
	}
	return 1;
}

int eof_menu_star_power_mark(void)
{
	int i, j;
	int insp = -1;
	unsigned long sel_start = -1;
	unsigned long sel_end = 0;

	for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
	{
		if(eof_selection.track == eof_selected_track && eof_selection.multi[i] && eof_song->track[eof_selected_track]->note[i]->type == eof_note_type)
		{
			if(eof_song->track[eof_selected_track]->note[i]->pos < sel_start)
			{
				sel_start = eof_song->track[eof_selected_track]->note[i]->pos;
			}
			if(eof_song->track[eof_selected_track]->note[i]->pos > sel_end)
			{
				sel_end = eof_song->track[eof_selected_track]->note[i]->pos + (eof_song->track[eof_selected_track]->note[i]->length > 20 ? eof_song->track[eof_selected_track]->note[i]->length : 20);
			}
		}
	}
	for(j = 0; j < eof_song->track[eof_selected_track]->star_power_paths; j++)
	{
		if(sel_end >= eof_song->track[eof_selected_track]->star_power_path[j].start_pos && sel_start <= eof_song->track[eof_selected_track]->star_power_path[j].end_pos)
		{
			insp = j;
		}
	}
	eof_prepare_undo(0);
	if(insp < 0)
	{
		eof_track_add_star_power(eof_song->track[eof_selected_track], sel_start, sel_end);
	}
	else
	{
		eof_song->track[eof_selected_track]->star_power_path[insp].start_pos = sel_start;
		eof_song->track[eof_selected_track]->star_power_path[insp].end_pos = sel_end;
	}
	eof_determine_hopos();
	return 1;
}

int eof_menu_star_power_unmark(void)
{
	int i, j;

	eof_prepare_undo(0);
	for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
	{
		if(eof_selection.track == eof_selected_track && eof_selection.multi[i] && eof_song->track[eof_selected_track]->note[i]->type == eof_note_type)
		{
			for(j = 0; j < eof_song->track[eof_selected_track]->star_power_paths; j++)
			{
				if(eof_song->track[eof_selected_track]->note[i]->pos >= eof_song->track[eof_selected_track]->star_power_path[j].start_pos && eof_song->track[eof_selected_track]->note[i]->pos <= eof_song->track[eof_selected_track]->star_power_path[j].end_pos)
				{
					eof_track_delete_star_power(eof_song->track[eof_selected_track], j);
					break;
				}
			}
		}
	}
	eof_determine_hopos();
	return 1;
}

int eof_menu_star_power_erase_all(void)
{
	if(alert(NULL, "Erase all star power from this track?", NULL, "&Yes", "&No", 'y', 'n') == 1)
	{
		eof_prepare_undo(0);
		eof_song->track[eof_selected_track]->star_power_paths = 0;
	}
	eof_determine_hopos();
	return 1;
}

int eof_menu_lyric_line_mark(void)
{
	int i, j;
	int insp = -1;
	long sel_start = -1;
	long sel_end = 0;

	for(i = 0; i < eof_song->vocal_track->lyrics; i++)
	{
		if(eof_selection.track == EOF_TRACK_VOCALS && eof_selection.multi[i])
		{
			if(eof_song->vocal_track->lyric[i]->pos < sel_start)
			{
				sel_start = eof_song->vocal_track->lyric[i]->pos;
				if(sel_start < eof_song->tags->ogg[eof_selected_ogg].midi_offset)
				{
					sel_start = eof_song->tags->ogg[eof_selected_ogg].midi_offset;
				}
			}
			if(eof_song->vocal_track->lyric[i]->pos > sel_end)
			{
				sel_end = eof_song->vocal_track->lyric[i]->pos + eof_song->vocal_track->lyric[i]->length;
				if(sel_end >= eof_music_length)
				{
					sel_end = eof_music_length - 1;
				}
			}
		}
	}
	eof_prepare_undo(0);	//Create the undo state before removing/adding phrase(s)
	for(j = eof_song->vocal_track->lines; j >= 0; j--)
	{
		if(sel_end >= eof_song->vocal_track->line[j].start_pos && sel_start <= eof_song->vocal_track->line[j].end_pos)
		{
			eof_vocal_track_delete_line(eof_song->vocal_track, j);
			insp = j;
		}
	}
	eof_vocal_track_add_line(eof_song->vocal_track, sel_start, sel_end);

	/* check for overlapping lines */
	for(i = 0; i < eof_song->vocal_track->lines; i++)
	{
		for(j = i; j < eof_song->vocal_track->lines; j++)
		{
			if(i != j && eof_song->vocal_track->line[i].start_pos <= eof_song->vocal_track->line[j].end_pos && eof_song->vocal_track->line[j].start_pos <= eof_song->vocal_track->line[i].end_pos)
			{
				eof_song->vocal_track->line[i].start_pos = eof_song->vocal_track->line[j].end_pos + 1;
			}
		}
	}
	eof_reset_lyric_preview_lines();
	return 1;
}

int eof_menu_lyric_line_unmark(void)
{
	int i, j;

	eof_prepare_undo(0);
	for(i = 0; i < eof_song->vocal_track->lyrics; i++)
	{
		if(eof_selection.track == EOF_TRACK_VOCALS && eof_selection.multi[i])
		{
			for(j = 0; j < eof_song->vocal_track->lines; j++)
			{
				if(eof_song->vocal_track->lyric[i]->pos >= eof_song->vocal_track->line[j].start_pos && eof_song->vocal_track->lyric[i]->pos <= eof_song->vocal_track->line[j].end_pos)
				{
					eof_vocal_track_delete_line(eof_song->vocal_track, j);
					break;
				}
			}
		}
	}
	eof_reset_lyric_preview_lines();
	return 1;
}

int eof_menu_lyric_line_erase_all(void)
{
	if(alert(NULL, "Erase all lyric lines?", NULL, "&Yes", "&No", 'y', 'n') == 1)
	{
		eof_prepare_undo(0);
		eof_song->vocal_track->lines = 0;
		eof_reset_lyric_preview_lines();
	}
	return 1;
}

int eof_menu_lyric_line_toggle_overdrive(void)
{
	char used[1024] = {0};
	int i, j;

	eof_prepare_undo(0);
	for(i = 0; i < eof_song->vocal_track->lyrics; i++)
	{
		if(eof_selection.track == EOF_TRACK_VOCALS && eof_selection.multi[i])
		{
			for(j = 0; j < eof_song->vocal_track->lines; j++)
			{
				if(eof_song->vocal_track->lyric[i]->pos >= eof_song->vocal_track->line[j].start_pos && eof_song->vocal_track->lyric[i]->pos <= eof_song->vocal_track->line[j].end_pos && !used[j])
				{
					eof_song->vocal_track->line[j].flags ^= EOF_LYRIC_LINE_FLAG_OVERDRIVE;
					used[j] = 1;
				}
			}
		}
	}
	return 1;
}

int eof_menu_hopo_auto(void)
{
	int i;

	eof_prepare_undo(0);
	for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
	{
		if(eof_selection.multi[i])
		{
			if(eof_song->track[eof_selected_track]->note[i]->flags & EOF_NOTE_FLAG_F_HOPO)
			{
				eof_song->track[eof_selected_track]->note[i]->flags ^= EOF_NOTE_FLAG_F_HOPO;
			}
			if(eof_song->track[eof_selected_track]->note[i]->flags & EOF_NOTE_FLAG_NO_HOPO)
			{
				eof_song->track[eof_selected_track]->note[i]->flags ^= EOF_NOTE_FLAG_NO_HOPO;
			}
		}
	}
	eof_determine_hopos();
	return 1;
}

int eof_menu_hopo_force_on(void)
{
	int i;

	eof_prepare_undo(0);
	for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
	{
		if(eof_selection.multi[i])
		{
			eof_song->track[eof_selected_track]->note[i]->flags |= EOF_NOTE_FLAG_F_HOPO;
			if(eof_song->track[eof_selected_track]->note[i]->flags & EOF_NOTE_FLAG_NO_HOPO)
			{
				eof_song->track[eof_selected_track]->note[i]->flags ^= EOF_NOTE_FLAG_NO_HOPO;
			}
		}
	}
	eof_determine_hopos();
	return 1;
}

int eof_menu_hopo_force_off(void)
{
	int i;

	eof_prepare_undo(0);
	for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
	{
		if(eof_selection.multi[i])
		{
			eof_song->track[eof_selected_track]->note[i]->flags |= EOF_NOTE_FLAG_NO_HOPO;
			if(eof_song->track[eof_selected_track]->note[i]->flags & EOF_NOTE_FLAG_F_HOPO)
			{
				eof_song->track[eof_selected_track]->note[i]->flags ^= EOF_NOTE_FLAG_F_HOPO;
			}
		}
	}
	eof_determine_hopos();
	return 1;
}

int eof_transpose_possible(int dir)
{
	int i;

	/* no notes, no transpose */
	if(eof_vocals_selected)
	{
		if(eof_song->vocal_track->lyrics <= 0)
		{
			return 0;
		}

		if(eof_count_selected_notes(NULL, 1) <= 0)
		{
			return 0;
		}

		for(i = 0; i < eof_song->vocal_track->lyrics; i++)
		{	//Test if the lyric can transpose the given amount in the given direction
			if(eof_selection.track == EOF_TRACK_VOCALS && eof_selection.multi[i])
			{
				if(eof_song->vocal_track->lyric[i]->note == 0)
				{
					return 0;
				}
				if(eof_song->vocal_track->lyric[i]->note - dir < MINPITCH)
				{
					return 0;
				}
				else if(eof_song->vocal_track->lyric[i]->note - dir > MAXPITCH)
				{
					return 0;
				}
			}
		}
		return 1;
	}
	else
	{
		if(eof_song->track[eof_selected_track]->notes <= 0)
		{
			return 0;
		}

		if(eof_count_selected_notes(NULL, 1) <= 0)
		{
			return 0;
		}

		for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
		{
			if(eof_selection.track == eof_selected_track && eof_selection.multi[i] && eof_song->track[eof_selected_track]->note[i]->type == eof_note_type)
			{
				if(eof_song->track[eof_selected_track]->note[i]->note & 1 && dir > 0)
				{
					return 0;
				}
				else if(eof_song->track[eof_selected_track]->note[i]->note & 16 && dir < 0)
				{
					return 0;
				}
			}
		}
		return 1;
	}
}

int eof_new_lyric_dialog(void)
{
	EOF_LYRIC * new_lyric = NULL;

	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_lyric_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_lyric_dialog);
	ustrcpy(eof_etext, "");
	if(eof_popup_dialog(eof_lyric_dialog, 2) == 3)
	{
		if(!eof_check_string(eof_etext) && !eof_pen_lyric.note)	//If the placed lyric is both pitchless AND textless
			return D_O_K;	//Return without adding

		eof_prepare_undo(0);
		new_lyric = eof_vocal_track_add_lyric(eof_song->vocal_track);
		new_lyric->pos = eof_pen_lyric.pos;
		new_lyric->note = eof_pen_lyric.note;
		new_lyric->length = eof_pen_lyric.length;
		ustrcpy(new_lyric->text, eof_etext);
		eof_selection.track = EOF_TRACK_VOCALS;
		eof_selection.current_pos = new_lyric->pos;
		eof_selection.range_pos_1 = eof_selection.current_pos;
		eof_selection.range_pos_2 = eof_selection.current_pos;
		memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
		eof_vocal_track_sort_lyrics(eof_song->vocal_track);
		eof_vocal_track_fixup_lyrics(eof_song->vocal_track, 0);
		eof_detect_difficulties(eof_song);
		eof_reset_lyric_preview_lines();
	}
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(screen);
	return D_O_K;
}

int eof_edit_lyric_dialog(void)
{
	if(eof_count_selected_notes(NULL, 0) != 1)
	{
		return 1;
	}
	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_lyric_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_lyric_dialog);
	ustrcpy(eof_etext, eof_song->vocal_track->lyric[eof_selection.current]->text);
	if(eof_popup_dialog(eof_lyric_dialog, 2) == 3)	//User hit OK on "Edit Lyric" dialog instead of canceling
	{
		if(ustricmp(eof_song->vocal_track->lyric[eof_selection.current]->text, eof_etext))	//If the updated string (eof_etext) is different
		{
			eof_prepare_undo(0);
			if(!eof_check_string(eof_etext))
			{	//If the updated string is empty or just whitespace
				eof_vocal_track_delete_lyric(eof_song->vocal_track, eof_selection.current);
			}
			else
			{
				ustrcpy(eof_song->vocal_track->lyric[eof_selection.current]->text, eof_etext);
			}
		}
	}
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(screen);
	return D_O_K;
}
