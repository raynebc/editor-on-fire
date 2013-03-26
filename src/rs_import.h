#ifndef EOF_RS_IMPORT_H
#define EOF_RS_IMPORT_H

#include "song.h"

EOF_PRO_GUITAR_TRACK *eof_load_rs(char * fn);
	//Parses the specified Rocksmith XML file and returns its content in a pro guitar track structure

#endif
