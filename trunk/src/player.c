#include <allegro.h>
#include "menu/song.h"
#include "main.h"
#include "dialog.h"
#include "mix.h"
#include "player.h"

void eof_music_play(void)
{
	int speed = eof_playback_speed;
	unsigned long i;

	if(eof_music_catalog_playback)
	{
		return;
	}
	eof_music_paused = 1 - eof_music_paused;
	if(KEY_EITHER_CTRL)
	{
		speed = 500;
	}
	else if(key[KEY_D])	//Force full playback speed
	{
		speed = 1000;
		key[KEY_D]=0;
	}
	if(eof_music_paused)
	{
		eof_stop_midi();
		alogg_stop_ogg(eof_music_track);
		eof_music_actual_pos = alogg_get_pos_msecs_ogg(eof_music_track);
		if(eof_play_selection)
		{
			eof_music_seek(eof_music_rewind_pos);
			eof_music_pos = eof_music_rewind_pos;
		}
		else if(!eof_smooth_pos)
		{
			eof_music_pos = eof_music_actual_pos;
		}
	}
	else
	{
		if(key[KEY_S] && (eof_count_selected_notes(NULL, 0) > 0))
		{
			eof_music_end_pos = 0;
			eof_music_rewind_pos = eof_music_length;
			for(i = 0; i < eof_track_get_size(eof_song, eof_selected_track); i++)
			{	//For each note in the current track
				if(eof_selection.multi[i] && (eof_get_note_pos(eof_song, eof_selected_track, i) + eof_get_note_length(eof_song, eof_selected_track, i) > eof_music_end_pos))
				{
					eof_music_end_pos = eof_get_note_pos(eof_song, eof_selected_track, i) + eof_get_note_length(eof_song, eof_selected_track, i);
				}
				if(eof_selection.multi[i] && (eof_get_note_pos(eof_song, eof_selected_track, i) < eof_music_rewind_pos))
				{
					eof_music_rewind_pos = eof_get_note_pos(eof_song, eof_selected_track, i);
				}
			}
			eof_music_seek(eof_music_rewind_pos);
			eof_play_selection = 1;
		}
		else
		{
			eof_play_selection = 0;
		}
		eof_music_rewind_pos = eof_music_pos;
		/* in Windows, subtracting the buffer size (buffer_size * 2 according to the Allegro manual)
		 * seems to get rid if the stuttering. */
		#ifdef ALLEGRO_WINDOWS
			alogg_seek_abs_msecs_ogg(eof_music_track, eof_music_pos - ((eof_buffer_size * (eof_smooth_pos ? 2 : 1)) * 1000 / alogg_get_wave_freq_ogg(eof_music_track)));
		#else
			alogg_seek_abs_msecs_ogg(eof_music_track, eof_music_pos);
		#endif
		eof_mix_find_claps();

	//Prevent playback as long as the user is holding down guitar/drum buttons
		unsigned long x;
		int held;	//Tracks whether the user is holding down one of the defined controller buttons
		do{
			held = 0;	//Reset this status
			eof_read_controller(&eof_drums);
			eof_read_controller(&eof_guitar);
			for(x = 0; x < 5; x++)
			{
				if((eof_drums.button[x].held) || (eof_guitar.button[x+2].held))
					held = 1;	//User is holding down one of the drum or guitar controller buttons
			}
		}while(held);

		if(alogg_play_ex_ogg(eof_music_track, eof_buffer_size, 255, 128, speed + eof_audio_fine_tune, 0) == ALOGG_OK)
		{
			eof_mix_start(eof_mix_msec_to_sample(alogg_get_pos_msecs_ogg(eof_music_track), alogg_get_wave_freq_ogg(eof_music_track)), speed);
			eof_entering_note_note = NULL;
			eof_entering_note_lyric = NULL;
			eof_entering_note = 0;
			eof_snote = 0;
			alogg_poll_ogg(eof_music_track);
			eof_music_actual_pos = alogg_get_pos_msecs_ogg(eof_music_track);
		}
		else
		{
			allegro_message("Can't play song!");
			eof_music_paused = 1;
		}
	}
}

void eof_catalog_play(void)
{
	if(eof_song->catalog->entries > 0)
	{
		if(!eof_music_paused)
		{
			eof_music_play();
		}
		else if(eof_music_catalog_playback)
		{
			eof_music_catalog_playback = 0;
			eof_music_catalog_pos = eof_song->catalog->entry[eof_selected_catalog_entry].start_pos + eof_av_delay;
			eof_stop_midi();
			alogg_stop_ogg(eof_music_track);
			alogg_seek_abs_msecs_ogg(eof_music_track, eof_music_pos);
		}
		else
		{
			alogg_seek_abs_msecs_ogg(eof_music_track, eof_song->catalog->entry[eof_selected_catalog_entry].start_pos);
			eof_music_catalog_pos = eof_song->catalog->entry[eof_selected_catalog_entry].start_pos + eof_av_delay;
			eof_music_catalog_playback = 1;
			if(alogg_play_ex_ogg(eof_music_track, eof_buffer_size, 255, 128, 1000 + eof_audio_fine_tune, 0) == ALOGG_OK)
			{
				eof_music_actual_pos = alogg_get_pos_msecs_ogg(eof_music_track);
				eof_mix_find_claps();
				eof_mix_start(eof_mix_msec_to_sample(alogg_get_pos_msecs_ogg(eof_music_track), alogg_get_wave_freq_ogg(eof_music_track)), 1000);
			}
			else
			{
				allegro_message("Can't play song!");
			}
		}
	}
}

void eof_music_seek(unsigned long pos)
{
	alogg_seek_abs_msecs_ogg(eof_music_track, pos + eof_av_delay);
	eof_music_pos = pos + eof_av_delay;
	eof_music_actual_pos = eof_music_pos;
	eof_mix_seek(eof_music_actual_pos);
}

void eof_music_rewind(void)
{
	int amount = 0;
	eof_stop_midi();
	if(!eof_music_catalog_playback)
	{
		if(KEY_EITHER_SHIFT)
		{
			amount = 1000;
		}
		else if(KEY_EITHER_CTRL)
		{
			amount = 10;
		}
		else
		{
			amount = 100;
		}
		if(eof_music_pos - eof_av_delay < amount)
		{
			eof_menu_song_seek_start();
		}
		else
		{
			alogg_seek_rel_msecs_ogg(eof_music_track, -amount);
			eof_music_actual_pos = alogg_get_pos_msecs_ogg(eof_music_track);
			eof_music_pos = eof_music_actual_pos;
			eof_mix_seek(eof_music_actual_pos);
		}
	}
}

void eof_music_forward(void)
{
	eof_stop_midi();
	if(!eof_music_catalog_playback)
	{
		if(KEY_EITHER_SHIFT)
		{
			alogg_seek_rel_msecs_ogg(eof_music_track, 1000);
		}
		else if(KEY_EITHER_CTRL)
		{
			alogg_seek_rel_msecs_ogg(eof_music_track, 10);
		}
		else
		{
			alogg_seek_rel_msecs_ogg(eof_music_track, 100);
		}
		eof_music_actual_pos = alogg_get_pos_msecs_ogg(eof_music_track);
		eof_music_pos = eof_music_actual_pos;
		eof_mix_seek(eof_music_actual_pos);
	}
}
