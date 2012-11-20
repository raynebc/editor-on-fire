//Note:	A non extended LRC file will only have a timestamp to define the start of the line, which should last
//		until the timestamp at the next line

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "LRC_parse.h"
#include "Lyric_storage.h"

#ifdef USEMEMWATCH
#ifdef EOF_BUILD	//In the EOF code base, memwatch.h is at the root
#include "../memwatch.h"
#else
#include "memwatch.h"
#endif
#endif


static char timestampbeg[]="[<";	//Accept any of these characters as valid characters to begin an LRC timestamp
static char timestampend[]="]>";	//Accept any of these characters as valid characters to end an LRC timestamp
static char timestampdel[]=":.";	//Accept any of these characters as valid characters to delimit LRC timestamp fields


void LRC_Load(FILE *inf)
{
	char *buffer,*buffer2;			//Buffers used to read from input file
	char *temp=NULL,*temp2=NULL;	//Used for string processing
	unsigned long ctr=0;
	unsigned long processedctr=0;	//The current line number being processed in the text file
	size_t maxlinelength;	//I will count the length of the longest line (including NULL char/newline) in the
									//input file so I can create a buffer large enough to read any line into
	unsigned long startstamp=0,endstamp=0,timestamp=0;
	char lyricinprogress=0;
					//Boolean: An ending timestamp was not found for the last piece.  An extended LRC
					//file should properly insert timestamps so that no lines are ended with an ending
					//timestamp missing from the last lyric piece.  A normal LRC file would typically
					//have no ending timestamp defined for any line of lyrics.  In such a case that an
					//end of line was reached without an ending timestamp, the start timestamp for the
					//next lyric piece should also be used to end the piece.  The last line in a normal
					//LRC file is not expected to provide a final ending timestamp.  One will need to be
					//be arbitrarily assumed (ie. by finding the mean duration of all other lyric pieces)
	char lineinprogress=0;
					//Boolean:  Last line of lyrics didn't end in a timestamp, and a timestamp is expected

	//These are used to calculate mean lyric duration if there is not an ending timestamp for the last lyric
	double mean=0.0;
	struct Lyric_Piece *temp3=NULL;
	struct Lyric_Line *templine=NULL;
	char groupswithnext=0;	//Tracks grouping, passed to AddLyricPiece()

	assert_wrapper(inf != NULL);	//This must not be NULL

//Find the length of the longest line
	maxlinelength=FindLongestLineLength(inf,1);

//Allocate buffers to read file line by line
	buffer=malloc_err(maxlinelength);
	buffer2=malloc_err(maxlinelength+1);	//Allow one extra character for inserting a hyphen

//Process each line of input file
	if(Lyrics.verbose)
		printf("\nImporting LRC lyrics from file \"%s\"\n\n",Lyrics.infilename);

//Look for tags
	Lyrics.TitleStringID=DuplicateString("[ti");
	Lyrics.ArtistStringID=DuplicateString("[ar");
	Lyrics.AlbumStringID=DuplicateString("[al");
	Lyrics.EditorStringID=DuplicateString("[by");
	Lyrics.OffsetStringID=DuplicateString("[offset");

	processedctr=0;			//This will be set to 1 at the beginning of the main while loop
	while(fgets(buffer,(int)maxlinelength,inf) != NULL)		//Read lines until end of file is reached, don't exit on EOF
	{
		processedctr++;

		temp2=strchr(buffer,'\n');	//Find first instance of newline char
		if(temp2 != NULL)
			temp2[0] = '\0';		//Truncate \n from line

		if(Lyrics.verbose)
			printf("Processing line #%lu: \"%s\"\n",processedctr,buffer);

		if(ParseTag(':',']',buffer,0))		//Look for tags, content starts after ':' char and extends to the following ']' char.  Do not negatize the offset
			continue;						//If a tag was processed, the rest of the line should be skipped

//Find first timestamp in this line
		temp=SeekNextLRCTimestamp(buffer);	//Point temp to first timestamp
		if(temp == NULL)
		{
			if(Lyrics.verbose)	printf("Warning: Line #%lu does not contain any timestamps.  Ignoring\n",processedctr);
			continue;	//Skip processing and read next line
		}

		if(lineinprogress == 0)
			CreateLyricLine();

		while(1)	//Loop to process timestamps and lyrics until end of line is reached
		{
			groupswithnext=0;	//Reset this condition

//Convert timestamp to integer and seek to first character past the timestamp
			if(SeekNextLRCTimestamp(temp) != temp)	//if temp doesn't point at a timestamp
				startstamp=endstamp;	//Use the last parsed timestamp as the starting timestamp
			else	//temp points to a timestamp, read it
			{
				timestamp=ConvertLRCTimestamp(&temp,NULL);

				if(lineinprogress != 0)	//If the previous lyric piece did not end yet
				{
					endstamp=timestamp;	//This timestamp doubles as the end timestamp for last lyric piece
					AddLyricPiece(buffer2,startstamp,endstamp,PITCHLESS,groupswithnext);	//Write lyric with no defined pitch
					EndLyricLine();	//The previous line of lyrics is now complete
					lineinprogress=0;	//Reset this condition

					if(!Lyrics.line_on)		//Only if the Lyrics structure has no line of lyrics open (last call to EndLyricLine() closed a populated line),
						CreateLyricLine();	//Initialize for next line of lyrics
				}
				startstamp=timestamp;	//This is the beginning timestamp for the next lyric piece
			}

//Find next timestamp if one exists on this line
			temp2=SeekNextLRCTimestamp(temp);

			if(temp2 == NULL)	//If there was no ending timestamp on the remainder of the line
			{
				if(temp[0] != '\0')
				{	//If there is text at the end of the lyric line
					strcpy(buffer2,temp);	//Copy rest of line into buffer2
					lyricinprogress=1;	//Next timestamp will end this lyric
				}
				break;			//skip processing remainder of line
			}

//Copy all characters between the two timestamps into buffer2, which is large enough to hold anything that buffer contains
			ctr=0;	//Will be used to index into buffer2
			while(temp < temp2)
			{
				if((temp[0] == '\0') || (temp[0] == '\r') || (temp[0] == '\n'))
				{
					(void) puts("Error: Unexpected end of buffer encountered\nAborting");
					exit_wrapper(1);
				}

				buffer2[ctr++]=*temp;	//Copy character into buffer2, increment buffer2's index
				temp++;			//Advance to next character in source string
			}
			buffer2[ctr]='\0';	//Terminate string

//Handle whitespace at the beginning of any parsed lyric piece as a signal that the piece will not group with previous piece
//No lyric between timestamps, ie "lyric[mm:ss:hh][mm:ss:hh]lyric" will be interpreted so that the two lyrics do not group
			if(isspace(buffer2[0]) || (buffer2[0] == '\0'))
			{
				assert_wrapper(Lyrics.curline != NULL);
				if(Lyrics.curline->pieces != NULL)	//If there was a previous lyric piece on this line
				{
					Lyrics.lastpiece->groupswithnext=0;	//Ensure it is set to not group with this lyric piece
				}
			}

//Convert ending timestamp to integer and seek to first character past the timestamp
//At this point, temp should point to the ending timestamp as per the previous while loop
			endstamp=ConvertLRCTimestamp(&temp,NULL);
			lyricinprogress=0;	//The ending timestamp was parsed

			if(buffer2[0] == '\0')	//If there was no lyric before the next starting timestamp character was reached
				continue;	//Skip adding a lyric

			if(!isspace(buffer2[strlen(buffer2)-1]))	//If this piece doesn't end in a space
				if(SeekNextLRCTimestamp(temp) != NULL)	//And there's another timestamp on this line
					groupswithnext=1;	//Flag this piece to allow for hyphen appending (based on nohyphens variable)

//Add lyric piece
			AddLyricPiece(buffer2,startstamp,endstamp,PITCHLESS,groupswithnext);	//Write lyric with no defined pitch
		}//end while(1)

		if(lyricinprogress == 0)	//if the line of lyrics ended with a timestamp
			EndLyricLine();
		else
			lineinprogress=1;		//A timestamp is expected to end this line of lyrics
	}//end while(fgets(buffer,maxlinelength,inf) != NULL)

	if(lineinprogress != 0)
	{	//If a line of lyrics doesn't end in a timestamp, and there are no other lines,
		//calculate the mean duration of all lyrics and use that as the duration for this last piece
		mean=0;
		if(Lyrics.verbose)	(void) puts("Warning: There is no ending timestamp for the last lyric.  One will be created.");

		for(templine=Lyrics.lines;templine!=NULL;templine=templine->next)	//For each line of lyrics
		{
			for(temp3=templine->pieces;temp3!=NULL;temp3=temp3->next)		//For each lyric in the line
				mean+=((double)temp3->duration)/Lyrics.piececount;	//Divide by the number of Lyric pieces altogether before adding to sum to prevent integer overflows
		}
		mean+=0.5;	//Add 0.5 so it will round to nearest integer when added to startstamp below
		AddLyricPiece(buffer2,startstamp,startstamp+mean,PITCHLESS,groupswithnext);	//Write lyric with no defined pitch
		if(Lyrics.verbose)	(void) putchar('\n');
	}

	ForceEndLyricLine();

//Release memory buffers and return
	free(buffer);
	free(buffer2);
	if(Lyrics.verbose)	printf("LRC import complete.  %lu lyrics loaded\n\n",Lyrics.piececount);
}

