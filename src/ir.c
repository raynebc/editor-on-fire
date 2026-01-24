#include <math.h>
#include "beat.h"
#include "event.h"
#include "lc_import.h"
#include "ir.h"
#include "main.h"
#include "midi.h"
#include "mix.h"
#include "rs.h"
#include "silence.h"
#include "undo.h"
#include "utility.h"
#include "menu/track.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

#define EOF_DEFAULT_TIME_DIVISION 480 // default time division used to convert midi_pos to msec_pos

int eof_ir_export_allow_fhp_finger_placements = 0;
int eof_ir_export_midi_wrote_finger_placements = 0;

int qsort_helper_immerrock(const void * e1, const void * e2)
{
//	eof_log("qsort_helper_immerrock() entered");
	char event1_is_note_on = 0, event1_is_note_off = 0, event2_is_note_on = 0, event2_is_note_off = 0;

	EOF_MIDI_EVENT ** thing1 = (EOF_MIDI_EVENT **)e1;
	EOF_MIDI_EVENT ** thing2 = (EOF_MIDI_EVENT **)e2;

	/* Chronological order takes precedence in sorting */
	if((*thing1)->pos < (*thing2)->pos)
		return -1;
	if((*thing1)->pos > (*thing2)->pos)
		return 1;

	/* Channel number takes second precedence */
	if((*thing1)->channel < (*thing2)->channel)
		return -1;
	if((*thing1)->channel > (*thing2)->channel)
		return 1;

	/* The index variable takes third precedence */
	if((*thing1)->index && (*thing2)->index)
	{	//If the index values of both events are nonzero
		if((*thing1)->index < (*thing2)->index)
			return -1;
		if((*thing2)->index < (*thing1)->index)
			return 1;
	}

	/* Track whether events 1 or 2 are note on/off events to simplify remaining checks */
	if((*thing1)->type == 0x90)
	{	//Event 1 is a note on event
		if(((*thing1)->velocity) == 0)
			event1_is_note_off = 1;	//Note on with velocity 0 is always treated as a note off
		else
			event1_is_note_on = 1;
	}
	if((*thing2)->type == 0x90)
	{	//Event 2 is a note on event
		if(((*thing2)->velocity) == 0)
			event2_is_note_off = 1;	//Note on with velocity 0 is always treated as a note off
		else
			event2_is_note_on = 1;
	}
	if((*thing1)->type == 0x80)
	{	//Event 1 is a note off event
		event1_is_note_off = 1;
	}
	if((*thing2)->type == 0x80)
	{	//Event 2 is a note off event
		event2_is_note_off = 1;
	}

	/* Pitch bend before note off takes fourth precedence */
	if(((*thing1)->type == 0xE0) && event2_is_note_off)
	{	//If the first event is a pitch bend and the other is a note off
		return -1;
	}
	if(((*thing2)->type == 0xE0) && event1_is_note_off)
	{	//If the first event is a note off and the other is a pitch bend
		return 1;
	}

	/* Note on/off event takes fourth precedence */
	if((event1_is_note_off || event1_is_note_on) && (!event2_is_note_off && !event2_is_note_on))
	{	//If the first event is a note on/off event and the other is not
		return -1;
	}
	if((event2_is_note_off || event2_is_note_on) && (!event1_is_note_off && !event1_is_note_on))
	{	//If the second event is a note on/off event and the other is not
		return 1;
	}

	/* Note number takes fifth precedence */
	if((*thing1)->note < (*thing2)->note)
		return -1;
	if((*thing1)->note > (*thing2)->note)
		return 1;

	/* Note off before Note on takes sixth precedence */
	if(event1_is_note_on && event2_is_note_off)
		return 1;
	if(event1_is_note_off && event2_is_note_on)
		return -1;

	// they are equal...
	return 0;
}

void eof_add_midi_pitch_bend_event(unsigned long pos, unsigned value, int channel, unsigned long index)
{
	unsigned char lsb, msb;

	lsb = value & 0x7F;			//The first seven bits of the bend value
	msb = (value >> 7) & 0x7F;	//The next seven bits of the bend value
	eof_add_midi_event_indexed(pos, 0xE0, lsb, msb, channel, index);
}

void eof_add_midi_pitch_bend_event_qsteps(unsigned long pos, unsigned quarter_steps, int channel, unsigned long index)
{
	unsigned long bendvalue = 8192 + (quarter_steps * 640);	//This is encoded in the MIDI event as the 7 least significant bits written in one byte, then the 7 most significant bits written as another byte

	eof_add_midi_pitch_bend_event(pos, bendvalue, channel, index);
}

