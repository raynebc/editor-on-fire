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
#include "menu/file.h"
#include "menu/edit.h"
#include "menu/song.h"
#include "menu/note.h"
#include "menu/beat.h"
#include "menu/help.h"

#ifdef ALLEGRO_WINDOWS
	char * eof_system_slash = "\\";
#else
	char * eof_system_slash = "/";
#endif

/* editable/dynamic text fields */
char eof_etext[1024] = {0};
char eof_etext2[1024] = {0};
char eof_etext3[1024] = {0};
char eof_etext4[1024] = {0};
char eof_etext5[1024] = {0};
char eof_etext6[1024] = {0};
char eof_etext7[1024] = {0};
char eof_help_text[4096] = {0};
char eof_ctext[13][1024] = {{0}};

static int eof_keyboard_shortcut = 0;
char eof_display_flats = 0;		//Used to allow eof_get_tone_name() to return note names containing flats.  By default, display as sharps instead

void eof_prepare_menus(void)
{
	eof_prepare_main_menu();
	eof_prepare_beat_menu();
	eof_prepare_file_menu();
	eof_prepare_edit_menu();
	eof_prepare_song_menu();
	eof_prepare_note_menu();
	eof_prepare_context_menu();
}

void eof_color_dialog(DIALOG * dp, int fg, int bg)
{
	int i;

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
	eof_prepare_menus();
	DIALOG_PLAYER * player = init_dialog(dp, n);
	eof_show_mouse(screen);
	clear_keybuf();
	while(1)
	{
		if(!update_dialog(player))
		{
			break;
		}
		/* special handling of the main menu */
		if(dp[0].proc == d_agup_menu_proc)
		{
			//A keypress (X) shifted left by 8 bits counts it as ALT+(X)
			/* if user wants the menu, force dialog to stay open */
			if(key[KEY_ALTGR] && !eof_keyboard_shortcut)
			{
				if(key[KEY_F])
				{
					simulate_keypress(KEY_F << 8);
					eof_keyboard_shortcut = 1;
					player->mouse_obj = 0;
				}
				else if(key[KEY_E])
				{
					simulate_keypress(KEY_E << 8);
					eof_keyboard_shortcut = 1;
					player->mouse_obj = 0;
				}
				else if(key[KEY_S])
				{
					simulate_keypress(KEY_S << 8);
					eof_keyboard_shortcut = 1;
					player->mouse_obj = 0;
				}
				else if(key[KEY_N])
				{
					simulate_keypress(KEY_N << 8);
					eof_keyboard_shortcut = 1;
					player->mouse_obj = 0;
				}
				else if(key[KEY_B])
				{
					simulate_keypress(KEY_B << 8);
					eof_keyboard_shortcut = 1;
					player->mouse_obj = 0;
				}
				else if(key[KEY_H])
				{
					simulate_keypress(KEY_H << 8);
					eof_keyboard_shortcut = 1;
					player->mouse_obj = 0;
				}
			}
			else if(key[KEY_F] || key[KEY_E] || key[KEY_S] || key[KEY_N] || key[KEY_B] || key[KEY_H])
			{
				eof_keyboard_shortcut = 1;
				player->mouse_obj = 0;
			}

			if(mouse_b & 1)
			{
				eof_keyboard_shortcut = 2;
			}
			if(eof_keyboard_shortcut == 2 && !(mouse_b & 1))
			{
				eof_keyboard_shortcut = 0;
			}

			/* if mouse isn't hovering over the menu, try and deactivate it */
			if(player->mouse_obj < 0)
			{
				/* if the user isn't about to press a menu shortcut */
				if(!KEY_EITHER_ALT && !eof_keyboard_shortcut)
				{
					break;
				}
			}
		}

		/* special handling of the song properties box */
		if(dp == eof_song_properties_dialog)
		{
			if(ustrlen(dp[14].dp) != oldlen)
			{
				object_message(&dp[14], MSG_DRAW, 0);
				oldlen = ustrlen(dp[14].dp);
			}
		}

		/* special handling of new project dialog */
		if(dp == eof_file_new_windows_dialog)
		{
			if((dp[3].flags & D_SELECTED) && ustrlen(eof_etext4) <= 0)
			{
				dp[5].flags = D_DISABLED;
				object_message(&dp[5], MSG_DRAW, 0);
				dflag = 1;
			}
			else
			{
				dp[5].flags = D_EXIT;
				if(dflag)
				{
					object_message(&dp[5], MSG_DRAW, 0);
					dflag = 0;
				}
			}
		}

		Idle(10);
	}
	ret = shutdown_dialog(player);

//   	ret = popup_dialog(dp, n);
	eof_clear_input();
   	gametime_reset();
   	eof_show_mouse(NULL);
	eof_keyboard_shortcut = 0;

   	return ret;
}

int eof_display_flats_menu(void)
{
   if(eof_display_flats)
   {
      eof_display_flats = 0;
      eof_note_menu[18].flags = 0;
   }
   else
   {
      eof_display_flats = 1;
      eof_note_menu[18].flags = D_SELECTED;
   }
   return 1;
}
