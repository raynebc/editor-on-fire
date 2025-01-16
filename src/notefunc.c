#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "notefunc.h"

char* notenames = "C\0 C#\0D\0 D#\0E\0 F\0 F#\0G\0 G#\0A\0 A#\0B\0 ";
char* notelist = "CxDxEFxGxAxB";
char returnval[10];

/**
 * Calculate the frequency (in Hz) of a given 12-tone note name
 *
 * Input is notename and octave number, with # and b as accidentals.
 * If it can't parse the note, it returns A4 (440Hz)
 *
 * @param note The note name, up to three characters, e.g. C#4
 * @return The frequency, in Hz, of the given note
 */
double notefunc_note_to_freq(char* note)
{
	char notename;
	char octavestr[10];
	int octavenum;
	char accidental = 0;
	int adj = 0;
	size_t notenum = 0, numnotes = strlen(notelist);

	notename = note[0];
	if(strlen(note) == 2)
	{
		strncpy(octavestr, note + 1, sizeof(octavestr) - 1);
		accidental = 0;
	}
	else if(strlen(note) == 3)
	{
		strncpy(octavestr, note + 2, sizeof(octavestr) - 1);
		accidental = note[1];
	}
	else
	{ //Couldn't parse, just return A4
		return 440;
	}

	octavestr[9] = '\0';	//Ensure NULL termination
	octavenum = atoi(octavestr);			//Parse the octave
	if(notename > 90)
	{ //Upper-case the note
		notename -= 32;
	}

	//fprintf(stderr,"%c,%d,%c\n",notename,octavenum,accidental);

	for(notenum=0;notenum < numnotes;notenum++)
	{ //Find the base note number
		if(notelist[notenum] == notename)
			break;
	}
	if(notenum == numnotes)
	{
		return 440;
	}	//Again, not a note we know!

	//Adjust for accidentals
	if(accidental)
	{
		if(accidental == '#')
		{
			adj = 1;
		}
		else if(accidental == 'b')
		{
			adj = -1;
		}
		if(!notenum && (adj < 0))
			notenum = numnotes - 1;   //Special case:  The flat of C will be treated as B
		else
			notenum = (notenum + adj) % numnotes;
	}

	//Round to two decimal places to avoid floating point weirdness
	return round(BASE_FREQ * pow(2,(double)octavenum+(notenum/12.0)) *100)/100;
}

/**
 * Get the closest 12-tone note name to a given frequency
 *
 * Currently always just outputs sharps for enharmonics
 *
 * @param freq The frequency in Hz to be matched
 * @return A string representation of the closest note
 */
char* notefunc_freq_to_note(double freq)
{
	double logof2 = log(2.0);
	double midval = round(fabs(log(freq/BASE_FREQ)/logof2) *100)/100;
	int octavenum = floor(midval);
	int notenum = round(12.0*(midval - octavenum));

	//fprintf(stderr,"%f(%f):%d,%d\n",freq,midval,octavenum,notenum);
	(void) snprintf(returnval, sizeof(returnval) - 1, "%s%d", notenames+notenum*3, octavenum);
	return returnval;
}
