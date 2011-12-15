#include "main.h"
#include "song.h"
#include "beat.h"
#include "chart_import.h"
#include "event.h"
#include "feedback.h"
#include "midi.h"	//For eof_apply_ts()
#include "menu/beat.h"
#include "menu/file.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

/* convert Feedback chart time to milliseconds for use with EOF */
static double chartpos_to_msec(struct FeedbackChart * chart, unsigned long chartpos)
{
//	eof_log("chartpos_to_msec() entered");

	if(!chart)
	{
		return 0;
	}
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
			if(current_anchor->next && (current_anchor->next->usec > 0))
			{
				beat_count = (double)(current_anchor->next->chartpos - current_anchor->chartpos) / (double)chart->resolution;
				beat_length = (((double)current_anchor->next->usec / 1000.0 + offset) - curpos) / beat_count;
			}

			convert = beat_length / (double)chart->resolution;
		}

		/* if the specified chartpos is past the next anchor, add the total time between
		 * the anchors */
		if(current_anchor->next && (chartpos >= current_anchor->next->chartpos))
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
	eof_log("\tImporting Feedback chart", 1);
	eof_log("eof_import_chart() entered", 1);

	struct FeedbackChart * chart = NULL;
	EOF_SONG * sp = NULL;
	int err=0;
	int i=0;
	char oggfn[1024] = {0};
	char searchpath[1024] = {0};
	char oldoggpath[1024] = {0};
	char errorcode[100] = "Import failed.  Error #";
	struct al_ffblk info; // for file search
	int ret=0;

	if(!fn)
	{
		return NULL;
	}
	eof_log_level &= ~2;	//Disable verbose logging
	chart = ImportFeedback((char *)fn, &err);
	if(chart == NULL)
	{
		snprintf(&errorcode[23],50,"%d",err);	//Perform a bounds checked conversion of Allegro's error code to string format
		alert(errorcode, NULL, errorcode, "OK", NULL, 0, KEY_ENTER);
		return NULL;
	}
	else
	{
		/* load audio */
		replace_filename(oggfn, fn, "guitar.ogg", 1024);	//Look for guitar.ogg by default
		if((chart->audiofile != NULL) && !exists(oggfn))
		{	//If the imported chart defines which audio file to use AND guitar.ogg doesn't exist
			replace_filename(oggfn, fn, chart->audiofile, 1024);
			if(!exists(oggfn))	//If the file doesn't exist in the chart's parent directory
				replace_filename(oggfn, fn, "guitar.ogg", 1024);	//Look for guitar.ogg instead
		}

		/* if the audio file doesn't exist, look for any OGG file in the chart directory */
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

		replace_filename(searchpath, oggfn, "", 1024);		//Store the path of the file's parent folder
		ret = eof_mp3_to_ogg(oggfn,searchpath);				//Create guitar.ogg in the folder
		if(ret != 0)										//If guitar.ogg was not created successfully
			return NULL;
		replace_filename(oggfn, oggfn, "guitar.ogg", 1024);	//guitar.ogg is the expected file

		if(!eof_load_ogg(oggfn))
		{
			DestroyFeedbackChart(chart, 1);
			ustrcpy(eof_last_ogg_path, oldoggpath); // remember previous OGG directory if we fail
			return NULL;
		}
		eof_music_length = alogg_get_length_msecs_ogg(eof_music_track);
		eof_music_actual_length = eof_music_length;

		/* create empty song */
		sp = eof_create_song_populated();
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
		struct dbText * current_event = chart->events;
		unsigned long max_chartpos = 0;
		unsigned long final_bpm = 120000;

		/* find the highest chartpos for beat markers */
		while(current_anchor)
		{
			if(current_anchor->chartpos > max_chartpos)
			{
				max_chartpos = current_anchor->chartpos;
			}

			if(current_anchor->BPM > 0)
			{	//If this is a valid tempo
				final_bpm = current_anchor->BPM;	//store it so that the final tempo can be remembered
			}

			current_anchor = current_anchor->next;
		}
		while(current_event)
		{
			if(current_event->chartpos > max_chartpos)
			{
				max_chartpos = current_event->chartpos;
			}
			current_event = current_event->next;
		}

		/* create beat markers */
		EOF_BEAT_MARKER * new_beat = NULL;
		unsigned long maxbeat = max_chartpos / chart->resolution;	//Determine how many beats must be created to encompass all tempo changes/text events
		if(max_chartpos % chart->resolution)
			maxbeat++;
		for(i = 0; i <= maxbeat; i++)
		{
			new_beat = eof_song_add_beat(sp);
			if(new_beat)
			{
				new_beat->fpos = chartpos_to_msec(chart, chart->resolution * i);
				new_beat->pos = new_beat->fpos + 0.5;
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
		sp->beat[sp->beats - 1]->ppqn = (double)60000000.0 / ((double)final_bpm / (double)1000.0);

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

				/* PART VOCALS */
				case 5:
				{
					track = EOF_TRACK_VOCALS;
					break;
				}

				/* PART RHYTHM */
				case 6:
				{
					track = EOF_TRACK_RHYTHM;
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
				struct dbNote * current_note = current_track->notes;
				unsigned long lastpos = -1;
				EOF_NOTE * new_note = NULL;
				unsigned long tracknum = sp->track[track]->tracknum;

				while(current_note)
				{
					/* import star power */
					if(current_note->gemcolor == '2')
					{
						eof_legacy_track_add_star_power(sp->legacy_track[tracknum], chartpos_to_msec(chart, current_note->chartpos), chartpos_to_msec(chart, current_note->chartpos + current_note->duration));
					}

					/* skip face-off sections for now */
					else if((current_note->gemcolor == '0') || (current_note->gemcolor == '1'))
					{
					}

					/* skip unknown section marker */
					else if(current_note->gemcolor == '3')
					{
					}

					/* import regular note */
					else
					{
						if(current_note->chartpos != lastpos)
						{
							new_note = eof_legacy_track_add_note(sp->legacy_track[tracknum]);
							if(new_note)
							{
								new_note->pos = chartpos_to_msec(chart, current_note->chartpos);
								new_note->length = chartpos_to_msec(chart, current_note->chartpos + current_note->duration) - (double)new_note->pos + 0.5;	//Round up
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
					}
					lastpos = current_note->chartpos;
					current_note = current_note->next;
				}
			}
			current_track = current_track->next;
		}

		/* load text events */
		long b;
		current_event = chart->events;
		while(current_event)
		{
			b = current_event->chartpos / chart->resolution;
			if(b >= sp->beats)
			{
				b = sp->beats - 1;
			}
			eof_song_add_text_event(sp, b, current_event->text, 0, 0);
			current_event = current_event->next;
		}

		/* load time signatures */
		if(eof_use_ts)
		{	//If the user opted to import TS changes
			current_anchor = chart->anchors;
			eof_apply_ts(4,4,0,sp,0);	//Apply a default TS of 4/4 on the first beat marker
			while(current_anchor)
			{
				if((current_anchor->TS != 0) && (current_anchor->chartpos % chart->resolution == 0))
				{	//If there is a Time Signature defined here, and it is defined on a beat marker
					if(current_anchor->chartpos / chart->resolution < sp->beats)
					{	//And the beat in question is defined in the beat[] array, apply the TS change
						eof_apply_ts(current_anchor->TS,4,current_anchor->chartpos / chart->resolution,sp,0);
					}
				}
				current_anchor = current_anchor->next;
			}
		}

		DestroyFeedbackChart(chart, 1);	//Free memory used by Feedback chart before exiting function
	}
	eof_selected_ogg = 0;

	/* check if there were lane 5 drums imported */
	unsigned long ctr, tracknum;
	tracknum = sp->track[EOF_TRACK_DRUM]->tracknum;
	for(ctr = 0; ctr < sp->legacy_track[tracknum]->notes; ctr++)
	{	//For each note in the drum track
		if(sp->legacy_track[tracknum]->note[ctr]->note & 32)
		{	//If this note uses lane 6
			eof_log("\t\"5 lane drums\" detected, enabling lane 6 for PART DRUMS", 1);
			sp->legacy_track[tracknum]->numlanes = 6;	//Enable lane 6 for the drum track
			sp->track[EOF_TRACK_DRUM]->flags |= EOF_TRACK_FLAG_SIX_LANES;	//Set the sixth lane flag
			break;
		}
	}

	eof_log("\tFeedback import completed", 1);

	eof_log_level |= 2;	//Enable verbose logging
	return sp;
}
