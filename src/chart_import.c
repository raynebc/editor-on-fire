#include <assert.h>
#include <ctype.h>
#include "main.h"
#include "song.h"
#include "beat.h"
#include "chart_import.h"
#include "event.h"
#include "midi.h"	//For eof_apply_ts()
#include "ini_import.h"	//For eof_import_ini()
#include "menu/beat.h"
#include "menu/file.h"
#include "utility.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

char *eof_chart_import_return_code_list[] = {
	"Success",														//Return code 0
	"Unable to open file",											//1
	"Unable to read file",											//2
	"Malformed section header",										//3
	"Malformed file (non terminated section)",						//4
	"Malformed file (extraneous [Song] section definition)",		//5
	"Malformed file (extraneous [SyncTrack] section definition)",	//6
	"Malformed file (extraneous [Events] section definition)",		//7
	"Invalid instrument section",									//8
	"Malformed file (extraneous closing curly brace)",				//9
	"Malformed item (missing equal sign)",							//10
	"Invalid chart resolution",										//11
	"Invalid sync item position",									//12
	"Malformed sync item (missing equal sign)",						//13
	"Invalid sync item type",										//14
	"Invalid sync item type",										//15
	"Invalid sync item parameter",									//16
	"Invalid sync item type",										//17
	"Invalid event time position",									//18
	"Malformed event item (missing equal sign)",					//19
	"Invalid event item type",										//20
	"Malformed event item (missing open quotation mark)",			//21
	"Malformed event item (missing close quotation mark)",			//22
	"Invalid gem item position",									//23
	"Malformed gem item (missing equal sign)",						//24
	"Invalid gem item type",										//25
	"Invalid gem item parameter 1",									//26
	"Invalid gem item parameter 2",									//27
	"Malformed file (gem defined outside instrument track)",		//28
	"Invalid section marker",										//29
	"Malformed file (item defined outside of track)"				//30
};

/* convert Feedback chart time to milliseconds for use with EOF */
static double chartpos_to_msec(struct FeedbackChart * chart, unsigned long chartpos, unsigned int *gridsnap)
{
//	eof_log("chartpos_to_msec() entered");

	double offset;
	double curpos;
	unsigned long lastchartpos = 0;
	double beat_length = 500.0;
	double beat_count;
	double convert;
	struct dbAnchor * current_anchor;
	unsigned long intervalnum = 0, interval = 1;

	unsigned long anchorctr = 0;

	if(!chart)
	{
		return 0;
	}

//If the calling function wanted to do so, track whether this chart position should be considered a grid snap position
	if(gridsnap)
	{
		unsigned long beat_chart_pos;						//The chart position of the beat in which the specified chart position is
		unsigned long beatlength_ticks = chart->resolution;	//The resolution defined in the .chart file is the number of ticks in one beat
		unsigned long snaplength;

		*gridsnap = 0;	//Unless found otherwise, assume this delta time is NOT a grid snap position
		beat_chart_pos = chartpos - (chartpos % beatlength_ticks);

		for(interval = 2; interval < EOF_MAX_GRID_SNAP_INTERVALS; interval++)
		{	//Check all of the possible supported custom grid snap intervals
			if(beatlength_ticks % interval != 0)
				continue;	//If the beat's tick length isn't divisible by this interval, skip it

			snaplength = beatlength_ticks / interval;	//Determine the number of ticks in one such grid snap interval
			for(intervalnum = 0; intervalnum < interval; intervalnum++)
			{	//For each instance of that grid snap
				if(beat_chart_pos + (intervalnum * snaplength) == chartpos)
				{	//If the target chart position matches this grid snap's chart position
					*gridsnap = 1;	//Signal this to the calling function
					if(eof_log_level > 1)
					{
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tDetected grid snap of chartpos %lu is %lu / %lu", chartpos, intervalnum, interval);
						eof_log(eof_log_string, 3);
					}
					break;			//Stop checking instances of this grid snap
				}
			}
			if(*gridsnap == 1)
				break;		//If this chart position was found to be a grid snap position, stop checking grid snaps
		}
	}

	current_anchor = chart->anchors;
	offset = chart->offset * 1000.0;
	curpos = offset;
	convert = beat_length / (double)chart->resolution; // current conversion rate of chartpos to milliseconds
	while(current_anchor)
	{	//For each anchor in the chart
		/* find current BPM */
		if(current_anchor->BPM > 0)
		{
			beat_length = 60000000.0 / (double)current_anchor->BPM;

			/* adjust BPM if next beat marker is an anchor */
			if(current_anchor->next && (current_anchor->next->usec > 0))
			{
				beat_count = (double)(current_anchor->next->chartpos - current_anchor->chartpos) / (double)chart->resolution;
				beat_length = (((double)current_anchor->next->usec / 1000.0 + offset) - curpos) / beat_count;
			}

			convert = beat_length / (double)chart->resolution;	//The length of one tick in milliseconds
		}

		/* if the specified chartpos is past the next anchor, add the total time between
		 * the anchors */
		if(current_anchor->next && (chartpos >= current_anchor->next->chartpos))
		{
			if(current_anchor->next->usec > 0)
			{	//If the next anchor has a defined timestamp, use that timestamp for improved accuracy over floating point math
				curpos = offset + (current_anchor->next->usec / 1000.0);
			}
			else
			{	//Otherwise calculate the timestamp
				curpos += (double)(current_anchor->next->chartpos - current_anchor->chartpos) * convert;
			}
			lastchartpos = current_anchor->next->chartpos;
		}

		/* otherwise add the time from the current anchor to the specified chartpos */
		else
		{
			double partial_beat = (double)(chartpos - lastchartpos) * convert;	//Use the beat length established at the previous anchor to determine how long this number of chart ticks is

			if(eof_log_level > 1)
			{
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tTarget position is between anchors, adding %fms", partial_beat);
				eof_log(eof_log_string, 3);
			}
			curpos += partial_beat;
			break;
		}

		if(eof_log_level > 1)
		{
			anchorctr++;
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tdB Anchor #%lu: Chartpos = %lu  BPM = %lu  TS = %d/%d  ms pos = %lu beat length = %f,  Detected pos = %fms", anchorctr, current_anchor->chartpos, current_anchor->BPM, current_anchor->TSN, current_anchor->TSD, current_anchor->usec, beat_length, curpos);
			eof_log(eof_log_string, 3);
		}
		current_anchor = current_anchor->next;
	}//For each anchor in the chart
	if(eof_log_level > 1)
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tFinal converted realtime is %fms", curpos);
		eof_log(eof_log_string, 3);
	}
	return curpos;
}

void eof_chart_import_process_note_markers(EOF_SONG *sp, unsigned long track, unsigned char difficulty)
{
	unsigned long ctr2, ctr3;

	if(!sp || (track >= sp->tracks))
		return;	//Invalid parameters

	if(track == EOF_TRACK_DRUM)
	{	//If this is the drum track
		return;	//Skip it, lane 6 for the drum track indicates a sixth lane and not HOPO, and drum notes can't be slider notes
	}

	eof_log("\t\tProcessing toggle HOPO and slider markers", 2);

	eof_track_sort_notes(sp, track);	//Sort the notes so the logic can assume they're in chronological order
	for(ctr2 = eof_get_track_size(sp, track); ctr2 > 0; ctr2--)
	{	//For each note in the track, in reverse order
		unsigned long pos = eof_get_note_pos(sp, track, ctr2 - 1);
		long len = eof_get_note_length(sp, track, ctr2 - 1);
		unsigned char type = eof_get_note_type(sp, track, ctr2 - 1);
		unsigned long tflags;

		if(eof_get_note_type(sp, track, ctr2 - 1) != difficulty)
			continue;	//If this note is not in the target difficulty, skip it
		if(!(eof_get_note_note(sp, track, ctr2 - 1) & 32))
			continue;	//If this note does not use lane 6 (a "N 5 #" .chart entry), skip it
		tflags = eof_get_note_tflags(sp, track, ctr2 - 1);
		if(tflags & EOF_NOTE_TFLAG_GHL_W3)
		{	//If this note is meant to be on lane 6 (a white 3 GHL note)
			eof_set_note_tflags(sp, track, ctr2 - 1, tflags & ~EOF_NOTE_TFLAG_GHL_W3);	//Clear that flag
			continue;	//And skip this note
		}

		for(ctr3 = 0; ctr3 < eof_get_track_size(sp, track); ctr3++)
		{	//For each note in the track
			unsigned long pos2 = eof_get_note_pos(sp, track, ctr3);

			if(pos2 > pos + len)
			{	//If this note occurs after the span of the HOPO notation
				break;	//Break from inner loop
			}
			if((pos2 >= pos) && (eof_get_note_type(sp, track, ctr3) == type) && (ctr3 != ctr2 - 1))
			{	//If this note is within the span of the HOPO notation, is in the same difficulty and isn't the toggle HOPO marker gem itself
				unsigned long flags, mpos;

				flags = eof_get_note_flags(sp, track, ctr3) ^ EOF_NOTE_FLAG_F_HOPO;	//Toggle the forced HOPO flag for this note
				eof_set_note_flags(sp, track, ctr3, flags);
				mpos = eof_get_note_midi_pos(sp, track, ctr3);
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tNote #%lu:  Diff = %d  Chartpos = %lu  Pos = %lums:  Toggling HOPO status to %s", ctr3, type, mpos, pos2, ((flags & EOF_NOTE_FLAG_F_HOPO) ? "ON" : "OFF"));
				eof_log(eof_log_string, 2);
			}
		}
		eof_track_delete_note(sp, track, ctr2 - 1);	//Delete the HOPO marker gem
	}

	/* check if unofficial "N 6 #" slider notation was found */
	//Mark notes that have slider status
	for(ctr2 = eof_get_track_size(sp, track); ctr2 > 0; ctr2--)
	{	//For each note in the track, in reverse order
		unsigned long pos = eof_get_note_pos(sp, track, ctr2 - 1);
		long len = eof_get_note_length(sp, track, ctr2 - 1);
		unsigned char type = eof_get_note_type(sp, track, ctr2 - 1);

		if(!(eof_get_note_note(sp, track, ctr2 - 1) & 64))
			continue;	//If this note does not use lane 7 (a "N 6 #" .chart entry), skip it

		for(ctr3 = 0; ctr3 < eof_get_track_size(sp, track); ctr3++)
		{	//For each note in the track
			unsigned long pos2 = eof_get_note_pos(sp, track, ctr3);

			if(pos2 > pos + len)
			{	//If this note occurs after the span of the slider notation
				break;	//Break from inner loop
			}
			if((pos2 >= pos) && (eof_get_note_type(sp, track, ctr3) == type) && (ctr3 != ctr2 - 1))
			{	//If this note is within the span of the slider notation, is in the same difficulty and isn't the slider marker gem itself
				unsigned long mpos = eof_get_note_midi_pos(sp, track, ctr3);
				eof_set_note_flags(sp, track, ctr3, (eof_get_note_flags(sp, track, ctr3) | EOF_GUITAR_NOTE_FLAG_IS_SLIDER));	//Set the slider flag for this note
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tNote #%lu:  Diff = %d  Chartpos = %lu  Pos = %lums:  Enabling slider status", ctr3, type, mpos, pos2);
				eof_log(eof_log_string, 2);
			}
		}
		eof_track_delete_note(sp, track, ctr2 - 1);	//Delete the slider marker gem
	}
}

