#include <allegro.h>
#include "main.h"
#include "dialog.h"
#include "note.h"
#include "beat.h"
#include "midi.h"
#include "undo.h"
#include "utility.h"
#include "menu/note.h"	//For pitch macros
#include "foflc/Lyric_storage.h"	//For RBA extraction
#include "foflc/Midi_parse.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

#define EOF_MIDI_TIMER_FREQUENCY  40
#define EOF_RBN_COMPATIBILITY		//Alters MIDI export logic to create a MIDI more suitable for use with the "Magma" RBN tool

static EOF_MIDI_EVENT * eof_midi_event[EOF_MAX_MIDI_EVENTS];
static unsigned long eof_midi_events = 0;
static char eof_midi_note_status[128] = {0};	//Tracks the on/off status of notes 0 through 127, maintained by eof_add_midi_event()

void eof_add_midi_event(unsigned long pos, int type, int note, int velocity, int channel)
{	//To avoid rounding issues during timing conversion, this should be called with the MIDI tick position of the event being stored
	eof_log("eof_add_midi_event() entered", 2);	//Only log this if verbose logging is on

	eof_midi_event[eof_midi_events] = malloc(sizeof(EOF_MIDI_EVENT));
	if(eof_midi_event[eof_midi_events])
	{
		eof_midi_event[eof_midi_events]->pos = pos;
		eof_midi_event[eof_midi_events]->type = type;
		eof_midi_event[eof_midi_events]->note = note;
		eof_midi_event[eof_midi_events]->velocity = velocity;
		eof_midi_event[eof_midi_events]->channel = channel;
		eof_midi_events++;

		if((note >= 0) && (note <= 127))
		{	//If the note is in bounds of a legal MIDI note, track its on/off status
			if(type == 0x80)	//Note Off
				eof_midi_note_status[note] = 0;
			else if(type == 0x90)	//Note On
				eof_midi_note_status[note] = 1;
		}
	}
}

void eof_add_midi_lyric_event(unsigned long pos, char * text)
{	//To avoid rounding issues during timing conversion, this should be called with the MIDI tick position of the event being stored
	eof_log("eof_add_midi_lyric_event() entered", 2);	//Only log this if verbose logging is on

	if(text)
	{
		eof_midi_event[eof_midi_events] = malloc(sizeof(EOF_MIDI_EVENT));
		if(eof_midi_event[eof_midi_events])
		{
			eof_midi_event[eof_midi_events]->pos = pos;
			eof_midi_event[eof_midi_events]->type = 0x05;
			eof_midi_event[eof_midi_events]->dp = text;
			eof_midi_events++;
		}
	}
}

void eof_clear_midi_events(void)
{
	eof_log("eof_clear_midi_events() entered", 1);

	unsigned long i;
	for(i = 0; i < eof_midi_events; i++)
	{
		free(eof_midi_event[i]);
	}
	eof_midi_events = 0;
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
		pack_putc(buffer, fp);
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

unsigned long ReadVarLen(PACKFILE * fp)
{
	eof_log("ReadVarLen() entered", 1);

	unsigned long value;
	unsigned char c;

	if(!fp)
	{
		return 0;
	}
	if((value = pack_getc(fp)) & 0x80)
	{
		value &= 0x7F;
		do
		{
			value = (value << 7) + ((c = pack_getc(fp)) & 0x7F);
		} while (c & 0x80);
	}

	return value;
}

/* sorter for MIDI events so I can ouput proper MTrk data */
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
	}

    /* put lyric events first, except for a lyric phrase on marker */
    if(((*thing1)->type == 0x05) && ((*thing2)->type == 0x90))
    {
    	if(((*thing2)->note == 105) || ((*thing2)->note == 106))
			return 1;	//lyric phrases should be written before the lyric event
		else
			return -1;
    }
    if(((*thing1)->type == 0x90) && ((*thing2)->type == 0x05))
    {
    	if(((*thing1)->note == 105) || ((*thing1)->note == 106))
			return -1;	//lyric phrase should be written before the lyric event
		else
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

    // they are equal...
    return 0;
}

/* sorter for MIDI events so I can ouput proper MTrk data */
/* This function isn't currently used anywhere */
int qsort_helper4(const void * e1, const void * e2)
{
	eof_log("qsort_helper4() entered", 1);

    EOF_MIDI_EVENT ** thing1 = (EOF_MIDI_EVENT **)e1;
    EOF_MIDI_EVENT ** thing2 = (EOF_MIDI_EVENT **)e2;

    if((*thing1)->pos < (*thing2)->pos)
	{
        return -1;
    }
    if((*thing1)->pos > (*thing2)->pos)
    {
        return 1;
    }

    // they are equal...
    return 0;
}

long eof_figure_beat(double pos)
{
	eof_log("eof_figure_beat() entered", 1);

	long i;

	for(i = 0; i < eof_song->beats - 1; i++)
	{
		if(i + 1 >= eof_song->beats)
		{				//If i references the last defined beat
			return i;	//return i instead of referencing an undefined beat
		}

		if((eof_song->beat[i]->pos <= pos) && (eof_song->beat[i + 1]->pos > pos))
		{
			return i;
		}
	}
	return -1;
}

double eof_calculate_bpm_absolute(double pos)
{
	eof_log("eof_calculate_bpm_absolute() entered", 1);

	long beat = eof_figure_beat(pos);
	if(beat >= 0)
	{
		return (double)60000000.0 / (double)eof_song->beat[beat]->ppqn;
	}
	return 0.0;
}

int eof_check_bpm_change(unsigned long start, unsigned long end)
{
	eof_log("eof_check_bpm_change() entered", 1);

	long startbeat = eof_figure_beat(start);
	long endbeat = eof_figure_beat(end);
//	unsigned long startbpm = sp->beat[startbeat].ppqn;
	long i;

	/* same beat, no brainer */
	if(startbeat == endbeat)
	{
		return 0;
	}

	/* different starting and ending bpm, uh huh */
	else if(eof_song->beat[startbeat]->ppqn != eof_song->beat[endbeat]->ppqn)
	{
		return 1;
	}

	else
	{
		for(i = startbeat; (i < endbeat) && (i < eof_song->beats); i++)
		{
			if(eof_song->beat[i]->ppqn != eof_song->beat[startbeat]->ppqn)
			{
				return 1;
			}
		}
	}
	return 0;
}

/* takes a segment of time and calculates it's actual delta,
   taking into account the BPM changes */
//The conversion of realtime to deltas is deltas=realtime * timedivision * BPM / (millis per minute)
//The term "BPM / (millis per minute)" can be mathematically simplified to "1000 / ppqn"
//The simplified formula is deltas=realtime * timedivision * 1000 / ppqn
double eof_calculate_delta(double start, double end)
{
	eof_log("eof_calculate_delta() entered", 1);

	long i;
	long startbeat = eof_figure_beat(start);
	long endbeat = eof_figure_beat(end);
	double total_delta = 0.0;	//Delta counter
	double total_time = 0.0;	//Count the segments of time that were converted, for debugging

	/* if no BPM change, calculate delta the easy way :) */
	if(!eof_check_bpm_change(start, end))
	{
		total_time = end - start;
		return (end - start) * EOF_DEFAULT_TIME_DIVISION * 1000 / eof_song->beat[0]->ppqn;
	}

	/* get first_portion */
	total_delta += (eof_song->beat[startbeat + 1]->fpos - start) * EOF_DEFAULT_TIME_DIVISION * 1000 / eof_song->beat[startbeat]->ppqn;
	total_time += eof_song->beat[startbeat + 1]->fpos - start;

	/* get rest of the portions */
	for(i = startbeat + 1; i < endbeat; i++)
	{
		total_delta += (eof_song->beat[i + 1]->fpos - eof_song->beat[i]->fpos) * EOF_DEFAULT_TIME_DIVISION * 1000 / eof_song->beat[i]->ppqn;
		total_time += eof_song->beat[i + 1]->fpos - eof_song->beat[i]->fpos;
	}

	/* get last portion */
	total_delta += (end - eof_song->beat[endbeat]->fpos) * EOF_DEFAULT_TIME_DIVISION * 1000 / eof_song->beat[endbeat]->ppqn;
	total_time += end - eof_song->beat[endbeat]->fpos;

	return total_delta;
}

unsigned long eof_count_tracks(void)
{
	eof_log("eof_count_tracks() entered", 1);

	unsigned long i;
	unsigned long count = 0;

	for(i = 1; i < eof_song->tracks; i++)
	{
		if(eof_get_track_size(eof_song, i))
		{
			count++;
		}
	}

	return count;
}

/* write MTrk data to a temp file so we can calculate the length in bytes of the track
   write MThd data and copy MTrk data from the temp file using the size of the temp file as the track length
   delete the temp file
   voila, correctly formatted MIDI file */
