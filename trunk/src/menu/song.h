#ifndef EOF_MENU_SONG_H
#define EOF_MENU_SONG_H

extern MENU eof_track_selected_menu[];
extern MENU eof_track_menu[];
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

void eof_prepare_song_menu(void);

int eof_menu_catalog_show(void);
int eof_menu_catalog_add(void);
int eof_menu_catalog_delete(void);
int eof_menu_catalog_previous(void);
int eof_menu_catalog_next(void);

int eof_menu_song_seek_start(void);
int eof_menu_song_seek_end(void);
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

void eof_set_percussion_cue(int cue_number);	//Sets eof_sound_chosen_percussion to the cue referred by eof_audio_cues_dialog[cue_number]

int eof_menu_previous_chord_result(void);
int eof_menu_next_chord_result(void);

extern struct eof_MIDI_data_track * eof_MIDI_track_list_to_enumerate;
int eof_menu_song_raw_MIDI_tracks(void);
char * eof_raw_midi_tracks_list(int index, int * size);
int eof_raw_midi_dialog_delete(DIALOG * d);
int eof_raw_midi_dialog_add(DIALOG * d);
	//Functions used in the dialog for managing the raw MIDI data tracks stored in the project
	//eof_raw_midi_tracks_list() is used in multiple functions, so it enumerates the contents of eof_MIDI_track_list_to_enumerate

#endif
