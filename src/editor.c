#include <math.h>
#include "main.h"
#include "menu/file.h"
#include "menu/edit.h"
#include "menu/song.h"
#include "menu/note.h"
#include "menu/beat.h"
#include "menu/help.h"
#include "menu/context.h"
#include "foflc/Lyric_storage.h"
#include "player.h"
#include "mix.h"
#include "undo.h"
#include "dialog.h"
#include "beat.h"
#include "editor.h"
#include "utility.h"	//For eof_check_string()
#include "note.h"		//For EOF_LYRIC_PERCUSSION definition
#include "midi.h"
#include "waveform.h"

int         eof_held_1 = 0;
int         eof_held_2 = 0;
int         eof_held_3 = 0;
int         eof_held_4 = 0;
int         eof_held_5 = 0;
int         eof_entering_note = 0;
EOF_NOTE *  eof_entering_note_note = NULL;
EOF_LYRIC *  eof_entering_note_lyric = NULL;
int         eof_snote = 0;
int         eof_last_snote = 0;
int         eof_moving_anchor = 0;
int         eof_adjusted_anchor = 0;
int         eof_anchor_diff[EOF_TRACKS_MAX + 1] = {0};

EOF_SNAP_DATA eof_snap;
EOF_SNAP_DATA eof_tail_snap;

float eof_pos_distance(float p1, float p2)
{
	return sqrt((p1 - p2) * (p1 - p2));
}

void eof_select_beat(int beat)
{
	int i;
	int beat_counter = 0;
	char first_measure = 0;
	eof_selected_beat = beat;
	eof_selected_measure = -1;
	eof_beat_in_measure = 0;
	eof_beats_in_measure = 1;

	for(i = 0; i <= beat; i++)
	{
		if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_START_4_4)
		{
			eof_beats_in_measure = 4;
			beat_counter = 0;
			first_measure++;
		}
		else if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_START_3_4)
		{
			eof_beats_in_measure = 3;
			beat_counter = 0;
			first_measure++;
		}
		else if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_START_5_4)
		{
			eof_beats_in_measure = 5;
			beat_counter = 0;
			first_measure++;
		}
		else if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_START_6_4)
		{
			eof_beats_in_measure = 6;
			beat_counter = 0;
			first_measure++;
		}
		else if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_CUSTOM_TS)
		{
			eof_beats_in_measure = ((eof_song->beat[i]->flags & 0xFF000000)>>24) + 1;
			beat_counter = 0;
			first_measure++;
		}
		if(first_measure == 1)
		{
			eof_selected_measure = 0;
			first_measure++;
		}
		eof_beat_in_measure = beat_counter;
		if(first_measure && eof_beat_in_measure == 0)
		{
			eof_selected_measure++;
		}
		beat_counter++;
		if(beat_counter >= eof_beats_in_measure)
		{
			beat_counter = 0;
		}
	}
}

void eof_set_tail_pos(int note, unsigned long pos)
{
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;

	if(eof_song->track[eof_selected_track]->track_format == EOF_LEGACY_TRACK_FORMAT)
	{
		if(note < eof_song->legacy_track[tracknum]->notes)
		{
			eof_song->legacy_track[tracknum]->note[note]->length = pos - eof_song->legacy_track[tracknum]->note[note]->pos;
		}
	}
}

void eof_set_vocal_tail_pos(int note, unsigned long pos)
{
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;

	if(note < eof_song->vocal_track[tracknum]->lyrics)
	{
		eof_song->vocal_track[tracknum]->lyric[note]->length = pos - eof_song->vocal_track[tracknum]->lyric[note]->pos;
	}
}

void eof_get_snap_ts(EOF_SNAP_DATA * sp, int beat)
{
	int tsbeat = 0;
	int i;

	for(i = beat; i >= 0; i--)
	{
		if(eof_song->beat[i]->flags & (EOF_BEAT_FLAG_START_3_4 | EOF_BEAT_FLAG_START_4_4 | EOF_BEAT_FLAG_START_5_4 | EOF_BEAT_FLAG_START_6_4 | EOF_BEAT_FLAG_CUSTOM_TS))
		{
			tsbeat = i;
			break;
		}
	}

	/* set default denominator in case no TS flags found */
	sp->numerator = 4;
	sp->denominator = 4;

	/* all TS presets have a denominator of 4 */
	if(eof_song->beat[tsbeat]->flags & EOF_BEAT_FLAG_START_3_4)
	{
		sp->numerator = 3;
		sp->denominator = 4;
	}
	if(eof_song->beat[tsbeat]->flags & EOF_BEAT_FLAG_START_4_4)
	{
		sp->numerator = 4;
		sp->denominator = 4;
	}
	if(eof_song->beat[tsbeat]->flags & EOF_BEAT_FLAG_START_5_4)
	{
		sp->numerator = 5;
		sp->denominator = 4;
	}
	if(eof_song->beat[tsbeat]->flags & EOF_BEAT_FLAG_START_6_4)
	{
		sp->numerator = 6;
		sp->denominator = 4;
	}

	/* decode custom TS */
	else if(eof_song->beat[tsbeat]->flags & EOF_BEAT_FLAG_CUSTOM_TS)
	{
		sp->numerator = ((eof_song->beat[tsbeat]->flags & 0xFF000000)>>24) + 1;
		sp->denominator = ((eof_song->beat[tsbeat]->flags & 0x00FF0000)>>16) + 1;
	}
}

void eof_snap_logic(EOF_SNAP_DATA * sp, unsigned long p)
{
	int i;
	int interval = 0;
	char measure_snap = 0;
	int note = 4;

	/* place pen at "real" location and adjust from there */
	sp->pos = p;

	/* ensure pen is within the song boundaries */
	if(sp->pos < eof_song->tags->ogg[eof_selected_ogg].midi_offset)
	{
		sp->pos = eof_song->tags->ogg[eof_selected_ogg].midi_offset;
	}
	else if(sp->pos >= eof_music_length)
	{
		sp->pos = eof_music_length - 1;
	}

	if(eof_snap_mode != EOF_SNAP_OFF)
	{

		/* find the snap beat */
		sp->beat = -1;
		if(sp->pos < eof_song->beat[eof_song->beats - 1]->pos)
		{
			for(i = 0; i < eof_song->beats - 1; i++)
			{
				if((sp->pos >= eof_song->beat[i]->pos) && (sp->pos < eof_song->beat[i + 1]->pos))
				{
					sp->beat = i;
					break;
				}
			}
		}

		/* make sure we found a suitable snap beat before proceeding */
		if(sp->beat >= 0)
		{
			if((sp->beat >= eof_song->beats - 2) && (sp->beat >= 2))
			{	//Don't reference a negative index of eof_song->beat[]
				sp->beat_length = eof_song->beat[sp->beat - 1]->pos - eof_song->beat[sp->beat - 2]->pos;
			}
			else if(sp->beat + 1 < eof_song->beats)
			{	//Don't read out of bounds
				sp->beat_length = eof_song->beat[sp->beat + 1]->pos - eof_song->beat[sp->beat]->pos;
			}
			eof_get_snap_ts(sp, sp->beat);
			switch(eof_snap_mode)
			{
				case EOF_SNAP_QUARTER:
				{
					interval = 1;
					measure_snap = 0;
					note = 4;
					break;
				}
				case EOF_SNAP_EIGHTH:
				{
					interval = 2;
					measure_snap = 0;
					note = 8;
					break;
				}
				case EOF_SNAP_TWELFTH:
				{
					interval = 3;
					measure_snap = 0;
					note = 12;
					break;
				}
				case EOF_SNAP_SIXTEENTH:
				{
					interval = 4;
					measure_snap = 0;
					note = 16;
					break;
				}
				case EOF_SNAP_TWENTY_FOURTH:
				{
					interval = 6;
					measure_snap = 0;
					note = 24;
					break;
				}
				case EOF_SNAP_THIRTY_SECOND:
				{
					interval = 8;
					measure_snap = 0;
					note = 32;
					break;
				}
				case EOF_SNAP_FORTY_EIGHTH:
				{
					interval = 12;
					measure_snap = 0;
					note = 48;
					break;
				}
				case EOF_SNAP_CUSTOM:
				{
					interval = eof_snap_interval;
					break;
				}
			}
		}

		/* do the actual snapping */
		float least_amount = sp->beat_length;
		int least = -1;
		if(eof_snap_mode != EOF_SNAP_CUSTOM)
		{
			if(note < sp->denominator)
			{
				return;
			}
			if(sp->denominator == 2)
			{
				interval *= 2;
			}
			else if(sp->denominator == 8)
			{
				if(interval % 2)
				{
					return;
				}
				interval /= 2;
			}
			else if(sp->denominator == 16)
			{
				if(interval % 4)
				{
					return;
				}
				interval /= 4;
			}
		}
		if(measure_snap)
		{
			int ts = 1;

			/* find the measure we are currently in */
			sp->measure_beat = 0;
			for(i = sp->beat; i >= 0; i--)
			{
				if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_START_3_4)
				{
					ts = 3;
					sp->measure_beat = i;
					break;
				}
				else if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_START_4_4)
				{
					ts = 4;
					sp->measure_beat = i;
					break;
				}
				else if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_START_5_4)
				{
					ts = 5;
					sp->measure_beat = i;
					break;
				}
				else if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_START_6_4)
				{
					ts = 6;
					sp->measure_beat = i;
					break;
				}
				else if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_CUSTOM_TS)
				{
					ts = ((eof_song->beat[i]->flags & 0xFF000000)>>24) + 1;
					sp->measure_beat = i;
					break;
				}
			}
			for(i = sp->beat; i >= sp->measure_beat; i--)
			{

				/* start of measure we are currently in */
				if((i - sp->measure_beat) % ts == 0)
				{
					sp->measure_beat = i;
					break;
				}
			}

			/* add up beat times to find measure length */
			int bl; // current beat length
			if(sp->measure_beat + ts < eof_song->beats)
			{
				bl = eof_song->beat[sp->measure_beat + 1]->pos - eof_song->beat[sp->measure_beat]->pos;
				sp->measure_length = 0;
				for(i = sp->measure_beat; i < sp->measure_beat + ts; i++)
				{
					if(i < eof_song->beats - 1)
					{
						bl = eof_song->beat[i + 1]->pos - eof_song->beat[i]->pos;
					}
					sp->measure_length += bl;
				}
			}
			else
			{
				bl = sp->beat_length * ts;
			}

			/* find the snap positions */
			for(i = 0; i < eof_snap_interval; i++)
			{
				sp->grid_pos[i] = eof_song->beat[sp->measure_beat]->pos + (((float)sp->measure_length / (float)eof_snap_interval) * (float)i);
			}
			sp->grid_pos[eof_snap_interval] = eof_song->beat[sp->measure_beat]->pos + sp->measure_length;

			/* see which one we snap to */
			for(i = 0; i < eof_snap_interval + 1; i++)
			{
				sp->grid_distance[i] = eof_pos_distance(sp->grid_pos[i], sp->pos);
			}
			for(i = 0; i < eof_snap_interval + 1; i++)
			{
				if(sp->grid_distance[i] < least_amount)
				{
					least = i;
					least_amount = sp->grid_distance[i];
				}
			}
			if(least >= 0)
			{
				sp->pos = sp->grid_pos[least];
			}
		}
		else
		{
			/* find the snap positions */
			for(i = 0; i < interval; i++)
			{
				sp->grid_pos[i] = eof_song->beat[sp->beat]->pos + (((float)sp->beat_length / (float)interval) * (float)i);
			}
			sp->grid_pos[interval] = eof_song->beat[sp->beat + 1]->pos;

			/* see which one we snap to */
			for(i = 0; i < interval + 1; i++)
			{
				sp->grid_distance[i] = eof_pos_distance(sp->grid_pos[i], sp->pos);
			}
			for(i = 0; i < interval + 1; i++)
			{
				if(sp->grid_distance[i] < least_amount)
				{
					least = i;
					least_amount = sp->grid_distance[i];
				}
			}
			if(least >= 0)
			{
				sp->pos = sp->grid_pos[least];
			}
		}
	}
}

void eof_snap_length_logic(EOF_SNAP_DATA * sp)
{
	if(eof_snap_mode != EOF_SNAP_OFF)
	{
		/* if snapped to the next beat, make sure length is calculated from that beat */
		if(sp->pos >= eof_song->beat[sp->beat + 1]->pos)
		{
			sp->beat = sp->beat + 1;
			if((sp->beat >= eof_song->beats - 2) && (sp->beat >= 2))
			{	//Don't reference a negative index of eof_song->beat[]
				sp->beat_length = eof_song->beat[sp->beat - 1]->pos - eof_song->beat[sp->beat - 2]->pos;
			}
			else if(sp->beat + 1 < eof_song->beats)
			{	//Don't read out of bounds
				sp->beat_length = eof_song->beat[sp->beat + 1]->pos - eof_song->beat[sp->beat]->pos;
			}
		}

		/* calculate the length of the snap position */
		switch(eof_snap_mode)
		{
			case EOF_SNAP_QUARTER:
			{
				sp->length = sp->beat_length;
				break;
			}
			case EOF_SNAP_EIGHTH:
			{
				sp->length = sp->beat_length / 2;
				break;
			}
			case EOF_SNAP_TWELFTH:
			{
				sp->length = sp->beat_length / 3;
				break;
			}
			case EOF_SNAP_SIXTEENTH:
			{
				sp->length = sp->beat_length / 4;
				break;
			}
			case EOF_SNAP_TWENTY_FOURTH:
			{
				sp->length = sp->beat_length / 6;
				break;
			}
			case EOF_SNAP_THIRTY_SECOND:
			{
				sp->length = sp->beat_length / 8;
				break;
			}
			case EOF_SNAP_FORTY_EIGHTH:
			{
				sp->length = sp->beat_length / 12;
				break;
			}
			case EOF_SNAP_CUSTOM:
			{
				if(eof_custom_snap_measure)
				{
					sp->length = ((double)sp->measure_length / (double)eof_snap_interval +0.5);	//Round up
				}
				else
				{
					sp->length = ((double)sp->beat_length / (double)eof_snap_interval +0.5);	//Round up
				}
				if(sp->length <= 0)
					sp->length = 1;	//Enforce a minimum grid snap length of 1ms
				break;
			}
		}
		if(eof_snap_mode != EOF_SNAP_CUSTOM)
		{
			if(sp->denominator == 2)
			{
				sp->length /= 2;
			}
			else if(sp->denominator == 8)
			{
				sp->length *= 2;
			}
			else if(sp->denominator == 16)
			{
				sp->length *= 4;
			}
		}
	}
	else
	{
		sp->length = 100;
	}
}

