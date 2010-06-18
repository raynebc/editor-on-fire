//To test the conditional compiling, pass the following preprocessor definition to the compiler to cause the exit and assert wrapper functions to use setjmp() to return to EOF
//#define EOF_BUILD


/*		LYRIC STRUCTURE OVERVIEW:
Import functions are expected to:
	-Handle the offsetoverride parameter appropriately
	-Load tags appropriately
	-Track the start and end of lines of lyrics, overdrive paths, etc.
	-Map the pitch as values from 36-84 where 36 is a C note in the lowest octave used
	-Determine if there is variation in pitch, setting Lyrics.pitch_tracking if so
	-Round the non-floating point start, end and duration values appropriately
	-Provide any non-hyphen tracking for word grouping (ie whitespace prefix or suffix)
	-Call ForceEndLyricLine() or otherwise ensure that there's no open line of lyrics when the import completes
Lyric structure functions are expected to:
	-Remove leading/trailing whitespace for all lyric pieces
	-Detect pitch style
	-Handle the filter parameter appropriately
	-Set the grouping status for each lyric piece (VL import needs to supply some grouping information as it does not end partial words in hyphens by itself)
	-Handle the noplus parameter appropriately
	-Depending on noplus, append a hyphen at the end of a lyric piece that has a piece grouped after it (unless it already ends in a hyphen)
	-Handle the nohyphens parameter appropriately
	-Handle the startstamp parameter appropriately
	-Apply the offset if one was loaded by the import function
	-Handle overlapping lyrics or lyrics with a duration of 0
	-Perform grouping appropriately
	-Remap pitches appropriately if exporting to MIDI format
Export functions will expect the contents of the lyrics structure to be as follows:
	-No leading/trailing whitespace
	-If there is supposed to be whitespace between two lyric pieces, grouping for the piece will be 0
	-The style for each piece will be 0 for normal, '*' for golden/star power, 'F' for freestyle (freestyle takes priority over SP) or '!' for pitchless
	-'+' lyric pieces are stored by themselves with grouping set to nonzero (if it's the first piece in the line, grouping is off)
	-Hyphens have already been removed according to the nohyphens parameter
Export functions are expected to:
	-Round timestamps, durations, etc. where appropriate
	-Leave the contents of the Lyric structure unchanged where feasible (except for instances where the written lyric must be altered, such as to define grouping for SKAR export)
*/


#ifndef _lyric_storage_h_
#define _lyric_storage_h_

#include <stdio.h>			//For the FILE declaration
#include <setjmp.h>			//For the jmp_buf declaration
#include <errno.h>			//For the errno declaration used in the source files
#include "ID3_parse.h"		//For the OmitID3Frame structure declaration


//
//Global Macros- All relevant source/header files will include this header file to obtain these declarations
//
#define PROGVERSION "FoFLyricConverter2.3"
#define LYRIC_NOTE_ON 50	//Previously, if note #s 60-100 were used for Note On events for lyrics, FoF interpreted
							//those notes to indicate playable instrument difficulties.  This was fixed, but I will
							//continue to use this pitch to denote a pitchless lyric during MIDI export
#define LRCTIMESTAMPMAXFIELDLENGTH 2
	//Used to define the max length of the minutes, seconds and hundredths fields in LRC timestamps

#define MINPITCH 36		//Harmonix's defined minimum pitch
#define MAXPITCH 84		//Harmonix's defined maximum pitch
#define PITCHLESS 254	//This is an invalid pitch number that will be used to represent the lyric having no defined pitch

//Input and output formats (stored to Lyrics.in_format and Lyrics.out_format, respectively)
#define SCRIPT_FORMAT 1
#define VL_FORMAT 2
#define MIDI_FORMAT 3
#define USTAR_FORMAT 4
#define LRC_FORMAT 5
#define VRHYTHM_FORMAT 6
#define ELRC_FORMAT 7
#define KAR_FORMAT 8
#define PITCHED_LYRIC_FORMAT 9
#define SKAR_FORMAT 10
#define ID3_FORMAT 11