EOF_SONG * eof_import_chart(const char * fn)
{
	struct FeedbackChart * chart = NULL;
	EOF_SONG * sp = NULL, *eof_song_backup;
	int err=0;
	char oggfn[1024] = {0};
	char searchpath[1024] = {0};
	char backup_filename[1024] = {0};
	char dest_name[20] = {0};
	struct al_ffblk info = {0, 0, 0, {0}, NULL}; // for file search
	int ret=0;
	struct dbText * current_event;
	unsigned long chartpos, max_chartpos;
	EOF_BEAT_MARKER *new_beat = NULL;
	struct dbAnchor *ptr, *ptr2;
	unsigned curnum=4,curden=4;		//Stores the current time signature details (default is 4/4)
	char midbeatchange = 0;
	unsigned long nextbeat;
	unsigned long curppqn = 500000;	//Stores the current tempo (default is 120BPM)
	unsigned long beatlength;
	char tschange;
	struct dbTrack * current_track;
	int track;
	int difficulty;
	unsigned long lastchartpos = 0;
	unsigned long b = 0;
	unsigned long ctr, ctr2, tracknum;
	char importguitartypes = 0, importbasstypes = 0;	//Tracks whether 5 or 6 lane guitar and bass types are to be imported (default is whichever one type is encountered, otherwise user is prompted)
	unsigned long threshold;
	unsigned int gridsnap = 0;
	char solo_status = 0;	//0 = Off and awaiting a solo on marker, 1 = On and awaiting a solo off marker
	double solo_on = 0.0, solo_off = 0.0;
	char lyric_status = 0;	//0 = Off and awaiting a phrase start marker, 1 = On and awaiting a phrase end marker
	double lyric_on = 0.0, lyric_off = 0.0;
	unsigned long pos, closestpos = 0;
	char limit_warned = 0;
	char oldoggpath[1024] = {0};
	clock_t start_time, cur_time;
	int event_realignment_warning = 0;

	unsigned long accentpos, ghostpos, cymbalpos;	//Track the last timestamps at which accent, ghost and cymbal markers were seen
	unsigned char accentmask, ghostmask;		//Tracks the accent and ghost bitmasks to apply to notes at the accent and ghost marker timestamp, resets to 0 whenever a marker at a new timestamp is reached
	unsigned long cymbalmask;					//Tracks the cymbal flags to apply to notes at the cymbal marker timestamp, resets to 0 whenver a marker at a new timestamp is reached

	eof_log("\tImporting Feedback chart", 1);
	eof_log("eof_import_chart() entered", 1);
	start_time = clock();

	if(!fn)
	{
		return NULL;
	}
	chart = ImportFeedback(fn, &err);
	if(chart == NULL)
	{	//Import failed
		(void) snprintf(oggfn, sizeof(oggfn) - 1, "Error:  %s", eof_chart_import_return_code_list[err % 31]);	//Display the appropriate error
		(void) alert("Error:", NULL, eof_chart_import_return_code_list[err % 31], "OK", NULL, 0, KEY_ENTER);
		return NULL;
	}
	threshold = (chart->resolution * (66.0 / 192.0)) + 0.5;	//This is the tick distance at which notes become forced strums instead of HOPOs (66/192 beat or further)

	sort_chart(chart);	//Sort markers before gems to ensure more reliable handling of markers and chords

	if(!eof_disable_backups)
	{	//If the user did not disable automatic backups
		/* backup "song.ini" if it exists in the folder with the imported MIDI
		as it will be overwritten upon save */
		(void) replace_filename(eof_temp_filename, fn, "song.ini", 1024);
		if(exists(eof_temp_filename))
		{
			/* do not overwrite an existing backup, this prevents the original backed up song.ini from
			being overwritten if the user imports the MIDI again */
			(void) replace_filename(backup_filename, fn, "song.ini.backup", 1024);
			if(!exists(backup_filename))
			{
				(void) eof_copy_file(eof_temp_filename, backup_filename);
			}
		}
	}
	memcpy(backup_filename, fn, 1024);	//Back up the filename that is passed, if the calling function passed the file selection dialog's return path, that buffer will be clobbered if a file dialog to select the audio is launched
										//This path will be used later to set the song and project paths at the end of the import

	/* identify/convert audio file to load */
	(void) replace_filename(eof_song_path, fn, "", 1024);	//Set the project folder path
	(void) replace_filename(oggfn, fn, "guitar.ogg", 1024);	//Look for guitar.ogg by default
	if((chart->audiofile != NULL) && !exists(oggfn))
	{	//If the imported chart defines which audio file to use AND guitar.ogg doesn't exist
		(void) replace_filename(oggfn, fn, chart->audiofile, 1024);
	}
	if(!exists(oggfn))
	{	//If neither guitar.ogg nor the file specified in the chart file exist, look for OGG files in the chart directory
		/* start file selector at chart directory */
		(void) replace_filename(searchpath, fn, "*.ogg", 1024);
		if(al_findfirst(searchpath, &info, FA_ALL))
		{
			(void) ustrcpy(oldoggpath, eof_last_ogg_path);
			(void) replace_filename(eof_last_ogg_path, fn, "", 1024);
		}

		/* if there is only one OGG file, load it */
		else if(al_findnext(&info))
		{
			(void) replace_filename(oggfn, fn, info.name, 1024);
		}
		al_findclose(&info);
	}

	if(exists(oggfn))
	{	//If an existing audio file has been identified
		(void) replace_filename(searchpath, oggfn, "", 1024);		//Store the path of the file's parent folder
		ret = eof_audio_to_ogg(oggfn, searchpath, dest_name, 1);	//Create a suitably named OGG in the folder, converting to OGG if necessary
		if(ret != 0)
		{	//If guitar.ogg was not created successfully
			DestroyFeedbackChart(chart, 1);
			return NULL;
		}
		(void) replace_filename(oggfn, fn, dest_name, 1024);	//Update the name of the OGG file that eof_load_ogg() is to access
	}

	/* create empty song */
	sp = eof_create_song_populated();
	if(!sp)
	{
		DestroyFeedbackChart(chart, 1);
		return NULL;
	}

	/* load audio */
	ogg_profile_name = sp->tags->ogg[0].filename;	//Store the pointer to the OGG profile filename to be updated by eof_load_ogg()
	if(!eof_load_ogg(oggfn, 2))	//If user does not provide audio, fail over to using silent audio
	{
		DestroyFeedbackChart(chart, 1);
		eof_destroy_song(sp);
		(void) ustrcpy(eof_last_ogg_path, oldoggpath); // remember previous OGG directory if we fail
		return NULL;
	}

	/* backup the current value of eof_song and assign sp to it so that grid snap logic can be performed during note creation */
	eof_song_backup = eof_song;
	eof_song = sp;

	/* copy tags */
	if(chart->name)
	{
		strncpy(sp->tags->title, chart->name, sizeof(sp->tags->title) - 1);
	}
	if(chart->artist)
	{
		strncpy(sp->tags->artist, chart->artist, sizeof(sp->tags->artist) - 1);
	}
	if(chart->charter)
	{
		strncpy(sp->tags->frettist, chart->charter, sizeof(sp->tags->frettist) - 1);
	}

	/* read INI file */
	(void) replace_filename(oggfn, fn, "song.ini", 1024);
	(void) eof_import_ini(sp, oggfn, 0);

	/* set up beat markers */
	sp->tags->ogg[0].midi_offset = chart->offset * 1000.0;
	chartpos = 0;

	/* find the highest chartpos for beat markers */
	max_chartpos = chart->chart_length;	//ImportFeedback() tracked the highest used chart position

	/* create beat markers */
	beatlength = chart->resolution;	//Until a time signature is defined, assume #/4 for the purpose of determining beat length
	curden = 4;

	cur_time = clock();
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tAdding beats (import currently at %.2f seconds)", (((double)cur_time - (double)start_time) / CLOCKS_PER_SEC));
	eof_log(eof_log_string, 1);

	while(chartpos <= max_chartpos)
	{	//Add new beats until enough have been added to encompass the last item in the chart
		new_beat = eof_song_add_beat(sp);
		if(new_beat)
		{	//If the beat was created successfully
			//Find the relevant tempo and time signature for the beat
			for(ptr = chart->anchors, tschange = 0; ptr != NULL; ptr = ptr->next)
			{	//For each anchor in the chart
				if(ptr->chartpos <= chartpos)
				{	//If the anchor is at or before the current position
					if(ptr->BPM)
					{	//If this anchor defines a tempo change (is nonzero)
						curppqn = (60000000.0 / (ptr->BPM / 1000.0)) + 0.5;	//Convert tempo
					}
					if(ptr->TSN || ptr->TSD)
					{	//If this anchor defines a tempo change (either the numerator or denominator is nonzero)
						curnum = ptr->TSN;	//Store this anchor's time signature
						curden = ptr->TSD;
						if(ptr->chartpos == chartpos)
						{	//If this change is at the current position
							tschange = 1;	//Keep note
						}
					}
				}
			}

			//Determine if this beat represents a mid-beat tempo change
			if(midbeatchange)
			{	//If this anchor was determined in the previous loop iteration to occur off-beat
				new_beat->flags |= EOF_BEAT_FLAG_MIDBEAT;	//Flag the beat as such so it can be removed after import if the user preference is to do so
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tMid beat tempo change at chart position %lu", chartpos);
				eof_log(eof_log_string, 1);
			}

			if(eof_use_ts && (tschange || (sp->beats == 1)))
			{	//If the user opted to import TS changes, and this anchor has a TS change (or this is the first beat)
				(void) eof_apply_ts(curnum,curden,sp->beats - 1,sp,0);	//Set the TS flags for this beat
			}
			new_beat->ppqn = curppqn;
			new_beat->midi_pos = chartpos;
			beatlength = (chart->resolution * 4) / curden;	//Get the expected length of one beat in the current time signature (ie. #/8 is half the length of the chart resolution)

			//Scan ahead to look for mid beat tempo or TS changes
			midbeatchange = 0;
			nextbeat = chartpos + beatlength;	//By default, the delta position of the next beat will be the standard length of delta ticks
			for(ptr2 = chart->anchors; ptr2 != NULL; ptr2 = ptr2->next)
			{	//For each anchor in the chart
				if(ptr2->chartpos > chartpos)
				{	//If this anchor is ahead of the current delta position
					if(ptr2->chartpos < nextbeat)
					{	//If this anchor occurs before the next beat marker
						nextbeat = ptr2->chartpos;	//Store its delta time
						midbeatchange = 1;
					}
					break;
				}
			}
			if(midbeatchange)
			{	//If there is a mid-beat tempo/TS change coming up, this beat needs to be anchored and its tempo (and the current effective tempo) altered
				//Also update beatlength to reflect that less than a full beat's worth of deltas will be used to advance to the next beat marker
				sp->beat[sp->beats - 1]->flags |= EOF_BEAT_FLAG_ANCHOR;
				curppqn = (double)curppqn * (((double)nextbeat - chartpos) / beatlength) + 0.5;	//Scale the current beat's tempo based on the adjusted delta length (rounded to nearest whole number)
				sp->beat[sp->beats - 1]->ppqn = curppqn;		//Update the beat's (now an anchor) tempo
				beatlength = nextbeat - chartpos;	//This is the distance between the current beat, and the upcoming mid-beat change
			}
		}//If the beat was created successfully
		chartpos += beatlength;	//Add the delta length of this beat to the delta counter
	}//Add new beats until enough have been added to encompass the last item in the chart

	eof_calculate_beats(sp);		//Set the beats' timestamps based on their tempo changes

	/* check for the presence of both normal and GHL guitar or bass tracks, prompt user which to import */
	if(chart->guitartypes > 2)
	{	//If the imported chart file has both normal AND GHL guitar tracks
		eof_clear_input();
		if(alert(NULL, "The imported file has both normal and GHL guitar parts.  Import which parts??", NULL, "&Normal", "&GHL", 'n', 'g') == 2)
		{	//If the user opts to import the GHL parts
			eof_log("\t\tImporting GHL guitar and skipping normal guitar", 1);
			importguitartypes = 2;
		}
		else
		{
			eof_log("\t\tImporting normal guitar and skipping GHL guitar", 1);
			importguitartypes = 1;
		}
	}
	if(chart->basstypes > 2)
	{	//If the imported chart file has both normal AND GHL bass tracks
		eof_clear_input();
		if(alert(NULL, "The imported file has both normal and GHL bass parts.  Import which parts??", NULL, "&Normal", "&GHL", 'n', 'g') == 2)
		{	//If the user opts to import the GHL parts
			eof_log("\t\tImporting GHL bass and skipping normal bass", 1);
			importbasstypes = 2;
		}
		else
		{
			eof_log("\t\tImporting normal bass and skipping GHL bass", 1);
			importbasstypes = 1;
		}
	}

	/* fill in notes */
	cur_time = clock();
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tImporting notes (import currently at %.2f seconds)", (((double)cur_time - (double)start_time) / CLOCKS_PER_SEC));
	eof_log(eof_log_string, 1);

	current_track = chart->tracks;
	while(current_track)
	{	//For each track
		EOF_LEGACY_TRACK *tp;

		accentpos = ghostpos = cymbalpos = accentmask = ghostmask = cymbalmask = 0;	//Reset these

		if(current_track->isguitar && importguitartypes)
		{	//If this is a guitar track and the user was prompted whether to import just the normal or the GHL guitar track
			if(current_track->isguitar != importguitartypes)
			{	//But this is not the type that the user opted to import
				current_track = current_track->next;
				lastchartpos = 0;	//Reset this value
				eof_log("\t\tSkipping track difficulty as per selected normal/GHL guitar import choice", 1);
				continue;	//Skip the track
			}
		}

		if(current_track->isbass && importbasstypes)
		{	//If this is a bass track and the user was prompted whether to import just the normal or the GHL bass track
			if(current_track->isbass != importbasstypes)
			{	//But this is not the type that the user opted to import
				current_track = current_track->next;
				lastchartpos = 0;	//Reset this value
				eof_log("\t\tSkipping track difficulty as per selected normal/GHL bass import choice", 1);
				continue;	//Skip the track
			}
		}

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
				if(current_track->isdrums == 1)
				{	//Normal drum track
					track = EOF_TRACK_DRUM;
				}
				else
				{	//Double drums track
					track = EOF_TRACK_DRUM_PS;
				}
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

			/* PART KEYS */
			case 7:
			{
				track = EOF_TRACK_KEYS;
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
		if(track > 0)
		{	//If the track is valid
			struct dbNote * current_note = current_track->notes;
			unsigned long lastpos = -1;	//The position of the last imported note (not updated for sections that are parsed)
			unsigned long lastduration = -1;
			char lastnotewasgem = 0;
			EOF_NOTE * new_note = NULL;
			EOF_NOTE * prev_note = NULL;
			char gemtype = 0, lastgemtype = 0;	//Tracks whether the current and previously added gems are normal notes or technique markers
			unsigned long ch_solo_pos = 0;	//Tracks the start position of the last Clone Hero solo event
			char ch_solo_on = 0;			//Tracks whether a Clone Hero solo event is in progress
			unsigned long notepos;
			unsigned long notes_imported = 0, notes_created = 0, notes_combined = 0;	//Statistics for debugging

			tracknum = sp->track[track]->tracknum;
			tp = sp->legacy_track[tracknum];

			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tTrack:  \"%s\" difficulty %d", sp->track[track]->name, difficulty);
			eof_log(eof_log_string, 2);

			if((current_track->isguitar > 1) || (current_track->isbass > 1))
			{	//If this is a GHL guitar or bass track, configure the track accordingly
				EOF_TRACK_ENTRY *ep = sp->track[track];		//Simplify
				ep->flags |= EOF_TRACK_FLAG_GHL_MODE;
				ep->flags |= EOF_TRACK_FLAG_GHL_MODE_MS;	//Denote that the new GHL lane ordering is in effect
				tp->numlanes = 6;
				ep->flags |= EOF_TRACK_FLAG_SIX_LANES;		//Set the open strum flag
				eof_log("\t\t\tImporting as GHL track", 2);
			}

			while(current_note)
			{	//For each note in the track
				unsigned long originalnotepos;	//In case a position is resnapped, this will store the original converted timing, for correctly retaining gem length

				notepos = chartpos_to_msec(chart, current_note->chartpos, &gridsnap) + 0.5;	//Round up
				originalnotepos = notepos;	//Keep a copy of this value
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tNote #%lu:  Chartpos = %lu  Pos = %lums  Gem value = %d", notes_imported++, current_note->chartpos, notepos, current_note->gemcolor);
				eof_log(eof_log_string, 1);
				if(gridsnap && !eof_is_any_beat_interval_position(notepos, NULL, NULL, NULL, &closestpos, eof_prefer_midi_friendly_grid_snapping))
				{	//If this chart position should be a beat interval, but the timing conversion did not result in this, re-snap the note honoring the user preference whether to use MIDI friendly grid snaps
					if(closestpos != ULONG_MAX)
					{	//If the nearest beat interval position was determined
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\tCorrecting position from %lums to %lums", notepos, closestpos);
						eof_log(eof_log_string, 1);
						notepos = closestpos;	//Change it to be the closest beat interval position to the converted timestamp
					}
				}

				/* import star power */
				if(current_note->gemcolor == '2')
				{
					unsigned long endpos = current_note->chartpos + current_note->duration;	//Simplify

					//Look ahead to see if the star power phrase ends at another note's start position.  If so, the marker needs to be shortened to NOT include that note
					if(current_note->duration > 1)
					{	//If the star power marker is at least 2 ticks long
						struct dbNote *noteptr;

						for(noteptr = current_note->next; noteptr != NULL; noteptr = noteptr->next)
						{	//For notes defined after the star power marker
							if(noteptr->chartpos > endpos)
							{	//If this and all remaining notes are outside the scope of the star power marker
								break;	//Stop processing the notes following the star power marker
							}
							if(noteptr->chartpos == endpos)
							{	//If this note starts at the star power marker's end position
								endpos--;	//Shorten the marker by one delta tick to ensure the note is outside its scope when written to MIDI
								break;	//Stop processing the notes following the star power marker
							}
						}
					}
					(void) eof_legacy_track_add_star_power(tp, notepos, chartpos_to_msec(chart, endpos, NULL));
				}

				/* import Clone Hero solo */
				else if(current_note->gemcolor == 'S')
				{	//The start of a solo
					if(difficulty == EOF_NOTE_AMAZING)
					{	//This event is only respected when defined in a track's expert difficulty
						ch_solo_pos = notepos;
						ch_solo_on = 1;
					}
				}
				else if(current_note->gemcolor == 'E')
				{	//The end of a solo
					if(difficulty == EOF_NOTE_AMAZING)
					{	//This event is only respected when defined in a track's expert difficulty
						if(ch_solo_on)
						{	//If the start of solo was defined earlier
							(void) eof_legacy_track_add_solo(tp, ch_solo_pos, chartpos_to_msec(chart, current_note->chartpos, NULL));
							ch_solo_on = 0;
						}
					}
				}

				/* skip face-off sections for now */
				else if((current_note->gemcolor == '0') || (current_note->gemcolor == '1'))
				{
				}

				/* skip unknown section markers */
				else if((current_note->gemcolor == '3') || (current_note->gemcolor == '4'))
				{
				}

				/* accented drum markers */
				else if(current_track->isdrums && (current_note->gemcolor >= 34) && (current_note->gemcolor <= 38))
				{
					if(accentpos != notepos)
					{
						accentmask = 0;		//An accent marker at a different timestamp was encountered, reset this mask
						accentpos = notepos;	//Track this as the active accent marker position
					}

					if(current_note->gemcolor == 34)
						accentmask |= 2;	//Lane 1 accent
					else if(current_note->gemcolor == 35)
						accentmask |= 4;	//Lane 2 accent
					else if(current_note->gemcolor == 36)
						accentmask |= 8;	//Lane 3 accent
					else if(current_note->gemcolor == 37)
						accentmask |= 16;	//Lane 4 accent
					else if(current_note->gemcolor == 38)
						accentmask |= 32;	//Lane 5 accent

					eof_merge_accent_mask_at_pos(sp, track, notepos, accentmask);	//Update the accent status of any existing notes at this position
				}

				/* ghost drum markers */
				else if(current_track->isdrums && (current_note->gemcolor >= 40) && (current_note->gemcolor <= 44))
				{
					if(ghostpos != notepos)
					{
						ghostmask = 0;		//A ghost marker at a different timestamp was encountered, reset this mask
						ghostpos = notepos;	//Track this as the active ghost marker position
					}

					if(current_note->gemcolor == 40)
						ghostmask |= 2;	//Lane 1 ghost
					else if(current_note->gemcolor == 41)
						ghostmask |= 4;	//Lane 2 ghost
					else if(current_note->gemcolor == 42)
						ghostmask |= 8;	//Lane 3 ghost
					else if(current_note->gemcolor == 43)
						ghostmask |= 16;	//Lane 4 ghost
					else if(current_note->gemcolor == 44)
						ghostmask |= 32;	//Lane 5 ghost

					eof_merge_ghost_mask_at_pos(sp, track, notepos, ghostmask);	//Update the ghost status of any existing notes at this position
				}

				/* cymbal drum markers */
				else if(current_track->isdrums && (current_note->gemcolor >= 66) && (current_note->gemcolor <= 68))
				{
					if(cymbalpos != notepos)
					{
						cymbalmask = 0;		//A cymbal marker at a different timestamp was encountered, reset this mask
						cymbalpos = notepos;	//Track this as the active cymbal marker position
					}

					if(current_note->gemcolor == 66)
						cymbalmask |= EOF_DRUM_NOTE_FLAG_Y_CYMBAL;	//Lane 2 cymbal
					else if(current_note->gemcolor == 67)
						cymbalmask |= EOF_DRUM_NOTE_FLAG_B_CYMBAL;	//Lane 3 cymbal
					else if(current_note->gemcolor == 68)
						cymbalmask |= EOF_DRUM_NOTE_FLAG_G_CYMBAL;	//Lane 4 cymbal

					eof_merge_flags_at_pos(sp, track, notepos, cymbalmask);	//Update the cymbal status of any existing notes at this position
				}

				/* drum roll */
				else if(current_track->isdrums && (current_note->gemcolor == 70))
				{
					unsigned long endpos = chartpos_to_msec(chart, current_note->chartpos + current_note->duration, NULL);
					(void) eof_track_add_tremolo(sp, track, notepos, endpos, 0xFF);
				}

				/* special drum roll */
				else if(current_track->isdrums && (current_note->gemcolor == 71))
				{
					unsigned long endpos = chartpos_to_msec(chart, current_note->chartpos + current_note->duration, NULL);
					(void) eof_track_add_trill(sp, track, notepos, endpos);
				}

				/* import regular note */
				else
				{
					unsigned char note;

					if((current_note->gemcolor == 5) || (current_note->gemcolor == 6))
					{	//If this gem is inverted HOPO or slider notation
						gemtype = 2;	//This gem is a technique marker
					}
					else
					{
						gemtype = 1;	//Otherwise it's a normal note
					}

					if(sp->track[track]->flags & EOF_TRACK_FLAG_GHL_MODE)
					{	//The gem color has to be appropriately remapped for GHL tracks
						if(current_note->gemcolor == 0)
						{
							note = 8;	//W1 is mapped to lane 4
						}
						else if(current_note->gemcolor == 1)
						{
							note = 16;	//W2 is mapped to lane 5
						}
						else if(current_note->gemcolor == 2)
						{
							note = 32;	//W3 is mapped to lane 6
						}
						else if(current_note->gemcolor == 3)
						{
							note = 1;	//B1 is mapped to lane 1
						}
						else if(current_note->gemcolor == 4)
						{
							note = 2;	//B2 is mapped to lane 2
						}
						else if(current_note->gemcolor == 8)
						{
							note = 4;	//B3 is mapped to lane 3
						}
						else if(current_note->gemcolor < 7)
						{	//Other values are for various notations
							note = 1 << current_note->gemcolor;
						}
					}
					else if(current_note->gemcolor == 69)
					{	//A drum fill is authored as a 5 note chord in the BRE difficulty
						if(difficulty == EOF_NOTE_AMAZING)
							note = 31;	//Only import these for expert difficulty tracks
						else
							note = 0;		//Otherwise create a blank note that will get deleted
					}
					else
					{	//Otherwise it's a simple bit shift
						note = 1 << current_note->gemcolor;
					}

					if((current_note->chartpos != lastpos) || (gemtype == 2) || (gemtype != lastgemtype) || (current_note->duration != lastduration))
					{	//If this note was at a different position than the last, if it represents a technique marker or if it's a different gem type than the previous one
						// or if this gem has a different length than the previous gem
						new_note = eof_legacy_track_add_note(tp);
						if(new_note)
						{
							new_note->midi_pos = current_note->chartpos;	//Track the note's original chart position to more reliably apply HOPO status
							new_note->pos = notepos;	//Assign the position (which may have been resnapped)
							new_note->length = chartpos_to_msec(chart, current_note->chartpos + current_note->duration, NULL) - (double)originalnotepos + 0.5;	//Determine the length (using the non resnapped position), round up
							if(!new_note->length)
								new_note->length = 1;	//Pad the note to 1ms length if it has no length (to ensure that each slider that is added always ends after the start position of its final note)
							if((current_note->chartpos == lastpos) && (current_note->duration != lastduration) && lastnotewasgem)
							{	//If the previous note was a gem (instead of a marker) and this gem and the previous one start at the same time but have different lengths
								new_note->eflags |= EOF_NOTE_EFLAG_DISJOINTED;	//Apply disjointed status to the new note
							}
							if((sp->track[track]->flags & EOF_TRACK_FLAG_GHL_MODE) && (current_note->gemcolor == 2))
							{	//If this is a white 3 GHL gem
								new_note->tflags |= EOF_NOTE_TFLAG_GHL_W3;	//Track that this lane 6 note will be treated as a gem on that lane instead of as a toggle HOPO marker
							}
							new_note->note = note;			//Set the translated note bitmask
							if(current_note->gemcolor == 'F')
							{	//If this is a drum fill
								new_note->type = EOF_NOTE_SPECIAL;	//It must go in the big rock ending difficulty
							}
							else
							{
								new_note->type = difficulty;
							}
							if(accentpos == notepos)
							{	//If an accent marker is in effect at this timestamp
								new_note->accent |= accentmask;			//Add the defined accent status
								new_note->accent &= new_note->note;	//Restrict it to the gems this note contains
							}
							if(ghostpos == notepos)
							{	//If a ghost marker is in effect at this timestamp
								new_note->ghost |= ghostmask;			//Add the defined ghost status
								new_note->ghost &= new_note->note;	//Restrict it to the gems this note contains
							}
							if(cymbalpos == notepos)
							{	//If a cymbal marker is in effect at this timestamp
								new_note->flags |= cymbalmask;		//Add the defined cymbal status
							}
							if(prev_note)
							{	//If a previous gem was imported
								if(current_note->chartpos == lastchartpos)
								{	//If this gem is at a different position than the last one that was imported
									new_note->flags = prev_note->flags;	//It will inherit the same flags in terms of HOPO status
								}
							}
							prev_note = new_note;		//Track the last created note
							lastchartpos = current_note->chartpos;	//Track the position of the gem for HOPO tracking
							notes_created++;
						}
						else if(!limit_warned)
						{	//If the note failed to be created
							if((tp->notes == EOF_MAX_NOTES) && !limit_warned)
							{	//The note limit was reached, and the user wasn't warned about this yet
								allegro_message("Warning:  At least one instrument track being imported exceeds EOF's note limit and cannot be imported in its entirety.  You can try to work around this by importing a copy of the chart that has had one or more other difficulties deleted from it.");
								limit_warned = 1;
							}
						}
					}
					else
					{	//Otherwise add a gem to the previously created note
						if(new_note)
						{	//Validate that a previous note has been created
							if(current_note->gemcolor == 2)
							{	//If this is a W3 gem
								new_note->tflags |= EOF_NOTE_TFLAG_GHL_W3;	//Apply this flag to reflect that this lane 6 gem is a W3 note and not a toggle HOPO marker
							}
							new_note->note |= note;			//Add the translated note bitmask
							notes_combined++;
						}
					}
					lastpos = current_note->chartpos;
					lastgemtype = gemtype;
					lastduration = current_note->duration;
					lastnotewasgem = current_note->is_gem;
				}
				current_note = current_note->next;
			}//For each note in the track

			cur_time = clock();
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tTrack notes imported (import currently at %.2f seconds)", (((double)cur_time - (double)start_time) / CLOCKS_PER_SEC));
			eof_log(eof_log_string, 1);

			eof_chart_import_process_note_markers(sp, track, difficulty);	//Process and remove toggle HOPO and slider marker gems where applicable so they no longer count against the track's note limit

			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tImported track difficulty:  %lu notes created, %lu gems combined to form chords", notes_created, notes_combined);
			eof_log(eof_log_string, 2);

			//Apply HOPO status to notes within the threshold
			for(ctr = 0, prev_note = NULL; ctr < tp->notes; ctr++)
			{	//For each note in the track that was added to
				if(tp->note[ctr]->type != difficulty)
				{	//If this note isn't in the difficulty that was just parsed
					continue;	//Skip it
				}
				if(prev_note)
				{	//If this isn't the first note in this track difficulty
					if(prev_note->midi_pos < tp->note[ctr]->midi_pos)
					{	//Verify this note is after the previous one in this difficulty
						if(tp->note[ctr]->midi_pos - prev_note->midi_pos < threshold)
						{	//If the later of the two notes is within the HOPO threshold
							if(eof_note_count_colors_bitmask(tp->note[ctr]->note) == 1)
							{	//If the later of the two notes is not a chord
								if(tp->note[ctr]->note != prev_note->note)
								{	//If this note and the one before it aren't identical in which lanes they use, GH3's HOPO criteria have been met
									if(!current_track->isdrums)
									{	//If it isn't a drum track being imported
										tp->note[ctr]->flags ^= EOF_NOTE_FLAG_F_HOPO;	//The note is a hammer on note (toggle, since a toggle HOPO marker may have already been processed for this note and this would lead to the correct result)
									}
								}
							}
						}
					}
				}
				prev_note = tp->note[ctr];		//Track this note to compare it with the next one and set its HOPO status appropriately
			}
			eof_track_find_crazy_notes(sp, track, 1);	//Mark overlapping notes with crazy status, but not notes that start at the exact same timestamp (will be given disjointed status or merge into chords as appropriate)
		}//If the track is valid
		current_track = current_track->next;
		lastchartpos = 0;	//Reset this value
	}//For each track

	/* load text events */
	current_event = chart->events;
	while(current_event)
	{	//For each text event from the chart file
		double beatpos = 0.0;

		//Determine the beat marker associated with the event, for text events
		for(ctr = 0, b = 0; ctr < sp->beats; ctr++)
		{	//For each beat in the project
			if(current_event->chartpos < sp->beat[ctr]->midi_pos)	//If the text event occurs before this beat
				continue;	//Skip it

			if(current_event->chartpos == sp->beat[ctr]->midi_pos)
			{	//If this is the beat that should contain the event
				b = ctr;	//Store this beat number.  Any off-beat text event will be stored as a floating text event
				beatpos = ctr;
				break;
			}
			else if((ctr + 1 < sp->beats) && (current_event->chartpos < sp->beat[ctr + 1]->midi_pos))
			{	//If the text event occurs in between this beat and the next
				unsigned long delta = current_event->chartpos - sp->beat[ctr]->midi_pos;		//The number of ticks into this beat the text event is
				unsigned long beatlen = sp->beat[ctr + 1]->midi_pos - sp->beat[ctr]->midi_pos;	//The number of delta ticks between the two beats

				beatpos = ctr + ((double)delta / (double)beatlen);
				break;
			}
		}

		//Determine the realtime position associated with the event, for solos, lyric lines and lyrics
		pos = chartpos_to_msec(chart, current_event->chartpos, &gridsnap) + 0.5;	//Store the real timestamp associated with the event, rounded up to nearest millisecond
		if(gridsnap && !eof_is_any_beat_interval_position(pos, NULL, NULL, NULL, &closestpos, eof_prefer_midi_friendly_grid_snapping))
		{	//If this chart position should be a beat interval, but the timing conversion did not result in this, re-snap the event honoring the user preference whether to use MIDI friendly grid snaps
			if(closestpos != ULONG_MAX)
			{	//If the nearest beat interval position was determined
				pos = closestpos;	//Change it to be the closest beat interval position to the converted timestamp
			}
		}

		if(!ustricmp(current_event->text, "[solo_on]") && !solo_status)
		{	//If this is a solo on event (and a solo definition isn't already in progress)
			solo_on = pos;
			solo_status = 1;
		}
		else if(!ustricmp(current_event->text, "[solo_off]") && solo_status)
		{	//If this is a solo off event (and a solo_off event is expected), add it to the guitar and lead guitar tracks (FoF's original behavior for these events)
			solo_off = chartpos_to_msec(chart, current_event->chartpos, NULL);	//Store the real timestamp associated with the end of the phrase
			solo_status = 0;
			(void) eof_track_add_solo(sp, EOF_TRACK_GUITAR, solo_on, solo_off + 0.5);	//Add the solo to the guitar track
			(void) eof_track_add_solo(sp, EOF_TRACK_GUITAR_COOP, solo_on, solo_off + 0.5);	//Add the solo to the lead guitar track
		}
		else if(eof_text_is_section_marker(current_event->text))
		{	//If this is a section event, rebuild the string to ensure it's in the proper format
			char buffer[256] = {0};
			int index = 0, index2 = 0;	//index1 will index into the rebuilt buffer[] string, index2 will index into the original current_event->text[] string
			unsigned long flags = 0;	//By default, the section is to be treated as an on-beat section

			buffer[index++] = '[';	//Begin with an open bracket
			while(current_event->text[index2] != '\0' && (index < 254))
			{	//While the end of the string hasn't been reached (with an additional overflow check)
				if((current_event->text[index2] != '[') && (current_event->text[index2] != ']'))
				{	//If this character isn't a bracket
						buffer[index++] = current_event->text[index2];	//Copy it into the rebuilt string
				}
				index2++;	//Iterate to the next character
			}
			buffer[index++] = ']';	//End with a closing bracket
			buffer[index++] = '\0';	//Terminate the string

			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tSection:  Original chart position = %lu ticks (%f beats).  Assigned position = %lums: %s", current_event->chartpos, beatpos, pos, buffer);
			eof_log(eof_log_string, 1);

			if(pos != sp->beat[b]->pos)
			{	//If the event is not on a beat position, it will be added as a floating text event
				eof_log("\t\t\t\t!This section is being added as a floating (off-beat) text event", 1);
				if(!event_realignment_warning)
				{	//If the user wasn't warned about this yet
					allegro_message("Note:  At least one section was defined mid-beat and is being stored as a \"floating\" text event.  Check logging for details.");
					event_realignment_warning = 1;
				}
				flags = EOF_EVENT_FLAG_FLOATING_POS;	//The call to eof_song_add_text_event() will receive this as a floating text event
				b = pos;								//with a millisecond timestamp instead of an assigned beat index
			}
			(void) eof_song_add_text_event(sp, b, buffer, 0, flags, 0);
		}
		else if(!ustricmp(current_event->text, "phrase_start"))
		{	//If this is a Clone Hero start of lyric line marker (and a lyric line definition isn't already in progress)
			if(!lyric_status)
			{	//If a lyric line definition isn't already in progress, handle this normally
				lyric_on = pos;
				lyric_status = 1;
			}
			else
			{	//Otherwise consider this an ending to the previous line in progress and the beginning of a new line
				lyric_off = chartpos_to_msec(chart, current_event->chartpos, NULL) - 1.0;	//End the previous line 1ms before the beginning of this line
				(void) eof_vocal_track_add_line(sp->vocal_track[0], lyric_on, lyric_off, 0xFF);
				lyric_on = pos;
			}
		}
		else if(!ustricmp(current_event->text, "phrase_end") && lyric_status)
		{	//If this is a Clone Hero end of lyric line marker (and such a marker is expected), add the lyric line definition to the vocal track
			lyric_off = chartpos_to_msec(chart, current_event->chartpos, NULL);	//Store the real timestamp associated with the end of the lyric line
			lyric_status = 0;
			(void) eof_vocal_track_add_line(sp->vocal_track[0], lyric_on, lyric_off + 0.5, 0xFF);
		}
		else
		{
			char *lptr = strcasestr_spec(current_event->text, "lyric ");	//Check for this text string

			if(lptr)
			{	//If this is a Clone Hero lyric definition
				EOF_LYRIC *temp = eof_vocal_track_add_lyric(sp->vocal_track[0]);

				if(temp)
				{	//If a lyric structure was successfully created
					temp->note = 0;	//Clone Hero lyrics do not define pitch
					temp->pos = pos;
					temp->length = 1;	//Clone Hero lyrics do not define duration
					(void) ustrncpy(temp->text, lptr, (int)sizeof(temp->text) - 1);	//Copy the event string that is expected to be the lyric text
				}
			}
			else
			{	//This is a normal text event, copy the string as-is
				unsigned long flags = 0;	//By default, the section is to be treated as an on-beat text event

				if(pos != sp->beat[b]->pos)
				{	//If the event is not on a beat position, it will be added as a floating text event
					eof_log("\t\t\t\t!This text event is being added as a floating (off-beat) text event", 1);
					flags = EOF_EVENT_FLAG_FLOATING_POS;	//The call to eof_song_add_text_event() will receive this as a floating text event
					b = pos;								//with a millisecond timestamp instead of an assigned beat index
				}
				(void) eof_song_add_text_event(sp, b, current_event->text, 0, flags, 0);
			}
		}
		current_event = current_event->next;	//Iterate to the next text event
	}//For each event from the chart file

	DestroyFeedbackChart(chart, 1);	//Free memory used by Feedback chart before exiting function

	/* check if there were lane 5 drums imported */
	assert(sp != NULL);	//Unneeded assertion to resolve false positive in Splint
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

	eof_sort_notes(sp);	//Ensure the notes are sorted as the slider phrase creation logic will expect

	//Create slider phrases
	cur_time = clock();
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tCreating slider phrases (import currently at %.2f seconds)", (((double)cur_time - (double)start_time) / CLOCKS_PER_SEC));
	eof_log(eof_log_string, 1);
	for(ctr = 1; ctr < sp->tracks; ctr++)
	{	//For each track
		//Add slider phrases to encompass marked notes
		for(ctr2 = 0; ctr2 < eof_get_track_size(sp, ctr); ctr2++)
		{	//For each note in the track
			if(eof_get_note_flags(sp, ctr, ctr2) & EOF_GUITAR_NOTE_FLAG_IS_SLIDER)
			{	//If this note is a slider note
				unsigned long end = 0;
				unsigned long start = eof_get_note_pos(sp, ctr, ctr2);	//Track the start position of this run of slider notes
				long length;

				while(ctr2 + 1 < eof_get_track_size(sp, ctr))
				{	//While there are additional notes to check
					unsigned long next = ctr2 + 1;

					if(!(eof_get_note_flags(sp, ctr, next) & EOF_GUITAR_NOTE_FLAG_IS_SLIDER))
						break;	//If the next note isn't a slider note, exit inner loop
					if(eof_get_note_pos(sp, ctr, next) > eof_get_note_pos(sp, ctr, ctr2) + eof_get_note_length(sp, ctr, ctr2) + 1000)
						break;	//If the next note begins more than a second after this one ends, exit inner loop
					ctr2++;		//Otherwise include this note in the slider note phrase
				}
				length = eof_get_note_length(sp, ctr, ctr2);
				if(length == 1)
				{	//If the last note in the slider phrase is only 1ms long
					length++;	//Increase the slider length by 1ms to better avoid special cases where the note will quantize to be outside the slider during MIDI export
				}
				end = eof_get_note_pos(sp, ctr, ctr2) + length;	//Track the end position of this run of slider notes
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tCreating slider section from %lums to %lums", start, end);
				eof_log(eof_log_string, 2);
				(void) eof_track_add_section(sp, ctr, EOF_SLIDER_SECTION, 0xFF, start, end, 0, NULL);	//Add the slider phrase
			}
		}
	}

	/* check if unofficial open strum notation (5 lane chord) was found */
	if(!eof_db_import_suppress_5nc_conversion)
	{	//If the user did not suppress converting these types of chords into open notes
		for(ctr = 1; ctr < sp->tracks; ctr++)
		{	//For each track
			if(sp->track[ctr]->track_format != EOF_LEGACY_TRACK_FORMAT)
			{	//If this isn't a legacy track
				continue;	//Skip it
			}
			for(ctr2 = 0; ctr2 < eof_get_track_size(sp, ctr); ctr2++)
			{	//For each note in the track
				if(eof_get_note_note(sp, ctr, ctr2) == 31)
				{	//If this note is a 5 lane chord, convert it to a lane 6 gem
					tracknum = sp->track[ctr]->tracknum;
					sp->track[ctr]->flags |= EOF_TRACK_FLAG_SIX_LANES;	//Set this flag
					sp->legacy_track[tracknum]->numlanes = 6;	//Set this track to have 6 lanes instead of 5
					eof_set_note_note(sp, ctr, ctr2, 32);
				}
			}
		}
	}

	/* check if Clone Hero's open strum notation ("N 7 #" lane 8 note) was found */
	for(ctr = 1; ctr < sp->tracks; ctr++)
	{	//For each track
		if(sp->track[ctr]->track_format != EOF_LEGACY_TRACK_FORMAT)
		{	//If this isn't a legacy track
			continue;	//Skip it
		}
		for(ctr2 = 0; ctr2 < eof_get_track_size(sp, ctr); ctr2++)
		{	//For each note in the track
			if(eof_get_note_note(sp, ctr, ctr2) == 128)
			{	//If this note is a lane 8 single note, convert it to a lane 6 gem
				tracknum = sp->track[ctr]->tracknum;
				sp->track[ctr]->flags |= EOF_TRACK_FLAG_SIX_LANES;	//Set this flag
				sp->legacy_track[tracknum]->numlanes = 6;	//Set this track to have 6 lanes instead of 5
				eof_set_note_note(sp, ctr, ctr2, 32);
				if(eof_track_is_ghl_mode(sp, ctr))
				{	//If this is a GHL track, open notes also require a status flag
					eof_set_note_flags(sp, ctr, ctr2, eof_get_note_flags(sp, ctr, ctr2) | EOF_GUITAR_NOTE_FLAG_GHL_OPEN);	//Set that flag
				}
			}
		}
	}

	/* mark anything that wasn't specifically made into a forced HOPO note as a forced strum */
	for(ctr = 1; ctr < sp->tracks; ctr++)
	{	//For each track
		if(sp->track[ctr]->track_behavior == EOF_GUITAR_TRACK_BEHAVIOR)
		{	//If this is a guitar or bass track
			for(ctr2 = 0; ctr2 < eof_get_track_size(sp, ctr); ctr2++)
			{	//For each note in the track
				if(!(eof_get_note_flags(sp, ctr, ctr2) & EOF_NOTE_FLAG_F_HOPO))
				{	//If this note is not a forced HOPO note
					eof_set_note_flags(sp, ctr, ctr2, (eof_get_note_flags(sp, ctr, ctr2) | EOF_NOTE_FLAG_NO_HOPO));	//Toggle the forced strum (non HOPO) flag for this note
				}
			}
		}
	}

//Update path variables
	(void) ustrcpy(eof_filename, backup_filename);
	(void) replace_filename(eof_song_path, backup_filename, "", 1024);
	(void) replace_filename(eof_last_eof_path, eof_filename, "", 1024);
	(void) ustrcpy(eof_loaded_song_name, get_filename(eof_filename));
	(void) replace_extension(eof_loaded_song_name, eof_loaded_song_name, "eof", 1024);

	cur_time = clock();
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tFeedback import completed in %.2f seconds", (((double)cur_time - (double)start_time) / CLOCKS_PER_SEC));
	eof_log(eof_log_string, 1);

	/* restore the original value of eof_song in case the calling function needed it */
	eof_song = eof_song_backup;

	return sp;
}

