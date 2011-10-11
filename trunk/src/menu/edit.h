#ifndef EOF_MENU_EDIT_H
#define EOF_MENU_EDIT_H

extern MENU eof_edit_paste_from_menu[];
extern MENU eof_edit_zoom_menu[];
extern MENU eof_edit_snap_menu[];
extern MENU eof_edit_selection_menu[];
extern MENU eof_edit_hopo_menu[];
extern MENU eof_edit_speed_menu[];
extern MENU eof_edit_menu[];

extern DIALOG eof_custom_snap_dialog[];

void eof_prepare_edit_menu(void);		//Enable/disable Edit menu items prior to drawing the menu

int eof_menu_edit_undo(void);
int eof_menu_edit_redo(void);
int eof_menu_edit_copy(void);
int eof_menu_edit_paste_logic(int oldpaste);	//If oldpaste is nonzero, uses old paste logic (notes paste to positions relative to each other), otherwise uses new paste logic (notes paste into positions relative to the copied notes positions within their beats)
int eof_menu_edit_paste(void);					//Calls eof_menu_edit_paste_logic() to use new paste logic
int eof_menu_edit_old_paste(void);				//Calls eof_menu_edit_paste_logic() to use old paste logic
int eof_menu_edit_metronome(void);
int eof_menu_edit_claps_all(void);
int eof_menu_edit_claps_green(void);
int eof_menu_edit_claps_red(void);
int eof_menu_edit_claps_yellow(void);
int eof_menu_edit_claps_blue(void);
int eof_menu_edit_claps_purple(void);
int eof_menu_edit_claps_orange(void);
int eof_menu_edit_claps(void);
int eof_menu_edit_claps_helper(unsigned long menu_item,char claps_flag);	//Sets the specified clap note setting
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
int eof_menu_edit_deselect_all(void);
int eof_menu_edit_select_rest(void);
int eof_menu_edit_select_previous(void);		//Selects all notes before the last selected note

int eof_menu_edit_snap_quarter(void);
int eof_menu_edit_snap_eighth(void);
int eof_menu_edit_snap_sixteenth(void);
int eof_menu_edit_snap_thirty_second(void);
int eof_menu_edit_snap_off(void);
int eof_menu_edit_snap_twelfth(void);
int eof_menu_edit_snap_twenty_fourth(void);
int eof_menu_edit_snap_forty_eighth(void);
int eof_menu_edit_snap_custom(void);

int eof_menu_edit_hopo_rf(void);
int eof_menu_edit_hopo_fof(void);
int eof_menu_edit_hopo_off(void);
int eof_menu_edit_hopo_manual(void);			//A HOPO preview mode that only displays forced HOPO notes as HOPOs
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

int eof_menu_edit_playback_speed_helper_faster(void);
int eof_menu_edit_playback_speed_helper_slower(void);
int eof_menu_edit_playback_100(void);
int eof_menu_edit_playback_75(void);
int eof_menu_edit_playback_50(void);
int eof_menu_edit_playback_25(void);
int eof_menu_edit_playback_custom(void);	//Allow user to specify custom playback rate between 1 and 99 percent speed

int eof_menu_edit_speed_slow(void);
int eof_menu_edit_speed_medium(void);
int eof_menu_edit_speed_fast(void);

int eof_menu_edit_paste_from_supaeasy(void);
int eof_menu_edit_paste_from_easy(void);
int eof_menu_edit_paste_from_medium(void);
int eof_menu_edit_paste_from_amazing(void);
int eof_menu_edit_paste_from_difficulty(unsigned long source_difficulty);	//Copies instrument notes from the specified difficulty into the currently selected difficulty
int eof_menu_edit_paste_from_catalog(void);

void eof_sanitize_note_flags(unsigned long *flags, unsigned long sourcetrack, unsigned long desttrack);
	//Clears flag bits that are invalid for the specified track and resolves status conflicts (ie. a note cannot slide up and down at the same time)
	//For some flags that are used for different statuses for different instruments, flags will be cleared as necessary (ie. during paste)
void eof_menu_edit_paste_clear_range(unsigned long track, int note_type, unsigned long start, unsigned long end);
	//Deletes all notes in the specified track difficulty that fall within the given start and end positions
void eof_menu_paste_read_clipboard_note(PACKFILE * fp, EOF_EXTENDED_NOTE *temp_note);
	//Reads the next note's (for tracks except PART VOCALS) worth of data from the open clipboard file
unsigned long eof_prepare_note_flag_merge(unsigned long flags, unsigned long track_behavior, unsigned long note);
	//Accepts an existing note's flags, the note's track behavior, and the bitmask of a note that will
	//merge with the existing note.  Any lane-specific flag for a lane that is populated in the
	//specified note bitmask will be cleared from the specified flags variable.  The updated flags
	//will be returned so that the calling function can process them as necessary.  This will allow
	//the existing note to inherit the flags of the merging note.

#endif