int eof_export_midi(EOF_SONG * sp, char * fn)
{
	eof_log("eof_export_midi() entered", 1);

	char header[14] = {'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 1, 0, 1, (EOF_DEFAULT_TIME_DIVISION >> 8), (EOF_DEFAULT_TIME_DIVISION & 0xFF)}; //The last two bytes are the time division
	char notetempname[EOF_TRACKS_MAX+1][15];
	char notetrackspopulated[EOF_TRACKS_MAX+1] = {0};
	char expertplustempname[] = {"expert+.tmp"};	//Stores the temporary filename for the Expert+ track data
	char tempotempname[] = {"tempo.tmp"};
	char eventtempname[] = {"event.tmp"};
	char beattempname[] = {"beat.tmp"};
	char expertplusfilename[1024] = {0};
	char expertplusshortname[] = {"expert+.mid"};
	PACKFILE * fp;
	PACKFILE * fp3 = NULL;					//File pointer for the Expert+ file
	unsigned long i, j, k;
	unsigned long ctr;
	unsigned long delta = 0;
	int midi_note_offset = 0;
	int vel;
	unsigned long tracknum=0;				//Used to de-obfuscate the track number

	unsigned long ppqn=0;					//Used to store conversion of BPM to ppqn
	struct Tempo_change *anchorlist=NULL;	//Linked list containing tempo changes
	struct Tempo_change *ptr=NULL;			//Conductor for the anchor linked list
	unsigned long lastdelta=0;				//Keeps track of the last anchor's absolute delta time
	char * tempstring = NULL;				//Used to store a copy of the lyric string into eof_midi_event[], so the string can be modified from the original
	char correctlyrics = 0;					//If nonzero, logic will be performed to correct the pitchless lyrics to have a pound character and have a generic pitch note
	char correctphrases = 0;				//If nonzero, logic will be performed to add missing lyric phrases to ensure all lyrics (except vocal percussion notes) are encompassed within a lyric phrase
	long length, deltalength;				//Used to cap drum notes
	char prodrums = 0;						//Tracks whether the drum track being written includes Pro drum notation
	char expertplus = 0;					//Tracks whether an expert+.mid track should be created to hold the Expert+ drum track
	char expertpluswritten = 0;				//Tracks whether an expert+.mid track has been written
	char eventstrackwritten = 0;			//Tracks whether an events track has been written
	char beattrackwritten = 0;				//Tracks whether a beat track has been written
	char trackctr;							//Used in the temp data creation to handle Expert+
	EOF_MIDI_TS_LIST *tslist=NULL;			//List containing TS changes
	unsigned char rootvel;					//Used to write root notes for pro guitar tracks
	unsigned long note, noteflags, notepos, deltapos;
	char type;
	int channel, velocity, bitmask, slidenote = 0, scale, chord;	//Used for pro guitar export
	EOF_PHRASE_SECTION *sectionptr;
	char *lastname = NULL, *currentname = NULL, nochord[]="NC", chordname[100]="";
	char match = 0;

	if(!sp | !fn)
	{
		return 0;	//Return failure
	}
	anchorlist=eof_build_tempo_list(sp);	//Create a linked list of all tempo changes in eof_song->beat[]
	if(anchorlist == NULL)	//If the anchor list could not be created
	{
		return 0;	//Return failure
	}

	//Initialize the temporary filename array
	for(i = 0; i < EOF_TRACKS_MAX+1; i++)
	{
		snprintf(notetempname[i],15,"eof%lu.tmp",i);
	}

	if(eof_use_ts)
	{	//If the user opted to use the time signatures during export
		tslist=eof_build_ts_list(anchorlist);	//Create a list of all TS changes in eof_song->beat[]
		if(tslist == NULL)
		{
			eof_destroy_tempo_list(anchorlist);
			return 0;	//Return failure
		}
	}
	else
	{	//Otherwise build a TS list containing just the default 4/4 time signature
		tslist = eof_create_ts_list();
		eof_midi_add_ts_realtime(tslist, eof_song->beat[0]->fpos, 4, 4, 0);	//use an implied TS of 4/4 on the first beat marker
	}

	eof_sort_notes(sp);	//Writing efficient on-the-fly HOPO phrasing relies on all notes being sorted

	//Write tracks
	for(j = 1; j < sp->tracks; j++)
	{	//For each track in the project
		if(eof_get_track_size(sp, j) == 0)	//If this track has no notes
			continue;	//Skip the track

		if(j == EOF_TRACK_DANCE)
		{	//Phase Shift's dance track specification is for dance notes to use a velocity of 127
			vel=127;
		}
		else
		{	//For other tracks, the generic velocity is 100
			vel=100;
		}

		notetrackspopulated[j] = 1;	//Remember that this track is populated
		eof_clear_midi_events();
		tracknum = sp->track[j]->tracknum;
		memset(eof_midi_note_status,0,sizeof(eof_midi_note_status));	//Clear note status array
		if(sp->track[j]->track_format == EOF_LEGACY_TRACK_FORMAT)
		{	//If this is a legacy track
			/* fill in notes */
//Detect whether Pro drum notation is being used
//Pro drum notation is that if a green, yellow or blue drum note is NOT to be marked as a cymbal,
//it must be marked with the appropriate MIDI note, otherwise the note defaults as a cymbal
			prodrums = 0;
			if(sp->track[j]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
			{	//If this is a drum track
				for(i = 0, prodrums = 0; i < eof_get_track_size(sp, j); i++)
				{	//For each note in the track
					note = eof_get_note_note(sp, j, i);
					noteflags = eof_get_note_flags(sp, j, i);
					if(	((note & 4) && ((noteflags & EOF_NOTE_FLAG_Y_CYMBAL))) ||
						((note & 8) && ((noteflags & EOF_NOTE_FLAG_B_CYMBAL))) ||
						((note & 16) && ((noteflags & EOF_NOTE_FLAG_G_CYMBAL))))
					{	//If this note contains a yellow, blue or purple (green in Rock Band) drum marked with pro drum notation
						prodrums = 1;
						break;
					}
				}
			}

			/* write the MTrk MIDI data to a temp file
			use size of the file as the MTrk header length */
			for(i = 0; i < eof_get_track_size(sp, j); i++)
			{	//For each note in the track
				if(j != EOF_TRACK_DANCE)
				{	//All legacy style tracks besides the dance track use the same offsets
					switch(eof_get_note_type(sp, j, i))
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
					}
				}
				else
				{
					switch(eof_get_note_type(sp, j, i))
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
					}
				}

				noteflags = eof_get_note_flags(sp, j, i);	//Store the note flags for easier use
				note = eof_get_note_note(sp, j, i);			//Store the note bitflag for easier use
				notepos = eof_get_note_pos(sp, j, i);		//Store the note position for easier use
				length = eof_get_note_length(sp, j, i);		//Store the note length for easier use
				type = eof_get_note_type(sp, j, i);			//Store the note type for easier use

				deltapos = eof_ConvertToDeltaTime(notepos,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION);	//Store the tick position of the note
				deltalength = eof_ConvertToDeltaTime(notepos + length,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION) - deltapos;	//Store the number of delta ticks representing the note's length
				if(deltalength < 1)
				{	//If some kind of rounding error or other issue caused the delta length to be less than 1, force it to the minimum length of 1
					deltalength = 1;
				}
				if((j == EOF_TRACK_DRUM) && (type != EOF_NOTE_SPECIAL))
				{	//Ensure that drum notes are not written with sustain (Unless they are BRE notes)
					deltalength = 1;
				}

				if(eof_open_bass_enabled() && (j == EOF_TRACK_BASS))
				{	//Ensure that for PART BASS, green gems and open bass notes don't exist at the same location
					if((note & 1) && (note & 32))
					{	//If this bass guitar note has lane 1 and 6 gems
						note &= ~(1);	//Clear lane 1
					}
				}

				/* write green note */
				if(note & 1)
				{
					if((j == EOF_TRACK_DRUM) && (noteflags & EOF_NOTE_FLAG_DBASS))
					{	//If the track being written is PART DRUMS, and this note is marked for Expert+ double bass
						eof_add_midi_event(deltapos, 0x90, 95, vel, 0);
						eof_add_midi_event(deltapos + deltalength, 0x80, 95, vel, 0);
						expertplus = 1;
					}
					else	//Otherwise write a normal green gem
					{
						eof_add_midi_event(deltapos, 0x90, midi_note_offset + 0, vel, 0);
						eof_add_midi_event(deltapos + deltalength, 0x80, midi_note_offset + 0, vel, 0);
					}
				}

				/* write red note */
				if(note & 2)
				{
					eof_add_midi_event(deltapos, 0x90, midi_note_offset + 1, vel, 0);
					eof_add_midi_event(deltapos + deltalength, 0x80, midi_note_offset + 1, vel, 0);
				}

				/* write yellow note */
				if(note & 4)
				{
					if((j == EOF_TRACK_DRUM) && prodrums && !eof_check_flags_at_legacy_note_pos(sp->legacy_track[tracknum],i,EOF_NOTE_FLAG_Y_CYMBAL))
					{	//If pro drum notation is in effect and no more yellow drum notes at this note's position are marked as cymbals
						if(type != EOF_NOTE_SPECIAL)
						{	//Write a pro yellow drum marker only if this isn't a BRE note
							eof_add_midi_event(deltapos, 0x90, RB3_DRUM_YELLOW_FORCE, vel, 0);
							eof_add_midi_event(deltapos + deltalength, 0x80, RB3_DRUM_YELLOW_FORCE, vel, 0);
						}
					}
					eof_add_midi_event(deltapos, 0x90, midi_note_offset + 2, vel, 0);
					eof_add_midi_event(deltapos + deltalength, 0x80, midi_note_offset + 2, vel, 0);
				}

				/* write blue note */
				if(note & 8)
				{
					if((j == EOF_TRACK_DRUM) && prodrums && !eof_check_flags_at_legacy_note_pos(sp->legacy_track[tracknum],i,EOF_NOTE_FLAG_B_CYMBAL))
					{	//If pro drum notation is in effect and no more blue drum notes at this note's position are marked as cymbals
						if(type != EOF_NOTE_SPECIAL)
						{	//Write a pro blue drum marker only if this isn't a BRE note
							eof_add_midi_event(deltapos, 0x90, RB3_DRUM_BLUE_FORCE, vel, 0);
							eof_add_midi_event(deltapos + deltalength, 0x80, RB3_DRUM_BLUE_FORCE, vel, 0);
						}
					}
					eof_add_midi_event(deltapos, 0x90, midi_note_offset + 3, vel, 0);
					eof_add_midi_event(deltapos + deltalength, 0x80, midi_note_offset + 3, vel, 0);
				}

				/* write purple note */
				if(note & 16)
				{	//Note: EOF/FoF refer to this note color as purple/orange whereas Rock Band displays it as green
					if((j == EOF_TRACK_DRUM) && prodrums && !eof_check_flags_at_legacy_note_pos(sp->legacy_track[tracknum],i,EOF_NOTE_FLAG_G_CYMBAL))
					{	//If pro drum notation is in effect and no more green drum notes at this note's position are marked as cymbals
						if(type != EOF_NOTE_SPECIAL)
						{	//Write a pro green drum marker if one isn't already in effect, but only if this isn't a BRE note
							eof_add_midi_event(deltapos, 0x90, RB3_DRUM_GREEN_FORCE, vel, 0);
							eof_add_midi_event(deltapos + deltalength, 0x80, RB3_DRUM_GREEN_FORCE, vel, 0);
						}
					}
					eof_add_midi_event(deltapos, 0x90, midi_note_offset + 4, vel, 0);
					eof_add_midi_event(deltapos + deltalength, 0x80, midi_note_offset + 4, vel, 0);
				}

				/* write open bass note, if the feature was enabled during save */
				if(eof_open_bass_enabled() && (j == EOF_TRACK_BASS) && (note & 32))
				{	//If this is an open bass note
					noteflags |= EOF_NOTE_FLAG_F_HOPO;	//Set the forced HOPO on flag, which is used to denote open bass
					eof_add_midi_event(deltapos, 0x90, midi_note_offset + 0, vel, 0);	//Write a gem for lane 1
					eof_add_midi_event(deltapos + deltalength, 0x80, midi_note_offset + 0, vel, 0);
				}

				/* write forced HOPO */
				if(noteflags & EOF_NOTE_FLAG_F_HOPO)
				{	//thekiwimaddog indicated that Rock Band uses HOPO phrases per note/chord
					eof_add_midi_event(deltapos, 0x90, midi_note_offset + 5, vel, 0);
					eof_add_midi_event(deltapos + deltalength, 0x80, midi_note_offset + 5, vel, 0);
				}

				/* write forced non-HOPO */
				else if(noteflags & EOF_NOTE_FLAG_NO_HOPO)
				{	//thekiwimaddog indicated that Rock Band uses HOPO phrases per note/chord
					eof_add_midi_event(deltapos, 0x90, midi_note_offset + 6, vel, 0);
					eof_add_midi_event(deltapos + deltalength, 0x80, midi_note_offset + 6, vel, 0);
				}
			}//For each note in the track

			/* fill in star power */
			for(i = 0; i < eof_get_num_star_power_paths(sp, j); i++)
			{	//For each star power path in the track
				sectionptr = eof_get_star_power_path(sp, j, i);
				deltapos = eof_ConvertToDeltaTime(sectionptr->start_pos,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION);	//Store the tick position of the phrase
				deltalength = eof_ConvertToDeltaTime(sectionptr->end_pos,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION) - deltapos;	//Store the number of delta ticks representing the phrase's length
				if(deltalength < 1)
				{	//If some kind of rounding error or other issue caused the delta length to be less than 1, force it to the minimum length of 1
					deltalength = 1;
				}
				eof_add_midi_event(deltapos, 0x90, 116, vel, 0);
				eof_add_midi_event(deltapos + deltalength, 0x80, 116, vel, 0);
			}

			/* fill in solos */
			for(i = 0; i < eof_get_num_solos(sp, j); i++)
			{	//For each solo in the track
				sectionptr = eof_get_solo(sp, j, i);
				deltapos = eof_ConvertToDeltaTime(sectionptr->start_pos,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION);	//Store the tick position of the phrase
				deltalength = eof_ConvertToDeltaTime(sectionptr->end_pos,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION) - deltapos;	//Store the number of delta ticks representing the phrase's length
				if(deltalength < 1)
				{	//If some kind of rounding error or other issue caused the delta length to be less than 1, force it to the minimum length of 1
					deltalength = 1;
				}
				eof_add_midi_event(deltapos, 0x90, 103, vel, 0);
				eof_add_midi_event(deltapos + deltalength, 0x80, 103, vel, 0);
			}

			/* fill in tremolos */
			for(i = 0; i < eof_get_num_tremolos(sp, j); i++)
			{	//For each tremolo in the track
				sectionptr = eof_get_tremolo(sp, j, i);
				deltapos = eof_ConvertToDeltaTime(sectionptr->start_pos,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION);	//Store the tick position of the phrase
				deltalength = eof_ConvertToDeltaTime(sectionptr->end_pos,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION) - deltapos;	//Store the number of delta ticks representing the phrase's length
				if(deltalength < 1)
				{	//If some kind of rounding error or other issue caused the delta length to be less than 1, force it to the minimum length of 1
					deltalength = 1;
				}
				eof_add_midi_event(deltapos, 0x90, 126, vel, 0);	//Note 126 denotes a tremolo marker
				eof_add_midi_event(deltapos + deltalength, 0x80, 126, vel, 0);
			}

			/* fill in trills */
			for(i = 0; i < eof_get_num_trills(sp, j); i++)
			{	//For each trill in the track
				sectionptr = eof_get_trill(sp, j, i);
				deltapos = eof_ConvertToDeltaTime(sectionptr->start_pos,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION);	//Store the tick position of the phrase
				deltalength = eof_ConvertToDeltaTime(sectionptr->end_pos,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION) - deltapos;	//Store the number of delta ticks representing the phrase's length
				if(deltalength < 1)
				{	//If some kind of rounding error or other issue caused the delta length to be less than 1, force it to the minimum length of 1
					deltalength = 1;
				}
				eof_add_midi_event(deltapos, 0x90, 127, vel, 0);	//Note 127 denotes a trill marker
				eof_add_midi_event(deltapos + deltalength, 0x80, 127, vel, 0);
			}

			for(i=0;i < 128;i++)
			{	//Ensure that any notes that are still on are terminated
				if(eof_midi_note_status[i] != 0)	//If this note was left on, send an alert message, as this is abnormal
				{
					allegro_message("MIDI export error:  Note %lu was not turned off", i);
//					notenum = eof_get_track_size(sp, j) - 1;	//The index of the last note in this track
//					eof_add_midi_event(eof_get_note_pos(sp, j, notenum) + eof_get_note_length(sp, j, notenum),0x80,i, vel, 0);
				}
			}
			qsort(eof_midi_event, eof_midi_events, sizeof(EOF_MIDI_EVENT *), qsort_helper3);
//			allegro_message("break1");

			for(trackctr=0;trackctr<=expertplus;trackctr++)
			{	//This loop makes a second pass to write the expert+ drum MIDI if applicable
				/* open the file */
				if(trackctr == 0)	//Writing the normal temp file
					fp = pack_fopen(notetempname[j], "w");
				else
				{			//Write the Expert+ temp file
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
					return 0;
				}

				/* write the track name */
				WriteVarLen(0, fp);
				pack_putc(0xFF, fp);
				pack_putc(0x03, fp);
				WriteVarLen(ustrlen(sp->track[j]->name), fp);
				pack_fwrite(sp->track[j]->name, ustrlen(sp->track[j]->name), fp);

				#ifdef EOF_RBN_COMPATIBILITY
				//RBN requires that drum mix events have been defined
				if(j == EOF_TRACK_DRUM)
				{
					eof_write_text_event(0, "[mix 0 drums0]", fp);
					eof_write_text_event(0, "[mix 1 drums0]", fp);
					eof_write_text_event(0, "[mix 2 drums0]", fp);
					eof_write_text_event(0, "[mix 3 drums0]", fp);
				}
				#endif

				/* add MIDI events */
				lastdelta = 0;
				for(i = 0; i < eof_midi_events; i++)
				{
					if((trackctr == 1) && (eof_midi_event[i]->note < 96))
						continue;	//Filter out all non Expert drum notes for the Expert+ track

					delta = eof_midi_event[i]->pos;
					WriteVarLen(delta-lastdelta, fp);	//Write this event's relative delta time
					lastdelta = delta;					//Store this event's absolute delta time
					pack_putc(eof_midi_event[i]->type, fp);
					pack_putc(eof_midi_event[i]->note, fp);
					pack_putc(vel, fp);
				}

				/* end of track */
				WriteVarLen(0, fp);
				pack_putc(0xFF, fp);
				pack_putc(0x2F, fp);
				pack_putc(0x00, fp);
				pack_fclose(fp);

				if(trackctr == 1)
				{	//If the Expert+ track data was written
					expertplus = 0;	//Reset this status
					expertpluswritten = 1;
				}
			}
		}//If this is a legacy track

		else if(sp->track[j]->track_format == EOF_VOCAL_TRACK_FORMAT)
		{	//If this is a vocal track
			/* make vocals track */
			/* pre-parse the lyrics to determine if any pitchless lyrics are present */
			correctlyrics = 0;	//By default, pitchless lyrics will not be changed to freestyle during export
			for(ctr = 0; ctr < sp->vocal_track[tracknum]->lyrics; ctr++)
			{
				if(sp->vocal_track[tracknum]->lyric[ctr]->note == 0)
				{	//If any of the lyrics are missing the pitch, prompt for whether they should be corrected
					eof_cursor_visible = 0;
					eof_pen_visible = 0;
					eof_show_mouse(screen);
					if(alert(NULL, "Write pitchless lyrics as playable freestyle?", NULL, "&Yes", "&No", 'y', 'n') == 1)
					{	//If user opts to have the lyrics corrected, update the correctlyrics variable
						correctlyrics = 1;
					}
					eof_show_mouse(NULL);
					eof_cursor_visible = 1;
					eof_pen_visible = 1;
					break;
				}
			}

			/* pre-parse the lyrics to see if any exist outside of lyric phrases */
			correctphrases = 0;	//By default, missing lyric phrases won't be inserted
			for(ctr = 0; ctr < sp->vocal_track[tracknum]->lyrics; ctr++)
			{
				if(eof_find_lyric_line(ctr) == NULL)
				{	//If this lyric is not in a line and is not a vocal percussion note
					eof_cursor_visible = 0;
					eof_pen_visible = 0;
					eof_show_mouse(screen);
					if(alert(NULL, "Add phrases for lyrics/percussions not in lyric phrases?", NULL, "&Yes", "&No", 'y', 'n') == 1)
					{	//If user opts to have missing lyric phrases inserted
						correctphrases = 1;
					}
					eof_show_mouse(NULL);
					eof_cursor_visible = 1;
					eof_pen_visible = 1;
					break;
				}
			}

			/* insert the missing lyric phrases if the user opted to do so */
			if(correctphrases)
			{
				char phrase_in_progress = 0;	//This will be used to track the open/closed status of the automatic phrases, so adjacent lyrics/percussions can be added to the same auto-generated phrase
				unsigned long phrase_start = 0;	//This will store the delta time of the last opened lyric phrase
				for(ctr = 0; ctr < sp->vocal_track[tracknum]->lyrics; ctr++)
				{
					if(eof_find_lyric_line(ctr) == NULL)
					{	//If this lyric is not in a line and is not a vocal percussion note, write the MIDI events for a line phrase to envelop it
						if(!phrase_in_progress)
						{	//If a note on for the phrase hasn't been added already
							phrase_start = eof_ConvertToDeltaTime(sp->vocal_track[tracknum]->lyric[ctr]->pos,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION);	//Store the tick position of the phrase
							eof_add_midi_event(phrase_start, 0x90, 105, vel, 0);
							phrase_in_progress = 1;
						}
						if(!((ctr + 1 < sp->vocal_track[tracknum]->lyrics) && (eof_find_lyric_line(ctr + 1) == NULL)))
						{	//Only if there isn't a next lyric that is also missing a vocal phrase, write the note off for the phrase
							deltalength = eof_ConvertToDeltaTime(sp->vocal_track[tracknum]->lyric[ctr]->pos + sp->vocal_track[tracknum]->lyric[ctr]->length,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION) - phrase_start;	//Store the number of delta ticks representing the phrase's length
							if(deltalength < 1)
							{	//If some kind of rounding error or other issue caused the delta length to be less than 1, force it to the minimum length of 1
								deltalength = 1;
							}
							eof_add_midi_event(phrase_start + deltalength, 0x80, 105, vel, 0);
							phrase_in_progress = 0;
						}
					}
				}
			}

			/* write the MTrk MIDI data to a temp file
			use size of the file as the MTrk header length */
			for(i = 0; i < sp->vocal_track[tracknum]->lyrics; i++)
			{
				//Copy each lyric string into a new array, perform correction on it if necessary
				tempstring = malloc(sizeof(sp->vocal_track[tracknum]->lyric[i]->text));
				if(tempstring == NULL)	//If allocation failed
				{
					eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
					eof_destroy_ts_list(tslist);	//Free memory used by the TS change list
					return 0;			//Return failure
				}
				sp->vocal_track[tracknum]->lyric[i]->text[EOF_MAX_LYRIC_LENGTH] = '\0';	//Guarantee that the lyric string is terminated
				memcpy(tempstring,sp->vocal_track[tracknum]->lyric[i]->text,sizeof(sp->vocal_track[tracknum]->lyric[i]->text));	//Copy to new array

				deltapos = eof_ConvertToDeltaTime(sp->vocal_track[tracknum]->lyric[i]->pos,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION);
				deltalength = eof_ConvertToDeltaTime(sp->vocal_track[tracknum]->lyric[i]->pos + sp->vocal_track[tracknum]->lyric[i]->length,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION) - deltapos;
				if(deltalength < 1)
				{	//If some kind of rounding error or other issue caused the delta length to be less than 1, force it to the minimum length of 1
					deltalength = 1;
				}
				if(sp->vocal_track[tracknum]->lyric[i]->note > 0)
				{	//For the vocal track, store the converted delta times, to allow for artificial padding for lyric phrase markers
					eof_add_midi_event(deltapos, 0x90, sp->vocal_track[tracknum]->lyric[i]->note, vel, 0);
					eof_add_midi_event(deltapos + deltalength, 0x80, sp->vocal_track[tracknum]->lyric[i]->note, vel, 0);
				}
				else if(correctlyrics)
				{	//If performing pitchless lyric correction, write pitch 50 instead to guarantee it is usable as a freestyle lyric
					eof_add_midi_event(deltapos, 0x90, 50, vel, 0);
					eof_add_midi_event(deltapos + deltalength, 0x80, 50, vel, 0);
					eof_set_freestyle(tempstring,1);		//Ensure the lyric properly ends with a freestyle character
				}
				//Write the string, which was only corrected if correctlyrics was nonzero and the pitch was not defined
				if(sp->vocal_track[tracknum]->lyric[i]->note != VOCALPERCUSSION)
				{	//Do not write a lyric string for vocal percussion notes
					eof_add_midi_lyric_event(eof_ConvertToDeltaTime(sp->vocal_track[tracknum]->lyric[i]->pos,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION), tempstring);
				}
			}
			/* fill in lyric lines */
			for(i = 0; i < sp->vocal_track[tracknum]->lines; i++)
			{
				deltapos = eof_ConvertToDeltaTime(sp->vocal_track[tracknum]->line[i].start_pos,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION);
				deltalength = eof_ConvertToDeltaTime(sp->vocal_track[tracknum]->line[i].end_pos,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION) - deltapos;
				if(deltalength < 1)
				{	//If some kind of rounding error or other issue caused the delta length to be less than 1, force it to the minimum length of 1
					deltalength = 1;
				}
				eof_add_midi_event(deltapos, 0x90, 105, vel, 0);
				eof_add_midi_event(deltapos + deltalength, 0x80, 105, vel, 0);
				if(sp->vocal_track[tracknum]->line[i].flags & EOF_LYRIC_LINE_FLAG_OVERDRIVE)
				{	//For the vocal track, store the converted delta times, to allow for artificial padding for lyric phrase markers
					eof_add_midi_event(eof_ConvertToDeltaTime(sp->vocal_track[tracknum]->line[i].start_pos,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION), 0x90, 116, vel, 0);
					eof_add_midi_event(eof_ConvertToDeltaTime(sp->vocal_track[tracknum]->line[i].end_pos,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION), 0x80, 116, vel, 0);
				}
			}

			/* insert padding as necessary between a lyric phrase on marker and the following lyric */
			#define EOF_LYRIC_PHRASE_PADDING 5
			unsigned long last_phrase = 0;	//Stores the absolute delta time of the last Note 105 On

			qsort(eof_midi_event, eof_midi_events, sizeof(EOF_MIDI_EVENT *), qsort_helper3);	//Lyric events must be sorted for padding logic to work
			for(i = 0; i < eof_midi_events; i++)
			{
				if((eof_midi_event[i]->type == 0x90) && (eof_midi_event[i]->note == 105))
				{	//If this is a lyric on phrase marker
					last_phrase = eof_midi_event[i]->pos;		//Store its position
				}
				else if((i + 2 < eof_midi_events) && (eof_midi_event[i]->type == 0x05) && (eof_midi_event[i+1]->type == 0x90) && (eof_midi_event[i+1]->note < 105) && (eof_midi_event[i+2]->type == 0x80) && (eof_midi_event[i+2]->note < 105))
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
				else if((i + 1 < eof_midi_events) && (eof_midi_event[i]->type == 0x90) && (eof_midi_event[i]->note == VOCALPERCUSSION) && (eof_midi_event[i+1]->type == 0x80) && (eof_midi_event[i+1]->note == VOCALPERCUSSION))
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

			qsort(eof_midi_event, eof_midi_events, sizeof(EOF_MIDI_EVENT *), qsort_helper3);

			/* open the file */
			fp = pack_fopen(notetempname[j], "w");
			if(!fp)
			{
				eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
				eof_destroy_ts_list(tslist);	//Free memory used by the TS change list
				return 0;
			}

			/* write the track name */
			WriteVarLen(0, fp);
			pack_putc(0xFF, fp);
			pack_putc(0x03, fp);
			WriteVarLen(ustrlen(sp->track[j]->name), fp);
			pack_fwrite(sp->track[j]->name, ustrlen(sp->track[j]->name), fp);

			/* add MIDI events */
			lastdelta=0;
			for(i = 0; i < eof_midi_events; i++)
			{
				delta = eof_midi_event[i]->pos;
				WriteVarLen(delta - lastdelta, fp);	//Write this lyric's relative delta time
				lastdelta=delta;					//Store this lyric's absolute delta time
				if(eof_midi_event[i]->type == 0x05)
				{
					pack_putc(0xFF, fp);
					pack_putc(0x05, fp);
					pack_putc(ustrlen(eof_midi_event[i]->dp), fp);
					pack_fwrite(eof_midi_event[i]->dp, ustrlen(eof_midi_event[i]->dp), fp);
					free(eof_midi_event[i]->dp);	//Free the copied string from memory
				}
				else
				{
					pack_putc(eof_midi_event[i]->type, fp);
					pack_putc(eof_midi_event[i]->note, fp);
					pack_putc(vel, fp);
				}
			}

			/* end of track */
			WriteVarLen(0, fp);
			pack_putc(0xFF, fp);
			pack_putc(0x2F, fp);
			pack_putc(0x00, fp);
			pack_fclose(fp);
		}//If this is a vocal track

		else if(sp->track[j]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If this is a pro guitar track
			/* fill in notes */
			/* write the MTrk MIDI data to a temp file
			use size of the file as the MTrk header length */
			lastname = nochord;
			for(i = 0; i < eof_get_track_size(sp, j); i++)
			{	//For each note in the track
				switch(eof_get_note_type(sp, j, i))
				{
					case EOF_NOTE_AMAZING:	//notes 96-101
					{
						midi_note_offset = 96;
						slidenote = 103;	//The note used to mark an Amazing slide
						break;
					}
					case EOF_NOTE_MEDIUM:	//notes 72-77
					{
						midi_note_offset = 72;
						slidenote = 79;		//The note used to mark a Medium slide
						break;
					}
					case EOF_NOTE_EASY:		//notes 48-58
					{
						midi_note_offset = 48;
						slidenote = 55;		//The note used to mark an easy slide
						break;
					}
					case EOF_NOTE_SUPAEASY:	//notes 24-29
					{
						midi_note_offset = 24;
						slidenote = 31;		//The note used to mark a supaeasy slide
						break;
					}
					case EOF_NOTE_SPECIAL:	//BRE/drum fill: notes 120-124
					{
						midi_note_offset = 120;
						break;
					}
				}

				noteflags = eof_get_note_flags(sp, j, i);	//Store the note flags for easier use
				note = eof_get_note_note(sp, j, i);			//Store the note bitflag for easier use
				notepos = eof_get_note_pos(sp, j, i);		//Store the note position for easier use
				length = eof_get_note_length(sp, j, i);		//Store the note length for easier use
				currentname = eof_get_note_name(sp, j, i);

				deltapos = eof_ConvertToDeltaTime(notepos,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION);	//Store the tick position of the note
				deltalength = eof_ConvertToDeltaTime(notepos + length,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION) - deltapos;	//Store the number of delta ticks representing the note's length
				if(deltalength < 1)
				{	//If some kind of rounding error or other issue caused the delta length to be less than 1, force it to the minimum length of 1
					deltalength = 1;
				}
				if((currentname == NULL) || (currentname[0] == '\0'))		//If this note has no name
					currentname = nochord;	//Refer to its name as "NC"
				if(((lastname == nochord) && (currentname != nochord)) || ((lastname != nochord) && (currentname == nochord)) || (ustrcmp(lastname, currentname) != 0))
				{	//If the previous note wasn't named and this one is, or the previous note was named and this one isn't, or the previous and current notes have different names
					snprintf(chordname, sizeof(chordname), "[Chord=\"%s\"]", currentname);	//Build the chord name text event
					tempstring = malloc(ustrsizez(chordname));
					if(tempstring != NULL)
					{	//If allocation was successful
						memcpy(tempstring, chordname, ustrsizez(chordname));	//Copy the string to the newly allocated memory
						eof_add_midi_lyric_event(deltapos, tempstring);			//Store the new string in a text event
					}
					lastname = currentname;
				}

				/* write slide sections */
				if(noteflags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN)
				{	//If this note slides down
					eof_add_midi_event(deltapos, 0x90, slidenote, 104, 0);	//Velocity 104 denotes slide down
					eof_add_midi_event(deltapos + deltalength, 0x80, slidenote, 104, 0);
				}
				else if(noteflags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP)
				{	//If this note slides up
					eof_add_midi_event(deltapos, 0x90, slidenote, 102, 0);	//Velocity 102 denotes slide up
					eof_add_midi_event(deltapos + deltalength, 0x80, slidenote, 102, 0);
				}

				/* write note gems */
				for(ctr = 0, bitmask = 1; ctr < 6; ctr++, bitmask <<= 1)
				{	//For each of the 6 usable strings
					if((noteflags & EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE) || (sp->pro_guitar_track[tracknum]->note[i]->frets[ctr] == 0xFF))
					{	//Mute gems are written on channel 3
						channel = 3;
					}
					else if(noteflags & EOF_PRO_GUITAR_NOTE_FLAG_HO)
					{	//Forced hammer on gems are written on channel 2
						channel = 2;
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
					{	//If this is a muted gem
						velocity = 100;	//Write it as a muted note at fret 0
					}
					else
					{	//Otherwise write it normally
						velocity = sp->pro_guitar_track[tracknum]->note[i]->frets[ctr] + 100;	//Velocity (100 + X) represents fret # X
					}
					if(note & bitmask)
					{	//If the note uses this string
						eof_add_midi_event(deltapos, 0x90, midi_note_offset + ctr, velocity, channel);	//Write the note on event
						eof_add_midi_event(deltapos + deltalength, 0x80, midi_note_offset + ctr, velocity, channel);	//Write the note off event
					}
				}

				if(eof_get_note_type(sp, j, i) == EOF_NOTE_AMAZING)
				{	//For the Expert difficulty, write required specialty notes
				/* write left hand position note, which is a note 108 with the same velocity of the lowest fret used in the pro guitar note */
					rootvel = 0xFF;
					for(ctr = 0, bitmask = 1; ctr < 6; ctr++, bitmask <<= 1)
					{	//For each of the 6 usable strings, from lowest to highest gauge
						if(note & bitmask)
						{	//If this string is used
							if(rootvel == 0xFF)
							{	//If no velocity has been recorded so far
								rootvel = sp->pro_guitar_track[tracknum]->note[i]->frets[ctr] + 100;	//Store the velocity used for this gem
							}
							else if(sp->pro_guitar_track[tracknum]->note[i]->frets[ctr] + 100 < rootvel)
							{	//Otherwise store this gem's velocity if it is lower than the others checked for this note
								rootvel = sp->pro_guitar_track[tracknum]->note[i]->frets[ctr] + 100;
							}
						}
					}
					eof_add_midi_event(deltapos, 0x90, 108, rootvel, 0);	//Note 108 denotes a root note
					eof_add_midi_event(deltapos + deltalength, 0x80, 108, 64, 0);	//Write the note off event (using the same velocity that RB3 MIDIs use)

				/* write root note, which is a note from 4 to 15, to represent the chord's major scale (where any E scale chord is 4, F is 5, Bb is 6, ..., Eb is 15) */
					if(eof_note_count_colors(sp, j, i) > 1)
					{	//If this is a chord
						scale = 0;	//By default, assume an E chord
						eof_lookup_chord(sp, j, i, &scale, &chord);	//Update the scale variable if a chord match is found
						scale = (scale + 9) % 16 + (4 * ((scale + 9) / 16));	//Convert the scale to RB3's numbering system
						eof_add_midi_event(deltapos, 0x90, scale, vel, 0);		//Write a root note reflecting the scale the chord is in
						eof_add_midi_event(deltapos + deltalength, 0x80, scale, vel, 0);
					}
				}
			}//For each note in the track

			/* fill in arpeggios */
			for(i = 0; i < eof_get_num_arpeggios(sp, j); i++)
			{	//For each arpeggio in the track
				sectionptr = eof_get_arpeggio(sp, j, i);
				deltapos = eof_ConvertToDeltaTime(sectionptr->start_pos,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION);	//Store the tick position of the phrase
				deltalength = eof_ConvertToDeltaTime(sectionptr->end_pos,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION) - deltapos;	//Store the number of delta ticks representing the phrase's length
				if(deltalength < 1)
				{	//If some kind of rounding error or other issue caused the delta length to be less than 1, force it to the minimum length of 1
					deltalength = 1;
				}
				eof_add_midi_event(deltapos, 0x90, 104, vel, 0);	//Note 104 denotes an arpeggio marker
				eof_add_midi_event(deltapos + deltalength, 0x80, 104, vel, 0);
			}

			/* fill in solos */
			for(i = 0; i < eof_get_num_solos(sp, j); i++)
			{	//For each solo in the track
				sectionptr = eof_get_solo(sp, j, i);
				deltapos = eof_ConvertToDeltaTime(sectionptr->start_pos,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION);	//Store the tick position of the phrase
				deltalength = eof_ConvertToDeltaTime(sectionptr->end_pos,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION) - deltapos;	//Store the number of delta ticks representing the phrase's length
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
				deltapos = eof_ConvertToDeltaTime(sectionptr->start_pos,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION);	//Store the tick position of the phrase
				deltalength = eof_ConvertToDeltaTime(sectionptr->end_pos,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION) - deltapos;	//Store the number of delta ticks representing the phrase's length
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
				deltapos = eof_ConvertToDeltaTime(sectionptr->start_pos,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION);	//Store the tick position of the phrase
				deltalength = eof_ConvertToDeltaTime(sectionptr->end_pos,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION) - deltapos;	//Store the number of delta ticks representing the phrase's length
				if(deltalength < 1)
				{	//If some kind of rounding error or other issue caused the delta length to be less than 1, force it to the minimum length of 1
					deltalength = 1;
				}
				eof_add_midi_event(deltapos, 0x90, 126, vel, 0);	//Note 126 denotes a tremolo marker
				eof_add_midi_event(deltapos + deltalength, 0x80, 126, vel, 0);
			}

			/* fill in trills */
			for(i = 0; i < eof_get_num_trills(sp, j); i++)
			{	//For each trill in the track
				sectionptr = eof_get_trill(sp, j, i);
				deltapos = eof_ConvertToDeltaTime(sectionptr->start_pos,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION);	//Store the tick position of the phrase
				deltalength = eof_ConvertToDeltaTime(sectionptr->end_pos,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION) - deltapos;	//Store the number of delta ticks representing the phrase's length
				if(deltalength < 1)
				{	//If some kind of rounding error or other issue caused the delta length to be less than 1, force it to the minimum length of 1
					deltalength = 1;
				}
				eof_add_midi_event(deltapos, 0x90, 127, vel, 0);	//Note 127 denotes a trill marker
				eof_add_midi_event(deltapos + deltalength, 0x80, 127, vel, 0);
			}

			for(i=0;i < 128;i++)
			{	//Ensure that any notes that are still on are terminated
				if(eof_midi_note_status[i] != 0)	//If this note was left on, send an alert message, as this is abnormal
				{
					allegro_message("MIDI export error:  Note %lu was not turned off", i);
//					notenum = eof_get_track_size(sp, j) - 1;	//The index of the last note in this track
//					eof_add_midi_event(eof_get_note_pos(sp, j, notenum) + eof_get_note_length(sp, j, notenum), 0x80, i, vel, 0);
				}
			}
			qsort(eof_midi_event, eof_midi_events, sizeof(EOF_MIDI_EVENT *), qsort_helper3);
//			allegro_message("break1");

			/* open the file */
			fp = pack_fopen(notetempname[j], "w");
			if(!fp)
			{
				eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
				eof_destroy_ts_list(tslist);	//Free memory used by the TS change list
				return 0;
			}

			/* write the track name */
			WriteVarLen(0, fp);
			pack_putc(0xFF, fp);
			pack_putc(0x03, fp);
			WriteVarLen(ustrlen(sp->track[j]->name), fp);
			pack_fwrite(sp->track[j]->name, ustrlen(sp->track[j]->name), fp);

			/* add MIDI events */
			lastdelta = 0;
			for(i = 0; i < eof_midi_events; i++)
			{
				delta = eof_midi_event[i]->pos;
				if(eof_midi_event[i]->type == 0x05)
				{	//Write a note name text event
					eof_write_text_event(delta-lastdelta, eof_midi_event[i]->dp, fp);
					free(eof_midi_event[i]->dp);	//Free the copied string from memory
				}
				else
				{	//Write a non meta MIDI event
					WriteVarLen(delta-lastdelta, fp);	//Write this event's relative delta time
					pack_putc(eof_midi_event[i]->type + eof_midi_event[i]->channel, fp);
					pack_putc(eof_midi_event[i]->note, fp);
					pack_putc(eof_midi_event[i]->velocity, fp);
				}
				lastdelta = delta;					//Store this event's absolute delta time
			}

			/* end of track */
			WriteVarLen(0, fp);
			pack_putc(0xFF, fp);
			pack_putc(0x2F, fp);
			pack_putc(0x00, fp);
			pack_fclose(fp);
		}//If this is a pro guitar track
	}//For each track in the project

/* make tempo track */
	fp = pack_fopen(tempotempname, "w");
	if(!fp)
	{
		eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
		eof_destroy_ts_list(tslist);	//Free memory used by the TS change list
		return 0;
	}

	#ifdef EOF_RBN_COMPATIBILITY
	//I've found that Magma will not recognize tracks correctly unless track 0 has a name defined
	/* write the track name */
	WriteVarLen(0, fp);
	pack_putc(0xFF, fp);
	pack_putc(0x03, fp);
	WriteVarLen(ustrlen(sp->tags->title), fp);
	snprintf(chordname, sizeof(chordname), "%s", sp->tags->title);	//Borrow this array to store the chart title
	pack_fwrite(chordname, ustrlen(chordname), fp);
	#endif

	lastdelta=0;
	unsigned long current_ts=0;
	unsigned long nextanchorpos=0,next_tspos=0;
	char whattowrite;	//Bitflag: bit 0=write tempo change, bit 1=write TS change
	ptr = anchorlist;
	while((ptr != NULL) || (eof_use_ts && (current_ts < tslist->changes)))
	{	//While there are tempo changes or TS changes (if the user specified to write TS changes) to write
		whattowrite = 0;
		if(ptr != NULL)
		{
			nextanchorpos=ptr->delta;
			whattowrite |= 1;
		}
		if(eof_use_ts && (current_ts < tslist->changes))
		{	//Only process TS changes if the user opted to do so
			next_tspos=tslist->change[current_ts]->pos;
			whattowrite |= 2;
		}

		if(whattowrite > 2)
		{	//If both a TS change and a tempo change remain to be written
			if(nextanchorpos < next_tspos)	//If the tempo change is earlier
				whattowrite = 1;			//write it first
			else if(eof_use_ts)
				whattowrite = 2;			//otherwise write the TS change first
		}

		if(whattowrite == 1)
		{	//If writing a tempo change
			WriteVarLen(ptr->delta - lastdelta, fp);	//Write this anchor's relative delta time
			lastdelta=ptr->delta;						//Store this anchor's absolute delta time

			ppqn = ((double) 60000000.0 / ptr->BPM) + 0.5;	//Convert BPM to ppqn, rounding up
			pack_putc(0xFF, fp);					//Write Meta Event 0x51 (Set Tempo)
			pack_putc(0x51, fp);
			pack_putc(0x03, fp);					//Write event length of 3
			pack_putc((ppqn & 0xFF0000) >> 16, fp);	//Write high order byte of ppqn
			pack_putc((ppqn & 0xFF00) >> 8, fp);	//Write middle byte of ppqn
			pack_putc((ppqn & 0xFF), fp);			//Write low order byte of ppqn
			ptr=ptr->next;							//Iterate to next anchor
		}
		else if(eof_use_ts)
		{	//If writing a TS change
			WriteVarLen(tslist->change[current_ts]->pos - lastdelta, fp);	//Write this time signature's relative delta time
			lastdelta=tslist->change[current_ts]->pos;						//Store this time signature's absolute delta time

			for(i=0;i<=8;i++)
			{	//Convert the denominator into the power of two required to write into the MIDI event
				if(tslist->change[current_ts]->den >> i == 1)	//if 2 to the power of i is the denominator
					break;
			}
			if(tslist->change[current_ts]->den >> i != 1)
			{	//If the loop ended before the appropriate value was found
				i = 2;	//An unsupported time signature was somehow set, change the denominator to 4
			}

			pack_putc(0xFF, fp);							//Write Meta Event 0x58 (Time Signature)
			pack_putc(0x58, fp);
			pack_putc(0x04, fp);							//Write event length of 4
			pack_putc(tslist->change[current_ts]->num, fp);	//Write the numerator
			pack_putc(i, fp);								//Write the denominator
			pack_putc(24, fp);								//Write the metronome interval (not used by EOF)
			pack_putc(8, fp);								//Write the number of 32nd notes per 24 ticks (not used by EOF)
			current_ts++;									//Iterate to next TS change
		}
	}
	WriteVarLen(0, fp);		//Write delta time
	pack_putc(0xFF, fp);	//Write Meta Event 0x2F (End Track)
	pack_putc(0x2F, fp);
	pack_putc(0x00, fp);	//Write padding
	pack_fclose(fp);


/* make events track */
	#ifdef EOF_RBN_COMPATIBILITY
	//Magma requires some default track events to be written, even if there are no text events defined
	if(sp->beats > 3)
	{	//Only add these if there are at least 4 beats
		//Check the existing events to see if a [music_start] event is defined
		match = 0;
		for(i = 0; i < sp->text_events; i++)
		{
			if(!ustrcmp(sp->text_event[i]->text, "[music_start]"))
			{
				match = 1;
			}
		}
		if(!match)
		{	//If there wasn't one, add one
			eof_add_text_event(sp, 2, "[music_start]");				//Add the [music_start] event two beats into the song (at the third beat)
		}

		//Check the existing events to see if a [music_end] event is defined
		match = 0;
		for(i = 0; i < sp->text_events; i++)
		{
			if(!ustrcmp(sp->text_event[i]->text, "[music_end]"))
			{
				match = 1;
			}
		}
		if(!match)
		{	//If there wasn't one, add one
			eof_add_text_event(sp, sp->beats-1, "[music_end]");		//Add the [music_end] event on the last beat
		}
	}
	#else
	//Otherwise only write an EVENTS track if the user manually defined text events
	if(sp->text_events)
	{
	#endif
		eof_sort_events();

		/* open the file */
		fp = pack_fopen(eventtempname, "w");
		if(!fp)
		{
			eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
			eof_destroy_ts_list(tslist);	//Free memory used by the TS change list
			return 0;
		}

		/* write the track name */
		WriteVarLen(0, fp);
		pack_putc(0xFF, fp);
		pack_putc(0x03, fp);
		WriteVarLen(ustrlen("EVENTS"), fp);
		pack_fwrite("EVENTS", ustrlen("EVENTS"), fp);

		/* add MIDI events */
		lastdelta = 0;
		for(i = 0; i < sp->text_events; i++)
		{
			if(sp->text_event[i]->beat >= sp->beats)
			{	//If the text event is corrupted
				sp->text_event[i]->beat = sp->beats - 1;	//Repair it by assigning it to the last beat marker
			}
			delta = eof_ConvertToDeltaTime(sp->beat[sp->text_event[i]->beat]->fpos,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION);
			eof_write_text_event(delta - lastdelta, sp->text_event[i]->text, fp);
			lastdelta = delta;					//Store this event's absolute delta time
		}

		#ifdef EOF_RBN_COMPATIBILITY
		//Magma requires that the [end] event is the last MIDI event in the track, so it will be written 1ms after the end of the audio
		//Check the existing events to see if a [music_end] event is defined
		match = 0;
		for(i = 0; i < sp->text_events; i++)
		{
			if(!ustrcmp(sp->text_event[i]->text, "[end]"))
			{
				match = 1;
			}
		}
		if(!match)
		{	//If there wasn't one, add one
			delta = eof_ConvertToDeltaTime(eof_music_length+1,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION);
			eof_write_text_event(delta - lastdelta, "[end]", fp);
			lastdelta = delta;					//Store this event's absolute delta time
		}
		#endif

		/* end of track */
		WriteVarLen(0, fp);
		pack_putc(0xFF, fp);
		pack_putc(0x2F, fp);
		pack_putc(0x00, fp);

		pack_fclose(fp);
		eventstrackwritten = 1;
	#ifndef EOF_RBN_COMPATIBILITY
	}
	#endif


/* make beat track */
	#ifdef EOF_RBN_COMPATIBILITY
		/* open the file */
	fp = pack_fopen(beattempname, "w");
	if(!fp)
	{
		eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
		eof_destroy_ts_list(tslist);	//Free memory used by the TS change list
		return 0;
	}

	/* write the track name */
	WriteVarLen(0, fp);
	pack_putc(0xFF, fp);
	pack_putc(0x03, fp);
	WriteVarLen(ustrlen("BEAT"), fp);
	pack_fwrite("BEAT", ustrlen("BEAT"), fp);

	/* parse the beat array, writing a note #12 at the first beat of every measure, and a note #13 at every other beat */
	unsigned long beat_counter = 0;
	unsigned beats_per_measure = 4;		//By default, a 4/4 time signature is assumed until a TS event is reached
	unsigned note_to_write = 0;
	unsigned length_to_write = 0;

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
		delta = eof_ConvertToDeltaTime(sp->beat[i]->pos,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION);
		WriteVarLen(delta-lastdelta, fp);	//Write this event's relative delta time
		lastdelta = delta;			//Store this event's absolute delta time
		pack_putc(0x90, fp);			//MIDI event 0x9 (note on), channel 0
		pack_putc(note_to_write, fp);		//Note 12 or 13
		pack_putc(100, fp);			//Pre-determined velocity

		//Write the note off event
		delta = eof_ConvertToDeltaTime(sp->beat[i]->pos + length_to_write,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION);
		WriteVarLen(delta-lastdelta, fp);	//Write this event's relative delta time
		lastdelta = delta;			//Store this event's absolute delta time
		pack_putc(0x80, fp);			//MIDI event 0x8 (note off), channel 0
		pack_putc(note_to_write, fp);		//Note 12 or 13
		pack_putc(100, fp);			//Pre-determined velocity

		//Increment to the next beat
		beat_counter++;
		if(beat_counter >= beats_per_measure)
		{
			beat_counter = 0;
		}
	}

	/* end of track */
	WriteVarLen(0, fp);
	pack_putc(0xFF, fp);
	pack_putc(0x2F, fp);
	pack_putc(0x00, fp);

	pack_fclose(fp);
	beattrackwritten = 1;
	#endif


	fp = pack_fopen(fn, "w");
	if(!fp)
	{
		eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
		eof_destroy_ts_list(tslist);	//Free memory used by the TS change list
		return 0;
	}

	/* write header data */
	header[11] = eof_count_tracks() + 1 + eventstrackwritten + beattrackwritten;	//Add 1 for track 0 and one each for the events and beat tracks if applicable
	pack_fwrite(header, 14, fp);

	if(expertpluswritten)
	{
		replace_filename(expertplusfilename, fn, expertplusshortname, 1024);	//Build the path for the output expert+ MIDI
		fp3 = pack_fopen(expertplusfilename, "w");
		if(!fp3)
			expertpluswritten = 0;	//Cancel trying to write Expert+ and save the normal MIDI instead
		else
			pack_fwrite(header, 14, fp3);	//Write the Expert+ file's MIDI header
	}


/* write tempo track */
	eof_dump_midi_track(tempotempname,fp);
	if(expertpluswritten)
	{	//If writing an expert+ MIDI as well
		eof_dump_midi_track(tempotempname,fp3);
	}

/* write text event track if there are any events */
	#ifndef EOF_RBN_COMPATIBILITY
	//If RBN compatibility is in effect, the events track will be populated by force with at least the required events
	if(sp->text_events)
	{
	#endif
		eof_dump_midi_track(eventtempname,fp);
		if(expertpluswritten)
		{
			eof_dump_midi_track(eventtempname,fp3);
		}
	#ifndef EOF_RBN_COMPATIBILITY
	}
	#endif

	#ifdef EOF_RBN_COMPATIBILITY
/* If RBN compatibility is in effect, write a beat track */
	eof_dump_midi_track(beattempname,fp);
	if(expertpluswritten)
	{	//If writing an expert+ MIDI as well
		eof_dump_midi_track(tempotempname,fp3);
	}
	#endif

/* write tracks */
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
					eof_dump_midi_track(expertplustempname,fp);
				}
				else
				{	//Otherwise write the regular track
					eof_dump_midi_track(notetempname[j],fp);
				}
			}
		}
		pack_fclose(fp);	//Close the output file
		fp = NULL;
	}
	eof_clear_midi_events();


