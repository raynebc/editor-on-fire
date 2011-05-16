#ifndef _midi_parse_h_
#define _midi_parse_h_

#include "Lyric_storage.h"	//Include PROGVERSION macro

//These two macros are used with Write_MIDI_Note()
#define MIDI_NOTE_ON 0x9
#define MIDI_NOTE_OFF 0x8

//This macro defines the velocity at which MIDI Note On events will use (value from 0 to 127)
#define MIDI_VELOCITY 127

//These macros are used with WriteMIDIString()
#define TEXT_MIDI_STRING 0x1
#define TRACK_NAME_MIDI_STRING 0x3
#define LYRIC_MIDI_STRING 0x5
#define SEQSPEC_MIDI_STRING 0x7F


//
//STRUCTURE DEFINITIONS
//
struct _MIDI_LYRIC_STRUCT_{
	struct MIDI_Lyric_Piece *head;
	struct MIDI_Lyric_Piece *tail;
};

struct Track_chunk{
	long fileposition;			//The file position of this track, beginning with the header
	unsigned long tracknum;		//The number of the track
	char *trackname;			//The name of the track
	unsigned long chunksize;	//The size of this track's chunk	(4 bytes)
	unsigned long textcount;	//The number of text events in this track (that don't begin with '[')
	unsigned long lyrcount;		//The number of lyric events in this track (that don't begin with '[')
	unsigned long spacecount;	//The number of lyric events that had whitespace
	unsigned long notecount;	//The number of Note On events in this track
};

struct Tempo_change{
	unsigned long delta;		//The delta time since last tempo change
	double realtime;			//The realtime of this tempo change in milliseconds
	double BPM;					//The BPM associated with this tempo change
	unsigned TS_num;			//The numerator of the Time Signature in effect at this tempo change
	unsigned TS_den;			//The numerator of the Time Signature in effect at this tempo change
	struct Tempo_change *next;	//Pointer to next tempo change structure
};	//This structure is used in a singly-linked list containing all tempo changes, and the delta times between them.
	//It will be used to find the realtime of events outside of track 0, which is generally the only track that
	//contains Set Tempo events

struct Header_chunk{
	unsigned long chunksize;	//This should always be 6: #of bytes needed to pass the end of chunk	(4 bytes)
	unsigned short formattype;	//0,1 or 2 (Type 0 is a single track midi, FoF uses type 1)		(2 bytes)
	unsigned short numtracks;	//The number of tracks.  For a type 0 midi, this is always 1		(2 bytes)
	unsigned short division;	//Time division used to decode track event delta times into real time	(2 bytes)
					//If most significant bit is 0, the remaining 15 bits define the
					//time division in ticks per beat (AKA pulses per quarter note or PPQN)
	struct Track_chunk *tracks;		//A pointer to an array of numtracks # of Track header structures
	struct Tempo_change *tempomap;	//A linked list of tempo change information
	struct Tempo_change *curtempo;	//The conductor of the linked list, pointing to the current tempo information
};

struct MIDI_Lyric_Piece{
	char *lyric;			//The string from the Lyric/Text event.  If this string is NULL, then it is treated as a line break
	unsigned char pitch;	//Pitch of the MIDI Note on associated with the Lyric, a value of PITCHLESS denotes a lyric without a defined pitch
	unsigned long start;	//Starting timestamp of the Lyric event
	unsigned long end;		//Ending timestamp of the Lyric event
	char groupswithnext;	//Grouping status (used for KAR import)
	char overdrive_on;		//Boolean: Overdrive is applied to this lyric
	struct MIDI_Lyric_Piece *prev;
	struct MIDI_Lyric_Piece *next;
};

struct _MIDISTRUCT_{
	struct Header_chunk hchunk;	//Store the MIDI header information in this structure
	char MPQN_defined;			//Boolean:  Defines whether a Set Tempo event has been reached
	unsigned long deltacounter;	//This will count the delta clocks in between tempo changes
	unsigned long absdelta;		//This will count the delta clocks in between track changes
	unsigned long endtime;		//The timestamp of the end of the MIDI file in milliseconds
	volatile double realtime;	//At each tempo change, the deltacounter will be used to determine the current time
	unsigned short trackswritten;	//The current number of MIDI tracks that have been written (for MIDI export formats such as MIDI, Vrhythm, KAR)
									//This value should be updated by any functions that write tracks to the output file
	double BPM;					//Default BPM of 120BPM is assumed until MPQN is defined
	unsigned TS_num;			//The numerator of the current time signature (defaults to 4)
	unsigned TS_den;			//The denominator of the current time signature (defaults to 4)