struct FeedbackChart *ImportFeedback(const char *filename, int *error)
{
	PACKFILE *inf=NULL;
	char songparsed=0,syncparsed=0,eventsparsed=0;
		//Flags to indicate whether each of the mentioned sections had already been parsed
	unsigned char currentsection=0;			//Will be set to 1 for [Song], 2 for [SyncTrack], 3 for [Events], 4 for an instrument section or 0xFF for an unrecognized section
	size_t maxlinelength=1024;
	char *buffer=NULL,*buffer2=NULL;		//Will be an array large enough to hold the largest line of text from input file
	unsigned long index=0,index2=0;			//Indexes for buffer and buffer2, respectively
	char *substring=NULL,*substring2=NULL;	//Used with strstr() to find tag strings in the input file
	unsigned long A=0,B=0,C=0;				//The first, second and third integer values read from the current line of the file
	int errorstatus=0;						//Passed to ParseLongInt()
	char anchortype=0;						//The anchor type being read in [SyncTrack]
	char notetype=0;						//The note type being read in the instrument track
	char *string1=NULL,*string2=NULL;		//Used to hold strings parsed with Read_db_string()
	char *textbuffer;
	char BOM[]={0xEF,0xBB,0xBF};

//Feedback chart structure variables
	struct FeedbackChart *chart=NULL;
	struct dbAnchor *curanchor=NULL;		//Conductor for the anchor linked list
	struct dbText *curevent=NULL;			//Conductor for the text event linked list
	struct dbNote *curnote=NULL;			//Conductor for the current instrument track's note linked list
	struct dbTrack *curtrack=NULL;			//Conductor for the instrument track linked list, which contains a linked list of notes
	void *temp=NULL;						//Temporary pointer used for storing newly-allocated memory
	static struct FeedbackChart emptychart;	//A static struct has all members auto-initialized to 0/NULL
	static struct dbAnchor emptyanchor;
	static struct dbText emptytext;
	static struct dbNote emptynote;

