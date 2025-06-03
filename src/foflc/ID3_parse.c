#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>
#include "Lyric_storage.h"
#include "ID3_parse.h"

#ifdef S_SPLINT_S
//Ensure Splint checks the code for EOF's code base
#define EOF_BUILD
#endif

#ifdef USEMEMWATCH
#ifdef EOF_BUILD	//In the EOF code base, memwatch.h is at the root
#include <allegro.h>	//For Unicode logic
#include "../main.h"
#include "../memwatch.h"
#else
#include "memwatch.h"
#endif
#endif


char *ReadTextInfoFrame(FILE *inf)
{
	long originalpos=0;
	char buffer[5]={0};		//Used to store the ID3 frame header as a NULL terminated string
	char buffer2[7]={0};	//Used to store frame size, flags and string encoding
	char *string=NULL;
	unsigned long framesize=0;
	int unicode = 0, bomsize = 0;

	if(inf == NULL)
		return NULL;

//Store the original file position
	originalpos=ftell(inf);
	if(originalpos < 0)		//If there was a file I/O error
		return NULL;

	if(fread(buffer,4,1,inf) != 1)	//Read the frame ID
		return NULL;			//If there was an I/O error, return error
	buffer[4] = '\0';			//Guarantee this string is terminated to resolve a false positive in Coverity
	if(fread(buffer2,7,1,inf) != 1)	//Read the rest of the frame header and the following byte (string encoding)
		return NULL;			//If there was an I/O error, return error

	if(!IsTextInfoID3FrameID(buffer) || ((buffer2[5] & 192) != 0))	//Verify that it is a valid text information frame
	{	//buffer2[5] is a flag byte whose MSB is expected to be zero
		(void) fseek(inf,originalpos,SEEK_SET);	//Return to original file position
		return NULL;						//Return error
	}

	//Allow EOF to try to parse UTF-8 formatted text, FoFLC will not attempt it
	#ifndef EOF_BUILD
		if(buffer2[6] != 0)
		{	//buffer[6] is the text encoding where 0 means ISO-8859-1 (ie. ANSI)
			(void) fseek(inf,originalpos,SEEK_SET);	//Return to original file position
			return NULL;						//Return error
		}
	#else
		if(buffer2[6] == 1)
		{
			unicode = U_UNICODE;	//UTF-16 with BOM
			bomsize = 2;
		}
		else if(buffer2[6] == 2)
			unicode = U_UNICODE;	//UTF-16BE without BOM
		else if(buffer2[6] == 3)
			unicode = U_UTF8;		//UTF-8
		else if(buffer2[6] != 0)
		{	//Unknown text encoding
			(void) fseek(inf,originalpos,SEEK_SET);	//Return to original file position
			return NULL;						//Return error
		}
	#endif

//Read the frame size as a Big Endian integer (4 bytes), which is the one byte text encoding, followed by any Byte Order Mark, followed by the NON-NULL-TERMINATED text
	framesize=((unsigned long)buffer2[0]<<24) | ((unsigned long)buffer2[1]<<16) | ((unsigned long)buffer2[2]<<8) | ((unsigned long)buffer2[3]);	//Convert to 4 byte integer
	assert(framesize <= 10000);		//Needless assert() to resolve a false positive with Coverity (it's probably impossible for this limit to be triggered with any sane ID3 tag)

	if(framesize < 2)	//The smallest valid frame size is 2, one byte for the encoding type and one for a null terminator (empty string)
	{
		(void) fseek(inf,originalpos,SEEK_SET);	//Return to original file position
		return NULL;
	}

	string=malloc((size_t)framesize + 1);	//Allocate enough space to store the data (one byte less than the framesize, yet one more for NULL terminator and one more in case it's a UTF-16 NULL terminator)
	if(!string)			//Memory allocation failure
	{
		(void) fseek(inf,originalpos,SEEK_SET);	//Return to original file position
		return NULL;
	}

//Read the data and NULL terminate the string
	if(fread(string,(size_t)framesize-1,1,inf) != 1)	//If the data failed to be read
	{
		(void) fseek(inf,originalpos,SEEK_SET);	//Return to original file position
		free(string);
		return NULL;
	}
	string[framesize-1]='\0';
	string[framesize] ='\0';	//In case UTF-16 is in use, the NULL terminator requires two NULL bytes

	#ifdef EOF_BUILD
		if(unicode)
		{	//If the text is in Unicode format, have Allegro convert it
			char *workbuffer = malloc(framesize+1);

			if(workbuffer)
			{	//If the buffer was allocated
				memset(workbuffer, 0, framesize + 1);		//Block set the work buffer to 0
				memcpy(workbuffer, string + bomsize, framesize+1-bomsize);	//Duplicate the Unicode string into the work buffer, EXCLUDING any byte order mark
				do_uconvert(workbuffer, unicode, string, U_CURRENT, framesize+1);	//Convert from the specified Unicode encoding to EOF's current encoding
				free(workbuffer);
			}
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tConverted Unicode ID3 tag metadata \"%s\"", string);
			eof_log(eof_log_string, 1);
		}
	#endif

	return string;
}

void DisplayID3Tag(char *filename)
{
	char *buffer=NULL;	//A buffer to store strings returned from ReadTextInfoFrame()
	struct ID3Tag tag={NULL,0,0,0,0,0,0.0,NULL,0,NULL,NULL,NULL,NULL};
	struct ID3Frame *temp=NULL;	//Conductor for the ID3 frames list

//Validate parameters
	if(!filename)
		return;

	tag.fp=fopen(filename,"rb");
	if(tag.fp == NULL)
		return;

	(void) ID3FrameProcessor(&tag);
	if(tag.id3v1present > 1)	//If the ID3v1 tag exists and is populated
	{
		(void) puts("ID3v1 tag:");
		if(tag.id3v1title != NULL)
			printf("\tTitle = \"%s\"\n",tag.id3v1title);
		if(tag.id3v1artist != NULL)
			printf("\tArtist = \"%s\"\n",tag.id3v1artist);
		if(tag.id3v1album != NULL)
			printf("\tAlbum = \"%s\"\n",tag.id3v1album);
		if(tag.id3v1year != NULL)
			printf("\tYear = \"%s\"\n",tag.id3v1year);
	}

//Display ID3v2 frames
	if(tag.frames != NULL)
	{
		(void) puts("ID3v2 tag:");
		for(temp=tag.frames;temp != NULL;temp=temp->next)
		{	//For each frame that was parsed, check the tags[] array to see if it is a text info frame
			assert_wrapper(temp->frameid != NULL);
			if(IsTextInfoID3FrameID(temp->frameid))
			{
				fseek_err(tag.fp,temp->pos,SEEK_SET);	//Seek to the ID3 frame in question
				buffer=ReadTextInfoFrame(tag.fp);		//Read and store the text info frame content
				if(buffer == NULL)
					printf("\tFrame: %s = Unreadable or Unicode format\n",temp->frameid);
				else
					printf("\tFrame: %s = \"%s\"\n",temp->frameid,buffer);
				free(buffer);	//Release text info frame content
			}
			else
				printf("\t%s frame present\n",temp->frameid);
		}//For each frame that was parsed
	}
	else if(!tag.id3v1present)
		(void) puts("No ID3 Tag detected");

	DestroyID3(&tag);	//Release the ID3 structure's memory
	(void) fclose(tag.fp);
}

