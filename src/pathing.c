#include "beat.h"
#include "main.h"
#include "midi.h"
#include "note.h"
#include "pathing.h"
#include "song.h"
#include "undo.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

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

unsigned long eof_ch_path_find_next_deployable_sp(EOF_SP_PATH_SOLUTION *solution, unsigned long start_index)
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

int eof_evaluate_ch_sp_path_solution(EOF_SP_PATH_SOLUTION *solution, unsigned long solution_num, int logging)
{
	int sp_deployed = 0;	//Set to nonzero if star power is currently in effect for the note being processed
	unsigned long tracksize, ctr, ctr2;
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

	if(!solution || !solution->deployments || !solution->note_measure_positions || !solution->note_beat_lengths || !eof_song || !solution->track || (solution->track >= eof_song->tracks))
		return 0;	//Invalid parameters

	///Look up cached scoring to skip as much calculation as possible
	for(ctr = 0; ctr < solution->num_deployments; ctr++)
	{	//For each star power deployment in this solution
		if(solution->deployments[ctr] == solution->deploy_cache[ctr].note_start)
		{	//If the star power began at the same time as is called for in this solution
			cache_number = ctr;	//Track the latest cache value that is applicable
		}
	}
	if(cache_number < solution->num_deployments)
	{	//If a valid cache entry was identified
		memcpy(&score, &solution->deploy_cache[cache_number], sizeof(EOF_SP_PATH_SCORING_STATE));	//Copy it into the scoring state structure

		ctr = score.note_end_native;	//Resume parsing from the first note after that cached data
		index = score.note_end;
		deployment_num = cache_number + 1;	//The next deployment will be one higher in number than this cached one
	}
	else
	{	//Start processing from the beginning of the track
		//Init score structure
		score.multiplier = 1;
		score.hitcounter = 0;
		score.sp_meter = score.sp_meter_t = 0.0;

		//Init solution structure
		solution->score = 0;
		solution->deployment_notes = 0;

		ctr = index = 0;	//Start parsing notes from the first note
		deployment_num = 0;
	}

	tracksize = eof_get_track_size(eof_song, solution->track);
	if(eof_log_level > 1)
	{	//Skip logging overhead if the logging level is too low
		unsigned long notenum, min, sec, ms;
		char tempstring[50];
		int cached = 0;

		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tTesting SP path solution #%lu:  Deploy at note indexes ", solution_num);

		for(ctr = 0; ctr < solution->num_deployments; ctr++)
		{	//For each deployment in the solution
			int thiscached = 0;

			notenum = eof_translate_track_diff_note_index(eof_song, solution->track, solution->diff, solution->deployments[ctr]);	//Find the real note number for this index
			if(notenum < tracksize)
			{	//If that number was found
				ms = eof_get_note_pos(eof_song, solution->track, notenum);	//Determine mm:ss.ms formatting of timestamp
				min = ms / 60000;
				ms -= 60000 * min;
				sec = ms / 1000;
				ms -= 1000 * sec;
				if((cache_number < solution->num_deployments) && (cache_number >= ctr))
				{	//If this deployment number is being looked up from cached scoring
					cached = 1;
					thiscached = 1;
				}
				snprintf(tempstring, sizeof(tempstring) - 1, "%s%s%lu (%lu:%lu.%lu)", (ctr ? ", " : ""), (thiscached ? "*" : ""), solution->deployments[ctr], min, sec, ms);
				strncat(eof_log_string, tempstring, sizeof(eof_log_string) - 1);
			}
		}

		if(!solution->num_deployments)
		{	//If there were no star power deployments in this solution
			strncat(eof_log_string, "(none)", sizeof(eof_log_string) - 1);
		}
		eof_log(eof_log_string, 2);

		if(cached)
		{
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tUsing cache for deployment at note index #%lu (Hitcount=%lu  Multiplier=x%lu  Total score=%lu  SP Meter=%lu%%  Uncapped SP Meter=%lu%%).  Resuming from note index #%lu", solution->deploy_cache[cache_number].note_start, score.hitcounter, score.multiplier, score.score, (unsigned long)(score.sp_meter * 100.0 + 0.5), (unsigned long)(score.sp_meter_t * 100.0 + 0.5), index);
			eof_log(eof_log_string, 2);
		}
	}
	for(; ctr < tracksize; ctr++)
	{	//For each note in the track being evaluated
		if(index >= solution->note_count)	//If all expected notes in the target track difficulty have been processed
			break;	//Stop processing notes
		if(eof_get_note_type(eof_song, solution->track, ctr) != solution->diff)
			continue;	//If this note isn't in the target track difficulty, skip it

		base_score = sp_base_score = sustain_score = sp_sustain_score = 0;
		notepos = eof_get_note_pos(eof_song, solution->track, ctr);
		notelength = eof_get_note_length(eof_song, solution->track, ctr);
		noteflags = eof_get_note_flags(eof_song, solution->track, ctr);
		disjointed = eof_get_note_eflags(eof_song, solution->track, ctr) & EOF_NOTE_EFLAG_DISJOINTED;

		if(disjointed)
		{	//If this is a disjointed gem, determine if this will be the gem processed for whammy star power bonus
			representative = eof_note_is_last_longest_gem(eof_song, solution->track, ctr);
		}

		///Determine whether star power is to be deployed at this note
		deploy_sp = 0;
		for(ctr2 = 0; ctr2 < solution->num_deployments; ctr2++)
		{
			if(solution->deployments[ctr2] == index)
			{
				deploy_sp = 1;
				break;
			}
		}

		///Determine whether star power is in effect for this note
		if(sp_deployed)
		{	//If star power was in deployment during the previous note, determine if it should still be in effect
			if(solution->note_measure_positions[index] >= sp_deployment_end + 0.0001)
			{	//If this note is beyond the end of the deployment (allow variance for math error)
				if(logging)
				{
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tStar power ended before note #%lu (index #%lu)", ctr, index);
					eof_log(eof_log_string, 1);
				}
				sp_deployed = 0;	//Star power is no longer in effect
				score.note_end = index;		//This note is the first after the star power deployment that was in effect for the previous note
				score.note_end_native = ctr;
				memcpy(&solution->deploy_cache[deployment_num++], &score, sizeof(EOF_SP_PATH_SCORING_STATE));	//Append this scoring data to the cache
			}
			else
			{	//If star power deployment is still in effect
				if(deploy_sp)
				{	//If the solution specifies deploying star power while it's already in effect
					if(logging)
					{
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tAttempted to deploy star power while it is already deployed.  Solution invalid.");
						eof_log(eof_log_string, 1);
					}
					return 0;	//This solution is invalid
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
					score.sp_meter = score.sp_meter_t = 0.0;	//The entire star power meter will be used
					sp_deployed = 1;	//Star power is now in effect
					score.note_start = index;	//Track the note index at which this star power deployment began, for caching purposes
					if(logging)
					{
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tDeploying star power at note #%lu (index #%lu)", ctr, index);
						eof_log(eof_log_string, 1);
					}
				}
				else
				{	//There is insufficient star power
					return 0;	//This solution is invalid
				}
			}
		}

		///Award star power for completing a star power phrase
		tflags = eof_get_note_tflags(eof_song, solution->track, ctr);
		if(tflags & EOF_NOTE_TFLAG_SP_END)
		{	//If this is the last note in a star power phrase
			if(sp_deployed)
			{	//If star power is in effect
				sp_deployment_end += 2.0;	//Extends the end of star power deployment by 2 measures (one fourth of the effect of a full star power meter)
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
		if((noteflags & EOF_NOTE_FLAG_SP) && (notelength > 1) && (sp_deployed))
		{	//If this note has star power, it has sustain and star power is deployed
			double step = 1.0 / 25.0;	//Every 1/25 beat of sustain, a point is awarded.  Evaluate star power gain/loss at every such interval
			double remaining_sustain = solution->note_beat_lengths[index];	//The amount of sustain remaining to be scored for this star power note
			double sp_whammy_gain = 1.0 / 32.0 / 25.0;	//The amount of star power gained from whammying a star power sustain (1/32 meter per beat) during one scoring interval (1/25 beat)
			double realpos = notepos;	//Keep track of the realtime position at each 1/25 beat interval of the note

			base_score = eof_note_count_colors(eof_song, solution->track, ctr) * 50;	//The base score for a note is 50 points per gem
			sp_base_score = base_score;		//Star power adds the same number of bonus points
			notescore = base_score + sp_base_score;
			solution->deployment_notes++;	//Track how many notes are played during star power deployment
			score.sp_meter = (sp_deployment_end - sp_deployment_start) / 8.0;	//Convert remaining star power deployment duration back to star power
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
					return 0;	//Invalidate the solution since it cannot be correctly scored
				}
				sp_drain = 1.0 / 8.0 / eof_song->beat[beat]->num_beats_in_measure / 25.0;	//Star power drains at a rate of 1/8 per measure of beats, and this is the amount of drain for a 1/25 beat interval
				score.sp_meter = score.sp_meter + sp_whammy_gain - sp_drain;	//Update the star power meter to reflect the amount gained and the amount consumed during this 1/25 beat interval
				score.sp_meter_t = score.sp_meter;
				if(score.sp_meter > 1.0)
					score.sp_meter = 1.0;	//Cap the star power meter at 100%

				//Score this sustain interval
				///Double special case:  For disjointed chords, since the whammy bonus star power is only being evaluated for the longest gem, and
				/// sustain points are being scored at the same time, such points have to be awareded for each of the applicable gems in the disjointed chord
				if(remaining_sustain < step)
				{	//If there was less than a point's worth of sustain left, it will be scored and then rounded to nearest point
					double floatscore;

					realpos = (double) notepos + notelength - 1.0;	//Set the realtime position to the last millisecond of the note
					if(disjointed)
					{	//If this is the longest, representative gem in a disjointed chord, count how many of the chord's gems extend far enough to receive this partial bonus point
						disjointed_multiplier = eof_note_count_gems_extending_to_pos(eof_song, solution->track, ctr, realpos + 0.5);
					}
					floatscore = remaining_sustain / step;	//This represents the fraction of one point this remaining sustain is worth
					if(score.sp_meter > 0.0)
					{	//If star power is still in effect
						floatscore = (2 * disjointed_multiplier);	//This remainder of sustain score is doubled due to star power bonus
						sp_sustain_score += floatscore + 0.5;		//For logging purposes, consider this a SP sustain point (since a partial sustain point and a partial SP sustain point won't be separately tracked)
					}
					else
					{
						floatscore = disjointed_multiplier;			//This remainder of sustain score does not get a star power bonus
						sustain_score += floatscore + 0.5;			//Track how many non star power sustain points were awarded
					}
					notescore += (unsigned long) (floatscore + 0.5);	//Round this remainder of sustain score to the nearest point
				}
				else
				{	//Normal scoring
					realpos += eof_get_beat_length(eof_song, beat) / 25.0;	//Update the realtime position by 1/25 of the length of the current beat the interval is in
					if(disjointed)
					{	//If this is the longest, representative gem in a disjointed chord, count how many of the chord's gems extend far enough to receive this bonus point
						disjointed_multiplier = eof_note_count_gems_extending_to_pos(eof_song, solution->track, ctr, realpos + 0.5);
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
			}
			else
			{	//Otherwise the star power deployment has ended
				score.sp_meter = score.sp_meter_t = 0.0;
				sp_deployed = 0;
				score.note_end = index + 1;	//The next note will be the first after this ended star power deployment
				score.note_end_native = ctr + 1;
				memcpy(&solution->deploy_cache[deployment_num++], &score, sizeof(EOF_SP_PATH_SCORING_STATE));	//Append this scoring data to the cache
				if(logging)
				{
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tStar power ended during note #%lu (index #%lu)", ctr, index);
					eof_log(eof_log_string, 1);
				}
			}
			notescore *= score.multiplier;	//Apply the current score multiplier in effect
		}//If this note has star power, it has sustain and star power is deployed
		else
		{	//Score the note and evaluate whammy star power gain separately
			double sustain = 0.0;	//The score for the portion of the current note's sustain that is subject to star power bonus
			double sustain2 = 0.0;	//The score for the portion of the current note's sustain that occurs after star power deployment ends mid-note (which is not awarded star power bonus)

			///Add any sustained star power note whammy bonus
			if(noteflags & EOF_NOTE_FLAG_SP)
			{	//If this note has star power
				if(notelength > 1)
				{	//If it has sustain
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
			base_score = eof_note_count_colors(eof_song, solution->track, ctr) * 50;	//The base score for a note is 50 points per gem
			notescore = base_score;
			if(notelength > 1)
			{	//If this note has any sustain, determine its score and whether any of that sustain score is not subject to SP bonus due to SP deployment ending in the middle of the sustain (score2)
				sustain2 = 25.0 * solution->note_beat_lengths[index];	//The sustain's base score is 25 points per beat

				if(solution->note_beat_lengths[index] >= 1.000 + 0.0001)
				{	//If this note length is one beat or longer (allowing variance for math error), only the portion occurring before the end of star power deployment is awarded SP bonus
					double sustain_end = eof_get_measure_position(notepos + notelength);

					if(sustain_end > sp_deployment_end)
					{	//If this sustain note ends after the ending of the star power deployment
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
				sustain_score = sustain + sustain2;	//Track how many of the points are being awarded for the sustain
				if(sp_deployed)			//If star power is in effect
				{
					sp_sustain_score = sustain;	//Track how many star power bonus points are being awarded for the sustain
					sustain *= 2.0;		//Apply star power bonus to the applicable portion of the sustain
				}
			}
			if(sp_deployed)				//If star power is in effect
			{
				notescore *= 2;					//It doubles the note's score
				sp_base_score = base_score;
				solution->deployment_notes++;	//Track how many notes are played during star power deployment
			}
			notescore += (sustain + sustain2 + 0.5);	//Round the sustain point total to the nearest integer and add to the note's score
			notescore *= score.multiplier;			//Apply the current score multiplier in effect
		}//Score the note and evaluate whammy star power gain separately

		///Update solution score
		is_solo = (tflags & EOF_NOTE_TFLAG_SOLO_NOTE) ? 1 : 0;	//Check whether this note was determined to be in a solo section
		if(is_solo)
		{	//If this note was flagged as being in a solo
			notescore += 100;	//It gets a bonus (not stackable with star power or multiplier) of 100 points if all notes in the solo are hit
		}
		solution->score += notescore;

		if(logging)
		{
			if(!sp_base_score)
			{	//No star power bonus
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tNote #%lu (index #%lu):  \tpos = %lums, \tbase = %lu, \tsustain = %lu, mult = x%lu, solo bonus = %lu, \tscore = %lu.  \tTotal score:  %lu\tSP Meter at %lu%% (uncapped %lu%%)", ctr, index, notepos, base_score, sustain_score, score.multiplier, is_solo * 100, notescore, solution->score, (unsigned long)(score.sp_meter * 100.0 + 0.5), (unsigned long)(score.sp_meter_t * 100.0 + 0.5));
			}
			else
			{	//Star power bonus
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tNote #%lu (index #%lu):  \tpos = %lums, \tbase = %lu, \tsustain = %lu, SP = %lu, SP sustain = %lu, mult = x%lu, solo bonus = %lu, \tscore = %lu.  \tTotal score:  %lu\tSP Meter at %lu%% (uncapped %lu%%)", ctr, index, notepos, base_score, sustain_score, sp_base_score, sp_sustain_score, score.multiplier, is_solo * 100, notescore, solution->score, (unsigned long)(score.sp_meter * 100.0 + 0.5), (unsigned long)(score.sp_meter_t * 100.0 + 0.5));
			}
			eof_log(eof_log_string, 1);
		}

		///Update multiplier
		score.hitcounter++;
		if(score.hitcounter == 30)
		{
			if(logging)
			{
				eof_log("\t\t\tMultiplier increased to x4", 1);
			}
			score.multiplier = 4;
		}
		else if(score.hitcounter == 20)
		{
			if(logging)
			{
				eof_log("\t\t\tMultiplier increased to x3", 1);
			}
			score.multiplier = 3;
		}
		else if(score.hitcounter == 10)
		{
			if(logging)
			{
				eof_log("\t\t\tMultiplier increased to x2", 1);
			}
			score.multiplier = 2;
		}

		index++;	//Keep track of the number of notes in this track difficulty that were processed
	}//For each note in the track being evaluated

	return 1;	//Return solution evaluated
}

int eof_menu_track_find_ch_sp_path(void)
{
	EOF_SP_PATH_SOLUTION *best, *testing;
	unsigned long note_count = 0;				//The number of notes in the active track difficulty, which will be the size of the various note arrays
	double *note_measure_positions;				//The position of each note in the active track difficulty defined in measures
	double *note_beat_lengths;					//The length of each note in the active track difficulty defined in beats
	unsigned char *sp_phrase_endings;			//The boolean status of whether each note in the active track difficulty is the last note in a star power phrase
	unsigned long worst_score = 0;				//The estimated maximum score when no star power is deployed
	unsigned long first_deploy = ULONG_MAX;		//The first note that occurs after the end of the second star power phrase, and is thus the first note at which star power can be deployed
	unsigned long *tdeployments, *bdeployments;	//An array defining the note index number of each deployment, ie deployments[0] being the first SP deployment, deployments[1] being the second, etc.
	EOF_SP_PATH_SCORING_STATE *deploy_cache;	//An array storing cached scoring data for star power deployments
	unsigned long ctr, ctr2, index, tracksize, numsolos;
	unsigned long validcount = 0, invalidcount = 0;
	EOF_PHRASE_SECTION *sectionptr;
	int error = 0;
	char undo_made = 0;
	clock_t starttime = 0, endtime = 0;
	double elapsed_time;
	unsigned long sp_phrase_count;
	double sp_sustain;				//The number of beats of star power sustain, used to count whammy bonus star power when estimating the maximum number of star power deployments
	unsigned long max_deployments;	//The estimated maximum number of star power deployments, based on the amount of star power note sustain and the number of star power phrases
	char windowtitle[101] = {0};

 	eof_log("eof_menu_track_find_ch_sp_path() entered", 1);

 	///Ensure there's a time signature in effect
	if(!eof_beat_stats_cached)
		eof_process_beat_statistics(eof_song, eof_selected_track);
	if(!eof_song->beat[0]->has_ts)
	{
		allegro_message("A time signature is required for the chart, but none in effect on the first beat.  4/4 will be applied.");

		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		undo_made = 1;
		eof_apply_ts(4, 4, 0, eof_song, 0);
		eof_process_beat_statistics(eof_song, eof_selected_track);	//Recalculate beat statistics
	}

	///Initialize arrays and structures
	(void) eof_count_selected_notes(&note_count);	//Count the number of notes in the active track difficulty

	if(!note_count)
		return 1;	//Don't both doing anything if there are no notes in the active track difficulty

	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tEvaluating CH path solution for \"%s\" difficulty %u", eof_song->track[eof_selected_track]->name, eof_note_type);
	eof_log(eof_log_string, 2);

	note_measure_positions = malloc(sizeof(double) * note_count);
	note_beat_lengths = malloc(sizeof(double) * note_count);
	sp_phrase_endings = malloc(sizeof(unsigned char) * note_count);
	tdeployments = malloc(sizeof(unsigned long) * note_count);
	bdeployments = malloc(sizeof(unsigned long) * note_count);
	deploy_cache = malloc(sizeof(EOF_SP_PATH_SCORING_STATE) * note_count);
	best = malloc(sizeof(EOF_SP_PATH_SOLUTION));
	testing = malloc(sizeof(EOF_SP_PATH_SOLUTION));

	if(!note_measure_positions || !note_beat_lengths || !sp_phrase_endings || !tdeployments || !bdeployments || !deploy_cache || !best || !testing)
	{	//If any of those failed to allocate
		if(note_measure_positions)
			free(note_measure_positions);
		if(note_beat_lengths)
			free(note_beat_lengths);
		if(sp_phrase_endings)
			free(sp_phrase_endings);
		if(tdeployments)
			free(tdeployments);
		if(bdeployments)
			free(bdeployments);
		if(deploy_cache)
			free(deploy_cache);
		if(best)
			free(best);
		if(testing)
			free(testing);

		eof_log("\tFailed to allocate memory", 1);
		return 1;
	}

	best->deployments = bdeployments;
	best->note_measure_positions = note_measure_positions;
	best->note_beat_lengths = note_beat_lengths;
	best->note_count = note_count;
	best->track = eof_selected_track;
	best->diff = eof_note_type;
	best->deploy_cache = deploy_cache;

	testing->deployments = tdeployments;
	testing->note_measure_positions = note_measure_positions;
	testing->note_beat_lengths = note_beat_lengths;
	testing->note_count = note_count;
	testing->track = eof_selected_track;
	testing->diff = eof_note_type;
	testing->deploy_cache = deploy_cache;

	//Init arrays
	memset(sp_phrase_endings, 0, sizeof(unsigned char) * note_count);
	memset(deploy_cache, ULONG_MAX, sizeof(unsigned long) * note_count);

	///Mark all notes that are in solo phrases with a temporary flag
	///Mark all notes that are the last note in a star power phrase with a different temporary flag
	tracksize = eof_get_track_size(eof_song, eof_selected_track);
	numsolos = eof_get_num_solos(eof_song, eof_selected_track);
	for(ctr = 0; ctr < tracksize; ctr++)
	{	//For each note in the active track
		unsigned long tflags, notepos;

		if(eof_get_note_type(eof_song, eof_selected_track, ctr) != eof_note_type)
			continue;	//If it's not in the active track difficulty, skip it

		notepos = eof_get_note_pos(eof_song, eof_selected_track, ctr);
		tflags = eof_get_note_tflags(eof_song, eof_selected_track, ctr);
		tflags &= ~(EOF_NOTE_TFLAG_SOLO_NOTE | EOF_NOTE_TFLAG_SP_END);	//Clear these temporary flags

		if(eof_note_is_last_in_sp_phrase(eof_song, eof_selected_track, ctr))
		{	//If the note is the last note in a star power phrase
			tflags |= EOF_NOTE_TFLAG_SP_END;	//Set the temporary flag that will track this condition to reduce repeatedly testing for this
		}
		for(ctr2 = 0; ctr2 < numsolos; ctr2++)
		{	//For each solo phrase in the active track
			sectionptr = eof_get_solo(eof_song, eof_selected_track, ctr2);
			if((notepos >= sectionptr->start_pos) && (notepos <= sectionptr->end_pos))
			{	//If the note is in this solo phrase
				tflags |= EOF_NOTE_TFLAG_SOLO_NOTE;	//Set the temporary flag that will track this note being in a solo
				break;
			}
		}
		eof_set_note_tflags(eof_song, eof_selected_track, ctr, tflags);
	}

	///Calculate the measure position and beat length of each note in the active track difficulty
	///Find the first note at which star power can be deployed
	///Record the end position of each star power phrase for faster detection of the last note in a star power phrase
	for(ctr = 0, index = 0; ctr < tracksize; ctr++)
	{	//For each note in the active track
		if(eof_get_note_type(eof_song, eof_selected_track, ctr) == eof_note_type)
		{	//If the note is in the active difficulty
			unsigned long notepos, notelength;
			double start, end;

			if(index >= note_count)
			{	//Bounds check
				free(note_measure_positions);
				free(note_beat_lengths);
				free(sp_phrase_endings);
				free(tdeployments);
				free(bdeployments);
				free(deploy_cache);
				free(best);
				free(testing);
				eof_log("\tNotes miscounted", 1);

				return 1;	//Logic error
			}

			//Set position and length information
			notepos = eof_get_note_pos(eof_song, eof_selected_track, ctr);
			note_measure_positions[index] = eof_get_measure_position(notepos);	//Store the measure position of the note
			notelength = eof_get_note_length(eof_song, eof_selected_track, ctr);
			start = eof_get_beat(eof_song, notepos) + (eof_get_porpos(notepos) / 100.0);	//The floating point beat position of the start of the note
			end = eof_get_beat(eof_song, notepos + notelength) + (eof_get_porpos(notepos + notelength) / 100.0);	//The floating point beat position of the end of the note
			note_beat_lengths[index] = end - start;		//Store the floating point beat length of the note

			//Populate the sp_phrase_endings[] array for faster future lookups
			if(eof_get_note_tflags(eof_song, eof_selected_track, ctr) & EOF_NOTE_TFLAG_SP_END)
			{	//If this is the last note in a star power phrase
				sp_phrase_endings[index] = 1;
			}

			index++;
		}//If the note is in the active difficulty
	}//For each note in the active track

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

	///Determine the maximum score when no star power is deployed
	best->num_deployments = 0;
	if(!eof_evaluate_ch_sp_path_solution(best, 0, (eof_log_level > 1 ? 1 : 0)))
	{	//Populate the "best" solution with the worst scoring solution, so any better solution will replace it
		eof_log("\tError scoring no star power usage", 1);
		allegro_message("Error scoring no star power usage");
		error = 1;	//The testing of all other solutions will be skipped
	}
	else
	{
		worst_score = best->score;
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tEstimated maximum score without deploying star power is %lu.", worst_score);
		eof_log(eof_log_string, 1);
	}

	first_deploy = eof_ch_path_find_next_deployable_sp(testing, 0);	//Starting from the first note in the target track difficulty, find the first note at which star power can be deployed
	if(first_deploy < note_count)
	{	//If the note was identified
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tFirst possible star power deployment is at note index %lu in this track difficulty.", first_deploy);
		eof_log(eof_log_string, 2);
	}
	else
	{
		eof_log("\tError detecting first available star power deployment", 1);
		error = 1;
	}

	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tEstimated maximum number of star power deployments is %lu", max_deployments);
	eof_log(eof_log_string, 2);

	///Test all possible solutions to find the highest scoring one
	starttime = clock();	//Track the start time

///Iterate and test solutions with a limit of that number of deployments, ie. for three deployments:
///Solutions where the first deployment is at first note:
/// 1 0 0 0 0	deployments={0,0,0}	*C1: num_deployments == 0, init first deployment at first_deploy
/// 1 1 0 0 0	deployments={0,1,0}	*C2: num_deployments < max_deployments, init next deployment (!use function to determine suitable index)
/// 1 1 1 0 0	deployments={0,1,2}	*C2: num_deployments < max_deployments, init next deployment (!use function to determine suitable index)
/// 1 1 0 1 0	deployments={0,1,3}	*C3: num_deployments == max_deployments, increment last deployment
/// 1 1 0 0 1	deployments={0,1,4}	*C3: num_deployments == max_deployments, increment last deployment
/// 1 0 1 0 0	deployments={0,2,0} *C4: num_deployments == max_deployments, can't increment last deployment because it was already at last usable index, remove it and increment previous deployment
/// 1 0 1 1 0	deployments={0,2,3}	*C2: num_deployments < max_deployments, init next deployment (!use function to determine suitable index)
/// 1 0 1 0 1	deployments={0,2,4}	*C3: num_deployments == max_deployments, increment last deployment
/// 1 0 0 1 0	deployments={0,3,0}	*C4: num_deployments == max_deployments, can't increment last deployment because it was already at last usable index, remove it and increment previous deployment
/// 1 0 0 1 1	deployments={0,3,4}	*C2: num_deployments < max_deployments, init next deployment (!use function to determine suitable index)
/// 1 0 0 0 1	deployments={0,4,0}	*C4: num_deployments == max_deployments, can't increment last deployment because it was already at last usable index, remove it and increment previous deployment
///Solutions where the first deployment is at second note:
/// 0 1 0 0 0	deployments={1,0,0}	*C4: num_deployments < max_deployments, can't init next deployment because the last usable index is in use, remove it and increment previous deployment
/// 0 1 1 0 0	deployments={1,2,0}	*C2
/// 0 1 1 1 0	deployments={1,2,3}	*C2
/// 0 1 1 0 1	deployments={1,2,4}	*C3
/// 0 1 0 1 0	deployments={1,3,0}	*C4
/// 0 1 0 1 1	deployments={1,3,4}	*C2
/// 0 1 0 0 1	deployments={1,4,0}	*C4
///Solutions where the first deployment is at third note:
/// 0 0 1 0 0	deployments={2,0,0}	*C4
/// 0 0 1 1 0	deployments={2,3,0}	*C2
/// 0 0 1 1 1	deployments={2,3,4}	*C2
/// 0 0 1 0 1	deployments={2,4,0}	*C4
///Solutions where the first deployment is at fourth note:
/// 0 0 0 1 0	deployments={3,0,0}	*C4
/// 0 0 0 1 1	deployments={3,4,0}	*C2
///Solutions where the first deployment is at fifth note:
/// 0 0 0 0 1	deployments={4,0,0}	*C4
///	END								*C5: first deployment is at last usable index, all solutions have been tested
	///Increment the solution array to create the next solution to test
	testing->num_deployments = 0;	//The first solution increment will test one deployment
	while(!error)
	{	//Continue testing until all solutions are tested, unless scoring the no deployment solution failed
		unsigned long next_deploy;

		if(key[KEY_ESC])
		{	//Allow user to cancel
			error = 2;
			break;
		}
		if((validcount + invalidcount) % 1000 == 0)
		{	//Update the title bar every 1000 solutions
			(void) snprintf(windowtitle, sizeof(windowtitle) - 1, "Testing SP path solution %lu - Press Esc to cancel", validcount + invalidcount);
			set_window_title(windowtitle);
		}

		//Increment solution
		if(testing->num_deployments < max_deployments)
		{	//If the current solution array has fewer than the maximum number of usable star power deployments
			if(!testing->num_deployments)
			{	///Case 1:  This is the first solution
				testing->deployments[0] = first_deploy;	//Initialize the first solution to test one deployment at the first deployable point
				testing->num_deployments++;	//One more deployment is in the solution
			}
			else
			{	//Add another deployment to the solution
				unsigned long previous_deploy = testing->deployments[testing->num_deployments - 1];	//This is the note at which the previous deployment occurs
				next_deploy = eof_ch_path_find_next_deployable_sp(testing, previous_deploy);	//Detect the next note after which another 50% of star power meter has accumulated

				if(next_deploy < note_count)
				{	//If a valid placement for the next deployment was found
					///Case 2:  Add one deployment to the solution
					testing->deployments[testing->num_deployments] = next_deploy;
					testing->num_deployments++;	//One more deployment is in the solution
				}
				else
				{	//If there are no further valid notes to test for this deployment
					///Case 4:  The last deployment has exhausted all notes, remove and advance previous solution
					testing->num_deployments--;	//Remove this deployment from the solution
					if(!testing->num_deployments)
					{	//If there are no more solutions to test
						///Case 5:  All solutions exhausted
						break;
					}
					next_deploy = testing->deployments[testing->num_deployments - 1] + 1;	//Advance the now-last deployment one note further
					if(next_deploy >= note_count)
					{	//If the previous deployment cannot advance even though the deployment that was just removed had come after it
						eof_log("\tLogic error:  Can't advance previous deployment after removing last deployment (1)", 1);
						error = 1;
						break;
					}
					testing->deployments[testing->num_deployments - 1]= next_deploy;
				}
			}
		}
		else if(testing->num_deployments == max_deployments)
		{	//If the maximum number of deployments are in the solution, move the last deployment to create a new solution to test
			next_deploy = testing->deployments[testing->num_deployments - 1] + 1;	//Advance the last deployment one note further
			if(next_deploy >= note_count)
			{	//If the last deployment cannot advance
				///Case 4:  The last deployment has exhausted all notes, remove and advance previous solution
				testing->num_deployments--;	//Remove this deployment from the solution
				if(!testing->num_deployments)
				{	//If there are no more solutions to test
					///Case 5:  All solutions exhausted
					break;
				}
				next_deploy = testing->deployments[testing->num_deployments - 1] + 1;	//Advance the now-last deployment one note further
				if(next_deploy >= note_count)
				{	//If the previous deployment cannot advance even though the deployment that was just removed had come after it
					eof_log("\tLogic error:  Can't advance previous deployment after removing last deployment (2)", 1);
					error = 1;
					break;
				}
				testing->deployments[testing->num_deployments - 1]= next_deploy;
			}
			else
			{	///Case 3:  Advance last deployment by one note
				testing->deployments[testing->num_deployments - 1] = next_deploy;
			}
		}
		else
		{	//If num_deployments > max_deployments
			eof_log("\tLogic error:  More than the maximum number of deployments entered the testing solution", 1);
			error = 1;
			break;
		}

		//Test and compare with the current best solution
		if(eof_evaluate_ch_sp_path_solution(testing, validcount + invalidcount + 1, 0))
		{	//If the solution was considered valid
			if((testing->score > best->score) || ((testing->score == best->score) && (testing->deployment_notes < best->deployment_notes)))
			{	//If this newly tested solution achieved a higher score than the current best, or if it matched the highest score but did so with fewer notes played during star power deployment
				best->score = testing->score;	//It is the new best solution, copy its data into the best solution structure
				best->deployment_notes = testing->deployment_notes;
				best->num_deployments = testing->num_deployments;
				memcpy(best->deployments, testing->deployments, sizeof(unsigned long) * note_count);
			}
			validcount++;	//Track the number of valid solutions tested
		}
		else
		{
			eof_log("\t\t*Solution invalid", 1);
			invalidcount++;	//Track the number of invalid solutions tested
		}
	}//Continue testing until all solutions are tested, unless scoring the no deployment solution failed

	endtime = clock();	//Track the end time
	elapsed_time = (double)(endtime - starttime) / (double)CLOCKS_PER_SEC;	//Convert to seconds

	///Report best solution
	if(error == 1)
	{
		allegro_message("Failed to detect optimum star power path.");
	}
	else if(error == 2)
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t%lu solutions tested (%lu valid, %lu invalid) in %.2f seconds (%.2f solutions per second)", validcount + invalidcount, validcount, invalidcount, elapsed_time, ((double)validcount + invalidcount)/elapsed_time);
		eof_log(eof_log_string, 1);
		allegro_message("User cancellation.%s", eof_log_string);
	}
	else
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t%lu solutions tested (%lu valid, %lu invalid) in %.2f seconds (%.2f solutions per second)", validcount + invalidcount, validcount, invalidcount, elapsed_time, ((double)validcount + invalidcount)/elapsed_time);
		eof_log(eof_log_string, 1);
		if((best->deployment_notes == 0) && (best->score > worst_score))
		{	//If the best score did not reflect notes being played in star power, but the score somehow was better than the score from when no star power deployment was tested
			eof_log("\tLogic error calculating solution.", 1);
			allegro_message("Logic error calculating solution.");
		}
		else if(best->deployment_notes == 0)
		{	//If there was no best use of star power
			eof_log("\tNo notes were playable during a star power deployment", 1);
			allegro_message("No notes were playable during a star power deployment");
		}
		else
		{	//There is at least one note played during star power deployment
			unsigned long notenum, min, sec, ms, flags;
			char timestamps[500], tempstring[20], scorestring[50];
			char *resultstring1 = "Optimum star power deployment in Clone Hero for this track difficulty is at these note timestamps (highlighted):";

			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t%s", resultstring1);
			eof_log(eof_log_string, 1);
			timestamps[0] = '\0';	//Empty the timestamps string

			for(ctr = 0; ctr < best->num_deployments; ctr++)
			{	//For each deployment in the best solution
				notenum = eof_translate_track_diff_note_index(eof_song, best->track, best->diff, best->deployments[ctr]);	//Find the real note number for this index
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
				snprintf(tempstring, sizeof(tempstring) - 1, "%s%lu:%lu.%lu", (ctr ? ", " : ""), min, sec, ms);	//Generate timestamp, prefixing with a comma and spacing after the first
				strncat(timestamps, tempstring, sizeof(timestamps) - 1);	//Append to timestamps string

				flags = eof_get_note_flags(eof_song, eof_selected_track, notenum);
				if(!(flags & EOF_NOTE_FLAG_HIGHLIGHT) && !undo_made)
				{	//If this is the first note being highlighted for the solution
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					undo_made = 1;
				}
				flags |= EOF_NOTE_FLAG_HIGHLIGHT;	//Enable highlighting for this note
				eof_set_note_flags(eof_song, eof_selected_track, notenum, flags);
			}
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t%s", timestamps);
			eof_log(eof_log_string, 1);
			(void) snprintf(scorestring, sizeof(scorestring) - 1, "Estimated score:  %lu", best->score);
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t%s", scorestring);
			eof_log(eof_log_string, 1);
			(void) eof_detect_difficulties(eof_song, eof_selected_track);	//Update highlighting variables
			eof_render();
			allegro_message("%s\n%s\n%s", resultstring1, timestamps, scorestring);
		}
	}

	///Cleanup
	free(note_measure_positions);
	free(note_beat_lengths);
	free(sp_phrase_endings);
	free(tdeployments);
	free(bdeployments);
	free(deploy_cache);
	free(best);
	free(testing);

	for(ctr = 0; ctr < tracksize; ctr++)
	{	//For each note in the active track
		unsigned long tflags = eof_get_note_tflags(eof_song, eof_selected_track, ctr);

		tflags &= ~EOF_NOTE_TFLAG_SOLO_NOTE;	//Clear the temporary flags
		tflags &= ~EOF_NOTE_TFLAG_SP_END;
		eof_set_note_tflags(eof_song, eof_selected_track, ctr, tflags);
	}

	eof_fix_window_title();

	return 1;
}
