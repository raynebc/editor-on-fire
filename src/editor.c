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
#include "tuning.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

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
long        eof_anchor_diff[EOF_TRACKS_MAX] = {0};

EOF_SNAP_DATA eof_snap;
EOF_SNAP_DATA eof_tail_snap;

float eof_pos_distance(float p1, float p2)
{
//	eof_log("eof_pos_distance() entered");

	if(p1 > p2)
	{
		return p1 - p2;
	}
	else
	{
		return p2 - p1;
	}
}

void eof_select_beat(int beat)
{
	eof_log("eof_select_beat() entered", 1);

	int i;
	int beat_counter = 0;
	char first_measure = 0;
	eof_selected_beat = beat;
	eof_selected_measure = -1;
	eof_beat_in_measure = 0;
	eof_beats_in_measure = 1;

	if(!eof_song || (beat >= eof_song->beats))
		return;

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

void eof_get_snap_ts(EOF_SNAP_DATA * sp, int beat)
{
//	eof_log("eof_get_snap_ts() entered");

	int tsbeat = 0;
	int i;

	if(!sp)
	{
		return;
	}
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
//	eof_log("eof_snap_logic() entered");

	int i;
	int interval = 0;
	char measure_snap = 0;
	int note = 4;

	if(!sp)
	{
		return;
	}
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
					measure_snap = 1;
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
//	eof_log("eof_snap_length_logic() entered");

	if(!sp)
	{
		return;
	}
	if(eof_snap_mode != EOF_SNAP_OFF)
	{
		/* if snapped to the next beat, make sure length is calculated from that beat */
		if(sp->pos >= eof_song->beat[sp->beat + 1]->pos)
		{
			sp->beat = sp->beat + 1;
		}
		if((sp->beat >= eof_song->beats - 2) && (sp->beat >= 2))
		{	//Don't reference a negative index of eof_song->beat[]
			sp->beat_length = eof_song->beat[sp->beat - 1]->pos - eof_song->beat[sp->beat - 2]->pos;
		}
		else if(sp->beat + 1 < eof_song->beats)
		{	//Don't read out of bounds
			sp->beat_length = eof_song->beat[sp->beat + 1]->pos - eof_song->beat[sp->beat]->pos;
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
//	eof_log("eof_read_editor_keys() entered");

	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	unsigned long bitmask = 0;	//Used for simplifying note placement logic
	EOF_NOTE * new_note = NULL;
	EOF_LYRIC * new_lyric = NULL;
	unsigned long numlanes = eof_count_track_lanes(eof_song, eof_selected_track);
	unsigned long note;

	if(!eof_song_loaded)
		return;	//Don't handle these keyboard shortcuts unless a chart is loaded

	eof_read_controller(&eof_guitar);
	eof_read_controller(&eof_drums);

/* keyboard shortcuts that may or may not be used when the chart/catalog is playing */

	/* seek to first note (CTRL+Home) */
	/* select previous (SHIFT+Home) */
	/* seek to beginning (Home) */
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

	/* seek to last note (CTRL+End) */
	/* select rest (SHIFT+End) */
	/* seek to end (End) */
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

	/* rewind (R) */
	if(key[KEY_R] && !KEY_EITHER_CTRL && !KEY_EITHER_SHIFT)
	{
		eof_menu_song_seek_rewind();
		key[KEY_R] = 0;
	}

	/* zoom in (+ on numpad) */
	if(key[KEY_PLUS_PAD])
	{
		eof_menu_edit_zoom_helper_in();
		key[KEY_PLUS_PAD] = 0;
	}

	/* zoom out (- on numpad) */
	if(key[KEY_MINUS_PAD])
	{
		eof_menu_edit_zoom_helper_out();
		key[KEY_MINUS_PAD] = 0;
	}

	/* show/hide catalog (Q) */
	if(key[KEY_Q])
	{
		eof_menu_catalog_show();
		key[KEY_Q] = 0;
	}

	/* previous chord name match (SHIFT+W)  */
	/* previous catalog entry (W) */
	if(key[KEY_W])
	{
		if(KEY_EITHER_SHIFT)
		{	//SHIFT+W will cycle to the previous chord name match
			eof_menu_previous_chord_result();
		}
		else
		{	//Otherwise W will cycle to the previous catalog entry
			eof_menu_catalog_previous();
		}
		key[KEY_W] = 0;
	}

	/* toggle expert+ bass drum (CTRL+E) */
	/* next chord name match (SHIFT+E) */
	/* next catalog entry (E) */
	if(key[KEY_E])
	{
		if(KEY_EITHER_CTRL)
		{	//CTRL+E will toggle Expert+ double bass drum notation
			eof_menu_note_toggle_double_bass();
		}
		else
		{
			if(KEY_EITHER_SHIFT)
			{	//SHIFT+E will cycle to the next chord name match
				eof_menu_next_chord_result();
			}
			else
			{	//Otherwise E will cycle to the next catalog entry
				eof_menu_catalog_next();
			}
		}
		key[KEY_E] = 0;
	}

	/* toggle green cymbal (CTRL+G in the drum track) */
	/* toggle grid snap (G) */
	if(key[KEY_G])
	{
		if(KEY_EITHER_CTRL)
		{	//CTRL+G in the drum track will toggle Pro green cymbal notation
			if(eof_selected_track == EOF_TRACK_DRUM)
			{
				eof_menu_note_toggle_rb3_cymbal_green();
				key[KEY_G] = 0;
			}
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
			key[KEY_G] = 0;
		}
	}

	/* toggle yellow cymbal (CTRL+Y) */
	if(key[KEY_Y] && KEY_EITHER_CTRL)
	{	//CTRL+Y will toggle Pro yellow cymbal notation
		eof_menu_note_toggle_rb3_cymbal_yellow();
		key[KEY_Y] = 0;
	}

	/* toggle blue cymbal (CTRL+B in the drum track) */
	/* toggle bend (CTRL+B in a pro guitar track) */
	if(key[KEY_B] && KEY_EITHER_CTRL)
	{	//CTRL+B will toggle Pro blue cymbal notation
		if(eof_selected_track == EOF_TRACK_DRUM)
		{	//If the drum track is active
			eof_menu_note_toggle_rb3_cymbal_blue();
		}
		else if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{
			eof_menu_note_toggle_bend();
		}
		key[KEY_B] = 0;
	}

	/* cycle track backward (CTRL+SHIFT+Tab) */
	/* cycle track forward (CTRL+Tab) */
	/* cycle difficulty backward (SHIFT+Tab) */
	/* cycle difficulty forward (Tab) */
	if(key[KEY_TAB])
	{
		if(KEY_EITHER_CTRL)
		{	//Track numbering begins at 1 instead of 0
			if(KEY_EITHER_SHIFT)	//Shift instrument back 1 number
			{
				if(eof_selected_track > EOF_TRACKS_MIN)
					eof_menu_track_selected_track_number(eof_selected_track-1);
				else
					eof_menu_track_selected_track_number(EOF_TRACKS_MAX - 1);	//Wrap around
			}
			else					//Shift instrument forward 1 number
			{
				if(eof_selected_track < EOF_TRACKS_MAX - 1)
					eof_menu_track_selected_track_number(eof_selected_track+1);
				else
					eof_menu_track_selected_track_number(EOF_TRACKS_MIN);	//Wrap around
			}
		}
		else
		{
			int eof_note_type_max = EOF_NOTE_AMAZING;	//By default, assume this track has 4 usable difficulties
			if(eof_selected_track == EOF_TRACK_DANCE)
			{
				eof_note_type_max = EOF_NOTE_CHALLENGE;	//However, the dance track has 5 usable difficulties
			}

			if(KEY_EITHER_SHIFT)
			{
				eof_note_type--;
				if(eof_note_type < 0)
				{
					eof_note_type = eof_note_type_max;
				}
			}
			else
			{
				eof_note_type++;
				if(eof_note_type > eof_note_type_max)
				{
					eof_note_type = 0;
				}
			}
			eof_detect_difficulties(eof_song);
		}
		eof_mix_find_claps();
		eof_mix_start_helper();
		key[KEY_TAB] = 0;
	}

	/* play/pause music (Space) */
	if(key[KEY_SPACE])
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
					eof_track_fixup_notes(eof_song, eof_selected_track, 1);
					if(eof_undo_last_type == EOF_UNDO_TYPE_RECORD)
					{
						eof_undo_last_type = 0;
					}
				}
			}
		}
		key[KEY_SPACE] = 0;
	}

	/* rewind (Left) */
	if(key[KEY_LEFT])
	{
		eof_music_rewind();
		if(KEY_EITHER_SHIFT && KEY_EITHER_CTRL)
		{	//If user is trying to seek at the slowest speed,
			key[KEY_LEFT] = 0;	//Clear this key state to allow seeking in accurate 1ms intervals
		}
	}

	/* fast forward (Right) */
	if(key[KEY_RIGHT])
	{
		eof_music_forward();
		if(KEY_EITHER_SHIFT && KEY_EITHER_CTRL)
		{	//If user is trying to seek at the slowest speed,
			key[KEY_RIGHT] = 0;	//Clear this key state to allow seeking in accurate 1ms intervals
		}
	}

	/* seek back one grid snap (CTRL+SHIFT+Pg Up) */
	/* seek back one screen (CTRL+Pg Up) */
	/* seek back one note (SHIFT+Pg Up) */
	/* seek back one beat (Pg Up) */
	if(key[KEY_PGUP])
	{
		if(!eof_music_catalog_playback)
		{
			if(KEY_EITHER_CTRL)
			{
				if(KEY_EITHER_SHIFT)
				{	//If both SHIFT and CTRL are being held
					eof_menu_song_seek_previous_grid_snap();
				}
				else
				{
					eof_menu_song_seek_previous_screen();
				}
			}
			else if(KEY_EITHER_SHIFT)
			{
				eof_menu_song_seek_previous_note();
			}
			else
			{
				long b = eof_get_beat(eof_song, eof_music_pos - eof_av_delay);

				if(b > 0)
				{
					if(eof_song->beat[b]->pos == eof_music_pos - eof_av_delay)
					{
						eof_set_seek_position(eof_song->beat[b - 1]->pos + eof_av_delay);
					}
					else
					{
						eof_set_seek_position(eof_song->beat[b]->pos + eof_av_delay);
					}
				}
				else
				{
					eof_set_seek_position(eof_song->beat[0]->pos + eof_av_delay);
				}
			}
		}
		key[KEY_PGUP] = 0;
	}

	/* seek forward one grid snap (CTRL+SHIFT+Pg Dn) */
	/* seek forward one screen (CTRL+Pg Dn) */
	/* seek forward one note (SHIFT+Pg Dn) */
	/* seek forward one beat (Pg Dn) */
	if(key[KEY_PGDN])
	{
		if(!eof_music_catalog_playback)
		{
			if(KEY_EITHER_CTRL)
			{
				if(KEY_EITHER_SHIFT)
				{	//If both SHIFT and CTRL are being held
					eof_menu_song_seek_next_grid_snap();
				}
				else
				{
					eof_menu_song_seek_next_screen();
				}
			}
			else if(KEY_EITHER_SHIFT)
			{
				eof_menu_song_seek_next_note();
			}
			else
			{
				long b = eof_get_beat(eof_song, eof_music_pos - eof_av_delay);
				if(eof_music_pos - eof_av_delay < 0)
				{
					b = -1;
				}

				if(((b < 0) || (b < eof_song->beats - 1)) && (eof_song->beat[b + 1]->pos < eof_music_actual_length))
				{
					eof_set_seek_position(eof_song->beat[b + 1]->pos + eof_av_delay);
				}
			}
		}
		key[KEY_PGDN] = 0;
	}

	/* transpose mini piano visible area up one octave (CTRL+SHIFT+Up) */
	/* transpose mini piano visible area up one (SHIFT+Up in a vocal track) */
	/* transpose lyric up one octave (CTRL+Up in a vocal track) */
	/* toggle pro guitar slide up (CTRL+Up in a pro guitar track) */
	/* transpose note up (Up) */
	/* toggle pro guitar strum up (SHIFT+Up in a pro guitar track) */
	if(key[KEY_UP])
	{
		if(KEY_EITHER_SHIFT)
		{
			if(eof_vocals_selected)
			{
				if(KEY_EITHER_CTRL)
				{	//CTRL and SHIFT held
					eof_vocals_offset += 12;
				}
				else
				{	//Only SHIFT held
					eof_vocals_offset++;
				}
				if(eof_vocals_offset > MAXPITCH)
				{
					eof_vocals_offset = MAXPITCH;
				}
			}
			else if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
			{
				eof_pro_guitar_toggle_strum_up();
			}
		}
		else if(eof_music_paused && !eof_music_catalog_playback)
		{	//Shift is not held and chart is paused
			if(KEY_EITHER_CTRL)
			{
				if(eof_vocals_selected)
				{	/* tranpose up octave */
					eof_menu_note_transpose_up_octave();	//Move up 12 pitches at once, performing a single undo beforehand
				}
				else if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
				{	/* toggle slide up */
					eof_menu_note_toggle_slide_up();
				}
			}
			else
			{
				eof_menu_note_transpose_up();
			}
			if(eof_vocals_selected && eof_mix_vocal_tones_enabled && (eof_selection.current < eof_song->vocal_track[tracknum]->lyrics) && (eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->note != EOF_LYRIC_PERCUSSION))
			{
				eof_mix_play_note(eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->note);
			}
		}
		key[KEY_UP] = 0;
	}

	/* transpose mini piano visible area down one octave (CTRL+SHIFT+Down) */
	/* transpose mini piano visible area down one (SHIFT+Down) */
	/* transpose lyric down one octave (CTRL+Down in a vocal track) */
	/* toggle pro guitar slide down (CTRL+Down in a pro guitar track) */
	/* transpose note down (Down) */
	/* toggle pro guitar strum down (SHIFT+Down in a pro guitar track) */
	if(key[KEY_DOWN])
	{
		if(KEY_EITHER_SHIFT)
		{
			if(eof_vocals_selected)
			{
				if(KEY_EITHER_CTRL)
				{	//CTRL and SHIFT held
					eof_vocals_offset -= 12;
				}
				else
				{	//Only SHIFT held
					eof_vocals_offset--;
				}
				if(eof_vocals_offset < MINPITCH)
				{
					eof_vocals_offset = MINPITCH;
				}
			}
			else if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
			{
				eof_pro_guitar_toggle_strum_down();
			}
		}
		else if(eof_music_paused && !eof_music_catalog_playback)
		{	//Shift is not held and chart is paused
			if(KEY_EITHER_CTRL)
			{
				if(eof_vocals_selected)
				{	/* transpose down octave */
					eof_menu_note_transpose_down_octave();	//Move down 12 pitches at once, performing a single undo beforehand
				}
				else if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
				{	/* toggle slide down */
					eof_menu_note_toggle_slide_down();
				}
			}
			else
			{
				eof_menu_note_transpose_down();
			}
			if(eof_vocals_selected && eof_mix_vocal_tones_enabled && (eof_selection.current < eof_song->vocal_track[tracknum]->lyrics) && (eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->note != EOF_LYRIC_PERCUSSION))
			{
				eof_mix_play_note(eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->note);
			}
		}
		key[KEY_DOWN] = 0;
	}

	/* decrease grid snap (,) */
	if(key[KEY_COMMA])
	{
		eof_snap_mode--;
		if(eof_snap_mode < 0)
		{
			eof_snap_mode = EOF_SNAP_FORTY_EIGHTH;
		}
		key[KEY_COMMA] = 0;
	}

	/* increase grid snap (.) */
	if(key[KEY_STOP])
	{
		eof_snap_mode++;
		if(eof_snap_mode > EOF_SNAP_FORTY_EIGHTH)
		{
			eof_snap_mode = 0;
		}
		key[KEY_STOP] = 0;
	}

	/* toggle palm muting (CTRL+M in a pro guitar track) */
	/* mark/remark lyric phrase (CTRL+M in a vocal track) */
	/* toggle strum mid (SHIFT+M in a pro guitar track) */
	/* toggle metronome (M) */
	if(key[KEY_M])
	{
		if(KEY_EITHER_CTRL)
		{
			if(eof_vocals_selected)
			{
				eof_menu_lyric_line_mark();
			}
			else
			{
				eof_menu_note_toggle_palm_muting();
			}
		}
		else
		{
			if(KEY_EITHER_SHIFT)
			{	//SHIFT+M toggles mid strum direction
				eof_pro_guitar_toggle_strum_mid();
			}
			else
			{	//M without SHIFT or CTRL toggles metronome
				eof_menu_edit_metronome();
			}
		}
		key[KEY_M] = 0;
	}

	/* toggle claps (C) */
	if(key[KEY_C] && !KEY_EITHER_CTRL)
	{
		eof_menu_edit_claps();
		key[KEY_C] = 0;
	}

	/* toggle vocal tones (V) */
	if(key[KEY_V] && !KEY_EITHER_CTRL)
	{
		eof_menu_edit_vocal_tones();
		key[KEY_V] = 0;
	}

	/* Hold and Classic input method logic */
	if(!KEY_EITHER_SHIFT && !eof_vocals_selected && !KEY_EITHER_CTRL)
	{	//If neither SHIFT nor CTRL are held and a non vocal track is active
		if(eof_input_mode == EOF_INPUT_HOLD)
		{	//If the input method is hold
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
			if(key[KEY_5] && (numlanes >= 5))
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
		}//If the input method is hold
		else if(eof_input_mode == EOF_INPUT_CLASSIC)
		{	//If the input method is classic
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
			if(key[KEY_5] && (numlanes >= 5))
			{
				eof_pen_note.note ^= 16;
				key[KEY_5] = 0;
			}
			if(key[KEY_6] && (numlanes >= 6))
			{	//Only allow use of the 6 key if lane 6 is available
				eof_pen_note.note ^= 32;
				key[KEY_6] = 0;
			}
		}//If the input method is classic
	}//If neither SHIFT nor CTRL are held and a non vocal track is active

	if((eof_input_mode == EOF_INPUT_CLASSIC) || (eof_input_mode == EOF_INPUT_HOLD))
	{	//If the input method is classic or hold
		if(key[KEY_ENTER] && (eof_music_pos - eof_av_delay >= eof_song->beat[0]->pos))
		{	//If the user pressed enter and the current seek position is not left of the first beat marker
			/* place note with default length if song is paused */
			if(eof_music_paused)
			{
				new_note = eof_track_add_create_note(eof_song, eof_selected_track, eof_pen_note.note, eof_music_pos - eof_av_delay, eof_snap.length, eof_note_type, NULL);
				if(new_note)
				{
					if(eof_mark_drums_as_cymbal)
					{	//If the user opted to make all new drum notes cymbals automatically
						eof_mark_new_note_as_cymbal(eof_song,eof_selected_track,eof_get_track_size(eof_song, eof_selected_track) - 1);
					}
					if(eof_mark_drums_as_double_bass)
					{	//If the user opted to make all new expert bass drum notes as double bass automatically
						eof_mark_new_note_as_double_bass(eof_song,eof_selected_track,eof_get_track_size(eof_song, eof_selected_track) - 1);
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
							eof_mark_new_note_as_cymbal(eof_song,eof_selected_track,eof_get_track_size(eof_song, eof_selected_track) - 1);
						}
						if(eof_mark_drums_as_double_bass)
						{	//If the user opted to make all new expert bass drum notes as double bass automatically
							eof_mark_new_note_as_double_bass(eof_song,eof_selected_track,eof_get_track_size(eof_song, eof_selected_track) - 1);
						}
						eof_entering_note_note = new_note;
						eof_entering_note = 1;
						eof_detect_difficulties(eof_song);
					}
				}
				else
				{
					eof_entering_note_note->length = (eof_music_pos - eof_av_delay) - eof_entering_note_note->pos - 10;
				}
			}
		}
	}//If the input method is classic or hold

