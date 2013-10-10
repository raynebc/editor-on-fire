#ifndef EOF_SONG_H
#define EOF_SONG_H

#include <allegro.h>

#define EOF_OLD_MAX_NOTES     65536
#define EOF_MAX_NOTES         32768
#define EOF_MAX_LYRICS EOF_MAX_NOTES
#define EOF_MAX_LYRIC_LINES    4096
#define EOF_MAX_LYRIC_LENGTH    255
#define EOF_MAX_CATALOG_ENTRIES 256
#define EOF_MAX_INI_SETTINGS     64
#define EOF_MAX_BOOKMARK_ENTRIES 10

#define EOF_NOTE_SUPAEASY    0
#define EOF_NOTE_EASY        1
#define EOF_NOTE_MEDIUM      2
#define EOF_NOTE_AMAZING     3
#define EOF_NOTE_SPECIAL     4
#define EOF_NOTE_CHALLENGE   4
#define EOF_MAX_DIFFICULTIES 5
	//Note: EOF_NOTE_SPECIAL references the same difficulty that the dance track uses for "Challenge" difficulty

#define EOF_NOTE_FLAG_HOPO       1	//This flag will be set by eof_determine_phrase_status() if the note displays as a HOPO
#define EOF_NOTE_FLAG_SP         2	//This flag will be set by eof_determine_phrase_status() if the note is in a star power section
#define EOF_NOTE_FLAG_CRAZY      4	//This flag will represent overlap allowed for guitar/dance/keys tracks, and will force pro guitar/bass chords to display with a chord box
#define EOF_NOTE_FLAG_F_HOPO     8
#define EOF_NOTE_FLAG_NO_HOPO   16
#define EOF_NOTE_FLAG_Y_CYMBAL  32	//This flag represents a yellow note charted as a RB3 Pro style cymbal (lane 3)
#define EOF_NOTE_FLAG_B_CYMBAL  64	//This flag represents a blue note charted as a RB3 Pro style cymbal (lane 4)
#define EOF_NOTE_FLAG_G_CYMBAL 128	//This flag represents a note charted as a RB3 Pro style green cymbal (lane 5)
#define EOF_NOTE_FLAG_DBASS    256	//This flag will represent Expert+ bass drum for the drum track (lane 1)
#define EOF_NOTE_FLAG_TEMP       536870912	//This flag will represent a note that was generated for temporary use and will be removed
#define EOF_NOTE_FLAG_HIGHLIGHT 1073741824
#define EOF_NOTE_FLAG_EXTENDED  2147483648	//The MSB will be reserved for use to indicate an additional flag variable is present

//The following flags pertain to pro guitar notes
#define EOF_PRO_GUITAR_NOTE_FLAG_HO				512			//This flag will represent a hammer on
#define EOF_PRO_GUITAR_NOTE_FLAG_PO				1024		//This flag will represent a pull off
#define EOF_PRO_GUITAR_NOTE_FLAG_TAP			2048		//This flag will represent a tapped note
#define EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP		4096		//This flag will represent a note that slides up to the next note
#define EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN		8192		//This flag will represent a note that slides down to the next note
#define EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE	16384		//This flag will represent a note whose strings are muted by the fretting hand
#define EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE		32768		//This flag will represent a note whose strings are muted by the strumming hand
#define EOF_PRO_GUITAR_NOTE_FLAG_UP_STRUM		262144		//This flag will represent a chord that is played by strumming up
#define EOF_PRO_GUITAR_NOTE_FLAG_DOWN_STRUM		524288		//This flag will represent a chord that is played by strumming down
#define EOF_PRO_GUITAR_NOTE_FLAG_MID_STRUM		1048576		//This flag will represent a chord that is played by strumming in the middle of the strings (ie. barely playing or not playing strings 1 and 6)
#define EOF_PRO_GUITAR_NOTE_FLAG_BEND			2097152		//This flag will represent a note that is bent after it is picked
#define EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC		4194304		//This flag will represent a note that is played as a harmonic
#define EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_REVERSE  8388608		//This flag will represent a note whose slide will be written as reversed (channel 11)
#define EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO        16777216	//This flag will represent a note that is played with vibrato
#define EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION    33554432	//This flag will indicate whether a note with a slide or bend flag defines the slide's ending fret or the bend's strength (used in Rocksmith)
#define EOF_PRO_GUITAR_NOTE_FLAG_POP            67108864	//This flag will represent a note that is played with pop technique (ie. bass)
#define EOF_PRO_GUITAR_NOTE_FLAG_SLAP           134217728	//This flag will represent a note that is played with slap technique (ie. bass)

//The following flags pertain to drum notes
#define EOF_DRUM_NOTE_FLAG_Y_HI_HAT_OPEN	512		//This flag means the yellow cymbal will be displayed in Phase Shift as an open hi hat (lane 3)
#define EOF_DRUM_NOTE_FLAG_Y_HI_HAT_PEDAL	1024	//This flag means the yellow cymbal will be displayed in Phase Shift as a pedal controlled hi hat (lane 3)
#define EOF_DRUM_NOTE_FLAG_R_RIMSHOT		2048	//This flag means the red drum note will be displayed in Phase Shift as a rim shot (lane 2)
#define EOF_DRUM_NOTE_FLAG_Y_SIZZLE			4096	//This flag means the yellow cymbal will be displayed in Phase Shift as a "sizzle" (partially open) hi hat (lane 3)

//The following flags pertain to dance notes
#define EOF_DANCE_FLAG_LANE_1_MINE		512		//This flag will represent a mine note for lane 1
#define EOF_DANCE_FLAG_LANE_2_MINE		1024	//This flag will represent a mine note for lane 2
#define EOF_DANCE_FLAG_LANE_3_MINE		2048	//This flag will represent a mine note for lane 3
#define EOF_DANCE_FLAG_LANE_4_MINE		4096	//This flag will represent a mine note for lane 4

//The following flags pertain to legacy guitar notes
#define EOF_GUITAR_NOTE_FLAG_IS_SLIDER	512		//This flag will be set by eof_determine_phrase_status() if the note is in a slider section

