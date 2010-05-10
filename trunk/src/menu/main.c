#include <allegro.h>
#include "../agup/agup.h"
#include "../main.h"
#include "main.h"
#include "file.h"
#include "edit.h"
#include "song.h"
#include "note.h"
#include "beat.h"
#include "help.h"

MENU eof_main_menu[] =
{
    {"&File", NULL, eof_file_menu, 0, NULL},
    {"&Edit", NULL, eof_edit_menu, D_DISABLED, NULL},
    {"&Song", NULL, eof_song_menu, D_DISABLED, NULL},
    {"&Note", NULL, eof_note_menu, D_DISABLED, NULL},
    {"&Beat", NULL, eof_beat_menu, D_DISABLED, NULL},
    {"&Help", NULL, eof_help_menu, 0, NULL},
    {NULL, NULL, NULL, 0, NULL}
};

DIALOG eof_main_dialog[] =
{
   /* (proc)         (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)           (dp2) (dp3) */
   { d_agup_menu_proc, 0, 0, 1024, 8, 0, 0, 0, 0, 0, 0, eof_main_menu, NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

void eof_prepare_main_menu(void)
{
	if(eof_song && eof_song_loaded)
	{
		eof_main_menu[1].flags = 0; // Edit
		eof_main_menu[2].flags = 0; // Song
		eof_main_menu[3].flags = 0; // Note
		eof_main_menu[4].flags = 0; // Beat
		
		/* disable Note menu when no notes are selected */
		if(eof_count_selected_notes(NULL, 1))
		{
			eof_main_menu[3].flags = 0; // Note
		}
		else
		{
			eof_main_menu[3].flags = D_DISABLED; // Note
		}
	}
	else
	{
		eof_main_menu[1].flags = D_DISABLED; // Edit
		eof_main_menu[2].flags = D_DISABLED; // Song
		eof_main_menu[3].flags = D_DISABLED; // Note
		eof_main_menu[4].flags = D_DISABLED; // Beat
	}
}

int eof_menu_cancel(void)
{
	return 1;
}
