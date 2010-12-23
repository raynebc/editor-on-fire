#include "main.h"
#include "beat.h"

//EOF_BEAT_MARKER sp->beat[EOF_MAX_BEATS];
//int sp->beats = 0;

long eof_get_beat(EOF_SONG * sp, unsigned long pos)
{
	long i;

	if(pos >= eof_music_length)
	{
		return -1;
	}
	for(i = 0; i < sp->beats; i++)
	{
		if(sp->beat[i]->pos > pos)
		{
			return i - 1;
		}
	}
	if(pos >= sp->beat[sp->beats - 1]->pos)
	{
		return sp->beats - 1;
	}
	return 0;
}

int eof_get_beat_length(EOF_SONG * sp, int beat)
{
	if(beat < sp->beats - 1)
	{
		return sp->beat[beat + 1]->pos - sp->beat[beat]->pos;
	}
	else
	{
		return sp->beat[sp->beats - 1]->pos - sp->beat[sp->beats - 2]->pos;
	}
}

void eof_calculate_beats(EOF_SONG * sp)
{
	unsigned long i;
	double curpos = 0.0;
	double beat_length;
	int cbeat = 0;

	/* correct BPM if it hasn't been set at all */
	if(sp->beats <= 0)
	{
		beat_length = (double)60000 / ((double)60000000.0 / (double)500000.0);	//Default beat length is 500ms, which reflects a tempo of 120BPM
		while(curpos < eof_music_length)
		{
			eof_song_add_beat(eof_song);
			sp->beat[sp->beats - 1]->ppqn = 500000;
			sp->beat[sp->beats - 1]->fpos = (double)sp->tags->ogg[eof_selected_ogg].midi_offset + curpos;
			sp->beat[sp->beats - 1]->pos = sp->beat[sp->beats - 1]->fpos +0.5;	//Round up
			curpos += beat_length;
		}
		return;
	}

	sp->beat[0]->fpos = (double)sp->tags->ogg[eof_selected_ogg].midi_offset;
	sp->beat[0]->pos = sp->beat[0]->fpos +0.5;	//Round up

	/* calculate the beat length */
	beat_length = (double)60000 / ((double)60000000.0 / (double)sp->beat[0]->ppqn);
	for(i = 1; i < sp->beats; i++)
	{
		curpos += beat_length;
		sp->beat[i]->fpos = (double)sp->tags->ogg[eof_selected_ogg].midi_offset + curpos;
		sp->beat[i]->pos = sp->beat[i]->fpos +0.5;	//Round up

		/* bpm changed */
		if(sp->beat[i]->ppqn != sp->beat[i - 1]->ppqn)
		{
			beat_length = (double)60000 / ((double)60000000.0 / (double)sp->beat[i]->ppqn);
		}
//		curpos += beat_length;
//		cbeat = i;	//If this for loop is reached, sp->beats is at least 1, allowing this to be set once outside the loop
	}
	cbeat = sp->beats - 1;	//The index of the last beat in the beat[] array
	curpos += beat_length;
	while(sp->tags->ogg[eof_selected_ogg].midi_offset + curpos < eof_music_length)
	{
		eof_song_add_beat(eof_song);
		sp->beat[sp->beats - 1]->ppqn = sp->beat[cbeat]->ppqn;
		sp->beat[sp->beats - 1]->fpos = (double)sp->tags->ogg[eof_selected_ogg].midi_offset + curpos;
		sp->beat[sp->beats - 1]->pos = sp->beat[sp->beats - 1]->fpos +0.5;	//Round up
		curpos += beat_length;
	}
	for(i = sp->beats; i > 0; i--)
	{
		if(sp->beat[i-1]->pos <= (double)eof_music_length)
		{
			break;
		}
		else
		{
			eof_song_delete_beat(eof_song, i-1);
		}
	}
}

int eof_beat_is_anchor(EOF_SONG * sp, int cbeat)
{
	if(cbeat >= EOF_MAX_BEATS)	//Bounds check
		return 0;

	if(cbeat <= 0)
	{
		return 1;
	}
	else if(sp->beat[cbeat]->flags & EOF_BEAT_FLAG_ANCHOR)
	{
		return 1;
	}
	else if(sp->beat[cbeat]->ppqn != sp->beat[cbeat - 1]->ppqn)
	{
		return 1;
	}
	return 0;
}

unsigned long eof_find_previous_anchor(EOF_SONG * sp, unsigned long cbeat)
{
	unsigned long beat = cbeat;

	if(cbeat >= EOF_MAX_BEATS)	//Bounds check
		return 0;

	do{
		beat--;
		if(sp->beat[beat]->flags & EOF_BEAT_FLAG_ANCHOR)
		{
			return beat;
		}
	}while(beat > 0);
	return 0;
}

