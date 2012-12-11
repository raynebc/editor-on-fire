#ifndef _rs_parse_h_
#define _rs_parse_h_

void Export_RS(FILE *outf);
	//Exports the loaded lyric structure to output file in Rocksmith XML format

void expand_xml_text(char *buffer, size_t size, const char *input, size_t warnsize);
	//Copies the input string into the specified buffer of the given size.  Any of the characters that XML requires to be escaped
	//are converted into the appropriate character sequence (ie. ' becomes &apos;).  If the expanded string's length is longer
	//than the given warning value, the user is given a warning message that the string will need to be shortened.

#endif //#ifndef _rs_parse_h_
