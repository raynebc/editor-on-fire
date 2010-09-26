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
extern MENU eof_song_menu[];

extern DIALOG eof_ini_dialog[];
extern DIALOG eof_ini_add_dialog[];
extern DIALOG eof_song_properties_dialog[];

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

int eof_menu_song_file_info(void);
int eof_menu_song_ini_settings(void);
int eof_menu_song_properties(void);
int eof_menu_song_test(void);
int eof_menu_audio_cues(void);
int eof_set_cue_volume(void *dp3, int d2);
	//Callback function used by the volume slider GUI control in eof_audio_cues_dialog[].  dp3 is a pointer to the appropriate cue's volume variable
	//d2 is the value that was set by the slider
	//returns nonzero on error

int eof_menu_track_selected_guitar(void);
int eof_menu_track_selected_bass(void);
int eof_menu_track_selected_guitar_coop(void);
int eof_menu_track_selected_rhythm(void);
int eof_menu_track_selected_drum(void);
int eof_menu_track_selected_vocals(void);
int eof_menu_track_selected_track_number(int tracknum);	//Changes to the specified track number

void eof_menu_song_waveform(void);
	//Toggle the display of the waveform on/off, generating the waveform data if necessary

#endif
