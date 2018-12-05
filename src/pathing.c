#include "beat.h"
#include "dialog.h"
#include "main.h"
#include "midi.h"
#include "note.h"
#include "pathing.h"
#include "song.h"
#include "undo.h"
#include "utility.h"
#include "agup/agup.h"
#include "modules/g-idle.h"
#include "dialog/proc.h"
#include <math.h>

#ifndef ALLEGRO_WINDOWS
//Mac/Linux builds will use fork()
#include  <sys/types.h>
#endif

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

EOF_SP_PATH_SOLUTION *eof_ch_sp_solution = NULL;	//This is used to store SP pathing and scoring information pertaining to Clone Hero
int eof_ch_sp_solution_macros_wanted = 0;			//This is used to track whether any expansion notes panel macros being displayed need scoring data from the SP pathing logic

int eof_note_is_last_longest_gem(EOF_SONG *sp, unsigned long track, unsigned long note)
{
	unsigned long notepos, index, longestnote;
	long prevnote, nextnote, thislength, longestlength;

	if(!sp || (track >= sp->tracks) || (note >= eof_get_track_size(sp, track)))
		return 0;	//Invalid parameters
	if(!(eof_get_note_eflags(sp, track, note) & EOF_NOTE_EFLAG_DISJOINTED))
		return 1;	//If the note doesn't have disjointed status, it's the longest note at this track difficulty and position

	notepos = eof_get_note_pos(sp, track, note);
	index = note;		//Unless found otherwise, the specified note is the first note at its position
	longestnote = note;	//And it will be considered the longest note at this position
	longestlength = eof_get_note_length(sp, track, note);
	while(1)
	{	//Find the first note in this track difficulty at this note's position
		prevnote = eof_track_fixup_previous_note(sp, track, index);

		if(prevnote < 0)
			break;	//No earlier notes
		if(eof_get_note_pos(sp, track, prevnote) != notepos)
			break;	//No earlier notes at the same position

		index = prevnote;	//Track the earliest note at this position
	}

	while(1)
	{	//Compare length among all notes at this position
		thislength = eof_get_note_length(sp, track, index);
		if(thislength >= longestlength)
		{	//If this note is at least as long as the longest one encountered so far
			longestnote = index;
			longestlength = thislength;
		}

		nextnote = eof_track_fixup_next_note(sp, track, index);

		if(nextnote < 0)
			break;	//No later notes
		if(eof_get_note_pos(sp, track, nextnote) != notepos)
			break;	//No later notes at the same position

		index = nextnote;	//Track the latest note at this position
	}

	if(longestnote == note)	//If no notes at this note's position were longer
		return 1;			//Return true

	return 0;	//Return false
}

unsigned long eof_note_count_gems_extending_to_pos(EOF_SONG *sp, unsigned long track, unsigned long note, unsigned long pos)
{
	unsigned long notepos, index, count = 0, targetlength;
	long prevnote, nextnote;

	if(!sp || (track >= sp->tracks) || (note >= eof_get_track_size(sp, track)))
		return 0;	//Invalid parameters

	notepos = eof_get_note_pos(sp, track, note);
	if(notepos > pos)
		return 0;	//Target position is before that of specified note, there will be no matches

	targetlength = pos - notepos;	//This will be the minimum length of notes to be a match
	index = note;		//Unless found otherwise, the specified note is the first note at its position
	while(1)
	{	//Find the first note in this track difficulty at this note's position
		prevnote = eof_track_fixup_previous_note(sp, track, index);

		if(prevnote < 0)
			break;	//No earlier notes
		if(eof_get_note_pos(sp, track, prevnote) != notepos)
			break;	//No earlier notes at the same position

		index = prevnote;	//Track the earliest note at this position
	}

	while(1)
	{	//Compare length among all notes at this position with the target length
		if(eof_get_note_length(sp, track, index) >= targetlength)
		{	//If this note is long enough to meet the input criteria
			count++;
		}

		nextnote = eof_track_fixup_next_note(sp, track, index);

		if(nextnote < 0)
			break;	//No later notes
		if(eof_get_note_pos(sp, track, nextnote) != notepos)
			break;	//No later notes at the same position
	}

	return count;
}

int eof_note_is_last_in_sp_phrase(EOF_SONG *sp, unsigned long track, unsigned long note)
{
	if(eof_get_note_flags(sp, track, note) & EOF_NOTE_FLAG_SP)
	{	//If this note has star power
		long nextnote = eof_track_fixup_next_note(sp, track, note);

		if(nextnote > 0)
		{	//If there's another note in the same track difficulty
			if(!(eof_get_note_flags(sp, track, nextnote) & EOF_NOTE_FLAG_SP))
			{	//If that next note does not have star power
				return 1;
			}
		}
		else
		{	//The note is the last in its track difficulty
			return 1;
		}
	}

	return 0;
}

unsigned long eof_translate_track_diff_note_index(EOF_SONG *sp, unsigned long track, unsigned char diff, unsigned long index)
{
	unsigned long ctr, tracksize, match;

	if(!sp || (track >= sp->tracks))
		return ULONG_MAX;	//Invalid parameters

	tracksize = eof_get_track_size(sp, track);
	for(ctr = 0, match = 0; ctr < tracksize; ctr++)
	{	//For each note in the specified track
		if(eof_get_note_type(sp, track, ctr) == diff)
		{	//If the note is in the target track difficulty
			if(match == index)
			{	//If this is the note being searched for
				return ctr;
			}
			match++;	//Track how many notes in the target difficulty have been parsed
		}
	}

	return ULONG_MAX;	//No such note was found
}

double eof_get_measure_position(unsigned long pos)
{
	unsigned long beat;
	EOF_BEAT_MARKER *bp;
	double measurepos;
	double beatpos;

	if(!eof_song)
		return 0.0;	//Invalid parameters

	if(!eof_beat_stats_cached)
		eof_process_beat_statistics(eof_song, eof_selected_track);

	beat = eof_get_beat(eof_song, pos);
	if(beat >= eof_song->beats)
		return 0.0;	//Error

	bp = eof_song->beat[beat];
	beatpos = ((double) pos - eof_song->beat[beat]->pos) / eof_get_beat_length(eof_song, beat);		//This is the percentage into its beat the specified position is (only compare the note against the beat's integer position, since the note position will have been rounded in either direction)
	measurepos = ((double) bp->beat_within_measure + beatpos) / bp->num_beats_in_measure;			//This is the percentage into its measure the specified position is
	measurepos += bp->measurenum - 1;																//Add the number of complete measures that are before this position (measurenum is numbered beginning with 1)

	return measurepos;
}

unsigned long eof_get_realtime_position(double pos)
{
	unsigned long ctr, prior_beat_num = ULONG_MAX;
	double prior_beat_pos = 0.0, m_beat_length, m_difference, b_difference, real_pos;

	if(!eof_song)
		return ULONG_MAX;	//Invalid parameters

	if(!eof_beat_stats_cached)
		eof_process_beat_statistics(eof_song, eof_selected_track);

	pos++;	//Increment pos to account for the numbering difference between the pathing logic and the beat statistics

	//Find the beat that is immediately at or before the target position
	for(ctr = 0; ctr < eof_song->beats; ctr++)
	{	//For each beat in the project
		double partial_measure;

		if(eof_song->beat[ctr]->num_beats_in_measure == 0)
			return ULONG_MAX;	//Logic error, avoid division by zero

		partial_measure = (double) eof_song->beat[ctr]->beat_within_measure / eof_song->beat[ctr]->num_beats_in_measure;
		if((double)eof_song->beat[ctr]->measurenum + partial_measure <= pos)
		{	//If this beat's measure position is less than the specified measure position
			prior_beat_num = ctr;	//Remember this beat index
			prior_beat_pos = (double)eof_song->beat[ctr]->measurenum + partial_measure;	//Remember its beat position
		}
		else
		{	//Otherwise this and all subsequent beats are after the target position
			break;
		}
	}
	if(prior_beat_num >= eof_song->beats)
	{	//If no such beat was found
		return ULONG_MAX;	//Return error
	}

	if(eof_song->beat[prior_beat_num]->num_beats_in_measure == 0)
		return ULONG_MAX;	//Logic error, avoid division by zero

	m_beat_length = 1.0 / eof_song->beat[prior_beat_num]->num_beats_in_measure;	//The length of that beat in terms of measures
	m_difference = pos - prior_beat_pos;	//The amount of measures between that beat and the target position
	b_difference = m_difference / m_beat_length;	//Scale that by the beat's length in measures to determine how far into the beat the target position is
	real_pos = eof_song->beat[prior_beat_num]->fpos + (eof_get_beat_length(eof_song, prior_beat_num) * b_difference);	//Translate that position into milliseconds

	return real_pos + 0.5;	//Round to nearest whole millisecond
}

unsigned long eof_ch_pathing_find_next_deployable_sp(EOF_SP_PATH_SOLUTION *solution, unsigned long start_index)
{
	unsigned long ctr, index, sp_count, tracksize;
	double sp_sustain = 0.0;	//The amount of star power sustain that has accumulated

	if(!solution || (solution->track >= eof_song->tracks) || (!solution->note_beat_lengths))
		return ULONG_MAX;	//Invalid parameters

	tracksize = eof_get_track_size(eof_song, solution->track);
	for(ctr = 0, index = 0, sp_count = 0; ctr < tracksize; ctr++)
	{	//For each note in the active track
		if(eof_get_note_type(eof_song, eof_selected_track, ctr) == eof_note_type)
		{	//If the note is in the active difficulty
			if(index >= start_index)
			{	//If the specified note index has been reached, track star power accumulation
				//Count the amount of whammied star power
				if(eof_get_note_length(eof_song, solution->track, ctr) > 1)
				{	//If the note has sustain
					if(eof_get_note_flags(eof_song, solution->track, ctr) & EOF_NOTE_FLAG_SP)
					{	//If the note has star power
						sp_sustain += solution->note_beat_lengths[index];	//Count the number of beats of star power that will be whammied for bonus star power
						while(sp_sustain >= 8.0 - 0.0001)
						{	//For every 8 beats' worth (allowing variance for math error) of star power sustain
							sp_sustain -= 8.0;
							sp_count++;	//Convert it to one star power phrase's worth of star power
						}
					}
				}
				//Count star power phrases to find the first available point of star power deployment
				if(eof_get_note_tflags(eof_song, solution->track, ctr) & EOF_NOTE_TFLAG_SP_END)
				{	//If this is the last note in a star power phrase
					sp_count++;	//Keep count
				}

				if(sp_count >= 2)
				{	//If there are now at least two star power phrase's worth of star power
					return index + 1;	//There is enough star power to deploy, which can be done at the next note
				}
			}
			index++;	//Track the number of notes in the target difficulty that have been encountered
		}
	}

	return ULONG_MAX;	//An applicable note was not found
}

unsigned long eof_ch_pathing_find_next_sp_note(EOF_SP_PATH_SOLUTION *solution, unsigned long start_index)
{
	unsigned long ctr, index, tracksize;

	if(!solution || (solution->track >= eof_song->tracks))
		return ULONG_MAX;	//Invalid parameters

	tracksize = eof_get_track_size(eof_song, solution->track);
	for(ctr = 0, index = 0; ctr < tracksize; ctr++)
	{	//For each note in the active track
		if(eof_get_note_type(eof_song, eof_selected_track, ctr) == eof_note_type)
		{	//If the note is in the active difficulty
			if(index >= start_index)
			{	//If the specified note index has been reached
				if(eof_get_note_flags(eof_song, solution->track, ctr) & EOF_NOTE_FLAG_SP)
				{	//If the note has star power
					return index;
				}
			}
			index++;	//Track the number of notes in the target difficulty that have been encountered
		}
	}

	return ULONG_MAX;	//An applicable note was not found
}

