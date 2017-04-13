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
	char buffer2[260] = {0}, buffer3[260];
	char *suffix, newline[]="+", nonewline[] = "";
	unsigned index1, index2;

	assert_wrapper(outf != NULL);	//This must not be NULL
	assert_wrapper(Lyrics.piececount != 0);	//This function is not to be called with an empty Lyrics structure

	if(Lyrics.verbose)	printf("\nFoFLC exporting Rocksmith XML lyrics to file \"%s\"\n",Lyrics.outfilename);

//Write the beginning lines of the XML file
	if(Lyrics.rocksmithver != 3)
	{	//The normal RS1 and RS2 lyrics are in UTF-8
		fputs_err("<?xml version='1.0' encoding='UTF-8'?>\n",outf);
	}
	else
	{	//The extended ASCII variant is in windows-1252
		fputs_err("<?xml version='1.0' encoding='windows-1252'?>\n",outf);
	}
#ifdef EOF_BUILD	//In the EOF code base, put a comment line indicating the program version
	fputs_err("<!-- " EOF_VERSION_STRING " -->\n", outf);	//Write EOF's version in an XML comment
#endif
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
			if(Lyrics.rocksmithver == 2)
			{	//If Rocksmith 2014 format is being exported, the maximum length per lyric is 48 characters
				expand_xml_text(buffer2, sizeof(buffer2) - 1, temp->lyric, 48, 2);	//Expand XML special characters into escaped sequences if necessary, and check against the maximum supported length of this field.  Filter out characters suspected of causing the game to crash.
			}
			if(Lyrics.rocksmithver == 3)
			{	//If Rocksmith 2014 format is being exported, and the "Allow RS2 extended ASCII lyrics" preference is enabled, allow a larger range of characters
				expand_xml_text(buffer2, sizeof(buffer2) - 1, temp->lyric, 48, 3);	//Expand XML special characters into escaped sequences if necessary, and check against the maximum supported length of this field.  Filter out characters suspected of causing the game to crash.
			}
			else
			{	//Otherwise the lyric limit is 32 characters
				expand_xml_text(buffer2, sizeof(buffer2) - 1, temp->lyric, 32, 2);	//Expand XML special characters into escaped sequences if necessary, and check against the maximum supported length of this field.  Filter out characters suspected of causing the game to crash.
			}
			for(index1 = index2 = 0; (size_t)index1 < strlen(buffer2); index1++)
			{	//For each character in the expanded XML string
				if(buffer2[index1] != '+')
				{	//If it's not a plus character
					buffer3[index2] = buffer2[index1];	//Copy it to another buffer
					index2++;
				}
			}
			buffer3[index2] = '\0';	//Terminate the new buffer
			if((temp->next == NULL) && (Lyrics.rocksmithver == 2))
			{	//This is the last lyric in the line, and Rocksmith 2014 format is being exported
				suffix = newline;	//Write a + after the lyric to indicate a line break
			}
			else
				suffix = nonewline;

			if(fprintf(outf,"  <vocal time=\"%.3f\" note=\"%u\" length=\"%.3f\" lyric=\"%s%s\"/>\n", (double)temp->start / 1000.0, temp->pitch, (double)temp->duration / 1000.0, buffer3, suffix) < 0)
			{
				printf("Error exporting lyric %lu\t%lu\ttext\t%s%s: %s\nAborting\n",temp->start,temp->duration,temp->lyric,suffix,strerror(errno));
				exit_wrapper(2);
			}

			if(Lyrics.verbose)
				printf("'%s'",temp->lyric);

			temp=temp->next;	//Advance to next lyric piece as normal
		}//end while(temp != NULL)

		curline=curline->next;	//Advance to next line of lyrics
		if(Lyrics.verbose)	(void) putchar('\n');
	}

//Close the <vocals> XML tag
	fputs_err("</vocals>",outf);

	if(Lyrics.verbose)	printf("\nRocksmith XML export complete.  %lu lyrics written",Lyrics.piececount);
}

int rs_filter_char(int character, char rs_filter)
{
	if((rs_filter > 1) && (character == '/'))
		return 1;
	if((character == '(') || (character == '}') || (character == ',') || (character == '\\') || (character == ':') || (character == '{') || (character == '"') || (character == ')'))
		return 1;
	if(((unsigned) character > 127) || !isprint(character))
		return 1;

	return 0;
}

