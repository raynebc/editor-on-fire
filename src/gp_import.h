#ifndef EOF_GP_IMPORT_H
#define EOF_GP_IMPORT_H

#define EOF_CODA_SYMBOL 0
#define EOF_DOUBLE_CODA_SYMBOL 1
#define EOF_SEGNO_SYMBOL 2
#define EOF_SEGNO_SEGNO_SYMBOL 3
#define EOF_FINE_SYMBOL 4
#define EOF_DA_CAPO_SYMBOL 5
#define EOF_DA_CAPO_AL_CODA_SYMBOL 6
#define EOF_DA_CAPO_AL_DOUBLE_CODA_SYMBOL 7
#define EOF_DA_CAPO_AL_FINE_SYMBOL 8
#define EOF_DA_SEGNO_SYMBOL 9
#define EOF_DA_SEGNO_AL_CODA_SYMBOL 10
#define EOF_DA_SEGNO_AL_DOUBLE_CODA_SYMBOL 11
#define EOF_DA_SEGNO_AL_FINE_SYMBOL 12
#define EOF_DA_SEGNO_SEGNO_SYMBOL 13
#define EOF_DA_SEGNO_SEGNO_AL_CODA_SYMBOL 14
#define EOF_DA_SEGNO_SEGNO_AL_DOUBLE_CODA_SYMBOL 15
#define EOF_DA_SEGNO_SEGNO_AL_FINE_SYMBOL 16
#define EOF_DA_CODA_SYMBOL 17
#define EOF_DA_DOUBLE_CODA_SYMBOL 18

#ifndef EOF_BUILD
	//Compile standalone parse utility, use macros to make the file compile without Allegro
	#define PACKFILE FILE
	#define pack_fread(x, y, z) fread(x, 1, y, z)
	#define pack_fopen fopen
	#define pack_fclose fclose
	#define pack_getc getc
	#define pack_fseek(x, y) fseek(x, y, SEEK_CUR)
	#define EOF_SONG char
	#define pack_feof feof
	void eof_gp_debug_log(FILE *inf, char *text);		//Prints the current file position and the specified string to the console
	extern char **eof_note_names;
