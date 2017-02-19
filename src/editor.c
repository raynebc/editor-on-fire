#include <math.h>
#include "menu/file.h"
#include "menu/edit.h"
#include "menu/song.h"
#include "menu/track.h"
#include "menu/note.h"
#include "menu/beat.h"
#include "menu/help.h"
#include "menu/context.h"
#include "modules/ocd3d.h"
#include "foflc/Lyric_storage.h"
#include "dialog.h"
#include "beat.h"
#include "editor.h"
#include "main.h"
#include "midi.h"
#include "mix.h"
#include "note.h"		//For EOF_LYRIC_PERCUSSION definition
#include "player.h"
#include "rs.h"
#include "song.h"
#include "spectrogram.h"
#include "tuning.h"
#include "undo.h"
#include "utility.h"	//For eof_check_string()
#include "waveform.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

int         eof_held_1 = 0;
int         eof_held_2 = 0;
int         eof_held_3 = 0;
int         eof_held_4 = 0;
int         eof_held_5 = 0;
int         eof_entering_note = 0;
EOF_NOTE *  eof_entering_note_note = NULL;
EOF_LYRIC *  eof_entering_note_lyric = NULL;
int         eof_snote = 0;
int         eof_last_snote = 0;
int         eof_moving_anchor = 0;
int         eof_adjusted_anchor = 0;
unsigned long eof_anchor_diff[EOF_TRACKS_MAX] = {0};
unsigned long eof_tech_anchor_diff[EOF_TRACKS_MAX] = {0};

EOF_SNAP_DATA eof_snap;
EOF_SNAP_DATA eof_tail_snap;

double eof_pos_distance(double p1, double p2)
{
//	eof_log("eof_pos_distance() entered");

	if(p1 > p2)
	{
		return p1 - p2;
	}

	return p2 - p1;
}

void eof_select_beat(unsigned long beat)
{
	eof_log("eof_select_beat() entered", 1);

	if(!eof_song || (beat >= eof_song->beats))
		return;

	eof_selected_beat = beat;
	if(!eof_beat_stats_cached)
	{	//If the cached beat statistics are not current
		eof_process_beat_statistics(eof_song, eof_selected_track);	//Rebuild them (from the perspective of the specified track)
	}
	eof_beat_in_measure = eof_song->beat[beat]->beat_within_measure;
	eof_beats_in_measure = eof_song->beat[beat]->num_beats_in_measure;
	if((eof_beat_in_measure >= 0) && (eof_beats_in_measure >= 0))
	{	//If valid measure information is available
		eof_selected_measure = eof_song->beat[beat]->measurenum;
	}
	else
	{	//Otherwise it's likely that no valid time signature is in effect
		eof_selected_measure = -1;
	}
}

void eof_get_snap_ts(EOF_SNAP_DATA * sp, unsigned long beat)
{
//	eof_log("eof_get_snap_ts() entered");

	unsigned long tsbeat = 0, i;

	if(!sp || !eof_song || (beat >= eof_song->beats))
	{
		return;
	}
	for(i = beat + 1; i > 0; i--)
	{	//For each beat at and before the specified beat, in reverse order
		if(eof_song->beat[i - 1]->flags & (EOF_BEAT_FLAG_START_2_4 | EOF_BEAT_FLAG_START_3_4 | EOF_BEAT_FLAG_START_4_4 | EOF_BEAT_FLAG_START_5_4 | EOF_BEAT_FLAG_START_6_4 | EOF_BEAT_FLAG_CUSTOM_TS))
		{
			tsbeat = i - 1;
			break;
		}
	}

	/* set default denominator in case no TS flags found */
	sp->numerator = 4;
	sp->denominator = 4;

	/* all TS presets have a denominator of 4 */
	if(eof_song->beat[tsbeat]->flags & EOF_BEAT_FLAG_START_2_4)
	{
		sp->numerator = 2;
		sp->denominator = 4;
	}
	else if(eof_song->beat[tsbeat]->flags & EOF_BEAT_FLAG_START_3_4)
	{
		sp->numerator = 3;
		sp->denominator = 4;
	}
	else if(eof_song->beat[tsbeat]->flags & EOF_BEAT_FLAG_START_4_4)
	{
		sp->numerator = 4;
		sp->denominator = 4;
	}
	else if(eof_song->beat[tsbeat]->flags & EOF_BEAT_FLAG_START_5_4)
	{
		sp->numerator = 5;
		sp->denominator = 4;
	}
	else if(eof_song->beat[tsbeat]->flags & EOF_BEAT_FLAG_START_6_4)
	{
		sp->numerator = 6;
		sp->denominator = 4;
	}

	/* decode custom TS */
	else if(eof_song->beat[tsbeat]->flags & EOF_BEAT_FLAG_CUSTOM_TS)
	{
		sp->numerator = ((eof_song->beat[tsbeat]->flags & 0xFF000000)>>24) + 1;
		sp->denominator = ((eof_song->beat[tsbeat]->flags & 0x00FF0000)>>16) + 1;
	}
}

void eof_snap_logic(EOF_SNAP_DATA * sp, unsigned long p)
{
//	eof_log("eof_snap_logic() entered");

	unsigned long least = ULONG_MAX;	//Initialize to an invalid value
	unsigned long i;
	unsigned interval = 1;
	char measure_snap = 0;	//Unless a custom per-measure grid snap is defined, all grid snaps are per beat
	double snaplength, least_amount;

	if(!sp || !eof_song || (eof_song->beats < 2))
	{
		return;
	}
	/* place pen at "real" location and adjust from there */
	sp->pos = sp->previous_snap = sp->next_snap = p;

	/* ensure pen is within the song boundaries */
	if(sp->pos < eof_song->tags->ogg[eof_selected_ogg].midi_offset)
	{
		sp->pos = eof_song->tags->ogg[eof_selected_ogg].midi_offset;
	}
	else if(sp->pos >= eof_chart_length)
	{
		sp->pos = eof_chart_length - 1;
	}

	if(eof_snap_mode != EOF_SNAP_OFF)
	{	//If grid snap is enabled
		/* find the snap beat */
		sp->beat = -1;
		if(sp->pos < eof_song->beat[eof_song->beats - 1]->pos)
		{	//If the target position is earlier than the last beat marker
			for(i = 0; i < eof_song->beats - 1; i++)
			{
				if((sp->pos >= eof_song->beat[i]->pos) && (sp->pos < eof_song->beat[i + 1]->pos))
				{
					sp->beat = i;
					break;
				}
			}
		}
		if((sp->beat < 0) || !eof_beat_num_valid(eof_song, sp->beat))
		{	//if no suitable beat was found
			return;
		}

		if(eof_beat_num_valid(eof_song, sp->beat + 1))
		{	//As long as the next beat is in bounds
			sp->beat_length = eof_song->beat[sp->beat + 1]->fpos - eof_song->beat[sp->beat]->fpos;	//Use its position to determine this beat's length
		}
		else if(sp->beat > 0)
		{	//Otherwise use the previous beat's position to determine this beat's length
			sp->beat_length = eof_song->beat[sp->beat]->fpos - eof_song->beat[sp->beat - 1]->fpos;
		}
		else
		{	//Logic error, the presence of at least two beats was already confirmed, but there aren't any now
			return;
		}
		eof_get_snap_ts(sp, sp->beat);
		switch(eof_snap_mode)
		{
			case EOF_SNAP_QUARTER:
			{
				interval = 1;
				break;
			}
			case EOF_SNAP_EIGHTH:
			{
				interval = 2;
				break;
			}
			case EOF_SNAP_TWELFTH:
			{
				interval = 3;
				break;
			}
			case EOF_SNAP_SIXTEENTH:
			{
				interval = 4;
				break;
			}
			case EOF_SNAP_TWENTY_FOURTH:
			{
				interval = 6;
				break;
			}
			case EOF_SNAP_THIRTY_SECOND:
			{
				interval = 8;
				break;
			}
			case EOF_SNAP_FORTY_EIGHTH:
			{
				interval = 12;
				break;
			}
			case EOF_SNAP_SIXTY_FORTH:
			{
				interval = 16;
				break;
			}
			case EOF_SNAP_NINTY_SIXTH:
			{
				interval = 24;
				break;
			}
			case EOF_SNAP_CUSTOM:
			{
				if(eof_snap_interval >= EOF_MAX_GRID_SNAP_INTERVALS)		//Bounds check
					eof_snap_interval = EOF_MAX_GRID_SNAP_INTERVALS - 1;
				interval = eof_snap_interval;
				if(eof_custom_snap_measure)
					measure_snap = 1;
				break;
			}
			default:
			break;
		}

		/* do the actual snapping */
		least_amount = sp->beat_length;
		if(measure_snap)
		{	//If performing snap by measure logic
			int ts = 1;
			double bl; // current beat length

			/* find the measure we are currently in */
			sp->measure_beat = 0;
			for(i = sp->beat + 1; i > 0; i--)
			{	//For each beat at and before the specified beat, in reverse order
				if(eof_song->beat[i - 1]->flags & EOF_BEAT_FLAG_START_2_4)
				{
					ts = 2;
					sp->measure_beat = i - 1;
					break;
				}
				else if(eof_song->beat[i - 1]->flags & EOF_BEAT_FLAG_START_3_4)
				{
					ts = 3;
					sp->measure_beat = i - 1;
					break;
				}
				else if(eof_song->beat[i - 1]->flags & EOF_BEAT_FLAG_START_4_4)
				{
					ts = 4;
					sp->measure_beat = i - 1;
					break;
				}
				else if(eof_song->beat[i - 1]->flags & EOF_BEAT_FLAG_START_5_4)
				{
					ts = 5;
					sp->measure_beat = i - 1;
					break;
				}
				else if(eof_song->beat[i - 1]->flags & EOF_BEAT_FLAG_START_6_4)
				{
					ts = 6;
					sp->measure_beat = i - 1;
					break;
				}
				else if(eof_song->beat[i - 1]->flags & EOF_BEAT_FLAG_CUSTOM_TS)
				{
					ts = ((eof_song->beat[i - 1]->flags & 0xFF000000)>>24) + 1;
					sp->measure_beat = i - 1;
					break;
				}
			}
			for(i = sp->beat; i >= sp->measure_beat; i--)
			{
				/* start of measure we are currently in */
				if((i - sp->measure_beat) % ts == 0)
				{
					sp->measure_beat = i;
					break;
				}
			}

			/* add up beat times to find measure length */
			if(sp->measure_beat + ts < eof_song->beats)
			{
				bl = eof_song->beat[sp->measure_beat + 1]->fpos - eof_song->beat[sp->measure_beat]->fpos;
				sp->measure_length = 0.0;
				for(i = sp->measure_beat; i < sp->measure_beat + ts; i++)
				{
					if(i < eof_song->beats - 1)
					{
						bl = eof_song->beat[i + 1]->fpos - eof_song->beat[i]->fpos;
					}
					sp->measure_length += bl;
				}
			}

			/* find the snap positions */
			snaplength = sp->measure_length / (double)interval;
			sp->grid_pos[0] = eof_song->beat[sp->measure_beat]->fpos;	//Set first grid position
			for(i = 1; i < interval; i++)
			{
				sp->grid_pos[i] = eof_song->beat[sp->measure_beat]->fpos + (snaplength * (double)i);
			}
			sp->grid_pos[interval] = eof_song->beat[sp->measure_beat]->fpos + sp->measure_length;	//Set last grid position

			/* see which one we snap to */
			for(i = 0; i < interval + 1; i++)
			{
				sp->grid_distance[i] = eof_pos_distance(sp->grid_pos[i], sp->pos);
			}
			sp->previous_snap = sp->grid_pos[0] + 0.5;		//Initialize these values
			sp->next_snap = sp->grid_pos[1] + 0.5;
			for(i = 0; i < interval + 1; i++)
			{	//For each calculated grid snap position
				if((unsigned long)(sp->grid_pos[i] + 0.5) < p)
				{	//If this grid snap position (rounded to nearest ms) is before the input timestamp
					sp->previous_snap = sp->grid_pos[i] + 0.5;	//Round to nearest ms
					if(i < interval)
					{	//As long as the next interval is defined in the snap data
						sp->next_snap = sp->grid_pos[i + 1] + 0.5;	//Round to nearest ms
					}
				}
				else if((unsigned long)(sp->grid_pos[i] + 0.5) == p)
				{	//If this grid snap position (rounded to nearest ms) is at the input timestamp
					if(i < interval)
					{	//As long as the next interval is defined in the snap data
						sp->next_snap = sp->grid_pos[i + 1] + 0.5;	//Round to nearest ms
					}
				}
				if(sp->grid_distance[i] < least_amount)
				{
					least = i;
					least_amount = sp->grid_distance[i];
				}
			}
			if(least < EOF_MAX_GRID_SNAP_INTERVALS)
			{	//If a valid grid snap position was found
				sp->pos = sp->grid_pos[least] + 0.5;	//Round to nearest ms
			}
		}//If performing snap by measure logic
		else
		{	//If performing snap by beat logic
			//Find the snap positions
			snaplength = sp->beat_length / (double)interval;
			sp->grid_pos[0] = eof_song->beat[sp->beat]->fpos;	//Set the first grid position
			for(i = 1; i < interval; i++)
			{
				sp->grid_pos[i] = eof_song->beat[sp->beat]->fpos + (snaplength * (double)i);	//Set grid snap positions
			}
			sp->grid_pos[interval] = eof_song->beat[sp->beat + 1]->fpos;	//Record the position of the last grid snap, which is the next beat

			//see which one we snap to
			for(i = 0; i < interval + 1; i++)
			{
				sp->grid_distance[i] = eof_pos_distance(sp->grid_pos[i], sp->pos);
			}
			sp->previous_snap = sp->grid_pos[0] + 0.5;		//Initialize these values
			sp->next_snap = sp->grid_pos[1] + 0.5;
			for(i = 0; i < interval + 1; i++)
			{	//For each calculated grid snap position
				if((unsigned long)(sp->grid_pos[i] + 0.5) < p)
				{	//If this grid snap position (rounded to nearest ms) is before the input timestamp
					sp->previous_snap = sp->grid_pos[i] + 0.5;	//Round to nearest ms
					if(i < interval)
					{	//As long as the next interval is defined in the snap data
						sp->next_snap = sp->grid_pos[i + 1] + 0.5;	//Round to nearest ms
					}
				}
				else if((unsigned long)(sp->grid_pos[i] + 0.5) == p)
				{	//If this grid snap position (rounded to nearest ms) is at the input timestamp
					if(i < interval)
					{	//As long as the next interval is defined in the snap data
						sp->next_snap = sp->grid_pos[i + 1] + 0.5;	//Round to nearest ms
					}
				}
				if(sp->grid_distance[i] < least_amount)
				{
					least = i;
					least_amount = sp->grid_distance[i];
				}
			}
			if(least < EOF_MAX_GRID_SNAP_INTERVALS)
			{	//If a valid grid snap position was found
				sp->pos = sp->grid_pos[least] + 0.5;	//Round to nearest ms
			}
		}//If performing snap by beat logic
		sp->intervals = interval + 1;	//Record the number of entries that are stored in the grid_pos[] and grid_distance[] arrays
		if(sp->previous_snap == eof_song->beat[sp->beat]->pos + 1)
		{	//Special case:  Rounding errors caused the previous snap to round up differently than the beat's integer timestamp
			if(sp->pos == sp->previous_snap)
				sp->pos = eof_song->beat[sp->beat]->pos;	//Correct the closest grid snap position if necessary
			sp->previous_snap = eof_song->beat[sp->beat]->pos;	//Round it back down to avoid a condition where a note can't be shortened to before a beat line due to rounding errors
		}
	}//If grid snap is enabled
}

void eof_snap_length_logic(EOF_SNAP_DATA * sp)
{
	char measure_snap = 0;	//Unless a custom per-measure grid snap is defined, all grid snaps are per beat
	int interval = 4;
//	eof_log("eof_snap_length_logic() entered");

	if(!sp)
	{
		return;
	}
	if(eof_snap_mode != EOF_SNAP_OFF)
	{	//If grid snap is not disabled
		/* if snapped to the next beat, make sure length is calculated from that beat */
		if(sp->pos >= eof_song->beat[sp->beat + 1]->pos)
		{
			sp->beat = sp->beat + 1;
		}
		if((sp->beat >= eof_song->beats - 2) && (sp->beat >= 2))
		{	//Don't reference a negative index of eof_song->beat[]
			sp->beat_length = eof_song->beat[sp->beat - 1]->fpos - eof_song->beat[sp->beat - 2]->fpos;
		}
		else if((sp->beat + 1 < eof_song->beats) && (sp->beat >= 0))
		{	//Don't read out of bounds
			sp->beat_length = eof_song->beat[sp->beat + 1]->fpos - eof_song->beat[sp->beat]->fpos;
		}

		/* calculate the length of the snap position */
		switch(eof_snap_mode)
		{
			case EOF_SNAP_QUARTER:
			{
				interval = 1;
				break;
			}
			case EOF_SNAP_EIGHTH:
			{
				interval = 2;
				break;
			}
			case EOF_SNAP_TWELFTH:
			{
				interval = 3;
				break;
			}
			case EOF_SNAP_SIXTEENTH:
			{
				interval = 4;
				break;
			}
			case EOF_SNAP_TWENTY_FOURTH:
			{
				interval = 6;
				break;
			}
			case EOF_SNAP_THIRTY_SECOND:
			{
				interval = 8;
				break;
			}
			case EOF_SNAP_FORTY_EIGHTH:
			{
				interval = 12;
				break;
			}
			case EOF_SNAP_SIXTY_FORTH:
			{
				interval = 16;
				break;
			}
			case EOF_SNAP_NINTY_SIXTH:
			{
				interval = 24;
				break;
			}
			case EOF_SNAP_CUSTOM:
			{
				interval = eof_snap_interval;
				if(eof_custom_snap_measure)
					measure_snap = 1;
				break;
			}
			default:
			{	//Invalid snap mode
				sp->length = 100;
				return;
			}
		}
		if(measure_snap)
		{
			sp->length = (sp->measure_length / (double)interval + 0.5);	//Round up
		}
		else
		{
			sp->length = (sp->beat_length / (double)interval + 0.5);	//Round up
		}
		if(sp->length <= 0)
			sp->length = 1;	//Enforce a minimum grid snap length of 1ms

	}//If grid snap is not disabled
	else
	{	//If grid snap is disabled
		sp->length = 100;	//Default snap length is 100ms
	}
}

unsigned long eof_next_grid_snap(unsigned long pos)
{
	EOF_SNAP_DATA temp = {0, 0.0, 0, 0.0, 0, 0, 0, {0.0}, {0.0}, 0, 0, 0, 0};
	unsigned long beat;

	if(!eof_song || (eof_snap_mode == EOF_SNAP_OFF))
		return 0;

	beat = eof_get_beat(eof_song, pos);	//Find which beat the specified position is in
	if(!eof_beat_num_valid(eof_song, beat))	//If the seek position is outside the chart
		return 0;

	if(pos >= eof_song->beat[eof_song->beats - 1]->pos)	//If the specified position is after the last beat marker
		return 0;

	eof_snap_logic(&temp, pos);				//Find the next grid snap position that occurs after the specified position

	return temp.next_snap;
}

int eof_find_grid_snap(unsigned long pos, int dir, unsigned long *result)
{
	unsigned long beat;

	if(!eof_song || !result || (eof_snap_mode == EOF_SNAP_OFF))
		return 0;

	beat = eof_get_beat(eof_song, pos);	//Find which beat the specified position is in
	if(!eof_beat_num_valid(eof_song, beat))	//If the specified position is outside the chart
		return 0;

	if(dir < 0)
	{	//If looking for the previous grid snap
		if(pos <= eof_song->beat[0]->pos)
			return 0;	//There are no seek positions before the first beat marker
		if(pos == eof_song->beat[beat]->pos)
			beat--;	//If the specified position is on a beat marker, the previous grid snap is in the previous beat
	}
	else
	{	//If looking for the nearest/next grid snap
		if(pos >= eof_song->beat[eof_song->beats - 1]->pos)
			return 0;	//There are no seek positions after the last beat marker
	}

	if(dir < 0)
	{	//If looking for the previous grid snap
		eof_snap_logic(&eof_tail_snap, eof_song->beat[beat]->pos);	//Find beat/measure length
		eof_snap_length_logic(&eof_tail_snap);	//Find length of one grid snap in the target beat
		if(eof_tail_snap.length > pos)
		{	//Special case:  Specified position is less than one grid snap from the beginning of the chart
			*result = eof_song->beat[0]->pos;	//The result is the first beat marker
		}
		else
		{
			eof_snap_logic(&eof_tail_snap, pos - eof_tail_snap.length);	//Find the grid snapped position of the new seek position
			*result = eof_tail_snap.pos;					//The result is the closest grid snap that is one snap length earlier than the specified position
		}
	}
	else if(dir == 0)
	{	//If looking for the nearest grid snap
		eof_snap_logic(&eof_tail_snap, pos);
		*result = eof_tail_snap.pos;						//The result is the nearest grid snap position
	}
	else
	{	//If looking for the next grid snap
		eof_snap_logic(&eof_tail_snap, pos);				//Find the grid snapped position of the new seek position
		*result = eof_tail_snap.next_snap;					//The result is the next grid snap position
	}

	return 1;	//Success
}

int eof_is_grid_snap_position(unsigned long pos)
{
	EOF_SNAP_DATA temp = {0, 0.0, 0, 0.0, 0, 0, 0, {0.0}, {0.0}, 0, 0, 0, 0};
	unsigned long beat;

	if(!eof_song || (eof_snap_mode == EOF_SNAP_OFF))
		return 0;

	beat = eof_get_beat(eof_song, pos);	//Find which beat the specified position is in
	if(!eof_beat_num_valid(eof_song, beat))	//If the seek position is outside the chart
		return 0;

	if(pos >= eof_song->beat[eof_song->beats - 1]->pos)	//If the specified position is after the last beat marker
		return 0;

	eof_snap_logic(&temp, pos);				//Find the next grid snap position that occurs after the specified position

	return (temp.pos == pos);
}

int eof_is_any_grid_snap_position(unsigned long pos, int *beat, char *gridsnapvalue, unsigned char *gridsnapnum)
{
	EOF_SNAP_DATA temp = {0, 0.0, 0, 0.0, 0, 0, 0, {0.0}, {0.0}, 0, 0, 0, 0};
	unsigned long beatnum;
	int retval = 0, ctr, lastsnap = EOF_SNAP_CUSTOM;
	char temp_mode = eof_snap_mode;	//Store the grid snap setting in use
	char foundgridsnapvalue = 0, foundgridsnapnum = 0;
	int foundbeat = -1;

	if(!eof_song)
		return 0;

	beatnum = eof_get_beat(eof_song, pos);	//Find which beat the specified position is in
	if(!eof_beat_num_valid(eof_song, beatnum))	//If the seek position is outside the chart
		return 0;

	if(pos >= eof_song->beat[eof_song->beats - 1]->pos)	//If the specified position is after the last beat marker
		return 0;

	if(temp_mode != EOF_SNAP_CUSTOM)
		lastsnap = EOF_SNAP_CUSTOM - 1;	//If no custom grid snap is in effect, it won't be checked in the loop below

	for(ctr = 1; ctr <= lastsnap; ctr++)
	{	//For each of the applicable grid snap settings
		eof_snap_mode = ctr;			//Put this grid snap setting into effect
		eof_snap_logic(&temp, pos);		//Find the next grid snap position that occurs after the specified position
		if(temp.pos != pos)
			continue;	//If the nearest grid snap position isn't the position specified by the calling function, skip this grid snap setting

		retval = 1;	//Track that a matching grid snap position was found
		foundgridsnapvalue = eof_snap_mode;
		foundbeat = temp.beat;
		if(gridsnapnum)
		{	//If the calling function wanted to know which grid snap number the position aligned with
			for(ctr = 0; (ctr < EOF_MAX_GRID_SNAP_INTERVALS) && (ctr < temp.intervals); ctr++)
			{	//For each grid snap position determined by eof_snap_logic()
				if((unsigned long)(temp.grid_pos[ctr] + 0.5) == pos)
				{	//If this is the matching grid snap position, when rounded to the nearest millisecond
					foundgridsnapnum = ctr;
					break;
				}
			}
		}
		break;
	}

	//Return any desired values back to the calling function
	if(gridsnapnum)
		*gridsnapnum = foundgridsnapnum;
	if(gridsnapvalue)
		*gridsnapvalue = foundgridsnapvalue;
	if(beat)
		*beat = foundbeat;
	eof_snap_mode = temp_mode;	//Restore the grid snap value that was in effect
	return retval;
}

int eof_find_grid_snap_position(unsigned long beat, char gridsnapvalue, unsigned char gridsnapnum, unsigned long *gridpos)
{
	EOF_SNAP_DATA temp = {0, 0.0, 0, 0.0, 0, 0, 0, {0.0}, {0.0}, 0, 0, 0, 0};
	char temp_mode = eof_snap_mode;	//Store the grid snap setting in use

	if(!eof_song || (beat >= eof_song->beats) || !gridpos)
		return 0;	//Invalid parameters

	if(!gridsnapvalue)
		return 0;	//The parameters do not define a valid grid snap position

	eof_snap_mode = gridsnapvalue;	//Put this grid snap setting into effect
	eof_snap_logic(&temp, eof_song->beat[beat]->pos);	//Calculate grid snaps for the specified beat
	eof_snap_mode = temp_mode;	//Restore the grid snap setting that was in use
	if(gridsnapnum >= temp.intervals)
		return 0;	//The gridsnapnum is not valid

	*gridpos = temp.grid_pos[gridsnapnum] + 0.5;	//Store the grid snap's real time position, rounded up to the nearest millisecond
	return 1;	//Return success
}

