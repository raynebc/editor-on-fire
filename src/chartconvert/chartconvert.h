#define ALLEGRO_USE_CONSOLE
#include <allegro.h>

#define PATH_WIDTH 1024
#define NUM_MIDI_TRACKS 9

//A linked list storing anchors, each of which is a Set Tempo, Anchor or Time Signature event
struct dbAnchor
{
	unsigned long chartpos;	//The defined chart position of the anchor
	unsigned long BPM;	//The tempo of this beat times 1000 (0 for no change from previous anchor)
	unsigned char TSN;	//The numerator of this beat's time signature (0 for no change from previous anchor)
	unsigned char TSD;	//The denominator of this beat's time signature (4 is assumed if the .chart file is pre-Moonscraper and doesn't define this value, 0 for no change from previous anchor)
	unsigned long usec;	//The real time position of this anchor in microseconds (millionths of a second) (0 if not an anchor)
	struct dbAnchor *next;	//Pointer to the next anchor in the list
};

//A linked list strong text events
struct dbText
{
	unsigned long chartpos;
	char *text;
	struct dbText *next;
};

//A linked list storing notes for an instrument track
struct dbNote
{
	unsigned long chartpos;
	unsigned char gemcolor;
		//This is 0-4 for a note definition, 5 for inverted HOPO notation, 6 is for slider notation,
		// 7 is for open strum, 8 for B3 GHL note, 'S' for a start of solo section, 'E' for an end of solo section
		// '0' for a Player 1 section, '1' for a Player 2 section or '2' for a Star Power section
	unsigned long duration;
	char is_hopo;	//Set to nonzero if the note is less than 66/192 quarter notes from the previous note
	char is_slider;	//Set to nonzero if the note is within a slider marker
	char is_gem;	//Set to nonzero if the note is a gem instead of a status marker (is valued from 0-4, 7 or 8)
	struct dbNote *next;
};

//The structure representing one MIDI event generated during MIDI export
struct MIDIevent
{
	unsigned long pos;
	int type;
	int note;
	int velocity;
	int channel;
	char *dp;
	char on;				//Used for sorting
	char off;				//Used for sorting
	char sysexon;			//Used for sorting
	struct MIDIevent *next;
};

//A linked list of instrument tracks, each of which stores a linked list of notes
struct dbTrack
{
	char *trackname;
	unsigned char tracktype;	//1 for guitar, 2 for lead guitar, 3 for for bass, 4 for drums, 5 for rhythm, 6 for keyboard, 7 for GHL guitar, 8 for GHL bass, 0 for other (ie. ignored)
	char difftype;				//1 for easy, 2 for medium, 3 for hard, 4 for expert
	struct dbNote *notes;
	struct MIDIevent *events;	//The linked list of MIDI events created during MIDI export
	char isdrums;				//Nonzero if it is a drums track, 1 reflects the normal drum track and 2 reflects the double drums track
	struct dbTrack *next;

	///Remove these if found to be unused
	char isguitar;				//Nonzero if it is a guitar track, set to 2 if it is a GHL guitar track
	char isbass;				//Nonzero if it is a bass track, set to 2 if it is a GHL bass track
};

//The structure representing the contents of a Feedback chart.  Relevant tags are stored, as are linked lists for all
//anchors, text events and instrument tracks
struct FeedbackChart
{
	//Tags from the [Song] section
	double offset;				//The chart's offset in SECONDS
	unsigned long resolution;	//The number of ticks in a quarternote

	struct dbAnchor *anchors;	//Linked list of anchors
	struct dbText *events;		//Linked list of text events
	struct dbTrack *tracks;		//Linked list of note tracks

	///Remove these if found to be unused
	char guitartypes;	//Tracks the presence of 6 lane guitar (0 = no guitar parts, 1 = 5 lane guitar parts only, 2 = 6 lane guitar parts only, 3 = 5 and 6 lane guitar parts)
	char basstypes;		//Tracks the presence of 6 lane bass (0 = no bass parts, 1 = 5 lane bass parts only, 2 = 6 lane bass parts only, 3 = 5 and 6 lane bass parts)
};

