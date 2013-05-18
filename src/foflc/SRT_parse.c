#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "Lyric_storage.h"
#include "SRT_parse.h"

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


int Write_SRT_Timestamp(FILE *outf,unsigned long timestamp)
{
	unsigned long hours=0,minutes=0,seconds=0,millis=0;

	if(outf == NULL)
		return -1;	//Return error

	hours = timestamp / 3600000;	//Calculate the hours field
	timestamp -= (hours * 3600000);
	minutes = timestamp / 60000;	//Calculate the minutes field
	timestamp -= (minutes * 60000);
	seconds = timestamp / 1000;	//Calculate the seconds field
	timestamp -= (seconds * 1000);
	millis = timestamp;		//Calculate the milliseconds field

	if(fprintf(outf,"%02lu:%02lu:%02lu,%03lu",hours,minutes,seconds,millis) < 0)
		return -1;	//Return error

	return 0;	//Return success
}

void Export_SRT(FILE *outf)
{
	struct Lyric_Line *curline=NULL;	//Conductor of the lyric line linked list
	struct Lyric_Piece *temp=NULL;		//A conductor for the lyric pieces list
	unsigned long ctr=1;			//The lyric counter

	assert_wrapper(outf != NULL);	//This must not be NULL
	assert_wrapper(Lyrics.piececount != 0);	//This function is not to be called with an empty Lyrics structure

	if(Lyrics.verbose)	printf("\nExporting SRT subtitles to file \"%s\"\n\nWriting subtitles\n",Lyrics.outfilename);


//Export the lyric pieces
	curline=Lyrics.lines;	//Point lyric line conductor to first line of lyrics
	while(curline != NULL)	//For each line of lyrics
	{
		temp=curline->pieces;	//Starting with the first piece of lyric in this line

		if(Lyrics.verbose)	printf("\tSubtitle line %lu: ",ctr);

		while(temp != NULL)				//For each piece of lyric in this line
		{
			if(ctr != 1)
			{	//All subtitles after the first are prefixed with two newline characters
				if(fprintf(outf,"\n\n") < 0)
				{
					printf("Error writing double space between subtitles: %s\nAborting\n",strerror(errno));
					exit_wrapper(1);
				}
			}
			if((fprintf(outf,"%lu\n",ctr) < 0) || Write_SRT_Timestamp(outf,temp->start) || (fprintf(outf," --> ") < 0) || Write_SRT_Timestamp(outf,temp->start+temp->duration) || (fprintf(outf,"\n%s",temp->lyric) < 0))
			{	//Write the subtitle number, the start timestamp, the "-->" separator, the end timestamp and the subtitle text
				printf("Error exporting lyric %lu: %s\nAborting\n",ctr,strerror(errno));
				exit_wrapper(2);
			}

			if(Lyrics.verbose)	printf("'%s'",temp->lyric);

			temp=temp->next;	//Advance to next lyric piece as normal
			ctr++;
		}//end while(temp != NULL)

		curline=curline->next;	//Advance to next line of lyrics

		if(Lyrics.verbose)	(void) putchar('\n');
	}

	if(Lyrics.verbose)	printf("\nSRT export complete.  %lu subtitles written",Lyrics.piececount);
}

