#include <allegro.h>
#include "main.h"
#include "dialog.h"
#include "note.h"
#include "beat.h"
#include "midi.h"
#include "midi_data_import.h"
#include "midi_import.h"	//For eof_parse_var_len()
#include "mix.h"	//For eof_set_seek_position()
#include "rs.h"		//For automated fret hand position generation functions
#include "song.h"
#include "undo.h"
#include "utility.h"
#include "tuning.h"
#include "menu/file.h"	//For drum velocities
#include "menu/note.h"	//For pitch macros
#include "menu/track.h"	//For eof_fret_hand_position_list_dialog_undo_made
#include "foflc/Lyric_storage.h"	//For RBA extraction
#include "foflc/Midi_parse.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

EOF_MIDI_EVENT * eof_midi_event[EOF_MAX_MIDI_EVENTS];
unsigned long eof_midi_events = 0;
char eof_midi_event_full = 0;					//Is set to nonzero when an overflow of the eof_midi_event[] array was prevented, is reset by eof_clear_midi_events()
char eof_midi_note_status[16][128] = {0};		//Used by some functions to track the on/off status of notes 0 through 127 on each of the 16 usable MIDI channels
unsigned long eof_midi_enddelta = 0, eof_midi_endbeatnum = 0;	//If these becomes nonzero, they define the position of a user-defined end event

void eof_add_midi_event(unsigned long pos, int type, int note, int velocity, int channel)
{
	eof_add_midi_event_indexed(pos, type, note, velocity, channel, 0);	//Add the event with an index value of 0
}

void eof_add_midi_event_indexed(unsigned long pos, int type, int note, int velocity, int channel, unsigned long index)
{	//To avoid rounding issues during timing conversion, this should be called with the MIDI tick position of the event being stored
	char note_off = 0;	//Will be set to non zero if this is a note off event
	char note_on = 0;	//Will be set to non zero if this is a note on event

	if((type == 0x80) || (type == 0x90))
	{	//Note on or off event
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tAdding MIDI event:  Pos = %lu, type = 0x%X, note = %d, vel = %d, ch = %d, index = %lu", pos, type, note, velocity, channel, index);
	}
	else
	{	//Other type of event
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tAdding MIDI event:  Pos = %lu, type = 0x%X, par 1 = %d, par 2 = %d, ch = %d, index = %lu", pos, type, note, velocity, channel, index);
	}
	eof_log(eof_log_string, 2);

	if(eof_midi_enddelta && (pos > eof_midi_enddelta))
		return;	//If attempting to write an event that exceeds a user-defined end event, don't do it

	if((type & 0xF0) == 0x80)
	{	//If this is a note off event, convert it to a note on event with a velocity of 0 (equivalent to note off) to make use of running status
		type = 0x90 | (type & 0x0F);	//Change the event status byte, keep the channel number
		velocity = 0;
		note_off = 1;
	}
	else if((type & 0xF0) == 0x90)
	{	//If this is a note on event
		note_on = 1;
	}

	if(eof_midi_events < EOF_MAX_MIDI_EVENTS)
	{	//If eof_midi_event[] is able to store more events
		eof_midi_event[eof_midi_events] = malloc(sizeof(EOF_MIDI_EVENT));
		if(eof_midi_event[eof_midi_events])
		{
			eof_midi_event[eof_midi_events]->pos = pos;
			eof_midi_event[eof_midi_events]->type = type;
			eof_midi_event[eof_midi_events]->note = note;
			eof_midi_event[eof_midi_events]->velocity = velocity;
			eof_midi_event[eof_midi_events]->channel = channel;
			eof_midi_event[eof_midi_events]->dp = NULL;
			eof_midi_event[eof_midi_events]->allocation = 0;
			eof_midi_event[eof_midi_events]->filtered = 0;
			eof_midi_event[eof_midi_events]->on = note_on;
			eof_midi_event[eof_midi_events]->off = note_off;
			eof_midi_event[eof_midi_events]->index = index;
			eof_midi_event[eof_midi_events]->sysexon = 0;
			eof_midi_event[eof_midi_events]->length = 0;	//The calling function is required to set this for HOPO on/off marker's Note Off event
			eof_midi_event[eof_midi_events]->needle = 0;	//Calling function must manually set this if wanted
			eof_midi_events++;

			if((note >= 0) && (note <= 127) && (channel >= 0) && (channel < 16))
			{	//If the note is in bounds of a legal MIDI note, track the writing of each on/off status
				if(note_off)	//Note Off
					eof_midi_note_status[channel][note] = 0;
				else if(note_on)	//Note On
					eof_midi_note_status[channel][note] = 1;
			}
		}
	}
	else
	{
		if(!eof_midi_event_full)
		{
			eof_log("Error:  The number of MIDI events that can be handled has been exceeded.", 1);
			eof_midi_event_full = 1;
		}
	}
}

void eof_add_midi_lyric_event(unsigned long pos, char * text, char allocation)
{	//To avoid rounding issues during timing conversion, this should be called with the MIDI tick position of the event being stored
	eof_log("eof_add_midi_lyric_event() entered", 2);	//Only log this if verbose logging is on

	if(eof_midi_enddelta && (pos > eof_midi_enddelta))
		return;	//If attempting to write an event that exceeds a user-defined end event, don't do it

	if(text)
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tAdding lyric:  pos: %lums\ttext: %s", pos, text);
		eof_log(eof_log_string, 2);

		if(eof_midi_events < EOF_MAX_MIDI_EVENTS)
		{	//If eof_midi_event[] is able to store more events
			eof_midi_event[eof_midi_events] = malloc(sizeof(EOF_MIDI_EVENT));
			if(eof_midi_event[eof_midi_events])
			{
				eof_midi_event[eof_midi_events]->pos = pos;
				eof_midi_event[eof_midi_events]->type = 0x05;
				eof_midi_event[eof_midi_events]->dp = text;
				eof_midi_event[eof_midi_events]->allocation = allocation;
				eof_midi_event[eof_midi_events]->filtered = 0;
				eof_midi_event[eof_midi_events]->on = 0;
				eof_midi_event[eof_midi_events]->off = 0;
				eof_midi_event[eof_midi_events]->needle = 0;	//Calling function must manually set this if wanted
				eof_midi_events++;
			}
			else
			{
				if(!eof_midi_event_full)
				{
					eof_log("Error:  The number of MIDI events that can be handled has been exceeded.", 1);
					eof_midi_event_full = 1;
				}
			}
		}
	}
}

void eof_add_midi_text_event(unsigned long pos, char * text, char allocation, unsigned long index)
{	//To avoid rounding issues during timing conversion, this should be called with the MIDI tick position of the event being stored
	eof_log("eof_add_midi_text_event() entered", 2);	//Only log this if verbose logging is on

	if(eof_midi_enddelta && (pos > eof_midi_enddelta))
		return;	//If attempting to write an event that exceeds a user-defined end event, don't do it

	if(text)
	{
		if(eof_midi_events < EOF_MAX_MIDI_EVENTS)
		{	//If eof_midi_event[] is able to store more events
			eof_midi_event[eof_midi_events] = malloc(sizeof(EOF_MIDI_EVENT));
			if(eof_midi_event[eof_midi_events])
			{
				eof_midi_event[eof_midi_events]->pos = pos;
				eof_midi_event[eof_midi_events]->type = 0x01;
				eof_midi_event[eof_midi_events]->dp = text;
				eof_midi_event[eof_midi_events]->allocation = allocation;
				eof_midi_event[eof_midi_events]->filtered = 0;
				eof_midi_event[eof_midi_events]->on = 0;
				eof_midi_event[eof_midi_events]->off = 0;
				eof_midi_event[eof_midi_events]->index = index;
				eof_midi_event[eof_midi_events]->needle = 0;	//Calling function must manually set this if wanted
				eof_midi_events++;
			}
			else
			{
				if(!eof_midi_event_full)
				{
					eof_log("Error:  The number of MIDI events that can be handled has been exceeded.", 1);
					eof_midi_event_full = 1;
				}
			}
		}
	}
}

void eof_add_sysex_event(unsigned long pos, int size, void *data, char sysexon)
{	//To avoid rounding issues during timing conversion, this should be called with the MIDI tick position of the event being stored
	void *datacopy = NULL;

	eof_log("eof_add_sysex_event() entered", 2);	//Only log this if verbose logging is on

	if((size > 0) && data)
	{
		if(eof_midi_events < EOF_MAX_MIDI_EVENTS)
		{	//If eof_midi_event[] is able to store more events
			eof_midi_event[eof_midi_events] = malloc(sizeof(EOF_MIDI_EVENT));
			if(eof_midi_event[eof_midi_events])
			{
				datacopy = malloc((size_t)size);
				if(datacopy)
				{
					memcpy(datacopy, data, (size_t)size);	//Copy the input data into the new buffer
					eof_midi_event[eof_midi_events]->pos = pos;
					eof_midi_event[eof_midi_events]->type = 0xF0;
					eof_midi_event[eof_midi_events]->note = size;	//Store the size of the Sysex message in the note variable
					eof_midi_event[eof_midi_events]->dp = (char *)datacopy;	//Store the newly buffered data
					eof_midi_event[eof_midi_events]->allocation = 1;	//At this time, all Sysex data chunks are stored in memory allocated within this function
					eof_midi_event[eof_midi_events]->filtered = 0;
					eof_midi_event[eof_midi_events]->on = 0;
					eof_midi_event[eof_midi_events]->off = 0;
					eof_midi_event[eof_midi_events]->index = 0;
					eof_midi_event[eof_midi_events]->sysexon = sysexon;
					eof_midi_event[eof_midi_events]->length = 0;
					eof_midi_event[eof_midi_events]->needle = 0;	//Calling function must manually set this if wanted
					eof_midi_events++;
				}
			}
			else
			{
				if(!eof_midi_event_full)
				{
					eof_log("Error:  The number of MIDI events that can be handled has been exceeded.", 1);
					eof_midi_event_full = 1;
				}
			}
		}
	}
}

void eof_add_phase_shift_sysex_phrase(unsigned long deltastart, unsigned long deltastop, unsigned char diff, unsigned char phraseid)
{
	unsigned char phase_shift_sysex_phrase[8] = {'P','S','\0',0,0,0,0,0xF7};	//This is used to write Sysex messages for features supported in Phase Shift (ie. open strum)

	phase_shift_sysex_phrase[3] = 0;	//Set the Sysex message ID (0 = phrase marker)
	phase_shift_sysex_phrase[4] = diff;	//Store the difficulty ID (0 = Easy, 1 = Medium, 2 = Hard, 3 = Expert)
	phase_shift_sysex_phrase[5] = phraseid;	//Set the phrase ID
	phase_shift_sysex_phrase[6] = 1;	//Set the phrase status (1 = Phrase start)
	eof_add_sysex_event(deltastart, 8, phase_shift_sysex_phrase, 1);	//Write the phrase start marker
	phase_shift_sysex_phrase[6] = 0;	//Set the phrase status (0 = Phrase stop)
	eof_add_sysex_event(deltastop, 8, phase_shift_sysex_phrase, 0);	//Write the phrase stop marker
}

unsigned long eof_find_midi_event_needle(unsigned char num)
{
	unsigned long ctr;
	for(ctr = 0; ctr < eof_midi_events; ctr++)
	{	//For each MIDI event
		if(eof_midi_event[ctr]->needle == num)
		{	//If this event is the needle number being sought
			return ctr;
		}
	}

	return ULONG_MAX;	//Needle not found
}

void eof_clear_midi_events(void)
{
	unsigned long i;

	eof_log("eof_clear_midi_events() entered", 1);

	for(i = 0; i < eof_midi_events; i++)
	{
		if(eof_midi_event[i]->allocation && eof_midi_event[i]->dp)
		{	//If this event has memory allocated for data
			free(eof_midi_event[i]->dp);	//Free it now
		}
		free(eof_midi_event[i]);
	}
	eof_midi_events = 0;
	eof_midi_event_full = 0;
}

int eof_midi_note_already_added(unsigned long deltastart, unsigned long deltaend, int note)
{
	unsigned long ctr;

	for(ctr = 0; ctr < eof_midi_events; ctr++)
	{	//For each stored MIDI event
		if((eof_midi_event[ctr]->pos >= deltastart) && (eof_midi_event[ctr]->pos <= deltaend) && (eof_midi_event[ctr]->note == note))
			return 1;	//The note already exists
	}

	return 0;
}

void WriteVarLen(unsigned long value, PACKFILE * fp)
{
//	eof_log("WriteVarLen() entered");

	unsigned long buffer;
	buffer = value & 0x7F;

	if(!fp)
	{
		return;
	}
	while((value >>= 7))
	{
		buffer <<= 8;
		buffer |= 0x80;
		buffer += (value & 0x7F);
	}

	while(1)
	{
		(void) pack_putc(buffer, fp);
		if(buffer & 0x80)
		{
			buffer >>= 8;
		}
		else
		{
			break;
		}
	}
}

/* sorter for MIDI events so I can output proper MTrk data */
int qsort_helper3(const void * e1, const void * e2)
{
//	eof_log("qsort_helper3() entered");

	EOF_MIDI_EVENT ** thing1 = (EOF_MIDI_EVENT **)e1;
	EOF_MIDI_EVENT ** thing2 = (EOF_MIDI_EVENT **)e2;

	/* Chronological order takes precedence in sorting */
	if((*thing1)->pos < (*thing2)->pos)
	{
		return -1;
	}
	if((*thing1)->pos > (*thing2)->pos)
	{
		return 1;
	}

	/* Logical order of custom Sysex markers:  Phrase on, note on, note off, Phrase off */
	if(((*thing1)->type == 0xF0) && ((*thing2)->type == 0x90))
	{	//Sort a Sysex event and a Note on event
		if((*thing1)->sysexon)
			return -1;	//This is a Sysex phrase on marker
		else
			return 1;	//This is a phrase off marker or some other Sysex message
	}
	if(((*thing1)->type == 0x90) && ((*thing2)->type == 0xF0))
	{	//Sort a Note on event and a Sysex event
		if((*thing2)->sysexon)
			return 1;	//The latter is a Sysex phrase on marker
		else
			return -1;	//The latter is a phrase off marker or some other Sysex message
	}
	if(((*thing1)->type == 0x80) && ((*thing2)->type == 0xF0))
	{	//Sort a Note off event and a Sysex event
		if((*thing2)->sysexon)
			return 1;	//The latter is a Sysex phrase on marker
		else
			return -1;	//The latter is a phrase off marker or some other Sysex message
	}
	if(((*thing1)->type == 0xF0) && ((*thing2)->type == 0x80))
	{	//Sort a Sysex event and a Note off event
		if((*thing1)->sysexon)
			return -1;	//This is a Sysex phrase on marker
		else
			return 1;	//This is a phrase off marker or some other Sysex message
	}

	/* Logical order of lyric markings:  Overdrive on, solo on, lyric on, lyric, lyric pitch, ..., lyric off, solo off, overdrive off */
	if(((*thing1)->type == 0x90) && ((*thing2)->type == 0x90))
	{	//If both things are Note On events
		if((*thing1)->note == 116)
			return -1;	//Overdrive on written first
		if((*thing2)->note == 116)
			return 1;	//Overdrive on written first
		if((*thing1)->note == 105)
			return -1;	//Lyric phrase on written second
		if((*thing2)->note == 105)
			return 1;	//Lyric phrase on written second

		if((*thing1)->note == 108)
			return -1;	//Left hand position note (pro guitar) written before gem notes
		if((*thing2)->note == 108)
			return 1;	//Left hand position note (pro guitar) written before gem notes

		//If two note on events occur at the same time for the same note, but one of them has a velocity of 0 (note off), sort the note off to be first
		if((*thing1)->note == (*thing2)->note)
		{	//If both note on events pertain to the same note
			if((*thing1)->velocity && !(*thing2)->velocity)	//Thing 1 is a note on, thing 2 is a note off
				return 1;
			if((*thing2)->velocity && !(*thing1)->velocity)	//Thing 2 is a note on, thing 1 is a note off
				return -1;
		}
	}

	/* put lyric events first, except for a lyric phrase on marker */
	if(((*thing1)->type == 0x05) && ((*thing2)->type == 0x90))
	{
		if(((*thing2)->note == 105) || ((*thing2)->note == 106))
			return 1;	//lyric phrases should be written before the lyric event

		return -1;
	}
	if(((*thing1)->type == 0x90) && ((*thing2)->type == 0x05))
	{
		if(((*thing1)->note == 105) || ((*thing1)->note == 106))
			return -1;	//lyric phrase should be written before the lyric event

		return 1;
	}

	/* put pro drum phrase on markers before regular notes */
	if(((*thing1)->type == 0x90) && (((*thing1)->note == RB3_DRUM_GREEN_FORCE) || ((*thing1)->note == RB3_DRUM_YELLOW_FORCE) || ((*thing1)->note == RB3_DRUM_BLUE_FORCE)))
		return -1;
	if(((*thing2)->type == 0x90) && (((*thing2)->note == RB3_DRUM_GREEN_FORCE) || ((*thing2)->note == RB3_DRUM_YELLOW_FORCE) || ((*thing2)->note == RB3_DRUM_BLUE_FORCE)))
		return 1;

	/* put notes first and then markers, will numerically sort in this order:  lyric pitch, lyric off, overdrive off */
	if((*thing1)->note < (*thing2)->note)
	{
		return -1;
	}
	if((*thing1)->note > (*thing2)->note)
	{
		return 1;
	}

	/* to avoid strange overlap problems, ensure that if a note on and a note off occur at the same time, the note off is written first.
	   This requires that all notes/phrases are at least 1 delta/ms long */
	if(((*thing1)->type == 0x90) && ((*thing2)->type == 0x80))
	{
		return 1;
	}
	if(((*thing1)->type == 0x80) && ((*thing2)->type == 0x90))
	{
		return -1;
	}

	/* if two text events are at the same position, their index number determines their sort order */
	if(((*thing1)->type == 0x01) && ((*thing2)->type == 0x01))
	{
		if((*thing1)->index < (*thing2)->index)
			return -1;
		if((*thing2)->index < (*thing1)->index)
			return 1;
	}

	// they are equal...
	return 0;
}

/* write MTrk data to a temp file so we can calculate the length in bytes of the track
   write MThd data and copy MTrk data from the temp file using the size of the temp file as the track length
   delete the temp file
   voila, correctly formatted MIDI file */
