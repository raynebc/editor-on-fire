#ifndef EOF_TUNING_H
#define EOF_TUNING_H

#include "song.h"

extern char *eof_note_names_flat[12];
extern char *eof_note_names_sharp[12];
extern char **eof_note_names;
extern char *eof_slash_note_names_flat[12];
extern char *eof_slash_note_names_sharp[12];
extern char **eof_slash_note_names;
extern char *eof_key_names_major[15];
extern char *eof_key_names_minor[15];
extern char **eof_key_names;

extern char eof_tuning_name[EOF_NAME_LENGTH+1];
extern char string_1_name[5];
extern char string_2_name[5];
extern char string_3_name[5];
extern char string_4_name[5];
extern char string_5_name[5];
extern char string_6_name[5];
extern char *tuning_list[EOF_TUNING_LENGTH];

extern unsigned long eof_chord_lookup_note;		//These will be used by eof_count_chord_lookup_matches() to cache its result to avoid unnecessary repeat lookups
extern unsigned long eof_chord_lookup_count;
extern unsigned char eof_chord_lookup_frets[6];
extern unsigned long eof_selected_chord_lookup;	//Will be used to cache the user-selected chord name variation to display (starts at #0)
extern unsigned long eof_cached_chord_lookup_variation;	//These will be used by eof_lookup_chord() to cache the result of a chord variation lookup
extern int eof_cached_chord_lookup_scale;
extern int eof_cached_chord_lookup_chord;
extern int eof_cached_chord_lookup_isslash;
extern int eof_cached_chord_lookup_bassnote;
extern int eof_cached_chord_lookup_retval;
extern int eof_enable_chord_cache;	//Will be set to nonzero when an un-named note is selected and a chord match is found and cached

typedef struct
{
	char *name;
	char is_bass;				//Is nonzero if this is a bass guitar tuning
	unsigned char numstrings;	//The number of strings defined for this tuning
	char tuning[EOF_TUNING_LENGTH];
} EOF_TUNING_DEFINITION;

#define EOF_NUM_TUNING_DEFINITIONS 23
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
	//The track's configured arrangement type will override the normal logic determining whether the track uses bass or guitar tuning
	//The tuning array is passed by reference, making it useful for the edit tuning dialog functions
int eof_lookup_default_string_tuning(EOF_PRO_GUITAR_TRACK *tp, unsigned long track, unsigned long stringnum);
	//Determines the default tuning of the given string for the given pro guitar/bass track, taking the number of strings into account for pro bass
	//The track's configured arrangement type will override the normal logic determining whether the track uses bass or guitar tuning
	//The returned number is the number of half steps above A (a value from 0 to 11) upon success (usable with eof_note_names[])
	//-1 is returned upon error
int eof_lookup_default_string_tuning_absolute(EOF_PRO_GUITAR_TRACK *tp, unsigned long track, unsigned long stringnum);
	//Determines the default tuning of the given string for the given pro guitar/bass track, taking the number of strings into account for pro bass
	//The returned number is the absolute MIDI note note upon success
	//-1 is returned upon error
int eof_lookup_tuned_note(EOF_PRO_GUITAR_TRACK *tp, unsigned long track, unsigned long stringnum, int halfsteps);
	//Determines the note of the specified string tuned the specified number of half steps above/below default tuning, taking the number of strings into account for pro bass
	//The returned number is the number of half steps above A (a value from 0 to 11) upon success (usable with eof_note_names[])
	//-1 is returned upon error
int eof_lookup_played_note(EOF_PRO_GUITAR_TRACK *tp, unsigned long track, unsigned long stringnum, unsigned long fretnum);
	//Looks up the tuning for the track and returns the note played by the specified string at the specified fret
	//The track's option whether or not to honor the defined tuning or assume default tuning is taken into account
	//The returned number is the number of half steps above A (a value from 0 to 11) upon success (usable with eof_note_names[])
	//If fretnum is 0xFF (muted), it should cause a -1 to be returned, as the guitar track will not have been set to use that many strings
	//-1 is returned upon error

int eof_lookup_chord(EOF_PRO_GUITAR_TRACK *tp, unsigned long track, unsigned long note, int *scale, int *chord, int *isslash, int *bassnote, unsigned long skipctr, int cache);
	//Examines the specified note and returns nonzero if a chord match is found, in which case the chord name is returned through scale and chord
	//scale is set to the index into eof_note_names[] that names the matching chord's major scale
	//chord is set to the index into eof_chord_names[] that names the matching chord
	//isslash is set to nonzero if the chord is detected as a slash chord, in which cass the bass note is returned through bassnote
	//If isslash is zero, combining the index into the eof_note_names[] and eof_chord_names[] arrays will provide a full chord name, such as "Amin6"
	//If isslash is nonzero, combining the index into the eof_note_names[], eof_chord_names[] and eof_slash_note_names[] arrays will provide a full chord name, such as "Dmaj/F#"
	//For manually-named notes, 2 is returned (and isslash is set to nonzero) if the scale is identified and the name contains a forward or backward slash but the bass note cannot be identified
	//3 is returned (and isslash is set to nonzero) if both the scale and bass note are identified, in which case bassnote will have the appropriate note
	//Zero is returned on error or if no match is found
	//If skipctr is nonzero, then the first [skipctr] number of matches are ignored during the lookup process, allowing alternate forms of chords to be returned
	//If cache is nonzero, then a successful match is cached through global variables (for use with the note window's functionality for displaying multiple names for a chord)

unsigned long eof_count_chord_lookup_matches(EOF_PRO_GUITAR_TRACK *tp, unsigned long track, unsigned long note);
	//Returns the number of chord lookup matches found for the specified note

unsigned long eof_check_against_chord_lookup_cache(EOF_PRO_GUITAR_TRACK *tp, unsigned long note);
	//Returns nonzero if the specified note matches the note cached from the previously executed chord name lookup logic

char *eof_get_key_signature(EOF_SONG *sp, unsigned long beatnum, char failureoption, char scale);
	//Returns a string representing the key signature change at the specified beat
	//If scale is 0, the key from the major scale is returned, otherwise the key from the minor scale is
	//If the beat contains a KS change, the appropriate index into either eof_key_names_major[] or eof_key_names_minor[] is returned
	//If the beat does not have a KS change, then the string "None" is returned if failureoption is nonzero, otherwise NULL is returned

int eof_track_is_bass_arrangement(EOF_PRO_GUITAR_TRACK *tp, unsigned long track);
	//Returns nonzero if the track number reflects a bass track, or if the track's arrangement type is set to 4 (Bass)

#endif
