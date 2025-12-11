#include <allegro.h>
#include <ctype.h>
#include "chart_import.h"
#include "dr.h"
#include "main.h"
#include "song.h"
#include "undo.h"
#include "utility.h"
#include "foflc/Lyric_storage.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

char        eof_note_type_name_dr[5][10] = {"Easy", "Medium", "Hard", "Extreme", "Extreme+"};

unsigned long eof_get_note_name_as_number(EOF_SONG * sp, unsigned long track, unsigned long notenum, unsigned long *number)
{
	unsigned long value = 0;
	char *name;
	int retval;

	if(!number)
		return 0;	//Invalid parameter

	name = eof_get_note_name(sp, track, notenum);

	retval = eof_read_macro_number(name, &value);		//Convert the note name into a number
	if(!retval)
		return 0;	//Return error

	*number = value;
	return 1;	//Return success
}

int eof_check_drums_rock_track(EOF_SONG * sp, unsigned long track)
{
	unsigned long ctr;
	int ret, err, drum_roll_count_warned = 0, drum_roll_chord_warned = 0;

	if(!eof_track_is_drums_rock_mode(sp, track))
		return 0;	//Not a Drums Rock enabled track

	//Validate drum roll hit counts (must be between 1 and 100)
	eof_determine_phrase_status(sp, track);	//Update the trill and tremolo status of each note (used to mark drum rolls)
	for(ctr = 0; ctr < eof_get_track_size(sp, track); ctr++)
	{	//For each note in the track
		unsigned long flags = eof_get_note_flags(sp, track, ctr);
		if((flags & EOF_NOTE_FLAG_IS_TREMOLO) || (flags & EOF_NOTE_FLAG_IS_TRILL))
		{	//If the note is in a drum roll or special drum roll
			unsigned long number = 0;

			err = eof_get_note_name_as_number(sp, track, ctr, &number);

			if(err || (number > 100))
			{	//If the drum roll hit count is not valid
				if(!drum_roll_count_warned)
				{	//If the user wasn't warned about this yet
					ret = alert3("Drums Rock:  At least one drum roll does not have a valid hit count", NULL, "Cancel and seek to first offending roll?", "Yes", "No", "Highlight all and cancel", 0, 0, 0);
					if(ret == 2)
					{	//User declined
						drum_roll_count_warned = 1;
						break;	//Stop checking the drum rolls
					}

					eof_seek_and_render_position(track, eof_get_note_type(sp, track, ctr), eof_get_note_pos(sp, track, ctr));	//Seek to the first offending note
					if(ret == 1)
					{	//If the user opted to cancel the save
						return 1;	//Return cancellation
					}

					drum_roll_count_warned = 3;
				}
				if(drum_roll_count_warned == 3)
				{	//User opted to highlight all offending drum rolls
					eof_set_note_flags(sp, track, ctr, flags | EOF_NOTE_FLAG_HIGHLIGHT);
				}
			}
		}
	}
	if(drum_roll_count_warned == 3)
		return 1;	//Return cancellation

	for(ctr = 0; ctr < eof_get_track_size(sp, track); ctr++)
	{	//For each note in the track
		unsigned long flags = eof_get_note_flags(sp, track, ctr);
		if((flags & EOF_NOTE_FLAG_IS_TREMOLO) || (flags & EOF_NOTE_FLAG_IS_TRILL))
		{	//If the note is in a drum roll or special drum roll
			if(eof_note_count_colors(sp, track, ctr) > 1)
			{	//If the note in drum roll is a chord
				if(!drum_roll_chord_warned)
				{	//If the user wasn't warned about this yet
					ret = alert3("Drums Rock:  At least one note in drum roll is a chord", "This isn't supported in-game", "Cancel and seek to first offending note?", "Yes", "No", "Highlight all and cancel", 0, 0, 0);
					if(ret == 2)
					{	//User declined
						drum_roll_chord_warned = 1;
						break;	//Stop checking the drum rolls
					}

					eof_seek_and_render_position(track, eof_get_note_type(sp, track, ctr), eof_get_note_pos(sp, track, ctr));	//Seek to the first offending note
					if(ret == 1)
					{	//If the user opted to cancel the save
						return 1;	//Return cancellation
					}

					drum_roll_chord_warned = 3;
				}
				if(drum_roll_chord_warned == 3)
				{	//User opted to highlight all offending drum rolls
					eof_set_note_flags(sp, track, ctr, flags | EOF_NOTE_FLAG_HIGHLIGHT);
				}
			}
		}
	}
	if(drum_roll_chord_warned == 3)
		return 1;	//User cancellation

	//Check for drum notes with more than two gems
	for(ctr = 0; ctr < eof_get_track_size(sp, track); ctr++)
	{	//For each note in the track
		if(eof_note_count_colors(sp, track, ctr)  > 2)
		{
			allegro_message("Drums Rock:  At least one drum note has more than two gems.  These would be altered during export.  Affected notes render in the piano roll with a maroon background.");
			break;	//Stop checking the notes
		}
	}

	return 0;	//No issues
}

