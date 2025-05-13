#ifndef EOF_MENU_EDIT_H
#define EOF_MENU_EDIT_H

#include "../song.h"

extern MENU eof_edit_paste_from_menu[];
extern MENU eof_edit_zoom_menu[];
extern MENU eof_edit_snap_menu[];
extern MENU eof_edit_selection_menu[];
extern MENU eof_edit_hopo_menu[];
extern MENU eof_edit_speed_menu[];
extern MENU eof_edit_menu[];
extern MENU eof_edit_claps_menu[];

extern DIALOG eof_custom_snap_dialog[];
extern DIALOG eof_menu_edit_select_by_note_length_dialog[];

void eof_prepare_edit_menu(void);		//Enable/disable Edit menu items prior to drawing the menu

int eof_menu_edit_undo(void);
int eof_menu_edit_redo(void);
int eof_menu_edit_copy(void);
int eof_menu_edit_cut(unsigned long anchor, int option);
	//Stores "Auto adjust" data to "eof.autoadjust" from the first anchor before the specified beat number up to a one of two positions specified by the option variable
	//If option is 1, the function stores data for all notes up until 1ms before the last beat marker (tempo change/increment/decrement applied to beat marker),
	// otherwise data for notes up until the next anchor are stored (auto-adjust when dragging anchors)
	//Option 0 is deprecated because the use of eof_calculate_beats() can recalculate timings for beats not directly in the scope of a beat marker drag operation,
	// and subsequently cause grid snap loss
int eof_menu_edit_cut_paste(unsigned long anchor, int option);
	//Performs "Auto adjust" logic (ie. when anchors are manipulated) to write the notes that were stored in "eof.autoadjust"
	//option should be the same value as when the eof.autoadjust data was created
	//The affected notes are deleted and then recreated with those in the autoadjust file
int eof_menu_edit_paste_logic(int function);
	//If function is 0, uses new paste logic (notes paste into positions relative to the copied notes positions within their beats)
	//If function is 1, uses old paste logic (notes paste to positions relative to each other)
	//If function is 2, uses new paste logic at the pen note position
int eof_menu_edit_paste(void);					//Calls eof_menu_edit_paste_logic() to use new paste logic
int eof_menu_edit_old_paste(void);				//Calls eof_menu_edit_paste_logic() to use old paste logic
int eof_menu_edit_paste_at_mouse(void);			//Calls eof_menu_edit_paste_logic() to use paste at pen note position logic
int eof_menu_edit_metronome(void);
int eof_menu_edit_claps_all(void);
int eof_menu_edit_claps_green(void);
int eof_menu_edit_claps_red(void);
int eof_menu_edit_claps_yellow(void);
int eof_menu_edit_claps_blue(void);
int eof_menu_edit_claps_purple(void);
int eof_menu_edit_claps_orange(void);
int eof_menu_edit_claps(void);
int eof_menu_edit_claps_helper(unsigned long menu_item, char claps_flag);	//Sets the specified clap note setting
int eof_menu_edit_vocal_tones(void);
int eof_menu_edit_midi_tones(void);		//Toggle MIDI tones on/off
int eof_menu_edit_bookmark_0(void);
int eof_menu_edit_bookmark_1(void);
int eof_menu_edit_bookmark_2(void);
int eof_menu_edit_bookmark_3(void);
int eof_menu_edit_bookmark_4(void);
int eof_menu_edit_bookmark_5(void);
int eof_menu_edit_bookmark_6(void);
int eof_menu_edit_bookmark_7(void);
int eof_menu_edit_bookmark_8(void);
int eof_menu_edit_bookmark_9(void);

int eof_menu_edit_select_all(void);
int eof_menu_edit_select_like(void);			//For each unique selected note, selects all matching notes/lyrics in the same track and difficulty
int eof_menu_edit_precise_select_like(void);	//Similar to eof_menu_edit_select_like(), but also requires notes have identical ghost bitmask, flags and extended flags in order to match
int eof_menu_edit_deselect_all(void);
int eof_menu_edit_select_rest(void);
int eof_menu_edit_select_previous(void);			//Selects all notes before the last selected note
int eof_menu_edit_invert_selection(void);			//Inverts the note selection (notes that aren't selected become selected and vice versa)

