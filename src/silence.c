#include <allegro.h>
#include "main.h"
#include "utility.h"

static unsigned long msec_to_samples(unsigned long msec)
{
	unsigned long sample;
	double second = (double)msec / (double)1000.0;
	unsigned long freq = alogg_get_wave_freq_ogg(eof_music_track);

	sample = (unsigned long)(second * (double)freq);
	return sample;
}

static SAMPLE * create_silence_sample(unsigned long ms)
{
	SAMPLE * sp = NULL;
	int bits;
	int stereo;
	int freq;
	unsigned long samples;
	int i;
	
	if(eof_music_track)
	{
		bits = alogg_get_wave_bits_ogg(eof_music_track);
		stereo = alogg_get_wave_is_stereo_ogg(eof_music_track);
		freq = alogg_get_wave_freq_ogg(eof_music_track);
		samples = msec_to_samples(ms);
		sp = create_sample(bits, stereo, freq, samples);
		if(sp)
		{
			if(bits == 8)
			{
				for(i = 0; i < samples; i++)
				{
					((unsigned char *)(sp->data))[i] = 0x80;
				}
			}
			else
			{
				for(i = 0; i < samples; i++)
				{
					((unsigned short *)(sp->data))[i] = 0x8000;
				}
			}
		}
	}
	return sp;
}

/* fill this in with a WAV saving routine (Allegro does not supply one for some
 * reason */
static int save_wav(const char * fn, SAMPLE * sp)
{
	return 0;
}

int eof_add_silence(const char * oggfn, unsigned long ms)
{
	char sys_command[1024] = {0};
	char backupfn[1024] = {0};
	char wavfn[1024] = {0};
	char soggfn[1024] = {0};
	SAMPLE * sp = create_silence_sample(ms);
	
	if(sp)
	{
		replace_filename(wavfn, eof_song_path, "silence.wav", 1024);
		
		if(!save_wav(wavfn, sp))
		{
			printf("error saving wav\n");
			return 0;
		}
		destroy_sample(sp);
		
		/* back up original file */
		sprintf(backupfn, "%s.backup", oggfn);
		eof_copy_file((char *)oggfn, backupfn);
		
		/* encode the silence */
		replace_filename(soggfn, eof_song_path, "silence.ogg", 1024);
		sprintf(sys_command, "oggenc -o \"%s\" -q %s \"%s\"", soggfn, eof_ogg_quality[(int)eof_ogg_setting], wavfn);
		if(system(sys_command) != 0)
		{
			return 0;
		}
		
		/* stitch the original file to the silence file */
		sprintf(sys_command, "oggCat \"%s\" \"%s\" \"%s\"", oggfn, soggfn, backupfn);
		if(system(sys_command))
		{
			return 0;
		}
		
		/* clean up */
		delete_file(soggfn);
		delete_file(wavfn);
		if(eof_load_ogg((char *)oggfn))
		{
			return 1;
		}
	}
	return 0;
}
