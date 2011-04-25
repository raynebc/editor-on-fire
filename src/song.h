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

#define EOF_NOTE_FLAG_HOPO       1	//This flag will be set by eof_determine_phrase_status() if the note displays as a HOPO
#define EOF_NOTE_FLAG_SP         2	//This flag will be set by eof_determine_phrase_status() if the note is in a star power section
#define EOF_NOTE_FLAG_CRAZY      4	//This flag will represent overlap allowed for guitar tracks
#define EOF_NOTE_FLAG_F_HOPO     8
#define EOF_NOTE_FLAG_NO_HOPO   16
#define EOF_NOTE_FLAG_Y_CYMBAL  32	//This flag represents a yellow note charted as a RB3 Pro style cymbal
#define EOF_NOTE_FLAG_B_CYMBAL  64	//This flag represents a blue note charted as a RB3 Pro style cymbal
#define EOF_NOTE_FLAG_G_CYMBAL 128	//This flag represents a note charted as a RB3 Pro style green cymbal (lane 5)
#define EOF_NOTE_FLAG_DBASS    256	//This flag will represent Expert+ bass drum for the drum track

//The following flags pertain to pro guitar notes
#define EOF_PRO_GUITAR_NOTE_FLAG_HO				512		//This flag will represent a hammer on
#define EOF_PRO_GUITAR_NOTE_FLAG_PO				1024	//This flag will represent a pull off
#define EOF_PRO_GUITAR_NOTE_FLAG_TAP			2048	//This flag will represent a tapped note
#define EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP		4096	//This flag will represent a note that slides up to the next note
#define EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN		8192	//This flag will represent a note that slides down to the next note
#define EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE	16384	//This flag will represent a note whose strings are muted by the fretting hand
#define EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE		32768	//This flag will represent a note whose strings are muted by the strumming hand
#define EOF_NOTE_FLAG_IS_TRILL		            65536	//This flag will be set by eof_determine_phrase_status() if the note is in a trill section
#define EOF_NOTE_FLAG_IS_TREMOLO		        131072	//This flag will be set by eof_determine_phrase_status() if the note is in a tremolo section

#define EOF_MAX_BEATS   32768
#define EOF_MAX_PHRASES   100
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

#define EOF_NAME_LENGTH 30

typedef struct
{
    char name[EOF_NAME_LENGTH+1];
	char          type;			//Stores the note's difficulty
	unsigned char note;			//Stores the note's fret values
	unsigned long midi_pos;
	unsigned long midi_length;
	unsigned long pos;
	long length;				//Keep as signed, since the npos logic uses signed math
	unsigned long flags;		//Stores various note statuses

} EOF_NOTE;

typedef struct
{
    unsigned char number;		//The chord's number (using RB3's chord number system)
    char name[EOF_NAME_LENGTH+1];
	char          type;			//Stores the note's difficulty
	unsigned char note;			//Stores the note's string statuses (set=played, reset=not played).  Bit 0 refers to string 6 (low E), bit 5 refers to string 1 (high e), etc.
	unsigned char ghost;		//Stores the note's ghost statuses.  Bit 0 indicates whether string 1 is ghosted, etc.
	unsigned char frets[16];	//Stores the fret number for each string, where frets[0] refers to string 6 (low E).  Possible values:0=Open strum, #=Fret # pressed, 0xFF=Muted
	unsigned char legacymask;	//If this is nonzero, it indicates that the user defined this as the bitmask to use when pasting this into a legacy track
	unsigned long midi_pos;
	long midi_length;			//Keep as signed, since the npos logic uses signed math
	unsigned long pos;
	long length;				//Keep as signed, since the npos logic uses signed math
	unsigned long flags;		//Stores various note statuses

} EOF_PRO_GUITAR_NOTE;

typedef struct
{
	char name[EOF_NAME_LENGTH+1];
	char          type;
	unsigned char note;
	unsigned long endbeat;
	unsigned long beat;       // which beat this note was copied from
	unsigned long pos;
	long length;				//Keep as signed, since the npos logic uses signed math
	unsigned long midi_pos;
	unsigned long midi_length;
	float         porpos;     // position of note within the beat (100.0 = full beat)
	float         porendpos;
	char          active;
	unsigned long flags;

} EOF_EXTENDED_NOTE;

