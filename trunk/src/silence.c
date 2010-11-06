#include <allegro.h>
#include "menu/song.h"
#include "main.h"
#include "waveform.h"
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
	int channels;
	int i;

	if(eof_music_track)
	{
		bits = alogg_get_wave_bits_ogg(eof_music_track);
		stereo = alogg_get_wave_is_stereo_ogg(eof_music_track);
		freq = alogg_get_wave_freq_ogg(eof_music_track);
		samples = msec_to_samples(ms);
		channels = stereo ? 2 : 1;
		sp = create_sample(bits, stereo, freq, samples);
		if(sp)
		{
			if(bits == 8)
			{
				for(i = 0; i < samples * channels; i++)
				{
					((unsigned char *)(sp->data))[i] = 0x80;
				}
			}
			else
			{
				for(i = 0; i < samples * channels; i++)
				{
					((unsigned short *)(sp->data))[i] = 0x8000;
				}
			}
		}
	}
	return sp;
}

/* saves a wave file to file pointer */
static int save_wav_fp(SAMPLE * sp, PACKFILE * fp)
{
	size_t channels, bits, freq;
	size_t data_size;
	size_t samples;
	size_t i, n;
	int val;

	channels = sp->stereo ? 2 : 1;
	bits = sp->bits;
	void * pval = NULL;

	if(channels < 1 || channels > 2)
	{
		return 0;
	}

	samples = sp->len;
	data_size = samples * channels * (bits / 8);
	n = samples * channels;

	freq = sp->freq;

	pack_fputs("RIFF", fp);
	pack_iputl(36 + data_size, fp);
	pack_fputs("WAVE", fp);

	pack_fputs("fmt ", fp);
	pack_iputl(16, fp);
	pack_iputw(1, fp);
	pack_iputw(channels, fp);
	pack_iputl(freq, fp);
	pack_iputl(freq * channels * (bits / 8), fp);	//ByteRate = SampleRate * NumChannels * BitsPerSample/8
	pack_iputw(channels * (bits / 8), fp);
	pack_iputw(bits, fp);

	pack_fputs("data", fp);
	pack_iputl(data_size, fp);

	if (bits == 8)
	{
		pval = sp->data;
		pack_fwrite(pval, samples * channels, fp);
	}
	else if (bits == 16)
	{
		pval = sp->data;
		unsigned short *data = pval;
		for (i = 0; i < n; i++)
		{
			pack_iputw((short)(data[i] - 0x8000), fp);
		}
	}
	else
	{
		TRACE("Unknown audio depth (%d) when saving wav ALLEGRO_FILE.\n", val);
		return 0;
	}

	return 1;
}


/* fill this in with a WAV saving routine (Allegro does not supply one for some
 * reason */
static int save_wav(const char * fn, SAMPLE * sp)
{
    PACKFILE * file;

    /* open file */
    file = pack_fopen(fn, "w");
    if(file == NULL)
    {
        return 0;
    }

    /* save WAV to the file */
    if(!save_wav_fp(sp, file))
    {
        return 0;
    }

    /* close the file */
    pack_fclose(file);

    return 1;
}

int eof_add_silence(const char * oggfn, unsigned long ms)
{
	char sys_command[1024] = {0};
	char backupfn[1024] = {0};
	char wavfn[1024] = {0};
	char soggfn[1024] = {0};
	if(ms == 0)
	{
		return 0;
	}

	set_window_title("Adjusting Silence...");

	/* back up original file */
	sprintf(backupfn, "%s.backup", oggfn);
	if(!exists(backupfn))
	{
		eof_copy_file((char *)oggfn, backupfn);
	}
	delete_file(oggfn);

	/* encode the silence */
/*	replace_filename(soggfn, eof_song_path, "silence.ogg", 1024);
	sprintf(sys_command, "oggSilence -d %d -l%lu -o \"%s\"", alogg_get_bitrate_ogg(eof_music_track), ms, soggfn);
	if(system(sys_command))
	{
		eof_copy_file(backupfn, (char *)oggfn);
		eof_fix_window_title();
		return 0;
	} */
	SAMPLE * silence_sample = create_silence_sample(ms);
	if(!silence_sample)
	{
		eof_fix_window_title();
		return 0;
	}
	replace_filename(wavfn, eof_song_path, "silence.wav", 1024);
	save_wav(wavfn, silence_sample);
	destroy_sample(silence_sample);
	replace_filename(soggfn, eof_song_path, "silence.ogg", 1024);
	#ifdef ALLEGRO_WINDOWS
		sprintf(sys_command, "oggenc2 -o \"%s\" -b %d \"%s\"", soggfn, alogg_get_bitrate_ogg(eof_music_track) / 1000, wavfn);
	#else
		sprintf(sys_command, "oggenc -o \"%s\" -b %d \"%s\"", soggfn, alogg_get_bitrate_ogg(eof_music_track) / 1000, wavfn);
	#endif
	if(system(sys_command))
	{
		eof_fix_window_title();
		return 0;
	}


	/* stitch the original file to the silence file */
	sprintf(sys_command, "oggCat \"%s\" \"%s\" \"%s\"", oggfn, soggfn, backupfn);
	if(system(sys_command))
	{
		eof_copy_file(backupfn, (char *)oggfn);
		eof_fix_window_title();
		return 0;
	}

	/* clean up */
	delete_file(soggfn);
	if(eof_load_ogg((char *)oggfn))
	{
		eof_fix_waveform_graph();
		eof_fix_window_title();
		return 1;
	}
	eof_fix_window_title();
	return 0;
}