#define NUMBEROFLYRICFORMATS 11
	//This defined number should be equal to the number of defined lyric macros above, for use with the LYRICFORMATNAMES[] array

//#define NDEBUG		//This will disable the assert macros in the source file if defined
//#define USEMEMWATCH	//Defining this will include the memwatch header in all source files

//Replace use of malloc_err with malloc and calloc_err with calloc when using Memwatch, to make allocations trackable
#ifdef USEMEMWATCH
#define malloc_err malloc
#define calloc_err calloc
#endif

#ifndef EOF_BUILD
	#define assert_wrapper(expression) (assert(expression))
		//Define this as a macro so that the line number and source file given in the assert message remain accurate
#endif


//
//STRUCTURE DEFINITIONS
//
struct Lyric_Piece
{
	unsigned long start;		//The start offset of the piece of lyric in real time (milliseconds)
	unsigned long duration;		//Duration of the piece of lyric in milliseconds
	unsigned char pitch;		//The pitch of the sung lyric, using Rock Band's mapping.  Valid range is 36-95, pitch 36 being mid C
//v2.3	Allow separate tracking for overdrive and freestyle
//	char style;					//If set to '*', denotes starpower/golden note, if set to 'F', denotes freestyle, if set to '!', denotes pitchless (ie. during MIDI import)
	char overdrive;				//Boolean:  If nonzero, this piece is overdrive
	char freestyle;				//Boolean:  If nonzero, this piece is freestyle
	char groupswithnext;		//Boolean:  If nonzero, this piece should group with the next piece (preserving grouping information if nohyphens is used)
	char hasequal;				//Boolean:  If nonzero, this piece had an equal sign during import, which is used by Rock Band to display as a hyphen WITHOUT grouping
	char *lyric;				//The string associated with this piece of lyric
	char leadspace;				//Boolean:  If nonzero, AddLyricPiece found leading whitespace on the string when this lyric was added
	char trailspace;			//Boolean:  If nonzero, AddLyricPiece found trailing whitespace on the string when this lyric was added
	struct Lyric_Piece *next;	//Pointer to next piece of lyrics in the linked list
	struct Lyric_Piece *prev;	//Pointer to next piece of lyrics in the linked list
};

struct Lyric_Line
{
	struct Lyric_Piece *pieces;		//A linked list of lyric pieces
	struct Lyric_Piece *curpiece;	//Conductor of the lyric piece linked list
	struct Lyric_Line *prev;		//Pointer to previous line of lyrics in the linked list
	struct Lyric_Line *next;		//Pointer to next line of lyrics in the linked list
	unsigned long piececount;		//The number of lyric pieces in this line.  Incremented upon adding a lyric piece
	unsigned long start;			//The start offset of the first piece of lyrics in the line in milliseconds
	unsigned long duration;			//The sum of all durations of the contained lyric pieces
};