int eof_export_immerrock_midi(EOF_SONG *sp, unsigned long track, unsigned char diff, char *fn)
{
	unsigned char header[14] = {'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 1, 0, 1, (EOF_DEFAULT_TIME_DIVISION >> 8), (EOF_DEFAULT_TIME_DIVISION & 0xFF)}; //The last two bytes are the time division
	unsigned long timedivision = EOF_DEFAULT_TIME_DIVISION;	//Unless the project is storing a tempo track, EOF's default time division will be used
	struct Tempo_change *anchorlist=NULL;	//Linked list containing tempo changes
	PACKFILE * fp;
	unsigned long i, stringnum, bitmask, deltapos, deltalength, channelctr;
	unsigned long delta = 0, nextdeltapos;
	int channel;
	int technique_vel[6] = {1, 6, 11, 16, 21, 26};	//Technique markers denote the affected string based on the velocity (ie. low E techniques are written with velocity 1)
	int finger_marker[6] = {0, 31, 32, 33, 34, 35};	//These are the finger placement marker note numbers for index (finger value 1) through thumb (finger value 5)
	unsigned long lastdelta=0;			//Keeps track of the last anchor's absolute delta time
	unsigned long totaltrackcounter = 0;	//Tracks the number of tracks to write to file
	EOF_MIDI_TS_LIST *tslist=NULL;		//List containing TS changes
	EOF_MIDI_KS_LIST *kslist;
	char notetempname[25] = {0};	//The temporary file created to store the binary content for the note MIDI track
	char tempotempname[30];
	unsigned char pitchmask, pitches[6] = {0};
	char notename[EOF_NAME_LENGTH+1] = {0};
	char *arrangement_name;
	char has_stored_tempo;		//Will be set to nonzero if the project contains a stored tempo track, which will affect timing conversion
	int lastevent = 0;	//Track the last event written so running status can be utilized
	char restore_tech_view = 0;			//If tech view is in effect, it is temporarily disabled so that the correct notes are exported
	char has_notes = 0;		//Track whether there's at least one note in the target track difficulty
	int error = 0;
	long next;
	unsigned long pad = EOF_DEFAULT_TIME_DIVISION / 2;	//To make notes more visible in IMMERROCK, pad to a minimum length of 1/8 (in #/4 meter) if possible without overlapping other notes
	EOF_PRO_GUITAR_TRACK *tp;
	int is_muted;				//Track whether a note is fully string muted
	unsigned long index = 1;	//Used to set the sort order for multiple pairs of note on/off events at the same timestamp as required by IMMERROCK
	unsigned long notes_written = 0;

	eof_log("eof_export_immerrock_midi() entered", 1);

	eof_ir_export_midi_wrote_finger_placements = 0;	//Reset this status
	if(!sp || !fn || (track >= sp->tracks) || (!eof_track_is_pro_guitar_track(sp, track)))
	{
		eof_log("\tError saving:  Invalid parameters", 1);
		return 0;	//Return failure
	}

	if(eof_validate_temp_folder())
	{	//Ensure the correct working directory and presence of the temporary folder
		eof_log("\tCould not validate working directory and temp folder", 1);
		return 0;	//Return failure
	}

	if(!eof_calculate_beat_delta_positions(sp, EOF_DEFAULT_TIME_DIVISION))
	{	//Calculate the delta position of each beat in the chart
		eof_log("\tCould not build beat delta positions", 1);
		return 0;	//Return failure
	}

	has_stored_tempo = eof_song_has_stored_tempo_track(sp) ? 1 : 0;	//Store this status

	//Build tempo and TS lists
	if(!eof_build_tempo_and_ts_lists(sp, &anchorlist, &tslist, &timedivision))
	{
		eof_log("\tError saving:  Cannot build tempo or TS list", 1);
		return 0;	//Return failure
	}
	kslist = eof_build_ks_list(sp);		//Build a list of key signature changes
	if(!kslist)
	{
		eof_log("\tError saving:  Cannot build key signature list", 1);
		eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
		eof_destroy_ts_list(tslist);		//Free memory used by the TS change list
		return 0;	//Return failure
	}
	header[12] = timedivision >> 8;		//Update the MIDI header to reflect the time division (which may have changed if a stored tempo track is present)
	header[13] = timedivision & 0xFF;

	//Generate temporary filenames
	(void) snprintf(tempotempname, sizeof(tempotempname) - 1, "%stempo.tmp", eof_temp_path_s);
	(void) snprintf(notetempname, sizeof(notetempname) - 1, "%seofnote.tmp", eof_temp_path_s);

	eof_sort_notes(sp);
	eof_sort_events(sp);
	eof_clear_midi_events();
	memset(eof_midi_note_status,0,sizeof(eof_midi_note_status));	//Clear note status array
	restore_tech_view = eof_menu_track_get_tech_view_state(sp, track);
	eof_menu_track_set_tech_view_state(sp, track, 0);	//Disable tech view if applicable
	tp = sp->pro_guitar_track[sp->track[track]->tracknum];

	//Write notes
	for(i = 0; i < eof_get_track_size(sp, track); i++)
	{	//For each note in the track
		unsigned long pos	, length, note, flags;
		unsigned char finger;

		if(!eof_note_applies_to_diff(sp, track, i, diff))
		{	//If this note isn't in the target difficulty (static or dynamic as applicable)
			continue;	//Skip it
		}

		pitchmask = eof_get_midi_pitches(sp, track, i, pitches);	//Determine how many exportable pitches this note/lyric has
		is_muted = eof_is_string_muted(sp, track, i);			//Determine if all played strings in the note are string muted (which would result in pitchmask being 0)
		if(!pitchmask && !is_muted)
			continue;	//If no pitches would be exported for this note/lyric, and that's not due to all played strings being string muted, skip it

		pos = eof_get_note_pos(sp, track, i);	//Cache these
		length = eof_get_note_length(sp, track, i);
		note = eof_get_note_note(sp, track, i);
		flags = eof_get_note_flags(sp, track, i);

		//Write note pitches
		deltapos = eof_ConvertToDeltaTime(pos, anchorlist, tslist, timedivision, 1, has_stored_tempo);
		deltalength = eof_ConvertToDeltaTime(pos + length, anchorlist, tslist, timedivision, 0, has_stored_tempo) - deltapos;
		next = eof_track_fixup_next_note_applicable_to_diff(sp, track, i, diff);	//Look up the next note applicable to this difficulty
		if(next >= 0)
		{	//If there is another note
			nextdeltapos = eof_ConvertToDeltaTime(eof_get_note_pos(sp, track, next), anchorlist, tslist, timedivision, 1, has_stored_tempo);	//Store its tick position
		}
		else
		{
			nextdeltapos = 0;
		}
		if(deltalength < 1)
		{	//If some kind of rounding error or other issue caused the delta length to be less than 1, force it to the minimum length of 1
			deltalength = 1;
		}
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tNote #%lu (pos = %lu, deltapos = %lu, length = %lu, delta length = %lu)", i, pos, deltapos, length, deltalength);
		eof_log(eof_log_string, 2);
		if(deltalength < pad)
		{	//If the note being exported is shorter than 1/64
			if(!nextdeltapos || (nextdeltapos > deltapos + pad))
			{	//If there is no next note or there is one and it is far away enough
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tPadding note #%lu (pos = %lu, deltapos = %lu, length = %lu, delta length = %lu) to %lu delta ticks", i, pos, deltapos, length, deltalength, pad);
				eof_log(eof_log_string, 2);
				deltalength = pad;
			}
			else
			{	//The next note isn't the full pad length away to avoid overlapping
				if(nextdeltapos > deltapos + 1)
				{	//If the note can be extended at least 1 ms without overlapping the next note
					unsigned long extend = nextdeltapos - deltapos - 1;	//This is as far as the note can be extended;

					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tPadding note #%lu (pos = %lu, deltapos = %lu, length = %lu, delta length = %lu) to %lu delta ticks (less than the desired padding of %lu ticks)", i, pos, deltapos, length, deltalength, extend, pad);
					eof_log(eof_log_string, 2);
					deltalength = extend;
				}
			}
		}
		for(stringnum = 0, bitmask = 1; stringnum < 6; stringnum++, bitmask <<= 1)
		{	//For each of the 6 usable strings
			if((note & bitmask) && !(eof_get_note_ghost(sp, track, i) & bitmask))
			{	//If this string is used and is not a ghost note
				channel = stringnum;	//IMMERROCK uses channel 0 for the thickest string
				eof_add_midi_event(deltapos, 0x90, pitches[stringnum], 79, channel);				//Velocity 79 indicates a note pitch definition
				eof_add_midi_event(deltapos + deltalength, 0x80, pitches[stringnum], 79, channel);
			}
		}

		//Write note techniques
		///Techniques are written in a way where each string has to have markers start and stop at the same timestamp
		///eof_add_midi_event_indexed() is used for these to ensure a correct sort order when qsort() is used
		for(stringnum = 0, bitmask = 1; stringnum < 6; stringnum++, bitmask <<= 1)
		{	//For each of the 6 usable strings (using velocities 1, 6, 11, 16, 21, 26 respectively)
			int retval;
			channel = stringnum;	//IMMERROCK uses channel 0 for the thickest string
			if(note & bitmask)
			{	//If this string is used
				EOF_RS_TECHNIQUES tech = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};		//Used to process bend points

				///Change all of these so the note off for each marker is the same timestamp as the note on
				if(flags & EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE)
				{	//If this note is palm muted
					eof_add_midi_event_indexed(deltapos, 0x90, 12, technique_vel[stringnum], 15, index++);		//Note 12, channel 15 with the string's dedicated velocity number indicates palm mute in IMMERROCK
					eof_add_midi_event_indexed(deltapos, 0x80, 12, 0, 15, index++);
				}
				if((flags & EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE) || (tp->note[i]->frets[stringnum] & 0x80))
				{	//If this note is fully string muted, or this specific string is
					eof_add_midi_event_indexed(deltapos, 0x90, 13, technique_vel[stringnum], 15, index++);		//Note 13, channel 15 with the string's dedicated velocity number indicates string mute in IMMERROCK
					eof_add_midi_event_indexed(deltapos, 0x80, 13, 0, 15, index++);
				}
				if(flags & EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC)
				{	//If this note is a natural harmonic
					eof_add_midi_event_indexed(deltapos, 0x90, 14, technique_vel[stringnum], 15, index++);		//Note 14, channel 15 with the string's dedicated velocity number indicates natural harmonic in IMMERROCK
					eof_add_midi_event_indexed(deltapos, 0x80, 14, 0, 15, index++);
				}
				if(flags & (EOF_PRO_GUITAR_NOTE_FLAG_HO | EOF_PRO_GUITAR_NOTE_FLAG_PO))
				{	//If this note is a hammer on or a pull off
					eof_add_midi_event_indexed(deltapos, 0x90, 15, technique_vel[stringnum], 15, index++);		//Note 15, channel 15 with the string's dedicated velocity number indicates hammer on or pull off in IMMERROCK
					eof_add_midi_event_indexed(deltapos, 0x80, 15, 0, 15, index++);
				}
				if(flags & EOF_PRO_GUITAR_NOTE_FLAG_TAP)
				{	//If this note is tapped
					eof_add_midi_event_indexed(deltapos, 0x90, 17, technique_vel[stringnum], 15, index++);		//Note 17, channel 15 with the string's dedicated velocity number indicates tapping in IMMERROCK
					eof_add_midi_event_indexed(deltapos, 0x80, 17, 0, 15, index++);
				}
				if(flags & EOF_PRO_GUITAR_NOTE_FLAG_DOWN_STRUM)
				{	//If this note is down strummed/picked
					eof_add_midi_event_indexed(deltapos, 0x90, 18, technique_vel[stringnum], 15, index++);		//Note 18, channel 15 with the string's dedicated velocity number indicates down strum in IMMERROCK
					eof_add_midi_event_indexed(deltapos, 0x80, 18, 0, 15, index++);
				}
				if(flags & EOF_PRO_GUITAR_NOTE_FLAG_UP_STRUM)
				{	//If this note is up strummed/picked
					eof_add_midi_event_indexed(deltapos, 0x90, 19, technique_vel[stringnum], 15, index++);		//Note 19, channel 15 with the string's dedicated velocity number indicates up strum in IMMERROCK
					eof_add_midi_event_indexed(deltapos, 0x80, 19, 0, 15, index++);
				}
				if(flags & (EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP | EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN))
				{	//If this note slides up or down
					eof_add_midi_event_indexed(deltapos, 0x90, 20, technique_vel[stringnum], 15, index++);		//Note 20, channel 15 with the string's dedicated velocity number indicates slide up or down in IMMERROCK
					eof_add_midi_event_indexed(deltapos, 0x80, 20, 0, 15, index++);
				}

				//Write finger placement markers
				retval = eof_pro_guitar_note_derive_string_fingering(sp, track, i, stringnum, &finger);
				if(retval > 0)
				{	//If the fingering is manually defined or can be determined based on FHP or arpeggio/handshape definition
					if(retval == 3)
					{	//If this string does not have a manually defined fingering but it could be derived from fret hand positions
						if(!eof_ir_export_allow_fhp_finger_placements)
						{	//If the user wasn't yet prompted whether to do this
							eof_ir_export_allow_fhp_finger_placements = alert("IMMERROCK:  One or more notes have undefined fingering, but these", "can be derived from fret hand positions if you trust", "their accuracy.  Allow this?", "Yes", "No", 0, 0);
						}
						if(eof_ir_export_allow_fhp_finger_placements != 1)
						{	//If the user does not opts to export finger placements derived from FHPs
							finger = 0;	//Remove the derived fingering and write no finger placement marker
						}
					}
					if((finger > 0) && (finger < 6))
					{	//If this fingering is valid
						eof_add_midi_event_indexed(deltapos, 0x90, finger_marker[finger], technique_vel[stringnum], 15, index++);		//The finger's allocated MIDI note, channel 15 with the string's dedicated velocity number indicates which finger is playing the string in IMMERROCK
						eof_add_midi_event_indexed(deltapos, 0x80, finger_marker[finger], 0, 15, index++);
						eof_ir_export_midi_wrote_finger_placements = 1;	//Track that at least one finger placement marker was written for this MIDI
					}
				}

				//Write bend points
				(void) eof_get_rs_techniques(sp, track, i, stringnum, &tech, 2, 1);	//Determine techniques used by this note/chordNote
				if(tech.bend)
				{	//If the note is a bend, write all applicable bend points
					unsigned long bendpoints, firstbend = 0, bendstrength_q;	//Used to parse any bend tech notes that may affect the exported note
					unsigned long stringdeltalength;		//Will track the length of this specific string's length in delta ticks (taking the stop tech note technique into account if applicable)
					unsigned long ctr;
					long nextnote;

					stringdeltalength = eof_ConvertToDeltaTime(pos + tech.length, anchorlist, tslist, timedivision, 0, has_stored_tempo) - deltapos;
					bendpoints = eof_pro_guitar_note_bitmask_has_bend_tech_note(tp, i, bitmask, &firstbend);	//Count how many bend tech notes overlap this note on the specified string
					if(!bendpoints)
					{	//If there are no bend points, write a bend point 1/3 into the note, and one at the note's end position to enforce the bend strength extending all the way to the end
						eof_add_midi_pitch_bend_event_qsteps(deltapos + (stringdeltalength / 3), tech.bendstrength_q, channel, index++);
						eof_add_midi_pitch_bend_event_qsteps(deltapos + stringdeltalength, tech.bendstrength_q, channel, index++);
					}
					else
					{	//If there's at least one bend tech note that overlaps the note being exported
						long pre_bend;
						unsigned long techflags;

						nextnote = eof_fixup_next_pro_guitar_note(tp, i);
						if(nextnote > 0)
						{	//If there was a next note
							if(pos + length == tp->pgnote[nextnote]->pos)
							{	//And this note extends all the way to it with no gap in between (this note has linkNext status)
								length--;	//Shorten the effective note length to ensure that a tech note at the next note's position is detected as affecting that note instead of this one
							}
						}

						//If there is a pre-bend tech note, it must be written first and any other pre-bend tech notes or bend tech notes at the note's starting position are to be ignored
						pre_bend = eof_pro_guitar_note_bitmask_has_pre_bend_tech_note(tp, i, bitmask);
						if(pre_bend >= 0)
						{	//If an applicable pre-bend tech note was found
							techflags = tp->technote[pre_bend]->flags;
							if((techflags & EOF_PRO_GUITAR_NOTE_FLAG_BEND) && (techflags & EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION))
							{	//If the tech note is a bend and has a bend strength defined
								if(tp->technote[pre_bend]->bendstrength & 0x80)
								{	//If this bend strength is defined in quarter steps
									bendstrength_q = tp->technote[pre_bend]->bendstrength & 0x7F;	//Obtain the defined bend strength in quarter steps (mask out the MSB)
								}
								else
								{	//The bend strength is defined in half steps
									bendstrength_q = tp->technote[pre_bend]->bendstrength * 2;		//Obtain the defined bend strength in quarter steps
								}
							}
							else
							{	//The bend strength is not defined, use a default value of 1 half step
								bendstrength_q = 2;
							}

							//Write the bend point explicitly at the note's start position regardless of the tech note's actual position
							eof_add_midi_pitch_bend_event_qsteps(deltapos, bendstrength_q, channel, index++);
						}

						for(ctr = firstbend; ctr < tp->technotes; ctr++)
						{	//For all tech notes, starting with the first applicable bend tech note
							unsigned long deltatechpos;

							if(tp->technote[ctr]->pos > pos + length)
							{	//If this tech note (and all those that follow) are after the end position of this note
								break;	//Break from loop, no more overlapping notes will be found
							}
							if((tp->technote[ctr]->type != tp->pgnote[i]->type) || !(bitmask & tp->technote[ctr]->note))
								continue;	//If the tech note isn't in the same difficulty as the pro guitar single note being exported or if it doesn't use the same string, skip it
							if(tp->technote[ctr]->pos < pos)
								continue;	//If this tech note does not overlap with the specified pro guitar regular note, skip it
							if(tp->technote[ctr]->eflags & EOF_PRO_GUITAR_NOTE_EFLAG_PRE_BEND)
								continue;	//If this is a pre-bend tech note, skip it as there's only one valid pre-bend per string and it was written already
							if((pre_bend >= 0) && (tp->technote[ctr]->pos == pos))
								continue;	//If a pre-bend tech note was written, and this tech note is at the note's start position, skip it as the pre-bend takes precedent in this conflict

							techflags = tp->technote[ctr]->flags;
							if((techflags & EOF_PRO_GUITAR_NOTE_FLAG_BEND) && (techflags & EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION))
							{	//If the tech note is a bend and has a bend strength defined, write the bend point at the specified position within the note
								if(tp->technote[ctr]->bendstrength & 0x80)
								{	//If this bend strength is defined in quarter steps
									bendstrength_q = tp->technote[ctr]->bendstrength & 0x7F;	//Obtain the defined bend strength in quarter steps (mask out the MSB)
								}
								else
								{	//The bend strength is defined in half steps
									bendstrength_q = tp->technote[ctr]->bendstrength * 2;		//Obtain the defined bend strength in quarter steps
								}
								deltatechpos = eof_ConvertToDeltaTime(tp->technote[ctr]->pos, anchorlist, tslist, timedivision, 1, has_stored_tempo);
								eof_add_midi_pitch_bend_event_qsteps(deltatechpos, bendstrength_q, channel, index++);
							}
						}
						eof_add_midi_pitch_bend_event_qsteps(deltapos + deltalength, bendstrength_q, channel, index++);	//Enforce the last-defined bend point at the end position of the note so the game reflects it is still in effect up to then
					}//If there's at least one bend tech note that overlaps the note being exported
				}//If the note is a bend, write all applicable bend points
				eof_add_midi_pitch_bend_event(deltapos + deltalength + 1, 8192, channel, index++);	//Set the pitch bend back to neutral 1 delta tick after the end of the note

				//Write vibrato
				if(flags & EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO)
				{
					unsigned long x = deltalength, time;	//Init x with a value that will prevent writing a vibrato pitch bend at the end of the note unless earlier pitch bends were written for the note
					long height;
					double y;
					double wave_period = 120;	//This is how many delta ticks it will take for the wave pattern to repeat (240 = every half beat)
					unsigned long pitch_bend_spacing = 28;	//How far apart in milliseconds the pitch bends are to be written
					double wave_amplitude = 384;	//This is how many units above or below the neutral value the pitch bend value changes (IMMERROCK prefers 384, which is 30% of a half-step's bend value)
					unsigned long bend_amount;	//This will reflect how much the note is bending in addition to any vibrato being performed

					if(flags & EOF_PRO_GUITAR_NOTE_FLAG_ACCENT)
					{	//If this note also has accent status
						wave_amplitude *= 2;	//Double the amplitude of the pitch bends to emphasize a stronger vibrato effect
					}

///Logic to export bend points every 30 delta ticks instead of based on realtime
/*					for(x = 0; x < deltalength; x += 30)
					{	//For each 30 delta tick interval of the note being exported
						y = sin(x * M_PI / 120.0);			//Solve for y where the sine wave is calculated to repeat its pattern every 240 ticks (where y would equal sin(2 * pi)
						if(y < 0)
							height = (320.0 * y) - 0.5;	//Scale this so that the amplitude of the vibrato bend wave pattern is 1/8 step (320 pitch bend units).  Round down to nearest negative integer
						else
							height = (320.0 * y) + 0.5;	//Scale this so that the amplitude of the vibrato bend wave pattern is 1/8 step (320 pitch bend units).  Round up to nearest positive integer
						eof_add_midi_pitch_bend_event(deltapos + x, 8192 + height, channel, index++);	//Add a pitch bend that is the given value above or below neutral
*/

					for(time = 0; time < length; time += pitch_bend_spacing)
					{	//For each time interval of the note being exported
						x = ((double)time * (double)deltalength) / (double)length;	//Get the delta tick amount into the note that this realtime position is
						y = sin(x * M_PI * 2.0 / wave_period);		//Solve for y where the sine wave is calculated to repeat its pattern in the given number of delta ticks (an x value of wave_period is where y would equal sin(2 * pi), where a sine wave officially repeats itself)
						bend_amount = eof_get_bend_strength_at_pos(sp, track, i, stringnum, pos + x) * 640;	//The pitch bend event will also add in the amount that the note bends
						if(y < 0)
							height = (wave_amplitude * y) - 0.5;	//Scale this so that the amplitude of the vibrato bend wave pattern is the specified number of pitch bend units.  Round down to nearest negative integer
						else
							height = (wave_amplitude * y) + 0.5;	//Scale this so that the amplitude of the vibrato bend wave pattern is the specified number of pitch bend units.  Round up to nearest positive integer
						eof_add_midi_pitch_bend_event(deltapos + x, 8192 + height + bend_amount, channel, index++);	//Add a pitch bend that is the given value above or below neutral
					}

					if(deltalength != x)
					{	//If the last written vibrato pitch bend was not at the stop timestamp of the note
						x = deltalength;	//The delta tick of the note's end position
						y = sin(x * M_PI * 2.0 / wave_period);		//Solve for y where the sine wave is calculated to repeat its pattern in the given number of delta ticks (an x value of wave_period is where y would equal sin(2 * pi), where a sine wave officially repeats itself)
						bend_amount = eof_get_bend_strength_at_pos(sp, track, i, stringnum, pos + length) * 640;	//The pitch bend event will also add in the amount that the note bends
						if(y < 0)
							height = (wave_amplitude * y) - 0.5;	//Scale this so that the amplitude of the vibrato bend wave pattern is the specified number of pitch bend units.  Round down to nearest negative integer
						else
							height = (wave_amplitude * y) + 0.5;	//Scale this so that the amplitude of the vibrato bend wave pattern is the specified number of pitch bend units.  Round up to nearest positive integer
						eof_add_midi_pitch_bend_event(deltapos + x, 8192 + height + bend_amount, channel, index++);	//Add a pitch bend that is the given value above or below neutral
					}
					eof_add_midi_pitch_bend_event(deltapos + deltalength + 1, 8192, channel, index++);	//Set the pitch bend back to neutral 1 delta tick after the end of the note
				}
			}//If this string is used
		}//For each of the 6 usable strings

		//Write note name if applicable
		if(eof_build_note_name(sp, track, i, notename))
		{	//If the note's name was manually defined or could be detected automatically
			char * tempstring = malloc((size_t)ustrsizez(notename));	//Allocate memory to store a copy of the note name, because chord detection will overwrite notename[] each time it is used
			if(tempstring != NULL)
			{	//If allocation was successful
				memcpy(tempstring, notename, (size_t)ustrsizez(notename));	//Copy the string to the newly allocated memory
				eof_add_midi_text_event(deltapos, tempstring, 1, 0xFFFFFFFF);	//Store the new string in a text event, send 1 for the allocation flag, because the text string is being stored in dynamic memory (provide a high index to ensure it doesn't influence sort order)
			}
		}

		if(eof_midi_event_full)
		{	//If the track exceeded the number of MIDI events that could be written
			allegro_message("Error:  Too many MIDI events, aborting MIDI export.");
			eof_log("Error:  Too many MIDI events, aborting MIDI export.", 1);
			error = 1;
		}
		has_notes = 1;
		notes_written++;
	}//For each note in the track

	//Write hand mode changes
	for(i = 0; i < tp->handmodechanges; i++)
	{	//For each hand mode change in the track
		int mode_marker = tp->handmodechange[i].end_pos ? 29 : 30;	//String mode is marked with note 29, chord mode with note 30

		deltapos = eof_ConvertToDeltaTime( tp->handmodechange[i].start_pos, anchorlist, tslist, timedivision, 0, has_stored_tempo);
		eof_add_midi_event_indexed(deltapos, 0x90, mode_marker, 100, 15, index++);		//Note 29 or 30, channel 15 indicate string hand mode or chord hand mode respectively
		eof_add_midi_event_indexed(deltapos, 0x80, mode_marker, 0, 15, index++);
	}

	if(!has_notes)
	{	//If this track has no notes (in the normal note set)
		eof_log("\tEmpty track difficulty.  Skipping export", 1);
		error = 2;
	}

	if(error)
	{
		eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
		eof_destroy_ts_list(tslist);			//Free memory used by the TS change list
		eof_destroy_ks_list(kslist);		//Free memory used by the KS change list
		eof_clear_midi_events();			//Free any memory allocated for the MIDI event array
		eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view if applicable
		return error;
	}

	//Cleanup
	for(channelctr = 0; channelctr < 16; channelctr++)
	{	//For each of the usable MIDI channels
		for(i = 0; i < 128; i++)
		{	//Ensure that any notes that are still on are terminated
			if(eof_midi_note_status[channelctr][i] == 0)
				continue;	//If this note was not left on, skip it

			//Otherwise send an alert message, as this is abnormal
			allegro_message("MIDI export error:  Note %lu was not turned off", i);
			eof_log("MIDI export error:  Note %lu was not turned off", 1);
			if(eof_midi_endbeatnum)
			{	//If the chart has a manually defined end event, that's probably the cause
				eof_clear_input();
				eof_log("\tend event was manually defined", 1);
				if(alert("It appears this is due to an [end] event that cuts out a note early", "Would you like to seek to the beat containing this [end] event?", NULL, "&Yes", "&No", 'y', 'n') == 1)
				{	//If user opts to seek to the offending event
					eof_set_seek_position(sp->beat[eof_midi_endbeatnum]->pos + eof_av_delay);
					eof_selected_beat = eof_midi_endbeatnum;
				}
			}
			eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
			eof_destroy_ts_list(tslist);			//Free memory used by the TS change list
			eof_destroy_ks_list(kslist);		//Free memory used by the KS change list
			eof_clear_midi_events();			//Free any memory allocated for the MIDI event array
			eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view if applicable
			return 0;	//Return failure
		}
	}
	///Due to how IMMERROCK defines techniques, the same MIDI note can be turned on and off multiple times at the same timestamp on the same channel
	///Removing overlapping notes will break this notation, so just use the IMMERROCK specialized quicksort
	qsort(eof_midi_event, (size_t)eof_midi_events, sizeof(EOF_MIDI_EVENT *), qsort_helper_immerrock);

	//Build temp file
	/* open the file */
	fp = pack_fopen(notetempname, "w");
	if(!fp)
	{
		eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
		eof_destroy_ts_list(tslist);			//Free memory used by the TS change list
		eof_destroy_ks_list(kslist);		//Free memory used by the KS change list
		eof_clear_midi_events();			//Free any memory allocated for the MIDI event array
		eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view if applicable
		eof_log("\tError saving:  Cannot open temporary MIDI track", 1);
		return 0;	//Return failure
	}

	/* determine the track name */
	arrangement_name = sp->track[track]->name;	//By default, the track's native name will be used
	if((sp->track[track]->flags & EOF_TRACK_FLAG_ALT_NAME) && (sp->track[track]->altname[0] != '\0'))
	{	//If this track has an alternate name defined
		arrangement_name = sp->track[track]->altname;	//Use it
	}
	else if(strcasestr_spec(sp->track[track]->name, "PART ") != NULL)
	{	//If this track's native name begins as expected
		arrangement_name = strcasestr_spec(sp->track[track]->name, "PART ");	//Use the end part of the name
	}

	/* write the track name */
	WriteVarLen(0, fp);
	(void) pack_putc(0xFF, fp);
	(void) pack_putc(0x03, fp);
	WriteVarLen(ustrsize(arrangement_name), fp);
	(void) pack_fwrite(arrangement_name, ustrsize(arrangement_name), fp);

	/* add MIDI events */
	lastdelta = 0;
	for(i = 0; i < eof_midi_events; i++)
	{
		if(eof_midi_event[i]->filtered)
			continue;	//If this event is filtered, skip it

		delta = eof_midi_event[i]->pos;
		if(eof_midi_event[i]->type == 0x01)
		{	//Write a note name text event
			eof_write_text_event(delta-lastdelta, eof_midi_event[i]->dp, fp);
			lastevent = 0;	//Reset running status after a meta/sysex event
		}
		else if(eof_midi_event[i]->type == 0x05)
		{	//Write a lyric event
			eof_write_lyric_event(delta-lastdelta, eof_midi_event[i]->dp, fp);
			lastevent = 0;	//Reset running status after a meta/sysex event
		}
		else
		{	//Write normal MIDI note events
			WriteVarLen(delta-lastdelta, fp);	//Write this event's relative delta time
			if(eof_midi_event[i]->type + eof_midi_event[i]->channel != lastevent)
			{	//With running status, the MIDI event type needn't be written if it's the same as the previous event
				(void) pack_putc(eof_midi_event[i]->type + eof_midi_event[i]->channel, fp);
			}
			(void) pack_putc(eof_midi_event[i]->note, fp);
			(void) pack_putc(eof_midi_event[i]->velocity, fp);
			lastevent = eof_midi_event[i]->type + eof_midi_event[i]->channel;
		}
		if(eof_midi_event[i]->allocation && eof_midi_event[i]->dp)
		{	//If this event has allocated memory to release
			free(eof_midi_event[i]->dp);	//Free it now
			eof_midi_event[i]->dp = NULL;
			eof_midi_event[i]->allocation = 0;
		}
		lastdelta = delta;					//Store this event's absolute delta time
	}

	/* write end of track */
	WriteVarLen(0, fp);
	(void) pack_putc(0xFF, fp);
	(void) pack_putc(0x2F, fp);
	(void) pack_putc(0x00, fp);
	(void) pack_fclose(fp);

	eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view if applicable

/* make tempo track */
	fp = pack_fopen(tempotempname, "w");
	if(!fp)
	{
		eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
		eof_destroy_ts_list(tslist);			//Free memory used by the TS change list
		eof_destroy_ks_list(kslist);		//Free memory used by the KS change list
		eof_clear_midi_events();			//Free any memory allocated for the MIDI event array
		eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view if applicable
		eof_log("\tError saving:  Cannot open temporary MIDI track", 1);
		return 0;	//Return failure
	}
	eof_write_tempo_track(NULL, anchorlist, tslist, kslist, fp);	//Write the tempo track to the temp file
	(void) pack_fclose(fp);

/* write the main MIDI file */
	fp = pack_fopen(fn, "w");
	if(!fp)
	{
		eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
		eof_destroy_ts_list(tslist);			//Free memory used by the TS change list
		eof_destroy_ks_list(kslist);		//Free memory used by the KS change list
		eof_clear_midi_events();			//Free any memory allocated for the MIDI event array
		eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view if applicable
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError saving:  Cannot open output MIDI file:  \"%s\"", strerror(errno));	//Get the Operating System's reason for the failure
		eof_log(eof_log_string, 1);
		return 0;	//Return failure
	}

	/* write header data */
	totaltrackcounter = 2;			//One tempo track and one instrument track are being written
	header[11] = totaltrackcounter;	//Write the total number of tracks present into the MIDI header
	(void) pack_fwrite(header, 14, fp);

