#ifndef EOF_SONG_H
#define EOF_SONG_H

#include <allegro.h>
#include "alogg/include/alogg.h"

///Item limits
#define EOF_MAX_TEXT_EVENTS       4096
#define EOF_MAX_GRID_SNAP_INTERVALS 97
#define EOF_NAME_LENGTH             30
#define EOF_SECTION_NAME_LENGTH     50
#define EOF_MAX_BEATS            32768
#define EOF_MAX_PHRASES           1000
#define EOF_MAX_OGGS                 8
#define EOF_OLD_MAX_NOTES        65536
#define EOF_MAX_NOTES            32768
#define EOF_MAX_LYRICS   EOF_MAX_NOTES
#define EOF_MAX_LYRIC_LINES       4096
#define EOF_MAX_LYRIC_LENGTH       255
#define EOF_MAX_CATALOG_ENTRIES    256
#define EOF_MAX_INI_SETTINGS        64
#define EOF_INI_LENGTH             512
#define EOF_MAX_BOOKMARK_ENTRIES    10


///Note flags
//The original 32 status flags are all used up by some common and some track-specific statuses
#define EOF_NOTE_FLAG_HOPO       1	//This flag will be set by eof_determine_phrase_status() if the note displays as a HOPO
#define EOF_NOTE_FLAG_SP         2	//This flag will be set by eof_determine_phrase_status() if the note is in a star power section
#define EOF_NOTE_FLAG_CRAZY      4	//This flag will represent overlap allowed for guitar/dance/keys tracks, and will force pro guitar/bass chords to display with a chord box
#define EOF_NOTE_FLAG_F_HOPO     8
#define EOF_NOTE_FLAG_NO_HOPO    16
#define EOF_NOTE_FLAG_HIGHLIGHT  1073741824	//This flag will represent a note that is highlighted (in yellow, by default) in the editor window (permanently, until manually cleared)
#define EOF_NOTE_FLAG_EXTENDED 2147483648UL	//The MSB will be set if an additional flag variable is present for the note in the project file
											//This flag will only be used during project save/load to determine whether another flags variable is written/read

//The following flags pertain to pro guitar notes
#define EOF_PRO_GUITAR_NOTE_FLAG_ACCENT         32			//This flag will represent a note that is played as an accent
#define EOF_PRO_GUITAR_NOTE_FLAG_P_HARMONIC     64			//This flag will represent a note that is played as a pinch harmonic
#define EOF_PRO_GUITAR_NOTE_FLAG_LINKNEXT       128			//This flag will represent a note that is linked to the next note in the track difficulty
#define EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE  256			//This flag will represent a note that has an unpitched slide
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
#define EOF_PRO_GUITAR_NOTE_FLAG_HD             268435456	//This flag will represent a note that exports to Rocksmith with high density (ie. a chord repeat line)
#define EOF_PRO_GUITAR_NOTE_FLAG_SPLIT          536870912	//This flag will represent a chord that exports to Rocksmith as single notes instead of a chord

//The following flags pertain to drum notes
#define EOF_DRUM_NOTE_FLAG_Y_CYMBAL         32		//This flag represents a yellow drum note charted as a RB3 Pro style cymbal (lane 3)
#define EOF_DRUM_NOTE_FLAG_B_CYMBAL         64		//This flag represents a blue drum note charted as a RB3 Pro style cymbal (lane 4)
#define EOF_DRUM_NOTE_FLAG_G_CYMBAL         128		//This flag represents a green drum note charted as a RB3 Pro style cymbal (lane 5)
#define EOF_DRUM_NOTE_FLAG_DBASS            256		//This flag will represent Expert+ bass drum for the drum track (lane 1)
#define EOF_DRUM_NOTE_FLAG_Y_HI_HAT_OPEN	512		//This flag means the yellow cymbal will be displayed in Phase Shift as an open hi hat (lane 3)
#define EOF_DRUM_NOTE_FLAG_Y_HI_HAT_PEDAL	1024	//This flag means the yellow cymbal will be displayed in Phase Shift as a pedal controlled hi hat (lane 3)
#define EOF_DRUM_NOTE_FLAG_R_RIMSHOT		2048	//This flag means the red drum note will be displayed in Phase Shift as a rim shot (lane 2)
#define EOF_DRUM_NOTE_FLAG_Y_SIZZLE			4096	//This flag means the yellow cymbal will be displayed in Phase Shift as a "sizzle" (partially open) hi hat (lane 3)
#define EOF_DRUM_NOTE_FLAG_Y_COMBO          8192	//This flag means the yellow drum note is treated as both a cymbal gem and a tom gem (for use in the Phase Shift drum track)
#define EOF_DRUM_NOTE_FLAG_B_COMBO         16384	//This flag means the blue drum note is treated as both a cymbal gem and a tom gem (for use in the Phase Shift drum track)
#define EOF_DRUM_NOTE_FLAG_G_COMBO         32768	//This flag means the green drum note is treated as both a cymbal gem and a tom gem (for use in the Phase Shift drum track)

//The following flags pertain to dance notes
#define EOF_DANCE_FLAG_LANE_1_MINE		512		//This flag will represent a mine note for lane 1
#define EOF_DANCE_FLAG_LANE_2_MINE		1024	//This flag will represent a mine note for lane 2
#define EOF_DANCE_FLAG_LANE_3_MINE		2048	//This flag will represent a mine note for lane 3
#define EOF_DANCE_FLAG_LANE_4_MINE		4096	//This flag will represent a mine note for lane 4

//The following flags pertain to legacy guitar notes
#define EOF_GUITAR_NOTE_FLAG_IS_SLIDER	512		//This flag will be set by eof_determine_phrase_status() if the note is in a slider section
#define EOF_GUITAR_NOTE_FLAG_GHL_OPEN   1024	//This flag will represent an open note in a Guitar Hero Live style track

//The following flags pertain to legacy and pro guitar notes
#define EOF_NOTE_FLAG_IS_TRILL		            65536	//This flag will be set by eof_determine_phrase_status() if the note is in a trill section
#define EOF_NOTE_FLAG_IS_TREMOLO		        131072	//This flag will be set by eof_determine_phrase_status() if the note is in a tremolo section