void eof_read_editor_keys(void)
{
	unsigned long i = 0;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	unsigned long bitmask = 0;	//Used for simplifying note placement logic
	EOF_NOTE * new_note = NULL;
	EOF_LYRIC * new_lyric = NULL;
	unsigned long numlanes = eof_count_track_lanes(eof_selected_track);

	eof_read_controller(&eof_guitar);
	eof_read_controller(&eof_drums);

	/* seek to beginning */
	if(key[KEY_HOME])
	{
		if(KEY_EITHER_CTRL)
		{
			eof_menu_song_seek_first_note();
		}
		else if(KEY_EITHER_SHIFT)
		{
			eof_menu_edit_select_previous();
		}
		else
		{
			eof_menu_song_seek_start();
		}
		key[KEY_HOME] = 0;
	}

	/* seek to end */
	if(key[KEY_END])
	{
		if(KEY_EITHER_CTRL)
		{
			eof_menu_song_seek_last_note();
		}
		else if(KEY_EITHER_SHIFT)
		{
			eof_menu_edit_select_rest();
		}
		else
		{
			eof_menu_song_seek_end();
		}
		key[KEY_END] = 0;
	}

	if(key[KEY_R] && !KEY_EITHER_CTRL)
	{
		eof_menu_song_seek_rewind();
		key[KEY_R] = 0;
	}

	if(key[KEY_PLUS_PAD])
	{
		eof_menu_edit_zoom_helper_in();
		key[KEY_PLUS_PAD] = 0;
	}
	if(key[KEY_MINUS_PAD])
	{
		eof_menu_edit_zoom_helper_out();
		key[KEY_MINUS_PAD] = 0;
	}
	if(key[KEY_Q])
	{
		eof_menu_catalog_show();
		key[KEY_Q] = 0;
	}
	if(key[KEY_W])
	{
		eof_menu_catalog_previous();
		key[KEY_W] = 0;
	}
	if(key[KEY_E])
	{
		if(KEY_EITHER_CTRL)
		{	//CTRL+E will toggle Expert+ double bass drum notation
			eof_menu_note_toggle_double_bass();
		}
		else
		{	//Otherwise E will cycle to the next catalog entry
			eof_menu_catalog_next();
		}
		key[KEY_E] = 0;
	}

	if(key[KEY_G])
	{
		if(KEY_EITHER_CTRL)
		{	//CTRL+G will toggle Pro green cymbal notation
			eof_menu_note_toggle_rb3_cymbal_green();
		}
		else
		{	//Otherwise G will toggle grid snap
			if(eof_snap_mode == EOF_SNAP_OFF)
			{
				eof_snap_mode = eof_last_snap_mode;
			}
			else
			{
				eof_last_snap_mode = eof_snap_mode;
				eof_snap_mode = EOF_SNAP_OFF;
			}
		}
		key[KEY_G] = 0;
	}
	if(key[KEY_Y] && KEY_EITHER_CTRL)
	{	//CTRL+Y will toggle Pro yellow cymbal notation
		eof_menu_note_toggle_rb3_cymbal_yellow();
		key[KEY_Y] = 0;
	}
	if(key[KEY_B] && KEY_EITHER_CTRL)
	{	//CTRL+B will toggle Pro blue cymbal notation
		eof_menu_note_toggle_rb3_cymbal_blue();
		key[KEY_B] = 0;
	}

	if(eof_music_paused)
	{
		if(key[KEY_SEMICOLON])
		{
			eof_menu_edit_playback_speed_helper_slower();
			key[KEY_SEMICOLON] = 0;
		}
		if(key[KEY_QUOTE])
		{
			eof_menu_edit_playback_speed_helper_faster();
			key[KEY_QUOTE] = 0;
		}
	}

	if(key[KEY_TAB])
	{
		/* change track */
		if(KEY_EITHER_CTRL)
		{	//Track numbering begins at 1 instead of 0
			eof_track_selected_menu[eof_selected_track-1].flags = 0;	//Clear the active track checkmark in the menu

			if(KEY_EITHER_SHIFT)	//Shift instrument back 1 number
			{
				if(eof_selected_track > EOF_TRACKS_MIN)
					eof_menu_track_selected_track_number(eof_selected_track-1);
				else
					eof_menu_track_selected_track_number(EOF_TRACKS_MAX);	//Wrap around
			}
			else					//Shift instrument forward 1 number
			{
				if(eof_selected_track < EOF_TRACKS_MAX)
					eof_menu_track_selected_track_number(eof_selected_track+1);
				else
					eof_menu_track_selected_track_number(EOF_TRACKS_MIN);	//Wrap around
			}

			if(eof_selected_track == EOF_TRACK_VOCALS)
				eof_vocals_selected = 1;
			else
				eof_vocals_selected = 0;

			eof_track_selected_menu[eof_selected_track-1].flags = D_SELECTED;	//Set the active track checkmark in the menu
			eof_detect_difficulties(eof_song);
			eof_fix_window_title();
			eof_mix_find_claps();
			eof_mix_start_helper();
		}

		/* or change difficulty */
		else
		{
			if(KEY_EITHER_SHIFT)
			{
				eof_note_type--;
				if(eof_note_type < 0)
				{
					eof_note_type = 3;
				}
				eof_mix_find_claps();
				eof_mix_start_helper();
			}
			else
			{
				eof_note_type++;
				if(eof_note_type > 3)
				{
					eof_note_type = 0;
				}
				eof_mix_find_claps();
				eof_mix_start_helper();
			}
			eof_detect_difficulties(eof_song);
		}
		key[KEY_TAB] = 0;
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
					if(eof_vocals_selected)
					{
						eof_vocal_track_fixup_lyrics(eof_song->vocal_track[tracknum], 1);
					}
					else
					{
						eof_legacy_track_fixup_notes(eof_song->legacy_track[tracknum], 1);
					}
					if(eof_undo_last_type == EOF_UNDO_TYPE_RECORD)
					{
						eof_undo_last_type = 0;
					}
				}
			}
		}
		key[KEY_SPACE] = 0;
	}

	if(key[KEY_LEFT])
	{
		eof_music_rewind();
	}
	if(key[KEY_RIGHT])
	{
		eof_music_forward();
	}
	if(key[KEY_PGUP])
	{
		if(!eof_music_catalog_playback)
		{
			if(KEY_EITHER_CTRL)
			{
				eof_menu_song_seek_previous_screen();
			}
			else if(KEY_EITHER_SHIFT)
			{
				eof_menu_song_seek_previous_note();
			}
			else
			{
				int b = eof_get_beat(eof_song, eof_music_pos - eof_av_delay);

				if(b > 0)
				{
					if(eof_song->beat[b]->pos == eof_music_pos - eof_av_delay)
					{
						alogg_seek_abs_msecs_ogg(eof_music_track, eof_song->beat[b - 1]->pos + eof_av_delay);
						eof_music_pos = eof_song->beat[b - 1]->pos + eof_av_delay;
					}
					else
					{
						alogg_seek_abs_msecs_ogg(eof_music_track, eof_song->beat[b]->pos + eof_av_delay);
						eof_music_pos = eof_song->beat[b]->pos + eof_av_delay;
					}
					eof_music_actual_pos = eof_music_pos;
					eof_mix_seek(eof_music_actual_pos);
				}
				else
				{
					eof_music_pos = eof_song->beat[0]->pos + eof_av_delay;
					eof_music_actual_pos = eof_music_pos;
					eof_mix_seek(eof_music_actual_pos);
				}
			}
		}
		key[KEY_PGUP] = 0;
	}
	if(key[KEY_PGDN])
	{
		if(!eof_music_catalog_playback)
		{
			if(KEY_EITHER_CTRL)
			{
				eof_menu_song_seek_next_screen();
			}
			else if(KEY_EITHER_SHIFT)
			{
				eof_menu_song_seek_next_note();
			}
			else
			{
				int b = eof_get_beat(eof_song, eof_music_pos - eof_av_delay);
				if(eof_music_pos - eof_av_delay < 0)
				{
					b = -1;
				}

				if((b < eof_song->beats - 1) && (eof_song->beat[b + 1]->pos < eof_music_actual_length))
				{
					alogg_seek_abs_msecs_ogg(eof_music_track, eof_song->beat[b + 1]->pos + eof_av_delay);
					eof_music_pos = eof_song->beat[b + 1]->pos + eof_av_delay;
					eof_music_actual_pos = eof_music_pos;
					eof_mix_seek(eof_music_actual_pos);
				}
			}
		}
		key[KEY_PGDN] = 0;
	}
	if(KEY_EITHER_SHIFT && key[KEY_UP])
	{
		if(eof_vocals_selected && (eof_vocals_offset < MAXPITCH - eof_screen_layout.vocal_view_size + 1))
		{
			if(KEY_EITHER_CTRL)
			{
				eof_vocals_offset += 12;
				if(eof_vocals_offset > MAXPITCH)
				{
					eof_vocals_offset = MAXPITCH;
				}
			}
			else
			{
				eof_vocals_offset++;
			}
		}
		key[KEY_UP] = 0;
	}
	else if(key[KEY_UP] && eof_music_paused && !eof_music_catalog_playback)
	{
		if(eof_vocals_selected && KEY_EITHER_CTRL)
		{
				eof_menu_note_transpose_up_octave();	//Move up 12 pitches at once, performing a single undo beforehand
		}
		else
		{
			eof_menu_note_transpose_up();
		}
		if(eof_vocals_selected && eof_mix_vocal_tones_enabled && (eof_selection.current < eof_song->vocal_track[tracknum]->lyrics) && (eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->note != EOF_LYRIC_PERCUSSION))
		{
			eof_mix_play_note(eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->note);
		}
		key[KEY_UP] = 0;
	}
	if(KEY_EITHER_SHIFT && key[KEY_DOWN])
	{
		if(eof_vocals_selected && eof_vocals_offset > MINPITCH)
		{
			if(KEY_EITHER_CTRL)
			{
				eof_vocals_offset -= 12;
				if(eof_vocals_offset < MINPITCH)
				{
					eof_vocals_offset = MINPITCH;
				}
			}
			else
			{
				eof_vocals_offset--;
			}
		}
		key[KEY_DOWN] = 0;
	}
	else if(key[KEY_DOWN] && eof_music_paused && !eof_music_catalog_playback)
	{
		if(eof_vocals_selected && KEY_EITHER_CTRL)
		{
				eof_menu_note_transpose_down_octave();	//Move down 12 pitches at once, performing a single undo beforehand
		}
		else
		{
			eof_menu_note_transpose_down();
		}
		if(eof_vocals_selected && eof_mix_vocal_tones_enabled && (eof_selection.current < eof_song->vocal_track[tracknum]->lyrics) && (eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->note != EOF_LYRIC_PERCUSSION))
		{
			eof_mix_play_note(eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->note);
		}
		key[KEY_DOWN] = 0;
	}
	if(key[KEY_A] && eof_music_paused && !KEY_EITHER_CTRL)
	{
		if(KEY_EITHER_SHIFT)
		{	//Beat>Anchor Beat
			eof_menu_beat_anchor();
		}
		else
		{	//Beat>Toggle Anchor
			eof_menu_beat_toggle_anchor();
		}
		key[KEY_A] = 0;
	}
	if(key[KEY_COMMA])
	{
		eof_snap_mode--;
		if(eof_snap_mode < 0)
		{
			eof_snap_mode = EOF_SNAP_FORTY_EIGHTH;
		}
		key[KEY_COMMA] = 0;
	}
	if(key[KEY_STOP])
	{
		eof_snap_mode++;
		if(eof_snap_mode > EOF_SNAP_FORTY_EIGHTH)
		{
			eof_snap_mode = 0;
		}
		key[KEY_STOP] = 0;
	}
	if(key[KEY_OPENBRACE] && eof_music_paused && !eof_music_catalog_playback)
	{
		if(eof_vocals_selected)
		{
			if(eof_count_selected_notes(NULL, 0) > 0)
			{
				eof_prepare_undo(EOF_UNDO_TYPE_NOTE_LENGTH);
			}
			if(eof_snap_mode == EOF_SNAP_OFF)
			{
				if(KEY_EITHER_CTRL)
				{
					for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
					{
						if((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i])
						{
							eof_song->vocal_track[tracknum]->lyric[i]->length -= 10;
						}
					}
				}
				else
				{
					for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
					{
						if((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i])
						{
							eof_song->vocal_track[tracknum]->lyric[i]->length -= 100;
						}
					}
				}
			}
			else
			{
				int b;
				for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
				{
					if((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i])
					{
						b = eof_get_beat(eof_song, eof_song->vocal_track[tracknum]->lyric[i]->pos + eof_song->vocal_track[tracknum]->lyric[i]->length - 1);
						if(b >= 0)
						{
							eof_snap_logic(&eof_tail_snap, eof_song->beat[b]->pos);
						}
						else
						{
							eof_snap_logic(&eof_tail_snap, eof_song->vocal_track[tracknum]->lyric[i]->pos + eof_song->vocal_track[tracknum]->lyric[i]->length - 1);
						}
						eof_snap_length_logic(&eof_tail_snap);
						eof_song->vocal_track[tracknum]->lyric[i]->length -= eof_tail_snap.length;
						if(eof_song->vocal_track[tracknum]->lyric[i]->length > 1)
						{
							eof_snap_logic(&eof_tail_snap, eof_song->vocal_track[tracknum]->lyric[i]->pos + eof_song->vocal_track[tracknum]->lyric[i]->length);
							eof_set_tail_pos(i, eof_tail_snap.pos);
						}
					}
				}
			}
			eof_vocal_track_fixup_lyrics(eof_song->vocal_track[tracknum], 1);
		}
		else
		{
			if(eof_count_selected_notes(NULL, 0) > 0)
			{
				eof_prepare_undo(EOF_UNDO_TYPE_NOTE_LENGTH);
			}
			if(eof_snap_mode == EOF_SNAP_OFF)
			{
				if(KEY_EITHER_CTRL)
				{
					for(i = 0; i < eof_song->legacy_track[tracknum]->notes; i++)
					{
						if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_song->legacy_track[tracknum]->note[i]->type == eof_note_type))
						{
							eof_song->legacy_track[tracknum]->note[i]->length -= 10;
						}
					}
				}
				else
				{
					for(i = 0; i < eof_song->legacy_track[tracknum]->notes; i++)
					{
						if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_song->legacy_track[tracknum]->note[i]->type == eof_note_type))
						{
							eof_song->legacy_track[tracknum]->note[i]->length -= 100;
						}
					}
				}
			}
			else
			{
				int b;
				for(i = 0; i < eof_song->legacy_track[tracknum]->notes; i++)
				{
					if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_song->legacy_track[tracknum]->note[i]->type == eof_note_type))
					{
						b = eof_get_beat(eof_song, eof_song->legacy_track[tracknum]->note[i]->pos + eof_song->legacy_track[tracknum]->note[i]->length - 1);
						if(b >= 0)
						{
							eof_snap_logic(&eof_tail_snap, eof_song->beat[b]->pos);
						}
						else
						{
							eof_snap_logic(&eof_tail_snap, eof_song->legacy_track[tracknum]->note[i]->pos + eof_song->legacy_track[tracknum]->note[i]->length - 1);
						}
						eof_snap_length_logic(&eof_tail_snap);
						eof_song->legacy_track[tracknum]->note[i]->length -= eof_tail_snap.length;
						if(eof_song->legacy_track[tracknum]->note[i]->length > 1)
						{
							eof_snap_logic(&eof_tail_snap, eof_song->legacy_track[tracknum]->note[i]->pos + eof_song->legacy_track[tracknum]->note[i]->length);
							eof_set_tail_pos(i, eof_tail_snap.pos);
						}
					}
				}
			}
			eof_legacy_track_fixup_notes(eof_song->legacy_track[tracknum], 1);
		}
		key[KEY_OPENBRACE] = 0;
	}
	if(key[KEY_CLOSEBRACE] && eof_music_paused && !eof_music_catalog_playback)
	{
		if(eof_vocals_selected)
		{
			if(eof_count_selected_notes(NULL, 0) > 0)
			{
				eof_prepare_undo(EOF_UNDO_TYPE_NOTE_LENGTH);
			}
			if(eof_snap_mode == EOF_SNAP_OFF)
			{
				if(KEY_EITHER_CTRL)
				{
					for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
					{
						if((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i])
						{
							eof_song->vocal_track[tracknum]->lyric[i]->length += 10;
						}
					}
				}
				else
				{
					for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
					{
						if((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i])
						{
							eof_song->vocal_track[tracknum]->lyric[i]->length += 100;
						}
					}
				}
			}
			else
			{
				for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
				{
					if((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i])
					{
						eof_snap_logic(&eof_tail_snap, eof_song->vocal_track[tracknum]->lyric[i]->pos + eof_song->vocal_track[tracknum]->lyric[i]->length + 1);
						if(eof_tail_snap.pos > eof_song->vocal_track[tracknum]->lyric[i]->pos + eof_song->vocal_track[tracknum]->lyric[i]->length + 1)
						{
							eof_set_tail_pos(i, eof_tail_snap.pos);
						}
						else
						{
							eof_snap_length_logic(&eof_tail_snap);
							eof_song->vocal_track[tracknum]->lyric[i]->length += eof_tail_snap.length;
							eof_snap_logic(&eof_tail_snap, eof_song->vocal_track[tracknum]->lyric[i]->pos + eof_song->vocal_track[tracknum]->lyric[i]->length);
							eof_set_vocal_tail_pos(i, eof_tail_snap.pos);
						}
					}
				}
			}
			eof_vocal_track_fixup_lyrics(eof_song->vocal_track[tracknum], 1);
		}//if(eof_vocals_selected)
		else
		{
			if(eof_count_selected_notes(NULL, 0) > 0)
			{
				eof_prepare_undo(EOF_UNDO_TYPE_NOTE_LENGTH);
			}
			if(eof_snap_mode == EOF_SNAP_OFF)
			{
				if(KEY_EITHER_CTRL)
				{
					for(i = 0; i < eof_song->legacy_track[tracknum]->notes; i++)
					{
						if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_song->legacy_track[tracknum]->note[i]->type == eof_note_type))
						{
							eof_song->legacy_track[tracknum]->note[i]->length += 10;
						}
					}
				}
				else
				{
					for(i = 0; i < eof_song->legacy_track[tracknum]->notes; i++)
					{
						if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_song->legacy_track[tracknum]->note[i]->type == eof_note_type))
						{
							eof_song->legacy_track[tracknum]->note[i]->length += 100;
						}
					}
				}
			}
			else
			{
				for(i = 0; i < eof_song->legacy_track[tracknum]->notes; i++)
				{
					if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_song->legacy_track[tracknum]->note[i]->type == eof_note_type))
					{
						eof_snap_logic(&eof_tail_snap, eof_song->legacy_track[tracknum]->note[i]->pos + eof_song->legacy_track[tracknum]->note[i]->length + 1);
						if(eof_tail_snap.pos > eof_song->legacy_track[tracknum]->note[i]->pos + eof_song->legacy_track[tracknum]->note[i]->length + 1)
						{
							eof_set_tail_pos(i, eof_tail_snap.pos);
						}
						else
						{
							eof_snap_length_logic(&eof_tail_snap);
							eof_song->legacy_track[tracknum]->note[i]->length += eof_tail_snap.length;
							eof_snap_logic(&eof_tail_snap, eof_song->legacy_track[tracknum]->note[i]->pos + eof_song->legacy_track[tracknum]->note[i]->length);
							eof_set_tail_pos(i, eof_tail_snap.pos);
						}
					}
				}
			}
			eof_legacy_track_fixup_notes(eof_song->legacy_track[tracknum], 1);
		}
		key[KEY_CLOSEBRACE] = 0;
	}

	if(key[KEY_T])
	{
		eof_menu_note_toggle_crazy();
		key[KEY_T] = 0;
	}

	/* select all */
	if(KEY_EITHER_CTRL && key[KEY_A] && eof_music_paused && !eof_music_catalog_playback)
	{
		eof_menu_edit_select_all();
		key[KEY_A] = 0;
	}

	if(key[KEY_L])
	{
		if(KEY_EITHER_CTRL && eof_music_paused && !eof_music_catalog_playback)
		{	/* select like */
			eof_menu_edit_select_like();
		}
		else if(eof_vocals_selected && (eof_selection.track == EOF_TRACK_VOCALS) && (eof_selection.current < eof_song->vocal_track[tracknum]->lyrics))
		{
			if(KEY_EITHER_SHIFT)
			{	//Split lyric
				eof_menu_split_lyric();
			}
			else
			{	//Edit lyric
				eof_edit_lyric_dialog();
			}
		}
		key[KEY_L] = 0;
	}

	if(key[KEY_F])
	{
		if(eof_vocals_selected)
		{	//Toggle freestyle
			eof_menu_toggle_freestyle();
		}
		key[KEY_F] = 0;
	}

	/* deselect all */
	if(KEY_EITHER_CTRL && key[KEY_D] && eof_music_paused && !eof_music_catalog_playback)
	{
		eof_menu_edit_deselect_all();
		key[KEY_D] = 0;
	}

	/* cycle HOPO status */
	if(key[KEY_H])
	{
		eof_menu_hopo_cycle();
		key[KEY_H]=0;
	}

	if(key[KEY_M])
	{	//Toggle metronome
		eof_menu_edit_metronome();
		key[KEY_M] = 0;
	}
	if(key[KEY_C] && !KEY_EITHER_CTRL)
	{	//Toggle claps
		eof_menu_edit_claps();
		key[KEY_C] = 0;
	}
	if(key[KEY_V] && !KEY_EITHER_CTRL)
	{	//Toggle vocal tones
		eof_menu_edit_vocal_tones();
		key[KEY_V] = 0;
	}
	if(KEY_EITHER_CTRL && eof_music_paused && !eof_music_catalog_playback && !eof_vocals_selected)
	{	//Toggle lanes on/off
		if(key[KEY_1])
		{
			eof_menu_note_toggle_green();
			key[KEY_1] = 0;
		}
		else if(key[KEY_2])
		{
			eof_menu_note_toggle_red();
			key[KEY_2] = 0;
		}
		else if(key[KEY_3])
		{
			eof_menu_note_toggle_yellow();
			key[KEY_3] = 0;
		}
		else if(key[KEY_4])
		{
			eof_menu_note_toggle_blue();
			key[KEY_4] = 0;
		}
		else if(key[KEY_5])
		{
			eof_menu_note_toggle_purple();
			key[KEY_5] = 0;
		}
		else if(key[KEY_6] && (numlanes >= 6))
		{	//Only allow use of the 6 key if lane 6 is available
			eof_menu_note_toggle_orange();
			key[KEY_6] = 0;
		}
	}
	else if(KEY_EITHER_SHIFT)
	{
		if(key[KEY_1])	//Change mini piano focus to first usable octave
		{
			eof_vocals_offset = MINPITCH;
			key[KEY_1] = 0;
		}
		else if(key[KEY_2])	//Change mini piano focus to second usable octave
		{
			eof_vocals_offset = MINPITCH+12;
			key[KEY_2] = 0;
		}
		else if(key[KEY_3])	//Change mini piano focus to third usable octave
		{
			eof_vocals_offset = MINPITCH+24;
			key[KEY_3] = 0;
		}
		else if(key[KEY_4])	//Change mini piano focus to fourth usable octave
		{
			eof_vocals_offset = MINPITCH+36;
			key[KEY_4] = 0;
		}
	}
	else if(!KEY_EITHER_CTRL)
	{
		if(eof_input_mode == EOF_INPUT_HOLD)
		{
			if(!KEY_EITHER_SHIFT && !eof_vocals_selected)
			{
				if(key[KEY_1])
				{
					eof_pen_note.note |= 1;	//Set the bit for lane 1
				}
				else
				{
					eof_pen_note.note &= (~1);	//Clear the bit for lane 1
				}
				if(key[KEY_2])
				{
					eof_pen_note.note |= 2;	//Set the bit for lane 2
				}
				else
				{
					eof_pen_note.note &= (~2);	//Clear the bit for lane 2
				}
				if(key[KEY_3])
				{
					eof_pen_note.note |= 4;	//Set the bit for lane 3
				}
				else
				{
					eof_pen_note.note &= (~4);	//Clear the bit for lane 3
				}
				if(key[KEY_4])
				{
					eof_pen_note.note |= 8;	//Set the bit for lane 4
				}
				else
				{
					eof_pen_note.note &= (~8);	//Clear the bit for lane 4
				}
				if(key[KEY_5])
				{
					eof_pen_note.note |= 16;	//Set the bit for lane 5
				}
				else
				{
					eof_pen_note.note &= (~16);	//Clear the bit for lane 5
				}
				if(key[KEY_6] && (numlanes >= 6))
				{	//Only allow use of the 6 key if lane 6 is available
					eof_pen_note.note |= 32;	//Set the bit for lane 6
				}
				else
				{
					eof_pen_note.note &= (~32);	//Clear the bit for lane 6
				}
			}
		}
		else if(eof_input_mode == EOF_INPUT_CLASSIC)
		{
			if(!KEY_EITHER_SHIFT && !eof_vocals_selected)
			{
				if(key[KEY_1])
				{
					eof_pen_note.note ^= 1;
					key[KEY_1] = 0;
				}
				if(key[KEY_2])
				{
					eof_pen_note.note ^= 2;
					key[KEY_2] = 0;
				}
				if(key[KEY_3])
				{
					eof_pen_note.note ^= 4;
					key[KEY_3] = 0;
				}
				if(key[KEY_4])
				{
					eof_pen_note.note ^= 8;
					key[KEY_4] = 0;
				}
				if(key[KEY_5])
				{
					eof_pen_note.note ^= 16;
					key[KEY_5] = 0;
				}
				if(key[KEY_6] && (numlanes >= 6))
				{	//Only allow use of the 6 key if lane 6 is available
					eof_pen_note.note ^= 32;
					key[KEY_6] = 0;
				}
			}
		}
		else if((eof_input_mode == EOF_INPUT_REX) && eof_music_paused && !eof_music_catalog_playback)
		{	//If the chart is paused, there is no fret catalog playback and the Rex Mundi input method is in use
			eof_hover_piece = -1;
			if((mouse_y >= eof_window_editor->y + 25 + EOF_EDITOR_RENDER_OFFSET) && (mouse_y < eof_window_editor->y + eof_screen_layout.fretboard_h + EOF_EDITOR_RENDER_OFFSET))
			{
				if(KEY_EITHER_SHIFT)
				{
					eof_snap.length = 1;
				}
				if(eof_vocals_selected)
				{
				}
				else
				{
					bitmask = 0;
					if(key[KEY_1])
					{
						bitmask = 1;
						key[KEY_1] = 0;
					}
					else if(key[KEY_2])
					{
						bitmask = 2;
						key[KEY_2] = 0;
					}
					else if(key[KEY_3])
					{
						bitmask = 4;
						key[KEY_3] = 0;
					}
					else if(key[KEY_4])
					{
						bitmask = 8;
						key[KEY_4] = 0;
					}
					else if(key[KEY_5])
					{
						bitmask = 16;
						key[KEY_5] = 0;
					}
					else if(key[KEY_6] && (numlanes >= 6))
					{	//Only allow use of the 6 key if lane 6 is available
						bitmask = 32;
						key[KEY_6] = 0;
					}

					if(bitmask)
					{	//If user has pressed any key from 1 through 6
						eof_selection.range_pos_1 = 0;
						eof_selection.range_pos_2 = 0;
						eof_prepare_undo(EOF_UNDO_TYPE_NOTE_SEL);
						if(eof_hover_note >= 0)
						{	//If the user edited an existing note
							eof_song->legacy_track[tracknum]->note[eof_hover_note]->note ^= bitmask;
							if(eof_mark_drums_as_cymbal)
							{	//If the user opted to make all new drum notes cymbals automatically
								eof_mark_edited_note_as_cymbal(eof_song,eof_selected_track,eof_hover_note,bitmask);
							}
							eof_selection.current = eof_hover_note;
							if(!eof_song->legacy_track[tracknum]->note[eof_hover_note]->note)
							{
								eof_track_delete_note(eof_song, eof_selected_track, eof_hover_note);
								eof_selection.multi[eof_selection.current] = 0;
								eof_selection.current = EOF_MAX_NOTES - 1;
								eof_legacy_track_sort_notes(eof_song->legacy_track[tracknum]);
								eof_legacy_track_fixup_notes(eof_song->legacy_track[tracknum], 1);
								eof_determine_hopos();
								eof_detect_difficulties(eof_song);
							}
							else
							{	//Run cleanup to prevent open bass<->lane 1 conflicts
								eof_legacy_track_fixup_notes(eof_song->legacy_track[tracknum], 0);
							}
							if(eof_selection.current != EOF_MAX_NOTES - 1)
							{
								memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
								eof_selection.track = eof_selected_track;
								eof_selection.multi[eof_selection.current] = 1;
								eof_selection.current_pos = eof_song->legacy_track[tracknum]->note[eof_selection.current]->pos;
								eof_selection.range_pos_1 = eof_selection.current_pos;
								eof_selection.range_pos_2 = eof_selection.current_pos;
							}
						}
						else
						{	//If the user created a new note
							eof_pen_note.note ^= bitmask;
							new_note = eof_track_add_create_note(eof_song, eof_selected_track, eof_pen_note.note, eof_pen_note.pos, eof_snap.length, eof_note_type, NULL);
							if(new_note)
							{
								if(eof_mark_drums_as_cymbal)
								{	//If the user opted to make all new drum notes cymbals automatically
									eof_mark_new_note_as_cymbal(eof_song,eof_selected_track,eof_song->legacy_track[tracknum]->notes-1);
								}
								eof_selection.current_pos = new_note->pos;
								eof_selection.range_pos_1 = eof_selection.current_pos;
								eof_selection.range_pos_2 = eof_selection.current_pos;
								eof_selection.track = eof_selected_track;
								memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
								eof_legacy_track_sort_notes(eof_song->legacy_track[tracknum]);
								eof_legacy_track_fixup_notes(eof_song->legacy_track[tracknum], 0);
								eof_determine_hopos();
								eof_selection.multi[eof_selection.current] = 1;
								eof_detect_difficulties(eof_song);
							}
						}
					}//If user has pressed any key from 1 through 6
				}
			}
		}//If the chart is paused, there is no fret catalog playback and the Rex Mundi input method is in use
		else if((eof_input_mode == EOF_INPUT_GUITAR_TAP) && !eof_music_paused && (eof_selected_track != EOF_TRACK_DRUM))
		{
			if(eof_guitar.button[2].held)
			{
				eof_held_1++;
			}
			else
			{
				eof_held_1 = 0;
			}
			if(eof_guitar.button[3].held)
			{
				eof_held_2++;
			}
			else
			{
				eof_held_2 = 0;
			}
			if(eof_guitar.button[4].held)
			{
				eof_held_3++;
			}
			else
			{
				eof_held_3 = 0;
			}
			if(eof_guitar.button[5].held)
			{
				eof_held_4++;
			}
			else
			{
				eof_held_4 = 0;
			}
			if(eof_guitar.button[6].held)
			{
				eof_held_5++;
			}
			else
			{
				eof_held_5 = 0;
			}
			bitmask = 0;
			if(eof_held_1 == 1)
			{
				bitmask = 1;
			}
			else if(eof_held_2 == 1)
			{
				bitmask = 2;
			}
			else if(eof_held_3 == 1)
			{
				bitmask = 4;
			}
			else if(eof_held_4 == 1)
			{
				bitmask = 8;
			}
			else if(eof_held_5 == 1)
			{
				bitmask = 16;
			}

			if(bitmask)
			{
				eof_prepare_undo(EOF_UNDO_TYPE_RECORD);
				if(eof_vocals_selected)
				{
					new_lyric = eof_track_add_create_note(eof_song, eof_selected_track, eof_vocals_offset, eof_music_pos - eof_av_delay - eof_guitar.delay, 1, 0, "");
					if(new_lyric)
					{
						eof_detect_difficulties(eof_song);
						eof_vocal_track_sort_lyrics(eof_song->vocal_track[tracknum]);
					}
				}
				else
				{
					new_note = eof_track_add_create_note(eof_song, eof_selected_track, bitmask, eof_music_pos - eof_av_delay - eof_guitar.delay, 1, eof_note_type, NULL);
					if(new_note)
					{
						if(eof_mark_drums_as_cymbal)
						{	//If the user opted to make all new drum notes cymbals automatically
							eof_mark_new_note_as_cymbal(eof_song,eof_selected_track,eof_song->legacy_track[tracknum]->notes-1);
						}
						eof_entering_note_note = new_note;
						eof_entering_note = 1;
						eof_detect_difficulties(eof_song);
						eof_legacy_track_sort_notes(eof_song->legacy_track[tracknum]);
					}
				}
			}
		}
		else if((eof_input_mode == EOF_INPUT_GUITAR_STRUM) && !eof_music_paused && (eof_selected_track != EOF_TRACK_DRUM))
		{
			if(eof_vocals_selected)
			{
				eof_last_snote = eof_snote;
				eof_snote = 0;

				if(eof_guitar.button[2].held || eof_guitar.button[3].held || eof_guitar.button[4].held || eof_guitar.button[5].held || eof_guitar.button[6].held)
				{
					eof_snote = 1;
				}
				if((eof_guitar.button[0].pressed || eof_guitar.button[1].pressed) && eof_snote)
				{
					if(eof_entering_note && eof_entering_note_lyric)
					{
						eof_entering_note_lyric->length = (eof_music_pos - eof_av_delay - eof_guitar.delay) - eof_entering_note_lyric->pos - 10;
					}
					eof_prepare_undo(EOF_UNDO_TYPE_RECORD);
					new_lyric = eof_track_add_create_note(eof_song, eof_selected_track, eof_vocals_offset, eof_music_pos - eof_av_delay - eof_guitar.delay, 1, 0, "");
					if(new_lyric)
					{
						eof_entering_note_lyric = new_lyric;
						eof_entering_note = 1;
						eof_detect_difficulties(eof_song);
						eof_vocal_track_sort_lyrics(eof_song->vocal_track[tracknum]);
					}
				}
				if(eof_entering_note)
				{
					if(eof_snote != eof_last_snote)
					{
						eof_entering_note = 0;
						eof_entering_note_lyric->length = (eof_music_pos - eof_av_delay - eof_guitar.delay) - eof_entering_note_lyric->pos - 10;
						eof_vocal_track_fixup_lyrics(eof_song->vocal_track[tracknum], 1);
					}
					else
					{
						eof_entering_note_lyric->length = (eof_music_pos - eof_av_delay - eof_guitar.delay) - eof_entering_note_lyric->pos - 10;
						eof_vocal_track_fixup_lyrics(eof_song->vocal_track[tracknum], 1);
					}
				}
			}//if(eof_vocals_selected)
			else
			{
				eof_last_snote = eof_snote;
				eof_snote = 0;

				if(eof_guitar.button[2].held)
				{
					eof_snote |= 1;
				}
				if(eof_guitar.button[3].held)
				{
					eof_snote |= 2;
				}
				if(eof_guitar.button[4].held)
				{
					eof_snote |= 4;
				}
				if(eof_guitar.button[5].held)
				{
					eof_snote |= 8;
				}
				if(eof_guitar.button[6].held)
				{
					eof_snote |= 16;
				}
				if(eof_open_bass_enabled() && (eof_selected_track == EOF_TRACK_BASS) && !eof_snote && (eof_guitar.button[0].held || eof_guitar.button[1].held))
				{	//If the strum is being held up/down with no frets, PART BASS is active, and open bass strumming is enabled
					eof_snote = 32;	//The strum note is lane 6
				}
				if(eof_guitar.button[0].pressed || eof_guitar.button[1].pressed)
				{	//If the user strummed
					if(eof_snote)
					{	//If a note is being added from this strum
						if(eof_entering_note && eof_entering_note_note)
						{
							eof_entering_note_note->length = (eof_music_pos - eof_av_delay - eof_guitar.delay) - eof_entering_note_note->pos - 10;
						}
						eof_prepare_undo(EOF_UNDO_TYPE_RECORD);
						new_note = eof_track_add_create_note(eof_song, eof_selected_track, eof_snote, eof_music_pos - eof_av_delay - eof_guitar.delay, 1, eof_note_type, NULL);
						if(new_note)
						{
							if(eof_mark_drums_as_cymbal)
							{	//If the user opted to make all new drum notes cymbals automatically
								eof_mark_new_note_as_cymbal(eof_song,eof_selected_track,eof_song->legacy_track[tracknum]->notes-1);
							}
							eof_entering_note_note = new_note;
							eof_entering_note = 1;
							eof_detect_difficulties(eof_song);
							eof_legacy_track_sort_notes(eof_song->legacy_track[tracknum]);
						}
					}
				}//If the user strummed
				if(eof_entering_note)
				{
					if(eof_snote != eof_last_snote)
					{
						eof_entering_note = 0;
						eof_entering_note_note->length = (eof_music_pos - eof_av_delay - eof_guitar.delay) - eof_entering_note_note->pos - 10;
						eof_legacy_track_fixup_notes(eof_song->legacy_track[tracknum], 1);
					}
					else
					{
						eof_entering_note_note->length = (eof_music_pos - eof_av_delay - eof_guitar.delay) - eof_entering_note_note->pos - 10;
						eof_legacy_track_fixup_notes(eof_song->legacy_track[tracknum], 1);
					}
				}
			}
		}
	}
	if((eof_input_mode == EOF_INPUT_CLASSIC) || (eof_input_mode == EOF_INPUT_HOLD))
	{
		if(key[KEY_ENTER] && eof_song_loaded)
		{
			/* place note with default length if song is paused */
			if(eof_music_paused)
			{
				new_note = eof_track_add_create_note(eof_song, eof_selected_track, eof_pen_note.note, eof_music_pos - eof_av_delay, eof_snap.length, eof_note_type, NULL);
				if(new_note)
				{
					if(eof_mark_drums_as_cymbal)
					{	//If the user opted to make all new drum notes cymbals automatically
						eof_mark_new_note_as_cymbal(eof_song,eof_selected_track,eof_song->legacy_track[tracknum]->notes-1);
					}
					eof_detect_difficulties(eof_song);
				}
				key[KEY_ENTER] = 0;
			}

			/* otherwise allow length to be determined by key holding */
			else
			{
				if(!eof_entering_note_note)
				{
					new_note = eof_track_add_create_note(eof_song, eof_selected_track, eof_pen_note.note, eof_music_pos - eof_av_delay, eof_snap.length, eof_note_type, NULL);
					if(new_note)
					{
						if(eof_mark_drums_as_cymbal)
						{	//If the user opted to make all new drum notes cymbals automatically
							eof_mark_new_note_as_cymbal(eof_song,eof_selected_track,eof_song->legacy_track[tracknum]->notes-1);
						}
						eof_entering_note_note = new_note;
						eof_entering_note = 1;
						eof_detect_difficulties(eof_song);
					}
				}
				else
				{
					eof_entering_note_note->length = (eof_music_pos - eof_av_delay) - eof_entering_note_note->pos - 10;
//					eof_song->legacy_track[tracknum]->note[eof_entering_note_note]->length = (eof_music_pos - eof_av_delay) - eof_song->legacy_track[tracknum]->note[eof_entering_note_note]->pos;
				}
			}
		}
		else if(!key[KEY_ENTER])
		{
			if(eof_entering_note)
			{
				eof_entering_note_note->length = (eof_music_pos - eof_av_delay) - eof_entering_note_note->pos;
				eof_entering_note = 0;
			}
			eof_entering_note_note = NULL;
		}
	}

	if(eof_music_paused && !eof_music_catalog_playback)
	{
		if(eof_song_loaded)
		{
			if(key[KEY_F2] || (KEY_EITHER_CTRL && key[KEY_S]))
			{
				eof_menu_file_save();
				key[KEY_F2] = 0;
			}
			if(key[KEY_DEL])
			{
				if(KEY_EITHER_CTRL)
				{
					eof_menu_beat_delete();
				}
				else
				{
					eof_menu_note_delete();
				}
				key[KEY_DEL] = 0;
			}
			if(key[KEY_0_PAD])
			{
				if(KEY_EITHER_CTRL)
				{
					eof_menu_edit_bookmark_0();
				}
				else
				{
					eof_menu_song_seek_bookmark_0();
				}
				key[KEY_0_PAD] = 0;
			}
			if(key[KEY_1_PAD])
			{
				if(KEY_EITHER_CTRL)
				{
					eof_menu_edit_bookmark_1();
				}
				else
				{
					eof_menu_song_seek_bookmark_1();
				}
				key[KEY_1_PAD] = 0;
			}
			if(key[KEY_2_PAD])
			{
				if(KEY_EITHER_CTRL)
				{
					eof_menu_edit_bookmark_2();
				}
				else
				{
					eof_menu_song_seek_bookmark_2();
				}
				key[KEY_2_PAD] = 0;
			}
			if(key[KEY_3_PAD])
			{
				if(KEY_EITHER_CTRL)
				{
					eof_menu_edit_bookmark_3();
				}
				else
				{
					eof_menu_song_seek_bookmark_3();
				}
				key[KEY_3_PAD] = 0;
			}
			if(key[KEY_4_PAD])
			{
				if(KEY_EITHER_CTRL)
				{
					eof_menu_edit_bookmark_4();
				}
				else
				{
					eof_menu_song_seek_bookmark_4();
				}
				key[KEY_4_PAD] = 0;
			}
			if(key[KEY_5_PAD])
			{
				if(KEY_EITHER_CTRL)
				{
					eof_menu_edit_bookmark_5();
				}
				else
				{
					eof_menu_song_seek_bookmark_5();
				}
				key[KEY_5_PAD] = 0;
			}
			if(key[KEY_6_PAD])
			{
				if(KEY_EITHER_CTRL)
				{
					eof_menu_edit_bookmark_6();
				}
				else
				{
					eof_menu_song_seek_bookmark_6();
				}
				key[KEY_6_PAD] = 0;
			}
			if(key[KEY_7_PAD])
			{
				if(KEY_EITHER_CTRL)
				{
					eof_menu_edit_bookmark_7();
				}
				else
				{
					eof_menu_song_seek_bookmark_7();
				}
				key[KEY_7_PAD] = 0;
			}
			if(key[KEY_8_PAD])
			{
				if(KEY_EITHER_CTRL)
				{
					eof_menu_edit_bookmark_8();
				}
				else
				{
					eof_menu_song_seek_bookmark_8();
				}
				key[KEY_8_PAD] = 0;
			}
			if(key[KEY_9_PAD])
			{
				if(KEY_EITHER_CTRL)
				{
					eof_menu_edit_bookmark_9();
				}
				else
				{
					eof_menu_song_seek_bookmark_9();
				}
				key[KEY_9_PAD] = 0;
			}
			if(key[KEY_F9])
			{
				clear_keybuf();
				eof_menu_song_properties();
				key[KEY_F9] = 0;
			}
			if(key[KEY_F12])
			{
				eof_menu_song_test();
				key[KEY_F12] = 0;
			}
			if(KEY_EITHER_CTRL && key[KEY_C])
			{
				eof_menu_edit_copy();
				key[KEY_C] = 0;
			}
			if(KEY_EITHER_CTRL && key[KEY_V])
			{
				eof_menu_edit_paste();
				key[KEY_V] = 0;
			}
			if(KEY_EITHER_CTRL && key[KEY_P])
			{	//CTRL+P is "Old Paste"
				eof_menu_edit_old_paste();
				key[KEY_P] = 0;
			}
			if(key[KEY_F5])
			{
				eof_menu_song_waveform();
				key[KEY_F5] = 0;
			}
		}
	}
}