int eof_check_note_conditional_selection(EOF_SONG *sp, unsigned long track, unsigned long notenum, unsigned long match_bitmask, unsigned long cymbal_match_bitmask);
	//Examines the specified note and returns nonzero if it matches the conditions selected in eof_menu_edit_conditional_selection_dialog[]
	//match_bitmask is a bitmask reflecting the checked lane numbers in the dialog
	//cymbal_match_bitmask is a bitmask reflecting the checked cymbal numbers in the dialog
int eof_menu_edit_conditional_selection_logic(int function);
	//If function is zero, notes in the active track are deselected based on the criteria set in eof_menu_edit_conditional_selection_dialog[]
	//otherwise such notes are selected
int eof_menu_edit_deselect_conditional(void);		//Allows user to specify conditions for deselecting notes from the current selection
int eof_menu_edit_select_conditional(void);			//Allows user to specify conditions for selecting notes in the current track difficulty

int eof_check_pgnote_status_selection(EOF_SONG *sp, unsigned long track, unsigned long notenum, unsigned long match_flags, unsigned long match_eflags);
	//Examines the specified pro guitarnote and returns nonzero if it matches the conditions selected in eof_menu_edit_pgstatus_selection_dialog[]
	//match_flags is a bitmask reflecting the checked status flags in the dialog
	//match_eflags is a bitmask reflecting the checked extended status flags in the dialog
int eof_menu_edit_pgstatus_selection_logic(int function);
	//If function is zero, notes in the active track are deselected based on the criteria set in eof_menu_edit_status_selection_dialog[]
	//otherwise such notes are selected
int eof_menu_edit_deselect_status(void);		//Allows user to specify statuses for deselecting notes from the current selection
int eof_menu_edit_select_status(void);			//Allows user to specify statuses for selecting notes in the current track difficulty

int eof_menu_edit_select_logic(int (*check)(EOF_SONG *, unsigned long, unsigned long));
	//Passes notes in the active track difficulty to the specified function that accepts a song structure pointer, a track number and a note number (in that order)
	// and which returns nonzero if it meets the conditions being checked, adding those that do to the current note selection
	//Returns 0 on error
int eof_menu_edit_select_chords(void);			//Selects notes that have more than 1 gem
int eof_menu_edit_select_single_notes(void);	//Selects notes that have only 1 gem
int eof_menu_edit_select_toms(void);			//Selects drum notes that contain any toms (lane 3, 4 or 5 gems that aren't cymbals)
int eof_menu_edit_select_cymbals(void);			//Selects drum notes that contain any cymbals (lane 3, 4 or 5 gems that aren't toms)
int eof_menu_edit_select_grid_snapped_notes(void);		//Selects notes that are on grid snap positions
int eof_menu_edit_select_non_grid_snapped_notes(void);	//Selects notes that are not on grid snap positions
int eof_menu_edit_select_highlighted_notes(void);		//Selects notes that are highlighted
int eof_menu_edit_select_non_highlighted_notes(void);	//Selects notes that are not highlighted
int eof_menu_edit_select_open_notes(void);				//Selects notes that are open notes
int eof_menu_edit_select_non_open_notes(void);			//Selects notes that are not open notes
int eof_menu_edit_select_notes_needing_fingering(void);	//Selcts notes that don't have fingering properly defined

int eof_menu_edit_deselect_logic(int (*check)(EOF_SONG *, unsigned long, unsigned long));
	//Passes notes in the active track difficulty to the specified function that returns nonzero if it meets the conditions being checked, removing those that do from the current note selection
	//Returns 0 on error
int eof_menu_edit_deselect_chords(void);			//Deselects notes that have more than 1 gem
int eof_menu_edit_deselect_single_notes(void);		//Deselects notes that have only 1 gem
int eof_menu_edit_deselect_toms(void);				//Deselects drum notes that contain any toms (lane 3, 4 or 5 gems that aren't cymbals)
int eof_menu_edit_deselect_cymbals(void);			//Deselects drum notes that contain any cymbals (lane 3, 4 or 5 gems that aren't toms)
int eof_menu_edit_deselect_grid_snapped_notes(void);		//Deselects notes that are on grid snap positions
int eof_menu_edit_deselect_non_grid_snapped_notes(void);	//Deselects notes that are not on grid snap positions
int eof_menu_edit_deselect_highlighted_notes(void);		//Deselects notes that are highlighted
int eof_menu_edit_deselect_non_highlighted_notes(void);	//Deselects notes that are not highlighted
int eof_menu_edit_deselect_open_notes(void);			//Deselects notes that are open notes
int eof_menu_edit_deselect_non_open_notes(void);		//Deselects notes that are not open notes

