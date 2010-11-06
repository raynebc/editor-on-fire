#include <allegro.h>
#include <stdio.h>
#include "song.h"
#include "utility.h"	//For eof_buffer_file()

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
	char * textbuffer = NULL;
	char * line_token = NULL;
	int textlength = 0;
	char * token;
	char * equals = NULL;
	int i;
	int j;

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
					eof_import_ini_settings++;
				}
			}
		}
	}
	for(i = 0; i < eof_import_ini_settings; i++)
	{
		if(!ustricmp(eof_import_ini_setting[i].type, "artist ") || !ustricmp(eof_import_ini_setting[i].type, "artist"))
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
		else if(!ustricmp(eof_import_ini_setting[i].type, "name ") || !ustricmp(eof_import_ini_setting[i].type, "name"))
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
		else if(!ustricmp(eof_import_ini_setting[i].type, "frets ") || !ustricmp(eof_import_ini_setting[i].type, "frets"))
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
		else if(!ustricmp(eof_import_ini_setting[i].type, "year ") || !ustricmp(eof_import_ini_setting[i].type, "year"))
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
		else if(!ustricmp(eof_import_ini_setting[i].type, "loading_phrase ") || !ustricmp(eof_import_ini_setting[i].type, "loading_phrase"))
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
		else if(!ustricmp(eof_import_ini_setting[i].type, "lyrics ") || !ustricmp(eof_import_ini_setting[i].type, "lyrics"))
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
		else if(!ustricmp(eof_import_ini_setting[i].type, "eighthnote_hopo ") || !ustricmp(eof_import_ini_setting[i].type, "eighthnote_hopo"))
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
		else if(!ustricmp(eof_import_ini_setting[i].type, "delay ") || !ustricmp(eof_import_ini_setting[i].type, "delay"))
		{
			sp->tags->ogg[0].midi_offset = atoi(eof_import_ini_setting[i].value);
			if(sp->tags->ogg[0].midi_offset < 0)
			{
				sp->tags->ogg[0].midi_offset = 0;
			}
		}
		else if(!ustricmp(eof_import_ini_setting[i].type, "scores ") || !ustricmp(eof_import_ini_setting[i].type, "score"))
		{
		}
		else if(!ustricmp(eof_import_ini_setting[i].type, "scores_ext ") || !ustricmp(eof_import_ini_setting[i].type, "scores_ext"))
		{
		}

		/* for custom settings */
		else
		{
			snprintf(sp->tags->ini_setting[sp->tags->ini_settings], 512, "%s = %s", eof_import_ini_setting[i].type, eof_import_ini_setting[i].value);
			sp->tags->ini_settings++;
		}
	}
	free(textbuffer);	//Free buffered INI file from memory
	return 1;
}