char *SeekNextLRCTimestamp(char *ptr)	//find next occurence of a timestamp in [xx:yy:zz] format, return pointer to beginning of timestamp, or NULL if no timestamp is found
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
		while(temp[0] != '\0')	//Check all characters until end of string
			if(strchr(timestampbeg,temp[0]))	//If this character is a beginning character for a timestamp
				break;
			else
				temp++;	//Seek to next character

		if(temp[0] == '\0')	//If end of string was reached without reaching a potential timestamp
			return NULL;	//return on failure

//validate minutes portion of timestamp
		ctr2=0;
		while(!retry)
		{
			if((temp[ctr] == '\0') || (!isdigit(temp[ctr]) && (!strchr(timestampdel,temp[ctr]))))	//Numerical char(s) followed by delimiter are expected
				retry=1;	//Signal to stop processing at ptr and find next [ character
			else			//Look for delimiter and increment index to look at next character
			{
				if(strchr(timestampdel,temp[ctr++]))	//If this character is a valid timestamp field delimiter
					break;		//If it was colon, break from loop
				else			//If it was a number
				{
					ctr2++;		//Increment the field length counter
					if(ctr2 > LRCTIMESTAMPMAXFIELDLENGTH)	//If this is longer than the defined limit of chars
						retry=1;	//Signal to stop processing at ptr and find next [ character
				}
			}
		}