char *SeekNextSRTTimestamp(char *ptr)	//find next occurence of a timestamp in hh:mm:ss,mmm format, return pointer to beginning of timestamp, or NULL if no timestamp is found
{
	char *temp=NULL;		//Parsing will set this to the first character of a potential timestamp (ie. '[')
	unsigned int ctr=0;		//Used for indexing past temp
	unsigned int ctr2=0;	//Used for counting the number of characters in each field
	char retry=0;			//Boolean: Parsing indicated the the loop determined that ptr didn't point at a timestamp, begin again at the main while loop

	if(ptr == NULL)
		return NULL;

	temp=ptr;	//begin at the given pointer

	while(1)
	{
		retry=0;	//Reset this condition
		ctr=1;		//Once start of timestamp is found, this will index to next char

//find start of timestamp
		while(temp[0] != '\0')		//Check all characters until end of string
			if(isdigit(temp[0]))	//If this character is a number
				break;
			else
				temp++;	//Seek to next character

		if(temp[0] == '\0')	//If end of string was reached without reaching a potential timestamp
			return NULL;	//return on failure

//validate hours portion of timestamp
		ctr2=0;
		while(!retry)
		{
			if((temp[ctr] == '\0') || (!isdigit(temp[ctr]) && (temp[ctr] != ':')))	//Numerical char(s) followed by delimiter are expected
				retry=1;	//Signal to stop processing at ptr and find next [ character
			else			//Look for delimiter and increment index to look at next character
			{
				if(temp[ctr++] == ':')	//If this character is a valid timestamp field delimiter
					break;		//If it was colon, break from loop
				else			//If it was a number
				{
					ctr2++;		//Increment the field length counter
					if(ctr2 > SRTTIMESTAMPMAXFIELDLENGTH)	//If this is longer than the defined limit of chars
						retry=1;	//Signal to stop processing at ptr and find next [ character
				}
			}
		}

//validate minutes portion of timestamp
		ctr2=0;
		while(!retry)
		{
			if((temp[ctr] == '\0') || (!isdigit(temp[ctr]) && (temp[ctr] != ':')))	//Numerical char(s) followed by delimiter are expected
				retry=1;
			else		//Look for delimiter and increment index to look at next character
			{
				if(temp[ctr++] == ':')	//If this character is a valid timestamp field delimiter
					break;		//If it was colon, break from loop
				else			//If it was a number
				{
					ctr2++;		//Increment the field length counter
					if(ctr2 > SRTTIMESTAMPMAXFIELDLENGTH)	//If this is longer than the defined limit of chars
						retry=1;	//Signal to stop processing at ptr and find next [ character
				}
			}
		}

//validate seconds portion of timestamp
		ctr2=0;
		while(!retry)
		{
			if((temp[ctr] == '\0') || (!isdigit(temp[ctr]) && (temp[ctr] != ',')))	//Numerical char(s) followed by delimiter are expected
				retry=1;
			else		//Look for delimiter and increment index to look at next character
			{
				if(temp[ctr++] == ',')	//If this character is a valid timestamp field delimiter
					break;		//If it was comma, break from loop
				else			//If it was a number
				{
					ctr2++;		//Increment the field length counter
					if(ctr2 > SRTTIMESTAMPMAXFIELDLENGTH)	//If this is longer than the defined limit of chars
						retry=1;	//Signal to stop processing at ptr and find next [ character
				}
			}
		}

//validate milliseconds portion of timestamp
		ctr2=0;
		while(!retry)
		{
			if((temp[ctr] == '\0') || (!isdigit(temp[ctr])))	//If not a numerical character
			{
				if(isspace(temp[ctr]) || (temp[ctr] == '\0'))//If this character is a whitespace/newline/carriage return/NULL character
					break;	//break from loop
				else
					retry=1;
			}
			else
			{
				ctr++;	//seek to next character
				ctr2++;	//Increment the field length counter
				if(ctr2 > SRTTIMESTAMPMAXFIELDLENGTH)	//If this is longer than the defined limit of chars
					retry=1;	//Signal to stop processing at ptr and find next [ character
			}
		}

		if(retry)		//If parsing failed
			temp++;		//Seek past this timestamp starting character to look for the next
		else			//If a suitable timestamp was found
			return temp;	//Return pointer
	}
}