int eof_export_midi(EOF_SONG * sp, char * fn, char featurerestriction, char fixvoxpitches, char fixvoxphrases, char format)
{
	unsigned char header[14] = {'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 1, 0, 1, (EOF_DEFAULT_TIME_DIVISION >> 8), (EOF_DEFAULT_TIME_DIVISION & 0xFF)}; //The last two bytes are the time division
	unsigned long timedivision = EOF_DEFAULT_TIME_DIVISION;	//Unless the project is storing a tempo track, EOF's default time division will be used
	char notetempname[EOF_TRACKS_MAX+1][25];
	char notetrackspopulated[EOF_TRACKS_MAX+1] = {0};
	char expertplustempname[30];	//Stores the temporary filename for the Expert+ track data
	char tempotempname[30];
	char eventtempname[30];
	char beattempname[30];
	char expertplusfilename[1024] = {0};
	char expertplusshortname[30];
	char *tempotrackname;
	PACKFILE * fp;
	PACKFILE * fp3 = NULL;					//File pointer for the Expert+ file
	unsigned long i, j, k;
	unsigned long ctr, channelctr;
	unsigned long delta = 0, delta2 = 0;
	int midi_note_offset = 0;
	int vel;
	unsigned long tracknum=0;				//Used to de-obfuscate the track number

	struct Tempo_change *anchorlist=NULL;	//Linked list containing tempo changes
	unsigned long lastdelta=0;				//Keeps track of the last anchor's absolute delta time
	char * tempstring = NULL;				//Used to store a copy of the lyric string into eof_midi_event[], so the string can be modified from the original
	long length, deltalength;				//Used to cap drum notes
	char prodrums = 0;						//Tracks whether the drum track being written includes Pro drum notation
	char expertplus = 0;					//Tracks whether an expert+.mid track should be created to hold the Expert+ drum track
	unsigned char expertpluswritten = 0;		//Tracks whether an expert+.mid track has been written
	char eventstrackwritten = 0;				//Tracks whether an events track has been written
	char beattrackwritten = 0;				//Tracks whether a beat track has been written
	unsigned long trackcounter = 0;			//Tracks the number of tracks to write to file
	char trackctr;							//Used in the temp data creation to handle Expert+
	EOF_MIDI_TS_LIST *tslist=NULL;			//List containing TS changes
	unsigned char rootvel;					//Used to write root notes for pro guitar tracks
	unsigned long note, noteflags, notepos, deltapos, nextdeltapos;
	unsigned char type, accent, ghost;
	int channel = 0, velocity = 0, scale, last_scale, chord = 0, isslash = 0, bassnote = 0;	//Used for pro guitar export
	unsigned long bitmask;
	EOF_PHRASE_SECTION *sectionptr;
	char *currentname = NULL, chordname[100]="";
	char fret_hand_pos_written;					//This is used to track whether the track's fret hand positions were completely written yet
	char fret_hand_positions_generated;			//This is used to track whether fret hand positions were automatically generated for an exported pro guitar/bass track's expert difficulty
	char fret_hand_positions_present;			//This is used to track whether fret hand positions are defined for an exported pro guitar/bass track's expert difficulty
	char fret_hand_pos_override_warning = 0;	//This is used to track whether the user has been warned that a track's fret hand positions will be overridden by the song property to only export a single fret hand position of 1
	struct eof_MIDI_data_track *trackptr;		//Used to count the number of raw MIDI tracks to export
	EOF_MIDI_KS_LIST *kslist;
	char stored_beat_track = 0;	//Will be set to nonzero if there is found to be a stored BEAT track in the project
	char slideup, slidedown;	//Will track whether a note has an upward/downward slide or unpitched slide
	char *accentstrings[6] = {"[1]", "[2]", "[3]", "[4]", "[5]", "[6]"};	//Used to export text events for accent notes
	char has_stored_tempo;		//Will be set to nonzero if the project contains a stored tempo track, which will affect timing conversion
	unsigned char default_velocities[6] = {100, 100, 100, 100, 100, 100};	//The velocities used for normal non-drum gems
	unsigned char default_ghost_velocities[6] = {1, 1, 1, 1, 1, 1};				//The velocities used for ghost non-drum gems
	unsigned char default_accent_velocities[6] = {127, 127, 127, 127, 127, 127};	//The velocities used for accent non-drum gems
	unsigned char *velocities = default_velocities;				//The velocities used during export
	unsigned char *ghost_velocities = default_ghost_velocities;
	unsigned char *accent_velocities = default_accent_velocities;

	memset(notetempname, 0, sizeof(notetempname));	//Init this memory to 0
	eof_log("eof_export_midi() entered", 1);

	if(!sp || !fn)
	{
		eof_log("\tError saving:  Invalid parameters", 1);
		return 0;	//Return failure
	}

	if(eof_validate_temp_folder())
	{	//Ensure the correct working directory and presence of the temporary folder
		eof_log("\tCould not validate working directory and temp folder", 1);
		return 0;	//Return failure
	}

	if(!eof_calculate_beat_delta_positions(sp, EOF_DEFAULT_TIME_DIVISION))
	{	//Calculate the delta position of each beat in the chart
		eof_log("\tCould not build beat delta positions", 1);
		return 0;	//Return failure
	}

	//Build tempo and TS lists
	if(!eof_build_tempo_and_ts_lists(sp, &anchorlist, &tslist, &timedivision))
	{
		eof_log("\tError saving:  Cannot build tempo or TS list", 1);
		return 0;	//Return failure
	}
	kslist = eof_build_ks_list(sp);		//Build a list of key signature changes
	if(!kslist)
	{
		eof_log("\tError saving:  Cannot build key signature list", 1);
		eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
		eof_destroy_ts_list(tslist);		//Free memory used by the TS change list
		return 0;	//Return failure
	}
	header[12] = timedivision >> 8;		//Update the MIDI header to reflect the time division (which may have changed if a stored tempo track is present)
	header[13] = timedivision & 0xFF;

	//Generate temporary filenames
	(void) snprintf(expertplustempname, sizeof(expertplustempname) - 1, "%sexpert+.tmp", eof_temp_path_s);
	(void) snprintf(tempotempname, sizeof(tempotempname) - 1, "%stempo.tmp", eof_temp_path_s);
	(void) snprintf(eventtempname, sizeof(eventtempname) - 1, "%sevent.tmp", eof_temp_path_s);
	(void) snprintf(beattempname, sizeof(beattempname) - 1, "%sbeat.tmp", eof_temp_path_s);
	(void) snprintf(expertplusshortname, sizeof(expertplusshortname) - 1, "%sexpert+.mid", eof_temp_path_s);
	for(i = 0; i < EOF_TRACKS_MAX+1; i++)
	{
		(void) snprintf(notetempname[i], sizeof(notetempname[i]) - 1, "%seof%lu.tmp", eof_temp_path_s, i);
	}

	eof_sort_notes(sp);	//Writing efficient on-the-fly HOPO phrasing relies on all notes being sorted
	has_stored_tempo = eof_song_has_stored_tempo_track(sp) ? 1 : 0;	//Store this status

	//Search for a user-defined end event
	eof_midi_enddelta = eof_midi_endbeatnum = 0;	//Assume there is no user-defined end event until one is found
	for(i = 0; i < sp->text_events; i++)
	{
		if(!ustrcmp(sp->text_event[i]->text, "[end]"))
		{	//If there is an end event defined here
			unsigned long eventpos = eof_get_text_event_pos(sp, i);

			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tEnd position located at %lums", eventpos);
			eof_log(eof_log_string, 1);
			eof_midi_endbeatnum = eof_get_beat(sp, eventpos);
			eof_midi_enddelta = eof_ConvertToDeltaTime(sp->beat[eof_midi_endbeatnum]->pos, anchorlist, tslist, timedivision, 0, has_stored_tempo);	//Get the delta time of this event
			break;
		}
	}

	if(featurerestriction)
	{	//If writing a Rock Band compliant MIDI
		//Magma requires some default track events to be written
		if(sp->tags->ogg[0].midi_offset != 0)
		{	//Rock Band songs are have a MIDI offset of 0
			eof_log("\t! Warning:  MIDI offset is not zero, this song may play out of sync in Rock Band", 1);
		}
		if(sp->beats > 3)
		{	//Only add these if there are at least 4 beats
			if(!eof_song_contains_event(sp, "[music_start]", 0, 0xFFFF, 0))
			{	//If the user did not define the music_start event
				eof_log("\t! Adding missing [music_start] event", 1);
				(void) eof_song_add_text_event(sp, 2, "[music_start]", 0, 0, 1);	//Add it as a temporary event two beats into the song (at the third beat)
			}
			if(!eof_song_contains_event(sp, "[music_end]", 0, 0xFFFF, 0))
			{	//If the user did not define the music_end event
				eof_log("\t! Adding missing [music_end] event", 1);
				(void) eof_song_add_text_event(sp, sp->beats-1, "[music_end]", 0, 0, 1);	//Add it as a temporary event on the last beat
			}
			if(!eof_song_contains_event_beginning_with(sp, "[mix 0", EOF_TRACK_DRUM))
			{	//If the user did not define an easy difficulty drum mix event
				eof_log("\t! Adding missing easy drum mix event", 1);
				(void) eof_song_add_text_event(sp, 0, "[mix 0 drums0]", EOF_TRACK_DRUM, 0, 1);	//Add one as a temporary event on the first beat of the drum track
			}
			if(!eof_song_contains_event_beginning_with(sp, "[mix 1", EOF_TRACK_DRUM))
			{	//If the user did not define a medium difficulty drum mix event
				eof_log("\t! Adding missing medium drum mix event", 1);
				(void) eof_song_add_text_event(sp, 0, "[mix 1 drums0]", EOF_TRACK_DRUM, 0, 1);	//Add one as a temporary event on the first beat of the drum track
			}
			if(!eof_song_contains_event_beginning_with(sp, "[mix 2", EOF_TRACK_DRUM))
			{	//If the user did not define a hard difficulty drum mix event
				eof_log("\t! Adding missing hard drum mix event", 1);
				(void) eof_song_add_text_event(sp, 0, "[mix 2 drums0]", EOF_TRACK_DRUM, 0, 1);	//Add one as a temporary event on the first beat of the drum track
			}
			if(!eof_song_contains_event_beginning_with(sp, "[mix 3", EOF_TRACK_DRUM))
			{	//If the user did not define an expert difficulty drum mix event
				eof_log("\t! Adding missing expert drum mix event", 1);
				(void) eof_song_add_text_event(sp, 0, "[mix 3 drums0]", EOF_TRACK_DRUM, 0, 1);	//Add one as a temporary event on the first beat of the drum track
			}
		}
	}//If writing a Rock Band compliant MIDI
	eof_sort_events(sp);

	//Write tracks
	for(j = 1; j < sp->tracks; j++)
	{	//For each track in the project
		char restore_tech_view = 0;				//If tech view is in effect, it is temporarily disabled so that the correct notes are exported
		char isghl = 0;							//Set to nonzero if the track is to be exported as a GHL track (non GHWT MIDI format, no export feature restrictions, track flag indicates GHL mode active)
		char ghlname[EOF_NAME_LENGTH + 1 + 4];	//GHL tracks have a " GHL" suffix applied to their name during export
		char write_enhanced_opens = 0;			//Set to nonzero if an "[ENHANCED_OPENS]" text event is defined anywhere in the track, or upon the encountering of the first legacy guitar open note/chord if the eof_midi_export_enhanced_open_marker preference is enabled

		if(eof_get_track_size_normal(sp, j) == 0)	//If this track has no notes
			continue;	//Skip the track
		if(j == EOF_TRACK_PRO_GUITAR_B)		//If this is the bonus pro guitar track
			continue;	//Skip the track
		if(eof_track_overridden_by_stored_MIDI_track(sp, j))	//If this track is overridden by a stored MIDI track
			continue;	//Skip the track
		if(!featurerestriction && !format && eof_track_is_ghl_mode(sp, j))
			isghl = 1;	//Note whether this track will export as a GHL track
		if(eof_song_contains_event(sp, "[ENHANCED_OPENS]", j, 0, 1))
			write_enhanced_opens = 1;	//Note that the current track has the enhanced opens modifier

		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tWriting track \"%s\"", sp->track[j]->name);
		eof_log(eof_log_string, 2);

		last_scale = INT_MAX;	//Reset this every track
		if(featurerestriction == 0)
		{
			if(sp->track[j]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
			{	//If this is a drum track, use any custom defined velocity values
				velocities = eof_drum_velocities;
				ghost_velocities = eof_drum_ghost_velocities;
				accent_velocities = eof_drum_accent_velocities;
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tUsing normal note velocities:  %u,%u,%u,%u,%u,%u", eof_drum_velocities[0], eof_drum_velocities[1], eof_drum_velocities[2], eof_drum_velocities[3], eof_drum_velocities[4], eof_drum_velocities[5]);
				eof_log(eof_log_string, 1);
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tUsing ghost note velocities:  %u,%u,%u,%u,%u,%u", eof_drum_ghost_velocities[0], eof_drum_ghost_velocities[1], eof_drum_ghost_velocities[2], eof_drum_ghost_velocities[3], eof_drum_ghost_velocities[4], eof_drum_ghost_velocities[5]);
				eof_log(eof_log_string, 1);
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tUsing accent note velocities:  %u,%u,%u,%u,%u,%u", eof_drum_accent_velocities[0], eof_drum_accent_velocities[1], eof_drum_accent_velocities[2], eof_drum_accent_velocities[3], eof_drum_accent_velocities[4], eof_drum_accent_velocities[5]);
				eof_log(eof_log_string, 1);
			}
		}
		else
		{	//Use the standard velocity values
			velocities = default_velocities;
			ghost_velocities = default_ghost_velocities;
			accent_velocities = default_accent_velocities;
		}

		if(featurerestriction == 1)
		{	//If writing a RBN2 or C3 compliant MIDI
			if((j != EOF_TRACK_GUITAR) && (j != EOF_TRACK_BASS) && (j != EOF_TRACK_DRUM) && (j != EOF_TRACK_VOCALS) && (j != EOF_TRACK_KEYS) && (j != EOF_TRACK_PRO_KEYS))
			{	//If this track is not valid for RBN2
				continue;	//Skip the track
			}
		}
		else if(featurerestriction == 2)
		{	//If writing a RB3 compliant pro guitar upgrade MIDI
			if((j != EOF_TRACK_PRO_BASS) && (j != EOF_TRACK_PRO_GUITAR) && (j != EOF_TRACK_PRO_BASS_22) && (j != EOF_TRACK_PRO_GUITAR_22))
			{	//If this track is not valid for a RB3 pro guitar upgrade
				continue;	//Skip the track
			}
		}
		else if(featurerestriction == 3)
		{	//If writing a C3 compliant MIDI
			if((j != EOF_TRACK_GUITAR) && (j != EOF_TRACK_BASS) && (j != EOF_TRACK_DRUM) && (j != EOF_TRACK_VOCALS) && (j != EOF_TRACK_KEYS) && (j != EOF_TRACK_PRO_KEYS) && (j != EOF_TRACK_PRO_BASS) && (j != EOF_TRACK_PRO_GUITAR) && (j != EOF_TRACK_PRO_BASS_22) && (j != EOF_TRACK_PRO_GUITAR_22))
			{	//if this track is not valid for Rock Band 3
				continue;	//Skip the track
			}
		}
		trackcounter++;	//Count this track towards the number of tracks to write to the completed MIDI

		if(j == EOF_TRACK_DANCE)
		{	//Phase Shift's dance track specification is for dance notes to use a velocity of 127
			vel = 127;
		}
		else
		{	//For other tracks, the generic velocity is 100
			vel = 100;
		}

		restore_tech_view = eof_menu_track_get_tech_view_state(sp, j);
		eof_menu_track_set_tech_view_state(sp, j, 0);	//Disable tech view if applicable

		notetrackspopulated[j] = 1;	//Remember that this track is populated
		eof_clear_midi_events();
		tracknum = sp->track[j]->tracknum;
		memset(eof_midi_note_status,0,sizeof(eof_midi_note_status));	//Clear note status array

		//Write track specific text events
		for(i = 0; i < sp->text_events; i++)
		{	//For each event in the song
			if(sp->text_event[i]->track == j)
			{	//If this event is specific to this track
				double eventpos;

				if(!(sp->text_event[i]->flags & EOF_EVENT_FLAG_FLOATING_POS))
				{	//If this text event is assigned to a beat marker
					if(sp->text_event[i]->pos >= sp->beats)
					{	//If the text event's beat number is invalid
						sp->text_event[i]->pos = sp->beats - 1;	//Repair it by assigning it to the last beat marker
					}
				}
				eventpos = eof_get_text_event_fpos(sp, i);
				deltapos = eof_ConvertToDeltaTime(eventpos, anchorlist, tslist, timedivision, 1, has_stored_tempo);	//Store the tick position of the beat
				eof_add_midi_text_event(deltapos, sp->text_event[i]->text, 0, i);	//Send 0 for the allocation flag, because the text string is being stored in static memory
			}
		}
		if(eof_get_note_pos(sp, j, 0) < 2450)
		{	//Magma will not allow any note/lyric to be before 2.45s
			eof_log("\t! A note or lyric appears before 2450ms, Magma will probably not accept this MIDI file", 1);
		}

		if(sp->track[j]->track_format == EOF_LEGACY_TRACK_FORMAT)
		{	//If this is a legacy track
			/* fill in notes */
//Detect whether Pro drum notation is being used
//Pro drum notation is that if a green, yellow or blue drum note is NOT to be marked as a cymbal,
//it must be marked with the appropriate MIDI note, otherwise the note defaults as a cymbal
			prodrums = eof_track_has_cymbals(sp, j);

			if(eof_force_pro_drum_midi_notation)
				prodrums = 1;	//If the preference to force the use of pro drum notation is enabled, ensure it will export that way

			//Create accent note text events as needed for the GHWT MIDI variant
			if(format == 1)
			{	//If writing the GHWT variant MIDI
				for(i = 0; i < eof_get_track_size(sp, j); i++)
				{	//For each note in the track
					unsigned char accentctr, accentmask;

					accent = eof_get_note_accent(sp, j, i);
					if(accent && (eof_get_note_type(sp, j, i) == EOF_NOTE_AMAZING))
					{	//If this note has one or more accented gems and is in the expert difficulty
						for(accentctr = 0, accentmask = 1; accentctr < 6; accentctr++, accentmask <<= 1)
						{	//For each lane in the note
							if(accent & accentmask)
							{	//If this lane is accented
								deltapos = eof_ConvertToDeltaTime(eof_get_note_pos(sp, j, i), anchorlist, tslist, timedivision, 1, has_stored_tempo);	//Store the tick position of the note
								eof_add_midi_text_event(deltapos, accentstrings[accentctr], 0, i);	//Send 0 for the allocation flag, because the text string is being stored in static memory
							}
						}
					}
				}
			}

			/* write the MTrk MIDI data to a temp file
			use size of the file as the MTrk header length */
			for(i = 0; i < eof_get_track_size(sp, j); i++)
			{	//For each note in the track
				noteflags = eof_get_note_flags(sp, j, i);	//Store the note flags for easier use
				note = eof_get_note_note(sp, j, i);			//Store the note bitflag for easier use
				notepos = eof_get_note_pos(sp, j, i);		//Store the note position for easier use
				length = eof_get_note_length(sp, j, i);		//Store the note length for easier use
				type = eof_get_note_type(sp, j, i);			//Store the note type for easier use
				accent = eof_get_note_accent(sp, j, i);		//Store the accent bitmask for easier use
				ghost = eof_get_note_ghost(sp, j, i);		//Store the ghost bitmask for easier use

				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tWriting note:  Pos = %lums\tLength = %ld\tMask = %lu", notepos, length, note);
				eof_log(eof_log_string, 2);

				if(isghl)
				{	//This is a GHL track
					switch(type)
					{
						case EOF_NOTE_AMAZING:	//Notes 94-102 (Lane 4, W1, maps to 95)
						{
							midi_note_offset = 95;
							break;
						}
						case EOF_NOTE_MEDIUM:	//Notes 82-90 (Lane 4, W1, maps to 83)
						{
							midi_note_offset = 83;
							break;
						}
						case EOF_NOTE_EASY:		//Notes 70-78 (Lane 4, W1, maps to 71)
						{
							midi_note_offset = 71;
							break;
						}
						case EOF_NOTE_SUPAEASY:	//Notes 58-66 (Lane 4, W1, maps to 59)
						{
							midi_note_offset = 59;
							break;
						}
						default:	//Don't write BRE difficulty notes to GHL format MIDI
						continue;
					}
				}
				else if(j == EOF_TRACK_DANCE)
				{	//This is the dance track
					switch(type)
					{
						case EOF_NOTE_CHALLENGE:	//notes 96-107
						{
							midi_note_offset = 96;
							break;
						}
						case EOF_NOTE_AMAZING:	//notes 84-95
						{
							midi_note_offset = 84;
							break;
						}
						case EOF_NOTE_MEDIUM:	//notes 72-83
						{
							midi_note_offset = 72;
							break;
						}
						case EOF_NOTE_EASY:		//notes 60-71
						{
							midi_note_offset = 60;
							break;
						}
						case EOF_NOTE_SUPAEASY:	//notes 48-59
						{
							midi_note_offset = 48;
							break;
						}
						default:
						break;
					}
				}
				else
				{	//All other legacy style tracks use the same offsets
					switch(type)
					{
						case EOF_NOTE_AMAZING:	//notes 96-100
						{
							midi_note_offset = 0x60;
							break;
						}
						case EOF_NOTE_MEDIUM:	//notes 84-88
						{
							midi_note_offset = 0x54;
							break;
						}
						case EOF_NOTE_EASY:		//notes 72-76
						{
							midi_note_offset = 0x48;
							break;
						}
						case EOF_NOTE_SUPAEASY:	//notes 60-64
						{
							midi_note_offset = 0x3C;
							break;
						}
						case EOF_NOTE_SPECIAL:	//BRE/drum fill: notes 120-124
						{
							midi_note_offset = 120;
							break;
						}
						default:
						break;
					}
				}

				deltapos = eof_ConvertToDeltaTime(notepos, anchorlist, tslist, timedivision, 1, has_stored_tempo);	//Store the tick position of the note
				deltalength = eof_ConvertToDeltaTime(notepos + length, anchorlist, tslist, timedivision, 0, has_stored_tempo) - deltapos;	//Store the number of delta ticks representing the note's length
				if(i + 1 < eof_get_track_size(sp, j))
				{	//If there is another note
					nextdeltapos = eof_ConvertToDeltaTime(eof_get_note_pos(sp, j, i + 1), anchorlist, tslist, timedivision, 1, has_stored_tempo);	//Store its tick position
				}
				else
				{
					nextdeltapos = 0;
				}
				if(deltalength < 1)
				{	//If some kind of rounding error or other issue caused the delta length to be less than 1, force it to the minimum length of 1
					deltalength = 1;
				}
				if((sp->track[j]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR) && (type != EOF_NOTE_SPECIAL) && !(noteflags & EOF_NOTE_FLAG_CRAZY))
				{	//Ensure that drum notes are not written with sustain (Unless they are BRE notes or have crazy status)
					deltalength = 1;
				}

				//Ensure open strum notes are prepared for export (for non GHL tracks)
				if(eof_legacy_guitar_note_is_open(sp, j, i))
				{	//If this is an open note
					if(!isghl)
					{	//If this is not exporting as a GHL mode track
						if(format == 1)
						{	//If writing the GHWT MIDI variant
							note = 31;	//Convert it to a 5 lane chord
						}
					}
				}

				if((featurerestriction == 1) || (featurerestriction == 3))
				{	//If writing a RBN2 or C3 compliant MIDI
					if((noteflags & EOF_GUITAR_NOTE_FLAG_IS_SLIDER) && eof_rbn_export_slider_hopo)
					{	//If this note is in a slider phrase and the user opted to export such notes as forced HOPO notes
						noteflags |= EOF_NOTE_FLAG_F_HOPO;	//Observe the forced HOPO flag being in use for the note
					}
				}

				/* write green note */
				if(note & 1)
				{
					if(isghl)
					{	//Write B1 GHL gem
						eof_add_midi_event(deltapos, 0x90, midi_note_offset + 3, vel, 0);
						eof_add_midi_event(deltapos + deltalength, 0x80, midi_note_offset + 3, vel, 0);
					}
					else
					{
						vel = velocities[0];	//Restore the default velocity
						if((sp->track[j]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR) && (featurerestriction == 0))
						{	//If this is a drum note and a Rock Band compliant MIDI is not being written
							if(accent & 1)
							{	//If this gem is accented
								vel = accent_velocities[0];	//Use the applicable accent velocity
							}
							if(ghost & 1)
							{	//If this gem is ghosted
								vel = ghost_velocities[0];		//Use the applicable ghost velocity
							}
						}
						if(!((noteflags & EOF_DRUM_NOTE_FLAG_DBASS) && sp->tags->double_bass_drum_disabled))
						{	//If this is not an expert+ bass drum note that would be skipped due to such notes being disabled
							if((sp->track[j]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR) && (type == 3) && (noteflags & EOF_DRUM_NOTE_FLAG_DBASS) && !featurerestriction)
							{	//If the track being written is a drum track, this note is in expert difficulty and marked for Expert+ double bass, and not writing a RB3 compliant MIDI
								eof_add_midi_event(deltapos, 0x90, 95, vel, 0);		//Note 95 is used for Expert+ bass notes
								eof_add_midi_event(deltapos + deltalength, 0x80, 95, vel, 0);
								expertplus = 1;
							}
							else	//Otherwise write a normal green gem
							{
								eof_add_midi_event(deltapos, 0x90, midi_note_offset + 0, vel, 0);
								eof_add_midi_event(deltapos + deltalength, 0x80, midi_note_offset + 0, vel, 0);
							}
						}
					}
				}

				/* write red note */
				if(note & 2)
				{
					if(isghl)
					{	//Write B2 GHL gem
						eof_add_midi_event(deltapos, 0x90, midi_note_offset + 4, vel, 0);
						eof_add_midi_event(deltapos + deltalength, 0x80, midi_note_offset + 4, vel, 0);
					}
					else
					{
						vel = velocities[1];	//Restore the default velocity
						if((sp->track[j]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR) && (featurerestriction == 0))
						{	//If this is a drum note and a Rock Band compliant MIDI is not being written
							if(accent & 2)
							{	//If this gem is accented
								vel = accent_velocities[1];	//Use the applicable accent velocity
							}
							if(ghost & 2)
							{	//If this gem is ghosted
								vel = ghost_velocities[1];		//Use the applicable ghost velocity
							}
						}
						eof_add_midi_event(deltapos, 0x90, midi_note_offset + 1, vel, 0);
						eof_add_midi_event(deltapos + deltalength, 0x80, midi_note_offset + 1, vel, 0);
						if(sp->track[j]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
						{	//If this is a drum track, prepare to write drum specific Sysex phrases if necessary
							if(noteflags & EOF_DRUM_NOTE_FLAG_R_RIMSHOT)
							{	//If this note is marked as a rim shot
								if(featurerestriction == 0)
								{	//Only write this notation if not writing a Rock Band compliant MIDI
									eof_add_phase_shift_sysex_phrase(deltapos, deltapos + deltalength, type, 7);	//Write custom rim shot phrase markers
								}
							}
						}
					}
				}

				/* write yellow note */
				if(note & 4)
				{
					if(isghl)
					{	//Write B3 GHL gem
						eof_add_midi_event(deltapos, 0x90, midi_note_offset + 5, vel, 0);
						eof_add_midi_event(deltapos + deltalength, 0x80, midi_note_offset + 5, vel, 0);
					}
					else
					{
						vel = velocities[2];	//Restore the default velocity
						if((sp->track[j]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR) && (featurerestriction == 0))
						{	//If this is a drum note and a Rock Band compliant MIDI is not being written
							if(accent & 4)
							{	//If this gem is accented
								vel = accent_velocities[2];	//Use the applicable accent velocity
							}
							if(ghost & 4)
							{	//If this gem is ghosted
								vel = ghost_velocities[2];		//Use the applicable ghost velocity
							}
						}
						eof_add_midi_event(deltapos, 0x90, midi_note_offset + 2, vel, 0);
						eof_add_midi_event(deltapos + deltalength, 0x80, midi_note_offset + 2, vel, 0);
						if((sp->track[j]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR) && prodrums && !eof_check_flags_at_legacy_note_pos(sp->legacy_track[tracknum],i,EOF_DRUM_NOTE_FLAG_Y_CYMBAL))
						{	//If pro drum notation is in effect and no more yellow drum notes at this note's position are marked as cymbals
							if((featurerestriction == 0) || (type == EOF_NOTE_AMAZING))
							{	//Write a pro yellow tom marker only if this is an Expert difficulty note (ie. not a BRE note) or if a Rock Band compliant MIDI is not being written
								if(type < EOF_NOTE_SPECIAL)
								{	//But absolutely not for the BRE difficulty
									if(!eof_midi_note_already_added(deltapos, deltapos + deltalength, RB3_DRUM_YELLOW_FORCE))
									{	//If a yellow tom marker wasn't written for this position in another difficulty
										eof_add_midi_event(deltapos, 0x90, RB3_DRUM_YELLOW_FORCE, vel, 0);
										eof_add_midi_event(deltapos + deltalength, 0x80, RB3_DRUM_YELLOW_FORCE, vel, 0);
									}
								}
							}
						}
					}
				}

				/* write hi hat notation for either red or yellow gems (to allow for notation during disco flips) */
				if(note & 6)
				{
					if(sp->track[j]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
					{	//If this is a drum track, prepare to write drum specific Sysex phrases if necessary
						if(featurerestriction == 0)
						{	//Only write these notations if not writing a Rock Band compliant MIDI
							if(noteflags & EOF_DRUM_NOTE_FLAG_Y_HI_HAT_OPEN)
							{	//If this note is marked as an open hi hat note
								eof_add_phase_shift_sysex_phrase(deltapos, deltapos + deltalength, type, 5);	//Write custom open hi hat phrase markers
							}
							else if(noteflags & EOF_DRUM_NOTE_FLAG_Y_HI_HAT_PEDAL)
							{	//If this note is marked as a pedal controlled hi hat note
								eof_add_phase_shift_sysex_phrase(deltapos, deltapos + deltalength, type, 6);	//Write custom pedal controlled hi hat phrase markers
							}
							else if(noteflags & EOF_DRUM_NOTE_FLAG_Y_SIZZLE)
							{	//If this note is marked as a sizzle hi hat note
								eof_add_phase_shift_sysex_phrase(deltapos, deltapos + deltalength, type, 8);	//Write custom sizzle hi hat phrase markers
							}
							if(noteflags & EOF_DRUM_NOTE_FLAG_Y_COMBO)
							{	//If this note is marked as a yellow tom/cymbal combo
								eof_add_phase_shift_sysex_phrase(deltapos, deltapos + deltalength, type, 17);	//Write custom yellow tom+cymbal phrase markers
							}
						}//Only write these notations if not writing a Rock Band compliant MIDI
					}
				}

				/* write blue note */
				if(note & 8)
				{
					if(isghl)
					{	//Write W1 GHL gem
						eof_add_midi_event(deltapos, 0x90, midi_note_offset, vel, 0);
						eof_add_midi_event(deltapos + deltalength, 0x80, midi_note_offset, vel, 0);
					}
					else
					{
						vel = velocities[3];	//Restore the default velocity
						if((sp->track[j]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR) && (featurerestriction == 0))
						{	//If this is a drum note and a Rock Band compliant MIDI is not being written
							if(accent & 8)
							{	//If this gem is accented
								vel = accent_velocities[3];	//Use the applicable accent velocity
							}
							if(ghost & 8)
							{	//If this gem is ghosted
								vel = ghost_velocities[3];		//Use the applicable ghost velocity
							}
						}
						if((sp->track[j]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR) && prodrums && !eof_check_flags_at_legacy_note_pos(sp->legacy_track[tracknum],i,EOF_DRUM_NOTE_FLAG_B_CYMBAL))
						{	//If pro drum notation is in effect and no more blue drum notes at this note's position are marked as cymbals
							if((featurerestriction == 0) || (type == EOF_NOTE_AMAZING))
							{	//Write a pro blue tom marker only if this is an Expert difficulty note (ie. not a BRE note) or if a Rock Band compliant MIDI is not being written
								if(type < EOF_NOTE_SPECIAL)
								{	//But absolutely not for the BRE difficulty
									if(!eof_midi_note_already_added(deltapos, deltapos + deltalength, RB3_DRUM_BLUE_FORCE))
									{	//If a blue tom marker wasn't written for this position in another difficulty
										eof_add_midi_event(deltapos, 0x90, RB3_DRUM_BLUE_FORCE, vel, 0);
										eof_add_midi_event(deltapos + deltalength, 0x80, RB3_DRUM_BLUE_FORCE, vel, 0);
									}
								}
							}
						}
						eof_add_midi_event(deltapos, 0x90, midi_note_offset + 3, vel, 0);
						eof_add_midi_event(deltapos + deltalength, 0x80, midi_note_offset + 3, vel, 0);
						if(noteflags & EOF_DRUM_NOTE_FLAG_B_COMBO)
						{	//If this note is marked as a blue tom/cymbal combo
							eof_add_phase_shift_sysex_phrase(deltapos, deltapos + deltalength, type, 18);	//Write custom blue tom+cymbal phrase markers
						}
					}
				}

				/* write purple note */
				if(note & 16)
				{	//Note: EOF/FoF refer to this note color as purple/orange whereas Rock Band displays it as green
					if(isghl)
					{	//Write W2 GHL gem
						eof_add_midi_event(deltapos, 0x90, midi_note_offset + 1, vel, 0);
						eof_add_midi_event(deltapos + deltalength, 0x80, midi_note_offset + 1, vel, 0);
					}
					else
					{
						vel = velocities[4];	//Restore the default velocity
						if((sp->track[j]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR) && (featurerestriction == 0))
						{	//If this is a drum note and a Rock Band compliant MIDI is not being written
							if(accent & 16)
							{	//If this gem is accented
								vel = accent_velocities[4];	//Use the applicable accent velocity
							}
							if(ghost & 16)
							{	//If this gem is ghosted
								vel = ghost_velocities[4];		//Use the applicable ghost velocity
							}
						}
						if((sp->track[j]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR) && prodrums && !eof_check_flags_at_legacy_note_pos(sp->legacy_track[tracknum],i,EOF_DRUM_NOTE_FLAG_G_CYMBAL))
						{	//If pro drum notation is in effect and no more green drum notes at this note's position are marked as cymbals
							if((featurerestriction == 0) || (type == EOF_NOTE_AMAZING))
							{	//Write a pro green tom marker only if this is an Expert difficulty note (ie. not a BRE note) or if a Rock Band compliant MIDI is not being written
								if(type < EOF_NOTE_SPECIAL)
								{	//But absolutely not for the BRE difficulty
									if(!eof_midi_note_already_added(deltapos, deltapos + deltalength, RB3_DRUM_GREEN_FORCE))
									{	//If a green tom marker wasn't written for this position in another difficulty
										eof_add_midi_event(deltapos, 0x90, RB3_DRUM_GREEN_FORCE, vel, 0);
										eof_add_midi_event(deltapos + deltalength, 0x80, RB3_DRUM_GREEN_FORCE, vel, 0);
									}
								}
							}
						}
						eof_add_midi_event(deltapos, 0x90, midi_note_offset + 4, vel, 0);
						eof_add_midi_event(deltapos + deltalength, 0x80, midi_note_offset + 4, vel, 0);
						if(noteflags & EOF_DRUM_NOTE_FLAG_G_COMBO)
						{	//If this note is marked as a green tom/cymbal combo
							eof_add_phase_shift_sysex_phrase(deltapos, deltapos + deltalength, type, 19);	//Write custom green tom+cymbal phrase markers
						}
					}
				}

				/* write W3 GHL gem */
				if(isghl && (note & 32) && !eof_legacy_guitar_note_is_open(sp, j, i))
				{	//If this is a lane 6 gem (without the GHL open note status) and the track is exporting as a GHL track
					eof_add_midi_event(deltapos, 0x90, midi_note_offset + 2, vel, 0);
					eof_add_midi_event(deltapos + deltalength, 0x80, midi_note_offset + 2, vel, 0);
				}

				/* write open strum note marker, if the feature was enabled during save */
				if(eof_legacy_guitar_note_is_open(sp, j, i))
				{	//If this is an open strum note
					if(isghl)
					{	//If this is a GHL mode track
						eof_add_midi_event(deltapos, 0x90, midi_note_offset - 1, vel, 0);				//Write as 1 note below lane 1
						eof_add_midi_event(deltapos + deltalength, 0x80, midi_note_offset - 1, vel, 0);
					}
					else if(featurerestriction == 0)
					{	//Otherwise only write this notation if not writing a Rock Band compliant MIDI
						unsigned long marker_length = deltalength;	//By default, write the marker to be the full length of the note

						if(noteflags & EOF_NOTE_FLAG_CRAZY)
						{	//If the open note has crazy status though
							marker_length = 1;	//End the marker immediately to better suit Clone Hero's MIDI logic to ensure overlapped notes aren't converted to open strums
						}
						if(!write_enhanced_opens && eof_midi_export_enhanced_open_marker)
						{	//If an "[ENHANCED_OPENS]" text event was not manually defined by the user, but the applicable export preference was enabled to use this notation
							eof_log("\t! Adding missing [ENHANCED_OPENS] event", 1);
							(void) eof_song_add_text_event(sp, 0, "[ENHANCED_OPENS]", j, 0, 1);	//Add it as a temporary event on the first beat in this track
							write_enhanced_opens = 1;	//Ensure enhanced open notation will be used below
						}

						if(write_enhanced_opens)
						{	//If enhanced open notes are being used, write it as 1 note below lane 1
							eof_add_midi_event(deltapos, 0x90, midi_note_offset - 1, vel, 0);
							eof_add_midi_event(deltapos + deltalength, 0x80, midi_note_offset - 1, vel, 0);
						}
						else
						{	//Otherwise write a sysex open note
							if(note == 32)
							{	//If this is just an open note (not an open chord, which would contain at least one gem on a lane other than lane 6)
								eof_add_midi_event(deltapos, 0x90, midi_note_offset + 0, vel, 0);	//Write a gem for lane 1
								eof_add_midi_event(deltapos + deltalength, 0x80, midi_note_offset + 0, vel, 0);
							}
							eof_add_phase_shift_sysex_phrase(deltapos, deltapos + marker_length, type, 1);	//Write custom open strum bass phrase markers
						}
					}
				}

				/* write fifth lane drum note, if the feature was enabled during save */
				if(eof_five_lane_drums_enabled() && (sp->track[j]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR) && (note & 32))
				{	//If this is a lane 6 gem (referred to as lane 5 for drums, seeing as bass drum doesn't use a lane)
					if(format == 1)
					{	//If writing the GHWT MIDI variant
						long pad = EOF_DEFAULT_TIME_DIVISION / 16;	//To make this notation more visible in Feedback, pad to a minimum length of 1/64 (in #/4 meter) if possible
						if(deltalength < pad)
						{	//If the note being exported is shorter than 1/64
							if(!nextdeltapos || (nextdeltapos > deltapos + pad))
							{	//If there is no next note or there is one and it is far away enough
								deltalength = pad;
							}
						}
						eof_add_midi_event(deltapos, 0x90, midi_note_offset + 9, vel, 0);	//Explicitly write this gem using the MIDI note 9 higher than lane 1 gems (ie. 105 for expert difficulty)
						eof_add_midi_event(deltapos + deltalength, 0x80, midi_note_offset + 9, vel, 0);
					}
					else
					{
						vel = velocities[5];	//Restore the default velocity
						if(featurerestriction == 0)
						{	//Only write this notation if not writing a Rock Band compliant MIDI
							if(accent & 32)
							{	//If this gem is accented
								vel = accent_velocities[5];	//Use the applicable accent velocity
							}
							if(ghost & 32)
							{	//If this gem is ghosted
								vel = ghost_velocities[5];		//Use the applicable ghost velocity
							}
							eof_add_midi_event(deltapos, 0x90, midi_note_offset + 5, vel, 0);
							eof_add_midi_event(deltapos + deltalength, 0x80, midi_note_offset + 5, vel, 0);
						}
					}
				}

				/* write forced HOPO */
				if(noteflags & EOF_NOTE_FLAG_F_HOPO)
				{
					unsigned long markerlength = deltalength;	//By default, a forced HOPO marker will be the length of the affected note

					if(noteflags & EOF_NOTE_FLAG_CRAZY)
						markerlength = 1;	//But if the note has crazy status, the marker will be 1 tick, ensuring it can overlap notes without affecting their HOPO status

					if(isghl)
					{	//If writing a GHL format track
						eof_add_midi_event(deltapos, 0x90, midi_note_offset + 6, vel, 0);
						eof_add_midi_event(deltapos + markerlength, 0x80, midi_note_offset + 6, vel, 0);
					}
					else if(format == 1)
					{	//If writing the GHWT MIDI variant
///The correct notes for GHWT forced HOPOs are not confirmed
///						long pad = EOF_DEFAULT_TIME_DIVISION / 16;	//To make this notation more visible in Feedback, pad to a minimum length of 1/64 (in #/4 meter) if possible
/*						if(deltalength < pad)
						{	//If the note being exported is shorter than 1/64
							if(!nextdeltapos || (nextdeltapos > deltapos + pad))
							{	//If there is no next note or there is one and it is far away enough
								deltalength = pad;
							}
						}
						eof_add_midi_event(deltapos, 0x90, midi_note_offset + 9, vel, 0);	//Explicitly write this marker using the MIDI note 9 higher than lane 1 gems (ie. 105 for expert difficulty)
						eof_add_midi_event(deltapos + deltalength, 0x80, midi_note_offset + 9, vel, 0);
*/
					}
					else if(format == 2)
					{	//If writing the GH3 MIDI variant
						eof_add_midi_event(deltapos, 0x90, midi_note_offset + 5, vel, 0);
						eof_add_midi_event(deltapos, 0x80, midi_note_offset + 5, vel, 0);
					}
					else if(!format)
					{	//If writing the standard MIDI
						//thekiwimaddog indicated that Rock Band uses HOPO phrases per note/chord
						if(deltapos > 0)
						{	//Don't allow a number underflow
							eof_add_midi_event(deltapos - 1, 0x80, midi_note_offset + 6, vel, 0);	//Place a HOPO off end marker 1 delta before this just in case a HOPO off phrase is in effect (the overlap logic will filter this if it isn't necessary)
						}
						eof_add_midi_event(deltapos, 0x90, midi_note_offset + 5, vel, 0);
						eof_add_midi_event(deltapos + markerlength, 0x80, midi_note_offset + 5, vel, 0);
						eof_midi_event[eof_midi_events - 1]->length = markerlength;	//Have the event remember the marker's length to ensure eof_check_for_hopo_phrase_overlap() doesn't change the marker's length to zero
					}
				}

				/* write forced non-HOPO */
				else if(noteflags & EOF_NOTE_FLAG_NO_HOPO)
				{
					unsigned long markerlength = deltalength;	//By default, a forced HOPO off marker will be the length of the affected note

					if(noteflags & EOF_NOTE_FLAG_CRAZY)
						markerlength = 1;	//But if the note has crazy status, the marker will be 1 ticks, ensuring it can overlap notes without affecting their HOPO status

					if(isghl)
					{	//If writing a GHL format track
						eof_add_midi_event(deltapos, 0x90, midi_note_offset + 7, vel, 0);
						eof_add_midi_event(deltapos + markerlength, 0x80, midi_note_offset + 7, vel, 0);
					}
					else if(format == 1)
					{	//If writing the GHWT MIDI variant
						long pad = EOF_DEFAULT_TIME_DIVISION / 16;	//To make this notation more visible in Feedback, pad to a minimum length of 1/64 (in #/4 meter) if possible
						if(deltalength < pad)
						{	//If the note being exported is shorter than 1/64
							if(!nextdeltapos || (nextdeltapos > deltapos + pad))
							{	//If there is no next note or there is one and it is far away enough
								deltalength = pad;
							}
						}
						eof_add_midi_event(deltapos, 0x90, midi_note_offset + 9, vel, 0);	//Explicitly write this marker using the MIDI note 9 higher than lane 1 gems (ie. 105 for expert difficulty)
						eof_add_midi_event(deltapos + markerlength, 0x80, midi_note_offset + 9, vel, 0);
					}
					else
					{	//thekiwimaddog indicated that Rock Band uses HOPO phrases per note/chord
						if(deltapos > 0)
						{	//Don't allow a number underflow
							eof_add_midi_event(deltapos - 1, 0x80, midi_note_offset + 5, vel, 0);	//Place a HOPO on end marker 1 delta before this just in case a HOPO on phrase is in effect (the overlap logic will filter this if it isn't necessary)
						}
						eof_add_midi_event(deltapos, 0x90, midi_note_offset + 6, vel, 0);
						eof_add_midi_event(deltapos + markerlength, 0x80, midi_note_offset + 6, vel, 0);
						eof_midi_event[eof_midi_events - 1]->length = markerlength;	//Have the event remember the marker's length to ensure eof_check_for_hopo_phrase_overlap() doesn't change the marker's length to zero
					}
				}

				/* write flams */
				if(noteflags & EOF_DRUM_NOTE_FLAG_FLAM)
				{
					if(featurerestriction == 0)
					{	//Only write flam notation if not writing a Rock Band compliant MIDI
						eof_add_midi_event(deltapos, 0x90, 109, vel, 0);
						eof_add_midi_event(deltapos + deltalength, 0x80, 109, vel, 0);
					}
				}
			}//For each note in the track

			/* fill in star power */
			for(i = 0; i < eof_get_num_star_power_paths(sp, j); i++)
			{	//For each star power path in the track
				int marker = 116;	//In Rock Band, star power is marked with note 116

				sectionptr = eof_get_star_power_path(sp, j, i);
				deltapos = eof_ConvertToDeltaTime(sectionptr->start_pos, anchorlist, tslist, timedivision, 1, has_stored_tempo);	//Store the tick position of the phrase
				deltalength = eof_ConvertToDeltaTime(sectionptr->end_pos, anchorlist, tslist, timedivision, 0, 1) - deltapos;		//Store the number of delta ticks representing the phrase's length
				if(deltalength < 1)
				{	//If some kind of rounding error or other issue caused the delta length to be less than 1, force it to the minimum length of 1
					deltalength = 1;
				}
				if(format == 1)
				{	//If writing the GHWT MIDI variant
					marker = 106;	//Use this marker instead
				}
				eof_add_midi_event(deltapos, 0x90, marker, vel, 0);
				eof_add_midi_event(deltapos + deltalength, 0x80, marker, vel, 0);
			}

			/* fill in solos */
			if(format != 1)
			{	//If not writing the GHWT format MIDI
				for(i = 0; i < eof_get_num_solos(sp, j); i++)
				{	//For each solo in the track
					sectionptr = eof_get_solo(sp, j, i);
					deltapos = eof_ConvertToDeltaTime(sectionptr->start_pos, anchorlist, tslist, timedivision, 1, has_stored_tempo);	//Store the tick position of the phrase
					deltalength = eof_ConvertToDeltaTime(sectionptr->end_pos, anchorlist, tslist, timedivision, 0, 1) - deltapos;		//Store the number of delta ticks representing the phrase's length
					if(deltalength < 1)
					{	//If some kind of rounding error or other issue caused the delta length to be less than 1, force it to the minimum length of 1
						deltalength = 1;
					}
					eof_add_midi_event(deltapos, 0x90, 103, vel, 0);
					eof_add_midi_event(deltapos + deltalength, 0x80, 103, vel, 0);
				}
			}

			/* fill in tremolos */
			for(i = 0; i < eof_get_num_tremolos(sp, j); i++)
			{	//For each tremolo in the track
				sectionptr = eof_get_tremolo(sp, j, i);
				if(sectionptr->difficulty == 0xFF)
				{	//Only export a tremolo section to MIDI if it applies to all difficulties
					deltapos = eof_ConvertToDeltaTime(sectionptr->start_pos, anchorlist, tslist, timedivision, 1, has_stored_tempo);	//Store the tick position of the phrase
					deltalength = eof_ConvertToDeltaTime(sectionptr->end_pos, anchorlist, tslist, timedivision, 0, 1) - deltapos;		//Store the number of delta ticks representing the phrase's length
					if(deltalength < 1)
					{	//If some kind of rounding error or other issue caused the delta length to be less than 1, force it to the minimum length of 1
						deltalength = 1;
					}
					eof_add_midi_event(deltapos, 0x90, 126, vel, 0);	//Note 126 denotes a tremolo marker
					eof_add_midi_event(deltapos + deltalength, 0x80, 126, vel, 0);
				}
			}

			/* fill in trills */
			for(i = 0; i < eof_get_num_trills(sp, j); i++)
			{	//For each trill in the track
				sectionptr = eof_get_trill(sp, j, i);
				deltapos = eof_ConvertToDeltaTime(sectionptr->start_pos, anchorlist, tslist, timedivision, 1, has_stored_tempo);	//Store the tick position of the phrase
				deltalength = eof_ConvertToDeltaTime(sectionptr->end_pos, anchorlist, tslist, timedivision, 0, 1) - deltapos;		//Store the number of delta ticks representing the phrase's length
				if(deltalength < 1)
				{	//If some kind of rounding error or other issue caused the delta length to be less than 1, force it to the minimum length of 1
					deltalength = 1;
				}
				eof_add_midi_event(deltapos, 0x90, 127, vel, 0);	//Note 127 denotes a trill marker
				eof_add_midi_event(deltapos + deltalength, 0x80, 127, vel, 0);
			}

			/* fill in sliders */
			if(featurerestriction == 0)
			{	//Only write slider notation if not writing a Rock Band compliant MIDI
				for(i = 0; i < eof_get_num_sliders(sp, j); i++)
				{	//For each slider in the track
					sectionptr = eof_get_slider(sp, j, i);
					deltapos = eof_ConvertToDeltaTime(sectionptr->start_pos, anchorlist, tslist, timedivision, 1, has_stored_tempo);	//Store the tick position of the phrase
					deltalength = eof_ConvertToDeltaTime(sectionptr->end_pos, anchorlist, tslist, timedivision, 0, 1) - deltapos;		//Store the number of delta ticks representing the phrase's length
					if(deltalength < 1)
					{	//If some kind of rounding error or other issue caused the delta length to be less than 1, force it to the minimum length of 1
						deltalength = 1;
					}
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tAdding slider #%lu: Start = %lums, stop = %lums (%lu ticks to %lu ticks)", i, sectionptr->start_pos, sectionptr->end_pos, deltapos, deltapos + deltalength);
					eof_log(eof_log_string, 2);
					if(format == 1)
					{	//If writing the GHWT MIDI variant
						eof_add_midi_event(deltapos, 0x90, 103, vel, 0);	//Use note 103 to mark the slider
						eof_add_midi_event(deltapos + deltalength, 0x80, 103, vel, 0);
					}
					else
					{
						eof_add_phase_shift_sysex_phrase(deltapos, deltapos + deltalength, 0xFF, 4);	//Write custom slider phrase markers
					}
				}
			}

			if(eof_midi_event_full)
			{	//If the track exceeded the number of MIDI events that could be written
				allegro_message("Error:  Too many MIDI events, aborting MIDI export.");
				eof_log("Error:  Too many MIDI events, aborting MIDI export.", 1);
				eof_destroy_ks_list(kslist);		//Free memory used by the KS change list
				eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
				eof_destroy_ts_list(tslist);		//Free memory used by the TS change list
				eof_clear_midi_events();			//Free any memory allocated for the MIDI event array
				return 0;	//Return failure
			}

			for(channelctr = 0; channelctr < 16; channelctr++)
			{	//For each of the usable MIDI channels
				for(i = 0; i < 128; i++)
				{	//Ensure that any notes that are still on are terminated
					if(eof_midi_note_status[channelctr][i] == 0)
						continue;	//If this note was not left on, skip it

					//Otherwise send an alert message, as this is abnormal
					allegro_message("MIDI export error:  Note %lu was not turned off", i);
					eof_log("MIDI export error:  Note %lu was not turned off", 1);
					if(eof_midi_endbeatnum)
					{	//If the chart has a manually defined end event, that's probably the cause
						eof_log("\tend event was manually defined", 1);
						eof_clear_input();
						if(alert("It appears this is due to an [end] event that cuts out a note early", "Would you like to seek to the beat containing this [end] event?", NULL, "&Yes", "&No", 'y', 'n') == 1)
						{	//If user opts to seek to the offending event
							eof_set_seek_position(sp->beat[eof_midi_endbeatnum]->pos + eof_av_delay);
							eof_selected_beat = eof_midi_endbeatnum;
						}
					}
					eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
					eof_destroy_ts_list(tslist);		//Free memory used by the TS change list
					eof_clear_midi_events();			//Free any memory allocated for the MIDI event array
					eof_destroy_ks_list(kslist);		//Free memory used by the KS change list
					return 0;	//Return failure
				}
			}
			qsort(eof_midi_event, (size_t)eof_midi_events, sizeof(EOF_MIDI_EVENT *), qsort_helper3);
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Post-sort event content:  %lu events", eof_midi_events);
			eof_log(eof_log_string, 3);
			eof_log_midi_event_list();

			eof_check_for_note_overlap();	//Filter out any improperly overlapping note on/off events
			eof_check_for_hopo_phrase_overlap();	//Ensure that no HOPO on/off phrases start/end at the same delta position as each other
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Post-overlap removal event content:  %lu events", eof_midi_events);
			eof_log(eof_log_string, 3);
			eof_log_midi_event_list();

			qsort(eof_midi_event, (size_t)eof_midi_events, sizeof(EOF_MIDI_EVENT *), qsort_helper3);	//Re-sort, since the previous function may have changed the events' order
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Post-sort event content:  %lu events", eof_midi_events);
			eof_log(eof_log_string, 3);
			eof_log_midi_event_list();
//			allegro_message("break1");

			for(trackctr = 0; trackctr <= expertplus; trackctr++)
			{	//This loop makes a second pass to write the expert+ drum MIDI if applicable
				int lastevent = 0;	//Track the last event written so running status can be utilized
				char *exportname = sp->track[j]->name;	//By default, the track's regular name is used for export

				if(isghl)
				{	//GHL tracks export with a " GHL" suffix by default
					(void) snprintf(ghlname, sizeof(ghlname) - 1, "%s GHL", exportname);
					exportname = ghlname;
				}

				if((sp->track[j]->flags & EOF_TRACK_FLAG_ALT_NAME) && (sp->track[j]->altname[0] != '\0'))
				{	//If the track has been renamed
					exportname = sp->track[j]->altname;	//Allow that name to export to the MIDI track
				}

				/* open the file */
				if(trackctr == 0)	//Writing the normal temp file
					fp = pack_fopen(notetempname[j], "w");
				else
				{	//Write the Expert+ temp file
					fp = pack_fopen(expertplustempname, "w");
					for(i = 0; i < eof_midi_events; i++)
					{	//Change all the double bass note events (note 95) to regular bass for the Expert+ track
						if(eof_midi_event[i]->note == 95)
							eof_midi_event[i]->note = 96;
					}
				}

				if(!fp)
				{
					eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
					eof_destroy_ts_list(tslist);	//Free memory used by the TS change list
					eof_destroy_ks_list(kslist);		//Free memory used by the KS change list
					eof_clear_midi_events();			//Free any memory allocated for the MIDI event array
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError saving:  Cannot open temporary MIDI track:  \"%s\"", strerror(errno));	//Get the Operating System's reason for the failure
					eof_log(eof_log_string, 1);
					return 0;	//Return failure
				}

				/* write the track name */
				WriteVarLen(0, fp);
				(void) pack_putc(0xFF, fp);
				(void) pack_putc(0x03, fp);
				WriteVarLen(ustrsize(exportname), fp);
				(void) pack_fwrite(exportname, ustrsize(exportname), fp);

				/* add MIDI events */
				lastdelta = 0;
				for(i = 0; i < eof_midi_events; i++)
				{
					if((trackctr == 1) && (eof_midi_event[i]->note < 96))
						continue;	//Filter out all non Expert drum notes for the Expert+ track

					if(eof_midi_event[i]->filtered)
						continue;	//If this event is filtered, skip it

					delta = eof_midi_event[i]->pos;
					if(eof_midi_event[i]->type == 0x01)
					{	//Text event
						eof_write_text_event(delta-lastdelta, eof_midi_event[i]->dp, fp);
						lastevent = 0;	//Reset running status after a meta/sysex event
					}
					else if(eof_midi_event[i]->type == 0xF0)
					{	//Sysex message
						WriteVarLen(delta - lastdelta, fp);			//Write this event's relative delta time
						(void) pack_putc(0xF0, fp);					//Sysex event
						WriteVarLen(eof_midi_event[i]->note, fp);	//Write the Sysex message's size
						(void) pack_fwrite(eof_midi_event[i]->dp, eof_midi_event[i]->note, fp);	//Write the Sysex data
						lastevent = 0;	//Reset running status after a meta/sysex event
					}
					else
					{	//Note on/off
						WriteVarLen(delta - lastdelta, fp);	//Write this event's relative delta time
						if(eof_midi_event[i]->type != lastevent)
						{	//With running status, the MIDI event type needn't be written if it's the same as the previous event
							(void) pack_putc(eof_midi_event[i]->type, fp);
						}
						(void) pack_putc(eof_midi_event[i]->note, fp);
						(void) pack_putc(eof_midi_event[i]->velocity, fp);
						lastevent = eof_midi_event[i]->type;
					}
					if(eof_midi_event[i]->allocation && eof_midi_event[i]->dp)
					{	//If this event has allocated memory to release
						free(eof_midi_event[i]->dp);	//Free it now
						eof_midi_event[i]->dp = NULL;
						eof_midi_event[i]->allocation = 0;
					}
					lastdelta = delta;					//Store this event's absolute delta time
				}

				/* end of track */
				WriteVarLen(0, fp);
				(void) pack_putc(0xFF, fp);
				(void) pack_putc(0x2F, fp);
				(void) pack_putc(0x00, fp);
				(void) pack_fclose(fp);

				if(trackctr == 1)
				{	//If the Expert+ track data was written
					expertplus = 0;	//Reset this status
					expertpluswritten = 1;
				}
			}
		}//If this is a legacy track

		else if(sp->track[j]->track_format == EOF_VOCAL_TRACK_FORMAT)
		{	//If this is a vocal track
			#define EOF_LYRIC_PHRASE_PADDING 0
			///EOF_LYRIC_PHRASE_PADDING was originally implemented with a value of 5 for the sake of working around bugs in FoFiX, but unless modern rhythm games are found to have similar needs, it isn't worth maintaining this legacy workaround
			unsigned long last_phrase = 0;	//Stores the absolute delta time of the last Note 105 On
			int lastevent = 0;	//Track the last event written so running status can be utilized
			char *exportname = sp->track[j]->name;	//By default, the track's regular name is used for export

			if((sp->track[j]->flags & EOF_TRACK_FLAG_ALT_NAME) && (sp->track[j]->altname[0] != '\0'))
			{	//If the track has been renamed
				exportname = sp->track[j]->altname;	//Allow that name to export to the MIDI track
			}

			/* make vocals track */
			/* insert the missing lyric phrases if the user opted to do so */
			if(fixvoxphrases)
			{
				char phrase_in_progress = 0;	//This will be used to track the open/closed status of the automatic phrases, so adjacent lyrics/percussions can be added to the same auto-generated phrase
				unsigned long phrase_start = 0;	//This will store the delta time of the last opened lyric phrase
				for(ctr = 0; ctr < sp->vocal_track[tracknum]->lyrics; ctr++)
				{
					if(eof_find_lyric_line(ctr) != NULL)
						continue;	//If this lyric is in a line or is a vocal percussion note, skip it

					//Otherwise write the MIDI events for a line phrase to envelop it
					if(!phrase_in_progress)
					{	//If a note on for the phrase hasn't been added already
						phrase_start = eof_ConvertToDeltaTime(sp->vocal_track[tracknum]->lyric[ctr]->pos, anchorlist, tslist, timedivision, 1, has_stored_tempo);	//Store the tick position of the phrase
						eof_add_midi_event(phrase_start, 0x90, 105, vel, 0);
						phrase_in_progress = 1;
					}
					if(!((ctr + 1 < sp->vocal_track[tracknum]->lyrics) && (eof_find_lyric_line(ctr + 1) == NULL)))
					{	//Only if there isn't a next lyric that is also missing a vocal phrase, write the note off for the phrase
						deltalength = eof_ConvertToDeltaTime(sp->vocal_track[tracknum]->lyric[ctr]->pos + sp->vocal_track[tracknum]->lyric[ctr]->length, anchorlist, tslist, timedivision, 0, has_stored_tempo) - phrase_start;	//Store the number of delta ticks representing the phrase's length
						if(deltalength < 1)
						{	//If some kind of rounding error or other issue caused the delta length to be less than 1, force it to the minimum length of 1
							deltalength = 1;
						}
						eof_add_midi_event(phrase_start + deltalength, 0x80, 105, vel, 0);
						phrase_in_progress = 0;
					}
				}
			}

			/* write the MTrk MIDI data to a temp file
			use size of the file as the MTrk header length */
			for(i = 0; i < sp->vocal_track[tracknum]->lyrics; i++)
			{	//For each lyric
				//Copy each lyric string into a new array, perform correction on it if necessary
				tempstring = malloc(sizeof(sp->vocal_track[tracknum]->lyric[i]->text));
				if(tempstring == NULL)	//If allocation failed
				{
					eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
					eof_destroy_ts_list(tslist);		//Free memory used by the TS change list
					eof_destroy_ks_list(kslist);		//Free memory used by the KS change list
					eof_clear_midi_events();			//Free any memory allocated for the MIDI event array
					eof_log("\tError saving:  Cannot allocate memory", 1);
					return 0;			//Return failure
				}
				sp->vocal_track[tracknum]->lyric[i]->text[EOF_MAX_LYRIC_LENGTH] = '\0';	//Guarantee that the lyric string is terminated
				memcpy(tempstring,sp->vocal_track[tracknum]->lyric[i]->text,sizeof(sp->vocal_track[tracknum]->lyric[i]->text));	//Copy to new array

				deltapos = eof_ConvertToDeltaTime(sp->vocal_track[tracknum]->lyric[i]->pos, anchorlist, tslist, timedivision, 1, has_stored_tempo);
				deltalength = eof_ConvertToDeltaTime(sp->vocal_track[tracknum]->lyric[i]->pos + sp->vocal_track[tracknum]->lyric[i]->length, anchorlist, tslist, timedivision, 0, has_stored_tempo) - deltapos;
				if(deltalength < 1)
				{	//If some kind of rounding error or other issue caused the delta length to be less than 1, force it to the minimum length of 1
					deltalength = 1;
				}
				if(sp->vocal_track[tracknum]->lyric[i]->note > 0)
				{	//If this lyric has a pitch definition (or is explicitly pitchless)
					eof_add_midi_event(deltapos, 0x90, sp->vocal_track[tracknum]->lyric[i]->note, vel, 0);
					eof_add_midi_event(deltapos + deltalength, 0x80, sp->vocal_track[tracknum]->lyric[i]->note, vel, 0);
				}
				else if(fixvoxpitches)
				{	//If performing pitchless lyric correction, write pitch 50 instead to guarantee it is usable as a freestyle lyric
					eof_add_midi_event(deltapos, 0x90, 50, vel, 0);
					eof_add_midi_event(deltapos + deltalength, 0x80, 50, vel, 0);
					eof_set_freestyle(tempstring,1);		//Ensure the lyric properly ends with a freestyle character
				}

				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tWriting lyric:  Pos = %lums\tLength = %ld\tText = \"%s\"", sp->vocal_track[tracknum]->lyric[i]->pos, sp->vocal_track[tracknum]->lyric[i]->length, tempstring);
				eof_log(eof_log_string, 2);

				//Write the string, which was only corrected if fixvoxpitches was nonzero and the pitch was not defined
				if(sp->vocal_track[tracknum]->lyric[i]->note != VOCALPERCUSSION)
				{	//Do not write a lyric string for vocal percussion notes
					eof_add_midi_lyric_event(eof_ConvertToDeltaTime(sp->vocal_track[tracknum]->lyric[i]->pos, anchorlist, tslist, timedivision, 1, has_stored_tempo), tempstring, 1);	//Track that the lyric text was dynamically allocated
				}
				else
				{
					free(tempstring);
				}
			}//For each lyric

			/* fill in lyric lines */
			for(i = 0; i < sp->vocal_track[tracknum]->lines; i++)
			{
				deltapos = eof_ConvertToDeltaTime(sp->vocal_track[tracknum]->line[i].start_pos, anchorlist, tslist, timedivision, 1, has_stored_tempo);
				deltalength = eof_ConvertToDeltaTime(sp->vocal_track[tracknum]->line[i].end_pos, anchorlist, tslist, timedivision, 0, has_stored_tempo) - deltapos;
				if(deltalength < 1)
				{	//If some kind of rounding error or other issue caused the delta length to be less than 1, force it to the minimum length of 1
					deltalength = 1;
				}
				eof_add_midi_event(deltapos, 0x90, 105, vel, 0);
				eof_add_midi_event(deltapos + deltalength, 0x80, 105, vel, 0);
				if(sp->vocal_track[tracknum]->line[i].flags & EOF_LYRIC_LINE_FLAG_OVERDRIVE)
				{	//For the vocal track, store the converted delta times, to allow for artificial padding for lyric phrase markers
					eof_add_midi_event(eof_ConvertToDeltaTime(sp->vocal_track[tracknum]->line[i].start_pos, anchorlist, tslist, timedivision, 1, has_stored_tempo), 0x90, 116, vel, 0);
					eof_add_midi_event(eof_ConvertToDeltaTime(sp->vocal_track[tracknum]->line[i].end_pos, anchorlist, tslist, timedivision, 0, has_stored_tempo), 0x80, 116, vel, 0);
				}
			}

			/* insert padding as necessary between a lyric phrase on marker and the following lyric */
			qsort(eof_midi_event, (size_t)eof_midi_events, sizeof(EOF_MIDI_EVENT *), qsort_helper3);	//Lyric events must be sorted for padding logic to work
			for(i = 0; i < eof_midi_events; i++)
			{
				if(eof_midi_event[i]->filtered)
					continue;	//If this event is filtered, skip it

				if((eof_midi_event[i]->on) && (eof_midi_event[i]->note == 105))
				{	//If this is a lyric on phrase marker
					last_phrase = eof_midi_event[i]->pos;		//Store its position
				}
				else if((i + 2 < eof_midi_events) && (eof_midi_event[i]->type == 0x05) && (eof_midi_event[i+1]->on) && (eof_midi_event[i+1]->note < 105) && (eof_midi_event[i+2]->off) && (eof_midi_event[i+2]->note < 105))
				{	//If this is a lyric event followed by a lyric pitch on and off
					if((eof_midi_event[i]->pos == eof_midi_event[i+1]->pos) && (eof_midi_event[i]->pos < last_phrase + EOF_LYRIC_PHRASE_PADDING))
					{	//If the lyric event and pitch are not at least EOF_LYRIC_PHRASE_PADDING deltas away from the lyric phrase on marker
						if(last_phrase + EOF_LYRIC_PHRASE_PADDING < eof_midi_event[i+2]->pos)
						{	//If the lyric event and pitch can be padded without overlapping the pitch off note, do it
							eof_midi_event[i]->pos = last_phrase + EOF_LYRIC_PHRASE_PADDING;	//Change the delta time of the lyric event
							eof_midi_event[i+1]->pos = last_phrase + EOF_LYRIC_PHRASE_PADDING;	//Change the delta time of the note on event
						}
					}
				}
				else if((i + 1 < eof_midi_events) && (eof_midi_event[i]->on) && (eof_midi_event[i]->note == VOCALPERCUSSION) && (eof_midi_event[i+1]->off) && (eof_midi_event[i+1]->note == VOCALPERCUSSION))
				{	//If this is a vocal percussion note on followed by a vocal percussion off
					if(eof_midi_event[i]->pos < last_phrase + EOF_LYRIC_PHRASE_PADDING)
					{	//If the vocal percussion on is not at least EOF_LYRIC_PHRASE_PADDING deltas away from the lyric phrase on marker
						if(last_phrase + EOF_LYRIC_PHRASE_PADDING < eof_midi_event[i+1]->pos)
						{	//If the vocal percussion on can be padded without overlapping the vocal percussion off note, do it
							eof_midi_event[i]->pos = last_phrase + EOF_LYRIC_PHRASE_PADDING;	//Change the delta time of the vocal percussion event
						}
					}
				}
			}

			if(eof_midi_event_full)
			{	//If the track exceeded the number of MIDI events that could be written
				allegro_message("Error:  Too many MIDI events, aborting MIDI export.");
				eof_log("Error:  Too many MIDI events, aborting MIDI export.", 1);
				eof_destroy_ks_list(kslist);		//Free memory used by the KS change list
				eof_destroy_ts_list(tslist);		//Free memory used by the TS change list
				eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
				eof_clear_midi_events();			//Free any memory allocated for the MIDI event array
				return 0;	//Return failure
			}

			for(channelctr = 0; channelctr < 16; channelctr++)
			{	//For each of the usable MIDI channels
				for(i = 0;i < 128; i++)
				{	//Ensure that any notes that are still on are terminated
					if(eof_midi_note_status[channelctr][i] == 0)
						continue;	//If this note was not left on, skip it

					//Otherwise send an alert message, as this is abnormal
					allegro_message("MIDI export error:  Note %lu was not turned off", i);
					eof_log("MIDI export error:  Note %lu was not turned off", 1);
					if(eof_midi_endbeatnum)
					{	//If the chart has a manually defined end event, that's probably the cause
						eof_log("\tend event was manually defined", 1);
						eof_clear_input();
						if(alert("It appears this is due to an [end] event that cuts out a note early", "Would you like to seek to the beat containing this [end] event?", NULL, "&Yes", "&No", 'y', 'n') == 1)
						{	//If user opts to seek to the offending event
							eof_set_seek_position(sp->beat[eof_midi_endbeatnum]->pos + eof_av_delay);
							eof_selected_beat = eof_midi_endbeatnum;
						}
					}
					eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
					eof_destroy_ts_list(tslist);		//Free memory used by the TS change list
					eof_clear_midi_events();			//Free any memory allocated for the MIDI event array
					eof_destroy_ks_list(kslist);		//Free memory used by the KS change list
					return 0;	//Return failure
				}
			}
			qsort(eof_midi_event, (size_t)eof_midi_events, sizeof(EOF_MIDI_EVENT *), qsort_helper3);
			eof_check_for_note_overlap();	//Filter out any improperly overlapping note on/off events
			/* open the file */
			fp = pack_fopen(notetempname[j], "w");
			if(!fp)
			{
				eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
				eof_destroy_ts_list(tslist);		//Free memory used by the TS change list
				eof_destroy_ks_list(kslist);		//Free memory used by the KS change list
				eof_clear_midi_events();			//Free any memory allocated for the MIDI event array
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError saving:  Cannot open temporary MIDI track:  \"%s\"", strerror(errno));	//Get the Operating System's reason for the failure
				eof_log(eof_log_string, 1);
				return 0;	//Return failure
			}

			/* write the track name */
			WriteVarLen(0, fp);
			(void) pack_putc(0xFF, fp);
			(void) pack_putc(0x03, fp);
			WriteVarLen(ustrsize(exportname), fp);
			(void) pack_fwrite(exportname, ustrsize(exportname), fp);

			/* add MIDI events */
			lastdelta = 0;
			for(i = 0; i < eof_midi_events; i++)
			{
				if(eof_midi_event[i]->filtered)
					continue;	//If this event is filtered, skip it

				delta = eof_midi_event[i]->pos;
				if(eof_midi_event[i]->type == 0x01)
				{	//Text event
					eof_write_text_event(delta-lastdelta, eof_midi_event[i]->dp, fp);
					lastevent = 0;	//Reset running status after a meta/sysex event
				}
				else if(eof_midi_event[i]->type == 0x05)
				{	//Lyric event
					eof_write_lyric_event(delta-lastdelta, eof_midi_event[i]->dp, fp);
					lastevent = 0;	//Reset running status after a meta/sysex event
				}
				else
				{
					WriteVarLen(delta - lastdelta, fp);	//Write this lyric's relative delta time
					if(eof_midi_event[i]->type != lastevent)
					{	//With running status, the MIDI event type needn't be written if it's the same as the previous event
						(void) pack_putc(eof_midi_event[i]->type, fp);
					}
					(void) pack_putc(eof_midi_event[i]->note, fp);
					(void) pack_putc(eof_midi_event[i]->velocity, fp);
					lastevent = eof_midi_event[i]->type;
				}
				if(eof_midi_event[i]->allocation && eof_midi_event[i]->dp)
				{	//If this event has allocated memory to release
					free(eof_midi_event[i]->dp);	//Free it now
					eof_midi_event[i]->dp = NULL;
					eof_midi_event[i]->allocation = 0;
				}
				lastdelta=delta;					//Store this lyric's absolute delta time
			}

			/* end of track */
			WriteVarLen(0, fp);
			(void) pack_putc(0xFF, fp);
			(void) pack_putc(0x2F, fp);
			(void) pack_putc(0x00, fp);
			(void) pack_fclose(fp);
		}//If this is a vocal track

		else if(eof_track_is_pro_guitar_track(sp, j))
		{	//If this is a pro guitar track
			/* prepare fret hand positions */
			unsigned long count;
			unsigned char last_written_hand_pos = 0;	//Tracks the last written hand position, so duplicate positions needn't be written to MIDI
			EOF_PRO_GUITAR_TRACK *tp = sp->pro_guitar_track[tracknum];
			int lastevent = 0;	//Track the last event written so running status can be utilized

			fret_hand_pos_written = 0;			//Reset these statuses
			fret_hand_positions_generated = 0;
			fret_hand_positions_present = 0;

			for(ctr = 0, count = 0; ctr < tp->handpositions; ctr++)
			{	//For each hand position in this track
				if(tp->handposition[ctr].difficulty == EOF_NOTE_AMAZING)
				{	//If this hand position exists in the Expert difficulty
					count++;
					fret_hand_positions_present = 1;	//Track that there is at least one fret hand position defined
				}
			}
			if(!count)
			{	//If there are no user-defined hand positions in Expert difficulty
				eof_fret_hand_position_list_dialog_undo_made = 1;	//Ensure no undo state is written during export
				eof_generate_efficient_hand_positions(sp, j, EOF_NOTE_AMAZING, 0, 1);	//Generate the fret hand positions (dynamic fret range tolerances) for the track difficulty being currently written
				for(ctr = 0, count = 0; ctr < tp->handpositions; ctr++)	//Re-count the hand positions
				{	//For each hand position in this track
					if(tp->handposition[ctr].difficulty == EOF_NOTE_AMAZING)
					{	//If this hand position exists in the Expert difficulty
						count++;
						fret_hand_positions_present = 1;	//Track that there is at least one fret hand position defined
					}
				}
				if(count)
				{	//If fret hand positions are present, write them to MIDI
					fret_hand_positions_generated = 1;	//Track that fret hand positions were generated so they can be removed after writing to MIDI
				}
				else
				{	//Fret hand positions couldn't be generated
					for(ctr = 0, count = 0; ctr < tp->notes; ctr++)
					{	//For each note in this track
						if(tp->note[ctr]->type == EOF_NOTE_AMAZING)
						{	//If this note exists in the Expert difficulty
							count++;
						}
					}
					if(count)
					{	//If there was at least one note in the Expert difficulty, and fret hand positions couldn't be generated automatically, alert the user
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Error:  Failed to automatically generate fret hand positions for \"%s\" during MIDI export.", sp->track[j]->name);
						eof_log(eof_log_string, 1);
						allegro_message("%s", eof_log_string);
					}
				}
			}

			/* fill in notes */
			/* write the MTrk MIDI data to a temp file
			use size of the file as the MTrk header length */
			for(i = 0; i < eof_get_track_size(sp, j); i++)
			{	//For each note in the track
				type = eof_get_note_type(sp, j, i);
				switch(type)
				{
					case EOF_NOTE_AMAZING:	//notes 96-101
					{
						midi_note_offset = 96;
						break;
					}
					case EOF_NOTE_MEDIUM:	//notes 72-77
					{
						midi_note_offset = 72;
						break;
					}
					case EOF_NOTE_EASY:		//notes 48-58
					{
						midi_note_offset = 48;
						break;
					}
					case EOF_NOTE_SUPAEASY:	//notes 24-29
					{
						midi_note_offset = 24;
						break;
					}
					case EOF_NOTE_SPECIAL:	//BRE fill: notes 120-125
					{
						midi_note_offset = 120;
						break;
					}
					default:	//Other note difficulties aren't supported for MIDI export
					{
						continue;
					}
				}

				if((sp->track[j]->flags & EOF_TRACK_FLAG_UNLIMITED_DIFFS) && (type == EOF_NOTE_SPECIAL))
				{	//If this note is a BRE note and the track's difficulty limit was removed
					continue;	//Skip writing this note
				}

				noteflags = eof_get_note_flags(sp, j, i);	//Store the note flags for easier use
				note = eof_get_note_note(sp, j, i);			//Store the note bitflag for easier use
				notepos = eof_get_note_pos(sp, j, i);		//Store the note position for easier use
				length = eof_get_note_length(sp, j, i);		//Store the note length for easier use
				currentname = eof_get_note_name(sp, j, i);

				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tWriting note:  Pos = %lums\tLength = %ld\tMask = %lu", notepos, length, note);
				eof_log(eof_log_string, 2);

				deltapos = eof_ConvertToDeltaTime(notepos, anchorlist, tslist, timedivision, 1, has_stored_tempo);	//Store the tick position of the note
				deltalength = eof_ConvertToDeltaTime(notepos + length, anchorlist, tslist, timedivision, 0, has_stored_tempo) - deltapos;	//Store the number of delta ticks representing the note's length
				if(deltalength < 1)
				{	//If some kind of rounding error or other issue caused the delta length to be less than 1, force it to the minimum length of 1
					deltalength = 1;
				}
				if(currentname && (currentname[0] != '\0'))
				{	//If this note has a name
					if(type <= EOF_NOTE_AMAZING)
					{	//only write names for the 4 difficulties, don't for BRE notes
						(void) snprintf(chordname, sizeof(chordname) - 1, "[chrd%d %s]", type, currentname);	//Build the chord name text event as per RB3's convention
						tempstring = malloc((size_t)ustrsizez(chordname));
						if(tempstring != NULL)
						{	//If allocation was successful
							memcpy(tempstring, chordname, (size_t)ustrsizez(chordname));	//Copy the string to the newly allocated memory
							eof_add_midi_text_event(deltapos, tempstring, 1, 0xFFFFFFFF);	//Store the new string in a text event, send 1 for the allocation flag, because the text string is being stored in dynamic memory (provide a high index to ensure it doesn't influence sort order)
						}
					}
				}

				/* write note gems */
				for(ctr = 0, bitmask = 1; ctr < 6; ctr++, bitmask <<= 1)
				{	//For each of the 6 usable strings
					if(noteflags & EOF_PRO_GUITAR_NOTE_FLAG_TAP)
					{	//Tapped notes are written on channel 4
						channel = 4;
					}
					else if((noteflags & EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE) || (sp->pro_guitar_track[tracknum]->note[i]->frets[ctr] & 0x80))
					{	//Mute gems are written on channel 3
						channel = 3;
					}
					else if(sp->pro_guitar_track[tracknum]->note[i]->ghost & bitmask)
					{	//Ghost note gems are written on channel 1
						channel = 1;
					}
					else
					{	//Normal gems are written on channel 0
						channel = 0;
					}

					if(sp->pro_guitar_track[tracknum]->note[i]->frets[ctr] == 0xFF)
					{	//If this is a muted gem with no fret specified
						velocity = 100;	//Write it as a muted note at fret 0
					}
					else
					{	//Otherwise write it normally
						velocity = (sp->pro_guitar_track[tracknum]->note[i]->frets[ctr] & 0x7F) + 100;	//Velocity (100 + X) represents fret # X (mask out the MSB, which is the mute status)
					}
					if(note & bitmask)
					{	//If the note uses this string
						eof_add_midi_event(deltapos, 0x90, midi_note_offset + ctr, velocity, channel);	//Write the note on event
						eof_add_midi_event(deltapos + deltalength, 0x80, midi_note_offset + ctr, velocity, channel);	//Write the note off event
					}
				}

				/* write hammer on/pull off */
				if((noteflags & EOF_PRO_GUITAR_NOTE_FLAG_HO) || (noteflags & EOF_PRO_GUITAR_NOTE_FLAG_PO))
				{	//If this note is marked as a hammer on or pull off (RB3 marks them both the same way)
					eof_add_midi_event(deltapos, 0x90, midi_note_offset + 6, 96, channel);	//Forced HO or PO markers are note # (lane 1 + 6)
					eof_add_midi_event(deltapos + deltalength, 0x80, midi_note_offset + 6, velocity, channel);
				}

				/* write slide sections */
				//First, determine if the note slides, and if so, in what direction
				slideup = 0;	//Reset these statuses
				slidedown = 0;
				if(noteflags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP)
				{	//If the note explicitly slides up
					slideup = 1;
				}
				else if(noteflags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN)
				{	//If the note explicitly slides down
					slidedown = 1;
				}
				else if(noteflags & EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE)
				{	//If the note has an unpitched slide
					unsigned char lowestfret = eof_get_lowest_fret_value(sp, j, i);	//Determine the fret value of the note's lowest fretted string
					if(lowestfret < tp->note[i]->unpitchend)
					{	//If the unpitched slide goes higher than this position
						slideup = 1;
					}
					else if(lowestfret > tp->note[i]->unpitchend)
					{	//If the unpitched slide goes lower than this position
						slidedown = 1;
					}
				}
				if(slideup || slidedown)
				{	//If this note slides up or down
//	The correct method to mark slides in RB3 has been found, and works (mostly), the sysex method is deprecated and will remain exported for the time being
					int slidechannel = 0;	//By default, slides are written over channel 0

					if(featurerestriction == 0)
					{	//Only write the slide Sysex notation if not writing a Rock Band compliant MIDI
						unsigned char phraseid = slideup ? 2 : 3;	//Set based on slide up (phrase id 2) or slide down (phrase id 3) appropriately
						eof_add_phase_shift_sysex_phrase(deltapos, deltapos + deltalength, type, phraseid);	//Write custom pro guitar slide phrase markers
					}

					if(noteflags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_REVERSE)
					{	//If this slide is to be written to indicate it is reversed
						slidechannel = 11;	//It must be written over channel 11
					}
					if(slidedown)
					{	//If this note slides down
						eof_add_midi_event(deltapos, 0x90, midi_note_offset + 7, 108, slidechannel);	//Slide markers are note # (lane 1 + 7).  Fret 8 or higher triggers a down slide in RB3
						eof_add_midi_event(deltapos + deltalength, 0x80, midi_note_offset + 7, 108, slidechannel);
					}
					else if(slideup)
					{	//If this note slides up
						eof_add_midi_event(deltapos, 0x90, midi_note_offset + 7, 107, slidechannel);	//Fret 7 or lower triggers an up slide in RB3
						eof_add_midi_event(deltapos + deltalength, 0x80, midi_note_offset + 7, 107, slidechannel);
					}
				}

				/* write palm mute marker */
				if(noteflags & EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE)
				{	//If this note has palm mute status
					if(featurerestriction == 0)
					{	//Only write the palm mute Sysex notation if not writing a Rock Band compliant MIDI
						eof_add_phase_shift_sysex_phrase(deltapos, deltapos + deltalength, type, 9);	//Write custom pro guitar palm mute phrase markers
					}
				}

				/* write vibrato marker */
				if(noteflags & EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO)
				{	//If this note has vibrato status
					if(featurerestriction == 0)
					{	//Only write the vibrato Sysex notation if not writing a Rock Band compliant MIDI
						eof_add_phase_shift_sysex_phrase(deltapos, deltapos + deltalength, type, 10);	//Write custom pro guitar vibrato phrase markers
					}
				}

				/* write harmonic marker */
				if(noteflags & EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC)
				{	//If this note has harmonic status
					if(featurerestriction == 0)
					{	//Only write the harmonic Sysex notation if not writing a Rock Band compliant MIDI
						eof_add_phase_shift_sysex_phrase(deltapos, deltapos + deltalength, type, 11);	//Write custom pro guitar harmonic phrase markers
					}
				}

				/* write pinch harmonic marker */
				if(noteflags & EOF_PRO_GUITAR_NOTE_FLAG_P_HARMONIC)
				{	//If this note has pinch harmonic status
					if(featurerestriction == 0)
					{	//Only write the harmonic Sysex notation if not writing a Rock Band compliant MIDI
						eof_add_phase_shift_sysex_phrase(deltapos, deltapos + deltalength, type, 12);	//Write custom pro guitar pinch phrase markers
					}
				}

				/* write bend marker */
				if(noteflags & EOF_PRO_GUITAR_NOTE_FLAG_BEND)
				{	//If this note has bend status
					if(featurerestriction == 0)
					{	//Only write the bend Sysex notation if not writing a Rock Band compliant MIDI
						eof_add_phase_shift_sysex_phrase(deltapos, deltapos + deltalength, type, 13);	//Write custom pro guitar bend phrase markers
					}
				}

				/* write accent marker */
				if(noteflags & EOF_PRO_GUITAR_NOTE_FLAG_ACCENT)
				{	//If this note has accent status
					if(featurerestriction == 0)
					{	//Only write the accent Sysex notation if not writing a Rock Band compliant MIDI
						eof_add_phase_shift_sysex_phrase(deltapos, deltapos + deltalength, type, 14);	//Write custom pro guitar accent phrase markers
					}
				}

				/* write pop marker */
				if(noteflags & EOF_PRO_GUITAR_NOTE_FLAG_POP)
				{	//If this note has pop status
					if(featurerestriction == 0)
					{	//Only write the pop Sysex notation if not writing a Rock Band compliant MIDI
						eof_add_phase_shift_sysex_phrase(deltapos, deltapos + deltalength, type, 15);	//Write custom pro guitar pop phrase markers
					}
				}

				/* write slap marker */
				if(noteflags & EOF_PRO_GUITAR_NOTE_FLAG_SLAP)
				{	//If this note has slap status
					if(featurerestriction == 0)
					{	//Only write the slap Sysex notation if not writing a Rock Band compliant MIDI
						eof_add_phase_shift_sysex_phrase(deltapos, deltapos + deltalength, type, 16);	//Write custom pro guitar slap phrase markers
					}
				}

				/* write strum direction markers */
				if(noteflags & EOF_PRO_GUITAR_NOTE_FLAG_DOWN_STRUM)
				{	//If this note strums down
					eof_add_midi_event(deltapos, 0x90, midi_note_offset + 9, 114, 15);	//Down strum markers are note # (lane 1 + 9), channel 15 (velocity 114 is typical)
					eof_add_midi_event(deltapos + deltalength, 0x80, midi_note_offset + 9, velocity, 15);
				}
				else if(noteflags & EOF_PRO_GUITAR_NOTE_FLAG_MID_STRUM)
				{	//If this note strums in the middle
					eof_add_midi_event(deltapos, 0x90, midi_note_offset + 9, 109, 14);	//Middle strum markers are note # (lane 1 + 9), channel 14 (velocity 109 is typical)
					eof_add_midi_event(deltapos + deltalength, 0x80, midi_note_offset + 9, velocity, 14);
				}
				else if(noteflags & EOF_PRO_GUITAR_NOTE_FLAG_UP_STRUM)
				{	//If this note strums up
					eof_add_midi_event(deltapos, 0x90, midi_note_offset + 9, 96, 13);	//Up strum markers are note # (lane 1 + 9), channel 13 (velocity 96 is typical)
					eof_add_midi_event(deltapos + deltalength, 0x80, midi_note_offset + 9, velocity, 13);
				}

				/* write fret hand positions */
				if(!fret_hand_pos_written)
				{	//If not all of the fret hand positions have been written yet
					if	((sp->tags->eof_fret_hand_pos_1_pg && ((sp->track[j]->track_type == EOF_TRACK_PRO_GUITAR) || (sp->track[j]->track_type == EOF_TRACK_PRO_GUITAR_22))) ||
						 (sp->tags->eof_fret_hand_pos_1_pb && ((sp->track[j]->track_type == EOF_TRACK_PRO_BASS) || (sp->track[j]->track_type == EOF_TRACK_PRO_BASS_22))))
					{	//If the user opted to write a single fret hand position of 1 for this pro guitar/bass track
						if(fret_hand_positions_present && !fret_hand_positions_generated && !fret_hand_pos_override_warning)
						{	//If this track had manually defined fret hand positions, and they are being overridden by the song property to define a single fret hand position at fret 1
							allegro_message("Warning:  This track has fret hand positions, but the song property to write a single fret hand position is overriding them");
							eof_log("Warning:  This track has fret hand positions, but the song property to write a single fret hand position is overriding them", 1);
							fret_hand_pos_override_warning = 1;
						}
						rootvel = 101;	//Velocity 101 represents the fretting hand positioned at fret 1
						eof_add_midi_event(deltapos, 0x90, 108, rootvel, 0);			//Note 108 denotes left hand position
						eof_add_midi_event(deltapos + deltalength, 0x80, 108, 64, 0);	//Write the note off event (using the same velocity that RB3 MIDIs use)
						fret_hand_pos_written = 1;	//Track that all necessary fret hand positions have been written
					}
					else if(fret_hand_positions_present)
					{	//If fret hand positions are already defined
						if(eof_get_note_type(sp, j, i) == EOF_NOTE_AMAZING)
						{	//If the note being written to MIDI is in Expert difficulty
							unsigned char position = eof_pro_guitar_track_find_effective_fret_hand_position(tp, EOF_NOTE_AMAZING, notepos);	//Get the effective fret hand position for this note
							if(!last_written_hand_pos || (last_written_hand_pos != position))
							{	//If this is the first hand position being written or the hand position has changed since the last one
								eof_add_midi_event(deltapos, 0x90, 108, position + 100, 0);		//Note 108 denotes left hand position (velocity = hand's fret position + 100)
								eof_add_midi_event(deltapos + deltalength, 0x80, 108, 64, 0);	//Write the note off event (using the same velocity that RB3 MIDIs use)
								last_written_hand_pos = position;	//Track the last written hand position
							}
						}
					}
					else
					{	//Fret hand positions are not defined, write "inefficient" fret hand positions on a per note basis
						if(eof_get_note_type(sp, j, i) == EOF_NOTE_AMAZING)
						{	//For the Expert difficulty, write left hand position notes
						/* write left hand position note, which is a note 108 with the same velocity of the lowest fret used in the pro guitar note */
							rootvel = 0xFF;
							for(ctr = 0, bitmask = 1; ctr < 6; ctr++, bitmask <<= 1)
							{	//For each of the 6 usable strings, from lowest to highest gauge
								if(note & bitmask)
								{	//If this string is used
									if(rootvel == 0xFF)
									{	//If no velocity has been recorded so far
										rootvel = (sp->pro_guitar_track[tracknum]->note[i]->frets[ctr] & 0x7F) + 100;	//Store the velocity used for this gem (mask out the MSB, used for mute status)
									}
									else if(sp->pro_guitar_track[tracknum]->note[i]->frets[ctr] + 100 < rootvel)
									{	//Otherwise store this gem's velocity if it is lower than the others checked for this note
										rootvel = (sp->pro_guitar_track[tracknum]->note[i]->frets[ctr] & 0x7F) + 100;
									}
								}
							}
							if(!last_written_hand_pos || (last_written_hand_pos != rootvel))
							{	//If this is the first hand position being written or the hand position has changed since the last one
								eof_add_midi_event(deltapos, 0x90, 108, rootvel, 0);			//Note 108 denotes left hand position
								eof_add_midi_event(deltapos + deltalength, 0x80, 108, 64, 0);	//Write the note off event (using the same velocity that RB3 MIDIs use)
								last_written_hand_pos = rootvel;	//Track the last written hand position
							}
						}
					}//Otherwise write the left hand positions based on notes in the expert difficulty
				}//If not all of the fret hand positions have been written yet

				/* write root note, which is a note from 4 to 15, to represent the chord's major scale (where any E scale chord is 4, F is 5, Gb is 6, ..., Eb is 15) */
				if(eof_get_note_type(sp, j, i) != EOF_NOTE_AMAZING)
					continue;	//If this note isn't in the expert difficulty, skip writing root notes

				if(eof_note_count_colors(sp, j, i) > 1)
				{	//If this is a chord
					scale = 17;	//Unless a chord name is found, write a root note of 17 (no name)
					if(eof_lookup_chord(sp->pro_guitar_track[tracknum], j, i, &scale, &chord, &isslash, &bassnote, 0, 0))
					{	//If the chord lookup logic found a match
						if(isslash)
						{	//If it was found to be a slash chord
							eof_add_midi_event(deltapos, 0x90, 16, vel, 0);		//Write a "slash" supplemental root note identifier of 16
							eof_add_midi_event(deltapos + deltalength, 0x80, 16, vel, 0);
						}
						scale = (scale + 9) % 16 + (4 * ((scale + 9) / 16));	//Convert the scale to RB3's numbering system
					}
					if(scale != last_scale)
					{	//It's not needed to write repeated root notes for consecutive matching notes
						eof_add_midi_event(deltapos, 0x90, scale, vel, 0);		//Write a root note reflecting the scale the chord is in
						eof_add_midi_event(deltapos + deltalength, 0x80, scale, vel, 0);
						last_scale = scale;
					}
				}
			}//For each note in the track

			/* fill in arpeggios */
			for(i = 0; i < eof_get_num_arpeggios(sp, j); i++)
			{	//For each arpeggio in the track
				sectionptr = eof_get_arpeggio(sp, j, i);
				deltapos = eof_ConvertToDeltaTime(sectionptr->start_pos, anchorlist, tslist, timedivision, 1, has_stored_tempo);	//Store the tick position of the phrase
				deltalength = eof_ConvertToDeltaTime(sectionptr->end_pos, anchorlist, tslist, timedivision, 0, has_stored_tempo) - deltapos;	//Store the number of delta ticks representing the phrase's length
				if(deltalength < 1)
				{	//If some kind of rounding error or other issue caused the delta length to be less than 1, force it to the minimum length of 1
					deltalength = 1;
				}
				switch(sectionptr->difficulty)
				{
					case EOF_NOTE_AMAZING:	//notes 96-101
					{
						midi_note_offset = 96;
						break;
					}
					case EOF_NOTE_MEDIUM:	//notes 72-77
					{
						midi_note_offset = 72;
						break;
					}
					case EOF_NOTE_EASY:		//notes 48-58
					{
						midi_note_offset = 48;
						break;
					}
					case EOF_NOTE_SUPAEASY:	//notes 24-29
					{
						midi_note_offset = 24;
						break;
					}
					default:	//Invalid difficulty for an arpeggio phrase
					{
						continue;
					}
				}
				eof_add_midi_event(deltapos, 0x90, midi_note_offset + 8, vel, 0);	//Arpeggio markers are note # (lane 1 + 8)
				eof_add_midi_event(deltapos + deltalength, 0x80, midi_note_offset + 8, vel, 0);
			}

			/* fill in solos */
			for(i = 0; i < eof_get_num_solos(sp, j); i++)
			{	//For each solo in the track
				sectionptr = eof_get_solo(sp, j, i);
				deltapos = eof_ConvertToDeltaTime(sectionptr->start_pos, anchorlist, tslist, timedivision, 1, has_stored_tempo);	//Store the tick position of the phrase
				deltalength = eof_ConvertToDeltaTime(sectionptr->end_pos, anchorlist, tslist, timedivision, 0, has_stored_tempo) - deltapos;	//Store the number of delta ticks representing the phrase's length
				if(deltalength < 1)
				{	//If some kind of rounding error or other issue caused the delta length to be less than 1, force it to the minimum length of 1
					deltalength = 1;
				}
				eof_add_midi_event(deltapos, 0x90, 115, vel, 0);	//Note 115 denotes a pro guitar solo marker
				eof_add_midi_event(deltapos + deltalength, 0x80, 115, vel, 0);
			}

			/* fill in star power */
			for(i = 0; i < eof_get_num_star_power_paths(sp, j); i++)
			{	//For each star power path in the track
				sectionptr = eof_get_star_power_path(sp, j, i);
				deltapos = eof_ConvertToDeltaTime(sectionptr->start_pos, anchorlist, tslist, timedivision, 1, has_stored_tempo);	//Store the tick position of the phrase
				deltalength = eof_ConvertToDeltaTime(sectionptr->end_pos, anchorlist, tslist, timedivision, 0, has_stored_tempo) - deltapos;	//Store the number of delta ticks representing the phrase's length
				if(deltalength < 1)
				{	//If some kind of rounding error or other issue caused the delta length to be less than 1, force it to the minimum length of 1
					deltalength = 1;
				}
				eof_add_midi_event(deltapos, 0x90, 116, vel, 0);	//Note 116 denotes a star power marker
				eof_add_midi_event(deltapos + deltalength, 0x80, 116, vel, 0);
			}

			/* fill in tremolos */
			for(i = 0; i < eof_get_num_tremolos(sp, j); i++)
			{	//For each tremolo in the track
				sectionptr = eof_get_tremolo(sp, j, i);
				if(sectionptr->difficulty == 0xFF)
				{	//Only export a tremolo section to MIDI if it applies to all difficulties
					deltapos = eof_ConvertToDeltaTime(sectionptr->start_pos, anchorlist, tslist, timedivision, 1, has_stored_tempo);	//Store the tick position of the phrase
					deltalength = eof_ConvertToDeltaTime(sectionptr->end_pos, anchorlist, tslist, timedivision, 0, has_stored_tempo) - deltapos;	//Store the number of delta ticks representing the phrase's length
					if(deltalength < 1)
					{	//If some kind of rounding error or other issue caused the delta length to be less than 1, force it to the minimum length of 1
						deltalength = 1;
					}
					eof_add_midi_event(deltapos, 0x90, 126, vel, 0);	//Note 126 denotes a tremolo marker
					eof_add_midi_event(deltapos + deltalength, 0x80, 126, vel, 0);
				}
			}

			/* fill in trills */
			for(i = 0; i < eof_get_num_trills(sp, j); i++)
			{	//For each trill in the track
				sectionptr = eof_get_trill(sp, j, i);
				deltapos = eof_ConvertToDeltaTime(sectionptr->start_pos, anchorlist, tslist, timedivision, 1, has_stored_tempo);	//Store the tick position of the phrase
				deltalength = eof_ConvertToDeltaTime(sectionptr->end_pos, anchorlist, tslist, timedivision, 0, has_stored_tempo) - deltapos;	//Store the number of delta ticks representing the phrase's length
				if(deltalength < 1)
				{	//If some kind of rounding error or other issue caused the delta length to be less than 1, force it to the minimum length of 1
					deltalength = 1;
				}
				eof_add_midi_event(deltapos, 0x90, 127, vel, 0);	//Note 127 denotes a trill marker
				eof_add_midi_event(deltapos + deltalength, 0x80, 127, vel, 0);
			}

			if(eof_midi_event_full)
			{	//If the track exceeded the number of MIDI events that could be written
				allegro_message("Error:  Too many MIDI events, aborting MIDI export.");
				eof_log("Error:  Too many MIDI events, aborting MIDI export.", 1);
				eof_destroy_ts_list(tslist);		//Free memory used by the TS change list
				eof_destroy_ks_list(kslist);		//Free memory used by the KS change list
				eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
				eof_clear_midi_events();			//Free any memory allocated for the MIDI event array
				return 0;	//Return failure
			}

			for(channelctr = 0; channelctr < 16; channelctr++)
			{	//For each of the usable MIDI channels
				for(i = 0; i < 128; i++)
				{	//Ensure that any notes that are still on are terminated
					if(eof_midi_note_status[channelctr][i] == 0)
						continue;	//If this note was not left on, skip it

					//Otherwise send an alert message, as this is abnormal
					allegro_message("MIDI export error:  Note %lu was not turned off", i);
					eof_log("MIDI export error:  Note %lu was not turned off", 1);
					if(eof_midi_endbeatnum)
					{	//If the chart has a manually defined end event, that's probably the cause
						eof_clear_input();
						eof_log("\tend event was manually defined", 1);
						if(alert("It appears this is due to an [end] event that cuts out a note early", "Would you like to seek to the beat containing this [end] event?", NULL, "&Yes", "&No", 'y', 'n') == 1)
						{	//If user opts to seek to the offending event
							eof_set_seek_position(sp->beat[eof_midi_endbeatnum]->pos + eof_av_delay);
							eof_selected_beat = eof_midi_endbeatnum;
						}
					}
					eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
					eof_destroy_ts_list(tslist);		//Free memory used by the TS change list
					eof_destroy_ks_list(kslist);		//Free memory used by the KS change list
					eof_clear_midi_events();			//Free any memory allocated for the MIDI event array
					return 0;	//Return failure
				}
			}
			qsort(eof_midi_event, (size_t)eof_midi_events, sizeof(EOF_MIDI_EVENT *), qsort_helper3);
			eof_check_for_note_overlap();	//Filter out any improperly overlapping note on/off events
//			allegro_message("break1");

			/* open the file */
			fp = pack_fopen(notetempname[j], "w");
			if(!fp)
			{
				eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
				eof_destroy_ts_list(tslist);		//Free memory used by the TS change list
				eof_destroy_ks_list(kslist);		//Free memory used by the KS change list
				eof_clear_midi_events();			//Free any memory allocated for the MIDI event array
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError saving:  Cannot open temporary MIDI track:  \"%s\"", strerror(errno));	//Get the Operating System's reason for the failure
				eof_log(eof_log_string, 1);
				return 0;	//Return failure
			}

			/* write the track name */
			WriteVarLen(0, fp);
			(void) pack_putc(0xFF, fp);
			(void) pack_putc(0x03, fp);
			WriteVarLen(ustrsize(sp->track[j]->name), fp);
			(void) pack_fwrite(sp->track[j]->name, ustrsize(sp->track[j]->name), fp);

			/* add MIDI events */
			lastdelta = 0;
			for(i = 0; i < eof_midi_events; i++)
			{
				if(eof_midi_event[i]->filtered)
					continue;	//If this event is filtered, skip it

				delta = eof_midi_event[i]->pos;
				if(eof_midi_event[i]->type == 0x01)
				{	//Write a note name text event
					eof_write_text_event(delta-lastdelta, eof_midi_event[i]->dp, fp);
					lastevent = 0;	//Reset running status after a meta/sysex event
				}
				else if(eof_midi_event[i]->type == 0xF0)
				{	//If this is a Sysex message
					WriteVarLen(delta-lastdelta, fp);	//Write this event's relative delta time
					(void) pack_putc(0xF0, fp);						//Sysex event
					WriteVarLen(eof_midi_event[i]->note, fp);	//Write the Sysex message's size
					(void) pack_fwrite(eof_midi_event[i]->dp, eof_midi_event[i]->note, fp);	//Write the Sysex data
					lastevent = 0;	//Reset running status after a meta/sysex event
				}
				else
				{	//Write a non meta MIDI event
					WriteVarLen(delta-lastdelta, fp);	//Write this event's relative delta time
					if(eof_midi_event[i]->type + eof_midi_event[i]->channel != lastevent)
					{	//With running status, the MIDI event type needn't be written if it's the same as the previous event
						(void) pack_putc(eof_midi_event[i]->type + eof_midi_event[i]->channel, fp);
					}
					(void) pack_putc(eof_midi_event[i]->note, fp);
					(void) pack_putc(eof_midi_event[i]->velocity, fp);
					lastevent = eof_midi_event[i]->type + eof_midi_event[i]->channel;
				}
				if(eof_midi_event[i]->allocation && eof_midi_event[i]->dp)
				{	//If this event has allocated memory to release
					free(eof_midi_event[i]->dp);	//Free it now
					eof_midi_event[i]->dp = NULL;
					eof_midi_event[i]->allocation = 0;
				}
				lastdelta = delta;					//Store this event's absolute delta time
			}

			/* clean up fret hand positions */
			if(fret_hand_positions_generated)
			{	//If fret hand positions were automatically generated, remove them now
				for(ctr = tp->handpositions; ctr > 0 ; ctr--)
				{	//For each hand position in this track, in reverse order
					if(tp->handposition[ctr - 1].difficulty == EOF_NOTE_AMAZING)
					{	//If this hand position exists in the Expert difficulty
						eof_pro_guitar_track_delete_hand_position(tp, ctr - 1);	//Delete the hand position
					}
				}
			}

			/* end of track */
			WriteVarLen(0, fp);
			(void) pack_putc(0xFF, fp);
			(void) pack_putc(0x2F, fp);
			(void) pack_putc(0x00, fp);
			(void) pack_fclose(fp);
		}//If this is a pro guitar track

		eof_menu_track_set_tech_view_state(sp, j, restore_tech_view);	//Re-enable tech view if applicable
	}//For each track in the project
	eof_clear_midi_events();			//Free any memory allocated for the MIDI event array

