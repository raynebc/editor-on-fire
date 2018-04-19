#ifndef EOF_NOTES_H
#define EOF_NOTES_H

#include <allegro.h>
#include "main.h"
#include "song.h"

int eof_expand_notes_window_text(char *src_buffer, char *dest_buffer, unsigned long dest_buffer_size, int *ypos);
	//Parses the content of the source buffer, expanding macros and populating the destination buffer of the given size
	//The destination buffer is guaranteed to be NULL terminated
	//ypos refers to the variable tracking the y coordinate of text output, allowing macros to alter the output position
	//If a macro is not recognized, the destination buffer is populated with a message indicating the first macro in the input buffer that wasn't recognized
	//Returns zero on error, and writes any failed macro text conversion
	//Returns 1 on a normal conversion
	//Returns 2 if there was a %EMPTY% macro to allow printing of an empty line
int eof_expand_notes_window_macro(char *macro, char *dest_buffer, unsigned long dest_buffer_size);
	//Matches the macro against supported macro strings
	//If a match is found, the corresponding text is generated and written to the destination buffer
	//Returns 2 if the macro is a conditional test that evaluates as false, which signals that the all content from the macro until the end of the line or next %ENDIF% instance is to be skipped
	//Returns 3 if the macro is a conditional test that evaluates as true, in which case the destination buffer is emptied
	//Returns 4 if the %EMPTY% macro is parsed, signaling to the calling function that the line is to be printed even if it is empty after macro expansion
	//Returns 5 if the %MOVE_DOWN_ONE_PIXEL% is parsed, signalling to the calling function to move the vertical position of text output down one pixel
	//Returns zero on error or if no matching string is found for the macro

#endif
