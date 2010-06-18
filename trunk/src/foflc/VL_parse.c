#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "Lyric_storage.h"
#include "VL_parse.h"

#ifdef USEMEMWATCH
#include <memwatch.h>
#endif


//
//GLOBAL VARIABLE DEFINITIONS
//
struct _VLSTRUCT_ VL;


int VL_PreLoad(FILE *inf,char validate)
{
	long ftell_result;				//Used to store return value from ftell()
	char buffer[5]={0,0,0,0,0};		//Used to read word/doubleword integers from file
	unsigned long ctr;				//Generic counter
	char *temp;						//Temporary pointer for allocated strings
	struct VL_Sync_entry se={0,0,0,0,0,NULL};		//Used to store sync entries from file during parsing
	struct VL_Text_entry *ptr1;		//Used for allocating links for the text chunk list
	struct VL_Sync_entry *ptr2;		//Used for allocation links for the sync chunk list
	struct VL_Text_entry *curtext;	//Conductor for text chunk linked list
	struct VL_Sync_entry *cursync;	//Conductor for sync chunk linked list

	assert_wrapper(inf != NULL);

//Initialize variables
	VL.numlines=0;
	VL.numsyncs=0;
	VL.filesize=0;
	VL.textsize=0;
	VL.syncsize=0;
	VL.Lyrics=NULL;
	VL.Syncs=NULL;
	curtext=NULL;	//list starts out empty
	cursync=NULL;	//list starts out empty
	se.next=NULL;	//In terms of linked lists, this always represents the last link

	if(Lyrics.verbose)	puts("Reading VL file header");

//Read file header
	fread_err(buffer,4,1,inf);		//Read 4 bytes, which should be 'V','L','2',0
	ReadDWORDLE(inf,&(VL.filesize));	//Read doubleword from file in little endian format

	if(strncmp(buffer,"VL2",4) != 0)
	{
		if(validate)
			return 1;
		else
		{
			puts("Error: Invalid file header string\nAborting");
			exit_wrapper(1);
		}
	}

//Read text header
	fread_err(buffer,4,1,inf);			//Read 4 bytes, which should be 'T','E','X','T'
	buffer[4]='\0';						//Add a NULL character to make buffer into a proper string
	ReadDWORDLE(inf,&(VL.textsize));	//Read doubleword from file in little endian format

	if(VL.textsize % 4 != 0)
	{
		puts("Error: VL spec. violation: Sync chunk is not padded to a double-word alignment");
		if(validate)
			return 2;
		else
		{
			puts("Aborting");
			exit_wrapper(2);
		}
	}

	if(strncmp(buffer,"TEXT",5) != 0)
	{
		puts("Error: Invalid text header string");
		if(validate)
			return 3;
		else
		{
			puts("Aborting");
			exit_wrapper(3);
		}
	}

//Read sync header, which should be th.textsize+16 bytes into the file
	fseek_err(inf,VL.textsize+16,SEEK_SET);
	fread_err(buffer,4,1,inf);		//Read 4 bytes, which should be 'S','Y','N','C'
	buffer[4]='\0';					//Add a NULL character to make buffer into a proper string
	ReadDWORDLE(inf,&(VL.syncsize));	//Read doubleword from file in little endian format

	if(strncmp(buffer,"SYNC",5) != 0)
	{
		puts("Error: Invalid sync header string");
		if(validate)
			return 4;
		else
		{
			puts("Aborting");
			exit_wrapper(4);
		}
	}

//Validate chunk sizes given in headers (textsize was already validated if the Sync header was correctly read)
	fseek_err(inf,0,SEEK_END);	//Seek to end of file
	ftell_result = ftell(inf);	//Get position of end of file
	if(ftell_result < 0)
	{
		printf("Error determining file length during file size validation: %s\n",strerror(errno));
		if(validate)
			return 5;
		else
		{
			puts("Aborting");
			exit_wrapper(5);
		}
	}
	if(ftell_result != VL.filesize+8)			//Validate filesize
	{
		puts("Error: Filesize does not match size given in file header");
		if(validate)
			return 6;
		else
		{
			puts("Aborting");
			exit_wrapper(6);
		}
	}
	if(VL.filesize != VL.textsize + VL.syncsize + 16)	//Validate syncsize
	{
		puts("Error: Incorrect size given in sync chunk");
		if(validate)
			return 7;
		else
		{
			puts("Aborting");
			exit_wrapper(7);
		}
	}

//Load tags
	if(Lyrics.verbose)	puts("Loading VL tag strings");
	fseek_err(inf,16,SEEK_SET);		//Seek to the Title tag

	for(ctr=0;ctr<5;ctr++)	//Expecting 5 null terminated unicode strings
	{
		temp=ReadUnicodeString(inf);	//Allocate array and read Unicode string

		if(temp[0] != '\0')	//If the string wasn't empty, save the tag
			switch(ctr)
			{	//Look at ctr to determine which tag we have read, and assign to the appropriate tag string:
				case 0:
					SetTag(temp,'n',0);
				break;

				case 1:
					SetTag(temp,'s',0);
				break;

				case 2:
					SetTag(temp,'a',0);
				break;

				case 3:
					SetTag(temp,'e',0);
				break;

				case 4:	//Offset
					SetTag(temp,'o',0);	//Do not multiply the offset by -1
				break;

				default:
					puts("Unexpected error");
					if(validate)
						return 8;
					else
					{
						puts("Aborting");
						exit_wrapper(8);
					}
				break;
			}//end switch(ctr)

		free(temp);	//Free string, a copy of which would have been stored in the Lyrics structure as necessary
	}//end for(ctr=0;ctr<5;)

//Validate the existence of three empty unicode strings
	for(ctr=0;ctr<3;ctr++)	//Expecting 3 empty, null terminated unicode strings
		if(ParseUnicodeString(inf) != 0)
		{
			puts("Error: Reserved string is not empty during load");
			if(validate)
				return 9;
			else
			{
				puts("Aborting");
				exit_wrapper(9);
			}
		}

//Load the lyric strings
	if(Lyrics.verbose)	puts("Loading text chunk entries");

	while(1)	//Read lyric lines
	{
	//Check to see if the Sync Chunk has been reached
		if(ftell_err(inf) >= VL.textsize + 16)	//The Text chunk size + the file and text header sizes is the position of the Sync chunk
			break;

	//Read lyric string
		temp=ReadUnicodeString(inf);

		ftell_result=ftell_err(inf);	//If this string is empty
		if(temp[0] == '\0')
		{
//v2.3	Moved this call to ftell outside of the if statement
//			ftell_result=ftell_err(inf);
			if(VL.textsize+16 - ftell_result <= 3)	//This 0 word value ends within 3 bytes of the sync header (is padding)
			{
				free(temp);	//Release string as it won't be used
				break;	//text chunk has been read
			}
			else
			{
				printf("Error: Empty lyric string detected before file position 0x%lX\n",ftell_result);
				if(validate)
					return 10;
				else
				{
					puts("Aborting");
					exit_wrapper(10);
				}
			}
		}
//v2.3	Moved this call to ftell further up
//		if(ftell_err(inf) > VL.textsize + 16)	//If reading this string caused the file position to cross into Sync chunk
		if(ftell_result > VL.textsize + 16)	//If reading this string caused the file position to cross into Sync chunk
		{
			puts("Error: Lyric string overlapped into Sync chunk");
			if(validate)
				return 11;
			else
			{
				puts("Aborting");
				exit_wrapper(11);
			}
		}

		if(Lyrics.verbose)	printf("\tLoading text piece #%lu: \"%s\"\n",VL.numlines,temp);

	//Allocate link
		ptr1=malloc_err(sizeof(struct VL_Text_entry));	//Allocate memory for new link

	//Build link and insert into list
		ptr1->text=temp;
		ptr1->next=NULL;

		if(VL.Lyrics == NULL)	//This is the first lyric piece read from the input file
			VL.Lyrics=ptr1;
		else
		{
			assert_wrapper(curtext != NULL);
			curtext->next=ptr1;	//The end of the list points forward to this new link
		}

		curtext=ptr1;	//This link is now at the end of the list

		VL.numlines++;	//one more lyric string has been parsed
	}//end while(1)

	if(Lyrics.verbose)	printf("%lu text chunk entries loaded\n",VL.numlines);

//Load the Sync points
	if(Lyrics.verbose)	puts("Loading sync chunk entries");

	fseek_err(inf,VL.textsize+16+8,SEEK_SET);	//Seek to first sync entry (8 bytes past start of sync header)

	while(ReadSyncEntry(&se,inf) == 0)	//Read all entries
	{
		if(se.start_char > se.end_char)
		{
			puts("Error: Invalid sync point offset is specified to start after end offset");
			if(validate)
				return 12;
			else
			{
				puts("Aborting");
				exit_wrapper(12);
			}
		}

		if(se.lyric_number!=0xFFFF && se.start_char!=0xFFFF && se.start_time!=0xFFFFFFFF)
		{	//This is a valid sync entry
		//Allocate link
			ptr2=malloc_err(sizeof(struct VL_Sync_entry));

		//Build link and insert into list
			memcpy(ptr2,&se,sizeof(struct VL_Sync_entry));	//copy structure into newly allocated link
			if(VL.Syncs == NULL)	//This is the first sync piece read from the input file
				VL.Syncs=ptr2;
			else					//The end of the list points forward to this new link
			{
				assert_wrapper(cursync != NULL);
				cursync->next=ptr2;
			}

			cursync=ptr2;		//This link is now at the end of the list
			VL.numsyncs++;		//one more sync entry has been parsed
		}
	}

	if(Lyrics.verbose)	puts("VL_PreLoad completed");
	return 0;
}

