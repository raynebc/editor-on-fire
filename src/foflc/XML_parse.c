#include <stdlib.h>
#include <assert.h>
#include "XML_parse.h"
#include "Lyric_storage.h"

#ifdef USEMEMWATCH
#ifdef EOF_BUILD	//In the EOF code base, memwatch.h is at the root
#include "../memwatch.h"
#else
#include "memwatch.h"
#endif
#endif


void XML_Load(FILE *inf)
{
	unsigned long maxlinelength;	//I will count the length of the longest line (including NULL char/newline) in the
									//input file so I can create a buffer large enough to read any line into
	char *buffer;					//Will be an array large enough to hold the largest line of text from input file
	char *substring=NULL;			//Used with strstr() to find tag strings in the input file
	char start=0;					//This will be set to nonzero once the <lyric> tag is read
	unsigned long index=0;			//Used to index within a line of input text
	unsigned long index2=0;			//Used to index within an output buffer
	unsigned long starttime=0;		//Converted long int value of numerical string representing a lyric's start time
	unsigned long stoptime=0;		//Converted long int value of numerical string representing a lyric's stop time
	unsigned long processedctr=0;	//The current line number being processed in the text file
	char textbuffer[101]={0};		//Allow for a 100 character lyric text
	char showread=0, textread=0, removeread=0;	//Tracks whether each of these tags have been read since the last lyric phrase was stored

	assert_wrapper(inf != NULL);	//This must not be NULL

	Lyrics.freestyle_on=1;		//Script is a pitch-less format, so import it as freestyle

//Find the length of the longest line
	maxlinelength=FindLongestLineLength(inf,1);

//Allocate memory buffer large enough to hold any line in this file
	buffer=(char *)malloc_err(maxlinelength);

	fgets_err(buffer,maxlinelength,inf);	//Read first line of text, capping it to prevent buffer overflow

	if(Lyrics.verbose)	printf("\nImporting XML lyrics from file \"%s\"\n\n",Lyrics.infilename);

	processedctr=0;			//This will be set to 1 at the beginning of the main while loop
	while(!feof(inf))		//Until end of file is reached
	{
		processedctr++;
		if(Lyrics.verbose)
			printf("\tProcessing line %lu\n",processedctr);

//Parse the <lyrics> tag
		if(!start)
		{	//If the starting tag hasn't been read yet
			substring=strcasestr_spec(buffer,"<lyrics>");	//Find the character after a match for "<lyrics>", if it exists
			if(substring != NULL)
			{	//This tag indicates the start of lyrics
				puts("\t\t<lyrics> tag encountered");
				start=1;
			}
		}
		else
		{	//It's been read, look for other tags

//Parse the </lyrics> tag
			substring=strcasestr_spec(buffer,"</lyrics>");	//Find the character after a match for "</lyrics>", if it exists
			if(substring != NULL)
			{	//This tag indicates the end of lyrics
				start=0;
				puts("\t\t</lyrics> tag encountered");
				break;
			}

//Parse the <show> tag
			substring=strcasestr_spec(buffer,"<show>");	//Find the character after a match for "<show>", if it exists
			if(substring != NULL)
			{	//This tag indicates the definition of the lyric phrase's start time
				starttime = ParseLongIntXML(substring,processedctr);
				printf("\t\t<show> tag value of %lu parsed\n",starttime);
				showread=1;
			}

//Parse the <text> tag
			substring=strcasestr_spec(buffer,"<text>");	//Find the character after a match for "<text>", if it exists
			if(substring != NULL)
			{	//This tag indicates the definition of the lyric phrase's text
				index=index2=0;	//Reset these indexes
				while(substring[index] != '\0')
				{
					if(substring[index] == '<')	//If the start of a tag was reached, it marks the end of the <text> tag
						break;
					if(index2 > 100)		//If the buffer would overflow by reading another character
						break;			//Truncate the buffered data
					textbuffer[index2++]=substring[index++];	//Copy the character to the buffer
				}
				textbuffer[index2]='\0';	//Terminate the buffer
				printf("\t\t<text> tag with content \"%s\" parsed\n",textbuffer);
				textread=1;
			}

//Parse the <remove> tag
			substring=strcasestr_spec(buffer,"<remove>");	//Find the character after a match for "<remove>", if it exists
			if(substring != NULL)
			{	//This tag indicates the definition of the lyric phrase's stop time
				stoptime = ParseLongIntXML(substring,processedctr);
				printf("\t\t<remove> tag value of %lu parsed\n",stoptime);
				removeread=1;
			}

//Add the lyric if all three tags have been read
			if(showread && textread && removeread)
			{
				printf("\t\tAdding line of lyrics: \"%s\"\n",textbuffer);
				CreateLyricLine();	//Initialize new line of lyrics
				AddLyricPiece(textbuffer,starttime,stoptime,PITCHLESS,0);	//Add lyric, no defined pitch
				EndLyricLine();		//Close the line of lyrics
				showread=textread=removeread=0;	//Reset these statuses
			}
		}

		fgets(buffer,maxlinelength,inf);	//Read next line of text, so the EOF condition can be checked, don't exit on EOF
	}//end while(!feof())

	free(buffer);	//No longer needed, release the memory before exiting function

	ForceEndLyricLine();

	if(Lyrics.verbose)	printf("XML import complete.  %lu lyrics loaded\n\n",Lyrics.piececount);
}

long int ParseLongIntXML(char *buffer,unsigned long linenum)
{
	char numberbuffer[11]={0};	//Allow for a 10 digit number
	unsigned long index=0;			//Used to index within a line of input text
	unsigned long index2=0;			//Used to index within an output buffer
	char *tempstr=NULL;
	long num;

	assert_wrapper(buffer != NULL);
	while(buffer[index] != '\0')
	{
		if(buffer[index] == '<')	//If the start of a tag was reached, it marks the end of the numerical tag
			break;
		if(index2 > 10)			//If the buffer would overflow by reading another character
			break;			//Truncate the buffered data
		numberbuffer[index2++]=buffer[index++];	//Copy the character to the buffer
	}
	numberbuffer[index2]='\0';	//Terminate the buffer
	tempstr=RemoveLeadingZeroes(numberbuffer);	//Create a new string with all leading zeroes removed
	if(tempstr[0] == '0')
	{	//If the string is explicitly 0
		num = 0;
	}
	else
	{	//Convert the string to a number
		num = atol(tempstr);	//Convert string to int
		if(!num)
		{	//atol() returned error
			printf("Error converting numerical string to numerical value.\nError occurs in line %lu of input file\nAborting\n",linenum);
			exit_wrapper(2);
		}
	}
	free(tempstr);
	return num;
}
