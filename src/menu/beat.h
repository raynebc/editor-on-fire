#ifndef EOF_MENU_BEAT_H
#define EOF_MENU_BEAT_H

extern MENU eof_beat_time_signature_menu[];
extern MENU eof_beat_menu[];

extern char eof_ts_menu_off_text[32];
extern char eof_trainer_string[5];

extern DIALOG eof_events_dialog[];
extern DIALOG eof_all_events_dialog[];
extern DIALOG eof_events_add_dialog[];
extern DIALOG eof_bpm_change_dialog[];
extern DIALOG eof_anchor_dialog[];
extern DIALOG eof_place_trainer_dialog[];

void eof_prepare_beat_menu(void);			//Enable/disable Beat menu items prior to drawing the menu
int eof_menu_beat_add(void);
int eof_menu_beat_bpm_change(void);
int eof_menu_beat_ts_4_4(void);
int eof_menu_beat_ts_3_4(void);
int eof_menu_beat_ts_5_4(void);
int eof_menu_beat_ts_6_4(void);
int eof_menu_beat_ts_custom(void);

int eof_menu_beat_ts_off(void);
int eof_menu_beat_delete(void);
int eof_menu_beat_anchor(void);
int eof_menu_beat_push_offset_back(void);	//Appends a new beat structure and moves all beats one forward, with their timestamps adjusted to compensate for the duration of the first beat.  Returns nonzero on success.
int eof_menu_beat_push_offset_up(void);
int eof_menu_beat_toggle_anchor(void);
int eof_menu_beat_delete_anchor(void);
int eof_menu_beat_calculate_bpm(void);
int eof_menu_beat_reset_bpm(void);
int eof_menu_beat_all_events(void);
int eof_menu_beat_events(void);
int eof_menu_beat_clear_events(void);
int eof_menu_beat_reset_offset(void);	//Similar to eof_menu_beat_push_offset_back(), but uses anchoring to change the MIDI offset to 0 without moving any existing beats, notes, text events, etc.
int eof_menu_beat_trainer_event(void);
int eof_edit_trainer_proc(int msg, DIALOG *d, int c);	//This is a modification of eof_verified_edit_proc() allowing the trainer strings to be redrawn when the input field is altered

int eof_all_events_radio_proc(int msg, DIALOG *d, int c);	//A radio button procedure that checks to see if the filter in the "All Events" dialog was changed.  If it was, it has Allegro redraw the list of events
unsigned long eof_retrieve_text_event(unsigned long index);	//Returns the actual event number of the specified filtered event number (for use with the "All Events" dialog)

#endif
