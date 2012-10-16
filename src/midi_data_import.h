#ifndef EOF_MIDI_DATA_IMPORT_H
#define EOF_MIDI_DATA_IMPORT_H

#include <allegro.h>
#include "song.h"

struct eof_MIDI_data_event
{
	double realtime;	//The timing of this event in milliseconds
	char *stringtime;	//A string representation of the event's realtime position (used to save to project because the conversion between string and double is lossy)
	unsigned long deltatime;	//Stores the original absolute delta time of the event, though this is only going to be relevant when storing a tempo track
	unsigned int size;	//The size of the buffered MIDI event
	void *data;			//The MIDI event data
	struct eof_MIDI_data_event *next;
};	//This structure will be used in a linked list defining the events in a MIDI track

struct eof_MIDI_tempo_change
{
	unsigned long absdelta;				//The absolute delta time of this tempo change
	double realtime;					//The realtime of this tempo change in milliseconds
	double BPM;							//The BPM associated with this tempo change
	struct eof_MIDI_tempo_change *next;	//Pointer to the next link in the list
};

double eof_MIDI_delta_to_realtime(struct eof_MIDI_tempo_change *tempolist, unsigned long absdelta, unsigned timedivision);
	//Accepts a linked list of tempo changes, an absolute delta time and the MIDI's time division
	//The equivalent realtime in milliseconds is returned
	//If tempolist is NULL, the default 120BPM is assumed
void eof_MIDI_empty_event_list(struct eof_MIDI_data_event *ptr);
	//Empties the MIDI event linked list
void eof_MIDI_empty_track_list(struct eof_MIDI_data_track *ptr);
	//Empties the track data linked list
void eof_MIDI_empty_tempo_list(struct eof_MIDI_tempo_change *ptr);
	//Empties the tempo map created by eof_get_raw_MIDI_data()

struct eof_MIDI_data_track *eof_get_raw_MIDI_data(MIDI *midiptr, unsigned tracknum, int offset);
	//Parses the given track of the specified MIDI, returning a linked list containing the track's name and a linked list of the track's events
	//Track 0 of the MIDI is first parsed to build a tempo map
	//Then the specified track is parsed to find the real time of each MIDI is read
	//Offset is an value in milliseconds added to the timestamp of each stored event
	//If offset is negative and would cause the timestamp of an event to be negative, an invalid offset warning is given and the function aborts
	//The raw MIDI data itself is stored in dynamically allocated memory
	//The returned MIDI event linked list's data is essentially the MIDI track in millisecond timing
	//NULL is returned on error
void eof_MIDI_add_track(EOF_SONG *sp, struct eof_MIDI_data_track *ptr);
	//Adds the track to the linked list of raw MIDI track data maintained within the EOF_SONG structure
	//The link is appended as-is, so it must point forward to NULL if appropriate
int eof_track_overridden_by_stored_MIDI_track(EOF_SONG *sp, unsigned long tracknum);
	//Returns nonzero if the project contains a stored MIDI track of the same name as the specified track
	//This would indicate that during MIDI export, the stored track takes precedence
int eof_events_overridden_by_stored_MIDI_track(EOF_SONG *sp);
	//Returns nonzero if the project contains a stored EVENTS MIDI track
	//This would indicate that during MIDI export, the stored track takes precedence

#endif
