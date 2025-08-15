#include <allegro.h>
#include "beat.h"
#include "beatable.h"
#include "main.h"
#include "midi.h"
#include "utility.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

void eof_write_beatable_song_section(PACKFILE *fp, unsigned char tsnum, unsigned char tsden, double point1, double point2, unsigned long beatcount)
{
	double diff, beatlength;
	unsigned long bpm;
	float fval;

	if(!fp || (point2 < point1))
		return;	//Invalid parameter

	diff = point2 - point1;			//The distance between the two given timestamps in milliseconds
	beatlength = diff / beatcount;	//Divide this by the number of beats this distance is said to be
	bpm = (60000.0 / beatlength) + 0.5;	//Calculate the tempo that would give this length for each beat, rounded to nearest integer

	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tWriting sync point:  Tempo = %lu, TS = %u/%u, Duration = %fms, Start = %fms, Transition time = %fms", bpm, tsnum, tsden, diff / 1000.0, point1 / 1000.0, 0.0);
	eof_log(eof_log_string, 1);

	(void) pack_iputl(bpm, fp);		//Write the tempo
	(void) pack_iputl(tsnum, fp);		//Write the time signature beat count
	(void) pack_iputl(tsden, fp);		//Write the time signature beat unit
	fval = diff / 1000.0;
	(void) pack_fwrite(&fval, 4, fp);	//Write the distance between the two specified timestamps, in floating point seconds
	fval = point1 / 1000.0;
	(void) pack_fwrite(&fval, 4, fp);	//Write the first specified timestamp, in floating point seconds
	fval = 0;
	(void) pack_fwrite(&fval, 4, fp);	//Write the animation transition time, in floating point seconds
}

