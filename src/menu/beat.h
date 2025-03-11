#ifndef EOF_MENU_BEAT_H
#define EOF_MENU_BEAT_H

#include "../song.h"
#include "../dialog.h"

extern MENU eof_beat_time_signature_menu[];
extern MENU eof_beat_menu[];
extern MENU eof_filtered_beat_menu[EOF_SCRATCH_MENU_SIZE];
extern MENU *eof_effective_beat_menu;
extern char eof_ts_menu_off_text[32];

extern MENU eof_beat_key_signature_menu[];
extern char eof_ks_menu_off_text[32];

extern char eof_trainer_string[5];

extern DIALOG eof_events_dialog[];
extern DIALOG eof_all_events_dialog[];
extern DIALOG eof_events_add_dialog[];
extern DIALOG eof_bpm_change_dialog[];
extern DIALOG eof_anchor_dialog[];
extern DIALOG eof_place_trainer_dialog[];
extern DIALOG eof_rocksmith_section_dialog[];
extern DIALOG eof_rocksmith_event_dialog[];

void eof_prepare_beat_menu(void);			//Enable/disable Beat menu items prior to drawing the menu
int eof_menu_beat_add(void);
int eof_menu_beat_bpm_change(void);
int eof_menu_beat_ts_4_4(void);
int eof_menu_beat_ts_2_4(void);
int eof_menu_beat_ts_3_4(void);
int eof_menu_beat_ts_5_4(void);
int eof_menu_beat_ts_6_4(void);
int eof_menu_beat_ts_custom_dialog(unsigned start);
	//Calls the custom TS dialog and places the chosen TS on the selected beat, making an undo state, allowing the calling function to handle remaining steps
	//If start is nonzero, the numerator field is given initial focus on dialog launch, otherwise the denominator is
	//Returns nonzero if a valid time signature was chosen and applied to the selected beat
int eof_menu_beat_ts_custom(void);	//Applies user-selected time signature and recalculates beat lengths depending on the accurate TS chart option
int eof_menu_beat_ts_convert(void);	//Applies user-selected time signature and recalculates tempos to leave beat markers in their existing positions
int eof_menu_beat_ts_off(void);

int eof_apply_key_signature(int signature, unsigned long beatnum, EOF_SONG *sp, char *undo_made);
	//Applies the specified key signature to the specified beat
	//If *undo_made is zero, an undo state is made before altering the chart and *undo_made is set to nonzero
int eof_menu_beat_ks_7_flats(void);
int eof_menu_beat_ks_6_flats(void);
int eof_menu_beat_ks_5_flats(void);
int eof_menu_beat_ks_4_flats(void);
int eof_menu_beat_ks_3_flats(void);
int eof_menu_beat_ks_2_flats(void);
int eof_menu_beat_ks_1_flat(void);
int eof_menu_beat_ks_0_flats(void);
int eof_menu_beat_ks_1_sharp(void);
int eof_menu_beat_ks_2_sharps(void);
int eof_menu_beat_ks_3_sharps(void);
int eof_menu_beat_ks_4_sharps(void);
int eof_menu_beat_ks_5_sharps(void);
int eof_menu_beat_ks_6_sharps(void);
int eof_menu_beat_ks_7_sharps(void);
int eof_menu_beat_ks_off(void);

void eof_menu_beat_delete_logic(unsigned long beat);	//Deletes the specified beats, recalculates the tempo map and moves affected text events back one beat marker
int eof_menu_beat_delete(void);
int eof_menu_beat_anchor(void);
int eof_menu_beat_push_offset_back(char *undo_made);
	//Appends a new beat structure and moves all beats one forward, with their timestamps adjusted to compensate for the duration of the first beat.  Returns nonzero on success.
	//If *undo_made is zero, an undo state is made before altering the chart and *undo_made is set to nonzero
int eof_menu_beat_push_offset_back_menu(void);	//Calls eof_menu_beat_push_offset_back() with the option of making an undo state
int eof_menu_beat_push_offset_up(void);
int eof_menu_beat_toggle_anchor(void);
int eof_menu_beat_move_to_seek_pos(void);	//Moves the selected beat to the seek position if there are no anchors between the selected beat's start and destination positions
int eof_menu_beat_export_beat_timings(void); //Exports all beat timings to either milliseconds or intervals per second

