#ifndef EOF_BEATABLE_H
#define EOF_BEATABLE_H

#include "song.h"

void eof_write_beatable_song_section(PACKFILE *fp, unsigned char tsnum, unsigned char tsden, double point1, double point2, unsigned long beatcount);
	//Writes a tempo/ts change to the specified file handle
	//point1 and point2 define the start and stop timestamps in ms, respectively, of the chart that this tempo/ts change covers
	//beatcount is the number of beats occuring during that time.  The time difference and beatcount are used to calculate a tempo

int eof_export_beatable(EOF_SONG *sp, unsigned long track, char *fn);
	//Writes the specified BEATABLE mode track to the specified file

int eof_validate_beatable_file(char *fn);
	//Parses a BEATABLE file for validity and logs its details
	//If the input filename is NULL, EOF will present a file browse dialog to select a file

#endif