int eof_menu_edit_select_gem_count_logic(int function);
	//If function is zero, notes in the active track are deselected based on whether notes do or don't have a specified number of gems
	//otherwise such notes are selected
int eof_menu_edit_select_gem_count(void);	//Calls eof_menu_edit_select_gem_count_logic() with the option to select notes
int eof_menu_edit_deselect_gem_count(void);	//Calls eof_menu_edit_select_gem_count_logic() with the option to deselect notes

int eof_menu_edit_select_on_or_off_beat_note_logic(int function, int position);
	//Alters the note selection based on notes in the active track difficulty that are either on a beat marker (position is nonzero) or not on a beat marker (position is zero)
	//If function is nonzero such notes are deselected, otherwise such notes are selected
	//Returns 0 on error
int eof_menu_edit_deselect_on_beat_notes(void);		//Calls eof_menu_edit_select_on_or_off_beat_note_logic() with the option to deselect on beat notes
int eof_menu_edit_deselect_off_beat_notes(void);	//Calls eof_menu_edit_select_on_or_off_beat_note_logic() with the option to deselect off beat notes
int eof_menu_edit_select_on_beat_notes(void);		//Calls eof_menu_edit_select_on_or_off_beat_note_logic() with the option to select on beat notes
int eof_menu_edit_select_off_beat_notes(void);		//Calls eof_menu_edit_select_on_or_off_beat_note_logic() with the option to select off beat notes

int eof_menu_edit_select_note_number_in_sequence_logic(int function);
	//Prompts user to specify a sequence number of a sequence length to alter the note selection
	//If function is zero, note # (sequence number) of every (sequence length) number of notes that are already selected becomes deselected
	//If function is nonzero, note # (sequence number) of every (sequence length) number of notes become selected, regardless of their current selection status
int eof_menu_edit_select_note_number_in_sequence(void);		//Calls eof_menu_edit_select_note_number_in_sequence_logic() with the option to select notes
int eof_menu_edit_deselect_note_number_in_sequence(void);	//Calls eof_menu_edit_select_note_number_in_sequence_logic() with the option to deselect notes

int eof_menu_edit_select_by_note_length_logic(int (*check)(long, long), int function);
	//Prompts the user to enter a threshold time using eof_menu_edit_select_by_note_length_dialog[]
	//A threshold of 0 is interpreted as one grid snap of the current grid snap length
	//Passes notes in the active track difficulty to the specified function that compares the note length (first argument) with the threshold (second argument)
	// and returns nonzero if the desired criterion is met
	//If function is zero, matching notes are removed from the note selection, otherwise they are added to the note selection
	//Returns 0 on error
int eof_menu_edit_select_all_shorter_than(void);	//Selects all notes in the active track difficulty that are shorter than a user specified length
int eof_menu_edit_select_all_longer_than(void);		//Selects all notes in the active track difficulty that are longer than a user specified length
int eof_menu_edit_select_all_of_length(void);		//Selects all notes in the active track difficulty that are exactly of the user specified length
int eof_menu_edit_deselect_all_shorter_than(void);	//Deselects all notes in the active track difficulty that are shorter than a user specified length
int eof_menu_edit_deselect_all_longer_than(void);	//Deselects all notes in the active track difficulty that are longer than a user specified length
int eof_menu_edit_deselect_all_of_length(void);		//Deselects all notes in the active track difficulty that are exactly of the user specified length

int eof_menu_edit_select_note_less_than_threshhold_of_next_note_logic(int function);
	//Prompts the user to enter a threshold time, a threshold of 0 is interpreted as one grid snap of the current grid snap length
	//Selects/deselects notes depending on whether they are less than the given threshold distance away of the note that immediately follows them
	//If function is zero, matching notes are removed from the note selection, otherwise they are added to the note selection
	//Returns 0 on error
int eof_menu_edit_select_note_within_threshhold_of_next_note_logic(int function, int function2);
	//Selects/deselects notes based on their proximity to the following note
	// If function is zero, deselection is performed, otherwise selection is performed
	// If function2 is zero, the note's starting position compared to the next note's position, otherwise the former's end position is compared
int eof_menu_edit_select_note_starting_within_threshhold_of_next_note(void);
	//Uses eof_menu_edit_select_note_within_threshhold_of_next_note_logic() to select all notes in the active track difficulty beginning within a threshold distance from the note they precede
