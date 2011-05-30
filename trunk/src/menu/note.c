#include <allegro.h>
#include "../agup/agup.h"
#include "../undo.h"
#include "../dialog.h"
#include "../dialog/proc.h"
#include "../player.h"
#include "../utility.h"
#include "../foflc/Lyric_storage.h"
#include "../main.h"
#include "note.h"
#include "ctype.h"

#ifdef USEMEMWATCH
#include "../memwatch.h"
#endif

char eof_solo_menu_mark_text[32] = "&Mark";
char eof_star_power_menu_mark_text[32] = "&Mark";
char eof_lyric_line_menu_mark_text[32] = "&Mark";
char eof_arpeggio_menu_mark_text[32] = "&Mark";
char eof_trill_menu_mark_text[32] = "&Mark";
char eof_tremolo_menu_mark_text[32] = "&Mark";
char eof_trill_menu_text[32] = "Trill";
char eof_tremolo_menu_text[32] = "Tremolo";

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

MENU eof_arpeggio_menu[] =
{
    {eof_arpeggio_menu_mark_text, eof_menu_arpeggio_mark, NULL, 0, NULL},
    {"&Remove", eof_menu_arpeggio_unmark, NULL, 0, NULL},
    {"&Erase All", eof_menu_arpeggio_erase_all, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_trill_menu[] =
{
    {eof_trill_menu_mark_text, eof_menu_trill_mark, NULL, 0, NULL},
    {"&Remove", eof_menu_trill_unmark, NULL, 0, NULL},
    {"&Erase All", eof_menu_trill_erase_all, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_tremolo_menu[] =
{
    {eof_tremolo_menu_mark_text, eof_menu_tremolo_mark, NULL, 0, NULL},
    {"&Remove", eof_menu_tremolo_unmark, NULL, 0, NULL},
    {"&Erase All", eof_menu_tremolo_erase_all, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_hopo_menu[] =
{
    {"&Auto", eof_menu_hopo_auto, NULL, 0, NULL},
    {"&Force On", eof_menu_hopo_force_on, NULL, 0, NULL},
    {"Force &Off", eof_menu_hopo_force_off, NULL, 0, NULL},
    {"&Cycle On/Off/Auto\tH", eof_menu_hopo_cycle, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_toggle_menu[] =
{
    {"&Green\tShift+1", eof_menu_note_toggle_green, NULL, 0, NULL},
    {"&Red\tShift+2", eof_menu_note_toggle_red, NULL, 0, NULL},
    {"&Yellow\tShift+3", eof_menu_note_toggle_yellow, NULL, 0, NULL},
    {"&Blue\tShift+4", eof_menu_note_toggle_blue, NULL, 0, NULL},
    {"&Purple\tShift+5", eof_menu_note_toggle_purple, NULL, 0, NULL},
    {"&Orange\tShift+6", eof_menu_note_toggle_orange, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_freestyle_menu[] =
{
    {"&On", eof_menu_set_freestyle_on, NULL, 0, NULL},
    {"O&ff", eof_menu_set_freestyle_off, NULL, 0, NULL},
    {"&Toggle\tF", eof_menu_toggle_freestyle, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_drum_menu[] =
{
    {"Toggle &Yellow cymbal\tCtrl+Y", eof_menu_note_toggle_rb3_cymbal_yellow, NULL, 0, NULL},
    {"Toggle &Blue cymbal\tCtrl+B", eof_menu_note_toggle_rb3_cymbal_blue, NULL, 0, NULL},
    {"Toggle &Green cymbal\tCtrl+G", eof_menu_note_toggle_rb3_cymbal_green, NULL, 0, NULL},
    {"Mark as &Non cymbal", eof_menu_note_remove_cymbal, NULL, 0, NULL},
    {"&Mark new notes as cymbals", eof_menu_note_default_cymbal, NULL, 0, NULL},
    {"Toggle &Expert+ bass drum\tCtrl+E", eof_menu_note_toggle_double_bass, NULL, 0, NULL},
    {"Mark new notes as Expert+", eof_menu_note_default_double_bass, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_proguitar_menu[] =
{
    {"Edit pro guitar &Note\tN", eof_menu_note_edit_pro_guitar_note, NULL, 0, NULL},
    {"Toggle tapping\tCtrl+T", eof_menu_note_toggle_tapping, NULL, 0, NULL},
    {"Mark as non &Tapping", eof_menu_note_remove_tapping, NULL, 0, NULL},
    {"Toggle Slide &Up\tCtrl+Up", eof_menu_note_toggle_slide_up, NULL, 0, NULL},
    {"Toggle Slide &Down\tCtrl+Down", eof_menu_note_toggle_slide_down, NULL, 0, NULL},
    {"Mark as non &Slide", eof_menu_note_remove_slide, NULL, 0, NULL},
    {"Toggle &Palm muting\tCtrl+M", eof_menu_note_toggle_palm_muting, NULL, 0, NULL},
    {"Mark as non palm &Muting", eof_menu_note_remove_palm_muting, NULL, 0, NULL},
    {"&Arpeggio", NULL, eof_arpeggio_menu, 0, NULL},
    {"Clear legacy bitmask", eof_menu_note_clear_legacy_values, NULL, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

MENU eof_note_menu[] =
{
    {"&Toggle", NULL, eof_note_toggle_menu, 0, NULL},
    {"Transpose Up\tUp", eof_menu_note_transpose_up, NULL, 0, NULL},
    {"Transpose Down\tDown", eof_menu_note_transpose_down, NULL, 0, NULL},
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
    {"Delete\tDel", eof_menu_note_delete, NULL, 0, NULL},
    {"Display semitones as flat", eof_display_flats_menu, NULL, 0, NULL},
    {"&Freestyle", NULL, eof_note_freestyle_menu, 0, NULL},
    {"&Drum", NULL, eof_note_drum_menu, 0, NULL},
    {"Pro &Guitar", NULL, eof_note_proguitar_menu, 0, NULL},
    {eof_trill_menu_text, NULL, eof_trill_menu, 0, NULL},
    {eof_tremolo_menu_text, NULL, eof_tremolo_menu, 0, NULL},
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
	int insp = 0, insolo = 0, inll = 0, inarpeggio = 0, intrill = 0, intremolo = 0;
	int spstart = -1, ssstart = -1, llstart = -1, arpeggiostart = -1, trillstart = -1, tremolostart = -1;
	int spend = -1, ssend = -1, llend = -1, arpeggioend = -1, trillend = -1, tremoloend = -1;
	int spp = 0, ssp = 0, llp = 0, arpeggiop = 0, trillp = 0, tremolop = 0;
	unsigned long i, j;
	unsigned long tracknum;
	int sel_start = eof_music_length, sel_end = 0;
	int firstnote = 0, lastnote;
	EOF_PHRASE_SECTION *sectionptr = NULL;
	unsigned long track_behavior = 0;

	if(eof_song && eof_song_loaded)
	{
		track_behavior = eof_song->track[eof_selected_track]->track_behavior;
		tracknum = eof_song->track[eof_selected_track]->tracknum;
		if(eof_vocals_selected)
		{	//PART VOCALS SELECTED
			for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
			{
				if((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i])
				{
					if(eof_song->vocal_track[tracknum]->lyric[i]->pos < sel_start)
					{
						sel_start = eof_song->vocal_track[tracknum]->lyric[i]->pos;
					}
					if(eof_song->vocal_track[tracknum]->lyric[i]->pos > sel_end)
					{
						sel_end = eof_song->vocal_track[tracknum]->lyric[i]->pos + eof_song->vocal_track[tracknum]->lyric[i]->length;
					}
				}
				if((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i])
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
			for(j = 0; j < eof_song->vocal_track[tracknum]->lines; j++)
			{
				if((sel_end >= eof_song->vocal_track[tracknum]->line[j].start_pos) && (sel_start <= eof_song->vocal_track[tracknum]->line[j].end_pos))
				{
					inll = 1;
					llstart = sel_start;
					llend = sel_end;
					llp = j;
				}
			}
		}
		else
		{	//PART VOCALS NOT SELECTED
			for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
			{	//For each note in the active track
				if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
				{
					if(eof_get_note_pos(eof_song, eof_selected_track, i) < sel_start)
					{
						sel_start = eof_get_note_pos(eof_song, eof_selected_track, i);
					}
					if(eof_get_note_pos(eof_song, eof_selected_track, i) > sel_end)
					{
						sel_end = eof_get_note_pos(eof_song, eof_selected_track, i) + eof_get_note_length(eof_song, eof_selected_track, i);
					}
				}
				if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
				{
					selected++;
					if(eof_get_note_note(eof_song, eof_selected_track, i))
					{
						vselected++;
					}
				}
				if(eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type)
				{
					if(firstnote < 0)
					{
						firstnote = i;
					}
					lastnote = i;
				}
			}
			for(j = 0; j < eof_get_num_star_power_paths(eof_song, eof_selected_track); j++)
			{	//For each star power path in the active track
				sectionptr = eof_get_star_power_path(eof_song, eof_selected_track, j);
				if((sel_end >= sectionptr->start_pos) && (sel_start <= sectionptr->end_pos))
				{
					insp = 1;
					spstart = sel_start;
					spend = sel_end;
					spp = j;
				}
			}
			for(j = 0; j < eof_get_num_solos(eof_song, eof_selected_track); j++)
			{	//For each solo section in the active track
				sectionptr = eof_get_solo(eof_song, eof_selected_track, j);
				if((sel_end >= sectionptr->start_pos) && (sel_start <= sectionptr->end_pos))
				{
					insolo = 1;
					ssstart = sel_start;
					ssend = sel_end;
					ssp = j;
				}
			}
			if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
			{	//If a pro guitar track is active
				for(j = 0; j < eof_song->pro_guitar_track[tracknum]->arpeggios; j++)
				{	//For each arpeggio phrase in the active track
					if((sel_end >= eof_song->pro_guitar_track[tracknum]->arpeggio[j].start_pos) && (sel_start <= eof_song->pro_guitar_track[tracknum]->arpeggio[j].end_pos))
					{
						inarpeggio = 1;
						arpeggiostart = sel_start;
						arpeggioend = sel_end;
						arpeggiop = j;
					}
				}
			}
			if((eof_song->track[eof_selected_track]->track_behavior == EOF_GUITAR_TRACK_BEHAVIOR) || (eof_song->track[eof_selected_track]->track_behavior == EOF_PRO_GUITAR_TRACK_BEHAVIOR) || (eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR))
			{	//If a legacy/pro guitar/bass or drum track is active
				for(j = 0; j < eof_get_num_trills(eof_song, eof_selected_track); j++)
				{	//For each trill phrase in the active track
					sectionptr = eof_get_trill(eof_song, eof_selected_track, j);
					if((sel_end >= sectionptr->start_pos) && (sel_start <= sectionptr->end_pos))
					{
						intrill = 1;
						trillstart = sel_start;
						trillend = sel_end;
						trillp = j;
					}
				}
				for(j = 0; j < eof_get_num_tremolos(eof_song, eof_selected_track); j++)
				{	//For each tremolo phrase in the active track
					sectionptr = eof_get_tremolo(eof_song, eof_selected_track, j);
					if((sel_end >= sectionptr->start_pos) && (sel_start <= sectionptr->end_pos))
					{
						intremolo = 1;
						tremolostart = sel_start;
						tremoloend = sel_end;
						tremolop = j;
					}
				}
			}
		}//PART VOCALS NOT SELECTED
		vselected = eof_count_selected_notes(NULL, 1);
		if(vselected)
		{	//ONE OR MORE NOTES/LYRICS SELECTED
			/* star power mark */
			sectionptr = eof_get_star_power_path(eof_song, eof_selected_track, spp);
			if((sectionptr != NULL) && (spstart == sectionptr->start_pos) && (spend == sectionptr->end_pos))
			{
				eof_star_power_menu[0].flags = D_DISABLED;
			}
			else
			{
				eof_star_power_menu[0].flags = 0;
			}

			/* solo mark */
			sectionptr = eof_get_solo(eof_song, eof_selected_track, ssp);
			if((sectionptr != NULL) && (ssstart == sectionptr->start_pos) && (ssend == sectionptr->end_pos))
			{
				eof_solo_menu[0].flags = D_DISABLED;
			}
			else
			{
				eof_solo_menu[0].flags = 0;
			}

			/* lyric line mark */
			if((llstart == eof_song->vocal_track[0]->line[llp].start_pos) && (llend == eof_song->vocal_track[0]->line[llp].end_pos))
			{
				eof_lyric_line_menu[0].flags = D_DISABLED;
			}
			else
			{
				eof_lyric_line_menu[0].flags = 0;
			}

			eof_note_menu[11].flags = 0; // solos
			eof_note_menu[12].flags = 0; // star power
		}
		else
		{	//NO NOTES/LYRICS SELECTED
			eof_star_power_menu[0].flags = D_DISABLED; // star power mark
			eof_solo_menu[0].flags = D_DISABLED; // solo mark
			eof_lyric_line_menu[0].flags = D_DISABLED; // lyric line mark
			eof_note_menu[6].flags = D_DISABLED; // edit lyric
			eof_note_menu[11].flags = D_DISABLED; // solos
			eof_note_menu[12].flags = D_DISABLED; // star power
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

		if(inarpeggio)
		{
			eof_arpeggio_menu[1].flags = 0;				//Remove arpeggio
			ustrcpy(eof_arpeggio_menu_mark_text, "Re-&Mark");
		}
		else
		{
			eof_arpeggio_menu[1].flags = D_DISABLED;	//Remove arpeggio
			ustrcpy(eof_arpeggio_menu_mark_text, "&Mark");
		}

		/* star power erase all */
		if(eof_get_num_star_power_paths(eof_song, eof_selected_track) > 0)
		{	//If there are one or more star power paths in the active track
			eof_star_power_menu[2].flags = 0;
		}
		else
		{
			eof_star_power_menu[2].flags = D_DISABLED;
		}

		/* solo erase all */
		if(eof_get_num_solos(eof_song, eof_selected_track) > 0)
		{	//If there are one or more solo paths in the active track
			eof_solo_menu[2].flags = 0;
		}
		else
		{
			eof_solo_menu[2].flags = D_DISABLED;
		}

		/* trill erase all */
		if(eof_get_num_trills(eof_song, eof_selected_track) > 0)
		{	//If there are one or more trill phrases in the active track
			eof_trill_menu[2].flags = 0;
		}
		else
		{
			eof_trill_menu[2].flags = D_DISABLED;
		}

		/* tremolo erase all */
		if(eof_get_num_tremolos(eof_song, eof_selected_track) > 0)
		{	//If there are one or more tremolo phrases in the active track
			eof_tremolo_menu[2].flags = 0;
		}
		else
		{
			eof_tremolo_menu[2].flags = D_DISABLED;
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
		{	//PART VOCALS SELECTED
			eof_note_menu[0].flags = D_DISABLED; // toggle
			eof_note_menu[1].flags = D_DISABLED; // transpose up
			eof_note_menu[2].flags = D_DISABLED; // transpose down

			if((eof_selection.current < eof_song->vocal_track[tracknum]->lyrics) && (vselected == 1))
			{	//Only enable edit and split lyric if only one lyric is selected
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
			eof_note_menu[15].flags = D_DISABLED; // HOPO

			/* lyric lines */
			if((eof_song->vocal_track[tracknum]->lines > 0) || vselected)
			{
				eof_note_menu[13].flags = 0;	// lyric lines
			}

			if(vselected)
			{
				eof_note_menu[19].flags = 0; // freestyle submenu
			}

			/* lyric lines erase all */
			if(eof_song->vocal_track[0]->lines > 0)
			{
				eof_lyric_line_menu[2].flags = 0;
			}
			else
			{
				eof_lyric_line_menu[2].flags = D_DISABLED;
			}
			eof_note_drum_menu[5].flags = D_DISABLED;	//Disable toggle Expert+ bass drum
			eof_note_menu[20].flags = D_DISABLED;	//Disable pro drum mode menu
		}
		else
		{	//PART VOCALS NOT SELECTED
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
			eof_note_menu[7].flags = D_DISABLED; // split lyric

			/* toggle crazy */
			if((track_behavior == EOF_GUITAR_TRACK_BEHAVIOR) || (track_behavior == EOF_PRO_GUITAR_TRACK_BEHAVIOR))
			{	//When a guitar track is active
				eof_note_menu[9].flags = 0;				//Enable toggle crazy
			}
			else
			{
				eof_note_menu[9].flags = D_DISABLED;	//Disable toggle crazy
			}

			/* toggle Expert+ bass drum */
			if(eof_selected_track != EOF_TRACK_DRUM)
			{	//When PART DRUMS is not active
				eof_note_menu[20].flags = D_DISABLED;	//Disable pro drum mode menu
			}
			else
			{	//When PART DRUMS is active
				eof_note_menu[20].flags = 0;			//Enable pro drum mode menu
			}

			if((eof_selected_track == EOF_TRACK_DRUM) && (eof_note_type == EOF_NOTE_AMAZING))
				eof_note_drum_menu[5].flags = 0;			//Enable toggle Expert+ bass drum only on Expert Drums
			else
				eof_note_drum_menu[5].flags = D_DISABLED;	//Otherwise disable the menu item

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

			/* HOPO */
			if((track_behavior == EOF_GUITAR_TRACK_BEHAVIOR) || (track_behavior == EOF_PRO_GUITAR_TRACK_BEHAVIOR))
			{	//When a guitar track is active
				eof_note_menu[15].flags = 0;
			}
			else
			{
				eof_note_menu[15].flags = D_DISABLED;
			}

			eof_note_menu[19].flags = D_DISABLED; // freestyle submenu

			/* Pro Guitar mode notation> */
			if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
			{	//If the active track is a pro guitar track
				eof_note_menu[21].flags = 0;			//Enable the Pro Guitar mode notation submenu

				/* Arpeggio>Erase all */
				if(eof_song->pro_guitar_track[tracknum]->arpeggios)
				{	//If there's at least one arpeggio phrase
					eof_arpeggio_menu[2].flags = 0;		//Enable Arpeggio>Erase all
				}
				else
				{
					eof_arpeggio_menu[2].flags = D_DISABLED;
				}
			}
			else
			{
				eof_note_menu[21].flags = D_DISABLED;	//Otherwise disable the submenu
			}

			/* Trill */
			eof_note_menu[22].flags = 0;	//Enable the Trill submenu
			if(intrill)
			{
				eof_trill_menu[1].flags = 0;	//Enable Trill>Remove
				ustrcpy(eof_trill_menu_mark_text, "Re-&Mark");
			}
			else
			{
				eof_trill_menu[1].flags = D_DISABLED;
				ustrcpy(eof_trill_menu_mark_text, "&Mark");
			}

			/* Tremolo */
			eof_note_menu[23].flags = 0;	//Enable the Tremolo submenu
			if(intremolo)
			{
				eof_tremolo_menu[1].flags = 0;	//Enable Tremolo>Remove
				ustrcpy(eof_tremolo_menu_mark_text, "Re-&Mark");
			}
			else
			{
				eof_tremolo_menu[1].flags = D_DISABLED;
				ustrcpy(eof_tremolo_menu_mark_text, "&Mark");
			}
			if((eof_song->track[eof_selected_track]->track_behavior == EOF_GUITAR_TRACK_BEHAVIOR) || (eof_song->track[eof_selected_track]->track_behavior == EOF_PRO_GUITAR_TRACK_BEHAVIOR))
			{	//If a legacy/pro guitar/bass track is active
				ustrcpy(eof_trill_menu_text, "Trill");
				ustrcpy(eof_tremolo_menu_text, "Tremolo");
			}
			else if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
			{	//If a legacy drum track is active
				ustrcpy(eof_trill_menu_text, "Special Drum Roll");
				ustrcpy(eof_tremolo_menu_text, "Drum Roll");
			}
			else
			{	//Disable these submenus unless a track that can use them is active
				eof_note_menu[22].flags = D_DISABLED;
				eof_note_menu[23].flags = D_DISABLED;
			}

			/* Toggle>Purple */
			if(eof_count_track_lanes(eof_song, eof_selected_track) > 4)
			{	//If the active track has a sixth usable lane
				eof_note_toggle_menu[4].flags = 0;	//Enable Toggle>Purple
			}
			else
			{
				eof_note_toggle_menu[4].flags = D_DISABLED;
			}

			/* Toggle>Orange */
			if(eof_count_track_lanes(eof_song, eof_selected_track) > 5)
			{	//If the active track has a sixth usable lane
				eof_note_toggle_menu[5].flags = 0;	//Enable Toggle>Orange
			}
			else
			{
				eof_note_toggle_menu[5].flags = D_DISABLED;
			}
		}//PART VOCALS NOT SELECTED
	}
}

int eof_menu_note_transpose_up(void)
{
	unsigned long i, j;
	unsigned long max = 31;	//This represents the highest valid note bitmask, based on the current track options (including open bass strumming)
	unsigned long flags, note, tracknum;

	if(!eof_transpose_possible(-1))
	{
		return 1;
	}
	if(eof_vocals_selected)
	{
		eof_prepare_undo(EOF_UNDO_TYPE_LYRIC_NOTE);	//Perform a cumulative undo for lyric pitch transpose operations
		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//For each lyric in the active track
			note = eof_get_note_note(eof_song, eof_selected_track, i);
			if((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i] && (note != EOF_LYRIC_PERCUSSION))
			{
				note++;
				eof_set_note_note(eof_song, eof_selected_track, i, note);
			}
		}
	}
	else
	{
		if(eof_open_bass_enabled() || (eof_count_track_lanes(eof_song, eof_selected_track) > 5))
		{	//If open bass is enabled, or the track has more than 5 lanes, lane 6 is valid for use
			max = 63;
		}
		if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
		{	//Special case:  Legacy view is in effect, display pro guitar notes as 5 lane notes
			max = 31;
		}
		tracknum = eof_song->track[eof_selected_track]->tracknum;
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//For each note in the active track
			if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
			{	//If the note is in the same track as the selected notes, is selected and is the same difficulty as the selected notes
				if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
				{	//If legacy view is in effect, get the note's legacy bitmask
					note = eof_song->pro_guitar_track[tracknum]->note[i]->legacymask;
				}
				else
				{	//Otherwise get the note's normal bitmask
					note = eof_get_note_note(eof_song, eof_selected_track, i);
				}
				note = (note << 1) & max;
				if((eof_selected_track == EOF_TRACK_BASS) && eof_open_bass_enabled() && (note & 32))
				{	//If open bass is enabled, and this transpose operation resulted in a bass guitar gem in lane 6
					flags = eof_get_note_flags(eof_song, eof_selected_track, i);
					eof_set_note_note(eof_song, eof_selected_track, i, 32);		//Clear all lanes except lane 6
					flags &= ~(EOF_NOTE_FLAG_CRAZY);	//Clear the crazy flag, which is invalid for open strum notes
					flags &= ~(EOF_NOTE_FLAG_F_HOPO);	//Clear the HOPO flags, which are invalid for open strum notes
					flags &= ~(EOF_NOTE_FLAG_NO_HOPO);
					eof_set_note_flags(eof_song, eof_selected_track, i, flags);
				}
				if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
				{	//If legacy view is in effect, set the note's legacy bitmask
					eof_song->pro_guitar_track[tracknum]->note[i]->legacymask = note;
				}
				else
				{	//Otherwise set the note's normal bitmask
					eof_set_note_note(eof_song, eof_selected_track, i, note);
				}
				if(!eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
				{	//If a pro guitar note was tranposed, move the fret values accordingly (only if legacy view isn't in effect)
					for(j = 15; j > 0; j--)
					{	//For the upper 15 frets
					eof_song->pro_guitar_track[tracknum]->note[i]->frets[j] = eof_song->pro_guitar_track[tracknum]->note[i]->frets[j-1];	//Cycle fret values up from lower lane
					}
					eof_song->pro_guitar_track[tracknum]->note[i]->frets[0] = 0xFF;	//Re-initialize lane 1 to the default of (muted)
				}
			}
		}
	}
	return 1;
}

int eof_menu_note_transpose_down(void)
{
	unsigned long i, j;
	unsigned long flags, note, tracknum;

	if(!eof_transpose_possible(1))
	{
		return 1;
	}
	if(eof_vocals_selected)
	{
		eof_prepare_undo(EOF_UNDO_TYPE_LYRIC_NOTE);	//Perform a cumulative undo for lyric pitch transpose operations
		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//For each lyric in the active track
			note = eof_get_note_note(eof_song, eof_selected_track, i);
			if((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i] && (note != EOF_LYRIC_PERCUSSION))
			{
				note--;
				eof_set_note_note(eof_song, eof_selected_track, i, note);
			}
		}
	}
	else
	{
		tracknum = eof_song->track[eof_selected_track]->tracknum;
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//For each note in the active track
			if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
			{	//If the note is in the same track as the selected notes, is selected and is the same difficulty as the selected notes
				if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
				{	//If legacy view is in effect, get the note's legacy bitmask
					note = eof_song->pro_guitar_track[tracknum]->note[i]->legacymask;
				}
				else
				{	//Otherwise get the note's normal bitmask
					note = eof_get_note_note(eof_song, eof_selected_track, i);
				}
				note = (note >> 1) & 63;
				if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
				{	//If legacy view is in effect, set the note's legacy bitmask
					eof_song->pro_guitar_track[tracknum]->note[i]->legacymask = note;
				}
				else
				{	//Otherwise set the note's normal bitmask
					eof_set_note_note(eof_song, eof_selected_track, i, note);
				}
				if((eof_selected_track == EOF_TRACK_BASS) && eof_open_bass_enabled() && (note & 1))
				{	//If open bass is enabled, and this tranpose operation resulted in a bass guitar gem in lane 1
					flags = eof_get_note_flags(eof_song, eof_selected_track, i);
					flags &= ~(EOF_NOTE_FLAG_F_HOPO);	//Clear the forced HOPO on flag, which conflicts with open bass strum notation
					eof_set_note_flags(eof_song, eof_selected_track, i, flags);
				}
				if(!eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
				{	//If a pro guitar note was tranposed, move the fret values accordingly (only if legacy view isn't in effect)
					for(j = 0; j < 15; j++)
					{	//For the lower 15 frets
						eof_song->pro_guitar_track[tracknum]->note[i]->frets[j] = eof_song->pro_guitar_track[tracknum]->note[i]->frets[j+1];	//Cycle fret values down from upper lane
					}
					eof_song->pro_guitar_track[tracknum]->note[i]->frets[15] = 0xFF;	//Re-initialize lane 15 to the default of (muted)
				}
			}
		}
	}
	return 1;
}

int eof_menu_note_transpose_up_octave(void)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;

	if(!eof_transpose_possible(-12))	//If lyric cannot move up one octave
		return 1;
	if(!eof_vocals_selected)			//If PART VOCALS is not active
		return 1;

	eof_prepare_undo(EOF_UNDO_TYPE_LYRIC_NOTE);	//Perform a cumulative undo for lyric pitch transpose operations

	for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
		if((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i] && (eof_song->vocal_track[tracknum]->lyric[i]->note != EOF_LYRIC_PERCUSSION))
			eof_song->vocal_track[tracknum]->lyric[i]->note+=12;

	return 1;
}

int eof_menu_note_transpose_down_octave(void)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;

	if(!eof_transpose_possible(12))		//If lyric cannot move down one octave
		return 1;
	if(!eof_vocals_selected)		//If PART VOCALS is not active
		return 1;


	eof_prepare_undo(EOF_UNDO_TYPE_LYRIC_NOTE);	//Perform a cumulative undo for lyric pitch transpose operations

	for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
		if((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i] && (eof_song->vocal_track[tracknum]->lyric[i]->note != EOF_LYRIC_PERCUSSION))
			eof_song->vocal_track[tracknum]->lyric[i]->note-=12;

	return 1;
}

int eof_menu_note_resnap(void)
{
	unsigned long i;
	unsigned long oldnotes = eof_get_track_size(eof_song, eof_selected_track);

	if(eof_snap_mode == EOF_SNAP_OFF)
	{
		return 1;
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{
			/* snap the note itself */
			eof_snap_logic(&eof_tail_snap, eof_get_note_pos(eof_song, eof_selected_track, i));
			eof_set_note_pos(eof_song, eof_selected_track, i, eof_tail_snap.pos);

			/* snap the tail */
			eof_snap_logic(&eof_tail_snap, eof_get_note_pos(eof_song, eof_selected_track, i) + eof_get_note_length(eof_song, eof_selected_track, i));
			eof_snap_length_logic(&eof_tail_snap);
			if(eof_get_note_length(eof_song, eof_selected_track, i) > 1)
			{
				eof_snap_logic(&eof_tail_snap, eof_get_note_pos(eof_song, eof_selected_track, i) + eof_get_note_length(eof_song, eof_selected_track, i));
				eof_note_set_tail_pos(eof_song, eof_selected_track, i, eof_tail_snap.pos);
			}
		}
	}
	eof_track_fixup_notes(eof_song, eof_selected_track, 1);
	if(oldnotes != eof_get_track_size(eof_song, eof_selected_track))
	{
		allegro_message("Warning! Some %s snapped to the same position and were automatically combined.", (eof_song->track[eof_selected_track]->track_format == EOF_VOCAL_TRACK_FORMAT) ? "lyrics" : "notes");
	}
	eof_determine_phrase_status();
	return 1;
}

int eof_menu_note_delete(void)
{
	unsigned long i, d = 0;

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{
			d++;
		}
	}
	if(d)
	{
		eof_prepare_undo(EOF_UNDO_TYPE_NOTE_SEL);
		for(i = eof_get_track_size(eof_song, eof_selected_track); i > 0; i--)
		{
			if((eof_selection.track == eof_selected_track) && eof_selection.multi[i-1] && (eof_get_note_type(eof_song, eof_selected_track, i-1) == eof_note_type))
			{
				eof_track_delete_note(eof_song, eof_selected_track, i-1);
				eof_selection.multi[i-1] = 0;
			}
		}
		eof_selection.current = EOF_MAX_NOTES - 1;
		eof_track_fixup_notes(eof_song, eof_selected_track, 0);
		eof_reset_lyric_preview_lines();
		eof_detect_difficulties(eof_song);
		eof_determine_phrase_status();
	}
	return 1;
}

int eof_menu_note_toggle_green(void)
{
	unsigned long i;
	unsigned long flags, note, tracknum = eof_song->track[eof_selected_track]->tracknum;

	if(eof_count_selected_notes(NULL, 0) <= 0)
	{
		return 1;
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{
			note = eof_get_note_note(eof_song, eof_selected_track, i);
			if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
			{	//If legacy view is in effect, alter the note's legacy bitmask
				eof_song->pro_guitar_track[tracknum]->note[i]->legacymask ^= 1;
			}
			else
			{	//Otherwise alter the note's normal bitmask
				eof_set_note_note(eof_song, eof_selected_track, i, note ^ 1);
			}
			if(eof_selected_track == EOF_TRACK_DRUM)
			{	//If green drum is being toggled on/off
				flags = eof_get_note_flags(eof_song, eof_selected_track, i);
				flags &= (~EOF_NOTE_FLAG_DBASS);		//Clear the Expert+ status if it is set
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);
			}
			else if(eof_selected_track == EOF_TRACK_BASS)
			{	//When a lane 1 bass note is added, open bass must be forced clear, because they use conflicting MIDI notation
				note &= ~(32);	//Clear the bit for lane 6 (open bass)
				eof_set_note_note(eof_song, eof_selected_track, i, note);
			}
		}
	}
	return 1;
}

int eof_menu_note_toggle_red(void)
{
	unsigned long i;
	unsigned long note, tracknum = eof_song->track[eof_selected_track]->tracknum;;

	if(eof_count_selected_notes(NULL, 0) <= 0)
	{
		return 1;
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{
			if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
			{	//If legacy view is in effect, alter the note's legacy bitmask
				eof_song->pro_guitar_track[tracknum]->note[i]->legacymask ^= 2;
			}
			else
			{	//Otherwise alter the note's normal bitmask
				note = eof_get_note_note(eof_song, eof_selected_track, i);
				eof_set_note_note(eof_song, eof_selected_track, i, note ^ 2);
			}
		}
	}
	return 1;
}

int eof_menu_note_toggle_yellow(void)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	unsigned long flags, note;

	if(eof_count_selected_notes(NULL, 0) <= 0)
	{
		return 1;
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{
			note = eof_get_note_note(eof_song, eof_selected_track, i);
			if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
			{	//If legacy view is in effect, alter the note's legacy bitmask
				eof_song->pro_guitar_track[tracknum]->note[i]->legacymask ^= 4;
			}
			else
			{	//Otherwise alter the note's normal bitmask
				eof_set_note_note(eof_song, eof_selected_track, i, note ^ 4);
			}
			if(eof_selected_track == EOF_TRACK_DRUM)
			{	//If yellow drum is being toggled on/off
				flags = eof_get_note_flags(eof_song, eof_selected_track, i);
				flags &= (~EOF_NOTE_FLAG_Y_CYMBAL);	//Clear the Pro yellow cymbal status if it is set
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);
				if(eof_mark_drums_as_cymbal && (note & 4))
				{	//If user specified to mark new notes as cymbals, and this note was toggled on
					eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_NOTE_FLAG_Y_CYMBAL,1);	//Set the yellow cymbal flag on all drum notes at this position
				}
			}
		}
	}
	return 1;
}

int eof_menu_note_toggle_blue(void)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	unsigned long flags, note;

	if(eof_count_selected_notes(NULL, 0) <= 0)
	{
		return 1;
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{
			note = eof_get_note_note(eof_song, eof_selected_track, i);
			if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
			{	//If legacy view is in effect, alter the note's legacy bitmask
				eof_song->pro_guitar_track[tracknum]->note[i]->legacymask ^= 8;
			}
			else
			{	//Otherwise alter the note's normal bitmask
				eof_set_note_note(eof_song, eof_selected_track, i, note ^ 8);
			}
			if(eof_selected_track == EOF_TRACK_DRUM)
			{	//If blue drum is being toggled on/off
				flags = eof_get_note_flags(eof_song, eof_selected_track, i);
				flags &= (~EOF_NOTE_FLAG_B_CYMBAL);	//Clear the Pro blue cymbal status if it is set
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);
				if(eof_mark_drums_as_cymbal && (note & 8))
				{	//If user specified to mark new notes as cymbals, and this note was toggled on
					eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_NOTE_FLAG_B_CYMBAL,1);	//Set the blue cymbal flag on all drum notes at this position
				}
			}
		}
	}
	return 1;
}

int eof_menu_note_toggle_purple(void)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	unsigned long flags, note;

	if(eof_count_selected_notes(NULL, 0) <= 0)
	{
		return 1;
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{
			note = eof_get_note_note(eof_song, eof_selected_track, i);
			if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
			{	//If legacy view is in effect, alter the note's legacy bitmask
				eof_song->pro_guitar_track[tracknum]->note[i]->legacymask ^= 16;
			}
			else
			{	//Otherwise alter the note's normal bitmask
				eof_set_note_note(eof_song, eof_selected_track, i, note ^ 16);
			}
			if(eof_selected_track == EOF_TRACK_DRUM)
			{	//If green drum is being toggled on/off
				flags = eof_get_note_flags(eof_song, eof_selected_track, i);
				flags &= (~EOF_NOTE_FLAG_G_CYMBAL);	//Clear the Pro green cymbal status if it is set
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);
				if(eof_mark_drums_as_cymbal && (note & 16))
				{	//If user specified to mark new notes as cymbals, and this note was toggled on
					eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_NOTE_FLAG_G_CYMBAL,1);	//Set the green cymbal flag on all drum notes at this position
				}
			}
		}
	}
	return 1;
}

int eof_menu_note_toggle_orange(void)
{
	unsigned long i;
	unsigned long flags, note, tracknum = eof_song->track[eof_selected_track]->tracknum;

	if(eof_count_track_lanes(eof_song, eof_selected_track) < 6)
	{
		return 1;	//Don't do anything if there is less than 6 tracks available
	}
	if(eof_count_selected_notes(NULL, 0) <= 0)
	{
		return 1;
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{
			if(eof_selected_track == EOF_TRACK_BASS)
			{	//When an open bass note is added, all other lanes must be forced clear, because they use conflicting MIDI notation
				flags = eof_get_note_flags(eof_song, eof_selected_track, i);
				eof_set_note_note(eof_song, eof_selected_track, i, 32);	//Clear all lanes except lane 6
				flags &= ~(EOF_NOTE_FLAG_CRAZY);		//Clear the crazy flag, which is invalid for open strum notes
				flags &= ~(EOF_NOTE_FLAG_F_HOPO);	//Clear the HOPO flags, which are invalid for open strum notes
				flags &= ~(EOF_NOTE_FLAG_NO_HOPO);
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);
			}
			else
			{
				if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
				{	//If legacy view is in effect, alter the note's legacy bitmask
					eof_song->pro_guitar_track[tracknum]->note[i]->legacymask ^= 32;
				}
				else
				{	//Otherwise alter the note's normal bitmask
					note = eof_get_note_note(eof_song, eof_selected_track, i);
					eof_set_note_note(eof_song, eof_selected_track, i, note ^ 32);
				}
			}
		}
	}
	return 1;
}

int eof_menu_note_toggle_crazy(void)
{
	unsigned long i;
	int u = 0;	//Is set to nonzero when an undo state has been made
	unsigned long track_behavior = eof_song->track[eof_selected_track]->track_behavior;
	unsigned long flags;

	if((track_behavior != EOF_GUITAR_TRACK_BEHAVIOR) && (track_behavior != EOF_PRO_GUITAR_TRACK_BEHAVIOR) && (track_behavior != EOF_DANCE_TRACK_BEHAVIOR))
		return 1;	//Do not allow this function to run unless a guitar or dance track is active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{	//If the note is in the active instrument difficulty and is selected
			if(!((eof_selected_track == EOF_TRACK_BASS) && (eof_get_note_note(eof_song, eof_selected_track, i) & 32)))
			{	//If the note is not an open bass strum note (lane 6)
				if(!u)
				{
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					u = 1;
				}
				flags = eof_get_note_flags(eof_song, eof_selected_track, i);
				flags ^= EOF_NOTE_FLAG_CRAZY;
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);
			}
		}
	}
	return 1;
}

int eof_menu_note_toggle_double_bass(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags;

	if(eof_selected_track != EOF_TRACK_DRUM)
		return 1;	//Do not allow this function to run when PART DRUMS is not active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == EOF_NOTE_AMAZING) && (eof_get_note_note(eof_song, eof_selected_track, i) & 1))
		{	//If this note is in the currently active track, is selected, is in the Expert difficulty and has a green gem
			if(!u)
			{
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				u = 1;
			}
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			flags ^= EOF_NOTE_FLAG_DBASS;
			eof_set_note_flags(eof_song, eof_selected_track, i, flags);
		}
	}
	return 1;
}