int FindID3Tag(struct ID3Tag *ptr)
{
	unsigned long tagpos=0;
	char tagid[3]={'I','D','3'};	//An ID3 tag will have this in the header
	unsigned char header[10]={0};	//Used to store the ID3 tag header
	unsigned char exheader[10]={0};	//The extended header is 6 bytes (plus 4 more if CRC is present)
	unsigned long tagsize=0;

	if((ptr == NULL) || (ptr->fp == NULL))
		return 0;	//Return failure

	if((SearchPhrase(ptr->fp,ptr->tagend,&tagpos,tagid,3,1) != 1) || (tagpos != 0))
		return 0;	//If an ID3 tag header wasn't found at the beginning of the file, return failure

	if(ferror(ptr->fp))
		return 0;	//Return failure upon file I/O error

	if(fread(header,10,1,ptr->fp) != 1)	//Read ID3 header
		return 0;	//Return failure upon I/O error

//Examine the flag in the header pertaining to extended header.  Parse the extended header if applicable
	if(header[5] & 64)	//If the extended header present bit is set
		if(fread(exheader,6,1,ptr->fp) != 1)	//Read extended header
			return 0;	//Return failure upon I/O error

//If applicable, examine the CRC present flag in the extended header.  Parse the CRC data if applicable
	if(exheader[4] & 128)	//If the CRC present bit is set
		if(fread(&(exheader[6]),4,1,ptr->fp) != 1)	//Read CRC data
			return 0;	//Return failure upon I/O error

	ptr->framestart=ftell(ptr->fp);	//Store the file position of the first byte beyond the header (first ID3 frame)

//Calculate the tag size.  The MSB of each byte is ignored, but otherwise the 4 bytes are treated as Big Endian
	tagsize=(((header[6] & 127) << 21) | ((header[7] & 127) << 14) | ((header[8] & 127) << 7) | (header[9] & 127));

//Calculate the position of the first byte outside the ID3 tag: tagpos + tagsize + tag header size
	ptr->tagstart=tagpos;
	ptr->tagend=ptr->framestart+tagsize;

	if(Lyrics.verbose>=2)
		printf("ID3v2 tag info:\n\tBegins at byte 0x%lX\n\tEnds after byte 0x%lX\n\tTag size is %lu bytes\n\tFirst frame begins at byte 0x%lX\n\n",ptr->tagstart,ptr->tagend-1,tagsize,ptr->framestart);

	return 1;
}

unsigned long GetMP3FrameDuration(struct ID3Tag *ptr)
{
	unsigned char buffer[4]={0};
	unsigned long samplerate=0;
	unsigned long samplesperframe=0;

	if((ptr == NULL) || (ptr->fp == NULL))
		return 0;

//Read the 4 byte frame header at the current file position
	errno=0;
	if(fread(buffer,4,1,ptr->fp) != 1)	//If there was a file I/O error
		return 0;

//Check for the "Frame Sync", where the first 11 bits of the frame header are set
	if((buffer[0] != 255) || ((buffer[1] & 224) != 224))	//If the first 11 bits are not set
		return 0;					//Return error

//Check the MPEG audio version ID, located immediately after the frame sync (bits 4 and 5 of the second byte)
//Then mask the fifth and sixth bit of the third header byte to determine the sample rate
	switch(buffer[1] & 24)	//Mask out all bits except 4 and 5
	{
		case 0:	//0,0	Nonstandard MPEG version 2.5
			if(Lyrics.verbose >= 2)
				(void) puts("MP3 info:\n\tNonstandard MPEG version 2.5");
			switch(buffer[2] & 12)	//Mask out all bits except 5 and 6
			{
				case 0:	//0,0
					samplerate=11025;
				break;

				case 4:	//0,1
					samplerate=12000;
				break;

				case 8: //1,0
					samplerate=8000;
				break;

				case 12://1,1	Reserved
				default:
				return 0;
			}
		break;

		case 8: //0,1	Reserved
		return 0;

		case 16://1,0	MPEG version 2
			if(Lyrics.verbose >= 2)
				(void) puts("MP3 info:\n\tMPEG version 2");
			switch(buffer[2] & 12)	//Mask out all bits except 5 and 6
			{
				case 0:	//0,0
					samplerate=22050;
				break;

				case 4:	//0,1
					samplerate=24000;
				break;

				case 8: //1,0
					samplerate=16000;
				break;

				case 12://1,1	Reserved
				default:
				return 0;
			}
		break;

		case 24://1,1	MPEG version 1
			if(Lyrics.verbose >= 2)
				(void) puts("MP3 info:\n\tMPEG version 1");
			switch(buffer[2] & 12)	//Mask out all bits except 5 and 6
			{
				case 0:	//0,0
					samplerate=44100;
				break;

				case 4:	//0,1
					samplerate=48000;
				break;

				case 8: //1,0
					samplerate=32000;
				break;

				case 12://1,1	Reserved
				default:
				return 0;
			}
		break;

		default:	//Not possible
		return 0;
	}

	if(Lyrics.verbose >=2)
		printf("\tSample rate: %lu Hz\n",samplerate);

//Check the Layer Description, located immediately after the MPEG audio version ID (bits 6 and 7 of the second header byte)
	switch(buffer[1] & 6)	//Mask out all bits except 6 and 7
	{
		case 0:	//0,0	Reserved
		return 0;

		case 2:	//0,1	Layer 3
			if(Lyrics.verbose >= 2)
				(void) puts("\tLayer 3");
			samplesperframe=1152;
		break;

		case 4: //1,0	Layer 2
			if(Lyrics.verbose >= 2)
				(void) puts("\tLayer 2");
			samplesperframe=1152;
		break;

		case 6: //1,1	Layer 1
			if(Lyrics.verbose >= 2)
				(void) puts("\tLayer 1");
			samplesperframe=384;
		break;

		default:
		return 0;
	}

	ptr->samplerate=samplerate;
	ptr->samplesperframe=samplesperframe;
	ptr->frameduration=(double)samplesperframe * 1000.0 / (double)samplerate;

	if(Lyrics.verbose >= 2)
		printf("\tDuration of one MPEG frame is %fms\n\n",ptr->frameduration);

	return samplerate;
}

