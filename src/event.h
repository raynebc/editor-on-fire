#ifndef EOF_EVENT_H
#define EOF_EVENT_H

#include "song.h"

extern EOF_TEXT_EVENT eof_text_event[EOF_MAX_TEXT_EVENTS];
extern char eof_event_list_text[EOF_MAX_TEXT_EVENTS][256];
extern int eof_text_events;
extern EOF_SONG *eof_song_qsort_events_ptr;

EOF_TEXT_EVENT * eof_song_add_text_event(EOF_SONG * sp, unsigned long pos, char * text, unsigned long track, unsigned long flags, char is_temporary);
	//Allocates, initializes and stores a new EOF_TEXT_EVENT structure into the text_event array.
	//Track specifies which track this event will be associated with, or 0 or if it's not track specific.
	//The provided flags are assigned to the new event, such as to define it as a floating text event or Rocksmith related event
	//A boolean status is tracked to regard whether the event is considered temporary.
	//Returns the newly allocated structure or NULL upon error
void eof_move_text_events(EOF_SONG * sp, unsigned long beat, unsigned long offset, int dir);
	//Moves all text events defined at or after the specified beat the specified number of beats in either direction
void eof_song_delete_text_event(EOF_SONG * sp, unsigned long event);
	//Removes and frees the specified text event from the text_events array.  All text events after the deleted text event are moved back in the array one position
int eof_song_resize_text_events(EOF_SONG * sp, unsigned long events);
	//Grows or shrinks the text events array to fit the specified number of notes by allocating/freeing EOF_LYRIC structures.  Return zero on error
void eof_sort_events(EOF_SONG * sp);
	//Performs a quicksort of the specified song's events array
void eof_delete_blank_events(EOF_SONG *sp);
	//Deletes any text events in the specified chart that have no text
int eof_song_qsort_events(const void * e1, const void * e2);
	//The comparitor function used to quicksort the events array
	//Requires that eof_song_qsort_events_ptr has been set to the song structure whose events are being sorted
unsigned long eof_song_lookup_first_event(EOF_SONG *sp, const char *text, unsigned long track, unsigned long flags, unsigned char track_specific);
	//Returns the index number of the first event matching the specified string exactly (case sensitive), or ULONG_MAX if no such event is found or upon error
	// and whose flags ANDED with the specified flags is nonzero (ie. for checking for existing of a RS phrase, use a flags value of all bits set to search for any event type)
	//If the event's flags is already 0, it will not be filtered out
	//If track_specific is nonzero, only events with a matching track number value are checked for comparison, otherwise all events are checked
char eof_song_contains_event(EOF_SONG *sp, const char *text, unsigned long track, unsigned long flags, unsigned char track_specific);
	//Uses eof_song_lookup_first_event() to determine whether there is at least one event matching the specified criteria (the event text is case sensitive)
	//Returns nonzero if there is at least one such event
char eof_song_contains_section_at_pos(EOF_SONG *sp, unsigned long pos, unsigned long track, unsigned long flags, unsigned char track_specific);
	//Similar to eof_song_contains_event(), but instead checks for the existence of any section event at the specified position matching the given flags/track specificity filtering
char eof_song_contains_event_beginning_with(EOF_SONG *sp, const char *text, unsigned long track);
	//Returns nonzero if there is one or more text events that begins with the specified string exactly (case sensitive substring)
	//If track is nonzero, only events in that track are checked for comparison, otherwise all events are checked
int eof_text_is_section_marker(const char *text);
	//Returns nonzero if the specified string is recognized as a section marker event (begins with "[section", "section" or "[prc_")
int eof_is_section_marker(EOF_TEXT_EVENT *ep, unsigned long track);
	//Returns nonzero if the specified text event is recognized as a section marker event based on either its text (as per eof_text_is_section_marker) or its flags
	//If track is nonzero, and the event's associated track is nonzero, the event's track must match the specified track for the function to return nonzero
unsigned long eof_events_set_rs_solo_phrase_status(char *name, unsigned long track, int status, char *undo_made);
	//For all RS phrase events with the specified name that are defined for the specified track (or 0 to indicate all tracks)
	//  set the RS solo phrase flag if status is nonzero, otherwise clear the flag
	//If *undo_made is zero, an undo state is made before altering the chart and *undo_made is set to nonzero
	//Returns the number of events whose RS solo phrase flag were altered

#endif
