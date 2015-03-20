#ifndef EOF_MENU_NOTE_H
#define EOF_MENU_NOTE_H

#include <allegro.h>

extern MENU eof_note_menu[];

extern DIALOG eof_lyric_dialog[];
extern DIALOG eof_split_lyric_dialog[];
extern DIALOG eof_note_name_dialog[];

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

int eof_menu_note_toggle_flag(unsigned char function, unsigned char track_format, unsigned long bitmask);
	//Toggles the specified bits of either the normal flags or the extended flags of selected notes in the active track difficulty
	//If function is nonzero, the extended flags are altered, otherwise the normal flags are
	//If track_format is not equal to EOF_ANY_TRACK_FORMAT, the alteration is only performed if the active track difficulty is of the specified track format
	//If no notes are manually selected, but Feedback input mode is in effect, any note at the current seek position is instead modified by this function
	//Functions that do nothing more than toggle one or more bits can call this, otherwise they will need to perform their own logic
int eof_menu_note_clear_flag(unsigned char function, unsigned char track_format, unsigned long bitmask);
	//Clears the specified bits of either the normal flags or the extended flags of selected notes in the active track difficulty
	//If function is nonzero, the extended flags are altered, otherwise the normal flags are
	//If track_format is not equal to EOF_ANY_TRACK_FORMAT, the alteration is only performed if the active track difficulty is of the specified track format
	//If no notes are manually selected, but Feedback input mode is in effect, any note at the current seek position is instead modified by this function
	//Functions that do nothing more than clear one or more bits can call this, otherwise they will need to perform their own logic

int eof_transpose_possible(int dir);			//Tests ability of instrument/vocal pitches to transpose (lower) by the given amount
int eof_menu_note_transpose_up(void);
int eof_menu_note_transpose_down(void);
int eof_menu_note_transpose_down_octave(void);	//Moves selected lyrics down one octave if possible
int eof_menu_note_transpose_up_octave(void);	//Moves selected lyrics up one octave if possible
int eof_menu_note_resnap(void);
int eof_menu_note_toggle_crazy(void);
int eof_menu_note_toggle_double_bass(void);			//Toggles the Expert+ double bass flag on selected expert bass drum notes
int eof_menu_note_remove_double_bass(void);			//Removes the Expert+ double bass flag on selected expert bass drum notes
int eof_menu_note_toggle_rb3_cymbal_green(void);	//Toggles the RB3 Pro green cymbal flag on selected purple drum notes (which correspond to green notes in RB)
int eof_menu_note_toggle_rb3_cymbal_yellow(void);	//Toggles the RB3 Pro yellow cymbal flag on selected yellow drum notes
int eof_menu_note_toggle_rb3_cymbal_blue(void);		//Toggles the RB3 Pro blue cymbal flag on selected blue drum notes
int eof_menu_note_remove_cymbal(void);				//Removes cymbal notation from selected drum notes
int eof_menu_note_default_cymbal(void);				//Toggles whether newly-placed blue, yellow or green drum notes are marked as cymbals automatically
int eof_menu_note_default_double_bass(void);		//Toggles whether newly-placed expert bass drum notes are marked as expert+ automatically
int eof_menu_note_default_open_hi_hat(void);		//Specifies whether newly-placed yellow drum notes are marked as open hi hat automatically
int eof_menu_note_default_pedal_hi_hat(void);		//Specifies whether newly-placed yellow drum notes are marked as pedal hi hat automatically
int eof_menu_note_default_sizzle_hi_hat(void);		//Specifies whether newly-placed yellow drum notes are marked as sizzle hi hat automatically
int eof_menu_note_default_no_hi_hat(void);			//Specifies that newly-placed yellow drum notes are NOT marked as one of the hi hat statuses automatically
int eof_menu_note_push_back(void);					//A currently unused Feedback input mode function
int eof_menu_note_push_up(void);
int eof_menu_note_delete(void);
int eof_menu_set_freestyle(char status);	//Applies the specified freestyle status to all selected lyrics
int eof_menu_set_freestyle_on(void);		//Applies freestyle for all selected lyrics
int eof_menu_set_freestyle_off(void);		//Removes freestyle for all selected lyrics
int eof_menu_toggle_freestyle(void);		//Toggles the freestyle status of all selected lyrics

