#include <allegro.h>
#include "main.h"
#include "note.h"
#include "beat.h"
#include "event.h"
#include "ini.h"
#include "ini_import.h"	//For eof_find_ini_setting_tag()
#include "legacy.h"
#include "tuning.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

char *eof_difficulty_ini_tags[EOF_TRACKS_MAX + 1] = {"", "diff_guitar", "diff_bass", "diff_guitar_coop", "diff_rhythm", "diff_drums", "diff_vocals", "diff_keys", "diff_bass_real", "diff_guitar_real", "diff_dance", "diff_bass_real_22", "diff_guitar_real_22", "diff_drums_real_ps", "diff_keys_real"};
int eof_rb_tier_values[7] = {1, 150, 208, 267, 325, 384, 442};	//Rock Band's tier values for difficulties 0 through 6, for use in creating DTA files

int eof_save_ini(EOF_SONG * sp, char * fn, char silent)
{
	PACKFILE * fp;
	char buffer[256] = {0};
	char ini_string[4096] = {0};
	unsigned long i, j, tracknum, index = 0;
	char *tuning_name = NULL;
	char slidesfound = 0;
	char hihatmarkersfound = 0;
	char rimshotmarkersfound = 0;
	int guitar_ghl_diff_written = 0, is_guitar_ghl_diff, is_bass_ghl_diff, guitar_ghl_diff_conflict_warned = 0, bass_ghl_diff_conflict_warned = 0;

	eof_log("eof_save_ini() entered", 1);

	if(!sp || !fn)
	{
		return 0;
	}
	fp = pack_fopen(fn, "w");
	if(!fp)
	{
		return 0;
	}

	/* write artist name */
	if(ustrlen(sp->tags->artist) > 0)
	{
		(void) ustrcpy(ini_string, "[song]\r\nartist = ");
		(void) ustrcat(ini_string, sp->tags->artist);
	}
	else
	{
		(void) ustrcpy(ini_string, "[song]\r\nartist = ");
		(void) ustrcat(ini_string, "Unknown Artist");
	}

	/* write song name */
	if(ustrlen(sp->tags->title) > 0)
	{
		(void) ustrcat(ini_string, "\r\nname = ");
		(void) ustrcat(ini_string, sp->tags->title);
	}
	else
	{
		(void) ustrcat(ini_string, "\r\nname = ");
		(void) ustrcat(ini_string, "Untitled");
	}

	/* write frettist/charter name */
	if(ustrlen(sp->tags->frettist) > 0)
	{
		(void) ustrcat(ini_string, "\r\nfrets = ");
		(void) ustrcat(ini_string, sp->tags->frettist);
		(void) ustrcat(ini_string, "\r\ncharter = ");		//Newer games like Phase Shift and Clone Hero also support this tag
		(void) ustrcat(ini_string, sp->tags->frettist);
	}

	/* write album name */
	if(ustrlen(sp->tags->album) > 0)
	{
		(void) ustrcat(ini_string, "\r\nalbum = ");
		(void) ustrcat(ini_string, sp->tags->album);
	}

	/* write genre */
	if(ustrlen(sp->tags->genre) > 0)
	{
		(void) ustrcat(ini_string, "\r\ngenre = ");
		(void) ustrcat(ini_string, sp->tags->genre);
	}

	/* write track number */
	if(ustrlen(sp->tags->tracknumber) > 0)
	{
		(void) ustrcat(ini_string, "\r\ntrack = ");
		(void) ustrcat(ini_string, sp->tags->tracknumber);
	}

	/* write midi offset */
	(void) ustrcat(ini_string, "\r\ndelay = ");
	(void) snprintf(buffer, sizeof(buffer) - 1, "%ld", sp->tags->ogg[0].midi_offset);
	(void) ustrcat(ini_string, buffer);

	/* write year */
	if(ustrlen(sp->tags->year) > 0)
	{
		(void) ustrcat(ini_string, "\r\nyear = ");
		(void) ustrcat(ini_string, sp->tags->year);
	}

	/* write loading text */
	if(ustrlen(sp->tags->loading_text) > 0)
	{
		(void) ustrcat(ini_string, "\r\nloading_phrase = ");
		(void) ustrcat(ini_string, sp->tags->loading_text);
	}

	/* write difficulty tags */
	for(i = 0; i < sp->tracks; i++)
	{	//For each track in the chart
		char *diff_tag;

		if(i == 0)
			continue;	//Until the band difficulty is implemented, skip this iteration
		if(eof_difficulty_ini_tags[i][0] == '\0')
			continue;	//If this track does not have a defined difficulty tag

		is_guitar_ghl_diff = is_bass_ghl_diff = 0;	//Reset these statuses
		diff_tag = eof_difficulty_ini_tags[i];	//By default, write a non GHL specific difficulty tag
		if(eof_track_is_ghl_mode(sp, i))
		{	//If this track has GHL mode enabled
			if(sp->track[i]->altname[0] != '\0')
			{	//If this GHL track is manually named, that name supersedes its native name
				if(!ustricmp(sp->track[i]->altname, "PART GUITAR GHL"))
				{	//Manually named to represent the GHL guitar track
					if(guitar_ghl_diff_written)
					{	//If a GHL mode guitar difficulty tag was already written
						continue;	//Skip writing a tag for this track instead of writing a conflicting difficulty tag
					}
					diff_tag = "diff_guitarghl";	//Write the guitar GHL difficulty tag
					guitar_ghl_diff_written = 1;
					is_guitar_ghl_diff = 1;
				}
				else if(!ustricmp(sp->track[i]->altname, "PART BASS GHL"))
				{	//Manually named to represent the GHL bass track
					diff_tag = "diff_bassghl";	//Write the bass GHL difficulty tag
					is_bass_ghl_diff = 1;
				}
			}
			else
			{	//Otherwise base the difficulty tags on the track's native name
				if(i == EOF_TRACK_BASS)
				{	//If this is the bass track
					diff_tag = "diff_bassghl";	//Write the bass GHL difficulty tag
					is_bass_ghl_diff = 1;
				}
				else
				{	//This is a guitar track
					if(guitar_ghl_diff_written)
					{	//If a GHL mode guitar difficulty tag was already written
						continue;	//Skip writing a tag for this track instead of writing a conflicting difficulty tag
					}
					diff_tag = "diff_guitarghl";	//Write the guitar GHL difficulty tag
					guitar_ghl_diff_written = 1;
					is_guitar_ghl_diff = 1;
				}
			}
		}

		//Check whether the track's defined difficulty is nullified by a user defined INI entry
		if(is_guitar_ghl_diff)
		{	//If the INI setting that is to be written is a GHL guitar track's difficulty
			if(eof_find_ini_setting_tag(sp, &index, "diff_guitarghl"))
			{	//If the user manually defined the GHL guitar difficulty
				if(!guitar_ghl_diff_conflict_warned && !silent)
				{	//If the user wasn't warned about this already and warnings aren't being suppressed
					allegro_message("Warning:  A GHL mode guitar track defines a difficulty that is overridden by a manually defined INI entry.");
					guitar_ghl_diff_conflict_warned = 1;
				}
				continue;	//Skip this INI setting
			}
		}
		if(is_bass_ghl_diff)
		{	//If the INI setting that is to be written is a GHL bass  track's difficulty
			if(eof_find_ini_setting_tag(sp, &index, "diff_bassghl"))
			{	//If the user manually defined the GHL bass difficulty
				if(!bass_ghl_diff_conflict_warned && !silent)
				{	//If the user wasn't warned about this already and warnings aren't being suppressed
					allegro_message("Warning:  A GHL mode bass track defines a difficulty that is overridden by a manually defined INI entry.");
					bass_ghl_diff_conflict_warned = 1;
				}
				continue;	//Skip this INI setting
			}
		}

		if(!eof_get_track_size_normal(sp, i))
		{	//If this track is empty
			(void) snprintf(buffer, sizeof(buffer) - 1, "\r\n%s = -1", diff_tag);	//Write an "empty track" difficulty tag
			(void) ustrcat(ini_string, buffer);
		}
		else
		{
			if(sp->track[i]->difficulty != 0xFF)
			{	//If the track's difficulty is defined
				(void) snprintf(buffer, sizeof(buffer) - 1, "\r\n%s = %u", diff_tag, sp->track[i]->difficulty);
				(void) ustrcat(ini_string, buffer);
			}
		}
	}
	if(((sp->track[EOF_TRACK_DRUM]->flags & 0x0F000000) >> 24) != 0x0F)
	{	//If there is a defined pro drum difficulty
		(void) snprintf(buffer, sizeof(buffer) - 1, "\r\ndiff_drums_real = %lu", (sp->track[EOF_TRACK_DRUM]->flags & 0x0F000000) >> 24);
		(void) ustrcat(ini_string, buffer);
	}
	else if(!eof_track_has_cymbals(sp, EOF_TRACK_DRUM))
	{	//Otherwise if there are also no cymbals defined
		(void) ustrcat(ini_string, "\r\ndiff_drums_real = -1");	//Write a pro drum not present tag
	}
	if(((sp->track[EOF_TRACK_VOCALS]->flags & 0x0F000000) >> 24) != 0x0F)
	{	//If there is a defined harmony difficulty
		(void) snprintf(buffer, sizeof(buffer) - 1, "\r\ndiff_vocals_harm = %lu", (sp->track[EOF_TRACK_VOCALS]->flags & 0x0F000000) >> 24);
		(void) ustrcat(ini_string, buffer);
	}
	else
	{
		char harmoniesfound = 0;
		struct eof_MIDI_data_track *trackptr;

		for(trackptr = sp->midi_data_head; trackptr != NULL; trackptr = trackptr->next)
		{	//For each raw MIDI track
			if(trackptr->trackname && (ustrstr(trackptr->trackname,"HARM") == trackptr->trackname))
			{	//If this track has a name, and it begins with the substring "HARM"
				harmoniesfound = 1;
				break;
			}
		}
		if(!harmoniesfound)
		{	//If none of the stored MIDI tracks appeared to have harmonies
			(void) ustrcat(ini_string, "\r\ndiff_vocals_harm = -1");	//Write a vocal harmonies not present tag
		}
	}
	if(sp->tags->difficulty != 0xFF)
	{	//If there is a defined band difficulty
		(void) snprintf(buffer, sizeof(buffer) - 1, "\r\ndiff_band = %lu", sp->tags->difficulty);
		(void) ustrcat(ini_string, buffer);
	}

	/* write other settings */
	if(sp->tags->lyrics)
	{
		(void) ustrcat(ini_string, "\r\nlyrics = True");
	}
	if(sp->tags->eighth_note_hopo)
	{
		(void) ustrcat(ini_string, "\r\neighthnote_hopo = 1");
	}

	/* check for use of open bass strumming and write a tag if necessary */
	for(i = 1; i < sp->tracks; i++)
	{	//For each track in the chart (skipping track 0)
		if(eof_open_strum_enabled(i))
		{	//If open strumming was enabled for this track during the time of the save
			tracknum = sp->track[i]->tracknum;
			for(j = 0; j < sp->legacy_track[tracknum]->notes; j++)
			{	//For each note in the bass guitar track
				if(sp->legacy_track[tracknum]->note[j]->note & 32)
				{	//If lane 6 (open bass) is populated
					(void) ustrcat(ini_string, "\r\nsysex_open_bass = True");	//Write the open strum Sysex presence tag (used in Phase Shift) to identify that this Sysex phrase will be written to MIDI
					i = sp->tracks;	//Trigger the exit of the outer loop
					break;	//Exit loops
				}
			}
		}
	}

	/* check for use of pro guitar slides and write a tag if necessary */
	for(i = 1; i < sp->tracks; i++)
	{	//For each track in the chart (skipping track 0)
		if(sp->track[i]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
			continue;	//If this isn't a pro guitar/bass track

		tracknum = sp->track[i]->tracknum;
		for(j = 0; j < sp->pro_guitar_track[tracknum]->notes; j++)
		{	//For each note in the pro guitar/bass track
			if((sp->pro_guitar_track[tracknum]->note[j]->flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP) || (sp->pro_guitar_track[tracknum]->note[j]->flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN))
			{	//If this note slides up or down
				slidesfound = 1;
				break;
			}
		}
	}
	if(slidesfound)
	{	//If this chart has at least one pro guitar slide
		(void) ustrcat(ini_string, "\r\nsysex_pro_slide = True");	//Write the pro guitar slide Sysex presence tag (used in Phase Shift) to identify that up/down slide Sysex phrases will be written to MIDI
	}

	/* check for use of open or pedal controlled hi hat or rim shot and write tags if necessary */
	tracknum = sp->track[EOF_TRACK_DRUM_PS]->tracknum;
	for(i = 0; i < sp->legacy_track[tracknum]->notes; i++)
	{	//For each note in the drum track
		if((sp->legacy_track[tracknum]->note[i]->flags & EOF_DRUM_NOTE_FLAG_Y_HI_HAT_OPEN) || (sp->legacy_track[tracknum]->note[i]->flags & EOF_DRUM_NOTE_FLAG_Y_HI_HAT_PEDAL) || (sp->legacy_track[tracknum]->note[i]->flags & EOF_DRUM_NOTE_FLAG_Y_SIZZLE))
		{	//If the note is marked for open, pedal controlled or sizzle hi hat
			hihatmarkersfound = 1;
		}
		if(sp->legacy_track[tracknum]->note[i]->flags & EOF_DRUM_NOTE_FLAG_R_RIMSHOT)
		{	//If the note is marked for rim shot
			rimshotmarkersfound = 1;
		}
	}
	if(hihatmarkersfound)
	{	//If this chart has at least one yellow cymbal marked as open, pedal controlled or sizzle hi hat
		(void) ustrcat(ini_string, "\r\nsysex_high_hat_ctrl = True");	//Write the high hat control Sysex presence tag (used in Phase Shift) to identify that hi hat Sysex phrases will be written to MIDI
	}
	if(rimshotmarkersfound)
	{	//If this chart has at least one note marked as a rim shot
		(void) ustrcat(ini_string, "\r\nsysex_rimshot = True");	//Write the rim shot Sysex presence tag (used in Phase Shift) to identify that rim shot Sysex phrases will be written to MIDI
	}

	/* check for use of slider sections and write a tag if necessary */
	for(i = 1; i < sp->tracks; i++)
	{	//For each track in the chart (skipping track 0)
		if(eof_get_num_sliders(sp, i))
		{	//If the track has a slider section
			(void) ustrcat(ini_string, "\r\nsysex_slider = True");	//Write the slider Sysex presence tag (used in Phase Shift) to identify that slider Sysex phrases will be written to MIDI
			break;
		}
	}

	(void) ustrcat(ini_string, "\r\nmultiplier_note = 116");	//Write this tag to indicate to Phase Shift that EOF is using Rock Band's notation for star power

	/* check for use of cymbal notation */
	if(eof_force_pro_drum_midi_notation || (eof_track_has_cymbals(sp, EOF_TRACK_DRUM) || eof_track_has_cymbals(sp, EOF_TRACK_DRUM_PS)))
	{	//If the use of pro drum notation is being forced by user preference or either drum track has any notes marked as cymbals
		(void) ustrcat(ini_string, "\r\npro_drums = True");	//Write the pro drum tag, which indicates notes default as cymbals unless marked as toms (used in Phase Shift)
	}

	/* check for five lane drums */
	if(eof_five_lane_drums_enabled())
	{	//If the drum track's fifth lane is enabled
		(void) ustrcat(ini_string, "\r\nfive_lane_drums = True");	//Write the five lane drum tag, which allows the game to decide how to present the chart during playback (used in Phase Shift)
	}

	/* check for accented drums */
	if(eof_track_has_accent(sp, EOF_TRACK_DRUM) || eof_track_has_accent(sp, EOF_TRACK_DRUM_PS))
	{
		(void) ustrcat(ini_string, "\r\neof_midi_import_drum_accent_velocity = 127");	//Write this tag to indicate to Phase Shift that EOF is using velocity 127 to define accented drum notes
	}

	/* check for ghosted drums */
	if(eof_track_has_ghost(sp, EOF_TRACK_DRUM) || eof_track_has_ghost(sp, EOF_TRACK_DRUM_PS))
	{
		(void) ustrcat(ini_string, "\r\neof_midi_import_drum_ghost_velocity = 1");		//Write this tag to indicate that EOF is using velocity 1 to define ghosted drum notes
	}

	/* write tuning tags */
	if(eof_get_track_size_normal(sp, EOF_TRACK_PRO_GUITAR))
	{	//If the 17 fret pro guitar track is populated
		(void) ustrcat(ini_string, "\r\n");
		tracknum = sp->track[EOF_TRACK_PRO_GUITAR]->tracknum;
		(void) ustrcat(ini_string, "real_guitar_tuning =");	//Write the pro guitar tuning tag
		for(i = 0; i < sp->pro_guitar_track[tracknum]->numstrings; i++)
		{	//For each string used in the track
			(void) snprintf(buffer, sizeof(buffer) - 1, " %d", sp->pro_guitar_track[tracknum]->tuning[i] % 12);	//Write the string's tuning value (signed integer), disregarding which octave the pitch is in since tunings of more than 11 steps are only allowed for RS2
			(void) ustrcat(ini_string, buffer);	//Append the string's tuning value to the ongoing INI string
		}
		tuning_name = eof_lookup_tuning_name(sp, EOF_TRACK_PRO_GUITAR, sp->pro_guitar_track[tracknum]->tuning);	//Check to see if this track's tuning has a defined name
		if(tuning_name != eof_tuning_unknown)
		{	//If the lookup found a name
			(void) snprintf(buffer, sizeof(buffer) - 1, " \"%s\"", tuning_name);
			(void) ustrcat(ini_string, buffer);
		}
	}
	if(eof_get_track_size_normal(sp, EOF_TRACK_PRO_GUITAR_22))
	{	//If the 22 fret pro guitar track is populated
		(void) ustrcat(ini_string, "\r\n");
		tracknum = sp->track[EOF_TRACK_PRO_GUITAR_22]->tracknum;
		(void) ustrcat(ini_string, "real_guitar_22_tuning =");	//Write the pro guitar tuning tag
		for(i = 0; i < sp->pro_guitar_track[tracknum]->numstrings; i++)
		{	//For each string used in the track
			(void) snprintf(buffer, sizeof(buffer) - 1, " %d", sp->pro_guitar_track[tracknum]->tuning[i] % 12);	//Write the string's tuning value (signed integer), disregarding which octave the pitch is in since tunings of more than 11 steps are only allowed for RS2
			(void) ustrcat(ini_string, buffer);	//Append the string's tuning value to the ongoing INI string
		}
		tuning_name = eof_lookup_tuning_name(sp, EOF_TRACK_PRO_GUITAR_22, sp->pro_guitar_track[tracknum]->tuning);	//Check to see if this track's tuning has a defined name
		if(tuning_name != eof_tuning_unknown)
		{	//If the lookup found a name
			(void) snprintf(buffer, sizeof(buffer) - 1, " \"%s\"", tuning_name);
			(void) ustrcat(ini_string, buffer);
		}
	}
	if(eof_get_track_size_normal(sp, EOF_TRACK_PRO_BASS))
	{	//If the 17 fret pro bass track is populated
		(void) ustrcat(ini_string, "\r\n");
		tracknum = sp->track[EOF_TRACK_PRO_BASS]->tracknum;
		(void) ustrcat(ini_string, "real_bass_tuning =");	//Write the pro bass tuning tag
		for(i = 0; i < sp->pro_guitar_track[tracknum]->numstrings; i++)
		{	//For each string used in the track
			(void) snprintf(buffer, sizeof(buffer) - 1, " %d", sp->pro_guitar_track[tracknum]->tuning[i] % 12);	//Write the string's tuning value (signed integer), disregarding which octave the pitch is in since tunings of more than 11 steps are only allowed for RS2
			(void) ustrcat(ini_string, buffer);	//Append the string's tuning value to the ongoing INI string
		}
		tuning_name = eof_lookup_tuning_name(sp, EOF_TRACK_PRO_BASS, sp->pro_guitar_track[tracknum]->tuning);	//Check to see if this track's tuning has a defined name
		if(tuning_name != eof_tuning_unknown)
		{	//If the lookup found a name
			(void) snprintf(buffer, sizeof(buffer) - 1, " \"%s\"", tuning_name);
			(void) ustrcat(ini_string, buffer);
		}
	}
	if(eof_get_track_size_normal(sp, EOF_TRACK_PRO_BASS_22))
	{	//If the 22 fret pro bass track is populated
		(void) ustrcat(ini_string, "\r\n");
		tracknum = sp->track[EOF_TRACK_PRO_BASS_22]->tracknum;
		(void) ustrcat(ini_string, "real_bass_22_tuning =");	//Write the pro bass tuning tag
		for(i = 0; i < sp->pro_guitar_track[tracknum]->numstrings; i++)
		{	//For each string used in the track
			(void) snprintf(buffer, sizeof(buffer) - 1, " %d", sp->pro_guitar_track[tracknum]->tuning[i] % 12);	//Write the string's tuning value (signed integer), disregarding which octave the pitch is in since tunings of more than 11 steps are only allowed for RS2
			(void) ustrcat(ini_string, buffer);	//Append the string's tuning value to the ongoing INI string
		}
		tuning_name = eof_lookup_tuning_name(sp, EOF_TRACK_PRO_BASS_22, sp->pro_guitar_track[tracknum]->tuning);	//Check to see if this track's tuning has a defined name
		if(tuning_name != eof_tuning_unknown)
		{	//If the lookup found a name
			(void) snprintf(buffer, sizeof(buffer) - 1, " \"%s\"", tuning_name);
			(void) ustrcat(ini_string, buffer);
		}
	}

	/* write custom INI settings */
	(void) ustrcat(ini_string, "\r\n");
	for(i = 0; i < sp->tags->ini_settings; i++)
	{
		if(ustrlen(sp->tags->ini_setting[i]) > 0)
		{
			(void) ustrcat(ini_string, sp->tags->ini_setting[i]);
			(void) ustrcat(ini_string, "\r\n");
		}
	}

	/* write song length */
	(void) snprintf(buffer, sizeof(buffer) - 1, "song_length = %lu", eof_music_length);
	(void) ustrcat(ini_string, buffer);	//Append the song length tag to the ongoing INI string
	(void) ustrcat(ini_string, "\r\n");

	(void) pack_fwrite(ini_string, ustrsize(ini_string), fp);
	(void) pack_fclose(fp);
	return 1;
}

int eof_save_upgrades_dta(EOF_SONG * sp, char * fn)
{
	char buffer[256] = {0};
	char buffer2[256] = {0};
	char buffer3[5] = {0};
	unsigned long i, tracknum;
	int tiervalue;
	PACKFILE * fp;

	eof_log("eof_save_upgrades_dta() entered", 1);

	if(!sp || !fn)
	{
		return 0;
	}
	fp = pack_fopen(fn, "r");	//Try to open the file for reading
	if(fp)
	{	//If the file exists already
		(void) pack_fclose(fp);
		return 0;
	}
	fp = pack_fopen(fn, "w");	//Try to open the file for writing
	if(!fp)
	{
		return 0;
	}

	if(ustrlen(sp->tags->title) > 0)
	{	//If there is a defined song title
		(void) snprintf(buffer, sizeof(buffer) - 1, "(%s\n", sp->tags->title);
		(void) pack_fputs(buffer, fp);
	}
	else
	{	//Use a placeholder
		(void) pack_fputs("(SONGNAME\n", fp);
	}
	(void) pack_fputs("   (upgrade_version 1)\n", fp);

	if(ustrlen(sp->tags->title) > 0)
	{	//If there is a defined song title
		(void) snprintf(buffer, sizeof(buffer) - 1, "   (midi_file \"songs_upgrades/%s_plus.mid\")\n", sp->tags->title);
		(void) pack_fputs(buffer, fp);
	}
	else
	{	//Use a placeholder
		(void) pack_fputs("   (midi_file \"songs_upgrades/SONGNAME_plus.mid\")\n", fp);
	}
	(void) pack_fputs("   (song_id ###)\n", fp);
	(void) pack_fputs("   (rank\n", fp);

	if(eof_get_track_size_normal(sp, EOF_TRACK_PRO_GUITAR))
	{	//If the pro guitar track is populated
		if(sp->track[EOF_TRACK_PRO_GUITAR]->difficulty != 0xFF)
		{	//If the track's difficulty is defined
			if(sp->track[EOF_TRACK_PRO_GUITAR]->difficulty < 7)
			{	//If the track's difficulty is valid
				tiervalue = eof_rb_tier_values[sp->track[EOF_TRACK_PRO_GUITAR]->difficulty];
			}
			else
			{	//Otherwise default to the lowest difficulty tier
				tiervalue = 1;
			}
			(void) snprintf(buffer, sizeof(buffer) - 1, "      (real_guitar %d)\n", tiervalue);
			(void) pack_fputs(buffer, fp);
		}
		else
		{
			(void) pack_fputs("      (real_guitar ###)\n", fp);
		}
	}
	else
	{	//Write data for a blank pro guitar track
		(void) pack_fputs("      (real_guitar 0)\n", fp);
	}

	if(eof_get_track_size_normal(sp, EOF_TRACK_PRO_BASS))
	{	//If the pro bass track is populated
		if(sp->track[EOF_TRACK_PRO_BASS]->difficulty != 0xFF)
		{	//If the track's difficulty is defined
			if(sp->track[EOF_TRACK_PRO_BASS]->difficulty < 7)
			{	//If the track's difficulty is valid
				tiervalue = eof_rb_tier_values[sp->track[EOF_TRACK_PRO_BASS]->difficulty];
			}
			else
			{	//Otherwise default to the lowest difficulty tier
				tiervalue = 1;
			}
			(void) snprintf(buffer, sizeof(buffer) - 1, "      (real_bass %d)\n", tiervalue);
			(void) pack_fputs(buffer, fp);
		}
		else
		{
			(void) pack_fputs("      (real_bass ###)\n", fp);
		}
	}
	else
	{	//Write data for a blank pro bass track
		(void) pack_fputs("      (real_bass 0)\n", fp);
	}
	(void) pack_fputs("   )\n", fp);

	tracknum = sp->track[EOF_TRACK_PRO_GUITAR]->tracknum;
	buffer2[0] = '\0';	//Ensure this string is empty
	for(i = 0; i < sp->pro_guitar_track[tracknum]->numstrings; i++)
	{	//For each string used in the track
		if(i != 0)
		{	//If this isn't the first string, append a space after the last tuning that was written
			(void) ustrcat(buffer2, " ");
		}
		(void) snprintf(buffer3, sizeof(buffer3) - 1, "%d", sp->pro_guitar_track[tracknum]->tuning[i] % 12);	//Write the string's tuning value (signed integer), disregarding which octave the pitch is in since tunings of more than 11 steps are only allowed for RS2
		(void) ustrcat(buffer2, buffer3);	//Append the string's tuning value to the ongoing tuning string
	}
	(void) snprintf(buffer, sizeof(buffer) - 1, "   (real_guitar_tuning (%s))\n", buffer2);	//Build the complete pro guitar tuning string line
	(void) pack_fputs(buffer, fp);

	tracknum = sp->track[EOF_TRACK_PRO_BASS]->tracknum;
	buffer2[0] = '\0';	//Ensure this string is empty
	for(i = 0; i < sp->pro_guitar_track[tracknum]->numstrings; i++)
	{	//For each string used in the track
		if(i != 0)
		{	//If this isn't the first string, append a space after the last tuning that was written
			(void) ustrcat(buffer2, " ");
		}
		(void) snprintf(buffer3, sizeof(buffer3) - 1, "%d", sp->pro_guitar_track[tracknum]->tuning[i] % 12);	//Write the string's tuning value (signed integer), disregarding which octave the pitch is in since tunings of more than 11 steps are only allowed for RS2
		(void) ustrcat(buffer2, buffer3);	//Append the string's tuning value to the ongoing tuning string
	}
	(void) snprintf(buffer, sizeof(buffer) - 1, "   (real_bass_tuning (%s))\n", buffer2);	//Build the complete pro bass tuning string line
	(void) pack_fputs(buffer, fp);

	(void) pack_fputs(")", fp);
	(void) pack_fclose(fp);
	return 1;
}
