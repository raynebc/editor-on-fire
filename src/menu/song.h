#ifndef EOF_MENU_SONG_H
#define EOF_MENU_SONG_H

#include "../song.h"

extern MENU eof_track_selected_menu[];
extern MENU eof_catalog_menu[];
extern MENU eof_star_power_menu[];
extern MENU eof_solo_menu[];
extern MENU eof_lyric_line_menu[];
extern MENU eof_song_seek_menu[];
extern MENU eof_song_seek_bookmark_menu[];
extern MENU eof_waveform_menu[];
extern MENU eof_spectrogram_menu[];
extern MENU eof_song_proguitar_menu[];
extern MENU eof_song_menu[];

extern DIALOG eof_ini_dialog[];
extern DIALOG eof_ini_add_dialog[];
extern DIALOG eof_song_properties_dialog[];
extern DIALOG eof_catalog_entry_name_dialog[];
extern DIALOG eof_raw_midi_tracks_dialog[];

extern char eof_menu_song_difficulty_list_strings[256][4];	//Used for the "paste from difficulty" and "fret hand positions>Copy From" menu functions

extern int eof_spectrogram_settings_wsize[];	//A list of window sizes for the spectrogram

void eof_prepare_song_menu(void);

int eof_menu_catalog_show(void);
int eof_menu_catalog_add(void);
int eof_menu_catalog_delete(void);
int eof_menu_catalog_previous(void);
int eof_menu_catalog_next(void);
int eof_menu_song_catalog_edit(void);

int eof_menu_song_seek_start(void);
int eof_menu_song_seek_end(void);	//Seeks to the end of the audio
int eof_menu_song_seek_chart_end(void);	//Seeks to the end of the chart
int eof_menu_song_seek_rewind(void);
int eof_menu_song_seek_first_note(void);
int eof_menu_song_seek_last_note(void);
int eof_menu_song_seek_previous_note(void);
int eof_menu_song_seek_previous_highlighted_note(void);
int eof_menu_song_seek_next_note(void);
int eof_menu_song_seek_next_highlighted_note(void);
int eof_menu_song_seek_previous_screen(void);
int eof_menu_song_seek_next_screen(void);
int eof_menu_song_seek_bookmark_0(void);
int eof_menu_song_seek_bookmark_1(void);
int eof_menu_song_seek_bookmark_2(void);
int eof_menu_song_seek_bookmark_3(void);
int eof_menu_song_seek_bookmark_4(void);
int eof_menu_song_seek_bookmark_5(void);
int eof_menu_song_seek_bookmark_6(void);
int eof_menu_song_seek_bookmark_7(void);
int eof_menu_song_seek_bookmark_8(void);
int eof_menu_song_seek_bookmark_9(void);
void eof_seek_by_grid_snap(int dir);
	//Seeks the current position back by the current grid snap value if dir < 0, or forward if dir is >= 0
	//If grid snap is disabled, the seek position is not changed
int eof_menu_song_seek_previous_grid_snap(void);	//Seeks to the previous grid snap position
int eof_menu_song_seek_next_grid_snap(void);		//Seeks to the next grid snap position
int eof_menu_song_seek_previous_anchor(void);		//Seeks to the previous anchor
int eof_menu_song_seek_next_anchor(void);			//Seeks to the next anchor
int eof_menu_song_seek_first_beat(void);			//Seeks to the first beat
int eof_menu_song_seek_previous_beat(void);			//Seeks to the previous beat
int eof_menu_song_seek_next_beat(void);				//Seeks to the next beat
int eof_menu_song_seek_previous_measure(void);		//Seeks to the previous measure
int eof_menu_song_seek_next_measure(void);			//Seeks to the next measure
int eof_menu_song_seek_beat_measure(void);			//Seeks to the specified beat or measure
int eof_menu_song_seek_catalog_entry(void);			//Seeks to the current fret catalog entry

int eof_menu_song_file_info(void);
int eof_menu_song_ini_settings(void);
int eof_ini_dialog_add(DIALOG * d);			//Performs the INI setting add action presented in the INI settings dialog
int eof_ini_dialog_delete(DIALOG * d);		//Performs the INI setting delete action presented in the INI settings dialog
void eof_ini_delete(unsigned long index);	//Deletes the specified INI setting from the active project
int eof_menu_song_properties(void);
int eof_menu_song_test(char application);	//Launches the current chart in FoF if application is 1, otherwise launches the chart in Phase Shift
int eof_menu_song_test_fof(void);	//Calls eof_menu_song_test() to launch the chart with FoF
int eof_menu_song_test_ps(void);	//Calls eof_menu_song_test() to launch the chart with Phase Shift
int eof_menu_audio_cues(void);
int eof_set_cue_volume(void *dp3, int d2);
	//Callback function used by the volume slider GUI control in eof_audio_cues_dialog[].  dp3 is a pointer to the appropriate cue's volume variable
	//d2 is the value that was set by the slider
	//returns nonzero on error
