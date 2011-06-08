#ifndef EOF_TUNING_H
#define EOF_TUNING_H

#include "song.h"

extern char *eof_major_scale_names_flat[12];
extern char *eof_major_scale_names_sharp[12];
extern char **eof_major_scale_names;
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
	char *formula;
} EOF_CHORD_DEFINITION;

#define EOF_NUM_DEFINED_CHORDS 26
extern EOF_CHORD_DEFINITION eof_chord_names[EOF_NUM_DEFINED_CHORDS];

char *eof_lookup_tuning(EOF_SONG *sp, unsigned long track, char *tuning);
	//This returns a string to a pre-defined string naming the track's tuning
	//The tuning array is passed by reference, making it useful for the edit tuning dialog functions
int eof_lookup_default_string_tuning(EOF_SONG *sp, unsigned long track, unsigned long stringnum);
	//Determines the default tuning of the given string for the given pro guitar/bass track, taking the number of strings into account for pro bass
	//The returned number is the number of half steps above A (a value from 0 to 11) upon success (usable with eof_major_scale_names[])
	//-1 is returned upon error
int eof_lookup_tuned_note(EOF_SONG *sp, unsigned long track, unsigned long stringnum, int halfsteps);
	//Determines the note of the specified string tuned the specified number of half steps above/below default tuning, taking the number of strings into account for pro bass
	//The returned number is the number of half steps above A (a value from 0 to 11) upon success (usable with eof_major_scale_names[])
	//-1 is returned upon error
int eof_lookup_played_note(EOF_SONG *sp, unsigned long track, unsigned long stringnum, unsigned long fretnum);
	//Looks up the tuning for the track and returns the note played by the specified string at the specified fret
	//The returned number is the number of half steps above A (a value from 0 to 11) upon success (usable with eof_major_scale_names[])
	//If fretnum is 0xFF (muted), it should cause a -1 to be returned, as the guitar track will not have been set to use that many strings
	//-1 is returned upon error

int eof_lookup_chord(EOF_SONG *sp, unsigned long track, unsigned long note, int *scale, int *chord, int *isslash, int *bassnote);
	//Examines the specified note and returns nonzero if a chord match is found, in which case the chord name is returned through scale and chord
	//scale is set to the index into eof_major_scale_names[] that names the matching chord's major scale
	//chord is set to the index into eof_chord_names[] that names the matching chord
	//isslash is set to nonzero if the chord is detected as a slash chord, in which cass the bass note is returned through bassnote
	//If isslash is zero, combining the index into the eof_major_scale_names[] and eof_chord_names[] arrays will provide a full chord name, such as "Amin6"
	//If isslash is nonzero, combining the index into the eof_major_scale_names[], eof_chord_names[] and eof_slash_note_names[] arrays will provide a full chord name, such as "Dmaj/F#"
	//Zero is returned on error or if no match is found

#endif