int eof_menu_note_edit_pro_guitar_note(void);		//Allows a pro guitar note's properties to be defined
int eof_menu_note_edit_pro_guitar_note_frets_fingers(char function, char *undo_made);
	//Allows a pro guitar note's fret and fingering values to be defined.
	//Rocksmith uses finger 0 to define use of the thumb, but EOF uses a value of 0 for undefined, so a user-specified value of 0, 't' or 'T' will be translated and stored as 5.
	//If function is zero, either the fret values or the fingerings can be edited, and if the latter is changed,
	//	this function will offer to update the finger arrays for matching non-selected matching notes in the active track
	//If function is nonzero, the fret value input boxes are disabled, the finger input boxes for unused strings are disabled,
	//	and changes to the fingerings are automatically applied to all matching non-selected notes in the active track
	//Returns zero if user canceled making edits
	//If *undo_made is zero, this function will create an undo state before modifying the chart and will set the referenced variable to nonzero
int eof_menu_note_edit_pro_guitar_note_frets_fingers_menu(void);
	//Calls eof_menu_note_edit_pro_guitar_note_frets_fingers() specifying function 0
int eof_correct_chord_fingerings_option(char report, char *undo_made);
	//Checks each pro guitar track in the active chart for chords that have incorrect fingering (defined when it shouldn't be or vice-versa)
	//If at least one such chord matches a chord shape definition, the function will prompt the user whether to automatically fill in fingering where possible
	//Otherwise for each such chord, the "Edit pro guitar fret/finger values" dialog is launched with the fret fields locked so that the fingerings can be defined
	//The given fingering is then applied to all matching notes in the track
	//If report is nonzero, a dialog message is displayed if no corrections were made, alerting the user that it was already correct
	//If report is zero, a dialog message is displayed BEFORE prompting the user to make corrections, since the user didn't manually invoke the function
	//If *undo_made is zero, this function will create an undo state before modifying the chart and will set the referenced variable to nonzero
int eof_correct_chord_fingerings(void);
	//Calls eof_correct_chord_fingerings_option specifying not to report if there were no corrections needed (to be called during save logic)
int eof_correct_chord_fingerings_menu(void);
	//Calls eof_correct_chord_fingerings_option specifying to report if there were no corrections needed (to be called from menu)