int ReadSyncEntry(struct VL_Sync_entry *ptr,FILE *inf)
{	//Portable function to read 16 bytes from the file. Returns nonzero upon error
	unsigned long buffer=0;
	long ftell_result;

	assert_wrapper((ptr != NULL) && (inf != NULL));	//These must not be NULL

//Determine if the next 16 bytes can be read without reaching end of file
	ftell_result=ftell_err(inf);

	if(ftell_result +16 > VL.filesize + 8)	//If the size of a sync entry would exceed the end of the file as defined in the VL header
		return 1;			//return EOF

//Read lyric number and reserved bytes
	ReadDWORDLE(inf,&buffer);			//Read 4 bytes from file (the lyric # and the 2 reserved bytes)
	ptr->lyric_number=buffer & 0xFF;	//Convert to 2 byte integer

//Read start char
	ReadWORDLE(inf,&(ptr->start_char));	//Read 2 bytes from file

//Read end char
	ReadWORDLE(inf,&(ptr->end_char));	//Read 2 bytes from file

//Read start time
	ReadDWORDLE(inf,&(ptr->start_time));	//Read 4 bytes from file

//Read end time
	ReadDWORDLE(inf,&(ptr->end_time));	//Read 4 bytes from file

	return 0;	//return success
}

void VL_Load(FILE *inf)
{
	unsigned long ctr;			//Generic counter
	unsigned long start_off;	//Starting offset of a lyric piece in milliseconds
	unsigned long end_off;		//Ending offset of a lyric piece in milliseconds
	char *temp;					//Array for string manipulation
	struct VL_Text_entry *curtext;	//Conductor for text chunk linked list
	struct VL_Sync_entry *cursync;	//Conductor for sync chunk linked list
	unsigned short cur_line_len;	//The length of the currently line of lyrics
	unsigned short start_char;	//The starting character offset for the current sync entry
	unsigned short end_char;	//The ending character offset for the current sync entry
	char groupswithnext;		//Tracks grouping, passed to AddLyricPiece()

	assert_wrapper(inf != NULL);	//This must not be NULL

	Lyrics.freestyle_on=1;		//VL is a pitch-less format, so import it as freestyle

	if(Lyrics.verbose)	printf("Importing VL lyrics from file \"%s\"\n\n",Lyrics.infilename);

//Build the VL storage structure
	VL_PreLoad(inf,0);

//Process offset
	if(Lyrics.offsetoverride == 0)
	{
		if(Lyrics.Offset == NULL)
		{
			if(Lyrics.verbose)	puts("No offset defined in VL file, applying offset of 0");

			Lyrics.realoffset=0;
		}
		else if(strcmp(Lyrics.Offset,"0") != 0)
		{	//If the VL file's offset is not zero and the command line offset is not specified
			assert_wrapper(Lyrics.Offset != NULL);	//atol() crashes if NULL is passed to it

			Lyrics.realoffset=atol(Lyrics.Offset); //convert to number
			if(Lyrics.realoffset == 0)	//atol returns 0 on error
			{
				printf("Error converting \"%s\" to integer value\nAborting\n",Lyrics.Offset);
				exit_wrapper(1);
			}

			if(Lyrics.verbose) printf("Applying offset defined in VL file: %ldms\n",Lyrics.realoffset);
		}	//if the VL file's offset is defined as 0, that's what Lyrics.realoffset is initialized to already
	}

	if(Lyrics.verbose)	puts("Processing lyrics and sync entries");

//Process sync points, adding lyrics to Lyrics structure
	cursync=VL.Syncs;	//Begin with first sync entry
	while(cursync != NULL)	//For each sync point
	{
		groupswithnext=0;	//Reset this condition
		start_off=cursync->start_time*10;	//VL stores offsets as increments of 10 milliseconds each
		end_off=cursync->end_time*10;

	//Validate the lyric line number
		if(cursync->lyric_number >= VL.numlines)	//lyric_number count starts w/ 0 instead of 1 and should never meet/exceed numlines
		{
			puts("Error: Invalid line number detected during VL load\nAborting");
			exit_wrapper(2);
		}

	//Validate the start and end character numbers in the sync entry
		start_char=cursync->start_char;
		end_char=cursync->end_char;
		if(start_char == 0xFFFF)
		{
			puts("Error: Sync entry has no valid lyric during VL load\nAborting");
			exit_wrapper(3);
		}

	//Seek to the correct lyric entry
		curtext=VL.Lyrics;	//Point conductor to first text entry
		for(ctr=0;ctr<cursync->lyric_number;ctr++)
			if(curtext->next == NULL)
			{
				puts("Error: Unexpected end of text piece linked list\nAborting");
				exit_wrapper(4);
			}
			else
				curtext=curtext->next;	//Seek forward to next piece

		cur_line_len = (unsigned short) strlen(curtext->text);	//This value will be used several times in the rest of the loop
		if(start_char >= cur_line_len)				//The starting offset cannot be past the end of the line of lyrics
		{
			puts("Error: Sync entry has invalid starting offset during VL load\nAborting");
			exit_wrapper(5);
		}
		if((end_char!=0xFFFF) && (end_char >= cur_line_len))
		{	//The ending offset cannot be past the end of the line of lyrics
			puts("Error: Sync entry has invalid ending offset during VL load\nAborting");
			exit_wrapper(6);
		}

	//Build the lyric based on the start and end character offsets
		temp=DuplicateString(curtext->text+start_char);
			//Copy the current text piece into a new string, starting from the character indexed by the sync entry

		if(Lyrics.verbose>=2)	printf("\tProcessing sync entry #%lu: \"%s\"\tstart char=%u\tend char=%u\t\n",ctr,curtext->text,start_char,end_char);

		if(end_char != 0xFFFF)	//If the sync entry ends before the end of the text piece
		{	//"abcdef"	strlen=6	st=2,en=4->"cde"
			if((isspace((unsigned char)temp[end_char-start_char-1])==0) && (isspace((unsigned char)temp[end_char-start_char])==0))
				//if this sync entry's text doesn't end in whitespace and the next entry's doesn't begin in whitespace
				groupswithnext=1;	//Allow AddLyricPiece to handle grouping and optional hyphen insertion

//I've had to go back and forth on this line, but currently, end_char-start_char seems to mark the location at which to truncate, not the last character to keep before truncating
			temp[end_char-start_char]='\0';	//Truncate the string as indicated by the sync entry's end index (after the end_char)
		}

	//Add lyric to Lyric structure
		if(cursync->start_char == 0)	//If this piece is the beginning of a line of lyrics
		{
		//Ensure a line of lyrics isn't already in progress
			if(Lyrics.line_on == 1)
			{
				puts("Error: Lyric lines overlap during VL load\nAborting");
				exit_wrapper(7);
			}

			if(Lyrics.verbose>=2)	puts("New line of lyrics:");

			CreateLyricLine();	//Initialize the line
		}

//Add lyric to Lyrics structure.
		AddLyricPiece(temp,start_off,end_off,PITCHLESS,groupswithnext);	//Add the lyric piece to the Lyric structure, no defined pitch
		free(temp);	//Release memory for this temporary string

		if((end_char == 0xFFFF) || (end_char == cur_line_len))	//If this piece ends a line of lyrics
		{
		//Ensure a line of lyrics is in progress
			if(Lyrics.line_on == 0)
			{
				puts("Error: End of lyric line detected when none is started during VL load\nAborting");
				exit_wrapper(8);
			}

			if(Lyrics.verbose>=2)	puts("End line of lyrics");

			EndLyricLine();		//End the line
		}

		cursync=cursync->next;
	}//end while(cursync != NULL)

	ForceEndLyricLine();

	if(Lyrics.verbose)	printf("VL import complete.  %lu lyrics loaded\n",Lyrics.piececount);

	ReleaseVL();	//Release memory used to build the VL structure
}