void eof_read_editor_keys(void)
{
//	eof_log("eof_read_editor_keys() entered");

	unsigned long tracknum;
	unsigned long bitmask = 0;	//Used for simplifying note placement logic
	EOF_NOTE * new_note = NULL;
	EOF_LYRIC * new_lyric = NULL;
	unsigned long numlanes = eof_count_track_lanes(eof_song, eof_selected_track);
	unsigned long note;
	unsigned char do_pg_up = 0, do_pg_dn = 0;
	int eof_scaled_mouse_y = mouse_y;	//Rescaled mouse coordinate that accounts for the x2 zoom display feature

	if(!eof_song_loaded)
		return;	//Don't handle these keyboard shortcuts unless a chart is loaded

	if(eof_track_is_legacy_guitar(eof_song, eof_selected_track) && !eof_open_strum_enabled(eof_selected_track))
	{	//Special case:  Legacy guitar tracks can use a sixth lane but hide that lane if it is not enabled
		numlanes = 5;
	}
	if(eof_screen_zoom)
	{	//If x2 zoom is in effect, take that into account for the mouse position
		eof_scaled_mouse_y = mouse_y / 2;
	}
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	eof_read_controller(&eof_guitar);
	eof_read_controller(&eof_drums);

///DEBUG
if(eof_key_code == KEY_PAUSE)
{
	eof_use_key();
	//Debug action here
}

/* keyboard shortcuts that may or may not be used when the chart/catalog is playing */

	/* seek to first note (CTRL+Home) */
	/* select previous (SHIFT+Home) */
	/* seek to beginning (Home) */
	/* seek to first beat (CTRL+SHIFT+Home) */
	if(eof_key_code == KEY_HOME)
	{
		if(KEY_EITHER_CTRL && !KEY_EITHER_SHIFT)
		{	//If CTRL is held but SHIFT is not
			(void) eof_menu_song_seek_first_note();
		}
		else if(KEY_EITHER_SHIFT && !KEY_EITHER_CTRL)
		{	//If SHIFT is held but CTRL is not
			eof_shift_used = 1;	//Track that the SHIFT key was used
			(void) eof_menu_edit_select_previous();
		}
		else if(KEY_EITHER_CTRL && KEY_EITHER_SHIFT)
		{	//If both CTRL and SHIFT are held
			eof_shift_used = 1;	//Track that the SHIFT key was used
			(void) eof_menu_song_seek_first_beat();
		}
		else
		{
			(void) eof_menu_song_seek_start();
		}
		eof_use_key();
	}

	/* seek to last note (CTRL+End) */
	/* select rest (SHIFT+End) */
	/* seek to end of audio (End) */
	/* seek to end of chart (CTRL+SHIFT+End) */
	if(eof_key_code == KEY_END)
	{
		if(KEY_EITHER_CTRL)
		{
			if(KEY_EITHER_SHIFT)
			{	//If both SHIFT and CTRL are being held
				eof_shift_used = 1;	//Track that the SHIFT key was used
				(void) eof_menu_song_seek_chart_end();
			}
			else
			{	//If only CTRL is being held
				(void) eof_menu_song_seek_last_note();
			}
		}
		else if(KEY_EITHER_SHIFT)
		{	//If only SHIFT is being held
			eof_shift_used = 1;	//Track that the SHIFT key was used
			(void) eof_menu_edit_select_rest();
		}
		else
		{	//If neither SHIFT nor CTRL are being held
			(void) eof_menu_song_seek_end();
		}
		eof_use_key();
	}

	/* rewind (R) */
	/* resnap notes (CTRL+SHIFT+R) */
	if(eof_key_char == 'r')
	{
		if(!KEY_EITHER_CTRL && !KEY_EITHER_SHIFT)
		{	//If neither CTRL nor SHIFT are held
			(void) eof_menu_song_seek_rewind();
			eof_use_key();
		}
		else if(KEY_EITHER_CTRL && KEY_EITHER_SHIFT)
		{	//If both CTRL and SHIFT are held
			(void) eof_menu_note_resnap();
			eof_shift_used = 1;	//Track that the SHIFT key was used
			eof_use_key();
		}
	}

	/* zoom in (+ on numpad) */
	/* increment AV delay (CTRL+SHIFT+(plus) on numpad) */
	/* lower 3D camera angle (SHIFT+(plus) on numpad) or BACKSLASH */
	if((eof_key_code == KEY_PLUS_PAD) || ((eof_key_code == KEY_BACKSLASH) && !KEY_EITHER_SHIFT && !KEY_EITHER_CTRL))
	{
		if(!KEY_EITHER_CTRL)
		{	//If CTRL is not being held
			if(!KEY_EITHER_SHIFT && !(eof_key_code == KEY_BACKSLASH))
			{	//If neither SHIFT nor backslash are being held either
				(void) eof_menu_edit_zoom_helper_in();
			}
			else
			{	//SHIFT is being held, CTRL is not
				eof_shift_used = 1;	//Track that the SHIFT key was used
				eof_vanish_y += 10;
				if(eof_vanish_y > 260)
				{	//Do not allow it to go higher than this, otherwise the view-able portion of the chart goes partly off-screen
					eof_vanish_y = 260;
				}
				eof_3d_fretboard_coordinates_cached = 0;	//The 3D rendering logic will need to rebuild the fretboard's 2D coordinate projections
				eof_set_3d_projection();
			}
		}
		else if(KEY_EITHER_SHIFT)
		{	//CTRL and SHIFT are both being held
			eof_shift_used = 1;	//Track that the SHIFT key was used
			eof_av_delay++;
		}
		eof_use_key();
	}

	/* zoom out (- on numpad) */
	/* decrement AV delay (CTRL+SHIFT+(minus) on numpad) */
	/* raise 3D camera angle (SHIFT+(minus) on numpad) or SHIFT+BACKSLASH */
	if((eof_key_code == KEY_MINUS_PAD) || ((eof_key_code == KEY_BACKSLASH) && KEY_EITHER_SHIFT && !KEY_EITHER_CTRL))
	{
		if(!KEY_EITHER_CTRL)
		{	//If CTRL is not being held
			if(!KEY_EITHER_SHIFT)
			{	//If SHIFT is not being held either
				(void) eof_menu_edit_zoom_helper_out();
			}
			else
			{	//SHIFT is being held, CTRL is not
				eof_shift_used = 1;	//Track that the SHIFT key was used
				eof_vanish_y -= 10;
				if(eof_vanish_y < -500)
				{	//Do not allow it to go too negative, things start to look weird
					eof_vanish_y = -500;
				}
				eof_3d_fretboard_coordinates_cached = 0;	//The 3D rendering logic will need to rebuild the fretboard's 2D coordinate projections
				eof_set_3d_projection();
			}
		}
		else if(KEY_EITHER_SHIFT)
		{	//CTRL and SHIFT are both being held
			eof_shift_used = 1;	//Track that the SHIFT key was used
			if(eof_av_delay > 0)
			{
				eof_av_delay--;
			}
		}
		eof_use_key();
	}

	/* reset 3D camera angle (SHIFT+Enter on numpad or CTRL+BACKSLASH) */
	if(((eof_key_code == KEY_ENTER_PAD) && KEY_EITHER_SHIFT) || ((eof_key_code == KEY_BACKSLASH) && KEY_EITHER_CTRL && !KEY_EITHER_SHIFT))
	{
		if(KEY_EITHER_SHIFT)
		{
			eof_shift_used = 1;	//Track that the SHIFT key was used
		}
		eof_vanish_y = 0;
		eof_3d_fretboard_coordinates_cached = 0;	//The 3D rendering logic will need to rebuild the fretboard's 2D coordinate projections
		eof_set_3d_projection();
		eof_use_key();
	}

	/* show/hide catalog (Q) */
	/* quick save (CTRL+Q) */
	/* double catalog width (SHIFT+Q) */
	if(eof_key_char == 'q')
	{
		if(!KEY_EITHER_SHIFT)
		{	//SHIFT is not held
			if(!KEY_EITHER_CTRL)
			{	//Neither SHIFT nor CTRL are held
				(void) eof_menu_catalog_show();
			}
			else
			{	//CTRL is held
				(void) eof_menu_file_quick_save();
			}
		}
		else
		{	//SHIFT is held
			eof_shift_used = 1;	//Track that the SHIFT key was used
			(void) eof_menu_catalog_toggle_full_width();
		}
		eof_use_key();
	}

	/* previous chord name match (CTRL+SHIFT+W)  */
	/* add catalog entry (SHIFT+W) */
	/* mark star power (CTRL+W) */
	/* previous catalog entry (W) */
	if(eof_key_char == 'w')
	{
		if(KEY_EITHER_SHIFT && KEY_EITHER_CTRL)
		{	//Both SHIFT and CTRL are held
			eof_shift_used = 1;	//Track that the SHIFT key was used
			(void) eof_menu_previous_chord_result();
			eof_use_key();
		}
		else if(KEY_EITHER_SHIFT && !KEY_EITHER_CTRL)
		{	//SHIFT is held but CTRL is not
			eof_shift_used = 1;	//Track that the SHIFT key was used
			(void) eof_menu_catalog_add();
			eof_use_key();
		}
		else if(!KEY_EITHER_SHIFT && KEY_EITHER_CTRL)
		{	//SHIFT is not held but CTRL is
			(void) eof_menu_star_power_mark();
			eof_use_key();
		}
		else if(!KEY_EITHER_SHIFT && !KEY_EITHER_CTRL)
		{	//Neither SHIFT nor CTRL are held
			(void) eof_menu_catalog_previous();
			eof_use_key();
		}
	}

	/* toggle expert+ bass drum (CTRL+E) */
	/* next chord name match (CTRL+SHIFT+E) */
	/* place section event (SHIFT+E) */
	/* next catalog entry (E) */
	if(eof_key_char == 'e')
	{
		if(KEY_EITHER_CTRL && !KEY_EITHER_SHIFT)
		{	//CTRL+E will toggle Expert+ double bass drum notation
			(void) eof_menu_note_toggle_double_bass();
			eof_use_key();
		}
		else
		{
			if(KEY_EITHER_SHIFT && KEY_EITHER_CTRL)
			{	//CTRL+SHIFT+E will cycle to the next chord name match
				eof_shift_used = 1;	//Track that the SHIFT key was used
				(void) eof_menu_next_chord_result();
				eof_use_key();
			}
			else if(!KEY_EITHER_SHIFT && !KEY_EITHER_CTRL)
			{	//E will cycle to the next catalog entry
				(void) eof_menu_catalog_next();
				eof_use_key();
			}
			else if(KEY_EITHER_SHIFT && !KEY_EITHER_CTRL)
			{	//SHIFT+E will place a section event
				eof_shift_used = 1;	//Track that the SHIFT key was used
				(void) eof_menu_beat_add_section();
				eof_use_key();
			}
		}
	}

	/* toggle green cymbal (CTRL+G in a drum track) */
	/* toggle grid snap (G) */
	/* display grid lines (SHIFT+G) */
	if(eof_key_char == 'g')
	{
		if(KEY_EITHER_CTRL)
		{
			if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
			{	//If a drum track is active
				(void) eof_menu_note_toggle_rb3_cymbal_green();
				eof_use_key();
			}
		}
		else
		{	//CTRL is not held
			if(KEY_EITHER_SHIFT)
			{	//SHIFT is held
				eof_shift_used = 1;	//Track that the SHIFT key was used
				(void) eof_menu_edit_toggle_grid_lines();
				eof_use_key();
			}
			else
			{	//Neither SHIFT nor CTRL are held
				if(eof_snap_mode == EOF_SNAP_OFF)
				{
					eof_snap_mode = eof_last_snap_mode;
				}
				else
				{
					eof_last_snap_mode = eof_snap_mode;
					eof_snap_mode = EOF_SNAP_OFF;
				}
				eof_use_key();
			}
		}
	}

	/* toggle yellow cymbal (CTRL+Y) */
	/* seek to next highlighted note (SHIFT+Y) */
	/* seek to previous highlighted note (CTRL+SHIFT+Y) */
	if(eof_key_char == 'y')
	{	//CTRL+Y will toggle Pro yellow cymbal notation
		if(KEY_EITHER_CTRL)
		{	//If CTRL is held
			if(KEY_EITHER_SHIFT)
			{	//If both CTRL and SHIFT are held
				eof_shift_used = 1;	//Track that the SHIFT key was used
				(void) eof_menu_song_seek_previous_highlighted_note();
				eof_use_key();
			}
			else
			{	//Only CTRL is held
				(void) eof_menu_note_toggle_rb3_cymbal_yellow();
				eof_use_key();
			}
		}
		else
		{	//CTRL is not held
			if(KEY_EITHER_SHIFT)
			{	//If only SHIFT is held
				eof_shift_used = 1;	//Track that the SHIFT key was used
				(void) eof_menu_song_seek_next_highlighted_note();
				eof_use_key();
			}
		}
	}

	/* toggle blue cymbal (CTRL+B in the drum track) */
	/* toggle bend (CTRL+B in a pro guitar track) */
	/* set bend strength (SHIFT+B in a pro guitar track) */
	/* seek to beat/measure (CTRL+SHIFT+B) */
	if(eof_key_char == 'b')
	{
		if(KEY_EITHER_CTRL)
		{	//If CTRL is held
			if(KEY_EITHER_SHIFT)
			{	//If both CTRL and SHIFT are held
				eof_shift_used = 1;	//Track that the SHIFT key was used
				(void) eof_menu_song_seek_beat_measure();
				eof_use_key();
			}
			else
			{	//Only CTRL is held
				if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
				{	//If a drum track is active
					(void) eof_menu_note_toggle_rb3_cymbal_blue();
					eof_use_key();
				}
				else if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
				{
					(void) eof_menu_note_toggle_bend();
					eof_use_key();
				}
			}
		}
		else if(KEY_EITHER_SHIFT)
		{	//If only SHIFT is held
			eof_shift_used = 1;	//Track that the SHIFT key was used
			(void) eof_pro_guitar_note_bend_strength_save();
			eof_use_key();
		}
	}

	/* cycle track backward (CTRL+SHIFT+Tab) */
	/* cycle track forward (CTRL+Tab) */
	/* cycle difficulty backward (SHIFT+Tab) */
	/* cycle difficulty forward (Tab) */
	if(eof_key_code == KEY_TAB)
	{	//Read the scan code because the ASCII code cannot represent CTRL or SHIFT with the tab key
		if(KEY_EITHER_CTRL)
		{	//Track numbering begins at 1 instead of 0
			if(KEY_EITHER_SHIFT)	//Shift instrument back 1 number
			{
				eof_shift_used = 1;	//Track that the SHIFT key was used
				if(eof_selected_track > EOF_TRACKS_MIN)
					(void) eof_menu_track_selected_track_number(eof_selected_track - 1, 1);
				else
					(void) eof_menu_track_selected_track_number(EOF_TRACKS_MAX - 1, 1);	//Wrap around
			}
			else					//Shift instrument forward 1 number
			{
				if(eof_selected_track < EOF_TRACKS_MAX - 1)
					(void) eof_menu_track_selected_track_number(eof_selected_track + 1, 1);
				else
					(void) eof_menu_track_selected_track_number(EOF_TRACKS_MIN, 1);	//Wrap around
			}
		}
		else
		{
			unsigned char eof_note_type_max = eof_set_active_difficulty(eof_note_type);	//Determine the number of usable difficulties in the active track

			if(KEY_EITHER_SHIFT)
			{
				eof_shift_used = 1;	//Track that the SHIFT key was used
				if(eof_note_type == 0)
				{	//Wrap around
					eof_note_type = eof_note_type_max;
				}
				else
				{
					eof_note_type--;
				}
			}
			else
			{
				if(eof_note_type >= eof_note_type_max)
				{	//Wrap around
					eof_note_type = 0;
				}
				else
				{
					eof_note_type++;
				}
			}
			eof_fix_window_title();
			(void) eof_detect_difficulties(eof_song, eof_selected_track);
		}
		eof_mix_find_claps();
		eof_mix_start_helper();
		eof_use_key();
	}

	/* play/pause music (Space) */
	/* play catalog entry (SHIFT+Space) */
	if((eof_key_code == KEY_SPACE) && !eof_silence_loaded)
	{	//Only allow playback controls when chart audio is loaded (use the scan code because CTRL+SHIFT+Space cannot be reliably detected via ASCII value)
		if(KEY_EITHER_SHIFT && !KEY_EITHER_CTRL)
		{
			eof_shift_used = 1;	//Track that the SHIFT key was used
			eof_catalog_play();
		}
		else
		{
			if(eof_music_catalog_playback)
			{
				eof_music_catalog_playback = 0;
				eof_music_catalog_pos = eof_song->catalog->entry[eof_selected_catalog_entry].start_pos + eof_av_delay;
				eof_stop_midi();
				alogg_stop_ogg(eof_music_track);
				alogg_seek_abs_msecs_ogg_ul(eof_music_track, eof_music_pos);
			}
			else
			{
				eof_music_play(0);
				if(eof_music_paused)
				{
					eof_track_fixup_notes(eof_song, eof_selected_track, 1);
					if(eof_undo_last_type == EOF_UNDO_TYPE_RECORD)
					{
						eof_undo_last_type = 0;
					}
					if(eof_input_mode == EOF_INPUT_FEEDBACK)
					{	//If Feedback input method is in effect
						eof_seek_to_nearest_grid_snap();	//Seek to the nearest grid snap position when playback is stopped
					}
				}
			}
		}
		eof_use_key();
	}//Only allow playback controls when chart audio is loaded

	/* Ensure the seek position is no further left than the first beat marker if Feedback input method is in effect */
	if((eof_input_mode == EOF_INPUT_FEEDBACK) && (eof_music_pos < eof_song->beat[0]->pos + eof_av_delay))
	{
		eof_set_seek_position(eof_song->beat[0]->pos + eof_av_delay);
	}

	/* rewind (Left, non Feedback input methods) */
	/* Decrease grid snap (Left, Feedback input method)*/
	if(key[KEY_LEFT])
	{
		if(eof_input_mode == EOF_INPUT_FEEDBACK)
		{
			stop_sample(eof_sound_grid_snap);
			(void) play_sample(eof_sound_grid_snap, 255.0 * (eof_tone_volume / 100.0), 127, 1000 + eof_audio_fine_tune, 0);	//Play this sound clip
			eof_snap_mode--;
			if(eof_snap_mode < 0)
			{
				eof_snap_mode = EOF_SNAP_NINTY_SIXTH;
			}
			key[KEY_LEFT] = 0;
		}
		else
		{
			if(eof_fb_seek_controls)
			{	//If the "Use dB style seek controls" preference is enabled
				(void) eof_menu_song_seek_previous_grid_snap();
				key[KEY_LEFT] = 0;
			}
			else
			{
				eof_music_rewind();
				if(KEY_EITHER_SHIFT && KEY_EITHER_CTRL)
				{	//If user is trying to seek at the slowest speed,
					eof_shift_used = 1;	//Track that the SHIFT key was used
					key[KEY_LEFT] = 0;	//Clear this key state to allow seeking in accurate 1ms intervals
				}
			}
		}
	}

	/* fast forward (Right, non Feedback input methods) */
	/* Increase grid snap (Right, Feedback input method)*/
	if(key[KEY_RIGHT])
	{
		if(eof_input_mode == EOF_INPUT_FEEDBACK)
		{
			stop_sample(eof_sound_grid_snap);
			(void) play_sample(eof_sound_grid_snap, 255.0 * (eof_tone_volume / 100.0), 127, 1000 + eof_audio_fine_tune, 0);	//Play this sound clip
			eof_snap_mode++;
			if(eof_snap_mode > EOF_SNAP_NINTY_SIXTH)
			{
				eof_snap_mode = 0;
			}
			key[KEY_RIGHT] = 0;
		}
		else
		{
			if(eof_fb_seek_controls)
			{	//If the "Use dB style seek controls" preference is enabled
				(void) eof_menu_song_seek_next_grid_snap();
				key[KEY_RIGHT] = 0;
			}
			else
			{
				eof_music_forward();
				if(KEY_EITHER_SHIFT && KEY_EITHER_CTRL)
				{	//If user is trying to seek at the slowest speed,
					eof_shift_used = 1;	//Track that the SHIFT key was used
					key[KEY_RIGHT] = 0;	//Clear this key state to allow seeking in accurate 1ms intervals
				}
			}
		}
	}

	/* The "Use dB style seek controls" user preference will control which direction the page up/down keys will seek */
	if(eof_key_code == KEY_PGUP)
	{
		if(eof_fb_seek_controls || (eof_input_mode == EOF_INPUT_FEEDBACK))
			do_pg_dn = 1;
		else
			do_pg_up = 1;
		eof_use_key();
	}
	if(eof_key_code == KEY_PGDN)
	{
		if(eof_fb_seek_controls || (eof_input_mode == EOF_INPUT_FEEDBACK))
			do_pg_up = 1;
		else
			do_pg_dn = 1;
		eof_use_key();
	}

	/* seek back one grid snap (CTRL+SHIFT+Pg Up when grid snap is enabled) */
	/* seek back one anchor (CTRL+SHIFT+Pg Up when grid snap is disabled) */
	/* seek back one screen (CTRL+Pg Up) */
	/* seek back one note (SHIFT+Pg Up, non Feedback input modes) */
	/* seek back one measure (Pg Up, Feedback input mode) */
	/* seek back one beat (Pg Up, non Feedback input modes) */
	if(do_pg_up)
	{
		if(!eof_music_catalog_playback)
		{
			if(KEY_EITHER_CTRL)
			{
				if(KEY_EITHER_SHIFT)
				{	//If both SHIFT and CTRL are being held
					eof_shift_used = 1;	//Track that the SHIFT key was used
					if(eof_snap_mode != EOF_SNAP_OFF)
					{	//If grid snap is enabled
						(void) eof_menu_song_seek_previous_grid_snap();
					}
					else
					{
						(void) eof_menu_song_seek_previous_anchor();
					}
				}
				else
				{	//If only CTRL is being held
					(void) eof_menu_song_seek_previous_screen();
				}
			}
			else if(KEY_EITHER_SHIFT)
			{	//If only SHIFT is being held
				eof_shift_used = 1;	//Track that the SHIFT key was used
				if(eof_input_mode == EOF_INPUT_FEEDBACK)
				{	//In Feedback input method, this changes the note selection
					(void) eof_menu_song_seek_previous_measure();
				}
				else
				{	//Otherwise seek to previous note
					(void) eof_menu_song_seek_previous_note();
				}
			}
			else
			{	//If neither SHIFT nor CTRL are being held
				if(eof_input_mode == EOF_INPUT_FEEDBACK)
				{
					(void) eof_menu_song_seek_previous_measure();
				}
				else
				{
					(void) eof_menu_song_seek_previous_beat();
				}
			}
		}
	}

	/* seek forward one grid snap (CTRL+SHIFT+Pg Dn when grid snap is enabled) */
	/* seek forward one anchor (CTRL+SHIFT+Pg Dn when grid snap is disabled) */
	/* seek forward one screen (CTRL+Pg Dn) */
	/* seek forward one note (SHIFT+Pg Dn, non Feedback input modes) */
	/* seek forward one measure (Pg Up, Feedback input mode) */
	/* seek forward one beat (Pg Dn, non Feedback input modes) */
	if(do_pg_dn)
	{
		if(!eof_music_catalog_playback)
		{
			if(KEY_EITHER_CTRL)
			{
				if(KEY_EITHER_SHIFT)
				{	//If both SHIFT and CTRL are being held
					eof_shift_used = 1;	//Track that the SHIFT key was used
					if(eof_snap_mode != EOF_SNAP_OFF)
					{	//If grid snap is enabled
						(void) eof_menu_song_seek_next_grid_snap();
					}
					else
					{
						(void) eof_menu_song_seek_next_anchor();
					}
				}
				else
				{	//If only CTRL is being held
					(void) eof_menu_song_seek_next_screen();
				}
			}
			else if(KEY_EITHER_SHIFT)
			{	//If only SHIFT is being held
				eof_shift_used = 1;	//Track that the SHIFT key was used
				if(eof_input_mode == EOF_INPUT_FEEDBACK)
				{	//In Feedback input method, this changes the note selection
					(void) eof_menu_song_seek_next_measure();
				}
				else
				{	//Otherwise seek to next note
					(void) eof_menu_song_seek_next_note();
				}
			}
			else
			{	//If neither SHIFT nor CTRL are being held
				if(eof_input_mode == EOF_INPUT_FEEDBACK)
				{
					(void) eof_menu_song_seek_next_measure();
				}
				else
				{
					(void) eof_menu_song_seek_next_beat();
				}
			}
		}
	}

	/* transpose mini piano visible area up one octave (CTRL+SHIFT+Up in a vocal track) */
	/* transpose mini piano visible area up one (SHIFT+Up in a vocal track, non Feedback input modes) */
	/* change to the next track of the same type (CTRL+SHIFT+Up in a non vocal track) */
	/* transpose lyric up one octave (CTRL+Up in a vocal track) */
	/* toggle pro guitar slide up (CTRL+Up in a pro guitar track) */
	/* transpose note up (Up, non Feedback input methods, normal seek controls) */
	/* seek forward (Up, Feedback style seek direction controls) */
	/* seek forward one grid snap (Up, Feedback input method) */
	/* toggle pro guitar strum up (SHIFT+Up in a pro guitar track, non Feedback input modes) */
	if(key[KEY_UP])
	{
		if(KEY_EITHER_SHIFT)
		{	//SHIFT is held
			if(eof_vocals_selected)
			{	//The vocal track is active
				if(KEY_EITHER_CTRL)
				{	//Both CTRL and SHIFT are held
					eof_shift_used = 1;	//Track that the SHIFT key was used
					eof_vocals_offset += 12;
				}
				else if(eof_input_mode != EOF_INPUT_FEEDBACK)
				{	//Only SHIFT is held, a non feedback input mode is in use
					eof_shift_used = 1;	//Track that the SHIFT key was used
					eof_vocals_offset++;
				}
				else if(eof_music_paused && !eof_music_catalog_playback)
				{	//Only SHIFT is held and feedback input mode is in use
					(void) eof_menu_song_seek_next_grid_snap();
				}
				if(eof_vocals_offset > MAXPITCH - 12)
				{	//Don't allow the offset to go higher than the last usable octave
					eof_vocals_offset = MAXPITCH - 12;
				}
			}
			else
			{	//A non vocal track is active
				if(KEY_EITHER_CTRL)
				{	//Both CTRL and SHIFT are held
					char format = eof_song->track[eof_selected_track]->track_format;	//Remember what format this track is
					eof_shift_used = 1;	//Track that the SHIFT key was used
					do{	//Cycle track forward until one of the same format is reached
						if(eof_selected_track < EOF_TRACKS_MAX - 1)
						{	//If this isn't the last usable track in the project
							eof_selected_track++;
						}
						else
						{
							eof_selected_track = EOF_TRACKS_MIN;	//Otherwise wrap around to the first usable track
						}
					}while(eof_song->track[eof_selected_track]->track_format != format);
					(void) eof_menu_track_selected_track_number(eof_selected_track, 1);	//Set the track
					eof_mix_find_claps();		//Update sound cues
				}
				else if(eof_input_mode != EOF_INPUT_FEEDBACK)
				{	//Only SHIFT is held, a non feedback input mode is in use
					if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
					{
						eof_shift_used = 1;	//Track that the SHIFT key was used
						(void) eof_pro_guitar_toggle_strum_up();
					}
				}
				else if(eof_music_paused && !eof_music_catalog_playback)
				{	//Only SHIFT is held and feedback input mode is in use
					(void) eof_menu_song_seek_next_grid_snap();
				}
			}
		}
		else if(eof_music_paused && !eof_music_catalog_playback)
		{	//Shift is not held and chart is paused
			if(KEY_EITHER_CTRL)
			{
				if(eof_vocals_selected)
				{	/* tranpose up octave */
					(void) eof_menu_note_transpose_up_octave();	//Move up 12 pitches at once, performing a single undo beforehand
				}
				else if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
				{	/* toggle slide up */
					(void) eof_menu_note_toggle_slide_up();
				}
			}
			else
			{	//Neither SHIFT nor CTRL are held
				if(eof_input_mode == EOF_INPUT_FEEDBACK)
				{
					(void) eof_menu_song_seek_next_grid_snap();
				}
				else
				{
					(void) eof_menu_note_transpose_up();
				}
			}
			if(eof_vocals_selected && eof_mix_vocal_tones_enabled && (eof_selection.current < eof_song->vocal_track[tracknum]->lyrics) && (eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->note != EOF_LYRIC_PERCUSSION))
			{
				eof_mix_play_note(eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->note);
			}
		}
		key[KEY_UP] = 0;
	}

	/* transpose mini piano visible area down one octave (CTRL+SHIFT+Down) */
	/* transpose mini piano visible area down one (SHIFT+Down, non Feedback input modes) */
	/* change to the previous track of the same type (CTRL+SHIFT+Down in a non vocal track) */
	/* transpose lyric down one octave (CTRL+Down in a vocal track) */
	/* toggle pro guitar slide down (CTRL+Down in a pro guitar track) */
	/* transpose note down (Down, non Feedback input methods) */
	/* seek backward one grid snap (Down, Feedback input method, normal seek controls) */
	/* seek backward (Down, Feedback style seek direction controls) */
	/* toggle pro guitar strum down (SHIFT+Down in a pro guitar track, non Feedback input modes) */
	if(key[KEY_DOWN])
	{
		if(KEY_EITHER_SHIFT)
		{	//SHIFT is held
			if(eof_vocals_selected)
			{	//The vocal track is active
				if(KEY_EITHER_CTRL)
				{	//Both CTRL and SHIFT are held
					eof_shift_used = 1;	//Track that the SHIFT key was used
					eof_vocals_offset -= 12;
				}
				else if(eof_input_mode != EOF_INPUT_FEEDBACK)
				{	//Only SHIFT is held, a non feedback input mode is in use
					eof_shift_used = 1;	//Track that the SHIFT key was used
					eof_vocals_offset--;
				}
				else if(eof_music_paused && !eof_music_catalog_playback)
				{	//Only SHIFT is held, the chart is paused and feedback input mode is in use
					(void) eof_menu_song_seek_previous_grid_snap();
				}
				if(eof_vocals_offset < MINPITCH)
				{
					eof_vocals_offset = MINPITCH;
				}
			}
			else
			{	//A non vocal track is active
				if(KEY_EITHER_CTRL)
				{	//Both CTRL and SHIFT are held
					char format = eof_song->track[eof_selected_track]->track_format;	//Remember what format this track is
					eof_shift_used = 1;	//Track that the SHIFT key was used
					do{	//Cycle track backward until one of the same format is reached
						if(eof_selected_track > EOF_TRACKS_MIN)
						{	//If this isn't the first usable track in the project
							eof_selected_track--;
						}
						else
						{
							eof_selected_track = EOF_TRACKS_MAX - 1;	//Otherwise wrap around to the last usable track
						}
					}while(eof_song->track[eof_selected_track]->track_format != format);
					(void) eof_menu_track_selected_track_number(eof_selected_track, 1);	//Set the track
					eof_mix_find_claps();		//Update sound cues
				}
				else if(eof_input_mode != EOF_INPUT_FEEDBACK)
				{	//Only SHIFT is held, a non feedback input mode is in use
					if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
					{
						eof_shift_used = 1;	//Track that the SHIFT key was used
						(void) eof_pro_guitar_toggle_strum_down();
					}
				}
				else if(eof_music_paused && !eof_music_catalog_playback)
				{	//Only SHIFT is held, the chart is paused and feedback input mode is in use
					eof_shift_used = 1;	//Track that the SHIFT key was used
					(void) eof_menu_song_seek_previous_grid_snap();
				}
			}
		}
		else if(eof_music_paused && !eof_music_catalog_playback)
		{	//Shift is not held and chart is paused
			if(KEY_EITHER_CTRL)
			{
				if(eof_vocals_selected)
				{	/* transpose down octave */
					(void) eof_menu_note_transpose_down_octave();	//Move down 12 pitches at once, performing a single undo beforehand
				}
				else if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
				{	/* toggle slide down */
					(void) eof_menu_note_toggle_slide_down();
				}
			}
			else
			{	//Neither SHIFT nor CTRL are held
				if(eof_input_mode == EOF_INPUT_FEEDBACK)
				{
					(void) eof_menu_song_seek_previous_grid_snap();
				}
				else
				{
					(void) eof_menu_note_transpose_down();
				}
			}
			if(eof_vocals_selected && eof_mix_vocal_tones_enabled && (eof_selection.current < eof_song->vocal_track[tracknum]->lyrics) && (eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->note != EOF_LYRIC_PERCUSSION))
			{
				eof_mix_play_note(eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->note);
			}
		}
		key[KEY_DOWN] = 0;
	}

	/* decrease grid snap (,) */
	if(eof_key_char == ',')
	{
		if(eof_input_mode == EOF_INPUT_FEEDBACK)
		{	//If Feedback input method is in effect
			stop_sample(eof_sound_grid_snap);
			(void) play_sample(eof_sound_grid_snap, 255.0 * (eof_tone_volume / 100.0), 127, 1000 + eof_audio_fine_tune, 0);	//Play this sound clip
		}
		eof_snap_mode--;
		if(eof_snap_mode < 0)
		{
			eof_snap_mode = EOF_SNAP_NINTY_SIXTH;
		}
		eof_fixup_notes(eof_song);	//Run the fixup logic for all tracks, so that if the "Highlight non grid snapped notes" feature is in use, the highlighting can discontinue taking the custom grid snap into account
		eof_use_key();
	}

	/* increase grid snap (.) */
	if(eof_key_char == '.')
	{
		if(eof_input_mode == EOF_INPUT_FEEDBACK)
		{	//If Feedback input method is in effect
			stop_sample(eof_sound_grid_snap);
			(void) play_sample(eof_sound_grid_snap, 255.0 * (eof_tone_volume / 100.0), 127, 1000 + eof_audio_fine_tune, 0);	//Play this sound clip
		}
		eof_snap_mode++;
		if(eof_snap_mode > EOF_SNAP_NINTY_SIXTH)
		{
			eof_snap_mode = 0;
		}
		eof_fixup_notes(eof_song);	//Run the fixup logic for all tracks, so that if the "Highlight non grid snapped notes" feature is in use, the highlighting can discontinue taking the custom grid snap into account
		eof_use_key();
	}

	/* manage RS phrases (CTRL+SHIFT+M in a pro guitar track) */
	/* toggle palm muting (CTRL+M in a pro guitar track) */
	/* mark/remark lyric phrase (CTRL+M in a vocal track) */
	/* toggle strum mid (SHIFT+M in a pro guitar track) */
	/* toggle metronome (M) */
	if(eof_key_char == 'm')
	{
		if(KEY_EITHER_CTRL)
		{
			if(KEY_EITHER_SHIFT)
			{	//If both CTRL and SHIFT are held
				eof_shift_used = 1;	//Track that the SHIFT key was used
				(void) eof_track_manage_rs_phrases();
			}
			else
			{	//If only CTRL is held
				if(eof_vocals_selected)
				{
					(void) eof_menu_lyric_line_mark();
				}
				else
				{
					(void) eof_menu_note_toggle_palm_muting();
				}
			}
		}
		else
		{	//CTRL is not held
			if(KEY_EITHER_SHIFT)
			{	//SHIFT+M toggles mid strum direction
				eof_shift_used = 1;	//Track that the SHIFT key was used
				(void) eof_pro_guitar_toggle_strum_mid();
			}
			else
			{	//M without SHIFT or CTRL toggles metronome
				(void) eof_menu_edit_metronome();
			}
		}
		eof_use_key();
	}

	/* toggle claps (C) */
	/* paste from catalog (SHIFT+C) */
	/* copy events (CTRL+SHIFT+C)*/
	if(eof_key_char == 'c')
	{
		if(!KEY_EITHER_CTRL && !KEY_EITHER_SHIFT)
		{	//Neither CTRL nor SHIFT held
			(void) eof_menu_edit_claps();
			eof_use_key();
		}
		else if(!KEY_EITHER_CTRL && KEY_EITHER_SHIFT)
		{	//If only SHIFT is held
			eof_shift_used = 1;	//Track that the SHIFT key was used
			(void) eof_menu_edit_paste_from_catalog();
			eof_use_key();
		}
		else if(KEY_EITHER_CTRL && KEY_EITHER_SHIFT)
		{	//If both CTRL and SHIFT are held
			eof_shift_used = 1;	//Track that the SHIFT key was used
			(void) eof_menu_beat_copy_events();
			eof_use_key();
		}
	}

	/* toggle vocal tones (V) */
	/* toggle vibrato (SHIFT+V in a pro guitar track) */
	/* paste events (CTRL+SHIFT+V) */
	if(eof_key_char == 'v')
	{
		if(!KEY_EITHER_CTRL)
		{	//If CTRL is not held
			if(!KEY_EITHER_SHIFT)
			{	//If SHIFT is not held
				(void) eof_menu_edit_vocal_tones();
			}
			else
			{	//SHIFT is held
				eof_shift_used = 1;	//Track that the SHIFT key was used
				(void) eof_menu_note_toggle_vibrato();
			}
			eof_use_key();
		}
		else if(KEY_EITHER_SHIFT)
		{	//If both CTRL and SHIFT are held
			eof_shift_used = 1;	//Track that the SHIFT key was used
			(void) eof_menu_beat_paste_events();
			eof_use_key();
		}
	}

	/* toggle MIDI tones (SHIFT+T) */
	if((eof_key_char == 't') && !KEY_EITHER_ALT)
	{
		if(KEY_EITHER_SHIFT && !KEY_EITHER_CTRL)
		{	//If SHIFT is held but CTRL is not
			(void) eof_menu_edit_midi_tones();
			eof_shift_used = 1;	//Track that the SHIFT key was used
			eof_use_key();
		}
	}

	/* toggle full screen 3D view (CTRL+F)*/
	if((eof_key_char == 'f') && KEY_EITHER_CTRL && !KEY_EITHER_SHIFT)
	{
		eof_full_screen_3d ^= 1;		//Toggle this setting on/off
		eof_use_key();
	}

	/* define custom time signature (SHIFT+I) */
	/* toggle info panel rendering (CTRL+I) */
	/* toggle ignore status (CTRL+SHIFT+I in a pro guitar track) */
	if(eof_key_char == 'i')
	{
		if(KEY_EITHER_CTRL)
		{	//If CTRL is held
			if(!KEY_EITHER_SHIFT)
			{	//CTRL is held but SHIFT is not
				eof_disable_info_panel = 1 - eof_disable_info_panel;
			}
			else
			{	//Both CTRL and SHIFT are held
				eof_shift_used = 1;	//Track that the SHIFT key was used
				(void) eof_menu_note_toggle_rs_ignore();
			}
			eof_use_key();
		}
		else
		{	//CTRL is not held
			if(KEY_EITHER_SHIFT)
			{	//SHIFT is held but CTRL is not
				(void) eof_menu_beat_ts_custom();
				eof_shift_used = 1;	//Track that the SHIFT key was used
				eof_use_key();
			}
		}
	}

	/* Hold and Classic input method logic */
	if(!KEY_EITHER_SHIFT && !eof_vocals_selected && !KEY_EITHER_CTRL)
	{	//If neither SHIFT nor CTRL are held and a non vocal track is active
		if(eof_input_mode == EOF_INPUT_HOLD)
		{	//If the input method is hold
			if(key[KEY_1])
			{
				eof_pen_note.note |= 1;	//Set the bit for lane 1
			}
			else
			{
				eof_pen_note.note &= (~1);	//Clear the bit for lane 1
			}
			if(key[KEY_2])
			{
				eof_pen_note.note |= 2;	//Set the bit for lane 2
			}
			else
			{
				eof_pen_note.note &= (~2);	//Clear the bit for lane 2
			}
			if(key[KEY_3])
			{
				eof_pen_note.note |= 4;	//Set the bit for lane 3
			}
			else
			{
				eof_pen_note.note &= (~4);	//Clear the bit for lane 3
			}
			if(key[KEY_4])
			{
				eof_pen_note.note |= 8;	//Set the bit for lane 4
			}
			else
			{
				eof_pen_note.note &= (~8);	//Clear the bit for lane 4
			}
			if(key[KEY_5] && (numlanes >= 5))
			{
				eof_pen_note.note |= 16;	//Set the bit for lane 5
			}
			else
			{
				eof_pen_note.note &= (~16);	//Clear the bit for lane 5
			}
			if(key[KEY_6] && (numlanes >= 6))
			{	//Only allow use of the 6 key if lane 6 is available
				eof_pen_note.note |= 32;	//Set the bit for lane 6
			}
			else
			{
				eof_pen_note.note &= (~32);	//Clear the bit for lane 6
			}
		}//If the input method is hold
		else if(eof_input_mode == EOF_INPUT_CLASSIC)
		{	//If the input method is classic
			if(eof_key_char == '1')
			{
				eof_pen_note.note ^= 1;
				eof_use_key();
			}
			if(eof_key_char == '2')
			{
				eof_pen_note.note ^= 2;
				eof_use_key();
			}
			if(eof_key_char == '3')
			{
				eof_pen_note.note ^= 4;
				eof_use_key();
			}
			if(eof_key_char == '4')
			{
				eof_pen_note.note ^= 8;
				eof_use_key();
			}
			if((eof_key_char == '5') && (numlanes >= 5))
			{
				eof_pen_note.note ^= 16;
				eof_use_key();
			}
			if((eof_key_char == '6') && (numlanes >= 6))
			{	//Only allow use of the 6 key if lane 6 is available
				eof_pen_note.note ^= 32;
				eof_use_key();
			}
		}//If the input method is classic
	}//If neither SHIFT nor CTRL are held and a non vocal track is active

	if((eof_input_mode == EOF_INPUT_CLASSIC) || (eof_input_mode == EOF_INPUT_HOLD))
	{	//If the input method is classic or hold
		if((eof_key_code == KEY_ENTER) && (eof_music_pos - eof_av_delay >= eof_song->beat[0]->pos) && !KEY_EITHER_SHIFT && !KEY_EITHER_CTRL)
		{	//If the user pressed enter and the current seek position is not left of the first beat marker, and neither SHIFT nor CTRL are held
			/* place note with default length if song is paused */
			if(eof_music_paused)
			{
				long notelen = eof_snap.length;	//The default length of the new note
				if(eof_new_note_length_1ms)
				{	//If the new note's length is being overridden as 1ms
					notelen = 1;
				}
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				new_note = eof_track_add_create_note(eof_song, eof_selected_track, eof_pen_note.note, eof_music_pos - eof_av_delay, notelen, eof_note_type, NULL);
				if(new_note)
				{
					if(eof_mark_drums_as_cymbal)
					{	//If the user opted to make all new drum notes cymbals automatically
						eof_mark_new_note_as_cymbal(eof_song,eof_selected_track,eof_get_track_size(eof_song, eof_selected_track) - 1);
					}
					if(eof_mark_drums_as_double_bass)
					{	//If the user opted to make all new expert bass drum notes as double bass automatically
						eof_mark_new_note_as_double_bass(eof_song,eof_selected_track,eof_get_track_size(eof_song, eof_selected_track) - 1);
					}
					if(eof_mark_drums_as_hi_hat)
					{	//If the user opted to make all new yellow drum notes as one of the specialized hi hat types automatically
						eof_mark_new_note_as_special_hi_hat(eof_song,eof_selected_track,eof_get_track_size(eof_song, eof_selected_track) - 1);
					}
					(void) eof_detect_difficulties(eof_song, eof_selected_track);
					eof_track_fixup_notes(eof_song, eof_selected_track, 1);
				}
				eof_use_key();
			}

			/* otherwise allow length to be determined by key holding */
			else
			{
				if(!eof_entering_note_note)
				{
					new_note = eof_track_add_create_note(eof_song, eof_selected_track, eof_pen_note.note, eof_music_pos - eof_av_delay, eof_snap.length, eof_note_type, NULL);
					if(new_note)
					{
						if(eof_mark_drums_as_cymbal)
						{	//If the user opted to make all new drum notes cymbals automatically
							eof_mark_new_note_as_cymbal(eof_song,eof_selected_track,eof_get_track_size(eof_song, eof_selected_track) - 1);
						}
						if(eof_mark_drums_as_double_bass)
						{	//If the user opted to make all new expert bass drum notes as double bass automatically
							eof_mark_new_note_as_double_bass(eof_song,eof_selected_track,eof_get_track_size(eof_song, eof_selected_track) - 1);
						}
						if(eof_mark_drums_as_hi_hat)
						{	//If the user opted to make all new yellow drum notes as one of the specialized hi hat types automatically
							eof_mark_new_note_as_special_hi_hat(eof_song,eof_selected_track,eof_get_track_size(eof_song, eof_selected_track) - 1);
						}
						eof_entering_note_note = new_note;
						eof_entering_note = 1;
						(void) eof_detect_difficulties(eof_song, eof_selected_track);
					}
				}
				else
				{
					eof_entering_note_note->length = (eof_music_pos - eof_av_delay) - eof_entering_note_note->pos - 10;
				}
			}
		}
	}//If the input method is classic or hold

	/* Toggle the second piano roll (SHIFT+Enter) */
	/* Swap the primary and secondary piano rolls (CTRL+Enter) */
	if(eof_key_code == KEY_ENTER)
	{
		if(KEY_EITHER_SHIFT)
		{
			(void) eof_menu_song_toggle_second_piano_roll();
			eof_shift_used = 1;		//Track that the SHIFT key was used
			eof_use_key();
		}
		else if(KEY_EITHER_CTRL)
		{
			(void) eof_menu_song_swap_piano_rolls();
			eof_mix_find_claps();		//Update sound cues
			eof_mix_start_helper();
			eof_use_key();
		}
	}

/* keyboard shortcuts that only apply when the chart is playing (non drum record type input methods) */

	if(!eof_music_paused && (eof_song->track[eof_selected_track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR))
	{	//If the chart is playing and a drum track is not active
		if(eof_input_mode == EOF_INPUT_GUITAR_TAP)
		{	//If the input method is guitar tap
			if(eof_guitar.button[2].held)
			{
				eof_held_1++;
			}
			else
			{
				eof_held_1 = 0;
			}
			if(eof_guitar.button[3].held)
			{
				eof_held_2++;
			}
			else
			{
				eof_held_2 = 0;
			}
			if(eof_guitar.button[4].held)
			{
				eof_held_3++;
			}
			else
			{
				eof_held_3 = 0;
			}
			if(eof_guitar.button[5].held)
			{
				eof_held_4++;
			}
			else
			{
				eof_held_4 = 0;
			}
			if(eof_guitar.button[6].held)
			{
				eof_held_5++;
			}
			else
			{
				eof_held_5 = 0;
			}
			bitmask = 0;
			if(eof_held_1 == 1)
			{
				bitmask = 1;
			}
			else if(eof_held_2 == 1)
			{
				bitmask = 2;
			}
			else if(eof_held_3 == 1)
			{
				bitmask = 4;
			}
			else if(eof_held_4 == 1)
			{
				bitmask = 8;
			}
			else if(eof_held_5 == 1)
			{
				bitmask = 16;
			}

			if(bitmask)
			{
				eof_prepare_undo(EOF_UNDO_TYPE_RECORD);
				if(eof_vocals_selected)
				{
					new_note = eof_track_add_create_note(eof_song, eof_selected_track, eof_vocals_offset, eof_music_pos - eof_av_delay - eof_guitar.delay, 1, 0, "");
				}
				else
				{
					new_note = eof_track_add_create_note(eof_song, eof_selected_track, bitmask, eof_music_pos - eof_av_delay - eof_guitar.delay, 1, eof_note_type, NULL);
					if(new_note)
					{
						if(eof_mark_drums_as_cymbal)
						{	//If the user opted to make all new drum notes cymbals automatically
							eof_mark_new_note_as_cymbal(eof_song,eof_selected_track,eof_get_track_size(eof_song, eof_selected_track) - 1);
						}
						if(eof_mark_drums_as_double_bass)
						{	//If the user opted to make all new expert bass drum notes as double bass automatically
							eof_mark_new_note_as_double_bass(eof_song,eof_selected_track,eof_get_track_size(eof_song, eof_selected_track) - 1);
						}
						if(eof_mark_drums_as_hi_hat)
						{	//If the user opted to make all new yellow drum notes as one of the specialized hi hat types automatically
							eof_mark_new_note_as_special_hi_hat(eof_song,eof_selected_track,eof_get_track_size(eof_song, eof_selected_track) - 1);
						}
						eof_entering_note_note = new_note;
						eof_entering_note = 1;
					}
				}
				if(new_note)
				{
					(void) eof_detect_difficulties(eof_song, eof_selected_track);
					eof_track_sort_notes(eof_song, eof_selected_track);
				}
			}
		}//If the input method is guitar tap
		else if(eof_input_mode == EOF_INPUT_GUITAR_STRUM)
		{	//If the input method is guitar strum
			if(eof_vocals_selected)
			{
				eof_last_snote = eof_snote;
				eof_snote = 0;

				if(eof_guitar.button[2].held || eof_guitar.button[3].held || eof_guitar.button[4].held || eof_guitar.button[5].held || eof_guitar.button[6].held)
				{
					eof_snote = 1;
				}
				if((eof_guitar.button[0].pressed || eof_guitar.button[1].pressed) && eof_snote)
				{
					if(eof_entering_note && eof_entering_note_lyric)
					{
						eof_entering_note_lyric->length = (eof_music_pos - eof_av_delay - eof_guitar.delay) - eof_entering_note_lyric->pos - 10;
					}
					eof_prepare_undo(EOF_UNDO_TYPE_RECORD);
					new_lyric = eof_track_add_create_note(eof_song, eof_selected_track, eof_vocals_offset, eof_music_pos - eof_av_delay - eof_guitar.delay, 1, 0, "");
					if(new_lyric)
					{
						eof_entering_note_lyric = new_lyric;
						eof_entering_note = 1;
						(void) eof_detect_difficulties(eof_song, eof_selected_track);
						eof_track_sort_notes(eof_song, eof_selected_track);
					}
				}
				if(eof_entering_note && eof_entering_note_lyric)
				{
					if(eof_snote != eof_last_snote)
					{
						eof_entering_note = 0;
						eof_entering_note_lyric->length = (eof_music_pos - eof_av_delay - eof_guitar.delay) - eof_entering_note_lyric->pos - 10;
					}
					else
					{
						eof_entering_note_lyric->length = (eof_music_pos - eof_av_delay - eof_guitar.delay) - eof_entering_note_lyric->pos - 10;
					}
					eof_track_fixup_notes(eof_song, eof_selected_track, 1);
				}
			}//if(eof_vocals_selected)
			else
			{	//If a non vocal track is selected
				eof_last_snote = eof_snote;
				eof_snote = 0;

				if(eof_guitar.button[2].held)
				{
					eof_snote |= 1;
				}
				if(eof_guitar.button[3].held)
				{
					eof_snote |= 2;
				}
				if(eof_guitar.button[4].held)
				{
					eof_snote |= 4;
				}
				if(eof_guitar.button[5].held)
				{
					eof_snote |= 8;
				}
				if(eof_guitar.button[6].held)
				{
					eof_snote |= 16;
				}
				if(eof_open_strum_enabled(eof_selected_track) && !eof_snote && (eof_guitar.button[0].held || eof_guitar.button[1].held))
				{	//If the strum is being held up/down with no frets, and open strumming is enabled
					eof_snote = 32;	//The strum note is lane 6
				}
				if(eof_guitar.button[0].pressed || eof_guitar.button[1].pressed)
				{	//If the user strummed
					if(eof_snote)
					{	//If a note is being added from this strum
						if(eof_entering_note && eof_entering_note_note)
						{
							eof_entering_note_note->length = (eof_music_pos - eof_av_delay - eof_guitar.delay) - eof_entering_note_note->pos - 10;
						}
						eof_prepare_undo(EOF_UNDO_TYPE_RECORD);
						new_note = eof_track_add_create_note(eof_song, eof_selected_track, eof_snote, eof_music_pos - eof_av_delay - eof_guitar.delay, 1, eof_note_type, NULL);
						if(new_note)
						{
							if(eof_mark_drums_as_cymbal)
							{	//If the user opted to make all new drum notes cymbals automatically
								eof_mark_new_note_as_cymbal(eof_song,eof_selected_track,eof_get_track_size(eof_song, eof_selected_track) - 1);
							}
							if(eof_mark_drums_as_double_bass)
							{	//If the user opted to make all new expert bass drum notes as double bass automatically
								eof_mark_new_note_as_double_bass(eof_song,eof_selected_track,eof_get_track_size(eof_song, eof_selected_track) - 1);
							}
							if(eof_mark_drums_as_hi_hat)
							{	//If the user opted to make all new yellow drum notes as one of the specialized hi hat types automatically
								eof_mark_new_note_as_special_hi_hat(eof_song,eof_selected_track,eof_get_track_size(eof_song, eof_selected_track) - 1);
							}
							eof_entering_note_note = new_note;
							eof_entering_note = 1;
							(void) eof_detect_difficulties(eof_song, eof_selected_track);
							eof_track_sort_notes(eof_song, eof_selected_track);
						}
					}
				}//If the user strummed
				if(eof_entering_note && eof_entering_note_note)
				{
					eof_entering_note_note->length = (eof_music_pos - eof_av_delay - eof_guitar.delay) - eof_entering_note_note->pos - 10;
					if(eof_snote != eof_last_snote)
					{
						eof_entering_note = 0;
					}
					eof_track_fixup_notes(eof_song, eof_selected_track, 1);
				}
			}//If a non vocal track is selected
		}//If the input method is guitar strum
	}//If the chart is playing and PART DRUMS is not active

/* keyboard shortcuts that apply when the chart is paused and nothing is playing */

	if(eof_music_paused && !eof_music_catalog_playback)
	{	//If the chart is paused and no catalog entries are playing
	/* lower playback speed (;) */
		if(eof_key_char == ';')
		{
			(void) eof_menu_edit_playback_speed_helper_slower();
			eof_use_key();
		}

	/* increase playback speed (') */
		if(eof_key_char == '\'')
		{
			(void) eof_menu_edit_playback_speed_helper_faster();
			eof_use_key();
		}

	/* toggle anchor (A) */
	/* select all (CTRL+A) */
	/* anchor beat (SHIFT+A) */
	/* toggle accent (CTRL+SHIFT+A in a pro guitar track) */
		if(eof_key_char == 'a')
		{
			if(!KEY_EITHER_CTRL)
			{	//If CTRL is not held
				if(KEY_EITHER_SHIFT)
				{	//If SHIFT is held
					eof_shift_used = 1;	//Track that the SHIFT key was used
					(void) eof_menu_beat_anchor();
				}
				else
				{	//If neither CTRL nor SHIFT are held
					(void) eof_menu_beat_toggle_anchor();
				}
			}
			else
			{	//If CTRL is held
				if(KEY_EITHER_SHIFT)
				{	//If both CTRL and SHIFT are held
					eof_shift_used = 1;	//Track that the SHIFT key was used
					(void) eof_menu_note_toggle_accent();
				}
				else
				{	//If only CTRL is held
					(void) eof_menu_edit_select_all();
				}
			}
			eof_use_key();
		}

	/* decrease note length ( [ , SHIFT+[ or CTRL+SHIFT+[ ) */
	/* move notes backward one grid snap position ( CTRL+[ ) */
		if(eof_key_code == KEY_OPENBRACE)
		{
			if(!KEY_EITHER_CTRL || (KEY_EITHER_CTRL && KEY_EITHER_SHIFT))
			{	//If CTRL is not held or if both CTRL and SHIFT are held
				unsigned long reductionvalue = 100;	//Default decrease length when grid snap is disabled
				if(eof_snap_mode == EOF_SNAP_OFF)
				{
					if(KEY_EITHER_SHIFT)
					{
						eof_shift_used = 1;	//Track that the SHIFT key was used
						if(KEY_EITHER_CTRL)
						{	//If both CTRL and SHIFT are held
							reductionvalue = 1;		//1 ms length change
						}
						else
						{	//SHIFT+scroll wheel
							reductionvalue = 10;	//10ms length change
						}
					}
				}
				else
				{
					reductionvalue = 0;	//Will indicate to eof_adjust_note_length() to use the grid snap value
				}
				eof_adjust_note_length(eof_song, eof_selected_track, reductionvalue, -1);	//Decrease selected notes by the appropriate length
			}
			else
			{	//If CTRL is held but SHIFT is not
				char undo_made = 0;

				eof_auto_adjust_sections(eof_song, eof_selected_track, 0, -1, &undo_made);		//Move sections accordingly by one grid snap
				eof_auto_adjust_tech_notes(eof_song, eof_selected_track, 0, -1, &undo_made);	//Move tech notes accordingly by one grid snap
				(void) eof_menu_note_move_by_grid_snap(-1, &undo_made);
				if(eof_song->tags->highlight_unsnapped_notes)
				{	//If the user has enabled the dynamic highlighting of non grid snapped notes
					eof_track_remove_highlighting(eof_song, eof_selected_track, 1);	//Remove existing temporary highlighting from the track
					eof_song_highlight_non_grid_snapped_notes(eof_song, eof_selected_track);	//Re-create the temporary highlighting as appropriate
				}
			}
			eof_use_key();
		}

	/* increase note length ( ] , SHIFT+ ] or CTRL+SHIFT+] ) */
	/* move notes forward one grid snap position ( CTRL+] ) */
		if(eof_key_code == KEY_CLOSEBRACE)
		{
			if(!KEY_EITHER_CTRL || (KEY_EITHER_CTRL && KEY_EITHER_SHIFT))
			{	//If CTRL is not held or if both CTRL and SHIFT are held
				unsigned long increasevalue = 100;	//Default increase length when grid snap is disabled
				if(eof_snap_mode == EOF_SNAP_OFF)
				{
					if(KEY_EITHER_SHIFT)
					{
						eof_shift_used = 1;	//Track that the SHIFT key was used
						if(KEY_EITHER_CTRL)
						{	//If both CTRL and SHIFT are held
							increasevalue = 1;		//1 ms length change
						}
						else
						{	//SHIFT+scroll wheel
							increasevalue = 10;	//10ms length change
						}
					}
				}
				else
				{
					increasevalue = 0;	//Will indicate to eof_adjust_note_length() to use the grid snap value
				}
				eof_adjust_note_length(eof_song, eof_selected_track, increasevalue, 1);	//Increase selected notes by the appropriate length
			}
			else
			{	//If CTRL is held but SHIFT is not
				char undo_made = 0;

				eof_auto_adjust_sections(eof_song, eof_selected_track, 0, 1, &undo_made);	//Move sections accordingly by one grid snap
				eof_auto_adjust_tech_notes(eof_song, eof_selected_track, 0, 1, &undo_made);	//Move tech notes accordingly by one grid snap
				(void) eof_menu_note_move_by_grid_snap(1, &undo_made);
				if(eof_song->tags->highlight_unsnapped_notes)
				{	//If the user has enabled the dynamic highlighting of non grid snapped notes
					eof_track_remove_highlighting(eof_song, eof_selected_track, 1);	//Remove existing temporary highlighting from the track
					eof_song_highlight_non_grid_snapped_notes(eof_song, eof_selected_track);	//Re-create the highlighting as appropriate
				}
			}
			eof_use_key();
		}

	/* toggle tapping status (CTRL+T in a pro guitar track) */
	/* toggle crazy status (T) */
	/* add tone change (CTRL+SHIFT+T in a pro guitar track) */
		if((eof_key_char == 't') && !KEY_EITHER_ALT)
		{
			if(KEY_EITHER_CTRL && !KEY_EITHER_SHIFT)
			{	//If CTRL is held but SHIFT is not
				if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
				{	//If a pro guitar track is active
					(void) eof_menu_note_toggle_tapping();
					eof_use_key();
				}
			}
			else if(!KEY_EITHER_SHIFT && !KEY_EITHER_CTRL)
			{	//If neither SHIFT nor CTRL are held
				(void) eof_menu_note_toggle_crazy();
				eof_use_key();
			}
			else if(KEY_EITHER_SHIFT && KEY_EITHER_CTRL)
			{	//If both SHIFT and CTRL are held
				if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
				{	//If a pro guitar track is active
					(void) eof_track_rs_tone_change_add();
					eof_shift_used = 1;	//Track that the SHIFT key was used
					eof_use_key();
				}
			}
		}

	/* Mark tremolo (CTRL+SHIFT+O) */
	if(KEY_EITHER_CTRL && KEY_EITHER_SHIFT && (eof_key_char == 'o'))
	{
		(void) eof_menu_tremolo_mark();
		eof_shift_used = 1;	//Track that the SHIFT key was used
		eof_use_key();
	}

	/* select like (CTRL+L) */
	/* precise select like (SHIFT+L) */
	/* edit lyric (L in PART VOCALS) */
	/* set slide end fret (CTRL+SHIFT+L in a pro guitar track) */
		if(eof_key_char == 'l')
		{
			if(KEY_EITHER_CTRL && !KEY_EITHER_SHIFT)
			{	//CTRL is held but SHIFT is not
				(void) eof_menu_edit_select_like();
				eof_use_key();
			}
			else if(KEY_EITHER_SHIFT && !KEY_EITHER_CTRL)
			{	//SHIFT is held but CTRL is not
				eof_shift_used = 1;	//Track that the SHIFT key was used
				(void) eof_menu_edit_precise_select_like();
				eof_use_key();
			}
			else if(eof_vocals_selected && (eof_selection.track == EOF_TRACK_VOCALS) && (eof_selection.current < eof_song->vocal_track[tracknum]->lyrics))
			{	//If PART VOCALS is active, and one of its lyrics is the current selected lyric
				if(!KEY_EITHER_SHIFT && !KEY_EITHER_CTRL)
				{	//Neither SHIFT nor CTRL are held
					(void) eof_edit_lyric_dialog();
					eof_use_key();
				}
			}
			else if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
			{	//If a pro guitar track is active
				if(KEY_EITHER_SHIFT && KEY_EITHER_CTRL)
				{	//Both SHIFT and CTRL are held
					eof_shift_used = 1;	//Track that the SHIFT key was used
					(void) eof_pro_guitar_note_slide_end_fret_save();
					eof_use_key();
				}
			}
		}

	/* toggle freestyle (F in a vocal track) */
		if((eof_key_char == 'f') && !KEY_EITHER_CTRL && eof_vocals_selected)
		{
			(void) eof_menu_toggle_freestyle();
			eof_use_key();
		}

	/* deselect all (CTRL+D) */
	/* double BPM (CTRL+SHIFT+D) */
		if(KEY_EITHER_CTRL && (eof_key_char == 'd'))
		{
			if(!KEY_EITHER_SHIFT)
			{	//If CTRL is held but SHIFT is not
				(void) eof_menu_edit_deselect_all();
				eof_use_key();
			}
			else
			{	//Both CTRL and SHIFT are held
				eof_shift_used = 1;	//Track that the SHIFT key was used
				(void) eof_menu_beat_double_tempo();
				eof_use_key();
			}
		}

	/* cycle HOPO status (H in a legacy track) */
	/* toggle hammer on status (H in a pro guitar track) */
	/* toggle harmonic (CTRL+H in a pro guitar track) */
	/* toggle pinch harmonic (SHIFT+H in a pro guitar track) */
	/* mark handshape phrase (CTRL+SHIFT+H in a pro guitar track) */
		if((eof_key_char == 'h') && !KEY_EITHER_ALT)
		{
			if(KEY_EITHER_CTRL)
			{	//If CTRL is held
				if(KEY_EITHER_SHIFT)
				{	//If SHIFT is also held
					(void) eof_menu_handshape_mark();
					eof_shift_used = 1;	//Track that the SHIFT key was used
				}
				else
				{	//Only CTRL is held
					if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
					{	//Toggle harmonic
						(void) eof_menu_note_toggle_harmonic();
					}
				}
			}
			else
			{	//If CTRL is not held
				if(KEY_EITHER_SHIFT)
				{	//If SHIFT is held
					eof_shift_used = 1;	//Track that the SHIFT key was used
					(void) eof_menu_note_toggle_pinch_harmonic();
				}
				else
				{	//SHIFT is not held
					if(eof_song->track[eof_selected_track]->track_format == EOF_LEGACY_TRACK_FORMAT)
					{	//Cycle HO/PO
						(void) eof_menu_hopo_cycle();
					}
					else if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
					{	//Toggle HO
						(void) eof_menu_pro_guitar_toggle_hammer_on();
					}
				}
			}
			eof_use_key();
		}

	/* toggle unpitched slide (CTRL+U in a pro guitar track) */
		if((eof_key_char == 'u') && KEY_EITHER_CTRL)
		{
			if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
			{
				(void) eof_pro_guitar_note_define_unpitched_slide();
			}
			eof_use_key();
		}

	/* toggle pull off status (P in a pro guitar track) */
	/* place Rocksmith phrase (SHIFT+P in a pro guitar track) */
	/* toggle pop status (CTRL+SHIFT+P in a pro guitar track) */
		if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If a pro guitar track is active
			if(eof_key_char == 'p')
			{
				if(!KEY_EITHER_CTRL && !KEY_EITHER_ALT && !KEY_EITHER_SHIFT)
				{	//If none of CTRL, ALT or SHIFT are held
					(void) eof_menu_pro_guitar_toggle_pull_off();
					eof_use_key();
				}
				else if(!KEY_EITHER_CTRL && !KEY_EITHER_ALT && KEY_EITHER_SHIFT)
				{	//If SHIFT is held, but neither CTRL nor ALT are
					(void) eof_rocksmith_phrase_dialog_add();
					eof_use_key();
					eof_shift_used = 1;	//Track that the SHIFT key was used
				}
				else if(KEY_EITHER_CTRL && !KEY_EITHER_ALT && KEY_EITHER_SHIFT)
				{	//If both CTRL and SHIFT are held, but ALT is not
					(void) eof_menu_note_toggle_pop();
					eof_use_key();
					eof_shift_used = 1;	//Track that the SHIFT key was used
				}
			}
		}

	/* toggle open hi hat (SHIFT+O) */
		if((eof_key_char == 'o') && !KEY_EITHER_CTRL && KEY_EITHER_SHIFT)
		{
			eof_shift_used = 1;	//Track that the SHIFT key was used
			(void) eof_menu_note_toggle_hi_hat_open();
			eof_use_key();
		}

	/* toggle pedal controlled hi hat (SHIFT+P in the PS drum track) */
		if((eof_key_char == 'p') && !KEY_EITHER_CTRL && KEY_EITHER_SHIFT && (eof_selected_track == EOF_TRACK_DRUM_PS))
		{
			eof_shift_used = 1;	//Track that the SHIFT key was used
			(void) eof_menu_note_toggle_hi_hat_pedal();
			eof_use_key();
		}

	/* toggle rim shot (SHIFT+R) */
		if((eof_key_char == 'r') && !KEY_EITHER_CTRL && KEY_EITHER_SHIFT)
		{
			eof_shift_used = 1;	//Track that the SHIFT key was used
			(void) eof_menu_note_toggle_rimshot();
			eof_use_key();
		}

	/* mark/remark slider (SHIFT+S, in a five lane guitar/bass track) */
	/* place Rocksmith section (SHIFT+S, in a pro guitar track) */
	/* toggle sizzle hi hat (SHIFT+S, in the PS drum track) */
	/* split lyric (SHIFT+S in PART VOCALS) */
		if((eof_key_char == 's') && !KEY_EITHER_CTRL && KEY_EITHER_SHIFT)
		{	//S and SHIFT are held, but CTRL is not
			if((eof_song->track[eof_selected_track]->track_behavior == EOF_GUITAR_TRACK_BEHAVIOR) && (eof_song->track[eof_selected_track]->track_format == EOF_LEGACY_TRACK_FORMAT))
			{	//If this is a 5 lane guitar/bass track
				eof_shift_used = 1;	//Track that the SHIFT key was used
				(void) eof_menu_slider_mark();
				eof_use_key();
			}
			else if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
			{	//If this is a pro guitar track
				eof_shift_used = 1;	//Track that the SHIFT key was used
				(void) eof_rocksmith_section_dialog_add();
				eof_use_key();
			}
			else if(eof_selected_track == EOF_TRACK_DRUM_PS)
			{	//If this is the Phase Shift drum track
				eof_shift_used = 1;	//Track that the SHIFT key was used
				(void) eof_menu_note_toggle_hi_hat_sizzle();
				eof_use_key();
			}
			else if(eof_vocals_selected && (eof_selection.track == EOF_TRACK_VOCALS) && (eof_selection.current < eof_song->vocal_track[tracknum]->lyrics))
			{	//If PART VOCALS is active, and one of its lyrics is the current selected lyric
				eof_shift_used = 1;	//Track that the SHIFT key was used
				(void) eof_menu_split_lyric();
				eof_use_key();
			}
		}

		if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If the active track is a pro guitar track
	/* edit pro guitar note (N in a pro guitar track) */
	/* toggle linknext status (SHIFT+N in a pro guitar track) */
			if((eof_key_char == 'n') && !KEY_EITHER_CTRL)
			{
				if(!KEY_EITHER_SHIFT)
				{	//If SHIFT is not held
					(void) eof_menu_note_edit_pro_guitar_note();
					eof_use_key();
				}
				else
				{	//If SHIFT is held
					eof_shift_used = 1;	//Track that the SHIFT key was used
					(void) eof_menu_note_toggle_linknext();
					eof_use_key();
				}
			}

	/* edit pro guitar fret/finger values (F in a pro guitar track) */
			if((eof_key_char == 'f') && !KEY_EITHER_CTRL && !KEY_EITHER_SHIFT)
			{
				(void) eof_menu_note_edit_pro_guitar_note_frets_fingers_menu();
				eof_use_key();
			}

	/* set fret hand position (SHIFT+F in a pro guitar track) */
			if((eof_key_char == 'f') && !KEY_EITHER_CTRL && KEY_EITHER_SHIFT)
			{
				eof_shift_used = 1;	//Track that the SHIFT key was used
				(void) eof_track_pro_guitar_set_fret_hand_position();
				eof_use_key();
			}

	/* list fret hand positions (CTRL+SHIFT+F in a pro guitar track) */
			if((eof_key_char == 'f') && KEY_EITHER_CTRL && KEY_EITHER_SHIFT)
			{
				eof_shift_used = 1;	//Track that the SHIFT key was used
				(void) eof_track_fret_hand_positions();
				eof_use_key();
			}

	/* toggle slap status (CTRL+SHIFT+S in a pro guitar track) */
			if((eof_key_char == 's') && KEY_EITHER_CTRL && !KEY_EITHER_ALT && KEY_EITHER_SHIFT)
			{	//If both CTRL and SHIFT are held, but ALT is not
				(void) eof_menu_note_toggle_slap();
				eof_use_key();
				eof_shift_used = 1;	//Track that the SHIFT key was used
			}

	/* set pro guitar fret values (CTRL+#, CTRL+Fn #, CTRL+X, CTRL+~, CTRL++, CTRL+-) */
	/* toggle pro guitar ghost status (CTRL+G) */
	/* Mark/Remark arpeggio (CTRL+SHIFT+G in a pro guitar track) */
			if(KEY_EITHER_CTRL && !KEY_EITHER_SHIFT)
			{	//If CTRL is held but SHIFT is not
				//CTRL+# or CTRL+Fn # in a pro guitar track sets the fret values of selected notes
				//CTRL+~ sets fret values to 0 and CTRL+X sets fret values to (muted)
				//CTRL+G sets the used frets of selected notes to be ghosted
				if(eof_key_code == KEY_TILDE)
				{
					eof_set_pro_guitar_fret_number(0,0);
					eof_use_key();
				}
				if(eof_key_char == '1')
				{
					eof_set_pro_guitar_fret_number(0,1);
					eof_use_key();
				}
				else if(eof_key_char == '2')
				{
					eof_set_pro_guitar_fret_number(0,2);
					eof_use_key();
				}
				else if(eof_key_char == '3')
				{
					eof_set_pro_guitar_fret_number(0,3);
					eof_use_key();
				}
				else if(eof_key_char == '4')
				{
					eof_set_pro_guitar_fret_number(0,4);
					eof_use_key();
				}
				else if(eof_key_char == '5')
				{
					eof_set_pro_guitar_fret_number(0,5);
					eof_use_key();
				}
				else if(eof_key_char == '6')
				{
					eof_set_pro_guitar_fret_number(0,6);
					eof_use_key();
				}
				else if(eof_key_char == '7')
				{
					eof_set_pro_guitar_fret_number(0,7);
					eof_use_key();
				}
				else if(eof_key_char == '8')
				{
					eof_set_pro_guitar_fret_number(0,8);
					eof_use_key();
				}
				else if(eof_key_char == '9')
				{
					eof_set_pro_guitar_fret_number(0,9);
					eof_use_key();
				}
				else if(eof_key_char == '0')
				{
					eof_set_pro_guitar_fret_number(0,10);
					eof_use_key();
				}
				else if(eof_key_code == KEY_F1)
				{
					eof_set_pro_guitar_fret_number(0,11);
					eof_use_key();
				}
				else if(eof_key_code == KEY_F2)
				{
					eof_set_pro_guitar_fret_number(0,12);
					eof_use_key();
				}
				else if(eof_key_code == KEY_F3)
				{
					eof_set_pro_guitar_fret_number(0,13);
					eof_use_key();
				}
				else if(eof_key_code == KEY_F4)
				{
					eof_set_pro_guitar_fret_number(0,14);
					eof_use_key();
				}
				else if(eof_key_code == KEY_F5)
				{
					eof_set_pro_guitar_fret_number(0,15);
					eof_use_key();
				}
				else if(eof_key_code == KEY_F6)
				{
					eof_set_pro_guitar_fret_number(0,16);
					eof_use_key();
				}
				else if(eof_key_code == KEY_F7)
				{
					eof_set_pro_guitar_fret_number(0,17);
					eof_use_key();
				}
				else if(eof_key_code == KEY_F8)
				{
					eof_set_pro_guitar_fret_number(0,18);
					eof_use_key();
				}
				else if(eof_key_code == KEY_F9)
				{
					eof_set_pro_guitar_fret_number(0,19);
					eof_use_key();
				}
				else if(eof_key_code == KEY_F10)
				{
					eof_set_pro_guitar_fret_number(0,20);
					eof_use_key();
				}
				else if(eof_key_code == KEY_F11)
				{
					eof_set_pro_guitar_fret_number(0,21);
					eof_use_key();
				}
				else if(eof_key_code == KEY_F12)
				{
					eof_set_pro_guitar_fret_number(0,22);
					eof_use_key();
				}
				else if(eof_key_char == 'x')
				{
					eof_set_pro_guitar_fret_number(0,255);	//CTRL+X sets frets to (muted)
					eof_use_key();
				}
				else if(eof_key_code == KEY_MINUS)			//Use the scan code because CTRL+- can't be reliably detected via ASCII value
				{
					eof_set_pro_guitar_fret_number(2,0);	//Decrement fret value
					eof_use_key();
				}
				else if(eof_key_code == KEY_EQUALS)			//Use the scan code because CTRL+= can't be reliably detected via ASCII value
				{
					eof_set_pro_guitar_fret_number(1,0);	//Increment fret value
					eof_use_key();
				}
				else if(eof_key_char == 'g')
				{
					(void) eof_menu_note_toggle_ghost();
					eof_use_key();
				}
			}//If CTRL is held but SHIFT is not
			else if(KEY_EITHER_SHIFT && !KEY_EITHER_CTRL)
			{	//If SHIFT is held, but CTRL is not
	/* set fret value shortcut bitmask (SHIFT+Numpad #) */
	/* toggle string mute status (SHIFT+X) */
				if(eof_key_code == KEY_0_PAD)
				{
					eof_shift_used = 1;	//Track that the SHIFT key was used
					eof_pro_guitar_fret_bitmask = 0;	//Disable all strings
					eof_use_key();
				}
				else if(eof_key_code == KEY_1_PAD)
				{
					eof_shift_used = 1;	//Track that the SHIFT key was used
					eof_pro_guitar_fret_bitmask ^= 32;	//Toggle string 1 (high E)
					eof_use_key();
				}
				else if(eof_key_code == KEY_2_PAD)
				{
					eof_shift_used = 1;	//Track that the SHIFT key was used
					eof_pro_guitar_fret_bitmask ^= 16;	//Toggle string 2
					eof_use_key();
				}
				else if(eof_key_code == KEY_3_PAD)
				{
					eof_shift_used = 1;	//Track that the SHIFT key was used
					eof_pro_guitar_fret_bitmask ^= 8;	//Toggle string 3
					eof_use_key();
				}
				else if(eof_key_code == KEY_4_PAD)
				{
					eof_shift_used = 1;	//Track that the SHIFT key was used
					eof_pro_guitar_fret_bitmask ^= 4;	//Toggle string 4
					eof_use_key();
				}
				else if(eof_key_code == KEY_5_PAD)
				{
					eof_shift_used = 1;	//Track that the SHIFT key was used
					eof_pro_guitar_fret_bitmask ^= 2;	//Toggle string 5
					eof_use_key();
				}
				else if(eof_key_code == KEY_6_PAD)
				{
					eof_shift_used = 1;	//Track that the SHIFT key was used
					eof_pro_guitar_fret_bitmask ^= 1;	//Toggle string 6 (low e)
					eof_use_key();
				}
				else if(eof_key_code == KEY_7_PAD)
				{
					eof_shift_used = 1;	//Track that the SHIFT key was used
					eof_pro_guitar_fret_bitmask = 63;	//Enable all strings
					eof_use_key();
				}
				else if(eof_key_code == KEY_8_PAD)
				{
					eof_shift_used = 1;	//Track that the SHIFT key was used
					eof_pro_guitar_fret_bitmask = 63;	//Enable all strings
					eof_use_key();
				}
				else if(eof_key_code == KEY_9_PAD)
				{
					eof_shift_used = 1;	//Track that the SHIFT key was used
					eof_pro_guitar_fret_bitmask = 63;	//Enable all strings
					eof_use_key();
				}
				else if(eof_key_char == 'x')
				{
					eof_shift_used = 1;	//Track that the SHIFT key was used
					(void)eof_menu_pro_guitar_toggle_string_mute();
					eof_use_key();
				}
			}//If SHIFT is held, but CTRL is not
			else if(KEY_EITHER_CTRL && KEY_EITHER_SHIFT)
			{	//If both CTRL and SHIFT are held
				if(eof_key_char == 'g')
				{
					eof_shift_used = 1;	//Track that the SHIFT key was used
					(void)eof_menu_arpeggio_mark();
					eof_use_key();
				}
			}
		}//If the active track is a pro guitar track

	/* halve BPM (CTRL+SHIFT+X) */
		if(KEY_EITHER_CTRL && KEY_EITHER_SHIFT && (eof_key_char == 'x'))
		{
			eof_shift_used = 1;	//Track that the SHIFT key was used
			(void) eof_menu_beat_halve_tempo();
			eof_use_key();
		}

	/* set active difficulty number (CTRL+SHIFT+~, CTRL+SHIFT+#, CTRL+SHIFT+Fn #) */
		if(KEY_EITHER_CTRL && KEY_EITHER_SHIFT)
		{	//If both CTRL and SHIFT are held
			if(eof_key_code == KEY_TILDE)
			{
				(void) eof_set_active_difficulty(0);
				eof_mix_find_claps();		//Update sound cues
				eof_shift_used = 1;	//Track that the SHIFT key was used
				eof_use_key();
			}
			if(eof_key_char == '1')
			{
				(void) eof_set_active_difficulty(1);
				eof_mix_find_claps();		//Update sound cues
				eof_shift_used = 1;	//Track that the SHIFT key was used
				eof_use_key();
			}
			else if(eof_key_char == '2')
			{
				(void) eof_set_active_difficulty(2);
				eof_mix_find_claps();		//Update sound cues
				eof_shift_used = 1;	//Track that the SHIFT key was used
				eof_use_key();
			}
			else if(eof_key_char == '3')
			{
				(void) eof_set_active_difficulty(3);
				eof_mix_find_claps();		//Update sound cues
				eof_shift_used = 1;	//Track that the SHIFT key was used
				eof_use_key();
			}
			else if(eof_key_char == '4')
			{
				(void) eof_set_active_difficulty(4);
				eof_mix_find_claps();		//Update sound cues
				eof_shift_used = 1;	//Track that the SHIFT key was used
				eof_use_key();
			}
			else if(eof_key_char == '5')
			{
				(void) eof_set_active_difficulty(5);
				eof_mix_find_claps();		//Update sound cues
				eof_shift_used = 1;	//Track that the SHIFT key was used
				eof_use_key();
			}
			else if(eof_key_char == '6')
			{
				(void) eof_set_active_difficulty(6);
				eof_mix_find_claps();		//Update sound cues
				eof_shift_used = 1;	//Track that the SHIFT key was used
				eof_use_key();
			}
			else if(eof_key_char == '7')
			{
				(void) eof_set_active_difficulty(7);
				eof_mix_find_claps();		//Update sound cues
				eof_shift_used = 1;	//Track that the SHIFT key was used
				eof_use_key();
			}
			else if(eof_key_char == '8')
			{
				(void) eof_set_active_difficulty(8);
				eof_mix_find_claps();		//Update sound cues
				eof_shift_used = 1;	//Track that the SHIFT key was used
				eof_use_key();
			}
			else if(eof_key_char == '9')
			{
				(void) eof_set_active_difficulty(9);
				eof_mix_find_claps();		//Update sound cues
				eof_shift_used = 1;	//Track that the SHIFT key was used
				eof_use_key();
			}
			else if(eof_key_char == '0')
			{
				(void) eof_set_active_difficulty(10);
				eof_mix_find_claps();		//Update sound cues
				eof_shift_used = 1;	//Track that the SHIFT key was used
				eof_use_key();
			}
			else if(eof_key_code == KEY_F1)
			{
				(void) eof_set_active_difficulty(11);
				eof_mix_find_claps();		//Update sound cues
				eof_shift_used = 1;	//Track that the SHIFT key was used
				eof_use_key();
			}
			else if(eof_key_code == KEY_F2)
			{
				(void) eof_set_active_difficulty(12);
				eof_mix_find_claps();		//Update sound cues
				eof_shift_used = 1;	//Track that the SHIFT key was used
				eof_use_key();
			}
			else if(eof_key_code == KEY_F3)
			{
				(void) eof_set_active_difficulty(13);
				eof_mix_find_claps();		//Update sound cues
				eof_shift_used = 1;	//Track that the SHIFT key was used
				eof_use_key();
			}
			else if(eof_key_code == KEY_F4)
			{
				(void) eof_set_active_difficulty(14);
				eof_mix_find_claps();		//Update sound cues
				eof_shift_used = 1;	//Track that the SHIFT key was used
				eof_use_key();
			}
			else if(eof_key_code == KEY_F5)
			{
				(void) eof_set_active_difficulty(15);
				eof_mix_find_claps();		//Update sound cues
				eof_shift_used = 1;	//Track that the SHIFT key was used
				eof_use_key();
			}
			else if(eof_key_code == KEY_F6)
			{
				(void) eof_set_active_difficulty(16);
				eof_mix_find_claps();		//Update sound cues
				eof_shift_used = 1;	//Track that the SHIFT key was used
				eof_use_key();
			}
			else if(eof_key_code == KEY_F7)
			{
				(void) eof_set_active_difficulty(17);
				eof_mix_find_claps();		//Update sound cues
				eof_shift_used = 1;	//Track that the SHIFT key was used
				eof_use_key();
			}
			else if(eof_key_code == KEY_F8)
			{
				(void) eof_set_active_difficulty(18);
				eof_mix_find_claps();		//Update sound cues
				eof_shift_used = 1;	//Track that the SHIFT key was used
				eof_use_key();
			}
			else if(eof_key_code == KEY_F9)
			{
				(void) eof_set_active_difficulty(19);
				eof_mix_find_claps();		//Update sound cues
				eof_shift_used = 1;	//Track that the SHIFT key was used
				eof_use_key();
			}
			else if(eof_key_code == KEY_F10)
			{
				(void) eof_set_active_difficulty(20);
				eof_mix_find_claps();		//Update sound cues
				eof_shift_used = 1;	//Track that the SHIFT key was used
				eof_use_key();
			}
			else if(eof_key_code == KEY_F11)
			{
				(void) eof_set_active_difficulty(21);
				eof_mix_find_claps();		//Update sound cues
				eof_shift_used = 1;	//Track that the SHIFT key was used
				eof_use_key();
			}
			else if(eof_key_code == KEY_F12)
			{
				(void) eof_set_active_difficulty(22);
				eof_mix_find_claps();		//Update sound cues
				eof_shift_used = 1;	//Track that the SHIFT key was used
				eof_use_key();
			}
		}//If both CTRL and SHIFT are held

	/* set mini piano octave focus (SHIFT+# in a vocal track) */
	/* toggle lane (SHIFT+# in a non vocal track) */
		if(KEY_EITHER_SHIFT && !KEY_EITHER_CTRL)
		{	//If SHIFT is held down and CTRL is not
			if(eof_vocals_selected)
			{	//SHIFT+# in a vocal track sets the octave view
				if(eof_key_char == '1')	//Change mini piano focus to first usable octave
				{
					eof_shift_used = 1;	//Track that the SHIFT key was used
					eof_vocals_offset = MINPITCH;
					eof_use_key();
				}
				else if(eof_key_char == '2')	//Change mini piano focus to second usable octave
				{
					eof_shift_used = 1;	//Track that the SHIFT key was used
					eof_vocals_offset = MINPITCH+12;
					eof_use_key();
				}
				else if(eof_key_char == '3')	//Change mini piano focus to third usable octave
				{
					eof_shift_used = 1;	//Track that the SHIFT key was used
					eof_vocals_offset = MINPITCH+24;
					eof_use_key();
				}
				else if(eof_key_char == '4')	//Change mini piano focus to fourth usable octave
				{
					eof_shift_used = 1;	//Track that the SHIFT key was used
					eof_vocals_offset = MINPITCH+36;
					eof_use_key();
				}
			}
			else
			{	//In other tracks, SHIFT+# toggles lanes on/off
				if(eof_key_char == '1')
				{
					eof_shift_used = 1;	//Track that the SHIFT key was used
					(void) eof_menu_note_toggle_green();
					eof_use_key();
				}
				else if(eof_key_char == '2')
				{
					eof_shift_used = 1;	//Track that the SHIFT key was used
					(void) eof_menu_note_toggle_red();
					eof_use_key();
				}
				else if(eof_key_char == '3')
				{
					eof_shift_used = 1;	//Track that the SHIFT key was used
					(void) eof_menu_note_toggle_yellow();
					eof_use_key();
				}
				else if(eof_key_char == '4')
				{
					eof_shift_used = 1;	//Track that the SHIFT key was used
					(void) eof_menu_note_toggle_blue();
					eof_use_key();
				}
				else if((eof_key_char == '5') && (numlanes >= 5))
				{
					eof_shift_used = 1;	//Track that the SHIFT key was used
					(void) eof_menu_note_toggle_purple();
					eof_use_key();
				}
				else if((eof_key_char == '6') && (numlanes >= 6))
				{	//Only allow use of the 6 key if lane 6 is available
					eof_shift_used = 1;	//Track that the SHIFT key was used
					(void) eof_menu_note_toggle_orange();
					eof_use_key();
				}
			}

	/* Redraw display (SHIFT+F5) */
#ifdef ALLEGRO_WINDOWS
			if(eof_key_code == KEY_F5)
			{
				eof_shift_used = 1;	//Track that the SHIFT key was used
				(void) eof_redraw_display();
				eof_use_key();
			}
#endif

			if(eof_key_code == KEY_F11)
			{	//Cycle the "Top of 2D pane shows" preference
				eof_shift_used = 1;	//Track that the SHIFT key was used
				if(eof_2d_render_top_option == 5)
				{	//If chord names are being displayed
					eof_2d_render_top_option = 7;	//Change it to hand positions
				}
				else if(eof_2d_render_top_option == 7)
				{	//If hand positions are being displayed
					eof_2d_render_top_option = 9;	//Change it to RS sections + phrases
				}
				else
				{	//Either the last preference in the sequence is being displayed, or none of them are
					//Reset the sequence by setting the first applicable preference option
					if(eof_top_of_2d_pane_cycle_count_2)
					{	//If user wants to skip displaying chord names via this shortcut
						eof_2d_render_top_option = 7;	//Change it to hand positions
					}
					else
					{
						eof_2d_render_top_option = 5;	//Change it to chord names
					}
				}
			}
		}//If SHIFT is held down and CTRL is not

		if(!KEY_EITHER_CTRL && ((eof_input_mode == EOF_INPUT_REX) || (eof_input_mode == EOF_INPUT_FEEDBACK)))
		{	//If CTRL is not held down and the input method is rex mundi or Feedback
			eof_hover_piece = -1;
			if((eof_input_mode == EOF_INPUT_FEEDBACK) || ((eof_scaled_mouse_y >= eof_window_editor->y + 25 + EOF_EDITOR_RENDER_OFFSET) && (eof_scaled_mouse_y < eof_window_editor->y + eof_screen_layout.fretboard_h + EOF_EDITOR_RENDER_OFFSET)))
			{	//If the mouse is in the fretboard area (or Feedback input method is in use)
				if(KEY_EITHER_SHIFT)
				{
					eof_snap.length = 1;
				}
				if(!eof_vocals_selected)
				{	//If a vocal track is not active
					bitmask = 0;
					if((eof_key_char == '1') && !KEY_EITHER_SHIFT)
					{
						bitmask = 1;
						eof_use_key();
					}
					else if((eof_key_char == '2') && !KEY_EITHER_SHIFT)
					{
						bitmask = 2;
						eof_use_key();
					}
					else if((eof_key_char == '3') && !KEY_EITHER_SHIFT)
					{
						bitmask = 4;
						eof_use_key();
					}
					else if((eof_key_char == '4') && !KEY_EITHER_SHIFT)
					{
						bitmask = 8;
						eof_use_key();
					}
					else if((eof_key_char == '5') && (numlanes >= 5) && !KEY_EITHER_SHIFT)
					{
						bitmask = 16;
						eof_use_key();
					}
					else if((eof_key_char == '6') && (numlanes >= 6) && !KEY_EITHER_SHIFT)
					{	//Only allow use of the 6 key if lane 6 is available
						bitmask = 32;
						eof_use_key();
					}

					if(bitmask)
					{	//If user has pressed any key from 1 through 6
						int effective_hover_note = eof_hover_note;	//By default, the effective hover note is based on the mouse position

						eof_selection.range_pos_1 = 0;
						eof_selection.range_pos_2 = 0;
						eof_prepare_undo(EOF_UNDO_TYPE_NOTE_SEL);
						if(eof_input_mode == EOF_INPUT_FEEDBACK)
						{	//If Feedback input mode is in effect, it is based on the seek position instead
							effective_hover_note = eof_seek_hover_note;
						}
						if(effective_hover_note >= 0)
						{	//If the user edited an existing note (editing hover note not allowed in Feedback input mode)
							if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
							{	//If legacy view is in effect, alter the note's legacy bitmask
								eof_song->pro_guitar_track[tracknum]->note[effective_hover_note]->legacymask ^= bitmask;
							}
							else
							{	//Otherwise alter the note's normal bitmask
								note = eof_get_note_note(eof_song, eof_selected_track, effective_hover_note);
								note ^= bitmask;
								eof_set_note_note(eof_song, eof_selected_track, effective_hover_note, note);
							}
							if(eof_mark_drums_as_cymbal)
							{	//If the user opted to make all new drum notes cymbals automatically
								eof_mark_edited_note_as_cymbal(eof_song,eof_selected_track,effective_hover_note,bitmask);	//Only apply this status to a new drum gem that was added
							}
							if(eof_mark_drums_as_double_bass)
							{	//If the user opted to make all new expert bass drum notes as double bass automatically
								eof_mark_edited_note_as_double_bass(eof_song,eof_selected_track,effective_hover_note,bitmask);	//Only apply this status to a new drum gem that was added
							}
							if(eof_mark_drums_as_hi_hat)
							{	//If the user opted to make all new yellow drum notes as one of the specialized hi hat types automatically
								eof_mark_edited_note_as_special_hi_hat(eof_song,eof_selected_track,effective_hover_note,bitmask);	//Only apply this status to a new drum gem that was added
							}
							eof_selection.current = effective_hover_note;
							if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
							{	//If legacy view is in effect, check the note's legacy bitmask
								note = eof_song->pro_guitar_track[tracknum]->note[effective_hover_note]->legacymask;
							}
							else
							{	//Otherwise check the note's normal bitmask and delete the note if necessary
								note = eof_get_note_note(eof_song, eof_selected_track, effective_hover_note);
							}
							if(note == 0)
							{	//If the note just had all lanes cleared, delete the note
								eof_track_delete_note(eof_song, eof_selected_track, effective_hover_note);
								eof_selection.multi[eof_selection.current] = 0;
								eof_selection.current = EOF_MAX_NOTES - 1;
								eof_track_sort_notes(eof_song, eof_selected_track);
								eof_track_fixup_notes(eof_song, eof_selected_track, 1);
								eof_determine_phrase_status(eof_song, eof_selected_track);
								(void) eof_detect_difficulties(eof_song, eof_selected_track);
							}

							if(eof_selection.current != EOF_MAX_NOTES - 1)
							{	//If a new gem was placed (toggle gem didn't delete the affected note)
								if(!eof_add_new_notes_to_selection)
								{	//If the user didn't opt to prevent clearing the note selection when adding gems
									memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
								}
								eof_selection.track = eof_selected_track;
								eof_selection.multi[eof_selection.current] = 1;
								eof_selection.current_pos = eof_get_note_pos(eof_song, eof_selected_track, eof_selection.current);
								eof_selection.range_pos_1 = eof_selection.current_pos;
								eof_selection.range_pos_2 = eof_selection.current_pos;
							}
						}
						else
						{	//If the user created a new note
							unsigned long targetpos;
							long notelen = eof_snap.length;	//The default length of the new note
							if(eof_new_note_length_1ms)
							{	//If the new note's length is being overridden as 1ms
								notelen = 1;
							}
							if(eof_input_mode == EOF_INPUT_FEEDBACK)
							{	//If Feedback input mode is in use, insert a single gem at the seek position
								targetpos = eof_music_pos - eof_av_delay;
								eof_pen_note.note = bitmask;
							}
							else
							{	//Otherwise insert a note (based on the pen note) at the mouse's position
								targetpos = eof_pen_note.pos;
								eof_pen_note.note ^= bitmask;
							}
							new_note = eof_track_add_create_note(eof_song, eof_selected_track, eof_pen_note.note, targetpos, notelen, eof_note_type, NULL);
							if(new_note)
							{
								if(eof_mark_drums_as_cymbal)
								{	//If the user opted to make all new drum notes cymbals automatically
									eof_mark_new_note_as_cymbal(eof_song,eof_selected_track,eof_get_track_size(eof_song, eof_selected_track) - 1);
								}
								if(eof_mark_drums_as_double_bass)
								{	//If the user opted to make all new expert bass drum notes as double bass automatically
									eof_mark_new_note_as_double_bass(eof_song,eof_selected_track,eof_get_track_size(eof_song, eof_selected_track) - 1);
								}
								if(eof_mark_drums_as_hi_hat)
								{	//If the user opted to make all new yellow drum notes as one of the specialized hi hat types automatically
									eof_mark_new_note_as_special_hi_hat(eof_song,eof_selected_track,eof_get_track_size(eof_song, eof_selected_track) - 1);
								}
								eof_selection.current_pos = eof_get_note_pos(eof_song, eof_selected_track, eof_get_track_size(eof_song, eof_selected_track) - 1);	//Get the position of the last note that was added
								eof_selection.range_pos_1 = eof_selection.current_pos;
								eof_selection.range_pos_2 = eof_selection.current_pos;
								eof_selection.track = eof_selected_track;
								if(!eof_add_new_notes_to_selection)
								{	//If the user didn't opt to prevent clearing the note selection when adding gems
									memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
								}
								eof_track_sort_notes(eof_song, eof_selected_track);
								eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Fixup notes and retain note selection
								eof_determine_phrase_status(eof_song, eof_selected_track);
								eof_selection.multi[eof_selection.current] = 1;	//Add new note to the selection
								(void) eof_detect_difficulties(eof_song, eof_selected_track);
							}
						}
					}//If user has pressed any key from 1 through 6
				}//If a vocal track is not active
			}//If the mouse is in the fretboard area
		}//If CTRL is not held down and the input method is rex mundi

	/* delete note (Del) */
	/* delete beat (CTRL+Del) */
	/* delete effective fret hand position (SHIFT+Del in a pro guitar track) */
	/* delete note with lower difficulties (CTRL+SHIFT+Del) */
		if(eof_key_code == KEY_DEL)
		{
			if(KEY_EITHER_SHIFT)
			{	//If SHIFT is held
				if(KEY_EITHER_CTRL)
				{	//If CTRL is also held
					(void) eof_menu_note_delete_with_lower_difficulties();
					eof_shift_used = 1;	//Track that the SHIFT key was used
				}
				else
				{	//SHIFT is held but CTRL is not
					(void) eof_track_delete_effective_fret_hand_position();
					eof_shift_used = 1;	//Track that the SHIFT key was used
				}
			}
			else
			{	//SHIFT is not held
				if(KEY_EITHER_CTRL)
				{	//If CTRL is held but SHIFT is not
					(void) eof_menu_beat_delete();
				}
				else
				{	//If neither CTRL nor SHIFT are held
					(void) eof_menu_note_delete();
				}
			}
			eof_use_key();
		}

	/* place bookmark (CTRL+Numpad #) */
	/* seek to bookmark (Numpad #) */
		if(!KEY_EITHER_SHIFT)
		{	//If SHIFT is not held
			if(eof_key_code == KEY_0_PAD)
			{
				if(KEY_EITHER_CTRL)
				{
					(void) eof_menu_edit_bookmark_0();
				}
				else
				{
					(void) eof_menu_song_seek_bookmark_0();
				}
				eof_use_key();
			}
			if(eof_key_code == KEY_1_PAD)
			{
				if(KEY_EITHER_CTRL)
				{
					(void) eof_menu_edit_bookmark_1();
				}
				else
				{
					(void) eof_menu_song_seek_bookmark_1();
				}
				eof_use_key();
			}
			if(eof_key_code == KEY_2_PAD)
			{
				if(KEY_EITHER_CTRL)
				{
					(void) eof_menu_edit_bookmark_2();
				}
				else
				{
					(void) eof_menu_song_seek_bookmark_2();
				}
				eof_use_key();
			}
			if(eof_key_code == KEY_3_PAD)
			{
				if(KEY_EITHER_CTRL)
				{
					(void) eof_menu_edit_bookmark_3();
				}
				else
				{
					(void) eof_menu_song_seek_bookmark_3();
				}
				eof_use_key();
			}
			if(eof_key_code == KEY_4_PAD)
			{
				if(KEY_EITHER_CTRL)
				{
					(void) eof_menu_edit_bookmark_4();
				}
				else
				{
					(void) eof_menu_song_seek_bookmark_4();
				}
				eof_use_key();
			}
			if(eof_key_code == KEY_5_PAD)
			{
				if(KEY_EITHER_CTRL)
				{
					(void) eof_menu_edit_bookmark_5();
				}
				else
				{
					(void) eof_menu_song_seek_bookmark_5();
				}
				eof_use_key();
			}
			if(eof_key_code == KEY_6_PAD)
			{
				if(KEY_EITHER_CTRL)
				{
					(void) eof_menu_edit_bookmark_6();
				}
				else
				{
					(void) eof_menu_song_seek_bookmark_6();
				}
				eof_use_key();
			}
			if(eof_key_code == KEY_7_PAD)
			{
				if(KEY_EITHER_CTRL)
				{
					(void) eof_menu_edit_bookmark_7();
				}
				else
				{
					(void) eof_menu_song_seek_bookmark_7();
				}
				eof_use_key();
			}
			if(eof_key_code == KEY_8_PAD)
			{
				if(KEY_EITHER_CTRL)
				{
					(void) eof_menu_edit_bookmark_8();
				}
				else
				{
					(void) eof_menu_song_seek_bookmark_8();
				}
				eof_use_key();
			}
			if(eof_key_code == KEY_9_PAD)
			{
				if(KEY_EITHER_CTRL)
				{
					(void) eof_menu_edit_bookmark_9();
				}
				else
				{
					(void) eof_menu_song_seek_bookmark_9();
				}
				eof_use_key();
			}
		}//If SHIFT is not held

	/* copy (CTRL+C) */
		if(KEY_EITHER_CTRL && !KEY_EITHER_SHIFT && (eof_key_char == 'c'))
		{
			(void) eof_menu_edit_copy();
			eof_use_key();
		}

	/* paste (CTRL+V) */
		if(KEY_EITHER_CTRL && !KEY_EITHER_SHIFT && (eof_key_char == 'v'))
		{	//If CTRL is held and SHIFT is not
			(void) eof_menu_edit_paste();
			eof_use_key();
		}

	/* old paste (CTRL+P) */
		if(KEY_EITHER_CTRL && !KEY_EITHER_SHIFT && (eof_key_char == 'p'))
		{	//If CTRL is held and SHIFT is not
			(void) eof_menu_edit_old_paste();
			eof_use_key();
		}
	}//If the chart is paused and no catalog entries are playing
	else if(!eof_music_catalog_playback)
	{	//If the chart is playing
	/* lower playback speed (;) */
		if(eof_key_code == KEY_SEMICOLON)
		{	//Read the scan code because the ASCII code cannot represent CTRL or SHIFT with the semicolon key
			if(eof_playback_speed != 100)
			{	//If playback isn't already at 10%
				char lctrl, rctrl;

				(void) eof_menu_edit_playback_speed_helper_slower();
				lctrl = key[KEY_LCONTROL];	//Store the CTRL key states
				rctrl = key[KEY_RCONTROL];
				key[KEY_LCONTROL] = 0;	//Clear both CTRL keys so they are not picked up by eof_music_play() to force half-speed playback
				key[KEY_RCONTROL] = 0;
				eof_music_play(0);	//Stop playback
				eof_music_play(0);	//Resume playback at new speed
				key[KEY_LCONTROL] = lctrl;	//Restore the CTRL key states
				key[KEY_RCONTROL] = rctrl;
			}
			eof_use_key();
		}

	/* increase playback speed (') */
		if(eof_key_code == KEY_QUOTE)
		{	//Read the scan code because the ASCII code cannot represent CTRL or SHIFT with the apostrophe key
			if(eof_playback_speed != 1000)
			{	//If playback isn't already at 100%
				char lctrl, rctrl;	//Used to store the CTRL key states

				(void) eof_menu_edit_playback_speed_helper_faster();
				lctrl = key[KEY_LCONTROL];		//Store the CTRL key states
				rctrl = key[KEY_RCONTROL];
				key[KEY_LCONTROL] = 0;	//Clear both CTRL keys so they are not picked up by eof_music_play() to force half-speed playback
				key[KEY_RCONTROL] = 0;
				eof_music_play(0);	//Stop playback
				eof_music_play(0);	//Resume playback at new speed
				key[KEY_LCONTROL] = lctrl;	//Restore the CTRL key states
				key[KEY_RCONTROL] = rctrl;
			}
			eof_use_key();
		}
	}//If the chart is playing
}

