#ifndef _rs_parse_h_
#define _rs_parse_h_

void Export_RS(FILE *outf);
	//Exports the loaded lyric structure to output file in Rocksmith XML format
void RS_Load(FILE *inf);
	//Perform all code necessary to load a Rocksmith format lyric file

void expand_xml_text(char *buffer, size_t size, const char *input, size_t warnsize);
	//Copies the input string into the specified buffer of the given size.  Any of the characters that XML requires to be escaped
	//are converted into the appropriate character sequence (ie. ' becomes &apos;).  If the expanded string's length is longer
	//than the given warning value, the user is given a warning message that the string will need to be shortened and the string
	//is truncated to be warnsize number of characters.  If warnsize is zero, no check or truncation is performed.
	//If size is zero, the function returns without doing anything.  Otherwise the buffer is guaranteed to be NULL terminated

void shrink_xml_text(char *buffer, size_t size, char *input);
	//Does the reverse of expand_xml_text(), converting each escape sequence into the appropriate individual character.
	//If size is zero, the function returns without doing anything.  Otherwise the buffer is guaranteed to be NULL terminated

int parse_xml_attribute_text(char *buffer, size_t size, char *target, char *input);
	//Parses the value of the specified target attribute from the specified input string into the given buffer
	//No more than size bytes are written to the buffer, which is guaranteed to be NULL terminated
	//Returns 1 on success

int parse_xml_attribute_number(char *target, char *input, long *output);
	//Parses and returns the numerical value of the specified target attribute from the specified input string
	//The numerical value is returned through *output
	//Returns 1 on success

int parse_xml_rs_timestamp(char *target, char *input, long *output);
	//Parses and returns the numerical value of the specified target attribute timestamp from the specified input string
	//Periods are discarded (which would convert Rocksmith formatted timestamps to milliseconds) and the value of the timestamp is returned through *output
	//Returns 1 on success


#endif //#ifndef _rs_parse_h_
