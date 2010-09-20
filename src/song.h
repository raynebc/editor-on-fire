#ifndef EOF_SONG_H
#define EOF_SONG_H

#include <allegro.h>

#define EOF_OLD_MAX_NOTES     65536
#define EOF_MAX_NOTES         32768
#define EOF_MAX_LYRICS EOF_MAX_NOTES
#define EOF_MAX_LYRIC_LINES    4096
#define EOF_MAX_LYRIC_LENGTH    255
#define EOF_MAX_TRACKS            5
#define EOF_MAX_CATALOG_ENTRIES 256
#define EOF_MAX_INI_SETTINGS     32
#define EOF_MAX_BOOKMARK_ENTRIES 10

#define EOF_NOTE_SUPAEASY 0
#define EOF_NOTE_EASY     1
#define EOF_NOTE_MEDIUM   2
#define EOF_NOTE_AMAZING  3
#define EOF_NOTE_SPECIAL  4

#define EOF_TRACK_GUITAR      0
#define EOF_TRACK_BASS        1
#define EOF_TRACK_GUITAR_COOP 2
#define EOF_TRACK_RHYTHM      3
#define EOF_TRACK_DRUM        4
#define EOF_TRACK_VOCALS      5
#define EOF_TRACK_MAX         5

#define EOF_NOTE_FLAG_HOPO     1
#define EOF_NOTE_FLAG_SP       2
#define EOF_NOTE_FLAG_CRAZY    4
#define EOF_NOTE_FLAG_F_HOPO   8
#define EOF_NOTE_FLAG_NO_HOPO 16

#define EOF_MAX_BEATS 32768

#define EOF_MAX_SOLOS      32

#define EOF_MAX_STAR_POWER 32

#define EOF_BEAT_FLAG_ANCHOR      1
#define EOF_BEAT_FLAG_EVENTS      2
#define EOF_BEAT_FLAG_START_4_4   4
#define EOF_BEAT_FLAG_START_3_4   8
#define EOF_BEAT_FLAG_START_5_4  16
#define EOF_BEAT_FLAG_START_6_4  32
#define EOF_BEAT_FLAG_CORRUPTED  64

#define EOF_LYRIC_LINE_FLAG_OVERDRIVE 1

#define EOF_MAX_TEXT_EVENTS 4096

#define EOF_DEFAULT_TIME_DIVISION 480 // default time division used to convert midi_pos to msec_pos

typedef struct
{

	char          type;
	char          note;
	unsigned long midi_pos;
	long          midi_length;
	unsigned long pos;
	long          length;
	char          flags;

} EOF_NOTE;

typedef struct
{

	char          type;
	char          note;
	short         endbeat;
	short         beat;       // which beat this note was copied from
	unsigned long pos;
	long          length;
	unsigned long midi_pos;
	long          midi_length;
	float         porpos;     // position of note within the beat (100.0 = full beat)
	float         porendpos;
	char          active;

} EOF_EXTENDED_NOTE;

typedef struct
{

	char          note;		// if zero, the lyric has no defined pitch
	char          text[EOF_MAX_LYRIC_LENGTH+1];
	unsigned long midi_pos;
	long          midi_length;
	unsigned long pos;
	long          length;

} EOF_LYRIC;

typedef struct
{

	char          note;
	char          text[EOF_MAX_LYRIC_LENGTH+1];
	unsigned long midi_pos;
	long          midi_length;
	unsigned long pos;
	long          length;

	short         beat;       // which beat this note was copied from
	short         endbeat;    // which beat this note was copied from
	float         porpos;     // position of note within the beat (100.0 = full beat)
	float         porendpos;

} EOF_EXTENDED_LYRIC;

typedef struct
{

	int midi_start_pos;
	int midi_end_pos;
	int start_pos;
	int end_pos;
	int flags;

} EOF_LYRIC_LINE;

typedef struct
{

	int midi_start_pos;
	int midi_end_pos;
	int start_pos;
	int end_pos;

} EOF_SOLO_ENTRY;

