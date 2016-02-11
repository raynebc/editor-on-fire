#include <allegro.h>
#include "menu/song.h"
#include "main.h"
#include "silence.h"
#include "utility.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

static unsigned long msec_to_samples(unsigned long msec)
{
	unsigned long sample;
	double second = (double)msec / (double)1000.0;
	unsigned long freq = alogg_get_wave_freq_ogg(eof_music_track);

	eof_log("msec_to_samples() entered", 1);

	sample = (unsigned long)(second * (double)freq);
	return sample;
}

SAMPLE * create_silence_sample(unsigned long ms)
{
	SAMPLE * sp = NULL;
	int bits;
	int stereo;
	int freq;
	unsigned long samples;
	int channels;
	int i;

	eof_log("create_silence_sample() entered", 1);

	if(eof_music_track)
	{
		bits = alogg_get_wave_bits_ogg(eof_music_track);
		stereo = alogg_get_wave_is_stereo_ogg(eof_music_track);
		freq = alogg_get_wave_freq_ogg(eof_music_track);
		samples = msec_to_samples(ms);
		channels = stereo ? 2 : 1;
	}
	else
	{
		bits = 16;
		stereo = 1;
		freq = 44100;
		samples = (double)(ms * freq) / 1000.0;
		channels = 2;
	}

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

	return sp;
}

/* saves a wave file to file pointer */
static int save_wav_fp(SAMPLE * sp, PACKFILE * fp)
{
	size_t channels, bits, freq;
	size_t data_size;
	size_t samples;
	size_t i, n;
	void * pval = NULL;
	unsigned short *data;

	eof_log("save_wav_fp() entered", 1);

	if(!sp || !fp)
	{
		return 0;
	}
	channels = sp->stereo ? (size_t)2 : (size_t)1;
	bits = (size_t)sp->bits;

	samples = (size_t)sp->len;
	data_size = samples * channels * (bits / 8);
	n = samples * channels;

	freq = (size_t)sp->freq;

	(void) pack_fputs("RIFF", fp);
	(void) pack_iputl(36 + (long)data_size, fp);
	(void) pack_fputs("WAVE", fp);

	(void) pack_fputs("fmt ", fp);
	(void) pack_iputl(16, fp);
	(void) pack_iputw(1, fp);
	(void) pack_iputw((int)channels, fp);
	(void) pack_iputl((long)freq, fp);
	(void) pack_iputl((long)(freq * channels * (bits / 8)), fp);	//ByteRate = SampleRate * NumChannels * BitsPerSample/8
	(void) pack_iputw((int)(channels * (bits / 8)), fp);
	(void) pack_iputw((int)bits, fp);

	(void) pack_fputs("data", fp);
	(void) pack_iputl((long)data_size, fp);

	if(bits == 8)
	{
		pval = sp->data;
		(void) pack_fwrite(pval, (long)(samples * channels), fp);
	}
	else if(bits == 16)
	{
		data = (unsigned short *)sp->data;
		for (i = 0; i < n; i++)
		{
			(void) pack_iputw((short)(data[i] - 0x8000), fp);
		}
	}
	else
	{
		TRACE("Unknown audio depth (%d) when saving wav ALLEGRO_FILE.\n", bits);
		return 0;
	}

	return 1;
}

int save_wav(const char * fn, SAMPLE * sp)
{
	PACKFILE * file;

	eof_log("save_wav() entered", 1);

	if(!fn || !sp)
	{
		return 0;
	}
	/* open file */
	file = pack_fopen(fn, "w");
	if(file == NULL)
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError saving WAV:  \"%s\"", strerror(errno));	//Get the Operating System's reason for the failure
		eof_log(eof_log_string, 1);
		return 0;
	}

	/* save WAV to the file */
	if(!save_wav_fp(sp, file))
	{
		(void) pack_fclose(file);
		return 0;
	}

	/* close the file */
	(void) pack_fclose(file);

	return 1;
}

