#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <assert.h>
#include "Lyric_storage.h"
#include "VL_parse.h"
#include "Midi_parse.h"
#include "LRC_parse.h"
#include "ID3_parse.h"
#include "SRT_parse.h"
#include "XML_parse.h"

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

//
//GLOBAL VARIABLE DEFINITIONS
//
struct _LYRICSSTRUCT_ Lyrics;
jmp_buf jumpbuffer;			//Used in the conditional compiling code to allow this program's logic to return program control to EOF
							//in the event of an exception that would normally terminate the program
jmp_buf FLjumpbuffer;		//This is used by FLC's internal logic to provide for exception handling (ie. in validating MIDI files with DetectLyricFormat())
char useFLjumpbuffer=0;		//Boolean:  If nonzero, FLC's logic intercepts in exit_wrapper() regardless of whether EOF_BUILD is defined
const char *LYRICFORMATNAMES[NUMBEROFLYRICFORMATS+1]={"UNKNOWN LYRIC TYPE","SCRIPT","VL","RB MIDI","UltraStar","LRC","Vocal Rhythm","ELRC","KAR","Pitched Lyrics","Soft Karaoke","ID3 Lyrics","SRT Subtitle","XML","JamBand","Rocksmith XML","Rocksmith 2 XML"};



//Integration code for merging with EOF
//The use of exit and assert should be avoided when these files are compiled into EOF,
//so these wrapper functions will be called instead.  If EOF_BUILD is defined during compile,
//the wrappers use longjmp() to return program control to EOF.  Otherwise, the normal exit
//and assert function calls are compiled into the program.  assert_wrapper, in the case that
//EOF_BUILD is not defined during compile, is defined as a macro in Lyric_storage.h

#ifdef EOF_BUILD
	void exit_wrapper(int status)
	{
		if(useFLjumpbuffer)					//If the longjmp() internal exception handling logic should be used
			longjmp(FLjumpbuffer,status);	//Return program control to appropriate foflc logic
		else
			longjmp(jumpbuffer,status);	//Return program control to the instruction line where setjmp() is called in EOF's main code
	}

	void assert_wrapper(int expression)
	{
		if(expression == 0)			//If the expression failed
			longjmp(jumpbuffer,1);	//Return program control to the instruction line where setjmp() is called in EOF's main code
	}
#else
	void exit_wrapper(int status)
	{
		if(useFLjumpbuffer)					//If the longjmp() internal exception handling logic should be used
			longjmp(FLjumpbuffer,status);	//Return program control to appropriate foflc logic
		else
			exit(status);					//Exit program, this only runs if EOF_BUILD is not defined during compile
	}
#endif


void InitLyrics(void)
{
	Lyrics.lines=NULL;	//Empty list
	Lyrics.curline=NULL;
	Lyrics.linecount=0;
	Lyrics.line_on=0;
	Lyrics.lyric_defined=0;
	Lyrics.lyric_on=0;
	Lyrics.Title=NULL;
	Lyrics.Artist=NULL;
	Lyrics.Album=NULL;
	Lyrics.Editor=NULL;
	Lyrics.Offset=NULL;
	Lyrics.Year=NULL;
	Lyrics.offsetoverride=0;
	Lyrics.realoffset=0;
	Lyrics.nohyphens=0;
	Lyrics.grouping=0;
	Lyrics.noplus=0;
	Lyrics.piececount=0;
	Lyrics.filter=NULL;
	Lyrics.defaultfilter=0;
	Lyrics.verbose=0;
	Lyrics.quick=0;
	Lyrics.in_format=0;
	Lyrics.pitch_tracking=0;
	Lyrics.explicittempo=0.0;
	Lyrics.startstamp=0;
	Lyrics.startstampspecified=0;
	Lyrics.overdrive_on=0;
	Lyrics.freestyle_on=0;
	Lyrics.brute=0;
	Lyrics.marklines=0;
	Lyrics.out_format=0;
	Lyrics.inputtrack=NULL;
	Lyrics.outputtrack=NULL;
	Lyrics.outfilename=NULL;
	Lyrics.infilename=NULL;
	Lyrics.srclyrname=NULL;
	Lyrics.srcrhythmname=NULL;
	Lyrics.dstlyrname=NULL;
	Lyrics.srcfilename=NULL;
	Lyrics.TitleStringID=NULL;
	Lyrics.ArtistStringID=NULL;
	Lyrics.AlbumStringID=NULL;
	Lyrics.EditorStringID=NULL;
	Lyrics.OffsetStringID=NULL;
	Lyrics.YearStringID=NULL;
	Lyrics.srcoffsetoverride=0;
	Lyrics.srcrealoffset=0;
	Lyrics.nolyrics=0;
	Lyrics.notenames=0;
	Lyrics.relative=0;
	Lyrics.nopitch=0;
	Lyrics.prevlineslast=NULL;
	Lyrics.reinit=1;	//Handler functions need to re-init static variables
	Lyrics.last_pitch=0;
	Lyrics.rocksmithver=1;
	Lyrics.nosrctag=NULL;
	Lyrics.nofstyle=0;
}

void CreateLyricLine(void)
{
	struct Lyric_Line *temp=NULL;				//Stores newly-allocated lyric line structure
	static struct Lyric_Line emptyLyric_Line;	//Auto-initialize all members to 0/NULL

	if(Lyrics.verbose>=2)	(void) puts("Initializing storage for new line of lyrics");

	if(Lyrics.line_on != 0)
	{
		(void) puts("Line of lyrics already open");
		return;
	}

//Allocate and initialize new Lyric Line structure for linked list
	temp=(struct Lyric_Line *)malloc_err(sizeof(struct Lyric_Line));
	*temp=emptyLyric_Line;	//Reliably initialize all values to 0/NULL

//Append new link to the linked list
	if(Lyrics.lines == NULL)	//Special case:  List is empty
	{
		Lyrics.lines=temp;		//Initialize linked list
		temp->prev=NULL;		//First link points back to nothing
	}
	else	//Attach new link to the list
	{
		temp->prev=Lyrics.curline;	//New link points back to previous link
		Lyrics.curline->next=temp;	//Attach new link
	}

	Lyrics.curline=temp;	//Point line conductor to new line

//	On rare occasion, some charts (Such as Nirvana- "In Bloom") will start a lyric piece before a line starts.
//	To handle this, the lyric stats must not be reset at the beginning of a line, but once the lyric is added.
	Lyrics.line_on=1;	//The line has been initiated
}

void EndLyricLine(void)
{
	struct Lyric_Piece *temp=NULL;	//A conductor to use for lyric overlap checking
	struct Lyric_Piece *temp2=NULL;	//A temporary pointer to use for lyric combination
	struct Lyric_Piece *next=NULL;	//A conditional next piece pointer needs to be used

	if(Lyrics.line_on == 0)	//If there is no open line of lyrics
		return;

	if(Lyrics.verbose>=2)	printf("Finalizing storage of line of lyrics #%lu\n",Lyrics.linecount+1);

//If the current line of lyrics is empty, return without closing the line
	if(Lyrics.curline->piececount == 0)
	{
		if(Lyrics.verbose)	(void) puts("(Empty line of lyrics ignored)\n");
		return;
	}

	Lyrics.linecount++;	//Increment the number of parsed lines of lyrics
	Lyrics.line_on=0;	//line_on=FALSE : No line of lyrics is currently open

//Perform error handling for oddities (ie. overlapping pieces)
	if(Lyrics.prevlineslast != NULL)	//If there was a previous line of lyrics
		temp=Lyrics.prevlineslast;		//begin checking with the last piece in that line
	else
		temp=Lyrics.curline->pieces;	//Point the conductor to first piece of lyric in the line

	while(temp != NULL)		//For each piece in the line
	{
		if(temp == Lyrics.prevlineslast)	//If we're looking at the last piece in the previous line
			next=Lyrics.curline->pieces;	//Point forward to the first piece in this line
		else
			next=temp->next;	//Point forward to next piece in this line

		if(next != NULL)	//If there's another piece that follows,
		{
			assert_wrapper(temp->lyric != NULL);
			assert_wrapper(next->lyric != NULL);

			if(temp->start > next->start)	//Lyrics are expected to be in ascending chronological order
			{	//If the next piece has a timestamp that precedes this piece's timestamp
				printf("Error: Lyric \"%s\" has a timestamp (%lu) that is earlier than the preceding lyric\nAborting\n",next->lyric,next->start);
				exit_wrapper(1);
			}

//Special handling for noplus
			if((temp != Lyrics.prevlineslast) && Lyrics.noplus && (!strcmp(next->lyric,"+") || !strcmp(next->lyric,"+-")))
			{	//If next is in the same line as temp, is a plus sign and the user specified noplus,
				//Remove the plus lyric, adjust the duration and grouping of the previous lyric
				if(Lyrics.verbose >= 2)	printf("\tNoplus- Merging plus lyric with \"%s\"\n",temp->lyric);
				temp->duration += (next->start - (temp->start + temp->duration)) + next->duration;	//Add the +'s duration and distance from this lyric to this lyric
				temp->groupswithnext=next->groupswithnext;	//Replace this lyric's grouping status with that of the plus lyric

				temp->next=next->next;					//Remove the + lyric from the list
				if(temp->next != NULL)					//If next was not the last lyric in the line
					temp->next->prev=temp;				//have the lyric that followed point back to this lyric
				free(next->lyric);						//Free the + lyric's structure
				free(next);
				Lyrics.piececount--;					//decrement the lyric piece counter
				Lyrics.curline->piececount--;			//decrement the line lyric counter
				continue;	//Don't continue processing, start checking now that the piece has combined
			}

//Special handling for overlapping
			if(temp->start + temp->duration > next->start)
			{	//If this piece extends past the timestamp of the next piece
				if((temp->start + temp->duration > next->start + 1) || (next->start - temp->start == 0))
					//Suppress warning if the overlapping isn't larger than 1ms (which is often caused by the rounding code), and if the lyric won't be shortened to 0ms
					printf("Warning: Lyric \"%s\" at %lums will overlap with \"%s\" at %lums.  Shortening to %lums\n",temp->lyric,temp->start,next->lyric,next->start,next->start-temp->start);

				if(next == Lyrics.curline->pieces)
				//If the previous line's duration is going to be shortened, update the line's duration
					Lyrics.curline->prev->duration-=temp->duration - (next->start - temp->start);

				temp->duration = next->start - temp->start;	//Set new duration to end when next lyric begins
			}

//Special handling for 0 length duration
			if(temp->duration == 0)
			{
				if(temp->start == next->start)	//If this lyric starts at same time as next piece
				{
					printf("Warning: Lyric \"%s\" at %lums has a duration of 0, combining with next lyric\n",temp->lyric,temp->start);
				//Combine this piece with the next (the next piece's pitch and duration will be kept)
					if(!isspace((temp->lyric)[strlen(temp->lyric)-1]) && !isspace((next->lyric)[strlen(next->lyric)-1]) && (temp->groupswithnext == 0))
						//If there is no whitespace between the two lyric pieces, but only if this piece doesn't group with the next
						temp->lyric=ResizedAppend(temp->lyric," ",1);		//Reallocate the string to end in a space

					temp->lyric=ResizedAppend(temp->lyric,next->lyric,1);	//Recreate temp->lyric to have next lyric piece appended
					temp->duration=next->duration;
					temp->pitch=next->pitch;
					temp->overdrive=next->overdrive;
					temp->freestyle=next->freestyle;
					temp->groupswithnext=next->groupswithnext;

					assert_wrapper(next != NULL);
					temp2=next->next;			//Remember address of the next lyric piece after the next piece
				//Delete next lyric piece
					free(next->lyric);			//release this string
					free(next);					//release this structure
					Lyrics.piececount--;		//decrement the lyric piece counter
					Lyrics.curline->piececount--;//decrement the line lyric counter
				//Reconnect linked list
					if(temp == Lyrics.prevlineslast)
					{	//If the link that was freed was the head of this line
						Lyrics.curline->pieces=temp2;	//Correct the head of the linked list
						if(temp2 != NULL)		//If there are lyrics left in this line
							temp2->prev=NULL;	//New head of list points back to nothing
						else
						{	//Remove this empty line and break from loop
							assert_wrapper(Lyrics.curline->prev != NULL);	//Cannot group the first line's lyrics to a previous line
							Lyrics.curline=Lyrics.curline->prev;	//Point back to previous line
							free(Lyrics.curline->next);				//Free empty line
							Lyrics.curline->next=NULL;				//What was the previous line points forward to nothing
							Lyrics.linecount--;

							break;
						}
					}
					else
					{
						temp->next=temp2;	//Point current link to next link
						if(temp->next != NULL)			//If there was a next link
							(temp->next)->prev=temp;	//Point next link back to current link
					}
				}
				else	//increase the duration of this lyric to 1 because it won't overlap with next piece
				{
					printf("Warning: Lyric \"%s\" at %lums has a duration of 0, increasing duration to 1\n",temp->lyric,temp->start);
					temp->duration=1;
				}
				continue;	//Don't skip to next piece.  Restart overlap checking with focus on this piece
			}
		}

		if(next == NULL)	//If this is the last piece in this line
			Lyrics.prevlineslast=temp;		//Remember this piece as the last piece

		temp=next;	//Point the conductor to the next lyric piece (if the last piece in the previous line was being
					//checked, next was already properly set to point to the first piece in this line
	}//For each piece in the line

	RecountLineVars(Lyrics.curline);
	if(Lyrics.verbose>=2)	(void) putchar('\n');
}