void ExportVL(FILE *outf)
{
	struct _VLSTRUCT_ *OutVL;	//The prepared VL structure to write to file
	struct VL_Text_entry *curtext;	//Conductor for the text chunk list
	struct VL_Text_entry *textnext;	//Used for OutVL deallocation
	struct VL_Sync_entry *cursync;	//Conductor for the sync chunk list
	struct VL_Sync_entry *syncnext;	//Used for OutVL deallocation
	unsigned long ctr;
	long filepos;

	assert_wrapper(outf != NULL);	//This must not be NULL

	if(Lyrics.verbose)	printf("\nExporting VL lyrics to file \"%s\"\n\n",Lyrics.outfilename);

//Write VL header: 'V','L','2',0
	fputc_err('V',outf);
	fputc_err('L',outf);
	fputc_err('2',outf);
	fputc_err(0,outf);

//Pad the next four bytes, which are supposed to hold the size of the file, minus the size of the VL header (8 bytes)
	WriteDWORDLE(outf,0);

	if(Lyrics.verbose>=2)	puts("Writing text chunk");

//Write Text chunk header: 'T','E','X','T'
	fputc_err('T',outf);
	fputc_err('E',outf);
	fputc_err('X',outf);
	fputc_err('T',outf);

//Pad the next four bytes, which are supposed to hold the size of the header chunk, minus the size of the chunk header (8 bytes)
	WriteDWORDLE(outf,0);

//Convert the Offset string to centiseconds from milliseconds (as this is how the VL format interprets it)
	if((Lyrics.Offset != NULL))	//If there is an offset
	{
		if(strlen(Lyrics.Offset) < 2)	//If the offset is 0 or 1 characters long, it will be lost, free the string
		{
			free(Lyrics.Offset);
			Lyrics.Offset=NULL;
		}
		else
			Lyrics.Offset[strlen(Lyrics.Offset)-1]='\0';	//Truncate last digit to effectively divide by ten
	}

//Write 8 Unicode strings, for Title, Artist, Album, Editor, Offset and 3 empty strings
	WriteUnicodeString(outf,Lyrics.Title);
	WriteUnicodeString(outf,Lyrics.Artist);
	WriteUnicodeString(outf,Lyrics.Album);
	WriteUnicodeString(outf,Lyrics.Editor);
	WriteUnicodeString(outf,NULL);	//The Offset tag should be 0 because all timestamps are written as being absolute
	WriteUnicodeString(outf,NULL);
	WriteUnicodeString(outf,NULL);
	WriteUnicodeString(outf,NULL);

//Create sync and lyric structures
	OutVL=VL_PreWrite();

//Write lyric strings
	curtext=OutVL->Lyrics;

	if(Lyrics.verbose)	puts("Writing VL file");

	while(curtext != NULL)	//For each text chunk
	{
		assert_wrapper(curtext->text != NULL);

		if(Lyrics.verbose)	printf("\tText chunk entry written: \"%s\"\n",curtext->text);

		WriteUnicodeString(outf,curtext->text);
		curtext=curtext->next;					//Point to next text chunk entry
	}

//Write padding if necessary
	ctr=ftell_err(outf)%4;	//ctr=# of padding bytes needed to align output file position to a DWORD boundary
	while(ctr--)	//For each byte of padding necessary
		fputc_err(0,outf);

//Go back and write the text chunk size after the text chunk header
	filepos=ftell_err(outf);		//Store file position
	OutVL->textsize=filepos-16;		//File position - VL and Text header sizes = text chunk size
	fseek_err(outf,12,SEEK_SET);	//File position 12 is where the text chunk size is to be stored
	WriteDWORDLE(outf,OutVL->textsize);	//Write text chunk size to file
	fseek_err(outf,filepos,SEEK_SET);		//Seek back to the end of the text chunk (where the Sync chunk will begin)

	if(Lyrics.verbose>=2)	puts("Writing Sync chunk");

//Write Sync chunk header: 'S','Y','N','C'
	fputc_err('S',outf);
	fputc_err('Y',outf);
	fputc_err('N',outf);
	fputc_err('C',outf);

//Pad the next four bytes, which are supposed to hold the size of the sync chunk, minus the size of the chunk header (8 bytes)
	WriteDWORDLE(outf,0);

//Write sync entries
	cursync=OutVL->Syncs;
	while(cursync != NULL)
	{
		WriteWORDLE(outf,cursync->lyric_number);//Write lyric line number this sync entry pertains to
		WriteWORDLE(outf,0);					//Write reserved Word value
		WriteWORDLE(outf,cursync->start_char);	//Write start offset
		WriteWORDLE(outf,cursync->end_char);	//Write end offset
		WriteDWORDLE(outf,cursync->start_time);	//Write start time
		WriteDWORDLE(outf,cursync->end_time);	//Write end time
		cursync=cursync->next;
	}

//Go back and write the sync chunk size after the sync chunk header
	filepos=ftell_err(outf);						//Store file position
	OutVL->syncsize=filepos - OutVL->textsize - 24;	//File position - Text chunk size - 3 header sizes = sync chunk size
	fseek_err(outf,OutVL->textsize+20,SEEK_SET);	//File position for sync chunk size is text chunk size +2.5 headers
	WriteDWORDLE(outf,OutVL->syncsize);				//Write sync chunk size to file

//Go back and write the VL file size after the VL header, which completes the VL export
	filepos-=8;							//Subtract the VL header size from the total file size
	fseek_err(outf,4,SEEK_SET);			//File posistion for VL file size is after the "VL2\0" string
	WriteDWORDLE(outf,filepos);			//Write the VL file size - header value

//Destroy the OutVL structure
	if(Lyrics.verbose>=2)	puts("\tReleasing memory from export VL structure");
	curtext=OutVL->Lyrics;	//Point conductor at first text chunk entry
	while(curtext != NULL)	//For each text chunk entry
	{
		textnext=curtext->next;
		if(curtext->text != NULL)
			free(curtext->text);	//Free string
		free(curtext);				//Free text chunk structure
		curtext=textnext;
	}

	cursync=OutVL->Syncs;	//Point conductor at first sync chunk entry
	while(cursync != NULL)	//For each sync chunk entry
	{
		syncnext=cursync->next;
		free(cursync);		//Free sync chunk structure
		cursync=syncnext;
	}

	free(OutVL);
	if(Lyrics.verbose)	printf("\nVL export complete.  %lu lyrics written\n",Lyrics.piececount);
}