struct _LYRICSSTRUCT_{
	//Note:  These pointers and their referenced data should not be modified by import/export functions
	struct Lyric_Line *lines;	//A linked list of Lyric Line structures
	struct Lyric_Line *curline;	//Conductor of the lyric line linked list
	struct Lyric_Piece *lastpiece;
		//Used to keep track of the last lyric piece added (ie. Ustar hyphen appending)
		//This is to be used sparingly and the prev Lyric piece pointer is to be used where feasible
	struct Lyric_Piece *prevlineslast;
		//The last lyric piece in the previous line of lyrics (used in EndLyricLine logic, moved outside the function because it couldn't be reinitialized very easily outside EndLyricLine())
	unsigned long linecount;	//Number of lines of lyrics.  Incremented upon parsing the ending of a line of lyrics
	char line_on;			//Boolean:  Specifies whether a line of lyrics is currently being parsed
							//	Set to true upon creating a new line of lyrics
							//	Set to false upon parsing the end of a line of lyrics
	char lyric_defined;		//Boolean:  Specifies whether a lyric event has been parsed and a corresponding Note On event is expected
	char lyric_on;			//Boolean:  Specifies whether a Note On event has been parsed regarding a lyric event
	char overdrive_on;		//Boolean:  Specifies whether AddLyricPiece will denote the lyric as SP (such as if MIDI Note On #116 has occurred)
	char freestyle_on;		//Boolean:	Specifies whether AddLyricPiece will denote the lyric as Freestyle (Used by UStarLoad)

//Substrings that identify each tag for the given import format:
	char *TitleStringID;
	char *ArtistStringID;
	char *AlbumStringID;
	char *EditorStringID;
	char *OffsetStringID;

//Tags
	char *Title;			//The title tag
	char *Artist;			//The artist tag
	char *Album;			//The album tag
	char *Editor;			//The editor tag
	char *Offset;			//The offset tag (additive offset to start time stamps)
							//	VL stores this in increments of 10ms, I will convert it to ms during import

//Offsets
	long realoffset;			//The offset in real time (milliseconds), which is subtracted from each lyric's timestamp
	char offsetoverride;		//If the user defines an offset as a command line parameter, this is set and the offset is
								//used to override any offset defined in the source script.txt or VL file.  This is also set
								//if startstamp is given as a parameter, once the correct offset is calculated
	char srcoffsetoverride;		//If the user defines an offset via command line for the source lyrics, this is set and
								//the offset is stored in srcrealoffset
	long srcrealoffset;			//The offset in real time (millieseconds), which is subtracted from timestamps in the source lyric file
	unsigned long startstamp;	//The custom first timestamp.  All lyric timestamps will be stored with the correct
								//skew so that the first timestamp matches this value (in milliseconds)
								//For now, this is implemented as a modification of the GAP value for UltraStar

//Flags
	char nohyphens;				//Bitflag:  If bit 0 (LSB first) is set, prevent hyphens from being inserted (where not already present) during load (ie. VL files)
								//	If bit 1 (LSB first) is set, remove hyphens that already exist at the end of lyrics during export
	char noplus;				//Boolean:  "+" lyric events will have their duration added to the previous lyric
								//	piece and will be discarded instead of being added to the Lyric structure
	char grouping;				//Specified grouping method for exported lyrics 0=none, 1=per word, 2=per line
	char defaultfilter;			//Boolean:  User did not define a custom filter list, one was allocated and will need to be deallocated
	char verbose;				//Specifies level of logging (1 is verbose, 2 is debug)
	char quick;					//If nonzero, skips all possible MIDI processing (only processing for PART VOCALS)
	char pitch_tracking;		//Boolean:  Specifies whether the input lyrics were a format that includes
								//			pitch detection (MIDI, UltraStar).  If false, generic pitch
								//			notes will be written where applicable (pitch #LYRIC_NOTE_ON
								//			for MIDI or pitch #0 for UltraStar)
	char startstampspecified;	//Is nonzero if a custom first timestamp was specified via command line
								//AddLyricPiece sets this to 2 when the appropriate offset is stored in realoffset
	char brute;					//Boolean:  For UltraStar output, all possible tempos will be tested to find the one with
								//			the most accuracy
	char marklines;				//Boolean:  For script output, #newline entries will be written to keep track of line
								//			demarcation
	char forcemidipitch;		//Boolean:  Pitch output is normally restricted to a generic pitch for MIDI output due to
								//			an outstanding bug in FoFiX.  If the forcemidipitch parameter is specified,
								//			this behavior can be overridden so that the pitches are exported appropriately
	char nolyrics;				//Boolean:	The lyrics are not required to be present in the input file (such as a lyricless MIDI)
								//			The lyrics will be created as asterisks
								//			Set to 1 if defined explicitly via command line
								//			Set to 2 if defined implicitly by importing vrhythm with a vrhythm ID instead of a pitched lyric file
	char notenames;				//Boolean:	For Vrhythm output, write the names of notes instead of the pitch numbers
	char relative;				//Boolean:  For UltraStar output, write the timestamps as relative instead of absolute
	char nopitch;				//Boolean:  For MIDI output, omit writing the vocal notes for pitchless lyrics (freestyle are still written normally)
	char reinit;				//Boolean:	Lyric handlers that use static variables should check for this.  If nonzero, re-init the static variables
								//			This is because when used in EOF, the program may require more than one import, and otherwise the static
								//			variables would never re-initialize

//Various
	unsigned long piececount;	//The number of lyric pieces in all lines.  Incremented upon adding a lyric piece
	char *filter;				//If not NULL, contains a string of characters that will be stripped from the end of lyric pieces.  This will
								//be primarily used to remove vocalization characters ('^', '=' and '#').
	unsigned char in_format;	//Specifies the declared input format
	unsigned char out_format;	//Specifies the declared output format
	double explicittempo;		//Set to nonzero if a tempo was passed as an argument
	unsigned char last_pitch;	//Used by various import functions to track pitch changes.  Reset to 0 by InitMIDI() and InitLyrics()
	struct OmitID3frame *nosrctag;	//The linked list of ID3 frames to omit during ID3 export

//Filenames
	char *outfilename;		//Stores the name of the output file
	char *infilename;		//Stores the name of the input file
	char *srclyrname;		//Stores the name of the input pitched lyrics file (for vocal rhythm import)
	char *srcrhythmname;	//Stores the name of the input vocal rhythm file (for vocal rhythm import)
	char *dstlyrname;		//Stores the name of the output pitched lyrics file (for vocal rhythm export)
	char *srcfile;			//Stores the name of the source midi file used to load MIDI timings and copy tracks from (when exporting to MIDI based formats like MIDI, KAR, Vrhythm)

//Track names
	char *inputtrack;		//Specifies the track from which vocals will be loaded during a MIDI based import
							//	By default, this should be "PART VOCALS", but Rock Band:Beatles supports
							//	harmony vocals that are stored in other MIDI tracks ("PART HARM1","PART HARM2")