void ID3_Load(FILE *inf)
{	//Parses the file looking for an ID3 tag.  If found, the first MP3 frame is examined to obtain the sample rate
	//Then an SYLT frame is searched for within the ID3 tag.  If found, synchronized lyrics are imported
	struct ID3Tag tag={NULL,0,0,0,0,0,0.0,NULL,0,NULL,NULL,NULL,NULL};
	struct ID3Frame *frameptr=NULL;
	struct ID3Frame *frameptr2=NULL;

	assert_wrapper(inf != NULL);	//This must not be NULL
	tag.fp=inf;

	if(Lyrics.verbose)	printf("Importing ID3 lyrics from file \"%s\"\n\nParsing input MPEG audio file\n",Lyrics.infilename);

	if(ID3FrameProcessor(&tag) == 0)	//Build a list of the ID3 frames
	{
		DestroyID3(&tag);	//Release the ID3 structure's memory
		return;				//Return if no frames were successfully parsed
	}

//Parse MPEG frame header
	fseek_err(tag.fp,tag.tagend,SEEK_SET);	//Seek to the first MP3 frame (immediately after the ID3 tag)
	if(GetMP3FrameDuration(&tag) == 0)		//Find the sample rate defined in the MP3 frame
	{
		DestroyID3(&tag);	//Release the ID3 structure's memory
		return;				//Return if the sample rate was not found or was invalid
	}

	frameptr=FindID3Frame(&tag,"SYLT");		//Search ID3 frame list for SYLT ID3 frame
	frameptr2=FindID3Frame(&tag,"USLT");	//Search ID3 frame list for USLT ID3 frame
	if(frameptr != NULL)
	{	//Perform Synchronized ID3 Lyric import
		fseek_err(tag.fp,frameptr->pos,SEEK_SET);	//Seek to SYLT ID3 frame
		SYLT_Parse(&tag);
	}
	else if(frameptr2 != NULL)
	{	//Perform Unsynchronized ID3 Lyric import
		(void) puts("\aUnsynchronized ID3 lyric import currently not supported");
		exit_wrapper(1);
	}
	else
	{
		DestroyID3(&tag);	//Release the ID3 structure's memory
		return;				//Return if neither lyric frame is present
	}

//Load song tags
	Lyrics.Title=GrabID3TextFrame(&tag,"TIT2",NULL,0);	//Return the defined song title, if it exists
	Lyrics.Artist=GrabID3TextFrame(&tag,"TPE1",NULL,0);	//Return the defined artist, if it exists
	Lyrics.Album=GrabID3TextFrame(&tag,"TALB",NULL,0);	//Return the defined album, if it exists
	Lyrics.Year=GrabID3TextFrame(&tag,"TYER",NULL,0);	//Return the defined year, if it exists

	if(Lyrics.Title == NULL)	//If there was no Title defined in the ID3v2 tag
		Lyrics.Title=DuplicateString(tag.id3v1title);	//Use one defined in the ID3v1 tag if it exists
	if(Lyrics.Artist == NULL)	//If there was no Artist defined in the ID3v2 tag
		Lyrics.Artist=DuplicateString(tag.id3v1artist);	//Use one defined in the ID3v1 tag if it exists
	if(Lyrics.Album == NULL)	//If there was no Album defined in the ID3v2 tag
		Lyrics.Album=DuplicateString(tag.id3v1album);	//Use one defined in the ID3v1 tag if it exists
	if(Lyrics.Year == NULL)		//If there was no Year defined in the ID3v2 tag
		Lyrics.Year=DuplicateString(tag.id3v1year);		//Use one defined in the ID3v1 tag if it exists

	ForceEndLyricLine();
	DestroyID3(&tag);	//Release the ID3 structure's memory

	if(Lyrics.verbose)	printf("ID3 import complete.  %lu lyrics loaded\n\n",Lyrics.piececount);
}

