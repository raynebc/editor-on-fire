#include <allegro.h>
#include "main.h"
#include "dialog.h"
#include "note.h"
#include "beat.h"
#include "midi.h"
#include "utility.h"
#include "menu/note.h"	//For pitch macros

#define EOF_MIDI_TIMER_FREQUENCY  40

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
    if(((*thing1)->type == 0x05) && ((*thing2)->type == 0x90))
    {
	    return -1;
    }
    if(((*thing1)->type == 0x90) && ((*thing2)->type == 0x05))
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
		return ((((end - start) / 1000.0) * EOF_TIME_DIVISION) * bpm) / tpm;
//		return ((double)(end / 10.0 - start / 10.0) * bpm) / tpm;
	}

	/* get first_portion */
	bpm = eof_calculate_bpm_absolute(start);
	portion = ((eof_song->beat[startbeat + 1]->fpos - start) / 1000.0) * EOF_TIME_DIVISION;
	total_delta += (portion * bpm) / tpm;

	/* get rest of the portions */
	for(i = startbeat + 1; i < endbeat; i++)
	{
		bpm = eof_calculate_bpm_absolute(eof_song->beat[i]->fpos);
		portion = ((eof_song->beat[i + 1]->fpos - eof_song->beat[i]->fpos) / 1000.0) * EOF_TIME_DIVISION;
		total_delta += (portion * bpm) / tpm;
	}

	/* get last portion */
	bpm = eof_calculate_bpm_absolute(end);
	portion = ((end - eof_song->beat[endbeat]->fpos) / 1000.0) * EOF_TIME_DIVISION;
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
				delta = ddelta + 0.5;	//Round up to nearest delta

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
			delta = ddelta + 0.5;	//Round up to nearest delta

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
	ppqn = 1;
	for(i = 0; i < sp->beats; i++)
	{

		/* if BPM change occurs, write a tempo change */
		if(sp->beat[i]->ppqn != ppqn)
		{
			vbpm = (double)60000000.0 / (double)ppqn;
			ddelta = ((((sp->beat[i]->fpos - last_fpos) / 1000.0) * EOF_TIME_DIVISION) * vbpm) / tpm;
			delta = ddelta + 0.5;	//Round up to nearest delta

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
			delta = ddelta + 0.5;	//Round up to nearest delta

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