/* write tempo track */
	(void) eof_dump_midi_track(tempotempname,fp);

/* write the instrument track */
	(void) eof_dump_midi_track(notetempname,fp);

/* cleanup */
	(void) pack_fclose(fp);	//Close the output file
	(void) delete_file(notetempname);
	(void) delete_file(tempotempname);

	eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
	eof_destroy_ts_list(tslist);			//Free memory used by the TS change list
	eof_destroy_ks_list(kslist);		//Free memory used by the KS change list
	eof_clear_midi_events();			//Free any memory allocated for the MIDI event array
	eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view if applicable
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tIMMERROCK MIDI export complete.  %lu notes written", notes_written);
	eof_log(eof_log_string, 1);

	return 1;	//Return success
}

int eof_search_image_files(char *folderpath, char *filenamebase, char *match, unsigned long match_arraysize)
{
	char filename[1024] = {0}, filepath[1024] = {0};

	if(!folderpath || !filenamebase || !match)
		return 0;

	//Build the full path to filenamebase.jpg
	strncpy(filename, filenamebase, sizeof(filename) - 1);
	(void) replace_extension(filename, filename, "jpg", sizeof(filename) - 1);
	(void) replace_filename(filepath, folderpath, filename, sizeof(filename) - 1);

	//Test whether filenamebase.jpg exists
	if(exists(filepath))
	{
		strncpy(match, filepath, match_arraysize);
		return 1;	//Return match
	}

	//Test whether filenamebase.jpeg exists
	(void) replace_extension(filepath, filepath, "jpeg", sizeof(filename) - 1);
	if(exists(filepath))
	{
		strncpy(match, filepath, match_arraysize);
		return 1;	//Return match
	}

	//Test whether filenamebase.png exists
	(void) replace_extension(filepath, filepath, "png", sizeof(filename) - 1);
	if(exists(filepath))
	{
		strncpy(match, filepath, match_arraysize);
		return 1;	//Return match
	}

	//Test whether filenamebase.tiff exists
	(void) replace_extension(filepath, filepath, "tiff", sizeof(filename) - 1);
	if(exists(filepath))
	{
		strncpy(match, filepath, match_arraysize);
		return 1;	//Return match
	}

	return 0;	//No matches
}

