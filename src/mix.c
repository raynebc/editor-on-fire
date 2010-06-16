#include <allegro.h>
#include "main.h"
#include "utility.h"
#include "beat.h"
#include "mix.h"

//AUDIOSTREAM * eof_mix_stream = NULL;
SAMPLE *    eof_sound_clap = NULL;
SAMPLE *    eof_sound_metronome = NULL;
SAMPLE *    eof_sound_note[EOF_MAX_VOCAL_TONES] = {NULL};
EOF_MIX_VOICE eof_voice[EOF_MIX_MAX_CHANNELS];
//int eof_mix_freq = 44100;
//int eof_mix_buffer_size = 4096;
char eof_mix_claps_enabled = 0;
char eof_mix_metronome_enabled = 0;
char eof_mix_claps_note = 31; /* enable all by default */
char eof_mix_vocal_tones_enabled = 0;

int           eof_mix_speed = 1000;
char          eof_mix_speed_ticker;
double        eof_mix_sample_count = 0.0;
double        eof_mix_sample_increment = 1.0;
unsigned long eof_mix_next_clap;
unsigned long eof_mix_next_metronome;
unsigned long eof_mix_next_note;

unsigned long eof_mix_clap_pos[EOF_MAX_NOTES] = {0};
int eof_mix_claps = 0;
int eof_mix_current_clap = 0;

unsigned long eof_mix_note_pos[EOF_MAX_NOTES] = {0};
unsigned long eof_mix_note_note[EOF_MAX_NOTES] = {0};
int eof_mix_notes = 0;
int eof_mix_current_note = 0;

unsigned long eof_mix_metronome_pos[EOF_MAX_BEATS] = {0};
int eof_mix_metronomes = 0;
int eof_mix_current_metronome = 0;

void eof_mix_callback(void * buf, int length)
{
	unsigned long bytes_left = length / 2;
	unsigned short * buffer = (unsigned short *)buf;
	unsigned long sum;
	unsigned long prod;
	unsigned long final;
	int i, j;
//	bytes_left = eof_mix_buffer_size / 4;
	int increment = alogg_get_wave_is_stereo_ogg(eof_music_track) ? 2 : 1;

	/* add audio data to the buffer */
	for(i = 0; i < bytes_left; i += increment)
	{

		/* mix voices */
		for(j = 0; j < EOF_MIX_MAX_CHANNELS; j++)
		{
			if(eof_voice[j].playing)
			{
				sum = buffer[i] + ((unsigned short *)(eof_voice[j].sp->data))[(unsigned long)eof_voice[j].pos];
				prod = (buffer[i]) * (((unsigned short *)(eof_voice[j].sp->data))[(unsigned long)eof_voice[j].pos]);
				if(buffer[i] < 32768 && ((unsigned short *)(eof_voice[j].sp->data))[(unsigned long)eof_voice[j].pos] < 32768)
				{
					final = prod >> 15;
				}
				else
				{
					final = 2 * sum - (prod >> 15) - 65536;
				}
				buffer[i] = final > 65535 ? 65535 : final;
				if(increment > 1)
				{
					sum = buffer[i + 1] + ((unsigned short *)(eof_voice[j].sp->data))[(unsigned long)eof_voice[j].pos];
					prod = (buffer[i + 1]) * (((unsigned short *)(eof_voice[j].sp->data))[(unsigned long)eof_voice[j].pos]);
					if(buffer[i + 1] < 32768 && ((unsigned short *)(eof_voice[j].sp->data))[(unsigned long)eof_voice[j].pos] < 32768)
					{
						final = prod >> 15;
					}
					else
					{
						final = 2 * sum - (prod >> 15) - 65536;
					}
					buffer[i + 1] = final > 65535 ? 65535 : final;
				}
				eof_voice[j].pos += eof_mix_sample_increment;
				if(eof_voice[j].pos >= eof_voice[j].sp->len)
				{
					eof_voice[j].playing = 0;
				}
			}
		}

		/* increment the sample and check sound triggers */
		eof_mix_sample_count += 1.0;
		if(eof_mix_next_clap >= 0 && eof_mix_sample_count >= eof_mix_next_clap && eof_mix_current_clap < eof_mix_claps)
		{
			if(eof_mix_claps_enabled)
			{
				eof_voice[0].sp = eof_sound_clap;
				eof_voice[0].pos = 0.0;
				eof_voice[0].playing = 1;
			}
			eof_mix_current_clap++;
			eof_mix_next_clap = eof_mix_clap_pos[eof_mix_current_clap];
		}
		if(eof_mix_next_metronome >= 0 && eof_mix_sample_count >= eof_mix_next_metronome && eof_mix_current_metronome < eof_mix_metronomes)
		{
			if(eof_mix_metronome_enabled)
			{
				eof_voice[1].sp = eof_sound_metronome;
				eof_voice[1].pos = 0.0;
				eof_voice[1].playing = 1;
			}
			eof_mix_current_metronome++;
			eof_mix_next_metronome = eof_mix_metronome_pos[eof_mix_current_metronome];
		}
		if(eof_mix_next_note >= 0 && eof_mix_sample_count >= eof_mix_next_note && eof_mix_current_note < eof_mix_notes)
		{
			if(eof_mix_vocal_tones_enabled && eof_sound_note[eof_mix_note_note[eof_mix_current_note]])
			{
				eof_voice[2].sp = eof_sound_note[eof_mix_note_note[eof_mix_current_note]];
				eof_voice[2].pos = 0.0;
				eof_voice[2].playing = 1;
			}
			eof_mix_current_note++;
			eof_mix_next_note = eof_mix_note_pos[eof_mix_current_note];
		}
	}
	eof_just_played = 1;
}

