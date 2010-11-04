#ifndef EOF_SILENCE_H
#define EOF_SILENCE_H

int eof_add_silence(const char * oggfn, unsigned long ms);	//Backs up the currently loaded OGG file if it hasn't been already and appends it to an OGG (containing only silence) of the specified length
int eof_add_silence_recode(const char * oggfn, unsigned long ms);
	//Similar to int eof_add_silence(), but a SAMPLE array is built and the entire audio is re-encoded

#endif