int eof_menu_note_toggle_rb3_cymbal_green(void)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	long u = 0;

	if(eof_selected_track != EOF_TRACK_DRUM)
		return 1;	//Do not allow this function to run when PART DRUMS is not active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			if(eof_get_note_note(eof_song, eof_selected_track, i) & 16)
			{	//If this drum note is purple (represents a green drum in Rock Band)
				if(!u)
				{	//Make a back up before changing the first note
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					u = 1;
				}
				eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_NOTE_FLAG_G_CYMBAL,2);	//Toggle the green cymbal flag on all drum notes at this position
			}
		}
	}
	return 1;
}

int eof_menu_note_toggle_rb3_cymbal_yellow(void)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	long u = 0;

	if(eof_selected_track != EOF_TRACK_DRUM)
		return 1;	//Do not allow this function to run when PART DRUMS is not active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			if(eof_get_note_note(eof_song, eof_selected_track, i) & 4)
			{	//If this drum note is yellow
				if(!u)
				{	//Make a back up before changing the first note
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					u = 1;
				}
				eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_NOTE_FLAG_Y_CYMBAL,2);	//Toggle the yellow cymbal flag on all drum notes at this position
			}
		}
	}
	return 1;
}

int eof_menu_note_toggle_rb3_cymbal_blue(void)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	long u = 0;

	if(eof_selected_track != EOF_TRACK_DRUM)
		return 1;	//Do not allow this function to run when PART DRUMS is not active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			if(eof_get_note_note(eof_song, eof_selected_track, i) & 8)
			{	//If this drum note is blue
				if(!u)
				{	//Make a back up before changing the first note
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					u = 1;
				}
				eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_NOTE_FLAG_B_CYMBAL,2);	//Toggle the blue cymbal flag on all drum notes at this position
			}
		}
	}
	return 1;
}