///Temporary note flags
//The following temporary flags are maintained internally and do not save to file (even during project save, clipboard, auto-adjust, etc.)
#define EOF_NOTE_TFLAG_TEMP         1	//This flag will represent a temporary status, such as a note that was generated for temporary use that will be removed
#define EOF_NOTE_TFLAG_IGNORE       2	//This flag will represent a note that is not exported to XML (such as a chord within an arpeggio that is converted into single notes)
#define EOF_NOTE_TFLAG_ARP          4	//This flag will represent a note that is within an arpeggio, for RS export of arpeggio/handshape phrases as handshape tags
#define EOF_NOTE_TFLAG_HAND         8	//This flag will represent a note that is within a handshape phrase, which is treated as a variation of an arpeggio, affecting export to RS2 XML
#define EOF_NOTE_TFLAG_ARP_FIRST   16	//This flag will represent a note that is the first note within its arpeggio phrase
#define EOF_NOTE_TFLAG_SORT        32	//This flag is applied to selected notes by eof_track_sort_notes() to allow it to recreate the note selection after sorting
#define EOF_NOTE_TFLAG_GHOST_HS    64	//This flag will represent a note that is added during RS2 export that is observed during the chord list building and handshape exports, but ignored otherwise
#define EOF_NOTE_TFLAG_TWIN       128	//This flag will represent a note that is either the original or ghost gem-less clone of a partial ghosted chord that is created during RS2 export
#define EOF_NOTE_TFLAG_COMBINE    256	//This flag will represent a note that was marked as ignored because its sustain is to be combined with that of a chordnote during RS2 export
#define EOF_NOTE_TFLAG_NO_LN      512	//This flag will indicate that the linknext status of the affected note is to be interpreted to be not set, due to how chordnotes and linked single notes can be combined, during RS2 export
#define EOF_NOTE_TFLAG_CCHANGE   1024	//This flag will indicate that a note is a chord change from RS import's perspective (for determining manually defined handshape phrases)
#define EOF_NOTE_TFLAG_HIGHLIGHT 2048	//This flag will represent a note that is highlighted (in cyan by default, instead of the yellow used for static highlighting) in the editor window (non permanent, such as for toggleable highlighting options)
#define EOF_NOTE_TFLAG_MINLENGTH 4096	//This flag will indicate that a temporary ignored note added for the chordnote mechanism needs to RS2 export with a minimum length of 1ms
#define EOF_NOTE_TFLAG_LN        8192	//This flag will indicate that the affected chord has chordify status and the chord tag should RS2 export with the linknext tag overridden to be enabled, which will cause the chord to link to the temporary single notes written for the chord
#define EOF_NOTE_TFLAG_HD       16384	//This flag will indicate that the affected chord should export with high density regardless of the value of the regular high density flag
#define EOF_NOTE_TFLAG_GHL_B3   32768	//This flag will indicate that the affected note is a "N 8 #" black 3 note being imported from a Feedback file instead of "N 5 #" toggle HOPO notation
#define EOF_NOTE_TFLAG_RESNAP   65536	//This flag will indicate that a note was defined as grid snapped in the imported MIDI, and that it should be resnapped if rounding errors result in it not being grid snapped after import


///Extended note flags
//Extended flags are track specific and should not be retained when copied to a track of a different format
//The following extended flags pertain to pro guitar notes
#define EOF_PRO_GUITAR_NOTE_EFLAG_IGNORE      1	//This flag specifies a note that will export to RS2 format with the "ignore" status set to nonzero, for special uses
#define EOF_PRO_GUITAR_NOTE_EFLAG_SUSTAIN     2	//This flag specifies a note that will export to RS2 format with its sustain even when it's a chord without techniques that normally require chordNote tags
#define EOF_PRO_GUITAR_NOTE_EFLAG_STOP        4	//This flag specifies a tech note that truncates the overlapped string's note at its position
#define EOF_PRO_GUITAR_NOTE_EFLAG_GHOST_HS    8	//This flag specifies a note that will export to RS2 format so that the chord tag written reflects ghost gems having been filtered out as per normal,
												//	but the handshape tag written reflecting a chord where the ghost gems are included.  This allows the author to have multiple partial chords display
												//	as if they all used the full chord's handshape in-game
#define EOF_PRO_GUITAR_NOTE_EFLAG_CHORDIFY   16	//This flag specifies a note with "chordify" status, affecting its export to RS2 XML as a chord tag with no chordnote subtags, and with ignored single notes if the chord uses sustain
#define EOF_PRO_GUITAR_NOTE_EFLAG_FINGERLESS 32	//This flag specifies that a chord has no defined fingering and will RS export reflecting as such
#define EOF_PRO_GUITAR_NOTE_EFLAG_PRE_BEND   64	//This flag specifies that a bend tech note is to be interpreted as a pre-bend even if it's not at the start position of the normal note it affects


///Beat flags
#define EOF_BEAT_FLAG_ANCHOR       1
#define EOF_BEAT_FLAG_EVENTS       2
#define EOF_BEAT_FLAG_START_4_4    4
#define EOF_BEAT_FLAG_START_3_4    8
#define EOF_BEAT_FLAG_START_5_4   16
#define EOF_BEAT_FLAG_START_6_4   32
#define EOF_BEAT_FLAG_CUSTOM_TS   64	//If this is nonzero, indicates that the first and second most significant bytes of the beat's flags store the TS numerator and denominator, respectively
#define EOF_BEAT_FLAG_KEY_SIG    128
#define EOF_BEAT_FLAG_MIDBEAT    256	//If this is nonzero, indicates that a beat was inserted to accommodate a mid-beat tempo change (during Feedback import)
#define EOF_BEAT_FLAG_START_2_4  512
#define EOF_BEAT_FLAG_EXTENDED 32768	//Reserve the highest unused bit to allow for another beat flag to be conditionally present


///Phrase flags
#define EOF_LYRIC_LINE_FLAG_OVERDRIVE 1
#define EOF_RS_ARP_HANDSHAPE          2	//A modifier for arpeggio sections that will cause the section to export to XML without "-arp" appended to the chord template name
										//It will also not force notes within the phrase to have crazy status or for chords to split into single notes


typedef struct
{
	char name[EOF_NAME_LENGTH + 1];
	unsigned char type;			//Stores the note's difficulty
	unsigned char note;			//Stores the note's fret values
	unsigned char accent;		//Stores the note's accent bitmask
	unsigned long midi_pos;
	unsigned long midi_length;
	unsigned long pos;
	long length;				//Keep as signed, since the npos logic uses signed math
	unsigned long flags;		//Stores various note statuses
	unsigned long tflags;		//Stores various temporary statuses

} EOF_NOTE;

typedef struct
{
	char name[EOF_NAME_LENGTH + 1];
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
	unsigned long eflags;		//Stores extended note statuses
	unsigned char bendstrength;	//The amount this note bends (0 if undefined or not applicable for regular notes, can be 0 for tech notes as that defines the release of a bend).  If the MSB is set, the value specifies quarter steps, otherwise it specifies half steps
	unsigned char slideend;		//The fret at which this slide ends (0 if undefined or not applicable)
	unsigned char unpitchend;	//The fret at which this unpitched slide ends (0 if undefined or not applicable)
	unsigned long tflags;		//Stores various temporary statuses (not written to project file)

} EOF_PRO_GUITAR_NOTE;

typedef struct
{
	char name[EOF_MAX_LYRIC_LENGTH + 1];	//Should be the longer of the note structure's name length or the lyric structure's name length
	unsigned char type;
	unsigned char note;
	unsigned char accent;
	unsigned long endbeat;
	unsigned long beat; 		//Which beat this note was copied from
	unsigned long pos;
	long length;				//Keep as signed, since the npos logic uses signed math
	unsigned long midi_pos;
	long midi_length;			//Keep as signed, since the npos logic uses signed math
	double        porpos;		//Position of note within the beat (100.0 = full beat)
	double        porendpos;	//Position of the end of the note within the beat
	unsigned long flags;
	unsigned long eflags;
	unsigned char legacymask;
	unsigned char frets[8];
	unsigned char finger[8];
	unsigned char ghost;
	unsigned char bendstrength;
	unsigned char slideend;
	unsigned char unpitchend;
	unsigned long arpeggnum;	//The arpeggio/handshape phrase number this note is part of (or 0xFFFFFFFF if not applicable)

} EOF_EXTENDED_NOTE;

typedef struct
{
	unsigned char type;		//The lyric set this lyric belongs to (0=PART VOCALS, 1=HARM1, 2=HARM2...)
	unsigned char note;		//If zero, the lyric has no defined pitch
	char text[EOF_MAX_LYRIC_LENGTH + 1];
	unsigned long midi_pos;
	long midi_length;		//Keep as signed, since the npos logic uses signed math
	unsigned long pos;
	long length;			//Keep as signed, since the npos logic uses signed math
	unsigned long flags;
	unsigned long tflags;	//Stores various temporary statuses

} EOF_LYRIC;