long eof_find_next_anchor(EOF_SONG * sp, unsigned long cbeat)
{
	unsigned long beat = cbeat;

	if(cbeat >= EOF_MAX_BEATS)	//Bounds check
		return 0;

	while(beat < sp->beats - 1)
	{
		beat++;
		if(sp->beat[beat]->flags & EOF_BEAT_FLAG_ANCHOR)
		{
			return beat;
		}
	}
	return -1;
}

void eof_realign_beats(EOF_SONG * sp, int cbeat)
{
	int i;
	int last_anchor = eof_find_previous_anchor(sp, cbeat);
	int next_anchor = eof_find_next_anchor(sp, cbeat);
	int beats = 0;
	int count = 1;
	double beats_length;
	double newbpm;
	double newppqn;

	if(next_anchor < 0)
	{
		next_anchor = sp->beats;
	}

	/* count beats */
/*	for(i = last_anchor; i < next_anchor; i++)
	{
		beats++;
	}
*/
	if(last_anchor < next_anchor)
		beats=next_anchor - last_anchor;	//The number of beats between the previous and next anchor

	/* figure out what the new BPM should be */
	beats_length = sp->beat[next_anchor]->pos - sp->beat[last_anchor]->pos;
	newbpm = (double)60000 / (beats_length / (double)beats);
	newppqn = (double)60000000 / newbpm;

	sp->beat[last_anchor]->ppqn = newppqn;

	/* replace beat markers */
	for(i = last_anchor; i < next_anchor - 1; i++)
	{
		sp->beat[i + 1]->pos = sp->beat[last_anchor]->pos + (beats_length / (double)beats) * (double)count;
		sp->beat[i + 1]->ppqn = sp->beat[last_anchor]->ppqn;
		count++;
	}
}

void eof_recalculate_beats(EOF_SONG * sp, int cbeat)
{
	int i;
	int last_anchor = eof_find_previous_anchor(sp, cbeat);
	int next_anchor = eof_find_next_anchor(sp, cbeat);
	int beats = 0;
	int count = 1;
	double beats_length;
	double newbpm;
	double newppqn;

	/* count beats */
/*	for(i = last_anchor; i < cbeat; i++)
	{
		beats++;
	}
*/
	if(cbeat >= EOF_MAX_BEATS)	//Bounds check
		return;

	if(last_anchor < cbeat)
		beats=cbeat - last_anchor;	//The number of beats between the previous anchor and the specified beat

	/* figure out what the new BPM should be */
	beats_length = sp->beat[cbeat]->pos - sp->beat[last_anchor]->pos;
	newbpm = (double)60000.0 / (beats_length / (double)beats);
	newppqn = (double)60000000.0 / newbpm;

	sp->beat[last_anchor]->ppqn = newppqn;

	/* replace beat markers */
	for(i = last_anchor; i < cbeat - 1; i++)
	{
		sp->beat[i + 1]->fpos = sp->beat[last_anchor]->fpos + (beats_length / (double)beats) * (double)count;
		sp->beat[i + 1]->pos = sp->beat[i + 1]->fpos +0.5;	//Round up
		sp->beat[i + 1]->ppqn = sp->beat[last_anchor]->ppqn;
		count++;
	}

	/* move rest of markers */
	if(next_anchor >= 0)
	{
		beats = 0;
		/* count beats */
/*		for(i = cbeat; i < next_anchor; i++)
		{
			beats++;
		}
*/
		if(cbeat < next_anchor)
			beats=next_anchor - cbeat;	//The number of beats between the specified beat and the next anchor

		beats_length = sp->beat[next_anchor]->pos - sp->beat[cbeat]->pos;
		newbpm = (double)60000 / (beats_length / (double)beats);
		newppqn = (double)60000000 / newbpm;

		sp->beat[cbeat]->ppqn = newppqn;

		count = 1;
		/* replace beat markers */
		for(i = cbeat; i < next_anchor - 1; i++)
		{
			sp->beat[i + 1]->fpos = sp->beat[cbeat]->pos + (beats_length / (double)beats) * (double)count;
			sp->beat[i + 1]->pos = sp->beat[i + 1]->fpos +0.5;	//Round up
			sp->beat[i + 1]->ppqn = sp->beat[cbeat]->ppqn;
			count++;
		}
	}
	else
	{
		for(i = cbeat + 1; i < sp->beats; i++)
		{
			sp->beat[i]->pos += eof_mickeys_x * eof_zoom;
			sp->beat[i]->fpos = sp->beat[i]->pos;
		}
	}
}