int eof_menu_note_remove_cymbal(void)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	long u = 0;
	unsigned long flags, oldflags, note;

	if(eof_selected_track != EOF_TRACK_DRUM)
		return 1;	//Do not allow this function to run when PART DRUMS is not active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			note = eof_get_note_note(eof_song, eof_selected_track, i);
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			oldflags = flags;	//Save an extra copy of the original flags
			if(	((note & 4) && (flags & EOF_NOTE_FLAG_Y_CYMBAL)) ||
				((note & 8) && (flags & EOF_NOTE_FLAG_B_CYMBAL)) ||
				((note & 16) && (flags & EOF_NOTE_FLAG_G_CYMBAL)))
			{	//If this note has a cymbal notation
				if(!u && (oldflags != flags))
				{	//Make a back up before changing the first note
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					u = 1;
				}
				eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_NOTE_FLAG_Y_CYMBAL,0);	//Clear the yellow cymbal flag on all drum notes at this position
				eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_NOTE_FLAG_B_CYMBAL,0);	//Clear the blue cymbal flag on all drum notes at this position
				eof_set_flags_at_legacy_note_pos(eof_song->legacy_track[tracknum],i,EOF_NOTE_FLAG_G_CYMBAL,0);	//Clear the green cymbal flag on all drum notes at this position
			}
		}
	}
	return 1;
}

int eof_menu_note_default_cymbal(void)
{
	if(eof_mark_drums_as_cymbal)
	{
		eof_mark_drums_as_cymbal = 0;
		eof_note_drum_menu[4].flags = 0;
	}
	else
	{
		eof_mark_drums_as_cymbal = 1;
		eof_note_drum_menu[4].flags = D_SELECTED;
	}
	return 1;
}

int eof_menu_note_default_double_bass(void)
{
	if(eof_mark_drums_as_double_bass)
	{
		eof_mark_drums_as_double_bass = 0;
		eof_note_drum_menu[6].flags = 0;
	}
	else
	{
		eof_mark_drums_as_double_bass = 1;
		eof_note_drum_menu[6].flags = D_SELECTED;
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
	unsigned long i;
	float porpos;
	long beat;

	if(eof_count_selected_notes(NULL, 0) > 0)
	{	//If notes are selected
		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//For each note in the active track
			if(eof_selection.multi[i])
			{
				beat = eof_get_beat(eof_song, eof_get_note_pos(eof_song, eof_selected_track, i));
				porpos = eof_get_porpos(eof_get_note_pos(eof_song, eof_selected_track, i));
				eof_set_note_pos(eof_song, eof_selected_track, i, eof_put_porpos(beat, porpos, eof_menu_note_push_get_offset()));
			}
		}
	}
	else
	{
		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{
			if(eof_get_note_pos(eof_song, eof_selected_track, i) >= eof_music_pos - eof_av_delay)
			{
				beat = eof_get_beat(eof_song, eof_get_note_pos(eof_song, eof_selected_track, i));
				porpos = eof_get_porpos(eof_get_note_pos(eof_song, eof_selected_track, i));
				eof_set_note_pos(eof_song, eof_selected_track, i, eof_put_porpos(beat, porpos, eof_menu_note_push_get_offset()));
			}
		}
	}
	return 1;
}

int eof_menu_note_push_up(void)
{
	unsigned long i;
	float porpos;
	long beat;

	if(eof_count_selected_notes(NULL, 0) > 0)
	{	//If notes are selected
		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//For each note in the active track
			if(eof_selection.multi[i])
			{
				beat = eof_get_beat(eof_song, eof_get_note_pos(eof_song, eof_selected_track, i));
				porpos = eof_get_porpos(eof_get_note_pos(eof_song, eof_selected_track, i));
				eof_set_note_pos(eof_song, eof_selected_track, i, eof_put_porpos(beat, porpos, -eof_menu_note_push_get_offset()));
			}
		}
	}
	else
	{
		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//For each note in the active track
			if(eof_get_note_pos(eof_song, eof_selected_track, i) >= eof_music_pos - eof_av_delay)
			{
				beat = eof_get_beat(eof_song, eof_get_note_pos(eof_song, eof_selected_track, i));
				porpos = eof_get_porpos(eof_get_note_pos(eof_song, eof_selected_track, i));
				eof_set_note_pos(eof_song, eof_selected_track, i, eof_put_porpos(beat, porpos, -eof_menu_note_push_get_offset()));
			}
		}
	}
	return 1;
}