//The following flags pertain to legacy and pro guitar notes
#define EOF_NOTE_FLAG_IS_TRILL		            65536	//This flag will be set by eof_determine_phrase_status() if the note is in a trill section
#define EOF_NOTE_FLAG_IS_TREMOLO		        131072	//This flag will be set by eof_determine_phrase_status() if the note is in a tremolo section

#define EOF_MAX_BEATS   32768
#define EOF_MAX_PHRASES   600
#define EOF_MAX_OGGS        8

#define EOF_BEAT_FLAG_ANCHOR       1
#define EOF_BEAT_FLAG_EVENTS       2
#define EOF_BEAT_FLAG_START_4_4    4
#define EOF_BEAT_FLAG_START_3_4    8
#define EOF_BEAT_FLAG_START_5_4   16
#define EOF_BEAT_FLAG_START_6_4   32
#define EOF_BEAT_FLAG_CUSTOM_TS   64	//If this is nonzero, indicates that the first and second most significant bytes of the beat's flags store the TS numerator and denominator, respectively
#define EOF_BEAT_FLAG_KEY_SIG    128
#define EOF_BEAT_FLAG_EXTENDED 32768	//Reserve the highest unused bit to allow for another beat flag to be conditionally present

#define EOF_LYRIC_LINE_FLAG_OVERDRIVE 1

#define EOF_MAX_TEXT_EVENTS 4096

#define EOF_DEFAULT_TIME_DIVISION 480 // default time division used to convert midi_pos to msec_pos

#define EOF_MAX_GRID_SNAP_INTERVALS 64

#define EOF_NAME_LENGTH 30
#define EOF_SECTION_NAME_LENGTH 50

typedef struct
{
    char name[EOF_NAME_LENGTH+1];
	unsigned char type;			//Stores the note's difficulty
	unsigned char note;			//Stores the note's fret values
	unsigned long midi_pos;
	unsigned long midi_length;
	unsigned long pos;
	long length;				//Keep as signed, since the npos logic uses signed math
	unsigned long flags;		//Stores various note statuses

} EOF_NOTE;

typedef struct
{
	char name[EOF_NAME_LENGTH+1];
	unsigned char type;			//Stores the note's difficulty
	unsigned char note;			//Stores the note's string statuses (set=played, reset=not played).  Bit 0 refers to string 6 (low E), bit 5 refers to string 1 (high e), etc.
	unsigned char ghost;		//Stores the note's ghost statuses.  Bit 0 indicates whether string 1 is ghosted, etc.
	unsigned char frets[8];		//Stores the fret number for each string, where frets[0] refers to string 6 (low E).  Possible values:0=Open strum, #=Fret # pressed, 0xFF=Muted (no fret specified).  MSB is the muted status (set = muted)
	unsigned char finger[8];	//Stores the finger number used to fret each string (0 = undefined/unused, 1 = index, 2 = middle, 3= ring, 4 = pinky, 5 = thumb)
	unsigned char legacymask;	//If this is nonzero, it indicates that the user defined this as the bitmask to use when pasting this into a legacy track
	unsigned long midi_pos;
	long midi_length;			//Keep as signed, since the npos logic uses signed math
	unsigned long pos;
	long length;				//Keep as signed, since the npos logic uses signed math
	unsigned long flags;		//Stores various note statuses
	unsigned char bendstrength;	//The number of half steps this note bends (0 if undefined or not applicable)
	unsigned char slideend;		//The fret at which this slide ends (0 if undefined or not applicable)

} EOF_PRO_GUITAR_NOTE;

