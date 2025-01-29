#include <allegro.h>
#include "dr.h"
#include "main.h"
#include "song.h"
#include "utility.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

/*

Export difficulty:
1.  Create subfolder ("Artist - Song - Difficulty")
2.  Create info.csv
3.  Create notes.csv
	For up to the first two gems in each note, check each bit in the note mask to track whether one or two gems are used, and which colors they are
	For now, have a global variable for each lane and each cymbal, with a value mapping them to a specific note type for Drums Rock, and have these mappings defined as config file entries
4.  Create preview.ogg if it doesn't exist yet
5.  Create song.ogg if it doesn't exist yet

Export chart
1.  Check each note in PART DRUMS to see if any of them have more than 2 gems
	If so, offer to cancel save and seek to first offending note, offer to highlight all offending notes
2.  Export each populated difficulty in PART DRUMS

*/

char        eof_note_type_name_dr[5][10] = {"Easy", "Medium", "Hard", "Extreme", "Extreme+"};

unsigned long eof_get_note_name_as_number(EOF_SONG * sp, unsigned long track, unsigned long notenum)
{
	unsigned long count = 0;
	char *name;
	int retval;

	name = eof_get_note_name(sp, track, notenum);
	retval = eof_read_macro_number(name, &count);		//Convert the note name into a number
	if(!retval)
		return 0;	//Error parsing number

	return count;
}

