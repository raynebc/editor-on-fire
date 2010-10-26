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

#define EOF_MIDI_TIMER_FREQUENCY  40

static EOF_MIDI_EVENT * eof_midi_event[EOF_MAX_MIDI_EVENTS];
static int eof_midi_events = 0;
static char eof_midi_note_status[128] = {0};	//Tracks the on/off status of notes 0 through 127, maintained by eof_add_midi_event()

void eof_add_midi_event(unsigned long pos, int type, int note)
{
	eof_midi_event[eof_midi_events] = malloc(sizeof(EOF_MIDI_EVENT));
	if(eof_midi_event[eof_midi_events])
	{
		eof_midi_event[eof_midi_events]->pos = pos;
		eof_midi_event[eof_midi_events]->type = type;
		eof_midi_event[eof_midi_events]->note = note;
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

void eof_add_midi_lyric_event(int pos, char * text)
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

void eof_clear_midi_events(void)
{
	int i;
	for(i = 0; i < eof_midi_events; i++)
	{
		free(eof_midi_event[i]);
	}
	eof_midi_events = 0;
}

void WriteVarLen(unsigned long value, PACKFILE * fp)
{
	unsigned long buffer;
	buffer = value & 0x7F;

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
	unsigned long value;
	unsigned char c;

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
		if((*thing1)->note == 103)
			return -1;	//Solo on written first
		if((*thing2)->note == 103)
			return 1;	//Solo on written first
		if((*thing1)->note == 105)
			return -1;	//Lyric phrase on written second
		if((*thing2)->note == 105)
			return 1;	//Lyric phrase on written second
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

    // they are equal...
    return 0;
}

/* sorter for MIDI events so I can ouput proper MTrk data */
int qsort_helper4(const void * e1, const void * e2)
{
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

int eof_figure_beat(double pos)
{
	int i;

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
	int beat = eof_figure_beat(pos);
	if(beat >= 0)
	{
		return (double)60000000.0 / (double)eof_song->beat[beat]->ppqn;
	}
	return 0.0;
}

int eof_check_bpm_change(unsigned long start, unsigned long end)
{
	int startbeat = eof_figure_beat(start);
	int endbeat = eof_figure_beat(end);
//	unsigned long startbpm = sp->beat[startbeat].ppqn;
	int i;

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
	int i;
	int startbeat = eof_figure_beat(start);
	int endbeat = eof_figure_beat(end);
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

int eof_count_tracks(void)
{
	int i;
	int count = 0;

	for(i = 0; i < EOF_MAX_TRACKS; i++)
	{
		if(eof_song->track[i]->notes > 0)
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
	char header[14] = {'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 1, 0, 1, (EOF_DEFAULT_TIME_DIVISION >> 8), (EOF_DEFAULT_TIME_DIVISION & 0xFF)}; //The last two bytes are the time division
	char trackheader[8] = {'M', 'T', 'r', 'k', 0, 0, 0, 0};
	char * tempname[8] = {"eof.tmp", "eof2.tmp", "eof3.tmp", "eof4.tmp", "eof5.tmp", "eof6.tmp", "eof7.tmp", "eof8.tmp"};
	char expertplustempname[] = {"eof9.tmp"};	//Stores the temporary filename for the Expert+ track data
	char expertplusfilename[1024] = {0};
	PACKFILE * fp;
	PACKFILE * fp2;
	PACKFILE * fp3 = NULL;					//File pointer for the Expert+ file
	int i, j;
	unsigned long ctr;
	unsigned long delta = 0;
	unsigned long track_length;
	int midi_note_offset = 0;
	int vel=0x64;	//Velocity

	unsigned long ppqn=0;					//Used to store conversion of BPM to ppqn
	struct Tempo_change *anchorlist=NULL;	//Linked list containing tempo changes
	struct Tempo_change *ptr=NULL;			//Conductor for the anchor linked list
	unsigned long lastdelta=0;				//Keeps track of the last anchor's absolute delta time
	char * tempstring = NULL;				//Used to store a copy of the lyric string into eof_midi_event[], so the string can be modified from the original
	char correctlyrics = 0;					//If nonzero, logic will be performed to correct the pitchless lyrics to have a pound character and have a generic pitch note
	char correctphrases = 0;				//If nonzero, logic will be performed to add missing lyric phrases to ensure all lyrics (except vocal percussion notes) are encompassed within a lyric phrase
	unsigned long length;					//Used to cap drum notes
	char prodrums = 0;						//Tracks whether the drum track being written includes Pro drum notation
	char expertplus = 0;					//Tracks whether an expert+.mid track should be created to hold the Expert+ drum track
	char expertpluswritten = 0;				//Tracks whether an expert+.mid track has been written
	char trackctr;							//Used in the temp data creation to handle Expert+
	EOF_MIDI_TS_LIST *tslist=NULL;			//List containing TS changes

	anchorlist=eof_build_tempo_list();	//Create a linked list of all tempo changes in eof_song->beat[]
	if(anchorlist == NULL)	//If the anchor list could not be created
		return 0;	//Return failure

	if(eof_use_midi_ts)
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

	eof_sort_notes();	//Writing efficient on-the-fly HOPO phrasing relies on all notes being sorted

	for(j = 0; j < EOF_MAX_TRACKS; j++)
	{
		/* fill in notes */
		if(sp->track[j]->notes > 0)
		{
			/* clear MIDI events list */
			eof_clear_midi_events();
			memset(eof_midi_note_status,0,sizeof(eof_midi_note_status));	//Clear note status array

//Detect whether Pro drum notation is being used
//Pro drum notation is that if a green, yellow or blue drum note is NOT to be marked as a cymbal,
//it must be marked with the appropriate MIDI note, otherwise the note defaults as a cymbal
			for(i = 0, prodrums = 0; i < sp->track[j]->notes; i++)
			{
				if(	((sp->track[j]->note[i]->note & 4) && ((sp->track[j]->note[i]->flags & EOF_NOTE_FLAG_Y_CYMBAL))) ||
					((sp->track[j]->note[i]->note & 8) && ((sp->track[j]->note[i]->flags & EOF_NOTE_FLAG_B_CYMBAL))) ||
					((sp->track[j]->note[i]->note & 16) && ((sp->track[j]->note[i]->flags & EOF_NOTE_FLAG_G_CYMBAL))))
				{	//If this note contains a yellow, blue or purple (green in Rock Band) drum marked with pro drum notation
					prodrums = 1;
					break;
				}
			}

			/* write the MTrk MIDI data to a temp file
	   		use size of the file as the MTrk header length */
			for(i = 0; i < sp->track[j]->notes; i++)
			{
				switch(sp->track[j]->note[i]->type)
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

				/* write green note */
				if(sp->track[j]->note[i]->note & 1)
				{
					if((j == EOF_TRACK_DRUM) && (sp->track[j]->note[i]->flags & EOF_NOTE_FLAG_DBASS))
					{	//If the track being written is PART DRUMS, and this note is marked for Expert+ double bass
						eof_add_midi_event(sp->track[j]->note[i]->pos, 0x90, 95);
						expertplus = 1;
					}
					else	//Otherwise write a normal green gem
						eof_add_midi_event(sp->track[j]->note[i]->pos, 0x90, midi_note_offset + 0);
				}

				/* write red note */
				if(sp->track[j]->note[i]->note & 2)
				{
					eof_add_midi_event(sp->track[j]->note[i]->pos, 0x90, midi_note_offset + 1);
				}

				/* write yellow note */
				if(sp->track[j]->note[i]->note & 4)
				{
					if((j == EOF_TRACK_DRUM) && prodrums && !eof_check_flags_at_note_pos(sp->track[j],i,EOF_NOTE_FLAG_Y_CYMBAL))
					{	//If pro drum notation is in effect and no more yellow drum notes at this note's position are marked as cymbals
						if(eof_midi_note_status[RB3_DRUM_YELLOW_FORCE] == 0)
						{	//Write a pro yellow drum marker if one isn't already in effect
							eof_add_midi_event(sp->track[j]->note[i]->pos, 0x90, RB3_DRUM_YELLOW_FORCE);
						}
					}
					eof_add_midi_event(sp->track[j]->note[i]->pos, 0x90, midi_note_offset + 2);
				}

				/* write blue note */
				if(sp->track[j]->note[i]->note & 8)
				{
					if((j == EOF_TRACK_DRUM) && prodrums && !eof_check_flags_at_note_pos(sp->track[j],i,EOF_NOTE_FLAG_B_CYMBAL))
					{	//If pro drum notation is in effect and no more blue drum notes at this note's position are marked as cymbals
						if(eof_midi_note_status[RB3_DRUM_BLUE_FORCE] == 0)
						{	//Write a pro blue drum marker if one isn't already in effect
							eof_add_midi_event(sp->track[j]->note[i]->pos, 0x90, RB3_DRUM_BLUE_FORCE);
						}
					}
					eof_add_midi_event(sp->track[j]->note[i]->pos, 0x90, midi_note_offset + 3);
				}

				/* write purple note */
				if(sp->track[j]->note[i]->note & 16)
				{	//Note: EOF/FoF refer to this note color as purple/orange whereas Rock Band displays it as green
					if((j == EOF_TRACK_DRUM) && prodrums && !eof_check_flags_at_note_pos(sp->track[j],i,EOF_NOTE_FLAG_G_CYMBAL))
					{	//If pro drum notation is in effect and no more green drum notes at this note's position are marked as cymbals
						if(eof_midi_note_status[RB3_DRUM_GREEN_FORCE] == 0)
						{	//Write a pro green drum marker if one isn't already in effect
							eof_add_midi_event(sp->track[j]->note[i]->pos, 0x90, RB3_DRUM_GREEN_FORCE);
						}
					}
					eof_add_midi_event(sp->track[j]->note[i]->pos, 0x90, midi_note_offset + 4);
				}

				/* write forced HOPO */
				if(sp->track[j]->note[i]->flags & EOF_NOTE_FLAG_F_HOPO)
				{
					if(eof_midi_note_status[midi_note_offset + 5] == 0)
					{	//Only write a phrase marker if one isn't already in effect
						eof_add_midi_event(sp->track[j]->note[i]->pos, 0x90, midi_note_offset + 5);
					}
				}

				/* write forced non-HOPO */
				else if(sp->track[j]->note[i]->flags & EOF_NOTE_FLAG_NO_HOPO)
				{
					if(eof_midi_note_status[midi_note_offset + 6] == 0)
					{	//Only write a phrase marker if one isn't already in effect
						eof_add_midi_event(sp->track[j]->note[i]->pos, 0x90, midi_note_offset + 6);
					}
				}

				if((j == EOF_TRACK_DRUM) && (sp->track[j]->note[i]->type != EOF_NOTE_SPECIAL))
				{	//Ensure that drum notes are not written with sustain (Unless they are BRE notes)
					length = 1;
				}
				else
				{
					length = sp->track[j]->note[i]->length;
				}

				/* write green note off */
				if(sp->track[j]->note[i]->note & 1)
				{
					if((j == EOF_TRACK_DRUM) && (sp->track[j]->note[i]->flags & EOF_NOTE_FLAG_DBASS))	//If the track being written is PART DRUMS, and this note is marked for Expert+ double bass
						eof_add_midi_event(sp->track[j]->note[i]->pos + length, 0x80, 95);
					else	//Otherwise end a normal green gem
						eof_add_midi_event(sp->track[j]->note[i]->pos + length, 0x80, midi_note_offset + 0);
				}

				/* write red note off */
				if(sp->track[j]->note[i]->note & 2)
				{
					eof_add_midi_event(sp->track[j]->note[i]->pos + length, 0x80, midi_note_offset + 1);
				}

				/* write yellow note off */
				if(sp->track[j]->note[i]->note & 4)
				{
					if((j == EOF_TRACK_DRUM) && prodrums && !eof_check_flags_at_note_pos(sp->track[j],i,EOF_NOTE_FLAG_Y_CYMBAL))
					{	//If pro drum notation is in effect and no more drum notes at this note's position are marked as cymbals
						if(eof_midi_note_status[RB3_DRUM_YELLOW_FORCE] == 1)
						{	//End a pro yellow drum marker if one is in effect
							eof_add_midi_event(sp->track[j]->note[i]->pos + length, 0x80, RB3_DRUM_YELLOW_FORCE);
						}
					}
					eof_add_midi_event(sp->track[j]->note[i]->pos + length, 0x80, midi_note_offset + 2);
				}

				/* write blue note off */
				if(sp->track[j]->note[i]->note & 8)
				{
					if((j == EOF_TRACK_DRUM) && prodrums && !eof_check_flags_at_note_pos(sp->track[j],i,EOF_NOTE_FLAG_B_CYMBAL))
					{	//If pro drum notation is in effect and no more blue drum notes at this note's position are marked as cymbals
						if(eof_midi_note_status[RB3_DRUM_BLUE_FORCE] == 1)
						{	//End a pro blue drum marker if one is in effect
							eof_add_midi_event(sp->track[j]->note[i]->pos + length, 0x80, RB3_DRUM_BLUE_FORCE);
						}
					}
					eof_add_midi_event(sp->track[j]->note[i]->pos + length, 0x80, midi_note_offset + 3);
				}

				/* write purple note off */
				if(sp->track[j]->note[i]->note & 16)
				{	//Note: EOF/FoF refer to this note color as purple/orange whereas Rock Band displays it as green
					if((j == EOF_TRACK_DRUM) && prodrums && !eof_check_flags_at_note_pos(sp->track[j],i,EOF_NOTE_FLAG_G_CYMBAL))
					{	//If pro drum notation is in effect and no more drum notes at this note's position are marked as cymbals
						if(eof_midi_note_status[RB3_DRUM_GREEN_FORCE] == 1)
						{	//End a pro green drum marker if one is in effect
							eof_add_midi_event(sp->track[j]->note[i]->pos + length, 0x80, RB3_DRUM_GREEN_FORCE);
						}
					}
					eof_add_midi_event(sp->track[j]->note[i]->pos + length, 0x80, midi_note_offset + 4);
				}

				/* write forced HOPO note off */
//				if((i + 1 >= sp->track[j]->notes) || !(sp->track[j]->note[i+1]->flags & EOF_NOTE_FLAG_F_HOPO))	//If this is the last note in the track or if the next note is not a forced HOPO on note
				if(sp->track[j]->note[i]->flags & EOF_NOTE_FLAG_F_HOPO)
				{	//thekiwimaddog indicated that Rock Band uses HOPO phrases per note/chord
					if(eof_midi_note_status[midi_note_offset + 5])
					{	//Only end a phrase marker if one is already in effect
						eof_add_midi_event(sp->track[j]->note[i]->pos + sp->track[j]->note[i]->length, 0x80, midi_note_offset + 5);
					}
				}

				/* write forced non-HOPO note off */
//				else if((i + 1 >= sp->track[j]->notes) || !(sp->track[j]->note[i+1]->flags & EOF_NOTE_FLAG_NO_HOPO))	//If this is the lst note in the track or if the next note is not a forced HOPO off note
				if(sp->track[j]->note[i]->flags & EOF_NOTE_FLAG_NO_HOPO)
				{	//thekiwimaddog indicated that Rock Band uses HOPO phrases per note/chord
					if(eof_midi_note_status[midi_note_offset + 6])
					{	//Only end a phrase marker if one is already in effect
						eof_add_midi_event(sp->track[j]->note[i]->pos + sp->track[j]->note[i]->length, 0x80, midi_note_offset + 6);
					}
				}
			}

			/* fill in star power */
			for(i = 0; i < sp->track[j]->star_power_paths; i++)
			{
				eof_add_midi_event(sp->track[j]->star_power_path[i].start_pos, 0x90, 116);
				eof_add_midi_event(sp->track[j]->star_power_path[i].end_pos, 0x80, 116);
			}

			/* fill in solos */
			for(i = 0; i < sp->track[j]->solos; i++)
			{
				eof_add_midi_event(sp->track[j]->solo[i].start_pos, 0x90, 103);
				eof_add_midi_event(sp->track[j]->solo[i].end_pos, 0x80, 103);
			}

			for(i=0;i < 128;i++)
			{	//Ensure that any notes that are still on are terminated
				if(eof_midi_note_status[i] != 0)	//If this note was left on, write a note off at the end of the last charted note
					eof_add_midi_event(sp->track[j]->note[sp->track[j]->notes-1]->pos + sp->track[j]->note[sp->track[j]->notes-1]->length,0x80,i);
			}
    		qsort(eof_midi_event, eof_midi_events, sizeof(EOF_MIDI_EVENT *), qsort_helper3);
//			allegro_message("break1");

			for(trackctr=0;trackctr<=expertplus;trackctr++)
			{
				/* open the file */
				if(trackctr == 0)	//Writing the normal temp file
					fp = pack_fopen(tempname[j], "w");
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
				pack_putc(0xff, fp);
				pack_putc(0x03, fp);
				WriteVarLen(ustrlen(eof_track_name[j]), fp);
				pack_fwrite(eof_track_name[j], ustrlen(eof_track_name[j]), fp);

				/* add MIDI events */
				lastdelta = 0;
				for(i = 0; i < eof_midi_events; i++)
				{
					if((trackctr == 1) && (eof_midi_event[i]->note < 96))
						continue;	//Filter out all non Expert drum notes for the Expert+ track

					delta = eof_ConvertToDeltaTime(eof_midi_event[i]->pos,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION);

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
		}
	}

/* make vocals track */
	if(sp->vocal_track->lyrics > 0)
	{
		/* clear MIDI events list */
		eof_clear_midi_events();

		/* pre-parse the lyrics to determine if any pitchless lyrics are present */
		correctlyrics = 0;	//By default, pitchless lyrics will not be changed to freestyle during export
		for(ctr = 0; ctr < sp->vocal_track->lyrics; ctr++)
		{
			if(sp->vocal_track->lyric[ctr]->note == 0)
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
		for(ctr = 0; ctr < sp->vocal_track->lyrics; ctr++)
		{
			if((FindLyricLine(ctr) == NULL) && (sp->vocal_track->lyric[ctr]->note != VOCALPERCUSSION))
			{	//If this lyric is not in a line and is not a vocal percussion note
				eof_cursor_visible = 0;
				eof_pen_visible = 0;
				eof_show_mouse(screen);
				if(alert(NULL, "Add phrases for lyrics not in lyric phrases?", NULL, "&Yes", "&No", 'y', 'n') == 1)
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
			for(ctr = 0; ctr < sp->vocal_track->lyrics; ctr++)
			{
				if((FindLyricLine(ctr) == NULL) && (sp->vocal_track->lyric[ctr]->note != VOCALPERCUSSION))
				{	//If this lyric is not in a line and is not a vocal percussion note, write the MIDI events for a line phrase to envelop it
					eof_add_midi_event(eof_ConvertToDeltaTime(sp->vocal_track->lyric[ctr]->pos,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION), 0x90, 105);
					eof_add_midi_event(eof_ConvertToDeltaTime(sp->vocal_track->lyric[ctr]->pos + sp->vocal_track->lyric[ctr]->length,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION), 0x80, 105);

				}
			}
		}

		/* write the MTrk MIDI data to a temp file
		use size of the file as the MTrk header length */
		for(i = 0; i < sp->vocal_track->lyrics; i++)
		{
			//Copy each lyric string into a new array, perform correction on it if necessary
			tempstring = malloc(sizeof(sp->vocal_track->lyric[i]->text));
			if(tempstring == NULL)	//If allocation failed
			{
				eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
				eof_destroy_ts_list(tslist);	//Free memory used by the TS change list
				return 0;			//Return failure
			}
			sp->vocal_track->lyric[i]->text[EOF_MAX_LYRIC_LENGTH] = '\0';	//Guarantee that the lyric string is terminated
			memcpy(tempstring,sp->vocal_track->lyric[i]->text,sizeof(sp->vocal_track->lyric[i]->text));	//Copy to new array

			if(sp->vocal_track->lyric[i]->note > 0)
			{	//For the vocal track, store the converted delta times, to allow for artificial padding for lyric phrase markers
				eof_add_midi_event(eof_ConvertToDeltaTime(sp->vocal_track->lyric[i]->pos,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION), 0x90, sp->vocal_track->lyric[i]->note);
				eof_add_midi_event(eof_ConvertToDeltaTime(sp->vocal_track->lyric[i]->pos + sp->vocal_track->lyric[i]->length,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION), 0x80, sp->vocal_track->lyric[i]->note);
			}
			else if(correctlyrics)
			{	//If performing pitchless lyric correction, write pitch 50 instead to guarantee it is usable as a freestyle lyric
				eof_add_midi_event(eof_ConvertToDeltaTime(sp->vocal_track->lyric[i]->pos,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION), 0x90, 50);
				eof_add_midi_event(eof_ConvertToDeltaTime(sp->vocal_track->lyric[i]->pos + sp->vocal_track->lyric[i]->length,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION), 0x80, 50);
				eof_set_freestyle(tempstring,1);		//Ensure the lyric properly ends with a freestyle character
			}

			//Write the string, which was only corrected if correctlyrics was nonzero and the pitch was not defined
			if(sp->vocal_track->lyric[i]->note != VOCALPERCUSSION)	//Do not write a lyric string for vocal percussion notes
				eof_add_midi_lyric_event(eof_ConvertToDeltaTime(sp->vocal_track->lyric[i]->pos,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION), tempstring);
		}
		/* fill in lyric lines */
		for(i = 0; i < sp->vocal_track->lines; i++)
		{
			eof_add_midi_event(eof_ConvertToDeltaTime(sp->vocal_track->line[i].start_pos,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION), 0x90, 105);
			eof_add_midi_event(eof_ConvertToDeltaTime(sp->vocal_track->line[i].end_pos,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION), 0x80, 105);
			if(sp->vocal_track->line[i].flags & EOF_LYRIC_LINE_FLAG_OVERDRIVE)
			{	//For the vocal track, store the converted delta times, to allow for artificial padding for lyric phrase markers
				eof_add_midi_event(eof_ConvertToDeltaTime(sp->vocal_track->line[i].start_pos,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION), 0x90, 116);
				eof_add_midi_event(eof_ConvertToDeltaTime(sp->vocal_track->line[i].end_pos,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION), 0x80, 116);
			}
		}

		/* insert padding as necessary between a lyric phrase on marker and the following lyric */
		#define EOF_LYRIC_PHRASE_PADDING 5
		unsigned long last_phrase = 0;	//Stores the absolute delta time of the last Note 105 On
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
		}

		qsort(eof_midi_event, eof_midi_events, sizeof(EOF_MIDI_EVENT *), qsort_helper3);

		/* open the file */
		fp = pack_fopen(tempname[7], "w");
		if(!fp)
		{
			eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
			eof_destroy_ts_list(tslist);	//Free memory used by the TS change list
			return 0;
		}

		/* write the track name */
		WriteVarLen(0, fp);
		pack_putc(0xff, fp);
		pack_putc(0x03, fp);
		WriteVarLen(ustrlen(eof_track_name[EOF_TRACK_VOCALS]), fp);
		pack_fwrite(eof_track_name[EOF_TRACK_VOCALS], ustrlen(eof_track_name[EOF_TRACK_VOCALS]), fp);

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
	}


/* make tempo track */
	fp = pack_fopen(tempname[5], "w");
	if(!fp)
	{
		eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
		eof_destroy_ts_list(tslist);	//Free memory used by the TS change list
		return 0;
	}

	lastdelta=0;
	unsigned long current_ts=0;
	unsigned long nextanchorpos=0,next_tspos=0;
	char whattowrite;	//Bitflag: bit 0=write tempo change, bit 1=write TS change
	ptr = anchorlist;
	while((ptr != NULL) || (eof_use_midi_ts && (current_ts < tslist->changes)))
	{	//While there are tempo changes or TS changes (if the user specified to write TS changes) to write
		whattowrite = 0;
		if(ptr != NULL)
		{
			nextanchorpos=ptr->delta;
			whattowrite |= 1;
		}
		if(eof_use_midi_ts && (current_ts < tslist->changes))
		{	//Only process TS changes if the user opted to do so
			next_tspos=tslist->change[current_ts]->pos;
			whattowrite |= 2;
		}

		if(whattowrite > 2)
		{	//If both a TS change and a tempo change remain to be written
			if(nextanchorpos < next_tspos)	//If the tempo change is earlier
				whattowrite = 1;			//write it first
			else if(eof_use_midi_ts)
				whattowrite = 2;			//otherwise write the TS change first
		}

		if(whattowrite == 1)
		{	//If writing a tempo change
			WriteVarLen(ptr->delta - lastdelta, fp);	//Write this anchor's relative delta time
			lastdelta=ptr->delta;						//Store this anchor's absolute delta time

			ppqn = ((double) 60000000.0 / ptr->BPM) + 0.5;	//Convert BPM to ppqn, rounding up
			pack_putc(0xff, fp);					//Write Meta Event 0x51 (Set Tempo)
			pack_putc(0x51, fp);
			pack_putc(0x03, fp);					//Write event length of 3
			pack_putc((ppqn & 0xFF0000) >> 16, fp);	//Write high order byte of ppqn
			pack_putc((ppqn & 0xFF00) >> 8, fp);	//Write middle byte of ppqn
			pack_putc((ppqn & 0xFF), fp);			//Write low order byte of ppqn
			ptr=ptr->next;							//Iterate to next anchor
		}
		else if(eof_use_midi_ts)
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

			pack_putc(0xff, fp);							//Write Meta Event 0x58 (Time Signature)
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
	if(sp->text_events)
	{
		eof_sort_events();

		/* open the file */
		fp = pack_fopen(tempname[6], "w");
		if(!fp)
		{
			eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
			eof_destroy_ts_list(tslist);	//Free memory used by the TS change list
			return 0;
		}

		/* write the track name */
		WriteVarLen(0, fp);
		pack_putc(0xff, fp);
		pack_putc(0x03, fp);
		WriteVarLen(ustrlen("EVENTS"), fp);
		pack_fwrite("EVENTS", ustrlen("EVENTS"), fp);

    		/* add MIDI events */
		lastdelta = 0;
		for(i = 0; i < sp->text_events; i++)
		{
			if(sp->text_event[i]->beat >= sp->beats)
			{	//If the text events are corrupted
				break;	//Don't dereference the beat[] array in a way that could crash
			}
			delta = eof_ConvertToDeltaTime(sp->beat[sp->text_event[i]->beat]->fpos,anchorlist,tslist,EOF_DEFAULT_TIME_DIVISION);

			WriteVarLen(delta - lastdelta, fp);	//Write this note's relative delta time
			lastdelta = delta;					//Store this note's absolute delta time
			pack_putc(0xFF, fp);
			pack_putc(0x01, fp);
			pack_putc(ustrlen(sp->text_event[i]->text), fp);
			pack_fwrite(sp->text_event[i]->text, ustrlen(sp->text_event[i]->text), fp);
		}

		/* end of track */
		WriteVarLen(0, fp);
		pack_putc(0xFF, fp);
		pack_putc(0x2F, fp);
		pack_putc(0x00, fp);

		pack_fclose(fp);
	}

	fp = pack_fopen(fn, "w");
	if(!fp)
	{
		eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
		eof_destroy_ts_list(tslist);	//Free memory used by the TS change list
		return 0;
	}

	/* write header data */
	header[11] = eof_count_tracks() + 1 + (sp->text_events > 0 ? 1 : 0) + (sp->vocal_track->lyrics > 0 ? 1 : 0);
	pack_fwrite(header, 14, fp);

	if(expertpluswritten)
	{
		header[11] = 1 + 1 + (sp->text_events > 0 ? 1 : 0);
		replace_filename(expertplusfilename, fn, "expert+.mid", 1024);
		fp3 = pack_fopen(expertplusfilename, "w");
		if(!fp3)
			expertpluswritten = 0;	//Cancel trying to write Expert+ and save the normal MIDI instead
		else
			pack_fwrite(header, 14, fp3);	//Write the Expert+ file's MIDI header
	}


/* write tempo track */
	track_length = file_size_ex(tempname[5]);
	fp2 = pack_fopen(tempname[5], "r");
	if(!fp2)
	{
		pack_fclose(fp);
		eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
		eof_destroy_ts_list(tslist);	//Free memory used by the TS change list
		return 0;
	}
	pack_fwrite(trackheader, 4, fp);
	pack_mputl(track_length, fp);
	for(i = 0; i < track_length; i++)
	{
		pack_putc(pack_getc(fp2), fp);
	}

	if(expertpluswritten)
	{	//Write the tempo track to the Expert+ MIDI file as well
		pack_fclose(fp2);	//Since Allegro's pack_fseek() function doesn't support backward seeks,
		fp2 = pack_fopen(tempname[5], "r");	//Close and re-open the file
		pack_fwrite(trackheader, 4, fp3);
		pack_mputl(track_length, fp3);
		for(i = 0; i < track_length; i++)
		{
			pack_putc(pack_getc(fp2), fp3);
		}
	}
	pack_fclose(fp2);


/* write text event track if there are any events */
	if(sp->text_events)
	{
		track_length = file_size_ex(tempname[6]);
		fp2 = pack_fopen(tempname[6], "r");
		if(!fp2)
		{
			pack_fclose(fp);
			eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
			eof_destroy_ts_list(tslist);	//Free memory used by the TS change list
			return 0;
		}
		pack_fwrite(trackheader, 4, fp);
		pack_mputl(track_length, fp);
		for(i = 0; i < track_length; i++)
		{
			pack_putc(pack_getc(fp2), fp);
		}

		if(expertpluswritten)
		{	//Write the events track to the Expert+ MIDI file as well
			pack_fclose(fp2);	//Since Allegro's pack_fseek() function doesn't support backward seeks,
			fp2 = pack_fopen(tempname[6], "r");	//Close and re-open the file
			pack_fwrite(trackheader, 4, fp3);
			pack_mputl(track_length, fp3);
			for(i = 0; i < track_length; i++)
			{
				pack_putc(pack_getc(fp2), fp3);
			}
		}
		pack_fclose(fp2);
	}


/* write note tracks */
	for(j = 0; j < EOF_MAX_TRACKS; j++)
	{
		if(eof_song->track[j]->notes)
		{
			track_length = file_size_ex(tempname[j]);
			fp2 = pack_fopen(tempname[j], "r");
			if(!fp2)
			{
				pack_fclose(fp);
				eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
				eof_destroy_ts_list(tslist);	//Free memory used by the TS change list
				return 0;
			}
			pack_fwrite(trackheader, 4, fp);
			pack_mputl(track_length, fp);

			for(i = 0; i < track_length; i++)
			{
				pack_putc(pack_getc(fp2), fp);
			}
			pack_fclose(fp2);
		}
	}

	if(expertpluswritten)
	{	//Write the PART DRUMS track to the Expert+ MIDI file as well
		track_length = file_size_ex(expertplustempname);
		fp2 = pack_fopen(expertplustempname, "r");
		if(fp2)
		{
			pack_fwrite(trackheader, 4, fp3);
			pack_mputl(track_length, fp3);

			for(i = 0; i < track_length; i++)
			{
				pack_putc(pack_getc(fp2), fp3);
			}
			pack_fclose(fp2);
			pack_fclose(fp3);	//Close Expert+ MIDI file
		}
	}


/* write lyric track if there are any lyrics */
	if(sp->vocal_track->lyrics)
	{
		track_length = file_size_ex(tempname[7]);
		fp2 = pack_fopen(tempname[7], "r");
		if(!fp2)
		{
			pack_fclose(fp);
			eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
			eof_destroy_ts_list(tslist);	//Free memory used by the TS change list
			return 0;
		}
		pack_fwrite(trackheader, 4, fp);
		pack_mputl(track_length, fp);
		for(i = 0; i < track_length; i++)
		{
			pack_putc(pack_getc(fp2), fp);
		}
		pack_fclose(fp2);
	}

	pack_fclose(fp);
	eof_clear_midi_events();


/* delete temporary files */
	delete_file("eof.tmp");
	delete_file("eof2.tmp");
	delete_file("eof3.tmp");
	delete_file("eof4.tmp");
	delete_file("eof5.tmp");
	delete_file("eof6.tmp");
	delete_file("eof7.tmp");
	delete_file("eof8.tmp");
	delete_file("eof9.tmp");	//Delete Expert+ temp file
	eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
	eof_destroy_ts_list(tslist);	//Free memory used by the TS change list

	return 1;
}

struct Tempo_change *eof_build_tempo_list(void)
{
	unsigned long ctr;
	struct Tempo_change *list=NULL;	//The linked list
	struct Tempo_change *temp=NULL;
	unsigned long lastppqn=0;	//Tracks the last anchor's PPQN value
	unsigned long deltactr=0;	//Counts the number of deltas between anchors
	unsigned den=4;				//Stores the most recent TS change's denominator (default to 4)

	if((eof_song == NULL) || (eof_song->beats < 1))
		return NULL;

	for(ctr=0;ctr < eof_song->beats;ctr++)
	{	//For each beat
		if(eof_use_midi_ts)
		{	//If the user opted to use time signatures during MIDI export
			eof_get_ts(NULL,&den,ctr);	//Update the TS denominator if applicable
		}
		if(eof_song->beat[ctr]->ppqn != lastppqn)
		{	//If this beat has a different tempo than the last, add it to the list
			if((list == NULL) && (eof_song->beat[ctr]->fpos != 0.0))	//If the first anchor isn't at position 0
				temp=eof_add_to_tempo_list(0,0.0,(double)120.0,list);	//Add a default 120BPM anchor

			lastppqn=eof_song->beat[ctr]->ppqn;	//Remember this ppqn
			temp=eof_add_to_tempo_list(deltactr,eof_song->beat[ctr]->fpos,(double)60000000.0/lastppqn,list);

			if(temp == NULL)
			{
				eof_destroy_tempo_list(list);	//Destroy list
				return NULL;			//Return error
			}
			list=temp;	//Update list pointer
		}


		deltactr+=((double)EOF_DEFAULT_TIME_DIVISION * den / 4.0) + 0.5;	//Add the number of deltas of one beat (scale to convert from deltas per quarternote) to the counter
	}

	return list;
}

struct Tempo_change *eof_add_to_tempo_list(unsigned long delta,double realtime,double BPM,struct Tempo_change *ptr)
{
	struct Tempo_change *temp=NULL;
	struct Tempo_change *cond=NULL;	//A conductor for the linked list

//Allocate and initialize new link
	temp=(struct Tempo_change *)malloc(sizeof(struct Tempo_change));
	if(temp == NULL)
		return NULL;
	temp->delta=delta;
	temp->realtime=realtime;
	temp->BPM=BPM;
	temp->next=NULL;

//Append to linked list
	if(ptr == NULL)		//If the passed list was empty
		return temp;	//Return the new head link

	for(cond=ptr;cond->next != NULL;cond=cond->next);	//Seek to last link in the list

	cond->next=temp;	//Last link points forward to new link
	return ptr;		//Return original head link
}

void eof_destroy_tempo_list(struct Tempo_change *ptr)
{
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
	delta+=(unsigned long)((reltime * (double)timedivision * temp->BPM / 240000.0 * (double)den) + 0.5);

//The old conversion formula that doesn't take time signature into account
//By using NewCreature's formula:	realtime = (delta / divisions) * (60000.0 / bpm)
//The formula for delta is:		delta = realtime * divisions * bpm / 60000
//	delta+=(unsigned long)((temptime * (double)timedivision * temp->BPM / (double)60000.0 + (double)0.5));			//Add .5 so that the delta counter is rounded to the nearest 1

	return delta;
}

int eof_extract_rba_midi(const char * source, const char * dest)
{
	FILE *fp=NULL;
	FILE *tempfile=NULL;
	unsigned long ctr=0;
	int jumpcode = 0;
	char buffer[15]={0};

//Open specified file
	if((source == NULL) || (dest == NULL))
		return 1;	//Return failure
	fp=fopen(source,"rb");
	if(fp == NULL)
		return 1;	//Return failure

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
	EOF_MIDI_TS_LIST * lp;
	lp = malloc(sizeof(EOF_MIDI_TS_LIST));
	if(!lp)
	{
		return NULL;
	}
	lp->changes = 0;
	return lp;
}

void eof_midi_add_ts_deltas(EOF_MIDI_TS_LIST * changes, unsigned long pos, unsigned long num, unsigned long den, unsigned long track)
{
	if((changes->changes < EOF_MAX_TS) && (track == 0))
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
	if((changes->changes < EOF_MAX_TS) && (track == 0))
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
		if(eof_get_ts(&num,&den,ctr) == 1)
		{	//If a time signature exists on this beat
			eof_midi_add_ts_realtime(tslist, eof_song->beat[ctr]->fpos, num, den, 0);	//Store the beat marker's time signature
			tslist->change[tslist->changes-1]->pos = deltapos;	//Store the time signature's position in deltas
		}

		beatlength = ((double)EOF_DEFAULT_TIME_DIVISION * den / 4.0);		//Determine the length of this beat in deltas
		deltafpos += beatlength;	//Add the delta length of this beat to the delta counter
		deltapos = deltafpos + 0.5;	//Round up to nearest delta
	}

	return tslist;
}

int eof_get_ts(unsigned *num,unsigned *den,int beatnum)
{
	unsigned numerator=0,denominator=0;

	if(beatnum >= eof_song->beats)
		return -1;	//Return error

	if(eof_song->beat[beatnum]->flags & EOF_BEAT_FLAG_START_4_4)
	{
		numerator = 4;
		denominator = 4;
	}
	else if(eof_song->beat[beatnum]->flags & EOF_BEAT_FLAG_START_3_4)
	{
		numerator = 3;
		denominator = 4;
	}
	else if(eof_song->beat[beatnum]->flags & EOF_BEAT_FLAG_START_5_4)
	{
		numerator = 5;
		denominator = 4;
	}
	else if(eof_song->beat[beatnum]->flags & EOF_BEAT_FLAG_START_6_4)
	{
		numerator = 6;
		denominator = 4;
	}
	else if(eof_song->beat[beatnum]->flags & EOF_BEAT_FLAG_CUSTOM_TS)
	{
		numerator = ((eof_song->beat[beatnum]->flags & 0xFF000000)>>24) + 1;
		denominator = ((eof_song->beat[beatnum]->flags & 0x00FF0000)>>16) + 1;
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
	int flags = 0;

	if((sp == NULL) || (beatnum >= sp->beats))
		return 0;	//Return error

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