int rs_lyric_filter_char_extended(int character)
{
	unsigned int code = (unsigned int)character;

	if(code < 33)
		return 1;	//Not allowed
	if(code < 96)
		return 0;	//ASCII characters 33 through 95 are allowed
	if(code == 96)
		return 1;	//Not allowed
	if(code < 127)
		return 0;	//ASCII characters 97 through 126 are allowed
	if(code == 127)
		return 1;	//Not allowed
	if(code == 128)
		return 0;	//ASCII character 128 is allowed
	if(code < 132)
		return 1;	//Not allowed
	if(code < 134)
		return 0;	//ASCII characters 132 and 133 are allowed
	if(code < 138)
		return 1;	//Not allowed
	if(code == 138)
		return 0;	//ASCII character 138 is allowed
	if(code == 139)
		return 1;	//Not allowed
	if(code == 140)
		return 0;	//ASCII character 140 is allowed
	if(code < 145)
		return 1;	//Not allowed
	if(code < 147)
		return 0;	//ASCII characters 145 and 146 are allowed
	if(code < 153)
		return 1;	//Not allowed
	if(code < 155)
		return 0;	//ASCII characters 153 and 154 are allowed
	if(code == 155)
		return 1;	//Not allowed
	if(code == 156)
		return 0;	//ASCII character 156 is allowed
	if(code == 157)
		return 1;	//Not allowed
	if(code == 158)
		return 0;	//ASCII character 158 is allowed
	if(code < 161)
		return 1;	//Not allowed
    if(code < 163)
		return 0;	//ASCII characters 161 and 162 are allowed
	if(code < 165)
		return 1;	//Not allowed
	if(code < 169)
		return 0;	//ASCII characters 165 through 168 are allowed
	if(code == 169)
		return 1;	//Not allowed
	if(code < 172)
		return 0;	//ASCII characters 170 and 171 are allowed
	if(code < 176)
		return 1;	//Not allowed
	if(code == 176)
		return 0;	//ASCII character 176 is allowed
	if(code == 177)
		return 1;	//Not allowed
	if(code < 181)
		return 0;	//ASCII characters 178 through 180 are allowed
	if(code == 181)
		return 1;	//Not allowed
	if(code < 195)
		return 0;	//ASCII characters 182 through 194 are allowed
	if(code == 195)
		return 1;	//Not allowed
	if(code < 205)
		return 0;	//ASCII characters 196 through 204 are allowed
	if(code == 205)
		return 1;	//Not allowed
	if(code < 208)
		return 0;	//ASCII characters 206 and 207 are allowed
	if(code == 208)
		return 1;	//Not allowed
	if(code < 213)
		return 0;	//ASCII characters 209 through 212 are allowed
	if(code == 213)
		return 1;	//Not allowed
	if(code == 214)
		return 0;	//ASCII character 214 is allowed
	if(code == 215)
		return 1;	//Not allowed
	if(code < 221)
		return 0;	//ASCII characters 216 through 220 are allowed
	if(code == 221)
		return 1;	//Not allowed
	if(code < 227)
		return 0;	//ASCII characters 222 through 226 are allowed
	if(code == 227)
		return 1;	//Not allowed
	if(code < 240)
		return 0;	//ASCII characters 228 through 239 are allowed
	if(code == 240)
		return 1;	//Not allowed
	if(code < 245)
		return 0;	//ASCII characters 241 through 244 are allowed
	if(code == 245)
		return 1;	//Not allowed
	if(code == 246)
		return 0;	//ASCII character 245 is allowed
	if(code == 247)
		return 1;	//Not allowed
	if(code < 253)
		return 0;	//ASCII characters 248 through 252 are allowed

	return 1;	//Not allowed
}

int rs_filter_string(char *string, char rs_filter)
{
	unsigned long ctr;

	if(!string)
		return -1;

	for(ctr = 0; string[ctr] != '\0'; ctr++)
	{	//For each character in the string until the terminator is reached
		if(rs_filter_char(string[ctr], rs_filter))	//If the character is rejected by the filter
			return 1;
	}

	return 0;
}

