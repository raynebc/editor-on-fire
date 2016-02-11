#ifndef EOF_RS_IMPORT_H
#define EOF_RS_IMPORT_H

#include "song.h"

int eof_parse_chord_template(char *name, size_t size, char *finger, char *frets, unsigned char *note, unsigned char *numstrings, unsigned char *isarp, unsigned long linectr, char *input);
	//Reads a Rocksmith XML format chord template from the input string into the destination variables:
	//	The name attribute is read into name[] which is an array at least size number of bytes in size
	//	The chord fingering is read into finger[] which is an array at least 6 bytes in size
	//	The fretting is read into frets[] which is an array at least 6 bytes in size
	//	The corresponding note bitmask is stored into *note
	//  If the parsed chord's display name contains "-arp", *isarp is set to nonzero if isarp is not NULL
	//If numstrings is not NULL, it is set to 4 if the chord templates reflect a 4 string instrument (ie. bass)
	//Returns zero on success
	//Upon error, the specific cause for failure is logged regarding line number linectr and nonzero is returned

EOF_PRO_GUITAR_TRACK *eof_load_rs(char * fn);
	//Parses the specified Rocksmith XML file and returns its content in a pro guitar track structure

#endif
