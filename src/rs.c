#include "beat.h"
#include "main.h"
#include "midi.h"
#include "rs.h"
#include <time.h>

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

int eof_export_rocksmith_track(EOF_SONG * sp, char * fn, unsigned long track)
{
	eof_log("eof_export_rocksmith() entered", 1);

	PACKFILE * fp;
	char buffer[256] = {0};
	time_t seconds;		//Will store the current time in seconds
	struct tm *caltime;	//Will store the current time in calendar format
	unsigned long ctr, ctr2, ctr3, numsections = 0, stringnum, bitmask;
	EOF_PRO_GUITAR_TRACK *tp;

	if(!sp || !fn || !sp->beats || (track >= sp->tracks) || (sp->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT) || !sp->track[track]->name)
	{
		eof_log("\tError saving:  Invalid parameters", 1);
		return 0;	//Return failure
	}

	//Count the number of populated difficulties in the track
	unsigned long diffnotes[4] = {0};	//Will track the number of notes in each difficulty
	char populated[4] = {0};	//Will track which difficulties are populated
	unsigned notetype;
	unsigned numdifficulties;
	for(ctr = 0; ctr < eof_get_track_size(sp, track); ctr++)
	{	//For each note in the track
		notetype = eof_get_note_type(sp, track, ctr);
		if(notetype < 4)
		{	//As long as this is Easy, Medium, Hard or Expert difficulty
			diffnotes[notetype]++;	//Increment this difficulty's note counter
			populated[notetype] = 1;	//Track that this difficulty is populated
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
			numsections++;	//Update counter
		}
	}
	if(numsections)
	{	//If there is at least one section event
		snprintf(buffer, sizeof(buffer), "  <phrases count=\"%lu\">\n", numsections);
		pack_fputs(buffer, fp);
		for(ctr = 0; ctr < sp->beats; ctr++)
		{	//For each beat in the chart
			if(sp->beat[ctr]->contained_section_event >= 0)
			{	//If this beat has a section event
				snprintf(buffer, sizeof(buffer), "    <phrase disparity=\"0\" ignore=\"0\" maxDifficulty=\"0\" name=\"%s\" solo=\"0\"/>\n", sp->text_event[sp->beat[ctr]->contained_section_event]->text);
				pack_fputs(buffer, fp);
			}
		}
		pack_fputs("  </phrases>\n", fp);
		snprintf(buffer, sizeof(buffer), "  <phraseIterations count=\"%lu\">\n", numsections);
		pack_fputs(buffer, fp);
		for(ctr = 0, ctr2 = 0; ctr < sp->beats; ctr++)
		{	//For each beat in the chart
			if(sp->beat[ctr]->contained_section_event >= 0)
			{	//If this beat has a section event
				snprintf(buffer, sizeof(buffer), "    <phraseIteration time=\"%.3f\" phraseId=\"%lu\"/>\n", sp->beat[ctr]->fpos, ctr2);
				pack_fputs(buffer, fp);
				ctr2++;
			}
		}
		pack_fputs("  </phraseIterations>\n", fp);
	}
	else
	{	//There are no sections, write an emtpy tag
		pack_fputs("  <phrases count=\"0\"/>\n", fp);
	}

	//Write some unknown information
	pack_fputs("  <linkedDiffs count=\"0\"/>\n", fp);
	pack_fputs("  <phraseProperties count=\"0\"/>\n", fp);
	pack_fputs("  <chordTemplates count=\"0\"/>\n", fp);
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

	//Write note information
	snprintf(buffer, sizeof(buffer), "  <levels count=\"%u\">\n", numdifficulties);
	pack_fputs(buffer, fp);
	eof_determine_phrase_status(sp, track);	//Update the tremolo status of each note
	tp = sp->pro_guitar_track[sp->track[track]->tracknum];
	for(ctr = 0, ctr2 = 0; ctr < 4; ctr++)
	{	//For each of the available difficulties
		if(populated[ctr])
		{	//If this difficulty is populated
			snprintf(buffer, sizeof(buffer), "    <level difficulty=\"%lu\">\n", ctr2);
			pack_fputs(buffer, fp);
			ctr2++;	//Increment the difficulty level number
			snprintf(buffer, sizeof(buffer), "      <notes count=\"%lu\">\n", diffnotes[ctr]);
			pack_fputs(buffer, fp);
			for(ctr3 = 0; ctr3 < eof_get_track_size(sp, track); ctr3++)
			{	//For each note in the track
				for(stringnum = 0, bitmask = 1; stringnum < tp->numstrings; stringnum++, bitmask <<= 1)
				{	//For each string used in this track
					if(eof_get_note_note(sp, track, ctr3) & bitmask)
					{	//If this string is used in this note
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
						hammeron = flags | EOF_PRO_GUITAR_NOTE_FLAG_HO;
						pulloff = flags | EOF_PRO_GUITAR_NOTE_FLAG_PO;
						harmonic = flags | EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC;
						hopo = hammeron | pulloff;
						palmmute = flags | EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE;
						tremolo = flags | EOF_NOTE_FLAG_IS_TREMOLO;

						if(((flags & EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE) == 0) && fret)
						{	//At this point, it doesn't seem Rocksmith supports string muted notes
							snprintf(buffer, sizeof(buffer), "        <note time=\"%.3f\" bend=\"%lu\" fret=\"%lu\" hammerOn=\"%d\" harmonic=\"%d\" hopo=\"%d\" ignore=\"0\" palmMute=\"%d\" pullOff=\"%d\" slideTo=\"%ld\" string=\"%lu\" sustain=\"%.3f\" tremolo=\"%d\"/>\n", (double)notepos / 1000.0, bend, fret, hammeron, harmonic, hopo, palmmute, pulloff, slideto, stringnum, (double)length / 1000.0, tremolo);
							pack_fputs(buffer, fp);
						}
					}//If this string is used in this note
				}//For each string used in this track
			}//For each note in the track
			pack_fputs("      <chords count=\"0\"/>\n", fp);
			pack_fputs("      <fretHandMutes count=\"0\"/>\n", fp);
			pack_fputs("      <anchors count=\"0\"/>\n", fp);
			pack_fputs("      <handShapes count=\"0\"/>\n", fp);
			pack_fputs("    </level>\n", fp);
		}//If this difficulty is populated
	}//For each of the available difficulties
	pack_fputs("  </levels>\n", fp);
	pack_fputs("</song>\n", fp);
	pack_fclose(fp);

	return 1;	//Return success
}
