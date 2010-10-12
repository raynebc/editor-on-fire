#include <allegro.h>
#include "main.h"
#include "utility.h"
#include "song.h"
#include "midi_import.h"
#include "ini_import.h"
#include "menu/note.h"
#include "foflc/Lyric_storage.h"

typedef struct
{

	unsigned long pos;
	unsigned char type;
	int d1, d2, d3, d4;
	unsigned long track;	//The track number this event is from
	char text[EOF_MAX_MIDI_TEXT_SIZE+1];

} EOF_IMPORT_MIDI_EVENT;

typedef struct
{

	EOF_IMPORT_MIDI_EVENT * event[EOF_IMPORT_MAX_EVENTS];
	int events;
	int type;

} EOF_IMPORT_MIDI_EVENT_LIST;

static MIDI * eof_work_midi = NULL;
static EOF_IMPORT_MIDI_EVENT_LIST * eof_import_events[EOF_MAX_IMPORT_MIDI_TRACKS];
static EOF_MIDI_TS_LIST * eof_import_ts_changes[EOF_MAX_IMPORT_MIDI_TRACKS];
static EOF_IMPORT_MIDI_EVENT_LIST * eof_import_bpm_events;
static EOF_IMPORT_MIDI_EVENT_LIST * eof_import_text_events;
double eof_import_bpm_pos[1024] = {0.0};
int eof_import_bpm_count = 0;

static EOF_IMPORT_MIDI_EVENT_LIST * eof_import_create_events_list(void)
{
	EOF_IMPORT_MIDI_EVENT_LIST * lp;
	lp = malloc(sizeof(EOF_IMPORT_MIDI_EVENT_LIST));
	if(!lp)
	{
		return NULL;
	}
	lp->events = 0;
	lp->type = -1;
	return lp;
}

static void eof_import_destroy_events_list(EOF_IMPORT_MIDI_EVENT_LIST * lp)
{
	int i;

	for(i = 0; i < lp->events; i++)
	{
		free(lp->event[i]);
	}
	free(lp);
}

/* parse_var_len:
 *  The MIDI file format is a strange thing. Time offsets are only 32 bits,
 *  yet they are compressed in a weird variable length format. This routine
 *  reads a variable length integer from a MIDI data stream. It returns the
 *  number read, and alters the data pointer according to the number of
 *  bytes it used.
 */
static unsigned long eof_parse_var_len(unsigned char * data, unsigned long pos, unsigned long * bytes_used)
{
	int cpos = pos;
	unsigned long val = *(&data[cpos]) & 0x7F;

	while(data[cpos] & 0x80)
	{
		cpos++;
		(*bytes_used)++;
//		(*data)++;
		val <<= 7;
		val += (data[cpos] & 0x7F);
	}

	(*bytes_used)++;
	return val;
}

static int eof_import_distance(int pos1, int pos2)
{
	int distance;

	if(pos1 > pos2)
	{
		distance = pos1 - pos2;
	}
	else
	{
		distance = pos2 - pos1;
	}
	return distance;
}

static int eof_import_closest_beat(EOF_SONG * sp, unsigned long pos)
{
	int i;
	int bb = -1, ab = -1;

	for(i = 0; i < sp->beats; i++)
	{
		if(sp->beat[i]->pos <= pos)
		{
			bb = i;
		}
	}
	for(i = sp->beats - 1; i >= 0; i--)
	{
		if(sp->beat[i]->pos >= pos)
		{
			ab = i;
		}
	}
	if((bb >= 0) && (ab >= 0))
	{
		if(eof_import_distance(sp->beat[bb]->pos, pos) < eof_import_distance(sp->beat[ab]->pos, pos))
		{
			return bb;
		}
		else
		{
			return ab;
		}
	}
	return -1;
}

static void eof_midi_import_add_event(EOF_IMPORT_MIDI_EVENT_LIST * events, unsigned long pos, unsigned char event, unsigned long d1, unsigned long d2, unsigned long track)
{
	if(events->events < EOF_IMPORT_MAX_EVENTS)
	{
		events->event[events->events] = malloc(sizeof(EOF_IMPORT_MIDI_EVENT));
		if(events->event[events->events])
		{
			events->event[events->events]->pos = pos;
			events->event[events->events]->type = event;
			events->event[events->events]->d1 = d1;
			events->event[events->events]->d2 = d2;
			events->event[events->events]->track = track;	//Store the event's track number

			events->events++;
		}
	}
	else
	{
//		allegro_message("too many events!");
	}
}

static void eof_midi_import_add_text_event(EOF_IMPORT_MIDI_EVENT_LIST * events, unsigned long pos, unsigned char event, char * text, unsigned long size, unsigned long track)
{
	if(events->events < EOF_IMPORT_MAX_EVENTS)
	{
		if(size > EOF_MAX_MIDI_TEXT_SIZE)	//Prevent a buffer overflow by truncating the string if necessary
			size = EOF_MAX_MIDI_TEXT_SIZE;

		events->event[events->events] = malloc(sizeof(EOF_IMPORT_MIDI_EVENT));
		if(events->event[events->events])
		{
			events->event[events->events]->pos = pos;
			events->event[events->events]->type = event;
			memcpy(events->event[events->events]->text, text, size);
			events->event[events->events]->text[size] = '\0';
			events->event[events->events]->track = track;	//Store the event's track number

			events->events++;
		}
	}
	else
	{
//		allegro_message("too many events!");
	}
}

