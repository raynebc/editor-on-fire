#include <allegro.h>
#include "main.h"
#include "beat.h"
#include "event.h"

//EOF_TEXT_EVENT eof_text_event[EOF_MAX_TEXT_EVENTS];
char eof_event_list_text[EOF_MAX_TEXT_EVENTS][256] = {{0}};
//int eof_text_events = 0;

void eof_add_text_event(EOF_SONG * sp, int beat, char * text)
{
	sp->text_event[sp->text_events] = malloc(sizeof(EOF_TEXT_EVENT));
	if(!sp->text_event[sp->text_events])
	{
		return;
	}
	ustrcpy(sp->text_event[sp->text_events]->text, text);
	sp->text_event[sp->text_events]->beat = beat;
	sp->text_events++;
}

void eof_move_text_events(EOF_SONG * sp, int beat, int offset)
{
	int i;
	
	for(i = 0; i < sp->text_events; i++)
	{
		if(sp->text_event[i]->beat >= beat)
		{
			sp->beat[sp->text_event[i]->beat]->flags = sp->beat[sp->text_event[i]->beat]->flags & EOF_BEAT_FLAG_ANCHOR;
			sp->text_event[i]->beat += offset;
			sp->beat[sp->text_event[i]->beat]->flags |= EOF_BEAT_FLAG_EVENTS;
		}
	}
}