void eof_editor_drum_logic(void)
{
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	int bitmask;	//Used for simplifying note placement logic
	EOF_NOTE * new_note = NULL;

	if(eof_drums.button[0].held)
	{
		eof_held_1++;
	}
	else
	{
		eof_held_1 = 0;
	}
	if(eof_drums.button[1].held)
	{
		eof_held_2++;
	}
	else
	{
		eof_held_2 = 0;
	}
	if(eof_drums.button[2].held)
	{
		eof_held_3++;
	}
	else
	{
		eof_held_3 = 0;
	}
	if(eof_drums.button[3].held)
	{
		eof_held_4++;
	}
	else
	{
		eof_held_4 = 0;
	}
	if(eof_drums.button[4].held)
	{
		eof_held_5++;
	}
	else
	{
		eof_held_5 = 0;
	}
	if(eof_entering_note_note && (eof_music_pos - eof_av_delay - eof_drums.delay >= eof_entering_note_note->pos + 50))
	{
		eof_entering_note_note = NULL;
	}
	bitmask = 0;
	if(eof_held_1 == 1)
	{
		bitmask |= 1;
	}
	if(eof_held_2 == 1)
	{
		bitmask |= 2;
	}
	if(eof_held_3 == 1)
	{
		bitmask |= 4;
	}
	if(eof_held_4 == 1)
	{
		bitmask |= 8;
	}
	if(eof_held_5 == 1)
	{
		bitmask |= 16;
	}

	if(bitmask)
	{
		eof_prepare_undo(EOF_UNDO_TYPE_RECORD);
		if(eof_entering_note_note && (eof_music_pos - eof_av_delay - eof_drums.delay < eof_entering_note_note->pos + 50))
		{
			eof_entering_note_note->note |= bitmask;
		}
		else
		{
			new_note = eof_track_add_create_note(eof_song, eof_selected_track, bitmask, eof_music_pos - eof_av_delay - eof_drums.delay, 1, eof_note_type, NULL);
			if(new_note)
			{
				if(eof_mark_drums_as_cymbal)
				{	//If the user opted to make all new drum notes cymbals automatically
					eof_mark_new_note_as_cymbal(eof_song,eof_selected_track,eof_song->legacy_track[tracknum]->notes-1);
				}
				eof_snap_logic(&eof_snap, new_note->pos);
				new_note->pos = eof_snap.pos;
				eof_entering_note_note = new_note;
				eof_entering_note = 1;
				eof_detect_difficulties(eof_song);
				eof_legacy_track_sort_notes(eof_song->legacy_track[tracknum]);
			}
		}
	}
}

