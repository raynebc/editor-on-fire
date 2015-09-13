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
static double chartpos_to_msec(struct FeedbackChart * chart, unsigned long chartpos)
{
//	eof_log("chartpos_to_msec() entered");

	double offset;
	double curpos;
	unsigned long lastchartpos = 0;
	double beat_length = 500.0;
	double beat_count;
	double convert;
	struct dBAnchor * current_anchor;
	if(!chart)
	{
		return 0;
	}
	current_anchor = chart->anchors;
	offset = chart->offset * 1000.0;
	curpos = offset;
	convert = beat_length / (double)chart->resolution; // current conversion rate of chartpos to milliseconds
	while(current_anchor)
	{
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
	struct FeedbackChart * chart = NULL;
	EOF_SONG * sp = NULL;
	int err=0;
	char oggfn[1024] = {0};
	char searchpath[1024] = {0};
	char backup_filename[1024] = {0};
	char oldoggpath[1024] = {0};
	struct al_ffblk info = {0, 0, 0, {0}, NULL}; // for file search
	int ret=0;
	struct dBAnchor * current_anchor;
	struct dbText * current_event;
	unsigned long chartpos, max_chartpos;
	EOF_BEAT_MARKER * new_beat = NULL;
	struct dBAnchor *ptr, *ptr2;
	unsigned curnum=4,curden=4;		//Stores the current time signature details (default is 4/4)
	char midbeatchange;
	unsigned long nextbeat;
	unsigned long curppqn = 500000;	//Stores the current tempo (default is 120BPM)
	double beatlength, chartfpos = 0;
	char tschange;
	struct dbTrack * current_track;
	int track;
	int difficulty;
	long b = 0;
	double solo_on = 0.0, solo_off = 0.0;
	char solo_status = 0;	//0 = Off and awaiting a solo on marker, 1 = On and awaiting a solo off marker
	unsigned long ctr, ctr2, ctr3, tracknum;

	eof_log("\tImporting Feedback chart", 1);
	eof_log("eof_import_chart() entered", 1);

	if(!fn)
	{
		return NULL;
	}
//	eof_log_level &= ~2;	//Disable verbose logging
	chart = ImportFeedback((char *)fn, &err);
	if(chart == NULL)
	{	//Import failed
		(void) snprintf(oggfn, sizeof(oggfn) - 1, "Error:  %s", eof_chart_import_return_code_list[err % 31]);	//Display the appropriate error
		(void) alert("Error:", NULL, eof_chart_import_return_code_list[err % 31], "OK", NULL, 0, KEY_ENTER);
		return NULL;
	}
	else
	{
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

		/* load audio */
		(void) replace_filename(eof_song_path, fn, "", 1024);	//Set the project folder path
		(void) replace_filename(oggfn, fn, "guitar.ogg", 1024);	//Look for guitar.ogg by default
		if((chart->audiofile != NULL) && !exists(oggfn))
		{	//If the imported chart defines which audio file to use AND guitar.ogg doesn't exist
			(void) replace_filename(oggfn, fn, chart->audiofile, 1024);
			if(!exists(oggfn))	//If the file doesn't exist in the chart's parent directory
				(void) replace_filename(oggfn, fn, "guitar.ogg", 1024);	//Look for guitar.ogg instead
		}

		/* if the audio file doesn't exist, look for any OGG file in the chart directory */
		if(!exists(oggfn))
		{
			/* no OGG file found, start file selector at chart directory */
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

		(void) replace_filename(searchpath, oggfn, "", 1024);		//Store the path of the file's parent folder
		ret = eof_mp3_to_ogg(oggfn,searchpath);				//Create guitar.ogg in the folder
		if(ret != 0)
		{	//If guitar.ogg was not created successfully
			DestroyFeedbackChart(chart, 1);
			return NULL;
		}
		(void) replace_filename(oggfn, oggfn, "guitar.ogg", 1024);	//guitar.ogg is the expected file

		if(!eof_load_ogg(oggfn, 1))	//If user does not provide audio, fail over to using silent audio
		{
			DestroyFeedbackChart(chart, 1);
			(void) ustrcpy(eof_last_ogg_path, oldoggpath); // remember previous OGG directory if we fail
			return NULL;
		}
		eof_music_length = alogg_get_length_msecs_ogg(eof_music_track);

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
		current_anchor = chart->anchors;
		current_event = chart->events;
		chartpos = max_chartpos = 0;

		/* find the highest chartpos for beat markers */
		while(current_anchor)
		{
			if(current_anchor->chartpos > max_chartpos)
			{
				max_chartpos = current_anchor->chartpos;
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
		beatlength = chart->resolution;
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
						if(ptr->TS)
						{	//If this anchor defines a tempo change (is nonzero)
							curnum = ptr->TS;	//Store this anchor's time signature (which Feedback stores as #/4)
							if(ptr->chartpos == chartpos)
							{	//If this change is at the current position
								tschange = 1;	//Keep note
							}
						}
					}
				}
				if(eof_use_ts && (tschange || (sp->beats == 1)))
				{	//If the user opted to import TS changes, and this anchor has a TS change (or this is the first beat)
					(void) eof_apply_ts(curnum,curden,sp->beats - 1,sp,0);	//Set the TS flags for this beat
				}
				new_beat->ppqn = curppqn;
				new_beat->midi_pos = chartpos;

				//Scan ahead to look for mid beat tempo or TS changes
				midbeatchange = 0;
				beatlength = chart->resolution;		//Determine the length of one full beat in delta ticks
				nextbeat = chartpos + beatlength + 0.5;	//By default, the delta position of the next beat will be the standard length of delta ticks
				for(ptr2 = chart->anchors; ptr2 != NULL; ptr2 = ptr2->next)
				{	//For each anchor in the chart
					if(ptr2->chartpos  > chartpos)
					{	//If this anchor is ahead of the current delta position
						if(ptr2->chartpos < nextbeat)
						{	//If this anchor occurs before the next beat marker
							(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tMid beat tempo change at chart position %lu", ptr2->chartpos);
							eof_log(eof_log_string, 1);
							nextbeat = ptr2->chartpos;	//Store its delta time
							midbeatchange = 1;
						}
						break;
					}
				}
				if(midbeatchange)
				{	//If there is a mid-beat tempo/TS change, this beat needs to be anchored and its tempo (and the current tempo) altered
					//Also update beatlength to reflect that less than a full beat's worth of deltas will be used to advance to the next beat marker
					sp->beat[sp->beats - 1]->flags |= EOF_BEAT_FLAG_ANCHOR;
					curppqn = (double)curppqn * (((double)nextbeat - chartpos) / beatlength) + 0.5;	//Scale the current beat's tempo based on the adjusted delta length (rounded to nearest whole number)
					sp->beat[sp->beats - 1]->ppqn = curppqn;		//Update the beat's (now an anchor) tempo
					beatlength = (double)nextbeat - chartpos;	//This is the distance between the current beat, and the upcoming mid-beat change
				}
			}//If the beat was created successfully
			chartfpos += beatlength;	//Add the delta length of this beat to the delta counter
			chartpos = chartfpos + 0.5;	//Get the current chart position, rounded up to an integer value
		}//Add new beats until enough have been added to encompass the last item in the chart

		eof_calculate_beats(sp);		//Set the beats' timestamps based on their tempo changes

		/* fill in notes */
		current_track = chart->tracks;
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
			if(track >= 0)
			{
				struct dbNote * current_note = current_track->notes;
				unsigned long lastpos = -1;	//The position of the last imported note (not updated for sections that are parsed)
				EOF_NOTE * new_note = NULL;

				tracknum = sp->track[track]->tracknum;
				while(current_note)
				{
					/* import star power */
					if(current_note->gemcolor == '2')
					{
						(void) eof_legacy_track_add_star_power(sp->legacy_track[tracknum], chartpos_to_msec(chart, current_note->chartpos), chartpos_to_msec(chart, current_note->chartpos + current_note->duration));
					}

					/* skip face-off sections for now */
					else if((current_note->gemcolor == '0') || (current_note->gemcolor == '1'))
					{
					}

					/* skip unknown section markers */
					else if((current_note->gemcolor == '3') || (current_note->gemcolor == '4'))
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
						lastpos = current_note->chartpos;
					}
					current_note = current_note->next;
				}
			}
			current_track = current_track->next;
		}

		/* load text events */
		current_event = chart->events;
		while(current_event)
		{
			b = current_event->chartpos / chart->resolution;
			if(b >= sp->beats)
			{
				b = sp->beats - 1;
			}
			if(!ustricmp(current_event->text, "[solo_on]") && !solo_status)
			{	//If this is a solo on event (and a solo_off event isn't expected)
				solo_on = chartpos_to_msec(chart, current_event->chartpos);	//Store the real timestamp associated with the start of the phrase
				solo_status = 1;
			}
			else if(!ustricmp(current_event->text, "[solo_off]") && solo_status)
			{	//If this is a solo off event (and a solo_off event is expected), add it to the guitar and lead guitar tracks (FoF's original behavior for these events)
				solo_off = chartpos_to_msec(chart, current_event->chartpos);	//Store the real timestamp associated with the end of the phrase
				solo_status = 0;
				(void) eof_track_add_solo(sp, EOF_TRACK_GUITAR, solo_on + 0.5, solo_off + 0.5);	//Add the solo to the guitar track
				(void) eof_track_add_solo(sp, EOF_TRACK_GUITAR_COOP, solo_on + 0.5, solo_off + 0.5);	//Add the solo to the lead guitar track
			}
			else if(eof_text_is_section_marker(current_event->text))
			{	//If this is a section event, rebuild the string to ensure it's in the proper format
				char buffer[256];
				int index = 0, index2 = 0;	//index1 will index into the rebuilt buffer[] string, index2 will index into the original current_event->text[] string

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
				(void) eof_song_add_text_event(sp, b, buffer, 0, 0, 0);
			}
			else
			{	//Otherwise copy the string as-is
				(void) eof_song_add_text_event(sp, b, current_event->text, 0, 0, 0);
			}
			current_event = current_event->next;	//Iterate to the next text event
		}

		/* load time signatures */
		if(eof_use_ts)
		{	//If the user opted to import TS changes
			current_anchor = chart->anchors;
			(void) eof_apply_ts(4,4,0,sp,0);	//Apply a default TS of 4/4 on the first beat marker
			while(current_anchor)
			{
				if((current_anchor->TS != 0) && (current_anchor->chartpos % chart->resolution == 0))
				{	//If there is a Time Signature defined here, and it is defined on a beat marker
					if(current_anchor->chartpos / chart->resolution < sp->beats)
					{	//And the beat in question is defined in the beat[] array, apply the TS change
						(void) eof_apply_ts(current_anchor->TS,4,current_anchor->chartpos / chart->resolution,sp,0);
					}
				}
				current_anchor = current_anchor->next;
			}
		}

		DestroyFeedbackChart(chart, 1);	//Free memory used by Feedback chart before exiting function
	}
	eof_selected_ogg = 0;

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

	/* check if unofficial forced HOPO notation was found */
	eof_sort_notes(sp);
	for(ctr = 1; ctr < sp->tracks; ctr++)
	{	//For each track
		if(ctr == EOF_TRACK_DRUM)
		{	//If this is the drum track
			continue;	//Skip it, lane 6 for the drum track indicates a sixth lane and not HOPO
		}
		for(ctr2 = 0; ctr2 < eof_get_track_size(sp, ctr); ctr2++)
		{	//For each note in the track
			if(eof_get_note_note(sp, ctr, ctr2) & 32)
			{	//If this note uses lane 6 (A "N 5 #" .chart entry)
				unsigned long pos = eof_get_note_pos(sp, ctr, ctr2);
				long len = eof_get_note_length(sp, ctr, ctr2);
				unsigned char type = eof_get_note_type(sp, ctr, ctr2);

				for(ctr3 = 0; ctr3 < eof_get_track_size(sp, ctr); ctr3++)
				{	//For each note in the track
					unsigned long pos2 = eof_get_note_pos(sp, ctr, ctr3);

					if(pos2 > pos + len)
					{	//If this note occurs after the span of the HOPO notation
						break;	//Break from inner loop
					}
					if((pos2 >= pos) && (eof_get_note_type(sp, ctr, ctr3) == type))
					{	//If this note is within the span of the HOPO notation and is in the same difficulty
						eof_set_note_flags(sp, ctr, ctr3, (eof_get_note_flags(sp, ctr, ctr3) | EOF_NOTE_FLAG_F_HOPO));	//Set the forced HOPO flag for this note
					}
				}
			}
		}
	}

