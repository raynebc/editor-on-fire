#include <allegro.h>
#include "dr.h"
#include "main.h"
#include "song.h"
#include "utility.h"

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
	char has_comma = 0;
	unsigned long ctr, index = 0;

	if(!input || !output)
		return;	//Invalid parameters

	output[0] = '\0';	//Terminate the output string
	//Check for commas
	for(ctr = 0; input[ctr] != '\0'; ctr++)
	{	//For each character in the input string
		if(input[ctr] == ',')
		{
			has_comma = 1;
			break;
		}
	}

	//Write the output string
	if(has_comma)
		uinsert(output, index++, '"');	//Append double quotation mark if any commas are in the source string

	for(ctr = 0; input[ctr] != '\0'; ctr++)
	{	//For each character in the input string
		uinsert(output, index++, input[ctr]);	//Append it to the output string
		if(input[ctr] == '"')
		{	//Writing " inside a CSV field requires writing it as two double quotation marks back to back
			uinsert(output, index++, input[ctr]);
		}
	}

	if(has_comma)
		uinsert(output, index++, '"');	//End with a double quotation mark if any commas are in the source string

	uinsert(output, index, '\0');	//Terminate the output string
}

int eof_export_drums_rock_track_diff(EOF_SONG * sp, unsigned long track, unsigned char diff, char *destpath)
{
	PACKFILE *fp;
	int err, ret;
	char temp_string[1024], temp_filename2[1024];
	unsigned long ctr;

	//Use song metadata and difficulty level to build the "Artist - Song - Difficulty" string and build a subfolder of that name in the project folder
	//write info.csv, notes.csv, preview.ogg, song.ogg
	if(diff > 4)
		return 0;	//Invalid difficulty level
	if(!eof_track_is_drums_rock_mode(sp, track))
		return 0;	//Not a valid track or Drums Rock is not enabled
	if(!destpath)
		return 0;	//Invalid destination folder path

	(void) eof_detect_difficulties(sp, track);	//Update difficulties to reflect the track being exported
	if(!eof_track_diff_populated_status[diff])
		return 0;	//This difficulty is empty

	//Build the path to the Drums Rock folder for this track difficulty
	(void) replace_filename(eof_temp_filename, destpath, "", 1024);	//Obtain the destination folder path
	put_backslash(eof_temp_filename);
	if(eof_check_string(eof_song->tags->artist))
	{	//If the artist of the song is defined
		(void) ustrcat(eof_temp_filename, eof_song->tags->artist);
		(void) ustrcat(eof_temp_filename, " - ");
	}
	if(eof_check_string(eof_song->tags->title))
	{	//If the title of the song is defined
		(void) ustrcat(eof_temp_filename, eof_song->tags->title);
		(void) ustrcat(eof_temp_filename, " - ");
	}
	(void) ustrcat(eof_temp_filename, eof_note_type_name_dr[diff]);

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
	fp = pack_fopen(eof_temp_filename, "w");
	if(!fp)
	{
		eof_log("\tError saving:  Cannot open info.csv for writing", 1);
		return 0;	//Return failure
	}
	pack_fputs("Song Name,Author Name,Difficulty,Song Duration in seconds,Song Map\n", fp);
	eof_build_sanitized_drums_rock_string(eof_song->tags->title, temp_string);
	(void) pack_fputs(temp_string, fp);	//Write song title
	(void) pack_putc(',', fp);
	eof_build_sanitized_drums_rock_string(eof_song->tags->artist, temp_string);
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
	(void) replace_filename(eof_temp_filename, eof_temp_filename, "Metadata.cfg", (int) sizeof(eof_temp_filename));
	fp = pack_fopen(eof_temp_filename, "w");
	if(!fp)
	{
		eof_log("\tError saving:  Cannot open Metadata.cfg for writing", 1);
		return 0;	//Return failure
	}
	(void) snprintf(temp_string, sizeof(temp_string) - 1, "title=%s - %s\n", sp->tags->artist, sp->tags->title);
	(void) pack_fputs(temp_string, fp);		//Write song title
	(void) snprintf(temp_string, sizeof(temp_string) - 1, "description=%s, %s\n", sp->tags->frettist, sp->tags->loading_text);
	(void) pack_fputs(temp_string, fp);		//Write description
	(void) pack_fputs("isPublic=true\n", fp);
	(void) snprintf(temp_string, sizeof(temp_string) - 1, "tags=%s, %s\n", eof_note_type_name_dr[diff], sp->tags->genre);
	(void) pack_fputs(temp_string, fp);		//Write tags
	(void) pack_fclose(fp);

	//Write song.ogg if it doesn't exist
	(void) replace_filename(eof_temp_filename, eof_temp_filename, "song.ogg", (int) sizeof(eof_temp_filename));
	if(!exists(eof_temp_filename))
		(void) eof_save_ogg(eof_temp_filename);

	//Write preview.ogg if it doesn't exist and it has been created for the project
	(void) replace_filename(temp_filename2, eof_song_path, "preview.ogg", 1024);
	if(exists(temp_filename2))
	{	//If the project has preview audio defined
		(void) replace_filename(eof_temp_filename, eof_temp_filename, "preview.ogg", (int) sizeof(eof_temp_filename));
		if(!exists(eof_temp_filename))
		{	//If preview.ogg doesn't exist in the Drums Rock export subfolder
			(void) eof_copy_file(temp_filename2, eof_temp_filename);	//Copy preview.ogg there
		}
	}

	//Write album.jpg if it doesn't exist in the destination and it exists in the project folder
	(void) replace_filename(temp_filename2, eof_song_path, "album.jpg", 1024);
	if(exists(temp_filename2))
	{	//If the project folder has album.jpg
		(void) replace_filename(eof_temp_filename, eof_temp_filename, "album.jpg", (int) sizeof(eof_temp_filename));
		if(!exists(eof_temp_filename))
		{	//If album.jpg doesn't exist in the Drums Rock export subfolder
			(void) eof_copy_file(temp_filename2, eof_temp_filename);	//Copy album.jpg there
		}
	}

	return 1;	//Return success
}