/* make tempo track */
	fp = pack_fopen(tempotempname, "w");
	if(!fp)
	{
		eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
		eof_destroy_ts_list(tslist);		//Free memory used by the TS change list
		eof_destroy_ks_list(kslist);		//Free memory used by the KS change list
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError saving:  Cannot open temporary MIDI track:  \"%s\"", strerror(errno));	//Get the Operating System's reason for the failure
		eof_log(eof_log_string, 1);
		return 0;	//Return failure
	}
	if(featurerestriction != 0)
	{	//If writing a RB3 compliant MIDI
		//I've found that Magma will not recognize tracks correctly unless track 0 has a name defined
		/* determine the appropriate track name */
		if(sp->tags->title[0] != '\0')
		{	//If a song title has been defined
			(void) snprintf(chordname, sizeof(chordname) - 1, "%s", sp->tags->title);	//Borrow this array to store the chart title
		}
		else
		{	//Make up a track name so it will build in Magma
			(void) snprintf(chordname, sizeof(chordname) - 1, "Tempo map");
			eof_log("\t! Song title is not defined, a fake song title was used as the name for track 0 so the song will build in Magma", 1);
		}
		tempotrackname = chordname;
	}
	else
	{
		tempotrackname = NULL;
	}
	eof_write_tempo_track(tempotrackname, anchorlist, tslist, kslist, fp);	//Write the tempo track to file
	(void) pack_fclose(fp);

