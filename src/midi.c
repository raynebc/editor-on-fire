#include <allegro.h>
#include "main.h"
#include "dialog.h"
#include "note.h"
#include "beat.h"
#include "midi.h"
#include "utility.h"
#include "menu/note.h"	//For pitch macros

#define EOF_MIDI_TIMER_FREQUENCY  40

#define EOF_IMPORT_MAX_EVENTS 100000

typedef struct
{

	unsigned long pos;
	unsigned char type;
	int d1, d2, d3, d4;
	char text[256];

} EOF_IMPORT_MIDI_EVENT;

typedef struct
{

	EOF_IMPORT_MIDI_EVENT * event[EOF_IMPORT_MAX_EVENTS];
	int events;
	int type;

} EOF_IMPORT_MIDI_EVENT_LIST;

EOF_IMPORT_MIDI_EVENT_LIST * eof_import_events[16];
EOF_IMPORT_MIDI_EVENT_LIST * eof_import_bpm_events;
EOF_IMPORT_MIDI_EVENT_LIST * eof_import_text_events;
double eof_import_bpm_pos[1024] = {0.0};
int eof_import_bpm_count = 0;

MIDI * eof_work_midi = NULL;

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

void eof_import_destroy_events_list(EOF_IMPORT_MIDI_EVENT_LIST * lp)
{
	int i;

	for(i = 0; i < lp->events; i++)
	{
		free(lp->event[i]);
	}
	free(lp);
}

static EOF_MIDI_EVENT * eof_midi_event[EOF_MAX_MIDI_EVENTS];
static int eof_midi_events = 0;

void eof_add_midi_event(int pos, int type, int note)
{
	eof_midi_event[eof_midi_events] = malloc(sizeof(EOF_MIDI_EVENT));
	if(eof_midi_event[eof_midi_events])
	{
		eof_midi_event[eof_midi_events]->pos = pos;
		eof_midi_event[eof_midi_events]->type = type;
		eof_midi_event[eof_midi_events]->note = note;
		eof_midi_events++;
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

int eof_import_figure_beat(unsigned long pos)
{
	int i;

	for(i = 0; i < eof_import_bpm_events->events; i++)
	{
		if(eof_import_bpm_events->event[i]->pos <= pos && eof_import_bpm_events->event[i + 1]->pos > pos)
		{
			return i;
		}
	}
	return -1;
}

void eof_midi_import_add_event(EOF_IMPORT_MIDI_EVENT_LIST * events, unsigned long pos, unsigned char event, unsigned long d1, unsigned long d2)
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

			events->events++;
		}
	}
	else
	{
//		allegro_message("too many events!");
	}
}

void eof_midi_import_add_text_event(EOF_IMPORT_MIDI_EVENT_LIST * events, unsigned long pos, unsigned char event, char * text, unsigned long size)
{
	if(events->events < EOF_IMPORT_MAX_EVENTS)
	{
		events->event[events->events] = malloc(sizeof(EOF_IMPORT_MIDI_EVENT));
		if(events->event[events->events])
		{
			events->event[events->events]->pos = pos;
			events->event[events->events]->type = event;
			memcpy(events->event[events->events]->text, text, size);
			events->event[events->events]->text[size] = '\0';

			events->events++;
		}
	}
	else
	{
//		allegro_message("too many events!");
	}
}

double eof_import_get_bpm(unsigned long pos)
{
	unsigned long ppqn = eof_import_bpm_events->events > 0 ? eof_import_bpm_events->event[0]->d1 : 500000;
	int i;

	for(i = 0; i < eof_import_bpm_events->events; i++)
	{
		if(eof_import_bpm_events->event[i]->pos <= pos)
		{
			ppqn = eof_import_bpm_events->event[i]->d1;
		}
		else
		{
			break;
		}
	}
	return (double)60000000.0 / (double)ppqn;
}

typedef struct
{

	char type[256];
	char value[1024];

} EOF_IMPORT_INI_SETTING;

EOF_IMPORT_INI_SETTING eof_import_ini_setting[32];
int eof_import_ini_settings = 0;

int eof_import_ini(EOF_SONG * sp, char * fn)
{
	PACKFILE * fp;
	char textbuffer[4096] = {0};
	char * line_token = NULL;
	int textlength = 0;
	char * token;
	char * equals = NULL;
	int i;
	int j;
	int size;

	size = file_size_ex(fn);
	fp = pack_fopen(fn, "r");
	if(!fp)
	{
		return 0;
	}
	eof_import_ini_settings = 0;

	pack_fread(textbuffer, size, fp);
	textlength = ustrlen(textbuffer);
	ustrtok(textbuffer, "\r\n");
//	pack_fgets(textline, 1024, fp);
	while(1)
	{
		line_token = ustrtok(NULL, "\r\n[]");
		if(!line_token)
		{
			break;
		}
		else
		{
			if(ustrlen(line_token) > 6)
			{
				/* find the first '=' */
				for(i = 0; i < ustrlen(line_token); i++)
				{
					if(ugetc(&line_token[uoffset(line_token, i)]) == '=')
					{
						equals = &line_token[uoffset(line_token, i)];
						break;
					}
				}

				/* if this line has an '=', process line as a setting */
				if(equals)
				{
					equals[0] = '\0';
					token = equals + 1;
					ustrcpy(eof_import_ini_setting[eof_import_ini_settings].type, line_token);
					ustrcpy(eof_import_ini_setting[eof_import_ini_settings].value, token);
					eof_import_ini_settings++;
				}
			}
		}
	}
	for(i = 0; i < eof_import_ini_settings; i++)
	{
		if(!ustricmp(eof_import_ini_setting[i].type, "artist ") || !ustricmp(eof_import_ini_setting[i].type, "artist"))
		{
			for(j = 0; j < ustrlen(eof_import_ini_setting[i].value); j++)
			{
				if(eof_import_ini_setting[i].value[j] != ' ')
				{
					ustrcpy(sp->tags->artist, &(eof_import_ini_setting[i].value[j]));
					break;
				}
			}
		}
		else if(!ustricmp(eof_import_ini_setting[i].type, "name ") || !ustricmp(eof_import_ini_setting[i].type, "name"))
		{
			for(j = 0; j < ustrlen(eof_import_ini_setting[i].value); j++)
			{
				if(eof_import_ini_setting[i].value[j] != ' ')
				{
					ustrcpy(sp->tags->title, &(eof_import_ini_setting[i].value[j]));
					break;
				}
			}
		}
		else if(!ustricmp(eof_import_ini_setting[i].type, "frets ") || !ustricmp(eof_import_ini_setting[i].type, "frets"))
		{
			for(j = 0; j < ustrlen(eof_import_ini_setting[i].value); j++)
			{
				if(eof_import_ini_setting[i].value[j] != ' ')
				{
					ustrcpy(sp->tags->frettist, &(eof_import_ini_setting[i].value[j]));
					break;
				}
			}
		}
		else if(!ustricmp(eof_import_ini_setting[i].type, "year ") || !ustricmp(eof_import_ini_setting[i].type, "year"))
		{
			for(j = 0; j < ustrlen(eof_import_ini_setting[i].value); j++)
			{
				if(eof_import_ini_setting[i].value[j] != ' ')
				{
					ustrcpy(sp->tags->year, &(eof_import_ini_setting[i].value[j]));
					break;
				}
			}
		}
		else if(!ustricmp(eof_import_ini_setting[i].type, "loading_phrase ") || !ustricmp(eof_import_ini_setting[i].type, "loading_phrase"))
		{
			for(j = 0; j < ustrlen(eof_import_ini_setting[i].value); j++)
			{
				if(eof_import_ini_setting[i].value[j] != ' ')
				{
					ustrcpy(sp->tags->loading_text, &(eof_import_ini_setting[i].value[j]));
					break;
				}
			}
		}
		else if(!ustricmp(eof_import_ini_setting[i].type, "lyrics ") || !ustricmp(eof_import_ini_setting[i].type, "lyrics"))
		{
			for(j = 0; j < ustrlen(eof_import_ini_setting[i].value); j++)
			{
				if(eof_import_ini_setting[i].value[j] != ' ')
				{
					if(!ustricmp(&(eof_import_ini_setting[i].value[j]), "True"))
					{
						sp->tags->lyrics = 1;
					}
					break;
				}
			}
		}
		else if(!ustricmp(eof_import_ini_setting[i].type, "eighthnote_hopo ") || !ustricmp(eof_import_ini_setting[i].type, "eighthnote_hopo"))
		{
			for(j = 0; j < ustrlen(eof_import_ini_setting[i].value); j++)
			{
				if(eof_import_ini_setting[i].value[j] != ' ')
				{
					if(!ustricmp(&(eof_import_ini_setting[i].value[j]), "1"))
					{
						sp->tags->eighth_note_hopo = 1;
					}
					break;
				}
			}
		}
		else if(!ustricmp(eof_import_ini_setting[i].type, "delay ") || !ustricmp(eof_import_ini_setting[i].type, "delay"))
		{
			sp->tags->ogg[0].midi_offset = atoi(eof_import_ini_setting[i].value);
			if(sp->tags->ogg[0].midi_offset < 0)
			{
				sp->tags->ogg[0].midi_offset = 0;
			}
		}

		/* for custom settings */
		else
		{
			sprintf(sp->tags->ini_setting[sp->tags->ini_settings], "%s = %s", eof_import_ini_setting[i].type, eof_import_ini_setting[i].value);
			sp->tags->ini_settings++;
		}
	}
	pack_fclose(fp);
	return 1;
}

