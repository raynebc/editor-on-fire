#ifndef EOF_MENU_TRACK_H
#define EOF_MENU_TRACK_H

#include "../song.h"
#include "../dialog.h"

extern MENU eof_track_menu[];
extern MENU eof_filtered_track_menu[EOF_SCRATCH_MENU_SIZE];
extern MENU eof_track_rocksmith_menu[];
extern MENU eof_track_proguitar_menu[];
extern MENU eof_track_rocksmith_arrangement_menu[];

extern DIALOG eof_pro_guitar_tuning_dialog[];
extern DIALOG eof_note_set_num_frets_strings_dialog[];
extern DIALOG eof_fret_hand_position_list_dialog[];

extern char eof_fret_hand_position_list_dialog_undo_made;			//Used to track an undo state having been made in the fret hand positions dialog
extern char eof_fret_hand_position_list_dialog_title_string[30];	//This will be rebuilt by the list box count function to display the number of positions present

extern char **eof_track_rs_tone_names_list_strings;				//A list of unique tone names that is build with eof_track_rebuild_rs_tone_names_list_strings()
extern unsigned long eof_track_rs_tone_names_list_strings_num;	//Indicates the number of entries in the above array

extern char eof_note_name_search_string[256];
extern char eof_note_name_search_dialog_title[50];
extern DIALOG eof_note_name_search_dialog[];
extern char eof_note_name_find_next_menu_name[20];
extern DIALOG eof_name_search_replace_dialog[];

int eof_menu_track_set_difficulty_0(void);
int eof_menu_track_set_difficulty_1(void);
int eof_menu_track_set_difficulty_2(void);
int eof_menu_track_set_difficulty_3(void);
int eof_menu_track_set_difficulty_4(void);
int eof_menu_track_set_difficulty_5(void);
int eof_menu_track_set_difficulty_6(void);
int eof_menu_track_set_difficulty_none(void);
int eof_menu_track_set_difficulty(unsigned difficulty);
	//Sets the difficulty level of the active track to a specific number or to an undefined state
	//Any secondary (pro drum, harmony) difficulty level for the active track is also set to the specified number
int eof_track_difficulty_dialog(void);
	//Allow the active track's difficulty to be set to a value from 0 through 6
	//In the case of drum tracks, the regular and pro drum difficulties are individually definable
	//In the case of vocal tracks, the regular and harmony difficulties are individually definable
int eof_track_rocksmith_toggle_difficulty_limit(void);
	//Toggles the 5 difficulty limit on/off.  If difficulties >= 4 are populated, appropriate warnings are given to the user regarding Rock Band vs. Rocksmith standards compatibility
int eof_track_rocksmith_insert_difficulty(void);
	//If the active pro guitar/bass track has fewer than 255 difficulties, prompts user to insert a new difficulty above or below the active difficulty
	//If the new difficulty has a populated difficulty above or below it, the user is prompted whether to import either (calling paste from logic)
int eof_track_delete_difficulty(void);
	//Deletes the active difficulty's content, decrementing all higher difficulty's content accordingly

extern char *eof_rocksmith_dynamic_difficulty_list_array[256];
	//Stores the pointer to each difficulty level's string that is allocated
char * eof_rocksmith_dynamic_difficulty_list_proc(int index, int * size);
	//List procedure for eof_rocksmith_dynamic_difficulty_list()
int eof_rocksmith_dynamic_difficulty_list(void);
	//Lists statistics about each dynamic difficulty level in the track

int eof_track_rename(void);				//Allows the user to define an alternate name for the active track

int eof_track_tuning(void);				//Allows the active track's tuning to be defined
int eof_tuning_definition_is_applicable(char track_is_bass, unsigned char numstrings, unsigned definition_num);
	//Returns nonzero if the specified tuning definition applies to a track with the given criteria
char * eof_tunings_list(int index, int * size);
	//A list function to display all of the pre-defined tunings
