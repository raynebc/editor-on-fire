#ifndef _rs_parse_h_
#define _rs_parse_h_

void Export_RS(FILE *outf);
	//Exports the loaded lyric structure to output file in Rocksmith XML format
void RS_Load(FILE *inf);
	//Perform all code necessary to load a Rocksmith format lyric file

int rs_filter_char(int character, char rs_filter, int islyric);
	//Returns nonzero if the character isn't an ASCII character (ie. greater than 127) or otherwise isn't a printable character
	//If islyric is zero, returns nonzero if character is any of the following characters:  ( } ,  \  : { " )
	//If rs_filter is greater than 1, the forward slash character is also not allowed
	//These characters can cause Rocksmith to crash if they are present in various free-text fields like chord names, lyric text or phrase names
	//Zero is returned if the character passed is not any of the offending characters
int rs_lyric_filter_char_extended(int character);
	//Similar to rs_filter_char(), but returns 0 for all ASCII and extended ASCII characters that have been found to work in Rocksmith 2014
	//Nonzero is returned for all characters found to not display or otherwise not work
int rs_lyric_substitute_char_extended(int character, int function);
	//Some extended ASCII characters aren't supported by Rocksmith for lyrics, and by default EOF doesn't export extended ASCII to RS lyrics
	//In the case of latin characters with accent marks, they can be easily substituted for the non-accented version of the character as applicable
	//The return value reflects the character chosen for substitution, or reflects the input value if no substitution was made
	//The input and output characters are Win-1252 encoding (extended ASCII characters encoded as a single byte each)
	//If function is 0, all accented ASCII characters have substitutions made
	//If function is 1, only the unsupported characters (ie. 'A' with a tilde accent) have substitutions made (ie. 'A')
int rs_filter_string(char *string, char rs_filter);
	//Returns 1 if any character in the provided string is considered a filtered character by rs_filter_char()
	//Currently only used to validate chord names for Rocksmith export, so rs_filter_char() is invoked with a zero value for islyric
	//Returns -1 on error
void expand_xml_text(char *buffer, size_t size, const char *input, size_t warnsize, char rs_filter, int islyric);
	//Copies the input string into the specified buffer of the given size.  Any of the characters that XML requires to be escaped
	//are converted into the appropriate character sequence (ie. ' becomes &apos;).  If the expanded string's length is longer
	//than the given warning value, the user is given a warning message that the string will need to be shortened and the string
	//is truncated to be warnsize number of characters.  If warnsize is zero, no check or truncation is performed.
	//If size is zero, the function returns without doing anything.  Otherwise the buffer is guaranteed to be NULL terminated
	//If warnsize is larger than size, the function returns without doing anything.
	//Any non-printable characters are removed
	//If rs_filter is 1 or 2, all non ASCII characters are ignored and not copied to the buffer
	//	If islyric is zero, the following characters are also not copied to the buffer:  ( } ,  \  : { " )
	//If rs_filter is 2, the forward slash character is also not copied to the buffer
	//If rs_filter is 3, rs_lyric_filter_char_extended() is used to determine if input lyric text is to be filtered
	//If rs_filter is 4, no characters are filtered

void shrink_xml_text(char *buffer, size_t size, char *input);
	//Does the reverse of expand_xml_text(), converting each escape sequence into the appropriate individual character.
	//If size is zero, the function returns without doing anything.  Otherwise the buffer is guaranteed to be NULL terminated

int parse_xml_attribute_text(char *buffer, size_t size, char *target, char *input);
	//Parses the value of the specified target attribute from the specified input string into the given buffer
	//No more than size bytes are written to the buffer, which is guaranteed to be NULL terminated
	//Returns 1 on success

int parse_xml_attribute_number(char *target, char *input, long *output);
	//Parses and returns the numerical value of the specified target attribute from the specified input string
	//If a number is parsed, it is returned through *output, otherwise *output remains unchanged
	//Returns 1 on success

int parse_xml_rs_timestamp(char *target, char *input, long *output);
	//Parses and returns the numerical value of the specified target attribute timestamp from the specified input string
	//Periods are discarded (which would convert Rocksmith formatted timestamps to milliseconds) and the value of the timestamp is returned through *output
	//Returns 1 on success


#endif //#ifndef _rs_parse_h_