	char *outputtrack;		//Specifies the track to which vocals will be written during a MIDI based export
};

struct Lyric_Format
{
	unsigned char format;		//This is one of the macro defined lyric types.  A display name for this format can be obtained by
								//indexing into the LYRICFORMATNAMES array with: LYRICFORMATNAMES[format]
	char *track;				//If the lyric format is a MIDI based format, this is the track containing the specified lyrics
	unsigned long count;		//This is the number of lyrics that are believed to be in the track (if it's a MIDI based format)
	struct Lyric_Format *next;	//The next detected format, or NULL if there are no others
};


//
//EOF INTEGRATION FUNCTIONS
//
void exit_wrapper(int status);				//What this program's logic calls instead of exit().  If #EOF_BUILD is not defined at compile time,
											//the wrapper function just passes the status variable to exit()
#ifdef EOF_BUILD
	void assert_wrapper(int expression);	//What this program's logic calls instead of assert().  If #EOF_BUILD is not defined at compile time,
											//the macro defined earlier in this source file calls assert() with the given expression
#endif


//
//GENERAL LYRIC HANDLING FUNCTIONS
//
void InitLyrics(void);
	//Initialize values in Lyrics structure, this is expected to be called before populating the Lyric structure,
	//such as with VL_Load, Script_Load or MIDI_Load
void CreateLyricLine(void);
	//Initialize a new line of lyrics in the Lyrics structure
void EndLyricLine(void);
	//End the current line of lyrics if it is not empty
	//Also performs error correction (durations of 0, overlapping) and plus sign truncation
void ForceEndLyricLine(void);
	//Similar to EndLyricLine, but if the current line of lyrics is empty, the line is removed from the linked list
	//and the line status is changed to closed.  This function should be called at the end of each import function
	//to ensure that there is no open line of lyrics
