#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include "main.h"
#include "tuning.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

char *eof_note_names_flat[12] =		{"A","Bb","B","C","Db","D","Eb","E","F","Gb","G","Ab"};		//The name of each scale in order from 0 (0 semitones above A) to 11 (11 semitones above A)
char *eof_note_names_sharp[12] =	{"A","A#","B","C","C#","D","D#","E","F","F#","G","G#"};		//The name of each scale in order from 0 (0 semitones above A) to 11 (11 semitones above A)
char **eof_note_names =	eof_note_names_sharp;	//By default, display scales with sharp accidentals
char *eof_slash_note_names_flat[12] =	{"/A","/Bb","/B","/C","/Db","/D","/Eb","/E","/F","/Gb","/G","/Ab"};	//The suffix of each type of slash chord
char *eof_slash_note_names_sharp[12] =	{"/A","/A#","/B","/C","/C#","/D","/D#","/E","/F","/F#","/G","/G#"};
char **eof_slash_note_names = eof_slash_note_names_sharp;	//By default, display slash chords with sharp accidentals
char eof_tuning_unknown[] = {"Unknown"};
char *eof_key_names_major[15] = {"B", "Gb", "Db", "Ab", "Eb", "Bb", "F", "C", "G", "D", "A", "E", "B", "Gb", "Db"};	//In MIDI, -7 represents 7 flats, 7 represents 7 sharps.  This array is that system transposed up by 7 to avoid using negative indexes
char *eof_key_names_minor[15] = {"g#", "eb", "bb", "f", "c", "g", "d", "a", "e", "b", "f#", "c#", "g#", "eb", "bb"};
char **eof_key_names = eof_key_names_major;
char *eof_none_string = "None";

char eof_tuning_name[EOF_NAME_LENGTH+1] = {"Unknown"};
char string_1_name[5] = {0};
char string_2_name[5] = {0};
char string_3_name[5] = {0};
char string_4_name[5] = {0};
char string_5_name[5] = {0};
char string_6_name[5] = {0};
char *tuning_list[EOF_TUNING_LENGTH] = {string_1_name,string_2_name,string_3_name,string_4_name,string_5_name,string_6_name};

unsigned long eof_chord_lookup_note = 0;
unsigned long eof_chord_lookup_count = 0;
unsigned char eof_chord_lookup_frets[6] = {0};
unsigned long eof_selected_chord_lookup = 0;
unsigned long eof_cached_chord_lookup_variation = 0;
int eof_cached_chord_lookup_scale = 0;
int eof_cached_chord_lookup_chord = 0;
int eof_cached_chord_lookup_isslash = 0;
int eof_cached_chord_lookup_bassnote = 0;
int eof_cached_chord_lookup_retval = 0;
int eof_enable_chord_cache = 0;

EOF_TUNING_DEFINITION eof_tuning_definitions[EOF_NUM_TUNING_DEFINITIONS] =
{
 {"Standard tuning", 1, 4, {0,0,0,0}},
 {"Standard tuning", 1, 5, {0,0,0,0,0}},
 {"Standard tuning", 1, 6, {0,0,0,0,0,0}},
 {"Standard tuning", 0, 6, {0,0,0,0,0,0}},
 {"D# tuning", 0, 6, {-1,-1,-1,-1,-1,-1}},
 {"D tuning", 0, 6, {-2,-2,-2,-2,-2,-2}},
 {"C# tuning", 0, 6, {-3,-3,-3,-3,-3,-3}},
 {"C tuning", 0, 6, {-4,-4,-4,-4,-4,-4}},
 {"B tuning", 0, 6, {-5,-5,-5,-5,-5,-5}},
 {"A# tuning", 0, 6, {-6,-6,-6,-6,-6,-6}},
 {"A tuning", 0, 6, {-7,-7,-7,-7,-7,-7}},
 {"G# tuning", 0, 6, {-8,-8,-8,-8,-8,-8}},
 {"G tuning", 0, 6, {-9,-9,-9,-9,-9,-8}},
 {"F# tuning", 0, 6, {-10,-10,-10,-10,-10,-10}},
 {"F tuning", 0, 6, {-11,-11,-11,-11,-11,-11}},
 {"Drop D tuning", 0, 6, {-2,0,0,0,0,0}}
};