int eof_menu_beat_delete_anchor_logic(char *undo_made);
	//If the currently selected beat is an anchor, deletes its anchor status, assigning it the previous anchor's tempo and updating beat positions
	//If *undo_made is zero, an undo state is made before altering the chart and *undo_made is set to nonzero
int eof_menu_beat_delete_anchor(void);
	//Calls eof_menu_beat_delete_anchor_logic() so that an undo state is performed

int eof_menu_beat_anchor_measures(void);
	//For beats that have a time signature in effect:
	//Anchors the first beat of every measure and deletes other anchors, allowing auto-adjust to perform if it is enabled

int eof_menu_beat_copy_tempo_map(void);
	//For all beat markers within the scope of the start and end points, copies the beats' tempos and time signatures to a clipboard file, to be applied later starting at a selected beat marker
int eof_menu_beat_paste_tempo_map(void);
	//Starting at the selected beat, applies the tempo and time signature changes stored by eof_menu_beat_copy_tempo_map()
int eof_menu_beat_validate_tempo_map(void);
	//Calls eof_detect_tempo_map_corruption() with the option to report a valid tempo map
int eof_menu_beat_lock_tempo_map(void);
	//Toggle the setting to lock the tempo map on/off

int eof_menu_beat_remove_mid_beat_status(void);	//If the selected beat has the EOF_BEAT_FLAG_MIDBEAT flag set, creates an undo state and clears that flag
int eof_menu_beat_calculate_bpm(void);
int eof_menu_beat_reset_bpm(void);		//Applies the first beat's tempo to all beats in the project
int eof_menu_beat_remove_ts(void);		//Removes all time signature changes from the project
int eof_menu_beat_all_events(void);
int eof_menu_beat_events(void);
int eof_menu_beat_clear_events(void);
int eof_menu_beat_clear_non_rs_events(void);	//Similar to eof_menu_beat_clear_events(), but only removes text events that don't have any of the Rocksmith flags
int eof_menu_beat_reset_offset(void);	//Similar to eof_menu_beat_push_offset_back(), but uses anchoring to change the MIDI offset to 0 without moving any existing beats, notes, text events, etc.
int eof_menu_beat_trainer_event(void);
int eof_edit_trainer_proc(int msg, DIALOG *d, int c);	//This is a modification of eof_verified_edit_proc() allowing the trainer strings to be redrawn when the input field is altered
int eof_menu_beat_add_section(void);

int eof_menu_beat_double_tempo(void);		//Performs eof_double_tempo() on the currently selected beat
int eof_menu_beat_halve_tempo(void);		//Performs eof_halve_tempo() on the currently selected beat
int eof_menu_beat_double_tempo_all(void);	//Performs eof_double_tempo() on all beats
int eof_menu_beat_halve_tempo_all(void);	//Performs eof_halve_tempo() on tall beats
int eof_menu_beat_set_RBN_tempos(void);
	//Uses eof_double_tempo() and eof_halve_tempo() to attempt to get all tempos within RBN's requirements of 40BPM to 300BPM
	//If this is not possible, the chart seeks to the problematic tempo and warns the user that it must be corrected manually

int eof_menu_beat_adjust_bpm(double amount);
	//Alters the tempo of the beat/anchor at/before the seek position by adding the specified amount
	//If the Feedback input method is in effect, the beat at/before the seek position is altered, otherwise the anchor at/before the seek position is altered
unsigned long eof_events_dialog_delete_events_count(void);
	//Counts the number of text events defined at the currently selected beat
void eof_rebuild_trainer_strings(void);
	//Recreates the trainer section strings appropriate for the currently pro guitar/bass track into eof_etext2, eof_etext3 and eof_etext4

int eof_events_add_check_proc(int msg, DIALOG *d, int c);
	//A checkbox procedure that disables the "Rocksmith solo phrase" checkbox if the user has just unchecked the "Rocksmith phrase marker" checkbox
	//If the user has just checked the "Rocksmith phrase marker" checkbox, the "Rocksmith solo phrase" checkbox becomes unchecked and enabled