int eof_display_flats_menu(void);	//Display the menu item to allow the user to toggle between displaying flat notes and sharp notes

int eof_menu_track_selected_1(void);
int eof_menu_track_selected_2(void);
int eof_menu_track_selected_3(void);
int eof_menu_track_selected_4(void);
int eof_menu_track_selected_5(void);
int eof_menu_track_selected_6(void);
int eof_menu_track_selected_7(void);
int eof_menu_track_selected_8(void);
int eof_menu_track_selected_9(void);
int eof_menu_track_selected_10(void);
int eof_menu_track_selected_11(void);
int eof_menu_track_selected_12(void);
int eof_menu_track_selected_13(void);
int eof_menu_track_selected_14(void);
int eof_menu_track_selected_track_number(unsigned long tracknum, int updatetitle);
	//Changes to the specified track number
	//If updatetitle is nonzero, EOF's program window title is set (it should not be updated when used to render the second piano roll)

int eof_menu_song_waveform_settings(void);
int eof_menu_song_waveform(void);	//Toggle the display of the waveform on/off, generating the waveform data if necessary
int eof_menu_song_spectrogram_settings(void);
int eof_menu_song_spectrogram(void);	//Toggle the display of the spectrogram on/off, generating the spectrogram data if necessary
int eof_menu_song_add_silence(void);

int eof_menu_catalog_edit_name(void);	//Brings up a dialog window allowing the user to define a fret catalog entry's name
int eof_menu_song_legacy_view(void);	//Toggles the view of pro guitar tracks as legacy notes

int eof_menu_song_lock_tempo_map(void);			//Toggle the setting to lock the tempo map on/off
int eof_menu_song_disable_click_drag(void);		//Toggle the setting to disable click and drag on/off

void eof_set_percussion_cue(int cue_number);	//Sets eof_sound_chosen_percussion to the cue referred by eof_audio_cues_dialog[cue_number]

int eof_menu_previous_chord_result(void);
int eof_menu_next_chord_result(void);

extern struct eof_MIDI_data_track * eof_MIDI_track_list_to_enumerate;
extern struct eof_MIDI_data_track *eof_parsed_MIDI;
extern char eof_raw_midi_track_undo_made;
int eof_menu_song_raw_MIDI_tracks(void);
char * eof_raw_midi_tracks_list(int index, int * size);
int eof_raw_midi_dialog_delete(DIALOG * d);
int eof_raw_midi_dialog_add(DIALOG * d);
	//Functions used in the dialog for managing the raw MIDI data tracks stored in the project
	//eof_raw_midi_tracks_list() is used in multiple functions, so it enumerates the contents of eof_MIDI_track_list_to_enumerate
int eof_raw_midi_track_import(DIALOG * d);
	//Finds the track that was selected in eof_raw_midi_add_track_dialog[], removes it from the eof_parsed_MIDI linked list and adds it to the active project

int eof_find_note_sequence(EOF_SONG *sp, unsigned long target_track, unsigned long target_start, unsigned long target_size, unsigned long input_track, unsigned long input_diff, unsigned long start_pos, char direction, unsigned long *hit_pos);
	//Searches for the previous/next match of the target sequence of notes (within the specified input of notes) relative to a given position of the specified track difficulty
	//If direction is negative, the first match BEFORE the given position is searched for
	//If direction is not negative, the first match AFTER the given position is searched for
	//If a match is found, this function returns nonzero, and the position of the match is returned through hit_pos
int eof_find_note_sequence_time_range(EOF_SONG *sp, unsigned long target_track, unsigned long target_diff, unsigned long target_start_pos, unsigned long target_end_pos, unsigned long input_track, unsigned long input_diff, unsigned long start_pos, char direction, unsigned long *hit_pos);
	//Identifies the target notes for a search based on a start and stop position (ie. a catalog entry) and performs the search with eof_find_note_sequence()
int eof_menu_catalog_find(char direction);
	//Performs a find previous or find next on the current catalog entry, depending on the direction variable (find previous if it is negative, otherwise find next)
int eof_menu_catalog_find_prev(void);
	//Performs a find previous operation on the current catalog entry
int eof_menu_catalog_find_next(void);
	//Performs a find next operation on the current catalog entry
int eof_menu_catalog_toggle_full_width(void);
	//Toggles whether the fret catalog will be rendered at the full width of EOF's program window
int eof_menu_song_seek_bookmark_help(int b);
	//Seeks to the specified bookmark.  Returns nonzero on success
int eof_is_number(char * buffer);
	//Returns nonzero if all characters in the specified string are numerical, or zero if otherwise or upon error