//Update path variables
	(void) ustrcpy(eof_filename, fn);
	(void) replace_filename(eof_song_path, fn, "", 1024);
	(void) replace_filename(eof_last_eof_path, eof_filename, "", 1024);
	(void) ustrcpy(eof_loaded_song_name, get_filename(eof_filename));
	(void) replace_extension(eof_loaded_song_name, eof_loaded_song_name, "eof", 1024);

	eof_log("\tFeedback import completed", 1);
//	eof_log_level |= 2;	//Enable verbose logging
	return sp;
}

struct FeedbackChart *ImportFeedback(char *filename, int *error)
{
	PACKFILE *inf=NULL;
	char songparsed=0,syncparsed=0,eventsparsed=0;
		//Flags to indicate whether each of the mentioned sections had already been parsed
	char currentsection=0;					//Will be set to 1 for [Song], 2 for [SyncTrack], 3 for [Events] or 4 for an instrument section
	size_t maxlinelength=0;					//I will count the length of the longest line (including NULL char/newline) in the
	char *buffer=NULL,*buffer2=NULL;		//Will be an array large enough to hold the largest line of text from input file
	unsigned long index=0,index2=0;			//Indexes for buffer and buffer2, respectively
	char *substring=NULL,*substring2=NULL;	//Used with strstr() to find tag strings in the input file
	unsigned long A=0,B=0,C=0;				//The first, second and third integer values read from the current line of the file
	int errorstatus=0;						//Passed to ParseLongInt()
	char anchortype=0;						//The achor type being read in [SyncTrack]
	char notetype=0;						//The note type being read in the instrument track
	char *string1=NULL,*string2=NULL;		//Used to hold strings parsed with Read_dB_string()

//Feedback chart structure variables
	struct FeedbackChart *chart=NULL;
	struct dBAnchor *curanchor=NULL;		//Conductor for the anchor linked list
	struct dbText *curevent=NULL;			//Conductor for the text event linked list
	struct dbNote *curnote=NULL;			//Conductor for the current instrument track's note linked list
	struct dbTrack *curtrack=NULL;			//Conductor for the instrument track linked list, which contains a linked list of notes
	void *temp=NULL;						//Temporary pointer used for storing newly-allocated memory
	static struct FeedbackChart emptychart;	//A static struct has all members auto-initialized to 0/NULL
	static struct dBAnchor emptyanchor;
	static struct dbText emptytext;
	static struct dbNote emptynote;