void eof_add_or_edit_text_event(EOF_TEXT_EVENT *ptr, unsigned long flags, char *undo_made);
	//If ptr is NULL, then a blank event dialog is launched, allowing the user to add a new text event, and the flags parameter is handled as follows:
	//	If (function & EOF_EVENT_FLAG_RS_PHRASE) is true, the "Rocksmith phrase marker" option is automatically checked, otherwise that checkbox is initialized to clear
	//	Similarly, if (function & EOF_EVENT_FLAG_RS_SECTION) is true, the "Rocksmith section marker" option is automatically checked
	//	Similarly, if (function & EOF_EVENT_FLAG_RS_EVENT) is true, the "Rocksmith event marker" option is automatically checked
	//If ptr is not NULL, the event dialog is populated with the specified event's details and the event dialog is launched, allowing the user to edit the existing text event
	//If *undo_made is zero, an undo state is made before altering the chart and *undo_made is set to nonzero
void eof_add_or_edit_floating_text_event(EOF_TEXT_EVENT *ptr, unsigned long flags, char *undo_made);
	//Similar to eof_add_or_edit_text_event(), but only creates/edits floating text events
int eof_events_dialog_add_function(unsigned long function);
	//Launches the add new text event dialog.
	//If (function & EOF_EVENT_FLAG_RS_PHRASE) is true, the "Rocksmith phrase marker" option is automatically checked, otherwise that checkbox is initialized to clear
	//Similarly, if (function & EOF_EVENT_FLAG_RS_SECTION) is true, the "Rocksmith section marker" option is automatically checked
	//Similarly, if (function & EOF_EVENT_FLAG_RS_EVENT) is true, the "Rocksmith event marker" option is automatically checked
int eof_events_dialog_add(DIALOG * d);	//Performs the Text event add action presented in the Events dialog
int eof_events_dialog_edit(DIALOG * d);	//Performs the Text event edit action presented in the Events dialog
int eof_events_dialog_delete(DIALOG * d);	//Performs the Text event delete action presented in the Events dialog
int eof_rocksmith_phrase_dialog_add(void);
	//Calls eof_events_dialog_add_function with a function value of EOF_EVENT_FLAG_RS_PHRASE
int eof_rocksmith_section_dialog_add(void);
	//Launches a dialog to allow the user to add a Rocksmith section to the selected beat from a pre-defined list of valid section names
int eof_rocksmith_event_dialog_add(void);
	//Launches a dialog to allow the user to add a Rocksmith event to the selected beat from a pre-defined list of valid event names
char * eof_rs_section_add_list(int index, int * size);
	//List box function for eof_rocksmith_section_dialog_add()
char * eof_rs_event_add_list(int index, int * size);
	//List box function for eof_rocksmith_event_dialog_add()
char * eof_events_list(int index, int * size);		//Dialog logic to display the chart's text events in the "Events" list box

int eof_all_events_radio_proc(int msg, DIALOG *d, int c);
	//A radio button procedure that checks to see if the filter in the "All Events" dialog was changed.
	//If it was, it has Allegro redraw the list of events
int eof_all_events_check_proc(int msg, DIALOG *d, int c);
	//A checkbox procedure that checks to see if the "This track only" filter in the "All Events" dialog was changed.
	//If it was, it has Allegro redraw the list of events
unsigned long eof_retrieve_text_event(unsigned long index);
	//Returns the actual event number of the specified filtered event number (for use with the "All Events" dialog)
int eof_event_is_not_filtered_from_listing(unsigned long index);
	//Returns nonzero if the specified event is visible from the perspective of the "All Events" dialog's filters
	//Returns zero on error
char * eof_events_list_all(int index, int * size);	//Dialog logic to display the chart's text events in the "All Events" list box

int eof_menu_beat_copy_rs_events(void);
	//Copies the RS section and RS phrase applicable to the selected beat in the current track to an event clipboard file
	//If the applicable RS section and phrase are the same event, only one event is stored on the clipboard
int eof_menu_beat_copy_events(void);
	//Copies all events on the selected beat that are visible to the active track to the event clipboard file
int eof_menu_beat_paste_events(void);
	//Adds the events stored in the event clipboard to the selected beat

int eof_events_dialog_move_up(DIALOG * d);	//If the selected event in Beat>Events is not the first event listed, swaps its position to be one higher (earlier) in the list
int eof_events_dialog_move_down(DIALOG * d);	//If the selected event in Beat>Events is not the last event listed, swaps its position tob e one lower (later) in the list

int eof_menu_beat_estimate_bpm(void);	//Uses the MiniBPM source package to estimate the tempo of the current chart audio

#endif
