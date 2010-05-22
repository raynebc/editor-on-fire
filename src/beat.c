#include "main.h"
#include "beat.h"

//EOF_BEAT_MARKER eof_song->beat[EOF_MAX_BEATS];
//int eof_song->beats = 0;

int eof_get_beat(unsigned long pos)
{
	int i;

	if(pos >= eof_music_length)
	{
		return -1;
	}
	for(i = 0; i < eof_song->beats; i++)
	{
		if(eof_song->beat[i]->pos > pos)
		{
			return i - 1;
		}
	}
	if(pos >= eof_song->beat[eof_song->beats - 1]->pos)
	{
		return eof_song->beats - 1;
	}
	return 0;
}

int eof_get_beat_length(int beat)
{
	if(beat < eof_song->beats - 1)
	{
		return eof_song->beat[beat + 1]->pos - eof_song->beat[beat]->pos;
	}
	else
	{
		return eof_song->beat[eof_song->beats - 1]->pos - eof_song->beat[eof_song->beats - 2]->pos;
	}
}

void eof_calculate_beats(void)
{
	int i;
	double curpos = 0.0;
	double beat_length;
	int cbeat = 0;

	/* correct BPM if it hasn't been set at all */
	if(eof_song->beats <= 0)
	{
		beat_length = (double)60000 / ((double)60000000.0 / (double)500000.0);	//Default beat length is 500ms, which reflects a tempo of 120BPM
		while(curpos < eof_music_length)
		{
			eof_song_add_beat(eof_song);
			eof_song->beat[eof_song->beats - 1]->ppqn = 500000;
			eof_song->beat[eof_song->beats - 1]->fpos = (double)eof_song->tags->ogg[eof_selected_ogg].midi_offset + curpos;
			eof_song->beat[eof_song->beats - 1]->pos = eof_song->beat[eof_song->beats - 1]->fpos +0.5;	//Round up
			curpos += beat_length;
		}
		return;
	}

	eof_song->beat[0]->fpos = (double)eof_song->tags->ogg[eof_selected_ogg].midi_offset;
	eof_song->beat[0]->pos = eof_song->beat[0]->fpos +0.5;	//Round up

	/* calculate the beat length */
	beat_length = (double)60000 / ((double)60000000.0 / (double)eof_song->beat[0]->ppqn);
	for(i = 1; i < eof_song->beats; i++)
	{
		curpos += beat_length;
		eof_song->beat[i]->fpos = (double)eof_song->tags->ogg[eof_selected_ogg].midi_offset + curpos;
		eof_song->beat[i]->pos = eof_song->beat[i]->fpos +0.5;	//Round up

		/* bpm changed */
		if(eof_song->beat[i]->ppqn != eof_song->beat[i - 1]->ppqn)
		{
			beat_length = (double)60000 / ((double)60000000.0 / (double)eof_song->beat[i]->ppqn);
		}
//		curpos += beat_length;
//		cbeat = i;	//If this for loop is reached, eof_song->beats is at least 1, allowing this to be set once outside the loop
	}
	cbeat = eof_song->beats - 1;	//The index of the last beat in the beat[] array
	curpos += beat_length;
	while(eof_song->tags->ogg[eof_selected_ogg].midi_offset + curpos < eof_music_length)
	{
		eof_song_add_beat(eof_song);
		eof_song->beat[eof_song->beats - 1]->ppqn = eof_song->beat[cbeat]->ppqn;
		eof_song->beat[eof_song->beats - 1]->fpos = (double)eof_song->tags->ogg[eof_selected_ogg].midi_offset + curpos;
		eof_song->beat[eof_song->beats - 1]->pos = eof_song->beat[eof_song->beats - 1]->fpos +0.5;	//Round up
		curpos += beat_length;
	}
	for(i = eof_song->beats - 1; i >= 0; i--)
	{
		if(eof_song->beat[i]->pos <= (double)eof_music_length)
		{
			break;
		}
		else
		{
			eof_song_delete_beat(eof_song, i);
		}
	}
}

/*	Unused function
void eof_change_bpm(int cbeat, unsigned long ppqn)
{
	int next_anchor = eof_find_next_anchor(cbeat);
	double old_beat_length = (double)60000 / ((double)60000000.0 / (double)eof_song->beat[cbeat]->ppqn);
	double beat_length = (double)60000 / ((double)60000000.0 / (double)ppqn);
	double beats;
	double offset;
	int i;

	if(next_anchor < 0)
	{
		return;
	}
	beats = next_anchor - cbeat;
	offset = (old_beat_length * beats) - (beat_length * beats);
	for(i = next_anchor; i < eof_song->beats; i++)
	{
		eof_song->beat[i]->pos += offset;
	}
	for(i = cbeat + 1; i < cbeat + beats; i++)
	{
		eof_song->beat[i]->pos = eof_song->beat[cbeat]->pos + (double)(i - cbeat) * beat_length;
	}
}
*/

