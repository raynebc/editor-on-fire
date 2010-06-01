#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>
#include "Lyric_storage.h"
#include "ID3_parse.h"

int SearchValues(FILE *inf,unsigned long breakpos,unsigned long *pos,const unsigned char *phrase,unsigned long phraselen,unsigned char autoseek)
{
	unsigned long originalpos;
	unsigned long matchpos=0;
	unsigned char c;
	unsigned long ctr=0;		//Used to index into the phrase[] array, beginning with the first character
	unsigned char success=0;
	unsigned long currentpos;

//Validate input
	if(!inf || !phrase)	//These input parameters are not allowed to be NULL
		return -1;

	if(!phraselen)
		return 0;	//There will be no matches to an empty search array

//Initialize for search
	errno=0;
	originalpos=ftell(inf);	//Store the original file position
	if(errno)		//If there was an I/O error
		return -1;

	c=fgetc(inf);		//Read the first character of the file
	if(ferror(inf))		//If there was an I/O error
	{
		fseek(inf,originalpos,SEEK_SET);
		return -1;
	}

//Perform search
	while(!feof(inf))	//While end of file hasn't been reached
	{
		if(breakpos != 0)
		{
			currentpos=ftell(inf);	//Check to see if the exit position has been reached
			if(ferror(inf))				//If there was an I/O error
			{
				fseek(inf,originalpos,SEEK_SET);
				return -1;
			}
			if(currentpos >= breakpos)	//If the exit position was reached
			{
				fseek(inf,originalpos,SEEK_SET);
				return 0;				//Return no match
			}
		}

	//Check if the next character in the search phrase has been matched
		if(c == phrase[ctr])
		{	//The next character was matched
			if(ctr == 0)	//The match was with the first character in the search array
			{
				matchpos=ftell(inf)-1;	//Store the position of this potential match (rewound one byte)
				if(ferror(inf))			//If there was an I/O error
				{
					fseek(inf,originalpos,SEEK_SET);
					return -1;
				}
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
	}

//Seek to the appropriate file position
	if(success && autoseek)		//If we should seek to the successful match
		fseek(inf,matchpos,SEEK_SET);
	else				//If we should return to the original file position
		fseek(inf,originalpos,SEEK_SET);

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

char *ReadTextInfoFrame(FILE *inf)
{
	//Expects the file position of inf to be at the beginning of an ID3 Frame header for a Text Information Frame
	//The first byte read (which will be from the Frame ID) is expected to be 'T' as per ID3v2 specifications
	//Reads and returns the string from the frame in a newly-allocated string
	//NULL is returned if the encoding is Unicode, if the frame is malformed or if there is an I/O error
	unsigned long originalpos=0;
	char buffer[11]={0};	//Used to store the ID3 frame header and the string encoding byte
	char *string=NULL;
	unsigned long framesize=0;

	if(inf == NULL)
		return NULL;

//Store the original file position
	originalpos=ftell(inf);
	if(ferror(inf))		//If there was a file I/O error
		return NULL;

	if(fread(buffer,11,1,inf) != 1)	//Read the frame header and the following byte (string encoding)
		return NULL;			//If there was an I/O error, return error

//Validate frame ID begins with 'T', ensure that the compression and encryption bits are not set, ensure Unicode string encoding is not specified
	if((buffer[0] != 'T') || ((buffer[9] & 192) != 0) || (buffer[10] != 0))
	{
		fseek(inf,originalpos,SEEK_SET);	//Return to original file position
		return NULL;				//Return error
	}

//Read the frame size as a Big Endian integer (4 bytes)
	framesize=((unsigned long)buffer[4]<<24) | ((unsigned long)buffer[5]<<16) | ((unsigned long)buffer[6]<<8) | ((unsigned long)buffer[7]);	//Convert to 4 byte integer

	if(framesize < 2)	//The smallest valid frame size is 2, one for the encoding type and one for a null terminator (empty string)
	{
		fseek(inf,originalpos,SEEK_SET);	//Return to original file position
		return NULL;
	}

	string=malloc(framesize);	//Allocate enough space to store the data (one less than the framesize, yet one more for NULL terminator)
	if(!string)			//Memory allocation failure
	{
		fseek(inf,originalpos,SEEK_SET);	//Return to original file position
		return NULL;
	}

//Read the data and NULL terminate the string
	if(fread(string,framesize-1,1,inf) != 1)	//If the data failed to be read
	{
		fseek(inf,originalpos,SEEK_SET);	//Return to original file position
		free(string);
		return NULL;
	}
	string[framesize-1]='\0';

	return string;
}

#ifdef EOF_BUILD
int ReadID3Tags(char *filename,char *artist,char *title,char *year)
#else
int ReadID3Tags(char *filename)
#endif
{
	const char *tags[]={"TPE1","TIT2","TYER"};	//The three tags to try to find in the file
	char *temp;
	int ctr;
	unsigned long ctr2,length;	//Used to validate year tag
	unsigned char yearvalid=1;	//Used to validate year tag
	unsigned char tagcount=0;
	struct ID3Tag tag={NULL,0,0,0,0,0.0};

//Validate parameters
	if(!filename)
		return 0;

#ifdef EOF_BUILD
	if(!artist || !title || !year)
		return 0;
#endif

	tag.fp=fopen(filename,"rb");
	if(tag.fp == NULL)
		return 0;

	if(FindID3Tag(&tag) == 0)	//Find start and end of ID3 tag
	{
		fclose(tag.fp);
		return 0;
	}

	for(ctr=0;ctr<3;ctr++)
	{	//For each tag we're looking for
		fseek(tag.fp,tag.framestart,SEEK_SET);	//Seek to first frame
		if(ferror(tag.fp))		//If there was a file I/O error
		{
			fclose(tag.fp);
			return 0;
		}

		if(SearchValues(tag.fp,tag.tagend,NULL,tags[ctr],4,1) == 1)
		{	//If there is a match for this tag, seek to it
			temp=ReadTextInfoFrame(tag.fp);	//Attempt to read it

			if(temp != NULL)
			{
				switch(ctr)
				{
					case 0: //Store Performer tag, truncated to fit
						if(strlen(temp) > 255)	//If this string won't fit without overflowing
							temp[255] = '\0';	//Make it fit

						#ifdef EOF_BUILD
						strcpy(artist,temp);
						#else
						printf("Tag: %s = \"%s\"\n",tags[ctr],temp);
						#endif

						free(temp);
					break;

					case 1:	//Store Title tag, truncated to fit
						if(strlen(temp) > 255)	//If this string won't fit without overflowing
							temp[255] = '\0';	//Make it fit

						#ifdef EOF_BUILD
						strcpy(title,temp);
						#else
						printf("Tag: %s = \"%s\"\n",tags[ctr],temp);
						#endif

						free(temp);
					break;

					case 2:	//Store Year tag, after it's validated
						length=strlen(temp);
						if(length < 5)
						{	//If the string is no more than 4 characters
							for(ctr2=0;ctr2<length;ctr2++)	//Check all digits to ensure they're numerical
								if(!isdigit(temp[ctr2]))
									yearvalid=0;

							if(yearvalid)
							{
								#ifdef EOF_BUILD
								strcpy(year,temp);
								#else
								printf("Tag: %s = \"%s\"\n",tags[ctr],temp);
								#endif
							}

							free(temp);
						}
					break;

					default:	//This shouldn't be reachable
						free(temp);
						fclose(tag.fp);
					return tagcount;
				}

				tagcount++;	//One more tag was read
			}//if(temp != NULL)
		}//If there is a match for this tag
	}//For each tag we're looking for

	fclose(tag.fp);
	return tagcount;
}

int FindID3Tag(struct ID3Tag *ptr)
{
	unsigned long tagpos;
	unsigned char tagid[4]={'I','D','3',3};	//An ID3 tag will have this in the header
	unsigned char tagarray[4]={0};
	unsigned long tagsize;

	if((ptr == NULL) || (ptr->fp == NULL))
		return 0;

	if(SearchValues(ptr->fp,ptr->tagend,&tagpos,tagid,4,0) == 1)	//Tag found
	{
		fseek(ptr->fp,tagpos+6,SEEK_SET);		//Seek to the ID3 tag size
		if(ferror(ptr->fp))
			return 0;			//Return error upon file I/O error
		ptr->framestart=tagpos+10;		//The first frame exists 10 bytes past the beginning of the tag header
		if(fread(tagarray,4,1,ptr->fp) != 1)	//Read the ID3 tag size
			return 0;			//Return error upon file I/O error

	//Calculate the tag size.  The MSB of each byte is ignored, but otherwise the 4 bytes are treated as Big Endian
		tagsize=(((tagarray[0] & 127) << 21) | ((tagarray[1] & 127) << 14) | ((tagarray[2] & 127) << 7) | (tagarray[3] & 127));

	//Calculate the position of the first byte outside the ID3 tag: tagpos + tagsize + tag header size
		ptr->tagend=tagpos + tagsize + 10;

		if(Lyrics.verbose>=2)
			printf("ID3 tag info:\n\tBegins at byte 0x%lX\n\tEnds after byte 0x%lX\n\tTag size is %ld bytes\n\tFirst frame begins at byte 0x%lX\n\n",ptr->framestart-10,ptr->tagend,tagsize,ptr->framestart);

		return 1;
	}

	return 0;
}

unsigned long GetMP3FrameDuration(struct ID3Tag *ptr)
{
	unsigned char buffer[4];
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
				puts("MP3 info:\n\tNonstandard MPEG version 2.5");
			switch(buffer[2] & 12)	//Mask out all bits except 5 and 6
			{
				case 0:	//0,0
					if(Lyrics.verbose >= 2)
						puts("\tSample rate: 11025 Hz");
					samplerate=11025;
				break;

				case 4:	//0,1
					if(Lyrics.verbose >= 2)
						puts("\tSample rate: 12000 Hz");
					samplerate=12000;
				break;

				case 8: //1,0
					if(Lyrics.verbose >= 2)
						puts("\tSample rate: 8000 Hz");
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
				puts("MP3 info:\n\tMPEG version 2");
			switch(buffer[2] & 12)	//Mask out all bits except 5 and 6
			{
				case 0:	//0,0
					if(Lyrics.verbose >= 2)
						puts("\tSample rate: 22050 Hz");
					samplerate=22050;
				break;

				case 4:	//0,1
					if(Lyrics.verbose >= 2)
						puts("\tSample rate: 24000 Hz");
					samplerate=24000;
				break;

				case 8: //1,0
					if(Lyrics.verbose >= 2)
						puts("\tSample rate: 16000 Hz");
					samplerate=16000;
				break;

				case 12://1,1	Reserved
				default:
				return 0;
			}
		break;

		case 24://1,1	MPEG version 1
			if(Lyrics.verbose >= 2)
				puts("MP3 info:\n\tMPEG version 1");
			switch(buffer[2] & 12)	//Mask out all bits except 5 and 6
			{
				case 0:	//0,0
					if(Lyrics.verbose >= 2)
						puts("\tSample rate: 44100 Hz");
					samplerate=44100;
				break;

				case 4:	//0,1
					if(Lyrics.verbose >= 2)
						puts("\tSample rate: 48000 Hz");
					samplerate=48000;
				break;

				case 8: //1,0
					if(Lyrics.verbose >= 2)
						puts("\tSample rate: 32000 Hz");
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

//Check the Layer Description, located immediately after the MPEG audio version ID (bits 6 and 7 of the second header byte)
	switch(buffer[1] & 6)	//Mask out all bits except 6 and 7
	{
		case 0:	//0,0	Reserved
		return 0;

		case 2:	//0,1	Layer 3
			if(Lyrics.verbose >= 2)
				puts("\tLayer 3");
			samplesperframe=1152;
		break;

		case 4: //1,0	Layer 2
			if(Lyrics.verbose >= 2)
				puts("\tLayer 2");
			samplesperframe=1152;
		break;

		case 6: //1,1	Layer 1
			if(Lyrics.verbose >= 2)
				puts("\tLayer 1");
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
	struct ID3Tag tag={NULL,0,0,0,0,0.0};

	assert_wrapper(inf != NULL);	//This must not be NULL
	tag.fp=inf;

	if(Lyrics.verbose)
		printf("\nImporting ID3 lyrics from file \"%s\"\n\n",Lyrics.infilename);

	if(FindID3Tag(&tag) == 0)	//Find start and end of ID3 tag
		return;			//Return if no ID3 tag was found

	fseek_err(tag.fp,tag.tagend,SEEK_SET);	//Seek to the first MP3 frame (immediately after the ID3 tag)

	if(GetMP3FrameDuration(&tag) == 0)	//Find the sample rate defined in the MP3 frame
		return;			//Return if the sample rate was not found or was invalid

	fseek_err(tag.fp,tag.framestart,SEEK_SET);	//Seek to first ID3 frame

	if(SearchValues(tag.fp,tag.tagend,NULL,"SYLT",4,1) == 1)	//Seek to SYLT ID3 frame
	{	//Perform Synchronized ID3 Lyric import
		SYLT_Parse(&tag);
	}
	else if(SearchValues(tag.fp,tag.tagend,NULL,"USLT",4,1) == 1)	//Seek to USLT ID3 frame
	{	//Perform Unsynchronized ID3 Lyric import
	}
	else
		return;							//Return if neither lyric frame is present

	ForceEndLyricLine();

	if(Lyrics.verbose)	printf("ID3 import complete.  %lu lyrics loaded\n\n",Lyrics.piececount);
}

void SYLT_Parse(struct ID3Tag *tag)
{
	unsigned char frameheader[10]={0};	//This is the ID3 frame header
	unsigned char syltheader[6]={0};	//This is the SYLT frame header, excluding the terminated descriptor string that follows
	char *contentdescriptor=NULL;		//The null terminated string located after the SYLT frame header
	char timestampformat=0;				//Will be set based on the SYLT header (1= MPEG Frames, 2=Milliseconds)
	char *string=NULL;					//Used to store SYLT strings that are read
	unsigned char timestamparray[4]={0};//Used to store the timestamp for a lyric string
	unsigned long timestamp=0;			//The timestamp converted to milliseconds
	long breakpos=0;					//Will be set to the first byte beyond the SYLT frame
	unsigned long framesize=0;
	char groupswithnext=0;				//Used for grouping logic
	char newline=0;						//Tracks the insertion of line breaks
	char linebreaks=0;					//Tracks whether the imported lyrics defined linebreaks (if not, each ID3 lyric should be treated as one line)
	struct Lyric_Piece *ptr=NULL,*ptr2=NULL;		//Used to insert line breaks as necessary
	struct Lyric_Line *lineptr=NULL;				//Used to insert line breaks as necessary

//Validate input
	assert((tag != NULL) && (tag->fp != NULL));
	breakpos=ftell_err(tag->fp);

//Load ID3 frame header
	fread_err(frameheader,10,1,tag->fp);	//Load ID3 frame header
	fread_err(syltheader,6,1,tag->fp);		//Load SYLT frame header
	contentdescriptor=ParseString(tag->fp);	//Load content descriptor string

	if(contentdescriptor == NULL)	//If the content descriptor string couldn't be read
	{
		puts("Damaged Content Descriptor String\nAborting");
		exit_wrapper(1);
	}

//Ensure that there isn't another SYLT frame in this ID3 tag
	if(SearchValues(tag->fp,tag->tagend,NULL,"SYLT",4,0) == 1)	//Search, but do not seek
	{
//Add some frame validation to make sure it really is an ID3 frame
//		puts("Multiple SYLT frames exist\nAborting");
//		exit_wrapper(2);
	}

//Process headers
	if((frameheader[9] & 192) != 0)
	{
		puts("ID3 Compression and Encryption are not supported\nAborting");
		exit_wrapper(3);
	}

	if(syltheader[0] != 0)
	{
		puts("Unicode ID3 lyrics are not supported\nAborting");
		exit_wrapper(4);
	}

	timestampformat=syltheader[4];
	if((timestampformat != 1) && (timestampformat != 2))
	{
		printf("Warning:  Invalid timestamp format (%d) specified, ms timing assumed\n",timestampformat);
		timestampformat=2;	//Assume millisecond timing
//		exit_wrapper(5);
	}

	//Process framesize as a 4 byte Big Endian integer
	framesize=((unsigned long)frameheader[4]<<24) | ((unsigned long)frameheader[5]<<16) | ((unsigned long)frameheader[6]<<8) | ((unsigned long)frameheader[7]);	//Convert to 4 byte integer
	breakpos=breakpos + framesize + 10;	//Find the position that is one byte past the end of the SYLT frame

	if(Lyrics.verbose>=2)
		printf("SYLT frame info:\n\tFrame size is %lu bytes\n\tEnds after byte 0x%lX\n\tTimestamp format: %s\n\tLanguage: %c%c%c\n\tContent Type %d\n\tContent Descriptor: %s\n\n",framesize,breakpos,timestampformat == 1 ? "MPEG frames" : "Milliseconds",syltheader[1],syltheader[2],syltheader[3],syltheader[5],contentdescriptor != NULL ? contentdescriptor : "(none)");

	free(contentdescriptor);	//Release this, it's not going to be used
	contentdescriptor=NULL;

//Load SYLT lyrics
	while(ftell(tag->fp) < breakpos)	//While we haven't reached the end of the SYLT frame
	{
	//Load the lyric text
		string=ParseString(tag->fp);	//Load SYLT lyric string
		if(string == NULL)
		{
			puts("Invalid SYLT lyric string\nAborting");
			exit_wrapper(6);
		}

	//Load the timestamp
		if(fread(timestamparray,4,1,tag->fp) != 1)	//Read timestamp
		{
			puts("Error reading SYLT timestamp\nAborting");
			exit_wrapper(7);
		}

	//Process the timestamp as a 4 byte Big Endian integer
		timestamp=(unsigned long)((timestamparray[0]<<24) | (timestamparray[1]<<16) | (timestamparray[2]<<8) | timestamparray[3]);
		if(timestampformat == 1)	//If this timestamp is in MPEG frames instead of milliseconds
			timestamp=((double)timestampformat * tag->frameduration + 0.5);	//Convert to milliseconds, rounding up

	//Perform grouping logic
		//Handle whitespace at the beginning of the parsed lyric piece as a signal that the piece will not group with previous piece
		if(isspace(string[0]))
			if(Lyrics.curline->pieces != NULL)	//If there was a previous lyric piece on this line
				Lyrics.lastpiece->groupswithnext=0;	//Ensure it is set to not group with this lyric piece

		if(isspace(string[strlen(string)-1]))	//If the lyric ends in a space
			groupswithnext=0;
		else
			groupswithnext=1;

	//Perform line break detection
		if(strchr(string,'\r') || strchr(string,'\n'))	//If this lyric contains a newline or carraige return
		{
			newline=1;				//Remember to insert a line break after the lyric is written
			linebreaks=1;			//Track that line break character(s) were found in the lyrics
		}
		else
			newline=0;

		if(Lyrics.line_on == 0)		//Ensure that a line phrase is started
			CreateLyricLine();

	//Add lyric piece, during testing, I'll just write it with a duration of 1ms
		AddLyricPiece(string,timestamp,timestamp+1,PITCHLESS,groupswithnext);	//Write lyric with no defined pitch

	//Perform line break logic
		if(newline)
		{
			EndLyricLine();
			newline=0;
		}
	}//While we haven't reached the end of the SYLT frame

//If the imported lyrics did not contain line breaks, they must be inserted manually
	if(!linebreaks && Lyrics.piececount)
	{
		if(Lyrics.verbose)	puts("\nImported ID3 lyrics did not contain line breaks, adding...");
		ptr=Lyrics.lines->pieces;
		lineptr=Lyrics.lines;	//Point to first line of lyrics (should be the only line)

		assert_wrapper((lineptr != NULL) && (ptr != NULL));	//This shouldn't be possible if Lyrics.piececount is nonzero

		if(lineptr->next != NULL)	//If there is another line of lyrics defined
			return;					//abort the insertion of automatic line breaks

		while((ptr != NULL) && (ptr->next != NULL))
		{
			ptr2=ptr->next;	//Store pointer to next lyric
			lineptr=InsertLyricLineBreak(lineptr,ptr2);	//Insert a line break between this lyric and the next, line conductor points to new line
			ptr=ptr2;		//lyric conductor points to the next lyric, which is at the beginning of the new line
		}
	}
}