/* make events track */
	if((featurerestriction != 2) && !eof_events_overridden_by_stored_MIDI_track(sp))
	{	//Do not write an events track in a pro guitar upgrade MIDI, or if the project has an events track stored into it
		if((sp->text_events) || (featurerestriction == 1) || (featurerestriction == 3))
		{	//If there are manually defined text events, or if writing a RBN2 or C3 compliant MIDI (which requires certain events)
			/* open the file */
			fp = pack_fopen(eventtempname, "w");
			if(!fp)
			{
				eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
				eof_destroy_ts_list(tslist);		//Free memory used by the TS change list
				eof_destroy_ks_list(kslist);		//Free memory used by the KS change list
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError saving:  Cannot open temporary MIDI track:  \"%s\"", strerror(errno));	//Get the Operating System's reason for the failure
				eof_log(eof_log_string, 1);
				return 0;	//Return failure
			}

			/* write the track name */
			WriteVarLen(0, fp);
			(void) pack_putc(0xFF, fp);
			(void) pack_putc(0x03, fp);
			WriteVarLen(ustrsize("EVENTS"), fp);
			(void) pack_fwrite("EVENTS", ustrsize("EVENTS"), fp);

			/* add MIDI events */
			lastdelta = 0;
			for(i = 0; i < sp->text_events; i++)
			{
				if(sp->text_event[i]->track == 0)
				{	//If the text event is global (not specific to any single track)
					double eventpos;

					if(!(sp->text_event[i]->flags & EOF_EVENT_FLAG_FLOATING_POS))
					{	//If this text event is assigned to a beat marker
						if(sp->text_event[i]->pos >= sp->beats)
						{	//If the text event is corrupted
							sp->text_event[i]->pos = sp->beats - 1;	//Repair it by assigning it to the last beat marker
						}
					}
					eventpos = eof_get_text_event_fpos(sp, i);
					delta = eof_ConvertToDeltaTime(eventpos, anchorlist, tslist, timedivision, 1, has_stored_tempo);
					eof_write_text_event(delta - lastdelta, sp->text_event[i]->text, fp);
					lastdelta = delta;					//Store this event's absolute delta time
				}
			}

			if((featurerestriction == 1) || (featurerestriction == 3))
			{	//If writing a RBN2 or C3 compliant MIDI
				//Magma requires that the [end] event is the last MIDI event in the track, so it will be written 1ms after the end of the audio
				//Check the existing events to see if such an event is already defined
				if(!eof_song_contains_event(sp, "[end]", 0, 0xFFFF, 0))
				{	//If the user did not define the end event, manually write it
					eof_log("\t! Adding missing [end] event", 1);
					delta = eof_chart_length + 1;	//Prepare to write the end event after the audio ends
					if(sp->beat[sp->beats - 1]->pos > delta)
					{	//If the last beat ends after the audio,
						delta = sp->beat[sp->beats - 1]->pos + 1;	//Prepare to write the end event after it instead
					}
					delta = eof_ConvertToDeltaTime(delta, anchorlist, tslist, timedivision, 1, has_stored_tempo);
					eof_write_text_event(delta - lastdelta, "[end]", fp);
					lastdelta = delta;					//Store this event's absolute delta time
				}
			}

			/* end of track */
			WriteVarLen(0, fp);
			(void) pack_putc(0xFF, fp);
			(void) pack_putc(0x2F, fp);
			(void) pack_putc(0x00, fp);

			(void) pack_fclose(fp);
			eventstrackwritten = 1;
		}//If there are manually defined text events, or if writing a RBN2 compliant MIDI (which requires certain events)
	}//Do not write an events track in a pro guitar upgrade MIDI

	//Remove all temporary text events that were added during this export
	for(i = sp->text_events; i > 0; i--)
	{	//For each text event (in reverse order)
		if(sp->text_event[i-1]->is_temporary)
		{	//If this text event has been marked as temporary
			eof_song_delete_text_event(sp, i-1);	//Delete it
		}
	}
	eof_sort_events(sp);	//Re-sort