/* keyboard shortcuts that only apply when the chart is playing (non drum record type input methods) */

	if(!eof_music_paused && (eof_selected_track != EOF_TRACK_DRUM))
	{	//If the chart is playing and PART DRUMS is not active
		if(eof_input_mode == EOF_INPUT_GUITAR_TAP)
		{	//If the input method is guitar tap
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
					new_note = eof_track_add_create_note(eof_song, eof_selected_track, eof_vocals_offset, eof_music_pos - eof_av_delay - eof_guitar.delay, 1, 0, "");
				}
				else
				{
					new_note = eof_track_add_create_note(eof_song, eof_selected_track, bitmask, eof_music_pos - eof_av_delay - eof_guitar.delay, 1, eof_note_type, NULL);
					if(new_note)
					{
						if(eof_mark_drums_as_cymbal)
						{	//If the user opted to make all new drum notes cymbals automatically
							eof_mark_new_note_as_cymbal(eof_song,eof_selected_track,eof_get_track_size(eof_song, eof_selected_track) - 1);
						}
						if(eof_mark_drums_as_double_bass)
						{	//If the user opted to make all new expert bass drum notes as double bass automatically
							eof_mark_new_note_as_double_bass(eof_song,eof_selected_track,eof_get_track_size(eof_song, eof_selected_track) - 1);
						}
						eof_entering_note_note = new_note;
						eof_entering_note = 1;
					}
				}
				if(new_note)
				{
					eof_detect_difficulties(eof_song);
					eof_track_sort_notes(eof_song, eof_selected_track);
				}
			}
		}//If the input method is guitar tap
		else if(eof_input_mode == EOF_INPUT_GUITAR_STRUM)
		{	//If the input method is guitar strum
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
						eof_track_sort_notes(eof_song, eof_selected_track);
					}
				}
				if(eof_entering_note)
				{
					if(eof_snote != eof_last_snote)
					{
						eof_entering_note = 0;
						eof_entering_note_lyric->length = (eof_music_pos - eof_av_delay - eof_guitar.delay) - eof_entering_note_lyric->pos - 10;
					}
					else
					{
						eof_entering_note_lyric->length = (eof_music_pos - eof_av_delay - eof_guitar.delay) - eof_entering_note_lyric->pos - 10;
					}
					eof_track_fixup_notes(eof_song, eof_selected_track, 1);
				}
			}//if(eof_vocals_selected)
			else
			{	//If a non vocal track is selected
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
								eof_mark_new_note_as_cymbal(eof_song,eof_selected_track,eof_get_track_size(eof_song, eof_selected_track) - 1);
							}
							if(eof_mark_drums_as_double_bass)
							{	//If the user opted to make all new expert bass drum notes as double bass automatically
								eof_mark_new_note_as_double_bass(eof_song,eof_selected_track,eof_get_track_size(eof_song, eof_selected_track) - 1);
							}
							eof_entering_note_note = new_note;
							eof_entering_note = 1;
							eof_detect_difficulties(eof_song);
							eof_track_sort_notes(eof_song, eof_selected_track);
						}
					}
				}//If the user strummed
				if(eof_entering_note)
				{
					if(eof_snote != eof_last_snote)
					{
						eof_entering_note = 0;
						eof_entering_note_note->length = (eof_music_pos - eof_av_delay - eof_guitar.delay) - eof_entering_note_note->pos - 10;
					}
					else
					{
						eof_entering_note_note->length = (eof_music_pos - eof_av_delay - eof_guitar.delay) - eof_entering_note_note->pos - 10;
					}
					eof_track_fixup_notes(eof_song, eof_selected_track, 1);
				}
			}//If a non vocal track is selected
		}//If the input method is guitar strum
	}//If the chart is playing and PART DRUMS is not active