typedef struct
{
	unsigned long midi_start_pos;
	unsigned long midi_end_pos;
	unsigned long start_pos;
	unsigned long end_pos;	//Will store other data in items that don't use an end position (such as the fret number for fret hand positions or whether a tone change is to the default tone)
	unsigned long flags;	//Store various phrase-specific data, such as a fret catalog entry's source track
	char name[EOF_SECTION_NAME_LENGTH + 1];
	unsigned char difficulty;	//The difficulty this phrase applies to (ie. arpeggios, hand positions, RS tremolos), or 0xFF if it otherwise applies to all difficulties

} EOF_PHRASE_SECTION;


///Track formats
#define EOF_LEGACY_TRACK_FORMAT					1
#define EOF_VOCAL_TRACK_FORMAT					2
#define EOF_PRO_KEYS_TRACK_FORMAT				3
#define EOF_PRO_GUITAR_TRACK_FORMAT				4
#define EOF_PRO_VARIABLE_LEGACY_TRACK_FORMAT	5
#define EOF_ANY_TRACK_FORMAT                  255


///Track behaviors
#define EOF_GUITAR_TRACK_BEHAVIOR		1
#define EOF_DRUM_TRACK_BEHAVIOR			2
#define EOF_VOCAL_TRACK_BEHAVIOR		3
#define EOF_KEYS_TRACK_BEHAVIOR			4
#define EOF_PRO_GUITAR_TRACK_BEHAVIOR	5
#define EOF_PRO_KEYS_TRACK_BEHAVIOR		6
#define EOF_DANCE_TRACK_BEHAVIOR        7


///Track numbers
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
#define EOF_TRACK_PRO_GUITAR_B	14
#define EOF_TRACK_PRO_KEYS		15


///Phrase numbers
#define EOF_SOLO_SECTION				1
#define EOF_SP_SECTION					2
#define EOF_BOOKMARK_SECTION			3
#define EOF_FRET_CATALOG_SECTION		4
#define EOF_LYRIC_PHRASE_SECTION		5
#define EOF_YELLOW_CYMBAL_SECTION       6	//Unused
#define EOF_BLUE_CYMBAL_SECTION         7	//Unused
#define EOF_GREEN_CYMBAL_SECTION        8	//Unused
#define EOF_TRILL_SECTION				9
#define EOF_ARPEGGIO_SECTION			10
#define EOF_TRAINER_SECTION				11	//Unused
#define EOF_CUSTOM_MIDI_NOTE_SECTION	12	//Unused
#define EOF_PREVIEW_SECTION				13	//Unused
#define EOF_TREMOLO_SECTION				14
#define EOF_SLIDER_SECTION				15
#define EOF_FRET_HAND_POS_SECTION       16
#define EOF_RS_POPUP_MESSAGE            17
#define EOF_RS_TONE_CHANGE              18
#define EOF_NUM_SECTION_TYPES           18


///Track flags
#define EOF_TRACK_FLAG_SIX_LANES		1
	//Specifies if the track has open strumming enabled (legacy bass or guitar tracks) or a fifth drum lane enabled (PART DRUMS)
#define EOF_TRACK_FLAG_ALT_NAME			2
	//Specifies if the track has an alternate display name (for RS export.  MIDI export will still use the native name)
#define EOF_TRACK_FLAG_UNLIMITED_DIFFS	4
	//Specifies a pro guitar track as not being limited to 4 difficulties plus one more for the BRE difficulty.  Higher numbered difficulties will be exported to Rocksmith format
#define EOF_TRACK_FLAG_GHL_MODE         8
	//Specifies a legacy guitar behavior track as having Guitar Hero Live characteristics (6 lanes plus an additional open note status)
#define EOF_TRACK_FLAG_RS_BONUS_ARR    16
	//Specifies that a pro guitar track is to be presented in Rocksmith as a bonus arrangement


///Difficulty numbers
#define EOF_NOTE_SUPAEASY    0
#define EOF_NOTE_EASY        1
#define EOF_NOTE_MEDIUM      2
#define EOF_NOTE_AMAZING     3
#define EOF_NOTE_SPECIAL     4
#define EOF_NOTE_CHALLENGE   4
#define EOF_MAX_DIFFICULTIES 5
	//Note: EOF_NOTE_SPECIAL references the same difficulty that the dance track uses for "Challenge" difficulty

