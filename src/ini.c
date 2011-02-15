#include <allegro.h>
#include "main.h"
#include "note.h"
#include "beat.h"
#include "event.h"
#include "ini.h"
#include "legacy.h"

char *eof_difficulty_ini_tags[EOF_TRACKS_MAX + 1] = {"", "diff_guitar", "diff_bass", "", "", "diff_drums", "diff_vocals", "diff_keys", "diff_bass_real", "diff_guitar_real", "diff_keys_real"};

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
	sprintf(buffer, "%ld", sp->tags->ogg[eof_selected_ogg].midi_offset);
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

	/* write difficulty tags */
	for(i = 0; i < sp->tracks; i++)
	{	//For each track in the chart
		if(i == 0)
			continue;	//Until the band difficulty is implemented, skip this iteration

		if((eof_difficulty_ini_tags[i][0] != '\0') && (sp->track[i]->difficulty != 0xFF))
		{	//If this track has a supported difficulty tag and the difficulty is defined
			sprintf(buffer, "\r\n%s = %d", eof_difficulty_ini_tags[i], sp->track[i]->difficulty);
			ustrcat(ini_string, buffer);
		}
	}
	if(((eof_song->track[EOF_TRACK_DRUM]->flags & 0xFF000000) >> 24) != 0xFF)
	{	//If there is a defined pro drum difficulty
		sprintf(buffer, "\r\ndiff_drums_real = %lu", (eof_song->track[EOF_TRACK_DRUM]->flags & 0xFF000000) >> 24);
		ustrcat(ini_string, buffer);
	}
	if(((eof_song->track[EOF_TRACK_VOCALS]->flags & 0xFF000000) >> 24) != 0xFF)
	{	//If there is a defined harmony difficulty
		sprintf(buffer, "\r\ndiff_vocals_harm = %lu", (eof_song->track[EOF_TRACK_VOCALS]->flags & 0xFF000000) >> 24);
		ustrcat(ini_string, buffer);
	}
	if(eof_song->tags->difficulty != 0xFF)
	{	//If there is a defined band difficulty
		sprintf(buffer, "\r\ndiff_band = %lu", eof_song->tags->difficulty);
		ustrcat(ini_string, buffer);
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
	if(eof_open_bass_enabled())
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