typedef struct
{
	unsigned char type;		//The lyric set this lyric belongs to (0=PART VOCALS, 1=HARM1, 2=HARM2...)
	unsigned char note;		// if zero, the lyric has no defined pitch
	char          text[EOF_MAX_LYRIC_LENGTH+1];
	unsigned long midi_pos;
	long midi_length;		//Keep as signed, since the npos logic uses signed math
	unsigned long pos;
	long length;			//Keep as signed, since the npos logic uses signed math
	unsigned long flags;

} EOF_LYRIC;

typedef struct
{
	char          note;
	char          text[EOF_MAX_LYRIC_LENGTH+1];
	unsigned long midi_pos;
	long          midi_length;	//Keep as signed, since the npos logic uses signed math
	unsigned long pos;
	long          length;		//Keep as signed, since the npos logic uses signed math

	unsigned long beat;       // which beat this note was copied from
	unsigned long endbeat;    // which beat this note was copied from
	float         porpos;     // position of note within the beat (100.0 = full beat)
	float         porendpos;

} EOF_EXTENDED_LYRIC;

typedef struct
{
	unsigned long midi_start_pos;
	unsigned long midi_end_pos;
	unsigned long start_pos;
	unsigned long end_pos;
	unsigned long flags;
    char name[EOF_NAME_LENGTH+1];

} EOF_PHRASE_SECTION;

#define EOF_LEGACY_TRACK_FORMAT					1
#define EOF_VOCAL_TRACK_FORMAT					2
#define EOF_PRO_KEYS_TRACK_FORMAT				3
#define EOF_PRO_GUITAR_TRACK_FORMAT				4
#define EOF_PRO_VARIABLE_LEGACY_TRACK_FORMAT	5

#define EOF_GUITAR_TRACK_BEHAVIOR		1
#define EOF_DRUM_TRACK_BEHAVIOR			2
#define EOF_VOCAL_TRACK_BEHAVIOR		3
#define EOF_KEYS_TRACK_BEHAVIOR			4
#define EOF_PRO_GUITAR_TRACK_BEHAVIOR	5
#define EOF_PRO_KEYS_TRACK_BEHAVIOR		6

#define EOF_TRACKS_MIN			1
#define EOF_TRACK_GUITAR		1
#define EOF_TRACK_BASS			2
#define EOF_TRACK_GUITAR_COOP	3
#define EOF_TRACK_RHYTHM		4
#define EOF_TRACK_DRUM			5
#define EOF_TRACK_VOCALS		6
#define EOF_TRACK_KEYS			7
#define EOF_TRACK_PRO_BASS		8
#define EOF_TRACK_PRO_GUITAR	9
#define EOF_TRACK_PRO_KEYS		10

#define EOF_TRACK_FLAG_OPEN_STRUM	1
	//Specifies if the track has open strumming enabled (ie. PART BASS)

#define EOF_TRACK_NAME_SIZE		31
typedef struct
{
	char track_format;				//Specifies which track format this is, using one of the macros above
	char track_behavior;			//Specifies which behavior this track follows, using one of the macros above
	char track_type;				//Specifies which type of track this is (ie default PART GUITAR, custom track, etc)
	unsigned long tracknum;			//Specifies which number of that type this track is, used as an index into the type-specific track arrays
	char name[EOF_NAME_LENGTH+1];	//Specifies the name of the track
	unsigned char difficulty;		//Specifies the difficulty level from 0-5 (standard 0-5 scale), or 6 for devil heads (extreme difficulty)
	unsigned long flags;
} EOF_TRACK_ENTRY;

#define EOF_LEGACY_TRACKS_MAX		6
typedef struct
{
	unsigned char numlanes;		//The number of lanes, keys, etc. in this track
	EOF_TRACK_ENTRY * parent;	//Allows an easy means to look up the global track using a legacy track pointer

	EOF_NOTE * note[EOF_MAX_NOTES];
	unsigned long notes;

	/* solos */
	EOF_PHRASE_SECTION solo[EOF_MAX_PHRASES];
	unsigned long solos;

	/* star power */
	EOF_PHRASE_SECTION star_power_path[EOF_MAX_PHRASES];
	unsigned long star_power_paths;

	/* trill sections */
	EOF_PHRASE_SECTION trill[EOF_MAX_PHRASES];
	unsigned long trills;

	/* tremolo sections */
	EOF_PHRASE_SECTION tremolo[EOF_MAX_PHRASES];
	unsigned long tremolos;

} EOF_LEGACY_TRACK;