int eof_menu_note_toggle_slide_up(void);		//Toggles the slide up status of all selected notes
int eof_menu_note_toggle_slide_down(void);		//Toggles the slide down status of all selected notes
int eof_menu_note_remove_slide(void);			//Removes the slide status of all selected notes
int eof_menu_note_reverse_slide(void);			//Reverses the slide direction of all selected notes that already slide (also removes the reverse slide status if it is applied)
int eof_menu_note_toggle_palm_muting(void);		//Toggles the palm muting status of all selected notes
int eof_menu_note_remove_palm_muting(void);		//Removes the palm muting status of all selected notes
int eof_menu_arpeggio_mark_logic(int handshape);	//Marks/remarks the selected notes as an arpeggio phrase if handshape is 0, or otherwise as a handshape phrase (a variation of arpeggio phrase)
int eof_menu_arpeggio_mark(void);				//Marks/remarks an arpeggio phrase encompassing the selected notes
int eof_menu_handshape_mark(void);				//Marks/remarks a handshape phrase encompassing the selected notes
int eof_menu_arpeggio_unmark_logic(int handshape);	//Removes either the arpeggio (if handshape is 0) or handshape (if handshape is nonzero) phrases encompassing the selected notes
int eof_menu_arpeggio_unmark(void);				//Removes arpeggio phrases that include any of the selected notes
int eof_menu_handshape_unmark(void);			//Removes handshape phrases that include any of the selected notes
void eof_pro_guitar_track_delete_arpeggio(EOF_PRO_GUITAR_TRACK * tp, unsigned long index);	//Deletes the specified arpeggio or handshape phrase
int eof_menu_arpeggio_erase_all(void);			//Removes all arpeggio phrases, freeing phrase names as necessary
int eof_menu_handshape_erase_all(void);			//Removes all handshape phrases, freeing phrase names as necessary
int eof_menu_trill_mark(void);					//Marks/remarks a trill phrase encompassing the seleted notes
int eof_menu_tremolo_mark(void);				//Marks/remarks a tremolo phrase encompassing the seleted notes
int eof_menu_slider_mark(void);					//Marks/remarks a slider phrase encompassing the selected notes
int eof_menu_trill_unmark(void);				//Removes trill phrases that include any of the selected notes
int eof_menu_tremolo_unmark(void);				//Removes tremolo phrases that include any of the selected notes
int eof_menu_slider_unmark(void);				//Removes slider phrases that include any of the selected notes
int eof_menu_trill_erase_all(void);				//Removes all trill phrases, freeing phrase names as necessary
int eof_menu_tremolo_erase_all(void);			//Removes all tremolo phrases, freeing phrase names as necessary
int eof_menu_slider_erase_all(void);			//Removes all slider phrases, freeing phrase names as necessary
int eof_menu_note_clear_legacy_values(void);	//Resets the legacy bitmasks of all selected notes
int eof_pro_guitar_toggle_strum_up(void);		//Toggles the strum up status for all selected pro guitar/bass notes, clearing strum down statuses where appropriate
int eof_pro_guitar_toggle_strum_down(void);		//Toggles the strum down status for all selected pro guitar/bass notes, clearing strum up statuses where appropriate
int eof_pro_guitar_toggle_strum_mid(void);		//Toggles the strum mid status for all selected pro guitar/bass notes, clearing strum up statuses where appropriate
int eof_menu_note_remove_strum_direction(void);	//Removes the strum direction status of all selected notes
int eof_menu_note_toggle_hi_hat_open(void);		//Toggles the open hi hat status for selected yellow drum notes
int eof_menu_note_toggle_hi_hat_pedal(void);	//Toggles the pedal controlled hi hat status for selected yellow (or red during a disco flip) drum notes
int eof_menu_note_toggle_hi_hat_sizzle(void);	//Toggles the sizzle hi hat status for selected yellow (or red during a disco flip) drum notes
int eof_menu_note_remove_hi_hat_status(void);	//Removes the open, pedal controlled and sizzle hi hat statuses for selected yellow (or red during a disco flip) drum notes
int eof_menu_note_toggle_rimshot(void);			//Toggles the rimshot status for selected red notes
int eof_menu_note_remove_rimshot(void);			//Removes the rimshot status for selected red notes
int eof_menu_note_toggle_tapping(void);			//Toggles the tapping status of all selected pro guitar notes
int eof_menu_note_remove_tapping(void);			//Removes the tapping status of all selected pro guitar notes
int eof_menu_note_toggle_bend(void);			//Toggles the bend status of all selected pro guitar notes
int eof_menu_note_remove_bend(void);			//Removes the bend status of all selected pro guitar notes
int eof_menu_note_toggle_harmonic(void);		//Toggles the harmonic status of all selected pro guitar notes
int eof_menu_note_remove_harmonic(void);		//Removes the harmonic status of all selected pro guitar notes
int eof_menu_note_toggle_vibrato(void);			//Toggles the vibrato status of all selected pro guitar notes
int eof_menu_note_remove_vibrato(void);			//Removes the vibrato status of all selected pro guitar notes
int eof_menu_note_toggle_pop(void);				//Toggles the pop status of all selected pro guitar notes
int eof_menu_note_remove_pop(void);				//Removes the pop status of all selected pro guitar notes
int eof_menu_note_toggle_slap(void);			//Toggles the slap status of all selected pro guitar notes
int eof_menu_note_remove_slap(void);			//Removes the slap status of all selected pro guitar notes
int eof_menu_note_toggle_accent(void);			//Toggles the accent status of all selected pro guitar notes
int eof_menu_note_remove_accent(void);			//Removes the accent status of all selected pro guitar notes
int eof_menu_note_toggle_pinch_harmonic(void);	//Toggles the pinch harmonic status of all selected pro guitar notes
int eof_menu_note_remove_pinch_harmonic(void);	//Removes the pinch harmonic status of all selected pro guitar notes
int eof_menu_note_toggle_rs_sustain(void);		//Toggles the sustain status of all selected pro guitar notes
int eof_menu_note_remove_rs_sustain(void);		//Removes the sustain status of all selected pro guitar notes
int eof_menu_pro_guitar_toggle_hammer_on(void);	//Toggles the hammer on status of all selected pro guitar notes
int eof_menu_pro_guitar_remove_hammer_on(void);	//Removes the hammer on status of all selected pro guitar notes
int eof_menu_pro_guitar_toggle_pull_off(void);	//Toggles the pull off status of all selected pro guitar notes
int eof_menu_pro_guitar_remove_pull_off(void);	//Removes the pull off status of all selected pro guitar notes
int eof_menu_note_toggle_ghost(void);			//Toggles the ghost flag for each populated string (that has keyboard shortcuts enabled) on each selected note
int eof_menu_note_remove_ghost(void);			//Clears the ghost flag for each populated string (that has keyboard shortcuts enabled) on each selected note
int eof_menu_note_remove_rs_ignore(void);		//Removes the ignore status of all selected pro guitar notes
int eof_menu_note_toggle_rs_ignore(void);		//Toggles the ignore status of all selected pro guitar notes
int eof_menu_note_remove_linknext(void);		//Removes the linknext status of all selected pro guitar notes
int eof_menu_note_toggle_linknext(void);		//Toggles the linknext status of all selected pro guitar notes
int eof_pro_guitar_note_slide_end_fret(char undo);
	//Prompts the user for an ending fret number to apply to all selected slide notes.  The number is validated against each selected note's slide direction before it is applied
	//If save is nonzero, a save state is created before any notes are altered