	unsigned long gemcount = 0;		//Used to count and log the number of gems parsed for each instrument section

	eof_log("ImportFeedback() entered", 1);

//Open file in text mode
	if(!filename)
	{
		return NULL;
	}
	inf=pack_fopen(filename,"rt");	//Open the file to ensure it is readable
	if(inf == NULL)
	{
		if(error)
			*error=1;
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError loading:  Cannot open input .chart file:  \"%s\"", strerror(errno));	//Get the Operating System's reason for the failure
		eof_log(eof_log_string, 1);
		return NULL;
	}
	(void) pack_fclose(inf);

//Initialize chart structure
	chart=(struct FeedbackChart *)malloc_err(sizeof(struct FeedbackChart));	//Allocate memory
	*chart=emptychart;		//Reliably set all member variables to 0/NULL
	chart->resolution=192;	//Default this to 192

	buffer2=(char *)malloc_err(maxlinelength);	//For now, assume that any string parsed from one of the lines in the chart file will fit in this buffer

	eof_log("\tBuffering Feedback chart file into memory", 1);
	textbuffer = (char *)eof_buffer_file(filename, 1, 0);	//Buffer the file into memory, adding a NULL terminator at the end of the buffer
	if(!buffer2 || !textbuffer)
	{
		eof_log("\tCannot allocate memory.", 1);
		if(buffer2)
			free(buffer2);
		if(textbuffer)
			free(textbuffer);
		free(chart);
		return NULL;
	}

