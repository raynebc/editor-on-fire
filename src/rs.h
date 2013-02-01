#ifndef EOF_RS_H
#define EOF_RS_H

typedef struct
{
	char *string;
	char *displayname;
} EOF_RS_PREDEFINED_SECTION;

#define EOF_NUM_RS_PREDEFINED_SECTIONS 30
extern EOF_RS_PREDEFINED_SECTION eof_rs_predefined_sections[EOF_NUM_RS_PREDEFINED_SECTIONS];

#define EOF_NUM_RS_PREDEFINED_EVENTS 4
extern EOF_RS_PREDEFINED_SECTION eof_rs_predefined_events[EOF_NUM_RS_PREDEFINED_EVENTS];

int eof_is_string_muted(EOF_SONG *sp, unsigned long track, unsigned long note);
	//Returns nonzero if all used strings in the note are fret hand muted

unsigned long eof_build_chord_list(EOF_SONG *sp, unsigned long track, unsigned long **results);
	//Parses the given pro guitar track, building a list of all unique chords
	//Memory is allocated to *results to return the list, referring to each note by number
	//The function returns the number of unique chords contained in the list
	//If there are no chords in the pro guitar track, or upon error, *results is set to NULL and 0 is returned

unsigned long eof_build_section_list(EOF_SONG *sp, unsigned long **results, unsigned long track);
	//Parses the given chart, building a list of all unique section markers (case insensitive),
	//from the perspective of the specified track (ie. a bass-specific section is not found when a guitar track is specified)
	//Memory is allocated to *results to return the list, referring to each text event by number
	//The function returns the number of unique sections contained in the list
	//If there are no sections in the pro guitar track, or upon error, *results is set to NULL and 0 is returned

int eof_export_rocksmith_track(EOF_SONG * sp, char * fn, unsigned long track, char *user_warned);
	//Writes the specified pro guitar track in Rocksmith's XML format, if the track is populated
	//fn is expected to point to an array at least 1024 bytes in size.  It's filename is altered to reflect the track's name (ie /PART REAL_GUITAR.xml)
	//If *user_warned is zero, the user is alerted that a track difficulty has no defined fret hand positions and they will be automatically generated, then *user_warned is set to nonzero
void eof_rs_compile_xml(EOF_SONG *sp, char *fn, unsigned long track);
	//Compiles the specified XML file for the specified track using the Rocksmith toolkit
	//The ouput SNG file will be written to the input file's folder
void eof_xml_compile_failure_log(char *logfilename);
	//Displays the specified log file, to be called if the call to xml2sng results in no SNG file having been created

void eof_pro_guitar_track_fix_fingerings(EOF_PRO_GUITAR_TRACK *tp, char *undo_made);
	//Checks all notes in the track and duplicates finger arrays of notes with complete finger definitions to matching notes without complete finger definitions
	//If any note has invalid fingering, it is cleared and will be allowed to be set by a valid fingering from a matching note.
	//If *undo_made is zero, this function will create an undo state before modifying the chart and will set the referenced variable to nonzero

int eof_pro_guitar_note_fingering_valid(EOF_PRO_GUITAR_TRACK *tp, unsigned long note);
	//Returns 0 if the fingering is invalid for the specified note (partially defined, or a finger specified for a string that is played open)
	//Returns 1 if the fingering is fully defined for the specified note
	//Returns 2 if the fingering is undefined for the specified note

void eof_song_fix_fingerings(EOF_SONG *sp, char *undo_made);
	//Runs eof_pro_guitar_track_fix_fingerings() on all pro guitar tracks in the specified chart
	//If *undo_made is zero, this function will create an undo state before modifying the chart and will set the referenced variable to nonzero

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
unsigned long eof_pro_guitar_track_find_effective_fret_hand_position_definition(EOF_PRO_GUITAR_TRACK *tp, unsigned char difficulty, unsigned long position);
	//Similar to eof_pro_guitar_track_find_effective_fret_hand_position(), but returns the hand position definition number in effect, or 0 if none are in effect
	//This is for use in eof_menu_song_fret_hand_positions() to pre-select the hand position in effect at the current seek position when the dialog is launched
unsigned long eof_find_effective_rs_phrase(unsigned long position);
	//Returns the phrase number in effect (at or before) at the specified timestamp, or 0 if no phrase was in effect at that position
	//This function assumes that the beat statistics are properly cached for the active track
int eof_time_range_is_populated(EOF_SONG *sp, unsigned long track, unsigned long start, unsigned long stop, unsigned char diff);
	//Returns nonzero if there are any notes in the specified track difficulty within the specified time range
	//Returnz zero if that range contains no notes or upon error

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

#endif