void eof_editor_drum_logic(void)
{
	int bitmask;	//Used for simplifying note placement logic
	EOF_NOTE * new_note = NULL;

	eof_log("eof_editor_drum_logic() entered", 2);

	if(eof_drums.button[0].held)
	{
		eof_held_1++;
	}
	else
	{
		eof_held_1 = 0;
	}
	if(eof_drums.button[1].held)
	{
		eof_held_2++;
	}
	else
	{
		eof_held_2 = 0;
	}
	if(eof_drums.button[2].held)
	{
		eof_held_3++;
	}
	else
	{
		eof_held_3 = 0;
	}
	if(eof_drums.button[3].held)
	{
		eof_held_4++;
	}
	else
	{
		eof_held_4 = 0;
	}
	if(eof_drums.button[4].held)
	{
		eof_held_5++;
	}
	else
	{
		eof_held_5 = 0;
	}
	if(eof_entering_note_note && (eof_music_pos - eof_av_delay - eof_drums.delay >= eof_entering_note_note->pos + 50))
	{
		eof_entering_note_note = NULL;
	}
	bitmask = 0;
	if(eof_held_1 == 1)
	{
		bitmask |= 1;
	}
	if(eof_held_2 == 1)
	{
		bitmask |= 2;
	}
	if(eof_held_3 == 1)
	{
		bitmask |= 4;
	}
	if(eof_held_4 == 1)
	{
		bitmask |= 8;
	}
	if(eof_held_5 == 1)
	{
		bitmask |= 16;
	}

	if(bitmask)
	{
		eof_prepare_undo(EOF_UNDO_TYPE_RECORD);
		if(eof_entering_note_note && (eof_music_pos - eof_av_delay - eof_drums.delay < eof_entering_note_note->pos + 50))
		{	//If altering an existing note
			eof_entering_note_note->note |= bitmask;
		}
		else
		{	//If creating a new note
			new_note = eof_track_add_create_note(eof_song, eof_selected_track, bitmask, eof_music_pos - eof_av_delay - eof_drums.delay, 1, eof_note_type, NULL);
			if(new_note)
			{
				if(eof_mark_drums_as_cymbal)
				{	//If the user opted to make all new drum notes cymbals automatically
					eof_mark_new_note_as_cymbal(eof_song,eof_selected_track,eof_get_track_size(eof_song, eof_selected_track) - 1);
				}
				if(eof_mark_drums_as_double_bass)
				{	//If the user opted to make all new expert bass drum notes as double bass automatically
					eof_mark_new_note_as_double_bass(eof_song,eof_selected_track,eof_get_track_size(eof_song, eof_selected_track) - 1);
				}
				if(eof_mark_drums_as_hi_hat)
				{	//If the user opted to make all new yellow drum notes as one of the specialized hi hat types automatically
					eof_mark_new_note_as_special_hi_hat(eof_song,eof_selected_track,eof_get_track_size(eof_song, eof_selected_track) - 1);
				}
				eof_snap_logic(&eof_snap, eof_get_note_pos(eof_song, eof_selected_track, eof_get_track_size(eof_song, eof_selected_track) - 1));
				eof_set_note_pos(eof_song, eof_selected_track, eof_get_track_size(eof_song, eof_selected_track) - 1, eof_snap.pos);
				eof_entering_note_note = new_note;
				eof_entering_note = 1;
				(void) eof_detect_difficulties(eof_song, eof_selected_track);
				eof_track_sort_notes(eof_song, eof_selected_track);
			}
		}
	}
}