int eof_evaluate_ch_sp_path_solution(EOF_SP_PATH_SOLUTION *solution, EOF_BIG_NUMBER *solution_num, int logging, int sequential)
{
	int sp_deployed = 0;	//Set to nonzero if star power is currently in effect for the note being processed
	unsigned long tracksize, notectr, ctr;
	unsigned long index;	//This will be the note number relative to only the notes in the target track difficulty, used to index into the solution's arrays
	double sp_deployment_start = 0.0;	//Used to track the start position of star power deployment
	double sp_deployment_end = 0.0;	//The measure position at which the current star power deployment will be over (non-inclusive, ie. a note at this position is NOT in the scope of the deployment)
	unsigned long notescore;		//The base score for the current note
	unsigned long notepos, noteflags, tflags, is_solo;
	long notelength;
	int disjointed;			//Set to nonzero if the note being processed has disjointed status, as it requires different handling for star power whammy bonus
	int representative = 0;	//Set to nonzero if the note being processed is the longest and last gem (criteria in that order) in a disjointed chord, and will be examined for star power whammy bonus
	unsigned long base_score, sp_base_score, sustain_score, sp_sustain_score;	//For logging score calculations
	int deploy_sp;

	//Score caching variables
	EOF_SP_PATH_SCORING_STATE score;		//Tracks current scoring information that will be copied to the score cache after each star power deployment ends
	unsigned long cache_number = ULONG_MAX;	//The cached deploy scoring structure selected to resume from in evaluating this solution
	unsigned long deployment_num;			//The instance number of the next star power deployment

	//Last caching variables (stores score caching at one note earlier than the last evaluated solution's last SP deployment)
	static EOF_SP_PATH_SCORING_STATE last_cache ;	//Stores scoring data for the note prior to the deployment, to be able to skip the most calculations for sequential solutions being tested (ie. deploy at note 1 and 50, note 1 and 51, note 1 and 52, etc).
	static unsigned long last_cache_deployment_num = 0;	//The next deployment number in effect as of when the last cache data was stored
	int using_last_cache = 0;

	if(!solution || !solution_num || !solution->deployments || !solution->note_measure_positions || !solution->note_beat_lengths || !eof_song || !solution->track || (solution->track >= eof_song->tracks) || (solution->num_deployments > solution->deploy_count))
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t*Invalid parameters.  Solution invalid.");
		eof_log_casual(eof_log_string, 1, 1, 1);
		return 1;	//Invalid parameters
	}

	if(eof_song->track[solution->track]->track_format != EOF_LEGACY_TRACK_FORMAT)
		return 6;	//Unsupported track format

	///Look up cached scoring to skip as much calculation as possible
	if((solution_num->value > 1) && sequential && (last_cache.note_start < solution->note_count))
	{	//Use last cache if the calling function called for its use and the cache data is valid
		using_last_cache = 1;
		memcpy(&score, &last_cache, sizeof(EOF_SP_PATH_SCORING_STATE));	//Copy the last cache into the scoring state structure

		notectr = score.note_end_native;	//Resume parsing from the first note after that cached data
		index = score.note_end;
		deployment_num = last_cache_deployment_num;	//The next deployment will be whichever one was next at this point in the previous solution's evaluation
	}
	else
	{	//Otherwise use normal caching if possible
		last_cache.note_start = ULONG_MAX;	//Invalidate the last cache structure every time the first solution is being tested or a non sequential solution is tested

		for(ctr = 0; ctr < solution->num_deployments; ctr++)
		{	//For each star power deployment in this solution
			if(solution->deployments[ctr] == solution->deploy_cache[ctr].note_start)
			{	//If the star power began at the same time as is called for in this solution
				cache_number = ctr;	//Track the latest cache value that is applicable
			}
			else
			{	//The cached deployment does not match
				break;	//This and all subsequent entries will be marked invalid in the for loop below
			}
		}
		for(; ctr < solution->deploy_count; ctr++)
		{	//For each remaining cache entry that wasn't re-used
			solution->deploy_cache[ctr].note_start = ULONG_MAX;	//Mark the entry as invalid
		}

		if(cache_number < solution->num_deployments)
		{	//If a valid cache entry was identified
			memcpy(&score, &solution->deploy_cache[cache_number], sizeof(EOF_SP_PATH_SCORING_STATE));	//Copy it into the scoring state structure

			notectr = score.note_end_native;	//Resume parsing from the first note after that cached data
			index = score.note_end;
			deployment_num = cache_number + 1;	//The next deployment will be one higher in number than this cached one
		}
		else
		{	//Start processing from the beginning of the track
			//Init score state structure
			score.multiplier = 1;
			score.hitcounter = 0;
			score.score = 0;
			score.deployment_notes = 0;
			score.sp_meter = score.sp_meter_t = 0.0;

			//Init solution structure
			solution->score = 0;
			solution->deployment_notes = 0;

			notectr = index = 0;	//Start parsing notes from the first note
			deployment_num = 0;
		}
	}//Otherwise use normal caching if possible

	///Optionally log the solution's deployment details
	tracksize = eof_get_track_size(eof_song, solution->track);
	if(logging)
	{	//Skip logging overhead if logging isn't enabled
		char tempstring[100];
		int cached = 0;

		if(solution_num->value == ULONG_MAX)
		{	//If there is no tracked number for this solution (ie. multiple worker processes were used or a user defined solution is being evaluated)
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tNote indexes ");
		}
		else
		{
			if(solution_num->overflow_count)
			{	//If more than 4 billion solutions have been tested
				unsigned long billion_count, leftover;

				billion_count = (solution_num->overflow_count * 4) + (solution_num->value / 1000000000);
				leftover = solution_num->value % 1000000000;
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tSolution #%lu billion + %lu:  Note indexes ", billion_count, leftover);
			}
			else
			{
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tSolution #%lu:  Note indexes ", solution_num->value);
			}
		}

		for(ctr = 0; ctr < solution->num_deployments; ctr++)
		{	//For each deployment in the solution
			int thiscached = 0;

///EXTRA DEBUG LOGGING
/*			{
				unsigned long notenum, min, sec, ms;
				notenum = eof_translate_track_diff_note_index(eof_song, solution->track, solution->diff, solution->deployments[ctr]);	//Find the real note number for this index
				if(notenum < tracksize)
				{	//If that number was found
					ms = eof_get_note_pos(eof_song, solution->track, notenum);	//Determine mm:ss.ms formatting of timestamp
					min = ms / 60000;
					ms -= 60000 * min;
					sec = ms / 1000;
					ms -= 1000 * sec;
					snprintf(tempstring, sizeof(tempstring) - 1, "%s%s%lu (%lu:%lu.%lu)", (ctr ? ", " : ""), (thiscached ? "*" : ""), solution->deployments[ctr], min, sec, ms);
				}
			}
*/

			if((cache_number < solution->num_deployments) && (cache_number >= ctr))
			{	//If this deployment number is being looked up from cached scoring
				cached = 1;
				thiscached = 1;
			}
			(void) snprintf(tempstring, sizeof(tempstring) - 1, "%s%s%lu", (ctr ? ", " : ""), (thiscached ? "*" : ""), solution->deployments[ctr]);
			strncat(eof_log_string, tempstring, sizeof(eof_log_string) - 1);
		}
		if(using_last_cache)
		{
			(void) snprintf(tempstring, sizeof(tempstring) - 1, " (Last cache resuming from index %lu)", index);
			strncat(eof_log_string, tempstring, sizeof(eof_log_string) - 1);
		}

		if(!solution->num_deployments)
		{	//If there were no star power deployments in this solution
			strncat(eof_log_string, "(none)", sizeof(eof_log_string) - 1);
		}
		eof_log_casual(eof_log_string, 1, 1, 1);

		if(cached)
		{
			if(logging > 1)
			{	//Extra debug logging
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tUsing cache for deployment at note index #%lu (Hitcount=%lu  Mult.=x%lu  Score=%lu  D.Notes=%lu  SP Meter=%.2f%%).  Resuming from note index #%lu", solution->deploy_cache[cache_number].note_start, score.hitcounter, score.multiplier, score.score, score.deployment_notes, (score.sp_meter * 100.0), index);
				eof_log_casual(eof_log_string, 1, 1, 1);
			}
			if(index > solution->deployments[deployment_num])
			{	//If the solution indicated to deploy at a note that was already surpassed by the cache entry
				if(logging)
				{
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t!Invalid:  Cached scoring shows the attempted deployment is not possible.");
					eof_log_casual(eof_log_string, 1, 0, 0);
				}
				return 2;	//Solution invalidated by cache
			}
		}
	}

	///Process the notes in the target track difficulty
	for(; notectr < tracksize; notectr++)
	{	//For each note in the track being evaluated
		unsigned long whammy_bonus_ctr = 0;	//Track how many 1/25 beat increments of whammy bonus star power were awarded, if any (each of which extend active SP deployment by .01 measures)

		if(index >= solution->note_count)	//If all expected notes in the target track difficulty have been processed
			break;	//Stop processing notes
		if(eof_get_note_type(eof_song, solution->track, notectr) != solution->diff)
			continue;	//If this note isn't in the target track difficulty, skip it

		if(using_last_cache)
		{	//If the last cache data was just used, the loaded scoring reflects this note already
			using_last_cache = 0;	//Skip this one note
			index++;	//Advance this counter
			continue;	//Skip to the next note in the track difficulty
		}

		base_score = sp_base_score = sustain_score = sp_sustain_score = 0;
		notepos = eof_get_note_pos(eof_song, solution->track, notectr);
		notelength = eof_get_note_length(eof_song, solution->track, notectr);
		noteflags = eof_get_note_flags(eof_song, solution->track, notectr);
		disjointed = eof_get_note_eflags(eof_song, solution->track, notectr) & EOF_NOTE_EFLAG_DISJOINTED;

		if(disjointed)
		{	//If this is a disjointed gem, determine if this will be the gem processed for whammy star power bonus
			representative = eof_note_is_last_longest_gem(eof_song, solution->track, notectr);
		}

		///Update multiplier
		score.hitcounter++;
		if(score.hitcounter == 30)
		{
			if(logging > 1)
			{
				eof_log_casual("\t\t\tMultiplier increased to x4", 1, 1, 1);
			}
			score.multiplier = 4;
		}
		else if(score.hitcounter == 20)
		{
			if(logging > 1)
			{
				eof_log_casual("\t\t\tMultiplier increased to x3", 1, 1, 1);
			}
			score.multiplier = 3;
		}
		else if(score.hitcounter == 10)
		{
			if(logging > 1)
			{
				eof_log_casual("\t\t\tMultiplier increased to x2", 1, 1, 1);
			}
			score.multiplier = 2;
		}

		///Determine whether star power is to be deployed at this note
		deploy_sp = 0;
		for(ctr = 0; ctr < solution->num_deployments; ctr++)
		{
			if(solution->deployments[ctr] == index)
			{
				deploy_sp = 1;
				break;
			}
		}

		///Determine whether star power is in effect for this note
		if(sp_deployed)
		{	//If star power was in deployment during the previous note, determine if it should still be in effect
			unsigned long truncated_note_position, truncated_sp_deployment_end_position;

			//Truncate the note's position and end of deployment position at 3 decimal places for more reliable comparison
			truncated_note_position = solution->note_measure_positions[index] * 1000;
			truncated_sp_deployment_end_position = sp_deployment_end * 1000;
			if(truncated_note_position >= truncated_sp_deployment_end_position)
			{	//If this note is beyond the end of the deployment
				if(logging > 1)
				{
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tStar power ended before note #%lu (index #%lu)", notectr, index);
					eof_log_casual(eof_log_string, 1, 1, 1);
				}
				sp_deployed = 0;	//Star power is no longer in effect
				score.note_end = index;		//This note is the first after the star power deployment that was in effect for the previous note
				score.note_end_native = notectr;

				memcpy(&solution->deploy_cache[deployment_num], &score, sizeof(EOF_SP_PATH_SCORING_STATE));	//Append this scoring data to the cache
				solution->deploy_cache[deployment_num].hitcounter--;	//The hit counter reflects this note having already been hit.  Compensate so this counter isn't higher than it should be when loading from cache
				if(logging > 1)
				{
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tCaching deployment information for deployment ending before this note (Hitcount=%lu  Mult.=x%lu  Score=%lu  D.Notes=%lu  SP Meter=%.2f%%).", solution->deploy_cache[deployment_num].hitcounter, score.multiplier, score.score, score.deployment_notes, (score.sp_meter * 100.0));
					eof_log_casual(eof_log_string, 1, 1, 1);
				}
				deployment_num++;
			}
			else
			{	//If star power deployment is still in effect
				if(deploy_sp)
				{	//If the solution specifies deploying star power while it's already in effect
					if(logging)
					{
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t*Attempted to deploy star power while it is already deployed.  Solution invalid.");
						eof_log_casual(eof_log_string, 1, 1, 1);
					}
					return 3;	//Solution attempting to deploy while star power is in effect
				}
			}
		}
		if(!sp_deployed)
		{	//If star power is not in effect
			if(deploy_sp)
			{	//If the solution specifies deploying star power at this note
				if(score.sp_meter >= 0.50 - 0.0001)
				{	//If the star power meter is at least half full (allow variance for math error)
					sp_deployment_start = solution->note_measure_positions[index];
					sp_deployment_end = sp_deployment_start + (8.0 * score.sp_meter);	//Determine the end timing of the star power (one full meter is 8 measures)
					solution->deployment_endings[deployment_num] = sp_deployment_end;	//Store the deployment end position in the solution structure
					score.sp_meter = score.sp_meter_t = 0.0;	//The entire star power meter will be used
					sp_deployed = 1;	//Star power is now in effect
					score.note_start = index;	//Track the note index at which this star power deployment began, for caching purposes
					if(logging > 1)
					{
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tDeploying star power at note #%lu (index #%lu).  Projected deployment ending is measure %.2f", notectr, index, sp_deployment_end + 1);
						eof_log_casual(eof_log_string, 1, 1, 1);
					}
				}
				else
				{	//There is insufficient star power
					if(logging)
					{
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t!Invalid:  Insufficient star power to deploy.");
						eof_log_casual(eof_log_string, 1, 1, 1);
					}
					return 4;	//Solution attempting to deploy without sufficient star power
				}
			}
		}

		///Award star power for completing a star power phrase
		tflags = eof_get_note_tflags(eof_song, solution->track, notectr);
		if(tflags & EOF_NOTE_TFLAG_SP_END)
		{	//If this is the last note in a star power phrase
			if(sp_deployed)
			{	//If star power is in effect
				sp_deployment_end += 2.0;	//Extends the end of star power deployment by 2 measures (one fourth of the effect of a full star power meter)
				solution->deployment_endings[deployment_num] += 2.0;	//Likewise, extend the deployment end position in the solution structure
				if(logging > 1)
				{
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tStar power deployment ending extended two measures (measure %.2f)", sp_deployment_end + 1);
					eof_log_casual(eof_log_string, 1, 1, 1);
				}
			}
			else
			{	//Star power is not in effect
				score.sp_meter += 0.25;	//Award 25% of a star power meter
				score.sp_meter_t += 0.25;
				if(score.sp_meter > 1.0)
					score.sp_meter = 1.0;	//Cap the star power meter at 100%
			}
		}

		///Special case:  Whammying a sustained star power note while star power is deployed (star power level has to be evaluated as each sustain point is awarded)
		if((noteflags & EOF_NOTE_FLAG_SP) && (solution->note_beat_lengths[index] > 0.0) && (sp_deployed))
		{	//If this note has star power, it has sustain that won't be dropped by Clone Hero (for being shorter than 1/12 measure long) and star power is deployed
			double step = 1.0 / 25.0;	//Every 1/25 beat of sustain, a point is awarded.  Evaluate star power gain/loss at every such interval
			double remaining_sustain = solution->note_beat_lengths[index];	//The amount of sustain remaining to be scored for this star power note
			double sp_whammy_gain = 1.0 / 32.0 / 25.0;	//The amount of star power gained from whammying a star power sustain (1/32 meter per beat) during one scoring interval (1/25 beat)
			double realpos = notepos;	//Keep track of the realtime position at each 1/25 beat interval of the note

			base_score = eof_note_count_colors(eof_song, solution->track, notectr) * 50;	//The base score for a note is 50 points per gem
			sp_base_score = base_score;		//Star power adds the same number of bonus points
			notescore = base_score + sp_base_score;
			score.deployment_notes++;	//Track how many notes are played during star power deployment
			score.sp_meter = (sp_deployment_end - solution->note_measure_positions[index]) / 8.0;	//Convert remaining star power deployment duration back to star power
			score.sp_meter_t = score.sp_meter;

			while(remaining_sustain > 0)
			{	//While there's still sustain to examine
				unsigned long beat = eof_get_beat(eof_song, realpos);	//The beat in which this 1/25 beat interval resides
				double sp_drain;	//The rate at which star power is consumed during deployment, dependent on the current time signature in effect
				unsigned long disjointed_multiplier = 1;	//In the case of disjointed chords, this will be set to the number of gems applicable for each sustain point scored

				///Double special case:  For disjointed chords, whammy bonus star power is only given for the longest gem, even though each gem gets points for sustain
				if(disjointed && !representative)
				{	//If this note is a single gem in a disjointed chord but it isn't the one that will be examined for whammy star power bonus
					break;	//Don't process sustain scoring for this gem
				}

				if(remaining_sustain < (step / 2.0))
				{	//If there's less than half a point of sustain left, it will be dropped instead of scored
					break;
				}

				//Update the star power meter
				if((beat >= eof_song->beats) || !eof_song->beat[beat]->has_ts)
				{	//If the beat containing this 1/25 beat interval couldn't be identified, or if the beat has no time signature in effect
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t*Logic error.  Solution invalid.");
					eof_log_casual(eof_log_string, 1, !logging, 1);
					return 5;	//General logic error
				}
				sp_drain = 1.0 / 8.0 / eof_song->beat[beat]->num_beats_in_measure / 25.0;	//Star power drains at a rate of 1/8 per measure of beats, and this is the amount of drain for a 1/25 beat interval
				score.sp_meter = score.sp_meter + sp_whammy_gain - sp_drain;	//Update the star power meter to reflect the amount gained and the amount consumed during this 1/25 beat interval
				whammy_bonus_ctr++;
				if(score.sp_meter > 1.0)
					score.sp_meter = 1.0;	//Cap the star power meter at 100%
				solution->deployment_endings[deployment_num] = sp_deployment_start + (8.0 * score.sp_meter);	//Convert sp_meter and store the current deployment end position in the solution structure

				//Score this sustain interval
				///Double special case:  For disjointed chords, since the whammy bonus star power is only being evaluated for the longest gem, and
				/// sustain points are being scored at the same time, such points have to be awarded for each of the applicable gems in the disjointed chord
				if(remaining_sustain < step)
				{	//If there was less than a point's worth of sustain left, it will be scored and then rounded to nearest point
					double floatscore;
					unsigned long ceiled_floatscore;

					realpos = (double) notepos + notelength - 1.0;	//Set the realtime position to the last millisecond of the note
					if(disjointed)
					{	//If this is the longest, representative gem in a disjointed chord, count how many of the chord's gems extend far enough to receive this partial bonus point
						disjointed_multiplier = eof_note_count_gems_extending_to_pos(eof_song, solution->track, notectr, realpos + 0.5);
					}
					floatscore = remaining_sustain / step;	//This represents the fraction of one point this remaining sustain is worth
					if(score.sp_meter > 0.0)
					{	//If star power is still in effect
						floatscore = (2 * disjointed_multiplier);	//This remainder of sustain score is doubled due to star power bonus
						ceiled_floatscore = ceil(floatscore);
						sp_sustain_score += ceiled_floatscore;		//For logging purposes, consider this a SP sustain point (since a partial sustain point and a partial SP sustain point won't be separately tracked)
					}
					else
					{
						floatscore = disjointed_multiplier;			//This remainder of sustain score does not get a star power bonus
						ceiled_floatscore = ceil(floatscore);
						sustain_score += ceiled_floatscore;			//Track how many non star power sustain points were awarded
					}
					notescore += ceiled_floatscore;					//Ceiling the sustain score as per Clone Hero's sustain scoring rules
				}
				else
				{	//Normal scoring
					realpos += eof_get_beat_length(eof_song, beat) / 25.0;	//Update the realtime position by 1/25 of the length of the current beat the interval is in
					if(disjointed)
					{	//If this is the longest, representative gem in a disjointed chord, count how many of the chord's gems extend far enough to receive this bonus point
						disjointed_multiplier = eof_note_count_gems_extending_to_pos(eof_song, solution->track, notectr, realpos + 0.5);
					}
					if(score.sp_meter > 0.0)
					{	//If star power is still in effect
						notescore += (2 * disjointed_multiplier);	//This point of sustain score is doubled due to star power bonus
						sustain_score += disjointed_multiplier;		//Track how many non star power sustain points were awarded
						sp_sustain_score += disjointed_multiplier;	//The same number of points are awarded as a star power bonus
					}
					else
					{
						notescore += disjointed_multiplier;			//This point of sustain score does not get a star power bonus
						sustain_score += disjointed_multiplier;		//Track how many non star power sustain points were awarded
					}
				}

				remaining_sustain -= step;	//One interval less of sustain to examine
			}//While there's still sustain to examine
			if(score.sp_meter > 0.0)
			{	//If the star power meter still has star power, calculate the new end of deployment position
				sp_deployment_start = eof_get_measure_position(notepos + notelength);	//The remainder of the deployment starts at this note's end position
				sp_deployment_end = sp_deployment_start + (8.0 * score.sp_meter);	//Determine the end timing of the star power (one full meter is 8 measures)
				solution->deployment_endings[deployment_num] = sp_deployment_end;	//Store the deployment end position in the solution structure
				score.sp_meter = score.sp_meter_t = 0.0;	//The entire star power meter will be used
			}
			else
			{	//Otherwise the star power deployment has ended
				score.sp_meter = score.sp_meter_t = 0.0;	//The star power meter was depleted
				sp_deployed = 0;
				score.note_end = index + 1;	//The next note will be the first after this ended star power deployment
				score.note_end_native = notectr + 1;

				memcpy(&solution->deploy_cache[deployment_num], &score, sizeof(EOF_SP_PATH_SCORING_STATE));	//Append this scoring data to the cache
				solution->deploy_cache[deployment_num].hitcounter--;	//The hit counter reflects this note having already been hit.  Compensate so this counter isn't higher than it should be when loading from cache
				if(logging > 1)
				{
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tCaching deployment information for deployment ending before this note (Hitcount=%lu  Mult.=x%lu  Score=%lu  D.Notes=%lu  SP Meter=%.2f%%).", solution->deploy_cache[deployment_num].hitcounter, score.multiplier, score.score, score.deployment_notes, (score.sp_meter * 100.0));
					eof_log_casual(eof_log_string, 1, 1, 1);
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tStar power ended during note #%lu (index #%lu)", notectr, index);
					eof_log_casual(eof_log_string, 1, 1, 1);
				}
				deployment_num++;
			}
			notescore *= score.multiplier;	//Apply the current score multiplier in effect
		}//If this note has star power, it has sustain that won't be dropped by Clone Hero (for being shorter than 1/12 measure long) and star power is deployed
		else
		{	//Score the note and evaluate whammy star power gain separately
			double sustain = 0.0;	//The score for the portion of the current note's sustain that is subject to star power bonus
			double sustain2 = 0.0;	//The score for the portion of the current note's sustain that occurs after star power deployment ends mid-note (which is not awarded star power bonus)

			///Add any sustained star power note whammy bonus
			if(noteflags & EOF_NOTE_FLAG_SP)
			{	//If this note has star power
				if(solution->note_beat_lengths[index] > 0.0)
				{	//If it has sustain that won't be dropped by Clone Hero (for being shorter than 1/12 measure long)
					///Double special case:  For disjointed chords, whammy bonus star power is only given for the longest gem
					if(!disjointed || representative)
					{	//If this is not a disjointed gem, or it is and this is the gem that is to be examined for the purpose of whammy star power bonus
						double sp_bonus = solution->note_beat_lengths[index] / 32.0;	//Award 1/32 of a star power meter per beat of whammied star power sustain

						score.sp_meter += sp_bonus;	//Add the star power bonus
						score.sp_meter_t += sp_bonus;
						if(score.sp_meter > 1.0)
							score.sp_meter = 1.0;	//Cap the star power meter at 100%
					}
				}
			}

			///Calculate the note's score
			base_score = eof_note_count_colors(eof_song, solution->track, notectr) * 50;	//The base score for a note is 50 points per gem
			notescore = base_score;
			if(solution->note_beat_lengths[index] > 0.0)
			{	//If this note has any sustain that won't be dropped by Clone Hero (for being shorter than 1/12 measure long),
				//determine its score and whether any of that sustain score is not subject to SP bonus due to SP deployment ending in the middle of the sustain (score2)
				sustain2 = 25.0 * solution->note_beat_lengths[index];	//The sustain's base score is 25 points per beat

				if(solution->note_beat_lengths[index] >= 1.000 + 0.0001)
				{	//If this note length is one beat or longer (allowing variance for math error), only the portion occurring before the end of star power deployment is awarded SP bonus
					double sustain_end = eof_get_measure_position(notepos + notelength);

					if(sp_deployed && (sustain_end > sp_deployment_end))
					{	//If star power is deployed and this sustain note ends after the ending of the star power deployment
						double note_measure_length = sustain_end - solution->note_measure_positions[index];		//The length of the note in measures
						double sp_portion = (sp_deployment_end - solution->note_measure_positions[index]) / note_measure_length;	//The percentage of the note that is subject to star power

						sustain = sustain2 * sp_portion;	//Determine the portion of the sustain points that are awarded star power bonus
						sustain2 -= sustain;				//Any remainder of the sustain points are not awarded star power bonus
					}
				}
				else
				{	//If this note length is less than a beat long
					sustain = sustain2;	//All of the sustain is awarded star power bonus
					sustain2 = 0;		//None of it will receive regular scoring
				}
				sustain_score = ceil(sustain + sustain2);	//Track (for logging purposes) how many of the points are being awarded for the sustain (ceilinged to int as per Clone Hero's sustain scoring rules)
				if(sp_deployed)			//If star power is in effect
				{
					sp_sustain_score = ceil(sustain);	//Track (for logging purposes) how many star power bonus points are being awarded for the sustain
					sustain *= 2.0;		//Apply star power bonus to the applicable portion of the sustain
				}
			}
			if(sp_deployed)				//If star power is in effect
			{
				notescore *= 2;					//It doubles the note's score
				sp_base_score = base_score;
				score.deployment_notes++;	//Track how many notes are played during star power deployment
			}
			notescore += ceil(sustain + sustain2);	//Ceiling the sustain point total (including relevant star power bonus) and add to the note's score
			notescore *= score.multiplier;			//Apply the current score multiplier in effect
		}//Score the note and evaluate whammy star power gain separately

		///Update solution score
		is_solo = (tflags & EOF_NOTE_TFLAG_SOLO_NOTE) ? 1 : 0;	//Check whether this note was determined to be in a solo section
		if(is_solo)
		{	//If this note was flagged as being in a solo
			notescore += 100;	//It gets a bonus (not stackable with star power or multiplier) of 100 points if all notes in the solo are hit
		}
		score.score += notescore;

		if(logging > 1)
		{
			if(!sp_base_score)
			{	//No star power bonus
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tNote #%lu (index #%lu): pos = %lums, m pos = %.2f, \tbase = %lu, sustain = %lu, \tmult = x%lu, solo bonus = %lu, hitctr = %lu,\tscore = %lu.  \tTotal score:  %lu\tSP Meter at %.2f%% (uncapped %.2f%%)", notectr, index, notepos, solution->note_measure_positions[index] + 1, base_score, sustain_score, score.multiplier, is_solo * 100, score.hitcounter, notescore, score.score, (score.sp_meter * 100.0), (score.sp_meter_t * 100.0));
			}
			else
			{	//Star power bonus
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tNote #%lu (index #%lu): pos = %lums, m pos = %.2f, \tbase = %lu, sustain = %lu, SP = %lu, SP sustain = %lu, \tmult = x%lu, solo bonus = %lu, hitctr = %lu,\tscore = %lu.  \tTotal score:  %lu\tSP Meter at %.2f%% (uncapped %.2f%%), Deployment ends at measure %.2f", notectr, index, notepos, solution->note_measure_positions[index] + 1, base_score, sustain_score, sp_base_score, sp_sustain_score, score.multiplier, is_solo * 100, score.hitcounter, notescore, score.score, (score.sp_meter * 100.0), (score.sp_meter_t * 100.0), sp_deployment_end + 1);
			}
			eof_log_casual(eof_log_string, 1, 1, 1);
			if(whammy_bonus_ctr)
			{	//If the active star power deployment's ending was extended due to whammying a star power sustain
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tWhammying this note extended the deployment by %.2f measures.  Projected deployment ending is measure %.2f", .01 * (double)whammy_bonus_ctr, sp_deployment_end + 1);
				eof_log_casual(eof_log_string, 1, 1, 1);
			}
		}

		///Store scoring data into the last cache structure if applicable
		if(index + 1 == solution->deployments[solution->num_deployments - 1])
		{	//If the last deployment for the solution is the next note
			memcpy(&last_cache, &score, sizeof(EOF_SP_PATH_SCORING_STATE));	//Store the scoring data to last cache
			last_cache.note_start = 0;					//This cache data will reflect scoring for all notes from the first one
			last_cache.note_end = index;				//Through all notes up to the one jus scored
			last_cache.note_end_native = notectr;
			last_cache_deployment_num = deployment_num;	//Store this value
			if(logging > 1)
			{	//Verbose log the caching
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tSaving last cache data (note index #%lu, mult = x%lu, hitctr = %lu, score = %lu, SP meter = %.2f%%)", last_cache.note_end, score.multiplier, score.hitcounter, score.score, score.sp_meter * 100.0);
				eof_log_casual(eof_log_string, 1, 1, 1);
			}
		}

		index++;	//Keep track of the number of notes in this track difficulty that were processed
	}//For each note in the track being evaluated

	solution->score = score.score;	//Set the final score to the one from the scoring state structure
	solution->deployment_notes = score.deployment_notes;
	if(logging)
	{	//Skip the overhead of building the logging string if it won't be logged
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tSolution score:  %lu", solution->score);
		if(logging > 1)
		{	//If all of the per-note scoring was logged
			eof_log_casual(eof_log_string, 2, 1, 1);	//This output will be on its own line and will need the log ID
		}
		else
		{
			eof_log_casual(eof_log_string, 2, 0, 0);
		}
	}

	return 0;	//Return solution evaluated
}