int eof_add_silence_recode(const char * oggfn, unsigned long ms)
{
	char sys_command[1024] = {0};
	char backupfn[1024] = {0};
	char wavfn[1024] = {0};
	char soggfn[1024] = {0};
	ALOGG_OGG *oggfile=NULL;
	SAMPLE *decoded=NULL,*combined=NULL;
	FILE *fp=NULL;
	int bits;
	int stereo;
	int freq;
	unsigned long samples;
	int channels;
	unsigned long ctr,index;

	if(ms == 0)
	{
		return 0;
	}
	set_window_title("Adjusting Silence...");

	/* back up original file */
	sprintf(backupfn, "%s.backup", oggfn);
	if(!exists(backupfn))
	{
		eof_copy_file((char *)oggfn, backupfn);
	}

	/* Decode the OGG file into memory */
	//Load OGG file into memory
	fp=fopen(oggfn,"rb");
	if(fp == NULL)
		return 0;	//Return failure
	oggfile=alogg_create_ogg_from_file(fp);
	if(oggfile == NULL)
	{
		fclose(fp);
		return 0;	//Return failure
	}
	//Decode OGG into memory
	decoded=alogg_create_sample_from_ogg(oggfile);
	if(decoded == NULL)
	{
		alogg_destroy_ogg(oggfile);
		fclose(fp);
		return 0;	//Return failure
	}

	/* Create a SAMPLE array large enough for the leading silence and the decoded OGG */
	bits = alogg_get_wave_bits_ogg(oggfile);
	stereo = alogg_get_wave_is_stereo_ogg(oggfile);
	freq = alogg_get_wave_freq_ogg(oggfile);
	alogg_destroy_ogg(oggfile);	//This is no longer needed
	oggfile = NULL;
	samples = msec_to_samples(ms);
	channels = stereo ? 2 : 1;
	combined = create_sample(bits,stereo,freq,samples+decoded->len);	//Create a sample array long enough for the silence and the OGG file
	if(combined == NULL)
	{
		destroy_sample(decoded);
		fclose(fp);
		return 0;	//Return failure
	}

	/* Add the PCM data for the silence */
	if(bits == 8)
	{	//Create 8 bit PCM data
		for(ctr=0,index=0;ctr < samples * channels;ctr++)
		{
			((unsigned char *)(combined->data))[index++] = 0x80;
		}
	}
	else
	{	//Create 16 bit PCM data
		for(ctr=0,index=0;ctr < samples * channels;ctr++)
		{
			((unsigned short *)(combined->data))[index++] = 0x8000;
		}
	}

	/* Add the decoded OGG PCM data*/
	if(bits == 8)
	{	//Copy 8 bit PCM data
		for(ctr=0;ctr < decoded->len;ctr++)
		{
			((unsigned char *)(combined->data))[index++] = ((unsigned char *)(decoded->data))[ctr];
		}
	}
	else
	{	//Copy 16 bit PCM data
		for(ctr=0;ctr < decoded->len;ctr++)
		{
			((unsigned short *)(combined->data))[index++] = ((unsigned short *)(decoded->data))[ctr];
		}
	}

	/* encode the audio */
	destroy_sample(decoded);	//This is no longer needed
	replace_filename(wavfn, eof_song_path, "encode.wav", 1024);
	printf("%s\n%s\n", eof_song_path, wavfn);
	save_wav(wavfn, combined);
	destroy_sample(combined);	//This is no longer needed
	replace_filename(soggfn, eof_song_path, "encode.ogg", 1024);
	#ifdef ALLEGRO_WINDOWS
		sprintf(sys_command, "oggenc2 -o \"%s\" -b %d \"%s\"", soggfn, alogg_get_bitrate_ogg(eof_music_track) / 1000, wavfn);
	#else
		sprintf(sys_command, "oggenc -o \"%s\" -b %d \"%s\"", soggfn, alogg_get_bitrate_ogg(eof_music_track) / 1000, wavfn);
	#endif
	if(system(sys_command))
	{
		eof_fix_window_title();
		return 0;
	}

	/* replace the current OGG file with the new file */
	eof_copy_file(soggfn, (char *)oggfn);	//Copy encode.ogg to the filename of the original OGG

	/* clean up */
	delete_file(soggfn);	//Delete encode.ogg
	if(eof_load_ogg((char *)oggfn))
	{
		eof_fix_waveform_graph();
		eof_fix_window_title();
		return 1;
	}
	eof_fix_window_title();
	return 0;
}

