/*

Up/down arrow: Move forward and backward.
Left/right arrow: Change note spacement.
12345: Place notes.
Space: Play song.

I have Finnish keyboard so these might be different for you:
2 keys next to backspace: Change BPM. With shift, changes 0,01 instead of normal 1.
Key next to right shift: Change playback rate.

Shift + up: Choose notes.
Ctrl + up/down: Move selected notes forward/backward. If no notes is selected, move everything from ahead.
Ctrl + left/right: Same as above, but makes yellow blue, blue orange and so on. Kinda dangerous though, dB doesn't have Ctrl-z (EOF has?) so that might ruin everything.
Ctrl-c / Ctrl-v: copy/paste.

*/

#include <allegro.h>
#include <stdio.h>
#include "song.h"
#include "menu/edit.h"
#include "menu/song.h"
#include "menu/note.h"
#include "beat.h"
#include "main.h"
#include "dialog.h"
#include "player.h"
#include "editor.h"
#include "feedback.h"
#include "undo.h"
#include "foflc/Lyric_storage.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

int         eof_feedback_step = 0;
int         eof_feedback_selecting = 0;
int         eof_feedback_selecting_start_pos = 0;
int         eof_feedback_selecting_end_pos = 0;
EOF_NOTE *  eof_feedback_new_note = NULL;
int         eof_feedback_note[5] = {0};

int eof_feedback_any_note(void)
{
	eof_log("eof_feedback_any_note() entered", 1);

	int i;

	for(i = 0; i < 5; i++)
	{
		if(eof_feedback_note[i])
		{
			return 1;
		}
	}
	return 0;
}