int eof_ch_sp_path_single_process_solve(EOF_SP_PATH_SOLUTION *best, EOF_SP_PATH_SOLUTION *testing, unsigned long first_deploy, unsigned long last_deploy, EOF_BIG_NUMBER *validcount, EOF_BIG_NUMBER *invalidcount, unsigned long *deployment_notes, char *kill_file)
{
	char windowtitle[101] = {0};
	int invalid_increment = 0;	//Set to nonzero if the last iteration of the loop manually incremented the solution due to the solution being invalid
	int retval;
	int sequential;	//Controls the use of the last cache mechanism in eof_evaluate_ch_sp_path_solution()
	EOF_BIG_NUMBER solution_count = {0};
	clock_t time1, time2;

	eof_log("eof_ch_sp_path_single_process_solve() entered", 1);

	if(!eof_song || !best || !testing || !validcount || !invalidcount || !deployment_notes)
	{
		eof_log("\tInvalid pointer parameters", 1);
		return 1;	//Invalid parameters
	}
	if(first_deploy > best->note_count)
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tLogic error:  First deployment (%lu) > the number of notes in the target track difficulty (%lu)", first_deploy, best->note_count);
		eof_log(eof_log_string, 1);
		return 1;	//Invalid parameters
	}

	testing->num_deployments = 0;	//The first solution increment will test one deployment
	testing->deployments[0] = first_deploy;	//Apply a correct value here for the sake of the title bar reflecting an accurate solution set number
	*deployment_notes = 0;			//Reset this count so the first solution will be counted as the one with the most notes played during SP deployments
	time1 = clock();				//Track the start time of this loop
	while(1)
	{	//Continue testing until all solutions (or specified solutions) are tested
		unsigned long next_deploy;

		sequential = 0;	//Unless the solution being tested is identical to the previous solution except that the last deployment is one note later, this will be zero
		eof_big_number_increment(&solution_count);	//Track number of tested solutions
		if(solution_count.value % 2000 == 0)
		{	//Every 2000 solutions
			if(solution_count.overflow_count)
			{	//If more than 4 billion solutions have been tested
				unsigned long billion_count, leftover;

				billion_count = (solution_count.overflow_count * 4) + (solution_count.value / 1000000000);
				leftover = solution_count.value % 1000000000;
				if(first_deploy == last_deploy)
				{	//If only one solution set is being tested
					(void) snprintf(windowtitle, sizeof(windowtitle) - 1, "Testing SP path solution %lu billion + %lu (set %lu)- Press Esc to cancel", billion_count, leftover, testing->deployments[0]);
				}
				else
				{
					(void) snprintf(windowtitle, sizeof(windowtitle) - 1, "Testing SP path solution %lu billion + %lu (set %lu/%lu)- Press Esc to cancel", billion_count, leftover, testing->deployments[0], testing->note_count);
				}
			}
			else
			{
				if(first_deploy == last_deploy)
				{	//If only one solution set is being tested
					(void) snprintf(windowtitle, sizeof(windowtitle) - 1, "Testing SP path solution %lu (set %lu)- Press Esc to cancel", solution_count.value, testing->deployments[0]);
				}
				else
				{
					(void) snprintf(windowtitle, sizeof(windowtitle) - 1, "Testing SP path solution %lu (set %lu/%lu)- Press Esc to cancel", solution_count.value, testing->deployments[0], testing->note_count);
				}
			}
			set_window_title(windowtitle);	//Update the title bar

			if(kill_file)
			{	//If the calling function wanted this function to check for a kill signal file
				time2 = clock();
				if((time2 - time1) / CLOCKS_PER_SEC >= EOF_SP_PATH_KILL_FILE_CHECK_INTERVAL)
				{	//If it's time to check for this signal
					if(exists(kill_file))
					{	//If the file does exist
						eof_log_casual("\tKill signal detected", 1, 1, 1);
						eof_log_casual(NULL, 1, 1, 1);
						return 2;	//User cancellation
					}
					time1 = time2;
				}
			}
			if(key[KEY_ESC])
			{	//Allow user to cancel
				return 2;	//User cancellation
			}
			if(eof_close_button_clicked)
			{	//Consider clicking the close window control a cancellation
				eof_close_button_clicked = 0;
				return 2;	//User cancellation
			}
		}

		//Increment solution
		if(!invalid_increment)
		{	//Don't increment the solution if the last iteration already did so
			if(testing->num_deployments < testing->deploy_count)
			{	//If the current solution array has fewer than the maximum number of usable star power deployments
				if(!testing->num_deployments)
				{	///Case 1:  This is the first solution
					testing->deployments[0] = first_deploy;	//Initialize the first solution to test one deployment at the specified note index
					testing->num_deployments++;	//One more deployment is in the solution
				}
				else
				{	//Add another deployment to the solution
					unsigned long previous_deploy = testing->deployments[testing->num_deployments - 1];	//This is the note at which the previous deployment occurs
					next_deploy = eof_ch_pathing_find_next_deployable_sp(testing, previous_deploy);	//Detect the next note after which another 50% of star power meter has accumulated

					if(next_deploy < testing->note_count)
					{	//If a valid placement for the next deployment was found
						///Case 2:  Add one deployment to the solution
						testing->deployments[testing->num_deployments] = next_deploy;
						testing->num_deployments++;	//One more deployment is in the solution
					}
					else
					{	//If there are no further valid notes to test for the next deployment
						next_deploy = previous_deploy + 1;	//Advance the last deployment one note further
						if(next_deploy >= testing->note_count)
						{	//If the last deployment cannot advance
							///Case 4:  The last deployment has exhausted all notes, remove and advance previous solution
							testing->num_deployments--;	//Remove this deployment from the solution
							if(!testing->num_deployments)
							{	//If there are no more solutions to test
								///Case 5:  All solutions exhausted
								break;
							}
							if((testing->num_deployments == 1) && (testing->deployments[testing->num_deployments - 1] + 1 > last_deploy))
							{	//If all solutions for the first deployment's specified range of notes have been tested
								///Case 6:  All specified solutions exhausted
								break;
							}
							next_deploy = testing->deployments[testing->num_deployments - 1] + 1;	//Advance the now-last deployment one note further
							if(next_deploy >= testing->note_count)
							{	//If the previous deployment cannot advance even though the deployment that was just removed had come after it
								eof_log("\tLogic error:  Can't advance previous deployment after removing last deployment (1)", 1);
								return 1;	//Return error
							}
							testing->deployments[testing->num_deployments - 1] = next_deploy;
						}
						else
						{	///Case 3:  Advance last deployment by one note
							sequential = 1;	//This scenario allows for the last cache data to be used in eof_evaluate_ch_sp_path_solution()
							testing->deployments[testing->num_deployments - 1] = next_deploy;
						}
					}
				}//Add another deployment to the solution
			}//If the current solution array has fewer than the maximum number of usable star power deployments
			else if(testing->num_deployments == testing->deploy_count)
			{	//If the maximum number of deployments are in the solution, move the last deployment to create a new solution to test
				next_deploy = testing->deployments[testing->num_deployments - 1] + 1;	//Advance the last deployment one note further
				if(next_deploy >= testing->note_count)
				{	//If the last deployment cannot advance
					///Case 4:  The last deployment has exhausted all notes, remove and advance previous solution
					testing->num_deployments--;	//Remove this deployment from the solution
					if(!testing->num_deployments)
					{	//If there are no more solutions to test
						///Case 5:  All solutions exhausted
						break;
					}
					if((testing->num_deployments == 1) && (testing->deployments[testing->num_deployments - 1] + 1 > last_deploy))
					{	//If all solutions for the first deployment's specified range of notes have been tested
						///Case 6:  All specified solutions exhausted
						break;
					}
					next_deploy = testing->deployments[testing->num_deployments - 1] + 1;	//Advance the now-last deployment one note further
					if(next_deploy >= testing->note_count)
					{	//If the previous deployment cannot advance even though the deployment that was just removed had come after it
						eof_log("\tLogic error:  Can't advance previous deployment after removing last deployment (2)", 1);
						return 1;	//Return error
					}
					testing->deployments[testing->num_deployments - 1]= next_deploy;
				}
				else
				{	///Case 3:  Advance last deployment by one note
					sequential = 1;	//This scenario allows for the last cache data to be used in eof_evaluate_ch_sp_path_solution()
					testing->deployments[testing->num_deployments - 1] = next_deploy;
				}
			}
			else
			{	//If num_deployments > max_deployments
				eof_log("\tLogic error:  More than the maximum number of deployments entered the testing solution", 1);
				return 1;	//Return error
			}
		}//Don't increment the solution if the last iteration already did so
		invalid_increment = 0;

		//Test and compare with the current best solution
		retval = eof_evaluate_ch_sp_path_solution(testing, &solution_count, (eof_log_level > 1 ? 1 : 0), sequential);	//Evaluate the solution (only perform light evaluation logging if verbose logging or higher is enabled)
		if(!retval)
		{	//If the solution was considered valid
			if((testing->score > best->score) || ((testing->score == best->score) && (testing->deployment_notes < best->deployment_notes)))
			{	//If this newly tested solution achieved a higher score than the current best, or if it matched the highest score but did so with fewer notes played during star power deployment
				eof_log_casual("\t!New best solution", 2, 0, 0);

				best->score = testing->score;	//It is the new best solution, copy its data into the best solution structure
				best->deployment_notes = testing->deployment_notes;
				best->num_deployments = testing->num_deployments;
				best->solution_number.overflow_count = validcount->overflow_count;
				best->solution_number.value = validcount->value;
				eof_big_number_add_big_number(&best->solution_number, invalidcount);	//Keep track of which solution number this is, for logging purposes

				if(testing->deployment_notes > *deployment_notes)
				{	//If this solution played more notes during star power than any previous solution
					*deployment_notes = testing->deployment_notes;	//Track this count
				}
				memcpy(best->deployments, testing->deployments, sizeof(unsigned long) * testing->deploy_count);
			}

			eof_big_number_increment(validcount);	//Track the number of valid solutions tested
		}
		else
		{	//If the solution was invalid
			next_deploy = ULONG_MAX;

			if(testing->num_deployments < testing->deploy_count)
			{	//If fewer than the maximum number of deployments was just tested and found invalid, all solutions using this set of deployments will also fail
				if(testing->num_deployments > 1)
				{	//If the next deployment can be advanced by one note
					next_deploy = testing->deployments[testing->num_deployments - 1] + 1;	//Get the note index one after the last tested deployment
					if(next_deploy < testing->note_count)
					{	//If the next deployment can be advanced to that note
						if(eof_log_level > 1)
						{	//Skip the overhead of building the logging string if it won't be logged
							(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t!Discarding solutions where deployment #%lu is at note index #%lu", testing->num_deployments, testing->deployments[testing->num_deployments - 1]);
							eof_log_casual(eof_log_string, 2, 0, 0);
						}
					}
				}
			}

			if(retval == 2)
			{	//If the last tested solution was invalidated by the cache, use the cache to skip other invalid solutions up until that cache entry's end of deployment
				if(testing->num_deployments > 1)
				{	//If the deployment that was invalidated is after the first one
					unsigned long note_end = testing->deploy_cache[testing->num_deployments - 2].note_end;	//Look up the first note occurring after the last successful star power deployment's end

					if((note_end < testing->note_count) && (note_end > testing->deployments[testing->num_deployments - 1]))
					{	//If the next deployment can be advanced to that note
						next_deploy = note_end;	//Do so
						if(eof_log_level > 1)
						{	//Skip the overhead of building the logging string if it won't be logged
							(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t!Skipping deployment #%lu to next note index #%lu", testing->num_deployments, next_deploy);
							eof_log_casual(eof_log_string, 2, 0, 0);
						}
					}
				}
			}

			if(retval == 4)
			{	//If the last tested solution was invalidated due to there being insufficient star power to deploy, skip all other solutions up until the next star power note
				unsigned long next_sp_note;

				next_deploy = testing->deployments[testing->num_deployments - 1] + 1;	//Get the note index one after the last tested deployment
				next_sp_note = eof_ch_pathing_find_next_sp_note(testing, next_deploy);	//Find the next note with star power (first opportunity to gain more star power)

				if(next_sp_note < testing->note_count)
				{	//If such a note exists
					next_deploy = next_sp_note;	//Do so
					if(eof_log_level > 1)
					{	//Skip the overhead of building the logging string if it won't be logged
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t!Skipping deployment #%lu to next SP note (index #%lu)", testing->num_deployments, next_deploy);
						eof_log_casual(eof_log_string, 2, 0, 0);
					}
				}
				else
				{	//No such star power note exists, all of the parent deployment's remaining solutions will also fail for the same reason
					//Remove the last deployment and increment the new-last deployment
					if(eof_log_level > 1)
					{	//Skip the overhead of building the logging string if it won't be logged
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t!There are no more SP notes.  Remaining solutions for deployment #%lu are invalid.", testing->num_deployments);
						eof_log_casual(eof_log_string, 2, 0, 0);
					}
					testing->deployments[testing->num_deployments - 1] = testing->note_count;	//Move the current deployment to the last usable index, the normal increment at the beginning of the loop will advance the parent solution to the next index
					next_deploy = ULONG_MAX;	//Override any manual iteration from the above checks
				}
			}

			if(next_deploy < testing->note_count)
			{	//If one of the above conditional tests defined the next solution
				testing->deployments[testing->num_deployments - 1] = next_deploy;
				invalid_increment = 1;	//Prevent the solution from being incremented again at the beginning of the next loop
			}

			eof_big_number_increment(invalidcount);	//Track the number of invalid solutions tested
		}//If the solution was invalid
		if(eof_log_level > 1)
		{	//If there was verbose logging made for this solution
			eof_log_casual("", 2, 0, 1);	//End the line of logging for this solution
		}
	}//Continue testing until all solutions (or specified solutions) are tested

	eof_log_casual(NULL, 2, 1, 1);	//Flush the buffered log writes to disk
	return 0;	//Return success
}

void eof_ch_pathing_mark_tflags(EOF_SP_PATH_SOLUTION *solution)
{
	unsigned long tracksize, numsolos, ctr, ctr2;
	EOF_PHRASE_SECTION *sectionptr;

	if(!solution || !eof_song || !solution->track || (solution->track >= eof_song->tracks))
		return;	//Invalid parameters

	tracksize = eof_get_track_size(eof_song, solution->track);
	numsolos = eof_get_num_solos(eof_song, solution->track);
	for(ctr = 0; ctr < tracksize; ctr++)
	{	//For each note in the active track
		unsigned long tflags, notepos;

		if(eof_get_note_type(eof_song, solution->track, ctr) != solution->diff)
			continue;	//If it's not in the active track difficulty, skip it

		notepos = eof_get_note_pos(eof_song, solution->track, ctr);
		tflags = eof_get_note_tflags(eof_song, solution->track, ctr);
		tflags &= ~(EOF_NOTE_TFLAG_SOLO_NOTE | EOF_NOTE_TFLAG_SP_END);	//Clear these temporary flags

		if(eof_note_is_last_in_sp_phrase(eof_song, solution->track, ctr))
		{	//If the note is the last note in a star power phrase
			tflags |= EOF_NOTE_TFLAG_SP_END;	//Set the temporary flag that will track this condition to reduce repeatedly testing for this
		}
		for(ctr2 = 0; ctr2 < numsolos; ctr2++)
		{	//For each solo phrase in the active track
			sectionptr = eof_get_solo(eof_song, solution->track, ctr2);
			if((notepos >= sectionptr->start_pos) && (notepos <= sectionptr->end_pos))
			{	//If the note is in this solo phrase
				tflags |= EOF_NOTE_TFLAG_SOLO_NOTE;	//Set the temporary flag that will track this note being in a solo
				break;
			}
		}
		eof_set_note_tflags(eof_song, solution->track, ctr, tflags);
	}
}

int eof_ch_sp_path_setup(EOF_SP_PATH_SOLUTION **bestptr, EOF_SP_PATH_SOLUTION **testingptr, char *undo_made)
{
	EOF_SP_PATH_SOLUTION *best = NULL, *testing = NULL;
	unsigned long note_count = 0;				//The number of notes in the active track difficulty, which will be the size of the various note arrays
	double *note_measure_positions = NULL;		//The position of each note in the active track difficulty defined in measures
	double *note_beat_lengths = NULL;			//The length of each note in the active track difficulty defined in beats

	unsigned long max_deployments;				//The estimated maximum number of star power deployments, based on the amount of star power note sustain and the number of star power phrases
	unsigned long *tdeployments, *bdeployments = NULL;	//An array defining the note index number of each deployment, ie deployments[0] being the first SP deployment, deployments[1] being the second, etc.
	EOF_SP_PATH_SCORING_STATE *deploy_cache;	//An array storing cached scoring data for star power deployments
	double *deployment_endings;					//An array storing end positions for each star power deployment

	unsigned long ctr, index, tracksize;
	unsigned long sp_phrase_count;
	double sp_sustain;				//The number of beats of star power sustain, used to count whammy bonus star power when estimating the maximum number of star power deployments

	//Variables for determining exact delta tick lengths that notes will have during MIDI export, since Clone Hero will discard the sutain of all notes shorter than 1/12 measure
	//The logic for determining these delta values is re-used from the MIDI export code to ensure the best accuracy
	char has_stored_tempo;		//Will be set to nonzero if the project contains a stored tempo track, which will affect timing conversion
	struct Tempo_change *anchorlist=NULL;	//Linked list containing tempo changes
	EOF_MIDI_TS_LIST *tslist=NULL;			//List containing TS changes
	unsigned long timedivision = EOF_DEFAULT_TIME_DIVISION;	//Unless the project is storing a tempo track, EOF's default time division will be used

 	eof_log("eof_ch_sp_path_setup() entered", 1);

	if(!testingptr || !eof_song)
	{
		eof_log("\tInvalid parameters.", 1);
		return 1;	//Return error
	}

 	///Ensure there's a time signature in effect
	if(!eof_beat_stats_cached)
		eof_process_beat_statistics(eof_song, eof_selected_track);
	if(!eof_song->beat[0]->has_ts)
	{
		allegro_message("A time signature is required for the chart, but none in effect on the first beat.  4/4 will be applied.");

		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		if(undo_made)
			*undo_made = 1;
		(void) eof_apply_ts(4, 4, 0, eof_song, 0);
		eof_process_beat_statistics(eof_song, eof_selected_track);	//Recalculate beat statistics
	}

	///Initialize variables for calculating MIDI export timings
	if(!eof_build_tempo_and_ts_lists(eof_song, &anchorlist, &tslist, &timedivision))
	{
		eof_log("\tError saving:  Cannot build tempo or TS list", 1);
		return 1;	//Return error
	}
	has_stored_tempo = eof_song_has_stored_tempo_track(eof_song) ? 1 : 0;	//Store this status
	if(!eof_calculate_beat_delta_positions(eof_song))
	{	//Calculate the delta position of each beat in the chart (required by eof_ConvertToDeltaTime() )
		eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
		eof_destroy_ts_list(tslist);		//Free memory used by the TS change list
		eof_log("\tCould not build beat delta positions", 1);
		return 1;	//Return error
	}

	///Initialize arrays and structures
	(void) eof_count_selected_notes(&note_count);	//Count the number of notes in the active track difficulty
	if(!note_count)
	{
		eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
		eof_destroy_ts_list(tslist);		//Free memory used by the TS change list
		return 2;	//Return cancellation (Don't both doing anything if there are no notes in the active track difficulty)
	}

	note_measure_positions = malloc(sizeof(double) * note_count);
	note_beat_lengths = malloc(sizeof(double) * note_count);
	if(bestptr)
	{	//If the calling function wanted to initialize two solution structures
		best = malloc(sizeof(EOF_SP_PATH_SOLUTION));
	}
	testing = malloc(sizeof(EOF_SP_PATH_SOLUTION));

	if(!note_measure_positions || !note_beat_lengths || (bestptr && !best) || !testing)
	{	//If any of those failed to allocate
		if(note_measure_positions)
			free(note_measure_positions);
		if(note_beat_lengths)
			free(note_beat_lengths);
		if(best)
			free(best);
		if(testing)
			free(testing);
		eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
		eof_destroy_ts_list(tslist);		//Free memory used by the TS change list

		eof_log("\tFailed to allocate memory", 1);
		return 1;	//Return error
	}

	if(bestptr)
	{	//If the calling function wanted to initialize two solution structures
		best->note_measure_positions = note_measure_positions;
		best->note_beat_lengths = note_beat_lengths;
		best->note_count = note_count;
		best->track = eof_selected_track;
		best->diff = eof_note_type;
		best->solution_number.overflow_count = best->solution_number.value = 0;
	}

	testing->note_measure_positions = note_measure_positions;
	testing->note_beat_lengths = note_beat_lengths;
	testing->note_count = note_count;
	testing->track = eof_selected_track;
	testing->diff = eof_note_type;
	testing->solution_number.overflow_count = testing->solution_number.value = 0;

	///Apply EOF_NOTE_TFLAG_SOLO_NOTE and EOF_NOTE_TFLAG_SP_END tflags appropriately to notes in the target track difficulty
	eof_ch_pathing_mark_tflags(testing);

	///Calculate the measure position and beat length of each note in the active track difficulty
	///Find the first note at which star power can be deployed
	///Record the end position of each star power phrase for faster detection of the last note in a star power phrase
	tracksize = eof_get_track_size(eof_song, eof_selected_track);
	for(ctr = 0, index = 0; ctr < tracksize; ctr++)
	{	//For each note in the active track
		if(eof_get_note_type(eof_song, eof_selected_track, ctr) == eof_note_type)
		{	//If the note is in the active difficulty
			unsigned long notepos, notelength, ctr2, endbeat, deltapos, deltalength, startbeat;
			double start, end, interval;

			if(index >= note_count)
			{	//Bounds check
				free(note_measure_positions);
				free(note_beat_lengths);
				if(bestptr)
				{	//If the calling function wanted to initialize two solution structures
					free(best);
				}
				free(testing);
				eof_log("\tNotes miscounted", 1);
				eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
				eof_destroy_ts_list(tslist);		//Free memory used by the TS change list

				return 1;	//Return error
			}

			//Set position and length information
			notepos = eof_get_note_pos(eof_song, eof_selected_track, ctr);
			note_measure_positions[index] = eof_get_measure_position(notepos);	//Store the measure position of the note
			notelength = eof_get_note_length(eof_song, eof_selected_track, ctr);
			start = eof_get_beat(eof_song, notepos) + (eof_get_porpos(notepos) / 100.0);	//The floating point beat position of the start of the note
			endbeat = eof_get_beat(eof_song, notepos + notelength);
			end = (double) endbeat + (eof_get_porpos(notepos + notelength) / 100.0);	//The floating point beat position of the end of the note

			//Determine whether length of the note as it will be in the exported MIDI file is shorter than 1/12 measure, which will determien whether the sustain will be kept in Clone Hero
			deltapos = eof_ConvertToDeltaTime(notepos, anchorlist, tslist, timedivision, 1, has_stored_tempo);	//Store the tick position of the note
			deltalength = eof_ConvertToDeltaTime(notepos + notelength, anchorlist, tslist, timedivision, 0, has_stored_tempo) - deltapos;	//Store the number of delta ticks representing the note's length
			startbeat = eof_get_beat(eof_song, notepos);	//Determine in which beat the note starts

			if((startbeat < eof_song->beats) && (deltalength < (double) EOF_DEFAULT_TIME_DIVISION * eof_song->beat[startbeat]->num_beats_in_measure / 12.0))
			{	//If the length of 1/12 measure (as of this note's start position) could be determined and this note is shorter than that length
				note_beat_lengths[index] = 0.0;	//This sustain will be discarded
			}
			else
			{	//The sustain will be kept
				//Allow for notes that end up to 2ms away from a 1/25 interval to have their beat length rounded to that interval
				interval = eof_get_beat_length(eof_song, end) / 25.0;
				for(ctr2 = 0; ctr2 < 26; ctr2++)
				{	//For each 1/25 beat interval in the note's end beat up until the next beat's position
					if(endbeat < eof_song->beats)
					{	//Error check
						double target = eof_song->beat[endbeat]->fpos + ((double) ctr2 * interval);

						if((notepos + notelength + 2 == (unsigned long) (target + 0.5)) || (notepos + notelength + 1 == (unsigned long) (target + 0.5)))
						{	//If the note ends 1 or 2 ms before this 1/25 beat interval
							end = eof_get_beat(eof_song, notepos + notelength) + ((double) ctr2 / 25.0);	//Re-target the note's end position exactly to this interval
						}
						else if((notepos + notelength - 2 == (unsigned long) (target + 0.5)) || (notepos + notelength - 1 == (unsigned long) (target + 0.5)))
						{	//If the note ends 1 or 2 ms after this 1/25 beat interval
							end = eof_get_beat(eof_song, notepos + notelength) + ((double) ctr2 / 25.0);	//Re-target the note's end position exactly to this interval
						}
					}
				}

				note_beat_lengths[index] = end - start;		//Store the floating point beat length of the note
			}


			index++;
		}//If the note is in the active difficulty
	}//For each note in the active track

	eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
	eof_destroy_ts_list(tslist);		//Free memory used by the TS change list

	///Estimate the maximum number of star power deployments, to limit the number of solutions tested
	sp_phrase_count = 0;
	sp_sustain = 0.0;
	for(index = 0; index < note_count; index++)
	{	//For each note in the active track difficulty
		unsigned long note = eof_translate_track_diff_note_index(eof_song, eof_selected_track, eof_note_type, index);

		if(note < tracksize)
		{	//If the real note number was identified
			unsigned long noteflags = eof_get_note_flags(eof_song, eof_selected_track, note);
			if(noteflags & EOF_NOTE_FLAG_SP)
			{	//If the note has star power
				if(eof_get_note_length(eof_song, eof_selected_track, note) > 1)
				{	//If the note has sustain
					sp_sustain += note_beat_lengths[index];	//Count the number of beats of star power that will be whammied for bonus star power
				}
				if(eof_get_note_tflags(eof_song, eof_selected_track, note) & EOF_NOTE_TFLAG_SP_END)
				{	//If this is the last note in a star power phrase
					sp_phrase_count++;	//Keep count to determine how much star power is gained throughout the track difficulty
				}
			}
		}
	}
	sp_phrase_count += (sp_sustain / 8.0) + 0.0001;	//Each beat of whammied star power sustain awards 1/32 meter of star power (1/8 as much star power as a completed star power phrase), (allow variance for math error)
	max_deployments = sp_phrase_count / 2;			//Two star power phrases' worth of star power is enough to deploy star power once

	///Initialize deployment arrays
	tdeployments = malloc(sizeof(unsigned long) * max_deployments);
	if(bestptr)
	{	//If the calling function wanted to initialize two solution structures
		bdeployments = malloc(sizeof(unsigned long) * max_deployments);
	}
	deploy_cache = malloc(sizeof(EOF_SP_PATH_SCORING_STATE) * max_deployments);
	deployment_endings = malloc(sizeof(double) * max_deployments);

	if(!tdeployments || (bestptr && !bdeployments) || !deploy_cache || !deployment_endings)
	{
		free(note_measure_positions);
		free(note_beat_lengths);
		if(bestptr)
		{	//If the calling function wanted to initialize two solution structures
			free(best);
		}
		free(testing);
		if(tdeployments)
			free(tdeployments);
		if(bdeployments)
			free(bdeployments);
		if(deploy_cache)
			free(deploy_cache);
		if(deployment_endings)
			free(deployment_endings);

		eof_log("\tFailed to allocate memory", 1);
		return 1;	//Return error
	}

	for(ctr = 0; ctr < max_deployments; ctr++)
	{	//For every entry in the deploy cache
		deploy_cache[ctr].note_start = ULONG_MAX;	//Mark it as invalid
	}
	if(bestptr)
	{	//If the calling function wanted to initialize two solution structures
		best->deployments = bdeployments;
		best->deploy_cache = deploy_cache;
		best->deploy_count = max_deployments;
		best->deployment_endings = deployment_endings;
	}
	testing->deployments = tdeployments;
	testing->deploy_cache = deploy_cache;
	testing->deploy_count = max_deployments;
	testing->deployment_endings = deployment_endings;

	//Return prepared solution structures to calling function
	if(bestptr)
	{	//If the calling function wanted to initialize two solution structures
		*bestptr = best;
	}
	*testingptr = testing;

	return 0;	//Return success
}

unsigned long eof_ch_sp_path_calculate_stars(EOF_SP_PATH_SOLUTION *solution, unsigned long *base_score_ptr, unsigned long *effective_score_ptr)
{
	unsigned long ctr, index, tracksize, base_score = 0, note_score, solo_note_count = 0, effective_score, star_count = 0;
	double sustain_score;

	if(!solution)
		return ULONG_MAX;	//Invalid parameters

	///Count the number of solo notes (the total score compared to the base score for awarded star count does not include solo bonuses)
	tracksize = eof_get_track_size(eof_song, solution->track);
	for(ctr = 0; ctr < tracksize; ctr++)
	{	//For each note in the active track
		if(eof_get_note_type(eof_song, solution->track, ctr) == eof_note_type)
		{	//If the note is in the active difficulty
			if(eof_get_note_tflags(eof_song, solution->track, ctr) & EOF_NOTE_TFLAG_SOLO_NOTE)
			{	//If the note was determined to be in a solo section
				solo_note_count++;
			}
		}
	}

	///Calculate the base score of the track difficulty
	for(ctr = 0, index = 0; ctr < tracksize; ctr++)
	{	//For each note in the active track
		if(eof_get_note_type(eof_song, solution->track, ctr) == eof_note_type)
		{	//If the note is in the active difficulty
			note_score = eof_note_count_colors(eof_song, solution->track, ctr) * 50;	//The base score for a note is 50 points per gem
			sustain_score = 25.0 * solution->note_beat_lengths[index];					//The sustain's base score is 25 points per beat
			base_score += note_score + ceil(sustain_score);								//Add the ceilinged sum of those to the base score

			index++;
		}
	}

	effective_score = solution->score - (100 * solo_note_count);	//The score compared against the base score does not include solo bonuses

	///Store values by reference if applicable
	if(base_score_ptr)
		*base_score_ptr = base_score;
	if(effective_score_ptr)
		*effective_score_ptr = effective_score;

	///Return the awarded star count
	if(effective_score >= base_score * 0.1)
		star_count = 1;
	if(effective_score >= base_score * 0.5)
		star_count = 2;
	if(effective_score >= base_score)
		star_count = 3;
	if(effective_score >= base_score * 2)
		star_count = 4;
	if(effective_score >= base_score * 2.8)
		star_count = 5;
	if(effective_score >= base_score * 3.6)
		star_count = 6;
	if(effective_score >= base_score * 4.4)
		star_count = 7;

	return star_count;
}

void eof_ch_sp_path_report_solution(EOF_SP_PATH_SOLUTION *solution, EOF_BIG_NUMBER *validcount, EOF_BIG_NUMBER *invalidcount, unsigned long deployment_notes, char *undo_made)
{
	unsigned long ctr, tracksize;
	unsigned long base_score = 0;
	EOF_BIG_NUMBER solution_count = {0};

	if(!eof_song || !validcount || !invalidcount || !solution || !solution->note_beat_lengths)
		return;	//Invalid parameters

	tracksize = eof_get_track_size(eof_song, eof_selected_track);

	///Report the solution
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tThe highest number of notes that could be played during star power deployment among all solutions was %lu (%.2f%%)", deployment_notes, (double)deployment_notes * 100.0 / solution->note_count);
	eof_log(eof_log_string, 1);
	if(solution->deployment_notes == 0)
	{	//If there was no best use of star power
		eof_log("\tNo notes were playable during a star power deployment", 1);
		allegro_message("No notes were playable during a star power deployment");
	}
	else
	{	//There is at least one note played during star power deployment
		unsigned long notenum, min, sec, ms, flags;
		unsigned long effective_score = 0, star_count;
		char timestamps[500], indexes[500], tempstring[20], scorestring[50], sololess_scorestring[50], multiplierstring[50], base_score_string[50];
		char *detected_solution_string = "Optimum star power deployment in Clone Hero for this track difficulty is at these note timestamps (highlighted):";
		char *user_solution_string = "User defined star power deployment in Clone Hero for this track difficulty at these note timestamps:";
		char *resultstring1 = detected_solution_string;	//Unless this function determines a user defined solution is being reported, the detected solution string is displayed
		char *score_disclaimer = "*Note:  This scoring is based on Clone Hero playing the chart as a MIDI, for which it will remove all sustains shorter than 1/12.";
		char deployment_notes_string[100] = {0};

		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t%s", resultstring1);
		eof_log(eof_log_string, 1);
		timestamps[0] = '\0';	//Empty the timestamps string
		indexes[0] = '\0';		//And the indexes string

		for(ctr = 0; ctr < solution->num_deployments; ctr++)
		{	//For each deployment in the best solution
			notenum = eof_translate_track_diff_note_index(eof_song, solution->track, solution->diff, solution->deployments[ctr]);	//Find the real note number for this index
			if(notenum >= tracksize)
			{	//If the note was not identified
				eof_log("\tLogic error displaying solution.", 1);
				allegro_message("Logic error displaying solution.");
				break;
			}

			//Build string of deployment timestamps
			ms = eof_get_note_pos(eof_song, eof_selected_track, notenum);	//Determine mm:ss.ms formatting of timestamp
			min = ms / 60000;
			ms -= 60000 * min;
			sec = ms / 1000;
			ms -= 1000 * sec;
			(void) snprintf(tempstring, sizeof(tempstring) - 1, "%s%lu:%lu.%lu", (ctr ? ", " : ""), min, sec, ms);	//Generate timestamp, prefixing with a comma and spacing after the first
			strncat(timestamps, tempstring, sizeof(timestamps) - 1);	//Append to timestamps string

			eof_big_number_add_big_number(&solution_count, validcount);		//Count the number of valid and invalid solutions tested
			eof_big_number_add_big_number(&solution_count, invalidcount);
			if(solution_count.overflow_count + solution_count.value > 1)
			{	//If this reporting is for the best detected solution instead of for a single user defined solution, highlight the solution's deployments
				flags = eof_get_note_flags(eof_song, eof_selected_track, notenum);
				if(undo_made && (*undo_made == 0))
				{	//If the calling function passed undo tracking to this function, and an undo state hasn't been made yet
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					*undo_made = 1;
				}
				flags |= EOF_NOTE_FLAG_HIGHLIGHT;	//Enable highlighting for this note
				eof_set_note_flags(eof_song, eof_selected_track, notenum, flags);
				(void) snprintf(deployment_notes_string, sizeof(deployment_notes_string) - 1, "A maximum of %lu notes (%.2f%%) can be played during any SP deployment combination.", deployment_notes, (double)deployment_notes * 100.0 / solution->note_count);
			}
			else
			{	//Otherwise change the dialog's wording to reflect a user defined solution
				resultstring1 = user_solution_string;
				(void) snprintf(deployment_notes_string, sizeof(deployment_notes_string) - 1, "A maximum of %lu notes (%.2f%%) can be played during these SP deployments.", deployment_notes, (double)deployment_notes * 100.0 / solution->note_count);
			}

			(void) snprintf(tempstring, sizeof(tempstring) - 1, "%s%lu", (ctr ? ", " : ""), solution->deployments[ctr]);
			strncat(indexes, tempstring, sizeof(indexes) - 1);	//Append to indexes string
		}
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t%s", timestamps);
		eof_log(eof_log_string, 1);
		eof_log("\tAt these note indexes:", 1);
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t%s", indexes);
		eof_log(eof_log_string, 1);
		(void) snprintf(scorestring, sizeof(scorestring) - 1, "Estimated score:  %lu", solution->score);
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t%s", scorestring);
		eof_log(eof_log_string, 1);

		star_count = eof_ch_sp_path_calculate_stars(solution, &base_score, &effective_score);	//Get the star count, base score and effective score (score minus solo bonuses)

		(void) snprintf(sololess_scorestring, sizeof(sololess_scorestring) - 1, "Without solo bonuses:  %lu", effective_score);
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t%s", sololess_scorestring);
		eof_log(eof_log_string, 1);
		(void) snprintf(base_score_string, sizeof(base_score_string) - 1, "Base score:  %lu", base_score);
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t%s", base_score_string);
		eof_log(eof_log_string, 1);
		(void) snprintf(multiplierstring, sizeof(multiplierstring) - 1, "Average multiplier:  %.2f (%lu star%s)", (double)effective_score / base_score, star_count, (star_count == 1) ? "" : "s");
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t%s", multiplierstring);
		eof_log(eof_log_string, 1);

		(void) eof_detect_difficulties(eof_song, eof_selected_track);	//Update highlighting variables
		eof_render();
		allegro_message("%s\n%s\n\n%s\n%s\n%s\n%s\n%s\n\n%s", resultstring1, timestamps, scorestring, sololess_scorestring, base_score_string, multiplierstring, score_disclaimer, deployment_notes_string);

		///Offer to apply SP deploy status to a detected best solution's notes, if no notes in the active track difficulty already have that status
		if(solution_count.overflow_count + solution_count.value > 1)
		{	//If this reporting is for the best detected solution instead of for a single user defined solution
			int sp_deploy_notes_exist = 0;	//Set to nonzero if any notes are found to have EOF_NOTE_EFLAG_SP_DEPLOY status

			for(ctr = 0; ctr < tracksize; ctr++)
			{	//For each note in the active track
				if(eof_get_note_type(eof_song, eof_selected_track, ctr) == eof_note_type)
				{	//If the note is in the active difficulty
					if(eof_get_note_eflags(eof_song, eof_selected_track, ctr) & EOF_NOTE_EFLAG_SP_DEPLOY)
					{	//If the note has the SP deploy status
						sp_deploy_notes_exist = 1;
						break;
					}
				}
			}

			if(!sp_deploy_notes_exist)
			{	//If no notes were found to have EOF_NOTE_EFLAG_SP_DEPLOY status already
				if(alert(NULL, "Apply SP deploy status to the best solution's notes and remove their highlighting?", NULL, "&Yes", "&No", 'y', 'n') == 1)
				{	//If the user opts to replace the highlighting status with SP deploy status
					for(ctr = 0; ctr < solution->num_deployments; ctr++)
					{	//For each deployment in the best solution
						notenum = eof_translate_track_diff_note_index(eof_song, solution->track, solution->diff, solution->deployments[ctr]);	//Find the real note number for this index
						if(notenum < tracksize)
						{	//If the note was identified
							flags = eof_get_note_flags(eof_song, eof_selected_track, notenum);		//Remove the highlight flag
							flags &= ~EOF_NOTE_FLAG_HIGHLIGHT;
							eof_set_note_flags(eof_song, eof_selected_track, notenum, flags);

							eof_set_note_ch_sp_deploy_status(eof_song, eof_selected_track, notenum, 1, undo_made);	//Apply the Clone Hero SP deploy flag
						}
					}
				}

				(void) eof_detect_difficulties(eof_song, eof_selected_track);	//Update highlighting variables
			}
		}
	}
}

char eof_menu_track_find_ch_sp_path_dialog_string[4];
DIALOG eof_menu_track_find_ch_sp_path_dialog[] =
{
	/* (proc)                (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                         (dp2) (dp3) */
	{ d_agup_window_proc,    0,   0,   360, 186, 0,   0,   0,    0,      0,   0,   "Find optimal CH star power path", NULL, NULL },
	{ d_agup_text_proc,      12,  40,  60,  12,  0,   0,   0,    0,      0,   0,   "Use this many worker processes:",NULL, NULL },
	{ eof_verified_edit_proc,12,  56,  90,  20,  0,   0,   0,    0,      2,   0,   eof_menu_track_find_ch_sp_path_dialog_string, "0123456789", NULL },
	{ d_agup_text_proc,      12,  80,  60,  12,  0,   0,   0,    0,      0,   0,   "More (up to the number of threads your CPU supports)",NULL, NULL },
	{ d_agup_text_proc,      12,  96,  60,  12,  0,   0,   0,    0,      0,   0,   "is faster, but will slow your computer down more.",NULL, NULL },
	{ d_agup_text_proc,      12,  112, 60,  12,  0,   0,   0,    0,      0,   0,   "",NULL, NULL },
	{ d_agup_check_proc,     50,  118, 210, 16,  0,   0,   0,    0,      1,   0,   "Enable worker process logging", NULL, NULL },
	{ d_agup_button_proc,    12,  144, 84,  28,  2,   23,  '\r', D_EXIT, 0,   0,   "OK",                        NULL, NULL },
	{ d_agup_button_proc,    110, 144, 78,  28,  2,   23,  0,    D_EXIT, 0,   0,   "Cancel",                    NULL, NULL },
	{ NULL,                  0,   0,   0,   0,   0,   0,   0,    0,      0,   0,   NULL,                        NULL, NULL }
};

int eof_menu_track_find_ch_sp_path(void)
{
	EOF_SP_PATH_SOLUTION *best = NULL, *testing = NULL;
	unsigned long worst_score = 0;				//The estimated maximum score when no star power is deployed
	unsigned long first_deploy = ULONG_MAX;		//The first note that occurs after the end of the second star power phrase, and is thus the first note at which star power can be deployed
	int worker_logging;
	unsigned long ctr, tracksize;
	EOF_BIG_NUMBER validcount = {0}, invalidcount = {0}, solution_count = {0};
	int error = 0;
	char undo_made = 0;
	clock_t starttime = 0, endtime = 0;
	double elapsed_time = 1.0;			//Don't init to 0 to avoid a division by 0 if an error occurs during processing
	long process_count;
	unsigned long deployment_notes = 0;	//Tracks the highest count of notes that were found to be playable during all star power deployments of any solution

 	eof_log("eof_menu_track_find_ch_sp_path() entered", 1);

 	if(!eof_song)
		return 1;	//No project loaded

///Prompt user how many processes to use to evaluate solutions
	eof_color_dialog(eof_menu_track_find_ch_sp_path_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_menu_track_find_ch_sp_path_dialog);
	if(eof_popup_dialog(eof_menu_track_find_ch_sp_path_dialog, 2) != 7)
	{	//If the user did not click OK
		return 1;
	}
	process_count = atol(eof_menu_track_find_ch_sp_path_dialog_string);
	if(process_count < 1)
	{
		allegro_message("Must specify at number of processes that is 1 or higher.");
		return 1;
	}
	worker_logging = (eof_menu_track_find_ch_sp_path_dialog[6].flags & D_SELECTED) ? 1 : 0;
	if(process_count > 1)
	{	//If the user specified multiple processes
		if(!eof_song->tags->revision)
		{	//If project hasn't been saved even once
			allegro_message("You must save the project after importing a chart in order to use this function with multiple processes.");
			return 1;
		}
		if(eof_changes)
		{	//If there are unsaved changes in the project
			allegro_message("You must save changes to the project before using this function with multiple processes.");
			return 1;
		}
	}
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tEvaluating CH path solution for \"%s\" difficulty %u", eof_song->track[eof_selected_track]->name, eof_note_type);
	eof_log(eof_log_string, 2);
	if(eof_ch_sp_path_setup(&best, &testing, &undo_made))
	{	//If the function that performs setup for the pathing logic failed
		return 1;
	}

	///Determine the maximum score when no star power is deployed
	best->num_deployments = 0;
	eof_log_casual("CH Scoring without star power:", 1, 1, 1);
	if(eof_evaluate_ch_sp_path_solution(best, &solution_count, 2, 0))
	{	//Populate the "best" solution with the worst scoring solution, so any better solution will replace it, verbose log the evaluation
		eof_log("\tError scoring no star power usage", 1);
		allegro_message("Error scoring no star power usage");
		error = 1;	//The testing of all other solutions will be skipped
	}
	else
	{
		worst_score = best->score;
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tEstimated maximum score without deploying star power is %lu.", worst_score);
		eof_log_casual(eof_log_string, 1, 1, 1);
	}
	eof_log_casual(NULL, 1, 1, 1);	//Flush the buffered log writes to disk

	first_deploy = eof_ch_pathing_find_next_deployable_sp(testing, 0);	//Starting from the first note in the target track difficulty, find the first note at which star power can be deployed
	if(first_deploy < testing->note_count)
	{	//If the note was identified
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tFirst possible star power deployment is at note index %lu in this track difficulty.", first_deploy);
		eof_log_casual(eof_log_string, 1, 1, 1);
	}
	else
	{
		eof_log("\tError detecting first available star power deployment", 1);
		error = 1;
	}

	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tEstimated maximum number of star power deployments is %lu", testing->deploy_count);
	eof_log_casual(eof_log_string, 1, 1, 1);
	eof_log_casual(NULL, 1, 1, 1);	//Flush the buffered log writes to disk

	///Test all possible solutions to find the highest scoring one
	if(!error)
	{	//If the no deployments score and first available star power deployment were successfully determined
		starttime = clock();	//Track the start time
		if(process_count == 1)
		{	//Perform a single process evaluation of all solutions
			error = eof_ch_sp_path_single_process_solve(best, testing, first_deploy, ULONG_MAX, &validcount, &invalidcount, &deployment_notes, NULL);
		}
		else
		{	//Perform a multi process evaluation of all solutions
			error = eof_ch_sp_path_supervisor_process_solve(best, testing, first_deploy, process_count, &validcount, &invalidcount, &deployment_notes, worker_logging);
		}
		endtime = clock();	//Track the end time
		elapsed_time = (double)(endtime - starttime) / (double)CLOCKS_PER_SEC;	//Convert to seconds
	}

	///Report best solution
	eof_big_number_add_big_number(&solution_count, &validcount);	//Count the number of valid and invalid solutions tested
	eof_big_number_add_big_number(&solution_count, &invalidcount);
	eof_log_casual(NULL, 1, 1, 1);	//Flush the buffered log writes to disk
	eof_fix_window_title();
	eof_render();
	if(error == 1)
	{
		if(!testing->deploy_count)
		{
			eof_log("\tThere are not enough star power phrases/sustains to deploy even once.", 1);
			allegro_message("There are not enough star power phrases/sustains to deploy even once.");
		}
		else
		{
			eof_log("\tFailed to detect optimum star power path.", 1);
			allegro_message("Failed to detect optimum star power path.");
		}
	}
	else if(error == 2)
	{
		set_window_title("Release Escape key...");
		while(key[KEY_ESC])
		{	//Wait for user to release Escape key
			Idle(10);
		}
		eof_fix_window_title();
		if(solution_count.overflow_count)
		{	//If more than 4 billion solutions were tested
			unsigned long billion_count, leftover;
			double million_count;

			billion_count = (solution_count.overflow_count * 4) + (solution_count.value / 1000000000);
			leftover = solution_count.value % 1000000000;
			million_count = (double)billion_count * 1000.0 + ((double)leftover / 1000000.0);	//The number of millions of solutions tested
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t%lu billion + %lu solutions tested (%luB + %lu valid, %luB + %lu invalid) in %.2f seconds (%f thousand solutions per second)", billion_count, leftover, validcount.overflow_count * 4, validcount.value, invalidcount.overflow_count * 4, invalidcount.value, elapsed_time, million_count * 1000.0 / elapsed_time);
		}
		else
		{
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t%lu solutions tested (%lu valid, %lu invalid) in %.2f seconds (%.2f solutions per second)", solution_count.value, validcount.value, invalidcount.value, elapsed_time, ((double)solution_count.value)/elapsed_time);
		}
		eof_log("\tUser cancellation.", 1);
		eof_log(eof_log_string, 1);
		allegro_message("User cancellation.%s", eof_log_string);
	}
	else
	{
		//If logging is enabled, verbose log the best solution
		if(eof_log_level)
		{
			//Prep the testing array to hold the best solution for evaluation and logging
			memcpy(testing->deployments, best->deployments, sizeof(unsigned long) * best->num_deployments);
			testing->num_deployments = best->num_deployments;
			for(ctr = 0; ctr < testing->deploy_count; ctr++)
			{	//For each entry in the deploy cache
				testing->deploy_cache[ctr].note_start = ULONG_MAX;	//Invalidate the entry
			}
			eof_log_casual("Best solution:", 1, 1, 1);
			if(process_count > 1)
			{	//If multiple worker processes were used to find the best solution
				best->solution_number.value = ULONG_MAX;	//Don't log a solution number since it has no meaning in this context
			}
			(void) eof_evaluate_ch_sp_path_solution(testing, &best->solution_number, 2, 0);
		}
		eof_log_casual(NULL, 1, 1, 1);	//Flush the buffered log writes to disk, and use the normal logging function from here

		if((best->deployment_notes == 0) && (best->score > worst_score))
		{	//If the best score did not reflect notes being played in star power, but the score somehow was better than the score from when no star power deployment was tested
			eof_log("\tLogic error calculating solution.", 1);
			allegro_message("Logic error calculating solution.");
		}
		else
		{	//The score was determined
			if(solution_count.overflow_count)
			{	//If more than 4 billion solutions were tested
				unsigned long billion_count, leftover;
				double million_count;

				billion_count = (solution_count.overflow_count * 4) + (solution_count.value / 1000000000);
				leftover = solution_count.value % 1000000000;
				million_count = (double)billion_count * 1000.0 + ((double)leftover / 1000000.0);	//The number of millions of solutions tested
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t%lu billion + %lu solutions tested (%luB + %lu valid, %luB + %lu invalid) in %.2f seconds (%f thousand solutions per second)", billion_count, leftover, validcount.overflow_count * 4, validcount.value, invalidcount.overflow_count * 4, invalidcount.value, elapsed_time, million_count * 1000.0 / elapsed_time);
			}
			else
			{
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t%lu solutions tested (%lu valid, %lu invalid) in %.2f seconds (%.2f solutions per second)", solution_count.value, validcount.value, invalidcount.value, elapsed_time, ((double)solution_count.value)/elapsed_time);
			}
			eof_log(eof_log_string, 1);

			eof_ch_sp_path_report_solution(best, &validcount, &invalidcount, deployment_notes, &undo_made);
		}
	}

	///Cleanup
	eof_destroy_sp_solution(testing);
	free(best->deployments);
	free(best);

	tracksize = eof_get_track_size(eof_song, eof_selected_track);
	for(ctr = 0; ctr < tracksize; ctr++)
	{	//For each note in the active track
		unsigned long tflags = eof_get_note_tflags(eof_song, eof_selected_track, ctr);

		tflags &= ~EOF_NOTE_TFLAG_SOLO_NOTE;	//Clear the temporary flags used by the pathing logic
		tflags &= ~EOF_NOTE_TFLAG_SP_END;
		eof_set_note_tflags(eof_song, eof_selected_track, ctr, tflags);
	}

	eof_destroy_sp_solution(eof_ch_sp_solution);	//Destroy the SP solution structure so it's rebuilt
	eof_ch_sp_solution = NULL;

	return 1;
}

