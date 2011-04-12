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
int eof_menu_note_default_double_bass(void);		//Toggles whether newly-placed expert bass drum notes are marked as expert+ automatically
int eof_menu_note_push_back(void);					//A currently unused Feedback input mode function
int eof_menu_note_push_up(void);
int eof_menu_note_delete(void);
int eof_menu_set_freestyle(char status);	//Applies the specified freestyle status to all selected lyrics
int eof_menu_set_freestyle_on(void);		//Applies freestyle for all selected lyrics
int eof_menu_set_freestyle_off(void);		//Removes freestyle for all selected lyrics
int eof_menu_toggle_freestyle(void);		//Toggles the freestyle status of all selected lyrics

int eof_menu_note_edit_pro_guitar_note(void);	//Allows a pro guitar's fret values to be defined
int eof_menu_note_toggle_tapping(void);			//Toggles the tapping status of all selected notes
int eof_menu_note_remove_tapping(void);			//Removes the tapping status of all selected notes
int eof_menu_note_toggle_slide_up(void);		//Toggles the slide up status of all selected notes
int eof_menu_note_toggle_slide_down(void);		//Toggles the slide down status of all selected notes
int eof_menu_note_remove_slide(void);			//Removes the slide status of all selected notes
int eof_menu_note_toggle_palm_muting(void);		//Toggles the palm muting status of all selected notes
int eof_menu_note_remove_palm_muting(void);		//Removes the palm muting status of all selected notes
int eof_menu_arpeggio_mark(void);				//Marks/remarks an arpeggio phrase encompassing the selected notes
int eof_menu_arpeggio_unmark(void);				//Removes arpeggio phrases that include any of the selected notes
void eof_pro_guitar_track_delete_arpeggio(EOF_PRO_GUITAR_TRACK * tp, unsigned long index);	//Deletes the specified arpeggio phrase
int eof_menu_arpeggio_erase_all(void);			//Removes all arpeggio phrases, freeing phrase names as necessary
int eof_menu_trill_mark(void);					//Marks/remarks a trill phrase encompassing the seleted notes
int eof_menu_tremolo_mark(void);				//Marks/remarks a tremolo phrase encompassing the seleted notes
int eof_menu_trill_unmark(void);				//Removes trill phrases that include any of the selected notes
int eof_menu_tremolo_unmark(void);				//Removes tremolo phrases that include any of the selected notes
int eof_menu_trill_erase_all(void);				//Removes all trill phrases, freeing phrase names as necessary
int eof_menu_tremolo_erase_all(void);			//Removes all tremolo phrases, freeing phrase names as necessary
int eof_menu_set_max_fret(void);				//Allows the maximum fret number for a pro guitar track to be defined
int eof_menu_note_clear_legacy_values(void);	//Resets the legacy bitmasks of all selected notes

int eof_menu_note_toggle_green(void);
int eof_menu_note_toggle_red(void);
int eof_menu_note_toggle_yellow(void);
int eof_menu_note_toggle_blue(void);
int eof_menu_note_toggle_purple(void);
int eof_menu_note_toggle_orange(void);

int eof_menu_split_lyric(void);

#endif
