#include <assert.h>
#include "beat.h"
#include "main.h"
#include "midi.h"
#include "rs.h"
#include "undo.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

int eof_beat_stats_cached = 0;	//Tracks whether the cached statistics for the projects beats is current (should be reset after each load, import, undo, redo or beat operation)

long eof_get_beat(EOF_SONG * sp, unsigned long pos)
{
	long i;

	eof_log("eof_get_beat() entered", 2);

	if(!sp)
	{
		return -1;
	}
	if(pos > eof_chart_length)
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

unsigned long eof_get_beat_length(EOF_SONG * sp, unsigned long beat)
{
	eof_log("eof_get_beat_length() entered", 2);

	if(!sp)
	{
		return 0;
	}
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
	unsigned long cbeat = 0;
	unsigned long target_length = eof_music_length;

	eof_log("eof_calculate_beats() entered", 1);

	if(!sp)
	{
		return;
	}

	if(eof_chart_length > eof_music_length)
	{
		target_length = eof_chart_length;
	}

	/* correct BPM if it hasn't been set at all */
	if(sp->beats == 0)
	{
		beat_length = (double)60000.0 / ((double)60000000.0 / (double)500000.0);	//Default beat length is 500ms, which reflects a tempo of 120BPM
		while(curpos < (double)target_length + beat_length)
		{	//While there aren't enough beats to cover the length of the chart, add beats
			if(eof_song_add_beat(sp))
			{	//If the beat was successfully added
				assert(sp->beats > 0);	//Put an assertion here to resolve a false positive with Coverity
				sp->beat[sp->beats - 1]->ppqn = 500000;
				sp->beat[sp->beats - 1]->fpos = (double)sp->tags->ogg[eof_selected_ogg].midi_offset + curpos;
				sp->beat[sp->beats - 1]->pos = sp->beat[sp->beats - 1]->fpos + 0.5;	//Round up
				curpos += beat_length;
			}
		}
		return;
	}

	sp->beat[0]->fpos = (double)sp->tags->ogg[eof_selected_ogg].midi_offset;
	sp->beat[0]->pos = sp->beat[0]->fpos + 0.5;	//Round up

	/* calculate the beat length */
	beat_length = (double)60000 / ((double)60000000.0 / (double)sp->beat[0]->ppqn);
	sp->beat[0]->flags |= EOF_BEAT_FLAG_ANCHOR;	//The first beat is always an anchor
	for(i = 1; i < sp->beats; i++)
	{
		curpos += beat_length;
		sp->beat[i]->fpos = (double)sp->tags->ogg[eof_selected_ogg].midi_offset + curpos;
		sp->beat[i]->pos = sp->beat[i]->fpos + 0.5;	//Round up

		/* bpm changed */
		if(sp->beat[i]->ppqn != sp->beat[i - 1]->ppqn)
		{
			sp->beat[i]->flags |= EOF_BEAT_FLAG_ANCHOR;	//Set the anchor flag
			beat_length = (double)60000 / ((double)60000000.0 / (double)sp->beat[i]->ppqn);
		}
	}
	cbeat = sp->beats - 1;	//The index of the last beat in the beat[] array
	curpos += beat_length;
	while(sp->tags->ogg[eof_selected_ogg].midi_offset + curpos < target_length + beat_length)
	{	//While there aren't enough beats to cover the length of the chart, add beats
		if(eof_song_add_beat(sp))
		{	//If the beat was successfully added
			sp->beat[sp->beats - 1]->ppqn = sp->beat[cbeat]->ppqn;
			sp->beat[sp->beats - 1]->fpos = (double)sp->tags->ogg[eof_selected_ogg].midi_offset + curpos;
			sp->beat[sp->beats - 1]->pos = sp->beat[sp->beats - 1]->fpos +0.5;	//Round up
			curpos += beat_length;
		}
	}
}

void eof_calculate_tempo_map(EOF_SONG * sp)
{
	unsigned long ctr, lastppqn = 0;

	if(!sp || (sp->beats < 3))
		return;

	for(ctr = 0; ctr < sp->beats - 1; ctr++)
	{	//For each beat in the chart
		sp->beat[ctr]->ppqn = 1000 * (sp->beat[ctr + 1]->pos - sp->beat[ctr]->pos);	//Calculate the tempo of the beat by getting its length
		if(!lastppqn || (lastppqn != sp->beat[ctr]->ppqn))
		{	//If the tempo is being changed at this beat, or this is the first beat
			sp->beat[ctr]->flags |= EOF_BEAT_FLAG_ANCHOR;	//Set the anchor flag
			lastppqn = sp->beat[ctr]->ppqn;
		}
		else
		{
			sp->beat[ctr]->flags &= ~EOF_BEAT_FLAG_ANCHOR;	//Clear the anchor flag
		}
	}
	sp->beat[sp->beats - 1]->ppqn = sp->beat[sp->beats - 2]->ppqn;	//The last beat's tempo is the same as the previous beat's
}

int eof_beat_is_anchor(EOF_SONG * sp, int cbeat)
{
	eof_log("eof_beat_is_anchor() entered", 2);

	if(cbeat >= EOF_MAX_BEATS)	//Bounds check
		return 0;

	if(!sp)
	{
		return 0;
	}
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
		eof_log("eof_beat_is_anchor() detected a beat flag error", 1);
		return 1;
	}
	return 0;
}