/* delete temporary files */
	for(i = 0; i < EOF_TRACKS_MAX+1; i++)
	{
		delete_file(notetempname[i]);
	}
	delete_file(expertplustempname);
	delete_file(tempotempname);
	delete_file(eventtempname);
	delete_file(beattempname);

	eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
	eof_destroy_ts_list(tslist);	//Free memory used by the TS change list

	return 1;
}

struct Tempo_change *eof_build_tempo_list(EOF_SONG *sp)
{
	eof_log("eof_build_tempo_list() entered", 1);

	unsigned long ctr;
	struct Tempo_change *list=NULL;	//The linked list
	struct Tempo_change *temp=NULL;
	unsigned long lastppqn=0;	//Tracks the last anchor's PPQN value
	unsigned long deltactr=0;	//Counts the number of deltas between anchors
	unsigned den=4;				//Stores the most recent TS change's denominator (default to 4)

	if((sp == NULL) || (sp->beats < 1))
	{
		return NULL;
	}
	for(ctr=0;ctr < sp->beats;ctr++)
	{	//For each beat
		if(eof_use_ts)
		{	//If the user opted to use time signatures during MIDI export
			eof_get_ts(sp,NULL,&den,ctr);	//Update the TS denominator if applicable
		}
		if(sp->beat[ctr]->ppqn != lastppqn)
		{	//If this beat has a different tempo than the last, add it to the list
			if((list == NULL) && (sp->beat[ctr]->fpos != 0.0))	//If the first anchor isn't at position 0
				temp=eof_add_to_tempo_list(0,0.0,(double)120.0,list);	//Add a default 120BPM anchor

			lastppqn=sp->beat[ctr]->ppqn;	//Remember this ppqn
			temp=eof_add_to_tempo_list(deltactr,sp->beat[ctr]->fpos,(double)60000000.0/lastppqn,list);

			if(temp == NULL)
			{
				eof_destroy_tempo_list(list);	//Destroy list
				return NULL;			//Return error
			}
			list=temp;	//Update list pointer
		}

//This commented out line was causing timing errors when importing a RB MIDI that used #/8 TS
//		deltactr+=((double)EOF_DEFAULT_TIME_DIVISION * den / 4.0) + 0.5;	//Add the number of deltas of one beat (scale to convert from deltas per quarternote) to the counter
		deltactr+=(double)EOF_DEFAULT_TIME_DIVISION + 0.5;	//Add the number of deltas of one beat to the counter
	}

