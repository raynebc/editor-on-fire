#include <allegro.h>
#include "chart_import.h"			//For FindLongestLineLength_ALLEGRO()
#include "foflc/Lyric_storage.h"	//For strcasestr_spec()
#include "foflc/RS_parse.h"			//For XML parsing functions
#include "main.h"
#include "midi.h"					//For eof_apply_ts()
#include "rs.h"
#include "rs_import.h"
#include "undo.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

#define RS_IMPORT_DEBUG

#define EOF_RS_PHRASE_IMPORT_LIMIT 200
#define EOF_RS_EVENT_IMPORT_LIMIT 200
#define EOF_RS_CHORD_TEMPLATE_IMPORT_LIMIT 400

int eof_parse_chord_template(char *name, size_t size, char *finger, char *frets, unsigned char *note, unsigned char *isarp, unsigned long linectr, char *input)
{
	unsigned long ctr, bitmask;
	long output = 0;
	int success_count;
	unsigned char arpfound = 0;

	if(!name || !size || !finger || !frets || !note || !input)
	{	//Note:  isarp is allowed to be NULL
		eof_log("Invalid parameters sent to eof_parse_chord_template()", 1);
		return 1;	//Return error
	}

	//Read the chord's display name to determine if it refers to arpeggio notation
	if(parse_xml_attribute_text(name, size, "displayName", input))
	{	//If the display name was successfully read
		if(strcasestr_spec(name, "-arp"))
		{	//If the display name contains -arp
			arpfound = 1;
		}
	}
	if(isarp)
	{	//If this pointer isn't NULL
		*isarp = arpfound;
	}

	//Read chord name
	if(!parse_xml_attribute_text(name, size, "chordName", input))
	{	//If the chord name could not be read
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading chord name on line #%lu.  Aborting", linectr);
		eof_log(eof_log_string, 1);
		return 2;	//Return error
	}

	//Read chord fingerings, if any are present
	output = -1;		//If the attribute is not parsed it will be considered unused
	(void) parse_xml_attribute_number("finger0", input, &output);	//Read the fingering for string 0
	finger[0] = output;	//Store the fingering

	output = -1;		//If the attribute is not parsed it will be considered unused
	(void) parse_xml_attribute_number("finger1", input, &output);	//Read the fingering for string 1
	finger[1] = output;	//Store the fingering

	output = -1;		//If the attribute is not parsed it will be considered unused
	(void) parse_xml_attribute_number("finger2", input, &output);	//Read the fingering for string 2
	finger[2] = output;	//Store the fingering

	output = -1;		//If the attribute is not parsed it will be considered unused
	(void) parse_xml_attribute_number("finger3", input, &output);	//Read the fingering for string 3
	finger[3] = output;	//Store the fingering

	output = -1;		//If the attribute is not parsed it will be considered unused
	(void) parse_xml_attribute_number("finger4", input, &output);	//Read the fingering for string 4
	finger[4] = output;	//Store the fingering

	output = -1;		//If the attribute is not parsed it will be considered unused
	(void) parse_xml_attribute_number("finger5", input, &output);	//Read the fingering for string 5
	finger[5] = output;	//Store the fingering

	for(ctr = 0; ctr < 6; ctr++)
	{	//For each of the 6 supported strings
		if(finger[ctr] == 0)
		{	//If the fingering was given as zero, it is the thumb
			finger[ctr] = 5;	//Convert to EOF's notation for thumb
		}
		if(finger[ctr] < 0)
		{	//If the fingering was given as a negative number, it is unused
			finger[ctr] = 0;	//Convert to EOF's notation for an unused/undefined finger
		}
	}

	//Read chord fretting
	success_count = 0;
	output = -1;		//If the attribute is not parsed it will be considered unused
	success_count += parse_xml_attribute_number("fret0", input, &output);	//Read the fretting for string 0
	frets[0] = output;	//Store the fretting

	output = -1;		//If the attribute is not parsed it will be considered unused
	success_count += parse_xml_attribute_number("fret1", input, &output);	//Read the fretting for string 1
	frets[1] = output;	//Store the fretting

	output = -1;		//If the attribute is not parsed it will be considered unused
	success_count += parse_xml_attribute_number("fret2", input, &output);	//Read the fretting for string 2
	frets[2] = output;	//Store the fretting

	output = -1;		//If the attribute is not parsed it will be considered unused
	success_count += parse_xml_attribute_number("fret3", input, &output);	//Read the fretting for string 3
	frets[3] = output;	//Store the fretting

	output = -1;		//If the attribute is not parsed it will be considered unused
	success_count += parse_xml_attribute_number("fret4", input, &output);	//Read the fretting for string 4
	frets[4] = output;	//Store the fretting

	output = -1;		//If the attribute is not parsed it will be considered unused
	success_count += parse_xml_attribute_number("fret5", input, &output);	//Read the fretting for string 5
	frets[5] = output;	//Store the fretting

	if(success_count < 1)
	{	//If not even one fret value was defined for this chord
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading chord fretting on line #%lu.  Aborting", linectr);
		eof_log(eof_log_string, 1);
		return 4;	//Return error
	}
	for(ctr = 0, *note = 0, bitmask = 1; ctr < 6; ctr++, bitmask <<= 1)
	{	//For each of the 6 supported strings
		if(frets[ctr] < 0)
		{	//If the fingering was given as a negative number, it is unused
			frets[ctr] = 0;	//Convert to EOF's notation for an unused/undefined finger
		}
		else
		{	//Otherwise the string is used
			*note |= bitmask;	//Set the mask bit for this string
		}
	}

	return 0;	//Return success
}

EOF_PRO_GUITAR_NOTE *eof_rs_import_note_tag_data(char *buffer, int function, EOF_PRO_GUITAR_TRACK *tp, unsigned long linectr, unsigned char curdiff)
{
	long time = 0, step = 0;
	long bend = 0, fret = 0, hammeron = 0, harmonic = 0, palmmute = 0, pulloff = 0, string = 0, sustain = 0, tremolo = 0, linknext = 0, accent = 0, ignore = 0, mute = 0, pinchharmonic = 0, tap = 0, vibrato = 0;
	long pluck = -1, slap = -1, slideto = -1, slideunpitchto = -1;
	unsigned long flags = 0;
	static EOF_PRO_GUITAR_NOTE *np = NULL;
	EOF_PRO_GUITAR_NOTE *tnp = NULL;
	char *ptr;

	if(!buffer || !tp)
		return NULL;	//Invalid parameters

	//Read note tag
	ptr = strcasestr_spec(buffer, "<note ");
	if(!ptr)
	{	//If it is not a note tag, check if it's a chordnote tag
		ptr = strcasestr_spec(buffer, "<chordNote ");
	}

	if(ptr)
	{	//If this is a note or chordnote tag
		//Read note attributes
		if(!parse_xml_rs_timestamp("time", buffer, &time))
		{	//If the timestamp was not readable
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading start timestamp on line #%lu.  Aborting", linectr);
			eof_log(eof_log_string, 1);
			return NULL;
		}
		(void) parse_xml_attribute_number("bend", buffer, &bend);
		(void) parse_xml_attribute_number("fret", buffer, &fret);
		if(fret >= tp->capo)
			fret -= tp->capo;	//Apply the capo if applicable
		(void) parse_xml_attribute_number("hammerOn", buffer, &hammeron);
		(void) parse_xml_attribute_number("harmonic", buffer, &harmonic);
		(void) parse_xml_attribute_number("palmMute", buffer, &palmmute);
		(void) parse_xml_attribute_number("pluck", buffer, &pluck);
		(void) parse_xml_attribute_number("pullOff", buffer, &pulloff);
		(void) parse_xml_attribute_number("slap", buffer, &slap);
		(void) parse_xml_attribute_number("slideTo", buffer, &slideto);
		if(slideto >= tp->capo)
			slideto -= tp->capo;	//Apply the capo if applicable
		(void) parse_xml_attribute_number("string", buffer, &string);
		if(!parse_xml_rs_timestamp("sustain", buffer, &sustain))
		{	//If the timestamp was not readable
			sustain = 0;	//Assume no sustain is used
		}
		(void) parse_xml_attribute_number("tremolo", buffer, &tremolo);

		//Read RS2 note attributes
		(void) parse_xml_attribute_number("linkNext", buffer, &linknext);
		(void) parse_xml_attribute_number("accent", buffer, &accent);
		(void) parse_xml_attribute_number("ignore", buffer, &ignore);
		(void) parse_xml_attribute_number(" mute", buffer, &mute);	//Include the leading space to avoid reading the value of the "PalmMute" attribute, which contains this substring at the end of the attribute name
		(void) parse_xml_attribute_number("harmonicPinch", buffer, &pinchharmonic);
		(void) parse_xml_attribute_number("slideUnpitchTo", buffer, &slideunpitchto);
		if(slideunpitchto >= tp->capo)
			slideunpitchto -= tp->capo;	//Apply the capo if applicable
		(void) parse_xml_attribute_number("tap", buffer, &tap);
		(void) parse_xml_attribute_number("vibrato", buffer, &vibrato);

		//Add note and set attributes
		if((string >= 0) && (string < 6))
		{	//As long as the string number is valid
			np = eof_pro_guitar_track_add_note(tp);	//Allocate, initialize and add the new note to the note array
			if(!np)
			{
				eof_log("\tError allocating memory.", 1);
				return NULL;
			}
			if(!function)
			{	//If this note isn't to be retained in the note array
				tp->notes--;
				tp->pgnotes--;
			}
			np->type = curdiff;
			np->note = 1 << (unsigned long) string;
			np->frets[(unsigned long) string] = fret;
			np->pos = time;
			np->length = sustain;
			if(bend)
			{
				flags |= EOF_PRO_GUITAR_NOTE_FLAG_BEND;
				flags |= EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION;
				np->bendstrength = bend;
			}
			if(hammeron)
				flags |= EOF_PRO_GUITAR_NOTE_FLAG_HO;
			if(harmonic)
				flags |= EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC;
			if(palmmute)
				flags |= EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE;
			if(pulloff)
				flags |= EOF_PRO_GUITAR_NOTE_FLAG_PO;
			if(tremolo)
				flags |= EOF_NOTE_FLAG_IS_TREMOLO;
			if(pluck > 0)
				flags |= EOF_PRO_GUITAR_NOTE_FLAG_POP;
			if(slap > 0)
				flags |= EOF_PRO_GUITAR_NOTE_FLAG_SLAP;
			if(slideto > 0)
			{
				flags |= EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION;
				np->slideend = slideto;
				if(slideto > fret)
				{	//The slide ends above the starting fret
					flags |= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP;
				}
				else
				{	//The slide ends below the starting fret
					flags |= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN;
				}
			}
			if(linknext)
				flags |= EOF_PRO_GUITAR_NOTE_FLAG_LINKNEXT;
			if(accent)
				flags |= EOF_PRO_GUITAR_NOTE_FLAG_ACCENT;
			if(ignore)
				np->eflags |= EOF_PRO_GUITAR_NOTE_EFLAG_IGNORE;
			if(mute)
				flags |= EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE;
			if(pinchharmonic)
				flags |= EOF_PRO_GUITAR_NOTE_FLAG_P_HARMONIC;
			if(slideunpitchto > 0)
			{
				flags |= EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE;
				np->unpitchend = slideunpitchto;
			}
			if(tap)
				flags |= EOF_PRO_GUITAR_NOTE_FLAG_TAP;
			if(vibrato)
				flags |= EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO;
			np->flags = flags;

			if(fret > tp->numfrets)
				tp->numfrets = fret;	//Track the highest used fret number

			return np;
		}//As long as the string number is valid
	}//If this is a note tag

	//Read bendValue tag
	ptr = strcasestr_spec(buffer, "<bendValue ");
	if(!ptr || !np)
		return NULL;	//If this isn't a bendValue tag or a normal note tag wasn't previously read, the tag hasn't been recognized

	if(!parse_xml_rs_timestamp("time", buffer, &time))
	{	//If the timestamp was not readable
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading start timestamp on line #%lu.  Aborting", linectr);
		eof_log(eof_log_string, 1);
		return NULL;
	}
	if(!parse_xml_rs_timestamp("step", buffer, &step))
	{	//If the bend strength was not readable
		step = 0;	//A strength of 0 is assumed
	}
	tnp = eof_pro_guitar_track_add_tech_note(tp);
	if(!tnp)
	{
		eof_log("\tError allocating memory.  Aborting", 1);
		return NULL;
	}
	if(!function)
	{	//If this note isn't to be retained in the note array
		tp->technotes--;
	}
	tnp->type = np->type;	//This bend tech note will use the same difficulty and string as the note it is applied to
	tnp->note = np->note;
	tnp->pos = time;
	tnp->length = 1;
	tnp->flags |= EOF_PRO_GUITAR_NOTE_FLAG_BEND;
	tnp->flags |= EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION;
	if(step % 1000)
	{	//If the bend strength was defined in quarter steps
		tnp->bendstrength = 0x80 | (step / 500);
	}
	else
	{	//The bend strength was defined in half steps
		tnp->bendstrength = step / 1000;
	}

	///DEBUG
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tPlaced bend tech note at %lums", tnp->pos);
	eof_log(eof_log_string, 1);

	return tnp;
}