/* make beat track */
	for(trackptr = sp->midi_data_head; trackptr != NULL; trackptr = trackptr->next)	//Add the number of raw MIDI tracks to export to the track counter
	{	//For each stored MIDI track
		if(trackptr->trackname && !ustricmp(trackptr->trackname, "(BEAT)"))
		{	//If this stored track name indicates that it's a beat track
			eof_log("\tSkipping the creation of a BEAT track, since there is one stored into the project already", 1);
			stored_beat_track = 1;
			break;
		}
	}
	if(((featurerestriction == 1) || (featurerestriction == 3)) && !stored_beat_track)
	{	//If writing a RBN2 or C3 compliant MIDI, make the beat track, which is required (unless a BEAT track was already stored into the project)
		unsigned long beat_counter = 0;
		unsigned beats_per_measure = 4;		//By default, a 4/4 time signature is assumed until a TS event is reached
		unsigned note_to_write = 0;
		unsigned length_to_write = 0;
		int lastevent = 0;	//Track the last event written so running status can be utilized

		/* open the file */
		fp = pack_fopen(beattempname, "w");
		if(!fp)
		{
			eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
			eof_destroy_ts_list(tslist);		//Free memory used by the TS change list
			eof_destroy_ks_list(kslist);		//Free memory used by the KS change list
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError saving:  Cannot open temporary MIDI track:  \"%s\"", strerror(errno));	//Get the Operating System's reason for the failure
			eof_log(eof_log_string, 1);
			return 0;	//Return failure
		}

		/* write the track name */
		WriteVarLen(0, fp);
		(void) pack_putc(0xFF, fp);
		(void) pack_putc(0x03, fp);
		WriteVarLen(ustrsize("BEAT"), fp);
		(void) pack_fwrite("BEAT", ustrsize("BEAT"), fp);

		/* parse the beat array, writing a note #12 at the first beat of every measure, and a note #13 at every other beat */
		lastdelta = 0;
		for(i = 0; i < sp->beats; i++)
		{
			//Determine if this is the first beat in a measure and which note number to write
			if(eof_get_ts(sp,&beats_per_measure,NULL,i) == 1)
			{	//If this beat is a time signature
				beat_counter = 0;
			}
			if(beat_counter == 0)
			{	//If this is the first beat in a measure (the downbeat), write a note #12
				note_to_write = 12;
			}
			else
			{	//Otherwise write a note #13 for all non downbeats
				note_to_write = 13;
			}

			//Determine the length of 1/4 of the current beat, which will be the length of the beat notes written
			//Based on the formula "length of beat = 60000 ms / BPM", the formula "length of beat = ppqn / 1000" can be derived
			length_to_write = (double)sp->beat[i]->ppqn / 1000.0 / 4.0 + 0.5;	//Round up to nearest millisecond

			//Write the note on event
			delta = eof_ConvertToDeltaTime(sp->beat[i]->pos, anchorlist, tslist, timedivision, 1, has_stored_tempo);
			delta2 = eof_ConvertToDeltaTime(sp->beat[i]->pos + length_to_write, anchorlist, tslist, timedivision, 0, has_stored_tempo);
			if(!eof_midi_enddelta || ((delta <= eof_midi_enddelta) && (delta2 <= eof_midi_enddelta)))
			{	//Only write this beat marker if it starts and stops before the end event
				WriteVarLen(delta-lastdelta, fp);	//Write this event's relative delta time
				lastdelta = delta;					//Store this event's absolute delta time
				if(lastevent != 0x90)
				{	//If no MIDI notes have been written for the BEAT track yet
					(void) pack_putc(0x90, fp);				//MIDI event 0x9 (note on), channel 0
				}
				lastevent = 0x90;					//Track that a status byte has been written, so that no others need to be for this track (running status)
				(void) pack_putc(note_to_write, fp);		//Note 12 or 13
				(void) pack_putc(100, fp);					//Pre-determined velocity

				//Write the note off event
				WriteVarLen(delta2-lastdelta, fp);	//Write this event's relative delta time
				lastdelta = delta2;					//Store this event's absolute delta time
				(void) pack_putc(note_to_write, fp);		//Note 12 or 13
				(void) pack_putc(0, fp);					//A note on with a velocity of 0 is treated as a note off
			}

			//Increment to the next beat
			beat_counter++;
			if(beat_counter >= beats_per_measure)
			{
				beat_counter = 0;
			}
		}

		/* end of track */
		WriteVarLen(0, fp);
		(void) pack_putc(0xFF, fp);
		(void) pack_putc(0x2F, fp);
		(void) pack_putc(0x00, fp);

		(void) pack_fclose(fp);
		beattrackwritten = 1;
	}	//If writing a RBN2 compliant MIDI, write the beat track, which is required

/* write the main MIDI file */
	fp = pack_fopen(fn, "w");
	if(!fp)
	{
		eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
		eof_destroy_ts_list(tslist);		//Free memory used by the TS change list
		eof_destroy_ks_list(kslist);		//Free memory used by the KS change list
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError saving:  Cannot open output MIDI file:  \"%s\"", strerror(errno));	//Get the Operating System's reason for the failure
		eof_log(eof_log_string, 1);
		return 0;	//Return failure
	}

	/* write header data */
	trackcounter += 1 + eventstrackwritten + beattrackwritten;	//Add 1 for track 0 and one each for the events and beat tracks if applicable

	if(featurerestriction != 2)
	{	//Unless writing a RB3 compliant pro guitar upgrade MIDI, include stored MIDI tracks
		for(trackptr = sp->midi_data_head; trackptr != NULL; trackptr = trackptr->next)	//Add the number of raw MIDI tracks to export to the track counter
		{	//For each stored MIDI track
			if(!trackptr->timedivision)	//If the stored track does not contain a time division (is not a stored tempo track)
				trackcounter++;			//Count it toward the number of tracks to be exported (track 0 is already counted)
		}
	}
	header[11] = trackcounter;	//Write the number of tracks present into the MIDI header
	(void) pack_fwrite(header, 14, fp);

	if(expertpluswritten)
	{
		(void) replace_filename(expertplusfilename, fn, expertplusshortname, 1024);	//Build the path for the output expert+ MIDI
		fp3 = pack_fopen(expertplusfilename, "w");
		if(!fp3)
			expertpluswritten = 0;	//Cancel trying to write Expert+ and save the normal MIDI instead
		else
			(void) pack_fwrite(header, 14, fp3);	//Write the Expert+ file's MIDI header
	}


/* write tempo track */
	(void) eof_dump_midi_track(tempotempname,fp);
	if(expertpluswritten)
	{	//If writing an expert+ MIDI as well
		(void) eof_dump_midi_track(tempotempname,fp3);
	}

/* write text event track if there are any events */
	if(eventstrackwritten)
	{	//If the events track was built and is cited as present in the MIDI header's track count
		(void) eof_dump_midi_track(eventtempname,fp);
		if(expertpluswritten)
		{
			(void) eof_dump_midi_track(eventtempname,fp3);
		}
	}

	if((featurerestriction == 1) || (featurerestriction == 3))
	{	//If writing a RBN2 or C3 compliant MIDI, write the beat track, which is required
		(void) eof_dump_midi_track(beattempname,fp);
		if(expertpluswritten)
		{	//If writing an expert+ MIDI as well
			(void) eof_dump_midi_track(tempotempname,fp3);
		}
	}

/* write the remaining tracks */
	for(k = 0; k <= expertpluswritten; k++)
	{	//Run loop a second time if expert plus MIDI is being written
		if(k > 0)
		{	//If the expert+ MIDI file is being written in this loop iteration
			fp = fp3;	//Switch the output packfile pointer to the expert+ MIDI
			fp3 = NULL;
		}
		for(j = 1; j < sp->tracks; j++)
		{
			if(notetrackspopulated[j])
			{	//If this track had a temp file created
				if((k > 0) && (j == EOF_TRACK_DRUM))
				{	//If the expert+ drum track is being written
					(void) eof_dump_midi_track(expertplustempname,fp);
				}
				else
				{	//Otherwise write the regular track
					(void) eof_dump_midi_track(notetempname[j],fp);
				}
			}
		}
		if(featurerestriction != 2)
		{	//Unless writing a RB3 compliant pro guitar upgrade MIDI, include stored MIDI tracks
			eof_MIDI_data_track_export(sp, fp, anchorlist, tslist, timedivision);	//Write any stored raw MIDI tracks to the output file
		}
		(void) pack_fclose(fp);	//Close the output file
		fp = NULL;
	}

/* delete temporary files */
	for(i = 0; i < EOF_TRACKS_MAX+1; i++)
	{
		(void) delete_file(notetempname[i]);
	}
	(void) delete_file(expertplustempname);
	(void) delete_file(tempotempname);
	(void) delete_file(eventtempname);
	(void) delete_file(beattempname);

	eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
	eof_destroy_ts_list(tslist);		//Free memory used by the TS change list
	eof_destroy_ks_list(kslist);		//Free memory used by the KS change list

	eof_log("\tMIDI export completed", 1);
	return 1;	//Return success
}

unsigned char eof_get_midi_pitches(EOF_SONG *sp, unsigned long track, unsigned long note, unsigned char *pitches)
{
	unsigned char pitchmask = 0, notenote, ctr, bitmask;

	if(!sp || !track || (track >= sp->tracks) || (note >= eof_get_track_size(sp, track)) || !pitches)
		return 0;	//Invalid parameters

	if(sp->track[track]->track_format == EOF_VOCAL_TRACK_FORMAT)
	{	//This is a vocal track
		notenote = eof_get_note_note(sp, track, note);
		if(notenote && (notenote != VOCALPERCUSSION))
		{	//If the lyric's pitch is defined
			pitches[0] = notenote;	//Store it in the pitch array
			pitchmask |= 1;	//Set the LSB of the return bitmask
		}
	}
	else if(eof_track_is_pro_guitar_track(sp, track))
	{	//This is a pro guitar track
		EOF_PRO_GUITAR_TRACK *tp = sp->pro_guitar_track[sp->track[track]->tracknum];

		for(ctr = 0, bitmask = 1; ctr < 6; ctr++, bitmask <<= 1)
		{	//For each of the 6 supported strings
			if(tp->note[note]->note & bitmask)
			{	//If this string is used
				if(!(tp->note[note]->ghost & bitmask))
				{	//If this string is not ghosted
					unsigned char effective_fret_value =  tp->note[note]->frets[ctr];
					if(tp->note[note]->frets[ctr] & 0x80)
					{	//If this string is muted
						effective_fret_value = 0;	//Have the pitch array element for this string reflect an open note
					}
					//This note is found by adding default tuning for the string, the offset defining the current tuning, the position of any capo in use and the fret number being played
					pitches[ctr] = tp->tuning[ctr] + eof_lookup_default_string_tuning_absolute(tp, track, ctr) + effective_fret_value + tp->capo;	//Store it in the pitch array
					if(!(tp->note[note]->frets[ctr] & 0x80))
					{	//If this string is not muted
						pitchmask |= bitmask;	//Set the appropriate bit in the mask
					}
				}
			}
		}
	}

	return pitchmask;
}

