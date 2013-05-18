#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "XML_parse.h"
#include "Lyric_storage.h"
#include "RS_parse.h"

#ifdef S_SPLINT_S
//Ensure Splint checks the code for EOF's code base
#define EOF_BUILD
#endif

#ifdef EOF_BUILD
#include "../main.h"
#endif

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

	if(!buffer || !input || !size)
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

void shrink_xml_text(char *buffer, size_t size, char *input)
{
	size_t input_length, index = 0, ctr;
	char *ptr;

	if(!buffer || !input || !size)
		return;	//Invalid parameters

	input_length = strlen(input);
	for(ctr = 0; ctr < input_length; ctr++)
	{	//For each character of the input string
		if(index + 1 >= size)
		{	//If there isn't enough buffer left to copy this character and terminate the string
			buffer[index] = '\0';	//Terminate the buffer
			return;			//And return
		}
		if(input[ctr] == '&')
		{	//If this is an escape character sequence
			ptr = &(input[ctr]);	//The address to the current position in the input buffer
			if(strstr(&(input[ctr]), "&quot;") == ptr)
			{	//If this is the quotation mark escape sequence
				buffer[index++] = '\"';
				ctr += 5;	//Seek past the sequence in the input buffer
			}
			else if(strstr(&(input[ctr]), "&apos;") == ptr)
			{	//If this is the apostrophe mark escape sequence
				buffer[index++] = '\'';
				ctr += 5;	//Seek past the sequence in the input buffer
			}
			else if(strstr(&(input[ctr]), "&lt;") == ptr)
			{	//If this is the less than escape sequence
				buffer[index++] = '<';
				ctr += 3;	//Seek past the sequence in the input buffer
			}
			else if(strstr(&(input[ctr]), "&gt;") == ptr)
			{	//If this is the greater than escape sequence
				buffer[index++] = '>';
				ctr += 5;	//Seek past the sequence in the input buffer
			}
			else if(strstr(&(input[ctr]), "&amp;") == ptr)
			{	//If this is the ampersand escape sequence
				buffer[index++] = '\"';
				ctr += 4;	//Seek past the sequence in the input buffer
			}
			else
			{
#ifdef EOF_BUILD
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tUnrecognized escape sequence in string \"%s\"", input);
				eof_log(eof_log_string, 1);
#else
				printf("\tUnrecognized escape sequence in string \"%s\"", input);
#endif
			}
		}
		else
		{	//This is a normal character
			buffer[index++] = input[ctr];	//Append it to the buffer
		}
	}
	buffer[index++] = '\0';	//Terminate the buffer
}

int parse_xml_attribute_text(char *buffer, size_t size, char *target, char *input)
{
	char *ptr;
	size_t index = 0;

	if(!buffer || !target || !input)
		return 0;	//Invalid parameters

	ptr = strcasestr_spec(input, target);	//Get the pointer to the first byte after the target attribute
	if(!ptr)
	{	//If the attribute is not present
		return 0;	//Return failure
	}

	if(size == 0)
		return 1;	//Do nothing more than the search if buffer size is 0

	while((*ptr != '\0') && isspace(*ptr))
	{	//While the end of the string hasn't been reached
		ptr++;	//Skip whitespace
	}
	if(*ptr != '=')
	{	//The next character is expected to be an equal sign
		return 0;	//If not, return failure
	}
	ptr++;
	while((*ptr != '\0') && isspace(*ptr))
	{	//While the end of the string hasn't been reached
		ptr++;	//Skip whitespace
	}
	if(*ptr != '\"')
	{	//The next character is expected to be a quotation mark
		return 0;	//If not, return failure
	}
	ptr++;

	while((*ptr != '\0') && (*ptr != '\"') && (index + 1 < size))
	{	//Until the end of the string or another quotation mark character is reached and the buffer isn't full
		buffer[index++] = *ptr;	//Copy the next character to the buffer
		ptr++;	//Iterate to next input character
	}

	buffer[index++] = '\0';	//Terminate the buffer
	return 1;	//Return success
}

int parse_xml_attribute_number(char *target, char *input, long *output)
{
	char buffer[11];

	if(!target || !input || !output)
		return 0;	//Invalid parameters

	if(!parse_xml_attribute_text(buffer, sizeof(buffer), target, input))
	{	//If the attribute's text wasn't parsed
		return 0;	//Return failure
	}

	*output = atol(buffer);
	return 1;	//Return success
}