int eof_tuning_preset(void);
	//Displays a dialog allowing the user to set one of the defined tuning definitions for the active track
	//Returns nonzero if the dialog is canceled
int eof_edit_tuning_proc(int msg, DIALOG *d, int c);
	//This is a modification of eof_verified_edit_proc() allowing the note names and tuning name to be redrawn when a tuning field is altered
int eof_track_set_num_frets_strings(void);
	//Allows the track's number of frets (pro guitar/bass) and strings (pro bass only) to be defined
int eof_track_pro_guitar_set_capo_position(void);
	//Allows the track's capo position to be defined
int eof_track_pro_guitar_toggle_ignore_tuning(void);
	//Toggles the option to have the chord name detection disregard the track's defined tuning and capo position and base it on standard tuning instead
int eof_track_transpose_tuning(EOF_PRO_GUITAR_TRACK* tp, char *tuningdiff);
	//Receives a pointer to an array of 6 signed chars indicating a subtractive tuning offset (in half steps) for each string
	//If any existing notes in the specified track will be affected by the tuning change, the user is prompted whether or not to transpose the notes
	//If so, the affected fret values will be raised or lowered accordingly
	//If a note cannot be altered (ie. values of 0 or the track's max fret number would be surpassed), the user is warned and the note is highlighted

unsigned long eof_track_pro_guitar_set_fret_hand_position_dialog_timestamp;
	//Stores the working timestamp used by the eof_track_pro_guitar_set_fret_hand_position_dialog() function, so the <- and -> functions can alter the target timestamp
int eof_track_pro_guitar_set_fret_hand_position_at_timestamp(unsigned long timestamp);
	//Allows the user to define a fret hand position for the active difficulty
int eof_track_pro_guitar_set_fret_hand_position(void);
	//Calls eof_track_pro_guitar_set_fret_hand_position_at_timestamp() specifying the seek position as the target timestamp
int eof_track_pro_guitar_set_fret_hand_position_at_mouse(void);
	//Calls eof_track_pro_guitar_set_fret_hand_position_at_timestamp() specifying the pen note (mouse) position as the target timestamp
int eof_track_pro_guitar_move_fret_hand_position_prev_note(DIALOG * d);
	//Alters the fret hand position set/edit dialog's target timestamp to the previous note in the active track difficulty, if applicable
int eof_track_pro_guitar_move_fret_hand_position_next_note(DIALOG * d);
	//Alters the fret hand position set/edit dialog's target timestamp to the next note in the active track difficulty, if applicable
int eof_track_fret_hand_positions(void);
	//Displays the fret hand positions defined for the active track difficulty, allowing them to be deleted
int eof_track_fret_hand_positions_copy_from(void);
	//Allows the user to copy fret hand positions defined in another difficulty into the current track difficulty
char * eof_track_fret_hand_positions_copy_from_list(int index, int * size);
	//The list dialog function for eof_track_fret_hand_positions_copy_from()
int eof_track_delete_effective_fret_hand_position(void);
	//Deletes the fret hand position in effect at the current seek position in the active track difficulty, if any
int eof_track_delete_fret_hand_position_at_pos(unsigned long track, unsigned char diff, unsigned long pos);
	//Deletes the fret hand defined at the specified timestamp of the specified track difficulty, if any
int eof_track_fret_hand_positions_generate_all(void);
	//Generates fret hand positions for all populated difficulties in the track, replacing any that already exist

int eof_track_rs_popup_add(void);
	//Allows the user to add a popup message displayed in-game in Rocksmith
int eof_track_rs_popup_messages(void);
	//Displays the popup messages defined for the current pro guitar/bass track
char * eof_track_rs_popup_messages_list(int index, int * size);
	//List dialog function for eof_track_rs_popup_messages()
int eof_track_rs_popup_messages_edit(DIALOG * d);
	//Allows the user to edit a popup message from the list
void eof_track_pro_guitar_sort_popup_messages(EOF_PRO_GUITAR_TRACK* tp);
	//Sorts the popup messages for the specified track