void SYLT_Parse(struct ID3Tag *tag)
{
	unsigned char frameheader[10]={0};	//This is the ID3 frame header
	unsigned char syltheader[6]={0};	//This is the SYLT frame header, excluding the terminated descriptor string that follows
	char *contentdescriptor=NULL;		//The null terminated string located after the SYLT frame header
	char timestampformat=0;				//Will be set based on the SYLT header (1= MPEG Frames, 2=Milliseconds)
	char *string=NULL;					//Used to store SYLT strings that are read
	char *string2=NULL;					//Used to build a display version of the string for debug output (minus newline)
	unsigned char timestamparray[4]={0};//Used to store the timestamp for a lyric string
	unsigned long timestamp=0;			//The timestamp converted to milliseconds
	unsigned long breakpos;				//Will be set to the first byte beyond the SYLT frame
	unsigned long framesize=0;
	char groupswithnext=0;				//Used for grouping logic
	char linebreaks=0;					//Tracks whether the imported lyrics defined linebreaks (if not, each ID3 lyric should be treated as one line)
	struct Lyric_Piece *ptr=NULL,*ptr2=NULL;	//Used to insert line breaks as necessary
	struct Lyric_Line *lineptr=NULL;			//Used to insert line breaks as necessary
	unsigned long length=0;				//Used to store the length of each string that is read from file
	unsigned long filepos=0;			//Used to track the current file position during SYLT lyric parsing

//Validate input
	assert_wrapper((tag != NULL) && (tag->fp != NULL));
	breakpos=ftell_err(tag->fp);

//Load and validate ID3 frame header and SYLT frame header
	fread_err(frameheader,10,1,tag->fp);			//Load ID3 frame header
	fread_err(syltheader,6,1,tag->fp);				//Load SYLT frame header
	if((frameheader[9] & 192) != 0)
	{
		(void) puts("ID3 Compression and Encryption are not supported\nAborting");
		exit_wrapper(3);
	}
	if(syltheader[0] != 0)
	{
		(void) puts("Unicode ID3 lyrics are not supported\nAborting");
		exit_wrapper(4);
	}

//Load and validate content descriptor string
	contentdescriptor=ReadString(tag->fp,NULL,0);	//Load content descriptor string
	if(contentdescriptor == NULL)	//If the content descriptor string couldn't be read
	{
		(void) puts("Damaged Content Descriptor String\nAborting");
		exit_wrapper(1);
	}

//Validate timestamp format
	timestampformat=syltheader[4];
	if((timestampformat != 1) && (timestampformat != 2))
	{
		printf("Warning: Invalid timestamp format (%d) specified, ms timing assumed\n",timestampformat);
		timestampformat=2;	//Assume millisecond timing
	}

//Process framesize as a 4 byte Big Endian integer
	framesize=((unsigned long)frameheader[4]<<24) | ((unsigned long)frameheader[5]<<16) | ((unsigned long)frameheader[6]<<8) | ((unsigned long)frameheader[7]);	//Convert to 4 byte integer
	if(framesize & 0x80808080)	//According to the ID3v2 specification, the MSB of each of the 4 bytes defining the tag size must be zero
		exit_wrapper(5);		//If this isn't the case, the size is invalid
	assert(framesize < 0x80000000);	//Redundant assert() to resolve a false positive with Coverity (this assertion will never be triggered because the above exit_wrapper() call would be triggered first)
	breakpos=breakpos + framesize + 10;	//Find the position that is one byte past the end of the SYLT frame

	if(Lyrics.verbose>=2)
		printf("SYLT frame info:\n\tFrame size is %lu bytes\n\tEnds after byte 0x%lX\n\tTimestamp format: %s\n\tLanguage: %c%c%c\n\tContent Type %u\n\tContent Descriptor: \"%s\"\n\n",framesize,breakpos-1,timestampformat == 1 ? "MPEG frames" : "Milliseconds",syltheader[1],syltheader[2],syltheader[3],syltheader[5],contentdescriptor != NULL ? contentdescriptor : "(none)");

	if(Lyrics.verbose)	(void) puts("Parsing SYLT frame:");

	free(contentdescriptor);	//Release this, it's not going to be used
	contentdescriptor=NULL;

//Load SYLT lyrics
	filepos=ftell_err(tag->fp);
	while(filepos < breakpos)	//While we haven't reached the end of the SYLT frame
	{
	//Load the lyric text
		string=ReadString(tag->fp,&length,0);	//Load SYLT lyric string, save the string length
		if(string == NULL)
		{
			(void) puts("Invalid SYLT lyric string\nAborting");
			exit_wrapper(6);
		}

	//Load the timestamp
		if(fread(timestamparray,4,1,tag->fp) != 1)	//Read timestamp
		{
			(void) puts("Error reading SYLT timestamp\nAborting");
			exit_wrapper(7);
		}

		filepos+=length+4;	//The number of bytes read from the input file during this iteration is the string length and the timestamp length

	//Process the timestamp as a 4 byte Big Endian integer
		timestamp=(unsigned long)((timestamparray[0]<<24) | (timestamparray[1]<<16) | (timestamparray[2]<<8) | timestamparray[3]);

		if(timestampformat == 1)	//If this timestamp is in MPEG frames instead of milliseconds
			timestamp=((double)timestamp * tag->frameduration + 0.5);	//Convert to milliseconds, rounding up

	//Perform line break logic
		assert(string != NULL);		//(check string for NULL again to satisfy cppcheck)
		if((string[0] == '\r') || (string[0] == '\n'))	//If this lyric begins with a newline or carriage return
		{
			EndLyricLine();		//End the lyric line before the lyric is added
			linebreaks=1;		//Track that line break character(s) were found in the lyrics
		}

		if(Lyrics.verbose >= 2)
		{
			string2=DuplicateString(string);		//Make a copy of the string for display purposes
			string2=TruncateString(string2,1);		//Remove leading/trailing whitespace, newline chars, etc.
			printf("Timestamp: 0x%X%X%X%X\t%lu %s\t\"%s\"\t%s",timestamparray[0],timestamparray[1],timestamparray[2],timestamparray[3],timestamp,(timestampformat == 1) ? "MPEG frames" : "Milliseconds",string2,(string[0]=='\n') ? "(newline)\n" : "\n");
			free(string2);
		}

	//Perform grouping logic
		//Handle whitespace at the beginning of the parsed lyric piece as a signal that the piece will not group with previous piece
		if(isspace(string[0]))
			if(Lyrics.curline->pieces != NULL)	//If there was a previous lyric piece on this line
				Lyrics.lastpiece->groupswithnext=0;	//Ensure it is set to not group with this lyric piece

		if(isspace(string[strlen(string)-1]))	//If the lyric ends in a space
			groupswithnext=0;
		else
			groupswithnext=1;

		if(Lyrics.line_on == 0)		//Ensure that a line phrase is started
			CreateLyricLine();

	//Add lyric piece, during testing, I'll just write it with a duration of 1ms
		AddLyricPiece(string,timestamp,timestamp+1,PITCHLESS,groupswithnext);	//Write lyric with no defined pitch
		free(string);	//Free string

		if((Lyrics.lastpiece != NULL) && (Lyrics.lastpiece->prev != NULL) && (Lyrics.lastpiece->prev->groupswithnext))	//If this piece groups with the previous piece
			Lyrics.lastpiece->prev->duration=Lyrics.lastpiece->start-Lyrics.realoffset-Lyrics.lastpiece->prev->start;	//Extend previous piece's length to reach this piece, take the current offset into account
	}//While we haven't reached the end of the SYLT frame

//If the imported lyrics did not contain line breaks, they must be inserted manually
	if(!linebreaks && Lyrics.piececount)
	{
		if(Lyrics.verbose)	(void) puts("\nImported ID3 lyrics did not contain line breaks, adding...");
		ptr=Lyrics.lines->pieces;
		lineptr=Lyrics.lines;	//Point to first line of lyrics (should be the only line)

		assert_wrapper((lineptr != NULL) && (ptr != NULL));	//This shouldn't be possible if Lyrics.piececount is nonzero

		if(lineptr->next != NULL)	//If there is another line of lyrics defined
			return;					//abort the insertion of automatic line breaks

		while((ptr != NULL) && (ptr->next != NULL))	//For each lyric
		{
			ptr2=ptr->next;	//Store pointer to next lyric
			lineptr=InsertLyricLineBreak(lineptr,ptr2);	//Insert a line break between this lyric and the next, line conductor points to new line
			ptr=ptr2;		//lyric conductor points to the next lyric, which is at the beginning of the new line
		}
	}
}