int eof_export_beatable(EOF_SONG *sp, unsigned long track, char *fn)
{
	unsigned long ctr, ctr2, notecount, ulong, diff, holdnotedatacollection, chaincount, chainend, linkcount, start, end, note;
	unsigned char bitmask;
	PACKFILE *fp;
	float fval;

	#define EOF_BEATABLE_SYNC_POINT_LIMIT 400
	unsigned long sync_point[EOF_BEATABLE_SYNC_POINT_LIMIT];
	unsigned long sync_points;	//The number of entries currently stored in sync_point[]
	unsigned long sync_count;	//The same as sync_points, plus one more if a sync point must be written at 0s because the first beat starts after that time

	if(!sp || !fn || (track >= sp->tracks) || (sp->beats < 2))
		return 1;	//Invalid parameters
	if(!eof_music_data)
		return 1;	//No audio buffered in memory
	if(!eof_track_is_beatable_mode(sp, track))
		return 1;	//The specified track is not designated as a BEATABLE track

	eof_log("eof_export_beatable() entered", 1);

	//Open output file
	fp = pack_fopen(fn, "w");
	if(!fp)
	{
		eof_log("\tError saving:  Cannot open file for writing", 1);
		return 1;	//Return failure
	}

	//Write header
	(void) pack_putc('b', fp);	//Write magic value
	(void) pack_iputl(4, fp);	//Write format version

	//Write audio blob
	(void) pack_iputl(eof_music_data_size, fp);	//Write size of audio blob
	(void) pack_fwrite(eof_music_data, eof_music_data_size, fp);	//Write the buffered OGG file

	//Write empty cover art blob
	(void) pack_iputl(0, fp);		//Write size of cover art blob
	(void) pack_iputl(0, fp);		//Write texture format number
	(void) pack_iputl(0, fp);		//Write image width
	(void) pack_iputl(0, fp);		//Write image height

	//Write song details
	WriteVarLen(ustrlen("ID"), fp);				//Write length of placeholder ID
	(void) pack_fwrite("ID", ustrlen("ID"), fp);	//Write placeholder ID, which isn't used by BEATABLE for custom songs, but BEATSMITH requires it to be present in order to save and close the "Edit song details" dialog

	if(eof_check_string(sp->tags->title))
	{	//If a song title is defined
		WriteVarLen(ustrlen(sp->tags->title), fp);	//Write length of song title
		(void) pack_fwrite(sp->tags->title, ustrlen(sp->tags->title), fp);	//Write song title
	}
	else
		(void) pack_putc(0, fp);	//Otherwise write no song title

	if(eof_check_string(sp->tags->artist))
	{	//If an artist name is defined
		WriteVarLen(ustrlen(sp->tags->artist), fp);	//Write length of artist name
		(void) pack_fwrite(sp->tags->artist, ustrlen(sp->tags->artist), fp);	//Write artist name
	}
	else
		(void) pack_putc(0, fp);	//Otherwise write no artist name

	fval = eof_chart_length / 1000.0;
	(void) pack_fwrite(&fval, 4, fp);	//Write a 4 byte floating point representation of the chart length in seconds
	(void) pack_iputl(0, fp);			//Write an audio delay of 0 microseconds

	//Write song structure
	eof_log("\t\tWriting sync points", 1);

	//Count sync points to be written
	sync_count = 0;
	if(sp->beat[0]->pos != 0)
		sync_count++;		//The game requires the first tempo change to be written at 0s, so an extra one is needed if the first beat starts after 0s
	for(sync_points = ctr = 0; ctr < sp->beats; ctr++)
	{	//For each beat in the project
		if(sync_points + sync_count>= EOF_BEATABLE_SYNC_POINT_LIMIT)
			break;	//The maximum number of sync points has been counted

		if(sp->beat[ctr]->has_ts)
		{
			if(!sp->beat[ctr]->beat_within_measure)
			{	//If there is a time signature in effect and this is the first beat in the measure, write a sync point for it
				sync_point[sync_points++] = ctr;	//Track that a sync point will be written for this beat
			}
		}
		else if(sp->beat[ctr]->flags & EOF_BEAT_FLAG_ANCHOR)
		{	//If there is no defined time signature in effect, but this beat is anchor, write a sync point for it
			sync_point[sync_points++] = ctr;	//Track that a sync point will be written for this beat
		}
	}
	sync_count += sync_points;

	//Write sync points
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tWriting %lu sync points", sync_count);
	eof_log(eof_log_string, 1);
	(void) pack_iputl(sync_count, fp);	//Write the number of tempo changes
	if(sp->beat[0]->pos != 0)
		eof_write_beatable_song_section(fp, 1, 4, 0.0, sp->beat[0]->fpos, 1);	//Insert a tempo change definition if the first beat is after 0s, the first beat will always have a sync point written because it will either be an anchor or the first beat of a measure
	for(ctr = 0; ctr < sync_points; ctr++)
	{	//For each sync point that was tracked
		unsigned ts_num = 1;	//Unless a time signature is in effect at this sync point, treat it like 1/4 meter
		unsigned ts_den = 4;

		if(sp->beat[sync_point[ctr]]->has_ts)
		{	//If there is a time signature in effect at this sync point
			ts_num = sp->beat[sync_point[ctr]]->num_beats_in_measure;
			ts_den = sp->beat[sync_point[ctr]]->beat_unit;
		}
		if(ctr + 1 >= sync_points)
		{	//If this is the last sync point that can be written
			eof_write_beatable_song_section(fp, ts_num, ts_den, sp->beat[ctr]->fpos, eof_chart_length, sp->beats - sync_point[ctr]);	//Write one last tempo change to encompass the rest of the chart
			break;
		}
		else
		{
			unsigned long numbeats = sync_point[ctr + 1] - sync_point[ctr];	//The number of beats between this sync point and the next
			eof_write_beatable_song_section(fp, ts_num, ts_den, sp->beat[sync_point[ctr]]->fpos, sp->beat[sync_point[ctr + 1]]->fpos, numbeats);		//Write a tempo change to reflect this beat's timing
		}
	}

	//Write note sheets
	for(diff = 0; diff < 4; diff++)
	{	//For each of the usable difficulty levels
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tWriting difficulty %lu", diff);
		eof_log(eof_log_string, 1);
		ulong = sp->track[track]->difficulty;	///Obtain the track's defined difficulty level (will eventually need to be a difficulty-specific difficulty level)
		if(!ulong || (ulong > 20))
			ulong = 1;					//If it's not a valid value, reset it to the lowest usable value
		(void) pack_iputl(ulong, fp);			//Write the difficulty level for this difficulty
		(void) pack_iputl(0, fp);				//Write no Score Rank configurations for this difficulty

		//Count tap notes
		for(ctr = notecount = 0; ctr < eof_get_track_size(sp, track); ctr++)
		{	//For each note in the track
			if((eof_get_note_type(sp, track, ctr) == diff) && (eof_get_note_length(sp, track, ctr) == 1) && (eof_get_note_note(sp, track, ctr) & 15))
			{	//If this note is in the target difficulty, has no sustain and has any gems on lanes 1 through 4
				notecount++;	//It will be written as a tap note
			}
		}
		//Write tap notes
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tWriting %lu tap notes", notecount);
		eof_log(eof_log_string, 1);
		(void) pack_iputl(notecount, fp);		//Write the number of tap notes for this difficulty
		for(ctr = notecount = 0; ctr < eof_get_track_size(sp, track); ctr++)
		{	//For each note in the track
			note = eof_get_note_note(sp, track, ctr);
			if((eof_get_note_type(sp, track, ctr) == diff) && (eof_get_note_length(sp, track, ctr) == 1) && (note & 15))
			{	//If this note is in the target difficulty, has no sustain and has any gems on lanes 1 through 4
				(void) pack_putc(0, fp);	//Write the deprecated IsOffBeat value
				(void) pack_iputl(eof_get_note_pos(sp, track, ctr) * 1000, fp);	//Write the note's timestamp in microseconds

				//Remap the EOF note bitmask to BEATABLE's system
				bitmask = 0;
				if(note & 1)
					bitmask |= 8;	//EOF lane 1 to BEATABLE L2 note
				if(note & 2)
					bitmask |= 4;	//EOF lane 2 to BEATABLE L1 note
				if(note & 4)
					bitmask |= 2;	//EOF lane 3 to BEATABLE R1 note
				if(note & 8)
					bitmask |= 1;	//EOF lane 4 to BEATABLE R2 note

				(void) pack_putc(bitmask, fp);	//Write the lane bitmask
			}
		}

		//Write hold note data collections
		(void) pack_iputl(4, fp);		//There is always a hold note collection written for each lane if any at all
		for(holdnotedatacollection = 0; holdnotedatacollection < 4; holdnotedatacollection++)
		{	//For each of the four hold note data collections, each of which is the collection of all hold/slide notes begin on that particular lane (L2, L1, R1, R2)
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tWriting hold note data collection %lu", holdnotedatacollection);
			eof_log(eof_log_string, 1);
			//Count hold note chains for this collection
			for(ctr = chaincount = 0; ctr < eof_get_track_size(sp, track); ctr++)
			{	//For each note in the track
				if(eof_get_note_note(sp, track, ctr) & (1 << holdnotedatacollection))
				{	//If the note has a gem on this hold note data collection's lane
					if(!(eof_get_note_tflags(sp, track, ctr) & EOF_NOTE_TFLAG_CHAINLINK) && (eof_get_note_type(sp, track, ctr) == diff) && (eof_get_note_length(sp, track, ctr) > 1))
					{	//If this note wasn't already counted as a link in a hold note chain, is in the target difficulty and has sustain
						chaincount++;	//This represents the start of a hold note chain
						for(ctr2 = eof_fixup_next_beatable_link(sp, track, ctr); ctr2 < eof_get_track_size(sp, track); ctr2 =  eof_fixup_next_beatable_link(sp, track, ctr2))
						{	//For all remaining hold notes in this hold note chain
							eof_set_note_tflags(sp, track, ctr2, eof_get_note_tflags(sp, track, ctr2) | EOF_NOTE_TFLAG_CHAINLINK);	//Mark the note as being a link in a hold note chain
						}
					}
				}
			}

			//Write hold note chains for this collection
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tWriting %lu hold note chains", chaincount);
			eof_log(eof_log_string, 1);
			(void) pack_iputl(chaincount, fp);		//Write the number of hold note chainsfor this difficulty
			for(ctr = 0; ctr < eof_get_track_size(sp, track); ctr++)
			{	//For each note in the track
				if(eof_get_note_note(sp, track, ctr) & (1 << holdnotedatacollection))
				{	//If the note has a gem on this hold note data collection's lane
					if(!(eof_get_note_tflags(sp, track, ctr) & EOF_NOTE_TFLAG_CHAINLINK) && (eof_get_note_type(sp, track, ctr) == diff) && (eof_get_note_length(sp, track, ctr) > 1))
					{	//If this note wasn't already counted as a link in a hold note chain, is in the target difficulty and has sustain
						chainend = ctr;	//Track the last note in this hold note chain
						start = eof_get_note_pos(sp, track, ctr);
						(void) pack_iputl(start * 1000, fp);	//This note is the start of a hold note chain, write its timestamp
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\tStart:  %lu microseconds (%.3f seconds)", start, (double)start / 1000000.0);
						eof_log(eof_log_string, 1);

						//Count the number of remaining links in this hold note chain
						for(ctr2 = eof_fixup_next_beatable_link(sp, track, ctr), linkcount = 0; ctr2 < eof_get_track_size(sp, track); ctr2 =  eof_fixup_next_beatable_link(sp, track, ctr2))
						{	//For all remaining hold notes in this hold note chain
							linkcount++;
						}
						linkcount++;	//Count one more to define the end position of the hold note chain, since in EOF there is no gem placed at the end position

						//Write the timestamps for the remaining linked hold notes in this chain
						(void) pack_iputl(linkcount, fp);		//Write the number of timestamps
						for(ctr2 = eof_fixup_next_beatable_link(sp, track, ctr); ctr2 < eof_get_track_size(sp, track); ctr2 =  eof_fixup_next_beatable_link(sp, track, ctr2))
						{	//For all remaining hold notes in this hold note chain
							chainend = ctr2;	//Track the last note in this hold note chain
							start = eof_get_note_pos(sp, track, ctr2);
							(void) pack_iputl(start * 1000, fp);	//This note is the start of a hold note chain, write its timestamp
							(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\t\tLink %lu:  Pos:  %lu microseconds (%.3f seconds)", ctr2, start, (double)start / 1000000.0);
							eof_log(eof_log_string, 1);
						}
						//Append a timestamp to reflect the end position of the last note in this chain
						end = eof_get_note_pos(sp, track, chainend) + eof_get_note_length(sp, track, chainend);
						(void) pack_iputl(end * 1000, fp);
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\t\tLink %lu:  Pos:  %lu microseconds (%.3f seconds)", ctr2, end, (double)end / 1000000.0);
						eof_log(eof_log_string, 1);

						//Write the lane bitmasks for the remaining linked hold notes in this chain
						(void) pack_iputl(linkcount, fp);		//Write the number of lane bitmasks
						for(ctr2 = eof_fixup_next_beatable_link(sp, track, ctr); ctr2 < eof_get_track_size(sp, track); ctr2 =  eof_fixup_next_beatable_link(sp, track, ctr2))
						{	//For all remaining hold notes in this hold note chain
							//Remap the EOF note bitmask to BEATABLE's system
							bitmask = 0;
							note = eof_get_note_note(sp, track, ctr2);
							if(note & 1)
								bitmask |= 8;	//EOF lane 1 to BEATABLE L2 note
							if(note & 2)
								bitmask |= 4;	//EOF lane 2 to BEATABLE L1 note
							if(note & 4)
								bitmask |= 2;	//EOF lane 3 to BEATABLE R1 note
							if(note & 8)
								bitmask |= 1;	//EOF lane 4 to BEATABLE R2 note

							(void) pack_putc(bitmask, fp);	//Write the lane bitmask
							(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\t\tLink %lu:  Bitmask:  %u", ctr2, bitmask);
							eof_log(eof_log_string, 1);
						}
						//Append a lane bitmask to reflect the end of the last note in this chain
						bitmask = 0;
						note = eof_get_note_note(sp, track, chainend);
						if(note & 1)
							bitmask |= 8;	//EOF lane 1 to BEATABLE L2 note
						if(note & 2)
							bitmask |= 4;	//EOF lane 2 to BEATABLE L1 note
						if(note & 4)
							bitmask |= 2;	//EOF lane 3 to BEATABLE R1 note
						if(note & 8)
							bitmask |= 1;	//EOF lane 4 to BEATABLE R2 note

						(void) pack_putc(bitmask, fp);	//Write the lane bitmask
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\t\tLink %lu:  Bitmask:  %u", ctr2, bitmask);
						eof_log(eof_log_string, 1);
					}
				}
			}//For each note in the track
		}//For each of the four hold note data collections

		//Count left snap notes
		for(ctr = notecount = 0; ctr < eof_get_track_size(sp, track); ctr++)
		{	//For each note in the track
			if((eof_get_note_type(sp, track, ctr) == diff) && (eof_get_note_note(sp, track, ctr) & 16) && (eof_get_note_flags(sp, track, ctr) & EOF_BEATABLE_NOTE_FLAG_LSNAP))
			{	//If this note is in the target difficulty and is a left snap note
				notecount++;
			}
		}
		//Write left snap notes
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tWriting %lu left snap notes", notecount);
		eof_log(eof_log_string, 1);
		(void) pack_iputl(notecount, fp);		//Write the number of left snap notes for this difficulty
		for(ctr = 0; ctr < eof_get_track_size(sp, track); ctr++)
		{	//For each note in the track
			if((eof_get_note_type(sp, track, ctr) == diff) && (eof_get_note_note(sp, track, ctr) & 16) && (eof_get_note_flags(sp, track, ctr) & EOF_BEATABLE_NOTE_FLAG_LSNAP))
			{	//If this note is in the target difficulty and is a left snap note
				(void) pack_iputl(eof_get_note_pos(sp, track, ctr) * 1000, fp);	//Write the note's timestamp in microseconds
			}
		}

		//Count right snap notes
		for(ctr = notecount = 0; ctr < eof_get_track_size(sp, track); ctr++)
		{	//For each note in the track
			if((eof_get_note_type(sp, track, ctr) == diff) && (eof_get_note_note(sp, track, ctr) & 16) && (eof_get_note_flags(sp, track, ctr) & EOF_BEATABLE_NOTE_FLAG_RSNAP))
			{	//If this note is in the target difficulty and is a right snap note
				notecount++;
			}
		}
		//Write right snap notes
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tWriting %lu right snap notes", notecount);
		eof_log(eof_log_string, 1);
		(void) pack_iputl(notecount, fp);		//Write the number of right snap notes for this difficulty
		for(ctr = 0; ctr < eof_get_track_size(sp, track); ctr++)
		{	//For each note in the track
			if((eof_get_note_type(sp, track, ctr) == diff) && (eof_get_note_note(sp, track, ctr) & 16) && (eof_get_note_flags(sp, track, ctr) & EOF_BEATABLE_NOTE_FLAG_RSNAP))
			{	//If this note is in the target difficulty and is a right snap note
				(void) pack_iputl(eof_get_note_pos(sp, track, ctr) * 1000, fp);	//Write the note's timestamp in microseconds
			}
		}

		//Count clap notes
		for(ctr = notecount = 0; ctr < eof_get_track_size(sp, track); ctr++)
		{	//For each note in the track
			if((eof_get_note_type(sp, track, ctr) == diff) && (eof_get_note_note(sp, track, ctr) & 32))
			{	//If this note is in the target difficulty and has a gem on lane 6
				notecount++;	//It will be written as a clap note
			}
		}
		//Write clap notes
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tWriting %lu clap notes", notecount);
		eof_log(eof_log_string, 1);
		(void) pack_iputl(notecount, fp);		//Write the number of clap notes for this difficulty
		for(ctr = notecount = 0; ctr < eof_get_track_size(sp, track); ctr++)
		{	//For each note in the track
			if((eof_get_note_type(sp, track, ctr) == diff) && (eof_get_note_note(sp, track, ctr) & 32))
			{	//If this note is in the target difficulty and has a gem on lane 6
				(void) pack_iputl(eof_get_note_pos(sp, track, ctr) * 1000, fp);	//Write the note's timestamp in microseconds
			}
		}
	}//For each of the usable difficulty levels

	//Cleanup
	//Clear EOF_NOTE_TFLAG_CHAINLINK flag set during hold note processing
	for(ctr = 0; ctr < eof_get_track_size(sp, track); ctr++)
	{
		eof_set_note_tflags(sp, track, ctr, eof_get_note_tflags(sp, track, ctr) & ~EOF_NOTE_TFLAG_CHAINLINK);	//Clear just this temporary flag
	}
	eof_log("\tBEATABLE export completed", 1);
	(void) pack_fclose(fp);
	if(!eof_validate_beatable_file(fn))
	{
		eof_log("\tValidation failed", 1);
		allegro_message("BEATABLE file validation failed (see EOF log for details).");
		return 1;	//Return failure
	}

	return 0;	//Success
}

int eof_validate_beatable_file(char *fn)
{
	PACKFILE *fp;
	unsigned long ctr, ctr2, ctr3, diff, tapnotes, leftsnapnotes, rightsnapnotes, clapnotes, holdnotecollections, holdnotechains, songsections, scorerankconfigs;
	unsigned long fp_pos = 0;	//Used to keep track of the number of bytes read in the file
	int word, error = 0;
	long dword;
	char byte;
	unsigned long blobsize, length;
	float fval;
	char *str, *score_ranks[7] = {"Undefined", "Rank D", "Rank C", "Rank B", "Rank A", "Rank S", "Rank S+"};
	char *holdnotecollection_names[4] = {"L2", "L1", "R1", "R2"};

	if(!fn)
	{	//If a file was not specified
		fn = ncd_file_select(0, eof_song_path, "Validate BEATABLE file", eof_filter_beatable_files);
		eof_clear_input();
		if(!fn)
			return 0;	//User cancellation
	}

	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tValidating BEATABLE file \"%s\"", fn);
	eof_log(eof_log_string, 1);

	//Open output file
	fp = pack_fopen(fn, "r");
	if(!fp)
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tError validating:  Cannot open file for reading:  %s", strerror(errno));
		eof_log(eof_log_string, 1);
		return 0;	//Return failure
	}

	//Validate header
	word = pack_getc(fp);	//Read the magic byte
	if(word != 'b')
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tError validating:  Invalid magic byte value, was %d and should be %d", word, 'b');
		eof_log(eof_log_string, 1);
		(void) pack_fclose(fp);
		return 0;	//Return failure
	}
	fp_pos++;
	dword = pack_igetl(fp);	//Read the version size
	if(dword != 4)
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tError validating:  Invalid format version, was %ld and should be 4", dword);
		eof_log(eof_log_string, 1);
		(void) pack_fclose(fp);
		return 0;	//Return failure
	}
	fp_pos += 4;

	//Parse audio blob
	blobsize = pack_igetl(fp);	//Read the audio blob size
	if(5 + 4 + blobsize + 16 + 11 + 24 + 16 > file_size_ex(fn))	//Header size + audio blob length + audio blob + empty cover art blob + no metadata + single tempo definition + 4 empty difficulties
	{	//If the blob size plus the minimum amount of data is too large to possibly fit based in the input file's size
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tError validating:  Audio blob size is too large for the file");
		eof_log(eof_log_string, 1);
		(void) pack_fclose(fp);
		return 0;	//Return failure
	}
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t0x%lX : Audio blob size:  %lu", fp_pos, blobsize);
	eof_log(eof_log_string, 1);
	fp_pos += 4;
	if(blobsize)
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t0x%lX - 0x%lX : (Audio blob)", fp_pos, fp_pos + blobsize);
	else
	{	//If there is no audio blob, there are 4 dwords defining an FMOD GUID
		unsigned long dword2, dword3, dword4;
		dword = pack_mgetl(fp);		//Read 4 dwords in big endian
		dword2 = pack_mgetl(fp);
		dword3 = pack_mgetl(fp);
		dword4 = pack_mgetl(fp);
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t0x%lX : FMOD ID: 0x%lX%lX%lX%lX", fp_pos, dword, dword2, dword3, dword4);
		blobsize = 16;	//Instead of an audio blob, 16 bytes were read instead
	}
	eof_log(eof_log_string, 1);
	if(pack_fseek(fp, blobsize) < 0)
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tError seeking past audio blob:  %s", strerror(errno));
		eof_log(eof_log_string, 1);
		(void) pack_fclose(fp);
		return 0;	//Return failure
	}
	fp_pos += blobsize;

	//Parse cover art data
	blobsize = pack_igetl(fp);	//Read the audio blob size
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t0x%lX : Cover art blob size:  %lu", fp_pos, blobsize);
	eof_log(eof_log_string, 1);
	fp_pos += 4;
	dword = pack_igetl(fp);	//Read the texture format number
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t0x%lX : Texture format number:  %lu", fp_pos, dword);
	eof_log(eof_log_string, 1);
	fp_pos += 4;
	dword = pack_igetl(fp);	//Read the image width
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t0x%lX : Image width:  %lu", fp_pos, dword);
	eof_log(eof_log_string, 1);
	fp_pos += 4;
	dword = pack_igetl(fp);	//Read the image height
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t0x%lX : Image height:  %lu", fp_pos, dword);
	eof_log(eof_log_string, 1);
	fp_pos += 4;
	if(blobsize)
	{	//If there is a cover art blob
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t0x%lX - 0x%lX : (Art blob)", fp_pos, fp_pos + blobsize);
		eof_log(eof_log_string, 1);
		if(pack_fseek(fp, blobsize) < 0)
		{
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tError seeking past art blob:  %s", strerror(errno));
			eof_log(eof_log_string, 1);
			(void) pack_fclose(fp);
			return 0;	//Return failure
		}
		fp_pos += blobsize;
	}

	//Parse song details
	dword = fp_pos;	//Back up this value before reading the VLV
	length = eof_pack_read_vlv(fp, &fp_pos);	//Read song ID string length
	str = NULL;
	if(length != ULONG_MAX)
	{
		str = (char *) malloc(length + 1);
		if(!str)
		{
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tError allocating %lu bytes for song ID string", length);
			eof_log(eof_log_string, 1);
			error = 1;
		}
		else if(pack_fread(str, length, fp) < length)
		{
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tError reading song ID:  %s", strerror(errno));
			eof_log(eof_log_string, 1);
			error = 1;
		}
		else
		{
			str[length] = '\0';	//NULL terminate the string
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t0x%lX : Song ID:  %s", dword, str);
			eof_log(eof_log_string, 1);
		}
		if(str)
			free(str);
		if(error)
		{
			(void) pack_fclose(fp);
			return 0;	//Return failure
		}
	}
	fp_pos += length;

	dword = fp_pos;	//Back up this value before reading the VLV
	length = eof_pack_read_vlv(fp, &fp_pos);	//Read song title string length
	str = NULL;
	if(length != ULONG_MAX)
	{
		str = (char *) malloc(length + 1);
		if(!str)
		{
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tError allocating %lu bytes for song title string", length);
			eof_log(eof_log_string, 1);
			error = 1;
		}
		else if(pack_fread(str, length, fp) < length)
		{
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tError reading song title:  %s", strerror(errno));
			eof_log(eof_log_string, 1);
			error = 1;
		}
		else
		{
			str[length] = '\0';	//NULL terminate the string
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t0x%lX : Song title:  %s", dword, str);
			eof_log(eof_log_string, 1);
		}
		if(str)
			free(str);
		if(error)
		{
			(void) pack_fclose(fp);
			return 0;	//Return failure
		}
	}
	fp_pos += length;

	dword = fp_pos;	//Back up this value before reading the VLV
	length = eof_pack_read_vlv(fp, &fp_pos);	//Read artist name string length
	str = NULL;
	if(length != ULONG_MAX)
	{
		str = (char *) malloc(length + 1);
		if(!str)
		{
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tError allocating %lu bytes for artist name string", length);
			eof_log(eof_log_string, 1);
			error = 1;
		}
		else if(pack_fread(str, length, fp) < length)
		{
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tError reading artist name:  %s", strerror(errno));
			eof_log(eof_log_string, 1);
			error = 1;
		}
		else
		{
			str[length] = '\0';	//NULL terminate the string
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t0x%lX : Artist name:  %s", dword, str);
			eof_log(eof_log_string, 1);
		}
		if(str)
			free(str);
		if(error)
		{
			(void) pack_fclose(fp);
			return 0;	//Return failure
		}
	}
	fp_pos += length;

	if(pack_fread(&fval, 4, fp) == EOF)
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tError reading song length:  %s", strerror(errno));
		eof_log(eof_log_string, 1);
	}
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t0x%lX : Song length:  %f", fp_pos, fval);
	eof_log(eof_log_string, 1);
	fp_pos += 4;
	dword = pack_igetl(fp);
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t0x%lX : Audio start offset:  %lu microseconds", fp_pos, dword);
	eof_log(eof_log_string, 1);
	fp_pos += 4;

	//Parse song structure
	songsections = pack_igetl(fp);
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t0x%lX : Song sections (%lu):", fp_pos, songsections);
	eof_log(eof_log_string, 1);
	fp_pos += 4;
	for(ctr = 0; ctr < songsections; ctr++)
	{	//For each song section
		unsigned long bpm, tsnum, tsden;
		float start, duration, time;

		bpm = pack_igetl(fp);
		tsnum = pack_igetl(fp);
		tsden = pack_igetl(fp);
		if(pack_fread(&duration, 4, fp) < 4)
			error = 1;
		if(pack_fread(&start, 4, fp) < 4)
			error = 1;
		if(pack_fread(&time, 4, fp) < 4)
			error = 1;

		if(error)
		{
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tError reading song section:  %s", strerror(errno));
			eof_log(eof_log_string, 1);
			(void) pack_fclose(fp);
			return 0;	//Return failure
		}
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t0x%lX : %lu BPM, TS:  %lu/%lu, Dur: %fs, Start: %fs, TTime: %fs", fp_pos, bpm, tsnum, tsden, duration, start, time);
		eof_log(eof_log_string, 1);
		fp_pos += 24;
	}

	//Parse note sheets
	for(diff = 0; diff < 4; diff++)
	{	//For each of the 4 difficulties
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t0x%lX : Difficulty:  %s", fp_pos, &eof_beatable_tab_name[diff][1]);
		eof_log(eof_log_string, 1);
		dword = pack_igetl(fp);	//Read difficulty rating
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tDifficulty rating:  %lu", dword);
		eof_log(eof_log_string, 1);
		fp_pos += 4;

	//Parse score rankings
		scorerankconfigs = pack_igetl(fp);	//Read the number of score rank configurations
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t0x%lX : Score rankings (%lu):", fp_pos, scorerankconfigs);
		eof_log(eof_log_string, 1);
		fp_pos += 4;
		for(ctr = 0; ctr < scorerankconfigs; ctr++)
		{	//For each score rank config
			unsigned long rank, score;

			rank = pack_igetl(fp);
			if(rank >= 7)
			{
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\tInvalid score rank:  %lu", dword);
				eof_log(eof_log_string, 1);
				(void) pack_fclose(fp);
				return 0;	//Return failure
			}
			score = pack_igetl(fp);
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\tScore rank:  %s, score:  %lu", score_ranks[rank], score);
			eof_log(eof_log_string, 1);
			fp_pos += 8;
		}

	//Parse tap note data
		tapnotes = pack_igetl(fp);	//Read the number of tap notes
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t0x%lX : Tap notes (%lu):", fp_pos, tapnotes);
		eof_log(eof_log_string, 1);
		fp_pos += 4;
		for(ctr = 0; ctr < tapnotes; ctr++)
		{	//For each tap note
			(void) pack_getc(fp);	//Read the deprecated IsOffbeat status
			dword = pack_igetl(fp);	//Read the note's timestamp
			byte = pack_getc(fp);	//Read the note's bitmask
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\tPos: %lu microseconds (%.3f seconds), bitmask: %X (%s%s%s%s)", dword, (double)dword / 1000000.0, byte, (byte & 8 ? "L2 " : ""), (byte & 4 ? "L1 " : ""), (byte & 2 ? "R1 " : ""), (byte & 1 ? "R2 " : ""));
			eof_log(eof_log_string, 1);
			fp_pos += 6;
		}

	//Parse hold note data
		holdnotecollections = pack_igetl(fp);	//Read the number of hold note collections
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t0x%lX : Hold note data collections (%lu)", fp_pos, holdnotecollections);
		eof_log(eof_log_string, 1);
		fp_pos += 4;
		for(ctr = 0; ctr < holdnotecollections; ctr++)
		{	//For each hold note collection
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tHold note data collection %lu (%s)", ctr, holdnotecollection_names[ctr]);
			eof_log(eof_log_string, 1);
			holdnotechains = pack_igetl(fp);	//Read the number of hold note chains in this collection
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\t0x%lX : Hold note chains (%lu)", fp_pos, holdnotechains);
			eof_log(eof_log_string, 1);
			fp_pos += 4;

			for(ctr2 = 0; ctr2 < holdnotechains; ctr2++)
			{	//For each hold note chain in this collection
				unsigned long start, links;
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\tHold note chain %lu", ctr2);
				eof_log(eof_log_string, 1);
				start = pack_igetl(fp);	//Read the timestmap of this hold note chain
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\t\t0x%lX : Start:  %lu microseconds (%.3f seconds)", fp_pos, start, (double)start / 1000000.0);
				eof_log(eof_log_string, 1);
				fp_pos += 4;
				links = pack_igetl(fp);	//Read the number of linked timestamps for this hold note chain
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\t\t\t0x%lX : Hold note chain linked timestamps (%lu)", fp_pos, links);
				eof_log(eof_log_string, 1);
				fp_pos += 4;

				for(ctr3 = 0; ctr3 < links; ctr3++)
				{	//For each additional timestamp in this hold note chain
					dword = pack_igetl(fp);	//Read the timestamp of this link
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\t\t\t\t0x%lX : Link %lu : Start:  %lu microseconds (%.3f seconds)", fp_pos, ctr3, dword, (double)dword / 1000000.0);
					eof_log(eof_log_string, 1);
					fp_pos += 4;
				}

				dword = pack_igetl(fp);	//Read the number of linked lane bitmasks for this hold note chain
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\t\t\t0x%lX : Hold note chain linked lane bitmasks (%lu)", fp_pos, links);
				eof_log(eof_log_string, 1);
				fp_pos += 4;

				if(dword != links)
				{
					eof_log("\t\t\t\t\t\tMismatching number of timestamps and lane bitmasks for this hold note chain", 1);
					(void) pack_fclose(fp);
					return 0;	//Return failure
				}

				for(ctr3 = 0; ctr3 < links; ctr3++)
				{	//For each additional lane bitmask in this hold note chain
					byte = pack_getc(fp);	//Read the lane bitmask of this link
					if(eof_note_count_colors_bitmask(byte) > 1)
					{
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\t\t\t\t0x%lX : Invalid lane bitmask defines use of more than 1 lane: %u", fp_pos, byte);
						eof_log(eof_log_string, 1);
						return 0;	//Return failure
					}
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\t\t\t\t0x%lX : Link %lu : Lane bitmask: %u (%s%s%s%s)", fp_pos, ctr3, byte, (byte & 8 ? "L2 " : ""), (byte & 4 ? "L1 " : ""), (byte & 2 ? "R1 " : ""), (byte & 1 ? "R2 " : ""));
					eof_log(eof_log_string, 1);
					fp_pos++;
				}
			}
		}

	//Parse left snap note data
		leftsnapnotes = pack_igetl(fp);	//Read the number of left snap notes
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t0x%lX : Left snap notes (%lu)", fp_pos, leftsnapnotes);
		eof_log(eof_log_string, 1);
		fp_pos += 4;
		for(ctr = 0; ctr < leftsnapnotes; ctr++)
		{	//For each left snap note
			dword = pack_igetl(fp);	//Read the note's timestamp
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\t0x%lX : Pos: %lu microseconds (%.3f seconds)", fp_pos, dword, (double)dword / 1000000.0);
			eof_log(eof_log_string, 1);
			fp_pos += 4;
		}

	//Parse right snap note data
		rightsnapnotes = pack_igetl(fp);	//Read the number of right snap notes
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t0x%lX : Right snap notes (%lu)", fp_pos, rightsnapnotes);
		eof_log(eof_log_string, 1);
		fp_pos += 4;
		for(ctr = 0; ctr < rightsnapnotes; ctr++)
		{	//For each right snap note
			dword = pack_igetl(fp);	//Read the note's timestamp
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\t0x%lX : Pos: %lu microseconds (%.3f seconds)", fp_pos, dword, (double)dword / 1000000.0);
			eof_log(eof_log_string, 1);
			fp_pos += 4;
		}

	//Parse clap note data
		clapnotes = pack_igetl(fp);	//Read the number of clap notes
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t0x%lX : Clap notes (%lu)", fp_pos, clapnotes);
		eof_log(eof_log_string, 1);
		fp_pos += 4;
		for(ctr = 0; ctr < clapnotes; ctr++)
		{	//For each clap note
			dword = pack_igetl(fp);	//Read the note's timestamp
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\t0x%lX : Pos: %lu microseconds (%.3f seconds)", fp_pos, dword, (double)dword / 1000000.0);
			eof_log(eof_log_string, 1);
			fp_pos += 4;
		}
	}//For each of the 4 difficulties

	//Cleanup
	(void) pack_fclose(fp);
	eof_log("\tValidation successful.", 1);
	return 1;	//Success
}

