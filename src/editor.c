#include <math.h>
#include "main.h"
#include "menu/file.h"
#include "menu/edit.h"
#include "menu/song.h"
#include "menu/note.h"
#include "menu/beat.h"
#include "menu/help.h"
#include "menu/context.h"
#include "player.h"
#include "mix.h"
#include "undo.h"
#include "dialog.h"
#include "beat.h"
#include "editor.h"

int         eof_held_1 = 0;
int         eof_held_2 = 0;
int         eof_held_3 = 0;
int         eof_held_4 = 0;
int         eof_held_5 = 0;
int         eof_entering_note = 0;
int         eof_entered_note = 0; // if a note has been entered since audio playback started
EOF_NOTE *  eof_entering_note_note = NULL;
EOF_LYRIC *  eof_entering_note_lyric = NULL;
int         eof_snote = 0;
int         eof_last_snote = 0;
int         eof_moving_anchor = 0;
int         eof_adjusted_anchor = 0;
int         eof_anchor_diff[EOF_MAX_TRACKS + 1] = {0};

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
	eof_selected_beat = beat;
	eof_selected_measure = 0;
	eof_beat_in_measure = 0;
	eof_beats_in_measure = 1;
	
	for(i = 0; i <= beat; i++)
	{
		if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_START_4_4)
		{
			eof_beats_in_measure = 4;
			beat_counter = 0;
		}
		else if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_START_3_4)
		{
			eof_beats_in_measure = 3;
			beat_counter = 0;
		}
		else if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_START_5_4)
		{
			eof_beats_in_measure = 5;
			beat_counter = 0;
		}
		else if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_START_6_4)
		{
			eof_beats_in_measure = 6;
			beat_counter = 0;
		}
		eof_beat_in_measure = beat_counter;
		if(eof_beat_in_measure == 0)
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
	if(note < eof_song->track[eof_selected_track]->notes)
	{
		eof_song->track[eof_selected_track]->note[note]->length = pos - eof_song->track[eof_selected_track]->note[note]->pos;
	}
}

void eof_set_vocal_tail_pos(int note, unsigned long pos)
{
	if(note < eof_song->vocal_track->lyrics)
	{
		eof_song->vocal_track->lyric[note]->length = pos - eof_song->vocal_track->lyric[note]->pos;
	}
}

void eof_snap_logic(EOF_SNAP_DATA * sp, unsigned long p)
{
	int i;
	
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
				if(sp->pos >= eof_song->beat[i]->pos && sp->pos < eof_song->beat[i + 1]->pos)
				{
					sp->beat = i;
					break;
				}
			}
		}
		
		/* make sure we found a suitable snap beat before proceeding */
		if(sp->beat >= 0)
		{
			if(sp->beat >= eof_song->beats - 2)
			{
				sp->beat_length = eof_song->beat[sp->beat - 1]->pos - eof_song->beat[sp->beat - 2]->pos;
			}
			else
			{
				sp->beat_length = eof_song->beat[sp->beat + 1]->pos - eof_song->beat[sp->beat]->pos;
			}
			float least_amount = sp->beat_length;
			int least = -1;
			switch(eof_snap_mode)
			{
				case EOF_SNAP_QUARTER:
				{
					if(eof_pos_distance(eof_song->beat[sp->beat]->pos, sp->pos) < eof_pos_distance(eof_song->beat[sp->beat + 1]->pos, sp->pos))
					{
						sp->pos = eof_song->beat[sp->beat]->pos;
					}
					else
					{
						sp->pos = eof_song->beat[sp->beat + 1]->pos;
					}
					break;
				}
				case EOF_SNAP_EIGHTH:
				{
					sp->grid_pos[0] = eof_song->beat[sp->beat]->pos;
					sp->grid_pos[1] = eof_song->beat[sp->beat]->pos + (float)(sp->beat_length) / 2.0;
					sp->grid_pos[2] = eof_song->beat[sp->beat + 1]->pos;
					for(i = 0; i < 3; i++)
					{
						sp->grid_distance[i] = eof_pos_distance(sp->grid_pos[i], sp->pos);
					}
					for(i = 0; i < 3; i++)
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
					break;
				}
				case EOF_SNAP_TWELFTH:
				{
					sp->grid_pos[0] = eof_song->beat[sp->beat]->pos;
					sp->grid_pos[1] = eof_song->beat[sp->beat]->pos + ((float)sp->beat_length / 3.0);
					sp->grid_pos[2] = eof_song->beat[sp->beat]->pos + ((float)sp->beat_length / 3.0) * 2.0;
					sp->grid_pos[3] = eof_song->beat[sp->beat]->pos + ((float)sp->beat_length / 3.0) * 3.0;
					sp->grid_pos[4] = eof_song->beat[sp->beat + 1]->pos;
					for(i = 0; i < 5; i++)
					{
						sp->grid_distance[i] = eof_pos_distance(sp->grid_pos[i], sp->pos);
					}
					for(i = 0; i < 5; i++)
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
					break;
				}
				case EOF_SNAP_SIXTEENTH:
				{
					sp->grid_pos[0] = eof_song->beat[sp->beat]->pos;
					sp->grid_pos[1] = eof_song->beat[sp->beat]->pos + ((float)sp->beat_length / 4.0);
					sp->grid_pos[2] = eof_song->beat[sp->beat]->pos + ((float)sp->beat_length / 4.0) * 2.0;
					sp->grid_pos[3] = eof_song->beat[sp->beat]->pos + ((float)sp->beat_length / 4.0) * 3.0;
					sp->grid_pos[4] = eof_song->beat[sp->beat + 1]->pos;
					for(i = 0; i < 5; i++)
					{
						sp->grid_distance[i] = eof_pos_distance(sp->grid_pos[i], sp->pos);
					}
					for(i = 0; i < 5; i++)
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
					break;
				}
				case EOF_SNAP_TWENTY_FOURTH:
				{
					sp->grid_pos[0] = eof_song->beat[sp->beat]->pos;
					sp->grid_pos[1] = eof_song->beat[sp->beat]->pos + ((float)sp->beat_length / 6.0);
					sp->grid_pos[2] = eof_song->beat[sp->beat]->pos + ((float)sp->beat_length / 6.0) * 2.0;
					sp->grid_pos[3] = eof_song->beat[sp->beat]->pos + ((float)sp->beat_length / 6.0) * 3.0;
					sp->grid_pos[4] = eof_song->beat[sp->beat]->pos + ((float)sp->beat_length / 6.0) * 4.0;
					sp->grid_pos[5] = eof_song->beat[sp->beat]->pos + ((float)sp->beat_length / 6.0) * 5.0;
					sp->grid_pos[6] = eof_song->beat[sp->beat + 1]->pos;
					for(i = 0; i < 7; i++)
					{
						sp->grid_distance[i] = eof_pos_distance(sp->grid_pos[i], sp->pos);
					}
					for(i = 0; i < 7; i++)
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
					break;
				}
				case EOF_SNAP_THIRTY_SECOND:
				{
					sp->grid_pos[0] = eof_song->beat[sp->beat]->pos;
					sp->grid_pos[1] = eof_song->beat[sp->beat]->pos + ((float)sp->beat_length / 8.0);
					sp->grid_pos[2] = eof_song->beat[sp->beat]->pos + ((float)sp->beat_length / 8.0) * 2.0;
					sp->grid_pos[3] = eof_song->beat[sp->beat]->pos + ((float)sp->beat_length / 8.0) * 3.0;
					sp->grid_pos[4] = eof_song->beat[sp->beat]->pos + ((float)sp->beat_length / 8.0) * 4.0;
					sp->grid_pos[5] = eof_song->beat[sp->beat]->pos + ((float)sp->beat_length / 8.0) * 5.0;
					sp->grid_pos[6] = eof_song->beat[sp->beat]->pos + ((float)sp->beat_length / 8.0) * 6.0;
					sp->grid_pos[7] = eof_song->beat[sp->beat]->pos + ((float)sp->beat_length / 8.0) * 7.0;
					sp->grid_pos[8] = eof_song->beat[sp->beat + 1]->pos;
					for(i = 0; i < 9; i++)
					{
						sp->grid_distance[i] = eof_pos_distance(sp->grid_pos[i], sp->pos);
					}
					for(i = 0; i < 9; i++)
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
					break;
				}
				case EOF_SNAP_FORTY_EIGHTH:
				{
					sp->grid_pos[0] = eof_song->beat[sp->beat]->pos;
					sp->grid_pos[1] = eof_song->beat[sp->beat]->pos + ((float)sp->beat_length / 12.0);
					sp->grid_pos[2] = eof_song->beat[sp->beat]->pos + ((float)sp->beat_length / 12.0) * 2.0;
					sp->grid_pos[3] = eof_song->beat[sp->beat]->pos + ((float)sp->beat_length / 12.0) * 3.0;
					sp->grid_pos[4] = eof_song->beat[sp->beat]->pos + ((float)sp->beat_length / 12.0) * 4.0;
					sp->grid_pos[5] = eof_song->beat[sp->beat]->pos + ((float)sp->beat_length / 12.0) * 5.0;
					sp->grid_pos[6] = eof_song->beat[sp->beat]->pos + ((float)sp->beat_length / 12.0) * 6.0;
					sp->grid_pos[7] = eof_song->beat[sp->beat]->pos + ((float)sp->beat_length / 12.0) * 7.0;
					sp->grid_pos[8] = eof_song->beat[sp->beat]->pos + ((float)sp->beat_length / 12.0) * 8.0;
					sp->grid_pos[9] = eof_song->beat[sp->beat]->pos + ((float)sp->beat_length / 12.0) * 9.0;
					sp->grid_pos[10] = eof_song->beat[sp->beat]->pos + ((float)sp->beat_length / 12.0) * 10.0;
					sp->grid_pos[11] = eof_song->beat[sp->beat]->pos + ((float)sp->beat_length / 12.0) * 11.0;
					sp->grid_pos[12] = eof_song->beat[sp->beat + 1]->pos;
					for(i = 0; i < 13; i++)
					{
						sp->grid_distance[i] = eof_pos_distance(sp->grid_pos[i], sp->pos);
					}
					for(i = 0; i < 13; i++)
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
					break;
				}
				case EOF_SNAP_CUSTOM:
				{
					/* find the snap positions */
					for(i = 0; i < eof_snap_interval; i++)
					{
						sp->grid_pos[i] = eof_song->beat[sp->beat]->pos + (((float)sp->beat_length / (float)eof_snap_interval) * (float)i);
					}
					sp->grid_pos[eof_snap_interval] = eof_song->beat[sp->beat + 1]->pos;
					
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
					break;
				}
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
			if(sp->beat >= eof_song->beats - 2)
			{
				sp->beat_length = eof_song->beat[sp->beat - 1]->pos - eof_song->beat[sp->beat - 2]->pos;
			}
			else
			{
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
		}
	}
	else
	{
		sp->length = 100;
	}
}