#define EOF_TRACK_NAME_SIZE		31
typedef struct
{
	char track_format;					//Specifies which track format this is, using one of the macros above
	char track_behavior;				//Specifies which behavior this track follows, using one of the macros above
	char track_type;					//Specifies which type of track this is (ie default PART GUITAR, custom track, etc)
	unsigned long tracknum;				//Specifies which number of that type this track is, used as an index into the type-specific track arrays
	char name[EOF_NAME_LENGTH + 1];		//Specifies the name of the track
	char altname[EOF_NAME_LENGTH + 1];	//Specifies the alternate name of the track (for RS export and MIDI export of GHL tracks)
	unsigned char difficulty;			//Specifies the difficulty level from 0-5 (standard 0-5 scale), or 6 for devil heads (extreme difficulty).  0xFF means the difficulty is undefined
	unsigned char numdiffs;				//Specifies the number of difficulties usable in this track, including BRE (is set to 5 unless the track's EOF_TRACK_FLAG_UNLIMITED_DIFFS flag is set)
	unsigned long flags;				//Various flags.  In the case of the normal drum track and the vocal track, the low nibble of the most significant byte stores the difficulty level (or 0xF if undefined)
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

#define EOF_PRO_GUITAR_TRACKS_MAX	5
#define EOF_TUNING_LENGTH 6	//For now, the tuning array will only track 6 strings
#define EOF_NUM_DEFINED_CHORDS 37

typedef struct
{
	unsigned char numfrets;			//The number of frets in this track
	unsigned char numstrings;		//The number of strings/lanes in this track
	unsigned char arrangement;		//The arrangement type of this track  (0 = Undefined, 1 = Combo, 2 = Rhythm, 3 = Lead, 4 = Bass)
	unsigned char ignore_tuning;	//If nonzero, indicates that the chord name detection reflects the DEFAULT tuning for the arrangement, and not the track's actual specified tuning
	unsigned char capo;				//If nonzero, specifies the presence of a capo on the specified fret number, affecting the chord lookup logic
	char tuning[EOF_TUNING_LENGTH];	//An array with at least (numstrings) elements, each of which defines the string's relative tuning as the +/- number of half steps from standard tuning (tuning[0] refers to lane 1's string, which is low E)
	EOF_TRACK_ENTRY * parent;		//Allows an easy means to look up the global track using a pro guitar track pointer

	/* the active note array */
	EOF_PRO_GUITAR_NOTE ** note;	//This array pointer and count variable are assigned to point to either the pro guitar notes or the tech notes, depending on which the user is currently authoring
	unsigned long notes;

	/* regular pro guitar notes */
	EOF_PRO_GUITAR_NOTE * pgnote[EOF_MAX_NOTES];
	unsigned long pgnotes;

	/* tech notes */
	EOF_PRO_GUITAR_NOTE * technote[EOF_MAX_NOTES];
	unsigned long technotes;

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
	EOF_PHRASE_SECTION popupmessage[EOF_MAX_PHRASES];
	unsigned long popupmessages;

	/* tone changes */
	EOF_PHRASE_SECTION tonechange[EOF_MAX_PHRASES];
	unsigned long tonechanges;
	char defaulttone[EOF_SECTION_NAME_LENGTH + 1];

} EOF_PRO_GUITAR_TRACK;

typedef struct
{
	unsigned long ppqn;
	unsigned long midi_pos;
	unsigned long pos;
	unsigned long flags;	//If the EOF_BEAT_FLAG_CUSTOM_TS flag is set, this variable's MSB is the TS's numerator (value 1 is stored as all bits 0), the 2nd MSB is the TS's denominator (value 1 is stored as all bits 0)
	char key;	//If negative, the value defines the number of flats present, ie. -2 is Bb.  If positive, the value defines the number of sharps present, ie. 4 is E
	double fpos;

	//These variables, set with eof_process_beat_statistics(), track various properties of the beat to relieve various functions of related processing
	//If has_ts is nonzero, num_beats_in_measure, beat_unit and beat_within_measure will be defined.  beat_within_measure is numbered starting with 0
	//the contained...event variables are set to -1 if the beat does not contain a qualifying text event
	unsigned long measurenum;
	unsigned num_beats_in_measure, beat_unit, beat_within_measure;
	int contained_section_event, contained_rs_section_event, contained_rs_section_event_instance_number;
	char contains_tempo_change, contains_ts_change, contains_end_event, has_ts;

} EOF_BEAT_MARKER;

typedef struct
{
	char filename[256];
	long midi_offset;	//Leave signed just in case this is eventually used to allow for insertion of leading silence via specifying a negative midi offset
	char modified;
	char description[EOF_NAME_LENGTH + 1];

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
	char rs_export_suppress_dd_warnings;
	char unshare_drum_phrasing;
	char highlight_unsnapped_notes;
	char accurate_ts;
	char highlight_arpeggios;

	EOF_OGG_INFO ogg[EOF_MAX_OGGS];
	unsigned short oggs;

	char ini_setting[EOF_MAX_INI_SETTINGS][EOF_INI_LENGTH];
	unsigned short ini_settings;

	unsigned long revision;
	unsigned long difficulty;		//Specifies the difficulty level from 0-5 (standard 0-5 scale), or 6 for devil heads (extreme difficulty)

	unsigned long start_point, end_point;	//Used to track manually set start and end timestamps for operations such as partial chart export.  A value of ULONG_MAX indicates that the point is undefined

} EOF_SONG_TAGS;

typedef struct
{
	unsigned long track;
	unsigned char type;
	unsigned long start_pos;
	unsigned long end_pos;
	char name[EOF_NAME_LENGTH + 1];

} EOF_CATALOG_ENTRY;

#define EOF_TEXT_EVENT_LENGTH 255
typedef struct
{
	char text[EOF_TEXT_EVENT_LENGTH + 1];
	unsigned long beat;
	unsigned long track;	//The track this event is tied to, or 0 if it goes into the EVENTS track (such as a generic section marker)
	char is_temporary;		//This is nonzero if the event is considered temporary (doesn't trigger undo/redo when added/deleted), required RBN events are added this way during save
	unsigned char flags;
	unsigned long index;	//Populated with the event's index in eof_sort_events(), since the quicksort algorithm can and will re-order list items that have equivalent sorting order

} EOF_TEXT_EVENT;

#define EOF_EVENT_FLAG_RS_PHRASE      1
#define EOF_EVENT_FLAG_RS_SECTION     2
#define EOF_EVENT_FLAG_RS_EVENT       4
#define EOF_EVENT_FLAG_RS_SOLO_PHRASE 8

typedef struct
{
	EOF_CATALOG_ENTRY entry[EOF_MAX_CATALOG_ENTRIES];
	unsigned long entries;

} EOF_CATALOG;

#define EOF_TRACKS_MAX	(EOF_LEGACY_TRACKS_MAX + EOF_VOCAL_TRACKS_MAX + EOF_PRO_GUITAR_TRACKS_MAX + 1)
//Add 1 to reflect that track[0] is added but not used
#define EOF_POWER_GIG_TRACKS_MAX (15 + 1)
#define EOF_GUITAR_HERO_ANIMATION_TRACKS_MAX (2 + 1)

extern EOF_TRACK_ENTRY eof_default_tracks[EOF_TRACKS_MAX + 1];
	//The list of default tracks that should be presented in EOF
extern EOF_TRACK_ENTRY eof_midi_tracks[EOF_TRACKS_MAX + 16];
	//The list of MIDI track names pertaining to each instrument and harmony track
extern EOF_TRACK_ENTRY eof_power_gig_tracks[EOF_POWER_GIG_TRACKS_MAX];
extern EOF_TRACK_ENTRY eof_guitar_hero_animation_tracks[EOF_GUITAR_HERO_ANIMATION_TRACKS_MAX];

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

	/* MIDI "resolution" (time division, currently unused) used to determine how MIDI is exported,
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
	char fpbeattimes;	//Is set to nonzero if floating point beat timings were read from the project file during load, allowing a precision lossy call to eof_calculate_beats() to be avoided in eof_init_after_load()

} EOF_SONG;

EOF_SONG * eof_create_song(void);	//Allocates, initializes and returns an EOF_SONG structure
void eof_destroy_song(EOF_SONG * sp);	//De-allocates the memory used by the EOF_SONG structure.  If eof_undo_in_progress is nonzero, the spectrogram and waveform data are destroyed if applicable.
int eof_load_song_pf(EOF_SONG * sp, PACKFILE * fp);	//Loads data from the specified PACKFILE pointer into the given EOF_SONG structure (called by eof_load_song()).  Returns 0 on error
EOF_SONG * eof_load_song(const char * fn);	//Loads the specified EOF file, validating the file header and loading the appropriate OGG file
int eof_save_song(EOF_SONG * sp, const char * fn);	//Saves the song to file.  Returns zero on error
EOF_SONG *eof_clone_chart_time_range(EOF_SONG *sp, unsigned long start, unsigned long end);	//Builds a new song structure containing the specified time range of content in the active project, or NULL on error

unsigned long eof_get_track_size(EOF_SONG *sp, unsigned long track);						//Returns the number of notes/lyrics in the specified track (or just that of its active note set if a pro guitar track is specified), or 0 on error
unsigned long eof_get_track_size_all(EOF_SONG *sp, unsigned long track);					//For pro guitar tracks, returns the sum of the note count of both the active and tech note sets, otherwise returns the result of eof_get_track_size()
unsigned long eof_get_track_size_normal(EOF_SONG *sp, unsigned long track);					//For pro guitar tracks, returns the note count of the normal note set only, otherwise returns the result of eof_get_track_size()
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
void eof_track_delete_note_with_difficulties(EOF_SONG *sp, unsigned long track, unsigned long notenum, int operation);
	//Deletes the specified note as well as other notes in the same position in other difficulties, depending on the value of the operation parameter:
	// if operation is < 0, notes at the same timestamp as the specified note in lower difficulties are also deleted
	// if operation is 0, only the specified note is deleted
	// if operation is > 0, notes at the same timestamp as the specified note in ANY difficulty is deleted
	//The specified track's notes are pre-sorted in order to ensure this function operates properly
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
unsigned long eof_get_note_tflags(EOF_SONG *sp, unsigned long track, unsigned long note);	//Returns the temporary flags of the specified track's note/lyric, or 0 on error
void eof_set_note_tflags(EOF_SONG *sp, unsigned long track, unsigned long note, unsigned long tflags);	//Sets the temporary flags of the specified track's note/lyric
unsigned char eof_get_note_eflags(EOF_SONG *sp, unsigned long track, unsigned long note);	//Returns the extended flags of the specified pro guitar note, or 0 on error
void eof_set_note_eflags(EOF_SONG *sp, unsigned long track, unsigned long note, unsigned char eflags);	//Sets the extended flags of the specified pro guitar note
unsigned char eof_get_note_note(EOF_SONG *sp, unsigned long track, unsigned long note);		//Returns the note bitflag of the specified track's note/lyric, or 0 on error
unsigned char eof_get_note_ghost(EOF_SONG *sp, unsigned long track, unsigned long note);	//Returns the ghost bitflag of the specified pro guitar track's note/lyric, or 0 on error or if the specified track isn't a pro guitar track
void eof_set_note_note(EOF_SONG *sp, unsigned long track, unsigned long note, unsigned char value);
	//Sets the note value of the specified track's note/lyric
	//If the specified note is a pro guitar note, any unused strings have their corresponding fret values reset to 0
unsigned char eof_get_note_accent(EOF_SONG *sp, unsigned long track, unsigned long note);		//Returns the note bitflag of the specified track's note, or 0 on error
void eof_set_note_accent(EOF_SONG *sp, unsigned long track, unsigned long note, unsigned char value);	//Sets the accent bitmask value of the specified track's note
char *eof_get_note_name(EOF_SONG *sp, unsigned long track, unsigned long note);				//Returns a pointer to the note's statically allocated name array, or a lyric's text array, or NULL on error
void eof_set_note_name(EOF_SONG *sp, unsigned long track, unsigned long note, char *name);	//Copies the string into the note's statically allocated name array, or a lyric's text array
void *eof_track_add_create_note(EOF_SONG *sp, unsigned long track, unsigned char note, unsigned long pos, long length, char type, char *text);
	//Adds and initializes the appropriate note for the specified track, returning the newly created note structure, or NULL on error
	//Automatic flags will be applied appropriately (ie. crazy status for all notes in PART KEYS)
	//text is used to initialize the note name or lyric text, and may be NULL
void eof_track_sort_notes(EOF_SONG *sp, unsigned long track);
	//Calls the appropriate sort function for the specified track.  eof_selection.multi[] is preserved before the sort and recreated afterward, since sorting invalidates
	// the selection array due to note numbering being changed
	//Functions that depend on notes being sorted should be able to expect that notes are sorted primarily by timestamp and secondarily by difficulty number
int eof_song_qsort_phrase_sections(const void * e1, const void * e2);	//A generic qsort comparitor that will sort phrase sections into chronological order
long eof_track_fixup_previous_note(EOF_SONG *sp, unsigned long track, unsigned long note);	//Returns the note/lyric one before the specified note/lyric number that is in the same difficulty, or -1 if there is none
long eof_track_fixup_next_note(EOF_SONG *sp, unsigned long track, unsigned long note);	//Returns the note/lyric one after the specified note/lyric number that is in the same difficulty, or -1 if there is none
void eof_track_fixup_notes(EOF_SONG *sp, unsigned long track, int sel);
	//Calls the appropriate fixup function for the specified track.  If sel is zero, the currently selected note is deselected automatically
	//Dynamic highlighting for the track's active note set is also updated
void eof_track_find_crazy_notes(EOF_SONG *sp, unsigned long track, int option);
	//Used during MIDI and GP imports to mark a note as "crazy" if it overlaps with the next note in the same difficulty
	//If option is nonzero, two notes that begin at the same timestamp are not given crazy status (ie. to improve GP import of multi-voice files)
int eof_track_add_star_power_path(EOF_SONG *sp, unsigned long track, unsigned long start_pos, unsigned long end_pos);	//Adds a star power phrase at the specified start and stop timestamp for the specified track.  Returns nonzero on success
void eof_track_delete_star_power_path(EOF_SONG *sp, unsigned long track, unsigned long pathnum);	//Deletes the specified star power phrase and moves all phrases that follow back in the array one position
int eof_track_add_solo(EOF_SONG *sp, unsigned long track, unsigned long start_pos, unsigned long end_pos);	//Adds a solo phrase at the specified start and stop timestamp for the specified track.  Returns nonzero on success
void eof_track_delete_solo(EOF_SONG *sp, unsigned long track, unsigned long pathnum);	//Deletes the specified solo phrase and moves all phrases that follow back in the array one position
void eof_note_set_tail_pos(EOF_SONG *sp, unsigned long track, unsigned long note, unsigned long pos);	//Sets the note's length value to (pos - [note]->pos)
EOF_PHRASE_SECTION *eof_lookup_track_section_type(EOF_SONG *sp, unsigned long track, unsigned long sectiontype, unsigned long *count, EOF_PHRASE_SECTION **ptr);
	//Stores the count and address of the specified section type of the specified chart into *count and **ptr
	//Returns the address of the section array (which is also stored into ptr) upon success, or NULL upon error
	//If the section type is not applicable for the track in question, *count is set to 0 and *ptr is set to NULL
int eof_track_add_section(EOF_SONG * sp, unsigned long track, unsigned long sectiontype, unsigned char difficulty, unsigned long start, unsigned long end, unsigned long flags, char *name);
	//Adds the specified section to the specified track if it's valid for the track, track 0 is used to reference certain items that are project-wide in scope
	//For bookmark sections, the end variable represents which bookmark number is being set
	//For fret catalog sections, the flags variable represents which track the catalog entry belongs to, otherwise it's only used for lyric sections
	//For lyric phrases, the difficulty field indicates which lyric set number (ie. PART VOCALS) the phrase applies to
	//For applicable section types, name may point to a section name string in which case it will be copied to the section's name array, or NULL in which case it will be ignored
	//The difficulty field is used for catalog, arpeggio and tremolo (if Rocksmith numbered difficulties are in effect) sections and fret hand positions
	//Returns zero on error
unsigned long eof_count_track_lanes(EOF_SONG *sp, unsigned long track);		//Returns the number of lanes in the specified track, or the default of 5.  The value returned is expected to be less than EOF_MAX_FRETS
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
unsigned long eof_get_num_tone_changes(EOF_SONG *sp, unsigned long track);				//Returns the number of tone changes in the specified track, or 0 on error
void eof_track_delete_trill(EOF_SONG *sp, unsigned long track, unsigned long index);	//Deletes the specified trill phrase and moves all phrases that follow back in the array one position
void eof_track_delete_tremolo(EOF_SONG *sp, unsigned long track, unsigned long index);	//Deletes the specified tremolo phrase and moves all phrases that follow back in the array one position
void eof_track_delete_arpeggio(EOF_SONG *sp, unsigned long track, unsigned long index);	//Deletes the specified arpeggio phrase and moves all phrases that follow back in the array one position
void eof_track_delete_slider(EOF_SONG *sp, unsigned long track, unsigned long index);	//Deletes the specified slider phrase and moves all phrases that follow back in the array one position
unsigned long eof_get_num_lyric_sections(EOF_SONG *sp, unsigned long track);	//Returns the number of lyric sections in the specified track, or 0 on error
EOF_PHRASE_SECTION *eof_get_lyric_section(EOF_SONG *sp, unsigned long track, unsigned long sectionnum);	//Returns a pointer to the specified lyric section, or NULL on error
void *eof_copy_note(EOF_SONG *ssp, unsigned long sourcetrack, unsigned long sourcenote, EOF_SONG *dsp, unsigned long desttrack, unsigned long pos, long length, char type);
	//Copies the specified note from the specified source project track as a new note in the specified project track, returning a pointer to the newly created note structure, or NULL on error
	//Temporary flags are not copied
	//The specified position, length and type are applied to the new note.  Other note variables such as the bitmask/pitch and name/lyric text are copied as-is
	//If the source is a pro guitar track and the destination is not, the source note's legacy bitmask is used if defined
	//If the source and destination are both pro guitar tracks, the source note's fret array, finger array, ghost bitmask, legacy bitmask, bend strength, slide end position and extended flags are copied
void *eof_copy_note_simple(EOF_SONG *sp, unsigned long sourcetrack, unsigned long sourcenote, unsigned long desttrack, unsigned long pos, long length, char type);
	//Uses eof_copy_note() using the specified song structure as both the source and destination
long eof_get_prev_note_type_num(EOF_SONG *sp, unsigned long track, unsigned long note);
	//Returns the note immediately before the specified one that is in the same difficulty, provided that the notes are sorted chronologically, or -1 if no such note exists
void eof_adjust_note_length(EOF_SONG * sp, unsigned long track, unsigned long amount, int dir);
	//If dir is >= 0, increases all selected notes in the active instrument difficulty by the specified amount
	//If dir is < 0, decreases all selected notes in the active instrument difficulty by the specified amount
	//If amount is 0, then the notes are adjusted by the current grid snap value
	//An undo state is only created if at least one note's length is altered
long eof_get_note_max_length(EOF_SONG *sp, unsigned long track, unsigned long note, char enforcegap);
	//Determines the maximum valid length for the specified note by comparing its position to the next note that marks the threshold,
	//minus the configured minimum distance between notes (if enforcegap is nonzero).  If the specified note is a crazy note, the threshold
	//is based on the position of the next note in the same track difficulty that uses any gems on the same lane, otherwise it's based on
	//that of the next note in the track difficulty.
	//enforcegap should be given as nonzero unless the note has linknext status, which forces a mandatory distance between notes to be ignored
	//0 is returned on error, LONG_MAX is returned if there is no note that follows (indicating the note's length is only limited by its variable capacity)
unsigned eof_get_effective_minimum_note_distance(EOF_SONG *sp, unsigned long track, unsigned long notenum);
	//Examines the specified note and determines the effective minimum note distance between this note and the next,
	// based on the current settings of eof_min_note_distance and eof_min_note_distance_intervals
	//Returns 0 if there is no applicable limit to the specified note's length
	//Returns UINT_MAX on error

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
int eof_song_qsort_legacy_notes(const void * e1, const void * e2);	//The comparitor function used to quicksort the legacy notes array, first by timestamp, second by difficulty, third by note mask
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
void eof_vocal_track_sort_lyrics(EOF_VOCAL_TRACK * tp);		//Performs a quicksort of the lyrics array by timestamp
int eof_song_qsort_lyrics(const void * e1, const void * e2);	//The comparitor function used to quicksort the lyrics array
long eof_fixup_previous_lyric(EOF_VOCAL_TRACK * tp, unsigned long lyric);	//Returns the previous lyric, or -1 if there is none
long eof_fixup_next_lyric(EOF_VOCAL_TRACK * tp, unsigned long lyric);	//Returns the next lyric, or -1 if there is none
void eof_vocal_track_fixup_lyrics(EOF_SONG *sp, unsigned long track, int sel);	//Performs cleanup of the specified lyric track (based on the currently loaded audio and chart).  If sel is zero, the currently selected note is deselected automatically.
int eof_vocal_track_add_star_power(EOF_VOCAL_TRACK * tp, unsigned long start_pos, unsigned long end_pos);	//Marks all lyric phrases within the specified time span for overdrive.  Returns nonzero on success
int eof_vocal_track_add_line(EOF_VOCAL_TRACK * tp, unsigned long start_pos, unsigned long end_pos, unsigned char difficulty);
	//Adds a lyric phrase at the specified start and stop timestamp for the specified track.  Returns nonzero on success
	//The difficulty level at this time is typically going to just be 0xFF to indicate it applies to the entire track
void eof_vocal_track_delete_line(EOF_VOCAL_TRACK * tp, unsigned long index);	//Deletes the specified lyric phrase and moves all phrases that follow back in the array one position

EOF_PRO_GUITAR_NOTE *eof_pro_guitar_track_add_note(EOF_PRO_GUITAR_TRACK *tp);
	//Allocates, initializes and stores a new EOF_PRO_GUITAR_NOTE structure into the note array of the active note set.  Returns the newly allocated structure or NULL upon error
EOF_PRO_GUITAR_NOTE *eof_pro_guitar_track_add_tech_note(EOF_PRO_GUITAR_TRACK *tp);	//Allocates, initializes and stores a new EOF_PRO_GUITAR_NOTE structure into the technote array.  Returns the newly allocated structure or NULL upon error
void eof_pro_guitar_track_sort_notes(EOF_PRO_GUITAR_TRACK * tp);	//Performs a quicksort of the active note set, first by timestamp, second by difficulty, third by note mask
void eof_pro_guitar_track_sort_tech_notes(EOF_PRO_GUITAR_TRACK * tp);	//Performs a quicksort of the specified track's tech notes, first by timestamp, second by difficulty, third by note mask
int eof_song_qsort_pro_guitar_notes(const void * e1, const void * e2);	//The comparitor function used to quicksort the pro guitar notes array
void eof_pro_guitar_track_delete_note(EOF_PRO_GUITAR_TRACK * tp, unsigned long note);	//Removes and frees the specified note from the notes array.  All notes after the deleted note are moved back in the array one position
long eof_fixup_previous_pro_guitar_note(EOF_PRO_GUITAR_TRACK * tp, unsigned long note);	//Returns the note one before the specified note number that is in the same difficulty, or -1 if there is none
long eof_fixup_next_pro_guitar_note(EOF_PRO_GUITAR_TRACK * tp, unsigned long note);	//Returns the note one after the specified note number that is in the same difficulty, or -1 if there is none
long eof_fixup_next_pro_guitar_note_ptr(EOF_PRO_GUITAR_TRACK * tp, EOF_PRO_GUITAR_NOTE * np);	//Similar to eof_fixup_next_pro_guitar_note, but returns the index of the first note in tp that is AFTER the specified note's timestamp and is in the same difficulty
long eof_fixup_next_pro_guitar_technote(EOF_PRO_GUITAR_TRACK * tp, unsigned long tnote);	//Returns the tech note one after the specified tech note number that is in the same difficulty, or -1 if there is none
long eof_track_fixup_first_pro_guitar_note(EOF_PRO_GUITAR_TRACK * tp, unsigned char diff);	//Returns the first note/lyric in the specified track difficulty, or -1 if there is none
void eof_pro_guitar_track_fixup_notes(EOF_SONG *sp, unsigned long track, int sel);
	//Performs cleanup of the specified instrument track.  If sel is zero, the currently selected note is deselected automatically.
	//This function will abort if any notes with temp or ignore flags are encountered, as that signifies Rocksmith export is in progress and this function would otherwise merge/delete notes inappropriately
void eof_pro_guitar_track_fixup_hand_positions(EOF_SONG *sp, unsigned long track);	//Performs cleanup of the specified pro guitar track's fret hand positions
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
void eof_pro_guitar_track_sort_fret_hand_positions(EOF_PRO_GUITAR_TRACK* tp);	//Sorts the specified track's fret hand positions by difficulty and then by timestamp
void eof_pro_guitar_track_delete_hand_position(EOF_PRO_GUITAR_TRACK *tp, unsigned long index);	//Deletes the specified fret hand position
void eof_pro_guitar_track_sort_arpeggios(EOF_PRO_GUITAR_TRACK* tp);	//Sorts the specified track's arpeggios by difficulty and then by timestamp

void eof_sort_notes(EOF_SONG *sp);	//Sorts the notes in all tracks
void eof_fixup_notes(EOF_SONG *sp);	//Performs cleanup of the notes in all tracks
unsigned char eof_detect_difficulties(EOF_SONG * sp, unsigned long track);
	//Sets the populated status indicator for the specified track's difficulty names by prefixing each populated difficulty name in the current track (stored in eof_note_type_name[], eof_vocal_tab_name[] and eof_dance_tab_name[]) with an asterisk
	//eof_track_diff_populated_status[] is updated so that each populated difficulty results in the corresponding element number being nonzero
	//eof_track_diff_populated_tech_note_status[] is also updated if the specified track is a pro guitar track, so that each difficulty with at least one tech note results in the corresponding element number being nonzero
	//eof_track_diff_highlighted_status[] and eof_track_diff_highlighted_tech_note_status[] are also updated
	//Returns the number of difficulties present in the specified track (ie. if the highest used difficulty is 9, 10 is returned because the numbering begins with 0), or 0 is returned upon error or empty track
	//If the specified track is also the active track, the program window title is redrawn to reflect the current populated status of the active track difficulty
	//This function updates the technotes and pgnotes counters to ensure that if used after a deletion operation, those values are correct in the event that subsequent operations refer to them

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

double eof_calc_beat_length(EOF_SONG *sp, unsigned long beat);
	//Returns the length of the specified beat based on the tempo and time signature in effect at its position or 0.0 on error

char eof_check_flags_at_legacy_note_pos(EOF_LEGACY_TRACK *tp, unsigned notenum, unsigned long flag);
	//Checks all notes in the track, from #notenum to the last, that are at the specified note's timestamp
	//for the specified flag.  If any of the notes have the flag set, nonzero is returned
	//This is used for writing RB3 style pro drum phrases during MIDI export
	//The track's notes array is expected to be sorted
void eof_set_flags_at_legacy_note_pos(EOF_LEGACY_TRACK *tp, unsigned notenum, unsigned long flag, char operation, char startpoint);
	//Sets or clears the specified flag on all notes at the specified note's timestamp
	//If startpoint is 0, the process will start from the first note in the track with note #notenum's position
	//	otherwise the process will start from note #notenum
	//If the calling function is processing sorted notes in ascending order without filtering by difficulty or selected status, startpoint can be set to nonzero to save needless processing
	//If operation is 0, the specified flag is cleared on applicable notes
	//If operation is 1, the specified flag is set on applicable notes
	//If operation is 2, the specified flag is toggled on applicable notes
	//The track's notes array is expected to be sorted

void eof_set_accent_at_legacy_note_pos(EOF_LEGACY_TRACK *tp, unsigned long pos, unsigned long mask, char operation);
	//Alters the specified accent bits on all notes at the specified note's timestamp
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
	//Allocates, initializes and returns an EOF_SONG structure pre-populated with the default legacy, vocal and pro guitar tracks

int eof_open_strum_enabled(unsigned long track);
	//Returns nonzero if the specified track is a legacy guitar/bass track and has the open strum flag set
int eof_lane_six_enabled(unsigned long track);
	//Returns nonzero if the specified track is a non drum legacy track with the open strum flag set, or if it's a pro guitar track with 6 strings
int eof_create_image_sequence(char benchmark_only);
	//Creates a PCX format image sequence in a subfolder of the chart's folder called "\sequence"
	//If benchmark_only is nonzero, image files are not written and the rendering performance (in frames per second) is displayed in the title bar
int eof_write_image_sequence(void);
	//Calls eof_create_image_sequence() with the option to save the images to disk
int eof_benchmark_image_sequence(void);
	//Calls eof_create_image_sequence() with the option to benchmark only

int eof_get_pro_guitar_note_fret_string(EOF_PRO_GUITAR_TRACK *tp, unsigned long note, char *fret_string);
	//Writes a string representation of the specified pro guitar/bass note from lowest to highest gauge string into fret_string[], which must be at least
	//3 * # of strings number of bytes long in order to store the maximum length string
	//Returns 0 on error or 1 on success
int eof_get_pro_guitar_note_finger_string(EOF_PRO_GUITAR_TRACK *tp, unsigned long note, char *finger_string);
	//Writes a string representation of the specified pro guitar/bass note's fingering into finger_string[], which must be at least
	//3 * # of strings number of bytes long in order to store the maximum length string
	//Padding is added as necessary so that the fingering aligns right with the fret string's contents, taking double digit fret numbers into account
	//Returns 0 on error or 1 on success
int eof_get_pro_guitar_note_tone_string(EOF_PRO_GUITAR_TRACK *tp, unsigned long note, char *tone_string);
	//Writes a string representation of the specified pro guitar/bass note's played tones into finger_string[], which must be at least
	//3 * # of strings number of bytes long in order to store the maximum length string
	//Returns 0 on error or 1 on success
int eof_get_pro_guitar_fret_shortcuts_string(char *shortcut_string);
	//Writes a string representation of the strings that are currently affected by the fret value shortcuts (ie. CTRL+#)
	// into shortcut_string[], which must be at least 55 characters long to store the maximum length string
	//Returns 0 on error or 1 on success

int eof_five_lane_drums_enabled(void);
	//A simple function returning nonzero if PART DRUM has the fifth lane enabled

char eof_track_has_cymbals(EOF_SONG *sp, unsigned long track);
	//Returns nonzero if the specified track is a drum track that contains notes marked as cymbals
int eof_track_is_legacy_guitar(EOF_SONG *sp, unsigned long track);
	//Returns nonzero if the specified track is a legacy guitar track
int eof_track_is_ghl_mode(EOF_SONG *sp, unsigned long track);
	//Returns nonzero if the specified track has GHL mode enabled

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
unsigned long eof_get_lowest_fret_value(EOF_SONG *sp, unsigned long track, unsigned long note);
	//Returns the lowest used fret in the specified pro guitar note
	//If the parameters are invalid or the specific pro guitar note's gems are all muted with no fret specified, 0 is returned
unsigned long eof_get_pro_guitar_note_highest_fret_value(EOF_PRO_GUITAR_NOTE *np);
	//Returns the highest used fret in the specified pro guitar note
	//If the parameters are invalid or the specific pro guitar note's gems are all muted with no fret specified, 0 is returned
unsigned long eof_get_highest_fret_value(EOF_SONG *sp, unsigned long track, unsigned long note);
	//Calls eof_get_pro_guitar_note_highest_fret_value() on the specified note

unsigned long eof_determine_chart_length(EOF_SONG *sp);
	//Parses the project and returns the ending position of the last note/tech note/lyric/text event/bookmark
void eof_truncate_chart(EOF_SONG *sp);
	//Uses eof_determine_chart_length() to set eof_chart_length to the larger of the last chart content and the loaded chart audio
	//Any beats that are more than two beats after this position are deleted from the project
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
void eof_track_remove_highlighting(EOF_SONG *sp, unsigned long track, char function);
	//Removes the highlight status on all notes (and tech notes if applicable) in the specified track
	//If function is 0, the permanent highlight flag (EOF_NOTE_FLAG_HIGHLIGHT) is removed from the notes
	//If function is 1, the temporary highlight flag (EOF_NOTE_TFLAG_HIGHLIGHT) is removed from the notes
	//If function is any other value, both highlight flags are removed from the notes

extern SAMPLE *eof_export_audio_time_range_sample;
void eof_export_audio_time_range(ALOGG_OGG * ogg, double start_time, double end_time, const char * fn);
	//Exports a time range of the specified OGG file to a file of the specified name
	//start_time and end_time are both in seconds and not milliseconds

char eof_pro_guitar_note_bitmask_has_tech_note(EOF_PRO_GUITAR_TRACK *tp, unsigned long note, unsigned long mask, unsigned long *technote_num);
	//Returns nonzero if the specified pro guitar note has at least one tech note that overlaps it on any of the specified strings in the specified bitmask
	//If technote_num is not NULL, the index number of the first relevant tech note is returned through it
	//If the specified note extends all the way to the next note (has linkNext status), only the tech notes before the next note's position are checked
char eof_pro_guitar_note_has_tech_note(EOF_PRO_GUITAR_TRACK *tp, unsigned long note, unsigned long *technote_num);
	//Uses eof_pro_guitar_note_bitmask_has_tech_note() to search for a technote overlapping any of the specified note's used strings
	//If technote_num is not NULL, the index number of the first relevant tech note is returned through it
char eof_pro_guitar_note_has_open_note(EOF_PRO_GUITAR_TRACK *tp, unsigned long note);
	//Returns nonzero if the specified note has any open notes
	//Returns zero on error or if the note does not have any open notes
unsigned long eof_pro_guitar_note_bitmask_has_bend_tech_note(EOF_PRO_GUITAR_TRACK *tp, unsigned long note, unsigned long mask, unsigned long *technote_num);
	//Similar to eof_pro_guitar_note_has_tech_note(), but returns the number of overlapping tech notes that have bend technique
	//Only the first applicable pre-bend tech note is included in the count
	//If there is a bend tech note at the note's start position AND a pre-bend tech note, only the pre-bend note is included in the count
	//If technote_num is not NULL, the index number of the first relevant bend tech note is returned through it
long eof_pro_guitar_note_bitmask_has_pre_bend_tech_note(EOF_PRO_GUITAR_TRACK *tp, unsigned long pgnote, unsigned long mask);
	//Similar to eof_pro_guitar_note_bitmask_has_bend_tech_note(), but returns the index of the first pre-bend tech note applicable for the note and bitmask
	//Returns -1 if no applicable pre-bend tech note exists
long eof_pro_guitar_note_bitmask_has_pre_bend_tech_note_ptr(EOF_PRO_GUITAR_TRACK *tp, EOF_PRO_GUITAR_NOTE *np, unsigned long mask);
	//Similar to eof_pro_guitar_note_bitmask_has_pre_bend_tech_note(), but accepts a note pointer instead of an index
	//and only checks for tech notes with normal bend status (instead of pre-bend) at the specified note's position, since this is used in RS import where pre-bend status hasn't been determined yet
char eof_pro_guitar_tech_note_overlaps_a_note(EOF_PRO_GUITAR_TRACK *tp, unsigned long technote, unsigned long mask, unsigned long *note_num);
	//Looks for the last regular pro guitar note that is overlapped by the specified tech note and mask
	//If the tech note is found to be at the start position of any overlapping notes, 1 is returned
	//If the tech note is found to overlap at least one note, 2 is returned
	// If note_num is not NULL, the matching regular note number is returned through it
unsigned long eof_pro_guitar_lookup_combined_tech_flags(EOF_PRO_GUITAR_TRACK *tp, unsigned long pgnote, unsigned long mask, unsigned long *flags, unsigned long *eflags);
	//Parses the specified track's tech notes, returning the number of tech notes that were found to overlap the specified note and use the specified note mask
	//The combination of all techniques applied to the specified gems of the specified normal note are returned through *flags and *eflags
	// *flags and *eflags are reset to 0 in the event no applicable tech notes are found
	//Returns 0 on error

void eof_pro_guitar_track_enforce_chord_density(EOF_SONG *sp, unsigned long track);
	//If the "Apply crazy to repeated chords separated by a rest" preference is enabled,
	//and if any chords are repeats of non-selected notes, and those chords are more than the
	//configured minimum note distance apart from the preceding notes (or 2ms, whichever is
	//larger) crazy status is applied so that the chords in question export as high density

void eof_song_enforce_mid_beat_tempo_change_removal(void);
	//If the "Imports drop mid beat tempos" preference is enabled, this function deletes beats that have the EOF_BEAT_FLAG_MIDBEAT flag
	//Regardless of that preference's setting, the flag is removed from all beats in the project

//Criterion checks for selection functions
int eof_note_is_chord(EOF_SONG *sp, unsigned long track, unsigned long notenum);	//Returns nonzero if the specified note contains more than one gem
int eof_note_is_single_note(EOF_SONG *sp, unsigned long track, unsigned long notenum);	//Returns nonzero if the specified note contains exactly one gem
int eof_note_is_tom(EOF_SONG *sp, unsigned long track, unsigned long notenum);		//Returns nonzero if the specified note contains any gems on lane 3, 4 or 5 that are not marked as cymbals
int eof_note_is_cymbal(EOF_SONG *sp, unsigned long track, unsigned long notenum);	//Returns nonzero if the specified note contains any gems on lane 3, 4 o4 5 that are cymbals
int eof_note_is_grid_snapped(EOF_SONG *sp, unsigned long track, unsigned long notenum);	//Returns nonzero if the specified note is on a grid snap position
int eof_note_is_not_grid_snapped(EOF_SONG *sp, unsigned long track, unsigned long notenum);	//Returns nonzero if the specified note exists and is not on a grid snap position
int eof_note_is_highlighted(EOF_SONG *sp, unsigned long track, unsigned long notenum);	//Returns nonzero if the specified note has either the static or dynamic highlight flags set
int eof_note_is_not_highlighted(EOF_SONG *sp, unsigned long track, unsigned long notenum);	//Returns nonzero if the specified note exists and is not highlighted
int eof_length_is_shorter_than(long length, long threshold);	//Returns nonzero if the length parameter is shorter than the threshold parameter
int eof_length_is_longer_than(long length, long threshold);		//Returns nonzero if the length parameter is longer than the threshold parameter
int eof_length_is_equal_to(long length, long threshold);		//Returns nonzero if both parameters are equal

void eof_auto_adjust_sections(EOF_SONG *sp, unsigned long track, unsigned long offset, char dir, char any, char *undo_made);
	//Examines all sections in the specified track, and for those which have notes that are all selected, their positions are moved
	//Returns with no changes made if the "Auto-Adjust sections/FHPs" preference is not enabled
	//If dir is negative, applicable sections are moved the specified offset number of ms earlier
	// otherwise applicable sections are moved the specified offset number of ms later
	//If offset is zero, applicable sections are moved in one grid snap in the specified direction instead of by a specific number of ms
	//If both offset AND dir are zero, applicable sections are re-snapped to the nearest grid snap positions
	//If any if zero, selection of a grid snap position is only based on the current grid snap setting
	//If any is nonzero, selection of a grid snap position is based on the nearest grid snap position of any grid size and ALL notes in the track are affected instead of just the selected ones
	//If undo_made is not NULL and references a value of 0, an undo state is made prior to the first section being moved, and *undo_made is set to nonzero

unsigned long eof_auto_adjust_tech_notes(EOF_SONG *sp, unsigned long track, unsigned long offset, char dir, char any, char *undo_made);
	//Examines all tech notes in the specified pro guitar track, and for those that are fully applied to notes that are selected, their positions are moved
	//All gems in a tech note must be applied to selected note(s) in order to be moved this way
	//Returns with no changes if the "Auto-Adjust tech notes" preference is not enabled
	//If dir is negative, applicable sections are moved the specified offset number of ms earlier
	// otherwise applicable sections are moved the specified offset number of ms later
	//If offset is zero, applicable sections are moved in one grid snap in the specified direction instead of by a specific number of ms
	//If both offset AND dir are zero, applicable sections are re-snapped to the nearest grid snap positions
	//If any if zero, selection of a grid snap position is only based on the current grid snap setting
	//If any is nonzero, selection of a grid snap position is based on the nearest grid snap position of any grid size and ALL notes in the track are affected instead of just the selected ones
	//If undo_made is not NULL and references a value of 0, an undo state is made prior to the first section being moved, and *undo_made is set to nonzero
	//Returns the number of tech notes that were moved, or 0 if none were or upon error

/*@unused@ Avoid complaints from Splint*/
static inline int eof_beat_num_valid(EOF_SONG *sp, unsigned long beatnum)
{
	return (sp && (beatnum < sp->beats));
}

void eof_convert_all_lyrics_from_extended_ascii(EOF_VOCAL_TRACK *tp);	//Uses eof_convert_from_extended_ascii() to convert all lyrics in the specified chart

struct eof_MIDI_data_track *eof_song_has_stored_tempo_track(EOF_SONG * sp);
	//Returns a pointer to the specified chart's stored tempo track
	//Returns NULL if no such track is stored or upon error

#endif
