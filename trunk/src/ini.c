#include <allegro.h>
#include "main.h"
#include "note.h"
#include "beat.h"
#include "event.h"
#include "ini.h"
#include "legacy.h"

int eof_save_ini(EOF_SONG * sp, char * fn)
{
	PACKFILE * fp;
	char buffer[256] = {0};
	char ini_string[4096] = {0};
	unsigned long i, tracknum;

	fp = pack_fopen(fn, "w");
	if(!fp)
	{
		return 0;
	}

	/* write artist name */
	if(ustrlen(sp->tags->artist) > 0)
	{
		ustrcpy(ini_string, "[song]\r\nartist = ");
		ustrcat(ini_string, sp->tags->artist);
	}
	else
	{
		ustrcpy(ini_string, "[song]\r\nartist = ");
		ustrcat(ini_string, "Unknown Artist");
	}

	/* write song name */
	if(ustrlen(sp->tags->title) > 0)
	{
		ustrcat(ini_string, "\r\nname = ");
		ustrcat(ini_string, sp->tags->title);
	}
	else
	{
		ustrcat(ini_string, "\r\nname = ");
		ustrcat(ini_string, "Untitled");
	}

	/* write frettist name */
	if(ustrlen(sp->tags->frettist) > 0)
	{
		ustrcat(ini_string, "\r\nfrets = ");
		ustrcat(ini_string, sp->tags->frettist);
	}

	/* write midi offset */
	ustrcat(ini_string, "\r\ndelay = ");
	sprintf(buffer, "%d", sp->tags->ogg[eof_selected_ogg].midi_offset);
	ustrcat(ini_string, buffer);

	/* write year */
	if(ustrlen(sp->tags->year) > 0)
	{
		ustrcat(ini_string, "\r\nyear = ");
		ustrcat(ini_string, sp->tags->year);
	}

	/* write loading text */
	if(ustrlen(sp->tags->loading_text) > 0)
	{
		ustrcat(ini_string, "\r\nloading_phrase = ");
		ustrcat(ini_string, sp->tags->loading_text);
	}

	/* write other settings */
	if(sp->tags->lyrics)
	{
		ustrcat(ini_string, "\r\nlyrics = True");
	}
	if(sp->tags->eighth_note_hopo)
	{
		ustrcat(ini_string, "\r\neighthnote_hopo = 1");
	}

	/* check for use of open bass strumming and write a tag if necessary */
	if(eof_open_bass)
	{	//If open bass was enabled during the time of the save
		tracknum = sp->track[EOF_TRACK_BASS]->tracknum;
		for(i = 0; i < sp->legacy_track[tracknum]->notes; i++)
		{	//For each note in the bass guitar track
			if(sp->legacy_track[tracknum]->note[i]->note & 32)
			{	//If lane 6 (open bass) is populated
				ustrcat(ini_string, "\r\nopen_strum = True");	//Write the open strum tag (used in Phase Shift)
				break;	//Exit loop
			}
		}
	}

	/* write custom INI settings */
	ustrcat(ini_string, "\r\n");
	for(i = 0; i < sp->tags->ini_settings; i++)
	{
		if(ustrlen(sp->tags->ini_setting[i]) > 0)
		{
			ustrcat(ini_string, sp->tags->ini_setting[i]);
			ustrcat(ini_string, "\r\n");
		}
	}

	pack_fwrite(ini_string, ustrlen(ini_string), fp);
	pack_fclose(fp);
	return 1;
}
