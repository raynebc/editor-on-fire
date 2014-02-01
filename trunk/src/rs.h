#ifndef EOF_RS_H
#define EOF_RS_H

typedef struct
{
	char *string;
	char *displayname;
} EOF_RS_PREDEFINED_SECTION;

typedef struct
{
	unsigned long pos;
	char *str;
} EOF_RS_CONTROL;	//A structure that will be used to build an array of popup message controls, clear message controls and tone change controls, so they can be chronologically sorted

typedef struct
{
	unsigned long length;
	char bend;						//Nonzero if this note is a bend, in which case it is the bend strength rounded up to the nearest number of half steps (used in RS2 notation)
	unsigned long bendstrength_q;	//The number of quarter steps this note bends (used in RS2 notation)
	unsigned long bendstrength_h;	//The number of half steps this note bends (used in RS1 notation)
	long slideto;					//If not negative, is the fret position the slide ends at
	char hammeron;					//Nonzero if this note is a hammer on
	char pulloff;					//Nonzero if this note is a pull off
	char harmonic;					//Nonzero if this note is a harmonic
	char hopo;						//Nonzero if this note is either a hammer on or a pull off
	char palmmute;					//Nonzero if this note is a palm mute
	char tremolo;					//Nonzero if this note is a tremolo
	char pop;						//1 if this note is played with pop technique, else -1
	char slap;						//1 if this note is played with slap technique, else -1
	char accent;					//Nonzero if this note is played as an accent
	char pinchharmonic;				//Nonzero if this note is a pinch harmonic
	char stringmute;				//Nonzero if this note is a string mute
	long unpitchedslideto;			//If not negative, is the fret position to which the note performs an unpitched slide
	char tap;						//Nonzero if this note is tapped
	char vibrato;					//Is set to 80 if the note is played with vibrato, otherwise zero
	char linknext;					//Nonzero if this note has the linknext option enabled
	char ignore;					//Nonzero if this note has the ignore status applied and RS2 export is being performed
} EOF_RS_TECHNIQUES;

#define EOF_NUM_RS_PREDEFINED_SECTIONS 30
extern EOF_RS_PREDEFINED_SECTION eof_rs_predefined_sections[EOF_NUM_RS_PREDEFINED_SECTIONS];

#define EOF_NUM_RS_PREDEFINED_EVENTS 4
extern EOF_RS_PREDEFINED_SECTION eof_rs_predefined_events[EOF_NUM_RS_PREDEFINED_EVENTS];

extern char *eof_rs_arrangement_names[5];

typedef struct
{
	unsigned char note;			//Stores the note's string statuses
	unsigned char frets[8];		//Stores the fret number for each string
	unsigned char finger[8];	//Stores the finger number used to fret each string
	char *name;					//Stores the name of the shape (ie. "D")
} EOF_CHORD_SHAPE;

#define EOF_MAX_CHORD_SHAPES 300
extern EOF_CHORD_SHAPE eof_chord_shape[EOF_MAX_CHORD_SHAPES];
extern unsigned long num_eof_chord_shapes;

int eof_is_string_muted(EOF_SONG *sp, unsigned long track, unsigned long note);
	//Returns nonzero if all used strings in the note are fret hand muted
int eof_is_partially_ghosted(EOF_SONG *sp, unsigned long track, unsigned long note);
	//Returns nonzero if the specified note contains at least one ghosted gem and one non ghosted gem

unsigned long eof_build_chord_list(EOF_SONG *sp, unsigned long track, unsigned long **results, char target);
	//Parses the given pro guitar track, building a list of all unique chords
	//Memory is allocated to *results to return the list, referring to each note by number
	//The function returns the number of unique chords contained in the list
	//If there are no chords in the pro guitar track, or upon error, *results is set to NULL and 0 is returned
	//If target is 1, then Rocksmith 1 authoring rules are also followed and chords that don't have at least two non string muted gems are not included