void AddLyricPiece(char *str,unsigned long start,unsigned long end,unsigned char pitch,char groupswithnext)
{
	struct Lyric_Piece *temp=NULL;	//Temporary pointer
	char overdrive=0,freestyle=0;	//Track the overdrive and freestyle statuses that are in effect
	char hasequal=0;				//Used to track whether this lyric piece had an equal sign (hyphen) at the end
	char linebreak=0;				//Enforce a linebreak if the lyric ends in a '/' (ie. Rock Band Beatles lyrics)
	char leadspace=0,trailspace=0;	//Used for leading/trailing whitespace detection

	if((pitch != PITCHLESS) && ((Lyrics.in_format==MIDI_FORMAT) || (Lyrics.in_format==KAR_FORMAT)))
	{
		if(pitch > 127)	//Pitches higher than 127 are not allowed, however do allow the pitch to be set to the defined PITCHLESS value
		{
			(void) puts("MIDI Violation: Valid pitch range is from 0 to 127\nAborting");
			exit_wrapper(1);
		}
	}

	if(str == NULL)		//If the string isn't even initialized
		return;			//return without adding it

	if(isspace(str[0]))				//If the string begins with whitespace
		leadspace=1;

	if(isspace(str[strlen(str)-1]))	//If the string ends with whitespace
		trailspace=1;

	str=TruncateString(str,0);	//Remove all leading and trailing whitespace, do NOT deallocate original string

	if(str[0] == '\0')			//If the string is empty
	{
		if(Lyrics.verbose>=2)	(void) puts("Empty lyric ignored");
		return;
	}

//EndLyricLine() now handles pieces that have a duration of 0, but negative durations still need to cause an abort
	if(start-Lyrics.realoffset > end-Lyrics.realoffset)
	{
		(void) puts("Error: Cannot add piece with a negative duration\nAborting");
		exit_wrapper(2);
	}

	if(Lyrics.verbose>=2)
	{
		if(pitch == PITCHLESS)
			printf("\tProcessing lyric piece '%s':\tdur=%lu\tNO PITCH\t",str,end-start);
		else
			printf("\tProcessing lyric piece '%s':\tdur=%lu\tpit=%u\t",str,end-start,pitch);
	}

//Parse the string looking for the freestyle vocal modifiers # and ^
	if((strchr(str,'#') != NULL) || (strchr(str,'^') != NULL))	//If the string contains '#' or '^'
		freestyle=1;

	if(Lyrics.freestyle_on)	//If Freestyle is manually enabled
		freestyle=1;

	if(Lyrics.overdrive_on)
		overdrive=1;

	if(str[strlen(str)-1] == '/')	//If this lyric ends in a forward slash
		linebreak=1;				//Insert a linebreak after adding the lyric

	if((str[0] != '\0') && (str[strlen(str)-1] == '='))	//If the last character in the string is an equal sign
	{
		str[strlen(str)-1]='\0';	//Truncate it
		hasequal=1;					//This lyric should be written to end in a hyphen and group
		groupswithnext=1;
	}

//FILTER HANDLING CODE
	if(Lyrics.filter != NULL)
	{
		while(str[0] != '\0')				//While the string is not empty
		{
			if(strchr(Lyrics.filter,str[strlen(str)-1]) != NULL)
			{	//If the last character in the lyric piece is in the filter list, truncate it from the string
				if(Lyrics.verbose)	printf("Filtered character '%c' removed\n",str[strlen(str)-1]);
				str[strlen(str)-1]='\0';	//Truncate the last character
			}
			else	//There is not a filtered character at the end of this piece of lyric, or no filtering was specified
				break;
		}
	}
	if(str[0] == '\0')	//If truncating filtered characters emptied the string
	{
		Lyrics.lyric_defined=0;					//Expecting a new lyric event
		Lyrics.lyric_on=0;						//Expecting a new Note On event as well
		free(str);								//de-allocate newly-created string
		if(Lyrics.verbose>=2)	(void) puts("(empty lyric dropped)");
		return;			//return without adding the empty string
	}

//Allocate memory for a new link
	temp=(struct Lyric_Piece *)malloc_err(sizeof(struct Lyric_Piece));

//Append new link to the linked list
	temp->next=NULL;			//New link will point forward to nothing

	assert_wrapper(Lyrics.curline != NULL);
	if(Lyrics.curline->pieces == NULL)	//Special case:  List is empty
	{
		Lyrics.curline->pieces=temp;	//Initialize linked list
		Lyrics.curline->curpiece=temp;	//Point conductor to first link
		temp->prev=NULL;				//First link points backward to nothing
		Lyrics.curline->start=start-Lyrics.realoffset;
			//Line starting offset is starting offset of this first piece - specified offset
	}
	else	//Attach new link to the list
	{
		temp->prev=Lyrics.curline->curpiece;	//New link points back to previousy-last link
		Lyrics.curline->curpiece->next=temp;	//Attach new link
		Lyrics.curline->curpiece=temp;			//Point conductor to new link
	}

	if((strlen(str) > 1) && (str[strlen(str)-1] == '-'))
		//If the last character in this lyric piece is a hyphen and the piece is more than just "-"
		groupswithnext=1;	//Next lyric piece will need to be grouped to this one

//If this lyric piece is just a + (or a plus with a grouping hyphen), ensure it is marked in the Lyric structure to append to the previous lyric
//Check Lyrics.curline->piececount to verify that this is only done if the + is not the first lyric piece in the line
	if(!strcmp(str,"+") && (Lyrics.curline->piececount != 0))
	{
		assert_wrapper(temp->prev != NULL);
		groupswithnext=temp->prev->groupswithnext;			//The pitch shift will inherit the grouping status of the lyric it groups with
		temp->prev->groupswithnext=1;						//The previous lyric is forced to group to the pitch shift

		#ifndef EOF_BUILD
		//Force the duration of the previous piece to join up with this lyric piece, as that + implies that the previous lyric was still being sung
		//Only perform this manipulation if NOT being used in EOF, as it is intended to give EOF the original timings
			temp->prev->duration=start-Lyrics.realoffset-temp->prev->start;	//Remember to offset start by realoffset, otherwise Lyrics.lastpiece->start could be the larger operand, causing an overflow
		#endif
	}

//Remember this as the last lyric piece in order for the noplus, hyphen insertion, etc. features to work
	Lyrics.lastpiece=temp;

//Handle -startstamp parameter
	if(Lyrics.startstampspecified==1)	//No offset has been calculated yet
	{
		Lyrics.realoffset=start-Lyrics.startstamp;	//Find the offset to subtract from all timestamps
		Lyrics.startstampspecified=2;
	}

//Validate the offset by ensuring that it is not larger than the timestamp of the lyric piece, as timestamps must be >0
	if((Lyrics.realoffset > 0) && ((unsigned long)Lyrics.realoffset > start))
	{
		(void) puts("Error: Offset cannot be larger than the timestamp of any lyric event\nAborting");
		exit_wrapper(3);
	}

//Error check to ensure a line is in progress
	if(Lyrics.line_on == 0)
	{
		(void) puts("Error: Line has not been initiated, unable to add lyric\nAborting");
		exit_wrapper(4);
	}

	assert_wrapper(Lyrics.curline != NULL);

//Initialize new link
	temp->start=start-Lyrics.realoffset;
		//The start offset of the piece of lyric in real time (milliseconds) - specified offset
	temp->duration=end-start;				//Duration of the piece of lyric in milliseconds
	temp->lyric=str;						//The string associated with this piece of lyric
	temp->pitch=pitch;						//Assign the specified pitch
	temp->freestyle=freestyle;				//Assign the freestyle status found
	temp->overdrive=overdrive;				//Assign the overdrive status found
	temp->groupswithnext=groupswithnext;	//Assign the grouping status based on the determined status
	temp->hasequal=hasequal;				//Assign the hyphen status
	temp->leadspace=leadspace;				//Assign the leadspace status
	temp->trailspace=trailspace;			//Assign the trailspace status

	Lyrics.curline->piececount++;			//This line of lyrics is holding another lyric piece
	Lyrics.piececount++;					//The Lyric structure is holding another lyric piece
	Lyrics.lyric_defined=0;					//Expecting a new lyric event
	Lyrics.lyric_on=0;						//Expecting a new Note On event as well

	if(Lyrics.verbose>=2)
	{
		printf("start=%lu\tAdj. start=%lu",start,start-Lyrics.realoffset);
		if(groupswithnext)	printf("\tGroupswithnext=TRUE");
		if(trailspace)	printf("\n\t\tLyric \"%s\" detected as having trailing whitespace, this will defeat grouping with the next lyric if applicable",str);
		(void) putchar('\n');
	}

	if(linebreak)	//If the lyric triggers a line break in the middle of the line of lyrics
	{
		if(Lyrics.verbose)	(void) puts("(Lyric defines a line break)");
		EndLyricLine();
		CreateLyricLine();
	}
}

struct Lyric_Piece *FindLyricNumber(unsigned long number)
{
	struct Lyric_Line *curline=NULL;
	struct Lyric_Piece *curpiece=NULL;
	unsigned long ctr=0;

	if((number > Lyrics.piececount) || (number == 0))
		return NULL;	//Specified lyric does not exist

	curline=Lyrics.lines;			//Point line conductor to first line of lyrics
	assert_wrapper(curline != NULL);
	curpiece=curline->pieces;		//Point lyric conductor to first lyric

	for(ctr=1;ctr<number;ctr++)
	{
		if((curpiece == NULL) || (curline == NULL))
		{
			(void) puts("Error: Invalid lyric entry\nAborting");
			exit_wrapper(1);
		}

	//Traverse to next lyric piece
		if(curpiece->next != NULL)	//There's another piece in this line
			curpiece=curpiece->next;//Point to it
		else if(curline->next != NULL)	//There's another line of lyrics
		{
			curline=curline->next;	//Point to it
			curpiece=curline->pieces;	//Point to first lyric in the line
		}
		else	//There are no more lyric pieces
			return NULL;
	}

//At this point, curpiece is pointed at the appropriate lyric piece
	return curpiece;
}

char *ResizedAppend(char *src1,const char *src2,char dealloc)
{	//Allocates enough memory to concatenate both strings and returns the concatenated string
	char *temp;
	size_t size1,size2;

	assert_wrapper((src1 != NULL) && (src2 != NULL));
	size1=strlen(src1);
	size2=strlen(src2);
	temp=malloc_err(size1+size2+1); //Allocate enough room to store both strings and a NULL terminator
	strcpy(temp,src1);

	if(dealloc)
		free(src1);			//Release this string

	return(strncat(temp,src2,size2));	//Concatenate src1 and src2 and return the resulting string, preventing overflow
}

char *Append(const char *src1,const char *src2)
{	//Allocates enough memory to concatenate both strings and returns the concatenated string
	char *temp;
	size_t size1, size2;

	assert_wrapper((src1 != NULL) && (src2 != NULL));

	size1=strlen(src1);
	size2=strlen(src2);
	temp=malloc_err(size1+size2+1); //Allocate enough room to store both strings and a NULL terminator
	strcpy(temp,src1);

	return(strncat(temp,src2,size2));	//Concatenate src1 and src2 and return the resulting string, preventing overflow
}