unsigned long eof_find_previous_anchor(EOF_SONG * sp, unsigned long cbeat)
{
	unsigned long beat = cbeat;

//	eof_log("eof_find_previous_anchor() entered");

	if(cbeat >= EOF_MAX_BEATS)	//Bounds check
	{
		return 0;
	}
	if(!sp)
	{
		return 0;
	}
	while(beat > 1)
	{
		beat--;
		if(sp->beat[beat]->flags & EOF_BEAT_FLAG_ANCHOR)
		{
			return beat;
		}
	}
	return 0;
}

long eof_find_next_anchor(EOF_SONG * sp, unsigned long cbeat)
{
	unsigned long beat = cbeat;

//	eof_log("eof_find_next_anchor() entered");

	if(cbeat >= EOF_MAX_BEATS)	//Bounds check
	{
		return 0;
	}
	if(!sp)
	{
		return 0;
	}
	while(beat < sp->beats - 1)
	{
		beat++;
		assert(beat < EOF_MAX_BEATS);	//Put an assertion here to resolve a false positive with Coverity
		if(sp->beat[beat]->flags & EOF_BEAT_FLAG_ANCHOR)
		{
			return beat;
		}
	}
	return -1;
}

void eof_realign_beats(EOF_SONG * sp, int cbeat)
{
	long i, last_anchor, next_anchor, beats = 0, count = 1;
	double beats_length;
	double newbpm;
	double newppqn;

	eof_log("eof_realign_beats() entered", 1);
	if(!sp)
	{
		return;
	}
	last_anchor = eof_find_previous_anchor(sp, cbeat);
	next_anchor = eof_find_next_anchor(sp, cbeat);
	if(next_anchor < 0)
	{
		next_anchor = sp->beats;
	}
	if(last_anchor >= next_anchor)
	{	//Avoid a division by zero or negative number
		return;
	}
	beats = next_anchor - last_anchor;	//The number of beats between the previous and next anchor

	/* figure out what the new BPM should be */
	beats_length = sp->beat[next_anchor]->pos - sp->beat[last_anchor]->pos;
	newbpm = (double)60000.0 / (beats_length / (double)beats);
	newppqn = (double)60000000.0 / newbpm;

	sp->beat[last_anchor]->ppqn = newppqn;

	/* replace beat markers */
	for(i = last_anchor; i < next_anchor - 1; i++)
	{
		sp->beat[i + 1]->fpos = sp->beat[last_anchor]->fpos + (beats_length / (double)beats) * (double)count;
		sp->beat[i + 1]->pos = sp->beat[i + 1]->fpos + 0.5;	//Round up to nearest ms
		sp->beat[i + 1]->ppqn = sp->beat[last_anchor]->ppqn;
		count++;
	}
}

