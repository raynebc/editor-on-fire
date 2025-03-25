#include "beat.h"
#include "event.h"
#include "lc_import.h"
#include "main.h"
#include "midi.h"
#include "mix.h"
#include "rs.h"
#include "silence.h"
#include "utility.h"
#include "menu/track.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

#define EOF_DEFAULT_TIME_DIVISION 480 // default time division used to convert midi_pos to msec_pos
int eof_export_immerrock_midi(EOF_SONG *sp, unsigned long track, unsigned char diff, char *fn)
{
	unsigned char header[14] = {'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 1, 0, 1, (EOF_DEFAULT_TIME_DIVISION >> 8), (EOF_DEFAULT_TIME_DIVISION & 0xFF)}; //The last two bytes are the time division
	unsigned long timedivision = EOF_DEFAULT_TIME_DIVISION;	//Unless the project is storing a tempo track, EOF's default time division will be used
	struct Tempo_change *anchorlist=NULL;	//Linked list containing tempo changes
	PACKFILE * fp;
	unsigned long i, k, bitmask, deltapos, deltalength, channelctr;
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
	unsigned long pad = EOF_DEFAULT_TIME_DIVISION / 2;	//To make notes more visible in Immerrock, pad to a minimum length of 1/8 (in #/4 meter) if possible without overlapping other notes
	EOF_PRO_GUITAR_TRACK *tp;
	int is_muted;				//Track whether a note is fully string muted
	unsigned long index = 1;	//Used to set the sort order for multiple pairs of note on/off events at the same timestamp as required by Immerrock
	unsigned long notes_written = 0;

	eof_log("eof_export_immerrock_midi() entered", 1);

	if(!sp || !fn || (track >= sp->tracks) || (sp->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
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
			else if(deltapos + pad >= nextdeltapos)
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
		for(k = 0, bitmask = 1; k < 6; k++, bitmask <<= 1)
		{	//For each of the 6 usable strings
			if(note & bitmask)
			{	//If this string is used
				channel = k;	//Immerrock uses channel 0 for the thickest string
				eof_add_midi_event(deltapos, 0x90, pitches[k], 79, channel);				//Velocity 79 indicates a note pitch definition
				eof_add_midi_event(deltapos + deltalength, 0x80, pitches[k], 79, channel);
			}
		}

		//Write note techniques
		///Techniques are written in a way where each string has to have markers start and stop at the same timestamp
		///eof_add_midi_event_indexed() is used for these to ensure a correct sort order when qsort() is used
		for(k = 0, bitmask = 1; k < 6; k++, bitmask <<= 1)
		{	//For each of the 6 usable strings (using velocities 1, 6, 11, 16, 21, 26 respectively)
			if(note & bitmask)
			{	//If this string is used
				///Change all of these so the note off for each marker is the same timestamp as the note on
				if(flags & EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE)
				{	//If this note is palm muted
					eof_add_midi_event_indexed(deltapos, 0x90, 12, technique_vel[k], 15, index++);		//Note 12, channel 15 with the string's dedicated velocity number indicates palm mute in Immerrock
					eof_add_midi_event_indexed(deltapos, 0x80, 12, 0, 15, index++);
				}
				if((flags & EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE) || (tp->note[i]->frets[k] & 0x80))
				{	//If this note is fully string muted, or this specific string is
					eof_add_midi_event_indexed(deltapos, 0x90, 13, technique_vel[k], 15, index++);		//Note 13, channel 15 with the string's dedicated velocity number indicates string mute in Immerrock
					eof_add_midi_event_indexed(deltapos, 0x80, 13, 0, 15, index++);
				}
				if(flags & EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC)
				{	//If this note is a natural harmonic
					eof_add_midi_event_indexed(deltapos, 0x90, 14, technique_vel[k], 15, index++);		//Note 14, channel 15 with the string's dedicated velocity number indicates natural harmonic in Immerrock
					eof_add_midi_event_indexed(deltapos, 0x80, 14, 0, 15, index++);
				}
				if(flags & (EOF_PRO_GUITAR_NOTE_FLAG_HO | EOF_PRO_GUITAR_NOTE_FLAG_PO))
				{	//If this note is a hammer on or a pull off
					eof_add_midi_event_indexed(deltapos, 0x90, 15, technique_vel[k], 15, index++);		//Note 15, channel 15 with the string's dedicated velocity number indicates hammer on or pull off in Immerrock
					eof_add_midi_event_indexed(deltapos, 0x80, 15, 0, 15, index++);
				}
				if(flags & EOF_PRO_GUITAR_NOTE_FLAG_TAP)
				{	//If this note is tapped
					eof_add_midi_event_indexed(deltapos, 0x90, 17, technique_vel[k], 15, index++);		//Note 17, channel 15 with the string's dedicated velocity number indicates tapping in Immerrock
					eof_add_midi_event_indexed(deltapos, 0x80, 17, 0, 15, index++);
				}
				if(flags & EOF_PRO_GUITAR_NOTE_FLAG_DOWN_STRUM)
				{	//If this note is down strummed/picked
					eof_add_midi_event_indexed(deltapos, 0x90, 18, technique_vel[k], 15, index++);		//Note 18, channel 15 with the string's dedicated velocity number indicates down strum in Immerrock
					eof_add_midi_event_indexed(deltapos, 0x80, 18, 0, 15, index++);
				}
				if(flags & EOF_PRO_GUITAR_NOTE_FLAG_UP_STRUM)
				{	//If this note is up strummed/picked
					eof_add_midi_event_indexed(deltapos, 0x90, 19, technique_vel[k], 15, index++);		//Note 19, channel 15 with the string's dedicated velocity number indicates up strum in Immerrock
					eof_add_midi_event_indexed(deltapos, 0x80, 19, 0, 15, index++);
				}
				if(flags & (EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP | EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN))
				{	//If this note slides up or down
					eof_add_midi_event_indexed(deltapos, 0x90, 20, technique_vel[k], 15, index++);		//Note 20, channel 15 with the string's dedicated velocity number indicates slide up or down in Immerrock
					eof_add_midi_event_indexed(deltapos, 0x80, 20, 0, 15, index++);
				}

		//Write finger placement markers
				finger = tp->pgnote[i]->finger[k];
				if((finger > 0) && (finger < 6))
				{	//If this string has a defined fingering that is valid
					eof_add_midi_event_indexed(deltapos, 0x90, finger_marker[finger], technique_vel[k], 15, index++);		//The finger's allocated MIDI note, channel 15 with the string's dedicated velocity number indicates which finger is playing the string in Immerrock
					eof_add_midi_event_indexed(deltapos, 0x80, finger_marker[finger], 0, 15, index++);
				}
			}
		}

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
	///Due to how Immerrock defines techniques, the same MIDI note can be turned on and off multiple times at the same timestamp on the same channel
	///Removing overlapping notes will break this notation, so just use the Immerrock specialized quicksort
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
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tImmerrock MIDI export complete.  %lu notes written", notes_written);
	eof_log(eof_log_string, 1);

	return 1;	//Return success
}

int eof_export_immerrock_diff(EOF_SONG * sp, unsigned long gglead, unsigned long ggrhythm, unsigned long ggbass, unsigned char diff, char *destpath, char option)
{
	PACKFILE *fp;
	int err = 0;
	int jumpcode = 0;	//Used to catch failure by EOF_EXPORT_TO_LC()
	char temp_string[1024], section[101], temp_filename2[1024], *ptr;
	double avg_tempo;
	unsigned long arrctr, ctr;
	unsigned long arr[3] = {gglead, ggrhythm, ggbass};
	int arr_populated[3] = {0, 0, 0};
	char *diff_strings[3] = {"Lead_Difficulty", "Rhythm_Difficulty", "Bass_Difficulty"};
	char *tuning_strings[3] = {"Lead_Tuning=", "Rhythm_Tuning=", "Bass_Tuning="};
	char *midi_names[3] = {"GGLead.mid", "GGRhythm.mid", "GGBass.mid"};
	char *blank_name = "";
	char numbered_diff[10] = {0};
	char *diff_name = blank_name;

	//Validate parameters
	if(!sp || !destpath)
		return 0;	//Invalid parameters
	if((gglead >= sp->tracks) || (ggrhythm >= sp->tracks) || (ggbass >= sp->tracks))
		return 0;	//Invalid tracks
	if(gglead && sp->track[gglead]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 0;	//Not a valid lead track
	if(ggrhythm && sp->track[ggrhythm]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 0;	//Not a valid lead track
	if(ggbass && sp->track[ggbass]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
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
	if(diff < 4)
	{	//If this is one of the four named Rock Band style difficulty levels
		diff_name =&( eof_note_type_name_rb[diff][1]);	//Skip the first character in this string, which is used to track the difficulty populated status
	}
	else
	{
		if(diff < 0xFF)
		{	//For anything less than a specified difficulty of 0xFF, include that difficulty number
			snprintf(numbered_diff, sizeof(numbered_diff) - 1, "DD %d", diff);
		}
		else
		{
			snprintf(numbered_diff, sizeof(numbered_diff) - 1, "DD max");
		}
		diff_name = numbered_diff;
	}


	//Build the path to the Immerrock folder for this track difficulty
	//Use song metadata and difficulty level to build the "Artist - Song - Difficulty" string and build a subfolder of that name in the project folder
	(void) replace_filename(eof_temp_filename, destpath, "", 1024);	//Obtain the destination folder path
	put_backslash(eof_temp_filename);
	temp_string[0] = '\0';	//Empty this string
	if(eof_check_string(eof_song->tags->artist))
	{	//If the artist of the song is defined
		(void) ustrcat(temp_string, eof_song->tags->artist);
		(void) ustrcat(temp_string, " - ");
	}
	if(eof_check_string(eof_song->tags->title))
	{	//If the title of the song is defined
		(void) ustrcat(temp_string, eof_song->tags->title);
		(void) ustrcat(temp_string, " - ");
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
			(void) ustrcat(temp_string, arrangement_name);
			(void) ustrcat(temp_string, " - ");
		}
	}
	(void) ustrcat(temp_string, diff_name);

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


	//Write Song.ogg
	put_backslash(eof_temp_filename);
	(void) replace_filename(eof_temp_filename, eof_temp_filename, "Song.ogg", (int) sizeof(eof_temp_filename));
	if(!exists(eof_temp_filename))
	{	//If the OGG file doesn't already exist at the destination
		if(!eof_copy_file(eof_loaded_ogg_name, eof_temp_filename))
		{
			allegro_message("Could not export audio!\n%s", eof_temp_filename);
			return 0;	//Return failure
		}
	}


	//Write Meta.txt
	///Meta.txt will be deprecated.  Remove it after the game formally supports Info.txxt
	(void) replace_filename(eof_temp_filename, eof_temp_filename, "Meta.txt", (int) sizeof(eof_temp_filename));
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tWriting \"%s\"", eof_temp_filename);
	eof_log(eof_log_string, 2);
	fp = pack_fopen(eof_temp_filename, "w");
	if(!fp)
	{
		eof_log("\tError saving:  Cannot open Meta.txt for writing", 1);
		return 0;	//Return failure
	}
	avg_tempo = 60000.0 / ((sp->beat[sp->beats - 1]->fpos - sp->beat[0]->fpos) / sp->beats);
	(void) snprintf(temp_string, sizeof(temp_string) - 1, "%s-%s (%s)-%lu-%lu-%s", sp->tags->artist, sp->tags->title, diff_name, (unsigned long)(avg_tempo + 0.5), eof_music_length / 1000, sp->tags->year);
	(void) pack_fputs(temp_string, fp);	//Write song length
	(void) pack_fclose(fp);


	//Write Info.txt
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
		(void) snprintf(temp_string, sizeof(temp_string) - 1, "Title=%s (%s)\n", sp->tags->title, diff_name);
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
	(void) snprintf(temp_string, sizeof(temp_string) - 1, "Lead_Fingering=%d\n", eof_pro_guitar_track_diff_has_fingering(sp, gglead, diff));
	(void) pack_fputs(temp_string, fp);		//Write lead arrangement fingering present status
	(void) snprintf(temp_string, sizeof(temp_string) - 1, "Rhythm_Fingering=%d\n", eof_pro_guitar_track_diff_has_fingering(sp, ggrhythm, diff));
	(void) pack_fputs(temp_string, fp);		//Write lead arrangement fingering present status
	(void) snprintf(temp_string, sizeof(temp_string) - 1, "Bass_Fingering=%d\n", eof_pro_guitar_track_diff_has_fingering(sp, ggbass, diff));
	(void) pack_fputs(temp_string, fp);		//Write lead arrangement fingering present status

	//Write difficulty levels
	for(arrctr = 0; arrctr < 3; arrctr++)
	{	//For each of the 3 arrangements that can be exported
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
	{	//For each of the 3 arrangements that can be exported
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
	(void) pack_fclose(fp);


	//Write Sections.txt
	fp = NULL;	//The file will only be opened for writing if at least one section marker is found
	for(ctr = 0; ctr < sp->text_events; ctr++)
	{	//For each text event in the project
		if(!sp->text_event[ctr]->track || (sp->text_event[ctr]->track == gglead) || (sp->text_event[ctr]->track == ggrhythm) || (sp->text_event[ctr]->track == ggbass))
		{	//If the text event has no associated track or is specific to any of the arrangements specified for export
			ptr = strcasestr_spec(eof_song->text_event[ctr]->text, "section ");	//Find this substring within the text event
			if(ptr)
			{	//If it exists
				unsigned long eventpos;
				unsigned min, sec, ms;

				if(!fp)
				{	//If this is the first section event encountered
					(void) replace_filename(eof_temp_filename, eof_temp_filename, "Sections.txt", (int) sizeof(eof_temp_filename));
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tWriting \"%s\"", eof_temp_filename);
					eof_log(eof_log_string, 2);
					fp = pack_fopen(eof_temp_filename, "w");
					if(!fp)
					{
						eof_log("\tError saving:  Cannot open Sections.txt for writing", 1);
						return 0;	//Return failure
					}
				}
				strncpy(section, ptr, sizeof(section) - 1);		//Copy the portion of the text event that came after "section "
				if(section[strlen(section) - 1] == ']')
				{	//If that included a trailing closing bracket
					section[strlen(section) - 1] = '\0';	//Truncate it from the end of the string
				}
				eventpos = eof_get_text_event_pos(sp, ctr);
				min = eventpos / 60000;
				sec = (eventpos / 1000) % 60;
				ms = eventpos % 1000;
				(void) snprintf(temp_string, sizeof(temp_string) - 1, "%u:%u.%u \"\%s\"\n", min, sec, ms, section);	//Use this if Immerrock can display section names
				(void) pack_fputs(temp_string, fp);	//Write section entry
			}
		}
	}
	(void) pack_fclose(fp);


	//Write Lyrics.txt
	if(sp->vocal_track[0]->lyrics)
	{	//If there are lyrics, export them in Immerrock's LRC variant format
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
			allegro_message("Immerrock lyric export failed.\nMake sure there are no Unicode or extended ASCII characters in EOF's folder path,\nbecause EOF's lyric export doesn't support them.");
		}
		else
		{
			(void) EOF_EXPORT_TO_LC(sp,eof_temp_filename,NULL,ILRC_FORMAT);	//Import lyrics into FLC lyrics structure and export to Immerrock LRC format
		}
		if(newline)
		{	//If a temporary lyric line was added
			if(tp->line[tp->lines - 1].start_pos != hide_pos)
			{	//If the last lyric line in the vocal track isn't the one that was created just above
				eof_log("!Logic error.  Lost the temporary lyric line in Immerrock lyric export", 1);
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
				eof_log("!Logic error.  Lost the temporary lyric in Immerrock lyric export", 1);
			}
			else
			{	//Delete the lyric
				eof_vocal_track_delete_lyric(tp, tp->lyrics - 1);
			}
		}
	}


	//Write arrangement MIDIs
	for(arrctr = 0; arrctr < 3; arrctr++)
	{	//For each of the 3 arrangements that can be exported
		if(arr_populated[arrctr])
		{	//If this arrangement was previously found to have notes in the specified difficulty
			(void) replace_filename(eof_temp_filename, eof_temp_filename, midi_names[arrctr], (int) sizeof(eof_temp_filename));
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tWriting \"%s\"", eof_temp_filename);
			eof_log(eof_log_string, 2);
			if(!eof_export_immerrock_midi(sp, arr[arrctr], diff, eof_temp_filename))
			{
				eof_log("\tFailed to export Immerrock MIDI", 1);
				return 0;	//Return failure
			}
		}
	}

	return 1;	//Return success
}

void eof_export_immerrock(void)
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

	if(!eof_song || !eof_song_loaded)
		return;	//If no project is loaded

	restore_tech_view = eof_menu_track_get_tech_view_state(eof_song, eof_selected_track);
	eof_menu_track_set_tech_view_state(eof_song, eof_selected_track, 0);	//Disable tech view if applicable

	eof_log("Exporting Immerrock files", 1);

	//Select the tracks to export
	for(ctr = 1; ctr < eof_song->tracks; ctr++)
	{	//For each track
		if(eof_song->track[ctr]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If it is a pro guitar track
			if(eof_get_track_size_normal(eof_song, ctr))
			{	//If the track has any normal notes
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
	if(!gglead && !ggrhythm && !ggbass)
	{
		warn_any = 1;
		warn1 = condition1;
	}

	//Export all difficulty levels of the selected arrangements
	(void) replace_filename(newfolderpath, eof_song_path, "", 1024);	//Obtain the destination path
	if(ddgglead || ddggrhythm || ddggbass)
	{	//If any of the chosen arrangements will have the flattened dynamic difficulties exported
		eof_export_immerrock_diff(eof_song, ddgglead, ddggrhythm, ddggbass, 0xFF, newfolderpath, 0);	//Export the full flattened dynamic difficulty of each
	}
	//Then export the first four difficulties of any of the chosen arrangements that don't have dynamic difficulties
	eof_export_immerrock_diff(eof_song, gglead, ggrhythm, ggbass, 0, newfolderpath, 0);	//Export easy
	eof_export_immerrock_diff(eof_song, gglead, ggrhythm, ggbass, 1, newfolderpath, 0);	//Export medium
	eof_export_immerrock_diff(eof_song, gglead, ggrhythm, ggbass, 2, newfolderpath, 0);	//Export hard
	eof_export_immerrock_diff(eof_song, gglead, ggrhythm, ggbass, 3, newfolderpath, 0);	//Export expert

	if(warn_any)
		allegro_message("Immerrock Export:\n%s%s%s", warn1, warn2, warn3);

	eof_menu_track_set_tech_view_state(eof_song, eof_selected_track, restore_tech_view);	//Re-enable tech view if applicable
}