EOF_CHORD_DEFINITION eof_chord_names[EOF_NUM_DEFINED_CHORDS] =
{
	{"sus2", "1,2,5"},
	{"madd4", "1,b3,4,5"},
	{"dim", "1,b3,b5"},
	{"dim7", "1,b3,b5,6"},
	{"m7b5", "1,b3,b5,b7"},
	{"min", "1,b3,5"},
	{"min7b6", "1,b3,5,b6,b7"},
	{"min6", "1,b3,5,6"},
	{"min6/Maj7", "1,b3,5,6,7"},
	{"min7", "1,b3,5,b7"},
	{"m/Maj7", "1,b3,5,7"},
	{"m#7aug5", "1,b3,#5,7"},
	{"3", "1,3"},
	{"add4", "1,3,4,5"},
	{"majb5", "1,3,b5"},
	{"7b5", "1,3,b5,b7"},
	{"maj7b5", "1,3,b5,7"},
	{"maj", "1,3,5"},
	{"6", "1,3,5,6"},
	{"7", "1,3,5,b7"},
	{"maj7", "1,3,5,7"},
	{"aug", "1,3,#5"},
	{"aug7", "1,3,#5,b7"},
	{"majAug7", "1,3,#5,7"},
	{"sus4", "1,4,5"},
	{"7sus4", "1,4,5,b7"},
	{"b5", "1,b5"},
	{"5", "1,5"}
};