extern char *midi_track_name[NUM_MIDI_TRACKS];	//The name of the MIDI track associated with each value of the dbTrack structure's tracktype variable
extern struct MIDIevent *midi_track_events[NUM_MIDI_TRACKS];	//Stores the linked list for each instrument, which is shared among the instrument's difficulties
extern char events_reallocated[NUM_MIDI_TRACKS];	//Specifies whether each of the MIDI events tracks were reallocated and will only need a single call to free()
extern clock_t start, end;	//Used to time how long the entire conversion takes on success
extern char **utf_argv;	//Used to store a copy of main()'s argument list converted to UTF-8 encoding, so the user can pass Unicode file parameters

///Various utilities
int build_utf8_argument_list(int argc, char **argv);
	//Builds utf_argv[] to store UTF8 formatted copies of the program's command line arguments
	//Returns nonzero on error
void *buffer_file(const char * fn, char appendnull);
	//Loads the specified file into a memory buffer and returns the buffer, or NULL upon error
	//If appendnull is nonzero, an extra 0 byte is written at the end of the buffer, potentially necessary if buffering a file that will be read as text, ensuring the buffer is NULL terminated
char *strcasestr_spec(char *str1,const char *str2);
	//Performs a case INSENSITIVE search of str2 in str1, returning the character AFTER the match in str1 if it exists, else NULL
char *duplicate_string(const char *str);
	//Allocates enough memory to copy the input string, copies it and returns the newly allocated string by reference
	//This function is provided to duplicate strings when the source string is a constant char array, eliminating
	//any applicable compiler warnings.  If str is NULL, NULL is returned.  If the allocation fails, NULL is returned.
char *truncate_string(char *str, char dealloc);
	//Creates and returns a duplicate of str without leading and trailing whitespace.
	//if dealloc is nonzero, str is de-allocated, so the calling function can overwrite the str pointer with the return value
	//Returns NULL on error
void sort_chart(struct FeedbackChart *chart);
	//Performs a bubble sort of the specified chart's linked lists, to ensure they will export to MIDI properly
	//Bubble sort isn't very efficient, but all notes are expected to be defined in order anyway so it shouldn't matter for properly made charts
int should_swap_events(struct MIDIevent *this_ptr, struct MIDIevent *next_ptr);
	//Compares the two MIDI events and returns nonzero if next_ptr should sort before this_ptr, used by insertion_sort_midi_events(()
	//Returns zero on error
void insertion_sort_midi_events(struct FeedbackChart *chart);
	//Insertion sorts the linked lists referenced in midi_track_events[], to ensure proper ordering during MIDI export
	//Resets the MIDI event linked list head pointers for each track in the chart, since new linked lists are created
	//The sort algorithm is optimized for mostly-sorted data that can be appended to the end of the sorted list that is created
void qsort_midi_events(struct FeedbackChart *chart);	//Optimization building a static array and quicksorting it
int qsort_midi_events_comparitor(const void * e1, const void * e2);	//A quicksort comparitor based on insertion_sort_midi_events(()
void set_hopo_status(struct FeedbackChart *chart);
	//Examines all tracks in the specified chart and sets the is_hopo variable where applicable for each note
	//First sets HOPO status based on note threshold, then inverts the HOPO status for all notes within toggle HOPO markers
	//Expects that the note linked lists have been sorted by sort_chart()
void set_slider_status(struct FeedbackChart *chart);
	//Examines all tracks in the specified chart and sets the is_slider variable where applicable for each note
	//Expects that the note linked lists have been sorted by sort_chart()

///Import functions
struct FeedbackChart *import_feedback(const char *filename);
	//Parses the specified file stream into a structure and returns it
	//Returns NULL on error
long int parse_long_int(char *buffer, unsigned long *startindex, int *errorstatus);
	//Takes a string and starting index and parses the string (ignoring leading whitespace) to find first numerical
	//character.  When a numerical character is found, leading zeroes are discarded and a numerical string is read
	//and converted to a long integer value, which is returned.  The index of the first character after the parsed
	//string is passed back into (*startindex) and should point to a whitespace character.
	//If errorstatus is NOT NULL, a nonzero number is instead stored into it and 0 is returned upon parse error
	//and the error is not printed to stdout
	//Note:  This function will return a negative value if a negative number is read from the input string