int eof_pro_guitar_note_define_unpitched_slide(void);
	//Toggles unpitched slide status for all selected notes
	//This status is only applied if the user provides a suitable end fret position for the slide
int eof_menu_note_remove_unpitched_slide(void);
	//Removes unpitched slide status from all selected notes
int eof_pro_guitar_note_slide_end_fret_save(void);
	//Calls eof_pro_guitar_note_slide_end_fret() specifying to make an undo before making changes (for when setting slide endings directly from the note menu)
int eof_pro_guitar_note_slide_end_fret_no_save(void);
	//Calls eof_pro_guitar_note_slide_end_fret() specifying not to make an undo before making changes
	//(for when using the keyboard shortcut to toggle slides, since an undo would already have been created)
int eof_pro_guitar_note_bend_strength(char undo);
	//Prompts the user for a bend strength in half steps to apply to all bend notes.  If save is nonzero, a save state is created before any notes are altered
int eof_pro_guitar_note_bend_strength_save(void);
	//Calls eof_pro_guitar_note_bend_strength() specifying to make an undo before making changes (for when setting bend strengths directly from the note menu)
int eof_pro_guitar_note_bend_strength_no_save(void);
	//Calls eof_pro_guitar_note_bend_strength() specifying not to make an undo before making changes
	//(for when using the keyboard shortcut to toggle bends, since an undo would already have been created)
int eof_menu_pro_guitar_remove_fingering(void);	//Clears the finger array for all selected notes
int eof_menu_remove_statuses(void);		//Clears all flags for selected notes, except notes that only have string mute status (since pro guitar fixup will re-apply it automatically)
int eof_rocksmith_convert_mute_to_palm_mute_single_note(void);
	//Removes string mute status from all selected notes and marks affected notes with palm mute status
	//If the note is a chord, it will be reduced to a single note by retaining the lowest lane in the chord (thickest string)

void eof_menu_note_cycle_selection_back(unsigned long notenum);
	//Manipulates eof_selection.multi[] to reflect the removal of notenum from the list of notes in the active track (such as when toggling off a note's only gem)
	//This allows the remaining notes to keep their selection status intact