unsigned long ID3FrameProcessor(struct ID3Tag *ptr)
{
	unsigned char header[10]={0};	//Used to store the ID3 frame header
	unsigned long filepos=0;
	int ctr2=0;						//Used to validate frame ID
	struct ID3Frame *temp=NULL;		//Used to allocate an ID3Frame link
	struct ID3Frame *head=NULL;		//Used as the head pointer
	struct ID3Frame *cond=NULL;		//Used as the conductor
	unsigned long ctr=0;			//Count the number of frames processed
	unsigned long framesize=0;		//Used to validate frame size
	char buffer[31]={0};			//Used to read an ID3v1 tag in the absence of an ID3v2 tag
	static struct ID3Frame emptyID3Frame;	//Auto-initialize all members to 0/NULL

//Validate input parameter
	if((ptr == NULL) || (ptr->fp == NULL))
		return 0;	//Return failure

//Find and parse the ID3 header
//ID3v1
	fseek_err(ptr->fp,-128,SEEK_END);	//Seek 128 back from the end of the file, where this tag would exist
	fread_err(buffer,3,1,ptr->fp);		//Read 3 bytes for the "TAG" header
	buffer[3] = '\0';	//Ensure NULL termination
	if(strcasecmp(buffer,"TAG") == 0)	//If this is an ID3v1 header
	{
		if(Lyrics.verbose)	(void) puts("Loading ID3v1 tag");
		ptr->id3v1present=1;	//Track that this tag exists
		fread_err(buffer,30,1,ptr->fp);		//Read the first field in the tag (song title)
		buffer[30] = '\0';	//Ensure NULL termination
		if(buffer[0] != '\0')	//If the string isn't empty
		{
			ptr->id3v1present=2;	//Track that this tag is populated
			ptr->id3v1title=DuplicateString(buffer);
			if(Lyrics.verbose)	(void) puts("\tTitle loaded");
		}
		fread_err(buffer,30,1,ptr->fp);		//Read the second field in the tag (artist)
		buffer[30] = '\0';	//Ensure NULL termination
		if(buffer[0] != '\0')	//If the string isn't empty
		{
			ptr->id3v1present=2;	//Track that this tag is populated
			ptr->id3v1artist=DuplicateString(buffer);
			if(Lyrics.verbose)	(void) puts("\tArtist loaded");
		}
		fread_err(buffer,30,1,ptr->fp);		//Read the third field in the tag (album)
		buffer[30] = '\0';	//Ensure NULL termination
		if(buffer[0] != '\0')	//If the string isn't empty
		{
			ptr->id3v1present=2;	//Track that this tag is populated
			ptr->id3v1album=DuplicateString(buffer);
			if(Lyrics.verbose)	(void) puts("\tAlbum loaded");
		}
		fread_err(buffer,4,1,ptr->fp);		//Read the fourth field in the tag (year)
		buffer[4]='\0';	//Terminate the buffer to make it a string
		if(buffer[0] != '\0')	//If the string isn't empty
		{
			ptr->id3v1present=2;	//Track that this tag is populated
			ptr->id3v1year=DuplicateString(buffer);
			if(Lyrics.verbose)	(void) puts("\tYear loaded");
		}
	}

//ID3v2
//Load ID3 frames into linked list
	rewind_err(ptr->fp);		//Rewind file so that the ID3v2 header can be searched for
	if(FindID3Tag(ptr) == 0)	//Locate the ID3 tag
		return 0;				//Return if there was none

	if(Lyrics.verbose)	(void) puts("Loading ID3v2 tag");
	fseek_err(ptr->fp,ptr->framestart,SEEK_SET);	//Seek to first ID3 frame
	filepos=ftell_err(ptr->fp);						//Record file position of first expected ID3 frame
	while((filepos >= ptr->framestart) && (filepos < ptr->tagend))
	{	//While file position is at or after the end of the ID3 header, before or at end of the ID3 tag
	//Read frame header into buffer
		if(fread(header,10,1,ptr->fp) != 1)
		{
			ptr->frames=head;	//Store the linked list into the ID3 tag structure
			return 0;	//Return failure on I/O error
		}

	//Validate frame ID
		for(ctr2=0;ctr2<4;ctr2++)
		{
			if(!isupper(header[ctr2]) && !isdigit(header[ctr2]))	//If the character is NOT valid for an ID3 frame ID
			{
				ptr->frames=head;	//Store the linked list into the ID3 tag structure
				return ctr;	//Return the number of frames that were already processed
			}
		}

	//Validate frame end
		framesize=((unsigned long)header[4]<<24) | ((unsigned long)header[5]<<16) | ((unsigned long)header[6]<<8) | ((unsigned long)header[7]);	//Convert to 4 byte integer
		if(filepos + framesize + 10 > ptr->tagend)	//If the defined frame size would cause the frame to extend beyond the end of the ID3 tag
		{
			ptr->frames=head;	//Store the linked list into the ID3 tag structure
			return 0;	//Return failure
		}

	//Initialize an ID3Frame structure
		temp=malloc_err(sizeof(struct ID3Frame));	//Allocate memory
		*temp=emptyID3Frame;						//Reliably initialize all values to 0/NULL
		temp->frameid=malloc_err(5);				//Allocate 5 bytes for the ID3 frame ID
		memcpy(temp->frameid,header,4);				//Copy the 4 byte ID3 frame ID into the pre-terminated string
		temp->frameid[4]='\0';						//Terminate the string
		temp->pos=filepos;	//Record the file position of the ID3 frame
		temp->length=((unsigned long)header[4]<<24) | ((unsigned long)header[5]<<16) | ((unsigned long)header[6]<<8) | ((unsigned long)header[7]);
			//Record the frame length (defined in header as 4 byte Big Endian integer)

		//Append ID3Frame link to the list
		if(head == NULL)	//Empty list
		{
			head=temp;	//Point head to new link
			cond=temp;	//Point conductor to new link
		}
		else
		{
			assert_wrapper(cond != NULL);	//The linked list is expected to not be corrupted
			cond->next=temp;	//Conductor points forward to new link
			temp->prev=cond;	//New link points back to conductor
			cond=temp;		//Conductor points to new link
		}

		if(Lyrics.verbose >= 2)	printf("\tFrame ID %s loaded\n",temp->frameid);
		ctr++;	//Iterate counter

		(void) fseek(ptr->fp,framesize,SEEK_CUR);	//Seek ahead to the beginning of the next ID3 frame
		filepos+=framesize + 10;	//Update file position
	}//While file position is at or after the end of the ID3 header, before or at end of the ID3 tag

	if(Lyrics.verbose)	printf("%lu ID3 frames loaded\n\n",ctr);

	ptr->frames=head;	//Store the linked list into the ID3 tag structure
	return ctr;			//Return the number of frames loaded
}

int ValidateID3FrameHeader(struct ID3Tag *ptr)
{
	unsigned long position=0;
	unsigned char buffer[10]={0};	//Used to read the frame header
	unsigned long ctr=0;
	unsigned long framesize=0;
	char success=1;	//Will assume success unless failure is detected

//Validate input parameters
	if((ptr == NULL) || (ptr->fp == NULL))
		return 0;	//Return failure

//Record file position
	position=ftell_err(ptr->fp);

//If file position is less than the start of the ID3 tag, return failure
	if(position < ptr->framestart)
		return 0;	//Return failure

//Read 10 bytes into buffer
	if(fread(buffer,10,1,ptr->fp) != 1)	//If there was a file I/O error
		success=0;	//Record failure

//Validate header:  Verify that each of the first four buffered characters are an uppercase letter or numerical character
	if(success)
	{
		for(ctr=0;ctr<4;ctr++)
		{
			if(!isupper(buffer[ctr]) && !isdigit(buffer[ctr]))	//If the character is valid for an ID3 frame ID
			{
				success=0;	//Record failure
				break;
			}
		}
	}

//Validate frame position:  Verify that the end of the frame occurs at or before the end of the ID3 tag
	if(success)
	{
		framesize=((unsigned long)buffer[4]<<24) | ((unsigned long)buffer[5]<<16) | ((unsigned long)buffer[6]<<8) | ((unsigned long)buffer[7]);	//Convert to 4 byte integer
		if(position + framesize + 10 >= ptr->tagend)	//If the defined frame size would cause the frame to extend beyond the end of the ID3 tag
			success=0;	//Record failure
	}

//Seek to original file position
	fseek_err(ptr->fp,position,SEEK_SET);

//Return failure/success status
	return success;	//Return success/failure status
}

void DestroyID3(struct ID3Tag *ptr)
{
	struct ID3Frame *temp=NULL,*temp2=NULL;

	if(ptr==NULL)
		return;

	if(Lyrics.verbose)	(void) puts("Destroying ID3 tags");

//Release ID3v1 strings
	if(ptr->id3v1title)
	{
		free(ptr->id3v1title);
		ptr->id3v1title=NULL;
	}
	if(ptr->id3v1artist)
	{
		free(ptr->id3v1artist);
		ptr->id3v1artist=NULL;
	}
	if(ptr->id3v1album)
	{
		free(ptr->id3v1album);
		ptr->id3v1album=NULL;
	}
	if(ptr->id3v1year)
	{
		free(ptr->id3v1year);
		ptr->id3v1year=NULL;
	}

//Release ID3v2 frame list
	if(ptr->frames != NULL)
	{
		for(temp=ptr->frames;temp!=NULL;temp=temp2)	//For each link
		{
			temp2=temp->next;	//Remember the address of the next link
			if(temp->frameid != NULL)
				free(temp->frameid);
			free(temp);
		}
		ptr->frames=NULL;
	}
}