void eof_read_editor_keys(void)
{
	int i = 0;
	EOF_NOTE * new_note = NULL;
	EOF_LYRIC * new_lyric = NULL;
	
	eof_read_controller(&eof_guitar);
	eof_read_controller(&eof_drums);
	
	/* seek to beginning */
	if(key[KEY_HOME])
	{
		if(KEY_EITHER_CTRL)
		{
			eof_menu_song_seek_first_note();
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
	
	if(key[KEY_R])
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
	if(key[KEY_E] && !KEY_EITHER_CTRL)
	{
		eof_menu_catalog_next();
		key[KEY_E] = 0;
	}
	
	if(key[KEY_G])
	{
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
		{
			if(KEY_EITHER_SHIFT)
			{
				if(eof_vocals_selected)
				{
					eof_selected_track = EOF_TRACK_DRUM;
					eof_vocals_selected = 0;
					eof_detect_difficulties(eof_song);
					eof_fix_window_title();
					eof_mix_find_claps();
					eof_mix_start_helper();
				}
				else
				{
					eof_track_selected_menu[eof_selected_track].flags = 0;
					eof_selected_track--;
					if(eof_selected_track < 0)
					{
						eof_vocals_selected = 1;
						eof_selected_track = 0;
//						eof_selected_track = 4;
					}
					eof_track_selected_menu[eof_selected_track].flags = D_SELECTED;
					eof_detect_difficulties(eof_song);
					eof_fix_window_title();
					eof_mix_find_claps();
					eof_mix_start_helper();
				}
			}
			else
			{
				if(eof_vocals_selected)
				{
					eof_selected_track = EOF_TRACK_GUITAR;
					eof_vocals_selected = 0;
					eof_detect_difficulties(eof_song);
					eof_fix_window_title();
					eof_mix_find_claps();
					eof_mix_start_helper();
				}
				else
				{
					eof_track_selected_menu[eof_selected_track].flags = 0;
					eof_selected_track++;
					if(eof_selected_track > 4)
					{
						eof_vocals_selected = 1;
						eof_selected_track = 4;
//						eof_selected_track = 0;
					}
					eof_track_selected_menu[eof_selected_track].flags = D_SELECTED;
					eof_detect_difficulties(eof_song);
					eof_fix_window_title();
					eof_mix_find_claps();
					eof_mix_start_helper();
				}
			}
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
						eof_vocal_track_fixup_lyrics(eof_song->vocal_track, 1);
					}
					else
					{
						eof_track_fixup_notes(eof_song->track[eof_selected_track], 1);
					}
				}
				else
				{
					eof_entered_note = 0;
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
				int b = eof_get_beat(eof_music_pos - eof_av_delay);
				
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
				int b = eof_get_beat(eof_music_pos - eof_av_delay);
				if(eof_music_pos - eof_av_delay < 0)
				{
					b = -1;
				}
				
				if(b < eof_song->beats - 1 && eof_song->beat[b + 1]->pos < eof_music_actual_length)
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
		if(eof_vocals_selected && eof_vocals_offset < 84 - eof_screen_layout.vocal_view_size + 1)
		{
			if(KEY_EITHER_CTRL)
			{
				eof_vocals_offset += 12;
				if(eof_vocals_offset > 84)
				{
					eof_vocals_offset = 84;
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
			for(i = 0; i < 12; i++)
			{
				eof_menu_note_transpose_up();
			}
		}
		else
		{
			eof_menu_note_transpose_up();
		}
		if(eof_vocals_selected && eof_mix_vocal_tones_enabled && eof_selection.current < eof_song->vocal_track->lyrics)
		{
			eof_mix_play_note(eof_song->vocal_track->lyric[eof_selection.current]->note);
		}
		key[KEY_UP] = 0;
	}
	if(KEY_EITHER_SHIFT && key[KEY_DOWN])
	{
		if(eof_vocals_selected && eof_vocals_offset > 36)
		{
			if(KEY_EITHER_CTRL)
			{
				eof_vocals_offset -= 12;
				if(eof_vocals_offset < 36)
				{
					eof_vocals_offset = 36;
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
			for(i = 0; i < 12; i++)
			{
				eof_menu_note_transpose_down();
			}
		}
		else
		{
			eof_menu_note_transpose_down();
		}
		if(eof_vocals_selected && eof_mix_vocal_tones_enabled && eof_selection.current < eof_song->vocal_track->lyrics)
		{
			eof_mix_play_note(eof_song->vocal_track->lyric[eof_selection.current]->note);
		}
		key[KEY_DOWN] = 0;
	}
	if(key[KEY_A] && eof_music_paused)
	{
		if(KEY_EITHER_CTRL)
		{
			eof_menu_beat_anchor();
		}
		else
		{
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
					for(i = 0; i < eof_song->vocal_track->lyrics; i++)
					{
						if(eof_selection.track == EOF_TRACK_VOCALS && eof_selection.multi[i])
						{
							eof_song->vocal_track->lyric[i]->length -= 10;
						}
					}
				}
				else
				{
					for(i = 0; i < eof_song->vocal_track->lyrics; i++)
					{
						if(eof_selection.track == EOF_TRACK_VOCALS && eof_selection.multi[i])
						{
							eof_song->vocal_track->lyric[i]->length -= 100;
						}
					}
				}
			}
			else
			{
				int b;
				for(i = 0; i < eof_song->vocal_track->lyrics; i++)
				{
					if(eof_selection.track == EOF_TRACK_VOCALS && eof_selection.multi[i])
					{
						b = eof_get_beat(eof_song->vocal_track->lyric[i]->pos + eof_song->vocal_track->lyric[i]->length - 1);
						if(b >= 0)
						{
							eof_snap_logic(&eof_tail_snap, eof_song->beat[b]->pos);
						}
						else
						{
							eof_snap_logic(&eof_tail_snap, eof_song->vocal_track->lyric[i]->pos + eof_song->vocal_track->lyric[i]->length - 1);
						}
						eof_snap_length_logic(&eof_tail_snap);
						eof_song->vocal_track->lyric[i]->length -= eof_tail_snap.length;
						if(eof_song->vocal_track->lyric[i]->length > 1)
						{
							eof_snap_logic(&eof_tail_snap, eof_song->vocal_track->lyric[i]->pos + eof_song->vocal_track->lyric[i]->length);
							eof_set_tail_pos(i, eof_tail_snap.pos);
						}
					}
				}
			}
			eof_vocal_track_fixup_lyrics(eof_song->vocal_track, 1);
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
					for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
					{
						if(eof_selection.track == eof_selected_track && eof_selection.multi[i] && eof_song->track[eof_selected_track]->note[i]->type == eof_note_type)
						{
							eof_song->track[eof_selected_track]->note[i]->length -= 10;
						}
					}
				}
				else
				{
					for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
					{
						if(eof_selection.track == eof_selected_track && eof_selection.multi[i] && eof_song->track[eof_selected_track]->note[i]->type == eof_note_type)
						{
							eof_song->track[eof_selected_track]->note[i]->length -= 100;
						}
					}
				}
			}
			else
			{
				int b;
				for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
				{
					if(eof_selection.track == eof_selected_track && eof_selection.multi[i] && eof_song->track[eof_selected_track]->note[i]->type == eof_note_type)
					{
						b = eof_get_beat(eof_song->track[eof_selected_track]->note[i]->pos + eof_song->track[eof_selected_track]->note[i]->length - 1);
						if(b >= 0)
						{
							eof_snap_logic(&eof_tail_snap, eof_song->beat[b]->pos);
						}
						else
						{
							eof_snap_logic(&eof_tail_snap, eof_song->track[eof_selected_track]->note[i]->pos + eof_song->track[eof_selected_track]->note[i]->length - 1);
						}
						eof_snap_length_logic(&eof_tail_snap);
						eof_song->track[eof_selected_track]->note[i]->length -= eof_tail_snap.length;
						if(eof_song->track[eof_selected_track]->note[i]->length > 1)
						{
							eof_snap_logic(&eof_tail_snap, eof_song->track[eof_selected_track]->note[i]->pos + eof_song->track[eof_selected_track]->note[i]->length);
							eof_set_tail_pos(i, eof_tail_snap.pos);
						}
					}
				}
			}
			eof_track_fixup_notes(eof_song->track[eof_selected_track], 1);
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
					for(i = 0; i < eof_song->vocal_track->lyrics; i++)
					{
						if(eof_selection.track == EOF_TRACK_VOCALS && eof_selection.multi[i])
						{
							eof_song->vocal_track->lyric[i]->length += 10;
						}
					}
				}
				else
				{
					for(i = 0; i < eof_song->vocal_track->lyrics; i++)
					{
						if(eof_selection.track == EOF_TRACK_VOCALS && eof_selection.multi[i])
						{
							eof_song->vocal_track->lyric[i]->length += 100;
						}
					}
				}
			}
			else
			{
				for(i = 0; i < eof_song->vocal_track->lyrics; i++)
				{
					if(eof_selection.track == EOF_TRACK_VOCALS && eof_selection.multi[i])
					{
						eof_snap_logic(&eof_tail_snap, eof_song->vocal_track->lyric[i]->pos + eof_song->vocal_track->lyric[i]->length + 1);
						if(eof_tail_snap.pos > eof_song->vocal_track->lyric[i]->pos + eof_song->vocal_track->lyric[i]->length + 1)
						{
							eof_set_tail_pos(i, eof_tail_snap.pos);
						}
						else
						{
							eof_snap_length_logic(&eof_tail_snap);
							eof_song->vocal_track->lyric[i]->length += eof_tail_snap.length;
							eof_snap_logic(&eof_tail_snap, eof_song->vocal_track->lyric[i]->pos + eof_song->vocal_track->lyric[i]->length);
							eof_set_tail_pos(i, eof_tail_snap.pos);
						}
					}
				}
			}
			eof_vocal_track_fixup_lyrics(eof_song->vocal_track, 1);
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
					for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
					{
						if(eof_selection.track == eof_selected_track && eof_selection.multi[i] && eof_song->track[eof_selected_track]->note[i]->type == eof_note_type)
						{
							eof_song->track[eof_selected_track]->note[i]->length += 10;
						}
					}
				}
				else
				{
					for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
					{
						if(eof_selection.track == eof_selected_track && eof_selection.multi[i] && eof_song->track[eof_selected_track]->note[i]->type == eof_note_type)
						{
							eof_song->track[eof_selected_track]->note[i]->length += 100;
						}
					}
				}
			}
			else
			{
				for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
				{
					if(eof_selection.track == eof_selected_track && eof_selection.multi[i] && eof_song->track[eof_selected_track]->note[i]->type == eof_note_type)
					{
						eof_snap_logic(&eof_tail_snap, eof_song->track[eof_selected_track]->note[i]->pos + eof_song->track[eof_selected_track]->note[i]->length + 1);
						if(eof_tail_snap.pos > eof_song->track[eof_selected_track]->note[i]->pos + eof_song->track[eof_selected_track]->note[i]->length + 1)
						{
							eof_set_tail_pos(i, eof_tail_snap.pos);
						}
						else
						{
							eof_snap_length_logic(&eof_tail_snap);
							eof_song->track[eof_selected_track]->note[i]->length += eof_tail_snap.length;
							eof_snap_logic(&eof_tail_snap, eof_song->track[eof_selected_track]->note[i]->pos + eof_song->track[eof_selected_track]->note[i]->length);
							eof_set_tail_pos(i, eof_tail_snap.pos);
						}
					}
				}
			}
			eof_track_fixup_notes(eof_song->track[eof_selected_track], 1);
		}
		key[KEY_CLOSEBRACE] = 0;
	}
	
	if(key[KEY_T])
	{
		eof_menu_note_toggle_crazy();
		key[KEY_T] = 0;
	}
	
	/* select all */
	if(KEY_EITHER_CTRL && key[KEY_E] && eof_music_paused && !eof_music_catalog_playback)
	{
		eof_menu_edit_select_all();
		key[KEY_E] = 0;
	}
	
	/* select like */
	if(key[KEY_L])
	{
		if(KEY_EITHER_CTRL && eof_music_paused && !eof_music_catalog_playback)
		{
			eof_menu_edit_select_like();
		}
		else if(eof_vocals_selected && eof_selection.track == EOF_TRACK_VOCALS && eof_selection.current < eof_song->vocal_track->lyrics)
		{
			if(KEY_EITHER_SHIFT)
			{
				eof_menu_split_lyric();
			}
			else
			{
				eof_edit_lyric_dialog();
			}
		}
		key[KEY_L] = 0;
	}
	
	/* select all */
	if(KEY_EITHER_CTRL && key[KEY_D] && eof_music_paused && !eof_music_catalog_playback)
	{
		eof_menu_edit_deselect_all();
		key[KEY_D] = 0;
	}
	
	if(key[KEY_M])
	{
		eof_menu_edit_metronome();
		key[KEY_M] = 0;
	}
	if(key[KEY_K])
	{
		eof_menu_edit_claps();
		key[KEY_K] = 0;
	}
	if(key[KEY_V] && !KEY_EITHER_CTRL)
	{
		eof_menu_edit_vocal_tones();
		key[KEY_V] = 0;
	}
	if(key[KEY_1] && KEY_EITHER_CTRL && eof_music_paused && !eof_music_catalog_playback && !eof_vocals_selected)
	{
		eof_menu_note_toggle_green();
		key[KEY_1] = 0;
	}
	else if(key[KEY_2] && KEY_EITHER_CTRL && eof_music_paused && !eof_music_catalog_playback && !eof_vocals_selected)
	{
		eof_menu_note_toggle_red();
		key[KEY_2] = 0;
	}
	else if(key[KEY_3] && KEY_EITHER_CTRL && eof_music_paused && !eof_music_catalog_playback && !eof_vocals_selected)
	{
		eof_menu_note_toggle_yellow();
		key[KEY_3] = 0;
	}
	else if(key[KEY_4] && KEY_EITHER_CTRL && eof_music_paused && !eof_music_catalog_playback && !eof_vocals_selected)
	{
		eof_menu_note_toggle_blue();
		key[KEY_4] = 0;
	}
	else if(key[KEY_5] && KEY_EITHER_CTRL && eof_music_paused && !eof_music_catalog_playback && !eof_vocals_selected)
	{
		eof_menu_note_toggle_purple();
		key[KEY_5] = 0;
	}
	else if(key[KEY_1] && KEY_EITHER_SHIFT)
	{
		eof_vocals_offset = 36;
		key[KEY_1] = 0;
	}
	else if(key[KEY_2] && KEY_EITHER_SHIFT)
	{
		eof_vocals_offset = 48;
		key[KEY_2] = 0;
	}
	else if(key[KEY_3] && KEY_EITHER_SHIFT)
	{
		eof_vocals_offset = 60;
		key[KEY_3] = 0;
	}
	else if(key[KEY_4] && KEY_EITHER_SHIFT)
	{
		eof_vocals_offset = 72;
		key[KEY_4] = 0;
	}
	else if(!KEY_EITHER_CTRL)
	{
		if(eof_input_mode == EOF_INPUT_HOLD)
		{
			if(!KEY_EITHER_SHIFT && !eof_vocals_selected)
			{
				if(key[KEY_1])
				{
					if(!(eof_pen_note.note & 1))
					{
						eof_pen_note.note ^= 1;
					}
				}
				else
				{
					if((eof_pen_note.note & 1))
					{
						eof_pen_note.note ^= 1;
					}
				}
				if(key[KEY_2])
				{
					if(!(eof_pen_note.note & 2))
					{
						eof_pen_note.note ^= 2;
					}
				}
				else
				{
					if((eof_pen_note.note & 2))
					{
						eof_pen_note.note ^= 2;
					}
				}
				if(key[KEY_3])
				{
					if(!(eof_pen_note.note & 4))
					{
						eof_pen_note.note ^= 4;
					}
				}
				else
				{
					if((eof_pen_note.note & 4))
					{
						eof_pen_note.note ^= 4;
					}
				}
				if(key[KEY_4])
				{
					if(!(eof_pen_note.note & 8))
					{
						eof_pen_note.note ^= 8;
					}
				}
				else
				{
					if((eof_pen_note.note & 8))
					{
						eof_pen_note.note ^= 8;
					}
				}
				if(key[KEY_5])
				{
					if(!(eof_pen_note.note & 16))
					{
						eof_pen_note.note ^= 16;
					}
				}
				else
				{
					if((eof_pen_note.note & 16))
					{
						eof_pen_note.note ^= 16;
					}
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
			}
		}
		else if(eof_input_mode == EOF_INPUT_REX && eof_music_paused && !eof_music_catalog_playback)
		{
			eof_hover_piece = -1;
			if(mouse_y >= eof_window_editor->y + 25 + EOF_EDITOR_RENDER_OFFSET && mouse_y < eof_window_editor->y + eof_screen_layout.fretboard_h + EOF_EDITOR_RENDER_OFFSET)
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
					if(key[KEY_1])
					{
						eof_selection.range_pos_1 = 0;
						eof_selection.range_pos_2 = 0;
						eof_prepare_undo(EOF_UNDO_TYPE_NOTE_SEL);
						if(eof_hover_note >= 0)
						{
							eof_song->track[eof_selected_track]->note[eof_hover_note]->note ^= 1;
							eof_selection.current = eof_hover_note;
							if(!eof_song->track[eof_selected_track]->note[eof_hover_note]->note)
							{
								eof_track_delete_note(eof_song->track[eof_selected_track], eof_hover_note);
								eof_selection.multi[eof_selection.current] = 0;
								eof_selection.current = EOF_MAX_NOTES - 1;
								eof_track_sort_notes(eof_song->track[eof_selected_track]);
								eof_track_fixup_notes(eof_song->track[eof_selected_track], 1);
								eof_determine_hopos();
								eof_detect_difficulties(eof_song);
							}
							if(eof_selection.current != EOF_MAX_NOTES - 1)
							{
								memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
								eof_selection.track = eof_selected_track;
								eof_selection.multi[eof_selection.current] = 1;
								eof_selection.current_pos = eof_song->track[eof_selected_track]->note[eof_selection.current]->pos;
								eof_selection.range_pos_1 = eof_selection.current_pos;
								eof_selection.range_pos_2 = eof_selection.current_pos;
							}
						}
						else
						{
							eof_pen_note.note ^= 1;
							new_note = eof_track_add_note(eof_song->track[eof_selected_track]);
							if(new_note)
							{
								eof_note_create(new_note, eof_pen_note.note & 1, eof_pen_note.note & 2, eof_pen_note.note & 4, eof_pen_note.note & 8, eof_pen_note.note & 16, eof_pen_note.pos, eof_snap.length);
								new_note->type = eof_note_type;
								eof_selection.current_pos = new_note->pos;
								eof_selection.range_pos_1 = eof_selection.current_pos;
								eof_selection.range_pos_2 = eof_selection.current_pos;
								eof_selection.track = eof_selected_track;
								memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
								eof_track_sort_notes(eof_song->track[eof_selected_track]);
								eof_track_fixup_notes(eof_song->track[eof_selected_track], 0);
								eof_determine_hopos();
								eof_selection.multi[eof_selection.current] = 1;
								eof_detect_difficulties(eof_song);
							}
						}
						key[KEY_1] = 0;
					}
					else if(key[KEY_2])
					{
						eof_selection.range_pos_1 = 0;
						eof_selection.range_pos_2 = 0;
						eof_prepare_undo(EOF_UNDO_TYPE_NOTE_SEL);
						if(eof_hover_note >= 0)
						{
							eof_song->track[eof_selected_track]->note[eof_hover_note]->note ^= 2;
							eof_selection.current = eof_hover_note;
							if(!eof_song->track[eof_selected_track]->note[eof_hover_note]->note)
							{
								eof_track_delete_note(eof_song->track[eof_selected_track], eof_hover_note);
								eof_selection.multi[eof_selection.current] = 0;
								eof_selection.current = EOF_MAX_NOTES - 1;
								eof_track_sort_notes(eof_song->track[eof_selected_track]);
								eof_track_fixup_notes(eof_song->track[eof_selected_track], 1);
								eof_determine_hopos();
								eof_detect_difficulties(eof_song);
							}
							if(eof_selection.current != EOF_MAX_NOTES - 1)
							{
								memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
								eof_selection.track = eof_selected_track;
								eof_selection.multi[eof_selection.current] = 1;
								eof_selection.current_pos = eof_song->track[eof_selected_track]->note[eof_selection.current]->pos;
								eof_selection.range_pos_1 = eof_selection.current_pos;
								eof_selection.range_pos_2 = eof_selection.current_pos;
							}
						}
						else
						{
							eof_pen_note.note ^= 2;
							new_note = eof_track_add_note(eof_song->track[eof_selected_track]);
							if(new_note)
							{
								eof_note_create(new_note, eof_pen_note.note & 1, eof_pen_note.note & 2, eof_pen_note.note & 4, eof_pen_note.note & 8, eof_pen_note.note & 16, eof_pen_note.pos, eof_snap.length);
								new_note->type = eof_note_type;
								eof_selection.current_pos = new_note->pos;
								eof_selection.range_pos_1 = eof_selection.current_pos;
								eof_selection.range_pos_2 = eof_selection.current_pos;
								eof_selection.track = eof_selected_track;
								memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
								eof_track_sort_notes(eof_song->track[eof_selected_track]);
								eof_track_fixup_notes(eof_song->track[eof_selected_track], 0);
								eof_determine_hopos();
								eof_selection.multi[eof_selection.current] = 1;
								eof_detect_difficulties(eof_song);
							}
						}
						key[KEY_2] = 0;
					}
					else if(key[KEY_3])
					{
						eof_selection.range_pos_1 = 0;
						eof_selection.range_pos_2 = 0;
						eof_prepare_undo(EOF_UNDO_TYPE_NOTE_SEL);
						if(eof_hover_note >= 0)
						{
							eof_song->track[eof_selected_track]->note[eof_hover_note]->note ^= 4;
							eof_selection.current = eof_hover_note;
							if(!eof_song->track[eof_selected_track]->note[eof_hover_note]->note)
							{
								eof_track_delete_note(eof_song->track[eof_selected_track], eof_hover_note);
								eof_selection.multi[eof_selection.current] = 0;
								eof_selection.current = EOF_MAX_NOTES - 1;
								eof_track_sort_notes(eof_song->track[eof_selected_track]);
								eof_track_fixup_notes(eof_song->track[eof_selected_track], 1);
								eof_determine_hopos();
								eof_detect_difficulties(eof_song);
							}
							if(eof_selection.current != EOF_MAX_NOTES - 1)
							{
								memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
								eof_selection.track = eof_selected_track;
								eof_selection.multi[eof_selection.current] = 1;
								eof_selection.current_pos = eof_song->track[eof_selected_track]->note[eof_selection.current]->pos;
								eof_selection.range_pos_1 = eof_selection.current_pos;
								eof_selection.range_pos_2 = eof_selection.current_pos;
							}
						}
						else
						{
							eof_pen_note.note ^= 4;
							new_note = eof_track_add_note(eof_song->track[eof_selected_track]);
							if(new_note)
							{
								eof_note_create(new_note, eof_pen_note.note & 1, eof_pen_note.note & 2, eof_pen_note.note & 4, eof_pen_note.note & 8, eof_pen_note.note & 16, eof_pen_note.pos, eof_snap.length);
								new_note->type = eof_note_type;
								eof_selection.current_pos = new_note->pos;
								eof_selection.range_pos_1 = eof_selection.current_pos;
								eof_selection.range_pos_2 = eof_selection.current_pos;
								eof_selection.track = eof_selected_track;
								memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
								eof_track_sort_notes(eof_song->track[eof_selected_track]);
								eof_track_fixup_notes(eof_song->track[eof_selected_track], 0);
								eof_determine_hopos();
								eof_selection.multi[eof_selection.current] = 1;
								eof_detect_difficulties(eof_song);
							}
						}
						key[KEY_3] = 0;
					}
					else if(key[KEY_4])
					{
						eof_selection.range_pos_1 = 0;
						eof_selection.range_pos_2 = 0;
						eof_prepare_undo(EOF_UNDO_TYPE_NOTE_SEL);
						if(eof_hover_note >= 0)
						{
							eof_song->track[eof_selected_track]->note[eof_hover_note]->note ^= 8;
							eof_selection.current = eof_hover_note;
							if(!eof_song->track[eof_selected_track]->note[eof_hover_note]->note)
							{
								eof_track_delete_note(eof_song->track[eof_selected_track], eof_hover_note);
								eof_selection.multi[eof_selection.current] = 0;
								eof_selection.current = EOF_MAX_NOTES - 1;
								eof_track_sort_notes(eof_song->track[eof_selected_track]);
								eof_track_fixup_notes(eof_song->track[eof_selected_track], 1);
								eof_determine_hopos();
								eof_detect_difficulties(eof_song);
							}
							if(eof_selection.current != EOF_MAX_NOTES - 1)
							{
								memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
								eof_selection.track = eof_selected_track;
								eof_selection.multi[eof_selection.current] = 1;
								eof_selection.current_pos = eof_song->track[eof_selected_track]->note[eof_selection.current]->pos;
								eof_selection.range_pos_1 = eof_selection.current_pos;
								eof_selection.range_pos_2 = eof_selection.current_pos;
							}
						}
						else
						{
							eof_pen_note.note ^= 8;
							new_note = eof_track_add_note(eof_song->track[eof_selected_track]);
							if(new_note)
							{
								eof_note_create(new_note, eof_pen_note.note & 1, eof_pen_note.note & 2, eof_pen_note.note & 4, eof_pen_note.note & 8, eof_pen_note.note & 16, eof_pen_note.pos, eof_snap.length);
								new_note->type = eof_note_type;
								eof_selection.current_pos = new_note->pos;
								eof_selection.range_pos_1 = eof_selection.current_pos;
								eof_selection.range_pos_2 = eof_selection.current_pos;
								eof_selection.track = eof_selected_track;
								memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
								eof_track_sort_notes(eof_song->track[eof_selected_track]);
								eof_track_fixup_notes(eof_song->track[eof_selected_track], 0);
								eof_determine_hopos();
								eof_selection.multi[eof_selection.current] = 1;
								eof_detect_difficulties(eof_song);
							}
						}
						key[KEY_4] = 0;
					}
					else if(key[KEY_5])
					{
						eof_selection.range_pos_1 = 0;
						eof_selection.range_pos_2 = 0;
						eof_prepare_undo(EOF_UNDO_TYPE_NOTE_SEL);
						if(eof_hover_note >= 0)
						{
							eof_song->track[eof_selected_track]->note[eof_hover_note]->note ^= 16;
							eof_selection.current = eof_hover_note;
							if(!eof_song->track[eof_selected_track]->note[eof_hover_note]->note)
							{
								eof_track_delete_note(eof_song->track[eof_selected_track], eof_hover_note);
								eof_selection.multi[eof_selection.current] = 0;
								eof_selection.current = EOF_MAX_NOTES - 1;
								eof_track_sort_notes(eof_song->track[eof_selected_track]);
								eof_track_fixup_notes(eof_song->track[eof_selected_track], 1);
								eof_determine_hopos();
								eof_detect_difficulties(eof_song);
							}
							if(eof_selection.current != EOF_MAX_NOTES - 1)
							{
								memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
								eof_selection.track = eof_selected_track;
								eof_selection.multi[eof_selection.current] = 1;
								eof_selection.current_pos = eof_song->track[eof_selected_track]->note[eof_selection.current]->pos;
								eof_selection.range_pos_1 = eof_selection.current_pos;
								eof_selection.range_pos_2 = eof_selection.current_pos;
							}
						}
						else
						{
							eof_pen_note.note ^= 16;
							new_note = eof_track_add_note(eof_song->track[eof_selected_track]);
							if(new_note)
							{
								eof_note_create(new_note, eof_pen_note.note & 1, eof_pen_note.note & 2, eof_pen_note.note & 4, eof_pen_note.note & 8, eof_pen_note.note & 16, eof_pen_note.pos, eof_snap.length);
								new_note->type = eof_note_type;
								eof_selection.current_pos = new_note->pos;
								eof_selection.range_pos_1 = eof_selection.current_pos;
								eof_selection.range_pos_2 = eof_selection.current_pos;
								eof_selection.track = eof_selected_track;
								memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
								eof_track_sort_notes(eof_song->track[eof_selected_track]);
								eof_track_fixup_notes(eof_song->track[eof_selected_track], 0);
								eof_determine_hopos();
								eof_selection.multi[eof_selection.current] = 1;
								eof_detect_difficulties(eof_song);
							}
						}
						key[KEY_5] = 0;
					}
				}
			}
		}
		else if(eof_input_mode == EOF_INPUT_GUITAR_TAP && !eof_music_paused && eof_selected_track != EOF_TRACK_DRUM)
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
			if(eof_held_1 == 1)
			{
				if(eof_vocals_selected)
				{
					if(!eof_entered_note)
					{
						eof_prepare_undo(0);
						eof_entered_note = 1;
					}
					new_lyric = eof_vocal_track_add_lyric(eof_song->vocal_track);
					if(new_lyric)
					{
						new_lyric->note = eof_vocals_offset;
						new_lyric->pos = eof_music_pos - eof_av_delay;
						new_lyric->length = 1;
						strcpy(new_lyric->text, "");
						eof_detect_difficulties(eof_song);
						eof_vocal_track_sort_lyrics(eof_song->vocal_track);
					}
				}
				else
				{
					if(!eof_entered_note)
					{
						eof_prepare_undo(0);
						eof_entered_note = 1;
					}
					new_note = eof_track_add_note(eof_song->track[eof_selected_track]);
					if(new_note)
					{
						eof_note_create(new_note, 1, 0, 0, 0, 0, eof_music_pos - eof_av_delay, 1);
						new_note->type = eof_note_type;
						eof_entering_note_note = new_note;
						eof_entering_note = 1;
						eof_detect_difficulties(eof_song);
						eof_track_sort_notes(eof_song->track[eof_selected_track]);
					}
				}
			}
			else if(eof_held_2 == 1)
			{
				if(eof_vocals_selected)
				{
					if(!eof_entered_note)
					{
						eof_prepare_undo(0);
						eof_entered_note = 1;
					}
					new_lyric = eof_vocal_track_add_lyric(eof_song->vocal_track);
					if(new_lyric)
					{
						new_lyric->note = eof_vocals_offset;
						new_lyric->pos = eof_music_pos - eof_av_delay;
						new_lyric->length = 1;
						strcpy(new_lyric->text, "");
						eof_detect_difficulties(eof_song);
						eof_vocal_track_sort_lyrics(eof_song->vocal_track);
					}
				}
				else
				{
					if(!eof_entered_note)
					{
						eof_prepare_undo(0);
						eof_entered_note = 1;
					}
					new_note = eof_track_add_note(eof_song->track[eof_selected_track]);
					if(new_note)
					{
						eof_note_create(new_note, 0, 1, 0, 0, 0, eof_music_pos - eof_av_delay, 1);
						new_note->type = eof_note_type;
						eof_entering_note_note = new_note;
						eof_entering_note = 1;
						eof_detect_difficulties(eof_song);
						eof_track_sort_notes(eof_song->track[eof_selected_track]);
					}
				}
			}
			else if(eof_held_3 == 1)
			{
				if(eof_vocals_selected)
				{
					if(!eof_entered_note)
					{
						eof_prepare_undo(0);
						eof_entered_note = 1;
					}
					new_lyric = eof_vocal_track_add_lyric(eof_song->vocal_track);
					if(new_lyric)
					{
						new_lyric->note = eof_vocals_offset;
						new_lyric->pos = eof_music_pos - eof_av_delay;
						new_lyric->length = 1;
						strcpy(new_lyric->text, "");
						eof_detect_difficulties(eof_song);
						eof_vocal_track_sort_lyrics(eof_song->vocal_track);
					}
				}
				else
				{
					if(!eof_entered_note)
					{
						eof_prepare_undo(0);
						eof_entered_note = 1;
					}
					new_note = eof_track_add_note(eof_song->track[eof_selected_track]);
					if(new_note)
					{
						eof_note_create(new_note, 0, 0, 1, 0, 0, eof_music_pos - eof_av_delay, 1);
						new_note->type = eof_note_type;
						eof_entering_note_note = new_note;
						eof_entering_note = 1;
						eof_detect_difficulties(eof_song);
						eof_track_sort_notes(eof_song->track[eof_selected_track]);
					}
				}
			}
			else if(eof_held_4 == 1)
			{
				if(eof_vocals_selected)
				{
					if(!eof_entered_note)
					{
						eof_prepare_undo(0);
						eof_entered_note = 1;
					}
					new_lyric = eof_vocal_track_add_lyric(eof_song->vocal_track);
					if(new_lyric)
					{
						new_lyric->note = eof_vocals_offset;
						new_lyric->pos = eof_music_pos - eof_av_delay;
						new_lyric->length = 1;
						strcpy(new_lyric->text, "");
						eof_detect_difficulties(eof_song);
						eof_vocal_track_sort_lyrics(eof_song->vocal_track);
					}
				}
				else
				{
					if(!eof_entered_note)
					{
						eof_prepare_undo(0);
						eof_entered_note = 1;
					}
					new_note = eof_track_add_note(eof_song->track[eof_selected_track]);
					if(new_note)
					{
						eof_note_create(new_note, 0, 0, 0, 1, 0, eof_music_pos - eof_av_delay, 1);
						new_note->type = eof_note_type;
						eof_entering_note_note = new_note;
						eof_entering_note = 1;
						eof_detect_difficulties(eof_song);
						eof_track_sort_notes(eof_song->track[eof_selected_track]);
					}
				}
			}
			else if(eof_held_5 == 1)
			{
				if(eof_vocals_selected)
				{
					if(!eof_entered_note)
					{
						eof_prepare_undo(0);
						eof_entered_note = 1;
					}
					new_lyric = eof_vocal_track_add_lyric(eof_song->vocal_track);
					if(new_lyric)
					{
						new_lyric->note = eof_vocals_offset;
						new_lyric->pos = eof_music_pos - eof_av_delay;
						new_lyric->length = 1;
						strcpy(new_lyric->text, "");
						eof_detect_difficulties(eof_song);
						eof_vocal_track_sort_lyrics(eof_song->vocal_track);
					}
				}
				else
				{
					if(!eof_entered_note)
					{
						eof_prepare_undo(0);
						eof_entered_note = 1;
					}
					new_note = eof_track_add_note(eof_song->track[eof_selected_track]);
					if(new_note)
					{
						eof_note_create(new_note, 0, 0, 0, 0, 1, eof_music_pos - eof_av_delay, 1);
						new_note->type = eof_note_type;
						eof_entering_note_note = new_note;
						eof_entering_note = 1;
						eof_detect_difficulties(eof_song);
						eof_track_sort_notes(eof_song->track[eof_selected_track]);
					}
				}
			}
		}
		else if(eof_input_mode == EOF_INPUT_GUITAR_STRUM && !eof_music_paused && eof_selected_track != EOF_TRACK_DRUM)
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
						eof_entering_note_lyric->length = (eof_music_pos - eof_av_delay) - eof_entering_note_lyric->pos - 10;
					}
					if(!eof_entered_note)
					{
						eof_prepare_undo(0);
						eof_entered_note = 1;
					}
					new_lyric = eof_vocal_track_add_lyric(eof_song->vocal_track);
					if(new_lyric)
					{
						new_lyric->pos = eof_music_pos - eof_av_delay;
						new_lyric->note = eof_vocals_offset;
						eof_entering_note_lyric = new_lyric;
						eof_entering_note = 1;
						eof_detect_difficulties(eof_song);
						eof_vocal_track_sort_lyrics(eof_song->vocal_track);
					}
				}
				if(eof_entering_note)
				{
					if(eof_snote != eof_last_snote)
					{
						eof_entering_note = 0;
						eof_entering_note_lyric->length = (eof_music_pos - eof_av_delay) - eof_entering_note_lyric->pos - 10;
						eof_track_fixup_notes(eof_song->track[eof_selected_track], 1);
					}
					else
					{
						eof_entering_note_lyric->length = (eof_music_pos - eof_av_delay) - eof_entering_note_lyric->pos - 10;
						eof_vocal_track_fixup_lyrics(eof_song->vocal_track, 1);
					}
				}
			}
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
				if((eof_guitar.button[0].pressed || eof_guitar.button[1].pressed) && eof_snote)
				{
					if(eof_entering_note && eof_entering_note_note)
					{
						eof_entering_note_note->length = (eof_music_pos - eof_av_delay) - eof_entering_note_note->pos - 10;
					}
					if(!eof_entered_note)
					{
						eof_prepare_undo(0);
						eof_entered_note = 1;
					}
					new_note = eof_track_add_note(eof_song->track[eof_selected_track]);
					if(new_note)
					{
						eof_note_create(new_note, 1, 0, 0, 0, 0, eof_music_pos - eof_av_delay, 1);
						new_note->note = eof_snote;
						new_note->type = eof_note_type;
						eof_entering_note_note = new_note;
						eof_entering_note = 1;
						eof_detect_difficulties(eof_song);
						eof_track_sort_notes(eof_song->track[eof_selected_track]);
					}
				}
				if(eof_entering_note)
				{
					if(eof_snote != eof_last_snote)
					{
						eof_entering_note = 0;
						eof_entering_note_note->length = (eof_music_pos - eof_av_delay) - eof_entering_note_note->pos - 10;
						eof_track_fixup_notes(eof_song->track[eof_selected_track], 1);
					}
					else
					{
						eof_entering_note_note->length = (eof_music_pos - eof_av_delay) - eof_entering_note_note->pos - 10;
						eof_track_fixup_notes(eof_song->track[eof_selected_track], 1);
					}
				}
			}
		}
	}
	if(eof_input_mode == EOF_INPUT_CLASSIC && eof_input_mode == EOF_INPUT_HOLD)
	{
		if(key[KEY_ENTER] && eof_song_loaded)
		{
		
			/* place note with default length if song is paused */
			if(eof_music_paused)
			{
				new_note = eof_track_add_note(eof_song->track[eof_selected_track]);
				if(new_note)
				{
					eof_note_create(new_note, eof_pen_note.note & 1, eof_pen_note.note & 2, eof_pen_note.note & 4, eof_pen_note.note & 8, eof_pen_note.note & 16, eof_music_pos - eof_av_delay, eof_snap.length);
					eof_song->track[eof_selected_track]->note[i]->type = eof_note_type;
					eof_detect_difficulties(eof_song);
				}
				key[KEY_ENTER] = 0;
			}
		
			/* otherwise allow length to be determined by key holding */
			else
			{
				if(!eof_entering_note_note)
				{
					new_note = eof_track_add_note(eof_song->track[eof_selected_track]);
					if(new_note)
					{
						eof_note_create(new_note, eof_pen_note.note & 1, eof_pen_note.note & 2, eof_pen_note.note & 4, eof_pen_note.note & 8, eof_pen_note.note & 16, eof_music_pos - eof_av_delay, eof_snap.length);
						eof_song->track[eof_selected_track]->note[i]->type = eof_note_type;
						eof_entering_note_note = new_note;
						eof_entering_note = 1;
						eof_detect_difficulties(eof_song);
					}
				}
				else
				{
					eof_entering_note_note->length = (eof_music_pos - eof_av_delay) - eof_entering_note_note->pos - 10;
//					eof_song->track[eof_selected_track]->note[eof_entering_note_note]->length = (eof_music_pos - eof_av_delay) - eof_song->track[eof_selected_track]->note[eof_entering_note_note]->pos;
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
			if(KEY_EITHER_CTRL && key[KEY_B])
			{
				eof_menu_edit_old_paste();
				key[KEY_B] = 0;
			}
		}
	}
}

void eof_editor_drum_logic(void)
{
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
	if(eof_entering_note_note && eof_music_pos - eof_av_delay >= eof_entering_note_note->pos + 50)
	{
		eof_entering_note_note = NULL;
	}
	if(eof_held_1 == 1)
	{
		if(eof_entering_note_note && eof_music_pos - eof_av_delay < eof_entering_note_note->pos + 50)
		{
			eof_entering_note_note->note |= 1;
		}
		else
		{
			new_note = eof_track_add_note(eof_song->track[eof_selected_track]);
			if(new_note)
			{
				eof_note_create(new_note, 1, 0, 0, 0, 0, eof_music_pos - eof_av_delay, 1);
				eof_snap_logic(&eof_snap, new_note->pos);
				new_note->pos = eof_snap.pos;
				new_note->type = eof_note_type;
				eof_entering_note_note = new_note;
				eof_entering_note = 1;
				eof_detect_difficulties(eof_song);
				eof_track_sort_notes(eof_song->track[eof_selected_track]);
			}
		}
	}
	if(eof_held_2 == 1)
	{
		if(eof_entering_note_note && eof_music_pos - eof_av_delay < eof_entering_note_note->pos + 50)
		{
			eof_entering_note_note->note |= 2;
		}
		else
		{
			new_note = eof_track_add_note(eof_song->track[eof_selected_track]);
			if(new_note)
			{
				eof_note_create(new_note, 0, 1, 0, 0, 0, eof_music_pos - eof_av_delay, 1);
				eof_snap_logic(&eof_snap, new_note->pos);
				new_note->pos = eof_snap.pos;
				new_note->type = eof_note_type;
				eof_entering_note_note = new_note;
				eof_entering_note = 1;
				eof_detect_difficulties(eof_song);
				eof_track_sort_notes(eof_song->track[eof_selected_track]);
			}
		}
	}
	if(eof_held_3 == 1)
	{
		if(eof_entering_note_note && eof_music_pos - eof_av_delay < eof_entering_note_note->pos + 50)
		{
			eof_entering_note_note->note |= 4;
		}
		else
		{
			new_note = eof_track_add_note(eof_song->track[eof_selected_track]);
			if(new_note)
			{
				eof_note_create(new_note, 0, 0, 1, 0, 0, eof_music_pos - eof_av_delay, 1);
				eof_snap_logic(&eof_snap, new_note->pos);
				new_note->pos = eof_snap.pos;
				new_note->type = eof_note_type;
				eof_entering_note_note = new_note;
				eof_entering_note = 1;
				eof_detect_difficulties(eof_song);
				eof_track_sort_notes(eof_song->track[eof_selected_track]);
			}
		}
	}
	if(eof_held_4 == 1)
	{
		if(eof_entering_note_note && eof_music_pos - eof_av_delay < eof_entering_note_note->pos + 50)
		{
			eof_entering_note_note->note |= 8;
		}
		else
		{
			new_note = eof_track_add_note(eof_song->track[eof_selected_track]);
			if(new_note)
			{
				eof_note_create(new_note, 0, 0, 0, 1, 0, eof_music_pos - eof_av_delay, 1);
				eof_snap_logic(&eof_snap, new_note->pos);
				new_note->pos = eof_snap.pos;
				new_note->type = eof_note_type;
				eof_entering_note_note = new_note;
				eof_entering_note = 1;
				eof_detect_difficulties(eof_song);
				eof_track_sort_notes(eof_song->track[eof_selected_track]);
			}
		}
	}
	if(eof_held_5 == 1)
	{
		if(eof_entering_note_note && eof_music_pos - eof_av_delay < eof_entering_note_note->pos + 50)
		{
			eof_entering_note_note->note |= 16;
		}
		else
		{
			new_note = eof_track_add_note(eof_song->track[eof_selected_track]);
			if(new_note)
			{
				eof_note_create(new_note, 0, 0, 0, 0, 1, eof_music_pos - eof_av_delay, 1);
				eof_snap_logic(&eof_snap, new_note->pos);
				new_note->pos = eof_snap.pos;
				new_note->type = eof_note_type;
				eof_entering_note_note = new_note;
				eof_entering_note = 1;
				eof_detect_difficulties(eof_song);
				eof_track_sort_notes(eof_song->track[eof_selected_track]);
			}
		}
	}
}

void eof_editor_logic(void)
{
	int i;
	int use_this_x = mouse_x;
	int next_note_pos = 0;
	EOF_NOTE * new_note = NULL;
	
	eof_hover_note = -1;
	eof_hover_note_2 = -1;
	eof_hover_lyric = -1;
	
	eof_mickey_z = eof_mouse_z - mouse_z;
	eof_mouse_z = mouse_z;
		
	if(eof_music_paused && eof_song_loaded)
	{
		int pos = eof_music_pos / eof_zoom;
		int npos;
		
		if(!(mouse_b & 1) && !(mouse_b & 2) && !key[KEY_INSERT])
		{
			eof_undo_toggle = 0;
			if(eof_notes_moved)
			{
				eof_track_sort_notes(eof_song->track[eof_selected_track]);
				eof_track_fixup_notes(eof_song->track[eof_selected_track], 1);
				eof_determine_hopos();
				eof_notes_moved = 0;
			}
		}
		
		/* mouse is in the fretboard area */
		if(mouse_y >= eof_window_editor->y + 25 + EOF_EDITOR_RENDER_OFFSET && mouse_y < eof_window_editor->y + eof_screen_layout.fretboard_h + EOF_EDITOR_RENDER_OFFSET)
		{
			int pos = eof_music_pos / eof_zoom;
			int lpos = pos < 300 ? (mouse_x - 20) * eof_zoom : ((pos - 300) + mouse_x - 20) * eof_zoom;
			eof_snap_logic(&eof_snap, lpos);
			eof_snap_length_logic(&eof_snap);
			eof_pen_note.pos = eof_snap.pos;
			use_this_x = lpos;
			eof_pen_visible = 1;
			for(i = 0; i < eof_song->track[eof_selected_track]->notes && eof_hover_note < 0; i++)
			{
				if(eof_song->track[eof_selected_track]->note[i]->type == eof_note_type)
				{
					npos = eof_song->track[eof_selected_track]->note[i]->pos;
					if(use_this_x > npos - (6 * eof_zoom) && use_this_x < npos + (6 * eof_zoom))
					{
						eof_hover_note = i;
					}
					else if(eof_pen_note.pos > npos - (6 * eof_zoom) && eof_pen_note.pos < npos + (6 * eof_zoom))
					{
						eof_hover_note = i;
					}
					if(npos > eof_pen_note.pos && next_note_pos == 0)
					{
						next_note_pos = npos;
					}
				}
			}
			if(eof_input_mode == EOF_INPUT_PIANO_ROLL || eof_input_mode == EOF_INPUT_REX)
			{
				if(eof_hover_note >= 0)
				{
					eof_pen_note.note = eof_song->track[eof_selected_track]->note[eof_hover_note]->note;
					eof_pen_note.length = eof_song->track[eof_selected_track]->note[eof_hover_note]->length;
					if(!eof_mouse_drug)
					{
						eof_pen_note.pos = eof_song->track[eof_selected_track]->note[eof_hover_note]->pos;
						switch(eof_hover_piece)
						{
							case 0:
							{
								if(eof_inverted_notes)
								{
									eof_pen_note.note |= 16;
								}
								else
								{
									eof_pen_note.note |= 1;
								}
								break;
							}
							case 1:
							{
								if(eof_inverted_notes)
								{
									eof_pen_note.note |= 8;
								}
								else
								{
									eof_pen_note.note |= 2;
								}
								break;
							}
							case 2:
							{
								eof_pen_note.note |= 4;
								break;
							}
							case 3:
							{
								if(eof_inverted_notes)
								{
									eof_pen_note.note |= 2;
								}
								else
								{
									eof_pen_note.note |= 8;
								}
								break;
							}
							case 4:
							{
								if(eof_inverted_notes)
								{
									eof_pen_note.note |= 1;
								}
								else
								{
									eof_pen_note.note |= 16;
								}
								break;
							}
						}
					}
				}
				else
				{
					if(eof_input_mode == EOF_INPUT_REX)
					{
						eof_pen_note.note = 0;
					}
					if(KEY_EITHER_SHIFT)
					{
						eof_pen_note.length = 1;
					}
					else
					{
						eof_pen_note.length = eof_snap.length;
						if(eof_hover_note < 0 && next_note_pos > 0 && eof_pen_note.pos + eof_pen_note.length >= next_note_pos)
						{
							eof_pen_note.length = next_note_pos - eof_pen_note.pos - 1;
						}
					}
				}
			}

			/* calculate piece for piano roll mode */
			if(eof_input_mode == EOF_INPUT_PIANO_ROLL)
			{
				if(mouse_y > eof_window_editor->y + EOF_EDITOR_RENDER_OFFSET + 35 - 10 && mouse_y < eof_window_editor->y + EOF_EDITOR_RENDER_OFFSET + 15 + 10 + eof_screen_layout.note_y[0])
				{
					eof_hover_piece = 0;
				}
				else if(mouse_y > eof_window_editor->y + EOF_EDITOR_RENDER_OFFSET + 35 - 10 + 20 && mouse_y < eof_window_editor->y + EOF_EDITOR_RENDER_OFFSET + 15 + 10 + eof_screen_layout.note_y[1])
				{
					eof_hover_piece = 1;
				}
				else if(mouse_y > eof_window_editor->y + EOF_EDITOR_RENDER_OFFSET + 35 - 10 + 40 && mouse_y < eof_window_editor->y + EOF_EDITOR_RENDER_OFFSET + 15 + 10 + eof_screen_layout.note_y[2])
				{
					eof_hover_piece = 2;
				}
				else if(mouse_y > eof_window_editor->y + EOF_EDITOR_RENDER_OFFSET + 35 - 10 + 60 && mouse_y < eof_window_editor->y + EOF_EDITOR_RENDER_OFFSET + 15 + 10 + eof_screen_layout.note_y[3])
				{
					eof_hover_piece = 3;
				}
				else if(mouse_y > eof_window_editor->y + EOF_EDITOR_RENDER_OFFSET + 35 - 10 + 80 && mouse_y < eof_window_editor->y + EOF_EDITOR_RENDER_OFFSET + 15 + 10 + eof_screen_layout.note_y[4])
				{
					eof_hover_piece = 4;
				}
				else
				{
					eof_hover_piece = -1;
				}
				if(eof_hover_note < 0)
				{
					switch(eof_hover_piece)
					{
						case -1:
						{
							eof_pen_note.note = 0;
							break;
						}
						case 0:
						{
							if(eof_inverted_notes)
							{
								eof_pen_note.note = 16;
							}
							else
							{
								eof_pen_note.note = 1;
							}
							break;
						}
						case 1:
						{
							if(eof_inverted_notes)
							{
								eof_pen_note.note = 8;
							}
							else
							{
								eof_pen_note.note = 2;
							}
							break;
						}
						case 2:
						{
							eof_pen_note.note = 4;
							break;
						}
						case 3:
						{
							if(eof_inverted_notes)
							{
								eof_pen_note.note = 2;
							}
							else
							{
								eof_pen_note.note = 8;
							}
							break;
						}
						case 4:
						{
							if(eof_inverted_notes)
							{
								eof_pen_note.note = 1;
							}
							else
							{
								eof_pen_note.note = 16;
							}
							break;
						}
					}
				}
			}
			
			/* handle initial click */
			if(mouse_b & 1 && eof_lclick_released)
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
						eof_selection.last_pos = eof_song->track[eof_selected_track]->note[eof_selection.last]->pos;
					}
					eof_selection.current = eof_hover_note;
					eof_selection.current_pos = eof_song->track[eof_selected_track]->note[eof_selection.current]->pos;
					if(KEY_EITHER_SHIFT)
					{
						if(eof_selection.range_pos_1 == 0 && eof_selection.range_pos_2 == 0)
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
					eof_peg_x = eof_song->track[eof_selected_track]->note[eof_pegged_note]->pos;
					if(!KEY_EITHER_CTRL)
					{
						
						/* Shift+Click selects range */
						if(KEY_EITHER_SHIFT && !ignore_range)
						{
							
							if(eof_selection.range_pos_1 < eof_selection.range_pos_2)
							{
								for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
								{
									if(eof_song->track[eof_selected_track]->note[i]->type == eof_note_type && eof_song->track[eof_selected_track]->note[i]->pos >= eof_selection.range_pos_1 && eof_song->track[eof_selected_track]->note[i]->pos <= eof_selection.range_pos_2)
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
								for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
								{
									if(eof_song->track[eof_selected_track]->note[i]->type == eof_note_type && eof_song->track[eof_selected_track]->note[i]->pos >= eof_selection.range_pos_2 && eof_song->track[eof_selected_track]->note[i]->pos <= eof_selection.range_pos_1)
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
			}
			unsigned long move_offset = 0;
			int revert = 0;
			int revert_amount = 0;
			if(mouse_b & 1 && !eof_lclick_released)
			{
				if(eof_mouse_drug && !KEY_EITHER_SHIFT && !KEY_EITHER_CTRL)
				{
					eof_mouse_drug++;
					if(eof_mouse_drug == 11)
					{
						eof_mickeys_x = mouse_x - eof_click_x;
					}
				}
				if(eof_mickeys_x != 0 && !eof_mouse_drug)
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
				if(eof_mouse_drug > 10 && eof_selection.current != EOF_MAX_NOTES - 1)
				{
					if(eof_snap_mode != EOF_SNAP_OFF && !KEY_EITHER_CTRL)
					{
						move_offset = eof_pen_note.pos - eof_song->track[eof_selected_track]->note[eof_selection.current]->pos;
					}
					if(!eof_undo_toggle && (move_offset != 0 || eof_snap_mode == EOF_SNAP_OFF || KEY_EITHER_CTRL))
					{
						eof_prepare_undo(0);
					}
					eof_notes_moved = 1;
					for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
					{
						if(eof_selection.multi[i])
						{
							if(eof_snap_mode == EOF_SNAP_OFF || KEY_EITHER_CTRL)
							{
								if(eof_song->track[eof_selected_track]->note[i]->pos == eof_selection.current_pos)
								{
									eof_selection.current_pos += eof_mickeys_x * eof_zoom;
								}
								eof_song->track[eof_selected_track]->note[i]->pos += eof_mickeys_x * eof_zoom;
							}
							else
							{
								if(eof_song->track[eof_selected_track]->note[i]->pos == eof_selection.current_pos)
								{
									eof_selection.current_pos += move_offset;
								}
								eof_song->track[eof_selected_track]->note[i]->pos += move_offset;
							}
							if(eof_song->track[eof_selected_track]->note[i]->pos + eof_song->track[eof_selected_track]->note[i]->length >= eof_music_length)
							{
								revert = 1;
								revert_amount = eof_song->track[eof_selected_track]->note[i]->pos + eof_song->track[eof_selected_track]->note[i]->length - eof_music_length;
							}
						}
					}
					if(revert)
					{
						for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
						{
							if(eof_selection.multi[i])
							{
								eof_song->track[eof_selected_track]->note[i]->pos -= revert_amount;
							}
						}
					}
				}
//				eof_song->track[eof_selected_track]->note[eof_pegged_note].pos = eof_peg_x + (mouse_x - eof_click_x) * eof_zoom;
			}
			if((mouse_b & 2 || key[KEY_INSERT]) && eof_rclick_released && eof_pen_note.note && eof_pen_note.pos < eof_music_length)
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
						switch(eof_hover_piece)
						{
							case 0:
							{
								if(eof_inverted_notes)
								{
									eof_song->track[eof_selected_track]->note[eof_hover_note]->note ^= 16;
								}
								else
								{
									eof_song->track[eof_selected_track]->note[eof_hover_note]->note ^= 1;
								}
								eof_selection.current = eof_hover_note;
								eof_selection.track = eof_selected_track;
								memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
								if(!eof_song->track[eof_selected_track]->note[eof_hover_note]->note)
								{
									eof_track_delete_note(eof_song->track[eof_selected_track], eof_hover_note);
									eof_selection.current = EOF_MAX_NOTES - 1;
									eof_track_sort_notes(eof_song->track[eof_selected_track]);
									eof_track_fixup_notes(eof_song->track[eof_selected_track], 1);
									eof_determine_hopos();
									eof_detect_difficulties(eof_song);
								}
								else
								{
									eof_selection.track = eof_selected_track;
									eof_selection.multi[eof_selection.current] = 1;
									eof_selection.current_pos = eof_song->track[eof_selected_track]->note[eof_selection.current]->pos;
									eof_selection.range_pos_1 = eof_selection.current_pos;
									eof_selection.range_pos_2 = eof_selection.current_pos;
								}
								break;
							}
							case 1:
							{
								if(eof_inverted_notes)
								{
									eof_song->track[eof_selected_track]->note[eof_hover_note]->note ^= 8;
								}
								else
								{
									eof_song->track[eof_selected_track]->note[eof_hover_note]->note ^= 2;
								}
								eof_selection.current = eof_hover_note;
								eof_selection.track = eof_selected_track;
								memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
								if(!eof_song->track[eof_selected_track]->note[eof_hover_note]->note)
								{
									eof_track_delete_note(eof_song->track[eof_selected_track], eof_hover_note);
									eof_selection.current = EOF_MAX_NOTES - 1;
									eof_track_sort_notes(eof_song->track[eof_selected_track]);
									eof_track_fixup_notes(eof_song->track[eof_selected_track], 1);
									eof_determine_hopos();
									eof_detect_difficulties(eof_song);
								}
								else
								{
									eof_selection.track = eof_selected_track;
									eof_selection.multi[eof_selection.current] = 1;
									eof_selection.current_pos = eof_song->track[eof_selected_track]->note[eof_selection.current]->pos;
									eof_selection.range_pos_1 = eof_selection.current_pos;
									eof_selection.range_pos_2 = eof_selection.current_pos;
								}
								break;
							}
							case 2:
							{
								eof_song->track[eof_selected_track]->note[eof_hover_note]->note ^= 4;
								eof_selection.current = eof_hover_note;
								eof_selection.track = eof_selected_track;
								memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
								if(!eof_song->track[eof_selected_track]->note[eof_hover_note]->note)
								{
									eof_track_delete_note(eof_song->track[eof_selected_track], eof_hover_note);
									eof_selection.current = EOF_MAX_NOTES - 1;
									eof_track_sort_notes(eof_song->track[eof_selected_track]);
									eof_track_fixup_notes(eof_song->track[eof_selected_track], 1);
									eof_determine_hopos();
									eof_detect_difficulties(eof_song);
								}
								else
								{
									eof_selection.track = eof_selected_track;
									eof_selection.multi[eof_selection.current] = 1;
									eof_selection.current_pos = eof_song->track[eof_selected_track]->note[eof_selection.current]->pos;
									eof_selection.range_pos_1 = eof_selection.current_pos;
									eof_selection.range_pos_2 = eof_selection.current_pos;
								}
								break;
							}
							case 3:
							{
								if(eof_inverted_notes)
								{
									eof_song->track[eof_selected_track]->note[eof_hover_note]->note ^= 2;
								}
								else
								{
									eof_song->track[eof_selected_track]->note[eof_hover_note]->note ^= 8;
								}
								eof_selection.current = eof_hover_note;
								eof_selection.track = eof_selected_track;
								memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
								if(!eof_song->track[eof_selected_track]->note[eof_hover_note]->note)
								{
									eof_track_delete_note(eof_song->track[eof_selected_track], eof_hover_note);
									eof_selection.current = EOF_MAX_NOTES - 1;
									eof_track_sort_notes(eof_song->track[eof_selected_track]);
									eof_track_fixup_notes(eof_song->track[eof_selected_track], 1);
									eof_determine_hopos();
									eof_detect_difficulties(eof_song);
								}
								else
								{
									eof_selection.track = eof_selected_track;
									eof_selection.multi[eof_selection.current] = 1;
									eof_selection.current_pos = eof_song->track[eof_selected_track]->note[eof_selection.current]->pos;
									eof_selection.range_pos_1 = eof_selection.current_pos;
									eof_selection.range_pos_2 = eof_selection.current_pos;
								}
								break;
							}
							case 4:
							{
								if(eof_inverted_notes)
								{
									eof_song->track[eof_selected_track]->note[eof_hover_note]->note ^= 1;
								}
								else
								{
									eof_song->track[eof_selected_track]->note[eof_hover_note]->note ^= 16;
								}
								eof_selection.current = eof_hover_note;
								eof_selection.track = eof_selected_track;
								memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
								if(!eof_song->track[eof_selected_track]->note[eof_hover_note]->note)
								{
									eof_track_delete_note(eof_song->track[eof_selected_track], eof_hover_note);
									eof_selection.current = EOF_MAX_NOTES - 1;
									eof_track_sort_notes(eof_song->track[eof_selected_track]);
									eof_track_fixup_notes(eof_song->track[eof_selected_track], 1);
									eof_determine_hopos();
									eof_detect_difficulties(eof_song);
								}
								else
								{
									eof_selection.track = eof_selected_track;
									eof_selection.multi[eof_selection.current] = 1;
									eof_selection.current_pos = eof_song->track[eof_selected_track]->note[eof_selection.current]->pos;
									eof_selection.range_pos_1 = eof_selection.current_pos;
									eof_selection.range_pos_2 = eof_selection.current_pos;
								}
								break;
							}
						}
					}
					else
					{
						new_note = eof_track_add_note(eof_song->track[eof_selected_track]);
						if(new_note)
						{
							eof_note_create(new_note, eof_pen_note.note & 1, eof_pen_note.note & 2, eof_pen_note.note & 4, eof_pen_note.note & 8, eof_pen_note.note & 16, eof_pen_note.pos, KEY_EITHER_SHIFT ? 1 : eof_snap.length);
							new_note->type = eof_note_type;
							eof_selection.current_pos = new_note->pos;
							eof_selection.range_pos_1 = eof_selection.current_pos;
							eof_selection.range_pos_2 = eof_selection.current_pos;
							memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
							eof_track_sort_notes(eof_song->track[eof_selected_track]);
							eof_track_fixup_notes(eof_song->track[eof_selected_track], 0);
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
					new_note = eof_track_add_note(eof_song->track[eof_selected_track]);
					if(new_note)
					{
						eof_note_create(new_note, eof_pen_note.note & 1, eof_pen_note.note & 2, eof_pen_note.note & 4, eof_pen_note.note & 8, eof_pen_note.note & 16, eof_pen_note.pos, KEY_EITHER_SHIFT ? 1 : eof_snap.length);
						new_note->type = eof_note_type;
						eof_selection.current_pos = new_note->pos;
						eof_selection.range_pos_1 = eof_selection.current_pos;
						eof_selection.range_pos_2 = eof_selection.current_pos;
						memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
						eof_track_sort_notes(eof_song->track[eof_selected_track]);
						eof_track_fixup_notes(eof_song->track[eof_selected_track], 0);
						eof_determine_hopos();
						eof_detect_difficulties(eof_song);
					}
				}
			}
			if(!(mouse_b & 2) && !key[KEY_INSERT])
			{
				eof_rclick_released = 1;
			}
		
//			eof_mickey_z = eof_mouse_z - mouse_z;
//			eof_mouse_z = mouse_z;
			if(eof_mickey_z != 0 && eof_count_selected_notes(NULL, 0))
			{
				eof_prepare_undo(EOF_UNDO_TYPE_NOTE_LENGTH);
			}
			if(eof_snap_mode == EOF_SNAP_OFF)
			{
				if(KEY_EITHER_CTRL)
				{
					for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
					{
						if(eof_selection.track == eof_selected_track && eof_selection.multi[i] && eof_song->track[eof_selected_track]->note[i]->type == eof_note_type)
						{
							eof_song->track[eof_selected_track]->note[i]->length -= eof_mickey_z * 10;
						}
					}
				}
				else
				{
					for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
					{
						if(eof_selection.track == eof_selected_track && eof_selection.multi[i] && eof_song->track[eof_selected_track]->note[i]->type == eof_note_type)
						{
							eof_song->track[eof_selected_track]->note[i]->length -= eof_mickey_z * 100;
						}
					}
				}
			}
			else
			{
				int b;
				if(eof_mickey_z > 0)
				{
					for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
					{
						if(eof_selection.track == eof_selected_track && eof_selection.multi[i] && eof_song->track[eof_selected_track]->note[i]->type == eof_note_type)
						{
							b = eof_get_beat(eof_song->track[eof_selected_track]->note[i]->pos + eof_song->track[eof_selected_track]->note[i]->length - 1);
							if(b >= 0)
							{
								eof_snap_logic(&eof_tail_snap, eof_song->beat[b]->pos);
							}
							else
							{
								eof_snap_logic(&eof_tail_snap, eof_song->track[eof_selected_track]->note[i]->pos + eof_song->track[eof_selected_track]->note[i]->length - 1);
							}
							eof_snap_length_logic(&eof_tail_snap);
//							allegro_message("%d, %d\n%lu, %lu", eof_tail_snap.length, eof_tail_snap.beat, eof_song->track[eof_selected_track]->note[i]->pos + eof_song->track[eof_selected_track]->note[i]->length, eof_song->beat[eof_tail_snap.beat]->pos);
							eof_song->track[eof_selected_track]->note[i]->length -= eof_tail_snap.length;
							if(eof_song->track[eof_selected_track]->note[i]->length > 1)
							{
								eof_snap_logic(&eof_tail_snap, eof_song->track[eof_selected_track]->note[i]->pos + eof_song->track[eof_selected_track]->note[i]->length);
								eof_set_tail_pos(i, eof_tail_snap.pos);
							}
						}
					}
				}
				else if(eof_mickey_z < 0)
				{
					for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
					{
						if(eof_selection.track == eof_selected_track && eof_selection.multi[i] && eof_song->track[eof_selected_track]->note[i]->type == eof_note_type)
						{
							eof_snap_logic(&eof_tail_snap, eof_song->track[eof_selected_track]->note[i]->pos + eof_song->track[eof_selected_track]->note[i]->length + 1);
							if(eof_tail_snap.pos > eof_song->track[eof_selected_track]->note[i]->pos + eof_song->track[eof_selected_track]->note[i]->length + 1)
							{
								eof_set_tail_pos(i, eof_tail_snap.pos);
							}
							else
							{
								eof_snap_length_logic(&eof_tail_snap);
								eof_song->track[eof_selected_track]->note[i]->length += eof_tail_snap.length;
								eof_snap_logic(&eof_tail_snap, eof_song->track[eof_selected_track]->note[i]->pos + eof_song->track[eof_selected_track]->note[i]->length);
								eof_set_tail_pos(i, eof_tail_snap.pos);
							}
//							eof_snap_logic(&eof_tail_snap, eof_song->track[eof_selected_track]->note[i]->pos + eof_song->track[eof_selected_track]->note[i]->length + 1);
						}
					}
				}
			}
			if(eof_mickey_z != 0)
			{
				eof_track_fixup_notes(eof_song->track[eof_selected_track], 1);
			}
		}
		else
		{
			eof_pen_visible = 0;
//			eof_hover_note = -1;
		}
		
		/* mouse is in beat marker area */
		pos = eof_music_pos / eof_zoom;
		if(mouse_y >= eof_window_editor->y + EOF_EDITOR_RENDER_OFFSET - 4 && mouse_y < eof_window_editor->y + EOF_EDITOR_RENDER_OFFSET + 18)
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
				if(mouse_x > npos - 16 && mouse_x < npos + 16 && eof_blclick_released)
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
				if(eof_mickeys_x != 0 && !eof_mouse_drug)
				{
					eof_mouse_drug++;
				}
				
				if(eof_blclick_released)
				{
					if(eof_hover_beat >= 0)
					{
						eof_select_beat(eof_hover_beat);
//						eof_selected_beat = eof_hover_beat;
					}
					eof_blclick_released = 0;
					eof_click_x = mouse_x;
					eof_mouse_drug = 0;
				}
				
				if(eof_mouse_drug > 10 && !eof_blclick_released && eof_selected_beat == 0 && eof_mickeys_x != 0 && eof_hover_beat == eof_selected_beat && !(eof_mickeys_x * eof_zoom < 0 && eof_song->beat[0]->pos == 0))
				{
					if(!eof_undo_toggle)
					{
						eof_prepare_undo(0);
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
								eof_song->beat[i]->fpos += (double)rdiff;
								eof_song->beat[i]->pos = eof_song->beat[i]->fpos;
							}
						}
						else
						{
							eof_song->beat[0]->pos += rdiff;
							eof_song->beat[0]->fpos = eof_song->beat[0]->pos;
						}
					}
					eof_song->tags->ogg[eof_selected_ogg].midi_offset = eof_song->beat[0]->pos;
				}
				else if(eof_mouse_drug > 10 && !eof_blclick_released && eof_selected_beat > 0 && eof_mickeys_x != 0 && ( (eof_beat_is_anchor(eof_hover_beat) || eof_anchor_all_beats || (eof_moving_anchor && eof_hover_beat == eof_selected_beat)) ))
				{
					if(!eof_undo_toggle)
					{
						eof_prepare_undo(0);
						eof_moving_anchor = 1;
						eof_last_midi_offset = eof_song->tags->ogg[eof_selected_ogg].midi_offset;
						eof_adjusted_anchor = 1;
						if((eof_note_auto_adjust && !KEY_EITHER_SHIFT) || (!eof_note_auto_adjust && KEY_EITHER_SHIFT))
						{
							eof_menu_edit_cut(eof_selected_beat, 0, 0.0);
						}
					}
					eof_song->beat[eof_selected_beat]->pos += eof_mickeys_x * eof_zoom;
					eof_song->beat[eof_selected_beat]->fpos = eof_song->beat[eof_selected_beat]->pos;
					if(eof_song->beat[eof_selected_beat]->pos <= eof_song->beat[eof_selected_beat - 1]->pos + 100 || (eof_selected_beat + 1 < eof_song->beats && eof_song->beat[eof_selected_beat]->pos >= eof_song->beat[eof_selected_beat + 1]->pos - 10))
					{
						eof_song->beat[eof_selected_beat]->pos -= eof_mickeys_x * eof_zoom;
						eof_song->beat[eof_selected_beat]->fpos = eof_song->beat[eof_selected_beat]->pos;
					}
					else
					{
						eof_recalculate_beats(eof_selected_beat);
					}
					eof_song->beat[eof_selected_beat]->flags |= EOF_BEAT_FLAG_ANCHOR;
				}
			}
			if(!(mouse_b & 1))
			{
				if(!eof_blclick_released)
				{
					eof_blclick_released = 1;
					if(eof_mouse_drug && eof_song->tags->ogg[eof_selected_ogg].midi_offset != eof_last_midi_offset)
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
						eof_calculate_beats();
					}
				}
				eof_moving_anchor = 0;
				eof_mouse_drug = 0;
				eof_adjusted_anchor = 0;
			}
			if((mouse_b & 2 || key[KEY_INSERT]) && eof_rclick_released && eof_hover_beat >= 0)
			{
				eof_select_beat(eof_hover_beat);
//				eof_selected_beat = eof_hover_beat;
				alogg_seek_abs_msecs_ogg(eof_music_track, eof_song->beat[eof_hover_beat]->pos + eof_av_delay);
				eof_music_actual_pos = eof_song->beat[eof_hover_beat]->pos + eof_av_delay;
				eof_music_pos = eof_music_actual_pos;
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
		if(mouse_y >= eof_window_editor->y + eof_window_editor->h - 17 && mouse_y < eof_window_editor->y + eof_window_editor->h)
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
	}
	else if(eof_song_loaded)
	{
		int pos = eof_music_pos / eof_zoom;
		int npos;
		for(i = 0; i < eof_song->beats - 1; i++)
		{
			if(eof_music_pos >= eof_song->beat[i]->pos && eof_music_pos < eof_song->beat[i + 1]->pos)
			{
				if(eof_hover_beat_2 != i)
				{
					eof_hover_beat_2 = i;
				}
				break;
			}
		}
		for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
		{
			if(eof_song->track[eof_selected_track]->note[i]->type == eof_note_type)
			{
				npos = eof_song->track[eof_selected_track]->note[i]->pos / eof_zoom;
				if(pos - eof_av_delay / eof_zoom > npos && pos - eof_av_delay / eof_zoom < npos + (eof_song->track[eof_selected_track]->note[i]->length > 100 ? eof_song->track[eof_selected_track]->note[i]->length : 100) / eof_zoom)
				{
					eof_hover_note = i;
					break;
				}
			}
		}
		if(i == eof_song->track[eof_selected_track]->notes)
		{
			eof_hover_note = -1;
		}
		for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
		{
			if(eof_song->track[eof_selected_track]->note[i]->type == eof_note_type)
			{
				npos = eof_song->track[eof_selected_track]->note[i]->pos;
				if(eof_music_pos > npos && eof_music_pos < npos + (eof_song->track[eof_selected_track]->note[i]->length > 100 ? eof_song->track[eof_selected_track]->note[i]->length : 100))
				{
					if(eof_hover_note_2 != i)
					{
						eof_hover_note_2 = i;
					}
					break;
				}
			}
		}
		for(i = 0; i < eof_song->vocal_track->lyrics; i++)
		{
			npos = eof_song->vocal_track->lyric[i]->pos;
			if(eof_music_pos - eof_av_delay > npos && eof_music_pos - eof_av_delay < npos + (eof_song->vocal_track->lyric[i]->length > 100 ? eof_song->vocal_track->lyric[i]->length : 100))
			{
				eof_hover_lyric = i;
				break;
			}
		}
		if(eof_selected_track == EOF_TRACK_DRUM)
		{
			eof_editor_drum_logic();
		}
	}
	
	if(eof_music_catalog_playback)
	{
		int npos;
		eof_hover_note_2 = -1;
		for(i = 0; i < eof_song->track[(int)eof_song->catalog->entry[eof_selected_catalog_entry].track]->notes; i++)
		{
			if(eof_song->track[(int)eof_song->catalog->entry[eof_selected_catalog_entry].track]->note[i]->type == eof_song->catalog->entry[eof_selected_catalog_entry].type)
			{
				npos = eof_song->track[(int)eof_song->catalog->entry[eof_selected_catalog_entry].track]->note[i]->pos + eof_av_delay;
				if(eof_music_catalog_pos > npos && eof_music_catalog_pos < npos + (eof_song->track[(int)eof_song->catalog->entry[eof_selected_catalog_entry].track]->note[i]->length > 100 ? eof_song->track[(int)eof_song->catalog->entry[eof_selected_catalog_entry].track]->note[i]->length : 100))
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
	
	/* select difficulty */
	if(mouse_y >= eof_window_editor->y + 7 && mouse_y < eof_window_editor->y + 20 + 8 && mouse_x > 12 && mouse_x < 12 + 5 * 80 + 12 - 1 && eof_song_loaded)
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
	
	/* handle playback controls */
	if(mouse_x >= eof_screen_layout.controls_x && mouse_x < eof_screen_layout.controls_x + 139 && mouse_y >= 22 + 8 && mouse_y < 22 + 17 + 8 && eof_song_loaded)
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
		
	if(((mouse_b & 2) || key[KEY_INSERT]) && eof_input_mode == EOF_INPUT_REX && eof_song_loaded)
	{
		eof_emergency_stop_music();
		eof_render();
		eof_show_mouse(screen);
		if(mouse_y >= eof_window_editor->y + EOF_EDITOR_RENDER_OFFSET - 4 && mouse_y < eof_window_editor->y + EOF_EDITOR_RENDER_OFFSET + 18)
		{
			int pos = eof_music_pos / eof_zoom;
//			int lpos = pos < 300 ? (mouse_x - 20) * eof_zoom : ((pos - 300) + mouse_x - 20) * eof_zoom;
			int lpos = pos < 300 ? (eof_song->beat[eof_selected_beat]->pos / eof_zoom + 20) : 300;
			eof_prepare_menus();
			do_menu(eof_beat_menu, lpos, mouse_y);
			eof_clear_input();
		}
		else if(mouse_y >= eof_window_editor->y + 25 + EOF_EDITOR_RENDER_OFFSET && mouse_y < eof_window_editor->y + eof_screen_layout.fretboard_h + EOF_EDITOR_RENDER_OFFSET)
		{
			if(eof_hover_note >= 0)
			{
				if(eof_count_selected_notes(NULL, 0) <= 0)
				{
					eof_selection.current = eof_hover_note;
					eof_selection.current_pos = eof_song->track[eof_selected_track]->note[eof_selection.current]->pos;
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
						eof_selection.current_pos = eof_song->track[eof_selected_track]->note[eof_selection.current]->pos;
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
	int i;
	int use_this_x = mouse_x;
	int next_note_pos = 0;
	
	eof_hover_note = -1;
	eof_hover_note_2 = -1;
	eof_hover_lyric = -1;
	
	eof_mickey_z = eof_mouse_z - mouse_z;
	eof_mouse_z = mouse_z;
		
	if(eof_music_paused && eof_song_loaded)
	{
		int pos = eof_music_pos / eof_zoom;
		int npos;
		
		if(!(mouse_b & 1) && !(mouse_b & 2) && !key[KEY_INSERT])
		{
			eof_undo_toggle = 0;
			if(eof_notes_moved)
			{
				eof_vocal_track_sort_lyrics(eof_song->vocal_track);
				eof_vocal_track_fixup_lyrics(eof_song->vocal_track, 1);
				eof_notes_moved = 0;
			}
		}
		
		/* mouse is in the mini keyboard area */
		if(mouse_x < 20 && mouse_y >= eof_window_editor->y + 25 + EOF_EDITOR_RENDER_OFFSET && mouse_y < eof_window_editor->y + eof_screen_layout.fretboard_h + EOF_EDITOR_RENDER_OFFSET)
		{
			eof_pen_lyric.pos = -1;
			eof_pen_lyric.length = 1;
			eof_pen_lyric.note = eof_vocals_offset + (EOF_EDITOR_RENDER_OFFSET + 35 + eof_screen_layout.vocal_y - mouse_y) / eof_screen_layout.vocal_tail_size;
			if(eof_pen_lyric.note < eof_vocals_offset || eof_pen_lyric.note >= eof_vocals_offset + eof_screen_layout.vocal_view_size)
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
				if(n >= eof_vocals_offset && n < eof_vocals_offset + eof_screen_layout.vocal_view_size && n != eof_last_tone)
				{
					eof_mix_play_note(n);
					eof_last_tone = n;
				}
			}
		}
		
		/* mouse is in the fretboard area */
		else if(mouse_y >= eof_window_editor->y + 25 + EOF_EDITOR_RENDER_OFFSET && mouse_y < eof_window_editor->y + eof_screen_layout.fretboard_h + EOF_EDITOR_RENDER_OFFSET)
		{
			int pos = eof_music_pos / eof_zoom;
			int lpos = pos < 300 ? (mouse_x - 20) * eof_zoom : ((pos - 300) + mouse_x - 20) * eof_zoom;
			eof_snap_logic(&eof_snap, lpos);
			eof_snap_length_logic(&eof_snap);
			eof_pen_lyric.pos = eof_snap.pos;
			eof_pen_lyric.length = 100;
			eof_pen_lyric.note = eof_vocals_offset + (EOF_EDITOR_RENDER_OFFSET + 35 + eof_screen_layout.vocal_y - mouse_y) / eof_screen_layout.vocal_tail_size;
			if(eof_pen_lyric.note < eof_vocals_offset || eof_pen_lyric.note >= eof_vocals_offset + eof_screen_layout.vocal_view_size)
			{
				eof_pen_lyric.note = 0;
			}
			use_this_x = lpos;
			eof_pen_visible = 1;
			for(i = 0; i < eof_song->vocal_track->lyrics && eof_hover_note < 0; i++)
			{
				npos = eof_song->vocal_track->lyric[i]->pos;
				if(use_this_x > npos && use_this_x < npos + eof_song->vocal_track->lyric[i]->length)
				{
					eof_hover_note = i;
				}
				else if(eof_pen_lyric.pos > npos - (6 * eof_zoom) && eof_pen_lyric.pos < npos + (6 * eof_zoom))
				{
					eof_hover_note = i;
				}
				if(npos > eof_pen_lyric.pos && next_note_pos == 0)
				{
					next_note_pos = npos;
				}
			}
			if(eof_hover_note >= 0 && !(mouse_b & 1))
			{
				eof_pen_lyric.pos = eof_song->vocal_track->lyric[eof_hover_note]->pos;
				eof_pen_lyric.length = eof_song->vocal_track->lyric[eof_hover_note]->length;
			}
			if(eof_input_mode == EOF_INPUT_PIANO_ROLL || eof_input_mode == EOF_INPUT_REX)
			{
				if(eof_hover_note >= 0)
				{
					if(!eof_mouse_drug)
					{
						eof_pen_lyric.pos = eof_song->vocal_track->lyric[eof_hover_note]->pos;
					}
				}
			}
			
			/* handle initial click */
			if(mouse_b & 1 && eof_lclick_released)
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
						eof_selection.last_pos = eof_song->vocal_track->lyric[eof_selection.last]->pos;
					}
					eof_selection.current = eof_hover_note;
					if(eof_mix_vocal_tones_enabled)
					{
						eof_mix_play_note(eof_song->vocal_track->lyric[eof_selection.current]->note);
					}
					eof_selection.current_pos = eof_song->vocal_track->lyric[eof_selection.current]->pos;
					if(KEY_EITHER_SHIFT)
					{
						if(eof_selection.range_pos_1 == 0 && eof_selection.range_pos_2 == 0)
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
					eof_peg_x = eof_song->vocal_track->lyric[eof_pegged_note]->pos;
					if(!KEY_EITHER_CTRL)
					{
						
						/* Shift+Click selects range */
						if(KEY_EITHER_SHIFT && !ignore_range)
						{
							
							if(eof_selection.range_pos_1 < eof_selection.range_pos_2)
							{
								for(i = 0; i < eof_song->vocal_track->lyrics; i++)
								{
									if(eof_song->vocal_track->lyric[i]->pos >= eof_selection.range_pos_1 && eof_song->vocal_track->lyric[i]->pos <= eof_selection.range_pos_2)
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
								for(i = 0; i < eof_song->vocal_track->lyrics; i++)
								{
									if(eof_song->vocal_track->lyric[i]->pos >= eof_selection.range_pos_2 && eof_song->vocal_track->lyric[i]->pos <= eof_selection.range_pos_1)
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
			unsigned long move_offset = 0;
			int revert = 0;
			int revert_amount = 0;
			if(mouse_b & 1 && !eof_lclick_released)
			{
				if(eof_mouse_drug)
				{
					eof_mouse_drug++;
					if(eof_mouse_drug == 11)
					{
						eof_mickeys_x = mouse_x - eof_click_x;
					}
				}
				if(eof_mickeys_x != 0 && !eof_mouse_drug)
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
				if(eof_mouse_drug > 10 && eof_selection.current != EOF_MAX_NOTES - 1)
				{
					if(eof_snap_mode != EOF_SNAP_OFF && !KEY_EITHER_CTRL)
					{
						move_offset = eof_pen_lyric.pos - eof_song->vocal_track->lyric[eof_selection.current]->pos;
					}
					if(!eof_undo_toggle && (move_offset != 0 || eof_snap_mode == EOF_SNAP_OFF || KEY_EITHER_CTRL))
					{
						eof_prepare_undo(0);
					}
					eof_notes_moved = 1;
					for(i = 0; i < eof_song->vocal_track->lyrics; i++)
					{
						if(eof_selection.multi[i])
						{
							if(eof_snap_mode == EOF_SNAP_OFF || KEY_EITHER_CTRL)
							{
								if(eof_song->vocal_track->lyric[i]->pos == eof_selection.current_pos)
								{
									eof_selection.current_pos += eof_mickeys_x * eof_zoom;
								}
								eof_song->vocal_track->lyric[i]->pos += eof_mickeys_x * eof_zoom;
							}
							else
							{
								if(eof_song->vocal_track->lyric[i]->pos == eof_selection.current_pos)
								{
									eof_selection.current_pos += move_offset;
								}
								eof_song->vocal_track->lyric[i]->pos += move_offset;
							}
							if(eof_song->vocal_track->lyric[i]->pos + eof_song->vocal_track->lyric[i]->length >= eof_music_length)
							{
								revert = 1;
								revert_amount = eof_song->vocal_track->lyric[i]->pos + eof_song->vocal_track->lyric[i]->length - eof_music_length;
							}
						}
					}
					if(revert)
					{
						for(i = 0; i < eof_song->vocal_track->lyrics; i++)
						{
							if(eof_selection.multi[i])
							{
								eof_song->vocal_track->lyric[i]->pos -= revert_amount;
							}
						}
					}
				}
			}
			if(((eof_input_mode != EOF_INPUT_REX && (mouse_b & 2 || key[KEY_INSERT])) || (eof_input_mode == EOF_INPUT_REX && !KEY_EITHER_SHIFT && !KEY_EITHER_CTRL && (key[KEY_1] || key[KEY_2] || key[KEY_3] || key[KEY_4] || key[KEY_5]))) && eof_rclick_released && eof_pen_lyric.pos < eof_music_length)
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
					if(mouse_y >= EOF_EDITOR_RENDER_OFFSET + 35 + eof_screen_layout.lyric_y && mouse_y < EOF_EDITOR_RENDER_OFFSET + 35 + eof_screen_layout.lyric_y + 16)
					{
						eof_vocal_track_delete_lyric(eof_song->vocal_track, eof_hover_note);
						eof_selection.current = EOF_MAX_NOTES - 1;
						eof_vocal_track_sort_lyrics(eof_song->vocal_track);
					}
					
					/* delete only the note */
					else if(eof_pen_lyric.note == eof_song->vocal_track->lyric[eof_hover_note]->note)
					{
						eof_song->vocal_track->lyric[eof_hover_note]->note = 0;
						eof_selection.current = eof_hover_note;
						eof_selection.track = eof_selected_track;
						memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
						eof_selection.multi[eof_selection.current] = 1;
						eof_selection.current_pos = eof_song->vocal_track->lyric[eof_selection.current]->pos;
						eof_selection.range_pos_1 = eof_selection.current_pos;
						eof_selection.range_pos_2 = eof_selection.current_pos;
					}
					
					/* move note */
					else
					{
						eof_song->vocal_track->lyric[eof_hover_note]->note = eof_pen_lyric.note;
						eof_selection.current = eof_hover_note;
						eof_selection.track = EOF_TRACK_VOCALS;
						memset(eof_selection.multi, 0, sizeof(char) * EOF_MAX_NOTES);
						eof_selection.multi[eof_selection.current] = 1;
						eof_selection.current_pos = eof_song->vocal_track->lyric[eof_selection.current]->pos;
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
					if(eof_mix_vocal_tones_enabled)
					{
						eof_mix_play_note(eof_pen_lyric.note);
					}
					eof_new_lyric_dialog();
				}
			}
			if(eof_input_mode == EOF_INPUT_REX)
			{
				if(!key[KEY_1] && !key[KEY_2] && !key[KEY_3] && !key[KEY_4] && !key[KEY_5])
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
			if(eof_mickey_z != 0 && eof_count_selected_notes(NULL, 0))
			{
				eof_prepare_undo(EOF_UNDO_TYPE_NOTE_LENGTH);
			}
			if(eof_snap_mode == EOF_SNAP_OFF)
			{
				if(KEY_EITHER_CTRL)
				{
					for(i = 0; i < eof_song->vocal_track->lyrics; i++)
					{
						if(eof_selection.track == EOF_TRACK_VOCALS && eof_selection.multi[i])
						{
							eof_song->vocal_track->lyric[i]->length -= eof_mickey_z * 10;
						}
					}
				}
				else
				{
					for(i = 0; i < eof_song->vocal_track->lyrics; i++)
					{
						if(eof_selection.track == EOF_TRACK_VOCALS && eof_selection.multi[i])
						{
							eof_song->vocal_track->lyric[i]->length -= eof_mickey_z * 100;
						}
					}
				}
			}
			else
			{
				int b;
				if(eof_mickey_z > 0)
				{
					for(i = 0; i < eof_song->vocal_track->lyrics; i++)
					{
						if(eof_selection.track == EOF_TRACK_VOCALS && eof_selection.multi[i])
						{
							b = eof_get_beat(eof_song->vocal_track->lyric[i]->pos + eof_song->vocal_track->lyric[i]->length - 1);
							if(b >= 0)
							{
								eof_snap_logic(&eof_tail_snap, eof_song->beat[b]->pos);
							}
							else
							{
								eof_snap_logic(&eof_tail_snap, eof_song->vocal_track->lyric[i]->pos + eof_song->vocal_track->lyric[i]->length - 1);
							}
							eof_snap_length_logic(&eof_tail_snap);
//							allegro_message("%d, %d\n%lu, %lu", eof_tail_snap.length, eof_tail_snap.beat, eof_song->track[eof_selected_track]->note[i]->pos + eof_song->track[eof_selected_track]->note[i]->length, eof_song->beat[eof_tail_snap.beat]->pos);
							eof_song->vocal_track->lyric[i]->length -= eof_tail_snap.length;
							if(eof_song->vocal_track->lyric[i]->length > 1)
							{
								eof_snap_logic(&eof_tail_snap, eof_song->vocal_track->lyric[i]->pos + eof_song->vocal_track->lyric[i]->length);
								eof_set_vocal_tail_pos(i, eof_tail_snap.pos);
							}
						}
					}
				}
				else if(eof_mickey_z < 0)
				{
					for(i = 0; i < eof_song->vocal_track->lyrics; i++)
					{
						if(eof_selection.track == EOF_TRACK_VOCALS && eof_selection.multi[i])
						{
							eof_snap_logic(&eof_tail_snap, eof_song->vocal_track->lyric[i]->pos + eof_song->vocal_track->lyric[i]->length + 1);
							if(eof_tail_snap.pos > eof_song->vocal_track->lyric[i]->pos + eof_song->vocal_track->lyric[i]->length + 1)
							{
								eof_set_vocal_tail_pos(i, eof_tail_snap.pos);
							}
							else
							{
								eof_snap_length_logic(&eof_tail_snap);
								eof_song->vocal_track->lyric[i]->length += eof_tail_snap.length;
								eof_snap_logic(&eof_tail_snap, eof_song->vocal_track->lyric[i]->pos + eof_song->vocal_track->lyric[i]->length);
								eof_set_vocal_tail_pos(i, eof_tail_snap.pos);
							}
						}
					}
				}
			}
			if(eof_mickey_z != 0)
			{
				eof_vocal_track_fixup_lyrics(eof_song->vocal_track, 1);
			}
			if(key[KEY_P])
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
		
		/* mouse is in beat marker area */
		pos = eof_music_pos / eof_zoom;
		if(mouse_y >= eof_window_editor->y + EOF_EDITOR_RENDER_OFFSET - 4 && mouse_y < eof_window_editor->y + EOF_EDITOR_RENDER_OFFSET + 18)
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
				if(mouse_x > npos - 16 && mouse_x < npos + 16 && eof_blclick_released)
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
				if(eof_mickeys_x != 0 && !eof_mouse_drug)
				{
					eof_mouse_drug++;
				}
				
				if(eof_blclick_released)
				{
					if(eof_hover_beat >= 0)
					{
						eof_select_beat(eof_hover_beat);
//						eof_selected_beat = eof_hover_beat;
					}
					eof_blclick_released = 0;
					eof_click_x = mouse_x;
					eof_mouse_drug = 0;
				}
				
				if(eof_mouse_drug > 10 && !eof_blclick_released && eof_selected_beat == 0 && eof_mickeys_x != 0 && eof_hover_beat == eof_selected_beat && !(eof_mickeys_x * eof_zoom < 0 && eof_song->beat[0]->pos == 0))
				{
					if(!eof_undo_toggle)
					{
						eof_prepare_undo(0);
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
				else if(eof_mouse_drug > 10 && !eof_blclick_released && eof_selected_beat > 0 && eof_mickeys_x != 0 && ( (eof_beat_is_anchor(eof_hover_beat) || eof_anchor_all_beats || (eof_moving_anchor && eof_hover_beat == eof_selected_beat)) ))
				{
					if(!eof_undo_toggle)
					{
						eof_prepare_undo(0);
						eof_moving_anchor = 1;
						eof_last_midi_offset = eof_song->tags->ogg[eof_selected_ogg].midi_offset;
						eof_adjusted_anchor = 1;
						if((eof_note_auto_adjust && !KEY_EITHER_SHIFT) || (!eof_note_auto_adjust && KEY_EITHER_SHIFT))
						{
							eof_menu_edit_cut(eof_selected_beat, 0, 0.0);
						}
					}
					eof_song->beat[eof_selected_beat]->pos += eof_mickeys_x * eof_zoom;
					if(eof_song->beat[eof_selected_beat]->pos <= eof_song->beat[eof_selected_beat - 1]->pos + 100 || (eof_selected_beat + 1 < eof_song->beats && eof_song->beat[eof_selected_beat]->pos >= eof_song->beat[eof_selected_beat + 1]->pos - 10))
					{
						eof_song->beat[eof_selected_beat]->pos -= eof_mickeys_x * eof_zoom;
						eof_song->beat[eof_selected_beat]->fpos = eof_song->beat[eof_selected_beat]->pos;
					}
					else
					{
						eof_recalculate_beats(eof_selected_beat);
					}
					eof_song->beat[eof_selected_beat]->flags |= EOF_BEAT_FLAG_ANCHOR;
				}
			}
			if(!(mouse_b & 1))
			{
				if(!eof_blclick_released)
				{
					eof_blclick_released = 1;
					if(eof_mouse_drug && eof_song->tags->ogg[eof_selected_ogg].midi_offset != eof_last_midi_offset)
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
						eof_calculate_beats();
					}
				}
				eof_moving_anchor = 0;
				eof_mouse_drug = 0;
				eof_adjusted_anchor = 0;
			}
			if((mouse_b & 2 || key[KEY_INSERT]) && eof_rclick_released && eof_hover_beat >= 0)
			{
				eof_select_beat(eof_hover_beat);
//				eof_selected_beat = eof_hover_beat;
				alogg_seek_abs_msecs_ogg(eof_music_track, eof_song->beat[eof_hover_beat]->pos + eof_av_delay);
				eof_music_actual_pos = eof_song->beat[eof_hover_beat]->pos + eof_av_delay;
				eof_music_pos = eof_music_actual_pos;
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
		if(mouse_y >= eof_window_editor->y + eof_window_editor->h - 17 && mouse_y < eof_window_editor->y + eof_window_editor->h)
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
	}
	else if(eof_song_loaded)
	{
		int pos = eof_music_pos / eof_zoom;
		int npos;
		for(i = 0; i < eof_song->beats - 1; i++)
		{
			if(eof_music_pos >= eof_song->beat[i]->pos && eof_music_pos < eof_song->beat[i + 1]->pos)
			{
				if(eof_hover_beat_2 != i)
				{
					eof_hover_beat_2 = i;
				}
				break;
			}
		}
		for(i = 0; i < eof_song->vocal_track->lyrics; i++)
		{
			npos = eof_song->vocal_track->lyric[i]->pos / eof_zoom;
			if(pos - eof_av_delay / eof_zoom > npos && pos - eof_av_delay / eof_zoom < npos + (eof_song->vocal_track->lyric[i]->length > 100 ? eof_song->vocal_track->lyric[i]->length : 100) / eof_zoom)
			{
				eof_hover_note = i;
				eof_pen_lyric.note = eof_song->vocal_track->lyric[i]->note;
				break;
			}
		}
		if(i == eof_song->vocal_track->lyrics)
		{
			eof_hover_note = -1;
			eof_pen_lyric.note = 0;
		}
		for(i = 0; i < eof_song->vocal_track->lyrics; i++)
		{
			npos = eof_song->vocal_track->lyric[i]->pos;
			if(eof_music_pos - eof_av_delay > npos && eof_music_pos - eof_av_delay < npos + (eof_song->vocal_track->lyric[i]->length > 100 ? eof_song->vocal_track->lyric[i]->length : 100))
			{
				eof_hover_note_2 = i;
				eof_hover_lyric = i;
				break;
			}
		}
	}
	
	/* select tab */
/*	if(mouse_y >= eof_window_editor->y + 7 && mouse_y < eof_window_editor->y + 20 + 8 && mouse_x > 12 && mouse_x < 12 + 5 * 80 + 12 - 1 && eof_song_loaded)
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
			if(eof_vocals_tab != eof_hover_type)
			{
				eof_vocals_tab = eof_hover_type;
				eof_mix_find_claps();
				eof_mix_start_helper();
			}
		}
	}
	else
	{
		eof_hover_type = -1;
	} */
	
	/* handle playback controls */
	if(mouse_x >= eof_screen_layout.controls_x && mouse_x < eof_screen_layout.controls_x + 139 && mouse_y >= 22 + 8 && mouse_y < 22 + 17 + 8 && eof_song_loaded)
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
		
	if(((mouse_b & 2) || key[KEY_INSERT]) && eof_input_mode == EOF_INPUT_REX && eof_song_loaded)
	{
		eof_emergency_stop_music();
		eof_render();
		eof_show_mouse(screen);
		if(mouse_y >= eof_window_editor->y + EOF_EDITOR_RENDER_OFFSET - 4 && mouse_y < eof_window_editor->y + EOF_EDITOR_RENDER_OFFSET + 18)
		{
			int pos = eof_music_pos / eof_zoom;
//			int lpos = pos < 300 ? (mouse_x - 20) * eof_zoom : ((pos - 300) + mouse_x - 20) * eof_zoom;
			int lpos = pos < 300 ? (eof_song->beat[eof_selected_beat]->pos / eof_zoom + 20) : 300;
			eof_prepare_menus();
			do_menu(eof_beat_menu, lpos, mouse_y);
			eof_clear_input();
		}
		else if(mouse_y >= eof_window_editor->y + 25 + EOF_EDITOR_RENDER_OFFSET && mouse_y < eof_window_editor->y + eof_screen_layout.fretboard_h + EOF_EDITOR_RENDER_OFFSET)
		{
			if(eof_hover_note >= 0)
			{
				if(eof_count_selected_notes(NULL, 0) <= 0)
				{
					eof_selection.current = eof_hover_note;
					eof_selection.current_pos = eof_song->vocal_track->lyric[eof_selection.current]->pos;
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
						eof_selection.current_pos = eof_song->vocal_track->lyric[eof_selection.current]->pos;
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
	else
	{
		ustrcpy(buffer, "");
	}
	return ret;
}

void eof_render_editor_window(void)
{
	int i, j;
	int pos = eof_music_pos / eof_zoom;
	int bpos[10];
	int lpos;
	int npos;
	int pmin = 0;
	int psec = 0;
	
	/* draw the starting position */
	if(pos < 300)
	{
		lpos = 20;
	}
	else
	{
		lpos = 20 - (pos - 300);
	}
	
	for(i = 0; i < 10; i++)
	{
		bpos[i] = eof_song->bookmark_pos[i] / eof_zoom;
	}
	
	if(eof_song_loaded)
	{

		/* fill in window background color */
		rectfill(eof_window_editor->screen, 0, 25 + 8, eof_window_editor->w - 1, eof_window_editor->h - 1, eof_color_gray);
		
		/* draw the difficulty tabs */
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
		for(i = 0; i < eof_song->track[eof_selected_track]->solos; i++)
		{
			if(pos < 300)
			{
				rectfill(eof_window_editor->screen, 20 + eof_song->track[eof_selected_track]->solo[i].start_pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 25, 20 + eof_song->track[eof_selected_track]->solo[i].end_pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 1, makecol(0, 0, 64));
			}
			else
			{
				rectfill(eof_window_editor->screen, 20 - (pos - 300) + eof_song->track[eof_selected_track]->solo[i].start_pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 25, 20 - (pos - 300) + eof_song->track[eof_selected_track]->solo[i].end_pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 1, makecol(0, 0, 64));
			}
		}
		
		for(i = 0; i < 5; i++)
		{
			hline(eof_window_editor->screen, lpos, EOF_EDITOR_RENDER_OFFSET + 35 + i * eof_screen_layout.string_space, lpos + (eof_music_length) / eof_zoom, eof_color_white);
		}
		vline(eof_window_editor->screen, lpos + (eof_music_length) / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 35, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 11, eof_color_white);
		
		/* draw second markers */
		if(pos < 300)
		{
			npos = 20;
		}
		else
		{
			npos = 20 - ((pos - 300));
		}
		for(i = (-npos / (1000 / eof_zoom)); i < (-npos / (1000 / eof_zoom)) + 12; i++)
		{
			pmin = i / 60;
			psec = i % 60;
			if(i * 1000 < eof_music_length)
			{
				for(j = 0; j < 10; j++)
				{
					vline(eof_window_editor->screen, npos + (1000 * i) / eof_zoom + (j * 100) / eof_zoom, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 9, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 1, makecol(64, 64, 64));
					if(j == 0)
					{
						vline(eof_window_editor->screen, npos + (1000 * i) / eof_zoom + (j * 100) / eof_zoom, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h + 5, eof_color_white);
					}
					else
					{
						vline(eof_window_editor->screen, npos + (1000 * i) / eof_zoom + (j * 100) / eof_zoom, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h + 1, eof_color_white);
					}
				}
				textprintf_ex(eof_window_editor->screen, eof_mono_font, npos + (1000 * i) / eof_zoom - 16, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h + 6, eof_color_white, -1, "%02d:%02d", pmin, psec);
			}
		}
		vline(eof_window_editor->screen, lpos, EOF_EDITOR_RENDER_OFFSET + 35, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 10, eof_color_white);
		rectfill(eof_window_editor->screen, lpos + 1, EOF_EDITOR_RENDER_OFFSET + 35 + 1, lpos + eof_song->tags->ogg[eof_selected_ogg].midi_offset / eof_zoom, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 12, eof_color_black);
		
		/* draw beat lines */
		unsigned long current_ppqn = eof_song->beat[0]->ppqn;
		int current_ppqn_used = 0;
		double current_bpm = (double)60000000.0 / (double)eof_song->beat[0]->ppqn;
		int bcol, bscol, bhcol;
		int beat_counter = 0;
		int beats_per_measure = 0;
		char buffer[16] = {0};
		for(i = 0; i < eof_song->beats; i++)
		{
			if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_START_4_4)
			{
				beats_per_measure = 4;
				beat_counter = 0;
			}
			else if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_START_3_4)
			{
				beats_per_measure = 3;
				beat_counter = 0;
			}
			else if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_START_5_4)
			{
				beats_per_measure = 5;
				beat_counter = 0;
			}
			else if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_START_6_4)
			{
				beats_per_measure = 6;
				beat_counter = 0;
			}
			vline(eof_window_editor->screen, npos + eof_song->beat[i]->pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 35 + 1, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 10 - 1, beat_counter == 0 ? eof_color_white : makecol(160, 160, 160));
			vline(eof_window_editor->screen, npos + eof_song->beat[i]->pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 25, EOF_EDITOR_RENDER_OFFSET + 34, makecol(64, 64, 64));
			if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_ANCHOR)
			{
				vline(eof_window_editor->screen, npos + eof_song->beat[i]->pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + (i % 2 == 0 ? 19 : 9), EOF_EDITOR_RENDER_OFFSET + 24, eof_color_red);
			}
			else
			{
				vline(eof_window_editor->screen, npos + eof_song->beat[i]->pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + (i % 2 == 0 ? 19 : 9), EOF_EDITOR_RENDER_OFFSET + 24, beat_counter == 0 ? eof_color_white : makecol(160, 160, 160));
			}
			bcol = makecol(128, 128, 128);
			bscol = makecol(255, 255, 255);
			bhcol = makecol(0, 255, 0);
			if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_EVENTS)
			{
				line(eof_window_editor->screen, npos + eof_song->beat[i]->pos / eof_zoom - 3, EOF_EDITOR_RENDER_OFFSET + 24, npos + eof_song->beat[i]->pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 24, eof_color_yellow);
				line(eof_window_editor->screen, npos + eof_song->beat[i]->pos / eof_zoom + 3, EOF_EDITOR_RENDER_OFFSET + 24, npos + eof_song->beat[i]->pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 24, eof_color_yellow);
			}
			if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_ANCHOR)
			{
				line(eof_window_editor->screen, npos + eof_song->beat[i]->pos / eof_zoom - 3, EOF_EDITOR_RENDER_OFFSET + 21, npos + eof_song->beat[i]->pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 24, eof_color_red);
				line(eof_window_editor->screen, npos + eof_song->beat[i]->pos / eof_zoom + 3, EOF_EDITOR_RENDER_OFFSET + 21, npos + eof_song->beat[i]->pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 24, eof_color_red);
			}
			if(eof_song->beat[i]->ppqn != current_ppqn)
			{
				current_ppqn = eof_song->beat[i]->ppqn;
				current_ppqn_used = 0;
				current_bpm = (double)60000000.0 / (double)eof_song->beat[i]->ppqn;
			}
			if(i == eof_selected_beat)
			{
				textprintf_ex(eof_window_editor->screen, eof_mono_font, npos + eof_song->beat[i]->pos / eof_zoom - 28, EOF_EDITOR_RENDER_OFFSET + 4 - (i % 2 == 0 ? 0 : 10), i == eof_hover_beat ? bhcol : i == eof_selected_beat ? bscol : bcol, -1, "<%03.2f>", current_bpm);
				current_ppqn_used = 1;
			}
			else if(!current_ppqn_used)
			{
				textprintf_ex(eof_window_editor->screen, eof_mono_font, npos + eof_song->beat[i]->pos / eof_zoom - 20, EOF_EDITOR_RENDER_OFFSET + 4 - (i % 2 == 0 ? 0 : 10), i == eof_hover_beat ? bhcol : i == eof_selected_beat ? bscol : bcol, -1, "%03.2f", current_bpm);
				current_ppqn_used = 1;
			}
			else
			{
				textprintf_ex(eof_window_editor->screen, eof_mono_font, npos + eof_song->beat[i]->pos / eof_zoom - 20 + 9, EOF_EDITOR_RENDER_OFFSET + 4 - (i % 2 == 0 ? 0 : 10), i == eof_hover_beat ? bhcol : i == eof_selected_beat ? bscol : bcol, -1, "-->");
			}
			if(eof_get_ts_text(i, buffer))
			{
				textprintf_ex(eof_window_editor->screen, eof_mono_font, npos + eof_song->beat[i]->pos / eof_zoom - 20 + 3, EOF_EDITOR_RENDER_OFFSET + 4 - (i % 2 == 0 ? 0 : 10) - 12, i == eof_hover_beat ? bhcol : i == eof_selected_beat ? bscol : bcol, -1, "(%s)", buffer);
			}
			beat_counter++;
			if(beat_counter >= beats_per_measure)
			{
				beat_counter = 0;
			}
		}
		
		/* draw the bookmark position */
		for(i = 0; i < 10; i++)
		{
			if(eof_song->bookmark_pos[i] != 0)
			{
				if(pos < 300)
				{
					vline(eof_window_editor->screen, 20 + (eof_song->bookmark_pos[i]) / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 20, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h + 4, makecol(96, 96, 255));
				}
				else
				{
					vline(eof_window_editor->screen, 20 - ((pos - 300)) + (eof_song->bookmark_pos[i]) / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 20, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h + 4, makecol(96, 96, 255));
				}
			}
		}
		
		for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
		{
			if(eof_note_type == eof_song->track[eof_selected_track]->note[i]->type)
			{
				if((eof_input_mode == EOF_INPUT_PIANO_ROLL || eof_input_mode == EOF_INPUT_REX) && eof_music_paused)
				{
					if(eof_hover_note == i)
					{
						eof_note_draw(eof_song->track[eof_selected_track]->note[i], (eof_selection.track == eof_selected_track && eof_selection.multi[i] && eof_music_paused) ? 1 : i == eof_hover_note ? 2 : 3);
					}
					else
					{
						eof_note_draw(eof_song->track[eof_selected_track]->note[i], (eof_selection.track == eof_selected_track && eof_selection.multi[i] && eof_music_paused) ? 1 : i == eof_hover_note ? 2 : 0);
					}
				}
				else
				{
					eof_note_draw(eof_song->track[eof_selected_track]->note[i], (eof_selection.track == eof_selected_track && eof_selection.multi[i] && eof_music_paused) ? 1 : i == eof_hover_note ? 2 : 0);
				}
			}
		}
		if(eof_music_paused && eof_pen_visible && eof_pen_note.pos < eof_music_length)
		{
			if(!eof_mouse_drug)
			{
				if(eof_input_mode == EOF_INPUT_PIANO_ROLL || eof_input_mode == EOF_INPUT_REX)
				{
					eof_note_draw(&eof_pen_note, 3);
				}
				else
				{
					eof_note_draw(&eof_pen_note, 0);
				}
			}
		}
	
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
				vline(eof_window_editor->screen, 20 + (eof_music_actual_length) / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 20, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h + 4, makecol(255, 0, 0));
			}
			else
			{
				vline(eof_window_editor->screen, 20 - ((pos - 300)) + (eof_music_actual_length) / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 20, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h + 4, makecol(255, 0, 0));
			}
		}
		
		for(i = 0; i < 5; i++)
		{
			if(i == eof_note_type)
			{
				textprintf_ex(eof_window_editor->screen, font, 20 + i * 80, 2 + 8, eof_color_black, -1, "%s", eof_note_type_name[i]);
			}
			else
			{
				textprintf_ex(eof_window_editor->screen, font, 20 + i * 80, 2 + 2 + 8, makecol(128, 128, 128), -1, "%s", eof_note_type_name[i]);
			}
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

void eof_render_vocal_editor_window(void)
{
	int i, j;
	int pos = eof_music_pos / eof_zoom;
	int bpos[10];
	int lpos;
	int npos;
	int pmin = 0;
	int psec = 0;
	
	/* draw the starting position */
	if(pos < 300)
	{
		lpos = 20;
	}
	else
	{
		lpos = 20 - (pos - 300);
	}
	
	for(i = 0; i < 10; i++)
	{
		bpos[i] = eof_song->bookmark_pos[i] / eof_zoom;
	}
	
	if(eof_song_loaded)
	{

		/* fill in window background color */
		rectfill(eof_window_editor->screen, 0, 25 + 8, eof_window_editor->w - 1, eof_window_editor->h - 1, eof_color_gray);
		
		/* draw the difficulty tabs */
		draw_sprite(eof_window_editor->screen, eof_image[EOF_IMAGE_VTAB0 + eof_vocals_tab], 0, 8);
		
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
		
		for(i = 0; i < 5; i += 4)
		{
			hline(eof_window_editor->screen, lpos, EOF_EDITOR_RENDER_OFFSET + 35 + i * eof_screen_layout.string_space, lpos + (eof_music_length) / eof_zoom, eof_color_white);
		}
		vline(eof_window_editor->screen, lpos + (eof_music_length) / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 35, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 11, eof_color_white);
		
		/* draw second markers */
		if(pos < 300)
		{
			npos = 20;
		}
		else
		{
			npos = 20 - ((pos - 300));
		}
		for(i = (-npos / (1000 / eof_zoom)); i < (-npos / (1000 / eof_zoom)) + 12; i++)
		{
			pmin = i / 60;
			psec = i % 60;
			if(i * 1000 < eof_music_length)
			{
				for(j = 0; j < 10; j++)
				{
					vline(eof_window_editor->screen, npos + (1000 * i) / eof_zoom + (j * 100) / eof_zoom, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 9, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 1, makecol(64, 64, 64));
					if(j == 0)
					{
						vline(eof_window_editor->screen, npos + (1000 * i) / eof_zoom + (j * 100) / eof_zoom, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h + 5, eof_color_white);
					}
					else
					{
						vline(eof_window_editor->screen, npos + (1000 * i) / eof_zoom + (j * 100) / eof_zoom, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h + 1, eof_color_white);
					}
				}
				textprintf_ex(eof_window_editor->screen, eof_mono_font, npos + (1000 * i) / eof_zoom - 16, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h + 6, eof_color_white, -1, "%02d:%02d", pmin, psec);
			}
		}
		vline(eof_window_editor->screen, lpos, EOF_EDITOR_RENDER_OFFSET + 35, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 10, eof_color_white);
		rectfill(eof_window_editor->screen, lpos + 1, EOF_EDITOR_RENDER_OFFSET + 35 + 1, lpos + eof_song->tags->ogg[eof_selected_ogg].midi_offset / eof_zoom, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 12, eof_color_black);
		
		/* draw beat lines */
		unsigned long current_ppqn = eof_song->beat[0]->ppqn;
		int current_ppqn_used = 0;
		double current_bpm = (double)60000000.0 / (double)eof_song->beat[0]->ppqn;
		int bcol, bscol, bhcol;
		int beat_counter = 0;
		int beats_per_measure = 0;
		char buffer[16] = {0};
		for(i = 0; i < eof_song->beats; i++)
		{
			if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_START_4_4)
			{
				beats_per_measure = 4;
				beat_counter = 0;
			}
			else if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_START_3_4)
			{
				beats_per_measure = 3;
				beat_counter = 0;
			}
			else if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_START_5_4)
			{
				beats_per_measure = 5;
				beat_counter = 0;
			}
			else if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_START_6_4)
			{
				beats_per_measure = 6;
				beat_counter = 0;
			}
			vline(eof_window_editor->screen, npos + eof_song->beat[i]->pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 35 + 1, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 10 - 1, beat_counter == 0 ? eof_color_white : makecol(160, 160, 160));
			vline(eof_window_editor->screen, npos + eof_song->beat[i]->pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 25, EOF_EDITOR_RENDER_OFFSET + 34, makecol(64, 64, 64));
			if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_ANCHOR)
			{
				vline(eof_window_editor->screen, npos + eof_song->beat[i]->pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + (i % 2 == 0 ? 19 : 9), EOF_EDITOR_RENDER_OFFSET + 24, eof_color_red);
			}
			else
			{
				vline(eof_window_editor->screen, npos + eof_song->beat[i]->pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + (i % 2 == 0 ? 19 : 9), EOF_EDITOR_RENDER_OFFSET + 24, beat_counter == 0 ? eof_color_white : makecol(160, 160, 160));
			}
			bcol = makecol(128, 128, 128);
			bscol = makecol(255, 255, 255);
			bhcol = makecol(0, 255, 0);
			if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_EVENTS)
			{
				line(eof_window_editor->screen, npos + eof_song->beat[i]->pos / eof_zoom - 3, EOF_EDITOR_RENDER_OFFSET + 24, npos + eof_song->beat[i]->pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 24, eof_color_yellow);
				line(eof_window_editor->screen, npos + eof_song->beat[i]->pos / eof_zoom + 3, EOF_EDITOR_RENDER_OFFSET + 24, npos + eof_song->beat[i]->pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 24, eof_color_yellow);
			}
			if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_ANCHOR)
			{
				line(eof_window_editor->screen, npos + eof_song->beat[i]->pos / eof_zoom - 3, EOF_EDITOR_RENDER_OFFSET + 21, npos + eof_song->beat[i]->pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 24, eof_color_red);
				line(eof_window_editor->screen, npos + eof_song->beat[i]->pos / eof_zoom + 3, EOF_EDITOR_RENDER_OFFSET + 21, npos + eof_song->beat[i]->pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 24, eof_color_red);
			}
			if(eof_song->beat[i]->ppqn != current_ppqn)
			{
				current_ppqn = eof_song->beat[i]->ppqn;
				current_ppqn_used = 0;
				current_bpm = (double)60000000.0 / (double)eof_song->beat[i]->ppqn;
			}
			if(i == eof_selected_beat)
			{
				textprintf_ex(eof_window_editor->screen, eof_mono_font, npos + eof_song->beat[i]->pos / eof_zoom - 28, EOF_EDITOR_RENDER_OFFSET + 4 - (i % 2 == 0 ? 0 : 10), i == eof_hover_beat ? bhcol : i == eof_selected_beat ? bscol : bcol, -1, "<%03.2f>", current_bpm);
				current_ppqn_used = 1;
			}
			else if(!current_ppqn_used)
			{
				textprintf_ex(eof_window_editor->screen, eof_mono_font, npos + eof_song->beat[i]->pos / eof_zoom - 20, EOF_EDITOR_RENDER_OFFSET + 4 - (i % 2 == 0 ? 0 : 10), i == eof_hover_beat ? bhcol : i == eof_selected_beat ? bscol : bcol, -1, "%03.2f", current_bpm);
				current_ppqn_used = 1;
			}
			else
			{
				textprintf_ex(eof_window_editor->screen, eof_mono_font, npos + eof_song->beat[i]->pos / eof_zoom - 20 + 9, EOF_EDITOR_RENDER_OFFSET + 4 - (i % 2 == 0 ? 0 : 10), i == eof_hover_beat ? bhcol : i == eof_selected_beat ? bscol : bcol, -1, "-->");
			}
			if(eof_get_ts_text(i, buffer))
			{
				textprintf_ex(eof_window_editor->screen, eof_mono_font, npos + eof_song->beat[i]->pos / eof_zoom - 20 + 3, EOF_EDITOR_RENDER_OFFSET + 4 - (i % 2 == 0 ? 0 : 10) - 12, i == eof_hover_beat ? bhcol : i == eof_selected_beat ? bscol : bcol, -1, "(%s)", buffer);
			}
			beat_counter++;
			if(beat_counter >= beats_per_measure)
			{
				beat_counter = 0;
			}
		}
		
		/* draw the bookmark position */
		for(i = 0; i < 10; i++)
		{
			if(eof_song->bookmark_pos[i] != 0)
			{
				if(pos < 300)
				{
					vline(eof_window_editor->screen, 20 + (eof_song->bookmark_pos[i]) / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 20, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h + 4, makecol(96, 96, 255));
				}
				else
				{
					vline(eof_window_editor->screen, 20 - ((pos - 300)) + (eof_song->bookmark_pos[i]) / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 20, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h + 4, makecol(96, 96, 255));
				}
			}
		}
		
		/* clear lyric text area */
		rectfill(eof_window_editor->screen, 0, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.lyric_y + 1, eof_window_editor->w - 1, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.lyric_y + 1 + 16, eof_color_black);
		hline(eof_window_editor->screen, lpos, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.lyric_y + 1 + 16, lpos + (eof_music_length) / eof_zoom, eof_color_white);
		
		/* draw lyric lines */
		for(i = 0; i < eof_song->vocal_track->lines; i++)
		{
			if(pos < 300)
			{
				rectfill(eof_window_editor->screen, 20 + eof_song->vocal_track->line[i].start_pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[0] - 2 + 8, 20 + eof_song->vocal_track->line[i].end_pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[0] + 2 + 8, (eof_song->vocal_track->line[i].flags & EOF_LYRIC_LINE_FLAG_OVERDRIVE) ? makecol(64, 128, 64) : makecol(0, 0, 127));
			}
			else
			{
				rectfill(eof_window_editor->screen, 20 - (pos - 300) + eof_song->vocal_track->line[i].start_pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[0] - 2 + 8, 20 - (pos - 300) + eof_song->vocal_track->line[i].end_pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[0] + 2 + 8, (eof_song->vocal_track->line[i].flags & EOF_LYRIC_LINE_FLAG_OVERDRIVE) ? makecol(64, 128, 64) : makecol(0, 0, 127));
			}
		}
		
		for(i = 0; i < eof_song->vocal_track->lyrics; i++)
		{
			if((eof_input_mode == EOF_INPUT_PIANO_ROLL || eof_input_mode == EOF_INPUT_REX) && eof_music_paused)
			{
				if(eof_hover_note == i)
				{
					eof_lyric_draw(eof_song->vocal_track->lyric[i], (eof_selection.track == EOF_TRACK_VOCALS && eof_selection.multi[i] && eof_music_paused) ? 1 : i == eof_hover_note ? 2 : 3);
				}
				else
				{
					eof_lyric_draw(eof_song->vocal_track->lyric[i], (eof_selection.track == EOF_TRACK_VOCALS && eof_selection.multi[i] && eof_music_paused) ? 1 : i == eof_hover_note ? 2 : 0);
				}
			}
			else
			{
				eof_lyric_draw(eof_song->vocal_track->lyric[i], (eof_selection.track == EOF_TRACK_VOCALS && eof_selection.multi[i] && eof_music_paused) ? 1 : i == eof_hover_note ? 2 : 0);
			}
		}
		if(eof_music_paused && eof_pen_visible && eof_pen_note.pos < eof_music_length)
		{
			if(!eof_mouse_drug)
			{
				if(eof_input_mode == EOF_INPUT_PIANO_ROLL || eof_input_mode == EOF_INPUT_REX)
				{
					eof_lyric_draw(&eof_pen_lyric, 3);
				}
				else
				{
					eof_lyric_draw(&eof_pen_lyric, 0);
				}
			}
		}
	
		/* draw mini keyboard */
		int kcol, kcol2;
		int n;
		int ny;
		int red = 0;
//		rectfill(eof_window_editor->screen, 0, EOF_EDITOR_RENDER_OFFSET + 25, 19, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 1, eof_color_gray);
		for(i = 0; i < eof_screen_layout.vocal_view_size; i++)
		{
			n = (eof_vocals_offset + i) % 12;
			ny = EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.vocal_y - (i % eof_screen_layout.vocal_view_size + 1) * eof_screen_layout.vocal_tail_size;
			if(eof_vocals_offset + i == eof_pen_lyric.note)
			{
				kcol = makecol(0, 255, 0);
				kcol2 = makecol(0, 192, 0);
				if(n == 0 && !red)
				{
					red = 1;
				}
				textprintf_ex(eof_window_editor->screen, font, 21, ny + eof_screen_layout.vocal_tail_size / 2 - text_height(font) / 2, eof_color_white, eof_color_black, "%s", eof_get_tone_name(eof_pen_lyric.note));
			}
			else if(n == 1 || n == 3 || n == 6 || n == 8 || n == 10)
			{
				kcol = makecol(16, 16, 16);
				kcol2 = makecol(0, 0, 0);
			}
			else if(n == 0)
			{
				kcol = makecol(255, 160, 160);
				kcol2 = makecol(192, 96, 96);
				red = 1;
			}
			else
			{
				kcol = makecol(255, 255, 255);
				kcol2 = makecol(192, 192, 192);
			}
			rectfill(eof_window_editor->screen, 0, ny, 19, ny + eof_screen_layout.vocal_tail_size - 1, kcol);
			hline(eof_window_editor->screen, 0, ny + eof_screen_layout.vocal_tail_size - 1, 19, kcol2);
		}
		
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
		
		/* draw the end of song position if necessary */
		if(eof_music_length != eof_music_actual_length)
		{
			if(pos < 300)
			{
				vline(eof_window_editor->screen, 20 + (eof_music_actual_length) / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 20, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h + 4, makecol(255, 0, 0));
			}
			else
			{
				vline(eof_window_editor->screen, 20 - ((pos - 300)) + (eof_music_actual_length) / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 20, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h + 4, makecol(255, 0, 0));
			}
		}
		
		for(i = 0; i < 5; i++)
		{
			if(i == eof_vocals_tab)
			{
				textprintf_ex(eof_window_editor->screen, font, 20 + i * 80, 2 + 8, eof_color_black, -1, "%s", eof_vocal_tab_name[i]);
			}
			else
			{
				textprintf_ex(eof_window_editor->screen, font, 20 + i * 80, 2 + 2 + 8, makecol(128, 128, 128), -1, "%s", eof_vocal_tab_name[i]);
			}
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