//Parse the contents of the file
	while(1)
	{	//While the buffered file hasn't been exhausted
		if(!chart->linesprocessed)
		{	//First line being read
			buffer = ustrtok(textbuffer, "\r\n");	//Initialize the tokenization and get first tokenized line

		//Skip Byte Order Mark if one is found on the first line
			if((buffer[0] == BOM[0]) && (buffer[1] == BOM[1]) && (buffer[2] == BOM[2]))
			{
				eof_log("\tSkipping byte order mark on line 1", 1);
				buffer[0] = buffer[1] = buffer[2] = ' ';	//Overwrite the BOM with spaces
			}
		}
		else
		{	//Subsequent lines
			buffer = ustrtok(NULL, "\r\n");	//Return the next tokenized line
		}

		if(!buffer)	//If a tokenized line of the file was not obtained
			break;

		chart->linesprocessed++;	//Track which line number is being parsed

//Skip leading whitespace
		index=0;	//Reset index counter to beginning
		while(buffer[index] != '\0')
		{
			if((buffer[index] != '\n') && (isspace((unsigned char)buffer[index])))
				index++;	//If this character is whitespace, skip to next character
			else
				break;
		}

		if((buffer[index] == '\0') || (buffer[index] == '{'))
		{	//If this line was empty, or contained characters we're ignoring
			continue;							//Skip ahead to the next line
		}

//Process section header
		if(buffer[index]=='[')	//If the line begins an open bracket, it identifies the start of a section
		{
			substring2=strchr(buffer,']');		//Find first closing bracket
			if(substring2 == NULL)
			{
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Feedback import failed on line #%lu:  Malformed section header (no closing bracket)", chart->linesprocessed);
				eof_log(eof_log_string, 1);
				DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
				if(error)
					*error=3;
				free(textbuffer);
				free(buffer2);
				return NULL;					//Malformed section header, return error
			}

			if(currentsection != 0)				//If a section is already being parsed
			{
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Feedback import failed on line #%lu:  Malformed file (section not closed before another begins)", chart->linesprocessed);
				eof_log(eof_log_string, 1);
				DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
				if(error)
					*error=4;
				free(textbuffer);
				free(buffer2);
				return NULL;					//Malformed file, return error
			}

			substring=strcasestr_spec(buffer,"Song");	//Case insensitive search, returning pointer to after the match
			if(substring && (substring <= substring2))	//If this line contained "Song" followed by "]"
			{
				if(songparsed != 0)					//If a section with this name was already parsed
				{
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Feedback import failed on line #%lu:  Malformed file ([Song] section defined more than once)", chart->linesprocessed);
					eof_log(eof_log_string, 1);
					DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
					if(error)
						*error=5;
					free(textbuffer);
					free(buffer2);
					return NULL;					//Multiple [song] sections, return error
				}
				songparsed=1;
				currentsection=1;	//Track that we're parsing [Song]
			}
			else
			{	//Not a [Song] section
				substring=strcasestr_spec(buffer,"SyncTrack");
				if(substring && (substring <= substring2))	//If this line contained "SyncTrack" followed by "]"
				{
					if(syncparsed != 0)					//If a section with this name was already parsed
					{
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Feedback import failed on line #%lu:  Malformed file ([SyncTrack] section defined more than once)", chart->linesprocessed);
						eof_log(eof_log_string, 1);
						DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
						if(error)
							*error=6;
						free(textbuffer);
						free(buffer2);
						return NULL;					//Multiple [SyncTrack] sections, return error
					}
					syncparsed=1;
					currentsection=2;	//Track that we're parsing [SyncTrack]
				}
				else
				{	//Not a [SyncTrack] section
					substring=strcasestr_spec(buffer,"Events");
					if(substring && (substring <= substring2))	//If this line contained "Events" followed by "]"
					{
						if(eventsparsed != 0)				//If a section with this name was already parsed
						{
							(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Feedback import failed on line #%lu:  Malformed file ([Events] section defined more than once)", chart->linesprocessed);
							eof_log(eof_log_string, 1);
							DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
							if(error)
								*error=7;
							free(textbuffer);
							free(buffer2);
							return NULL;					//Multiple [Events] sections, return error
						}
						eventsparsed=1;
						currentsection=3;	//Track that we're parsing [Events]
					}
					else
					{	//This is an instrument section
						gemcount = 0;	//Reset this counter
						temp=(void *)Validate_db_instrument(buffer);
						if(temp == NULL)					//Not a valid Feedback instrument section name
						{
							(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Invalid instrument section \"%s\" on line #%lu.  Ignoring.", buffer, chart->linesprocessed);
							eof_log(eof_log_string, 1);
							currentsection = 0xFF;	//This section will be ignored
						}
						else
						{	//This is a valid Feedback instrument section name
							currentsection=4;
							chart->tracksloaded++;	//Keep track of how many instrument tracks are loaded

							//Create and insert instrument link in the instrument list
							if(chart->tracks == NULL)					//If the list is empty
							{
								chart->tracks=(struct dbTrack *)temp;	//Point head of list to this link
								curtrack=chart->tracks;					//Point conductor to this link
							}
							else
							{
								assert(curtrack != NULL);				//Put an assertion here to resolve a false positive with Coverity
								curtrack->next=(struct dbTrack *)temp;	//Conductor points forward to this link
								curtrack=curtrack->next;				//Point conductor to this link
							}

							//Initialize instrument link
							curnote=NULL;			//Reset the conductor for the track's note list

							chart->guitartypes |= curtrack->isguitar;	//Cumulatively track the presence of 5 and 6 lane guitar tracks
							chart->basstypes |= curtrack->isbass;		//Ditto for 5 and 6 lane bass tracks
						}
					}
				}//Not a [SyncTrack] section
			}//Not a [Song] section

			continue;				//Skip ahead to the next line
		}//If the line begins an open bracket...

//Process end of section
		if(buffer[index]=='}')	//If the line begins with a closing curly brace, it is the end of the current section
		{
			if(currentsection == 0)				//If no section is being parsed
			{
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Feedback import failed on line #%lu:  Malformed file (unexpected closing curly brace)", chart->linesprocessed);
				eof_log(eof_log_string, 1);
				DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
				if(error)
					*error=9;
				free(textbuffer);
				free(buffer2);
				return NULL;					//Malformed file, return error
			}
			if(currentsection == 4)
			{	//If this was an instrument track
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tParsed instrument section \"%s\":  %lu gems", curtrack->trackname, gemcount);
				eof_log(eof_log_string, 2);
			}
			currentsection=0;
			continue;							//Skip ahead to the next line
		}

//Process normal line input
		if(currentsection != 0xFF)
		{	//If this section was identified and is to be processed
			substring=strchr(buffer,'=');		//Any line within the section is expected to contain an equal sign
			if(substring == NULL)
			{
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Feedback import failed on line #%lu:  Malformed item (missing equal sign)", chart->linesprocessed);
				eof_log(eof_log_string, 1);
				DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
				if(error)
					*error=10;
				free(textbuffer);
				free(buffer2);
				return NULL;					//Invalid section entry, return error
			}
		}

	//Process [Song]
		if(currentsection == 1)
		{
			if(Read_db_string(buffer,&string1,&string2))
			{	//If a valid definition of (string) = (string) or (string) = "(string)" was found
				if(strcasecmp(string1,"Name") == 0)
				{
					if(chart->name)
						free(chart->name);	//If a song name was already imported, free that string
					chart->name=string2;	//Save the song name tag
				}
				else if(strcasecmp(string1,"Artist") == 0)
				{
					if(chart->artist)
						free(chart->artist);	//If a song artist was already imported, free that string
					chart->artist=string2;	//Save the song artist tag
				}
				else if(strcasecmp(string1,"Charter") == 0)
				{
					if(chart->charter)
						free(chart->charter);	//If a chart editor was already imported, free that string
					chart->charter=string2;	//Save the chart editor tag
				}
				else if(strcasecmp(string1,"Offset") == 0)
				{
					chart->offset=atof(string2);
					free(string2);	//This string is no longer used
					string2 = NULL;
				}
				else if(strcasecmp(string1,"Resolution") == 0)
				{
					index2=0;	//Use this as an index for string2
					chart->resolution=(unsigned long)ParseLongInt(string2,&index2,chart->linesprocessed,&errorstatus);	//Parse string2 as a number
					if(errorstatus)						//If ParseLongInt() failed
					{
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Feedback import failed on line #%lu:  Invalid chart resolution", chart->linesprocessed);
						eof_log(eof_log_string, 1);
						DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
						if(error)
							*error=11;
						free(textbuffer);
						free(buffer2);
						free(string1);
						free(string2);
						return NULL;					//Invalid number, return error
					}
					free(string2);	//This string is no longer used
					string2 = NULL;
				}
				else if(strcasecmp(string1,"MusicStream") == 0)
				{
					if(chart->audiofile)
						free(chart->audiofile);	//If the audio file name was already imported, free that string
					chart->audiofile=string2;	//Save the name of the audio file for the chart
				}
				else
				{	//No recognized tag was found
					free(string2);
					string2 = NULL;
				}
				free(string1);	//This string is no longer used
				string1 = NULL;
			}
		}//Process [Song]

	//Process [SyncTrack]
		else if(currentsection == 2)
		{	//# = ID # is expected
		//Load first number
			A=(unsigned long)ParseLongInt(buffer,&index,chart->linesprocessed,&errorstatus);
			if(errorstatus)						//If ParseLongInt() failed
			{
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Feedback import failed on line #%lu:  Invalid sync item position", chart->linesprocessed);
				eof_log(eof_log_string, 1);
				DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
				if(error)
					*error=12;
				free(textbuffer);
				free(buffer2);
				return NULL;					//Invalid number, return error
			}
			if(A > chart->chart_length)
				chart->chart_length = A;		//Track the highest used chart position

		//Skip whitespace and parse to equal sign
			index=0;	//Reset index, to parse an int from after the equal sign
			while(substring[index] != '\0')
			{
				if((substring[index] != '\n') && (isspace((unsigned char)substring[index])))
					index++;	//If this character is whitespace, skip to next character
				else
					break;
			}

		//Skip equal sign
			if(substring[index++] != '=')	//Check the character and increment index
			{
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Feedback import failed on line #%lu:  Malformed sync item (missing equal sign)", chart->linesprocessed);
				eof_log(eof_log_string, 1);
				DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
				if(error)
					*error=13;
				free(textbuffer);
				free(buffer2);
				return NULL;				//Invalid SyncTrack entry, return error
			}

		//Skip whitespace and parse to event ID
			while(substring[index] != '\0')
			{
				if((substring[index] != '\n') && (isspace((unsigned char)substring[index])))
					index++;	//If this character is whitespace, skip to next character
				else
					break;
			}

			anchortype=toupper(substring[index]);	//Store as uppercase
			if((anchortype == 'A') || (anchortype == 'B'))
				index++;	//Advance to next whitespace character
			else if(anchortype == 'T')
			{
				if(substring[index+1] != 'S')		//If the next character doesn't complete the anchor type
				{
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Feedback import failed on line #%lu:  Invalid sync item type \"T%c\"", chart->linesprocessed, substring[index+1]);
					eof_log(eof_log_string, 1);
					DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
					if(error)
						*error=14;
					free(textbuffer);
					free(buffer2);
					return NULL;					//Invalid anchor type, return error
				}
				index+=2;	//This anchor type is two characters instead of one
			}
			else
			{
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Feedback import failed on line #%lu:  Invalid sync item type \"%c\"", chart->linesprocessed, anchortype);
				eof_log(eof_log_string, 1);
				DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
				if(error)
					*error=15;
				free(textbuffer);
				free(buffer2);
				return NULL;					//Invalid anchor type, return error
			}

		//Load second number
			B=(unsigned long)ParseLongInt(substring,&index,chart->linesprocessed,&errorstatus);
			if(errorstatus)						//If ParseLongInt() failed
			{
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Feedback import failed on line #%lu:  Invalid sync item parameter", chart->linesprocessed);
				eof_log(eof_log_string, 1);
				DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
				if(error)
					*error=16;
				free(textbuffer);
				free(buffer2);
				return NULL;					//Invalid number, return error
			}

		//If this is a time signature change, check for a second parameter, which Moonscraper uses to define the denominator (the power to which 2 is raised)
			if(anchortype == 'T')
			{
				C=(unsigned long)ParseLongInt(substring,&index,chart->linesprocessed,&errorstatus);
				if(errorstatus)
				{	//If another number was not read on this line
					C = 2;	//Assume 4 (2^2 == 4)
					errorstatus = 0;
				}
				C = 1 << C;
			}

		//If this anchor event has the same chart time as the last, just write this event's information with the last's
			if(curanchor && (curanchor->chartpos == A))
			{
				if(anchortype == 'A')		//If this is an Anchor event
					curanchor->usec=B;		//Store the anchor realtime position
				else if(anchortype == 'B')	//If this is a Tempo event
					curanchor->BPM=B;		//Store the tempo
				else if(anchortype == 'T')	//If this is a Time Signature event
				{
					curanchor->TSN = B;		//Store the Time Signature
					curanchor->TSD = C;
				}
				else
				{
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Feedback import failed on line #%lu:  Invalid sync item type \"%c\"", chart->linesprocessed, anchortype);
					eof_log(eof_log_string, 1);
					DestroyFeedbackChart(chart,1);
					if(error)
						*error=17;
					free(textbuffer);
					free(buffer2);
					return NULL;			//Invalid anchor type, return error
				}
			}
			else
			{
		//Create and insert anchor link into the anchor list
				temp=malloc_err(sizeof(struct dbAnchor));	//Allocate memory
				*((struct dbAnchor *)temp)=emptyanchor;		//Reliably set all member variables to 0/NULL
				if(chart->anchors == NULL)					//If the list is empty
				{
					chart->anchors=(struct dbAnchor *)temp;	//Point head of list to this link
					curanchor=chart->anchors;				//Point conductor to this link
				}
				else
				{
					assert(curanchor != NULL);					//Put an assertion here to resolve a false positive with Coverity
					curanchor->next=(struct dbAnchor *)temp;	//Conductor points forward to this link
					curanchor=curanchor->next;					//Point conductor to this link
				}

		//Initialize anchor link
				curanchor->chartpos=A;	//The first number read is the chart position
				switch(anchortype)
				{
					case 'B':				//If this was a tempo event
						curanchor->BPM=B;	//The second number represents 1000 times the tempo
					break;

					case 'T':				//If this was a time signature event
						curanchor->TSN=B;	//Store the numerator of the time signature
						curanchor->TSD=C;	//Store the denominator of the time signature
					break;

					case 'A':				//If this was an anchor event
						curanchor->usec=B;	//Store the anchor's timestamp in microseconds
					break;

					default:
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Feedback import failed on line #%lu:  Invalid sync item type \"%c\"", chart->linesprocessed, anchortype);
						eof_log(eof_log_string, 1);
						DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
						if(error)
							*error=18;
					free(textbuffer);
					free(buffer2);
					return NULL;			//Invalid anchor type, return error
				}
			}
		}//Process [SyncTrack]

	//Process [Events]
		else if(currentsection == 3)
		{	//# = E "(STRING)" is expected
		//Load first number
			A=ParseLongInt(buffer,&index,chart->linesprocessed,&errorstatus);
			if(errorstatus)						//If ParseLongInt() failed
			{
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Feedback import failed on line #%lu:  Invalid event time position", chart->linesprocessed);
				eof_log(eof_log_string, 1);
				DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
				if(error)
					*error=19;
				free(textbuffer);
				free(buffer2);
				return NULL;					//Invalid number, return error
			}
			if(A > chart->chart_length)
				chart->chart_length = A;		//Track the highest used chart position

		//Skip whitespace and parse to equal sign
			index=0;	//Reset index, to parse an int from after the equal sign
			while(substring[index] != '\0')
			{
				if((substring[index] != '\n') && (isspace((unsigned char)substring[index])))
					index++;	//If this character is whitespace, skip to next character
				else
					break;
			}

		//Skip equal sign
			if(substring[index++] != '=')	//Check the character and increment index
			{
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Feedback import failed on line #%lu:  Malformed event item (missing equal sign)", chart->linesprocessed);
				eof_log(eof_log_string, 1);
				DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
				if(error)
					*error=20;
				free(textbuffer);
				free(buffer2);
				return NULL;				//Invalid Event entry, return error
			}

		//Skip whitespace and parse to event ID
			while(substring[index] != '\0')
			{
				if((substring[index] != '\n') && (isspace((unsigned char)substring[index])))
					index++;	//If this character is whitespace, skip to next character
				else
					break;
			}

			if(substring[index++] == 'E')	//Check if this is a text event indicator (and increment index)
			{
			//Seek to opening quotation mark
				while((substring[index] != '\0') && (substring[index] != '"'))
					index++;

				if(substring[index++] != '"')		//Check if this was a null character instead of quotation mark (and increment index)
				{
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Feedback import failed on line #%lu:  Malformed event item (missing open quotation mark)", chart->linesprocessed);
					eof_log(eof_log_string, 1);
					DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
					if(error)
						*error=21;
					free(textbuffer);
					free(buffer2);
					return NULL;					//Invalid Event entry, return error
				}

			//Load string by copying all characters to the second buffer (up to the next quotation mark)
				buffer2[0]='\0';	//Truncate string
				index2=0;			//Reset buffer2's index
				while(substring[index] != '"')			//For all characters up to the next quotation mark
				{
					if(substring[index] == '\0')		//If a null character is reached unexpectedly
					{
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Feedback import failed on line #%lu:  Malformed event item (missing close quotation mark)", chart->linesprocessed);
						eof_log(eof_log_string, 1);
						DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
						if(error)
							*error=22;
						free(textbuffer);
						free(buffer2);
						return NULL;					//Invalid Event entry, return error
					}
					buffer2[index2++]=substring[index++];	//Copy the character to the second buffer, incrementing both indexes
				}
				buffer2[index2]='\0';	//Truncate the second buffer to form a complete string

			//Create and insert event link into event list
				temp=malloc_err(sizeof(struct dbText));		//Allocate memory
				*((struct dbText *)temp)=emptytext;			//Reliably set all member variables to 0/NULL
				if(chart->events == NULL)					//If the list is empty
				{
					chart->events=(struct dbText *)temp;	//Point head of list to this link
					curevent=chart->events;					//Point conductor to this link
				}
				else
				{
					assert(curevent != NULL);				//Put an assertion here to resolve a false positive with Coverity
					curevent->next=(struct dbText *)temp;	//Conductor points forward to this link
					curevent=curevent->next;				//Point conductor to this link
				}

			//Initialize event link- Duplicate buffer2 into a newly created dbText link, adding it to the list
				curevent->chartpos=A;						//The first number read is the chart position
				curevent->text=DuplicateString(buffer2);	//Copy buffer2 to new string and store in list
			}
			else
			{	//This is not a recognized event entry
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Ignoring unrecognized event on line #%lu:  \"%s\"", chart->linesprocessed, buffer);
				eof_log(eof_log_string, 1);
			}
		}//Process [Events]

	//Process instrument tracks
		else if(currentsection == 4)
		{	//"# = N # #" or "# = S # #" is expected
		//Load first number
			A=ParseLongInt(buffer,&index,chart->linesprocessed,&errorstatus);
			if(errorstatus)						//If ParseLongInt() failed
			{
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Feedback import failed on line #%lu:  Invalid gem item position", chart->linesprocessed);
				eof_log(eof_log_string, 1);
				DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
				if(error)
					*error=23;
				free(textbuffer);
				free(buffer2);
				return NULL;					//Invalid number, return error
			}
			if(A > chart->chart_length)
				chart->chart_length = A;		//Track the highest used chart position

		//Skip whitespace and parse to equal sign
			index=0;	//Reset index, to parse an int from after the equal sign
			while(substring[index] != '\0')
			{
				if((substring[index] != '\n') && (isspace((unsigned char)substring[index])))
					index++;	//If this character is whitespace, skip to next character
				else
					break;
			}

		//Skip equal sign
			if(substring[index++] != '=')	//Check the character and increment index
			{
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Feedback import failed on line #%lu:  Malformed gem item (missing equal sign)", chart->linesprocessed);
				eof_log(eof_log_string, 1);
				DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
				if(error)
					*error=24;
				free(textbuffer);
				free(buffer2);
				return NULL;				//Invalid instrument entry, return error
			}

		//Skip whitespace and parse to event ID
			while(substring[index] != '\0')
			{
				if((substring[index] != '\n') && (isspace((unsigned char)substring[index])))
					index++;	//If this character is whitespace, skip to next character
				else
					break;
			}

			notetype=0;	//By default, assume this is going to be a note definition
			switch(substring[index])
			{
				case 'S':		//Player/overdrive section
					notetype=1;	//This is a section marker
					index++; 	//Increment index past N or S identifier
				break;

				case 'E':		//Text event
					if(strstr(&substring[index + 1], "*"))
					{	//If the E is followed by an asterisk
						notetype=2;	//This is a toggle HOPO marker
					}
					else if(strstr(&substring[index + 1], "O"))
					{	//If the E is followed by the letter O
						char *check = strcasestr_spec(substring, "= E O");	//Obtain the next character after this sequence of characters
						if(check && (*check == '\0'))
						{	//If that's the sequence this line ended in
							notetype=3;	//This is an open strum marker
						}
						else
							continue;	//Otherwise ignore this event
					}
					else if(strcasestr_spec(substring, "= E soloend"))
					{
						notetype='E';	//This is a Clone Hero end of solo marker
					}
					else if(strcasestr_spec(substring, "= E solo"))
					{
						notetype='S';	//This is a Clone Hero start of solo marker
					}
					else
					{
						continue;
					}
				break;

				case 'N':		//Note indicator, and increment index
					index++; 	//Increment index past N or S identifier
				break;

				default:
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Feedback import failed on line #%lu:  Invalid gem item type \"%c\"", chart->linesprocessed, substring[index]);
					eof_log(eof_log_string, 1);
					DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
					if(error)
						*error=25;
				free(textbuffer);
				free(buffer2);
				return NULL;		//Invalid instrument entry, return error
			}

			if(notetype == 2)
			{	//If this is a toggle HOPO marker, treat it as a note authored as "N 5 0"
				notetype=0;
				B=5;
				C=0;
			}
			else if(notetype == 3)
			{	//If this is an open strum marker, treat it as a note authored as "N 7 0"
				notetype=0;
				B=7;
				C=0;
			}
			else if(notetype == 'S')
			{	//If this is a Clone Hero start of solo marker
				notetype=0;
				B='S';
				C=0;
			}
			else if(notetype == 'E')
			{	//If this is a Clone Hero end of solo marker
				notetype=0;
				B='E';
				C=0;
			}
			else
			{	//Otherwise parse the second and third parameters
			//Load second number
				B=ParseLongInt(substring,&index,chart->linesprocessed,&errorstatus);
				if(errorstatus)						//If ParseLongInt() failed
				{
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Feedback import failed on line #%lu:  Invalid gem item parameter 1", chart->linesprocessed);
					eof_log(eof_log_string, 1);
					DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
					if(error)
						*error=26;
					free(textbuffer);
					free(buffer2);
					return NULL;					//Invalid number, return error
				}

			//Load third number
				C=ParseLongInt(substring,&index,chart->linesprocessed,&errorstatus);
				if(errorstatus)						//If ParseLongInt() failed
				{
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Feedback import failed on line #%lu:  Invalid gem item parameter 2", chart->linesprocessed);
					eof_log(eof_log_string, 1);
					DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
					if(error)
						*error=27;
					free(textbuffer);
					free(buffer2);
					return NULL;					//Invalid number, return error
				}
			}

		//Create a note link and add it to the current Note list
			if(curtrack == NULL)				//If the instrument track linked list is not initialized
			{
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Feedback import failed on line #%lu:  Malformed file (gem defined outside instrument track)", chart->linesprocessed);
				eof_log(eof_log_string, 1);
				DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
				if(error)
					*error=28;
				free(textbuffer);
				free(buffer2);
				return NULL;					//Malformed file, return error
			}

			temp=malloc_err(sizeof(struct dbNote));		//Allocate memory
			*((struct dbNote *)temp)=emptynote;			//Reliably set all member variables to 0/NULL
			if(curtrack->notes == NULL)					//If the list is empty
			{
				curtrack->notes=(struct dbNote *)temp;	//Point head of list to this link
				curnote=curtrack->notes;				//Point conductor to this link
			}
			else
			{
				assert(curnote != NULL);				//Put an assertion here to resolve a false positive with Coverity
				curnote->next=(struct dbNote *)temp;	//Conductor points forward to this link
				curnote=curnote->next;					//Point conductor to this link
			}

		//Initialize note link
			curnote->chartpos=A;	//The first number read is the chart position

			if(!notetype)				//This was a note definition
				curnote->gemcolor=B;	//The second number read is the gem color
			else						//This was a player section marker
			{
				if(B > 4)	//Only values of 0, 1 or 2 are valid for player section markers, 3 and 4 are unknown but will be kept and ignored for now during transfer to EOF
				{
					if(B == 64)
					{	//Drum fill
						curnote->gemcolor = 69;
					}
					else if(B == 65)
					{	//Drum roll
						curnote->gemcolor = 70;
					}
					else if(B == 66)
					{	//Special drum roll
						curnote->gemcolor = 71;
					}
					else
					{
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Ignoring unrecognized section marker #%lu on line #%lu.", B, chart->linesprocessed);
						eof_log(eof_log_string, 1);
						continue;
					}
				}
				else
					curnote->gemcolor='0'+B;	//Store 0 as '0', 1 as '1' or 2 as '2', ...
			}
			if((curnote->gemcolor <= 8) && (curnote->gemcolor != 5) && (curnote->gemcolor != 6))
			{	//If this gem is value 0-4, 7 or 8
				curnote->is_gem = 1;
			}
			curnote->duration=C;			//The third number read is the note duration
			if(A + C > chart->chart_length)
				chart->chart_length = A + C;		//Track the highest used chart position
			gemcount++;						//Track the number of gems parsed and stored into the list
		}//Process instrument tracks

	//Error: Content in file outside of a defined section
		else if(currentsection != 0xFF)
		{	//If this isn't a section that is being ignored
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Feedback import failed on line #%lu:  Malformed file (item defined outside of track)", chart->linesprocessed);
			eof_log(eof_log_string, 1);
			DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
			if(error)
				*error=30;
			free(textbuffer);
			free(buffer2);
			return NULL;					//Malformed file, return error
		}
	}//Until end of file is reached

	if(error)
		*error=0;

	free(textbuffer);
	free(buffer2);

	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tEnd of import.  Highest used chart position detected as %lu", chart->chart_length);
	eof_log(eof_log_string, 2);

	return chart;
}

