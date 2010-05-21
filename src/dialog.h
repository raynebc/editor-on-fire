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


//These functions are defined by source files in the /menu folder
char * eof_input_list(int index, int * size);		//Dialog logic to display the usable input methods in a list box
char * eof_ini_list(int index, int * size);			//Dialog logic to display the chart's INI definitions in a list box
char * eof_events_list(int index, int * size);		//Dialog logic to display the chart's text events in the "Events" list box
char * eof_events_list_all(int index, int * size);	//Dialog logic to display the chart's text events in the "All Events" list box
int eof_ogg_settings(void);		//Launches the MP3->OGG conversion dialog window
int eof_menu_edit_cut(int anchor, int option, float offset);		//Performs non "Auto adjust" logic for when anchors are manipulated?
int eof_menu_edit_cut_paste(int anchor, int option, float offset);	//Performs "Auto adjust" logic for when anchors are manipulated?
int eof_ini_dialog_add(DIALOG * d);		//Performs the INI setting add action presented in the INI settings dialog
int eof_ini_dialog_delete(DIALOG * d);	//Performs the INI setting delete action presented in the INI settings dialog
int eof_events_dialog_add(DIALOG * d);	//Performs the Text event add action presented in the Events dialog
int eof_events_dialog_edit(DIALOG * d);	//Performs the Text event edit action presented in the Events dialog
int eof_events_dialog_delete(DIALOG * d);	//Performs the Text event delete action presented in the Events dialog
int eof_new_lyric_dialog(void);		//Launches the input box to accept the text for a newly created lyric
int eof_edit_lyric_dialog(void);	//Performs the Edit Lyric action presented in the note menu


void eof_prepare_menus(void);	//Configures menus based on conditions within EOF, such as to enable/disable menu items
int eof_popup_dialog(DIALOG * dp, int n);	//Opens the dialog menu and returns the activated item number
//void eof_setup_menus(void);	//Unused
void eof_color_dialog(DIALOG * dp, int fg, int bg);	//Applies the global foreground and background colors to the dialog
int eof_display_flats_menu(void);	//Display the menu item to allow the user to toggle between displaying flat notes and sharp notes
//void eof_fix_menu_undo(void);
//int eof_menu_cancel(void);	//Unused


#endif