	eof_log("ImportFeedback() entered", 1);

//Open file in text mode
	if(!filename)
	{
		return NULL;
	}
	inf=pack_fopen(filename,"rt");
	if(inf == NULL)
	{
		if(error)
			*error=1;
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError loading:  Cannot open input .chart file:  \"%s\"", strerror(errno));	//Get the Operating System's reason for the failure
		eof_log(eof_log_string, 1);
		return NULL;
	}

//Initialize chart structure
	chart=(struct FeedbackChart *)malloc_err(sizeof(struct FeedbackChart));	//Allocate memory
	*chart=emptychart;		//Reliably set all member variables to 0/NULL
	chart->resolution=192;	//Default this to 192

//Allocate memory buffers large enough to hold any line in this file
	maxlinelength=(size_t)FindLongestLineLength_ALLEGRO(filename,1);
	if(!maxlinelength)
	{
		eof_log("\tError finding the largest line in the file.  Aborting", 1);
		(void) pack_fclose(inf);
		free(chart);
		return NULL;
	}
	buffer=(char *)malloc_err(maxlinelength);
	buffer2=(char *)malloc_err(maxlinelength);

//Read first line of text, capping it to prevent buffer overflow
	if(!pack_fgets(buffer,(int)maxlinelength,inf))
	{	//I/O error
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Feedback import failed on line #%lu:  Unable to read from file:  \"%s\"", chart->linesprocessed, strerror(errno));
		eof_log(eof_log_string, 1);
		if(error)
			*error=2;
		(void) pack_fclose(inf);
		free(buffer);
		free(buffer2);
		free(chart);
		return NULL;
	}

