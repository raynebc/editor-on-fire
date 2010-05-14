#ifndef EOF_DIALOG_H
#define EOF_DIALOG_H

#include <allegro.h>

#ifdef ALLEGRO_WINDOWS
	#define EOF_LINUX_DISABLE 0
#else
	#define EOF_LINUX_DISABLE D_DISABLED
#endif

extern char eof_etext[1024];
extern char eof_etext2[1024];
extern char eof_etext3[1024];
extern char eof_etext4[1024];
extern char eof_etext5[1024];
extern char eof_etext6[1024];
extern char eof_etext7[1024];
extern char eof_help_text[4096];
extern char eof_ctext[13][1024];
extern char eof_display_flats;

char * eof_input_list(int index, int * size);
char * eof_ini_list(int index, int * size);
char * eof_events_list(int index, int * size);
char * eof_events_list_all(int index, int * size);

void eof_prepare_menus(void);
int eof_popup_dialog(DIALOG * dp, int n);
void eof_setup_menus(void);
void eof_color_dialog(DIALOG * dp, int fg, int bg);
int eof_display_flats_menu(void);	//Display the menu item to allow the user to toggle between displaying flat notes and sharp notes
//void eof_fix_menu_undo(void);

int eof_menu_cancel(void);

int eof_ogg_settings(void);

int eof_menu_edit_cut(int anchor, int option, float offset);
int eof_menu_edit_cut_paste(int anchor, int option, float offset);

int eof_ini_dialog_add(DIALOG * d);
int eof_ini_dialog_delete(DIALOG * d);

int eof_events_dialog_add(DIALOG * d);
int eof_events_dialog_edit(DIALOG * d);
int eof_events_dialog_delete(DIALOG * d);

int eof_new_lyric_dialog(void);
int eof_edit_lyric_dialog(void);

#endif