void eof_editor_logic(void)
{
//	eof_log("eof_editor_logic() entered");

	unsigned long i, note, notepos, numtabs;
	unsigned long tracknum;
	unsigned long bitmask = 0;	//Used to reduce duplicated logic
	EOF_NOTE * new_note = NULL;
	int pos = eof_music_pos / eof_zoom;
	int lpos;
	long notelength;
	int eof_scaled_mouse_x = mouse_x, eof_scaled_mouse_y = mouse_y;	//Rescaled mouse coordinates that account for the x2 zoom display feature

	if(!eof_song_loaded)
		return;
	if(eof_vocals_selected)
		return;

	if(eof_screen_zoom)
	{	//If x2 zoom is in effect, take that into account for the mouse position
		eof_scaled_mouse_x = mouse_x / 2;
		eof_scaled_mouse_y = mouse_y / 2;
	}
	eof_editor_logic_common();

	if((eof_pen_note.note & 32) && eof_track_is_legacy_guitar(eof_song, eof_selected_track) && !eof_open_strum_enabled(eof_selected_track))
	{	//Special case:  Lane 6 of the pen note was enabled, but the active track does not currently have that lane enabled
		eof_pen_note.note &= ~32;	//Clear that lane from the pen note
	}
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	if(eof_music_paused)
	{	//If the chart is paused
		char infretboard = 0;

		if(!(mouse_b & 1) && !(mouse_b & 2) && !(eof_key_code == KEY_INSERT))
		{	//If the left and right mouse buttons and insert key are NOT pressed
			eof_undo_toggle = 0;
			if(eof_notes_moved)
			{
				eof_track_sort_notes(eof_song, eof_selected_track);
				eof_track_fixup_notes(eof_song, eof_selected_track, 1);
				eof_determine_phrase_status(eof_song, eof_selected_track);
				eof_notes_moved = 0;
			}
		}

		/* mouse is in the fretboard area (or Feedback input method is in use) */
		if((eof_scaled_mouse_y >= eof_window_editor->y + 25 + EOF_EDITOR_RENDER_OFFSET) && (eof_scaled_mouse_y < eof_window_editor->y + eof_screen_layout.fretboard_h + EOF_EDITOR_RENDER_OFFSET))
		{	//If the mouse is in the fretboard area
			infretboard = 1;
		}
		if((eof_input_mode == EOF_INPUT_FEEDBACK) || infretboard)
		{
			int x_tolerance = 6 * eof_zoom;	//This is how far left or right of a note the mouse is allowed to be to still be considered to hover over that note
			unsigned long move_offset = 0;
			char move_direction = 1;	//Assume right mouse movement by default
			int revert = 0;
			int revert_amount = 0;
			char undo_made = 0;

			lpos = pos < 300 ? (eof_scaled_mouse_x - 20) * eof_zoom : ((pos - 300) + eof_scaled_mouse_x - 20) * eof_zoom;	//Translate mouse position to a time position
			if(infretboard)
			{	//Only search for the mouse hover note if the mouse is actually in the fretboard area
				eof_hover_note = eof_find_hover_note(lpos, x_tolerance, 1);	//Find the mouse hover note
			}
			if(eof_input_mode == EOF_INPUT_FEEDBACK)
			{	//If Feedback input method is in effect
				x_tolerance = 2;	//The hover note tracking is much tighter since keyboard seek commands are more precise than mouse controls
				eof_seek_hover_note = eof_find_hover_note(eof_music_pos - eof_av_delay, x_tolerance, 0);	//Find the seek hover note
			}

			if((eof_input_mode == EOF_INPUT_PIANO_ROLL) || (eof_input_mode == EOF_INPUT_REX) || (eof_input_mode == EOF_INPUT_FEEDBACK))
			{	//If piano roll, rex mundi or feedback input modes are in use
				if(eof_hover_note >= 0)
				{	//If a note is being moused over
					if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
					{	//If legacy view is in effect, set the pen note to the legacy mask
						eof_pen_note.note = eof_song->pro_guitar_track[tracknum]->note[eof_hover_note]->legacymask;
					}
					else
					{	//Otherwise set it to the note's normal bitmask
						eof_pen_note.note = eof_get_note_note(eof_song, eof_selected_track, eof_hover_note);
					}
					eof_pen_note.length = eof_get_note_length(eof_song, eof_selected_track, eof_hover_note);
					if(!eof_mouse_drug)
					{
						eof_pen_note.pos = eof_get_note_pos(eof_song, eof_selected_track, eof_hover_note);
						eof_pen_note.note |= eof_find_pen_note_mask();	//Set the appropriate bits
					}
				}//If a note is being moused over
				else
				{	//If a note is not being moused over
					if((eof_input_mode == EOF_INPUT_REX) || (eof_input_mode == EOF_INPUT_FEEDBACK))
					{
						eof_pen_note.note = 0;
					}
					/* calculate piece for piano roll mode */
					else if(eof_input_mode == EOF_INPUT_PIANO_ROLL)
					{
						eof_pen_note.note = eof_find_pen_note_mask();	//Find eof_hover_piece and set the appropriate bits in the pen note
					}

					if(KEY_EITHER_SHIFT)
					{	//If shift is held down, a new note will be 1ms long
						eof_pen_note.length = 1;
					}
					else
					{	//Otherwise it will be as long as the current grid snap value (or 100ms if grid snap is off)
						eof_pen_note.length = eof_snap.length;
					}
				}//If a note is not being moused over
			}//If piano roll, rex mundi or feedback input modes are in use

			/* handle initial middle click (only if full screen 3D view is not in effect) */
			if(!eof_full_screen_3d && (mouse_b & 4) && eof_mclick_released)
			{	//If the middle click hasn't been processed yet
				while(mouse_b & 4);	//Wait until the middle mouse button is released before proceeding (this depends on automatic mouse polling, so EOF cannot call poll_mouse() manually or this becomes an infinite loop)
				if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
				{	//If a pro guitar track is active
					if(eof_count_selected_notes(NULL))
					{	//If any notes in the active track difficulty are selected
						(void) eof_menu_note_edit_pro_guitar_note();
					}
					else if(eof_hover_note >= 0)
					{	//if a note is being hovered over
						eof_selection.current = eof_hover_note;		//Temporarily mark the hover note as the selected note
						eof_selection.multi[eof_hover_note] = 1;
						(void) eof_menu_note_edit_pro_guitar_note();
						eof_selection.current = EOF_MAX_NOTES - 1;	//Clear the selected note statuses
						eof_selection.multi[eof_hover_note] = 0;
					}
					eof_mclick_released = 0;
				}
			}
			else
			{	//If the middle mouse button is not held
				eof_mclick_released = 1;	//Track that the middle click button has been release and is ready to be processed again the next time it is clicked
			}

			/* handle initial left click (only if full screen 3D view is not in effect) */
			if(!eof_full_screen_3d && (mouse_b & 1) && eof_lclick_released)
			{
				int ignore_range = 0;

				eof_click_x = eof_scaled_mouse_x;
				eof_click_y = eof_scaled_mouse_y;
				eof_lclick_released = 0;
				if(eof_hover_note >= 0)
				{
					if(eof_selection.current != EOF_MAX_NOTES - 1)
					{
						eof_selection.last = eof_selection.current;
						eof_selection.last_pos = eof_selection.current_pos;
					}
					else
					{
						eof_selection.last = eof_hover_note;
						eof_selection.last_pos = eof_get_note_pos(eof_song, eof_selected_track, eof_selection.last);
					}
					eof_selection.current = eof_hover_note;
					if(eof_mix_midi_tones_enabled)
					{	//If MIDI tones are enabled
						eof_play_pro_guitar_note_midi(eof_song, eof_selected_track, eof_selection.current);	//Play the tones for this note if it's a pro guitar/bass note
					}
					eof_selection.current_pos = eof_get_note_pos(eof_song, eof_selected_track, eof_selection.current);
					if(KEY_EITHER_SHIFT)
					{
						eof_shift_used = 1;	//Track that the SHIFT key was used
						if((eof_selection.range_pos_1 == 0) && (eof_selection.range_pos_2 == 0) && (eof_selection.current_pos == 0))
						{
							ignore_range = 1;
							eof_selection.range_pos_1 = eof_selection.current_pos;
							eof_selection.range_pos_2 = eof_selection.current_pos;
						}
						else
						{
							if(eof_selection.current_pos > eof_selection.range_pos_2)
							{
								eof_selection.range_pos_2 = eof_selection.current_pos;
							}
							else
							{
								eof_selection.range_pos_1 = eof_selection.current_pos;
							}
						}
					}
					else
					{
						eof_selection.range_pos_1 = eof_selection.current_pos;
						eof_selection.range_pos_2 = eof_selection.current_pos;
					}
					eof_pegged_note = eof_selection.current;
					eof_peg_x = eof_get_note_pos(eof_song, eof_selected_track, eof_pegged_note);
					if(!KEY_EITHER_CTRL)
					{
						/* Shift+Click selects range */
						if(KEY_EITHER_SHIFT && !ignore_range)
						{
							eof_shift_used = 1;	//Track that the SHIFT key was used
							if(eof_selection.range_pos_1 < eof_selection.range_pos_2)
							{
								for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
								{
									if((eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type) && (eof_get_note_pos(eof_song, eof_selected_track, i) >= eof_selection.range_pos_1) && (eof_get_note_pos(eof_song, eof_selected_track, i) <= eof_selection.range_pos_2))
									{
										if(eof_selection.track != eof_selected_track)
										{
											eof_selection.track = eof_selected_track;
											memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
										}
										eof_selection.multi[i] = 1;
										eof_undo_last_type = EOF_UNDO_TYPE_NONE;
									}
								}
							}
							else
							{
								for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
								{
									if((eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type) && (eof_get_note_pos(eof_song, eof_selected_track, i) >= eof_selection.range_pos_2) && (eof_get_note_pos(eof_song, eof_selected_track, i) <= eof_selection.range_pos_1))
									{
										if(eof_selection.track != eof_selected_track)
										{
											eof_selection.track = eof_selected_track;
											memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
										}
										eof_selection.multi[i] = 1;
										eof_undo_last_type = EOF_UNDO_TYPE_NONE;
									}
								}
							}
						}
						else
						{
							if(!eof_selection.multi[eof_selection.current])
							{
								memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
							}
							if(eof_selection.multi[eof_selection.current] == 1)
							{
								if(eof_selection.track != eof_selected_track)
								{
									eof_selection.track = eof_selected_track;
									memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
								}
								eof_selection.multi[eof_selection.current] = 2;
								eof_undo_last_type = EOF_UNDO_TYPE_NONE;
							}
							else
							{
								if(eof_selection.track != eof_selected_track)
								{
									eof_selection.track = eof_selected_track;
									memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
								}
								eof_selection.multi[eof_selection.current] = 1;
								eof_undo_last_type = EOF_UNDO_TYPE_NONE;
							}
						}
					}

					/* Ctrl+Click adds to selected notes */
					else
					{
						if(eof_selection.multi[eof_selection.current] == 1)
						{
							if(eof_selection.track != eof_selected_track)
							{
								eof_selection.track = eof_selected_track;
								memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
							}
							eof_selection.multi[eof_selection.current] = 2;
							eof_undo_last_type = EOF_UNDO_TYPE_NONE;
						}
						else
						{
							if(eof_selection.track != eof_selected_track)
							{
								eof_selection.track = eof_selected_track;
								memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
							}
							eof_selection.multi[eof_selection.current] = 1;
							eof_undo_last_type = EOF_UNDO_TYPE_NONE;
						}
					}
				}

				/* clicking on no note deselects all notes */
				else
				{
					if(!KEY_EITHER_SHIFT && !KEY_EITHER_CTRL)
					{	//Neither SHIFT nor CTRL are held
						(void) eof_menu_edit_deselect_all();
					}
				}
			}// handle initial left click
			if(!(mouse_b & 1))
			{	//If the left mouse button is NOT pressed
				if(!eof_lclick_released)
				{
					eof_lclick_released = 1;	//Set the "just released" status
				}

				/* mouse button just released, check to see what needs doing */
				else if(eof_lclick_released == 1)
				{
					if(!eof_mouse_drug)
					{
						if(!KEY_EITHER_CTRL)
						{
							if(!KEY_EITHER_SHIFT)
							{	//SHIFT is not held
								memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
								if(eof_selection.multi[eof_selection.current])
								{
									eof_selection.multi[eof_selection.current] = 0;
								}
								else
								{
									eof_selection.track = eof_selected_track;
									eof_selection.multi[eof_selection.current] = 1;
								}
								eof_undo_last_type = EOF_UNDO_TYPE_NONE;
							}
						}
						else
						{
							if(eof_selection.multi[eof_selection.current] == 2)
							{
								eof_selection.multi[eof_selection.current] = 0;
								eof_undo_last_type = EOF_UNDO_TYPE_NONE;
							}
							else if(eof_hover_note >= 0)
							{
								if(eof_selection.track != eof_selected_track)
								{
									eof_selection.track = eof_selected_track;
									memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
								}
								eof_selection.multi[eof_selection.current] = 1;
								eof_undo_last_type = EOF_UNDO_TYPE_NONE;
							}
						}
					}
					eof_lclick_released = 2;	//Set the "left click release handled" status
				}
				else
				{
					if(eof_mouse_drug)
					{
						eof_mouse_drug = 0;
					}
				}
			}//If the left mouse button is NOT pressed
			if(!eof_full_screen_3d && !eof_song->tags->click_drag_disabled && (mouse_b & 1) && !eof_lclick_released && (lpos >= 0))
			{	//If neither full screen 3D view is in use nor is click and drag disabled, the left mouse button is being held and the mouse is right of the left edge of the piano roll
				if(eof_mouse_drug && !KEY_EITHER_SHIFT && !KEY_EITHER_CTRL)
				{
					eof_mouse_drug++;
					if(eof_mouse_drug == 11)
					{
						eof_mickeys_x = eof_scaled_mouse_x - eof_click_x;
					}
				}
				if((eof_mickeys_x != 0) && !eof_mouse_drug)
				{
					eof_mouse_drug++;
				}
				if((eof_mouse_drug > 10) && (eof_selection.current != EOF_MAX_NOTES - 1))
				{
					if((eof_snap_mode != EOF_SNAP_OFF) && !KEY_EITHER_CTRL)
					{	//Move notes by grid snap
						if(eof_pen_note.pos < eof_get_note_pos(eof_song, eof_selected_track, eof_selection.current))
						{	//Left mouse movement
							move_direction = -1;
							move_offset = eof_get_note_pos(eof_song, eof_selected_track, eof_selection.current) - eof_pen_note.pos;
						}
						else
						{	//Right mouse movement
							move_direction = 1;
							move_offset = eof_pen_note.pos - eof_get_note_pos(eof_song, eof_selected_track, eof_selection.current);
						}
					}
					else
					{
						if(eof_mickeys_x < 0)
						{	//Left mouse movement
							move_direction = -1;
							move_offset = -eof_mickeys_x * eof_zoom;
						}
						else
						{	//Right mouse movement
							move_direction = 1;
							move_offset = eof_mickeys_x * eof_zoom;
						}
					}
					if(move_offset)
					{
						eof_auto_adjust_sections(eof_song, eof_selected_track, move_offset, move_direction, &undo_made);	//Move sections accordingly
						eof_auto_adjust_tech_notes(eof_song, eof_selected_track, move_offset, move_direction, &undo_made);	//Move tech notes accordingly
					}
					for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
					{	//For each note in the active track
						if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i])
							continue;	//If the note is not selected, skip it

						notepos = eof_get_note_pos(eof_song, eof_selected_track, i);
						if(notepos == eof_selection.current_pos)
						{
							if(move_direction < 0)
							{	//Left mouse movement
								eof_selection.current_pos -= move_offset;
							}
							else
							{	//Right mouse movement
								eof_selection.current_pos += move_offset;
							}
						}
						if(move_direction < 0)
						{	//If the user is moving notes left
							if((move_offset > notepos) || (notepos - move_offset < eof_song->beat[0]->pos))
							{	//If the move would make the note position negative or otherwise earlier than the first beat
								move_offset = notepos - eof_song->beat[0]->pos;	//Adjust the move offset to line it up with the first beat marker
							}
						}
						if(move_offset == 0)
						{	//If the offset has become 0
							break;	//Don't move the notes
						}
						if(!undo_made)
						{	//Only create the undo state before moving the first note
							eof_notes_moved = 1;
							eof_prepare_undo(EOF_UNDO_TYPE_NONE);
							undo_made = 1;
						}
						eof_move_note_pos(eof_song, eof_selected_track, i, move_offset, move_direction);
						notepos = eof_get_note_pos(eof_song, eof_selected_track, i);	//Get the updated note position
						notelength = eof_get_note_length(eof_song, eof_selected_track, i);
						if(notepos + notelength >= eof_chart_length)
						{	//If the moved note is at or after the end of the chart
							revert |= 1;
							revert_amount = notepos + notelength - eof_chart_length;	//This positive value will be subtracted from the note via the revert loop
						}
						else if(notepos <= eof_song->beat[0]->pos)
						{	//If the moved note is at or before the first beat marker
							revert |= 2;
							revert_amount = eof_song->beat[0]->pos - notepos;			//This negative value will be subtracted from the note via the revert loop
						}
					}
					if(revert)
					{
						if(revert == 3)
						{	//If something unexpected happened and the drag operation resulted in both edges of the chart having note(s) pushed beyond the edge
							allegro_message("Logic error in eof_editor_logic() note drag handling");
						}
						for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
						{
							if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
							{
								eof_set_note_pos(eof_song, eof_selected_track, i, eof_get_note_pos(eof_song, eof_selected_track, i) - revert_amount);
							}
						}
					}
				}
			}//If neither full screen 3D view is in use nor is click and drag disabled, the left mouse button is being held and the mouse is right of the left edge of the piano roll
			if(!eof_full_screen_3d && ((mouse_b & 2) || eof_key_code == KEY_INSERT) && eof_rclick_released && eof_pen_note.note && (eof_pen_note.pos < eof_chart_length))
			{	//Full screen 3D view is not in effect, right mouse click or Insert key pressed, and the pen note is valid
				eof_selection.range_pos_1 = 0;
				eof_selection.range_pos_2 = 0;
				eof_rclick_released = 0;
				if((eof_input_mode == EOF_INPUT_PIANO_ROLL) || (eof_input_mode == EOF_INPUT_CLASSIC) || (eof_input_mode == EOF_INPUT_HOLD))
				{	//All three of these input methods toggle gems when clicking on the right mouse button
					if(!eof_undo_toggle)
					{
						eof_prepare_undo(EOF_UNDO_TYPE_NOTE_SEL);
					}
					if(eof_hover_note >= 0)
					{	//If altering an existing note
						if(eof_input_mode == EOF_INPUT_PIANO_ROLL)
						{	//Piano roll input method must use special logic to determine the lane ordering for the pen note
							bitmask = eof_find_pen_note_mask();	//Set the appropriate bits
						}
						else
						{	//The other two use the pen note bitmask normally
							bitmask = eof_pen_note.note;
						}

						if(bitmask)
						{	//If a valid pen bitmask was obtained
							eof_selection.current = eof_hover_note;
							eof_selection.track = eof_selected_track;
							if(!eof_add_new_notes_to_selection)
							{	//If the user didn't opt to prevent clearing the note selection when adding gems
								memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
							}
							if(eof_legacy_view && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
							{	//If legacy view is in effect, alter the note's legacy bitmask
								note = eof_song->pro_guitar_track[tracknum]->note[eof_hover_note]->legacymask;
								note ^= bitmask;
								eof_song->pro_guitar_track[tracknum]->note[eof_hover_note]->legacymask = note;
							}
							else
							{	//Otherwise alter the note's normal bitmask and delete the note if necessary
								note = eof_get_note_note(eof_song, eof_selected_track, eof_hover_note);	//Examine the hover note...
								note ^= bitmask;	//as it would look by toggling the pen note's gem
								if(note == 0)
								{	//If the note just had all lanes cleared, delete the note
									eof_track_delete_note(eof_song, eof_selected_track, eof_hover_note);
									eof_selection.current = EOF_MAX_NOTES - 1;
									eof_track_sort_notes(eof_song, eof_selected_track);
									eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Fixup notes and retain note selection
									eof_determine_phrase_status(eof_song, eof_selected_track);
									(void) eof_detect_difficulties(eof_song, eof_selected_track);
								}
								else if(note & bitmask)
								{	//If toggling this lane on, create a new note at the hover note's position, with the same length and type as the hover note
									long notelen = eof_get_note_length(eof_song, eof_selected_track, eof_hover_note);	//The default length of the new note
									if(eof_new_note_length_1ms)
									{	//If the new note's length is being overridden as 1ms
										notelen = 1;
									}
									if(eof_track_add_create_note(eof_song, eof_selected_track, bitmask, eof_get_note_pos(eof_song, eof_selected_track, eof_hover_note), notelen, eof_note_type, NULL))
									{	//If the new note was created successfully
										if(eof_mark_drums_as_cymbal)
										{	//If the user opted to make all new drum notes cymbals automatically
											eof_mark_new_note_as_cymbal(eof_song,eof_selected_track,eof_get_track_size(eof_song, eof_selected_track) - 1);
										}
										if(eof_mark_drums_as_double_bass)
										{	//If the user opted to make all new expert bass drum notes as double bass automatically
											eof_mark_new_note_as_double_bass(eof_song,eof_selected_track,eof_get_track_size(eof_song, eof_selected_track) - 1);
										}
										if(eof_mark_drums_as_hi_hat)
										{	//If the user opted to make all new yellow drum notes as one of the specialized hi hat types automatically
											eof_mark_new_note_as_special_hi_hat(eof_song,eof_selected_track,eof_get_track_size(eof_song, eof_selected_track) - 1);
										}
										eof_track_sort_notes(eof_song, eof_selected_track);
									}
								}
								else
								{	//Otherwise a gem is being toggled off
									eof_set_note_note(eof_song, eof_selected_track, eof_hover_note, note);
								}
							}
							if(note != 0)
							{	//Cleanup edited/added note
								eof_selection.track = eof_selected_track;
								eof_selection.multi[eof_selection.current] = 1;
								eof_selection.current_pos = eof_get_note_pos(eof_song, eof_selected_track, eof_selection.current);
								eof_selection.range_pos_1 = eof_selection.current_pos;
								eof_selection.range_pos_2 = eof_selection.current_pos;
								eof_track_fixup_notes(eof_song, eof_selected_track, 0);	//Run cleanup to prevent open bass<->lane 1 conflicts
							}
						}
					}//If altering an existing note
					else
					{	//If creating a new note
						long notelen = eof_snap.length;	//The default length of the new note
						if(KEY_EITHER_SHIFT || eof_new_note_length_1ms)
						{	//If the new note's length is being overridden as 1ms
							notelen = 1;
						}
						new_note = eof_track_add_create_note(eof_song, eof_selected_track, eof_pen_note.note, eof_pen_note.pos, notelen, eof_note_type, NULL);
						if(KEY_EITHER_SHIFT)
						{
							eof_shift_used = 1;	//Track that the SHIFT key was used
						}
						if(new_note)
						{
							if(eof_mark_drums_as_cymbal)
							{	//If the user opted to make all new drum notes cymbals automatically
								eof_mark_new_note_as_cymbal(eof_song,eof_selected_track,eof_get_track_size(eof_song, eof_selected_track) - 1);
							}
							if(eof_mark_drums_as_double_bass)
							{	//If the user opted to make all new expert bass drum notes as double bass automatically
								eof_mark_new_note_as_double_bass(eof_song,eof_selected_track,eof_get_track_size(eof_song, eof_selected_track) - 1);
							}
							if(eof_mark_drums_as_hi_hat)
							{	//If the user opted to make all new yellow drum notes as one of the specialized hi hat types automatically
								eof_mark_new_note_as_special_hi_hat(eof_song,eof_selected_track,eof_get_track_size(eof_song, eof_selected_track) - 1);
							}
							eof_selection.current_pos = eof_get_note_pos(eof_song, eof_selected_track, eof_get_track_size(eof_song, eof_selected_track) - 1);
							eof_selection.range_pos_1 = eof_selection.current_pos;
							eof_selection.range_pos_2 = eof_selection.current_pos;
							eof_selection.track = eof_selected_track;
							if(!eof_add_new_notes_to_selection)
							{	//If the user didn't opt to prevent clearing the note selection when adding gems
								memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
							}
							eof_track_sort_notes(eof_song, eof_selected_track);
							eof_track_fixup_notes(eof_song, eof_selected_track, 1);	//Fixup notes and retain note selection
							eof_determine_phrase_status(eof_song, eof_selected_track);
							eof_selection.multi[eof_selection.current] = 1;	//Add new note to the selection
							(void) eof_detect_difficulties(eof_song, eof_selected_track);
						}
					}
				}
			}//Full screen 3D view is not in effect, right mouse click or Insert key pressed, and the pen note is valid
			if(!(mouse_b & 2) && !(eof_key_code == KEY_INSERT))
			{
				eof_rclick_released = 1;
			}

			/* increment/decrement fret value (CTRL+scroll wheel) */
			/* increase/decrease note length (scroll wheel , SHIFT+scroll wheel or CTRL+SHIFT+scroll wheel) */
			if(eof_mickey_z != 0)
			{	//If there was scroll wheel activity
				if(KEY_EITHER_CTRL && !KEY_EITHER_SHIFT)
				{	//increment/decrement fret value
					if(eof_mickey_z > 0)
					{	//Decrement fret value
						eof_set_pro_guitar_fret_number(2,0);	//Decrement fret value
					}
					else if(eof_mickey_z < 0)
					{	//Increment fret value
						eof_set_pro_guitar_fret_number(1,0);	//Increment fret value
					}
				}
				else
				{	//increase/decrease note length
					unsigned long adjust = abs(eof_mickey_z) * 100;	//Default adjustment length when grid snap is disabled

					if(eof_snap_mode == EOF_SNAP_OFF)
					{
						if(KEY_EITHER_SHIFT)
						{
							eof_shift_used = 1;	//Track that the SHIFT key was used
							if(KEY_EITHER_CTRL)
							{	//CTRL+SHIFT+scroll wheel
								adjust = abs(eof_mickey_z);	//1 ms length change
							}
							else
							{	//SHIFT+scroll wheel
								adjust = abs(eof_mickey_z) * 10;	//10ms length change
							}
						}
					}
					else
					{
						adjust = 0;	//Will indicate to eof_adjust_note_length() to use the grid snap value
					}
					if(eof_mickey_z > 0)
					{	//Decrease note length
						eof_adjust_note_length(eof_song, eof_selected_track, adjust, -1);	//Decrease selected notes by the appropriate length
					}
					else if(eof_mickey_z < 0)
					{	//Increase note length
						eof_adjust_note_length(eof_song, eof_selected_track, adjust, 1);	//Increase selected notes by the appropriate length
					}
				}
			}//If there was scroll wheel activity
		}//mouse is in the fretboard area
		else
		{
			eof_pen_visible = 0;
		}
	}//If the chart is paused
	else
	{	//If the chart is not paused
		for(i = 0; i < eof_song->beats - 1; i++)
		{
			if((eof_music_pos >= eof_song->beat[i]->pos) && (eof_music_pos < eof_song->beat[i + 1]->pos))
			{
				if(eof_hover_beat_2 != i)
				{
					eof_hover_beat_2 = i;
				}
				break;
			}
		}
		if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
		{
			eof_editor_drum_logic();
		}
	}//If the chart is not paused

	/* select difficulty */
	numtabs = eof_get_number_displayed_tabs();
	if((eof_scaled_mouse_y >= eof_window_editor->y + 7) && (eof_scaled_mouse_y < eof_window_editor->y + 20 + 8) && (eof_scaled_mouse_x > 12) && (eof_scaled_mouse_x < 12 + numtabs * 80 + 12 - 1) && (mouse_b & 1) && !eof_full_screen_3d)
	{	//If the left mouse button is held down and the mouse is over one of the difficulty tabs, and full screen 3d mode isn't in effect
		eof_hover_type = (eof_scaled_mouse_x - 12) / 80;	//Determine which tab number was clicked
		if(eof_hover_type < 0)
		{	//Bounds check
			eof_hover_type = 0;
		}
		else if(eof_hover_type >= numtabs)
		{
			eof_hover_type = numtabs - 1;
		}
		if(eof_song->track[eof_selected_track]->flags & EOF_TRACK_FLAG_UNLIMITED_DIFFS)
		{	//If this track is not limited to 5 difficulties
			if(eof_hover_type == numtabs - 1)
			{	//If the last tab was clicked
				eof_hover_type = eof_song->track[eof_selected_track]->numdiffs - 1;	//Change to the highest difficulty in the track
			}
			else if(eof_hover_type > 0)
			{	//If the first tab (which will already change to the track's lowest difficulty) wasn't clicked
				if(eof_note_type < numtabs / 2)
				{	//If the tabs represent the lowest difficulties
					eof_hover_type = eof_hover_type - 1;
				}
				else if(eof_note_type >= eof_song->track[eof_selected_track]->numdiffs - (numtabs / 2))
				{	//If the tabs represent the highest difficulties
					eof_hover_type = eof_song->track[eof_selected_track]->numdiffs - numtabs + 1 + eof_hover_type;
				}
				else
				{	//If the center tab represents the active difficulty
					eof_hover_type = eof_hover_type + eof_note_type - (numtabs / 2);
				}
			}
			mouse_b &= ~1;	//Clear the left mouse button status or else the tab logic will run during next loop and cause the highest difficulty to be accepted
		}
		if(eof_note_type != eof_hover_type)
		{
			eof_note_type = eof_hover_type;
			eof_mix_find_claps();
			eof_mix_start_helper();
			eof_fix_window_title();
			(void) eof_detect_difficulties(eof_song, eof_selected_track);
		}
	}
	else
	{
		eof_hover_type = -1;
	}

	if(((mouse_b & 2) || (eof_key_code == KEY_INSERT)) && ((eof_input_mode == EOF_INPUT_REX) || (eof_input_mode == EOF_INPUT_FEEDBACK)))
	{	//If the right mouse button or Insert key is pressed, a song is loaded and Rex Mundi or Feedback input mode is in use
		eof_emergency_stop_music();
		eof_render();
		eof_show_mouse(screen);
		if(eof_full_screen_3d)
		{	//If full screen 3D view is in effect
			(void) do_menu(eof_right_click_menu_full_screen_3d_view, mouse_x, mouse_y);
			eof_clear_input();
		}
		else
		{	//Full screen 3D view is not in effect
			if((eof_scaled_mouse_y >= eof_window_editor->y + EOF_EDITOR_RENDER_OFFSET - 4 - 19) && (eof_scaled_mouse_y < eof_window_editor->y + EOF_EDITOR_RENDER_OFFSET + 18 + 8))
			{	//mouse is in beat marker area
				lpos = pos < 300 ? (eof_song->beat[eof_selected_beat]->pos / eof_zoom + 20) : 300;
				eof_prepare_menus();
				(void) do_menu(eof_beat_menu, lpos, mouse_y);
				eof_clear_input();
			}
			else if((eof_scaled_mouse_y >= eof_window_editor->y + 25 + EOF_EDITOR_RENDER_OFFSET) && (eof_scaled_mouse_y < eof_window_editor->y + eof_screen_layout.fretboard_h + EOF_EDITOR_RENDER_OFFSET))
			{	//mouse is in the fretboard area
				if(eof_hover_note >= 0)
				{
					if(eof_count_selected_notes(NULL) == 0)
					{	//No notes are selected
						eof_selection.current = eof_hover_note;
						eof_selection.current_pos = eof_get_note_pos(eof_song, eof_selected_track, eof_selection.current);
						if(eof_selection.track != eof_selected_track)
						{
							eof_selection.track = eof_selected_track;
							memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
						}
						eof_selection.multi[eof_selection.current] = 1;
						eof_render();
					}
					else
					{
						if(!eof_selection.multi[eof_hover_note])
						{
							eof_selection.current = eof_hover_note;
							eof_selection.current_pos = eof_get_note_pos(eof_song, eof_selected_track, eof_selection.current);
							memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
							eof_selection.multi[eof_selection.current] = 1;
							eof_render();
						}
					}
				}
				eof_prepare_menus();
				if(eof_count_selected_notes(NULL) > 0)
				{
					(void) do_menu(eof_right_click_menu_note, mouse_x, mouse_y);
				}
				else
				{
					(void) do_menu(eof_right_click_menu_normal, mouse_x, mouse_y);
				}
				eof_clear_input();
			}
			else if(eof_scaled_mouse_y < eof_window_3d->y)
			{
				eof_prepare_menus();
				(void) do_menu(eof_right_click_menu_normal, mouse_x, mouse_y);
				eof_clear_input();
			}
		}//Full screen 3D view is not in effect
		eof_show_mouse(NULL);
	}//If the right mouse button or Insert key is pressed, a song is loaded and Rex Mundi or Feedback input mode is in use
}