	unsigned char diff_lo,diff_hi;
		//The lower and upper boundaries for which range of MIDI notes to parse, based on the indicated difficulty
		//These variables are set by AnalyzeVrhythmID()
		//	These are only used for vocal rhythm import
	char mixedtrack;
		//Boolean: The instrument track containing vocal rhythm also contains normal instrument notes.  If this is TRUE,
		// then the track will need to be rebuilt during a vocal rhythm to MIDI conversion
	char *unfinalizedlyric;
		//For importing pitchless MIDI lyrics, the Lyric_Handler() won't add it to the MIDI Lyics list unless another Lyric event or Note On is reached
		//If the last lyric in the imported track is pitchless, neither of these conditions occur, so this string will be non NULL if there was a lyric
		//that was parsed and not handled in this circumstance.  It would need to be manually checked for after import and finalized
		//This pointer is reset to NULL when the lyric is added to the MIDI lyric list
	unsigned long unfinalizedlyrictime;
		//This is the timestamp that corresponds to unfinalizedlyric
	char unfinalizedoverdrive_on;
		//This is the overdrive status that corresponds to unfinalizedlyric
	char unfinalizedgroupswithnext;
		//This is the grouping status that corresponds to unfinalizedlyric
	char miditype;
		//If 1, the input file is a RIFF-MIDI (a SMF format file enclosed in an RIFF header)
		//If 2, the input file is a Rock Band Audition file (SMF embedded within a binary file)
};

struct TEPstruct	//This is structure containing all variables utilized within the track event processor
{					//A pointer to this will be passed to an event handling function
	unsigned char eventtype;		//Stores the MIDI event type in the upper half (and MIDI channel in the lower half)
	unsigned char lasteventtype;	//Stores the last event, for use with Running Status bytes
	unsigned char lastwritteneventtype;	//Stores the last event that was written, for use with Running Status bytes
	unsigned char runningstatus;	//Boolean:  Running status is in effect
	unsigned char m_eventtype;		//Stores the meta event type if the current event is a meta event
	char *trackname;				//Stores the name of the current track, if provided
	char *buffer;					//Used to store pointers to temporary strings when reading events (lyric/text event string)
	unsigned long tracknum;			//The track number being processed
	unsigned long delta;			//Stores the variable length delta value of the current event
	unsigned long varlen;			//unencoded variable length value (ie. for meta event length)
	unsigned long length;			//stores the length of a meta event
	unsigned long current_MPQN;		//stores the current MPQN value
	unsigned long processed;		//Counter for the number of processed events
	unsigned char parameters[4];	//Used to read in the parameters for MIDI events
	FILE *inf;						//The input file pointer
	FILE *outf;						//This is used in the MIDI_Build_handler, which writes to output file
	long int startindex;			//The file stream index of the DELTA TIME for the processed event
	long int eventindex;			//The file stream index of the MIDI EVENT following the delta time for the processed event
	long int endindex;				//The file stream index of the DELTA TIME for the next event to process.  Endindex
									//is not populated for the event handler unless the handler is called after parsing
									//the event
};

//
//GENERAL IMPORT AND EXPORT FUNCTIONS
//
void InitMIDI(void);
	//Initialize variables in the MIDI structure
void MIDI_Load(FILE *inf,int (*event_handler)(struct TEPstruct *data),char suppress_errors);
	//Perform all code necessary to load a MIDI.  If event_handler is a pointer to Lyric_handler, the handler also
	//loads lyrics into the Lyric structure.  If NULL is provided, only the MIDI structure is populated (tempo map and track information)
	//The suppress_errors variable is passed to TrackEventProcessor()
void Parse_Song_Ini(char *midipath,char loadoffset,char loadothertags);
	//Accepts the full path to the RB MIDI, and looks for song.ini at that path.  If found, tags are loaded as is appropriate
	//If loadoffset is nonzero, then the offset tag is loaded from the song.ini file
	//If loadothertags is nonzero, then the other tags are loaded.  This is to allow the offset to be loaded by itself without
	//affecting import tags when a source MIDI provided
void VRhythm_Load(char *srclyrname,char *srcmidiname,FILE *inf);
	//srclyrname is the file name of the input pitched lyrics file
	//srcmidiname is the file name of the input vocal rhythm file (so that the appropriate song.ini can be parsed)
	//inf is a file handle to the MIDI containing the vocal rhythm
void PitchedLyric_Load(FILE *inf);
	//A modified version of UStar_Load designed to load lyric files containing pitches and lyrics only (no timing)
	//This function requires that the Lyrics structure is already populated