int read_db_string(char *source, char **str1, char **str2);
	//Scans the source string for a valid dB tag: text = text	or	text = "text"
	//The text to the left of the equal sign is returned through str1 as a new string, with whitespace truncated
	//The text to the right of the equal sign is returned through str2 as a new string, with whitespace truncated
	//If the first non whitespace character encountered after the equal sign is a quotation mark, all characters after
	//that quotation mark up to the next are returned through str2
	//Nonzero is returned upon success, or zero is returned if source did not contain two sets of non whitespace characters
	//separated by an equal sign character, if the closing quotation mark is missing or memory allocation fails.
int validate_db_track_diff_string(char *diffstring, struct dbTrack *track);
	//Compares the input string against all supported track difficulty names, setting values in *track (ie. tracktype, isguitar, isdrums) accordingly
	//track->tracktype will be set to 0 if the string is a known instrument difficulty but isn't to be imported
	//Returns 0 on error or if the string doesn't reflect a supported name
struct dbTrack *validate_db_instrument(char *buffer);
	//Validates that buffer contains a valid dB instrument track name enclosed in brackets []
	//buffer is expected to point to the opening bracket
	//If it is valid, a dbTrack structure is allocated and initialized:
	//(track name is allocated, tracktype and difftype are set and the linked lists are set to NULL)
	//The track structure is returned, otherwise NULL is returned if the string did not contain a valid
	//track name.  buffer[] is modified to remove any whitespace after the closing bracket
void destroy_feedback_chart(struct FeedbackChart *ptr);
	//Releases memory used by the CONTENTS of the passed FeedbackChart, including that of the contained linked lists


///Export functions
int export_midi(const char *filename, struct FeedbackChart *chart);
	//Writes the data in the chart to a MIDI file of the specified name
	//returns nonzero on error
int write_word_be(unsigned short data, PACKFILE *outf);
	//Writes a two byte value to the file in big endian order
	//Returns nonzero upon error
int write_dword_be(unsigned long data, PACKFILE *outf);
	//Writes a four byte value to the file in big endian order
	//Returns nonzero upon error
int write_var_length(unsigned long data, PACKFILE *outf);
	//Writes a variable length value to the file
	//Returns nonzero upon error
int dump_midi_track(const char *inputfile, PACKFILE *outf);
	//Writes a MIDI track header to the output file, followed by the size of the input file, followed by the contents of the input file
	//Returns nonzero upon error
int write_string_meta_event(unsigned char type, const char *str, PACKFILE *outf);
	//Writes a meta MIDI event of the specified type (ie. 1 for text event, 3 for track name, 5 for lyric event) to the given file stream
	//This involves writing the event, the string length and then the string
	//Returns nonzero upon error
struct MIDIevent *append_midi_event(struct dbTrack *track);
	//Appends a new event structure to the specified track's MIDI event linked list
	//Returns the pointer to the new structure, or NULL upon error
int add_midi_event(struct dbTrack *track, unsigned long pos, int type, int note, int velocity, int channel);
	//Appends a new event with the provided values to the specified track's MIDI event linked list
	//Returns nonzero upon error
int add_midi_lyric_event(struct dbTrack *track, unsigned long pos, char *text);
	//Appends a new lyric event with the provided values to the specified track's MIDI event linked list
	//A newly allocated copy of the specified text is stored in the new link
	//Returns nonzero upon error
int add_midi_text_event(struct dbTrack *track, unsigned long pos, char *text);
	//Appends a new text event with the provided values to the specified track's MIDI event linked list
	//A newly allocated copy of the specified text is stored in the new link
	//Returns nonzero upon error
int add_midi_sysex_event(struct dbTrack *track, unsigned long pos, int size, void *data, char sysexon);
	//Appends a new Sysex event with the provided values to the specified track's MIDI event linked list
	//A newly allocated copy of the specified data is stored in the new link
	//sysexon tracks whether it is a start of Sysex marker, for sorting purposes
	//Returns nonzero upon error
