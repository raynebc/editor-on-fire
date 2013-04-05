#ifndef EOF_MENU_BEAT_H
#define EOF_MENU_BEAT_H

#include "../song.h"

extern MENU eof_beat_time_signature_menu[];
extern MENU eof_beat_menu[];
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
int eof_menu_beat_ts_3_4(void);
int eof_menu_beat_ts_5_4(void);
int eof_menu_beat_ts_6_4(void);
int eof_menu_beat_ts_custom(void);
int eof_menu_beat_ts_off(void);

int eof_apply_key_signature(int signature, unsigned long beatnum, EOF_SONG *sp);	//Applies the specified key signature to the specified beat
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

int eof_menu_beat_delete(void);
int eof_menu_beat_anchor(void);
int eof_menu_beat_push_offset_back(char *undo_made);
	//Appends a new beat structure and moves all beats one forward, with their timestamps adjusted to compensate for the duration of the first beat.  Returns nonzero on success.
	//If *undo_made is zero, an undo state is made before altering the chart and *undo_made is set to nonzero
int eof_menu_beat_push_offset_back_menu(void);	//Calls eof_menu_beat_push_offset_back() with the option of making an undo state
int eof_menu_beat_push_offset_up(void);
int eof_menu_beat_toggle_anchor(void);
int eof_menu_beat_delete_anchor(void);
int eof_menu_beat_calculate_bpm(void);
int eof_menu_beat_reset_bpm(void);
int eof_menu_beat_all_events(void);
int eof_menu_beat_events(void);
int eof_menu_beat_clear_events(void);
int eof_menu_beat_clear_non_rs_events(void);	//Similar to eof_menu_beat_clear_events(), but only removes text events that don't have any of the Rocksmith flags
int eof_menu_beat_reset_offset(void);	//Similar to eof_menu_beat_push_offset_back(), but uses anchoring to change the MIDI offset to 0 without moving any existing beats, notes, text events, etc.
int eof_menu_beat_trainer_event(void);
int eof_edit_trainer_proc(int msg, DIALOG *d, int c);	//This is a modification of eof_verified_edit_proc() allowing the trainer strings to be redrawn when the input field is altered

int eof_all_events_radio_proc(int msg, DIALOG *d, int c);	//A radio button procedure that checks to see if the filter in the "All Events" dialog was changed.  If it was, it has Allegro redraw the list of events
unsigned long eof_retrieve_text_event(unsigned long index);	//Returns the actual event number of the specified filtered event number (for use with the "All Events" dialog)

int eof_menu_beat_double_tempo(void);		//Performs eof_double_tempo() on the currently selected beat
int eof_menu_beat_halve_tempo(void);		//Performs eof_halve_tempo() on the currently selected beat
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

void eof_add_or_edit_text_event(EOF_TEXT_EVENT *ptr, unsigned long flags, char *undo_made);
	//If ptr is NULL, then a blank event dialog is launched, allowing the user to add a new text event, and the flags parameter is handled as follows:
	//	If (function & EOF_EVENT_FLAG_RS_PHRASE) is true, the "Rocksmith phrase marker" option is automatically checked, otherwise that checkbox is initialized to clear
	//	Similarly, if (function & EOF_EVENT_FLAG_RS_SECTION) is true, the "Rocksmith section marker" option is automatically checked
	//	Similarly, if (function & EOF_EVENT_FLAG_RS_EVENT) is true, the "Rocksmith event marker" option is automatically checked
	//If ptr is not NULL, the event dialog is populated with the specified event's details and the event dialog is launched, allowing the user to edit the existing text event
	//If *undo_made is zero, an undo state is made before altering the chart and *undo_made is set to nonzero
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
char * eof_events_list_all(int index, int * size);	//Dialog logic to display the chart's text events in the "All Events" list box

int eof_menu_beat_copy_rs_events(void);
	//Copies the RS section and RS phrase applicable to the selected beat in the current track to an event clipboard file
	//If the applicable RS section and phrase are the same event, only one event is stored on the clipboard
int eof_menu_beat_paste_rs_events(void);
	//Adds the events stored in the event clipboard to the selected beat

int eof_events_dialog_move_up(DIALOG * d);	//If the selected event in Beat>Events is not the first event listed, swaps its position to be one higher (earlier) in the list
int eof_events_dialog_move_down(DIALOG * d);	//If the selected event in Beat>Events is not the last event listed, swaps its position tob e one lower (later) in the list

#endif
