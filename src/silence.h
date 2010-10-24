#ifndef EOF_SILENCE_H
#define EOF_SILENCE_H

int eof_add_silence(const char * oggfn, unsigned long ms);	//Backs up the currently loaded OGG file if it hasn't been already and appends it to an OGG (containing only silence) of the specified length

#endif