void eof_recalculate_beats(EOF_SONG * sp, int cbeat)
{
	int i;
	int last_anchor = eof_find_previous_anchor(sp, cbeat);
	long next_anchor = eof_find_next_anchor(sp, cbeat);
	int beats = 0;
	int count = 1;
	double beats_length;
	double newbpm;
	double newppqn;

	eof_log("eof_recalculate_beats() entered", 2);	//Only log this if verbose logging is on

	if(cbeat >= EOF_MAX_BEATS)	//Bounds check
	{
		return;
	}
	if(!sp)
	{
		return;
	}
	if(last_anchor < cbeat)
	{
		beats=cbeat - last_anchor;	//The number of beats between the previous anchor and the specified beat
	}

	/* figure out what the new BPM should be */
	beats_length = sp->beat[cbeat]->fpos - sp->beat[last_anchor]->fpos;
	if(!beats_length || !beats)
		return;	//Error condition
	newbpm = (double)60000.0 / (beats_length / (double)beats);
	newppqn = (double)60000000.0 / newbpm;

	sp->beat[last_anchor]->ppqn = newppqn;

	/* replace beat markers */
	for(i = last_anchor; i < cbeat - 1; i++)
	{	//For all beats from the previous anchor up to the beat that was moved
		sp->beat[i + 1]->fpos = sp->beat[last_anchor]->fpos + (beats_length / (double)beats) * (double)count;
		sp->beat[i + 1]->pos = sp->beat[i + 1]->fpos + 0.5;	//Round up
		sp->beat[i + 1]->ppqn = sp->beat[last_anchor]->ppqn;
		count++;
	}

	/* move rest of markers */
	if(next_anchor >= 0)
	{	//If there is another anchor, adjust all beat timings up until that anchor
		beats = 0;
		if(cbeat < next_anchor)
			beats=next_anchor - cbeat;	//The number of beats between the specified beat and the next anchor

		beats_length = sp->beat[next_anchor]->fpos - sp->beat[cbeat]->fpos;
		if(!beats_length || !beats)
			return;	//Error condition
		newbpm = (double)60000 / (beats_length / (double)beats);
		newppqn = (double)60000000 / newbpm;

		sp->beat[cbeat]->ppqn = newppqn;

		count = 1;
		/* replace beat markers */
		for(i = cbeat; i < next_anchor - 1; i++)
		{
			assert(i + 1< EOF_MAX_BEATS);	//Put an assertion here to resolve a false positive with Coverity
			sp->beat[i + 1]->fpos = sp->beat[cbeat]->fpos + (beats_length / (double)beats) * (double)count;
			sp->beat[i + 1]->pos = sp->beat[i + 1]->fpos + 0.5;	//Round up
			sp->beat[i + 1]->ppqn = sp->beat[cbeat]->ppqn;
			count++;
		}
	}
	else
	{	//Otherwise adjust beat timings for all remaining beats in the project
		for(i = cbeat + 1; i < sp->beats; i++)
		{
			assert(i < EOF_MAX_BEATS);	//Put an assertion here to resolve a false positive with Coverity
			sp->beat[i]->fpos += eof_mickeys_x * eof_zoom;
			sp->beat[i]->pos = sp->beat[i]->fpos + 0.5;	//Round up
		}
	}
}

