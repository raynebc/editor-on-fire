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
	if(Lyrics.rocksmithver < 3)
	{	//The normal RS1 and RS2 lyrics are in UTF-8
		fputs_err("<?xml version='1.0' encoding='UTF-8'?>\n",outf);
	}
	else
	{	//The extended ASCII variants are in windows-1252
		fputs_err("<?xml version='1.0' encoding='windows-1252'?>\n",outf);
	}
	if(fprintf(outf,"<vocals count=\"%lu\">\n", Lyrics.piececount) < 0)
	{
		printf("Error writing to XML file: %s\nAborting\n",strerror(errno));
		exit_wrapper(2);
	}
#ifdef EOF_BUILD	//In the EOF code base, put a comment line indicating the program version
	fputs_err("<!-- " EOF_VERSION_STRING " -->\n", outf);	//Write EOF's version in an XML comment
#endif

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
				expand_xml_text(buffer2, sizeof(buffer2) - 1, temp->lyric, 48, 2, 1, 0, NULL);	//Expand XML special characters into escaped sequences if necessary, and check against the maximum supported length of this field.  Allow characters that are supported in lyrics but not other parts of the RS XML.
			}
			else if(Lyrics.rocksmithver == 3)
			{	//If Rocksmith 2014 format is being exported, and compatible extended ASCII characters are allowed
				expand_xml_text(buffer2, sizeof(buffer2) - 1, temp->lyric, 48, 3, 1, 0, NULL);	//Expand XML special characters into escaped sequences if necessary, and check against the maximum supported length of this field.  Allow characters that are supported in lyrics but not other parts of the RS XML.
			}
			else if (Lyrics.rocksmithver == 4)
			{	//A Rocksmith 2014 style format that doesn't force lyric content to use compliant characters
				expand_xml_text(buffer2, sizeof(buffer2) - 1, temp->lyric, 48, 4, 1, 0, NULL);
			}
			else
			{	//Otherwise the lyric limit is 32 characters
				expand_xml_text(buffer2, sizeof(buffer2) - 1, temp->lyric, 32, 2, 1, 0, NULL);	//Expand XML special characters into escaped sequences if necessary, and check against the maximum supported length of this field.  Allow characters that are supported in lyrics but not other parts of the RS XML.
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

int rs_filter_char(int character, char rs_filter, int islyric, int isphrase_section, int ischordname)
{
	if((rs_filter > 1) && (character == '/'))
		return 1;
	if(ischordname && ((character == '(') || (character == ')')))
		return 0;
	if(!islyric)
	{	//XML strings don't allow the following characters except for lyric text
		if((character == '(') || (character == '}') || (character == ',') || (character == '\\') || (character == ':') || (character == '{') || (character == '"') || (character == ')'))
			return 1;
	}
	if(isphrase_section)
	{	//RS phrases/sections shouldn't have non alphanumeric characters
		if(!isalnum(character))
			return 1;
	}
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

int rs_lyric_substitute_char_extended(int character, int function)
{
	unsigned int code = (unsigned int)character;

	//Substitute for characters that Rocksmith doesn't allow for lyrics
	if(code == 142)
	{	//Capital Z with caron
		code = 'Z';
	}
	else if(code == 159)
	{	//Capital Y with umlaut
		code = 'Y';
	}
	else if(code == 255)
	{	//Lowercase y with umlaut
		code = 'y';
	}
	else if(code == 195)
	{	//Capital A with tilde
		code = 'A';
	}
	else if(code == 227)
	{	//Lowercase a with tilde
		code = 'a';
	}
	else if(code == 205)
	{	//Capital I with acute
		code = 'I';
	}
	else if(code == 213)
	{	//Capital O with tilde
		code = 'O';
	}
	else if(code == 245)
	{	//Lowercase o with tilde
		code = 'o';
	}
	else if(code == 221)
	{	//Capital Y with acute
		code = 'Y';
	}
	else if(code == 253)
	{	//Lowercase y with acute
		code = 'y';
	}

	if(function)
		return (int)code;	//If the calling function didn't opt to substitute for ALL accented Latin characters, skip the logic below

	//Substitute for characters the author hasn't allowed ("Allow RS2 extended ASCII lyrics" preference not enabled)
	if((code == 192) || (code == 193) || (code == 194) || (code == 196) || (code == 197))
	{	//Capital A with grave, acute, circumflex, umlaut or ring accent
		code = 'A';
	}
	else if((code == 224) || (code == 225) || (code == 226) || (code == 228) || (code == 229))
	{	//Lowercase a with grave, acute, circumflex, umlaut or ring accent
		code = 'a';
	}
	else if(code == 199)
	{	//Capital C with cedilla accent
		code = 'C';
	}
	else if(code == 231)
	{	//Lowercase c with cedilla accent
		code = 'c';
	}
	else if((code == 200) || (code == 201) || (code == 202) || (code == 203))
	{	//Capital E with grave, acute, circumflex or umlaut accent
		code = 'E';
	}
	else if((code == 232) || (code == 233) || (code == 234) || (code == 235))
	{	//Lowercase e with grave, acute, circumflex or umlaut accent
		code = 'e';
	}
	else if((code == 204) || (code == 206) || (code == 207))
	{	//Capital I with grave, circumflex or umlaut accent
		code = 'I';
	}
	else if((code == 236) || (code == 237) || (code == 238) || (code == 239))
	{	//Lowercase i with grave, acute, circumflex or umlaut accent
		code = 'i';
	}
	else if(code == 209)
	{	//Capital N with tilde
		code = 'N';
	}
	else if(code == 241)
	{	//Lowercase n with tilde
		code = 'n';
	}
	else if((code == 210) || (code == 211) || (code == 212) || (code == 214) || (code == 216))
	{	//Capital O with grave, acute, circumflex, umlaut or slash accent
		code = 'O';
	}
	else if((code == 242) || (code == 243) || (code == 244) || (code == 246) || (code == 248))
	{	//Lowercase o with grave, acute, circumflex, umlaut or slash accent
		code = 'o';
	}
	else if((code == 217) || (code == 218) || (code == 219) || (code == 220))
	{	//Capital U with grave, acute, circumflex or umlaut accent
		code = 'U';
	}
	else if((code == 249) || (code == 250) || (code == 251) || (code == 252))
	{	//Lowercase u with grave, acute, circumflex or umlaut accent
		code = 'u';
	}
	else if(code == 138)
	{	//Capital S with caron
		code = 'S';
	}
	else if(code == 154)
	{	//Lowercase s with caron
		code = 's';
	}
	else if(code == 158)
	{	//Lowercase z with caron
		code = 'z';
	}

	return (int)code;
}

int rs_filter_string(char *string, char rs_filter)
{
	unsigned long ctr;

	if(!string)
		return -1;

	for(ctr = 0; string[ctr] != '\0'; ctr++)
	{	//For each character in the string until the terminator is reached
		if(rs_filter_char(string[ctr], rs_filter, 0, 0, 1))	//If the character is rejected by the filter, not allowing the ASCII characters exclusively usable for lyrics
			return 1;
	}

	return 0;
}

void expand_xml_text(char *buffer, size_t size, const char *input, size_t warnsize, char rs_filter, int islyric, int isphrase_section, char *exceptions)
{
	size_t input_length, index = 0, ctr;

	if(!buffer || !input || !size || (warnsize > size))
		return;	//Invalid parameters

	input_length = strlen(input);
	for(ctr = 0; ctr < input_length; ctr++)
	{	//For each character of the input string
		int character = (unsigned char)input[ctr];
		int exception_match = 0;

///Process exceptions
		if(exceptions)
		{	//If an exceptions string was specified
			unsigned long eindex = 0;
			for(eindex = 0; exceptions[eindex] != '\0'; eindex++)
			{	//For each character in the exceptions string
				if(exceptions[eindex] == character)
				{	//If the character being examined is in the exceptions
					exception_match = 1;
					break;
				}
			}
		}

		if(!exception_match)
		{	//If the character was not in the exceptions list, check if it should be filtered
			if(rs_filter < 3)
			{	//For normal filtering, substitute accented Latin characters for non-accented characters before checking if the character is allowed
				character = rs_lyric_substitute_char_extended(character, 0);
				if(!isprint(character))
					continue;	//If extended ASCII isn't being allowed and this isn't a printable character less than ASCII value 128, omit it
			}

			if(rs_filter)
			{
				if(rs_filter < 3)
				{	//Normal filtering (1 or 2)
					if(rs_filter_char(character, rs_filter, islyric, isphrase_section, 0))
						continue;	//If filtering out characters for Rocksmith, omit affected characters
				}
				else if(rs_filter == 3)
				{	//Filtering to allow extended ASCII
					character = rs_lyric_substitute_char_extended(character, 1);	//Substitute only unsupported accented Latin characters for non-accented versions
					if(rs_lyric_filter_char_extended(character))
						continue;	//If filtering out characters for Rocksmith lyrics, omit affected characters
				}
				else if(rs_filter == 4)
				{	//No filtering
				}
			}
		}

		if(character == '\"')
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
		else if(character == '\'')
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
		else if(character == '<')
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
		else if(character == '>')
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
		else if(character == '&')
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
			buffer[index++] = character;
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
	long timevar = 0, note = 0, length = 0;
	char lyric[256], lyric2[256];
	char *index = NULL;
	int readerrordetected = 0;
	unsigned long endlinemarkers = 0;	//Tracks the number of end of line + markers

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
				if(strstr(buffer, "<!"))
				{	//If the line contains an XML comment, skip it and read the next line of text into the buffer
					(void) fgets_err(buffer, (int)maxlinelength, inf);	//Read next line of text, so the EOF condition can be checked, don't exit on EOF
					processedctr++;
					continue;
				}

				if(strcasestr_spec(buffer, "</vocals>"))	//If the end of the vocals XML tag is reached
					break;									//Stop processing lines

				if(!parse_xml_rs_timestamp("time", buffer, &timevar))
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
				AddLyricPiece(lyric2, timevar, timevar + length, note, 0);	//Add lyric
				if(index)				//If a + character was filtered out of the lyric
				{
					endlinemarkers++;	//Count the number of these that were encountered
					EndLyricLine();		//End lyric line, as this is a line break mechanism in RS2014 formatted lyrics
				}

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
	if(endlinemarkers < 2)
		Lyrics.message = 1;	//Track that fewer than 2 line ending markers were encountered

	if(Lyrics.verbose)	printf("\nRocksmith import complete.  %lu lyrics loaded\n\n",Lyrics.piececount);
}

void RS_detect_lyric_lines(void)
{
	struct Lyric_Line *curline=NULL;		//Conductor of the lyric line linked list
	struct Lyric_Piece *temp=NULL;		//A conductor for the lyric pieces list
	unsigned long ctr, length, upper;
	char punctuation;		//Tracks whether the last lyric that was processed ended in a line ending punctuation character ('.' , '?' or '!')
	char splitcondition1;	//Is nonzero if the lyric's first character is an upper case letter and all remaining characters (if any) are lowercase, and the lyric isn't just the letter 'i' or begin with 'i' and an apostrophe.  The only exception is if a capital I is found when the previous lyric ended in a line ending punctuation character
	char splitcondition2;	//Is nonzero if the lyric begins at least 2 seconds after the end of the previous lyric in the same line
	char linessplit = 0;		//Tracks whether lyric lines were split, in which case each line's piececount and duration should be corrected to pass future validation logic

	if(Lyrics.verbose)	printf("\nDetecting missing lyric end of line markers for Rocksmith lyrics");

	curline=Lyrics.lines;	//Point lyric line conductor to first line of lyrics
	while(curline != NULL)	//For each line of lyrics
	{
		punctuation = 0;			//Reset this boolean
		temp=curline->pieces;	//Starting with the first piece of lyric in this line
		while(temp != NULL)				//For each piece of lyric in this line
		{
			if(temp->prev != NULL)
			{	//If this isn't the first lyric in the line
				//Test condition 1:  The first and only the first character in a lyric being a capital letter (excluding the letter i and contractions beginning with the letter i, unless the previous lyric ended in a punctuation character that ends a sentence)
				splitcondition1 = splitcondition2 = 0;		//Reset these booleans
				length = strlen(temp->lyric);

				if(length)
				{	//The lyric is at least 1 character long
					if(isupper(temp->lyric[0]))
					{	//The first character is an upper case letter
						if(length > 1)
						{	//If there is at least one more character
							if((temp->lyric[0] == 'I') && (temp->lyric[1] == '\''))
							{	//If the lyric is the letter i in upper case followed by an apostrophe
								if(!punctuation)
								{	//If the previous lyric didn't end in punctuation marking the end of a sentence
									splitcondition1 = 0;		//This isn't evidence of the first lyric in a line because in this case I is always supposed to be capitalized
								}
							}
							else
							{
								upper = 0;	//Reset this to track whether any of the remaining characters were upper case letters
								for(ctr = 1; ctr < length; ctr++)
								{	//For the remaining characters in the lyric
									if(isupper(temp->lyric[ctr]))
									{
										upper = 1;
										break;
									}
								}
								if(!upper)
								{	//If none of the remainer of the lyric was upper case letters
									splitcondition1 = 1;	//This lyric may be considered as the first lyric in a line
								}
							}
						}
						else if((tolower(temp->lyric[0]) != 'i') || punctuation)
						{	//If a single character lyric is upper case and isn't just the letter i, with the exception of capital i when the previous lyric ended in a punctuation character that ends a sentence
							splitcondition1 = 1;	//This lyric may be considered as the first lyric in a line
						}
					}
					if((temp->lyric[length - 1] == '.') || (temp->lyric[length - 1] == '?') || (temp->lyric[length - 1] == '!'))
					{	//If this lyric ends in punctuation that would end a sentence
						punctuation = 1;
					}
					else
						punctuation = 0;
				}
				else
				{
					punctuation = 0;
				}

				//Test condition 2:  The lyric begins at least 2 seconds after the end of the previous lyric
				if(temp->start >= temp->prev->start + temp->prev->duration + 2000)
				{
					splitcondition2 = 1;	//This lyric may be considered as the first lyric in a line
				}

				if(splitcondition1 || splitcondition2)
				{	//If either condition for splitting the current line of lyrics were met
					struct Lyric_Line *newline;

					//Terminate the current lyric line at the previous lyric
					temp->prev->next = NULL;	//The previous lyric is now the last lyric in its line
					temp->prev = NULL;		//This lyric is now the first lyric in its line

					//Create a new lyric line and initialize it to begin with the current lyric
					newline=(struct Lyric_Line *)malloc_err(sizeof(struct Lyric_Line));
					memset(newline, 0, sizeof(struct Lyric_Line));	//Init the new lyric line structure to all zeroes
					newline->pieces = temp;

					//Insert the new lyric line into the doubly linked list
					if(curline->next)
						curline->next->prev = newline;	//If there is a next line, it now points backward to the new line
					newline->next = curline->next;	//The new lyric line will point forward to any line the current line pointed forward to
					curline->next = newline;			//The current line points forward to the new line
					newline->prev = curline;			//The new line points backward to the current line

					curline = newline;	//Update conductor to point to the new line
					linessplit = 1;
				}
			}//If this isn't the first lyric in the line

			temp=temp->next;	//Advance to next lyric piece as normal
		}//end while(temp != NULL)

		curline=curline->next;	//Advance to next line of lyrics
	}

	//If any lyric lines were split, repair the variables in the lyric lines
	if(linessplit)
	{
		unsigned long linecount = 0;
		curline=Lyrics.lines;	//Point lyric line conductor to first line of lyrics
		while(curline != NULL)	//For each line of lyrics
		{
			unsigned long piececount = 0;
			linecount++;
			temp=curline->pieces;	//Starting with the first piece of lyric in this line
			while(temp != NULL)
			{	//For each piece of lyric in this line
				if(temp->prev == NULL)
				{	//If this is the first lyric in the line
					curline->start = temp->start;	//Update the line's start time
				}
				if(temp->next == NULL)
				{	//If this is the last lyric in the line
					curline->duration = temp->start + temp->duration - curline->start;	//Update the line's duration
				}
				piececount++;		//Count the number of lyrics in ths line
				temp=temp->next;	//Advance to next lyric piece as normal
			}

			curline->piececount = piececount;	//Update the line's piece count
			curline=curline->next;	//Advance to next line of lyrics
		}
		Lyrics.linecount = linecount;	//Update the Lyrics structure's line counter
	}
}