unsigned long eof_fixup_next_beatable_link(EOF_SONG *sp, unsigned long track, unsigned long notenum)
{
	unsigned long ctr, length, targetpos, thispos;
	unsigned char diff, note;

	if(!sp || !track || (track >= sp->tracks))
		return ULONG_MAX;	//Invalid parameters
	if(!eof_track_is_beatable_mode(sp, track))
		return ULONG_MAX;	//Only BEATABLE tracks are allowed by this function
	length = eof_get_note_length(sp, track, notenum);
	if(length < 2)
		return ULONG_MAX;	//Only hold (sustained) notes can link to another hold note

	diff = eof_get_note_type(sp, track, notenum);
	note = eof_get_note_note(sp, track, notenum);
	targetpos = eof_get_note_pos(sp, track, notenum);
	for(ctr = notenum + 1; notenum < eof_get_track_size(sp, track); notenum++)
	{	//For the remaining notes in the track
		thispos = eof_get_note_pos(sp, track, ctr);
		if(thispos < targetpos + length)
			continue;	//If this note doesn't start at/after the target note, skip it
		if(thispos > targetpos + length + EOF_BEATABLE_LINK_THRESHOLD)
			return ULONG_MAX;	//If this note starts too far after the target note to link to it, all remaining ones will as well, stop checking notes
		if(eof_get_note_length(sp, track, ctr) < 2)
			continue;	//If this note is not a hold note (has no sustain), skip it
		if(eof_get_note_type(sp, track, ctr) != diff)
			continue;	//If this note isn't in the target note's difficulty, skip it
		if(!(eof_get_note_note(sp, track, ctr) & note))
			continue;	//If this note doesn't have any lanes in common with the target note, skip it

		//If all the above checks passed, this hold note links to the target hold note
		return ctr;	//Return match
	}

	return ULONG_MAX;	//Return no match
}
