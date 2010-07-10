#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "Script_parse.h"
#include "Lyric_storage.h"

#ifdef USEMEMWATCH
#include <memwatch.h>
#endif


void Script_Load(FILE *inf)
{
	unsigned long maxlinelength=0;	//I will count the length of the longest line (including NULL char/newline) in the
									//input file so I can create a buffer large enough to read any line into
	char *buffer=NULL;				//Will be an array large enough to hold the largest line of text from input file
	char *substring=NULL;			//Used with strstr() to find tag strings in the input file
	unsigned long index=0;			//Used to index within a line of text
	unsigned long starttime=0;		//Converted long int value of numerical string representing a lyric's start time
	unsigned long duration=0;		//Converted long int value of numerical string representing a lyric's duration
	unsigned long processedctr=0;	//The current line number being processed in the text file
	char endlines=0;				//Boolean:  The marklines tag was read in the input file.
									//	If true, line demarcation is altered to recognize lines that
									//	begin with "#newline" as the end of a line of lyrics.  This,
									//	along with using trailing hyphens, allows for grouping logic
									//	on word or syllable synced script files

	assert_wrapper(inf != NULL);	//This must not be NULL

	Lyrics.freestyle_on=1;		//Script is a pitch-less format, so import it as freestyle

//Find the length of the longest line
	maxlinelength=FindLongestLineLength(inf,1);

//Allocate memory buffer large enough to hold any line in this file
	buffer=(char *)malloc_err(maxlinelength);

//Define beginning string for each tag
	Lyrics.TitleStringID=DuplicateString("[ti");
	Lyrics.ArtistStringID=DuplicateString("[ar");
	Lyrics.AlbumStringID=DuplicateString("[al");
	Lyrics.EditorStringID=DuplicateString("[by");
	Lyrics.OffsetStringID=DuplicateString("[offset");

	fgets_err(buffer,maxlinelength,inf);	//Read first line of text, capping it to prevent buffer overflow

	if(Lyrics.verbose)	printf("\nImporting script lyrics from file \"%s\"\n\n",Lyrics.infilename);

	processedctr=0;			//This will be set to 1 at the beginning of the main while loop
	while(!feof(inf))		//Until end of file is reached
	{
		processedctr++;
//Skip leading whitespace
		index=0;	//Reset index counter to beginning

		if((buffer[index] != '\n') && (isspace((unsigned char)buffer[index])))
			index++;	//If this character is whitespace, skip to next character

		if(buffer[index]=='#')	//If the line begins with a pound symbol
		{
			if(ParseTag(':',']',buffer,0) <= 0)	//Look for tags, content starts after ':' char and extends to following ']' char.  Do not negatize the offset
			{	//If the regular tags are not found in the line
	//Look for marklines tag
				substring=strstr(buffer,"[marklines]");
				if(substring != NULL)
					endlines=1;		//Expecting line logic
				else
				{
	//Look for newline indicator (only processed if the "# [marklines]" tag is implemented
					substring=strstr(buffer,"#newline");
					if(substring != NULL)
					{
						if(endlines)		//newline logic was detected
							EndLyricLine();	//End lyric line when this is read
						else if(Lyrics.verbose)
							puts("Marklines tag not present, ignoring newline");
					}
					else
						if(Lyrics.verbose)	printf("Ignoring unrecognized tag in line %lu: \"%s\"\n",processedctr,buffer);
				}
			}

			fgets(buffer,maxlinelength,inf);	//Read next line of text, so the EOF condition can be checked, don't exit on EOF
			continue;
		}//end if(buffer[index]=='#')

//Parse starting offset
		starttime=ParseLongInt(buffer,&index,processedctr,NULL);

//Parse duration
		duration=ParseLongInt(buffer,&index,processedctr,NULL);

//Parse the string "text"
		substring=strcasestr_spec(&(buffer[index]),"text");	//Find the character after a match for "text", if it exists
		if(substring == NULL)
		{
			printf("Error in line %lu: The \"text\" parameter is missing in the input file\nAborting\n",processedctr);
			exit_wrapper(1);
		}
		index=0;

//Find the next non whitespace after that
		while(substring[index] != '\0')
		{
			if(isspace((unsigned char)substring[index]) == 0)
				break;

			index++;
		}

//Add lyric.  At this point, substring[index] points to text string to add.  If there was nothing but whitespace after
//"text", AddLyricPiece() will receive a string of 0 characters, and it will ignore it
		//Verify that the last character of the string is a null terminator and not newline
		if(substring[strlen(substring)-1]=='\n')
			substring[strlen(substring)-1]='\0';

		if(Lyrics.line_on == 0)	//If a line isn't in progress
			CreateLyricLine();	//Initialize new line of lyrics

		AddLyricPiece(&(substring[index]),starttime,starttime+duration,PITCHLESS,0);	//Add lyric, no defined pitch

		if(endlines == 0)		//If newline recognition is not enabled
			EndLyricLine();		//End line of lyrics after each line entry in input file

		fgets(buffer,maxlinelength,inf);	//Read next line of text, so the EOF condition can be checked, don't exit on EOF
	}//end while(!feof())

	free(buffer);	//No longer needed, release the memory before exiting function

	ForceEndLyricLine();

	if(!endlines && (Lyrics.out_format!=SCRIPT_FORMAT))
		//If the input lyrics (possibly) weren't syllable synced, and the export format supports syllable sync
		puts("\aWarning: Imported Script format lyrics may not have sufficient accuracy for the specified export");

	if(Lyrics.verbose)	printf("Script import complete.  %lu lyrics loaded\n\n",Lyrics.piececount);
}

