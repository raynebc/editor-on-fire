#ifndef EOF_DIALOG_PROC_H
#define EOF_DIALOG_PROC_H

#include <allegro.h>

int eof_verified_edit_proc(int msg, DIALOG *d, int c);
	//Provides an input character validation string via d->dp2.  Any user inputted character not in the string is dropped (arrow keys, delete, backspace and escape are allowed)

#endif
