#include "main.h"
#include "song.h"
#include "beat.h"
#include "chart_import.h"
#include "feedback.h"
#include "menu/beat.h"

/* convert Feedback chart time to milliseconds for use with EOF */
static double chartpos_to_msec(struct FeedbackChart * chart, unsigned long chartpos)
{
	double offset = chart->offset * 1000.0;
	double curpos = offset;
	unsigned long lastchartpos = 0;
	double beat_length = 500.0;
	double beat_count;
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
				beat_count = (double)(current_anchor->next->chartpos - current_anchor->chartpos) / (double)chart->resolution;
				beat_length = (((double)current_anchor->next->usec / 1000.0 + offset) - curpos) / beat_count;
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
	int i;
	char oggfn[1024] = {0};
	char searchpath[1024] = {0};
	char oldoggpath[1024] = {0};
	struct al_ffblk info; // for file search
	
	chart = ImportFeedback((char *)fn, &err);
	if(chart)
	{
		/* load audio */
		replace_filename(oggfn, fn, "guitar.ogg", 1024);
		
		/* if "guitar.ogg" doesn't exist, look for any OGG file in the chart directory */
		if(!exists(oggfn))
		{
			
			/* no OGG file found, start file selector at chart directory */
			replace_filename(searchpath, fn, "*.ogg", 1024);
			if(al_findfirst(searchpath, &info, FA_ALL))
			{
				ustrcpy(oldoggpath, eof_last_ogg_path);
				replace_filename(eof_last_ogg_path, fn, "", 1024);
			}
			
			/* if there is only one OGG file, load it */
			else if(al_findnext(&info))
			{
				replace_filename(oggfn, fn, info.name, 1024);
			}
			al_findclose(&info);
		}
		if(!eof_load_ogg(oggfn))
		{
			DestroyFeedbackChart(chart, 1);
			ustrcpy(eof_last_ogg_path, oldoggpath); // remember previous OGG directory if we fail
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
		sp->tags->ogg[0].midi_offset = chart->offset * 1000.0;
		struct dBAnchor * current_anchor = chart->anchors;
		unsigned long max_chartpos = 0;
		
		/* find the highest chartpos for beat markers */
		while(current_anchor)
		{
			if(current_anchor->chartpos > max_chartpos)
			{
				max_chartpos = current_anchor->chartpos;
			}
			current_anchor = current_anchor->next;
		}
		
		/* create beat markers */
		EOF_BEAT_MARKER * new_beat = NULL;
		for(i = 0; i < max_chartpos / chart->resolution; i++)
		{
			new_beat = eof_song_add_beat(sp);
			if(new_beat)
			{
				new_beat->fpos = chartpos_to_msec(chart, chart->resolution * i);
				new_beat->pos = new_beat->fpos;
				new_beat->flags |= EOF_BEAT_FLAG_ANCHOR;
			}
		}
		
		/* nudge beat markers so we get the correct BPM calculations */
		for(i = 1; i < sp->beats; i++)
		{
			eof_mickeys_x = 1;
			eof_recalculate_beats(sp, i);
			eof_mickeys_x = -1;
			eof_recalculate_beats(sp, i);
		}
		
		/* unanchor non-anchor beat markers */
		for(i = 1; i < sp->beats; i++)
		{
			if(sp->beat[i]->ppqn == sp->beat[i - 1]->ppqn)
			{
				sp->beat[i]->flags ^= EOF_BEAT_FLAG_ANCHOR;
			}
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
				unsigned long lastpos = -1;
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
