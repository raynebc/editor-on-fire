#ifndef EOF_SONG_H
#define EOF_SONG_H

#include <allegro.h>

#define EOF_OLD_MAX_NOTES     65536
#define EOF_MAX_NOTES         32768
#define EOF_MAX_LYRICS EOF_MAX_NOTES
#define EOF_MAX_LYRIC_LINES    4096
#define EOF_MAX_LYRIC_LENGTH    255
#define EOF_MAX_CATALOG_ENTRIES 256
#define EOF_MAX_INI_SETTINGS     32
#define EOF_MAX_BOOKMARK_ENTRIES 10

#define EOF_NOTE_SUPAEASY    0
#define EOF_NOTE_EASY        1
#define EOF_NOTE_MEDIUM      2
#define EOF_NOTE_AMAZING     3
#define EOF_MAX_DIFFICULTIES 4
#define EOF_NOTE_SPECIAL     4

#define EOF_TRACK_GUITAR      0
#define EOF_TRACK_BASS        1
#define EOF_TRACK_GUITAR_COOP 2
#define EOF_TRACK_RHYTHM      3
#define EOF_TRACK_DRUM        4
#define EOF_TRACK_VOCALS      5

#define EOF_NOTE_FLAG_HOPO       1
#define EOF_NOTE_FLAG_SP         2
#define EOF_NOTE_FLAG_CRAZY      4	//This flag will represent overlap allowed for guitar tracks
#define EOF_NOTE_FLAG_DBASS      4	//This flag will represent Expert+ bass drum for the drum track
#define EOF_NOTE_FLAG_F_HOPO     8
#define EOF_NOTE_FLAG_NO_HOPO   16
#define EOF_NOTE_FLAG_Y_CYMBAL  32	//This flag represents a yellow note charted as a RB3 Pro style cymbal
#define EOF_NOTE_FLAG_B_CYMBAL  64	//This flag represents a blue note charted as a RB3 Pro style cymbal
#define EOF_NOTE_FLAG_G_CYMBAL 128	//This flag represents a note charted as a RB3 Pro style green cymbal (pad 4)

#define EOF_MAX_BEATS   32768
#define EOF_MAX_SOLOS      32
#define EOF_MAX_STAR_POWER 32
#define EOF_MAX_OGGS        8

#define EOF_BEAT_FLAG_ANCHOR      1
#define EOF_BEAT_FLAG_EVENTS      2
#define EOF_BEAT_FLAG_START_4_4   4
#define EOF_BEAT_FLAG_START_3_4   8
#define EOF_BEAT_FLAG_START_5_4  16
#define EOF_BEAT_FLAG_START_6_4  32
#define EOF_BEAT_FLAG_CUSTOM_TS  64
#define EOF_BEAT_FLAG_CORRUPTED 128

#define EOF_LYRIC_LINE_FLAG_OVERDRIVE 1

#define EOF_MAX_TEXT_EVENTS 4096

#define EOF_DEFAULT_TIME_DIVISION 480 // default time division used to convert midi_pos to msec_pos

#define EOF_MAX_GRID_SNAP_INTERVALS 64

typedef struct
{

	char          type;			//Stores the note's difficulty
	char          note;			//Stores the note's fret values
	unsigned long midi_pos;
	long          midi_length;
	unsigned long pos;
	long          length;
	char          flags;		//Stores various note statuses

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
	char          flags;

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

#define EOF_LEGACY_TRACKS_MAX		5
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

} EOF_TRACK_LEGACY;

#define EOF_VOCAL_TRACKS_MAX		1
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
	unsigned long flags;	//If the EOF_BEAT_FLAG_CUSTOM_TS flag is set, this variable's MSB is the TS's numerator (value 1 is stored as all bits 0), the 2nd MSB is the TS's denominator (value 1 is stored as all bits 0)

	double fpos;

} EOF_BEAT_MARKER;