int eof_beat_is_anchor(int cbeat)
{
	if(cbeat >= EOF_MAX_BEATS)	//Bounds check
		return 0;

	if(cbeat <= 0)
	{
		return 1;
	}
	else if(eof_song->beat[cbeat]->flags & EOF_BEAT_FLAG_ANCHOR)
	{
		return 1;
	}
	else if(eof_song->beat[cbeat]->ppqn != eof_song->beat[cbeat - 1]->ppqn)
	{
		return 1;
	}
	return 0;
}

int eof_find_previous_anchor(int cbeat)
{
	int beat = cbeat;

	if(cbeat >= EOF_MAX_BEATS)	//Bounds check
		return 0;

	while(beat >= 0)
	{
		beat--;
		if(eof_song->beat[beat]->flags & EOF_BEAT_FLAG_ANCHOR)
		{
			return beat;
		}
	}
	return 0;
}

int eof_find_next_anchor(int cbeat)
{
	int beat = cbeat;

	if(cbeat >= EOF_MAX_BEATS)	//Bounds check
		return 0;

	while(beat < eof_song->beats - 1)
	{
		beat++;
		if(eof_song->beat[beat]->flags & EOF_BEAT_FLAG_ANCHOR)
		{
			return beat;
		}
	}
	return -1;
}

void eof_realign_beats(int cbeat)
{
	int i;
	int last_anchor = eof_find_previous_anchor(cbeat);
	int next_anchor = eof_find_next_anchor(cbeat);
	int beats = 0;
	int count = 1;
	double beats_length;
	double newbpm;
	double newppqn;

	if(next_anchor < 0)
	{
		next_anchor = eof_song->beats;
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
	beats_length = eof_song->beat[next_anchor]->pos - eof_song->beat[last_anchor]->pos;
	newbpm = (double)60000 / (beats_length / (double)beats);
	newppqn = (double)60000000 / newbpm;

	eof_song->beat[last_anchor]->ppqn = newppqn;

	/* replace beat markers */
	for(i = last_anchor; i < next_anchor - 1; i++)
	{
		eof_song->beat[i + 1]->pos = eof_song->beat[last_anchor]->pos + (beats_length / (double)beats) * (double)count;
		eof_song->beat[i + 1]->ppqn = eof_song->beat[last_anchor]->ppqn;
		count++;
	}
}

void eof_recalculate_beats(int cbeat)
{
	int i;
	int last_anchor = eof_find_previous_anchor(cbeat);
	int next_anchor = eof_find_next_anchor(cbeat);
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
	beats_length = eof_song->beat[cbeat]->pos - eof_song->beat[last_anchor]->pos;
	newbpm = (double)60000.0 / (beats_length / (double)beats);
	newppqn = (double)60000000.0 / newbpm;

	eof_song->beat[last_anchor]->ppqn = newppqn;

	/* replace beat markers */
	for(i = last_anchor; i < cbeat - 1; i++)
	{
		eof_song->beat[i + 1]->fpos = eof_song->beat[last_anchor]->fpos + (beats_length / (double)beats) * (double)count;
		eof_song->beat[i + 1]->pos = eof_song->beat[i + 1]->fpos +0.5;	//Round up
		eof_song->beat[i + 1]->ppqn = eof_song->beat[last_anchor]->ppqn;
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

		beats_length = eof_song->beat[next_anchor]->pos - eof_song->beat[cbeat]->pos;
		newbpm = (double)60000 / (beats_length / (double)beats);
		newppqn = (double)60000000 / newbpm;

		eof_song->beat[cbeat]->ppqn = newppqn;

		count = 1;
		/* replace beat markers */
		for(i = cbeat; i < next_anchor - 1; i++)
		{
			eof_song->beat[i + 1]->fpos = eof_song->beat[cbeat]->pos + (beats_length / (double)beats) * (double)count;
			eof_song->beat[i + 1]->pos = eof_song->beat[i + 1]->fpos +0.5;	//Round up
			eof_song->beat[i + 1]->ppqn = eof_song->beat[cbeat]->ppqn;
			count++;
		}
	}
	else
	{
		for(i = cbeat + 1; i < eof_song->beats; i++)
		{
			eof_song->beat[i]->pos += eof_mickeys_x * eof_zoom;
			eof_song->beat[i]->fpos = eof_song->beat[i]->pos;
		}
	}
}
