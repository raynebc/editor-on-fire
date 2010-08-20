#ifndef _id3_parse_h_
#define _id3_parse_h_

struct ID3Frame
{
	char *frameid;			//A null terminated respresentation of the ID3v2 frame ID, ie "SYLT"
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

	//ID3v2 information
	struct ID3Frame *frames;	//A linked list of ID3Frames that is populated by ID3FrameProcessor()

	//ID3v1 information
	char id3v1present;			//If 1, an ID3v1 tag exists in the file, although none of the strings are necessarily defined
								//If 2, one or more of the ID3v1 strings are populated
	char *id3v1title,*id3v1artist,*id3v1album,*id3v1year;
};

struct OmitID3frame
{
	char *frameid;	//The ID of the frame to omit, ie. "TPE1"
	struct OmitID3frame *next;	//The next link in the list
};

char *ReadTextInfoFrame(FILE *inf);
	//Expects the file position of inf to be at the beginning of an ID3 Frame header for a Text Information Frame
	//The first byte read (which will be from the Frame ID) is expected to be 'T' as per ID3v2 specifications
	//Reads and returns the contents of the frame in a newly-allocated string
	//NULL is returned if the encoding is Unicode, if the frame is malformed or if there is an I/O error
	//Upon successful parse, the file position is let at one byte past the end of the frame that was read
	//Upon error, inf is returned to its original file position
int FindID3Tag(struct ID3Tag *ptr);
	//Find the ID3 tag header so that all parsing can take place within the confines of the tag instead
	//of through the entire file.  The file pointer in the passed structure is expected to be opened to the file to parse
	//The file positions of the start of the tag and the first byte outside the tag are populated in ptr.
	//Nonzero is returned upon success or zero is returned upon failure
void DisplayID3Tag(char *filename);
	//A function to test the ID3 tag reading logic by printing the found tags to STDOUT
	//The contents of each text information frame is displayed, all other frames are just displayed as their frame IDs
unsigned long GetMP3FrameDuration(struct ID3Tag *ptr);
	//Expects that the file position is at the first MP3 frame, and not the ID3 tag
	//Returns the sample rate, or 0 on error.  Valid sample rates are: 8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100 and 48000
	//The MPEG version and layer description are examined to determine the sample rate and the number of samples per frame
	//These two values are used to determine the realtime duration of one MPEG frame in milliseconds (stored as double floating point)
	//The sample rate, samples per frame and MPEG frame duration are stored in ptr
void ID3_Load(FILE *inf);
	//Parses the file looking for an ID3 tag.  If found, the first MP3 frame is examined to obtain information to
	//determine the realtime duration of one MPEG frame.  Then an SYLT frame is searched for within the ID3 tag.
	//If found, synchronized lyrics are imported if they exist
void SYLT_Parse(struct ID3Tag *tag);
	//Called by ID3_Load()
	//Expects the structure's file pointer to be at the beginning of an SYLT frame
	//The ID3 tag must have been processed so that the start and end of the tag is known
	//The sample rate is also expected to be nonzero if MPEG frame format timestamps are defined
	//Parses the lyrics from the SYLT frame and loads them into the Lyrics structure
unsigned long ID3FrameProcessor(struct ID3Tag *ptr);
	//Parses the file given in the structure, building a list of all ID3 frames encountered.  The extended header is looked for and
	//skipped.  Any non-valid frame header is skipped.  The file position is not required to be anything in particular, this function
	//will locate the ID3 header and seek to it automatically.
	//Returns the number of frames parsed and stored into the ID3Frame list (0 would be considered a failure)
int ValidateID3FrameHeader(struct ID3Tag *ptr);
	//Reads the presumed 10 byte header at the current file position
	//Returns nonzero (success) if the first four bytes are all capital alphabetical and/or numerical, the file position is at or after the
	//position of the defined start of the tag, and the end file position is at or before the defined end of the tag
	//The file position is returned unchanged from its original position
void DestroyID3(struct ID3Tag *ptr);
	//Deallocates all memory used in the ID3Frame linked list, and the ID3v1 strings
unsigned long BuildID3Tag(struct ID3Tag *ptr,FILE *outf);
	//Writes the modified ID3 tag from the input file (including the imported lyrics) to the output file
	//All non ID3 data is written as-is
	//Returns the number of frames written to output file
void Export_ID3(FILE *inf, FILE *outf);
	//Takes an input MP3 file and writes the contents of the Lyrics structure to the output file,
	//preserving the other ID3 and audio data from the input file
struct ID3Frame *FindID3Frame(struct ID3Tag *tag,const char *frameid);
	//Accepts an ID3Tag structure that has been initialized by FindID3Tag() and ID3FrameProcessor()
	//Searches the frame list for the first instance of a frame matching the given ID string and returns it, or NULL if no match is found
char *GrabID3TextFrame(struct ID3Tag *tag,const char *frameid,char *buffer,unsigned long buffersize);
	//Accepts an ID3Tag structure that has been initialized by FindID3Tag() and ID3FrameProcessor()
	//Searches the frame list for the first instance of a frame matching the given ID string
	//The frame is treated as a text info frame and parsed as such.
	//If buffer is not NULL, the frame content is copied to the buffer and buffer's pointer is returned
	//If necessary, the string content will be truncated to fit based on the given size of the buffer
	//If buffer is NULL, the allocated string is returned
	//NULL is returned in the event of an error or if the text is in Unicode
struct OmitID3frame *AddOmitID3framelist(struct OmitID3frame *ptr,const char *frameid);
	//Creates a link for the given frame ID and inserts it to the front of the list, returning the address of the updated HEAD link
	//frameid is copied to a new array and is not altered
int SearchOmitID3framelist(struct OmitID3frame *ptr,const char *frameid);
	//Returns 1 if a given frame ID is in the omit list, 2 if the omit list has a frame ID entry beginning with '*' (wildcard)
	//otherwise 0 is returned
void DestroyOmitID3framelist(struct OmitID3frame *ptr);
	//Deallocates all memory used in the omit list
void WriteTextInfoFrame(FILE *outf,const char *frameid,const char *string);
	//Writes an ID3 text information frame to the file
char IsTextInfoID3FrameID(char *frameid);
	//Returns nonzero if the frame ID matches that of a Text Information frame, based on the ID3v2 specification

#endif //#ifndef _id3_parse_h_