void ReadWORDLE(FILE *inf,unsigned short *data)
{
	unsigned char buffer[2]={0};

	assert_wrapper((inf != NULL) && (data != NULL));	//These must not be NULL

	if(fread(buffer,2,1,inf) != 1)	//Read 2 bytes into the buffer
	{
		printf("Error reading WORD from file: %s\nAborting\n",strerror(errno));
		exit_wrapper(1);
	}

	*data=((unsigned short)buffer[0] | ((unsigned short)buffer[1]<<8));	//Convert to 2 byte integer
}

void WriteWORDLE(FILE *outf,unsigned short data)
{
	assert_wrapper(outf != NULL);

	fputc_err(data & 0xFF,outf);	//Write the least significant byte first
	fputc_err(data >> 8,outf);		//Write the most significant byte last
}

void ReadDWORDLE(FILE *inf,unsigned long *data)
{
	unsigned char buffer[4]={0};

	assert_wrapper((inf != NULL) && (data != NULL));	//These must not be NULL

	if(fread(buffer,4,1,inf) != 1)	//Read 4 bytes into the buffer
	{
		printf("Error reading DWORD from file: %s\nAborting\n",strerror(errno));
		exit_wrapper(1);
	}

	*data=((unsigned long)buffer[0]) | ((unsigned long)buffer[1]<<8) | ((unsigned long)buffer[2]<<16) | ((unsigned long)buffer[3]<<24);
		//Convert to 4 byte integer
}

void WriteDWORDLE(FILE *outf,unsigned long data)
{
	assert_wrapper(outf != NULL);

	fputc_err(data & 0xFF,outf);	//Write the least significant byte first
	data=data>>8;					//Drop the least significant byte
	fputc_err(data & 0xFF,outf);	//Write the next least significant byte
	data=data>>8;					//Drop the least significant byte
	fputc_err(data & 0xFF,outf);	//Write the next least significant byte
	data=data>>8;					//Drop the least significant byte
	fputc_err(data & 0xFF,outf);	//Write the most significant byte last
}

void ReadWORDBE(FILE *inf, unsigned short *ptr)
{	//Read 2 bytes in Big Endian format from file into *ptr, performing bit shifting accordingly
	unsigned char buffer[2]={0};

	assert_wrapper((inf != NULL) && (ptr != NULL));	//These must not be NULL

	fread_err(buffer,2,1,inf);

	*ptr=(unsigned short)((buffer[0]<<8) | buffer[1]);	//Convert to 2 byte integer
}

void WriteWORDBE(FILE *outf,unsigned short data)
{
	assert_wrapper(outf != NULL);

	fputc_err(data >> 8,outf);		//Write the most significant byte first
	fputc_err(data & 0xFF,outf);	//Write the least significant byte last
}

void ReadDWORDBE(FILE *inf, unsigned long *ptr)
{	//Read 4 bytes in Big Endian format from file into *ptr and performs bit shifting accordingly
	unsigned char buffer[4]={0};

	assert_wrapper((inf != NULL) && (ptr != NULL));	//These must not be NULL

	fread_err(buffer,4,1,inf);

	*ptr=((unsigned long)buffer[0]<<24) | ((unsigned long)buffer[1]<<16) | ((unsigned long)buffer[2]<<8) | ((unsigned long)buffer[3]);	//Convert to 4 byte integer
}

void WriteDWORDBE(FILE *outf,unsigned long data)
{
	assert_wrapper(outf != NULL);

	fputc_err((data >> 24) & 0xFF,outf);	//Write the most significant byte first
	fputc_err((data >> 16) & 0xFF,outf);	//Write the next most significant byte
	fputc_err((data >> 8) & 0xFF,outf);		//Write the next most significant byte
	fputc_err(data & 0xFF,outf);			//Write the least significant byte last
}

unsigned long ParseUnicodeString(FILE *inf)
{
	unsigned long ctr=0;			//The length of the currently-parsed string
	unsigned char input[2]={0};		//used to read in two bytes at a time from input file

	assert_wrapper(inf != NULL);	//This must not be NULL

	while(1)	//Read from input file until NULL terminator is found
	{
		if(fread(input,2,1,inf) != 1)
		{
			printf("Error parsing Unicode string: %s\nAborting\n",strerror(errno));
			exit_wrapper(1);
		}

		if(input[1] != 0)
		{
			(void) puts("Error: Unicode character detected in tag string\nAborting");
			exit_wrapper(2);
		}

		if((input[0]==0) && (input[1]==0))	//We read the unicode null terminator
			return ctr;	//return length of string, not counting NULL terminator
		else
			ctr++;	//one more character has been parsed
	}
}

char *ReadUnicodeString(FILE *inf)
{
	long position;
	size_t length;
	char *string;
	unsigned long ctr=0;
	unsigned char input[2]={0};

	assert_wrapper(inf != NULL);	//This must not be NULL

//Prepare to read string
	position=ftell_err(inf);		//Store file position before parsing string
	length=(size_t)ParseUnicodeString(inf);	//Find length of string converted to ANSI encoding
	string=malloc_err(length+1);	//Allocate storage for string, plus one byte for NULL terminator

//Read string into memory buffer
	fseek_err(inf,position,SEEK_SET);		//Return to original position in the file

	for(ctr=0;ctr<(unsigned long)length;ctr++)	//Read a WORD from file for each character we counted
	{
		if(fread(input,2,1,inf) != 1)
		{
			printf("Error reading Unicode string: %s\nAborting\n",strerror(errno));
			exit_wrapper(1);
		}

		if(input[1] != 0)
		{
			(void) puts("Error: Unicode character detected while reading string\nAborting");
			exit_wrapper(2);
		}

		string[ctr]=input[0];	//The byte to keep is the low order byte
	}

//A null Unicode byte is now expected
	if((fgetc_err(inf) != '\0') || (fgetc_err(inf) != '\0'))
	{
		(void) puts("Error: Unicode NULL character was expected but not read\nAborting");
		exit_wrapper(3);
	}

//NULL terminate the buffer and return it
	string[ctr]='\0';
	return string;
}

long int ParseLongInt(char *buffer,unsigned long *startindex,unsigned long linenum,int *errorstatus)
{
	unsigned long index;
	char buffer2[11]={0};		//An array large enough to hold a 10 digit numerical string
	unsigned long index2=0;		//index within buffer2
	long value=0;				//The converted long int value of the numerical string

	assert_wrapper((buffer != NULL) && (startindex != NULL));	//These must not be NULL
	index=*startindex;	//Dereference starting index for ease of use

//Ignore leading whitespace, seeking to first numerical character (leading 0's are discarded)
	while(buffer[index] != '\0')
	{
		if((buffer[index]=='0') && (isdigit((unsigned char)buffer[index+1])))	//If this is a leading zero
			index++;					//skip it
		else
		{
			if(isdigit((unsigned char)buffer[index]))	//If this is a numerical character (but not a leading 0)
				break;			//exit loop

			if(buffer[index] == '-')					//If this is a negative sign
				break;			//exit loop

			if(isspace((unsigned char)buffer[index]))	//If this is a white space character
				index++;		//skip it
			else
			{				//An invalid character was found
				if(errorstatus != NULL)
				{	//Suppress error and return it through the pointer
					*errorstatus=1;
					return 0;
				}
				else
				{	//Print error and exit
					printf("Error: Invalid character encountered when numerical string is expected.\nError occurs in line %lu of input file\nAborting\n",linenum);
					exit_wrapper(1);
				}
			}
		}
	}

//Parse the numerical characters into buffer2
	if(buffer[index] == '\0')
	{
		if(errorstatus != NULL)
		{	//Suppress error and return it through the pointer
			*errorstatus=2;
			return 0;
		}
		else
		{
			printf("Error: Unexpected end of line encountered.\nError occurs in line %lu of input file\nAborting\n",linenum);
			exit_wrapper(2);
		}
	}

	while(isspace((unsigned char)buffer[index]) == 0)
	{	//Until whitespace or NULL character is found
		if(isdigit((unsigned char)buffer[index]))	//If this is a numerical character
			buffer2[index2++]=buffer[index];	//Store it, move index2 forward
		else if(buffer[index] == '-')			//if this is a negative sign
			buffer2[index2++]=buffer[index];	//Store it, move index2 forward
		else
		{	//is not a numerical character
			if(buffer[index]=='\0')	//If the end of the string in buffer is reached
				break;

			if(errorstatus != NULL)
			{	//Suppress error and return it through the pointer
				*errorstatus=3;
				return 0;
			}
			else
			{
				printf("Error: Non-numerical character encountered when numerical string is expected.\nError occurs in line %lu of input file\nAborting\n",linenum);
				exit_wrapper(3);
			}
		}

		if(index2 > 10)	//We only are accounting for a numerical string 10 characters long
		{
			if(errorstatus != NULL)
			{	//Suppress error and return it through the pointer
				*errorstatus=4;
				return 0;
			}
			else
			{
				printf("Error: The encountered numerical string is too large.\nError occurs in line %lu of input file\nAborting\n",linenum);
				exit_wrapper(4);
			}
		}

		index++;	//Look at next character
	}
	buffer2[index2]='\0';		//Null terminate parsed numerical string
	(*startindex)=index;		//Pass our indexed position of buffer back into the passed variable

//Convert numerical string to numerical value
	if(buffer2[0]=='0')	//If the number is actually zero
		value=0;
	else
	{
		assert_wrapper(buffer2 != NULL);	//atol() may crash the program if it is passed NULL

		value=atol(buffer2);	//Convert string to int
		if(value == 0)
		{
			if(errorstatus != NULL)
			{	//Suppress error and return it through the pointer
				*errorstatus=5;
				return 0;
			}
			else
			{
				printf("Error converting numerical string to numerical value.\nError occurs in line %lu of input file\nAborting\n",linenum);
				exit_wrapper(5);
			}
		}
	}

	return value;
}

char *DuplicateString(const char *str)
{
	char *ptr=NULL;
	size_t size=0;

	if(str == NULL)
		return NULL;

	size=strlen(str);
	ptr=malloc_err(size+1);

	strcpy(ptr,str);
	return ptr;
}

char *TruncateString(char *str,char dealloc)
{
	unsigned long ctr=0;
	char *newstr=NULL;
	char *originalstr=str;

	assert_wrapper(str != NULL);	//This must not be NULL

//Truncate leading whitespace
	while(str[0] != '\0')					//Until NULL terminator is reached
		if(isspace((unsigned char)str[0]) == 0)	//If this is a non whitespace character
			break;					//break from loop
		else
			str++;					//Drop whitespace from start of string

//Parse the string backwards to truncate trailing whitespace
//Note, isspace() seems to detect certain characters (ie. accented chars) incorrectly
//unless the input character is casted to unsigned char?
	for(ctr=(unsigned long)strlen(str);ctr>0;ctr--)
		if(isspace((unsigned char)str[ctr-1]))	//If this character is whitespace
			str[ctr-1]='\0';		//truncate string to drop the whitespace
		else		//If this isn't whitespace
			break;	//Stop parsing

	newstr=DuplicateString(str);
	if(dealloc)
		free(originalstr);
	return newstr;	//Return new string
}

