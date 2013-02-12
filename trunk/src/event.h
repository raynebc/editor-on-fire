#ifndef EOF_EVENT_H
#define EOF_EVENT_H

#include "song.h"

extern EOF_TEXT_EVENT eof_text_event[EOF_MAX_TEXT_EVENTS];
extern char eof_event_list_text[EOF_MAX_TEXT_EVENTS][256];
extern int eof_text_events;

EOF_TEXT_EVENT * eof_song_add_text_event(EOF_SONG * sp, unsigned long beat, char * text, unsigned long track, unsigned long flags, char is_temporary);
	//Allocates, initializes and stores a new EOF_TEXT_EVENT structure into the text_event array.
	//Track specifies which track this event will be associated with, or 0 or if it's not track specific.
	//A boolen status is tracked to regard whether the event is considered temporary.
	//Returns the newly allocated structure or NULL upon error
void eof_move_text_events(EOF_SONG * sp, unsigned long beat, unsigned long offset, int dir);
	//Moves all text events defined at or after the specified beat the specified number of beats in either direction
void eof_song_delete_text_event(EOF_SONG * sp, unsigned long event);
	//Removes and frees the specified text event from the text_events array.  All text events after the deleted text event are moved back in the array one position
int eof_song_resize_text_events(EOF_SONG * sp, unsigned long events);
	//Grows or shrinks the text events array to fit the specified number of notes by allocating/freeing EOF_LYRIC structures.  Return zero on error
void eof_sort_events(EOF_SONG * sp);
	//Performs a quicksort of the specified song's events array
int eof_song_qsort_events(const void * e1, const void * e2);
	//The comparitor function used to quicksort the events array
char eof_song_contains_event(EOF_SONG *sp, const char *text, unsigned long track, unsigned long flags);
	//Returns nonzero if there is one or more text events that matches the specified string exactly (case sensitive)
	// and whose flags ANDED with the specified flags is nonzero (ie. for checking for existing of a RS phrase, use a flags value of all bits set to search for any event type)
	//If track is nonzero, only events in that track are checked for comparison, otherwise all events are checked
char eof_song_contains_event_beginning_with(EOF_SONG *sp, const char *text, unsigned long track);
	//Returns nonzero if there is one or more text events that begins with the specified string exactly (case sensitive substring)
	//If track is nonzero, only events in that track are checked for comparison, otherwise all events are checked
int eof_text_is_section_marker(const char *text);
	//Returns nonzero if the specified string is recognized as a section marker event (begins with "[section", "section" or "[prc_"
int eof_is_section_marker(EOF_TEXT_EVENT *ep, unsigned long track);
	//Returns nonzero if the specified text event is recognized as a section marker event based on either its text (as per eof_text_is_section_marker) or its flags
	//If track is nonzero, and the event's associated track is nonzero, the event's track must match the specified track for the function to return nonzero

#endif
