#ifndef _xml_parse_h_
#define _xml_parse_h_

#include <stdio.h>

long int ParseLongIntXML(char *buffer,unsigned long linenum);
	//Takes a string, reads characters into a buffer until a < character is reached and returns the parsed string as an integer value
	//Upon number conversion error, linenum is used to generate a logged error indicating which line the malformed number is in

void XML_Load(FILE *inf);
	//Loads the Guitar Praise XML formatted file into the Lyrics structure

#endif //#ifndef _xml_parse_h_