int eof_track_rs_popup_messages_delete(DIALOG * d);
	//Deletes the selected popup message from eof_track_rs_popup_messages()
int eof_track_rs_popup_messages_delete_all(DIALOG * d);
	//Deletes all popup messages from eof_track_rs_popup_messages()
int eof_track_rs_popup_messages_seek(DIALOG * d);
	//Seeks to the selected popup message from eof_track_rs_popup_messages()
void eof_track_pro_guitar_delete_popup_message(EOF_PRO_GUITAR_TRACK *tp, unsigned long index);
	//Deletes the specified popup message
int eof_find_effective_rs_popup_message(unsigned long pos, unsigned long *popupnum);
	//Returns nonzero if there is a popup message in effect for the current pro guitar/bass track at the specified position
	//The corresponding popup message number for the track is returned through popupnum
	//Zero is returned if no popup message is in effect or upon error

int eof_track_rs_tone_change_add_at_timestamp(unsigned long timestamp);
	//Allows the user to add a tone change at the specified timestamp, for use in Rocksmith
int eof_track_rs_tone_change_add(void);
	//Calls eof_track_rs_tone_change_add_at_timestamp() specifying the seek position as the timestamp
int eof_track_rs_tone_change_add_at_mouse(void);
	//Calls eof_track_rs_tone_change_add_at_timestamp() specifying the pen note (mouse) position as the timestamp
int eof_track_rs_tone_changes(void);
	//Displays the tone changes defined for the current pro guitar/bass track
char * eof_track_rs_tone_changes_list(int index, int * size);
	//List dialog function for eof_track_rs_tone_changes()
int eof_track_rs_tone_changes_edit(DIALOG * d);
	//Allows the user to edit a tone change from the list
void eof_track_pro_guitar_sort_tone_changes(EOF_PRO_GUITAR_TRACK* tp);
	//Sorts the tone changes for the specified track
int eof_track_rs_tone_changes_delete(DIALOG * d);
	//Deletes the selected tone change from eof_track_rs_tone_changes()
int eof_track_rs_tone_changes_delete_all(DIALOG * d);
	//Deletes all tone changes from eof_track_rs_tone_changes()
int eof_track_rs_tone_changes_seek(DIALOG * d);
	//Seeks to the selected tone change from eof_track_rs_tone_changes()
void eof_track_pro_guitar_delete_tone_change(EOF_PRO_GUITAR_TRACK *tp, unsigned long index);
	//Deletes the specified tone change
int eof_track_find_effective_rs_tone_change(unsigned long pos, unsigned long *changenum);
	//Returns nonzero if there is a tone change for the current pro guitar/bass track at or before the specified position
	//The corresponding tone change number for the track is returned through changenum
	//Zero is returned if no tone change is in effect or upon error

int eof_track_rs_tone_names(void);
	//Displays the tone names referenced by all tone changes in the current pro guitar/bass track
char * eof_track_rs_tone_names_list(int index, int * size);
	//List dialog function for eof_track_rs_tone_names()
void eof_track_rebuild_rs_tone_names_list_strings(unsigned long track, char allowsuffix);
	//Allocates memory for and builds eof_track_rs_tone_names_list_strings[] to include a list of strings containing all unique tone names used in the specified track
	//If allowsuffix is nonzero, " (D)" is appended to the tone name string of whichever of the tone names is the current default (if any)
	//	This parameter should be passed as zero when using the names for anything besides display purposes
	//eof_track_rs_tone_names_list_strings_num indicates how many strings are in the array
	//If the specified track's default tone is no longer valid (not used by any tone changes), its default tone string is emptied
void eof_track_destroy_rs_tone_names_list_strings(void);
	//Destroys the eof_track_rs_tone_names_list_strings[] array and frees its memory
int eof_track_rs_tone_names_default(DIALOG * d);
	//Sets the selected tone name to be the default tone for the active track