int Read_db_string(char *source, char **str1, char **str2)
{
	//Scans the source string for a valid dB tag: text = text	or	text = "text"
	//The text to the left of the equal sign is returned through str1 as a new string, with whitespace truncated
	//The text to the right of the equal sign is returned through str2 as a new string, with whitespace truncated
	//If the first non whitespace character encountered after the equal sign is a quotation mark, all characters after
	//that quotation mark up to the next are returned through str2
	//Nonzero is returned upon success, or zero is returned if source did not contain two sets of non whitespace characters
	//separated by an equal sign character, or if the closing quotation mark is missing.

//	eof_log("Read_db_string() entered");

	unsigned long srcindex;	//Index into source string
	unsigned long index;	//Index into destination strings
	char *string1,*string2;	//Working strings
	char findquote=0;	//Boolean:	The element to the right of the equal sign began with a quotation mark
				//		str2 will stop filling with characters when the next quotation mark is read

	if(!source || !str1 || !str2)
	{
		return 0;	//return error
	}

//Allocate memory for strings
	string1=malloc_err(strlen(source)+1);
	string2=malloc_err(strlen(source)+2);

//Parse the string to the left of the expected equal sign into string1
	index=0;
	for(srcindex=0;source[srcindex] != '=';srcindex++)	//Parse characters until equal sign is found
	{
		if(source[srcindex] == '\0')	//If the string ended unexpectedly
		{
			free(string1);
			free(string2);
			return 0;		//return error
		}

		string1[index++]=source[srcindex];	//Append character to string1 and increment index into string
	}
	string1[index]='\0';	//Truncate string1
	srcindex++;		//Seek past the equal sign
	string1=TruncateString(string1,1);	//Re-allocate string to remove leading and trailing whitespace

//Skip leading whitespace
	while(source[srcindex] != '\0')
	{
		if(!isspace(source[srcindex]))	//Character is not whitespace and not NULL character
		{
			if(source[srcindex] == '"')	//If the first character after the = is found to be a quotation mark
			{
				srcindex++;			//Seek past the quotation mark
				findquote=1;		//Expect another quotation mark to end the string
			}

			break;	//Exit while loop
		}
		srcindex++;	//Increment to look at next character
	}

//Parse the string to the right of the equal sign into string2
	index=0;
	while(source[srcindex] != '\0')	//There was nothing but whitespace after the equal sign
	{
		if(findquote && (source[srcindex] == '"'))	//If we should stop at this quotation mark
			break;
		string2[index++]=source[srcindex++];	//Append character to string1 and increment both indexes
	}

	string2[index]='\0';	//Truncate string2

	if(index)	//If there were characters copied to string2
		string2=TruncateString(string2,1);	//Re-allocate string to remove the trailing whitespace

	*str1=string1;		//Return string1 through pointer
	*str2=string2;		//Return string2 through pointer
	return 1;		//Return success
}