/* read keys only available to editor (use after song is loaded */
void eof_editor_logic_feedback(void)
{
	eof_log("eof_editor_logic_feedback() entered", 1);

	unsigned long tracknum;
	long fbeat = 0;
	int fcpos[32];
	int fppos[32];
	int fsnap_divider[32] = {1, 2, 3, 4, 6, 8, 12}; // divide beat into this many portions
	int i;
	int npos;
	double bpm = 120.0;
	unsigned long cppqn = eof_song->beat[fbeat]->ppqn;

	if(!eof_song_loaded)
		return;

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	fbeat = eof_get_beat(eof_song, eof_music_pos - eof_av_delay);
	if(fbeat >= 0)
	{
		bpm = (double)60000000.0 / (double)eof_song->beat[fbeat]->ppqn;
	}

	if(key[KEY_SLASH])
	{
		if(eof_music_paused)
		{
			eof_menu_edit_playback_speed_helper_faster();
		}
		key[KEY_SLASH] = 0;
	}

	if(key[KEY_M])
	{
		eof_menu_edit_metronome();
		key[KEY_M] = 0;
	}
	if(key[KEY_C] && !KEY_EITHER_CTRL && !KEY_EITHER_SHIFT)
	{
		eof_menu_edit_claps();
		key[KEY_C] = 0;
	}
	if(KEY_EITHER_CTRL && !KEY_EITHER_SHIFT && key[KEY_C])
	{
		eof_menu_edit_copy();
		key[KEY_C] = 0;
	}
	if(KEY_EITHER_CTRL && !KEY_EITHER_SHIFT && key[KEY_V])
	{
		eof_menu_edit_paste();
		key[KEY_V] = 0;
	}

	/* play/pause music */
	if(key[KEY_SPACE] && eof_song_loaded)
	{
		if(KEY_EITHER_SHIFT)
		{
			eof_catalog_play();
		}
		else
		{
			if(eof_music_catalog_playback)
			{
				eof_music_catalog_playback = 0;
				eof_music_catalog_pos = eof_song->catalog->entry[eof_selected_catalog_entry].start_pos + eof_av_delay;
				eof_stop_midi();
				alogg_stop_ogg(eof_music_track);
				alogg_seek_abs_msecs_ogg(eof_music_track, eof_music_pos);
			}
			else
			{
				eof_music_play();
				if(eof_music_paused)
				{
					eof_menu_song_seek_rewind();
				}
			}
		}
		key[KEY_SPACE] = 0;
	}

	if(!eof_music_paused)
	{
		for(i = 0; i < eof_song->legacy_track[tracknum]->notes; i++)
		{
			if(eof_song->legacy_track[tracknum]->note[i]->type == eof_note_type)
			{
				npos = eof_song->legacy_track[tracknum]->note[i]->pos;
				if((eof_music_pos - eof_av_delay > npos) && (eof_music_pos - eof_av_delay < npos + (eof_song->legacy_track[tracknum]->note[i]->length > 100 ? eof_song->legacy_track[tracknum]->note[i]->length : 100)))
				{
					if(eof_hover_note != i)
					{
						eof_hover_note = i;
					}
					break;
				}
			}
		}
		return;
	}

	/* change quantization */
	if(key[KEY_LEFT])
	{
		if(KEY_EITHER_CTRL)
		{
			eof_menu_note_transpose_down();
		}
		else
		{
			eof_snap_mode--;
			if(eof_snap_mode < 1)
			{
				eof_snap_mode = 7;
			}
			eof_music_pos = eof_song->beat[fbeat]->pos + eof_av_delay;
			eof_feedback_step = 0;
		}
		key[KEY_LEFT] = 0;
	}
	if(key[KEY_RIGHT])
	{
		if(KEY_EITHER_CTRL)
		{
			eof_menu_note_transpose_up();
		}
		else
		{
			eof_snap_mode++;
			if(eof_snap_mode > 7)
			{
				eof_snap_mode = 1;
			}
			eof_music_pos = eof_song->beat[fbeat]->pos + eof_av_delay;
			eof_feedback_step = 0;
		}
		key[KEY_RIGHT] = 0;
	}

	/* change BPM */
	if(key[KEY_MINUS])
	{
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		eof_menu_edit_cut(fbeat + 1, 1);
		if(KEY_EITHER_SHIFT)
		{
			bpm -= 0.1;
		}
		else
		{
			bpm -= 1.0;
		}
		for(i = fbeat; i < EOF_MAX_BEATS; i++)
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
		eof_song->beat[fbeat]->flags = EOF_BEAT_FLAG_ANCHOR;
		eof_calculate_beats(eof_song);
		eof_menu_edit_cut_paste(fbeat + 1, 1);
		key[KEY_MINUS] = 0;
	}
	if(key[KEY_EQUALS])
	{
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		eof_menu_edit_cut(fbeat + 1, 1);
		if(KEY_EITHER_SHIFT)
		{
			bpm += 0.1;
		}
		else
		{
			bpm += 1.0;
		}
		for(i = fbeat; i < EOF_MAX_BEATS; i++)
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
		eof_song->beat[fbeat]->flags = EOF_BEAT_FLAG_ANCHOR;
		eof_calculate_beats(eof_song);
		eof_menu_edit_cut_paste(fbeat + 1, 1);
		key[KEY_EQUALS] = 0;
	}

	if(eof_snap_mode == 0)
	{
		eof_snap_mode = 1;
	}
	if(eof_music_pos - eof_av_delay < eof_song->beat[0]->pos)
	{
		eof_music_pos = eof_song->beat[0]->pos + eof_av_delay;
	}

	/* measure the length of the beat */
	eof_snap.length = eof_song->beat[fbeat + 1]->pos - eof_song->beat[fbeat]->pos;

	/* find snap positions */
	for(i = 0; i < fsnap_divider[eof_snap_mode - 1]; i++)
	{
		fcpos[i] = eof_song->beat[fbeat]->pos + ((float)eof_snap.length / (float)fsnap_divider[eof_snap_mode - 1]) * ((float)(i)) + eof_av_delay;
	}
	if(fbeat > 0)
	{
		for(i = 0; i < fsnap_divider[eof_snap_mode - 1]; i++)
		{
			fppos[i] = eof_song->beat[fbeat - 1]->pos + ((float)eof_snap.length / (float)fsnap_divider[eof_snap_mode - 1]) * ((float)(i)) + eof_av_delay;
		}
	}

	eof_hover_note = -1;
	eof_seek_hover_note = -1;
	for(i = 0; i < eof_song->legacy_track[tracknum]->notes; i++)
	{
		if((eof_song->legacy_track[tracknum]->note[i]->type == eof_note_type) && (eof_song->legacy_track[tracknum]->note[i]->pos > eof_music_pos - eof_av_delay - 5) && (eof_song->legacy_track[tracknum]->note[i]->pos < eof_music_pos - eof_av_delay + 5))
		{
			eof_hover_note = i;
			break;
		}
	}
	if(!KEY_EITHER_SHIFT)
	{
		eof_feedback_selecting = 0;
	}
	if(key[KEY_UP])
	{
		if(KEY_EITHER_CTRL)
		{
			eof_menu_note_push_back();
		}
		else
		{
			if(KEY_EITHER_SHIFT)
			{
				if(!eof_feedback_selecting)
				{
					eof_feedback_selecting = 1;
					eof_feedback_selecting_start_pos = eof_music_pos - eof_av_delay;
				}
				else
				{
					eof_feedback_selecting_end_pos = eof_music_pos - eof_av_delay;
				}
			}
			if(eof_snap_mode == 1)
			{
				eof_music_pos = eof_song->beat[fbeat + 1]->pos + eof_av_delay;
			}
			else
			{
				if(eof_feedback_step < fsnap_divider[eof_snap_mode - 1])
				{
					eof_feedback_step++;
					if(eof_feedback_step >= fsnap_divider[eof_snap_mode - 1])
					{
						eof_music_pos = eof_song->beat[fbeat + 1]->pos + eof_av_delay;
						eof_feedback_step = 0;
					}
					else
					{
						eof_music_pos = fcpos[eof_feedback_step];
					}
				}
			}
			if(KEY_EITHER_SHIFT)
			{
				if(eof_feedback_selecting)
				{
					eof_feedback_selecting = 1;
					eof_feedback_selecting_end_pos = eof_music_pos - eof_av_delay;
				}
			}
			if(eof_feedback_any_note() && eof_feedback_new_note)
			{
				eof_feedback_new_note->length = eof_music_pos - eof_av_delay - eof_feedback_new_note->pos;
			}
		}
		key[KEY_UP] = 0;
	}
	memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
	for(i = 0; i < eof_song->legacy_track[tracknum]->notes; i++)
	{
		if((eof_song->legacy_track[tracknum]->note[i]->type == eof_note_type) && (eof_song->legacy_track[tracknum]->note[i]->pos >= eof_feedback_selecting_start_pos) && (eof_song->legacy_track[tracknum]->note[i]->pos <= eof_feedback_selecting_end_pos))
		{
			eof_selection.multi[i] = 1;
		}
	}
	if(key[KEY_DOWN])
	{
		if(KEY_EITHER_CTRL)
		{
			eof_menu_note_push_up();
		}
		else if(eof_music_pos > eof_song->beat[0]->pos + eof_av_delay)
		{
			if(eof_snap_mode == 1)
			{
				if(fbeat > 0)
				{
					eof_music_pos = eof_song->beat[fbeat - 1]->pos + eof_av_delay;
				}
			}
			else
			{
				if(eof_feedback_step > 0)
				{
					eof_feedback_step--;
					eof_music_pos = fcpos[eof_feedback_step];
				}
				else
				{
					eof_feedback_step = fsnap_divider[eof_snap_mode - 1] - 1;
					eof_music_pos = fppos[eof_feedback_step];
				}
			}
			if(eof_feedback_any_note() && eof_feedback_new_note)
			{
				eof_feedback_new_note->length = eof_music_pos - eof_av_delay - eof_feedback_new_note->pos;
				if(eof_feedback_new_note->length < 1)
				{
					eof_feedback_new_note->length = 1;
				}
			}
		}
		key[KEY_DOWN] = 0;
	}
	eof_hover_note = -1;
	eof_seek_hover_note = -1;
	for(i = 0; i < eof_song->legacy_track[tracknum]->notes; i++)
	{
		if((eof_song->legacy_track[tracknum]->note[i]->type == eof_note_type) && (eof_song->legacy_track[tracknum]->note[i]->pos > eof_music_pos - eof_av_delay - 5) && (eof_song->legacy_track[tracknum]->note[i]->pos < eof_music_pos - eof_av_delay + 5))
		{
			eof_hover_note = i;
			break;
		}
	}
	if(key[KEY_1])
	{
		if(eof_hover_note < 0)
		{
			if(!eof_feedback_new_note)
			{
				eof_feedback_new_note = eof_legacy_track_add_note(eof_song->legacy_track[tracknum]);
				eof_legacy_track_note_create(eof_feedback_new_note, 1, 0, 0, 0, 0, 0, eof_music_pos - eof_av_delay, 1);
				eof_feedback_new_note->type = eof_note_type;
				eof_selection.current_pos = eof_feedback_new_note->pos;
				eof_selection.track = eof_selected_track;
				memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
				eof_sort_notes(eof_song);
				eof_fixup_notes(eof_song);
				eof_determine_phrase_status(eof_selected_track);
				eof_selection.multi[eof_selection.current] = 1;
			}
			eof_feedback_note[0] = 1;
		}
		else if(eof_feedback_note[0] == 0)
		{
			eof_feedback_new_note = eof_song->legacy_track[tracknum]->note[eof_hover_note];
			eof_song->legacy_track[tracknum]->note[eof_hover_note]->note ^= 1;
			eof_feedback_note[0] = 2;
		}
	}
	else
	{
		eof_feedback_note[0] = 0;
	}
	if(key[KEY_2])
	{
		if(eof_hover_note < 0)
		{
			if(!eof_feedback_new_note)
			{
				eof_feedback_new_note = eof_legacy_track_add_note(eof_song->legacy_track[tracknum]);
				eof_legacy_track_note_create(eof_feedback_new_note, 0, 1, 0, 0, 0, 0, eof_music_pos - eof_av_delay, 1);
				eof_feedback_new_note->type = eof_note_type;
				eof_selection.current_pos = eof_feedback_new_note->pos;
				eof_selection.track = eof_selected_track;
				memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
				eof_sort_notes(eof_song);
				eof_fixup_notes(eof_song);
				eof_determine_phrase_status(eof_selected_track);
				eof_selection.multi[eof_selection.current] = 1;
			}
			eof_feedback_note[1] = 1;
		}
		else if(eof_feedback_note[1] == 0)
		{
			eof_feedback_new_note = eof_song->legacy_track[tracknum]->note[eof_hover_note];
			eof_song->legacy_track[tracknum]->note[i]->note ^= 2;
			eof_feedback_note[1] = 2;
		}
	}
	else
	{
		eof_feedback_note[1] = 0;
	}
	if(key[KEY_3])
	{
		if(eof_hover_note < 0)
		{
			if(!eof_feedback_new_note)
			{
				eof_feedback_new_note = eof_legacy_track_add_note(eof_song->legacy_track[tracknum]);
				eof_legacy_track_note_create(eof_feedback_new_note, 0, 0, 1, 0, 0, 0, eof_music_pos - eof_av_delay, 1);
				eof_feedback_new_note->type = eof_note_type;
				eof_selection.current_pos = eof_feedback_new_note->pos;
				eof_selection.track = eof_selected_track;
				memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
				eof_sort_notes(eof_song);
				eof_fixup_notes(eof_song);
				eof_determine_phrase_status(eof_selected_track);
				eof_selection.multi[eof_selection.current] = 1;
			}
			eof_feedback_note[2] = 1;
		}
		else if(eof_feedback_note[2] == 0)
		{
			eof_feedback_new_note = eof_song->legacy_track[tracknum]->note[eof_hover_note];
			eof_song->legacy_track[tracknum]->note[i]->note ^= 4;
			eof_feedback_note[2] = 2;
		}
	}
	else
	{
		eof_feedback_note[2] = 0;
	}
	if(key[KEY_4])
	{
		if(eof_hover_note < 0)
		{
			if(!eof_feedback_new_note)
			{
				eof_feedback_new_note = eof_legacy_track_add_note(eof_song->legacy_track[tracknum]);
				eof_legacy_track_note_create(eof_feedback_new_note, 0, 0, 0, 1, 0, 0, eof_music_pos - eof_av_delay, 1);
				eof_feedback_new_note->type = eof_note_type;
				eof_selection.current_pos = eof_feedback_new_note->pos;
				eof_selection.track = eof_selected_track;
				memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
				eof_sort_notes(eof_song);
				eof_fixup_notes(eof_song);
				eof_determine_phrase_status(eof_selected_track);
				eof_selection.multi[eof_selection.current] = 1;
			}
			eof_feedback_note[3] = 1;
		}
		else if(eof_feedback_note[3] == 0)
		{
			eof_feedback_new_note = eof_song->legacy_track[tracknum]->note[eof_hover_note];
			eof_song->legacy_track[tracknum]->note[i]->note ^= 8;
			eof_feedback_note[3] = 2;
		}
	}
	else
	{
		eof_feedback_note[3] = 0;
	}
	if(key[KEY_5])
	{
		if(eof_hover_note < 0)
		{
			if(!eof_feedback_new_note)
			{
				eof_feedback_new_note = eof_legacy_track_add_note(eof_song->legacy_track[tracknum]);
				eof_legacy_track_note_create(eof_feedback_new_note, 0, 0, 0, 0, 1, 0, eof_music_pos - eof_av_delay, 1);
				eof_feedback_new_note->type = eof_note_type;
				eof_selection.current_pos = eof_feedback_new_note->pos;
				eof_selection.track = eof_selected_track;
				memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
				eof_sort_notes(eof_song);
				eof_fixup_notes(eof_song);
				eof_determine_phrase_status(eof_selected_track);
				eof_selection.multi[eof_selection.current] = 1;
			}
			eof_feedback_note[4] = 1;
		}
		else if(eof_feedback_note[4] == 0)
		{
			eof_feedback_new_note = eof_song->legacy_track[tracknum]->note[eof_hover_note];
			eof_song->legacy_track[tracknum]->note[eof_hover_note]->note ^= 16;
			eof_feedback_note[4] = 2;
		}
	}
	else
	{
		eof_feedback_note[4] = 0;
	}

	/* not placing notes */
	if(!key[KEY_1] && !key[KEY_2] && !key[KEY_3] && !key[KEY_4] && !key[KEY_5])
	{
		eof_feedback_new_note = NULL;
	}

	if((eof_hover_note >= 0) && !eof_song->legacy_track[tracknum]->note[eof_hover_note]->note)
	{
		eof_track_delete_note(eof_song, eof_selected_track, eof_hover_note);
		eof_sort_notes(eof_song);
		eof_fixup_notes(eof_song);
		eof_determine_phrase_status(eof_selected_track);
		eof_selection.current = EOF_MAX_NOTES - 1;
		eof_selection.multi[eof_hover_note] = 0;
	}
}