struct _VLSTRUCT_ *VL_PreWrite(void)
{
	struct _VLSTRUCT_ *OutVL;			//Create a VL structure to build the exported VL format
	char *lyrline=NULL;						//A pointer to an array large enough to store the largest line of lyrics
	char *temp=NULL;
	unsigned long maxlength;			//The calculated length of the longest lyric line (excluding null terminator)
	unsigned long charcount;			//The running sum of the length of all lyric pieces in a line
	unsigned long index;				//The current index into the lyric line, stored with each sync entry
	unsigned long linenum;				//The number of the currently-processed lyric line
	struct Lyric_Piece *curpiece=NULL;	//Conductor for lyric piece linked list
	struct Lyric_Line *curline=NULL;	//Conductor for lyric line linked list
	struct VL_Text_entry *curtext=NULL;	//Text entry for current lyric piece
	struct VL_Sync_entry *cursync=NULL;	//Sync entry for current lyric piece
	struct VL_Sync_entry *newsync=NULL;	//Used to build new sync entries to insert into list
	struct VL_Text_entry *newtext=NULL;	//Used to build new text entries to insert into list
	unsigned long lastendtime;			//The end time for the last lyric piece.  The next lyric's timestamp must be at least 1 more than this

	if(Lyrics.verbose)	puts("Building VL structure");

//Allocate the ExportVL structure
	OutVL=calloc_err(1,sizeof(struct _VLSTRUCT_));	//Allocate and init to 0
	cursync=NULL;
	curtext=NULL;

//Initialize the ExportVL structure
	OutVL->numlines=Lyrics.linecount;
	OutVL->numsyncs=Lyrics.piececount;

//Find the length of the longest line of lyrics
	maxlength=0;
	curline=Lyrics.lines;		//Conductor points to first line of lyrics
	while(curline != NULL)	//For each line of lyrics
	{
		charcount=0;	//reset count
		curpiece=curline->pieces;	//Conductor points to first lyric in the line
		while(curpiece != NULL)		//For each lyric piece in the line
		{
			if(curpiece->lyric != NULL)
				charcount+=strlen(curpiece->lyric);	//Add the length of the lyric

			curpiece=curpiece->next;	//Point to next lyric piece
		}

		if(charcount>maxlength)		//If this line has more characters than any other so far
			maxlength=charcount;	//store the number of characters

		curline=curline->next;		//Point to next lyric line
	}

//Initialize lyrline
	lyrline=malloc_err(maxlength+1+Lyrics.piececount);	//Add piececount to allow for spacing between each word

//Process lyric pieces
	curline=Lyrics.lines;	//Conductor points to first line of lyrics
	lastendtime=0;
	linenum=0;	//This will be used to index into the lyric array
	cursync=OutVL->Syncs;	//Point conductor to first link in sync chunk list

