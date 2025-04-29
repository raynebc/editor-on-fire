#include <allegro.h>
#include "main.h"
#include "beat.h"
#include "event.h"
#include "undo.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

char eof_event_list_text[EOF_MAX_TEXT_EVENTS][256] = {{0}};
EOF_SONG *eof_song_qsort_events_ptr;	//A pointer to the relevant song structure for the sort, since beat positions need to be able to be referenced

EOF_TEXT_EVENT * eof_song_add_text_event(EOF_SONG * sp, unsigned long pos, char * text, unsigned long track, unsigned long flags, char is_temporary)
{
// 	eof_log("eof_song_add_text_event() entered");

	if(!sp || !text || (sp->text_events >= EOF_MAX_TEXT_EVENTS))
		return NULL;	//Invalid parameters
	if(!(flags & EOF_EVENT_FLAG_FLOATING_POS) && (pos >= sp->beats))
	{	//If this text event is assigned to a beat marker, but the specified beat is invalid
		return NULL;	//Invalid parameters
	}

	sp->text_event[sp->text_events] = malloc(sizeof(EOF_TEXT_EVENT));
	if(!sp->text_event[sp->text_events])
		return NULL;	//If the allocation failed, return NULL

	sp->text_event[sp->text_events]->text[0] = '\0';	//Eliminate false positive in Splint
	(void) ustrcpy(sp->text_event[sp->text_events]->text, text);
	sp->text_event[sp->text_events]->pos = pos;
	if(track >= sp->tracks)
	{	//If this is an invalid track
		track = 0;	//Make this a global text event
	}
	sp->text_event[sp->text_events]->track = track;
	sp->text_event[sp->text_events]->flags = flags;
	sp->text_event[sp->text_events]->is_temporary = is_temporary;
	if(!(flags & EOF_EVENT_FLAG_FLOATING_POS))
	{	//If this text event is assigned to a beat marker
		sp->beat[pos]->flags |= EOF_BEAT_FLAG_EVENTS;	//Set the events flag for the beat
	}
	sp->text_event[sp->text_events]->index = 0;
	sp->text_events++;

	return sp->text_event[sp->text_events-1];	//Return successfully created text event
}