unsigned long eof_mix_msec_to_sample(unsigned long msec, int freq)
{
	unsigned long sample;
	double second = (double)msec / (double)1000.0;

	sample = (unsigned long)(second * (double)freq);
	return sample;
}

/*Unused function, it may have malfunctioned anyway, because it divided seconds by 1000 and casted
 to integer from floating point, which would not have returned the correct millisecond time value?
unsigned long eof_mix_sample_to_msec(unsigned long sample, int freq)
{
	unsigned long msec;
	double second = (double)sample / (double)freq;

	msec = (unsigned long)(second / (double)1000.0);
	return msec;
}
*/

void eof_mix_find_claps(void)
{
	int i;

	eof_mix_claps = 0;
	eof_mix_current_clap = 0;
	if(eof_vocals_selected)
	{
		for(i = 0; i < eof_song->vocal_track->lyrics; i++)
		{
			eof_mix_clap_pos[eof_mix_claps] = eof_mix_msec_to_sample(eof_song->vocal_track->lyric[i]->pos, alogg_get_wave_freq_ogg(eof_music_track));
			eof_mix_claps++;
		}
	}
	else
	{
		for(i = 0; i < eof_song->track[eof_selected_track]->notes; i++)
		{
			if(eof_song->track[eof_selected_track]->note[i]->type == eof_note_type && (eof_song->track[eof_selected_track]->note[i]->note & eof_mix_claps_note))
			{
				eof_mix_clap_pos[eof_mix_claps] = eof_mix_msec_to_sample(eof_song->track[eof_selected_track]->note[i]->pos, alogg_get_wave_freq_ogg(eof_music_track));
				eof_mix_claps++;
			}
		}
	}

	eof_mix_metronomes = 0;
	eof_mix_current_metronome = 0;
	for(i = 0; i < eof_song->beats; i++)
	{
		eof_mix_metronome_pos[eof_mix_metronomes] = eof_mix_msec_to_sample(eof_song->beat[i]->pos, alogg_get_wave_freq_ogg(eof_music_track));
		eof_mix_metronomes++;
	}

	eof_mix_notes = 0;
	eof_mix_current_note = 0;
	for(i = 0; i < eof_song->vocal_track->lyrics; i++)
	{
		eof_mix_note_pos[eof_mix_notes] = eof_mix_msec_to_sample(eof_song->vocal_track->lyric[i]->pos, alogg_get_wave_freq_ogg(eof_music_track));
		eof_mix_note_note[eof_mix_notes] = eof_song->vocal_track->lyric[i]->note;
		eof_mix_notes++;
	}
}