unsigned long BuildID3Tag(struct ID3Tag *ptr,FILE *outf)
{
	unsigned long tagpos=0;		//Records the position of the ID3 tag in the output file
	unsigned long framepos=0;	//Records the position of the SYLT frame in the output file
	unsigned long ctr=0;
	struct ID3Frame *temp=NULL;
	struct Lyric_Line *curline=NULL;	//Conductor of the lyric line linked list
	struct Lyric_Piece *curpiece=NULL;	//A conductor for the lyric pieces list
	unsigned long framesize=0;	//The calculated size of the written SYLT frame
	unsigned long tagsize=0;	//The calculated size of the modified ID3 tag
	unsigned char array[4]={0};	//Used to build the 28 bit ID3 tag size
	unsigned char defaultID3tag[10]={'I','D','3',3,0,0,0,0,0,0};
		//If the input file had no ID3 tag, this tag will be written
	unsigned long fileendpos=0;	//Used to store the size of the input file
	char newline=0;				//ID3 spec indicates that newline characters should be used at the beginning
								//of the new line instead of at the end of a line, use this to track for this

//Validate input parameters
	if((ptr == NULL) || (ptr->fp == NULL) || (outf == NULL))
		return 0;	//Return failure

	if(Lyrics.verbose)	printf("\nExporting ID3 lyrics to file \"%s\"\n",Lyrics.outfilename);

//Conditionally copy the existing ID3v2 tag from the source file, or create one from scratch
	if(ptr->frames == NULL)
	{	//If there was no ID3 tag in the input file
		if(Lyrics.verbose)	(void) puts("Writing new ID3v2 tag");
		tagpos=ftell_err(outf);	//Record this file position so the tag size can be rewritten later
		fwrite_err(defaultID3tag,10,1,outf);	//Write a pre-made ID3 tag
	//Write tag information obtained from input file
		if(Lyrics.Title != NULL)
			WriteTextInfoFrame(outf,"TIT2",Lyrics.Title);					//Write song title frame
		if(Lyrics.Artist != NULL)
			WriteTextInfoFrame(outf,"TPE1",Lyrics.Artist);					//Write song artist frame
		if(Lyrics.Album != NULL)
			WriteTextInfoFrame(outf,"TALB",Lyrics.Album);					//Write album frame
}
	else
	{	//If there was an ID3v2 tag in the source file
		rewind_err(ptr->fp);

//If the ID3v2 header isn't at the start of the source file, copy all data that precedes it to the output file
		BlockCopy(ptr->fp,outf,(size_t)(ptr->tagstart - ftell_err(ptr->fp)));

//Copy the original ID3v2 header from source file to output file (record the file position)
		if(Lyrics.verbose)	(void) puts("Copying ID3v2 tag header");
		tagpos=ftell_err(outf);	//Record this file position so the tag size can be rewritten later
		BlockCopy(ptr->fp,outf,(size_t)(ptr->framestart - ftell_err(ptr->fp)));

//Write tag information from input file if applicable, and ensure that equivalent ID3v2 frames from the source file are omitted
		if(Lyrics.Title != NULL)
		{
			WriteTextInfoFrame(outf,"TIT2",Lyrics.Title);					//Write song title frame
			Lyrics.nosrctag=AddOmitID3framelist(Lyrics.nosrctag,"TIT2");	//Linked list create/append to omit source song title frame
		}
		if(Lyrics.Artist != NULL)
		{
			WriteTextInfoFrame(outf,"TPE1",Lyrics.Artist);					//Write song artist frame
			Lyrics.nosrctag=AddOmitID3framelist(Lyrics.nosrctag,"TPE1");	//Linked list create/append to omit source song artist frame
		}
		if(Lyrics.Album != NULL)
		{
			WriteTextInfoFrame(outf,"TALB",Lyrics.Album);					//Write album frame
			Lyrics.nosrctag=AddOmitID3framelist(Lyrics.nosrctag,"TALB");	//Linked list create/append to omit source album frame
		}
		if(Lyrics.Year != NULL)
		{
			WriteTextInfoFrame(outf,"TYER",Lyrics.Year);					//Write year frame
			Lyrics.nosrctag=AddOmitID3framelist(Lyrics.nosrctag,"TYER");	//Linked list create/append to omit source year frame
		}

//Omit any existing SYLT frame from the source file
		Lyrics.nosrctag=AddOmitID3framelist(Lyrics.nosrctag,"SYLT");		//Linked list create/append with SYLT frame ID to ensure SYLT source frame is omitted

//Write all frames from source MP3 to export MP3 except for those in the omit list
		for(temp=ptr->frames;temp!=NULL;temp=temp->next)
		{	//For each ID3Frame in the list
			if(SearchOmitID3framelist(Lyrics.nosrctag,temp->frameid) == 0)	//If the source frame isn't to be omitted
			{
				if(Lyrics.verbose >= 2)	printf("\tCopying frame \"%s\"\n",temp->frameid);

				if((unsigned long)ftell_err(ptr->fp) != temp->pos)	//If the input file isn't already positioned at the frame
					fseek_err(ptr->fp,temp->pos,SEEK_SET);			//Seek to it now
				BlockCopy(ptr->fp,outf,(size_t)temp->length + 10);	//Copy frame body size + header size number of bytes
				ctr++;	//Increment counter
			}
			else if(Lyrics.verbose >= 2)	printf("\tOmitting \"%s\" frame from source file\n",temp->frameid);
		}
	}//If there was an ID3 tag in the input file

	if(Lyrics.verbose)	(void) puts("Writing SYLT frame header");

//Write SYLT frame header
	framepos=ftell_err(outf);	//Record this file position so the frame size can be rewritten later
	fputs_err("SYLTxxxx\xC0",outf);	//Write the Frame ID, 4 dummy bytes for the unknown frame size and the first flag byte (preserve frame for both tag and file alteration)
	fputc_err(0,outf);		//Write second flag byte (no compression, encryption or grouping identity)
	fputc_err(0,outf);		//ASCII encoding
	fputs_err("eng\x02\x01",outf);	//Write the language as "English", the timestamp format as milliseconds and the content type as "lyrics"
	fputs_err(PROGVERSION,outf);	//Embed the program version as the content descriptor
	fputc_err(0,outf);		//Write NULL terminator for content descriptor

//Write SYLT frame using the Lyrics structure
	curline=Lyrics.lines;	//Point lyric line conductor to first line of lyrics

	if(Lyrics.verbose)	(void) puts("Writing SYLT lyrics");

	while(curline != NULL)	//For each line of lyrics
	{
		curpiece=curline->pieces;	//Starting with the first piece of lyric in this line
		while(curpiece != NULL)		//For each piece of lyric in this line
		{
			if(newline)					//If the previous lyric was the last in its line
			{
				fputc_err('\n',outf);	//Append a newline character in front of this lyric entry
				newline=0;				//Reset this status
			}
			fputs_err(curpiece->lyric,outf);	//Write the lyric
			if(Lyrics.verbose >= 2)	printf("\t\"%s\"\tstart=%lu\t",curpiece->lyric,curpiece->start);

//Line/word grouping logic
			if(curpiece->next == NULL)		//If this is the last lyric in the line
			{
				newline=1;
				if(Lyrics.verbose >= 2) printf("(newline)");
			}
			else if(!curpiece->groupswithnext)	//Otherwise, if this lyric does not group with the next
			{
				fputc_err(' ',outf);			//Append a space
				if(Lyrics.verbose >= 2)	printf("(whitespace)");
			}
			if(Lyrics.verbose >= 2)	(void) putchar('\n');

			fputc_err(0,outf);					//Write a NULL terminator
			WriteDWORDBE(outf,curpiece->start);	//Write the lyric's timestamp as a big endian value
			curpiece=curpiece->next;			//Point to next lyric in the line
		}
		curline=curline->next;				//Point to next line of lyrics
	}

	framesize=ftell_err(outf)-framepos-10;	//Find the length of the SYLT frame that was written (minus frame header size)
	ctr++;	//Increment counter

	if(ptr->frames != NULL)
	{	//If the input MP3 had an ID3 tag
//Copy any unidentified data (ie. padding) that occurs between the end of the last ID3 frame in the list and the defined end of the ID3 tag
		for(temp=ptr->frames;temp->next!=NULL;temp=temp->next);		//Seek to last frame link
		fseek_err(ptr->fp,temp->pos + temp->length + 10,SEEK_SET);	//Seek to one byte past the end of the frame
		BlockCopy(ptr->fp,outf,(size_t)(ptr->tagend - ftell_err(ptr->fp)));
	}

	tagsize=ftell_err(outf)-tagpos-10;	//Find the length of the ID3 tag that has been written (minus tag header size)

	if(Lyrics.verbose)	(void) puts("Copying audio data");
	fileendpos=GetFileEndPos(ptr->fp);	//Find the position of the last byte in the input MP3 file (the filesize)

	if(ptr->id3v1present && SearchOmitID3framelist(Lyrics.nosrctag,"*"))	//If the user specified to leave off the ID3v1 tag, and the source MP3 has an ID3 tag
	{
		BlockCopy(ptr->fp,outf,(size_t)(fileendpos - ftell_err(ptr->fp) - 128));			//Copy rest of source file to output file (minus 128 bytes, the size of the ID3v1 tag)
		ptr->id3v1present=0;												//Consider the tag as being removed, so a new ID3v1 tag is written below
	}
	else
		BlockCopy(ptr->fp,outf,(size_t)(fileendpos - ftell_err(ptr->fp)));				//Copy rest of source file to output file

//Write/Overwrite ID3v1 tag
	if(ptr->id3v1present)			//If an ID3v1 tag existed in the source MP3
	{	//Overwrite it with tags from the input file
		if(Lyrics.verbose)	(void) puts("Editing ID3v1 tag");
		fseek_err(outf,-125,SEEK_CUR);		//Seek 125 bytes back, where the first field of this tag should exist
		if(Lyrics.Title != NULL)						//If the input file defined a Title
			WritePaddedString(outf,Lyrics.Title,30,0);	//Overwrite the Title field (30 bytes)
		else
			fseek_err(outf,30,SEEK_CUR);				//Otherwise seek 30 bytes ahead to the next field

		if(Lyrics.Artist != NULL)						//If the input file defined an Artist
			WritePaddedString(outf,Lyrics.Artist,30,0);	//Overwrite the Artist field (30 bytes)
		else
			fseek_err(outf,30,SEEK_CUR);				//Otherwise seek 30 bytes ahead to the next field

		if(Lyrics.Album != NULL)						//If the input file defined an Album
			WritePaddedString(outf,Lyrics.Album,30,0);	//Overwrite the Album field (30 bytes)
		else
			fseek_err(outf,30,SEEK_CUR);				//Otherwise seek 30 bytes ahead to the next field

		if(Lyrics.Year != NULL)							//If the input file defined a Year
			WritePaddedString(outf,Lyrics.Year,4,0);	//Overwrite the Year field (4 bytes)
	}
	else
	{	//Write a new ID3v1 tag
		if(Lyrics.verbose)	(void) puts("Writing new ID3v1 tag");
		fseek_err(outf,0,SEEK_END);	//Seek to end of file
		fputs_err("TAG",outf);		//Write ID3v1 header
		WritePaddedString(outf,Lyrics.Title,30,0);	//Write the Title field (30 bytes)
		WritePaddedString(outf,Lyrics.Artist,30,0);	//Write the Artist field (30 bytes)
		WritePaddedString(outf,Lyrics.Album,30,0);	//Write the Album field (30 bytes)
		WritePaddedString(outf,ptr->id3v1year,4,0);	//Write the Year field (4 bytes)
		WritePaddedString(outf,NULL,30,0);			//Write a blank Comment field (30 bytes)
		fputc_err(255,outf);						//Write unknown genre (1 byte)
	}

	if(Lyrics.verbose)	(void) puts("Correcting ID3 headers");

//Rewind to the SYLT header in the output file and write the correct frame length
	fseek_err(outf,framepos+4,SEEK_SET);	//Seek to where the SYLT frame size is to be written
	WriteDWORDBE(outf,framesize);			//Write the SYLT frame size

//Rewind to the ID3 header in the output file and write the correct ID3 tag length
	fseek_err(outf,ptr->tagstart+6,SEEK_SET);	//Seek to where the ID3 tag size is to be written
	if(tagsize > 0x0FFFFFFF)
	{
		(void) puts("\aError:  Tag size is larger than the ID3 specification allows");
		exit_wrapper(2);
	}
	array[3]=tagsize & 127;			//Mask out everything except the lowest 7 bits
	array[2]=(tagsize>>7) & 127;	//Mask out everything except the 2nd set of 7 bits
	array[1]=(tagsize>>14) & 127;	//Mask out everything except the 3rd set of 7 bits
	array[0]=(tagsize>>21) & 127;	//Mask out everything except the 4th set of 7 bits
	fwrite_err(array,4,1,outf);		//Write the ID3 tag size

	if(Lyrics.verbose)	printf("\nID3 export complete.  %lu lyrics written\n",Lyrics.piececount);

	return ctr;	//Return counter
}