void eof_ch_sp_path_worker(char *job_file)
{
	PACKFILE *inf, *outf;
	char filename[1024];
	char jobreadyfilename[1024];
	char jobkillfilename[1024];
	char project_path[1024] = {0};
	int error = 0, canceled = 0, done = 0, idle = 0, retval, firstjob = 1;
	EOF_SP_PATH_SOLUTION best = {0}, testing = {0};
	unsigned long ctr, first_deploy = ULONG_MAX, last_deploy = ULONG_MAX, deployment_notes = 0;
	EOF_BIG_NUMBER validcount = {0}, invalidcount = {0};

	//Validate initial job file path
	eof_log_cwd();
	if(!job_file || !exists(job_file))
	{	//Invalid parameter
		error = 1;
		if(ch_sp_path_worker_logging)
		{
			eof_log("\tCan't find job", 1);
		}
		else
		{
			(void) snprintf(filename, sizeof(filename) - 1, "%scantfindjob", eof_temp_path_s);
			outf = pack_fopen(filename, "w");	//Create that file
			(void) pack_fclose(outf);
		}

		return;
	}
	(void) replace_extension(jobreadyfilename, job_file, "jobready", 1024);		//Create the path to the file indicating the job file is ready to read
	(void) replace_extension(jobkillfilename, job_file, "kill", 1024);			//Create the path to the kill signal file

	//Worker main loop
	while(EOF_PERSISTENT_WORKER && !done && !error && !canceled)
	{	//If the worker process is to run until signaled to end by the supervisor, run until the worker's job is completed
		if(exists(jobreadyfilename))
		{	//If the supervisor indicated that a job file is ready to access
			if(!firstjob)
			{	//If this isn't the first job, re-initialize variables
				validcount.overflow_count = validcount.value = invalidcount.overflow_count = invalidcount.value = 0;
				for(ctr = 0; ctr < best.deploy_count; ctr++)
				{	//For every entry in the deploy cache
					best.deploy_cache[ctr].note_start = ULONG_MAX;	//Mark it as invalid
				}
			}

			//Parse the job file
			if(ch_sp_path_worker_logging)
			{
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Loading job file \"%s\"", job_file);
				eof_log(eof_log_string, 1);
			}
			inf = pack_fopen(job_file, "rb");
			if(!inf)
			{	//If the job file can't be loaded
				error = 1;
				if(ch_sp_path_worker_logging)
				{
					eof_log("\tCan't load job", 1);
				}
				else
				{
					(void) snprintf(filename, sizeof(filename) - 1, "%scantloadjob", eof_temp_path_s);
					outf = pack_fopen(filename, "w");	//Create that file
					(void) pack_fclose(outf);
				}
				break;
			}

			if(firstjob)
			{	//If this is the worker's first job file, read common job data
				if(eof_load_song_string_pf(project_path, inf, sizeof(project_path)))
				{	//If the project file path can't be read
					error = 1;
					if(ch_sp_path_worker_logging)
					{
						eof_log("\tCant read project path", 1);
					}
					else
					{
						(void) snprintf(filename, sizeof(filename) - 1, "%scantreadprojectpath", eof_temp_path_s);
						outf = pack_fopen(filename, "w");	//Create that file
						(void) pack_fclose(outf);
					}
					break;
				}

				eof_song = eof_load_song(project_path);
				if(!eof_song)
				{	//If the project file couldn't be loaded
					error = 1;
					if(ch_sp_path_worker_logging)
					{
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tCan't load project \"%s\"", project_path);
						eof_log(eof_log_string, 1);
					}
					else
					{
						(void) snprintf(filename, sizeof(filename) - 1, "%scantloadproject", eof_temp_path_s);
						outf = pack_fopen(filename, "w");	//Create that file
						(void) pack_fclose(outf);
					}
					break;
				}

				//Finish setup after loading project
				eof_song_loaded = 1;
				eof_init_after_load(0);
				eof_fixup_notes(eof_song);

				//Initialize solution array with job file contents
				best.deploy_count = pack_igetl(inf);
				testing.deploy_count = best.deploy_count;
				best.deployments = malloc(sizeof(unsigned long) * best.deploy_count);
				testing.deployments = malloc(sizeof(unsigned long) * best.deploy_count);
				best.deploy_cache = malloc(sizeof(EOF_SP_PATH_SCORING_STATE) * best.deploy_count);
				testing.deploy_cache = best.deploy_cache;
				best.deployment_endings = malloc(sizeof(double) * best.deploy_count);
				testing.deployment_endings = best.deployment_endings;
				if(best.deploy_cache)
				{	//If the deploy cache array was allocated
					for(ctr = 0; ctr < best.deploy_count; ctr++)
					{	//For every entry in the deploy cache
						best.deploy_cache[ctr].note_start = ULONG_MAX;	//Mark it as invalid
					}
				}

				best.note_count = pack_igetl(inf);
				testing.note_count = best.note_count;
				best.note_measure_positions = malloc(sizeof(double) * best.note_count);
				testing.note_measure_positions = best.note_measure_positions;
				if(best.note_measure_positions)
				{	//If the measure positions array was allocated
					for(ctr = 0; ctr < best.note_count; ctr++)
					{	//For every entry in the measure position array
						double temp = 0.0;

						if(pack_fread(&temp, (long) sizeof(double), inf) != (long) sizeof(double))
						{	//If the double floating point value was not read
							error = 1;
							if(ch_sp_path_worker_logging)
							{
								eof_log("\tCan't read floating point position value", 1);
							}
							else
							{
								(void) snprintf(filename, sizeof(filename) - 1, "%scantreadfppos", eof_temp_path_s);
								outf = pack_fopen(filename, "w");	//Create that file
								(void) pack_fclose(outf);
							}
							break;
						}
						else
						{
							best.note_measure_positions[ctr] = temp;
						}
					}
				}
				if(error)	//If the floating point positions failed to be read
					break;

				best.note_beat_lengths = malloc(sizeof(double) * best.note_count);
				testing.note_beat_lengths = best.note_beat_lengths;
				if(best.note_beat_lengths)
				{	//If the note lengths array was allocated
					for(ctr = 0; ctr < best.note_count; ctr++)
					{	//For every entry in the note lengths array
						double temp = 0.0;

						if(pack_fread(&temp, (long) sizeof(double), inf) != (long) sizeof(double))
						{	//If the double floating point value was not read
							error = 1;
							if(ch_sp_path_worker_logging)
							{
								eof_log("\tCan't read floating point length value", 1);
							}
							else
							{
								(void) snprintf(filename, sizeof(filename) - 1, "%scantreadfplength", eof_temp_path_s);
								outf = pack_fopen(filename, "w");	//Create that file
								(void) pack_fclose(outf);
							}
							break;
						}
						else
						{
							best.note_beat_lengths[ctr] = temp;
						}
					}
				}
				if(error)	//If the floating point lengths failed to be read
					break;

				best.track = pack_igetl(inf);
				testing.track = best.track;
				best.diff = pack_igetw(inf);
				testing.diff = best.diff;

				if(!best.deployments || !testing.deployments || !best.deploy_cache || !best.note_measure_positions || !best.note_beat_lengths || !best.deployment_endings)
				{	//If any of the arrays failed to allocate
					error = 1;
					if(ch_sp_path_worker_logging)
					{
						eof_log("\tCan't allocate memory", 1);
					}
					else
					{
						(void) snprintf(filename, sizeof(filename) - 1, "%scantallocate", eof_temp_path_s);
						outf = pack_fopen(filename, "w");	//Create that file
						(void) pack_fclose(outf);
					}
				}
			}//If this is the worker's first job file, read common job data

			//Read the unique job data (the first and last numbers of the range of solution sets to test)
			first_deploy = pack_igetl(inf);
			last_deploy = pack_igetl(inf);
			(void) pack_fclose(inf);	//Close the job file

			//Initialize structures
			best.num_deployments = testing.num_deployments = 0;
			best.score = testing.score = 0;

			//Signal to the supervisor that the worker process will begin testing solutions
			(void) replace_extension(filename, job_file, "running", (int) sizeof(filename));	//Build the path to this signal file
			outf = pack_fopen(filename, "w");	//Create that file
			(void) pack_fclose(outf);
			idle = 0;
		}//If a job file is present, parse it
		else
		{	//If no job is present, check for the kill signal from the supervisor
			(void) replace_extension(filename, job_file, "kill", 1024);		//Create the path to the kill signal file
			if(exists(filename))
			{	//If this signal file is present
				if(ch_sp_path_worker_logging)
				{
					eof_log("\tKill signal detected", 1);
				}
				done = 1;
				(void) delete_file(filename);
			}
		}

		//Test solutions
		if(!error && !done && !idle)
		{	//If an error or kill signal hasn't occurred yet, and the worker is not waiting for a job
			if(firstjob)
			{	//If this is the first job being processed, set the target track difficulty and apply temporary flags as appropriate
				eof_selected_track = best.track;
				eof_note_type = best.diff;
				(void) eof_detect_difficulties(eof_song, eof_selected_track);
				eof_ch_pathing_mark_tflags(&testing);
			}

			//Test the specified range of solutions
			if(ch_sp_path_worker_logging)
			{
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tTesting solution sets #%lu through #%lu", first_deploy, last_deploy);
				eof_log(eof_log_string, 1);
			}
			retval = eof_ch_sp_path_single_process_solve(&best, &testing, first_deploy, last_deploy, &validcount, &invalidcount, &deployment_notes, jobkillfilename);
			if(retval == 1)
			{	//If the solution testing failed
				error = 1;
				if(ch_sp_path_worker_logging)
				{
					eof_log("\tSolving failed", 1);
				}
				else
				{
					(void) snprintf(filename, sizeof(filename) - 1, "%ssolvefailed", eof_temp_path_s);
					outf = pack_fopen(filename, "w");	//Create that file
					(void) pack_fclose(outf);
				}
			}
			else if(retval == 2)
			{	//If the testing was canceled
				eof_log("\tUser cancellation", 1);
				canceled = 1;
			}
			firstjob = 0;	//All jobs after the first will re-use common data so subsequent job files can eliminate this redundant information
		}//If an error or kill signal hasn't occurred yet, and the worker is not waiting for a job

		if(error)
		{	//If the job was not successfully processed
			(void) replace_extension(filename, job_file, "fail", 1024);		//Create a results file to specify failure
			outf = pack_fopen(filename, "w");
			(void) pack_fclose(outf);	//Close the results file
			idle = 0;
		}
		else if(canceled)
		{	//If the user canceled the job during solution evaluation
			(void) replace_extension(filename, job_file, "cancel", 1024);	//Create a results file to specify cancellation
			outf = pack_fopen(filename, "w");
			if(outf)
			{
				(void) pack_iputl(validcount.overflow_count, outf);
				(void) pack_iputl(validcount.value, outf);
				(void) pack_iputl(invalidcount.overflow_count, outf);
				(void) pack_iputl(invalidcount.value, outf);
				(void) pack_fclose(outf);	//Close the results file
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tCancel status file \"%s\" written", filename);
				eof_log(eof_log_string, 1);
				(void) eof_validate_temp_folder();
			}
			idle = 0;
		}
		else if(done)
		{	//If the supervisor commanded the worker to stop
			(void) replace_extension(filename, job_file, "dead", 1024);	//Create a results file to specify the worker will terminate
			outf = pack_fopen(filename, "w");
			(void) pack_fclose(outf);	//Close the results file
			idle = 0;
		}
		else if(!idle)
		{	//Normal completion, write best solution information to disk
			if(ch_sp_path_worker_logging)
			{
				char tempstring[30];

				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tJob (solution sets #%lu-#%lu) completed.  Best solution:  Deploy at indexes ", first_deploy, last_deploy);
				for(ctr = 0; ctr < testing.num_deployments; ctr++)
				{	//For each deployment in the chosen solution
					(void) snprintf(tempstring, sizeof(tempstring) - 1, "%s%lu", (ctr ? ", " : ""), testing.deployments[ctr]);
					strncat(eof_log_string, tempstring, sizeof(eof_log_string) - 1);
				}
				(void) snprintf(tempstring, sizeof(tempstring) - 1, ".  Score = %lu", testing.score);
				strncat(eof_log_string, tempstring, sizeof(eof_log_string) - 1);
				eof_log(eof_log_string, 1);
				eof_log("Waiting for next job", 1);
			}
			(void) replace_extension(filename, job_file, "success", 1024);	//Create a results file to contain the best solution
			outf = pack_fopen(filename, "w");
			if(outf)
			{
				(void) pack_iputl(best.score, outf);
				(void) pack_iputl(best.deployment_notes, outf);		//Write the number of notes played during star power in the best solution
				(void) pack_iputl(deployment_notes, outf);			//Write the highest count of notes that were found to be playable during all of this job's tested solutions
				(void) pack_iputl(validcount.overflow_count, outf);
				(void) pack_iputl(validcount.value, outf);
				(void) pack_iputl(invalidcount.overflow_count, outf);
				(void) pack_iputl(invalidcount.value, outf);
				(void) pack_iputl(best.num_deployments, outf);
				for(ctr = 0; ctr < best.num_deployments; ctr++)
				{	//For each deployment in the solution
					(void) pack_iputl(best.deployments[ctr], outf);	//Write it to disk
				}
				(void) pack_fclose(outf);				//Close the results file
			}
			(void) delete_file(jobreadyfilename);	//Delete the job file readiness signal
			(void) delete_file(job_file);			//Delete the job file to signal to the supervisor process that the results file is ready to access
			set_window_title("Worker process idle.  Press Esc to cancel");
			idle = 1;	//Until another job file or a kill signal is seen, this worker is in idle status
		}

		if(key[KEY_ESC])
		{
			canceled = 1;
		}
		if(eof_close_button_clicked)
		{	//Consider clicking the close window control a cancellation
			eof_close_button_clicked = 0;
			canceled = 1;
		}
		if(idle)
		{	//If this worker has nothing to do
			Idle(10);	//Wait a bit before checking again
		}
	}//If the worker process is to run until signaled to end by the supervisor, run until the worker's job is completed

	//Cleanup
	if(best.deployments)
		free(best.deployments);
	if(testing.deployments)
		free(testing.deployments);
	if(best.deploy_cache)
		free(best.deploy_cache);
	if(best.note_measure_positions)
		free(best.note_measure_positions);
	if(best.note_beat_lengths)
		free(best.note_beat_lengths);
	if(best.deployment_endings)
		free(best.deployment_endings);

	if(error)
	{	//If the job failed, create a copy of the job file to a new name for troubleshooting
		(void) replace_extension(filename, job_file, "failedjob", (int) sizeof(filename));
		(void) eof_copy_file(job_file, filename);
	}

	(void) delete_file(jobreadyfilename);	//Delete the job file readiness signal
	(void) delete_file(job_file);	//Delete the job file to signal to the supervisor process that the worker is no longer running
}