int eof_add_silence(const char * oggfn, unsigned long ms)
{
	char sys_command[1024] = {0};
	char backupfn[1024] = {0};	//The file path of the backup of the target audio file
	char wavfn[1024] = {0};		//The file path of the silent WAV file created
	char soggfn[1024] = {0};	//The file path of the silent OGG file created
	char oggcfn[1024] = {0};	//The file path to the oggCat utility
	char old_wd[1024] = {0};	//Store working directory before changing it so we can get back
	char *rel_oggfn;			//Relative file path to the target audio file
	char *rel_backupfn;			//Relative file path to the backup of the target audio file
	SAMPLE * silence_sample;
	int retval;

	if(!oggfn || (ms == 0) || eof_silence_loaded)
	{
		return 1;	//Return error:  Invalid parameters
	}

 	eof_log("eof_add_silence() entered", 1);

	set_window_title("Adjusting Silence...");

	/* back up original file */
	(void) snprintf(backupfn, sizeof(backupfn) - 1, "%s.backup", oggfn);
	if(!exists(backupfn))
	{
		(void) eof_copy_file((char *)oggfn, backupfn);
	}
	(void) delete_file(oggfn);

	silence_sample = create_silence_sample(ms);
	if(!silence_sample)
	{
		eof_fix_window_title();
		return 2;	//Return error:  Couldn't create silent audio
	}
	(void) replace_filename(wavfn, eof_song_path, "silence.wav", 1024);
	(void) save_wav(wavfn, silence_sample);
	destroy_sample(silence_sample);
	(void) replace_filename(soggfn, eof_song_path, "silence.ogg", 1024);
	#ifdef ALLEGRO_WINDOWS
		(void) uszprintf(sys_command, (int) sizeof(sys_command) - 1, "oggenc2 -o \"%s\" -b %d \"%s\"", soggfn, alogg_get_bitrate_ogg(eof_music_track) / 1000, wavfn);
	#else
		(void) uszprintf(sys_command, (int) sizeof(sys_command) - 1, "oggenc -o \"%s\" -b %d \"%s\"", soggfn, alogg_get_bitrate_ogg(eof_music_track) / 1000, wavfn);
	#endif
	if(eof_system(sys_command))
	{
		eof_fix_window_title();
		return 3;	//Return error:  Could not encode silent audio
	}

	/* stitch the original file to the silence file */
	(void) getcwd(old_wd, 1024);
	(void) eof_chdir(eof_song_path);	//Change directory to the project's folder, since oggCat does not support paths that have any Unicode/extended ASCII, relative paths will be given
	#ifdef ALLEGRO_WINDOWS
		get_executable_name(oggcfn, 1024);
		(void) replace_filename(oggcfn, oggcfn, "oggCat.exe", 1024);	//Build the full path to oggCat
	#else
		ustrzcpy(oggcfn, 1024, "oggCat");
	#endif
	rel_oggfn = get_filename(oggfn);		//Get the relative path to the target OGG file
	rel_backupfn = get_filename(backupfn);	//Get the relative path to the backup of the target OGG file
	//Call oggCat while the current working directory is the project folder.  This way, if the project folder's path contains any Unicode or extended ASCII, oggCat won't fail
	#ifdef ALLEGRO_WINDOWS
		//For some reason, the Windows build needs extra quotation marks to enclose the entire command if the command begins with a full executable path in quote marks
		(void) uszprintf(sys_command, (int) sizeof(sys_command) - 1, "\"\"%s\" \"%s\" \"silence.ogg\" \"%s\"\"", oggcfn, rel_oggfn, rel_backupfn);
	#else
		(void) uszprintf(sys_command, (int) sizeof(sys_command) - 1, "\"%s\" \"%s\" \"silence.ogg\" \"%s\"", oggcfn, rel_oggfn, rel_backupfn);	//Use oggCat to overwrite the target OGG file with the silent audio concatenated with the backup of the target OGG file
	#endif
	retval = eof_system(sys_command);

	/* change back to the project directory */
	if(eof_chdir(old_wd))
	{
		allegro_message("Could not change directory to EOF's program folder!\n%s", backupfn);
		return 4;	//Return error:  Could not set working directory
	}

	if(retval)
	{	//If the command failed
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError issuing command \"%s\" from path \"%s\"", sys_command, eof_song_path);
		eof_log(eof_log_string, 1);
		(void) eof_copy_file(backupfn, (char *)oggfn);	//Restore the original OGG file
		eof_fix_window_title();
		return 5;	//Return error:  oggCat failed
	}

	/* clean up */
	(void) delete_file(wavfn);	//Delete silence.wav
	(void) delete_file(soggfn);	//Delete silence.ogg
	if(eof_load_ogg((char *)oggfn, 0))
	{	//If the combined audio was loaded
		eof_fix_waveform_graph();
		eof_fix_spectrogram();
		eof_fix_window_title();
		eof_chart_length = eof_music_length;
		return 0;	//Return success
	}
	eof_fix_window_title();

	//If this part of the function is reached, the OGG failed to load
	if(exists(oggfn))
		return 6;	//Return error:  Could not load new audio, but audio file exists

	return 7;	//Return error:  Could not load new audio, file does not exist
}

