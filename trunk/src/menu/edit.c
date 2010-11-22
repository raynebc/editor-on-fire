#include <allegro.h>
#include "../agup/agup.h"
#include "../undo.h"
#include "../dialog.h"
#include "../mix.h"
#include "../main.h"	//Inclusion for eof_custom_snap_measure
#include "../dialog/proc.h"
#include "edit.h"
#include "song.h"

MENU eof_edit_paste_from_menu[] =
{
    {"&Supaeasy", eof_menu_edit_paste_from_supaeasy, NULL, 0, NULL},
    {"&Easy", eof_menu_edit_paste_from_easy, NULL, 0, NULL},
    {"&Medium", eof_menu_edit_paste_from_medium, NULL, 0, NULL},
    {"&Amazing", eof_menu_edit_paste_from_amazing, NULL, 0, NULL},
    {"", NULL, NULL, 0, NULL},
    {"&Catalog", eof_menu_edit_paste_from_catalog, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_edit_snap_menu[] =
{
    {"1/4", eof_menu_edit_snap_quarter, NULL, 0, NULL},
    {"1/8", eof_menu_edit_snap_eighth, NULL, 0, NULL},
    {"1/16", eof_menu_edit_snap_sixteenth, NULL, 0, NULL},
    {"1/32", eof_menu_edit_snap_thirty_second, NULL, 0, NULL},
    {"", NULL, NULL, 0, NULL},
    {"1/12", eof_menu_edit_snap_twelfth, NULL, 0, NULL},
    {"1/24", eof_menu_edit_snap_twenty_fourth, NULL, 0, NULL},
    {"1/48", eof_menu_edit_snap_forty_eighth, NULL, 0, NULL},
    {"", NULL, NULL, 0, NULL},
    {"&Custom", eof_menu_edit_snap_custom, NULL, 0, NULL},
    {"", NULL, NULL, 0, NULL},
    {"Off\tG", eof_menu_edit_snap_off, NULL, D_SELECTED, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_edit_claps_menu[] =
{
    {"&All", eof_menu_edit_claps_all, NULL, D_SELECTED, NULL},
    {"&Green", eof_menu_edit_claps_green, NULL, 0, NULL},
    {"&Red", eof_menu_edit_claps_red, NULL, 0, NULL},
    {"&Yellow", eof_menu_edit_claps_yellow, NULL, 0, NULL},
    {"&Blue", eof_menu_edit_claps_blue, NULL, 0, NULL},
    {"&Purple", eof_menu_edit_claps_purple, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_edit_hopo_menu[] =
{
    {"&RF", eof_menu_edit_hopo_rf, NULL, D_SELECTED, NULL},
    {"&FOF", eof_menu_edit_hopo_fof, NULL, 0, NULL},
    {"&Off", eof_menu_edit_hopo_off, NULL, 0, NULL},
    {"&Manual", eof_menu_edit_hopo_manual, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_edit_zoom_menu[] =
{
    {"1/1&0", eof_menu_edit_zoom_10, NULL, D_SELECTED, NULL},
    {"1/&9", eof_menu_edit_zoom_9, NULL, 0, NULL},
    {"1/&8", eof_menu_edit_zoom_8, NULL, 0, NULL},
    {"1/&7", eof_menu_edit_zoom_7, NULL, 0, NULL},
    {"1/&6", eof_menu_edit_zoom_6, NULL, 0, NULL},
    {"1/&5", eof_menu_edit_zoom_5, NULL, 0, NULL},
    {"1/&4", eof_menu_edit_zoom_4, NULL, 0, NULL},
    {"1/&3", eof_menu_edit_zoom_3, NULL, 0, NULL},
    {"1/&2", eof_menu_edit_zoom_2, NULL, 0, NULL},
    {"1/&1", eof_menu_edit_zoom_1, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_edit_playback_menu[] =
{
    {"&100%", eof_menu_edit_playback_100, NULL, D_SELECTED, NULL},
    {"&75%", eof_menu_edit_playback_75, NULL, 0, NULL},
    {"&50%", eof_menu_edit_playback_50, NULL, 0, NULL},
    {"&25%", eof_menu_edit_playback_25, NULL, 0, NULL},
    {"&Custom", eof_menu_edit_playback_custom, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_edit_speed_menu[] =
{
    {"&Slow", eof_menu_edit_speed_slow, NULL, D_SELECTED, NULL},
    {"&Medium", eof_menu_edit_speed_medium, NULL, 0, NULL},
    {"&Fast", eof_menu_edit_speed_fast, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_edit_bookmark_menu[] =
{
    {"&0\tCtrl+Num 0", eof_menu_edit_bookmark_0, NULL, 0, NULL},
    {"&1\tCtrl+Num 1", eof_menu_edit_bookmark_1, NULL, 0, NULL},
    {"&2\tCtrl+Num 2", eof_menu_edit_bookmark_2, NULL, 0, NULL},
    {"&3\tCtrl+Num 3", eof_menu_edit_bookmark_3, NULL, 0, NULL},
    {"&4\tCtrl+Num 4", eof_menu_edit_bookmark_4, NULL, 0, NULL},
    {"&5\tCtrl+Num 5", eof_menu_edit_bookmark_5, NULL, 0, NULL},
    {"&6\tCtrl+Num 6", eof_menu_edit_bookmark_6, NULL, 0, NULL},
    {"&7\tCtrl+Num 7", eof_menu_edit_bookmark_7, NULL, 0, NULL},
    {"&8\tCtrl+Num 8", eof_menu_edit_bookmark_8, NULL, 0, NULL},
    {"&9\tCtrl+Num 9", eof_menu_edit_bookmark_9, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_edit_selection_menu[] =
{
    {"&Select All\tCtrl+A", eof_menu_edit_select_all, NULL, 0, NULL},
    {"Select &Like\tCtrl+L", eof_menu_edit_select_like, NULL, 0, NULL},
    {"Select &Rest\tShift+End", eof_menu_edit_select_rest, NULL, 0, NULL},
    {"&Deselect All\tCtrl+D", eof_menu_edit_deselect_all, NULL, 0, NULL},
    {"Select &Previous\tShift+Home", eof_menu_edit_select_previous, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_edit_menu[] =
{
    {"&Undo\tCtrl+Z", eof_menu_edit_undo, NULL, D_DISABLED, NULL},
    {"&Redo\tCtrl+R", eof_menu_edit_redo, NULL, D_DISABLED, NULL},
    {"", NULL, NULL, 0, NULL},
    {"&Copy\tCtrl+C", eof_menu_edit_copy, NULL, 0, NULL},
    {"&Paste\tCtrl+V", eof_menu_edit_paste, NULL, 0, NULL},
    {"Old Paste\tCtrl+P", eof_menu_edit_old_paste, NULL, 0, NULL},
    {"Paste &From", NULL, eof_edit_paste_from_menu, 0, NULL},
    {"", NULL, NULL, 0, NULL},
    {"&Grid Snap", NULL, eof_edit_snap_menu, 0, NULL},
    {"&Zoom", NULL, eof_edit_zoom_menu, 0, NULL},
    {"Preview Sp&eed", NULL, eof_edit_speed_menu, 0, NULL},
    {"Playback R&ate", NULL, eof_edit_playback_menu, 0, NULL},
    {"Preview &HOPO", NULL, eof_edit_hopo_menu, 0, NULL},
    {"", NULL, NULL, 0, NULL},
    {"&Metronome\tM", eof_menu_edit_metronome, NULL, 0, NULL},
    {"Claps\tC", eof_menu_edit_claps, NULL, 0, NULL},
    {"Clap &Notes", NULL, eof_edit_claps_menu, 0, NULL},
    {"&Vocal Tones\tV", eof_menu_edit_vocal_tones, NULL, 0, NULL},
    {"MIDI &Tones", eof_menu_edit_midi_tones, NULL, 0, NULL},
    {"", NULL, NULL, 0, NULL},
    {"&Bookmark", NULL, eof_edit_bookmark_menu, 0, NULL},
    {"", NULL, NULL, 0, NULL},
    {"&Selection", NULL, eof_edit_selection_menu, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

DIALOG eof_custom_snap_dialog[] =
{
   /* (proc)				(x)		(y)		(w)		(h)  		(fg)	(bg) (key) (flags)	(d1) (d2) (dp)			(dp2) (dp3) */
   { d_agup_shadow_box_proc,32,		68,		170, 	72 + 8 +15,	2,		23,  0,    0,		0,   0,   NULL,			NULL, NULL },
   { d_agup_text_proc,		56,		84,		64,		8,			2,		23,  0,    0,		0,   0,   "Intervals:",	NULL, NULL },
   { eof_verified_edit_proc,112,	80,		66,		20,			2,		23,  0,    0,		8,   0,   eof_etext2,	"0123456789", NULL },
   { d_agup_radio_proc,		42,		105,	68,		15,			2,		23,  0,    0,		0,   0,   "beat",		NULL, NULL },
   { d_agup_radio_proc,		120,	105,	68,		15,			2,		23,  0,    0,		0,   0,   "measure",	NULL, NULL },
   { d_agup_button_proc,	42,		125,	68,		28,			2,		23,  '\r', D_EXIT,	0,   0,   "OK",			NULL, NULL },
   { d_agup_button_proc,	120,	125,	68,		28,			2,		23,  0,    D_EXIT,	0,   0,   "Cancel",		NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

DIALOG eof_custom_speed_dialog[] =
{
   /* (proc)			(x)	(y)	(w)		(h)  	(fg)	(bg) (key) (flags)	(d1) (d2) (dp)		(dp2) 		(dp3) */
   { d_agup_shadow_box_proc,	32,	68,	170, 	72 + 8 +15,	2,	23,  0,    0,		0,   0,   NULL,		NULL, 		NULL },
   { d_agup_text_proc,		56,	84,	64,		8,	2,	23,  0,    0,		0,   0,   "Percent:",	NULL, 		NULL },
   { eof_verified_edit_proc,	112,	80,	66,		20,	2,	23,  0,    0,		8,   0,   eof_etext2,	"0123456789",	NULL },
   { d_agup_button_proc,	42,	125,	68,		28,	2,	23,  '\r', D_EXIT,	0,   0,   "OK",		NULL, 		NULL },
   { d_agup_button_proc,	120,	125,	68,		28,	2,	23,  0,    D_EXIT,	0,   0,   "Cancel",	NULL, 		NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

void eof_prepare_edit_menu(void)
{
	int i;
	int vselected = 0;
//	int cnotes = 0;	//This was never effectively used

	if(eof_song && eof_song_loaded)
	{
		/* undo */
		if(eof_undo_count > 0)
		{
			eof_edit_menu[0].flags = 0;
		}
		else
		{
			eof_edit_menu[0].flags = D_DISABLED;
		}

		/* redo */
		if(eof_redo_count)
		{
			eof_edit_menu[1].flags = 0;
		}
		else
		{
			eof_edit_menu[1].flags = D_DISABLED;
		}

		/* copy */
		vselected = eof_count_selected_notes(NULL, 1);
		if(vselected)
		{
			eof_edit_menu[3].flags = 0;		//copy
			eof_edit_selection_menu[1].flags = 0;	//select like

			/* select rest */
			if(eof_selection.current != (eof_vocals_selected ? eof_song->vocal_track[0]->lyrics -1 : eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->notes -1))
			{
				eof_edit_selection_menu[2].flags = 0;
			}
			else
			{
				eof_edit_selection_menu[2].flags = D_DISABLED;
			}

			/* deselect all */
			eof_edit_selection_menu[3].flags = 0;

			if(eof_selection.current != 0)
			{
				eof_edit_selection_menu[4].flags = 0;	//select previous
			}
			else
			{
				eof_edit_selection_menu[4].flags = D_DISABLED;	//Select previous cannot be used when the first note/lyric was just selected
			}
		}
		else
		{
			eof_edit_menu[3].flags = D_DISABLED;		//copy
			eof_edit_selection_menu[1].flags = D_DISABLED;	//select like
			eof_edit_selection_menu[2].flags = D_DISABLED;	//select rest
			eof_edit_selection_menu[3].flags = D_DISABLED;	//deselect all
			eof_edit_selection_menu[4].flags = D_DISABLED;	//select previous
		}

		/* paste, paste old */
		if(eof_vocals_selected)
		{
			if(exists("eof.vocals.clipboard"))
			{
				eof_edit_menu[4].flags = 0;
				eof_edit_menu[5].flags = 0;
			}
			else
			{
				eof_edit_menu[4].flags = D_DISABLED;
				eof_edit_menu[5].flags = D_DISABLED;
			}
		}
		else
		{
			if(exists("eof.clipboard"))
			{
				eof_edit_menu[4].flags = 0;
				eof_edit_menu[5].flags = 0;
			}
			else
			{
				eof_edit_menu[4].flags = D_DISABLED;
				eof_edit_menu[5].flags = D_DISABLED;
			}
		}

		/* select all, selection */
		if(eof_vocals_selected)
		{
			if(eof_song->vocal_track[0]->lyrics > 0)
			{
				eof_edit_selection_menu[0].flags = 0;
				eof_edit_menu[22].flags = 0;
			}
			else
			{
				eof_edit_selection_menu[0].flags = D_DISABLED;
				eof_edit_menu[22].flags = D_DISABLED;
			}
		}
		else
		{
			if(eof_note_type_name[eof_note_type][0] == '*')
			{
				eof_edit_selection_menu[0].flags = 0;
				eof_edit_menu[22].flags = 0;
			}
			else
			{
				eof_edit_selection_menu[0].flags = D_DISABLED;
				eof_edit_menu[22].flags = D_DISABLED;
			}
		}

		/* zoom */
		for(i = 0; i < 9; i++)
		{
			eof_edit_zoom_menu[i].flags = 0;
		}
		eof_edit_zoom_menu[10 - eof_zoom].flags = D_SELECTED;

		/* hopo */
		for(i = 0; i < 3; i++)
		{
			eof_edit_hopo_menu[i].flags = 0;
		}
		eof_edit_hopo_menu[(int)eof_hopo_view].flags = D_SELECTED;

		/* speed */
		for(i = 0; i < 3; i++)
		{
			eof_edit_speed_menu[i].flags = 0;
		}
		switch(eof_zoom_3d)
		{
			case 5:
			{
				eof_edit_speed_menu[0].flags = D_SELECTED;
				break;
			}
			case 3:
			{
				eof_edit_speed_menu[1].flags = D_SELECTED;
				break;
			}
			case 2:
			{
				eof_edit_speed_menu[2].flags = D_SELECTED;
				break;
			}
		}

		/* paste from difficulty */
		for(i = 0; i < 4; i++)	//For each of the four difficulties
		{
			if((eof_note_type_name[i][0] == '*') && (i != eof_note_type) && !eof_vocals_selected)	//If the difficulty is populated, isn't the active difficulty and PART VOCALS isn't active
			{
				eof_edit_paste_from_menu[i].flags = 0;		//Enable paste from the difficulty
			}
			else
			{
				eof_edit_paste_from_menu[i].flags = D_DISABLED;	//(Paste from difficulty isn't supposed to be usable in PART VOCALS)
			}
		}

		/* paste from catalog */
		if(eof_selected_catalog_entry < eof_song->catalog->entries)
		{
			if((eof_music_pos >= eof_song->catalog->entry[eof_selected_catalog_entry].start_pos) && (eof_music_pos <= eof_song->catalog->entry[eof_selected_catalog_entry].end_pos) && (eof_song->catalog->entry[eof_selected_catalog_entry].track == eof_selected_track) && (eof_song->catalog->entry[eof_selected_catalog_entry].type == eof_note_type))
			{
				eof_edit_paste_from_menu[5].flags = D_DISABLED;
			}
			else if((eof_song->catalog->entry[eof_selected_catalog_entry].track == EOF_TRACK_VOCALS) && !eof_vocals_selected)
			{
				eof_edit_paste_from_menu[5].flags = D_DISABLED;
			}
			else if((eof_song->catalog->entry[eof_selected_catalog_entry].track != EOF_TRACK_VOCALS) && eof_vocals_selected)
			{
				eof_edit_paste_from_menu[5].flags = D_DISABLED;
			}
/*cnotes was never set to anything besides 0, so this can be removed
			else if(cnotes > 0)
			{
				eof_edit_paste_from_menu[5].flags = D_DISABLED;
			}
*/
			else
			{
				eof_edit_paste_from_menu[5].flags = 0;
			}
		}
		else
		{
			eof_edit_paste_from_menu[5].flags = D_DISABLED;
		}

		/* paste from */
		eof_edit_menu[6].flags = D_DISABLED;
		for(i = 0; i < 6; i++)
		{
			if((i != 4) && (eof_edit_paste_from_menu[i].flags == 0))
			{
				eof_edit_menu[6].flags = 0;
			}
		}

		/* selection */
		for(i = 0; i < 12; i++)
		{
			eof_edit_snap_menu[i].flags = 0;
		}
		switch(eof_snap_mode)
		{
			case EOF_SNAP_QUARTER:
			{
				eof_edit_snap_menu[0].flags = D_SELECTED;
				break;
			}
			case EOF_SNAP_EIGHTH:
			{
				eof_edit_snap_menu[1].flags = D_SELECTED;
				break;
			}
			case EOF_SNAP_SIXTEENTH:
			{
				eof_edit_snap_menu[2].flags = D_SELECTED;
				break;
			}
			case EOF_SNAP_THIRTY_SECOND:
			{
				eof_edit_snap_menu[3].flags = D_SELECTED;
				break;
			}
			case EOF_SNAP_TWELFTH:
			{
				eof_edit_snap_menu[5].flags = D_SELECTED;
				break;
			}
			case EOF_SNAP_TWENTY_FOURTH:
			{
				eof_edit_snap_menu[6].flags = D_SELECTED;
				break;
			}
			case EOF_SNAP_FORTY_EIGHTH:
			{
				eof_edit_snap_menu[7].flags = D_SELECTED;
				break;
			}
			case EOF_SNAP_CUSTOM:
			{
				eof_edit_snap_menu[9].flags = D_SELECTED;
				break;
			}
			case EOF_SNAP_OFF:
			{
				eof_edit_snap_menu[11].flags = D_SELECTED;
				break;
			}
		}

		/* MIDI tones */
		if(!eof_midi_initialized)
			eof_edit_menu[18].flags = D_DISABLED;
	}
}

int eof_menu_edit_undo(void)
{
	eof_undo_apply();
	eof_redo_toggle = 1;
	if((eof_catalog_menu[0].flags & D_SELECTED) && (eof_song->catalog->entries > 0))
	{
		eof_music_catalog_pos = eof_song->catalog->entry[eof_selected_catalog_entry].start_pos + eof_av_delay;
	}
	return 1;
}

int eof_menu_edit_redo(void)
{
	eof_redo_apply();
	eof_redo_toggle = 0;
	if((eof_catalog_menu[0].flags & D_SELECTED) && (eof_song->catalog->entries > 0))
	{
		eof_music_catalog_pos = eof_song->catalog->entry[eof_selected_catalog_entry].start_pos + eof_av_delay;
	}
	return 1;
}

int eof_menu_edit_cut_vocal(int anchor, int option)
{
	int i;
	int first_pos = -1;
	int first_beat = -1;
	int start_pos, end_pos;
	int last_anchor, next_anchor;
	int copy_notes = 0;
	float tfloat;
	PACKFILE * fp;

	/* set boundary */
	eof_anchor_diff[EOF_TRACK_VOCALS] = 0;
	last_anchor = eof_find_previous_anchor(eof_song, anchor);
	next_anchor = eof_find_next_anchor(eof_song, anchor);
	start_pos = eof_song->beat[last_anchor]->pos;
	if(next_anchor < 0)
	{
		end_pos = eof_song->beat[eof_song->beats - 1]->pos - 1;
	}
	else
	{
		end_pos = eof_song->beat[next_anchor]->pos;
	}
	if(option == 1)
	{
		end_pos = eof_song->beat[eof_song->beats - 1]->pos - 1;
	}

	for(i = 0; i < eof_song->vocal_track[0]->lyrics; i++)
	{
		if((eof_song->vocal_track[0]->lyric[i]->pos >= start_pos) && (eof_song->vocal_track[0]->lyric[i]->pos < end_pos))
		{
			copy_notes++;
			if(eof_song->vocal_track[0]->lyric[i]->pos < first_pos)
			{
				first_pos = eof_song->vocal_track[0]->lyric[i]->pos;
				eof_anchor_diff[EOF_TRACK_VOCALS] = eof_get_beat(eof_song, eof_song->vocal_track[0]->lyric[i]->pos) - last_anchor;
			}
			if(first_beat == -1)
			{
				first_beat = eof_get_beat(eof_song, eof_song->vocal_track[0]->lyric[i]->pos);
			}
		}
	}

	/* get ready to write clipboard to disk */
	fp = pack_fopen("eof.autoadjust.vocals", "w");
	if(!fp)
	{
		allegro_message("Clipboard error!");
		return 1;
	}

	pack_iputl(copy_notes, fp);
	pack_iputl(first_beat, fp);
	for(i = 0; i < eof_song->vocal_track[0]->lyrics; i++)
	{
		if((eof_song->vocal_track[0]->lyric[i]->pos >= start_pos) && (eof_song->vocal_track[0]->lyric[i]->pos < end_pos))
		{
			pack_putc(eof_song->vocal_track[0]->lyric[i]->note, fp);
			pack_iputl(eof_song->vocal_track[0]->lyric[i]->pos - first_pos, fp);
			pack_iputl(eof_get_beat(eof_song, eof_song->vocal_track[0]->lyric[i]->pos), fp);
			pack_iputl(eof_get_beat(eof_song, eof_song->vocal_track[0]->lyric[i]->pos + eof_song->vocal_track[0]->lyric[i]->length), fp);
			tfloat = eof_get_porpos(eof_song->vocal_track[0]->lyric[i]->pos);
			pack_fwrite(&tfloat, sizeof(float), fp);
			tfloat = eof_get_porpos(eof_song->vocal_track[0]->lyric[i]->pos + eof_song->vocal_track[0]->lyric[i]->length);
			pack_fwrite(&tfloat, sizeof(float), fp);
			pack_iputw(ustrlen(eof_song->vocal_track[0]->lyric[i]->text), fp);
			pack_fwrite(eof_song->vocal_track[0]->lyric[i]->text, ustrlen(eof_song->vocal_track[0]->lyric[i]->text), fp);
		}
	}
	pack_fclose(fp);
	return 1;
}

int eof_menu_edit_cut_paste_vocal(int anchor, int option)
{
	int i, t;
	int first_beat = 0;
	int this_beat = 0;
	int start_pos, end_pos;
	int last_anchor, next_anchor;
	PACKFILE * fp;
	int copy_notes = 0;
	EOF_EXTENDED_LYRIC temp_lyric;
	EOF_LYRIC * new_lyric = NULL;

	this_beat = eof_find_previous_anchor(eof_song, anchor) + eof_anchor_diff[EOF_TRACK_VOCALS];

	/* set boundary */
	last_anchor = eof_find_previous_anchor(eof_song, anchor);
	next_anchor = eof_find_next_anchor(eof_song, anchor);
	start_pos = eof_song->beat[last_anchor]->pos;
	if(next_anchor < 0)
	{
		end_pos = eof_song->beat[eof_song->beats - 1]->pos - 1;
	}
	else
	{
		end_pos = eof_song->beat[next_anchor]->pos;
	}
	if(option == 1)
	{
		end_pos = eof_song->beat[eof_song->beats - 1]->pos - 1;
	}

	fp = pack_fopen("eof.autoadjust.vocals", "r");
	if(!fp)
	{
		allegro_message("Clipboard error!");
		return 1;
	}
	for(i = eof_song->vocal_track[0]->lyrics - 1; i >= 0; i--)
	{
		if((eof_song->vocal_track[0]->lyric[i]->pos >= start_pos) && (eof_song->vocal_track[0]->lyric[i]->pos < end_pos))
		{
			eof_vocal_track_delete_lyric(eof_song->vocal_track[0], i);
		}
	}

	memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
	copy_notes = pack_igetl(fp);
	first_beat = pack_igetl(fp);

	for(i = 0; i < copy_notes; i++)
	{

		/* read the note */
		temp_lyric.note = pack_getc(fp);
		temp_lyric.pos = pack_igetl(fp);
		temp_lyric.beat = pack_igetl(fp);
		temp_lyric.endbeat = pack_igetl(fp);
		pack_fread(&temp_lyric.porpos, sizeof(float), fp);
		pack_fread(&temp_lyric.porendpos, sizeof(float), fp);
		t = pack_igetw(fp);
		pack_fread(temp_lyric.text, t, fp);
		temp_lyric.text[t] = '\0';

		if(temp_lyric.pos < eof_music_length)
		{
			new_lyric = eof_vocal_track_add_lyric(eof_song->vocal_track[0]);
			if(new_lyric)
			{
				new_lyric->note = temp_lyric.note;
				new_lyric->pos = eof_put_porpos(temp_lyric.beat - first_beat + this_beat, temp_lyric.porpos, 0.0);
				new_lyric->length = eof_put_porpos(temp_lyric.endbeat - first_beat + this_beat, temp_lyric.porendpos, 0.0) - new_lyric->pos;
				ustrcpy(new_lyric->text, temp_lyric.text);
			}
		}
	}
	eof_vocal_track_sort_lyrics(eof_song->vocal_track[0]);
	pack_fclose(fp);
	eof_fixup_notes();
	eof_determine_hopos();
	return 1;
}

int eof_menu_edit_copy_vocal(void)
{
	int i;
	int first_pos = -1;
	int first_beat = -1;
	char note_check = 0;
	int copy_notes = 0;
	float tfloat;
	PACKFILE * fp;

	/* first, scan for selected notes */
	for(i = 0; i < eof_song->vocal_track[0]->lyrics; i++)
	{
		if((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i])
		{
			copy_notes++;
			if(eof_song->vocal_track[0]->lyric[i]->pos < first_pos)
			{
				first_pos = eof_song->vocal_track[0]->lyric[i]->pos;
			}
			if(first_beat == -1)
			{
				first_beat = eof_get_beat(eof_song, eof_song->vocal_track[0]->lyric[i]->pos);
			}
		}
	}
	if(copy_notes <= 0)
	{
		return 1;
	}

	/* get ready to write clipboard to disk */
	fp = pack_fopen("eof.vocals.clipboard", "w");
	if(!fp)
	{
		allegro_message("Clipboard error!");
		return 1;
	}
	pack_iputl(copy_notes, fp);
	pack_iputl(first_beat, fp);

	for(i = 0; i < eof_song->vocal_track[0]->lyrics; i++)
	{
		if((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i])
		{

			/* check for accidentally moved note */
			if(!note_check)
			{
				if(eof_song->beat[eof_get_beat(eof_song, eof_song->vocal_track[0]->lyric[i]->pos) + 1]->pos - eof_song->vocal_track[0]->lyric[i]->pos <= 10)
				{
					if(alert(NULL, "First note appears to be off.", "Adjust?", "&Yes", "&No", 'y', 'n') == 1)
					{
						eof_song->vocal_track[0]->lyric[i]->pos = eof_song->beat[eof_get_beat(eof_song, eof_song->vocal_track[0]->lyric[i]->pos) + 1]->pos;
					}
					eof_clear_input();
				}
				note_check = 1;
			}

			/* write note data to disk */
			pack_putc(eof_song->vocal_track[0]->lyric[i]->note, fp);
			pack_iputl(eof_song->vocal_track[0]->lyric[i]->pos - first_pos, fp);
			pack_iputl(eof_get_beat(eof_song, eof_song->vocal_track[0]->lyric[i]->pos), fp);
			pack_iputl(eof_get_beat(eof_song, eof_song->vocal_track[0]->lyric[i]->pos + eof_song->vocal_track[0]->lyric[i]->length), fp);
			pack_iputl(eof_song->vocal_track[0]->lyric[i]->length, fp);
			tfloat = eof_get_porpos(eof_song->vocal_track[0]->lyric[i]->pos);
			pack_fwrite(&tfloat, sizeof(float), fp);
			tfloat = eof_get_porpos(eof_song->vocal_track[0]->lyric[i]->pos + eof_song->vocal_track[0]->lyric[i]->length);
			pack_fwrite(&tfloat, sizeof(float), fp);
			pack_iputw(ustrlen(eof_song->vocal_track[0]->lyric[i]->text), fp);
			pack_fwrite(eof_song->vocal_track[0]->lyric[i]->text, ustrlen(eof_song->vocal_track[0]->lyric[i]->text), fp);
		}
	}
	pack_fclose(fp);
	return 1;
}

/* delete lyrics which overlap the specified range */
static void eof_menu_edit_paste_clear_range_vocal(unsigned long start, unsigned long end)
{
	int i;

	for(i = eof_song->vocal_track[0]->lyrics - 1; i >= 0; i--)
	{
		if((eof_song->vocal_track[0]->lyric[i]->pos >= start) && (eof_song->vocal_track[0]->lyric[i]->pos <= end))
		{
			eof_vocal_track_delete_lyric(eof_song->vocal_track[0], i);
		}
		else if((eof_song->vocal_track[0]->lyric[i]->pos + eof_song->vocal_track[0]->lyric[i]->length >= start) && (eof_song->vocal_track[0]->lyric[i]->pos + eof_song->vocal_track[0]->lyric[i]->length <= end))
		{
			eof_vocal_track_delete_lyric(eof_song->vocal_track[0], i);
		}
	}
}

int eof_menu_edit_paste_vocal(void)
{
	int i, j, t;
	unsigned long paste_pos[EOF_MAX_NOTES] = {0};
	int paste_count = 0;
	int first_beat = 0;
	int this_beat = eof_get_beat(eof_song, eof_music_pos - eof_av_delay);
	int copy_notes;
	int new_pos = -1;
	int new_end_pos = -1;
	int last_pos = -1;
	EOF_EXTENDED_LYRIC temp_lyric;
	EOF_LYRIC * new_lyric = NULL;
	PACKFILE * fp;


	/* open the file */
	fp = pack_fopen("eof.vocals.clipboard", "r");
	if(!fp)
	{
		allegro_message("Clipboard error!\nNothing to paste!");
		return 1;
	}
	if(first_beat + this_beat >= eof_song->beats - 1)
	{
		return 1;
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NOTE_SEL);
	copy_notes = pack_igetl(fp);
	first_beat = pack_igetl(fp);

	memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
	eof_selection.current = EOF_MAX_NOTES - 1;
	eof_selection.current_pos = 0;
	for(i = 0; i < copy_notes; i++)
	{

		/* read the note */
		temp_lyric.note = pack_getc(fp);
		temp_lyric.pos = pack_igetl(fp);
		temp_lyric.beat = pack_igetl(fp);
		if(temp_lyric.beat - first_beat + this_beat >= eof_song->beats - 1)
		{
			break;
		}
		temp_lyric.endbeat = pack_igetl(fp);
		temp_lyric.length = pack_igetl(fp);
		pack_fread(&temp_lyric.porpos, sizeof(float), fp);
		pack_fread(&temp_lyric.porendpos, sizeof(float), fp);
		t = pack_igetw(fp);
		pack_fread(temp_lyric.text, t, fp);
		temp_lyric.text[t] = '\0';

		if(eof_music_pos + temp_lyric.pos - eof_av_delay < eof_music_length)
		{
			if(last_pos >= 0)
			{
				last_pos = new_end_pos + 1;
			}
			new_pos = eof_put_porpos(temp_lyric.beat - first_beat + this_beat, temp_lyric.porpos, 0.0);
			new_end_pos = eof_put_porpos(temp_lyric.endbeat - first_beat + this_beat, temp_lyric.porendpos, 0.0);
			if(last_pos < 0)
			{
				last_pos = new_pos;
			}
			eof_menu_edit_paste_clear_range_vocal(last_pos, new_end_pos);
			new_lyric = eof_vocal_track_add_lyric(eof_song->vocal_track[0]);
			if(new_lyric)
			{
				new_lyric->note = temp_lyric.note;
				new_lyric->pos = new_pos;
				new_lyric->length = new_end_pos - new_lyric->pos;
				ustrcpy(new_lyric->text, temp_lyric.text);
				paste_pos[paste_count] = new_lyric->pos;
				paste_count++;
			}
		}
	}
	pack_fclose(fp);
	eof_vocal_track_sort_lyrics(eof_song->vocal_track[0]);
	eof_vocal_track_fixup_lyrics(eof_song->vocal_track[0], 0);
	if((paste_count > 0) && (eof_selection.track != EOF_TRACK_VOCALS))
	{
		eof_selection.track = EOF_TRACK_VOCALS;
		memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
	}
	for(i = 0; i < paste_count; i++)
	{
		for(j = 0; j < eof_song->vocal_track[0]->lyrics; j++)
		{
			if(eof_song->vocal_track[0]->lyric[j]->pos == paste_pos[i])
			{
				eof_selection.multi[j] = 1;
				break;
			}
		}
	}
	return 1;
}

int eof_menu_edit_old_paste_vocal(void)
{
	int i, j, t;
	unsigned long paste_pos[EOF_MAX_NOTES] = {0};
	int paste_count = 0;
	int copy_notes;
	int first_beat;
	PACKFILE * fp;
	int new_pos = -1;
	int new_end_pos = -1;
	int last_pos = -1;
	EOF_EXTENDED_LYRIC temp_lyric;
	EOF_LYRIC * new_lyric = NULL;

	fp = pack_fopen("eof.vocals.clipboard", "r");
	if(!fp)
	{
		allegro_message("Clipboard error!\nNothing to paste!");
		return 1;
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NOTE_SEL);
	memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
	copy_notes = pack_igetl(fp);
	first_beat = pack_igetl(fp);
	for(i = 0; i < copy_notes; i++)
	{
		/* read the note */
		temp_lyric.note = pack_getc(fp);
		temp_lyric.pos = pack_igetl(fp);
		temp_lyric.beat = pack_igetl(fp);
		temp_lyric.endbeat = pack_igetl(fp);
		temp_lyric.length = pack_igetl(fp);
		pack_fread(&temp_lyric.porpos, sizeof(float), fp);
		pack_fread(&temp_lyric.porendpos, sizeof(float), fp);
		t = pack_igetw(fp);
		pack_fread(temp_lyric.text, t, fp);
		temp_lyric.text[t] = '\0';

		if(eof_music_pos + temp_lyric.pos - eof_av_delay < eof_music_length)
		{
			if(last_pos >= 0)
			{
				last_pos = new_end_pos + 1;
			}
			new_pos = eof_music_pos + temp_lyric.pos - eof_av_delay;
			new_end_pos = new_pos + temp_lyric.length;
			if(last_pos < 0)
			{
				last_pos = new_pos;
			}
			eof_menu_edit_paste_clear_range_vocal(last_pos, new_end_pos);
			new_lyric = eof_vocal_track_add_lyric(eof_song->vocal_track[0]);
			if(new_lyric)
			{
				new_lyric->note = temp_lyric.note;
				new_lyric->pos = new_pos;
				new_lyric->length = temp_lyric.length;
				ustrcpy(new_lyric->text, temp_lyric.text);
				paste_pos[paste_count] = new_lyric->pos;
				paste_count++;
			}
		}
	}
	eof_vocal_track_sort_lyrics(eof_song->vocal_track[0]);
	eof_vocal_track_fixup_lyrics(eof_song->vocal_track[0], 0);
	if((paste_count > 0) && (eof_selection.track != EOF_TRACK_VOCALS))
	{
		eof_selection.track = EOF_TRACK_VOCALS;
		memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
	}
	for(i = 0; i < paste_count; i++)
	{
		for(j = 0; j < eof_song->vocal_track[0]->lyrics; j++)
		{
			if(eof_song->vocal_track[0]->lyric[j]->pos == paste_pos[i])
			{
				eof_selection.multi[j] = 1;
				break;
			}
		}
	}
	return 1;
}

int eof_menu_edit_cut(int anchor, int option, float offset)
{
	int i, j;
	int first_pos[EOF_TRACKS_MAX] = {-1, -1, -1, -1, -1};
	int first_beat[EOF_TRACKS_MAX] = {-1, -1, -1, -1, -1};
	int start_pos, end_pos;
	int last_anchor, next_anchor;
	int copy_notes[EOF_TRACKS_MAX] = {0};
	float tfloat;
	PACKFILE * fp;

	/* set boundary */
	for(i = 0; i < EOF_TRACKS_MAX; i++)
	{
		eof_anchor_diff[i] = 0;
	}
	last_anchor = eof_find_previous_anchor(eof_song, anchor);
	next_anchor = eof_find_next_anchor(eof_song, anchor);
	start_pos = eof_song->beat[last_anchor]->pos;
	if(next_anchor < 0)
	{
		end_pos = eof_song->beat[eof_song->beats - 1]->pos - 1;
	}
	else
	{
		end_pos = eof_song->beat[next_anchor]->pos;
	}
	if(option == 1)
	{
		end_pos = eof_song->beat[eof_song->beats - 1]->pos - 1;
	}

	for(j = 0; j < EOF_LEGACY_TRACKS_MAX; j++)
	{
		for(i = 0; i < eof_song->legacy_track[j]->notes; i++)
		{
			if((eof_song->legacy_track[j]->note[i]->pos + eof_song->legacy_track[j]->note[i]->length >= start_pos) && (eof_song->legacy_track[j]->note[i]->pos < end_pos))
			{
				copy_notes[j]++;
				if(eof_song->legacy_track[j]->note[i]->pos < first_pos[j])
				{
					first_pos[j] = eof_song->legacy_track[j]->note[i]->pos;
					eof_anchor_diff[j] = eof_get_beat(eof_song, eof_song->legacy_track[j]->note[i]->pos) - last_anchor;
				}
				if(first_beat[j] == -1)
				{
					first_beat[j] = eof_get_beat(eof_song, eof_song->legacy_track[j]->note[i]->pos);
				}
			}
		}
	}

	/* get ready to write clipboard to disk */
	fp = pack_fopen("eof.autoadjust", "w");
	if(!fp)
	{
		allegro_message("Clipboard error!");
		return 1;
	}

	/* copy all tracks */
	for(j = 0; j < EOF_LEGACY_TRACKS_MAX; j++)
	{
		/* notes */
		pack_iputl(copy_notes[j], fp);
		pack_iputl(first_beat[j], fp);
		for(i = 0; i < eof_song->legacy_track[j]->notes; i++)
		{
			if((eof_song->legacy_track[j]->note[i]->pos + eof_song->legacy_track[j]->note[i]->length >= start_pos) && (eof_song->legacy_track[j]->note[i]->pos < end_pos))
			{
				pack_iputl(eof_song->legacy_track[j]->note[i]->type, fp);
				pack_iputl(eof_song->legacy_track[j]->note[i]->note, fp);
				pack_iputl(eof_song->legacy_track[j]->note[i]->pos - first_pos[j], fp);
				tfloat = eof_get_porpos(eof_song->legacy_track[j]->note[i]->pos);
				pack_fwrite(&tfloat, sizeof(float), fp);
				tfloat = eof_get_porpos(eof_song->legacy_track[j]->note[i]->pos + eof_song->legacy_track[j]->note[i]->length);
				pack_fwrite(&tfloat, sizeof(float), fp);
				pack_iputl(eof_get_beat(eof_song, eof_song->legacy_track[j]->note[i]->pos), fp);
				pack_iputl(eof_get_beat(eof_song, eof_song->legacy_track[j]->note[i]->pos + eof_song->legacy_track[j]->note[i]->length), fp);
				pack_iputl(eof_song->legacy_track[j]->note[i]->length, fp);
				pack_iputl(eof_song->legacy_track[j]->note[i]->flags, fp);	//Write the note flags
			}
		}
		/* star power */
		for(i = 0; i < eof_song->legacy_track[j]->star_power_paths; i++)
		{
			/* which beat */
			pack_iputl(eof_get_beat(eof_song, eof_song->legacy_track[j]->star_power_path[i].start_pos), fp);
			tfloat = eof_get_porpos(eof_song->legacy_track[j]->star_power_path[i].start_pos);
			pack_fwrite(&tfloat, sizeof(float), fp);
			pack_iputl(eof_get_beat(eof_song, eof_song->legacy_track[j]->star_power_path[i].end_pos), fp);
			tfloat = eof_get_porpos(eof_song->legacy_track[j]->star_power_path[i].end_pos);
			pack_fwrite(&tfloat, sizeof(float), fp);
		}

		/* solos */
		for(i = 0; i < eof_song->legacy_track[j]->solos; i++)
		{
			/* which beat */
			pack_iputl(eof_get_beat(eof_song, eof_song->legacy_track[j]->solo[i].start_pos), fp);
			tfloat = eof_get_porpos(eof_song->legacy_track[j]->solo[i].start_pos);
			pack_fwrite(&tfloat, sizeof(float), fp);
			pack_iputl(eof_get_beat(eof_song, eof_song->legacy_track[j]->solo[i].end_pos), fp);
			tfloat = eof_get_porpos(eof_song->legacy_track[j]->solo[i].end_pos);
			pack_fwrite(&tfloat, sizeof(float), fp);
		}
	}
	pack_fclose(fp);
	return eof_menu_edit_cut_vocal(anchor, option);
}

int eof_menu_edit_cut_paste(int anchor, int option, float offset)
{
	int i, j, b;
//	unsigned long paste_pos[EOF_MAX_NOTES] = {0};
//	int paste_count = 0;
	int first_beat[EOF_TRACKS_MAX] = {0};
	int this_beat[EOF_TRACKS_MAX] = {0};
	int start_pos, end_pos;
	int last_anchor, next_anchor;
	PACKFILE * fp;
	int copy_notes[EOF_TRACKS_MAX];
	EOF_EXTENDED_NOTE temp_note;
	EOF_NOTE * new_note = NULL;
	float tfloat;

	for(i = 0; i < EOF_TRACKS_MAX; i++)
	{
		this_beat[i] = eof_find_previous_anchor(eof_song, anchor) + eof_anchor_diff[i];
	}

	/* set boundary */
	last_anchor = eof_find_previous_anchor(eof_song, anchor);
	next_anchor = eof_find_next_anchor(eof_song, anchor);
	start_pos = eof_song->beat[last_anchor]->pos;
	if(next_anchor < 0)
	{
		end_pos = eof_song->beat[eof_song->beats - 1]->pos - 1;
	}
	else
	{
		end_pos = eof_song->beat[next_anchor]->pos;
	}
	if(option == 1)
	{
		end_pos = eof_song->beat[eof_song->beats - 1]->pos - 1;
	}

	fp = pack_fopen("eof.autoadjust", "r");
	if(!fp)
	{
		allegro_message("Clipboard error!");
		return 1;
	}
	for(j = 0; j < EOF_LEGACY_TRACKS_MAX; j++)
	{
		for(i = eof_song->legacy_track[j]->notes - 1; i >= 0; i--)
		{
			if((eof_song->legacy_track[j]->note[i]->pos + eof_song->legacy_track[j]->note[i]->length >= start_pos) && (eof_song->legacy_track[j]->note[i]->pos < end_pos))
			{
				eof_track_delete_note(eof_song->legacy_track[j], i);
			}
		}
	}

	memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
	for(j = 0; j < EOF_LEGACY_TRACKS_MAX; j++)
	{
		copy_notes[j] = pack_igetl(fp);
		first_beat[j] = pack_igetl(fp);

		for(i = 0; i < copy_notes[j]; i++)
		{

			/* read the note */
			temp_note.type = pack_igetl(fp);
			temp_note.note = pack_igetl(fp);
			temp_note.pos = pack_igetl(fp);
			pack_fread(&temp_note.porpos, sizeof(float), fp);
			pack_fread(&temp_note.porendpos, sizeof(float), fp);
			temp_note.beat = pack_igetl(fp);
			temp_note.endbeat = pack_igetl(fp);
			temp_note.length = pack_igetl(fp);
			temp_note.flags = pack_igetl(fp);	//Store the note flags

			if(temp_note.pos + temp_note.length < eof_music_length)
			{
				new_note = eof_track_add_note(eof_song->legacy_track[j]);
				if(new_note)
				{
					new_note->type = temp_note.type;
					new_note->note = temp_note.note;
					new_note->pos = eof_put_porpos(temp_note.beat - first_beat[j] + this_beat[j], temp_note.porpos, 0.0);
					new_note->length = eof_put_porpos(temp_note.endbeat - first_beat[j] + this_beat[j], temp_note.porendpos, 0.0) - new_note->pos;
					new_note->flags = temp_note.flags;
				}
			}
		}
		eof_track_sort_notes(eof_song->legacy_track[j]);

		/* star power */
		for(i = 0; i < eof_song->legacy_track[j]->star_power_paths; i++)
		{
			/* which beat */
			b = pack_igetl(fp);
			pack_fread(&tfloat, sizeof(float), fp);
			eof_song->legacy_track[j]->star_power_path[i].start_pos = eof_put_porpos(b, tfloat, 0.0);
			b = pack_igetl(fp);
			pack_fread(&tfloat, sizeof(float), fp);
			eof_song->legacy_track[j]->star_power_path[i].end_pos = eof_put_porpos(b, tfloat, 0.0);
		}

		/* solos */
		for(i = 0; i < eof_song->legacy_track[j]->solos; i++)
		{
			/* which beat */
			b = pack_igetl(fp);
			pack_fread(&tfloat, sizeof(float), fp);
			eof_song->legacy_track[j]->solo[i].start_pos = eof_put_porpos(b, tfloat, 0.0);
			b = pack_igetl(fp);
			pack_fread(&tfloat, sizeof(float), fp);
			eof_song->legacy_track[j]->solo[i].end_pos = eof_put_porpos(b, tfloat, 0.0);
		}
	}
	pack_fclose(fp);
	eof_fixup_notes();
	eof_determine_hopos();
	return eof_menu_edit_cut_paste_vocal(anchor, option);
}

int eof_menu_edit_copy(void)
{
	if(eof_vocals_selected)
	{
		return eof_menu_edit_copy_vocal();
	}
	int i;
	int first_pos = -1;
	int first_beat = -1;
	char note_check = 0;
	int copy_notes = 0;
	float tfloat;
	PACKFILE * fp;

	/* first, scan for selected notes */
	for(i = 0; i < eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->notes; i++)
	{
		if((eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[i]->type == eof_note_type) && (eof_selection.track == eof_selected_track && eof_selection.multi[i]))
		{
			copy_notes++;
			if(eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[i]->pos < first_pos)
			{
				first_pos = eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[i]->pos;
			}
			if(first_beat == -1)
			{
				first_beat = eof_get_beat(eof_song, eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[i]->pos);
			}
		}
	}
	if(copy_notes <= 0)
	{
		return 1;
	}

	/* get ready to write clipboard to disk */
	fp = pack_fopen("eof.clipboard", "w");
	if(!fp)
	{
		allegro_message("Clipboard error!");
		return 1;
	}
	pack_iputl(eof_selected_track, fp);	//Store the source track number
	pack_iputl(copy_notes, fp);
	pack_iputl(first_beat, fp);

	for(i = 0; i < eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->notes; i++)
	{
		if((eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[i]->type == eof_note_type) && (eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{

			/* check for accidentally moved note */
			if(!note_check)
			{
				if(eof_song->beat[eof_get_beat(eof_song, eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[i]->pos) + 1]->pos - eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[i]->pos <= 10)
				{
					if(alert(NULL, "First note appears to be off.", "Adjust?", "&Yes", "&No", 'y', 'n') == 1)
					{
						eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[i]->pos = eof_song->beat[eof_get_beat(eof_song, eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[i]->pos) + 1]->pos;
					}
					eof_clear_input();
				}
				note_check = 1;
			}

			/* write note data to disk */
			pack_iputl(eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[i]->note, fp);				//Write the note fret values
			pack_iputl(eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[i]->pos - first_pos, fp);	//Write the note's position relative to within the selection
			tfloat = eof_get_porpos(eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[i]->pos);
			pack_fwrite(&tfloat, sizeof(float), fp);	//Write the percent representing the note's start position within a beat
			tfloat = eof_get_porpos(eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[i]->pos + eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[i]->length);
			pack_fwrite(&tfloat, sizeof(float), fp);	//Write the percent representing the note's end position within a beat
			pack_iputl(eof_get_beat(eof_song, eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[i]->pos), fp);	//Write the beat the note starts in
			pack_iputl(eof_get_beat(eof_song, eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[i]->pos + eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[i]->length), fp);	//Write the beat the note ends in
			pack_iputl(eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[i]->length, fp);	//Write the note's length
			pack_iputl(eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[i]->flags, fp);	//Write the note's flags
		}
	}
	pack_fclose(fp);
	return 1;
}

int eof_menu_edit_paste(void)
{
	if(eof_vocals_selected)
	{
		return eof_menu_edit_paste_vocal();
	}
	int i, j;
	unsigned long paste_pos[EOF_MAX_NOTES] = {0};
	int paste_count = 0;
	int first_beat = 0;
	int this_beat = eof_get_beat(eof_song, eof_music_pos - eof_av_delay);
	int copy_notes;
	EOF_EXTENDED_NOTE temp_note;
	EOF_NOTE * new_note = NULL;
	PACKFILE * fp;
	int sourcetrack = 0;	//Will store the track that this clipboard data was from


	/* open the file */
	fp = pack_fopen("eof.clipboard", "r");
	if(!fp)
	{
		allegro_message("Clipboard error!\nNothing to paste!");
		return 1;
	}
	if(first_beat + this_beat >= eof_song->beats - 1)
	{
		return 1;
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NOTE_SEL);
	sourcetrack = pack_igetl(fp);	//Read the source track of the clipboard data
	copy_notes = pack_igetl(fp);
	first_beat = pack_igetl(fp);

	memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
	eof_selection.current = EOF_MAX_NOTES - 1;
	eof_selection.current_pos = 0;
	for(i = 0; i < copy_notes; i++)
	{

		/* read the note */
		temp_note.note = pack_igetl(fp);	//Read the note fret values
		temp_note.pos = pack_igetl(fp);		//Read the note's position relative to within the selection
		pack_fread(&temp_note.porpos, sizeof(float), fp);	//Read the percent representing the note's start position within a beat
		pack_fread(&temp_note.porendpos, sizeof(float), fp);	//Read the percent representing the note's end position within a beat
		temp_note.beat = pack_igetl(fp);	//Read the beat the note starts in
		temp_note.endbeat = pack_igetl(fp);	//Read the beat the note ends in
		if((temp_note.beat - first_beat + this_beat >= eof_song->beats - 1) || (temp_note.endbeat - first_beat + this_beat >= eof_song->beats - 1))
		{
			break;
		}
		temp_note.length = pack_igetl(fp);	//Read the note's length
		temp_note.flags = pack_igetl(fp);	//Read the note's flags
		eof_sanitize_note_flags(&temp_note.flags,eof_selected_track,sourcetrack);	//Ensure the note flags are validated for the track being pasted into

		if(eof_music_pos + temp_note.pos + temp_note.length - eof_av_delay < eof_music_length)
		{
			new_note = eof_track_add_note(eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]);
			if(new_note)
			{
				new_note->type = eof_note_type;
				new_note->note = temp_note.note;
				new_note->pos = eof_put_porpos(temp_note.beat - first_beat + this_beat, temp_note.porpos, 0.0);
				new_note->length = eof_put_porpos(temp_note.endbeat - first_beat + this_beat, temp_note.porendpos, 0.0) - new_note->pos;
				new_note->flags = temp_note.flags;
				paste_pos[paste_count] = new_note->pos;
				paste_count++;
			}
		}
	}
	pack_fclose(fp);
	eof_track_sort_notes(eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]);
	eof_fixup_notes();
	eof_determine_hopos();
	eof_detect_difficulties(eof_song);
	if((paste_count > 0) && (eof_selection.track != eof_selected_track))
	{
		eof_selection.track = eof_selected_track;
		memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
	}
	for(i = 0; i < paste_count; i++)
	{
		for(j = 0; j < eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->notes; j++)
		{
			if((eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[j]->type == eof_note_type) && (eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[j]->pos == paste_pos[i]))
			{
				eof_selection.multi[j] = 1;
				break;
			}
		}
	}
	return 1;
}

int eof_menu_edit_old_paste(void)
{
	if(eof_vocals_selected)
	{
		return eof_menu_edit_old_paste_vocal();
	}
	int i, j;
	unsigned long paste_pos[EOF_MAX_NOTES] = {0};
	int paste_count = 0;
	int copy_notes;
	int first_beat;
	PACKFILE * fp;
	EOF_EXTENDED_NOTE temp_note;
	EOF_NOTE * new_note = NULL;
	int sourcetrack = 0;	//Will store the track that this clipboard data was from

	fp = pack_fopen("eof.clipboard", "r");
	if(!fp)
	{
		allegro_message("Clipboard error!\nNothing to paste!");
		return 1;
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NOTE_SEL);
	memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
	sourcetrack = pack_igetl(fp);	//Read the source track of the clipboard data
	copy_notes = pack_igetl(fp);
	first_beat = pack_igetl(fp);
	for(i = 0; i < copy_notes; i++)
	{
		/* read the note */
		temp_note.note = pack_igetl(fp);	//Read the note fret values
		temp_note.pos = pack_igetl(fp);		//Read the note's position relative to within the selection
		pack_fread(&temp_note.porpos, sizeof(float), fp);	//Read the percent representing the note's start position within a beat
		pack_fread(&temp_note.porendpos, sizeof(float), fp);	//Read the percent representing the note's end position within a beat
		temp_note.beat = pack_igetl(fp);	//Read the beat the note starts in
		temp_note.endbeat = pack_igetl(fp);	//Read the beat the note ends in
		temp_note.length = pack_igetl(fp);	//Read the note's length
		temp_note.flags = pack_igetl(fp);	//Read the note's flags
		eof_sanitize_note_flags(&temp_note.flags,eof_selected_track,sourcetrack);	//Ensure the note flags are validated for the track being pasted into

		if(eof_music_pos + temp_note.pos + temp_note.length - eof_av_delay < eof_music_length)
		{
			new_note = eof_track_add_note(eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]);
			if(new_note)
			{
				new_note->note = temp_note.note;
				new_note->pos = eof_music_pos + temp_note.pos - eof_av_delay;
				new_note->length = temp_note.length;
				new_note->flags = temp_note.flags;
				new_note->type = eof_note_type;
				paste_pos[paste_count] = new_note->pos;
				paste_count++;
			}
		}
	}
	eof_track_sort_notes(eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]);
	eof_fixup_notes();
	eof_determine_hopos();
	eof_detect_difficulties(eof_song);
	if((paste_count > 0) && (eof_selection.track != eof_selected_track))
	{
		eof_selection.track = eof_selected_track;
		memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
	}
	for(i = 0; i < paste_count; i++)
	{
		for(j = 0; j < eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->notes; j++)
		{
			if((eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[j]->type == eof_note_type) && (eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[j]->pos == paste_pos[i]))
			{
				eof_selection.multi[j] = 1;
				break;
			}
		}
	}
	return 1;
}

int eof_menu_edit_snap_quarter(void)
{
	eof_snap_mode = EOF_SNAP_QUARTER;
	return 1;
}

int eof_menu_edit_snap_eighth(void)
{
	eof_snap_mode = EOF_SNAP_EIGHTH;
	return 1;
}

int eof_menu_edit_snap_sixteenth(void)
{
	eof_snap_mode = EOF_SNAP_SIXTEENTH;
	return 1;
}

int eof_menu_edit_snap_thirty_second(void)
{
	eof_snap_mode = EOF_SNAP_THIRTY_SECOND;
	return 1;
}

int eof_menu_edit_snap_twelfth(void)
{
	eof_snap_mode = EOF_SNAP_TWELFTH;
	return 1;
}

int eof_menu_edit_snap_twenty_fourth(void)
{
	eof_snap_mode = EOF_SNAP_TWENTY_FOURTH;
	return 1;
}

int eof_menu_edit_snap_forty_eighth(void)
{
	eof_snap_mode = EOF_SNAP_FORTY_EIGHTH;
	return 1;
}

int eof_menu_edit_snap_custom(void)
{
	int last_interval = eof_snap_interval;
	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_custom_snap_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_custom_snap_dialog);
	sprintf(eof_etext2, "%d", eof_snap_interval);
	if(eof_custom_snap_measure == 0)
	{	//If the custom grid snap is per beats
		eof_custom_snap_dialog[3].flags = D_SELECTED;	//Activate "per beat" radio button by default
		eof_custom_snap_dialog[4].flags = 0;
	}
	else
	{
		eof_custom_snap_dialog[3].flags = 0;
		eof_custom_snap_dialog[4].flags = D_SELECTED;	//Activate "per measure" radio button by default
	}
	if(eof_popup_dialog(eof_custom_snap_dialog, 2) == 5)
	{
		eof_snap_interval = atoi(eof_etext2);

		if(eof_custom_snap_dialog[4].flags & D_SELECTED)	//If user selected per measure instead of per beat
		{
			eof_custom_snap_measure = 1;
		}
		else
		{
			eof_custom_snap_measure = 0;
		}

		if((eof_snap_interval > EOF_MAX_GRID_SNAP_INTERVALS) || (eof_snap_interval < 1))
		{
			eof_render();
			eof_snap_interval = last_interval;
			allegro_message("Invalid snap setting, must be between 1 and %d",EOF_MAX_GRID_SNAP_INTERVALS);
		}
		else
		{
			eof_snap_mode = EOF_SNAP_CUSTOM;
		}
	}
	printf("%d\n", eof_snap_interval);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(NULL);
	return 1;
}

int eof_menu_edit_zoom_helper_in(void)
{
	return eof_menu_edit_zoom_level(eof_zoom - 1);
}

int eof_menu_edit_zoom_helper_out(void)
{
	return eof_menu_edit_zoom_level(eof_zoom + 1);
}

int eof_menu_edit_zoom_10(void)
{
	return eof_menu_edit_zoom_level(10);
}

int eof_menu_edit_zoom_9(void)
{
	return eof_menu_edit_zoom_level(9);
}

int eof_menu_edit_zoom_8(void)
{
	return eof_menu_edit_zoom_level(8);
}

int eof_menu_edit_zoom_7(void)
{
	return eof_menu_edit_zoom_level(7);
}

int eof_menu_edit_zoom_6(void)
{
	return eof_menu_edit_zoom_level(6);
}

int eof_menu_edit_zoom_5(void)
{
	return eof_menu_edit_zoom_level(5);
}

int eof_menu_edit_zoom_4(void)
{
	return eof_menu_edit_zoom_level(4);
}

int eof_menu_edit_zoom_3(void)
{
	return eof_menu_edit_zoom_level(3);
}

int eof_menu_edit_zoom_2(void)
{
	return eof_menu_edit_zoom_level(2);
}

int eof_menu_edit_zoom_1(void)
{
	return eof_menu_edit_zoom_level(1);
}

int eof_menu_edit_zoom_level(int zoom)
{
	int i;
	if((zoom > 0) && zoom <= EOF_NUM_ZOOM_LEVELS)
	{
		eof_zoom = zoom;
		for(i = 0; i < EOF_NUM_ZOOM_LEVELS; i++)
		{
			eof_edit_zoom_menu[i].flags = 0;
		}
		eof_edit_zoom_menu[EOF_NUM_ZOOM_LEVELS - zoom].flags = D_SELECTED;
	}

	return 1;
}

int eof_menu_edit_playback_speed_helper_faster(void)
{
	int i;

	for(i = 0; i < 5; i++)
	{
		eof_edit_playback_menu[i].flags = 0;
	}
	eof_playback_speed = (eof_playback_speed /250)*250;	//Account for custom playback rate (force to round down to a multiple of 250)
	eof_playback_speed += 250;
	if(eof_playback_speed > 1000)
	{
		if(eof_input_mode == EOF_INPUT_FEEDBACK)
		{
			eof_playback_speed = 250;
		}
		else
		{
			eof_playback_speed = 1000;
		}
	}
	eof_edit_playback_menu[(1000 - eof_playback_speed) / 250].flags = D_SELECTED;
	return 1;
}

int eof_menu_edit_playback_speed_helper_slower(void)
{
	int i;

	for(i = 0; i < 5; i++)
	{
		eof_edit_playback_menu[i].flags = 0;
	}
	eof_playback_speed = (eof_playback_speed /250)*250;	//Account for custom playback rate (force to round down to a multiple of 250)
	eof_playback_speed -= 250;
	if(eof_playback_speed < 250)
	{
		eof_playback_speed = 250;
	}
	eof_edit_playback_menu[(1000 - eof_playback_speed) / 250].flags = D_SELECTED;
	return 1;
}

int eof_menu_edit_playback_100(void)
{
	int i;

	for(i = 0; i < 5; i++)
	{
		eof_edit_playback_menu[i].flags = 0;
	}
	eof_edit_playback_menu[0].flags = D_SELECTED;
	eof_playback_speed = 1000;
	return 1;
}

int eof_menu_edit_playback_75(void)
{
	int i;

	for(i = 0; i < 5; i++)
	{
		eof_edit_playback_menu[i].flags = 0;
	}
	eof_edit_playback_menu[1].flags = D_SELECTED;
	eof_playback_speed = 750;
	return 1;
}

int eof_menu_edit_playback_50(void)
{
	int i;

	for(i = 0; i < 5; i++)
	{
		eof_edit_playback_menu[i].flags = 0;
	}
	eof_edit_playback_menu[2].flags = D_SELECTED;
	eof_playback_speed = 500;
	return 1;
}

int eof_menu_edit_playback_25(void)
{
	int i;

	for(i = 0; i < 5; i++)
	{
		eof_edit_playback_menu[i].flags = 0;
	}
	eof_edit_playback_menu[3].flags = D_SELECTED;
	eof_playback_speed = 250;
	return 1;
}

int eof_menu_edit_playback_custom(void)
{
	int i;
	int userinput=0;

	for(i = 0; i < 5; i++)
	{
		eof_edit_playback_menu[i].flags = 0;
	}
	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_custom_snap_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_custom_speed_dialog);
	sprintf(eof_etext2, "%d", eof_playback_speed/10);		//Load the current playback speed into a string
	if(eof_popup_dialog(eof_custom_speed_dialog, 2) == 3)		//If user activated "OK" from the custom speed dialog
	{
		userinput = atoi(eof_etext2);
//		if((userinput < 1) || (userinput > 99))			//User cannot specify to play at any speed not between 1% and 99%
//			return 1;

		eof_playback_speed = userinput * 10;
	}
	printf("%d\n", eof_playback_speed);
	eof_edit_playback_menu[4].flags = D_SELECTED;
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(NULL);
	return 1;
}

int eof_menu_edit_speed_slow(void)
{
	int i;
	eof_zoom_3d = 5;
	for(i = 0; i < 3; i++)
	{
		eof_edit_speed_menu[i].flags = 0;
	}
	eof_edit_speed_menu[0].flags = D_SELECTED;
	return 1;
}

int eof_menu_edit_speed_medium(void)
{
	int i;
	eof_zoom_3d = 3;
	for(i = 0; i < 3; i++)
	{
		eof_edit_speed_menu[i].flags = 0;
	}
	eof_edit_speed_menu[1].flags = D_SELECTED;
	return 1;
}

int eof_menu_edit_speed_fast(void)
{
	int i;
	eof_zoom_3d = 2;
	for(i = 0; i < 3; i++)
	{
		eof_edit_speed_menu[i].flags = 0;
	}
	eof_edit_speed_menu[2].flags = D_SELECTED;
	return 1;
}

int eof_menu_edit_snap_off(void)
{
	eof_snap_mode = EOF_SNAP_OFF;
	return 1;
}

int eof_menu_edit_hopo_rf(void)
{
	return eof_menu_edit_hopo_helper(EOF_HOPO_RF);
}

int eof_menu_edit_hopo_fof(void)
{
	return eof_menu_edit_hopo_helper(EOF_HOPO_FOF);
}

int eof_menu_edit_hopo_off(void)
{
	return eof_menu_edit_hopo_helper(EOF_HOPO_OFF);
}

int eof_menu_edit_hopo_manual(void)
{
	return eof_menu_edit_hopo_helper(EOF_NUM_HOPO_MODES);
}

int eof_menu_edit_hopo_helper(int hopo_view)
{
	int i;
	if(hopo_view < EOF_NUM_HOPO_MODES)
	{
		for(i = 0; i < EOF_NUM_HOPO_MODES; i++)
		{
			eof_edit_hopo_menu[i].flags = 0;
		}
		eof_edit_hopo_menu[hopo_view].flags = D_SELECTED;
		eof_hopo_view = hopo_view;
		eof_determine_hopos();
	}
	return 1;
}

int eof_menu_edit_metronome(void)
{
	if(eof_mix_metronome_enabled)
	{
		eof_mix_metronome_enabled = 0;
		eof_edit_menu[14].flags = 0;
	}
	else
	{
		eof_mix_metronome_enabled = 1;
		eof_edit_menu[14].flags = D_SELECTED;
	}
	return 1;
}

int eof_menu_edit_claps_all(void)
{
	return eof_menu_edit_claps_helper(0,31);
}

int eof_menu_edit_claps_green(void)
{
	return eof_menu_edit_claps_helper(1,1);
}

int eof_menu_edit_claps_red(void)
{
	return eof_menu_edit_claps_helper(2,2);
}

int eof_menu_edit_claps_yellow(void)
{
	return eof_menu_edit_claps_helper(3,4);
}

int eof_menu_edit_claps_blue(void)
{
	return eof_menu_edit_claps_helper(4,8);
}

int eof_menu_edit_claps_purple(void)
{
	return eof_menu_edit_claps_helper(5,16);
}

int eof_menu_edit_claps_helper(unsigned long menu_item,char claps_flag)
{
	int i;
	for(i = 0; i < 6; i++)
	{
		eof_edit_claps_menu[i].flags = 0;
	}
	eof_edit_claps_menu[menu_item].flags = D_SELECTED;
	eof_mix_claps_note = claps_flag;
	return 1;
}

int eof_menu_edit_claps(void)
{
	if(eof_mix_claps_enabled)
	{
		eof_mix_claps_enabled = 0;
		eof_edit_menu[15].flags = 0;
	}
	else
	{
		eof_mix_claps_enabled = 1;
		eof_edit_menu[15].flags = D_SELECTED;
	}
	return 1;
}

int eof_menu_edit_vocal_tones(void)
{
	if(eof_mix_vocal_tones_enabled)
	{
		eof_mix_vocal_tones_enabled = 0;
		eof_mix_percussion_enabled = 0;
		eof_edit_menu[17].flags = 0;
	}
	else
	{
		eof_mix_vocal_tones_enabled = 1;
		eof_mix_percussion_enabled = 1;
		eof_edit_menu[17].flags = D_SELECTED;
	}
	return 1;
}

int eof_menu_edit_midi_tones(void)
{
	if(eof_mix_midi_tones_enabled)
	{
		eof_mix_midi_tones_enabled = 0;
		eof_edit_menu[18].flags = 0;
	}
	else
	{
		eof_mix_midi_tones_enabled = 1;
		eof_edit_menu[18].flags = D_SELECTED;
	}
	return 1;
}

int eof_menu_edit_bookmark_helper(int b)
{
	if(eof_music_pos <= eof_av_delay)
		return 1;	//Do not place a bookmark at a negative or zero chart position

	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	if(eof_song->bookmark_pos[b] != eof_music_pos - eof_av_delay)
	{
		eof_song->bookmark_pos[b] = eof_music_pos - eof_av_delay;
	}
	else
	{
		eof_song->bookmark_pos[b] = 0;
	}
	return 1;
}

int eof_menu_edit_bookmark_0(void)
{
	return eof_menu_edit_bookmark_helper(0);
}

int eof_menu_edit_bookmark_1(void)
{
	return eof_menu_edit_bookmark_helper(1);
}

int eof_menu_edit_bookmark_2(void)
{
	return eof_menu_edit_bookmark_helper(2);
}

int eof_menu_edit_bookmark_3(void)
{
	return eof_menu_edit_bookmark_helper(3);
}

int eof_menu_edit_bookmark_4(void)
{
	return eof_menu_edit_bookmark_helper(4);
}

int eof_menu_edit_bookmark_5(void)
{
	return eof_menu_edit_bookmark_helper(5);
}

int eof_menu_edit_bookmark_6(void)
{
	return eof_menu_edit_bookmark_helper(6);
}

int eof_menu_edit_bookmark_7(void)
{
	return eof_menu_edit_bookmark_helper(7);
}

int eof_menu_edit_bookmark_8(void)
{
	return eof_menu_edit_bookmark_helper(8);
}

int eof_menu_edit_bookmark_9(void)
{
	return eof_menu_edit_bookmark_helper(9);
}

int eof_menu_edit_select_all_vocal(void)
{
	int i;

	for(i = 0; i < eof_song->vocal_track[0]->lyrics; i++)
	{
		eof_selection.track = EOF_TRACK_VOCALS;
		eof_selection.multi[i] = 1;
	}
	return 1;
}

int eof_menu_edit_select_all(void)
{
	if(eof_vocals_selected)
	{
		return eof_menu_edit_select_all_vocal();
	}
	int i;

	for(i = 0; i < eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->notes; i++)
	{
		if(eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[i]->type == eof_note_type)
		{
			eof_selection.track = eof_selected_track;
			eof_selection.multi[i] = 1;
		}
		else
		{
			eof_selection.multi[i] = 0;
		}
	}
	return 1;
}

int eof_menu_edit_select_like_vocal(void)
{
	char ntype[256];
	int ntypes = 0;
	int i, j;

	if(eof_selection.track != EOF_TRACK_VOCALS)
	{
		return 1;
	}
	if(eof_selection.current >= eof_song->vocal_track[0]->lyrics)
	{
		return 1;
	}
	for(i = 0; i < eof_song->vocal_track[0]->lyrics; i++)
	{
		if(eof_selection.multi[i])
		{
			for(j = 0; j < ntypes; j++)
			{
				if(ntype[j] == eof_song->vocal_track[0]->lyric[i]->note)
				{
					break;
				}
			}
			if(j == ntypes)
			{
				ntype[ntypes] = eof_song->vocal_track[0]->lyric[i]->note;
				ntypes++;
			}
		}
	}
	memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
	for(i = 0; i < eof_song->vocal_track[0]->lyrics; i++)
	{
		for(j = 0; j < ntypes; j++)
		{
			if(eof_song->vocal_track[0]->lyric[i]->note == ntype[j])
			{
				eof_selection.multi[i] = 1;
			}
		}
	}
	printf("done\n");
	return 1;
}

int eof_menu_edit_select_like(void)
{
	int i, j;
	char ntype[32];
	int ntypes = 0;

	if(eof_vocals_selected)
	{
		return eof_menu_edit_select_like_vocal();
	}
	if(eof_selection.track != eof_selected_track)
	{
		return 1;
	}
	if(eof_selection.current >= eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->notes)
	{
		return 1;
	}
	for(i = 0; i < eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->notes; i++)
	{
		if(eof_selection.multi[i] && (eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[i]->type == eof_note_type))
		{
			for(j = 0; j < ntypes; j++)
			{
				if(ntype[j] == eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[i]->note)
				{
					break;
				}
			}
			if(j == ntypes)
			{
				ntype[ntypes] = eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[i]->note;
				ntypes++;
			}
		}
	}
	memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
	for(i = 0; i < eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->notes; i++)
	{
		for(j = 0; j < ntypes; j++)
		{
			if((eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[i]->type == eof_note_type) && (eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[i]->note == ntype[j]))
			{
				eof_selection.track = eof_selected_track;
				eof_selection.multi[i] = 1;
			}
		}
	}
	return 1;
}

int eof_menu_edit_deselect_all(void)
{
	memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
	eof_selection.current = EOF_MAX_NOTES - 1;
	eof_selection.current_pos = 0;
	return 1;
}

int eof_menu_edit_select_rest_vocal(void)
{
	int i;

	if(eof_count_selected_notes(NULL, 0) <= 0)
	{
		return 1;
	}
/*Instead of finding the first selected note, start with the last note that was selected
	for(i = 0; i < eof_song->vocal_track[0]->lyrics; i++)
	{
		if(eof_selection.multi[i])
		{
			break;
		}
	}
*/
	if(eof_selection.current == EOF_MAX_NOTES - 1)	//No Notes selected?
		return 1;	//Don't perform this operation

	for(i = eof_selection.current; i < eof_song->vocal_track[0]->lyrics; i++)
	{
		eof_selection.multi[i] = 1;
	}
	return 1;
}

int eof_menu_edit_select_rest(void)
{
	int i;

	if(eof_vocals_selected)
	{
		return eof_menu_edit_select_rest_vocal();
	}
	if(eof_count_selected_notes(NULL, 0) <= 0)
	{
		return 1;
	}
/*Instead of finding the first selected note, start with the last note that was selected
	for(i = 0; i < eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->notes; i++)
	{
		if(eof_selection.multi[i] && eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[i]->type == eof_note_type)
		{
			break;
		}
	}
*/
	if(eof_selection.current == EOF_MAX_NOTES - 1)	//No notes selected?
		return 1;	//Don't perform this operation

	for(i = eof_selection.current; i < eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->notes; i++)
	{
		if(eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[i]->type == eof_note_type)
		{
			eof_selection.multi[i] = 1;
		}
	}

	return 1;
}

int eof_menu_edit_paste_from_supaeasy(void)
{
	return eof_menu_edit_paste_from_difficulty(EOF_NOTE_SUPAEASY);
}

int eof_menu_edit_paste_from_easy(void)
{
	return eof_menu_edit_paste_from_difficulty(EOF_NOTE_EASY);
}

int eof_menu_edit_paste_from_medium(void)
{
	return eof_menu_edit_paste_from_difficulty(EOF_NOTE_MEDIUM);
}

int eof_menu_edit_paste_from_amazing(void)
{
	return eof_menu_edit_paste_from_difficulty(EOF_NOTE_AMAZING);
}

int eof_menu_edit_paste_from_difficulty(unsigned long source_difficulty)
{
	unsigned long i;
	EOF_NOTE * new_note = NULL;

	if((eof_note_type != source_difficulty) && (source_difficulty < EOF_MAX_DIFFICULTIES))
	{	//If the current difficulty is different than the source difficulty
		if(eof_note_type_name[eof_note_type][0] == '*')
		{	//If the current difficulty is populated
			if(alert(NULL, "Overwrite notes in this difficulty?", NULL, "&Yes", "&No", 'y', 'n') == 2)
			{
				return 1;
			}
		}
		eof_clear_input();
		eof_prepare_undo(EOF_UNDO_TYPE_NOTE_SEL);
		for(i = eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->notes; i > 0; i--)
		{	//For each note in this instrument track, from last to first
			if(eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[i]->type == eof_note_type)
			{	//If this note is in the current difficulty
				eof_track_delete_note(eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum], i - 1);	//Delete it
			}
		}
		for(i = 0; i < eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->notes; i++)
		{	//For each note in this instrument track
			if(eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[i]->type == source_difficulty)
			{	//If this note is in the source difficulty
				new_note = eof_track_add_note(eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]);	//Create a note
				if(new_note)
				{	//And copy the source note to the destination difficulty
					new_note->type = eof_note_type;
					new_note->note = eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[i]->note;
					new_note->pos = eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[i]->pos;
					new_note->length = eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[i]->length;
					new_note->flags = eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[i]->flags;
				}
			}
		}
		eof_detect_difficulties(eof_song);
	}
	return 1;
}

static int notes_in_beat(int beat)
{
	int count = 0;
	int i;

	if(beat > eof_song->beats - 2)
	{
		for(i = 0; i < eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->notes; i++)
		{
			if((eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[i]->type == eof_note_type) && (eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[i]->pos >= eof_song->beat[beat]->pos))
			{
				count++;
			}
		}
	}
	else
	{
		for(i = 0; i < eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->notes; i++)
		{
			if((eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[i]->type == eof_note_type) && (eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[i]->pos >= eof_song->beat[beat]->pos) && (eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[i]->pos < eof_song->beat[beat + 1]->pos))
			{
				count++;
			}
		}
	}
	return count;
}

static int lyrics_in_beat(int beat)
{
	int count = 0;
	int i;

	if(beat > eof_song->beats - 2)
	{
		for(i = 0; i < eof_song->vocal_track[0]->lyrics; i++)
		{
			if(eof_song->vocal_track[0]->lyric[i]->pos >= eof_song->beat[beat]->pos)
			{
				count++;
			}
		}
	}
	else
	{
		for(i = 0; i < eof_song->vocal_track[0]->lyrics; i++)
		{
			if((eof_song->vocal_track[0]->lyric[i]->pos >= eof_song->beat[beat]->pos) && (eof_song->vocal_track[0]->lyric[i]->pos < eof_song->beat[beat + 1]->pos))
			{
				count++;
			}
		}
	}
	return count;
}

int eof_menu_edit_paste_from_catalog(void)
{
	int i, j;
	unsigned long paste_pos[EOF_MAX_NOTES] = {0};
	int paste_count = 0;
	int note_count = 0;
	int first = -1;
	int first_beat = -1;
	int start_beat = eof_get_beat(eof_song, eof_music_pos - eof_av_delay);
	int this_beat = -1;
	int current_beat = eof_get_beat(eof_song, eof_music_pos - eof_av_delay);
	int last_current_beat = current_beat;
	int end_beat = -1;
	float nporpos, nporendpos;
	EOF_NOTE * new_note = NULL;
	EOF_LYRIC * new_lyric = NULL;

	if(eof_song->catalog->entries > 0)
	{
		/* make sure we can paste */
		if(eof_music_pos - eof_av_delay < eof_song->beat[0]->pos)
		{
			return 1;
		}

		/* make sure we can't paste inside of the catalog entry */
		if((eof_song->catalog->entry[eof_selected_catalog_entry].track == eof_selected_track) && (eof_song->catalog->entry[eof_selected_catalog_entry].type == eof_note_type) && (eof_music_pos - eof_av_delay >= eof_song->catalog->entry[eof_selected_catalog_entry].start_pos) && (eof_music_pos - eof_av_delay <= eof_song->catalog->entry[eof_selected_catalog_entry].end_pos))
		{
			return 1;
		}
		if(eof_vocals_selected)
		{
			if(eof_song->catalog->entry[eof_selected_catalog_entry].track != EOF_TRACK_VOCALS)
			{
				return 1;
			}
			for(i = 0; i < eof_song->vocal_track[0]->lyrics; i++)
			{
				if((eof_song->vocal_track[0]->lyric[i]->pos >= eof_song->catalog->entry[eof_selected_catalog_entry].start_pos) && (eof_song->vocal_track[0]->lyric[i]->pos + eof_song->vocal_track[0]->lyric[i]->length <= eof_song->catalog->entry[eof_selected_catalog_entry].end_pos))
				{
					note_count++;
				}
			}
			if(note_count <= 0)
			{
				return 1;
			}
			eof_prepare_undo(EOF_UNDO_TYPE_NOTE_SEL);
			for(i = 0; i < eof_song->vocal_track[0]->lyrics; i++)
			{
				/* this note needs to be copied */
				if((eof_song->vocal_track[0]->lyric[i]->pos >= eof_song->catalog->entry[eof_selected_catalog_entry].start_pos) && (eof_song->vocal_track[0]->lyric[i]->pos + eof_song->vocal_track[0]->lyric[i]->length <= eof_song->catalog->entry[eof_selected_catalog_entry].end_pos))
				{
					if(first == -1)
					{
						first_beat = eof_get_beat(eof_song, eof_song->vocal_track[0]->lyric[i]->pos);
						first = 1;
					}
					this_beat = eof_get_beat(eof_song, eof_song->vocal_track[0]->lyric[i]->pos);
					if(this_beat < 0)
					{
						break;
					}
					last_current_beat = current_beat;
					current_beat = eof_get_beat(eof_song, eof_music_pos - eof_av_delay) + (this_beat - first_beat);
					if(current_beat >= eof_song->beats - 1)
					{
						break;
					}

					/* if we run into notes, abort */
					if(lyrics_in_beat(current_beat) && (last_current_beat != current_beat))
					{
						break;
					}
					nporpos = eof_get_porpos(eof_song->vocal_track[0]->lyric[i]->pos);
					nporendpos = eof_get_porpos(eof_song->vocal_track[0]->lyric[i]->pos + eof_song->vocal_track[0]->lyric[i]->length);
					end_beat = eof_get_beat(eof_song, eof_song->vocal_track[0]->lyric[i]->pos + eof_song->vocal_track[0]->lyric[i]->length);
					if(end_beat < 0)
					{
						break;
					}

					/* paste the note */
					if(end_beat - first_beat + start_beat < eof_song->beats)
					{
						new_lyric = eof_vocal_track_add_lyric(eof_song->vocal_track[0]);
						if(new_lyric)
						{
							new_lyric->note = eof_song->vocal_track[0]->lyric[i]->note;
							strcpy(new_lyric->text, eof_song->vocal_track[0]->lyric[i]->text);
							new_lyric->pos = eof_put_porpos(current_beat, nporpos, 0.0);
							new_lyric->length = eof_put_porpos(end_beat - first_beat + start_beat, nporendpos, 0.0) - new_lyric->pos;
							paste_pos[paste_count] = new_lyric->pos;
							paste_count++;
						}
					}
				}
			}
			eof_vocal_track_sort_lyrics(eof_song->vocal_track[0]);
			eof_vocal_track_fixup_lyrics(eof_song->vocal_track[0], 0);
			eof_detect_difficulties(eof_song);
			eof_selection.current_pos = 0;
			memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
			for(i = 0; i < paste_count; i++)
			{
				for(j = 0; j < eof_song->vocal_track[0]->lyrics; j++)
				{
					if(eof_song->vocal_track[0]->lyric[j]->pos == paste_pos[i])
					{
						eof_selection.track = EOF_TRACK_VOCALS;
						eof_selection.multi[j] = 1;
						break;
					}
				}
			}
		}
		else
		{
			if(eof_song->catalog->entry[eof_selected_catalog_entry].track == EOF_TRACK_VOCALS)
			{
				return 1;
			}
			for(i = 0; i < eof_song->legacy_track[(int)eof_song->catalog->entry[eof_selected_catalog_entry].track]->notes; i++)
			{
				if((eof_song->legacy_track[(int)eof_song->catalog->entry[eof_selected_catalog_entry].track]->note[i]->type == eof_song->catalog->entry[eof_selected_catalog_entry].type) && (eof_song->legacy_track[(int)eof_song->catalog->entry[eof_selected_catalog_entry].track]->note[i]->pos >= eof_song->catalog->entry[eof_selected_catalog_entry].start_pos) && (eof_song->legacy_track[(int)eof_song->catalog->entry[eof_selected_catalog_entry].track]->note[i]->pos + eof_song->legacy_track[(int)eof_song->catalog->entry[eof_selected_catalog_entry].track]->note[i]->length <= eof_song->catalog->entry[eof_selected_catalog_entry].end_pos))
				{
					note_count++;
				}
			}
			if(note_count <= 0)
			{
				return 1;
			}
			eof_prepare_undo(EOF_UNDO_TYPE_NOTE_SEL);
			for(i = 0; i < eof_song->legacy_track[(int)eof_song->catalog->entry[eof_selected_catalog_entry].track]->notes; i++)
			{
				/* this note needs to be copied */
				if((eof_song->legacy_track[(int)eof_song->catalog->entry[eof_selected_catalog_entry].track]->note[i]->type == eof_song->catalog->entry[eof_selected_catalog_entry].type) && (eof_song->legacy_track[(int)eof_song->catalog->entry[eof_selected_catalog_entry].track]->note[i]->pos >= eof_song->catalog->entry[eof_selected_catalog_entry].start_pos) && (eof_song->legacy_track[(int)eof_song->catalog->entry[eof_selected_catalog_entry].track]->note[i]->pos + eof_song->legacy_track[(int)eof_song->catalog->entry[eof_selected_catalog_entry].track]->note[i]->length <= eof_song->catalog->entry[eof_selected_catalog_entry].end_pos))
				{
					if(first == -1)
					{
						first_beat = eof_get_beat(eof_song, eof_song->legacy_track[(int)eof_song->catalog->entry[eof_selected_catalog_entry].track]->note[i]->pos);
						first = 1;
					}
					this_beat = eof_get_beat(eof_song, eof_song->legacy_track[(int)eof_song->catalog->entry[eof_selected_catalog_entry].track]->note[i]->pos);
					if(this_beat < 0)
					{
						break;
					}
					last_current_beat = current_beat;
					current_beat = eof_get_beat(eof_song, eof_music_pos - eof_av_delay) + (this_beat - first_beat);
					if(current_beat >= eof_song->beats - 1)
					{
						break;
					}

					/* if we run into notes, abort */
					if(notes_in_beat(current_beat) && (last_current_beat != current_beat))
					{
						break;
					}
					nporpos = eof_get_porpos(eof_song->legacy_track[(int)eof_song->catalog->entry[eof_selected_catalog_entry].track]->note[i]->pos);
					nporendpos = eof_get_porpos(eof_song->legacy_track[(int)eof_song->catalog->entry[eof_selected_catalog_entry].track]->note[i]->pos + eof_song->legacy_track[(int)eof_song->catalog->entry[eof_selected_catalog_entry].track]->note[i]->length);
					end_beat = eof_get_beat(eof_song, eof_song->legacy_track[(int)eof_song->catalog->entry[eof_selected_catalog_entry].track]->note[i]->pos + eof_song->legacy_track[(int)eof_song->catalog->entry[eof_selected_catalog_entry].track]->note[i]->length);
					if(end_beat < 0)
					{
						break;
					}

					/* paste the note */
					if(end_beat - first_beat + start_beat < eof_song->beats)
					{
						new_note = eof_track_add_note(eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]);
						if(new_note)
						{
							new_note->type = eof_note_type;
							new_note->note = eof_song->legacy_track[(int)eof_song->catalog->entry[eof_selected_catalog_entry].track]->note[i]->note;
							new_note->pos = eof_put_porpos(current_beat, nporpos, 0.0);
							new_note->length = eof_put_porpos(end_beat - first_beat + start_beat, nporendpos, 0.0) - new_note->pos;
							paste_pos[paste_count] = new_note->pos;
							paste_count++;
						}
					}
				}
			}
			eof_track_sort_notes(eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]);
			eof_fixup_notes();
			eof_determine_hopos();
			eof_detect_difficulties(eof_song);
			eof_selection.current_pos = 0;
			memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
			for(i = 0; i < paste_count; i++)
			{
				for(j = 0; j < eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->notes; j++)
				{
					if((eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[j]->pos == paste_pos[i]) && (eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[j]->type == eof_note_type))
					{
						eof_selection.track = eof_selected_track;
						eof_selection.multi[j] = 1;
						break;
					}
				}
			}
		}
	}
	return 1;
}

int eof_menu_edit_select_previous_vocal(void)
{
	int i;

	if(eof_count_selected_notes(NULL, 0) <= 0)	//If no notes are selected
	{
		return 1;
	}
	if(eof_selection.current == EOF_MAX_NOTES - 1)	//No Notes selected?
		return 1;	//Don't perform this operation

	for(i = 0; (i < eof_selection.current) && (i < eof_song->vocal_track[0]->lyrics); i++)
	{
		eof_selection.multi[i] = 1;
	}
	return 1;
}

int eof_menu_edit_select_previous(void)
{
	int i;

	if(eof_vocals_selected)
	{
		return eof_menu_edit_select_previous_vocal();
	}
	if(eof_count_selected_notes(NULL, 0) <= 0)	//If no notes are selected
	{
		return 1;
	}
	if(eof_selection.current == EOF_MAX_NOTES - 1)	//No notes selected?
		return 1;	//Don't perform this operation

	for(i = 0; (i < eof_selection.current) && (i < eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->notes); i++)
	{
		if(eof_song->legacy_track[eof_song->track[eof_selected_track-1]->tracknum]->note[i]->type == eof_note_type)
		{
			eof_selection.multi[i] = 1;
		}
	}

	return 1;
}

void eof_sanitize_note_flags(char *flags,int desttrack,int srctrack)
{
	if(flags == NULL)
		return;

	switch(desttrack)
	{
/*	//Since EOF_NOTE_FLAG_DBASS and EOF_NOTE_FLAG_CRAZY now share the same status bit, comment this out until Expert+ is tracked on its own flag bit again
		case EOF_TRACK_GUITAR:		//All guitar based tracks must not have the double bass flag set
		case EOF_TRACK_BASS:
		case EOF_TRACK_GUITAR_COOP:
		case EOF_TRACK_RHYTHM:
			*flags &= (~EOF_NOTE_FLAG_DBASS);	//Erase the double bass flag
			break;
*/
		case EOF_TRACK_DRUM:	//The drum track must not have any HOPO or extended sustain flags set
			*flags &= (~EOF_NOTE_FLAG_HOPO);	//Erase the temporary HOPO flag
			*flags &= (~EOF_NOTE_FLAG_F_HOPO);	//Erase the forced HOPO ON flag
			*flags &= (~EOF_NOTE_FLAG_NO_HOPO);	//Erase the forced HOPO OFF flag
		//The crazy flag is shared among the guitar and drum tracks because there weren't enough flags to give PART DRUMS pro charting support
			if(srctrack != EOF_TRACK_DRUM)
			{	//If the pasted notes are not from the drum track, erase the shared crazy status (Expert+ bass drum)
				*flags &= (~EOF_NOTE_FLAG_CRAZY);	//Erase the "crazy" note flag
			}
			break;

		default:	//Other tracks aren't accounted for yet
			*flags = 0;	//Clear all flags just to be safe
			break;
	}
}
