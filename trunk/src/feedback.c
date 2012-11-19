/*

Up/down arrow: Move forward and backward.
Left/right arrow: Change note spacement.
12345: Place notes.
Space: Play song.

I have Finnish keyboard so these might be different for you:
2 keys next to backspace: Change BPM. With shift, changes 0,01 instead of normal 1.
Key next to right shift: Change playback rate.

Shift + up: Choose notes.
Ctrl + up/down: Move selected notes forward/backward. If no notes is selected, move everything from ahead.
Ctrl + left/right: Same as above, but makes yellow blue, blue orange and so on. Kinda dangerous though, dB doesn't have Ctrl-z (EOF has?) so that might ruin everything.
Ctrl-c / Ctrl-v: copy/paste.

*/

#include <allegro.h>
#include <stdio.h>
#include "song.h"
#include "menu/edit.h"
#include "menu/song.h"
#include "menu/note.h"
#include "beat.h"
#include "main.h"
#include "dialog.h"
#include "player.h"
#include "editor.h"
#include "feedback.h"
#include "undo.h"
#include "foflc/Lyric_storage.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

int         eof_feedback_step = 0;
int         eof_feedback_selecting = 0;
int         eof_feedback_selecting_start_pos = 0;
int         eof_feedback_selecting_end_pos = 0;
EOF_NOTE *  eof_feedback_new_note = NULL;
int         eof_feedback_note[5] = {0};

int eof_feedback_any_note(void)
{
	int i;

	eof_log("eof_feedback_any_note() entered", 1);

	for(i = 0; i < 5; i++)
	{
		if(eof_feedback_note[i])
		{
			return 1;
		}
	}
	return 0;
}
