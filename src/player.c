#include <allegro.h>
#include "menu/song.h"
#include "main.h"
#include "dialog.h"
#include "mix.h"
#include "player.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

void eof_music_play(char resumelastspeed)
{
	static int speed = 1000;		//1000 is 100% playback speed
	unsigned long i;
	int ret, newspeed = 1000;
	int ctrl_held = 0;

	eof_log("eof_music_play() entered", 1);

	if(eof_music_catalog_playback || eof_silence_loaded || !eof_music_track)
	{
		eof_log("\tInvalid playback conditions", 3);
		return;
	}
	eof_music_paused = 1 - eof_music_paused;
	if(!resumelastspeed)
	{	//If the previous playback speed isn't being re-used, determine what speed to play at
		newspeed = eof_playback_speed;	//By default, use the currently configured playback rate
		#ifdef ALLEGRO_MACOSX
			//In OS X, CMD+Space brings up the OS search instead, so allow the use the real CTRL keys for slow playback shortcuts
			if(key[KEY_LCONTROL] || key[KEY_RCONTROL])
			{	//If CTRL is being held
				ctrl_held = 1;
			}
		#endif
		if(KEY_EITHER_CTRL)
			ctrl_held = 1;

		if(ctrl_held)
		{	//If CTRL is being held
			if(KEY_EITHER_SHIFT)
			{	//If SHIFT is also being held
				eof_shift_used = 1;	//Track that the SHIFT key was used
				newspeed = 250;
			}
			else
			{
				newspeed = 500;
			}
		}
		else if(key[KEY_D])	//Force full playback speed
		{
			newspeed = 1000;
			key[KEY_D]=0;
		}
	}
	if(eof_music_paused)
	{
		eof_log("\tStopping playback", 1);
		eof_stop_midi();
		alogg_stop_ogg(eof_music_track);
		eof_music_actual_pos = alogg_get_pos_msecs_ogg_ul(eof_music_track);
		if(eof_play_selection)
		{
			eof_music_seek(eof_music_rewind_pos);
			eof_set_music_pos(&eof_music_pos, eof_music_rewind_pos);
		}
		else if(!eof_smooth_pos)
		{
			eof_set_music_pos(&eof_music_pos, eof_music_actual_pos);
		}
	}
	else if(eof_music_pos.value - eof_av_delay < eof_music_length)
	{	//Otherwise if the seek position is before the end of the audio, begin playback
		unsigned long x;
		int held;	//Tracks whether the user is holding down one of the defined controller buttons

		if(!resumelastspeed)
		{	//If the previous playback speed isn't being re-used, set the speed based on the current playback rate or any keyboard modifiers used
			speed = newspeed;
		}
		if(key[KEY_S] && (eof_count_selected_and_unselected_notes(NULL) > 0))
		{	//If S is being held, and there are selected notes, play back the audio from the first selected note to the last
			eof_music_end_pos = 0;
			eof_music_rewind_pos = eof_chart_length;
			for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
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
		{	//Otherwise just play until the end is reached or user stops playback
			eof_play_selection = 0;
		}
		eof_music_rewind_pos = eof_music_pos.value;
		/* in Windows, subtracting the buffer size (buffer_size * 2 according to the Allegro manual)
		 * seems to get rid of the stuttering. */
		eof_log("\t\tSeeking audio to chart position", 3);
		#ifdef ALLEGRO_WINDOWS
			alogg_seek_abs_msecs_ogg_ul(eof_music_track, eof_music_pos.value - ((eof_buffer_size * (eof_smooth_pos ? 2 : 1)) * 1000 / (unsigned long)alogg_get_wave_freq_ogg(eof_music_track)));
		#else
			alogg_seek_abs_msecs_ogg_ul(eof_music_track, eof_music_pos.value);
		#endif
		eof_mix_find_claps();

	//Prevent playback as long as the user is holding down guitar/drum buttons
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

		eof_log("\t\tCalling OGG playback function", 3);
		if(speed == 1000)
		{	//100% speed playback
			eof_log("\tStarting playback", 1);
			ret = alogg_play_ex_ogg(eof_music_track, eof_buffer_size, 255, 128, speed + eof_audio_fine_tune, 0);
		}
		else
		{
			if(eof_playback_time_stretch)
			{	//Time stretched playback at any speed other than 100%
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tStarting %d%% speed time stretch playback", speed / 10);
				eof_log(eof_log_string, 1);
				ret = alogg_play_ogg_ts(eof_music_track, eof_buffer_size, 255, 128, speed);
			}
			else
			{	//Non time stretched playback at any speed other than 100%
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tStarting %d%% speed playback", speed / 10);
				eof_log(eof_log_string, 1);
				ret = alogg_play_ex_ogg(eof_music_track, eof_buffer_size, 255, 128, speed + eof_audio_fine_tune, 0);
			}
		}

		if(ret == ALOGG_OK)
		{
			eof_mix_start(speed);
			eof_entering_note_note = NULL;
			eof_entering_note_lyric = NULL;
			eof_entering_note = 0;
			eof_snote = 0;
			(void) alogg_poll_ogg(eof_music_track);
			eof_music_actual_pos = alogg_get_pos_msecs_ogg_ul(eof_music_track);
			eof_log("\t\tPlayback started", 3);
		}
		else
		{
			allegro_message("Can't play song!");
			eof_music_paused = 1;
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tPlayback failed (error %d)", ret);
			eof_log(eof_log_string, 3);
		}
	}
	else
	{	//Otherwise ensure chart stays paused
		eof_music_paused = 1;
		eof_log("\t\tPlayback canceled", 3);
	}
}

void eof_catalog_play(void)
{
	eof_log("eof_catalog_play() entered", 1);

	if((eof_song->catalog->entries > 0) && !eof_silence_loaded)
	{	//Only play a catalog entry if there's at least one, and there is chart audio loaded
		if(!eof_music_paused)
		{	//Chart/catalog is playing
			eof_music_play(0);
		}
		else if(eof_music_catalog_playback)
		{	//Catalog is playing, pause it
			eof_music_catalog_playback = 0;
			eof_music_catalog_pos = eof_song->catalog->entry[eof_selected_catalog_entry].start_pos + eof_av_delay;
			eof_stop_midi();
			alogg_stop_ogg(eof_music_track);
			alogg_seek_abs_msecs_ogg_ul(eof_music_track, eof_music_pos.value);
		}
		else
		{	//Nothing is playing
			eof_music_catalog_pos = eof_song->catalog->entry[eof_selected_catalog_entry].start_pos + eof_av_delay;
			eof_log("\t\tSeeking audio to catalog position", 3);
			#ifdef ALLEGRO_WINDOWS
				alogg_seek_abs_msecs_ogg_ul(eof_music_track, eof_music_catalog_pos - ((eof_buffer_size * (eof_smooth_pos ? 2 : 1)) * 1000 / (unsigned long)alogg_get_wave_freq_ogg(eof_music_track)));
			#else
				alogg_seek_abs_msecs_ogg_ul(eof_music_track, eof_music_pos.value);
			#endif

			eof_music_catalog_playback = 1;
			if(alogg_play_ex_ogg(eof_music_track, eof_buffer_size, 255, 128, 1000 + eof_audio_fine_tune, 0) == ALOGG_OK)
			{
				eof_music_actual_pos = alogg_get_pos_msecs_ogg_ul(eof_music_track);
				eof_mix_find_claps();
				eof_mix_start(1000);
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
	eof_log("eof_music_seek() entered", 1);

	alogg_seek_abs_msecs_ogg_ul(eof_music_track, pos + eof_av_delay);
	eof_set_music_pos(&eof_music_pos, pos + eof_av_delay);
	eof_music_actual_pos = eof_music_pos.value;
	eof_mix_seek(eof_music_actual_pos);
	eof_reset_lyric_preview_lines();
}

void eof_music_rewind(void)
{
	unsigned long amount = 0;
//	eof_log("eof_music_rewind() entered", 3);

	eof_stop_midi();
	if(!eof_music_catalog_playback)
	{
		if(KEY_EITHER_SHIFT)
		{
			eof_shift_used = 1;	//Track that the SHIFT key was used
			if(KEY_EITHER_CTRL)
			{	//If both SHIFT and CTRL are being held
				amount = 1;
			}
			else
			{	//Only SHIFT is being held
				amount = 1000;
			}
		}
		else if(KEY_EITHER_CTRL)
		{
			amount = 10;
		}
		else
		{
			amount = 100;
		}
		if(eof_music_pos.value - eof_av_delay < amount)
		{
			(void) eof_menu_song_seek_start();
		}
		else
		{
			eof_set_seek_position(eof_music_pos.value - amount);
		}
	}
}

void eof_music_forward(void)
{
	unsigned long amount = 0, target;
//	eof_log("eof_music_forward() entered", 3);

	eof_stop_midi();
	if(!eof_music_catalog_playback)
	{
		if(KEY_EITHER_SHIFT)
		{
			eof_shift_used = 1;	//Track that the SHIFT key was used
			if(KEY_EITHER_CTRL)
			{	//If both SHIFT and CTRL are being held
				amount = 1;
			}
			else
			{	//Only SHIFT is being held
				amount = 1000;
			}
		}
		else if(KEY_EITHER_CTRL)
		{
			amount = 10;
		}
		else
		{
			amount = 100;
		}
		target = eof_music_pos.value + amount;
		if(target > eof_chart_length)
		{	//If the seek would exceed the end of the chart
			target = eof_chart_length;
		}
		eof_set_seek_position(target);
	}
}