int eof_add_silence_recode_mp3(const char * oggfn, unsigned long ms)
{
	char sys_command[1024] = {0};
	char backupfn[1024] = {0};
	char wavfn[1024] = {0};
	char soggfn[1024] = {0};
	char mp3fn[1024] = {0};
	SAMPLE * decoded = NULL;
	SAMPLE * combined = NULL;
	int bits;
	int stereo;
	int freq;
	unsigned long samples;
	int channels;
	unsigned long ctr,index;

	if(ms == 0)
	{
		return 0;
	}
	set_window_title("Adjusting Silence...");

	/* back up original file */
	sprintf(backupfn, "%s.backup", oggfn);
	if(!exists(backupfn))
	{
		eof_copy_file((char *)oggfn, backupfn);
	}

	/* decode MP3 */
	replace_filename(wavfn, eof_song_path, "decode.wav", 1024);
	replace_filename(mp3fn, eof_song_path, "original.mp3", 1024);
	sprintf(sys_command, "lame --decode \"%s\" \"%s\"", mp3fn, wavfn);
	system(sys_command);

	/* insert silence */
	decoded = load_sample(wavfn);
	bits = decoded->bits;
	stereo = decoded->stereo;
	freq = decoded->freq;
	samples = msec_to_samples(ms);
	channels = stereo ? 2 : 1;
	combined = create_sample(bits,stereo,freq,samples+decoded->len);	//Create a sample array long enough for the silence and the OGG file
	if(combined == NULL)
	{
		destroy_sample(decoded);
		return 0;	//Return failure
	}
	/* Add the PCM data for the silence */
	if(bits == 8)
	{	//Create 8 bit PCM data
		for(ctr=0,index=0;ctr < samples * channels;ctr++)
		{
			((unsigned char *)(combined->data))[index++] = 0x80;
		}
	}
	else
	{	//Create 16 bit PCM data
		for(ctr=0,index=0;ctr < samples * channels;ctr++)
		{
			((unsigned short *)(combined->data))[index++] = 0x8000;
		}
	}

	/* Add the decoded OGG PCM data*/
	if(bits == 8)
	{	//Copy 8 bit PCM data
		for(ctr=0;ctr < decoded->len;ctr++)
		{
			((unsigned char *)(combined->data))[index++] = ((unsigned char *)(decoded->data))[ctr];
		}
	}
	else
	{	//Copy 16 bit PCM data
		for(ctr=0;ctr < decoded->len;ctr++)
		{
			((unsigned short *)(combined->data))[index++] = ((unsigned short *)(decoded->data))[ctr];
		}
	}

	/* save combined WAV */
	replace_filename(wavfn, eof_song_path, "encode.wav", 1024);
	save_wav(wavfn, combined);

	/* destroy samples */
	destroy_sample(decoded);	//This is no longer needed
	destroy_sample(combined);	//This is no longer needed

	/* encode the audio */
	printf("%s\n%s\n", eof_song_path, wavfn);
	replace_filename(soggfn, eof_song_path, "encode.ogg", 1024);
	#ifdef ALLEGRO_WINDOWS
		sprintf(sys_command, "oggenc2 -o \"%s\" -b %d \"%s\"", soggfn, alogg_get_bitrate_ogg(eof_music_track) / 1000, wavfn);
	#else
		sprintf(sys_command, "oggenc -o \"%s\" -b %d \"%s\"", soggfn, alogg_get_bitrate_ogg(eof_music_track) / 1000, wavfn);
	#endif

	if(system(sys_command))
	{
		eof_fix_window_title();
		return 0;
	}

	/* replace the current OGG file with the new file */
	eof_copy_file(soggfn, (char *)oggfn);	//Copy encode.ogg to the filename of the original OGG

	/* clean up */
	replace_filename(wavfn, eof_song_path, "decode.wav", 1024);
	delete_file(wavfn);		//Delete decode.wav
	replace_filename(wavfn, eof_song_path, "encode.wav", 1024);
	delete_file(wavfn);		//Delete encode.wav
	delete_file(soggfn);	//Delete encode.ogg

	if(eof_load_ogg((char *)oggfn))
	{
		eof_fix_waveform_graph();
		eof_fix_window_title();
		return 1;
	}
	eof_fix_window_title();
	return 1;
}