void eof_move_text_events(EOF_SONG * sp, unsigned long beat, unsigned long offset, int dir)
{
	unsigned long i;

	eof_log("eof_move_text_events() entered", 3);

	if(!sp)
	{
		return;
	}
	for(i = 0; i < sp->text_events; i++)
	{	//For each text event
		if(!(sp->text_event[i]->flags & EOF_EVENT_FLAG_FLOATING_POS))
		{	//If this text event is assigned to a beat marker
			if(sp->text_event[i]->pos < beat)
				continue;	//If this event's beat is before the move takes effect, skip it

			if(sp->text_event[i]->pos >= sp->beats)
				continue;	//Do not allow an out of bound access
			sp->beat[sp->text_event[i]->pos]->flags &= ~(EOF_BEAT_FLAG_EVENTS);	//Clear the event flag
			if(dir < 0)
			{
				if(offset > sp->text_event[i]->pos)
					continue;	//Do not allow an underflow
				sp->text_event[i]->pos -= offset;
			}
			else
			{
				if(sp->text_event[i]->pos + offset >= sp->beats)
					continue;	//Do not allow an overflow
				sp->text_event[i]->pos += offset;
			}
			sp->beat[sp->text_event[i]->pos]->flags |= EOF_BEAT_FLAG_EVENTS;	//Set the event flag
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
		eof_song_qsort_events_ptr = sp;	//Set the song structure that will be referenced by the comparitor function
		qsort(sp->text_event, (size_t)sp->text_events, sizeof(EOF_TEXT_EVENT *), eof_song_qsort_events);
	}
}

void eof_delete_blank_events(EOF_SONG *sp)
{
	unsigned long ctr, deletecount;

 	eof_log("eof_delete_blank_events() entered", 1);

	if(sp)
	{
		for(ctr = sp->text_events, deletecount = 0; ctr > 0; ctr--)
		{	//For each text event in the project, in reverse order
			if(sp->text_event[ctr - 1]->text[0] == '\0')
			{	//If the text event has no actual text
				eof_song_delete_text_event(sp, ctr - 1);	//Remove it
				deletecount++;
			}
		}
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tRemoved %lu empty text event%s", deletecount, (deletecount == 1) ? "" : "s");
		eof_log(eof_log_string, 1);
	}
}

int eof_song_qsort_events(const void * e1, const void * e2)
{
	EOF_TEXT_EVENT ** thing1 = (EOF_TEXT_EVENT **)e1;
	EOF_TEXT_EVENT ** thing2 = (EOF_TEXT_EVENT **)e2;
	unsigned long pos1, pos2;

	pos1 = eof_get_text_event_pos_ptr(eof_song_qsort_events_ptr, *thing1);
	pos2 = eof_get_text_event_pos_ptr(eof_song_qsort_events_ptr, *thing2);
	if(pos1 < pos2)
	{
		return -1;
	}
	else if(pos1 == pos2)
	{
		if((*thing1)->index < ((*thing2)->index))
			return -1;	//If the first event's index is lower, it will sort earlier

		return 1;
	}

	return 1;
}

unsigned long eof_song_lookup_first_event(EOF_SONG *sp, const char *text, unsigned long track, unsigned long flags, unsigned char track_specific)
{
	unsigned long i;

	if(!sp || !text)
		return ULONG_MAX;	//Invalid parameters

	for(i = 0; i < sp->text_events; i++)
	{
		if(sp->text_event[i]->flags && ((sp->text_event[i]->flags & flags) == 0))
		{	//If this event has any set flags and the specified flags filters out this event
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
			return i;	//Return match found
		}
	}
	return ULONG_MAX;	//Return no match found
}

char eof_song_contains_event(EOF_SONG *sp, const char *text, unsigned long track, unsigned long flags, unsigned char track_specific)
{
	if(!sp || !text)
		return 0;	//Invalid parameters

	if(eof_song_lookup_first_event(sp, text, track, flags, track_specific) != ULONG_MAX)
		return 1;		//Return match found

	return 0;	//Return no match found
}

char eof_song_contains_section_at_pos(EOF_SONG *sp, unsigned long pos, unsigned long track, unsigned long flags, unsigned char track_specific)
{
	unsigned long i;

	if(!sp)
		return 0;	//Invalid parameters

	for(i = 0; i < sp->text_events; i++)
	{
		if(sp->text_event[i]->flags && ((sp->text_event[i]->flags & flags) == 0))
		{	//If this event has any set flags and the specified flags filters out this event
			continue;	//Skip this event
		}
		if(track_specific)
		{	//If the track specific flag is to be matched
			if(sp->text_event[i]->track != track)
			{	//If this event isn't in the specified track
				continue;	//Skip this event
			}
		}
		if(eof_get_text_event_pos(sp, i) == pos)
		{	//If the text event exists at the specified position
			if(eof_text_is_section_marker(sp->text_event[i]->text))
			{	//If the text event is considered a section
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

unsigned long eof_events_set_rs_solo_phrase_status(char *name, unsigned long track, int status, char *undo_made)
{
	unsigned long ctr, altered = 0;

	if(!eof_song || !undo_made || !name || (track >= eof_song->tracks))
		return 0;	//Invalid parameters

	for(ctr = 0; ctr < eof_song->text_events; ctr++)
	{	//For each event in the project
		if(eof_song->text_event[ctr]->track != track)		//If this event isn't in the target scope
			continue;					//Skip it
		if(ustricmp(name, eof_song->text_event[ctr]->text))	//If the event doesn't have a matching (case-insensitive) name
			continue;					//Skip it
		if(!(eof_song->text_event[ctr]->flags & EOF_EVENT_FLAG_RS_PHRASE))	//If this event isn't a Rocksmith phrase
			continue;							//Skip it

		if(status)
		{	//If the RS solo phrase flag is to be set for all matching events
			if(!(eof_song->text_event[ctr]->flags & EOF_EVENT_FLAG_RS_SOLO_PHRASE))
			{	//If this event doesn't have RS solo phrase status already
				if(*undo_made == 0)
				{	//If an undo state should be made
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					*undo_made = 1;
				}
				altered++;
				eof_song->text_event[ctr]->flags |= EOF_EVENT_FLAG_RS_SOLO_PHRASE;	//Set the flag
			}
		}
		else
		{	//If the RS solo phrase flag is to be cleared for all matching events
			if(eof_song->text_event[ctr]->flags & EOF_EVENT_FLAG_RS_SOLO_PHRASE)
			{	//If this event has the RS solo phrase status
				if(*undo_made == 0)
				{	//If an undo state should be made
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					*undo_made = 1;
				}
				altered++;
				eof_song->text_event[ctr]->flags &= ~EOF_EVENT_FLAG_RS_SOLO_PHRASE;	//Clear the flag
			}

		}
	}

	return altered;
}