void PostProcessLyrics(void)
{
	struct Lyric_Line *lineptr=NULL;
	struct Lyric_Piece *pieceptr=NULL;
	struct Lyric_Piece *temp=NULL;	//The lyric piece being appended
	unsigned long ctr=0;			//Used to validate the piece count in each line
	unsigned long totalpiecectr=0;	//Used to validate the piece count among all lines of lyrics
	unsigned long totallinectr=0;	//Used to validate the line count
	char hyphenadded=0;				//Used to track whether a hyphen was inserted, so it isn't immediatly removed by the hyphen truncation logic
	unsigned long start=0,stop=0;	//The recorded beginning and ending timestamps of lyric lines (for duration validation)

	if(Lyrics.verbose)	(void) puts("Performing import post-processing\n");

	assert_wrapper(Lyrics.line_on == 0);	//It's expected that there are no unclosed lyric lines
	assert_wrapper(Lyrics.piececount != 0);	//It's expected that there are lyrics

//Validate line variables and lyric grouping, perform hyphen logic
	totalpiecectr=totallinectr=0;
	for(lineptr=Lyrics.lines;lineptr!=NULL;lineptr=lineptr->next,totallinectr++)
	{	//For each line of lyrics
		ctr=0;

		if(lineptr->next != NULL)					//If there's another line of lyrics
			if((lineptr->next)->prev != lineptr)	//If next line doesn't properly point backward to this line
			{
				(void) puts("Error: Lyrics lines list damaged\nAborting");
				exit_wrapper(1);
			}

		for(pieceptr=lineptr->pieces;pieceptr!=NULL;pieceptr=pieceptr->next)
		{	//For each lyric in the line
			if(pieceptr == lineptr->pieces)	//If this is the first lyric in the line
				start=pieceptr->start;		//Record its timestamp

			if(pieceptr->next != NULL)					//If there's another lyric in this line
			{
				if((pieceptr->next)->prev != pieceptr)	//If the next lyric doesn't properly point backward to this lyric
				{
					(void) puts("Error: Lyric list damaged\nAborting");
					exit_wrapper(2);
				}
			}
			else
				stop=pieceptr->start+pieceptr->duration;	//Record the ending timestamp of the last lyric of this line

		//Validate grouping status (if grouping is specified, there must be another lyric in the line)
			if(pieceptr->groupswithnext && (pieceptr->next == NULL))
				//If this piece groups with next, but there is no next lyric, disable grouping status
				pieceptr->groupswithnext=0;

			assert_wrapper(pieceptr->lyric != NULL);

			hyphenadded=0;	//Reset this status
		//Perform hyphen insertion first (to prevent the hyphens from being added after truncation)
			if(((Lyrics.nohyphens & 1) == 0) && pieceptr->groupswithnext)
			//If hyphen insertion isn't disabled, and this piece groups with the next
				if(pieceptr->lyric[strlen(pieceptr->lyric)-1] != '-')	//If the lyric doesn't already end in a hyphen
				{
					assert_wrapper(pieceptr->next != NULL);	//If there is no next piece, but this piece was marked for grouping, abort program
					assert_wrapper(pieceptr->next->lyric != NULL);	//The next lyric needs to have a string defined
					if(((pieceptr->lyric)[0] != '+') && ((pieceptr->next->lyric)[0] != '+'))
					{	//Only append an inserted hyphen if this lyric and the next lyric aren'pitch shifts
						pieceptr->lyric=ResizedAppend(pieceptr->lyric,"-",1);//Resize the string to include an appended hyphen
						hyphenadded=1;
					}
				}

		//Perform hyphen truncation
			if((strlen(pieceptr->lyric) > 1) && (pieceptr->lyric[strlen(pieceptr->lyric)-1] == '-') && !hyphenadded)
			//If the last character in this lyric piece is a hyphen and the piece is more than just "-", AND the hyphen wasn't just added by the hyphen insertion logic
				if((Lyrics.nohyphens & 2) != 0)	//If hyphens from input lyrics are to be suppressed
					pieceptr->lyric[strlen(pieceptr->lyric)-1]='\0';	//truncate the hyphen

		//Handle equal sign character handling
			if(pieceptr->hasequal)
			{	//If the input lyric had an equal sign, append it without regard to the nohyphens setting
				if(Lyrics.out_format == MIDI_FORMAT)		//If outputting to MIDI, this must be written back as '='
					pieceptr->lyric=ResizedAppend(pieceptr->lyric,"=",1);//Resize the string to include an appended = char
				else if(pieceptr->lyric[strlen(pieceptr->lyric)-1] != '-')	//If lyric doesn't already end in a hyphen
					pieceptr->lyric=ResizedAppend(pieceptr->lyric,"-",1);//Resize the string to include an appended hyphen
			}

			ctr++;	//Another lyric piece counted
		}//For each lyric in the line

		if(ctr != lineptr->piececount)
		{
			printf("Error: The lyric piece count in the line #%lu is incorrect.  Was %lu but was expected to be %lu\nAborting", totallinectr + 1, ctr, lineptr->piececount);
			exit_wrapper(3);
		}

//Validate line duration
		assert_wrapper(stop >= start);	//The line of lyrics cannot end before it begins
		if(lineptr->duration != stop-start)
		{
			printf("Error: The lyric piece duration sum in line #%lu is incorrect.  Was %lu but was expected to be %lu\nAborting", totallinectr + 1, lineptr->duration, stop-start);
			exit_wrapper(4);
		}
		totalpiecectr+=ctr;

//If lyric import left an un-ended, empty lyric line in progress (ie. ended in a tambourine section)
		if((lineptr->next == NULL) && (lineptr->piececount == 0))
		{	//If this is the last line in the structure and it
			assert_wrapper(Lyrics.line_on);				//This condition shouldn't be possible unless line_on is nonzero
			assert_wrapper(lineptr->pieces == NULL);	//And the line's lyric list is empty
			if(lineptr->prev != NULL)
				(lineptr->prev)->next=NULL;		//Previous line points forward to nothing
			free(lineptr);
			lineptr=NULL;
			(void) puts("(Discarded empty unfinalized line of lyrics)\n");
			break;	//Break from for loop
		}
	}//For each line of lyrics

	if(totalpiecectr != Lyrics.piececount)
	{
		(void) puts("Error: The total lyric count in the Lyrics structure is incorrect\nAborting");
		exit_wrapper(5);
	}

	if(totallinectr != Lyrics.linecount)
	{
		printf("Error: The total line count in the Lyrics structure (%lu) is incorrect (%lu expected)\nAborting\n",totallinectr,Lyrics.linecount);
		exit_wrapper(6);
	}

//Perform grouping logic
	if(Lyrics.grouping)
	{
		for(lineptr=Lyrics.lines;lineptr!=NULL;lineptr=lineptr->next)	//For each line of lyrics
		{
			//Perform word grouping
			pieceptr=lineptr->pieces;
			while(pieceptr!=NULL)
			{	//For each lyric in the line
				if(pieceptr->next == NULL)
					break;			//No more lyrics to group

				temp=pieceptr->next;
				assert_wrapper((pieceptr->lyric!=NULL) && (temp!=NULL) && (temp->lyric!=NULL));

				if(pieceptr->groupswithnext)
				{	//If this piece groups with next, Combine the two lyric pieces
					pieceptr->lyric=ResizedAppend(pieceptr->lyric,temp->lyric,1);
				}
				else if(Lyrics.grouping == 2)
				{	//If line grouping is enabled, append a space followed by the next lyric piece
					pieceptr->lyric=ResizedAppend(pieceptr->lyric," ",1);	//Append a space
					pieceptr->lyric=ResizedAppend(pieceptr->lyric,temp->lyric,1);
				}
				else	//No word grouping to perform on this lyric
				{
					pieceptr=pieceptr->next;	//Point to next lyric in the line
					continue;			//Skip code below and start over
				}

				//Grouping was performed, amend the lyric line accordingly
				pieceptr->duration=temp->start+temp->duration-pieceptr->start;	//The new duration is the duration from the start of the current lyric to the end of the appended lyric

				//Inherit these values
				pieceptr->groupswithnext=temp->groupswithnext;
				pieceptr->next=temp->next;

				//Deallocate the removed link, correct link pointers
				free(temp->lyric);
				free(temp);

				lineptr->piececount--;
				Lyrics.piececount--;
				if(pieceptr->next != NULL)	//If there is a piece after the combined lyric,
					pieceptr->next->prev=pieceptr;	//Have it point backward to this piece
			}//while(pieceptr!=NULL)
		}
	}
}

void SetTag(char *string,char tagID,char negatizeoffset)
{
	char *string2;	//This will be created as a copy of the input string, having any leading/trailing whitespace removed

	assert_wrapper(string != NULL);	//This must not be NULL

	string2=TruncateString(string,0);	//Copy string, removing leading/trailing whitespace
	switch(tagID)
	{
		case 'n':	//Store Title tag
			if(Lyrics.Title != NULL)
			{
				printf("Warning: Extra Title tag: \"%s\".  Ignoring\n",string2);
				free(string2);
			}
			else
			{
				Lyrics.Title=string2;
				if(Lyrics.verbose)	printf("Loaded tag: \"name = %s\"\n",string2);
			}
		break;

		case 's':	//Store Artist tag
			if(Lyrics.Artist != NULL)
			{
				printf("Warning: Extra Artist tag: \"%s\".  Ignoring\n",string2);
				free(string2);
			}
			else
			{
				Lyrics.Artist=string2;
				if(Lyrics.verbose)	printf("Loaded tag: \"artist = %s\"\n",string2);
			}
		break;

		case 'a':	//Store Album tag
			if(Lyrics.Album != NULL)
			{
				printf("Warning: Extra Album tag: \"%s\".  Ignoring\n",string2);
				free(string2);
			}
			else
			{
				Lyrics.Album=string2;
				if(Lyrics.verbose)	printf("Loaded tag: \"album = %s\"\n",string2);
			}
		break;

		case 'e':	//Store Editor tag
			if(Lyrics.Editor != NULL)
			{
				printf("Warning: Extra Editor tag: \"%s\".  Ignoring\n",string2);
				free(string2);
			}
			else
			{
				Lyrics.Editor=string2;
				if(Lyrics.verbose)	printf("Loaded tag: \"frets = %s\"\n",string2);
			}
		break;

		case 'o':	//Store Offset tag
			if((Lyrics.Offset != NULL) || (Lyrics.offsetoverride != 0))
			{	//Only store this tag if it wasn't already defined (ie. given via command line)
				if(Lyrics.offsetoverride != 0)
					(void) puts("Offset controlled via command line.  Ignoring Offset tag");
				else
					printf("Warning: Extra Offset tag: \"%s\".  Ignoring\n",string2);

				free(string2);
			}
			else
			{
				assert_wrapper(string2 != NULL);	//atol() may crash the program if it is passed NULL
					//Convert delay string to a real number
				if(strcmp(string2,"0") != 0)
				{	//If song.ini's offset is not zero
					Lyrics.realoffset=atol(string2); //convert to number
					if(Lyrics.realoffset == 0)	//atol returns 0 on error
					{
						printf("Error converting delay tag \"%s\" to integer value\nAborting\n",string2);
						exit_wrapper(1);
					}
					if(Lyrics.realoffset < 0)
					{
						printf("Error: Song.ini delay is not allowed to be negative\nAborting\n");
						exit_wrapper(2);
					}

					if(negatizeoffset)
					{	//Delay is given as positive, but should be treated as negative
						Lyrics.realoffset=-Lyrics.realoffset;
						Lyrics.Offset=DuplicateString("-");	//Begin string with negative sign
						Lyrics.Offset=ResizedAppend(Lyrics.Offset,string2,1);	//Append the Offset string
					}
					else
						Lyrics.Offset=string2;

					if(Lyrics.verbose)
					{
						printf("Loaded tag: \"delay = %s\"\n",string2);
						if(Lyrics.verbose>=2)	printf("\tConverted delay is %ldms\n",Lyrics.realoffset);
						if(negatizeoffset)	(void) puts("The offset was made negative because it will be used subtractively");
					}

					if(negatizeoffset)
						free(string2);
				}
				else	//if song.ini's delay is defined as 0, store the string "0" in the Offset
				{		//as, this is required in order for UltraStar texts with an explicit gap
						//of 0 to work
					Lyrics.Offset=string2;
					if(Lyrics.verbose)	(void) puts("Loaded tag: \"delay = 0\"");
				}
			}
		break;

		case 'y':	//Store Year tag
			if(Lyrics.Year != NULL)
			{
				printf("Warning: Extra Year tag: \"%s\".  Ignoring\n",string2);
				free(string2);
			}
			else
			{
				Lyrics.Year=string2;
				if(Lyrics.verbose)	printf("Loaded tag: \"year = %s\"\n",string2);
			}
		break;

		default:
			(void) puts("Unexpected error during tag handling\nAborting");
			exit_wrapper(3);
		break;
	}
}

size_t FindLongestLineLength(FILE *inf,char exit_on_empty)
{
	size_t maxlinelength = 0, ctr = 0;
	int inputchar=0;

	assert_wrapper(inf != NULL);	//This must not be NULL

//Find the length of the longest line
	if(Lyrics.verbose>=2)	(void) puts("Parsing file to find the length of the longest line");

	rewind_err(inf);		//rewind file
	do{
		ctr=0;			//Reset line length counter
		do{
			inputchar=fgetc(inf);	//get a character, do not exit on EOF
			ctr++;					//increment line length counter
		}while((inputchar != EOF) && (inputchar != '\n'));//Repeat until end of file or newline character is read

		if(ctr > maxlinelength)		//If this line was the longest yet,
			maxlinelength = ctr;	//Store its length
	}while(inputchar != EOF);	//Repeat until end of file is reached

	if(maxlinelength < 2)		//If the input file contained nothing but empty lines or no text at all
	{
		if(!exit_on_empty)
		{
			rewind_err(inf);
			return 0;
		}
		else
		{
			(void) puts("Error: File is empty\nAborting");
			exit_wrapper(1);
		}
	}
	maxlinelength++;		//Must increment this to account for newline character

	if(Lyrics.verbose>=2)
		printf("Longest line detected is %lu characters\n", (unsigned long)maxlinelength);

	rewind_err(inf);		//rewind file
	return maxlinelength;
}