void AddLyricPiece(char *str,unsigned long start,unsigned long end,unsigned char pitch,char groupswithnext);
	//Add a lyric piece if it is not all whitespace.  The string str is copied into a new array, so str can be
	//discarded afterwards.  The array referenced by str is edited to remove trailing whitespace
	//groupswithnext specifies whether the calling function has determined whether the lyric groups with the NEXT
	//lyric piece:
	//	If nonzero, the lyric has been determined to group
	//	If zero, the lyric grouping has not been determined (and will be determined based on appended - or + chars)
	//Groupswithnext is usually only set during VL, Ustar, LRC and KAR import
	//If pitch is equal to PITCHLESS (254), the lyric is added as being pitchless
struct Lyric_Piece *FindLyricNumber(unsigned long number);
	//Returns a pointer to the specified lyric piece number, or NULL if the specied lyric number doesn't exist
void PostProcessLyrics(void);
	//To be called after lyric import.  This function will validate the Lyrics structure, including counters,
	//grouping status, perform hyphen insertion/truncation and perform word and line grouping appropriately
int ParseTag(char startchar,char endchar,char *inputstring,char negatizeoffset);
	//Searches for tags as defined by the tag identifier strings and populates the Lyrics structure
	//using a COPY of inputstring.  Inputstring is not modified, stored or released by this function
	//If endchar is '\0', then the contents of the tag are expected to extend until the end of the input string
	//Negatizeoffset is boolean and specifies whether the passed offset string needs to be made negative, this
	//signifies that it is a positive offset that is treated as negative, so that it increases the timestamp when
	//it is subtracted from timestamps (ie. MIDI song.ini delay or UltraStar gap tag)
	//Returns a positive value if a tag was stored
void SetTag(char *string,char tagID,char negatizeoffset);
	//Store tag into Lyrics structure.  A copy of string is stored into the Lyrics structure as applicable
	//The calling function is responsible for releasing memory
	//Negatizeoffset is boolean and specifies whether the passed offset string needs to be made negative, this
	//signifies that it is a positive offset that is treated as negative, in this it is to be subtracted from
	//timestamps (ie. MIDI song.ini delay or UltraStar gap tag)
struct Lyric_Line *InsertLyricLineBreak(struct Lyric_Line *lineptr,struct Lyric_Piece *lyrptr);
	//Split the linked list into two different lines, inserting the break in front of the lyric referenced by lyrptr, in the lyric line referenced by lineptr
	//returns the newly-created line structure that now contains the lyric linked list that now begins with lyrptr, or returns lineptr if no split occurred
void RecountLineVars(struct Lyric_Line *start);
	//Rebuild the piececount and duration for each line of lyrics, starting with the line given
void ReleaseMemory(char release_all);
	//Release all memory consumed by the VL, MIDI and Lyrics data structures
	//If release_all is nonzero, strings that are set by command line are also released, otherwise they are kept
	//Such strings should only be released when the program is finished, otherwise command line parameters may be lost
	//release_all should be 0 if being called by the lyric detection logic
void EnumerateFormatDetectionList(struct Lyric_Format *detectionlist);
	//Used to display information about the detected lyric formats detailed in the list
	//If the passed parameter is NULL, the information displayed is that it is not a valid lyric file
void DestroyLyricFormatList(struct Lyric_Format *ptr);
	//Used to deallocate the linked list returned by DetectLyricFormat()


//
//STRING FUNCTIONS
//
char *DuplicateString(const char *str);
	//Allocates enough memory to copy the input string, copies it and returns the newly allocated string by reference
	//This function is provided to duplicate strings when the source string is a constant char array, eliminating
	//any applicable compiler warnings
char *TruncateString(char *str,char dealloc);
	//Creates and returns a duplicate of str without leading and trailing whitespace.
	//if dealloc is nonzero, str is de-allocated, so the calling function can overwrite the str pointer with the return value
char *ResizedAppend(char *src1,const char *src2,char dealloc);
	//Allocates enough memory to concatenate both strings and returns the concatenated string
	//if dealloc is zero and src2 is "", the net result of the function call is to allocate an array, copy to it from src1 and return it
	//if dealloc is nonzero, src1 is de-allocated, so the calling function can overwrite the src1 pointer with the return value
