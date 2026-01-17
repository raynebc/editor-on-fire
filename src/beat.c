#include <assert.h>
#include <math.h>
#include <ctype.h>
#include "beat.h"
#include "main.h"
#include "midi.h"
#include "rs.h"
#include "song.h"
#include "undo.h"
#include "menu/edit.h"	//For auto-adjust functions

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

int eof_beat_stats_cached = 0;	//Tracks whether the cached statistics for the projects beats is current (should be reset after each load, import, undo, redo or beat operation)

///Disabled for now until a more useful purpose for this feature is developed
int eof_skip_mid_beats_in_measure_numbering = 0;	//Tracks whether beats inserted for mid beat tempo or TS changes should be ignored when counting beats and measures
													//This should remain 0 until after any warnings related to mid beat changes are displayed at the end of file imports
													//This should also be set to 0 during the time eof_save_helper_checks() is running so that it can warn about interrupted measures, etc.

unsigned long eof_get_beat(EOF_SONG * sp, unsigned long pos)
{
	unsigned long i;

//	eof_log("eof_get_beat() entered", 2);

	if(!sp)
	{
		return ULONG_MAX;
	}
	if(pos > eof_chart_length)
	{
		return ULONG_MAX;
	}
	if(!sp->beats)
	{
		return ULONG_MAX;
	}
	if(pos < sp->beat[0]->pos)
	{
		return ULONG_MAX;
	}
	for(i = 1; i < sp->beats; i++)
	{	//For each beat after the first
		if(sp->beat[i]->pos > pos)
		{	//If this beat is after the specified position
			return i - 1;	//If is within the previous beat
		}
	}
	if(pos >= sp->beat[sp->beats - 1]->pos)
	{
		return sp->beats - 1;
	}
	return 0;
}

unsigned long eof_get_nearest_beat(EOF_SONG * sp, unsigned long pos)
{
	unsigned long beat;

	if(!sp)
		return ULONG_MAX;			//Invalid parameter

	beat = eof_get_beat(sp, pos);	//Find which beat the specified position is in
	if(beat == ULONG_MAX)			//If that couldn't be determined
		return ULONG_MAX;			//Return error

	if(beat + 1 >= sp->beats)
		return beat;	//If there is no next beat, this is the beat closes to the specified position

	if(sp->beat[beat + 1]->pos - pos < pos - sp->beat[beat]->pos)
		return beat + 1;	//If the next beat is closer to the specified position, return it

	return beat;	//The beat immediately at/before the specified position is the closes one
}

double eof_get_beat_length(EOF_SONG * sp, unsigned long beat)
{
//	eof_log("eof_get_beat_length() entered", 3);

	if(!sp || (beat >= sp->beats) || (sp->beats < 2))
	{	//Invalid paramters
		return 0;
	}
	if(beat < sp->beats - 1)
	{
		return sp->beat[beat + 1]->fpos - sp->beat[beat]->fpos;
	}

	return sp->beat[sp->beats - 1]->fpos - sp->beat[sp->beats - 2]->fpos;
}