long ftell_err(FILE *fp)
{
	long result;

	assert_wrapper(fp != NULL);
	result=ftell(fp);
	if(result < 0)
	{
		printf("Error determining file position: %s\nAborting\n",strerror(errno));
		exit_wrapper(1);
	}

	return result;
}

void rewind_err(FILE *fp)
{
	assert_wrapper(fp != NULL);
	if(fseek(fp,0,SEEK_SET) != 0)
	{
		printf("Error rewinding file: %s\nAborting\n",strerror(errno));
		exit_wrapper(1);
	}
}

void fseek_err(FILE *stream,long int offset,int origin)
{
	assert_wrapper(stream != NULL);
	if(fseek(stream,offset,origin) != 0)
	{
		printf("Error seeking through file: %s\nAborting\n",strerror(errno));
		exit_wrapper(1);
	}
}

void fread_err(void *ptr,size_t size,size_t count,FILE *stream)
{
	assert_wrapper((ptr != NULL) && (stream != NULL));
	if(fread(ptr,size,count,stream) != count)
	{
		printf("Error reading file: %s\nAborting\n",strerror(errno));
		exit_wrapper(1);
	}
}

void fwrite_err(const void *ptr,size_t size,size_t count,FILE *stream)
{
	assert_wrapper((ptr != NULL) && (stream != NULL));
	if(fwrite(ptr,size,count,stream) != count)
	{
		printf("Error writing to file: %s\nAborting\n",strerror(errno));
		exit_wrapper(1);
	}
}

FILE *fopen_err(const char *filename,const char *mode)
{
	FILE *fp;

	assert_wrapper((filename != NULL) && (mode != NULL));
	fp=fopen(filename,mode);
	if(fp == NULL)
	{
		printf("Error opening file \"%s\": %s\nAborting\n",filename,strerror(errno));
		exit_wrapper(1);
	}

	return fp;
}

void fflush_err(FILE *stream)
{
	assert_wrapper(stream != NULL);
	if(fflush(stream) != 0)
	{
		printf("Error flushing data to stream: %s\nAborting\n",strerror(errno));
		exit_wrapper(1);
	}
}

void fclose_err(FILE *stream)
{
	assert_wrapper(stream != NULL);

	if(fclose(stream) == EOF)
	{
		printf("Error closing file: %s\nAborting\n",strerror(errno));
		printf("%d",errno);
		exit_wrapper(1);
	}
}

void fputc_err(int character,FILE *stream)
{
	assert_wrapper(stream != NULL);
	if(fputc(character,stream) == EOF)
	{
		printf("Error writing to file: %s\nAborting\n",strerror(errno));
		exit_wrapper(1);
	}
}

int fgetc_err(FILE *stream)
{
	int result;

	assert_wrapper(stream != NULL);
	result=fgetc(stream);
	if(result == EOF)
	{
		printf("Error reading from stream/file: %s\nAborting\n",strerror(result));
		exit_wrapper(1);
	}

	return result;
}

char *fgets_err(char *str,int num,FILE *stream)
{
	char *result;

	assert_wrapper((str != NULL) && (stream != NULL));
	result=fgets(str,num,stream);
	if(result == NULL)
	{
		printf("Error reading from file: %s\nAborting\n",strerror(errno));
		exit_wrapper(1);
	}

	return result;
}

void fputs_err(const char *str,FILE *stream)
{
	assert_wrapper((str != NULL) && (stream != NULL));
	if(fputs(str,stream) == EOF)
	{
		printf("Error writing to output file: %s\nAborting\n",strerror(errno));
		exit_wrapper(1);
	}
}

#ifndef USEMEMWATCH
void *malloc_err(size_t size)
{
	void *ptr=malloc(size);

	if(ptr == NULL)
	{
		printf("Error allocating memory: %s\nAborting\n",strerror(errno));
		exit_wrapper(1);
	}

//Keep this line available for debugging
//if(Lyrics.verbose)	printf("Malloc'd pointer=%p\n",ptr);

	return ptr;
}

#endif

char *strcasestr_spec(char *str1,const char *str2)
{	//Performs a case INSENSITIVE search of str2 in str1, returning the character AFTER the match in str1 if it exists, else NULL
	char *temp1=NULL;	//Used for string matching
	const char *temp2=NULL;

	assert_wrapper((str1 != NULL) && (str2 != NULL));

	while(*str1 != '\0')	//Until end of str1 is reached
	{
		if(tolower(*str1) == tolower(*str2))	//If the first character in both strings match
		{
			for(temp1=str1,temp2=str2;((*temp1 != '\0') && (*temp2 != '\0'));temp1++,temp2++)
			{	//Compare remaining characters in str2 with those starting at current index into str1
				if(tolower(*temp1) != tolower(*temp2))
					break;	//string mismatch, break loop and keep looking
			}
			if((*temp2) == '\0')	//If all characters in str2 were matched
				return temp1;		//Return pointer to character after the match in str1
		}
		str1++;	//Increment index into string
	}

	return NULL;	//No match was found, return NULL
}

int ParseTag(char startchar,char endchar,char *inputstring,char negatizeoffset)
{
	char *str;		//A copy of the input string, modified, to be passed to SetTag
	char *temp=NULL,*temp2=NULL,*temp3=NULL;
	size_t length=0;
	char tagID=0;	//The tag identified in the input string

	assert_wrapper(inputstring != NULL);
	str=DuplicateString(inputstring);

//Truncate carriage return and newline characters from the end of the copied input string
	while(1)
	{
		length=strlen(str);
		if((str[length-1]=='\r') || (str[length-1]=='\n'))
			str[length-1]='\0';
		else
			break;
	}

//Verify that a start of tag indicator occurs
	temp2=strchr(str,startchar);
	if(temp2 == NULL)	//If there was no start of tag indicator
	{
		free(str);		//release working copy of string
		return 0;		//return no tag
	}

//Identify which tag, if any, is contained in the copied input string
	temp=NULL;
	if(Lyrics.TitleStringID != NULL)
		temp=strcasestr_spec(str,Lyrics.TitleStringID);

	if((temp != NULL) && (temp <= temp2))	//If the string contained the Title tag that preceded a start of tag indicator
		tagID='n';
	else
	{
		if(Lyrics.ArtistStringID != NULL)
			temp=strcasestr_spec(str,Lyrics.ArtistStringID);

		if((temp != NULL) && (temp <= temp2))	//If the string contained the Artist tag that preceded a start of tag indicator
			tagID='s';
		else
		{
			if(Lyrics.AlbumStringID != NULL)
				temp=strcasestr_spec(str,Lyrics.AlbumStringID);

			if((temp != NULL) && (temp <= temp2))	//If the string contained the Album tag that preceded a start of tag indicator
				tagID='a';
			else
			{
				if(Lyrics.EditorStringID != NULL)
					temp=strcasestr_spec(str,Lyrics.EditorStringID);

				if((temp != NULL) && (temp <= temp2))	//If the string contained the Editor tag that preceded a start of tag indicator
					tagID='e';
				else
				{
					if(Lyrics.OffsetStringID != NULL)
						temp=strcasestr_spec(str,Lyrics.OffsetStringID);

					if((temp != NULL) && (temp <= temp2))	//If the string contained the Offset tag that preceded a start of tag indicator
						tagID='o';
					else
					{
						if(Lyrics.YearStringID != NULL)
							temp=strcasestr_spec(str,Lyrics.YearStringID);

						if((temp != NULL) && (temp <= temp2))	//If the string contained the Year tag that preceded a start of tag indicator
							tagID='y';
						else
						{
							if(strchr(str,startchar) && strchr(str,endchar))	//If the string contains the tag's opening and closing characters
								if(Lyrics.verbose)
									printf("Unrecognized or ignored tag in line \"%s\"\n",str);

							free(str);		//release working copy of string
							return 0;	//Return no tag
						}
					}
				}
			}
		}
	}

//Verify that a start of tag indicator occurs AFTER the tag identifier
	temp2=strchr(temp,startchar);
	if(temp2 == NULL)	//If the start of tag indicator was not found
	{
		free(str);		//release working copy of string
		return -1;		//return invalid tag defined
	}
	temp2++;			//Seek past start of tag indicator

//Verify that an end of tag indicator occurs AFTER the start of tag indicator, truncate string at the end of tag indicator
	if(endchar != '\0')	//Only truncate the string if an end of tag indicator is expected
	{
		temp3=temp2;
		while((*temp3 != endchar) && (*temp3 != '\0'))
			temp3++;	//Parse through string until endchar or null character are found

		if(*temp3 == '\0')	//if the end tag indicator was not found
		{
			free(str);		//release working copy of string
			return -1;		//return invalid tag defined
		}

		*temp3='\0';		//Truncate copied input string
	}

	SetTag(temp2,tagID,negatizeoffset);	//Either free the str string or store it in the Lyrics structure as a tag
	free(str);	//release working copy of string
	return 1;
}

void WriteUnicodeString(FILE *outf,char *str)	//Takes an 8 bit encoded ANSI string and writes it in the two byte encoded character format used by VL
{						//If str is NULL, an empty Unicode string is written (two 0's)
	unsigned long ctr=0;

	assert_wrapper(outf != NULL);	//This must not be NULL

	if(str != NULL)
	{
		for(ctr=0;str[ctr] != '\0';ctr++)	//For each character in the string that precedes the NULL terminator
		{	//Write ANSI character and a 0 value
			fputc_err(str[ctr],outf);
			fputc_err(0,outf);
		}
	}

	//Write Unicode NULL
	fputc_err(0,outf);
	fputc_err(0,outf);
}

struct Lyric_Line *InsertLyricLineBreak(struct Lyric_Line *lineptr,struct Lyric_Piece *lyrptr)
{	//Split the linked list into two different lines, inserting the break in front of the lyric referenced by lyrptr, in the lyric line referenced by lineptr
	//returns the newly-created line structure that now contains the lyric linked list that now begins with lyrptr, or returns lineptr if no split occurred
	struct Lyric_Line *templine=NULL;

	assert_wrapper((lineptr != NULL) && (lyrptr != NULL));	//These must not be NULL

	if(lyrptr->prev == NULL)	//If there is no lyric before this lyric
	{
		if(Lyrics.verbose>=2)	(void) puts("\tLine break ignored");
		return lineptr;			//Ignore this request to split the line, return original line ptr
	}

//Create and insert line structure into linked list
	templine=(struct Lyric_Line *)malloc_err(sizeof(struct Lyric_Line));
	templine->next=lineptr->next;				//New line points forward to whatever this line pointed forward to
	lineptr->next=templine;						//Newly-terminated line points forward to this new line
	templine->prev=lineptr;						//New line points back to the pre-existing line
	templine->piececount=templine->duration=0;	//Init these values

//Split lyric linked list
	templine->pieces=lyrptr;		//Begin new line with the remaining list of lyric pieces
	templine->start=lyrptr->start;	//Set the beginning timestamp of this line of lyrics
	lyrptr->prev->groupswithnext=0;	//Ensure the lyric that occurs immediately before the split has grouping disabled
	lyrptr->prev->next=NULL;		//Last lyric in previous line points forward to nothing
	lyrptr->prev=NULL;				//First lyric in new line points back to nothing

	Lyrics.linecount++;	//Increment this counter
	return templine;	//Return new line conductor
}

void RecountLineVars(struct Lyric_Line *start)
{	//Rebuild piececount and duration for each line of lyrics, starting with the given line
	unsigned long piecectr=0;
	struct Lyric_Line *curline;			//Conductor of the lyric line linked list
	struct Lyric_Piece *curpiece=NULL;	//Condudctor of the lyric linked list

	curline=start;			//Point line conductor to given line
	while(curline != NULL)
	{	//For each line of lyrics
		piecectr=0;

		curpiece=curline->pieces;	//Reset lyric piece linked list conductor
		while(curpiece != NULL)
		{	//For each lyric in this line
			if(curpiece->next == NULL)		//If it's the last lyric in the line
			{
				assert_wrapper(curpiece->start + curpiece->duration > curline->pieces->start);
				curline->duration=curpiece->start + curpiece->duration - curline->pieces->start;
			}

			piecectr++;						//Increment lyric piece counter
			curpiece=curpiece->next;		//Point to next lyric in the line
		}

		curline->piececount=piecectr;
		curline=curline->next;	//Point to next line of lyrics
	}//For each line of lyrics
}

