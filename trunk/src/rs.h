#ifndef EOF_RS_H
#define EOF_RS_H

int eof_is_string_muted(EOF_SONG *sp, unsigned long track, unsigned long note);
	//Returns nonzero if all used strings in the note are fret hand muted

unsigned long eof_build_chord_list(EOF_SONG *sp, unsigned long track, unsigned long **results);
	//Parses the given pro guitar track, building a list of all unique chords
	//Memory is allocated to *results to return the list, referring to each note by number
	//The function returns the number of unique chords contained in the list
	//If there are no chords in the pro guitar track, or upon error, *results is set to NULL and 0 is returned

unsigned long eof_build_section_list(EOF_SONG *sp, unsigned long **results);
	//Parses the given chart, building a list of all unique section markers (case insensitive)
	//Memory is allocated to *results to return the list, referring to each text event by number
	//The function returns the number of unique sections contained in the list
	//If there are no sections in the pro guitar track, or upon error, *results is set to NULL and 0 is returned

int eof_export_rocksmith_track(EOF_SONG * sp, char * fn, unsigned long track);
	//Writes the specified pro guitar track in Rocksmith's XML format, if the track is populated
	//fn is expected to point to an array at least 1024 bytes in size.  It's filename is altered to reflect the track's name (ie /PART REAL_GUITAR.xml)

void eof_pro_guitar_track_fix_fingerings(EOF_PRO_GUITAR_TRACK *tp);
	//Checks all notes in the track and duplicates finger arrays of notes with complete finger definitions to matching notes without complete finger definitions
	//If any note has partially defined fingering, it is cleared and will be allowed to be set by a valid fingering from a matching note.

int eof_pro_guitar_note_fingering_valid(EOF_PRO_GUITAR_TRACK *tp, unsigned long note);
	//Returns 0 if the fingering is partially defined for the specified note
	//Returns 1 if the fingering is fully defined for the specified note
	//Returns 2 if the fingering is undefined for the specified note

void eof_song_fix_fingerings(EOF_SONG *sp);
	//Runs eof_pro_guitar_track_fix_fingerings() on all pro guitar tracks in the specified chart

#endif
