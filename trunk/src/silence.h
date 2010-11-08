#ifndef EOF_SILENCE_H
#define EOF_SILENCE_H

int eof_add_silence(const char * oggfn, unsigned long ms);	//Backs up the currently loaded OGG file if it hasn't been already and appends it to an OGG (containing only silence) of the specified length
SAMPLE * create_silence_sample(unsigned long ms);
	//Creates and returns a SAMPLE array of the specified number of milliseconds' worth of silent PCM data, or NULL on error
	//If a chart is loaded, the current OGG file's bitrate, frequency, etc. are used
	//If a chart is not loaded, 128Kbps bitrate, 44.1KHz frequency, 2 channels are used
int save_wav(const char * fn, SAMPLE * sp);			//Saves the SAMPLE array to a WAV format file of the specified name.  Returns zero on error
int eof_add_silence_recode(const char * oggfn, unsigned long ms);
int eof_add_silence_recode_mp3(const char * oggfn, unsigned long ms);
	//Similar to int eof_add_silence(), but a SAMPLE array is built and the entire audio is re-encoded

#endif