void eof_editor_logic(void)
{
	unsigned long i;
	unsigned long tracknum = 0;
	unsigned long bitmask = 0;	//Used to reduce duplicated logic
	int use_this_x = mouse_x;
	int next_note_pos = 0;
	EOF_NOTE * new_note = NULL;
	int pos = eof_music_pos / eof_zoom;
	int npos, lpos;

	if(!eof_song_loaded)
		return;
	if(eof_vocals_selected)
		return;

	eof_editor_logic_common();

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	if(eof_music_paused)
	{	//If the chart is paused
		if(!(mouse_b & 1) && !(mouse_b & 2) && !key[KEY_INSERT])
		{	//If the left and right mouse buttons and insert key are NOT pressed
			eof_undo_toggle = 0;
			if(eof_notes_moved)
			{
				eof_legacy_track_sort_notes(eof_song->legacy_track[tracknum]);
				eof_legacy_track_fixup_notes(eof_song->legacy_track[tracknum], 1);
				eof_determine_hopos();
				eof_notes_moved = 0;
			}
		}

		/* mouse is in the fretboard area */
		if((mouse_y >= eof_window_editor->y + 25 + EOF_EDITOR_RENDER_OFFSET) && (mouse_y < eof_window_editor->y + eof_screen_layout.fretboard_h + EOF_EDITOR_RENDER_OFFSET))
		{
			lpos = pos < 300 ? (mouse_x - 20) * eof_zoom : ((pos - 300) + mouse_x - 20) * eof_zoom;
			eof_snap_logic(&eof_snap, lpos);
			eof_snap_length_logic(&eof_snap);
			eof_pen_note.pos = eof_snap.pos;
			use_this_x = lpos;
			eof_pen_visible = 1;
			for(i = 0; (i < eof_song->legacy_track[tracknum]->notes) && (eof_hover_note < 0); i++)
			{	//For each note in the active track, until a hover note is found
				if(eof_song->legacy_track[tracknum]->note[i]->type == eof_note_type)
				{	//If the note is in the active difficulty
					npos = eof_song->legacy_track[tracknum]->note[i]->pos;
					if((use_this_x > npos - (6 * eof_zoom)) && (use_this_x < npos + (6 * eof_zoom)))
					{
						eof_hover_note = i;
					}
					else if((eof_pen_note.pos > npos - (6 * eof_zoom)) && (eof_pen_note.pos < npos + (6 * eof_zoom)))
					{
						eof_hover_note = i;
					}
					if((npos > eof_pen_note.pos) && (next_note_pos == 0))
					{
						next_note_pos = npos;
					}
				}
			}
			if((eof_input_mode == EOF_INPUT_PIANO_ROLL) || (eof_input_mode == EOF_INPUT_REX))
			{	//If piano roll or rex mundi input modes are in use
				if(eof_hover_note >= 0)
				{	//If a note is being moused over
					eof_pen_note.note = eof_song->legacy_track[tracknum]->note[eof_hover_note]->note;
					eof_pen_note.length = eof_song->legacy_track[tracknum]->note[eof_hover_note]->length;
					if(!eof_mouse_drug)
					{
						eof_pen_note.pos = eof_song->legacy_track[tracknum]->note[eof_hover_note]->pos;
						eof_pen_note.note |= eof_find_pen_note_mask();	//Set the appropriate bits
					}
				}
				else
				{
					if(eof_input_mode == EOF_INPUT_REX)
					{
						eof_pen_note.note = 0;
					}
					if(KEY_EITHER_SHIFT)
					{	//If shift is held down, a new note will be 1ms long
						eof_pen_note.length = 1;
					}
					else
					{	//Otherwise it will be as long as the current grid snap value (or 100ms if grid snap is off)
						eof_pen_note.length = eof_snap.length;
						if((eof_hover_note < 0) && (next_note_pos > 0) && (eof_pen_note.pos + eof_pen_note.length >= next_note_pos))
						{
							eof_pen_note.length = next_note_pos - eof_pen_note.pos - 1;
						}
					}
				}
			}//If piano roll or rex mundi input modes are in use

			/* calculate piece for piano roll mode */
			if(eof_input_mode == EOF_INPUT_PIANO_ROLL)
			{
				eof_pen_note.note = eof_find_pen_note_mask();	//Find eof_hover_piece and set the appropriate bits in the pen note
			}

			/* handle initial click */
			if((mouse_b & 1) && eof_lclick_released)
			{
				int ignore_range = 0;
				eof_click_x = mouse_x;
				eof_click_y = mouse_y;
				eof_lclick_released = 0;
				if(eof_hover_note >= 0)
				{
					if(eof_selection.current != EOF_MAX_NOTES - 1)
					{
						eof_selection.last = eof_selection.current;
						eof_selection.last_pos = eof_selection.current_pos;
					}
					else
					{
						eof_selection.last = eof_hover_note;
						eof_selection.last_pos = eof_song->legacy_track[tracknum]->note[eof_selection.last]->pos;
					}
					eof_selection.current = eof_hover_note;
					eof_selection.current_pos = eof_song->legacy_track[tracknum]->note[eof_selection.current]->pos;
					if(KEY_EITHER_SHIFT)
					{
						if((eof_selection.range_pos_1 == 0) && (eof_selection.range_pos_2 == 0))
						{
							ignore_range = 1;
							eof_selection.range_pos_1 = eof_selection.current_pos;
							eof_selection.range_pos_2 = eof_selection.current_pos;
						}
						else
						{
							if(eof_selection.current_pos > eof_selection.range_pos_2)
							{
								eof_selection.range_pos_2 = eof_selection.current_pos;
							}
							else
							{
								eof_selection.range_pos_1 = eof_selection.current_pos;
							}
						}
					}
					else
					{
						eof_selection.range_pos_1 = eof_selection.current_pos;
						eof_selection.range_pos_2 = eof_selection.current_pos;
					}
					eof_pegged_note = eof_selection.current;
					eof_peg_x = eof_song->legacy_track[tracknum]->note[eof_pegged_note]->pos;
					if(!KEY_EITHER_CTRL)
					{
						/* Shift+Click selects range */
						if(KEY_EITHER_SHIFT && !ignore_range)
						{
							if(eof_selection.range_pos_1 < eof_selection.range_pos_2)
							{
								for(i = 0; i < eof_song->legacy_track[tracknum]->notes; i++)
								{
									if((eof_song->legacy_track[tracknum]->note[i]->type == eof_note_type) && (eof_song->legacy_track[tracknum]->note[i]->pos >= eof_selection.range_pos_1) && (eof_song->legacy_track[tracknum]->note[i]->pos <= eof_selection.range_pos_2))
									{
										if(eof_selection.track != eof_selected_track)
										{
											eof_selection.track = eof_selected_track;
											memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
										}
										eof_selection.multi[i] = 1;
										eof_undo_last_type = EOF_UNDO_TYPE_NONE;
									}
								}
							}
							else
							{
								for(i = 0; i < eof_song->legacy_track[tracknum]->notes; i++)
								{
									if((eof_song->legacy_track[tracknum]->note[i]->type == eof_note_type) && (eof_song->legacy_track[tracknum]->note[i]->pos >= eof_selection.range_pos_2) && (eof_song->legacy_track[tracknum]->note[i]->pos <= eof_selection.range_pos_1))
									{
										if(eof_selection.track != eof_selected_track)
										{
											eof_selection.track = eof_selected_track;
											memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
										}
										eof_selection.multi[i] = 1;
										eof_undo_last_type = EOF_UNDO_TYPE_NONE;
									}
								}
							}
						}
						else
						{
							if(!eof_selection.multi[eof_selection.current])
							{
//								printf("notes %d\n", eof_notes_selected());
								memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
							}
							if(eof_selection.multi[eof_selection.current] == 1)
							{
								if(eof_selection.track != eof_selected_track)
								{
									eof_selection.track = eof_selected_track;
									memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
								}
								eof_selection.multi[eof_selection.current] = 2;
								eof_undo_last_type = EOF_UNDO_TYPE_NONE;
							}
							else
							{
								if(eof_selection.track != eof_selected_track)
								{
									eof_selection.track = eof_selected_track;
									memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
								}
								eof_selection.multi[eof_selection.current] = 1;
								eof_undo_last_type = EOF_UNDO_TYPE_NONE;
							}
						}
					}

					/* Ctrl+Click adds to selected notes */
					else
					{
						if(eof_selection.multi[eof_selection.current] == 1)
						{
							if(eof_selection.track != eof_selected_track)
							{
								eof_selection.track = eof_selected_track;
								memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
							}
							eof_selection.multi[eof_selection.current] = 2;
							eof_undo_last_type = EOF_UNDO_TYPE_NONE;
						}
						else
						{
							if(eof_selection.track != eof_selected_track)
							{
								eof_selection.track = eof_selected_track;
								memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
							}
							eof_selection.multi[eof_selection.current] = 1;
							eof_undo_last_type = EOF_UNDO_TYPE_NONE;
						}
					}
				}

				/* clicking on no note deselects all notes */
				else
				{
					if(!KEY_EITHER_SHIFT && !KEY_EITHER_CTRL)
					{
						memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
						eof_selection.current = EOF_MAX_NOTES - 1;
						eof_selection.current_pos = 0;
						eof_selection.range_pos_1 = 0;
						eof_selection.range_pos_2 = 0;
					}
				}
			}// handle initial click
			if(!(mouse_b & 1))
			{	//If the left mouse button is NOT pressed
				if(!eof_lclick_released)
				{
					eof_lclick_released++;
				}

				/* mouse button just released, check to see what needs doing */
				else if(eof_lclick_released == 1)
				{
					if(!eof_mouse_drug)
					{
						if(!KEY_EITHER_CTRL)
						{
							if(KEY_EITHER_SHIFT)
							{
							}
							else
							{
								memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
								if(eof_selection.multi[eof_selection.current])
								{
									eof_selection.multi[eof_selection.current] = 0;
								}
								else
								{
									eof_selection.track = eof_selected_track;
									eof_selection.multi[eof_selection.current] = 1;
								}
								eof_undo_last_type = EOF_UNDO_TYPE_NONE;
							}
						}
						else
						{
							if(eof_selection.multi[eof_selection.current] == 2)
							{
								eof_selection.multi[eof_selection.current] = 0;
								eof_undo_last_type = EOF_UNDO_TYPE_NONE;
							}
							else if(eof_hover_note >= 0)
							{
								if(eof_selection.track != eof_selected_track)
								{
									eof_selection.track = eof_selected_track;
									memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
								}
								eof_selection.multi[eof_selection.current] = 1;
								eof_undo_last_type = EOF_UNDO_TYPE_NONE;
							}
						}
					}
					eof_lclick_released++;
				}
				else
				{
					if(eof_mouse_drug)
					{
						eof_mouse_drug = 0;
					}
				}
			}//If the left mouse button is NOT pressed
			unsigned long move_offset = 0;
			int revert = 0;
			int revert_amount = 0;
			if((mouse_b & 1) && !eof_lclick_released)
			{
				if(eof_mouse_drug && !KEY_EITHER_SHIFT && !KEY_EITHER_CTRL)
				{
					eof_mouse_drug++;
					if(eof_mouse_drug == 11)
					{
						eof_mickeys_x = mouse_x - eof_click_x;
					}
				}
				if((eof_mickeys_x != 0) && !eof_mouse_drug)
				{
//					eof_mouse_drug = 1;
					eof_mouse_drug++;
				}
				if(pos < 300)
				{
					npos = 20;
				}
				else
				{
					npos = 20 - (pos - 300);
				}
				if((eof_mouse_drug > 10) && (eof_selection.current != EOF_MAX_NOTES - 1))
				{
					if((eof_snap_mode != EOF_SNAP_OFF) && !KEY_EITHER_CTRL)
					{
						move_offset = eof_pen_note.pos - eof_song->legacy_track[tracknum]->note[eof_selection.current]->pos;
					}
					if(!eof_undo_toggle && ((move_offset != 0) || (eof_snap_mode == EOF_SNAP_OFF) || KEY_EITHER_CTRL))
					{
						eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					}
					eof_notes_moved = 1;
					for(i = 0; i < eof_song->legacy_track[tracknum]->notes; i++)
					{
						if(eof_selection.multi[i])
						{
							if((eof_snap_mode == EOF_SNAP_OFF) || KEY_EITHER_CTRL)
							{
								if(eof_song->legacy_track[tracknum]->note[i]->pos == eof_selection.current_pos)
								{
									eof_selection.current_pos += eof_mickeys_x * eof_zoom;
								}
								eof_song->legacy_track[tracknum]->note[i]->pos += eof_mickeys_x * eof_zoom;
							}
							else
							{
								if(eof_song->legacy_track[tracknum]->note[i]->pos == eof_selection.current_pos)
								{
									eof_selection.current_pos += move_offset;
								}
								eof_song->legacy_track[tracknum]->note[i]->pos += move_offset;
							}
							if(eof_song->legacy_track[tracknum]->note[i]->pos + eof_song->legacy_track[tracknum]->note[i]->length >= eof_music_length)
							{
								revert = 1;
								revert_amount = eof_song->legacy_track[tracknum]->note[i]->pos + eof_song->legacy_track[tracknum]->note[i]->length - eof_music_length;
							}
						}
					}
					if(revert)
					{
						for(i = 0; i < eof_song->legacy_track[tracknum]->notes; i++)
						{
							if(eof_selection.multi[i])
							{
								eof_song->legacy_track[tracknum]->note[i]->pos -= revert_amount;
							}
						}
					}
				}
//				eof_song->legacy_track[tracknum]->note[eof_pegged_note].pos = eof_peg_x + (mouse_x - eof_click_x) * eof_zoom;
			}
			if(((mouse_b & 2) || key[KEY_INSERT]) && eof_rclick_released && eof_pen_note.note && (eof_pen_note.pos < eof_music_length))
			{
				eof_selection.range_pos_1 = 0;
				eof_selection.range_pos_2 = 0;
				eof_rclick_released = 0;
				if(eof_input_mode == EOF_INPUT_PIANO_ROLL)
				{
					if(!eof_undo_toggle)
					{
						eof_prepare_undo(EOF_UNDO_TYPE_NOTE_SEL);
					}
					if(eof_hover_note >= 0)
					{
						bitmask = eof_find_pen_note_mask();	//Set the appropriate bits

						if(bitmask)
						{
							eof_song->legacy_track[tracknum]->note[eof_hover_note]->note ^= bitmask;
							eof_selection.current = eof_hover_note;
							eof_selection.track = eof_selected_track;
							memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
							if(!eof_song->legacy_track[tracknum]->note[eof_hover_note]->note)
							{
								eof_track_delete_note(eof_song, eof_selected_track, eof_hover_note);
								eof_selection.current = EOF_MAX_NOTES - 1;
								eof_legacy_track_sort_notes(eof_song->legacy_track[tracknum]);
								eof_legacy_track_fixup_notes(eof_song->legacy_track[tracknum], 1);
								eof_determine_hopos();
								eof_detect_difficulties(eof_song);
							}
							else
							{
								eof_selection.track = eof_selected_track;
								eof_selection.multi[eof_selection.current] = 1;
								eof_selection.current_pos = eof_song->legacy_track[tracknum]->note[eof_selection.current]->pos;
								eof_selection.range_pos_1 = eof_selection.current_pos;
								eof_selection.range_pos_2 = eof_selection.current_pos;
								eof_legacy_track_fixup_notes(eof_song->legacy_track[tracknum], 0);	//Run cleanup to prevent open bass<->lane 1 conflicts
							}
						}
					}
					else
					{
						new_note = eof_track_add_create_note(eof_song, eof_selected_track, eof_pen_note.note, eof_pen_note.pos, (KEY_EITHER_SHIFT ? 1 : eof_snap.length), eof_note_type, NULL);
						if(new_note)
						{
							if(eof_mark_drums_as_cymbal)
							{	//If the user opted to make all new drum notes cymbals automatically
								eof_mark_new_note_as_cymbal(eof_song,eof_selected_track,eof_song->legacy_track[tracknum]->notes-1);
							}
							eof_selection.current_pos = new_note->pos;
							eof_selection.range_pos_1 = eof_selection.current_pos;
							eof_selection.range_pos_2 = eof_selection.current_pos;
							eof_selection.track = eof_selected_track;
							memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
							eof_legacy_track_sort_notes(eof_song->legacy_track[tracknum]);
							eof_legacy_track_fixup_notes(eof_song->legacy_track[tracknum], 0);
							eof_determine_hopos();
							eof_detect_difficulties(eof_song);
						}
					}
				}
				else if(eof_input_mode != EOF_INPUT_REX)
				{
					if(!eof_undo_toggle)
					{
						eof_prepare_undo(EOF_UNDO_TYPE_NOTE_SEL);
					}
					new_note = eof_track_add_create_note(eof_song, eof_selected_track, eof_pen_note.note, eof_pen_note.pos, (KEY_EITHER_SHIFT ? 1 : eof_snap.length), eof_note_type, NULL);
					if(new_note)
					{
						if(eof_mark_drums_as_cymbal)
						{	//If the user opted to make all new drum notes cymbals automatically
							eof_mark_new_note_as_cymbal(eof_song,eof_selected_track,eof_song->legacy_track[tracknum]->notes-1);
						}
						eof_selection.current_pos = new_note->pos;
						eof_selection.range_pos_1 = eof_selection.current_pos;
						eof_selection.range_pos_2 = eof_selection.current_pos;
						eof_selection.track = eof_selected_track;
						memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
						eof_legacy_track_sort_notes(eof_song->legacy_track[tracknum]);
						eof_legacy_track_fixup_notes(eof_song->legacy_track[tracknum], 0);
						eof_determine_hopos();
						eof_detect_difficulties(eof_song);
					}
				}
			}
			if(!(mouse_b & 2) && !key[KEY_INSERT])
			{
				eof_rclick_released = 1;
			}

			if((eof_mickey_z != 0) && eof_count_selected_notes(NULL, 0))
			{
				eof_prepare_undo(EOF_UNDO_TYPE_NOTE_LENGTH);
			}
			if(eof_snap_mode == EOF_SNAP_OFF)
			{
				if(KEY_EITHER_CTRL)
				{
					for(i = 0; i < eof_song->legacy_track[tracknum]->notes; i++)
					{
						if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_song->legacy_track[tracknum]->note[i]->type == eof_note_type))
						{
							eof_song->legacy_track[tracknum]->note[i]->length -= eof_mickey_z * 10;
						}
					}
				}
				else
				{
					for(i = 0; i < eof_song->legacy_track[tracknum]->notes; i++)
					{
						if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_song->legacy_track[tracknum]->note[i]->type == eof_note_type))
						{
							eof_song->legacy_track[tracknum]->note[i]->length -= eof_mickey_z * 100;
						}
					}
				}
			}
			else
			{
				int b;
				if(eof_mickey_z > 0)
				{
					for(i = 0; i < eof_song->legacy_track[tracknum]->notes; i++)
					{
						if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_song->legacy_track[tracknum]->note[i]->type == eof_note_type))
						{
							b = eof_get_beat(eof_song, eof_song->legacy_track[tracknum]->note[i]->pos + eof_song->legacy_track[tracknum]->note[i]->length - 1);
							if(b >= 0)
							{
								eof_snap_logic(&eof_tail_snap, eof_song->beat[b]->pos);
							}
							else
							{
								eof_snap_logic(&eof_tail_snap, eof_song->legacy_track[tracknum]->note[i]->pos + eof_song->legacy_track[tracknum]->note[i]->length - 1);
							}
							eof_snap_length_logic(&eof_tail_snap);
//							allegro_message("%d, %d\n%lu, %lu", eof_tail_snap.length, eof_tail_snap.beat, eof_song->legacy_track[tracknum]->note[i]->pos + eof_song->legacy_track[tracknum]->note[i]->length, eof_song->beat[eof_tail_snap.beat]->pos);
							eof_song->legacy_track[tracknum]->note[i]->length -= eof_tail_snap.length;
							if(eof_song->legacy_track[tracknum]->note[i]->length > 1)
							{
								eof_snap_logic(&eof_tail_snap, eof_song->legacy_track[tracknum]->note[i]->pos + eof_song->legacy_track[tracknum]->note[i]->length);
								eof_set_tail_pos(i, eof_tail_snap.pos);
							}
						}
					}
				}
				else if(eof_mickey_z < 0)
				{
					for(i = 0; i < eof_song->legacy_track[tracknum]->notes; i++)
					{
						if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_song->legacy_track[tracknum]->note[i]->type == eof_note_type))
						{
							eof_snap_logic(&eof_tail_snap, eof_song->legacy_track[tracknum]->note[i]->pos + eof_song->legacy_track[tracknum]->note[i]->length + 1);
							if(eof_tail_snap.pos > eof_song->legacy_track[tracknum]->note[i]->pos + eof_song->legacy_track[tracknum]->note[i]->length + 1)
							{
								eof_set_tail_pos(i, eof_tail_snap.pos);
							}
							else
							{
								eof_snap_length_logic(&eof_tail_snap);
								eof_song->legacy_track[tracknum]->note[i]->length += eof_tail_snap.length;
								eof_snap_logic(&eof_tail_snap, eof_song->legacy_track[tracknum]->note[i]->pos + eof_song->legacy_track[tracknum]->note[i]->length);
								eof_set_tail_pos(i, eof_tail_snap.pos);
							}
//							eof_snap_logic(&eof_tail_snap, eof_song->legacy_track[tracknum]->note[i]->pos + eof_song->legacy_track[tracknum]->note[i]->length + 1);
						}
					}
				}
			}
			if(eof_mickey_z != 0)
			{
				eof_legacy_track_fixup_notes(eof_song->legacy_track[tracknum], 1);
			}
		}//mouse is in the fretboard area
		else
		{
			eof_pen_visible = 0;
//			eof_hover_note = -1;
		}
	}//If the chart is paused
	else
	{	//If the chart is not paused
		for(i = 0; i < eof_song->beats - 1; i++)
		{
			if((eof_music_pos >= eof_song->beat[i]->pos) && (eof_music_pos < eof_song->beat[i + 1]->pos))
			{
				if(eof_hover_beat_2 != i)
				{
					eof_hover_beat_2 = i;
				}
				break;
			}
		}
		for(i = 0; i < eof_song->legacy_track[tracknum]->notes; i++)
		{
			if(eof_song->legacy_track[tracknum]->note[i]->type == eof_note_type)
			{
				npos = eof_song->legacy_track[tracknum]->note[i]->pos / eof_zoom;
				if((pos - eof_av_delay / eof_zoom > npos) && (pos - eof_av_delay / eof_zoom < npos + (eof_song->legacy_track[tracknum]->note[i]->length > 100 ? eof_song->legacy_track[tracknum]->note[i]->length : 100) / eof_zoom))
				{
					eof_hover_note = i;
					break;
				}
			}
		}
		if(i == eof_song->legacy_track[tracknum]->notes)
		{
			eof_hover_note = -1;
		}
		for(i = 0; i < eof_song->legacy_track[tracknum]->notes; i++)
		{
			if(eof_song->legacy_track[tracknum]->note[i]->type == eof_note_type)
			{
				npos = eof_song->legacy_track[tracknum]->note[i]->pos;
				if((eof_music_pos > npos) && (eof_music_pos < npos + (eof_song->legacy_track[tracknum]->note[i]->length > 100 ? eof_song->legacy_track[tracknum]->note[i]->length : 100)))
				{
					if(eof_hover_note_2 != i)
				{
						eof_hover_note_2 = i;
					}
					break;
				}
			}
		}
		if(eof_selected_track == EOF_TRACK_DRUM)
		{
			eof_editor_drum_logic();
		}
	}//If the chart is not paused

	if(eof_music_catalog_playback)
	{	//If a fret catalog entry is playing
		tracknum = eof_song->catalog->entry[eof_selected_catalog_entry].track;	//The track this catalog entry pertains to
		eof_hover_note_2 = -1;
		if(eof_song->catalog->entry[eof_selected_catalog_entry].track != EOF_TRACK_VOCALS)
		{	//Perform fret catalog playback logic for legacy format tracks
			for(i = 0; i < eof_song->legacy_track[tracknum]->notes; i++)
			{
				if(eof_song->legacy_track[tracknum]->note[i]->type == eof_song->catalog->entry[eof_selected_catalog_entry].type)
				{
					npos = eof_song->legacy_track[tracknum]->note[i]->pos + eof_av_delay;
					if((eof_music_catalog_pos > npos) && (eof_music_catalog_pos < npos + (eof_song->legacy_track[tracknum]->note[i]->length > 100 ? eof_song->legacy_track[tracknum]->note[i]->length : 100)))
					{
						if(eof_hover_note_2 != i)
						{
							eof_hover_note_2 = i;
						}
						break;
					}
				}
			}
		}
		else
		{
			for(i = 0; i < eof_song->vocal_track[0]->lyrics; i++)
			{
				npos = eof_song->vocal_track[0]->lyric[i]->pos + eof_av_delay;
				if((eof_music_catalog_pos > npos) && (eof_music_catalog_pos < npos + (eof_song->vocal_track[0]->lyric[i]->length > 100 ? eof_song->vocal_track[0]->lyric[i]->length : 100)))
				{
					if(eof_hover_note_2 != i)
					{
						eof_hover_note_2 = i;
					}
					break;
				}
			}
		}
	}//If a fret catalog entry is playing

	/* select difficulty */
	if((mouse_y >= eof_window_editor->y + 7) && (mouse_y < eof_window_editor->y + 20 + 8) && (mouse_x > 12) && (mouse_x < 12 + 5 * 80 + 12 - 1))
	{
		eof_hover_type = (mouse_x - 12) / 80;
		if(eof_hover_type < 0)
		{
			eof_hover_type = 0;
		}
		else if(eof_hover_type > 4)
		{
			eof_hover_type = 4;
		}
		if(mouse_b & 1)
		{
			if(eof_note_type != eof_hover_type)
			{
				eof_note_type = eof_hover_type;
				eof_mix_find_claps();
				eof_mix_start_helper();
				eof_detect_difficulties(eof_song);
			}
		}
	}
	else
	{
		eof_hover_type = -1;
	}

	if(((mouse_b & 2) || key[KEY_INSERT]) && (eof_input_mode == EOF_INPUT_REX))
	{	//If the right mouse button or Insert key is pressed, a song is loaded and Rex Mundi input mode is in use
		eof_emergency_stop_music();
		eof_render();
		eof_show_mouse(screen);
		if((mouse_y >= eof_window_editor->y + EOF_EDITOR_RENDER_OFFSET - 4) && (mouse_y < eof_window_editor->y + EOF_EDITOR_RENDER_OFFSET + 18))
		{
			lpos = pos < 300 ? (eof_song->beat[eof_selected_beat]->pos / eof_zoom + 20) : 300;
			eof_prepare_menus();
			do_menu(eof_beat_menu, lpos, mouse_y);
			eof_clear_input();
		}
		else if((mouse_y >= eof_window_editor->y + 25 + EOF_EDITOR_RENDER_OFFSET) && (mouse_y < eof_window_editor->y + eof_screen_layout.fretboard_h + EOF_EDITOR_RENDER_OFFSET))
		{
			if(eof_hover_note >= 0)
			{
				if(eof_count_selected_notes(NULL, 0) <= 0)
				{
					eof_selection.current = eof_hover_note;
					eof_selection.current_pos = eof_song->legacy_track[tracknum]->note[eof_selection.current]->pos;
					if(eof_selection.track != eof_selected_track)
					{
						eof_selection.track = eof_selected_track;
						memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
					}
					eof_selection.multi[eof_selection.current] = 1;
					eof_render();
				}
				else
				{
					if(!eof_selection.multi[eof_hover_note])
					{
						eof_selection.current = eof_hover_note;
						eof_selection.current_pos = eof_song->legacy_track[tracknum]->note[eof_selection.current]->pos;
						memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
						eof_selection.multi[eof_selection.current] = 1;
						eof_render();
					}
				}
			}
			eof_prepare_menus();
			if(eof_count_selected_notes(NULL, 0) > 0)
			{
				do_menu(eof_right_click_menu_note, mouse_x, mouse_y);
			}
			else
			{
				do_menu(eof_right_click_menu_normal, mouse_x, mouse_y);
			}
			eof_clear_input();
		}
		else if(mouse_y < eof_window_3d->y)
		{
			eof_prepare_menus();
			do_menu(eof_right_click_menu_normal, mouse_x, mouse_y);
			eof_clear_input();
		}
		eof_show_mouse(NULL);
	}
}