void Export_Script(FILE *outf)
{
	struct Lyric_Line *curline=NULL;	//Conductor of the lyric line linked list
	struct Lyric_Piece *temp=NULL;		//A conductor for the lyric pieces list
	int errornumber=0;

	assert_wrapper(outf != NULL);	//This must not be NULL
	assert_wrapper(Lyrics.piececount != 0);	//This function is not to be called with an empty Lyrics structure

	if(Lyrics.verbose)	printf("\nExporting script lyrics to file \"%s\"\n\nWriting tags\n",Lyrics.outfilename);

//Write the tags if they were specified in the input file
	if(Lyrics.Title != NULL)
		if(fprintf(outf,"# [ti:%s]\n",Lyrics.Title) < 0)
			errornumber=errno;

	if(Lyrics.Artist != NULL)
		if(fprintf(outf,"# [ar:%s]\n",Lyrics.Artist) < 0)
			errornumber=errno;

	if(Lyrics.Album != NULL)
		if(fprintf(outf,"# [al:%s]\n",Lyrics.Album) < 0)
			errornumber=errno;

	if(Lyrics.Editor != NULL)
		if(fprintf(outf,"# [by:%s]\n",Lyrics.Editor) < 0)
			errornumber=errno;

	if(Lyrics.marklines)
		if(fprintf(outf,"# [marklines]\n") < 0)
			errornumber=errno;

	if(errornumber != 0)
	{
		printf("Error writing tags: %s\nAborting\n",strerror(errno));
		exit_wrapper(1);
	}
	//The delay tag is not written because the timestamps have already been modified to reflect it.  This will
	//prevent them from being misinterpreted and having the timestamps delayed a second time

	if(Lyrics.verbose)	puts("Writing lyrics");

//Export the lyric pieces
	curline=Lyrics.lines;	//Point lyric line conductor to first line of lyrics
	while(curline != NULL)	//For each line of lyrics
	{
		temp=curline->pieces;	//Starting with the first piece of lyric in this line

		if(Lyrics.verbose)	printf("\tLyric line: ");

		while(temp != NULL)				//For each piece of lyric in this line
		{
			if(fprintf(outf,"%lu\t%lu\ttext\t%s\n",temp->start,temp->duration,temp->lyric) < 0)
			{
				printf("Error exporting lyric %lu\t%lu\ttext\t%s: %s\nAborting\n",temp->start,temp->duration,temp->lyric,strerror(errno));
				exit_wrapper(2);
			}

			if(Lyrics.verbose)	printf("'%s'",temp->lyric);

			temp=temp->next;	//Advance to next lyric piece as normal
		}//end while(temp != NULL)

		curline=curline->next;	//Advance to next line of lyrics

		if(Lyrics.marklines && curline)	//Only write "#newlines" tag if there's another line of lyrics
			if(Lyrics.grouping != 2)	//Only print #newline tags if line grouping isn't being used
				fputs_err("#newline\n",outf);

		if(Lyrics.verbose)	putchar('\n');
	}

	if(Lyrics.verbose)	printf("\nScript export complete.  %lu lyrics written",Lyrics.piececount);
}