unsigned long eof_calculate_beats_logic(EOF_SONG * sp, int addbeats)
{
	unsigned long i;
	double curpos = 0.0;
	double beat_length;
	unsigned long cbeat = 0;
	unsigned long target_length = eof_music_length;
	unsigned num = 4, den = 4, lastden = 4;
	unsigned long beats_added = 0;

	eof_log("eof_calculate_beats() entered", 3);

	if(!sp)
	{
		return 0;
	}

	if(eof_chart_length > eof_music_length)
	{
		target_length = eof_chart_length;
	}

	/* correct BPM if it hasn't been set at all */
	if(sp->beats == 0)
	{
		beat_length = 60000.0 / (60000000.0 / 500000.0);	//Default beat length is 500ms, which reflects a tempo of 120BPM in 4/4 meter
		while(curpos < (double)target_length + beat_length)
		{	//While there aren't enough beats to cover the length of the chart, add beats
			if(!eof_song_append_beats(sp, 1))	//If a beat couldn't be appended
				return beats_added;
			curpos += beat_length;
			beats_added++;
		}
		return beats_added;
	}

	sp->beat[0]->fpos = (double)sp->tags->ogg[0].midi_offset;
	sp->beat[0]->pos = sp->beat[0]->fpos + 0.5;	//Round up

	/* calculate the beat length */
	beat_length = eof_calc_beat_length(sp, 0);
	sp->beat[0]->flags |= EOF_BEAT_FLAG_ANCHOR;	//The first beat is always an anchor
	for(i = 1; i < sp->beats; i++)
	{	//For each beat
		curpos += beat_length;
		sp->beat[i]->fpos = (double)sp->tags->ogg[0].midi_offset + curpos;
		sp->beat[i]->pos = sp->beat[i]->fpos + 0.5;	//Round up

		/* bpm changed */
		if(sp->beat[i]->ppqn != sp->beat[i - 1]->ppqn)
		{
			sp->beat[i]->flags |= EOF_BEAT_FLAG_ANCHOR;	//Set the anchor flag
		}
		/* TS denominator changed */
		(void) eof_get_ts(sp, &num, &den, i);	//Lookup any time signature defined at the beat
		if(den != lastden)
		{
			sp->beat[i]->flags |= EOF_BEAT_FLAG_ANCHOR;	//Set the anchor flag
		}
		lastden = den;	//Track the TS denominator in use
		beat_length = eof_calc_beat_length(sp, i);	//Recalculate the beat length every beat because either a time signature change or a tempo change will alter it
	}
	if(addbeats)
	{	//If the calling function wanted to add beats to extend to reflect eof_chart_length
		cbeat = sp->beats - 1;	//The index of the last beat in the beat[] array
		curpos += beat_length;
		while(sp->tags->ogg[0].midi_offset + curpos < target_length + beat_length)
		{	//While there aren't enough beats to cover the length of the chart, add beats
			if(eof_song_add_beat(sp))
			{	//If the beat was successfully added
				sp->beat[sp->beats - 1]->ppqn = sp->beat[cbeat]->ppqn;
				sp->beat[sp->beats - 1]->fpos = (double)sp->tags->ogg[0].midi_offset + curpos;
				sp->beat[sp->beats - 1]->pos = sp->beat[sp->beats - 1]->fpos +0.5;	//Round up
				curpos += beat_length;
				beats_added++;
			}
			else
			{
				eof_log("\teof_calculate_beats() failed", 1);
				return beats_added;
			}
		}
	}
	if(eof_chart_length < sp->beat[sp->beats - 1]->pos)
	{	//If the chart length needs to be updated to reflect the beat map making the chart longer
		eof_chart_length = sp->beat[sp->beats - 1]->pos;
	}

	return beats_added;
}

unsigned long eof_calculate_beats(EOF_SONG * sp)
{
	return eof_calculate_beats_logic(sp, 1);
}

void eof_calculate_tempo_map(EOF_SONG * sp)
{
	unsigned long ctr, lastppqn = 0;
	unsigned num = 4, den = 4, lastden = 4;
	int has_ts_change;
	double ppqn;

	if(!sp || (sp->beats < 3))
		return;

	for(ctr = 0; ctr < sp->beats - 1; ctr++)
	{	//For each beat in the chart
		has_ts_change = eof_get_ts(sp, &num, &den, ctr);	//Lookup any time signature defined at the beat
		ppqn = 1000.0 * (sp->beat[ctr + 1]->fpos - sp->beat[ctr]->fpos);	//Calculate the tempo of the beat by getting its length (this is the formula "beat_length = 60000 / BPM" rewritten to solve for ppqn)
		if(sp->tags->accurate_ts && (den != 4))
		{	//If the accurate time signatures song property is enabled, and the time signature necessitates adjustment (isn't #/4)
			ppqn *= (double)den / 4.0;	//Adjust for the time signature
		}
		sp->beat[ctr]->ppqn = ppqn + 0.5;	//Round to nearest integer
		if(!lastppqn || (lastppqn != sp->beat[ctr]->ppqn))
		{	//If the tempo is being changed at this beat, or this is the first beat
			sp->beat[ctr]->flags |= EOF_BEAT_FLAG_ANCHOR;	//Set the anchor flag
			lastppqn = sp->beat[ctr]->ppqn;
		}
		else if(has_ts_change && (den != lastden))
		{	//If the time signature denominator changes
			sp->beat[ctr]->flags |= EOF_BEAT_FLAG_ANCHOR;	//Set the anchor flag
		}
		else
		{
			sp->beat[ctr]->flags &= ~EOF_BEAT_FLAG_ANCHOR;	//Clear the anchor flag
		}
		lastden = den;	//Track the TS denominator in use
	}
	sp->beat[sp->beats - 1]->ppqn = sp->beat[sp->beats - 2]->ppqn;	//The last beat's tempo is the same as the previous beat's
}

