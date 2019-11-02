#include <allegro.h>
#include "agup/agup.h"
#include "modules/wfsel.h"
#include "modules/gametime.h"
#include "modules/g-idle.h"
#include "foflc/Lyric_storage.h"
#include "foflc/Midi_parse.h"
#include "menu/main.h"
#include "menu/file.h"
#include "menu/edit.h"
#include "menu/song.h"
#include "menu/track.h"
#include "menu/note.h"
#include "menu/beat.h"
#include "menu/help.h"
#include "menu/context.h"
#include "lc_import.h"
#include "main.h"
#include "player.h"
#include "ini.h"
#include "editor.h"
#include "window.h"
#include "dialog.h"
#include "beat.h"
#include "event.h"
#include "midi.h"
#include "undo.h"
#include "mix.h"
#include "tuning.h"
#include "dialog/main.h"
#include "menu/file.h"
#include "menu/edit.h"
#include "menu/song.h"
#include "menu/note.h"
#include "menu/beat.h"
#include "menu/help.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

#ifdef ALLEGRO_WINDOWS
	char * eof_system_slash = "\\";
#else
	char * eof_system_slash = "/";
#endif

#define EOF_MSG_BUTTONFOCUS D_USER
//Allegro's GUI system reserves bit flags 0 through 6 for its own use, the others are available for
//program use.  Define a bitflag for use to filter dialog focus changing events that use the mouse
//click from those that don't

/* editable/dynamic text fields */
char eof_etext[1024] = {0};
char eof_etext2[1024] = {0};
char eof_etext3[1024] = {0};
char eof_etext4[1024] = {0};
char eof_etext5[1024] = {0};
char eof_etext6[1024] = {0};
char eof_etext7[1024] = {0};
char eof_etext8[1024] = {0};
char eof_etext9[1024] = {0};
char eof_etext10[1024] = {0};
char *eof_help_text = NULL;
char eof_ctext[8][1024] = {{0}};

static int eof_keyboard_shortcut = 0;
static int eof_main_menu_activated = 0;
int eof_close_menu = 0;	//A variable that can be set to nonzero to try to force the dialog logic closed, as some menu functions seem to get stuck and stop processing input until an Escape keypress or mouse click

char eof_menu_track_names[EOF_TRACKS_MAX][EOF_TRACK_NAME_SIZE] = {{0}};
	//A list of the names of each track, built by eof_prepare_menus()

void eof_prepare_menus(void)
{
	unsigned long i;

	eof_log("eof_prepare_menus() entered", 2);

	if(eof_song && eof_song_loaded)
	{	//If a chart is loaded
		for(i = 0; i < EOF_TRACKS_MAX - 1; i++)
		{	//For each track supported by EOF
			if((i + 1 < EOF_TRACKS_MAX) && (i + 1 < eof_song->tracks) && (eof_song->track[i + 1] != NULL))
			{	//If the track exists, copy its name into the track name string representing this track
				(void) ustrcpy(eof_menu_track_names[i], eof_song->track[i + 1]->name);
					//Copy the track name to the menu string
			}
			else
			{	//Write a blank string for the track name
				(void) ustrcpy(eof_menu_track_names[i],"");
			}
		}
	}

	eof_prepare_main_menu();
	eof_prepare_beat_menu();
	eof_prepare_file_menu();
	eof_prepare_edit_menu();
	eof_prepare_song_menu();
	eof_prepare_track_menu();
	eof_prepare_note_menu();
	eof_prepare_context_menu();
}

void eof_color_dialog(DIALOG * dp, int fg, int bg)
{
	int i;

	eof_log("eof_color_dialog() entered", 2);

	if(dp == NULL)
		return;	//Invalid DIALOG pointer

	for(i = 0; dp[i].proc != NULL; i++)
	{
		dp[i].fg = fg;
		dp[i].bg = bg;
	}
}