int eof_export_music_midi(EOF_SONG *sp, char *fn, char format)
{
	unsigned char header[14] = {'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 1, 0, 1, (EOF_DEFAULT_TIME_DIVISION >> 8), (EOF_DEFAULT_TIME_DIVISION & 0xFF)}; //The last two bytes are the time division
	unsigned long timedivision = EOF_DEFAULT_TIME_DIVISION;	//Unless the project is storing a tempo track, EOF's default time division will be used
	struct Tempo_change *anchorlist=NULL;	//Linked list containing tempo changes
	PACKFILE * fp;
	unsigned long i, j, k, bitmask, deltapos, deltalength, channelctr;
	unsigned long delta = 0;
	int vel, channel = 0;
	unsigned long lastdelta=0;				//Keeps track of the last anchor's absolute delta time
	unsigned long trackcounter = 0;			//Tracks the number of pro guitar and vocal tracks to write to file
	unsigned long totaltrackcounter = 0;	//Tracks the number of tracks to write to file
	EOF_MIDI_TS_LIST *tslist=NULL;			//List containing TS changes
	struct eof_MIDI_data_track *trackptr;		//Used to count the number of raw MIDI tracks to export
	EOF_MIDI_KS_LIST *kslist;
	#define EOF_MUSIC_MIDI_TRACKS_MAX (EOF_PRO_GUITAR_TRACKS_MAX + EOF_PRO_GUITAR_TRACKS_MAX + EOF_VOCAL_TRACKS_MAX + 1)
	char notetempname[EOF_MUSIC_MIDI_TRACKS_MAX][25] = {{0}};	//The list of temporary files created to store the binary content for each MIDI track, a maximum potential of two copies of each pro guitar track and one of each vocal track
	char tempotempname[30];
	char eventtempname[30];
	unsigned char pitchmask, pitches[6] = {0};
	char *name, notename[EOF_NAME_LENGTH+1] = {0};
	char eventstrackwritten = 0;			//Tracks whether an events track has been written
	char *arrangement_name;
	unsigned long numpasses = 1, passnum;
	char has_stored_tempo;		//Will be set to nonzero if the project contains a stored tempo track, which will affect timing conversion

	eof_log("eof_export_music_midi() entered", 1);

	if(!sp || !fn)
	{
		eof_log("\tError saving:  Invalid parameters", 1);
		return 0;	//Return failure
	}

	if(eof_validate_temp_folder())
	{	//Ensure the correct working directory and presence of the temporary folder
		eof_log("\tCould not validate working directory and temp folder", 1);
		return 0;	//Return failure
	}

	if(!eof_calculate_beat_delta_positions(sp, EOF_DEFAULT_TIME_DIVISION))
	{	//Calculate the delta position of each beat in the chart
		eof_log("\tCould not build beat delta positions", 1);
		return 0;	//Return failure
	}

	has_stored_tempo = eof_song_has_stored_tempo_track(sp) ? 1 : 0;	//Store this status

	//Build tempo and TS lists
	if(!eof_build_tempo_and_ts_lists(sp, &anchorlist, &tslist, &timedivision))
	{
		eof_log("\tError saving:  Cannot build tempo or TS list", 1);
		return 0;	//Return failure
	}
	kslist = eof_build_ks_list(sp);		//Build a list of key signature changes
	if(!kslist)
	{
		eof_log("\tError saving:  Cannot build key signature list", 1);
		eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
		eof_destroy_ts_list(tslist);		//Free memory used by the TS change list
		return 0;	//Return failure
	}
	header[12] = timedivision >> 8;		//Update the MIDI header to reflect the time division (which may have changed if a stored tempo track is present)
	header[13] = timedivision & 0xFF;

	//Generate temporary filenames
	(void) snprintf(tempotempname, sizeof(tempotempname) - 1, "%stempo.tmp", eof_temp_path_s);
	(void) snprintf(eventtempname, sizeof(eventtempname) - 1, "%sevent.tmp", eof_temp_path_s);
	for(i = 0; i < EOF_MUSIC_MIDI_TRACKS_MAX; i++)
	{
		(void) snprintf(notetempname[i], sizeof(notetempname[i]) - 1, "%seof%lu.tmp", eof_temp_path_s, i);
	}

	eof_sort_notes(sp);
	eof_sort_events(sp);

	if(format)
	{	//If writing a Fretlight style MIDI
		numpasses = 2;	//Each pro guitar/bass track will be written twice, once in Synthesia format and once in Fretlight format (the former is used for audio playback and the latter for tablature signaling)
	}

	//Write tracks
	for(passnum = 0; passnum < numpasses; passnum++)
	{	//Parse each track twice, on first pass write the tracks in Synthesia style, on second pass write the tracks in Fretlight style
		for(j = 1; j < sp->tracks; j++)
		{	//For each track in the project
			int lastevent = 0;	//Track the last event written so running status can be utilized
			char restore_tech_view = 0;			//If tech view is in effect, it is temporarily disabled so that the correct notes are exported

			if(eof_get_track_size_normal(sp, j) == 0)	//If this track has no notes (in the normal note set)
				continue;	//Skip the track

			if((!eof_track_is_pro_guitar_track(sp, j)) && (sp->track[j]->track_format != EOF_VOCAL_TRACK_FORMAT))	//If this isn't a vocal or pro guitar track
				continue;	//Skip the track

			if(passnum && (sp->track[j]->track_format == EOF_VOCAL_TRACK_FORMAT))
				continue;	//Only write the vocal track once

			if(eof_track_overridden_by_stored_MIDI_track(sp, j))	//If this track is overridden by a stored MIDI track
				continue;	//Skip the track

			eof_clear_midi_events();
			memset(eof_midi_note_status,0,sizeof(eof_midi_note_status));	//Clear note status array

			restore_tech_view = eof_menu_track_get_tech_view_state(sp, j);
			eof_menu_track_set_tech_view_state(sp, j, 0);	//Disable tech view if applicable

			//Write track specific text events
			for(i = 0; i < sp->text_events; i++)
			{	//For each event in the song
				if(sp->text_event[i]->track == j)
				{	//If this event is specific to this track
					double eventpos;

					if(!(sp->text_event[i]->flags & EOF_EVENT_FLAG_FLOATING_POS))
					{	//If this text event is assigned to a beat marker
						if(sp->text_event[i]->pos >= sp->beats)
						{	//If the text event's beat number is invalid
							sp->text_event[i]->pos = sp->beats - 1;	//Repair it by assigning it to the last beat marker
						}
					}
					eventpos = eof_get_text_event_fpos(sp, i);
					deltapos = eof_ConvertToDeltaTime(eventpos, anchorlist, tslist, timedivision, 1, has_stored_tempo);	//Store the tick position of the note
					eof_add_midi_text_event(deltapos, sp->text_event[i]->text, 0, i);	//Send 0 for the allocation flag, because the text string is being stored in static memory
				}
			}

			//Write notes
			for(i = 0; i < eof_get_track_size(sp, j); i++)
			{	//For each note/lyric in the track
				unsigned long pos = eof_get_note_pos(sp, j, i);	//Cache this value
				unsigned long nextpos = 0, length, minlength;
				unsigned long beat;

				length = eof_get_note_length(sp, j, i);
				if(i + 1 < eof_get_track_size(sp, j))
				{	//If there is another note in the track
					nextpos = eof_get_note_pos(sp, j, i + 1);
					if(pos == nextpos)
					{	//If this note and the next one are at the same position
						continue;	//Skip this one and use the higher difficulty note
					}
				}
				beat = eof_get_beat(sp, pos);
				if(eof_beat_num_valid(sp, beat))
				{	//If the beat in which this note exists could be determined
					double beat_length = eof_calc_beat_length(sp, beat);	//Get the length of the beat
					minlength = beat_length / 8;	//Get the desired minimum length for the note (1/8 beat)
					if(length < minlength)
					{	//If the note's length is shorter than that
						if(!nextpos || (pos + minlength < nextpos))
						{	//If there is no next note or if this note can be extended without overlapping the next note
							length = minlength;	//Do so
						}
						else
						{	//Otherwise just extend it as far as it can go without overlapping
							if(nextpos > pos + 1)
							{	//This is only possible if the next note is more than 1ms away
								length = nextpos - pos - 1;
							}
						}
					}
				}

				pitchmask = eof_get_midi_pitches(sp, j, i, pitches);	//Determine how many exportable pitches this note/lyric has
				if(!pitchmask)
					continue;	//If no pitches would be exported for this note/lyric, skip it

				//Write note/lyric pitches
				deltapos = eof_ConvertToDeltaTime(pos, anchorlist, tslist, timedivision, 1, has_stored_tempo);
				deltalength = eof_ConvertToDeltaTime(pos + length, anchorlist, tslist, timedivision, 0, has_stored_tempo) - deltapos;
				if(deltalength < 1)
				{	//If some kind of rounding error or other issue caused the delta length to be less than 1, force it to the minimum length of 1
					deltalength = 1;
				}
				if(!passnum)
				{	//Writing a Synthesia style MIDI
					if((eof_track_is_pro_guitar_track(sp, j)) && (eof_get_note_flags(sp, j, i) & EOF_PRO_GUITAR_NOTE_FLAG_ACCENT))
					{	//If this is a pro guitar note played as an accent
						vel = 127;	//Use the maximum velocity possible
					}
					else
					{
						vel = 64;	//Otherwise use half the maximum
					}
				}
				else
				{	//Writing a Fretlight style MIDI
					vel = 127;
				}
				for(k = 0, bitmask = 1; k < 6; k++, bitmask <<= 1)
				{	//For each of the 6 possible values in the pitch array
					if(pitchmask & bitmask)
					{	//If this pitch is defined in the array
						if(passnum)
						{	//Writing a Fretlight style MIDI
							channel = 15 - k;	//Fretlight's channel numbering is such that low E uses channel 15 and high E uses channel 10
						}
						eof_add_midi_event(deltapos, 0x90, pitches[k], vel, channel);
						eof_add_midi_event(deltapos + deltalength, 0x80, pitches[k], vel, channel);
					}
				}

				//Write note name or lyric text if applicable
				if(sp->track[j]->track_format == EOF_VOCAL_TRACK_FORMAT)
				{	//If a lyric is being exported
					name = eof_get_note_name(sp, j, i);
					if(name && (name[0] != '\0'))
					{	//If the lyric has defined text
						eof_add_midi_lyric_event(deltapos, name, 0);	//Track that the lyric text was NOT dynamically allocated
					}
				}
				else
				{	//A pro guitar note is being exported
					if(eof_build_note_name(sp, j, i, notename))
					{	//If the note's name was manually defined or could be detected automatically
						char * tempstring = malloc((size_t)ustrsizez(notename));	//Allocate memory to store a copy of the note name, because chord detection will overwrite notename[] each time it is used
						if(tempstring != NULL)
						{	//If allocation was successful
							memcpy(tempstring, notename, (size_t)ustrsizez(notename));	//Copy the string to the newly allocated memory
							eof_add_midi_text_event(deltapos, tempstring, 1, 0xFFFFFFFF);	//Store the new string in a text event, send 1 for the allocation flag, because the text string is being stored in dynamic memory (provide a high index to ensure it doesn't influence sort order)
						}
					}
				}

				if(eof_midi_event_full)
				{	//If the track exceeded the number of MIDI events that could be written
					allegro_message("Error:  Too many MIDI events, aborting MIDI export.");
					eof_log("Error:  Too many MIDI events, aborting MIDI export.", 1);
					eof_destroy_ks_list(kslist);		//Free memory used by the KS change list
					eof_destroy_ts_list(tslist);		//Free memory used by the TS change list
					eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
					eof_clear_midi_events();		//Free any memory allocated for the MIDI event array
					return 0;	//Return failure
				}
			}//For each note/lyric in the track

			//Cleanup
			for(channelctr = 0; channelctr < 16; channelctr++)
			{	//For each of the usable MIDI channels
				for(i = 0; i < 128; i++)
				{	//Ensure that any notes that are still on are terminated
					if(eof_midi_note_status[channelctr][i] == 0)
						continue;	//If this note was not left on, skip it

					//Otherwise send an alert message, as this is abnormal
					allegro_message("MIDI export error:  Note %lu was not turned off", i);
					eof_log("MIDI export error:  Note %lu was not turned off", 1);
					if(eof_midi_endbeatnum)
					{	//If the chart has a manually defined end event, that's probably the cause
						eof_clear_input();
						eof_log("\tend event was manually defined", 1);
						if(alert("It appears this is due to an [end] event that cuts out a note early", "Would you like to seek to the beat containing this [end] event?", NULL, "&Yes", "&No", 'y', 'n') == 1)
						{	//If user opts to seek to the offending event
							eof_set_seek_position(sp->beat[eof_midi_endbeatnum]->pos + eof_av_delay);
							eof_selected_beat = eof_midi_endbeatnum;
						}
					}
					eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
					eof_destroy_ts_list(tslist);		//Free memory used by the TS change list
					eof_destroy_ks_list(kslist);		//Free memory used by the KS change list
					eof_clear_midi_events();		//Free any memory allocated for the MIDI event array
					return 0;	//Return failure
				}
			}
			qsort(eof_midi_event, (size_t)eof_midi_events, sizeof(EOF_MIDI_EVENT *), qsort_helper3);
			eof_check_for_note_overlap();	//Filter out any improperly overlapping note on/off events

			//Build temp file
			/* open the file */
			if(trackcounter >= EOF_MUSIC_MIDI_TRACKS_MAX)
			{	//If this function attempted to store more tracks than it was prepared for
				allegro_message("Error:  Too many MIDI tracks, aborting MIDI export.");
				eof_log("Error:  Too many MIDI tracks, aborting MIDI export.", 1);
				eof_destroy_ks_list(kslist);		//Free memory used by the KS change list
				eof_destroy_ts_list(tslist);		//Free memory used by the TS change list
				eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
				eof_clear_midi_events();		//Free any memory allocated for the MIDI event array
				return 0;	//Return failure
			}
			fp = pack_fopen(notetempname[trackcounter], "w");
			if(!fp)
			{
				eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
				eof_destroy_ts_list(tslist);		//Free memory used by the TS change list
				eof_destroy_ks_list(kslist);		//Free memory used by the KS change list
				eof_clear_midi_events();			//Free any memory allocated for the MIDI event array
				eof_log("\tError saving:  Cannot open temporary MIDI track", 1);
				return 0;	//Return failure
			}

			/* determine the track name */
			arrangement_name = sp->track[j]->name;	//By default, the track's native name will be used
			if((sp->track[j]->flags & EOF_TRACK_FLAG_ALT_NAME) && (sp->track[j]->altname[0] != '\0'))
			{	//If this track has an alternate name defined
				arrangement_name = sp->track[j]->altname;	//Use it
			}
			else if(strcasestr_spec(sp->track[j]->name, "PART ") != NULL)
			{	//If this track's native name begins as expected
				arrangement_name = strcasestr_spec(sp->track[j]->name, "PART ");	//Use the end part of the name
			}

			/* write the track name */
			WriteVarLen(0, fp);
			(void) pack_putc(0xFF, fp);
			(void) pack_putc(0x03, fp);
			if(!passnum)
			{	//If writing a Synthesia style MIDI
				WriteVarLen(ustrsize(arrangement_name), fp);
				(void) pack_fwrite(arrangement_name, ustrsize(arrangement_name), fp);

				/* set the guitar/bass MIDI instrument as appropriate */
				if(eof_track_is_pro_guitar_track(sp, j))
				{	//If this is a pro guitar/bass track
					int tone = eof_midi_synth_instrument_guitar;	//By default, assume a guitar arrangement

					if(sp->pro_guitar_track[sp->track[j]->tracknum]->arrangement == EOF_BASS_ARRANGEMENT)
					{	//If this track's arrangement type is bass
						tone = eof_midi_synth_instrument_bass;	//Use the configured bass MIDI tone instead
					}
					WriteVarLen(0, fp);
					(void) pack_putc(0xC0 + channel, fp);	//Write MIDI event 0xC (Program change)
					(void) pack_putc(tone, fp);				//Write instrument number
				}
			}
			else
			{	//If writing a Fretlight style MIDI
				char prefix[] = "FMP - ";	//The required track name prefix

				WriteVarLen(ustrsize(arrangement_name) + ustrsize(prefix), fp);	//Include the extra number of characters needed for the prefix
				(void) pack_fwrite(prefix, ustrsize(prefix), fp);
				(void) pack_fwrite(arrangement_name, ustrsize(arrangement_name), fp);
			}

			/* add MIDI events */
			lastdelta = 0;
			for(i = 0; i < eof_midi_events; i++)
			{
				if(eof_midi_event[i]->filtered)
					continue;	//If this event is filtered, skip it

				delta = eof_midi_event[i]->pos;
				if(eof_midi_event[i]->type == 0x01)
				{	//Write a note name text event
					eof_write_text_event(delta-lastdelta, eof_midi_event[i]->dp, fp);
					lastevent = 0;	//Reset running status after a meta/sysex event
				}
				else if(eof_midi_event[i]->type == 0x05)
				{	//Write a lyric event
					eof_write_lyric_event(delta-lastdelta, eof_midi_event[i]->dp, fp);
					lastevent = 0;	//Reset running status after a meta/sysex event
				}
				else
				{	//Write normal MIDI note events
					WriteVarLen(delta-lastdelta, fp);	//Write this event's relative delta time
					if(eof_midi_event[i]->type + eof_midi_event[i]->channel != lastevent)
					{	//With running status, the MIDI event type needn't be written if it's the same as the previous event
						(void) pack_putc(eof_midi_event[i]->type + eof_midi_event[i]->channel, fp);
					}
					(void) pack_putc(eof_midi_event[i]->note, fp);
					(void) pack_putc(eof_midi_event[i]->velocity, fp);
					lastevent = eof_midi_event[i]->type + eof_midi_event[i]->channel;
				}
				if(eof_midi_event[i]->allocation && eof_midi_event[i]->dp)
				{	//If this event has allocated memory to release
					free(eof_midi_event[i]->dp);	//Free it now
					eof_midi_event[i]->dp = NULL;
					eof_midi_event[i]->allocation = 0;
				}
				lastdelta = delta;					//Store this event's absolute delta time
			}

			/* write end of track */
			WriteVarLen(0, fp);
			(void) pack_putc(0xFF, fp);
			(void) pack_putc(0x2F, fp);
			(void) pack_putc(0x00, fp);
			(void) pack_fclose(fp);

			trackcounter++;	//Count this track towards the number of tracks to write to the completed MIDI
			channel++;	//For Synthesia format MIDIs, the next track to export will use a different MIDI channel for the note on/off events

			eof_menu_track_set_tech_view_state(sp, j, restore_tech_view);	//Re-enable tech view if applicable
		}//For each track in the project
	}//Parse each track twice

	eof_clear_midi_events();		//Free any memory allocated for the MIDI event array

/* make tempo track */
	fp = pack_fopen(tempotempname, "w");
	if(!fp)
	{
		eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
		eof_destroy_ts_list(tslist);		//Free memory used by the TS change list
		eof_destroy_ks_list(kslist);		//Free memory used by the KS change list
		eof_clear_midi_events();			//Free any memory allocated for the MIDI event array
		eof_log("\tError saving:  Cannot open temporary MIDI track", 1);
		return 0;	//Return failure
	}
	eof_write_tempo_track(NULL, anchorlist, tslist, kslist, fp);	//Write the tempo track to the temp file
	(void) pack_fclose(fp);

/* make events track */
	if(sp->text_events)
	{	//If there are manually defined text events
		fp = pack_fopen(eventtempname, "w");
		if(!fp)
		{
			eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
			eof_destroy_ts_list(tslist);		//Free memory used by the TS change list
			eof_destroy_ks_list(kslist);		//Free memory used by the KS change list
			eof_log("\tError saving:  Cannot open temporary MIDI track", 1);
			return 0;	//Return failure
		}

		/* write the track name */
		WriteVarLen(0, fp);
		(void) pack_putc(0xFF, fp);
		(void) pack_putc(0x03, fp);
		WriteVarLen(ustrsize("EVENTS"), fp);
		(void) pack_fwrite("EVENTS", ustrsize("EVENTS"), fp);

		/* add MIDI events */
		lastdelta = 0;
		for(i = 0; i < sp->text_events; i++)
		{
			if(sp->text_event[i]->track == 0)
			{	//If the text event is global (not specific to any single track)
				double eventpos;

				if(!(sp->text_event[i]->flags & EOF_EVENT_FLAG_FLOATING_POS))
				{	//If this text event is assigned to a beat marker
					if(sp->text_event[i]->pos >= sp->beats)
					{	//If the text event is corrupted
						sp->text_event[i]->pos = sp->beats - 1;	//Repair it by assigning it to the last beat marker
					}
				}
				eventpos = eof_get_text_event_fpos(sp, i);
				delta = eof_ConvertToDeltaTime(eventpos, anchorlist, tslist, timedivision, 1, has_stored_tempo);
				eof_write_text_event(delta - lastdelta, sp->text_event[i]->text, fp);
				lastdelta = delta;					//Store this event's absolute delta time
			}
		}

		/* end of track */
		WriteVarLen(0, fp);
		(void) pack_putc(0xFF, fp);
		(void) pack_putc(0x2F, fp);
		(void) pack_putc(0x00, fp);

		(void) pack_fclose(fp);
		eventstrackwritten = 1;
	}//If there are manually defined text events, or if writing a RBN2 compliant MIDI (which requires certain events)

/* write the main MIDI file */
	fp = pack_fopen(fn, "w");
	if(!fp)
	{
		eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
		eof_destroy_ts_list(tslist);		//Free memory used by the TS change list
		eof_destroy_ks_list(kslist);		//Free memory used by the KS change list
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError saving:  Cannot open output MIDI file:  \"%s\"", strerror(errno));	//Get the Operating System's reason for the failure
		eof_log(eof_log_string, 1);
		return 0;	//Return failure
	}

	/* write header data */
	totaltrackcounter = trackcounter + 1 + eventstrackwritten;		//Add 1 for track 0 and one for the events tracks if applicable

	for(trackptr = sp->midi_data_head; trackptr != NULL; trackptr = trackptr->next)	//Add the number of raw MIDI tracks to export to the total track counter
	{	//For each stored MIDI track
		if(!trackptr->timedivision)	//If the stored track does not contain a time division (is not a stored tempo track)
			totaltrackcounter++;			//Count it toward the number of tracks to be exported (track 0 is already counted)
	}
	header[11] = totaltrackcounter;	//Write the total number of tracks present into the MIDI header
	(void) pack_fwrite(header, 14, fp);

/* write tempo track */
	(void) eof_dump_midi_track(tempotempname,fp);

/* write text event track if there are any events */
	if(sp->text_events)
	{	//If there are manually defined text events
		(void) eof_dump_midi_track(eventtempname,fp);
	}

/* write the remaining tracks */
	for(j = 0; j < trackcounter; j++)
	{	//For each track that was written to a temporary file
		(void) eof_dump_midi_track(notetempname[j],fp);
	}
	eof_MIDI_data_track_export(sp, fp, anchorlist, tslist, timedivision);	//Write any stored raw MIDI tracks to the output file

/* cleanup */
	(void) pack_fclose(fp);	//Close the output file
	for(i = 0; i < trackcounter; i++)
	{	//For each temporary file that was created
		(void) delete_file(notetempname[i]);
	}
	(void) delete_file(tempotempname);
	(void) delete_file(eventtempname);

	eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
	eof_destroy_ts_list(tslist);		//Free memory used by the TS change list
	eof_destroy_ks_list(kslist);		//Free memory used by the KS change list

	return 1;	//Return success
}

int eof_export_llplus_midi(char *fn)
{
	unsigned char header[14] = {'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 1, 0, 1, (EOF_DEFAULT_TIME_DIVISION >> 8), (EOF_DEFAULT_TIME_DIVISION & 0xFF)}; //The last two bytes are the time division
	unsigned long timedivision = EOF_DEFAULT_TIME_DIVISION;	//Unless the project is storing a tempo track, EOF's default time division will be used
	struct Tempo_change *anchorlist=NULL;	//Linked list containing tempo changes
	PACKFILE * fp;
	unsigned long i, deltapos, deltalength, channelctr;
	unsigned long delta = 0;
	int vel = 64;
	unsigned long lastdelta=0;				//Keeps track of the last anchor's absolute delta time
	EOF_MIDI_TS_LIST *tslist=NULL;			//List containing TS changes
	EOF_MIDI_KS_LIST *kslist;
	char tempotempname[30];
	char notetempname[30];
	char has_stored_tempo;		//Will be set to nonzero if the project contains a stored tempo track, which will affect timing conversion
	int lastevent = 0;	//Track the last event written so running status can be utilized
	char *arrangement_name;

	eof_log("eof_export_llplus_midi() entered", 1);

	if(!eof_song || !fn)
	{
		eof_log("\tError saving:  Invalid parameters", 1);
		return 0;	//Return failure
	}

	(void) eof_detect_difficulties(eof_song, eof_selected_track);	//Update eof_track_diff_populated_status[] to reflect all populated difficulties for the active track
	if(!eof_track_diff_populated_status[eof_note_type])
		return 0;	//Empty track difficulty

	if(eof_validate_temp_folder())
	{	//Ensure the correct working directory and presence of the temporary folder
		eof_log("\tCould not validate working directory and temp folder", 1);
		return 0;	//Return failure
	}

	if(!eof_calculate_beat_delta_positions(eof_song, EOF_DEFAULT_TIME_DIVISION))
	{	//Calculate the delta position of each beat in the chart
		eof_log("\tCould not build beat delta positions", 1);
		return 0;	//Return failure
	}

	has_stored_tempo = eof_song_has_stored_tempo_track(eof_song) ? 1 : 0;	//Store this status

	//Build tempo and TS lists
	if(!eof_build_tempo_and_ts_lists(eof_song, &anchorlist, &tslist, &timedivision))
	{
		eof_log("\tError saving:  Cannot build tempo or TS list", 1);
		return 0;	//Return failure
	}
	kslist = eof_build_ks_list(eof_song);		//Build a list of key signature changes
	if(!kslist)
	{
		eof_log("\tError saving:  Cannot build key signature list", 1);
		eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
		eof_destroy_ts_list(tslist);		//Free memory used by the TS change list
		return 0;	//Return failure
	}
	header[12] = timedivision >> 8;		//Update the MIDI header to reflect the time division (which may have changed if a stored tempo track is present)
	header[13] = timedivision & 0xFF;

	//Generate temporary filenames
	(void) snprintf(tempotempname, sizeof(tempotempname) - 1, "%stempo.tmp", eof_temp_path_s);
	(void) snprintf(notetempname, sizeof(notetempname) - 1, "%snotes.tmp", eof_temp_path_s);

	eof_sort_notes(eof_song);
	eof_clear_midi_events();
	memset(eof_midi_note_status,0,sizeof(eof_midi_note_status));	//Clear note status array

	//Write notes
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note
		unsigned long pos, length;
		unsigned char note;

		if(eof_get_note_type(eof_song, eof_selected_track, i) != eof_note_type)
			continue;	//If this note isn't in the active track difficulty, skip it

		pos = eof_get_note_pos(eof_song, eof_selected_track, i);
		length = eof_get_note_length(eof_song, eof_selected_track, i);
		note = eof_get_note_note(eof_song, eof_selected_track, i);
		deltapos = eof_ConvertToDeltaTime(pos, anchorlist, tslist, timedivision, 1, has_stored_tempo);
		deltalength = eof_ConvertToDeltaTime(pos + length, anchorlist, tslist, timedivision, 0, has_stored_tempo) - deltapos;
		if(deltalength < 1)
		{	//If some kind of rounding error or other issue caused the delta length to be less than 1, force it to the minimum length of 1
			deltalength = 1;
		}

		if(note & 1)
		{	//Left lane gem
			eof_add_midi_event(deltapos, 0x90, 38, vel, 0);		//Note 38 is a left note
			eof_add_midi_event(deltapos + deltalength, 0x80, 38, vel, 0);

			if(length > 1)
			{	//Hold notes also require another MIDI note
				eof_add_midi_event(deltapos, 0x90, 39, vel, 0);		//Note 39 is a left hold note
				eof_add_midi_event(deltapos + deltalength, 0x80, 39, vel, 0);
			}
		}

		if(note & 2)
		{
			eof_add_midi_event(deltapos, 0x90, 36, vel, 0);		//Note 36 is a right note
			eof_add_midi_event(deltapos + deltalength, 0x80, 36, vel, 0);

			if(length > 1)
			{	//Hold notes also require another MIDI note
				eof_add_midi_event(deltapos, 0x90, 35, vel, 0);		//Note 35 is a right hold note
				eof_add_midi_event(deltapos + deltalength, 0x80, 35, vel, 0);
			}
		}

		if(eof_midi_event_full)
		{	//If the track exceeded the number of MIDI events that could be written
			allegro_message("Error:  Too many MIDI events, aborting MIDI export.");
			eof_log("Error:  Too many MIDI events, aborting MIDI export.", 1);
			eof_destroy_ks_list(kslist);		//Free memory used by the KS change list
			eof_destroy_ts_list(tslist);		//Free memory used by the TS change list
			eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
			eof_clear_midi_events();		//Free any memory allocated for the MIDI event array
			return 0;	//Return failure
		}
	}//For each note

	//Cleanup
	for(channelctr = 0; channelctr < 16; channelctr++)
	{	//For each of the usable MIDI channels
		for(i = 0; i < 128; i++)
		{	//Ensure that any notes that are still on are terminated
			if(eof_midi_note_status[channelctr][i] == 0)
				continue;	//If this note was not left on, skip it

			//Otherwise send an alert message, as this is abnormal
			allegro_message("MIDI export error:  Note %lu was not turned off", i);
			eof_log("MIDI export error:  Note %lu was not turned off", 1);
			if(eof_midi_endbeatnum)
			{	//If the chart has a manually defined end event, that's probably the cause
				eof_clear_input();
				eof_log("\tend event was manually defined", 1);
				if(alert("It appears this is due to an [end] event that cuts out a note early", "Would you like to seek to the beat containing this [end] event?", NULL, "&Yes", "&No", 'y', 'n') == 1)
				{	//If user opts to seek to the offending event
					eof_set_seek_position(eof_song->beat[eof_midi_endbeatnum]->pos + eof_av_delay);
					eof_selected_beat = eof_midi_endbeatnum;
				}
			}
			eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
			eof_destroy_ts_list(tslist);		//Free memory used by the TS change list
			eof_destroy_ks_list(kslist);		//Free memory used by the KS change list
			eof_clear_midi_events();		//Free any memory allocated for the MIDI event array
			return 0;	//Return failure
		}
	}
	qsort(eof_midi_event, (size_t)eof_midi_events, sizeof(EOF_MIDI_EVENT *), qsort_helper3);
	eof_check_for_note_overlap();	//Filter out any improperly overlapping note on/off events

	//Build temp file
	/* open the file */
	fp = pack_fopen(notetempname, "w");
	if(!fp)
	{
		eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
		eof_destroy_ts_list(tslist);		//Free memory used by the TS change list
		eof_destroy_ks_list(kslist);		//Free memory used by the KS change list
		eof_clear_midi_events();			//Free any memory allocated for the MIDI event array
		eof_log("\tError saving:  Cannot open temporary MIDI track", 1);
		return 0;	//Return failure
	}

	/* determine the track name */
	arrangement_name = eof_song->track[eof_selected_track]->name;	//By default, the track's native name will be used
	if((eof_song->track[eof_selected_track]->flags & EOF_TRACK_FLAG_ALT_NAME) && (eof_song->track[eof_selected_track]->altname[0] != '\0'))
	{	//If this track has an alternate name defined
		arrangement_name = eof_song->track[eof_selected_track]->altname;	//Use it
	}
	else if(strcasestr_spec(eof_song->track[eof_selected_track]->name, "PART ") != NULL)
	{	//If this track's native name begins as expected
		arrangement_name = strcasestr_spec(eof_song->track[eof_selected_track]->name, "PART ");	//Use the end part of the name
	}

	/* write the track name */
	WriteVarLen(0, fp);
	(void) pack_putc(0xFF, fp);
	(void) pack_putc(0x03, fp);
	WriteVarLen(ustrsize(arrangement_name), fp);	//Include the extra number of characters needed for the prefix
	(void) pack_fwrite(arrangement_name, ustrsize(arrangement_name), fp);

	/* add MIDI events */
	lastdelta = 0;
	for(i = 0; i < eof_midi_events; i++)
	{
		if(eof_midi_event[i]->filtered)
			continue;	//If this event is filtered, skip it

		delta = eof_midi_event[i]->pos;
		WriteVarLen(delta-lastdelta, fp);	//Write this event's relative delta time
		if(eof_midi_event[i]->type + eof_midi_event[i]->channel != lastevent)
		{	//With running status, the MIDI event type needn't be written if it's the same as the previous event
			(void) pack_putc(eof_midi_event[i]->type + eof_midi_event[i]->channel, fp);
		}
		(void) pack_putc(eof_midi_event[i]->note, fp);
		(void) pack_putc(eof_midi_event[i]->velocity, fp);
		lastevent = eof_midi_event[i]->type + eof_midi_event[i]->channel;
		lastdelta = delta;					//Store this event's absolute delta time
	}

	/* write end of track */
	WriteVarLen(0, fp);
	(void) pack_putc(0xFF, fp);
	(void) pack_putc(0x2F, fp);
	(void) pack_putc(0x00, fp);
	(void) pack_fclose(fp);

	eof_clear_midi_events();		//Free any memory allocated for the MIDI event array

/* make tempo track */
	fp = pack_fopen(tempotempname, "w");
	if(!fp)
	{
		eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
		eof_destroy_ts_list(tslist);		//Free memory used by the TS change list
		eof_destroy_ks_list(kslist);		//Free memory used by the KS change list
		eof_clear_midi_events();			//Free any memory allocated for the MIDI event array
		eof_log("\tError saving:  Cannot open temporary MIDI track", 1);
		return 0;	//Return failure
	}
	eof_write_tempo_track(NULL, anchorlist, tslist, kslist, fp);	//Write the tempo track to the temp file
	(void) pack_fclose(fp);

/* write the main MIDI file */
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tWriting MIDI file:  \"%s\"", fn);	//Get the Operating System's reason for the failure
	eof_log(eof_log_string, 1);
	fp = pack_fopen(fn, "w");
	if(!fp)
	{
		eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
		eof_destroy_ts_list(tslist);			//Free memory used by the TS change list
		eof_destroy_ks_list(kslist);		//Free memory used by the KS change list
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError saving:  Cannot open output MIDI file:  \"%s\"", strerror(errno));	//Get the Operating System's reason for the failure
		eof_log(eof_log_string, 1);
		return 0;	//Return failure
	}

	/* write header data */
	header[11] = 2;	//Write the total number of tracks present into the MIDI header
	(void) pack_fwrite(header, 14, fp);