unsigned long eof_build_section_list(EOF_SONG *sp, unsigned long **results, unsigned long track);
	//Parses the given chart, building a list of all unique section markers (case insensitive),
	//from the perspective of the specified track (ie. a bass-specific section is not found when a guitar track is specified)
	//Memory is allocated to *results to return the list, referring to each text event by number
	//The function returns the number of unique sections contained in the list
	//If there are no sections in the pro guitar track, or upon error, *results is set to NULL and 0 is returned

int eof_export_rocksmith_1_track(EOF_SONG * sp, char * fn, unsigned long track, char *user_warned);
	//Writes the specified pro guitar track in Rocksmith 1's XML format, if the track is populated
	//fn is expected to point to an array at least 1024 bytes in size, and is the target path for the exported XML file.
	//	It is used to build an appropriate name for the XML file, based on the track's defined arrangement type or the presence of an alternate track name
	// *user_warned maintains a set of flags about whether various problems were found and warned about to the user:
	//	1:  At least one track difficulty has no fret hand positions, they will be automatically generated
	//	2:  At least one track uses a fret value higher than 22
	//	4:  At least one open note is marked with bend or slide status
	//	8:  At least one note slides to or above fret 22
	//  16:  There is no COUNT phrase defined and the first beat already contains a phrase
	//  32:  There is at least one phrase or section defined after the END phrase
	//Rocksmith 1 doesn't support arrangements using a capo, so the capo position is added to the frets values of all fretted notes

int eof_export_rocksmith_2_track(EOF_SONG * sp, char * fn, unsigned long track, char *user_warned);
	//Writes the specified pro guitar track in Rocksmith 2's XML format, if the track is populated
	//fn is expected to point to an array at least 1024 bytes in size, and is the target path for the exported XML file.
	//	It is used to build an appropriate name for the XML file, based on the track's defined arrangement type or the presence of an alternate track name
	// *user_warned maintains a set of flags about whether various problems were found and warned about to the user:
	//	1:  At least one track difficulty has no fret hand positions, they will be automatically generated
	//	4:  At least one open note is marked with bend or slide status
	//	8:  At least one note slides to or above fret 22
	//  16:  There is no COUNT phrase defined and the first beat already contains a phrase
	//  32:  There is at least one phrase or section defined after the END phrase
	//Rocksmith 2 supports arrangements using a capo, but it does not consider the capo to be the nut position, so the capo position still needs to be added
	// to the fret values of all fretted notes

void eof_rs_export_cleanup(EOF_SONG * sp, unsigned long track);
	//Cleanup that should be performed before either RS export function returns
	//This function removes the ignore status from the specified track's notes and deletes temporary notes that had been added

void eof_pro_guitar_track_fix_fingerings(EOF_PRO_GUITAR_TRACK *tp, char *undo_made);
	//Checks all notes in the track and duplicates finger arrays of chords with complete finger definitions to matching chords without complete finger definitions
	//If any note has invalid fingering, it is cleared and will be allowed to be set by a valid fingering from a matching note
	//Single notes do not have their fingerings populated by this function, but if the fingering is invalid it is still cleared
	//If *undo_made is zero, this function will create an undo state before modifying the chart and will set the referenced variable to nonzero
int eof_pro_guitar_note_fingering_valid(EOF_PRO_GUITAR_TRACK *tp, unsigned long note);
	//Returns 0 if the fingering is invalid for the specified note (partially defined, or a finger specified for a string that is played open)
	//Returns 1 if the fingering is fully defined for the specified note
	//Returns 2 if the fingering is undefined for the specified note
void eof_song_fix_fingerings(EOF_SONG *sp, char *undo_made);
	//Runs eof_pro_guitar_track_fix_fingerings() on all pro guitar tracks in the specified chart
	//If *undo_made is zero, this function will create an undo state before modifying the chart and will set the referenced variable to nonzero