/* keyboard shortcuts that apply when the chart is paused and nothing is playing */

	if(eof_music_paused && !eof_music_catalog_playback)
	{	//If the chart is paused and no catalog entries are playing
	/* lower playback speed (;) */
		if(key[KEY_SEMICOLON])
		{
			eof_menu_edit_playback_speed_helper_slower();
			key[KEY_SEMICOLON] = 0;
		}

	/* increase playback speed (') */
		if(key[KEY_QUOTE])
		{
			eof_menu_edit_playback_speed_helper_faster();
			key[KEY_QUOTE] = 0;
		}

	/* anchor beat (SHIFT+A) */
	/* toggle anchor (A) */
	/* select all (CTRL+A) */
		if(key[KEY_A])
		{
			if(!KEY_EITHER_CTRL)
			{
				if(KEY_EITHER_SHIFT)
				{
					eof_menu_beat_anchor();
				}
				else
				{
					eof_menu_beat_toggle_anchor();
				}
			}
			else
			{
				eof_menu_edit_select_all();
			}
			key[KEY_A] = 0;
		}

	/* decrease note length ( [ or CTRL+[ ) */
		if(key[KEY_OPENBRACE])
		{
			unsigned long reductionvalue = 100;	//Default decrease length when grid snap is disabled
			if(eof_snap_mode == EOF_SNAP_OFF)
			{
				if(KEY_EITHER_CTRL)
				{
					reductionvalue = 10;
				}
			}
			else
			{
				reductionvalue = 0;	//Will indicate to eof_adjust_note_length() to use the grid snap value
			}
			eof_adjust_note_length(eof_song, eof_selected_track, reductionvalue, -1);	//Decrease selected notes by the appropriate length
			key[KEY_OPENBRACE] = 0;
		}

	/* increase note length ( ] or CTRL+] ) */
		if(key[KEY_CLOSEBRACE])
		{
			unsigned long increasevalue = 100;	//Default increase length when grid snap is disabled
			if(eof_snap_mode == EOF_SNAP_OFF)
			{
				if(KEY_EITHER_CTRL)
				{
					increasevalue = 10;
				}
			}
			else
			{
				increasevalue = 0;	//Will indicate to eof_adjust_note_length() to use the grid snap value
			}
			eof_adjust_note_length(eof_song, eof_selected_track, increasevalue, 1);	//Increase selected notes by the appropriate length
			key[KEY_CLOSEBRACE] = 0;
		}

	/* toggle tapping status (CTRL+T in a pro guitar track) */
	/* toggle crazy status (T) */
		if(key[KEY_T])
		{
			if(KEY_EITHER_CTRL)
			{
				if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
				{	/* toggle tapping */
					eof_menu_note_toggle_tapping();
				}
			}
			else
			{	/* toggle crazy */
				eof_menu_note_toggle_crazy();
			}
			key[KEY_T] = 0;
		}

	/* select like (CTRL+L) */
	/* split lyric (SHIFT+L in PART VOCALS) */
	/* edit lyric (L in PART VOCALS */
	/* enable legacy view (SHIFT+L in pro guitar track) */
		if(key[KEY_L])
		{
			if(KEY_EITHER_CTRL)
			{	/* select like */
				eof_menu_edit_select_like();
			}
			else if(eof_vocals_selected && (eof_selection.track == EOF_TRACK_VOCALS) && (eof_selection.current < eof_song->vocal_track[tracknum]->lyrics))
			{	//If PART VOCALS is active, and one of its lyrics is the current selected lyric
				if(KEY_EITHER_SHIFT)
				{	//Split lyric
					eof_menu_split_lyric();
				}
				else
				{	//Edit lyric
					eof_edit_lyric_dialog();
				}
			}
			else if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
			{
				if(KEY_EITHER_SHIFT)
				{
					eof_menu_song_legacy_view();
				}
			}
			key[KEY_L] = 0;
		}

	/* toggle freestyle (F in a vocal track) */
		if(key[KEY_F])
		{
			if(eof_vocals_selected)
			{	//Toggle freestyle
				eof_menu_toggle_freestyle();
			}
			key[KEY_F] = 0;
		}

	/* deselect all (CTRL+D) */
		if(KEY_EITHER_CTRL && key[KEY_D])
		{
			eof_menu_edit_deselect_all();
			key[KEY_D] = 0;
		}

	/* cycle HOPO status (H in a legacy track) */
	/* toggle hammer on status (H in a pro guitar track) */
		if(key[KEY_H] && !KEY_EITHER_ALT)
		{
			if(eof_song->track[eof_selected_track]->track_format == EOF_LEGACY_TRACK_FORMAT)
			{
				eof_menu_hopo_cycle();
			}
			else if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
			{
				eof_menu_pro_guitar_toggle_hammer_on();
			}
			key[KEY_H] = 0;
		}

	/* toggle pull off status (P in a pro guitar track) */
		if(key[KEY_P] && !KEY_EITHER_CTRL && !KEY_EITHER_ALT && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
		{
			eof_menu_pro_guitar_toggle_pull_off();
			key[KEY_P] = 0;
		}

	/* toggle open hi hat (SHIFT+O) */
		if(key[KEY_O] && !KEY_EITHER_CTRL && KEY_EITHER_SHIFT)
		{
			eof_menu_note_toggle_hi_hat_open();
			key[KEY_O] = 0;
		}

	/* toggle pedal controlled hi hat (SHIFT+P) */
		if(key[KEY_P] && !KEY_EITHER_CTRL && KEY_EITHER_SHIFT)
		{
			eof_menu_note_toggle_hi_hat_pedal();
			key[KEY_P] = 0;
		}

	/* toggle sizzle hi hat (SHIFT+S) */
		if(key[KEY_S] && !KEY_EITHER_CTRL && KEY_EITHER_SHIFT)
		{
			eof_menu_note_toggle_hi_hat_sizzle();
			key[KEY_S] = 0;
		}

	/* toggle rim shot (SHIFT+R) */
		if(key[KEY_R] && !KEY_EITHER_CTRL && KEY_EITHER_SHIFT)
		{
			eof_menu_note_toggle_rimshot();
			key[KEY_R] = 0;
		}

	/* mark/remark slider (SHIFT+S) */
		if(key[KEY_S] && !KEY_EITHER_CTRL && KEY_EITHER_SHIFT)
		{
			eof_menu_slider_mark();
			key[KEY_S] = 0;
		}

		if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If the active track is a pro guitar track
	/* edit pro guitar note (N in a pro guitar track) */
			if((key[KEY_N] && !KEY_EITHER_CTRL))
			{
				eof_menu_note_edit_pro_guitar_note();
				key[KEY_N] = 0;
			}

	/* set pro guitar fret values (CTRL+#, CTRL+Fn #, CTRL+X, CTRL+~, CTRL++, CTRL+-) */
	/* toggle pro guitar ghost status (CTRL+G) */
			if(KEY_EITHER_CTRL && !KEY_EITHER_SHIFT)
			{	//If CTRL is held but SHIFT is not
				//CTRL+# or CTRL+Fn # in a pro guitar track sets the fret values of selected notes
				//CTRL+~ sets fret values to 0 and CTRL+X sets fret values to (muted)
				if(key[KEY_TILDE])
				{
					eof_set_pro_guitar_fret_number(0,0);
					key[KEY_TILDE] = 0;
				}
				if(key[KEY_1])
				{
					eof_set_pro_guitar_fret_number(0,1);
					key[KEY_1] = 0;
				}
				else if(key[KEY_2])
				{
					eof_set_pro_guitar_fret_number(0,2);
					key[KEY_2] = 0;
				}
				else if(key[KEY_3])
				{
					eof_set_pro_guitar_fret_number(0,3);
					key[KEY_3] = 0;
				}
				else if(key[KEY_4])
				{
					eof_set_pro_guitar_fret_number(0,4);
					key[KEY_4] = 0;
				}
				else if(key[KEY_5])
				{
					eof_set_pro_guitar_fret_number(0,5);
					key[KEY_5] = 0;
				}
				else if(key[KEY_6])
				{
					eof_set_pro_guitar_fret_number(0,6);
					key[KEY_6] = 0;
				}
				else if(key[KEY_7])
				{
					eof_set_pro_guitar_fret_number(0,7);
					key[KEY_7] = 0;
				}
				else if(key[KEY_8])
				{
					eof_set_pro_guitar_fret_number(0,8);
					key[KEY_8] = 0;
				}
				else if(key[KEY_9])
				{
					eof_set_pro_guitar_fret_number(0,9);
					key[KEY_9] = 0;
				}
				else if(key[KEY_0])
				{
					eof_set_pro_guitar_fret_number(0,10);
					key[KEY_0] = 0;
				}
				else if(key[KEY_F1])
				{
					eof_set_pro_guitar_fret_number(0,11);
					key[KEY_F1] = 0;
				}
				else if(key[KEY_F2])
				{
					eof_set_pro_guitar_fret_number(0,12);
					key[KEY_F2] = 0;
				}
				else if(key[KEY_F3])
				{
					eof_set_pro_guitar_fret_number(0,13);
					key[KEY_F3] = 0;
				}
				else if(key[KEY_F4])
				{
					eof_set_pro_guitar_fret_number(0,14);
					key[KEY_F4] = 0;
				}
				else if(key[KEY_F5])
				{
					eof_set_pro_guitar_fret_number(0,15);
					key[KEY_F5] = 0;
				}
				else if(key[KEY_F6])
				{
					eof_set_pro_guitar_fret_number(0,16);
					key[KEY_F6] = 0;
				}
				else if(key[KEY_F7])
				{
					eof_set_pro_guitar_fret_number(0,17);
					key[KEY_F7] = 0;
				}
				else if(key[KEY_F8])
				{
					eof_set_pro_guitar_fret_number(0,18);
					key[KEY_F8] = 0;
				}
				else if(key[KEY_F9])
				{
					eof_set_pro_guitar_fret_number(0,19);
					key[KEY_F9] = 0;
				}
				else if(key[KEY_F10])
				{
					eof_set_pro_guitar_fret_number(0,20);
					key[KEY_F10] = 0;
				}
				else if(key[KEY_F11])
				{
					eof_set_pro_guitar_fret_number(0,21);
					key[KEY_F11] = 0;
				}
				else if(key[KEY_F12])
				{
					eof_set_pro_guitar_fret_number(0,22);
					key[KEY_F12] = 0;
				}
				else if(key[KEY_X])
				{
					eof_set_pro_guitar_fret_number(0,255);	//CTRL+X sets frets to (muted)
					key[KEY_X] = 0;
				}
				else if(key[KEY_MINUS])
				{
					eof_set_pro_guitar_fret_number(2,0);	//Decrement fret value
					key[KEY_MINUS] = 0;
				}
				else if(key[KEY_EQUALS])
				{
					eof_set_pro_guitar_fret_number(1,0);	//Increment fret value
					key[KEY_EQUALS] = 0;
				}
				else if(key[KEY_G])
				{
					eof_menu_note_toggle_ghost();
					key[KEY_G] = 0;
				}
			}//If CTRL is held but SHIFT is not
			else if(KEY_EITHER_SHIFT && !KEY_EITHER_CTRL)
			{	//If SHIFT is held, but CTRL is not
	/* set fret value shortcut bitmask (SHIFT+Esc, SHIFT+Fn #) */
				if(key[KEY_ESC])
				{
					eof_pro_guitar_fret_bitmask = 0;	//Disable all strings
					key[KEY_ESC] = 0;
				}
				else if(key[KEY_F1])
				{
					eof_pro_guitar_fret_bitmask ^= 32;	//Toggle string 1 (high E)
					key[KEY_F1] = 0;
				}
				else if(key[KEY_F2])
				{
					eof_pro_guitar_fret_bitmask ^= 16;	//Toggle string 2
					key[KEY_F2] = 0;
				}
				else if(key[KEY_F3])
				{
					eof_pro_guitar_fret_bitmask ^= 8;	//Toggle string 3
					key[KEY_F3] = 0;
				}
				else if(key[KEY_F4])
				{
					eof_pro_guitar_fret_bitmask ^= 4;	//Toggle string 4
					key[KEY_F4] = 0;
				}
				else if(key[KEY_F5])
				{
					eof_pro_guitar_fret_bitmask ^= 2;	//Toggle string 5
					key[KEY_F5] = 0;
				}
				else if(key[KEY_F6])
				{
					eof_pro_guitar_fret_bitmask ^= 1;	//Toggle string 6 (low e)
					key[KEY_F6] = 0;
				}
				else if(key[KEY_F7])
				{
					eof_pro_guitar_fret_bitmask = 63;	//Enable all strings
					key[KEY_F7] = 0;
				}
				else if(key[KEY_F8])
				{
					eof_pro_guitar_fret_bitmask = 63;	//Enable all strings
					key[KEY_F8] = 0;
				}
				else if(key[KEY_F9])
				{
					eof_pro_guitar_fret_bitmask = 63;	//Enable all strings
					key[KEY_F9] = 0;
				}
				else if(key[KEY_F10])
				{
					eof_pro_guitar_fret_bitmask = 63;	//Enable all strings
					key[KEY_F10] = 0;
				}
				else if(key[KEY_F11])
				{
					eof_pro_guitar_fret_bitmask = 63;	//Enable all strings
					key[KEY_F11] = 0;
				}
				else if(key[KEY_F12])
				{
					eof_pro_guitar_fret_bitmask = 63;	//Enable all strings
					key[KEY_F12] = 0;
				}
			}//If SHIFT is held, but CTRL is not
		}//If the active track is a pro guitar track

	/* set mini piano octave focus (SHIFT+# in a vocal track) */
	/* toggle lane (SHIFT+# in a non vocal track) */
		if(KEY_EITHER_SHIFT && !KEY_EITHER_CTRL)
		{	//If SHIFT is held down and CTRL is not
			if(eof_vocals_selected)
			{	//SHIFT+# in a vocal track sets the octave view
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
			else
			{	//In other tracks, SHIFT+# toggles lanes on/off
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
				else if(key[KEY_5] && (numlanes >= 5))
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
		}//If SHIFT is held down and CTRL is not

		if((!KEY_EITHER_CTRL) && (eof_input_mode == EOF_INPUT_REX))
		{	//If CTRL is not held down and the input method is rex mundi
			eof_hover_piece = -1;
			if((mouse_y >= eof_window_editor->y + 25 + EOF_EDITOR_RENDER_OFFSET) && (mouse_y < eof_window_editor->y + eof_screen_layout.fretboard_h + EOF_EDITOR_RENDER_OFFSET))
			{	//If the mouse is in the fretboard area
				if(KEY_EITHER_SHIFT)
				{
					eof_snap.length = 1;
				}
				if(!eof_vocals_selected)
				{	//If a vocal track is not active
					bitmask = 0;
					if(key[KEY_1] && !KEY_EITHER_SHIFT)
					{
						bitmask = 1;
						key[KEY_1] = 0;
					}
					else if(key[KEY_2] && !KEY_EITHER_SHIFT)
					{
						bitmask = 2;
						key[KEY_2] = 0;
					}
					else if(key[KEY_3] && !KEY_EITHER_SHIFT)
					{
						bitmask = 4;
						key[KEY_3] = 0;
					}
					else if(key[KEY_4] && !KEY_EITHER_SHIFT)
					{
						bitmask = 8;
						key[KEY_4] = 0;
					}
					else if(key[KEY_5] && (numlanes >= 5) && !KEY_EITHER_SHIFT)
					{
						bitmask = 16;
						key[KEY_5] = 0;
					}
					else if(key[KEY_6] && (numlanes >= 6) && !KEY_EITHER_SHIFT)
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
							if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
							{	//If legacy view is in effect, alter the note's legacy bitmask
								eof_song->pro_guitar_track[tracknum]->note[eof_hover_note]->legacymask ^= bitmask;
							}
							else
							{	//Otherwise alter the note's normal bitmask
								note = eof_get_note_note(eof_song, eof_selected_track, eof_hover_note);
								note ^= bitmask;
								eof_set_note_note(eof_song, eof_selected_track, eof_hover_note, note);
							}
							if(eof_mark_drums_as_cymbal)
							{	//If the user opted to make all new drum notes cymbals automatically
								eof_mark_edited_note_as_cymbal(eof_song,eof_selected_track,eof_hover_note,bitmask);
							}
							if(eof_mark_drums_as_double_bass)
							{	//If the user opted to make all new expert bass drum notes as double bass automatically
								eof_mark_new_note_as_double_bass(eof_song,eof_selected_track,eof_hover_note);
							}
							eof_selection.current = eof_hover_note;
							if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
							{	//If legacy view is in effect, check the note's legacy bitmask
								note = eof_song->pro_guitar_track[tracknum]->note[eof_hover_note]->legacymask;
							}
							else
							{	//Otherwise check the note's normal bitmask and delete the note if necessary
								note = eof_get_note_note(eof_song, eof_selected_track, eof_hover_note);
								if(note == 0)
								{	//If the note just had all lanes cleared, delete the note
									eof_track_delete_note(eof_song, eof_selected_track, eof_hover_note);
									eof_selection.multi[eof_selection.current] = 0;
									eof_selection.current = EOF_MAX_NOTES - 1;
									eof_track_sort_notes(eof_song, eof_selected_track);
									eof_track_fixup_notes(eof_song, eof_selected_track, 1);
									eof_determine_phrase_status();
									eof_detect_difficulties(eof_song);
								}
							}

							if(eof_selection.current != EOF_MAX_NOTES - 1)
							{	//If a new gem was placed (toggle gem didn't delete the affected note)
								if(!eof_add_new_notes_to_selection)
								{	//If the user didn't opt to prevent clearing the note selection when adding gems
									memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
								}
								eof_selection.track = eof_selected_track;
								eof_selection.multi[eof_selection.current] = 1;
								eof_selection.current_pos = eof_get_note_pos(eof_song, eof_selected_track, eof_selection.current);
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
									eof_mark_new_note_as_cymbal(eof_song,eof_selected_track,eof_get_track_size(eof_song, eof_selected_track) - 1);
								}
								if(eof_mark_drums_as_double_bass)
								{	//If the user opted to make all new expert bass drum notes as double bass automatically
									eof_mark_new_note_as_double_bass(eof_song,eof_selected_track,eof_get_track_size(eof_song, eof_selected_track) - 1);
								}
								eof_selection.current_pos = eof_get_note_pos(eof_song, eof_selected_track, eof_get_track_size(eof_song, eof_selected_track) - 1);	//Get the position of the last note that was added
								eof_selection.range_pos_1 = eof_selection.current_pos;
								eof_selection.range_pos_2 = eof_selection.current_pos;
								eof_selection.track = eof_selected_track;
								if(!eof_add_new_notes_to_selection)
								{	//If the user didn't opt to prevent clearing the note selection when adding gems
									memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
								}
								eof_track_sort_notes(eof_song, eof_selected_track);
								eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Fixup notes and retain note selection
								eof_determine_phrase_status();
								eof_selection.multi[eof_selection.current] = 1;
								eof_detect_difficulties(eof_song);
							}
						}
					}//If user has pressed any key from 1 through 6
				}//If a vocal track is not active
			}//If the mouse is in the fretboard area
		}//If CTRL is not held down and the input method is rex mundi

	/* delete beat (CTRL+Del) */
	/* delete note (Del) */
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

	/* place bookmark (CTRL+Numpad) */
	/* seek to bookmark (Numpad) */
		if(!KEY_EITHER_SHIFT)
		{	//If SHIFT is not held
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
		}//If SHIFT is not held

	/* copy (CTRL+C) */
		if(KEY_EITHER_CTRL && key[KEY_C])
		{
			eof_menu_edit_copy();
			key[KEY_C] = 0;
		}

	/* paste (CTRL+V) */
		if(KEY_EITHER_CTRL && key[KEY_V])
		{
			eof_menu_edit_paste();
			key[KEY_V] = 0;
		}

	/* old paste (CTRL+P) */
		if(KEY_EITHER_CTRL && key[KEY_P])
		{	//CTRL+P is "Old Paste"
			eof_menu_edit_old_paste();
			key[KEY_P] = 0;
		}
	}//If the chart is paused and no catalog entries are playing
}