int eof_menu_note_create_bre(void)
{
	unsigned long i;
	long first_pos = 0;
	long last_pos = eof_music_length;
	EOF_NOTE * new_note = NULL;

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{
			if(eof_get_note_pos(eof_song, eof_selected_track, i) > first_pos)
			{
				first_pos = eof_get_note_pos(eof_song, eof_selected_track, i);
			}
			if(eof_get_note_pos(eof_song, eof_selected_track, i) < last_pos)
			{
				last_pos = eof_get_note_pos(eof_song, eof_selected_track, i);
			}
		}
	}

	/* create the BRE marking note */
	if((first_pos != 0) && (last_pos != eof_music_length))
	{
		new_note = eof_track_add_create_note(eof_song, eof_selected_track, 31, first_pos, last_pos - first_pos, EOF_NOTE_SPECIAL, NULL);
	}
	return 1;
}

/* split a lyric into multiple pieces (look for ' ' characters) */
static void eof_split_lyric(int lyric)
{
	unsigned long i, l, c = 0, lastc;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	int first = 1;
	int piece = 1;
	int pieces = 1;
	char * token = NULL;
	EOF_LYRIC * new_lyric = NULL;

	if(!eof_vocals_selected)
		return;

	/* see how many pieces there are */
	for(i = 0; i < strlen(eof_song->vocal_track[tracknum]->lyric[lyric]->text); i++)
	{
		lastc = c;
		c = eof_song->vocal_track[tracknum]->lyric[lyric]->text[i];
		if((c == ' ') && (lastc != ' '))
		{
			pieces++;
		}
	}

	/* shorten the original note */
	if((eof_song->vocal_track[tracknum]->lyric[lyric]->note >= MINPITCH) && (eof_song->vocal_track[tracknum]->lyric[lyric]->note <= MAXPITCH))
	{
		l = eof_song->vocal_track[tracknum]->lyric[lyric]->length > 100 ? eof_song->vocal_track[tracknum]->lyric[lyric]->length : 250 * pieces;
	}
	else
	{
		l = 250 * pieces;
	}
	eof_song->vocal_track[tracknum]->lyric[lyric]->length = l / pieces - 20;
	if(eof_song->vocal_track[tracknum]->lyric[lyric]->length < 1)
	{
		eof_song->vocal_track[tracknum]->lyric[lyric]->length = 1;
	}

	/* split at spaces */
	strtok(eof_song->vocal_track[tracknum]->lyric[lyric]->text, " ");
	do
	{
		token = strtok(NULL, " ");
		if(token)
		{
//			if(!first)
			{
				new_lyric = eof_track_add_create_note(eof_song, eof_selected_track, eof_song->vocal_track[tracknum]->lyric[lyric]->note, eof_song->vocal_track[tracknum]->lyric[lyric]->pos + (l / pieces) * piece, l / pieces - 20, 0, token);
				piece++;
			}
			first = 0;
		}
	} while(token != NULL);
	eof_track_sort_notes(eof_song, eof_selected_track);
	eof_track_fixup_notes(eof_song, eof_selected_track, 1);
}

int eof_menu_split_lyric(void)
{
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	if(!eof_vocals_selected)
		return 1;
	if(eof_count_selected_notes(NULL, 0) != 1)
	{
		return 1;
	}
	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_split_lyric_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_split_lyric_dialog);
	ustrcpy(eof_etext, eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->text);
	if(eof_popup_dialog(eof_split_lyric_dialog, 2) == 3)
	{
		if(ustricmp(eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->text, eof_etext))
		{
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			ustrcpy(eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->text, eof_etext);
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
	unsigned long i, j;
	long insp = -1;
	long sel_start = -1;
	long sel_end = 0;
	EOF_PHRASE_SECTION *soloptr = NULL;

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{
			if(eof_get_note_pos(eof_song, eof_selected_track, i) < sel_start)
			{
				sel_start = eof_get_note_pos(eof_song, eof_selected_track, i);
			}
			if(eof_get_note_pos(eof_song, eof_selected_track, i) + eof_get_note_length(eof_song, eof_selected_track, i) > sel_end)
			{
				sel_end = eof_get_note_pos(eof_song, eof_selected_track, i) + eof_get_note_length(eof_song, eof_selected_track, i);
			}
		}
	}
	for(j = 0; j < eof_get_num_solos(eof_song, eof_selected_track); j++)
	{	//For each solo in the track
		soloptr = eof_get_solo(eof_song, eof_selected_track, j);
		if((sel_end >= soloptr->start_pos) && (sel_start <= soloptr->end_pos))
		{
			insp = j;
		}
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	if(insp < 0)
	{
		eof_track_add_solo(eof_song, eof_selected_track, sel_start, sel_end);
	}
	else
	{
		soloptr = eof_get_solo(eof_song, eof_selected_track, insp);
		soloptr->start_pos = sel_start;
		soloptr->end_pos = sel_end;
	}
	return 1;
}

int eof_menu_solo_unmark(void)
{
	unsigned long i, j;
	EOF_PHRASE_SECTION *soloptr = NULL;

	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{
			for(j = 0; j < eof_get_num_solos(eof_song, eof_selected_track); j++)
			{	//For each solo section in the track
				soloptr = eof_get_solo(eof_song, eof_selected_track, j);
				if((eof_get_note_pos(eof_song, eof_selected_track, i) >= soloptr->start_pos) && (eof_get_note_pos(eof_song, eof_selected_track, i) + eof_get_note_length(eof_song, eof_selected_track, i) <= soloptr->end_pos))
				{
					eof_track_delete_solo(eof_song, eof_selected_track, j);
					break;
				}
			}
		}
	}
	return 1;
}

int eof_menu_solo_erase_all(void)
{
	unsigned long ctr;
	EOF_PHRASE_SECTION *sectionptr;

	if(alert(NULL, "Erase all solos from this track?", NULL, "&Yes", "&No", 'y', 'n') == 1)
	{
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		for(ctr = 0; ctr < eof_get_num_solos(eof_song, eof_selected_track); ctr++)
		{	//For each existing solo section
			sectionptr = eof_get_solo(eof_song, eof_selected_track, ctr);
			if(sectionptr->name)
			{	//If this phrase has a name
				free(sectionptr->name);	//Free it
			}
		}
		eof_set_num_solos(eof_song, eof_selected_track, 0);
	}
	return 1;
}

int eof_menu_star_power_mark(void)
{
	unsigned long i, j;
	long insp = -1;
	long sel_start = -1;
	long sel_end = 0;
	EOF_PHRASE_SECTION *starpowerptr = NULL;

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{
			if(eof_get_note_pos(eof_song, eof_selected_track, i) < sel_start)
			{
				sel_start = eof_get_note_pos(eof_song, eof_selected_track, i);
			}
			if(eof_get_note_pos(eof_song, eof_selected_track, i) > sel_end)
			{
				sel_end = eof_get_note_pos(eof_song, eof_selected_track, i) + (eof_get_note_length(eof_song, eof_selected_track, i) > 20 ? eof_get_note_length(eof_song, eof_selected_track, i) : 20);
			}
		}
	}
	for(j = 0; j < eof_get_num_star_power_paths(eof_song, eof_selected_track); j++)
	{	//For each star power path in the active track
		starpowerptr = eof_get_star_power_path(eof_song, eof_selected_track, j);
		if((sel_end >= starpowerptr->start_pos) && (sel_start <= starpowerptr->end_pos))
		{
			insp = j;
		}
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	if(insp < 0)
	{
		eof_track_add_star_power_path(eof_song, eof_selected_track, sel_start, sel_end);
	}
	else
	{
		starpowerptr = eof_get_star_power_path(eof_song, eof_selected_track, insp);
		if(starpowerptr != NULL)
		{
			starpowerptr->start_pos = sel_start;
			starpowerptr->end_pos = sel_end;
		}
	}
	eof_determine_phrase_status();
	return 1;
}

int eof_menu_star_power_unmark(void)
{
	unsigned long i, j;
	EOF_PHRASE_SECTION *starpowerptr = NULL;

	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{
			for(j = 0; j < eof_get_num_star_power_paths(eof_song, eof_selected_track); j++)
			{	//For each star power path in the track
				starpowerptr = eof_get_star_power_path(eof_song, eof_selected_track, j);
				if((eof_get_note_pos(eof_song, eof_selected_track, i) >= starpowerptr->start_pos) && (eof_get_note_pos(eof_song, eof_selected_track, i) <= starpowerptr->end_pos))
				{
					eof_track_delete_star_power_path(eof_song, eof_selected_track, j);
					break;
				}
			}
		}
	}
	eof_determine_phrase_status();
	return 1;
}

int eof_menu_star_power_erase_all(void)
{
	unsigned long ctr;
	EOF_PHRASE_SECTION *sectionptr;

	if(alert(NULL, "Erase all star power from this track?", NULL, "&Yes", "&No", 'y', 'n') == 1)
	{
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		for(ctr = 0; ctr < eof_get_num_star_power_paths(eof_song, eof_selected_track); ctr++)
		{	//For each existing star power section
			sectionptr = eof_get_star_power_path(eof_song, eof_selected_track, ctr);
			if(sectionptr->name)
			{	//If this phrase has a name
				free(sectionptr->name);	//Free it
			}
		}
		eof_set_num_star_power_paths(eof_song, eof_selected_track, 0);
	}
	eof_determine_phrase_status();
	return 1;
}

int eof_menu_lyric_line_mark(void)
{
	unsigned long i, j;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	long sel_start = -1;
	long sel_end = 0;
	int originalflags = 0; //Used to apply the line's original flags after the line is recreated

	if(!eof_vocals_selected)
		return 1;

	for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
	{
		if((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i])
		{
			if(eof_song->vocal_track[tracknum]->lyric[i]->pos < sel_start)
			{
				sel_start = eof_song->vocal_track[tracknum]->lyric[i]->pos;
				if(sel_start < eof_song->tags->ogg[eof_selected_ogg].midi_offset)
				{
					sel_start = eof_song->tags->ogg[eof_selected_ogg].midi_offset;
				}
			}
			if(eof_song->vocal_track[tracknum]->lyric[i]->pos > sel_end)
			{
				sel_end = eof_song->vocal_track[tracknum]->lyric[i]->pos + eof_song->vocal_track[tracknum]->lyric[i]->length;
				if(sel_end >= eof_music_length)
				{
					sel_end = eof_music_length - 1;
				}
			}
		}
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Create the undo state before removing/adding phrase(s)
	for(j = eof_song->vocal_track[tracknum]->lines; j > 0; j--)
	{
		if((sel_end >= eof_song->vocal_track[tracknum]->line[j-1].start_pos) && (sel_start <= eof_song->vocal_track[tracknum]->line[j-1].end_pos))
		{
			originalflags=eof_song->vocal_track[tracknum]->line[j-1].flags;	//Save this line's flags before deleting it
			eof_vocal_track_delete_line(eof_song->vocal_track[tracknum], j-1);
		}
	}
	eof_vocal_track_add_line(eof_song->vocal_track[tracknum], sel_start, sel_end);

	if(eof_song->vocal_track[tracknum]->lines >0)
		eof_song->vocal_track[tracknum]->line[eof_song->vocal_track[tracknum]->lines-1].flags = originalflags;	//Restore the line's flags

	/* check for overlapping lines */
	for(i = 0; i < eof_song->vocal_track[tracknum]->lines; i++)
	{
		for(j = i; j < eof_song->vocal_track[tracknum]->lines; j++)
		{
			if((i != j) && (eof_song->vocal_track[tracknum]->line[i].start_pos <= eof_song->vocal_track[tracknum]->line[j].end_pos) && (eof_song->vocal_track[tracknum]->line[j].start_pos <= eof_song->vocal_track[tracknum]->line[i].end_pos))
			{
				eof_song->vocal_track[tracknum]->line[i].start_pos = eof_song->vocal_track[tracknum]->line[j].end_pos + 1;
			}
		}
	}
	eof_reset_lyric_preview_lines();
	return 1;
}

int eof_menu_lyric_line_unmark(void)
{
	unsigned long i, j;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;

	if(!eof_vocals_selected)
		return 1;

	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
	{
		if((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i])
		{
			for(j = 0; j < eof_song->vocal_track[tracknum]->lines; j++)
			{
				if((eof_song->vocal_track[tracknum]->lyric[i]->pos >= eof_song->vocal_track[tracknum]->line[j].start_pos) && (eof_song->vocal_track[tracknum]->lyric[i]->pos <= eof_song->vocal_track[tracknum]->line[j].end_pos))
				{
					eof_vocal_track_delete_line(eof_song->vocal_track[tracknum], j);
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
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;

	if(!eof_vocals_selected)
		return 1;

	if(alert(NULL, "Erase all lyric lines?", NULL, "&Yes", "&No", 'y', 'n') == 1)
	{
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		eof_song->vocal_track[tracknum]->lines = 0;
		eof_reset_lyric_preview_lines();
	}
	return 1;
}

int eof_menu_lyric_line_toggle_overdrive(void)
{
	char used[1024] = {0};
	unsigned long i, j;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;

	if(!eof_vocals_selected)
		return 1;

	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
	{
		if((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i])
		{
			for(j = 0; j < eof_song->vocal_track[tracknum]->lines; j++)
			{
				if((eof_song->vocal_track[tracknum]->lyric[i]->pos >= eof_song->vocal_track[tracknum]->line[j].start_pos) && (eof_song->vocal_track[tracknum]->lyric[i]->pos <= eof_song->vocal_track[tracknum]->line[j].end_pos && !used[j]))
				{
					eof_song->vocal_track[tracknum]->line[j].flags ^= EOF_LYRIC_LINE_FLAG_OVERDRIVE;
					used[j] = 1;
				}
			}
		}
	}
	return 1;
}

int eof_menu_hopo_auto(void)
{
	unsigned long i;
	char undo_made = 0;	//Set to nonzero if an undo state was saved
	unsigned long flags, oldflags;

	if((eof_selected_track == EOF_TRACK_DRUM) || eof_vocals_selected)
		return 1;	//Do not allow this function to run when PART DRUMS or PART VOCALS is active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if(eof_selection.multi[i])
		{	//If the note is selected
			if(!((eof_selected_track == EOF_TRACK_BASS) && (eof_get_note_note(eof_song, eof_selected_track, i) & 32)))
			{	//If the note is not an open bass strum note
				flags = eof_get_note_flags(eof_song, eof_selected_track, i);
				oldflags = flags;					//Save another copy of the original flags
				flags &= (~EOF_NOTE_FLAG_F_HOPO);	//Clear the HOPO on flag
				flags &= (~EOF_NOTE_FLAG_NO_HOPO);	//Clear the HOPO off flag
				if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
				{	//If this is a pro guitar note, ensure that Hammer on, Pull of and Tap statuses are cleared
					flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_HO);	//Clear the hammer on flag
					flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_PO);	//Clear the pull off flag
					flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_TAP);	//Clear the tap flag
				}
				if(!undo_made && (flags != oldflags))
				{	//If an undo state hasn't been made yet
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make one
					undo_made = 1;
				}
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);
			}
		}
	}
	eof_determine_phrase_status();
	return 1;
}