int eof_validate_db_track_diff_string(char *diffstring, struct dbTrack *track)
{
	char *ptr;

	if(!diffstring || !track)
		return 0;	//Invalid parameters

	//Initialize chart variables
	track->tracktype = 0;	//Unless specified otherwise, this track will not import
	track->isguitar = track->isdrums = track->isbass = 0;

	//Check for supported instrument difficulties
	ptr = strcasestr_spec(diffstring,"Single]");
	if(ptr && (ptr[0] == '\0'))
	{	//If the string ends in "Single]"
		track->tracktype = 1;	//Track that this is a "Guitar" track
		track->isguitar = 1;
		return 1;	//Return success
	}

	ptr = strcasestr_spec(diffstring,"SingleGuitar]");
	if(ptr && (ptr[0] == '\0'))
	{	//If the string ends in "SingleGuitar]"
		track->tracktype = 1;	//Track that this is a "Guitar" track
		track->isguitar = 1;
		return 1;	//Return success
	}

	ptr = strcasestr_spec(diffstring,"DoubleGuitar]");
	if(ptr && (ptr[0] == '\0'))
	{	//If the string ends in "DoubleGuitar]"
		track->tracktype = 2;	//Track that this is a "Lead Guitar" track
		track->isguitar = 1;
		return 1;	//Return success
	}

	ptr = strcasestr_spec(diffstring,"DoubleBass]");
	if(ptr && (ptr[0] == '\0'))
	{	//If the string ends in "DoubleBass]"
		track->tracktype = 3;	//Track that this is a "Bass" track
		track->isbass = 1;
		return 1;	//Return success
	}

	ptr = strcasestr_spec(diffstring,"SingleRhythm]");
	if(ptr && (ptr[0] == '\0'))
	{	//If the string ends in "SingleRhythm]"
		track->tracktype = 3;	//Track that this is a "Bass" track
		track->isbass = 1;
		return 1;	//Return success
	}

	ptr = strcasestr_spec(diffstring,"Drums]");
	if(ptr && (ptr[0] == '\0'))
	{	//If the string ends in "Drums]"
		track->tracktype = 4;	//Track that this is a "Drums" track
		track->isdrums = 1;
		return 1;	//Return success
	}

	ptr = strcasestr_spec(diffstring,"DoubleDrums]");
	if(ptr && (ptr[0] == '\0'))
	{	//If the string ends in "DoubleDrums]"
		track->tracktype = 4;	//Doubledrums is an expert+ drum track
		track->isdrums = 2;	//This will be imported into the Phase Shift drum track instead of the normal drum track
		return 1;	//Return success
	}

	ptr = strcasestr_spec(diffstring,"Vocals]");
	if(ptr && (ptr[0] == '\0'))
	{	//If the string ends in "Vocals]"
		track->tracktype = 5;	//Track that this is a "Vocals" track
		track->isdrums = 0;
		return 1;	//Return success
	}

	ptr = strcasestr_spec(diffstring,"DoubleRhythm]");
	if(ptr && (ptr[0] == '\0'))
	{	//If the string ends in "DoubleRhythm]"
		track->tracktype = 6;	//Track that this is a "Rhythm" track
		track->isguitar = 1;
		return 1;	//Return success
	}

	ptr = strcasestr_spec(diffstring,"Keyboard]");
	if(ptr && (ptr[0] == '\0'))
	{	//If the string ends in "Keyboard]"
		track->tracktype = 7;	//Track that this is a keyboard track
		return 1;	//Return success
	}

	//Check for 6 lane tracks, authored by MoonScraper for use in Clone Hero
	ptr = strcasestr_spec(diffstring,"GHLGuitar]");
	if(ptr && (ptr[0] == '\0'))
	{	//If the string ends in "GHLGuitar]"
		track->tracktype = 1;	//Track that this is a "Guitar" track
		track->isguitar = 2;	//This signals that this is a 6 lane guitar track
		return 1;	//Return success
	}

	ptr = strcasestr_spec(diffstring,"GHLBass]");
	if(ptr && (ptr[0] == '\0'))
	{	//If the string ends in "GHLBass]"
		track->tracktype = 3;	//Track that this is a "Bass" track
		track->isbass = 2;		//This signals that this is a 6 lane bass track
		return 1;	//Return success
	}

	//Check for other instrument difficulties that are valid but not supported
	ptr = strcasestr_spec(diffstring,"EnhancedGuitar]");
	if(ptr && (ptr[0] == '\0'))
	{	//If the string ends in "EnhancedGuitar]"
		track->isguitar = 1;	//EnhancedGuitar is a guitar track, but it won't be imported
		return 1;	//Return success
	}

	ptr = strcasestr_spec(diffstring,"CoopLead]");
	if(ptr && (ptr[0] == '\0'))
	{	//If the string ends in "CoopLead]"
		track->isguitar = 1;	//CoopLead is a guitar track, but it won't be imported
		return 1;	//Return success
	}

	ptr = strcasestr_spec(diffstring,"CoopBass]");
	if(ptr && (ptr[0] == '\0'))
	{	//If the string ends in "CoopBass]"
		track->isguitar = 1;	//CoopBass is a guitar track, but it won't be imported
		return 1;	//Return success
	}

	ptr = strcasestr_spec(diffstring,"10KeyGuitar]");
	if(ptr && (ptr[0] == '\0'))
	{	//If the string ends in "10KeyGuitar]"
		track->isguitar = 1;	//10KeyGuitar is a guitar track, but it won't be imported
		return 1;	//Return success
	}

	return 0;	//Return failure
}