	while(curline != NULL)	//For each line of lyrics
	{
		lyrline[0]='\0';	//Empty the string buffer
		index=0;			//Reset index to 0
		curpiece=curline->pieces;	//Conductor points to first lyric in the line

		while(curpiece != NULL)	//For each lyric piece in this line
		{
			if(curpiece->lyric != NULL)	//Only process if there is actual lyric text for this lyric piece
			{
			//Allocate a new sync list link
				newsync=malloc_err(sizeof(struct VL_Sync_entry));

			//Build sync entry
				newsync->lyric_number=linenum;	//Initialize lyric number, which should be the lyric line number it refers to
				newsync->start_char=index;		//Initialize sync entry start index

				if(curpiece->next != NULL)	//If there's another lyric piece in this line
					newsync->end_char=index+strlen(curpiece->lyric)-1;	//Find ending offset of this sync entry
				else
					newsync->end_char=0xFFFF;	//The lyric reaches the end of the line of lyrics

				newsync->start_time=curpiece->start/10;	//VL stores timestamps in 10ms increments
				newsync->end_time=(curpiece->start+curpiece->duration)/10;

				if(newsync->end_time <= newsync->start_time)	//If the duration of the lyric was less than 10ms (became 0 during the division)
					newsync->end_time+=1;						//add the minimum duration of 10ms

				if((lastendtime != 0) && (newsync->start_time < lastendtime))	//Ensure that this doesn't overlap with last piece
					newsync->start_time=lastendtime+1;

				lastendtime=newsync->end_time;	//Store this to prevent overlapping with next piece
				index=newsync->end_char+1;	//Set lyric character index one higher than the end of this sync entry
				newsync->next=NULL;			//This will be the last link in the list

			//Insert sync entry into linked list
				if(OutVL->Syncs == NULL)		//This is the first sync entry in the list
					OutVL->Syncs=newsync;
				else						//Last link points forward to this link
					cursync->next=newsync;

				cursync=newsync;			//This becomes the new last link in the list

			//Append lyric piece to string and append a space if necessary
				strcat(lyrline,curpiece->lyric);
				if((curpiece->next != NULL) && (curpiece->groupswithnext == 0))
				{	//There is another lyric piece in this line and this piece does not group with it
					strcat(lyrline," ");
					index++;				//Increment the index to preserve subsequent sync piece placement
				}
			}

			curpiece=curpiece->next;	//Point to next lyric piece
		}//end while(curpiece != NULL)

	//Make a permanent copy of the combined lyric string and store it in the lyric array
		temp=DuplicateString(lyrline);	//Copy completed lyric line into new string
		linenum++;							//Iterate lyric line number
	//Allocate new text link
		newtext=malloc_err(sizeof(struct VL_Text_entry));

	//Build new link and insert into list
		newtext->text=temp;
		newtext->next=NULL;
		if(OutVL->Lyrics == NULL)	//This is the first text chunk entry
			OutVL->Lyrics=newtext;
		else							//Last link points forward to this link
		{
			assert_wrapper(curtext != NULL);
			curtext->next=newtext;
		}
		curtext=newtext;				//This becomes the last link in the list

		curline=curline->next;			//Point to next lyric line

		if(Lyrics.verbose>=2)	printf("\tStored text chunk entry \"%s\"\n",temp);
	}//end while(curline != NULL)

	free(lyrline);	//No longer needed
	return OutVL;	//Return completed structure
}

void ReleaseVL(void)
{
	struct VL_Text_entry *texttemp;
	struct VL_Text_entry *textnext;
	struct VL_Sync_entry *synctemp;
	struct VL_Sync_entry *syncnext;

	if(Lyrics.verbose)	puts("Cleaning up VL structure");

//Release VL structure's memory
	texttemp=VL.Lyrics;		//Point conductor at first text chunk entry
	while(texttemp != NULL)	//For each text chunk entry
	{
		textnext=texttemp->next;
		if(texttemp->text != NULL)
			free(texttemp->text);	//Free string
		free(texttemp);				//Free text chunk structure
		texttemp=textnext;
	}

	synctemp=VL.Syncs;		//Point conductor at first sync chunk entry
	while(synctemp != NULL)	//For each sync chunk entry
	{
		syncnext=synctemp->next;
		free(synctemp);		//Free sync chunk structure
		synctemp=syncnext;
	}

	VL.numlines=VL.numsyncs=VL.filesize=VL.textsize=VL.syncsize=0;
}