double eof_calculate_beat_pos_by_prev_beat_tempo(EOF_SONG *sp, unsigned long beat)
{
	unsigned num = 4, den = 4;
	double length;

	if(!sp || (beat >= sp->beats))
		return 0.0;	//Invalid parameters

	if(!beat)
	{	//If the first beat was specified
		return sp->tags->ogg[0].midi_offset;	//Return the current OGG profile's MIDI delay (first beat position)
	}

	length = (double)sp->beat[beat - 1]->ppqn / 1000.0;	//Calculate the length of the previous beat from its tempo (this is the formula "beat_length = 60000 / BPM", where BPM = 60000000 / ppqn)
	(void) eof_get_effective_ts(sp, &num, &den, beat - 1, 0);	//Get the time signature in effect at the previous beat
	if(sp->tags->accurate_ts && (den != 4))
	{	//If the user enabled the accurate time signatures song property, and the time signature necessitates adjustment (isn't #/4)
		length *= 4.0 / (double)den;	//Adjust for the time signature
	}

	return sp->beat[beat - 1]->fpos + length;
}

int eof_detect_tempo_map_corruption(EOF_SONG *sp, int report)
{
	unsigned long ctr, expected_pos_int;
	double expected_pos;
	int corrupt = 0;

	if(!sp)
		return 1;	//Invalid parameters

	for(ctr = 0; ctr < sp->beats; ctr++)
	{	//For each beat
		expected_pos = eof_calculate_beat_pos_by_prev_beat_tempo(sp, ctr);
		expected_pos_int = expected_pos + 0.5;	//Round to nearest ms

		if((expected_pos_int != sp->beat[ctr]->pos) && (fabs(expected_pos - sp->beat[ctr]->fpos) > 0.5))
		{	//If this beat's timestamp doesn't accurately reflect the previous beat's tempo
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "The tempo map is corrupt beginning with beat #%lu (defined position is %fms, %lums, expected is %fms, %lums)", ctr, sp->beat[ctr]->fpos, sp->beat[ctr]->pos, expected_pos, expected_pos_int);
			eof_log(eof_log_string, 1);
			corrupt = 1;

			if(sp->tags->tempo_map_locked)
			{	//If the chart's tempo map is locked
				allegro_message("%s\n\nHowever the tempo map is locked.  To correct this, disable the \"Song>Lock tempo map\" option and re-run this check with \"Beat>Validate tempo map\".", eof_log_string);
				break;
			}
			else
			{
				if(alert(eof_log_string, NULL, "Recreate tempo changes based on beat positions?", "&Yes", "&No", 'y', 'n') == 1)
				{	//If the user opts to correct the tempo map
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					if(sp->beat[0]->pos != sp->tags->ogg[0].midi_offset)
					{	//If the first beat's position doesn't reflect the MIDI delay of the current OGG profile
						sp->beat[0]->pos = sp->beat[0]->fpos = sp->tags->ogg[0].midi_offset;
					}
					eof_calculate_tempo_map(sp);
					return 1;
				}
				else
				{
					break;
				}
			}
		}
	}

	if(!corrupt && report)
	{
		allegro_message("Tempo map is valid");
	}

	return 1;
}

void eof_change_accurate_ts(EOF_SONG * sp, char function)
{
	unsigned long ctr, lastppqn = 0;
	unsigned num = 4, den = 4;

	if(!sp || (sp->beats < 3))
		return;

	for(ctr = 0; ctr < sp->beats - 1; ctr++)
	{	//For each beat in the chart
		(void) eof_get_ts(sp, &num, &den, ctr);	//Lookup any time signature defined at the beat
		if(den != 4)
		{	//If a denominator other than 4 is in effect, change the tempo
			sp->beat[ctr]->ppqn = 1000 * (sp->beat[ctr + 1]->pos - sp->beat[ctr]->pos);	//Calculate the tempo of the beat by getting its length (this is the formula "beat_length = 60000 / BPM" rewritten to solve for ppqn)
			if(function)
			{	//If the time signature is to be observed
				sp->beat[ctr]->ppqn *= (double)den / 4.0;	//Adjust for the time signature
			}
			if(!lastppqn || (lastppqn != sp->beat[ctr]->ppqn))
			{	//If the tempo is being changed at this beat, or this is the first beat
				sp->beat[ctr]->flags |= EOF_BEAT_FLAG_ANCHOR;	//Set the anchor flag
			}
		}
		lastppqn = sp->beat[ctr]->ppqn;	//Track the tempo in effect
	}
	sp->beat[sp->beats - 1]->ppqn = sp->beat[sp->beats - 2]->ppqn;	//The last beat's tempo is the same as the previous beat's
}

