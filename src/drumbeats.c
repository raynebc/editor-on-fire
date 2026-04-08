#include <allegro.h>
#include "drumbeats.h"
#include "event.h"
#include "main.h"
#include "midi.h"
#include "mix.h"
#include "utility.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

#define EOF_DRUMBEATS_DEFAULT_VELOCITY 100

int eof_export_drumbeats_midi(EOF_SONG *sp, unsigned long track, unsigned char diff, char *fn)
{
	unsigned char header[14] = {'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 1, 0, 1, (EOF_DEFAULT_TIME_DIVISION >> 8), (EOF_DEFAULT_TIME_DIVISION & 0xFF)}; //The last two bytes are the time division
	unsigned long timedivision = EOF_DEFAULT_TIME_DIVISION;	//Unless the project is storing a tempo track, EOF's default time division will be used
	struct Tempo_change *anchorlist=NULL;	//Linked list containing tempo changes
	PACKFILE * fp;
	unsigned long i, deltapos, deltalength, channelctr;
	unsigned long delta = 0;
	unsigned long lastdelta=0;			//Keeps track of the last anchor's absolute delta time
	unsigned long totaltrackcounter = 0;	//Tracks the number of tracks to write to file
	EOF_MIDI_TS_LIST *tslist=NULL;		//List containing TS changes
	EOF_MIDI_KS_LIST *kslist;
	char notetempname[25] = {0};	//The temporary file created to store the binary content for the note MIDI track
	char tempotempname[30];
	char *arrangement_name;
	char has_stored_tempo;		//Will be set to nonzero if the project contains a stored tempo track, which will affect timing conversion
	int lastevent = 0;	//Track the last event written so running status can be utilized
	int error = 0;
	unsigned long notes_written = 0;
	int vel;
	EOF_PHRASE_SECTION *sectionptr;

	eof_log("eof_export_drumbeats_midi() entered", 1);

	//Validate parameters
	if(!sp || !fn || (track >= sp->tracks))
	{
		eof_log("\tError saving:  Invalid parameters", 1);
		return 0;	//Return failure
	}
	if(sp->track[track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR)
	{	//Only allow drum tracks to export to this format
		eof_log("\tNot a drum track.  Aborting export", 1);
		return 0;	//Return failure
	}
	if(!eof_get_track_diff_size(sp, track, diff))
	{	//Target track difficulty has no notes
		eof_log("\tEmpty track difficulty.  Aborting export", 1);
		return 0;
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

	//Write notes
	for(i = 0; i < eof_get_track_size(sp, track); i++)
	{	//For each note in the track
		unsigned long pos	, length, note, flags, ghost;

		if(!eof_note_applies_to_diff(sp, track, i, diff))
		{	//If this note isn't in the target difficulty (static or dynamic as applicable)
			continue;	//Skip it
		}

		pos = eof_get_note_pos(sp, track, i);	//Cache these
		length = eof_get_note_length(sp, track, i);
		note = eof_get_note_note(sp, track, i);
		flags = eof_get_note_flags(sp, track, i);
		ghost = eof_get_note_ghost(sp, track, i);
		deltapos = eof_ConvertToDeltaTime(pos, anchorlist, tslist, timedivision, 1, has_stored_tempo);
		deltalength = eof_ConvertToDeltaTime(pos + length, anchorlist, tslist, timedivision, 0, has_stored_tempo) - deltapos;
		if(deltalength < 1)
		{	//If some kind of rounding error or other issue caused the delta length to be less than 1, force it to the minimum length of 1
			deltalength = 1;
		}
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tNote #%lu (pos = %lu, deltapos = %lu, length = %lu, delta length = %lu)", i, pos, deltapos, length, deltalength);
		eof_log(eof_log_string, 2);

		//Write bass drum
		if(note & 1)
		{
			vel = EOF_DRUMBEATS_DEFAULT_VELOCITY;
			eof_add_midi_event(deltapos, 0x90, 36, vel, 0);
			eof_add_midi_event(deltapos + deltalength, 0x80, 36, vel, 0);
		}

		//Write snare
		if(note & 2)
		{
			if(ghost & 2)
				vel = 1;	//If the snare is a ghost note, use the minimum velocity
			else
				vel = EOF_DRUMBEATS_DEFAULT_VELOCITY;

			eof_add_midi_event(deltapos, 0x90, 38, vel, 0);
			eof_add_midi_event(deltapos + deltalength, 0x80, 38, vel, 0);
		}

		//Write low tom, closed hi hat or open hi hat
		if(note & 4)
		{
			vel = EOF_DRUMBEATS_DEFAULT_VELOCITY;

			if((flags & EOF_DRUM_NOTE_FLAG_Y_COMBO) || !(flags & EOF_DRUM_NOTE_FLAG_Y_CYMBAL))
			{	//If this gem is marked as a yellow cymbal+tom combo, or is not marked as a yellow cymbal, write low tom
				eof_add_midi_event(deltapos, 0x90, 65, vel, 0);
				eof_add_midi_event(deltapos + deltalength, 0x80, 65, vel, 0);
			}
			if((flags & EOF_DRUM_NOTE_FLAG_Y_COMBO) || (flags & EOF_DRUM_NOTE_FLAG_Y_CYMBAL))
			{	//If this gem is marked as a yellow cymbal+tom combo, or is marked as a yellow cymbal
				if(flags & EOF_DRUM_NOTE_FLAG_Y_HI_HAT_OPEN)
				{	//If this gem is marked as an open hi hat
					eof_add_midi_event(deltapos, 0x90, 55, vel, 0);
					eof_add_midi_event(deltapos + deltalength, 0x80, 55, vel, 0);
				}
				else
				{	//This gem is a closed hi hat
					eof_add_midi_event(deltapos, 0x90, 51, vel, 0);
					eof_add_midi_event(deltapos + deltalength, 0x80, 51, vel, 0);
				}
			}
		}

		//Write mid tom or left cymbal
		if(note & 8)
		{
			vel = EOF_DRUMBEATS_DEFAULT_VELOCITY;

			if((flags & EOF_DRUM_NOTE_FLAG_B_COMBO) || !(flags & EOF_DRUM_NOTE_FLAG_B_CYMBAL))
			{	//If this gem is marked as a blue cymbal+tom combo, or is not marked as a blue cymbal, write mid tom
				eof_add_midi_event(deltapos, 0x90, 69, vel, 0);
				eof_add_midi_event(deltapos + deltalength, 0x80, 69, vel, 0);
			}
			if((flags & EOF_DRUM_NOTE_FLAG_B_COMBO) || (flags & EOF_DRUM_NOTE_FLAG_B_CYMBAL))
			{	//If this gem is marked as a blue cymbal+tom combo, or is marked as a blue cymbal, write left cymbal
				eof_add_midi_event(deltapos, 0x90, 77, vel, 0);
				eof_add_midi_event(deltapos + deltalength, 0x80, 77, vel, 0);
			}
		}

		//Write high tom or right cymbal
		if(note & 16)
		{
			vel = EOF_DRUMBEATS_DEFAULT_VELOCITY;

			if((flags & EOF_DRUM_NOTE_FLAG_G_COMBO) || !(flags & EOF_DRUM_NOTE_FLAG_G_CYMBAL))
			{	//If this gem is marked as a green cymbal+tom combo, or is not marked as a green cymbal, write high tom
				eof_add_midi_event(deltapos, 0x90, 71, vel, 0);
				eof_add_midi_event(deltapos + deltalength, 0x80, 71, vel, 0);
			}
			if((flags & EOF_DRUM_NOTE_FLAG_G_COMBO) || (flags & EOF_DRUM_NOTE_FLAG_G_CYMBAL))
			{	//If this gem is marked as a green cymbal+tom combo, or is marked as a green cymbal, write right cymbal
				eof_add_midi_event(deltapos, 0x90, 79, vel, 0);
				eof_add_midi_event(deltapos + deltalength, 0x80, 79, vel, 0);
			}
		}

		///Add new statuses for ride, bell and china cymbals, then add export logic

		if(eof_midi_event_full)
		{	//If the track exceeded the number of MIDI events that could be written
			allegro_message("Error:  Too many MIDI events, aborting MIDI export.");
			eof_log("Error:  Too many MIDI events, aborting MIDI export.", 1);
			error = 1;
		}
		notes_written++;
	}//For each note in the track

	//Write golden note (star power) markers
	for(i = 0; i < eof_get_num_star_power_paths(sp, track); i++)
	{	//For each star power path in the track
		vel = EOF_DRUMBEATS_DEFAULT_VELOCITY;
		sectionptr = eof_get_star_power_path(sp, track, i);
		deltapos = eof_ConvertToDeltaTime(sectionptr->start_pos, anchorlist, tslist, timedivision, 1, has_stored_tempo);	//Store the tick position of the phrase
		deltalength = eof_ConvertToDeltaTime(sectionptr->end_pos, anchorlist, tslist, timedivision, 0, 1) - deltapos;		//Store the number of delta ticks representing the phrase's length
		if(deltalength < 1)
		{	//If some kind of rounding error or other issue caused the delta length to be less than 1, force it to the minimum length of 1
			deltalength = 1;
		}
		eof_add_midi_event(deltapos, 0x90, 35, vel, 0);
		eof_add_midi_event(deltapos + deltalength, 0x80, 35, vel, 0);
	}

	if(error)
	{
		eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
		eof_destroy_ts_list(tslist);			//Free memory used by the TS change list
		eof_destroy_ks_list(kslist);		//Free memory used by the KS change list
		eof_clear_midi_events();			//Free any memory allocated for the MIDI event array
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
			return 0;	//Return failure
		}
	}
	qsort(eof_midi_event, (size_t)eof_midi_events, sizeof(EOF_MIDI_EVENT *), qsort_helper3);

	//Build temp file
	/* open the file */
	fp = pack_fopen(notetempname, "w");
	if(!fp)
	{
		eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
		eof_destroy_ts_list(tslist);			//Free memory used by the TS change list
		eof_destroy_ks_list(kslist);		//Free memory used by the KS change list
		eof_clear_midi_events();			//Free any memory allocated for the MIDI event array
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

/* make tempo track */
	fp = pack_fopen(tempotempname, "w");
	if(!fp)
	{
		eof_destroy_tempo_list(anchorlist);	//Free memory used by the anchor list
		eof_destroy_ts_list(tslist);			//Free memory used by the TS change list
		eof_destroy_ks_list(kslist);		//Free memory used by the KS change list
		eof_clear_midi_events();			//Free any memory allocated for the MIDI event array
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
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tDrumBeats MIDI export complete.  %lu notes written", notes_written);
	eof_log(eof_log_string, 1);

	return 1;	//Return success
}

int eof_export_drumbeats(EOF_SONG *sp, unsigned long track, char *destpath)
{
	unsigned diff;
	char diff_written[4] = {0};
	char *midi_names[4] = {"notes_easy.mid", "notes_medium.mid", "notes_hard.mid", "notes_expert.mid"};
	char temp_string[1024];
	int err;
	PACKFILE *fp;

	//Validate parameters
	if(!sp || (track >= sp->tracks))
	{
		eof_log("\tError saving:  Invalid parameters", 1);
		return 0;	//Return failure
	}
	if(sp->track[track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR)
	{	//Only allow drum tracks to export to this format
		eof_log("\tNot a drum track.  Aborting export", 1);
		return 0;	//Return failure
	}
	if(!eof_get_track_size(sp, track))
	{	//Target track has no notes
		eof_log("\tEmpty track.  Aborting export", 1);
		return 0;
	}

	eof_log("eof_export_drumbeats() entered", 2);
	(void) replace_filename(eof_temp_filename, destpath, "", 1024);	//Obtain the destination folder path

	//Write song.ogg
	if(eof_silence_loaded)
	{
		allegro_message("DrumBeats:  No chart audio is loaded so the export folder will be missing audio.");
		eof_log("No chart audio is loaded so the export folder will be missing audio.", 2);
	}
	else
	{
		(void) replace_filename(eof_temp_filename, eof_temp_filename, "song.ogg", (int) sizeof(eof_temp_filename));
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tWriting \"%s\"", eof_temp_filename);
		eof_log(eof_log_string, 2);
		if(!eof_conditionally_copy_file(eof_loaded_ogg_name, eof_temp_filename))
		{
			allegro_message("Could not export audio!\n%s", eof_temp_filename);
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Could not export audio!\n%s", eof_temp_filename);
			eof_log(eof_log_string, 2);

			return 0;	//Return failure
		}
	}

	//Write each MIDI file
	err = 0;
	for(diff = 0; diff < 4; diff++)
	{	//For each of the first four difficulties
		if(!eof_get_track_diff_size(sp, track, diff))
		{	//If there are no notes in this track difficulty
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tTrack difficulty #%u is empty.  Skipping.", diff);
			eof_log(eof_log_string, 2);
			continue;
		}
		(void) replace_filename(eof_temp_filename, eof_temp_filename, midi_names[diff], (int) sizeof(eof_temp_filename));
		if(eof_export_drumbeats_midi(sp, track, diff, eof_temp_filename))
		{
			diff_written[diff] = 1;
		}
		else
		{
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tFailed to export difficulty #%u", diff);
			eof_log(eof_log_string, 1);
			err = 1;
		}
	}

	//Write packer utility JSON
	(void) replace_filename(eof_temp_filename, eof_temp_filename, "project.json", (int) sizeof(eof_temp_filename));
	fp = pack_fopen(eof_temp_filename, "w");
	if(!fp)
	{
		eof_log("\tError saving:  Cannot open project.json for writing", 1);
		err = 1;
	}
	else
	{
		(void) pack_fputs("{\n", fp);
		if(eof_check_string(sp->tags->title))
		{	//If a song title is defined
			(void) snprintf(temp_string, sizeof(temp_string) - 1, "	\"Title\": \"%s\",\n", sp->tags->title);
			(void) pack_fputs(temp_string, fp);		//Write song title
		}
		if(eof_check_string(sp->tags->artist))
		{	//If artist name is defined
			(void) snprintf(temp_string, sizeof(temp_string) - 1, "	\"Artist\": \"%s\",\n", sp->tags->artist);
			(void) pack_fputs(temp_string, fp);		//Write artist
		}
		if(eof_check_string(sp->tags->frettist))
		{	//If the chart author is defined
			(void) snprintf(temp_string, sizeof(temp_string) - 1, "	\"Charter\": \"%s\",\n", sp->tags->frettist);
			(void) pack_fputs(temp_string, fp);		//Write chart author
		}
		if(eof_check_string(sp->tags->genre))
		{	//If the genre is defined
			(void) snprintf(temp_string, sizeof(temp_string) - 1, "	\"Genre\": \"%s\",\n", sp->tags->genre);
			(void) pack_fputs(temp_string, fp);		//Write genre
		}
		(void) snprintf(temp_string, sizeof(temp_string) - 1, "	\"Song_Audio\": \"song.ogg\",\n");
		(void) pack_fputs(temp_string, fp);		//Write chart audio filename
		if(diff_written[0])
		{	//If the easy difficulty MIDI was exported
			(void) snprintf(temp_string, sizeof(temp_string) - 1, "	\"Notes_Easy\": \"%s\",\n", midi_names[0]);
			(void) pack_fputs(temp_string, fp);		//Write easy MIDI filename
		}
		if(diff_written[1])
		{	//If the medium difficulty MIDI was exported
			(void) snprintf(temp_string, sizeof(temp_string) - 1, "	\"Notes_Medium\": \"%s\",\n", midi_names[1]);
			(void) pack_fputs(temp_string, fp);		//Write medium MIDI filename
		}
		if(diff_written[2])
		{	//If the hard difficulty MIDI was exported
			(void) snprintf(temp_string, sizeof(temp_string) - 1, "	\"Notes_Hard\": \"%s\",\n", midi_names[2]);
			(void) pack_fputs(temp_string, fp);		//Write hard MIDI filename
		}
		if(diff_written[3])
		{	//If the expert difficulty MIDI was exported
			(void) snprintf(temp_string, sizeof(temp_string) - 1, "	\"Notes_Expert\": \"%s\",\n", midi_names[3]);
			(void) pack_fputs(temp_string, fp);		//Write expert MIDI filename
		}
		(void) pack_fputs("}\n", fp);
		pack_fclose(fp);
	}

	if(err)
	{
		allegro_message("There were one or more errors.  Check log for details.");
	}

	return 1; 		//Return success
}
