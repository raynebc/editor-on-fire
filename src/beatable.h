#ifndef EOF_BEATABLE_H
#define EOF_BEATABLE_H

#include "song.h"

#define EOF_BEATABLE_LINK_THRESHOLD 3
	//How close a hold note must be to a previous one to be able to link to it

void eof_write_beatable_song_section(PACKFILE *fp, unsigned char tsnum, unsigned char tsden, double point1, double point2, unsigned long beatcount);
	//Writes a tempo/ts change to the specified file handle
	//point1 and point2 define the start and stop timestamps in ms, respectively, of the chart that this tempo/ts change covers
	//beatcount is the number of beats occuring during that time.  The time difference and beatcount are used to calculate a tempo

int eof_export_beatable(EOF_SONG *sp, unsigned long track, char *fn);
	//Writes the specified BEATABLE mode track to the specified file

int eof_validate_beatable_file(char *fn);
	//Parses a BEATABLE file for validity and logs its details
	//Returns nonzero if the file's structure is valid and was parsed
	//If the input filename is NULL, EOF will present a file browse dialog to select a file

unsigned char eof_get_beatable_note_end(EOF_SONG *sp, unsigned long track, unsigned long notenum);
	//Examines the specified note and returns the note bitmask reflecting the lane on which the note ends
	//If the note has sustain, this reflects any slide statuses
	//0 is returned upon error
unsigned long eof_fixup_prev_beatable_link(EOF_SONG *sp, unsigned long track, unsigned long notenum);
	//Returns the note index of the hold note that is linked by the specified note, if any
	//Returns ULONG_MAX if there is no such note or upon error
unsigned long eof_fixup_next_beatable_link(EOF_SONG *sp, unsigned long track, unsigned long notenum);
	//Returns the note index of the hold note that links to the specified note, if any
	//Returns ULONG_MAX if there is no such note or upon error

#endif
