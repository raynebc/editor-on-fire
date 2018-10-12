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
	unsigned long tracksize, ctr;
	unsigned long index;	//This will be the note number relative to only the notes in the target track difficulty, used to index into the solution's arrays
	double sp_meter = 0.0;	//The current charge of the star power meter, which must be at least 0.5 before star power can be deployed
	double sp_meter_t = 0.0;	//The uncapped star power level, used to log the total amount of star power that can be gained between deployments
	double sp_deployment_start = 0.0;	//Used to track the start position of star power deployment
	double sp_deployment_end = 0.0;	//The measure position at which the current star power deployment will be over (non-inclusive, ie. a note at this position is NOT in the scope of the deployment)
	unsigned long multiplier = 1;	//The current score multiplier, which increments every 10 hit notes, up to a maximum value of 4
	unsigned long hitcounter = 0;	//Tracks how many notes in a row were hit, for updating the multiplier
	unsigned long notescore;	//The base score for the current note
	unsigned long notepos, noteflags, tflags, is_solo;
	long notelength;
	int disjointed;			//Set to nonzero if the note being processed has disjointed status, as it requires different handling for star power whammy bonus
	int representative = 0;	//Set to nonzero if the note being processed is the longest and last gem (criteria in that order) in a disjointed chord, and will be examined for star power whammy bonus
	unsigned long base_score, sp_base_score, sustain_score, sp_sustain_score;	//For logging score calculations

	if(!solution || !solution->deploy_status || !solution->note_measure_positions || !solution->note_beat_lengths || !eof_song || !solution->track || (solution->track >= eof_song->tracks))
		return 0;	//Invalid parameters

	solution->score = 0;
	solution->deployment_notes = 0;
	tracksize = eof_get_track_size(eof_song, solution->track);

	if(eof_log_level > 1)
	{	//Skip logging overhead if the logging level is too low
		unsigned long notenum, min, sec, ms, deploycount;
		char tempstring[50];

		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tTesting SP path solution #%lu:  Deploy at note indexes ", solution_num);

		for(ctr = 0, deploycount = 0; ctr < solution->note_count; ctr++)
		{	//For each note in the active track difficulty
			if(solution->deploy_status[ctr])
			{	//If star power was to be deployed at this note
				notenum = eof_translate_track_diff_note_index(eof_song, solution->track, solution->diff, ctr);	//Find the real note number for this index
				if(notenum < tracksize)
				{	//If that number was found
					ms = eof_get_note_pos(eof_song, solution->track, notenum);	//Determine mm:ss.ms formatting of timestamp
					min = ms / 60000;
					ms -= 60000 * min;
					sec = ms / 1000;
					ms -= 1000 * sec;
					snprintf(tempstring, sizeof(tempstring) - 1, "%s%lu (%lu:%lu.%lu)", (deploycount ? ", " : ""), ctr, min, sec, ms);
					strncat(eof_log_string, tempstring, sizeof(eof_log_string) - 1);
					deploycount++;
				}
			}
		}
		if(!deploycount)
		{	//If there were no star power deployments in this solution
			strncat(eof_log_string, "(none)", sizeof(eof_log_string) - 1);
		}
		eof_log(eof_log_string, 2);
	}
	for(ctr = 0, index = 0; ctr < tracksize; ctr++)
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
			}
			else
			{	//If star power deployment is still in effect
				if(solution->deploy_status[index])
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
			if(solution->deploy_status[index])
			{	//If the solution specifies deploying star power at this note
				if(sp_meter >= 0.50 - 0.0001)
				{	//If the star power meter is at least half full (allow variance for math error)
					sp_deployment_start = solution->note_measure_positions[index];
					sp_deployment_end = sp_deployment_start + (8.0 * sp_meter);	//Determine the end timing of the star power (one full meter is 8 measures)
					sp_meter = sp_meter_t = 0.0;	//The entire star power meter will be used
					sp_deployed = 1;	//Star power is now in effect
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
				sp_meter += 0.25;	//Award 25% of a star power meter
				sp_meter_t += 0.25;
				if(sp_meter > 1.0)
					sp_meter = 1.0;	//Cap the star power meter at 100%
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
			sp_meter = (sp_deployment_end - sp_deployment_start) / 8.0;	//Convert remaining star power deployment duration back to star power
			sp_meter_t = sp_meter;

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
				sp_meter = sp_meter + sp_whammy_gain - sp_drain;	//Update the star power meter to reflect the amount gained and the amount consumed during this 1/25 beat interval
				sp_meter_t = sp_meter;
				if(sp_meter > 1.0)
					sp_meter = 1.0;	//Cap the star power meter at 100%

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
					if(sp_meter > 0.0)
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
					if(sp_meter > 0.0)
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
			if(sp_meter > 0.0)
			{	//If the star power meter still has star power, calculate the new end of deployment position
				sp_deployment_start = eof_get_measure_position(notepos + notelength);	//The remainder of the deployment starts at this note's end position
				sp_deployment_end = sp_deployment_start + (8.0 * sp_meter);	//Determine the end timing of the star power (one full meter is 8 measures)
			}
			else
			{	//Otherwise the star power deployment has ended
				sp_meter = sp_meter_t = 0.0;
				sp_deployed = 0;
				if(logging)
				{
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tStar power ended during note #%lu (index #%lu)", ctr, index);
					eof_log(eof_log_string, 1);
				}
			}
			notescore *= multiplier;	//Apply the current score multiplier in effect
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

						sp_meter += sp_bonus;	//Add the star power bonus
						sp_meter_t += sp_bonus;
						if(sp_meter > 1.0)
							sp_meter = 1.0;	//Cap the star power meter at 100%
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
			notescore *= multiplier;			//Apply the current score multiplier in effect
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
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tNote #%lu (index #%lu):  \tpos = %lums, \tbase = %lu, \tsustain = %lu, mult = x%lu, solo bonus = %lu, \tscore = %lu.  \tTotal score:  %lu\tSP Meter at %lu%% (uncapped %lu%%)", ctr, index, notepos, base_score, sustain_score, multiplier, is_solo * 100, notescore, solution->score, (unsigned long)(sp_meter * 100.0 + 0.5), (unsigned long)(sp_meter_t * 100.0 + 0.5));
			}
			else
			{	//Star power bonus
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tNote #%lu (index #%lu):  \tpos = %lums, \tbase = %lu, \tsustain = %lu, SP = %lu, SP sustain = %lu, mult = x%lu, solo bonus = %lu, \tscore = %lu.  \tTotal score:  %lu\tSP Meter at %lu%% (uncapped %lu%%)", ctr, index, notepos, base_score, sustain_score, sp_base_score, sp_sustain_score, multiplier, is_solo * 100, notescore, solution->score, (unsigned long)(sp_meter * 100.0 + 0.5), (unsigned long)(sp_meter_t * 100.0 + 0.5));
			}
			eof_log(eof_log_string, 1);
		}

		///Update multiplier
		hitcounter++;
		if(hitcounter == 30)
		{
			if(logging)
			{
				eof_log("\t\t\tMultiplier increased to x4", 1);
			}
			multiplier = 4;
		}
		else if(hitcounter == 20)
		{
			if(logging)
			{
				eof_log("\t\t\tMultiplier increased to x3", 1);
			}
			multiplier = 3;
		}
		else if(hitcounter == 10)
		{
			if(logging)
			{
				eof_log("\t\t\tMultiplier increased to x2", 1);
			}
			multiplier = 2;
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
	unsigned char *deploybest, *deploytesting;	//The status of star power deployment at each note in the active track difficulty
	unsigned char *sp_phrase_endings;			//The boolean status of whether each note in the active track difficulty is the last note in a star power phrase
	unsigned long worst_score = 0;				//The estimated maximum score when no star power is deployed
	unsigned long first_deploy = ULONG_MAX;		//The first note that occurs after the end of the second star power phrase, and is thus the first note at which star power can be deployed
	unsigned long *deployments;					//An array defining the note index number of each deployment, ie deployments[0] being the first SP deployment, deployments[1] being the second, etc.
	unsigned long ctr, ctr2, index, tracksize, numsolos;
	unsigned long validcount = 0, invalidcount = 0;
///	unsigned long pre_reject_count = 0;
	EOF_PHRASE_SECTION *sectionptr;
	int error = 0;
	char undo_made = 0;
	clock_t starttime = 0, endtime = 0;
	unsigned long sp_phrase_count;
	double sp_sustain;				//The number of beats of star power sustain, used to count whammy bonus star power when estimating the maximum number of star power deployments
	unsigned long max_deployments;	//The estimated maximum number of star power deployments, based on the amount of star power note sustain and the number of star power phrases
///	unsigned long deploy_ctr;

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
	deploybest = malloc(sizeof(unsigned char) * note_count);
	deploytesting = malloc(sizeof(unsigned char) * note_count);
	sp_phrase_endings = malloc(sizeof(unsigned char) * note_count);
	deployments = malloc(sizeof(unsigned long) * note_count);
	best = malloc(sizeof(EOF_SP_PATH_SOLUTION));
	testing = malloc(sizeof(EOF_SP_PATH_SOLUTION));

	if(!note_measure_positions || !note_beat_lengths || !deploybest || !deploytesting || !best || !testing || !sp_phrase_endings || !deployments)
	{	//If any of those failed to allocate
		if(note_measure_positions)
			free(note_measure_positions);
		if(note_beat_lengths)
			free(note_beat_lengths);
		if(deploybest)
			free(deploybest);
		if(deploytesting)
			free(deploytesting);
		if(sp_phrase_endings)
			free(sp_phrase_endings);
		if(deployments)
			free(deployments);
		if(best)
			free(best);
		if(testing)
			free(testing);

		eof_log("\tFailed to allocate memory", 1);
		return 1;
	}

	memset(deploybest, 0, sizeof(unsigned char) * note_count);
	best->deploy_status = deploybest;
	best->deployments = deployments;
	best->note_measure_positions = note_measure_positions;
	best->note_beat_lengths = note_beat_lengths;
	best->note_count = note_count;
	best->track = eof_selected_track;
	best->diff = eof_note_type;

	testing->deploy_status = deploytesting;
	testing->deployments = deployments;
	testing->note_measure_positions = note_measure_positions;
	testing->note_beat_lengths = note_beat_lengths;
	testing->note_count = note_count;
	testing->track = eof_selected_track;
	testing->diff = eof_note_type;

	memset(sp_phrase_endings, 0, sizeof(unsigned char) * note_count);

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
				free(deploybest);
				free(deploytesting);
				free(sp_phrase_endings);
				free(deployments);
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
///		int pre_rejected = 0;
		unsigned long next_deploy;

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

		//Build the solution[] array from the deployments[] array
		memset(testing->deploy_status, 0, sizeof(unsigned char) * note_count);
		for(ctr = 0; ctr < testing->num_deployments; ctr++)
		{	//For each deployment in this solution
			testing->deploy_status[testing->deployments[ctr]] = 1;	//Update the deploy_status array with each deployment
		}

///These screens are no longer needed, since the solution incrementing doesn't create invalid solutions
/*		///Pre-screen this solution to see if it can be quickly rejected before thoroughly scoring it
		///Screen 1:  Too many star power deployments (this will filter most of the solutions to be tested)
		deploy_ctr = 0;
		for(index = first_deploy; index < note_count; index++)
		{	//Starting with the first note at which star power deployment is possible
			if(testing->deploy_status[index])
			{	//If star power is to be deployed at this note
				deploy_ctr++;	//Count how many deployments were found in this screening
				if(deploy_ctr > max_deployments)
				{	//If the number of maximum possible star power deployments was exceeded
					pre_rejected = 1;
					pre_reject_count++;
					break;
				}
			}
		}

		///Screen 2:  Star power deployment with insufficient star power
		if(!pre_rejected)
		{	//If the solution hasn't already been rejected
			sp_phrase_count = 0;
			sp_sustain = 0.0;
			for(ctr = 0, index = 0; ctr < note_count; ctr++)
			{	//For each note in the active track difficulty
				unsigned long noteflags;

				if(eof_get_note_type(eof_song, eof_selected_track, ctr) != eof_note_type)
					continue;	//If the note is not in the active track difficulty, skip it

				noteflags = eof_get_note_flags(eof_song, eof_selected_track, ctr);
				if(noteflags & EOF_NOTE_FLAG_SP)
				{	//If the note has star power
					if(eof_get_note_length(eof_song, eof_selected_track, ctr) > 1)
					{	//If the note has sustain
						sp_sustain += note_beat_lengths[index];	//Count the number of beats of star power that will be whammied for bonus star power
						while(sp_sustain >= 8.0 - 0.0001)
						{	//For every 8 beats' worth (allowing variance for math error) of star power sustain
							sp_sustain -= 8.0;
							sp_phrase_count++;	//Convert it to one star power phrase's worth of star power
						}
					}
					if(sp_phrase_endings[index])
					{	//If this note is the end of a star power phrase
						sp_phrase_count++;	//Count the number of ended star power phrases since the star power deployment
					}
				}

				if(testing->deploy_status[index])
				{	//If star power is to be deployed at this note
					if(sp_phrase_count < 2)
					{	//Not enough star power has accumulated
						pre_rejected = 1;		//Reject this solution
						pre_reject_count++;
						break;
					}
					else
					{	//All saved star power will be consumed
						sp_phrase_count = 0;	//All star power will drain before it can start again
						sp_sustain = 0.0;		//Any star power gained from sustained notes is also consumed
					}
				}

				index++;	//Count the number of notes in the active track difficulty that have been parsed
			}
		}

		if(pre_rejected)
		{	//If this solution was rejected
			if(pre_reject_count == ULONG_MAX)
			{	//If this counter has hit its limit, log and reset the counter
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t%lu invalid solutions skipped", pre_reject_count);
				eof_log(eof_log_string, 1);
				pre_reject_count = 0;
			}
			continue;	//Skip scoring this solution and test the next solution instead
		}

		//Test and compare with the current best solution
		if(pre_reject_count)
		{	//If the count of rejected solutions hasn't been logged since the last rejected one
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t%lu invalid solutions skipped", pre_reject_count);
			eof_log(eof_log_string, 1);
			pre_reject_count = 0;
		}
*/

		//Test and compare with the current best solution
		if(eof_evaluate_ch_sp_path_solution(testing, validcount + invalidcount + 1, 0))
		{	//If the solution was considered valid
			if((testing->score > best->score) || ((testing->score == best->score) && (testing->deployment_notes < best->deployment_notes)))
			{	//If this newly tested solution achieved a higher score than the current best, or if it matched the highest score but did so with fewer notes played during star power deployment
				best->score = testing->score;	//It is the new best solution, copy its data into the best solution structure
				best->deployment_notes = testing->deployment_notes;
				memcpy(best->deploy_status, testing->deploy_status, sizeof(unsigned char) * note_count);
			}
			validcount++;	//Track the number of valid solutions tested
		}
		else
		{
			invalidcount++;	//Track the number of invalid solutions tested
		}
	}//Continue testing until all solutions are tested, unless scoring the no deployment solution failed

	endtime = clock();	//Track the start time

	///Report best solution
	if(error)
	{
		allegro_message("Failed to detect optimum star power path.");
	}
	else
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t%lu solutions tested (%lu valid, %lu invalid) in %f seconds", validcount + invalidcount, validcount, invalidcount, ((double)(endtime - starttime) / (double)CLOCKS_PER_SEC));
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
			unsigned long notenum, min, sec, ms, deploycount, flags;
			char timestamps[500], tempstring[20], scorestring[50];
			char *resultstring1 = "Optimum star power deployment in Clone Hero for this track difficulty is at these note timestamps (highlighted):";

			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t%s", resultstring1);
			eof_log(eof_log_string, 1);
			timestamps[0] = '\0';	//Empty the timestamps string

			for(ctr = 0, deploycount = 0; ctr < note_count; ctr++)
			{	//For each note in the active track difficulty
				if(best->deploy_status[ctr])
				{	//If star power was to be deployed at this note
					notenum = eof_translate_track_diff_note_index(eof_song, eof_selected_track, eof_note_type, ctr);	//Find the real note number for this index

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
					snprintf(tempstring, sizeof(tempstring) - 1, "%s%lu:%lu.%lu", (deploycount ? ", " : ""), min, sec, ms);	//Generate timestamp, prefixing with a comma and spacing after the first
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
	free(deploybest);
	free(deploytesting);
	free(sp_phrase_endings);
	free(deployments);
	free(best);
	free(testing);

	for(ctr = 0; ctr < tracksize; ctr++)
	{	//For each note in the active track
		unsigned long tflags = eof_get_note_tflags(eof_song, eof_selected_track, ctr);

		tflags &= ~EOF_NOTE_TFLAG_SOLO_NOTE;	//Clear the temporary flags
		tflags &= ~EOF_NOTE_TFLAG_SP_END;
		eof_set_note_tflags(eof_song, eof_selected_track, ctr, tflags);
	}

	return 1;
}