char *ConvertNoteNum(unsigned char notenum)
{	//Map a note number to a note name
	signed int octave=0;	//The positive or negative octave number
	char *string=NULL;		//The converted note name
	char buffer[3]={0};		//A temporary buffer to build a string

	assert_wrapper(notenum<128);	//Valid note numbers are 0->127

//Obtain note letter
	switch(notenum % 12)
	{
		case 1:		//Note C# (run both this case and the next)
			buffer[1]='#';
		case 0:		//Note C
			buffer[0]='C';
			break;
		case 3:		//Note D# (run both this case and the next)
			buffer[1]='#';
		case 2:		//Note D
			buffer[0]='D';
			break;
		case 4:		//Note E
			buffer[0]='E';
			break;
		case 6:		//Note F# (run both this case and the next)
			buffer[1]='#';
		case 5:		//Note F
			buffer[0]='F';
			break;
		case 8:		//Note G# (run both this case and the next)
			buffer[1]='#';
		case 7:		//Note G
			buffer[0]='G';
			break;
		case 10:	//Note A# (run both this case and the next)
			buffer[1]='#';
		case 9:		//Note A
			buffer[0]='A';
			break;
		case 11:	//Note B
			buffer[0]='B';
			break;
		default:
			(void) puts("Logic error: x%12 cannot equal 12\nAborting\n");
			exit_wrapper(1);
			break;
	}

	string=DuplicateString(buffer);	//Store the note letter and sharpness

//Obtain octave signing
	octave = notenum/12 -1;
	if(octave < 0)
		string=ResizedAppend(string,"-1",1);	//Reallocate the string to append a minus sign for the negative octave
	else
	{
//Obtain octave number
		assert_wrapper(octave <= 9);	//The octave must be less than 10
		buffer[0]='0'+octave;			//Convert the octave to a character representation of '0' through '9'
		buffer[1]=0;				//Append a null character to create a valid string
		string=ResizedAppend(string,buffer,1);	//Reallocate the string to append the octave
	}

//The created string should now be composed as: [Note letter][Sharp sign][Octave sign][Octave]
	return string;
}

void ReleaseMemory(char release_all)
{
	struct Lyric_Piece *piecestemp=NULL;
	struct Lyric_Piece *piecesnext=NULL;
	struct Lyric_Line *linestemp=NULL;
	struct Lyric_Line *linesnext=NULL;

	if(Lyrics.verbose>=2)	(void) puts("\tReleasing memory");

	ReleaseMIDI();				//Release its memory

	if(Lyrics.verbose>=2)	(void) puts("\t\tLyric storage structures");

//Release Lyric structure's memory
	linestemp=Lyrics.lines;
	while(linestemp != NULL)
	{	//De-allocate lyric line linked list
		piecestemp=linestemp->pieces;

		while(piecestemp != NULL)
		{	//De-allocate lyric piece linked list
			if(piecestemp->lyric != NULL)
				free(piecestemp->lyric);

			piecesnext=piecestemp->next;	//Save address of next link
			free(piecestemp);			//De-allocate this link
			piecestemp=piecesnext;		//Point to next link
		}

		linesnext=linestemp->next;	//Save address of next link
		free(linestemp);			//De-allocate this link
		linestemp=linesnext;		//Point to next link
	}
	Lyrics.lines = NULL;	//This list is now empty

	if(Lyrics.verbose>=2)	(void) puts("\t\t\tReleasing strings");

//Release tag related strings
	if(Lyrics.Title != NULL)
	{
		free(Lyrics.Title);
		Lyrics.Title=NULL;
	}
	if(Lyrics.Artist != NULL)
	{
		free(Lyrics.Artist);
		Lyrics.Artist=NULL;
	}
	if(Lyrics.Album != NULL)
	{
		free(Lyrics.Album);
		Lyrics.Album=NULL;
	}
	if(Lyrics.Editor != NULL)
	{
		free(Lyrics.Editor);
		Lyrics.Editor=NULL;
	}
	if(Lyrics.TitleStringID != NULL)
	{
		free(Lyrics.TitleStringID);
		Lyrics.TitleStringID=NULL;
	}
	if(Lyrics.ArtistStringID != NULL)
	{
		free(Lyrics.ArtistStringID);
		Lyrics.ArtistStringID=NULL;
	}
	if(Lyrics.AlbumStringID != NULL)
	{
		free(Lyrics.AlbumStringID);
		Lyrics.AlbumStringID=NULL;
	}
	if(Lyrics.EditorStringID != NULL)
	{
		free(Lyrics.EditorStringID);
		Lyrics.EditorStringID=NULL;
	}
	if(Lyrics.OffsetStringID != NULL)
	{
		free(Lyrics.OffsetStringID);
		Lyrics.OffsetStringID=NULL;
	}
	if(Lyrics.Offset && !Lyrics.offsetoverride)	//If the offset was not defined by command line
	{
		free(Lyrics.Offset);
		Lyrics.Offset=NULL;
	}
	if(Lyrics.Year != NULL)
	{
		free(Lyrics.Year);
		Lyrics.Year=NULL;
	}

//Release command line controlled strings, unless ReleaseMemory() was called during format detection instead of during program exit
	if(release_all)
	{
		if(Lyrics.Offset != NULL)
		{
			free(Lyrics.Offset);
			Lyrics.Offset=NULL;
		}
		if(Lyrics.inputtrack != NULL)
		{
			free(Lyrics.inputtrack);
			Lyrics.inputtrack=NULL;
		}
		if(Lyrics.outputtrack != NULL)
		{
			free(Lyrics.outputtrack);
			Lyrics.outputtrack=NULL;
		}
		if(Lyrics.defaultfilter != 0)
		{
			free(Lyrics.filter);
			Lyrics.filter=NULL;
		}
	}
}

int FindNextNumber(char *buffer,unsigned long *startindex)
{	//If there is a numerical character at or after buffer[startindex], nonzero is returned and startindex will contain the index of the character
	unsigned long index;

	assert_wrapper((buffer != NULL) && (startindex != NULL));
	index=*startindex;	//Dereference starting index for ease of use

//Seek past all non numerical characters
	while(buffer[index] != '\0')
	{
		if(isdigit((unsigned char)buffer[index]))	//If this is a numerical character
			break;			//exit loop
		else
			index++;		//iterate to next character
	}

	if(isdigit((unsigned char)buffer[index]))	//If this is a numerical character
	{
		(*startindex)=index;	//Store the index of this number
		return 1;		//Return search hit
	}

	return 0;	//Return search miss
}

struct Lyric_Format *DetectLyricFormat(char *file)
{
	size_t maxlinelength=0;
	unsigned long index=0,ctr=0;
	char *temp=NULL,*temp2=NULL,temp3=0;
	char *buffer=NULL;			//Used for text file testing
	int errorcode=0,jumpcode=0;
	unsigned long processedctr=0;	//The current line number being processed in the text file
	char timestampchar[]="[<";		//Accept any of these characters as valid characters to begin an LRC timestamp
	char quicktemp=0;				//Used to store the original user setting of the quick processing flag (Lyrics.quick)
	FILE *inf=NULL;
	struct Lyric_Format *detectionlist=NULL;	//The linked list of all detected lyric formats in the specified file
	struct Lyric_Format *curdetection=NULL;		//The conductor for the above linked list (used in the MIDI detection logic)
	struct ID3Tag tag={NULL,0,0,0,0,0,0.0,NULL,0,NULL,NULL,NULL,NULL};	//Used for ID3 detection
	static const struct Lyric_Format emptyLyric_Format;	//Auto-initialize all members to 0/NULL
	char isxml=0;	//Tracks whether an XML file header was read

	assert_wrapper(file != NULL);
	InitLyrics();	//Initialize all variables in the Lyrics structure

	if(Lyrics.verbose>=2)	printf("Detecting lyric type of file \"%s\"\n",file);

//Allocate and initialize the linked list of detections to null data
	detectionlist=malloc_err(sizeof(struct Lyric_Format));
	*detectionlist=emptyLyric_Format;	//Reliably initialize all values to 0/NULL
	curdetection=detectionlist;

//Detect text based lyric based lyric types
	inf=fopen_err(file,"rt");		//Open file in text mode

	//Find the length of the longest line
	maxlinelength=FindLongestLineLength(inf,0);
	if(!maxlinelength)
	{	//File is empty
		DestroyLyricFormatList(detectionlist);
		fclose_err(inf);
		return NULL;	//Return invalid file
	}
	buffer=(char *)malloc_err(maxlinelength);

//Read first line, test for presence of "midi" followed by an equal sign (pitched lyric file)
	if(fgets(buffer, (int)maxlinelength,inf) == NULL)	//Read next line of text, capping it to prevent buffer overflow, don't exit on EOF
	{
		free(buffer);
		DestroyLyricFormatList(detectionlist);
		fclose_err(inf);
		return NULL;	//If NULL is returned from fgets, EOF was reached and no bytes were read from file.  Return invalid file
	}

	temp=strstr(buffer,"midi");	//Search for the string "midi"
	temp2=strchr(buffer,'=');	//Search for equal sign
	if(temp && temp2 && (temp2>temp))		//If first line contains "midi" followed by an equal sign
	{
		free(buffer);
		fclose_err(inf);
		detectionlist->format=PITCHED_LYRIC_FORMAT;
		return detectionlist;
	}