	return list;
}

struct Tempo_change *eof_add_to_tempo_list(unsigned long delta,double realtime,double BPM,struct Tempo_change *ptr)
{
	eof_log("eof_add_to_tempo_list() entered", 2);	//Only log this if verbose logging is on

	struct Tempo_change *temp=NULL;
	struct Tempo_change *cond=NULL;	//A conductor for the linked list

//Allocate and initialize new link
	temp=(struct Tempo_change *)malloc(sizeof(struct Tempo_change));
	if(temp == NULL)
	{
		return NULL;
	}
	temp->delta=delta;
	temp->realtime=realtime;
	temp->BPM=BPM;
	temp->next=NULL;

//Append to linked list
	if(ptr == NULL)		//If the passed list was empty
	{
		return temp;	//Return the new head link
	}
	for(cond=ptr;cond->next != NULL;cond=cond->next);	//Seek to last link in the list

	cond->next=temp;	//Last link points forward to new link
	return ptr;			//Return original head link
}

void eof_destroy_tempo_list(struct Tempo_change *ptr)
{
	eof_log("eof_destroy_tempo_list() entered", 1);

	struct Tempo_change *temp=NULL;

	while(ptr != NULL)
	{
		temp=ptr->next;	//Store this pointer
		free(ptr);	//Free this link
		ptr=temp;	//Point to next link
	}
}

