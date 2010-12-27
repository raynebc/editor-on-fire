#ifndef EOF_MENU_NOTE_H
#define EOF_MENU_NOTE_H

#include <allegro.h>

extern MENU eof_note_menu[];

extern DIALOG eof_lyric_dialog[];
extern DIALOG eof_split_lyric_dialog[];

extern char eof_solo_menu_mark_text[32];
extern char eof_star_power_menu_mark_text[32];
extern char eof_lyric_line_menu_mark_text[32];

void eof_prepare_note_menu(void);

int eof_menu_solo_mark(void);
int eof_menu_solo_unmark(void);
int eof_menu_solo_erase_all(void);

int eof_menu_star_power_mark(void);
int eof_menu_star_power_unmark(void);
int eof_menu_star_power_erase_all(void);

int eof_menu_lyric_line_mark(void);
int eof_menu_lyric_line_unmark(void);
int eof_menu_lyric_line_erase_all(void);
int eof_menu_lyric_line_toggle_overdrive(void);

int eof_menu_hopo_auto(void);
int eof_menu_hopo_cycle(void);					//Cycles the selected note(s) between auto, on and off HOPO modes
int eof_menu_hopo_force_on(void);
int eof_menu_hopo_force_off(void);

int eof_transpose_possible(int dir);			//Tests ability of instrument/vocal pitches to transpose by the given amount
int eof_menu_note_transpose_up(void);
int eof_menu_note_transpose_down(void);
int eof_menu_note_transpose_down_octave(void);	//Moves selected lyrics down one octave if possible
int eof_menu_note_transpose_up_octave(void);	//Moves selected lyrics up one octave if possible
int eof_menu_note_resnap(void);
int eof_menu_note_create_bre(void);
int eof_menu_note_toggle_crazy(void);
int eof_menu_note_toggle_double_bass(void);			//Toggles the Expert+ double bass flag on selected expert bass drum notes
int eof_menu_note_toggle_rb3_cymbal_green(void);	//Toggles the RB3 Pro green cymbal flag on selected purple drum notes (which correspond to green notes in RB)
int eof_menu_note_toggle_rb3_cymbal_yellow(void);	//Toggles the RB3 Pro yellow cymbal flag on selected yellow drum notes
int eof_menu_note_toggle_rb3_cymbal_blue(void);		//Toggles the RB3 Pro blue cymbal flag on selected blue drum notes
int eof_menu_note_remove_cymbal(void);				//Removes cymbal notation from selected drum notes
int eof_menu_note_default_cymbal(void);				//Toggles whether newly-placed blue, yellow or green drum notes are marked as cymbals automatically
int eof_menu_note_push_back(void);
int eof_menu_note_push_up(void);
int eof_menu_note_delete(void);
int eof_menu_set_freestyle(char status);	//Applies the specified freestyle status to all selected lyrics
int eof_menu_set_freestyle_on(void);		//Applies freestyle for all selected lyrics
int eof_menu_set_freestyle_off(void);		//Removes freestyle for all selected lyrics
int eof_menu_toggle_freestyle(void);		//Toggles the freestyle status of all selected lyrics
int eof_menu_note_edit_pro_guitar_note(void);	//Allows a pro guitar's fret values to be defined

int eof_menu_note_toggle_green(void);
int eof_menu_note_toggle_red(void);
int eof_menu_note_toggle_yellow(void);
int eof_menu_note_toggle_blue(void);
int eof_menu_note_toggle_purple(void);
int eof_menu_note_toggle_orange(void);

int eof_menu_split_lyric(void);

#endif