int eof_menu_note_toggle_green(void);	//Toggles the gem on lane 1 (originally colored green)
int eof_menu_note_toggle_red(void);		//Toggles the gem on lane 2 (originally colored red)
int eof_menu_note_toggle_yellow(void);	//Toggles the gem on lane 3 (originally colored yellow)
int eof_menu_note_toggle_blue(void);	//Toggles the gem on lane 4 (originally colored blue)
int eof_menu_note_toggle_purple(void);	//Toggles the gem on lane 5 (originally colored purple)
int eof_menu_note_toggle_orange(void);	//Toggles the gem on lane 6 (originally colored orange)

int eof_menu_note_clear_green(void);	//Clears the gem on lane 1 (originally colored green)
int eof_menu_note_clear_red(void);		//Clears the gem on lane 2 (originally colored red)
int eof_menu_note_clear_yellow(void);	//Clears the gem on lane 3 (originally colored yellow)
int eof_menu_note_clear_blue(void);		//Clears the gem on lane 4 (originally colored blue)
int eof_menu_note_clear_purple(void);	//Clears the gem on lane 5 (originally colored purple)
int eof_menu_note_clear_orange(void);	//Clears the gem on lane 6 (originally colored orange)

int eof_menu_split_lyric(void);
int eof_menu_note_edit_name(void);				//Enables the name for selected notes to be altered

int eof_menu_copy_solos_track_1(void);
int eof_menu_copy_solos_track_2(void);
int eof_menu_copy_solos_track_3(void);
int eof_menu_copy_solos_track_4(void);
int eof_menu_copy_solos_track_5(void);
int eof_menu_copy_solos_track_6(void);
int eof_menu_copy_solos_track_7(void);
int eof_menu_copy_solos_track_8(void);
int eof_menu_copy_solos_track_9(void);
int eof_menu_copy_solos_track_10(void);
int eof_menu_copy_solos_track_11(void);
int eof_menu_copy_solos_track_12(void);
int eof_menu_copy_solos_track_13(void);
int eof_menu_copy_solos_track_number(EOF_SONG *sp, int sourcetrack, int desttrack);
	//Copies the solo phrases from the specified source track to the destination track

int eof_menu_copy_sp_track_1(void);
int eof_menu_copy_sp_track_2(void);
int eof_menu_copy_sp_track_3(void);
int eof_menu_copy_sp_track_4(void);
int eof_menu_copy_sp_track_5(void);
int eof_menu_copy_sp_track_6(void);
int eof_menu_copy_sp_track_7(void);
int eof_menu_copy_sp_track_8(void);
int eof_menu_copy_sp_track_9(void);
int eof_menu_copy_sp_track_10(void);
int eof_menu_copy_sp_track_11(void);
int eof_menu_copy_sp_track_12(void);
int eof_menu_copy_sp_track_13(void);
int eof_menu_copy_sp_track_number(EOF_SONG *sp, int sourcetrack, int desttrack);
	//Copies the star power phrases from the specified source track to the destination track

int eof_menu_copy_arpeggio_track_1(void);
int eof_menu_copy_arpeggio_track_2(void);
int eof_menu_copy_arpeggio_track_3(void);
int eof_menu_copy_arpeggio_track_4(void);
int eof_menu_copy_arpeggio_track_5(void);
int eof_menu_copy_arpeggio_track_6(void);
int eof_menu_copy_arpeggio_track_7(void);
int eof_menu_copy_arpeggio_track_8(void);
int eof_menu_copy_arpeggio_track_9(void);
int eof_menu_copy_arpeggio_track_10(void);
int eof_menu_copy_arpeggio_track_11(void);
int eof_menu_copy_arpeggio_track_12(void);
int eof_menu_copy_arpeggio_track_13(void);
int eof_menu_copy_arpeggio_track_number(EOF_SONG *sp, int sourcetrack, int desttrack);
	//Copies the arpeggio phrases from the specified source track to the destination track