int eof_menu_hopo_cycle(void)
{
	unsigned long i;
	char undo_made = 0;	//Set to nonzero if an undo state was saved
	unsigned long flags;
	unsigned long track_behavior = eof_song->track[eof_selected_track]->track_behavior;

	if((track_behavior != EOF_GUITAR_TRACK_BEHAVIOR) && (track_behavior != EOF_PRO_GUITAR_TRACK_BEHAVIOR))
		return 1;	//Do not allow this function to run unless a guitar track is active

	if((eof_count_selected_notes(NULL, 0) > 0))
	{
		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//For each note in the active track
			if(eof_selection.multi[i])
			{	//If the note is selected
				if(!((eof_selected_track == EOF_TRACK_BASS) && (eof_get_note_note(eof_song, eof_selected_track, i) & 32)))
				{	//If the note is not an open bass strum note
					if((eof_selected_track == EOF_TRACK_BASS) && eof_open_bass_enabled() && (eof_get_note_note(eof_song, eof_selected_track, i) & 1))
					{	//If open bass strumming is enabled and this is a bass guitar note that uses lane 1
						continue;	//Skip this note, as open bass and forced HOPO on lane 1 conflict
					}
					if(!undo_made)
					{	//If an undo state hasn't been made yet
						eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make one
						undo_made = 1;
					}
					flags = eof_get_note_flags(eof_song, eof_selected_track, i);
					if(flags & EOF_NOTE_FLAG_F_HOPO)
					{	//If the note was a forced on HOPO, make it a forced off HOPO
						flags &= ~EOF_NOTE_FLAG_F_HOPO;	//Turn off forced on hopo
						flags |= EOF_NOTE_FLAG_NO_HOPO;	//Turn on forced off hopo
						if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
						{	//If this is a pro guitar note, ensure that Hammer on, Pull of and Tap statuses are cleared
							flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_HO);	//Clear the hammer on flag
							flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_PO);	//Clear the pull off flag
							flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_TAP);	//Clear the tap flag
						}
					}
					else if(flags & EOF_NOTE_FLAG_NO_HOPO)
					{	//If the note was a forced off HOPO, make it an auto HOPO
						flags &= ~EOF_NOTE_FLAG_F_HOPO;		//Clear the forced on hopo flag
						flags &= ~EOF_NOTE_FLAG_NO_HOPO;	//Clear the forced off hopo flag
						if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
						{	//If this is a pro guitar note, ensure that Hammer on, Pull of and Tap statuses are cleared
							flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_HO);	//Clear the hammer on flag
							flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_PO);	//Clear the pull off flag
							flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_TAP);	//Clear the tap flag
						}
					}
					else
					{	//If the note was an auto HOPO, make it a forced on HOPO
						flags |= EOF_NOTE_FLAG_F_HOPO;	//Turn on forced on hopo
						if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
						{	//If this is a pro guitar note
							flags |= EOF_PRO_GUITAR_NOTE_FLAG_HO;		//Set the hammer on flag (default HOPO type)
							flags &= ~(EOF_PRO_GUITAR_NOTE_FLAG_PO);	//Clear the pull off flag
							flags &= ~(EOF_PRO_GUITAR_NOTE_FLAG_TAP);	//Clear the tap flag
						}
					}
					eof_set_note_flags(eof_song, eof_selected_track, i, flags);
				}
			}
		}
		eof_determine_phrase_status();
	}
	return 1;
}

int eof_menu_hopo_force_on(void)
{
	unsigned long i;
	char undo_made = 0;	//Set to nonzero if an undo state was saved
	unsigned long flags, oldflags;

	if((eof_selected_track == EOF_TRACK_DRUM) || eof_vocals_selected)
		return 1;	//Do not allow this function to run when PART DRUMS or PART VOCALS is active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if(eof_selection.multi[i])
		{
			if(!((eof_selected_track == EOF_TRACK_BASS) && (eof_get_note_note(eof_song, eof_selected_track, i) & 32)))
			{	//If the note is not an open bass strum note
				if((eof_selected_track == EOF_TRACK_BASS) && eof_open_bass_enabled() && (eof_get_note_note(eof_song, eof_selected_track, i) & 1))
				{	//If open bass strumming is enabled and this is a bass guitar note that uses lane 1
					continue;	//Skip this note, as open bass and forced HOPO on lane 1 conflict
				}
				flags = eof_get_note_flags(eof_song, eof_selected_track, i);
				oldflags = flags;					//Save another copy of the original flags
				flags |= EOF_NOTE_FLAG_F_HOPO;		//Set the HOPO on flag
				flags &= (~EOF_NOTE_FLAG_NO_HOPO);	//Clear the HOPO off flag
				if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
				{	//If this is a pro guitar note
					flags |= EOF_PRO_GUITAR_NOTE_FLAG_HO;		//Set the hammer on flag (default HOPO type)
					flags &= ~(EOF_PRO_GUITAR_NOTE_FLAG_PO);	//Clear the pull off flag
					flags &= ~(EOF_PRO_GUITAR_NOTE_FLAG_TAP);	//Clear the tap flag
				}
				if(!undo_made && (flags != oldflags))
				{	//If an undo state hasn't been made yet
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make one
					undo_made = 1;
				}
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);
			}
		}
	}
	eof_determine_phrase_status();
	return 1;
}

int eof_menu_hopo_force_off(void)
{
	unsigned long i;
	char undo_made = 0;	//Set to nonzero if an undo state was saved
	unsigned long flags, oldflags;

	if((eof_selected_track == EOF_TRACK_DRUM) || eof_vocals_selected)
		return 1;	//Do not allow this function to run when PART DRUMS or PART VOCALS is active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if(eof_selection.multi[i])
		{
			if(!((eof_selected_track == EOF_TRACK_BASS) && (eof_get_note_note(eof_song, eof_selected_track, i) & 32)))
			{	//If the note is not an open bass strum note
				flags = eof_get_note_flags(eof_song, eof_selected_track, i);
				oldflags = flags;					//Save another copy of the original flags
				flags |= EOF_NOTE_FLAG_NO_HOPO;		//Set the HOPO off flag
				flags &= (~EOF_NOTE_FLAG_F_HOPO);	//Clear the HOPO on flag
				if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
				{	//If this is a pro guitar note, ensure that Hammer on, Pull of and Tap statuses are cleared
					flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_HO);	//Clear the hammer on flag
					flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_PO);	//Clear the pull off flag
					flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_TAP);	//Clear the tap flag
				}
				if(!undo_made && (flags != oldflags))
				{	//If an undo state hasn't been made yet
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make one
					undo_made = 1;
				}
				eof_set_note_flags(eof_song, eof_selected_track, i, flags);
			}
		}
	}
	eof_determine_phrase_status();
	return 1;
}