int eof_check_for_immerrock_album_art(char *folderpath, char *album_art_filename, unsigned long filenamesize, char force_recheck)
{
	static clock_t last_checktime = 0;
	static char first_check = 1;
	static char filematch[1024];
	clock_t this_checktime;

	if(!folderpath || !album_art_filename || !filenamesize)
		return 0;	//Invalid parameter

	album_art_filename[0] = '\0';	//Empty this string
	this_checktime = clock();
	if(force_recheck || first_check || ((this_checktime - last_checktime) / CLOCKS_PER_SEC >=  10))
	{	//If the calling function demands a recheck, if this is the first call to the function or at least 10 seconds have passed since this function performed file system checks
		eof_search_image_files(folderpath, "Cover", album_art_filename, filenamesize);
		if(album_art_filename[0] == '\0')	//If no JPG, PNG or TIFF file with a base filename of "Cover" exists in the export folder
			eof_search_image_files(folderpath, "Album", album_art_filename, filenamesize);
		if(album_art_filename[0] == '\0')	//If no JPG, PNG or TIFF file with a base filename of "Album" exists in the export folder
			eof_search_image_files(folderpath, "Label", album_art_filename, filenamesize);
		if(album_art_filename[0] == '\0')	//If no JPG, PNG or TIFF file with a base filename of "Label" exists in the export folder
			eof_search_image_files(folderpath, "Image", album_art_filename, filenamesize);

		if(exists(album_art_filename))
		{	//If a JPG, PNG or TIFF file with a base filename of "Album", "Cover", "Label" or "Image" exist in the target folder
			strncpy(filematch, album_art_filename, sizeof(filematch) - 1);	//Cache the result
		}
		else
			filematch[0] = '\0';	//Cache a lack of result

		last_checktime = clock();	//Cache the clock time of these file I/O operations
	}

	first_check = 0;
	if(filematch[0] != '\0')
	{
		strncpy(album_art_filename, filematch, filenamesize);
		return 1;	//Return cached result
	}

	return 0;
}