int eof_menu_copy_trill_track_1(void);
int eof_menu_copy_trill_track_2(void);
int eof_menu_copy_trill_track_3(void);
int eof_menu_copy_trill_track_4(void);
int eof_menu_copy_trill_track_5(void);
int eof_menu_copy_trill_track_6(void);
int eof_menu_copy_trill_track_7(void);
int eof_menu_copy_trill_track_8(void);
int eof_menu_copy_trill_track_9(void);
int eof_menu_copy_trill_track_10(void);
int eof_menu_copy_trill_track_11(void);
int eof_menu_copy_trill_track_12(void);
int eof_menu_copy_trill_track_13(void);
int eof_menu_copy_trill_track_number(EOF_SONG *sp, int sourcetrack, int desttrack);
	//Copies the trill phrases from the specified source track to the destination track

int eof_menu_copy_tremolo_track_1(void);
int eof_menu_copy_tremolo_track_2(void);
int eof_menu_copy_tremolo_track_3(void);
int eof_menu_copy_tremolo_track_4(void);
int eof_menu_copy_tremolo_track_5(void);
int eof_menu_copy_tremolo_track_6(void);
int eof_menu_copy_tremolo_track_7(void);
int eof_menu_copy_tremolo_track_8(void);
int eof_menu_copy_tremolo_track_9(void);
int eof_menu_copy_tremolo_track_10(void);
int eof_menu_copy_tremolo_track_11(void);
int eof_menu_copy_tremolo_track_12(void);
int eof_menu_copy_tremolo_track_13(void);
int eof_menu_copy_tremolo_track_number(EOF_SONG *sp, int sourcetrack, int desttrack);
	//Copies the tremolo phrases from the specified source track to the destination track

float eof_menu_note_push_get_offset(void);
	//Returns a value representing the currently-set grid snap interval as a percentage of the duration of one beat
int eof_feedback_mode_update_note_selection(void);
	//If Feedback input mode is in use, eof_selection.multi[] indicates no notes are selected and there is a seek hover note, note #eof_seek_hover_note is marked as selected in the array and nonzero is returned
	//This allows the selection array to reflect the note at the seek position if no notes are explicitly selected, and after note manipulation, the seek hover note can be unselected
	//Otherwise zero is returned and the array is not altered

int eof_note_menu_read_gp_lyric_texts(void);
	//Prompts user to browse for a text file containing lyric texts formatted in the style of Guitar Pro
	//The text for lyrics in the active chart are replaced by text found in the file, starting with the first lyric

int eof_menu_note_simplify_chords(void);
	//Removes the top-most (highest lane) gem on each selected chord

int eof_menu_pro_guitar_toggle_string_mute(void);
	//For each selected note, applies string mute status to all used strings if any of the note's used strings aren't already string muted,
	//otherwise clears string mute status from all used strings in the note

int eof_menu_note_reflect(char function);
	//If function is 1, selected notes are reflected vertically so that a gem on lane 1 is moved to the highest lane, etc.
	//If function is 2, selected notes are reflected horizontally so that the first selected note is swapped with the last selected note, etc.
	//If function is 3, both the vertical and horizontal reflect operations are performed
int eof_menu_note_reflect_vertical(void);
	//Calls eof_menu_note_reflect() with the option to perform vertical reflection
int eof_menu_note_reflect_horizontal(void);
	//Calls eof_menu_note_reflect() with the option to perform horizontal reflection
int eof_menu_note_reflect_both(void);
	//Calls eof_menu_note_reflect() with the option to perform both vertical and horizontal reflection

int eof_menu_note_move_tech_note_to_previous_note_pos(void);
	//Moves each selected tech notes to the start position of the regular notes before it, if any

int eof_menu_note_move_by_grid_snap(int dir);
	//Moves each selected note forward (if dir is >= 0) or backward (if dir < 0) by one grid snap if possible
int eof_menu_note_move_back_grid_snap(void);
	//Calls eof_menu_note_move_by_grid_snap() with the option to move notes backward one grid snap
int eof_menu_note_move_forward_grid_snap(void);
	//Calls eof_menu_note_move_by_grid_snap() with the option to move notes forward one grid snap

#endif
