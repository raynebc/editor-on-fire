#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "XML_parse.h"
#include "Lyric_storage.h"
#include "RS_parse.h"

#ifdef USEMEMWATCH
#ifdef EOF_BUILD	//In the EOF code base, memwatch.h is at the root
#include <allegro.h>
#include "../memwatch.h"
#else
#include "memwatch.h"
#endif
#endif

void Export_RS(FILE *outf)
{
	struct Lyric_Line *curline=NULL;	//Conductor of the lyric line linked list
	struct Lyric_Piece *temp=NULL;		//A conductor for the lyric pieces list
	char buffer2[260];

	assert_wrapper(outf != NULL);	//This must not be NULL
	assert_wrapper(Lyrics.piececount != 0);	//This function is not to be called with an empty Lyrics structure

	if(Lyrics.verbose)	printf("\nExporting Rocksmith XML lyrics to file \"%s\"\n",Lyrics.outfilename);

//Write the beginning lines of the XML file
	fputs_err("<?xml version='1.0' encoding='UTF-8'?>\n",outf);
	if(fprintf(outf,"<vocals count=\"%lu\">\n", Lyrics.piececount) < 0)
	{
		printf("Error writing to XML file: %s\nAborting\n",strerror(errno));
		exit_wrapper(2);
	}

//Export the lyric pieces
	if(Lyrics.verbose)	(void) puts("Writing lyrics");
	curline=Lyrics.lines;	//Point lyric line conductor to first line of lyrics
	while(curline != NULL)	//For each line of lyrics
	{
		temp=curline->pieces;	//Starting with the first piece of lyric in this line

		if(Lyrics.verbose)	printf("\tLyric line: ");

		while(temp != NULL)				//For each piece of lyric in this line
		{
			expand_xml_text(buffer2, sizeof(buffer2) - 1, temp->lyric, 32);	//Expand XML special characters into escaped sequences if necessary, and check against the maximum supported length of this field
			if(fprintf(outf,"  <vocal time=\"%.3f\" note=\"%u\" length=\"%.3f\" lyric=\"%s\"/>\n", (double)temp->start / 1000.0, temp->pitch, (double)temp->duration / 1000.0, buffer2) < 0)
			{
				printf("Error exporting lyric %lu\t%lu\ttext\t%s: %s\nAborting\n",temp->start,temp->duration,temp->lyric,strerror(errno));
				exit_wrapper(2);
			}

			if(Lyrics.verbose)	printf("'%s'",temp->lyric);

			temp=temp->next;	//Advance to next lyric piece as normal
		}//end while(temp != NULL)

		curline=curline->next;	//Advance to next line of lyrics
		if(Lyrics.verbose)	(void) putchar('\n');
	}

//Close the <vocals> XML tag
	fputs_err("</vocals>",outf);

	if(Lyrics.verbose)	printf("\nRocksmith XML export complete.  %lu lyrics written",Lyrics.piececount);
}

void expand_xml_text(char *buffer, size_t size, const char *input, size_t warnsize)
{
	size_t input_length, index = 0, ctr;

	if(!buffer || !input)
		return;	//Invalid parameters

	input_length = strlen(input);
	for(ctr = 0; ctr < input_length; ctr++)
	{	//For each character of the input string
		if(input[ctr] == '\"')
		{	//Expand quotation mark character
			if(index + 6 + 1 >= size)
			{	//If there isn't enough buffer left to expand this character and terminate the string
				buffer[index] = '\0';	//Terminate the buffer
				return;			//And return
			}
			buffer[index++] = '&';
			buffer[index++] = 'q';
			buffer[index++] = 'u';
			buffer[index++] = 'o';
			buffer[index++] = 't';
			buffer[index++] = ';';
		}
		else if(input[ctr] == '\'')
		{	//Expand apostrophe
			if(index + 6 + 1 >= size)
			{	//If there isn't enough buffer left to expand this character and terminate the string
				buffer[index] = '\0';	//Terminate the buffer
				return;			//And return
			}
			buffer[index++] = '&';
			buffer[index++] = 'a';
			buffer[index++] = 'p';
			buffer[index++] = 'o';
			buffer[index++] = 's';
			buffer[index++] = ';';
		}
		else if(input[ctr] == '<')
		{	//Expand less than sign
			if(index + 4 + 1 >= size)
			{	//If there isn't enough buffer left to expand this character and terminate the string
				buffer[index] = '\0';	//Terminate the buffer
				return;			//And return
			}
			buffer[index++] = '&';
			buffer[index++] = 'l';
			buffer[index++] = 't';
			buffer[index++] = ';';
		}
		else if(input[ctr] == '>')
		{	//Expand greater than sign
			if(index + 4 + 1 >= size)
			{	//If there isn't enough buffer left to expand this character and terminate the string
				buffer[index] = '\0';	//Terminate the buffer
				return;			//And return
			}
			buffer[index++] = '&';
			buffer[index++] = 'g';
			buffer[index++] = 't';
			buffer[index++] = ';';
		}
		else if(input[ctr] == '&')
		{	//Expand ampersand
			if(index + 5 + 1 >= size)
			{	//If there isn't enough buffer left to expand this character and terminate the string
				buffer[index] = '\0';	//Terminate the buffer
				return;			//And return
			}
			buffer[index++] = '&';
			buffer[index++] = 'a';
			buffer[index++] = 'm';
			buffer[index++] = 'p';
			buffer[index++] = ';';
		}
		else
		{	//Transfer character as-is
			if(index + 1 + 1 >= size)
			{	//If there isn't enough buffer left to copy this character and terminate the string
				buffer[index] = '\0';	//Terminate the buffer
				return;			//And return
			}
			buffer[index++] = input[ctr];
		}
	}//For each character of the input string
	buffer[index++] = '\0';	//Terminate the string

	if(index > warnsize + 1)
	{	//If the expanded string is above the given threshold (accounting for the extra byte used by the string terminator)
#ifdef EOF_BUILD
		allegro_message("Warning:  The string\n\"%s\"\nis longer than %lu characters.  It will need to be shortened or it will be truncated.", buffer, (unsigned long)warnsize);
#else
		printf("\nWarning:  The string\n\"%s\"\nis longer than %lu characters.  It will need to be shortened or it will be truncated.", buffer, (unsigned long)warnsize);
#endif
	}
}