void eof_vocal_editor_logic(void)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	int use_this_x = mouse_x;
	int next_note_pos = 0;
	EOF_SNAP_DATA drag_snap; // help with dragging across snap locations

	if(!eof_song_loaded)
		return;
	if(!eof_vocals_selected)
		return;

	eof_editor_logic_common();

	if(eof_music_paused)
	{	//If the chart is paused
		int npos;

		if(!(mouse_b & 1) && !(mouse_b & 2) && !key[KEY_INSERT])
		{
			eof_undo_toggle = 0;
			if(eof_notes_moved)
			{
				eof_vocal_track_sort_lyrics(eof_song->vocal_track[tracknum]);
				eof_vocal_track_fixup_lyrics(eof_song->vocal_track[tracknum], 1);
				eof_notes_moved = 0;
			}
		}

		/* mouse is in the mini keyboard area */
		if((mouse_x < 20) && (mouse_y >= eof_window_editor->y + 25 + EOF_EDITOR_RENDER_OFFSET) && (mouse_y < eof_window_editor->y + eof_screen_layout.fretboard_h + EOF_EDITOR_RENDER_OFFSET))
		{
			eof_pen_lyric.pos = -1;
			eof_pen_lyric.length = 1;
			eof_pen_lyric.note = eof_vocals_offset + (EOF_EDITOR_RENDER_OFFSET + 35 + eof_screen_layout.vocal_y - mouse_y) / eof_screen_layout.vocal_tail_size;
			if((eof_pen_lyric.note < eof_vocals_offset) || (eof_pen_lyric.note >= eof_vocals_offset + eof_screen_layout.vocal_view_size))
			{
				eof_pen_lyric.note = 0;
			}
			if(!(mouse_b & 1))
			{
				eof_last_tone = 0;
			}
			if(mouse_b & 1)
			{
				int n = eof_vocals_offset + (EOF_EDITOR_RENDER_OFFSET + 35 + eof_screen_layout.vocal_y - mouse_y) / eof_screen_layout.vocal_tail_size;
				if((n >= eof_vocals_offset) && (n < eof_vocals_offset + eof_screen_layout.vocal_view_size) && (n != eof_last_tone))
				{
					eof_mix_play_note(n);
					eof_last_tone = n;
				}
			}
		}

		/* mouse is in the fretboard area */
		else if((mouse_y >= eof_window_editor->y + 25 + EOF_EDITOR_RENDER_OFFSET) && (mouse_y < eof_window_editor->y + eof_screen_layout.fretboard_h + EOF_EDITOR_RENDER_OFFSET))
		{
			int pos = eof_music_pos / eof_zoom;
			int lpos = pos < 300 ? (mouse_x - 20) * eof_zoom : ((pos - 300) + mouse_x - 20) * eof_zoom;
			int rpos = 0; // place to store pen_lyric.pos in case we are hovering over a note and need the original position before it was changed to the note location
			eof_snap_logic(&eof_snap, lpos);
			eof_snap_length_logic(&eof_snap);
			eof_pen_lyric.pos = eof_snap.pos;
			rpos = eof_pen_lyric.pos;
			eof_pen_lyric.length = eof_snap.length;
			if(key[KEY_BACKSPACE])
			{	//If entering a vocal percussion note
				eof_pen_lyric.note = EOF_LYRIC_PERCUSSION;
			}
			else
			{
				eof_pen_lyric.note = eof_vocals_offset + (EOF_EDITOR_RENDER_OFFSET + 35 + eof_screen_layout.vocal_y - mouse_y) / eof_screen_layout.vocal_tail_size;
			}
			if((eof_pen_lyric.note < eof_vocals_offset) || (eof_pen_lyric.note >= eof_vocals_offset + eof_screen_layout.vocal_view_size))
			{
				eof_pen_lyric.note = 0;
			}
			use_this_x = lpos;
			eof_pen_visible = 1;
			for(i = 0; (i < eof_song->vocal_track[tracknum]->lyrics) && (eof_hover_note < 0); i++)
			{
				npos = eof_song->vocal_track[tracknum]->lyric[i]->pos;
				if((use_this_x > npos) && (use_this_x < npos + eof_song->vocal_track[tracknum]->lyric[i]->length))
				{
					eof_hover_note = i;
				}
				else if((eof_pen_lyric.pos > npos - (6 * eof_zoom)) && (eof_pen_lyric.pos < npos + (6 * eof_zoom)))
				{
					eof_hover_note = i;
				}
				if((npos > eof_pen_lyric.pos) && (next_note_pos == 0))
				{
					next_note_pos = npos;
				}
			}
			if((eof_hover_note >= 0) && !(mouse_b & 1))
			{
				eof_pen_lyric.pos = eof_song->vocal_track[tracknum]->lyric[eof_hover_note]->pos;
				eof_pen_lyric.length = eof_song->vocal_track[tracknum]->lyric[eof_hover_note]->length;
			}
			if((eof_input_mode == EOF_INPUT_PIANO_ROLL) || (eof_input_mode == EOF_INPUT_REX))
			{
				if(eof_hover_note >= 0)
				{
					if(!eof_mouse_drug)
					{
						eof_pen_lyric.pos = eof_song->vocal_track[tracknum]->lyric[eof_hover_note]->pos;
					}
				}
			}

			/* handle initial click */
			if((mouse_b & 1) && eof_lclick_released)
			{
				int ignore_range = 0;
				eof_click_x = mouse_x;
				eof_click_y = mouse_y;
				eof_lclick_released = 0;
				if(eof_hover_note >= 0)
				{
					if(eof_selection.current != EOF_MAX_NOTES - 1)
					{
						eof_selection.last = eof_selection.current;
						eof_selection.last_pos = eof_selection.current_pos;
					}
					else
					{
						eof_selection.last = eof_hover_note;
						eof_selection.last_pos = eof_song->vocal_track[tracknum]->lyric[eof_selection.last]->pos;
					}
					eof_selection.current = eof_hover_note;
					if(eof_mix_vocal_tones_enabled)
					{
						if(eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->note != EOF_LYRIC_PERCUSSION)
							eof_mix_play_note(eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->note);
						else
							play_sample(eof_sound_chosen_percussion, 255.0 * (eof_percussion_volume / 100.0), 127, 1000 + eof_audio_fine_tune, 0);
					}
					eof_selection.current_pos = eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->pos;
					if(KEY_EITHER_SHIFT)
					{
						if((eof_selection.range_pos_1 == 0) && (eof_selection.range_pos_2 == 0))
						{
							ignore_range = 1;
							eof_selection.range_pos_1 = eof_selection.current_pos;
							eof_selection.range_pos_2 = eof_selection.current_pos;
						}
						else
						{
							if(eof_selection.current_pos > eof_selection.range_pos_2)
							{
								eof_selection.range_pos_2 = eof_selection.current_pos;
							}
							else
							{
								eof_selection.range_pos_1 = eof_selection.current_pos;
							}
						}
					}
					else
					{
						eof_selection.range_pos_1 = eof_selection.current_pos;
						eof_selection.range_pos_2 = eof_selection.current_pos;
					}
					eof_pegged_note = eof_selection.current;
					eof_peg_x = lpos - eof_song->vocal_track[tracknum]->lyric[eof_pegged_note]->pos;
					eof_last_pen_pos = rpos;
					if(!KEY_EITHER_CTRL)
					{
						/* Shift+Click selects range */
						if(KEY_EITHER_SHIFT && !ignore_range)
						{
							if(eof_selection.range_pos_1 < eof_selection.range_pos_2)
							{
								for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
								{
									if((eof_song->vocal_track[tracknum]->lyric[i]->pos >= eof_selection.range_pos_1) && (eof_song->vocal_track[tracknum]->lyric[i]->pos <= eof_selection.range_pos_2))
									{
										if(eof_selection.track != EOF_TRACK_VOCALS)
										{
											eof_selection.track = EOF_TRACK_VOCALS;
											memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
										}
										eof_selection.multi[i] = 1;
										eof_undo_last_type = EOF_UNDO_TYPE_NONE;
									}
								}
							}
							else
							{
								for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
								{
									if((eof_song->vocal_track[tracknum]->lyric[i]->pos >= eof_selection.range_pos_2) && (eof_song->vocal_track[tracknum]->lyric[i]->pos <= eof_selection.range_pos_1))
									{
										if(eof_selection.track != EOF_TRACK_VOCALS)
										{
											eof_selection.track = EOF_TRACK_VOCALS;
											memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
										}
										eof_selection.multi[i] = 1;
										eof_undo_last_type = EOF_UNDO_TYPE_NONE;
									}
								}
							}
						}
						else
						{
							if(!eof_selection.multi[eof_selection.current])
							{
								memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
							}
							if(eof_selection.multi[eof_selection.current] == 1)
							{
								if(eof_selection.track != EOF_TRACK_VOCALS)
								{
									eof_selection.track = EOF_TRACK_VOCALS;
									memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
								}
								eof_selection.multi[eof_selection.current] = 2;
								eof_undo_last_type = EOF_UNDO_TYPE_NONE;
							}
							else
							{
								if(eof_selection.track != EOF_TRACK_VOCALS)
								{
									eof_selection.track = EOF_TRACK_VOCALS;
									memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
								}
								eof_selection.multi[eof_selection.current] = 1;
								eof_undo_last_type = EOF_UNDO_TYPE_NONE;
							}
						}
					}

					/* Ctrl+Click adds to selected notes */
					else
					{
						if(eof_selection.multi[eof_selection.current] == 1)
						{
							if(eof_selection.track != EOF_TRACK_VOCALS)
							{
								eof_selection.track = EOF_TRACK_VOCALS;
								memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
							}
							eof_selection.multi[eof_selection.current] = 2;
							eof_undo_last_type = EOF_UNDO_TYPE_NONE;
						}
						else
						{
							if(eof_selection.track != EOF_TRACK_VOCALS)
							{
								eof_selection.track = EOF_TRACK_VOCALS;
								memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
							}
							eof_selection.multi[eof_selection.current] = 1;
							eof_undo_last_type = EOF_UNDO_TYPE_NONE;
						}
					}
				}

				/* clicking on no note deselects all notes */
				else
				{
					if(!KEY_EITHER_SHIFT && !KEY_EITHER_CTRL)
					{
						memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
						eof_selection.current = EOF_MAX_NOTES - 1;
						eof_selection.current_pos = 0;
						eof_selection.range_pos_1 = 0;
						eof_selection.range_pos_2 = 0;
					}
				}
			}
			if(!(mouse_b & 1))
			{
				if(!eof_lclick_released)
				{
					eof_lclick_released++;
				}

				/* mouse button just released, check to see what needs doing */
				else if(eof_lclick_released == 1)
				{
					if(!eof_mouse_drug)
					{
						if(!KEY_EITHER_CTRL)
						{
							if(KEY_EITHER_SHIFT)
							{
							}
							else
							{
								memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
								if(eof_selection.multi[eof_selection.current])
								{
									eof_selection.multi[eof_selection.current] = 0;
								}
								else
								{
									eof_selection.track = EOF_TRACK_VOCALS;
									eof_selection.multi[eof_selection.current] = 1;
								}
								eof_undo_last_type = EOF_UNDO_TYPE_NONE;
							}
						}
						else
						{
							if(eof_selection.multi[eof_selection.current] == 2)
							{
								eof_selection.multi[eof_selection.current] = 0;
								eof_undo_last_type = EOF_UNDO_TYPE_NONE;
							}
							else if(eof_hover_note >= 0)
							{
								if(eof_selection.track != EOF_TRACK_VOCALS)
								{
									eof_selection.track = EOF_TRACK_VOCALS;
									memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
								}
								eof_selection.multi[eof_selection.current] = 1;
								eof_undo_last_type = EOF_UNDO_TYPE_NONE;
							}
						}
					}
					eof_lclick_released++;
				}
				else
				{
					if(eof_mouse_drug)
					{
						eof_mouse_drug = 0;
					}
				}
			}
			int move_offset = 0;
			int revert = 0;
			int revert_amount = 0;
			if((mouse_b & 1) && !eof_lclick_released)
			{
				if(eof_mouse_drug)
				{
					eof_mouse_drug++;
					if(eof_mouse_drug == 11)
					{
						eof_mickeys_x = mouse_x - eof_click_x;
					}
				}
				if((eof_mickeys_x != 0) && !eof_mouse_drug)
				{
//					eof_mouse_drug = 1;
					eof_mouse_drug++;
				}
				if(pos < 300)
				{
					npos = 20;
				}
				else
				{
					npos = 20 - (pos - 300);
				}
				if((eof_mouse_drug > 10) && (eof_selection.current != EOF_MAX_NOTES - 1))
				{
					if((eof_snap_mode != EOF_SNAP_OFF) && !KEY_EITHER_CTRL)
					{
						eof_snap_logic(&drag_snap, lpos - eof_peg_x);
						move_offset = (int)drag_snap.pos - (int)eof_song->vocal_track[tracknum]->lyric[eof_pegged_note]->pos;
						eof_last_pen_pos = rpos;
					}
					if(!eof_undo_toggle && ((move_offset != 0) || (eof_snap_mode == EOF_SNAP_OFF) || KEY_EITHER_CTRL))
					{
						eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					}
					eof_notes_moved = 1;
					for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
					{
						if(eof_selection.multi[i])
						{
							if((eof_snap_mode == EOF_SNAP_OFF) || KEY_EITHER_CTRL)
							{
								if(eof_song->vocal_track[tracknum]->lyric[i]->pos == eof_selection.current_pos)
								{
									eof_selection.current_pos += eof_mickeys_x * eof_zoom;
								}
								eof_song->vocal_track[tracknum]->lyric[i]->pos += eof_mickeys_x * eof_zoom;
							}
							else
							{
								if(eof_song->vocal_track[tracknum]->lyric[i]->pos == eof_selection.current_pos)
								{
									eof_selection.current_pos += move_offset;
								}
								eof_song->vocal_track[tracknum]->lyric[i]->pos += move_offset;
							}
							if(eof_song->vocal_track[tracknum]->lyric[i]->pos + eof_song->vocal_track[tracknum]->lyric[i]->length >= eof_music_length)
							{
								revert = 1;
								revert_amount = eof_song->vocal_track[tracknum]->lyric[i]->pos + eof_song->vocal_track[tracknum]->lyric[i]->length - eof_music_length;
							}
						}
					}
					if(revert)
					{
						for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
						{
							if(eof_selection.multi[i])
							{
								eof_song->vocal_track[tracknum]->lyric[i]->pos -= revert_amount;
							}
						}
					}
				}
			}
			if((((eof_input_mode != EOF_INPUT_REX) && ((mouse_b & 2) || key[KEY_INSERT])) || (((eof_input_mode == EOF_INPUT_REX) && !KEY_EITHER_SHIFT && !KEY_EITHER_CTRL && (key[KEY_1] || key[KEY_2] || key[KEY_3] || key[KEY_4] || key[KEY_5] || key[KEY_6])) && eof_rclick_released && (eof_pen_lyric.pos < eof_music_length))) || key[KEY_BACKSPACE])
			{
				eof_selection.range_pos_1 = 0;
				eof_selection.range_pos_2 = 0;
				eof_rclick_released = 0;
				if(eof_hover_note >= 0)
				{
					if(!eof_undo_toggle)
					{
						eof_prepare_undo(EOF_UNDO_TYPE_NOTE_SEL);
					}

					/* delete whole lyric if clicking word */
					if((mouse_y >= EOF_EDITOR_RENDER_OFFSET + 35 + eof_screen_layout.lyric_y) && (mouse_y < EOF_EDITOR_RENDER_OFFSET + 35 + eof_screen_layout.lyric_y + 16))
					{
						eof_track_delete_note(eof_song, eof_selected_track, eof_hover_note);	//Delete the hovered over lyric
						eof_selection.current = EOF_MAX_NOTES - 1;
						eof_vocal_track_sort_lyrics(eof_song->vocal_track[tracknum]);
					}

					/* delete only the note */
					else if(eof_pen_lyric.note == eof_song->vocal_track[tracknum]->lyric[eof_hover_note]->note)
					{
						if(!eof_check_string(eof_song->vocal_track[tracknum]->lyric[eof_hover_note]->text))	//If removing the pitch from a note that is already textless
						{	//Perform the cleanup code as if the lyric was deleted by inputting on top of the lyric text as above
							eof_track_delete_note(eof_song, eof_selected_track, eof_hover_note);	//Delete the hovered over lyric
							eof_selection.current = EOF_MAX_NOTES - 1;
							eof_vocal_track_sort_lyrics(eof_song->vocal_track[tracknum]);
						}
						else	//Just remove the pitch
						{
							eof_song->vocal_track[tracknum]->lyric[eof_hover_note]->note = 0;
							eof_selection.current = eof_hover_note;
							eof_selection.track = eof_selected_track;
							memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
							eof_selection.multi[eof_selection.current] = 1;
							eof_selection.current_pos = eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->pos;
							eof_selection.range_pos_1 = eof_selection.current_pos;
							eof_selection.range_pos_2 = eof_selection.current_pos;
						}
					}

					/* move note */
					else
					{
						eof_song->vocal_track[tracknum]->lyric[eof_hover_note]->note = eof_pen_lyric.note;
						eof_selection.current = eof_hover_note;
						eof_selection.track = EOF_TRACK_VOCALS;
						memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
						eof_selection.multi[eof_selection.current] = 1;
						eof_selection.current_pos = eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->pos;
						eof_selection.range_pos_1 = eof_selection.current_pos;
						eof_selection.range_pos_2 = eof_selection.current_pos;
						if(eof_mix_vocal_tones_enabled)
						{
							eof_mix_play_note(eof_pen_lyric.note);
						}
					}
				}

				/* create new note */
				else
				{
					if(key[KEY_BACKSPACE])
					{	//Map the percussion note here
						eof_pen_lyric.note = EOF_LYRIC_PERCUSSION;
						key[KEY_BACKSPACE] = 0;
					}
					if(eof_mix_vocal_tones_enabled)
					{
						if(eof_pen_lyric.note == EOF_LYRIC_PERCUSSION)
						{	//Play the percussion at the user-specified cue volume
							play_sample(eof_sound_chosen_percussion, 255.0 * (eof_percussion_volume / 100.0), 127, 1000 + eof_audio_fine_tune, 0);
						}
						else
						{
							eof_mix_play_note(eof_pen_lyric.note);
						}
					}
					eof_new_lyric_dialog();
				}
			}
			if(eof_input_mode == EOF_INPUT_REX)
			{
				if(!key[KEY_1] && !key[KEY_2] && !key[KEY_3] && !key[KEY_4] && !key[KEY_5] && !key[KEY_6])
				{
					eof_rclick_released = 1;
				}
			}
			else
			{
				if(!(mouse_b & 2) && !key[KEY_INSERT])
				{
					eof_rclick_released = 1;
				}
			}
			if((eof_mickey_z != 0) && eof_count_selected_notes(NULL, 0))
			{
				eof_prepare_undo(EOF_UNDO_TYPE_NOTE_LENGTH);
			}
			if(eof_snap_mode == EOF_SNAP_OFF)
			{
				if(KEY_EITHER_CTRL)
				{
					for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
					{
						if((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i])
						{
							eof_song->vocal_track[tracknum]->lyric[i]->length -= eof_mickey_z * 10;
						}
					}
				}
				else
				{
					for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
					{
						if((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i])
						{
							eof_song->vocal_track[tracknum]->lyric[i]->length -= eof_mickey_z * 100;
						}
					}
				}
			}
			else
			{
				int b;
				if(eof_mickey_z > 0)
				{
					for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
					{
						if((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i])
						{
							b = eof_get_beat(eof_song, eof_song->vocal_track[tracknum]->lyric[i]->pos + eof_song->vocal_track[tracknum]->lyric[i]->length - 1);
							if(b >= 0)
							{
								eof_snap_logic(&eof_tail_snap, eof_song->beat[b]->pos);
							}
							else
							{
								eof_snap_logic(&eof_tail_snap, eof_song->vocal_track[tracknum]->lyric[i]->pos + eof_song->vocal_track[tracknum]->lyric[i]->length - 1);
							}
							eof_snap_length_logic(&eof_tail_snap);
//							allegro_message("%d, %d\n%lu, %lu", eof_tail_snap.length, eof_tail_snap.beat, eof_song->legacy_track[tracknum]->note[i]->pos + eof_song->legacy_track[tracknum]->note[i]->length, eof_song->beat[eof_tail_snap.beat]->pos);
							eof_song->vocal_track[tracknum]->lyric[i]->length -= eof_tail_snap.length;
							if(eof_song->vocal_track[tracknum]->lyric[i]->length > 1)
							{
								eof_snap_logic(&eof_tail_snap, eof_song->vocal_track[tracknum]->lyric[i]->pos + eof_song->vocal_track[tracknum]->lyric[i]->length);
								eof_set_vocal_tail_pos(i, eof_tail_snap.pos);
							}
						}
					}
				}
				else if(eof_mickey_z < 0)
				{
					for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
					{
						if((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i])
						{
							eof_snap_logic(&eof_tail_snap, eof_song->vocal_track[tracknum]->lyric[i]->pos + eof_song->vocal_track[tracknum]->lyric[i]->length + 1);
							if(eof_tail_snap.pos > eof_song->vocal_track[tracknum]->lyric[i]->pos + eof_song->vocal_track[tracknum]->lyric[i]->length + 1)
							{
								eof_set_vocal_tail_pos(i, eof_tail_snap.pos);
							}
							else
							{
								eof_snap_length_logic(&eof_tail_snap);
								eof_song->vocal_track[tracknum]->lyric[i]->length += eof_tail_snap.length;
								eof_snap_logic(&eof_tail_snap, eof_song->vocal_track[tracknum]->lyric[i]->pos + eof_song->vocal_track[tracknum]->lyric[i]->length);
								eof_set_vocal_tail_pos(i, eof_tail_snap.pos);
							}
						}
					}
				}
			}
			if(eof_mickey_z != 0)
			{
				eof_vocal_track_fixup_lyrics(eof_song->vocal_track[tracknum], 1);
			}
			if(key[KEY_P] && !KEY_EITHER_CTRL)
			{
				if(eof_pen_lyric.note != eof_last_tone)
				{
					eof_last_tone = eof_pen_lyric.note;
					eof_mix_play_note(eof_pen_lyric.note);
				}
			}
			if(!key[KEY_P])
			{
				eof_last_tone = -1;
			}
		}
		else
		{
			eof_pen_visible = 0;
		}
	}//If the chart is paused
	else
	{	//If the chart is not paused
		int pos = eof_music_pos / eof_zoom;
		int npos;
		for(i = 0; i < eof_song->beats - 1; i++)
		{
			if((eof_music_pos >= eof_song->beat[i]->pos) && (eof_music_pos < eof_song->beat[i + 1]->pos))
			{
				if(eof_hover_beat_2 != i)
				{
					eof_hover_beat_2 = i;
				}
				break;
			}
		}
		for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
		{
			npos = eof_song->vocal_track[tracknum]->lyric[i]->pos / eof_zoom;
			if((pos - eof_av_delay / eof_zoom > npos) && (pos - eof_av_delay / eof_zoom < npos + (eof_song->vocal_track[tracknum]->lyric[i]->length > 100 ? eof_song->vocal_track[tracknum]->lyric[i]->length : 100) / eof_zoom))
			{
				eof_hover_note = i;
				eof_pen_lyric.note = eof_song->vocal_track[tracknum]->lyric[i]->note;
				break;
			}
		}
		if(i == eof_song->vocal_track[tracknum]->lyrics)
		{
			eof_hover_note = -1;
			eof_pen_lyric.note = 0;
		}
		for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
		{
			npos = eof_song->vocal_track[tracknum]->lyric[i]->pos;
			if((eof_music_pos - eof_av_delay > npos) && (eof_music_pos - eof_av_delay < npos + (eof_song->vocal_track[tracknum]->lyric[i]->length > 100 ? eof_song->vocal_track[tracknum]->lyric[i]->length : 100)))
			{
				eof_hover_note_2 = i;
				eof_hover_lyric = i;
				break;
			}
		}
	}//If the chart is not paused

	if(((mouse_b & 2) || key[KEY_INSERT]) && (eof_input_mode == EOF_INPUT_REX))
	{
		eof_emergency_stop_music();
		eof_render();
		eof_show_mouse(screen);
		if((mouse_y >= eof_window_editor->y + EOF_EDITOR_RENDER_OFFSET - 4) && (mouse_y < eof_window_editor->y + EOF_EDITOR_RENDER_OFFSET + 18))
		{
			int pos = eof_music_pos / eof_zoom;
//			int lpos = pos < 300 ? (mouse_x - 20) * eof_zoom : ((pos - 300) + mouse_x - 20) * eof_zoom;
			int lpos = pos < 300 ? (eof_song->beat[eof_selected_beat]->pos / eof_zoom + 20) : 300;
			eof_prepare_menus();
			do_menu(eof_beat_menu, lpos, mouse_y);
			eof_clear_input();
		}
		else if((mouse_y >= eof_window_editor->y + 25 + EOF_EDITOR_RENDER_OFFSET) && (mouse_y < eof_window_editor->y + eof_screen_layout.fretboard_h + EOF_EDITOR_RENDER_OFFSET))
		{
			if(eof_hover_note >= 0)
			{
				if(eof_count_selected_notes(NULL, 0) <= 0)
				{
					eof_selection.current = eof_hover_note;
					eof_selection.current_pos = eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->pos;
					if(eof_selection.track != EOF_TRACK_VOCALS)
					{
						eof_selection.track = EOF_TRACK_VOCALS;
						memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
					}
					eof_selection.multi[eof_selection.current] = 1;
					eof_render();
				}
				else
				{
					if(!eof_selection.multi[eof_hover_note])
					{
						eof_selection.current = eof_hover_note;
						eof_selection.current_pos = eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->pos;
						memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
						eof_selection.multi[eof_selection.current] = 1;
						eof_render();
					}
				}
			}
			eof_prepare_menus();
			if(eof_count_selected_notes(NULL, 0) > 0)
			{
				do_menu(eof_right_click_menu_note, mouse_x, mouse_y);
				eof_clear_input();
			}
			else
			{
				do_menu(eof_right_click_menu_normal, mouse_x, mouse_y);
				eof_clear_input();
			}
		}
		else if(mouse_y < eof_window_3d->y)
		{
			eof_prepare_menus();
			do_menu(eof_right_click_menu_normal, mouse_x, mouse_y);
			eof_clear_input();
		}
		eof_show_mouse(NULL);
	}
}