int eof_detect_immerrock_arrangements(unsigned long *lead, unsigned long *rhythm, unsigned long *bass)
{
	unsigned long ctr, tracknum;

	if(!eof_song || !lead || !rhythm || !bass)
		return 0;	//Invalid parameters

	*lead = *rhythm = *bass = 0;	//Reset these values

	//Select the tracks to export
	for(ctr = 1; ctr < eof_song->tracks; ctr++)
	{	//For each track
		if(eof_track_is_pro_guitar_track(eof_song, ctr))
		{	//If it is a pro guitar track
			if(eof_get_track_size_normal(eof_song, ctr))
			{	//If the track has any normal notes
				tracknum = eof_song->track[ctr]->tracknum;	//Simplify
				switch(eof_song->pro_guitar_track[tracknum]->arrangement)
				{
					case 2:	//Rhythm arrangement
						if(!(*rhythm))
							*rhythm = ctr;	//Track the first rhythm arrangement encountered
					break;

					case 3:	//Lead arrrangement
						if(!(*lead))
							*lead = ctr;	//Track the first lead arrangement encountered
					break;

					case 4:	//Bass arrangement
						if(!(*bass))
							*bass = ctr;	//Track the first bass arrangement encountered
					break;
				}
			}
		}
	}

	return 1;
}

unsigned long eof_count_immerrock_sections(void)
{
	unsigned long ctr, lead = 0, rhythm = 0, bass = 0, section_count = 0;
	char *ptr;

	if(!eof_song)
		return 0;

	(void) eof_detect_immerrock_arrangements(&lead, &rhythm, &bass);

	for(ctr = 0; ctr < eof_song->text_events; ctr++)
	{	//For each text event in the project
		if(!eof_song->text_event[ctr]->track || (eof_song->text_event[ctr]->track == lead) || (eof_song->text_event[ctr]->track == rhythm) || (eof_song->text_event[ctr]->track == bass))
		{	//If the text event has no associated track or is specific to any of the arrangements specified for export
			ptr = NULL;
			if(eof_song->text_event[ctr]->flags & EOF_EVENT_FLAG_RS_SECTION)
			{	//If this text event is flagged as a Rocksmith section
				ptr = eof_song->text_event[ctr]->text;	//Use the section name verbatim
			}
			else
			{	//Otherwise use the presence of "section " as a way to identify the section marker
				ptr = strcasestr_spec(eof_song->text_event[ctr]->text, "section ");	//Find this substring within the text event, getting the pointer to the character that follows it
			}
			if(ptr)
			{	//If the text event is considered a section marker
				section_count++;
			}
		}
	}

	return section_count;
}

unsigned long eof_count_immerrock_chords_missing_fingering(unsigned long *total)
{
	unsigned long ctr, tracknum, count = 0, totalcount = 0;
	char restore_tech_view;
	EOF_PRO_GUITAR_TRACK *tp;
	unsigned char diff = eof_note_type;	//By default, only check notes in the active track difficulty

	if(!eof_song || (!eof_track_is_pro_guitar_track(eof_song, eof_selected_track)))
		return 0;		//Only run when a pro guitar track is active

	restore_tech_view = eof_menu_track_get_tech_view_state(eof_song, eof_selected_track);
	eof_menu_track_set_tech_view_state(eof_song, eof_selected_track, 0); //Disable tech view if applicable
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	tp = eof_song->pro_guitar_track[tracknum];
	if(eof_track_has_dynamic_difficulty(eof_song, eof_selected_track))
		diff = eof_song->track[eof_selected_track]->numdiffs;	//If this is a dynamic difficulty track, check the highest difficulty level, flattened
	for(ctr = 0; ctr < tp->notes; ctr++)
	{	//For each note in the active track
		if(!eof_note_applies_to_diff(eof_song, eof_selected_track, ctr, diff))
		{	//If this note isn't in the target difficulty (static or dynamic as applicable)
			continue;	//Skip it
		}
		if(eof_note_count_rs_lanes(eof_song, eof_selected_track, ctr, 1 | 4) > 1)
		{	//If the note has at least two gems that aren't string muted (cound ghosted gems)
			if(eof_pro_guitar_note_fingering_valid(tp, ctr, 0) != 1)
			{	//If the fingering is not deemed valid for this note
				count++;
			}
			totalcount++;
		}
	}
	eof_menu_track_set_tech_view_state(eof_song, eof_selected_track, restore_tech_view); //Re-enable tech view if applicable
	if(total != NULL)
		*total = totalcount;
	return count;
}

int eof_lookup_immerrock_effective_section_at_pos(EOF_SONG *sp, unsigned long pos, char *section_name, unsigned long section_name_size)
{
	unsigned long lead = 0, rhythm = 0, bass = 0, ctr;
	char *ptr;

	if(!sp || !section_name || (section_name_size < 11))
		return 0;	//Invalid parameters

	if(eof_detect_immerrock_arrangements(&lead, &rhythm, &bass))
	{	//If the arrangement designations were determined
		section_name[0] = '\0';	//Empty this string
		for(ctr = 0; ctr < sp->text_events; ctr++)
		{	//For each text event in the project
			if(eof_get_text_event_pos(sp, ctr) > pos)
				break;	//If this text event and all others are after the target position, stop checking events

			if(!sp->text_event[ctr]->track || (sp->text_event[ctr]->track == lead) || (sp->text_event[ctr]->track == rhythm) || (sp->text_event[ctr]->track == bass))
			{	//If the text event has no associated track or is specific to any of the arrangements specified for export
				ptr = NULL;
				if(sp->text_event[ctr]->flags & EOF_EVENT_FLAG_RS_SECTION)
				{	//If this text event is flagged as a Rocksmith section
					ptr = sp->text_event[ctr]->text;	//Use the section name verbatim
				}
				else
				{	//Otherwise use the presence of "section " as a way to identify the section marker
					ptr = strcasestr_spec(sp->text_event[ctr]->text, "section ");	//Find this substring within the text event, getting the pointer to the character that follows it
				}
				if(ptr)
				{	//If the text event is considered a section marker
					strncpy(section_name, ptr, section_name_size);		//Copy the section name
					if(!(sp->text_event[ctr]->flags & EOF_EVENT_FLAG_RS_SECTION))
					{	//If the section name isn't a Rocksmith section, modify the string if necessary
						if(section_name[strlen(section_name) - 1] == ']')
						{	//If that included a trailing closing bracket
							section_name[strlen(section_name) - 1] = '\0';	//Truncate it from the end of the string
						}
					}
				}
			}
		}
		if(section_name[0] != '\0')
		{	//If a section event was found to be at/before the seek position
			return 1;	//Return section found
		}
	}
	return 0;	//No matching section found
}

unsigned char eof_pro_guitar_track_find_effective_hand_mode_change(EOF_PRO_GUITAR_TRACK *tp, unsigned long position, EOF_PHRASE_SECTION **ptr, unsigned long *index)
{
	unsigned long ctr;
	unsigned char effective = 0;	//The default hand mode in effect is 0 (chord)

	if(!tp)
		return 0;

	for(ctr = 0; ctr < tp->handmodechanges; ctr++)
	{	//For each hand mode change in the track
		if(tp->handmodechange[ctr].start_pos <= position)
		{	//If the hand mode change is at or before the specified timestamp
			effective = tp->handmodechange[ctr].end_pos;	//Track the mode that it sets
			if(ptr)
				*ptr = &tp->handmodechange[ctr];
			if(index)
				*index = ctr;
		}
		else
		{	//This hand mode change is beyond the specified timestamp
			return effective;	//Return the last hand mode change that was found (if any)
		}
	}

	return effective;	//Return the last hand mode change definition that was found (if any)
}

int eof_pro_guitar_track_set_hand_mode_change_at_timestamp(unsigned long timestamp, unsigned long mode)
{
	unsigned long tracknum;
	EOF_PHRASE_SECTION *ptr = NULL;		//If the seek position has a hand mode change defined, this will reference it
	EOF_PRO_GUITAR_TRACK *tp;

	if(!eof_song_loaded || !eof_song)
		return D_O_K;	//Do not allow this function to run if a chart is not loaded
	if(!eof_track_is_pro_guitar_track(eof_song, eof_selected_track))
		return D_O_K;	//Do not allow this function to run when a pro guitar format track is not active

	//Find the pointer to the hand mode change at the current seek position, if there is one
	tracknum = eof_song->track[eof_selected_track]->tracknum;
	tp = eof_song->pro_guitar_track[tracknum];

	(void) eof_pro_guitar_track_find_effective_hand_mode_change(tp, timestamp, &ptr, NULL);	//Look up any mode change defined at the specified timestamp
	if(ptr && (ptr->start_pos == timestamp) && (ptr->end_pos != mode))
	{	//If an existing hand mode change at the specified timestamp is to be altered
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		ptr->end_pos = mode;
	}
	else
	{	//A new hand mode change is to be added
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		(void) eof_track_add_section(eof_song, eof_selected_track, EOF_HAND_MODE_CHANGE, 0xFF, timestamp, mode, 0, "");
	}

	eof_close_menu = 1;				//Force the main menu to close, as this function had a tendency to get hung in the menu logic when activated by keyboard
	return D_O_K;
}

void eof_pro_guitar_track_delete_hand_mode_change(EOF_PRO_GUITAR_TRACK *tp, unsigned long index)
{
	unsigned long ctr;

	if(tp && (index < tp->handmodechanges))
	{
		tp->handmodechange[index].name[0] = '\0';	//Empty the name string
		for(ctr = index; ctr < tp->handmodechanges; ctr++)
		{
			memcpy(&tp->handmodechange[ctr], &tp->handmodechange[ctr + 1], sizeof(EOF_PHRASE_SECTION));
		}
		tp->handmodechanges--;
	}
}


unsigned long eof_ir_get_rs_section_instance_number(EOF_SONG *sp, unsigned long gglead, unsigned long ggrhythm, unsigned long ggbass, unsigned long event)
{
	unsigned long ctr, count = 1;
	unsigned long last_match = ULONG_MAX;	//The position at which the last match occurs

	if(!sp || (event >= sp->text_events) || !(sp->text_event[event]->flags & EOF_EVENT_FLAG_RS_SECTION))
		return 0;	//If the parameters are invalid, or the specified text event is not a Rocksmith section
	if(sp->text_event[event]->track && (sp->text_event[event]->track != gglead) && (sp->text_event[event]->track != ggrhythm) && (sp->text_event[event]->track != ggbass))
		return 0;	//If the specified event is assigned to a track other than any of the specified ones

	for(ctr = 0; ctr < event; ctr++)
	{	//For each text event in the chart that is before the specified event
		if(sp->text_event[ctr]->flags & EOF_EVENT_FLAG_RS_SECTION)
		{	//If the text event is marked as a Rocksmith section
			if(!sp->text_event[ctr]->track || (sp->text_event[ctr]->track == gglead) || (sp->text_event[ctr]->track == ggrhythm) || (sp->text_event[ctr]->track == ggbass))
			{	//If the text event is not track specific or is assigned to any of the three specified tracks
				if(sp->text_event[ctr]->pos != last_match)
				{	//If this text event isn't at the same position as the last matching text event
					if(!ustrcmp(sp->text_event[ctr]->text, sp->text_event[event]->text))
					{	//If the text event's text matches
						count++;	//Increment the instance counter
						last_match = sp->text_event[ctr]->pos;	//Track this match's position so multiple matches at the same position don't increment the count multiple times
					}
				}
			}
		}
	}

	return count;
}

