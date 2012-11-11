#include "beat.h"
#include "event.h"
#include "main.h"
#include "midi.h"
#include "rs.h"
#include "undo.h"
#include <time.h>

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

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
	eof_log("eof_rs_build_chord_list() entered", 1);

	unsigned long ctr, ctr2, unique_count = 0;
	EOF_PRO_GUITAR_NOTE **notelist;	//An array large enough to hold a pointer to every note in the track
	EOF_PRO_GUITAR_TRACK *tp;
	char match;

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

unsigned long eof_build_section_list(EOF_SONG *sp, unsigned long **results)
{
	eof_log("eof_build_section_list() entered", 1);

	unsigned long ctr, ctr2, unique_count = 0;
	EOF_TEXT_EVENT **eventlist;	//An array large enough to hold a pointer to every text event in the chart
	char match;

	if(!results)
		return 0;	//Return error

	if(!sp)
	{
		*results = NULL;
		return 0;	//Return error
	}

	//Duplicate the chart's tet events array
	eventlist = malloc(sizeof(EOF_TEXT_EVENT *) * EOF_MAX_TEXT_EVENTS);
	if(!eventlist)
	{
		*results = NULL;
		return 0;	//Return error
	}
	memcpy(eventlist, sp->text_event, sizeof(EOF_TEXT_EVENT *) * EOF_MAX_TEXT_EVENTS);	//Copy the event array

	//Overwrite each pointer in the duplicate event array that isn't a unique section marker with NULL
	for(ctr = 0; ctr < sp->text_events; ctr++)
	{	//For each text event in the chart
		if(eof_is_section_marker(sp->text_event[ctr]->text))
		{	//If this event is a section marker
			match = 0;
			for(ctr2 = ctr + 1; ctr2 < sp->text_events; ctr2++)
			{	//For each event in the chart that follows this event
				if(!ustricmp(sp->text_event[ctr]->text, sp->text_event[ctr2]->text))
				{	//If this event matches one that follows it
					eventlist[ctr] = NULL;	//Eliminate this event from the list
					match = 1;	//Note that this chord matched one of the others
					break;
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

int eof_export_rocksmith_track(EOF_SONG * sp, char * fn, unsigned long track)
{
	eof_log("eof_export_rocksmith() entered", 1);

	PACKFILE * fp;
	char buffer[256] = {0};
	time_t seconds;		//Will store the current time in seconds
	struct tm *caltime;	//Will store the current time in calendar format
	unsigned long ctr, ctr2, ctr3, ctr4, numsections = 0, stringnum, bitmask, numsinglenotes, numchords, *chordlist, chordlistsize, *sectionlist, sectionlistsize;
	EOF_PRO_GUITAR_TRACK *tp;

	if(!sp || !fn || !sp->beats || (track >= sp->tracks) || (sp->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT) || !sp->track[track]->name)
	{
		eof_log("\tError saving:  Invalid parameters", 1);
		return 0;	//Return failure
	}

	//Count the number of populated difficulties in the track
	tp = sp->pro_guitar_track[sp->track[track]->tracknum];
	char populated[4] = {0};	//Will track which difficulties are populated
	unsigned notetype;
	unsigned numdifficulties;
	for(ctr = 0; ctr < tp->notes; ctr++)
	{	//For each note in the track
		notetype = eof_get_note_type(sp, track, ctr);
		if(notetype < 4)
		{	//As long as this is Easy, Medium, Hard or Expert difficulty
			if((eof_get_note_flags(sp, track, ctr) & EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE) == 0)
			{	//And it's not a string muted note
				populated[notetype] = 1;	//Track that this difficulty is populated
			}
		}
	}
	numdifficulties = populated[0] + populated[1] + populated[2] + populated[3];
	if(!numdifficulties)
	{
		eof_log("Cannot export this track in Rocksmith format, it has no populated difficulties", 1);
		return 0;	//Return failure
	}

	//Update target file name and open it for writing
	replace_filename(fn, fn, sp->track[track]->name, 1024);
	replace_extension(fn, fn, "xml", 1024);
	fp = pack_fopen(fn, "w");
	if(!fp)
	{
		eof_log("\tError saving:  Cannot open file for writing", 1);
		return 0;	//Return failure
	}

	//Write the beginning of the XML file
	pack_fputs("<?xml version='1.0' encoding='UTF-8'?>\n", fp);
	pack_fputs("<song>\n", fp);
	snprintf(buffer, sizeof(buffer), "  <title>%s</title>\n", sp->tags->title);
	pack_fputs(buffer, fp);
	snprintf(buffer, sizeof(buffer), "  <arrangement>%s</arrangement>\n", sp->track[track]->name);
	pack_fputs(buffer, fp);
	pack_fputs("  <part>1</part>\n", fp);
	pack_fputs("  <offset>0.000</offset>\n", fp);
	eof_truncate_chart(sp);	//Update the chart length
	snprintf(buffer, sizeof(buffer), "  <songLength>%.3f</songLength>\n", (double)eof_chart_length / 1000.0);
	pack_fputs(buffer, fp);
	seconds = time(NULL);
	caltime = localtime(&seconds);
	if(caltime)
	{	//If the calendar time could be determined
		snprintf(buffer, sizeof(buffer), "  <lastConversionDateTime>%u-%u-%u %u:%u</lastConversionDateTime>\n", caltime->tm_mon + 1, caltime->tm_mday, caltime->tm_year % 100, caltime->tm_hour % 12, caltime->tm_min);
	}
	else
	{
		snprintf(buffer, sizeof(buffer), "  <lastConversionDateTime>UNKNOWN</lastConversionDateTime>\n");
	}
	pack_fputs(buffer, fp);

	//Write the section information
	eof_process_beat_statistics(sp);	//Cache section name information into the beat structures
	for(ctr = 0; ctr < sp->beats; ctr++)
	{	//For each beat in the chart
		if(sp->beat[ctr]->contained_section_event >= 0)
		{	//If this beat has a section event
			numsections++;	//Update section marker instance counter
		}
	}
	sectionlistsize = eof_build_section_list(sp, &sectionlist);	//Build a list of all unique section markers in the chart
	if(sectionlistsize)
	{	//If there is at least one section marker
		unsigned long phraseid;
		snprintf(buffer, sizeof(buffer), "  <phrases count=\"%lu\">\n", sectionlistsize);	//Write the number of unique sections
		pack_fputs(buffer, fp);
		for(ctr = 0; ctr < sectionlistsize; ctr++)
		{	//For each of the entries in the unique section list
			snprintf(buffer, sizeof(buffer), "    <phrase disparity=\"0\" ignore=\"0\" maxDifficulty=\"0\" name=\"%s\" solo=\"0\"/>\n", sp->text_event[sectionlist[ctr]]->text);
			pack_fputs(buffer, fp);
		}
		pack_fputs("  </phrases>\n", fp);
		snprintf(buffer, sizeof(buffer), "  <phraseIterations count=\"%lu\">\n", numsections);
		pack_fputs(buffer, fp);
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
					allegro_message("Error:  Couldn't match chord with chord template.  Aborting Rocksmith export.");
					eof_log("Error:  Couldn't match chord with chord template.  Aborting Rocksmith export.", 1);
					free(sectionlist);
					return 0;	//Return error
				}
				snprintf(buffer, sizeof(buffer), "    <phraseIteration time=\"%.3f\" phraseId=\"%lu\"/>\n", sp->beat[ctr]->fpos / 1000.0, phraseid);
				pack_fputs(buffer, fp);
			}
		}
		pack_fputs("  </phraseIterations>\n", fp);
		free(sectionlist);
	}
	else
	{	//There are no section markers, write a single default phrase
		pack_fputs("  <phrases count=\"1\">\n", fp);
		snprintf(buffer, sizeof(buffer), "    <phrase disparity=\"0\" ignore=\"1\" maxDifficulty=\"0\" name=\"COUNT\" solo=\"0\"/>\n");
		pack_fputs(buffer, fp);
		pack_fputs("  </phrases>\n", fp);
		pack_fputs("  <phraseIterations count=\"1\">\n", fp);
		snprintf(buffer, sizeof(buffer), "    <phraseIteration time=\"%.3f\" phraseId=\"0\"/>\n", sp->beat[0]->fpos / 1000.0);	//Place this phrase at the first beat marker's position
		pack_fputs(buffer, fp);
		pack_fputs("  </phraseIterations>\n", fp);
	}

	//Write some unknown information
	pack_fputs("  <linkedDiffs count=\"0\"/>\n", fp);
	pack_fputs("  <phraseProperties count=\"0\"/>\n", fp);

	//Write chord templates
	chordlistsize = eof_build_chord_list(sp, track, &chordlist);	//Build a list of all unique chords in the track
	if(!chordlistsize)
	{	//If there were no chords, write an empty chord template tag
		pack_fputs("  <chordTemplates count=\"0\"/>\n", fp);
	}
	else
	{
		char notename[EOF_NAME_LENGTH+1];	//String large enough to hold any chord name supported by EOF
		long fret0, fret1, fret2, fret3, fret4, fret5;	//Will store the fret number played on each string (-1 means the string is not played)
		long *fret[6] = {&fret0, &fret1, &fret2, &fret3, &fret4, &fret5};	//Allow the fret numbers to be accessed via array
		char *fingerunknown = "#";
		char *fingerunused = "-1";
		char *finger0, *finger1, *finger2, *finger3, *finger4, *finger5;	//Each will be set to either fingerunknown or fingerunused
		char **finger[6] = {&finger0, &finger1, &finger2, &finger3, &finger4, &finger5};	//Allow the finger strings to be accessed via array
		char finger0def[2] = "0", finger1def[2] = "1", finger2def[2] = "2", finger3def[2] = "3", finger4def[2] = "4", finger5def[2] = "5";	//Static strings for building manually-defined finger information
		char *fingerdef[6] = {finger0def, finger1def, finger2def, finger3def, finger4def, finger5def};	//Allow the fingerdef strings to be accessed via array
		unsigned long bitmask;
		char fingeringpresent;	//Is set to nonzero if fingering is defined for ANY of this note's used strings

		snprintf(buffer, sizeof(buffer), "  <chordTemplates count=\"%lu\">\n", chordlistsize);
		pack_fputs(buffer, fp);
		for(ctr = 0; ctr < chordlistsize; ctr++)
		{	//For each of the entries in the unique chord list
			fingeringpresent = 0;	//Reset this status
			notename[0] = '\0';	//Empty the note name string
			eof_build_note_name(sp, track, chordlist[ctr], notename);	//Build the note name (if it exists) into notename[]

			for(ctr2 = 0, bitmask = 1; ctr2 < 6; ctr2++, bitmask <<= 1)
			{	//For each of the 6 supported strings
				if((eof_get_note_note(sp, track, chordlist[ctr]) & bitmask) && (ctr2 < tp->numstrings) && ((tp->note[chordlist[ctr]]->frets[ctr2] & 0x80) == 0))
				{	//If the chord entry uses this string (verifying that the string number is supported by the track) and the string is not fret hand muted
					if(tp->note[chordlist[ctr]]->finger[ctr2])
					{	//If the fingering for this string is defined
						fingeringpresent = 1;	//Track that there is fingering defined, at least partially
						break;
					}
				}
			}
			for(ctr2 = 0, bitmask = 1; ctr2 < 6; ctr2++, bitmask <<= 1)
			{	//For each of the 6 supported strings
				if((eof_get_note_note(sp, track, chordlist[ctr]) & bitmask) && (ctr2 < tp->numstrings) && ((tp->note[chordlist[ctr]]->frets[ctr2] & 0x80) == 0))
				{	//If the chord entry uses this string (verifying that the string number is supported by the track) and the string is not fret hand muted
					*(fret[ctr2]) = tp->note[chordlist[ctr]]->frets[ctr2] & 0x7F;	//Retrieve the fret played on this string (masking out the muting bit)
					if(tp->note[chordlist[ctr]]->finger[ctr2])
					{	//If the fingering for this string is defined
						char *temp = fingerdef[ctr2];	//Simplify logic below
						temp[0] = '0' + tp->note[chordlist[ctr]]->finger[ctr2];	//Convert decimal to ASCII
						temp[1] = '\0';	//Truncate string
						*(finger[ctr2]) = temp;
					}
					else
					{
						if(!fingeringpresent)
						{	//If no fingering information is present for the chord
							*(finger[ctr2]) = fingerunused;		//Write a -1, this will allow the XML to compile even if the user defines no fingering for any chords
						}
						else
						{	//If there is fingering defined for any of the other strings
							*(finger[ctr2]) = fingerunknown;	//Write the placeholder #, the user will have to update it
						}
					}
				}
				else
				{	//The chord entry does not use this string
					*(fret[ctr2]) = -1;
					*(finger[ctr2]) = fingerunused;
				}
			}

			snprintf(buffer, sizeof(buffer), "    <chordTemplate chordName=\"%s\" finger0=\"%s\" finger1=\"%s\" finger2=\"%s\" finger3=\"%s\" finger4=\"%s\" finger5=\"%s\" fret0=\"%ld\" fret1=\"%ld\" fret2=\"%ld\" fret3=\"%ld\" fret4=\"%ld\" fret5=\"%ld\"/>\n", notename, finger0, finger1, finger2, finger3, finger4, finger5, fret0, fret1, fret2, fret3, fret4, fret5);
			pack_fputs(buffer, fp);
		}//For each of the entries in the unique chord list
		pack_fputs("  </chordTemplates>\n", fp);
	}

	//Write some unknown information
	pack_fputs("  <fretHandMuteTemplates count=\"0\"/>\n", fp);

	//Write the beat timings
	snprintf(buffer, sizeof(buffer), "  <ebeats count=\"%lu\">\n", sp->beats);
	pack_fputs(buffer, fp);
	unsigned beatspermeasure = 4, beatcounter = 0;
	long displayedmeasure, measurenum = 0;
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
		snprintf(buffer, sizeof(buffer), "    <ebeat time=\"%.3f\" measure=\"%ld\"/>\n", sp->beat[ctr]->fpos / 1000.0, displayedmeasure);
		pack_fputs(buffer, fp);
		beatcounter++;
		if(beatcounter >= beatspermeasure)
		{
			beatcounter = 0;
		}
	}
	pack_fputs("  </ebeats>\n", fp);

	//Write some unknown information
	pack_fputs("  <controls count =\"2\">\n", fp);
	pack_fputs("    <control time=\"5.100\" code=\"ShowMessageBox(hint1, Rocksmith Custom Song Project Demo)\"/>\n", fp);
	pack_fputs("    <control time=\"30.100\" code=\"ClearAllMessageBoxes()\"/>\n", fp);
	pack_fputs("  </controls>\n", fp);
	pack_fputs("  <sections count=\"0\"/>\n", fp);
	pack_fputs("  <events count=\"0\"/>\n", fp);

	//Write difficulty
	snprintf(buffer, sizeof(buffer), "  <levels count=\"%u\">\n", numdifficulties);
	pack_fputs(buffer, fp);
	eof_determine_phrase_status(sp, track);	//Update the tremolo status of each note
	for(ctr = 0, ctr2 = 0; ctr < 4; ctr++)
	{	//For each of the available difficulties
		if(populated[ctr])
		{	//If this difficulty is populated
			//Count the number of single notes and chords in this difficulty
			for(ctr3 = 0, numsinglenotes = 0, numchords = 0; ctr3 < tp->notes; ctr3++)
			{	//For each note in the track
				if(eof_get_note_type(sp, track, ctr3) == ctr)
				{	//If the note is in this difficulty
					unsigned long lanecount = eof_note_count_colors(sp, track, ctr3);	//Count the number of used lanes in this note
					if(lanecount == 1)
					{	//If the note has only one gem
						numsinglenotes++;	//Increment counter
					}
					else if(lanecount > 1)
					{	//If the note has multiple gems
						if(!eof_is_string_muted(sp, track, ctr3))
						{	//If the chord is not fully string muted
							numchords++;	//Increment counter
						}
					}
					else
					{	//Note had no gems, throw error
						allegro_message("Error:  A note with no gems was encountered.  Aborting Rocksmith export");
						eof_log("Error:  A note with no gems was encountered.  Aborting Rocksmith export", 1);
						free(chordlist);
						pack_fclose(fp);
						return 0;	//Return failure
					}
				}
			}

			//Write single notes
			snprintf(buffer, sizeof(buffer), "    <level difficulty=\"%lu\">\n", ctr2);
			pack_fputs(buffer, fp);
			ctr2++;	//Increment the populated difficulty level number
			if(numsinglenotes)
			{	//If there's at least one single note in this difficulty
				snprintf(buffer, sizeof(buffer), "      <notes count=\"%lu\">\n", numsinglenotes);
				pack_fputs(buffer, fp);
				for(ctr3 = 0; ctr3 < tp->notes; ctr3++)
				{	//For each note in the track
					if((eof_get_note_type(sp, track, ctr3) == ctr) && (eof_note_count_colors(sp, track, ctr3) == 1))
					{	//If this note is in this difficulty and is a single note (and not a chord)
						for(stringnum = 0, bitmask = 1; stringnum < tp->numstrings; stringnum++, bitmask <<= 1)
						{	//For each string used in this track
							if((eof_get_note_note(sp, track, ctr3) & bitmask) && ((tp->note[ctr3]->frets[stringnum] & 0x80) == 0))
							{	//If this string is used in this note and it is not fret hand muted
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

								flags = eof_get_note_flags(sp, track, ctr3);
								notepos = eof_get_note_pos(sp, track, ctr3);
								length = eof_get_note_length(sp, track, ctr3);
								if(length == 1)
								{	//In Rocksmith, even a 1ms note is displayed as a sustained note
									length = 0;	//A sustain of 0 prevents that
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
								hopo = (hammeron & pulloff) ? 1 : 0;
								palmmute = (flags & EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE) ? 1 : 0;
								tremolo = (flags & EOF_NOTE_FLAG_IS_TREMOLO) ? 1 : 0;

								if((flags & EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE) == 0)
								{	//At this point, it doesn't seem Rocksmith supports string muted notes
									snprintf(buffer, sizeof(buffer), "        <note time=\"%.3f\" bend=\"%lu\" fret=\"%lu\" hammerOn=\"%d\" harmonic=\"%d\" hopo=\"%d\" ignore=\"0\" palmMute=\"%d\" pullOff=\"%d\" slideTo=\"%ld\" string=\"%lu\" sustain=\"%.3f\" tremolo=\"%d\"/>\n", (double)notepos / 1000.0, bend, fret, hammeron, harmonic, hopo, palmmute, pulloff, slideto, stringnum, (double)length / 1000.0, tremolo);
									pack_fputs(buffer, fp);
								}
							}//If this string is used in this note
						}//For each string used in this track
					}//If this note is in this difficulty and is a single note (and not a chord)
				}//For each note in the track
				pack_fputs("      </notes>\n", fp);
			}//If there's at least one single note in this difficulty
			else
			{	//There are no single notes in this difficulty, write an empty notes tag
				pack_fputs("      <notes count=\"0\"/>\n", fp);
			}

			//Write chords
			if(numchords)
			{	//If there's at least one chord in this difficulty
				unsigned long chordid;
				char *upstrum = "up";
				char *downstrum = "down";
				char *direction;	//Will point to either upstrum or downstrum as appropriate
				double notepos;
				snprintf(buffer, sizeof(buffer), "      <chords count=\"%lu\">\n", numchords);
				pack_fputs(buffer, fp);
				for(ctr3 = 0; ctr3 < tp->notes; ctr3++)
				{	//For each note in the track
					if((eof_get_note_type(sp, track, ctr3) == ctr) && (eof_note_count_colors(sp, track, ctr3) > 1) && !eof_is_string_muted(sp, track, ctr3))
					{	//If this note is in this difficulty and is a chord that isn't fully string muted
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
							free(chordlist);
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
						notepos = (double)tp->note[ctr3]->pos / 1000.0;
						snprintf(buffer, sizeof(buffer), "        <chord time=\"%.3f\" chordId=\"%lu\" highDensity=\"0\" ignore=\"0\" strum=\"%s\"/>\n", notepos, chordid, direction);
						pack_fputs(buffer, fp);
					}//If this note is in this difficulty and is a chord
				}//For each note in the track
				pack_fputs("      </chords>\n", fp);
			}
			else
			{	//There are no chords in this difficulty, write an empty chords tag
				pack_fputs("      <chords count=\"0\"/>\n", fp);
			}

			//Write other stuff
			pack_fputs("      <fretHandMutes count=\"0\"/>\n", fp);

			//Write anchors (fret hand positions)
			unsigned long anchorcount;
			for(ctr3 = 0, anchorcount = 0; ctr3 < tp->handpositions; ctr3++)
			{	//For each hand position defined in the track
				if(tp->handposition[ctr3].difficulty == ctr)
				{	//If the hand position is in this difficulty
					anchorcount++;
				}
			}
			if(anchorcount)
			{	//If there's at least one anchor in this difficulty
				snprintf(buffer, sizeof(buffer), "      <anchors count=\"%lu\">\n", anchorcount);
				pack_fputs(buffer, fp);
				for(ctr3 = 0, anchorcount = 0; ctr3 < tp->handpositions; ctr3++)
				{	//For each hand position defined in the track
					if(tp->handposition[ctr3].difficulty == ctr)
					{	//If the hand position is in this difficulty
						snprintf(buffer, sizeof(buffer), "        <anchor time=\"%.3f\" fret=\"%lu\"/>\n", (double)tp->handposition[ctr3].start_pos / 1000.0, tp->handposition[ctr3].end_pos);
						pack_fputs(buffer, fp);
					}
				}
				pack_fputs("      </anchors>\n", fp);
			}
			else
			{	//There are no anchors in this difficulty, write an empty anchors tag
				pack_fputs("      <anchors count=\"0\"/>\n", fp);
			}

			//Write other stuff
			pack_fputs("      <handShapes count=\"0\"/>\n", fp);
			pack_fputs("    </level>\n", fp);
		}//If this difficulty is populated
	}//For each of the available difficulties
	pack_fputs("  </levels>\n", fp);
	pack_fputs("</song>\n", fp);
	pack_fclose(fp);
	free(chordlist);

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
	char string_finger_defined = 0, string_finger_undefined = 0;

	if(!tp || (note >= tp->notes))
		return 0;	//Invalid parameters

	for(ctr = 0, bitmask = 1; ctr < tp->numstrings; ctr++, bitmask <<= 1)
	{	//For each string supported by this track
		if(tp->note[note]->note & bitmask)
		{	//If this string is used
			if(tp->note[note]->frets[ctr] != 0)
			{	//If the string isn't being played open
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
				else
				{	//This string does not have a finger definition, which is correct
					string_finger_defined = 1;	//Track it as if it was a string with valid fingering
				}
			}
		}
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