int eof_ch_sp_path_supervisor_process_solve(EOF_SP_PATH_SOLUTION *best, EOF_SP_PATH_SOLUTION *testing, unsigned long first_deploy, unsigned long worker_count, EOF_BIG_NUMBER *validcount, EOF_BIG_NUMBER *invalidcount, unsigned long *deployment_notes, int worker_logging)
{
	EOF_SP_PATH_WORKER *workers = NULL;
	PACKFILE *fp;
	unsigned long ctr, workerctr, solutionctr, num_workers_running, num_workers_launched = 0, num_jobs_completed = 0, idle_ctr = 1500;
	clock_t start_time, end_time, time1, time2;
	clock_t waiting_overhead = 0;	//Track how many clocks are spent waiting for worker processes to go from waiting to running state
	clock_t solutions_exhausted_time = 0;	//Track the time of the first instance where there is an idle worker process and no solutions let to assign
	char filename[50];	//Stores the job filename for worker processes
	char jobreadyfilename[50];	//Used to create the jobready file for a worker process
	char commandline[1050];	//Stores the command line for launching a worker process
	char exepath[1050] = {0};
	char tempstr[100];
	int done = 0, error = 0, canceled = 0, workerstatuschange;
	double elapsed_time;
	unsigned long trackdiffsize = 0;
	unsigned long job_deploy_notes;	//Used to store a job file's highest count of notes that were found to be playable during all star power deployments of any of the job's solutions

	if(!best || !testing || !validcount || !invalidcount || !worker_count || !deployment_notes)
		return 1;	//Invalid parameters

	eof_log("eof_ch_sp_path_supervisor_process_solve() entered", 1);
	start_time = clock();

	//Initialize temp folder and job file path array
	if(eof_validate_temp_folder())
	{	//Ensure the correct working directory and presence of the temporary folder
		eof_log("\tCould not validate working directory and temp folder", 1);
		return 1;	//Return failure
	}

	//Initialize worker array
	workers = malloc(sizeof(EOF_SP_PATH_WORKER) * worker_count);
	if(!workers)
		return 1;	//Couldn't allocate memory
	for(ctr = 0; ctr < worker_count; ctr++)
	{	//For each worker entry
		workers[ctr].status = EOF_SP_PATH_WORKER_IDLE;
		workers[ctr].assignment_size = ((double) testing->note_count - first_deploy) * 0.005 / worker_count;	//Begin by dividing the first 0.5% of solution sets to all workers evenly
		if(!workers[ctr].assignment_size)
			workers[ctr].assignment_size = 1;	//Job size is a minimum of 1 solution set
		workers[ctr].job_count = 1;
	}

	for(workerctr = 0; workerctr < worker_count; workerctr++)
	{	//For each worker process
		//Delete any previously created status, job and results files for this worker process ID
		(void) snprintf(filename, sizeof(filename) - 1, "%s%lu.job", eof_temp_path_s, workerctr);
		(void) delete_file(filename);	//Delete #.job file
		(void) replace_extension(filename, filename, "jobready", (int) sizeof(filename));
		(void) delete_file(filename);	//Delete #.running file
		(void) replace_extension(filename, filename, "running", (int) sizeof(filename));
		(void) delete_file(filename);	//Delete #.running file
		(void) replace_extension(filename, filename, "fail", (int) sizeof(filename));
		(void) delete_file(filename);	//Delete #.fail file
		(void) replace_extension(filename, filename, "cancel", (int) sizeof(filename));
		(void) delete_file(filename);	//Delete #.cancel file
		(void) replace_extension(filename, filename, "success", (int) sizeof(filename));
		(void) delete_file(filename);	//Delete #.success file
		(void) replace_extension(filename, filename, "kill", (int) sizeof(filename));
		(void) delete_file(filename);	//Delete #.kill file
		(void) replace_extension(filename, filename, "dead", (int) sizeof(filename));
		(void) delete_file(filename);	//Delete #.dead file
	}

	//Delete any existing files used to report worker failure reasons
	(void) replace_extension(filename, filename, "cantfindjob", (int) sizeof(filename));
	(void) delete_file(filename);
	(void) replace_extension(filename, filename, "cantloadjob", (int) sizeof(filename));
	(void) delete_file(filename);
	(void) replace_extension(filename, filename, "cantreadprojectpath", (int) sizeof(filename));
	(void) delete_file(filename);
	(void) replace_extension(filename, filename, "cantloadproject", (int) sizeof(filename));
	(void) delete_file(filename);
	(void) replace_extension(filename, filename, "cantreadfppos", (int) sizeof(filename));
	(void) delete_file(filename);
	(void) replace_extension(filename, filename, "cantreadfplength", (int) sizeof(filename));
	(void) delete_file(filename);
	(void) replace_extension(filename, filename, "cantallocate", (int) sizeof(filename));
	(void) delete_file(filename);
	(void) replace_extension(filename, filename, "solvefailed", (int) sizeof(filename));
	(void) delete_file(filename);

	//Evaluate solutions
	solutionctr = first_deploy;	//The first worker process will be directed to calculate this solution set
	(void) eof_count_selected_notes(&trackdiffsize);	//Count the number of notes in the active track difficulty
	*deployment_notes = 0;			//Reset this count so the first solution will be counted as the one with the most notes played during SP deployments
	while(!done && !error && !canceled)
	{	//Until the supervisor's job is completed
		workerstatuschange = num_workers_running = 0;

		if(!idle_ctr)
		{	//Force the worker statuses to be re-rendered every 1500 idle loop iterations
			workerstatuschange = 1;
			idle_ctr = 1500;
		}

		if(testing->note_count > trackdiffsize)
		{	//If the note count in the testing structure has become corrupted
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tLogic error:  Testing structure note count is corrupted (note_count = %lu, target difficulty note count = %lu)", testing->note_count, trackdiffsize);
			eof_log_casual(eof_log_string, 1, 1, 1);
		}

		//Check worker process status
		for(workerctr = 0; workerctr < worker_count; workerctr++)
		{	//For each worker process
			(void) snprintf(filename, sizeof(filename) - 1, "%s%lu.job", eof_temp_path_s, workerctr);	//Build the file path for the process's job/results file

			if(workers[workerctr].status == EOF_SP_PATH_WORKER_IDLE)
			{	//If this process is waiting for a job
				if(!error && !canceled)
				{	//If the supervisor hasn't detected a failed/canceled status
					if(solutionctr <= testing->note_count)
					{	//If there are solutions left to test
						fp = pack_fopen(filename, "wb");	//Create the job file
						if(!fp)
						{	//If the file couldn't be created
							error = 1;
						}
						else
						{	//The job file was opened for writing
							unsigned long last_deploy;

							//Build the job file
							last_deploy = solutionctr + workers[workerctr].assignment_size;
							if(last_deploy > testing->note_count)
							{	//If this worker is processing the last of the solution sets
								last_deploy = testing->note_count;	//Bounds check this variable so that the correct value is given to the worker
							}
							if(workers[workerctr].job_count == 1)
							{	//If this is the first job being sent to this worker, include extra data that will be re-used in subsequent jobs
								(void) eof_save_song_string_pf(eof_filename, fp);	//Write the path to the EOF project file
								(void) pack_iputl(testing->deploy_count, fp);	//Write the detected maximum number of deployments so the worker can create an appropriately sized depoyment and cache arrays
								(void) pack_iputl(testing->note_count, fp);		//Write the number of notes in the target track difficulty
								for(ctr = 0; ctr < testing->note_count; ctr++)
								{	//For every entry in the note measure position array
									if(pack_fwrite(&testing->note_measure_positions[ctr], (long) sizeof(double), fp) != (long) sizeof(double))	//Write the note measure position
									{	//If the double floating point value was not written
										error = 1;
										break;
									}
								}
								for(ctr = 0; ctr < testing->note_count; ctr++)
								{	//For every entry in the note beat lengths array
									if(pack_fwrite(&testing->note_beat_lengths[ctr], (long) sizeof(double), fp) != (long) sizeof(double))	//Write the note beat length
									{	//If the double floating point value was not written
										error = 1;
										break;
									}
								}
								(void) pack_iputl(testing->track, fp);	//Write the target track
								(void) pack_iputw(testing->diff, fp);	//Write the target difficulty
							}

							(void) pack_iputl(solutionctr, fp);		//Write the beginning of the solution set range the worker is to test
							(void) pack_iputl(last_deploy, fp);		//Write the ending of the solution set range the worker is to test
							(void) pack_fclose(fp);

							(void) replace_extension(jobreadyfilename, filename, "jobready", (int) sizeof(filename));
							fp = pack_fopen(jobreadyfilename, "w");	//Create a file to signal to the worker that the job file is ready to access
							(void) pack_fclose(fp);

							if(EOF_PERSISTENT_WORKER && (workers[workerctr].job_count > 1))
							{	//If the worker process for this job is already running
								(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tDelivered job file \"%s\" (solution set #%lu to #%lu) to existing worker process", filename, solutionctr, last_deploy);
								eof_log_casual(eof_log_string, 1, 1, 1);
							}

							//Launch the worker process if applicable
							if(!error)
							{	//If there hasn't been an I/O error
								if(workers[workerctr].job_count == 1)
								{	//If this job will initiate a worker process
									get_executable_name(exepath, (int) sizeof(exepath) - 1);	//Get the full path to the running EOF executable

									#ifdef ALLEGRO_WINDOWS
										///Windows can easily launch another EOF instance over the command line
										if(worker_logging)
										{	//If the user specified to perform per-worker logging
											(void) snprintf(commandline, sizeof(commandline) - 1, "start /BELOWNORMAL \"\" \"%s\" -ch_sp_path_worker \"%s\" -ch_sp_path_worker_logging", exepath, filename);	//Build the parameters to invoke EOF as a worker process to handle this job file
										}
										else
										{	//Supervisor logging only
											(void) snprintf(commandline, sizeof(commandline) - 1, "start /BELOWNORMAL \"\" \"%s\" -ch_sp_path_worker \"%s\"", exepath, filename);	//Build the parameters to invoke EOF as a worker process to handle this job file
										}

										(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tLaunching worker process #%lu to evaluate solution sets #%lu-#%lu with command line:  %s", workerctr, solutionctr, last_deploy, commandline);
										eof_log_casual(eof_log_string, 1, 1, 1);

										if(eof_system(commandline))
										{	//If the command failed to run
											eof_log_casual("\t\tFailed to launch process", 1, 1, 1);
											error = 1;
											break;
										}
									#else
										///Mac and other *nix based platforms must use a more complicated means of forking the EOF process
										eof_log_casual(NULL, 1, 1, 1);	//Flush the buffered log writes to disk
										pid_t pid = fork();
										if(pid < 0)
										{	//fork() failed
											eof_log_casual("\t\tFailed to fork process", 1, 1, 1);
											error = 1;
											break;
										}
										else if(pid == 0)
										{	//If this is the worker process created by fork()
											eof_stop_logging();	//Prevent the worker process from logging in the supervisor's log
											if(worker_logging)
											{	//If the user specified to perform per-worker logging
												ch_sp_path_worker_logging = 1;			//Signal to eof_start_logging() that this worker process will log
												ch_sp_path_worker_number = workerctr;	//Signal to eof_start_logging() this worker's process number
												eof_start_logging();					//Restart logging to create this worker process's log file
												(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tSuccessfully forked worker process number %lu", workerctr);
												eof_log_casual(eof_log_string, 1, 1, 1);
											}

											//Perform cleanup for items allocated by the supervisor, which the worker won't use
											free(workers);
											eof_destroy_sp_solution(testing);
											free(best->deployments);
											free(best);

											eof_ch_sp_path_worker(filename);	//Divert the process to the worker pathing function
											eof_quit = 1;	//Signal the main function to exit

											return 1;
										}
										else
										{	//This is the original supervisor process that called fork()
											(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tSuccessfully forked EOF process, child PID is %d", pid);
											eof_log_casual(eof_log_string, 1, 1, 1);
										}
									#endif
								}//If this job will initiate a worker process

								num_workers_launched++;
								workers[workerctr].status = EOF_SP_PATH_WORKER_WAITING;
								workers[workerctr].first_deploy = solutionctr;
								workers[workerctr].last_deploy = last_deploy;
								workers[workerctr].start_time = clock();	//Record the start time of the worker process
								workerstatuschange = 1;
								solutionctr = last_deploy + 1;	//Update the counter tracking which solution set to assign next
							}
						}//The job file was opened for writing
					}//If there are solutions left to test
					else
					{	//There are no solutions left to test
						if(!solutions_exhausted_time)
						{	//If this condition hasn't been logged yet
							solutions_exhausted_time = clock();
							elapsed_time = ((double)solutions_exhausted_time - start_time) / CLOCKS_PER_SEC;
							(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tSolutions to test exhausted at %.2f seconds into supervisor process execution.", elapsed_time);
							eof_log_casual(eof_log_string, 1, 1, 1);
						}
						(void) replace_extension(filename, filename, "kill", (int) sizeof(filename));	//Build the path to the file to command the worker process to end
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tSignaling worker process #%lu to terminate", workerctr);
						eof_log_casual(eof_log_string, 1, 1, 1);
						fp = pack_fopen(filename, "w");	//Create that file
						(void) pack_fclose(fp);
						workers[workerctr].status = EOF_SP_PATH_WORKER_STOPPING;
						workerstatuschange = 1;
					}//There are no solutions left to test

					//Check if user canceled an idle worker process, don't consider this a cancellation of the supervisor, just the process
					(void) replace_extension(filename, filename, "dead", (int) sizeof(filename));	//Build the path to this worker's termination status file
					if(exists(filename))
					{
						workers[workerctr].status = EOF_SP_PATH_WORKER_STOPPED;
						workerstatuschange = 1;
					}
				}//If the supervisor hasn't detected a failed/canceled status
			}//If this process is waiting for a job
			else if(workers[workerctr].status == EOF_SP_PATH_WORKER_WAITING)
			{	//If this process was previously dispatched, but hasn't been verified to be running yet
				clock_t current_time = clock();

				num_workers_running++;	//For the purposes of tracking how many workers are currently running, waiting status counts
				(void) replace_extension(filename, filename, "running", (int) sizeof(filename));	//Build the path to this worker's running status file
				if(exists(filename))
				{	//If the worker process created this file to signal that it is processing the job
					(void) delete_file(filename);	//Delete the .running file to acknowledge that the worker is running
					workers[workerctr].status = EOF_SP_PATH_WORKER_RUNNING;
					waiting_overhead += current_time - workers[workerctr].start_time;	//Keep track of how many clocks are spent for worker processes being in waiting state
					workerstatuschange = 1;
				}
				else
				{	//Otherwise check how long it's been since the process was launched
					elapsed_time = (double)(current_time - workers[workerctr].start_time) / (double)CLOCKS_PER_SEC;	//Convert to seconds
					if(elapsed_time > 10.0)
					{	//If the worker process hasn't been detected within 10 seconds
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tFailed to detect worker process #%lu", workerctr);
						eof_log_casual(eof_log_string, 1, 1, 1);
						error = 1;
					}
				}
			}
			else if(workers[workerctr].status == EOF_SP_PATH_WORKER_RUNNING)
			{	//If this process was previously running, check if it's done
				num_workers_running++;	//Track the number of workers in running status
				(void) replace_extension(filename, filename, "job", (int) sizeof(filename));	//Build the path to this worker's job file
				if(!exists(filename))
				{	//If the job file no longer exists, the worker has completed, check the results
					int retry;

					for(retry = 5; retry > 0; retry--)
					{	//Attempt to read the results file up to five times
						(void) replace_extension(filename, filename, "fail", (int) sizeof(filename));	//Build the path to check whether this worker process failed
						if(exists(filename))
						{	//If the worker failed
							(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tDetected failure by worker process #%lu", workerctr);
							eof_log_casual(eof_log_string, 1, 1, 1);
							error = 1;
						}
						else
						{	//If the worker did not create a file indicating failure status
							(void) replace_extension(filename, filename, "cancel", (int) sizeof(filename));	//Build the path to check whether this worker process was canceled
							if(exists(filename))
							{	//If the user canceled the worker
								(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tDetected cancellation by worker process #%lu", workerctr);
								eof_log_casual(eof_log_string, 1, 1, 1);
								canceled = 1;
								workers[workerctr].status = EOF_SP_PATH_WORKER_STOPPED;
								break;	//Break from retry loop
							}
							else
							{	//If the worker was not canceled
								(void) replace_extension(filename, filename, "success", (int) sizeof(filename));	//Build the path to check whether this worker process succeeded
								if(exists(filename))
								{	//If the worker succeeded
									for(ctr = 0; ctr < 5; ctr++)
									{	//Try to read the success file up to five times
										fp = pack_fopen(filename, "rb");
										if(fp)		//If the file was opened
											break;	//exit this loop
										Idle(2);	//Brief wait before retry
									}
									if(!fp)
									{	//If the results couldn't be accessed
										(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tFailed to open \"%s\" results file", filename);
										eof_log_casual(eof_log_string, 1, 1, 1);
										error = 1;
										break;	//Break from retry loop
									}
									else
									{	//The results file was opened
										EOF_BIG_NUMBER vcount, icount, sum = {0};
										char tempstring[30];

										workers[workerctr].end_time = clock();	//Record the completion time of the worker

										//Parse the results
										testing->score = pack_igetl(fp);
										testing->deployment_notes = pack_igetl(fp);
										job_deploy_notes = pack_igetl(fp);
										if(job_deploy_notes > *deployment_notes)
										{	//If this job contained a solution that played more notes during combined star power deployments than any other tested solution
											*deployment_notes = job_deploy_notes;	//Track this count
										}
										vcount.overflow_count = pack_igetl(fp);
										vcount.value = pack_igetl(fp);
										eof_big_number_add_big_number(validcount, &vcount);		//Keep a sum of the valid solutions, account for overflow
										icount.overflow_count = pack_igetl(fp);
										icount.value = pack_igetl(fp);
										eof_big_number_add_big_number(invalidcount, &icount);	//Keep a sum of the invalid solutions, account for overflow

										testing->num_deployments = pack_igetl(fp);	//The number of deployments in the solution
										for(ctr = 0; ctr < testing->num_deployments; ctr++)
										{	//For each deployment in the solution
											testing->deployments[ctr] = pack_igetl(fp);	//Read the deployment's note index
										}
										(void) pack_fclose(fp);

										//Log the results
										eof_big_number_add_big_number(&sum, &vcount);	//Count the number of valid and invalid solutions tested
										eof_big_number_add_big_number(&sum, &icount);
										elapsed_time = (double)(workers[workerctr].end_time - workers[workerctr].start_time) / (double)CLOCKS_PER_SEC;	//Convert to seconds
										if(sum.overflow_count)
										{	//If more than 4 billion solutions were tested
											unsigned long billion_count, leftover;

											billion_count = (sum.overflow_count * 4) + (sum.value / 1000000000);
											leftover = sum.value % 1000000000;
											(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tWorker process %lu completed evaluation of solution sets #%lu-#%lu (job #%lu, %lu billion + %lu solutions) in %.2f seconds.  Its best solution:  Deploy at indexes ", workerctr, workers[workerctr].first_deploy, workers[workerctr].last_deploy, workers[workerctr].job_count, billion_count, leftover, elapsed_time);
										}
										else
										{
											(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tWorker process %lu completed evaluation of solution sets #%lu-#%lu (job #%lu, %lu solutions) in %.2f seconds.  Its best solution:  Deploy at indexes ", workerctr, workers[workerctr].first_deploy, workers[workerctr].last_deploy, workers[workerctr].job_count, sum.value, elapsed_time);
										}
										for(ctr = 0; ctr < testing->num_deployments; ctr++)
										{	//For each deployment in the chosen solution
											(void) snprintf(tempstring, sizeof(tempstring) - 1, "%s%lu", (ctr ? ", " : ""), testing->deployments[ctr]);
											strncat(eof_log_string, tempstring, sizeof(eof_log_string) - 1);
										}
										(void) snprintf(tempstring, sizeof(tempstring) - 1, ".  Score = %lu", testing->score);
										strncat(eof_log_string, tempstring, sizeof(eof_log_string) - 1);
										eof_log_casual(eof_log_string, 1, 1, 1);
										if(elapsed_time < 5.0)
										{	//If the worker completed in less than five seconds
											workers[workerctr].assignment_size *= (5.0 / elapsed_time);	//Scale the job assignment size to take at least five seconds long
										}

										//Compare with current best solution
										if((testing->score > best->score) || ((testing->score == best->score) && (testing->deployment_notes < best->deployment_notes)))
										{	//If this worker's solution is the best solution
											eof_log_casual("\t\t!New best solution", 1, 1, 1);

											best->score = testing->score;	//It is the new best solution, copy its data into the best solution structure
											best->deployment_notes = testing->deployment_notes;
											best->num_deployments = testing->num_deployments;

											if(testing->deployment_notes > *deployment_notes)
											{	//If this solution played more notes during star power than any previous solution
												*deployment_notes = testing->deployment_notes;	//Track this count
											}
											memcpy(best->deployments, testing->deployments, sizeof(unsigned long) * testing->deploy_count);
										}

										//Update the worker status
										workers[workerctr].status = EOF_SP_PATH_WORKER_IDLE;
										workerstatuschange = 1;
										num_jobs_completed++;	//Track the number of completed solution jobs
										if(EOF_PERSISTENT_WORKER)
										{	//If the worker will remain running after completing this job
											workers[workerctr].job_count++;	//Track how many jobs the worker has been assigned, as all job files ffter the first will require less data to be written to file
										}
										break;	//Break from retry loop
									}//The results file was opened
								}//If the worker succeeded
								else
								{	//None of the expected results file names were found
									if(retry)
									{	//If there are read attempts left before giving up
										Idle(2);	//Brief wait before retry
									}
									else
									{
										eof_log_casual("\tWorker failed to report status.", 1, 1, 1);
										error = 1;
										break;	//Break from retry loop
									}
								}
							}//If the worker was not canceled
						}//If the worker did not create a file indicating failure status
					}//Attempt to read the results file up to five times
					if(error)
					{
						set_window_title("SP pathing failed.  Waiting for workers to finish/cancel.");
						workers[workerctr].status = EOF_SP_PATH_WORKER_FAILED;
						workerstatuschange = 1;
					}
					else
					{	//If the expected results file was processed, delete it
						if(!canceled)
						{	//Except for cancel status files, whose tested solution counts will be read later
							(void) delete_file(filename);
						}
					}
				}//If the job file no longer exists, the worker has completed, check the results
			}//If this process was previously dispatched, check if it's done
			else if(workers[workerctr].status == EOF_SP_PATH_WORKER_STOPPING)
			{	//If this process was previously commanded to end, check if it has acknowledged
				(void) replace_extension(filename, filename, "dead", (int) sizeof(filename));	//Build the path to this worker's termination status file
				if(exists(filename))
				{
					workers[workerctr].status = EOF_SP_PATH_WORKER_STOPPED;
					workerstatuschange = 1;
				}
			}
		}//For each worker process

		//Check for user cancellation in the supervisor process
		if(key[KEY_ESC])
		{
			canceled = 1;
		}
		if(eof_close_button_clicked)
		{	//Consider clicking the close window control a cancellation
			eof_close_button_clicked = 0;
			canceled = 1;
		}
		if(canceled)
		{
			workerstatuschange = 1;
		}

		if(!workerstatuschange && !num_workers_running && (solutionctr > testing->note_count))
		{	//If no workers have changed state, none are in running state and all solution sets have already been assigned to workers
			done = 1;
		}
		else if(!workerstatuschange)
		{	//If all workers are in the same status as during the previous check
			(void) snprintf(filename, sizeof(filename) - 1, "%seof.recover.on", eof_temp_path_s);	//Build the path for EOF's recovery status file
			if(!exists(filename))
			{	//If the recovery status file is not seen
				if(eof_validate_temp_folder())
				{	//Ensure the correct working directory and presence of the temporary folder
					eof_log("\tCould not validate working directory and temp folder", 1);
					allegro_message("Error:  Working directory lost.");
					error = 1;
				}
			}
			if(key[KEY_PAUSE])
			{
				EOF_BIG_NUMBER sum = {0};

				elapsed_time = (double)(clock() - start_time) / (double)CLOCKS_PER_SEC;	//Convert to seconds
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tElapsed time is %.2f seconds.", elapsed_time);
				eof_log_casual(eof_log_string, 1, 1, 0);

				eof_big_number_add_big_number(&sum, validcount);	//Count the number of valid and invalid solutions tested
				eof_big_number_add_big_number(&sum, invalidcount);
				if(sum.overflow_count)
				{	//If more than 4 billion solutions were tested
					unsigned long billion_count, leftover;

					billion_count = (sum.overflow_count * 4) + (sum.value / 1000000000);
					leftover = sum.value % 1000000000;
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t%lu billion + %lu solutions have been evaluated.", billion_count, leftover);
				}
				else
				{
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t%lu solutions have been evaluated.", sum.value);
				}
				eof_log_casual(eof_log_string, 1, 0, 1);
				eof_log_casual(NULL, 1, 1, 1);	//Flush the buffered log writes to disk
				eof_log_cwd();
				key[KEY_PAUSE] = 0;
			}
			Idle(10);	//Wait a bit before checking worker process statuses again
			idle_ctr--;
		}
		else
		{
			int ypos = 0;
			eof_log_casual(NULL, 1, 1, 1);	//Flush the buffered log writes to disk

			///Blit each worker status to the screen
			clear_to_color(eof_window_note_upper_left->screen, eof_color_gray);
			for(workerctr = 0; workerctr < worker_count; workerctr++)
			{	//For each worker process
				char *idle_status = "IDLE";
				char *waiting_status = "WAITING";
				char *running_status = "RUNNING";
				char *error_status = "ERROR";
				char *stopping_status = "STOPPING";
				char *stopped_status = "STOPPED";
				char *current_status;

				if(workers[workerctr].status == EOF_SP_PATH_WORKER_IDLE)
					current_status = idle_status;
				else if(workers[workerctr].status == EOF_SP_PATH_WORKER_WAITING)
					current_status = waiting_status;
				else if(workers[workerctr].status == EOF_SP_PATH_WORKER_RUNNING)
					current_status = running_status;
				else if(workers[workerctr].status == EOF_SP_PATH_WORKER_STOPPING)
					current_status = stopping_status;
				else if(workers[workerctr].status == EOF_SP_PATH_WORKER_STOPPED)
					current_status = stopped_status;
				else
					current_status = error_status;

				if(workers[workerctr].status == EOF_SP_PATH_WORKER_RUNNING)
				{	//If the worker process is running, display its assigned solution sets
					if(workers[workerctr].first_deploy == workers[workerctr].last_deploy)
					{	//Worker process is only evaluating one solution set
						(void) snprintf(tempstr, sizeof(tempstr) - 1, "Worker process #%lu status: %s.  Processing job #%lu:  Evaluating solution set #%lu", workerctr, current_status, workers[workerctr].job_count, workers[workerctr].first_deploy);
					}
					else
					{	//Worker process is evaluating multiple solution sets
						(void) snprintf(tempstr, sizeof(tempstr) - 1, "Worker process #%lu status: %s.  Processing job #%lu:  Evaluating solution set #%lu -> #%lu", workerctr, current_status, workers[workerctr].job_count, workers[workerctr].first_deploy, workers[workerctr].last_deploy);
					}
				}
				else
				{	//Otherwise just report the worker process's status
					(void) snprintf(tempstr, sizeof(tempstr) - 1, "Worker process #%lu status: %s", workerctr, current_status);
				}
				textout_ex(eof_window_note_upper_left->screen, eof_font, tempstr, 2, ypos, eof_color_white, -1);
				ypos += 12;
			}
			if(error)
			{
				textout_ex(eof_window_note_upper_left->screen, eof_font, "Error encountered.  Waiting for workers to finish/cancel.", 2, ypos, eof_color_white, -1);
			}
			else if(canceled)
			{
				textout_ex(eof_window_note_upper_left->screen, eof_font, "User cancellation.  Waiting for workers to finish/cancel.", 2, ypos, eof_color_white, -1);
			}
			else
			{
				textout_ex(eof_window_note_upper_left->screen, eof_font, "Press Escape to cancel.", 2, ypos, eof_color_white, -1);
			}
			(void) snprintf(tempstr, sizeof(tempstr) - 1, "%lu worker%s running.  Last assigned solution set is %lu/%lu.", num_workers_running, ((num_workers_running > 1) ? "s" : ""), (solutionctr != first_deploy) ? (solutionctr - 1) : first_deploy, testing->note_count);
			set_window_title(tempstr);
			blit(eof_window_note_upper_left->screen, screen, 0, 0, 0, 0, eof_window_note_upper_left->w, eof_window_note_upper_left->h);	//Render the screen normally
		}
	}//Until the supervisor's job is completed

	//Command all worker processes to end
	for(workerctr = 0; workerctr < worker_count; workerctr++)
	{	//For each worker process
		if((workers[workerctr].status != EOF_SP_PATH_WORKER_STOPPED) && (workers[workerctr].status != EOF_SP_PATH_WORKER_STOPPING))
		{	//If the worker hasn't already been stopped or commanded to stop
			(void) snprintf(filename, sizeof(filename) - 1, "%s%lu.kill", eof_temp_path_s, workerctr);	//Build the file path for the process's kill command
			fp = pack_fopen(filename, "w");	//Create that file
			(void) pack_fclose(fp);
		}
	}

	//If the processing was cancelled, wait for and read the tested solution counts for each worker
	time1 = clock();
	while(canceled)
	{	//If the processing was cancelled
		int all_stopped = 1;	//Set to nonzero if any workers are not already stopped

		for(workerctr = 0; workerctr < worker_count; workerctr++)
		{	//For each worker process
			if(workers[workerctr].status != EOF_SP_PATH_WORKER_STOPPED)
			{	//If the process hasn't been confirmed to have stopped yet
				all_stopped = 0;
				(void) snprintf(filename, sizeof(filename) - 1, "%s%lu.job", eof_temp_path_s, workerctr);	//Build the file path for the process's job file
				if(!exists(filename))
				{	//If the worker has deleted that file
					(void) replace_extension(filename, filename, "cancel", (int) sizeof(filename));		//Build the path for the process's cancel status file
					if(exists(filename))
					{	//If it exists
						fp = pack_fopen(filename, "rb");
						if(fp)
						{	//The results file was opened
							EOF_BIG_NUMBER vcount, icount;

							vcount.overflow_count = pack_igetl(fp);					//Read the number of valid solutions
							vcount.value = pack_igetl(fp);
							eof_big_number_add_big_number(validcount, &vcount);		//Add to the supervisor's ongoing sum of valid solutions, account for overflow
							icount.overflow_count = pack_igetl(fp);					//Read the number of invalid solutions
							icount.value = pack_igetl(fp);
							eof_big_number_add_big_number(invalidcount, &icount);	//Add to the supervisor's ongonig sum of invalid solutions, account for overflow

							workers[workerctr].status = EOF_SP_PATH_WORKER_STOPPED;	//This worker process is confirmed to have stopped
							(void) pack_fclose(fp);
						}
					}
				}
			}
		}

		if(all_stopped)
		{	//If there are no other cancel status files to wait for
			break;	//Exit this while loop
		}
		else
		{	//Otherwise if the threshold amount of time for the files to be recceived hasn't been reached, wait and check for more status files
			time2 = clock();
			if((time2 - time1) / CLOCKS_PER_SEC > EOF_SP_PATH_KILL_FILE_CHECK_INTERVAL + 1)
			{	//If it's been more than a second longer than the interval the workers use to check for a kill command
				break;	//Stop waiting for their status files and report the solution tested counts that are already known and exit this while loop
			}
			Idle(10);	//Wait a bit before checking worker process statuses files again
		}
	}

	end_time = clock();
	elapsed_time = ((double) end_time - start_time) / CLOCKS_PER_SEC;
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tSupervisor finished at %.2f seconds into supervisor process execution.  A total of %lu worker instances were launched and %lu jobs completed, with a combined waiting overhead of %.2f seconds.", elapsed_time, num_workers_launched, num_jobs_completed, (double)waiting_overhead / CLOCKS_PER_SEC);
	eof_log_casual(eof_log_string, 1, 1, 1);

	//Clean up
	free(workers);

	eof_log_casual(NULL, 1, 1, 1);	//Flush the buffered log writes to disk, if any

	if(error)
		return 1;	//Return error
	if(canceled)
		return 2;	//Return cancellation

	return 0;	//Return success
}

int eof_menu_track_evaluate_user_ch_sp_path(void)
{
	EOF_SP_PATH_SOLUTION *testing = NULL;
	unsigned long ctr, index, tracksize, deploy_count;
	char undo_made = 0;
	int error = 0, retval;
	char error_string[100];

 	eof_log("eof_menu_track_evaluate_user_ch_sp_path() entered", 1);

 	if(!eof_song)
		return 1;	//No project loaded

 	///Check whether there's a time signature in effect
	if(!eof_beat_stats_cached)
		eof_process_beat_statistics(eof_song, eof_selected_track);
	if(!eof_song->beat[0]->has_ts)
	{
		allegro_message("A time signature must be in effect on the first beat.  Use \"Beat>Time Signature>\" to apply one.");
		return 1;
	}

	///Initialize the solution structure to reflect the notes in the active track difficulty that have EOF_NOTE_EFLAG_SP_DEPLOY status
	if(eof_ch_sp_path_setup(NULL, &testing, &undo_made))
	{	//If the function that performs setup for the pathing logic failed
		return 1;
	}

	tracksize = eof_get_track_size(eof_song, eof_selected_track);
	for(ctr = 0, index = 0, deploy_count = 0; ctr < tracksize; ctr++)
	{	//For each note in the active track
		if(eof_get_note_type(eof_song, eof_selected_track, ctr) == eof_note_type)
		{	//If the note is in the active difficulty
			if(eof_get_note_eflags(eof_song, eof_selected_track, ctr) & EOF_NOTE_EFLAG_SP_DEPLOY)
			{	//If the note has the SP deploy status
				if(deploy_count >= testing->deploy_count)
				{	//If the user specified more deployments than is possible
					(void) snprintf(error_string, sizeof(error_string) - 1, "Solution invalid:  More than the possible number of %lu star power deployments are specified.", testing->deploy_count);
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t%s", error_string);
					eof_log(eof_log_string, 1);
					allegro_message("%s", error_string);
					error = 1;
					break;
				}

				testing->deployments[deploy_count] = index;	//Record a deployment for this note index
				deploy_count++;
			}
			index++;	//Track the number of notes in the target difficulty that have been encountered
		}
	}
	testing->num_deployments = deploy_count;	//Record the number of deployments for this solution

	///Evaluate the solution
	if(!deploy_count)
	{	//If the user did not specify deployment at any notes
		allegro_message("No star power deployments were defined.  Use functions in the \"Note>Clone Hero>SP deploy>\" menu to specify deployment at desired notes and try again.");
	}
	else if(!error)
	{	//If the user specified deployment count is valid
		EOF_BIG_NUMBER num = {0, ULONG_MAX};	//Initialize this with num.value set to ULONG_MAX to prevent eof_evaluate_ch_sp_path_solution() from logging a solution number

		eof_log_casual("User specified solution:", 1, 1, 1);
		retval = eof_evaluate_ch_sp_path_solution(testing, &num, 2, 0);
		eof_log_casual(NULL, 1, 1, 1);	//Flush the buffered log writes to disk, and use the normal logging function from here

		///Report the solution
		if(retval)
		{	//If the solution failed evaluation
			if(retval == 1)
				(void) snprintf(error_string, sizeof(error_string) - 1, "Testing failed:  Invalid parameters");
			else if(retval == 2)
				(void) snprintf(error_string, sizeof(error_string) - 1, "Testing failed:  Solution invalidated by cache");
			else if(retval == 3)
				(void) snprintf(error_string, sizeof(error_string) - 1, "Invalid solution:  Attempting to deploy while star power is in effect.");
			else if(retval == 4)
				(void) snprintf(error_string, sizeof(error_string) - 1, "Invalid solution:  Attempting to deploy without sufficient star power.");
			else if(retval == 5)
				(void) snprintf(error_string, sizeof(error_string) - 1, "Testing failed:  Logic error.");
			else if(retval == 6)
				(void) snprintf(error_string, sizeof(error_string) - 1, "Testing failed:  Unsupported track format.");
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t%s", error_string);
			eof_log(eof_log_string, 1);
			allegro_message("%s\nCheck EOF's log for more details.", error_string);
		}
		else
		{	//The solution was found valid
			EOF_BIG_NUMBER num2 = {0, 0};

			num.value = 0;
			eof_ch_sp_path_report_solution(testing, &num, &num2, testing->deployment_notes, NULL);	//Report for one tested solution
		}
	}

	///Cleanup
	eof_destroy_sp_solution(testing);

	tracksize = eof_get_track_size(eof_song, eof_selected_track);
	for(ctr = 0; ctr < tracksize; ctr++)
	{	//For each note in the active track
		unsigned long tflags = eof_get_note_tflags(eof_song, eof_selected_track, ctr);

		tflags &= ~EOF_NOTE_TFLAG_SOLO_NOTE;	//Clear the temporary flags used by the pathing logic
		tflags &= ~EOF_NOTE_TFLAG_SP_END;
		eof_set_note_tflags(eof_song, eof_selected_track, ctr, tflags);
	}

	return 1;
}

inline void eof_big_number_add(EOF_BIG_NUMBER *bignum, unsigned long addend)
{
	unsigned long num_until_overflow = EOF_BIG_NUMBER_OVERFLOW_VALUE - bignum->value;	//The lowest addend that will trigger an overflow

	if(num_until_overflow <= addend)
	{	//If this addition will overflow
		addend -= num_until_overflow;	//Add the portion of the addend that will overflow
		bignum->overflow_count++;		//Track that another overflow occurred
		bignum->value = 0;				//Reset the non overflowed value accordingly
	}
	bignum->value += addend;			//Add any remaining amount of the addend to the target big number
}

inline void eof_big_number_increment(EOF_BIG_NUMBER *bignum)
{
	if(bignum->value >= EOF_BIG_NUMBER_OVERFLOW_VALUE - 1)
	{	//If this increment will overflow
		bignum->overflow_count++;	//Track that another overflow occurred
		bignum->value = 0;			//Reset the non overflowed value accordingly
	}
	else
		bignum->value++;			//Otherwise the value of the big number increases by 1
}

inline void eof_big_number_add_big_number(EOF_BIG_NUMBER *bignum, EOF_BIG_NUMBER *addend)
{
	eof_big_number_add(bignum, addend->value);	//Add the addend's non overflowed value and update the target's overflow value if appropriate
	bignum->overflow_count += addend->overflow_count;	//Add the addend's overflow count to the target
}

void eof_set_note_ch_sp_deploy_status(EOF_SONG *sp, unsigned long track, unsigned long note, int function, char *undo_made)
{
	unsigned char sp_deploy;
	unsigned long eflags, tracknum;

	if((sp == NULL) || !track || (track >= sp->tracks) || !undo_made)
		return;	//Invalid parameters
	if(sp->track[track]->track_format != EOF_LEGACY_TRACK_FORMAT)
		return;	//For now, only legacy notes track SP deployment because it's the only track format Clone Hero supports

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	sp_deploy = eof_song->legacy_track[tracknum]->note[note]->sp_deploy;
	if(function < 0)
	{	//If Clone Hero SP deploy status is being toggled for the specified note
		if(*undo_made == 0)
		{	//If an undo state hasn't been made
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			*undo_made = 1;
		}
		sp_deploy ^= 1;		//Toggle the status
	}
	else if(function == 0)
	{	//If Clone Hero SP deploy status is being removed from the specified note
		if((sp_deploy & 1) && (*undo_made == 0))
		{	//If the specified note has CH SP deploy status and an undo state hasn't been made
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			*undo_made = 1;
		}
		sp_deploy &= ~1;	//Clear the status
	}
	else
	{	//Clone Hero SP deploy status is being added to the specified note
		if(!(sp_deploy & 1) && (*undo_made == 0))
		{	//If the specified note does not have CH SP deploy status and an undo state hasn't been made
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			*undo_made = 1;
		}
		sp_deploy |= 1;		//Add the status
	}

	eof_song->legacy_track[tracknum]->note[note]->sp_deploy = sp_deploy;	//Update the deploy status for the specified note
	eflags = eof_get_note_eflags(eof_song, eof_selected_track, note);
	if(sp_deploy)
	{	//If the note has SP deploy status
		eflags |= EOF_NOTE_EFLAG_SP_DEPLOY;	//Ensure the extended status flag for this is set
	}
	else
	{	//The note has no SP deploy status
		eflags &= ~EOF_NOTE_EFLAG_SP_DEPLOY;	//Ensure the extended status flag for this is cleared
	}
	eof_set_note_eflags(eof_song, eof_selected_track, note, eflags);
}

void eof_destroy_sp_solution(EOF_SP_PATH_SOLUTION *ptr)
{
	if(ptr)
	{
		free(ptr->note_measure_positions);
		free(ptr->note_beat_lengths);
		free(ptr->deployments);
		free(ptr->deploy_cache);
		free(ptr->deployment_endings);
		free(ptr);
	}
}

void eof_ch_sp_solution_rebuild(void)
{
	static unsigned long solution_track = 0xFF;		//Records the track for which the solution structure was built
	static unsigned solution_diff = 0xFF;			//Records the difficulty for which the solution structure was built

	//Destroy the solution structure if necessary
	if(eof_ch_sp_solution && ((eof_selected_track != solution_track) || (eof_note_type != solution_diff)))
	{	//If the global solution structure was previously built, but the active track difficulty has changed
		eof_destroy_sp_solution(eof_ch_sp_solution);	//Destroy it so it gets rebuilt
		eof_ch_sp_solution = NULL;
	}

	//Build the solution structure if necessary
	if(eof_ch_sp_solution_macros_wanted || eof_show_ch_sp_durations)
	{	//If any notes panel macros to display star power scoring information are in use, or the user enabled "Show CH SP durations"
		if(!eof_ch_sp_solution)
		{	//If the global solution structure needs to be built
			unsigned long tracksize, ctr, index, deploy_count;
			int error = 0, retval;
			EOF_BIG_NUMBER num = {0, ULONG_MAX};	//Initialize this with num.value set to ULONG_MAX to prevent eof_evaluate_ch_sp_path_solution() from logging a solution number

			//Verify the first beat has a time signature
			if(!eof_beat_stats_cached)
				eof_process_beat_statistics(eof_song, eof_selected_track);	//Rebuild beat statistics if necessary
			if(!eof_song->beat[0]->has_ts)	//If the first beat marker has no time signature
				return;						//Abort

			//Create and initialize the solution structure to reflect the marked SP deployments in the active track difficulty
			if(eof_ch_sp_path_setup(NULL, &eof_ch_sp_solution, NULL))
			{	//If the function that performs setup for the pathing logic failed
				return;	//Abort
			}
			tracksize = eof_get_track_size(eof_song, eof_selected_track);
			for(ctr = 0, index = 0, deploy_count = 0; ctr < tracksize; ctr++)
			{	//For each note in the active track
				if(eof_get_note_type(eof_song, eof_selected_track, ctr) == eof_note_type)
				{	//If the note is in the active difficulty
					if(eof_get_note_eflags(eof_song, eof_selected_track, ctr) & EOF_NOTE_EFLAG_SP_DEPLOY)
					{	//If the note has the SP deploy status
						if(deploy_count >= eof_ch_sp_solution->deploy_count)
						{	//If the user specified more deployments than is possible
							error = 1;	//Processing will be canceled
							break;
						}

						eof_ch_sp_solution->deployments[deploy_count] = index;	//Record a deployment for this note index
						deploy_count++;
					}
					index++;	//Track the number of notes in the target difficulty that have been encountered
				}
			}
			eof_ch_sp_solution->num_deployments = deploy_count;	//Record the number of deployments for this solution
			if(error)	//If the soltuion strucutre couldn't be initialized (ie. had an invalid number of deployments)
			{
				eof_ch_sp_solution->score = 0;	//Ensure the score is reset to 0 to mark this as an invalid solution
				return;	//Abort
			}

			//Evaluate the solution
			retval = eof_evaluate_ch_sp_path_solution(eof_ch_sp_solution, &num, 0, 0);
			if(retval)
			{	//If the evaluation failed for any reason
				eof_ch_sp_solution->score = 0;	//Ensure the score is reset to 0 to mark this as an invalid solution
			}
			solution_track = eof_selected_track;	//Remember which track difficulty the solution data pertains to
			solution_diff = eof_note_type;

			//Cleanup
			for(ctr = 0; ctr < tracksize; ctr++)
			{	//For each note in the active track
				unsigned long tflags = eof_get_note_tflags(eof_song, eof_selected_track, ctr);

				tflags &= ~EOF_NOTE_TFLAG_SOLO_NOTE;	//Clear the temporary flags used by the pathing logic
				tflags &= ~EOF_NOTE_TFLAG_SP_END;
				eof_set_note_tflags(eof_song, eof_selected_track, ctr, tflags);
			}
		}
	}
}