EOF_BEAT_MARKER * eof_song_add_beat(EOF_SONG * sp)
{
 	eof_log("eof_song_add_beat() entered", 2);

 	if(sp == NULL)
		return NULL;

	if(sp->beats < EOF_MAX_BEATS)
	{	//If the maximum number of beats hasn't already been defined
		sp->beat[sp->beats] = malloc(sizeof(EOF_BEAT_MARKER));
		if(sp->beat[sp->beats] != NULL)
		{
			sp->beat[sp->beats]->pos = 0;
			sp->beat[sp->beats]->ppqn = 500000;
			sp->beat[sp->beats]->flags = 0;
			sp->beat[sp->beats]->midi_pos = 0;
			sp->beat[sp->beats]->key = 0;
			sp->beat[sp->beats]->fpos = 0.0;
			sp->beat[sp->beats]->measurenum = 0;
			sp->beat[sp->beats]->beat_within_measure = -1;
			sp->beat[sp->beats]->num_beats_in_measure = 0;
			sp->beat[sp->beats]->beat_unit = 0;
			sp->beat[sp->beats]->contained_section_event = -1;
			sp->beat[sp->beats]->contains_tempo_change = 0;
			sp->beat[sp->beats]->contains_ts_change = 0;
			sp->beat[sp->beats]->contains_end_event = 0;
			sp->beats++;
			return sp->beat[sp->beats - 1];
		}
	}

	return NULL;
}

void eof_song_delete_beat(EOF_SONG * sp, unsigned long beat)
{
	unsigned long i;

 	eof_log("eof_song_delete_beat() entered", 2);

	if(sp)
	{
		free(sp->beat[beat]);
		for(i = beat; i < sp->beats - 1; i++)
		{
			sp->beat[i] = sp->beat[i + 1];
		}
		sp->beats--;
	}
}

int eof_song_resize_beats(EOF_SONG * sp, unsigned long beats)
{
	unsigned long i;
	unsigned long oldbeats;

	if(!sp)
	{
		return 0;
	}

	oldbeats = sp->beats;
	if(beats > oldbeats)
	{
		for(i = oldbeats; i < beats; i++)
		{
			if(!eof_song_add_beat(sp))
			{
				return 0;	//Return failure
			}
		}
	}
	else if(beats < oldbeats)
	{
		for(i = beats; i < oldbeats; i++)
		{
			free(sp->beat[i]);
			sp->beats--;
		}
	}

	return 1;	//Return success
}

int eof_song_append_beats(EOF_SONG * sp, unsigned long beats)
{
	unsigned long i;
	double beat_length;

	if(!sp)
	{
		return 0;
	}

	beat_length = (double)60000.0 / ((double)60000000.0 / (double)sp->beat[sp->beats - 1]->ppqn);	//Get the length of the current last beat
	for(i = 0; i < beats; i++)
	{
		if(!eof_song_add_beat(sp))
		{
			return 0;	//Return failure
		}
		sp->beat[sp->beats - 1]->ppqn = sp->beat[sp->beats - 2]->ppqn;		//Set this beat's tempo to match the previous beat
		sp->beat[sp->beats - 1]->fpos = sp->beat[sp->beats - 2]->fpos + beat_length;	//Set this beat's position to one beat length after the previous beat
		sp->beat[sp->beats - 1]->pos = sp->beat[sp->beats - 1]->fpos + 0.5;	//Round up
	}

	return 1;	//Return success
}

void eof_double_tempo(EOF_SONG * sp, unsigned long beat, char *undo_made)
{
	unsigned long i, ppqn;

 	eof_log("eof_double_tempo() entered", 1);

	if(!sp || (beat >= sp->beats))
		return;	//Invalid parameters

	if(!(undo_made && (*undo_made != 0)))
	{	//If the calling function didn't explicitly opt to skip making an undo state
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		if(undo_made)
			*undo_made = 1;
	}
	sp->beat[beat]->flags |= EOF_BEAT_FLAG_ANCHOR;	//Ensure this beat is an anchor
	ppqn = sp->beat[beat]->ppqn;	//Store this beat's tempo for reference
	do
	{
		sp->beat[beat]->ppqn /= 2;	//Double the tempo on this beat
		if(eof_song_add_beat(sp) == NULL)
		{	//If a new beat couldn't be created
			(void) eof_undo_apply();	//Undo this failed operation
			allegro_message("Failed to double tempo.  Operation canceled.");
			return;
		}
		for(i = sp->beats - 1; i > beat; i--)
		{	//For each beat after the selected beat, in reverse order
			memcpy(sp->beat[i], sp->beat[i - 1], sizeof(EOF_BEAT_MARKER));
		}
		sp->beat[beat + 1]->flags = 0;				//Clear the flags of the new beat
		sp->beat[beat + 1]->ppqn = sp->beat[beat]->ppqn;	//Copy the current beat's tempo to the new beat
		for(i = 0; i < sp->text_events; i++)
		{	//For each text event
			if(sp->text_event[i]->beat >= beat + 1)	//If the event is at the added beat or after
			{
				sp->text_event[i]->beat++;			//Move it forward one beat
				sp->beat[sp->text_event[i]->beat]->flags |= EOF_BEAT_FLAG_EVENTS;	//Ensure that beat is marked as having a text event
			}
		}

		beat++;	//Skip past the next beat, which was just created
		beat++;	//Iterate to the next beat
	}while((beat < sp->beats) && (sp->beat[beat]->ppqn == ppqn));	//Continue until all beats have been processed or a tempo change is reached

	eof_calculate_beats(sp);	//Set the beats' timestamps based on their tempo changes
	eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current
}