unsigned long eof_ConvertToDeltaTime(double realtime,struct Tempo_change *anchorlist,EOF_MIDI_TS_LIST *tslist,unsigned long timedivision)
{	//Uses the Tempo Changes list to calculate the absolute delta time of the specified realtime
//	eof_log("eof_ConvertToDeltaTime() entered");

	struct Tempo_change *temp=anchorlist;	//Stores the closest tempo change before the specified realtime
	double tstime=0.0;						//Stores the realtime position of the closest TS change before the specified realtime
	unsigned long tsdelta=0;				//Stores the delta time position of the closest TS change before the specified realtime
	unsigned int den=4;						//Stores the denominator of the closest TS change before the specified realtime (defaults to 4 as per MIDI specification)
	unsigned long delta=0;
	double reltime=0.0;
	unsigned long ctr=0;

	assert_wrapper(temp != NULL);	//Ensure the tempomap is populated

//Find the last time signature change at or before the specified real time value
	if((tslist != NULL) && (tslist->changes > 0))
	{	//If there's at least one TS change
		for(ctr=0;ctr < tslist->changes;ctr++)
		{
			if(realtime >= tslist->change[ctr]->realtime)
			{	//If the TS change is at or before the target realtime
				den = tslist->change[ctr]->den;				//Store this time signature's denominator for use in the conversion
				tstime = tslist->change[ctr]->realtime;		//Store the realtime position
				tsdelta = tslist->change[ctr]->pos;			//Store the delta time position
			}
		}
	}

//Find the last tempo change at or before the specified real time value
	while((temp->next != NULL) && (realtime >= (temp->next)->realtime))	//For each tempo change,
	{	//If the tempo change is at or before the target realtime
		temp=temp->next;	//Advance to that time stamp
	}

//Find the latest tempo or TS change that occurs before the target realtime position and use that event's timing for the conversion
	if(tstime > temp->realtime)
	{	//If the TS change is closer to the target realtime, find the delta time relative from this event
		delta=tsdelta;				//Store the absolute delta time for this TS change
		reltime=realtime - tstime;	//Find the relative timestamp from this TS change
	}
	else
	{	//Find the delta time relative from the closest tempo change
		delta=temp->delta;	//Store the absolute delta time for the anchor
		reltime=realtime - temp->realtime;	//Find the relative timestamp from this tempo change
	}

//reltime is the amount of time we need to find a relative delta for, and add to the absolute delta time of the nearest preceding tempo/TS change
//By using the updated formula respecting time signature:	realtime = (delta / divisions) * (60000.0 / BPM) * TS_den/4;
//The formula for delta is:		delta = realtime * divisions * BPM / 60000 * TS_den / 4

//This commented out line was causing timing errors when importing a RB MIDI that used #/8 TS
//	delta+=(unsigned long)((reltime * (double)timedivision * temp->BPM / 240000.0 * (double)den) + 0.5);
	delta+=(unsigned long)((reltime * (double)timedivision * temp->BPM / 60000.0) + 0.5);

//The old conversion formula that doesn't take time signature into account
//By using NewCreature's formula:	realtime = (delta / divisions) * (60000.0 / bpm)
//The formula for delta is:		delta = realtime * divisions * bpm / 60000
//	delta+=(unsigned long)((temptime * (double)timedivision * temp->BPM / (double)60000.0 + (double)0.5));			//Add .5 so that the delta counter is rounded to the nearest 1

	return delta;
}