char *Append(const char *src1,const char *src2);
	//The same as ResizedAppend(), but does not take de-allocation into account, allowing the easier use of const char * types
char *strcasestr_spec(char *str1,const char *str2);
	//Performs a case INSENSITIVE search of str2 in str1, returning the character AFTER the match in str1 if it exists, else NULL
char *ConvertNoteNum(unsigned char notenum);
	//Takes a note number from 0 through 127 and returns a string representation of the note name, such as "C#-1" or "G9"


//
//FILE I/O / MEMORY FUNCTIONS
//
void ReadWORDLE(FILE *inf,unsigned short *data);
	//Takes a pointer to a 2 byte unsigned short, reads 2 bytes from input file and uses bit shifts to arrange them properly into memory
	//Little endian encoding is assumed, where the first byte read is expected to be the least significant
	//VL files are written in Little Endian format
void WriteWORDLE(FILE *outf,unsigned short data);
	//Writes a two byte value to file in Little Endian format (least significant byte written first)
void ReadDWORDLE(FILE *inf,unsigned long *data);
	//Takes a pointer to a 4 byte unsigned long, reads 4 bytes from input file and uses bit shifts to arrange them properly into memory
	//Little endian encoding is assumed, where the first byte read is expected to be the least significant
	//VL files are written in Little Endian format
void WriteDWORDLE(FILE *outf,unsigned long data);
	//Writes a four byte value to file in Little ndian format (least significant byte written first)
void ReadWORDBE(FILE *inf, unsigned short *ptr);
	//Read 2 bytes in Big Endian format from file into *ptr, performing bit shifting accordingly
void WriteWORDBE(FILE *outf,unsigned short data);
	//Writes a two byte value to file in Big Endian format (most significant byte written first)
void ReadDWORDBE(FILE *inf, unsigned long *ptr);
	//Read 4 bytes in Big Endian format from file into *ptr, performing bit shifting accordingly
void WriteDWORDBE(FILE *outf,unsigned long data);
	//Writes a four byte value to file in Big ndian format (most significant byte written first)
unsigned long ParseUnicodeString(FILE *inf);
	//Reads from input file, returning the length of the Unicode string (not including NULL terminator) at the current file position,
	//returning half the length (ANSI format)
void WriteUnicodeString(FILE *outf,char *str);
	//Takes an 8 bit encoded ANSI string (UTF-8) and writes it in the two byte encoded character format used by VL
	//If str is NULL, an empty Unicode string is written (two 0's)
char *ReadUnicodeString(FILE *inf);
	//Counts the length of the Unicode string (UTF-8) at the current file position, allocates an array, reads the string into
	//the array and returns it by reference
long int ParseLongInt(char *buffer,unsigned long *startindex,unsigned long linenum,int *errorstatus);
	//Takes a string and starting index and parses the string (ignoring leading whitespace) to find first numerical
	//character.  When a numerical character is found, leading zeroes are discarded and a numerical string is read
	//and converted to a long integer value, which is returned.  The index of the first character after the parsed
	//string is passed back into (*startindex) and should point to a whitespace character.
	//linenum should be set to the line number of the file being parsed.  Upon error, this line number is indicated,
	//assisting in the identification of the problematic input and the program will exit if errorstatus is NULL.
	//If errorstatus is NOT NULL, a nonzero number is instead stored into it and 0 is returned upon parse error
	//and the error is not printed to stdout
	//Note:  This function will return a negative value if a negative number is read from the input string
unsigned long FindLongestLineLength(FILE *inf,char exit_on_empty);
	//Returns the length of the longest line in the input text file.  If 0 is returned, the file should be
	//considered empty.  If exit_on_empty is nonzero., exit(1) is called if the parsed file is empty
	//The returned count adds one byte to account for the required null terminator
	//The file is automatically rewound before the function returns
int FindNextNumber(char *buffer,unsigned long *startindex);
	//If there is a numerical character at or after buffer[startindex], nonzero is returned and startindex will contain the index of the character