int eof_transpose_possible(int dir)
{
	unsigned long i, note, tracknum;
	unsigned long max = 16;	//This represents the highest note bitmask value that will be allowed to transpose up, based on the current track options (including open bass strumming)

	/* no notes, no transpose */
	if(eof_vocals_selected)
	{
		if(eof_get_track_size(eof_song, eof_selected_track) <= 0)
		{
			return 0;
		}

		if(eof_count_selected_notes(NULL, 1) <= 0)
		{
			return 0;
		}

		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//Test if the lyric can transpose the given amount in the given direction
			if((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i])
			{
				if((eof_get_note_note(eof_song, eof_selected_track, i) == 0) || (eof_get_note_note(eof_song, eof_selected_track, i) == EOF_LYRIC_PERCUSSION))
				{	//Cannot transpose a pitchless lyric or a vocal percussion note
					return 0;
				}
				if(eof_get_note_note(eof_song, eof_selected_track, i) - dir < MINPITCH)
				{
					return 0;
				}
				else if(eof_get_note_note(eof_song, eof_selected_track, i) - dir > MAXPITCH)
				{
					return 0;
				}
			}
		}
		return 1;
	}
	else
	{
		if(eof_open_bass_enabled() || (eof_count_track_lanes(eof_song, eof_selected_track) > 5))
		{	//If open bass is enabled, or the track has more than 5 lanes, lane 5 can transpose up to lane 6
			max = 32;
		}
		if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
		{	//Special case:  Legacy view is in effect, revert to lane 4 being the highest that can transpose up
			max = 16;
		}
		if(eof_count_track_lanes(eof_song, eof_selected_track) == 4)
		{	//If the active track has 4 lanes (ie. PART DANCE), make lane 3 the highest that can transpose up
			max = 8;
		}
		if(eof_get_track_size(eof_song, eof_selected_track) <= 0)
		{
			return 0;
		}
		if(eof_count_selected_notes(NULL, 1) <= 0)
		{
			return 0;
		}

		tracknum = eof_song->track[eof_selected_track]->tracknum;
		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//For each note in the active track
			if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
			{	//If the note is selected and in the active instrument difficulty
				if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
				{	//If legacy view is in effect, get the note's legacy bitmask
					note = eof_song->pro_guitar_track[tracknum]->note[i]->legacymask;
				}
				else
				{	//Otherwise get the note's normal bitmask
					note = eof_get_note_note(eof_song, eof_selected_track, i);
				}
				if(note == 0)
				{	//Special case: In legacy view, a note's legacy bitmask is undefined
					return 0;	//Disallow tranposing for this selection of notes
				}
				if((note & 1) && (dir > 0))
				{
					return 0;
				}
				else if((note & max) && (dir < 0))
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
	int ret=0;

	if(!eof_vocals_selected)
		return D_O_K;

	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_lyric_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_lyric_dialog);
	ustrcpy(eof_etext, "");

	if(eof_pen_lyric.note != EOF_LYRIC_PERCUSSION)		//If not entering a percussion note
	{
		ret = eof_popup_dialog(eof_lyric_dialog, 2);	//prompt for lyric text
		if(!eof_check_string(eof_etext) && !eof_pen_lyric.note)	//If the placed lyric is both pitchless AND textless
			return D_O_K;	//Return without adding
	}

	if((ret == 3) || (eof_pen_lyric.note == EOF_LYRIC_PERCUSSION))
	{
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		new_lyric = eof_track_add_create_note(eof_song, eof_selected_track, eof_pen_lyric.note, eof_pen_lyric.pos, eof_pen_lyric.length, 0, NULL);
		ustrcpy(new_lyric->text, eof_etext);
		eof_selection.track = EOF_TRACK_VOCALS;
		eof_selection.current_pos = new_lyric->pos;
		eof_selection.range_pos_1 = eof_selection.current_pos;
		eof_selection.range_pos_2 = eof_selection.current_pos;
		memset(eof_selection.multi, 0, sizeof(eof_selection.multi));
		eof_track_sort_notes(eof_song, eof_selected_track);
		eof_track_fixup_notes(eof_song, eof_selected_track, 0);
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
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;

	if(!eof_vocals_selected)
		return 1;

	if(eof_count_selected_notes(NULL, 0) != 1)
	{
		return 1;
	}
	eof_cursor_visible = 0;
	eof_render();
	eof_color_dialog(eof_lyric_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_lyric_dialog);
	ustrcpy(eof_etext, eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->text);
	if(eof_popup_dialog(eof_lyric_dialog, 2) == 3)	//User hit OK on "Edit Lyric" dialog instead of canceling
	{
		if(eof_is_freestyle(eof_etext))		//If the text entered had one or more freestyle characters
			eof_set_freestyle(eof_etext,1);	//Perform any necessary corrections

		if(ustricmp(eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->text, eof_etext))	//If the updated string (eof_etext) is different
		{
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			if(!eof_check_string(eof_etext))
			{	//If the updated string is empty or just whitespace
				eof_track_delete_note(eof_song, eof_selected_track, eof_selection.current);
			}
			else
			{
				ustrcpy(eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->text, eof_etext);
				eof_fix_lyric(eof_song->vocal_track[tracknum],eof_selection.current);	//Correct the freestyle character if necessary
			}
		}
	}
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(screen);
	return D_O_K;
}

int eof_menu_set_freestyle(char status)
{
	unsigned long i=0,ctr=0;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;

	if(!eof_vocals_selected)
		return 1;

//Determine if any lyrics will actually be affected by this action
	if(eof_vocals_selected && (eof_selection.track == EOF_TRACK_VOCALS))
	{	//If lyrics are selected
		for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
		{	//For each lyric...
			if(eof_selection.multi[i])
			{	//...that is selected, count the number of lyrics that would be altered
				if(eof_lyric_is_freestyle(eof_song->vocal_track[tracknum],i) && (status == 0))
					ctr++;	//Increment if a lyric would change from freestyle to non freestyle
				else if(!eof_lyric_is_freestyle(eof_song->vocal_track[tracknum],i) && (status != 0))
					ctr++;	//Increment if a lyric would change from non freestyle to freestyle
			}
		}

//If so, make an undo state and perform the action on the lyrics
		if(ctr)
		{	//If at least one lyric is going to be modified
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make an undo state

			for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
			{	//For each lyric...
				if(eof_selection.multi[i])
				{	//...that is selected, apply the specified freestyle status
					eof_set_freestyle(eof_song->vocal_track[tracknum]->lyric[i]->text,status);
				}
			}
		}
	}

	return 1;
}

int eof_menu_set_freestyle_on(void)
{
	return eof_menu_set_freestyle(1);
}

int eof_menu_set_freestyle_off(void)
{
	return eof_menu_set_freestyle(0);
}

int eof_menu_toggle_freestyle(void)
{
	unsigned long i=0;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;

	if(eof_vocals_selected && (eof_selection.track == EOF_TRACK_VOCALS) && eof_count_selected_notes(NULL, 0))
	{	//If lyrics are selected
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make an undo state

		for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
		{	//For each lyric...
			if(eof_selection.multi[i])
			{	//...that is selected, toggle its freestyle status
				eof_toggle_freestyle(eof_song->vocal_track[tracknum],i);
			}
		}
	}

	return 1;
}

char eof_note_edit_name[EOF_NAME_LENGTH+1] = {0};

DIALOG eof_pro_guitar_note_dialog[] =
{
/*	(proc)					(x)  (y)  (w)  (h) (fg) (bg) (key) (flags) (d1)       (d2) (dp)          (dp2)          (dp3) */
	{d_agup_window_proc,    0,   48,  224, 332,2,   23,  0,    0,      0,         0,   "Edit pro guitar note",NULL, NULL },
	{d_agup_text_proc,      16,  80,  64,  8,  2,   23,  0,    0,      0,         0,   "Name:",      NULL,          NULL },
	{d_agup_edit_proc,		74,  76,  134, 20, 2,   23,  0,    0, EOF_NAME_LENGTH,0,eof_note_edit_name,       NULL, NULL },

	//Note:  In guitar theory, string 1 refers to high e
	{d_agup_text_proc,      16,  128, 64,  8,  2,   23,  0,    0,      0,         0,   "String 1:",  NULL,          NULL },
	{eof_verified_edit_proc,74,  124, 22,  20, 2,   23,  0,    0,      2,         0,   eof_string1,  "0123456789Xx",NULL },
	{d_agup_text_proc,      16,  152, 64,  8,  2,   23,  0,    0,      0,         0,   "String 2:",  NULL,          NULL },
	{eof_verified_edit_proc,74,  148, 22,  20, 2,   23,  0,    0,      2,         0,   eof_string2,  "0123456789Xx",NULL },
	{d_agup_text_proc,      16,  176, 64,  8,  2,   23,  0,    0,      0,         0,   "String 3:",  NULL,          NULL },
	{eof_verified_edit_proc,74,  172, 22,  20, 2,   23,  0,    0,      2,         0,   eof_string3,  "0123456789Xx",NULL },
	{d_agup_text_proc,      16,  200, 64,  8,  2,   23,  0,    0,      0,         0,   "String 4:",  NULL,          NULL },
	{eof_verified_edit_proc,74,  196, 22,  20, 2,   23,  0,    0,      2,         0,   eof_string4,  "0123456789Xx",NULL },
	{d_agup_text_proc,      16,  224, 64,  8,  2,   23,  0,    0,      0,         0,   "String 5:",  NULL,          NULL },
	{eof_verified_edit_proc,74,  220, 22,  20, 2,   23,  0,    0,      2,         0,   eof_string5,  "0123456789Xx",NULL },
	{d_agup_text_proc,      16,  248, 64,  8,  2,   23,  0,    0,      0,         0,   "String 6:",  NULL,          NULL },
	{eof_verified_edit_proc,74,  244, 22,  20, 2,   23,  0,    0,      2,         0,   eof_string6,  "0123456789Xx",NULL },

	{d_agup_text_proc,      16,  108, 64,  8,  2,   23,  0,    0,      0,         0,   "Pro",        NULL,          NULL },
	{d_agup_text_proc,      160, 108, 64,  8,  2,   23,  0,    0,      0,         0,   "Legacy",     NULL,          NULL },
	{d_agup_check_proc,		158, 151, 64,  16, 2,   23,  0,    0,      0,         0,   "Lane 5",     NULL,          NULL },
	{d_agup_check_proc,		158, 175, 64,  16, 2,   23,  0,    0,      0,         0,   "Lane 4",     NULL,          NULL },
	{d_agup_check_proc,		158, 199, 64,  16, 2,   23,  0,    0,      0,         0,   "Lane 3",     NULL,          NULL },
	{d_agup_check_proc,		158, 223, 64,  16, 2,   23,  0,    0,      0,         0,   "Lane 2",     NULL,          NULL },
	{d_agup_check_proc,		158, 247, 64,  16, 2,   23,  0,    0,      0,         0,   "Lane 1",     NULL,          NULL },

	{d_agup_text_proc,      102, 108, 64,  8,  2,   23,  0,    0,      0,         0,   "Ghost",      NULL,          NULL },
	{d_agup_check_proc,		100, 127, 14,  16, 2,   23,  0,    0,      0,         0,   "",           NULL,          NULL },
	{d_agup_check_proc,		100, 151, 14,  16, 2,   23,  0,    0,      0,         0,   "",           NULL,          NULL },
	{d_agup_check_proc,		100, 175, 14,  16, 2,   23,  0,    0,      0,         0,   "",           NULL,          NULL },
	{d_agup_check_proc,		100, 199, 14,  16, 2,   23,  0,    0,      0,         0,   "",           NULL,          NULL },
	{d_agup_check_proc,		100, 223, 14,  16, 2,   23,  0,    0,      0,         0,   "",           NULL,          NULL },
	{d_agup_check_proc,		100, 247, 14,  16, 2,   23,  0,    0,      0,         0,   "",           NULL,          NULL },

	{d_agup_text_proc,      10,  292, 64,  8,  2,   23,  0,    0,      0,         0,   "Slide:",     NULL,          NULL },
	{d_agup_text_proc,      10,  312, 64,  8,  2,   23,  0,    0,      0,         0,   "Mute:",      NULL,          NULL },
	{d_agup_radio_proc,		10,  272, 38,  16, 2,   23,  0,    0,      1,         0,   "HO",         NULL,          NULL },
	{d_agup_radio_proc,		58,  272, 38,  16, 2,   23,  0,    0,      1,         0,   "PO",         NULL,          NULL },
	{d_agup_radio_proc,		102, 272, 45,  16, 2,   23,  0,    0,      1,         0,   "Tap",        NULL,          NULL },
	{d_agup_radio_proc,		154, 272, 50,  16, 2,   23,  0,    0,      1,         0,   "None",       NULL,          NULL },
	{d_agup_radio_proc,		58,  292, 38,  16, 2,   23,  0,    0,      2,         0,   "Up",         NULL,          NULL },
	{d_agup_radio_proc,		102, 292, 54,  16, 2,   23,  0,    0,      2,         0,   "Down",       NULL,          NULL },
	{d_agup_radio_proc,		154, 292, 64,  16, 2,   23,  0,    0,      2,         0,   "Neither",    NULL,          NULL },
	{d_agup_radio_proc,		46,  312, 58,  16, 2,   23,  0,    0,      3,         0,   "String",     NULL,          NULL },
	{d_agup_radio_proc,		102, 312, 52,  16, 2,   23,  0,    0,      3,         0,   "Palm",       NULL,          NULL },
	{d_agup_radio_proc,		154, 312, 64,  16, 2,   23,  0,    0,      3,         0,   "Neither",    NULL,          NULL },

	{d_agup_button_proc,    20,  340, 68,  28, 2,   23,  '\r', D_EXIT, 0,         0,   "OK",         NULL,          NULL },
	{d_agup_button_proc,    140, 340, 68,  28, 2,   23,  0,    D_EXIT, 0,         0,   "Cancel",     NULL,          NULL },
	{NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

int eof_menu_note_edit_pro_guitar_note(void)
{
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	unsigned long ctr, ctr2, fretcount, i;
	char undo_made = 0;	//Set to nonzero when an undo state is created
	long fretvalue;
	char allmuted;					//Used to track whether all used strings are string muted
	unsigned long bitmask = 0;		//Used to build the updated pro guitar note bitmask
	unsigned char legacymask;		//Used to build the updated legacy note bitmask
	unsigned char ghostmask;		//Used to build the updated ghost bitmask
	unsigned long flags;			//Used to build the updated flag bitmask
	char namechanged = 0, legacymaskchanged = 0;
	EOF_PRO_GUITAR_NOTE junknote;	//Just used with sizeof() to get the name string's length to guarantee a safe string copy
	char *newname = NULL;
	char autoprompt[100] = {0};
	char previously_refused;
	char declined_list[EOF_MAX_NOTES] = {0};	//This lists all notes whose names/legacy bitmasks the user declined to have applied to the edited note
	char autobitmask[10] = {0};
	unsigned long index = 0;

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run unless the pro guitar track is active

	if(eof_selection.current >= eof_get_track_size(eof_song, eof_selected_track))
		return 1;	//Do not allow this function to run if a valid note isn't selected

	if(!eof_music_paused)
	{
		eof_music_play();
	}

	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	eof_color_dialog(eof_pro_guitar_note_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_pro_guitar_note_dialog);

//Update the note name text box to match the last selected note
	memcpy(eof_note_edit_name, eof_song->pro_guitar_track[tracknum]->note[eof_selection.current]->name, sizeof(eof_note_edit_name));

//Update the fret text boxes (listed from top to bottom as string 1 through string 6)
	fretcount = eof_count_track_lanes(eof_song, eof_selected_track);
	if(eof_legacy_view)
	{	//Special case:  If legacy view is enabled, correct fretcount
		fretcount = eof_song->pro_guitar_track[tracknum]->numstrings;
	}
	for(ctr = 0, bitmask = 1; ctr < 6; ctr++, bitmask<<=1)
	{	//For each of the 6 supported strings
		if(ctr < fretcount)
		{	//If this track uses this string, copy the fret value to the appropriate string
			eof_pro_guitar_note_dialog[4 + (2 * ctr)].flags = 0;	//Ensure this text box is enabled
			if(eof_song->pro_guitar_track[tracknum]->note[eof_selection.current]->note & bitmask)
			{	//If this string is already defined as being in use, copy its fret value to the string
				if(eof_song->pro_guitar_track[tracknum]->note[eof_selection.current]->frets[ctr] == 0xFF)
				{	//If this string is muted
					snprintf(eof_fret_strings[ctr], 3, "X");
				}
				else
				{
					snprintf(eof_fret_strings[ctr], 3, "%d", eof_song->pro_guitar_track[tracknum]->note[eof_selection.current]->frets[ctr]);
				}
			}
			else
			{	//Otherwise empty the string
				eof_fret_strings[ctr][0] = '\0';
			}
		}
		else
		{	//Otherwise disable the text box for this fret and empty the string
			eof_pro_guitar_note_dialog[4 + (2 * ctr)].flags = D_DISABLED;	//Ensure this text box is disabled
			eof_fret_strings[ctr][0] = '\0';
		}
	}

//Update the legacy bitmask checkboxes
	legacymask = eof_song->pro_guitar_track[tracknum]->note[eof_selection.current]->legacymask;
	eof_pro_guitar_note_dialog[17].flags = (legacymask & 16) ? D_SELECTED : 0;
	eof_pro_guitar_note_dialog[18].flags = (legacymask & 8) ? D_SELECTED : 0;
	eof_pro_guitar_note_dialog[19].flags = (legacymask & 4) ? D_SELECTED : 0;
	eof_pro_guitar_note_dialog[20].flags = (legacymask & 2) ? D_SELECTED : 0;
	eof_pro_guitar_note_dialog[21].flags = (legacymask & 1) ? D_SELECTED : 0;

//Update the ghost bitmask checkboxes
	ghostmask = eof_song->pro_guitar_track[tracknum]->note[eof_selection.current]->ghost;
	eof_pro_guitar_note_dialog[23].flags = (ghostmask & 32) ? D_SELECTED : 0;
	eof_pro_guitar_note_dialog[24].flags = (ghostmask & 16) ? D_SELECTED : 0;
	eof_pro_guitar_note_dialog[25].flags = (ghostmask & 8) ? D_SELECTED : 0;
	eof_pro_guitar_note_dialog[26].flags = (ghostmask & 4) ? D_SELECTED : 0;
	eof_pro_guitar_note_dialog[27].flags = (ghostmask & 2) ? D_SELECTED : 0;
	eof_pro_guitar_note_dialog[28].flags = (ghostmask & 1) ? D_SELECTED : 0;

//Update the note flag radio buttons
	for(ctr = 0; ctr < 10; ctr++)
	{	//Clear each of the 10 status radio buttons
		eof_pro_guitar_note_dialog[31 + ctr].flags = 0;
	}
	flags = eof_song->pro_guitar_track[tracknum]->note[eof_selection.current]->flags;
	if(flags & EOF_PRO_GUITAR_NOTE_FLAG_HO)
	{	//Select "HO"
		eof_pro_guitar_note_dialog[31].flags = D_SELECTED;
	}
	else if(flags & EOF_PRO_GUITAR_NOTE_FLAG_PO)
	{	//Select "PO"
		eof_pro_guitar_note_dialog[32].flags = D_SELECTED;
	}
	else if(flags & EOF_PRO_GUITAR_NOTE_FLAG_TAP)
	{	//Select "Tap"
		eof_pro_guitar_note_dialog[33].flags = D_SELECTED;
	}
	else
	{	//Select "None"
		eof_pro_guitar_note_dialog[34].flags = D_SELECTED;
	}
	if(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP)
	{	//Select "Up"
		eof_pro_guitar_note_dialog[35].flags = D_SELECTED;
	}
	else if(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN)
	{	//Select "Down"
		eof_pro_guitar_note_dialog[36].flags = D_SELECTED;
	}
	else
	{	//Select "Neither"
		eof_pro_guitar_note_dialog[37].flags = D_SELECTED;
	}
	if(flags & EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE)
	{	//Select "String"
		eof_pro_guitar_note_dialog[38].flags = D_SELECTED;
	}
	else if(flags &EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE)
	{	//Select "Palm"
		eof_pro_guitar_note_dialog[39].flags = D_SELECTED;
	}
	else
	{	//Select "Neither"
		eof_pro_guitar_note_dialog[40].flags = D_SELECTED;
	}

	bitmask = 0;
	if(eof_popup_dialog(eof_pro_guitar_note_dialog, 0) == 41)
	{	//If user clicked OK
		//Validate and store the input
		if(eof_count_selected_notes(NULL, 0) > 1)
		{	//If multiple notes are selected, warn the user
			if(alert(NULL, "Warning:  These fret values will be applied to all selected notes.", NULL, "&OK", "&Cancel", 0, 0) == 2)
			{	//If user opts to cancel the operation
				return 1;
			}
		}

		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//For each note in the track
			namechanged = 0;	//Reset this status
			if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
			{	//If the note is in the active instrument difficulty and is selected
//Save the updated note name  (listed from top to bottom as string 1 through string 6)
				if((!undo_made) && ustrcmp(eof_note_edit_name, eof_song->pro_guitar_track[tracknum]->note[i]->name))
				{	//If the name was changed
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					undo_made = 1;
					memcpy(eof_song->pro_guitar_track[tracknum]->note[i]->name, eof_note_edit_name, sizeof(eof_note_edit_name));
					namechanged = 1;
				}

				for(ctr = 0, allmuted = 1; ctr < 6; ctr++)
				{	//For each of the 6 supported strings
					if(eof_fret_strings[ctr][0] != '\0')
					{	//If this string isn't empty, set the fret value
						bitmask |= (1 << ctr);	//Set the appropriate bit for this lane
						if(toupper(eof_fret_strings[ctr][0]) == 'X')
						{	//If the user defined this string as muted
							fretvalue = 0xFF;
						}
						else
						{	//Get the appropriate fret value
							fretvalue = atol(eof_fret_strings[ctr]);
							if((fretvalue < 0) || (fretvalue > eof_song->pro_guitar_track[tracknum]->numfrets))
							{	//If the conversion to number failed, or an invalid fret number was entered, enter a value of (muted) for the string
								fretvalue = 0xFF;
							}
						}
						if(!undo_made && (fretvalue != eof_song->pro_guitar_track[tracknum]->note[eof_selection.current]->frets[ctr]))
						{	//If an undo state hasn't been made yet, and this fret value changed
							eof_prepare_undo(EOF_UNDO_TYPE_NONE);
							undo_made = 1;
						}
						eof_song->pro_guitar_track[tracknum]->note[i]->frets[ctr] = fretvalue;
						if(fretvalue != 0xFF)
						{	//Track whether the all used strings in this note/chord are muted
							allmuted = 0;
						}
					}
					else
					{	//Clear the fret value and return the fret back to its default of 0 (open)
						bitmask &= ~(1 << ctr);	//Clear the appropriate bit for this lane
						if(!undo_made && (eof_song->pro_guitar_track[tracknum]->note[i]->frets[ctr] != 0))
						{	//If an undo state hasn't been made yet, and this fret value changed
							eof_prepare_undo(EOF_UNDO_TYPE_NONE);
							undo_made = 1;
						}
						eof_song->pro_guitar_track[tracknum]->note[i]->frets[ctr] = 0;
					}
				}//For each of the 6 supported strings
				if(bitmask == 0)
				{	//If edits results in this note having no played strings
					eof_track_delete_note(eof_song, eof_selected_track, i);	//Delete this note because it no longer exists
					i--;		//Compensate for the counter increment, since note[i] already references the next note
					continue;	//Skip the rest of the processing for this note
				}
//Save the updated note bitmask
				if(!undo_made && (eof_song->pro_guitar_track[tracknum]->note[i]->note != bitmask))
				{	//If an undo state hasn't been made yet, and the note bitmask changed
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					undo_made = 1;
				}
				eof_song->pro_guitar_track[tracknum]->note[i]->note = bitmask;

//Save the updated legacy note bitmask
				legacymask = 0;
				if(eof_pro_guitar_note_dialog[17].flags == D_SELECTED)
					legacymask |= 16;
				if(eof_pro_guitar_note_dialog[18].flags == D_SELECTED)
					legacymask |= 8;
				if(eof_pro_guitar_note_dialog[19].flags == D_SELECTED)
					legacymask |= 4;
				if(eof_pro_guitar_note_dialog[20].flags == D_SELECTED)
					legacymask |= 2;
				if(eof_pro_guitar_note_dialog[21].flags == D_SELECTED)
					legacymask |= 1;
				if(!undo_made && (legacymask != eof_song->pro_guitar_track[tracknum]->note[i]->legacymask))
				{
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					undo_made = 1;
					legacymaskchanged = 1;
				}
				eof_song->pro_guitar_track[tracknum]->note[i]->legacymask = legacymask;

//Save the updated ghost bitmask
				ghostmask = 0;
				if(eof_pro_guitar_note_dialog[23].flags == D_SELECTED)
					ghostmask |= 32;
				if(eof_pro_guitar_note_dialog[24].flags == D_SELECTED)
					ghostmask |= 16;
				if(eof_pro_guitar_note_dialog[25].flags == D_SELECTED)
					ghostmask |= 8;
				if(eof_pro_guitar_note_dialog[26].flags == D_SELECTED)
					ghostmask |= 4;
				if(eof_pro_guitar_note_dialog[27].flags == D_SELECTED)
					ghostmask |= 2;
				if(eof_pro_guitar_note_dialog[28].flags == D_SELECTED)
					ghostmask |= 1;
				if(!undo_made && (ghostmask != eof_song->pro_guitar_track[tracknum]->note[i]->ghost))
				{
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					undo_made = 1;
				}
				eof_song->pro_guitar_track[tracknum]->note[i]->ghost = ghostmask;

//Save the updated note flag bitmask
				flags = 0;
				if(eof_pro_guitar_note_dialog[31].flags == D_SELECTED)
				{	//HO is selected
					flags |= EOF_PRO_GUITAR_NOTE_FLAG_HO;	//Set the hammer on flag
					flags &= (~EOF_NOTE_FLAG_NO_HOPO);		//Clear the forced HOPO off note
					flags |= EOF_NOTE_FLAG_F_HOPO;			//Set the legacy HOPO flag
				}
				else if(eof_pro_guitar_note_dialog[32].flags == D_SELECTED)
				{	//PO is selected
					flags |= EOF_PRO_GUITAR_NOTE_FLAG_PO;	//Set the pull off flag
					flags &= (~EOF_NOTE_FLAG_NO_HOPO);		//Clear the forced HOPO off flag
					flags |= EOF_NOTE_FLAG_F_HOPO;			//Set the legacy HOPO flag
				}
				else if(eof_pro_guitar_note_dialog[33].flags == D_SELECTED)
				{	//Tap is selected
					flags |= EOF_PRO_GUITAR_NOTE_FLAG_TAP;	//Set the tap flag
					flags &= (~EOF_NOTE_FLAG_NO_HOPO);		//Clear the forced HOPO off flag
					flags |= EOF_NOTE_FLAG_F_HOPO;			//Set the legacy HOPO flag
				}
				if(eof_pro_guitar_note_dialog[35].flags == D_SELECTED)
				{	//Up is selected
					flags |= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP;
				}
				else if(eof_pro_guitar_note_dialog[36].flags == D_SELECTED)
				{	//Down is selected
					flags |= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN;
				}
				if(eof_pro_guitar_note_dialog[38].flags == D_SELECTED)
				{	//String is selected
					flags |= EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE;
				}
				else if(eof_pro_guitar_note_dialog[39].flags == D_SELECTED)
				{	//Palm is selected
					flags |= EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE;
				}
				if(!allmuted)
				{	//If any used strings in this note/chord weren't string muted
					flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE);	//Clear the string mute flag
				}
				else if(!(flags & EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE))
				{	//If all strings are muted and the user didn't specify a palm mute
					flags |= EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE;		//Set the string mute flag
				}
				flags |= (eof_song->pro_guitar_track[tracknum]->note[i]->flags & EOF_NOTE_FLAG_CRAZY);	//Retain the note's original crazy status
				if(!undo_made && (flags != eof_song->pro_guitar_track[tracknum]->note[i]->flags))
				{
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					undo_made = 1;
				}
				eof_song->pro_guitar_track[tracknum]->note[i]->flags = flags;

//Prompt whether matching notes need to have their name updated
				newname = eof_get_note_name(eof_song, eof_selected_track, i);
				if(newname != NULL)
				{
					if(newname[0] != '\0')
					{	//If the note name contains text
						for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
						{	//For each note in the active track
							if((ctr != i) && (eof_note_compare(eof_selected_track, i, ctr) == 0))
							{	//If this note isn't the one that was just edited, but it matches it
								if(ustrcmp(newname, eof_get_note_name(eof_song, eof_selected_track, ctr)))
								{	//If the two notes have different names
									if(alert(NULL, "Update other matching notes in this track to have the same name?", NULL, "&Yes", "&No", 'y', 'n') == 1)
									{	//If the user opts to use the updated note name on matching notes in this track
										for(; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
										{	//For each note in the active track, starting from the one that just matched the comparison
											if((ctr != i) && (eof_note_compare(eof_selected_track, i, ctr) == 0))
											{	//If this note isn't the one that was just edited, but it matches it, copy the edited note's name to this note
												ustrncpy(eof_get_note_name(eof_song, eof_selected_track, ctr), newname, sizeof(junknote.name));
											}
										}
									}
									break;	//Break from loop
								}
							}
						}//For each note in the active track
					}//If the note name contains text

//Or prompt whether this note's name should be updated from the other existing notes
					else
					{	//The note name is not defined
						memset(declined_list, 0, sizeof(declined_list));	//Clear the declined list
						for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
						{	//For each note in the active track
							if((ctr != i) && (eof_note_compare(eof_selected_track, i, ctr) == 0))
							{	//If this note isn't the one that was just edited, but it matches it
								newname = eof_get_note_name(eof_song, eof_selected_track, ctr);
								if(newname && (newname[0] != '\0'))
								{	//If this note has a name
									previously_refused = 0;
									for(ctr2 = 0; ctr2 < ctr; ctr2++)
									{	//For each previous note that was checked
										if(declined_list[ctr2] && !ustrcmp(newname, eof_get_note_name(eof_song, eof_selected_track, ctr2)))
										{	//If this note name matches one the user previously rejected to assign to the edited note
											declined_list[ctr] = 1;	//Automatically decline this instance of the same name
											previously_refused = 1;
											break;
										}
									}
									if(!previously_refused)
									{	//If this name isn't one the user already refused
										snprintf(autoprompt, sizeof(autoprompt), "Set this note's name to \"%s\"?",newname);
										if(alert(NULL, autoprompt, NULL, "&Yes", "&No", 'y', 'n') == 1)
										{	//If the user opts to assign this note's name to the edited note
											ustrncpy(eof_get_note_name(eof_song, eof_selected_track, i), newname, sizeof(junknote.name));
											break;
										}
										else
										{	//Otherwise mark this note's name as refused
											declined_list[ctr] = 1;	//Mark this note's name as having been declined
										}
									}
								}//If this note has a name
							}//If this note isn't the one that was just edited, but it matches it
						}//For each note in the active track
					}//The note name is not defined
				}//if(newname != NULL)

//Prompt whether matching notes need to have their legacy bitmask updated
				if(legacymask)
				{	//If the legacy bitmask isn't all lanes blank
					for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
					{	//For each note in the active track
						if((ctr != i) && (eof_get_note_type(eof_song, eof_selected_track, ctr) == eof_note_type) && (eof_note_compare(eof_selected_track, i, ctr) == 0))
						{	//If this note isn't the one that was just edited, but it matches it and is in the active track difficulty
							if(legacymask != eof_song->pro_guitar_track[tracknum]->note[ctr]->legacymask)
							{	//If the two notes have different legacy bitmasks
								if(alert(NULL, "Update other matching notes in this track difficulty to have the same legacy bitmask?", NULL, "&Yes", "&No", 'y', 'n') == 1)
								{	//If the user opts to use the updated note legacy bitmask on matching notes in this track
									for(; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
									{	//For each note in the active track, starting from the one that just matched the comparison
										if((ctr != i) && (eof_get_note_type(eof_song, eof_selected_track, ctr) == eof_note_type) && (eof_note_compare(eof_selected_track, i, ctr) == 0))
										{	//If this note isn't the one that was just edited, but it matches it and is in the active track difficulty, copy the edited note's legacy bitmask to this note
											eof_song->pro_guitar_track[tracknum]->note[ctr]->legacymask = legacymask;
										}
									}
								}
								break;	//Break from loop
							}
						}
					}//For each note in the active track
				}//If the legacy bitmask isn't all lanes blank

//Or prompt whether this note' legacy bitmask should be updated from the other existing notes
				else
				{	//The note's legacy bitmask is not defined
					memset(declined_list, 0, sizeof(declined_list));	//Clear the declined list
					for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
					{	//For each note in the active track
						if((ctr != i) && (eof_note_compare(eof_selected_track, i, ctr) == 0))
						{	//If this note isn't the one that was just edited, but it matches it
							legacymask = eof_song->pro_guitar_track[tracknum]->note[ctr]->legacymask;
							if(legacymask)
							{	//If this note has a legacy bitmask
								previously_refused = 0;
								for(ctr2 = 0; ctr2 < ctr; ctr2++)
								{	//For each previous note that was checked
									if(declined_list[ctr2] && (legacymask == eof_song->pro_guitar_track[tracknum]->note[ctr2]->legacymask))
									{	//If this note's legacy mask matches one the user previously rejected to assign to the edited note
										declined_list[ctr] = 1;	//Automatically decline this instance of the same legacy bitmask
										previously_refused = 1;
										break;
									}
								}
								if(!previously_refused)
								{	//If this legacy bitmask isn't one the user already refused
									for(ctr2 = 0, bitmask = 1, index = 0; ctr2 < 5; ctr2++, bitmask<<=1)
									{	//For each of the legacy bitmasks 5 usable bits
										if(legacymask & bitmask)
										{	//If this bit is set, append the fret number to the autobitmask string
											autobitmask[index++] = '1' + ctr2;
										}
										autobitmask[index] = '\0';	//Ensure the string is terminated
									}
									snprintf(autoprompt, sizeof(autoprompt), "Set this note's legacy bitmask to \"%s\"?",autobitmask);
									if(alert(NULL, autoprompt, NULL, "&Yes", "&No", 'y', 'n') == 1)
									{	//If the user opts to assign this note's legacy bitmask to the edited note
										eof_song->pro_guitar_track[tracknum]->note[i]->legacymask = legacymask;
										break;
									}
									else
									{	//Otherwise mark this note's name as refused
										declined_list[ctr] = 1;	//Mark this note's name as having been declined
									}
								}
							}//If this note has a legacy bitmask
						}//If this note isn't the one that was just edited, but it matches it
					}//For each note in the active track
				}//The note's legacy bitmask is not defined
			}//If the note is in the active instrument difficulty and is selected
		}//For each note in the track
	}//If user clicked OK

	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	return 1;
}

int eof_menu_note_toggle_tapping(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags;

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			flags ^= EOF_PRO_GUITAR_NOTE_FLAG_TAP;	//Toggle the tap flag
			if(flags & EOF_PRO_GUITAR_NOTE_FLAG_TAP)
			{	//If tapping was just enabled
				flags |= EOF_NOTE_FLAG_F_HOPO;			//Set the legacy HOPO on flag (no strum required for this note)
				flags &= ~(EOF_NOTE_FLAG_NO_HOPO);		//Clear the HOPO off flag
				flags &= ~(EOF_PRO_GUITAR_NOTE_FLAG_HO);	//Clear the hammer on flag
				flags &= ~(EOF_PRO_GUITAR_NOTE_FLAG_PO);	//Clear the pull off flag
			}
			else
			{	//If tapping was just disabled
				flags &= ~(EOF_NOTE_FLAG_F_HOPO);		//Clear the legacy HOPO on flag
			}
			if(!u)
			{	//Make a back up before changing the first note
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				u = 1;
			}
			eof_set_note_flags(eof_song, eof_selected_track, i, flags);
		}
	}
	return 1;
}

int eof_menu_note_remove_tapping(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags, oldflags;

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			oldflags = flags;							//Save an extra copy of the original flags
			flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_TAP);	//Clear the tap flag
			flags &= ~(EOF_NOTE_FLAG_F_HOPO);			//Clear the legacy HOPO on flag
			if(!u && (oldflags != flags))
			{	//Make a back up before changing the first note
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				u = 1;
			}
			eof_set_note_flags(eof_song, eof_selected_track, i, flags);
		}
	}
	return 1;
}