int eof_export_immerrock_diff(EOF_SONG *sp, unsigned long gglead, unsigned long ggrhythm, unsigned long ggbass, unsigned char diff, char *destpath, char option, char silent)
{
	PACKFILE *fp;
	int err = 0;
	int jumpcode = 0;	//Used to catch failure by EOF_EXPORT_TO_LC()
	char temp_string[1024], section[101], temp_filename2[1024], *ptr, album_art_filename[1024];
	double avg_tempo;
	unsigned long arrctr, ctr;
	unsigned long arr[3] = {gglead, ggrhythm, ggbass};
	int arr_populated[3] = {0, 0, 0};
	int arr_finger_placements[3] = {0, 0, 0};	//Tracks whether finger placement markers were written for each arrangement
	char *diff_strings[3] = {"Lead_Difficulty", "Rhythm_Difficulty", "Bass_Difficulty"};
	char *tuning_strings[3] = {"Lead_Tuning=", "Rhythm_Tuning=", "Bass_Tuning="};
	char *midi_names[3] = {"GGLead.mid", "GGRhythm.mid", "GGBass.mid"};
	char *blank_name = "";
	char numbered_diff[10] = {0};
	char *diff_name = blank_name;
	unsigned long eventpos, lasteventpos = 0, eventswritten = 0;
	unsigned min, sec, ms;

	//Validate parameters
	if(!sp || !destpath)
		return 0;	//Invalid parameters
	if((gglead >= sp->tracks) || (ggrhythm >= sp->tracks) || (ggbass >= sp->tracks))
		return 0;	//Invalid tracks
	if(gglead && (!eof_track_is_pro_guitar_track(sp, gglead)))
		return 0;	//Not a valid lead track
	if(ggrhythm && (!eof_track_is_pro_guitar_track(sp, ggrhythm)))
		return 0;	//Not a valid lead track
	if(ggbass && (!eof_track_is_pro_guitar_track(sp, ggbass)))
		return 0;	//Not a valid lead track

	eof_log("eof_export_immerrock_track_diff() entered", 2);
	if(diff == 0xFF)
	{	//If all dynamic difficulties are to be flattened and exported
		if(gglead && eof_get_track_flattened_diff_size(sp, gglead, diff))
			arr_populated[0] = 1;
		if(ggrhythm && eof_get_track_flattened_diff_size(sp, ggrhythm, diff))
			arr_populated[1] = 1;
		if(ggbass && eof_get_track_flattened_diff_size(sp, ggbass, diff))
			arr_populated[2] = 1;
	}
	else
	{	//Otherwise only the specified difficulty is to be exported
		if(gglead && eof_get_track_diff_size_normal(sp, gglead, diff))
			arr_populated[0] = 1;
		if(ggrhythm && eof_get_track_diff_size_normal(sp, ggrhythm, diff))
			arr_populated[1] = 1;
		if(ggbass && eof_get_track_diff_size_normal(sp, ggbass, diff))
			arr_populated[2] = 1;
	}
	if(!arr_populated[0] && !arr_populated[1] && !arr_populated[2])
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tNo arrangements have notes in difficulty %u.", diff);
		eof_log(eof_log_string, 1);
		return 0;	//None of the arrangements have notes in the specified difficulty
	}
	//File>Save will call this function with a difficulty parameter of 0xFF to export the tracks with dynamic difficulty, or with a lesser value for all static difficulty tracks
	//File>Export>IMMERROCK only specifies the difficulty number to export, so the dynamic difficulty status of the track should be determined in order to
	//  properly choose a difficulty name or number to use
	if(diff < 4)
	{	//Determine whether difficulty name applies instead of number
		int use_diff_numbering = 1;

		if(gglead && !eof_track_has_dynamic_difficulty(sp, gglead))
			use_diff_numbering = 0;
		if(ggrhythm && !eof_track_has_dynamic_difficulty(sp, ggrhythm))
			use_diff_numbering = 0;
		if(ggbass && !eof_track_has_dynamic_difficulty(sp, ggbass))
			use_diff_numbering = 0;

		if(!use_diff_numbering)
		{	//If any of the tracks being exported do not have dynamic difficulty, then either File>Save is being used to save the static difficulty tracks
			// or File>Export>IMMERROCK is being used to export one static difficulty track
			diff_name =&( eof_note_type_name_rb[diff][1]);	//Skip the first character in this string, which is used to track the difficulty populated status
		}
		else
		{	//File>Export>IMMERROCK is being used to export one dynamic difficulty level of one track
			snprintf(numbered_diff, sizeof(numbered_diff) - 1, "DD %d", diff);
			diff_name = numbered_diff;
		}
	}
	else
	{
		if(diff < 0xFF)
		{	//For anything less than a specified difficulty of 0xFF, include that difficulty number
			snprintf(numbered_diff, sizeof(numbered_diff) - 1, "DD %d", diff);
		}
		else
		{
			numbered_diff[0] = '\0';	//Empty the string
		}
		diff_name = numbered_diff;
	}


	//Build the path to the IMMERROCK folder for this track difficulty
	//Use song metadata and difficulty level to build the "Artist - Song - Difficulty" string and build a subfolder of that name in the project folder
	(void) replace_filename(eof_temp_filename, destpath, "", 1024);	//Obtain the destination folder path
	put_backslash(eof_temp_filename);
	temp_string[0] = '\0';	//Empty this string
	if(eof_check_string(sp->tags->artist))
	{	//If the artist of the song is defined
		(void) ustrcat(temp_string, sp->tags->artist);
	}
	if(eof_check_string(sp->tags->title))
	{	//If the title of the song is defined
		if(temp_string[0] != '\0')
			(void) ustrcat(temp_string, " - ");
		(void) ustrcat(temp_string, sp->tags->title);
	}
	if(temp_string[0] == '\0')
	{	//If there is no defined artist or song title
		(void) ustrcat(temp_string, "IMMERROCK");	//Use this as the name for the destination folder
	}
	if(option == 1)
	{	//If a single arrangement is being exported, include its name in the export folder name
		char *arrangement_name;
		unsigned long effective_arrangement;

		if(gglead)
			effective_arrangement = gglead;
		else if(ggrhythm)
			effective_arrangement = ggrhythm;
		else
			effective_arrangement = ggbass;

		if(effective_arrangement && (effective_arrangement < sp->tracks))
		{	//Bounds check
			if((sp->track[effective_arrangement]->flags & EOF_TRACK_FLAG_ALT_NAME) && (sp->track[effective_arrangement]->altname[0] != '\0'))
			{	//If the track has an alternate name
				arrangement_name = sp->track[effective_arrangement]->altname;
			}
			else
			{	//Otherwise use the track's native name
				arrangement_name = sp->track[effective_arrangement]->name;
			}
			if(temp_string[0] != '\0')
				(void) ustrcat(temp_string, " - ");
			(void) ustrcat(temp_string, arrangement_name);
		}
	}
	if(diff_name[0] != '\0')
	{	//If a specific difficulty other than the flattened maximum dynamic difficulty is being exported
		if(temp_string[0] != '\0')
			(void) ustrcat(temp_string, " - ");
		(void) ustrcat(temp_string, diff_name);
	}

	//Build the subfolder if it doesn't already exist
	eof_build_sanitized_filename_string(temp_string, temp_filename2);	//Filter out characters that can't be used in filenames
	(void) ustrcat(eof_temp_filename, temp_filename2);	//Append to the destination folder path
	if(!eof_folder_exists(eof_temp_filename))
	{	//If the export subfolder doesn't already exist
		err = eof_mkdir(eof_temp_filename);
		if(err && !eof_folder_exists(eof_temp_filename))
		{	//If it couldn't be created and is still not found to exist (in case the previous check was a false negative)
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Could not create folder!\n%s", eof_temp_filename);
			eof_log(eof_log_string, 1);
			return 0;	//Return failure:  Could not create export folder
		}
	}
	put_backslash(eof_temp_filename);


	//Write Song.ogg
	if(eof_silence_loaded)
	{
		if(!silent)
			allegro_message("IMMERROCK:  No chart audio is loaded so the export folder will be missing audio.");

		eof_log("No chart audio is loaded so the export folder will be missing audio.", 2);
	}
	else
	{
		(void) replace_filename(eof_temp_filename, eof_temp_filename, "Song.ogg", (int) sizeof(eof_temp_filename));
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tWriting \"%s\"", eof_temp_filename);
		eof_log(eof_log_string, 2);
		if(!eof_conditionally_copy_file(eof_loaded_ogg_name, eof_temp_filename))
		{
			if(!silent)
				allegro_message("Could not export audio!\n%s", eof_temp_filename);

			return 0;	//Return failure
		}
	}


	//Write Sections.txt
	fp = NULL;	//The file will only be opened for writing if at least one section marker is found
	eof_log("\tParsing sections.",  2);
	eof_sort_events(sp);		//Ensure all events are sorted chronologically
	for(ctr = 0; ctr < sp->text_events; ctr++)
	{	//For each text event in the project
		if(!sp->text_event[ctr]->track || (sp->text_event[ctr]->track == gglead) || (sp->text_event[ctr]->track == ggrhythm) || (sp->text_event[ctr]->track == ggbass))
		{	//If the text event has no associated track or is specific to any of the arrangements specified for export
			ptr = NULL;
			eventpos = eof_get_text_event_pos(sp, ctr);
			if(eventswritten && (eventpos == lasteventpos))
			{	//If this event is at the same position as the last section event that was exported
				continue;	//Skip it
			}
			if(sp->text_event[ctr]->flags & EOF_EVENT_FLAG_RS_SECTION)
			{	//If this text event is flagged as a Rocksmith section
				unsigned long count = eof_ir_get_rs_section_instance_number(sp, gglead, ggrhythm, ggbass, ctr);	//Determine the instance number of this section within the scope of the arrangements being exported
				if(count)
				{	//If the instance number was determined
					(void) snprintf(temp_string, sizeof(temp_string) - 1, "%s %lu", sp->text_event[ctr]->text, count);	//Build a string to display it with its number
					ptr = temp_string;
				}
				else
				{
					ptr = sp->text_event[ctr]->text;	//Use the section name verbatim
				}
			}
			else
			{	//Otherwise use the presence of "section " as a way to identify the section marker
				ptr = strcasestr_spec(sp->text_event[ctr]->text, "section ");	//Find this substring within the text event, getting the pointer to the character that follows it
			}
			if(ptr)
			{	//If the text event is considered a section marker
				if(!fp)
				{	//If this is the first section event encountered
					eof_log("\t\tFound first section, creating sections.txt", 2);
					(void) replace_filename(eof_temp_filename, eof_temp_filename, "Sections.txt", (int) sizeof(eof_temp_filename));
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tWriting \"%s\"", eof_temp_filename);
					eof_log(eof_log_string, 2);
					fp = pack_fopen(eof_temp_filename, "w");
					if(!fp)
					{
						eof_log("\tError saving:  Cannot open Sections.txt for writing", 1);
						return 0;	//Return failure
					}
					else
					{	//Insert an empty section at 0 seconds to suit IMMERROCK's preferences in its Phrase Refiner feature
						if(eof_get_text_event_pos(sp, ctr) > 0)
						{	//As long as the first defined section isn't already at 0 seconds
							(void) pack_fputs("0:0 \"\"\n", fp);	//Write an empty section at that position
						}
					}
				}
				strncpy(section, ptr, sizeof(section) - 1);		//Copy the section name
				if(!(sp->text_event[ctr]->flags & EOF_EVENT_FLAG_RS_SECTION))
				{	//If the section name isn't a Rocksmith section, modify the string if necessary
					if(section[strlen(section) - 1] == ']')
					{	//If that included a trailing closing bracket
						section[strlen(section) - 1] = '\0';	//Truncate it from the end of the string
					}
				}
				min = eventpos / 60000;
				sec = (eventpos / 1000) % 60;
				ms = eventpos % 1000;
				(void) snprintf(temp_string, sizeof(temp_string) - 1, "%u:%u.%u \"\%s\"\n", min, sec, ms, section);	//Use this if IMMERROCK can display section names
				(void) pack_fputs(temp_string, fp);	//Write section entry
				lasteventpos = eventpos;	//Track the position of the last event that was exported
				eventswritten++;			//Track how many events were exported
			}
		}
	}

	//Append an empty section after all of the note content in the project to suit IMMERROCK's preferences in its Phrase Refiner feature
	if(fp)
	{	//If any sections were written
		eventpos = eof_determine_chart_length(sp) + 1;	//Obtain the timestamp of the end of the project's content, plus 1 millisecond
		min = eventpos / 60000;
		sec = (eventpos / 1000) % 60;
		ms = eventpos % 1000;
		(void) snprintf(temp_string, sizeof(temp_string) - 1, "%u:%u.%u \"\"\n", min, sec, ms);
		(void) pack_fputs(temp_string, fp);	//Write an empty section  at that position
		(void) pack_fclose(fp);
	}


	//Write Lyrics.txt
	if(sp->vocal_track[0]->lyrics)
	{	//If there are lyrics, export them in IMMERROCK's LRC variant format
		EOF_VOCAL_TRACK *tp = sp->vocal_track[0];	//Simplify
		EOF_LYRIC *temp_lyric = NULL;
		char newline = 0;
		unsigned long last_ending = tp->lyric[tp->lyrics - 1]->pos + tp->lyric[tp->lyrics - 1]->length;
		unsigned long hide_pos;	//The timestamp at which a "Hide();" lyric entry will be added, which is required to avoid a game crash if lyrics are displayed

		hide_pos = last_ending + 3000;
		temp_lyric = eof_track_add_create_note(sp, EOF_TRACK_VOCALS, 0, hide_pos, 1000, 0, "Hide();");	//Append a temporary control event to hide the lyrics 3 seconds after that ending
		if(eof_vocal_track_add_line(tp, hide_pos, hide_pos + 1000, 0xFF))
			newline = 1;	//Track whether a new lyric line was successfully added to encompass that control event

		qsort(tp->line, (size_t)tp->lines, sizeof(EOF_PHRASE_SECTION), eof_song_qsort_phrase_sections);	//Sort the lyric lines
		(void) replace_filename(eof_temp_filename, eof_temp_filename, "Lyrics.txt", (int) sizeof(eof_temp_filename));
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tWriting \"%s\"", eof_temp_filename);
		eof_log(eof_log_string, 2);
		jumpcode=setjmp(jumpbuffer); //Store environment/stack/etc. info in the jmp_buf array
		if(jumpcode!=0) //if program control returned to the setjmp() call above returning any nonzero value
		{	//Lyric export failed
			(void) puts("Assert() handled successfully!");
			if(!silent)
				allegro_message("IMMERROCK lyric export failed.\nMake sure there are no Unicode or extended ASCII characters in EOF's folder path,\nbecause EOF's lyric export doesn't support them.");
		}
		else
		{
			(void) EOF_EXPORT_TO_LC(sp,eof_temp_filename,NULL,ILRC_FORMAT);	//Import lyrics into FLC lyrics structure and export to IMMERROCK LRC format
		}
		if(newline)
		{	//If a temporary lyric line was added
			if(tp->line[tp->lines - 1].start_pos != hide_pos)
			{	//If the last lyric line in the vocal track isn't the one that was created just above
				eof_log("!Logic error.  Lost the temporary lyric line in IMMERROCK lyric export", 1);
			}
			else
			{	//Delete the line
				eof_vocal_track_delete_line(tp, tp->lines - 1);
			}
		}
		if(temp_lyric)
		{	//If a temporary lyric was added
			if(tp->lyric[tp->lyrics - 1]->pos != hide_pos)
			{	//If the last lyric in the vocal track isn't the one that was created just above
				eof_log("!Logic error.  Lost the temporary lyric in IMMERROCK lyric export", 1);
			}
			else
			{	//Delete the lyric
				eof_vocal_track_delete_lyric(tp, tp->lyrics - 1);
			}
		}
	}


	//Write album art
	album_art_filename[0] = '\0';	//Empty this string
	(void) replace_filename(eof_temp_filename, eof_temp_filename, "", (int) sizeof(eof_temp_filename));	//Build path to export folder
	if(!eof_search_image_files(eof_temp_filename, "Cover", album_art_filename, sizeof(album_art_filename) - 1))
	{	//If no JPG, PNG or TIFF file with a base filename of "Cover" exists in the export folder
		eof_log("\tAlbum art not found in export folder, checking project folder.", 2);
		eof_check_for_immerrock_album_art(eof_song_path, album_art_filename, sizeof(album_art_filename) - 1, 1);	//Check for any suitable files in the project folder
		if(exists(album_art_filename))
		{	//If a JPG, PNG or TIFF file with a base filename of "Album", "Cover", "Label" or "Image" exist in the project folder
			(void) replace_extension(temp_filename2, "Cover.jpg", get_extension(album_art_filename), sizeof(temp_filename2) - 1);	//Build output filename based on the extension of the found file
			(void) replace_filename(eof_temp_filename, eof_temp_filename, get_filename(temp_filename2), (int) sizeof(eof_temp_filename));	//Build path to destination file
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tWriting \"%s\"", eof_temp_filename);
			eof_log(eof_log_string, 2);
			(void) eof_conditionally_copy_file(album_art_filename, eof_temp_filename);	//Copy the album art to the export folder if a file of the same size doesn't already exist
		}
		else
			eof_log("\tAlbum art not found in project folder.", 2);
	}
	else
		eof_log("\tAlbum art already exists in export folder, skipping copy", 2);


	//Write arrangement MIDIs
	for(arrctr = 0; arrctr < 3; arrctr++)
	{	//For each of the 3 arrangements that can be exported, in the order of lead, rhythm and bass
		if(arr_populated[arrctr])
		{	//If this arrangement was previously found to have notes in the specified difficulty
			(void) replace_filename(eof_temp_filename, eof_temp_filename, midi_names[arrctr], (int) sizeof(eof_temp_filename));
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tWriting \"%s\"", eof_temp_filename);
			eof_log(eof_log_string, 2);
			if(!eof_export_immerrock_midi(sp, arr[arrctr], diff, eof_temp_filename))
			{
				eof_log("\tFailed to export IMMERROCK MIDI", 1);
				return 0;	//Return failure
			}
			arr_finger_placements[arrctr] = eof_ir_export_midi_wrote_finger_placements;	//Keep track of whether any finger placements were written for this arrangement, to be reflected in Info.txt
		}
	}


	//Write Info.txt
	//Write this after the MIDIs, so finger placement statuses can reflect if any derived fingerings were exported
	(void) replace_filename(eof_temp_filename, eof_temp_filename, "Info.txt", (int) sizeof(eof_temp_filename));
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tWriting \"%s\"", eof_temp_filename);
	eof_log(eof_log_string, 2);
	fp = pack_fopen(eof_temp_filename, "w");
	if(!fp)
	{
		eof_log("\tError saving:  Cannot open Info.txt for writing", 1);
		return 0;	//Return failure
	}
	if(eof_check_string(sp->tags->artist))
	{	//If the string has anything other than whitespace
		(void) snprintf(temp_string, sizeof(temp_string) - 1, "Artist=%s\n", sp->tags->artist);
		(void) pack_fputs(temp_string, fp);	//Write artist name
	}
	if(eof_check_string(sp->tags->title))
	{	//If the string has anything other than whitespace
		if(diff_name[0] != '\0')
		{	//If a specific difficulty other than the flattened maximum dynamic difficulty is being exported
			(void) snprintf(temp_string, sizeof(temp_string) - 1, "Title=%s (%s)\n", sp->tags->title, diff_name);
		}
		else
		{
			(void) snprintf(temp_string, sizeof(temp_string) - 1, "Title=%s\n", sp->tags->title);
		}
		(void) pack_fputs(temp_string, fp);	//Write song title
	}
	if(eof_check_string(sp->tags->album))
	{	//If the string has anything other than whitespace
		(void) snprintf(temp_string, sizeof(temp_string) - 1, "Album=%s\n", sp->tags->album);
		(void) pack_fputs(temp_string, fp);	//Write album name
	}
	if(eof_check_string(sp->tags->year))
	{	//If the string has anything other than whitespace
		(void) snprintf(temp_string, sizeof(temp_string) - 1, "Year=%s\n", sp->tags->year);
		(void) pack_fputs(temp_string, fp);	//Write release year
	}
	(void) snprintf(temp_string, sizeof(temp_string) - 1, "Min:Sec=%lu:%02lu\n", eof_music_length / 60000, (eof_music_length / 1000) % 60);
	(void) pack_fputs(temp_string, fp);		//Write song length
	avg_tempo = 60000.0 / ((sp->beat[sp->beats - 1]->fpos - sp->beat[0]->fpos) / sp->beats);
	(void) snprintf(temp_string, sizeof(temp_string) - 1, "BPM=%lu\n", (unsigned long)(avg_tempo + 0.5));
	(void) pack_fputs(temp_string, fp);		//Write average tempo
	(void) snprintf(temp_string, sizeof(temp_string) - 1, "Lead_Fingering=%d\n", arr_finger_placements[0]);
	(void) pack_fputs(temp_string, fp);		//Write lead arrangement fingering present status
	(void) snprintf(temp_string, sizeof(temp_string) - 1, "Rhythm_Fingering=%d\n", arr_finger_placements[1]);
	(void) pack_fputs(temp_string, fp);		//Write lead arrangement fingering present status
	(void) snprintf(temp_string, sizeof(temp_string) - 1, "Bass_Fingering=%d\n", arr_finger_placements[2]);
	(void) pack_fputs(temp_string, fp);		//Write lead arrangement fingering present status

	//Write difficulty levels
	for(arrctr = 0; arrctr < 3; arrctr++)
	{	//For each of the 3 arrangements that can be exported, in the order of lead, rhythm and bass
		if(arr[arrctr])
		{	//If this arrangement is being exported
			if(sp->track[arr[arrctr]]->difficulty != 0xFF)
			{	//If this tracks's difficulty is defined
				unsigned char difflevel = sp->track[arr[arrctr]]->difficulty;
				if(difflevel < 1)	//Bounds check
					difflevel = 1;
				if(difflevel > 5)
					difflevel = 5;
				(void) snprintf(temp_string, sizeof(temp_string) - 1, "%s=%u\n", diff_strings[arrctr], difflevel);
				(void) pack_fputs(temp_string, fp);	//Write song length
			}
		}
	}

	//Write tuning strings
	for(arrctr = 0; arrctr < 3; arrctr++)
	{	//For each of the 3 arrangements that can be exported, in the order of lead, rhythm and bass
		if(!arr_populated[arrctr])	//If this arrangement doesn't have notes to export
			continue;		//Skit ip

		(void) pack_fputs(tuning_strings[arrctr], fp);
		for(ctr = 0; ctr < sp->pro_guitar_track[sp->track[arr[arrctr]]->tracknum]->numstrings; ctr++)
		{	//For each string used in the track
			if(ctr != 0)
			{	//If this isn't the first string, append a comma and a space after the last tuning that was written
				(void) pack_putc(',', fp);
				(void) pack_putc(' ', fp);
			}
			(void) snprintf(temp_string, sizeof(temp_string) - 1, "%d", sp->pro_guitar_track[sp->track[arr[arrctr]]->tracknum]->tuning[ctr] % 12);	//Write the string's tuning value (signed integer), disregarding which octave the pitch is in since tunings of more than 11 steps are only allowed for RS2
			(void) pack_fputs(temp_string, fp);	//Append the string's tuning value
		}
		(void) pack_putc('\n', fp);
	}

	(void) snprintf(temp_string, sizeof(temp_string) - 1, "ChartDelay=%ld\n", sp->tags->ogg[0].midi_offset);
	(void) pack_fputs(temp_string, fp);	//Write chart offset
	if(eof_check_string(sp->tags->genre))
	{	//If the string has anything other than whitespace
		(void) snprintf(temp_string, sizeof(temp_string) - 1, "Genre=%s\n", sp->tags->genre);
		(void) pack_fputs(temp_string, fp);	//Write genre
	}
	if(eof_check_string(sp->tags->frettist))
	{	//If the string has anything other than whitespace
		(void) snprintf(temp_string, sizeof(temp_string) - 1, "Author=%s\n", sp->tags->frettist);
		(void) pack_fputs(temp_string, fp);	//Write chart author
	}
	if(eof_check_string(sp->tags->tracknumber))
	{	//If the string has anything other than whitespace
		(void) snprintf(temp_string, sizeof(temp_string) - 1, "TrackNumber=%s\n", sp->tags->tracknumber);
		(void) pack_fputs(temp_string, fp);	//Write track number
	}
	(void) pack_fclose(fp);


	return 1;	//Return success
}