void ReleaseMIDI(void);
	//Releases memory from the MIDI structure, to be called after MIDI_Load and VRhythm_Load
void Export_MIDI(FILE *outf);
	//A MIDI track is written with lyric and note events based on the contents of the Lyrics structure
	//Exporting either a RB style MIDI or an unofficial KAR file is supported and controlled by the value of Lyric.out_format
void Export_Vrhythm(FILE *outmidi,FILE *outlyric,char *vrhythmid);
	//Exports in vocal rhythm format.  MIDI notes are written to outmidi in the track and note range identified in vrhythmid, following the same usage as vrhythm import
	//Lyrics and pitches are written to outlyric
void Export_SKAR(FILE *outf);
	//Exports in Soft Karaoke format


//
//GENERAL MIDI READING FUNCTIONS
//
void ReadMIDIHeader(FILE *inf,char suppress_errors);
	//Loads and validates the MIDI header.  If verbose is nonzero, outputs values
	//If suppress_errors is nonzero, error messages are not displayed to stdout
int ReadTrackHeader(FILE *inf,struct Track_chunk *tchunk);
	//Loads and validates a track header, returns 1 if EOF is encountered reading the next byte in the file, meaning the file cleanly ended.
	//returns -1 if the track header is invalid or 0 if the track header was successfully parsed
	//tchunk is a pointer to a track header structure stored within hchunk.tracks
int ReadVarLength(FILE *inf, unsigned long *ptr);
	//Read variable length value into *ptr performing bit shifting accordingly
	//Returns the number of bytes read from the input file, or zero upon error (invalid VLV)
char *ReadMetaEventString(FILE *inf,long int length);
	//Allocates an array of length+1 and reads the ASCII string of the specified length The pointer to the array is returned


//
//GENERAL MIDI WRITING FUNCTIONS
//
void WriteVarLength(FILE *outf, unsigned long value);
	//Write variable length value to output file.  Returns zero on success, or 1 if value is too large to be written in a 4 byte VLV
void CopyTrack(FILE *inf,unsigned short tracknum,FILE *outf);
	//Copies the specified MIDI track to the output file stream.  Expects the MIDI structure to have been populated by parsing the input MIDI.
void WriteMIDIString(FILE *outf,unsigned long delta,int stringtype, const char *str);
	//Writes the specified delta value (if writedelta is nonzero),
	//followed by the specified string type (Text, Trackname, Lyric, Seq. Specific) to the output MIDI file
void Write_MIDI_Note(unsigned int notenum,unsigned int channelnum,unsigned int notestatus,FILE *outf,char skipstatusbyte);
	//Writes the given note number with status MIDI_NOTE_ON or MIDI_NOTE_OFF to the file stream given
	//Note Off events will be written as Note On events with a velocity of 0 (to save space with running status)
	//If skipstatusbyte is nonzero, then the status byte (MIDI_NOTE_ON) will not be written (used in running status logic)
long int Write_MIDI_Track_Header(FILE *outf);
	//Writes a MIDI track header with null data for the chunk size.  The file position of the chunk size is returned for calculating and writing the chunk size afterward
void Write_Default_Track_Zero(FILE *outmidi);
	//Writes a MIDI file header specifying 1 track, writes a track for track 0 and sets MIDIstruct.hchunk.division and MIDIstruct.hchunk.tempomap
	//After calling this, the MIDI variables are configured and ready to write the export track
void Copy_Source_MIDI(FILE *inf,FILE *outf);
	//Copy all tracks besides destination vocal track(s) to output file.  Should only be called if a source MIDI was provided
void Write_Tempo_Change(double BPM,FILE *outf);
	//Writes Meta event 81, specifying the MPQN (milliseconds per quarter note), where MPQN = MICROSECONDS_PER_MINUTE/BPM = 60000000/BPM


//
//MIDI TRACK PARSING AND EVENT HANDLERS
//
unsigned long TrackEventProcessor(FILE *inf,FILE *outf,unsigned char break_on,char ismeta,int (*event_handler)(struct TEPstruct *data),char callbefore,struct Track_chunk *tchunk,char suppress_errors);
	//Process events within MIDI track at current file position in inf, documenting to stdout if verbose is enabled.  If event
	//number break_on is read and ismeta equals 0, program returns with value 0.  If meta event type break_on is read and
	//ismeta equals 1, program returns with value 0.  Otherwise the number of processed events is returned.
	//A break_on value of 0x1 and ismeta value of 0 can be passed to process the entire track, as there is no MIDI event 0x1
	// This function takes a pointer to a function that uses a TEPstruct struct pointer as a parameter and returns an integer run status:
	//	-1 upon error, 0 upon event ignored, 1 upon event successfully handled
	//If callbefore is nonzero, the handler is called before the event is parsed further than its status byte, otherwise it is called
	//after the event is parsed and read into the TEPstruct.  If the function pointer is NULL, no handler is called.
	//tchunk is a pointer to the track chunk structure in the MIDI structure for this track, providing the handler and event
	//processor access to information such as the track name, number, etc.
	//If suppress_errors is nonzero, then MIDI formatting errors are not printed to stdout