//validate seconds portion of timestamp
		ctr2=0;
		while(!retry)
		{
			if((temp[ctr] == '\0') || (!isdigit(temp[ctr]) && (!strchr(timestampdel,temp[ctr]))))	//Numerical char(s) followed by delimiter are expected
				retry=1;
			else		//Look for delimiter and increment index to look at next character
			{
				if(strchr(timestampdel,temp[ctr++]))	//If this character is a valid timestamp field delimiter
					break;		//If it was colon, break from loop
				else			//If it was a number
				{
					ctr2++;		//Increment the field length counter
					if(ctr2 > LRCTIMESTAMPMAXFIELDLENGTH)	//If this is longer than the defined limit of chars
						retry=1;	//Signal to stop processing at ptr and find next [ character
				}
			}
		}

//validate hundredths portion of timestamp
		ctr2=0;
		while(!retry)
		{
			if((temp[ctr] == '\0') || (!isdigit(temp[ctr])))	//If not a numerical character
			{
				if(strchr(timestampend,temp[ctr]))//If this character is an end character for a timestamp
					break;	//break from loop
				else
					retry=1;
			}
			else
			{
				ctr++;	//seek to next character
				ctr2++;	//Increment the field length counter
				if(ctr2 > LRCTIMESTAMPMAXFIELDLENGTH)	//If this is longer than the defined limit of chars
					retry=1;	//Signal to stop processing at ptr and find next [ character
			}
		}

		if(retry)		//If parsing failed
			temp++;		//Seek past this timestamp starting character to look for the next
		else			//If a suitable timestamp was found
			return temp;	//Return pointer
	}
}

