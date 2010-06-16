#ifndef LC_IMPORT_H
#define LC_IMPORT_H

#include "song.h"

int EOF_IMPORT_VIA_LC(EOF_VOCAL_TRACK *tp, struct Lyric_Format **lp, int format, char *inputfilename, char *string2);
	//A reference to a destination EOF_VOCAL_TRACK structure and a destination Lyric_Format pointer must be given
	//The lyric format will be detected if format is 0.  If there is only one format detected in the lyrics, it will be
	//imported into the given EOF lyric structure.  The return values are as follows:
	//	-2 if it is a MIDI file with more than one track containing lyrics.  The user should be prompted for which
	//		track to import from.  lp will be modified to refer to a linked list of all candiate lyric formats the
	//		file may contain.
	//	-1 if the detected type is PITCHED LYRICS, the matching VRHYTHM MIDI file needs to be supplied
	//		lp will be modified to refer to a linked list of all candidate lyric formats the file may contain.
	//	0 if the input file was not a valid lyric file.  The import process should halt with error presented to the user.
	//	1 if the import was successful
	//If -1 is returned EOF would need to prompt the user to select the appropriate Vrhythm MIDI file (if pitched lyrics are detected)
	//or select the MIDI track to import from (if separate MIDI tracks have separate lyrics).  (*lp) will refer to a list of possibilities
	//the user should be prompted with, otherwise EOF can make the decision internally.
	//string2 is expected to be either the track name to import (any MIDI based import besides Vrhythm)
	//	string2 may be NULL for SKAR import, as the track to import is pre-determined
	//	string2 may be NULL for MIDI import, as "PART VOCALS" will be assumed by default
	//  string2 is ignored for Pitched Lyric import.  The user will be prompted to browse to a Vocal Rhythm MIDI
	//  string2 is required for KAR import, used if supplied for SKAR/MIDI import as mentioned above, otherwise it is ignored
	//If the format is not zero (no auto detection), lp is not used and may be NULL

int EOF_EXPORT_TO_LC(EOF_VOCAL_TRACK * tp,char *outputfilename,char *string2,int format);
	//Accept the EOF lyric structure (tp) by reference, populate the Lyrics structure with it and perform
	//the specified export.  string2 will be required to be either the pitched lyric filename to create
	//during Vrhythm export, or the name to call the exported track (for MIDI based lyric exports)
	//	string2 may be NULL for SKAR import, as the track to import is pre-determined
	//	string2 may be NULL for MIDI import, as "PART VOCALS" will be assumed by default
	//For export formats besides Vrhythm or other MIDI formats, string2 may be NULL
	//Returns 1 on success, -1 on error and 0 if no lyrics were found in the structure

int EOF_TRANSFER_FROM_LC(EOF_VOCAL_TRACK * tp, struct _LYRICSSTRUCT_ * lp);
	//Load the contents of the Lyrics structure into an EOF lyric structure
	//This is called from EOF_IMPORT_VIA_LC() after the lyrics have been imported


#endif