double eof_ConvertToRealTime(unsigned long absolutedelta,struct Tempo_change *anchorlist,EOF_MIDI_TS_LIST *tslist,unsigned long timedivision,unsigned long offset)
{
	struct Tempo_change *temp=anchorlist;	//Point to first link in list
	double temptimer=0.0;	//Will be used to seek to appropriate beginning tempo change
	unsigned long tempdelta=0;
	double tempBPM=0.0;			//Stores BPM of current tempo change
	unsigned int den=0;			//Stores the denominator of the current time signature
	unsigned long ctr=0;

//Find the last time signature change before the target delta time
	if((tslist == NULL) || (tslist->changes == 0))
		den=4;				//As per MIDI specification, default to a time signature of 4/4 if no TS changes are present
	else
	{
		for(ctr=0;ctr < tslist->changes;ctr++)
		{
			if(absolutedelta >= tslist->change[ctr]->pos)	//If the delta time is enough to reach next tempo change
				den = tslist->change[ctr]->den;		//Store this time signature's denominator for use in the conversion
		}
	}

//Find the last tempo change before the target delta time
	if(temp == NULL)
	{
		tempdelta=absolutedelta;
		tempBPM=120.0;			//Default to a tempo of 120BPM if no tempo changes are present
	}
	else
	{
		temptimer=temp->realtime;	//Start with the timestamp of the starting tempo change
		tempBPM=temp->BPM;

	//Find the real time of the specified delta time, which is relative from the defined starting timestamp
		while((temp->next != NULL))
		{		//Traverse tempo changes until absolutedelta is insufficient to reach the next tempo change
				//or until there are no further tempo changes
			if(absolutedelta >= (temp->next)->delta)	//If the delta time is enough to reach next tempo change
			{
				temp=temp->next;						//Advance to next tempo change
				temptimer=temp->realtime;				//Set timer
				tempBPM=temp->BPM;						//Set BPM
			}
			else
				break;	//break from loop
		}
		tempdelta=absolutedelta-temp->delta;
	}

//At this point, we have reached the tempo change that absolutedelta resides within, find the realtime
//The updated theoretical conversion formula that takes the time signature into account is: deltas / (time division) * (15000.0 / BPM) * (TS denominator)
	temptimer+=(double)tempdelta / (double)timedivision * ((double)15000.0 / tempBPM) * den;

//The old conversion formula that doesn't take time signature into account
//	temptimer+=(double)tempdelta / (double)timedivision * ((double)60000.0 / tempBPM);
	return temptimer+offset;
}

inline unsigned long eof_ConvertToRealTimeInt(unsigned long absolutedelta,struct Tempo_change *anchorlist,EOF_MIDI_TS_LIST *tslist,unsigned long timedivision,unsigned long offset)
{
	return eof_ConvertToRealTime(absolutedelta,anchorlist,tslist,timedivision,offset) + 0.5;
}