#define EOF_VOCAL_TRACKS_MAX		1
typedef struct
{
	unsigned char toneset;		//The tone cue set assigned for playback of this track
	EOF_TRACK_ENTRY * parent;	//Allows an easy means to look up the global track using a vocal track pointer

	EOF_LYRIC * lyric[EOF_MAX_LYRICS];
	unsigned long lyrics;

	/* lyric lines */
	EOF_PHRASE_SECTION line[EOF_MAX_LYRIC_LINES];
	unsigned long lines;

} EOF_VOCAL_TRACK;

#define EOF_PRO_GUITAR_TRACKS_MAX	2
typedef struct
{
	unsigned char numfrets;		//The number of frets in this track
	unsigned char numstrings;	//The number of strings/lanes in this track
	unsigned char *tuning;		//An array with at least (numstrings) elements, each of which defines the string's note when played open (tuning[0] refers to string 1, which is high E)
	EOF_TRACK_ENTRY * parent;	//Allows an easy means to look up the global track using a pro guitar track pointer

	EOF_PRO_GUITAR_NOTE * note[EOF_MAX_NOTES];
	unsigned long notes;

	/* solos */
	EOF_PHRASE_SECTION solo[EOF_MAX_PHRASES];
	unsigned long solos;

	/* star power */
	EOF_PHRASE_SECTION star_power_path[EOF_MAX_PHRASES];
	unsigned long star_power_paths;

	/* arpeggios */
	EOF_PHRASE_SECTION arpeggio[EOF_MAX_PHRASES];
	unsigned long arpeggios;

	/* trill sections */
	EOF_PHRASE_SECTION trill[EOF_MAX_PHRASES];
	unsigned long trills;

	/* tremolo sections */
	EOF_PHRASE_SECTION tremolo[EOF_MAX_PHRASES];
	unsigned long tremolos;

} EOF_PRO_GUITAR_TRACK;

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
	long  midi_offset;	//Leave signed just in case this is eventually used to allow for insertion of leading silence via specifying a negative midi offset
	char modified;
	unsigned long flags;
	char description[EOF_NAME_LENGTH+1];

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
	unsigned long difficulty;		//Specifies the difficulty level from 0-5 (standard 0-5 scale), or 6 for devil heads (extreme difficulty)

} EOF_SONG_TAGS;

typedef struct
{
	unsigned long track;
	char type;
	unsigned long start_pos;
	unsigned long end_pos;
	char name[EOF_NAME_LENGTH+1];

} EOF_CATALOG_ENTRY;

typedef struct
{
	char text[256];
	unsigned long beat;

} EOF_TEXT_EVENT;

typedef struct
{
	EOF_CATALOG_ENTRY entry[EOF_MAX_CATALOG_ENTRIES];
	unsigned long entries;

} EOF_CATALOG;

#define EOF_TRACKS_MAX	(EOF_LEGACY_TRACKS_MAX + EOF_VOCAL_TRACKS_MAX + EOF_PRO_GUITAR_TRACKS_MAX + 1)
//Add 1 to reflect that track[0] is added but not used

extern EOF_TRACK_ENTRY eof_default_tracks[EOF_TRACKS_MAX + 1];
	//The list of default tracks that should be presented in EOF
extern EOF_TRACK_ENTRY eof_midi_tracks[EOF_TRACKS_MAX + 12];
	//The list of MIDI track names pertaining to each instrument and harmony track

