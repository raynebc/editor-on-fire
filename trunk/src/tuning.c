#include "tuning.h"
#include "stdio.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

char *eof_note_names[12] =			{"A","A#","B","C","C#","D","D#","E","F","F#","G","G#"};
char *eof_major_scale_names_flat[12] =	{"A","Bb","B","C","Db","D","Eb","E","F","Gb","G","Ab"};		//The name of each scale in order from 0 (0 semitones above A) to 11 (11 semitones above A)
char *eof_major_scale_names_sharp[12] =	{"A","A#","B","C","C#","D","D#","E","F","F#","G","G#"};		//The name of each scale in order from 0 (0 semitones above A) to 11 (11 semitones above A)
char **eof_major_scale_names =	eof_major_scale_names_sharp;	//By default, display scales with sharp accidentals
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

char *eof_lookup_tuning(EOF_SONG *sp, unsigned long track, char *tuning)
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

int eof_lookup_default_string_tuning(EOF_SONG *sp, unsigned long track, unsigned long stringnum)
{	//A A# B C C# D D# E F F# G  G#
	//0 1  2 3 4  5 6  7 8 9  10 11
	unsigned long tracknum;
	int default_tuning_6_string[] = {7,0,5,10,2,7};		//Half steps above note A, representing "EADGBE"
	int default_tuning_4_string_bass[] = {7,0,5,10};	//Half steps above note A, representing "EADG";
	int default_tuning_5_string_bass[] = {2,7,0,5,10};	//Half steps above note A, representing "BEADG";
	int default_tuning_6_string_bass[] = {2,7,0,5,10,3};//Half steps above note A, representing "BEADGC";

	if((sp == NULL) || (track >= sp->tracks) || (sp->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return -1;	//Invalid song pointer or track number
	tracknum = sp->track[track]->tracknum;
	if(stringnum >= sp->pro_guitar_track[tracknum]->numstrings)
		return -1;	//Invalid string number
	if(track == EOF_TRACK_PRO_GUITAR)
	{	//Standard tuning is EADGBe
		return default_tuning_6_string[stringnum];	//Return this string's default tuning
	}
	else if(track == EOF_TRACK_PRO_BASS)
	{
		if(sp->pro_guitar_track[tracknum]->numstrings == 4)
		{	//Standard tuning for 4 string bass is EADG
			return default_tuning_4_string_bass[stringnum];	//Return this string's default tuning
		}
		else if(sp->pro_guitar_track[tracknum]->numstrings == 5)
		{	//Standard tuning for 5 string bass is BEADG
			return default_tuning_5_string_bass[stringnum];	//Return this string's default tuning
		}
		else if(sp->pro_guitar_track[tracknum]->numstrings == 6)
		{	//Standard tuning for 6 string bass is BEADGC
			return default_tuning_6_string_bass[stringnum];	//Return this string's default tuning
		}
	}

	return -1;	//Unsupported pro guitar track
}

int eof_lookup_tuned_note(EOF_SONG *sp, unsigned long track, unsigned long stringnum, int halfsteps)
{
	unsigned long tracknum;
	int notenum;

	if((sp == NULL) || (track >= sp->tracks) || (sp->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return -1;	//Invalid song pointer or track number
	tracknum = sp->track[track]->tracknum;
	if(stringnum >= sp->pro_guitar_track[tracknum]->numstrings)
		return -1;	//Invalid string number

	notenum = eof_lookup_default_string_tuning(sp, track, stringnum);	//Look up the default tuning for this string
	if(notenum < 0)	//If lookup failed,
		return -1;	//Return error
	notenum += halfsteps;	//Adjust for the number of half steps above/below the default tuning

	notenum %= 12;
	if(notenum < 0)		//Handle for down tuning
		notenum += 12;

	return (notenum % 12);	//Return the note value in terms of half steps above note A
}

int eof_lookup_played_note(EOF_SONG *sp, unsigned long track, unsigned long stringnum, unsigned long fretnum)
{
	unsigned long tracknum;
	int notenum;

	if((sp == NULL) || (track >= sp->tracks) || (sp->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return -1;	//Invalid song pointer or track number
	tracknum = sp->track[track]->tracknum;
	if((stringnum >= sp->pro_guitar_track[tracknum]->numstrings) || (fretnum > sp->pro_guitar_track[tracknum]->numfrets))
		return -1;	//Invalid string number or fret number

	notenum = eof_lookup_tuned_note(sp, track, stringnum, sp->pro_guitar_track[tracknum]->tuning[stringnum]);	//Look up the open note this string plays
	if(notenum < 0)	//If lookup failed,
		return -1;	//Return error
	notenum += fretnum;	//Take the fret number into account

	return (notenum % 12);	//Return the note value in terms of half steps above note A
}

int eof_lookup_chord(EOF_SONG *sp, unsigned long track, unsigned long note, int *scale, int *chord)
{
	unsigned long tracknum, bitmask;
	static char major_scales[12][7];
	char notes_played[12] = {0};   	//Will indicate which notes are present in the chord
	static char scales_created = 0;	//This will be set to one after the major_scales[] array has been created
	char chord_intervals[30];		//This stores the referred note's interval makeup for the current scale being checked against
	char interval_name[5];
	unsigned long ctr, ctr2, ctr3, halfstep, skipaccidental;
	int retval;

	if((sp == NULL) || (track >= sp->tracks) || (note >= eof_get_track_size(sp, track)) || !scale || !chord)
		return 0;	//Return error if any of the parameters are not valid

	if(sp->track[track]->track_behavior != EOF_PRO_GUITAR_TRACK_BEHAVIOR)
		return 0;	//Return error if the track is not supported
	tracknum = sp->track[track]->tracknum;

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

	//Determine which musical notes are contained in the specified note
	for(ctr = 0, bitmask = 1; ctr < sp->pro_guitar_track[tracknum]->numstrings; ctr++, bitmask <<= 1)
	{	//For each string in this track
		if(sp->pro_guitar_track[tracknum]->note[note]->note & bitmask)
		{	//If this string is used in the note
			retval = eof_lookup_played_note(sp, track, ctr, sp->pro_guitar_track[tracknum]->note[note]->frets[ctr]);	//Look up the note played on this string at the specified fret
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
		for(ctr2 = 0; ctr2 < 7; ctr2++)
		{	//For each interval in the scale
			//See if any normal/flat/sharp variations of this interval is played
			interval_name[0] = '\0';	//Empty the interval name string
			halfstep = (major_scales[ctr][ctr2] + 11) % 12;	//Look up one half step lower than the note for this interval (as a value from 0 to 11)
			if(notes_played[halfstep])
			{	//If the flat of this interval is played in this note
				skipaccidental = 0;	//Reset this status
				for(ctr3 = 0; ctr3 < 7; ctr3++)
				{	//Check to make sure the flat of this interval isn't already a non accidental interval (ie. In the A major scale, interval 3 and flat interval 4 are the same note)
					if(halfstep == major_scales[ctr][ctr3])
					{	//Skip checking for the flat variation of this interval
						skipaccidental = 1;
						break;
					}
				}
				if(!skipaccidental)
				{	//If the flat of this interval is played in this note
					snprintf(interval_name, 5, "b%lu", ctr2+1);	//Use the flat variation of this interval (numbering starts at 1 instead of 0)
				}
			}
			else
			{
				halfstep = (halfstep + 2) % 12;	//Look up one half step higher than the note for this interval (two half steps above the previous flat, as a value from 0 to 11)
				if((notes_played[halfstep]) && (ctr2 == 4))
				{	//If the sharp of this interval is played in this note (only check interval 5 for sharp interval variations to avoid matching issues)
					skipaccidental = 0;	//Reset this status
					for(ctr3 = 0; ctr3 < 7; ctr3++)
					{	//Check to make sure the sharp of this interval isn't already a non accidental interval (ie. In the A major scale, sharp interval 3 and interval 4 are the same note)
						if(halfstep == major_scales[ctr][ctr3])
						{	//Skip checking for the flat variation of this interval
							skipaccidental = 1;
							break;
						}
					}
					if(!skipaccidental)
					{	//If the flat of this interval is played in this note
						snprintf(interval_name, 5, "#%lu", ctr2+1);	//Use the sharp variation of this interval (numbering starts at 1 instead of 0)
					}
				}
				else
				{
					halfstep = (halfstep + 11) % 12;	//Look up the note for this interval (one half step below the previous sharp, as a value from 0 to 11)
					if(notes_played[halfstep])
					{	//If the normal interval is played in this note
						snprintf(interval_name, 5, "%lu", ctr2+1);	//Use this interval (numbering starts at 1 instead of 0)
					}
				}
			}
			//If a variation of the interval was found, add it to the intervals list
			if(interval_name[0] != '\0')
			{	//If a normal/flat/sharp variation of this interval exists in the note,
				if(chord_intervals[0] != '\0')
				{	//If this isn't the first interval stored in the array
					strcat(chord_intervals, ",");	//append a comma first
				}
				strcat(chord_intervals, interval_name);	//append the interval variation to the chord_intervals[] string
			}
		}
		//Look up the list of defined chords to see if this note's intervals matches any
		for(ctr2 = 0; ctr2 < EOF_NUM_DEFINED_CHORDS; ctr2++)
		{	//For each defiend chord
			if(!strcmp(eof_chord_names[ctr2].formula, chord_intervals))
			{	//A match was found
				*scale = ctr;	//Pass the scale back through the pointer
				*chord = ctr2;	//Pass the chord name back through the pointer
				return 1;		//Return match found
			}
		}
	}

	return 0;	//Return no match found
}
