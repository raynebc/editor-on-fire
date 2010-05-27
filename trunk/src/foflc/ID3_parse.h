#ifndef _id3_parse_h_
#define _id3_parse_h_

int SearchValues(FILE *inf,unsigned long *pos,const unsigned char *phrase,unsigned long phraselen,unsigned char autoseek);
	//Searches from the current file position of inf for the first match of the specified array of characters
	//phrase is an array of characters to find, and phraselen is the number of characters defined in the array
	//If a match is found, the file position is returned through pos (if it isn't NULL) and 1 is returned
	//If inf or phrase are NULL or if an I/O error occurs, -1 is returned
	//If the file is parsed but no match is found, 0 is returned and pos is not modified
	//If autoseek is nonzero, inf is left positioned at the first match, otherwise it is returned at its original file position

char *ReadTextInfoFrame(FILE *inf);
	//Expects the file position of inf to be at the beginning of an ID3 Frame header for a Text Information Frame
	//The first byte read (which will be from the Frame ID) is expected to be 'T' as per ID3v2 specifications
	//Reads and returns the string from the frame in a newly-allocated string
	//NULL is returned if the encoding is Unicode, if the frame is malformed or if there is an I/O error
	//Upon successful parse, the file position is one byte past the end of the frame that was read
	//Upon error, inf is returned to its original file position

int ReadTags(FILE *inf);
	//A function to test the ID3 tag reading logic

#endif //#ifndef _id3_parse_h_