unsigned char eof_convert_drums_rock_note_mask(EOF_SONG *sp, unsigned long track, unsigned long note)
{
	unsigned ctr, bitmask, count = 0;
	unsigned char newnote = 0, notemask, is_drum_roll = 0;
	unsigned long flags;

	if(!sp || (track >= sp->tracks))
		return 0;	//Invalid parameters

	notemask = eof_get_note_note(sp, track, note);
	flags = eof_get_note_flags(sp, track, note);
	if((flags & EOF_NOTE_FLAG_IS_TREMOLO) || (flags & EOF_NOTE_FLAG_IS_TRILL))
		is_drum_roll = 1;	//Track that the note is in a drum roll

	//Perform remapping logic if enabled
	if((sp->track[track]->flags  & EOF_TRACK_FLAG_DRUMS_ROCK_REMAP) && !(flags & EOF_NOTE_FLAG_CRAZY))
	{	//If remapping was enabled and the note is not marked with crazy status
		unsigned char remapped = 0;

		if(notemask & 1)
		{	//Remap lane 1 gem
			remapped |= 1 << (drums_rock_remap_lane_1 - 1);
		}
		if(notemask & 2)
		{	//Remap lane 2 gem
			remapped |= 1 << (drums_rock_remap_lane_2 - 1);
		}
		if(notemask & 4)
		{	//Remap lane 3 gem
			if(flags & EOF_DRUM_NOTE_FLAG_Y_CYMBAL)
			{	//Remap lane 3 cymbal
				remapped |= 1 << (drums_rock_remap_lane_3_cymbal - 1);
			}
			else
			{	//Remap lane 3 tom
				remapped |= 1 << (drums_rock_remap_lane_3 - 1);
			}
		}
		if(notemask & 8)
		{	//Remap lane 4 gem
			if(flags & EOF_DRUM_NOTE_FLAG_B_CYMBAL)
			{	//Remap lane 4 cymbal
				remapped |= 1 << (drums_rock_remap_lane_4_cymbal - 1);
			}
			else
			{	//Remap lane 4 tom
				remapped |= 1 << (drums_rock_remap_lane_4 - 1);
			}
		}
		if(notemask & 16)
		{	//Remap lane 5 gem
			if(flags & EOF_DRUM_NOTE_FLAG_G_CYMBAL)
			{	//Remap lane 5 cymbal
				remapped |= 1 << (drums_rock_remap_lane_5_cymbal - 1);
			}
			else
			{	//Remap lane 5 tom
				remapped |= 1 << (drums_rock_remap_lane_5 - 1);
			}
		}
		if(notemask & 32)
		{	//Remap lane 6 gem
			remapped |= 1 << (drums_rock_remap_lane_6 - 1);
		}
		notemask = remapped;	//The rest of the function will use this converted note mask
	}

	//Ensure the bitmask contains no more than 2 gems (or 1 if it is in a drum roll)
	//Special rule:  If a drum chord includes bass, always keep that gem
	if(notemask & 8)
	{
		if(is_drum_roll)
			return 8;		//Drum rolls only keep the first gem

		newnote |= 8;
		count++;
	}

	//Special rule:  If a drum chord includes snare, always keep that gem
	if(notemask & 4)
	{
		if(is_drum_roll)
			return 4;		//Drum rolls only keep the first gem

		newnote |= 4;
		count++;
	}

	//Keep up to a total of two gems from whatever is remaining
	for(ctr = 0, bitmask = 1; (ctr < 6) && (count < 2); ctr++, bitmask <<= 1)
	{	//For each of the 6 supported lanes, only until two gems are selected to be kept
		if((notemask & bitmask) && !(newnote & bitmask))
		{	//If there is a gem on this lane that isn't already copied into the output mask
			if(is_drum_roll)
				return bitmask;	//Drum rolls only keep the first gem

			count++;
			newnote |= bitmask;	//Set it in the new note mask
		}
	}

	return newnote;
}

void eof_build_sanitized_drums_rock_string(char *input, char *output)
{
	unsigned long ctr, index = 0;

	if(!input || !output || (input == output))
		return;	//Invalid parameters

	//Write the output string
	output[0] = '\0';	//Terminate the output string
	for(ctr = 0; input[ctr] != '\0'; ctr++)
	{	//For each character in the input string
		if(input[ctr] == ',')
			continue;	//If the input character is a comma, skip it
		if(input[ctr] == '+')
			continue;	//If the input character is a plus sign, skip it

		uinsert(output, index++, input[ctr]);	//Append it to the output string
		if(input[ctr] == '"')
		{	//Writing " inside a CSV field requires writing it as two double quotation marks back to back
			uinsert(output, index++, input[ctr]);
		}
	}

	uinsert(output, index, '\0');	//Terminate the output string
}