int eof_track_rs_tone_names_rename(DIALOG * d);
	//Renames all instances of the selected tone name

int eof_track_rocksmith_arrangement_set(unsigned char num);
	//Validates that the active track is a pro guitar track and sets the track's arrangement type to the specified number
	//If num is > 4, a value of 0 is set instead
int eof_track_rocksmith_arrangement_undefined(void);	//Sets an undefined arrangement type
int eof_track_rocksmith_arrangement_combo(void);		//Sets a combo arrangement type
int eof_track_rocksmith_arrangement_rhythm(void);		//Sets a rhythm arrangement type
int eof_track_rocksmith_arrangement_lead(void);			//Sets a lead arrangement type
int eof_track_rocksmith_arrangement_bass(void);			//Sets a bass arrangement type

int eof_track_manage_rs_phrases(void);
	//Displays the phrases defined for the current pro guitar/bass track, along with the maxdifficulty of each
int eof_track_manage_rs_phrases_seek(DIALOG * d);
	//Seeks to the beat containing the selected phrase
int eof_track_manage_rs_phrases_add_or_remove_level(int function);
	//Used by eof_track_manage_rs_phrases_add_level() and eof_track_manage_rs_phrases_delete_level() to manipulate the difficulties for the selected RS phrase
	//If function is negative, the notes, arpeggios, hand positions and tremolos in the active difficulty are deleted, and those of higher difficulties are decremented
	//If function is >= 0, the notes, arpeggios, hand positions and tremolos in the active difficulty are duplicated into the next difficulty, and those of higher difficulties are incremented
	//	Also, the presence of a hand position at the first note of the leveled up phrase is enforced, as this is done by official Rocksmith charts to ensure hand positions stay correct despite leveling up/down
int eof_track_manage_rs_phrases_add_level(DIALOG * d);
	//Increments the difficulty level of the notes, arpeggios and hand positions in the selected phrase (either the selected instance or within all instances, depending on user selection)
	//and copies the now higher-difficulty's content back into the active difficulty, making it convenient to author a new difficulty level for the notes in question.
int eof_track_manage_rs_phrases_remove_level(DIALOG * d);
	//Decrements the notes within the selected phrase (either the selected instance or within all instances, depending on user selection)
	//and deletes the affected notes in the current difficulty, making it convenient to remove a difficulty level for the notes in question.

int eof_track_flatten_difficulties(void);	//Launches a dialog to specify a threshold distance and calls eof_flatten_difficulties on the active track difficulty
int eof_track_unflatten_difficulties(void);	//Runs eof_unflatten_difficulties() on the active track difficulty

void eof_prepare_track_menu(void);

int eof_menu_track_open_strum(void);		//Toggle the ability to use a sixth lane in the active legacy guitar/bass track
int eof_menu_track_five_lane_drums(void);	//Toggle the ability to use a fifth lane in PART DRUM on/off
int eof_menu_track_disable_double_bass_drums(void);	//Toggle the setting to disable expert+ bass drum gem export on/off
int eof_menu_track_drumsrock_enable_drumsrock_export(void);	//Toggle the setting to export a drum track to Drums Rock format
int eof_menu_track_drumsrock_enable_remap(void);	//Toggle the setting to use defined remappings for drum notes during Drums Rock preview and export
int eof_menu_track_unshare_drum_phrasing(void);
	//By default, the drum and PS drum tracks use the same star power, solo, drum roll and special drum roll phrases
	//This function toggles that setting on/off

int eof_menu_track_beatable_enable_beatable_export(void);
	//Toggle the setting to export a non legacy drum track to BEATABLE format

int eof_track_erase_track(void);			//Allows the user to erase the active track's contents
int eof_track_erase_track_difficulty(void);	//Allows the user to erase the active track difficulty's contents

int eof_menu_track_remove_highlighting(void);	//Removes highlighting from the notes in the active track