int Lyricless_handler(struct TEPstruct *data);
	//New lyric handler to load vocal rhythm from MIDIs without lyrics
	//If data is passed as NULL, NULL is returned
int MIDI_Build_handler(struct TEPstruct *data);
	//This handler will copy the input MIDI track to the output file, except for note on and off events that are in the vocal rhythm instrument difficulty
	//(as defined in the pitched lyric file).  This will be performed by examining each event, and copying all data within the event if it
	//isn't in the specified difficulty.  Otherwise the delta value of the vocal rhythm event is added to the next event that is parsed.
	//The handler does not check the track name, so it is expected that this handler will only be used on the MIDI track that is to be filtered.
int SKAR_handler(struct TEPstruct *data);
	//This handler loads a standard "Soft Karaoke" KAR file, which stores lyrics as text events in a track called "Words"
int Lyric_handler(struct TEPstruct *data);
	//This is the new KAR_pitch_handler logic that is designed to handle both RB and KAR MIDIs
int MIDI_Stats(struct TEPstruct *data);
	//Tracks the following statistical information for the track: notecount, textcount, lyrcount and spacecount


//
//MIDI TIMING CONVERSION FUNCTIONS
//
double ConvertToRealTime(unsigned long absolutedelta,double starttime);
	//This function expects the Tempo linked list to have been fully populated by reading Track 0.
	//absolutedelta is a delta counter that will be converted to realtime starting at the tempo
	//change defined in starttime.  If starttime is 0, all tempo changes are checked starting with
	//the first, otherwise tempo changes are checked starting with the one specified
	//starttime should be provided as 0 if absolutedelta is absolute within the entire track
unsigned long ConvertToDeltaTime(unsigned long starttime);
	//Uses the Tempo Changes list to calculate the absolute delta time of the specified realtime.


//
//GENERAL LYRIC HANDLING FUNCTIONS
//
void AddMIDILyric(char *str,unsigned long start,unsigned char pitch,char isoverdrive,char groupswithnext);
	//Adds the Lyric and Note On information to the linked list.  The original string is stored in the list and returned by RemoveMIDILyric()
	//if isoverdrive is nonzero, the piece is remembered to have Overdrive applied when added to the Lyrics structure
	//if pitch is set to PITCHLESS, then the lyric is treated as having no defined pitch
	//The grouping status is stored as given, but is only relevant during KAR import
struct MIDI_Lyric_Piece *EndMIDILyric(unsigned char pitch,unsigned long end);
	//Searches for the first incomplete (missing end timestamp) MIDI Lyric list entry with a matching pitch and writes the ending timestamp
	//This allows an indefinite number of Lyrics to be appended and when they are ended, they can be processed in the proper order (First In, First Out)
int GetLyricPiece(void);
	//If the first link in the MIDI lyrics list is completed, it is removed from the list, added to the Lyrics list
	//1 is returned if a lyric piece is extracted
	//2 is returned if a line break is extracted
	//Otherwise zero is returned if nothing was extracted
char *AnalyzeVrhythmID(const char *buffer);
	//Accepts a string to what follows "MIDI=" in the pitched lyric file or what is provided via command line for vrhythm export (or lyricless vrhythm import)
	//The string is expected to be two characters, the first of which is a letter identifing the instrument track, the second a number identifying the difficulty
	//The returned string is the name of the MIDI track pertaining to the given identifier, or NULL if the ID string is invalid
	//MIDIstruct.diff_lo and MIDIstruct.diff_hi are set for the upper and lower boundaries for the acceptable instrument notes for the vrhythm import/export
	//If buffer is passed as NULL, NULL is returned


//
//EXTERNAL GLOBAL VARIABLE DEFINITIONS
//
extern struct _MIDISTRUCT_ MIDIstruct;			//This structure is used for MIDI/KAR/Vrhythm import
extern struct _MIDI_LYRIC_STRUCT_ MIDI_Lyrics;	//This structure is used for MIDI/KAR lyric import


#endif //#ifndef _midi_parse_h_
