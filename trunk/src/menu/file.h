#ifndef EOF_MENU_FILE_H
#define EOF_MENU_FILE_H

extern MENU eof_file_menu[];

extern DIALOG eof_settings_dialog[];
extern DIALOG eof_preferences_dialog[];
extern DIALOG eof_display_dialog[];
extern DIALOG eof_ogg_settings_dialog[];
extern DIALOG eof_controller_settings_dialog[];
extern DIALOG eof_file_new_dialog[];
extern DIALOG eof_file_new_windows_dialog[];

extern DIALOG eof_lyric_detections_dialog[];	//The dialog used to prompt the user to select a MIDI track to import from
extern struct Lyric_Format *lyricdetectionlist;	//Dialog windows cannot be passed local variables, requiring the use of this global variable

void eof_prepare_file_menu(void);

int eof_menu_file_new_wizard(void);
int eof_menu_file_load(void);
int eof_menu_file_save(void);
int eof_menu_file_quick_save(void);
int eof_menu_file_lyrics_import(void);
int eof_menu_file_save_as(void);
int eof_menu_file_load_ogg(void);
int eof_menu_file_midi_import(void);
int eof_menu_file_settings(void);
int eof_menu_file_preferences(void);
int eof_menu_file_display(void);
int eof_menu_file_controllers(void);
int eof_menu_file_song_folder(void);
int eof_menu_file_link(void);
int eof_menu_file_exit(void);

char * eof_ogg_list(int index, int * size);
char * eof_guitar_list(int index, int * size);
char * eof_drum_list(int index, int * size);
char * eof_display_list(int index, int * size);

int eof_controller_settings_guitar(DIALOG * d);
int eof_controller_settings_drums(DIALOG * d);
int eof_guitar_controller_redefine(DIALOG * d);
int eof_drum_controller_redefine(DIALOG * d);

char *eof_lyric_detections_list_all(int index, int * size);
	//The dialog procedure that returns the strings to display in the list box

void eof_lyric_import_prompt(int *selectedformat, char **selectedtrack);
	//Requires storage for the selected format, track name to be passed back
	//The detection list created by DetectLyricFormat() is expected to be stored in the global lyricdetectionlist variable
	//This function displays the dialog for selecting one of of multiple lyric imports for a file
	//*selectedformat is set to 0 if a selection is not made

struct Lyric_Format *GetDetectionNumber(struct Lyric_Format *detectionlist, unsigned long number);
	//Returns detection #number from the detection linked list

#endif