int eof_check_drums_rock_track(EOF_SONG * sp, unsigned long track)
{
	unsigned long ctr;
	int ret, drum_roll_warned = 0;

	if(!eof_track_is_drums_rock_mode(sp, track))
		return 0;	//Not a Drums Rock enabled track

	//Validate drum roll hit counts (must be between 1 and 100)
	eof_determine_phrase_status(sp, track);	//Update the trill and tremolo status of each note (used to mark drum rolls)
	for(ctr = 0; ctr < eof_get_track_size(sp, track); ctr++)
	{	//For each note in the track
		unsigned long flags = eof_get_note_flags(sp, track, ctr);
		if((flags & EOF_NOTE_FLAG_IS_TREMOLO) || (flags & EOF_NOTE_FLAG_IS_TRILL))
		{	//If the note is in a drum roll or special drum roll
			unsigned long number = eof_get_note_name_as_number(sp, track, ctr);

			if(!number || (number > 100))
			{	//If the drum roll hit count is not valid
				if(!drum_roll_warned)
				{	//If the user wasn't warned about this yet
					ret = alert3("Drums Rock:  At least one drum roll does not have a valid hit count", NULL, "Cancel and seek to first offending roll?", "Yes", "No", "Highlight all and cancel", 0, 0, 0);
					if(ret == 2)
					{	//User declined
						drum_roll_warned = 1;
						break;	//Stop checking the drum rolls
					}

					eof_seek_and_render_position(track, eof_get_note_type(sp, track, ctr), eof_get_note_pos(sp, track, ctr));	//Seek to the first offending note
					if(ret == 1)
					{	//If the user opted to cancel the save
						return 1;	//Return cancellation
					}

					drum_roll_warned = 3;
				}
				if(drum_roll_warned == 3)
				{	//User opted to highlight all offending drum rolls
					eof_set_note_flags(sp, track, ctr, flags | EOF_NOTE_FLAG_HIGHLIGHT);
				}
			}
		}
	}
	if(drum_roll_warned == 3)
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

unsigned char eof_reduce_drums_rock_note_mask(unsigned char note)
{
	unsigned ctr, mask, count = 0;
	unsigned char newnote = 0;

	//Special rule:  If a drum chord includes bass, always keep that gem
	if(note & 8)
	{
		newnote |= 8;
		count++;
	}

	//Special rule:  If a drum chord includes snare, always keep that gem
	if(note & 4)
	{
		newnote |= 4;
		count++;
	}

	//Keep up to a total of two gems from whatever is remaining
	for(ctr = 0, mask = 1; (ctr < 6) && (count < 2); ctr++, mask <<= 1)
	{	//For each of the 6 supported lanes, only until two gems are selected to be kept
		if((note & mask) && !(newnote & mask))
		{	//If there is a gem on this lane that isn't already copied into the output mask
			count++;
			newnote |= mask;	//Set it in the new note mask
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

int eof_export_drums_rock_track_diff(EOF_SONG * sp, unsigned long track, unsigned char diff)
{
	PACKFILE *fp;
	int err;
	char temp_string[1024], temp_filename2[1024];
	unsigned long ctr;

	//Use song metadata and difficulty level to build the "Artist - Song - Difficulty" string and build a subfolder of that name in the project folder
	//write info.csv, notes.csv, preview.ogg, song.ogg
	if(diff > 4)
		return 0;	//Invalid difficulty level
	if(!eof_track_is_drums_rock_mode(sp, track))
		return 0;	//Not a valid track or Drums Rock is not enabled

	(void) eof_detect_difficulties(sp, track);	//Update difficulties to reflect the track being exported
	if(!eof_track_diff_populated_status[diff])
		return 0;	//This difficulty is empty

	//Build the path to the Drums Rock folder for this track difficulty
	(void) replace_filename(eof_temp_filename, eof_song_path, "", 1024);	//Obtain the project folder path
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
	(void) pack_fwrite(temp_string, ustrsize(temp_string), fp);	//Write song title
	(void) pack_putc(',', fp);
	eof_build_sanitized_drums_rock_string(eof_song->tags->artist, temp_string);
	(void) pack_fwrite(temp_string, ustrsize(temp_string), fp);	//Write artist name
	(void) pack_putc(',', fp);
	(void) snprintf(temp_string, sizeof(temp_string) - 1, "%u", diff);
	(void) pack_fwrite(temp_string, ustrsize(temp_string), fp);	//Write difficulty level
	(void) pack_putc(',', fp);
	(void) snprintf(temp_string, sizeof(temp_string) - 1, "%lu", eof_music_length / 1000);
	(void) pack_fwrite(temp_string, ustrsize(temp_string), fp);	//Write song length
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
		unsigned long flags, ctr2, bitmask, gem1 = 0, gem2 = 0, enemytype = 0, drumrollcount = 0, aux;
		unsigned char notemask;

		//Process note for export
		notemask = eof_reduce_drums_rock_note_mask(eof_get_note_note(sp, track, ctr));	//Convert the note bitmask for Drums Rock
		for(ctr2 = 0, bitmask = 1, gem1= 0, gem2 = 0; ctr2 < 6; ctr2++, bitmask <<= 1)
		{	//For each of the 6 supported lanes
			if(notemask & bitmask)
			{	//If this lane is used
				if(!gem1)
				{	//If this is the first gem seen for this note
					gem1 = gem2 = ctr2 + 1;	//Drums rock lane ordering begins with 1 instead of 0
					enemytype = 1;			//Export as single gem drum note
				}
				else
				{	//This is the second gem seen for this note
					gem2 = ctr2 + 1;	//Drums rock lane ordering begins with 1 instead of 0
					enemytype = 2;			//Export as dual gem drum note
					break;	//There will be no third gem to export
				}
			}
		}
		flags = eof_get_note_flags(sp, track, ctr);
		if((flags & EOF_NOTE_FLAG_IS_TREMOLO) || (flags & EOF_NOTE_FLAG_IS_TRILL))
		{	//If the note is in a drum roll or special drum roll
			drumrollcount = eof_get_note_name_as_number(sp, track, ctr);

			enemytype = 3;	//Export as a drum roll
			if(!drumrollcount || (drumrollcount > 100))
			{	//If the drum roll hit count is not valid, write it as 3
				drumrollcount = 3;
			}
		}

		//Write the note data
		(void) snprintf(temp_string, sizeof(temp_string) - 1, "%.2f,", (double) eof_get_note_pos(sp, track, ctr) / 1000.0);
		(void) pack_fwrite(temp_string, ustrsize(temp_string), fp);	//Write timestamp
		(void) snprintf(temp_string, sizeof(temp_string) - 1, "%lu,", enemytype);
		(void) pack_fwrite(temp_string, ustrsize(temp_string), fp);	//Write enemy type
		(void) snprintf(temp_string, sizeof(temp_string) - 1, "%lu,%lu,1,", gem1, gem2);
		(void) pack_fwrite(temp_string, ustrsize(temp_string), fp);	//Write enemy color 1, color 2 and number of enemies fields
		if(drumrollcount)
		{	//If this note is a drum roll
			(void) snprintf(temp_string, sizeof(temp_string) - 1, "%lu,", drumrollcount);
			(void) pack_fwrite(temp_string, ustrsize(temp_string), fp);	//Write drum roll interval (hit count)
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
		(void) pack_fwrite(temp_string, ustrsize(temp_string), fp);	//Write Aux value and end the line
	}
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
			(void) eof_copy_file(temp_filename2, eof_temp_filename);	//Copies the source file to the destination file.  Returns 0 upon error
		}
	}

	return 1;	//Return success
}