typedef struct
{
	char name[EOF_NAME_LENGTH+1];
	unsigned char type;
	unsigned char note;
	unsigned long endbeat;
	unsigned long beat;       // which beat this note was copied from
	unsigned long pos;
	long length;				//Keep as signed, since the npos logic uses signed math
	unsigned long midi_pos;
	unsigned long midi_length;
	float         porpos;     // position of note within the beat (100.0 = full beat)
	float         porendpos;
	unsigned long flags;
	unsigned long legacymask;
	unsigned char frets[8];
	unsigned char finger[8];
	unsigned long ghostmask;
	unsigned char bendstrength;
	unsigned char slideend;

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
	unsigned char note;
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
	unsigned long end_pos;	//Will store other data in items that don't use an end position (such as the fret number for fret hand positions)
	unsigned long flags;
	char name[EOF_SECTION_NAME_LENGTH+1];
	unsigned char difficulty;	//The difficulty this phrase applies to (ie. arpeggios, hand positions, RS tremolos), or 0xFF if it otherwise applies to all difficulties

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
#define EOF_DANCE_TRACK_BEHAVIOR        7

//Track types
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
#define EOF_TRACK_DANCE         10
#define EOF_TRACK_PRO_BASS_22	11
#define EOF_TRACK_PRO_GUITAR_22	12
#define EOF_TRACK_DRUM_PS		13
#define EOF_TRACK_PRO_KEYS		14

#define EOF_SOLO_SECTION				1
#define EOF_SP_SECTION					2
#define EOF_BOOKMARK_SECTION			3
#define EOF_FRET_CATALOG_SECTION		4
#define EOF_LYRIC_PHRASE_SECTION		5
#define EOF_YELLOW_CYMBAL_SECTION       6
#define EOF_BLUE_CYMBAL_SECTION         7
#define EOF_GREEN_CYMBAL_SECTION        8
#define EOF_TRILL_SECTION				9
#define EOF_ARPEGGIO_SECTION			10
#define EOF_TRAINER_SECTION				11
#define EOF_CUSTOM_MIDI_NOTE_SECTION	12
#define EOF_PREVIEW_SECTION				13
#define EOF_TREMOLO_SECTION				14
#define EOF_SLIDER_SECTION				15
#define EOF_FRET_HAND_POS_SECTION       16
#define EOF_RS_POPUP_MESSAGE            17
#define EOF_RS_TONE_CHANGE              18

#define EOF_TRACK_FLAG_SIX_LANES		1
	//Specifies if the track has open strumming enabled (legacy bass or guitar tracks) or a fifth drum lane enabled (PART DRUMS)
#define EOF_TRACK_FLAG_ALT_NAME			2
	//Specifies if the track has an alternate display name (for RS export.  MIDI export will still use the native name)
#define EOF_TRACK_FLAG_UNLIMITED_DIFFS	4
	//Specifies if the track is not limited to 4 difficulties and one more for the BRE difficulty.  Higher numbered difficulties will be exported to Rocksmith format

#define EOF_TRACK_NAME_SIZE		31
typedef struct
{
	char track_format;				//Specifies which track format this is, using one of the macros above
	char track_behavior;			//Specifies which behavior this track follows, using one of the macros above
	char track_type;				//Specifies which type of track this is (ie default PART GUITAR, custom track, etc)
	unsigned long tracknum;			//Specifies which number of that type this track is, used as an index into the type-specific track arrays
	char name[EOF_NAME_LENGTH+1];	//Specifies the name of the track
	char altname[EOF_NAME_LENGTH+1];//Specifies the alternate name of the track (for RS export)
	unsigned char difficulty;		//Specifies the difficulty level from 0-5 (standard 0-5 scale), or 6 for devil heads (extreme difficulty).  0xFF means the difficulty is undefined
	unsigned char numdiffs;			//Specifies the number of difficulties usable in this track, including BRE (is set to 5 unless the track's EOF_TRACK_FLAG_UNLIMITED_DIFFS flag is set)
	unsigned long flags;
} EOF_TRACK_ENTRY;

#define EOF_LEGACY_TRACKS_MAX		8
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

	/* slider sections */
	EOF_PHRASE_SECTION slider[EOF_MAX_PHRASES];
	unsigned long sliders;

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

#define EOF_PRO_GUITAR_TRACKS_MAX	4
#define EOF_TUNING_LENGTH 6	//For now, the tuning array will only track 6 strings
#define EOF_NUM_DEFINED_CHORDS 28

typedef struct
{
	unsigned char numfrets;			//The number of frets in this track
	unsigned char numstrings;		//The number of strings/lanes in this track
	unsigned char arrangement;		//The arrangement type of this track  (0 = Undefined, 1 = Combo, 2 = Rhythm, 3 = Lead, 4 = Bass)
	unsigned char ignore_tuning;	//If nonzero, indicates that the chord name detection reflects the DEFAULT tuning for the arrangement, and not the track's actual specified tuning
	char tuning[EOF_TUNING_LENGTH];	//An array with at least (numstrings) elements, each of which defines the string's relative tuning as the +/- number of half steps from standard tuning (tuning[0] refers to lane 1's string, which is low E)
	EOF_TRACK_ENTRY * parent;		//Allows an easy means to look up the global track using a pro guitar track pointer

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

	/* fret hand positions */
	EOF_PHRASE_SECTION handposition[EOF_MAX_NOTES];	//There has to be the ability to store as many fret hand position changes as there are notes
	unsigned long handpositions;

	/* popup messages */
	EOF_PHRASE_SECTION popupmessage[EOF_MAX_NOTES];
	unsigned long popupmessages;

	/* tone changes */
	EOF_PHRASE_SECTION tonechange[EOF_MAX_PHRASES];
	unsigned long tonechanges;

} EOF_PRO_GUITAR_TRACK;

typedef struct
{
	unsigned long ppqn;
	unsigned long midi_pos;
	unsigned long pos;
	unsigned long flags;	//If the EOF_BEAT_FLAG_CUSTOM_TS flag is set, this variable's MSB is the TS's numerator (value 1 is stored as all bits 0), the 2nd MSB is the TS's denominator (value 1 is stored as all bits 0)
	char key;	//If negative, the value defines the number of flats present, ie. -2 is Bb.  If positive, the value defines the number of sharps present, ie. 4 is E
	double fpos;

	//These variables track various properties of the beat to relieve rendering functions of some processing
	unsigned long measurenum;
	int beat_within_measure, num_beats_in_measure, beat_unit, contained_section_event, contained_rs_section_event, contained_rs_section_event_instance_number;
	char contains_tempo_change, contains_ts_change, contains_end_event;

} EOF_BEAT_MARKER;

typedef struct
{
	char filename[256];
	long  midi_offset;	//Leave signed just in case this is eventually used to allow for insertion of leading silence via specifying a negative midi offset
	char modified;
	char description[EOF_NAME_LENGTH+1];

} EOF_OGG_INFO;

typedef struct
{
	char artist[256];
	char title[256];
	char frettist[256];
	char album[256];
	char year[32];
	char loading_text[512];
	char lyrics;
	char eighth_note_hopo;
	char eof_fret_hand_pos_1_pg;
	char eof_fret_hand_pos_1_pb;
	char tempo_map_locked;
	char double_bass_drum_disabled;
	char click_drag_disabled;
	char rs_chord_technique_export;
	char unshare_drum_phrasing;

	EOF_OGG_INFO ogg[EOF_MAX_OGGS];
	short oggs;

	char ini_setting[EOF_MAX_INI_SETTINGS][512];
	short ini_settings;

	unsigned long revision;
	unsigned long difficulty;		//Specifies the difficulty level from 0-5 (standard 0-5 scale), or 6 for devil heads (extreme difficulty)

} EOF_SONG_TAGS;

typedef struct
{
	unsigned long track;
	unsigned char type;
	unsigned long start_pos;
	unsigned long end_pos;
	char name[EOF_NAME_LENGTH+1];

} EOF_CATALOG_ENTRY;

typedef struct
{
	char text[256];
	unsigned long beat;
	unsigned long track;	//The track this event is tied to, or 0 if it goes into the EVENTS track (such as a generic section marker)
	char is_temporary;		//This is nonzero if the event is considered temporary (doesn't trigger undo/redo when added/deleted), required RBN events are added this way during save
	unsigned char flags;
	unsigned long index;	//Populated with the event's index in eof_sort_events(), since the quicksort algorithm can and will re-order list items that have equivalent sorting order

} EOF_TEXT_EVENT;

#define EOF_EVENT_FLAG_RS_PHRASE	1
#define EOF_EVENT_FLAG_RS_SECTION	2
#define EOF_EVENT_FLAG_RS_EVENT		4

typedef struct
{
	EOF_CATALOG_ENTRY entry[EOF_MAX_CATALOG_ENTRIES];
	unsigned long entries;

} EOF_CATALOG;

#define EOF_TRACKS_MAX	(EOF_LEGACY_TRACKS_MAX + EOF_VOCAL_TRACKS_MAX + EOF_PRO_GUITAR_TRACKS_MAX + 1)
//Add 1 to reflect that track[0] is added but not used
#define EOF_POWER_GIG_TRACKS_MAX (15 + 1)

extern EOF_TRACK_ENTRY eof_default_tracks[EOF_TRACKS_MAX + 1];
	//The list of default tracks that should be presented in EOF
extern EOF_TRACK_ENTRY eof_midi_tracks[EOF_TRACKS_MAX + 12];
	//The list of MIDI track names pertaining to each instrument and harmony track
extern EOF_TRACK_ENTRY eof_power_gig_tracks[EOF_POWER_GIG_TRACKS_MAX];

struct eof_MIDI_data_track
{
	char *trackname;
	char *description;
	struct eof_MIDI_data_event *events;
	struct eof_MIDI_data_track *next;
	unsigned long timedivision;	//Is set to nonzero when the track being stored is a tempo track
};
	//This structure will be used in a linked list defining the tracks in a MIDI file
	//Each will contain a linked list of the track's events

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

	struct eof_MIDI_data_track * midi_data_head, * midi_data_tail;	//Used to maintain a linked list of MIDI events that have been imported and that will be exported on save

	/* miscellaneous */
	unsigned long bookmark_pos[EOF_MAX_BOOKMARK_ENTRIES];	//A bookmark is set if its position is set to nonzero
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
void eof_song_empty_track(EOF_SONG * sp, unsigned long track);								//Deletes all notes from the track
void eof_track_resize(EOF_SONG *sp, unsigned long track, unsigned long size);				//Performs the appropriate logic to resize the specified track
unsigned char eof_get_note_type(EOF_SONG *sp, unsigned long track, unsigned long note);				//Returns the type (difficulty/lyric set) of the specified track's note/lyric, or 0xFF on error
void eof_set_note_type(EOF_SONG *sp, unsigned long track, unsigned long note, unsigned char type);	//Sets the type (difficulty/lyric set) of the specified track's note/lyric
unsigned long eof_get_note_pos(EOF_SONG *sp, unsigned long track, unsigned long note);		//Returns the position of the specified track's note/lyric, or 0 on error
void eof_set_note_pos(EOF_SONG *sp, unsigned long track, unsigned long note, unsigned long pos);	//Sets the position of the specified track's note/lyric
void eof_move_note_pos(EOF_SONG *sp, unsigned long track, unsigned long note, unsigned long amount, char dir);
	//If dir > 0, increases the position of the specified note by the specified amount
	//If dir < 0, decreases the position of the specified note by the specified amount ONLY if it won't make the position negative
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
	//text is used to initialize the note name or lyric text, and may be NULL
void *eof_track_add_create_note2(EOF_SONG *sp, unsigned long track, EOF_NOTE *note);
	//Adds and initializes the appropriate note for the specified track, returning the newly created note structure, or NULL on error
	//If track refers to a legacy track, it is created and initialized using the passed structure
	//If track refers to a pro guitar track, a pro guitar note is partially initialized and the rest of the data is set to default values, ie. fret values set to 0xFF (muted)
void eof_track_sort_notes(EOF_SONG *sp, unsigned long track);		//Calls the appropriate sort function for the specified track
long eof_track_fixup_previous_note(EOF_SONG *sp, unsigned long track, unsigned long note);	//Returns the note/lyric one before the specified note/lyric number that is in the same difficulty, or -1 if there is none
long eof_track_fixup_next_note(EOF_SONG *sp, unsigned long track, unsigned long note);	//Returns the note/lyric one after the specified note/lyric number that is in the same difficulty, or -1 if there is none
void eof_track_fixup_notes(EOF_SONG *sp, unsigned long track, int sel);	//Calls the appropriate fixup function for the specified track.  If sel is zero, the currently selected note is deselected automatically.
void eof_track_find_crazy_notes(EOF_SONG *sp, unsigned long track);	//Used during MIDI import to mark a note as "crazy" if it overlaps with the next note in the same difficulty
int eof_track_add_star_power_path(EOF_SONG *sp, unsigned long track, unsigned long start_pos, unsigned long end_pos);	//Adds a star power phrase at the specified start and stop timestamp for the specified track.  Returns nonzero on success
void eof_track_delete_star_power_path(EOF_SONG *sp, unsigned long track, unsigned long pathnum);	//Deletes the specified star power phrase and moves all phrases that follow back in the array one position
int eof_track_add_solo(EOF_SONG *sp, unsigned long track, unsigned long start_pos, unsigned long end_pos);	//Adds a solo phrase at the specified start and stop timestamp for the specified track.  Returns nonzero on success
void eof_track_delete_solo(EOF_SONG *sp, unsigned long track, unsigned long pathnum);	//Deletes the specified solo phrase and moves all phrases that follow back in the array one position
void eof_note_set_tail_pos(EOF_SONG *sp, unsigned long track, unsigned long note, unsigned long pos);	//Sets the note's length value to (pos - [note]->pos)
int eof_track_add_section(EOF_SONG * sp, unsigned long track, unsigned long sectiontype, unsigned char difficulty, unsigned long start, unsigned long end, unsigned long flags, char *name);
	//Adds the specified section to the specified track if it's valid for the track
	//For bookmark sections, the end variable represents which bookmark number is being set
	//For fret catalog sections, the flags variable represents which track the catalog entry belongs to, otherwise it's only used for lyric sections
	//For lyric phrases, the difficulty field indicates which lyric set number (ie. PART VOCALS) the phrase applies to
	//For applicable section types, name may point to a section name string in which case it will be copied to the section's name array, or NULL in which case it will be ignored
	//The difficulty field is used for catalog, arpeggio and tremolo (if Rocksmith numbered difficulties are in effect) sections and fret hand positions
	//Returns zero on error
unsigned long eof_count_track_lanes(EOF_SONG *sp, unsigned long track);		//Returns the number of lanes in the specified track, or the default of 5
int eof_track_add_trill(EOF_SONG *sp, unsigned long track, unsigned long start_pos, unsigned long end_pos);	//Adds a trill phrase at the specified start and stop timestamp
unsigned long eof_get_num_trills(EOF_SONG *sp, unsigned long track);		//Returns the number of trill phrases in the specified track, or 0 on error
EOF_PHRASE_SECTION *eof_get_trill(EOF_SONG *sp, unsigned long track, unsigned long index);		//Returns a pointer to the specified trill phrase, or NULL on error
void eof_set_num_trills(EOF_SONG *sp, unsigned long track, unsigned long number);	//Sets the number of trill phrases in the specified track
int eof_track_add_tremolo(EOF_SONG *sp, unsigned long track, unsigned long start_pos, unsigned long end_pos, unsigned char diff);
	//Adds a tremolo phrase at the specified start and stop timestamp
	//If diff is not 0xFF, and the specified track is a pro guitar track, the tremolo phrase will apply to the specified track difficulty only
unsigned long eof_get_num_tremolos(EOF_SONG *sp, unsigned long track);		//Returns the number of tremolo phrases in the specified track, or 0 on error
EOF_PHRASE_SECTION *eof_get_tremolo(EOF_SONG *sp, unsigned long track, unsigned long index);	//Returns a pointer to the specified tremolo phrase, or NULL on error
void eof_set_num_tremolos(EOF_SONG *sp, unsigned long track, unsigned long number);	//Sets the number of tremolo phrases in the specified track
unsigned long eof_get_num_sliders(EOF_SONG *sp, unsigned long track);		//Returns the number of slider phrases in the specified track, or 0 on error
EOF_PHRASE_SECTION *eof_get_slider(EOF_SONG *sp, unsigned long track, unsigned long index);	//Returns a pointer to the specified slider phrase, or NULL on error
void eof_set_num_sliders(EOF_SONG *sp, unsigned long track, unsigned long number);	//Sets the number of slider phrases in the specified track
unsigned long eof_get_num_arpeggios(EOF_SONG *sp, unsigned long track);		//Returns the number of arpeggio phrases in the specified track, or 0 on error
EOF_PHRASE_SECTION *eof_get_arpeggio(EOF_SONG *sp, unsigned long track, unsigned long index);	//Returns a pointer to the specified arpeggio phrase, or NULL on error
void eof_set_num_arpeggios(EOF_SONG *sp, unsigned long track, unsigned long number);	//Sets the number of arpeggio phrases in the specified track
unsigned long eof_get_num_popup_messages(EOF_SONG *sp, unsigned long track);			//Returns the number of popup messages in the specified track, or 0 on error
void eof_track_delete_trill(EOF_SONG *sp, unsigned long track, unsigned long index);	//Deletes the specified trill phrase and moves all phrases that follow back in the array one position
void eof_track_delete_tremolo(EOF_SONG *sp, unsigned long track, unsigned long index);	//Deletes the specified tremolo phrase and moves all phrases that follow back in the array one position
void eof_track_delete_arpeggio(EOF_SONG *sp, unsigned long track, unsigned long index);	//Deletes the specified arpeggio phrase and moves all phrases that follow back in the array one position
void eof_track_delete_slider(EOF_SONG *sp, unsigned long track, unsigned long index);	//Deletes the specified slider phrase and moves all phrases that follow back in the array one position
unsigned long eof_get_num_lyric_sections(EOF_SONG *sp, unsigned long track);	//Returns the number of lyric sections in the specified track, or 0 on error
EOF_PHRASE_SECTION *eof_get_lyric_section(EOF_SONG *sp, unsigned long track, unsigned long sectionnum);	//Returns a pointer to the specified lyric section, or NULL on error
void *eof_copy_note(EOF_SONG *sp, unsigned long sourcetrack, unsigned long sourcenote, unsigned long desttrack, unsigned long pos, long length, char type);
	//Copies the specified note to the specified track as a new note, returning a pointer to the newly created note structure, or NULL on error
	//The specified position, length and type are applied to the new note.  Other note variables such as the bitmask/pitch and name/lyric text are copied as-is
	//If the source is a pro guitar track and the destination is not, the source note's legacy bitmask is used if defined
	//If the source and destination are both pro guitar tracks, the source note's fret array, finger array, ghost bitmask, legacy bitmask, bend strength and slide end position are copied
long eof_get_prev_note_type_num(EOF_SONG *sp, unsigned long track, unsigned long note);
	//Returns the note immediately before the specified that is in the same difficulty, provided that the notes are sorted chronologically, or -1 if no such note exists
void eof_adjust_note_length(EOF_SONG * sp, unsigned long track, unsigned long amount, int dir);
	//If dir is >= 0, increases all selected notes in the active instrument difficulty by the specified amount
	//If dir is < 0, decreases all selected notes in the active instrument difficulty by the specified amount
	//If amount is 0, then the notes are adjusted by the current grid snap value
	//An undo state is only created if at least one note's length is altered
long eof_fixup_next_note(EOF_SONG *sp, unsigned long track, unsigned long note);	//Returns the note one after the specified note number that is in the same difficulty, or -1 if there is none
unsigned long eof_get_note_max_length(EOF_SONG *sp, unsigned long track, unsigned long note);
	//Determines the maximum valid length for the specified note by comparing its position to the next note that marks the threshold,
	//minus the configured minimum distance between notes.  If the specified note is a crazy note, the threshold is based on the
	//position of the next note in the same track difficulty that uses any gems on the same lane, otherwise it's based on that of the
	//next note in the track difficulty.
	//0 is returned on error, ULONG_MAX is returned if there is no note that follows (indicating the note's length is only limited by its variable capacity)
void eof_erase_track_content(EOF_SONG *sp, unsigned long track, unsigned char diff, char diffonly);
	//If diffonly is zero, the entire specified track has its contents deleted
	//If diffonly is nonzero, only the content from the specified track difficulty is deleted
void eof_erase_track(EOF_SONG *sp, unsigned long track);
	//Calls eof_erase_track_content() with the option to erase the entire track
void eof_erase_track_difficulty(EOF_SONG *sp, unsigned long track, unsigned char diff);
	//Calls eof_erase_track_content() with the option to erase the specified track difficulty only

EOF_NOTE * eof_legacy_track_add_note(EOF_LEGACY_TRACK * tp);	//Allocates, initializes and stores a new EOF_NOTE structure into the notes array.  Returns the newly allocated structure or NULL upon error
void eof_legacy_track_delete_note(EOF_LEGACY_TRACK * tp, unsigned long note);	//Removes and frees the specified note from the notes array.  All notes after the deleted note are moved back in the array one position
void eof_legacy_track_sort_notes(EOF_LEGACY_TRACK * tp);	//Performs a quicksort of the notes array
int eof_song_qsort_legacy_notes(const void * e1, const void * e2);	//The comparitor function used to quicksort the legacy notes array
long eof_fixup_previous_legacy_note(EOF_LEGACY_TRACK * tp, unsigned long note);	//Returns the note one before the specified note number that is in the same difficulty, or -1 if there is none
long eof_fixup_next_legacy_note(EOF_LEGACY_TRACK * tp, unsigned long note);	//Returns the note one after the specified note number that is in the same difficulty, or -1 if there is none
void eof_legacy_track_fixup_notes(EOF_SONG *sp, unsigned long track, int sel);	//Performs cleanup of the specified instrument track.  If sel is zero, the currently selected note is deselected automatically
int eof_legacy_track_add_star_power(EOF_LEGACY_TRACK * tp, unsigned long start_pos, unsigned long end_pos);	//Adds a star power phrase at the specified start and stop timestamp for the specified track.  Returns nonzero on success
void eof_legacy_track_delete_star_power(EOF_LEGACY_TRACK * tp, unsigned long index);	//Deletes the specified star power phrase and moves all phrases that follow back in the array one position
int eof_legacy_track_add_solo(EOF_LEGACY_TRACK * tp, unsigned long start_pos, unsigned long end_pos);	//Adds a solo phrase at the specified start and stop timestamp for the specified track.  Returns nonzero on success
void eof_legacy_track_delete_solo(EOF_LEGACY_TRACK * tp, unsigned long index);	//Deletes the specified solo phrase and moves all phrases that follow back in the array one position
int eof_legacy_track_add_trill(EOF_LEGACY_TRACK * tp, unsigned long start_pos, unsigned long end_pos);	//Adds a trill phrase at the specified start and stop timestamp for the specified track.  Returns nonzero on success
void eof_legacy_track_delete_trill(EOF_LEGACY_TRACK * tp, unsigned long index);		//Deletes the specified trill phrase and moves all phrases that follow back in the array one position
int eof_legacy_track_add_tremolo(EOF_LEGACY_TRACK * tp, unsigned long start_pos, unsigned long end_pos);	//Adds a tremolo phrase at the specified start and stop timestamp for the specified track.  Returns nonzero on success
void eof_legacy_track_delete_tremolo(EOF_LEGACY_TRACK * tp, unsigned long index);	//Deletes the specified tremolo phrase and moves all phrases that follow back in the array one position

EOF_LYRIC * eof_vocal_track_add_lyric(EOF_VOCAL_TRACK * tp);	//Allocates, initializes and stores a new EOF_LYRIC structure into the lyrics array.  Returns the newly allocated structure or NULL upon error
void eof_vocal_track_delete_lyric(EOF_VOCAL_TRACK * tp, unsigned long lyric);	//Removes and frees the specified lyric from the lyrics array.  All lyrics after the deleted lyric are moved back in the array one position
void eof_vocal_track_sort_lyrics(EOF_VOCAL_TRACK * tp);		//Performs a quicksort of the lyrics array
int eof_song_qsort_lyrics(const void * e1, const void * e2);	//The comparitor function used to quicksort the lyrics array
long eof_fixup_previous_lyric(EOF_VOCAL_TRACK * tp, unsigned long lyric);	//Returns the previous lyric, or -1 if there is none
long eof_fixup_next_lyric(EOF_VOCAL_TRACK * tp, unsigned long lyric);	//Returns the next lyric, or -1 if there is none
void eof_vocal_track_fixup_lyrics(EOF_SONG *sp, unsigned long track, int sel);	//Performs cleanup of the specified lyric track (based on the currently loaded audio and chart).  If sel is zero, the currently selected note is deselected automatically.
int eof_vocal_track_add_star_power(EOF_VOCAL_TRACK * tp, unsigned long start_pos, unsigned long end_pos);	//Marks all lyric phrases within the specified time span for overdrive.  Returns nonzero on success
int eof_vocal_track_add_line(EOF_VOCAL_TRACK * tp, unsigned long start_pos, unsigned long end_pos);	//Adds a lyric phrase at the specified start and stop timestamp for the specified track.  Returns nonzero on success
void eof_vocal_track_delete_line(EOF_VOCAL_TRACK * tp, unsigned long index);	//Deletes the specified lyric phrase and moves all phrases that follow back in the array one position

EOF_PRO_GUITAR_NOTE *eof_pro_guitar_track_add_note(EOF_PRO_GUITAR_TRACK *tp);	//Allocates, initializes and stores a new EOF_PRO_GUITAR_NOTE structure into the notes array.  Returns the newly allocated structure or NULL upon error
void eof_pro_guitar_track_sort_notes(EOF_PRO_GUITAR_TRACK * tp);	//Performs a quicksort of the notes array
int eof_song_qsort_pro_guitar_notes(const void * e1, const void * e2);	//The comparitor function used to quicksort the pro guitar notes array
void eof_pro_guitar_track_delete_note(EOF_PRO_GUITAR_TRACK * tp, unsigned long note);	//Removes and frees the specified note from the notes array.  All notes after the deleted note are moved back in the array one position
long eof_fixup_previous_pro_guitar_note(EOF_PRO_GUITAR_TRACK * tp, unsigned long note);	//Returns the note one before the specified note number that is in the same difficulty, or -1 if there is none
long eof_fixup_next_pro_guitar_note(EOF_PRO_GUITAR_TRACK * tp, unsigned long note);	//Returns the note one after the specified note number that is in the same difficulty, or -1 if there is none
void eof_pro_guitar_track_fixup_notes(EOF_SONG *sp, unsigned long track, int sel);	//Performs cleanup of the specified instrument track.  If sel is zero, the currently selected note is deselected automatically.
int eof_pro_guitar_track_add_star_power(EOF_PRO_GUITAR_TRACK * tp, unsigned long start_pos, unsigned long end_pos);	//Adds a star power phrase at the specified start and stop timestamp for the specified track.  Returns nonzero on success
void eof_pro_guitar_track_delete_star_power(EOF_PRO_GUITAR_TRACK * tp, unsigned long index);	//Deletes the specified star power phrase and moves all phrases that follow back in the array one position
int eof_pro_guitar_track_add_solo(EOF_PRO_GUITAR_TRACK * tp, unsigned long start_pos, unsigned long end_pos);	//Adds a solo phrase at the specified start and stop timestamp for the specified track.  Returns nonzero on success
void eof_pro_guitar_track_delete_solo(EOF_PRO_GUITAR_TRACK * tp, unsigned long index);	//Deletes the specified solo phrase and moves all phrases that follow back in the array one position
int eof_pro_guitar_track_add_trill(EOF_PRO_GUITAR_TRACK * tp, unsigned long start_pos, unsigned long end_pos);	//Adds a trill phrase at the specified start and stop timestamp for the specified track.  Returns nonzero on success
void eof_pro_guitar_track_delete_trill(EOF_PRO_GUITAR_TRACK * tp, unsigned long index);	//Deletes the specified trill phrase and moves all phrases that follow back in the array one position
int eof_pro_guitar_track_add_tremolo(EOF_PRO_GUITAR_TRACK * tp, unsigned long start_pos, unsigned long end_pos, unsigned char diff);
	//Adds a tremolo phrase at the specified start and stop timestamp for the specified track.  Returns nonzero on success
	//If diff is not 0xFF, the tremolo phrase will apply to the specified track difficulty only
void eof_pro_guitar_track_delete_tremolo(EOF_PRO_GUITAR_TRACK * tp, unsigned long index);	//Deletes the specified tremolo phrase and moves all phrases that follow back in the array one position
void eof_set_pro_guitar_fret_number(char function, unsigned long fretvalue);
	//Alters each selected pro guitar note's fret values on used strings (that match the eof_pro_guitar_fret_bitmask bitmask) based on the parameters:
	//If function is 0, the applicable strings' fret values are set to the specified value
	//If function is 1, the applicable strings' fret values are incremented
	//If function is 2, the applicable strings' fret values are decremented
int eof_detect_string_gem_conflicts(EOF_PRO_GUITAR_TRACK *tp, unsigned long newnumstrings);
	//If there are any gems on a string higher than the specified number of strings for the specified track, the highest used string number is returned
	//0 is returned if there are no conflicts
	//-1 is returned on error
unsigned long eof_get_pro_guitar_note_note(EOF_PRO_GUITAR_TRACK *tp, unsigned long note);		//Returns the note bitflag of the specified pro guitar note, or 0 on error
void eof_pro_guitar_track_sort_fret_hand_positions(EOF_PRO_GUITAR_TRACK* tp);	//Sorts the specified tracks fret hand positions by difficulty and then by timestamp
void eof_pro_guitar_track_delete_hand_position(EOF_PRO_GUITAR_TRACK *tp, unsigned long index);	//Deletes the specified fret hand position

void eof_sort_notes(EOF_SONG *sp);	//Sorts the notes in all tracks
void eof_fixup_notes(EOF_SONG *sp);	//Performs cleanup of the note selection, beats and all tracks
unsigned char eof_detect_difficulties(EOF_SONG * sp, unsigned long track);
	//Sets the populated status indicator for the specified track's difficulty names by prefixing each populated difficulty name in the current track (stored in eof_note_type_name[], eof_vocal_tab_name[] and eof_dance_tab_name[]) with an asterisk
	//eof_track_diff_populated_status[] is also updated so that each populated difficulty results in the corresponding element number being nonzero
	//Returns the number of difficulties present in the specified track (ie. if the highest used difficulty is 9, 10 is returned because the numbering begins with 0), or 0 is returned upon error or empty track
	//If the specified track is also the active track, the program window title is redrawn to reflect the current populated status of the active track difficulty

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
	//Checks all notes in the track, from #notenum to the last, that are at the specified note's timestamp
	//for the specified flag.  If any of the notes have the flag set, nonzero is returned
	//This is used for writing RB3 style pro drum phrases during MIDI export
	//The track's notes array is expected to be sorted
void eof_set_flags_at_legacy_note_pos(EOF_LEGACY_TRACK *tp,unsigned notenum,unsigned long flag,char operation,char startpoint);
	//Sets or clears the specified flag on all notes at the specified note's timestamp
	//If startpoint is 0, the process will start from the first note in the track with note #notenum's position
	//	otherwise the process will start from note #notenum
	//If the calling function is processing sorted notes in ascending order without filtering by difficulty or selected status, startpoint can be set to nonzero to save needless processing
	//If operation is 0, the specified flag is cleared on applicable notes
	//If operation is 1, the specified flag is set on applicable notes
	//If operation is 2, the specified flag is toggled on applicable notes
	//The track's notes array is expected to be sorted

int eof_load_song_string_pf(char *const buffer, PACKFILE *fp, const size_t buffersize);
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

inline int eof_open_strum_enabled(unsigned long track);
	//A simple function returning nonzero if the specified track has open strumming enabled
int eof_create_image_sequence(char benchmark_only);
	//Creates a PCX format image sequence in a subfolder of the chart's folder called "\sequence"
	//If benchmark_only is nonzero, image files are not written and the rendering performance (in frames per second) is displayed in the title bar
int eof_write_image_sequence(void);
	//Calls eof_create_image_sequence() with the option to save the images to disk

int eof_get_pro_guitar_note_fret_string(EOF_PRO_GUITAR_TRACK *tp, unsigned long note, char *pro_guitar_string);
	//Writes a string representation of the specified pro guitar/bass note from lowest to highest gauge string into pro_guitar_string[] which must be at least
	//3 * # of strings number of bytes long in order to store the maximum length string
	//Returns 0 on error or 1 on success
inline int eof_five_lane_drums_enabled(void);
	//A simple function returning nonzero if PART DRUM has the fifth lane enabled

char eof_track_has_cymbals(EOF_SONG *sp, unsigned long track);
	//Returns nonzero if the specified track is a drum track that contains notes marked as cymbals

char eof_search_for_note_near(EOF_SONG *sp, unsigned long track, unsigned long targetpos, unsigned long delta, char type, unsigned long *match);
	//Looks for one or more notes within [delta] number of milliseconds of the specified position of the specified track difficulty
	//If such a note is found, nonzero is returned and match refers to the note number of the closest note
	//If two notes are an equal distance from the target time (one before, one after), the earlier note is returned
int eof_thin_notes_to_match_target_difficulty(EOF_SONG *sp, unsigned long sourcetrack, unsigned long targettrack, unsigned long delta, char type);
	//Checks all notes in the target track difficulty
	//For each note, if it isn't within [delta] number of milliseconds of a note in the source track difficulty, it is deleted
	//Returns zero on error
unsigned long eof_get_highest_fret(EOF_SONG *sp, unsigned long track, char scope);
	//Examines notes in the specified track based on the scope value (0 = all notes, nonzero = selected notes)
	//Returns the highest used fret value of notes in scope, or 0 if all such notes are muted with no fret specified
unsigned long eof_get_highest_clipboard_fret(char *clipboardfile);
	//Parses the notes in the clipboard file, returning the highest used fret among them (or 0 if all such notes are muted with no fret specified)
unsigned long eof_get_highest_clipboard_lane(char *clipboardfile);
	//Parses the notes in the clipboard file, returning the highest used lane among them (or 0 if all such notes have no gems, ie. corrupted clipboard)
unsigned long eof_get_highest_fret_value(EOF_SONG *sp, unsigned long track, unsigned long note);
	//Returns the highest used fret in the specified pro guitar note
	//If the parameters are invalid or the specific pro guitar note's gems are all muted with no fret specified, 0 is returned

unsigned long eof_determine_chart_length(EOF_SONG *sp);
	//Parses the project and returns the ending position of the last last note/lyric/text event/bookmark
void eof_truncate_chart(EOF_SONG *sp);
	//Uses eof_determine_chart() to set eof_chart_length to the larger of the last chart content and the loaded chart audio
	//Any beats that are more than one beat after this position are deleted from the project
	//For this reason, eof_selected_beat is validated and will be reset to 0 if it refers to a beat that doesn't exist anymore
	//The project is likewise padded with such number of beats if there aren't that many
	//This should be called after Guitar Pro import (which adds beats even if no tracks are selected for import), after loading a project/file/undo/redo state (before calling cleanup logic) and after loading an audio file

int eof_check_if_notes_exist_beyond_audio_end(EOF_SONG *sp);
	//Checks whether any notes/lyrics in the chart extend beyond the end of the chart audio
	//If so, the first offending track number is returned.  Otherwise, or if no audio is loaded, zero is returned.

void eof_flatten_difficulties(EOF_SONG *sp, unsigned long srctrack, unsigned char srcdiff, unsigned long desttrack, unsigned char destdiff, unsigned long threshold);
	//Builds a cumulative set of notes in the specified track that exist at or below the specified source track difficulty, placing them in specified destination track difficulty
	//The set is built by using the highest difficulty note at each position (notes within the threshold number of milliseconds are considered to be in the same position)
	//The input track is expected to be authored in the style of Rocksmith, where notes in one difficulty replace or add to the notes in the lower difficulty
	//The resulting notes are suitable for a flat difficulty system (like that used in Guitar Hero or Rock Band)
void eof_track_add_or_remove_track_difficulty_content_range(EOF_SONG *sp, unsigned long track, unsigned char diff, unsigned long startpos, unsigned long endpos, int function, int prompt, char *undo_made);
	//Modifies notes in the specified time range of the specified track difficulty according to the specified function
	//If the specified track is a pro guitar track, arpeggios, hand positions and tremolos in the specified time range are also modified accordingly
	//If function is negative, the items in the track difficulty are deleted, and the difficulty of those of higher difficulties are decremented, effectively leveling down that range of the track
	//If function is >= 0, the items in the track difficulty are duplicated into the next difficulty, and those of higher difficulties are incremented, effectively leveling up that range of the track
	//If prompt is zero, the function prompts the user whether to re-align notes that are up to 10ms before the start position (which is described to the user as a "phrase").
	//If prompt is one, the function suppresses the prompt and assumes a yes response.  If prompt is two, the function suppresses the prompt and assumes a no response.
	//If prompt is three, this check is skipped
	//If *undo_made is zero, an undo state is made before the function modifies the chart
void eof_unflatten_difficulties(EOF_SONG *sp, unsigned long track);
	//Converts from a "flat" difficulty system (like Rock Band) to an additive one (like Rocksmith),
	// where each difficulty only defines RS phrases that are different than the previous difficulty
	//Parses from, highest to lowest difficulty, all Rocksmith phrases defined in the specified track
	//If the notes in one RS phrase's difficulty matches those in the previous difficulty,
	// the phrase's notes in the upper difficulty are deleted
	//Expects the beat statistics and eof_track_diff_populated_status[] array to already reflect the track being processed

void eof_hightlight_all_notes_above_fret_number(EOF_SONG *sp, unsigned long track, unsigned char fretnum);
	//Sets the highlight status on all notes in the specified track that use any fret higher than the specified number
void eof_track_remove_highlighting(EOF_SONG *sp, unsigned long track);
	//Removes the highlight status on all notes in the specified track

#endif