int eof_halve_tempo(EOF_SONG * sp, unsigned long beat, char *undo_made)
{
	unsigned long ctr, ppqn, i;
	char changefound = 0;	//This will be set to nonzero if during beat counting, a tempo change is reached
	int retval = 0;	//This will be -1 if an odd number of beats were affected, indicating the user may need to manually alter a tempo

 	eof_log("eof_halve_tempo() entered", 1);

	if(!sp || (beat >= sp->beats))
		return 0;	//Invalid parameters

	ppqn = sp->beat[beat]->ppqn;	//Store this beat's tempo for reference
	for(ctr = 1; beat + ctr < sp->beats; ctr++)
	{	//Count the number of beats until the end of chart is reached
		if(sp->beat[beat + ctr]->ppqn != ppqn)
		{	//If a tempo change is found, break from counting
			changefound = 1;
			break;
		}
	}

	if(ctr < 2)
		return -1;	//If no beats can be modified without destroying the tempo map, return

	if(!(undo_made && (*undo_made != 0)))
	{	//If the calling function didn't explicitly opt to skip making an undo state
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		if(undo_made)
			*undo_made = 1;
	}
	if(changefound && (ctr & 1))
	{	//If the tempo changes an odd number beats away from the starting beat
		sp->beat[beat + ctr - 1]->flags |= EOF_BEAT_FLAG_ANCHOR;	//Make the last beat before the change an anchor
		retval = -1;
		ctr--;	//And remove it from the set of beats to be processed
	}

	do{
		sp->beat[beat]->ppqn *= 2;	//Halve the tempo on this beat
		eof_song_delete_beat(sp, beat + 1);	//Delete the next beat
		for(i = 0; i < sp->text_events; i++)
		{	//For each text event
			if(sp->text_event[i]->beat >= beat + 1)	//If the event is at the deleted beat or after
			{
				sp->text_event[i]->beat--;			//Move it back one beat
				sp->beat[sp->text_event[i]->beat]->flags |= EOF_BEAT_FLAG_EVENTS;	//Ensure that beat is marked as having a text event
			}
		}

		ctr -= 2;	//Two beats have been processed
		beat++;		//Iterate to the next beat to alter
	}while((beat < sp->beats) && (ctr > 0));	//Continue until all applicable beats have been processed

	eof_calculate_beats(sp);	//Set the beats' timestamps based on their tempo changes
	eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current
	return retval;
}

unsigned long eof_get_measure(unsigned long measure, unsigned char count_only)
{
	unsigned long i, beat_counter = 0, measure_counter = 0;
	unsigned beats_per_measure = 0;
	char first_measure = 0;	//Set to nonzero when the first measure marker is reached

	if(!eof_song)
		return 0;

	for(i = 0; i < eof_song->beats; i++)
	{
		if(eof_get_ts(eof_song,&beats_per_measure,NULL,i) == 1)
		{	//If this beat has a time signature change
			first_measure = 1;	//Note that a time signature change has been found
			beat_counter = 0;
		}
		if(first_measure && (beat_counter == 0))
		{	//If there was a TS change or the beat markers incremented enough to reach the next measure
			measure_counter++;
			if(!count_only && (measure_counter == measure))
			{	//If this function was asked to find a measure, and it has been found
				return i;
			}
		}
		beat_counter++;
		if(beat_counter >= beats_per_measure)
		{
			beat_counter = 0;
		}
	}
	if(count_only)
	{	//If this function was asked to count the number of measures
		return measure_counter;
	}

	return 0;	//The measure was not found
}

