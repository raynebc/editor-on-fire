#ifndef _id3_parse_h_
#define _id3_parse_h_

struct ID3Frame
{
	char *FrameID;			//A null terminated respresentation of the frame ID, ie "SYLT"
	unsigned long pos;		//The file position of the frame header
	unsigned long length;	//The length of the frame (minus header)
	struct ID3Frame *prev;	//Previous link in the list
	struct ID3Frame *next;	//Next link in the list
};

struct ID3Tag
{
	FILE *fp;					//The file pointer to the file being parsed
	unsigned long tagstart;		//This is the file position of the first byte in the ID3 Tag header
	unsigned long framestart;	//This is the file position of the first byte past the ID3 Tag header(s), which is the first frame header
	unsigned long tagend;		//This is the file position of the first byte past the ID3 Tag

	//These three variables are set by GetMP3FrameDuration(), if this is an MP3 file
	unsigned long samplerate;	//The detected sample rate
	unsigned long samplesperframe;	//This is 384 for Layer 1 or 1152 for Layer 2 or Layer 3
	double frameduration;		//The realtime duration, in millis, of one MPEG frame (samplesperframe * 1000 / samplerate)

	struct ID3Frame *frames;	//A linked list of ID3Frames that is populated by ID3FrameProcessor()
};

int SearchValues(FILE *inf,unsigned long breakpos,unsigned long *pos,const unsigned char *phrase,unsigned long phraselen,unsigned char autoseek);
	//Searches from the current file position of inf for the first match of the specified array of characters
	//If file position breakpos is reached or exceeded, and breakpos is nonzero, the function will end the search even if no match was found
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

int FindID3Tag(struct ID3Tag *ptr);
	//Find the ID3 tag header so that all parsing can take place within the confines of the tag instead
	//of through the entire file.  The file pointer in the passed structure is expected to be opened to the file to parse
	//The start of the tag and the file position of the first byte outside the tag are populated in ptr.
	//Nonzero is returned upon success or zero is returned upon failure

#ifdef EOF_BUILD
int ReadID3Tags(char *filename,char *artist,char *title,char *year);
	//Rewinds the input file and searches it for the TIT2, TYER and TPE1 frames in an existing ID3v2 tag
	//Tag strings are read into the pointers that are passed via reference
	//Returns the number of tags read, or 0 if none are read (ie. error)
#else
int ReadID3Tags(char *filename);
	//A function to test the ID3 tag reading logic by printing the found tags to STDOUT
#endif


int FindID3Tag(struct ID3Tag *ptr);
	//Called by ReadID3Tags() to find the ID3 tag header so that all parsing can take place within the confines of the tag instead
	//of through the entire file.  The file pointer in the passed structure is expected to be opened to the file to parse
	//The start of the tag and the file position of the first byte outside the tag are populated in ptr.
	//Nonzero is returned upon success or zero is returned upon failure

unsigned long GetMP3FrameDuration(struct ID3Tag *ptr);
	//Expects that the file position is at the first MP3 frame, and not the ID3 tag
	//Returns the sample rate, or 0 on error.  Valid sample rates are: 8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100 and 48000
	//The MPEG version and layer description are examined to determine the sample rate and the number of samples per frame
	//These two values are used to determine the realtime duration of one MPEG frame in milliseconds (stored as double floating point)

void ID3_Load(FILE *inf);
	//Parses the file looking for an ID3 tag.  If found, the first MP3 frame is examined to obtain information to
	//determine the realtime duration of one MPEG frame.  Then an SYLT frame is searched for within the ID3 tag.
	//If found, synchronized lyrics are imported

void SYLT_Parse(struct ID3Tag *tag);
	//Called by ID3_Load()
	//Expects the structure's file pointer to be at the beginning of an SYLT frame
	//The ID3 tag must have been processed so that the start and end of the tag is known
	//The sample rate is also expected to be nonzero if MPEG frame format timestamps are defined
	//Parses the lyrics from the SYLT frame and loads them into the Lyrics structure

unsigned long ID3FrameProcessor(struct ID3Tag *ptr);
	//Parses the file given in the structure, building a list of all ID3 frames encountered.  The extended header is looked for and
	//skipped.  Any non-valid frame header is skipped
	//Returns the number of frames parsed and stored into the ID3Frame list (0 would be considered a failure)

int ValidateID3FrameHeader(struct ID3Tag *ptr);
	//Reads the presumed 10 byte header at the current file position
	//Returns nonzero (success) if the first four bytes are all capital alphabetical and/or numerical, the file position is at or after the
	//position of the defined start of the tag, and the end file position is at or before the defined end of the tag
	//The file position is returned unchanged from its original position

void DestroyID3FrameList(struct ID3Tag *ptr);
	//Deallocates all memory used in the ID3Frame linked list

unsigned long BuildID3Tag(struct ID3Tag *ptr,FILE *outf);
	//Writes the modified ID3 tag from the input file (including the imported lyrics) to the output file
	//All non ID3 data is written as-is
	//Returns the number of frames written to output file

void Export_ID3(FILE *inf, FILE *outf);
	//Takes an input MP3 file and writes the contents of the Lyrics structure to the output file,
	//preserving the other ID3 and audio data from the input file

#endif //#ifndef _id3_parse_h_
