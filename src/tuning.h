#ifndef EOF_TUNING_H
#define EOF_TUNING_H

#include "song.h"

extern char *eof_note_names_flat[12];
extern char *eof_note_names_sharp[12];
extern char **eof_note_names;
extern char *eof_slash_note_names_flat[12];
extern char *eof_slash_note_names_sharp[12];
extern char **eof_slash_note_names;

extern char eof_tuning_name[EOF_NAME_LENGTH+1];
extern char string_1_name[5];
extern char string_2_name[5];
extern char string_3_name[5];
extern char string_4_name[5];
extern char string_5_name[5];
extern char string_6_name[5];
extern char *tuning_list[EOF_TUNING_LENGTH];

typedef struct
{
	char *name;
	char isbass;				//Is nonzero if this is a bass guitar tuning
	unsigned char numstrings;	//The number of strings defined for this tuning
	char tuning[EOF_TUNING_LENGTH];
} EOF_TUNING_DEFINITION;

#define EOF_NUM_TUNING_DEFINITIONS 16
extern EOF_TUNING_DEFINITION eof_tuning_definitions[EOF_NUM_TUNING_DEFINITIONS];
extern char eof_tuning_unknown[];

typedef struct
{
	char *chordname;
	char *formula;		//The interval formula for the chord, ie. "1,3,5" is a major chord
} EOF_CHORD_DEFINITION;

extern EOF_CHORD_DEFINITION eof_chord_names[EOF_NUM_DEFINED_CHORDS];

char *eof_lookup_tuning_name(EOF_SONG *sp, unsigned long track, char *tuning);
	//This returns a string to a pre-defined string naming the track's tuning
	//The tuning array is passed by reference, making it useful for the edit tuning dialog functions
int eof_lookup_default_string_tuning(EOF_PRO_GUITAR_TRACK *tp, unsigned long track, unsigned long stringnum);
	//Determines the default tuning of the given string for the given pro guitar/bass track, taking the number of strings into account for pro bass
	//The returned number is the number of half steps above A (a value from 0 to 11) upon success (usable with eof_note_names[])
	//-1 is returned upon error
int eof_lookup_tuned_note(EOF_PRO_GUITAR_TRACK *tp, unsigned long track, unsigned long stringnum, int halfsteps);
	//Determines the note of the specified string tuned the specified number of half steps above/below default tuning, taking the number of strings into account for pro bass
	//The returned number is the number of half steps above A (a value from 0 to 11) upon success (usable with eof_note_names[])
	//-1 is returned upon error
int eof_lookup_played_note(EOF_PRO_GUITAR_TRACK *tp, unsigned long track, unsigned long stringnum, unsigned long fretnum);
	//Looks up the tuning for the track and returns the note played by the specified string at the specified fret
	//The returned number is the number of half steps above A (a value from 0 to 11) upon success (usable with eof_note_names[])
	//If fretnum is 0xFF (muted), it should cause a -1 to be returned, as the guitar track will not have been set to use that many strings
	//-1 is returned upon error

int eof_lookup_chord(EOF_PRO_GUITAR_TRACK *tp, unsigned long track, unsigned long note, int *scale, int *chord, int *isslash, int *bassnote);
	//Examines the specified note and returns nonzero if a chord match is found, in which case the chord name is returned through scale and chord
	//scale is set to the index into eof_note_names[] that names the matching chord's major scale
	//chord is set to the index into eof_chord_names[] that names the matching chord
	//isslash is set to nonzero if the chord is detected as a slash chord, in which cass the bass note is returned through bassnote
	//If isslash is zero, combining the index into the eof_note_names[] and eof_chord_names[] arrays will provide a full chord name, such as "Amin6"
	//If isslash is nonzero, combining the index into the eof_note_names[], eof_chord_names[] and eof_slash_note_names[] arrays will provide a full chord name, such as "Dmaj/F#"
	//For manually-named notes, 2 is returned (and isslash is set to nonzero) if the scale is identified and the name contains a forward or backward slash but the bass note cannot be identified
	//	3 is returned (and isslash is set to nonzero) if both the scale and bass note are identified, in which cass bassnote will have the appropriate note
	//Zero is returned on error or if no match is found

void eof_pro_guitar_track_build_chord_variations(EOF_SONG *sp, unsigned long track);
	//Builds the eof_chord_variations[][][] array for the specified track so that the first EOF_MAX_CHORD_VARIATIONS number of each chord scale and type are enumerated
	//The bass note of each chord variation (the note played on the lowest-pitched string that is used) is stored
	//When the chord variations array is built, eof_lookup_chord() will attempt to look up a matching slash chord if the passed note data doesn't match a normal chord
	//This function should be called to rebuild a pro guitar/bass track's chord variations array whenever that track's string count or tuning has changed
void EOF_DEBUG_OUTPUT_CHORD_VARIATION_ARRAYS(void);
	//Dumps the eof_chord_variations array contents for pro guitar and pro bass to "eof_chords.txt"

#endif