#else
	struct eof_gp_measure
	{
		unsigned char num, den;		//The 8 bit numerator and denominator defined in guitar pro time signatures
		unsigned char start_of_repeat;	//If nonzero, indicates that this measure is the start of a repeat (measure 0 is this by default)
		unsigned char num_of_repeats;	//If nonzero, indicates the end of a repeat as well as how many repeats
		unsigned char alt_endings; 		//Alternative endings mask, is 0 if no alternate ending occurs
		unsigned char tripletfeel;		//The triplet feel duration in effect for this measure (0 = none, 1 = 8th note, 2 = 16th note)
	};

	struct eof_guitar_pro_struct
	{
		unsigned long numtracks;			//The number of tracks loaded from the guitar pro file
		unsigned fileversion;				//The version of the GP format being imported
		char **names;						//An array of strings, representing the native name of each loaded track
		char *instrument_types;				//An array of values indicating the instrument type of each loaded track (1 = guitar, 2 = bass, 3 = drum, 0 = other)
		EOF_PRO_GUITAR_TRACK **track;		//An array of pro guitar track pointers, representing the imported note data of each loaded track
		EOF_TEXT_EVENT * text_event[EOF_MAX_TEXT_EVENTS];	//An array of pro guitar text event structures, representing the section markers and beat text imported for each loaded track
		struct eof_gp_measure *measure;		//An array of measure data from the Guitar Pro file
		unsigned long measures;			//The number of elements in the above array
		unsigned long text_events;			//The size of the text_event[] array
		unsigned int symbols[19];			//The position (measure number) of each musical symbol (ie. Segno)
		char coda_activated;				//Indicates that an "...al coda" symbol was reached, meaning that when "da coda" is reached, the GP track unwrapping will redirect to that symbol
		char double_coda_activated;			//Indicates that an "...al double coda" symbol was reached, meaning that when "da double coda" is reached, the GP track unwrapping will redirect to that symbol
		char fine_activated;					//Indicates that an "...al fine" symbol was reached, meaning that when "fine" is reached, the GP track unwrapping will end
	};

	struct eof_gpa_sync_point
	{
		unsigned long realtime_pos;		//The realtime position of the sync point in milliseconds
		char is_negative;				//Tracks whether the imported timestamp is below 0 seconds
		unsigned long measure;		//The measure at which this sync point exists, numbered starting from 0
		double pos_in_measure;			//The position (from 0 to 1) in the measure at which the sync point exists
		double qnote_length;			//The length of each quarter note from this sync point until the next
		double real_qnote_length;		//Multiple users have encountered instances where Go PlayAlong exported invalid quarter note timings to XML, EOF will recreate them by dividing the time between sync points by the number of beats between them
		double beat_length;			//The measured length of each beat based on the realtime distance between sync points and the number of beats between them
		char processed;				//Is set to nonzero after the sync point is incorporated into the project's tempo map
	};

	void eof_gp_debug_log(PACKFILE *inf, char *text);
		//Logs the text
	EOF_SONG *eof_import_gp(const char * fn);
		//Currently parses through the specified GP file and outputs its details to stdout
	struct eof_guitar_pro_struct *eof_load_gp(const char * fn, char *undo_made);
		//Parses the specified guitar pro file, returning a structure of track information and a populated pro guitar track for each
		//Returns NULL on error
		//NOTE:  Beats are added to the current project if there aren't as many as defined in the GP file.
		//If the user opts to import the GP file's time signatures, an undo state will be made if undo_made is not NULL and *undo_made is zero.  The referenced memory will then be set to nonzero
	int eof_unwrap_gp_track(struct eof_guitar_pro_struct *gp, unsigned long track, char import_ts, char beats_only);
		//Unwrap the specified track in the guitar pro structure into a new pro guitar track
		//If the track being unwrapped is 0, text events will be unwrapped and gp's text events array will be replaced if there are any text events
		// (so that the text events are only unwrapped once instead of duplicating for each track that is unwrapped)
		//If import_ts is nonzero, the active project's time signatures are updated to reflect those of the unwrapped transcription
		//If beats_only is nonzero, no notes or text events are unwrapped, but beats and time signatures are (ie. to unwrap the measures before applying beat timings in GPA import)
		//Returns nonzero on error
	char eof_copy_notes_in_beat_range(EOF_SONG *ssp, EOF_PRO_GUITAR_TRACK *source, unsigned long startbeat, unsigned long numbeats, EOF_SONG *dsp, EOF_PRO_GUITAR_TRACK *dest, unsigned long destbeat);
		//Copies the notes within the specified range of beats in the source track to the same number of beats starting at the specified beat in the destination track
		//Since the application of time signatures can alter the beat map (and thus beat timing), the beat array from *sp is used as the source beat map
		// and that from *dp is used as the destination beat map
		//Returns zero on error
	int eof_get_next_gpa_sync_point(char **buffer, struct eof_gpa_sync_point *ptr);
		//Reads the next GPA sync point into the referenced structure, advancing *buffer to point to the character after the timestamp read (is '#' between timestamps or '<' after the last timestamp)
		//Returns nonzero on success

#endif

struct guitar_pro_bend
{
	unsigned char summaryheight;	//The bend height specified in the selection list, defining a summarized height of the bend in quarter steps
	unsigned char bendpoints;		//The number of bend points defined in the structure (maximum value is 30)
	unsigned char bendpos[30];		//An array defining the position of each bend point (Valued 0 through 60, in sixtieths of the note's duration)
	unsigned char bendheight[30];	//An array defining the height of each bend point in quarter steps
};

void pack_ReadWORDLE(PACKFILE *inf, unsigned *data);
	//Read a little endian format 2 byte integer from the specified file.  If data isn't NULL, the value is stored into it.
void pack_ReadDWORDLE(PACKFILE *inf, unsigned long *data);
	//Read a little endian format 4 byte integer from the specified file.  If data isn't NULL, the value is stored into it.
int eof_read_gp_string(PACKFILE *inf, unsigned *length, char *buffer, char readfieldlength);
	//Read a string, prefixed by a one-byte string length, from the specified file
	//If readfieldlength is nonzero, a 4 byte field length that prefixes the string length is also read
	//If length is not NULL, the string length is returned through it.
	//Buffer must be able to store at least 256 bytes to avoid overflowing.
int eof_gp_parse_bend(PACKFILE *inf, struct guitar_pro_bend *bp);
	//Parses the bend at the current file position, outputting debug logging appropriately
	//If bp is not NULL, the bend data is stored into the structure
	//Returns nonzero if there is an error parsing, such as end of file being reached unexpectedly

#endif


