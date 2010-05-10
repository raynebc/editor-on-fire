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

#endif