//Parse the contents of the file
	while(!pack_feof(inf))		//Until end of file is reached
	{
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

		if((buffer[index] == '\n') || (buffer[index] == '\r') || (buffer[index] == '\0') || (buffer[index] == '{'))
		{	//If this line was empty, or contained characters we're ignoring
			(void) pack_fgets(buffer,(int)maxlinelength,inf);	//Read next line of text, so the EOF condition can be checked, don't exit on EOF
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
				free(buffer);
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
				free(buffer);
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
					free(buffer);
					free(buffer2);
					return NULL;					//Multiple [song] sections, return error
				}
				songparsed=1;
				currentsection=1;	//Track that we're parsing [Song]
			}
			else
			{
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
						free(buffer);
						free(buffer2);
						return NULL;					//Multiple [SyncTrack] sections, return error
					}
					syncparsed=1;
					currentsection=2;	//Track that we're parsing [SyncTrack]
				}
				else
				{
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
							free(buffer);
							free(buffer2);
							return NULL;					//Multiple [Events] sections, return error
						}
						eventsparsed=1;
						currentsection=3;	//Track that we're parsing [Events]
					}
					else
					{	//This is an instrument section
						temp=(void *)Validate_dB_instrument(buffer);
						if(temp == NULL)					//Not a valid Feedback instrument section name
						{
							(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Feedback import failed on line #%lu:  Invalid instrument section \"%s\"", chart->linesprocessed, buffer);
							eof_log(eof_log_string, 1);
							DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
							if(error)
								*error=8;
							free(buffer);
							free(buffer2);
							return NULL;					//Invalid instrument section, return error
						}
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
					}
				}
			}

			(void) pack_fgets(buffer,(int)maxlinelength,inf);	//Read next line of text, so the EOF condition can be checked, don't exit on EOF
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
				free(buffer);
				free(buffer2);
				return NULL;					//Malformed file, return error
			}
			currentsection=0;
			(void) pack_fgets(buffer,(int)maxlinelength,inf);	//Read next line of text, so the EOF condition can be checked, don't exit on EOF
			continue;							//Skip ahead to the next line
		}

