#ifndef EOF_MENU_HELP_H
#define EOF_MENU_HELP_H

extern MENU eof_help_menu[];

extern DIALOG eof_help_dialog[];

int eof_menu_help_keys(void);
int eof_menu_help_manual(void);
int eof_menu_help_tutorial(void);
int eof_menu_help_vocals_tutorial(void);
int eof_menu_help_pro_guitar_tutorial(void);
int eof_menu_help_immerrock_tutorial(void);
int eof_menu_help_about(void);

int eof_reset_display(void);				//Rebuilds the program window with the current window size
int eof_reset_audio(void);				//Re-initializes the sound handling

#endif