	temp=strstr(buffer,"<?xml version");	//Search for indication of XML header tag
	if(temp)
	{
		isxml = 1;
	}
	temp=strstr(buffer,"<vocals");	//Search for the first non XML header line in a Rocksmith vocal XML file, in case the header is omitted
	if(temp)
	{
		isxml = 1;
	}

//Continue reading lines until one begins with something other than #, then test for the text based formats (Script, UltraStar, LRC/ELRC)
	while(!feof(inf))		//Until end of file is reached
	{
		processedctr++;

		temp=strstr(buffer,"<lyrics>");	//Search for lyrics XML tag used in Guitar Praise lyrics
		if(temp && isxml)
		{	//If this tag AND the XML header tag were read
			free(buffer);
			fclose_err(inf);
			detectionlist->format=XML_FORMAT;
			return detectionlist;
		}

		temp=strstr(buffer,"<vocals count");	//Search for vocals XML tag used in Rocksmith lyrics
		if(temp && isxml)
		{	//If this tag AND the XML header tag were read
			free(buffer);
			fclose_err(inf);
			detectionlist->format=RS_FORMAT;
			return detectionlist;
		}

		for(index=0;buffer[index]!='\0';index++)	//Skip leading whitespace
			if(!isspace(buffer[index]))				//If this character is the first non whitespace character in the line
				break;								//Break from this loop

		if(buffer[index] == '\0')		//If this line was just whitespace
		{
			(void) fgets(buffer, (int)maxlinelength,inf);	//Read next line of text, so the EOF condition can be checked, don't exit on EOF
			continue;							//Process next line
		}

		if(buffer[index]=='#')	//If this line is a Script or UltraStar tag
		{
			(void) fgets(buffer, (int)maxlinelength,inf);	//Read next line of text, so the EOF condition can be checked, don't exit on EOF
			continue;			//Skip this line
		}
		temp3=buffer[index];	//Store this character
		errorcode=0;

//Test for UltraStar
		if((temp3=='*') || (temp3==':') || (temp3=='-') || (toupper(temp3)=='F'))
		{	//A character that implies UltraStar format, validate
			index++;	//Seek past line style
			(void) ParseLongInt(buffer,(unsigned long *)&index,processedctr,&errorcode);	//Parse the expected timestamp, return error in errorcode upon failure
			if(!errorcode && (temp3=='-'))	//If this is a line break followed by a timestamp (valid UltraStar syntax)
			{
				free(buffer);
				fclose_err(inf);
				detectionlist->format=USTAR_FORMAT;
				return detectionlist;
			}

			//Otherwise, for all other UltraStar line styles, 2 more valid numbers are expected
			if(!errorcode)
				(void) ParseLongInt(buffer,&index,processedctr,&errorcode);	//Parse the expected duration, return error in errorcode upon failure
			if(!errorcode)
				(void) ParseLongInt(buffer,&index,processedctr,&errorcode);	//Parse the expected pitch, return error in errorcode upon failure

			if(errorcode)	//If the timestamp, duration or the pitch failed to parse
			{
				(void) fgets(buffer, (int)maxlinelength,inf);	//Read next line of text, so the EOF condition can be checked, don't exit on EOF
				continue;
			}
			else			//Otherwise, this was a standard lyric definition (valid UltraStar syntax)
			{
				free(buffer);
				fclose_err(inf);
				detectionlist->format=USTAR_FORMAT;
				return detectionlist;
			}
		}

//Test for Script
		if(isdigit(temp3))
		{	//A numerical character implies Script format, validate
			(void) ParseLongInt(buffer,&index,processedctr,&errorcode);	//Parse the expected timestamp, return error in errorcode upon failure
			if(!errorcode)
				(void) ParseLongInt(buffer,&index,processedctr,&errorcode);	//Parse the expected duration, return error in errorcode upon failure

			if(!errorcode && (strstr(&(buffer[index]),"text") != NULL))	//If the timestamp and duration parsed, and the string "text" was found after the two numbers (valid Script format)
			{
				free(buffer);
				fclose_err(inf);
				detectionlist->format=SCRIPT_FORMAT;
				return detectionlist;
			}
		}

//Test for LRC
		if(strchr(timestampchar,temp3))
		{	//This character is one of defined characters used to start an LRC timestamp, validate
			temp=SeekNextLRCTimestamp(&(buffer[index]));	//Find first timestamp if one exists on this line
			if(temp != NULL)	//If a timestamp is found
			{
				(void) ConvertLRCTimestamp(&temp,&errorcode);
				if(!errorcode)	//If the timestamp parsed correctly (valid LRC format)
				{
					temp2=SeekNextLRCTimestamp(temp);	//Look for a second timestamp on the same line (Extended LRC)
					if(temp2 != NULL)	//If a second timestamp is found
					{
						(void) ConvertLRCTimestamp(&temp2,&errorcode);
						if(!errorcode)	//If the timestamp parsed correctly (valid ELRC format)
						{
							free(buffer);
							fclose_err(inf);
							detectionlist->format=ELRC_FORMAT;
							return detectionlist;
						}
					}

					detectionlist->format=LRC_FORMAT;
				}
			}

			(void) fgets(buffer, (int)maxlinelength,inf);	//Read next line of text, so the EOF condition can be checked, don't exit on EOF
			continue;
		}

//Test for SRT
		if(isdigit(temp3))
		{	//This character is one of defined characters used to start an LRC timestamp, validate
			errorcode=0;	//Reset this, as a failed Script match would set errorcode
			temp=SeekNextSRTTimestamp(&(buffer[index]));	//Find first timestamp if one exists on this line
			if(temp != NULL)	//If a timestamp is found
			{
				(void) ConvertSRTTimestamp(&temp,&errorcode);
				if(!errorcode)	//If the timestamp parsed correctly (valid SRT format)
				{
					temp2=SeekNextSRTTimestamp(temp);	//Look for a second timestamp on the same line
					if(temp2 != NULL)	//If a second timestamp is found
					{
						(void) ConvertSRTTimestamp(&temp2,&errorcode);
						if(!errorcode)	//If the timestamp parsed correctly (valid ELRC format)
						{
							free(buffer);
							fclose_err(inf);
							detectionlist->format=SRT_FORMAT;
							return detectionlist;
						}
					}

				}
			}

			(void) fgets(buffer,(int)maxlinelength,inf);	//Read next line of text, so the EOF condition can be checked, don't exit on EOF
			continue;
		}

//Test for C9C
		if(strstr(&(buffer[index]),"ENDFILE") != NULL)
		{	//BandJam files end in a line beginning with this string
			free(buffer);
			fclose_err(inf);
			detectionlist->format=C9C_FORMAT;
			return detectionlist;
		}

	//At this point, the line wasn't identified to be any particular format, process next line
		(void) fgets(buffer,(int)maxlinelength,inf);	//Read next line of text, so the EOF condition can be checked, don't exit on EOF
	}//while(!feof(inf))

	if(detectionlist->format == LRC_FORMAT)
	{
		free(buffer);
		fclose_err(inf);
		return detectionlist;
	}

//Detect binary based lyric types
	fclose_err(inf);
	inf=fopen_err(file,"rb");		//Open file in binary mode
	free(buffer);	//This doesn't need to be used anymore
	buffer=NULL;

//Test for VL file
	errorcode=VL_PreLoad(inf,1);	//Load and validate VL file (without exiting upon failure)
	ReleaseVL();
	ReleaseMemory(0);

	if(errorcode == 0)
	{
		fclose_err(inf);
		detectionlist->format=VL_FORMAT;
		return detectionlist;		//Success (valid VL file)
	}

	if(errorcode > 1)	//If VL file header was present, but the VL file was invalid
	{
		fclose_err(inf);
		DestroyLyricFormatList(detectionlist);
		return NULL;		//Return invalid file
	}

//Test for MP3 file with ID3 tag
	rewind_err(inf);
	tag.fp=inf;
	if(FindID3Tag(&tag) != 0)	//Find start and end of ID3 tag
	{	//If the ID3 tag was found
		fseek_err(tag.fp,tag.tagend,SEEK_SET);	//Seek to the first MP3 frame (immediately after the ID3 tag)
		if(GetMP3FrameDuration(&tag) != 0)	//Find the sample rate defined in the MP3 frame
		{	//If the MP3 head was able to be parsed
			fseek_err(tag.fp,tag.framestart,SEEK_SET);	//Seek to first ID3 frame
			if(SearchPhrase(tag.fp,tag.tagend,NULL,"SYLT",4,1) == 1)	//Search for and seek to SYLT ID3 frame
			{	//If an SYLT frame header was found
				fclose_err(inf);
				detectionlist->format=ID3_FORMAT;
				return detectionlist;	//Success (valid MP3 file with ID3 lyrics)
			}
		}
	}

//Test for MIDI file
	rewind_err(inf);
	InitMIDI();				//Initialize all variables in the MIDI structure
	quicktemp=Lyrics.quick;	//Store this value
	Lyrics.quick=0;			//Force quick processing OFF (so MIDI_Load will not skip parsing entire tracks)
	useFLjumpbuffer=1;	//Allow FLC's logic to intercept in exit_wrapper
	jumpcode=setjmp(FLjumpbuffer);
	if(jumpcode!=0) //if program control returned to the setjmp() call above returning any nonzero value
	{
		ReleaseMemory(0);
		fclose_err(inf);
		useFLjumpbuffer=0;		//Restore normal functionality of exit_wrapper
		Lyrics.quick=quicktemp;	//Restore original quick processing setting
		DestroyLyricFormatList(detectionlist);
		return NULL;			//This statement is reached if ReadMIDIHeader() below calles exit_wrapper(), indicating an invalid MIDI header (it is not a MIDI or any of the above defined lyric types, return unknown file)
	}

	ReadMIDIHeader(inf,1);	//Suppress errors regarding invalid MIDI header

	//If this point is reached, ReadMIDIHeader() completed without error, indicating the file begins with a valid MIDI header
	rewind_err(inf);
	ReleaseMIDI();	//Deallocate MIDI structures populated by ReadMIDIHeader()
	jumpcode=setjmp(FLjumpbuffer);
	if(jumpcode!=0) //if program control returned to the setjmp() call above returning any nonzero value
	{
		ReleaseMemory(0);
		fclose_err(inf);
		useFLjumpbuffer=0;	//Restore normal functionality of exit_wrapper
		Lyrics.quick=quicktemp;	//Restore original quick processing setting
		DestroyLyricFormatList(detectionlist);
		return NULL;	//This statement is reached if MIDI_Load() below calles exit_wrapper(), indicating an invalid MIDI file
	}
	MIDI_Load(inf,MIDI_Stats,1);	//Call MIDI_Load with the statistics tracking handler.  Lyric structure is NOT re-init'd- it's already populated.  SUPPRESS error messages during detection

	Lyrics.quick=quicktemp;	//Restore original quick processing setting

	//If this point is reached, MIDI_Load() completed without error, indicating that the file is a valid MIDI file
	for(ctr=1;ctr<MIDIstruct.hchunk.numtracks;ctr++)	//For each MIDI track (skipping track 0)
	{
		if((MIDIstruct.hchunk.tracks[ctr]).trackname == NULL)	//If this track name doesn't exist (ie. Track 0)
			continue;	//Skip to next track

		if(!strcasecmp((MIDIstruct.hchunk.tracks[ctr]).trackname,"EVENTS"))	//If this track is named "EVENTS" (RB format MIDI)
			continue;	//Skip to next track

		if(Lyrics.verbose>=2)
			printf("**Track=\"%s\"\ttextcount=%lu\tlyrcount=%lu\tnotecount=%lu\n",(MIDIstruct.hchunk.tracks[ctr]).trackname,(MIDIstruct.hchunk.tracks[ctr]).textcount,(MIDIstruct.hchunk.tracks[ctr]).lyrcount,(MIDIstruct.hchunk.tracks[ctr]).notecount);

//If the tail of the detection list is populated, create and init a new link in the list and update the conductor
		if(curdetection->format != 0)
		{
			curdetection->next=malloc_err(sizeof(struct Lyric_Format));
			*curdetection->next=emptyLyric_Format;	//Reliably initialize all values to 0/NULL
			curdetection=curdetection->next;
		}
//curdetection now points to an initialized link that can be written to

//Skip detection of a Vocal Rhythm MIDI, as there's no good way to tell what would be the vocal rhythm notes compared to regular instrument notes
		if(	!strcasecmp((MIDIstruct.hchunk.tracks[ctr]).trackname,"PART GUITAR") ||
			!strcasecmp((MIDIstruct.hchunk.tracks[ctr]).trackname,"PART BASS") ||
			!strcasecmp((MIDIstruct.hchunk.tracks[ctr]).trackname,"PART GUITAR COOP") ||
			!strcasecmp((MIDIstruct.hchunk.tracks[ctr]).trackname,"PART RHYTHM") ||
			!strcasecmp((MIDIstruct.hchunk.tracks[ctr]).trackname,"PART DRUM") ||
			!strcasecmp((MIDIstruct.hchunk.tracks[ctr]).trackname,"PART DRUMS"))
		{	//If the track is any of RB/FoF instrument tracks
				continue;	//Examine next track
		}

//RB MIDI detection logic
		if((MIDIstruct.hchunk.tracks[ctr]).spacecount == 0)	//if the track has no whitespace in any existing lyric/text events (if any were present)
		{
			if((MIDIstruct.hchunk.tracks[ctr]).notecount && ((MIDIstruct.hchunk.tracks[ctr]).textcount || (MIDIstruct.hchunk.tracks[ctr]).lyrcount))
			{	//If the track contains both note events and lyric/text events
				curdetection->format=MIDI_FORMAT;
				curdetection->track=DuplicateString((MIDIstruct.hchunk.tracks[ctr]).trackname);
				curdetection->count=(MIDIstruct.hchunk.tracks[ctr]).textcount+(MIDIstruct.hchunk.tracks[ctr]).lyrcount;	//Store the number of lyrics
				continue;	//Examine next track
			}
		}

//SKAR detection logic
		if(!strcasecmp((MIDIstruct.hchunk.tracks[ctr]).trackname,"Words"))	//If the track name is "Words"
		{
			if(!(MIDIstruct.hchunk.tracks[ctr]).notecount && (MIDIstruct.hchunk.tracks[ctr]).textcount)	//If the track contains text events and NO note events
			{
				curdetection->format=SKAR_FORMAT;
				curdetection->track=DuplicateString((MIDIstruct.hchunk.tracks[ctr]).trackname);
				curdetection->count=(MIDIstruct.hchunk.tracks[ctr]).textcount;	//Store the number of lyrics
				continue;	//Examine next track
			}
		}
//KAR detection logic
		else	//The track name is not "Words"
		{
			if((MIDIstruct.hchunk.tracks[ctr]).textcount + (MIDIstruct.hchunk.tracks[ctr]).lyrcount > 1)	//If the track contains at least 2 lyric or text events
			{
				curdetection->format=KAR_FORMAT;
				curdetection->track=DuplicateString((MIDIstruct.hchunk.tracks[ctr]).trackname);
				curdetection->count=(MIDIstruct.hchunk.tracks[ctr]).textcount+(MIDIstruct.hchunk.tracks[ctr]).lyrcount;	//Store the number of lyrics
				continue;	//Examine next track
			}
		}
	}//For each MIDI track

//If the detection list is empty, return NULL
	if(detectionlist->format == 0)
	{
		ReleaseMemory(0);
		fclose_err(inf);
		useFLjumpbuffer=0;	//Restore normal functionality of exit_wrapper
		Lyrics.quick=quicktemp;	//Restore original quick processing setting
		DestroyLyricFormatList(detectionlist);
		return NULL;	//No valid lyric format detected
	}

//Parse the detection list and remove an existing unpopulated link, if it exists
	for(curdetection=detectionlist;curdetection != NULL;curdetection=curdetection->next)
	{
		if(curdetection->next != NULL)	//If there's another link
		{
			if(curdetection->next->format == 0)	//If it's unpopulated
			{
				free(curdetection->next);	//Release its memory
				curdetection->next=NULL;	//Remove it from the list
			}
		}
	}

