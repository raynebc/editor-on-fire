#ifndef EOF_BF_H
#define EOF_BF_H

#include "song.h"

int eof_export_bandfuse(EOF_SONG * sp, char * fn, unsigned short *user_warned);
	//Exports the populated pro guitar/bass tracks to the specified file in XML format
	// *user_warned maintains a set of flags about whether various problems were already found and warned about to the user (ie. during Rocksmith export):
	//	1:  At least one track difficulty has no fret hand positions, they will be automatically generated
	//	2:  At least one track uses a fret value higher than 22
	//	4:  At least one open note is marked with bend or slide status
	//	8:  At least one note slides to or above fret 22
	//  16:  There is no COUNT phrase defined and the first beat already contains a phrase
	//  32:  There is at least one phrase or section defined after the END phrase
	//  64:  There is no intro RS section, but the beat marker before the first note already has a section.
	//  128:  There is no noguitar RS section, but the beat marker after the last note already has a section

#endif