unsigned long ConvertSRTTimestamp(char **ptr,int *errorstatus)
{
	char *temp=NULL;
	unsigned int ctr=1;
	char failed=0;	//Boolean: Parsing indicated that ptr didn't point at a valid timestamp, abort
	char hours[SRTTIMESTAMPMAXFIELDLENGTH+1]={0};		//Allow for pre-defined # of hours chars (and NULL terminator)
	char minutes[SRTTIMESTAMPMAXFIELDLENGTH+1]={0};		//Allow for pre-defined # of minute chars (and NULL terminator)
	char seconds[SRTTIMESTAMPMAXFIELDLENGTH+1]={0};		//Allow for pre-defined # of seconds chars (and NULL terminator)
	char millis[SRTTIMESTAMPMAXFIELDLENGTH+1]={0};		//Allow for pre-defined # of milliseconds chars (and NULL terminator)
	unsigned int index=0;	//index variable into the 3 timestamp strings
	unsigned long sum=0;
	long conversion=0;	//Will store the integer conversions of each of the 3 timestamp strings

	if(ptr == NULL)
		failed=1;
	else
	{
		temp=*ptr;	//To improve readability for string parsing

		if(temp == NULL)
			failed=1;
	}

//Validate that the string has brackets
	if((temp != NULL) && !isdigit(temp[0]))	//If this character is not a beginning character for a timestamp
		failed=1;

//Parse hours portion of timestamp
	index=0;
	while(!failed)
	{
		assert_wrapper(temp != NULL);
		if((temp[ctr] == '\0') || (!isdigit(temp[ctr]) && (temp[ctr] != ':')))	//Numerical char(s) followed by delimiter are expected
			failed=1;
		else
		{
			if(isdigit(temp[ctr]))	//Is a numerical character
			{
				if(index > SRTTIMESTAMPMAXFIELDLENGTH)	//If more than the defined limit of chars have been parsed
					failed=2;	//This is more than we allow for
				else
					hours[index++]=temp[ctr++];	//copy character into hours string, increment indexes
			}
			else
			{			//this character is a delimiter
				ctr++;	//seek past the colon
				break;	//break from loop
			}
		}
	}
	assert_wrapper(index <= SRTTIMESTAMPMAXFIELDLENGTH+1);	//Ensure that writing the NULL character won't overflow
	hours[index]='\0';	//Terminate hours string

//validate minutes portion of timestamp
	index=0;
	while(!failed)
	{
		assert_wrapper(temp != NULL);
		if((temp[ctr] == '\0') || (!isdigit(temp[ctr]) && (temp[ctr] != ':')))	//Numerical char(s) followed by delimiter are expected
			failed=1;
		else
		{
			if(isdigit(temp[ctr]))	//Is a numerical character
			{
				if(index > SRTTIMESTAMPMAXFIELDLENGTH)	//If more than the defined limit of chars have been parsed
					failed=2;	//This is more than we allow for
				else
					minutes[index++]=temp[ctr++];	//copy character into minutes string, increment indexes
			}
			else
			{			//this character is a delimiter
				ctr++;	//seek past the colon
				break;	//break from loop
			}
		}
	}
	assert_wrapper(index <= SRTTIMESTAMPMAXFIELDLENGTH+1);	//Ensure that writing the NULL character won't overflow
	minutes[index]='\0';	//Terminate minutes string

//validate seconds portion of timestamp
	index=0;
	while(!failed)
	{
		assert_wrapper(temp != NULL);
		if((temp[ctr] == '\0') || (!isdigit(temp[ctr]) && (temp[ctr] != ',')))	//Numerical char(s) followed by delimiter are expected
			failed=1;
		else
		{
			if(isdigit(temp[ctr]))	//Is a numerical character
			{
				if(index > SRTTIMESTAMPMAXFIELDLENGTH)	//If more than the defined limit of chars have been parsed
					failed=2;	//This is more than we allow for
				else
					seconds[index++]=temp[ctr++];	//copy character into seconds string, increment indexes
			}
			else
			{			//this character is a delimiter
				ctr++;	//seek past the colon
				break;	//break from loop
			}
		}
	}
	assert_wrapper(index <= SRTTIMESTAMPMAXFIELDLENGTH+1);	//Ensure that writing the NULL character won't overflow
	seconds[index]='\0';	//Terminate seconds string

//validate milliseconds portion of timestamp
	index=0;
	while(!failed)
	{
		assert_wrapper(temp != NULL);
		if((temp[ctr] == '\0') || !isdigit(temp[ctr]))	//If not a numerical character
		{
			if(isspace(temp[ctr]) || (temp[ctr] == '\0'))//If this character is a whitespace/newline/carriage return/NULL character
				break;	//break from loop
			else
				failed=1;
		}
		else
		{
			if(index > SRTTIMESTAMPMAXFIELDLENGTH)	//If more than the defined limit of chars have been parsed
				failed=2;
			else
				millis[index++]=temp[ctr++];	//copy character into milliseconds string, increment indexes
		}
	}
	assert_wrapper(index <= SRTTIMESTAMPMAXFIELDLENGTH+1);	//Ensure that writing the NULL character won't overflow
	millis[index]='\0';	//Terminate milliseconds string

	if(failed)		//If parsing failed
	{
		printf("Error: Invalid timestamp \"%s\"\n",temp);
		if(errorstatus != NULL)
		{
			*errorstatus=1;
			return 0;
		}
		else
		{
			(void) puts("Aborting");
			exit_wrapper(1);
		}
	}

	assert_wrapper((ptr != NULL) && (temp != NULL));
	*ptr=&(temp[ctr+1]);	//Store address of first character following end of timestamp


//Convert hours string to integer and add to sum
	temp=RemoveLeadingZeroes(hours);
	if(temp[0] != '0')	//If hours is not 0
	{
		conversion=atol(temp);	//get integer conversion
		if(conversion<1)	//Values of 0 are errors from atol(), negative values are not allowed for timestamps
		{
			(void) puts("Error converting string to integer\nAborting");
		if(errorstatus != NULL)
		{
			*errorstatus=2;
			return 0;
		}
		else
			exit_wrapper(2);
		}
		sum+=conversion*3600000;	//one hour is 3600000 milliseconds
	}
	free(temp);

//Convert minutes string to integer and add to sum
	temp=RemoveLeadingZeroes(minutes);
	if(temp[0] != '0')	//If minutes is not 0
	{
		conversion=atol(temp);	//get integer conversion
		if(conversion<1)	//Values of 0 are errors from atol(), negative values are not allowed for timestamps
		{
			(void) puts("Error converting string to integer\nAborting");
		if(errorstatus != NULL)
		{
			*errorstatus=2;
			return 0;
		}
		else
			exit_wrapper(2);
		}
		sum+=conversion*60000;	//one minute is 60,000 milliseconds
	}
	free(temp);

//Convert seconds string to integer and add to sum
	temp=RemoveLeadingZeroes(seconds);
	if(temp[0] != '0')	//If minutes is not 0
	{
		conversion=atol(temp);	//get integer conversion
		if(conversion<1)	//Values of 0 are errors from atol(), negative values are not allowed for timestamps
		{
			(void) puts("Error converting string to integer\nAborting");
			if(errorstatus != NULL)
			{
				*errorstatus=3;
				return 0;
			}
			else
				exit_wrapper(3);
		}
		sum+=conversion*1000;	//one second is 1,000 milliseconds
	}
	free(temp);

//Convert milliseconds string to integer and add to sum
	temp=RemoveLeadingZeroes(millis);
	if(temp[0] != '0')	//If minutes is not 0
	{
		conversion=atol(temp);	//get integer conversion
		if(conversion<1)	//Values of 0 are errors from atol(), negative values are not allowed for timestamps
		{
			(void) puts("Error converting string to integer\nAborting");
			if(errorstatus != NULL)
			{
				*errorstatus=4;
				return 0;
			}
			else
				exit_wrapper(4);
		}
		sum+=conversion;
	}
	free(temp);
	return sum;
}