void Export_ID3(FILE *inf, FILE *outf)
{
	struct ID3Tag tag={NULL,0,0,0,0,0,0.0,NULL,0,NULL,NULL,NULL,NULL};

//Validate parameters
	assert_wrapper((inf != NULL) && (outf != NULL));
	assert_wrapper(Lyrics.piececount != 0);	//This function is not to be called with an empty Lyrics structure

	tag.fp=inf;	//Store the input file pointer into the ID3 tag structure

//Seek to first MPEG frame in input file (after ID3 tag, otherwise presume it's at the beginning of the file)
	if(Lyrics.verbose)	(void) puts("Parsing input MPEG audio file");
	if(FindID3Tag(&tag))		//Find start and end of ID3 tag
		fseek_err(tag.fp,tag.tagend,SEEK_SET);	//Seek to the first MP3 frame (immediately after the ID3 tag)
	else
		rewind_err(tag.fp);	//Rewind file if no ID3 tag is found

//Load MPEG information
	if(GetMP3FrameDuration(&tag) == 0)	//Find the sample rate defined in the MP3 frame at the current file position
	{
		printf("Error loading MPEG information from file \"%s\"\nAborting",Lyrics.srcfilename);
		exit_wrapper(1);
	}

//Load any existing ID3 frames
	(void) ID3FrameProcessor(&tag);

//Build output file
	(void) BuildID3Tag(&tag,outf);

//Release memory
	DestroyID3(&tag);	//Release the ID3 structure's memory
	DestroyOmitID3framelist(Lyrics.nosrctag);	//Release ID3 frame omission list
}