int eof_extract_rba_midi(const char * source, const char * dest)
{
	eof_log("eof_extract_rba_midi() entered", 1);

	FILE *fp=NULL;
	FILE *tempfile=NULL;
	unsigned long ctr=0;
	int jumpcode = 0;
	char buffer[15]={0};

//Open specified file
	if((source == NULL) || (dest == NULL))
	{
		return 1;	//Return failure
	}
	fp=fopen(source,"rb");
	if(fp == NULL)
	{
		return 1;	//Return failure
	}

//Set up for catching an exception thrown by FoFLC's logic in the event of an error (such as an invalid MIDI file)
	jumpcode=setjmp(jumpbuffer); //Store environment/stack/etc. info in the jmp_buf array
	if(jumpcode!=0) //if program control returned to the setjmp() call above returning any nonzero value
	{
		puts("Assert() handled sucessfully!");
		eof_show_mouse(NULL);
		eof_cursor_visible = 1;
		eof_pen_visible = 1;
		fclose(fp);
		if(tempfile)
			fclose(tempfile);
		ReleaseMIDI();
		ReleaseMemory(1);
		return 1;	//Return failure
	}

//Load MIDI information (parsing the RBA header if present)
	ReleaseMIDI();		//Ensure FoFLC variables are reset
	ReleaseMemory(1);
	InitMIDI();
	InitLyrics();
	Lyrics.quick=1;		//Should be fine to skip everything except loading basic track info
	MIDI_Load(fp,NULL,0);
	if(MIDIstruct.hchunk.numtracks)
	{	//If at least one valid MIDI track was parsed
//Copy MIDI contents into file
		tempfile=fopen(dest,"wb");
		if(tempfile != NULL)
		{	//Seek to MIDI header and begin copying content
			rewind(fp);
			if(SearchPhrase(fp,0,NULL,"MThd",4,1) != 1)	//Search for and seek to MIDI header
			{
				fclose(tempfile);
				fclose(fp);
				ReleaseMIDI();
				ReleaseMemory(1);
				return 1;	//Return error
			}

			//Copy the file header
			fread_err(buffer,14,1,fp);			//Read MIDI header
			fwrite_err(buffer,14,1,tempfile);	//Write MIDI header

			//Copy tracks
			for(ctr=0;ctr<MIDIstruct.hchunk.numtracks;ctr++)	//For each track
			{
				CopyTrack(fp,ctr,tempfile);
			}
			fclose(tempfile);
		}
		ReleaseMIDI();
		ReleaseMemory(1);
	}

	fclose(fp);
	return 0;	//Return success
}

