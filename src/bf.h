#ifndef EOF_BF_H
#define EOF_BF_H

#include "song.h"

unsigned eof_pro_guitar_note_lookup_string_fingering(EOF_PRO_GUITAR_TRACK *tp, unsigned long note, unsigned long stringnum, unsigned defval);
	//Determines an appropriate fingering for the specified gem of the specified note
	//If the note is an open note, 0 is returned
	//If the note's fingering is defined for that gem, that fingering is returned
	// (1 = index, 2 = middle, 3 = ring, 4 = pinky, 5 = thumb)
	//If no fingering is defined for that gem, it is derived from the fret hand position in effect at that note's position, if any
	// (a gem at the FHP is assumed to be played with the index, one fret away is assumed to be played with the middle finger, etc)
	//If no valid (designating a fret value less than or equal to the gem's fret value) FHP is in effect at the note's position, the given default value is returned

int eof_export_bandfuse(EOF_SONG * sp, char * fn, unsigned short *user_warned);
	//Exports the populated pro guitar/bass tracks to the specified file in XML format
	// *user_warned maintains a set of flags about whether various problems were already found and warned about to the user (ie. during Rocksmith export):
	//	1:  At least one track difficulty has no fret hand positions, they will be automatically generated
	///The following warning codes aren't implemented, but the following could be used to maintain parity with RS exports
	//	2:  At least one track uses a fret value higher than 22
	//	4:  At least one open note is marked with bend or slide status
	//	8:  At least one note slides to or above fret 22
	//  256:  At least one track uses a fret value higher than 24

#endif
