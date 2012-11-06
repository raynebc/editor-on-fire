#ifndef EOF_RS_H
#define EOF_RS_H

int eof_is_string_muted(EOF_SONG *sp, unsigned long track, unsigned long note);
	//Returns nonzero if all used strings in the note are fret hand muted

unsigned long eof_build_chord_list(EOF_SONG *sp, unsigned long track, unsigned long **results);
	//Parses the given pro guitar track, building a list of all unique chords
	//Memory is allocated to *results to return the list, referring to each note by number
	//The function returns the number of unique chords contained in the list
	//If there are no chords in the pro guitar track, or upon error, *results is set to NULL and 0 is returned

int eof_export_rocksmith_track(EOF_SONG * sp, char * fn, unsigned long track);
	//Writes the specified pro guitar track in Rocksmith's XML format
	//fn is expected to point to an array at least 1024 bytes in size.  It's filename is altered to reflect the track's name (ie /PART REAL_GUITAR.xml)

#endif