long ftell_err(FILE *fp);
	//A wrapper function that returns the result of ftell(), automatically checking for and throwing any error given
void fseek_err(FILE *stream,long int offset,int origin);
	//A wrapper function that performs the seek operation, automatically checking for and throwing any error given
void fread_err(void *ptr,size_t size,size_t count,FILE *stream);
	//A wrapper function that performs the read operation, automatically checking for and throwing any error given
void fwrite_err(const void *ptr,size_t size,size_t count,FILE *stream);
	//A wrapper function that performs the write operation, automatically checking for and throwing any error given
FILE *fopen_err(const char *filename,const char *mode);
	//A wrapper function that opens the file, automatically checking for and throwing any error given
void fflush_err(FILE *stream);
	//A wrapper function that flushes the stream, automatically checking for and throwing any error given
void fclose_err(FILE *stream);
	//A wrapper function that closes the file, automatically checking for and throwing any error given
void fputc_err(int character,FILE *stream);
	//A wrapper function that writes the character, automatically checking for and throwing any error given
int fgetc_err(FILE *stream);
	//A wrapper function that gets the character, automatically checking for and throwing any error given
char *fgets_err(char *str,int num,FILE *stream);
	//A wrapper function that gets the string, automatically checking for and throwing any error given
void fputs_err(const char *str,FILE *stream);
	//A wrapper function that writes the string, automatically checking for and throwing any error given
struct Lyric_Format *DetectLyricFormat(char *file);
	//Determines and returns information (as a linked list) about lyrics detected in the input file
	//If the file matches multiple lyric types (one of the MIDI based formats), multiple links in the list will exist.
	//Vocal Rhythm MIDI will NOT be detected
	//If NULL is returned, the file is not valid for import (invalid lyrics or unknown type)
	//NOTE:  Only MIDI tracks that have a name are included in the detection for MIDI type formats
//v2.3	Updated ParseString() to give the number of bytes read, so the calling function can omit calls to fseek
//char *ParseString(FILE *inf);
char *ReadString(FILE *inf,unsigned long *bytesread);
	//Parses a null terminated ASCII string at the current file position, allocates memory for it and returns it
	//NULL is returned upon error
	//Upon success, the file position is left after the null terminator of the string that was read
unsigned long GetFileEndPos(FILE *fp);
	//Accepts a binary mode stream pointer, seeks to the last byte, obtains the file position,
	//seeks to the original file position and returns the file position of the last byte
int BlockCopy(FILE *inf,FILE *outf,unsigned long num);
	//Accepts an input file and output file, copying the specified number of bytes from the former to the latter
	//Returns 0 on success, -1 on memory allocation error, -2 on file I/O error or -3 on other error


#ifndef USEMEMWATCH
void *malloc_err(size_t size);
	//A wrapper function that allocates memory and returns a pointer to it, automatically checking for and throwing any error given
void *calloc_err(size_t num,size_t size);
	//A wrapper function that allocates memory, inits it to 0s and returns a pointer to it, automatically checking for and throwing any error given
#endif


//
//EXTERNAL GLOBAL VAR DEFINITIONS
//
extern struct _LYRICSSTRUCT_ Lyrics;
extern jmp_buf jumpbuffer;
extern jmp_buf FLjumpbuffer;	//This is used by FLC's internal logic to provide for exception handling (ie. in validating MIDI files with DetectLyricFormat())
extern char useFLjumpbuffer;	//Boolean:  If nonzero, FLC's logic intercepts in exit_wrapper() regardless of whether EOF_BUILD is defined
								//this must be zero in order for exit_wrapper to return control to EOF
extern const char *LYRICFORMATNAMES[];	//This is a list of strings pertaining to the 10 lyric file types #defined near the beginning of Lyric_storage.c

void DEBUG_QUERY_LAST_PIECE(void);	//Debugging to query the info for the last lyric piece
void DEBUG_DUMP_LYRICS(void);		//Debugging to display all lyrics


#endif /* #ifndef _lyric_storage_h_ */
