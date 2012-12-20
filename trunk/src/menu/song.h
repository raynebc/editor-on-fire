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
extern MENU eof_song_proguitar_menu[];
extern MENU eof_song_menu[];

extern DIALOG eof_ini_dialog[];
extern DIALOG eof_ini_add_dialog[];
extern DIALOG eof_song_properties_dialog[];
extern DIALOG eof_catalog_entry_name_dialog[];
extern DIALOG eof_pro_guitar_tuning_dialog[];
extern DIALOG eof_note_set_num_frets_strings_dialog[];
extern DIALOG eof_raw_midi_tracks_dialog[];
extern DIALOG eof_fret_hand_position_list_dialog[];

extern char eof_fret_hand_position_list_dialog_undo_made;	//Used to track an undo state having been made in the fret hand positions dialog
extern char eof_fret_hand_position_list_dialog_title_string[30];	//This will be rebuilt by the list box count function to display the number of positions present

extern char eof_menu_song_difficulty_list_strings[256][4];	//Used for the "paste from difficulty" and "fret hand positions>Copy From" menu functions

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
int eof_menu_song_seek_next_note(void);
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
int eof_menu_song_seek_previous_beat(void);			//Seeks to the previous beat
int eof_menu_song_seek_next_beat(void);				//Seeks to the next beat
int eof_menu_song_seek_previous_measure(void);		//Seeks to the previous measure
int eof_menu_song_seek_next_measure(void);			//Seeks to the next measure
int eof_menu_song_seek_beat_measure(void);			//Seeks to the specified beat or measure
int eof_menu_song_seek_catalog_entry(void);			//Seeks to the current fret catalog entry

int eof_menu_song_file_info(void);
int eof_menu_song_ini_settings(void);
int eof_menu_song_properties(void);
int eof_menu_song_test(char application);	//Launches the current chart in FoF if application is 1, otherwise launches the chart in Phase Shift
int eof_menu_song_test_fof(void);	//Calls eof_menu_song_test() to launch the chart with FoF
int eof_menu_song_test_ps(void);	//Calls eof_menu_song_test() to launch the chart with Phase Shift
int eof_menu_audio_cues(void);
int eof_set_cue_volume(void *dp3, int d2);
	//Callback function used by the volume slider GUI control in eof_audio_cues_dialog[].  dp3 is a pointer to the appropriate cue's volume variable
	//d2 is the value that was set by the slider
	//returns nonzero on error

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
int eof_menu_track_selected_track_number(int tracknum);	//Changes to the specified track number

int eof_menu_song_waveform_settings(void);
int eof_menu_song_waveform(void);	//Toggle the display of the waveform on/off, generating the waveform data if necessary
int eof_menu_song_add_silence(void);

int eof_menu_song_open_bass(void);		//Toggle the ability to use a sixth lane in PART BASS on/off
int eof_menu_catalog_edit_name(void);	//Brings up a dialog window allowing the user to define a fret catalog entry's name
int eof_menu_song_legacy_view(void);	//Toggles the view of pro guitar tracks as legacy notes
int eof_song_track_difficulty_dialog(void);	//Allow the active track's difficulty to be set to a value from 0 through 6
int eof_menu_song_track_tuning(void);	//Allows the active track's tuning to be defined
int eof_edit_tuning_proc(int msg, DIALOG *d, int c);	//This is a modification of eof_verified_edit_proc() allowing the note names and tuning name to be redrawn when a tuning field is altered
int eof_menu_set_num_frets_strings(void);		//Allows the track's number of frets (pro guitar/bass) and strings (pro bass only) to be defined

int eof_menu_song_five_lane_drums(void);		//Toggle the ability to use a fifth lane in PART DRUM on/off
int eof_menu_song_lock_tempo_map(void);			//Toggle the setting to lock the tempo map on/off
int eof_menu_song_disable_double_bass_drums(void);	//Toggle the setting to disable expert+ bass drum gem export on/off

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
	//Toggles whether the fret catalog will be rendered the full width of EOF's program window
int eof_menu_song_seek_bookmark_help(int b);
	//Seeks to the specified bookmark.  Returns nonzero on success
int eof_is_number(char * buffer);
	//Returns nonzero if all characters in the specified string are numerical.
int eof_pro_guitar_set_fret_hand_position(void);
	//Allows the user to define a fret hand position for the active difficulty
int eof_menu_song_fret_hand_positions(void);
	//Displays the fret hand positions defined for the active track difficulty, allowing them to be deleted
int eof_song_proguitar_rename_track(void);
	//Allows the user to define an alternate name for the active track
int eof_song_proguitar_toggle_difficulty_limit(void);
	//Toggles the 5 difficulty limit on/off.  If difficulties >= 4 are populated, appropriate warnings are given to the user regarding Rock Band vs. Rocksmith standards compatibility
int eof_song_proguitar_insert_difficulty(void);
	//If the active pro guitar/bass track has fewer than 255 difficulties, prompts user to insert a new difficulty above or below the active difficulty
	//If the new difficulty has a populated difficulty above or below it, the user is prompted whether to import either (calling paste from logic)
int eof_song_proguitar_delete_difficulty(void);
	//Deletes the active difficulty's content, decrementing all higher difficulty's content accordingly
char * eof_menu_song_fret_hand_positions_copy_from_list(int index, int * size);
	//The list dialog function for eof_menu_song_fret_hand_positions_copy_from()
int eof_menu_song_fret_hand_positions_copy_from(void);
	//Allows the user to copy fret hand positions defined in another difficulty into the current track difficulty

#endif
