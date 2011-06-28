#include <assert.h>
#include <ctype.h>
#include <math.h>
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

char eof_tuning_name[EOF_NAME_LENGTH+1] = {"Unknown"};
char string_1_name[5] = {0};
char string_2_name[5] = {0};
char string_3_name[5] = {0};
char string_4_name[5] = {0};
char string_5_name[5] = {0};
char string_6_name[5] = {0};
char *tuning_list[EOF_TUNING_LENGTH] = {string_1_name,string_2_name,string_3_name,string_4_name,string_5_name,string_6_name};

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
	{"7sus", "1,4,5,b7"},
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

	if(track == EOF_TRACK_PRO_BASS)
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
	if(track == EOF_TRACK_PRO_GUITAR)
	{	//Standard tuning is EADGBe
		return default_tuning_6_string[stringnum];	//Return this string's default tuning
	}
	else if(track == EOF_TRACK_PRO_BASS)
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

int eof_lookup_chord(EOF_PRO_GUITAR_TRACK *tp, unsigned long track, unsigned long note, int *scale, int *chord, int *isslash, int *bassnote)
{
	unsigned long bitmask;
	static char major_scales[12][7];
	char notes_played[12] = {0}; 	//Will indicate which notes are present in the chord
	static char scales_created = 0;	//This will be set to one after the major_scales[] array has been created
	char chord_intervals[30];		//This stores the referred note's interval makeup for the current scale being checked against
	char *chord_intervals_index;	//This is an index into chord_intervals[] for added string processing efficiency
	unsigned long ctr, ctr2, ctr3, ctr4, halfstep, halfstep2, skipaccidental;
	int retval;
	char *name, **notename = eof_note_names_sharp;

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
				if((name[ctr] == '\0') && (notename[ctr3][1] != '\0'))	//If the note name doesn't have a second character
					continue;	//Skip to next note
				if(tolower(name[ctr++]) != notename[ctr3][1])	//If the second character (accidental) doesn't match the note name
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
				retval = eof_lookup_played_note(tp, track, ctr, tp->note[note]->frets[ctr]);	//Look up the note played on this string at the specified fret
				if(retval >= 0)
				{	//The note lookup succeeded
					retval %= 12;	//Guarantee it is a value from 0 to 11
					notes_played[retval] = 1;	//Mark this musical note as existing in the specified note
				}
			}
		}

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
					*isslash = 0;	//This was detected as a normal chord, not a slash chord
					*scale = ctr;	//Pass the scale back through the pointer
					*chord = ctr2;	//Pass the chord name back through the pointer
					return 1;		//Return match found
				}
			}
		}//For each major scale

		//If no match has been found with standard chords, check for slash chords if the track's chord variations array is usable
		if(tp->eof_chord_variations_array_ready)
		{	//If the array is marked as usable
		}
	}//Otherwise perform chord lookup based on notes played

	return 0;	//Return no match found
}