int eof_add_silence_recode(const char * oggfn, unsigned long ms)
{
	char sys_command[1024] = {0};
	char backupfn[1024] = {0};
	char wavfn[1024] = {0};
	char soggfn[1024] = {0};
	ALOGG_OGG *oggfile=NULL;
	SAMPLE *decoded=NULL,*combined=NULL;
	int bits;
	int stereo;
	int freq;
	unsigned long samples;
	int channels;
	unsigned long ctr,index;
	void * oggbuffer = NULL;
	int bitrate;

 	eof_log("eof_add_silence_recode() entered", 1);

	if(!oggfn || (ms == 0) || eof_silence_loaded)
	{
		return 41;	//Return failure:  Invalid parameters
	}
	set_window_title("Adjusting Silence...");

	/* back up original file */
	(void) snprintf(backupfn, sizeof(backupfn) - 1, "%s.backup", oggfn);
	if(!exists(backupfn))
	{
		(void) eof_copy_file((char *)oggfn, backupfn);
	}

	/* Decode the OGG file into memory */
	//Load OGG file into memory
	oggbuffer = eof_buffer_file(oggfn, 0);	//Decode the OGG from buffer instead of from file because the latter cannot support special characters in the file path due to limitations with fopen()
	if(!oggbuffer)
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading OGG:  \"%s\"", strerror(errno));	//Get the Operating System's reason for the failure
		eof_log(eof_log_string, 1);
		return 42;	//Return failure:  Could not buffer chart audio into memory
	}
	oggfile=alogg_create_ogg_from_buffer(oggbuffer, (int)file_size_ex(oggfn));
	if(oggfile == NULL)
	{
		eof_log("ALOGG failed to open input audio file", 1);
		free(oggbuffer);
		return 43;	//Return failure:  Could not process buffered chart audio
	}

	//Decode OGG into memory
	decoded=alogg_create_sample_from_ogg(oggfile);
	if(decoded == NULL)
	{
		alogg_destroy_ogg(oggfile);
		free(oggbuffer);
		return 44;	//Return failure:  Could not decode chart audio to memory
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
		return 45;	//Return failure:  Could not create a sample array for the combined audio
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
		for(ctr=0;ctr < decoded->len * channels;ctr++)
		{
			((unsigned char *)(combined->data))[index++] = ((unsigned char *)(decoded->data))[ctr];
		}
	}
	else
	{	//Copy 16 bit PCM data
		for(ctr=0;ctr < decoded->len * channels;ctr++)
		{
			((unsigned short *)(combined->data))[index++] = ((unsigned short *)(decoded->data))[ctr];
		}
	}

	/* encode the audio */
	destroy_sample(decoded);	//This is no longer needed
	free(oggbuffer);
	(void) replace_filename(wavfn, eof_song_path, "encode.wav", 1024);
	(void) save_wav(wavfn, combined);
	destroy_sample(combined);	//This is no longer needed
	(void) replace_filename(soggfn, eof_song_path, "encode.ogg", 1024);
	bitrate = alogg_get_bitrate_ogg(eof_music_track) / 1000;
	if(!bitrate)
	{	//A user found that in an audio file with a really high sample rate (ie. 96KHz), alogg_get_bitrate_ogg() may return zero instead of an expected value
		bitrate = 256;	//In case this happens, use a bitrate of 256Kbps, which should be good enough for a very high quality file
	}
	#ifdef ALLEGRO_WINDOWS
		(void) uszprintf(sys_command, (int) sizeof(sys_command) - 1, "oggenc2 -o \"%s\" -b %d \"%s\"", soggfn, bitrate, wavfn);
	#else
		(void) uszprintf(sys_command, (int) sizeof(sys_command) - 1, "oggenc -o \"%s\" -b %d \"%s\"", soggfn, bitrate, wavfn);
	#endif

	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tCalling oggenc as follows:  %s", sys_command);
	eof_log(eof_log_string, 1);
	if(eof_system(sys_command))
	{	//If oggenc failed, retry again by specifying a quality level (specifying bitrate can fail in some circumstances)
		eof_log("\t\toggenc failed.  Retrying by specifying a quality level instead of a target bitrate", 1);
		#ifdef ALLEGRO_WINDOWS
			(void) uszprintf(sys_command, (int) sizeof(sys_command) - 1, "oggenc2 -o \"%s\" -q 9 \"%s\"", soggfn, wavfn);
		#else
			(void) uszprintf(sys_command, (int) sizeof(sys_command) - 1, "oggenc -o \"%s\" -q 9 \"%s\"", soggfn, wavfn);
		#endif
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tCalling oggenc as follows:  %s", sys_command);
		eof_log(eof_log_string, 1);
		if(eof_system(sys_command))
		{	//If oggenc failed again
			(void) ustrzcat(sys_command, (int) sizeof(sys_command) - 1, " 2> oggenc.log");	//Append a redirection to capture the output of oggenc
			if(eof_system(sys_command))
			{	//Run one last time to catch the error output
				eof_log("\tOggenc failed.  Please see oggenc.log for any errors it gave.", 1);
				eof_fix_window_title();
				return 46;	//Return failure:  Could not encode combined audio
			}
		}
	}

	/* replace the current OGG file with the new file */
	(void) eof_copy_file(soggfn, (char *)oggfn);	//Copy encode.ogg to the filename of the original OGG

	/* clean up */
	(void) delete_file(soggfn);	//Delete encode.ogg
	(void) delete_file(wavfn);		//Delete encode.wav
	if(eof_load_ogg((char *)oggfn, 0))
	{	//If the combined audio was loaded
		eof_fix_waveform_graph();
		eof_fix_spectrogram();
		eof_fix_window_title();
		eof_chart_length = eof_music_length;
		return 0;	//Return success
	}
	eof_fix_window_title();

	return 47;	//Return error:  Could not load new audio
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

 	eof_log("eof_add_silence_recode_mp3() entered", 1);

	if(!oggfn || (ms == 0) || eof_silence_loaded)
	{
		return 21;	//Return error:  Invalid parameters
	}
	set_window_title("Adjusting Silence...");

	/* back up original file */
	(void) snprintf(backupfn, sizeof(backupfn) - 1, "%s.backup", oggfn);
	if(!exists(backupfn))
	{
		(void) eof_copy_file((char *)oggfn, backupfn);
	}

	/* decode MP3 */
	(void) replace_filename(wavfn, eof_song_path, "decode.wav", 1024);
	(void) replace_filename(mp3fn, eof_song_path, "original.mp3", 1024);
	(void) uszprintf(sys_command, (int) sizeof(sys_command) - 1, "lame --decode \"%s\" \"%s\"", mp3fn, wavfn);
	(void) eof_system(sys_command);

	/* insert silence */
	decoded = load_sample(wavfn);
	if(!decoded)
	{
		allegro_message("Error opening file.\nMake sure there are no Unicode or extended ASCII characters in this chart's file path.");
		return 22;	//Return failure:  Could not load decoded MP3 file
	}
	bits = decoded->bits;
	stereo = decoded->stereo;
	freq = decoded->freq;
	samples = (decoded->freq * ms) / 1000;	//Calculate this manually instead of using msec_to_samples() because that function assumes the sample rate matches the current chart audio, and this may not be the case with the original MP3 file the user provided
	channels = stereo ? 2 : 1;
	combined = create_sample(bits,stereo,freq,samples+decoded->len);	//Create a sample array long enough for the silence and the OGG file
	if(combined == NULL)
	{
		destroy_sample(decoded);
		return 23;	//Return failure:  Could not create a sample array for the combined audio
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
		for(ctr=0;ctr < decoded->len * channels;ctr++)
		{
			((unsigned char *)(combined->data))[index++] = ((unsigned char *)(decoded->data))[ctr];
		}
	}
	else
	{	//Copy 16 bit PCM data
		for(ctr=0;ctr < decoded->len * channels;ctr++)
		{
			((unsigned short *)(combined->data))[index++] = ((unsigned short *)(decoded->data))[ctr];
		}
	}

	/* save combined WAV */
	(void) replace_filename(wavfn, eof_song_path, "encode.wav", 1024);
	if(!save_wav(wavfn, combined))
	{
		destroy_sample(decoded);	//This is no longer needed
		destroy_sample(combined);	//This is no longer needed
		return 24;	//Return failure:  Could not create the combined audio WAV file
	}

	/* destroy samples */
	destroy_sample(decoded);	//This is no longer needed
	destroy_sample(combined);	//This is no longer needed

	/* encode the audio */
	printf("%s\n%s\n", eof_song_path, wavfn);
	(void) replace_filename(soggfn, eof_song_path, "encode.ogg", 1024);
	#ifdef ALLEGRO_WINDOWS
		(void) uszprintf(sys_command, (int) sizeof(sys_command) - 1, "oggenc2 -o \"%s\" -b %d \"%s\"", soggfn, alogg_get_bitrate_ogg(eof_music_track) / 1000, wavfn);
	#else
		(void) uszprintf(sys_command, (int) sizeof(sys_command) - 1, "oggenc -o \"%s\" -b %d \"%s\"", soggfn, alogg_get_bitrate_ogg(eof_music_track) / 1000, wavfn);
	#endif

	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tCalling oggenc as follows:  %s", sys_command);
	eof_log(eof_log_string, 1);
	if(eof_system(sys_command))
	{	//If oggenc failed, retry again by specifying a quality level (specifying bitrate can fail in some circumstances)
		eof_log("\t\toggenc failed.  Retrying by specifying a quality level instead of a target bitrate", 1);
		#ifdef ALLEGRO_WINDOWS
			(void) uszprintf(sys_command, (int) sizeof(sys_command) - 1, "oggenc2 -o \"%s\" -q 9 \"%s\"", soggfn, wavfn);
		#else
			(void) uszprintf(sys_command, (int) sizeof(sys_command) - 1, "oggenc -o \"%s\" -q 9 \"%s\"", soggfn, wavfn);
		#endif
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tCalling oggenc as follows:  %s", sys_command);
		eof_log(eof_log_string, 1);
		if(eof_system(sys_command))
		{	//If oggenc failed again
			(void) ustrzcat(sys_command, (int) sizeof(sys_command) - 1, " 2> oggenc.log");	//Append a redirection to capture the output of oggenc
			if(eof_system(sys_command))
			{	//Run one last time to catch the error output
				eof_log("\tOggenc failed.  Please see oggenc.log for any errors it gave.", 1);
				eof_fix_window_title();
				return 25;	//Return failure:  Could not encode combined audio
			}
		}
	}

	/* replace the current OGG file with the new file */
	(void) eof_copy_file(soggfn, (char *)oggfn);	//Copy encode.ogg to the filename of the original OGG

	/* clean up */
	(void) replace_filename(wavfn, eof_song_path, "decode.wav", 1024);
	(void) delete_file(wavfn);		//Delete decode.wav
	(void) replace_filename(wavfn, eof_song_path, "encode.wav", 1024);
	(void) delete_file(wavfn);		//Delete encode.wav
	(void) delete_file(soggfn);	//Delete encode.ogg
	if(eof_load_ogg((char *)oggfn, 0))
	{	//If the combined audio was loaded
		eof_fix_waveform_graph();
		eof_fix_spectrogram();
		eof_fix_window_title();
		eof_chart_length = eof_music_length;
		return 0;	//Return success
	}
	eof_fix_window_title();

	return 26;	//Return error:  Could not load new audio
}