void eof_process_beat_statistics(EOF_SONG * sp, unsigned long track)
{
	unsigned long current_ppqn = 0;
	int beat_counter = -1;
	unsigned beats_per_measure = 0;
	unsigned beat_unit = 0;
	unsigned long measure_counter = 0;
	char first_measure = 0;	//Set to nonzero when the first measure marker is reached
	unsigned long i;

	eof_log("eof_process_beat_statistics() entered", 2);

	if(!sp)
		return;

	for(i = 0; i < sp->beats; i++)
	{	//For each beat
		if(eof_get_ts(sp, &beats_per_measure, &beat_unit, i) == 1)
		{	//If this beat has a time signature change
			first_measure = 1;	//Note that a time signature change has been found
			beat_counter = 0;
			sp->beat[i]->contains_ts_change = 1;
		}
		else
		{
			sp->beat[i]->contains_ts_change = 0;
		}
		if(first_measure && (beat_counter == 0))
		{	//If there was a TS change or the beat markers incremented enough to reach the next measure
			measure_counter++;
		}

	//Cache beat statistics
		sp->beat[i]->measurenum = measure_counter;
		sp->beat[i]->beat_within_measure = beat_counter;
		sp->beat[i]->beat_unit = beat_unit;
		sp->beat[i]->num_beats_in_measure = beats_per_measure;
		if(sp->beat[i]->ppqn != current_ppqn)
		{	//If this beat contains a tempo change
			current_ppqn = sp->beat[i]->ppqn;
			sp->beat[i]->contains_tempo_change = 1;
		}
		else
		{
			sp->beat[i]->contains_tempo_change = 0;
		}
		sp->beat[i]->contained_section_event = -1;		//Reset this until the beat is found to contain a section event
		sp->beat[i]->contains_end_event = 0;			//Reset this boolean status
		sp->beat[i]->contained_rs_section_event = sp->beat[i]->contained_rs_section_event_instance_number = -1;	//Reset these until the beat is found to contain a Rocksmith section

		if(sp->beat[i]->flags & EOF_BEAT_FLAG_EVENTS)
		{	//If this beat has one or more text events
			unsigned long ctr, count;

			for(ctr = 0; ctr < sp->text_events; ctr++)
			{	//For each text event
				if(sp->text_event[ctr]->beat == i)
				{	//If the event is assigned to this beat
					if(eof_is_section_marker(sp->text_event[ctr], track))
					{	//If the text event's string or flags indicate a section marker (from the perspective of the specified track)
						sp->beat[i]->contained_section_event = ctr;
						break;	//And break from the loop
					}
					else if(!ustrcmp(sp->text_event[ctr]->text, "[end]"))
					{	//If this is the [end] event
						sp->beat[i]->contains_end_event = 1;
						break;
					}
				}
			}

			for(ctr = 0; ctr < sp->text_events; ctr++)
			{	//For each text event
				if(sp->text_event[ctr]->beat == i)
				{	//If the event is assigned to this beat
					count = eof_get_rs_section_instance_number(sp, track, ctr);	//Determine if this event is a Rocksmith section, and if so, which instance number it is
					if(count)
					{	//If the event is assigned to this beat
						sp->beat[i]->contained_rs_section_event = ctr;
						sp->beat[i]->contained_rs_section_event_instance_number = count;
						break;
					}
				}
			}
		}

	//Update beat counter
		beat_counter++;
		if(beat_counter >= beats_per_measure)
		{
			beat_counter = 0;
		}
	}//For each beat

	eof_beat_stats_cached = 1;
}
