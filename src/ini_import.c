#include <allegro.h>
#include <stdio.h>
#include "song.h"
#include "utility.h"	//For eof_buffer_file()
#include "ini.h"		//For eof_difficulty_ini_tags[]
#include "main.h"		//For logging
#include "ini_import.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

typedef struct
{

	char type[256];
	char value[1024];

} EOF_IMPORT_INI_SETTING;

EOF_IMPORT_INI_SETTING eof_import_ini_setting[EOF_MAX_INI_SETTINGS];
int eof_import_ini_settings = 0;

char eof_ini_pro_drum_tag_present;		//Is set to nonzero if eof_import_ini() finds the "pro_drums = True" tag (to influence MIDI import)
char eof_ini_star_power_tag_present;	//Is set to nonzero if eof_import_ini() finds the "multiplier_note = 116" tag (to influence MIDI import)
char eof_ini_sysex_open_bass_present;	//Is set to nonzero if eof_import_ini() finds the "sysex_open_bass = True" tag (to influence MIDI import)

/* it would probably be easier to use Allegro's configuration routines to read
 * the ini files since it looks like they are formatted correctly */
int eof_import_ini(EOF_SONG * sp, char * fn)
{
	eof_log("eof_import_ini() entered", 1);

	char * textbuffer = NULL;
	char * line_token = NULL;
	int textlength = 0;
	char * token;
	char * equals = NULL;
	int i;
	int j;
	unsigned long stringlen, tracknum;
	char setting_stored;
	unsigned ctr;	//Used to count the number of strings defined in pro guitar/bass tuning tag
	char *value_index;

	eof_ini_pro_drum_tag_present = 0;	//Reset this condition to false
	eof_ini_star_power_tag_present = 0;	//Reset this condition to false
	eof_ini_sysex_open_bass_present = 0;//Reset this condition to false

	if(!sp || !fn)
	{
		return 0;
	}
	eof_log("\tBuffering INI file into memory", 1);
	eof_import_ini_settings = 0;
	textbuffer = (char *)eof_buffer_file(fn, 1);	//Buffer the INI file into memory, adding a NULL terminator at the end of the buffer
	if(!textbuffer)
	{
		eof_log("\tCannot open INI file, skipping", 1);
		return 0;
	}
	textlength = ustrlen(textbuffer);
	eof_log("\tTokenizing INI file buffer", 1);
	ustrtok(textbuffer, "\r\n[]");
	eof_log("\tParsing INI file buffer", 1);
	while(1)
	{
		line_token = ustrtok(NULL, "\r\n[]");
		if(!line_token)
		{
			break;
		}
		else
		{
			if(ustrlen(line_token) > 6)
			{
				/* find the first '=' */
				for(i = 0; i < ustrlen(line_token); i++)
				{
					if(ugetc(&line_token[uoffset(line_token, i)]) == '=')
					{
						equals = &line_token[uoffset(line_token, i)];
						break;
					}
				}

				/* if this line has an '=', process line as a setting */
				if(equals)
				{
					if(eof_import_ini_settings == EOF_MAX_INI_SETTINGS)
						break;	//Break from while loop if the eof_import_ini_setting[] array is full

					equals[0] = '\0';
					token = equals + 1;
					ustrncpy(eof_import_ini_setting[eof_import_ini_settings].type, line_token, 256-1);
					ustrncpy(eof_import_ini_setting[eof_import_ini_settings].value, token, 1024-1);
					while(1)
					{	//Drop all trailing space characters from the tag type string
						stringlen = ustrlen(eof_import_ini_setting[eof_import_ini_settings].type);
						if(stringlen < 1)
							break;
						if(eof_import_ini_setting[eof_import_ini_settings].type[stringlen - 1] == ' ')
						{	//If the last character in this tag type is a space character, truncate it to simplify the tag matching below
							eof_import_ini_setting[eof_import_ini_settings].type[stringlen - 1] = '\0';
						}
						else
							break;
					}
					if(eof_import_ini_setting[eof_import_ini_settings].type[0] != '\0')
					{	//If the type string wasn't just space characters up to the equal sign, add the INI setting
						eof_import_ini_settings++;
					}
				}
			}
		}
	}
	eof_log("\tProcessing INI file contents", 1);
	for(i = 0; i < eof_import_ini_settings; i++)
	{	//For each imported INI setting
		if(eof_import_ini_setting[i].type == '\0')
			continue;	//Skip this INI type if its string is empty

		value_index = eof_import_ini_setting[i].value;	//Prepare to skip leading whitespace
		while((*value_index != '\0') && (*value_index == ' '))
		{	//If this is a space character
			value_index++;	//Point to the next character
		}

		if(*value_index != '\0')
		{	//If the value portion of the entry has content
			if(!ustricmp(eof_import_ini_setting[i].type, "artist"))
			{
				ustrncpy(sp->tags->artist, value_index, 256-1);
			}
			else if(!ustricmp(eof_import_ini_setting[i].type, "name"))
			{
				ustrncpy(sp->tags->title, value_index, 256-1);
			}
			else if(!ustricmp(eof_import_ini_setting[i].type, "frets"))
			{
				ustrncpy(sp->tags->frettist, value_index, 256-1);
			}
			else if(!ustricmp(eof_import_ini_setting[i].type, "year"))
			{
				ustrncpy(sp->tags->year, value_index, 32-1);
			}
			else if(!ustricmp(eof_import_ini_setting[i].type, "loading_phrase"))
			{
				ustrncpy(sp->tags->loading_text, value_index, 512-1);
			}
			else if(!ustricmp(eof_import_ini_setting[i].type, "lyrics"))
			{
				if(!ustricmp(value_index, "True"))
				{
					sp->tags->lyrics = 1;
				}
			}
			else if(!ustricmp(eof_import_ini_setting[i].type, "eighthnote_hopo"))
			{
				if(!ustricmp(value_index, "1"))
				{
					sp->tags->eighth_note_hopo = 1;
				}
			}
			else if(!ustricmp(eof_import_ini_setting[i].type, "delay"))
			{
				sp->tags->ogg[0].midi_offset = atoi(value_index);
				if(sp->tags->ogg[0].midi_offset < 0)
				{
					sp->tags->ogg[0].midi_offset = 0;
				}
			}
			else if(!ustricmp(eof_import_ini_setting[i].type, "score"))
			{
			}
			else if(!ustricmp(eof_import_ini_setting[i].type, "scores"))
			{
			}
			else if(!ustricmp(eof_import_ini_setting[i].type, "scores_ext"))
			{
			}
///This handling is obsolete due to the sysex_open_bass INI tag
//			else if(!ustricmp(eof_import_ini_setting[i].type, "open_strum"))
//			{	//The deprecated open strum INI tag
//				if(!ustricmp(value_index, "True"))
//				{
//					sp->track[EOF_TRACK_BASS]->flags |= EOF_TRACK_FLAG_SIX_LANES;	//Set the sixth lane flag
//				}
//			}
			else if(!ustricmp(eof_import_ini_setting[i].type, "real_guitar_tuning"))
			{
				ctr = 0;	//Reset counter
				tracknum = sp->track[EOF_TRACK_PRO_GUITAR]->tracknum;
				line_token = ustrtok(value_index, " \r\n");	//Find first token (string of characters that isn't whitespace, carriage return or newline)
				while(line_token != NULL)
				{	//For each string tuning that is parsed
					if(line_token[0] == '"')
						break;	//Stop parsing if the tuning name string has been reached
					if(ctr >= EOF_TUNING_LENGTH)
						break;	//Do not read more string tunings than are supported
					sp->pro_guitar_track[tracknum]->tuning[ctr] = atol(line_token) % 12;	//Convert the string to an integer value
					ctr++;	//Increment the counter
					line_token = ustrtok(NULL, " \r\n");	//Find next token
				}
				sp->pro_guitar_track[tracknum]->numstrings = ctr;	//Define the number of strings in the track based on the tuning tag
				if((ctr < 4 || ctr > 6))
				{	//If the number of strings defined isn't supported
					allegro_message("Warning:  Invalid pro guitar tuning tag.  Reverting to 6 string standard tuning.");
					sp->pro_guitar_track[tracknum]->numstrings = 6;
					memset(sp->pro_guitar_track[tracknum]->tuning, 0, EOF_TUNING_LENGTH);
				}
				//Copy the parsed tuning data to the 22 fret pro guitar track
				unsigned long tracknum2 = sp->track[EOF_TRACK_PRO_GUITAR_22]->tracknum;
				memcpy(sp->pro_guitar_track[tracknum2]->tuning, sp->pro_guitar_track[tracknum], EOF_TUNING_LENGTH);	//Copy the tuning array
				sp->pro_guitar_track[tracknum2]->numstrings = sp->pro_guitar_track[tracknum]->numstrings;	//Copy the string count
			}
			else if(!ustricmp(eof_import_ini_setting[i].type, "real_bass_tuning"))
			{
				ctr = 0;	//Reset counter
				tracknum = sp->track[EOF_TRACK_PRO_BASS]->tracknum;
				line_token = ustrtok(value_index, " \r\n");	//Find first token (string of characters that isn't whitespace, carriage return or newline)
				while(line_token != NULL)
				{	//For each string tuning that is parsed
					if(line_token[0] == '"')
						break;	//Stop parsing if the tuning name string has been reached
					if(ctr >= EOF_TUNING_LENGTH)
						break;	//Do not read more string tunings than are supported
					sp->pro_guitar_track[tracknum]->tuning[ctr] = atol(line_token) % 12;	//Convert the string to an integer value
					ctr++;	//Increment the counter
					line_token = ustrtok(NULL, " \r\n");	//Find next token
				}
				sp->pro_guitar_track[tracknum]->numstrings = ctr;	//Define the number of strings in the track based on the tuning tag
				if((ctr < 4 || ctr > 6))
				{	//If the number of strings defined isn't supported
					allegro_message("Warning:  Invalid pro bass tuning tag.  Reverting to 6 string standard tuning.");
					sp->pro_guitar_track[tracknum]->numstrings = 6;
					memset(sp->pro_guitar_track[tracknum]->tuning, 0, EOF_TUNING_LENGTH);
				}
				//Copy the parsed tuning data to the 22 fret pro bass track
				unsigned long tracknum2 = sp->track[EOF_TRACK_PRO_BASS_22]->tracknum;
				memcpy(sp->pro_guitar_track[tracknum2]->tuning, sp->pro_guitar_track[tracknum], EOF_TUNING_LENGTH);	//Copy the tuning array
				sp->pro_guitar_track[tracknum2]->numstrings = sp->pro_guitar_track[tracknum]->numstrings;	//Copy the string count
			}
			else if(!ustricmp(eof_import_ini_setting[i].type, "pro_drums"))
			{
				if(!ustricmp(value_index, "True"))
				{
					eof_ini_pro_drum_tag_present = 1;
				}
			}
			else if(!ustricmp(eof_import_ini_setting[i].type, "five_lane_drums"))
			{
				if(!ustricmp(value_index, "True"))
				{
					unsigned long tracknum = sp->track[EOF_TRACK_DRUM]->tracknum;
					sp->track[EOF_TRACK_DRUM]->flags |= EOF_TRACK_FLAG_SIX_LANES;	//Set the five lane drum flag
					sp->legacy_track[tracknum]->numlanes = 6;						//Set the lane count
				}
			}
			else if(!ustricmp(eof_import_ini_setting[i].type, "multiplier_note"))
			{
				if(!ustricmp(value_index, "116"))
				{
					eof_ini_star_power_tag_present = 1;	//MIDI import won't have to convert solos phrases to star power, EOF's notation for star power style phrases was found
				}
			}
			else if(!ustricmp(eof_import_ini_setting[i].type, "sysex_open_bass"))
			{
				if(!ustricmp(value_index, "True"))
				{
					eof_ini_sysex_open_bass_present = 1;	//MIDI import will interpret forced HOPO lane 1 bass to be a HOPO bass gem and not an open strum
				}
			}
			else if(!ustricmp(eof_import_ini_setting[i].type, "sysex_pro_slide") || !ustricmp(eof_import_ini_setting[i].type, "sysex_high_hat_ctrl") || !ustricmp(eof_import_ini_setting[i].type, "sysex_rimshot") || !ustricmp(eof_import_ini_setting[i].type, "sysex_slider"))
			{	//These Sysex indicators are only used by Phase Shift
			}

			/* for custom settings or difficulty strings */
			else
			{
				setting_stored = 0;
				for(j = 0; (j < EOF_TRACKS_MAX + 1) && (j < sp->tracks); j++)
				{	//For each string in the eof_difficulty_ini_tags[] array (for each currently supported track number)
					if(eof_difficulty_ini_tags[j] && !ustricmp(eof_import_ini_setting[i].type, eof_difficulty_ini_tags[j]))
					{	//If this INI setting matches the difficulty tag, store the difficulty value into the appropriate track structure
						int value = atoi(value_index);
						if(value >= 0)
						{	//Only store the difficulty if it isn't negative (-1 means empty track)
							sp->track[j]->difficulty = value;
						}
						setting_stored = 1;	//Consider this INI tag handled (if it was -1, it will be dropped instead of being stored as a custom INI tag)
						break;
					}
				}
				if(!setting_stored)
				{
					if(!ustricmp(eof_import_ini_setting[i].type, "diff_drums_real"))
					{	//If this is a pro drum difficulty tag
						sp->track[EOF_TRACK_DRUM]->flags &= ~(0xFF << 24);		//Clear the drum track's flag's most significant byte
						sp->track[EOF_TRACK_DRUM]->flags |= (atoi(value_index) << 24);	//Store the pro drum difficulty in the drum track's flag's most significant byte
					}
					else if(!ustricmp(eof_import_ini_setting[i].type, "diff_vocals_harm"))
					{	//If this is a harmony difficulty tag
						sp->track[EOF_TRACK_VOCALS]->flags &= ~(0xFF << 24);	//Clear the vocal track's flag's most significant byte
						sp->track[EOF_TRACK_VOCALS]->flags |= (atoi(value_index) << 24);	//Store the harmony difficulty in the vocal track's flag's most significant byte
					}
					else if(!ustricmp(eof_import_ini_setting[i].type, "diff_band"))
					{	//If this is a band difficulty tag
						sp->tags->difficulty = atoi(value_index);
						if(sp->tags->difficulty > 6)
						{	//If the band difficulty is invalid, set it to undefined
							sp->tags->difficulty = 0xFF;
						}
					}
					else
					{	//Store it as a custom INI setting
						if(sp->tags->ini_settings < EOF_MAX_INI_SETTINGS)
						{	//If the maximum number of INI settings isn't already defined
							snprintf(sp->tags->ini_setting[sp->tags->ini_settings], 512, "%s = %s", eof_import_ini_setting[i].type, value_index);
							sp->tags->ini_settings++;
						}
					}
				}
			}
		}//If the value portion of the entry has content
	}//For each imported INI setting
	eof_log("\tFreeing INI buffer", 1);
	free(textbuffer);	//Free buffered INI file from memory
	return 1;
}