/* write tempo track */
	(void) eof_dump_midi_track(tempotempname,fp);

/* write the note track */
	(void) eof_dump_midi_track(notetempname,fp);

/* cleanup */
	(void) pack_fclose(fp);	//Close the output file
	(void) delete_file(notetempname);
	(void) delete_file(tempotempname);

	eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
	eof_destroy_ts_list(tslist);		//Free memory used by the TS change list
	eof_destroy_ks_list(kslist);		//Free memory used by the KS change list

	return 1;	//Return success
}

struct Tempo_change *eof_build_tempo_list(EOF_SONG *sp)
{
	unsigned long ctr;
	struct Tempo_change *list=NULL;	//The linked list
	struct Tempo_change *temp=NULL;
	unsigned long lastppqn=0;	//Tracks the last anchor's PPQN value
	unsigned long deltapos = 0;		//Stores the ongoing delta time
	double deltafpos = 0.0;			//Stores the ongoing delta time (with double floating precision)
	double beatlength = EOF_DEFAULT_TIME_DIVISION;		//Stores the current beat's length in deltas
	unsigned num=4,den=4;			//Stores the current time signature

	eof_log("eof_build_tempo_list() entered", 1);

	if((sp == NULL) || (sp->beats < 1))
	{
		return NULL;
	}
	for(ctr=0;ctr < sp->beats;ctr++)
	{	//For each beat
		(void) eof_get_ts(sp, &num, &den, ctr);	//Get the time signature in effect
		if(sp->beat[ctr]->ppqn != lastppqn)
		{	//If this beat has a different tempo than the last, or it is the first beat, add it to the list
			lastppqn=sp->beat[ctr]->ppqn;	//Remember this ppqn
			temp=eof_add_to_tempo_list(deltapos, sp->beat[ctr]->fpos, 60000000.0 / lastppqn, list);

			if(temp == NULL)
			{	//Test the return value of eof_add_to_tempo_list()
				eof_destroy_tempo_list(list);	//Destroy list
				return NULL;			//Return error
			}
			list=temp;	//Update list pointer
		}

		if(sp->tags->accurate_ts)
		{	//If accurate time signatures are to be observed for export
			if(den != 4)
			{	//If the time signature necessitates adjustment (isn't #/4)
				beatlength = (double)EOF_DEFAULT_TIME_DIVISION / ((double)den / 4.0);		//Determine the length of this beat in deltas (taking the time signature into consideration)
			}
			else
			{
				beatlength = EOF_DEFAULT_TIME_DIVISION;
			}
		}
		deltafpos += beatlength;	//Add the delta length of this beat to the delta counter
		deltapos = deltafpos + 0.5;	//Round up to nearest delta
	}

	return list;
}

struct Tempo_change *eof_add_to_tempo_list(unsigned long delta, double realtime, double BPM, struct Tempo_change *ptr)
{
	struct Tempo_change *temp;
	struct Tempo_change *cond = NULL;	//A conductor for the linked list

	if(eof_log_level >= 3)
	{
		eof_log("eof_add_to_tempo_list() entered", 3);

		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tAdding tempo change:  Delta = %lu, pos = %f, BPM = %f", delta, realtime, BPM);
		eof_log(eof_log_string, 3);
	}

//Allocate and initialize new link
	temp = (struct Tempo_change *)malloc(sizeof(struct Tempo_change));
	if(temp == NULL)
	{
		return NULL;
	}
	temp->delta = delta;
	temp->realtime = realtime;
	temp->BPM = BPM;
	temp->next = NULL;
	temp->TS_den = temp->TS_num = 4;

//Append to linked list
	if(ptr == NULL)		//If the passed list was empty
	{
		return temp;	//Return the new head link
	}
	for(cond = ptr; cond->next != NULL; cond = cond->next);	//Seek to last link in the list

	cond->next = temp;	//Last link points forward to new link
	return ptr;			//Return original head link
}

void eof_destroy_tempo_list(struct Tempo_change *ptr)
{
	struct Tempo_change *temp = NULL;

	eof_log("eof_destroy_tempo_list() entered", 1);

	while(ptr != NULL)
	{
		temp=ptr->next;	//Store this pointer
		free(ptr);	//Free this link
		ptr=temp;	//Point to next link
	}
}

unsigned long eof_ConvertToDeltaTime(double realtime, struct Tempo_change *anchorlist, EOF_MIDI_TS_LIST *tslist, unsigned long timedivision, char snaptobeat, char disablequantize)
{	//Uses the Tempo Changes list to calculate the absolute delta time of the specified realtime
//	eof_log("eof_ConvertToDeltaTime() entered");

	struct Tempo_change *temp = anchorlist;	//Stores the closest tempo change before the specified realtime
	double tstime = 0.0;					//Stores the realtime position of the closest TS change before the specified realtime
	unsigned long tsdelta = 0;				//Stores the delta time position of the closest TS change before the specified realtime
	unsigned long delta = 0;
	double reltime = 0.0;
	unsigned long ctr;
	unsigned long realtimeint = realtime;	//The integer representation of the target timestamp is used in the grid snap matching logic
	unsigned long beatnum, gridsnapnum, interval;

	assert_wrapper(temp != NULL);	//Ensure the tempomap is populated

	//Determine if a clean delta value can be obtained with integer math, depending on whether the specified timestamp falls within
	//a 1/# beat interval where the beat's delta length is divisible by #
	if(!disablequantize)
	{	//Only do this if the calling function didn't prevent it
		beatnum = eof_get_beat(eof_song, realtime + 0.5);	//Round to nearest ms to compare against integer beat timings
		if(beatnum < eof_song->beats)
		{	//If the beat containing this timestamp was identified
			double beatlength = eof_calc_beat_length(eof_song, beatnum);
			if(beatlength >= 1.0)
			{	//If a suitable beat length was obtained
				unsigned num = 4, den = 4;

				if(eof_get_effective_ts(eof_song, &num, &den, beatnum, 0))
				{	//If the time signature of the target position's beat was obtained
					unsigned long beatlength_delta = EOF_DEFAULT_TIME_DIVISION * 4 / den;	//One beat is considered to be (EOF_DEFAULT_TIME_DIVISION * 4 / TS_DENOMINATOR) delta ticks in length
					unsigned long beat_deltapos = eof_song->beat[beatnum]->midi_pos;

					if(beat_deltapos < ULONG_MAX)
					{	//If the delta time of the target position's beat was found
						for(interval = 2; interval < EOF_MAX_GRID_SNAP_INTERVALS; interval++)
						{	//Check all of the possible supported custom grid snap intervals
							double snaplength;

							if(beatlength_delta % interval != 0)
								continue;	//If the beat's delta length isn't divisible by this interval, skip it

							snaplength = beatlength / (double) interval;	//Determine the real time length of one such grid snap interval
							for(gridsnapnum = 0; gridsnapnum < interval; gridsnapnum++)
							{	//For each instance of that grid snap
								unsigned long gridsnappos = (eof_song->beat[beatnum]->fpos + (double)gridsnapnum * snaplength) + 0.5;
								if((realtimeint == gridsnappos) || (realtimeint + 1 == gridsnappos) || (realtimeint == gridsnappos + 1))
								{	//If the target timestamp matches this grid snap position, or is within 1ms to account for rounding error
									return beat_deltapos + (gridsnapnum * beatlength_delta / interval);
								}
							}
						}
					}
				}
			}
		}
	}

//Find the last time signature change at or before the specified real time value
	if((tslist != NULL) && (tslist->changes > 0))
	{	//If there's at least one TS change
		for(ctr = 0; ctr < tslist->changes; ctr++)
		{
			if(realtime >= tslist->change[ctr]->realtime)
			{	//If the TS change is at or before the target realtime
				tstime = tslist->change[ctr]->realtime;		//Store the realtime position
				tsdelta = tslist->change[ctr]->pos;			//Store the delta time position
			}
		}
	}

//Find the last tempo change at or before the specified real time value
	while((temp->next != NULL) && (realtime >= (temp->next)->realtime))	//For each tempo change,
	{	//If the tempo change is at or before the target realtime
		temp = temp->next;	//Advance to that time stamp
	}

//Find the latest tempo or TS change that occurs before the target realtime position and use that event's timing for the conversion
	if(tstime > temp->realtime)
	{	//If the TS change is closer to the target realtime, find the delta time relative from this event
		delta = tsdelta;				//Store the absolute delta time for this TS change
		reltime = realtime - tstime;	//Find the relative timestamp from this TS change
	}
	else
	{	//Find the delta time relative from the closest tempo change
		delta = temp->delta;	//Store the absolute delta time for the anchor
		reltime = realtime - temp->realtime;	//Find the relative timestamp from this tempo change
	}

//reltime is the amount of time we need to find a relative delta for, and add to the absolute delta time of the nearest preceding tempo/TS change
	delta += (unsigned long)((reltime * (double)timedivision * temp->BPM / 60000.0) + 0.5);

//Add logic so that if the calculated delta time is 1 delta away from lining up with a beat marker (based on time division), adjust to match
	if(snaptobeat)
	{
		if((delta % timedivision) == (timedivision - 1))
		{	//If the delta time is 1 tick before a beat marker
			delta++;
		}
		else if((delta % timedivision) == 1)
		{	//If the delta time is 1 tick after a beat marker
			delta--;
		}
	}

	return delta;
}

int eof_calculate_beat_delta_positions(EOF_SONG *sp, unsigned long timedivision)
{
	unsigned long ctr, counter = 0;
	unsigned num = 4, den = 4;

	if(!sp)
		return 0;	//Invalid parameters

	for(ctr = 0; ctr < sp->beats; ctr++)
	{	//For every beat in the chart
		sp->beat[ctr]->midi_pos = counter;	//The current delta tick counter represents this beat's delta position

		if(eof_get_ts(sp, &num, &den, ctr) == -1)
		{	//If the beat's time signature could not be read
			allegro_message("eof_calculate_beat_delta_positions() error");
			return 0;	//Error
		}

		counter += (timedivision * 4) / den;	//Add this beat's length (in delta ticks) to the counter
	}

	return 1;
}

int eof_extract_rba_midi(const char * source, const char * dest)
{
	FILE *fp = NULL;
	FILE *tempfile = NULL;
	unsigned long ctr = 0;
	int jumpcode = 0;
	char buffer[15] = {0};

	eof_log("eof_extract_rba_midi() entered", 1);

//Open specified file
	if((source == NULL) || (dest == NULL))
	{
		return 1;	//Return failure
	}
	fp = fopen(source,"rb");
	if(fp == NULL)
	{
		return 1;	//Return failure
	}

//Set up for catching an exception thrown by FoFLC's logic in the event of an error (such as an invalid MIDI file)
	jumpcode = setjmp(jumpbuffer); //Store environment/stack/etc. info in the jmp_buf array
	if(jumpcode != 0) //if program control returned to the setjmp() call above returning any nonzero value
	{
		(void) puts("Assert() handled successfully!");
		eof_show_mouse(NULL);
		eof_cursor_visible = 1;
		eof_pen_visible = 1;
		(void) fclose(fp);
		ReleaseMIDI();
		ReleaseMemory(1);
		return 1;	//Return failure
	}

//Load MIDI information (parsing the RBA header if present)
	ReleaseMIDI();		//Ensure FoFLC variables are reset
	ReleaseMemory(1);
	InitMIDI();
	InitLyrics();
	Lyrics.quick = 1;		//Should be fine to skip everything except loading basic track info
	MIDI_Load(fp, NULL, 0);
	if(MIDIstruct.hchunk.numtracks)
	{	//If at least one valid MIDI track was parsed
//Copy MIDI contents into file
		tempfile = fopen(dest,"wb");
		if(tempfile != NULL)
		{	//Seek to MIDI header and begin copying content
			rewind(fp);
			if(SearchPhrase(fp, 0, NULL, "MThd", 4, 1) != 1)	//Search for and seek to MIDI header
			{
				(void) fclose(tempfile);
				(void) fclose(fp);
				ReleaseMIDI();
				ReleaseMemory(1);
				return 1;	//Return error
			}

			//Copy the file header
			fread_err(buffer, 14, 1, fp);			//Read MIDI header
			fwrite_err(buffer, 14, 1, tempfile);	//Write MIDI header

			//Copy tracks
			for(ctr = 0; ctr < MIDIstruct.hchunk.numtracks; ctr++)	//For each track
			{
				CopyTrack(fp,ctr,tempfile);
			}
			(void) fclose(tempfile);
		}
		ReleaseMIDI();
		ReleaseMemory(1);
	}

	(void) fclose(fp);
	return 0;	//Return success
}

EOF_MIDI_TS_LIST * eof_create_ts_list(void)
{
	unsigned long ctr;
	EOF_MIDI_TS_LIST * lp;

	eof_log("eof_create_ts_list() entered", 1);

	lp = malloc(sizeof(EOF_MIDI_TS_LIST));
	if(!lp)
	{
		eof_log("\tMemory allocation failed", 1);
		return NULL;
	}

	eof_log("\tInitializing entries", 3);
	for(ctr = 0; ctr < EOF_MAX_TS; ctr++)
	{	//Init all pointers in the array as NULL
		lp->change[ctr] = NULL;
	}
	lp->changes = 0;

	eof_log("\teof_create_ts_list() completed", 3);
	return lp;
}

void eof_midi_add_ts_deltas(EOF_MIDI_TS_LIST * changes, unsigned long pos, unsigned long num, unsigned long den, unsigned long track)
{
//	eof_log("eof_midi_add_ts_deltas() entered");

	if(changes && (changes->changes < EOF_MAX_TS) && (track == 0))
	{	//For now, only store time signatures in track 0
		changes->change[changes->changes] = malloc(sizeof(EOF_MIDI_TS_CHANGE));
		if(changes->change[changes->changes])
		{
			changes->change[changes->changes]->pos = pos;		//Store the TS change's delta time
			changes->change[changes->changes]->num = num;		//Store the TS numerator
			changes->change[changes->changes]->den = den;		//Store the TS denominator
			changes->change[changes->changes]->realtime = 0;	//Will be set later, once all tempo changes are parsed
			changes->change[changes->changes]->track = track;	//Store the event's track number

			changes->changes++;				//Increment counter
		}
	}
}

void eof_midi_add_ts_realtime(EOF_MIDI_TS_LIST * changes, double pos, unsigned long num, unsigned long den, unsigned long track)
{
	eof_log("eof_midi_add_ts_realtime() entered", 1);

	if(changes && (changes->changes < EOF_MAX_TS) && (track == 0))
	{	//For now, only store time signatures in track 0
		changes->change[changes->changes] = malloc(sizeof(EOF_MIDI_TS_CHANGE));
		if(changes->change[changes->changes])
		{
			changes->change[changes->changes]->pos = 0;			//Will be set later
			changes->change[changes->changes]->num = num;		//Store the TS numerator
			changes->change[changes->changes]->den = den;		//Store the TS denominator
			changes->change[changes->changes]->realtime = pos;	//Store the realtime stamp
			changes->change[changes->changes]->track = track;	//Store the event's track number

			changes->changes++;				//Increment counter
		}
	}
}

void eof_destroy_ts_list(EOF_MIDI_TS_LIST *ptr)
{
	unsigned long i;

	eof_log("eof_destroy_ts_list() entered", 1);

	if(ptr != NULL)
	{
		for(i = 0; i < ptr->changes; i++)
		{
			free(ptr->change[i]);
		}
		free(ptr);
	}
}

EOF_MIDI_TS_LIST *eof_build_ts_list(EOF_SONG *sp)
{
	unsigned long ctr;
	unsigned num = 4, den = 4;			//Stores the current time signature
	EOF_MIDI_TS_LIST * tslist = NULL;
	unsigned long deltapos = 0;		//Stores the ongoing delta time
	double deltafpos = 0.0;			//Stores the ongoing delta time (with double floating precision)
	double beatlength = 0.0;		//Stores the current beat's length in deltas

	eof_log("eof_build_ts_list() entered", 1);

	if((sp == NULL) || (sp->beats == 0))
		return NULL;
	tslist = eof_create_ts_list();
	if(tslist == NULL)
		return NULL;

	for(ctr = 0; ctr < sp->beats; ctr++)
	{	//For each beat, create a list of Time Signature changes and store the appropriate delta position of each
		if(eof_get_ts(sp, &num, &den, ctr) == 1)
		{	//If a time signature exists on this beat
			eof_midi_add_ts_realtime(tslist, sp->beat[ctr]->fpos, num, den, 0);	//Store the beat marker's time signature
			tslist->change[tslist->changes-1]->pos = deltapos;	//Store the time signature's position in deltas
		}

		beatlength = (double)EOF_DEFAULT_TIME_DIVISION / ((double)den / 4.0);		//Determine the length of this beat in deltas (taking the time signature into consideration)
		deltafpos += beatlength;	//Add the delta length of this beat to the delta counter
		deltapos = deltafpos + 0.5;	//Round up to nearest delta
	}

	return tslist;
}

int eof_get_ts(EOF_SONG *sp, unsigned *num, unsigned *den, unsigned long beatnum)
{
//	eof_log("eof_get_ts() entered");

	unsigned numerator = 0, denominator = 0;

	if((sp == NULL) || (beatnum >= sp->beats) || (sp->beat[beatnum] == NULL))
		return -1;	//Return error

	if(sp->beat[beatnum]->flags & EOF_BEAT_FLAG_START_4_4)
	{
		numerator = 4;
		denominator = 4;
	}
	else if(sp->beat[beatnum]->flags & EOF_BEAT_FLAG_START_2_4)
	{
		numerator = 2;
		denominator = 4;
	}
	else if(sp->beat[beatnum]->flags & EOF_BEAT_FLAG_START_3_4)
	{
		numerator = 3;
		denominator = 4;
	}
	else if(sp->beat[beatnum]->flags & EOF_BEAT_FLAG_START_5_4)
	{
		numerator = 5;
		denominator = 4;
	}
	else if(sp->beat[beatnum]->flags & EOF_BEAT_FLAG_START_6_4)
	{
		numerator = 6;
		denominator = 4;
	}
	else if(sp->beat[beatnum]->flags & EOF_BEAT_FLAG_CUSTOM_TS)
	{
		numerator = ((sp->beat[beatnum]->flags & 0xFF000000)>>24) + 1;
		denominator = ((sp->beat[beatnum]->flags & 0x00FF0000)>>16) + 1;
	}
	else
		return 0;	//Return no TS change

	if(num)
		*num = numerator;
	if(den)
		*den = denominator;

	return 1;	//Return success
}

int eof_get_effective_ts(EOF_SONG *sp, unsigned *num, unsigned *den, unsigned long beatnum, int function)
{
	unsigned numerator = 4, denominator = 4;

	if((sp == NULL) || (beatnum >= sp->beats) || (sp->beat[beatnum] == NULL))
		return 0;	//Return error

	if(function)
	{	//If the calling function does not want to assume 4/4 in the absence of a defined time signature
		numerator = denominator = 0;
	}
	while(eof_get_ts(sp, &numerator, &denominator, beatnum) != 1)
	{	//Until a TS change is seen, check each beat in reverse, from the selected beat to the first one
		if(beatnum == 0)
		{	//If this is the first beat
			break;	//There are no other beats to check
		}
		beatnum--;
	}

	if(num)
		*num = numerator;
	if(den)
		*den = denominator;

	return 1;	//Return success
}

int eof_apply_ts(unsigned num, unsigned den, unsigned long beatnum, EOF_SONG *sp, char undo)
{
//	eof_log("eof_apply_ts() entered");

	unsigned long flags = 0;

	if((sp == NULL) || (beatnum >= sp->beats))
		return 0;	//Invalid parameters
	if((num == 0) || (num > 256) || ((den != 1) && (den != 2) && (den != 4) && (den != 8) && (den != 16) && (den != 32) && (den != 64) && (den != 128) && (den != 256)))
		return 0;	//If a valid time signature wasn't specified, return error

	//Clear the beat's status except for its anchor, event and key signature flags
	flags = sp->beat[beatnum]->flags & EOF_BEAT_FLAG_ANCHOR;
	flags |= sp->beat[beatnum]->flags & EOF_BEAT_FLAG_EVENTS;
	flags |= sp->beat[beatnum]->flags & EOF_BEAT_FLAG_KEY_SIG;

	if((num == 2) && (den == 4))
	{
		flags = flags | EOF_BEAT_FLAG_START_2_4;
	}
	else if((num == 3) && (den == 4))
	{
		flags = flags | EOF_BEAT_FLAG_START_3_4;
	}
	else if((num == 4) && (den == 4))
	{
		flags = flags | EOF_BEAT_FLAG_START_4_4;
	}
	else if((num == 5) && (den == 4))
	{
		flags = flags | EOF_BEAT_FLAG_START_5_4;
	}
	else if((num == 6) && (den == 4))
	{
		flags = flags | EOF_BEAT_FLAG_START_6_4;
	}
	else
	{
		num--;	//Convert these numbers to a system where all bits zero represents 1 and all bits set represents 256
		den--;
		flags = flags | EOF_BEAT_FLAG_CUSTOM_TS;
		flags |= (num << 24);
		flags |= (den << 16);
	}
	if(flags != sp->beat[beatnum]->flags)
	{	//If the application of this time signature really changes the beat's flags
		if(undo)	//If calling function specified to make an undo state (would be if applying the change to the active project)
		{
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make an undo state
			eof_beat_stats_cached = 0;				//Mark the cached beat stats as not current
		}
		sp->beat[beatnum]->flags = flags;	//Apply the flag changes
	}

	return 1;
}

int eof_dump_midi_track(const char *inputfile, PACKFILE *outf)
{
	unsigned long track_length;
	PACKFILE *inf = NULL;
	char trackheader[8] = {'M', 'T', 'r', 'k', 0, 0, 0, 0};
	unsigned long i;

	eof_log("eof_dump_midi_track() entered", 1);

	if((inputfile == NULL) || (outf == NULL))
	{
		return 0;	//Return failure
	}
	track_length = (unsigned long)file_size_ex(inputfile);
	inf = pack_fopen(inputfile, "r");	//Open input file for reading
	if(!inf)
	{
		return 0;	//Return failure
	}

	(void) pack_fwrite(trackheader, 4, outf);	//Write the output track header
	(void) pack_mputl(track_length, outf);		//Write the output track length
	for(i = 0; i < track_length; i++)
	{	//For each byte in the input file
		(void) pack_putc(pack_getc(inf), outf);	//Copy the byte to the output file
	}
	(void) pack_fclose(inf);

	return 1;	//Return success
}

void eof_write_text_event(unsigned long deltas, const char *str, PACKFILE *fp)
{
	unsigned long length;
	if(str && fp)
	{
		length = ustrsize(str);
		if(length < 128)
		{	//EOF's implementation is a bit lax and only accounts for 1 byte variable length values (7 bits)
			WriteVarLen(deltas, fp);
			(void) pack_putc(0xFF, fp);	//Text event
			(void) pack_putc(0x01, fp);
			(void) pack_putc(length, fp);
			(void) pack_fwrite(str, length, fp);
		}
	}
}

void eof_write_lyric_event(unsigned long deltas, const char *str, PACKFILE *fp)
{
	unsigned long length;
	if(str && fp)
	{
		length = ustrsize(str);
		if(length < 128)
		{	//EOF's implementation is a bit lax and only accounts for 1 byte variable length values (7 bits)
			WriteVarLen(deltas, fp);
			(void) pack_putc(0xFF, fp);	//Lyric event
			(void) pack_putc(0x05, fp);
			(void) pack_putc(length, fp);
			(void) pack_fwrite(str, length, fp);
		}
	}
}