void eof_vocal_editor_logic(void)
{
//	eof_log("eof_vocal_editor_logic() entered");

	unsigned long i, notepos;
	unsigned long tracknum;
	EOF_SNAP_DATA drag_snap = {0, 0.0, 0, 0.0, 0, 0, 0, {0.0}, {0.0}, 0, 0, 0, 0}; // help with dragging across snap locations
	int eof_scaled_mouse_x = mouse_x, eof_scaled_mouse_y = mouse_y;	//Rescaled mouse coordinates that account for the x2 zoom display feature

	if(!eof_song_loaded)
		return;
	if(!eof_vocals_selected)
		return;

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	if(eof_screen_zoom)
	{	//If x2 zoom is in effect, take that into account for the mouse position
		eof_scaled_mouse_x = mouse_x / 2;
		eof_scaled_mouse_y = mouse_y / 2;
	}
	eof_editor_logic_common();

	if(eof_music_paused)
	{	//If the chart is paused
		if(!(mouse_b & 1) && !(mouse_b & 2) && !(eof_key_code == KEY_INSERT))
		{
			eof_undo_toggle = 0;
			if(eof_notes_moved)
			{
				eof_track_sort_notes(eof_song, eof_selected_track);
				eof_track_fixup_notes(eof_song, eof_selected_track, 1);
				eof_notes_moved = 0;
			}
		}

		/* mouse is in the mini keyboard area */
		if((eof_scaled_mouse_x < 20) && (eof_scaled_mouse_y >= eof_window_editor->y + 25 + EOF_EDITOR_RENDER_OFFSET) && (eof_scaled_mouse_y < eof_window_editor->y + eof_screen_layout.fretboard_h + EOF_EDITOR_RENDER_OFFSET))
		{
			eof_pen_lyric.pos = -1;
			eof_pen_lyric.length = 1;
			eof_pen_lyric.note = eof_vocals_offset + (EOF_EDITOR_RENDER_OFFSET + 35 + eof_screen_layout.vocal_y - eof_scaled_mouse_y) / eof_screen_layout.vocal_tail_size;
			if((eof_pen_lyric.note < eof_vocals_offset) || (eof_pen_lyric.note >= eof_vocals_offset + eof_screen_layout.vocal_view_size))
			{
				eof_pen_lyric.note = 0;
			}
			if(!(mouse_b & 1))
			{
				eof_last_tone = 0;
			}
			if(!eof_full_screen_3d && (mouse_b & 1))
			{
				int n = eof_vocals_offset + (EOF_EDITOR_RENDER_OFFSET + 35 + eof_screen_layout.vocal_y - eof_scaled_mouse_y) / eof_screen_layout.vocal_tail_size;
				if((n >= eof_vocals_offset) && (n < eof_vocals_offset + eof_screen_layout.vocal_view_size) && (n != eof_last_tone))
				{
					eof_mix_play_note(n);
					eof_last_tone = n;
				}
			}
		}

		/* mouse is in the fretboard area */
		else if((eof_scaled_mouse_y >= eof_window_editor->y + 25 + EOF_EDITOR_RENDER_OFFSET) && (eof_scaled_mouse_y < eof_window_editor->y + eof_screen_layout.fretboard_h + EOF_EDITOR_RENDER_OFFSET))
		{
			int x_tolerance = 6 * eof_zoom;	//This is how far left or right of a lyric the mouse is allowed to be to still be considered to hover over that lyric
			int pos = eof_music_pos / eof_zoom;
			int lpos = pos < 300 ? (eof_scaled_mouse_x - 20) * eof_zoom : ((pos - 300) + eof_scaled_mouse_x - 20) * eof_zoom;
			int rpos; // place to store pen_lyric.pos in case we are hovering over a note and need the original position before it was changed to the note location
			unsigned long move_offset = 0;
			char move_direction = 1;	//Assume right mouse movement by default
			int revert = 0;
			int revert_amount = 0;
			char undo_made = 0;

			eof_snap_logic(&eof_snap, lpos);
			eof_snap_length_logic(&eof_snap);
			eof_pen_lyric.pos = eof_snap.pos;
			rpos = eof_pen_lyric.pos;
			eof_pen_lyric.length = eof_snap.length;
			if(eof_key_code == KEY_BACKSPACE)
			{	//If entering a vocal percussion note
				eof_pen_lyric.note = EOF_LYRIC_PERCUSSION;
			}
			else
			{
				eof_pen_lyric.note = eof_vocals_offset + (EOF_EDITOR_RENDER_OFFSET + 35 + eof_screen_layout.vocal_y - eof_scaled_mouse_y) / eof_screen_layout.vocal_tail_size;
			}
			if((eof_pen_lyric.note < eof_vocals_offset) || (eof_pen_lyric.note >= eof_vocals_offset + eof_screen_layout.vocal_view_size))
			{
				eof_pen_lyric.note = 0;
			}
			eof_pen_visible = 1;

			eof_hover_note = eof_find_hover_note(lpos, x_tolerance, 1);	//Find the mouse hover note
			if(eof_input_mode == EOF_INPUT_FEEDBACK)
			{	//If Feedback input method is in effect
				x_tolerance = 2;	//The hover note tracking is much tighter since keyboard seek commands are more precise than mouse controls
				eof_seek_hover_note = eof_find_hover_note(eof_music_pos - eof_av_delay, x_tolerance, 0);	//Find the seek hover note
			}

			if((eof_hover_note >= 0) && !(mouse_b & 1))
			{
				eof_pen_lyric.pos = eof_song->vocal_track[tracknum]->lyric[eof_hover_note]->pos;
				eof_pen_lyric.length = eof_song->vocal_track[tracknum]->lyric[eof_hover_note]->length;
			}
			if((eof_input_mode == EOF_INPUT_PIANO_ROLL) || (eof_input_mode == EOF_INPUT_REX))
			{
				if(eof_hover_note >= 0)
				{
					if(!eof_mouse_drug)
					{
						eof_pen_lyric.pos = eof_song->vocal_track[tracknum]->lyric[eof_hover_note]->pos;
					}
				}
			}

			/* handle initial middle click (only if full screen 3D view is not in effect) */
			if(!eof_full_screen_3d && (mouse_b & 4) && eof_mclick_released)
			{	//If the middle click hasn't been processed yet
				while(mouse_b & 4);	//Wait until the middle mouse button is released before proceeding (this depends on automatic mouse polling, so EOF cannot call poll_mouse() manually or this becomes an infinite loop)
				if(eof_hover_lyric)
				{	//If a lyric is being hovered over
					(void) eof_menu_edit_deselect_all();
					eof_selection.current = eof_hover_note;
					eof_selection.multi[eof_hover_note] = 1;
					(void) eof_edit_lyric_dialog();
				}
				eof_mclick_released = 0;
			}
			else
			{	//If the middle mouse button is not held
				eof_mclick_released = 1;	//Track that the middle click button has been release and is ready to be processed again the next time it is clicked
			}

			/* handle initial left click */
			if(!eof_full_screen_3d && (mouse_b & 1) && eof_lclick_released)
			{
				int ignore_range = 0;

				eof_click_x = eof_scaled_mouse_x;
				eof_click_y = eof_scaled_mouse_y;
				eof_lclick_released = 0;
				if(eof_hover_note >= 0)
				{
					if(eof_selection.current != EOF_MAX_NOTES - 1)
					{
						eof_selection.last = eof_selection.current;
						eof_selection.last_pos = eof_selection.current_pos;
					}
					else
					{
						eof_selection.last = eof_hover_note;
						eof_selection.last_pos = eof_song->vocal_track[tracknum]->lyric[eof_selection.last]->pos;
					}
					eof_selection.current = eof_hover_note;
					if(eof_mix_vocal_tones_enabled)
					{
						if(eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->note != EOF_LYRIC_PERCUSSION)
							eof_mix_play_note(eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->note);
						else
							(void) play_sample(eof_sound_chosen_percussion, 255.0 * (eof_percussion_volume / 100.0), 127, 1000 + eof_audio_fine_tune, 0);
					}
					eof_selection.current_pos = eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->pos;
					if(KEY_EITHER_SHIFT)
					{
						eof_shift_used = 1;	//Track that the SHIFT key was used
						if((eof_selection.range_pos_1 == 0) && (eof_selection.range_pos_2 == 0) && (eof_selection.current_pos == 0))
						{
							ignore_range = 1;
							eof_selection.range_pos_1 = eof_selection.current_pos;
							eof_selection.range_pos_2 = eof_selection.current_pos;
						}
						else
						{
							if(eof_selection.current_pos > eof_selection.range_pos_2)
							{
								eof_selection.range_pos_2 = eof_selection.current_pos;
							}
							else
							{
								eof_selection.range_pos_1 = eof_selection.current_pos;
							}
						}
					}
					else
					{
						eof_selection.range_pos_1 = eof_selection.current_pos;
						eof_selection.range_pos_2 = eof_selection.current_pos;
					}
					eof_pegged_note = eof_selection.current;
					eof_peg_x = lpos - eof_song->vocal_track[tracknum]->lyric[eof_pegged_note]->pos;
					eof_last_pen_pos = rpos;
					if(!KEY_EITHER_CTRL)
					{
						/* Shift+Click selects range */
						if(KEY_EITHER_SHIFT && !ignore_range)
						{
							eof_shift_used = 1;	//Track that the SHIFT key was used
							if(eof_selection.range_pos_1 < eof_selection.range_pos_2)
							{
								for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
								{
									if((eof_song->vocal_track[tracknum]->lyric[i]->pos >= eof_selection.range_pos_1) && (eof_song->vocal_track[tracknum]->lyric[i]->pos <= eof_selection.range_pos_2))
									{
										if(eof_selection.track != EOF_TRACK_VOCALS)
										{
											eof_selection.track = EOF_TRACK_VOCALS;
											memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
										}
										eof_selection.multi[i] = 1;
										eof_undo_last_type = EOF_UNDO_TYPE_NONE;
									}
								}
							}
							else
							{
								for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
								{
									if((eof_song->vocal_track[tracknum]->lyric[i]->pos >= eof_selection.range_pos_2) && (eof_song->vocal_track[tracknum]->lyric[i]->pos <= eof_selection.range_pos_1))
									{
										if(eof_selection.track != EOF_TRACK_VOCALS)
										{
											eof_selection.track = EOF_TRACK_VOCALS;
											memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
										}
										eof_selection.multi[i] = 1;
										eof_undo_last_type = EOF_UNDO_TYPE_NONE;
									}
								}
							}
						}
						else
						{
							if(!eof_selection.multi[eof_selection.current])
							{
								memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
							}
							if(eof_selection.multi[eof_selection.current] == 1)
							{
								if(eof_selection.track != EOF_TRACK_VOCALS)
								{
									eof_selection.track = EOF_TRACK_VOCALS;
									memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
								}
								eof_selection.multi[eof_selection.current] = 2;
								eof_undo_last_type = EOF_UNDO_TYPE_NONE;
							}
							else
							{
								if(eof_selection.track != EOF_TRACK_VOCALS)
								{
									eof_selection.track = EOF_TRACK_VOCALS;
									memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
								}
								eof_selection.multi[eof_selection.current] = 1;
								eof_undo_last_type = EOF_UNDO_TYPE_NONE;
							}
						}
					}

					/* Ctrl+Click adds to selected notes */
					else
					{
						if(eof_selection.multi[eof_selection.current] == 1)
						{
							if(eof_selection.track != EOF_TRACK_VOCALS)
							{
								eof_selection.track = EOF_TRACK_VOCALS;
								memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
							}
							eof_selection.multi[eof_selection.current] = 2;
							eof_undo_last_type = EOF_UNDO_TYPE_NONE;
						}
						else
						{
							if(eof_selection.track != EOF_TRACK_VOCALS)
							{
								eof_selection.track = EOF_TRACK_VOCALS;
								memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
							}
							eof_selection.multi[eof_selection.current] = 1;
							eof_undo_last_type = EOF_UNDO_TYPE_NONE;
						}
					}
				}

				/* clicking on no note deselects all notes */
				else
				{
					if(!KEY_EITHER_SHIFT && !KEY_EITHER_CTRL)
					{	//Neither SHIFT nor CTRL are held
						(void) eof_menu_edit_deselect_all();
					}
				}
			}//handle initial left click
			if(!(mouse_b & 1))
			{	//If the left mouse button is NOT being held
				if(!eof_lclick_released)
				{
					eof_lclick_released = 1;	//Set the "just released" status
				}

				/* mouse button just released, check to see what needs doing */
				else if(eof_lclick_released == 1)
				{
					if(!eof_mouse_drug)
					{
						if(!KEY_EITHER_CTRL)
						{
							if(!KEY_EITHER_SHIFT)
							{	//SHIFT is not held
								memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
								if(eof_selection.multi[eof_selection.current])
								{
									eof_selection.multi[eof_selection.current] = 0;
								}
								else
								{
									eof_selection.track = EOF_TRACK_VOCALS;
									eof_selection.multi[eof_selection.current] = 1;
								}
								eof_undo_last_type = EOF_UNDO_TYPE_NONE;
							}
						}
						else
						{
							if(eof_selection.multi[eof_selection.current] == 2)
							{
								eof_selection.multi[eof_selection.current] = 0;
								eof_undo_last_type = EOF_UNDO_TYPE_NONE;
							}
							else if(eof_hover_note >= 0)
							{
								if(eof_selection.track != EOF_TRACK_VOCALS)
								{
									eof_selection.track = EOF_TRACK_VOCALS;
									memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
								}
								eof_selection.multi[eof_selection.current] = 1;
								eof_undo_last_type = EOF_UNDO_TYPE_NONE;
							}
						}
					}
					eof_lclick_released = 2;	//Set the "left click release handled" status
				}
				else
				{
					if(eof_mouse_drug)
					{
						eof_mouse_drug = 0;
					}
				}
			}
			if(!eof_full_screen_3d && (mouse_b & 1) && !eof_song->tags->click_drag_disabled && !eof_lclick_released && (lpos >= eof_peg_x))
			{	//If neither full screen 3D view is is use nor is click and drag disabled, the left mouse button is being held and the mouse is right of the left edge of the piano roll
				if(eof_mouse_drug)
				{
					eof_mouse_drug++;
					if(eof_mouse_drug == 11)
					{
						eof_mickeys_x = eof_scaled_mouse_x - eof_click_x;
					}
				}
				if((eof_mickeys_x != 0) && !eof_mouse_drug)
				{
					eof_mouse_drug++;
				}
				if((eof_mouse_drug > 10) && (eof_selection.current != EOF_MAX_NOTES - 1))
				{
					if((eof_snap_mode != EOF_SNAP_OFF) && !KEY_EITHER_CTRL)
					{	//Move notes by grid snap
						eof_snap_logic(&drag_snap, lpos - eof_peg_x);
						if(drag_snap.pos < eof_song->vocal_track[tracknum]->lyric[eof_pegged_note]->pos)
						{	//Left mouse movement
							move_direction = -1;
							move_offset = eof_song->vocal_track[tracknum]->lyric[eof_pegged_note]->pos - drag_snap.pos;
						}
						else
						{	//Right mouse movement
							move_direction = 1;
							move_offset = drag_snap.pos - eof_song->vocal_track[tracknum]->lyric[eof_pegged_note]->pos;
						}
						eof_last_pen_pos = rpos;
					}
					else
					{
						if(eof_mickeys_x < 0)
						{	//Left mouse movement
							move_direction = -1;
							move_offset = -eof_mickeys_x * eof_zoom;
						}
						else
						{	//Right mouse movement
							move_direction = 1;
							move_offset = eof_mickeys_x * eof_zoom;
						}
					}
					if(move_offset)
						eof_auto_adjust_sections(eof_song, EOF_TRACK_VOCALS, move_offset, move_direction, &undo_made);	//Move lyric sections accordingly
					for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
					{	//For each lyric in the track
						notepos = eof_get_note_pos(eof_song, eof_selected_track, i);
						if((eof_selection.track != eof_selected_track) || !eof_selection.multi[i])
							continue;	//If this lyric is not selected, skip it

						if(notepos == eof_selection.current_pos)
						{
							if(move_direction < 0)
							{	//Left mouse movement
								eof_selection.current_pos -= move_offset;
							}
							else
							{	//Right mouse movement
								eof_selection.current_pos += move_offset;
							}
						}
						if(move_direction < 0)
						{	//If the user is moving lyrics left
							if((move_offset > notepos) || (notepos - move_offset < eof_song->beat[0]->pos))
							{	//If the move would make the lyric position negative or otherwise earlier than the first beat
								move_offset = notepos - eof_song->beat[0]->pos;	//Adjust the move offset to line it up with the first beat marker
							}
						}
						if(move_offset == 0)
						{	//If the offset has become 0
							break;	//Don't move the notes
						}
						if(!undo_made)
						{	//Only create the undo state before moving the first note
							eof_notes_moved = 1;
							eof_prepare_undo(EOF_UNDO_TYPE_NONE);
							undo_made = 1;
						}
						eof_move_note_pos(eof_song, eof_selected_track, i, move_offset, move_direction);
						notepos = eof_get_note_pos(eof_song, eof_selected_track, i);	//Get the updated lyric position
						if(notepos + eof_song->vocal_track[tracknum]->lyric[i]->length >= eof_chart_length)
						{
							revert = 1;
							revert_amount = notepos + eof_song->vocal_track[tracknum]->lyric[i]->length - eof_chart_length;
						}
					}
					if(revert)
					{
						for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
						{
							if((eof_selection.track == eof_selected_track) && eof_selection.multi[i])
							{
								eof_song->vocal_track[tracknum]->lyric[i]->pos -= revert_amount;
							}
						}
					}
				}
			}//If neither full screen 3D view is is use nor is click and drag disabled, the left mouse button is being held and the mouse is right of the left edge of the piano roll
			if(!eof_full_screen_3d && ((((eof_input_mode != EOF_INPUT_REX) && ((mouse_b & 2) || (eof_key_code == KEY_INSERT))) || (((eof_input_mode == EOF_INPUT_REX) && !KEY_EITHER_SHIFT && !KEY_EITHER_CTRL && ((eof_key_char == '1') || (eof_key_char == '2') || (eof_key_char == '3') || (eof_key_char == '4') || (eof_key_char == '5') || (eof_key_char == '6'))) && eof_rclick_released && (eof_pen_lyric.pos < eof_chart_length))) || (eof_key_code == KEY_BACKSPACE)))
			{	//If full screen 3D view is not in effect and input to add a note is provided
				eof_selection.range_pos_1 = 0;
				eof_selection.range_pos_2 = 0;
				if(eof_hover_note >= 0)
				{	//If clicking on an existing lyric
					if(!eof_undo_toggle)
					{
						eof_prepare_undo(EOF_UNDO_TYPE_NOTE_SEL);
					}

					/* delete whole lyric if clicking word */
					if((eof_scaled_mouse_y >= EOF_EDITOR_RENDER_OFFSET + 35 + eof_screen_layout.lyric_y) && (eof_scaled_mouse_y < EOF_EDITOR_RENDER_OFFSET + 35 + eof_screen_layout.lyric_y + 16))
					{
						eof_track_delete_note(eof_song, eof_selected_track, eof_hover_note);	//Delete the hovered over lyric
						eof_selection.current = EOF_MAX_NOTES - 1;
						eof_track_sort_notes(eof_song, eof_selected_track);
					}

					/* delete only the note */
					else if(eof_pen_lyric.note == eof_song->vocal_track[tracknum]->lyric[eof_hover_note]->note)
					{
						if(!eof_check_string(eof_song->vocal_track[tracknum]->lyric[eof_hover_note]->text))	//If removing the pitch from a note that is already textless
						{	//Perform the cleanup code as if the lyric was deleted by inputting on top of the lyric text as above
							eof_track_delete_note(eof_song, eof_selected_track, eof_hover_note);	//Delete the hovered over lyric
							eof_selection.current = EOF_MAX_NOTES - 1;
							eof_track_sort_notes(eof_song, eof_selected_track);
						}
						else if(eof_rclick_released)	//Just remove the pitch, but only if the right mouse button was just clicked
						{
							eof_song->vocal_track[tracknum]->lyric[eof_hover_note]->note = 0;
							eof_selection.current = eof_hover_note;
							eof_selection.track = eof_selected_track;
							memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
							eof_selection.multi[eof_selection.current] = 1;
							eof_selection.current_pos = eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->pos;
							eof_selection.range_pos_1 = eof_selection.current_pos;
							eof_selection.range_pos_2 = eof_selection.current_pos;
						}
					}

					/* move note */
					else
					{
						eof_song->vocal_track[tracknum]->lyric[eof_hover_note]->note = eof_pen_lyric.note;
						eof_selection.current = eof_hover_note;
						eof_selection.track = EOF_TRACK_VOCALS;
						memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
						eof_selection.multi[eof_selection.current] = 1;
						eof_selection.current_pos = eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->pos;
						eof_selection.range_pos_1 = eof_selection.current_pos;
						eof_selection.range_pos_2 = eof_selection.current_pos;
						if(eof_mix_vocal_tones_enabled)
						{
							eof_mix_play_note(eof_pen_lyric.note);
						}
					}
					eof_rclick_released = 0;	//Track that the right mouse button is being held
				}

				/* create new note */
				else
				{
					eof_rclick_released = 0;	//Track that the right mouse button is being held
					if(eof_key_code == KEY_BACKSPACE)
					{	//Map the percussion note here
						eof_pen_lyric.note = EOF_LYRIC_PERCUSSION;
						eof_use_key();
					}
					if(eof_mix_vocal_tones_enabled)
					{
						if(eof_pen_lyric.note == EOF_LYRIC_PERCUSSION)
						{	//Play the percussion at the user-specified cue volume
							(void) play_sample(eof_sound_chosen_percussion, 255.0 * (eof_percussion_volume / 100.0), 127, 1000 + eof_audio_fine_tune, 0);
						}
						else
						{
							eof_mix_play_note(eof_pen_lyric.note);
						}
					}
					(void) eof_new_lyric_dialog();
				}
			}
			if(eof_input_mode == EOF_INPUT_REX)
			{
				if(!((eof_key_char == '1') || (eof_key_char == '2') || (eof_key_char == '3') || (eof_key_char == '4') || (eof_key_char == '5') || (eof_key_char == '6')))
				{
					eof_rclick_released = 1;
				}
			}
			else
			{
				if(!(mouse_b & 2) && !(eof_key_code == KEY_INSERT))
				{
					eof_rclick_released = 1;
				}
			}
			if((eof_mickey_z != 0) && eof_count_selected_notes(NULL))
			{
				eof_prepare_undo(EOF_UNDO_TYPE_NOTE_LENGTH);
			}
			if(eof_snap_mode == EOF_SNAP_OFF)
			{
				if(KEY_EITHER_CTRL)
				{
					for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
					{
						if((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i])
						{
							eof_song->vocal_track[tracknum]->lyric[i]->length -= eof_mickey_z * 10;
						}
					}
				}
				else
				{
					for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
					{
						if((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i])
						{
							eof_song->vocal_track[tracknum]->lyric[i]->length -= eof_mickey_z * 100;
						}
					}
				}
			}
			else
			{
				unsigned long b;
				if(eof_mickey_z > 0)
				{
					for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
					{
						if((eof_selection.track != EOF_TRACK_VOCALS) || !eof_selection.multi[i])
							continue;	//If the vocal track isn't selected or this lyric isn't selected, skip this lyric

						b = eof_get_beat(eof_song, eof_song->vocal_track[tracknum]->lyric[i]->pos + eof_song->vocal_track[tracknum]->lyric[i]->length - 1);
						if(eof_beat_num_valid(eof_song, b))
						{
							eof_snap_logic(&eof_tail_snap, eof_song->beat[b]->pos);
						}
						else
						{
							eof_snap_logic(&eof_tail_snap, eof_song->vocal_track[tracknum]->lyric[i]->pos + eof_song->vocal_track[tracknum]->lyric[i]->length - 1);
						}
						eof_snap_length_logic(&eof_tail_snap);
//							allegro_message("%d, %d\n%lu, %lu", eof_tail_snap.length, eof_tail_snap.beat, eof_get_note_pos(eof_selected_track, i) + eof_get_note_length(eof_selected_track, i), eof_song->beat[eof_tail_snap.beat]->pos);	//Debugging
						eof_song->vocal_track[tracknum]->lyric[i]->length -= eof_tail_snap.length;
						if(eof_song->vocal_track[tracknum]->lyric[i]->length > 1)
						{
							eof_snap_logic(&eof_tail_snap, eof_song->vocal_track[tracknum]->lyric[i]->pos + eof_song->vocal_track[tracknum]->lyric[i]->length);
							eof_note_set_tail_pos(eof_song, eof_selected_track, i, eof_tail_snap.pos);
						}
					}
				}
				else if(eof_mickey_z < 0)
				{
					for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
					{
						if((eof_selection.track != EOF_TRACK_VOCALS) || !eof_selection.multi[i])
							continue;	//If the vocal track isn't selected or this lyric isn't selected, skip this lyric

						eof_snap_logic(&eof_tail_snap, eof_song->vocal_track[tracknum]->lyric[i]->pos + eof_song->vocal_track[tracknum]->lyric[i]->length + 1);
						if(eof_tail_snap.pos > eof_song->vocal_track[tracknum]->lyric[i]->pos + eof_song->vocal_track[tracknum]->lyric[i]->length + 1)
						{
							eof_note_set_tail_pos(eof_song, eof_selected_track, i, eof_tail_snap.pos);
						}
						else
						{
							eof_snap_length_logic(&eof_tail_snap);
							eof_song->vocal_track[tracknum]->lyric[i]->length += eof_tail_snap.length;
							eof_snap_logic(&eof_tail_snap, eof_song->vocal_track[tracknum]->lyric[i]->pos + eof_song->vocal_track[tracknum]->lyric[i]->length);
							eof_note_set_tail_pos(eof_song, eof_selected_track, i, eof_tail_snap.pos);
						}
					}
				}
			}
			if(eof_mickey_z != 0)
			{
				eof_track_fixup_notes(eof_song, eof_selected_track, 1);
			}
			if((eof_key_char == 'p') && !KEY_EITHER_CTRL && !KEY_EITHER_SHIFT)
			{	//P is held, but neither CTRL nor SHIFT are
				if(eof_pen_lyric.note != eof_last_tone)
				{
					eof_last_tone = eof_pen_lyric.note;
					eof_mix_play_note(eof_pen_lyric.note);
				}
			}
			if(!(eof_key_char == 'p'))
			{
				eof_last_tone = -1;
			}
		}
		else
		{
			eof_pen_visible = 0;
		}
	}//If the chart is paused
	else
	{	//If the chart is not paused
		for(i = 0; i < eof_song->beats - 1; i++)
		{
			if((eof_music_pos >= eof_song->beat[i]->pos) && (eof_music_pos < eof_song->beat[i + 1]->pos))
			{
				if(eof_hover_beat_2 != i)
				{
					eof_hover_beat_2 = i;
				}
				break;
			}
		}
		if(i == eof_song->vocal_track[tracknum]->lyrics)
		{
			eof_pen_lyric.note = 0;
		}
	}//If the chart is not paused

	if(((mouse_b & 2) || (eof_key_code == KEY_INSERT)) && ((eof_input_mode == EOF_INPUT_REX) || (eof_input_mode == EOF_INPUT_FEEDBACK)))
	{	//If the right mouse button or Insert key is pressed, a song is loaded and Rex Mundi or Feedback input mode is in use
		eof_emergency_stop_music();
		eof_render();
		eof_show_mouse(screen);
		if(eof_full_screen_3d)
		{	//If full screen 3D view is in effect
			(void) do_menu(eof_right_click_menu_full_screen_3d_view, mouse_x, mouse_y);
			eof_clear_input();
		}
		else
		{	//Full screen 3D view is not in effect
			if((eof_scaled_mouse_y >= eof_window_editor->y + EOF_EDITOR_RENDER_OFFSET - 4 - 19) && (eof_scaled_mouse_y < eof_window_editor->y + EOF_EDITOR_RENDER_OFFSET + 18 + 8))
			{	//mouse is in beat marker area
				int pos = eof_music_pos / eof_zoom;
				int lpos = pos < 300 ? (eof_song->beat[eof_selected_beat]->pos / eof_zoom + 20) : 300;

				eof_prepare_menus();
				(void) do_menu(eof_beat_menu, lpos, mouse_y);
				eof_clear_input();
			}
			else if((eof_scaled_mouse_y >= eof_window_editor->y + 25 + EOF_EDITOR_RENDER_OFFSET) && (eof_scaled_mouse_y < eof_window_editor->y + eof_screen_layout.fretboard_h + EOF_EDITOR_RENDER_OFFSET))
			{	//mouse is in the fretboard area
				if(eof_hover_note >= 0)
				{
					if(eof_count_selected_notes(NULL) == 0)
					{	//If no notes are selected
						eof_selection.current = eof_hover_note;
						eof_selection.current_pos = eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->pos;
						if(eof_selection.track != EOF_TRACK_VOCALS)
						{
							eof_selection.track = EOF_TRACK_VOCALS;
							memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
						}
						eof_selection.multi[eof_selection.current] = 1;
						eof_render();
					}
					else
					{
						if(!eof_selection.multi[eof_hover_note])
						{
							eof_selection.current = eof_hover_note;
							eof_selection.current_pos = eof_song->vocal_track[tracknum]->lyric[eof_selection.current]->pos;
							memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
							eof_selection.multi[eof_selection.current] = 1;
							eof_render();
						}
					}
				}
				eof_prepare_menus();
				if(eof_count_selected_notes(NULL) > 0)
				{
					(void) do_menu(eof_right_click_menu_note, mouse_x, mouse_y);
					eof_clear_input();
				}
				else
				{
					(void) do_menu(eof_right_click_menu_normal, mouse_x, mouse_y);
					eof_clear_input();
				}
			}
			else if(eof_scaled_mouse_y < eof_window_3d->y)
			{
				eof_prepare_menus();
				(void) do_menu(eof_right_click_menu_normal, mouse_x, mouse_y);
				eof_clear_input();
			}
		}//Full screen 3D view is not in effect
		eof_show_mouse(NULL);
	}//If the right mouse button or Insert key is pressed, a song is loaded and Rex Mundi or Feedback input mode is in use
}