struct dbTrack *Validate_db_instrument(char *buffer)
{
	//Validates that buffer contains a valid dB instrument track name enclosed in brackets []
	//buffer is expected to point to the opening bracket
	//If it is valid, a dbTrack structure is allocated and initialized:
	//(track name is allocated, tracktype and difftype are set and the linked lists are set to NULL)
	//The track structure is returned, otherwise NULL is returned if the string did not contain a valid
	//track name.  buffer[] is modified to remove any whitespace after the closing bracket

	unsigned long index=0;	//Used to index into buffer
	char *endbracket=NULL;	//The pointer to the end bracket
	char *diffstring=NULL;	//Used to find the difficulty substring
	char *retstring=NULL;	//Used to create the instrument track string
	struct dbTrack *track=NULL;		//Used to create the structure that is returned
	static struct dbTrack emptychart;	//Static structures have all members auto initialize to 0/NULL
	char difftype=0;	//Used to track the difficulty of the track

	eof_log("Validate_db_instrument() entered", 1);

	if(buffer == NULL)
		return NULL;	//Return error

//Validate the opening bracket, to which buffer is expected to point
	if(buffer[0] != '[')	//If the opening bracket is missing
		return NULL;	//Return error

//Validate the presence of the closing bracket
	endbracket=strchr(buffer,']');
	if(endbracket == NULL)
		return NULL;	//Return error

//Verify that no non whitespace characters exist after the closing bracket
	index=1;	//Reset index to point to the character after the closing bracket
	while(endbracket[index] != '\0')
		if(!isspace(endbracket[index++]))	//Check if this character isn't whitespace (and increment index)
			return NULL;			//If it isn't whitespace, return error

//Truncate whitespace after closing bracket
	endbracket[1]='\0';	//Write a NULL character after the closing bracket

//Verify that a valid diffulty is specified, seeking past the opening bracket pointed to by buffer[0]
	//Test for Easy
	diffstring=strcasestr_spec(&buffer[1],"Easy");
	if(diffstring == NULL)
	{
	//Test for Medium
		diffstring=strcasestr_spec(&buffer[1],"Medium");
		if(diffstring == NULL)
		{
	//Test for Hard
			diffstring=strcasestr_spec(&buffer[1],"Hard");
			if(diffstring == NULL)
			{
	//Test for Expert
				diffstring=strcasestr_spec(&buffer[1],"Expert");
				if(diffstring == NULL)	//If none of the four valid difficulty strings were found
					return NULL;	//Return error

				difftype=4;	//Track that this is an Expert difficulty
			}
			else
				difftype=3;	//Track that this is a Hard difficulty
		}
		else
			difftype=2;	//Track that this is a Medium difficulty
	}
	else
		difftype=1;	//Track that this is is an Easy difficulty

//Create and initialize the instrument structure
	track=malloc_err(sizeof(struct dbTrack));	//Allocate memory
	*track=emptychart;							//Reliably set all member variables to 0/NULL

//At this point, diffstring points to the character AFTER the matching difficulty string.  Verify that a valid instrument is specified
	if(!eof_validate_db_track_diff_string(diffstring, track))
	{	//If the instrument difficulty is not recognized
		free(track);
		return NULL;	//Return error
	}

//Create a new string containing the instrument name, minus the brackets
	retstring=DuplicateString(&buffer[1]);
	retstring[strlen(retstring)-1]='\0';	//Truncate the trailing bracket

	track->trackname=retstring;					//Store the instrument track name
	track->difftype=difftype;
	return track;
}

void DestroyFeedbackChart(struct FeedbackChart *ptr, char freestruct)
{
	struct dbAnchor *anchorptr;	//Conductor for the anchors linked list
	struct dbText *eventptr;	//Conductor for the events linked list
	struct dbTrack *trackptr;	//Conductor for the tracks linked list
	struct dbNote *noteptr;	//Conductor for the notes linked lists

	eof_log("DestroyFeedbackChart() entered", 1);

	if(!ptr)
	{
		return;
	}

//Free and re-init tags
	if(ptr->name)
	{
		free(ptr->name);
		ptr->name=NULL;
	}
	if(ptr->artist)
	{
		free(ptr->artist);
		ptr->artist=NULL;
	}
	if(ptr->charter)
	{
		free(ptr->charter);
		ptr->charter=NULL;
	}
	if(ptr->audiofile)
	{
		free(ptr->audiofile);
		ptr->audiofile=NULL;
	}

//Re-init variables
	ptr->offset=ptr->resolution=ptr->linesprocessed=ptr->tracksloaded=0;

//Empty anchors list
	while(ptr->anchors != NULL)
	{
		anchorptr=ptr->anchors->next;	//Store link to next anchor
		free(ptr->anchors);		//Free current anchor
		ptr->anchors=anchorptr;	//Point to next anchor
	}

//Empty events list
	while(ptr->events != NULL)
	{
		eventptr=ptr->events->next;	//Store link to next event
		free(ptr->events->text);	//Free event text
		free(ptr->events);		//Free current event
		ptr->events=eventptr;		//Point to next event
	}

//Empty tracks list
	while(ptr->tracks != NULL)
	{
		trackptr=ptr->tracks->next;	//Store link to next instrument track

		while(ptr->tracks->notes != NULL)
		{
			noteptr=ptr->tracks->notes->next;	//Store link to next note
			free(ptr->tracks->notes);		//Free current note
			ptr->tracks->notes=noteptr;		//Point to next note
		}

		free(ptr->tracks->trackname);	//Free track name
		free(ptr->tracks);		//Free current track
		ptr->tracks=trackptr;		//Point to next track
	}

//Optionally free the passed Feedback chart structure itself
	if(freestruct)
		free(ptr);
}

unsigned long FindLongestLineLength_ALLEGRO(const char *filename, char exit_on_empty)
{
	unsigned long maxlinelength=0;
	unsigned long ctr=0;
	int inputchar=0;
	PACKFILE *inf;

	if(filename == NULL)
		return 0;	//Return error
	inf = pack_fopen(filename, "rt");
	if(inf == NULL)
	{	//File open failed
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError loading:  Cannot open input file:  \"%s\"", strerror(errno));	//Get the Operating System's reason for the failure
		eof_log(eof_log_string, 1);
		return 0;	//Return error
	}

	do{
		ctr=0;			//Reset line length counter
		do{
			inputchar=pack_getc(inf);	//get a character, do not exit on EOF
			ctr++;					//increment line length counter
		}while((inputchar != EOF) && (inputchar != '\n'));//Repeat until end of file or newline character is read

		if(ctr > maxlinelength)		//If this line was the longest yet,
			maxlinelength=ctr;	//Store its length
	}while(inputchar != EOF);	//Repeat until end of file is reached

	if(maxlinelength < 2)		//If the input file contained nothing but empty lines or no text at all
	{
		if(!exit_on_empty)
		{
			(void) pack_fclose(inf);
			return 0;
		}

		(void) puts("Error: File is empty\nAborting");
		exit_wrapper(2);
	}
	maxlinelength++;		//Must increment this to account for newline character

	(void) pack_fclose(inf);
	return maxlinelength;
}

void sort_chart(struct FeedbackChart *chart)
{
	char sorted;
	struct dbAnchor *anchor_ptr, *prev_anchor_ptr;
	struct dbText *text_ptr, *prev_text_ptr;
	struct dbTrack *track_ptr;
	struct dbNote *note_ptr, *prev_note_ptr;

	if(!chart)
		return;	//Invalid parameter

	//Sort anchors
	do{
		sorted = 1;
		for(anchor_ptr = chart->anchors, prev_anchor_ptr = NULL; anchor_ptr != NULL; anchor_ptr = anchor_ptr->next)
		{	//For each anchor in the linked list
			if(anchor_ptr->next && (anchor_ptr->next->chartpos < anchor_ptr->chartpos))
			{	//If there's another anchor and it should sort before this one
				struct dbAnchor *this_ptr = anchor_ptr, *next_ptr = anchor_ptr->next;

				#ifdef CCDEBUG
					(void) printf("\tSorting anchor (Delta pos = %lu, tempo = %lu, TS = %u/%u, usec = %lu) before anchor (Delta pos = %lu, tempo = %lu, TS = %u/%u, usec = %lu).\n", anchor_ptr->chartpos, anchor_ptr->BPM, anchor_ptr->TSN, anchor_ptr->TSD, anchor_ptr->usec, anchor_ptr->next->chartpos, anchor_ptr->next->BPM, anchor_ptr->next->TSN, anchor_ptr->next->TSD, anchor_ptr->next->usec);
				#endif

				this_ptr->next = next_ptr->next;
				next_ptr->next = this_ptr;
				if(prev_anchor_ptr == NULL)
				{	//If the head link of the list was swapped
					chart->anchors = next_ptr;			//Update the linked list pointer in the chart structure
				}
				else
				{
					prev_anchor_ptr->next = next_ptr;	//Point the previous link forward to the earlier of the two links just sorted
				}
				sorted = 0;
				break;	//Restart the for loop
			}
			prev_anchor_ptr = anchor_ptr;
		}
	}while(!sorted);

	//Sort events
	do{
		sorted = 1;
		for(text_ptr = chart->events, prev_text_ptr = NULL; text_ptr != NULL; text_ptr = text_ptr->next)
		{	//For each text event in the linked list
			if(text_ptr->next && (text_ptr->next->chartpos < text_ptr->chartpos))
			{	//If there's another text event and it should sort before this one
				struct dbText *this_ptr = text_ptr, *next_ptr = text_ptr->next;

				#ifdef CCDEBUG
					(void) printf("\tSorting text event (Delta pos = %lu, \"%s\") before text event (Delta pos = %lu, \"%s\".\n", text_ptr->chartpos, text_ptr->text, text_ptr->next->chartpos, text_ptr->next->text);
				#endif

				this_ptr->next = next_ptr->next;
				next_ptr->next = this_ptr;
				if(prev_text_ptr == NULL)
				{	//If the head link of the list was swapped
					chart->events = next_ptr;	//Update the linked list pointer in the chart structure
				}
				else
				{
					prev_text_ptr->next = next_ptr;	//Point the previous link forward to the earlier of the two links just sorted
				}
				sorted = 0;
				break;	//Restart the for loop
			}
			prev_text_ptr = text_ptr;
		}
	}while(!sorted);

	//Sort track content
	for(track_ptr = chart->tracks; track_ptr != NULL; track_ptr = track_ptr->next)
	{	//For each track
		#ifdef CCDEBUG
			(void) printf("\tSorting track %s.\n", track_ptr->trackname);
		#endif
		//Sort notes
		do{
			sorted = 1;
			for(note_ptr = track_ptr->notes, prev_note_ptr = NULL; note_ptr != NULL; note_ptr = note_ptr->next)
			{	//For each note in the linked list
				if(note_ptr->next)
				{	//If there's another note and it should sort before this one
					int swap = 0;

					if(note_ptr->next->chartpos < note_ptr->chartpos)
					{	//If the next note occurs before this note
						swap = 1;
					}
					if((note_ptr->next->chartpos == note_ptr->chartpos) && (note_ptr->next->gemcolor > note_ptr->gemcolor))
					{	//If the two notes have the same position, but the next one has a higher lane (to help reliably sort toggle HOPO markers before the gems they affect, suiting a singly linked list)
						swap = 1;
					}
					if(swap)
					{
						struct dbNote *this_ptr = note_ptr, *next_ptr = note_ptr->next;

						this_ptr->next = next_ptr->next;
						next_ptr->next = this_ptr;
						if(prev_note_ptr == NULL)
						{	//If the head link of the list was swapped
							track_ptr->notes = next_ptr;	//Update the linked list pointer in the track structure
						}
						else
						{
							prev_note_ptr->next = next_ptr;	//Point the previous link forward to the earlier of the two links just sorted
						}
						sorted = 0;
						break;	//Restart the for loop
					}
				}
				prev_note_ptr = note_ptr;
			}
		}while(!sorted);
	}
}

void eof_merge_flags_at_pos(EOF_SONG *sp, unsigned long track, unsigned long pos, unsigned long flags)
{
	unsigned long i, tempflags;

	for(i = 0; i < eof_get_track_size(sp, track); i++)
	{	//For each note in the track
		if(eof_get_note_pos(sp, track, i) == pos)
		{	//If this note is at the target position
			tempflags = eof_get_note_flags(sp, track, i);
			tempflags |= flags;
			eof_set_note_flags(sp, track, i, tempflags);
		}
	}
}

void eof_merge_accent_mask_at_pos(EOF_SONG *sp, unsigned long track, unsigned long pos, unsigned char accent)
{
	unsigned long i, tempaccent;

	for(i = 0; i < eof_get_track_size(sp, track); i++)
	{	//For each note in the track
		if(eof_get_note_pos(sp, track, i) == pos)
		{	//If this note is at the target position
			tempaccent = eof_get_note_accent(sp, track, i);
			tempaccent |= accent;
			tempaccent &= eof_get_note_note(sp, track, i);	//Only retain accent status for gems that this note contains
			eof_set_note_accent(sp, track, i, tempaccent);
		}
	}
}

void eof_merge_ghost_mask_at_pos(EOF_SONG *sp, unsigned long track, unsigned long pos, unsigned char ghost)
{
	unsigned long i, tempghost;

	for(i = 0; i < eof_get_track_size(sp, track); i++)
	{	//For each note in the track
		if(eof_get_note_pos(sp, track, i) == pos)
		{	//If this note is at the target position
			tempghost = eof_get_note_ghost(sp, track, i);
			tempghost |= ghost;
			tempghost &= eof_get_note_note(sp, track, i);	//Only retain ghost status for gems that this note contains
			eof_set_note_ghost(sp, track, i, tempghost);
		}
	}
}