unsigned long ConvertLRCTimestamp(char **ptr,int *errorstatus)
{	//Accepts pointer to a timestamp in [xx:yy:zz] format, returns conversion in milliseconds.  ptr is advanced to first character after end of timestamp
	char *temp=NULL;
	unsigned int ctr=1;
	char failed=0;	//Boolean: Parsing indicated that ptr didn't point at a valid timestamp, abort
	char minutes[LRCTIMESTAMPMAXFIELDLENGTH+1]={0};		//Allow for pre-defined # of minute chars (and NULL terminator)
	char seconds[LRCTIMESTAMPMAXFIELDLENGTH+1]={0};		//Allow for pre-defined # of seconds chars (and NULL terminator)
	char hundredths[LRCTIMESTAMPMAXFIELDLENGTH+1]={0};	//Allow for pre-defined # of hundredths chars (and NULL terminator)
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
	if((temp != NULL) && !strchr(timestampbeg,temp[0]))	//If this character is not a beginning character for a timestamp
		failed=1;

//Parse minutes portion of timestamp
	index=0;
	while(!failed)
	{
		assert_wrapper(temp != NULL);
		if((temp[ctr] == '\0') || (!isdigit(temp[ctr]) && (!strchr(timestampdel,temp[ctr]))))	//Numerical char(s) followed by delimiter are expected
			failed=1;
		else
		{
			if(isdigit(temp[ctr]))	//Is a numerical character
			{
				if(index > LRCTIMESTAMPMAXFIELDLENGTH)	//If more than the defined limit of chars have been parsed
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
	assert_wrapper(index <= LRCTIMESTAMPMAXFIELDLENGTH+1);	//Ensure that writing the NULL character won't overflow
	minutes[index]='\0';	//Terminate minutes string

//validate seconds portion of timestamp
	index=0;
	while(!failed)
	{
		assert_wrapper(temp != NULL);
		if((temp[ctr] == '\0') || (!isdigit(temp[ctr]) && (!strchr(timestampdel,temp[ctr]))))	//Numerical char(s) followed by delimiter are expected
			failed=1;
		else
		{
			if(isdigit(temp[ctr]))	//Is a numerical character
			{
				if(index > LRCTIMESTAMPMAXFIELDLENGTH)	//If more than the defined limit of chars have been parsed
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

	assert_wrapper(index <= LRCTIMESTAMPMAXFIELDLENGTH+1);	//Ensure that writing the NULL character won't overflow
	seconds[index]='\0';	//Terminate seconds string

//validate hundredths portion of timestamp
	index=0;
	while(!failed)
	{
		assert_wrapper(temp != NULL);
		if(temp[ctr] == '\0')	//end of string was reached before end of timestamp
			failed=1;
		else if(!isdigit(temp[ctr]))	//If not a numerical character
		{
			if(strchr(timestampend,temp[ctr]))//If this character is an end character for a timestamp
				break;	//break from loop
			else
				failed=1;
		}
		else
		{
			if(index > LRCTIMESTAMPMAXFIELDLENGTH)	//If more than the defined limit of chars have been parsed
				failed=2;
			else
				hundredths[index++]=temp[ctr++];	//copy character into milliseconds string, increment indexes
		}
	}

	assert_wrapper(index <= LRCTIMESTAMPMAXFIELDLENGTH+1);	//Ensure that writing the NULL character won't overflow
	hundredths[index]='\0';	//Terminate hundredths string

	if(failed)		//If parsing failed
	{
		printf("Error: Invalid timestamp \"%s\"\nAborting\n",temp);
		if(errorstatus != NULL)
		{
			*errorstatus=1;
			return 0;
		}
		else
			exit_wrapper(1);
	}

	assert_wrapper((ptr != NULL) && (temp != NULL));
	*ptr=&(temp[ctr+1]);	//Store address of first character following end of timestamp

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

//Convert hundredths string to integer and add to sum
	temp=RemoveLeadingZeroes(hundredths);
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
		sum+=conversion*10;		//one hundredths of one second is 10 milliseconds
	}
	free(temp);
	return sum;
}

void WriteLRCTimestamp(FILE *outf,char openchar,char closechar,unsigned long time)
{	//Accepts the time given in milliseconds and writes a timestamp to specified FILE stream, using the specified characters at
	//the beginning and end of the timestamp: ie. <##:##.##> or [##:##.##]
	unsigned long minutes, seconds, millis;	//Values to store the converted timestamp

	assert_wrapper(outf != NULL);			//This must not be NULL
	assert_wrapper(!isdigit(openchar));		//This must not be a number
	assert_wrapper(!isdigit(closechar));	//This must not be a number

//Convert the given time from milliseconds to minutes, seconds and remaining milliseconds
	millis=time;
	minutes=millis/60000;
	millis-=minutes*60000;
	seconds=millis/1000;
	millis-=seconds*1000;

//Round to hundredth of a second
	if(millis%10 > 5)
		millis=millis/10 + 1;
	else
		millis=millis/10;

//Write timestamp
	if(fprintf(outf,"%c%02lu:%02lu.%02lu%c",openchar,minutes,seconds,millis,closechar) < 0)
	{
		printf("Error writing timestamp: %s\nAborting\n",strerror(errno));
		exit_wrapper(1);
	}
}

void Export_LRC(FILE *outf)
{
	struct Lyric_Line *curline=NULL;	//Conductor of the lyric line linked list
	struct Lyric_Piece *temp=NULL;		//A conductor for the lyric pieces list
	int errornumber=0;

	assert_wrapper(outf != NULL);			//This must not be NULL
	assert_wrapper(Lyrics.piececount != 0);	//This function is not to be called with an empty Lyrics structure

	if(Lyrics.verbose)	printf("\nExporting LRC lyrics to file \"%s\"\n\nWriting tags\n",Lyrics.outfilename);

//Write tags
	if(Lyrics.Title != NULL)
		if(fprintf(outf,"[ti:%s]\n",Lyrics.Title) < 0)
			errornumber=errno;

	if(Lyrics.Artist != NULL)
		if(fprintf(outf,"[ar:%s]\n",Lyrics.Artist) < 0)
			errornumber=errno;

	if(Lyrics.Album != NULL)
		if(fprintf(outf,"[al:%s]\n",Lyrics.Album) < 0)
			errornumber=errno;

	if(Lyrics.Editor != NULL)
		if(fprintf(outf,"[by:%s]\n",Lyrics.Editor) < 0)
			errornumber=errno;

	if(Lyrics.Offset != NULL)
		if(fprintf(outf,"[offset:%s]\n",Lyrics.Offset) < 0)
			errornumber=errno;

	if(errornumber != 0)
	{
		printf("Error exporting tags: %s\nAborting\n",strerror(errornumber));
		exit_wrapper(1);
	}

//Write lyrics
	if(Lyrics.verbose)	(void) puts("Writing lyrics");

	curline=Lyrics.lines;	//Point lyric line conductor to first line of lyrics

	while(curline != NULL)	//For each line of lyrics
	{
		if(Lyrics.verbose)	printf("\tLyric line: ");

		temp=curline->pieces;	//Starting with the first piece of lyric in this line

	//Write the line's initial timestamp
		if(temp != NULL)	//Check for NULL again to satisfy cppcheck
			WriteLRCTimestamp(outf,'[',']',temp->start);	//Write timestamp of first lyric (using brackets)

		while(temp != NULL)	//For each piece of lyric in this line
		{
			assert_wrapper(temp->lyric != NULL);

		//Write lyric
			fputs_err(temp->lyric,outf);

			if(Lyrics.verbose)	printf("'%s'",temp->lyric);

		//Insert a space for non-grouped lyrics
			if((temp->next != NULL) && (!temp->groupswithnext))	//If there is another lyric in this line and it does not group with this one
				fputc_err(' ',outf);	//Append a space before the timestamp is written

			if(temp->next == NULL)
			{	//This was the last lyric in the line, write a timestamp to end this line, and a newline char
				if(Lyrics.out_format == ELRC_FORMAT)	//Only if outputting in extended LRC format, write next/last timstamp in this line
					WriteLRCTimestamp(outf,'<','>',temp->start + temp->duration);

				fputc_err('\n',outf);
			}
			else if(Lyrics.out_format == ELRC_FORMAT)
				//There is another lyric in this line and outputting in extended LRC format, write its timestamp
					WriteLRCTimestamp(outf,'<','>',temp->next->start);

			temp=temp->next;	//Advance to next lyric piece as normal
		}//while(temp != NULL)

		curline=curline->next;	//Advance to next line of lyrics

		if(Lyrics.verbose)	(void) putchar('\n');
	}//end while(curline != NULL)

	if(Lyrics.verbose)	printf("\nLRC export complete.  %lu lyrics written\n",Lyrics.piececount);
}