int eof_get_ts_text(unsigned long beat, char * buffer)
{
//	eof_log("eof_get_ts_text() entered");
	int ret = 0;

	if(!buffer || !eof_song || (beat >= eof_song->beats))
	{
		return 0;
	}
	if(eof_song->beat[beat]->flags & EOF_BEAT_FLAG_START_4_4)
	{
		(void) ustrcpy(buffer, "4/4");
		ret = 1;
	}
	else if(eof_song->beat[beat]->flags & EOF_BEAT_FLAG_START_2_4)
	{
		(void) ustrcpy(buffer, "2/4");
		ret = 1;
	}
	else if(eof_song->beat[beat]->flags & EOF_BEAT_FLAG_START_3_4)
	{
		(void) ustrcpy(buffer, "3/4");
		ret = 1;
	}
	else if(eof_song->beat[beat]->flags & EOF_BEAT_FLAG_START_5_4)
	{
		(void) ustrcpy(buffer, "5/4");
		ret = 1;
	}
	else if(eof_song->beat[beat]->flags & EOF_BEAT_FLAG_START_6_4)
	{
		(void) ustrcpy(buffer, "6/4");
		ret = 1;
	}
	else if(eof_song->beat[beat]->flags & EOF_BEAT_FLAG_CUSTOM_TS)
	{
		(void) uszprintf(buffer, 16, "%lu/%lu", ((eof_song->beat[beat]->flags & 0xFF000000)>>24)+1, ((eof_song->beat[beat]->flags & 0x00FF0000)>>16)+1);
		ret = 1;
	}
	else
	{
		(void) ustrcpy(buffer, "");
	}
	return ret;
}

int eof_get_tempo_text(unsigned long beat, char * buffer)
{
	double current_bpm;

	if(!buffer || !eof_song || (beat >= eof_song->beats))
	{
		return 0;
	}

	current_bpm = 60000000.0 / (double)eof_song->beat[beat]->ppqn;
	(void) uszprintf(buffer, 16, "%03.2f ", current_bpm);

	return 1;
}