	ReleaseMemory(0);
	fclose_err(inf);
	useFLjumpbuffer=0;	//Restore normal functionality of exit_wrapper
	Lyrics.quick=quicktemp;	//Restore original quick processing setting

//Return the detection list
	return detectionlist;
}

void DEBUG_QUERY_LAST_PIECE(void)
{	//Debugging to query the info for the last lyric piece
	struct Lyric_Piece *debugpiece;

	debugpiece=FindLyricNumber(Lyrics.piececount);
	assert_wrapper(debugpiece != NULL);
	printf("**Last lyric info: Lyric=\"%s\"\tStart=%lums\tTotal line count=%lu\n\n",debugpiece->lyric,debugpiece->start,Lyrics.linecount);
}

void DEBUG_DUMP_LYRICS(void)
{	//Debugging to display all lyrics
	struct Lyric_Line *curline=NULL;
	struct Lyric_Piece *curpiece=NULL;
	unsigned long linectr=0;

	for(curline=Lyrics.lines,linectr=1;curline!=NULL;curline=curline->next,linectr++)
	{	//For each line of lyrics
		printf("Line %lu: \"",linectr);
		for(curpiece=curline->pieces;curpiece!=NULL;curpiece=curpiece->next)
		{	//For each lyric in the ilne
			assert_wrapper(curpiece->lyric != NULL);
			printf("'%s' ",curpiece->lyric);
		}
		(void) puts("\"");
	}
}

void EnumerateFormatDetectionList(struct Lyric_Format *detectionlist)
{
	struct Lyric_Format *ptr=NULL;	//Conductor for the linked list
	int lasttype=0;					//Used to test for the abnormal condition of a file being detected with both MIDI and non-MIDI based formats

	if(detectionlist == NULL)
	{
		(void) puts("\tNot a valid lyric file");
		return;
	}

	for(ptr=detectionlist;ptr!=NULL;ptr=ptr->next)
	{	//For each entry
		switch(ptr->format)
		{
			case MIDI_FORMAT:
			case VRHYTHM_FORMAT:
			case KAR_FORMAT:
			case SKAR_FORMAT:
				if(lasttype == 2)
				{
					(void) puts("Logic error:  A file cannot be both a MIDI format and a non MIDI format");
					return;
				}
				printf("\tTrack \"%s\": Format is %s\t (%lu lyrics)\n",ptr->track,LYRICFORMATNAMES[ptr->format],ptr->count);
				lasttype=1;
			break;

			case SCRIPT_FORMAT:
			case VL_FORMAT:
			case USTAR_FORMAT:
			case LRC_FORMAT:
			case ELRC_FORMAT:
			case PITCHED_LYRIC_FORMAT:
			case ID3_FORMAT:
			case SRT_FORMAT:
			case XML_FORMAT:
			case C9C_FORMAT:
			case RS_FORMAT:
			case RS2_FORMAT:
				if(lasttype == 1)
				{
					(void) puts("Logic error:  A file cannot be both a MIDI format and a non MIDI format");
					return;
				}
				printf("Format is %s\n",LYRICFORMATNAMES[ptr->format]);
				lasttype=2;
			break;

			default:
				(void) puts("ERROR:  Malformed lyric detection list");
			return;
		}
	}//For each entry
}

void DestroyLyricFormatList(struct Lyric_Format *ptr)
{
	struct Lyric_Format *ptr2=NULL,*ptr3=NULL;

	for(ptr2=ptr;ptr2!=NULL;)		//For each link in the list
	{
		if(ptr2->track != NULL)
			free(ptr2->track);	//Free the track name string
		ptr3=ptr2->next;
		free(ptr2);			//Free the link
		ptr2=ptr3;			//Point to next link
	}
}

void ForceEndLyricLine(void)
{
	struct Lyric_Line *templine=NULL;	//Used for removal of an empty, unclosed line

	if(Lyrics.line_on)
	{
		assert_wrapper(Lyrics.curline != NULL);
		if(Lyrics.curline->piececount == 0)	//If this unclosed line is empty
		{	//Manually remove it from the Lyrics structure
			if(Lyrics.curline->prev != NULL)
				(Lyrics.curline->prev)->next=NULL;	//Previous line points forward to nothing

			templine=Lyrics.curline->prev;		//Save pointer to previous line
			free(Lyrics.curline);				//Release empty line
			Lyrics.curline=templine;			//Point conductor to previous line
			Lyrics.line_on=0;					//Mark lyric line status as closed
		}
		else
		{
			EndLyricLine();	//Close it normally
			assert_wrapper(Lyrics.line_on == 0);	//The above call to EndLyricLine() is required to succeed
		}
	}
}

char *ReadString(FILE *inf,unsigned long *bytesread,unsigned long maxread)
{
	unsigned long length=0,index=0;
	unsigned long position=0;
	char *string=NULL;
	int c=0;

	if(inf == NULL)
		return NULL;

//First pass, parse the string to find its length
	errno=0;
	position=ftell_err(inf);	//Save the current file position
	c=fgetc(inf);
	while(c != EOF)
	{
		length++;	//One more byte was read successfully
		if(c=='\0')	//If it was the NULL terminator
			break;	//Exit loop
		c=fgetc(inf);	//Read next character
	}

	if(length == 0)		//If no characters could be read
		return NULL;
	if(maxread && (length > maxread))
		length=maxread;	//Limit the number of characters to read to the number specified by the calling function

//Allocate string and prepare for second pass
	string=malloc_err((size_t)length);		//Length already takes the null terminator into account
	fseek_err(inf,position,SEEK_SET);	//Return to the original file position

//Second pass, read the string into the allocated memory, performings bounds checking
	c=fgetc(inf);	//Read first character
	while(c != EOF)
	{
		if(index + 1 >= length)	//If index points to the last usable index of the allocated string
			break;		//Exit loop to guarantee that the string is truncated to fit and is terminated
		if(c=='\0')		//If it was the NULL terminator
			break;		//Exit loop, and write the NULL terminator below
		string[index++]=c;	//Append character to string
		c=fgetc(inf);	//Read next character
	}
	string[index]='\0';	//Terminate string

	if(bytesread != NULL)	//If a pointer was given to store the number of bytes read from file
		*bytesread=length;	//Store the number

	return string;
}

unsigned long GetFileEndPos(FILE *fp)
{
	unsigned long originalpos, endpos;

	assert_wrapper(fp != NULL);
	originalpos=ftell_err(fp);		//Record original file position
	(void) fseek(fp,0,SEEK_END);			//Seek to end of file
	endpos=ftell_err(fp);			//Record end file position
	fseek_err(fp,originalpos,SEEK_SET);	//Seek to original file position

	return endpos;				//Return end of file position
}

void BlockCopy(FILE *inf, FILE *outf, size_t num)
{
	unsigned char *buffer=NULL;
	int c=0;
	size_t ctr = 0;

	if(Lyrics.verbose >= 2)
	{
		unsigned long pos = (unsigned long)ftell(inf);
		printf("\t\tBlock copying %lu bytes (File position 0x%lX to 0x%lX)\n", (unsigned long)num, pos, pos + (unsigned long)num - 1);
	}

	assert_wrapper((inf != NULL) && (outf != NULL));

	if(num == 0)	//If not copying any data
		return;		//return without doing anything

	buffer=(unsigned char *)malloc((size_t)num);
	if(buffer != NULL)
	{	//Perform block read and copy
		fread_err(buffer,num,1,inf);	//If reading failed
		fwrite_err(buffer,(size_t)num,1,outf);	//If writing failed
		free(buffer);
	}
	else
	{	//Perform slow copy
		(void) puts("Block copy failed, performing slow copy");
		for(ctr = 0; ctr < num; ctr++)
		{
			c = fgetc_err(inf);		//Read one byte from input file
			fputc_err(c, outf);		//Write it to the output file
		}
	}
}

int SearchPhrase(FILE *inf,unsigned long breakpos,unsigned long *pos,const char *phrase,unsigned long phraselen,unsigned char autoseek)
{
	unsigned long originalpos=0;
	unsigned long matchpos=0;
	unsigned char c=0;
	unsigned long ctr=0;		//Used to index into the phrase[] array, beginning with the first character
	unsigned char success=0;
	unsigned long currentpos=0;	//Store the current file position

//Validate input
	if(!inf || !phrase)	//These input parameters are not allowed to be NULL
		return -1;

	if(!phraselen)
		return 0;	//There will be no matches to an empty search array

//Initialize for search
	errno=0;
	originalpos=ftell(inf);	//Store the original file position
	currentpos=originalpos;
	if(errno)		//If there was an I/O error
		return -1;

	c=fgetc(inf);		//Read the first character of the file
	currentpos++;		//Track that one more byte has been read
	if(ferror(inf))		//If there was an I/O error
	{
		(void) fseek(inf,originalpos,SEEK_SET);
		return -1;
	}

//Perform search
	while(!feof(inf))	//While end of file hasn't been reached
	{
		if(breakpos != 0)
		{
			if(currentpos >= breakpos)	//If the exit position was reached
			{
				(void) fseek(inf,originalpos,SEEK_SET);
				return 0;				//Return no match
			}
		}

	//Check if the next character in the search phrase has been matched
		if(c == phrase[ctr])
		{	//The next character was matched
			if(ctr == 0)	//The match was with the first character in the search array
			{
				matchpos=currentpos-1;	//Store the position of this potential match (rewound one byte)
			}
			ctr++;	//Advance to the next character in search array
			if(ctr == phraselen)
			{	//If all characters have been matched
				success=1;
				break;
			}
		}
		else	//Character did not match
			ctr=0;			//Ensure that the first character in the search array is looked for

		c=fgetc(inf);	//Read the next character of the file
		currentpos++;	//Track that one more byte has been read
	}

//Seek to the appropriate file position
	if(success && autoseek)		//If we should seek to the successful match
		(void) fseek(inf,matchpos,SEEK_SET);
	else				//If we should return to the original file position
		(void) fseek(inf,originalpos,SEEK_SET);

	if(ferror(inf))		//If there was an I/O error
		return -1;

//Return match/non match
	if(success)
	{
		if(pos != NULL)
			*pos=matchpos;
		return 1;	//Return match
	}

	return 0;	//Return no match
}

void WritePaddedString(FILE *outf,char *str,unsigned long num,unsigned char padding)
{
	unsigned long ctr=0;

//Validate parameters
	assert_wrapper(outf != NULL);

	if(num == 0)	//Writing 0 bytes to file has the effect of doing nothing
		return;	//so return without doing anything

//Write characters from str if a string was provided
	if(str != NULL)
		while(ctr < num)
		{
			if(str[ctr] == '\0')	//If this is the end of the string
				break;		//Exit loop

			fputc_err(str[ctr++],outf);	//Write the character to the output file, increment counter
		}

//Write any necessary padding
	while(ctr < num)				//For each remaining character up to the limit
	{
		fputc_err(padding,outf);	//Write padding
		ctr++;						//increment counter
	}
}

char *RemoveLeadingZeroes(char *str)	//Allocate and return a string representing str without leading 0's
{
	unsigned int ctr=0;
	char *temp=NULL;
	size_t size=0;

	assert_wrapper(str != NULL);	//This must not be NULL

	if(str[0] != '\0')	//If passed string is at least one character long
		while(1)
		{
			if((str[ctr] == '0') && (str[ctr+1] != '\0'))	//If this char is '0' and the next character isn't NULL terminator
				ctr++;	//increment index
			else
				break;	//if the next character was the NULL terminator, it would break from loop
		}

	size=strlen(&(str[ctr]));
	temp=malloc_err(size+1);		//Allocate enough room to store truncated string AND a NULL terminator
	strcpy(temp,&(str[ctr]));	//Copy input string, starting from past the leading zeroes
	return temp;	//Return new string
}