int eof_menu_edit_deselect_note_starting_within_threshhold_of_next_note(void);
	//Uses eof_menu_edit_select_note_within_threshhold_of_next_note_logic() to deselect all notes in the active track difficulty beginning within a threshold distance from the note they precede
int eof_menu_edit_select_note_ending_within_threshhold_of_next_note(void);
	//Uses eof_menu_edit_select_note_within_threshhold_of_next_note_logic() to select all notes in the active track difficulty ending within a threshold distance from the note they precede
int eof_menu_edit_deselect_note_ending_within_threshhold_of_next_note(void);
	//Uses eof_menu_edit_select_note_within_threshhold_of_next_note_logic() to deselect all notes in the active track difficulty ending within a threshold distance from the note they precede

int eof_menu_edit_set_start_point_at_timestamp(unsigned long timestamp);
	//Sets/clears the start point at the given timestamp
int eof_menu_edit_set_start_point(void);
	//Calls eof_menu_edit_set_start_point_at_timestamp() specifying the seek position as the timestamp
int eof_menu_edit_set_start_point_at_mouse(void);
	//Calls eof_menu_edit_set_start_point_at_timestamp() specifying the pen note (mouse) position as the timestamp
int eof_menu_edit_set_end_point_at_timestamp(unsigned long timestamp);
	//Sets/clears the end point at the given timestamp
int eof_menu_edit_set_end_point(void);
	//Calls eof_menu_edit_set_end_point_at_timestamp() specifying the seek position as the timestamp
int eof_menu_edit_set_end_point_at_mouse(void);
	//Calls eof_menu_edit_set_end_point_at_timestamp() specifying the pen note (mouse) position as the timestamp

int eof_menu_edit_snap_quarter(void);
int eof_menu_edit_snap_eighth(void);
int eof_menu_edit_snap_sixteenth(void);
int eof_menu_edit_snap_thirty_second(void);
int eof_menu_edit_snap_off(void);
int eof_menu_edit_snap_twelfth(void);
int eof_menu_edit_snap_twenty_fourth(void);
int eof_menu_edit_snap_forty_eighth(void);
int eof_menu_edit_snap_sixty_forth(void);
int eof_menu_edit_snap_ninty_sixth(void);
int eof_custom_grid_snap_edit_proc(int msg, DIALOG *d, int c);
	//If the user types the letter b, sets the "beat" radio button in eof_custom_snap_dialog[]
	//If the user types the letter m, sets the "measure" radio button in eof_custom_snap_dialog[]
	//All other input is filtered through eof_verified_edit_proc()
int eof_menu_edit_snap_custom(void);
int eof_menu_edit_toggle_grid_lines(void);

int eof_menu_edit_hopo_rf(void);
int eof_menu_edit_hopo_fof(void);
int eof_menu_edit_hopo_off(void);
int eof_menu_edit_hopo_manual(void);			//A HOPO preview mode that only displays forced HOPO notes as HOPOs
int eof_menu_edit_hopo_gh3(void);
int eof_menu_edit_hopo_helper(int hopo_view);	//Sets the specified HOPO preview mode

#define EOF_NUM_ZOOM_LEVELS 10
int eof_menu_edit_zoom_helper_in(void);
int eof_menu_edit_zoom_helper_out(void);
int eof_menu_edit_zoom_10(void);
int eof_menu_edit_zoom_9(void);
int eof_menu_edit_zoom_8(void);
int eof_menu_edit_zoom_7(void);
int eof_menu_edit_zoom_6(void);
int eof_menu_edit_zoom_5(void);
int eof_menu_edit_zoom_4(void);
int eof_menu_edit_zoom_3(void);
int eof_menu_edit_zoom_2(void);
int eof_menu_edit_zoom_1(void);
int eof_menu_edit_zoom_level(int zoom);	//Sets the zoom to the specified level
int eof_menu_edit_zoom_custom(void);	//Sets the zoom to a user defined level

int eof_menu_edit_playback_speed_helper_faster(void);
int eof_menu_edit_playback_speed_helper_slower(void);
int eof_menu_edit_playback_time_stretch(void);
int eof_menu_edit_playback_100(void);
int eof_menu_edit_playback_75(void);
int eof_menu_edit_playback_50(void);
int eof_menu_edit_playback_25(void);
int eof_menu_edit_playback_custom(void);	//Allow user to specify custom playback rate between 1 and 99 percent speed

