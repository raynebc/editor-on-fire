#include <allegro.h>
#include <stdio.h>
#include "song.h"
#include "utility.h"	//For eof_buffer_file()
#include "ini.h"		//For eof_difficulty_ini_tags[]
#include "main.h"		//For logging

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

	if(!sp || !fn)
	{
		return 0;
	}
	eof_import_ini_settings = 0;
	textbuffer = (char *)eof_buffer_file(fn);	//Buffer the INI file into memory
	if(!textbuffer)
	{
		return 0;
	}
	textlength = ustrlen(textbuffer);
	ustrtok(textbuffer, "\r\n");
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
					ustrcpy(eof_import_ini_setting[eof_import_ini_settings].type, line_token);
					ustrcpy(eof_import_ini_setting[eof_import_ini_settings].value, token);
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
	for(i = 0; i < eof_import_ini_settings; i++)
	{	//For each imported INI setting
		if(eof_import_ini_setting[i].type == '\0')
			continue;	//Skip this INI type if its string is empty

		if(!ustricmp(eof_import_ini_setting[i].type, "artist"))
		{
			for(j = 0; j < ustrlen(eof_import_ini_setting[i].value); j++)
			{
				if(eof_import_ini_setting[i].value[j] != ' ')
				{
					ustrcpy(sp->tags->artist, &(eof_import_ini_setting[i].value[j]));
					break;
				}
			}
		}
		else if(!ustricmp(eof_import_ini_setting[i].type, "name"))
		{
			for(j = 0; j < ustrlen(eof_import_ini_setting[i].value); j++)
			{
				if(eof_import_ini_setting[i].value[j] != ' ')
				{
					ustrcpy(sp->tags->title, &(eof_import_ini_setting[i].value[j]));
					break;
				}
			}
		}
		else if(!ustricmp(eof_import_ini_setting[i].type, "frets"))
		{
			for(j = 0; j < ustrlen(eof_import_ini_setting[i].value); j++)
			{
				if(eof_import_ini_setting[i].value[j] != ' ')
				{
					ustrcpy(sp->tags->frettist, &(eof_import_ini_setting[i].value[j]));
					break;
				}
			}
		}
		else if(!ustricmp(eof_import_ini_setting[i].type, "year"))
		{
			for(j = 0; j < ustrlen(eof_import_ini_setting[i].value); j++)
			{
				if(eof_import_ini_setting[i].value[j] != ' ')
				{
					ustrcpy(sp->tags->year, &(eof_import_ini_setting[i].value[j]));
					break;
				}
			}
		}
		else if(!ustricmp(eof_import_ini_setting[i].type, "loading_phrase"))
		{
			for(j = 0; j < ustrlen(eof_import_ini_setting[i].value); j++)
			{
				if(eof_import_ini_setting[i].value[j] != ' ')
				{
					ustrcpy(sp->tags->loading_text, &(eof_import_ini_setting[i].value[j]));
					break;
				}
			}
		}
		else if(!ustricmp(eof_import_ini_setting[i].type, "lyrics"))
		{
			for(j = 0; j < ustrlen(eof_import_ini_setting[i].value); j++)
			{
				if(eof_import_ini_setting[i].value[j] != ' ')
				{
					if(!ustricmp(&(eof_import_ini_setting[i].value[j]), "True"))
					{
						sp->tags->lyrics = 1;
					}
					break;
				}
			}
		}
		else if(!ustricmp(eof_import_ini_setting[i].type, "eighthnote_hopo"))
		{
			for(j = 0; j < ustrlen(eof_import_ini_setting[i].value); j++)
			{
				if(eof_import_ini_setting[i].value[j] != ' ')
				{
					if(!ustricmp(&(eof_import_ini_setting[i].value[j]), "1"))
					{
						sp->tags->eighth_note_hopo = 1;
					}
					break;
				}
			}
		}
		else if(!ustricmp(eof_import_ini_setting[i].type, "delay"))
		{
			sp->tags->ogg[0].midi_offset = atoi(eof_import_ini_setting[i].value);
			if(sp->tags->ogg[0].midi_offset < 0)
			{
				sp->tags->ogg[0].midi_offset = 0;
			}
		}
		else if(!ustricmp(eof_import_ini_setting[i].type, "score"))
		{
		}
		else if(!ustricmp(eof_import_ini_setting[i].type, "scores_ext"))
		{
		}
		else if(!ustricmp(eof_import_ini_setting[i].type, "open_strum"))
		{
			for(j = 0; j < ustrlen(eof_import_ini_setting[i].value); j++)
			{
				if(eof_import_ini_setting[i].value[j] != ' ')
				{
					if(!ustricmp(&(eof_import_ini_setting[i].value[j]), "True"))
					{
						sp->track[EOF_TRACK_BASS]->flags |= EOF_TRACK_FLAG_OPEN_STRUM;	//Set the open bass strum flag
					}
					break;
				}
			}
		}
		else if(!ustricmp(eof_import_ini_setting[i].type, "real_guitar_tuning"))
		{
			ctr = 0;	//Reset counter
			tracknum = sp->track[EOF_TRACK_PRO_GUITAR]->tracknum;
			line_token = ustrtok(eof_import_ini_setting[i].value, " \r\n");	//Find first token (string of characters that isn't whitespace, carriage return or newline)
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
		}
		else if(!ustricmp(eof_import_ini_setting[i].type, "real_bass_tuning"))
		{
			ctr = 0;	//Reset counter
			tracknum = sp->track[EOF_TRACK_PRO_BASS]->tracknum;
			line_token = ustrtok(eof_import_ini_setting[i].value, " \r\n");	//Find first token (string of characters that isn't whitespace, carriage return or newline)
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
		}

		/* for custom settings or difficulty strings */
		else
		{
			setting_stored = 0;
			for(j = 0; j < EOF_TRACKS_MAX + 1; j++)
			{	//For each string in the eof_difficulty_ini_tags[] array
				if(eof_difficulty_ini_tags[j] && !ustricmp(eof_import_ini_setting[i].type, eof_difficulty_ini_tags[j]))
				{	//If this INI setting matches the difficulty tag, store the difficulty value into the appropriate track structure
					sp->track[j]->difficulty = atoi(eof_import_ini_setting[i].value);
					setting_stored = 1;
					break;
				}
			}
			if(!setting_stored)
			{
				if(!ustricmp(eof_import_ini_setting[i].type, "diff_drums_real"))
				{	//If this is a pro drum difficulty tag
					sp->track[EOF_TRACK_DRUM]->flags &= ~(0xFF << 24);		//Clear the drum track's flag's most significant byte
					sp->track[EOF_TRACK_DRUM]->flags |= (atoi(eof_import_ini_setting[i].value) << 24);	//Store the pro drum difficulty in the drum track's flag's most significant byte
				}
				else if(!ustricmp(eof_import_ini_setting[i].type, "diff_vocals_harm"))
				{	//If this is a harmony difficulty tag
					sp->track[EOF_TRACK_VOCALS]->flags &= ~(0xFF << 24);	//Clear the vocal track's flag's most significant byte
					sp->track[EOF_TRACK_VOCALS]->flags |= (atoi(eof_import_ini_setting[i].value) << 24);	//Store the harmony difficulty in the vocal track's flag's most significant byte
				}
				else if(!ustricmp(eof_import_ini_setting[i].type, "diff_band"))
				{	//If this is a band difficulty tag
					sp->tags->difficulty = atoi(eof_import_ini_setting[i].value);
					if(sp->tags->difficulty > 6)
					{	//If the band difficulty is invalid, set it to undefined
						sp->tags->difficulty = 0xFF;
					}
				}
				else
				{	//Store it as a custom INI setting
					snprintf(sp->tags->ini_setting[sp->tags->ini_settings], 512, "%s = %s", eof_import_ini_setting[i].type, eof_import_ini_setting[i].value);
					sp->tags->ini_settings++;
				}
			}
		}
	}//For each imported INI setting
	free(textbuffer);	//Free buffered INI file from memory
	return 1;
}
