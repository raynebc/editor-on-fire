#ifndef EOF_RS_IMPORT_H
#define EOF_RS_IMPORT_H

#include "song.h"

int eof_parse_xml_attribute_text(char *buffer, size_t size, char *target, char *input);
	//Parses the value of the specified target attribute from the specified input string into the given buffer
	//No more than size bytes are written to the buffer, which is guaranteed to be NULL terminated
	//Returns 1 on success

int eof_parse_xml_attribute_number(char *target, char *input, long *output);
	//Parses and returns the numerical value of the specified target attribute from the specified input string
	//The numerical value is returned through *output
	//Returns 1 on success

int eof_parse_xml_rs_timestamp(char *target, char *input, long *output);
	//Parses and returns the numerical value of the specified target attribute timestamp from the specified input string
	//Periods are discarded (which would convert Rocksmith formatted timestamps to milliseconds) and the value of the timestamp is returned through *output
	//Returns 1 on success

EOF_PRO_GUITAR_TRACK *eof_load_rs(char * fn);
	//Parses the specified Rocksmith XML file and returns its content in a pro guitar track structure

#endif
