#ifndef EOF_DIALOG_H
#define EOF_DIALOG_H

#include <allegro.h>
#include "song.h"

#ifdef ALLEGRO_WINDOWS
	#define EOF_LINUX_DISABLE 0
#else
	#define EOF_LINUX_DISABLE D_DISABLED
#endif

#define EOF_SCRATCH_MENU_SIZE 30
//A copy of each top level menu array of this number of elements will be created by eof_create_filtered_menu() to remove hidden menu items, since Allegro doesn't natively support D_HIDDEN for MENU objects

extern char eof_etext[1024];
extern char eof_etext2[1024];
extern char eof_etext3[1024];
extern char eof_etext4[1024];
extern char eof_etext5[1024];
extern char eof_etext6[1024];
extern char eof_etext7[1024];
extern char eof_etext8[1024];
extern char eof_etext9[1024];
extern char eof_etext10[1024];
extern char *eof_help_text;
extern char eof_ctext[8][1024];
extern char eof_menu_track_names[EOF_TRACKS_MAX][EOF_TRACK_NAME_SIZE];

extern int eof_close_menu;	//Set to nonzero to signal to eof_popup_dialog() that the menus should be closed

//These functions are defined by source files in the /menu folder
char * eof_input_list(int index, int * size);		//Dialog logic to display the usable input methods in a list box
char * eof_ini_list(int index, int * size);			//Dialog logic to display the chart's INI definitions in a list box
char * eof_colors_list(int index, int * size);		//Dialog logic to display the usable color sets in the Preferences dialog
int eof_ogg_settings(void);					//Launches eof_ogg_settings_dialog, allowing the user to specify an OGG encoding quality setting, returns 0 upon user cancellation
										//eof_ogg_setting is set to the user selected value, which indexes into the appropriate value in eof_ogg_quality[]
int eof_new_lyric_dialog(void);		//Launches the input box to accept the text for a newly created lyric
int eof_edit_lyric_dialog(void);	//Performs the Edit Lyric action presented in the note menu


void eof_prepare_menus(void);	//Configures menus based on conditions within EOF, such as to enable/disable menu items
int eof_popup_dialog(DIALOG * dp, int n);
	//Opens the dialog menu, giving focus to object #n and returns the activated item number
	//If the user cancels with the escape key, the return value may indicate whatever dialog item had focus as of when escape was pressed
void eof_color_dialog(DIALOG * dp, int fg, int bg);	//Applies the global foreground and background colors to the dialog

int eof_unused_menu_function(void);	//A function that can be put into a dialog menu when the function will not be used (ie. copy from submenus that are pro guitar specific)

int eof_create_filtered_menu(MENU *input_menu, MENU *output_menu, unsigned menu_size);
	//A function that copies all non-hidden items (containing the D_HIDDEN flag) from the input menu into the output menu
	//menu_size defines the number of elements the output menu is capable of storing
	//returns zero on error

#endif