//Process normal line input
		substring=strchr(buffer,'=');		//Any line within the section is expected to contain an equal sign
		if(substring == NULL)
		{
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Feedback import failed on line #%lu:  Malformed item (missing equal sign)", chart->linesprocessed);
			eof_log(eof_log_string, 1);
			DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
			if(error)
				*error=10;
			free(buffer);
			free(buffer2);
			return NULL;					//Invalid section entry, return error
		}

	//Process [Song]
		if(currentsection == 1)
		{
			if(Read_dB_string(buffer,&string1,&string2))
			{	//If a valid definition of (string) = (string) or (string) = "(string)" was found
				if(strcasecmp(string1,"Name") == 0)
					chart->name=string2;	//Save the song name tag
				else if(strcasecmp(string1,"Artist") == 0)
					chart->artist=string2;	//Save the song artist tag
				else if(strcasecmp(string1,"Charter") == 0)
					chart->charter=string2;	//Save the chart editor tag
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
						free(buffer);
						free(buffer2);
						free(string1);
						free(string2);
						return NULL;					//Invalid number, return error
					}
					free(string2);	//This string is no longer used
					string2 = NULL;
				}
				else if(strcasecmp(string1,"MusicStream") == 0)
					chart->audiofile=string2;	//Save the name of the audio file for the chart
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
				free(buffer);
				free(buffer2);
				return NULL;					//Invalid number, return error
			}

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
				free(buffer);
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
					free(buffer);
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
				free(buffer);
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
				free(buffer);
				free(buffer2);
				return NULL;					//Invalid number, return error
			}

		//If this anchor event has the same chart time as the last, just write this event's information with the last's
			if(curanchor && (curanchor->chartpos == A))
			{
				if(anchortype == 'A')		//If this is an Anchor event
					curanchor->usec=B;		//Store the anchor realtime position
				else if(anchortype == 'B')	//If this is a Tempo event
					curanchor->BPM=B;		//Store the tempo
				else if(anchortype == 'T')	//If this is a Time Signature event
					curanchor->TS=B;		//Store the Time Signature
				else
				{
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Feedback import failed on line #%lu:  Invalid sync item type \"%c\"", chart->linesprocessed, anchortype);
					eof_log(eof_log_string, 1);
					DestroyFeedbackChart(chart,1);
					if(error)
						*error=17;
					free(buffer);
					free(buffer2);
					return NULL;			//Invalid anchor type, return error
				}
			}
			else
			{
		//Create and insert anchor link into the anchor list
				temp=malloc_err(sizeof(struct dBAnchor));	//Allocate memory
				*((struct dBAnchor *)temp)=emptyanchor;		//Reliably set all member variables to 0/NULL
				if(chart->anchors == NULL)					//If the list is empty
				{
					chart->anchors=(struct dBAnchor *)temp;	//Point head of list to this link
					curanchor=chart->anchors;				//Point conductor to this link
				}
				else
				{
					assert(curanchor != NULL);					//Put an assertion here to resolve a false positive with Coverity
					curanchor->next=(struct dBAnchor *)temp;	//Conductor points forward to this link
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
						curanchor->TS=B;	//Store the numerator of the time signature
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
					free(buffer);
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
				free(buffer);
				free(buffer2);
				return NULL;					//Invalid number, return error
			}

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
				free(buffer);
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
					free(buffer);
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
						free(buffer);
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
				free(buffer);
				free(buffer2);
				return NULL;					//Invalid number, return error
			}

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
				free(buffer);
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

				case 'E':		//Text event, skip it
					(void) pack_fgets(buffer,(int)maxlinelength,inf);	//Read next line of text, so the EOF condition can be checked, don't exit on EOF
				continue;

				case 'N':		//Note indicator, and increment index
					index++; 	//Increment index past N or S identifier
				break;

				default:
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Feedback import failed on line #%lu:  Invalid gem item type \"%c\"", chart->linesprocessed, substring[index]);
					eof_log(eof_log_string, 1);
					DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
					if(error)
						*error=25;
				free(buffer);
				free(buffer2);
				return NULL;		//Invalid instrument entry, return error
			}

		//Load second number
			B=ParseLongInt(substring,&index,chart->linesprocessed,&errorstatus);
			if(errorstatus)						//If ParseLongInt() failed
			{
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Feedback import failed on line #%lu:  Invalid gem item parameter 1", chart->linesprocessed);
				eof_log(eof_log_string, 1);
				DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
				if(error)
					*error=26;
				free(buffer);
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
				free(buffer);
				free(buffer2);
				return NULL;					//Invalid number, return error
			}

		//Create a note link and add it to the current Note list
			if(curtrack == NULL)				//If the instrument track linked list is not initialized
			{
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Feedback import failed on line #%lu:  Malformed file (gem defined outside instrument track)", chart->linesprocessed);
				eof_log(eof_log_string, 1);
				DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
				if(error)
					*error=28;
				free(buffer);
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
				if(B > 4)							//Only values of 0, 1 or 2 are valid for player section markers, 3 and 4 are unknown but will be kept and ignored for now during transfer to EOF
				{
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Feedback import failed on line #%lu:  Invalid section marker #%lu", chart->linesprocessed, B);
					eof_log(eof_log_string, 1);
					DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
					if(error)
						*error=29;
					free(buffer);
					free(buffer2);
					return NULL;					//Invalid player section marker, return error
				}
				curnote->gemcolor='0'+B;	//Store 0 as '0', 1 as '1' or 2 as '2', ...
			}
			curnote->duration=C;			//The third number read is the note duration
		}//Process instrument tracks

	//Error: Content in file outside of a defined section
		else
		{
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Feedback import failed on line #%lu:  Malformed file (item defined outside of track)", chart->linesprocessed);
			eof_log(eof_log_string, 1);
			DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
			if(error)
				*error=30;
			free(buffer);
			free(buffer2);
			return NULL;					//Malformed file, return error
		}

		(void) pack_fgets(buffer,(int)maxlinelength,inf);	//Read next line of text, so the EOF condition can be checked, don't exit on EOF
	}//Until end of file is reached

	if(error)
		*error=0;

	free(buffer);
	free(buffer2);
	(void) pack_fclose(inf);

	return chart;
}