unsigned long eof_import_midi_to_eof(int offset, unsigned long pos)
{
	int i, beat = 0;
	double current_pos = offset;
	double delta;
	double bpm = eof_import_get_bpm(pos);

	if(eof_import_bpm_events->events == 1)
	{
		delta = pos - eof_import_bpm_events->event[0]->pos;
		bpm = eof_import_get_bpm(pos);
		current_pos += ((double)delta / (double)eof_work_midi->divisions) * ((double)60000 / bpm);
		return current_pos;
	}
	/* find the BPM area this position lies in */
	for(i = 0; i < eof_import_bpm_events->events; i++)
	{
		if(eof_import_bpm_events->event[i]->pos > pos)
		{
			beat = i - 1;
			break;
		}
	}

	if(i == eof_import_bpm_events->events)
	{
		beat = eof_import_bpm_events->events - 1;
	}

	/* sum up the total of the time up to the BPM area the position lies in */
	for(i = 0; i < beat; i++)
	{
		delta = eof_import_bpm_events->event[i + 1]->pos - eof_import_bpm_events->event[i]->pos;
		if(delta >= 0)
		{
			bpm = eof_import_get_bpm(eof_import_bpm_events->event[i]->pos);
			current_pos += ((double)delta / (double)eof_work_midi->divisions) * ((double)60000 / bpm);
		}
	}
	if(pos > eof_import_bpm_events->event[beat]->pos)
	{
		if(beat >= 0)
		{
			delta = pos - eof_import_bpm_events->event[beat]->pos;
			bpm = eof_import_get_bpm(pos);
			current_pos += ((double)delta / (double)eof_work_midi->divisions) * ((double)60000.0 / bpm);
		}
	}
	return current_pos;
}