int eof_export_drums_rock_track_diff(EOF_SONG * sp, unsigned long track, unsigned char diff, char *destpath)
{
	PACKFILE *fp;
	int err, ret;
	char temp_string[1024], temp_filename2[1024], tag_in_progress;
	unsigned long ctr;
	double avg_tempo;

	//Use song metadata and difficulty level to build the "Artist - Song - Difficulty" string and build a subfolder of that name in the project folder
	if(diff > 3)
		return 0;	//Invalid difficulty level
	if(!eof_track_is_drums_rock_mode(sp, track))
		return 0;	//Not a valid track or Drums Rock is not enabled
	if(!destpath)
		return 0;	//Invalid destination folder path
	if(!eof_get_track_diff_size(sp, track, diff))
		return 0;	//This difficulty is empty

 	eof_log("eof_export_drums_rock_track_diff() entered", 1);

	//Build the path to the Drums Rock folder for this track difficulty
	(void) replace_filename(eof_temp_filename, destpath, "", 1024);	//Obtain the destination folder path
	put_backslash(eof_temp_filename);
	temp_string[0] = '\0';	//Empty this string
	if(eof_check_string(sp->tags->artist))
	{	//If the artist of the song is defined
		(void) ustrcat(temp_string, sp->tags->artist);
		(void) ustrcat(temp_string, " - ");
	}
	if(eof_check_string(sp->tags->title))
	{	//If the title of the song is defined
		(void) ustrcat(temp_string, sp->tags->title);
		(void) ustrcat(temp_string, " - ");
	}
	(void) ustrcat(temp_string, eof_note_type_name_dr[diff]);
	eof_build_sanitized_filename_string(temp_string, temp_filename2);	//Filter out characters that can't be used in filenames
	(void) ustrcat(eof_temp_filename, temp_filename2);	//Append to the destination folder path

	//Build the subfolder if it doesn't already exist
	if(!eof_folder_exists(eof_temp_filename))
	{	//If the export subfolder doesn't already exist
		err = eof_mkdir(eof_temp_filename);
		if(err && !eof_folder_exists(eof_temp_filename))
		{	//If it couldn't be created and is still not found to exist (in case the previous check was a false negative)
			allegro_message("Could not create folder!\n%s", eof_temp_filename);
			return 0;	//Return failure:  Could not create export folder
		}
	}

	//Write info.csv
	put_backslash(eof_temp_filename);
	(void) replace_filename(eof_temp_filename, eof_temp_filename, "info.csv", (int) sizeof(eof_temp_filename));
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tWriting \"%s\"", eof_temp_filename);
	eof_log(eof_log_string, 2);
	fp = pack_fopen(eof_temp_filename, "w");
	if(!fp)
	{
		eof_log("\tError saving:  Cannot open info.csv for writing", 1);
		return 0;	//Return failure
	}
	pack_fputs("Song Name,Author Name,Difficulty,Song Duration in seconds,Song Map\n", fp);
	eof_build_sanitized_drums_rock_string(sp->tags->title, temp_string);
	(void) pack_fputs(temp_string, fp);	//Write song title
	(void) pack_putc(',', fp);
	eof_build_sanitized_drums_rock_string(sp->tags->artist, temp_string);
	(void) pack_fputs(temp_string, fp);	//Write artist name
	(void) pack_putc(',', fp);
	(void) snprintf(temp_string, sizeof(temp_string) - 1, "%u", diff);
	(void) pack_fputs(temp_string, fp);	//Write difficulty level
	(void) pack_putc(',', fp);
	(void) snprintf(temp_string, sizeof(temp_string) - 1, "%lu", eof_music_length / 1000);
	(void) pack_fputs(temp_string, fp);	//Write song length
	(void) pack_putc(',', fp);
	(void) pack_putc('0', fp);		//Write song map
	(void) pack_fclose(fp);

	//Write notes.csv
	(void) replace_filename(eof_temp_filename, eof_temp_filename, "notes.csv", (int) sizeof(eof_temp_filename));
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tWriting \"%s\"", eof_temp_filename);
	eof_log(eof_log_string, 2);
	fp = pack_fopen(eof_temp_filename, "w");
	if(!fp)
	{
		eof_log("\tError saving:  Cannot open notes.csv for writing", 1);
		return 0;	//Return failure
	}
	pack_fputs("Time [s],Enemy Type,Aux Color 1,Aux Color 2,No Enemies,interval,Aux\n", fp);
	eof_determine_phrase_status(sp, track);	//Update the trill and tremolo status of each note (used to mark drum rolls)
	for(ctr = 0; ctr < eof_get_track_size(sp, track); ctr++)
	{	//For each note in the track
		unsigned long flags, ctr2, bitmask, gem1 = 0, gem2 = 0, enemytype = 0, drumrollcount = 0, aux, value;
		unsigned char notemask;

		if(eof_get_note_type(sp, track, ctr) != diff)
			continue;	//If this note isn't in the target difficulty, skip it

		//Process note for export
		notemask = eof_convert_drums_rock_note_mask(sp, track, ctr);	//Convert the note bitmask for Drums Rock
		for(ctr2 = 0, bitmask = 1, gem1= 0, gem2 = 0; ctr2 < 6; ctr2++, bitmask <<= 1)
		{	//For each of the 6 supported lanes
			if(notemask & bitmask)
			{	//If this lane is used
				switch(ctr2)
				{	//The gem numbers do not match the left to right lane ordering of how they display in-game
					case 0:
						value = 3;	//Orange exports as gem 3
					break;
					case 1:
						value = 5;	//Blue exports as gem 5
					break;
					case 2:
						value = 1;	//Yellow exports as gem 1
					break;
					case 3:
						value = 2;	//Red exports as gem 2
					break;
					case 4:
						value = 6;	//Green exports as gem 6
					break;
					default:
						value = 4;	//Purple exports as gem 4
					break;
				}
				if(!gem1)
				{	//If this is the first gem seen for this note
					gem1 = gem2 = value;
					enemytype = 1;	//Export as single gem drum note
				}
				else
				{	//This is the second gem seen for this note
					gem2 = value;
					enemytype = 2;			//Export as dual gem drum note
					break;	//There will be no third gem to export
				}
			}
		}
		flags = eof_get_note_flags(sp, track, ctr);
		if((flags & EOF_NOTE_FLAG_IS_TREMOLO) || (flags & EOF_NOTE_FLAG_IS_TRILL))
		{	//If the note is in a drum roll or special drum roll
			ret = eof_get_note_name_as_number(sp, track, ctr, &drumrollcount);
			if(!ret || (drumrollcount > 100))
			{	//If the drum roll hit count is not valid, write it as 3
				drumrollcount = 3;
			}

			if(drumrollcount)
			{	//if the drum roll count is not explicitly defined as 0 for this note to disable the roll
				enemytype = 3;	//Export as a drum roll
				gem2 = gem1;		//Drums Rock only supports one lane per drum roll
			}
		}

		//Write the note data
		(void) snprintf(temp_string, sizeof(temp_string) - 1, "%.2f,", (double) eof_get_note_pos(sp, track, ctr) / 1000.0);
		(void) pack_fputs(temp_string, fp);	//Write timestamp
		(void) snprintf(temp_string, sizeof(temp_string) - 1, "%lu,", enemytype);
		(void) pack_fputs(temp_string, fp);	//Write enemy type
		(void) snprintf(temp_string, sizeof(temp_string) - 1, "%lu,%lu,1,", gem1, gem2);
		(void) pack_fputs(temp_string, fp);	//Write enemy color 1, color 2 and number of enemies fields
		if(drumrollcount)
		{	//If this note is a drum roll
			(void) snprintf(temp_string, sizeof(temp_string) - 1, "%lu,", drumrollcount);
			(void) pack_fputs(temp_string, fp);	//Write drum roll interval (hit count)
		}
		else
		{
			(void) pack_putc(',', fp);	//Write an empty drum roll interval field
		}
		switch(gem1)
		{	//The "Aux" field depends on the "Aux color 1" field's value
			case 1:
				aux = 6;
			break;
			case 2:
				aux = 7;
			break;
			case 3:
				aux = 5;
			break;
			case 4:
				aux = 8;
			break;
			case 5:
				aux = 5;
			break;
			case 6:
				aux = 8;
			break;
			default:	//Invalid?
				aux = 8;
		}
		(void) snprintf(temp_string, sizeof(temp_string) - 1, "%lu\n", aux);
		(void) pack_fputs(temp_string, fp);	//Write Aux value and end the line
	}
	(void) pack_fclose(fp);

	//Write Metadata.cfg
	tag_in_progress = 0;
	(void) replace_filename(eof_temp_filename, eof_temp_filename, "Metadata.cfg", (int) sizeof(eof_temp_filename));
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tWriting \"%s\"", eof_temp_filename);
	eof_log(eof_log_string, 2);
	fp = pack_fopen(eof_temp_filename, "w");
	if(!fp)
	{
		eof_log("\tError saving:  Cannot open Metadata.cfg for writing", 1);
		return 0;	//Return failure
	}
	(void) snprintf(temp_string, sizeof(temp_string) - 1, "title=%s - %s\n", sp->tags->artist, sp->tags->title);
	(void) pack_fputs(temp_string, fp);		//Write song title
	(void) pack_fputs("description=", fp);	//Write description
	if(eof_check_string(sp->tags->title))
	{	//If a song title is defined
		tag_in_progress = 1;
		(void) pack_fputs("Song:  ", fp);
		(void) pack_fputs(sp->tags->title, fp);
	}
	if(eof_check_string(sp->tags->artist))
	{	//If artist name is defined
		if(tag_in_progress)
			(void) pack_fputs(" | ", fp);	//Use spaces and a pipe character to separate fields in the description tag
		tag_in_progress = 1;
		(void) pack_fputs("Artist:  ", fp);
		(void) pack_fputs(sp->tags->artist, fp);
	}
	if(eof_check_string(sp->tags->genre))
	{	//If the genre is defined
		if(tag_in_progress)
			(void) pack_fputs(" | ", fp);	//Use spaces and a pipe character to separate fields in the description tag
		tag_in_progress = 1;
		(void) pack_fputs("Genre:  ", fp);
		(void) pack_fputs(sp->tags->genre, fp);
	}
	if(eof_check_string(sp->tags->frettist))
	{	//If the chart author is defined
		if(tag_in_progress)
			(void) pack_fputs(" | ", fp);	//Use spaces and a pipe character to separate fields in the description tag
		tag_in_progress = 1;
		(void) pack_fputs("Charter:  ", fp);
		(void) pack_fputs(sp->tags->frettist, fp);
	}
	if(tag_in_progress)
		(void) pack_fputs(" | ", fp);	//Use spaces and a pipe character to separate fields in the description tag
	tag_in_progress = 1;
	(void) pack_fputs("Difficulty:  ", fp);
	(void) pack_fputs(eof_note_type_name_dr[diff], fp);

	avg_tempo = 60000.0 / ((sp->beat[sp->beats - 1]->fpos - sp->beat[0]->fpos) / sp->beats);
	(void) pack_fputs(" | ", fp);	//Use spaces and a pipe character to separate fields in the description tag
	(void) pack_fputs("Average BPM:  ", fp);
	(void) snprintf(temp_string, sizeof(temp_string) - 1, "%lu", (unsigned long)(avg_tempo + 0.5));
	(void) pack_fputs(temp_string, fp);

	(void) pack_fputs(" | ", fp);	//Use spaces and a pipe character to separate fields in the description tag
	(void) pack_fputs("Total notes:  ", fp);
	(void) snprintf(temp_string, sizeof(temp_string) - 1, "%lu", eof_get_track_diff_size(sp, track, diff));
	(void) pack_fputs(temp_string, fp);

	if(eof_check_string(sp->tags->loading_text))
	{	//If there is loading text
		(void) pack_fputs(" | ", fp);	//Use spaces and a pipe character to separate fields in the description tag
		(void) pack_fputs(sp->tags->loading_text, fp);
	}
	(void) pack_fputs("\n", fp);
	(void) pack_fputs("isPublic=true\n", fp);
	(void) snprintf(temp_string, sizeof(temp_string) - 1, "tags=%s", eof_note_type_name_dr[diff]);
	(void) pack_fputs(temp_string, fp);		//Write difficulty tag
	if(eof_check_string(sp->tags->genre))
	{	//If there is a defined genre
		(void) snprintf(temp_string, sizeof(temp_string) - 1, ",%s", sp->tags->genre);
		for(ctr = 0; ctr < ustrlen(temp_string); ctr++)
		{	//For each character in the genre string
			if(isspace(ugetat(temp_string, ctr)))
			{	//If it is whitespace
				usetat(temp_string, ctr, '_');	//Replace it with an underscore
			}
		}
		(void) pack_fputs(temp_string, fp);		//Write genre tag
	}
	(void) pack_fputs("\n", fp);
	(void) pack_fclose(fp);

	//Write song.ogg if it doesn't exist
	if(eof_silence_loaded)
	{
		allegro_message("Drums Rock:  No chart audio is loaded so the export folder will be missing audio.");
		eof_log("No chart audio is loaded so the export folder will be missing audio.", 2);
	}
	else
	{
		(void) replace_filename(eof_temp_filename, eof_temp_filename, "song.ogg", (int) sizeof(eof_temp_filename));
		if(!exists(eof_temp_filename))
		{
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tWriting \"%s\"", eof_temp_filename);
			eof_log(eof_log_string, 2);
			(void) eof_save_ogg(eof_temp_filename);
		}
	}

	//Write preview.ogg if it doesn't exist and it has been created for the project
	(void) replace_filename(temp_filename2, eof_song_path, "preview.ogg", 1024);
	(void) replace_filename(eof_temp_filename, eof_temp_filename, "preview.ogg", (int) sizeof(eof_temp_filename));
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tWriting \"%s\"", eof_temp_filename);
	eof_log(eof_log_string, 2);
	(void) eof_conditionally_copy_file(temp_filename2, eof_temp_filename);	//Copy preview.ogg there if a file of the same size doesn't already exist

	//Write album.jpg if it doesn't exist in the destination and it exists in the project folder
	(void) replace_filename(temp_filename2, eof_song_path, "album.jpg", 1024);
	(void) replace_filename(eof_temp_filename, eof_temp_filename, "album.jpg", (int) sizeof(eof_temp_filename));
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tWriting \"%s\"", eof_temp_filename);
	eof_log(eof_log_string, 2);
	(void) eof_conditionally_copy_file(temp_filename2, eof_temp_filename);	//Copy album.jpg there if a file of the same size doesn't already exist

	return 1;	//Return success
}