int Read_dB_string(char *source,char **str1, char **str2)
{
	//Scans the source string for a valid dB tag: text = text	or	text = "text"
	//The text to the left of the equal sign is returned through str1 as a new string, with whitespace truncated
	//The text to the right of the equal sign is returned through str2 as a new string, with whitespace truncated
	//If the first non whitespace character encountered after the equal sign is a quotation mark, all characters after
	//that quotation mark up to the next are returned through str2
	//Nonzero is returned upon success, or zero is returned if source did not contain two sets of non whitespace characters
	//separated by an equal sign character, or if the closing quotation mark is missing.

//	eof_log("Read_dB_string() entered");

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

struct dbTrack *Validate_dB_instrument(char *buffer)
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
	char *inststring=NULL;	//Used to find the instrument substring
	char *retstring=NULL;	//Used to create the instrument track string
	struct dbTrack *chart=NULL;		//Used to create the structure that is returned
	static struct dbTrack emptychart;	//Static structures have all members auto initialize to 0/NULL
	char tracktype=0,difftype=0;	//Used to track the instrument type and difficulty of the track, based on the name
	char isguitar=0,isdrums=0;		//Used to track secondary/tertiary guitar/drum tracks

	eof_log("Validate_dB_instrument() entered", 1);

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
				else
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

//At this point, diffstring points to the character AFTER the matching difficulty string.  Verify that a valid instrument is specified
	//Test for Single (Guitar)
		inststring=strcasestr_spec(diffstring,"Single]");
		if(inststring == NULL)
		{
	//Test for DoubleGuitar (Lead Guitar)
			inststring=strcasestr_spec(diffstring,"DoubleGuitar]");
			if(inststring == NULL)
			{
	//Test for DoubleBass (Bass)
				inststring=strcasestr_spec(diffstring,"DoubleBass]");
				if(inststring == NULL)
				{
	//Test for EnhancedGuitar
					inststring=strcasestr_spec(diffstring,"EnhancedGuitar]");
					if(inststring == NULL)
					{
	//Test for CoopLead
						inststring=strcasestr_spec(diffstring,"CoopLead]");
						if(inststring == NULL)
						{
	//Test for CoopBass
							inststring=strcasestr_spec(diffstring,"CoopBass]");
							if(inststring == NULL)
							{
	//Test for 10KeyGuitar
								inststring=strcasestr_spec(diffstring,"10KeyGuitar]");
								if(inststring == NULL)
								{
	//Test for Drums
									inststring=strcasestr_spec(diffstring,"Drums]");
									if(inststring == NULL)
									{
	//Test for DoubleDrums (Expert+ drums)
										inststring=strcasestr_spec(diffstring,"DoubleDrums]");
										if(inststring == NULL)
										{
	//Test for Vocals (Vocal Rhythm)
											inststring=strcasestr_spec(diffstring,"Vocals]");
											if(inststring == NULL)
											{
	//Test for Keyboard
												inststring=strcasestr_spec(diffstring,"Keyboard]");
												if(inststring == NULL)
												{
	//Test for SingleGuitar (Guitar)
													inststring=strcasestr_spec(diffstring,"SingleGuitar]");	//This is another track name that can represent "Single"
													if(inststring == NULL)
													{
	//Test for SingleRhythm (Bass)
														inststring=strcasestr_spec(diffstring,"SingleRhythm]");	//This is another track name that can represent "DoubleBass"
														if(inststring == NULL)
														{
	//Test for DoubleRhythm (Rhythm)
															inststring=strcasestr_spec(diffstring,"DoubleRhythm]");
															if(inststring == NULL)
															{		//If none of the valid instrument names were found
																return NULL;	//Return error
															}
															else
															{
																tracktype=6;	//Track that this is a "Rhythm" track
																isguitar=1;
															}
														}
														else
														{
															tracktype=3;	//Track that this is a "Bass" track
															isguitar=1;
														}
													}
													else
													{
														tracktype=1;	//Track that this is a "Guitar" track
														isguitar=1;
													}
												}
												else
													tracktype=7;
											}
											else
												tracktype=5;	//Track that this is a "Vocals" track
										}
										else
											isdrums=1;	//DoubleDrums is a drums track
									}
									else
									{
										tracktype=4;	//Track that this is a "Drums" track
										isdrums=1;
									}
								}
								else
									isguitar=1;	//10KeyGuitar is a guitar track
							}
							else
								isguitar=1;	//CoopBass is a guitar track
						}
						else
							isguitar=1;	//CoopLead is a guitar track
					}
					else
						isguitar=1;		//EnhancedGuitar is a guitar track
				}
				else
				{
					tracktype=3;	//Track that this is a "Bass" track
					isguitar=1;
				}
			}
			else
			{
				tracktype=2;	//Track that this is a "Lead Guitar" track
				isguitar=1;
			}
		}
		else
		{
			tracktype=1;	//Track that this is a "Guitar" track
			isguitar=1;
		}

//Validate that the character immediately after the instrument substring is the NULL terminator
	if(inststring[0] != '\0')
		return NULL;

//Create a new string containing the instrument name, minus the brackets
	retstring=DuplicateString(&buffer[1]);
	retstring[strlen(retstring)-1]='\0';	//Truncate the trailing bracket

//Create and initialize the instrument structure
	chart=malloc_err(sizeof(struct dbTrack));	//Allocate memory
	*chart=emptychart;							//Reliably set all member variables to 0/NULL
	chart->trackname=retstring;					//Store the instrument track name
	chart->tracktype=tracktype;
	chart->difftype=difftype;
	chart->isguitar=isguitar;
	chart->isdrums=isdrums;
	return chart;
}

void DestroyFeedbackChart(struct FeedbackChart *ptr, char freestruct)
{
	struct dBAnchor *anchorptr;	//Conductor for the anchors linked list
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

unsigned long FindLongestLineLength_ALLEGRO(const char *filename,char exit_on_empty)
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
		else
		{
			(void) puts("Error: File is empty\nAborting");
			exit_wrapper(2);
		}
	}
	maxlinelength++;		//Must increment this to account for newline character

	(void) pack_fclose(inf);
	return maxlinelength;
}