void expand_xml_text(char *buffer, size_t size, const char *input, size_t warnsize, char rs_filter)
{
	size_t input_length, index = 0, ctr;

	if(!buffer || !input || !size || (warnsize > size))
		return;	//Invalid parameters

	input_length = strlen(input);
	for(ctr = 0; ctr < input_length; ctr++)
	{	//For each character of the input string
		if((rs_filter != 3) && !isprint((unsigned char)input[ctr]))
			continue;	//If extended ASCII isn't being allowed and this isn't a printable character, omit it
		if(rs_filter)
		{
			if(rs_filter < 3)
			{	//Normal filtering
				if(rs_filter_char(input[ctr], rs_filter))
					continue;	//If filtering out characters for Rocksmith, omit affected characters
			}
			else if(rs_filter == 3)
			{	//Filtering to allow extended ASCII
				unsigned char uchar = (unsigned char)input[ctr];
				if(rs_lyric_filter_char_extended(uchar))
					continue;	//If filtering out characters for Rocksmith lyrics, omit affected characters
			}
		}

		if(input[ctr] == '\"')
		{	//Expand quotation mark character
			if(index + 6 + 1 > warnsize)
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
			if(index + 6 + 1 > warnsize)
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
			if(index + 4 + 1 > warnsize)
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
			if(index + 4 + 1 > warnsize)
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
			if(index + 5 + 1 > warnsize)
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
			if(index + 1 + 1 > warnsize)
			{	//If there isn't enough buffer left to copy this character and terminate the string
				buffer[index] = '\0';	//Terminate the buffer
				return;			//And return
			}
			buffer[index++] = input[ctr];
		}
	}//For each character of the input string
	buffer[index++] = '\0';	//Terminate the string

	if(warnsize && (index > warnsize + 1))
	{	//If there is a threshold length specified for the output string, and the expanded string exceeds it (accounting for the extra byte used by the string terminator)
#ifdef EOF_BUILD
		allegro_message("Warning:  The string\n\"%s\"\nis longer than %lu characters.  It will need to be shortened or it will be truncated.", buffer, (unsigned long)warnsize);
#else
		printf("\nWarning:  The string\n\"%s\"\nis longer than %lu characters.  It will need to be shortened or it will be truncated.", buffer, (unsigned long)warnsize);
#endif
		buffer[warnsize] = '\0';	//Truncate the string
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
				ctr += 3;	//Seek past the sequence in the input buffer
			}
			else if(strstr(&(input[ctr]), "&amp;") == ptr)
			{	//If this is the ampersand escape sequence
				buffer[index++] = '&';
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
	char buffer[11] = {0}, buffer2[11], decimalfound = 0;
	unsigned long index = 0, index2 = 0;
	long wholeseconds = 0, milliseconds = 0;

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
		else
		{
			buffer2[index2] = '\0';	//Terminate the second buffer
			wholeseconds = atol(buffer2);	//Convert the string before the decimal point (whole seconds) to an integer value
			index2 = 0;	//Empty buffer2 so that it can store the fractional part of the string
			decimalfound = 1;
		}
		index++;
	}
	buffer2[index2] = '\0';	//Terminate the second buffer
	if(decimalfound)
	{	//If the decimal point was read, the second buffer is expected to have the fractional part of the timestamp
		while(index2 < 3)
		{	//Pad the second buffer with zeros until it defines to the third decimal place
			buffer2[index2++] = '0';
		}
		buffer2[index2] = '\0';	//Terminate the second buffer
		milliseconds = atol(buffer2);	//Convert the number of milliseconds to an integer value
	}
	else
	{	//Otherwise the second buffer just has the number of seconds
		wholeseconds = atol(buffer2);
	}

	*output = (wholeseconds * 1000) + milliseconds;
	return 1;	//Return success
}

void RS_Load(FILE *inf)
{
	size_t maxlinelength;	//I will count the length of the longest line (including NULL char/newline) in the
							//input file so I can create a buffer large enough to read any line into
	char *buffer;			//Will be an array large enough to hold the largest line of text from input file
	unsigned long processedctr=0;	//The current line number being processed in the text file
	long time = 0, note = 0, length = 0;
	char lyric[256], lyric2[256];
	char *index = NULL;
	int readerrordetected = 0;

	assert_wrapper(inf != NULL);	//This must not be NULL

//Find the length of the longest line
	maxlinelength=FindLongestLineLength(inf,1);

//Allocate memory buffer large enough to hold any line in this file
	buffer=(char *)malloc_err(maxlinelength);

//Load each line and parse it
	(void) fgets_err(buffer, (int)maxlinelength, inf);	//Read first line of text, capping it to prevent buffer overflow

	if(Lyrics.verbose)	printf("\nImporting Rocksmith lyrics from file \"%s\"\n\n",Lyrics.infilename);

	processedctr=0;			//This will be set to 1 at the beginning of the main while loop
	while(!feof(inf) && !readerrordetected)		//Until end of file is reached or fgets() returns an I/O error
	{
		processedctr++;

		if(strcasestr_spec(buffer, "<vocals"))
		{	//If this is the vocals tag
			CreateLyricLine();	//Start a line of lyrics

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
				if(Lyrics.line_on == 0)	//If a line isn't in progress (ie. there was a line break after the previous lyric)
					CreateLyricLine();	//Initialize new line of lyrics
				index = strchr(lyric2, '+');	//Get a pointer to the first plus character in this lyric, if any
				if(index)
				{	//If this lyric contained a + character
					while(index[0] != '\0')
					{	//Until the end of the string is reached
						index[0] = index[1];	//Shift all remaining characters in the lyric back one, overwriting the +
						index++;
					}
				}
				AddLyricPiece(lyric2, time, time + length, note, 0);	//Add lyric
				if(index)				//If a + character was filtered out of the lyric
					EndLyricLine();		//End lyric line, as this is a line break mechanism in RS2014 formatted lyrics

				(void) fgets_err(buffer, (int)maxlinelength, inf);	//Read next line of text, so the EOF condition can be checked, don't exit on EOF
				processedctr++;
			}

			EndLyricLine();	//End the line of lyrics
			break;	//Only process one vocals tag
		}

		if(fgets(buffer, (int)maxlinelength,inf) == NULL)	//Read next line of text, so the EOF condition can be checked, don't exit on EOF
			readerrordetected = 1;
		processedctr++;
	}//while(!feof(inf) && !readerrordetected)


	ForceEndLyricLine();

	free(buffer);	//No longer needed, release the memory before exiting function

	if(Lyrics.verbose)	printf("\nRocksmith import complete.  %lu lyrics loaded\n\n",Lyrics.piececount);
}