int save_wav_with_silence_appended(const char * fn, SAMPLE * sp, unsigned long ms)
{
	unsigned long i, samples, channels, index = 0;
	SAMPLE * silence, * combined;
	int retval;

 	eof_log("save_wav_with_silence_appended() entered", 1);

	if(!fn || !sp)
		return 0;	//Invalid parameters

	if(!ms)
	{	//If the calling function specified writing the WAV with no silence appended
		return save_wav(fn, sp);	//Write the audio normally
	}

	//Generate the silent audio to conform to the input sample's specifications
	samples = (double)ms * (double)sp->freq / 1000.0;
	channels = sp->stereo ? 2 : 1;
	silence = create_sample(sp->bits, sp->stereo, sp->freq, samples);
	if(!silence)
		return 0;	//Return failure
	if(sp->bits == 8)
	{
		for(i = 0; i < samples * channels; i++)
		{
			((unsigned char *)(silence->data))[i] = 0x80;
		}
	}
	else
	{
		for(i = 0; i < samples * channels; i++)
		{
			((unsigned short *)(silence->data))[i] = 0x8000;
		}
	}

	//Combine the input audio and silence
	combined = create_sample(sp->bits, sp->stereo, sp->freq, sp->len + samples);
	if(!combined)
	{
		free(silence);
		return 0;	//Return failure
	}
	if(sp->bits == 8)
	{
		for(i = 0; i < sp->len * channels; i++)
		{	//For each sample in the input audio
			((unsigned char *)(combined->data))[index++] = ((unsigned char *)(sp->data))[i];	//Copy it into the combined audio sample
		}
		for(i = 0; i < silence->len * channels; i++)
		{	//For each sample in the silent audio
			((unsigned char *)(combined->data))[index++] = ((unsigned char *)(silence->data))[i];	//Copy it into the combined audio sample
		}
	}
	else
	{
		for(i = 0; i < sp->len * channels; i++)
		{	//For each sample in the input audio
			((unsigned short *)(combined->data))[index++] = ((unsigned short *)(sp->data))[i];	//Copy it into the combined audio sample
		}
		for(i = 0; i < silence->len * channels; i++)
		{	//For each sample in the silent audio
			((unsigned short *)(combined->data))[index++] = ((unsigned short *)(silence->data))[i];	//Copy it into the combined audio sample
		}
	}

	//Write the combined audio to file and return
	retval = save_wav(fn, combined);	//Write the combined audio
	destroy_sample(combined);
	destroy_sample(silence);
	return retval;
}