struct ID3Frame *FindID3Frame(struct ID3Tag *tag,const char *frameid)
{
	struct ID3Frame *ptr=NULL;

//Validate parameters
	assert_wrapper((tag != NULL) && (tag->fp != NULL) && (tag->frames != NULL) && (frameid != NULL));

//Locate the specified frame
	for(ptr=tag->frames;ptr != NULL;ptr=ptr->next)	//For each frame in the linked list
		if(strcasecmp(ptr->frameid,frameid) == 0)	//If this is the frame being searched for
			return ptr;	//Return success

	return NULL;	//If no result was found, return failure
}

char *GrabID3TextFrame(struct ID3Tag *tag,const char *frameid,char *buffer,unsigned long buffersize)
{
	struct ID3Frame *frameptr=NULL;
	char *string=NULL;

//Validate parameters
	assert_wrapper((tag != NULL) && (tag->fp != NULL) && (tag->frames != NULL) && (frameid != NULL));
	if(buffer != NULL)
		assert_wrapper(buffersize != 0);

//Locate the specified frame
	frameptr=FindID3Frame(tag,frameid);
	if(frameptr == NULL)	//If no match was found
		return NULL;		//return without altering the buffer

//Seek to the frame's location in the file
	if(fseek(tag->fp,frameptr->pos,SEEK_SET) != 0)	//If there was a file I/O error seeking
		return NULL;		//return without altering the buffer

//Read the text info string
	string=ReadTextInfoFrame(tag->fp);
	if(string == NULL)	//If there was an error reading the string
		return NULL;	//return without altering the buffer

//If performing the logic to buffer the string
	if(buffer != NULL)
	{	//Verify string will fit in buffer
		if(strlen(string) + 1 > (size_t)buffersize)	//If the string (including NULL terminator) is larger than the buffer
			string[buffersize-1]='\0';		//truncate the string to fit

		strcpy(buffer,string);	//Copy string to buffer
		free(string);			//Release memory used by the string
		return buffer;			//Return success
	}

//Otherwise return the allocated string
	return string;
}

struct OmitID3frame *AddOmitID3framelist(struct OmitID3frame *ptr,const char *frameid)
{
	struct OmitID3frame *temp;

	assert_wrapper(frameid != NULL);

//Allocate and init new link
	temp=malloc_err(sizeof(struct OmitID3frame));
	temp->frameid=DuplicateString(frameid);

//Append link to existing list if applicable
	temp->next=ptr;		//The new link becomes the head and points forward to whatever the head was

	return temp;	//Otherwise return the new linked list head
}

int SearchOmitID3framelist(struct OmitID3frame *ptr,const char *frameid)
{
	assert_wrapper(frameid != NULL);
	while(ptr != NULL)
	{
		assert_wrapper(ptr->frameid != NULL);
		if(ptr->frameid[0]=='*')
			return 2;	//Return wildcard match
		if(strcasecmp(ptr->frameid,frameid) == 0)
			return 1;	//Return exact match

		ptr=ptr->next;
	}

	return 0;	//Return no match
}

void DestroyOmitID3framelist(struct OmitID3frame *ptr)
{
	struct OmitID3frame *temp=NULL;

	while(ptr != NULL)
	{
		if(ptr->frameid != NULL)
			free(ptr->frameid);

		temp=ptr->next;	//Save address to next link
		free(ptr);		//Free this link
		ptr=temp;		//Point to next link
	}
}

void WriteTextInfoFrame(FILE *outf,const char *frameid,const char *string)
{
	size_t size=0;

//Validate input parameters
	assert_wrapper((outf != NULL) && (frameid != NULL) && (string != NULL));
	if(strlen(frameid) != 4)
	{
		printf("Error: Attempted export of invalid frame ID \"%s\"\nAborting",frameid);
		exit_wrapper(1);
	}

	if(Lyrics.verbose >= 2)	printf("\tWriting frame \"%s\"\n",frameid);

	size=strlen(string)+1;		//The frame payload size is the string and the encoding byte

//Write ID3 frame header
	fwrite_err(frameid,4,1,outf);			//Write frame ID
	WriteDWORDBE(outf,(unsigned long)size);	//Write frame size (total frame size-header size) in Big Endian format
	fputc_err(0xC0,outf);					//Write first flag byte (preserve frame for both tag and file alteration)
	fputc_err(0,outf);						//Write second flag byte (no compression, encryption or grouping identity)

//Write frame data
	fputc_err(0,outf);					//Write ASCII encoding
	fwrite_err(string,(size_t)size-1,1,outf);	//Write the string
}

char IsTextInfoID3FrameID(char *frameid)
{
	#define NUMID3TAGS 38
	const char *tags[NUMID3TAGS]={"TALB","TBPM","TCOM","TCON","TCOP","TDAT","TDLY","TENC","TEXT","TFLT","TIME","TIT1","TIT2","TIT3","TKEY","TLAN","TLEN","TMED","TOAL","TOFN","TOLY","TOPE","TORY","TOWN","TPE1","TPE2","TPE3","TPE4","TPOS","TPUB","TRCK","TRDA","TRSN","TRSO","TSIZ","TSRC","TSSE","TYER"};
	unsigned long ctr=0;
	char found=0;	//Used to track whether each frame was a text info frame defined in the tags[] array above

//Validate parameters
	if(frameid == NULL)
		return 0;

	for(ctr=0;ctr<NUMID3TAGS;ctr++)
	{	//For each defined text info frame id
		if(strcasecmp(tags[ctr],frameid) == 0)
		{	//If the specified text info frame id matches this ID3 frame
			found=1;
			break;	//Stop searching the tags[] array
		}
	}//For each defined text info frame id

	return found;	//Return found status
}
