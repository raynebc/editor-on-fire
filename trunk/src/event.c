#include <allegro.h>
#include "main.h"
#include "beat.h"
#include "event.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

//EOF_TEXT_EVENT eof_text_event[EOF_MAX_TEXT_EVENTS];
char eof_event_list_text[EOF_MAX_TEXT_EVENTS][256] = {{0}};
//int eof_text_events = 0;

void eof_add_text_event(EOF_SONG * sp, int beat, char * text)
{
	eof_log("eof_add_text_event() entered", 1);

	if(sp && text && (sp->text_events < EOF_MAX_TEXT_EVENTS))
	{	//If the maximum number of text events hasn't been defined already
		sp->text_event[sp->text_events] = malloc(sizeof(EOF_TEXT_EVENT));
		if(!sp->text_event[sp->text_events])
		{
			return;
		}
		ustrcpy(sp->text_event[sp->text_events]->text, text);
		sp->text_event[sp->text_events]->beat = beat;
		sp->text_events++;
	}
}

void eof_move_text_events(EOF_SONG * sp, unsigned long beat, unsigned long offset, int dir)
{
	eof_log("eof_move_text_events() entered", 1);

	unsigned long i;

	if(!sp)
	{
		return;
	}
	for(i = 0; i < sp->text_events; i++)
	{
		if(sp->text_event[i]->beat >= beat)
		{
			if(sp->text_event[i]->beat >= sp->beats)
				continue;	//Do not allow an out of bound access
			sp->beat[sp->text_event[i]->beat]->flags &= ~(EOF_BEAT_FLAG_EVENTS);	//Clear the event flag
			if(dir < 0)
			{
				if(offset > sp->text_event[i]->beat)
					continue;	//Do not allow an underflow
				sp->text_event[i]->beat -= offset;
			}
			else
			{
				if(sp->text_event[i]->beat + offset >= sp->beats)
					continue;	//Do not allow an overflow
				sp->text_event[i]->beat += offset;
			}
			sp->beat[sp->text_event[i]->beat]->flags |= EOF_BEAT_FLAG_EVENTS;	//Set the event flag
		}
	}
}