int eof_lookup_chord_shape(EOF_PRO_GUITAR_NOTE *np, unsigned long *shapenum, unsigned long skipctr);
	//Examines the specified note and returns nonzero if a matching chord shape definition is found, in which case the shape number is returned through shapenum if it is not NULL
	//If skipctr is nonzero, then the first [skipctr] number of matches are ignored during the lookup process, allowing additional definition matches to be found
void eof_apply_chord_shape_definition(EOF_PRO_GUITAR_NOTE *np, unsigned long shapenum);
	//Applies the specified chord shape to the specified note, transposing the shape definition to suit the lowest fretted string of the specified note
unsigned long eof_count_chord_shape_matches(EOF_PRO_GUITAR_NOTE *np);
	//Returns the number of chord shape definitions that match the specified note
void eof_load_chord_shape_definitions(char *fn);
	//Reads chord templates from the specified file and stores them into the eof_chord_shape[] array
void eof_destroy_shape_definitions(void);
	//Empties the eof_chord_shape[] array, releasing the entries' allocated memory

int eof_generate_hand_positions_current_track_difficulty(void);
	//Calls eof_generate_efficient_hand_positions() specifying the current track difficulty
	//Returns 0 on error
int eof_note_can_be_played_within_fret_tolerance(EOF_PRO_GUITAR_TRACK *tp, unsigned long note, unsigned char *current_low, unsigned char *current_high);
	//Examines the highest and lowest used frets of the specified note, and whether the updated highest and lowest fret would be within the specified tolerance for fret difference (ie. can be played without moving the fret hand)
	//If the note is considered playable without requiring an addition fret hand position change, current_high and current_low are updated by reference and nonzero is returned
	//if the new difference between the high and low fret are out of the tolerance range, zero is returned and the calling function should place a fret hand position for the previously checked notes
	//The tolerance for each given fret hand position is stored in eof_fret_range_tolerances[], which must have enough definitions as there are frets for the specified pro guitar track, plus 1 to account that entry 0 is unused
	//If the specified note only contains open strings, the next note with fretted strings is checked instead.  This to allow the camera in Rocksmith time to position between anchors while open strings are played.
void eof_build_fret_range_tolerances(EOF_PRO_GUITAR_TRACK *tp, unsigned char difficulty, char dynamic);
	//Re-allocate and build the array referenced by eof_fret_range_tolerances, optionally examining the specified track difficulty's notes to determine range for each fret
	//If dynamic is zero (should be used when generating hand positions for Rocksmith), the range tolerance for all frets is set to 4, otherwise they are defined as follows:
	//For each note examined, the range of frets used is considered to be playable and the range for the note's lowest used fret position is updated accordingly
	//If any particular fret is not used as the lowest fret for any chords, the fret range is defaulted to 4
void eof_generate_efficient_hand_positions(EOF_SONG *sp, unsigned long track, char difficulty, char warnuser, char dynamic);
	//Uses eof_note_can_be_played_within_fret_tolerance() and eof_build_fret_range_tolerances() to build an efficient set of fret hand positions for the specified track
	//If eof_fret_hand_position_list_dialog_undo_made is nonzero, an undo state is made before changing the existing hand positions
	//If warnuser is nonzero, the user is prompted to confirm deletion of existing fret hand positions
	//The dynamic parameter is passed to eof_build_fret_range_tolerances(), indicating whether to base the fret range tolerances from the chart or leave them all at 4
	//Dynamic should be left at 0 for generating positions for Rocksmith (limitation with the game only allows for range of 4).  It will produce higher quality positions for Rock Band 3 when dynamic is nonzero.
unsigned char eof_pro_guitar_track_find_effective_fret_hand_position(EOF_PRO_GUITAR_TRACK *tp, unsigned char difficulty, unsigned long position);
	//Returns the fret hand position in effect (at or before) at the specified timestamp in the specified track difficulty
	//Returns nonzero if a fret hand position is in effect