int eof_menu_song_toggle_second_piano_roll(void);
	//Toggles on/off the display of the secondary piano roll
int eof_menu_song_swap_piano_rolls(void);
	//Swaps the primary and secondary piano rolls so that the track difficulty that was displayed in the secondary roll becomes available for editing
int eof_menu_song_toggle_piano_roll_sync(void);
	//Toggles on/off the synchronization of the two piano rolls

int eof_menu_song_rocksmith_export_chord_techniques(void);	//Toggle the setting to export chords techniques by writing both a chord and a single note with appropriate techniques to XML at the same position
int eof_menu_song_rocksmith_suppress_dynamic_difficulty_warnings(void);	//Toggle the setting to suppress dynamic difficulty warnings that are generated during save

int eof_check_fret_hand_positions_option(char report, char *undo_made);
	//Checks all pro guitar tracks that have any fret hand positions defined to see if changes are needed for the track to work properly in Rocksmith
	//When a problem is identified, EOF seeks to the offending part of the chart and prompts the user whether to cancel or continue looking for errors
	//If report is nonzero, this function is more verbose and will report if there are no problems found.  It will also warn about chords too wide to fix into chord boxes
	//	nonzero will be returned if the user cancels the testing
	//If report is zero, this function doesn't display any dialogs, it only returns nonzero if problems were found
int eof_check_fret_hand_positions(void);
	//Calls eof_check_fret_hand_positions_option specifying not to report if there were no corrections needed (to be called during save logic)
int eof_check_fret_hand_positions_menu(void);
	//Calls eof_check_fret_hand_positions_option specifying to report if there were no corrections needed (to be called from menu)

int eof_run_time_range_dialog(unsigned long *start, unsigned long *end);
	//Runs the dialog to accept start and end timestamps from the user
	//The input fields are initialized to the values at *start and *end
	//eof_etext[] is used as the title for the dialog
	//Returns nonzero if user clicks OK, and returns the specified timestamps through *start and *end
	//Returns zero if user cancels, either field is empty, both fields have the same value or upon error
int eof_menu_song_export_song_preview(void);
	//Allows the user to define a portion of the chart audio to export to preview.wav and preview.ogg in the project folder

int eof_menu_song_highlight_non_grid_snapped_notes(void);
	//Toggles highlighting for all notes in the active track that aren't at any grid snap position
	//The highlighting is recreated every time the track's fixup logic runs
void eof_song_highlight_non_grid_snapped_notes(EOF_SONG *sp, unsigned long track);
	//Performs highlighting for all notes in the specified track that aren't at any grid snap position
	//If notes are properly sorted, adjacent notes at the same timestamp in different difficulties will reduce processing time by re-using the grid snap lookup results
	//If a custom grid snap is in effect, its position is also compared
int eof_menu_song_highlight_arpeggios(void);	//Enables highlighting for all notes in the active track that are within arpeggio phrases
void eof_song_highlight_arpeggios(EOF_SONG *sp, unsigned long track);
	//Performs highlighting for all notes in the specified track that are within arpeggio phrases

unsigned long eof_menu_song_compare_difficulties(unsigned long track1, unsigned diff1, unsigned long track2, unsigned diff2, int function);
	//Compares the first track difficulty against the second, provided that they are different track difficulties of the same format
	//Any normal notes in the first track difficulty that are missing or different from the second track difficulty are highlighted
	//Tech notes are processed in a way so that they are only considered a non match if they apply a technique on any gem of a note
	// that isn't applied to the same gem of a comparable note in the other track difficulty.  Bend and stop tech notes are timing
	// specific though and are required to have a comparable technote with matching timestamp and note mask in both tracks in order
	// to match.
	//Returns the number of differences that are identified, equivalent to the number of notes or tech notes that are highlighted
	//If function is nonzero, notes that exist at the same timstamp in each of the track difficulties but otherwise differ
	// do not cause the difference count to increase, allowing them to not be double-counted when this function is used afterward
	// to compare the tracks in the opposite direction
	//Returns 0 on error
int eof_menu_song_compare_piano_rolls(int function);
	//Calls eof_menu_song_compare_difficulties(), highlighting and counting differences between first primary and secondary piano rolls:
	//If function is zero, the secondary piano roll is compared against the primary piano roll
	//If function is nonzero, the secondary roll is compared against the primary, and then the primary is compared against the seconday
	// and the combined count of differences is displayed, avoiding double counting in the event that dislike notes are at matching timestamps
	// between the two track difficulties
int eof_menu_song_singly_compare_piano_rolls(void);
	//Calls eof_menu_song_compare_piano_rolls() comparing the secondary piano roll against the primary piano roll
int eof_menu_song_doubly_compare_piano_rolls(void);
	//Calls eof_menu_song_compare_piano_rolls() comparing the secondary piano roll against the primary piano roll
	// and then vice-versa

#endif