//#define EOF_CHORD_BUILD_DEBUG
void eof_pro_guitar_track_build_chord_variations(EOF_SONG *sp, unsigned long track)
{
	unsigned long tracknum, ctr, bitmask;
	EOF_PRO_GUITAR_TRACK dummy_track = {0};		//Passed to tuning logic, note 0 will be updated with each possible string/fret permutation and tested
	EOF_PRO_GUITAR_TRACK *real_track = NULL;	//Refers to the track passed to this function
	unsigned char *frets = NULL;			//This will refer to the fret array for the note being permutated in the dummy track
	char done = 0;
	int scale, chord, isslash, bassnote;
	unsigned long array_depth;
	unsigned long debug_ctr = 0, debug_total_ctr = 0;	//Every 1000 loookups, the title bar will be updated and a line outputted to the EOF log
	long double power;

	if((sp == NULL) || (track >= sp->tracks) || (sp->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return;	//Invalid song pointer or track number

	eof_log("eof_pro_guitar_track_build_chord_variations() entered", 1);

	//Initialize
	tracknum = sp->track[track]->tracknum;
	real_track = sp->pro_guitar_track[tracknum];
	real_track->eof_chord_variations_array_ready = 0;	//Reset this so the chord lookup logic skips looking for slash chords
	dummy_track.eof_chord_variations_array_ready = 0;
	memset(real_track->eof_chord_variations, 0, sizeof(EOF_CHORD_VARIATION));	//Initialize the chord variations array to undefined
	memset(real_track->eof_chord_num_variations, 0, sizeof(EOF_CHORD_NUM_VARIATION_ARRAY));
	dummy_track.numfrets = real_track->numfrets;
	dummy_track.numstrings = real_track->numstrings;
	memcpy(dummy_track.tuning, real_track->tuning, sizeof(dummy_track.tuning));
	dummy_track.note[0] = malloc(sizeof(EOF_PRO_GUITAR_NOTE));
	if(dummy_track.note[0] == NULL)
	{
		return;
	}
	dummy_track.note[0]->name[0] = '\0';
	dummy_track.notes = 1;
	frets = dummy_track.note[0]->frets;

	//Determine how many lookups will be needed to build this track
	if(dummy_track.numfrets > 11)
	{
		dummy_track.numfrets = 11;	//Note values repeat every 12 half steps, so it's not necessary to go higher
	}
	power = pow(dummy_track.numfrets + 2, dummy_track.numstrings);	//Add 2 because 0xFF (string not played) and 0 (string played open) are valid will be checked as well
	sprintf(eof_log_string, "\tBuilding chord list for \"%s\" (%d strings with %d frets = %lu permutations)", sp->track[track]->name, dummy_track.numstrings, dummy_track.numfrets, (unsigned long)power);
	eof_log(eof_log_string, 1);

	memset(frets, 0xFF, EOF_TUNING_LENGTH);	//Initialize the frets array to all muted
	while(!done)
	{	//Until all permutations of fret values for all available strings have been tested
		debug_total_ctr++;
		debug_ctr++;
		if(debug_ctr >= 1000)
		{	//Every 1000 lookups
			debug_ctr = 0;
			sprintf(eof_log_string, "Building chords for \"%s\" %.2f%%", sp->track[track]->name, (double)(100.0*debug_total_ctr/power));
			set_window_title(eof_log_string);

			#ifdef EOF_CHORD_BUILD_DEBUG
			char string[10];
			sprintf(eof_log_string, "\tTesting [");
			#endif

			dummy_track.note[0]->note = 0;	//Reset the note bitmask so it can be rebuilt
			for(ctr = 0, bitmask = 1; ctr < dummy_track.numstrings; ctr++, bitmask <<= 1)
			{	//For each string in this track
				#ifdef EOF_CHORD_BUILD_DEBUG
				sprintf(string, "%u ", frets[ctr]);
				strcat(eof_log_string, string);
				#endif
				if(frets[ctr] != 0xFF)
				{	//If this string isn't muted
					dummy_track.note[0]->note |= bitmask;	//Mark this string as played
				}
			}
			#ifdef EOF_CHORD_BUILD_DEBUG
			strcat(eof_log_string, "]");
			eof_log(eof_log_string, 1);
			#endif
		}

		//Perform lookup and handle match
		if(eof_lookup_chord(&dummy_track, track, 0, &scale, &chord, &isslash, &bassnote))
		{	//If this fret permutation matches a chord
			scale %= 12;	//Ensure this is between 0 and 11
			array_depth = real_track->eof_chord_num_variations[scale][chord];	//Get the depth of the third dimension of this chord
			if(array_depth < EOF_MAX_CHORD_VARIATIONS)
			{	//If there is enough room in the chord variation array to store this
				bassnote = -1;
				for(ctr = 0; ctr < dummy_track.numstrings; ctr++)
				{	//For each string in this track
					if((bassnote < 0) && (frets[ctr] != 0xFF))
					{	//If this string is used and the lowest used string's fret value hasn't been obtained
						bassnote = eof_lookup_played_note(&dummy_track, track, ctr, frets[ctr]);	//Look up the note played by this string
					}
				}
				if(bassnote >= 0)
				{	//If the bass note was determined
					memcpy(real_track->eof_chord_variations[scale][chord][array_depth].frets, frets, EOF_TUNING_LENGTH);	//Copy the tuning array
					real_track->eof_chord_variations[scale][chord][array_depth].bassnote = bassnote;	//Copy the bass note
					(real_track->eof_chord_num_variations[scale][chord])++;	//Update the depth array to indicate one more entry in the 3D chord variations array
				}
			}
		}

		//Increment fret value
		frets[0]++;		//Increment the fret number on the first string
		for(ctr = 0; ctr < dummy_track.numstrings; ctr++)
		{	//Check the validity of each string's current fret value
			if((frets[ctr] != 0xFF) && (frets[ctr] > dummy_track.numfrets))
			{	//If this fret number overflowed, the increment travels up to next string
				frets[ctr] = 0xFF;	//Reset this string to muted
				if(ctr + 1 < dummy_track.numstrings)
				{	//If there's a next string to pass the increment onto
					frets[ctr+1]++;	//Increment fret number of next string
				}
				else
				{
					done = 1;	//All permutations of all strings have been tested
				}
			}
		}
	}

	free(dummy_track.note[0]);
	real_track->eof_chord_variations_array_ready = 1;	//Mark the chord variations array as ready to use
	eof_fix_window_title();
}

void EOF_DEBUG_OUTPUT_CHORD_VARIATION_ARRAYS(void)
{
	char output_filename[1024] = {0};
	FILE *fp;
	unsigned long ctr, ctr2, ctr3, ctr4, ctr5, chord_counter, track, fretnum;
	EOF_PRO_GUITAR_TRACK *tp;
	int played_note;

	if(!eof_song)
		return;

	get_executable_name(output_filename, 1024);	//Get the path of the EOF binary that is running
	replace_filename(output_filename, output_filename, "eof_chords.txt", 1024);
	fp = fopen(output_filename, "w");

	if(fp)
	{
		for(ctr = 0; ctr < 2; ctr++)
		{	//For each of the pro guitar tracks
			tp = eof_song->pro_guitar_track[ctr];	//Get the pointer to the pro guitar track
			track = EOF_TRACK_PRO_BASS + ctr;

			if(tp->eof_chord_variations_array_ready)
			{	//If the track had its chord variations array built
				fprintf(fp, "\"%s\"\t", tp->parent->name);	//Write the track's name
				//Count the total number of chord variations for this track
				for(ctr2 = 0, chord_counter = 0; ctr2 < 12; ctr2++)
				{	//For each scale
					for(ctr3 = 0; ctr3 < EOF_NUM_DEFINED_CHORDS; ctr3++)
					{	//For each defined chord
						chord_counter += tp->eof_chord_num_variations[ctr2][ctr3];
					}
				}
				fprintf(fp, "%lu chord variations in total\n", chord_counter);
				fprintf(fp, "Tuning:");
				for(ctr2 = 0; ctr2 < tp->numstrings; ctr2++)
				{	//For each string, output the tuning
					played_note = eof_lookup_tuned_note(tp, track, ctr2, tp->tuning[ctr2]);
					fprintf(fp, " %s", eof_note_names[played_note]);
				}
				fprintf(fp, "\n");
				for(ctr2 = 0; ctr2 < 12; ctr2++)
				{	//For each scale
					fprintf(fp, "\tScale of %s:\n", eof_note_names[ctr2]);	//Write the scale name
					for(ctr3 = 0; ctr3 < EOF_NUM_DEFINED_CHORDS; ctr3++)
					{	//For each chord definition
						fprintf(fp, "\t\t%s%s (%s): %u variations\n", eof_note_names[ctr2], eof_chord_names[ctr3].chordname, eof_chord_names[ctr3].formula, tp->eof_chord_num_variations[ctr2][ctr3]);	//Write the chord name
						for(ctr4 = 0; ctr4 < tp->eof_chord_num_variations[ctr2][ctr3]; ctr4++)
						{	//For each variation of this chord that was found
							fprintf(fp, "\t\t\tFrets:  ");
							for(ctr5 = 0; ctr5 < tp->numstrings; ctr5++)
							{	//For each string in the chord entry
								fretnum = tp->eof_chord_variations[ctr2][ctr3][ctr4].frets[ctr5];
								if(fretnum != 0xFF)
								{	//If this string isn't muted
									played_note = eof_lookup_played_note(tp, track, ctr5, fretnum);
									assert((played_note >= 0) && (played_note < 12));
									fprintf(fp, "%lu (%s),", fretnum, eof_note_names[played_note]);
								}
								else
								{
									fprintf(fp, "X ,");
								}
							}
							played_note = tp->eof_chord_variations[ctr2][ctr3][ctr4].bassnote;
							assert((played_note >= 0) && (played_note < 12));
							fprintf(fp, "\tBass note is %s\n", eof_note_names[played_note]);
						}
					}
				}
			}
			else
			{
				fprintf(fp, "\n\"%s\" has no chord variations built", tp->parent->name);
			}

			fprintf(fp, "\n");
		}

		fclose(fp);
	}
}