void eof_write_tempo_track(char *trackname, struct Tempo_change *anchorlist, EOF_MIDI_TS_LIST *tslist, EOF_MIDI_KS_LIST *kslist, PACKFILE *outf)
{
	unsigned long lastdelta = 0, current_ts = 0, current_ks = 0, nexteventpos = 0, ppqn, i;
	char whattowrite;
	struct Tempo_change *ptr;	//Conductor for the anchor linked list

	if(!anchorlist || !tslist || !kslist || !outf)
		return;	//Invalid parameters

	if(trackname)
	{	//If a name for the MIDI track was given
		/* write the track name */
		WriteVarLen(0, outf);
		(void) pack_putc(0xFF, outf);
		(void) pack_putc(0x03, outf);
		WriteVarLen(ustrsize(trackname), outf);
		(void) pack_fwrite(trackname, ustrsize(trackname), outf);
	}

	ptr = anchorlist;
	while((ptr != NULL) || (eof_use_ts && (current_ts < tslist->changes)) || (current_ks < kslist->changes))
	{	//While there are tempo changes, TS changes (if the user specified to write TS changes) or KS changes to write
		whattowrite = 0;
		if(ptr != NULL)
		{	//If there are any tempo changes left
			nexteventpos = ptr->delta;
			whattowrite = 1;
		}
		if(eof_use_ts && (current_ts < tslist->changes))
		{	//If there are any TS changes left (only if the user opted to do export TS changes)
			if(!whattowrite || (nexteventpos > tslist->change[current_ts]->pos))
			{	//If no tempo changes were left, or this TS change is earlier than any such changes
				nexteventpos = tslist->change[current_ts]->pos;
				whattowrite = 2;	//Write the TS change first
			}
		}
		if(current_ks < kslist->changes)
		{
			if(!whattowrite || (nexteventpos > kslist->change[current_ks]->pos))
			{	//If no tempo/TS changes were left, or this KS change is earlier than any such changes
				nexteventpos = kslist->change[current_ks]->pos;
				whattowrite = 3;	//Write the KS change first
			}
		}

		if((whattowrite == 1) && ptr)
		{	//If writing a tempo change (again, checking to ensure ptr is not NULL to satisfy cppcheck's warnings)
			if(!eof_midi_enddelta || (ptr->delta <= eof_midi_enddelta))
			{	//Only process this if it occurs at or before the end event
				WriteVarLen(ptr->delta - lastdelta, outf);	//Write this anchor's relative delta time
				lastdelta=ptr->delta;						//Store this anchor's absolute delta time

				ppqn = (60000000.0 / ptr->BPM) + 0.5;		//Convert BPM to ppqn, rounding up
				(void) pack_putc(0xFF, outf);						//Write Meta Event 0x51 (Set Tempo)
				(void) pack_putc(0x51, outf);
				(void) pack_putc(0x03, outf);						//Write event length of 3
				(void) pack_putc((ppqn & 0xFF0000) >> 16, outf);	//Write high order byte of ppqn
				(void) pack_putc((ppqn & 0xFF00) >> 8, outf);		//Write middle byte of ppqn
				(void) pack_putc((ppqn & 0xFF), outf);				//Write low order byte of ppqn
			}
			ptr=ptr->next;							//Iterate to next anchor
		}
		else if(eof_use_ts && (whattowrite == 2))
		{	//If writing a TS change
			if(!eof_midi_enddelta || (tslist->change[current_ts]->pos <= eof_midi_enddelta))
			{	//Only process this if it occurs at or before the end event
				WriteVarLen(tslist->change[current_ts]->pos - lastdelta, outf);	//Write this time signature's relative delta time
				lastdelta=tslist->change[current_ts]->pos;						//Store this time signature's absolute delta time

				for(i = 0; i <= 8; i++)
				{	//Convert the denominator into the power of two required to write into the MIDI event
					if(tslist->change[current_ts]->den >> i == 1)	//if 2 to the power of i is the denominator
						break;
				}
				if(tslist->change[current_ts]->den >> i != 1)
				{	//If the loop ended before the appropriate value was found
					i = 2;	//An unsupported time signature was somehow set, change the denominator to 4
				}

				(void) pack_putc(0xFF, outf);							//Write Meta Event 0x58 (Time Signature)
				(void) pack_putc(0x58, outf);
				(void) pack_putc(0x04, outf);							//Write event length of 4
				(void) pack_putc(tslist->change[current_ts]->num, outf);	//Write the numerator
				(void) pack_putc(i, outf);								//Write the denominator
				(void) pack_putc(24, outf);								//Write the metronome interval (not used by EOF)
				(void) pack_putc(8, outf);								//Write the number of 32nd notes per 24 ticks (not used by EOF)
			}
			current_ts++;										//Iterate to next TS change
		}
		else if(whattowrite == 3)
		{	//If writing a KS change
			if(!eof_midi_enddelta || (kslist->change[current_ks]->pos <= eof_midi_enddelta))
			{	//Only process this if it occurs at or before the end event
				WriteVarLen(kslist->change[current_ks]->pos - lastdelta, outf);	//Write this key signature's relative delta time
				lastdelta=kslist->change[current_ks]->pos;						//Store this key signature's absolute delta time

				(void) pack_putc(0xFF, outf);								//Write Meta Event 0x59 (key signature)
				(void) pack_putc(0x59, outf);
				(void) pack_putc(0x02, outf);								//Write event length of 2
				(void) pack_putc(kslist->change[current_ks]->key, outf);	//Write the key (value from -7 to 7)
				(void) pack_putc(0, outf);								//For now, only major keys are handled (1 would pertain to minor keys)
			}
			current_ks++;										//Iterate to next KS change
		}
	}//While there are tempo changes, TS changes (if the user specified to write TS changes) or KS changes to write
	WriteVarLen(0, outf);		//Write delta time
	(void) pack_putc(0xFF, outf);	//Write Meta Event 0x2F (End Track)
	(void) pack_putc(0x2F, outf);
	(void) pack_putc(0x00, outf);	//Write padding
}

void eof_MIDI_data_track_export(EOF_SONG *sp, PACKFILE *outf, struct Tempo_change *anchorlist, EOF_MIDI_TS_LIST *tslist, unsigned long timedivision)
{
	struct eof_MIDI_data_track *trackptr;
	struct eof_MIDI_data_event *eventptr;
	char trackheader[4] = {'M', 'T', 'r', 'k'};
	unsigned char endoftrack[3] = {0xFF, 0x2F, 0};
	char tempfname[30] = {0};
	PACKFILE *tempf;
	unsigned long lastdelta, deltapos, track_length, ctr;
	char has_stored_tempo;		//Will be set to nonzero if the project contains a stored tempo track, which will affect timing conversion

	if(!sp || !outf || !anchorlist || !sp->midi_data_head)
		return;

	eof_log("eof_MIDI_data_track_export() entered", 1);

	if(eof_validate_temp_folder())
	{	//Ensure the correct working directory and presence of the temporary folder
		eof_log("\tCould not validate working directory and temp folder", 1);
		return;
	}

	has_stored_tempo = eof_song_has_stored_tempo_track(sp) ? 1 : 0;	//Store this status

	for(trackptr = sp->midi_data_head; trackptr != NULL; trackptr = trackptr->next)
	{	//For each raw MIDI track
		if(trackptr->timedivision)
			continue;	//If this track is not a tempo track (which will be written in eof_export_midi() ), skip it

		//Otherwise write the track's MIDI data to a temporary file so its size can be obtained easily
		(void) snprintf(tempfname, sizeof(tempfname) - 1, "%smididatatemp.tmp", eof_temp_path_s);
		tempf = pack_fopen(tempfname, "w");	//Open temporary file for writing
		if(!tempf)
			return;
		lastdelta = 0;
		for(eventptr = trackptr->events; eventptr != NULL; eventptr = eventptr->next)
		{	//For each event in the track
			deltapos = eof_ConvertToDeltaTime(eventptr->realtime + anchorlist->realtime, anchorlist, tslist, timedivision, 0, has_stored_tempo);	//Store the tick position of the event (accounting for the MIDI delay of the active beat map)
			WriteVarLen(deltapos - lastdelta, tempf);		//Write this event's relative delta time
			if((eventptr->size == 2) && (((unsigned char *)eventptr->data)[0] == 0xFF) && (((unsigned char *)eventptr->data)[1] == 0x2F))
			{	//If this is an end of track event that is missing the length field
				(void) pack_fwrite(endoftrack, 3, tempf);	//Write a complete end of track event
			}
			else
			{	//Otherwise write the event data as-is
				(void) pack_fwrite(eventptr->data, eventptr->size, tempf);	//Write this event's data
			}
			lastdelta = deltapos;
		}
		(void) pack_fclose(tempf);					//Close the temporary file

//Get the track's size and write it to the output MIDI file
		track_length = (unsigned long)file_size_ex(tempfname);	//Get the temporary file's size
		tempf = pack_fopen(tempfname, "r");			//Open temporary file for reading
		if(!tempf)
			return;
		(void) pack_fwrite(trackheader, 4, outf);	//Write the output track header
		(void) pack_mputl(track_length, outf);		//Write the output track length
		for(ctr = 0; ctr < track_length; ctr++)
		{	//For each byte of the temporary file
			(void) pack_putc(pack_getc(tempf), outf);	//Copy the byte to the output MIDI file
		}
		(void) pack_fclose(tempf);	//Close the temporary file
		(void) delete_file(tempfname);
	}
}

void eof_check_vocals(EOF_SONG* sp, char *fixvoxpitches, char *fixvoxphrases)
{
	unsigned long j, ctr, tracknum;
	char pitchesprompt = 0, phrasesprompt = 0;

	if(!sp || !fixvoxpitches || !fixvoxphrases)
		return;

	*fixvoxpitches = *fixvoxphrases = 0;	//By default, don't make changes
	for(j = 1; j < sp->tracks; j++)
	{	//For each track
		if(sp->track[j]->track_format != EOF_VOCAL_TRACK_FORMAT)
			continue;	//If this isn't a vocal track, skip it

		//Pre-parse the lyrics to determine if any pitchless lyrics are present
		tracknum = sp->track[j]->tracknum;
		for(ctr = 0; !pitchesprompt && (ctr < sp->vocal_track[tracknum]->lyrics); ctr++)
		{	//Check each lyric (for pitches, but only if the user hasn't been prompted yet for this check)
			if(sp->vocal_track[tracknum]->lyric[ctr]->note != 0)
				continue;	//If this lyric isn't missing its pitch, skip it

			//Otherwise prompt for whether lyrics should be corrected
			eof_cursor_visible = 0;
			eof_pen_visible = 0;
			eof_show_mouse(screen);
			eof_clear_input();
			if(alert(NULL, "Write pitchless lyrics as playable freestyle?", NULL, "&Yes", "&No", 'y', 'n') == 1)
			{	//If user opts to have the lyrics corrected, update the fixvoxpitches variable
				*fixvoxpitches = 1;
			}
			pitchesprompt = 1;
			eof_show_mouse(NULL);
			eof_cursor_visible = 1;
			eof_pen_visible = 1;
			break;
		}

		//Pre-parse the lyrics to see if any exist outside of lyric phrases
		for(ctr = 0; !phrasesprompt && (ctr < sp->vocal_track[tracknum]->lyrics); ctr++)
		{	//Check each lyric (for phrase containment, but only if the user hasn't been prompted yet for this check)
			if(eof_find_lyric_line(ctr) != NULL)
				continue;	//If this lyric is in a line or is a vocal percussion note, skip it

			eof_cursor_visible = 0;
			eof_pen_visible = 0;
			eof_show_mouse(screen);
			eof_clear_input();
			if(alert(NULL, "Add MIDI phrases for lyrics/percussions not in lyric phrases?", NULL, "&Yes", "&No", 'y', 'n') == 1)
			{	//If user opts to have missing lyric phrases inserted
				*fixvoxphrases = 1;
			}
			phrasesprompt = 1;
			eof_show_mouse(NULL);
			eof_cursor_visible = 1;
			eof_pen_visible = 1;
			break;
		}
	}
}

int eof_build_tempo_and_ts_lists(EOF_SONG *sp, struct Tempo_change **anchorlistptr, EOF_MIDI_TS_LIST **tslistptr, unsigned long *timedivision)
{
	struct eof_MIDI_data_track *trackptr;
	struct Tempo_change *anchorlist = NULL, *temp = NULL;
	struct eof_MIDI_data_event *eventptr;
	EOF_MIDI_TS_LIST *tslist;
	unsigned long eventindex, num, den, realden, bytes_used, ctr;
	unsigned long lastppqn=0;	//Tracks the last anchor's PPQN value
	unsigned char eventtype, lasteventtype = 0, meventtype;
	char tsstored = 0;
	unsigned char *dataptr;

	eof_log("eof_build_tempo_and_ts_lists() entered", 1);

	if(!sp || !anchorlistptr || !tslistptr || !timedivision)
		return 0;	//Return error
	tslist = eof_create_ts_list();
	if(tslist == NULL)
		return 0;

	trackptr = eof_song_has_stored_tempo_track(sp);	//Obtain a pointer to the stored tempo map if there is one
	if(!trackptr)
	{	//If building the tempo and TS lists from the active chart's native tempo map
		eof_log("\tBuilding tempo and TS lists from project's tempo track", 1);
		*timedivision = EOF_DEFAULT_TIME_DIVISION;
		anchorlist=eof_build_tempo_list(sp);	//Create a linked list of all tempo changes in eof_song->beat[]
		if(anchorlist == NULL)	//If the anchor list could not be created
		{
			eof_log("\tError saving:  Cannot build anchor list", 1);
			free(tslist);					//Drop the previously created TS list
			return 0;	//Return failure
		}
		free(tslist);					//Drop the previously created TS list
		if(eof_use_ts)
		{	//If the user opted to use the time signatures during export
			tslist = eof_build_ts_list(sp);	//And recreate it with a list of all TS changes in eof_song->beat[]
			if(tslist == NULL)
			{
				eof_log("\tError saving:  Cannot build TS list", 1);
				eof_destroy_tempo_list(anchorlist);
				return 0;	//Return failure
			}
		}
		else
		{	//Otherwise build a TS list containing just the default 4/4 time signature
			tslist = eof_create_ts_list();
			eof_midi_add_ts_realtime(tslist, sp->beat[0]->fpos, 4, 4, 0);	//use an implied TS of 4/4 on the first beat marker
		}
	}
	else
	{	//If building the tempo and TS lists from the stored tempo map
		eof_log("\tBuilding tempo and TS lists from stored tempo track", 1);
		*timedivision = trackptr->timedivision;
		for(eventptr = trackptr->events; eventptr != NULL; eventptr = eventptr->next)
		{	//For each event in the stored track
			dataptr = eventptr->data;
			eventindex = 0;
			eventtype = dataptr[eventindex];	//Read the MIDI event type
			if(eventtype < 0x80)		//All events have to have bit 7 set, if not, it's running status
			{
				eventtype = lasteventtype;	//As per running status, this event is the same as the last non meta/sysex event
			}
			else
			{
				eventindex++;		//Not a running event, so advance forward in the event data
			}

			if((eventtype & 0xF) != 0xF)
				continue;	//If this isn't a meta event, skip the rest of the processing for this event

			meventtype = dataptr[eventindex];	//Read the meta event type
			eventindex++;
			bytes_used = 0;
			(void) eof_parse_var_len(dataptr, eventindex, &bytes_used);	//Read the meta event length
			eventindex += bytes_used;	//Advance by the size of the variable length value parsed above
			if(meventtype == 0x51)
			{	//Tempo change
				unsigned long ppqn;

				if(!lastppqn && eventptr->deltatime)
				{	//If this is the first tempo change and it isn't at delta time 0
					temp = eof_add_to_tempo_list(0, sp->beat[0]->fpos, 120.0, anchorlist);	//Insert a tempo of 120BPM at delta position 0 (the position of the first beat marker)
					if(!temp)
					{	//Error creating link in tempo list
						eof_destroy_tempo_list(anchorlist);	//Destroy list
						eof_destroy_ts_list(tslist);
						return 0;			//Return failure
					}
					anchorlist = temp;	//Update list pointer
				}
				ppqn = (dataptr[eventindex]<<16) | (dataptr[eventindex+1]<<8) | dataptr[eventindex+2];	//Read the 3 byte big endian value
///				eventindex += 3;	//This updated value for eventindex is not used
				lastppqn = ppqn;	//Remember this value
				if(lastppqn == 0)
				{	//If an invalid value was read
					eof_destroy_tempo_list(anchorlist);	//Destroy list
					eof_destroy_ts_list(tslist);
					return 0;			//Return failure
				}
				temp = eof_add_to_tempo_list(eventptr->deltatime, eventptr->realtime + sp->beat[0]->fpos, 60000000.0/lastppqn, anchorlist);	//Store the tempo change, taking the MIDI delay into account
				if(temp == NULL)
				{	//Test the return value of eof_add_to_tempo_list()
					eof_destroy_tempo_list(anchorlist);	//Destroy list
					eof_destroy_ts_list(tslist);
					return 0;			//Return failure
				}
				anchorlist = temp;	//Update list pointer
			}
			else if(meventtype == 0x58)
			{	//Time signature change
				num = dataptr[eventindex];	//Read the numerator
				den = dataptr[eventindex+1];	//Read the value to which the power of 2 must be raised to define the denominator
///				eventindex += 2;	//This updated value for eventindex is not used
				if(den <= 7)
				{	//For now, don't support a time signature denominator larger than 128
					for(ctr = 0, realden = 1; ctr < den; ctr++)
					{	//Find 2^(d2), the actual denominator of this time signature
						realden = realden << 1;
					}
					if(!tsstored && eventptr->deltatime)
					{	//If this is the first TS change and it isn't at delta time 0
						eof_midi_add_ts_realtime(tslist, sp->beat[0]->fpos, 4, 4, 0);	//Insert a TS of 4/4 at delta position 0 (the position of the first beat marker)
						tslist->change[tslist->changes-1]->pos = 0;
					}
					eof_midi_add_ts_realtime(tslist, eventptr->realtime + sp->beat[0]->fpos, num, realden, 0);	//Store the beat marker's time signature, taking the MIDI delay into account
					tslist->change[tslist->changes-1]->pos = eventptr->deltatime;					//Store the time signature's position in deltas
					tsstored = 1;
				}
			}
		}//For each event in the stored track
	}//If building the tempo and TS lists from the stored tempo map

	*anchorlistptr = anchorlist;	//Return the tempo list through the pointer
	*tslistptr = tslist;		//Return the TS list through the pointer
	return 1;
}

void eof_check_for_note_overlap(void)
{
	unsigned long ctr, ctr2, channelctr;
	int channel;

	eof_log("eof_check_for_note_overlap() entered", 1);
	memset(eof_midi_note_status,0,sizeof(eof_midi_note_status));	//Clear note status array
	for(ctr = 0; ctr < eof_midi_events; ctr++)
	{	//For each cached MIDI event
		if(eof_midi_event[ctr]->filtered)
			continue;	//If this event is already filtered out, skip it

		channel = eof_midi_event[ctr]->channel;	//Keep track of which channel this MIDI event pertains to
		if(eof_midi_event[ctr]->on)
		{	//If this is a note on event
			if(eof_midi_note_status[channel][eof_midi_event[ctr]->note] == 1)
			{	//If this note was already on
				eof_midi_event[ctr]->filtered = 1;	//Filter this event from being written to MIDI
				for(ctr2 = ctr + 1; ctr2 < eof_midi_events; ctr2++)
				{	//Examine the rest of the events to filter the next note off for this note #
					if((eof_midi_event[ctr]->note == eof_midi_event[ctr2]->note) && (eof_midi_event[ctr2]->off) && (!eof_midi_event[ctr2]->filtered))
					{	//If this event is a note off for the target note #, and it isn't already filtered out
						eof_midi_event[ctr2]->filtered = 1;	//Filter this event from being written to MIDI
						break;
					}
				}
			}
			else
			{
				eof_midi_note_status[channel][eof_midi_event[ctr]->note] = 1;	//Track that it is now on
			}
		}
		else if(eof_midi_event[ctr]->off)
		{	//If this is a note off event
			if(eof_midi_note_status[channel][eof_midi_event[ctr]->note] == 0)
			{	//If this note was already off
				eof_midi_event[ctr]->filtered = 1;	//Filter this event from being written to MIDI
			}
			else
			{
				eof_midi_note_status[channel][eof_midi_event[ctr]->note] = 0;	//Track that it is now off
			}
		}
	}

	//Check eof_midi_note_status[] to make sure no notes remain on
	for(channelctr = 0; channelctr < 16; channelctr++)
	{	//For each of the usable MIDI channels
		for(ctr = 0; ctr < 128; ctr++)
		{
			if(eof_midi_note_status[channelctr][ctr] != 0)
			{
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tWarning:  Note %lu is not ended.", ctr);
				eof_log(eof_log_string, 1);
			}
		}
	}
}

void eof_check_for_hopo_phrase_overlap(void)
{
	#define HOPO_ARRAY_SIZE 8
	int HOPO_notes[HOPO_ARRAY_SIZE] = {65, 66, 77, 78, 89, 90, 101, 102};	//A list of each of the HOPO on/off phrase markers
	int HOPO_notes_off[HOPO_ARRAY_SIZE] = {66, 65, 78, 77, 90, 89, 102, 101};	//A list of the opposite markers from each index in HOPO_notes[], ie. the phrase that should end before a corresponding HOPO phrase starts
	unsigned long ctr, ctr2, ctr3;
	char phrasealtered;

	eof_log("eof_check_for_hopo_phrase_overlap() entered", 1);
	for(ctr = 0; ctr < eof_midi_events; ctr++)
	{	//For each cached MIDI event
		if(eof_midi_event[ctr]->filtered)
			continue;	//If this event is already filtered out, skip it
		if(!eof_midi_event[ctr]->off)
			continue;	//If this is not a note off event, skip it

		phrasealtered = 0;
		for(ctr2 = 0; ((size_t)ctr2 < HOPO_ARRAY_SIZE) && !phrasealtered; ctr2++)
		{	//For each of the note numbers in the HOPO marker list (or until the HOPO phrase's end position has been altered)
			if(eof_midi_event[ctr]->note == HOPO_notes[ctr2])
			{	//If the event is a HOPO phrase end marker
				for(ctr3 = ctr; (ctr3 > 0) && (eof_midi_event[ctr3 - 1]->pos == eof_midi_event[ctr]->pos); ctr3--);	//Rewind to the first MIDI event at this delta position

				for(;(ctr3 < eof_midi_events) && (eof_midi_event[ctr3]->pos == eof_midi_event[ctr]->pos); ctr3++)
				{	//For each MIDI event at this position
					if(eof_midi_event[ctr3]->note == HOPO_notes_off[ctr2])
					{	//If this is a marker for the opposite HOPO phrase type
						if(eof_midi_event[ctr]->pos > 0)	//Don't allow an underflow
						{	//Don't allow an underflow
							if(eof_midi_event[ctr]->length > 1)
							{	//If the HOPO on/off marker is more than one tick long
								eof_midi_event[ctr]->pos--;		//Decrement the HOPO marker's off event to be one delta earlier
								eof_midi_event[ctr]->length--;	//Track the new marker length for debugging purposes
								phrasealtered = 1;
							}
						}
						break;
					}
				}
			}
		}
	}//For each cached MIDI event
}

EOF_MIDI_KS_LIST * eof_create_ks_list(void)
{
	unsigned long ctr;
	EOF_MIDI_KS_LIST * lp;

	eof_log("eof_create_ks_list() entered", 1);

	lp = malloc(sizeof(EOF_MIDI_KS_LIST));
	if(!lp)
	{
		return NULL;
	}

	for(ctr=0;ctr<EOF_MAX_KS;ctr++)
	{	//Init all pointers in the array as NULL
		lp->change[ctr]=NULL;
	}
	lp->changes = 0;
	return lp;
}

EOF_MIDI_KS_LIST *eof_build_ks_list(EOF_SONG *sp)
{
	unsigned long ctr;
	EOF_MIDI_KS_LIST * kslist = NULL;

	eof_log("eof_build_ks_list() entered", 1);

	if((sp == NULL) || (sp->beats == 0))
		return NULL;
	kslist = eof_create_ks_list();
	if(kslist == NULL)
		return NULL;

	for(ctr = 0; ctr < sp->beats; ctr++)
	{	//For each beat, create a list of Time Signature changes and store the appropriate delta position of each
		if((sp->beat[ctr]->flags & EOF_BEAT_FLAG_KEY_SIG) && (kslist->changes < EOF_MAX_KS))
		{	//If a key signature exists on this beat, and the maximum number of key signatures hasn't been reached
			kslist->change[kslist->changes] = malloc(sizeof(EOF_MIDI_KS_CHANGE));	//Allocate memory
			if(kslist->change[kslist->changes])
			{	//If the memory was allocated
				kslist->change[kslist->changes]->pos = ctr * EOF_DEFAULT_TIME_DIVISION;	//All beat markers are on multiples of the time division
				kslist->change[kslist->changes]->realtime = sp->beat[ctr]->fpos;
				kslist->change[kslist->changes]->key = sp->beat[ctr]->key;
				kslist->changes++;	//Increment counter
			}
		}
	}

	return kslist;
}

void eof_destroy_ks_list(EOF_MIDI_KS_LIST *ptr)
{
	unsigned long i;

	eof_log("eof_destroy_ks_list() entered", 1);

	if(ptr != NULL)
	{
		for(i = 0; i < ptr->changes; i++)
		{
			free(ptr->change[i]);
		}
		free(ptr);
	}
}

void eof_write_ghwt_drum_array_txt(EOF_SONG *sp, char *fn)
{
	PACKFILE * fp;
	char buffer[20] = {0};
	unsigned long ctr, code, length;
	unsigned char note, accent;
	EOF_LEGACY_TRACK *ptr;

	eof_log("eof_write_ghwt_drum_array_txt() entered", 1);

	if(!sp || !fn)
	{
		return;
	}

	ptr = sp->legacy_track[sp->track[EOF_TRACK_DRUM]->tracknum];
	if(!ptr->notes)
	{	//Empty drum track
		return;
	}

	fp = pack_fopen(fn, "w");
	if(!fp)
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError saving:  Cannot open output GHWT array.txt file:  \"%s\"", strerror(errno));	//Get the Operating System's reason for the failure
		eof_log(eof_log_string, 1);
		return;	//Return failure
	}

	for(ctr = 0; ctr < ptr->notes; ctr++)
	{	//For each drum note
		if(ptr->note[ctr]->type != EOF_NOTE_AMAZING)
			continue;	//If the note isn't in the expert difficulty, skip it

		(void) snprintf(buffer, sizeof(buffer) - 1, "%lu\n", ptr->note[ctr]->pos);	//Build a string with the note's timestamp
		(void) pack_fputs(buffer, fp);

		//Build the code, the two most significant bytes being gems and status flags, the two less significant bytes being the length
		code = 0;
		note = ptr->note[ctr]->note;	//Simplify
		accent = ptr->note[ctr]->accent;

		if(note & 1)	//Bass drum
			code |= 32;
		if(note & 2)
		{	//Lane 2 (red)
			code |= 2;
			if(accent & 2)	//Accented lane 2
				code |= 256;
		}
		if(note & 4)
		{	//Lane 3 (yellow)
			code |= 4;
			if(accent & 4)	//Accented lane 3
				code |= 512;
		}
		if(note & 8)
		{	//Lane 4 (blue)
			code |= 8;
			if(accent & 8)	//Accented lane 4
				code |= 1024;
		}
		if(note & 16)
		{	//Lane 5 (orange)
			code |= 16;
			if(accent & 16)	//Accented lane 5
				code |= 2048;
		}
		if(note & 32)
		{	//Lane 6 (green)
			code |= 1;
			if(accent & 32)	//Accented lane 6
				code |= 128;
		}
		code <<= 16;					//That note data belongs in the upper two bytes of the 4 byte code

		length = ptr->note[ctr]->length;
		if(length > 0xFFFF)
			length = 0xFFFF;	//Ensure the length doesn't exceed what can be written in two bytes
		code += length;			//Add the note length to put it in the lower two bytes of the code

		(void) snprintf(buffer, sizeof(buffer) - 1, "%lu\n", code);	//Write the code to the file
		(void) pack_fputs(buffer, fp);
	}

	(void) pack_fclose(fp);
}

void eof_log_midi_event_list(void)
{
	unsigned long ctr;
	EOF_MIDI_EVENT **ptr;

	if(eof_log_level < 3)	//Only perform this logic for the sake of exhaustive logging or higher
		return;

	for(ctr = 0; ctr < eof_midi_events; ctr++)
	{	//For each event in the list
		ptr = &eof_midi_event[ctr];
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tMIDI event:  Pos = %lu, type = 0x%X, note = %d, vel = %d, ch = %d", (*ptr)->pos, (*ptr)->type, (*ptr)->note, (*ptr)->velocity, (*ptr)->channel);
		eof_log(eof_log_string, 3);
	}
}
