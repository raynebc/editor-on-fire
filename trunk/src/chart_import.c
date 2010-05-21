#include "main.h"
#include "song.h"
#include "beat.h"
#include "chart_import.h"
#include "feedback.h"

/* convert Feedback chart time to milliseconds for use with EOF */
static double chartpos_to_msec(struct FeedbackChart * chart, unsigned long chartpos)
{
	double curpos = chart->offset;
	unsigned long lastchartpos = 0;
	double beat_length = 500.0;
	int beat_count;
	double convert = beat_length / (double)chart->resolution; // current conversion rate of chartpos to milliseconds
	struct dBAnchor * current_anchor = chart->anchors;
	while(current_anchor)
	{
		/* find current BPM */
		if(current_anchor->BPM > 0)
		{
			beat_length = (double)60000000.0 / (double)current_anchor->BPM;
			
			/* adjust BPM if next beat marker is an anchor */
			if(current_anchor->next && current_anchor->next->usec > 0)
			{
				beat_count = (current_anchor->next->chartpos - current_anchor->chartpos) / chart->resolution;
				beat_length = (((double)current_anchor->next->usec / 1000.0) - curpos) / (double)beat_count;
			}
			
			convert = beat_length / (double)chart->resolution;
		}
		
		/* if the specified chartpos is past the next anchor, add the total time between
		 * the anchors */
		if(current_anchor->next && chartpos >= current_anchor->next->chartpos)
		{
			curpos += (double)(current_anchor->next->chartpos - current_anchor->chartpos) * convert;
			lastchartpos = current_anchor->next->chartpos;
		}
		
		/* otherwise add the time from the current anchor to the specified chartpos */
		else
		{
			curpos += (double)(chartpos - lastchartpos) * convert;
			break;
		}
		current_anchor = current_anchor->next;
	}
	return curpos;
}

EOF_SONG * eof_import_chart(const char * fn)
{

	struct FeedbackChart * chart = NULL;
	EOF_SONG * sp = NULL;
	int err;
	char oggfn[1024] = {0};
	
	chart = ImportFeedback((char *)fn, &err);
	if(chart)
	{
		/* load audio */
		if(strlen(chart->audiofile) > 0)
		{
			replace_filename(oggfn, fn, chart->audiofile, 1024);
		}
		else
		{
			strcpy(oggfn, "");
		}
		if(!eof_load_ogg(oggfn))
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
		}
		if(chart->artist)
		{
			strcpy(sp->tags->artist, chart->artist);
		}
		if(chart->charter)
		{
			strcpy(sp->tags->frettist, chart->charter);
		}
		
		/* set up beat markers */
		sp->tags->ogg[0].midi_offset = chart->offset;
		struct dBAnchor * current_anchor = chart->anchors;
		unsigned long ppqn = 500000;
		double curpos = sp->tags->ogg[0].midi_offset;
		double beat_length = 500.0;
		double bpm = 120.0;
		int beat_count;
		int i;
		unsigned long lastbpm = 120000;
		int new_anchor = 0;
		EOF_BEAT_MARKER * new_beat = NULL;
		/* usec member can be 0, meaning anchor is really just a BPM change
		 * need to use chartpos to find beat marker position */
		while(current_anchor)
		{
			new_anchor = 1;
			if(current_anchor->BPM > 0)
			{
				ppqn = (double)60000000.0 / ((double)current_anchor->BPM / 1000.0);
				beat_length = (double)60000 / ((double)60000000.0 / (double)ppqn);
				/* adjust BPM if next beat marker is an anchor */
				if(current_anchor->next && current_anchor->next->usec > 0)
				{
					beat_count = (current_anchor->next->chartpos - current_anchor->chartpos) / chart->resolution;
					beat_length = (((double)current_anchor->next->usec / 1000.0) - curpos) / (double)beat_count;
					bpm = (double)60000.0 / beat_length;
					ppqn = (double)60000000.0 / bpm;
				}
			}
			
			/* if there is another anchor, fill in beats up to that anchor */
			if(current_anchor->next)
			{
				for(i = 0; i < (current_anchor->next->chartpos - current_anchor->chartpos) / chart->resolution; i++)
				{
					new_beat = eof_song_add_beat(sp);
					if(new_beat)
					{
						new_beat->ppqn = ppqn;
						new_beat->fpos = curpos;
						new_beat->pos = new_beat->fpos;
						if(new_anchor && lastbpm != current_anchor->BPM)
						{
							new_beat->flags |= EOF_BEAT_FLAG_ANCHOR;
							new_anchor = 0;
						}
					}
					curpos += beat_length;
				}
			}
			
			lastbpm = current_anchor->BPM;
			current_anchor = current_anchor->next;
		}
		
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
	return sp;
}