extern MENU eof_menu_track_rocksmith_tone_change_copy_menu[EOF_TRACKS_MAX];
int eof_menu_track_copy_tone_changes_track_8(void);
int eof_menu_track_copy_tone_changes_track_9(void);
int eof_menu_track_copy_tone_changes_track_11(void);
int eof_menu_track_copy_tone_changes_track_12(void);
int eof_menu_track_copy_tone_changes_track_14(void);
int eof_menu_track_copy_tone_changes_track_number(EOF_SONG *sp, unsigned long sourcetrack, unsigned long desttrack);
	//Copies the tone changes from the specified source track to the destination track

extern MENU eof_menu_track_rocksmith_popup_copy_menu[EOF_TRACKS_MAX];
int eof_menu_track_copy_popups_track_8(void);
int eof_menu_track_copy_popups_track_9(void);
int eof_menu_track_copy_popups_track_11(void);
int eof_menu_track_copy_popups_track_12(void);
int eof_menu_track_copy_popups_track_14(void);
int eof_menu_track_copy_popups_track_number(EOF_SONG *sp, unsigned long sourcetrack, unsigned long desttrack);
	//Copies the popup messages from the specified source track to the destination track

extern MENU eof_track_proguitar_fret_hand_copy_menu[EOF_TRACKS_MAX];
int eof_menu_track_copy_fret_hand_track_8(void);
int eof_menu_track_copy_fret_hand_track_9(void);
int eof_menu_track_copy_fret_hand_track_11(void);
int eof_menu_track_copy_fret_hand_track_12(void);
int eof_menu_track_copy_fret_hand_track_14(void);
int eof_menu_track_copy_fret_hand_track_number(EOF_SONG *sp, unsigned long sourcetrack, unsigned long desttrack);
	//Copies the fret hand positions from the specified source track to the destination track

int eof_menu_track_clone_track_1(void);
int eof_menu_track_clone_track_2(void);
int eof_menu_track_clone_track_3(void);
int eof_menu_track_clone_track_4(void);
int eof_menu_track_clone_track_5(void);
int eof_menu_track_clone_track_6(void);
int eof_menu_track_clone_track_7(void);
int eof_menu_track_clone_track_8(void);
int eof_menu_track_clone_track_9(void);
int eof_menu_track_clone_track_10(void);
int eof_menu_track_clone_track_11(void);
int eof_menu_track_clone_track_12(void);
int eof_menu_track_clone_track_13(void);
int eof_menu_track_clone_track_14(void);
int eof_menu_track_clone_track_number(EOF_SONG *sp, unsigned long sourcetrack, unsigned long desttrack);
	//Replaces the contents of the destination track with a copy of all content from the source track

int eof_menu_thin_notes_track_1(void);
int eof_menu_thin_notes_track_2(void);
int eof_menu_thin_notes_track_3(void);
int eof_menu_thin_notes_track_4(void);
int eof_menu_thin_notes_track_5(void);
int eof_menu_thin_notes_track_6(void);
int eof_menu_thin_notes_track_7(void);
int eof_menu_thin_notes_track_8(void);
int eof_menu_thin_notes_track_9(void);
int eof_menu_thin_notes_track_10(void);
int eof_menu_thin_notes_track_11(void);
int eof_menu_thin_notes_track_12(void);
int eof_menu_thin_notes_track_13(void);
int eof_menu_thin_notes_track_14(void);
	//Thins out the notes in the active track difficulty to match those in the active difficulty in the specified track
	//If a note in the active track isn't within a threshold distance of any note in the specified track, it is deleted

