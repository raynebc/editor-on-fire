#include "main.h"
#include "song.h"
#include "beat.h"
#include "chart_import.h"
#include "feedback.h"

/* convert Feedback chart time to milliseconds for use with EOF */
/*static double chartpos_to_msec(struct FeedbackChart * chart, unsigned long chartpos)
{
	unsigned long ppqn = 500000;
	double curpos = chart->offset;
	unsigned long lastchartpos = 0;
	double beat_length = (double)60000 / ((double)60000000.0 / (double)ppqn);
	double convert = beat_length / (double)chart->resolution; // current conversion rate of chartpos to milliseconds
	struct dBAnchor * current_anchor = chart->anchors;
	while(current_anchor)
	{
		// find current BPM
		if(current_anchor->BPM > 0)
		{
			ppqn = (double)60000000.0 / ((double)current_anchor->BPM / 1000.0);
			beat_length = (double)60000 / ((double)60000000.0 / (double)ppqn);
			convert = beat_length / (double)chart->resolution;
		}
		if(chartpos > current_anchor->chartpos)
		{
			curpos += (double)(current_anchor->chartpos - lastchartpos) * convert;
			lastchartpos = current_anchor->chartpos;
		}
		else
		{
			curpos += (double)(chartpos - lastchartpos) * convert;
			break;
		}
		current_anchor = current_anchor->next;
	}
	return curpos;
}
*/

static double chartpos_to_msec(struct FeedbackChart * chart, unsigned long chartpos)
{
	struct dBAnchor *targetanchor,*conductor;
	double anchortime=0.0;
	double tickduration=0.0;

	if((chart == NULL) || (chart->anchors == NULL))
		return 0.0;	//Return error

//Check to make sure that the chart begins with a tempo event at chart position 0, otherwise it's not a valid chart?
	if(chart->anchors->chartpos != 0)
		return 0.0;	//Return error

//Find the anchor that is closest to the chart position immediately before chartpos
	targetanchor=chart->anchors;
	for(conductor=chart->anchors;conductor != NULL;conductor=conductor->next)	//For each anchor
	{
		if(conductor->next != NULL)						//If there's another anchor that follows
			if(conductor->next->chartpos <= chartpos)	//If the next anchor starts no later than the target chart position
			targetanchor=conductor->next;	//Note it
	}
	anchortime=targetanchor->usec / 1000.0;	//Store the time of this anchor

//Find the duration of one tick
	tickduration=(60000.0 / (targetanchor->BPM / 1000.0) / chart->resolution);

//Return the timestamp of the anchor plus the cumulative duration of the remaining ticks
	return (anchortime + (tickduration * (chartpos - targetanchor->chartpos)));
}

EOF_SONG * eof_import_chart(const char * fn)
{

	struct FeedbackChart * chart = NULL;
	EOF_SONG * sp = NULL;
	int err;

	chart = ImportFeedback((char *)fn, &err);
	if(chart)
	{
		/* load audio */
		if(!eof_load_ogg(chart->audiofile))
		{
			DestroyFeedbackChart(chart, 1);
			return NULL;
		}
		eof_music_length = alogg_get_length_msecs_ogg(eof_music_track);
		eof_music_actual_length = eof_music_length;

		/* create empty song */
		sp = eof_create_song();
		if(!sp)
		{
			DestroyFeedbackChart(chart, 1);
			return NULL;
		}

		/* copy tags */
		if(chart->name)
		{
			strcpy(sp->tags->title, chart->name);
			printf("%s\n", sp->tags->title);
		}
		if(chart->artist)
		{
			strcpy(sp->tags->artist, chart->artist);
			printf("%s\n", sp->tags->artist);
		}
		if(chart->charter)
		{
			strcpy(sp->tags->frettist, chart->charter);
			printf("%s\n", sp->tags->frettist);
		}

		/* set up beat markers */
		sp->tags->ogg[0].midi_offset = chart->offset;
		struct dBAnchor * current_anchor = chart->anchors;
		unsigned long ppqn = 500000;
		double curpos = sp->tags->ogg[0].midi_offset;
		double beat_length;
		int new_anchor = 0;
		EOF_BEAT_MARKER * new_beat = NULL;
		printf("begin anchors\n");
		/* usec member can be 0, meaning anchor is really just a BPM change
		 * need to use chartpos to find beat marker position */
		while(current_anchor)
		{
//			if(current_anchor->usec > 0 || current_anchor->chartpos == 0)
			{
				curpos = chartpos_to_msec(chart, current_anchor->chartpos);
//				printf("usec = %lu\n", current_anchor->usec);
				new_anchor = 1;
				if(current_anchor->BPM > 0)
				{
					ppqn = (double)60000000.0 / ((double)current_anchor->BPM / 1000.0);
				}
				beat_length = (double)60000 / ((double)60000000.0 / (double)ppqn);
				while(curpos < current_anchor->usec / 1000 - 100)
				{
					new_beat = eof_song_add_beat(sp);
					if(new_beat)
					{
						new_beat->ppqn = ppqn;
						new_beat->pos = curpos;
						if(new_anchor)
						{
							new_beat->flags |= EOF_BEAT_FLAG_ANCHOR;
							new_anchor = 0;
						}
					}
					if(current_anchor->usec == 0)
					{
						break;
					}
					curpos += beat_length;
				}
			}
			current_anchor = current_anchor->next;
		}
		printf("end anchors\n");

		/* fill in notes */
		struct dbTrack * current_track = chart->tracks;
		int track;
		int difficulty;
		while(current_track)
		{

			/* convert track number to EOF numbering scheme */
			switch(current_track->tracktype)
			{

				/* PART GUITAR */
				case 1:
				{
					track = EOF_TRACK_GUITAR;
					break;
				}

				/* PART GUITAR COOP */
				case 2:
				{
					track = EOF_TRACK_GUITAR_COOP;
					break;
				}

				/* PART BASS */
				case 3:
				{
					track = EOF_TRACK_BASS;
					break;
				}

				/* PART DRUMS */
				case 4:
				{
					track = EOF_TRACK_DRUM;
					break;
				}

				default:
				{
					track = -1;
					break;
				}
			}
			difficulty = current_track->difftype - 1;

			/* if it is a valid track */
			if(track >= 0)
			{
				struct dbNotelist * current_note = current_track->notes;
				unsigned long lastpos = 0;
				EOF_NOTE * new_note = NULL;
				while(current_note)
				{
					if(current_note->chartpos != lastpos)
					{
						new_note = eof_track_add_note(sp->track[track]);
						if(new_note)
						{
							new_note->pos = chartpos_to_msec(chart, current_note->chartpos);
							new_note->length = chartpos_to_msec(chart, current_note->chartpos + current_note->duration) - new_note->pos;
							new_note->note = 1 << current_note->gemcolor;
							new_note->type = difficulty;
						}
					}
					else
					{
						if(new_note)
						{
							new_note->note |= (1 << current_note->gemcolor);
						}
					}
					lastpos = current_note->chartpos;
					current_note = current_note->next;
				}
			}
			current_track = current_track->next;
		}
	}
	printf("done\n");

	return sp;
}