void eof_render_editor_notes(EOF_WINDOW *window)
{
	unsigned long start;	//Will store the timestamp of the left visible edge of the piano roll
	unsigned long i, numnotes;
	char drawhighlight = 0;

	if(!eof_song_loaded || !window)
		return;	//Invalid parameter

	numnotes = eof_get_track_size(eof_song, eof_selected_track);	//Get the number of notes in this legacy/pro guitar track
	start = eof_determine_piano_roll_left_edge();
	if(((eof_input_mode == EOF_INPUT_PIANO_ROLL) || (eof_input_mode == EOF_INPUT_REX) || (eof_input_mode == EOF_INPUT_FEEDBACK)) && eof_music_paused)
	{	//Cache the results of the check for whether the hover note should be drawn highlighted
		drawhighlight = 1;
	}
	for(i = 0; i < numnotes; i++)
	{	//Render all visible notes in the list
		if((eof_note_type == eof_get_note_type(eof_song, eof_selected_track, i)) && (eof_get_note_pos(eof_song, eof_selected_track, i) + eof_get_note_length(eof_song, eof_selected_track, i) >= start))
		{	//If this note is in the selected instrument difficulty and would render at or after the left edge of the piano roll
			if(drawhighlight && (eof_hover_note == i))
			{	//If the input mode is piano roll, rex mundi or Feedback and the chart is paused, and the note is being moused over
				(void) eof_note_draw(eof_selected_track, i, ((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && eof_music_paused) ? 1 : i == eof_hover_note ? 2 : 3, window);
			}
			else
			{	//Render the note normally
				if(eof_note_draw(eof_selected_track, i, ((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && eof_music_paused) ? 1 : i == eof_hover_note ? 2 : 0, window) == 1)
					break;	//If this note was rendered right of the viewable area, all following notes will too, so stop rendering
			}
		}
	}
	if(eof_music_paused && eof_pen_visible && (eof_pen_note.pos < eof_chart_length))
	{
		if(!eof_mouse_drug)
		{
			if(window != eof_window_editor2)
			{	//Only render the pen note for the primary piano roll
				if((eof_input_mode == EOF_INPUT_PIANO_ROLL) || (eof_input_mode == EOF_INPUT_REX) || (eof_input_mode == EOF_INPUT_FEEDBACK))
				{
					(void) eof_note_draw(0, 0, 3, window);	//Render the pen note
				}
				else
				{
					(void) eof_note_draw(0, 0, 0, window);	//Render the pen note
				}
			}
		}
	}
}

void eof_render_editor_window(EOF_WINDOW *window, unsigned char windownum)
{
//	eof_log("eof_render_editor_window() entered");
	EOF_PRO_GUITAR_TRACK *tp = NULL;
	char render_tech_notes = 0;
	unsigned long temptrack;
	int temphover;

	if(!eof_song_loaded || !window)
		return;

	if(eof_disable_2d_rendering || eof_full_screen_3d)	//If the user disabled the 2D window's rendering (or enabled full screen 3D view)
		return;											//Return immediately

	if(eof_song->track[eof_selected_track]->track_format == EOF_VOCAL_TRACK_FORMAT)
	{	//If this is a vocal track
		eof_render_vocal_editor_window(window);
		return;
	}
	else if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
	{	//If the track being rendered is a pro guitar track
		tp = eof_song->pro_guitar_track[eof_song->track[eof_selected_track]->tracknum];
		if((tp->note == tp->technote) && (windownum == 1))
		{	//If tech view is in effect for the active track and the primary piano roll is being drawn
			render_tech_notes = 1;	//Track that the regular notes will be rendered and then the tech notes will render on top of them
		}
	}

	eof_render_editor_window_common(window);	//Perform rendering that is common to the note and vocal editor displays
	if(render_tech_notes && tp)
	{	//If tech notes are to render on top of the regular notes
		eof_menu_pro_guitar_track_disable_tech_view(tp);	//Switch back to the regular note array
		temphover = eof_hover_note;				//Temporarily clear the hover note, since any hover note in effect at this time applies to the tech notes and not the normal notes
		eof_hover_note = -1;
		temptrack = eof_selection.track;		//Likewise temporarily clear the selection track number
		eof_selection.track = 0;
		eof_render_editor_notes(window);		//Render its notes
		eof_hover_note = temphover;				//Restore the hover note
		eof_selection.track = temptrack;		//Restore the selection track
		eof_menu_pro_guitar_track_enable_tech_view(tp);	//Switch back to the tech note array
	}
	eof_render_editor_notes(window);			//Render the notes in the active track
	eof_render_editor_window_common2(window);	//Perform post-rendering that is common to the note and vocal editor displays
}

void eof_render_vocal_editor_window(EOF_WINDOW *window)
{
//	eof_log("eof_render_vocal_editor_window() entered");

	unsigned long i;
	unsigned long tracknum;
	int pos = eof_music_pos / eof_zoom;	//Current seek position
	int lpos;							//The position of the first beat marker
	unsigned long start;	//Will store the timestamp of the left visible edge of the piano roll
	int kcol, kcol2;
	int n;
	int ny;
	int red = 0;

	if(!eof_song_loaded || (eof_song->track[eof_selected_track]->track_format != EOF_VOCAL_TRACK_FORMAT) || !window)
		return;
	if(eof_disable_2d_rendering || eof_full_screen_3d)	//If the disabled the 2D window's rendering (or enabled full screen 3D view)
		return;											//Return immediately

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	eof_render_editor_window_common(window);

	if(pos < 300)
	{
		lpos = 20;
	}
	else
	{
		lpos = 20 - (pos - 300);
	}

	/* clear lyric text area */
	rectfill(window->screen, 0, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.lyric_y + 1, window->w - 1, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.lyric_y + 1 + 16, eof_color_black);
	hline(window->screen, lpos, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.lyric_y + 1 + 16, lpos + (eof_chart_length) / eof_zoom, eof_color_white);

	/* draw lyric lines */
	for(i = 0; i < eof_song->vocal_track[tracknum]->lines; i++)
	{ 	//The -5 is a vertical offset to allow the phrase marker rectangle render as high as the lyric text itself.  The +4 is an offset to allow the rectangle to render as low as the lyric text
		rectfill(window->screen, lpos + eof_song->vocal_track[tracknum]->line[i].start_pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + (15-5) + eof_screen_layout.note_y[0] - 2 + 8, lpos + eof_song->vocal_track[tracknum]->line[i].end_pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[0] + 2 + 8 +4, (eof_song->vocal_track[tracknum]->line[i].flags & EOF_LYRIC_LINE_FLAG_OVERDRIVE) ? makecol(64, 128, 64) : makecol(0, 0, 127));
	}

	start = eof_determine_piano_roll_left_edge();
	for(i = 0; i < eof_song->vocal_track[tracknum]->lyrics; i++)
	{	//For each lyric
		if(eof_song->vocal_track[tracknum]->lyric[i]->pos + eof_song->vocal_track[tracknum]->lyric[i]->length >= start)
		{	//If the lyric would render at or after the left edge of the piano roll
			if(((eof_input_mode == EOF_INPUT_PIANO_ROLL) || (eof_input_mode == EOF_INPUT_REX)) && eof_music_paused && (eof_hover_note == i))
			{	//If the chart is paused and the lyric is moused over
				(void) eof_lyric_draw(eof_song->vocal_track[tracknum]->lyric[i], ((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i] && eof_music_paused) ? 1 : i == eof_hover_note ? 2 : 3, window);
			}
			else
			{
				if(eof_lyric_draw(eof_song->vocal_track[tracknum]->lyric[i], ((eof_selection.track == EOF_TRACK_VOCALS) && eof_selection.multi[i] && eof_music_paused) ? 1 : i == eof_hover_note ? 2 : 0, window) > 0)
					break;	//Break if the function indicated that the lyric was rendered beyond the clip window
			}
		}
	}
	if(eof_music_paused && eof_pen_visible && (eof_pen_note.pos < eof_chart_length))
	{
		if(!eof_mouse_drug)
		{
			if((eof_input_mode == EOF_INPUT_PIANO_ROLL) || (eof_input_mode == EOF_INPUT_REX))
			{
				(void) eof_lyric_draw(&eof_pen_lyric, 3, window);
			}
			else
			{
				(void) eof_lyric_draw(&eof_pen_lyric, 0, window);
			}
		}
	}

	/* draw mini keyboard */
	for(i = 0; i < eof_screen_layout.vocal_view_size; i++)
	{
		n = (eof_vocals_offset + i) % 12;
		ny = EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.vocal_y - (i % eof_screen_layout.vocal_view_size + 1) * eof_screen_layout.vocal_tail_size;
		if(eof_vocals_offset + i == eof_pen_lyric.note)
		{
			kcol = eof_color_green;
			kcol2 = makecol(0, 192, 0);
			if((n == 0) && !red)
			{
				red = 1;
			}
			textprintf_ex(window->screen, font, 21, ny + eof_screen_layout.vocal_tail_size / 2 - text_height(font) / 2, eof_color_white, eof_color_black, "%s", eof_get_tone_name(eof_pen_lyric.note));
		}
		else if((n == 1) || (n == 3) || (n == 6) || (n == 8) || (n == 10))
		{
			kcol = makecol(16, 16, 16);
			kcol2 = eof_color_black;
		}
		else if(n == 0)
		{
			kcol = makecol(255, 160, 160);
			kcol2 = makecol(192, 96, 96);
			red = 1;
		}
		else
		{
			kcol = eof_color_white;
			kcol2 = eof_color_silver;
		}
		rectfill(window->screen, 0, ny, 19, ny + eof_screen_layout.vocal_tail_size - 1, kcol);
		hline(window->screen, 0, ny + eof_screen_layout.vocal_tail_size - 1, 19, kcol2);
	}

	eof_render_editor_window_common2(window);
}

void eof_render_editor_window_2(void)
{
	int temp_type, temp_track, temp_pos, temp_hover;
	long temp_selected;

	if(eof_display_second_piano_roll)
	{	//If the secondary piano roll is to be displayed
		char restore_tech_view = 0;		//If tech view is in effect, it is temporarily disabled until after the secondary piano roll has been rendered

		if(eof_note_type2 > 255)
		{	//If the difficulty hasn't been initialized
			eof_note_type2 = eof_note_type;
		}
		if(eof_selected_track2 == 0)
		{	//If the track hasn't been initialized
			eof_selected_track2 = eof_selected_track;
		}
		if(eof_music_pos2 < 0)
		{	//If the position hasn't been initialized
			eof_music_pos2 = eof_music_pos;
		}

		restore_tech_view = eof_menu_track_get_tech_view_state(eof_song, eof_selected_track2);
		eof_menu_track_set_tech_view_state(eof_song, eof_selected_track2, 0);	//Disable tech view for the second piano roll's track if applicable

		temp_type = eof_note_type;					//Remember the active difficulty
		temp_track = eof_selected_track;			//Remember the active track number
		temp_pos = eof_music_pos;					//Remember the active position
		temp_selected = eof_selection.current;		//Remember the selected note
		temp_hover = eof_hover_note;				//Remember the hover note
		eof_selection.current = EOF_MAX_NOTES - 1;	//Clear the selected note
		eof_hover_note = -1;						//Clear the hover note

		if(!eof_sync_piano_rolls)
		{	//If the secondary piano roll is tracking its own position
			eof_music_pos = eof_music_pos2;	//Change to that position
		}
		(void) eof_menu_track_selected_track_number(eof_selected_track2, 0);	//Change to the track of the secondary piano roll, update coordinates, color set, etc.
		eof_note_type = eof_note_type2;									//Set the secondary piano roll's difficulty
		eof_render_editor_window(eof_window_editor2, 2);				//Render this track difficulty to the secondary piano roll screen

		(void) eof_menu_track_selected_track_number(temp_track, 0);	//Restore the active track number
		eof_note_type = temp_type;									//Restore the active difficulty
		eof_music_pos = temp_pos;									//Restore the active position
		eof_selection.current = temp_selected;						//Restore the selected note
		eof_hover_note = temp_hover;								//Restore the hover note
		eof_process_beat_statistics(eof_song, eof_selected_track);	//Rebuild the beat stats to reflect the primary piano roll so edit operations work as expected
		eof_menu_track_set_tech_view_state(eof_song, eof_selected_track2, restore_tech_view);	//Re-enable tech view for the second piano roll's track if applicable
	}//If the secondary piano roll is to be displayed
}

unsigned long eof_determine_piano_roll_left_edge(void)
{
//	eof_log("eof_determine_piano_roll_left_edge() entered");

	unsigned long pos = eof_music_pos / eof_zoom;

	if(pos <= 320)
	{
		return 0;
	}

	//Return the lowest timestamp that would be displayable given the current seek position and zoom level,
	return ((pos - 320) * eof_zoom);
}

unsigned long eof_determine_piano_roll_right_edge(void)
{
//	eof_log("eof_determine_piano_roll_right_edge() entered", 1);

	unsigned long pos = eof_music_pos / eof_zoom;

	if(pos < 300)
	{
		return ((eof_window_editor->screen->w - 1) - 20) * eof_zoom;
	}

	return ((eof_window_editor->screen->w - 1) - 320 + pos) * eof_zoom;
}

void eof_render_editor_window_common(EOF_WINDOW *window)
{
//	eof_log("eof_render_editor_window_common() entered");

	unsigned long i, j, ctr, notepos, markerlength;
	int pos = eof_music_pos / eof_zoom;	//Current seek position
	int lpos;							//The position of the first beatmarker
	int pmin = 0;
	int psec = 0;
	int xcoord;							//Used to cache x coordinate to reduce recalculations
	int col,col2;						//Temporary color variables
	unsigned long start;				//Will store the timestamp of the left visible edge of the piano roll
	unsigned long stop;					//Will store the timestamp of the right visible edge of the piano roll
	unsigned long numlanes;				//The number of fretboard lanes that will be rendered
	unsigned long numsections = 0;		//Used to abstract the solo sections
	EOF_PHRASE_SECTION *sectionptr = NULL;	//Used to abstract sections
	unsigned long bitmask, usedlanes;
	long notelength;
	unsigned long msec, roundedstart;
	double current_bpm;
	int bcol, bscol, bhcol;
	char buffer[16] = {0};
	char *ksname;
	char capo = 0;		//Is set to nonzero if a capo position was rendered, causing the first second marker to not be rendered

	if(!eof_song_loaded || !window)
		return;

	numlanes = eof_count_track_lanes(eof_song, eof_selected_track);
	if(eof_track_is_legacy_guitar(eof_song, eof_selected_track) && !eof_open_strum_enabled(eof_selected_track))
	{	//Special case:  Legacy guitar tracks can use a sixth lane but hide that lane if it is not enabled
		numlanes = 5;
	}
	eof_set_2D_lane_positions(eof_selected_track);	//Update the ychart[] array

	if(eof_display_second_piano_roll)
	{	//If the secondary piano roll is being displayed
		eof_process_beat_statistics(eof_song, eof_selected_track);	//Rebuild the beat stats so that the primary and secondary piano rolls can each display the correct RS phrases and sections
	}

	/* draw the starting position */
	if(pos < 300)
	{
		lpos = 20;
	}
	else
	{
		lpos = 20 - (pos - 300);
	}

	start = eof_determine_piano_roll_left_edge();	//Find the timestamp of the left visible edge of the piano roll
	stop = eof_determine_piano_roll_right_edge();	//Find the timestamp of the right visible edge of the piano roll

	/* fill in window background color */
	rectfill(window->screen, 0, 25 + 8, window->w - 1, window->h - 1, eof_color_gray);

	/* draw the playback controls */
	if(window != eof_window_editor2)
	{	//Only draw these controls for the main piano roll
		if(eof_selected_control < 0)
		{
			draw_sprite(eof_screen, eof_image[EOF_IMAGE_CONTROLS_BASE], eof_screen_layout.controls_x, 22 + 8);
		}
		else
		{
			draw_sprite(eof_screen, eof_image[EOF_IMAGE_CONTROLS_0 + eof_selected_control], eof_screen_layout.controls_x, 22 + 8);
		}
		textprintf_ex(eof_screen, eof_mono_font, eof_screen_layout.controls_x + 153, 23 + 8, eof_color_white, -1, "%02lu:%02lu", ((eof_music_pos - eof_av_delay) / 1000) / 60, ((eof_music_pos - eof_av_delay) / 1000) % 60);
	}

	/* draw fretboard area */
	rectfill(window->screen, 0, EOF_EDITOR_RENDER_OFFSET + 25, window->w - 1, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 1, eof_color_black);

	/* draw solo sections */
	if(eof_selected_track != EOF_TRACK_VOCALS)
	{	//If the vocal track is not active
		numsections = eof_get_num_solos(eof_song, eof_selected_track);
		for(i = 0; i < numsections; i++)
		{	//For each solo section in the track
			sectionptr = eof_get_solo(eof_song, eof_selected_track, i);	//Obtain the information for this legacy/pro guitar solo
			if(sectionptr != NULL)
			{
				if((sectionptr->end_pos >= start) && (sectionptr->start_pos <= stop))	//If the solo section would render between the left and right edges of the piano roll
					rectfill(window->screen, lpos + sectionptr->start_pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 25, lpos + sectionptr->end_pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 1, eof_color_dark_blue);
			}
		}

	/* draw SP sections */
		numsections = eof_get_num_star_power_paths(eof_song, eof_selected_track);
		for(i = 0; i < numsections; i++)
		{	//For each solo section in the track
			sectionptr = eof_get_star_power_path(eof_song, eof_selected_track, i);	//Obtain the information for this star power section
			if(sectionptr != NULL)
			{
				if((sectionptr->end_pos >= start) && (sectionptr->start_pos <= stop))	//If the star power section would render between the left and right edges of the piano roll, render a silver rectangle from the top most lane to the top of the fretboard area
					rectfill(window->screen, lpos + sectionptr->start_pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 25, lpos + sectionptr->end_pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[0], eof_color_silver);
			}
		}

		if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If a pro guitar/bass track is active
			unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
			EOF_PRO_GUITAR_TRACK *tp = eof_song->pro_guitar_track[tracknum];

	/* draw track tuning */
			if((pos <= 320) && !eof_legacy_view)
			{	//If the area left of the first beat marker is visible and legacy view is not in effect
				int notenum;

				for(i = 0; i < EOF_TUNING_LENGTH; i++)
				{	//For each usable string in the track
					if(i < tp->numstrings)
					{	//If this string is used by the track
						notenum = eof_lookup_tuned_note(tp, eof_selected_track, i, tp->tuning[i]);	//Look up the open note this string plays
						notenum %= 12;	//Ensure the value is in the range of [0,11]
						textprintf_ex(window->screen, eof_font, lpos - 17, EOF_EDITOR_RENDER_OFFSET + 8 + ychart[i], eof_color_white, -1, "%s", eof_note_names[notenum]);	//Draw the tuning
					}
				}
			}

	/* draw arpeggio sections */
			for(i = 0; i < eof_song->pro_guitar_track[tracknum]->arpeggios; i++)
			{	//For each arpeggio section in the track
				sectionptr = &eof_song->pro_guitar_track[tracknum]->arpeggio[i];
				if((sectionptr->end_pos >= start) && (sectionptr->start_pos <= stop) && ((sectionptr->difficulty == eof_note_type) || (sectionptr->difficulty == 0xFF)))
				{	//If the arpeggio section would render between the left and right edges of the piano roll, and the section applies to the active difficulty, fill the bottom lane with turquoise
					int arpeggiocolor = eof_color_turquoise;	//Normal arpeggio phrases will render in turquoise

					if(sectionptr->flags & EOF_RS_ARP_HANDSHAPE)
					{	//If this arpeggio is configured to export as a normal handshape
						arpeggiocolor = eof_color_lighter_blue;
					}
					rectfill(window->screen, lpos + sectionptr->start_pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[numlanes - 2], lpos + sectionptr->end_pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[numlanes - 1], arpeggiocolor);
				}
			}

	/* draw undefined legacy mask markers */
			if(eof_legacy_view)
			{	//If legacy view is in effect
				int markerpos;
				col = makecol(176, 48, 96);	//Store maroon color
				for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
				{	//For each note in this track
					notepos = eof_get_note_pos(eof_song, eof_selected_track, i);
					notelength = eof_get_note_length(eof_song, eof_selected_track, i);
					if((eof_note_type != eof_get_note_type(eof_song, eof_selected_track, i)) || (notepos + notelength < start) || (notepos > stop))
						continue;	//If this note isn't in the active difficulty, or would render before the left edge of the piano roll or after the right edge, skip it

					if(eof_song->pro_guitar_track[tracknum]->note[i]->legacymask)
						continue;	//If this note has a defined legacy mask, skip it

					//Otherwise render a maroon colored section a minimum of eof_screen_layout.note_size pixels long
					markerlength = notelength / eof_zoom;
					if(markerlength < eof_screen_layout.note_size)
					{	//If this marker isn't at least as wide as a note gem
						markerlength = eof_screen_layout.note_size;	//Make it longer
					}
					markerpos = lpos + (notepos / eof_zoom);
					if(notepos + notelength >= start)
					{	//If the notes ends at or right of the left edge of the screen
						if(markerpos <= window->screen->w)
						{	//If the marker starts at or left of the right edge of the screen (is visible)
							rectfill(window->screen, markerpos, EOF_EDITOR_RENDER_OFFSET + 25, markerpos + markerlength, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 1, col);
						}
						else
						{	//Otherwise this and all remaining undefined legacy mask markers are not visible
							break;	//Stop rendering them
						}
					}
				}
			}

	/* draw capo position */
			if(tp->capo)
			{	//If the track uses a capo
				if(pos <= 320)
				{	//If the area left of the first beat marker is visible
					textprintf_ex(window->screen, eof_mono_font, lpos - 19, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 3, eof_color_yellow, -1, "C=%d", tp->capo);
					capo = 1;
				}
			}
		}//If a pro guitar/bass track is active
		else
		{	//If a non pro guitar/bass track is active
			/* draw slider sections */
			if(eof_song->track[eof_selected_track]->track_behavior == EOF_GUITAR_TRACK_BEHAVIOR)
			{	//If this is a legacy guitar track
				numsections = eof_get_num_sliders(eof_song, eof_selected_track);
				for(i = 0; i < numsections; i++)
				{	//For each slider section in the track
					sectionptr = eof_get_slider(eof_song, eof_selected_track, i);	//Obtain the information for this slider section
					if(sectionptr != NULL)
					{
						if((sectionptr->end_pos >= start) && (sectionptr->start_pos <= stop))	//If the slider section would render between the left and right edges of the piano roll, render a dark purple rectangle above the fretboard area
							rectfill(window->screen, lpos + sectionptr->start_pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15, lpos + sectionptr->end_pos / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 5 + eof_screen_layout.note_y[0], eof_color_dark_purple);
					}
				}
			}
		}//If a non pro guitar/bass track is active
	}//If the vocal track is not active

	/* draw highlight markers */
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in this track
		int markerpos;
		notepos = eof_get_note_pos(eof_song, eof_selected_track, i);
		notelength = eof_get_note_length(eof_song, eof_selected_track, i);
		if(notepos > stop)
		{	//If the note would render beyond the right edge of the piano roll
			break;	//This note and all remaining notes are too far ahead to render on-screen
		}
		if((eof_note_type != eof_get_note_type(eof_song, eof_selected_track, i)) || (notepos + notelength < start))
			continue;	//If this note is not in the active difficulty or would render before the left edge of the piano roll, skip it

		if(!eof_note_is_highlighted(eof_song, eof_selected_track, i))
			continue;	//If this note is not flagged to be highlighted, skip it

		//Otherwise render a yellow colored background
		markerlength = notelength / eof_zoom;
		if(markerlength < eof_screen_layout.note_size)
		{	//If this marker isn't at least as wide as a note gem
			markerlength = eof_screen_layout.note_size;	//Make it longer
		}
		markerpos = lpos + (notepos / eof_zoom);
		if(notepos + notelength >= start)
		{	//If the notes ends at or right of the left edge of the screen
			if(markerpos <= window->screen->w)
			{	//If the marker starts at or left of the right edge of the screen (is visible)
				rectfill(window->screen, markerpos, EOF_EDITOR_RENDER_OFFSET + 25, markerpos + markerlength, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 1, eof_color_yellow);
			}
			else
			{	//Otherwise this and all remaining highlighted notes are not visible
				break;	//Stop rendering them
			}
		}
	}

	/* draw seek selection */
	if(eof_seek_selection_start != eof_seek_selection_end)
	{	//If there is a seek selection
		if((eof_seek_selection_end >= start) && (eof_seek_selection_start <= stop))	//If the selection would render between the left and right edges of the piano roll, render a red rectangle from the top most lane to the bottom most lane
			rectfill(window->screen, lpos + eof_seek_selection_start / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[0], lpos + eof_seek_selection_end / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[numlanes - 1], eof_color_red);
	}

	/* draw start/end point marking */
	if((eof_song->tags->start_point != ULONG_MAX) && (eof_song->tags->end_point != ULONG_MAX) && (eof_song->tags->start_point != eof_song->tags->end_point))
	{	//If both the start and end points are defined with different timestamps
		if((eof_song->tags->end_point >= start) && (eof_song->tags->start_point <= stop))	//If the start/end marker would render between the left and right edges of the piano roll, render a light gray rectangle beneath the beat markers
			rectfill(window->screen, lpos + eof_song->tags->start_point / eof_zoom, EOF_EDITOR_RENDER_OFFSET - 4, lpos + eof_song->tags->end_point / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 25, eof_color_light_red);
	}

	/* draw trill and tremolo sections */
	if(eof_get_num_trills(eof_song, eof_selected_track) || eof_get_num_tremolos(eof_song, eof_selected_track))
	{	//If this track has any trill or tremolo sections
		int half_string_space = eof_screen_layout.string_space / 2;
		for(j = 0; j < 2; j++)
		{	//For each of the two phrase types (trills and tremolos)
			if(j == 0)
			{	//On the first pass, render trill sections
				numsections = eof_get_num_trills(eof_song, eof_selected_track);
			}
			else
			{	//On the second pass, render tremolo sections
				numsections = eof_get_num_tremolos(eof_song, eof_selected_track);
			}
			for(i = 0; i < numsections; i++)
			{	//For each trill or tremolo section in the track
				if(j == 0)
				{	//On the first pass, render trill sections
					sectionptr = eof_get_trill(eof_song, eof_selected_track, i);
				}
				else
				{	//On the second pass, render tremolo sections
					sectionptr = eof_get_tremolo(eof_song, eof_selected_track, i);
				}
				if(sectionptr == NULL)
					continue;	//If the section does not exist, skip it

				if((sectionptr->end_pos < start) || (sectionptr->start_pos > stop))
					continue;	//If the trill or tremolo section would render outside the left and right edges of the piano roll, skip it

				if(j)
				{	//If tremolo sections are being rendered
					if(eof_song->track[eof_selected_track]->flags & EOF_TRACK_FLAG_UNLIMITED_DIFFS)
					{	//If the track's difficulty limit has been removed
						if(sectionptr->difficulty != eof_note_type)	//And the tremolo section does not apply to the active track difficulty
							continue;	//Skip rendering it
					}
					else
					{
						if(sectionptr->difficulty != 0xFF)	//Otherwise if the tremolo section does not apply to all track difficulties
							continue;	//Skip rendering it
					}
				}
				usedlanes = eof_get_used_lanes(eof_selected_track, sectionptr->start_pos, sectionptr->end_pos, eof_note_type);	//Determine which lane(s) use this phrase
				if(usedlanes == 0)
				{	//If there are no notes in this marker, render the marker in all lanes
					usedlanes = 0xFF;
				}
				for(ctr = 0, bitmask = 1; ctr < numlanes; ctr++, bitmask <<= 1)
				{	//For each of the track's usable lanes
					if(usedlanes & bitmask)
					{	//If this lane is used in the phrase
						int x1 = lpos + sectionptr->start_pos / eof_zoom;
						int y1 = EOF_EDITOR_RENDER_OFFSET + 15 + ychart[ctr] - half_string_space;
						int x2 = lpos + sectionptr->end_pos / eof_zoom;
						int y2 = EOF_EDITOR_RENDER_OFFSET + 15 + ychart[ctr] + half_string_space;

						if(y1 < EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[0])
							y1 = EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[0];	//Ensure that the phrase cannot render above the top most lane
						if(y2 > EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[numlanes-1])
							y2 = EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[numlanes-1];	//Ensure that the phrase cannot render below the bottom most lane
						rectfill(window->screen, x1, y1, x2, y2, eof_colors[ctr].lightcolor);	//Draw a rectangle one lane high centered over that lane's fret line
					}
				}
			}//For each trill or tremolo section in the track
		}//For each of the two phrase types (trills and tremolos)
	}//If this track has any trill or tremolo sections

	if(window != eof_window_editor2)
	{	//Only draw the graphs for the main piano roll
		if(eof_display_spectrogram)
			(void) eof_render_spectrogram(eof_spectrogram);

		if(eof_display_waveform)
			(void) eof_render_waveform(eof_waveform);
	}

	/* draw fretboard area */
	for(i = 0; i < numlanes; i++)
	{
		if(!i || (i + 1 >= numlanes))
		{	//Ensure the top and bottom lines extend to the left of the piano roll
			hline(window->screen, lpos, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[i], lpos + (eof_chart_length) / eof_zoom, eof_color_white);
		}
		else if(eof_selected_track != EOF_TRACK_VOCALS)
		{	//Otherwise, if not drawing the vocal editor, draw the other fret lines from the first beat marker to the end of the chart
			hline(window->screen, lpos + eof_song->tags->ogg[eof_selected_ogg].midi_offset / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[i], lpos + (eof_chart_length) / eof_zoom, eof_color_white);
		}
	}
	vline(window->screen, lpos + (eof_chart_length) / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 35, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 11, eof_color_white);

	/* draw second markers */
	roundedstart = start / 1000;
	roundedstart *= 1000;		//Roundedstart is start rounded down to nearest second
	for(msec = roundedstart; msec < stop + 1000; msec += 1000)
	{	//Draw up to 1 second beyond the right edge of the screen's worth of second markers
		if(msec >= eof_chart_length)
			continue;	//If this millisecond is beyond the end of the chart, skip it

		for(j = 0; j < 1000; j+=100)
		{	//Draw markers every 100ms (1/10 second)
			xcoord = lpos + (msec + j) / eof_zoom;
			if(xcoord > window->screen->w)	//If this and all remaining second markers would render out of view
				break;
			if(xcoord < 0)
				continue;	//If this second marker would not be visible, skip it

			vline(window->screen, xcoord, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 9, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 1, eof_color_gray);
			if(j == 0)
			{	//Each second marker is drawn taller
				if(!capo)
				{	//If this second marker isn't being skipped to avoid rendering on top of the capo indicator
					vline(window->screen, xcoord, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h + 5, eof_color_white);
				}
				capo = 0;	//All second markers after the first will render regardless of whether the capo indicator is shown
			}
			else
			{	//Each 1/10 second marker is drawn shorter
				vline(window->screen, xcoord, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h + 1, eof_color_white);
			}
		}
		if(!eof_display_seek_pos_in_seconds)
		{	//If the seek position is to be displayed as minutes:seconds
			pmin = msec / 60000;		//Find minute count of this second marker
			psec = (msec % 60000)/1000;	//Find second count of this second marker
			textprintf_ex(window->screen, eof_mono_font, lpos + (msec / eof_zoom) - 16, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h + 6, eof_color_white, -1, "%02d:%02d", pmin, psec);
		}
		else
		{	//If the seek position is to be displayed as seconds
			psec = msec/1000;	//Find second count of this second marker
			textprintf_centre_ex(window->screen, eof_mono_font, lpos + (msec / eof_zoom), EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h + 6, eof_color_white, -1, "%ds", psec);
		}
	}
	vline(window->screen, lpos, EOF_EDITOR_RENDER_OFFSET + 35, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 10, eof_color_white);

	/* draw beat lines */
	bcol = makecol(128, 128, 128);
	bscol = eof_color_white;
	bhcol = eof_color_green;
	col = makecol(112, 112, 112);	//Cache this color
	col2 = eof_color_dark_silver;	//Cache this color

	for(i = 0; i < eof_song->beats; i++)
	{	//For each beat
		xcoord = lpos + eof_song->beat[i]->pos / eof_zoom;
		if(xcoord >= window->screen->w)
		{	//If this beat would render further right than the right edge of the screen
			break;	//Skip rendering this and all other beat markers, which would continue to render off screen
		}
		else if(xcoord >= 0)
		{	//The beat would render visibly
			int beatlinecol;

			if(i == eof_selected_beat)
			{	//Draw selected beat's tempo
				current_bpm = 60000000.0 / (double)eof_song->beat[i]->ppqn;
				textprintf_ex(window->screen, eof_mono_font, xcoord - 28, EOF_EDITOR_RENDER_OFFSET + 7 - (i % 2 == 0 ? 0 : 10), i == eof_hover_beat ? bhcol : i == eof_selected_beat ? bscol : bcol, -1, "<%03.2f>", current_bpm);
			}
			else if(eof_song->beat[i]->contains_tempo_change)
			{	//Draw tempo if this beat has a tempo change
				current_bpm = 60000000.0 / (double)eof_song->beat[i]->ppqn;
				textprintf_ex(window->screen, eof_mono_font, xcoord - 20, EOF_EDITOR_RENDER_OFFSET + 7 - (i % 2 == 0 ? 0 : 10), i == eof_hover_beat ? bhcol : i == eof_selected_beat ? bscol : bcol, -1, "%03.2f", current_bpm);
			}
			else
			{	//Draw beat arrow
				textprintf_ex(window->screen, eof_mono_font, xcoord - 20 + 9, EOF_EDITOR_RENDER_OFFSET + 6 - (i % 2 == 0 ? 0 : 10), i == eof_hover_beat ? bhcol : i == eof_selected_beat ? bscol : bcol, -1, "-->");
			}
			if(eof_song->beat[i]->has_ts && eof_get_ts_text(i, buffer))
			{	//If this beat has a time signature, regardless of whether it is different from the one in effect
				textprintf_centre_ex(window->screen, eof_mono_font, xcoord - 20 + 19, EOF_EDITOR_RENDER_OFFSET - 4 - (i % 2 == 0 ? 0 : 10), i == eof_hover_beat ? bhcol : i == eof_selected_beat ? bscol : bcol, -1, "(%s)", buffer);
			}

			//Only render vertical lines if the x position is visible on-screen, otherwise the entire line will not be visible anyway
			if(i == eof_hover_beat)
			{	//If this beat is hovered over
				beatlinecol = bhcol;	//Render the beat line in green
			}
			else if((eof_song->beat[i]->contained_rs_section_event >= 0) || (eof_song->beat[i]->contained_section_event >= 0))
			{	//If this beat contains a RS section or RS phrase
				beatlinecol = eof_color_red;	//Render the beat line in red
			}
			else
			{	//Otherwise render it in gray (if it's not the start of a measure) or in white
				beatlinecol = (eof_song->beat[i]->has_ts && (eof_song->beat[i]->beat_within_measure == 0)) ? eof_color_white : col;
			}
			vline(window->screen, xcoord, EOF_EDITOR_RENDER_OFFSET + 35 + 1, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 10 - 1, beatlinecol);
			vline(window->screen, xcoord, EOF_EDITOR_RENDER_OFFSET + 25, EOF_EDITOR_RENDER_OFFSET + 34, eof_color_gray);
			if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_ANCHOR)
			{
				vline(window->screen, xcoord, EOF_EDITOR_RENDER_OFFSET + (i % 2 == 0 ? 19 : 9), EOF_EDITOR_RENDER_OFFSET + 24, eof_color_red);
			}
			else
			{
				vline(window->screen, xcoord, EOF_EDITOR_RENDER_OFFSET + (i % 2 == 0 ? 19 : 9), EOF_EDITOR_RENDER_OFFSET + 24, (eof_song->beat[i]->has_ts && (eof_song->beat[i]->beat_within_measure == 0)) ? eof_color_white : col2);
			}
		}//The beat would render visibly

		//Render section names and event markers
		if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_EVENTS)
		{	//If this beat has any text events
			line(window->screen, xcoord - 3, EOF_EDITOR_RENDER_OFFSET + 24, xcoord + 3, EOF_EDITOR_RENDER_OFFSET + 24, eof_color_yellow);
			if((eof_2d_render_top_option == 6) || (eof_2d_render_top_option == 9))
			{	//If the user has opted to render section names (Rocksmith phrases) at the top of the 2D window, either by themselves or in combination with RS sections
				char *string = "%s";	//If the phrase isn't track specific, it will display normally
				char *string2 = "*%s";	//Otherwise it will be prefixed by an asterisk
				char *ptr = string;
				if(eof_song->beat[i]->contained_section_event >= 0)
				{	//If this beat has a section (RS phrase) event
					int bg_color = eof_color_gray;	//By default, section names will render with a gray background

					if((eof_write_rs_files || eof_write_rs2_files) && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
					{	//If the user wants to save Rocksmith capable files, and a pro guitar/bass track is active, determine if the section (Rocksmith phrase) is identical in both this and the previous difficulty or if the phrase is fully leveled in the active difficulty
						unsigned long i2, startpos, endpos;
						char retval;
						for(i2 = i + 1; i2 < eof_song->beats; i2++)
						{	//For each remaining beat
							if((eof_song->beat[i2]->contained_section_event < 0) && (i2 + 1 < eof_song->beats))
								continue;	//If this beat does not have a section event (RS phrase) and isn't the last beat of the chart, skip it

							//Otherwise it marks the end of any current phrase and the potential start of another
							startpos = eof_song->beat[i]->pos;		//The outer loop is tracking the phrase being processed
							endpos = eof_song->beat[i2]->pos - 1;	//Track this as the end position of the previous phrase marker
							retval = eof_compare_time_range_with_previous_or_next_difficulty(eof_song, eof_selected_track, startpos, endpos, eof_note_type, 1);	//Compare the phrase's content between this difficulty and the next
							if(retval < 0)
							{	//If the phrase was empty in this difficulty, render the section name with a blue background
								bg_color = eof_color_blue;
							}
							else if(eof_note_type < 255)
							{	//If there's a higher difficulty than the active difficulty
								if(!eof_time_range_is_populated(eof_song, eof_selected_track, startpos, endpos, eof_note_type + 1))
								{	//If the next difficulty is empty, render the section name with a green background
									bg_color = eof_color_dark_green;
								}
								else if(!retval)
								{	//If this phrase is identical among this difficulty and the next, render the section name with a red background
									bg_color = eof_color_red;
								}
							}
							break;
						}
					}
					if(eof_song->text_event[eof_song->beat[i]->contained_section_event]->track != 0)
					{	//If this RS phrase is track specific
						ptr = string2;
					}
					if(eof_2d_render_top_option == 6)
					{	//If the RS phrases are being rendered by themselves, place them at the top of the piano roll
						textprintf_ex(window->screen, eof_font, xcoord - 6, 30, eof_color_yellow, bg_color, ptr, eof_song->text_event[eof_song->beat[i]->contained_section_event]->text);	//Display it
					}
					else
					{	//Otherwise draw them them one line below the absolute top
						textprintf_ex(window->screen, eof_font, xcoord - 6, 30 + 12, eof_color_yellow, bg_color, ptr, eof_song->text_event[eof_song->beat[i]->contained_section_event]->text);	//Display it
					}
				}
				else if(eof_song->beat[i]->contains_end_event)
				{	//Or if this beat contains an end event
					textprintf_ex(window->screen, eof_font, xcoord - 6, 25 + 5, eof_color_red, eof_color_black, "[end]");	//Display it
				}
			}
			if((eof_2d_render_top_option == 8) || (eof_2d_render_top_option == 9))
			{	//If the user has opted to render Rocksmith sections at the top of the 2D window, either by themselves or in combination with RS phrases
				char *string = "%s %d";		//If the RS section isn't track specific, it will display normally
				char *string2 = "*%s %d";	//Otherwise it will be prefixed by an asterisk
				char *ptr = string;
				if(eof_song->beat[i]->contained_rs_section_event >= 0)
				{	//If this beat has a Rocksmith section, display the section name and instance number
					if(eof_song->text_event[eof_song->beat[i]->contained_rs_section_event]->track != 0)
					{	//If this RS section is track specific
						ptr = string2;
					}
					textprintf_ex(window->screen, eof_font, xcoord - 6, 25 + 5, eof_color_white, -1, ptr, eof_song->text_event[eof_song->beat[i]->contained_rs_section_event]->text, eof_song->beat[i]->contained_rs_section_event_instance_number);
				}
			}
		}//If this beat has any text events

		if(eof_song->beat[i]->has_ts && (eof_song->beat[i]->beat_within_measure == 0))
		{	//If this is a measure marker, draw the measure number to the right of the beat line
			textprintf_ex(window->screen, eof_mono_font, xcoord + 2, EOF_EDITOR_RENDER_OFFSET + 22 - 7, eof_color_yellow, -1, "%lu", eof_song->beat[i]->measurenum);
		}
		if(eof_song->beat[i]->flags & EOF_BEAT_FLAG_ANCHOR)
		{	//Draw anchor marker
			line(window->screen, xcoord - 3, EOF_EDITOR_RENDER_OFFSET + 21, xcoord, EOF_EDITOR_RENDER_OFFSET + 24, eof_color_red);
			line(window->screen, xcoord + 3, EOF_EDITOR_RENDER_OFFSET + 21, xcoord, EOF_EDITOR_RENDER_OFFSET + 24, eof_color_red);
		}
		ksname = eof_get_key_signature(eof_song, i, 0, 0);
		if(ksname)
		{	//If this beat has a key signature defined, draw the key signature above the timestamp
			textprintf_ex(window->screen, eof_mono_font, xcoord + 2, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 2, eof_color_yellow, -1, "%s", ksname);
		}
	}//For each beat

	/* draw grid lines */
	if(eof_render_grid_lines)
	{	//If the rendering of grid lines is enabled
		unsigned long gridpos = start;	//Begin with the timestamp of the visible left edge of the piano roll

		if(gridpos < eof_song->beat[0]->pos)
		{	//If the left edge of the piano roll is before the first beat marker's position
			gridpos = eof_song->beat[0]->pos;	//Start from there
		}
		gridpos = eof_next_grid_snap(gridpos);	//Find the first grid snap from that timestamp
		while((gridpos > 0) && (gridpos < stop))
		{	//If a grid snap position was identified and it renders before the right edge of the screen
			unsigned long lastgridpos = gridpos;
			xcoord = lpos + gridpos / eof_zoom;
			vline(window->screen, xcoord, EOF_EDITOR_RENDER_OFFSET + 35 + 1, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 10 - 1, eof_color_yellow);
			gridpos = eof_next_grid_snap(gridpos);	//Find the next grid snap
			if(gridpos == lastgridpos)
			{	//If the grid snap logic couldn't find another grid snap position
				break;
			}
		}
	}

	/* draw the bookmark position */
	for(i = 0; i < EOF_MAX_BOOKMARK_ENTRIES; i++)
	{
		if(eof_song->bookmark_pos[i] > 0)	//If this bookmark exists
		{
			vline(window->screen, lpos + eof_song->bookmark_pos[i] / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 20, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h + 4, eof_color_light_blue);
		}
	}

	/* draw fret hand positions */
	if((eof_2d_render_top_option == 7) && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
	{	//If the user opted to render fret hand positions at the top of the 2D panel, and this is a pro guitar track
		unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
		EOF_PRO_GUITAR_TRACK *tp = eof_song->pro_guitar_track[tracknum];
		for(i = 0; i < tp->handpositions; i++)
		{	//For each fret hand position in the track
			if(tp->handposition[i].difficulty == eof_note_type)
			{	//If this fret hand position is in the active difficulty
				xcoord = lpos + tp->handposition[i].start_pos / eof_zoom;
				if(xcoord >= window->screen->w)
				{	//If this hand position would render further right than the right edge of the screen
					break;	//Skip rendering this and all other hand positions, which would continue to render off screen
				}
				if(xcoord >= -25)
				{	//If the hand position renders close enough to or after the left edge of the screen, consider it visible
					vline(window->screen, xcoord, 25 + 5 + 14, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 11, eof_color_red);
					textprintf_centre_ex(window->screen, eof_font, xcoord , 25 + 5, eof_color_red, eof_color_black, "%lu", tp->handposition[i].end_pos);	//Display it
				}
			}
		}
	}
}