int eof_menu_note_toggle_slide_up(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags;

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			flags ^= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP;			//Toggle the slide up flag
			flags &= ~(EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN);	//Clear the slide down flag
			if(!u)
			{	//Make a back up before changing the first note
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				u = 1;
			}
			eof_set_note_flags(eof_song, eof_selected_track, i, flags);
		}
	}
	eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Fixup notes to adjust the slide note's length as appropriate
	return 1;
}

int eof_menu_note_toggle_slide_down(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags;

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			flags ^= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN;		//Toggle the slide down flag
			flags &= ~(EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP);		//Clear the slide down flag
			if(!u)
			{	//Make a back up before changing the first note
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				u = 1;
			}
			eof_set_note_flags(eof_song, eof_selected_track, i, flags);
		}
	}
	eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Fixup notes to adjust the slide note's length as appropriate
	return 1;
}

int eof_menu_note_remove_slide(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags, oldflags;

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			oldflags = flags;							//Save an extra copy of the original flags
			flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP);		//Clear the tap flag
			flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN);	//Clear the tap flag
			if(!u && (oldflags != flags))
			{	//Make a back up before changing the first note
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				u = 1;
			}
			eof_set_note_flags(eof_song, eof_selected_track, i, flags);
		}
	}
	return 1;
}

int eof_menu_note_toggle_palm_muting(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags;

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			flags ^= EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE;		//Toggle the palm mute flag
			flags &= ~(EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE);	//Clear the string mute flag
			if(!u)
			{	//Make a back up before changing the first note
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				u = 1;
			}
			eof_set_note_flags(eof_song, eof_selected_track, i, flags);
		}
	}
	return 1;
}