typedef struct
{

	char filename[256];
	int  midi_offset;
	char modified;

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

	EOF_OGG_INFO  ogg[EOF_MAX_OGGS];
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

#define EOF_LEGACY_TRACK_TYPE				1
#define EOF_VOCAL_TRACK_TYPE				2
#define EOF_PRO_KEYS_TRACK_TYPE				3
#define EOF_PRO_GUITAR_TRACK_TYPE			4
#define EOF_PRO_VARIABLE_LEGACY_TRACK_TYPE	5
typedef struct
{
	char tracktype;			//Specifies which track type this is, using one of the macros above
	unsigned long tracknum;	//Specifies which number of that type this track is, used as an index into the type-specific track arrays
} EOF_TRACK_ENTRY;

#define EOF_MAX_TRACKS	(EOF_LEGACY_TRACKS_MAX + EOF_VOCAL_TRACKS_MAX)

typedef struct
{

	/* song info */
	EOF_SONG_TAGS * tags;

	/* MIDI "resolution" used to determine how MIDI is exported,
	 * when importing we should store the value from the source file here to
	 * simplify import and to minimize changes made to the file upon export */
	int resolution;

	/* track data */
	EOF_TRACK_LEGACY * legacy_track[EOF_LEGACY_TRACKS_MAX];
	unsigned long legacytracks;

	EOF_VOCAL_TRACK * vocal_track[EOF_VOCAL_TRACKS_MAX];
	unsigned long vocaltracks;

	EOF_TRACK_ENTRY * track[EOF_MAX_TRACKS];	//track[] is a list of all existing tracks among all track types
	unsigned long tracks;

	EOF_BEAT_MARKER * beat[EOF_MAX_BEATS];
	unsigned long beats;

	EOF_TEXT_EVENT * text_event[EOF_MAX_TEXT_EVENTS];
	unsigned long text_events;

	/* miscellaneous */
	unsigned long bookmark_pos[EOF_MAX_BOOKMARK_ENTRIES];
	EOF_CATALOG * catalog;

} EOF_SONG;

EOF_SONG * eof_create_song(void);	//Allocates, initializes and returns an EOF_SONG structure
void eof_destroy_song(EOF_SONG * sp);	//De-allocates the memory used by the EOF_SONG structure
int eof_load_song_pf(EOF_SONG * sp, PACKFILE * fp);	//Loads data from the specified PACKFILE pointer into the given EOF_SONG structure (called by eof_load_song())
EOF_SONG * eof_load_song(const char * fn);	//Loads the specified EOF file, validating the file header and loading the appropriate OGG file
int eof_save_song(EOF_SONG * sp, const char * fn);	//Saves the song to file

EOF_NOTE * eof_track_add_note(EOF_TRACK_LEGACY * tp);	//Allocates, initializes and stores a new EOF_NOTE structure into the notes array.  Returns the newly allocated structure or NULL upon error
void eof_track_delete_note(EOF_TRACK_LEGACY * tp, int note);	//Removes and frees the specified note from the notes array.  All notes after the deleted note are moved back in the array one position
void eof_track_sort_notes(EOF_TRACK_LEGACY * tp);	//Performs a quicksort of the notes array
int eof_song_qsort_notes(const void * e1, const void * e2);	//The comparitor function used to quicksort the notes array
int eof_fixup_next_note(EOF_TRACK_LEGACY * tp, int note);	//Returns the note one after the specified note number that is in the same difficulty, or -1 if there is none
void eof_track_find_crazy_notes(EOF_TRACK_LEGACY * tp);	//Used during MIDI import to mark a note as "crazy" if it overlaps with the next note in the same difficulty
void eof_track_fixup_notes(EOF_TRACK_LEGACY * tp, int sel);	//Performs cleanup of the specified instrument track
void eof_track_resize(EOF_TRACK_LEGACY * tp, int notes);	//Grows or shrinks the notes array to fit the specified number of notes by allocating/freeing EOF_NOTE structures
void eof_track_add_star_power(EOF_TRACK_LEGACY * tp, unsigned long start_pos, unsigned long end_pos);	//Adds a star power phrase at the specified start and stop timestamp for the specified track
void eof_track_delete_star_power(EOF_TRACK_LEGACY * tp, int index);	//Deletes the specified star power phrase and moves all phrases that follow back in the array one position
void eof_track_add_solo(EOF_TRACK_LEGACY * tp, unsigned long start_pos, unsigned long end_pos);	//Adds a solo phrase at the specified start and stop timestamp for the specified track
void eof_track_delete_solo(EOF_TRACK_LEGACY * tp, int index);	//Deletes the specified solo phrase and moves all phrases that follow back in the array one position

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
int eof_song_resize_beats(EOF_SONG * sp, int beats);	//Grows or shrinks the beats array to fit the specified number of beats by allocating/freeing EOF_BEAT_MARKER structures.  Returns zero on error

EOF_TEXT_EVENT * eof_song_add_text_event(EOF_SONG * sp, int beat, char * text);	//Allocates, initializes and stores a new EOF_TEXT_EVENT structure into the text_event array.  Returns the newly allocated structure or NULL upon error
void eof_song_delete_text_event(EOF_SONG * sp, int event);	//Removes and frees the specified text event from the text_events array.  All text events after the deleted text event are moved back in the array one position
void eof_song_move_text_events(EOF_SONG * sp, int beat, int offset);	//Displaces all beats starting with the specified beat by the given additive offset.  Each affected beat's flags are cleared except for the anchor and text event(s) present flags
int eof_song_resize_text_events(EOF_SONG * sp, int events);	//Grows or shrinks the text events array to fit the specified number of notes by allocating/freeing EOF_LYRIC structures.  Return zero on error
void eof_sort_events(void);	//Performs a quicksort of the events array
int eof_song_qsort_events(const void * e1, const void * e2);	//The comparitor function used to quicksort the events array

void eof_sort_notes(void);	//Sorts the notes in all tracks
void eof_fixup_notes(void);	//Performs cleanup of the note selection, beats and all tracks
void eof_detect_difficulties(EOF_SONG * sp);	//Sets the populated status by prefixing each populated difficulty name in the current track (stored in eof_note_type_name[] and eof_vocal_tab_name[]) with an asterisk

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

char eof_check_flags_at_note_pos(EOF_TRACK_LEGACY *tp,unsigned notenum,char flag);
	//Checks all notes in the track at the specified note's timestamp (numbered starting at number 0)
	//for the specified flag.  If any of the notes have the flag set, nonzero is returned
	//This is used for writing RB3 style pro drum phrases during MIDI export
	//The track's notes array is expected to be sorted
void eof_set_flags_at_note_pos(EOF_TRACK_LEGACY *tp,unsigned notenum,char flag,char operation);
	//Sets or clears the specified flag on all notes at the specified note's timestamp (numbered starting at 0)
	//If operation is 0, the specified flag is cleared on applicable notes
	//If operation is 1, the specified flag is set on applicable notes
	//If operation is 2, the specified flag is toggled on applicable notes
	//The track's notes array is expected to be sorted

int eof_load_song_string_pf(char *buffer, PACKFILE *fp, unsigned long buffersize);
	//Reads the next two bytes of the PACKFILE to determine how many characters long the string is
	//That number of bytes is read, the first (buffersize-1) of which are copied to the buffer
	//If buffersize is 0, the string is parsed in the file but not stored, otherwise the buffer is NULL terminated
	//Nonzero is returned on error

EOF_TRACK_LEGACY * eof_song_add_legacy_track(EOF_SONG * sp);
	//Allocates, initializes and stores a new EOF_TRACK structure into the tracks array.  Returns the newly allocated structure or NULL upon error
int eof_song_delete_legacy_track(EOF_SONG * sp, unsigned long track);
	//Deletes the specified track only if it contains no notes.  Returns zero on error
EOF_VOCAL_TRACK * eof_song_add_vocal_track(EOF_SONG * sp);
	//Allocates, initializes and stores a new EOF_VOCAL_TRACK structure into the vocal tracks array.  Returns the newly allocated structure or NULL upon error
int eof_song_delete_vocal_track(EOF_SONG * sp, unsigned long track);
	//Deletes the specified track only if it contains no notes.  Returns zero on error
	//Until EOF can store an array of vocal tracks, track 0 will refer to the only destroyable track (sp->vocal_track)

#endif