void eof_render_editor_window_common2(EOF_WINDOW *window)
{
//	eof_log("eof_render_editor_window_common2() entered");

	int pos = eof_music_pos / eof_zoom;	//Current seek position compensated for zoom level
	int zoom = eof_av_delay / eof_zoom;	//AV delay compensated for zoom level
	unsigned long i;
	unsigned long selected_tab;
	int lpos;							//The position of the first beatmarker
	char *tab_name;
	char tab_text[32] = {0};			//Used to generate the string for the difficulty name/number, including the note/tech note population statuses
	int scroll_pos;
	EOF_PRO_GUITAR_TRACK *tp = NULL;
	unsigned char diffnum = 0, isdiff;
	int xcoord, ycoord, fgcol, bgcol;

	if(!eof_song_loaded || !window)
		return;

	if(pos < 300)
	{
		lpos = 20;
	}
	else
	{
		lpos = 20 - (pos - 300);
	}

	/* draw the end of song position if necessary*/
	if(eof_chart_length != eof_music_length)
	{
		vline(window->screen, lpos + (eof_music_length / eof_zoom), EOF_EDITOR_RENDER_OFFSET + 20, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h + 4, eof_color_red);
	}

	/* draw the current position */
	if(pos >= zoom)
	{
		vline(window->screen, lpos + (eof_music_pos - eof_av_delay) / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 25, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 1, eof_color_green);
	}

	/* draw the difficulty tabs (after the section names, which otherwise render a couple pixels over the tabs) */
	if(window != eof_window_editor2)
	{	//Only draw difficulty tabs for the main piano roll
		unsigned long numtabs = eof_get_number_displayed_tabs();	//Determine how many tabs are to be displayed

		//Draw a border between the piano roll and the difficulty tab Y boundaries of the editor
		hline(window->screen, 0, 32, window->w - 1, eof_color_black);
		hline(window->screen, 0, 31, window->w - 1, eof_color_gray3);
		hline(window->screen, 0, 30, window->w - 1, eof_color_light_gray);
		hline(window->screen, 0, 28, window->w - 1, eof_color_white);

		//Determine which tab is active (in the foreground)
		if(!(eof_song->track[eof_selected_track]->flags & EOF_TRACK_FLAG_UNLIMITED_DIFFS))
		{	//If the track's difficulty limit has not been removed
			selected_tab = eof_note_type;	//The active tab is the same as the active difficulty number
		}
		else
		{
			if(eof_note_type < numtabs / 2)
			{	//If any of the left-of-center tabs represent the active difficulty
				selected_tab = eof_note_type + 1;
			}
			else if(eof_note_type >= eof_song->track[eof_selected_track]->numdiffs - (numtabs / 2))
			{	//If any of the right-of-center tabs represent the active difficulty
				selected_tab = numtabs - (eof_song->track[eof_selected_track]->numdiffs - eof_note_type) - 1;
			}
			else
			{	//Otherwise the center tab represents the active difficulty
				selected_tab = numtabs / 2;
			}
		}
		if(selected_tab >= numtabs)
			selected_tab = 0;	//Bounds check

		//Render the tabs
		for(i = 0; i < numtabs; i++)
		{	//For each tab that needs to be rendered
			if(i != selected_tab)
			{	//Skip the active tab in this loop, it will render afterward
				draw_sprite(window->screen, eof_image[EOF_IMAGE_TAB_BG], 8 + (i * 80), 8);
			}
		}
		draw_sprite(window->screen, eof_image[EOF_IMAGE_TAB_FG], 8 + (selected_tab * 80) - 2, 8);	//The foreground tab is 4 pixels wider, so center it over the adjacent tabs

		//Draw difficulty tab text
		for(i = 0; i < numtabs; i++)
		{	//For each difficulty tab rendered
			int has_tech = 0;	//Set to nonzero if this difficulty contains tech notes that aren't shown (due to tech view not being enabled)

			isdiff = 1;	//Unless determined that the tab represents a change to lowest/highest difficulty (<< and >>), this condition is set
			if(eof_song->track[eof_selected_track]->flags & EOF_TRACK_FLAG_UNLIMITED_DIFFS)
			{	//If this track is not limited to 5 difficulties
				if(i == 0)
				{	//The first tab will be "<<"
					(void) snprintf(tab_text, sizeof(tab_text) - 1, "<<");
					isdiff = 0;
				}
				else if(i == numtabs - 1)
				{	//The last tab will be ">>"
					(void) snprintf(tab_text, sizeof(tab_text) - 1, ">>");
					isdiff = 0;
				}
				else
				{	//The other tabs will be rendered as difficulty numbers
					if(eof_note_type < numtabs / 2)
					{	//If the tabs represent the lowest difficulties
						diffnum = i - 1;
					}
					else if(eof_note_type >= eof_song->track[eof_selected_track]->numdiffs - (numtabs / 2))
					{	//If the tabs represent the highest difficulties
						diffnum = eof_song->track[eof_selected_track]->numdiffs - numtabs + 1 + i;
					}
					else
					{	//If the center tab represents the active difficulty
						diffnum = i + eof_note_type - (numtabs / 2);
					}
					(void) snprintf(tab_text, sizeof(tab_text) - 1, " %u", diffnum);
					if(eof_track_diff_populated_status[diffnum])
					{	//If this difficulty is populated
						tab_text[0] = '*';
					}
				}
			}//If this track is not limited to 5 difficulties
			else
			{	//The track difficulty is named
				if(eof_selected_track == EOF_TRACK_VOCALS)
				{
					tab_name = eof_vocal_tab_name[i];
				}
				else if(eof_selected_track == EOF_TRACK_DANCE)
				{
					tab_name = eof_dance_tab_name[i];
				}
				else
				{
					tab_name = eof_note_type_name[i];
				}
				(void) snprintf(tab_text, sizeof(tab_text) - 1, "%s", tab_name);	//Copy the difficulty name to the string
				diffnum = i;	//The difficulty number is the same as the tab number
			}
			if(eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
			{	//If a pro guitar track is being rendered
				tp = eof_song->pro_guitar_track[eof_song->track[eof_selected_track]->tracknum];
				if((tp->note != tp->technote) && isdiff && eof_track_diff_populated_tech_note_status[diffnum])
				{	//If tech view is NOT in effect, this tab represents a specific difficulty and there is at least one tech note in this difficulty
					(void) strncat(tab_text, "(*)", sizeof(tab_text) - strlen(tab_text) - 1);	//Append the tech notes populated indicator
					has_tech = 1;
				}
			}
			if(eof_track_diff_highlighted_status[diffnum])
			{	//If this difficulty has highlighting
				if(eof_song->track[eof_selected_track]->flags & EOF_TRACK_FLAG_UNLIMITED_DIFFS)
				{	//If the difficulty limit is removed
					if(i != numtabs - 1)
					{	//If this tab isn't being displayed as ">>"
						bgcol = eof_color_yellow;	//Render a yellow background behind the difficulty name
					}
					else
					{
						bgcol = -1;	//Otherwise use a transparent background
					}
				}
				else
				{
					bgcol = eof_color_yellow;	//Render a yellow background behind the difficulty name
				}
			}
			else
			{
				bgcol = -1;	//Otherwise use a transparent background
			}
			xcoord = 47 + i * 80;	//The center of the difficulty tab being rendered
			if(i == selected_tab)
			{
				ycoord = 2 + 8;
				fgcol = eof_color_black;
			}
			else
			{
				ycoord = 2 + 2 + 8;
				fgcol = makecol(128, 128, 128);
			}
			textprintf_centre_ex(window->screen, font, xcoord, ycoord, fgcol, bgcol, "%s", tab_text);
			if(has_tech)
			{	//If the tech notes indicator was drawn, it will need to be redrawn with the appropriate background color to indicate whether tech notes are highlighted
				if(eof_track_diff_highlighted_tech_note_status[diffnum])
				{	//If any of those tech notes are highlighted
					bgcol = eof_color_yellow;
				}
				else
				{
					bgcol = eof_color_light_gray;
				}
				textout_ex(window->screen, font, "(*)", xcoord + text_length(font, tab_text) / 2 - text_length(font, "(*)"), ycoord, fgcol, bgcol);
			}
			if((eof_selected_track == EOF_TRACK_VOCALS) && (i == eof_vocals_tab))
			{	//Break after  rendering the one difficulty tab name for the vocal track
				break;
			}
		}//For each difficulty tab rendered
	}//Only draw difficulty tabs for the main piano roll
	else
	{	//Otherwise display the secondary piano roll's track difficulty
		char temp[101] = {0};
		temp[0] = '\0';	//Empty the string
		eof_cat_track_difficulty_string(temp);	//Fill the string with the name and number of the track difficulty being rendered
		if(eof_track_diff_highlighted_status[eof_note_type2])
		{	//If any notes in the secondary piano roll's active tab have highlighting
			bgcol = eof_color_yellow;	//Render a yellow background behind the difficulty name
		}
		else
		{
			bgcol = -1;	//Otherwise use a transparent background
		}
		textprintf_ex(window->screen, font, 50, 2 + 2 + 8, makecol(128, 128, 128), bgcol, "%s", temp);
	}

	/* render the scroll bar */
	if(window != eof_window_editor2)
	{	//Only draw the scroll bar for the main piano roll
		scroll_pos = ((double)(eof_screen->w - 8.0) / (double)eof_chart_length) * (double)eof_music_pos;
		rectfill(window->screen, 0, eof_screen_layout.scrollbar_y, window->w - 1, window->h - 2, eof_color_light_gray);
		draw_sprite(window->screen, eof_image[EOF_IMAGE_SCROLL_HANDLE], scroll_pos + 2, eof_screen_layout.scrollbar_y);

		vline(window->screen, 0, 24 + 8, window->h + 4, eof_color_dark_silver);
		vline(window->screen, 1, 25 + 8, window->h + 4, eof_color_black);
		hline(window->screen, 0, window->h - 1, window->w - 1, eof_color_white);
	}
}

void eof_mark_new_note_as_cymbal(EOF_SONG *sp, unsigned long track, unsigned long notenum)
{	//Perform the eof_mark_edited_note_as_cymbal() logic, applying cymbal status to all relevant frets in the note
	eof_log("eof_mark_new_note_as_cymbal() entered", 1);
	eof_mark_edited_note_as_cymbal(sp,track,notenum,0xFFFF);
}

void eof_mark_edited_note_as_cymbal(EOF_SONG *sp, unsigned long track, unsigned long notenum, unsigned long bitmask)
{
	unsigned long tracknum;

	eof_log("eof_mark_edited_note_as_cymbal() entered", 1);

	if((sp == NULL) || (track >= sp->tracks) || !track)
		return;	//Invalid parameters
	tracknum = sp->track[track]->tracknum;

	if(sp->track[track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
	{	//For now, it's assumed that any drum behavior track will be a legacy format track
		if((notenum >= sp->legacy_track[tracknum]->notes) || (sp->legacy_track[tracknum]->note[notenum] == NULL))
			return;

		if((sp->legacy_track[tracknum]->note[notenum]->note & 4) && (bitmask & 4))
		{	//If the note was changed to include a yellow note
			eof_set_flags_at_legacy_note_pos(sp->legacy_track[tracknum],notenum,EOF_DRUM_NOTE_FLAG_Y_CYMBAL,1,0);	//Set the yellow cymbal flag on all drum notes at this position
		}
		if((sp->legacy_track[tracknum]->note[notenum]->note & 8) && (bitmask & 8))
		{	//If the note was changed to include a blue note
			eof_set_flags_at_legacy_note_pos(sp->legacy_track[tracknum],notenum,EOF_DRUM_NOTE_FLAG_B_CYMBAL,1,0);	//Set the blue cymbal flag on all drum notes at this position
		}
		if((sp->legacy_track[tracknum]->note[notenum]->note & 16) && (bitmask & 16))
		{	//If the note was changed to include a green note
			eof_set_flags_at_legacy_note_pos(sp->legacy_track[tracknum],notenum,EOF_DRUM_NOTE_FLAG_G_CYMBAL,1,0);	//Set the green cymbal flag on all drum notes at this position
		}
	}
}

void eof_mark_new_note_as_double_bass(EOF_SONG *sp, unsigned long track, unsigned long notenum)
{
	eof_log("eof_mark_new_note_as_double_bass() entered", 1);
	eof_mark_edited_note_as_double_bass(sp, track, notenum, 0xFFFF);
}

void eof_mark_edited_note_as_double_bass(EOF_SONG *sp, unsigned long track, unsigned long notenum, unsigned long bitmask)
{
	unsigned long tracknum;

	eof_log("eof_mark_edited_note_as_double_bass() entered", 1);

	if((sp == NULL) || (track >= sp->tracks) || !track)
		return;	//Invalid parameters
	tracknum = sp->track[track]->tracknum;

	if((sp->track[track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR) && (eof_get_note_type(sp, eof_selected_track, notenum) == EOF_NOTE_AMAZING))
	{	//If the note is an Expert drum note
		if((notenum >= sp->legacy_track[tracknum]->notes) || (sp->legacy_track[tracknum]->note[notenum] == NULL))
			return;

		//For now, it's assumed that any drum behavior track will be a legacy format track
		if((sp->legacy_track[tracknum]->note[notenum]->note & 1) && (bitmask & 1))
		{	//If the note was changed to include a bass drum note
			sp->legacy_track[tracknum]->note[notenum]->flags |= EOF_DRUM_NOTE_FLAG_DBASS;	//Set the double bass flag
		}
	}
}

void eof_mark_new_note_as_special_hi_hat(EOF_SONG *sp, unsigned long track, unsigned long notenum)
{
	eof_log("eof_mark_new_note_as_special_hi_hat() entered", 1);
	eof_mark_edited_note_as_special_hi_hat(sp, track, notenum, 0xFFFF);
}

void eof_mark_edited_note_as_special_hi_hat(EOF_SONG *sp, unsigned long track, unsigned long notenum, unsigned long bitmask)
{
	unsigned long tracknum, flags;

	eof_log("eof_mark_edited_note_as_special_hi_hat() entered", 1);

	if((sp == NULL) || (track >= sp->tracks) || !track)
		return;	//Invalid parameters
	tracknum = sp->track[track]->tracknum;

	if(sp->track[track]->track_type == EOF_TRACK_DRUM_PS)
	{	//If the PS drum track is active
		if((notenum >= sp->legacy_track[tracknum]->notes) || (sp->legacy_track[tracknum]->note[notenum] == NULL))
			return;

		if((sp->legacy_track[tracknum]->note[notenum]->note & 4) && (bitmask & 4))
		{	//If the note has lane 3 populated
			eof_set_flags_at_legacy_note_pos(sp->legacy_track[tracknum],notenum,EOF_DRUM_NOTE_FLAG_Y_CYMBAL,1,0);	//Set the yellow cymbal flag on all drum notes at this position

			if(eof_drum_modifiers_affect_all_difficulties)
			{	//If the user wants to apply this change to notes at this position among all difficulties
				eof_set_flags_at_legacy_note_pos(sp->legacy_track[tracknum],notenum,EOF_DRUM_NOTE_FLAG_Y_HI_HAT_OPEN,0,0);	//Clear open hi hat status on all drum notes at this position
				eof_set_flags_at_legacy_note_pos(sp->legacy_track[tracknum],notenum,EOF_DRUM_NOTE_FLAG_Y_HI_HAT_PEDAL,0,0);	//Clear pedal hi hat status on all drum notes at this position
				eof_set_flags_at_legacy_note_pos(sp->legacy_track[tracknum],notenum,EOF_DRUM_NOTE_FLAG_Y_SIZZLE,0,0);		//Clear sizzle hi hat status on all drum notes at this position
				eof_set_flags_at_legacy_note_pos(sp->legacy_track[tracknum],notenum,eof_mark_drums_as_hi_hat,1,0);			//Set the selected hi hat status on all drum notes at this position
			}
			else
			{	//Just apply it to the new note
				flags = eof_get_note_flags(sp, track, notenum);	//Get the note's flags
				flags &= ~EOF_DRUM_NOTE_FLAG_Y_HI_HAT_OPEN;		//Clear open hi hat status
				flags &= ~EOF_DRUM_NOTE_FLAG_Y_HI_HAT_PEDAL;	//Clear pedal hi hat status
				flags &= ~EOF_DRUM_NOTE_FLAG_Y_SIZZLE;			//Clear sizzle hi hat status
				flags |= eof_mark_drums_as_hi_hat;				//Set the appropriate hi hat status
				eof_set_note_flags(sp, track, notenum, flags);	//Apply the flag change
			}
		}
	}
}

unsigned char eof_find_pen_note_mask(void)
{
//	eof_log("eof_find_pen_note_mask() entered");

	unsigned long laneborder;
	unsigned long i;
	unsigned long lanecount;
	int eof_scaled_mouse_y = mouse_y;	//Rescaled mouse y coordinate that accounts for the x2 zoom display feature

	if(eof_screen_zoom)
	{	//If x2 zoom is in effect, take that into account for the mouse position
		eof_scaled_mouse_y = mouse_y / 2;
	}

	//Determine which lane the mouse is in
	eof_hover_piece = -1;
	lanecount = eof_count_track_lanes(eof_song, eof_selected_track);
	if(eof_track_is_legacy_guitar(eof_song, eof_selected_track) && !eof_open_strum_enabled(eof_selected_track))
	{	//Special case:  Legacy guitar tracks can use a sixth lane but hide that lane if it is not enabled
		lanecount = 5;
	}
	for(i = 0; i < lanecount; i++)
	{	//For each of the usable lanes
		laneborder = eof_window_editor->y + EOF_EDITOR_RENDER_OFFSET + 15 + 10 + eof_screen_layout.note_y[i];	//This represents the y position of the boundary between the current lane and the next
		if((eof_scaled_mouse_y < laneborder) && (eof_scaled_mouse_y > laneborder - eof_screen_layout.string_space))
		{	//If the mouse's Y position is below the boundary to the next lane but above the boundary to the previous lane
			eof_hover_piece = i;	//Store the lane number
		}
	}

	if(eof_hover_piece < 0)	//If no hover piece was found
		return 0;			//Return 0

	/* see if we are inverting the lanes */
	if(eof_inverted_notes || (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
	{	//If the user opted to invert the notes in the piano roll, or if the current track is a pro guitar/bass track, return the appropriate inverted pen mask
		return ((1 << (lanecount - 1)) >> (unsigned)eof_hover_piece);	//This finds the appropriate invert mask, where mousing over the bottom most lane activates lane 1 for the pen mask (cast to unsigned because splint is too stupid to take into account that it was already checked for negativity)
	}

	return (1 << (unsigned)eof_hover_piece);	//Return the normal pen mask, where mousing over the top most lane activates lane 1 for the pen mask
}

void eof_editor_logic_common(void)
{
//	eof_log("eof_editor_logic_common() entered");

	int pos = eof_music_pos / eof_zoom;
	int npos;
	unsigned long i;
	int eof_scaled_mouse_x = mouse_x, eof_scaled_mouse_y = mouse_y;	//Rescaled mouse coordinates that account for the x2 zoom display feature

	if(!eof_song_loaded)
		return;

	eof_hover_note = -1;
	eof_seek_hover_note = -1;
	eof_hover_note_2 = -1;
	eof_hover_lyric = -1;

	eof_mickey_z = eof_mouse_z - mouse_z;
	eof_mouse_z = mouse_z;

	if(eof_screen_zoom)
	{	//If x2 zoom is in effect, take that into account for the mouse position
		eof_scaled_mouse_x = mouse_x / 2;
		eof_scaled_mouse_y = mouse_y / 2;
	}

	if(eof_music_paused)
	{	//If the chart is paused
		/* mouse is in beat marker area */
		if((eof_scaled_mouse_y >= eof_window_editor->y + EOF_EDITOR_RENDER_OFFSET - 4 - 19) && (eof_scaled_mouse_y < eof_window_editor->y + EOF_EDITOR_RENDER_OFFSET + 18 + 8))
		{
			if(eof_blclick_released)
			{	//If the left mouse button is released
				for(i = 0; i < eof_song->beats; i++)
				{
					if(pos < 300)
					{
						npos = (20 + (eof_song->beat[i]->pos / eof_zoom));
					}
					else
					{
						npos = (20 - (pos - 300) + (eof_song->beat[i]->pos / eof_zoom));
					}
					if(eof_scaled_mouse_x <= npos - 16)
					{	//If the mouse is too far to the left of this (and all subsequent) beat markers
						break;	//Stop checking the beats because none of them could have been clicked on
					}
					else if(eof_scaled_mouse_x < npos + 16)
					{	//Otherwise if the mouse is within the left and right boundaries of this beat's click window
						eof_hover_beat = i;
						break;
					}
				}
			}
			if(!eof_full_screen_3d && (mouse_b & 1))
			{	//If full screen 3d is not in use and the left mouse button is held
				if(eof_mouse_drug)
				{
					eof_mouse_drug++;
					if(eof_mouse_drug == 11)
					{
						eof_mickeys_x = eof_scaled_mouse_x - eof_click_x;
					}
				}
				if((eof_mickeys_x != 0) && !eof_mouse_drug)
				{
					eof_mouse_drug++;
				}

				if(eof_blclick_released)
				{
					if(eof_beat_num_valid(eof_song, eof_hover_beat))
					{
						eof_select_beat(eof_hover_beat);
					}
					eof_blclick_released = 0;
					eof_click_x = eof_scaled_mouse_x;
					eof_mouse_drug = 0;
				}

				if(!eof_song->tags->click_drag_disabled)
				{	//If click and drag isn't disabled, check whether a beat marker is being moved
					if((eof_mouse_drug > 10) && !eof_blclick_released && (eof_selected_beat == 0) && (eof_mickeys_x != 0) && (eof_hover_beat == eof_selected_beat) && !((eof_mickeys_x * eof_zoom < 0) && (eof_song->beat[0]->pos == 0)))
					{	//If moving the first beat marker
						long rdiff = eof_mickeys_x * eof_zoom;

						if(!eof_undo_toggle)
						{
							eof_prepare_undo(EOF_UNDO_TYPE_NONE);
							eof_moving_anchor = 1;
							eof_last_midi_offset = eof_song->tags->ogg[eof_selected_ogg].midi_offset;
						}
						if((long)eof_song->beat[0]->pos + rdiff >= 0)
						{
							if(!KEY_EITHER_CTRL)
							{
								for(i = 0; i < eof_song->beats; i++)
								{
									eof_song->beat[i]->fpos += rdiff;
									eof_song->beat[i]->pos = eof_song->beat[i]->fpos + 0.5;	//Round up to nearest ms
								}
							}
							else
							{
								eof_song->beat[0]->fpos += rdiff;
								eof_song->beat[0]->pos = eof_song->beat[0]->fpos + 0.5;	//Round up to nearest ms
							}
						}
						eof_song->tags->ogg[eof_selected_ogg].midi_offset = eof_song->beat[0]->pos;
					}//If moving the first beat marker
					else if((eof_mouse_drug > 10) && !eof_blclick_released && (eof_beat_num_valid(eof_song, eof_selected_beat)) && (eof_mickeys_x != 0) && ((eof_beat_is_anchor(eof_song, eof_hover_beat) || eof_anchor_all_beats || (eof_moving_anchor && (eof_hover_beat == eof_selected_beat)))))
					{	//If moving a beat marker other than the first
						if(!eof_song->tags->tempo_map_locked)
						{	//If the tempo map is not locked
							if(!eof_undo_toggle)
							{
								eof_prepare_undo(EOF_UNDO_TYPE_NONE);
								eof_moving_anchor = 1;
								eof_last_midi_offset = eof_song->tags->ogg[eof_selected_ogg].midi_offset;
								eof_adjusted_anchor = 1;
								if((eof_note_auto_adjust && !KEY_EITHER_SHIFT) || (!eof_note_auto_adjust && KEY_EITHER_SHIFT))
								{
									if(KEY_EITHER_SHIFT)
									{
										eof_shift_used = 1;	//Track that the SHIFT key was used
									}
									(void) eof_menu_edit_cut(eof_selected_beat, 0);
								}
							}
							eof_song->beat[eof_selected_beat]->fpos += eof_mickeys_x * eof_zoom;
							eof_song->beat[eof_selected_beat]->pos = eof_song->beat[eof_selected_beat]->fpos + 0.5;	//Round up to nearest ms
							if(((eof_selected_beat > 0) && (eof_song->beat[eof_selected_beat]->pos <= eof_song->beat[eof_selected_beat - 1]->pos + 50)) || ((eof_selected_beat + 1 < eof_song->beats) && (eof_song->beat[eof_selected_beat]->pos >= eof_song->beat[eof_selected_beat + 1]->pos - 50)))
							{	//If the beat being drug was moved to within within 50ms of the previous/next beat marker, undo the move
								eof_song->beat[eof_selected_beat]->fpos -= eof_mickeys_x * eof_zoom;
								eof_song->beat[eof_selected_beat]->pos = eof_song->beat[eof_selected_beat]->fpos + 0.5;	//Round up to nearest ms
							}
							else
							{	//Update beat timings to reflect the beat being clicked and drug
								eof_recalculate_beats(eof_song, eof_selected_beat);
							}
							eof_song->beat[eof_selected_beat]->flags |= EOF_BEAT_FLAG_ANCHOR;
							eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current
						}//If the tempo map is not locked
					}//If moving a beat marker other than the first
				}//If click and drag isn't disabled, check whether a beat marker is being moved
			}//If full screen 3d is not in use and the left mouse button is held
			if(!(mouse_b & 1))
			{	//If the left mouse button is not held
				if(!eof_blclick_released)
				{	//If the release of the left mouse button has not been processed
					eof_blclick_released = 1;
					if(!eof_song->tags->click_drag_disabled && (!eof_song->tags->tempo_map_locked || (eof_selected_beat == 0)))
					{	//If click and drag is not disabled and either the tempo map is not locked or the first beat marker was manipulated, allow the marker to be moved
						if(eof_mouse_drug && (eof_song->tags->ogg[eof_selected_ogg].midi_offset != eof_last_midi_offset))
						{	//If the first beat marker's position has changed
							if((eof_note_auto_adjust && !KEY_EITHER_SHIFT) || (!eof_note_auto_adjust && KEY_EITHER_SHIFT))
							{	//Move all notes by the same amount that the first beat moved
								if(KEY_EITHER_SHIFT)
								{
									eof_shift_used = 1;	//Track that the SHIFT key was used
								}
								(void) eof_adjust_notes(eof_song->tags->ogg[eof_selected_ogg].midi_offset - eof_last_midi_offset);
							}
							eof_fixup_notes(eof_song);
						}
					}
				}
				if(eof_adjusted_anchor)
				{
					if((eof_note_auto_adjust && !KEY_EITHER_SHIFT) || (!eof_note_auto_adjust && KEY_EITHER_SHIFT))
					{	//Auto-adjust the notes after clicking, dragging and releasing a beat
						if(KEY_EITHER_SHIFT)
						{
							eof_shift_used = 1;	//Track that the SHIFT key was used
						}
						(void) eof_menu_edit_cut_paste(eof_selected_beat, 0);
						eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current
					}
				}
				eof_moving_anchor = 0;
				eof_mouse_drug = 0;
				eof_adjusted_anchor = 0;
			}//If the left mouse button is not held
			if(!eof_full_screen_3d && ((mouse_b & 2) || (eof_key_code == KEY_INSERT)) && eof_rclick_released && (eof_beat_num_valid(eof_song, eof_hover_beat)))
			{	//If full screen 3d is not in use and the right mouse key or insert are held over a beat marker
				eof_select_beat(eof_hover_beat);
				alogg_seek_abs_msecs_ogg_ul(eof_music_track, eof_song->beat[eof_hover_beat]->pos + eof_av_delay);
				eof_music_actual_pos = eof_song->beat[eof_hover_beat]->pos + eof_av_delay;
				eof_music_pos = eof_music_actual_pos;
				eof_reset_lyric_preview_lines();
				eof_rclick_released = 0;
			}
			if(!(mouse_b & 2) && !(eof_key_code == KEY_INSERT))
			{
				eof_rclick_released = 1;
			}
		}//mouse is in beat marker area
		else
		{
			eof_hover_beat = ULONG_MAX;
			eof_adjusted_anchor = 0;
		}

		/* handle scrollbar click */
		if((eof_scaled_mouse_y >= eof_window_editor->y + eof_window_editor->h - 17) && (eof_scaled_mouse_y < eof_window_editor->y + eof_window_editor->h))
		{
			if(!eof_full_screen_3d && (mouse_b & 1))
			{
				eof_music_actual_pos = ((double)eof_chart_length / (double)(eof_screen->w - 8.0)) * (double)(eof_scaled_mouse_x - 4.0);
				if(eof_music_actual_pos > eof_chart_length)
				{
					eof_music_actual_pos = eof_chart_length;
				}
				eof_set_seek_position(eof_music_actual_pos + eof_av_delay);
			}
		}

		/* handle initial SHIFT key press */
		if(KEY_EITHER_SHIFT && eof_shift_released)
		{	//If this is a new SHIFT keypress
			eof_shift_used = 0;	//Track that a SHIFT keyboard command hasn't been used yet
			eof_shift_released = 0;	//Track that SHIFT is now held down
		}

		/* handle initial SHIFT key release */
		else if(!KEY_EITHER_SHIFT && !eof_shift_released)
		{
			if(!eof_shift_used && (eof_input_mode == EOF_INPUT_FEEDBACK))
			{	//If the SHIFT key wasn't used for anything while it was held down
				eof_update_seek_selection(0, 0, 0);	//Clear the seek selection
			}
			eof_shift_released = 1;
		}
	}//If the chart is paused
	else
	{	//The chart is playing
		eof_feedback_input_mode_update_selected_beat();	//Update the selected beat and measure if Feedback input mode is in use
	}

	//Find hover notes/lyrics for chart and fret catalog playback
	if(eof_music_catalog_playback || !eof_music_paused)
	{	//If the chart or fret catalog is playing
		int examined_music_pos = eof_music_pos;		//By default, assume the chart position is to be used to find hover notes/etc.
		int examined_track = eof_selected_track;	//By default, assume the active track is to be used to find hover notes/etc.
		int examined_type = eof_note_type;			//By default, assume the active difficulty is to be used to find hover notes/etc.
		int examined_pos, zoom;

		if(eof_music_catalog_playback)
		{	//If the fret catalog is playing
			examined_music_pos = eof_music_catalog_pos;	//Use the fret catalog position instead
			examined_track = eof_song->catalog->entry[eof_selected_catalog_entry].track;	//Use the fret catalog entry's track instead
			examined_type = eof_song->catalog->entry[eof_selected_catalog_entry].type;	//Use the fret catalog entry's difficulty instead
		}

		//Find the hover note
		examined_pos = examined_music_pos / eof_zoom;
		zoom = eof_av_delay / eof_zoom;	//Cache this value
		for(i = 0; i < eof_get_track_size(eof_song, examined_track); i++)
		{
			if(eof_get_note_type(eof_song, examined_track, i) == examined_type)
			{
				npos = eof_get_note_pos(eof_song, examined_track, i) / eof_zoom;
				if((examined_pos - zoom > npos) && (examined_pos - zoom < npos + (eof_get_note_length(eof_song, examined_track, i) > 100 ? eof_get_note_length(eof_song, examined_track, i) : 100) / eof_zoom))
				{
					eof_hover_note = i;
					if(eof_vocals_selected)
					{	//If the lyric track is active, set the lyric pen note
						eof_pen_lyric.note = eof_get_note_note(eof_song, examined_track, i);
					}
					break;
				}
			}
		}

		//Find the hover 2 note
		for(i = 0; i < eof_get_track_size(eof_song, examined_track); i++)
		{
			if(eof_get_note_type(eof_song, examined_track, i) == examined_type)
			{
				npos = eof_get_note_pos(eof_song, examined_track, i) + eof_av_delay;
				if((examined_music_pos > npos) && (examined_music_pos < npos + (eof_get_note_length(eof_song, examined_track, i) > 100 ? eof_get_note_length(eof_song, examined_track, i) : 100)))
				{
					eof_hover_note_2 = i;
					break;
				}
			}
		}

		//Find the hover lyric, if there is one
		for(i = 0; i < eof_song->vocal_track[0]->lyrics; i++)
		{	//Find the hover lyric if there is one
			npos = eof_song->vocal_track[0]->lyric[i]->pos;
			if((examined_music_pos - eof_av_delay > npos) && (examined_music_pos - eof_av_delay < npos + (eof_song->vocal_track[0]->lyric[i]->length > 100 ? eof_song->vocal_track[0]->lyric[i]->length : 100)))
			{
				eof_hover_lyric = i;
				break;
			}
		}
	}

	/* handle playback controls */
	if((eof_scaled_mouse_x >= eof_screen_layout.controls_x) && (eof_scaled_mouse_x < eof_screen_layout.controls_x + 139) && (eof_scaled_mouse_y >= 22 + 8) && (eof_scaled_mouse_y < 22 + 17 + 8))
	{
		if(!eof_full_screen_3d && (mouse_b & 1))
		{
			eof_selected_control = (eof_scaled_mouse_x - eof_screen_layout.controls_x) / 30;
			eof_blclick_released = 0;
			if(eof_selected_control == 1)
			{
				eof_music_rewind();
			}
			else if(eof_selected_control == 3)
			{
				eof_music_forward();
			}
		}
		if(!(mouse_b & 1))
		{
			if(!eof_blclick_released)
			{
				eof_blclick_released = 1;
				if(eof_selected_control == 0)
				{
					(void) eof_menu_song_seek_start();
				}
				else if(eof_selected_control == 2)
				{
					eof_music_play(0);
				}
				else if(eof_selected_control == 4)
				{
					(void) eof_menu_song_seek_end();
				}
				eof_selected_control = -1;
			}
		}
	}
	else
	{
		eof_selected_control = -1;
	}
}

void eof_seek_to_nearest_grid_snap(void)
{
	unsigned long beat;

	if(!eof_song || (eof_snap_mode == EOF_SNAP_OFF) || (eof_music_pos <= eof_song->beat[0]->pos) || (eof_music_pos >= eof_song->beat[eof_song->beats - 1]->pos))
		return;	//Return if there's no song loaded, grid snap is off, or the seek position is outside the range of beats in the chart

	beat = eof_get_beat(eof_song, eof_music_pos);	//Find which beat the current seek position is in
	if(!eof_beat_num_valid(eof_song, beat))	//If the seek position is outside the chart
		return;

	if(eof_music_pos == eof_song->beat[beat]->pos)
		return;	//If the current position is on a beat marker, do not seek anywhere

	//Find the distance to the previous grid snap
	eof_snap_logic(&eof_tail_snap, eof_music_pos - eof_av_delay);
	eof_set_seek_position(eof_tail_snap.pos + eof_av_delay);	//Seek to the nearest grid snap
}

long eof_find_hover_note(long targetpos, int x_tolerance, char snaplogic)
{
	unsigned long i, npos, leftboundary, hoverlane = 0;
	long nlen;
	if(targetpos < 0)
	{
		targetpos = 0;
	}
	if(snaplogic)
	{
		eof_snap_logic(&eof_snap, targetpos);
		eof_snap_length_logic(&eof_snap);
		eof_pen_note.pos = eof_snap.pos;
		eof_pen_visible = 1;
	}
	if(eof_note_tails_clickable)
	{	//If the user enabled the preference to include note tails in the clickable area for notes
		hoverlane = eof_find_pen_note_mask();	//Find which lane the mouse is currently hovering over
	}
	for(i = 0; (i < eof_get_track_size(eof_song, eof_selected_track)); i++)
	{	//For each note in the active track, until a hover note is found
		if(eof_get_note_type(eof_song, eof_selected_track, i) != eof_note_type)
			continue;	//If the note is not in the active difficulty, skip it

		npos = eof_get_note_pos(eof_song, eof_selected_track, i);
		if(npos < x_tolerance)
		{	//Avoid an underflow here
			leftboundary = 0;
		}
		else
		{
			leftboundary = npos - x_tolerance;
		}
		if(eof_note_tails_clickable && (eof_vocals_selected || (hoverlane & eof_get_note_note(eof_song, eof_selected_track, i))))
		{	//If the user enabled the preference to include note tails in the clickable area for notes, and the vocal track is active or the mouse is hovering over a lane this note uses
			long next = eof_track_fixup_next_note(eof_song, eof_selected_track, i);	//Find the next note in the track difficulty

			nlen = eof_get_note_length(eof_song, eof_selected_track, i);
			if(nlen < 0)
			{	//If the note length was not retrievable
				nlen = 0;
			}
			else if(next > 0)
			{	//If there was a next note, ensure that the clickable area of the note is shortened to allow clickable space for that next note
				unsigned long nextpos = eof_get_note_pos(eof_song, eof_selected_track, next);

				if(npos + nlen + x_tolerance >= nextpos - x_tolerance)
				{	//If the note's tail extends into the clickable area of the next note
					if(nlen >= x_tolerance)
					{	//If the note length's clickable area can be reduced by the tolerance
						nlen -= x_tolerance;
					}
					else
					{	//Otherwise don't allow the note's tail to be clickable because it's too close to the next note
						nlen = 0;
					}
				}
			}
		}
		else
		{	//Otherwise only include the note head as the clickable area, including the specified tolerance
			nlen = 0;
		}
		if((targetpos >= leftboundary) && (targetpos <= npos + nlen + x_tolerance))
		{
			return i;
		}
		else if(snaplogic && ((eof_pen_note.pos >= npos - x_tolerance) && (eof_pen_note.pos <= npos + nlen + x_tolerance)))
		{	//If the position wasn't close enough to a note, but snaplogic is enabled, check the position's closest grid snap
			return i;
		}
	}
	return -1;	//No appropriate hover note found
}

void eof_update_seek_selection(unsigned long start, unsigned long stop, char deselect)
{
	unsigned long t1 = start, t2 = start, i, notepos;
	char first = 1;

//Sort the start and stop time in chronological order
	eof_seek_selection_start = start;
	eof_seek_selection_end = stop;
	if(stop < start)
	{	//If the stop time is earlier
		t1 = stop;
	}
	else
	{	//Otherwise the start time is earlier
		t2 = stop;
	}

//Clear the selection array
	memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
	if(start == stop)
	{	//If the seek selection is removed
		eof_selection.current = EOF_MAX_NOTES - 1;
		eof_selection.current_pos = 0;
	}
	if(deselect)
	{	//If the calling function did not want to go any further than deselecting notes
		return;
	}

//Update the selection
	eof_selection.track = eof_selected_track;
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{
		notepos = eof_get_note_pos(eof_song, eof_selected_track, i);
		if((eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type) && (notepos >= t1) && (notepos <= t2))
		{	//If the note is within the seek selection
			if(first)
			{
				eof_selection.range_pos_1 = notepos;
				first = 0;
			}
			eof_selection.range_pos_2 = notepos;
			eof_selection.current = i;
			eof_selection.current_pos = notepos;
			eof_selection.multi[i] = 1;
			eof_undo_last_type = EOF_UNDO_TYPE_NONE;
		}
	}
}

void eof_feedback_input_mode_update_selected_beat(void)
{
	static unsigned long lastbeat = ULONG_MAX;
	if(eof_input_mode == EOF_INPUT_FEEDBACK)
	{	//If feedback input mode is in use
		unsigned long beat;
		unsigned long adjustedpos = eof_music_pos - eof_av_delay;	//Find the actual chart position
		beat = eof_get_beat(eof_song, adjustedpos);
		if(beat != lastbeat)
		{	//If the seek position is at a different beat than the last call
			if(eof_beat_num_valid(eof_song, beat))
			{	//If the seek position is within the chart
				eof_select_beat(beat);	//Set eof_selected_beat and eof_selected_measure
			}
			lastbeat = beat;
		}
	}
}

unsigned char eof_set_active_difficulty(unsigned char diff)
{
	unsigned char eof_note_type_max = EOF_NOTE_AMAZING;	//By default, assume this track has 4 usable difficulties

	if(eof_song->track[eof_selected_track]->flags & EOF_TRACK_FLAG_UNLIMITED_DIFFS)
	{	//If this track is not limited to 5 difficulties
		eof_note_type_max = eof_song->track[eof_selected_track]->numdiffs - 1;	//All the track's usable difficulties (numbered starting from 0) can be tabbed to
	}
	else if(eof_selected_track == EOF_TRACK_DANCE)
	{
		eof_note_type_max = EOF_NOTE_CHALLENGE;	//However, the dance track has 5 usable difficulties
	}
	else if(eof_selected_track == EOF_TRACK_VOCALS)
	{	//The vocal track only has 1 usable difficulty
		eof_note_type_max = EOF_NOTE_SUPAEASY;
	}

	if(diff <= eof_note_type_max)
	{	//If the specified track difficulty is valid
		eof_note_type = diff;	//Make it the active difficulty
	}
	eof_fix_window_title();
	return eof_note_type_max;
}

unsigned long eof_get_number_displayed_tabs(void)
{
	unsigned long numtabs = 5;	//By default, 5 difficulty tabs display for a track

	if(!eof_song || (eof_selected_track >= eof_song->tracks))
		return 0;	//Error

	//Determine how many tabs to draw
	if(eof_selected_track == EOF_TRACK_VOCALS)
	{	//If a vocal track is active
		numtabs = 1;
	}
	else if(eof_song->track[eof_selected_track]->flags & EOF_TRACK_FLAG_UNLIMITED_DIFFS)
	{	//If the track's difficulty limit has been removed
		//The tabs begin rendering 8 pixels from the left edge of the screen, end where the playback controls are rendered and need
		// to allow two more pixels since the foreground tab is 2 pixels wider than background tabs
		numtabs = (eof_screen_layout.controls_x - 10) / 80;	//Find the maximum number of displayable tabs
		if(numtabs > (unsigned long)eof_song->track[eof_selected_track]->numdiffs + 2)
		{	//Only allow enough tabs to display the difficulties in use, plus an additional tab each for << and >>
			numtabs = eof_song->track[eof_selected_track]->numdiffs + 2;
		}
	}

	return numtabs;
}