EOF_MIDI_TS_LIST * eof_create_ts_list(void)
{
	eof_log("eof_create_ts_list() entered", 1);

	unsigned long ctr;

	EOF_MIDI_TS_LIST * lp;
	lp = malloc(sizeof(EOF_MIDI_TS_LIST));
	if(!lp)
	{
		return NULL;
	}

	for(ctr=0;ctr<EOF_MAX_TS;ctr++)
	{	//Init all pointers in the array as NULL
		lp->change[ctr]=NULL;
	}
	lp->changes = 0;
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
	eof_log("eof_destroy_ts_list() entered", 1);

	unsigned long i;

	if(ptr != NULL)
	{
		for(i = 0; i < ptr->changes; i++)
		{
			free(ptr->change[i]);
		}
		free(ptr);
	}
}

EOF_MIDI_TS_LIST *eof_build_ts_list(struct Tempo_change *anchorlist)
{
	eof_log("eof_build_ts_list() entered", 1);

	unsigned long ctr;
	unsigned num=4,den=4;			//Stores the current time signature
	EOF_MIDI_TS_LIST * tslist=NULL;
	unsigned long deltapos = 0;		//Stores the ongoing delta time
	double deltafpos = 0.0;			//Stores the ongoing delta time (with double floating precision)
	double beatlength = 0.0;		//Stores the current beat's length in deltas

	if((eof_song == NULL) || (eof_song->beats <= 0))
		return NULL;
	tslist = eof_create_ts_list();
	if(tslist == NULL)
		return NULL;

	for(ctr=0;ctr < eof_song->beats;ctr++)
	{	//For each beat, create a list of Time Signature changes and store the appropriate delta position of each
		if(eof_get_ts(eof_song,&num,&den,ctr) == 1)
		{	//If a time signature exists on this beat
			eof_midi_add_ts_realtime(tslist, eof_song->beat[ctr]->fpos, num, den, 0);	//Store the beat marker's time signature
			tslist->change[tslist->changes-1]->pos = deltapos;	//Store the time signature's position in deltas
		}

//This commented out line was causing timing errors when importing a RB MIDI that used #/8 TS
//		beatlength = ((double)EOF_DEFAULT_TIME_DIVISION * den / 4.0);		//Determine the length of this beat in deltas
		beatlength = (double)EOF_DEFAULT_TIME_DIVISION;		//Determine the length of this beat in deltas
		deltafpos += beatlength;	//Add the delta length of this beat to the delta counter
		deltapos = deltafpos + 0.5;	//Round up to nearest delta
	}

	return tslist;
}