void SRT_Load(FILE *inf)
{
	char *buffer;		//Buffer used to read from input file
	char *temp=NULL;	//Used for string processing
	unsigned long processedctr=0;	//The current line number being processed in the text file
	size_t maxlinelength;	//I will count the length of the longest line (including NULL char/newline) in the
									//input file so I can create a buffer large enough to read any line into
	unsigned long startstamp=0,endstamp=0;
	unsigned long ctr=0;

	assert_wrapper(inf != NULL);	//This must not be NULL

//Find the length of the longest line
	maxlinelength=FindLongestLineLength(inf,1);

//Allocate buffers to read file line by line
	buffer=malloc_err(maxlinelength);

//Process each line of input file
	if(Lyrics.verbose)
		printf("\nImporting SRT subtitles from file \"%s\"\n\n",Lyrics.infilename);

	processedctr=0;			//This will be set to 1 at the beginning of the main while loop
	while(fgets(buffer,(int)maxlinelength,inf) != NULL)		//Read lines until end of file is reached, don't exit on EOF
	{
		processedctr++;

//Find first timestamp in this line
		temp=SeekNextSRTTimestamp(buffer);	//Point temp to first timestamp
		if(temp == NULL)	//If there is none, skip this line
			continue;	//Skip processing and read next line
		startstamp=ConvertSRTTimestamp(&temp,NULL);

//Find second timestamp in this line
		temp=SeekNextSRTTimestamp(temp);	//Point temp to second timestamp
		if(temp == NULL)
		{
			if(Lyrics.verbose)	printf("Warning: Line #%lu does not contain the ending timestamp.  Ignoring\n",processedctr);
			continue;	//Skip processing and read next line
		}
		endstamp=ConvertSRTTimestamp(&temp,NULL);

//Read next line, which is expected to be the subtitle entry
		if(fgets(buffer,(int)maxlinelength,inf) == NULL)
			break;	//If another line couldn't be read, exit loop

		ctr = (unsigned long)strlen(buffer);	//Find index of the string's NULL terminator
		while((ctr > 0) && ((buffer[ctr-1] == '\n') || (buffer[ctr-1] == '\r')))
		{	//If the string isn't empty and the last character is a newline or carriage return
			buffer[ctr-1] = '\0';	//Truncate it from the string
			ctr--;					//Track the position of the end of the string
		}

//Add lyric piece as a lyric line
		CreateLyricLine();
		AddLyricPiece(buffer,startstamp,endstamp,PITCHLESS,0);	//Write lyric with no defined pitch
		EndLyricLine();
	}//end while(fgets(buffer,maxlinelength,inf) != NULL)

	ForceEndLyricLine();

//Release memory buffers and return
	free(buffer);
	if(Lyrics.verbose)	printf("SRT import complete.  %lu subtitles loaded\n\n",Lyrics.piececount);
}