typedef struct
{

	int midi_start_pos;
	int midi_end_pos;
	int start_pos;
	int end_pos;

} EOF_STAR_POWER_ENTRY;

typedef struct
{

	EOF_NOTE * note[EOF_MAX_NOTES];
	unsigned long notes;

	/* solos */
	EOF_SOLO_ENTRY solo[EOF_MAX_SOLOS];
	short solos;

	/* star power */
	EOF_STAR_POWER_ENTRY star_power_path[EOF_MAX_STAR_POWER];
	short star_power_paths;

} EOF_TRACK;

typedef struct
{

	EOF_LYRIC * lyric[EOF_MAX_LYRICS];
	unsigned long lyrics;

	/* lyric lines */
	EOF_LYRIC_LINE line[EOF_MAX_LYRIC_LINES];
	short lines;

} EOF_VOCAL_TRACK;

typedef struct
{

	unsigned long ppqn;
	unsigned long midi_pos;
	unsigned long pos;
	unsigned long flags;

	double fpos;

} EOF_BEAT_MARKER;

typedef struct
{

	char filename[256];
	int  midi_offset;

} EOF_OGG_INFO;

typedef struct
{

	char          artist[256];
	char          title[256];
	char          frettist[256];
	char          year[32];
	char          loading_text[512];
	char          lyrics;
	char          eighth_note_hopo;

	EOF_OGG_INFO  ogg[8];
	short         oggs;

	char          ini_setting[EOF_MAX_INI_SETTINGS][512];
	short         ini_settings;

	unsigned long revision;

} EOF_SONG_TAGS;

typedef struct
{

	char track;
	char type;
	int start_pos;
	int end_pos;

} EOF_CATALOG_ENTRY;

typedef struct
{

	char text[256];
	int beat;

} EOF_TEXT_EVENT;

typedef struct
{

	EOF_CATALOG_ENTRY entry[EOF_MAX_CATALOG_ENTRIES];
	int entries;

} EOF_CATALOG;

typedef struct
{

	/* song info */
	EOF_SONG_TAGS * tags;

	/* MIDI "resolution" used to determine how MIDI is exported,
	 * when importing we should store the value from the source file here to
	 * simplify import and to minimize changes made to the file upon export */
	int resolution;

	/* track data */
	EOF_TRACK       * track[EOF_MAX_TRACKS];
	EOF_VOCAL_TRACK * vocal_track;

	EOF_BEAT_MARKER * beat[EOF_MAX_BEATS];
	int beats;

	EOF_TEXT_EVENT * text_event[EOF_MAX_TEXT_EVENTS];
	int text_events;

	/* miscellaneous */
	int bookmark_pos[EOF_MAX_BOOKMARK_ENTRIES];
	EOF_CATALOG * catalog;

} EOF_SONG;

struct waveformslice
{
	unsigned min;	//The lowest amplitude for the samples
	unsigned peak;	//The peak amplitude for the samples
	double rms;	//The root mean square for the samples
};

struct wavestruct
{
	char *oggfilename;
	unsigned long slicelength;		//The length of one slice of the graph in milliseconds
	unsigned long slicesize;		//The number of samples in one slice of the graph
	unsigned long numslices;		//The number of waveform structures in the arrays below
	char is_stereo;					//This OGG has two audio channels
	struct waveformslice *slices;	//The waveform data for the only channel (mono) or left channel (stereo)
	unsigned int maxamp;			//The highest amplitude of all mono/left channel samples in the audio file
	unsigned long yaxis;			//The y coordinate representing the y axis the mono/left channel graph will render to
	unsigned long height;			//The height of the mono/left channel graph
	struct waveformslice *slices2;	//The waveform data for the right channel (stereo only), will be NULL unless the OGG was in stereo
	unsigned int maxamp2;			//The highest amplitude of all right channel samples in the audio file
	unsigned long yaxis2;			//The y coordinate representing the y axis the right channel graph will render to
	unsigned long height2;			//The height of the right channel graph
	unsigned int zeroamp;			//The amplitude representing a 0 amplitude for this waveform (32768 for 16 bit audio samples, 128 for 8 bit audio samples)
};