EOF_PHRASE_SECTION *eof_pro_guitar_track_find_effective_fret_hand_position_definition(EOF_PRO_GUITAR_TRACK *tp, unsigned char difficulty, unsigned long position, unsigned long *index, unsigned long *diffindex, char function);
	//Similar to eof_pro_guitar_track_find_effective_fret_hand_position(), but returns a pointer to the hand position in effect, or NULL if none are in effect
	//If function is nonzero, then the hand position must be exactly at the specified time position to be considered "in effect"
	//If non NULL is returned, the index of the effective fret hand position is returned through *index if its pointer isn't NULL, and the position index within the specified difficulty is returned through *diffindex if its pointer isn't NULL
	//If NULL is returned, neither *index nor *diffindex are altered
unsigned long eof_find_effective_rs_phrase(unsigned long position);
	//Returns the phrase number in effect (at or before) at the specified timestamp, or 0 if no phrase was in effect at that position
	//This function assumes that the beat statistics are properly cached for the active track
int eof_time_range_is_populated(EOF_SONG *sp, unsigned long track, unsigned long start, unsigned long stop, unsigned char diff);
	//Returns nonzero if there are any notes in the specified track difficulty within the specified time range
	//Returns zero if that range contains no notes or upon error

char *eof_rs_section_text_valid(char *string);
	//Compares the given string against the string entries in eof_rs_predefined_sections[]
	//If the string matches the native or display name of an entry, a pointer to the native name is returned (to be stored as a text event), otherwise NULL is returned
int eof_rs_event_text_valid(char *string);
	//Compares the given string against the string entries in eof_rs_predefined_events[] and returns nonzero if it matches one of them
unsigned long eof_get_rs_section_instance_number(EOF_SONG *sp, unsigned long track, unsigned long event);
	//If the specified event's flags indicate a Rocksmith section, counts the number of matching (case sensitive) text events preceding it
	//The instance number of the specified Rocksmith section (numbered starting with 1) is returned
	//Rocksmith sections are allowed to be track specific, so the instance number takes this into account from the perspective of the specified track
	//If a Rocksmith section's associated track is 0, then it is not track specific
	//Upon error, or if the given event is not a Rocksmith section in the specified track, 0 is returned

void eof_get_rocksmith_wav_path(char *buffer, const char *parent_folder, size_t num);
	//Builds the path to the WAV file (used for Rocksmith) that is written to specified parent folder path during save
	//num defines the buffer's maximum size
	//This is (song name).wav if the song title song property is defined, otherwise guitar.wav

void eof_delete_rocksmith_wav(void);
	//Deletes the Rocksmith WAV file based on the path created by the chart's song title
	//If such a file does not exist (this includes if the song title has characters that are invalid for a file name), "guitar.wav" is deleted

char eof_compare_time_range_with_previous_or_next_difficulty(EOF_SONG *sp, unsigned long track, unsigned long start, unsigned long stop, unsigned char diff, char compareto);
	//Returns 1 if the notes in the specified track and time range don't match with those in the previous (if compareto is negative) or next difficulty (if compareto is >= 0)
	//Returns -1 if there are no notes in the active difficulty of the phrase
	//Returns 0 if they match, there is no previous/next difficulty, or upon error
	//Note lengths and flags are compared in this function.

unsigned char eof_find_fully_leveled_rs_difficulty_in_time_range(EOF_SONG *sp, unsigned long track, unsigned long start, unsigned long stop, unsigned char relative);
	//Examines the notes in all difficulties of the specified track and returns the lowest difficulty number that represents the fully leveled
	//version of the specified time range, such as for finding appropriate maxdifficulty values for each Rocksmith phrase
	//If relative is nonzero, the difficulty level returned is converted to the relative numbering system to define difficulty levels, based only on populated difficulties
	//If no notes are in the range, 0 (EOF's lowest difficulty number) is returned

