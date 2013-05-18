#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "JB_parse.h"
#include "Lyric_storage.h"

#ifdef S_SPLINT_S
//Ensure Splint checks the code for EOF's code base
#define EOF_BUILD
#endif

#ifdef USEMEMWATCH
#ifdef EOF_BUILD	//In the EOF code base, memwatch.h is at the root
#include "../memwatch.h"
#else
#include "memwatch.h"
#endif
#endif

void JB_Load(FILE *inf)
{
	size_t maxlinelength;	//I will count the length of the longest line (including NULL char/newline) in the
									//input file so I can create a buffer large enough to read any line into
	char *buffer;					//Will be an array large enough to hold the largest line of text from input file
	unsigned long index=0;			//Used to index within a line of input text
	unsigned long index2=0;			//Used to index within an output buffer
	char textbuffer[101]={0};	//Allow for a 100 character lyric text
	unsigned long processedctr=0;	//The current line number being processed in the text file
	unsigned long bufferctr=0;	//Ensure textbuffer[] isn't overflowed
	char notename=0;					//Used for parsing note names
	double timestamp=0.0;		//Used to read timestamp
	char linetype=0;		//Is set to one of the following:  1 = lyric, 2 = line break, 3 = end of file
	unsigned char pitch=0;		//Stores the lyric pitch transposed to middle octave

	assert_wrapper(inf != NULL);	//This must not be NULL

//Find the length of the longest line
	maxlinelength=FindLongestLineLength(inf,1);

//Allocate memory buffer large enough to hold any line in this file
	buffer=(char *)malloc_err(maxlinelength);

	(void) fgets_err(buffer,(int)maxlinelength,inf);	//Read first line of text, capping it to prevent buffer overflow

	if(Lyrics.verbose)	printf("\nImporting C9C lyrics from file \"%s\"\n\n",Lyrics.infilename);

	processedctr=0;			//This will be set to 1 at the beginning of the main while loop
	while(!feof(inf))		//Until end of file is reached
	{
		processedctr++;
		if(Lyrics.verbose)
			printf("\tProcessing line %lu\n",processedctr);

		index = 0;
//Read end of file
		if(strcasestr_spec(buffer,"ENDFILE"))
		{	//A line starting with "ENDFILE" denotes the end of the lyric entries
			linetype = 3;
		}

//Read the lyric pitch
		else if(isalpha(buffer[index]))
		{	//A line starting with an alphabetical letter is a normal lyric entry
			linetype = 1;
			notename = toupper(buffer[index++]);

			if(isalpha(buffer[index]))
				index++;	//The first lyric entry seems to repeat the note name

			pitch=60;		//The pitches will be interpreted as ranging from C4 to B4
			switch(notename)	//Add value of note in current octave
			{
				case 'B':
					pitch+=11;
				break;

				case 'A':
				pitch+=9;
				break;

				case 'G':
					pitch+=7;
				break;

				case 'F':
					pitch+=5;
				break;

				case 'E':
					pitch+=4;
				break;

				case 'D':
					pitch+=2;
				break;

				default:
				break;
			}

			if(buffer[index] == '#')
			{	//If the note name is followed by a sharp character,
				pitch++;	//increase the pitch by one half step
				index++;	//Seek past the sharp character
			}

			while(buffer[index] != ':')
			{	//Seek to the expected colon character
				if(buffer[index] == '\0')
				{	//The line ends unexpectedly
					printf("Error: Invalid lyric entry in line %lu during C9C lyric import (colon missing)\nAborting\n",processedctr);
					exit_wrapper(1);
				}
				index++;
			}
			index++;	//Seek beyond the colon

//Read the lyric text
			index2=bufferctr=0;
			while(!isspace(buffer[index]))
			{	//Until whitespace is reached
				if(buffer[index] == '\0')
				{	//The line ends unexpectedly
					printf("Error: Invalid lyric entry in line %lu during C9C lyric import (whitespace missing)\nAborting\n",processedctr);
					exit_wrapper(2);
				}
				textbuffer[index2++] = buffer[index++];	//Copy the character to a buffer
				bufferctr++;
				if(bufferctr == 100)
				{	//Unexpectedly long lyric reached
					printf("Error: Invalid lyric entry in line %lu during C9C lyric import (lyric is too long)\nAborting\n",processedctr);
					exit_wrapper(3);
				}
			}
			textbuffer[index2++] = '\0';	//Terminate the string
		}//A line starting with an alphabetical letter is a normal lyric entry

//Read line break
		else if(buffer[index] == '-')
		{	//A line starting with "--:S" is the start of a period of silence between lyrics (will be treated as a line break)
			linetype = 2;
		}
		else
		{	//Invalid input
			printf("Error: Invalid input \"%s\" in line %lu during C9C import\nAborting\n",&(buffer[index]),processedctr);
			exit_wrapper(4);
		}

//Seek to timestamp
		while(!isdigit(buffer[index]))
		{	//Until a number (the timestamp) is reached
			if(buffer[index] == '\0')
			{	//The line ends unexpectedly
				printf("Error: Invalid line break entry in line %lu during C9C lyric import (timestamp missing)\nAborting\n",processedctr);
				exit_wrapper(5);
			}
			index++;
		}

//Read timestamp
		if(sscanf(&(buffer[index]), "%20lf", &timestamp) != 1)
		{	//Double floating point value didn't parse
			printf("Error: Invalid lyric entry in line %lu during C9C lyric import (error parsing timestamp)\nAborting\n",processedctr);
			exit_wrapper(6);
		}
		timestamp *= 1000.0;	//Convert to milliseconds

//Adjust previous lyric's end position
		if(Lyrics.lastpiece)
		{	//If there was a previous lyric
			unsigned long length;

			assert_wrapper(Lyrics.lastpiece->lyric != NULL);
			length = (unsigned long)strlen(Lyrics.lastpiece->lyric);
			Lyrics.lastpiece->duration = timestamp + 0.5 - Lyrics.realoffset - Lyrics.lastpiece->start;	//Remember to offset start by realoffset, otherwise Lyrics.lastpiece->start could be the larger operand, causing an overflow
			if(Lyrics.lastpiece->lyric[length - 1] == '-')
			{	//If the previous lyric ended in a hyphen, the previous lyric lasts all the way up to the start of this one
				Lyrics.lastpiece->groupswithnext=1;	//The previous lyric piece will group with this one
			}
			else
			{	//Otherwise space out the lyrics a bit, 1/32 second was suggested
				if(Lyrics.lastpiece->duration > 31)
					Lyrics.lastpiece->duration -= 31;	//31ms ~= 1 sec/32
			}
		}

//Add lyric
		if(linetype == 1)	//If this line defined a new lyric
		{
			//Track for pitch changes, enabling Lyrics.pitch_tracking if applicable
			if((Lyrics.last_pitch != 0) && (Lyrics.last_pitch != pitch))	//There's a pitch change
				Lyrics.pitch_tracking=1;
			Lyrics.last_pitch=pitch;	//Consider this the last defined pitch

			if(Lyrics.line_on != 1)	//If we're at this point, there should be a line of lyrics in progress
				CreateLyricLine();

			AddLyricPiece(textbuffer,timestamp + 0.5,timestamp + 0.5,pitch,0);	//Add lyric with no defined duration
		}

//Add line break
		else if(linetype == 2)
		{	//If this line defined a line break
			EndLyricLine();
			Lyrics.lastpiece = NULL;	//Prevent the first lyric from the new line from altering the previous lyric's duration, which was set by the line break position
		}

//End processing
		else
			break;

		(void) fgets(buffer,(int)maxlinelength,inf);	//Read next line of text, so the EOF condition can be checked, don't exit on EOF
	}//Until end of file is reached

	free(buffer);	//No longer needed, release the memory before exiting function

	ForceEndLyricLine();
	RecountLineVars(Lyrics.lines);	//Rebuild line durations since this lyric format required adjusting timestamps after lines were parsed

	if(Lyrics.verbose)	printf("C9C import complete.  %lu lyrics loaded\n\n",Lyrics.piececount);
}