unsigned long eof_evaluate_rs_import_gap_solution(EOF_TECHNOTE_GAP *gaps, unsigned long numgaps, unsigned *solution, unsigned long solutionsize)
{
	unsigned long ctr, ctr2, count, lowestgapsize = ULONG_MAX, gapsize;

	if(!gaps || !solution)
		return ULONG_MAX;	//Invalid parameters

	for(ctr = 0; ctr < numgaps; ctr++)
	{	//For each gap in the problem
		for(ctr2 = 0, count = 0; ctr2 < solutionsize; ctr2++)
		{	//For each technote in the problem
			if(solution[ctr2] == ctr)
			{	//If this technote is in the gap being examined
				count++;
			}
		}
		gapsize = (double)gaps[ctr].size / (double)(count + 1) + 0.5;	//Determine how far apart each technote in this gap would be spaced, round up
		if(gapsize < lowestgapsize)
		{
			lowestgapsize = gapsize;	//Track the smallest gap size
		}
	}

	return lowestgapsize;
}

char eof_rs_import_process_chordnotes(EOF_PRO_GUITAR_TRACK *tp, EOF_PRO_GUITAR_NOTE *np, EOF_PRO_GUITAR_NOTE **chordnote, unsigned long chordnotectr, unsigned long numtechnotes)
{
	unsigned long cflags;	//The flags that all the chordnotes have in common
	unsigned long ceflags;	//The eflags that all the chordnotes have in common
	unsigned neededtechnotes = 0;
	unsigned long ctr, ctr2;
	char error = 0;
	unsigned char prebends[7] = {0};	//Tracks how many chordnotes use each of the valid pre-bend strengths
	long prebend_index;
	unsigned char dominant_prebend = 0, dominant_prebend_count = 0;	//Used to track the most commonly used pre-bend strength for the specified chord

	if(!np || !chordnote || !chordnotectr || (chordnotectr >= 7))
		return 1;	//Invalid parameters

	//Examine chordnote lengths to detect stop and sustain statuses
	for(ctr = 0; ctr < chordnotectr; ctr++)
	{	//For each chordnote that was parsed
		if(!chordnote[ctr]->length)
			continue;	//If this chordnote has a zero length, skip it

		if(chordnote[ctr]->length < np->length)
		{	//If the chordnote was shorter than any of the other chordnotes, add a stop tech note to define this
			EOF_PRO_GUITAR_NOTE *tnp = eof_pro_guitar_track_add_tech_note(tp);
			if(!tnp)
			{
				eof_log("\tError allocating memory for a stop tech note.  Aborting", 1);
				error = 1;
				break;	//Break from loop
			}
			tnp->pos = np->pos + chordnote[ctr]->length;	//The stop note is placed where this chordnote ends
			tnp->type = np->type;
			tnp->note = 1 << ctr;
			tnp->eflags |= EOF_PRO_GUITAR_NOTE_EFLAG_STOP;
			numtechnotes++;	//Track that a technote was added

			///DEBUG
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tPlaced stop tech note at %lums", tnp->pos);
			eof_log(eof_log_string, 1);
		}
		if(!(chordnote[ctr]->flags & (EOF_PRO_GUITAR_NOTE_FLAG_BEND | EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP | EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN | EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE  | EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO | EOF_NOTE_FLAG_IS_TREMOLO)))
		{	//If none of the techniques that normally warrant a chordnote's sustain to be kept are in use for the note
			chordnote[ctr]->eflags |= EOF_PRO_GUITAR_NOTE_EFLAG_SUSTAIN;	//Consider it as having sustain status
		}
	}

	if(!error)
	{	//If the chordnote lengths were processed
		//Examine chordnote flaqs to determine which statuses are used in all of the chord's chordnotes
		long slideamount = 0;	//The number of frets the first chordnote slides (pitched)
		long uslideamount = 0;	//The number of frets the first chordnote slides (unpitched)

		cflags = chordnote[0]->flags;	//Initialize these variables with the first chordnote's flags
		ceflags = chordnote[0]->eflags;
		if((cflags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP) || (cflags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN))
		{	//If the first chord note slides (pitched)
			slideamount = chordnote[0]->slideend - eof_pro_guitar_note_lowest_fret_np(chordnote[0]);
		}
		if(cflags & EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE)
		{	//If the first chord note slides (unpitched)
			uslideamount = chordnote[0]->unpitchend - eof_pro_guitar_note_lowest_fret_np(chordnote[0]);
		}
		for(ctr = 1; ctr < chordnotectr; ctr++)
		{	//For each of the chordnotes after the first
			cflags &= chordnote[ctr]->flags;	//Clear any flags that this chordnote doesn't also have from the common flags variable
			ceflags &= chordnote[ctr]->eflags;
			if((chordnote[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP) || (chordnote[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN))
			{	//If this chordnote slides (pitched)
				if(slideamount != chordnote[ctr]->slideend - eof_pro_guitar_note_lowest_fret_np(chordnote[ctr]))
				{	//If this chordnote slides a different amount than the first chordnote
					cflags &= ~EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP;		//They don't have a slide in common
					cflags &= ~EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN;
				}
			}
			if(chordnote[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE)
			{	//If this chordnote slides (unpitched)
				if(uslideamount != chordnote[ctr]->unpitchend - eof_pro_guitar_note_lowest_fret_np(chordnote[ctr]))
				{	//If this chordnote unpitched slides a different amount than the first chordnote
					cflags &= ~EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE;	//They don't have an unpitched slide in common
				}
			}
		}

		//Examine pre-bend information
		//Determine which pre-bend strength is the most used one on this chordnote
		eof_pro_guitar_track_sort_notes(tp);		//Ensure both note sets are sorted
		eof_pro_guitar_track_sort_tech_notes(tp);
		for(ctr = 0; ctr < chordnotectr; ctr++)
		{	//For each of the chordnotes
			prebend_index = eof_pro_guitar_note_bitmask_has_pre_bend_tech_note_ptr(tp, chordnote[ctr], chordnote[ctr]->note);
			if(prebend_index >= 0)
			{	//If this chordnote has an applicable pre-bend technote
				unsigned char strength = tp->technote[prebend_index]->bendstrength;

				if(strength > 6)
					strength = 6;	//For the sake of this logic, cap the pre-bend at 6 quarter steps, which is the highest that Rocksmith supports
				prebends[strength]++;	//Track how many times each of the bend strengths are used, in order to determine the strength used by the most chordnotes in this chord
			}
		}
		for(ctr = 0; ctr < 7; ctr++)
		{	//For each of the valid pre-bend strengths
			if(prebends[ctr] > dominant_prebend_count)
			{	//If this strength was used more than the previous one being tracked
				dominant_prebend = ctr;					//Track the pre-bend strength used by a plurality of this chord's chordnotes
				dominant_prebend_count = prebends[ctr];
			}
		}
		//Any pre-bend that is different from the dominant pre-bend strength needs to be recorded as a pre-bend tech note
		for(ctr = 0; ctr < chordnotectr; ctr++)
		{	//For each of the chordnotes
			prebend_index = eof_pro_guitar_note_bitmask_has_pre_bend_tech_note_ptr(tp, chordnote[ctr], chordnote[ctr]->note);
			if(prebend_index >= 0)
			{	//If this chordnote has an applicable pre-bend technote
				unsigned char strength = tp->technote[prebend_index]->bendstrength;

				prebends[ctr] = strength;	//Replace prebends[] with the strength of each pre-bend
				if(strength != dominant_prebend)
				{	//If this string will require a pre-bend tech note
					chordnote[ctr]->eflags |= EOF_PRO_GUITAR_NOTE_EFLAG_PRE_BEND;
					chordnote[ctr]->flags &= ~ EOF_PRO_GUITAR_NOTE_FLAG_BEND;	//Remove this flag so the normal bend logic doesn't interfere with pre-bend logic
				}
			}
		}

		//Move any flags that all chordnotes had in common from the chordnote flags to the chord itself
		// and transfer leftover statuses to tech notes already applied to the chord where possible
		np->flags |= cflags;
		np->eflags |= ceflags;
		if(slideamount)
		{	//If an end of slide position is being transferred
			np->slideend = eof_pro_guitar_note_lowest_fret_np(np) + slideamount;
		}
		if(uslideamount)
		{	//If an end of unpitched slide position is being transferred
			np->unpitchend = eof_pro_guitar_note_lowest_fret_np(np) + uslideamount;
		}
		for(ctr = 0; ctr < chordnotectr; ctr++)
		{	//For each chordnote that was parsed
			chordnote[ctr]->flags &= ~cflags;	//Clear any of the flags that were moved to the chord
			chordnote[ctr]->eflags &= ~ceflags;

			if(chordnote[ctr]->flags || chordnote[ctr]->eflags)
			{	//If any of this chordnote's flags are still set
				unsigned long tnnum = 0;

				if(eof_pro_guitar_note_bitmask_has_tech_note(tp, tp->notes - 1, chordnote[ctr]->note, &tnnum) && !(chordnote[ctr]->eflags & EOF_PRO_GUITAR_NOTE_EFLAG_PRE_BEND))
				{	//If the chord being parsed already has a technote on this string due to having added bend or stop tech notes, but only as long as pre-bend status isn't being applied
					tp->technote[tnnum]->flags |= chordnote[ctr]->flags;	//Transfer the flags to it instead of creating another tech note
					tp->technote[tnnum]->eflags |= chordnote[ctr]->eflags;
					chordnote[ctr]->flags = chordnote[ctr]->eflags = 0;
				}
				else
				{
					neededtechnotes++;	//It will require a technote to apply to that individual gem in the chord
				}
			}
		}

		//Create technotes to store any remaining statuses
		if(neededtechnotes)
		{	//If there are technotes remaining to be placed
			EOF_TECHNOTE_GAP *gaps;	//Stores information about the spacing between the hard-coded points of this chord (its start and end points and fixed position stop/bend tech notes)
			EOF_TECHNOTE_GAP *gaps2;	//Used to rebuild the gaps array to remove zero sized gaps (caused by bend tech notes that can be at the same timestamp for multiple strings)
			unsigned *solution;		//Stores a possible solution indicating which gap number each of the remaining technotes is to be placed into
			unsigned long solutionval;	//The smallest amount of time between any technote in the solution and another obstacle (fixed position technote or chord start/end)
			unsigned *bestsolution;		//Stores the solution that was evaluated with the largest buffer of time between each of the remaining technotes
			unsigned long bestsolutionval;	//The value of the best evaluated solution (higher is better)
			unsigned long numgaps = numtechnotes + 1;	//The number of gaps within which the remaining technotes may be placed
			unsigned long numgaps2;	//The number of gaps existing when the gaps array is rebuilt
			unsigned long tnnum = 0, lasttechpos;
			int done = 0;

			//Ensure the chord is long enough to encompass all its technotes
			if(numtechnotes + neededtechnotes > 1)
			{	//If the chord requires more than a total of 1 tech note
				if(np->length / (numtechnotes + neededtechnotes + 1) < 1)
				{	//If there's not enough space on this chord to place technotes with a 1ms buffer on each side, lengthen the chord
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\t\tWarning:  Chord at %lums is too short to place necessary tech notes.  Adjusting to %lums length.", np->pos, numtechnotes + neededtechnotes + 1);
					eof_log(eof_log_string, 1);
					np->length = numtechnotes + neededtechnotes + 1;
				}
			}

			//Build an array defining available spaces between the existing technotes on this chord
			gaps = malloc(sizeof(EOF_TECHNOTE_GAP) * numgaps);
			gaps2 = malloc(sizeof(EOF_TECHNOTE_GAP) * numgaps);

			//Build one array each to store the solution currently being tested and the best examined solution
			solution = malloc(sizeof(unsigned) * neededtechnotes);
			bestsolution = malloc(sizeof(unsigned) * neededtechnotes);
			if(!gaps || !gaps2 || !solution || !bestsolution)
			{	//If the array was not allocated
				if(gaps)
					free(gaps);
				if(gaps2)
					free(gaps2);
				if(solution)
					free(solution);
				if(bestsolution)
					free(bestsolution);
				eof_log("\tError allocating memory for technote placement logic.  Aborting", 1);
				error = 1;
			}
			if(!error)
			{	//If the solution arrays were created
				memset(gaps, 0, sizeof(EOF_TECHNOTE_GAP) * numgaps);
				memset(gaps2, 0, sizeof(EOF_TECHNOTE_GAP) * numgaps);
				memset(solution, 0, sizeof(unsigned) * neededtechnotes);
				memset(bestsolution, 0, sizeof(unsigned) * neededtechnotes);
				lasttechpos = np->pos;	//The first gap will begin at the beginning of the chord
				gaps[numgaps - 1].stop = np->pos + np->length;	//The last gap will end at the end of the chord
				if(eof_pro_guitar_note_bitmask_has_tech_note(tp, tp->notes - 1, np->note, &tnnum))
				{	//If there's at least one technote overlapping this chord
					EOF_PRO_GUITAR_NOTE *tnp = tp->technote[tnnum];
					unsigned long gapnum = 0;
					long nexttn;
					while(tnp)
					{	//While the pointer refers to a technote that overlaps the chord being processed
						if(gapnum >= numgaps)
						{	//If more tech notes were encountered than expected
							eof_log("\tError processing technote gaps.  Aborting", 1);
							error = 1;
							break;	//Break from loop
						}
						gaps[gapnum].start = lasttechpos;	//The previous gap's ending is the start of this gap
						gaps[gapnum].stop = tnp->pos;		//This tech note marks the ending of this gap
						lasttechpos = tnp->pos;				//As well as the beginning of the next gap
						gapnum++;

						//Iterate to the next technote that overlaps the chord being processed, if any
						nexttn = eof_fixup_next_pro_guitar_technote(tp, tnnum);
						if((nexttn > 0) && (tp->technote[nexttn]->pos >= np->pos) && (tp->technote[nexttn]->pos <= np->pos + np->length))
						{	//If there is another technote in this track difficulty, and it overlaps the chord being processed
							tnp = tp->technote[nexttn];
							tnnum = nexttn;
						}
						else
						{
							if(gapnum < numgaps)
							{	//Bounds check
								gaps[gapnum].start = lasttechpos;	//The last tech note is the beginning of the final gap
							}
							tnp = NULL;
						}
					}
				}
				else
				{	//No technotes were found, initialize the array to reflect one gap spanning the entire chord length
					if(numtechnotes)
					{	//If the expected number of technotes was greater than 0
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\t\tError:  Cannot find tech notes for chord at %lums.  Aborting", np->pos);
						eof_log(eof_log_string, 1);
						error = 1;
					}
					gaps[0].start = np->pos;
					gaps[0].stop = np->pos + np->length;
				}
				if(!error)
				{	//If the gaps array was initialized
					for(ctr = 0; ctr < numgaps; ctr++)
					{	//For each of the gaps in the array
						gaps[ctr].size = gaps[ctr].stop - gaps[ctr].start;	//Set the gap size variable
					}

					//Rebuild the gaps array to remove all zero sized gaps
					for(ctr = 0, ctr2 = 0, numgaps2 = 0; ctr < numgaps; ctr++)
					{	//For each of the gaps in this array
						if(gaps[ctr].size)
						{	//If this gap size is nonzero
							gaps2[ctr2++] = gaps[ctr];	//Copy this into the new gaps array
							numgaps2++;					//Keep count of the new gaps array size
						}
					}
					free(gaps);			//Destroy the old gaps array
					gaps = gaps2;		//Use the new one instead
					numgaps = numgaps2;
					gaps2 = NULL;

					//Test the default solution of all technotes being placed in the first gap
					bestsolutionval = eof_evaluate_rs_import_gap_solution(gaps, numgaps, solution, neededtechnotes);

					//Until all solutions have been examined, build a new solution to be tested
					while(!done)
					{
						unsigned elementnum = 0;

						//Increment the solution
						solution[0]++;	//Change the gap number this technote will use
						while(solution[elementnum] >= numgaps)
						{	//If this element is overflowed, reset and increment the next element of the solution
							solution[elementnum] = 0;
							elementnum++;	//Examine the next element
							if(elementnum >= neededtechnotes)
							{	//If all elements have been exhausted
								done = 1;	//All permutations have been tested, set condition to break from outer while loop
								break;		//Break from inner while loop
							}
							solution[elementnum]++;	//Increment this element
						}
						if(done)
							break;

						//Evaluate the new solution and keep it if it is better than the previous best solution
						solutionval = eof_evaluate_rs_import_gap_solution(gaps, numgaps, solution, neededtechnotes);
						if(solutionval > bestsolutionval)
						{	//If this new solution is better
							memcpy(bestsolution, solution, sizeof(unsigned) * neededtechnotes);	//Clone the solution
							bestsolutionval = solutionval;
						}
					}

					//Count how many technotes will be stored into each gap as part of the best solution
					for(ctr = 0; ctr < neededtechnotes; ctr++)
					{	//For each technote to be placed
						gaps[bestsolution[ctr]].capacity++;
					}

					//Apply the timing of the best solution that was found
					for(ctr = 0, solutionval = 0; ctr < chordnotectr; ctr++)
					{	//For each chordnote that was parsed (re-use solutionval to track which technote number this is)
						EOF_PRO_GUITAR_NOTE *tnp;
						EOF_TECHNOTE_GAP *gp;

						if(!chordnote[ctr]->flags && !chordnote[ctr]->eflags)
							continue;	//If this chordnote doesn't have any statuses left to be placed, skip it

						tnp = eof_pro_guitar_track_add_tech_note(tp);
						if(!tnp)
						{
							eof_log("\tError allocating memory for chord tech note.  Aborting", 1);
							error = 1;
							break;	//Break from loop
						}
						gp = &gaps[bestsolution[solutionval]];	//Determine which gap this technote will be placed into
						tnp->pos = (double)gp->start + ((double)(gp->population + 1.0) * ((double)gp->size / (gp->capacity + 1.0))) + 0.5;	//Place this technote into the next available interval within this gap (round up to nearest ms)
						gp->population++;		//This gap now has one more technote in it
						tnp->type = np->type;
						///tnp->note = 1 << ctr;	//Not working?
						tnp->note = chordnote[ctr]->note;
						tnp->flags = chordnote[ctr]->flags;
						tnp->eflags = chordnote[ctr]->eflags;
						if(chordnote[ctr]->eflags & EOF_PRO_GUITAR_NOTE_EFLAG_PRE_BEND)
						{	//If this tech note is a pre-bend
							tnp->flags |= EOF_PRO_GUITAR_NOTE_FLAG_BEND;		//Set the bend flag as well
							tnp->bendstrength = chordnote[ctr]->bendstrength;	//And apply the correct bend strength for this pre-bend
							tnp->flags |= EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION;
						}
						tnp->slideend = chordnote[ctr]->slideend;
						tnp->unpitchend = chordnote[ctr]->unpitchend;
						if(tnp->slideend || tnp->unpitchend)
						{	//If an end of slide position was applied
							tnp->flags |= EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION;	//Re-apply the Rocksmith notation flag to indicate that the end of slide is defined
						}
						solutionval++;

						///DEBUG
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tPlaced chordnote tech note at %lums", tnp->pos);
						eof_log(eof_log_string, 1);
					}
				}//If the gaps array was initialized

				//Cleanup
				free(gaps);
				free(solution);
				free(bestsolution);
			}//If the solution arrays were created
		}//If there are technotes remaining to be placed
	}//If the chordnote lengths were processed

	//Cleanup
	for(ctr = 0; ctr < 6; ctr++)
	{	//For each entry in the chordnote array
		if(chordnote[ctr])
		{	//If this element has a pro guitar note structure pointer
			free(chordnote[ctr]);
			chordnote[ctr] = NULL;
		}
	}
	return error;	//Return success/error status
}

EOF_PRO_GUITAR_TRACK *eof_load_rs(char * fn)
{
	char *buffer = NULL, *buffer2 = NULL;		//Will be an array large enough to hold the largest line of text from input file
	EOF_PRO_GUITAR_TRACK *tp = NULL;
	size_t maxlinelength;
	unsigned long linectr = 1, tagctr;
	PACKFILE *inf = NULL;
	char *ptr, *ptr2, tag[256];
	char error = 0, warning = 0;
	char *phraselist[EOF_RS_PHRASE_IMPORT_LIMIT] = {0};	//Will store the list of phrase names
	unsigned long phraselist_count = 0;	//Keeps count of how many phrases have been imported
	EOF_TEXT_EVENT *eventlist[EOF_RS_EVENT_IMPORT_LIMIT] = {0};	//Will store the list of imported RS phrases, RS sections and RS events
	unsigned long eventlist_count = 0;	//Keeps count of how many events have been imported
	EOF_PRO_GUITAR_NOTE *chordlist[EOF_RS_CHORD_TEMPLATE_IMPORT_LIMIT] = {0};	//Will store the list of chord templates.  Those that are arpeggio templates will have the EOF_NOTE_TFLAG_ARP tflag set
	unsigned long chordlist_count = 0;	//Keeps count of how many chord templates have been imported
	unsigned long beat_count = 0;		//Keeps count of which ebeat is being parsed
	unsigned long note_count = 0;		//Keeps count of how many notes have been imported
	char strum_dir = 0;					//Tracks whether any chords were marked as up strums
	unsigned long ctr, ctr2, ctr3;
	long output = 0;
	char four_string = 1;	//Set to nonzero if string 5 or 6 is used by any chords or notes

	if(!eof_song || !eof_song_loaded)
		return NULL;	//For now, don't do anything unless a project is active

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return NULL;	//Don't do anything unless the active track is a pro guitar/bass track

	eof_log("\tImporting Rocksmith file", 1);
	eof_log("eof_load_rs() entered", 1);

	//Initialize pointers and handles
	if(!fn)
	{
		return NULL;
	}
	inf = pack_fopen(fn, "rt");	//Open file in text mode
	if(!inf)
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError loading:  Cannot open input xml file:  \"%s\"", strerror(errno));	//Get the Operating System's reason for the failure
		eof_log(eof_log_string, 1);
		return NULL;
	}

	//Allocate memory buffers large enough to hold any line in this file
	maxlinelength = (size_t)FindLongestLineLength_ALLEGRO(fn, 0);
	if(!maxlinelength)
	{
		eof_log("\tError finding the largest line in the file.  Aborting", 1);
		(void) pack_fclose(inf);
		return NULL;
	}
	buffer = (char *)malloc(maxlinelength);
	buffer2 = (char *)malloc(maxlinelength);
	if(!buffer || !buffer2)
	{
		if(buffer)
		{	//If the first buffer was allocated
			free(buffer);
		}
		if(buffer2)
		{	//If the second buffer was allocated, not sure why this would happen if the first failed
			free(buffer2);
		}
		eof_log("\tError allocating memory.  Aborting", 1);
		(void) pack_fclose(inf);
		return NULL;
	}
	memset(buffer, 0, maxlinelength);	//Fill with 0s to satisfy Splint
	memset(buffer2, 0, maxlinelength);

	//Allocate and initialize a pro guitar structure
	tp = malloc(sizeof(EOF_PRO_GUITAR_TRACK));
	if(!tp)
	{
		eof_log("\tError allocating memory.  Aborting", 1);
		(void) pack_fclose(inf);
		free(buffer);
		free(buffer2);
		return NULL;
	}
	memset(tp, 0, sizeof(EOF_PRO_GUITAR_TRACK));	//Initialize memory block to 0 to avoid crashes when not explicitly setting counters that were newly added to the pro guitar structure
	tp->numstrings = 6;	//The number of strings that will be used in the arrangement is unknown, default to 6
	tp->numfrets = 22;	//The number of frets that will be used in the arrangement is unknown, default to 22
	tp->note = tp->pgnote;	//Put the regular pro guitar note array into effect
	tp->parent = eof_song->pro_guitar_track[eof_song->track[eof_selected_track]->tracknum]->parent;	//Initialize the parent so that the alternate track name can be set if appropriate

	//Read first line of text, capping it to prevent buffer overflow
	if(!pack_fgets(buffer, (int)maxlinelength, inf))
	{	//I/O error
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Rocksmith import failed on line #%lu:  Unable to read from file:  \"%s\"", linectr, strerror(errno));
		eof_log(eof_log_string, 1);
		error = 1;
	}

	//Parse the contents of the file
	while(!error && !pack_feof(inf))
	{	//Until there was an error reading from the file or end of file is reached
		#ifdef RS_IMPORT_DEBUG
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tProcessing line #%lu", linectr);
			eof_log(eof_log_string, 2);
		#endif

		//Separate the line into the opening XML tag (buffer) and the content between the opening and closing tag (buffer2)
		ptr = strcasestr_spec(buffer, ">");
		if(ptr)
		{	//If this line contained an XML tag
			strcpy(buffer2, ptr);	//Copy the portion of the buffer beginning after the opening tag
			ptr2 = strchr(buffer2, '<');
			if(ptr2)
			{	//If this line contains a closing XML tag
				*ptr2 = '\0';	//Drop it from buffer2
			}
			else
			{	//Otherwise empty buffer2
				buffer2[0] = '\0';
			}
			*ptr = '\0';	//Drop everything after the first tag from buffer
		}
		else
		{	//This line had no XML, skip it
			(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text, so the EOF condition can be checked
			linectr++;
			continue;
		}

		if(strcasestr_spec(buffer, "<scoreUrl>"))
		{	//If this is a scoreUrl tag (Go PlayAlong XML file)
			eof_log("This appears to be a Go PlayAlong XML file.  Aborting", 1);
			allegro_message("This appears to be a Go PlayAlong XML file.  Use \"File>Guitar Pro Import\" instead.");
			error = 1;
		}
		else if(strcasestr_spec(buffer, "<song>") || strcasestr_spec(buffer, "<song version"))
		{	//If this is the song tag
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make an undo state because the tempo map will be overwritten
			for(ctr = eof_song->text_events; ctr > 0; ctr--)
			{	//For each of the project's text events (in reverse order)
				if(eof_song->text_event[ctr - 1]->track == eof_selected_track)
				{	//If the text event is assigned to the track being replaced
					eof_song_delete_text_event(eof_song, ctr - 1);	//Delete it
				}
			}
		}
		else if(strcasestr_spec(buffer, "<vocals "))
		{	//If this is the vocals tag
			allegro_message("This is a lyric file, not a guitar or bass arrangement.\nUse File>Import>Lyric to load this file.");
			eof_log("\tError:  This is a lyric file, not a guitar or bass arrangement.", 1);
			free(buffer);
			free(buffer2);
			free(tp);
			return NULL;
		}
		else if(strcasestr_spec(buffer, "<title>"))
		{	//If this is the title tag
			shrink_xml_text(tag, sizeof(tag), buffer2);	//Convert any escape sequences in the tag content to normal text
			strncpy(eof_song->tags->title, tag, sizeof(eof_song->tags->title) - 1);
		}
		else if(strcasestr_spec(buffer, "<arrangement>"))
		{	//If this is the arrangement tag
			char match = 0;

			shrink_xml_text(tag, sizeof(tag), buffer2);	//Convert any escape sequences in the tag content to normal text
			for(ctr = 1; ctr < EOF_TRACKS_MAX; ctr++)
			{	//Compare the arrangement name against EOF's natively supported tracks
				if(!ustricmp(tag, eof_midi_tracks[ctr].name))
				{	//If any native track name matches the arrangement name from this tag
					match = 1;
					break;
				}
			}
			if(!match)
			{	//If this arrangement name isn't one of EOF's native track names
				tp->parent->flags |= EOF_TRACK_FLAG_ALT_NAME;
				strncpy(tp->parent->altname, tag, sizeof(tp->parent->altname) - 1);

				//If the arrangement name matches one of the Rocksmith arrangement types
				if(!ustricmp(tag, "Combo"))
				{
					tp->arrangement = 1;
				}
				else if(!ustricmp(tag, "Rhythm"))
				{
					tp->arrangement = 2;
				}
				else if(!ustricmp(tag, "Lead"))
				{
					tp->arrangement = 3;
				}
				else if(!ustricmp(tag, "Bass"))
				{
					tp->arrangement = 4;
				}
			}
		}
		else if(strcasestr_spec(buffer, "<tuning"))
		{	//If this is the tuning tag
			int success_count = 0;

			success_count += parse_xml_attribute_number("string0", buffer, &output);	//Read the value of string 0's tuning from the opening tag
			tp->tuning[0] = output;	//Store the tuning
			success_count += parse_xml_attribute_number("string1", buffer, &output);	//Read the value of string 1's tuning from the opening tag
			tp->tuning[1] = output;	//Store the tuning
			success_count += parse_xml_attribute_number("string2", buffer, &output);	//Read the value of string 2's tuning from the opening tag
			tp->tuning[2] = output;	//Store the tuning
			success_count += parse_xml_attribute_number("string3", buffer, &output);	//Read the value of string 3's tuning from the opening tag
			tp->tuning[3] = output;	//Store the tuning
			success_count += parse_xml_attribute_number("string4", buffer, &output);	//Read the value of string 4's tuning from the opening tag
			tp->tuning[4] = output;	//Store the tuning
			success_count += parse_xml_attribute_number("string5", buffer, &output);	//Read the value of string 5's tuning from the opening tag
			tp->tuning[5] = output;	//Store the tuning

			if(success_count != 6)
			{	//If 6 string tunings were not read
				eof_log("Error reading tuning values from XML.  Aborting", 1);
				error = 1;
				break;
			}
		}
		else if(strcasestr_spec(buffer, "<capo>"))
		{	//If this is the capo tag (RS2)
			long capo;
			if((buffer2[0] != '\0') && (buffer2[0] != '0'))
			{	//If a nonzero capo is defined
				capo = atol(buffer2);
				if(capo > 0)
				{	//If the number was successfully read
					tp->capo = capo;
				}
			}
		}
		else if(strcasestr_spec(buffer, "<artistName>"))
		{	//If this is the artistName tag
			shrink_xml_text(tag, sizeof(tag), buffer2);	//Convert any escape sequences in the tag content to normal text
			strncpy(eof_song->tags->artist, tag, sizeof(eof_song->tags->artist) - 1);
		}
		else if(strcasestr_spec(buffer, "<albumName>"))
		{	//If this is the albumName tag
			shrink_xml_text(tag, sizeof(tag), buffer2);	//Convert any escape sequences in the tag content to normal text
			strncpy(eof_song->tags->album, tag, sizeof(eof_song->tags->album) - 1);
		}
		else if(strcasestr_spec(buffer, "<albumYear>"))
		{	//If this is the albumYear tag
			shrink_xml_text(tag, sizeof(tag), buffer2);	//Convert any escape sequences in the tag content to normal text
			strncpy(eof_song->tags->year, tag, sizeof(eof_song->tags->year) - 1);
		}
		else if(strcasestr_spec(buffer, "<phrases") && !strstr(buffer, "/>"))
		{	//If this is the phrases tag and it isn't empty
			#ifdef RS_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tProcessing <phrases> tag on line #%lu", linectr);
				eof_log(eof_log_string, 1);
			#endif

			(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
			linectr++;
			while(!error || !pack_feof(inf))
			{	//Until there was an error reading from the file or end of file is reached
				if(strcasestr_spec(buffer, "</phrases"))
				{	//If this is the end of the phrases tag
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t%lu phrases loaded", phraselist_count);
					eof_log(eof_log_string, 1);
					break;	//Break from loop
				}
				if(phraselist_count < EOF_RS_PHRASE_IMPORT_LIMIT)
				{	//If another phrase name can be stored
					if(!parse_xml_attribute_text(tag, sizeof(tag), "name", buffer))
					{	//If the phrase name could not be read
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading phrase name on line #%lu.  Aborting", linectr);
						eof_log(eof_log_string, 1);
						error = 1;
						break;	//Break from inner loop
					}
					phraselist[phraselist_count] = malloc(strlen(tag) + 1);	//Allocate memory to store the phrase name
					if(!phraselist[phraselist_count])
					{
						eof_log("\tError allocating memory.  Aborting", 1);
						error = 1;
						break;	//Break from inner loop
					}
					strcpy(phraselist[phraselist_count], tag);
					phraselist_count++;
				}
				else
				{	//Otherwise the phrase limit has been exceeded
					eof_log("\t\tError:  Phrase limit exceeded.  Aborting", 1);
					error = 1;
					break;	//Break from inner loop
				}

				(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
				linectr++;	//Increment line counter
			}
			if(error)
				break;	//Break from outer loop
		}//If this is the phrases tag and it isn't empty
		else if(strcasestr_spec(buffer, "<phraseIterations") && !strstr(buffer, "/>"))
		{	//If this is the phraseIterations tag and it isn't empty
			long timestamp = 0, id = 0;
			unsigned long phraseitctr = 0;

			#ifdef RS_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tProcessing <phraseIterations> tag on line #%lu", linectr);
				eof_log(eof_log_string, 1);
			#endif

			(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
			linectr++;
			while(!error || !pack_feof(inf))
			{	//Until there was an error reading from the file or end of file is reached
				if(strcasestr_spec(buffer, "</phraseIterations"))
				{	//If this is the end of the phraseIterations tag
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t%lu phrase iterations loaded", phraseitctr);
					eof_log(eof_log_string, 1);
					break;	//Break from loop
				}
				if(strcasestr_spec(buffer, "<heroLevels"))
				{	//If this is the start of a heroLevels tag, ignore all lines of XML until the end of the tag is reached
					#ifdef RS_IMPORT_DEBUG
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tIgnoring <heroLevels> tag on line #%lu", linectr);
						eof_log(eof_log_string, 1);
					#endif
				}
				else if(strcasestr_spec(buffer, "<heroLevel "))
				{	//Ignore a heroLevel tag
				}
				else if(strcasestr_spec(buffer, "</heroLevels"))
				{	//Ignore the end of a heroLevels tag
				}
				else if(strcasestr_spec(buffer, "</phraseIteration>"))
				{	//Ignore the end of a phraseIteration tag, which would occur after a heroLevels tag
				}
				else
				{
					if(eventlist_count < EOF_RS_EVENT_IMPORT_LIMIT)
					{	//If another text event can be stored
						if(!parse_xml_rs_timestamp("time", buffer, &timestamp))
						{	//If the timestamp was not readable
							(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading timestamp on line #%lu.  Aborting", linectr);
							eof_log(eof_log_string, 1);
							error = 1;
							break;	//Break from inner loop
						}
						if(!parse_xml_attribute_number("phraseId", buffer, &id) || (id < 0))
						{	//If the phrase ID was not readable or was invalid
							(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading phrase ID on line #%lu.  Aborting", linectr);
							eof_log(eof_log_string, 1);
							error = 1;
							break;	//Break from inner loop
						}
						if((unsigned long)id >= phraselist_count)
						{	//If this phrase ID was not defined in the phrase tag
							(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError:  Invalid phrase ID on line #%lu.  Aborting", linectr);
							eof_log(eof_log_string, 1);
							error = 1;
							break;	//Break from inner loop
						}
						eventlist[eventlist_count] = malloc(sizeof(EOF_TEXT_EVENT));	//Allocate memory to store the text event
						if(!eventlist[eventlist_count])
						{
							eof_log("\tError allocating memory.  Aborting", 1);
							error = 1;
							break;	//Break from inner loop
						}
						strncpy(eventlist[eventlist_count]->text, phraselist[id], sizeof(eventlist[eventlist_count]->text) - 1);	//Copy the phrase name
						eventlist[eventlist_count]->track = eof_selected_track;
						eventlist[eventlist_count]->flags = EOF_EVENT_FLAG_RS_PHRASE;
						eventlist[eventlist_count]->beat = timestamp;	//Store the real timestamp, it will need to be converted to the beat number later
						phraseitctr++;
						eventlist_count++;
					}
					else
					{	//Otherwise the text event limit has been exceeded
						eof_log("\t\tError:  Text event limit exceeded.  Aborting", 1);
						error = 1;
						break;	//Break from inner loop
					}
				}

				(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
				linectr++;	//Increment line counter
			}
			if(error)
				break;	//Break from outer loop
		}//If this is the phraseIterations tag and it isn't empty
		else if(strcasestr_spec(buffer, "<chordTemplates") && !strstr(buffer, "/>"))
		{	//If this is the chordTemplates tag and it isn't empty
			char finger[8] = {0};
			char frets[8] = {0};
			unsigned char note = 0, isarp = 0;

			#ifdef RS_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tProcessing <chordTemplates> tag on line #%lu", linectr);
				eof_log(eof_log_string, 1);
			#endif

			(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
			linectr++;
			while(!error || !pack_feof(inf))
			{	//Until there was an error reading from the file or end of file is reached
				if(strcasestr_spec(buffer, "</chordTemplates"))
				{	//If this is the end of the chordTemplates tag
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t%lu chord templates loaded", chordlist_count);
					eof_log(eof_log_string, 1);
					break;	//Break from loop
				}
				if(chordlist_count < EOF_RS_CHORD_TEMPLATE_IMPORT_LIMIT)
				{	//If another chord can be stored
					if(eof_parse_chord_template(tag, sizeof(tag), finger, frets, &note, &isarp, linectr, buffer))
					{	//If there was an error reading the chord template
						error = 1;
						break;
					}
					if(note & 48)
					{	//If a fifth or sixth string is used by this chord
						four_string = 0;
					}
					//Track the highest used fret number
					for(ctr = 0; ctr < 6; ctr++)
					{	//For each of the 6 supported strings
						if(frets[ctr] >= tp->capo)
							frets[ctr] -= tp->capo;		//Apply the capo if applicable
						if(frets[ctr] > tp->numfrets)
							tp->numfrets = frets[ctr];	//Track the highest used fret number
					}

					//Add chord template to list
					chordlist[chordlist_count] = malloc(sizeof(EOF_PRO_GUITAR_NOTE));
					if(!chordlist[chordlist_count])
					{
						eof_log("\tError allocating memory.  Aborting", 1);
						error = 1;
						break;	//Break from inner loop
					}
					memset(chordlist[chordlist_count], 0, sizeof(EOF_PRO_GUITAR_NOTE));	//Initialize memory block to 0
					strncpy(chordlist[chordlist_count]->name, tag, sizeof(chordlist[chordlist_count]->name) - 1);	//Store the chord name
					memcpy(chordlist[chordlist_count]->finger, finger, 8);	//Store the finger array
					memcpy(chordlist[chordlist_count]->frets, frets, 8);	//Store the fret array
					chordlist[chordlist_count]->note = note;	//Store the note mask
					if(isarp)
					{	//If the chord template was found to be associated with arpeggio technique
						chordlist[chordlist_count]->tflags |= EOF_NOTE_TFLAG_ARP;
					}
					chordlist_count++;
				}//If another chord can be stored
				else
				{	//Otherwise the chord limit has been exceeded
					eof_log("\t\tError:  Chord template limit exceeded.  Aborting", 1);
					error = 1;
					break;	//Break from inner loop
				}

				(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
				linectr++;	//Increment line counter
			}//Until there was an error reading from the file or end of file is reached
			if(error)
				break;	//Break from outer loop
		}//If this is the chordTemplates tag and it isn't empty
		else if(strcasestr_spec(buffer, "<ebeats") && !strstr(buffer, "/>"))
		{	//If this is the ebeats tag and it isn't empty
			#ifdef RS_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tProcessing <ebeats> tag on line #%lu", linectr);
				eof_log(eof_log_string, 1);
			#endif

			(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
			linectr++;
			while(!error || !pack_feof(inf))
			{	//Until there was an error reading from the file or end of file is reached
				if(strcasestr_spec(buffer, "</ebeats"))
				{	//If this is the end of the ebeats tag
					for(ctr = eof_song->beats; ctr > beat_count; ctr--)
					{	//For each of the remaining beats in the project (which weren't initialized), in reverse order
						eof_song_delete_beat(eof_song, ctr - 1);	//Delete it.  eof_truncate_chart() will be run by the calling function to add beats as necessary
					}
					break;	//Break from loop
				}
				if(!parse_xml_rs_timestamp("time", buffer, &output))
				{	//If the timestamp was not readable
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading timestamp on line #%lu.  Aborting", linectr);
					eof_log(eof_log_string, 1);
					error = 1;
					break;	//Break from inner loop
				}
				if(beat_count >= eof_song->beats)
				{	//If a beat has to be added to the project
					if(!eof_song_add_beat(eof_song))
					{	//If there was an error adding a beat
						eof_log("\tError adding beat.  Aborting", 1);
						error = 1;
						break;	//Break from inner loop
					}
				}
				eof_song->beat[beat_count]->pos = output;			//Store the integer timestamp
				eof_song->beat[beat_count]->fpos = (double)output;	//Store the floating point timestamp
				if(!beat_count)
				{	//If this is the first beat, update the chart's MIDI delay
					eof_song->tags->ogg[eof_selected_ogg].midi_offset = output;
				}
				beat_count++;

				(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
				linectr++;	//Increment line counter
			}//Until there was an error reading from the file or end of file is reached
			if(error)
				break;	//Break from outer loop

			eof_calculate_tempo_map(eof_song);	//Determine all tempo changes based on the beats' timestamps
			if(!beat_count)
			{	//If no beats were parsed
				eof_log("No valid beat tags found.  Aborting", 1);
				error = 1;
				break;
			}
		}//If this is the ebeats tag and it isn't empty
		else if(strcasestr_spec(buffer, "<controls") && !strstr(buffer, "/>"))
		{	//If this is the controls tag and it isn't empty
			#ifdef RS_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tProcessing <controls> tag on line #%lu", linectr);
				eof_log(eof_log_string, 1);
			#endif

			(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
			linectr++;
			while(!error || !pack_feof(inf))
			{	//Until there was an error reading from the file or end of file is reached
				if(strcasestr_spec(buffer, "</controls"))
				{	//If this is the end of the controls tag
					break;	//Break from loop
				}
				if(!parse_xml_rs_timestamp("time", buffer, &output))
				{	//If the timestamp was not readable
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading timestamp on line #%lu.  Aborting", linectr);
					eof_log(eof_log_string, 1);
					error = 1;
					break;	//Break from inner loop
				}
				ptr = strcasestr_spec(buffer, "ShowMessageBox(");
				if(ptr)
				{	//If this is a start of a popup message
					if(tp->popupmessages < EOF_MAX_PHRASES)
					{	//If another popup message can be stored
						ptr = strcasestr_spec(ptr, ",");	//The message text begins after the first comma in the XML tag
						if(!ptr)
						{
							(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading start of control message on line #%lu.  Aborting", linectr);
							eof_log(eof_log_string, 1);
							error = 1;
							break;	//Break from inner loop
						}
						ptr2 = strchr(ptr, ')');	//The message text ends at the first closing parenthesis after the start of the message
						if(!ptr2)
						{
							(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading end of control message on line #%lu.  Aborting", linectr);
							eof_log(eof_log_string, 1);
							error = 1;
							break;	//Break from inner loop
						}
						*ptr2 = '\0';	//Truncate the buffer
						tp->popupmessage[tp->popupmessages].start_pos = output;	//Store the starting timestamp
						tp->popupmessage[tp->popupmessages].end_pos = 0;		//Use 0 to indicate an un-ended popup message
						strncpy(tp->popupmessage[tp->popupmessages].name, ptr, sizeof(tp->popupmessage[tp->popupmessages].name) - 1);	//Store the message text
						tp->popupmessages++;
					}
				}
				else if((ptr = strcasestr_spec(buffer, "ClearAllMessageBoxes()")) && ptr)
				{	//If this is the end of any popup messages that are not ended yet
					for(ctr = 0; ctr < tp->popupmessages; ctr++)
					{	//For each popup message that has been imported so far
						if(!tp->popupmessage[ctr].end_pos)
						{	//If the message hasn't been ended
							tp->popupmessage[ctr].end_pos = output;		//Store the ending timestamp
						}
					}
				}
				else if((ptr = strcasestr_spec(buffer, "CDlcTone(")) && ptr)
				{	//if this is a tone change
					if(tp->tonechanges < EOF_MAX_PHRASES)
					{	//If another tone change can be stored
						if(!ptr)
						{
							(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading start of control message on line #%lu.  Aborting", linectr);
							eof_log(eof_log_string, 1);
							error = 1;
							break;	//Break from inner loop
						}
						ptr2 = strchr(ptr, ')');	//The tone key name ends at the first closing parenthesis after the start of the name
						if(!ptr2)
						{
							(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading end of control message on line #%lu.  Aborting", linectr);
							eof_log(eof_log_string, 1);
							error = 1;
							break;	//Break from inner loop
						}
						*ptr2 = '\0';	//Truncate the buffer
						tp->tonechange[tp->tonechanges].start_pos = output;		//Store the starting timestamp
						tp->tonechange[tp->tonechanges].end_pos = 0;			//Tone changes don't use an end position
						strncpy(tp->tonechange[tp->tonechanges].name, ptr, sizeof(tp->tonechange[tp->tonechanges].name) - 1);	//Store the message text
						tp->tonechanges++;
					}
				}
				else
				{
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError:  Unrecognized control message on line #%lu.  Aborting", linectr);
					eof_log(eof_log_string, 1);
					error = 1;
					break;	//Break from inner loop
				}

				(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
				linectr++;	//Increment line counter
			}//Until there was an error reading from the file or end of file is reached
			if(error)
				break;	//Break from outer loop

			for(ctr = 0; ctr < tp->popupmessages; ctr++)
			{	//For each popup message that was imported
				if(!tp->popupmessage[ctr].end_pos)
				{	//If the message hasn't been ended
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tWarning:  Control message #%lu was not ended.  Correcting", ctr);
					eof_log(eof_log_string, 1);
					tp->popupmessage[ctr].end_pos = tp->popupmessage[ctr].start_pos + 1;
				}
			}
		}//If this is the controls tag and it isn't empty
		else if(strcasestr_spec(buffer, "<tonebase"))
		{	//If this is the tonebase tag (RS2)
			shrink_xml_text(tag, sizeof(tag), buffer2);	//Convert any escape sequences in the tag content to normal text
			strncpy(tp->defaulttone, tag, sizeof(tp->defaulttone) - 1);
		}
		else if(strcasestr_spec(buffer, "<tones") && !strstr(buffer, "/>"))
		{	//If this is the tones tag (RS2) and it isn't empty
			unsigned long tonectr = 0;

			#ifdef RS_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tProcessing <tones> tag on line #%lu", linectr);
				eof_log(eof_log_string, 1);
			#endif

			(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
			linectr++;
			while(!error || !pack_feof(inf))
			{	//Until there was an error reading from the file or end of file is reached
				if(strcasestr_spec(buffer, "</tones"))
				{	//If this is the end of the tones tag
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t%lu tones loaded", tonectr);
					eof_log(eof_log_string, 1);
					break;	//Break from loop
				}
				if(tonectr < EOF_MAX_PHRASES)
				{	//If another tone change can be stored
					if(!parse_xml_rs_timestamp("time", buffer, &output))
					{	//If the timestamp was not readable
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading timestamp on line #%lu.  Aborting", linectr);
						eof_log(eof_log_string, 1);
						error = 1;
						break;	//Break from inner loop
					}
					if(!parse_xml_attribute_text(tag, sizeof(tag), "name", buffer))
					{	//If the section name could not be read
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading tone name on line #%lu.  Aborting", linectr);
						eof_log(eof_log_string, 1);
						error = 1;
						break;	//Break from inner loop
					}
					tp->tonechange[tp->tonechanges].start_pos = output;		//Store the starting timestamp
					strncpy(tp->tonechange[tp->tonechanges].name, tag, sizeof(tp->tonechange[tp->tonechanges].name) - 1);	//Store the tone name
					tp->tonechanges++;
				}
				else
				{	//Otherwise the tone change limit has been exceeded
					eof_log("\t\tError:  Tone change limit exceeded.  Aborting", 1);
					error = 1;
					break;	//Break from inner loop
				}

				(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
				linectr++;	//Increment line counter
			}
			if(error)
				break;	//Break from outer loop
		}//If this is the tones tag (RS2) and it isn't empty
		else if(strcasestr_spec(buffer, "<sections") && !strstr(buffer, "/>"))
		{	//If this is the sections tag and it isn't empty
			unsigned long sectionctr = 0;

			#ifdef RS_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tProcessing <sections> tag on line #%lu", linectr);
				eof_log(eof_log_string, 1);
			#endif

			(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
			linectr++;
			while(!error || !pack_feof(inf))
			{	//Until there was an error reading from the file or end of file is reached
				if(strcasestr_spec(buffer, "</sections"))
				{	//If this is the end of the sections tag
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t%lu sections loaded", sectionctr);
					eof_log(eof_log_string, 1);
					break;	//Break from loop
				}
				if(eventlist_count < EOF_RS_EVENT_IMPORT_LIMIT)
				{	//If another text event can be stored
					if(!parse_xml_attribute_text(tag, sizeof(tag), "name", buffer))
					{	//If the section name could not be read
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading section name on line #%lu.  Aborting", linectr);
						eof_log(eof_log_string, 1);
						error = 1;
						break;	//Break from inner loop
					}
					if(!eof_rs_section_text_valid(tag))
					{	//If this isn't a valid name for a RS section
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tWarning:  Invalid section name on line #%lu.", linectr);
						eof_log(eof_log_string, 1);
					}
					if(!parse_xml_rs_timestamp("startTime", buffer, &output))
					{	//If the timestamp was not readable
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading timestamp on line #%lu.  Aborting", linectr);
						eof_log(eof_log_string, 1);
						error = 1;
						break;	//Break from inner loop
					}
					eventlist[eventlist_count] = malloc(sizeof(EOF_TEXT_EVENT));	//Allocate memory to store the text event
					if(!eventlist[eventlist_count])
					{
						eof_log("\tError allocating memory.  Aborting", 1);
						error = 1;
						break;	//Break from inner loop
					}
					strncpy(eventlist[eventlist_count]->text, tag, sizeof(eventlist[eventlist_count]->text) - 1);	//Copy the section name
					eventlist[eventlist_count]->track = eof_selected_track;
					eventlist[eventlist_count]->flags = EOF_EVENT_FLAG_RS_SECTION;
					eventlist[eventlist_count]->beat = output;	//Store the real timestamp, it will need to be converted to the beat number later
					sectionctr++;
					eventlist_count++;
				}
				else
				{	//Otherwise the text event limit has been exceeded
					eof_log("\t\tError:  Text event limit exceeded.  Aborting", 1);
					error = 1;
					break;	//Break from inner loop
				}

				(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
				linectr++;	//Increment line counter
			}
			if(error)
				break;	//Break from outer loop
		}//If this is the sections tag and it isn't empty
		else if(strcasestr_spec(buffer, "<events") && !strstr(buffer, "/>"))
		{	//If this is the events tag and it isn't empty
			unsigned long eventctr = 0;

			#ifdef RS_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tProcessing <events> tag on line #%lu", linectr);
				eof_log(eof_log_string, 1);
			#endif

			(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
			linectr++;
			while(!error || !pack_feof(inf))
			{	//Until there was an error reading from the file or end of file is reached
				if(strcasestr_spec(buffer, "</events"))
				{	//If this is the end of the events tag
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t%lu events loaded", eventctr);
					eof_log(eof_log_string, 1);
					break;	//Break from loop
				}
				if(eventlist_count < EOF_RS_EVENT_IMPORT_LIMIT)
				{	//If another text event can be stored
					if(!parse_xml_rs_timestamp("time", buffer, &output))
					{	//If the timestamp was not readable
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading timestamp on line #%lu.  Aborting", linectr);
						eof_log(eof_log_string, 1);
						error = 1;
						break;	//Break from inner loop
					}
					if(!parse_xml_attribute_text(tag, sizeof(tag), "code", buffer))
					{	//If the event code could not be read
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading event code on line #%lu.  Aborting", linectr);
						eof_log(eof_log_string, 1);
						error = 1;
						break;	//Break from inner loop
					}
					eventlist[eventlist_count] = malloc(sizeof(EOF_TEXT_EVENT));	//Allocate memory to store the text event
					if(!eventlist[eventlist_count])
					{
						eof_log("\tError allocating memory.  Aborting", 1);
						error = 1;
						break;	//Break from inner loop
					}
					strncpy(eventlist[eventlist_count]->text, tag, sizeof(eventlist[eventlist_count]->text) - 1);	//Copy the event code
					eventlist[eventlist_count]->track = eof_selected_track;
					eventlist[eventlist_count]->flags = EOF_EVENT_FLAG_RS_EVENT;
					eventlist[eventlist_count]->beat = output;	//Store the real timestamp, it will need to be converted to the beat number later
					eventctr++;
					eventlist_count++;
				}
				else
				{	//Otherwise the text event limit has been exceeded
					eof_log("\t\tError:  Text event limit exceeded.  Aborting", 1);
					error = 1;
					break;	//Break from inner loop
				}

				(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
				linectr++;	//Increment line counter
			}
			if(error)
				break;	//Break from outer loop
		}//If this is the events tag and it isn't empty
		else if(strcasestr_spec(buffer, "<levels") && !strstr(buffer, "/>"))
		{	//If this is the levels tag and it isn't empty
			long curdiff = 0, time = 0;
			long fret = 0, palmmute, mute;
			unsigned long flags;
			EOF_PRO_GUITAR_NOTE *np = NULL;

			#ifdef RS_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tProcessing <levels> tag on line #%lu", linectr);
				eof_log(eof_log_string, 1);
			#endif

			(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
			linectr++;
			while(!error || !pack_feof(inf))
			{	//Until there was an error reading from the file or end of file is reached
				if(strcasestr_spec(buffer, "</levels"))
				{	//If this is the end of the levels tag
					break;	//Break from loop
				}

				//Read level tag
				ptr = strcasestr_spec(buffer, "<level difficulty");
				if(ptr)
				{	//If this is a level tag

					#ifdef RS_IMPORT_DEBUG
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tProcessing <level> tag on line #%lu", linectr);
						eof_log(eof_log_string, 1);
					#endif

					if(!parse_xml_attribute_number("difficulty", buffer, &curdiff))
					{	//If the difficulty number was not readable
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading difficulty number on line #%lu.  Aborting", linectr);
						eof_log(eof_log_string, 1);
						error = 1;
						break;	//Break from inner loop
					}
					(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
					linectr++;
					while(!error || !pack_feof(inf))
					{	//Until there was an error reading from the file or end of file is reached
						if(strcasestr_spec(buffer, "</level>"))
						{	//If this is the end of the level tag
							break;	//Break from loop
						}

						//Read notes tag
						if(strcasestr_spec(buffer, "<notes") && !strstr(buffer, "/>"))
						{	//If this is the notes tag and it isn't empty
							EOF_PRO_GUITAR_NOTE *npp = NULL;	//Track the previously imported single note for split status purposes

							#ifdef RS_IMPORT_DEBUG
								(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\tProcessing <notes> tag on line #%lu", linectr);
								eof_log(eof_log_string, 1);
							#endif

							tagctr = 0;
							(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
							linectr++;
							while(!error || !pack_feof(inf))
							{	//Until there was an error reading from the file or end of file is reached
								if(strcasestr_spec(buffer, "</notes"))
								{	//If this is the end of the notes tag
									#ifdef RS_IMPORT_DEBUG
										(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\t\tAdded %lu notes", tagctr);
										eof_log(eof_log_string, 1);
									#endif
									break;	//Break from loop
								}

								//Read note tag
								ptr = strcasestr_spec(buffer, "<note ");
								if(ptr)
								{	//If this is a note tag
									np = eof_rs_import_note_tag_data(buffer, 1, tp, linectr, curdiff);	//Parse the note tag and add the note to the track
									if(!np)
									{	//If there was an error doing so
										error = 1;
										break;	//Break from inner loop
									}
									if(npp)
									{	//If there was a previous single note imported for this difficulty
										if(npp->pos == np->pos)
										{	//If it was at the same timestamp, apply split status to both notes to ensure it's kept when fixup logic merges them
											npp->flags |= EOF_PRO_GUITAR_NOTE_FLAG_SPLIT;
											np->flags |= EOF_PRO_GUITAR_NOTE_FLAG_SPLIT;
										}
									}
									if(np->note & 48)
									{	//If a fifth or sixth string is used by this chord
										four_string = 0;
									}
									npp = np;	//Remember the last imported single note
									note_count++;
									tagctr++;

									fret = eof_get_pro_guitar_note_highest_fret_value(np);
									if(fret > tp->numfrets)
										tp->numfrets = fret;	//Track the highest used fret number
								}//If this is a note tag

								//Read bendValue tag
								ptr = strcasestr_spec(buffer, "<bendValue ");
								if(ptr)
								{	//If this is a bendValue tag
									if(!eof_rs_import_note_tag_data(buffer, 1, tp, linectr, curdiff))
									{	//If there was an error parsing the bendvalue tag and adding a technote as appropriate
										error = 1;
										break;	//Break from inner loop
									}
								}//If this is a bendValue tag

								(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
								linectr++;
							}//Until there was an error reading from the file or end of file is reached
							if(error)
								break;	//Break from inner loop
						}//If this is a notes tag
						else if(strcasestr_spec(buffer, "<chords") && !strstr(buffer, "/>"))
						{	//If this is a chords tag and it isn't empty
							long id = 0, lastid = -1;
							EOF_PRO_GUITAR_NOTE *chordnote[6] = {NULL, NULL, NULL, NULL, NULL, NULL};	//Stores chordnote data
							unsigned long chordnotectr = 7;	//Initialize this with an invalid value so chordnote parsing can track whether a chord tag is read
							unsigned long numtechnotes = 0;	//The number of tech notes created for this chord

							#ifdef RS_IMPORT_DEBUG
								(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\tProcessing <chords> tag on line #%lu", linectr);
								eof_log(eof_log_string, 1);
							#endif

							tagctr = 0;
							(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
							linectr++;
							while(!error || !pack_feof(inf))
							{	//Until there was an error reading from the file or end of file is reached
								if(strcasestr_spec(buffer, "</chords>"))
								{	//If this is the end of the chords tag
									#ifdef RS_IMPORT_DEBUG
										(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\t\tAdded %lu chords", tagctr);
										eof_log(eof_log_string, 1);
									#endif
									break;	//Break from loop
								}

								if(strcasestr_spec(buffer, "</chord>"))
								{	//If this is the end of the chord tag, process and add technotes for the contents of the chordnote[] array as appropriate
									error = eof_rs_import_process_chordnotes(tp, np, chordnote, chordnotectr, numtechnotes);
									if(error)	//If there was an error performing these tasks
										break;	//Break from loop
									chordnotectr = 7;	//Reset this to an invalid value so the chordnote logic can tell that a chord tag is NOT being parsed
								}//If this is the end of the chord tag

								//Read chord tag
								if(strcasestr_spec(buffer, "<chord "))
								{	//If this is a chord tag
									long highdensity;

									chordnotectr = 0;	//Reset this so the chordnote logic can tell that a chord tag is being parsed
									numtechnotes = 0;	//Reset this count
									if(note_count < EOF_MAX_NOTES)
									{	//If another chord can be stored
										flags = 0;

										//Read chord attributes
										if(!parse_xml_rs_timestamp("time", buffer, &time))
										{	//If the timestamp was not readable
											(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading timestamp on line #%lu.  Aborting", linectr);
											eof_log(eof_log_string, 1);
											error = 1;
											break;	//Break from inner loop
										}
										if(!parse_xml_attribute_number("chordId", buffer, &id) || (id < 0))
										{	//If the chord ID was not readable or was invalid
											(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading chord ID on line #%lu.  Aborting", linectr);
											eof_log(eof_log_string, 1);
											error = 1;
											break;	//Break from inner loop
										}
										if((unsigned long)id >= chordlist_count)
										{	//If this chord ID was not defined in the chordTemplates tag
											(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError:  Invalid chord ID on line #%lu.  Aborting", linectr);
											eof_log(eof_log_string, 1);
											error = 1;
											break;	//Break from inner loop
										}
										if(!parse_xml_attribute_text(tag, sizeof(tag), "strum", buffer))
										{	//If the strum direction could not be read
											(void) strncpy(tag, "down", sizeof(tag) - 1);	//Assume down strum
										}
										mute = highdensity = palmmute = 0;
										(void) parse_xml_attribute_number("fretHandMute", buffer, &mute);
										(void) parse_xml_attribute_number("highDensity", buffer, &highdensity);
										(void) parse_xml_attribute_number("palmMute", buffer, &palmmute);

										//Add chord and set attributes
										np = eof_pro_guitar_track_add_note(tp);	//Allocate, initialize and add the new note to the note array
										if(!np)
										{
											eof_log("\tError allocating memory.  Aborting", 1);
											error = 1;
											break;	//Break from inner loop
										}
										memcpy(np, chordlist[id], sizeof(EOF_PRO_GUITAR_NOTE));	//Copy the chord template into the new note
										np->pos = time;
										if(strcasestr_spec(tag, "up"))
										{	//If the strum direction is marked as up
											flags |= EOF_PRO_GUITAR_NOTE_FLAG_UP_STRUM;
											strum_dir = 1;	//Track that this arrangement defines chord strum directions
										}
										if(mute)
											flags |= EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE;
										if(highdensity)
											flags |= EOF_PRO_GUITAR_NOTE_FLAG_HD;
										if(palmmute)
											flags |= EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE;
										np->flags = flags;
										np->type = curdiff;
										note_count++;
										tagctr++;
										if((lastid >= 0) && (lastid != id))
										{	//If this chord tag references a different chord ID than the previous one
											np->tflags |= EOF_NOTE_TFLAG_CCHANGE;	//Track this condition
										}
										lastid = id;	//Remember the last parsed chord's chord template ID
									}//If another chord can be stored
								}//If this is a chord tag

								//Read chordnote tag
								if(strcasestr_spec(buffer, "<chordNote "))
								{	//If this is a chordnote tag
									EOF_PRO_GUITAR_NOTE *cnp;

									if(chordnotectr >= 6)
									{	//If six chordnotes were already read for this chord
										(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tChord contains too many chordnote tags on line #%lu.  Aborting", linectr);
										eof_log(eof_log_string, 1);
										error = 1;
										break;	//Break from inner loop
									}
									cnp = eof_rs_import_note_tag_data(buffer, 0, tp, linectr, curdiff);	//Parse the note tag and store the note data in the chordnote array
									if(!cnp)
									{	//If there was an error doing so
										error = 1;
										break;	//Break from inner loop
									}
									chordnote[chordnotectr] = cnp;
									if(cnp->length > np->length)
									{
										np->length = cnp->length;	//The chord inherits its longest chordnote length
									}
									chordnotectr++;
								}//If this is a chordnote tag

								//Read bendValue tag
								ptr = strcasestr_spec(buffer, "<bendValue ");
								if(ptr)
								{	//If this is a bendValue tag
									if(!eof_rs_import_note_tag_data(buffer, 1, tp, linectr, curdiff))
									{	//If there was an error parsing the bendvalue tag and adding a technote as appropriate
										error = 1;
										break;	//Break from inner loop
									}
									numtechnotes++;	//Track that a technote was added
								}//If this is a bendValue tag

								(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
								linectr++;	//Increment line counter
							}//If this is the end of the chord tag
							if(error)
							{
								for(ctr = 0; ctr < 6; ctr++)
								{	//For each entry in the chordnote array
									if(chordnote[ctr])
									{	//If this element has a pro guitar note structure pointer
										free(chordnote[ctr]);
										chordnote[ctr] = NULL;
									}
								}
								break;	//Break from inner loop
							}
						}//If this is a chords tag and it isn't empty
						else if(strcasestr_spec(buffer, "<anchors") && !strstr(buffer, "/>"))
						{	//If this is an anchors tag and it isn't empty
							#ifdef RS_IMPORT_DEBUG
								(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\tProcessing <anchors> tag on line #%lu", linectr);
								eof_log(eof_log_string, 1);
							#endif

							tagctr = 0;
							(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
							linectr++;
							while(!error || !pack_feof(inf))
							{	//Until there was an error reading from the file or end of file is reached
								if(strcasestr_spec(buffer, "</anchors"))
								{	//If this is the end of the anchors tag
									#ifdef RS_IMPORT_DEBUG
										(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\t\tAdded %lu anchors", tagctr);
										eof_log(eof_log_string, 1);
									#endif
									break;	//Break from loop
								}

								//Read anchor tag
								ptr = strcasestr_spec(buffer, "<anchor ");
								if(ptr)
								{	//If this is an anchor tag
									if(tp->handpositions < EOF_MAX_NOTES)
									{	//If another fret hand position can be stored
										if(!parse_xml_rs_timestamp("time", buffer, &time))
										{	//If the timestamp was not readable
											(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading timestamp on line #%lu.  Aborting", linectr);
											eof_log(eof_log_string, 1);
											error = 1;
											break;	//Break from inner loop
										}
										if(!parse_xml_attribute_number("fret", buffer, &fret))
										{	//If the fret number was not readable
											(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading fret number on line #%lu.  Aborting", linectr);
											eof_log(eof_log_string, 1);
											error = 1;
											break;	//Break from inner loop
										}
										if(fret - tp->capo > 19)
										{	//If the anchor is higher than RS1 and RB3 support (taking the capo into account)
											if(eof_write_rs_files || eof_write_rb_files)
											{	//If either of those exports is enabled, log it and warn the user
												(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tHigh anchor (fret %ld) at position %ld on line #%lu", fret - tp->capo, time, linectr);
												eof_log(eof_log_string, 1);
												if(!(warning & 1))
												{	//If the user wasn't warned about this error yet
													allegro_message("Warning:  This arrangement contains at least one fret hand position higher than fret 19.\nOffending positions won't work in Rocksmith 1 or RB3.");
													warning |= 1;
												}
											}
										}
 										if(fret - tp->capo > 21)
										{	//If the anchor is not valid (taking the capo into account), even for RS2, log it and warn the user
											(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tIgnoring invalid anchor (fret %ld) at position %ld on line #%lu", fret - tp->capo, time, linectr);
 											eof_log(eof_log_string, 1);
 											if(!(warning & 2))
 											{	//If the user wasn't warned about this error yet
												allegro_message("Warning:  This arrangement contains at least one invalid fret hand position (higher than fret 21).\n  Offending positions were dropped");
 												warning |= 2;
 											}
 										}
 										else
										{	//Otherwise add it to the track
											tp->handposition[tp->handpositions].start_pos = time;
											tp->handposition[tp->handpositions].end_pos = fret - tp->capo;
											tp->handposition[tp->handpositions].difficulty = curdiff;
											tp->handpositions++;
										}
									}//If another fret hand position can be stored
								}//If this is an anchor tag

								(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
								linectr++;	//Increment line counter
								tagctr++;
							}
							if(error)
								break;	//Break from inner loop
						}//If this is an anchors tag and it isn't empty
						else if(strcasestr_spec(buffer, "<handShapes") && !strstr(buffer, "/>"))
						{	//If this is an handShapes tag and it isn't empty
							long start, end, chordid;

							#ifdef RS_IMPORT_DEBUG
								(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\tProcessing <handShapes> tag on line #%lu", linectr);
								eof_log(eof_log_string, 1);
							#endif

							eof_pro_guitar_track_sort_notes(tp);
							tagctr = 0;
							(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
							linectr++;
							while(!error || !pack_feof(inf))
							{	//Until there was an error reading from the file or end of file is reached
								if(strcasestr_spec(buffer, "</handShapes"))
								{	//If this is the end of the handShapes tag
									#ifdef RS_IMPORT_DEBUG
										(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\t\tParsed %lu handshapes", tagctr);
										eof_log(eof_log_string, 1);
									#endif
									break;	//Break from loop
								}

								//Read handShape tag
								ptr = strcasestr_spec(buffer, "<handShape ");
								if(ptr)
								{	//If this is a handShape tag
									char arp = 0, hand = 0;

									start = end = chordid = 0;
									if(!parse_xml_rs_timestamp("startTime", buffer, &start) || (start < 0))
									{	//If the start timestamp was not readable or was invalid
										(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading start timestamp on line #%lu.  Aborting", linectr);
										eof_log(eof_log_string, 1);
										error = 1;
										break;	//Break from inner loop
									}
									if(!parse_xml_rs_timestamp("endTime", buffer, &end) || (end < 0))
									{	//If the end timestamp was not readable or was invalid
										(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading end timestamp on line #%lu.  Aborting", linectr);
										eof_log(eof_log_string, 1);
										error = 1;
										break;	//Break from inner loop
									}
									if(!parse_xml_attribute_number("chordId", buffer, &chordid) || (chordid < 0))
									{	//If the chord ID number was not readable or was invalid
										(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading chord ID number on line #%lu.  Aborting", linectr);
										eof_log(eof_log_string, 1);
										error = 1;
										break;	//Break from inner loop
									}
									if((unsigned long)chordid >= chordlist_count)
									{	//If this chord ID was not defined in the chordTemplates tag
										(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError:  Invalid chord ID on line #%lu.  Aborting", linectr);
										eof_log(eof_log_string, 1);
										error = 1;
										break;	//Break from inner loop
									}
									if(chordlist[chordid]->tflags & EOF_NOTE_TFLAG_ARP)
									{	//If this is an arpeggio
										arp = 1;
									}
									else
									{
										char hassingle = 0;
										char firstfound = 0;
										char firstchordfound = 0;
										char note_offset = 0;
										char chord_change = 0;
										long notenum = eof_track_fixup_first_pro_guitar_note(tp, curdiff);	//Find the first note in the current track difficulty
										long prevnote = -1, nextnote = -1;
										unsigned long notepos;

										while(notenum >= 0)
										{	//While there are notes in this track difficulty to examine
											notepos = tp->note[notenum]->pos;	//Simplify
											if(notepos > (unsigned long)end)
												break;	//This note and all subsequent notes are after the scope of this handshape tag
											nextnote = eof_fixup_next_pro_guitar_note(tp, notenum);	//Find the next note in this track difficulty, if any
											if(notepos >= (unsigned long)start)
											{	//This note is within the handshape tag
												if(!firstfound)
												{	//If this is the first note found to be in the scope of the handshape tag
													if(notepos > (unsigned long)start)
													{	//If the note begins after the start of the handshape tag, it must have been a handshape phrase manually defined by the author
														note_offset = 1;	//Track this condition
													}
												}
												if(eof_note_count_colors_bitmask(tp->note[notenum]->note) == 1)
												{	//If this is a single note
													if((prevnote < 0) || (tp->note[prevnote]->pos != notepos))
													{	//If there is no previous note, or if this note starts at a different time than the previous note
														if((nextnote < 0) || (tp->note[nextnote]->pos != notepos))
														{	//If there is no next note, or if this note starts at a different time than the next note
															hassingle = 1;	//This is a single note
														}
													}
												}
												else
												{	//This is a chord
													if(firstchordfound)
													{	//If this isn't the first chord in the handshape tag's scope
														if(tp->note[notenum]->tflags & EOF_NOTE_TFLAG_CCHANGE)
														{	//If this chord represents a chord change
															chord_change = 1;	//Track this condition
														}
														else
														{	//Repeat chords are high density by default
															tp->note[notenum]->flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_HD;	//Don't keep high density explicitly flagged for them
														}
													}
													firstchordfound = 1;
												}

												prevnote = notenum;	//Track the last examined note that was in this handshape tag's scope
												firstfound = 1;
											}
											notenum = nextnote;	//Iterate to the next note in this track difficulty, if there is one
										}
										if(hassingle || note_offset || chord_change)
										{	//If this handshape tag has any of these criteria
											hand = 1;	//This would have required a handshape phrase to author
										}
									}

									//Add an arpeggio/handshape phrase to the project as appropriate
									if(arp || hand)
									{	//If the handshape tag was determined to be associated with a manually authored arpeggio or handshape phrase
										if(tp->arpeggios < EOF_MAX_PHRASES)
										{	//If another arpeggio/handshape can be stored
											tp->arpeggio[tp->arpeggios].start_pos = start;
											tp->arpeggio[tp->arpeggios].end_pos = end;
											tp->arpeggio[tp->arpeggios].difficulty = curdiff;
											if(hand)
											{	//If this should be a handshape instead of an arpeggio
												tp->arpeggio[tp->arpeggios].flags |= EOF_RS_ARP_HANDSHAPE;	//Set the appropriate flag
											}
											tp->arpeggios++;
										}
									}
								}//If this is a handShape tag
								(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
								linectr++;	//Increment line counter
								tagctr++;
							}
							if(error)
								break;	//Break from inner loop
						}//If this is an handShapes tag and it isn't empty

						(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
						linectr++;
					}//Until there was an error reading from the file or end of file is reached
					if(error)
						break;	//Break from outer loop
				}//If this is a level tag

				(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
				linectr++;
			}//Until there was an error reading from the file or end of file is reached
		}//If this is the levels tag and it isn't empty

		(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
		linectr++;	//Increment line counter
	}//Until there was an error reading from the file or end of file is reached

	if(!error)
	{	//If import succeeded
#ifdef RS_IMPORT_DEBUG
		eof_log("\tParsing complete.  Finishing up", 1);
#endif

		//Create tremolo phrases
		for(ctr3 = 0; ctr3 < 256; ctr3++)
		{	//For each of the 256 possible difficulties
			for(ctr2 = 0; ctr2 < tp->notes; ctr2++)
			{	//For each note in the track
				unsigned long startpos, endpos, count;

				if(tp->note[ctr2]->type != ctr3)
					continue;	//If the note is not in the difficulty being parsed, skip it
				if(!(tp->note[ctr2]->flags & EOF_NOTE_FLAG_IS_TREMOLO))
					continue;	//If this note is not marked as being in a tremolo, skip it

				startpos = tp->note[ctr2]->pos;	//Mark the start of this phrase
				endpos = startpos + tp->note[ctr2]->length;	//Initialize the end position of the phrase
				while(++ctr2 < tp->notes)
				{	//For the consecutive remaining notes in the track
					if(tp->note[ctr2]->type == ctr3)
					{	//If the note is in the difficulty being parsed
						if(tp->note[ctr2]->flags & EOF_NOTE_FLAG_IS_TREMOLO)
						{	//And the next note is also marked as a tremolo
							endpos = tp->note[ctr2]->pos + tp->note[ctr2]->length;	//Update the end position of the phrase
						}
						else
						{
							break;	//Break from while loop.  This note isn't a tremolo so the next pass doesn't need to check it either
						}
					}
				}
				count = tp->tremolos;
				if(tp->tremolos < EOF_MAX_PHRASES)
				{	//If the track can store the tremolo section
					tp->tremolo[count].start_pos = startpos;
					tp->tremolo[count].end_pos = endpos;
					tp->tremolo[count].flags = 0;
					tp->tremolo[count].difficulty = ctr3;	//Tremolo phrases are difficulty-specific in Rocksmith
					tp->tremolo[count].name[0] = '\0';
					tp->tremolos++;
				}
			}
		}

		//Apply string muting for notes
		for(ctr = 0; ctr < tp->notes; ctr++)
		{	//For each note in the track
			unsigned long bitmask;

			if(tp->note[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE)
			{	//If the string mute status was set during import
				for(ctr2 = 0, bitmask = 1; ctr2 < 6; ctr2++, bitmask <<= 1)
				{	//For each of the 6 supported strings
					if(tp->note[ctr]->note & bitmask)
					{	//If this string is used
						tp->note[ctr]->frets[ctr2] |= 0x80;	//Set the mute bit
					}
				}
			}
		}

		//Apply strum directions
		eof_clear_input();
		if(strum_dir && (alert("At least one chord was marked as strum up.", "Would you like to mark all non strum-up chords as strum down?", NULL, "&Yes", "&No", 'y', 'n') == 1))
		{	//If there were any up strum chords, and user opts to mark all others as down strum chords
			unsigned long bitmask, count;

			for(ctr = 0; ctr < tp->notes; ctr++)
			{	//For each note in the track
				for(ctr2 = 0, count = 0, bitmask = 1; ctr2 < 6; ctr2++, bitmask <<= 1)
				{	//For each of the 6 supported strings
					if(tp->note[ctr]->note & bitmask)
					{	//If this string is used
						count++;	//Increment counter
					}
				}
				if((count > 1) && !(tp->note[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_UP_STRUM))
				{	//If this note is a chord that isn't already marked to strum upward
					tp->note[ctr]->flags |= EOF_PRO_GUITAR_NOTE_FLAG_DOWN_STRUM;	//Mark it to strum downward
				}
			}
		}

		//Derive chord lengths from single note lengths
		eof_pro_guitar_track_sort_notes(tp);
		for(ctr = 0; ctr < tp->notes; ctr++)
		{	//For each note in the track
			if(eof_note_count_colors_bitmask(tp->note[ctr]->note) <= 1)
				continue;	//If the note is not a chord, skip it

			//Otherwise look for single notes at the same position in other difficulties
			for(ctr2 = 0; ctr2 < tp->notes; ctr2++)
			{	//For each note in the track
				if(tp->note[ctr2]->pos > tp->note[ctr]->pos)
				{	//If the note is beyond the note from the outer loop
					break;	//None of the remaining notes will be at the appropriate position
				}
				else if(tp->note[ctr2]->pos == tp->note[ctr]->pos)
				{	//If the notes are at the same position
					if(tp->note[ctr2]->length > tp->note[ctr]->length)
					{	//If the note in the inner loop is longer (ie. a single note)
						tp->note[ctr]->length = tp->note[ctr2]->length;	//Assign its length to the chord from the outer loop
					}
				}
			}
		}

		//Adjust string count
		if((tp->arrangement == 4) && four_string)
		{	//If the XML defined the arrangement as a bass arrangement and all chords and notes were limited to the 4 lower strings
			tp->numstrings = 4;
		}

	}//If import succeeded

	//Cleanup
	for(ctr = 0; ctr < phraselist_count; ctr++)
	{	//For each phrase name that was stored
		free(phraselist[ctr]);	//Free it
	}
	phraselist_count = 0;
	for(ctr = 0; ctr < eventlist_count; ctr++)
	{	//For each text event that was stored
		if(!error)
		{	//If import succeeded
			for(ctr2 = 0; ctr2 < eof_song->beats; ctr2++)
			{	//For each beat in the project
				if((ctr2 + 1 >= eof_song->beats) || (eventlist[ctr]->beat < eof_song->beat[ctr2 + 1]->pos))
				{	//If this text event falls before the next beat, or if there isn't another beat
					(void) eof_song_add_text_event(eof_song, ctr2, eventlist[ctr]->text, eof_selected_track, eventlist[ctr]->flags, 0);	//Add the event to this beat
					break;
				}
			}
		}
		free(eventlist[ctr]);	//Free the text event, since it's been added to the project
	}

	//Convert imported events to time signature changes
	if(eof_use_ts)
	{	//If the user opted to import TS changes
		for(ctr = eof_song->text_events; ctr > 0; ctr--)
		{	//For each text event (in reverse order)
			char buffer3[5] = {0};
			char *ptr3 = strcasestr_spec(eof_song->text_event[ctr - 1]->text, "TS:");
			unsigned index = 0;
			long num, den;

			if(!ptr3)
				continue;	//If this text event is not formatted like a time signature change, skip it

			while((index < 5) && (*ptr3 != '/'))
			{	//Copy the numerator into the buffer
				buffer3[index++] = *ptr3;
				ptr3++;
			}
			if(!index || (index >= 5))
				continue;	//Skip converting this text event if an error was encountered
			buffer3[index] = '\0';		//Terminate the buffer
			num = atol(buffer3);		//Convert the string to a number
			if(!num || (num > 999) || (*ptr3 != '/'))
				continue;	//Skip converting this text event if an error was encountered
			ptr3++;	//Advance past the forward slash
			index = 0;
			while((index < 5) && (*ptr3 != '\0'))
			{	//Copy the denominator into the buffer
				buffer3[index++] = *ptr3;
				ptr3++;
			}
			if(!index || (index >= 5))
				continue;	//Skip converting this text event if an error was encountered
			buffer3[index] = '\0';		//Terminate the buffer
			den = atol(buffer3);		//Convert the string to a number
			if(!den || (den > 999))
				continue;	//Skip converting this text event if an error was encountered
			(void) eof_apply_ts(num, den, eof_song->text_event[ctr - 1]->beat, eof_song, 0);	//Add the time signature to the active project
			eof_song_delete_text_event(eof_song, ctr - 1);	//Delete the converted text event
		}
		eof_calculate_tempo_map(eof_song);	//Rebuild tempo map to account for any time signature changes
	}

	eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current
	for(ctr = 0; ctr < chordlist_count; ctr++)
	{	//For each chord template that was stored
		free(chordlist[ctr]);	//Free it
	}
	(void) pack_fclose(inf);
	free(buffer);
	free(buffer2);

	if(!error)
	{	//If import succeeded
		eof_sort_events(eof_song);
		eof_pro_guitar_track_sort_fret_hand_positions(tp);

		return tp;
	}

	for(ctr = 0; ctr < tp->pgnotes; ctr++)
	{	//For each normal note that was imported
		free(tp->pgnote[ctr]);
	}
	for(ctr = 0; ctr < tp->technotes; ctr++)
	{	//For each tech note that was imported
		free(tp->technote[ctr]);
	}
	free(tp);

	return NULL;
}