typedef struct
{
	/* song info */
	EOF_SONG_TAGS * tags;

	/* MIDI "resolution" used to determine how MIDI is exported,
	 * when importing we should store the value from the source file here to
	 * simplify import and to minimize changes made to the file upon export */
	int resolution;

	/* track data */
	EOF_TRACK_ENTRY * track[EOF_TRACKS_MAX];	//track[] is a list of all existing tracks among all track types
	unsigned long tracks;						//track[0] is a dummy track and does not store actual track data
	//one more track entry is allowed for so that track[EOF_TRACKS_MAX] will correctly index the last usable track

	EOF_LEGACY_TRACK * legacy_track[EOF_LEGACY_TRACKS_MAX];
	unsigned long legacy_tracks;

	EOF_VOCAL_TRACK * vocal_track[EOF_VOCAL_TRACKS_MAX];
	unsigned long vocal_tracks;

	EOF_PRO_GUITAR_TRACK * pro_guitar_track[EOF_PRO_GUITAR_TRACKS_MAX];
	unsigned long pro_guitar_tracks;

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
int eof_load_song_pf(EOF_SONG * sp, PACKFILE * fp);	//Loads data from the specified PACKFILE pointer into the given EOF_SONG structure (called by eof_load_song()).  Returns 0 on error
EOF_SONG * eof_load_song(const char * fn);	//Loads the specified EOF file, validating the file header and loading the appropriate OGG file
int eof_save_song(EOF_SONG * sp, const char * fn);	//Saves the song to file

unsigned long eof_get_track_size(EOF_SONG *sp, unsigned long track);						//Returns the number of notes/lyrics in the specified track, or 0 on error
unsigned long eof_get_chart_size(EOF_SONG *sp);												//Returns the number of notes/lyrics in the chart, or 0 on error
unsigned long eof_get_used_lanes(unsigned long track, unsigned long startpos, unsigned long endpos, char type);
	//Returns a bitmask representing all lanes used within the specified track difficulty during the specified time span
unsigned long eof_get_num_solos(EOF_SONG *sp, unsigned long track);							//Returns the number of solos in the specified track, or 0 on error
void eof_set_num_solos(EOF_SONG *sp, unsigned long track, unsigned long number);			//Sets the number of solos in the specified track
EOF_PHRASE_SECTION *eof_get_solo(EOF_SONG *sp, unsigned long track, unsigned long solonum);	//Returns a pointer to the specified solo, or NULL on error
unsigned long eof_get_num_star_power_paths(EOF_SONG *sp, unsigned long track);				//Returns the number of star power paths in the specified track, or 0 on error
void eof_set_num_star_power_paths(EOF_SONG *sp, unsigned long track, unsigned long number);	//Sets the number of star power paths in the specified track
EOF_PHRASE_SECTION *eof_get_star_power_path(EOF_SONG *sp, unsigned long track, unsigned long pathnum);	//Returns a pointer to the specified star power path, or NULL on error
void *eof_track_add_note(EOF_SONG *sp, unsigned long track);								//Calls the appropriate add function for the specified track, returning the newly allocated structure or NULL upon error
void eof_track_delete_note(EOF_SONG *sp, unsigned long track, unsigned long note);			//Performs the appropriate logic to remove the specified note/lyric from the specified track
void eof_track_resize(EOF_SONG *sp, unsigned long track, unsigned long size);				//Performs the appropriate logic to resize the specified track
char eof_get_note_type(EOF_SONG *sp, unsigned long track, unsigned long note);				//Returns the type (difficulty/lyric set) of the specified track's note/lyric, or 0xFF on error
void eof_set_note_type(EOF_SONG *sp, unsigned long track, unsigned long note, char type);	//Sets the type (difficulty/lyric set) of the specified track's note/lyric
unsigned long eof_get_note_pos(EOF_SONG *sp, unsigned long track, unsigned long note);		//Returns the position of the specified track's note/lyric, or 0 on error
void eof_set_note_pos(EOF_SONG *sp, unsigned long track, unsigned long note, unsigned long pos);	//Sets the position of the specified track's note/lyric
long eof_get_note_length(EOF_SONG *sp, unsigned long track, unsigned long note);			//Returns the length of the specified track's note/lyric, or 0 on error
void eof_set_note_length(EOF_SONG *sp, unsigned long track, unsigned long note, long length);	//Sets the length of the specified track's note/lyric
unsigned long eof_get_note_flags(EOF_SONG *sp, unsigned long track, unsigned long note);	//Returns the flags of the specified track's note/lyric, or 0 on error
void eof_set_note_flags(EOF_SONG *sp, unsigned long track, unsigned long note, unsigned long flags);	//Sets the flags of the specified track's note/lyric
unsigned long eof_get_note_note(EOF_SONG *sp, unsigned long track, unsigned long note);		//Returns the note bitflag of the specified track's note/lyric, or 0 on error
void eof_set_note_note(EOF_SONG *sp, unsigned long track, unsigned long note, unsigned long value);	//Sets the note value of the specified track's note/lyric
char *eof_get_note_name(EOF_SONG *sp, unsigned long track, unsigned long note);				//Returns a pointer to the note's statically allocated name array, or a lyric's text array, or NULL on error
void eof_set_note_name(EOF_SONG *sp, unsigned long track, unsigned long note, char *name);	//Copies the string into the note's statically allocated name array, or a lyric's text array
void *eof_track_add_create_note(EOF_SONG *sp, unsigned long track, unsigned long note, unsigned long pos, long length, char type, char *text);
	//Adds and initializes the appropriate note for the specified track, returning the newly created note structure, or NULL on error
	//Automatic flags will be applied appropriately (ie. crazy status for all notes in PART KEYS)
	//text is used to initialize the note name or lyric text
void *eof_track_add_create_note2(EOF_SONG *sp, unsigned long track, EOF_NOTE *note);
	//Adds and initializes the appropriate note for the specified track, returning the newly created note structure, or NULL on error
	//If track refers to a legacy track, it is created and initialized using the passed structure
	//If track refers to a pro guitar track, a pro guitar note is partially initialized and the rest of the data is set to default values, ie. fret values set to 0xFF (muted)
void eof_track_sort_notes(EOF_SONG *sp, unsigned long track);		//Calls the appropriate sort function for the specified track
long eof_track_fixup_next_note(EOF_SONG *sp, unsigned long track, unsigned long note);	//Returns the note/lyric one after the specified note/lyric number that is in the same difficulty, or -1 if there is none
void eof_track_fixup_notes(EOF_SONG *sp, unsigned long track, int sel);	//Calls the appropriate fixup function for the specified track
void eof_track_find_crazy_notes(EOF_SONG *sp, unsigned long track);	//Used during MIDI import to mark a note as "crazy" if it overlaps with the next note in the same difficulty
void eof_track_add_star_power_path(EOF_SONG *sp, unsigned long track, unsigned long start_pos, unsigned long end_pos);	//Adds a star power phrase at the specified start and stop timestamp for the specified track
void eof_track_delete_star_power_path(EOF_SONG *sp, unsigned long track, unsigned long pathnum);	//Deletes the specified star power phrase and moves all phrases that follow back in the array one position
void eof_track_add_solo(EOF_SONG *sp, unsigned long track, unsigned long start_pos, unsigned long end_pos);	//Adds a solo phrase at the specified start and stop timestamp for the specified track
void eof_track_delete_solo(EOF_SONG *sp, unsigned long track, unsigned long pathnum);	//Deletes the specified solo phrase and moves all phrases that follow back in the array one position
void eof_note_set_tail_pos(EOF_SONG *sp, unsigned long track, unsigned long note, unsigned long pos);	//Sets the note's length value to (pos - [note]->pos)
int eof_track_add_section(EOF_SONG * sp, unsigned long track, unsigned long sectiontype, char difficulty, unsigned long start, unsigned long end, unsigned long flags, char *name);
	//Adds the specified section to the specified track if it's valid for the track
	//For bookmark sections, the end variable represents which bookmark number is being set
	//For fret catalog sections, the flags variable represents which track the catalog entry belongs to
	//For lyric phrases, the difficulty field indicates which lyric set number (ie. PART VOCALS) the phrase applies to
	//For applicable section types, name may point to a section name string in which case it will be copied to the section's name array, or NULL in which case it will be ignored
	//Returns zero on error
unsigned long eof_count_track_lanes(EOF_SONG *sp, unsigned long track);		//Returns the number of lanes in the specified track, or the default of 5
unsigned long eof_get_num_trills(EOF_SONG *sp, unsigned long track);		//Returns the number of trill phrases in the specified track, or 0 on error
EOF_PHRASE_SECTION *eof_get_trill(EOF_SONG *sp, unsigned long track, unsigned long index);		//Returns a pointer to the specified trill phrase, or NULL on error
void eof_set_num_trills(EOF_SONG *sp, unsigned long track, unsigned long number);	//Sets the number of trill phrases in the specified track
unsigned long eof_get_num_tremolos(EOF_SONG *sp, unsigned long track);		//Returns the number of tremolo phrases in the specified track, or 0 on error
EOF_PHRASE_SECTION *eof_get_tremolo(EOF_SONG *sp, unsigned long track, unsigned long index);	//Returns a pointer to the specified tremolo phrase, or NULL on error
void eof_set_num_tremolos(EOF_SONG *sp, unsigned long track, unsigned long number);	//Sets the number of tremolo phrases in the specified track
unsigned long eof_get_num_arpeggios(EOF_SONG *sp, unsigned long track);		//Returns the number of arpeggio phrases in the specified track, or 0 on error
EOF_PHRASE_SECTION *eof_get_arpeggio(EOF_SONG *sp, unsigned long track, unsigned long index);	//Returns a pointer to the specified arpeggio phrase, or NULL on error
void eof_set_num_arpeggios(EOF_SONG *sp, unsigned long track, unsigned long number);	//Sets the number of arpeggio phrases in the specified track
void eof_track_delete_trill(EOF_SONG *sp, unsigned long track, unsigned long index);	//Deletes the specified trill phrase and moves all phrases that follow back in the array one position
void eof_track_delete_tremolo(EOF_SONG *sp, unsigned long track, unsigned long index);	//Deletes the specified tremolo phrase and moves all phrases that follow back in the array one position
unsigned long eof_get_num_lyric_sections(EOF_SONG *sp, unsigned long track);	//Returns the number of lyric sections in the specified track, or 0 on error
EOF_PHRASE_SECTION *eof_get_lyric_section(EOF_SONG *sp, unsigned long track, unsigned long sectionnum);	//Returns a pointer to the specified lyric section, or NULL on error
void *eof_copy_note(EOF_SONG *sp, unsigned long sourcetrack, unsigned long sourcenote, unsigned long desttrack, unsigned long pos, long length, char type);
	//Copies the specified note to the specified track, returning a pointer to the newly created note structure, or NULL on error
	//The specified position, length and type are applied to the new note.  Other note variables such as the bitmask/pitch and name/lyric text are copied as-is
	//If the source is a pro guitar track and the destination is not, the source note's legacy bitmask is used if defined
	//If the source and destination are both pro guitar tracks, the source note's fret values are copied
long eof_get_prev_note_type_num(EOF_SONG *sp, unsigned long track, unsigned long note);
	//Returns the note immediately before the specified that is in the same difficulty, provided that the notes are sorted chronologically, or -1 if no such note exists
void eof_adjust_note_length(EOF_SONG * sp, unsigned long track, unsigned long amount, int dir);
	//If dir is >= 0, increases all selected notes in the active instrument difficulty by the specified amount
	//If dir is < 0, decreases all selected notes in the active instrument difficulty by the specified amount
	//If amount is 0, then the notes are adjusted by the current grid snap value
	//An undo state is only created if at least one note's length is altered

EOF_NOTE * eof_legacy_track_add_note(EOF_LEGACY_TRACK * tp);	//Allocates, initializes and stores a new EOF_NOTE structure into the notes array.  Returns the newly allocated structure or NULL upon error
void eof_legacy_track_delete_note(EOF_LEGACY_TRACK * tp, unsigned long note);	//Removes and frees the specified note from the notes array.  All notes after the deleted note are moved back in the array one position
void eof_legacy_track_sort_notes(EOF_LEGACY_TRACK * tp);	//Performs a quicksort of the notes array
int eof_song_qsort_legacy_notes(const void * e1, const void * e2);	//The comparitor function used to quicksort the legacy notes array
long eof_fixup_next_legacy_note(EOF_LEGACY_TRACK * tp, unsigned long note);	//Returns the note one after the specified note number that is in the same difficulty, or -1 if there is none
void eof_legacy_track_fixup_notes(EOF_LEGACY_TRACK * tp, int sel);	//Performs cleanup of the specified instrument track
void eof_legacy_track_add_star_power(EOF_LEGACY_TRACK * tp, unsigned long start_pos, unsigned long end_pos);	//Adds a star power phrase at the specified start and stop timestamp for the specified track
void eof_legacy_track_delete_star_power(EOF_LEGACY_TRACK * tp, unsigned long index);	//Deletes the specified star power phrase and moves all phrases that follow back in the array one position
void eof_legacy_track_add_solo(EOF_LEGACY_TRACK * tp, unsigned long start_pos, unsigned long end_pos);	//Adds a solo phrase at the specified start and stop timestamp for the specified track
void eof_legacy_track_delete_solo(EOF_LEGACY_TRACK * tp, unsigned long index);	//Deletes the specified solo phrase and moves all phrases that follow back in the array one position

EOF_LYRIC * eof_vocal_track_add_lyric(EOF_VOCAL_TRACK * tp);	//Allocates, initializes and stores a new EOF_LYRIC structure into the lyrics array.  Returns the newly allocated structure or NULL upon error
void eof_vocal_track_delete_lyric(EOF_VOCAL_TRACK * tp, unsigned long lyric);	//Removes and frees the specified lyric from the lyrics array.  All lyrics after the deleted lyric are moved back in the array one position
void eof_vocal_track_sort_lyrics(EOF_VOCAL_TRACK * tp);		//Performs a quicksort of the lyrics array
int eof_song_qsort_lyrics(const void * e1, const void * e2);	//The comparitor function used to quicksort the lyrics array
long eof_fixup_next_lyric(EOF_VOCAL_TRACK * tp, unsigned long lyric);	//Returns the next lyric, or -1 if there is none
void eof_vocal_track_fixup_lyrics(EOF_VOCAL_TRACK * tp, int sel);	//Performs cleanup of the specified lyric track
void eof_vocal_track_add_line(EOF_VOCAL_TRACK * tp, unsigned long start_pos, unsigned long end_pos);	//Adds a lyric phrase at the specified start and stop timestamp for the specified track
void eof_vocal_track_delete_line(EOF_VOCAL_TRACK * tp, unsigned long index);	//Deletes the specified lyric phrase and moves all phrases that follow back in the array one position

EOF_PRO_GUITAR_NOTE *eof_pro_guitar_track_add_note(EOF_PRO_GUITAR_TRACK *tp);	//Allocates, initializes and stores a new EOF_PRO_GUITAR_NOTE structure into the notes array.  Returns the newly allocated structure or NULL upon error
void eof_pro_guitar_track_sort_notes(EOF_PRO_GUITAR_TRACK * tp);	//Performs a quicksort of the notes array
int eof_song_qsort_pro_guitar_notes(const void * e1, const void * e2);	//The comparitor function used to quicksort the pro guitar notes array
void eof_pro_guitar_track_delete_note(EOF_PRO_GUITAR_TRACK * tp, unsigned long note);	//Removes and frees the specified note from the notes array.  All notes after the deleted note are moved back in the array one position
long eof_fixup_next_pro_guitar_note(EOF_PRO_GUITAR_TRACK * tp, unsigned long note);	//Returns the note one after the specified note number that is in the same difficulty, or -1 if there is none
void eof_pro_guitar_track_fixup_notes(EOF_PRO_GUITAR_TRACK * tp, int sel);	//Performs cleanup of the specified instrument track
void eof_pro_guitar_track_add_star_power(EOF_PRO_GUITAR_TRACK * tp, unsigned long start_pos, unsigned long end_pos);	//Adds a star power phrase at the specified start and stop timestamp for the specified track
void eof_pro_guitar_track_delete_star_power(EOF_PRO_GUITAR_TRACK * tp, unsigned long index);	//Deletes the specified star power phrase and moves all phrases that follow back in the array one position
void eof_pro_guitar_track_add_solo(EOF_PRO_GUITAR_TRACK * tp, unsigned long start_pos, unsigned long end_pos);	//Adds a solo phrase at the specified start and stop timestamp for the specified track
void eof_pro_guitar_track_delete_solo(EOF_PRO_GUITAR_TRACK * tp, unsigned long index);	//Deletes the specified solo phrase and moves all phrases that follow back in the array one position
void eof_set_pro_guitar_fret_number(char function, unsigned long fretvalue);
	//Alters each selected pro guitar note's fret values on used strings (that match the eof_pro_guitar_fret_bitmask bitmask) based on the parameters:
	//If function is 0, the applicable strings' fret values are set to the specified value
	//If function is 1, the applicable strings' fret values are incremented
	//If function is 2, the applicable strings' fret values are decremented

EOF_BEAT_MARKER * eof_song_add_beat(EOF_SONG * sp);	//Allocates, initializes and stores a new EOF_BEAT_MARKER structure into the beats array.  Returns the newly allocated structure or NULL upon error
void eof_song_delete_beat(EOF_SONG * sp, unsigned long beat);	//Removes and frees the specified beat from the beats array.  All beats after the deleted beat are moved back in the array one position
int eof_song_resize_beats(EOF_SONG * sp, unsigned long beats);	//Grows or shrinks the beats array to fit the specified number of beats by allocating/freeing EOF_BEAT_MARKER structures.  Returns zero on error

EOF_TEXT_EVENT * eof_song_add_text_event(EOF_SONG * sp, unsigned long beat, char * text);	//Allocates, initializes and stores a new EOF_TEXT_EVENT structure into the text_event array.  Returns the newly allocated structure or NULL upon error
void eof_song_delete_text_event(EOF_SONG * sp, unsigned long event);	//Removes and frees the specified text event from the text_events array.  All text events after the deleted text event are moved back in the array one position
void eof_song_move_text_events(EOF_SONG * sp, unsigned long beat, int offset);	//Displaces all beats starting with the specified beat by the given additive offset.  Each affected beat's flags are cleared except for the anchor and text event(s) present flags
int eof_song_resize_text_events(EOF_SONG * sp, unsigned long events);	//Grows or shrinks the text events array to fit the specified number of notes by allocating/freeing EOF_LYRIC structures.  Return zero on error
void eof_sort_events(void);	//Performs a quicksort of the events array
int eof_song_qsort_events(const void * e1, const void * e2);	//The comparitor function used to quicksort the events array

void eof_sort_notes(EOF_SONG *sp);	//Sorts the notes in all tracks
void eof_fixup_notes(EOF_SONG *sp);	//Performs cleanup of the note selection, beats and all tracks
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

long eof_song_tick_to_msec(EOF_SONG * sp, unsigned long tick); // convert tick value to real time, or -1 on error
long eof_song_msec_to_tick(EOF_SONG * sp, unsigned long msec); // convert real time value to tick, or -1 on error

char eof_check_flags_at_legacy_note_pos(EOF_LEGACY_TRACK *tp,unsigned notenum,unsigned long flag);
	//Checks all notes in the track at the specified note's timestamp (numbered starting at number 0)
	//for the specified flag.  If any of the notes have the flag set, nonzero is returned
	//This is used for writing RB3 style pro drum phrases during MIDI export
	//The track's notes array is expected to be sorted
void eof_set_flags_at_legacy_note_pos(EOF_LEGACY_TRACK *tp,unsigned notenum,unsigned long flag,char operation);
	//Sets or clears the specified flag on all notes at the specified note's timestamp (numbered starting at 0)
	//If operation is 0, the specified flag is cleared on applicable notes
	//If operation is 1, the specified flag is set on applicable notes
	//If operation is 2, the specified flag is toggled on applicable notes
	//The track's notes array is expected to be sorted

int eof_load_song_string_pf(char *const buffer, PACKFILE *fp, const unsigned long buffersize);
	//Reads the next two bytes of the PACKFILE to determine how many characters long the string is
	//That number of bytes is read, the first (buffersize-1) of which are copied to the buffer
	//If buffersize is 0, the string is parsed in the file but not stored, otherwise the buffer is NULL terminated
	//Nonzero is returned on error
int eof_save_song_string_pf(char *const buffer, PACKFILE *fp);
	//Writes two bytes for the length of the string, followed by the string (minus the NULL terminator)
	//If buffer is NULL, two zero bytes (representing empty string) are written to the file
	//The length of the string written is in bytes, not chars, so Unicode strings could be supported
	//Nonzero is returned on error

int eof_song_add_track(EOF_SONG * sp, EOF_TRACK_ENTRY * trackdetails);
	//Adds a new track using the specified details.  The tracknum field of the track entry structure is ignored
	//Returns zero on error
int eof_song_delete_track(EOF_SONG * sp, unsigned long track);
	//Deletes the specified track from the main track array and the appropriate track type array, but only if the track contains no notes.  Returns zero on error
EOF_SONG * eof_create_song_populated(void);
	//Allocates, initializes and returns an EOF_SONG structure pre-populated with the default legacy and vocal tracks

#define EOF_SOLO_SECTION				1
#define EOF_SP_SECTION					2
#define EOF_BOOKMARK_SECTION			3
#define EOF_FRET_CATALOG_SECTION		4
#define EOF_LYRIC_PHRASE_SECTION		5
#define EOF_YELLOW_TOM_SECTION			6
#define EOF_BLUE_TOM_SECTION			7
#define EOF_GREEN_TOM_SECTION			8
#define EOF_TRILL_SECTION				9
#define EOF_ARPEGGIO_SECTION			10
#define EOF_TRAINER_SECTION				11
#define EOF_CUSTOM_MIDI_NOTE_SECTION	12
#define EOF_PREVIEW_SECTION				13
#define EOF_TREMOLO_SECTION				14

inline int eof_open_bass_enabled(void);
	//A simple function returning nonzero if PART BASS has open strumming enabled
int eof_create_image_sequence(void);
	//Creates a PCX format image sequence in a subfolder of the chart's folder called "\sequence"

#endif