int eof_import_drums_rock_track_diff(char * fn)
{
	char *buffer = NULL, *buffer2 = NULL, *ptr, undo_made = 0, binary[9] = {0}, *suberror = "";
	PACKFILE *inf = NULL;
	EOF_PHRASE_SECTION *rp;
	size_t maxlinelength;
	int error = 0, drumrollerror = 0, retval;
	unsigned long linectr = 1, value = 0, ctr, ctr2, ctr3;
	EOF_NOTE *np;
	unsigned char gemconversion[7] = {0, 4, 8, 1, 32, 2, 16};	//Remap Drums Rocks's gem numbering to EOF note bitmask
	unsigned char note;

	eof_log("\tImporting Drums Rock file", 1);
	eof_log("eof_import_drums_rock_track_diff() entered", 1);

	if(!eof_song || !eof_song_loaded)
		return 1;	//For now, don't do anything unless a project is active
	if(!fn)
		return 1;	//Invalid parameter
	if(!exists(fn))
		return 1;	//Invalid parameter
	if(eof_song->track[eof_selected_track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR)
		return 1;	//For now, don't import unless a drum track is active

	//Parse info.csv if it exists
	(void) replace_filename(eof_temp_filename, fn, "info.csv", 1024);	//Build expected path to info.csv
	inf = pack_fopen(eof_temp_filename, "rt");				//Open file in text mode
	if(inf)
	{	//If info.csv was opened
		maxlinelength = (size_t)FindLongestLineLength_ALLEGRO(fn, 0);
		if(!maxlinelength)
		{
			eof_log("\tError finding the largest line in info.csv file.", 1);
			error = 1;
		}
		else
		{
			buffer = (char *)malloc(maxlinelength);
			buffer2 = (char *)malloc(maxlinelength);
			if(!buffer || !buffer2)
			{
				if(buffer)
					free(buffer);
				if(buffer2)
					free(buffer2);
				eof_log("\tError allocating memory.  Aborting", 1);
				(void) pack_fclose(inf);
				return 1;
			}
		}
		//Read first line of text, capping it to prevent buffer overflow
		if(!error)
		{
			if(!pack_fgets(buffer, (int)maxlinelength, inf))
			{	//I/O error
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Drums Rock import failed on line #%lu of info.csv:  Unable to read from file:  \"%s\"", linectr, strerror(errno));
				eof_log(eof_log_string, 1);
				error = 1;
			}
			else
			{	//Validate contents of first line
				if(strcasestr_normal(buffer, "Song Name,Author Name,Difficulty,Song Duration in seconds,Song Map") != buffer)
				{	//If the first line doesn't begin with the expected field definitions
					eof_log("info.csv header row content not recognized", 1);
					error = 1;
				}
			}
			linectr++;
		}
		//Read song properties from second line of text
		if(!error)
		{
			if(!pack_fgets(buffer, (int)maxlinelength, inf))
			{	//I/O error
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Drums Rock import failed on line #%lu of info.csv:  Unable to read from file:  \"%s\"", linectr, strerror(errno));
				eof_log(eof_log_string, 1);
				error = 1;
			}
			else
			{	//Parse the line's content
				ptr = ustrtok(buffer, ",");	//Initialize the tokenization and get first tokenized field (Song title)
				if(ptr && eof_check_string(ptr))
				{	//If the song title is defined
					if(!eof_check_string(eof_song->tags->title))
					{	//If the song title isn't defined for the active project
						(void) ustrcpy(eof_song->tags->title, ptr);	//Define it in song properties
					}
				}

				ptr = ustrtok(NULL, ",");	//Get the next tokenized field (artist name)
				if(ptr && eof_check_string(ptr))
				{	//If the artist is defined
					if(!eof_check_string(eof_song->tags->artist))
					{	//If the artist isn't defined for the active project
						(void) ustrcpy(eof_song->tags->artist, ptr);	//Define it in song properties
					}
				}

				ptr = ustrtok(NULL, ",");	//Get the next tokenized field (track difficulty)
				if(ptr && eof_check_string(ptr))
				{	//If the track difficulty is defined
					retval = eof_read_macro_number(ptr, &value);		//Convert the string into a number
					if(retval && (value != eof_note_type) && (value < 4))
					{	//If the number was parsed and it indicates the notes are for a difficulty other than the active difficulty, and the specified difficulty is valid
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "This notes.csv file pertains to the \"%s\" difficulty.", eof_note_type_name_dr[value]);
						if(alert(eof_log_string, NULL, "Change to that difficulty before importing?", "&Yes", "&No", 'y', 'n') == 1)
						{	//If the user opts to change to the specified difficulty
							eof_note_type = value;
						}
					}
				}
			}
		}

		if(error)
			eof_log("Parsing of info.csv skipped", 1);

		//Cleanup
		(void) pack_fclose(inf);
		free(buffer);
		free(buffer2);
		error = 0;
		linectr = 1;
	}

	//Check whether the active track difficulty has content
	for(ctr = eof_get_track_size(eof_song, eof_selected_track); ctr > 0; ctr--)
	{	//For each note in the track, in reverse order
		if(eof_get_note_type(eof_song, eof_selected_track, ctr - 1) == eof_note_type)
		{	//If the note is in the active track difficulty
			if(!undo_made)
			{	//If the user wasn't prompted about this yet
				if(alert("This track already has notes", "Importing this Drums Rock track will overwrite this track's contents", "Continue?", "&Yes", "&No", 'y', 'n') != 1)
				{	//If the active track difficulty is already populated and the user doesn't opt to overwrite it
					eof_log("\t\tUser cancellation", 1);
					return 2;	//Return user cancellation
				}
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				undo_made = 1;
			}
			eof_track_delete_note(eof_song, eof_selected_track, ctr - 1);	//Delete the note
		}
	}
	if(!undo_made)
	{
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		undo_made = 1;
	}
	eof_determine_phrase_status(eof_song, eof_selected_track);	//Update the drum roll status for existing notes, to check later

	//Parse notes.csv
	for(ctr = 0; ctr < 2; ctr++)
	{	//On the first iteration of the loop, attempt to open and validate the user-specified file, if it fails, retry with the file name "notes.csv" if that wasn't the name passed to this function
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tAttempting to parse notes CSV file:  \"%s\"", fn);
		eof_log(eof_log_string, 1);
		inf = pack_fopen(fn, "rt");				//Open file in text mode
		if(inf)
		{	//If the file was opened
			maxlinelength = (size_t)FindLongestLineLength_ALLEGRO(fn, 0);
			if(!maxlinelength)
			{
				eof_log("\tError finding the largest line in notes CSV file.  Skipping", 1);
				error = 1;
			}
			else
			{
				buffer = (char *)malloc(maxlinelength);
				if(!buffer)
				{
					eof_log("\tError allocating memory.  Aborting", 1);
					(void) pack_fclose(inf);
					return 1;
				}
			}
			//Read first line of text, capping it to prevent buffer overflow
			if(!error)
			{
				if(!pack_fgets(buffer, (int)maxlinelength, inf))
				{	//I/O error
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tDrums Rock import failed on line #%lu of notes CSV:  Unable to read from file:  \"%s\"", linectr, strerror(errno));
					eof_log(eof_log_string, 1);
					error = 1;
				}
				else
				{	//Validate contents of first line
					if(strcasestr_normal(buffer, "Time [s],Enemy Type,Aux Color 1,Aux Color 2,No Enemies,interval,Aux") != buffer)
					{	//If the first line doesn't begin with the expected field definitions
						eof_log("\tnotes CSV header row content not recognized.", 1);
						if(ctr == 0)
						{	//If this was the first pass of the loop, see if the user simply selected a file that isn't the expected notes.csv file
							if(stricmp(get_filename(fn), "notes.csv"))
							{	//If the user specified a file not named notes.csv
								(void) replace_filename(fn, fn, "notes.csv", 1024);	//Build expected path to notes.csv
								if(exists(fn))
								{	//If that file exists, attempt to load it in the next for loop iteration
									eof_log("\tnotes.csv was not specified as the import file, but it does exist, attempting to import that file.", 1);
									(void) pack_fclose(inf);
								}
								else
								{	//There is no other known file to parse other than the user-specified one
									eof_log("\tSpecified CSV file failed to import and notes.csv does not exist at that folder level.  Aborting.", 1);
									(void) pack_fclose(inf);
									free(buffer);
									return 1;
								}
							}
						}
						else
						{	//If this was the second pass of the for loop, neither the user-specified file nor explicitly-named notes.csv file could be parsed
							eof_log("\tnotes.csv also failed to be parsed.  Aborting.", 1);
							(void) pack_fclose(inf);
							free(buffer);
							return 1;
						}
					}
					else
					{	//The file passed the header row validation, exit for loop and continue parsing the file
						break;
					}
				}
			}
		}
	}
	(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
	linectr++;	//Increment line counter

	//Parse the contents of the file
	while(!error && !pack_feof(inf))
	{	//Until there was an error or end of file is reached
		double timestamp;
		unsigned long enemytype, auxcolor1, auxcolor2, interval;
		char *intervalstring = NULL;

		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tProcessing line #%lu", linectr);
		eof_log(eof_log_string, 3);

		ptr = ustrtok(buffer, ",");	//Initialize the tokenization and get first tokenized field (note timestamp)
		if(ptr)
		{	//If the timestamp was tokenized
			timestamp = atof(ptr) * 1000.0;	//Convert timestamp to milliseconds
		}
		else
		{
			suberror = "Could not read timestamp";
			error = 1;
			break;	//Stop parsing file
		}

		ptr = ustrtok(NULL, ",");	//Get the next tokenized field (enemy type)
		if(ptr)
		{	//If the enemy type was tokenized
			retval = eof_read_macro_number(ptr, &enemytype);
			if(!retval || (enemytype == 0) || (enemytype > 3))
			{
				suberror = "Invalid enemy type";
				error = 1;	//The enemy type was not parsed or is invalid
				break;	//Stop parsing file
			}
		}
		else
		{
			suberror = "Could not read enemy type";
			error = 1;
			break;	//Stop parsing file
		}

		ptr = ustrtok(NULL, ",");	//Get the next tokenized field (aux color 1)
		if(ptr)
		{	//If the aux color 1 field was tokenized
			retval = eof_read_macro_number(ptr, &auxcolor1);
			if(!retval || (auxcolor1 == 0) || (auxcolor1 > 6))
			{
				suberror = "Invalid aux color 1";
				error = 1;	//The aux color 1 value was not parsed or is invalid
				break;	//Stop parsing file
			}
		}
		else
		{
			suberror = "Could not read aux color 1";
			error = 1;
			break;	//Stop parsing file
		}

		ptr = ustrtok(NULL, ",");	//Get the next tokenized field (aux color 2)
		if(ptr)
		{	//If the aux color 2 field was tokenized
			retval = eof_read_macro_number(ptr, &auxcolor2);
			if(!retval || (auxcolor2 == 0) || (auxcolor2 > 6))
			{
				suberror = "Invalid aux color 2";
				error = 1;	//The aux color 2 value was not parsed or is invalid
				break;	//Stop parsing file
			}
		}
		else
		{
			suberror = "Could not read aux color 2";
			error = 1;
			break;	//Stop parsing file
		}

		ptr = ustrtok(NULL, ",");	//Get the next tokenized field (Number of enemies), which is ignored
		if(!ptr)
		{
			suberror = "Could not read number of enemies";
			error = 1;
			break;	//Stop parsing file
		}

		intervalstring = ustrtok(NULL, ",");	//Get the next tokenized field (interval)
		if(intervalstring)
		{	//If the interval field was tokenized
			retval = eof_read_macro_number(ptr, &interval);
			if(enemytype && (!retval || (interval == 0)))
			{
				suberror = "Invalid interval";
				error = 1;	//The interval value must be defined because the note is a drum roll, but it was not parsed or is invalid
				break;	//Stop parsing file
			}
		}
		else
		{
			suberror = "Could not read interval";
			error = 1;
			break;	//Stop parsing file
		}

		(void) ustrtok(NULL, ",");	//Get the next tokenized field (Aux), which is intentionally ignored

		//Create note
		if(auxcolor1 > 6)
			auxcolor1 = 0;	//Bounds check
		if(auxcolor2 > 6)
			auxcolor2 = 0;	//Bounds check
		note = gemconversion[auxcolor1];
		if(enemytype == 2)
			note += gemconversion[auxcolor2];	//Add the second note's bitmask if this is a chord
		if(enemytype != 3)
			intervalstring = "";		//If this note isn't a drum roll, ensure it is created with no name string

		np = eof_track_add_create_note(eof_song, eof_selected_track, note, timestamp + 0.5, 1, eof_note_type, intervalstring);
		if(!np)
		{	//If the note was not added
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Drums Rock import failed on line #%lu of notes.csv:  Unable to add note.  Aborting.", linectr);
			eof_log(eof_log_string, 1);
			error = 2;
			break;	//Stop parsing file
		}
		else
		{
			(void) eof_byte_to_binary_string(note, binary);	//Make a string representation of the note bitmask
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tAdded note:  Mask = %u (%s), pos = %lu, enemytype = %lu", note, binary, (unsigned long)(timestamp + 0.5), enemytype);
			eof_log(eof_log_string, 2);

			if(enemytype == 3)
			{	//If this note is a drum roll
				np->flags |= EOF_NOTE_FLAG_IS_TREMOLO;	//Track that this note needs to be in a drum roll
			}
		}

		(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
		linectr++;	//Increment line counter
	}//Until there was an error or end of file is reached

	if(error == 1)
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Drums Rock import failed on line #%lu of notes.csv:  Malformed note (%s).", linectr, suberror);
		eof_log(eof_log_string, 1);
	}

	//Cleanup
	(void) pack_fclose(inf);
	free(buffer);
	eof_track_sort_notes(eof_song, eof_selected_track);

	//Create drum rolls
	eof_log("\tCreating drum rolls", 2);
	for(ctr = 0; ctr < eof_get_track_size(eof_song, eof_selected_track); ctr++)
	{	//For each note in the active track
		if(eof_get_note_type(eof_song, eof_selected_track, ctr) == eof_note_type)
		{	//If the note was one that was just imported
			unsigned long pos = eof_get_note_pos(eof_song, eof_selected_track, ctr), pos2;
			char inroll = 0;

			if(eof_get_note_flags(eof_song, eof_selected_track, ctr) & EOF_NOTE_FLAG_IS_TREMOLO)
			{	//If the note needs to be marked as a drum roll
				for(ctr2 = 0; ctr2 < eof_get_num_tremolos(eof_song, eof_selected_track); ctr2++)
				{	//For each tremolo phrase in the active track
					rp = eof_get_tremolo(eof_song, eof_selected_track, ctr2);
					if((pos >= rp->start_pos) && (pos <= rp->end_pos))
					{	//If the note is in an existing drum roll phrase
						inroll = 1;
						break;
					}
				}
				if(!inroll)
				{	//If the note wasn't found to be in an existing drum roll yet, check special drum rolls
					for(ctr2 = 0; ctr2 < eof_get_num_trills(eof_song, eof_selected_track); ctr2++)
					{	//For each trill phrase in the active track
						rp = eof_get_trill(eof_song, eof_selected_track, ctr2);
						if((pos >= rp->start_pos) && (pos <= rp->end_pos))
						{	//If the note is in an existing special drum roll phrase
							inroll = 1;
							break;
						}
					}
				}
				if(!inroll)
				{	//If the note wasn't found to be in an existing drum roll, add a drum roll for it
					if(!eof_track_add_tremolo(eof_song, eof_selected_track, pos, pos + 1, 0xFF))
					{	//If the drum roll could not be added
						if(!drumrollerror)
						{	//If the user wasn't warned about this yet
							char *error_message = "Warning:  At least one drum roll could not be added during import";
							allegro_message(error_message);
							eof_log(error_message, 1);
							drumrollerror = 1;
						}
					}
					else
					{	//Suppress the drum roll for notes that pre-existed this import and were not already in a drum roll
						for(ctr3 = 0; ctr3 < eof_get_track_size(eof_song, eof_selected_track); ctr3++)
						{	//For each note in the track
							if(eof_get_note_type(eof_song, eof_selected_track, ctr3) == eof_note_type)
								continue;	//If this is a note that was just imported, skip it

							pos2 = eof_get_note_pos(eof_song, eof_selected_track, ctr3);
							if(pos2 > pos + 1)
								break;	//If this note and all others are after the newly added drum roll, stop checking the notes

							if(pos2 >= pos)
							{	//If this note overlaps the newly added drum roll
								if(!(eof_get_note_flags(eof_song, eof_selected_track, ctr3) & (EOF_NOTE_FLAG_IS_TRILL | EOF_NOTE_FLAG_IS_TREMOLO)))
								{	//If, before the import, this note was in neither a trill nor a tremolo phrase as determined by eof_determine_phrase_status() earlier
									eof_set_note_name(eof_song, eof_selected_track, ctr3, "0");	//Ensure the drum roll is suppressed for this note
								}
							}
						}
					}
				}
			}
		}
	}

	return error;	//Return current error status
}