int eof_get_ts_text(int beat, char * buffer)
{
	int ret = 0;
	if(eof_song->beat[beat]->flags & EOF_BEAT_FLAG_START_4_4)
	{
		ustrcpy(buffer, "4/4");
		ret = 1;
	}
	else if(eof_song->beat[beat]->flags & EOF_BEAT_FLAG_START_3_4)
	{
		ustrcpy(buffer, "3/4");
		ret = 1;
	}
	else if(eof_song->beat[beat]->flags & EOF_BEAT_FLAG_START_5_4)
	{
		ustrcpy(buffer, "5/4");
		ret = 1;
	}
	else if(eof_song->beat[beat]->flags & EOF_BEAT_FLAG_START_6_4)
	{
		ustrcpy(buffer, "6/4");
		ret = 1;
	}
	else if(eof_song->beat[beat]->flags & EOF_BEAT_FLAG_CUSTOM_TS)
	{
		uszprintf(buffer, 16, "%lu/%lu", ((eof_song->beat[beat]->flags & 0xFF000000)>>24)+1, ((eof_song->beat[beat]->flags & 0x00FF0000)>>16)+1);
		ret = 1;
	}
	else
	{
		ustrcpy(buffer, "");
	}
	return ret;
}

void eof_render_editor_window(void)
{
	unsigned long start;	//Will store the timestamp of the left visible edge of the piano roll
	unsigned long i,numnotes;

	if(!eof_song_loaded)
		return;

	eof_render_editor_window_common();	//Perform rendering that is common to the note and the vocal editor displays

	numnotes = eof_track_get_size(eof_song, eof_selected_track);	//Get the number of notes in this legacy/pro guitar track
	start = eof_determine_piano_roll_left_edge();
	for(i = 0; i < numnotes; i++)
	{	//Render all visible notes in the list
		if((eof_note_type == eof_get_note_difficulty(eof_selected_track, i)) && (eof_get_note_pos(eof_selected_track, i) + eof_get_note_length(eof_selected_track, i) >= start))
		{	//If this note is in the selected instrument difficulty and would render at or after the left edge of the piano roll
			if(((eof_input_mode == EOF_INPUT_PIANO_ROLL) || (eof_input_mode == EOF_INPUT_REX)) && eof_music_paused && (eof_hover_note == i))
			{	//If the input mode is piano roll or rex mundi and the chart is paused, and the note is being moused over
				eof_note_draw(eof_selected_track, i, ((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && eof_music_paused) ? 1 : i == eof_hover_note ? 2 : 3, eof_window_editor);
			}
			else
			{	//Render the note normally
				if(eof_note_draw(eof_selected_track, i, ((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && eof_music_paused) ? 1 : i == eof_hover_note ? 2 : 0, eof_window_editor) == 1)
					break;	//If this note was rendered right of the viewable area, all following notes will too, so stop rendering
			}
		}
	}
	if(eof_music_paused && eof_pen_visible && (eof_pen_note.pos < eof_music_length))
	{
		if(!eof_mouse_drug)
		{
			if((eof_input_mode == EOF_INPUT_PIANO_ROLL) || (eof_input_mode == EOF_INPUT_REX))
			{
				eof_note_draw(0, 0, 3, eof_window_editor);	//Render the pen note
			}
			else
			{
				eof_note_draw(0, 0, 0, eof_window_editor);	//Render the pen note
			}
		}
	}

	eof_render_editor_window_common2();
}

void eof_render_vocal_editor_window(void)
{
	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	int pos = eof_music_pos / eof_zoom;	//Current seek position
	int lpos;							//The position of the first beat marker
	unsigned long start;	//Will store the timestamp of the left visible edge of the piano roll

	if(!eof_song_loaded)
		return;
	if(!eof_vocals_selected)
		return;

	eof_render_editor_window_common();

	if(pos < 300)
	{
		lpos = 20;
	}
	else
	{
		lpos = 20 - (pos - 300);
	}

	/* clear lyric text area */
	rectfill(eof_window_editor->screen, 0, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.lyric_y + 1, eof_window_editor->w - 1, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.lyric_y + 1 + 16, eof_color_black);
	hline(eof_window_editor->screen, lpos, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.lyric_y + 1 + 16, lpos + (eof_music_length) / eof_zoom, eof_color_white);

	/* draw lyric lines */
	for(i = 0; i < eof_song->vocal_track[tracknum]->lines; i++)
	{ 	//The -5 is a vertical offset to allow the phrase marker rectangle render as high as the lyric text itself.  The +4 is an offset to allow the rectangle to render as low as the lyric text
		rectfill(eof_window_editor->screen, lpos + eof_song->vocal_track[tracknum]->line[i].start_pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + (15-5) + eof_screen_layout.note_y[0] - 2 + 8, lpos + eof_song->vocal_track[tracknum]->line[i].end_pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[0] + 2 + 8 +4, (eof_song->vocal_track[tracknum]->line[i].flags & EOF_LYRIC_LINE_FLAG_OVERDRIVE) ? makecol(64, 128, 64) : makecol(0, 0, 127));
	}

	start = eof_determine_piano_roll_left_edge();
	for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
	{
		if(eof_song->vocal_track[tracknum]->lyric[i]->pos + eof_song->vocal_track[tracknum]->lyric[i]->length >= start)
		{	//If the lyric would render at or after the left edge of the piano roll
			if(((eof_input_mode == EOF_INPUT_PIANO_ROLL) || (eof_input_mode == EOF_INPUT_REX)) && eof_music_paused && (eof_hover_note == i))
			{	//If the chart is paused and the lyric is moused over
				eof_lyric_draw(eof_song->vocal_track[tracknum]->lyric[i], ((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i] && eof_music_paused) ? 1 : i == eof_hover_note ? 2 : 3, eof_window_editor);
			}
			else
			{
				if(eof_lyric_draw(eof_song->vocal_track[tracknum]->lyric[i], ((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i] && eof_music_paused) ? 1 : i == eof_hover_note ? 2 : 0, eof_window_editor) > 0)
					break;	//Break if the function indicated that the lyric was rendered beyond the clip window
			}
		}
	}
	if(eof_hover_note >= 0)
	{
		eof_lyric_draw(eof_song->vocal_track[tracknum]->lyric[eof_hover_note], 2, eof_window_editor);
	}
	if(eof_music_paused && eof_pen_visible && (eof_pen_note.pos < eof_music_length))
	{
		if(!eof_mouse_drug)
		{
			if((eof_input_mode == EOF_INPUT_PIANO_ROLL) || (eof_input_mode == EOF_INPUT_REX))
			{
				eof_lyric_draw(&eof_pen_lyric, 3, eof_window_editor);
			}
			else
			{
				eof_lyric_draw(&eof_pen_lyric, 0, eof_window_editor);
			}
		}
	}

	/* draw mini keyboard */
	int kcol, kcol2;
	int n;
	int ny;
	int red = 0;
	for(i = 0; i < eof_screen_layout.vocal_view_size; i++)
	{
		n = (eof_vocals_offset + i) % 12;
		ny = EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.vocal_y - (i % eof_screen_layout.vocal_view_size + 1) * eof_screen_layout.vocal_tail_size;
		if(eof_vocals_offset + i == eof_pen_lyric.note)
		{
			kcol = eof_color_green;
			kcol2 = makecol(0, 192, 0);
			if((n == 0) && !red)
			{
				red = 1;
			}
			textprintf_ex(eof_window_editor->screen, font, 21, ny + eof_screen_layout.vocal_tail_size / 2 - text_height(font) / 2, eof_color_white, eof_color_black, "%s", eof_get_tone_name(eof_pen_lyric.note));
		}
		else if((n == 1) || (n == 3) || (n == 6) || (n == 8) || (n == 10))
		{
			kcol = makecol(16, 16, 16);
			kcol2 = eof_color_black;
		}
		else if(n == 0)
		{
			kcol = makecol(255, 160, 160);
			kcol2 = makecol(192, 96, 96);
			red = 1;
		}
		else
		{
			kcol = eof_color_white;
			kcol2 = eof_color_silver;
		}
		rectfill(eof_window_editor->screen, 0, ny, 19, ny + eof_screen_layout.vocal_tail_size - 1, kcol);
		hline(eof_window_editor->screen, 0, ny + eof_screen_layout.vocal_tail_size - 1, 19, kcol2);
	}

	eof_render_editor_window_common2();
}

unsigned long eof_determine_piano_roll_left_edge(void)
{
	unsigned long pos = eof_music_pos / eof_zoom;

	if(pos <= 320)
	{
		return 0;
	}
	else
	{	//Return the lowest timestamp that would be displayable given the current seek position and zoom level,
		return ((pos - 320) * eof_zoom);
	}
}

unsigned long eof_determine_piano_roll_right_edge(void)
{
	unsigned long pos = eof_music_pos / eof_zoom;

	if(pos < 300)
	{
		return ((eof_window_editor->screen->w - 1) - 20) * eof_zoom;
	}
	else
	{
		return ((eof_window_editor->screen->w - 1) - 320 + pos) * eof_zoom;
	}
}

void eof_render_editor_window_common(void)
{
	unsigned long i, j;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	int pos = eof_music_pos / eof_zoom;	//Current seek position
	int lpos;							//The position of the first beatmarker
	int pmin = 0;
	int psec = 0;
	int xcoord;							//Used to cache x coordinate to reduce recalculations
	int col,col2;						//Temporary color variables
	unsigned long start;				//Will store the timestamp of the left visible edge of the piano roll
	unsigned long numlanes;				//The number of fretboard lanes that will be rendered

	if(!eof_song_loaded)
		return;

	numlanes = eof_count_track_lanes(eof_selected_track);

	/* draw the starting position */
	if(pos < 300)
	{
		lpos = 20;
	}
	else
	{
		lpos = 20 - (pos - 300);
	}

	start = eof_determine_piano_roll_left_edge();	//Find the timestamp of the left visible edge of the piano roll

	/* fill in window background color */
	rectfill(eof_window_editor->screen, 0, 25 + 8, eof_window_editor->w - 1, eof_window_editor->h - 1, eof_color_gray);

	/* draw the difficulty tabs */
	if(eof_selected_track == EOF_TRACK_VOCALS)
		draw_sprite(eof_window_editor->screen, eof_image[EOF_IMAGE_VTAB0 + eof_vocals_tab], 0, 8);
	else
		draw_sprite(eof_window_editor->screen, eof_image[EOF_IMAGE_TAB0 + eof_note_type], 0, 8);

	/* draw the playback controls */
	if(eof_selected_control < 0)
	{
		draw_sprite(eof_screen, eof_image[EOF_IMAGE_CONTROLS_BASE], eof_screen_layout.controls_x, 22 + 8);
	}
	else
	{
		draw_sprite(eof_screen, eof_image[EOF_IMAGE_CONTROLS_0 + eof_selected_control], eof_screen_layout.controls_x, 22 + 8);
	}
	textprintf_ex(eof_screen, eof_mono_font, eof_screen_layout.controls_x + 153, 23 + 8, eof_color_white, -1, "%02d:%02d", ((eof_music_pos - eof_av_delay) / 1000) / 60, ((eof_music_pos - eof_av_delay) / 1000) % 60);

	/* draw fretboard area */
	rectfill(eof_window_editor->screen, 0, EOF_EDITOR_RENDER_OFFSET + 25, eof_window_editor->w - 1, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 1, eof_color_black);

	if(eof_selected_track != EOF_TRACK_VOCALS)
	{
		col = makecol(0, 0, 64);	//Store dark blue color
		/* draw solo sections */
		for(i = 0; i < eof_song->legacy_track[tracknum]->solos; i++)
		{
			if(eof_song->legacy_track[tracknum]->solo[i].end_pos >= start)	//If the solo section would render at or after the left edge of the piano roll
				rectfill(eof_window_editor->screen, lpos + eof_song->legacy_track[tracknum]->solo[i].start_pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 25, lpos + eof_song->legacy_track[tracknum]->solo[i].end_pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 1, col);
		}
	}

	if(eof_display_waveform)
		eof_render_waveform(eof_waveform);

	for(i = 0; i < numlanes; i++)
	{
		if(!i || (i + 1 >= numlanes))
		{	//Ensure the top and bottom lines extend to the left of the piano roll
			hline(eof_window_editor->screen, lpos, EOF_EDITOR_RENDER_OFFSET + 35 + i * eof_screen_layout.string_space, lpos + (eof_music_length) / eof_zoom, eof_color_white);
		}
		else if(eof_selected_track != EOF_TRACK_VOCALS)
		{	//Otherwise, if not drawing the vocal editor, draw the other fret lines from the first beat marker to the end of the chart
			hline(eof_window_editor->screen, lpos + eof_song->tags->ogg[eof_selected_ogg].midi_offset / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 35 + i * eof_screen_layout.string_space, lpos + (eof_music_length) / eof_zoom, eof_color_white);
		}
	}
	vline(eof_window_editor->screen, lpos + (eof_music_length) / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 35, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 11, eof_color_white);

	/* draw second markers */
	unsigned long msec,roundedstart;
	roundedstart = start / 1000;
	roundedstart *= 1000;		//Roundedstart is start is rounded down to nearest second
	for(msec = roundedstart; msec < start + 12000; msec += 1000)
	{	//Draw up to 12 seconds of markers
		pmin = msec / 60000;		//Find minute count of this second marker
		psec = (msec % 60000)/1000;	//Find second count of this second marker
		if(msec < eof_music_length)
		{
			for(j = 0; j < 10; j++)
			{
				xcoord = lpos + (msec + (j * 100)) / eof_zoom;
				if(xcoord > eof_window_editor->screen->w)	//If this and all remaining second markers would render out of view
					break;
				if(xcoord >= 0)	//If this second marker would be visible
				{
					vline(eof_window_editor->screen, xcoord, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 9, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 1, eof_color_gray);
					if(j == 0)
					{
						vline(eof_window_editor->screen, xcoord, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h + 5, eof_color_white);
					}
					else
					{
						vline(eof_window_editor->screen, xcoord, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h + 1, eof_color_white);
					}
				}
			}
			textprintf_ex(eof_window_editor->screen, eof_mono_font, lpos + (msec / eof_zoom) - 16, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h + 6, eof_color_white, -1, "%02d:%02d", pmin, psec);
		}
	}
	vline(eof_window_editor->screen, lpos, EOF_EDITOR_RENDER_OFFSET + 35, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 10, eof_color_white);

	/* draw beat lines */
	unsigned long current_ppqn = eof_song->beat[0]->ppqn;
	int current_ppqn_used = 0;
	double current_bpm = (double)60000000.0 / (double)eof_song->beat[0]->ppqn;
	int bcol, bscol, bhcol;
	unsigned long beat_counter = 0;
	unsigned beats_per_measure = 0;
	char buffer[16] = {0};
	unsigned long measure_counter=0;
	unsigned long beat_in_measure=0;
	char first_measure = 0;

	bcol = makecol(128, 128, 128);
	bscol = eof_color_white;
	bhcol = eof_color_green;
	col = makecol(112, 112, 112);	//Cache this color
	col2 = makecol(160, 160, 160);	//Cache this color

	for(i = 0; i < eof_song->beats; i++)
	{
		if(eof_get_ts(eof_song,&beats_per_measure,NULL,i) == 1)
		{	//If this beat is a time signature
			first_measure = 1;
			beat_counter = 0;
		}
		beat_in_measure = beat_counter;
		if(first_measure && beat_in_measure == 0)
		{
			measure_counter++;
		}
		xcoord = lpos + eof_song->beat[i]->pos / eof_zoom;

		if((xcoord >= 0) && (xcoord < eof_window_editor->screen->w))
		{	//Only render vertical lines if they would be visible
			vline(eof_window_editor->screen, xcoord, EOF_EDITOR_RENDER_OFFSET + 35 + 1, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 10 - 1, (first_measure && beat_counter == 0) ? eof_color_white : col);
			vline(eof_window_editor->screen, xcoord, EOF_EDITOR_RENDER_OFFSET + 25, EOF_EDITOR_RENDER_OFFSET + 34, eof_color_gray);
			if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_ANCHOR)
			{
				vline(eof_window_editor->screen, xcoord, EOF_EDITOR_RENDER_OFFSET + (i % 2 == 0 ? 19 : 9), EOF_EDITOR_RENDER_OFFSET + 24, eof_color_red);
			}
			else
			{
				vline(eof_window_editor->screen, xcoord, EOF_EDITOR_RENDER_OFFSET + (i % 2 == 0 ? 19 : 9), EOF_EDITOR_RENDER_OFFSET + 24, beat_counter == 0 ? eof_color_white : col2);
			}
		}

		if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_EVENTS)
		{	//Draw event marker
			line(eof_window_editor->screen, xcoord - 3, EOF_EDITOR_RENDER_OFFSET + 24, xcoord + 3, EOF_EDITOR_RENDER_OFFSET + 24, eof_color_yellow);
		}
		if(first_measure && beat_counter == 0)
		{	//If this is a measure marker, draw the measure number to the right of the beat line
			textprintf_ex(eof_window_editor->screen, eof_mono_font, xcoord + 2, EOF_EDITOR_RENDER_OFFSET + 22, eof_color_yellow, -1, "%lu", measure_counter);
		}
		if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_ANCHOR)
		{	//Draw anchor marker
			line(eof_window_editor->screen, xcoord - 3, EOF_EDITOR_RENDER_OFFSET + 21, xcoord, EOF_EDITOR_RENDER_OFFSET + 24, eof_color_red);
			line(eof_window_editor->screen, xcoord + 3, EOF_EDITOR_RENDER_OFFSET + 21, xcoord, EOF_EDITOR_RENDER_OFFSET + 24, eof_color_red);
		}
		if(eof_song->beat[i]->ppqn != current_ppqn)
		{
			current_ppqn = eof_song->beat[i]->ppqn;
			current_ppqn_used = 0;
			current_bpm = (double)60000000.0 / (double)eof_song->beat[i]->ppqn;
		}
		if(i == eof_selected_beat)
		{	//Draw selected beat's tempo
			textprintf_ex(eof_window_editor->screen, eof_mono_font, xcoord - 28, EOF_EDITOR_RENDER_OFFSET + 4 - (i % 2 == 0 ? 0 : 10), i == eof_hover_beat ? bhcol : i == eof_selected_beat ? bscol : bcol, -1, "<%03.2f>", current_bpm);
			current_ppqn_used = 1;
		}
		else if(!current_ppqn_used)
		{	//Draw tempo
			textprintf_ex(eof_window_editor->screen, eof_mono_font, xcoord - 20, EOF_EDITOR_RENDER_OFFSET + 4 - (i % 2 == 0 ? 0 : 10), i == eof_hover_beat ? bhcol : i == eof_selected_beat ? bscol : bcol, -1, "%03.2f", current_bpm);
			current_ppqn_used = 1;
		}
		else
		{	//Draw beat arrow
			textprintf_ex(eof_window_editor->screen, eof_mono_font, xcoord - 20 + 9, EOF_EDITOR_RENDER_OFFSET + 4 - (i % 2 == 0 ? 0 : 10), i == eof_hover_beat ? bhcol : i == eof_selected_beat ? bscol : bcol, -1, "-->");
		}
		if(eof_get_ts_text(i, buffer))
		{	//Draw time signature
			textprintf_centre_ex(eof_window_editor->screen, eof_mono_font, xcoord - 20 + 3 + 16, EOF_EDITOR_RENDER_OFFSET + 4 - (i % 2 == 0 ? 0 : 10) - 12, i == eof_hover_beat ? bhcol : i == eof_selected_beat ? bscol : bcol, -1, "(%s)", buffer);
		}
		beat_counter++;
		if(beat_counter >= beats_per_measure)
		{
			beat_counter = 0;
		}
	}

	/* draw the bookmark position */
	for(i = 0; i < EOF_MAX_BOOKMARK_ENTRIES; i++)
	{
		if(eof_song->bookmark_pos[i] > 0)	//If this bookmark exists
		{
			vline(eof_window_editor->screen, lpos + eof_song->bookmark_pos[i] / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 20, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h + 4, makecol(96, 96, 255));
		}
	}
}

