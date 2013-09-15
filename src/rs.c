#include <allegro.h>
#include <time.h>
#include "agup/agup.h"
#include "beat.h"
#include "dialog.h"
#include "event.h"
#include "main.h"
#include "midi.h"
#include "mix.h"	//For eof_set_seek_position()
#include "rs.h"
#include "song.h"	//For eof_pro_guitar_track_delete_hand_position()
#include "tuning.h"	//For eof_lookup_tuned_note()
#include "undo.h"
#include "utility.h"	//For eof_system()
#include "foflc/RS_parse.h"	//For expand_xml_text()
#include "menu/beat.h"	//For eof_rocksmith_phrase_dialog_add()
#include "menu/song.h"	//For eof_menu_track_selected_track_number()
#include "menu/track.h"	//For eof_fret_hand_position_list_dialog_undo_made and eof_fret_hand_position_list_dialog[]

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

EOF_RS_PREDEFINED_SECTION eof_rs_predefined_sections[EOF_NUM_RS_PREDEFINED_SECTIONS] =
{
	{"intro", "Intro"},
	{"outro", "Outro"},
	{"verse", "Verse"},
	{"chorus", "Chorus"},
	{"bridge", "Bridge"},
	{"solo", "Solo"},
	{"ambient", "Ambient"},
	{"breakdown", "Breakdown"},
	{"interlude", "Interlude"},
	{"prechorus", "Pre Chorus"},
	{"transition", "Transition"},
	{"postchorus", "Post Chorus"},
	{"hook", "Hook"},
	{"riff", "Riff"},
	{"fadein", "Fade In"},
	{"fadeout", "Fade Out"},
	{"buildup", "Buildup"},
	{"preverse", "Pre Verse"},
	{"modverse", "Modulated Verse"},
	{"postvs", "Post Verse"},
	{"variation", "Variation"},
	{"modchorus", "Modulated Chorus"},
	{"head", "Head"},
	{"modbridge", "Modulated Bridge"},
	{"melody", "Melody"},
	{"postbrdg", "Post Bridge"},
	{"prebrdg", "Pre Bridge"},
	{"vamp", "Vamp"},
	{"noguitar", "No Guitar"},
	{"silence", "Silence"}
};

EOF_RS_PREDEFINED_SECTION eof_rs_predefined_events[EOF_NUM_RS_PREDEFINED_EVENTS] =
{
	{"B0", "High pitch tick (B0)"},
	{"B1", "Low pitch tick (B1)"},
	{"E1", "Crowd happy (E1)"},
	{"E3", "Crowd wild (E3)"}
};

unsigned char *eof_fret_range_tolerances = NULL;	//A dynamically allocated array that defines the fretting hand's range for each fret on the guitar neck, numbered where fret 1's range is defined at eof_fret_range_tolerances[1]

char *eof_rs_arrangement_names[5] = {"Undefined", "Combo", "Rhythm", "Lead", "Bass"};	//Indexes 1 through 4 represent the valid arrangement names for Rocksmith arrangements

