#include <allegro.h>
#include <ctype.h>
#include "chart_import.h"
#include "ini_import.h"
#include "main.h"
#include "midi.h"
#include "sm.h"
#include "song.h"
#include "utility.h"
#include "undo.h"
#include "foflc/Lyric_storage.h"
#include "menu/edit.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

#define EOF_MAX_SM_NOTES 1000

typedef struct
{
	char gem[4];
} EOF_STEPMANIA_NOTE;

int eof_stepmania_cleanup_input(char *buffer)
{
	unsigned long ctr;
	int semicolon = 0;

	if(!buffer)
		return 0;

	for(ctr = 0; buffer[ctr] != '\0'; ctr++)
	{
		if((buffer[ctr] == '/') && (buffer[ctr + 1] == '/'))
		{
			buffer[ctr] = '\0';	//Truncate comment from the line
			break;
		}
		if(buffer[ctr] == ';')
		{
			buffer[ctr] = '\0';	//Truncate end of tag symbol
			semicolon = 1;	//Track that the semicolon was read
			break;
		}
	}
	while(ctr && isspace(buffer[ctr]))
	{	//While the last character in the buffer is whitespace
		buffer[ctr--] = '\0';	//Truncate it from the buffer
	}

	return semicolon;
}

int eof_import_stepmania(char * fn)
{
	size_t maxlinelength;
	char *buffer = NULL, *ptr;
	char is_ssc = 0, pad_defined = 0, type_defined = 0, diff_defined = 0;	//Used for parsing SSC files, which have some differences from SM files
	int error = 0, tag_in_progress = 0, title_parsed = 0, titletranslit_parsed = 0, artist_parsed = 0, artisttranslit_parsed = 0, genre_parsed = 0, credit_parsed = 0, note_diff_parsed[5] = {0}, semicolon = 0, bpms_parsed = 0, adjust = 0, offset_parsed = 0, measure_in_progress;
	PACKFILE *inf = NULL;
	unsigned long linectr = 0, ctr, ctr2, samplestart_parsed = ULONG_MAX, samplelength_parsed = ULONG_MAX, note_diff_in_progress = 0, tempos_parsed = 0, measurectr, notectr;
	char title[101] = {0}, artist[101] = {0}, charter[101] = {0}, genre[31] = {0}, credit[101] = {0};
	char *note_diff_names[5] = {"Beginner", "Easy", "Medium", "Hard", "Challenge"};
	struct FeedbackChart *chart = NULL;	//Used to store the Stepmania file's tempo changes separately from the project's tempo map, since the former might not be imported
	struct dbAnchor *curanchor = NULL;	//The conductor pointing to the newest anchor added to the linked list
	EOF_STEPMANIA_NOTE notes[EOF_MAX_SM_NOTES];	//An array large enough to store the maximum number of notes that EOF will parse in one measure of the Stepmania file
	unsigned long last_hold[4] = {ULONG_MAX, ULONG_MAX, ULONG_MAX, ULONG_MAX};	//The note index of the last hold/roll note added on each lane

	//Validate input
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Importing StepMania file \"%s\"", fn);
	eof_log(eof_log_string, 1);
	eof_log("eof_import_stepmania() entered", 1);

	if(!eof_song || !eof_song_loaded)
		return 1;	//For now, don't do anything unless a project is active
	if(!fn)
		return 1;	//Invalid parameter
	if(!exists(fn))
		return 1;	//Invalid parameter

	//Allocate line buffer
	maxlinelength = (size_t)FindLongestLineLength_ALLEGRO(fn, 0);
	if(!maxlinelength)
	{
		eof_log("\tError finding the largest line in input file.", 1);
		error = 1;
	}
	else
	{
		buffer = (char *)malloc(maxlinelength);
		if(!buffer)
		{
			eof_log("\tError allocating memory.  Aborting", 1);
			return 1;
		}
	}

	//Prompt whether to override DANCE track if it contains any notes
	if(eof_get_track_size(eof_song, EOF_TRACK_DANCE))
	{
		eof_clear_input();
		if(alert("The dance track contains notes and will be", "erased in order to import the StepMania file.", "Continue?", "&Yes", "&No", 'y', 'n') != 1)
		{	//If the user does not opt to erase the track
			eof_log("\tUser cancellation.  Aborting", 1);
			free(buffer);
			return 2;
		}
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		eof_erase_track(eof_song, EOF_TRACK_DANCE, 1);
	}

	//Prompt whether to continue if the tempo map is locked
	if(eof_song->tags->tempo_map_locked)
	{	//If the user has locked the tempo map
		eof_clear_input();
		if(alert("The tempo map is locked and will prevent the", "import of the StepMania file's tempo changes.", "Continue?", "&Yes", "&No", 'y', 'n') != 1)
		{	//If the user does not opt to continue
			eof_log("\tUser cancellation.  Aborting", 1);
			free(buffer);
			return 2;
		}
	}
	else
	{	//If the tempo map is not locked
		eof_log("\tResetting tempo map", 1);
		if(eof_get_chart_size(eof_song) > 0)
		{	//If there is at least one note/lyric in the project, prompt to adjust them
			eof_clear_input();
			if(alert(NULL, "Adjust existing notes to imported tempo map?", NULL, "&Yes", "&No", 'y', 'n') == 1)
			{	//If user opts to auto-adjust the existing notes
				adjust = 1;
				eof_log("\tAuto adjusting existing notes", 1);
				(void) eof_menu_edit_cut(0, 1);	//Save auto-adjust data for the entire chart
			}
		}
		for(ctr = 0; ctr < eof_song->beats; ctr++)
		{	//For each beat in the project
			eof_remove_ts(ctr);	//Remove any defined time signature
			eof_song->beat[ctr]->ppqn = 500000;	//Default to 120BPM
			if(ctr)
			{	//If this isn't the first beat
				eof_song->beat[ctr]->flags &= ~EOF_BEAT_FLAG_ANCHOR;	//Clear the anchor flag
			}
		}
		eof_song->tags->ogg[0].midi_offset = 0;
		eof_apply_ts(4, 4, 0, eof_song, 0);	//Default to 4/4 meter
		eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current
		eof_calculate_beats(eof_song);	//Rebuild the beat timings
	}

	//Initialize chart structure
	chart = (struct FeedbackChart *)malloc(sizeof(struct FeedbackChart));	//Allocate memory
	if(!chart)
	{
		eof_log("\tError allocating memory.  Aborting", 1);
		free(buffer);
		return 1;
	}
	memset(chart, 0, sizeof(struct FeedbackChart));	//Init the structure to 0.  Offset will remain 0s unless defined otherwise, the anchors linked is initialized to NULL
	chart->resolution = 1000;	//Stepmania charts define beat positions up to thousandths of a second resolution, so indicate 1000 ticks per beat as the resolution

	//Open input file
	inf = pack_fopen(fn, "rt");	//Open file in text mode
	if(!inf)
	{
		eof_log("\tFailed to open input file.  Aborting", 1);
		free(buffer);
		free(chart);
		return 1;
	}

	//Parse input file
	while(!error)
	{	//While an error was not encountered
		(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
		if(pack_feof(inf))
			break;	//End of file

		linectr++;	//Increment line counter
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tProcessing line #%lu", linectr);
		eof_log(eof_log_string, 3);

		semicolon = eof_stepmania_cleanup_input(buffer);	//Clean up buffer content, tracking whether a semicolon was read

		//Process line buffer
		if(!tag_in_progress)
		{	//No tag is in progress yet
			ptr = strcasestr_spec(buffer, "#TITLETRANSLIT:");
			if(ptr)
			{	//If this line contains the #TITLETRANSLIT tag
				if(!titletranslit_parsed)
				{	//If this tag wasn't parsed already
					strncpy(title, ptr, sizeof(title) - 1);	//Store the title
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tRead song title \"%s\"", title);
					eof_log(eof_log_string, 2);
				}
				titletranslit_parsed = title_parsed = 1;
				continue;	//Read next line of input
			}

			ptr = strcasestr_spec(buffer, "#TITLE:");
			if(ptr)
			{	//If this line contains the #TITLE tag
				if(!titletranslit_parsed && !title_parsed)
				{	//If neither this tag nor the translated title were parsed already
					strncpy(title, ptr, sizeof(title) - 1);	//Store the title
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tRead song title \"%s\"", title);
					eof_log(eof_log_string, 2);
				}
				title_parsed = 1;
				continue;	//Read next line of input
			}

			ptr = strcasestr_spec(buffer, "#ARTISTTRANSLIT:");
			if(ptr)
			{	//If this line contains the #ARTISTTRANSLIT tag
				if(!artisttranslit_parsed)
				{	//If this tag wasn't parsed already
					strncpy(artist, ptr, sizeof(artist) - 1);	//Store the artist
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tRead artist name \"%s\"", artist);
					eof_log(eof_log_string, 2);
				}
				artisttranslit_parsed = artist_parsed = 1;
				continue;	//Read next line of input
			}

			ptr = strcasestr_spec(buffer, "#ARTIST:");
			if(ptr)
			{	//If this line contains the #ARTIST tag
				if(!artisttranslit_parsed && !artist_parsed)
				{	//If neither this tag nor the translated title were parsed already
					strncpy(artist, ptr, sizeof(artist) - 1);	//Store the artist
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tRead artist name \"%s\"", artist);
					eof_log(eof_log_string, 2);
				}
				artist_parsed = 1;
				continue;	//Read next line of input
			}

			ptr = strcasestr_spec(buffer, "#GENRE:");
			if(ptr)
			{	//If this line contains the #GENRE tag
				if(!genre_parsed)
				{	//If this tag wasn't parsed already
					strncpy(genre, ptr, sizeof(genre) - 1);	//Store the genre
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tRead genre \"%s\"", genre);
					eof_log(eof_log_string, 2);
				}
				genre_parsed = 1;
				continue;	//Read next line of input
			}

			ptr = strcasestr_spec(buffer, "#CREDIT:");
			if(ptr)
			{	//If this line contains the #CREDIT tag
				if(!credit_parsed)
				{	//If this tag wasn't parsed already
					strncpy(credit, ptr, sizeof(credit) - 1);	//Store the chart author
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tRead credit \"%s\"", credit);
					eof_log(eof_log_string, 2);
				}
				credit_parsed = 1;
				continue;	//Read next line of input
			}

			ptr = strcasestr_spec(buffer, "#SAMPLESTART:");
			if(ptr)
			{	//If this line contains the #SAMPLESTART tag
				if(samplestart_parsed == ULONG_MAX)
				{	//If this tag wasn't parsed already
					samplestart_parsed = (atof(ptr) * 1000.0) + 0.5;	//Convert timestamp to milliseconds
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tRead preview start time %lums", samplestart_parsed);
					eof_log(eof_log_string, 2);
				}
				continue;	//Read next line of input
			}

			ptr = strcasestr_spec(buffer, "#SAMPLELENGTH:");
			if(ptr)
			{	//If this line contains the #SAMPLELENGTH tag
				if(samplelength_parsed == ULONG_MAX)
				{	//If this tag wasn't parsed already
					samplelength_parsed = (atof(ptr) * 1000.0) + 0.5;	//Convert timestamp to milliseconds
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tRead preview duration %lums", samplelength_parsed);
					eof_log(eof_log_string, 2);
				}
				continue;	//Read next line of input
			}

			ptr = strcasestr_spec(buffer, "#OFFSET:");
			if(ptr)
			{	//If this line contains the #OFFSET tag
				if(!offset_parsed )
				{	//If this tag wasn't parsed already
					offset_parsed = 1;
					chart->offset = atof(ptr);	//The chart offset in seconds
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tRead offset %fs", chart->offset);
					eof_log(eof_log_string, 2);

					if(chart->offset < 0)
						allegro_message("Warning:  This chart has a negative offset of %fs and will need to be manually adjusted.", chart->offset);
					else
					{
						eof_song->tags->ogg[0].midi_offset = (chart->offset * 1000.0) + 0.5;	//Update the project's chart delay, to nearest millisecond
					}
				}
				continue;	//Read next line of input
			}

			ptr = strcasestr_spec(buffer, "#BPMS:");
			if(ptr)
			{	//If this line contains the #BPMS tag
				if(bpms_parsed)
				{	//If this tag was parsed already
					eof_log("\t\tMalformed file:  Multiple #BPMS tags.  Aborting", 1);
					error = 1;
					continue;
				}

				eof_log("\t\tParsing #BPMS tag", 1);
				bpms_parsed = 1;
				tag_in_progress = 1;	//Expect to parse tempo changes on this line
			}

			ptr = strcasestr_spec(buffer, "#CHARTSTYLE:");
			if(ptr)
			{	//If this line contains the #CHARTSTYLE tag
				is_ssc = 1;	//This tag is used in SSC files instead of SM files
				ptr = strcasestr_spec(buffer, "Pad");
				if(ptr)
					pad_defined = 1;	//Supported dance track style confirmed
				else
					pad_defined = 0;	//Unsupported dance track style
			}

			ptr = strcasestr_spec(buffer, "#STEPSTYPE:");
			if(ptr)
			{	//If this line contains the #STEPSTYPE tag
				is_ssc = 1;	//This tag is used in SSC files instead of SM files
				ptr = strcasestr_spec(buffer, "dance-single");
				if(ptr)
					type_defined = 1;	//Supported dance track type confirmed
				else
					type_defined = 0;	//Unsupported dance track type
			}

			ptr = strcasestr_spec(buffer, "#DIFFICULTY:");
			if(ptr)
			{	//If this line contains the #DIFFICULTY tag
				is_ssc = 1;	//This tag is used in SSC files instead of SM files
				diff_defined = 1;	//Unless a supported difficulty name isn't read below, consider this information confirmed
				if(strcasestr_spec(buffer, "Beginner"))
					note_diff_in_progress = 0;
				else if(strcasestr_spec(buffer, "Easy"))
					note_diff_in_progress = 1;
				else if(strcasestr_spec(buffer, "Medium"))
					note_diff_in_progress = 2;
				else if(strcasestr_spec(buffer, "Hard"))
					note_diff_in_progress = 3;
				else if(strcasestr_spec(buffer, "Challenge"))
					note_diff_in_progress = 4;
				else
					diff_defined = 0;
			}

			///Stepmania supports a #TIMESIGNATURES tag, but it doesn't seem widely used and supposedly does not affect timing, only the display of measure markers

			if(!tag_in_progress)
			{	//No tag is in progress yet
				ptr = strcasestr_spec(buffer, "#NOTES:");
				if(ptr)
				{	//If this line contains the #NOTES tag
					size_t length;

					eof_log("\t\tParsing #NOTES tag", 1);

					if(is_ssc)
					{	//If this is an SSC file, the track type and difficulty are expected to have been parsed already
						if(!pad_defined || !type_defined || !diff_defined)
						{	//If any of the required descriptors for this track dififculty haven't been parsed
							eof_log("\t\tUnrecognized track difficulty.  Skipping", 1);
							continue;
						}
					}
					else
					{
						//Read next line defining the chart type
						(void) pack_fgets(buffer, (int)maxlinelength, inf);
						linectr++;	//Increment line counter
						if(pack_feof(inf))
						{
							error = 1;	//Failed to read note type
							continue;
						}
						if(!strcasestr_normal(buffer, "dance-single"))
						{	//If this isn't a dance-single chart type
							(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tUnsupported chart type:%s.  Skipping", buffer);
							eof_log(eof_log_string, 2);
							continue;
						}

						//Read next line defining the charter name
						(void) pack_fgets(buffer, (int)maxlinelength, inf);
						linectr++;	//Increment line counter
						if(pack_feof(inf))
						{
							error = 1;	//Failed to read charter name
							continue;
						}
						for(ctr = 0; isspace(buffer[ctr]); ctr++);
						ptr = &(buffer[ctr]);	//Find the first non whitespace character in the charter name
						length = strlen(ptr);
						if(length && (ptr[length - 1] == ':'))
							ptr[length - 1] = '\0';	//If the charter name is followed by a colon as expected, truncate the colon from the string
						if(ptr[0] != '\0')
						{
							strncpy(charter, ptr, sizeof(charter) - 1);	//Store the charter name
							(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tRead charter name \"%s\"", charter);
							eof_log(eof_log_string, 2);
						}

						//Read next line defining the difficulty level
						(void) pack_fgets(buffer, (int)maxlinelength, inf);
						linectr++;	//Increment line counter
						if(pack_feof(inf))
						{
							error = 1;	//Failed to read difficulty level
							continue;
						}
						if(strcasestr_spec(buffer, "Beginner"))
							note_diff_in_progress = 0;
						else if(strcasestr_spec(buffer, "Easy"))
							note_diff_in_progress = 1;
						else if(strcasestr_spec(buffer, "Medium"))
							note_diff_in_progress = 2;
						else if(strcasestr_spec(buffer, "Hard"))
							note_diff_in_progress = 3;
						else if(strcasestr_spec(buffer, "Challenge"))
							note_diff_in_progress = 4;
						else
						{
							(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tUnrecognized difficulty level \"%s\"", buffer);
							eof_log(eof_log_string, 1);
							error =1;	//Did not read a valid difficulty level
							continue;
						}

						//Read and ignore the difficulty rating
						(void) pack_fgets(buffer, (int)maxlinelength, inf);
						linectr++;	//Increment line counter
						if(pack_feof(inf))
						{
							error = 1;	//Failed to read difficulty rating
							continue;
						}

						//Read and ignore the 5 "radar values"
						(void) pack_fgets(buffer, (int)maxlinelength, inf);
						linectr++;	//Increment line counter
						if(pack_feof(inf))
						{
							error = 1;	//Failed to read radar values
							continue;
						}
					}
					if(note_diff_parsed[note_diff_in_progress])
					{	//If this difficulty was parsed already
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tMalformed file:  Multiple definitions of difficulty %s.  Aborting", note_diff_names[note_diff_in_progress]);
						eof_log(eof_log_string, 1);
						error = 1;
						continue;
					}
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tRead difficulty name %s", note_diff_names[note_diff_in_progress]);
					eof_log(eof_log_string, 1);
					note_diff_parsed[note_diff_in_progress] = 1;	//Track that this difficulty's notes tag has been seen
					tag_in_progress = 2;	//Expect to parse notes on subsequent lines

					//Read first note entry in the measure
					(void) pack_fgets(buffer, (int)maxlinelength, inf);
					semicolon = eof_stepmania_cleanup_input(buffer);	//Clean up buffer content, tracking whether a semicolon was read
					linectr++;	//Increment line counter

					measurectr = 0;	//Track the number of measures that have been parsed
					measure_in_progress = 0;	//No notes have been read in the first measure of notes yet
				}//If this line contains the #NOTES tag
			}//No tag is in progress yet
		}//No tag is in progress yet

		if(tag_in_progress == 1)
		{	//Process #BPMS tag
			double beat = 0, tempo = 0;
			struct dbAnchor *newlink = NULL;

			ptr = strcasestr_spec(buffer, "#BPMS:");
			while(eof_read_stepmania_tempo(&ptr, &beat, &tempo))
			{	//For each tempo change that can be read from the line buffer
				tempos_parsed++;
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tRead tempo change #%lu:  Beat: %.3f , Tempo: %.3f", tempos_parsed, beat, tempo);
				eof_log(eof_log_string, 2);
				if((tempos_parsed == 1) && (beat != 0.0))
					eof_log("\t!Warning:  First tempo change does not occur at first beat", 1);

				//Initialize and insert the new anchor into the linked list
				newlink = malloc(sizeof(struct dbAnchor));
				if(!newlink)
				{
					eof_log("\tError allocating memory.  Aborting", 1);
					error = 1;
					break;
				}
				memset(newlink, 0, sizeof(struct dbAnchor));	//Init the link to 0
				newlink->BPM = (tempo * 1000.0) + 0.5;	//Store the tempo as thousandths of BPM, rounded to the nearest, suitable for use with the Feedback timing logic
				newlink->chartpos = (beat * 1000.0) + 0.5;	//Store the beat number as thousandths of beats, rounded to the nearest
				if(!chart->anchors)
				{	//If the linked list is empty
					chart->anchors = newlink;	//Point head of list to this link
				}
				else
				{
					if(!curanchor)
					{
						eof_log("\tLogic error.  Aborting", 1);
						free(newlink);
						error = 1;
						break;
					}
					curanchor->next = newlink;	//Conductor points forward to this link
				}
				curanchor = newlink;		//Point conductor to this link
			}//For each tempo change that can be read from the line buffer
			if(semicolon)
			{	//If this line contained a semicolon, it ends any tag in progress during this line
				int midbeatchange = 0;
				unsigned long curppqn = 500000;	//Stores the current tempo (default is 120BPM)
				unsigned long lasttempochangebeat = 0;	//Tracks the beat at which the last tempo change was applied

				tag_in_progress = 0;
				if(!tempos_parsed)
				{
					eof_log("\tNo tempo changes were defined.  Aborting", 1);
					error = 1;
					continue;
				}

				//Apply the tempo changes
				eof_log("\tApplying tempo changes", 1);
				if(!eof_song->tags->tempo_map_locked)
				{	//If the tempo map is not locked
					for(curanchor = chart->anchors; curanchor != NULL; curanchor = curanchor->next)
					{	//For each tempo change in the linked list
						unsigned long thisbeat = curanchor->chartpos / 1000;	//The beat number this tempo change applies to

						//Identify the beat to store this tempo change
						if(midbeatchange)
						{	//If this tempo change was determined by the previous loop iteration to occur mid beat
							thisbeat++;	//The tempo change will apply one beat later in to store it
						}
						while(eof_song->beats <= thisbeat)
						{	//Until enough beats exist in the chart to contain this tempo change
							if(!eof_song_add_beat(eof_song))
							{	//If a beat could not be added
								eof_log("\tError adding beat.  Aborting", 1);
								error = 1;
								break;
							}
						}
						if(midbeatchange)
						{
							eof_song->beat[thisbeat]->flags |= EOF_BEAT_FLAG_MIDBEAT;	//Flag the beat as such so it can be removed after import if the user preference is to do so
							(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tMid beat tempo change at beat %.3f", curanchor->chartpos / 1000.0);
							eof_log(eof_log_string, 1);
						}
						if(error)
							break;

						//Set the tempo appropriately for previous and current beats
						while(lasttempochangebeat < thisbeat)
						{	//For each beat prior to the one that will contain the latest tempo change
							eof_song->beat[lasttempochangebeat]->ppqn = curppqn;	//Apply the previous tempo change to the beat
							lasttempochangebeat++;
						}
						curppqn = (60000000.0 / (curanchor->BPM / 1000.0)) + 0.5;	//Convert latest tempo change
						eof_song->beat[thisbeat]->ppqn = curppqn;				//Apply latest tempo change to the target beat

						//Scan ahead to look for mid beat tempo or TS changes
						midbeatchange = 0;
						if(curanchor->next && (curanchor->next->chartpos % 1000))
						{	//If there is another tempo change after the current one and it occurs mid beat, this beat needs to be anchored and its tempo (and the current effective tempo) altered
							double beatfpos = (curanchor->next->chartpos % 1000) / 1000.0;	//How far into the beat that tempo change occurs
							midbeatchange = 1;
							eof_song->beat[thisbeat]->flags |= EOF_BEAT_FLAG_ANCHOR;
							curppqn = ((double)curppqn * beatfpos) + 0.5;	//Scale the current beat's tempo based on the adjusted beat length (rounded to nearest whole number)
							eof_song->beat[thisbeat]->ppqn = curppqn;	//Update this beat's tempo
						}
						lasttempochangebeat = thisbeat;
					}
				}//If the tempo map is not locked

				//Set the last applied tempo change to the remaining beats in the project
				while(lasttempochangebeat < eof_song->beats)
				{	//For each beat prior to the one that will contain the latest tempo change
					eof_song->beat[lasttempochangebeat]->ppqn = curppqn;	//Apply the previous tempo change to the beat
					lasttempochangebeat++;
				}
				eof_calculate_beats(eof_song);	//Set the beats' timestamps based on their tempo changes

				if(adjust)
				{	//If the project's tempo map was altered and the user opted to auto-adjust the existing notes
					(void) eof_menu_edit_cut_paste(0, 1);	//Apply auto-adjust data for the entire chart
				}
			}
		}
		else if(tag_in_progress == 2)
		{	//Process #NOTES tag
			unsigned long index = 0;

			///If NOTES tag is in progress
				///Read the next note definition
					///Expected to be 4 digits
						///Until 4 digits were read, read the next one
							///If an opening curly brace is parsed, skip until the closing curly brace as it defines unsupported attack effect
							///If an opening bracket is parsed, skip until the closing bracket as it defines unsupported sound effect
			if(!measure_in_progress)
			{	//If this will be the first note read for the current measure
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tParsing measure #%lu on line #%lu", measurectr + 1, linectr);
				eof_log(eof_log_string, 2);
				measure_in_progress = 1;
				notectr = 0;	//No notes have been read in this measure yet
			}

			for(index = 0; ((buffer[index] != '\0') && isspace(buffer[index])); index++);	//Skip any leading whitespace
			if((buffer[index] == ',') || (semicolon && (buffer[index] == '\0')))
			{	//A comma ends the curent measure, as would a line that was only a semicolon (which would be removed to leave just an empty line)
				double interval;
				unsigned long measure_chartpos, measure_length, note_chartpos, notepos, snappos;

				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tEnd of measure on line #%lu.  %lu notes read.", linectr, notectr);
				eof_log(eof_log_string, 2);
				measure_in_progress = 0;

				eof_log("\t\t\tCreating notes:", 2);
				measure_length = chart->resolution * 4;	//The number of chart ticks in a measure, assuming 4 beats per measure
				measure_chartpos = measure_length * measurectr;	//The chart position of this measure
				interval = (double)measure_length / (double)notectr;	//The length of the interval between each note definition in this measure
				for(ctr = 0; (ctr < notectr) && !error; ctr++)
				{	//For each note parsed for this measure, or until an error occurs
					//Calculate timing
					note_chartpos = measure_chartpos + (interval * ctr) + 0.5;	//The chart position of this note definition
					notepos = eof_chartpos_to_msec(chart, note_chartpos, NULL) + 0.5;	//Convert this chart position to milliseconds
					while((eof_song->beats < 2) || (notepos > eof_song->beat[eof_song->beats - 1]->pos))
					{	//Until the project has enough beats to encompass this note
						if(eof_song_append_beats(eof_song, 1))
						{	//If a beat was successfully added
							unsigned long newbeatpos = eof_song->beat[eof_song->beats - 1]->pos;
							if(newbeatpos > eof_chart_length)
								eof_chart_length = newbeatpos;	//Update eof_chart_length if appropriate, so that the grid snap logic will work
						}
					}
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\tNote entry #%lu, pos = %lums", ctr, notepos);
					eof_log(eof_log_string, 2);
					if(!eof_is_any_beat_interval_position(notepos, NULL, NULL, NULL, &snappos, eof_prefer_midi_friendly_grid_snapping))
					{	//If that millisecond position isn't grid snapped
						if(snappos != ULONG_MAX)
						{	//If a valid snap position was determined
							notepos = snappos;	//Resnap it
							(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\t\tResnapped to %lums", notepos);
							eof_log(eof_log_string, 2);
						}
					}

					//Add note gems
					for(ctr2 = 0; ctr2 < 4; ctr2++)
					{	//For each of the 4 lanes in this dance chart
						EOF_NOTE *newnote;
						unsigned long newnotenum;
						char gemtype = notes[ctr].gem[ctr2];	//Simplify
						if(gemtype == '3')
						{	//The end of a hold/roll note on this lane
							if(last_hold[ctr2] == ULONG_MAX)
							{	//If there is no beginning defined for such a note
								(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\t\tMalformed file:  End of hold note specified with no defined beginning on line #%lu", linectr);
								eof_log(eof_log_string, 1);
							}
							else
							{	//Update the length of the hold/roll note in progress on this lane
								unsigned long newlength = notepos - eof_get_note_pos(eof_song, EOF_TRACK_DANCE, last_hold[ctr2]);
								eof_set_note_length(eof_song, EOF_TRACK_DANCE, last_hold[ctr2], newlength);
								(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\t\tSustain note #%lu modified (lane #%lu, endpos = %lums)", last_hold[ctr2], ctr2, newlength);
								eof_log(eof_log_string, 1);
								last_hold[ctr2] = ULONG_MAX;	//Reset this to indicate no hold/roll note is in progress on this lane
							}
						}
						else if(gemtype != '0')
						{	//A new note gem on this lane
							newnote = eof_track_add_create_note(eof_song, EOF_TRACK_DANCE, 1 << ctr2, notepos, 1, note_diff_in_progress, NULL);
							newnotenum = eof_get_track_size(eof_song, EOF_TRACK_DANCE);
							if(!newnote || !newnotenum)
							{
								(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\t\tFailed to add note.  Aborting");
								eof_log(eof_log_string, 1);
								error = 1;
								break;
							}

							newnotenum--;	//The index of the last added note is the track size minus one, since the notes are not sorted
							if(gemtype == '2')
							{	//If this is the beginning of a hold note
								last_hold[ctr2] = newnotenum;	//Store the index of this new note to be updated when the note ends
								(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\t\tHold note #%lu added (lane #%lu)", newnotenum, ctr2);
							}
							else if(gemtype == '4')
							{	//If this is the beginning of a roll note
								last_hold[ctr2] = newnotenum;	//Store the index of this new note to be updated when the note ends
								eof_set_note_roll(eof_song, EOF_TRACK_DANCE, newnotenum, 1 << ctr2);	//Make the gem that was just added a roll
								(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\t\tRoll note #%lu added (lane #%lu)", newnotenum, ctr2);
							}
							else if(gemtype == 'M')
							{
								eof_set_note_mine(eof_song, EOF_TRACK_DANCE, newnotenum, 1 << ctr2);	//Make the gem that was just added a mine
								(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\t\tMine note #%lu added (lane #%lu)", newnotenum, ctr2);
							}
							else
								(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\t\tTap note #%lu added (lane #%lu)", newnotenum, ctr2);

							///Determine whether to add support for 'L' (lift) and 'F' (fake) note types, each of which would need a bitmask to track per-lane
							eof_log(eof_log_string, 1);
						}
					}
				}//For each note parsed for this measure
				measurectr++;
			}
			else if(buffer[index] != '\0')
			{	//If the line isn't blank (ie. an end of tag semicolon that was removed or just padding), a note definition of four ASCII characters and an end of line is expected
				if(notectr >= EOF_MAX_SM_NOTES)
				{
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tNote per measure limit of %d exceeded on line #%lu.  Aborting", EOF_MAX_SM_NOTES, linectr);
					eof_log(eof_log_string, 1);
					error = 1;
					continue;
				}
				if((strlen(&buffer[index]) < 4) || (buffer[index + 4] != '\0'))
				{
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tMalformed note entry on line #%lu.  Aborting", linectr);
					eof_log(eof_log_string, 1);
					error = 1;
					continue;
				}
				notes[notectr].gem[0] = buffer[index++];	//Read each of the four bytes into the notes array
				notes[notectr].gem[1] = buffer[index++];
				notes[notectr].gem[2] = buffer[index++];
				notes[notectr].gem[3] = buffer[index++];
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tNote read:  %c%c%c%c", notes[notectr].gem[0], notes[notectr].gem[1], notes[notectr].gem[2], notes[notectr].gem[3]);
				eof_log(eof_log_string, 2);
				notectr++;
			}

			if(semicolon)
			{	//If this line contained a semicolon, it ends any tag in progress during this line
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tEnd of #NOTES tag on line #%lu", linectr);
				eof_log(eof_log_string, 2);
				tag_in_progress = 0;

				for(ctr = 0; ctr < 4; ctr++)
				{	//For each of the 4 lanes
					if(last_hold[ctr] != ULONG_MAX)
					{	//If a hold or roll note didn't have its ending defined
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tMalformed file:  Hold/roll note #%lu with no defined ending", last_hold[ctr]);
						eof_log(eof_log_string, 2);
						last_hold[ctr] = ULONG_MAX;
					}
				}

				if(is_ssc)
				{
					pad_defined = type_defined = diff_defined = 1;	//Reset these statuses, they are defined per track difficulty
				}
			}
		}//Process #NOTES tag
	}//While an error was not encountered

	eof_track_sort_notes(eof_song, EOF_TRACK_DANCE);
	eof_track_find_crazy_notes(eof_song, eof_selected_track, 1);		//Mark overlapping notes with crazy status
	eof_track_find_disjointed_notes(eof_song, eof_selected_track);	//Mark notes that begin at the same timestamp and have different lengths as disjointed
	eof_truncate_chart(eof_song);								//Ensure a suitable number of beats are in the project and set eof_chart_length so the fixup logic doesn't delete notes improperly
	eof_track_fixup_notes(eof_song, EOF_TRACK_DANCE, 1);		//Merge gems into chords as appropriate, perform other cleanup

	//Apply metadata
	if(eof_check_string(title))
	{	//If a song title was defined
		if(!eof_check_string(eof_song->tags->title))
		{	//If the song title isn't defined for the active project
			(void) ustrcpy(eof_song->tags->title, title);	//Define it in song properties
		}
	}
	if(eof_check_string(artist))
	{	//If the artist was defined
		if(!eof_check_string(eof_song->tags->artist))
		{	//If the artist isn't defined for the active project
			(void) ustrcpy(eof_song->tags->artist, artist);	//Define it in song properties
		}
	}
	if(eof_check_string(charter))
	{	//If the chart author was defined
		if(!eof_check_string(eof_song->tags->frettist))
		{	//If the chart author isn't defined for the active project
			(void) ustrcpy(eof_song->tags->frettist, charter);	//Define it in song properties
		}
	}
	else if(eof_check_string(credit))
	{	//If the SSC chart author tag was defined
		if(!eof_check_string(eof_song->tags->frettist))
		{	//If the chart author isn't defined for the active project
			(void) ustrcpy(eof_song->tags->frettist, credit);	//Define it in song properties
		}
	}
	if(eof_check_string(genre))
	{	//If the genre was defined
		if(!eof_check_string(eof_song->tags->genre))
		{	//If the genre isn't defined for the active project
			(void) ustrcpy(eof_song->tags->genre, genre);	//Define it in song properties
		}
	}

	//Apply song preview timings
	if(samplestart_parsed != ULONG_MAX)
	{	//If the preview start time (in milliseconds) was read
		unsigned long index = 0, end_time;

		if(!eof_find_ini_setting_tag(eof_song, &index, "preview_start_time"))
		{	//If the preview start time isn't defined for the active project
			if(samplelength_parsed != ULONG_MAX)
				end_time = samplestart_parsed + samplelength_parsed;	//If the preview length was read, use it
			else
				end_time = samplelength_parsed + 30000;	//Otherwise default to 30 seconds

			if(eof_song->tags->ini_settings + 2 < EOF_MAX_INI_SETTINGS)
			{	//If the preview start and end INI tags can be stored into the project
				(void) snprintf(eof_song->tags->ini_setting[eof_song->tags->ini_settings], EOF_INI_LENGTH - 1, "preview_start_time = %lu", samplestart_parsed);
				eof_song->tags->ini_settings++;
				(void) snprintf(eof_song->tags->ini_setting[eof_song->tags->ini_settings], EOF_INI_LENGTH - 1, "preview_end_time = %lu", end_time);
				eof_song->tags->ini_settings++;
			}
		}
	}

	//Cleanup
	free(buffer);
	while(chart->anchors)
	{	//While there are links in the anchors list
		struct dbAnchor *next = chart->anchors->next;	//Retain the address to the next link
		free(chart->anchors);	//Free this link
		chart->anchors = next;	//Update the head of the list
	}
	free(chart);
	pack_fclose(inf);
	eof_log("\tStepmania import complete", 1);

	return error;	//Return current error status
}

int eof_read_stepmania_tempo(char **strptr, double *beat, double *tempo)
{
	#define EOF_SM_TEMPO_STRING_SIZE 15
	char beatstr[EOF_SM_TEMPO_STRING_SIZE + 1] = {0}, tempostr[EOF_SM_TEMPO_STRING_SIZE + 1] = {0};
	unsigned index1, index2;
	double beatvar, tempovar;
	char *string;

	if(!strptr || !(*strptr))
		return 0;	//Error
	string = *strptr;	//Simplify

	//Read the beat number
	for(index1 = index2 = 0; string[index1] != '='; index1++)
	{	//Until the equal sign is read
		if(index2 >= EOF_SM_TEMPO_STRING_SIZE)
			return 0;	//Malformed beat number
		if(string[index1] == '\0')
			return 0;	//End of line
		beatstr[index2++] = string[index1];	//Append the next character to the beat number string
	}
	beatstr[index2] = '\0';	//Terminate the beat number string
	beatvar = atof(beatstr);
	if((beatvar == 0.0) && !eof_string_is_zero(beatstr))
		return 0;	//Beat number string could not be parsed as a number

	//Read the tempo
	index1++;	//Seek past the equal sign
	for(index2 = 0; string[index1] != ','; index1++)
	{	//Until a comma is read to separate this tempo change from the next
		if(index2 >= EOF_SM_TEMPO_STRING_SIZE)
			return 0;	//Malformed tempo
		if(string[index1] == '\0')
			break;	//End of line (the semicolon would have been truncated from the input by now)
		if(string[index1] == ';')
			break;	//End of #BPMS tag
		tempostr[index2++] = string[index1];	//Append the next character to the tempo string
	}
	if(string[index1] == ',')
		index1++;	//Seek past the comma if it delimited this tempo definition
	tempostr[index2] = '\0';	//Terminate the tempo string
	tempovar = atof(tempostr);
	if(tempovar == 0.0)
		return 0;	//Tempo number string could not be parsed as a number (or it was actually defined as 0.0, which would be invalid)

	if(beat)
		*beat = beatvar;		//Store the beat number if the calling function wanted it
	if(tempo)
		*tempo = tempovar;	//Store the tempo if the calling function wanted it
	*strptr = &(string[index1]);	//Pass the current position in the input string back to the calling function

	return 1;	//Success
}