void eof_render_editor_window_common2(void)
{
	int i;
	int pos = eof_music_pos / eof_zoom;	//Current seek position

	if(!eof_song_loaded)
		return;

	/* draw the current position */
	if(pos > eof_av_delay / eof_zoom)
	{
		if(pos < 300)
		{
			vline(eof_window_editor->screen, 20 + pos - eof_av_delay / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 25, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 1, eof_color_green);
		}
		else
		{
			vline(eof_window_editor->screen, 320 - eof_av_delay / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 25, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 1, eof_color_green);
		}
	}

	/* draw the end of song position if necessary*/
	if(eof_music_length != eof_music_actual_length)
	{
		if(pos < 300)
		{
			vline(eof_window_editor->screen, 20 + (eof_music_actual_length) / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 20, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h + 4, eof_color_red);
		}
		else
		{
			vline(eof_window_editor->screen, 20 - ((pos - 300)) + (eof_music_actual_length) / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 20, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h + 4, eof_color_red);
		}
	}

	for(i = 0; i < 5; i++)
	{	//Draw tab difficulty names
		if((eof_selected_track == EOF_TRACK_VOCALS) && (i == eof_vocals_tab))
		{
			textprintf_ex(eof_window_editor->screen, font, 20 + i * 80, 2 + 8, eof_color_black, -1, "%s", eof_vocal_tab_name[i]);
			break;
		}
		if(i == eof_note_type)
		{
			textprintf_ex(eof_window_editor->screen, font, 20 + i * 80, 2 + 8, eof_color_black, -1, "%s", eof_note_type_name[i]);
		}
		else
		{
			textprintf_ex(eof_window_editor->screen, font, 20 + i * 80, 2 + 2 + 8, makecol(128, 128, 128), -1, "%s", eof_note_type_name[i]);
		}
	}

	/* render the scroll bar */
	int scroll_pos = ((float)(eof_screen->w - 8) / (float)eof_music_length) * (float)eof_music_pos;
	draw_sprite(eof_window_editor->screen, eof_image[EOF_IMAGE_SCROLL_BAR], 0, eof_screen_layout.scrollbar_y);
	draw_sprite(eof_window_editor->screen, eof_image[EOF_IMAGE_SCROLL_HANDLE], scroll_pos + 2, eof_screen_layout.scrollbar_y);

	vline(eof_window_editor->screen, 0, 24 + 8, eof_window_editor->h + 4, makecol(160, 160, 160));
	vline(eof_window_editor->screen, 1, 25 + 8, eof_window_editor->h + 4, eof_color_black);
	hline(eof_window_editor->screen, 1, eof_window_editor->h - 2, eof_window_editor->w - 1, makecol(224, 224, 224));
	hline(eof_window_editor->screen, 0, eof_window_editor->h - 1, eof_window_editor->w - 1, eof_color_white);
}