EOF_SONG * eof_create_song(void);	//Allocates, initializes and returns an EOF_SONG structure
void eof_destroy_song(EOF_SONG * sp);	//De-allocates the memory used by the EOF_SONG structure
int eof_load_song_pf(EOF_SONG * sp, PACKFILE * fp);	//Loads data from the specified PACKFILE pointer into the given EOF_SONG structure (called by eof_load_song())
EOF_SONG * eof_load_song(const char * fn);	//Loads the specified EOF file, validating the file header and loading the appropriate OGG file
int eof_save_song(EOF_SONG * sp, const char * fn);	//Saves the song to file

EOF_NOTE * eof_track_add_note(EOF_TRACK * tp);	//Allocates, initializes and stores a new EOF_NOTE structure into the notes array.  Returns the newly allocated structure or NULL upon error
void eof_track_delete_note(EOF_TRACK * tp, int note);	//Removes and frees the specified note from the notes array.  All notes after the deleted note are moved back in the array one position
void eof_track_sort_notes(EOF_TRACK * tp);	//Performs a quicksort of the notes array
int eof_song_qsort_notes(const void * e1, const void * e2);	//The comparitor function used to quicksort the notes array
int eof_fixup_next_note(EOF_TRACK * tp, int note);	//Returns the note one after the specified note number that is in the same difficulty, or -1 if there is none
void eof_track_find_crazy_notes(EOF_TRACK * tp);	//Used during MIDI import to mark a note as "crazy" if it overlaps with the next note in the same difficulty
void eof_track_fixup_notes(EOF_TRACK * tp, int sel);	//Performs cleanup of the specified instrument track
void eof_track_resize(EOF_TRACK * tp, int notes);	//Grows or shrinks the notes array to fit the specified number of notes by allocating/freeing EOF_NOTE structures
void eof_track_add_star_power(EOF_TRACK * tp, unsigned long start_pos, unsigned long end_pos);	//Adds a star power phrase at the specified start and stop timestamp for the specified track
void eof_track_delete_star_power(EOF_TRACK * tp, int index);	//Deletes the specified star power phrase and moves all phrases that follow back in the array one position
void eof_track_add_solo(EOF_TRACK * tp, unsigned long start_pos, unsigned long end_pos);	//Adds a solo phrase at the specified start and stop timestamp for the specified track
void eof_track_delete_solo(EOF_TRACK * tp, int index);	//Deletes the specified solo phrase and moves all phrases that follow back in the array one position

EOF_LYRIC * eof_vocal_track_add_lyric(EOF_VOCAL_TRACK * tp);	//Allocates, initializes and stores a new EOF_LYRIC structure into the lyrics array.  Returns the newly allocated structure or NULL upon error
void eof_vocal_track_delete_lyric(EOF_VOCAL_TRACK * tp, int lyric);	//Removes and frees the specified lyric from the lyrics array.  All lyrics after the deleted lyric are moved back in the array one position
void eof_vocal_track_sort_lyrics(EOF_VOCAL_TRACK * tp);		//Performs a quicksort of the lyrics array
int eof_song_qsort_lyrics(const void * e1, const void * e2);	//The comparitor function used to quicksort the lyrics array
int eof_fixup_next_lyric(EOF_VOCAL_TRACK * tp, int lyric);	//Returns the next lyric, or -1 if there is none
void eof_vocal_track_fixup_lyrics(EOF_VOCAL_TRACK * tp, int sel);	//Performs cleanup of the specified lyric track
void eof_vocal_track_resize(EOF_VOCAL_TRACK * tp, int lyrics);	//Grows or shrinks the lyrics array to fit the specified number of notes by allocating/freeing EOF_LYRIC structures
void eof_vocal_track_add_line(EOF_VOCAL_TRACK * tp, unsigned long start_pos, unsigned long end_pos);	//Adds a lyric phrase at the specified start and stop timestamp for the specified track
void eof_vocal_track_delete_line(EOF_VOCAL_TRACK * tp, int index);	//Deletes the specified lyric phrase and moves all phrases that follow back in the array one position