int eof_popup_dialog(DIALOG * dp, int n)
{
	int ret;
	int oldlen = 0;
	int dflag = 0;
	DIALOG_PLAYER * player;

	eof_log("eof_popup_dialog() entered", 2);

	if(!dp)	//If this pointer is NULL for any reason
		return D_O_K;

	eof_close_menu = 0;
	eof_prepare_menus();
	dp[n].flags |= EOF_MSG_BUTTONFOCUS;		//Set the button focus flag of the object that needs to have the default focus
	player = init_dialog(dp, n);
	eof_show_mouse(screen);
	clear_keybuf();
	while(1)
	{
		if((eof_has_focus == 2) && (dp == eof_main_dialog))
		{	//If EOF just switched back into focus and no dialogs except the main menu were running
			eof_has_focus = 1;
			break;	//Don't allow EOF to get stuck in this loop just because ALT+Tab was used
		}

		/* Read the keyboard input and simulate the keypresses so the dialog
		 * player can pick up keyboard input after we intercepted it. This lets
		 * us be aware of any keyboard input so we can react accordingly. */
		eof_read_keyboard_input(0);	//Don't drop ASCII values for number pad key presses
		if(eof_key_pressed)
		{
			if(eof_key_code == KEY_ESC)
			{	//Escape causes the menu to close immediately
				eof_close_menu = 1;	//Close a nested parent call to eof_popup_dialog, if any
				break;
			}
			if(eof_key_char)
			{
				simulate_ukeypress(eof_key_uchar, eof_key_code);
			}
			else if(eof_key_code)
			{
				simulate_ukeypress(0, eof_key_code);
			}
		}
		if(!update_dialog(player))
		{
			break;
		}
		if(eof_close_menu)
		{	//If another function (ie. a dialog) signaled to close the menu, do so immediately
			eof_close_menu = 0;
			break;
		}
		/* special handling of the main menu */
		if(dp[0].proc == d_agup_menu_proc)
		{
			/* user has opened the menu with a shortcut key */
			if((eof_key_char == 'f') || (eof_key_char == 'e') || (eof_key_char == 's') || (eof_key_char == 't') || (eof_key_char == 'n') || (eof_key_char == 'b') || (eof_key_char == 'h'))
			{
				eof_keyboard_shortcut = 1;
				eof_main_menu_activated = 1;
				player->mouse_obj = 0;
			}
			else if(!eof_keyboard_shortcut && ((eof_key_code == KEY_M) || (eof_key_code == KEY_V) || (eof_key_code == KEY_C) || (eof_key_code == KEY_D) || (eof_key_code == KEY_PLUS_PAD) || (eof_key_code == KEY_MINUS_PAD) || (eof_key_code == KEY_R) || (eof_key_code == KEY_Y) || (eof_key_code == KEY_G) || (eof_key_code == KEY_P) || (eof_key_code == KEY_I) || (eof_key_code == KEY_PGDN) || (eof_key_code == KEY_PGUP)))
			{	///Experimental:  User pressed a different ALT+key combination, seems like eof_key_code must be tested instead of eof_key_char,
				/// possibly because only the characters associated with the main menu accelerators (ie. &F for the File menu) are sent to this
				/// dialog for processing, but scan codes for other keys are still sent
				///If a menu wasn't already opened, allow these key codes to exit the main menu
				///This is necessary in order for the key to be tested for an ALT+[key_code] shortcut outside of this function
				player->mouse_obj = 0;	///Not needed, but GDB isn't honoring the break point on the break statement
				break;	//Escape from the menu system so the other key shortcut handling logic can process this
			}
			else if(!eof_keyboard_shortcut && (eof_key_code == KEY_B) && (KEY_EITHER_CTRL))
			{	//Allow CTRL+ALT+B to break from the menu system
				player->mouse_obj = 0;
				break;	//Escape from the menu system so the other key shortcut handling logic can process this
			}
			else if(eof_mouse_z != mouse_z)
			{	///Also allow ALT+scroll wheel to exit the main menu
				eof_mickey_z = eof_mouse_z - mouse_z;	//Update eof_mickey_z so the scrolling can immediately be detected by eof_editor_logic_common()
				player->mouse_obj = 0;	///Not needed, but GDB isn't honoring the break point on the break statement
				break;	//Escape from the menu system so the other key shortcut handling logic can process this
			}

			/* detect if menu was activated with a click */
			if((mouse_b & 1) || (mouse_b & 2))
			{
				eof_keyboard_shortcut = 2;
				eof_main_menu_activated = 1;
			}

			/* allow menu to be closed if mouse button released after it was
			 * opened with a click */
			if((eof_keyboard_shortcut == 2) && !(mouse_b & 1) && !(mouse_b & 2))
			{
				if(player->mouse_obj < 0)
				{	//If the mouse isn't hovering over the menu anymore
					eof_main_menu_activated = 0;
					eof_keyboard_shortcut = 0;
				}
			}

			if(!KEY_EITHER_ALT && !eof_main_menu_activated)
			{	//If ALT isn't held and the menu hasn't been activated
				break;	//Escape from the menu system
			}
		}

		/* special handling of the song properties box */
		if(dp == eof_song_properties_dialog)
		{
			if(ustrlen(eof_song_properties_dialog[20].dp) != oldlen)
			{	//If the loading text field was altered
				(void) object_message(&eof_song_properties_dialog[22], MSG_DRAW, 0);	//Redraw the loading text preview
				oldlen = ustrlen(eof_song_properties_dialog[22].dp);
			}
		}

		/* special handling of new project dialog */
		if(dp == eof_file_new_windows_dialog)
		{
			if((eof_file_new_windows_dialog[3].flags & D_SELECTED) && (ustrlen(eof_etext4) <= 0))
			{
				eof_file_new_windows_dialog[5].flags = D_DISABLED;
				(void) object_message(&eof_file_new_windows_dialog[5], MSG_DRAW, 0);
				dflag = 1;
			}
			else
			{
				eof_file_new_windows_dialog[5].flags = D_EXIT;
				if(dflag)
				{
					(void) object_message(&eof_file_new_windows_dialog[5], MSG_DRAW, 0);
					dflag = 0;
				}
			}
		}
		/* clear keyboard data so it doesn't show up in the next run of the
		 * loop */
		if(eof_key_pressed)
		{
			eof_use_key();
		}

		Idle(10);
	}
	ret = shutdown_dialog(player);

//	ret = popup_dialog(dp, n);
	if(!KEY_EITHER_ALT)
	{	//If ALT is still being held
		eof_clear_input();	//One of the monitored key scan codes must have triggered this, allow the key code to survive to be detected in the editor logic
	}

	gametime_reset();
	eof_show_mouse(NULL);
	eof_keyboard_shortcut = 0;
	eof_main_menu_activated = 0;
	eof_close_button_clicked = 0;	//Clear any tracking of the close button having been clicked while a dialog is open

	return ret;
}

int eof_unused_menu_function(void)
{
	return 1;
}