void eof_mark_new_note_as_cymbal(EOF_SONG *sp, unsigned long track, unsigned long notenum)
{	//Perform the eof_mark_edited_note_as_cymbal() logic, applying cymbal status to all relevant frets in the note
	eof_mark_edited_note_as_cymbal(sp,track,notenum,0xFFFF);
}

void eof_mark_edited_note_as_cymbal(EOF_SONG *sp, unsigned long track, unsigned long notenum, unsigned long bitmask)
{
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return;
	tracknum = sp->track[track]->tracknum;

	if(sp->track[track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
	{	//For now, it's assumed that any drum behavior track will be a legacy format track
		if((notenum >= sp->legacy_track[tracknum]->notes) || (sp->legacy_track[tracknum]->note[notenum] == NULL))
			return;

		if((sp->legacy_track[tracknum]->note[notenum]->note & 4) && (bitmask & 4))
		{	//If the note was changed to include a yellow note
			eof_set_flags_at_legacy_note_pos(sp->legacy_track[tracknum],notenum,EOF_NOTE_FLAG_Y_CYMBAL,1);	//Set the yellow cymbal flag on all drum notes at this position
		}
		if((sp->legacy_track[tracknum]->note[notenum]->note & 8) && (bitmask & 8))
		{	//If the note was changed to include a blue note
			eof_set_flags_at_legacy_note_pos(sp->legacy_track[tracknum],notenum,EOF_NOTE_FLAG_B_CYMBAL,1);	//Set the blue cymbal flag on all drum notes at this position
		}
		if((sp->legacy_track[tracknum]->note[notenum]->note & 16) && (bitmask & 16))
		{	//If the note was changed to include a green note
			eof_set_flags_at_legacy_note_pos(sp->legacy_track[tracknum],notenum,EOF_NOTE_FLAG_G_CYMBAL,1);	//Set the green cymbal flag on all drum notes at this position
		}
	}
}

unsigned char eof_find_pen_note_mask(void)
{
	unsigned long laneborder;
	int bitmaskshift;	//Used to find the pen note bitmask if the notes are inverted (taking lane 6 into account)
	unsigned char returnvalue = 0;
	unsigned long i,tracknum;

	//Determine which lane the mouse is in
	eof_hover_piece = -1;
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	for(i = 0; i < eof_song->legacy_track[tracknum]->numlanes; i++)
	{	//For each of the usable lanes
		laneborder = eof_window_editor->y + EOF_EDITOR_RENDER_OFFSET + 15 + 10 + eof_screen_layout.note_y[i];	//This represents the y position of the boundary between the current lane and the next
		if((mouse_y < laneborder) && (mouse_y > laneborder - eof_screen_layout.string_space))
		{	//If the mouse's Y position is below the boundary to the next lane but above the boundary to the previous lane
			eof_hover_piece = i;	//Store the lane number
		}
	}
	if(eof_hover_note < 0)
	{
		bitmaskshift = eof_song->legacy_track[tracknum]->numlanes - 5;	//If the 6th lane is in use, the inverted mask will be shifted left by one
		switch(eof_hover_piece)
		{
			case 0:
				if(eof_inverted_notes)
				{
					returnvalue = 16 << bitmaskshift;
				}
				else
				{
					returnvalue = 1;
				}
			break;

			case 1:
				if(eof_inverted_notes)
				{
					returnvalue = 8 << bitmaskshift;
				}
				else
				{
					returnvalue = 2;
				}
			break;

			case 2:
				if(eof_inverted_notes)
				{
					returnvalue = 4 << bitmaskshift;
				}
				else
				{
					returnvalue = 4;
				}
			break;

			case 3:
				if(eof_inverted_notes)
				{
					returnvalue = 2 << bitmaskshift;
				}
				else
				{
					returnvalue = 8;
				}
			break;

			case 4:
				if(eof_inverted_notes)
				{
					returnvalue = 1 << bitmaskshift;
				}
				else
				{
					returnvalue = 16;
				}
			break;

			case 5:
				if(eof_inverted_notes)
				{
					returnvalue = 1;
				}
				else
				{
					returnvalue = 32;
				}
			break;
		}
	}
	return returnvalue;
}

void eof_editor_logic_common(void)
{
	int pos = eof_music_pos / eof_zoom;
	int npos;
	unsigned long i;

	if(!eof_song_loaded)
		return;

	eof_hover_note = -1;
	eof_hover_note_2 = -1;
	eof_hover_lyric = -1;

	eof_mickey_z = eof_mouse_z - mouse_z;
	eof_mouse_z = mouse_z;

	if(eof_music_paused)
	{	//If the chart is paused
		/* mouse is in beat marker area */
		if((mouse_y >= eof_window_editor->y + EOF_EDITOR_RENDER_OFFSET - 4) && (mouse_y < eof_window_editor->y + EOF_EDITOR_RENDER_OFFSET + 18))
		{
			for(i = 0; i < eof_song->beats; i++)
			{
				if(pos < 300)
				{
					npos = (20 + (eof_song->beat[i]->pos / eof_zoom));
				}
				else
				{
					npos = (20 - (pos - 300) + (eof_song->beat[i]->pos / eof_zoom));
				}
				if((mouse_x > npos - 16) && (mouse_x < npos + 16) && eof_blclick_released)
				{
					eof_hover_beat = i;
					break;
				}
			}
			if(mouse_b & 1)
			{
				if(eof_mouse_drug)
				{
					eof_mouse_drug++;
					if(eof_mouse_drug == 11)
					{
						eof_mickeys_x = mouse_x - eof_click_x;
					}
				}
				if((eof_mickeys_x != 0) && !eof_mouse_drug)
				{
					eof_mouse_drug++;
				}

				if(eof_blclick_released)
				{
					if(eof_hover_beat >= 0)
					{
						eof_select_beat(eof_hover_beat);
					}
					eof_blclick_released = 0;
					eof_click_x = mouse_x;
					eof_mouse_drug = 0;
				}

				if((eof_mouse_drug > 10) && !eof_blclick_released && (eof_selected_beat == 0) && (eof_mickeys_x != 0) && (eof_hover_beat == eof_selected_beat) && !((eof_mickeys_x * eof_zoom < 0) && (eof_song->beat[0]->pos == 0)))
				{
					if(!eof_undo_toggle)
					{
						eof_prepare_undo(EOF_UNDO_TYPE_NONE);
						eof_moving_anchor = 1;
						eof_last_midi_offset = eof_song->tags->ogg[eof_selected_ogg].midi_offset;
					}
					int rdiff = eof_mickeys_x * eof_zoom;
					if((int)eof_song->beat[0]->pos + rdiff < 0)
					{
						rdiff = -eof_song->beat[0]->pos;
					}
					else
					{
						if(!KEY_EITHER_CTRL)
						{
							for(i = 0; i < eof_song->beats; i++)
							{
								eof_song->beat[i]->pos += rdiff;
							}
						}
						else
						{
							eof_song->beat[0]->pos += rdiff;
						}
					}
					eof_song->tags->ogg[eof_selected_ogg].midi_offset = eof_song->beat[0]->pos;
				}
				else if((eof_mouse_drug > 10) && !eof_blclick_released && (eof_selected_beat > 0) && (eof_mickeys_x != 0) && ((eof_beat_is_anchor(eof_song, eof_hover_beat) || eof_anchor_all_beats || (eof_moving_anchor && (eof_hover_beat == eof_selected_beat)))))
				{
					if(!eof_undo_toggle)
					{
						eof_prepare_undo(EOF_UNDO_TYPE_NONE);
						eof_moving_anchor = 1;
						eof_last_midi_offset = eof_song->tags->ogg[eof_selected_ogg].midi_offset;
						eof_adjusted_anchor = 1;
						if((eof_note_auto_adjust && !KEY_EITHER_SHIFT) || (!eof_note_auto_adjust && KEY_EITHER_SHIFT))
						{
							eof_menu_edit_cut(eof_selected_beat, 0, 0.0);
						}
					}
					eof_song->beat[eof_selected_beat]->pos += eof_mickeys_x * eof_zoom;
					if((eof_song->beat[eof_selected_beat]->pos <= eof_song->beat[eof_selected_beat - 1]->pos + 100) || ((eof_selected_beat + 1 < eof_song->beats) && (eof_song->beat[eof_selected_beat]->pos >= eof_song->beat[eof_selected_beat + 1]->pos - 10)))
					{
						eof_song->beat[eof_selected_beat]->pos -= eof_mickeys_x * eof_zoom;
						eof_song->beat[eof_selected_beat]->fpos = eof_song->beat[eof_selected_beat]->pos;
					}
					else
					{
						eof_recalculate_beats(eof_song, eof_selected_beat);
					}
					eof_song->beat[eof_selected_beat]->flags |= EOF_BEAT_FLAG_ANCHOR;
				}
			}
			if(!(mouse_b & 1))
			{
				if(!eof_blclick_released)
				{
					eof_blclick_released = 1;
					if(eof_mouse_drug && (eof_song->tags->ogg[eof_selected_ogg].midi_offset != eof_last_midi_offset))
					{
						if((eof_note_auto_adjust && !KEY_EITHER_SHIFT) || (!eof_note_auto_adjust && KEY_EITHER_SHIFT))
						{
							eof_adjust_notes(eof_song->tags->ogg[eof_selected_ogg].midi_offset - eof_last_midi_offset);
						}
						eof_fixup_notes();
					}
				}
				if(eof_adjusted_anchor)
				{
					if((eof_note_auto_adjust && !KEY_EITHER_SHIFT) || (!eof_note_auto_adjust && KEY_EITHER_SHIFT))
					{
						eof_menu_edit_cut_paste(eof_selected_beat, 0, 0.0);
						eof_calculate_beats(eof_song);
					}
				}
				eof_moving_anchor = 0;
				eof_mouse_drug = 0;
				eof_adjusted_anchor = 0;
			}
			if(((mouse_b & 2) || key[KEY_INSERT]) && eof_rclick_released && (eof_hover_beat >= 0))
			{
				eof_select_beat(eof_hover_beat);
				alogg_seek_abs_msecs_ogg(eof_music_track, eof_song->beat[eof_hover_beat]->pos + eof_av_delay);
				eof_music_actual_pos = eof_song->beat[eof_hover_beat]->pos + eof_av_delay;
				eof_music_pos = eof_music_actual_pos;
				eof_reset_lyric_preview_lines();
				eof_rclick_released = 0;
			}
			if(!(mouse_b & 2) && !key[KEY_INSERT])
			{
				eof_rclick_released = 1;
			}
		}
		else
		{
			eof_hover_beat = -1;
			eof_adjusted_anchor = 0;
		}

		/* handle scrollbar click */
		if((mouse_y >= eof_window_editor->y + eof_window_editor->h - 17) && (mouse_y < eof_window_editor->y + eof_window_editor->h))
		{
			if(mouse_b & 1)
			{
				eof_music_actual_pos = ((float)eof_music_length / (float)(eof_screen->w - 8)) * (float)(mouse_x - 4);
				alogg_seek_abs_msecs_ogg(eof_music_track, eof_music_actual_pos);
				eof_music_actual_pos = alogg_get_pos_msecs_ogg(eof_music_track);
				eof_music_pos = eof_music_actual_pos;
				if(eof_music_pos - eof_av_delay < 0)
				{
					eof_menu_song_seek_start();
				}
				eof_mix_seek(eof_music_actual_pos);
			}
		}
	}//If the chart is paused

	/* handle playback controls */
	if((mouse_x >= eof_screen_layout.controls_x) && (mouse_x < eof_screen_layout.controls_x + 139) && (mouse_y >= 22 + 8) && (mouse_y < 22 + 17 + 8))
	{
		if(mouse_b & 1)
		{
			eof_selected_control = (mouse_x - eof_screen_layout.controls_x) / 30;
			eof_blclick_released = 0;
			if(eof_selected_control == 1)
			{
				eof_music_rewind();
			}
			else if(eof_selected_control == 3)
			{
				eof_music_forward();
			}
		}
		if(!(mouse_b & 1))
		{
			if(!eof_blclick_released)
			{
				eof_blclick_released = 1;
				if(eof_selected_control == 0)
				{
					eof_menu_song_seek_start();
				}
				else if(eof_selected_control == 2)
				{
					eof_music_play();
				}
				else if(eof_selected_control == 4)
				{
					eof_menu_song_seek_end();
				}
				eof_selected_control = -1;
			}
		}
	}
	else
	{
		eof_selected_control = -1;
	}
}