#define EOF_ZOOM_3D_MIN 1
#define EOF_ZOOM_3D_MAX 5
int eof_menu_edit_speed_number(int value);
int eof_menu_edit_speed_slow(void);
int eof_menu_edit_speed_medium(void);
int eof_menu_edit_speed_fast(void);
int eof_menu_edit_speed_hyper(void);

int eof_menu_edit_paste_from_supaeasy(void);
int eof_menu_edit_paste_from_easy(void);
int eof_menu_edit_paste_from_medium(void);
int eof_menu_edit_paste_from_amazing(void);
int eof_menu_edit_paste_from_challenge(void);
int eof_menu_edit_paste_from_difficulty(unsigned long source_difficulty, char *undo_made);
	//Copies instrument notes from the specified difficulty into the currently selected difficulty
	//Also prompts to copy any existing fret hand positions and arpeggios if the track is a pro guitar track
	//If *undo_made is zero, an undo state is made and the referenced value is set to nonzero
int eof_menu_edit_paste_from_catalog(void);

void eof_sanitize_note_flags(unsigned long *flags, unsigned long sourcetrack, unsigned long desttrack);
	//Clears flag bits that are invalid for the specified track and resolves status conflicts (ie. a note cannot slide up and down at the same time)
	//For some flags that are used for different statuses for different instruments, flags will be cleared as necessary (ie. during paste)
	//Extended flags are always track-specific and should be appropriately removed when pasting from one track format to another
void eof_menu_edit_paste_clear_range(unsigned long track, int note_type, unsigned long start, unsigned long end);
	//Deletes all notes in the specified track difficulty that START within the given start and end positions
void eof_read_clipboard_note(PACKFILE *fp, EOF_EXTENDED_NOTE *temp_note, unsigned long namelength);
	//Reads one note definition from the specified clipboard file into the given extended note structure
	//namelength specifies the maximum number of characters to store into temp_note
	//  Notes can store EOF_NAME_LENGTH + 1 characters, but lyrics can store EOF_MAX_LYRIC_LENGTH + 1 characters
void eof_read_clipboard_position_beat_interval_data(PACKFILE *fp, unsigned long *beat, unsigned char *intervalvalue, unsigned char *intervalnum);
	//Reads one timestamp's worth of beat interval position data from the specified clipboard file into the specified pointers
void eof_write_clipboard_note(PACKFILE *fp, EOF_SONG *sp, unsigned long track, unsigned long note, unsigned long first_pos);
	//Writes the specified note to the specified clipboard file
	//first_pos is the position of the first note being copied to the clipboard, since each selected note's position is
	//  written as a relative position of the first selected note (ie. the first note is written with a position of 0)
void eof_write_clipboard_position_snap_data(PACKFILE *fp, unsigned long pos);
	//Write's the specified timestamp's grid snap position data to the specified clipboard file
unsigned long eof_prepare_note_flag_merge(unsigned long flags, unsigned long track_behavior, unsigned long notemask);
	//Accepts an existing note's flags, the note's track behavior, and the bitmask of a note that will
	//merge with the existing note.  Any lane-specific flag for a lane that is populated in the
	//specified note bitmask will be cleared from the specified flags variable.  The updated flags
	//will be returned so that the calling function can process them as necessary.  This will allow
	//the existing note to inherit the flags of the merging note.
int eof_menu_edit_copy_vocal(void);
	//Copies selected lyrics to the vocal clipboard file
int eof_menu_edit_paste_vocal_logic(int function);
///	//Using the vocal clipboard file as the source, if oldpaste is nonzero, uses old paste logic (notes paste to positions relative to each other), otherwise uses new paste logic (notes paste into positions relative to the copied notes positions within their beats)
	//If function is 0, uses new paste logic (notes paste into positions relative to the copied notes positions within their beats)
	//If function is 1, uses old paste logic (notes paste to positions relative to each other)
	//If function is 2, uses new paste logic at the pen note position
int eof_menu_edit_bookmark_helper(int b);	//Sets the specified bookmark number to the current seek position
char * eof_menu_song_paste_from_difficulty_list(int index, int * size);
	//The list dialog function for eof_menu_song_paste_from_difficulty();
int eof_menu_song_paste_from_difficulty(void);
	//Allows the user to copy one difficulty's contents into the active track difficulty

int eof_menu_edit_benchmark_rubberband(void);	//Times how long it takes to have rubberband process the current chart audio to half tempo

#endif
