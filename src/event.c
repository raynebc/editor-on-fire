#include <allegro.h>
#include "main.h"
#include "beat.h"
#include "event.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

char eof_event_list_text[EOF_MAX_TEXT_EVENTS][256] = {{0}};

EOF_TEXT_EVENT * eof_song_add_text_event(EOF_SONG * sp, unsigned long beat, char * text, unsigned long track, unsigned long flags, char is_temporary)
{
// 	eof_log("eof_song_add_text_event() entered");

	if(sp && text && (sp->text_events < EOF_MAX_TEXT_EVENTS) && (beat < sp->beats))
	{	//If the maximum number of text events hasn't already been defined, and the specified beat number is valid
		sp->text_event[sp->text_events] = malloc(sizeof(EOF_TEXT_EVENT));
		if(sp->text_event[sp->text_events])
		{
			sp->text_event[sp->text_events]->text[0] = '\0';	//Eliminate false positive in Splint
			(void) ustrcpy(sp->text_event[sp->text_events]->text, text);
			sp->text_event[sp->text_events]->beat = beat;
			if(track >= sp->tracks)
			{	//If this is an invalid track
				track = 0;	//Make this a global text event
			}
			sp->text_event[sp->text_events]->track = track;
			sp->text_event[sp->text_events]->flags = flags;
			sp->text_event[sp->text_events]->is_temporary = is_temporary;
			sp->beat[beat]->flags |= EOF_BEAT_FLAG_EVENTS;	//Set the events flag for the beat
			sp->text_event[sp->text_events]->index = 0;
			sp->text_events++;
			return sp->text_event[sp->text_events-1];	//Return successfully created text event
		}
	}
	return NULL;	//Return failure
}

void eof_move_text_events(EOF_SONG * sp, unsigned long beat, unsigned long offset, int dir)
{
	unsigned long i;

	eof_log("eof_move_text_events() entered", 1);

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

void eof_song_delete_text_event(EOF_SONG * sp, unsigned long event)
{
	unsigned long i;

 	eof_log("eof_song_delete_text_event() entered", 1);

	if(sp && (event <= sp->text_events))
	{
		free(sp->text_event[event]);
		for(i = event; i < sp->text_events - 1; i++)
		{
			sp->text_event[i] = sp->text_event[i + 1];
		}
		sp->text_events--;
		eof_cleanup_beat_flags(sp);	//Rebuild event flags for all beats to ensure they're valid
	}
}

int eof_song_resize_text_events(EOF_SONG * sp, unsigned long events)
{
	unsigned long i;
	unsigned long oldevents;

	if(!sp)
	{
		return 0;
	}
	oldevents = sp->text_events;
	if(events > oldevents)
	{
		for(i = oldevents; i < events; i++)
		{
			if(!eof_song_add_text_event(sp, 0, "", 0, 0, 0))
			{
				return 0;	//Return failure
			}
		}
	}
	else if(events < oldevents)
	{
		for(i = events; i < oldevents; i++)
		{
			free(sp->text_event[i]);
			sp->text_events--;
		}
	}
	return 1;	//Return succes
}

void eof_sort_events(EOF_SONG * sp)
{
	unsigned long ctr;

 	eof_log("eof_sort_events() entered", 1);

	if(sp)
	{
		for(ctr = 0; ctr < sp->text_events; ctr++)
		{	//For each text event in the project
			sp->text_event[ctr]->index = ctr;	//Store the native index into the event to prevent qsort() from corrupting the order of events that otherwise have matching sort criteria
		}
		qsort(sp->text_event, (size_t)sp->text_events, sizeof(EOF_TEXT_EVENT *), eof_song_qsort_events);
	}
}

int eof_song_qsort_events(const void * e1, const void * e2)
{
	EOF_TEXT_EVENT ** thing1 = (EOF_TEXT_EVENT **)e1;
	EOF_TEXT_EVENT ** thing2 = (EOF_TEXT_EVENT **)e2;

	if((*thing1)->beat < (*thing2)->beat)
	{
		return -1;
	}
	else if((*thing1)->beat == (*thing2)->beat)
	{
		if((*thing1)->index < ((*thing2)->index))
			return -1;	//If the first event's index is lower, it will sort earlier

		return 1;
	}

	return 1;
}

char eof_song_contains_event(EOF_SONG *sp, const char *text, unsigned long track, unsigned long flags, unsigned char track_specific)
{
	unsigned long i;

	if(sp && text)
	{
		for(i = 0; i < sp->text_events; i++)
		{
			if((sp->text_event[i]->flags & flags) == 0)
			{	//If the specified flags filters out this event
				continue;	//Skip this event
			}
			if(track_specific)
			{	//If the track specific flag is to be matched
				if(sp->text_event[i]->track != track)
				{	//If this event isn't in the specified track
					continue;	//Skip this event
				}
			}
			if(!ustrcmp(sp->text_event[i]->text, text))
			{
				return 1;	//Return match found
			}
		}
	}
	return 0;	//Return no match found
}

char eof_song_contains_event_beginning_with(EOF_SONG *sp, const char *text, unsigned long track)
{
	unsigned long i;

	if(sp && text)
	{
		for(i = 0; i < sp->text_events; i++)
		{
			if(track && (sp->text_event[i]->track != track))
			{	//If searching for events in a specific track, and this event isn't in that track
				continue;	//Skip this event
			}
			if(ustrstr(sp->text_event[i]->text, text) == sp->text_event[i]->text)
			{	//If a substring search returns the beginning of an existing text event as a match
				return 1;	//Return match found
			}
		}
	}
	return 0;	//Return no match found
}

int eof_text_is_section_marker(const char *text)
{
	if(text)
	{
		if((ustrstr(text, "[section") == text) ||
			(ustrstr(text, "section") == text) ||
			(ustrstr(text, "[prc_") == text))
			return 1;	//If the input string begins with any of these substrings, consider this a section event
	}
	return 0;
}

int eof_is_section_marker(EOF_TEXT_EVENT *ep, unsigned long track)
{
	if(ep)
	{
		if((ep->flags & EOF_EVENT_FLAG_RS_PHRASE) || eof_text_is_section_marker(ep->text))
		{	//If this event's flags denote it as a Rocksmith phrase, or its text indicates a section
			if((track == 0) || (ep->track == 0))
			{	//If the calling function wanted to ignore the text event's associated track, or the event has no associated track
				return 1;
			}
			else if(ep->track == track)
			{	//The text event must also be in the event's associated track to count as a section marker
				return 1;
			}
		}
	}
	return 0;
}