int eof_is_string_muted(EOF_SONG *sp, unsigned long track, unsigned long note)
{
	unsigned long ctr, bitmask;
	EOF_PRO_GUITAR_TRACK *tp;
	int allmuted = 1;	//Will be set to nonzero if any used strings aren't fret hand muted

	if(!sp || (track >= sp->tracks) || (note >= eof_get_track_size(sp, track)) || (sp->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return 0;	//Return error

	tp = sp->pro_guitar_track[sp->track[track]->tracknum];
	for(ctr = 0, bitmask = 1; ctr < 6; ctr++, bitmask <<= 1)
	{	//For each of the 6 supported strings
		if(ctr < tp->numstrings)
		{	//If this is a string used in the track
			if(tp->note[note]->note & bitmask)
			{	//If this is a string used in the note
				if((tp->note[note]->frets[ctr] & 0x80) == 0)
				{	//If this string is not fret hand muted
					allmuted = 0;
					break;
				}
			}
		}
	}

	return allmuted;	//Return nonzero if all of the used strings were fret hand muted
}

unsigned long eof_build_chord_list(EOF_SONG *sp, unsigned long track, unsigned long **results)
{
	unsigned long ctr, ctr2, unique_count = 0;
	EOF_PRO_GUITAR_NOTE **notelist;	//An array large enough to hold a pointer to every note in the track
	EOF_PRO_GUITAR_TRACK *tp;
	char match;

	eof_log("eof_rs_build_chord_list() entered", 1);

	if(!results)
		return 0;	//Return error

	if(!sp || (track >= sp->tracks))
	{
		*results = NULL;
		return 0;	//Return error
	}

	//Duplicate the track's note array
	notelist = malloc(sizeof(EOF_PRO_GUITAR_NOTE *) * EOF_MAX_NOTES);	//Allocate memory to duplicate the note[] array
	if(!notelist)
	{
		*results = NULL;
		return 0;	//Return error
	}
	tp = sp->pro_guitar_track[sp->track[track]->tracknum];
	memcpy(notelist, tp->note, sizeof(EOF_PRO_GUITAR_NOTE *) * EOF_MAX_NOTES);	//Copy the note array

	//Overwrite each pointer in the duplicate note array that isn't a unique chord with NULL
	for(ctr = 0; ctr < tp->notes; ctr++)
	{	//For each note in the track
		if((eof_note_count_colors(sp, track, ctr) > 1) && !eof_is_string_muted(sp, track, ctr))
		{	//If this note is a chord (has a gem on at least two lanes) and is not a fully string muted chord
			match = 0;
			for(ctr2 = ctr + 1; ctr2 < tp->notes; ctr2++)
			{	//For each note in the track that follows this note
				if(!eof_note_compare_simple(sp, track, ctr, ctr2))
				{	//If this note matches one that follows it
					notelist[ctr] = NULL;	//Eliminate this note from the list
					match = 1;	//Note that this chord matched one of the others
					break;
				}
			}
			if(!match)
			{	//If this chord didn't match any of the other notes
				unique_count++;	//Increment unique chord counter
			}
		}
		else
		{	//This not is not a chord
			notelist[ctr] = NULL;	//Eliminate this note from the list since it's not a chord
		}
	}

	if(!unique_count)
	{	//If there were no chords
		*results = NULL;	//Return empty result set
		free(notelist);
		return 0;
	}

	//Allocate and build an array with the note numbers representing the unique chords
	*results = malloc(sizeof(unsigned long) * unique_count);	//Allocate enough memory to store the note number of each unique chord
	if(*results == NULL)
	{
		free(notelist);
		return 0;
	}
	for(ctr = 0, ctr2 = 0; ctr < tp->notes; ctr++)
	{	//For each note in the track
		if(notelist[ctr] != NULL)
		{	//If this was a unique chord
			if(ctr2 < unique_count)
			{	//Bounds check
				(*results)[ctr2++] = ctr;	//Append the note number
			}
		}
	}

	//Cleanup and return results
	free(notelist);
	return unique_count;
}

unsigned long eof_build_section_list(EOF_SONG *sp, unsigned long **results, unsigned long track)
{
	unsigned long ctr, ctr2, unique_count = 0;
	EOF_TEXT_EVENT **eventlist;	//An array large enough to hold a pointer to every text event in the chart
	char match;

	eof_log("eof_build_section_list() entered", 1);

	if(!results)
		return 0;	//Return error

	if(!sp)
	{
		*results = NULL;
		return 0;	//Return error
	}

	//Duplicate the chart's text events array
	eventlist = malloc(sizeof(EOF_TEXT_EVENT *) * EOF_MAX_TEXT_EVENTS);
	if(!eventlist)
	{
		*results = NULL;
		return 0;	//Return error
	}
	memcpy(eventlist, sp->text_event, sizeof(EOF_TEXT_EVENT *) * EOF_MAX_TEXT_EVENTS);	//Copy the event array

	//In the case of beats that contain multiple sections, only keep ones that are cached in the beat statistics
	eof_process_beat_statistics(sp, track);	//Rebuild beat stats from the perspective of the track being examined
	for(ctr = 0; ctr < sp->text_events; ctr++)
	{	//For each text event in the chart
		match = 0;
		for(ctr2 = 0; ctr2 < sp->beats; ctr2++)
		{	//For each beat in the chart
			if(sp->beat[ctr2]->contained_section_event == ctr)
			{	//If the beat's statistics indicate this section is used
				match = 1;	//Note that this section is to be kept
				break;
			}
		}
		if(!match)
		{	//If none of the beat stats used this section
			eventlist[ctr] = NULL;	//Eliminate this section from the list since only 1 section per beat will be exported
		}
	}

	//Overwrite each pointer in the duplicate event array that isn't a unique section marker with NULL and count the unique events
	for(ctr = 0; ctr < sp->text_events; ctr++)
	{	//For each text event in the chart
		if(eventlist[ctr] != NULL)
		{	//If this section hasn't been eliminated yet
			if(eof_is_section_marker(sp->text_event[ctr], track))
			{	//If the text event's string or flags indicate a section marker (from the perspective of the specified track)
				match = 0;
				for(ctr2 = ctr + 1; ctr2 < sp->text_events; ctr2++)
				{	//For each event in the chart that follows this event
					if(eventlist[ctr2] != NULL)
					{	//If the event wasn't already eliminated
						if(eof_is_section_marker(sp->text_event[ctr2], track) &&!ustricmp(sp->text_event[ctr]->text, sp->text_event[ctr2]->text))
						{	//If this event is also a section event from the perspective of the track being examined, and its text matches
							eventlist[ctr] = NULL;	//Eliminate this event from the list
							match = 1;	//Note that this section matched one of the others
							break;
						}
					}
				}
				if(!match)
				{	//If this section marker didn't match any of the other events
					unique_count++;	//Increment unique section counter
				}
			}
			else
			{	//This event is not a section marker
				eventlist[ctr] = NULL;	//Eliminate this note from the list since it's not a chord
			}
		}
	}

	if(!unique_count)
	{	//If there were no section markers
		*results = NULL;	//Return empty result set
		free(eventlist);
		return 0;
	}

	//Allocate and build an array with the event numbers representing the unique section markers
	*results = malloc(sizeof(unsigned long) * unique_count);	//Allocate enough memory to store the event number of each unique section marker
	if(*results == NULL)
	{
		free(eventlist);
		return 0;
	}
	for(ctr = 0, ctr2 = 0; ctr < sp->text_events; ctr++)
	{	//For each event in the chart
		if(eventlist[ctr] != NULL)
		{	//If this was a unique section marker
			if(ctr2 < unique_count)
			{	//Bounds check
				(*results)[ctr2++] = ctr;	//Append the event number
			}
		}
	}

	//Cleanup and return results
	free(eventlist);
	return unique_count;
}

int eof_song_qsort_control_events(const void * e1, const void * e2)
{
	EOF_RS_CONTROL * thing1 = (EOF_RS_CONTROL *)e1;
	EOF_RS_CONTROL * thing2 = (EOF_RS_CONTROL *)e2;

	//Sort by timestamp
	if(thing1->pos < thing2->pos)
	{
		return -1;
	}
	else if(thing1->pos > thing2->pos)
	{
		return 1;
	}

	//They are equal
	return 0;
}

int eof_export_rocksmith_track(EOF_SONG * sp, char * fn, unsigned long track, char *user_warned)
{
	PACKFILE * fp;
	char buffer[600] = {0}, buffer2[512] = {0};
	time_t seconds;		//Will store the current time in seconds
	struct tm *caltime;	//Will store the current time in calendar format
	unsigned long ctr, ctr2, ctr3, ctr4, ctr5, numsections, stringnum, bitmask, numsinglenotes, numchords, *chordlist, chordlistsize, *sectionlist, sectionlistsize, xml_end, numevents = 0;
	EOF_PRO_GUITAR_TRACK *tp;
	char *arrangement_name;	//This will point to the track's native name unless it has an alternate name defined
	unsigned numdifficulties;
	unsigned char bre_populated = 0;
	unsigned long phraseid;
	unsigned beatspermeasure = 4, beatcounter = 0;
	long displayedmeasure, measurenum = 0;
	long startbeat;	//This will indicate the first beat containing a note in the track
	long endbeat;	//This will indicate the first beat after the exported track's last note
	char standard[] = {0,0,0,0,0,0};
	char standardbass[] = {0,0,0,0};
	char eb[] = {-1,-1,-1,-1,-1,-1};
	char dropd[] = {-2,0,0,0,0,0};
	char openg[] = {-2,-2,0,0,0,-2};
	char *tuning;
	char isebtuning = 1;	//Will track whether all strings are tuned to -1
	char notename[EOF_NAME_LENGTH+1];	//String large enough to hold any chord name supported by EOF
	int scale, chord, isslash, bassnote;	//Used for power chord detection
	int standard_tuning = 0, non_standard_chords = 0, barre_chords = 0, power_chords = 0, notenum, dropd_tuning = 1, dropd_power_chords = 0, open_chords = 0, double_stops = 0, palm_mutes = 0, harmonics = 0, hopo = 0, tremolo = 0, slides = 0, bends = 0, tapping = 0, vibrato = 0, slappop = 0, octaves = 0, fifths_and_octaves = 0;	//Used for technique detection
	char is_bass = 0;	//Is set to nonzero if the specified track is to be considered a bass guitar track
	char end_phrase_found = 0;	//Will track if there was a manually defined END phrase
	unsigned long chordid, handshapectr;
	unsigned long handshapestart, handshapeend;
	long nextnote;

	eof_log("eof_export_rocksmith() entered", 1);

	if(!sp || !fn || !sp->beats || (track >= sp->tracks) || (sp->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT) || !sp->track[track]->name || !user_warned)
	{
		eof_log("\tError saving:  Invalid parameters", 1);
		return 0;	//Return failure
	}

	tp = sp->pro_guitar_track[sp->track[track]->tracknum];
	if(eof_get_highest_fret(sp, track, 0) > 22)
	{	//If the track being exported uses any frets higher than 22
		if((*user_warned & 2) == 0)
		{	//If the user wasn't alerted about this issue yet
			allegro_message("Warning:  At least one track (\"%s\") uses a fret higher than 22.  This will cause Rocksmith to crash.", sp->track[track]->name);
			*user_warned |= 2;
		}
	}
	for(ctr = 0; ctr < eof_get_track_size(sp, track); ctr++)
	{	//For each note in the track
		unsigned char slideend;

		if(eof_note_count_rs_lanes(sp, track, ctr) == 1)
		{	//If the note will export as a single note
			if(eof_get_note_flags(sp, track, ctr) & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP)
			{	//If the note slides up
				slideend = tp->note[ctr]->slideend;
				if(!(eof_get_note_flags(sp, track, ctr) & EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION))
				{	//If this slide's end position is not defined
					slideend = eof_get_highest_fret_value(sp, track, ctr) + 1;	//Assume a 1 fret slide
				}
				if(slideend >= 22)
				{	//If the slide goes to or above fret 22
					if((*user_warned & 8) == 0)
					{	//If the user wasn't alerted about this issue yet
						eof_set_seek_position(tp->note[ctr]->pos + eof_av_delay);	//Seek to the note
						(void) eof_menu_track_selected_track_number(track, 1);		//Set the active instrument track
						eof_find_lyric_preview_lines();
						eof_render();					//Redraw the screen
						allegro_message("Warning:  At least one note slides to or above fret 22.  This will cause Rocksmith to crash.");
						*user_warned |= 8;
					}
					break;
				}
			}
		}
	}

	//Count the number of populated difficulties in the track
	(void) eof_detect_difficulties(sp, track);	//Update eof_track_diff_populated_status[] to reflect all populated difficulties for this track
	if((sp->track[track]->flags & EOF_TRACK_FLAG_UNLIMITED_DIFFS) == 0)
	{	//If the track is using the traditional 5 difficulty system
		if(eof_track_diff_populated_status[4])
		{	//If the BRE difficulty is populated
			bre_populated = 1;	//Track that it was
		}
		eof_track_diff_populated_status[4] = 0;	//Ensure that the BRE difficulty is not exported
	}
	for(ctr = 0, numdifficulties = 0; ctr < 256; ctr++)
	{	//For each possible difficulty
		if(eof_track_diff_populated_status[ctr])
		{	//If this difficulty is populated
			numdifficulties++;	//Increment this counter
		}
	}
	if(!numdifficulties)
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Cannot export track \"%s\"in Rocksmith format, it has no populated difficulties", sp->track[track]->name);
		eof_log(eof_log_string, 1);
		if(bre_populated)
		{	//If the BRE difficulty was the only one populated, warn that it is being omitted
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Warning:  Track \"%s\" only has notes in the BRE difficulty.\nThese are not exported in Rocksmith format unless you remove the difficulty limit (Song>Rocksmith>Remove difficulty limit).", sp->track[track]->name);
			allegro_message(eof_log_string);
			eof_log(eof_log_string, 1);
		}
		return 0;	//Return failure
	}

	//Update target file name and open it for writing
	if((sp->track[track]->flags & EOF_TRACK_FLAG_ALT_NAME) && (sp->track[track]->altname[0] != '\0'))
	{	//If the track has an alternate name
		arrangement_name = sp->track[track]->altname;
	}
	else
	{	//Otherwise use the track's native name
		arrangement_name = sp->track[track]->name;
	}
	(void) snprintf(buffer, 600, "%s.xml", arrangement_name);
	(void) replace_filename(fn, fn, buffer, 1024);
	fp = pack_fopen(fn, "w");
	if(!fp)
	{
		eof_log("\tError saving:  Cannot open file for writing", 1);
		return 0;	//Return failure
	}

	//Update the track's arrangement name
	if(tp->arrangement)
	{	//If the track's arrangement type has been defined
		arrangement_name = eof_rs_arrangement_names[tp->arrangement];	//Use the corresponding string in the XML file
	}

	//Get the smaller of the chart length and the music length, this will be used to write the songlength tag
	xml_end = eof_music_length;
	if(eof_silence_loaded || (eof_chart_length < eof_music_length))
	{	//If the chart length is shorter than the music length, or there is no chart audio loaded
		xml_end = eof_chart_length;	//Use the chart's length instead
	}

	//Write the beginning of the XML file
	(void) pack_fputs("<?xml version='1.0' encoding='UTF-8'?>\n", fp);
	(void) pack_fputs("<song>\n", fp);
	expand_xml_text(buffer2, sizeof(buffer2) - 1, sp->tags->title, 64);	//Expand XML special characters into escaped sequences if necessary, and check against the maximum supported length of this field
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <title>%s</title>\n", buffer2);
	(void) pack_fputs(buffer, fp);
	expand_xml_text(buffer2, sizeof(buffer2) - 1, arrangement_name, 32);	//Expand XML special characters into escaped sequences if necessary, and check against the maximum supported length of this field
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <arrangement>%s</arrangement>\n", buffer2);
	(void) pack_fputs(buffer, fp);
	(void) pack_fputs("  <part>1</part>\n", fp);
	(void) pack_fputs("  <offset>0.000</offset>\n", fp);
	eof_truncate_chart(sp);	//Update the chart length
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <songLength>%.3f</songLength>\n", (double)(xml_end - 1) / 1000.0);	//Make sure the song length is not longer than the actual audio, or the chart won't reach an end in-game
	(void) pack_fputs(buffer, fp);
	seconds = time(NULL);
	caltime = localtime(&seconds);
	if(caltime)
	{	//If the calendar time could be determined
		(void) snprintf(buffer, sizeof(buffer) - 1, "  <lastConversionDateTime>%u-%u-%u %u:%02u</lastConversionDateTime>\n", caltime->tm_mon + 1, caltime->tm_mday, caltime->tm_year % 100, caltime->tm_hour % 12, caltime->tm_min);
	}
	else
	{
		(void) snprintf(buffer, sizeof(buffer) - 1, "  <lastConversionDateTime>UNKNOWN</lastConversionDateTime>\n");
	}
	(void) pack_fputs(buffer, fp);

	//Write additional tags to pass song information to the Rocksmith toolkit
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <startBeat>%.3f</startBeat>\n", sp->beat[0]->fpos / 1000.0);	//The position of the first beat
	(void) pack_fputs(buffer, fp);
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <averageTempo>%.3f</averageTempo>\n", 60000.0 / ((sp->beat[sp->beats - 1]->fpos - sp->beat[0]->fpos) / sp->beats));	//The average tempo (60000ms / the average beat length in ms)
	(void) pack_fputs(buffer, fp);
	tuning = tp->tuning;	//By default, use the track's original tuning array
	for(ctr = 0; ctr < 6; ctr++)
	{	//For each string EOF supports
		if(ctr >= tp->numstrings)
		{	//If the track doesn't use this string
			tp->tuning[ctr] = 0;	//Ensure the tuning is cleared accordingly
		}
	}
	for(ctr = 0; ctr < tp->numstrings; ctr++)
	{	//For each string in this track
		if(tp->tuning[ctr] != -1)
		{	//If this string isn't tuned a half step down
			isebtuning = 0;
			break;
		}
	}
	is_bass = eof_track_is_bass_arrangement(tp, track);
	if(isebtuning && !(is_bass && (tp->numstrings > 4)))
	{	//If all strings were tuned down a half step (except for bass tracks with more than 4 strings, since in those cases, the lowest string is not tuned to E)
		tuning = eb;	//Remap 4 or 5 string Eb tuning as {-1,-1,-1,-1,-1,-1}
	}
	if(memcmp(tuning, standard, 6) && memcmp(tuning, standardbass, 4) && memcmp(tuning, eb, 6) && memcmp(tuning, dropd, 6) && memcmp(tuning, openg, 6))
	{	//If the track's tuning doesn't match any supported by Rocksmith
		allegro_message("Warning:  This track (%s) uses a tuning that isn't one known to be supported in Rocksmith.\nTuning and note recognition may not work as expected in-game", sp->track[track]->name);
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Warning:  This track (%s) uses a tuning that isn't known to be supported in Rocksmith.  Tuning and note recognition may not work as expected in-game", sp->track[track]->name);
		eof_log(eof_log_string, 1);
	}
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <tuning string0=\"%d\" string1=\"%d\" string2=\"%d\" string3=\"%d\" string4=\"%d\" string5=\"%d\" />\n", tuning[0], tuning[1], tuning[2], tuning[3], tuning[4], tuning[5]);
	(void) pack_fputs(buffer, fp);
	expand_xml_text(buffer2, sizeof(buffer2) - 1, sp->tags->artist, 256);	//Replace any special characters in the artist song property with escape sequences if necessary
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <artistName>%s</artistName>\n", buffer2);
	(void) pack_fputs(buffer, fp);
	expand_xml_text(buffer2, sizeof(buffer2) - 1, sp->tags->album, 256);	//Replace any special characters in the album song property with escape sequences if necessary
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <albumName>%s</albumName>\n", buffer2);
	(void) pack_fputs(buffer, fp);
	expand_xml_text(buffer2, sizeof(buffer2) - 1, sp->tags->year, 32);	//Replace any special characters in the year song property with escape sequences if necessary
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <albumYear>%s</albumYear>\n", buffer2);
	(void) pack_fputs(buffer, fp);
	if(!memcmp(tuning, standard, 6))
	{	//All unused strings had their tuning set to 0, so if all bytes of this array are 0, the track is in standard tuning
		standard_tuning = 1;
	}
	notenum = eof_lookup_tuned_note(tp, track, 0, tp->tuning[0]);	//Look up the open note the lowest string plays
	notenum %= 12;	//Ensure the value is in the range of [0,11]
	if(notenum == 5)
	{	//If the lowest string is tuned to D
		for(ctr = 1; ctr < 6; ctr++)
		{	//For the other 5 strings
			if(tp->tuning[ctr] != 0)
			{	//If the string is not in standard tuning
				dropd_tuning = 0;
			}
		}
	}
	else
	{	//The lowest string is not tuned to D
		dropd_tuning = 0;
	}
	eof_determine_phrase_status(sp, track);	//Update the tremolo status of each note
	for(ctr = 0; ctr < tp->notes; ctr++)
	{	//For each note in the track
		if(eof_note_count_rs_lanes(sp, track, ctr) > 1)
		{	//If the note will export as a chord (more than one non ghosted/muted gem)
			if(!non_standard_chords && !eof_build_note_name(sp, track, ctr, notename))
			{	//If the chord has no defined or detected name (only if this condition hasn't been found already)
				non_standard_chords = 1;
			}
			if(!barre_chords && eof_pro_guitar_note_is_barre_chord(tp, ctr))
			{	//If the chord is a barre chord (only if this condition hasn't been found already)
				barre_chords = 1;
			}
			if(!power_chords && eof_lookup_chord(tp, track, ctr, &scale, &chord, &isslash, &bassnote, 0, 0))
			{	//If the chord lookup found a match (only if this condition hasn't been found already)
				if(chord == 27)
				{	//27 is the index of the power chord formula in eof_chord_names[]
					power_chords = 1;
					if(dropd_tuning)
					{	//If this track is in drop d tuning
						dropd_power_chords = 1;
					}
				}
			}
			if(!open_chords)
			{	//Only if no open chords have been found already
				for(ctr2 = 0, bitmask = 1; ctr2 < 6; ctr2++, bitmask <<= 1)
				{	//For each of the 6 supported strings
					if((tp->note[ctr]->note & bitmask) && (tp->note[ctr]->frets[ctr2] == 0))
					{	//If this string is used and played open
						open_chords = 1;
					}
				}
			}
			if(!double_stops && eof_pro_guitar_note_is_double_stop(tp, ctr))
			{	//If the chord is a double stop (only if this condition hasn't been found already)
				double_stops = 1;
			}
			if(is_bass)
			{	//If the arrangement being exported is bass
				int thisnote, lastnote = -1, failed = 0;
				for(ctr2 = 0, bitmask = 1; ctr2 < 6; ctr2++, bitmask <<= 1)
				{	//For each of the 6 supported strings
					if((tp->note[ctr]->note & bitmask) && !(tp->note[ctr]->frets[ctr2] & 0x80))
					{	//If this string is played and not string muted
						thisnote = eof_lookup_played_note(tp, track, ctr2, tp->note[ctr]->frets[ctr2]);	//Determine what note is being played
						if((lastnote >= 0) && (lastnote != thisnote))
						{	//If this string's played note doesn't match the note played by previous strings
							failed = 1;
							break;
						}
						lastnote = thisnote;
					}
				}
				if(!failed)
				{	//If non muted strings in the chord played the same note
					octaves = 1;
				}
			}
		}//If the note is a chord (more than one non ghosted gem)
		if(tp->note[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE)
		{	//If the note is palm muted
			palm_mutes = 1;
		}
		if(tp->note[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC)
		{	//If the note is a harmonic
			harmonics = 1;
		}
		if((tp->note[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_HO) || (tp->note[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_PO))
		{	//If the note is a hammer on or pull off
			hopo = 1;
		}
		if(tp->note[ctr]->flags & EOF_NOTE_FLAG_IS_TREMOLO)
		{	//If the note is played with tremolo
			tremolo = 1;
		}
		if((tp->note[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP) || (tp->note[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN))
		{	//If the note slides up or down
			slides = 1;
		}
		if(tp->note[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_BEND)
		{	//If the note is bent
			bends = 1;
		}
		if(tp->note[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_TAP)
		{	//If the note is tapped
			tapping = 1;
		}
		if(tp->note[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO)
		{	//If the note is played with vibrato
			vibrato = 1;
		}
		if((tp->note[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_SLAP) || (tp->note[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_POP))
		{	//If the note is played by slapping or popping
			slappop = 1;
		}
	}//For each note in the track
	if(is_bass)
	{	//If the arrangement being exported is bass
		if(double_stops || octaves)
		{	//If either of these techniques were detected
			fifths_and_octaves = 1;
		}
		double_stops = 0;
	}
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <arrangementProperties represent=\"1\" standardTuning=\"%d\" nonStandardChords=\"%d\" barreChords=\"%d\" powerChords=\"%d\" dropDPower=\"%d\" openChords=\"%d\" fingerPicking=\"0\" pickDirection=\"0\" doubleStops=\"%d\" palmMutes=\"%d\" harmonics=\"%d\" pinchHarmonics=\"0\" hopo=\"%d\" tremolo=\"%d\" slides=\"%d\" unpitchedSlides=\"0\" bends=\"%d\" tapping=\"%d\" vibrato=\"%d\" fretHandMutes=\"0\" slapPop=\"%d\" twoFingerPicking=\"0\" fifthsAndOctaves=\"%d\" syncopation=\"0\" bassPick=\"0\" />\n", standard_tuning, non_standard_chords, barre_chords, power_chords, dropd_power_chords, open_chords, double_stops, palm_mutes, harmonics, hopo, tremolo, slides, bends, tapping, vibrato, slappop, fifths_and_octaves);
	(void) pack_fputs(buffer, fp);

	//Check if any RS phrases or sections need to be added
	startbeat = eof_get_beat(sp, tp->note[0]->pos);	//Find the beat containing the track's first note
	if(startbeat < 0)
	{	//If the beat couldn't be found
		startbeat = 0;	//Set this to the first beat
	}
	endbeat = eof_get_beat(sp, tp->note[tp->notes - 1]->pos + tp->note[tp->notes - 1]->length);	//Find the beat containing the end of the track's last note
	if((endbeat < 0) || (endbeat + 1 >= sp->beats))
	{	//If the beat couldn't be found, or the extreme last beat in the track has the last note
		endbeat = sp->beats - 1;	//Set this to the last beat
	}
	else
	{
		endbeat++;	//Otherwise set it to the first beat that follows the end of the last note
	}
	eof_process_beat_statistics(sp, track);	//Cache section name information into the beat structures (from the perspective of the specified track)
	if(!eof_song_contains_event(sp, "COUNT", track, EOF_EVENT_FLAG_RS_PHRASE, 1) && !eof_song_contains_event(sp, "COUNT", 0, EOF_EVENT_FLAG_RS_PHRASE, 1))
	{	//If the user did not define a COUNT phrase that applies to either the track being exported or all tracks
		if(sp->beat[0]->contained_section_event >= 0)
		{	//If there is already a phrase defined on the first beat
			allegro_message("Warning:  There is no COUNT phrase, but the first beat marker already has a phrase.\nYou should move that phrase because only one phrase per beat is exported.");
		}
		eof_log("\t! Adding missing COUNT phrase", 1);
		(void) eof_song_add_text_event(sp, 0, "COUNT", 0, EOF_EVENT_FLAG_RS_PHRASE, 1);	//Add it as a temporary event at the first beat
	}
	for(ctr = 0; ctr < sp->beats; ctr++)
	{	//For each beat
		if((sp->beat[ctr]->contained_section_event >= 0) && !ustricmp(sp->text_event[sp->beat[ctr]->contained_section_event]->text, "END"))
		{	//If this beat contains an "END" RS phrase
			for(ctr2 = ctr + 1; ctr2 < sp->beats; ctr2++)
			{	//For each remaining beat
				if((sp->beat[ctr2]->contained_section_event >= 0) || (sp->beat[ctr2]->contained_rs_section_event >= 0))
				{	//If the beat contains an RS phrase or RS section
					eof_2d_render_top_option = 36;	//Change the user preference to display RS phrases and sections
					eof_set_seek_position(sp->beat[ctr]->pos + eof_av_delay);	//Seek to the beat containing the offending END phrase
					eof_selected_beat = ctr;		//Select it
					eof_find_lyric_preview_lines();
					eof_render();					//Redraw the screen
					allegro_message("Warning:  Beat #%lu contains an END phrase, but there's at least one more phrase or section after it.\nThis will cause dynamic difficulty and/or riff repeater to not work correctly.", ctr);
					break;
				}
			}
			end_phrase_found = 1;
			break;
		}
	}
	if(!end_phrase_found)
	{	//If the user did not define a END phrase
		if(sp->beat[endbeat]->contained_section_event >= 0)
		{	//If there is already a phrase defined on the beat following the last note
			eof_2d_render_top_option = 36;	//Change the user preference to display RS phrases and sections
			eof_set_seek_position(sp->beat[endbeat]->pos + eof_av_delay);	//Seek to the beat where the END phrase should go
			eof_selected_beat = endbeat;		//Select it
			(void) eof_menu_track_selected_track_number(track, 1);		//Set the active instrument track
			eof_find_lyric_preview_lines();
			eof_render();					//Redraw the screen
			allegro_message("Warning:  There is no END phrase, but the beat marker after the last note in \"%s\" already has a phrase.\nYou should move that phrase because only one phrase per beat is exported.", sp->track[track]->name);
		}
		eof_log("\t! Adding missing END phrase", 1);
		(void) eof_song_add_text_event(sp, endbeat, "END", 0, EOF_EVENT_FLAG_RS_PHRASE, 1);	//Add it as a temporary event at the last beat
	}
	eof_sort_events(sp);	//Re-sort events
	eof_process_beat_statistics(sp, track);	//Cache section name information into the beat structures (from the perspective of the specified track)
	if(!eof_song_contains_event(sp, "intro", track, EOF_EVENT_FLAG_RS_SECTION, 1) && !eof_song_contains_event(sp, "intro", 0, EOF_EVENT_FLAG_RS_SECTION, 1))
	{	//If the user did not define an intro RS section that applies to either the track being exported or all tracks
		if(sp->beat[startbeat]->contained_rs_section_event >= 0)
		{	//If there is already a RS section defined on the first beat containing a note
			allegro_message("Warning:  There is no intro RS section, but the beat marker before the first note already has a section.\nYou should move that section because only one section per beat is exported.");
		}
		eof_log("\t! Adding missing intro RS section", 1);
		(void) eof_song_add_text_event(sp, startbeat, "intro", 0, EOF_EVENT_FLAG_RS_SECTION, 1);	//Add a temporary one
	}
	if(!eof_song_contains_event(sp, "noguitar", track, EOF_EVENT_FLAG_RS_SECTION, 1) && !eof_song_contains_event(sp, "noguitar", 0, EOF_EVENT_FLAG_RS_SECTION, 1))
	{	//If the user did not define a noguitar RS section that applies to either the track being exported or all tracks
		if(sp->beat[endbeat]->contained_rs_section_event >= 0)
		{	//If there is already a RS section defined on the first beat after the last note
			allegro_message("Warning:  There is no noguitar RS section, but the beat marker after the last note already has a section.\nYou should move that section because only one section per beat is exported.");
		}
		eof_log("\t! Adding missing noguitar RS section", 1);
		(void) eof_song_add_text_event(sp, endbeat, "noguitar", 0, EOF_EVENT_FLAG_RS_SECTION, 1);	//Add a temporary one
	}

	//Write the phrases
	eof_sort_events(sp);	//Re-sort events
	eof_process_beat_statistics(sp, track);	//Cache section name information into the beat structures (from the perspective of the specified track)
	for(ctr = 0, numsections = 0; ctr < sp->beats; ctr++)
	{	//For each beat in the chart
		if(sp->beat[ctr]->contained_section_event >= 0)
		{	//If this beat has a section event (RS phrase)
			numsections++;	//Update section marker instance counter
		}
	}
	sectionlistsize = eof_build_section_list(sp, &sectionlist, track);	//Build a list of all unique section markers (Rocksmith phrases) in the chart (from the perspective of the track being exported)
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <phrases count=\"%lu\">\n", sectionlistsize);	//Write the number of unique phrases
	(void) pack_fputs(buffer, fp);
	for(ctr = 0; ctr < sectionlistsize; ctr++)
	{	//For each of the entries in the unique section (RS phrase) list
		char * currentphrase = NULL;
		unsigned long startpos = 0, endpos = 0;		//Track the start and end position of the each instance of the phrase
		unsigned char maxdiff, ongoingmaxdiff = 0;	//Track the highest fully leveled difficulty used among all phraseinstances
		char started = 0;

		//Determine the highest maxdifficulty present among all instances of this phrase
		for(ctr2 = 0; ctr2 < sp->beats; ctr2++)
		{	//For each beat
			if((sp->beat[ctr2]->contained_section_event >= 0) || ((ctr2 + 1 >= eof_song->beats) && started))
			{	//If this beat contains a section event (Rocksmith phrase) or a phrase is in progress and this is the last beat, it marks the end of any current phrase and the potential start of another
				started = 0;
				endpos = sp->beat[ctr2]->pos - 1;	//Track this as the end position of the previous phrase marker
				if(currentphrase)
				{	//If the first instance of the phrase was already encountered
					if(!ustricmp(currentphrase, sp->text_event[sectionlist[ctr]]->text))
					{	//If the phrase that just ended is an instance of the phrase being written
						maxdiff = eof_find_fully_leveled_rs_difficulty_in_time_range(sp, track, startpos, endpos, 1);	//Find the maxdifficulty value for this phrase instance, converted to relative numbering
						if(maxdiff > ongoingmaxdiff)
						{	//If that phrase instance had a higher maxdifficulty than the other instances checked so far
							ongoingmaxdiff = maxdiff;	//Track it
						}
					}
				}
				started = 1;
				startpos = sp->beat[ctr2]->pos;	//Track the starting position
				currentphrase = sp->text_event[eof_song->beat[ctr2]->contained_section_event]->text;	//Track which phrase is being examined
			}
		}

		//Write the phrase definition using the highest difficulty found among all instances of the phrase
		expand_xml_text(buffer2, sizeof(buffer2) - 1, sp->text_event[sectionlist[ctr]]->text, 32);	//Expand XML special characters into escaped sequences if necessary, and check against the maximum supported length of this field
		(void) snprintf(buffer, sizeof(buffer) - 1, "    <phrase disparity=\"0\" ignore=\"0\" maxDifficulty=\"%u\" name=\"%s\" solo=\"0\"/>\n", ongoingmaxdiff, buffer2);
		(void) pack_fputs(buffer, fp);
	}//For each of the entries in the unique section (RS phrase) list
	(void) pack_fputs("  </phrases>\n", fp);
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <phraseIterations count=\"%lu\">\n", numsections);	//Write the number of phrase instances
	(void) pack_fputs(buffer, fp);
	for(ctr = 0; ctr < sp->beats; ctr++)
	{	//For each beat in the chart
		if(sp->beat[ctr]->contained_section_event >= 0)
		{	//If this beat has a section event
			for(ctr2 = 0; ctr2 < sectionlistsize; ctr2++)
			{	//For each of the entries in the unique section list
				if(!ustricmp(sp->text_event[sp->beat[ctr]->contained_section_event]->text, sp->text_event[sectionlist[ctr2]]->text))
				{	//If this event matches a section marker entry
					phraseid = ctr2;
					break;
				}
			}
			if(ctr2 >= sectionlistsize)
			{	//If the section couldn't be found
				allegro_message("Error:  Couldn't find section in unique section list.  Aborting Rocksmith export.");
				eof_log("Error:  Couldn't find section in unique section list.  Aborting Rocksmith export.", 1);
				free(sectionlist);
				return 0;	//Return error
			}
			(void) snprintf(buffer, sizeof(buffer) - 1, "    <phraseIteration time=\"%.3f\" phraseId=\"%lu\"/>\n", sp->beat[ctr]->fpos / 1000.0, phraseid);
			(void) pack_fputs(buffer, fp);
		}
	}
	(void) pack_fputs("  </phraseIterations>\n", fp);
	if(sectionlistsize)
	{	//If there were any entries in the unique section list
		free(sectionlist);	//Free the list now
	}

	//Write some unknown information
	(void) pack_fputs("  <linkedDiffs count=\"0\"/>\n", fp);
	(void) pack_fputs("  <phraseProperties count=\"0\"/>\n", fp);

	if(sp->tags->rs_chord_technique_export)
	{	//If the user opted to export chord techniques to the Rocksmith XML files
		//Check for chords with techniques, and for each, add a temporary single note with those techniques at the same position
		//Rocksmith will render the single note on top of the chord box so that the player can see which techniques are indicated
		eof_determine_phrase_status(sp, track);	//Update the tremolo status of each note
		for(ctr = tp->notes; ctr > 0; ctr--)
		{	//For each note in the track, in reverse order
			if(eof_note_count_rs_lanes(sp, track, ctr - 1) > 1)
			{	//If this note will export as a chord (at least two non ghosted/muted gems)
				unsigned long target = EOF_PRO_GUITAR_NOTE_FLAG_BEND | EOF_PRO_GUITAR_NOTE_FLAG_HO | EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC | EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE | EOF_PRO_GUITAR_NOTE_FLAG_POP | EOF_PRO_GUITAR_NOTE_FLAG_SLAP | EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP | EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN | EOF_NOTE_FLAG_IS_TREMOLO;	//A list of all statuses to try to notate for chords
				unsigned long bitmask;
				EOF_PRO_GUITAR_NOTE *new_note;

				if(tp->note[ctr - 1]->flags & target)
				{	//If this note has any of the statuses that can be displayed in Rocksmith for single notes
					for(ctr2 = 0, bitmask = 1; ctr2 < 6; ctr2++, bitmask <<= 1)
					{	//For each of the six supported strings
						if(tp->note[ctr - 1]->note & bitmask)
						{	//If this string is used
							new_note = eof_track_add_create_note(sp, track, bitmask, tp->note[ctr - 1]->pos, tp->note[ctr - 1]->length, tp->note[ctr - 1]->type, NULL);	//Initialize a new single note at this position
							if(new_note)
							{	//If the new note was created
								new_note->flags = tp->note[ctr - 1]->flags;					//Clone the flags
								new_note->flags |= EOF_NOTE_FLAG_TEMP;						//Mark the note as temporary
								new_note->bendstrength = tp->note[ctr - 1]->bendstrength;	//Copy the bend strength
								new_note->slideend = tp->note[ctr - 1]->slideend;			//And the slide end position
								new_note->frets[ctr2] = tp->note[ctr - 1]->frets[ctr2];		//And this string's fret value
							}
							break;
						}
					}
				}
			}
		}
		eof_track_sort_notes(sp, track);	//Re-sort the notes
	}//If the user opted to export chord techniques to the Rocksmith XML files

	//Write chord templates
	chordlistsize = eof_build_chord_list(sp, track, &chordlist);	//Build a list of all unique chords in the track
	if(!chordlistsize)
	{	//If there were no chords, write an empty chord template tag
		char *filler_template_6_string = "   <chordTemplate chordName=\"\" finger0=\"1\" finger1=\"-1\" finger2=\"-1\" finger3=\"1\" finger4=\"-1\" finger5=\"-1\" fret0=\"14\" fret1=\"-1\" fret2=\"-1\" fret3=\"14\" fret4=\"-1\" fret5=\"-1\"/>\n";
		char *filler_template_5_string = "   <chordTemplate chordName=\"\" finger0=\"1\" finger1=\"-1\" finger2=\"-1\" finger3=\"1\" finger4=\"-1\" fret0=\"14\" fret1=\"-1\" fret2=\"-1\" fret3=\"14\" fret4=\"-1\"/>\n";
		char *filler_template_4_string = "   <chordTemplate chordName=\"\" finger0=\"1\" finger1=\"-1\" finger2=\"-1\" finger3=\"1\" fret0=\"14\" fret1=\"-1\" fret2=\"-1\" fret3=\"14\"/>\n";
///The Rocksmith toolkit (v1.01) has a bug where a song package cannot be created if any arrangements have no chord templates, so write at least one
//		(void) pack_fputs("  <chordTemplates count=\"0\"/>\n", fp);
		(void) pack_fputs("  <chordTemplates count=\"1\">\n", fp);
		switch(tp->numstrings)
		{	//The chord template written should match the number of strings used in the track
			case 4:
				(void) pack_fputs(filler_template_4_string, fp);
			break;
			case 5:
				(void) pack_fputs(filler_template_5_string, fp);
			break;
			default:
				(void) pack_fputs(filler_template_6_string, fp);
			break;
		}
		(void) pack_fputs("  </chordTemplates>\n", fp);
	}
	else
	{	//There were chords
		long fret0, fret1, fret2, fret3, fret4, fret5;	//Will store the fret number played on each string (-1 means the string is not played)
		long *fret[6] = {&fret0, &fret1, &fret2, &fret3, &fret4, &fret5};	//Allow the fret numbers to be accessed via array
		char *fingerunused = "-1";
		char *finger0, *finger1, *finger2, *finger3, *finger4, *finger5;	//Each will be set to either fingerunknown or fingerunused
		char **finger[6] = {&finger0, &finger1, &finger2, &finger3, &finger4, &finger5};	//Allow the finger strings to be accessed via array
		char finger0def[2] = "0", finger1def[2] = "1", finger2def[2] = "2", finger3def[2] = "3", finger4def[2] = "4", finger5def[2] = "5";	//Static strings for building manually-defined finger information
		char *fingerdef[6] = {finger0def, finger1def, finger2def, finger3def, finger4def, finger5def};	//Allow the fingerdef strings to be accessed via array
		unsigned long bitmask;

		(void) snprintf(buffer, sizeof(buffer) - 1, "  <chordTemplates count=\"%lu\">\n", chordlistsize);
		(void) pack_fputs(buffer, fp);
		for(ctr = 0; ctr < chordlistsize; ctr++)
		{	//For each of the entries in the unique chord list
			notename[0] = '\0';	//Empty the note name string
			(void) eof_build_note_name(sp, track, chordlist[ctr], notename);	//Build the note name (if it exists) into notename[]

			for(ctr2 = 0, bitmask = 1; ctr2 < 6; ctr2++, bitmask <<= 1)
			{	//For each of the 6 supported strings
				if((eof_get_note_note(sp, track, chordlist[ctr]) & bitmask) && (ctr2 < tp->numstrings) && ((tp->note[chordlist[ctr]]->frets[ctr2] & 0x80) == 0) && !(tp->note[chordlist[ctr]]->ghost & bitmask))
				{	//If the chord entry uses this string (verifying that the string number is supported by the track) and the string is not fret hand muted or ghosted
					*(fret[ctr2]) = tp->note[chordlist[ctr]]->frets[ctr2] & 0x7F;	//Retrieve the fret played on this string (masking out the muting bit)
					if(tp->note[chordlist[ctr]]->finger[ctr2])
					{	//If the fingering for this string is defined
						char *temp = fingerdef[ctr2];	//Simplify logic below
						temp[0] = '0' + tp->note[chordlist[ctr]]->finger[ctr2];	//Convert decimal to ASCII
						temp[1] = '\0';	//Truncate string
						*(finger[ctr2]) = temp;
					}
					else
					{	//The fingering is not defined, regardless of whether the string is open or fretted
						*(finger[ctr2]) = fingerunused;		//Write a -1, this will allow the XML to compile even if the chord's fingering is incomplete/undefined
					}
				}
				else
				{	//The chord entry does not use this string
					*(fret[ctr2]) = -1;
					*(finger[ctr2]) = fingerunused;
				}
			}

			expand_xml_text(buffer2, sizeof(buffer2) - 1, notename, 32);	//Expand XML special characters into escaped sequences if necessary, and check against the maximum supported length of this field
			(void) snprintf(buffer, sizeof(buffer) - 1, "    <chordTemplate chordName=\"%s\" finger0=\"%s\" finger1=\"%s\" finger2=\"%s\" finger3=\"%s\" finger4=\"%s\" finger5=\"%s\" fret0=\"%ld\" fret1=\"%ld\" fret2=\"%ld\" fret3=\"%ld\" fret4=\"%ld\" fret5=\"%ld\"/>\n", buffer2, finger0, finger1, finger2, finger3, finger4, finger5, fret0, fret1, fret2, fret3, fret4, fret5);
			(void) pack_fputs(buffer, fp);
		}//For each of the entries in the unique chord list
		(void) pack_fputs("  </chordTemplates>\n", fp);
	}//There were chords

	//Write some unknown information
	(void) pack_fputs("  <fretHandMuteTemplates count=\"0\"/>\n", fp);

	//Write the beat timings
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <ebeats count=\"%lu\">\n", sp->beats);
	(void) pack_fputs(buffer, fp);
	for(ctr = 0; ctr < sp->beats; ctr++)
	{	//For each beat in the chart
		if(eof_get_ts(sp,&beatspermeasure,NULL,ctr) == 1)
		{	//If this beat has a time signature change
			beatcounter = 0;
		}
		if(!beatcounter)
		{	//If this is the first beat in a measure
			measurenum++;
			displayedmeasure = measurenum;
		}
		else
		{	//Otherwise the measure is displayed as -1 to indicate no change from the previous beat's measure number
			displayedmeasure = -1;
		}
		(void) snprintf(buffer, sizeof(buffer) - 1, "    <ebeat time=\"%.3f\" measure=\"%ld\"/>\n", sp->beat[ctr]->fpos / 1000.0, displayedmeasure);
		(void) pack_fputs(buffer, fp);
		beatcounter++;
		if(beatcounter >= beatspermeasure)
		{
			beatcounter = 0;
		}
	}
	(void) pack_fputs("  </ebeats>\n", fp);

	//Write message boxes for the loading text song property (if defined) and each user defined popup message
	if(sp->tags->loading_text[0] != '\0')
	{	//If the loading text is defined
		char expanded_text[512];	//A string to expand the user defined text into
		(void) strftime(expanded_text, sizeof(expanded_text), sp->tags->loading_text, caltime);	//Expand any user defined calendar date/time tokens
		expand_xml_text(buffer2, sizeof(buffer2) - 1, expanded_text, 512);	//Expand XML special characters into escaped sequences if necessary, and check against the maximum supported length of this field

		(void) eof_track_add_section(eof_song, track, EOF_RS_POPUP_MESSAGE, 0, 5100, 10100, 1, sp->tags->loading_text);	//Insert this as a popup message, setting the flag to nonzero to mark is as temporary
		eof_track_pro_guitar_sort_popup_messages(tp);	//Sort the popup messages
	}
	if(tp->popupmessages || tp->tonechanges)
	{	//If at least one popup message or tone change is to be written
		unsigned long count, controlctr = 0;
		size_t stringlen;
		EOF_RS_CONTROL *controls = NULL;

		//Allocate memory for a list of control events
		count = tp->popupmessages * 2;	//Each popup message needs one control message to display and one to clear
		count += tp->tonechanges;		//Each tone change needs one control message
		(void) snprintf(buffer, sizeof(buffer) - 1, "  <controls count=\"%lu\">\n", count);
		(void) pack_fputs(buffer, fp);
		controls = malloc(sizeof(EOF_RS_CONTROL) * count);	//Allocate memory for a list of Rocksmith control events
		if(!controls)
		{
			eof_log("\tError saving:  Cannot allocate memory for control list", 1);
			return 0;	//Return failure
		}

		//Build the list of control events
		for(ctr = 0; ctr < tp->popupmessages; ctr++)
		{	//For each popup message
			//Add the popup message display control to the list
			expand_xml_text(buffer2, sizeof(buffer2) - 1, tp->popupmessage[ctr].name, EOF_SECTION_NAME_LENGTH);	//Expand XML special characters into escaped sequences if necessary, and check against the maximum supported length of this field
			stringlen = (size_t)snprintf(NULL, 0, "    <control time=\"%.3f\" code=\"ShowMessageBox(hint%lu, %s)\"/>\n", tp->popupmessage[ctr].start_pos / 1000.0, ctr + 1, buffer2) + 1;	//Find the number of characters needed to store this string
			controls[controlctr].str = malloc(stringlen + 1);	//Allocate memory to build the string
			if(!controls[controlctr].str)
			{
				eof_log("\tError saving:  Cannot allocate memory for control event", 1);
				while(controlctr > 0)
				{	//Free previously allocated strings
					free(controls[controlctr - 1].str);
					controlctr--;
				}
				free(controls);
				return 0;	//Return failure
			}
			(void) snprintf(controls[controlctr].str, stringlen, "    <control time=\"%.3f\" code=\"ShowMessageBox(hint%lu, %s)\"/>\n", tp->popupmessage[ctr].start_pos / 1000.0, ctr + 1, buffer2);
			controls[controlctr].pos = tp->popupmessage[ctr].start_pos;
			controlctr++;

			//Add the clear message control to the list
			stringlen = (size_t)snprintf(NULL, 0, "    <control time=\"%.3f\" code=\"ClearAllMessageBoxes()\"/>\n", tp->popupmessage[ctr].end_pos / 1000.0) + 1;	//Find the number of characters needed to store this string
			controls[controlctr].str = malloc(stringlen + 1);	//Allocate memory to build the string
			if(!controls[controlctr].str)
			{
				eof_log("\tError saving:  Cannot allocate memory for control event", 1);
				while(controlctr > 0)
				{	//Free previously allocated strings
					free(controls[controlctr - 1].str);
					controlctr--;
				}
				free(controls);
				return 0;	//Return failure
			}
			(void) snprintf(controls[controlctr].str, stringlen, "    <control time=\"%.3f\" code=\"ClearAllMessageBoxes()\"/>\n", tp->popupmessage[ctr].end_pos / 1000.0);
			controls[controlctr].pos = tp->popupmessage[ctr].end_pos;
			controlctr++;
		}
		for(ctr = 0; ctr < tp->tonechanges; ctr++)
		{	//For each tone change
			//Add the tone change control to the list
			char tempname[EOF_SECTION_NAME_LENGTH+1];
			stringlen = (size_t)snprintf(NULL, 0, "    <control time=\"%.3f\" code=\"CDlcTone(%s)\"/>\n", tp->tonechange[ctr].start_pos / 1000.0, tp->tonechange[ctr].name) + 1;	//Find the number of characters needed to store this string
			controls[controlctr].str = malloc(stringlen + 1);	//Allocate memory to build the string
			if(!controls[controlctr].str)
			{
				eof_log("\tError saving:  Cannot allocate memory for control event", 1);
				while(controlctr > 0)
				{	//Free previously allocated strings
					free(controls[controlctr - 1].str);
					controlctr--;
				}
				free(controls);
				return 0;	//Return failure
			}
			///Until the Rocksmith toolkit exposes proper tone key names, convert spaces to underscores, which is one known change that the toolkit makes to the
			/// display name in order to derive a key name.  Build another copy of the string to do this
			for(ctr2 = 0; ctr2 < (unsigned long)strlen(tp->tonechange[ctr].name); ctr2++)
			{	//For each character in the tone name
				if(tp->tonechange[ctr].name[ctr2] == ' ')
				{	//If it's a space character
					tempname[ctr2] = '_';	//Copy it to the new string as an underscore instead
				}
				else
				{	//Otherwise copy it as-is
					tempname[ctr2] = tp->tonechange[ctr].name[ctr2];
				}
			}
			tempname[ctr2] = '\0';	//Terminate the string
			(void) snprintf(controls[controlctr].str, stringlen, "    <control time=\"%.3f\" code=\"CDlcTone(%s)\"/>\n", tp->tonechange[ctr].start_pos / 1000.0, tempname);
			controls[controlctr].pos = tp->tonechange[ctr].start_pos;
			controlctr++;
		}

		//Sort, write and free the list of control events
		qsort(controls, (size_t)count, sizeof(EOF_RS_CONTROL), eof_song_qsort_control_events);
		for(ctr = 0; ctr < count; ctr++)
		{	//For each control event
			(void) pack_fputs(controls[ctr].str, fp);	//Write the control event string
			free(controls[ctr].str);	//Free the string
		}
		free(controls);	//Free the array

		(void) pack_fputs("  </controls>\n", fp);

		//Remove any loading text popup that was inserted into the track
		for(ctr = 0; ctr < tp->popupmessages; ctr++)
		{	//For each popup message
			if(tp->popupmessage[ctr].flags)
			{	//If the flags field was made nonzero
				eof_track_pro_guitar_delete_popup_message(tp, ctr);	//Delete this temporary popup message
				break;
			}
		}
	}//If at least one popup message or tone change is to be written

	//Write sections
	for(ctr = 0, numsections = 0; ctr < sp->beats; ctr++)
	{	//For each beat in the chart
		if(sp->beat[ctr]->contained_rs_section_event >= 0)
		{	//If this beat has a Rocksmith section
			numsections++;	//Update Rocksmith section instance counter
		}
	}
	if(numsections)
	{	//If there is at least one Rocksmith section defined in the chart (which should be the case since default ones were inserted earlier if there weren't any)
		(void) snprintf(buffer, sizeof(buffer) - 1, "  <sections count=\"%lu\">\n", numsections);
		(void) pack_fputs(buffer, fp);
		for(ctr = 0; ctr < sp->beats; ctr++)
		{	//For each beat in the chart
			if(sp->beat[ctr]->contained_rs_section_event >= 0)
			{	//If this beat has a Rocksmith section
				expand_xml_text(buffer2, sizeof(buffer2) - 1, sp->text_event[sp->beat[ctr]->contained_rs_section_event]->text, 32);	//Expand XML special characters into escaped sequences if necessary, and check against the maximum supported length of this field
				(void) snprintf(buffer, sizeof(buffer) - 1, "    <section name=\"%s\" number=\"%d\" startTime=\"%.3f\"/>\n", buffer2, sp->beat[ctr]->contained_rs_section_event_instance_number, sp->beat[ctr]->fpos / 1000.0);
				(void) pack_fputs(buffer, fp);
			}
		}
		(void) pack_fputs("  </sections>\n", fp);
	}
	else
	{
		allegro_message("Error:  Default RS sections that were added are missing.  Skipping writing the <sections> tag.");
	}

	//Write events
	for(ctr = 0, numevents = 0; ctr < sp->text_events; ctr++)
	{	//For each event in the chart
		if(sp->text_event[ctr]->flags & EOF_EVENT_FLAG_RS_EVENT)
		{	//If the event is marked as a Rocksmith event
			if(!sp->text_event[ctr]->track || (sp->text_event[ctr]->track  == track))
			{	//If the event applies to the specified track
				numevents++;
			}
		}
	}
	if(numevents)
	{	//If there is at least one Rocksmith event defined in the chart
		(void) snprintf(buffer, sizeof(buffer) - 1, "  <events count=\"%lu\">\n", numevents);
		(void) pack_fputs(buffer, fp);
		for(ctr = 0, numevents = 0; ctr < sp->text_events; ctr++)
		{	//For each event in the chart
			if(sp->text_event[ctr]->flags & EOF_EVENT_FLAG_RS_EVENT)
			{	//If the event is marked as a Rocksmith event
				if(!sp->text_event[ctr]->track || (sp->text_event[ctr]->track  == track))
				{	//If the event applies to the specified track
					expand_xml_text(buffer2, sizeof(buffer2) - 1, sp->text_event[ctr]->text, 256);	//Expand XML special characters into escaped sequences if necessary, and check against the maximum supported length of this field
					(void) snprintf(buffer, sizeof(buffer) - 1, "    <event time=\"%.3f\" code=\"%s\"/>\n", sp->beat[sp->text_event[ctr]->beat]->fpos / 1000.0, buffer2);
					(void) pack_fputs(buffer, fp);
				}
			}
		}
		(void) pack_fputs("  </events>\n", fp);
	}
	else
	{	//Otherwise write an empty events tag
		(void) pack_fputs("  <events count=\"0\"/>\n", fp);
	}

	//Write note difficulties
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <levels count=\"%u\">\n", numdifficulties);
	(void) pack_fputs(buffer, fp);
	for(ctr = 0, ctr2 = 0; ctr < 256; ctr++)
	{	//For each of the possible difficulties
		if(eof_track_diff_populated_status[ctr])
		{	//If this difficulty is populated
			unsigned long anchorcount;
			char anchorsgenerated = 0;	//Tracks whether anchors were automatically generated and will need to be deleted after export

			//Count the number of single notes and chords in this difficulty
			for(ctr3 = 0, numsinglenotes = 0, numchords = 0; ctr3 < tp->notes; ctr3++)
			{	//For each note in the track
				if(eof_get_note_type(sp, track, ctr3) == ctr)
				{	//If the note is in this difficulty
					unsigned long lanecount = eof_note_count_rs_lanes(sp, track, ctr3);	//Count the number of non ghosted/muted gems for this note
					if(lanecount == 1)
					{	//If the note has only one gem
						numsinglenotes++;	//Increment counter
					}
					else if(lanecount > 1)
					{	//If the note has multiple gems
						numchords++;	//Increment counter
					}
				}
			}

			//Write single notes
			(void) snprintf(buffer, sizeof(buffer) - 1, "    <level difficulty=\"%lu\">\n", ctr2);
			(void) pack_fputs(buffer, fp);
			ctr2++;	//Increment the populated difficulty level number
			if(numsinglenotes)
			{	//If there's at least one single note in this difficulty
				(void) snprintf(buffer, sizeof(buffer) - 1, "      <notes count=\"%lu\">\n", numsinglenotes);
				(void) pack_fputs(buffer, fp);
				for(ctr3 = 0; ctr3 < tp->notes; ctr3++)
				{	//For each note in the track
					if((eof_get_note_type(sp, track, ctr3) == ctr) && (eof_note_count_rs_lanes(sp, track, ctr3) == 1))
					{	//If this note is in this difficulty and will export as a single note (only one gem has non ghosted/muted status)
						for(stringnum = 0, bitmask = 1; stringnum < tp->numstrings; stringnum++, bitmask <<= 1)
						{	//For each string used in this track
							if((eof_get_note_note(sp, track, ctr3) & bitmask) && ((tp->note[ctr3]->frets[stringnum] & 0x80) == 0) && !(tp->note[ctr3]->ghost & bitmask))
							{	//If this string is used in this note, it is not fret hand muted and it is not ghosted
								unsigned long flags;
								unsigned long notepos;
								unsigned long bend = 0;		//The number of half steps this note bends
								long slideto = -1;			//If not negative, is the fret position the slide ends at
								unsigned long fret;			//The fret number used for this string
								char hammeron;				//Nonzero if this note is a hammer on
								char pulloff;				//Nonzero if this note is a pull off
								char harmonic;				//Nonzero if this note is a harmonic
								char hopo;					//Nonzero if this note is either a hammer on or a pull off
								char palmmute;				//Nonzero if this note is a palm mute
								unsigned long length;
								char tremolo;				//Nonzero if this note is a tremolo
								char pop;					//1 if this note is played with pop technique, else -1
								char slap;					//1 if this note is played with slap technique, else -1

								flags = eof_get_note_flags(sp, track, ctr3);
								notepos = eof_get_note_pos(sp, track, ctr3);
								length = eof_get_note_length(sp, track, ctr3);
								if((flags & EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE) == 0)
								{	//At this point, it doesn't seem Rocksmith supports string muted notes
									if((length == 1) && !(flags & EOF_PRO_GUITAR_NOTE_FLAG_BEND) && !(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP) && !(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN))
									{	//If the note is has the absolute minimum length and isn't a bend or a slide note (bend and slide notes are required to have a length > 0 or Rocksmith will crash)
										length = 0;	//Convert to a length of 0 so that it doesn't display as a sustain note in-game
									}
									fret = tp->note[ctr3]->frets[stringnum] & 0x7F;	//Get the fret value for this string (mask out the muting bit)
									if((flags & EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION) == 0)
									{	//If this note doesn't have definitions for bend strength or the ending fret for slides
										if(flags & EOF_PRO_GUITAR_NOTE_FLAG_BEND)
										{	//If this note bends
											bend = 1;	//Assume a 1 half step bend
										}
										if(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP)
										{	//If this note slides up and the user hasn't defined the ending fret of the slide
											slideto = fret + 1;	//Assume a 1 fret slide until logic is added for the author to add this information
										}
										else if(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN)
										{	//If this note slides down and the user hasn't defined the ending fret of the slide
											slideto = fret - 1;	//Assume a 1 fret slide until logic is added for the author to add this information
										}
									}
									else
									{	//This note defines the bend strength and ending fret for slides
										if(flags & EOF_PRO_GUITAR_NOTE_FLAG_BEND)
										{	//If this note bends
											bend = tp->note[ctr3]->bendstrength;	//Obtain the defined bend strength
										}
										if((flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP) || (flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN))
										{	//If this note slides
											slideto = tp->note[ctr3]->slideend;
										}
									}
									//Determine boolean note statuses
									hammeron = (flags & EOF_PRO_GUITAR_NOTE_FLAG_HO) ? 1 : 0;
									pulloff = (flags & EOF_PRO_GUITAR_NOTE_FLAG_PO) ? 1 : 0;
									harmonic = (flags & EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC) ? 1 : 0;
									hopo = (hammeron | pulloff) ? 1 : 0;
									palmmute = (flags & EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE) ? 1 : 0;
									tremolo = (flags & EOF_NOTE_FLAG_IS_TREMOLO) ? 1 : 0;
									pop = (flags & EOF_PRO_GUITAR_NOTE_FLAG_POP) ? 1 : -1;
									slap = (flags & EOF_PRO_GUITAR_NOTE_FLAG_SLAP) ? 1 : -1;
									if((pop > 0) || (slap > 0))
									{	//If the note has pop or slap notation
										length = 0;	//Remove all sustain for the note, because Rocksmith display pop/slap sustain notes as non pop/slap sustained notes
									}
									if(!eof_pro_guitar_note_lowest_fret(tp, ctr3))
									{	//If this note contains no fretted strings
										if(bend || (slideto >= 0))
										{	//If it is also marked as a bend or slide note, omit these statuses because they're invalid for open notes
											bend = 0;
											slideto = -1;
											if((*user_warned & 4) == 0)
											{	//If the user wasn't alerted that one or more open notes have these statuses improperly applied
												allegro_message("Warning:  At least one open note is marked with bend or slide status.\nThis is not supported, so these statuses are being omitted for such notes.");
												*user_warned |= 4;
											}
										}
									}
									(void) snprintf(buffer, sizeof(buffer) - 1, "        <note time=\"%.3f\" bend=\"%lu\" fret=\"%lu\" hammerOn=\"%d\" harmonic=\"%d\" hopo=\"%d\" ignore=\"0\" palmMute=\"%d\" pluck=\"%d\" pullOff=\"%d\" slap=\"%d\" slideTo=\"%ld\" string=\"%lu\" sustain=\"%.3f\" tremolo=\"%d\"/>\n", (double)notepos / 1000.0, bend, fret, hammeron, harmonic, hopo, palmmute, pop, pulloff, slap, slideto, stringnum, (double)length / 1000.0, tremolo);
									(void) pack_fputs(buffer, fp);
									break;	//Only one note entry is valid for each single note, so break from loop
								}//If the note isn't string muted
							}//If this string is used in this note
						}//For each string used in this track
					}//If this note is in this difficulty and is a single note (and not a chord)
				}//For each note in the track
				(void) pack_fputs("      </notes>\n", fp);
			}//If there's at least one single note in this difficulty
			else
			{	//There are no single notes in this difficulty, write an empty notes tag
				(void) pack_fputs("      <notes count=\"0\"/>\n", fp);
			}

			//Write chords
			if(numchords)
			{	//If there's at least one chord in this difficulty
				unsigned long chordid;
				char *upstrum = "up";
				char *downstrum = "down";
				char *direction;	//Will point to either upstrum or downstrum as appropriate
				double notepos;
				char highdensity;		//Any chord within the threshold proximity of an identical chord has the highDensity boolean property set to true

				(void) snprintf(buffer, sizeof(buffer) - 1, "      <chords count=\"%lu\">\n", numchords);
				(void) pack_fputs(buffer, fp);
				for(ctr3 = 0; ctr3 < tp->notes; ctr3++)
				{	//For each note in the track
					if((eof_get_note_type(sp, track, ctr3) == ctr) && (eof_note_count_rs_lanes(sp, track, ctr3) > 1))
					{	//If this note is in this difficulty and will export as a chord (at least two non ghosted/muted gems) that isn't fully string muted
						for(ctr4 = 0; ctr4 < chordlistsize; ctr4++)
						{	//For each of the entries in the unique chord list
							if(!eof_note_compare_simple(sp, track, ctr3, chordlist[ctr4]))
							{	//If this note matches a chord entry
								chordid = ctr4;	//Store the chord entry number
								break;
							}
						}
						if(ctr4 >= chordlistsize)
						{	//If the chord couldn't be found
							allegro_message("Error:  Couldn't match chord with chord template.  Aborting Rocksmith export.");
							eof_log("Error:  Couldn't match chord with chord template.  Aborting Rocksmith export.", 1);
							if(chordlist)
							{	//If the chord list was built
								free(chordlist);
							}
							return 0;	//Return error
						}
						if(tp->note[ctr3]->flags & EOF_PRO_GUITAR_NOTE_FLAG_UP_STRUM)
						{	//If this note explicitly strums up
							direction = upstrum;	//Set the direction string to match
						}
						else
						{	//Otherwise the direction defaults to down
							direction = downstrum;
						}
						highdensity = eof_note_has_high_chord_density(sp, track, ctr3);	//Determine whether the chord will export with high density
						notepos = (double)tp->note[ctr3]->pos / 1000.0;
						(void) snprintf(buffer, sizeof(buffer) - 1, "        <chord time=\"%.3f\" chordId=\"%lu\" highDensity=\"%d\" ignore=\"0\" strum=\"%s\"/>\n", notepos, chordid, highdensity, direction);
						(void) pack_fputs(buffer, fp);
					}//If this note is in this difficulty and is a chord
				}//For each note in the track
				(void) pack_fputs("      </chords>\n", fp);
			}
			else
			{	//There are no chords in this difficulty, write an empty chords tag
				(void) pack_fputs("      <chords count=\"0\"/>\n", fp);
			}

			//Write other stuff
			(void) pack_fputs("      <fretHandMutes count=\"0\"/>\n", fp);

			//Write anchors (fret hand positions)
			for(ctr3 = 0, anchorcount = 0; ctr3 < tp->handpositions; ctr3++)
			{	//For each hand position defined in the track
				if(tp->handposition[ctr3].difficulty == ctr)
				{	//If the hand position is in this difficulty
					anchorcount++;
				}
			}
			if(!anchorcount)
			{	//If there are no anchors in this track difficulty, automatically generate them
				if((*user_warned & 1) == 0)
				{	//If the user wasn't alerted that one or more track difficulties have no fret hand positions defined
					allegro_message("Warning:  At least one track difficulty has no fret hand positions defined.  They will be created automatically.");
					*user_warned |= 1;
				}
				eof_fret_hand_position_list_dialog_undo_made = 1;	//Ensure no undo state is written during export
				eof_generate_efficient_hand_positions(sp, track, ctr, 0, 0);	//Generate the fret hand positions for the track difficulty being currently written (use a static fret range tolerance of 4 for all frets)
				anchorsgenerated = 1;
			}
			for(ctr3 = 0, anchorcount = 0; ctr3 < tp->handpositions; ctr3++)	//Re-count the hand positions
			{	//For each hand position defined in the track
				if(tp->handposition[ctr3].difficulty == ctr)
				{	//If the hand position is in this difficulty
					anchorcount++;
				}
			}
			if(anchorcount)
			{	//If there's at least one anchor in this difficulty
				(void) snprintf(buffer, sizeof(buffer) - 1, "      <anchors count=\"%lu\">\n", anchorcount);
				(void) pack_fputs(buffer, fp);
				for(ctr3 = 0, anchorcount = 0; ctr3 < tp->handpositions; ctr3++)
				{	//For each hand position defined in the track
					if(tp->handposition[ctr3].difficulty == ctr)
					{	//If the hand position is in this difficulty
						(void) snprintf(buffer, sizeof(buffer) - 1, "        <anchor time=\"%.3f\" fret=\"%lu\"/>\n", (double)tp->handposition[ctr3].start_pos / 1000.0, tp->handposition[ctr3].end_pos);
						(void) pack_fputs(buffer, fp);
					}
				}
				(void) pack_fputs("      </anchors>\n", fp);
			}
			else
			{	//There are no anchors in this difficulty, write an empty anchors tag
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Error:  Failed to automatically generate fret hand positions for level %lu of\n\"%s\" during MIDI export.", ctr2, fn);
				eof_log(eof_log_string, 1);
				allegro_message(eof_log_string);
				(void) pack_fputs("      <anchors count=\"0\"/>\n", fp);
			}
			if(anchorsgenerated)
			{	//If anchors were automatically generated for this track difficulty, remove them now
				for(ctr3 = tp->handpositions; ctr3 > 0; ctr3--)
				{	//For each hand position defined in the track, in reverse order
					if(tp->handposition[ctr3 - 1].difficulty == ctr)
					{	//If the hand position is in this difficulty
						eof_pro_guitar_track_delete_hand_position(tp, ctr3 - 1);	//Delete the hand position
					}
				}
			}

			//Write hand shapes
			//Count the number of hand shapes to write
			handshapectr = 0;
			for(ctr3 = 0; ctr3 < tp->notes; ctr3++)
			{	//For each note in the track
				if((eof_get_note_type(sp, track, ctr3) == ctr) && (eof_note_count_colors(sp, track, ctr3) > 1) && !eof_is_string_muted(sp, track, ctr3))
				{	//If this note is in this difficulty and is a chord that isn't fully string muted
					unsigned long chord = ctr3;	//Store a copy of this note number because ctr3 will be manipulated below

					//Find this chord's ID
					for(ctr4 = 0; ctr4 < chordlistsize; ctr4++)
					{	//For each of the entries in the unique chord list
						if(!eof_note_compare_simple(sp, track, ctr3, chordlist[ctr4]))
						{	//If this note matches a chord entry
							chordid = ctr4;	//Store the chord entry number
							break;
						}
					}
					if(ctr4 >= chordlistsize)
					{	//If the chord couldn't be found
						allegro_message("Error:  Couldn't match chord with chord template.  Aborting Rocksmith export.");
						eof_log("Error:  Couldn't match chord with chord template.  Aborting Rocksmith export.", 1);
						if(chordlist)
						{	//If the chord list was built
							free(chordlist);
						}
						return 0;	//Return error
					}
					handshapestart = eof_get_note_pos(sp, track, ctr3);	//Store this chord's start position

					//If this chord is at the beginning of an arpeggio phrase, skip the rest of the notes in that phrase
					for(ctr5 = 0; ctr5 < tp->arpeggios; ctr5++)
					{	//For each arpeggio phrase in the track
						if(((tp->note[ctr3]->pos + 10 >= tp->arpeggio[ctr5].start_pos) && (tp->note[ctr3]->pos <= tp->arpeggio[ctr5].start_pos + 10)) && (tp->note[ctr3]->type == tp->arpeggio[ctr5].difficulty))
						{	//If this chord's start position is within 10ms of an arpeggio phrase in this track difficulty
							while(1)
							{
								nextnote = eof_fixup_next_note(sp, track, ctr3);
								if((nextnote >= 0) && (tp->note[nextnote]->pos <= tp->arpeggio[ctr5].end_pos))
								{	//If there is another note and it is in the same arpeggio phrase
									ctr3 = nextnote;	//Iterate to that note, and check subsequent notes to see if they are also in the phrase
								}
								else
								{	//The next note (if any) is not in the arpeggio phrase
									break;	//Break from while loop
								}
							}
							break;	//Break from for loop
						}
					}

					//Examine subsequent notes to see if they match this chord
					while(1)
					{
						nextnote = eof_fixup_next_note(sp, track, ctr3);
						if((nextnote >= 0) && !eof_note_compare_simple(sp, track, chord, nextnote))
						{	//If there is another note and it matches this chord
							ctr3 = nextnote;	//Iterate to that note, and check subsequent notes to see if they match
						}
						else
						{	//The next note (if any) is not a repeat of this note
							handshapeend = eof_get_note_pos(sp, track, ctr3) + eof_get_note_length(sp, track, ctr3);	//End the hand shape at the end of this chord
							break;	//Break from while loop
						}
					}

					handshapectr++;	//One more hand shape has been counted
				}//If this note is in this difficulty and is a chord that isn't fully string muted
			}//For each note in the track

			if(handshapectr)
			{	//If there was at least one hand shape to write
				//Write the hand shapes
				(void) snprintf(buffer, sizeof(buffer) - 1, "      <handShapes count=\"%lu\">\n", handshapectr);
				(void) pack_fputs(buffer, fp);
				for(ctr3 = 0; ctr3 < tp->notes; ctr3++)
				{	//For each note in the track
					if((eof_get_note_type(sp, track, ctr3) == ctr) && (eof_note_count_colors(sp, track, ctr3) > 1) && !eof_is_string_muted(sp, track, ctr3))
					{	//If this note is in this difficulty and is a chord that isn't fully string muted
						unsigned long chord = ctr3;	//Store a copy of this note number because ctr3 will be manipulated below

						//Find this chord's ID
						for(ctr4 = 0; ctr4 < chordlistsize; ctr4++)
						{	//For each of the entries in the unique chord list
							if(!eof_note_compare_simple(sp, track, ctr3, chordlist[ctr4]))
							{	//If this note matches a chord entry
								chordid = ctr4;	//Store the chord entry number
								break;
							}
						}
						if(ctr4 >= chordlistsize)
						{	//If the chord couldn't be found
							allegro_message("Error:  Couldn't match chord with chord template.  Aborting Rocksmith export.");
							eof_log("Error:  Couldn't match chord with chord template.  Aborting Rocksmith export.", 1);
							if(chordlist)
							{	//If the chord list was built
								free(chordlist);
							}
							return 0;	//Return error
						}
						handshapestart = eof_get_note_pos(sp, track, ctr3);	//Store this chord's start position (in seconds)

						//If this chord is at the beginning of an arpeggio phrase, skip the rest of the notes in that phrase
						for(ctr5 = 0; ctr5 < tp->arpeggios; ctr5++)
						{	//For each arpeggio phrase in the track
							if(((tp->note[ctr3]->pos + 10 >= tp->arpeggio[ctr5].start_pos) && (tp->note[ctr3]->pos <= tp->arpeggio[ctr5].start_pos + 10)) && (tp->note[ctr3]->type == tp->arpeggio[ctr5].difficulty))
							{	//If this chord's start position is within 10ms of an arpeggio phrase in this track difficulty
								while(1)
								{
									nextnote = eof_fixup_next_note(sp, track, ctr3);
									if((nextnote >= 0) && (tp->note[nextnote]->pos <= tp->arpeggio[ctr5].end_pos))
									{	//If there is another note and it is in the same arpeggio phrase
										ctr3 = nextnote;	//Iterate to that note, and check subsequent notes to see if they are also in the phrase
									}
									else
									{	//The next note (if any) is not in the arpeggio phrase
										break;	//Break from while loop
									}
								}
								break;	//Break from for loop
							}
						}

						//Examine subsequent notes to see if they match this chord
						while(1)
						{
							nextnote = eof_fixup_next_note(sp, track, ctr3);
							if((nextnote >= 0) && !eof_note_compare_simple(sp, track, chord, nextnote))
							{	//If there is another note and it matches this chord
								ctr3 = nextnote;	//Iterate to that note, and check subsequent notes to see if they match
							}
							else
							{	//The next note (if any) is not a repeat of this note
								handshapeend = eof_get_note_pos(sp, track, ctr3) + eof_get_note_length(sp, track, ctr3);	//End the hand shape at the end of this chord

								if((handshapeend - handshapestart < 56) && (handshapestart + 56 < eof_get_note_pos(sp, track, nextnote)))
								{	//If the hand shape would be shorter than 56ms, and the next note is further than 56ms away
									handshapeend = eof_get_note_pos(sp, track, ctr3) + 56;	//Pad the hand shape to 56ms
								}
								break;	//Break from while loop
							}
						}

						//Write this hand shape
						(void) snprintf(buffer, sizeof(buffer) - 1, "        <handShape chordId=\"%lu\" endTime=\"%.3f\" startTime=\"%.3f\"/>\n", chordid, (double)handshapeend / 1000.0, (double)handshapestart / 1000.0);
						(void) pack_fputs(buffer, fp);
					}//If this note is in this difficulty and is a chord that isn't fully string muted
				}//For each note in the track
				(void) pack_fputs("      </handShapes>\n", fp);
			}
			else
			{	//There are no chords in this difficulty, write an empty hand shape tag
				(void) pack_fputs("      <handShapes count=\"0\"/>\n", fp);
			}

			//Write closing level tag
			(void) pack_fputs("    </level>\n", fp);
		}//If this difficulty is populated
	}//For each of the available difficulties
	(void) pack_fputs("  </levels>\n", fp);
	(void) pack_fputs("</song>\n", fp);
	(void) pack_fclose(fp);

	//Cleanup
	if(chordlist)
	{	//If the chord list was built
		free(chordlist);
	}
	//Remove all temporary text events that were added
	for(ctr = sp->text_events; ctr > 0; ctr--)
	{	//For each text event (in reverse order)
		if(sp->text_event[ctr - 1]->is_temporary)
		{	//If this text event has been marked as temporary
			eof_song_delete_text_event(sp, ctr - 1);	//Delete it
		}
	}
	eof_sort_events(sp);	//Re-sort events
	//Remove all temporary notes that were added
	for(ctr = tp->notes; ctr > 0; ctr--)
	{	//For each note in the track, in reverse order
		if(tp->note[ctr - 1]->flags & EOF_NOTE_FLAG_TEMP)
		{	//If this is a temporary note that was added for chord technique notation
			eof_track_delete_note(sp, track, ctr - 1);	//Delete it
		}
	}
	eof_track_sort_notes(sp, track);	//Re-sort the notes

	return 1;	//Return success
}

void eof_pro_guitar_track_fix_fingerings(EOF_PRO_GUITAR_TRACK *tp, char *undo_made)
{
	unsigned long ctr2, ctr3;
	unsigned char *array;	//Points to the finger array being replicated to matching notes
	int retval;

	if(!tp)
		return;	//Invalid parameter

	for(ctr2 = 0; ctr2 < tp->notes; ctr2++)
	{	//For each note in the track (outer loop)
		retval = eof_pro_guitar_note_fingering_valid(tp, ctr2);
		if(retval == 1)
		{	//If the note's fingering was complete
			array = tp->note[ctr2]->finger;
			for(ctr3 = 0; ctr3 < tp->notes; ctr3++)
			{	//For each note in the track (inner loop)
				if((ctr2 != ctr3) && (eof_pro_guitar_note_compare(tp, ctr2, tp, ctr3) == 0))
				{	//If this note matches the note being examined in the outer loop, and we're not comparing the note to itself
					if(eof_pro_guitar_note_fingering_valid(tp, ctr3) != 1)
					{	//If the fingering of the inner loop's note is invalid/undefined
						if(undo_made && !(*undo_made))
						{	//If an undo hasn't been made yet
							eof_prepare_undo(EOF_UNDO_TYPE_NONE);
							*undo_made = 1;
						}
						memcpy(tp->note[ctr3]->finger, array, 8);	//Overwrite it with the current finger array
					}
					else
					{	//The inner loop's note has a valid fingering array defined
						array = tp->note[ctr3]->finger;	//Use this finger array for remaining matching notes in the track
					}
				}
			}//For each note in the track (inner loop)
		}
		else if(retval == 0)
		{	//If the note's fingering was defined, but invalid
			if(undo_made && !(*undo_made))
			{	//If an undo hasn't been made yet
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				*undo_made = 1;
			}
			memset(tp->note[ctr2], 0, 8);	//Clear it
		}
	}
}

int eof_pro_guitar_note_fingering_valid(EOF_PRO_GUITAR_TRACK *tp, unsigned long note)
{
	unsigned long ctr, bitmask;
	char string_finger_defined = 0, string_finger_undefined = 0, all_strings_open = 1;

	if(!tp || (note >= tp->notes))
		return 0;	//Invalid parameters

	for(ctr = 0, bitmask = 1; ctr < tp->numstrings; ctr++, bitmask <<= 1)
	{	//For each string supported by this track
		if(tp->note[note]->note & bitmask)
		{	//If this string is used
			if((tp->note[note]->frets[ctr] & 0x80) == 0)
			{	//If the string isn't muted
				if(tp->note[note]->frets[ctr] != 0)
				{	//If the string isn't being played open, it requires a fingering
					all_strings_open = 0;	//Track that the note used at least one fretted string
					if(tp->note[note]->finger[ctr] != 0)
					{	//If this string has a finger definition
						string_finger_defined = 1;	//Track that a string was defined
					}
					else
					{	//This string does not have a finger definition
						string_finger_undefined = 1;	//Track that a string was undefined
					}
				}
				else
				{	//If the string is being played open, it must not have a finger defined
					if(tp->note[note]->finger[ctr] != 0)
					{	//If this string has a finger definition
						string_finger_defined = string_finger_undefined = 1;	//Set an error condition
						break;
					}
				}
			}
		}
	}

	if(all_strings_open && !string_finger_defined)
	{	//If the note only had open strings played, and no fingering was defined, this is valid
		return 1;	//Return fingering valid
	}
	if(string_finger_defined && string_finger_undefined)
	{	//If a note only had partial finger definition
		return 0;	//Return fingering invalid
	}
	if(string_finger_defined)
	{	//If the finger definition was complete
		return 1;	//Return fingering valid
	}
	return 2;	//Return fingering undefined
}

void eof_song_fix_fingerings(EOF_SONG *sp, char *undo_made)
{
	unsigned long ctr;

	if(!sp)
		return;	//Invalid parameter

	for(ctr = 1; ctr < sp->tracks; ctr++)
	{	//For each track (skipping the NULL global track 0)
		if(sp->track[ctr]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If this is a pro guitar track
			eof_pro_guitar_track_fix_fingerings(sp->pro_guitar_track[sp->track[ctr]->tracknum], undo_made);	//Correct and complete note fingering where possible, performing an undo state before making changes
		}
	}
}

void eof_generate_efficient_hand_positions(EOF_SONG *sp, unsigned long track, char difficulty, char warnuser, char dynamic)
{
	unsigned long ctr, ctr2, tracknum, count, bitmask, beatctr, startpos = 0, endpos;
	EOF_PRO_GUITAR_TRACK *tp;
	unsigned char current_low, current_high, last_anchor = 0;
	EOF_PRO_GUITAR_NOTE *current_note = NULL;	//Tracks the first note in the set of notes having its hand position found
	char force_change, started = 0;

	if(!sp || (track >= sp->tracks) || (sp->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return;	//Invalid parameters

	//Remove any existing fret hand positions defined for this track difficulty
	tracknum = sp->track[track]->tracknum;
	tp = sp->pro_guitar_track[tracknum];
	if(tp->notes == 0)
		return;	//Invalid parameters (track must have at least 1 note)
	for(ctr = tp->handpositions; ctr > 0; ctr--)
	{	//For each existing hand positions in this track (in reverse order)
		if(tp->handposition[ctr - 1].difficulty == difficulty)
		{	//If this hand position is defined for the specified difficulty
			if(warnuser)
			{
				eof_clear_input();
				key[KEY_Y] = 0;
				key[KEY_N] = 0;
				if(alert(NULL, "Existing fret hand positions for the active track difficulty will be removed.", "Continue?", "&Yes", "&No", 'y', 'n') != 1)
				{	//If the user does not opt to remove the existing hand positions
					return;
				}
			}
			warnuser = 0;
			if(!eof_fret_hand_position_list_dialog_undo_made)
			{	//If an undo state hasn't been made yet since launching this dialog
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				eof_fret_hand_position_list_dialog_undo_made = 1;
			}
			eof_pro_guitar_track_delete_hand_position(tp, ctr - 1);	//Delete the hand position
		}
	}
	eof_pro_guitar_track_sort_fret_hand_positions(tp);	//Sort the positions

	//Count the number of notes in the specified track difficulty and allocate arrays large enough to store the lowest and highest fret number used in each
	for(ctr = 0, count = 0; ctr < tp->notes; ctr++)
	{	//For each note in the track
		if(tp->note[ctr]->type == difficulty)
		{	//If it is in the specified difficulty
			count++;	//Increment this counter
		}
	}

	if(!count)
	{	//If this track difficulty has no notes
		return;	//Exit function
	}

	eof_build_fret_range_tolerances(tp, difficulty, dynamic);	//Allocate and build eof_fret_range_tolerances[], using the calling function's chosen option regarding tolerances
	if(!eof_fret_range_tolerances)
	{	//eof_fret_range_tolerances[] wasn't built
		return;
	}
	if(!eof_fret_hand_position_list_dialog_undo_made)
	{	//If an undo hasn't been made yet, do it now as there will be at least one fret hand position added
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
	}

	//Iterate through this track difficulty's notes and determine efficient hand positions
	current_low = current_high = 0;	//Reset these at the start of generating hand positions
	for(ctr = 0; ctr < tp->notes; ctr++)
	{	//For each note in the track
		if((tp->note[ctr]->type == difficulty) && !(tp->note[ctr]->flags & EOF_NOTE_FLAG_TEMP))
		{	//If it is in the specified difficulty and isn't marked as a temporary note (a single note inserted to allow chord techniques to appear in Rocksmith)
			if(!current_note)
			{	//If this is the first note since the last hand position
				current_note = tp->note[ctr];	//Store its address
			}
			force_change = 0;	//Reset this condition
			for(ctr2 = 0, bitmask = 1; ctr2 < 6; ctr2++, bitmask <<= 1)
			{	//For each of the 6 supported strings
				if((tp->note[ctr]->note & bitmask) && (tp->note[ctr]->finger[ctr2] == 1))
				{	//If this note uses this string, and the string is defined as being fretted by the index finger
					if(eof_pro_guitar_note_lowest_fret(tp, ctr) != last_anchor)
					{	//If this note's lowest fret would indicate a change in hand position
						if(current_note == tp->note[ctr])
						{	//If this is a forced position changed following another forced change
							current_low = 0;	//The logic below will set the position to reflect this change
						}
						force_change = 1;
						break;
					}
				}
			}
			if(force_change || !eof_note_can_be_played_within_fret_tolerance(tp, ctr, &current_low, &current_high))
			{	//If a position change was determined to be necessary based on fingering, or this note can't be included in the set of notes played with a single fret hand position
				if(!current_low)
				{	//If a fret hand position hasn't been placed yet
					current_low = eof_pro_guitar_note_lowest_fret(tp, ctr);	//Track this note's high and low frets
					current_high = eof_pro_guitar_note_highest_fret(tp, ctr);
				}
				if(current_low > 19)
				{	//Ensure the fret hand position is capped at 19, since 22 is the highest fret supported in either Rock Band or Rocksmith
					current_low = 19;
				}
				if(current_low != last_anchor)
				{	//As long as the hand position being written is different from the previous one
					(void) eof_track_add_section(sp, track, EOF_FRET_HAND_POS_SECTION, difficulty, current_note->pos, current_low, 0, NULL);	//Add the best determined fret hand position
					last_anchor = current_low;
				}
				if(force_change)
				{	//If a fret hand position change was forced due to note fingering
					if(current_note != tp->note[ctr])
					{	//If the position for this note was not written yet
						current_low = eof_pro_guitar_note_lowest_fret(tp, ctr);	//Initialize the low fret used for this note
						current_note = tp->note[ctr];	//Store this note's address
						if(current_low != last_anchor)
						{	//As long as the hand position being written is different from the previous one
							(void) eof_track_add_section(sp, track, EOF_FRET_HAND_POS_SECTION, difficulty, current_note->pos, current_low, 0, NULL);	//Add the fret hand position for this forced position change
							last_anchor = current_low;
						}
					}
					current_note = NULL;	//This note's position will not receive another hand position, the next loop iteration will look for any necessary position changes starting with the next note's location
				}
				else
				{
					current_note = tp->note[ctr];	//Store this note's address
				}
				current_low = eof_pro_guitar_note_lowest_fret(tp, ctr);	//Track this note's high and low frets
				current_high = eof_pro_guitar_note_highest_fret(tp, ctr);
			}
		}//If it is in the specified difficulty and isn't marked as a temporary note (a single note inserted to allow chord techniques to appear in Rocksmith)
	}//For each note in the track

	//The last one or more notes examined need to have their hand position placed
	if(!current_low)
	{	//If only open notes were played in this track difficulty
		current_low = 1;	//Place the fret hand position at fret 1
	}
	else if(current_low > 19)
	{	//Ensure the fret hand position is capped at 19, since 22 is the highest fret supported in either Rock Band or Rocksmith
		current_low = 19;
	}
	if((current_low != last_anchor) && current_note)
	{	//If the last parsed note requires a position change
		(void) eof_track_add_section(sp, track, EOF_FRET_HAND_POS_SECTION, difficulty, current_note->pos, current_low, 0, NULL);	//Add the best determined fret hand position
	}

	//Ensure that a fret hand position is defined in each phrase, at or before its first note
	for(beatctr = 0; beatctr < sp->beats; beatctr++)
	{	//For each beat in the project
		if((sp->beat[beatctr]->contained_section_event >= 0) || ((beatctr + 1 >= sp->beats) && started))
		{	//If this beat has a section event (RS phrase) or a phrase is in progress and this is the last beat, it marks the end of any current phrase and the potential start of another
			if(started)
			{	//If the first phrase marker has been encountered, this beat marks the end of a phrase
				endpos = sp->beat[beatctr]->pos - 1;	//Track this as the end position of the phrase
				eof_enforce_rs_phrase_begin_with_fret_hand_position(sp, track, difficulty, startpos, endpos, &eof_fret_hand_position_list_dialog_undo_made);	//Add a fret hand position
			}//If the first phrase marker has been encountered, this beat marks the end of a phrase

			started = 1;	//Track that a phrase has been encountered
			startpos = eof_song->beat[beatctr]->pos;	//Track the starting position of the phrase
		}//If this beat has a section event (RS phrase) or a phrase is in progress and this is the last beat, it marks the end of any current phrase and the potential start of another
	}//For each beat in the project

	//Clean up
	free(eof_fret_range_tolerances);
	eof_fret_range_tolerances = NULL;	//Clear this array so that the next call to eof_build_fret_range_tolerances() rebuilds it accordingly
	eof_pro_guitar_track_sort_fret_hand_positions(tp);	//Sort the positions
	eof_render();
}

int eof_generate_hand_positions_current_track_difficulty(void)
{
	int junk;
	unsigned long diffindex = 0;

	if(!eof_song || (eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return 0;	//Invalid parameters

	eof_generate_efficient_hand_positions(eof_song, eof_selected_track, eof_note_type, 1, 0);	//Warn the user if existing hand positions will be deleted (use a static fret range tolerance of 4 for all frets, since it's assumed the author is charting for Rocksmith if they use this function)

	(void) eof_pro_guitar_track_find_effective_fret_hand_position_definition(eof_song->pro_guitar_track[eof_song->track[eof_selected_track]->tracknum], eof_note_type, eof_music_pos - eof_av_delay, NULL, &diffindex, 0);
		//Obtain the fret hand position change now in effect at the current seek position
	eof_fret_hand_position_list_dialog[1].d1 = diffindex;	//Pre-select it in the list
	(void) dialog_message(eof_fret_hand_position_list_dialog, MSG_START, 0, &junk);	//Re-initialize the dialog
	(void) dialog_message(eof_fret_hand_position_list_dialog, MSG_DRAW, 0, &junk);	//Redraw dialog
	return D_REDRAW;
}

int eof_note_can_be_played_within_fret_tolerance(EOF_PRO_GUITAR_TRACK *tp, unsigned long note, unsigned char *current_low, unsigned char *current_high)
{
	unsigned char effective_lowest, effective_highest;	//Stores the cumulative highest and lowest fret values with the input range and the next note for tolerance testing
	long next;

	if(!tp || !current_low || !current_high || (note >= tp->notes) || (*current_low > *current_high) || (*current_high > tp->numfrets) || !eof_fret_range_tolerances)
		return 0;	//Invalid parameters

	while(1)
	{
		effective_lowest = eof_pro_guitar_note_lowest_fret(tp, note);	//Find the highest and lowest fret used in the note
		effective_highest = eof_pro_guitar_note_highest_fret(tp, note);
		if(!effective_lowest && (note + 1 < tp->notes))
		{	//If only open strings are played in the note
			next = eof_fixup_next_pro_guitar_note(tp, note);	//Determine if there's another note in this track difficulty
			if(next >= 0)
			{	//If there was another note
				note = next;	//Examine it and recheck its highest and lowest frets (so that position changes can be made for further ahead notes)
				continue;
			}
		}
		break;
	}

	if(!(*current_low))
	{	//If there's no hand position in effect yet, init the currently tracked high and low frets with this note's
		*current_low = effective_lowest;
		*current_high = effective_highest;
		return 1;	//Return note can be played without an additional hand position
	}

	if(eof_pro_guitar_note_is_barre_chord(tp, note))
	{	//If this note is a barre chord
		if(*current_low != effective_lowest)
		{	//If the ongoing lowest fret value is not at lowest used fret in this barre chord (where the index finger will need to be to play the chord)
			return 0;	//Return note cannot be played without an additional hand position
		}
	}
	if(!effective_lowest)
	{	//If this note didn't have a low fret value
		effective_lowest = *current_low;	//Keep the ongoing lowest fret value
	}
	else if((*current_low != 0) && (*current_low < effective_lowest))
	{	//Otherwise keep the ongoing lowest fret value only if it is defined and is lower than this note's lowest fret value
		effective_lowest = *current_low;
	}
	if(*current_high > effective_highest)
	{	//Obtain the higher of these two values
		effective_highest = *current_high;
	}

	if(effective_highest - effective_lowest + 1 > eof_fret_range_tolerances[effective_lowest])
	{	//If this note can't be played at the same hand position as the one already in effect
		return 0;	//Return note cannot be played without an additional hand position
	}

	*current_low = effective_lowest;	//Update the effective highest and lowest used frets
	*current_high = effective_highest;
	return 1;	//Return note can be played without an additional hand position
}

void eof_build_fret_range_tolerances(EOF_PRO_GUITAR_TRACK *tp, unsigned char difficulty, char dynamic)
{
	unsigned long ctr;
	unsigned char lowest, highest, range;

	if(!tp)
	{	//Invalid parameters
		eof_fret_range_tolerances = NULL;
		return;
	}

	if(eof_fret_range_tolerances)
	{	//If this array was previously built
		free(eof_fret_range_tolerances);	//Release its memory as it will be rebuilt to suit this track difficulty
	}
	eof_fret_range_tolerances = malloc((size_t)tp->numfrets + 1);	//Allocate memory for an array large enough to specify the fret hand's range for each fret starting the numbering with 1 instead of 0
	if(!eof_fret_range_tolerances)
	{	//Couldn't allocate memory
		return;
	}

	//Initialize the tolerance of each fret to 4
	memset(eof_fret_range_tolerances, 4, (size_t)tp->numfrets + 1);	//Set a default range of 4 frets for the entire guitar neck

	if(!dynamic)
	{	//If the tolerances aren't being built from the specified track, return with all tolerances initialized to 4
		return;
	}

	//Find the range of each fret as per the notes in the track
	for(ctr = 0; ctr < tp->notes; ctr++)
	{	//For each note in the specified track
		if(tp->note[ctr]->type == difficulty)
		{	//If it is in the specified difficulty
			lowest = eof_pro_guitar_note_lowest_fret(tp, ctr);	//Get the lowest and highest fret used by this note
			highest = eof_pro_guitar_note_highest_fret(tp, ctr);

			range = highest - lowest + 1;	//Determine the range used by this note, and assume it's range of frets is playable since that's how the chord is defined for play
			if(eof_fret_range_tolerances[lowest] < range)
			{	//If the current fret range for this fret position is lower than this chord uses
				eof_fret_range_tolerances[lowest] = range;	//Update the range to reflect that this chord is playable
			}
		}
	}

	//Update the array so that any range that is valid for a lower fret number is valid for a higher fret number
	range = eof_fret_range_tolerances[1];	//Start with the range of fret 1
	for(ctr = 2; ctr < tp->numfrets + 1; ctr++)
	{	//For each of the frets in the array, starting with the second
		if(eof_fret_range_tolerances[ctr] < range)
		{	//If this fret's defined range is lower than a lower (longer fret)
			eof_fret_range_tolerances[ctr] = range;	//Update it
		}
		range = eof_fret_range_tolerances[ctr];	//Track the current fret's range
	}
}

unsigned char eof_pro_guitar_track_find_effective_fret_hand_position(EOF_PRO_GUITAR_TRACK *tp, unsigned char difficulty, unsigned long position)
{
	unsigned long ctr;
	unsigned char effective = 0;

	if(!tp)
		return 0;

	for(ctr = 0; ctr < tp->handpositions; ctr++)
	{	//For each hand position in the track
		if(tp->handposition[ctr].difficulty == difficulty)
		{	//If the hand position is in the specified difficulty
			if(tp->handposition[ctr].start_pos <= position)
			{	//If the hand position is at or before the specified timestamp
				effective = tp->handposition[ctr].end_pos;	//Track its fret number
			}
			else
			{	//This hand position is beyond the specified timestamp
				return effective;	//Return the last hand position that was found (if any) in this track difficulty
			}
		}
	}

	return effective;	//Return the last hand position definition that was found (if any) in this track difficulty
}

EOF_PHRASE_SECTION *eof_pro_guitar_track_find_effective_fret_hand_position_definition(EOF_PRO_GUITAR_TRACK *tp, unsigned char difficulty, unsigned long position, unsigned long *index, unsigned long *diffindex, char function)
{
	unsigned long ctr, ctr2 = 0;
	EOF_PHRASE_SECTION *ptr = NULL;

	if(!tp)
		return 0;	//Invalid parameters

	for(ctr = 0; ctr < tp->handpositions; ctr++)
	{	//For each hand position in the track
		if(tp->handposition[ctr].difficulty == difficulty)
		{	//If the hand position is in the specified difficulty
			if(tp->handposition[ctr].start_pos <= position)
			{	//If the hand position is at or before the specified timestamp
				if(function && (tp->handposition[ctr].start_pos != position))
				{	//If the calling function only wanted to identify the fret hand position exactly at the target time stamp
					continue;	//Skip to next position if this one isn't at the target time
				}
				ptr = &tp->handposition[ctr];	//Store its address
				if(index)
				{	//If index isn't NULL
					*index = ctr;		//Track this fret hand position definition number
				}
				if(diffindex)
				{	//If diffindex isn't NULL
					*diffindex = ctr2;	//Track the position number this is within the target difficulty
				}
			}
			else
			{	//This hand position is beyond the specified timestamp
				return ptr;		//Return any hand position address that has been found
			}
			ctr2++;
		}
	}

	return ptr;	//Return any hand position address that has been found
}

char *eof_rs_section_text_valid(char *string)
{
	unsigned long ctr;

	if(!string)
		return NULL;	//Return error

	for(ctr = 0; ctr < EOF_NUM_RS_PREDEFINED_SECTIONS; ctr++)
	{	//For each pre-defined Rocksmith section
		if(!ustricmp(eof_rs_predefined_sections[ctr].string, string) || !ustricmp(eof_rs_predefined_sections[ctr].displayname, string))
		{	//If the string matches this Rocksmith section entry's native or display name
			return eof_rs_predefined_sections[ctr].string;	//Return match
		}
	}
	return NULL;	//Return no match
}

int eof_rs_event_text_valid(char *string)
{
	unsigned long ctr;

	if(!string)
		return 0;	//Return error

	for(ctr = 0; ctr < EOF_NUM_RS_PREDEFINED_EVENTS; ctr++)
	{	//For each pre-defined Rocksmith event
		if(!ustrcmp(eof_rs_predefined_events[ctr].string, string))
		{	//If the string matches this Rocksmith event entry
			return 1;	//Return match
		}
	}
	return 0;	//Return no match
}

unsigned long eof_get_rs_section_instance_number(EOF_SONG *sp, unsigned long track, unsigned long event)
{
	unsigned long ctr, count = 1;

	if(!sp || (event >= sp->text_events) || !(sp->text_event[event]->flags & EOF_EVENT_FLAG_RS_SECTION))
		return 0;	//If the parameters are invalid, or the specified text event is not a Rocksmith section
	if(sp->text_event[event]->track && (sp->text_event[event]->track != track))
		return 0;	//If the specified event is assigned to a track other than the one specified

	for(ctr = 0; ctr < event; ctr++)
	{	//For each text event in the chart that is before the specified event
		if(sp->text_event[ctr]->flags & EOF_EVENT_FLAG_RS_SECTION)
		{	//If the text event is marked as a Rocksmith section
			if(!sp->text_event[ctr]->track || (sp->text_event[ctr]->track == track))
			{	//If the text event is not track specific or is assigned to the specified track
				if(!ustrcmp(sp->text_event[ctr]->text, sp->text_event[event]->text))
				{	//If the text event's text matches
					count++;	//Increment the instance counter
				}
			}
		}
	}

	return count;
}

void eof_get_rocksmith_wav_path(char *buffer, const char *parent_folder, size_t num)
{
	(void) replace_filename(buffer, parent_folder, "", (int)num - 1);	//Obtain the destination path

	//Build target WAV file name
	put_backslash(buffer);
	if(eof_song->tags->title[0] != '\0')
	{	//If the chart has a defined song title
		(void) ustrncat(buffer, eof_song->tags->title, (int)num - 1);
	}
	else
	{	//Otherwise default to "guitar"
		(void) ustrncat(buffer, "guitar", (int)num - 1);
	}
	(void) ustrncat(buffer, ".wav", (int)num - 1);
	buffer[num - 1] = '\0';	//Ensure the finalized string is terminated
}

void eof_delete_rocksmith_wav(void)
{
	char checkfn[1024] = {0};

	eof_get_rocksmith_wav_path(checkfn, eof_song_path, sizeof(checkfn));	//Build the path to the WAV file written for Rocksmith during save
	if(exists(checkfn))
	{	//If the path based on the song title exists
		(void) delete_file(checkfn);	//Delete it, if it exists, since changing the chart's OGG will necessitate rewriting the WAV file during save
	}
	else
	{	//Otherwise delete guitar.wav because this is the name it will use if the song title has characters invalid for a filename
		(void) replace_filename(checkfn, eof_song_path, "guitar.wav", 1024);
		(void) delete_file(checkfn);
	}
}

char eof_compare_time_range_with_previous_or_next_difficulty(EOF_SONG *sp, unsigned long track, unsigned long start, unsigned long stop, unsigned char diff, char compareto)
{
	unsigned long ctr2, ctr3, thispos, thispos2;
	unsigned char note_found;
	unsigned char comparediff, thisdiff, populated = 0;

	if(!sp || (track >= sp->tracks) || (start > stop))
		return 0;	//Invalid parameters
	if(!diff && (compareto < 0))
		return 0;	//There is no difficulty before the first difficulty
	if((diff == 255) && (compareto >= 0))
		return 0;	//There is no difficulty after the last difficulty

	if(compareto < 0)
	{	//Compare the specified difficulty with the previous difficulty
		if(!diff)
			return 0;	//There is no difficulty before the first difficulty
		comparediff = diff - 1;
		if(((sp->track[track]->flags & EOF_TRACK_FLAG_UNLIMITED_DIFFS) == 0) && (comparediff == 4))
		{	//If the track is using the traditional 5 difficulty system and the difficulty previous to the being examined is the BRE difficulty
			comparediff--;	//Compare to the previous difficulty
		}
	}
	else
	{	//Compare the specified difficulty with the next difficulty
		if(diff == 255)
			return 0;	//There is no difficulty after the last difficulty
		comparediff = diff + 1;
		if(((sp->track[track]->flags & EOF_TRACK_FLAG_UNLIMITED_DIFFS) == 0) && (comparediff == 4))
		{	//If the track is using the traditional 5 difficulty system and the difficulty next to the being examined is the BRE difficulty
			comparediff++;	//Compare to the next difficulty
		}
	}

	//First pass:  Compare notes in the specified difficulty with those in the comparing difficulty
	for(ctr2 = 0; ctr2 < eof_get_track_size(sp, track); ctr2++)
	{	//For each note in the track
		thispos = eof_get_note_pos(sp, track, ctr2);	//Get this note's position
		if(thispos > stop)
		{	//If this note (and all remaining notes, since they are expected to remain sorted) is beyond the specified range, break from loop
			break;
		}

		if(thispos >= start)
		{	//If this note is at or after the start of the specified range, check its difficulty
			thisdiff = eof_get_note_type(sp, track, ctr2);	//Get this note's difficulty
			if(thisdiff == diff)
			{	//If this note is in the difficulty being examined
				populated = 1;	//Track that there was at least 1 note in the specified difficulty of the phrase
				//Compare this note to the one at the same position in the comparing difficulty, if there is one
				note_found = 0;	//This condition will be set if this note is found in the comparing difficulty
				if(compareto < 0)
				{	//If comparing to the previous difficulty, parse notes backwards
					for(ctr3 = ctr2; ctr3 > 0; ctr3--)
					{	//For each previous note in the track, for performance reasons going backward from the one being examined in the outer loop (which has a higher note number when sorted)
						thispos2 = eof_get_note_pos(sp, track, ctr3 - 1);	//Get this note's position
						thisdiff = eof_get_note_type(sp, track, ctr3 - 1);	//Get this note's difficulty
						if(thispos2 < thispos)
						{	//If this note and all previous ones are before the one being examined in the outer loop
							break;
						}
						if((thispos == thispos2) && (thisdiff == comparediff))
						{	//If this note is at the same position and one difficulty lower than the one being examined in the outer loop
							note_found = 1;	//Track that a note at the same position was found in the previous difficulty
							if(eof_note_compare(sp, track, ctr2, track, ctr3 - 1, 1))
							{	//If the two notes don't match (including lengths and flags)
								return 1;	//Return difference found
							}
							break;
						}
					}
				}
				else
				{	//Comparing to the next difficulty, parse notes forwards
					for(ctr3 = ctr2 + 1; ctr3 < eof_get_track_size(sp, track); ctr3++)
					{	//For each remaining note in the track
						thispos2 = eof_get_note_pos(sp, track, ctr3);	//Get this note's position
						thisdiff = eof_get_note_type(sp, track, ctr3);	//Get this note's difficulty
						if(thispos2 > thispos)
						{	//If this note and all subsequent ones are after the one being examined in the outer loop
							break;
						}
						if((thispos == thispos2) && (thisdiff == comparediff))
						{	//If this note is at the same position and one difficulty lower than the one being examined in the outer loop
							note_found = 1;	//Track that a note at the same position was found in the previous difficulty
							if(eof_note_compare(sp, track, ctr2, track, ctr3 - 1, 1))
							{	//If the two notes don't match (including lengths and flags)
								return 1;	//Return difference found
							}
							break;
						}
					}
				}
				if(!note_found)
				{	//If this note has no note at the same position in the previous difficulty
					return 1;	//Return difference found
				}
			}//If this note is in the difficulty being examined
		}//If this note is at or after the start of the specified range, check its difficulty
	}//For each note in the track

	if(!populated)
	{	//If no notes were contained within the time range in the specified difficulty
		return -1;	//Return empty time range
	}

	//Second pass:  Compare notes in the comparing difficulty with those in the specified difficulty
	for(ctr2 = 0; ctr2 < eof_get_track_size(sp, track); ctr2++)
	{	//For each note in the track
		thispos = eof_get_note_pos(sp, track, ctr2);	//Get this note's position
		if(thispos > stop)
		{	//If this note (and all remaining notes, since they are expected to remain sorted) is beyond the specified range, break from loop
			break;
		}

		if(thispos >= start)
		{	//If this note is at or after the start of the specified range, check its difficulty
			thisdiff = eof_get_note_type(sp, track, ctr2);	//Get this note's difficulty
			if(thisdiff == comparediff)
			{	//If this note is in the difficulty being compared
				//Compare this note to the one at the same position in the examined difficulty, if there is one
				note_found = 0;	//This condition will be set if this note is found in the examined difficulty
				if(compareto < 0)
				{	//If comparing the comparing difficulty with the next, parse notes forwards
					for(ctr3 = ctr2 + 1; ctr3 < eof_get_track_size(sp, track); ctr3++)
					{	//For each remaining note in the track
						thispos2 = eof_get_note_pos(sp, track, ctr3);	//Get this note's position
						thisdiff = eof_get_note_type(sp, track, ctr3);	//Get this note's difficulty
						if(thispos2 > thispos)
						{	//If this note and all subsequent ones are after the one being examined in the outer loop
							break;
						}
						if((thispos == thispos2) && (thisdiff == diff))
						{	//If this note is in the specified difficulty and at the same position as the one being examined in the outer loop
							note_found = 1;	//Track that a note at the same position was found in the previous difficulty
							if(eof_note_compare(sp, track, ctr2, track, ctr3 - 1, 1))
							{	//If the two notes don't match (including lengths and flags)
								return 1;	//Return difference found
							}
							break;
						}
					}
				}
				else
				{	//Comparing the comparing difficulty with the previous, parse notes backwards
					for(ctr3 = ctr2; ctr3 > 0; ctr3--)
					{	//For each previous note in the track, for performance reasons going backward from the one being examined in the outer loop (which has a higher note number when sorted)
						thispos2 = eof_get_note_pos(sp, track, ctr3 - 1);	//Get this note's position
						thisdiff = eof_get_note_type(sp, track, ctr3 - 1);	//Get this note's difficulty
						if(thispos2 < thispos)
						{	//If this note and all previous ones are before the one being examined in the outer loop
							break;
						}
						if((thispos == thispos2) && (thisdiff == diff))
						{	//If this note is in the specified difficulty and at the same position as the one being examined in the outer loop
							note_found = 1;	//Track that a note at the same position was found in the previous difficulty
							if(eof_note_compare(sp, track, ctr2, track, ctr3 - 1, 1))
							{	//If the two notes don't match (including lengths and flags)
								return 1;	//Return difference found
							}
							break;
						}
					}
				}
				if(!note_found)
				{	//If this note has no note at the same position in the previous difficulty
					return 1;	//Return difference found
				}
			}//If this note is in the difficulty being compared
		}//If this note is at or after the start of the specified range, check its difficulty
	}//For each note in the track

	return 0;	//Return no difference found
}

unsigned char eof_find_fully_leveled_rs_difficulty_in_time_range(EOF_SONG *sp, unsigned long track, unsigned long start, unsigned long stop, unsigned char relative)
{
	unsigned char reldiff, fullyleveleddiff = 0;
	unsigned long ctr;

	if(!sp || (track >= sp->tracks) || (start > stop))
		return 0;	//Invalid parameters

	(void) eof_detect_difficulties(sp, track);	//Update eof_track_diff_populated_status[] to reflect all populated difficulties for this track
	if((sp->track[track]->flags & EOF_TRACK_FLAG_UNLIMITED_DIFFS) == 0)
	{	//If the track is using the traditional 5 difficulty system
		eof_track_diff_populated_status[4] = 0;	//Ensure that the BRE difficulty is ignored
	}
	for(ctr = 0; ctr < 256; ctr++)
	{	//For each of the possible difficulties
		if(eof_track_diff_populated_status[ctr])
		{	//If this difficulty isn't empty
			if(eof_compare_time_range_with_previous_or_next_difficulty(sp, track, start, stop, ctr, -1) > 0)
			{	//If this difficulty isn't empty and had more notes than the previous or any of the notes within the phrase were different than those in the previous difficulty
				fullyleveleddiff = ctr;			//Track the lowest difficulty number that represents the fully leveled time range of notes
			}
		}
	}//For each of the possible difficulties

	if(!relative)
	{	//If the resulting difficulty number is not to be converted to Rocksmith's relative difficulty number system
		return fullyleveleddiff;
	}

	//Remap fullyleveleddiff to the relative difficulty
	for(ctr = 0, reldiff = 0; ctr < 256; ctr++)
	{	//For each of the possible difficulties
		if(((sp->track[track]->flags & EOF_TRACK_FLAG_UNLIMITED_DIFFS) == 0) && (ctr == 4))
		{	//If the track is using the traditional 5 difficulty system and the difficulty being examined is the BRE difficulty
			continue;	//Don't process the BRE difficulty, it will not be exported
		}
		if(fullyleveleddiff == ctr)
		{	//If the corresponding relative difficulty has been found
			return reldiff;	//Return it
		}
		if(eof_track_diff_populated_status[ctr])
		{	//If the track is populated
			reldiff++;
		}
	}
	return 0;	//Error
}

int eof_check_rs_sections_have_phrases(EOF_SONG *sp, unsigned long track)
{
	unsigned long ctr, old_text_events;
	char user_prompted = 0;

	if(!sp || (track >= sp->tracks))
		return 1;	//Invalid parameters
	if(!eof_get_track_size(sp, track))
		return 0;	//Empty track

	eof_process_beat_statistics(sp, track);	//Cache section name information into the beat structures (from the perspective of the specified track)
	for(ctr = 0; ctr < sp->beats; ctr++)
	{	//For each beat
		if(sp->beat[ctr]->contained_rs_section_event >= 0)
		{	//If this beat contains a RS section
			if(sp->beat[ctr]->contained_section_event < 0)
			{	//But it doesn't contain a RS phrase
				eof_clear_input();
				key[KEY_Y] = 0;
				key[KEY_N] = 0;
				if(!user_prompted && alert("At least one Rocksmith section doesn't have a Rocksmith phrase at the same position.", "This can cause the chart's sections to work incorrectly", "Would you like to place Rocksmith phrases to correct this?", "&Yes", "&No", 'y', 'n') != 1)
				{	//If the user hasn't already answered this prompt, and doesn't opt to correct the issue
					return 2;	//Return user cancellation
				}
				user_prompted = 1;
				(void) eof_menu_track_selected_track_number(track, 1);		//Change the active track
				eof_selected_beat = ctr;					//Change the selected beat
				eof_set_seek_position(sp->beat[ctr]->pos + eof_av_delay);	//Seek to the beat
				if(eof_2d_render_top_option != 36)
				{	//If the piano roll isn't already displaying both RS sections and phrases
					eof_2d_render_top_option = 35;					//Change the user preference to render RS sections at the top of the piano roll
				}
				eof_find_lyric_preview_lines();
				eof_render();							//Render the track so the user can see where the correction needs to be made, along with the RS section in question

				while(1)
				{
					old_text_events = sp->text_events;				//Remember how many text events there were before launching dialog
					(void) eof_rocksmith_phrase_dialog_add();			//Launch dialog for user to add a Rocksmith phrase
					if(old_text_events == sp->text_events)
					{	//If the number of text events defined in the chart didn't change, the user canceled
						return 2;	//Return user cancellation
					}
					eof_process_beat_statistics(sp, track);	//Rebuild beat statistics to check if user added a Rocksmith phrase
					if(sp->beat[ctr]->contained_section_event < 0)
					{	//If user added a text event, but it wasn't a Rocksmith phrase
						eof_clear_input();
						key[KEY_Y] = 0;
						key[KEY_N] = 0;
						if(alert("You didn't add a Rocksmith phrase.", NULL, "Do you want to continue adding RS phrases for RS sections that are missing them?", "&Yes", "&No", 'y', 'n') != 1)
						{	//If the user doesn't opt to finish correcting the issue
							return 2;	//Return user cancellation
						}
					}
					else
					{	//User added the missing RS phrase
						break;
					}
				}
			}
		}
	}

	return 0;	//Return completion
}

unsigned long eof_find_effective_rs_phrase(unsigned long position)
{
	unsigned long ctr, effective = 0;

	for(ctr = 0; ctr < eof_song->beats; ctr++)
	{	//For each beat in the chart
		if(eof_song->beat[ctr]->contained_section_event >= 0)
		{	//If this beat has a section event (RS phrase)
			if(eof_song->beat[ctr]->pos <= position)
			{	//If this phrase is at or before the position being checked
				effective++;
			}
			else
			{	//If this phrase is after the position being checked
				break;
			}
		}
	}

	if(effective)
	{	//If the specified phrase was found
		return effective - 1;	//Return its number
	}

	return 0;	//Return no phrase found
}

int eof_time_range_is_populated(EOF_SONG *sp, unsigned long track, unsigned long start, unsigned long stop, unsigned char diff)
{
	unsigned long ctr2, thispos;
	unsigned char thisdiff;

	if(!sp || (track >= sp->tracks) || (start > stop))
		return 0;	//Invalid parameters

	for(ctr2 = 0; ctr2 < eof_get_track_size(sp, track); ctr2++)
	{	//For each note in the track
		thispos = eof_get_note_pos(sp, track, ctr2);	//Get this note's position
		if(thispos > stop)
		{	//If this note (and all remaining notes, since they are expected to remain sorted) is beyond the specified range, break from loop
			break;
		}
		if(thispos >= start)
		{	//If this note is at or after the start of the specified range, check its difficulty
			thisdiff = eof_get_note_type(sp, track, ctr2);	//Get this note's difficulty
			if(thisdiff == diff)
			{	//If this note is in the difficulty being examined
				return 1;	//Return specified range at the specified difficulty is populated
			}
		}
	}

	return 0;	//Return not populated
}

int eof_note_has_high_chord_density(EOF_SONG *sp, unsigned long track, unsigned long note)
{
	long prev;

	if((sp == NULL) || (track >= sp->tracks))
		return 0;	//Error

	if(sp->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 0;	//Note is not a pro guitar/bass note

	if(eof_get_note_flags(sp, track, note) & EOF_NOTE_FLAG_CRAZY)
		return 0;	//Note is marked with crazy status, which forces it to export as low density

	if(eof_note_count_rs_lanes(sp, track, note) < 2)
		return 0;	//Note is not a chord (not at least 2 non ghosted/muted gems) or is entirely string muted

	prev = eof_track_fixup_previous_note(sp, track, note);
	if(prev < 0)
		return 0;	//No earlier note

	if(eof_get_note_pos(sp, track, note) > eof_get_note_pos(sp, track, prev) + 10000)
		return 0;	//Note is not within 10000ms of the previous note

	if(eof_note_compare(sp, track, note, track, prev, 0))
		return 0;	//Note does not match the previous note (ignoring note flags and lengths)

	return 1;	//All criteria passed, note is high density
}

void eof_enforce_rs_phrase_begin_with_fret_hand_position(EOF_SONG *sp, unsigned long track, unsigned char diff, unsigned long startpos, unsigned long endpos, char *undo_made)
{
	unsigned long ctr, firstnotepos, tracknum;
	char found = 0;
	unsigned char position;
	EOF_PRO_GUITAR_TRACK *tp;

	if(!sp || (track >= sp->tracks) || (sp->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return;	//Invalid parameters
	tracknum = sp->track[track]->tracknum;
	tp = sp->pro_guitar_track[tracknum];

	//Find the position of the first note in the phrase
	found = 0;	//Reset this condition
	for(ctr = 0; ctr < tp->notes; ctr++)
	{	//For each note in the track
		if(tp->note[ctr]->type == diff)
		{	//If the note is in the active difficulty
			if((tp->note[ctr]->pos >= startpos) && (tp->note[ctr]->pos <= endpos))
			{	//If this is the first note in the phrase
				firstnotepos = tp->note[ctr]->pos;	//Note the position
				found = 1;
				break;
			}
		}
	}

	//Determine if the necessary fret hand position was set between the start of the phrase and the first note
	if(found)
	{	//If the phrase has a note in it
		found = 0;	//Reset this condition
		for(ctr = 0; ctr < tp->handpositions; ctr++)
		{	//For each hand position in the track
			if(tp->handposition[ctr].difficulty == diff)
			{	//If the hand position is in the active difficulty
				if((tp->handposition[ctr].start_pos >= startpos) && (tp->handposition[ctr].start_pos <= firstnotepos))
				{	//If the hand position is defined anywhere between the start of the phrase and the start of the first note in that phrase
					found = 1;
					break;
				}
			}
		}
		if(!found)
		{	//If a hand position needs to be added to the difficulty
			position = eof_pro_guitar_track_find_effective_fret_hand_position(tp, diff, firstnotepos);
			if(position)
			{	//If a fret hand position was is in effect (placed anywhere earlier in the difficulty)
				if(undo_made && !(*undo_made))
				{	//If an undo state needs to be made
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					*undo_made = 1;
				}
				//Place that fret hand position for the active difficulty
				(void) eof_track_add_section(sp, track, EOF_FRET_HAND_POS_SECTION, diff, firstnotepos, position, 0, NULL);
				eof_pro_guitar_track_sort_fret_hand_positions(tp);	//Sort the positions
			}
		}
	}
}