void eof_mix_init(void)
{
	int i;
	char fbuffer[1024] = {0};
	ALOGG_OGG * temp_ogg = NULL;
	char * buffer = NULL;

	eof_sound_clap = load_wav("eof.dat#clap.wav");
	if(!eof_sound_clap)
	{
		allegro_message("Couldn't load clap sound!");
	}
	eof_sound_metronome = load_wav("eof.dat#metronome.wav");
	if(!eof_sound_metronome)
	{
		allegro_message("Couldn't load metronome sound!");
	}
	for(i = 0; i < EOF_MAX_VOCAL_TONES; i++)
	{
		sprintf(fbuffer, "eof.dat#piano.esp/NOTE_%02d_OGG", i);
		buffer = eof_buffer_file(fbuffer);
		if(buffer)
		{
			temp_ogg = alogg_create_ogg_from_buffer(buffer, file_size_ex(fbuffer));
			if(temp_ogg)
			{
				eof_sound_note[i] = alogg_create_sample_from_ogg(temp_ogg);
				alogg_destroy_ogg(temp_ogg);
			}
			else
			{
				printf("sound load error\n");
			}
		}
	}
	alogg_set_buffer_callback(eof_mix_callback);
}

void eof_mix_exit(void)
{
	int i;

	destroy_sample(eof_sound_clap);
	eof_sound_clap=NULL;
	destroy_sample(eof_sound_metronome);
	eof_sound_metronome=NULL;

	for(i = 0; i < EOF_MAX_VOCAL_TONES; i++)
	{
		if(eof_sound_note[i] != NULL)
		{
			destroy_sample(eof_sound_note[i]);
			eof_sound_note[i]=NULL;
		}
	}
}

void eof_mix_start_helper(void)
{
	int i;

	eof_mix_current_clap = -1;
	eof_mix_next_clap = -1;
	for(i = 0; i < eof_mix_claps; i++)
	{
		if(eof_mix_clap_pos[i] >= eof_mix_sample_count)
		{
			eof_mix_current_clap = i;
			eof_mix_next_clap = eof_mix_clap_pos[i];
			break;
		}
	}
	eof_mix_current_metronome = -1;
	eof_mix_next_metronome = -1;
	for(i = 0; i < eof_mix_metronomes; i++)
	{
		if(eof_mix_metronome_pos[i] >= eof_mix_sample_count)
		{
			eof_mix_current_metronome = i;
			eof_mix_next_metronome = eof_mix_metronome_pos[i];
			break;
		}
	}
	eof_mix_current_note = -1;
	eof_mix_next_note = -1;
	for(i = 0; i < eof_mix_notes; i++)
	{
		if(eof_mix_note_pos[i] >= eof_mix_sample_count)
		{
			eof_mix_current_note = i;
			eof_mix_next_note = eof_mix_note_pos[i];
			break;
		}
	}
}

void eof_mix_start(unsigned long start, int speed)
{
	int i;

	eof_mix_next_clap = -1;
	eof_mix_next_metronome = -1;
	for(i = 0; i < EOF_MIX_MAX_CHANNELS; i++)
	{
		eof_voice[i].sp = NULL;
		eof_voice[i].pos = 0.0;
		eof_voice[i].playing = 0;
	}
	eof_mix_speed = speed;
	eof_mix_speed_ticker = 0;
	eof_mix_sample_count = start;
	eof_mix_sample_increment = (1000.0 / (float)eof_mix_speed) * (44100.0 / (float)alogg_get_wave_freq_ogg(eof_music_track));
	eof_mix_start_helper();
}

void eof_mix_seek(int pos)
{
	int i;

	eof_mix_next_clap = -1;
	eof_mix_next_metronome = -1;

	eof_mix_sample_count = eof_mix_msec_to_sample(pos, alogg_get_wave_freq_ogg(eof_music_track));
	for(i = 0; i < eof_mix_claps; i++)
	{
		if(eof_mix_clap_pos[i] >= eof_mix_sample_count)
		{
			eof_mix_current_clap = i;
			eof_mix_next_clap = eof_mix_clap_pos[i];
			break;
		}
	}
	for(i = 0; i < eof_mix_metronomes; i++)
	{
		if(eof_mix_metronome_pos[i] >= eof_mix_sample_count)
		{
			eof_mix_current_metronome = i;
			eof_mix_next_metronome = eof_mix_metronome_pos[i];
			break;
		}
	}
	for(i = 0; i < eof_mix_notes; i++)
	{
		if(eof_mix_note_pos[i] >= eof_mix_sample_count)
		{
			eof_mix_current_note = i;
			eof_mix_next_note = eof_mix_note_pos[i];
			break;
		}
	}
}

void eof_mix_play_note(int note)
{
	if((note < EOF_MAX_VOCAL_TONES) && eof_sound_note[note])
	{
		play_sample(eof_sound_note[note], 255, 127, 1000 + eof_audio_fine_tune, 0);
	}
}