unsigned long eof_import_midi_to_eof_optimized(unsigned long pos)
{
	int i, beat = 0;
	double current_pos;
	double delta;
	double bpm;

//	return eof_import_midi_to_eof(sp->tags->ogg[0].midi_offset, pos);

	/* find the BPM area this position lies in */
	for(i = 0; i < eof_import_bpm_events->events; i++)
	{
		if(eof_import_bpm_events->event[i]->pos > pos)
		{
			beat = i - 1;
			break;
		}
	}
	if(beat < 0)
	{
		beat = 0;
	}
	if(i == eof_import_bpm_events->events)
	{
		beat = eof_import_bpm_events->events - 1;
	}
	if(beat < 0)
	{
		beat = 0;
	}
	current_pos = eof_import_bpm_pos[beat];
	delta = pos - eof_import_bpm_events->event[beat]->pos;
	bpm = eof_import_get_bpm(pos);
	current_pos += ((double)delta / (double)eof_work_midi->divisions) * ((double)60000.0 / bpm);
	return current_pos;
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
	if(bb >= 0 && ab >= 0)
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

EOF_SONG * eof_import_midi(const char * fn)
{
	EOF_SONG * sp;
	int pticker = 0;
	int ptotal_events = 0;
	int percent;
	int i, j, k;
	int rbg = 0;
	int tracks = 0;
	int track[16] = {0};
	int track_pos;
	unsigned long delta;
	unsigned long absolute_pos;
	unsigned long bytes_used;
	unsigned char current_event;
	unsigned char current_event_hi = 0;
	unsigned char last_event = 0;
	unsigned char current_meta_event;
	unsigned long d1, d2, d3, d4;
	char text[256];
	char nfn[1024] = {0};
	char backup_filename[1024] = {0};
	char ttit[256] = {0};
	unsigned long ppqn = 500000; // default of 120 BPM

	unsigned long curpos = 0;

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
		return NULL;
	}
	replace_filename(backup_filename, fn, "song.ini", 1024);
	eof_import_ini(sp, backup_filename);

	/* read INI file */

	/* parse MIDI data */
	for(i = 0; i < 16; i++)
	{
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
		return 0;
	}
	eof_import_text_events = eof_import_create_events_list();
	if(!eof_import_text_events)
	{
		return 0;
	}
	for(i = 0; i < tracks; i++)
	{
		eof_import_events[i] = eof_import_create_events_list();
		if(!eof_import_events[i])
		{
			return 0;
		}
		track_pos = 0;
		absolute_pos = sp->tags->ogg[0].midi_offset;
		while(track_pos < eof_work_midi->track[track[i]].len)
		{

			/* read delta */
			bytes_used = 0;
			delta = eof_parse_var_len(eof_work_midi->track[track[i]].data, track_pos, &bytes_used);
			absolute_pos += delta;
			track_pos += bytes_used;

			/* read event type */
			if(current_event_hi >= 0x80 && current_event_hi < 0xF0)
			{
				last_event = current_event_hi;
			}
			current_event = eof_work_midi->track[track[i]].data[track_pos];
			current_event_hi = current_event & 0xF0;
			switch(current_event_hi)
			{

				/* note off */
				case 0x80:
				{
					track_pos++;
					d1 = eof_work_midi->track[track[i]].data[track_pos];
					track_pos++;
					d2 = eof_work_midi->track[track[i]].data[track_pos];
					track_pos++;
					eof_midi_import_add_event(eof_import_events[i], absolute_pos, 0x80, d1, d2);
					ptotal_events++;
					break;
				}

				/* note on */
				case 0x90:
				{
					track_pos++;
					d1 = eof_work_midi->track[track[i]].data[track_pos];
					track_pos++;
					d2 = eof_work_midi->track[track[i]].data[track_pos];
					track_pos++;
					if(d2 <= 0)
					{
						eof_midi_import_add_event(eof_import_events[i], absolute_pos, 0x80, d1, d2);
					}
					else
					{
						eof_midi_import_add_event(eof_import_events[i], absolute_pos, 0x90, d1, d2);
					}
					ptotal_events++;
					break;
				}

				/* key aftertouch */
				case 0xA0:
				{
					track_pos++;
					d1 = eof_work_midi->track[track[i]].data[track_pos];
					track_pos++;
					d2 = eof_work_midi->track[track[i]].data[track_pos];
					track_pos++;
					break;
				}

				/* controller change */
				case 0xB0:
				{
					track_pos++;
					d1 = eof_work_midi->track[track[i]].data[track_pos];
					track_pos++;
					d2 = eof_work_midi->track[track[i]].data[track_pos];
					track_pos++;
					break;
				}

				/* program change */
				case 0xC0:
				{
					track_pos++;
					d1 = eof_work_midi->track[track[i]].data[track_pos];
					track_pos++;
					break;
				}

				/* channel aftertouch */
				case 0xD0:
				{
					track_pos++;
					d1 = eof_work_midi->track[track[i]].data[track_pos];
					track_pos++;
					break;
				}

				/* pitch wheel change */
				case 0xE0:
				{
					track_pos++;
					d1 = eof_work_midi->track[track[i]].data[track_pos];
					track_pos++;
					d2 = eof_work_midi->track[track[i]].data[track_pos];
					track_pos++;
					break;
				}

				/* running event */
				default:
				{
					switch(last_event)
					{

						/* note off */
						case 0x80:
						{
							d1 = eof_work_midi->track[track[i]].data[track_pos];
							track_pos++;
							d2 = eof_work_midi->track[track[i]].data[track_pos];
							track_pos++;
							eof_midi_import_add_event(eof_import_events[i], absolute_pos, 0x80, d1, d2);
							ptotal_events++;
							break;
						}

						/* note on */
						case 0x90:
						{
							d1 = eof_work_midi->track[track[i]].data[track_pos];
							track_pos++;
							d2 = eof_work_midi->track[track[i]].data[track_pos];
							track_pos++;
							if(d2 <= 0)
							{
								eof_midi_import_add_event(eof_import_events[i], absolute_pos, 0x80, d1, d2);
							}
							else
							{
								eof_midi_import_add_event(eof_import_events[i], absolute_pos, 0x90, d1, d2);
							}
							ptotal_events++;
							break;
						}

						/* key aftertouch */
						case 0xA0:
						{
							d1 = eof_work_midi->track[track[i]].data[track_pos];
							track_pos++;
							d2 = eof_work_midi->track[track[i]].data[track_pos];
							track_pos++;
							break;
						}

						/* controller change */
						case 0xB0:
						{
							d1 = eof_work_midi->track[track[i]].data[track_pos];
							track_pos++;
							d2 = eof_work_midi->track[track[i]].data[track_pos];
							track_pos++;
							break;
						}

						/* program change */
						case 0xC0:
						{
							d1 = eof_work_midi->track[track[i]].data[track_pos];
							track_pos++;
							break;
						}

						/* channel aftertouch */
						case 0xD0:
						{
							d1 = eof_work_midi->track[track[i]].data[track_pos];
							track_pos++;
							break;
						}

						/* pitch wheel change */
						case 0xE0:
						{
							d1 = eof_work_midi->track[track[i]].data[track_pos];
							track_pos++;
							d2 = eof_work_midi->track[track[i]].data[track_pos];
							track_pos++;
							break;
						}

					}
					break;
				}

				/* meta event */
				case 0xF0:
				{
					if(current_event == 0xF0)
					{
						track_pos++;
						bytes_used = 0;
						d3 = eof_parse_var_len(eof_work_midi->track[track[i]].data, track_pos, &bytes_used);
						track_pos += bytes_used;
						track_pos += d3;
					}
					else if(current_event == 0xFF)
					{
						track_pos++;
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
								strncpy(text, (char *)&eof_work_midi->track[track[i]].data[track_pos + 1], eof_work_midi->track[track[i]].data[track_pos]);
								eof_midi_import_add_text_event(eof_import_text_events, absolute_pos, 0x01, text, eof_work_midi->track[track[i]].data[track_pos]);
								track_pos += eof_work_midi->track[track[i]].data[track_pos] + 1;
								ptotal_events++;
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
									text[j] = eof_work_midi->track[track[i]].data[track_pos];
								}
								text[j] = '\0';
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
								strncpy(text, (char *)&eof_work_midi->track[track[i]].data[track_pos + 1], eof_work_midi->track[track[i]].data[track_pos]);
								eof_midi_import_add_text_event(eof_import_events[i], absolute_pos, 0x05, text, eof_work_midi->track[track[i]].data[track_pos]);
								track_pos += eof_work_midi->track[track[i]].data[track_pos] + 1;
								ptotal_events++;
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
//								if(track_pos >= eof_work_midi->track[track[i]].len)
//								{
//								}
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
								eof_midi_import_add_event(eof_import_bpm_events, absolute_pos, 0x51, d4, 0);
								track_pos++;
								break;
							}

							/* time signature */
							case 0x58:
							{
								track_pos += 5;
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
								track_pos++;
								break;
							}
						}
					}
					break;
				}
			}
		}
	}

	/* optimize position calculator */
	eof_import_bpm_count = 0;
	if(eof_import_bpm_events->events <= 0)
	{
		eof_midi_import_add_event(eof_import_bpm_events, sp->tags->ogg[0].midi_offset, 0x51, 500000, 0);
	}
	for(i = 0; i < eof_import_bpm_events->events; i++)
	{
		eof_import_bpm_pos[eof_import_bpm_count] = eof_import_midi_to_eof(sp->tags->ogg[0].midi_offset, eof_import_bpm_events->event[i]->pos);
		eof_import_bpm_count++;
	}

	/* second pass, create EOF notes */
	int picked_track;

	/* for Rock Band songs, we need to ignore certain tracks */
	char used_track[16] = {0};

	unsigned char diff = 0;
	unsigned char diff_chart[5] = {1, 2, 4, 8, 16};
	int note_count[16] = {0};
	int first_note;
	unsigned long hopopos[4];
	char hopotype[4];
	int hopodiff;
	for(i = 0; i < tracks; i++)
	{

		picked_track = eof_import_events[i]->type >= 0 ? eof_import_events[i]->type : rbg == 0 ? EOF_TRACK_GUITAR : -1;
		first_note = note_count[picked_track];
		if(picked_track >= 0 && !used_track[picked_track])
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
							sp->vocal_track->line[sp->vocal_track->lines].start_pos = eof_import_midi_to_eof_optimized(eof_import_events[i]->event[j]->pos);
							last_105 = sp->vocal_track->lines;
//							sp->vocal_track->lines++;
						}
						else if(eof_import_events[i]->event[j]->d1 == 106)
						{
							sp->vocal_track->line[sp->vocal_track->lines].start_pos = eof_import_midi_to_eof_optimized(eof_import_events[i]->event[j]->pos);
							last_106 = sp->vocal_track->lines;
//							sp->vocal_track->lines++;
						}
						/* overdrive */
						else if(eof_import_events[i]->event[j]->d1 == 116)
						{
							overdrive_pos = eof_import_midi_to_eof_optimized(eof_import_events[i]->event[j]->pos);
						}
						/* percussion */
						else if(eof_import_events[i]->event[j]->d1 == 96 || eof_import_events[i]->event[j]->d1 == 97)
						{
						}
						else if(eof_import_events[i]->event[j]->d1 >= MINPITCH && eof_import_events[i]->event[j]->d1 <= MAXPITCH)
						{
							for(k = 0; k < note_count[picked_track]; k++)
							{
								if(sp->vocal_track->lyric[k]->pos == eof_import_midi_to_eof_optimized(eof_import_events[i]->event[j]->pos))
								{
									break;
								}
							}
							/* no note associated with this lyric so create a new note */
							if(k == note_count[picked_track])
							{
								sp->vocal_track->lyric[note_count[picked_track]]->note = eof_import_events[i]->event[j]->d1;
								sp->vocal_track->lyric[note_count[picked_track]]->pos = eof_import_midi_to_eof_optimized(eof_import_events[i]->event[j]->pos);
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
							sp->vocal_track->line[last_105].end_pos = eof_import_midi_to_eof_optimized(eof_import_events[i]->event[j]->pos);
							sp->vocal_track->lines++;
							if(overdrive_pos == sp->vocal_track->line[last_105].start_pos)
							{
								sp->vocal_track->line[last_105].flags |= EOF_LYRIC_LINE_FLAG_OVERDRIVE;
							}
						}
						else if(eof_import_events[i]->event[j]->d1 == 106)
						{
							sp->vocal_track->line[last_106].end_pos = eof_import_midi_to_eof_optimized(eof_import_events[i]->event[j]->pos);
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
						else if(eof_import_events[i]->event[j]->d1 == 96 || eof_import_events[i]->event[j]->d1 == 97)
						{
						}
						else if(eof_import_events[i]->event[j]->d1 >= MINPITCH && eof_import_events[i]->event[j]->d1 <= MAXPITCH)
						{
							if(note_count[picked_track] > 0)
							{
								sp->vocal_track->lyric[note_count[picked_track] - 1]->length = eof_import_midi_to_eof_optimized(eof_import_events[i]->event[j]->pos) - sp->vocal_track->lyric[note_count[picked_track] - 1]->pos;
							}
						}
					}

					/* lyric */
					else if((eof_import_events[i]->event[j]->type == 0x05 || eof_import_events[i]->event[j]->type == 0x01) && eof_import_events[i]->event[j]->text[0] != '[')
					{
						for(k = 0; k < note_count[picked_track]; k++)
						{
							if(sp->vocal_track->lyric[k]->pos == eof_import_midi_to_eof_optimized(eof_import_events[i]->event[j]->pos))
							{
								break;
							}
						}

						/* no note associated with this lyric so create a new note */
						if(k == note_count[picked_track])
						{
							strcpy(sp->vocal_track->lyric[note_count[picked_track]]->text, eof_import_events[i]->event[j]->text);
							sp->vocal_track->lyric[note_count[picked_track]]->note = 0;
							sp->vocal_track->lyric[note_count[picked_track]]->pos = eof_import_midi_to_eof_optimized(eof_import_events[i]->event[j]->pos);
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
						if(eof_import_events[i]->event[j]->d1 >= 0x3C && eof_import_events[i]->event[j]->d1 < 0x3C + 6)
						{
							sp->track[picked_track]->note[note_count[picked_track]]->type = EOF_NOTE_SUPAEASY;
							diff = eof_import_events[i]->event[j]->d1 - 0x3C;
						}
						else if(eof_import_events[i]->event[j]->d1 >= 0x48 && eof_import_events[i]->event[j]->d1 < 0x48 + 6)
						{
							sp->track[picked_track]->note[note_count[picked_track]]->type = EOF_NOTE_EASY;
							diff = eof_import_events[i]->event[j]->d1 - 0x48;
						}
						else if(eof_import_events[i]->event[j]->d1 >= 0x54 && eof_import_events[i]->event[j]->d1 < 0x54 + 6)
						{
							sp->track[picked_track]->note[note_count[picked_track]]->type = EOF_NOTE_MEDIUM;
							diff = eof_import_events[i]->event[j]->d1 - 0x54;
						}
						else if(eof_import_events[i]->event[j]->d1 >= 0x60 && eof_import_events[i]->event[j]->d1 < 0x60 + 6)
						{
							sp->track[picked_track]->note[note_count[picked_track]]->type = EOF_NOTE_AMAZING;
							diff = eof_import_events[i]->event[j]->d1 - 0x60;
						}
						else if(eof_import_events[i]->event[j]->d1 >= 120 && eof_import_events[i]->event[j]->d1 <= 124)
						{
							sp->track[picked_track]->note[note_count[picked_track]]->type = EOF_NOTE_SPECIAL;
							diff = eof_import_events[i]->event[j]->d1 - 120;
						}
						else
						{
							sp->track[picked_track]->note[note_count[picked_track]]->type = -1;
						}

						/* store forced HOPO marker, when the note off for this marker occurs, search for note with same
						   position and apply it to that note */
						if(eof_import_events[i]->event[j]->d1 == 0x3C + 5)
						{
							hopopos[0] = eof_import_midi_to_eof_optimized(eof_import_events[i]->event[j]->pos);
							hopotype[0] = 0;
						}
						else if(eof_import_events[i]->event[j]->d1 == 0x48 + 5)
						{
							hopopos[1] = eof_import_midi_to_eof_optimized(eof_import_events[i]->event[j]->pos);
							hopotype[1] = 0;
						}
						else if(eof_import_events[i]->event[j]->d1 == 0x54 + 5)
						{
							hopopos[2] = eof_import_midi_to_eof_optimized(eof_import_events[i]->event[j]->pos);
							hopotype[2] = 0;
						}
						else if(eof_import_events[i]->event[j]->d1 == 0x60 + 5)
						{
							hopopos[3] = eof_import_midi_to_eof_optimized(eof_import_events[i]->event[j]->pos);
							hopotype[3] = 0;
						}
						else if(eof_import_events[i]->event[j]->d1 == 0x3C + 6)
						{
							hopopos[0] = eof_import_midi_to_eof_optimized(eof_import_events[i]->event[j]->pos);
							hopotype[0] = 1;
						}
						else if(eof_import_events[i]->event[j]->d1 == 0x48 + 6)
						{
							hopopos[1] = eof_import_midi_to_eof_optimized(eof_import_events[i]->event[j]->pos);
							hopotype[1] = 1;
						}
						else if(eof_import_events[i]->event[j]->d1 == 0x54 + 6)
						{
							hopopos[2] = eof_import_midi_to_eof_optimized(eof_import_events[i]->event[j]->pos);
							hopotype[2] = 1;
						}
						else if(eof_import_events[i]->event[j]->d1 == 0x60 + 6)
						{
							hopopos[3] = eof_import_midi_to_eof_optimized(eof_import_events[i]->event[j]->pos);
							hopotype[3] = 1;
						}

						/* star power and solos */
						if(eof_import_events[i]->event[j]->d1 == 116 && sp->track[picked_track]->star_power_paths < EOF_MAX_STAR_POWER)
						{
							sp->track[picked_track]->star_power_path[sp->track[picked_track]->star_power_paths].start_pos = eof_import_midi_to_eof_optimized(eof_import_events[i]->event[j]->pos);
						}
						else if(eof_import_events[i]->event[j]->d1 == 103 && sp->track[picked_track]->solos < EOF_MAX_SOLOS)
						{
							sp->track[picked_track]->solo[sp->track[picked_track]->solos].start_pos = eof_import_midi_to_eof_optimized(eof_import_events[i]->event[j]->pos);
						}

						if(sp->track[picked_track]->note[note_count[picked_track]]->type != -1)
						{
							for(k = first_note; k < note_count[picked_track]; k++)
							{
								if(sp->track[picked_track]->note[k]->pos == eof_import_midi_to_eof_optimized(eof_import_events[i]->event[j]->pos) && sp->track[picked_track]->note[k]->type == sp->track[picked_track]->note[note_count[picked_track]]->type)
								{
									break;
								}
							}
							if(k == note_count[picked_track])
							{
								sp->track[picked_track]->note[note_count[picked_track]]->note = diff_chart[diff];
								sp->track[picked_track]->note[note_count[picked_track]]->pos = eof_import_midi_to_eof_optimized(eof_import_events[i]->event[j]->pos);
								sp->track[picked_track]->note[note_count[picked_track]]->length = 100;
								sp->track[picked_track]->note[note_count[picked_track]]->flags = 0;
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
						if(eof_import_events[i]->event[j]->d1 >= 0x3C && eof_import_events[i]->event[j]->d1 < 0x3C + 6)
						{
							sp->track[picked_track]->note[note_count[picked_track]]->type = EOF_NOTE_SUPAEASY;
							diff = eof_import_events[i]->event[j]->d1 - 0x3C;
						}
						else if(eof_import_events[i]->event[j]->d1 >= 0x48 && eof_import_events[i]->event[j]->d1 < 0x48 + 6)
						{
							sp->track[picked_track]->note[note_count[picked_track]]->type = EOF_NOTE_EASY;
							diff = eof_import_events[i]->event[j]->d1 - 0x48;
						}
						else if(eof_import_events[i]->event[j]->d1 >= 0x54 && eof_import_events[i]->event[j]->d1 < 0x54 + 6)
						{
							sp->track[picked_track]->note[note_count[picked_track]]->type = EOF_NOTE_MEDIUM;
							diff = eof_import_events[i]->event[j]->d1 - 0x54;
						}
						else if(eof_import_events[i]->event[j]->d1 >= 0x60 && eof_import_events[i]->event[j]->d1 < 0x60 + 6)
						{
							sp->track[picked_track]->note[note_count[picked_track]]->type = EOF_NOTE_AMAZING;
							diff = eof_import_events[i]->event[j]->d1 - 0x60;
						}
						else if(eof_import_events[i]->event[j]->d1 >= 120 && eof_import_events[i]->event[j]->d1 <= 124)
						{
							sp->track[picked_track]->note[note_count[picked_track]]->type = EOF_NOTE_SPECIAL;
							diff = eof_import_events[i]->event[j]->d1 - 120;
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
							{
								if(sp->track[picked_track]->note[k]->type == hopodiff && hopopos[hopodiff] == eof_import_midi_to_eof_optimized(eof_import_events[i]->event[j]->pos))
								{
									if(hopotype[hopodiff] == 0)
									{
										sp->track[picked_track]->note[k]->flags |= EOF_NOTE_FLAG_F_HOPO;
									}
									else
									{
										sp->track[picked_track]->note[k]->flags |= EOF_NOTE_FLAG_NO_HOPO;
									}
									break;
								}
							}
						}

						if(eof_import_events[i]->event[j]->d1 == 116 && sp->track[picked_track]->star_power_paths < EOF_MAX_STAR_POWER)
						{
							sp->track[picked_track]->star_power_path[sp->track[picked_track]->star_power_paths].end_pos = eof_import_midi_to_eof_optimized(eof_import_events[i]->event[j]->pos) - 1;
							sp->track[picked_track]->star_power_paths++;
						}
						else if(eof_import_events[i]->event[j]->d1 == 103 && sp->track[picked_track]->solos < EOF_MAX_SOLOS)
						{
							sp->track[picked_track]->solo[sp->track[picked_track]->solos].end_pos = eof_import_midi_to_eof_optimized(eof_import_events[i]->event[j]->pos) - 1;
							sp->track[picked_track]->solos++;
						}
						if(note_count[picked_track] > 0 && sp->track[picked_track]->note[note_count[picked_track] - 1]->type != -1)
	//					if(sp->track[picked_track]->note[note_count[picked_track] - 1]->type != -1)
						{
							for(k = note_count[picked_track] - 1; k >= first_note; k--)
							{
	//							if(sp->track[picked_track]->note[k]->type == sp->track[picked_track]->note[note_count[picked_track] - 1]->type && (sp->track[picked_track]->note[k]->note & diff_chart[diff]))
								if(sp->track[picked_track]->note[k]->type == sp->track[picked_track]->note[note_count[picked_track]]->type && (sp->track[picked_track]->note[k]->note & diff_chart[diff]))
								{
	//								allegro_message("break %d, %d, %d", k, sp->track[picked_track]->note[k]->note, sp->track[picked_track]->note[note_count[picked_track]]->note);
									sp->track[picked_track]->note[k]->length = eof_import_midi_to_eof_optimized(eof_import_events[i]->event[j]->pos) - sp->track[picked_track]->note[k]->pos;
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
			if(sp->vocal_track->lyric[j]->pos >= sp->vocal_track->line[i].start_pos && sp->vocal_track->lyric[j]->pos <= sp->vocal_track->line[i].end_pos)
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
		return NULL;
	}
	eof_song_loaded = 1;
	eof_music_length = alogg_get_length_msecs_ogg(eof_music_track);

	/* third pass, create tempo map */
	ppqn = eof_import_bpm_events->events > 0 ? eof_import_bpm_events->event[0]->d1 : 500000; // default of 120 BPM
	double bl = (double)1000.0 / (((double)60000000.0 / (double)ppqn) / (double)60.0);
	double beat_count;
	curpos = 0;
	if(eof_import_bpm_events->events <= 1)
	{
		eof_song_add_beat(sp);
		sp->beat[0]->pos = sp->tags->ogg[0].midi_offset;
		sp->beat[0]->fpos = sp->beat[0]->pos;
		sp->beat[0]->ppqn = ppqn;
		bl = (double)1000.0 / (((double)60000000.0 / (double)ppqn) / (double)60.0);
		beat_count = ((double)eof_music_length) / bl + 0.1 - 1.0;
		for(i = 0; i < beat_count; i++)
		{
			eof_song_add_beat(sp);
			sp->beat[sp->beats - 1]->pos = (double)sp->tags->ogg[0].midi_offset + bl * (i + 1);
			sp->beat[sp->beats - 1]->fpos = sp->beat[sp->beats - 1]->pos;
			sp->beat[sp->beats - 1]->ppqn = ppqn;
		}
	}
	else
	{
		for(i = 0; i < eof_import_bpm_events->events; i++)
		{
			if(eof_import_bpm_events->event[i]->type == 0x51)
			{
				eof_song_add_beat(sp);
				sp->beat[sp->beats - 1]->pos = eof_import_midi_to_eof(sp->tags->ogg[0].midi_offset, eof_import_bpm_events->event[i]->pos);
				sp->beat[sp->beats - 1]->fpos = sp->beat[sp->beats - 1]->pos;
				sp->beat[sp->beats - 1]->ppqn = eof_import_bpm_events->event[i]->d1;
				if(ppqn != eof_import_bpm_events->event[i]->d1)
				{
					sp->beat[sp->beats - 1]->flags |= EOF_BEAT_FLAG_ANCHOR;
				}
				ppqn = eof_import_bpm_events->event[i]->d1;
				bl = (double)1000.0 / (((double)60000000.0 / (double)eof_import_bpm_events->event[i]->d1) / (double)60.0);
				if(i < eof_import_bpm_events->events - 1)
				{
					beat_count = (eof_import_midi_to_eof(sp->tags->ogg[0].midi_offset, eof_import_bpm_events->event[i + 1]->pos) - eof_import_midi_to_eof(sp->tags->ogg[0].midi_offset, eof_import_bpm_events->event[i]->pos)) / bl + 0.1 - 1.0;
				}
				else
				{
					beat_count = ((double)eof_music_length - eof_import_midi_to_eof(sp->tags->ogg[0].midi_offset, eof_import_bpm_events->event[i]->pos)) / bl + 0.1 - 1.0;
				}
				for(j = 0; j < beat_count - 1; j++)
				{
					eof_song_add_beat(sp);
					sp->beat[sp->beats - 1]->pos = eof_import_midi_to_eof(sp->tags->ogg[0].midi_offset, eof_import_bpm_events->event[i]->pos) + bl * (double)(j + 1);
					sp->beat[sp->beats - 1]->fpos = sp->beat[sp->beats - 1]->pos;
					sp->beat[sp->beats - 1]->ppqn = eof_import_bpm_events->event[i]->d1;
				}
				ppqn = eof_import_bpm_events->event[i]->d1;
			}
		}
	}

	/* create text events */
	int b = -1;
	unsigned long tp;
	for(i = 0; i < eof_import_text_events->events; i++)
	{
		if(eof_import_text_events->event[i]->type == 0x01)
		{
			tp = eof_import_midi_to_eof_optimized(eof_import_text_events->event[i]->pos);
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

	/* free MIDI */
	destroy_midi(eof_work_midi);

	eof_changes = 0;
//	eof_setup_menus();
	eof_music_pos = 0;
	eof_selected_track = 0;
	eof_selected_ogg = 0;

	return sp;
}

/* sorter for MIDI events so I can ouput proper MTrk data */
int qsort_helper3(const void * e1, const void * e2)
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

    /* put lyrics first */
    if((*thing1)->type == 0x05 && (*thing2)->type == 0x90)
    {
	    return -1;
    }
    if((*thing1)->type == 0x90 && (*thing2)->type == 0x05)
    {
	    return 1;
    }

    /* put notes first and then markers */
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
		if(eof_song->beat[i]->pos <= pos && eof_song->beat[i + 1]->pos > pos)
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
		for(i = startbeat; i < endbeat && i < eof_song->beats; i++)
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
double eof_calculate_delta(double start, double end)
{
	int i;
	double bpm;
	double tpm = 60.0;
	int startbeat = eof_figure_beat(start);
	int endbeat = eof_figure_beat(end);
	double total_delta = 0.0;
	double portion;

	/* if no BPM change, calculate delta the easy way :) */
	if(!eof_check_bpm_change(start, end))
	{
		bpm = eof_calculate_bpm_absolute(start);
		return ((((end - start) / 1000.0) * 120.0) * bpm) / tpm;
//		return ((double)(end / 10.0 - start / 10.0) * bpm) / tpm;
	}

	/* get first_portion */
	bpm = eof_calculate_bpm_absolute(start);
	portion = ((eof_song->beat[startbeat + 1]->fpos - start) / 1000.0) * 120.0;
	total_delta += (portion * bpm) / tpm;

	/* get rest of the portions */
	for(i = startbeat + 1; i < endbeat; i++)
	{
		bpm = eof_calculate_bpm_absolute(eof_song->beat[i]->fpos);
		portion = ((eof_song->beat[i + 1]->fpos - eof_song->beat[i]->fpos) / 1000.0) * 120.0;
		total_delta += (portion * bpm) / tpm;
	}

	/* get last portion */
	bpm = eof_calculate_bpm_absolute(end);
	portion = ((end - eof_song->beat[endbeat]->fpos) / 1000.0) * 120.0;
	total_delta += (portion * bpm) / tpm;

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
	char header[14] = {'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 1, 0, 1, 0x0, 0x78}; // 120 ticks per beat
	char trackheader[8] = {'M', 'T', 'r', 'k', 0, 0, 0, 0};
	char * tempname[8] = {"eof.tmp", "eof2.tmp", "eof3.tmp", "eof4.tmp", "eof5.tmp", "eof6.tmp", "eof7.tmp", "eof8.tmp"};
	PACKFILE * fp;
	PACKFILE * fp2;
	int i, j;
	unsigned long last_pos = 0;
	double last_fpos = 0.0;
	unsigned long delta = 0;
	unsigned long track_length;
	int midi_note_offset = 0;
	int offset_used = 0;
	unsigned long ppqn;

	double accumulator = 0.0;
	double ddelta;
	double vbpm = (double)60000000.0 / (double)sp->beat[0]->ppqn;
	double tpm = 60.0;
	int vel;

	for(j = 0; j < EOF_MAX_TRACKS; j++)
	{

		/* fill in notes */
		if(sp->track[j]->notes > 0)
		{

			/* clear MIDI events list */
			eof_clear_midi_events();

			/* write the MTrk MIDI data to a temp file
	   		use size of the file as the MTrk header length */
			for(i = 0; i < sp->track[j]->notes; i++)
			{
				switch(sp->track[j]->note[i]->type)
				{
					case EOF_NOTE_AMAZING:
					{
						midi_note_offset = 0x60;
						break;
					}
					case EOF_NOTE_MEDIUM:
					{
						midi_note_offset = 0x54;
						break;
					}
					case EOF_NOTE_EASY:
					{
						midi_note_offset = 0x48;
						break;
					}
					case EOF_NOTE_SUPAEASY:
					{
						midi_note_offset = 0x3C;
						break;
					}
					case EOF_NOTE_SPECIAL:
					{
						midi_note_offset = 120;
						break;
					}
				}

				/* write green note */
				if(sp->track[j]->note[i]->note & 1)
				{
					eof_add_midi_event(sp->track[j]->note[i]->pos, 0x90, midi_note_offset + 0);
				}

				/* write yellow note */
				if(sp->track[j]->note[i]->note & 2)
				{
					eof_add_midi_event(sp->track[j]->note[i]->pos, 0x90, midi_note_offset + 1);
				}

				/* write red note */
				if(sp->track[j]->note[i]->note & 4)
				{
					eof_add_midi_event(sp->track[j]->note[i]->pos, 0x90, midi_note_offset + 2);
				}

				/* write blue note */
				if(sp->track[j]->note[i]->note & 8)
				{
					eof_add_midi_event(sp->track[j]->note[i]->pos, 0x90, midi_note_offset + 3);
				}

				/* write purple note */
				if(sp->track[j]->note[i]->note & 16)
				{
					eof_add_midi_event(sp->track[j]->note[i]->pos, 0x90, midi_note_offset + 4);
				}

				/* write forced HOPO */
				if(sp->track[j]->note[i]->flags & EOF_NOTE_FLAG_F_HOPO)
				{
					eof_add_midi_event(sp->track[j]->note[i]->pos, 0x90, midi_note_offset + 5);
				}

				/* write forced non-HOPO */
				else if(sp->track[j]->note[i]->flags & EOF_NOTE_FLAG_NO_HOPO)
				{
					eof_add_midi_event(sp->track[j]->note[i]->pos, 0x90, midi_note_offset + 6);
				}

				/* write green note off */
				if(sp->track[j]->note[i]->note & 1)
				{
					eof_add_midi_event(sp->track[j]->note[i]->pos + sp->track[j]->note[i]->length, 0x80, midi_note_offset + 0);
				}

				/* write yellow note off */
				if(sp->track[j]->note[i]->note & 2)
				{
					eof_add_midi_event(sp->track[j]->note[i]->pos + sp->track[j]->note[i]->length, 0x80, midi_note_offset + 1);
				}

				/* write red note off */
				if(sp->track[j]->note[i]->note & 4)
				{
					eof_add_midi_event(sp->track[j]->note[i]->pos + sp->track[j]->note[i]->length, 0x80, midi_note_offset + 2);
				}

				/* write blue note off */
				if(sp->track[j]->note[i]->note & 8)
				{
					eof_add_midi_event(sp->track[j]->note[i]->pos + sp->track[j]->note[i]->length, 0x80, midi_note_offset + 3);
				}

				/* write purple note off */
				if(sp->track[j]->note[i]->note & 16)
				{
					eof_add_midi_event(sp->track[j]->note[i]->pos + sp->track[j]->note[i]->length, 0x80, midi_note_offset + 4);
				}

				/* write forced HOPO note off */
				if(sp->track[j]->note[i]->flags & EOF_NOTE_FLAG_F_HOPO)
				{
					eof_add_midi_event(sp->track[j]->note[i]->pos + sp->track[j]->note[i]->length, 0x80, midi_note_offset + 5);
				}

				/* write forced non-HOPO note off */
				else if(sp->track[j]->note[i]->flags & EOF_NOTE_FLAG_NO_HOPO)
				{
					eof_add_midi_event(sp->track[j]->note[i]->pos + sp->track[j]->note[i]->length, 0x80, midi_note_offset + 6);
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

			last_pos = sp->tags->ogg[eof_selected_ogg].midi_offset;
    		qsort(eof_midi_event, eof_midi_events, sizeof(EOF_MIDI_EVENT *), qsort_helper3);
//			allegro_message("break1");

			/* open the file */
			fp = pack_fopen(tempname[j], "w");
			if(!fp)
			{
				return 0;
			}

			/* write the track name */
			WriteVarLen(0, fp);
			pack_putc(0xff, fp);
			pack_putc(0x03, fp);
			WriteVarLen(ustrlen(eof_track_name[j]), fp);
			pack_fwrite(eof_track_name[j], ustrlen(eof_track_name[j]), fp);

	    	/* add MIDI events */
			for(i = 0; i < eof_midi_events; i++)
			{
				ddelta = eof_calculate_delta(last_pos, eof_midi_event[i]->pos);
				delta = ddelta;
				accumulator += (ddelta - (double)delta);
				if(accumulator > 1.0)
				{
					delta += 1;
					accumulator -= 1.0;
				}

				last_pos = eof_midi_event[i]->pos;
//				vel = eof_midi_event[i]->type == 0x80 ? 0x40 : 0x64;
				vel = 0x64;
				WriteVarLen(delta, fp);
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
		}
	}

	/* make vocals track */
	if(sp->vocal_track->lyrics > 0)
	{

		/* clear MIDI events list */
		eof_clear_midi_events();

		/* write the MTrk MIDI data to a temp file
		use size of the file as the MTrk header length */
		for(i = 0; i < sp->vocal_track->lyrics; i++)
		{
			eof_add_midi_lyric_event(sp->vocal_track->lyric[i]->pos, sp->vocal_track->lyric[i]->text);
			if(sp->vocal_track->lyric[i]->note > 0)
			{
				eof_add_midi_event(sp->vocal_track->lyric[i]->pos, 0x90, sp->vocal_track->lyric[i]->note);
				eof_add_midi_event(sp->vocal_track->lyric[i]->pos + sp->vocal_track->lyric[i]->length, 0x80, sp->vocal_track->lyric[i]->note);
			}
		}
		/* fill in lyric lines */
		for(i = 0; i < sp->vocal_track->lines; i++)
		{
			eof_add_midi_event(sp->vocal_track->line[i].start_pos, 0x90, 105);
			eof_add_midi_event(sp->vocal_track->line[i].end_pos, 0x80, 105);
			if(sp->vocal_track->line[i].flags & EOF_LYRIC_LINE_FLAG_OVERDRIVE)
			{
				eof_add_midi_event(sp->vocal_track->line[i].start_pos, 0x90, 116);
				eof_add_midi_event(sp->vocal_track->line[i].end_pos, 0x80, 116);
			}
		}

		last_pos = sp->tags->ogg[eof_selected_ogg].midi_offset;
		qsort(eof_midi_event, eof_midi_events, sizeof(EOF_MIDI_EVENT *), qsort_helper3);

		/* open the file */
		fp = pack_fopen(tempname[7], "w");
		if(!fp)
		{
			return 0;
		}

		/* write the track name */
		WriteVarLen(0, fp);
		pack_putc(0xff, fp);
		pack_putc(0x03, fp);
		WriteVarLen(ustrlen(eof_track_name[EOF_TRACK_VOCALS]), fp);
		pack_fwrite(eof_track_name[EOF_TRACK_VOCALS], ustrlen(eof_track_name[EOF_TRACK_VOCALS]), fp);

		/* add MIDI events */
		for(i = 0; i < eof_midi_events; i++)
		{
			ddelta = eof_calculate_delta(last_pos, eof_midi_event[i]->pos);
			delta = ddelta;
			accumulator += (ddelta - (double)delta);
			if(accumulator > 1.0)
			{
				delta += 1;
				accumulator -= 1.0;
			}

			last_pos = eof_midi_event[i]->pos;
			vel = 0x64;
			WriteVarLen(delta, fp);
			if(eof_midi_event[i]->type == 0x05)
			{
				pack_putc(0xFF, fp);
				pack_putc(0x05, fp);
				pack_putc(ustrlen(eof_midi_event[i]->dp), fp);
				pack_fwrite(eof_midi_event[i]->dp, ustrlen(eof_midi_event[i]->dp), fp);
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

	/* write tempo track */
	fp = pack_fopen(tempname[5], "w");
	if(!fp)
	{
		return 0;
	}
	last_pos = sp->tags->ogg[eof_selected_ogg].midi_offset;
	last_fpos = last_pos;
	offset_used = 0;
	accumulator = 0.0;
	ppqn = 1;
	for(i = 0; i < sp->beats; i++)
	{

		/* if BPM change occurs, write a tempo change */
		if(sp->beat[i]->ppqn != ppqn)
		{
			vbpm = (double)60000000.0 / (double)ppqn;
			ddelta = ((((sp->beat[i]->fpos - last_fpos) / 1000.0) * 120.0) * vbpm) / tpm;
			delta = ddelta;
			accumulator += (ddelta - (double)delta);
			if(accumulator > 1.0)
			{
				delta += 1;
				accumulator -= 1.0;
			}

			last_fpos = sp->beat[i]->fpos;
			ppqn = sp->beat[i]->ppqn;

			WriteVarLen(delta, fp);
			pack_putc(0xff, fp);
			pack_putc(0x51, fp);
			pack_putc(0x03, fp);
			pack_putc((sp->beat[i]->ppqn & 0xFF0000) >> 16, fp);
			pack_putc((sp->beat[i]->ppqn & 0xFF00) >> 8, fp);
			pack_putc((sp->beat[i]->ppqn & 0xFF), fp);
		}
	}
	WriteVarLen(0, fp);
	pack_putc(0xFF, fp);
	pack_putc(0x2F, fp);
	pack_putc(0x00, fp);
	pack_fclose(fp);

	/* track name is "EVENTS"
	   event = 0xFF, meta = 0x01
	   length = string length */
	/* use delta calculator from the note tracks */

	if(sp->text_events)
	{

		eof_sort_events();

		/* open the file */
		fp = pack_fopen(tempname[6], "w");
		if(!fp)
		{
			return 0;
		}

		/* write the track name */
		WriteVarLen(0, fp);
		pack_putc(0xff, fp);
		pack_putc(0x03, fp);
		WriteVarLen(ustrlen("EVENTS"), fp);
		pack_fwrite("EVENTS", ustrlen("EVENTS"), fp);

		last_pos = sp->tags->ogg[0].midi_offset;

    	/* add MIDI events */
		for(i = 0; i < sp->text_events; i++)
		{
			ddelta = eof_calculate_delta(last_pos, sp->beat[sp->text_event[i]->beat]->fpos);
			delta = ddelta;
			accumulator += (ddelta - (double)delta);
			if(accumulator > 1.0)
			{
				delta += 1;
				accumulator -= 1.0;
			}

			last_pos = sp->beat[sp->text_event[i]->beat]->pos;
			WriteVarLen(delta, fp);
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
		return 0;
	}

	/* write header data */
	header[11] = eof_count_tracks() + 1 + (sp->text_events > 0 ? 1 : 0) + (sp->vocal_track->lyrics > 0 ? 1 : 0);
	pack_fwrite(header, 14, fp);

	/* write tempo track */
	track_length = file_size_ex(tempname[5]);
	fp2 = pack_fopen(tempname[5], "r");
	if(!fp2)
	{
		pack_fclose(fp);
		return 0;
	}
	pack_fwrite(trackheader, 4, fp);
	pack_mputl(track_length, fp);
	for(i = 0; i < track_length; i++)
	{
		pack_putc(pack_getc(fp2), fp);
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

	/* write lyric track if there are any lyrics */
	if(sp->vocal_track->lyrics)
	{
		track_length = file_size_ex(tempname[7]);
		fp2 = pack_fopen(tempname[7], "r");
		if(!fp2)
		{
			pack_fclose(fp);
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

	return 1;
}
