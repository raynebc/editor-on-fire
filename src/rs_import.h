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

EOF_PRO_GUITAR_NOTE *eof_rs_import_note_tag_data(char *buffer, int function, EOF_PRO_GUITAR_TRACK *tp, unsigned long linectr);
	//Parses the note, chordnote or bendvalue XML tag contained in the specified array,
	// creating a pro guitar note structure as defined by the tag
	//If function is nonzero, the note is appended to the specified track structure
	//If a bendvalue tag is parsed, the difficulty and string number of the last normal note to be parsed by this function is applied to the created tech note
	//The created note is returned, or NULL is returned on error
	//linectr is cited as the XML line number in error logging

EOF_PRO_GUITAR_TRACK *eof_load_rs(char * fn);
	//Parses the specified Rocksmith XML file and returns its content in a pro guitar track structure

#endif
