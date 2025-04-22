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
#include "track.h"

#ifdef USEMEMWATCH
#include "../memwatch.h"
#endif

MENU eof_main_menu[] =
{
	{"&File", NULL, eof_file_menu, 0, NULL},
	{"&Edit", NULL, eof_edit_menu, D_DISABLED, NULL},
	{"&Song", NULL, eof_song_menu, D_DISABLED, NULL},
	{"&Track",NULL, eof_track_menu, D_DISABLED, NULL},
	{"&Note", NULL, eof_note_menu, D_DISABLED, NULL},
	{"&Beat", NULL, eof_beat_menu, D_DISABLED, NULL},
	{"&Help", NULL, eof_help_menu, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

void eof_prepare_main_menu(void)
{
	eof_main_menu[4].flags = D_DISABLED; //The note menu is always disabled unless at least one note is selected

	if(eof_song && eof_song_loaded)
	{
		eof_main_menu[1].flags = 0; // Edit
		eof_main_menu[2].flags = 0; // Song
		eof_main_menu[3].flags = 0; // Track
		eof_main_menu[5].flags = 0; // Beat

		/* disable Note menu when no notes are selected */
		if(eof_count_selected_and_unselected_notes(NULL) || ((eof_input_mode == EOF_INPUT_FEEDBACK) && (eof_seek_hover_note >= 0)) || eof_count_notes_starting_in_time_range(eof_song, eof_selected_track, eof_note_type, eof_song->tags->start_point, eof_song->tags->end_point))
		{	//If notes are selected, or the seek position is at a note position when Feedback input mode is in use, or one or more notes exist between the start and end points in the active track difficulty
			eof_main_menu[4].flags = 0; // Note
		}
	}
	else
	{
		eof_main_menu[1].flags = D_DISABLED; // Edit
		eof_main_menu[2].flags = D_DISABLED; // Song
		eof_main_menu[5].flags = D_DISABLED; // Beat
	}
}