EOF_BEAT_MARKER * eof_song_add_beat(EOF_SONG * sp);	//Allocates, initializes and stores a new EOF_BEAT_MARKER structure into the beats array.  Returns the newly allocated structure or NULL upon error
void eof_song_delete_beat(EOF_SONG * sp, int beat);	//Removes and frees the specified beat from the beats array.  All beats after the deleted beat are moved back in the array one position
int eof_song_resize_beats(EOF_SONG * sp, int beats);	//Grows or shrinks the beats array to fit the specified number of beats by allocating/freeing EOF_BEAT_MARKER structures.  Returns nonzero on success

void eof_song_add_text_event(EOF_SONG * sp, int beat, char * text);	//Allocates, initializes and stores a new EOF_TEXT_EVENT structure into the text_event array.  Returns the newly allocated structure or NULL upon error
void eof_song_delete_text_event(EOF_SONG * sp, int event);	//Removes and frees the specified text event from the text_events array.  All text events after the deleted text event are moved back in the array one position
void eof_song_move_text_events(EOF_SONG * sp, int beat, int offset);	//Displaces all beats starting with the specified beat by the given additive offset.  Each affected beat's flags are cleared except for the anchor and text event(s) present flags
void eof_song_resize_text_events(EOF_SONG * sp, int events);	//Grows or shrinks the text events array to fit the specified number of notes by allocating/freeing EOF_LYRIC structures
void eof_sort_events(void);	//Performs a quicksort of the events array
int eof_song_qsort_events(const void * e1, const void * e2);	//The comparitor function used to quicksort the events array

void eof_sort_notes(void);	//Sorts the notes in all tracks
void eof_fixup_notes(void);	//Performs cleanup of the note selection, beats and all tracks
void eof_detect_difficulties(EOF_SONG * sp);	//Sets the populated status by prefixing each populated tracks' difficulty name (stored in eof_note_type_name[] and eof_vocal_tab_name[]) with an asterisk

struct wavestruct *eof_create_waveform(char *oggfilename,unsigned long slicelength);
	//Decompresses the specified OGG file into memory and creates waveform data
	//slicelength is the length of one waveform graph slice in milliseconds
	//The correct number of samples are used to represent each column (slice) of the graph
	//The waveform data is returned, otherwise NULL is returned upon error

int eof_process_next_waveform_slice(struct wavestruct *waveform,SAMPLE *audio,unsigned long slicenum);
	//Processes waveform->slicesize number of audio samples, or if there are not enough, the remainder of the samples, storing the peak amplitude and RMS into waveform->slices[slicenum]
	//If the audio is stereo, the data for the right channel is likewise processed and stored into waveform->slices2[slicenum]
	//Returns 0 on success, 1 when all samples are exhausted or -1 on error

int eof_is_freestyle(char *ptr);
	//Returns 1 if the specified lyric contains a freestyle character (# or ^)
	//Returns 0 if the specified lyric contains no freestyle character
	//Returns -1 on error, such as if the pointer is NULL

int eof_lyric_is_freestyle(EOF_VOCAL_TRACK * tp, unsigned long lyricnumber);
	//Returns 1 if the specified lyric contains a freestyle character (# or ^)
	//Returns 0 if the specified lyric contains no freestyle character
	//Returns -1 on error, such as if the specified lyric does not exist

void eof_fix_lyric(EOF_VOCAL_TRACK * tp, unsigned long lyricnumber);
	//Ensures that the string is properly terminated and any existing freestyle character is placed properly at the end of the string
	//Should be called for each lyric before the MIDI is written

void eof_set_freestyle(char *ptr, char status);
	//Rewrites the lyric string, which will end with a # if status is nonzero

void eof_toggle_freestyle(EOF_VOCAL_TRACK * tp, unsigned long lyricnumber);
	//Makes a lyric freestyle if it isn't already and vice versa

int eof_song_tick_to_msec(EOF_SONG * sp, int track, unsigned long tick); // convert tick value to real time
int eof_song_msec_to_tick(EOF_SONG * sp, int track, unsigned long msec); // convert real time value to tick

#endif