int parse_xml_rs_timestamp(char *target, char *input, long *output)
{
	char buffer[11], buffer2[11];
	unsigned long index = 0, index2 = 0;

	if(!target || !input || !output)
		return 0;	//Invalid parameters

	if(!parse_xml_attribute_text(buffer, sizeof(buffer), target, input))
	{	//If the attribute's text wasn't parsed
		return 0;	//Return failure
	}

	while(buffer[index] != '\0')
	{	//For each character in the parsed timestamp
		if(buffer[index] != '.')
		{	//If it's not a period
			buffer2[index2++] = buffer[index];	//Copy it to the second buffer
		}
		index++;
	}
	buffer2[index2] = '\0';	//Terminate the second buffer

	*output = atol(buffer2);
	return 1;	//Return success
}

void RS_Load(FILE *inf)
{
	size_t maxlinelength;	//I will count the length of the longest line (including NULL char/newline) in the
							//input file so I can create a buffer large enough to read any line into
	char *buffer;			//Will be an array large enough to hold the largest line of text from input file
	unsigned long processedctr=0;	//The current line number being processed in the text file
	long time, note, length, count;
	char lyric[256], lyric2[256];

	assert_wrapper(inf != NULL);	//This must not be NULL

//Find the length of the longest line
	maxlinelength=FindLongestLineLength(inf,1);

//Allocate memory buffer large enough to hold any line in this file
	buffer=(char *)malloc_err(maxlinelength);

//Load each line and parse it
	(void) fgets_err(buffer, (int)maxlinelength, inf);	//Read first line of text, capping it to prevent buffer overflow

	if(Lyrics.verbose)	printf("\nImporting Rocksmith lyrics from file \"%s\"\n\n",Lyrics.infilename);

	processedctr=0;			//This will be set to 1 at the beginning of the main while loop
	while(!feof(inf))		//Until end of file is reached
	{
		processedctr++;

		if(strcasestr_spec(buffer, "<vocals"))
		{	//If this is the vocals tag
			CreateLyricLine();	//Start a line of lyrics
			if(parse_xml_attribute_number("count", buffer, &count) && count)
			{	//If the count attribute of this tag is readable and greater than 0
				(void) fgets_err(buffer, (int)maxlinelength, inf);	//Read next line of text
				processedctr++;
				while(!feof(inf))
				{	//Until there was an error reading from the file or end of file is reached
					if(strcasestr_spec(buffer, "</vocals>"))	//If the end of the vocals XML tag is reached
						break;									//Stop processing lines

					if(!parse_xml_rs_timestamp("time", buffer, &time))
					{	//If the timestamp was not readable
						printf("Error reading timestamp on line #%lu.  Aborting", processedctr);
						exit_wrapper(1);
					}
					if(!parse_xml_attribute_number("note", buffer, &note))
					{	//If the pitch was not readable
						printf("Error reading pitch on line #%lu.  Aborting", processedctr);
						exit_wrapper(2);
					}
					if(!parse_xml_rs_timestamp("length", buffer, &length))
					{	//If the length was not readable
						printf("Error reading length on line #%lu.  Aborting", processedctr);
						exit_wrapper(3);
					}
					if(!parse_xml_attribute_text(lyric, sizeof(lyric), "lyric", buffer))
					{	//If the lyric text could not be read
						printf("Error reading lyric text on line #%lu.  Aborting", processedctr);
						exit_wrapper(4);
					}
					shrink_xml_text(lyric2, sizeof(lyric2), lyric);		//Condense XML escape sequences to normal text
					if((Lyrics.last_pitch != 0) && (Lyrics.last_pitch != note))	//There's a pitch change
						Lyrics.pitch_tracking=1;
					Lyrics.last_pitch=note;	//Consider this the last defined pitch
					AddLyricPiece(lyric2, time , time + length, note, 0);	//Add lyric

					(void) fgets_err(buffer, (int)maxlinelength, inf);	//Read next line of text, so the EOF condition can be checked, don't exit on EOF
					processedctr++;
				}
			}
			EndLyricLine();	//End the line of lyrics
			break;	//Only process one vocals tag
		}

		(void) fgets(buffer, (int)maxlinelength,inf);	//Read next line of text, so the EOF condition can be checked, don't exit on EOF
		processedctr++;
	}


	ForceEndLyricLine();

	free(buffer);	//No longer needed, release the memory before exiting function

	if(Lyrics.verbose)	printf("\nRocksmith import complete.  %lu lyrics loaded\n\n",Lyrics.piececount);
}