void eof_editor_drum_logic(void)
{
	eof_log("eof_editor_drum_logic() entered", 1);

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
					eof_mark_new_note_as_cymbal(eof_song,eof_selected_track,eof_get_track_size(eof_song, eof_selected_track) - 1);
				}
				if(eof_mark_drums_as_double_bass)
				{	//If the user opted to make all new expert bass drum notes as double bass automatically
					eof_mark_new_note_as_double_bass(eof_song,eof_selected_track,eof_get_track_size(eof_song, eof_selected_track) - 1);
				}
				eof_snap_logic(&eof_snap, eof_get_note_pos(eof_song, eof_selected_track, eof_get_track_size(eof_song, eof_selected_track) - 1));
				eof_set_note_pos(eof_song, eof_selected_track, eof_get_track_size(eof_song, eof_selected_track) - 1, eof_snap.pos);
				eof_entering_note_note = new_note;
				eof_entering_note = 1;
				eof_detect_difficulties(eof_song);
				eof_track_sort_notes(eof_song, eof_selected_track);
			}
		}
	}
}

void eof_editor_logic(void)
{
//	eof_log("eof_editor_logic() entered");

	unsigned long i, note, notepos;
	unsigned long tracknum;
	unsigned long bitmask = 0;	//Used to reduce duplicated logic
	int use_this_x = mouse_x;
	int next_note_pos = 0;
	EOF_NOTE * new_note = NULL;
	int pos = eof_music_pos / eof_zoom;
	int npos, lpos;
	long notelength;

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
				eof_track_sort_notes(eof_song, eof_selected_track);
				eof_track_fixup_notes(eof_song, eof_selected_track, 1);
				eof_determine_phrase_status();
				eof_notes_moved = 0;
			}
		}

		/* mouse is in the fretboard area */
		if((mouse_y >= eof_window_editor->y + 25 + EOF_EDITOR_RENDER_OFFSET) && (mouse_y < eof_window_editor->y + eof_screen_layout.fretboard_h + EOF_EDITOR_RENDER_OFFSET))
		{
			int x_tolerance = 6 * eof_zoom;	//This is how far left or right of a note the mouse is allowed to be to still be considered to hover over that note
			lpos = pos < 300 ? (mouse_x - 20) * eof_zoom : ((pos - 300) + mouse_x - 20) * eof_zoom;
			eof_snap_logic(&eof_snap, lpos);
			eof_snap_length_logic(&eof_snap);
			eof_pen_note.pos = eof_snap.pos;
			use_this_x = lpos;
			eof_pen_visible = 1;
			for(i = 0; (i < eof_get_track_size(eof_song, eof_selected_track)) && (eof_hover_note < 0); i++)
			{	//For each note in the active track, until a hover note is found
				if(eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type)
				{	//If the note is in the active difficulty
					npos = eof_get_note_pos(eof_song, eof_selected_track, i);
					if((use_this_x > npos - x_tolerance) && (use_this_x < npos + x_tolerance))
					{
						eof_hover_note = i;
					}
					else if((eof_pen_note.pos > npos - x_tolerance) && (eof_pen_note.pos < npos + x_tolerance))
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
					if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
					{	//If legacy view is in effect, set the pen note to the legacy mask
						eof_pen_note.note = eof_song->pro_guitar_track[tracknum]->note[eof_hover_note]->legacymask;
					}
					else
					{	//Otherwise set it to the note's normal bitmask
						eof_pen_note.note = eof_get_note_note(eof_song, eof_selected_track, eof_hover_note);
					}
					eof_pen_note.length = eof_get_note_length(eof_song, eof_selected_track, eof_hover_note);
					if(!eof_mouse_drug)
					{
						eof_pen_note.pos = eof_get_note_pos(eof_song, eof_selected_track, eof_hover_note);
						eof_pen_note.note |= eof_find_pen_note_mask();	//Set the appropriate bits
					}
				}
				else
				{
					if(eof_input_mode == EOF_INPUT_REX)
					{
						eof_pen_note.note = 0;
					}
					/* calculate piece for piano roll mode */
					else if(eof_input_mode == EOF_INPUT_PIANO_ROLL)
					{
						eof_pen_note.note = eof_find_pen_note_mask();	//Find eof_hover_piece and set the appropriate bits in the pen note
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

			/* handle initial left click */
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
						eof_selection.last_pos = eof_get_note_pos(eof_song, eof_selected_track, eof_selection.last);
					}
					eof_selection.current = eof_hover_note;
					eof_selection.current_pos = eof_get_note_pos(eof_song, eof_selected_track, eof_selection.current);
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
					eof_peg_x = eof_get_note_pos(eof_song, eof_selected_track, eof_pegged_note);
					if(!KEY_EITHER_CTRL)
					{
						/* Shift+Click selects range */
						if(KEY_EITHER_SHIFT && !ignore_range)
						{
							if(eof_selection.range_pos_1 < eof_selection.range_pos_2)
							{
								for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
								{
									if((eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type) && (eof_get_note_pos(eof_song, eof_selected_track, i) >= eof_selection.range_pos_1) && (eof_get_note_pos(eof_song, eof_selected_track, i) <= eof_selection.range_pos_2))
									{
										if(eof_selection.track != eof_selected_track)
										{
											eof_selection.track = eof_selected_track;
											memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
										}
										eof_selection.multi[i] = 1;
										eof_undo_last_type = EOF_UNDO_TYPE_NONE;
									}
								}
							}
							else
							{
								for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
								{
									if((eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type) && (eof_get_note_pos(eof_song, eof_selected_track, i) >= eof_selection.range_pos_2) && (eof_get_note_pos(eof_song, eof_selected_track, i) <= eof_selection.range_pos_1))
									{
										if(eof_selection.track != eof_selected_track)
										{
											eof_selection.track = eof_selected_track;
											memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
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
//								printf("notes %d\n", eof_notes_selected());	//Debugging
								memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
							}
							if(eof_selection.multi[eof_selection.current] == 1)
							{
								if(eof_selection.track != eof_selected_track)
								{
									eof_selection.track = eof_selected_track;
									memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
								}
								eof_selection.multi[eof_selection.current] = 2;
								eof_undo_last_type = EOF_UNDO_TYPE_NONE;
							}
							else
							{
								if(eof_selection.track != eof_selected_track)
								{
									eof_selection.track = eof_selected_track;
									memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
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
								memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
							}
							eof_selection.multi[eof_selection.current] = 2;
							eof_undo_last_type = EOF_UNDO_TYPE_NONE;
						}
						else
						{
							if(eof_selection.track != eof_selected_track)
							{
								eof_selection.track = eof_selected_track;
								memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
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
						memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
						eof_selection.current = EOF_MAX_NOTES - 1;
						eof_selection.current_pos = 0;
						eof_selection.range_pos_1 = 0;
						eof_selection.range_pos_2 = 0;
					}
				}
			}// handle initial left click
			if(!(mouse_b & 1))
			{	//If the left mouse button is NOT pressed
				if(!eof_lclick_released)
				{
					eof_lclick_released = 1;	//Set the "just released" status
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
								memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
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
									memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
								}
								eof_selection.multi[eof_selection.current] = 1;
								eof_undo_last_type = EOF_UNDO_TYPE_NONE;
							}
						}
					}
					eof_lclick_released = 2;	//Set the "left click release handled" status
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
			char move_direction = 1;	//Assume right mouse movement by default
			int revert = 0;
			int revert_amount = 0;
			char undo_made = 0;
			if((mouse_b & 1) && !eof_lclick_released && (lpos >= 0))
			{	//If the left mouse button is being held and the mouse is right of the left edge of the piano roll
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
					{	//Move notes by grid snap
						if(eof_pen_note.pos < eof_get_note_pos(eof_song, eof_selected_track, eof_selection.current))
						{	//Left mouse movement
							move_direction = -1;
							move_offset = eof_get_note_pos(eof_song, eof_selected_track, eof_selection.current) - eof_pen_note.pos;
						}
						else
						{	//Right mouse movement
							move_direction = 1;
							move_offset = eof_pen_note.pos - eof_get_note_pos(eof_song, eof_selected_track, eof_selection.current);
						}
					}
					else
					{
						if(eof_mickeys_x < 0)
						{	//Left mouse movement
							move_direction = -1;
							move_offset = -eof_mickeys_x * eof_zoom;
						}
						else
						{	//Right mouse movement
							move_direction = 1;
							move_offset = eof_mickeys_x * eof_zoom;
						}
					}
					for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
					{	//For each note in the active track
						if(eof_selection.multi[i])
						{	//If the note is selected
							notepos = eof_get_note_pos(eof_song, eof_selected_track, i);
							if(notepos == eof_selection.current_pos)
							{
								if(move_direction < 0)
								{	//Left mouse movement
									eof_selection.current_pos -= move_offset;
								}
								else
								{	//Right mouse movement
									eof_selection.current_pos += move_offset;
								}
							}
							if(move_direction < 0)
							{	//If the user is moving notes left
								if((move_offset > notepos) || (notepos - move_offset < eof_song->beat[0]->pos))
								{	//If the move would make the note position negative or otherwise earlier than the first beat
									move_offset = notepos - eof_song->beat[0]->pos;	//Adjust the move offset to line it up with the first beat marker
								}
							}
							if(move_offset == 0)
							{	//If the offset has become 0
								break;	//Don't move the notes
							}
							if(!undo_made)
							{	//Only create the undo state before moving the first note
								eof_notes_moved = 1;
								eof_prepare_undo(EOF_UNDO_TYPE_NONE);
								undo_made = 1;
							}
							eof_move_note_pos(eof_song, eof_selected_track, i, move_offset, move_direction);
							notepos = eof_get_note_pos(eof_song, eof_selected_track, i);	//Get the updated note position
							notelength = eof_get_note_length(eof_song, eof_selected_track, i);
							if(notepos + notelength >= eof_music_length)
							{	//If the moved note is at or after the end of the chart
								revert |= 1;
								revert_amount = notepos + notelength - eof_music_length;	//This positive value will be subtracted from the note via the revert loop
							}
							else if(notepos <= eof_song->beat[0]->pos)
							{	//If the moved note is at or before the first beat marker
								revert |= 2;
								revert_amount = eof_song->beat[0]->pos - notepos;			//This negative value will be subtracted from the note via the revert loop
							}
						}
					}
					if(revert)
					{
						if(revert == 3)
						{	//If something unexpected happened and the drag operation resulted in both edges of the chart having note(s) pushed beyond the edge
							allegro_message("Logic error in eof_editor_logic() note drag handling");
						}
						for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
						{
							if(eof_selection.multi[i])
							{
								eof_set_note_pos(eof_song, eof_selected_track, i, eof_get_note_pos(eof_song, eof_selected_track, i) - revert_amount);
							}
						}
					}
				}
			}//If the left mouse button is being held
			if(((mouse_b & 2) || key[KEY_INSERT]) && eof_rclick_released && eof_pen_note.note && (eof_pen_note.pos < eof_music_length))
			{	//Right mouse click or Insert key pressed, and the pen note is valid
				eof_selection.range_pos_1 = 0;
				eof_selection.range_pos_2 = 0;
				eof_rclick_released = 0;
				if((eof_input_mode == EOF_INPUT_PIANO_ROLL) || (eof_input_mode == EOF_INPUT_CLASSIC) || (eof_input_mode == EOF_INPUT_HOLD))
				{	//All three of these input methods toggle gems when clicking on the right mouse button
					if(!eof_undo_toggle)
					{
						eof_prepare_undo(EOF_UNDO_TYPE_NOTE_SEL);
					}
					if(eof_hover_note >= 0)
					{	//If altering an existing note
						if(eof_input_mode == EOF_INPUT_PIANO_ROLL)
						{	//Piano roll input method must use special logic to determine the lane ordering for the pen note
							bitmask = eof_find_pen_note_mask();	//Set the appropriate bits
						}
						else
						{	//The other two use the pen note bitmask normally
							bitmask = eof_pen_note.note;
						}

						if(bitmask)
						{	//If a valid pen bitmask was obtained
							eof_selection.current = eof_hover_note;
							eof_selection.track = eof_selected_track;
							if(!eof_add_new_notes_to_selection)
							{	//If the user didn't opt to prevent clearing the note selection when adding gems
								memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
							}
							if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
							{	//If legacy view is in effect, alter the note's legacy bitmask
								note = eof_song->pro_guitar_track[tracknum]->note[eof_hover_note]->legacymask;
								note ^= bitmask;
								eof_song->pro_guitar_track[tracknum]->note[eof_hover_note]->legacymask = note;
							}
							else
							{	//Otherwise alter the note's normal bitmask and delete the note if necessary
								note = eof_get_note_note(eof_song, eof_selected_track, eof_hover_note);	//Examine the hover note...
								note ^= bitmask;	//as it would look by toggling the pen note's gem
								if(note == 0)
								{	//If the note just had all lanes cleared, delete the note
									eof_track_delete_note(eof_song, eof_selected_track, eof_hover_note);
									eof_selection.current = EOF_MAX_NOTES - 1;
									eof_track_sort_notes(eof_song, eof_selected_track);
									eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Fixup notes and retain note selection
									eof_determine_phrase_status();
									eof_detect_difficulties(eof_song);
								}
								else if(note & bitmask)
								{	//If toggling this lane on, create a new note
									void *new_note = NULL;
									new_note = eof_track_add_create_note(eof_song, eof_selected_track, bitmask, eof_get_note_pos(eof_song, eof_selected_track, eof_hover_note), eof_get_note_length(eof_song, eof_selected_track, eof_hover_note), eof_note_type, NULL);
										//Create a new note at the hover note's position, with the same length and type as the hover note
									if(new_note)
									{	//If the new note was created successfully
										if(eof_mark_drums_as_cymbal)
										{	//If the user opted to make all new drum notes cymbals automatically
											eof_mark_new_note_as_cymbal(eof_song,eof_selected_track,eof_get_track_size(eof_song, eof_selected_track) - 1);
										}
										if(eof_mark_drums_as_double_bass)
										{	//If the user opted to make all new expert bass drum notes as double bass automatically
											eof_mark_new_note_as_double_bass(eof_song,eof_selected_track,eof_get_track_size(eof_song, eof_selected_track) - 1);
										}
										eof_track_sort_notes(eof_song, eof_selected_track);
									}
								}
								else
								{	//Otherwise a gem is being toggled off
									eof_set_note_note(eof_song, eof_selected_track, eof_hover_note, note);
								}
							}
							if(note != 0)
							{	//Cleanup edited/added note
								eof_selection.track = eof_selected_track;
								eof_selection.multi[eof_selection.current] = 1;
								eof_selection.current_pos = eof_get_note_pos(eof_song, eof_selected_track, eof_selection.current);
								eof_selection.range_pos_1 = eof_selection.current_pos;
								eof_selection.range_pos_2 = eof_selection.current_pos;
								eof_track_fixup_notes(eof_song, eof_selected_track, 0);	//Run cleanup to prevent open bass<->lane 1 conflicts
							}
						}
					}//If altering an existing note
					else
					{	//If creating a new note
						new_note = eof_track_add_create_note(eof_song, eof_selected_track, eof_pen_note.note, eof_pen_note.pos, (KEY_EITHER_SHIFT ? 1 : eof_snap.length), eof_note_type, NULL);
						if(new_note)
						{
							if(eof_mark_drums_as_cymbal)
							{	//If the user opted to make all new drum notes cymbals automatically
								eof_mark_new_note_as_cymbal(eof_song,eof_selected_track,eof_get_track_size(eof_song, eof_selected_track) - 1);
							}
							if(eof_mark_drums_as_double_bass)
							{	//If the user opted to make all new expert bass drum notes as double bass automatically
								eof_mark_new_note_as_double_bass(eof_song,eof_selected_track,eof_get_track_size(eof_song, eof_selected_track) - 1);
							}
							eof_selection.current_pos = eof_get_note_pos(eof_song, eof_selected_track, eof_get_track_size(eof_song, eof_selected_track) - 1);
							eof_selection.range_pos_1 = eof_selection.current_pos;
							eof_selection.range_pos_2 = eof_selection.current_pos;
							eof_selection.track = eof_selected_track;
							if(!eof_add_new_notes_to_selection)
							{	//If the user didn't opt to prevent clearing the note selection when adding gems
								memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
							}
							eof_track_sort_notes(eof_song, eof_selected_track);
							eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Fixup notes and retain note selection
							eof_determine_phrase_status();
							eof_detect_difficulties(eof_song);
						}
					}
				}
			}//Right mouse click or Insert key pressed, and the pen note is valid
			if(!(mouse_b & 2) && !key[KEY_INSERT])
			{
				eof_rclick_released = 1;
			}

			/* increment/decrement fret value (CTRL+scroll wheel) */
			/* increase/decrease note length (scroll wheel or SHIFT+scroll wheel) */
			if(eof_mickey_z != 0)
			{	//If there was scroll wheel activity
				if(KEY_EITHER_CTRL && !KEY_EITHER_SHIFT)
				{	//increment/decrement fret value
					if(eof_mickey_z > 0)
					{	//Decrement fret value
						eof_set_pro_guitar_fret_number(2,0);	//Decrement fret value
					}
					else if(eof_mickey_z < 0)
					{	//Increment fret value
						eof_set_pro_guitar_fret_number(1,0);	//Increment fret value
					}
				}
				else
				{	//increase/decrease note length
					unsigned long adjust = abs(eof_mickey_z) * 100;	//Default adjustment length when grid snap is disabled
					if(eof_snap_mode == EOF_SNAP_OFF)
					{
						if(KEY_EITHER_SHIFT)
						{
							if(KEY_EITHER_CTRL)
							{	//CTRL+SHIFT+scroll wheel
								adjust = abs(eof_mickey_z);	//1 ms length change
							}
							else
							{	//SHIFT+scroll wheel
								adjust = abs(eof_mickey_z) * 10;	//10ms length change
							}
						}
					}
					else
					{
						adjust = 0;	//Will indicate to eof_adjust_note_length() to use the grid snap value
					}
					if(eof_mickey_z > 0)
					{	//Decrease note length
						eof_adjust_note_length(eof_song, eof_selected_track, adjust, -1);	//Decrease selected notes by the appropriate length
					}
					else if(eof_mickey_z < 0)
					{	//Increase note length
						eof_adjust_note_length(eof_song, eof_selected_track, adjust, 1);	//Increase selected notes by the appropriate length
					}
				}
			}//If there was scroll wheel activity
		}//mouse is in the fretboard area
		else
		{
			eof_pen_visible = 0;
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
		if(eof_selected_track == EOF_TRACK_DRUM)
		{
			eof_editor_drum_logic();
		}
	}//If the chart is not paused

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
		{	//mouse is in the fretboard area
			if(eof_hover_note >= 0)
			{
				if(eof_count_selected_notes(NULL, 0) <= 0)
				{
					eof_selection.current = eof_hover_note;
					eof_selection.current_pos = eof_get_note_pos(eof_song, eof_selected_track, eof_selection.current);
					if(eof_selection.track != eof_selected_track)
					{
						eof_selection.track = eof_selected_track;
						memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
					}
					eof_selection.multi[eof_selection.current] = 1;
					eof_render();
				}
				else
				{
					if(!eof_selection.multi[eof_hover_note])
					{
						eof_selection.current = eof_hover_note;
						eof_selection.current_pos = eof_get_note_pos(eof_song, eof_selected_track, eof_selection.current);
						memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
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
//	eof_log("eof_vocal_editor_logic() entered");

	unsigned long i, notepos;
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
				eof_track_sort_notes(eof_song, eof_selected_track);
				eof_track_fixup_notes(eof_song, eof_selected_track, 1);
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
				int x_tolerance = 6 * eof_zoom;	//This is how far left or right of a lyric the mouse is allowed to be to still be considered to hover over that lyric
				npos = eof_song->vocal_track[tracknum]->lyric[i]->pos;
				if((use_this_x > npos) && (use_this_x < npos + eof_song->vocal_track[tracknum]->lyric[i]->length))
				{
					eof_hover_note = i;
				}
				else if((eof_pen_lyric.pos > npos - x_tolerance) && (eof_pen_lyric.pos < npos + x_tolerance))
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

			/* handle initial left click */
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
											memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
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
											memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
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
								memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
							}
							if(eof_selection.multi[eof_selection.current] == 1)
							{
								if(eof_selection.track != EOF_TRACK_VOCALS)
								{
									eof_selection.track = EOF_TRACK_VOCALS;
									memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
								}
								eof_selection.multi[eof_selection.current] = 2;
								eof_undo_last_type = EOF_UNDO_TYPE_NONE;
							}
							else
							{
								if(eof_selection.track != EOF_TRACK_VOCALS)
								{
									eof_selection.track = EOF_TRACK_VOCALS;
									memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
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
								memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
							}
							eof_selection.multi[eof_selection.current] = 2;
							eof_undo_last_type = EOF_UNDO_TYPE_NONE;
						}
						else
						{
							if(eof_selection.track != EOF_TRACK_VOCALS)
							{
								eof_selection.track = EOF_TRACK_VOCALS;
								memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
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
						memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
						eof_selection.current = EOF_MAX_NOTES - 1;
						eof_selection.current_pos = 0;
						eof_selection.range_pos_1 = 0;
						eof_selection.range_pos_2 = 0;
					}
				}
			}//handle initial left click
			if(!(mouse_b & 1))
			{	//If the left mouse button is NOT being held
				if(!eof_lclick_released)
				{
					eof_lclick_released = 1;	//Set the "just released" status
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
								memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
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
									memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
								}
								eof_selection.multi[eof_selection.current] = 1;
								eof_undo_last_type = EOF_UNDO_TYPE_NONE;
							}
						}
					}
					eof_lclick_released = 2;	//Set the "left click release handled" status
				}
				else
				{
					if(eof_mouse_drug)
					{
						eof_mouse_drug = 0;
					}
				}
			}
			unsigned long move_offset = 0;
			char move_direction = 1;	//Assume right mouse movement by default
			int revert = 0;
			int revert_amount = 0;
			char undo_made = 0;
			if((mouse_b & 1) && !eof_lclick_released && (lpos >= eof_peg_x))
			{	//If the left mouse button is being held and the mouse is right of the left edge of the piano roll
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
					{	//Move notes by grid snap
						eof_snap_logic(&drag_snap, lpos - eof_peg_x);
						if(drag_snap.pos < eof_song->vocal_track[tracknum]->lyric[eof_pegged_note]->pos)
						{	//Left mouse movement
							move_direction = -1;
							move_offset = eof_song->vocal_track[tracknum]->lyric[eof_pegged_note]->pos - drag_snap.pos;
						}
						else
						{	//Right mouse movement
							move_direction = 1;
							move_offset = drag_snap.pos - eof_song->vocal_track[tracknum]->lyric[eof_pegged_note]->pos;
						}
						eof_last_pen_pos = rpos;
					}
					else
					{
						if(eof_mickeys_x < 0)
						{	//Left mouse movement
							move_direction = -1;
							move_offset = -eof_mickeys_x * eof_zoom;
						}
						else
						{	//Right mouse movement
							move_direction = 1;
							move_offset = eof_mickeys_x * eof_zoom;
						}
					}
					for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
					{	//For each lyric in the track
						notepos = eof_get_note_pos(eof_song, eof_selected_track, i);
						if(eof_selection.multi[i])
						{
							if(notepos == eof_selection.current_pos)
							{
								if(move_direction < 0)
								{	//Left mouse movement
									eof_selection.current_pos -= move_offset;
								}
								else
								{	//Right mouse movement
									eof_selection.current_pos += move_offset;
								}
							}
							if(move_direction < 0)
							{	//If the user is moving lyrics left
								if((move_offset > notepos) || (notepos - move_offset < eof_song->beat[0]->pos))
								{	//If the move would make the lyric position negative or otherwise earlier than the first beat
									move_offset = notepos - eof_song->beat[0]->pos;	//Adjust the move offset to line it up with the first beat marker
								}
							}
							if(move_offset == 0)
							{	//If the offset has become 0
								break;	//Don't move the notes
							}
							if(!undo_made)
							{	//Only create the undo state before moving the first note
								eof_notes_moved = 1;
								eof_prepare_undo(EOF_UNDO_TYPE_NONE);
								undo_made = 1;
							}
							eof_move_note_pos(eof_song, eof_selected_track, i, move_offset, move_direction);
							notepos = eof_get_note_pos(eof_song, eof_selected_track, i);	//Get the updated lyric position
							if(notepos + eof_song->vocal_track[tracknum]->lyric[i]->length >= eof_music_length)
							{
								revert = 1;
								revert_amount = notepos + eof_song->vocal_track[tracknum]->lyric[i]->length - eof_music_length;
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
				if(eof_hover_note >= 0)
				{	//If clicking on an existing lyric
					if(!eof_undo_toggle)
					{
						eof_prepare_undo(EOF_UNDO_TYPE_NOTE_SEL);
					}

					/* delete whole lyric if clicking word */
					if((mouse_y >= EOF_EDITOR_RENDER_OFFSET + 35 + eof_screen_layout.lyric_y) && (mouse_y < EOF_EDITOR_RENDER_OFFSET + 35 + eof_screen_layout.lyric_y + 16))
					{
						eof_track_delete_note(eof_song, eof_selected_track, eof_hover_note);	//Delete the hovered over lyric
						eof_selection.current = EOF_MAX_NOTES - 1;
						eof_track_sort_notes(eof_song, eof_selected_track);
					}

					/* delete only the note */
					else if(eof_pen_lyric.note == eof_song->vocal_track[tracknum]->lyric[eof_hover_note]->note)
					{
						if(!eof_check_string(eof_song->vocal_track[tracknum]->lyric[eof_hover_note]->text))	//If removing the pitch from a note that is already textless
						{	//Perform the cleanup code as if the lyric was deleted by inputting on top of the lyric text as above
							eof_track_delete_note(eof_song, eof_selected_track, eof_hover_note);	//Delete the hovered over lyric
							eof_selection.current = EOF_MAX_NOTES - 1;
							eof_track_sort_notes(eof_song, eof_selected_track);
						}
						else if(eof_rclick_released)	//Just remove the pitch, but only if the right mouse button was just clicked
						{
							eof_song->vocal_track[tracknum]->lyric[eof_hover_note]->note = 0;
							eof_selection.current = eof_hover_note;
							eof_selection.track = eof_selected_track;
							memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
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
						memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
						eof_selection.multi[eof_selection.current] = 1;
						eof_selection.current_pos = eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->pos;
						eof_selection.range_pos_1 = eof_selection.current_pos;
						eof_selection.range_pos_2 = eof_selection.current_pos;
						if(eof_mix_vocal_tones_enabled)
						{
							eof_mix_play_note(eof_pen_lyric.note);
						}
					}
					eof_rclick_released = 0;	//Track that the right mouse button is being held
				}

				/* create new note */
				else
				{
					eof_rclick_released = 0;	//Track that the right mouse button is being held
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
				long b;
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
//							allegro_message("%d, %d\n%lu, %lu", eof_tail_snap.length, eof_tail_snap.beat, eof_get_note_pos(eof_selected_track, i) + eof_get_note_length(eof_selected_track, i), eof_song->beat[eof_tail_snap.beat]->pos);	//Debugging
							eof_song->vocal_track[tracknum]->lyric[i]->length -= eof_tail_snap.length;
							if(eof_song->vocal_track[tracknum]->lyric[i]->length > 1)
							{
								eof_snap_logic(&eof_tail_snap, eof_song->vocal_track[tracknum]->lyric[i]->pos + eof_song->vocal_track[tracknum]->lyric[i]->length);
								eof_note_set_tail_pos(eof_song, eof_selected_track, i, eof_tail_snap.pos);
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
								eof_note_set_tail_pos(eof_song, eof_selected_track, i, eof_tail_snap.pos);
							}
							else
							{
								eof_snap_length_logic(&eof_tail_snap);
								eof_song->vocal_track[tracknum]->lyric[i]->length += eof_tail_snap.length;
								eof_snap_logic(&eof_tail_snap, eof_song->vocal_track[tracknum]->lyric[i]->pos + eof_song->vocal_track[tracknum]->lyric[i]->length);
								eof_note_set_tail_pos(eof_song, eof_selected_track, i, eof_tail_snap.pos);
							}
						}
					}
				}
			}
			if(eof_mickey_z != 0)
			{
				eof_track_fixup_notes(eof_song, eof_selected_track, 1);
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
		if(i == eof_song->vocal_track[tracknum]->lyrics)
		{
			eof_pen_lyric.note = 0;
		}
	}//If the chart is not paused

	if(((mouse_b & 2) || key[KEY_INSERT]) && (eof_input_mode == EOF_INPUT_REX))
	{	//Open context menu if right click or Insert is used in Rex Mundi input mode
		eof_emergency_stop_music();
		eof_render();
		eof_show_mouse(screen);
		if((mouse_y >= eof_window_editor->y + EOF_EDITOR_RENDER_OFFSET - 4) && (mouse_y < eof_window_editor->y + EOF_EDITOR_RENDER_OFFSET + 18))
		{
			int pos = eof_music_pos / eof_zoom;
			int lpos = pos < 300 ? (eof_song->beat[eof_selected_beat]->pos / eof_zoom + 20) : 300;
			eof_prepare_menus();
			do_menu(eof_beat_menu, lpos, mouse_y);
			eof_clear_input();
		}
		else if((mouse_y >= eof_window_editor->y + 25 + EOF_EDITOR_RENDER_OFFSET) && (mouse_y < eof_window_editor->y + eof_screen_layout.fretboard_h + EOF_EDITOR_RENDER_OFFSET))
		{	//mouse is in the fretboard area
			if(eof_hover_note >= 0)
			{
				if(eof_count_selected_notes(NULL, 0) <= 0)
				{
					eof_selection.current = eof_hover_note;
					eof_selection.current_pos = eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->pos;
					if(eof_selection.track != EOF_TRACK_VOCALS)
					{
						eof_selection.track = EOF_TRACK_VOCALS;
						memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
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
						memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
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
//	eof_log("eof_get_ts_text() entered");

	if(!buffer)
	{
		return 0;
	}

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
//	eof_log("eof_render_editor_window() entered");

	unsigned long start;	//Will store the timestamp of the left visible edge of the piano roll
	unsigned long i,numnotes;

	if(!eof_song_loaded)
		return;

	if(eof_disable_2d_rendering)	//If the user wanted to disable the rendering of the 2D window to improve performance
		return;						//Return immediately

	eof_render_editor_window_common();	//Perform rendering that is common to the note and the vocal editor displays

	numnotes = eof_get_track_size(eof_song, eof_selected_track);	//Get the number of notes in this legacy/pro guitar track
	start = eof_determine_piano_roll_left_edge();
	for(i = 0; i < numnotes; i++)
	{	//Render all visible notes in the list
		if((eof_note_type == eof_get_note_type(eof_song, eof_selected_track, i)) && (eof_get_note_pos(eof_song, eof_selected_track, i) + eof_get_note_length(eof_song, eof_selected_track, i) >= start))
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
//	eof_log("eof_render_vocal_editor_window() entered");

	unsigned long i;
	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	int pos = eof_music_pos / eof_zoom;	//Current seek position
	int lpos;							//The position of the first beat marker
	unsigned long start;	//Will store the timestamp of the left visible edge of the piano roll

	if(!eof_song_loaded)
		return;
	if(!eof_vocals_selected)
		return;

	if(eof_disable_2d_rendering)	//If the user wanted to disable the rendering of the 2D window to improve performance
		return;						//Return immediately

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
//	eof_log("eof_determine_piano_roll_left_edge() entered");

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
//	eof_log("eof_determine_piano_roll_right_edge() entered", 1);

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
//	eof_log("eof_render_editor_window_common() entered");

	unsigned long i, j, ctr, notepos, markerlength;
	int pos = eof_music_pos / eof_zoom;	//Current seek position
	int lpos;							//The position of the first beatmarker
	int pmin = 0;
	int psec = 0;
	int xcoord;							//Used to cache x coordinate to reduce recalculations
	int col,col2;						//Temporary color variables
	unsigned long start;				//Will store the timestamp of the left visible edge of the piano roll
	unsigned long stop;					//Will store the timestamp of the right visible edge of the piano roll
	unsigned long numlanes;				//The number of fretboard lanes that will be rendered
	short numsections = 0;					//Used to abstract the solo sections
	EOF_PHRASE_SECTION *sectionptr = NULL;	//Used to abstract sections
	unsigned long tracknum;
	unsigned long bitmask, usedlanes;
	long notelength;

	int colors[EOF_MAX_FRETS] = {makecol(170,255,170), makecol(255,156,156), makecol(255,255,224), makecol(156,156,255), makecol(255,156,255), makecol(255,170,128)};	//Lightened versions of the standard fret colors

	if(!eof_song_loaded)
		return;

	numlanes = eof_count_track_lanes(eof_song, eof_selected_track);
	eof_set_2D_lane_positions(eof_selected_track);	//Update the ychart[] array

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
	stop = eof_determine_piano_roll_right_edge();	//Find the timestamp of the right visible edge of the piano roll

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

	/* draw solo sections */
	if(eof_selected_track != EOF_TRACK_VOCALS)
	{
		numsections = eof_get_num_solos(eof_song, eof_selected_track);
		for(i = 0; i < numsections; i++)
		{	//For each solo section in the track
			sectionptr = eof_get_solo(eof_song, eof_selected_track, i);	//Obtain the information for this legacy/pro guitar solo
			if(sectionptr != NULL)
			{
				if((sectionptr->end_pos >= start) && (sectionptr->start_pos <= stop))	//If the solo section would render between the left and right edges of the piano roll
					rectfill(eof_window_editor->screen, lpos + sectionptr->start_pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 25, lpos + sectionptr->end_pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 1, eof_color_dark_blue);
			}
		}
	}

	/* draw SP sections */
	if(eof_selected_track != EOF_TRACK_VOCALS)
	{
		numsections = eof_get_num_star_power_paths(eof_song, eof_selected_track);
		for(i = 0; i < numsections; i++)
		{	//For each solo section in the track
			sectionptr = eof_get_star_power_path(eof_song, eof_selected_track, i);	//Obtain the information for this star power section
			if(sectionptr != NULL)
			{
				if((sectionptr->end_pos >= start) && (sectionptr->start_pos <= stop))	//If the star power section would render between the left and right edges of the piano roll, render a silver rectangle from the top most lane to the top of the fretboard area
					rectfill(eof_window_editor->screen, lpos + sectionptr->start_pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 25, lpos + sectionptr->end_pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[0], eof_color_silver);
			}
		}
	}

	/* draw arpeggio sections */
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
	{
		col = makecol(51,166,153);	//Store turquoise color (use (68,221,204) for light turquoise)
		for(i = 0; i < eof_song->pro_guitar_track[tracknum]->arpeggios; i++)
		{	//For each arpeggio section in the track
			sectionptr = &eof_song->pro_guitar_track[tracknum]->arpeggio[i];
			if((sectionptr->end_pos >= start) && (sectionptr->start_pos <= stop) && (sectionptr->difficulty == eof_note_type))
			{	//If the arpeggio section would render between the left and right edges of the piano roll, and the section applies to the active difficulty, fill the bottom lane with turquoise
				rectfill(eof_window_editor->screen, lpos + sectionptr->start_pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[numlanes - 2], lpos + sectionptr->end_pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[numlanes - 1], col);
			}
		}
	}

	/* draw trill and tremolo sections */
	if(eof_get_num_trills(eof_song, eof_selected_track) || eof_get_num_tremolos(eof_song, eof_selected_track))
	{	//If this track has any trill or tremolo sections
		for(j = 0; j < 2; j++)
		{	//For each of the two phrase types (trills and tremolos)
			if(j == 0)
			{	//On the first pass, render trill sections
				numsections = eof_get_num_trills(eof_song, eof_selected_track);
			}
			else
			{	//On the second pass, render tremolo sections
				numsections = eof_get_num_tremolos(eof_song, eof_selected_track);
			}
			for(i = 0; i < numsections; i++)
			{	//For each trill or tremolo section in the track
				if(j == 0)
				{	//On the first pass, render trill sections
					sectionptr = eof_get_trill(eof_song, eof_selected_track, i);
				}
				else
				{	//On the second pass, render tremolo sections
					sectionptr = eof_get_tremolo(eof_song, eof_selected_track, i);
				}
				if(sectionptr != NULL)
				{	//If the section exists
					if((sectionptr->end_pos >= start) && (sectionptr->start_pos <= stop))
					{	//If the trill or tremolo section would render between the left and right edges of the piano roll
						usedlanes = eof_get_used_lanes(eof_selected_track, sectionptr->start_pos, sectionptr->end_pos, eof_note_type);	//Determine which lane(s) use this phrase
						if(usedlanes == 0)
						{	//If there are no notes in this marker, render the marker in all lanes
							usedlanes = 0xFF;
						}
						for(ctr = 0, bitmask = 1; ctr < eof_count_track_lanes(eof_song, eof_selected_track); ctr++, bitmask <<= 1)
						{	//For each of the track's usable lanes
							if(usedlanes & bitmask)
							{	//If this lane is used in the phrase
								int x1 = lpos + sectionptr->start_pos / eof_zoom;
								int y1 = EOF_EDITOR_RENDER_OFFSET + 15 + ychart[ctr] - (eof_screen_layout.string_space / 2);
								int x2 = lpos + sectionptr->end_pos / eof_zoom;
								int y2 = EOF_EDITOR_RENDER_OFFSET + 15 + ychart[ctr] + (eof_screen_layout.string_space / 2);
								if(y1 < EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[0])
									y1 = EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[0];	//Ensure that the phrase cannot render above the top most lane
								if(y2 > EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[numlanes-1])
									y2 = EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[numlanes-1];	//Ensure that the phrase cannot render below the bottom most lane
								rectfill(eof_window_editor->screen, x1, y1, x2, y2, colors[ctr]);	//Draw a rectangle one lane high centered over that lane's fret line
							}
						}
					}
				}
			}
		}
	}//If this track has any trill or tremolo sections

	/* draw slider sections */
	if((eof_song->track[eof_selected_track]->track_behavior == EOF_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_format == EOF_LEGACY_TRACK_FORMAT))
	{	//If this is a legacy guitar track
		numsections = eof_get_num_sliders(eof_song, eof_selected_track);
		for(i = 0; i < numsections; i++)
		{	//For each slider section in the track
			sectionptr = eof_get_slider(eof_song, eof_selected_track, i);	//Obtain the information for this slider section
			if(sectionptr != NULL)
			{
				if((sectionptr->end_pos >= start) && (sectionptr->start_pos <= stop))	//If the slider section would render between the left and right edges of the piano roll, render a dark purple rectangle above the fretboard area
					rectfill(eof_window_editor->screen, lpos + sectionptr->start_pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15, lpos + sectionptr->end_pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 5 + eof_screen_layout.note_y[0], eof_color_dark_purple);
			}
		}
	}

	/* draw undefined legacy mask markers */
	if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
	{	//If legacy view is in effect
		col = makecol(176, 48, 96);	//Store maroon color
		for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
		{	//For each note in this track
			notepos = eof_get_note_pos(eof_song, eof_selected_track, i);
			notelength = eof_get_note_length(eof_song, eof_selected_track, i);
			if((eof_note_type == eof_get_note_type(eof_song, eof_selected_track, i)) && (notepos + notelength >= start) && (notepos <= stop))
			{	//If this note is in the selected instrument difficulty and would render between the left and right edges of the piano roll
				if(eof_song->pro_guitar_track[tracknum]->note[i]->legacymask == 0)
				{	//If this note does not have a defined legacy mask, render a maroon colored section a minimum of eof_screen_layout.note_size pixels long
					markerlength = notelength / eof_zoom;
					if(markerlength < eof_screen_layout.note_size)
					{	//If this marker isn't at least as wide as a note gem
						markerlength = eof_screen_layout.note_size;	//Make it longer
					}
					rectfill(eof_window_editor->screen, lpos + notepos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 25, lpos + (notepos / eof_zoom) + markerlength, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 1, col);
				}
			}
		}
	}

	if(eof_display_waveform)
		eof_render_waveform(eof_waveform);

	/* draw fretboard area */
	for(i = 0; i < numlanes; i++)
	{
		if(!i || (i + 1 >= numlanes))
		{	//Ensure the top and bottom lines extend to the left of the piano roll
			hline(eof_window_editor->screen, lpos, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[i], lpos + (eof_music_length) / eof_zoom, eof_color_white);
		}
		else if(eof_selected_track != EOF_TRACK_VOCALS)
		{	//Otherwise, if not drawing the vocal editor, draw the other fret lines from the first beat marker to the end of the chart
			hline(eof_window_editor->screen, lpos + eof_song->tags->ogg[eof_selected_ogg].midi_offset / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[i], lpos + (eof_music_length) / eof_zoom, eof_color_white);
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
	char first_measure = 0;	//Set to nonzero when the first measure marker is reached
	char notvisible;

	bcol = makecol(128, 128, 128);
	bscol = eof_color_white;
	bhcol = eof_color_green;
	col = makecol(112, 112, 112);	//Cache this color
	col2 = makecol(160, 160, 160);	//Cache this color

	for(i = 0; i < eof_song->beats; i++)
	{
		if(eof_get_ts(eof_song,&beats_per_measure,NULL,i) == 1)
		{	//If this beat has a time signature change
			first_measure = 1;	//Note that a time signature change has been found
			beat_counter = 0;
		}
		if(first_measure && (beat_counter == 0))
		{	//If there was a TS change or the beat markers incremented enough to reach the next measure
			measure_counter++;
		}
		xcoord = lpos + eof_song->beat[i]->pos / eof_zoom;
		if(xcoord >= eof_window_editor->screen->w)
		{	//If this beat would render further right than the right edge of the screen
			notvisible = 1;
		}
		else if(xcoord < 0)
		{	//If the beat marker would render further left than the left edge of the screen
			notvisible = -1;
		}
		else
		{	//The beat would render visibly
			notvisible = 0;
		}

		//Perform beat tracking logic even if the beat isn't going to be rendered (to track tempo changes, etc)
		if(eof_song->beat[i]->ppqn != current_ppqn)
		{
			current_ppqn = eof_song->beat[i]->ppqn;
			current_ppqn_used = 0;
			current_bpm = (double)60000000.0 / (double)eof_song->beat[i]->ppqn;
		}
		if(i == eof_selected_beat)
		{	//Draw selected beat's tempo
			if(notvisible == 0)
			{	//Only render the tempo if the beat is visible
				textprintf_ex(eof_window_editor->screen, eof_mono_font, xcoord - 28, EOF_EDITOR_RENDER_OFFSET + 6 - (i % 2 == 0 ? 0 : 10), i == eof_hover_beat ? bhcol : i == eof_selected_beat ? bscol : bcol, -1, "<%03.2f>", current_bpm);
			}
			current_ppqn_used = 1;
		}
		else if(!current_ppqn_used)
		{	//Draw tempo
			if(notvisible == 0)
			{	//Only render the tempo if the beat is visible
				textprintf_ex(eof_window_editor->screen, eof_mono_font, xcoord - 20, EOF_EDITOR_RENDER_OFFSET + 6 - (i % 2 == 0 ? 0 : 10), i == eof_hover_beat ? bhcol : i == eof_selected_beat ? bscol : bcol, -1, "%03.2f", current_bpm);
			}
			current_ppqn_used = 1;
		}
		else
		{	//Draw beat arrow
			if(notvisible == 0)
			{	//Only render the beat arrow if the beat is visible
				textprintf_ex(eof_window_editor->screen, eof_mono_font, xcoord - 20 + 9, EOF_EDITOR_RENDER_OFFSET + 6 - (i % 2 == 0 ? 0 : 10), i == eof_hover_beat ? bhcol : i == eof_selected_beat ? bscol : bcol, -1, "-->");
			}
		}
		if(eof_get_ts_text(i, buffer))
		{	//Draw time signature
			if(notvisible == 0)
			{	//Only render the time signature if the beat is visible
				textprintf_centre_ex(eof_window_editor->screen, eof_mono_font, xcoord - 20 + 3 + 16, EOF_EDITOR_RENDER_OFFSET + 6 - (i % 2 == 0 ? 0 : 10) - 12, i == eof_hover_beat ? bhcol : i == eof_selected_beat ? bscol : bcol, -1, "(%s)", buffer);
			}
		}

		if(notvisible > 0)
		{	//If this beat marker would render further right than the right edge of the screen
			break;	//Skip rendering this and all other beat markers, which would continue to render off screen
		}
		if(notvisible >= 0)
		{	//If this beat marker would not render further left than the left edge of the screen
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

			if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_EVENTS)
			{	//Draw event marker
				line(eof_window_editor->screen, xcoord - 3, EOF_EDITOR_RENDER_OFFSET + 24, xcoord + 3, EOF_EDITOR_RENDER_OFFSET + 24, eof_color_yellow);
			}
			if(first_measure && (beat_counter == 0))
			{	//If this is a measure marker, draw the measure number to the right of the beat line
				textprintf_ex(eof_window_editor->screen, eof_mono_font, xcoord + 2, EOF_EDITOR_RENDER_OFFSET + 22 - 7, eof_color_yellow, -1, "%lu", measure_counter);
			}
			if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_ANCHOR)
			{	//Draw anchor marker
				line(eof_window_editor->screen, xcoord - 3, EOF_EDITOR_RENDER_OFFSET + 21, xcoord, EOF_EDITOR_RENDER_OFFSET + 24, eof_color_red);
				line(eof_window_editor->screen, xcoord + 3, EOF_EDITOR_RENDER_OFFSET + 21, xcoord, EOF_EDITOR_RENDER_OFFSET + 24, eof_color_red);
			}
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
//	eof_log("eof_render_editor_window_common2() entered");

	int i;
	int pos = eof_music_pos / eof_zoom;	//Current seek position compensated for zoom level
	int zoom = eof_av_delay / eof_zoom;	//AV delay compensated for zoom level

	if(!eof_song_loaded)
		return;

	/* draw the current position */
	if(pos > zoom)
	{
		if(pos < 300)
		{
			vline(eof_window_editor->screen, 20 + pos - zoom, EOF_EDITOR_RENDER_OFFSET + 25, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 1, eof_color_green);
		}
		else
		{
			vline(eof_window_editor->screen, 320 - zoom, EOF_EDITOR_RENDER_OFFSET + 25, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 1, eof_color_green);
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

	char *tab_name;
	for(i = 0; i < 5; i++)
	{	//Draw tab difficulty names
		if(eof_selected_track == EOF_TRACK_VOCALS)
		{
			tab_name = eof_vocal_tab_name[i];
		}
		else if(eof_selected_track == EOF_TRACK_DANCE)
		{
			tab_name = eof_dance_tab_name[i];
		}
		else
		{
			tab_name = eof_note_type_name[i];
		}
		if(i == eof_note_type)
		{
			textprintf_centre_ex(eof_window_editor->screen, font, 50 + i * 80, 2 + 8, eof_color_black, -1, "%s", tab_name);
		}
		else
		{
			textprintf_centre_ex(eof_window_editor->screen, font, 50 + i * 80, 2 + 2 + 8, makecol(128, 128, 128), -1, "%s", tab_name);
		}
		if((eof_selected_track == EOF_TRACK_VOCALS) && (i == eof_vocals_tab))
		{
			break;
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
	eof_log("eof_mark_new_note_as_cymbal() entered", 1);

	if(sp)
	{
		eof_mark_edited_note_as_cymbal(sp,track,notenum,0xFFFF);
	}
}

void eof_mark_edited_note_as_cymbal(EOF_SONG *sp, unsigned long track, unsigned long notenum, unsigned long bitmask)
{
	eof_log("eof_mark_edited_note_as_cymbal() entered", 1);

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

void eof_mark_new_note_as_double_bass(EOF_SONG *sp, unsigned long track, unsigned long notenum)
{
	eof_log("eof_mark_new_note_as_double_bass() entered", 1);

	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return;
	tracknum = sp->track[track]->tracknum;

	if((sp->track[track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR) && (eof_get_note_type(eof_song, eof_selected_track, notenum) == EOF_NOTE_AMAZING))
	{	//If the note is an Expert drum note
		if((notenum >= sp->legacy_track[tracknum]->notes) || (sp->legacy_track[tracknum]->note[notenum] == NULL))
			return;

		//For now, it's assumed that any drum behavior track will be a legacy format track
		if(sp->legacy_track[tracknum]->note[notenum]->note & 1)
		{	//If the note has lane 1 populated
			sp->legacy_track[tracknum]->note[notenum]->flags |= EOF_NOTE_FLAG_DBASS;	//Set the double bass flag
		}
	}
}

unsigned char eof_find_pen_note_mask(void)
{
//	eof_log("eof_find_pen_note_mask() entered");

	unsigned long laneborder;
	unsigned long i;
	unsigned long lanecount = 0;

	//Determine which lane the mouse is in
	eof_hover_piece = -1;
	lanecount = eof_count_track_lanes(eof_song, eof_selected_track);
	for(i = 0; i < lanecount; i++)
	{	//For each of the usable lanes
		laneborder = eof_window_editor->y + EOF_EDITOR_RENDER_OFFSET + 15 + 10 + eof_screen_layout.note_y[i];	//This represents the y position of the boundary between the current lane and the next
		if((mouse_y < laneborder) && (mouse_y > laneborder - eof_screen_layout.string_space))
		{	//If the mouse's Y position is below the boundary to the next lane but above the boundary to the previous lane
			eof_hover_piece = i;	//Store the lane number
		}
	}

	/* see if we are inverting the lanes */
	if(eof_inverted_notes || (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
	{	//If the user opted to invert the notes in the piano roll, or if the current track is a pro guitar/bass track, return the appropriate inverted pen mask
		return ((1 << (lanecount - 1)) >> eof_hover_piece);	//This finds the appropriate invert mask, where mousing over the bottom most lane activates lane 1 for the pen mask
	}

	return (1 << eof_hover_piece);	//Return the normal pen mask, where mousing over the top most lane activates lane 1 for the pen mask
}

void eof_editor_logic_common(void)
{
//	eof_log("eof_editor_logic_common() entered");

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
					eof_song->beat[eof_selected_beat]->fpos += eof_mickeys_x * eof_zoom;
					eof_song->beat[eof_selected_beat]->pos = eof_song->beat[eof_selected_beat]->fpos + 0.5;	//Round up to nearest ms
					if(((eof_selected_beat > 0) && (eof_song->beat[eof_selected_beat]->pos <= eof_song->beat[eof_selected_beat - 1]->pos + 50)) || ((eof_selected_beat + 1 < eof_song->beats) && (eof_song->beat[eof_selected_beat]->pos >= eof_song->beat[eof_selected_beat + 1]->pos - 50)))
					{	//If the beat being drug was moved to within within 50ms of the previous/next beat marker, undo the move
						eof_song->beat[eof_selected_beat]->fpos -= eof_mickeys_x * eof_zoom;
						eof_song->beat[eof_selected_beat]->pos = eof_song->beat[eof_selected_beat]->fpos + 0.5;	//Round up to nearest ms
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
						eof_fixup_notes(eof_song);
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

	//Find hover notes/lyrics for chart and fret catalog playback
	if(eof_music_catalog_playback || !eof_music_paused)
	{	//If the chart or fret catalog is playing
		int examined_music_pos = eof_music_pos;		//By default, assume the chart position is to be used to find hover notes/etc.
		int examined_track = eof_selected_track;	//By default, assume the active track is to be used to find hover notes/etc.
		int examined_type = eof_note_type;			//By default, assume the active difficulty is to be used to find hover notes/etc.

		if(eof_music_catalog_playback)
		{	//If the fret catalog is playing
			examined_music_pos = eof_music_catalog_pos;	//Use the fret catalog position instead
			examined_track = eof_song->catalog->entry[eof_selected_catalog_entry].track;	//Use the fret catalog entry's track instead
			examined_type = eof_song->catalog->entry[eof_selected_catalog_entry].type;	//Use the fret catalog entry's difficulty instead
		}

		//Find the hover note
		int examined_pos = examined_music_pos / eof_zoom;
		int zoom = eof_av_delay / eof_zoom;	//Cache this value
		for(i = 0; i < eof_get_track_size(eof_song, examined_track); i++)
		{
			if(eof_get_note_type(eof_song, examined_track, i) == examined_type)
			{
				npos = eof_get_note_pos(eof_song, examined_track, i) / eof_zoom;
				if((examined_pos - zoom > npos) && (examined_pos - zoom < npos + (eof_get_note_length(eof_song, examined_track, i) > 100 ? eof_get_note_length(eof_song, examined_track, i) : 100) / eof_zoom))
				{
					eof_hover_note = i;
					if(eof_vocals_selected)
					{	//If the lyric track is active, set the lyric pen note
						eof_pen_lyric.note = eof_get_note_note(eof_song, examined_track, i);
					}
					break;
				}
			}
		}

		//Find the hover 2 note
		for(i = 0; i < eof_get_track_size(eof_song, examined_track); i++)
		{
			if(eof_get_note_type(eof_song, examined_track, i) == examined_type)
			{
				npos = eof_get_note_pos(eof_song, examined_track, i) + eof_av_delay;
				if((examined_music_pos > npos) && (examined_music_pos < npos + (eof_get_note_length(eof_song, examined_track, i) > 100 ? eof_get_note_length(eof_song, examined_track, i) : 100)))
				{
					eof_hover_note_2 = i;
					break;
				}
			}
		}

		//Find the hover lyric, if there is one
		for(i = 0; i < eof_song->vocal_track[0]->lyrics; i++)
		{	//Find the hover lyric if there is one
			npos = eof_song->vocal_track[0]->lyric[i]->pos;
			if((examined_music_pos - eof_av_delay > npos) && (examined_music_pos - eof_av_delay < npos + (eof_song->vocal_track[0]->lyric[i]->length > 100 ? eof_song->vocal_track[0]->lyric[i]->length : 100)))
			{
				eof_hover_lyric = i;
				break;
			}
		}
	}

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