int eof_menu_track_toggle_tech_view(void);	//Toggles tech view for the active pro guitar track
char eof_menu_pro_guitar_track_get_tech_view_state(EOF_PRO_GUITAR_TRACK *tp);	//Returns nonzero if the specified pro guitar track has tech view enabled, otherwise returns zero
void eof_menu_pro_guitar_track_disable_tech_view(EOF_PRO_GUITAR_TRACK *tp);	//Disables tech view for the specified pro guitar track, preserving the tech note array size
void eof_menu_pro_guitar_track_enable_tech_view(EOF_PRO_GUITAR_TRACK *tp);	//Enables tech view for the specified pro guitar track, preserving the normal note array size
void eof_menu_pro_guitar_track_set_tech_view_state(EOF_PRO_GUITAR_TRACK *tp, char state);	//Changes the tech view status for the specified pro guitar track to enabled if state is nonzero, otherwise tech view is disabled
char eof_menu_track_get_tech_view_state(EOF_SONG *sp, unsigned long track);	//Returns nonzero if the specified track is a pro guitar track and has tech view enabled, otherwise returns zero
void eof_menu_track_set_tech_view_state(EOF_SONG *sp, unsigned long track, char state);	//Changes the tech view status for the specified pro guitar track to enabled if state is nonzero, otherwise tech view is disabled
void eof_menu_track_toggle_tech_view_state(EOF_SONG *sp, unsigned long track);	//Toggles the tech view status for the specified pro guitar track
void eof_menu_pro_guitar_track_update_note_counter(EOF_PRO_GUITAR_TRACK *tp);	//Uses the value of tp->notes to update tp->pgnotes if tech view is NOT in effect, or tp->technotes if tech view is in effect

int eof_menu_track_repair_grid_snap(void);
	//Prompts the user for a threshold distance and moves all unsnapped notes in the active track to their nearest grid snap position of any grid size
	// if that grid snap position is within the threshold distance of the note's current timestamp
	//If the auto-adjust tech notes preference is enabled, tech notes are moved the same number of milliseconds as the notes they apply to

int eof_menu_track_clone_track_to_clipboard(void);
	//Creates an "eof.clone.clipboard" file containing all content in the active track
	//Returns zero on success
int eof_menu_track_clone_track_from_clipboard(void);
	//Adds the content defined in "eof.clone.clipboard" into the active track, if compatible (cannot clone a legacy/pro guitar track into PART VOCALS or vice-versa)
	//Returns zero on success

int eof_track_menu_enable_ghl_mode(void);
	//Toggles the "Track>Enable GHL mode" flag for legacy guitar/bass tracks

int eof_menu_track_rs_normal_arrangement(void);			//Clear the bonus and alternate RS arrangement flags
int eof_menu_track_rs_bonus_arrangement(void);			//Set the bonus RS arrangement flag and clear the alternate arrangement flag
int eof_menu_track_rs_alternate_arrangement(void);		//Set the alternate RS arrangement flag and clear the bonus arrangement flag
int eof_menu_track_rs_picked_bass_arrangement(void);	//Toggle the picked bass arrangement flag

int eof_menu_track_offset(void);	//Shifts all track content by a user specified offset

int eof_menu_track_check_chord_snapping(void);
	//Prompts user for a timing unit (deltas or ms) and a threshold amount and checks the active track's notes to determine whether any chords would resnap with those criteria
	//Such instances are highlighted and the user is prompted whether to resnap those notes int chords

int eof_menu_track_erase_note_names(void);
	//Erases the name from every note in the track

int eof_track_set_chord_hand_mode_change(void);
	//Uses eof_pro_guitar_track_set_hand_mode_change_at_timestamp() to set chord hand mode at the seek position
int eof_track_set_string_hand_mode_change(void);
	//Uses eof_pro_guitar_track_set_hand_mode_change_at_timestamp() to set string hand mode at the seek position
int eof_track_delete_effective_hand_mode_change(void);
	//Deletes the hand mode change in effect at the seek position
void eof_track_pro_guitar_sort_hand_mode_changes(EOF_PRO_GUITAR_TRACK* tp);
	//Sorts the specified track's hand mode changes by timestamp

int eof_note_name_find_next(void);		//Seeks to the next note with the specified name (note or lyric)
int eof_name_search_replace(void);		//Performs search and replace for lyric text or note names

#endif