EOF_SONG * eof_import_midi(const char * fn)
{
	EOF_SONG * sp;
	int pticker = 0;
	int ptotal_events = 0;
	int percent;
	int i, j, k;
	int rbg = 0;
	int tracks = 0;
	int track[EOF_MAX_IMPORT_MIDI_TRACKS] = {0};
	int track_pos;
	unsigned long delta;
	unsigned long absolute_pos;
	unsigned long last_delta_time=0;	//Will store the absolute delta time of the last parsed MIDI event
	unsigned long bytes_used;
	unsigned char current_event;
	unsigned char current_event_hi = 0;
	unsigned char last_event = 0;
	unsigned char current_meta_event;
	unsigned long d1=0, d2=0, d3, d4;
	char text[EOF_MAX_MIDI_TEXT_SIZE+1];
	char nfn[1024] = {0};
	char backup_filename[1024] = {0};
	char ttit[256] = {0};

	/* load MIDI */
	eof_work_midi = load_midi(fn);
	if(!eof_work_midi)
	{
		return 0;
	}

	/* backup "notes.mid" if it exists in the folder with the imported MIDI
	   as it will be overwritten upon save */
	replace_filename(eof_temp_filename, fn, "notes.mid", 1024);
	if(exists(eof_temp_filename))
	{
		/* do not overwrite an existing backup, this prevents the original backed up MIDI from
		   being overwritten if the user imports the MIDI again */
		replace_filename(backup_filename, fn, "notes.mid.backup", 1024);
		if(!exists(backup_filename))
		{
			eof_copy_file(eof_temp_filename, backup_filename);
		}
	}

	sp = eof_create_song();
	if(!sp)
	{
		destroy_midi(eof_work_midi);
		return NULL;
	}

	/* read INI file */
	replace_filename(backup_filename, fn, "song.ini", 1024);
	eof_import_ini(sp, backup_filename);


	/* parse MIDI data */
	for(i = 0; i < EOF_MAX_IMPORT_MIDI_TRACKS; i++)
	{	//Note each of the first EOF_MAX_IMPORT_MIDI_TRACKS number of tracks that have MIDI events
		if(eof_work_midi->track[i].data)
		{
			track[tracks] = i;
			tracks++;
		}
	}

	/* first pass, build events list for each track */
	eof_import_bpm_events = eof_import_create_events_list();
	if(!eof_import_bpm_events)
	{
		destroy_midi(eof_work_midi);
		return 0;
	}
	eof_import_text_events = eof_import_create_events_list();
	if(!eof_import_text_events)
	{
		eof_import_destroy_events_list(eof_import_bpm_events);
		destroy_midi(eof_work_midi);
		return 0;
	}
	for(i = 0; i < tracks; i++)
	{
		last_event = 0;	//Running status resets at beginning of each track
		eof_import_events[i] = eof_import_create_events_list();
		eof_import_ts_changes[i] = eof_create_ts_list();
		if(!eof_import_events[i])
		{
			eof_import_destroy_events_list(eof_import_bpm_events);
			eof_import_destroy_events_list(eof_import_text_events);
			destroy_midi(eof_work_midi);
			return 0;
		}
		track_pos = 0;
//		absolute_pos = sp->tags->ogg[0].midi_offset;
		absolute_pos = 0;
		while(track_pos < eof_work_midi->track[track[i]].len)
		{
			/* read delta */
			bytes_used = 0;
			delta = eof_parse_var_len(eof_work_midi->track[track[i]].data, track_pos, &bytes_used);
			absolute_pos += delta;
			if(absolute_pos > last_delta_time)
				last_delta_time = absolute_pos;	//Remember the delta position of the latest event in the MIDI (among all tracks)
			track_pos += bytes_used;

			/* read event type */
			if((current_event_hi >= 0x80) && (current_event_hi < 0xF0))
			{	//If the last loop iteration's event was normal
				last_event = current_event_hi;	//Store it
			}
			current_event = eof_work_midi->track[track[i]].data[track_pos];
			current_event_hi = current_event & 0xF0;

			if(current_event_hi < 0x80)	//If this event is a running status event
			{
				current_event_hi = last_event;	//Recall the previous normal event
			}
			else
			{
				track_pos++;	//Increment buffer pointer past the status byte
			}

			ptotal_events++;	//Any event that is parsed should increment this counter
			if(current_event_hi < 0xF0)
			{	//If it's not a Meta event, assume that two parameters are to be read for the event (this will be undone for Program Change and Channel Aftertouch, which don't have a second parameter)
				d1 = eof_work_midi->track[track[i]].data[track_pos++];
				d2 = eof_work_midi->track[track[i]].data[track_pos++];
			}
			switch(current_event_hi)
			{

				/* note off */
				case 0x80:
				{
					eof_midi_import_add_event(eof_import_events[i], absolute_pos, 0x80, d1, d2, i);
					break;
				}

				/* note on */
				case 0x90:
				{
					if(d2 <= 0)
					{	//Any Note On event with a velocity of 0 is to be treated as a Note Off event
						eof_midi_import_add_event(eof_import_events[i], absolute_pos, 0x80, d1, d2, i);
					}
					else
					{
						eof_midi_import_add_event(eof_import_events[i], absolute_pos, 0x90, d1, d2, i);
					}
					break;
				}

				/* key aftertouch */
				case 0xA0:
				{
					break;
				}

				/* controller change */
				case 0xB0:
				{
					break;
				}

				/* program change */
				case 0xC0:
				{
					track_pos--;	//Rewind one byte, there is no second parameter
					break;
				}

				/* channel aftertouch */
				case 0xD0:
				{
					track_pos--;	//Rewind one byte, there is no second parameter
					break;
				}

				/* pitch wheel change */
				case 0xE0:
				{
					break;
				}

				/* meta event */
				case 0xF0:
				{
					if((current_event == 0xF0) || (current_event == 0xF7))
					{	//If it's a Sysex event
						bytes_used = 0;
						d3 = eof_parse_var_len(eof_work_midi->track[track[i]].data, track_pos, &bytes_used);
						track_pos += bytes_used;
						track_pos += d3;
					}
					else if(current_event == 0xFF)
					{
						current_meta_event = eof_work_midi->track[track[i]].data[track_pos];
						track_pos++;
						if(current_meta_event != 0x51)
						{
						}
						switch(current_meta_event)
						{

							/* set sequence number */
							case 0x00:
							{
								track_pos += 3;
								break;
							}

							/* text event */
							case 0x01:
							{
								for(j = 0; j < eof_work_midi->track[track[i]].data[track_pos]; j++)
								{
									if(j < EOF_MAX_MIDI_TEXT_SIZE)			//If this wouldn't overflow the buffer
										text[j] = eof_work_midi->track[track[i]].data[track_pos + 1 + j];
									else
										break;
								}
								if(j >= EOF_MAX_MIDI_TEXT_SIZE)	//If the string needs to be truncated
									text[EOF_MAX_MIDI_TEXT_SIZE] = '\0';
								eof_midi_import_add_text_event(eof_import_text_events, absolute_pos, 0x01, text, eof_work_midi->track[track[i]].data[track_pos], i);
								track_pos += eof_work_midi->track[track[i]].data[track_pos] + 1;
								break;
							}

							/* copyright */
							case 0x02:
							{
								track_pos += eof_work_midi->track[track[i]].data[track_pos] + 1;
								break;
							}

							/* track name */
							case 0x03:
							{
								d3 = eof_work_midi->track[track[i]].data[track_pos];
								for(j = 0; j < d3; j++)
								{
									track_pos++;
									if(j < EOF_MAX_MIDI_TEXT_SIZE)			//If this wouldn't overflow the buffer
										text[j] = eof_work_midi->track[track[i]].data[track_pos];
								}
								if(j <= EOF_MAX_MIDI_TEXT_SIZE)				//If the string fit in the buffer
									text[j] = '\0';							//Terminate the string normally
								else
									text[EOF_MAX_MIDI_TEXT_SIZE] = '\0';	//Otherwise truncate it
								track_pos++;

								/* detect what kind of track this is */
								if(!ustricmp(text, "PART GUITAR"))
								{
									eof_import_events[i]->type = EOF_TRACK_GUITAR;
									rbg = 1;
								}
								else if(!ustricmp(text, "PART BASS"))
								{
									eof_import_events[i]->type = EOF_TRACK_BASS;
								}
								else if(!ustricmp(text, "PART GUITAR COOP"))
								{
									eof_import_events[i]->type = EOF_TRACK_GUITAR_COOP;
								}
								else if(!ustricmp(text, "PART RHYTHM"))
								{
									eof_import_events[i]->type = EOF_TRACK_RHYTHM;
								}
								else if(!ustricmp(text, "PART DRUM") || !ustricmp(text, "PART DRUMS"))
								{
									eof_import_events[i]->type = EOF_TRACK_DRUM;
								}
								else if(!ustricmp(text, "PART VOCALS"))
								{
									eof_import_events[i]->type = EOF_TRACK_VOCALS;
								}
								else
								{
								}
								break;
							}

							/* instrument name */
							case 0x04:
							{
								track_pos += eof_work_midi->track[track[i]].data[track_pos] + 1;
								break;
							}

							/* lyric */
							case 0x05:
							{
								for(j = 0; j < eof_work_midi->track[track[i]].data[track_pos]; j++)
								{
									if(j < EOF_MAX_MIDI_TEXT_SIZE)			//If this wouldn't overflow the buffer
										text[j] = eof_work_midi->track[track[i]].data[track_pos + 1 + j];
									else
										break;
								}
								if(j >= EOF_MAX_MIDI_TEXT_SIZE)	//If the string needs to be truncated
									text[EOF_MAX_MIDI_TEXT_SIZE] = '\0';
								eof_midi_import_add_text_event(eof_import_events[i], absolute_pos, 0x05, text, eof_work_midi->track[track[i]].data[track_pos], i);
								track_pos += eof_work_midi->track[track[i]].data[track_pos] + 1;
								break;
							}

							/* marker */
							case 0x06:
							{
								track_pos += eof_work_midi->track[track[i]].data[track_pos] + 1;
								break;
							}

							/* cue point */
							case 0x07:
							{
								track_pos++;
								track_pos += eof_work_midi->track[track[i]].data[track_pos] + 1;
								break;
							}

							/* MIDI channel prefix */
							case 0x20:
							{
								track_pos += 2;
								break;
							}

							/* end of track */
							case 0x2F:
							{
								track_pos += 1;
								break;
							}

							/* set tempo */
							case 0x51:
							{
								track_pos += 1;
								d1 = (eof_work_midi->track[track[i]].data[track_pos]);
								track_pos++;
								d2 = (eof_work_midi->track[track[i]].data[track_pos]);
								track_pos++;
								d3 = (eof_work_midi->track[track[i]].data[track_pos]);
								d4 = (d1 << 16) | (d2 << 8) | (d3);

								if((eof_import_bpm_events->events <= 0) && (absolute_pos > sp->tags->ogg[0].midi_offset))
								{	//If the first explicit Set Tempo event is not at the beginning of the track
//									eof_midi_import_add_event(eof_import_bpm_events, sp->tags->ogg[0].midi_offset, 0x51, 500000, 0);	//Insert the default tempo of 120BPM at the beginning of the tempo list
									eof_midi_import_add_event(eof_import_bpm_events, 0, 0x51, 500000, 0, i);	//Insert the default tempo of 120BPM at the beginning of the tempo list
								}
								eof_midi_import_add_event(eof_import_bpm_events, absolute_pos, 0x51, d4, 0, i);
								track_pos++;
								break;
							}

							/* time signature */
							case 0x58:
							{
								unsigned long ctr,realden;

//								track_pos += 5;
								track_pos++;
								d1 = (eof_work_midi->track[track[i]].data[track_pos++]);	//Numerator
								d2 = (eof_work_midi->track[track[i]].data[track_pos++]);	//Denominator
								d3 = (eof_work_midi->track[track[i]].data[track_pos++]);	//Metronome
								d4 = (eof_work_midi->track[track[i]].data[track_pos++]);	//32nds

								for(ctr=0,realden=1;ctr<d2;ctr++)
								{	//Find 2^(d2)
									realden = realden << 1;
								}

								eof_midi_add_ts_deltas(eof_import_ts_changes[i],absolute_pos,d1,realden,i);
								break;
							}

							/* key signature */
							case 0x59:
							{
								track_pos += 3;
								break;
							}

							/* sequencer info */
							case 0x7F:
							{
								track_pos += eof_work_midi->track[track[i]].data[track_pos] + 1;
								break;
							}

							default:
							{
								ptotal_events--;	//This was not a valid MIDI Meta event, undo the event counter increment
								track_pos++;
								break;
							}
						}
					}
					break;
				}

				default:
				{
					ptotal_events--;	//This was not a valid MIDI event, undo the event counter increment
					track_pos--;		//Rewind one of the two bytes that were seeked
					break;
				}
			}
		}
	}

struct Tempo_change *anchorlist=NULL;	//Anchor linked list

	/* second pass, create tempo map */
	unsigned long deltapos = 0;		//Stores the ongoing delta time
	double deltafpos = 0.0;			//Stores the ongoing delta time (with double floating precision)
//	double realtimepos = sp->tags->ogg[0].midi_offset;	//Stores the ongoing real time (the first beat marker will be at the MIDI delay position)
	double realtimepos = 0.0;		//Stores the ongoing real time (start at 0s, displace by the MIDI delay where appropriate)
	unsigned lastnum=0,lastden=0;		//Stores the last applied time signature details
	unsigned curnum=4,curden=4;		//Stores the current time signature details (default is 4/4)
	unsigned long lastppqn=0;		//Stores the last applied tempo information
	unsigned long curppqn=500000;		//Stores the current tempo in PPQN (default is 120BPM)

	unsigned long ctr;
	double beatlength;

	while(deltapos <= last_delta_time)
	{//Add new beats until enough have been added to encompass the last MIDI event
		if(eof_song_add_beat(sp) == NULL)	//Add a new beat
		{					//Or return failure if that doesn't succeed
			eof_import_destroy_events_list(eof_import_bpm_events);
			eof_import_destroy_events_list(eof_import_text_events);
			destroy_midi(eof_work_midi);
			eof_destroy_tempo_list(anchorlist);
			return 0;
		}

	//Find the relevant tempo and time signature for the beat
		for(ctr = 0; ctr < eof_import_bpm_events->events; ctr++)
		{	//For each imported tempo change
			if(eof_import_bpm_events->event[ctr]->pos <= deltapos)
			{	//If the tempo change is at or before the current delta time
				curppqn = eof_import_bpm_events->event[ctr]->d1;	//Store the PPQN value
			}
		}
		for(ctr = 0; ctr < eof_import_ts_changes[0]->changes; ctr++)
		{	//For each imported TS change
			if(eof_import_ts_changes[0]->change[ctr]->pos <= deltapos)
			{	//If the TS change is at or before the current delta time
				curnum = eof_import_ts_changes[0]->change[ctr]->num;	//Store the numerator and denominator
				curden = eof_import_ts_changes[0]->change[ctr]->den;
			}
		}

	//Store timing information in the beat structure
		sp->beat[sp->beats - 1]->fpos = realtimepos + sp->tags->ogg[0].midi_offset;
		sp->beat[sp->beats - 1]->pos = realtimepos + sp->tags->ogg[0].midi_offset + 0.5;	//Round up to nearest millisecond
		sp->beat[sp->beats - 1]->midi_pos = deltapos;
		sp->beat[sp->beats - 1]->ppqn = curppqn;
		if((lastnum != curnum) || (lastden != curden))
		{	//If this time signature is different than  the last beat's time signature
			eof_apply_ts(curnum,curden,sp->beats - 1,sp,0);	//Set the TS flags for this beat
			lastnum = curnum;
			lastden = curden;
		}

	//Update anchor linked list
		if(lastppqn != curppqn)
		{	//If this tempo is different than the last beat's tempo
			anchorlist=eof_add_to_tempo_list(deltapos,realtimepos,60000000.0 / curppqn,anchorlist);
			sp->beat[sp->beats - 1]->flags |= EOF_BEAT_FLAG_ANCHOR;
			lastppqn = curppqn;
		}

	//Update delta and realtime counters
		beatlength = ((double)eof_work_midi->divisions * curden / 4.0);		//Determine the length of this beat in delta ticks
		realtimepos += beatlength / eof_work_midi->divisions * (15000.0 / (60000000.0 / curppqn)) * curden;	//Add the realtime length of this beat to the time counter
		deltafpos += beatlength;	//Add the delta length of this beat to the delta counter
		deltapos = deltafpos + 0.5;	//Round up to nearest delta tick
	}

	/* third pass, create EOF notes */
	int picked_track;

	/* for Rock Band songs, we need to ignore certain tracks */
	char used_track[EOF_MAX_IMPORT_MIDI_TRACKS] = {0};

	unsigned char diff = 0;
	unsigned char diff_chart[5] = {1, 2, 4, 8, 16};
	int note_count[EOF_MAX_IMPORT_MIDI_TRACKS] = {0};
	int first_note;
	unsigned long hopopos[4];
	char hopotype[4];
	int hopodiff;
	for(i = 0; i < tracks; i++)
	{

		picked_track = eof_import_events[i]->type >= 0 ? eof_import_events[i]->type : rbg == 0 ? EOF_TRACK_GUITAR : -1;
		first_note = note_count[picked_track];
		if((picked_track >= 0) && !used_track[picked_track])
		{
			if(picked_track == EOF_TRACK_VOCALS)
			{
				int last_105 = 0;
				int last_106 = 0;
				int overdrive_pos = -1;
				for(j = 0; j < eof_import_events[i]->events; j++)
				{
					if(key[KEY_ESC])
					{
						/* clean up and return */
						eof_import_destroy_events_list(eof_import_bpm_events);
						eof_import_destroy_events_list(eof_import_text_events);
						eof_destroy_tempo_list(anchorlist);
						for(i = 0; i < tracks; i++)
						{
							eof_import_destroy_events_list(eof_import_events[i]);
						}
						destroy_midi(eof_work_midi);
						eof_destroy_song(sp);
						set_window_title("EOF - No Song");
						return NULL;
					}
					if(pticker % 200 == 0)
					{
						percent = (pticker * 100) / ptotal_events;
						sprintf(ttit, "MIDI Import (%d%%)", percent <= 100 ? percent : 100);
						set_window_title(ttit);
					}

					eof_vocal_track_resize(sp->vocal_track, note_count[picked_track] + 1);
					/* note on */
					if(eof_import_events[i]->event[j]->type == 0x90)
					{
						/* lyric line indicator */
						if(eof_import_events[i]->event[j]->d1 == 105)
						{
							sp->vocal_track->line[sp->vocal_track->lines].start_pos = eof_ConvertToRealTimeInt(eof_import_events[i]->event[j]->pos,anchorlist,eof_import_ts_changes[0],eof_work_midi->divisions,sp->tags->ogg[0].midi_offset);
							sp->vocal_track->line[sp->vocal_track->lines].flags=0;	//Init flags for this line as 0
							last_105 = sp->vocal_track->lines;
//							sp->vocal_track->lines++;
						}
						else if(eof_import_events[i]->event[j]->d1 == 106)
						{
							sp->vocal_track->line[sp->vocal_track->lines].start_pos = eof_ConvertToRealTimeInt(eof_import_events[i]->event[j]->pos,anchorlist,eof_import_ts_changes[0],eof_work_midi->divisions,sp->tags->ogg[0].midi_offset);
							sp->vocal_track->line[sp->vocal_track->lines].flags=0;	//Init flags for this line as 0
							last_106 = sp->vocal_track->lines;
//							sp->vocal_track->lines++;
						}
						/* overdrive */
						else if(eof_import_events[i]->event[j]->d1 == 116)
						{
							overdrive_pos = eof_ConvertToRealTimeInt(eof_import_events[i]->event[j]->pos,anchorlist,eof_import_ts_changes[0],eof_work_midi->divisions,sp->tags->ogg[0].midi_offset);
						}
						else if((eof_import_events[i]->event[j]->d1 == 96) || (eof_import_events[i]->event[j]->d1 == 97) || ((eof_import_events[i]->event[j]->d1 >= MINPITCH) && (eof_import_events[i]->event[j]->d1 <= MAXPITCH)))
						{	//If this is a vocal percussion note (96 or 97) or if it is a valid vocal pitch
							for(k = 0; k < note_count[picked_track]; k++)
							{
								if(sp->vocal_track->lyric[k]->pos == eof_ConvertToRealTimeInt(eof_import_events[i]->event[j]->pos,anchorlist,eof_import_ts_changes[0],eof_work_midi->divisions,sp->tags->ogg[0].midi_offset))
								{
									break;
								}
							}
							/* no note associated with this lyric so create a new note */
							if(k == note_count[picked_track])
							{
								sp->vocal_track->lyric[note_count[picked_track]]->note = eof_import_events[i]->event[j]->d1;
								sp->vocal_track->lyric[note_count[picked_track]]->pos = eof_ConvertToRealTimeInt(eof_import_events[i]->event[j]->pos,anchorlist,eof_import_ts_changes[0],eof_work_midi->divisions,sp->tags->ogg[0].midi_offset);
								sp->vocal_track->lyric[note_count[picked_track]]->length = 100;
								note_count[picked_track]++;
							}
							else
							{
								sp->vocal_track->lyric[k]->note = eof_import_events[i]->event[j]->d1;
							}
						}
					}

					/* note off so get length of note */
					else if(eof_import_events[i]->event[j]->type == 0x80)
					{
						/* lyric line indicator */
						if(eof_import_events[i]->event[j]->d1 == 105)
						{
							sp->vocal_track->line[last_105].end_pos = eof_ConvertToRealTimeInt(eof_import_events[i]->event[j]->pos,anchorlist,eof_import_ts_changes[0],eof_work_midi->divisions,sp->tags->ogg[0].midi_offset);
							sp->vocal_track->lines++;
							if(overdrive_pos == sp->vocal_track->line[last_105].start_pos)
							{
								sp->vocal_track->line[last_105].flags |= EOF_LYRIC_LINE_FLAG_OVERDRIVE;
							}
						}
						else if(eof_import_events[i]->event[j]->d1 == 106)
						{
							sp->vocal_track->line[last_106].end_pos = eof_ConvertToRealTimeInt(eof_import_events[i]->event[j]->pos,anchorlist,eof_import_ts_changes[0],eof_work_midi->divisions,sp->tags->ogg[0].midi_offset);
							sp->vocal_track->lines++;
							if(overdrive_pos == sp->vocal_track->line[last_106].start_pos)
							{
								sp->vocal_track->line[last_106].flags |= EOF_LYRIC_LINE_FLAG_OVERDRIVE;
							}
						}
						/* overdrive */
						else if(eof_import_events[i]->event[j]->d1 == 116)
						{
						}
						/* percussion */
						else if((eof_import_events[i]->event[j]->d1 == 96) || (eof_import_events[i]->event[j]->d1 == 97))
						{
						}
						else if((eof_import_events[i]->event[j]->d1 >= MINPITCH) && (eof_import_events[i]->event[j]->d1 <= MAXPITCH))
						{
							if(note_count[picked_track] > 0)
							{
								sp->vocal_track->lyric[note_count[picked_track] - 1]->length = eof_ConvertToRealTimeInt(eof_import_events[i]->event[j]->pos,anchorlist,eof_import_ts_changes[0],eof_work_midi->divisions,sp->tags->ogg[0].midi_offset) - sp->vocal_track->lyric[note_count[picked_track] - 1]->pos;
							}
						}
					}

					/* lyric */
					else if(((eof_import_events[i]->event[j]->type == 0x05) || (eof_import_events[i]->event[j]->type == 0x01)) && (eof_import_events[i]->event[j]->text[0] != '['))
					{
						for(k = 0; k < note_count[picked_track]; k++)
						{
							if(sp->vocal_track->lyric[k]->pos == eof_ConvertToRealTimeInt(eof_import_events[i]->event[j]->pos,anchorlist,eof_import_ts_changes[0],eof_work_midi->divisions,sp->tags->ogg[0].midi_offset))
							{
								break;
							}
						}

						/* no note associated with this lyric so create a new note */
						if(k == note_count[picked_track])
						{
							strcpy(sp->vocal_track->lyric[note_count[picked_track]]->text, eof_import_events[i]->event[j]->text);
							sp->vocal_track->lyric[note_count[picked_track]]->note = 0;
							sp->vocal_track->lyric[note_count[picked_track]]->pos = eof_ConvertToRealTimeInt(eof_import_events[i]->event[j]->pos,anchorlist,eof_import_ts_changes[0],eof_work_midi->divisions,sp->tags->ogg[0].midi_offset);
							sp->vocal_track->lyric[note_count[picked_track]]->length = 100;
							note_count[picked_track]++;
						}
						else
						{
							strcpy(sp->vocal_track->lyric[k]->text, eof_import_events[i]->event[j]->text);
						}
					}
					pticker++;
				}
				eof_vocal_track_resize(sp->vocal_track, note_count[picked_track]);
				if(sp->vocal_track->lyrics > 0)
				{
					used_track[picked_track] = 1;
				}
			}
			else
			{
				for(j = 0; j < eof_import_events[i]->events; j++)
				{
					if(key[KEY_ESC])
					{
						/* clean up and return */
						eof_import_destroy_events_list(eof_import_bpm_events);
						eof_import_destroy_events_list(eof_import_text_events);
						eof_destroy_tempo_list(anchorlist);
						for(i = 0; i < tracks; i++)
						{
							eof_import_destroy_events_list(eof_import_events[i]);
						}
						destroy_midi(eof_work_midi);
						eof_destroy_song(sp);
						set_window_title("EOF - No Song");
						return NULL;
					}
					if(pticker % 20 == 0)
					{
						sprintf(ttit, "MIDI Import (%d%%)", (pticker * 100) / ptotal_events);
						set_window_title(ttit);
					}

					eof_track_resize(sp->track[picked_track], note_count[picked_track] + 1);
					/* note on */
					if(eof_import_events[i]->event[j]->type == 0x90)
					{
						sp->track[picked_track]->note[note_count[picked_track]]->flags = 0;	//Clear the flag here so that the flag can be set if it's an Expert+ double bass note
						if((eof_import_events[i]->event[j]->d1 >= 0x3C) && (eof_import_events[i]->event[j]->d1 < 0x3C + 6))
						{
							sp->track[picked_track]->note[note_count[picked_track]]->type = EOF_NOTE_SUPAEASY;
							diff = eof_import_events[i]->event[j]->d1 - 0x3C;
						}
						else if((eof_import_events[i]->event[j]->d1 >= 0x48) && (eof_import_events[i]->event[j]->d1 < 0x48 + 6))
						{
							sp->track[picked_track]->note[note_count[picked_track]]->type = EOF_NOTE_EASY;
							diff = eof_import_events[i]->event[j]->d1 - 0x48;
						}
						else if((eof_import_events[i]->event[j]->d1 >= 0x54) && (eof_import_events[i]->event[j]->d1 < 0x54 + 6))
						{
							sp->track[picked_track]->note[note_count[picked_track]]->type = EOF_NOTE_MEDIUM;
							diff = eof_import_events[i]->event[j]->d1 - 0x54;
						}
						else if((eof_import_events[i]->event[j]->d1 >= 0x60) && (eof_import_events[i]->event[j]->d1 < 0x60 + 6))
						{
							sp->track[picked_track]->note[note_count[picked_track]]->type = EOF_NOTE_AMAZING;
							diff = eof_import_events[i]->event[j]->d1 - 0x60;
						}
						else if((eof_import_events[i]->event[j]->d1 >= 120) && (eof_import_events[i]->event[j]->d1 <= 124))
						{
							sp->track[picked_track]->note[note_count[picked_track]]->type = EOF_NOTE_SPECIAL;
							diff = eof_import_events[i]->event[j]->d1 - 120;
						}
						else if((picked_track == EOF_TRACK_DRUM) && (eof_import_events[i]->event[j]->d1 == 95))
						{
							sp->track[picked_track]->note[note_count[picked_track]]->type = EOF_NOTE_AMAZING;
							sp->track[picked_track]->note[note_count[picked_track]]->flags ^= EOF_NOTE_FLAG_DBASS;	//Apply this status flag
							diff = eof_import_events[i]->event[j]->d1 - 0x60 + 1;	//Treat as gem 1 (bass drum)
						}
						else
						{
							sp->track[picked_track]->note[note_count[picked_track]]->type = -1;
						}

						/* store forced HOPO marker, when the note off for this marker occurs, search for note with same
						   position and apply it to that note */
						if(eof_import_events[i]->event[j]->d1 == 0x3C + 5)
						{
							hopopos[0] = eof_ConvertToRealTimeInt(eof_import_events[i]->event[j]->pos,anchorlist,eof_import_ts_changes[0],eof_work_midi->divisions,sp->tags->ogg[0].midi_offset);
							hopotype[0] = 0;
						}
						else if(eof_import_events[i]->event[j]->d1 == 0x48 + 5)
						{
							hopopos[1] = eof_ConvertToRealTimeInt(eof_import_events[i]->event[j]->pos,anchorlist,eof_import_ts_changes[0],eof_work_midi->divisions,sp->tags->ogg[0].midi_offset);
							hopotype[1] = 0;
						}
						else if(eof_import_events[i]->event[j]->d1 == 0x54 + 5)
						{
							hopopos[2] = eof_ConvertToRealTimeInt(eof_import_events[i]->event[j]->pos,anchorlist,eof_import_ts_changes[0],eof_work_midi->divisions,sp->tags->ogg[0].midi_offset);
							hopotype[2] = 0;
						}
						else if(eof_import_events[i]->event[j]->d1 == 0x60 + 5)
						{
							hopopos[3] = eof_ConvertToRealTimeInt(eof_import_events[i]->event[j]->pos,anchorlist,eof_import_ts_changes[0],eof_work_midi->divisions,sp->tags->ogg[0].midi_offset);
							hopotype[3] = 0;
						}
						else if(eof_import_events[i]->event[j]->d1 == 0x3C + 6)
						{
							hopopos[0] = eof_ConvertToRealTimeInt(eof_import_events[i]->event[j]->pos,anchorlist,eof_import_ts_changes[0],eof_work_midi->divisions,sp->tags->ogg[0].midi_offset);
							hopotype[0] = 1;
						}
						else if(eof_import_events[i]->event[j]->d1 == 0x48 + 6)
						{
							hopopos[1] = eof_ConvertToRealTimeInt(eof_import_events[i]->event[j]->pos,anchorlist,eof_import_ts_changes[0],eof_work_midi->divisions,sp->tags->ogg[0].midi_offset);
							hopotype[1] = 1;
						}
						else if(eof_import_events[i]->event[j]->d1 == 0x54 + 6)
						{
							hopopos[2] = eof_ConvertToRealTimeInt(eof_import_events[i]->event[j]->pos,anchorlist,eof_import_ts_changes[0],eof_work_midi->divisions,sp->tags->ogg[0].midi_offset);
							hopotype[2] = 1;
						}
						else if(eof_import_events[i]->event[j]->d1 == 0x60 + 6)
						{
							hopopos[3] = eof_ConvertToRealTimeInt(eof_import_events[i]->event[j]->pos,anchorlist,eof_import_ts_changes[0],eof_work_midi->divisions,sp->tags->ogg[0].midi_offset);
							hopotype[3] = 1;
						}

						/* star power and solos */
						if((eof_import_events[i]->event[j]->d1 == 116) && (sp->track[picked_track]->star_power_paths < EOF_MAX_STAR_POWER))
						{
							sp->track[picked_track]->star_power_path[sp->track[picked_track]->star_power_paths].start_pos = eof_ConvertToRealTimeInt(eof_import_events[i]->event[j]->pos,anchorlist,eof_import_ts_changes[0],eof_work_midi->divisions,sp->tags->ogg[0].midi_offset);
						}
						else if((eof_import_events[i]->event[j]->d1 == 103) && (sp->track[picked_track]->solos < EOF_MAX_SOLOS))
						{
							sp->track[picked_track]->solo[sp->track[picked_track]->solos].start_pos = eof_ConvertToRealTimeInt(eof_import_events[i]->event[j]->pos,anchorlist,eof_import_ts_changes[0],eof_work_midi->divisions,sp->tags->ogg[0].midi_offset);
						}

						if(sp->track[picked_track]->note[note_count[picked_track]]->type != -1)
						{
							for(k = first_note; k < note_count[picked_track]; k++)
							{
								if((sp->track[picked_track]->note[k]->pos == eof_ConvertToRealTimeInt(eof_import_events[i]->event[j]->pos,anchorlist,eof_import_ts_changes[0],eof_work_midi->divisions,sp->tags->ogg[0].midi_offset)) && (sp->track[picked_track]->note[k]->type == sp->track[picked_track]->note[note_count[picked_track]]->type))
								{
									break;
								}
							}
							if(k == note_count[picked_track])
							{
								sp->track[picked_track]->note[note_count[picked_track]]->note = diff_chart[diff];
								sp->track[picked_track]->note[note_count[picked_track]]->pos = eof_ConvertToRealTimeInt(eof_import_events[i]->event[j]->pos,anchorlist,eof_import_ts_changes[0],eof_work_midi->divisions,sp->tags->ogg[0].midi_offset);
								sp->track[picked_track]->note[note_count[picked_track]]->length = 100;
								note_count[picked_track]++;
							}
							else
							{
								sp->track[picked_track]->note[k]->note |= diff_chart[diff];
							}
						}
					}

					/* note off so get length of note */
					else if(eof_import_events[i]->event[j]->type == 0x80)
					{
						if((eof_import_events[i]->event[j]->d1 >= 0x3C) && (eof_import_events[i]->event[j]->d1 < 0x3C + 6))
						{
							sp->track[picked_track]->note[note_count[picked_track]]->type = EOF_NOTE_SUPAEASY;
							diff = eof_import_events[i]->event[j]->d1 - 0x3C;
						}
						else if((eof_import_events[i]->event[j]->d1 >= 0x48) && (eof_import_events[i]->event[j]->d1 < 0x48 + 6))
						{
							sp->track[picked_track]->note[note_count[picked_track]]->type = EOF_NOTE_EASY;
							diff = eof_import_events[i]->event[j]->d1 - 0x48;
						}
						else if((eof_import_events[i]->event[j]->d1 >= 0x54) && (eof_import_events[i]->event[j]->d1 < 0x54 + 6))
						{
							sp->track[picked_track]->note[note_count[picked_track]]->type = EOF_NOTE_MEDIUM;
							diff = eof_import_events[i]->event[j]->d1 - 0x54;
						}
						else if((eof_import_events[i]->event[j]->d1 >= 0x60) && (eof_import_events[i]->event[j]->d1 < 0x60 + 6))
						{
							sp->track[picked_track]->note[note_count[picked_track]]->type = EOF_NOTE_AMAZING;
							diff = eof_import_events[i]->event[j]->d1 - 0x60;
						}
						else if((eof_import_events[i]->event[j]->d1 >= 120) && (eof_import_events[i]->event[j]->d1 <= 124))
						{
							sp->track[picked_track]->note[note_count[picked_track]]->type = EOF_NOTE_SPECIAL;
							diff = eof_import_events[i]->event[j]->d1 - 120;
						}
						else if((picked_track == EOF_TRACK_DRUM) && (eof_import_events[i]->event[j]->d1 == 95))
						{
							sp->track[picked_track]->note[note_count[picked_track]]->type = EOF_NOTE_AMAZING;
							diff = eof_import_events[i]->event[j]->d1 - 0x60 + 1;	//Treat as gem 1 (bass drum)
						}
						else
						{
							sp->track[picked_track]->note[note_count[picked_track]]->type = -1;
						}

						/* detect forced HOPO */
						hopodiff = -1;
						if(eof_import_events[i]->event[j]->d1 == 0x3C + 5)
						{
							hopodiff = 0;
						}
						else if(eof_import_events[i]->event[j]->d1 == 0x48 + 5)
						{
							hopodiff = 1;
						}
						else if(eof_import_events[i]->event[j]->d1 == 0x54 + 5)
						{
							hopodiff = 2;
						}
						else if(eof_import_events[i]->event[j]->d1 == 0x60 + 5)
						{
							hopodiff = 3;
						}
						else if(eof_import_events[i]->event[j]->d1 == 0x3C + 6)
						{
							hopodiff = 0;
						}
						else if(eof_import_events[i]->event[j]->d1 == 0x48 + 6)
						{
							hopodiff = 1;
						}
						else if(eof_import_events[i]->event[j]->d1 == 0x54 + 6)
						{
							hopodiff = 2;
						}
						else if(eof_import_events[i]->event[j]->d1 == 0x60 + 6)
						{
							hopodiff = 3;
						}
						if(hopodiff >= 0)
						{
							for(k = note_count[picked_track] - 1; k >= first_note; k--)
							{	//Check for each note that has been imported
//								if((sp->track[picked_track]->note[k]->type == hopodiff) && (hopopos[hopodiff] == eof_ConvertToRealTimeInt(eof_import_events[i]->event[j]->pos,0,anchorlist,eof_work_midi->divisions,sp->tags->ogg[0].midi_offset)))
								if((sp->track[picked_track]->note[k]->type == hopodiff) && (sp->track[picked_track]->note[k]->pos >= hopopos[hopodiff]) && (sp->track[picked_track]->note[k]->pos <= eof_ConvertToRealTimeInt(eof_import_events[i]->event[j]->pos,anchorlist,eof_import_ts_changes[0],eof_work_midi->divisions,sp->tags->ogg[0].midi_offset)))
								{	//If the note is in the same difficulty as the HOPO phrase, and its timestamp falls between the HOPO On and HOPO Off marker
									if(hopotype[hopodiff] == 0)
									{
										sp->track[picked_track]->note[k]->flags |= EOF_NOTE_FLAG_F_HOPO;
									}
									else
									{
										sp->track[picked_track]->note[k]->flags |= EOF_NOTE_FLAG_NO_HOPO;
									}
//									break;
								}
							}
						}

						if((eof_import_events[i]->event[j]->d1 == 116) && (sp->track[picked_track]->star_power_paths < EOF_MAX_STAR_POWER))
						{
							sp->track[picked_track]->star_power_path[sp->track[picked_track]->star_power_paths].end_pos = eof_ConvertToRealTimeInt(eof_import_events[i]->event[j]->pos,anchorlist,eof_import_ts_changes[0],eof_work_midi->divisions,sp->tags->ogg[0].midi_offset) - 1;
							sp->track[picked_track]->star_power_paths++;
						}
						else if((eof_import_events[i]->event[j]->d1 == 103) && (sp->track[picked_track]->solos < EOF_MAX_SOLOS))
						{
							sp->track[picked_track]->solo[sp->track[picked_track]->solos].end_pos = eof_ConvertToRealTimeInt(eof_import_events[i]->event[j]->pos,anchorlist,eof_import_ts_changes[0],eof_work_midi->divisions,sp->tags->ogg[0].midi_offset) - 1;
							sp->track[picked_track]->solos++;
						}
						if((note_count[picked_track] > 0) && (sp->track[picked_track]->note[note_count[picked_track] - 1]->type != -1))
						{
							for(k = note_count[picked_track] - 1; k >= first_note; k--)
							{
								if((sp->track[picked_track]->note[k]->type == sp->track[picked_track]->note[note_count[picked_track]]->type) && (sp->track[picked_track]->note[k]->note & diff_chart[diff]))
								{
	//								allegro_message("break %d, %d, %d", k, sp->track[picked_track]->note[k]->note, sp->track[picked_track]->note[note_count[picked_track]]->note);
									sp->track[picked_track]->note[k]->length = eof_ConvertToRealTimeInt(eof_import_events[i]->event[j]->pos,anchorlist,eof_import_ts_changes[0],eof_work_midi->divisions,sp->tags->ogg[0].midi_offset) - sp->track[picked_track]->note[k]->pos;
									if(sp->track[picked_track]->note[k]->length <= 0)
									{
										sp->track[picked_track]->note[k]->length = 1;
									}
									break;
								}
							}
						}
					}
					pticker++;
				}
				eof_track_resize(sp->track[picked_track], note_count[picked_track]);
				if(sp->track[picked_track]->notes > 0)
				{
					eof_track_find_crazy_notes(sp->track[picked_track]);
					used_track[picked_track] = 1;
				}
			}
		}
	}
	/* delete empty lyric lines */
	int lc;
	for(i = sp->vocal_track->lines - 1; i >= 0; i--)
	{
		lc = 0;
		for(j = 0; j < sp->vocal_track->lyrics; j++)
		{
			if((sp->vocal_track->lyric[j]->pos >= sp->vocal_track->line[i].start_pos) && (sp->vocal_track->lyric[j]->pos <= sp->vocal_track->line[i].end_pos))
			{
				lc++;
			}
		}
		if(lc <= 0)
		{
			eof_vocal_track_delete_line(sp->vocal_track, i);
		}
	}

	replace_filename(eof_song_path, fn, "", 1024);
	append_filename(nfn, eof_song_path, "guitar.ogg", 1024);
	if(!eof_load_ogg(nfn))
	{
		eof_destroy_song(sp);
		eof_import_destroy_events_list(eof_import_bpm_events);
		eof_import_destroy_events_list(eof_import_text_events);
		for(i = 0; i < tracks; i++)
		{
			eof_import_destroy_events_list(eof_import_events[i]);
		}
		destroy_midi(eof_work_midi);
		eof_destroy_tempo_list(anchorlist);
		return NULL;
	}
	eof_song_loaded = 1;
	eof_music_length = alogg_get_length_msecs_ogg(eof_music_track);

	/* create text events */
	int b = -1;
	unsigned long tp;
	for(i = 0; i < eof_import_text_events->events; i++)
	{
		if(eof_import_text_events->event[i]->type == 0x01)
		{
			tp = eof_ConvertToRealTimeInt(eof_import_text_events->event[i]->pos,anchorlist,eof_import_ts_changes[0],eof_work_midi->divisions,sp->tags->ogg[0].midi_offset);
			b = eof_import_closest_beat(sp, tp);
			if(b >= 0)
			{
//				allegro_message("%s", eof_import_text_events->event[i]->text);
				eof_song_add_text_event(sp, b, eof_import_text_events->event[i]->text);
				sp->beat[b]->flags |= EOF_BEAT_FLAG_EVENTS;
			}
		}
	}

	/* convert solos to star power for GH charts using FoFiX's method */
	for(i = 0; i < EOF_MAX_TRACKS; i++)
	{
		if(sp->track[i]->star_power_paths < 2)
		{
			for(j = 0; j < sp->track[i]->solos; j++)
			{
				sp->track[i]->star_power_path[j].start_pos = sp->track[i]->solo[j].start_pos;
				sp->track[i]->star_power_path[j].end_pos = sp->track[i]->solo[j].end_pos;
			}
			sp->track[i]->star_power_paths = sp->track[i]->solos;
			sp->track[i]->solos = 0;
		}
	}

	eof_import_destroy_events_list(eof_import_bpm_events);
	eof_import_destroy_events_list(eof_import_text_events);
	for(i = 0; i < tracks; i++)
	{
		eof_import_destroy_events_list(eof_import_events[i]);
	}
	for(i = 0; i < tracks; i++)
	{
		eof_destroy_ts_list(eof_import_ts_changes[i]);
	}

	/* free MIDI */
	destroy_midi(eof_work_midi);
	eof_destroy_tempo_list(anchorlist);

	eof_changes = 0;
//	eof_setup_menus();
	eof_music_pos = 0;
	eof_selected_track = 0;
	eof_selected_ogg = 0;

	return sp;
}