char *eof_lookup_tuning_name(EOF_SONG *sp, unsigned long track, char *tuning)
{
	unsigned long tracknum, ctr, ctr2;
	char matchfailed;
	char isbass = 0;	//By default, assume this is not a pro bass track

	if((sp == NULL) || (track >= sp->tracks) || (sp->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return eof_tuning_unknown;	//Invalid song pointer or track number
	tracknum = sp->track[track]->tracknum;
	if(sp->pro_guitar_track[tracknum]->numstrings > EOF_TUNING_LENGTH)
		return eof_tuning_unknown;	//Unsupported number of strings

	if((track == EOF_TRACK_PRO_BASS) || (track == EOF_TRACK_PRO_BASS_22))
	{
		isbass = 1;
	}

	for(ctr = 0; ctr < EOF_NUM_TUNING_DEFINITIONS; ctr++)
	{	//For each defined tuning
		matchfailed = 0;	//Reset this failure status
		if((eof_tuning_definitions[ctr].numstrings == sp->pro_guitar_track[tracknum]->numstrings) && (isbass == eof_tuning_definitions[ctr].isbass))
		{	//If this is a valid tuning definition to check against (matching number of strings and guitar type)
			for(ctr2 = 0; ctr2 < eof_tuning_definitions[ctr].numstrings; ctr2++)
			{	//For each string in the definition
				if(eof_tuning_definitions[ctr].tuning[ctr2] != tuning[ctr2])
				{	//This string doesn't match the definition's tuning
					matchfailed = 1;
					break;
				}
			}
			if(!matchfailed)
			{	//If all strings matched the definition
				return eof_tuning_definitions[ctr].name;	//Return this tuning's name
			}
		}
	}
	return eof_tuning_unknown;	//No matching tuning definition found
}

int eof_lookup_default_string_tuning(EOF_PRO_GUITAR_TRACK *tp, unsigned long track, unsigned long stringnum)
{	//A A# B C C# D D# E F F# G  G#
	//0 1  2 3 4  5 6  7 8 9  10 11
	int default_tuning_6_string[] = {7,0,5,10,2,7};		//Half steps above note A, representing "EADGBE"
	int default_tuning_4_string_bass[] = {7,0,5,10};	//Half steps above note A, representing "EADG";
	int default_tuning_5_string_bass[] = {2,7,0,5,10};	//Half steps above note A, representing "BEADG";
	int default_tuning_6_string_bass[] = {2,7,0,5,10,3};//Half steps above note A, representing "BEADGC";

	if(tp == NULL)
		return -1;	//Invalid track pointer
	if(stringnum >= tp->numstrings)
		return -1;	//Invalid string number
	if((track == EOF_TRACK_PRO_GUITAR) || (track == EOF_TRACK_PRO_GUITAR_22))
	{	//Standard tuning is EADGBe
		return default_tuning_6_string[stringnum];	//Return this string's default tuning
	}
	else if((track == EOF_TRACK_PRO_BASS) || (track == EOF_TRACK_PRO_BASS_22))
	{
		if(tp->numstrings == 4)
		{	//Standard tuning for 4 string bass is EADG
			return default_tuning_4_string_bass[stringnum];	//Return this string's default tuning
		}
		else if(tp->numstrings == 5)
		{	//Standard tuning for 5 string bass is BEADG
			return default_tuning_5_string_bass[stringnum];	//Return this string's default tuning
		}
		else if(tp->numstrings == 6)
		{	//Standard tuning for 6 string bass is BEADGC
			return default_tuning_6_string_bass[stringnum];	//Return this string's default tuning
		}
	}

	return -1;	//Unsupported pro guitar track
}

int eof_lookup_tuned_note(EOF_PRO_GUITAR_TRACK *tp, unsigned long track, unsigned long stringnum, int halfsteps)
{
	int notenum;

	if(tp == NULL)
		return -1;	//Invalid track pointer
	if(stringnum >= tp->numstrings)
		return -1;	//Invalid string number

	notenum = eof_lookup_default_string_tuning(tp, track, stringnum);	//Look up the default tuning for this string
	if(notenum < 0)	//If lookup failed,
		return -1;	//Return error
	notenum += halfsteps;	//Adjust for the number of half steps above/below the default tuning

	notenum %= 12;
	if(notenum < 0)		//Handle for down tuning
		notenum += 12;

	return (notenum % 12);	//Return the note value in terms of half steps above note A
}

int eof_lookup_played_note(EOF_PRO_GUITAR_TRACK *tp, unsigned long track, unsigned long stringnum, unsigned long fretnum)
{
	int notenum;

	if(tp == NULL)
		return -1;	//Invalid track pointer
	if((stringnum >= tp->numstrings) || (fretnum > tp->numfrets))
		return -1;	//Invalid string number or fret number

	notenum = eof_lookup_tuned_note(tp, track, stringnum, tp->tuning[stringnum]);	//Look up the open note this string plays
	if(notenum < 0)	//If lookup failed,
		return -1;	//Return error
	notenum += fretnum;	//Take the fret number into account

	return (notenum % 12);	//Return the note value in terms of half steps above note A
}

int eof_lookup_chord(EOF_PRO_GUITAR_TRACK *tp, unsigned long track, unsigned long note, int *scale, int *chord, int *isslash, int *bassnote, unsigned long skipctr, int cache)
{
	unsigned long bitmask, stringctr = 0;
	static char major_scales[12][7];
	char notes_played[12] = {0}; 	//Will indicate which notes are present in the chord
	static char scales_created = 0;	//This will be set to one after the major_scales[] array has been created
	char chord_intervals[30];		//This stores the referred note's interval makeup for the current scale being checked against
	char *chord_intervals_index;	//This is an index into chord_intervals[] for added string processing efficiency
	unsigned long ctr, ctr2, ctr3, ctr4, halfstep, halfstep2, skipaccidental, pass;
	int retval, bass = -1;			//bass will track the bass note (for now, the note played on the lowest used string) of the chord
	char *name, **notename = eof_note_names_sharp;
	unsigned long originalskipctr = skipctr;	//Save this for caching purposes

	if((tp == NULL) || (note >= tp->notes) || !scale || !chord || !isslash || !bassnote)
		return 0;	//Return error if any of the parameters are not valid

	name = tp->note[note]->name;	//Keep it simple
	if(name[0] != '\0')
	{	//If the note was manually named, parse the name to determine its scale and possibly bass note
		for(ctr2 = 0; ctr2 < 2; ctr2++)
		{	//For each of the 2 note name arrays
			if(ctr2)
			{	//On the second pass, parse for flat accidental names
				notename = eof_note_names_flat;
			}
			for(ctr3 = 0; ctr3 < 12; ctr3++)
			{	//For each of the 12 notes
				for(ctr = 0; (name[ctr] != '\0') && isspace(name[ctr]); ctr++);	//Use ctr to index past any leading whitespace
				if(toupper(name[ctr++]) != notename[ctr3][0])	//If the first character (note) doesn't match the note name
					continue;	//Skip to next note
				if((notename[ctr3][1] != '\0') && (tolower(name[ctr++]) != notename[ctr3][1]))	//If the second character (accidental) doesn't match the note name's second character (if it has one)
					continue;	//Skip to next note
				//At this point, a viable match has been found and name[ctr] points to the character after the note name and accidental (if any)
				*scale = ctr3;		//Store the note value
				if(strchr(name, '/') || strchr(name, '\\'))
				{	//If this note name has a forward or backward slash
					notename = eof_note_names_sharp;	//Check for sharp bass notes first
					for(ctr2 = 0; ctr2 < 2; ctr2++)
					{	//For each of the 2 note name arrays
						if(ctr2)
						{	//On the second pass, parse for flat accidental names
							notename = eof_note_names_flat;
						}
						for(; (name[ctr] != '\0') && isspace(name[ctr]); ctr++);	//Use ctr to index past any whitespace between the slash and the bass note name
						for(ctr3 = 0; ctr3 < 12; ctr3++)
						{	//For each of the 12 notes
							if(toupper(name[ctr]) != notename[ctr3][0])	//If the first character (note) doesn't match the note name
								continue;	//Skip to next note
							if((name[ctr] == '\0') && (notename[ctr3][1] != '\0'))	//If the note name doesn't have a second character
								continue;	//Skip to next note
							if(tolower(name[ctr+1]) != notename[ctr3][1])	//If the second character (accidental) doesn't match the note name
								continue;	//Skip to next note
							//At this point, a viable bass note match has been found
							*isslash = 1;	//A chord name and bass note have been identified, consider this a slash chord
							*bassnote = ctr3;
							return 3;		//Return slash chord match found
						}
					}
					//If this point was reached, the bass note couldn't be identified, but leave *isslash set so it can be marked as a slash chord during MIDI export
					*isslash = 1;	//Consider it a slash chord and try to identify the bass note
					return 2;	//Return match found (possible slash chord match)
				}
				*isslash = 0;	//No slash character was found, but the chord scale was found
				return 1;	//Return match found
			}
		}
	}
	else
	{	//Otherwise perform chord lookup based on notes played
		if(eof_check_against_chord_lookup_cache(tp, note))
		{	//If this note is the same as that of the cached lookup
			if((cache && (eof_cached_chord_lookup_variation == skipctr)) || !cache)
			{	//If running caching logic, return the cached result only if it's the cached variation number
				//Otherwise if not caching (ie. running a normal name lookup in the editor pane or export logic, return the cached name variation
				*scale = eof_cached_chord_lookup_scale;
				*chord = eof_cached_chord_lookup_chord;
				*isslash = eof_cached_chord_lookup_isslash;
				*bassnote = eof_cached_chord_lookup_bassnote;
				return eof_cached_chord_lookup_retval;
			}
		}

		//If the major scales table hasn't been created yet, do so now
		if(!scales_created)
		{
			for(ctr = 0; ctr < 12; ctr++)
			{	//For each note in the musical scale from 0 (A) to 11 (G#)
				major_scales[ctr][0] = ctr;	//The first interval in any major scale is that scale's namesake
				major_scales[ctr][1] = (ctr + 2) % 12;	//The second interval is two half steps higher
				major_scales[ctr][2] = (ctr + 4) % 12;	//The third interval is two half steps higher
				major_scales[ctr][3] = (ctr + 5) % 12;	//The fourth interval is one half step higher
				major_scales[ctr][4] = (ctr + 7) % 12;	//The fifth interval is two half steps higher
				major_scales[ctr][5] = (ctr + 9) % 12;	//The sixth interval is two half steps higher
				major_scales[ctr][6] = (ctr + 11) % 12;	//The seventh interval is two half steps higher
			}
			scales_created = 1;
		}

		//Determine which musical notes are contained with the specified note's strings
		for(ctr = 0, bitmask = 1; ctr < tp->numstrings; ctr++, bitmask <<= 1)
		{	//For each string in this track
			if(tp->note[note]->note & bitmask)
			{	//If this string is used in the note
				stringctr++;	//Keep track of how many strings are used
				retval = eof_lookup_played_note(tp, track, ctr, tp->note[note]->frets[ctr]);	//Look up the note played on this string at the specified fret
				if(retval >= 0)
				{	//The note lookup succeeded
					if(bass < 0)
					{
						bass = retval;	//Store the bass note
					}
					retval %= 12;	//Guarantee it is a value from 0 to 11
					notes_played[retval] = 1;	//Mark this musical note as existing in the specified note
				}
			}
		}

		if(stringctr < 2)	//If there aren't at least two strings used
		{
			return 0;		//Return no matches
		}

		for(pass = 0; pass < 2; pass++)
		{	//On the first pass, perform normal lookup.  On the second pass, perform (hybrid) slash chord lookup (disregarding the bass note)
		//Look up the note against each major scale
			for(ctr = 0; ctr < 12; ctr++)
			{	//For each major scale
				//Create the list of intervals this note uses for this scale
				chord_intervals[0] = '\0';	//Truncate the intervals string
				chord_intervals_index = chord_intervals;
				halfstep = major_scales[ctr][0];	//Chord formulas always start with the lowest scale interval
				for(ctr2 = 0; ctr2 < 12; ctr2++, halfstep++)
				{	//For each of the 12 notes, starting which the note that is represented by interval 1 of this scale
					if(halfstep >= 12)
					{	//Wrap around the note
						halfstep -= 12;
					}
					if(notes_played[halfstep])
					{	//If this note is played, check to see if it matches an interval
						for(ctr3 = 0; ctr3 < 7; ctr3++)
						{	//For each interval in the scale
							if(halfstep == major_scales[ctr][ctr3])
							{	//If the note matches this interval
								if(chord_intervals[0] != '\0')
								{	//If this isn't the first interval stored in the array
									*(chord_intervals_index++) = ',';			//Append a comma first
								}
								*(chord_intervals_index++) = '0' + (ctr3 + 1);	//Write the interval value into the string
								break;
							}
							else
							{	//The normal interval is not played in this note
								halfstep2 = (major_scales[ctr][ctr3] + 11) % 12;	//Get the note value for the flat of this interval
								if(halfstep == halfstep2)
								{	//If the note matches the flat of this interval
									skipaccidental = 0;	//Reset this status
									for(ctr4 = 0; ctr4 < 7; ctr4++)
									{	//Check to make sure the flat of this interval isn't already a non accidental interval (ie. In the A major scale, interval 3 and flat interval 4 are the same note)
										if(halfstep2 == major_scales[ctr][ctr4])
										{	//The flat of this interval is already a non accidental interval for this scale, don't add this to the ongoing interal list
											skipaccidental = 1;
											break;
										}
									}
									if(!skipaccidental)
									{	//If the flat of this interval is played in this note
										if(chord_intervals[0] != '\0')
										{	//If this isn't the first interval stored in the array
											*(chord_intervals_index++) = ',';			//Append a comma first
										}
										*(chord_intervals_index++) = 'b';				//Write flat character into the string
										*(chord_intervals_index++) = '0' + (ctr3 + 1);	//Write the interval value
										break;
									}
								}
								else if(ctr3 == 4)
								{	//The flat interval is not played in this note (only check for a sharp of the fifth interval to avoid matching issues)
									halfstep2 = (halfstep2 + 2) % 12;	//Get the note value for the sharp of this interval (two halfsteps above the previous flat value)
									if(halfstep == halfstep2)
									{	//If the note matches the sharp of this interval
										skipaccidental = 0;	//Reset this status
										for(ctr4 = 0; ctr4 < 7; ctr4++)
										{	//Check to make sure the flat of this interval isn't already a non accidental interval (ie. In the A major scale, interval 3 and flat interval 4 are the same note)
											if(halfstep2 == major_scales[ctr][ctr4])
											{	//The flat of this interval is already a non accidental interval for this scale, don't add this to the ongoing interal list
												skipaccidental = 1;
												break;
											}
										}
										if(!skipaccidental)
										{	//If the sharp of this interval is played in this note
											if(chord_intervals[0] != '\0')
											{	//If this isn't the first interval stored in the array
												*(chord_intervals_index++) = ',';			//Append a comma first
											}
											*(chord_intervals_index++) = '#';				//Write sharp character into the string
											*(chord_intervals_index++) = '0' + (ctr3 + 1);	//Write the interval value
											break;
										}
									}//If the note matches the sharp of this interval
								}//The flat interval is not played in this note (only check for a sharp of the fifth interval to avoid matching issues)
							}//The normal interval is not played in this note
						}//For each interval in the scale
					}//If this note is played, check to see if it matches an interval
				}//For each of the 12 notes, starting which the note that is represented by interval 1 of this scale

				*(chord_intervals_index) = '\0';				//Terminate the string
				//Look up the list of defined chords to see if this note's intervals matches any
				for(ctr2 = 0; ctr2 < EOF_NUM_DEFINED_CHORDS; ctr2++)
				{	//For each defined chord
					if(!strcmp(eof_chord_names[ctr2].formula, chord_intervals))
					{	//A match was found
						if(!skipctr)
						{	//If this match is not supposed to be skipped
							if(cache)
							{	//If a successful lookup is to be cached to global variables
								eof_cached_chord_lookup_scale = ctr;
								eof_cached_chord_lookup_chord = ctr2;
								eof_cached_chord_lookup_variation = originalskipctr;
								eof_chord_lookup_note = eof_get_pro_guitar_note_note(tp, note);		//Cache the looked up note's details
								memcpy(eof_chord_lookup_frets, tp->note[note]->frets, 6);
								eof_enable_chord_cache = 1;
							}
							*scale = ctr;	//Pass the scale back through the pointer
							*chord = ctr2;	//Pass the chord name back through the pointer
							if(!pass)
							{	//This is the first pass (normal chord)
								if((bass != ctr) && eof_inverted_chords_slash)
								{	//If this is an inverted chord and the user opted to have such chords detect as slash chords
									if(cache)
									{	//If a successful lookup is to be cached to global variables
										eof_cached_chord_lookup_isslash = 1;
										eof_cached_chord_lookup_bassnote = bass;
										eof_cached_chord_lookup_retval = 3;
									}
									*isslash = 1;		//This was detected as a slash chord
									*bassnote = bass;	//Pass the bass note back through the pointer
									return 3;			//Return slash chord match found
								}
								if(cache)
								{	//If a successful lookup is to be cached to global variables
									eof_cached_chord_lookup_isslash = 0;
									eof_cached_chord_lookup_retval = 1;
								}
								*isslash = 0;	//This was detected as a normal chord, not a slash chord
								return 1;		//Return match found
							}
							else
							{	//This is the second pass (hybrid slash chord)
								if(cache)
								{	//If a successful lookup is to be cached to global variables
									eof_cached_chord_lookup_isslash = 1;
									eof_cached_chord_lookup_bassnote = bass;
									eof_cached_chord_lookup_retval = 3;
								}
								*isslash = 1;		//This was detected as a slash chord
								*bassnote = bass;	//Pass the bass note back through the pointer
								return 3;			//Return slash chord match found
							}
						}
						else
						{	//If this match is supposed to be skipped
							skipctr--;	//Decrement the number of matches to skip
						}
					}
				}
			}//For each major scale
			notes_played[bass] = 0;	//Remove the bass note from the lookup so the second pass can search for hybrid slash chords
		}//On the first pass, perform normal lookup.  On the second pass, perform (hybrid) slash chord lookup (disregarding the bass note)
	}//Otherwise perform chord lookup based on notes played

	return 0;	//Return no match found
}

unsigned long eof_check_against_chord_lookup_cache(EOF_PRO_GUITAR_TRACK *tp, unsigned long note)
{
	unsigned long thisnote, ctr, bitmask, match = 0;

	//Check to see if the specified note matches that of the previously cached lookup
	thisnote = eof_get_pro_guitar_note_note(tp, note);
	if(eof_chord_lookup_note == thisnote)
	{	//If this note has the same note bitmask as the previous lookup
		match = 1;	//Assume it's a match unless one of the fret values isn't equal
		for(ctr = 0, bitmask = 1; ctr < 6; ctr++, bitmask <<= 1)
		{	//For each of the 6 usable strings
			if(eof_chord_lookup_note & bitmask)
			{	//If this string is in use
				if(eof_chord_lookup_frets[ctr] != tp->note[note]->frets[ctr])
				{	//If this string's fret value doesn't match that of the previous lookup
					match = 0;
					break;
				}
			}
		}
	}
	return match;
}

unsigned long eof_count_chord_lookup_matches(EOF_PRO_GUITAR_TRACK *tp, unsigned long track, unsigned long note)
{
	unsigned long ctr = 0, matchcount = 0;
	int scale, chord, isslash, bassnote;

	//Check to see if the specified note matches that of the previous lookup
	if(eof_check_against_chord_lookup_cache(tp, note))
	{	//If eof_chord_lookup_count wasn't reset by selecting another note, and this note is the same as that of the previous lookup
		return eof_chord_lookup_count;	//Return the previous lookup's result
	}

	eof_selected_chord_lookup = 0;	//Revert to using the first chord lookup result whenever a different note is selected
	while(eof_lookup_chord(tp, track, note, &scale, &chord, &isslash, &bassnote, ctr, 0))
	{	//As long as a chord variation is found
		matchcount++;	//Increment the number of matches found
		ctr++;			//Increment the number of matches skipped
	}

	eof_chord_lookup_count = matchcount;	//Cache the result
	return matchcount;
}

char *eof_get_key_signature(EOF_SONG *sp, unsigned long beatnum, char failureoption, char scale)
{
	if(sp && (beatnum < sp->beats) && (sp->beat[beatnum]->flags & EOF_BEAT_FLAG_KEY_SIG))
	{	//If this is a valid beat and it has a key signature defined
		if((sp->beat[beatnum]->key >= -7) && (sp->beat[beatnum]->key <= 7))
		{	//If the key signature is valid
			if(!scale)
				return eof_key_names_major[sp->beat[beatnum]->key + 7];	//Return the appropriate key name from the array
			else
				return eof_key_names_minor[sp->beat[beatnum]->key + 7];	//Return the appropriate key name from the array
		}
	}

	if(failureoption)
		return eof_none_string;
	else
		return NULL;
}