int eof_get_ts(EOF_SONG *sp,unsigned *num,unsigned *den,int beatnum)
{
//	eof_log("eof_get_ts() entered");

	unsigned numerator=0,denominator=0;

	if((sp == NULL) || (beatnum >= sp->beats) || (sp->beat[beatnum] == NULL))
		return -1;	//Return error

	if(sp->beat[beatnum]->flags & EOF_BEAT_FLAG_START_4_4)
	{
		numerator = 4;
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

int eof_apply_ts(unsigned num,unsigned den,int beatnum,EOF_SONG *sp,char undo)
{
//	eof_log("eof_apply_ts() entered");

	int flags = 0;

	if((sp == NULL) || (beatnum >= sp->beats))
	{
		return 0;	//Return error
	}
	if((num > 0) && (num <= 256) && ((den == 1) || (den == 2) || (den == 4) || (den == 8) || (den == 16) || (den == 32) || (den == 64) || (den == 128) || (den == 256)))
	{	//If this is a valid time signature
		//Clear the beat's status except for its anchor and event flags
		flags = sp->beat[beatnum]->flags & EOF_BEAT_FLAG_ANCHOR;
		flags |= sp->beat[beatnum]->flags & EOF_BEAT_FLAG_EVENTS;

		if(undo)	//If calling function specified to make an undo state
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make an undo state

		if((num == 3) && (den == 4))
		{
			sp->beat[beatnum]->flags = flags | EOF_BEAT_FLAG_START_3_4;
		}
		else if((num == 4) && (den == 4))
		{
			sp->beat[beatnum]->flags = flags | EOF_BEAT_FLAG_START_4_4;
		}
		else if((num == 5) && (den == 4))
		{
			sp->beat[beatnum]->flags = flags | EOF_BEAT_FLAG_START_5_4;
		}
		else if((num == 6) && (den == 4))
		{
			sp->beat[beatnum]->flags = flags | EOF_BEAT_FLAG_START_6_4;
		}
		else
		{
			num--;	//Convert these numbers to a system where all bits zero represents 1 and all bits set represents 256
			den--;
			sp->beat[beatnum]->flags = flags | EOF_BEAT_FLAG_CUSTOM_TS;
			sp->beat[beatnum]->flags |= (num << 24);
			sp->beat[beatnum]->flags |= (den << 16);
		}
	}

	return 1;
}

int eof_dump_midi_track(const char *inputfile,PACKFILE *outf)
{
	eof_log("eof_dump_midi_track() entered", 1);

	unsigned long track_length;
	PACKFILE *inf = NULL;
	char trackheader[8] = {'M', 'T', 'r', 'k', 0, 0, 0, 0};
	unsigned long i;

	if((inputfile == NULL) || (outf == NULL))
	{
		return 0;	//Return failure
	}
	track_length = file_size_ex(inputfile);
	inf = pack_fopen(inputfile, "r");	//Open input file for reading
	if(!inf)
	{
		return 0;	//Return failure
	}

	pack_fwrite(trackheader, 4, outf);	//Write the output track header
	pack_mputl(track_length, outf);		//Write the output track length
	for(i = 0; i < track_length; i++)
	{	//For each byte in the input file
		pack_putc(pack_getc(inf), outf);	//Copy the byte to the output file
	}
	pack_fclose(inf);

	return 1;	//Return success
}

void eof_write_text_event(unsigned long deltas, const char *str, PACKFILE *fp)
{
	unsigned long length;
	if(str && fp)
	{
		length = ustrlen(str);
		if(length < 128)
		{	//EOF's implementation is a bit lax and only accounts for 1 byte variable length values (7 bits)
			WriteVarLen(deltas, fp);
			pack_putc(0xFF, fp);	//Text event
			pack_putc(0x01, fp);
			pack_putc(length, fp);
			pack_fwrite(str, length, fp);
		}
	}
}