int eof_menu_note_remove_palm_muting(void)
{
	unsigned long i;
	long u = 0;
	unsigned long flags, oldflags;

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			oldflags = flags;								//Save an extra copy of the original flags
			flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE);	//Clear the palm mute flag
			if(!u && (oldflags != flags))
			{	//Make a back up before changing the first note
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				u = 1;
			}
			eof_set_note_flags(eof_song, eof_selected_track, i, flags);
		}
	}
	return 1;
}

int eof_menu_arpeggio_mark(void)
{
	unsigned long i, j;
	unsigned long sel_start = 0;
	unsigned long sel_end = 0;
	char firstnote = 0;						//Is set to nonzero when the first selected note in the active track difficulty is found
	char existingphrase = 0;				//Is set to nonzero if any selected notes are within an existing phrase
	unsigned long existingphrasenum = 0;	//Is set to the last arpeggio phrase number that encompasses existing notes
	unsigned long tracknum;
	unsigned long flags;

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run unless a pro guitar track is active

	//Find the start and end position of the collection of selected notes in the active difficulty
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{	//If the note is selected and is in the active track and difficulty
			if(firstnote == 0)
			{	//This is the first encountered selected note
				sel_start = eof_get_note_pos(eof_song, eof_selected_track, i);
				sel_end = sel_start;
				firstnote = 1;
			}
			else
			{
				if(eof_get_note_pos(eof_song, eof_selected_track, i) < sel_start)
				{
					sel_start = eof_get_note_pos(eof_song, eof_selected_track, i);
				}
				if(eof_get_note_pos(eof_song, eof_selected_track, i) + eof_get_note_length(eof_song, eof_selected_track, i) > sel_end)
				{
					sel_end = eof_get_note_pos(eof_song, eof_selected_track, i) + eof_get_note_length(eof_song, eof_selected_track, i);
				}
			}
		}
	}
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	for(j = 0; j < eof_song->pro_guitar_track[tracknum]->arpeggios; j++)
	{	//For each arpeggio section in the track
		if((sel_end >= eof_song->pro_guitar_track[tracknum]->arpeggio[j].start_pos) && (sel_start <= eof_song->pro_guitar_track[tracknum]->arpeggio[j].end_pos))
		{	//If the selection of notes is within this arpeggio's start and end position
			existingphrase = 1;	//Note it
			existingphrasenum = j;
		}
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	if(!existingphrase)
	{	//If the selected notes are not within an existing arpeggio phrase, create one
		eof_track_add_section(eof_song, eof_selected_track, EOF_ARPEGGIO_SECTION, 0, sel_start, sel_end, 0, NULL);
	}
	else
	{	//Otherwise edit the existing phrase
		eof_song->pro_guitar_track[tracknum]->arpeggio[existingphrasenum].start_pos = sel_start;
		eof_song->pro_guitar_track[tracknum]->arpeggio[existingphrasenum].end_pos = sel_end;
	}
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type) && ((eof_get_note_pos(eof_song, eof_selected_track, i) >= sel_start) && (eof_get_note_pos(eof_song, eof_selected_track, i) <= sel_end)))
		{	//If the note is in the active track difficulty and is within the created/edited phrase
			flags = eof_get_note_flags(eof_song, eof_selected_track, i);
			flags |= EOF_NOTE_FLAG_CRAZY;	//Set the note's crazy flag
			eof_set_note_flags(eof_song, eof_selected_track, i, flags);
		}
	}
	return 1;
}

int eof_menu_arpeggio_unmark(void)
{
	unsigned long i, j;
	unsigned long tracknum;

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Don't allow this function to run unless a pro guitar track is active

	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{	//If the note is selected and is in the active track difficulty
			for(j = 0; j < eof_song->pro_guitar_track[tracknum]->arpeggios; j++)
			{	//For each arpeggio section in the track
				if((eof_get_note_pos(eof_song, eof_selected_track, i) >= eof_song->pro_guitar_track[tracknum]->arpeggio[j].start_pos) && (eof_get_note_pos(eof_song, eof_selected_track, i) + eof_get_note_length(eof_song, eof_selected_track, i) <= eof_song->pro_guitar_track[tracknum]->arpeggio[j].end_pos))
				{	//If the note is encompassed within this arpeggio section
					eof_pro_guitar_track_delete_arpeggio(eof_song->pro_guitar_track[tracknum], j);	//Delete the arpeggio section
					break;
				}
			}
		}
	}
	return 1;
}

void eof_pro_guitar_track_delete_arpeggio(EOF_PRO_GUITAR_TRACK * tp, unsigned long index)
{
	unsigned long i;

	if(index >= tp->arpeggios)
		return;

	tp->arpeggio[index].name[0] = '\0';	//Empty the name string
	for(i = index; i < tp->arpeggios - 1; i++)
	{
		memcpy(&tp->arpeggio[i], &tp->arpeggio[i + 1], sizeof(EOF_PHRASE_SECTION));
	}
	tp->arpeggios--;
}

int eof_menu_arpeggio_erase_all(void)
{
	unsigned long ctr, tracknum;

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run unless a pro guitar track is active

	if(alert(NULL, "Erase all arpeggios from this track?", NULL, "&Yes", "&No", 'y', 'n') == 1)
	{
		tracknum = eof_song->track[eof_selected_track]->tracknum;
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		for(ctr = 0; ctr < eof_song->pro_guitar_track[tracknum]->arpeggios; ctr++)
		{	//For each arpeggio section in this track
			eof_song->pro_guitar_track[tracknum]->arpeggio[ctr].name[0] = '\0';	//Empty the name string
		}
		eof_song->pro_guitar_track[tracknum]->arpeggios = 0;
	}
	return 1;
}

int eof_menu_trill_mark(void)
{
	unsigned long i, j;
	unsigned long sel_start = 0;
	unsigned long sel_end = 0;
	char firstnote = 0;						//Is set to nonzero when the first selected note in the active track difficulty is found
	char existingphrase = 0;				//Is set to nonzero if any selected notes are within an existing phrase
	unsigned long existingphrasenum = 0;	//Is set to the last trill phrase number that encompasses existing notes
	unsigned long tracknum;
	EOF_PHRASE_SECTION *sectionptr;

	if((eof_song->track[eof_selected_track]->track_behavior != EOF_PRO_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_behavior != EOF_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR))
		return 1;	//Do not allow this function to run unless a pro/legacy guitar/bass/drum track is active

	//Find the start and end position of the collection of selected notes in the active difficulty
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{	//If the note is selected and is in the active track and difficulty
			if(firstnote == 0)
			{	//This is the first encountered selected note
				sel_start = eof_get_note_pos(eof_song, eof_selected_track, i);
				sel_end = sel_start;
				firstnote = 1;
			}
			else
			{
				if(eof_get_note_pos(eof_song, eof_selected_track, i) < sel_start)
				{
					sel_start = eof_get_note_pos(eof_song, eof_selected_track, i);
				}
				if(eof_get_note_pos(eof_song, eof_selected_track, i) + eof_get_note_length(eof_song, eof_selected_track, i) > sel_end)
				{
					sel_end = eof_get_note_pos(eof_song, eof_selected_track, i) + eof_get_note_length(eof_song, eof_selected_track, i);
				}
			}
		}
	}
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	for(j = 0; j < eof_get_num_trills(eof_song, eof_selected_track); j++)
	{	//For each trill section in the track
		sectionptr = eof_get_trill(eof_song, eof_selected_track, j);
		if((sel_end >= sectionptr->start_pos) && (sel_start <= sectionptr->end_pos))
		{	//If the selection of notes is within this trill section's start and end position
			existingphrase = 1;	//Note it
			existingphrasenum = j;
		}
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	if(!existingphrase)
	{	//If the selected notes are not within an existing trill phrase, create one
		eof_track_add_section(eof_song, eof_selected_track, EOF_TRILL_SECTION, 0, sel_start, sel_end, 0, NULL);
	}
	else
	{	//Otherwise edit the existing phrase
		sectionptr = eof_get_trill(eof_song, eof_selected_track, existingphrasenum);
		sectionptr->start_pos = sel_start;
		sectionptr->end_pos = sel_end;
	}
	eof_determine_phrase_status();
	return 1;
}

int eof_menu_tremolo_mark(void)
{
	unsigned long i, j;
	unsigned long sel_start = 0;
	unsigned long sel_end = 0;
	char firstnote = 0;						//Is set to nonzero when the first selected note in the active track difficulty is found
	char existingphrase = 0;				//Is set to nonzero if any selected notes are within an existing phrase
	unsigned long existingphrasenum = 0;	//Is set to the last tremolo phrase number that encompasses existing notes
	unsigned long tracknum;
	EOF_PHRASE_SECTION *sectionptr;

	if((eof_song->track[eof_selected_track]->track_behavior != EOF_PRO_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_behavior != EOF_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR))
		return 1;	//Do not allow this function to run unless a pro/legacy guitar/bass/drum track is active

	//Find the start and end position of the collection of selected notes in the active difficulty
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{	//If the note is selected and is in the active track and difficulty
			if(firstnote == 0)
			{	//This is the first encountered selected note
				sel_start = eof_get_note_pos(eof_song, eof_selected_track, i);
				sel_end = sel_start;
				firstnote = 1;
			}
			else
			{
				if(eof_get_note_pos(eof_song, eof_selected_track, i) < sel_start)
				{
					sel_start = eof_get_note_pos(eof_song, eof_selected_track, i);
				}
				if(eof_get_note_pos(eof_song, eof_selected_track, i) + eof_get_note_length(eof_song, eof_selected_track, i) > sel_end)
				{
					sel_end = eof_get_note_pos(eof_song, eof_selected_track, i) + eof_get_note_length(eof_song, eof_selected_track, i);
				}
			}
		}
	}
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	for(j = 0; j < eof_get_num_tremolos(eof_song, eof_selected_track); j++)
	{	//For each tremolo section in the track
		sectionptr = eof_get_tremolo(eof_song, eof_selected_track, j);
		if((sel_end >= sectionptr->start_pos) && (sel_start <= sectionptr->end_pos))
		{	//If the selection of notes is within this tremolo section's start and end position
			existingphrase = 1;	//Note it
			existingphrasenum = j;
		}
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	if(!existingphrase)
	{	//If the selected notes are not within an existing tremolo phrase, create one
		eof_track_add_section(eof_song, eof_selected_track, EOF_TREMOLO_SECTION, 0, sel_start, sel_end, 0, NULL);
	}
	else
	{	//Otherwise edit the existing phrase
		sectionptr = eof_get_tremolo(eof_song, eof_selected_track, existingphrasenum);
		sectionptr->start_pos = sel_start;
		sectionptr->end_pos = sel_end;
	}
	eof_determine_phrase_status();
	return 1;
}

int eof_menu_trill_unmark(void)
{
	unsigned long i, j;
	unsigned long tracknum;
	EOF_PHRASE_SECTION *sectionptr;

	if((eof_song->track[eof_selected_track]->track_behavior != EOF_PRO_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_behavior != EOF_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR))
		return 1;	//Do not allow this function to run unless a pro/legacy guitar/bass/drum track is active

	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{	//If the note is selected and is in the active track difficulty
			for(j = 0; j < eof_get_num_trills(eof_song, eof_selected_track); j++)
			{	//For each trill section in the track
				sectionptr = eof_get_trill(eof_song, eof_selected_track, j);
				if((eof_get_note_pos(eof_song, eof_selected_track, i) >= sectionptr->start_pos) && (eof_get_note_pos(eof_song, eof_selected_track, i) + eof_get_note_length(eof_song, eof_selected_track, i) <= sectionptr->end_pos))
				{	//If the note is encompassed within this trill section
					eof_track_delete_trill(eof_song, eof_selected_track, j);	//Delete the trill section
					break;
				}
			}
		}
	}
	eof_determine_phrase_status();
	return 1;
}

int eof_menu_tremolo_unmark(void)
{
	unsigned long i, j;
	unsigned long tracknum;
	EOF_PHRASE_SECTION *sectionptr;

	if((eof_song->track[eof_selected_track]->track_behavior != EOF_PRO_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_behavior != EOF_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR))
		return 1;	//Do not allow this function to run unless a pro/legacy guitar/bass/drum track is active

	eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{	//If the note is selected and is in the active track difficulty
			for(j = 0; j < eof_get_num_tremolos(eof_song, eof_selected_track); j++)
			{	//For each tremolo section in the track
				sectionptr = eof_get_tremolo(eof_song, eof_selected_track, j);
				if((eof_get_note_pos(eof_song, eof_selected_track, i) >= sectionptr->start_pos) && (eof_get_note_pos(eof_song, eof_selected_track, i) + eof_get_note_length(eof_song, eof_selected_track, i) <= sectionptr->end_pos))
				{	//If the note is encompassed within this tremolo section
					eof_track_delete_tremolo(eof_song, eof_selected_track, j);	//Delete the tremolo section
					break;
				}
			}
		}
	}
	eof_determine_phrase_status();
	return 1;
}

int eof_menu_trill_erase_all(void)
{
	char drum_track_prompt[] = "Erase all special drum rolls from this track?";
	char other_track_prompt[] = "Erase all trills from this track?";
	char *prompt = other_track_prompt;	//By default, assume a non drum track

	if((eof_song->track[eof_selected_track]->track_behavior != EOF_PRO_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_behavior != EOF_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR))
		return 1;	//Do not allow this function to run unless a pro/legacy guitar/bass/drum track is active

	if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
		prompt = drum_track_prompt;	//If this is a drum track, refer to the sections as "special drum rolls" instead of "trills"

	if(alert(NULL, prompt, NULL, "&Yes", "&No", 'y', 'n') == 1)
	{
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		eof_set_num_trills(eof_song, eof_selected_track, 0);
	}
	eof_determine_phrase_status();
	return 1;
}

int eof_menu_tremolo_erase_all(void)
{
	char drum_track_prompt[] = "Erase all drum rolls from this track?";
	char other_track_prompt[] = "Erase all tremolos from this track?";
	char *prompt = other_track_prompt;	//By default, assume a non drum track

	if((eof_song->track[eof_selected_track]->track_behavior != EOF_PRO_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_behavior != EOF_GUITAR_TRACK_BEHAVIOR) & (eof_song->track[eof_selected_track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR))
		return 1;	//Do not allow this function to run unless a pro/legacy guitar/bass/drum track is active

	if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
		prompt = drum_track_prompt;	//If this is a drum track, refer to the sections as "drum rolls" instead of "tremolos"

	if(alert(NULL, prompt, NULL, "&Yes", "&No", 'y', 'n') == 1)
	{
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		eof_set_num_tremolos(eof_song, eof_selected_track, 0);
	}
	eof_determine_phrase_status();
	return 1;
}

int eof_menu_note_clear_legacy_values(void)
{
	unsigned long i;
	long u = 0;
	unsigned long tracknum;

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 1;	//Do not allow this function to run when a pro guitar format track is not active

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the active track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
		{	//If this note is in the currently active track and is selected
			if(!u && eof_song->pro_guitar_track[tracknum]->note[i]->legacymask)
			{	//Make a back up before clearing the first legacy bitmask
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				u = 1;
			}
			eof_song->pro_guitar_track[tracknum]->note[i]->legacymask = 0;
		}
	}
	return 1;
}
