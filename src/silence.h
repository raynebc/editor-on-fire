#ifndef EOF_SILENCE_H
#define EOF_SILENCE_H

#include <allegro.h>

int eof_add_silence(char * oggfn, unsigned long ms);	//Backs up the currently loaded OGG file if it hasn't been already and appends it to an OGG (containing only silence) of the specified length
SAMPLE * create_silence_sample(unsigned long ms);
	//Creates and returns a SAMPLE array of the specified number of milliseconds' worth of silent PCM data, or NULL on error
	//If a chart is loaded, the current OGG file's bitrate, frequency, etc. are used
	//If a chart is not loaded, 128Kbps bitrate, 44.1KHz frequency, 2 channels are used
SAMPLE * create_silence_sample(unsigned long ms);
	//Generates a sample of silence of the specified length
int save_wav(const char * fn, SAMPLE * sp);
	//Writes the specified sample to the specified file name in PCM WAV format
	//Returns nonzero on success
int eof_add_silence(char * oggfn, unsigned long ms);
	//Backs up the specified OGG file and uses OggCat to insert the specified amount of silence at the beginning of the OGG file
	//Returns nonzero on error
int eof_add_silence_recode(char * oggfn, unsigned long ms);
	//Backs up the specified OGG file and inserts the specified amount of silence at the beginning of the OGG file by processing in PCM WAV format and then re-encoding to OGG format
	//Returns nonzero on error
int eof_add_silence_recode_mp3(char * oggfn, unsigned long ms);
	//Similar to eof_add_silence_recode(), but decodes the originally converted MP3 file (original.mp3) instead of the OGG file, preserving as much of the original audio quality as possible
	//Returns nonzero on error
int save_wav_with_silence_appended(const char * fn, SAMPLE * sp, unsigned long ms);
	//Accepts an input audio sample and writes it to the specified WAV file, appending the specified amount of silence in ms
	//This is needed for Rocksmith export since the game requires the song audio to be longer than the chart
	//Returns nonzero on success

#endif