int eof_check_rs_sections_have_phrases(EOF_SONG *sp, unsigned long track);
	//Checks the specified track to ensure that each beat that has a Rocksmith section also has a Rocksmith phrase (from the perspective of the specified track).
	//If not, the user is prompted whether to cancel save and seek to the offending beat.  If user accepts, the top of the piano roll is changed to display RS sections and the
	// chart seeks to the offending beat in the specified track, automatically launching "Place RS Phrase" and then checking the remaining beats.
	//If user refuses on any of the prompts or cancels any "Place RS Phrase" dialogs, or upon error, nonzero is returned.
	//If the track is not populated, the function returns zero without checking the RS sections.

int eof_note_has_high_chord_density(EOF_SONG *sp, unsigned long track, unsigned long note, char target);
	//Returns nonzero if the specified note will export to XML as a high density chord
	//This is based on whether the chord is close enough to a matching, previous chord
	// and whether the note's "crazy" flag is set (which overrides it to be low density)
	//If target is 1, then Rocksmith 1 authoring rules are followed and string muted chords are ignored
	//If target is 2, then Rocksmith 2 authoring rules are followed and a chord with techniques that
	// require chordNote tags to be written and chords that follow chord slides are reflected as low density,
	// and string muted chords that follow other chords are automatically counted as high density because RS2
	// uses a separate XML attribute to cause it to display as string muted

int eof_enforce_rs_phrase_begin_with_fret_hand_position(EOF_SONG *sp, unsigned long track, unsigned char diff, unsigned long startpos, unsigned long endpos, char *undo_made, char check_only);
	//Looks at the fret hand positions in the specified track difficulty within the specified time span, which should be the beginning and end of a RS phrase.
	//  If no such fret hand position exists, but a fret hand position is defined earlier in the difficulty, that hand position is defined at the first note within the phrase,
	//  ensuring that there is an explicit hand position defined within each difficulty of each phrase, which ensures that they work properly in Rocksmith,
	//  which can otherwise skip displaying position changes depending on the difficulties in effect going from one phrase to the next
	//If *undo_made is zero, this function will create an undo state before adding any fret hand position
	//Returns nonzero if it was found that a phrase was needed to be added

void eof_export_rocksmith_showlights(EOF_SONG * sp, char * fn, unsigned long track);
	//Exports a showlights file for the specified track to the specified file (to be used for Rocksmith 2014 customs)
	//This XML file defines the MIDI note played for each highest-level note (or bass notes in the case of chords)

unsigned long eof_get_highest_fret_in_time_range(EOF_SONG *sp, unsigned long track, unsigned char difficulty, unsigned long start, unsigned long stop);
	//Examines all notes within the specified time range in the specified track difficulty and returns the highest fret value encountered
	//Returns 0 if there are no fretted notes in the range, or upon error
	//Expects the notes to be sorted in order to maximize performance

unsigned long eof_get_rs_techniques(EOF_SONG *sp, unsigned long track, unsigned long notenum, unsigned long stringnum, EOF_RS_TECHNIQUES *ptr, char target);
	//Reads the flags of the specified note and sets variables in the specified techniques structure
	//stringnum is used to set the fret and pitched/unpitched slide end fret values (which take the track's capo into account) for a specified string,
	//  as well as determining which tech notes affect the note on the specified string, and must be a value from 0 to 5.
	//  The correct end position for each slide is tracked for the specified string by determining how many frets the slide is
	//If the note has pop or slap status, the length in the techniques structure is set to 0 to reflect
	//	that Rocksmith requires such techniques to be on non sustained notes
	//Unless the note has bend or slide status, the length in the techniques structure is set to 0 if the note has EOF's minimum length of 1ms
	//A flags bitmask is returned that is nonzero if the note contains any statuses that would necessitate chordNote subtag(s) if the examined note is a chord
	//If ptr is NULL, no logic is performed besides returning the flags that the note contains that would necessitate chordNote subtag(s) if the examined note is a chord
	//If target is 1, then Rocksmith 1 authoring rules are followed and a note cannot be both a slide/bend AND a pop/slap note, as they have conflicting sustain requirements
	//The capo position is added to the end position of pitched and unpitched slides, as required by Rocksmith in order for them to display correctly for capo'd arrangements

#endif