int eof_beat_is_anchor(EOF_SONG * sp, unsigned long cbeat)
{
	eof_log("eof_beat_is_anchor() entered", 3);

	if(cbeat >= EOF_MAX_BEATS)	//Bounds check
		return 0;

	if(!sp || (cbeat >= sp->beats))
	{
		return 0;
	}
	if(cbeat == 0)
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
	if(!sp || (cbeat >= sp->beats))
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

unsigned long eof_find_next_anchor(EOF_SONG * sp, unsigned long cbeat)
{
	unsigned long beat = cbeat;

//	eof_log("eof_find_next_anchor() entered");

	if(cbeat >= EOF_MAX_BEATS)	//Bounds check
	{
		return 0;
	}
	if(!sp || (cbeat >= sp->beats))
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
	return ULONG_MAX;
}

void eof_realign_beats(EOF_SONG * sp, unsigned long cbeat)
{
	unsigned long i, last_anchor, next_anchor, beats = 0, count = 1;
	double beats_length;
	double newbpm;
	double newppqn;
	double multiplier = 1.0;	//This is the multiplier used to convert between beat lengths and quarter note lengths, depending on whether the accurate TS chart property is enabled

	eof_log("eof_realign_beats() entered", 3);
	if(!sp)
	{
		return;
	}
	last_anchor = eof_find_previous_anchor(sp, cbeat);
	next_anchor = eof_find_next_anchor(sp, cbeat);
	if(!eof_beat_num_valid(sp, next_anchor))
	{
		next_anchor = sp->beats - 1;
	}
	if(last_anchor >= next_anchor)
	{	//Avoid a division by zero or negative number
		return;
	}
	beats = next_anchor - last_anchor;	//The number of beats between the previous and next anchor

	/* figure out what the new BPM should be */
	beats_length = sp->beat[next_anchor]->fpos - sp->beat[last_anchor]->fpos;
	if(sp->tags->accurate_ts)
	{	//If the user enabled the accurate time signatures song property
		unsigned num = 4, den = 4;
		for(i = 0; i < cbeat; i++)
		{	//For each beat BEFORE the target
			(void) eof_get_ts(sp, &num, &den, i);	//Lookup any time signature defined at the beat
		}
		if(den != 4)
		{	//If the time signature necessitates adjustment (isn't #/4)
			multiplier = (double)den / 4.0;	//Get the beat length that is in effect when the target beat is reached
		}
	}
	newbpm = 60000.0 / (beats_length * multiplier / (double)beats);
	newppqn = 60000000.0 / newbpm;

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

void eof_recalculate_beats(EOF_SONG * sp, unsigned long cbeat)
{
	unsigned long i;
	unsigned long last_anchor = eof_find_previous_anchor(sp, cbeat);
	unsigned long next_anchor = eof_find_next_anchor(sp, cbeat);
	int beats = 0;
	int count = 1;
	double beats_length;
	double newbpm;
	double newppqn;
	double multiplier = 1.0;	//This is the multiplier used to convert between beat lengths and quarter note lengths, depending on whether the accurate TS chart property is enabled

	eof_log("eof_recalculate_beats() entered", 2);	//Only log this if verbose logging is on

	if(cbeat >= EOF_MAX_BEATS)	//Bounds check
	{
		return;
	}
	if(!sp || (cbeat >= sp->beats))
	{
		return;
	}
	if(last_anchor < cbeat)
	{
		beats = cbeat - last_anchor;	//The number of beats between the previous anchor and the specified beat
	}

	/* figure out what the new BPM should be */
	beats_length = sp->beat[cbeat]->fpos - sp->beat[last_anchor]->fpos;
	if((cbeat == last_anchor) || !beats)
		return;	//Error condition
	if(sp->tags->accurate_ts)
	{	//If the user enabled the accurate time signatures song property
		unsigned num = 4, den = 4;
		for(i = 0; i < cbeat; i++)
		{	//For each beat BEFORE the target
			(void) eof_get_ts(sp, &num, &den, i);	//Lookup any time signature defined at the beat
		}
		if(den != 4)
		{	//If the time signature necessitates adjustment (isn't #/4)
			multiplier = (double)den / 4.0;	//Get the beat length that is in effect when the target beat is reached
		}
	}
	newbpm = 60000.0 / (beats_length * multiplier / (double)beats);
	newppqn = 60000000.0 / newbpm;

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
	if(eof_beat_num_valid(sp, next_anchor))
	{	//If there is another anchor, adjust all beat timings up until that anchor
		beats = 0;
		multiplier = 1.0;
		if(cbeat < next_anchor)
			beats = next_anchor - cbeat;	//The number of beats between the specified beat and the next anchor

		beats_length = sp->beat[next_anchor]->fpos - sp->beat[cbeat]->fpos;
		if((next_anchor == cbeat) || !beats)
			return;	//Error condition
		if(sp->tags->accurate_ts)
		{	//If the user enabled the accurate time signatures song property
			unsigned num = 4, den = 4;
			for(i = 0; i < cbeat; i++)
			{	//For each beat BEFORE the target
				(void) eof_get_ts(sp, &num, &den, i);	//Lookup any time signature defined at the beat
			}
			(void) eof_get_ts(sp, &num, &den, cbeat);	//Lookup any time signature change defined at the beat
			if(den != 4)
			{	//If the time signature necessitates adjustment (isn't #/4)
				multiplier = (double)den / 4.0;	//Get the beat length that is in effect when the target beat is reached
			}
		}
		newbpm = 60000.0 / (beats_length * multiplier / (double)beats);	//Re-apply the accurate TS multiplier here if applicable
		newppqn = 60000000.0 / newbpm;

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
// 	eof_log("eof_song_add_beat() entered", 3);

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
			sp->beat[sp->beats]->beat_within_measure = 0;
			sp->beat[sp->beats]->num_beats_in_measure = 0;
			sp->beat[sp->beats]->beat_unit = 0;
			sp->beat[sp->beats]->contained_section_event = -1;
			sp->beat[sp->beats]->contained_rs_section_event = -1;
			sp->beat[sp->beats]->contained_rs_section_event_instance_number = -1;
			sp->beat[sp->beats]->contains_tempo_change = 0;
			sp->beat[sp->beats]->contains_ts_change = 0;
			sp->beat[sp->beats]->contains_end_event = 0;
			sp->beat[sp->beats]->has_ts = 0;
			sp->beats++;
			return sp->beat[sp->beats - 1];
		}
	}
	else
	{
		eof_log("Cannot add another beat.  Limit has been reached.", 1);
	}

	return NULL;
}

void eof_song_delete_beat(EOF_SONG * sp, unsigned long beat)
{
	unsigned long i;

// 	eof_log("eof_song_delete_beat() entered", 3);

	if(sp && (beat < sp->beats))
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
	double beat_length = 500.0;	//If the project has no beats yet, the first appended one will be in 120BPM by default

	if(!sp)
	{
		return 0;
	}

	if(sp->beats)
	{	//If there is at least one beat in the project already
		beat_length = eof_calc_beat_length(sp, sp->beats - 1);	//Get the length of the current last beat
	}
	for(i = 0; i < beats; i++)
	{
		if(!eof_song_add_beat(sp) || (sp->beats < 1))
		{	//If another beat was not added or if there is not at least one beat in the project at this point
			return 0;	//Return failure
		}
		if(sp->beats >= 2)
		{	//If there are at least two beats in the project now
			sp->beat[sp->beats - 1]->ppqn = sp->beat[sp->beats - 2]->ppqn;		//Set this beat's tempo to match the previous beat
			sp->beat[sp->beats - 1]->fpos = sp->beat[sp->beats - 2]->fpos + beat_length;	//Set this beat's position to one beat length after the previous beat
		}
		else
		{	//Otherwise set this beat's tempo to 120BPM
			sp->beat[sp->beats - 1]->ppqn = 500000;
			sp->beat[sp->beats - 1]->fpos = 0;
		}
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
			if(!(sp->text_event[i]->flags & EOF_EVENT_FLAG_FLOATING_POS))
			{	//If this text event is assigned to a beat marker
				if(sp->text_event[i]->pos >= beat + 1)	//If the event is at the added beat or after
				{
					sp->text_event[i]->pos++;			//Move it forward one beat
					sp->beat[sp->text_event[i]->pos]->flags |= EOF_BEAT_FLAG_EVENTS;	//Ensure that beat is marked as having a text event
				}
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
			if(!(sp->text_event[i]->flags & EOF_EVENT_FLAG_FLOATING_POS))
			{	//If this text event is assigned to a beat marker
				if(sp->text_event[i]->pos >= beat + 1)	//If the event is at the deleted beat or after
				{
					sp->text_event[i]->pos--;			//Move it back one beat
					sp->beat[sp->text_event[i]->pos]->flags |= EOF_BEAT_FLAG_EVENTS;	//Ensure that beat is marked as having a text event
				}
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
		if(eof_get_ts(eof_song, &beats_per_measure, NULL, i) == 1)
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
	unsigned beat_counter = 0;
	unsigned beats_per_measure = 0;
	unsigned beat_unit = 0;
	unsigned long measure_counter = 0;
	char first_measure = 0;	//Set to nonzero when the first measure marker is reached
	unsigned long i, ctr, count;
	unsigned curnum = 0, curden = 0;

	eof_log("eof_process_beat_statistics() entered", 3);

	if(!sp)
		return;

	for(i = 0; i < sp->beats; i++)
	{	//For each beat
		//Update measure counter
		if((eof_get_ts(sp, &beats_per_measure, &beat_unit, i) == 1) && ((beats_per_measure != curnum) || (beat_unit != curden)))
		{	//If this beat has a time signature, and it is different from the time signature in effect
			if(!first_measure)
			{	//If this is the first time signature change encountered
				measure_counter = 1;	//This marks the beginning of the first measure
			}
			else
			{	//If this time signature change interrupts a measure in progress
				measure_counter++;	//This marks the start of a new measure
			}
			first_measure = 1;	//Note that a time signature change has been found
			beat_counter = 0;
			sp->beat[i]->contains_ts_change = 1;
			curnum = beats_per_measure;	//Keep track of the current time signature
			curden = beat_unit;
		}
		else
		{
			sp->beat[i]->contains_ts_change = 0;

			if((sp->beat[i]->flags & EOF_BEAT_FLAG_MIDBEAT) && eof_skip_mid_beats_in_measure_numbering)
			{	//If this is a mid beat tempo change and such beats are to be skipped during measure numbering
				if(beat_counter)
				{	//Logic check
					beat_counter--;	//Undo the previous beat's decrement of the beat counter so this mid-beat doesn't count
				}
			}
			else
			{	//Normal beat counting
				if(beat_counter >= beats_per_measure)
				{	//if the beat count has incremented enough to reach the next measure
					beat_counter = 0;
					measure_counter++;
				}
			}
		}

	//Cache beat statistics
		sp->beat[i]->measurenum = measure_counter;
		sp->beat[i]->beat_within_measure = beat_counter;
		sp->beat[i]->beat_unit = beat_unit;
		sp->beat[i]->num_beats_in_measure = beats_per_measure;
		sp->beat[i]->has_ts = first_measure;	//Track whether a TS is in effect
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

		beat_counter++;	//Update beat counter
	}//For each beat

	//Process text events to update the variables tracking the contained section, RS section and end events
	for(ctr = 0; ctr < sp->text_events; ctr++)
	{	//For each text event
		if(!(sp->text_event[ctr]->flags & EOF_EVENT_FLAG_FLOATING_POS))
		{	//If this text event is assigned to a beat marker
			if(eof_is_section_marker(sp->text_event[ctr], track))
			{	//If the text event's string or flags indicate a section marker (from the perspective of the specified track)
				sp->beat[sp->text_event[ctr]->pos]->contained_section_event = ctr;
			}
			else if(!ustrcmp(sp->text_event[ctr]->text, "[end]"))
			{	//If this is the [end] event
				sp->beat[sp->text_event[ctr]->pos]->contains_end_event = 1;
			}

			count = eof_get_rs_section_instance_number(sp, track, ctr);	//Determine if this event is a Rocksmith section, and if so, which instance number it is
			if(count)
			{	//If the event is a Rocksmith section
				sp->beat[sp->text_event[ctr]->pos]->contained_rs_section_event = ctr;
				sp->beat[sp->text_event[ctr]->pos]->contained_rs_section_event_instance_number = count;
			}
		}
	}

	eof_beat_stats_cached = 1;
}

unsigned long eof_beat_contains_mover_rs_phrase_index(EOF_SONG *sp, unsigned long beat, unsigned long track, unsigned long *index)
{
	unsigned long ctr, num = 0;

	if(!sp || (beat >= sp->beats) || (track >= sp->tracks))
		return 0;	//Invalid parameters

	for(ctr = 0; ctr < sp->text_events; ctr++)
	{	//For each text event
		if(!(sp->text_event[ctr]->flags & EOF_EVENT_FLAG_FLOATING_POS) && (sp->text_event[ctr]->pos == beat))
		{	//If this text event is assigned to the specified beat marker
			num = eof_is_mover_phrase(sp, track, sp->text_event[ctr]);	//Determine whether this event is a moveR phrase and if so, get the numeric value from it
			if(num)
			{	//If it was found to be a valid moveR phrase
				if(index)
				{	//If the calling function wanted the index of the event
					*index = ctr;
				}
				return num;
			}
		}
	}

	return 0;	//No mover phrase found on the specified beat
}

unsigned long eof_beat_contains_mover_rs_phrase(EOF_SONG *sp, unsigned long beat, unsigned long track, EOF_TEXT_EVENT **ptr)
{
	unsigned long num, index = 0;

	num = eof_beat_contains_mover_rs_phrase_index(sp, beat, track, &index);

	if(num && ptr)
	{	//If a moveR phrase was found on that beat, and the calling function wanted to receive a pointer to it
		*ptr = sp->text_event[index];	//Store it by reference in the pointer
	}

	return num;	//Return the operand value of the moveR phrase if one was found
}

double eof_get_distance_in_beats(EOF_SONG *sp, unsigned long pos1, unsigned long pos2)
{
	unsigned long temppos;
	unsigned long beat1, beat2;
	double bpos1, bpos2;

	if(!sp)
		return 0.0;	//Invalid parameters

	//Ensure pos1 and pos2 are in ascending order
	if(pos1 > pos2)
	{	//If the positions aren't in order
		temppos = pos1;	//Swap them
		pos1 = pos2;
		pos2 = temppos;
	}

	//Get the beat positions
	beat1 = eof_get_beat(sp, pos1);
	beat2 = eof_get_beat(sp, pos2);
	if((beat1 >= sp->beats) || (beat2 >= sp->beats))
	{	//If the beat containing either position can't be found
		return 0.0;	//Error
	}

	bpos1 = beat1 + (eof_get_porpos_sp(sp, pos1) / 100.0);
	bpos2 = beat2 + (eof_get_porpos_sp(sp, pos2) / 100.0);

	return bpos2 - bpos1;
}

void eof_detect_mid_measure_ts_changes(void)
{
	unsigned long ctr, mid_change_count = 0;
	unsigned suggested_num = 4, suggested_den = 4;

	if(!eof_song || (eof_song->beats < 2))
		return;

	eof_process_beat_statistics(eof_song, eof_selected_track);
	for(ctr = 1; ctr < eof_song->beats; ctr++)
	{	//For each beat after the first
		if(!eof_song->beat[ctr]->contains_ts_change)
			continue;	//If this beat does not have a time signature change, skip it

		if(!eof_song->beat[ctr - 1]->has_ts || (eof_song->beat[ctr - 1]->beat_within_measure == eof_song->beat[ctr - 1]->num_beats_in_measure - 1))
			continue;	//If the previous beat does not have a time signature, or if it was the last beat in its measure (the beat_within_measure stat is numbered starting with 0), skip it

		if(!mid_change_count)
		{	//If this is the first offending time signature change
			suggested_num = eof_song->beat[ctr - 1]->beat_within_measure + 1;	//Track the last beat number in the measure and account for the zero numbering
			for(ctr = ctr - 1; ctr > 0; ctr--)
			{	//For each of the previous beats
				if(eof_song->beat[ctr]->beat_within_measure == 0)
				{	//If this is the first beat in the affected measure
					break;
				}
			}
			suggested_den = eof_song->beat[ctr]->beat_unit;
			eof_selected_beat = ctr;	//Select the affected beat marker
		}
		mid_change_count++;
	}

	if(mid_change_count)
	{	//If there were any offending time signature changes
		eof_beat_stats_cached = 0;
		eof_seek_and_render_position(eof_selected_track, eof_note_type, eof_song->beat[eof_selected_beat]->pos);	//seek to the beat in question and render

		allegro_message("%lu measures are interrupted by a time signature change.\nThis can cause problems in some rhythm games.\nSuggested T/S for this one is %d/%u.", mid_change_count, suggested_num, suggested_den);
	}
}

unsigned long eof_get_text_event_pos_ptr(EOF_SONG *sp, EOF_TEXT_EVENT *ptr)
{
	if(!ptr)
		return ULONG_MAX;	//Invalid parameter

	if(ptr->flags & EOF_EVENT_FLAG_FLOATING_POS)
	{	//The event's position is in milliseconds
		return ptr->pos;
	}
	else
	{	//The event's position refers to its assigned beat number
		if(!eof_beat_num_valid(sp, ptr->pos))
			return ULONG_MAX;	//Invalid beat number

		return sp->beat[ptr->pos]->pos;
	}
}

unsigned long eof_get_text_event_pos(EOF_SONG *sp, unsigned long event)
{
	if(!sp || (event >= sp->text_events))
		return ULONG_MAX;	//Invalid parameters

	return eof_get_text_event_pos_ptr(sp, sp->text_event[event]);
}

double eof_get_text_event_fpos_ptr(EOF_SONG *sp, EOF_TEXT_EVENT *ptr)
{
	if(!ptr)
		return 0.0;	//Invalid parameter

	if(ptr->flags & EOF_EVENT_FLAG_FLOATING_POS)
	{	//The event's position is in milliseconds
		return ptr->pos;
	}
	else
	{	//The event's position refers to its assigned beat number
		if(!eof_beat_num_valid(sp, ptr->pos))
			return 0.0;	//Invalid beat number

		return sp->beat[ptr->pos]->fpos;
	}
}

double eof_get_text_event_fpos(EOF_SONG *sp, unsigned long event)
{
	if(!sp || (event >= sp->text_events))
		return 0.0;	//Invalid parameters

	return eof_get_text_event_fpos_ptr(sp, sp->text_event[event]);
}

unsigned long eof_get_text_event_beat(EOF_SONG *sp, unsigned long event)
{
	unsigned long matchbeat, matchpos;

	if(!sp || (event >= sp->text_events))
		return ULONG_MAX;	//Invalid parameters

	if(sp->text_event[event]->flags & EOF_EVENT_FLAG_FLOATING_POS)
	{	//If this is a floating text event
		matchpos = eof_get_text_event_pos(sp, event);	//Get the position of this event in milliseconds
		matchbeat = eof_get_beat(sp, matchpos);		//Obtain the beat that this event is in
		if(matchbeat >= sp->beats)
		{	//Logic error
			return ULONG_MAX;
		}
	}
	else
	{	//This event is assigned to a beat
		matchbeat = sp->text_event[event]->pos;		//Obtain the assigned beat number
	}

	return matchbeat;
}

void eof_apply_tempo(unsigned long ppqn, unsigned long beatnum, char adjust)
{
	unsigned long ctr;

	if(!eof_song || (beatnum >= eof_song->beats))
		return;	//Invalid parameter

	if(adjust)
	{	//If auto-adjust is to be performed
		(void) eof_menu_edit_cut(0, 1);	//Save auto-adjust data for the entire chart
	}
	eof_song->beat[beatnum]->ppqn = ppqn;	//Update the specified beat's tempo
	if((beatnum > 0) && (eof_song->beat[beatnum]->ppqn != eof_song->beat[beatnum - 1]->ppqn))
	{	//If there is a previous beat and it is a different tempo
		eof_song->beat[beatnum]->flags |= EOF_BEAT_FLAG_ANCHOR;	//Flag the specified beat as an anchor
	}
	for(ctr = beatnum + 1; ctr < eof_song->beats; ctr++)
	{	//For all remaining beats in the project
		if(eof_song->beat[ctr]->flags & EOF_BEAT_FLAG_ANCHOR)	//If this beat is an anchor
			break;	//Stop applying the tempo to beats
		eof_song->beat[ctr]->ppqn = ppqn;	//Update the beat's tempo
	}
	if(adjust)
	{	//If auto-adjust is to be performed
		(void) eof_menu_edit_cut_paste(0, 1);	//Apply auto-adjust data for the entire chart
	}
}

void eof_remove_ts(unsigned long beatnum)
{
//Clear the beat's status except for its anchor and event flags
	unsigned long flags;

	if(!eof_song || !eof_beat_num_valid(eof_song, beatnum))
		return;	//Invalid parameter

	flags = eof_song->beat[beatnum]->flags;
	flags &= ~EOF_BEAT_FLAG_START_4_4;	//Clear this TS flag
	flags &= ~EOF_BEAT_FLAG_START_2_4;	//Clear this TS flag
	flags &= ~EOF_BEAT_FLAG_START_3_4;	//Clear this TS flag
	flags &= ~EOF_BEAT_FLAG_START_5_4;	//Clear this TS flag
	flags &= ~EOF_BEAT_FLAG_START_6_4;	//Clear this TS flag
	flags &= ~EOF_BEAT_FLAG_CUSTOM_TS;	//Clear this TS flag
	flags &= ~0xFF000000;	//Clear any custom TS numerator
	flags &= ~0x00FF0000;	//Clear any custom TS denominator
	eof_song->beat[beatnum]->flags = flags;
}

int eof_check_for_anchors_between_selected_beat_and_seek_pos(void)
{
	unsigned long beat1, beat2;

	if(!eof_song)
		return 1;							//No project loaded
	if(!eof_beat_num_valid(eof_song, eof_selected_beat))
		return 1;							//Logic error

	//Check for the presence of any anchored beat markers between the selected beat and the seek position
	beat1 = beat2 = eof_get_beat(eof_song, eof_music_pos.value - eof_av_delay);
	if(!eof_beat_num_valid(eof_song, beat1) || !eof_beat_num_valid(eof_song, beat2))
		return 1;	//Logic error (seek position may be after the project's last beat)

	if(beat1 == beat2)
		return 0;	//If the beat is only moving forward before the next beat's current position, allow it

	if(eof_selected_beat < beat1)
	{	//If the selected beat is before the seek position
		beat1 = eof_selected_beat;
	}
	else
	{	//The selected beat is at/after the seek position
		beat2 = eof_selected_beat;
	}
	while(beat1 < beat2)
	{	//For each beat between the selected beat's start position and beat containing the end position
		if(beat1 != eof_selected_beat)
		{	//If this isn't the beat which is currently selected (which will move regardless of whether it's already an anchor)
			if(eof_song->beat[beat1]->flags & EOF_BEAT_FLAG_ANCHOR)
			{	//If this beat is an anchor
				return 1;
			}
		}
		beat1++;
	}

	return 0;	//None of the beats were anchors
}