void eof_export_immerrock(char silent)
{
	unsigned long gglead = 0, ggrhythm = 0, ggbass = 0;			//The arrangements identified for export with static difficulties
	unsigned long ddgglead = 0, ddggrhythm = 0, ddggbass = 0;	//The arrangements identified for export as a single dynamic difficulty
	char *condition1 = "! No arrangements were identified for export.\n";
	char *condition2 = "! At least one track has no defined arrangement type.  Set this with Track>Rocksmith>Arrangement Type\n";
	char *condition3 = "! At least two tracks have the same arrangement type.\n";
	char *blank = "";
	char warn_any = 0, *warn1 = blank, *warn2 = blank, *warn3 = blank;
	unsigned long ctr, tracknum;
	char newfolderpath[1024] = {0};
	char restore_tech_view = 0;			//If tech view is in effect, it is temporarily disabled so that the correct notes are exported
	int pro_guitar_content_present = 0;
	clock_t start_time, cur_time;

	if(!eof_song || !eof_song_loaded)
		return;	//If no project is loaded

	restore_tech_view = eof_menu_track_get_tech_view_state(eof_song, eof_selected_track);
	eof_menu_track_set_tech_view_state(eof_song, eof_selected_track, 0);	//Disable tech view if applicable

	eof_log("Exporting IMMERROCK files", 1);
	start_time = clock();

	//Select the tracks to export
	for(ctr = 1; ctr < eof_song->tracks; ctr++)
	{	//For each track
		if(eof_track_is_pro_guitar_track(eof_song, ctr))
		{	//If it is a pro guitar track
			if(eof_get_track_size_normal(eof_song, ctr))
			{	//If the track has any normal notes
				pro_guitar_content_present = 1;
				tracknum = eof_song->track[ctr]->tracknum;	//Simplify
				switch(eof_song->pro_guitar_track[tracknum]->arrangement)
				{
					case 2:	//Rhythm arrangement
						if(ggrhythm || ddggrhythm)
						{
							warn_any = 1;
							warn3 = condition3;	//A rhythm arrangement was already found
						}
						else
						{
							if(eof_track_has_dynamic_difficulty(eof_song, ctr))
							{	//If this track has dynamic difficulty, only the highest difficulty level will export
								(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tSelected \"%s\" as rhythm arrangement (dynamic difficulty)", eof_song->track[ctr]->name);
								eof_log(eof_log_string, 2);
								ddggrhythm = ctr;
							}
							else
							{
								(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tSelected \"%s\" as rhythm arrangement (static difficulties)", eof_song->track[ctr]->name);
								eof_log(eof_log_string, 2);
								ggrhythm = ctr;
							}
						}
					break;

					case 3:	//Lead arrrangement
						if(gglead || ddgglead)
						{
							warn_any = 1;
							warn3 = condition3;	//A rhythm arrangement was already found
						}
						else
						{
							if(eof_track_has_dynamic_difficulty(eof_song, ctr))
							{	//If this track has dynamic difficulty, only the highest difficulty level will export
								(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tSelected \"%s\" as lead arrangement (dynamic difficulty)", eof_song->track[ctr]->name);
								eof_log(eof_log_string, 2);
								ddgglead = ctr;
							}
							else
							{
								(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tSelected \"%s\" as lead arrangement (static difficulties)", eof_song->track[ctr]->name);
								eof_log(eof_log_string, 2);
								gglead = ctr;
							}
						}
					break;

					case 4:	//Bass arrangement
						if(ggbass || ddggbass)
						{
							warn_any = 1;
							warn3 = condition3;	//A rhythm arrangement was already found
						}
						else
						{
							if(eof_track_has_dynamic_difficulty(eof_song, ctr))
							{	//If this track has dynamic difficulty, only the highest difficulty level will export
								(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tSelected \"%s\" as bass arrangement (dynamic difficulty)", eof_song->track[ctr]->name);
								eof_log(eof_log_string, 2);
								ddggbass = ctr;
							}
							else
							{
								(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tSelected \"%s\" as bass arrangement (static difficulties)", eof_song->track[ctr]->name);
								eof_log(eof_log_string, 2);
								ggbass = ctr;
							}
						}
					break;

					case 0:	//Undefined arrangement
					default:	//Invalid arrangement type
						warn_any = 1;
						warn2 = condition2;
					break;
				}
			}
		}
	}
	if(pro_guitar_content_present && !gglead && !ggrhythm && !ggbass && !ddgglead && !ddggrhythm && !ddggbass)
	{	//If at least one pro guitar track in the project had normal notes, but no arrangements were selected for export
		warn_any = 1;
		warn1 = condition1;
	}

	//Export all difficulty levels of the selected arrangements
	if(silent || !eof_offer_fhp_derived_finger_placements)
		eof_ir_export_allow_fhp_finger_placements = 2;	//If prompts are being suppressed, or the option to prompt for this is not enabled in export preferences, automatically decline an offer to derive missing finger placements from FHPs
	else
		eof_ir_export_allow_fhp_finger_placements = 0;	//Otherwise allow the user to be prompted
	(void) replace_filename(newfolderpath, eof_song_path, "", 1024);	//Obtain the destination path
	if(ddgglead || ddggrhythm || ddggbass)
	{	//If any of the chosen arrangements will have the flattened dynamic difficulties exported
		eof_export_immerrock_diff(eof_song, ddgglead, ddggrhythm, ddggbass, 0xFF, newfolderpath, 0, silent);	//Export the full flattened dynamic difficulty of each
	}
	//Then export the first four difficulties of any of the chosen arrangements that don't have dynamic difficulties
	eof_export_immerrock_diff(eof_song, gglead, ggrhythm, ggbass, 0, newfolderpath, 0, silent);	//Export easy
	eof_export_immerrock_diff(eof_song, gglead, ggrhythm, ggbass, 1, newfolderpath, 0, silent);	//Export medium
	eof_export_immerrock_diff(eof_song, gglead, ggrhythm, ggbass, 2, newfolderpath, 0, silent);	//Export hard
	eof_export_immerrock_diff(eof_song, gglead, ggrhythm, ggbass, 3, newfolderpath, 0, silent);	//Export expert

	if(!silent && warn_any)
	{	//If there are any warnings to display, and warnings aren't suppressed (ie. not performing quick save)
		allegro_message("IMMERROCK Export:\n%s%s%s", warn1, warn2, warn3);
	}

	eof_menu_track_set_tech_view_state(eof_song, eof_selected_track, restore_tech_view);	//Re-enable tech view if applicable
	eof_log("\tIMMERROCK export complete.", 1);

	cur_time = clock();
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tIMMERROCK export completed in %.2f seconds", (((double)cur_time - (double)start_time) / CLOCKS_PER_SEC));
	eof_log(eof_log_string, 1);
}
