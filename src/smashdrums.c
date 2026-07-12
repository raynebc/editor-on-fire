#include "beat.h"
#include "dialog.h"
#include "ir.h"
#include "main.h"
#include "silence.h"
#include "song.h"
#include "smashdrums.h"
#include "utility.h"
#include "foflc/Lyric_storage.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

unsigned char eof_text_event_is_smashdrums_phase(char *str)
{
	if(!str)
		return 0;	//Invalid parameter

	if(strcasestr_spec(str, "intro"))
		return 1;
	if(strcasestr_spec(str, "verse"))
		return 2;
	if(strcasestr_spec(str, "prechorus"))
		return 3;;
	if(strcasestr_spec(str, "pre chorus"))
		return 3;;
	if(strcasestr_spec(str, "pre-chorus"))
		return 3;;
	if(strcasestr_spec(str, "chorus"))
		return 4;
	if(strcasestr_spec(str, "bridge"))
		return 5;
	if(strcasestr_spec(str, "solo"))
		return 6;
	if(strcasestr_spec(str, "outro"))
		return 7;

	return 0;		//No match found
}

int eof_remap_smashdrums_note_masks(EOF_SONG *sp, unsigned long track, unsigned long notenum, unsigned char *notemask, unsigned char *ghostmask, unsigned char *accentmask)
{
	unsigned char note, ghost, accent, translated_note = 0, translated_ghost = 0, translated_accent = 0, tom_bitmask = 0, cymbal_bitmask = 0;
	unsigned long flags;

	if(!sp || !track || (track >= sp->tracks) || !notemask || !ghostmask || !accentmask)
		return 0;	//Invalid parameters

	note = eof_get_note_note(sp, track, notenum);
	ghost = eof_get_note_ghost(sp, track, notenum);
	accent = eof_get_note_accent(sp, track, notenum);
	flags = eof_get_note_flags(sp, track, notenum);
	eof_get_drum_note_masks(sp, track, notenum, &tom_bitmask, &cymbal_bitmask);
	tom_bitmask &= 28;	//The gems in this note reflecting a lane 3, 4 or 5 tom
	cymbal_bitmask &= 24;	//The gems in this note reflecting a blue or green cymbal (not yellow cymbal, which exports as hi hat)

	if(note & 1)
	{	//Lane 1
		translated_note |= 1;	//Bass drum
		if(ghost & 1)
			translated_ghost |= 1;	//Ghosted bass drum
		else if(accent & 1)
			translated_accent |= 1;	//Accented bass drum
	}
	if(note & 2)
	{	//Lane 2
		translated_note |= 2;	//Snare drum
		if(ghost & 2)
			translated_ghost |= 2;	//Ghosted snare drum
		else if(accent & 2)
			translated_accent |= 2;	//Accented snare drum
	}
	if(cymbal_bitmask)
	{	//Any cymbal other than hi hat
		translated_note |= 4;	//Cymbal
		if(ghost & cymbal_bitmask)
			translated_ghost |= 4;	//Ghosted cymbal
		else if(accent & cymbal_bitmask)
			translated_accent |= 4;	//Accented cymbal
	}
	if(tom_bitmask)
	{	//Any lane 3, 4 or 5 tom
		translated_note |= 8;	//Tom
		if(ghost & tom_bitmask)
			translated_ghost |= 8;	//Ghosted tom
		else if(accent & tom_bitmask)
			translated_accent |= 8;	//Accented tom
	}
	if((note & 4) && (flags & EOF_DRUM_NOTE_FLAG_Y_CYMBAL))
	{	//Lane 3 cymbal
		translated_note |= 16;	//Hi hat
		if(ghost & 4)
			translated_ghost |= 16;	//Ghosted hi hat
		else if(accent & 4)
			translated_accent |= 16;	//Accented hi hat
	}
	if(note & 32)
	{	//Lane 6 note
		translated_note |= 32;	//Clapfire
		if(ghost & 32)
			translated_ghost |= 32;	//Ghosted clapfire
		else if(accent & 32)
			translated_accent |= 32;	//Accented clapfire
	}

	*notemask = translated_note;
	*ghostmask = translated_ghost;
	*accentmask = translated_accent;
	return 1;	//Return success
}


int eof_export_smashdrums(EOF_SONG *sp, unsigned long track, char *destpath)
{
	char *preview_name = "preview.ogg", *art_name = "cover.png";
	char temp_string[1024], album_art_filename[1024];
	char * buffer = NULL;
	ALOGG_OGG * temp_ogg = NULL;
	SAMPLE *preview = NULL;
	PACKFILE *fp;
	unsigned long ctr, notectr, lanectr, bitmask, phase_count, last_phase_match, phases_written, diffs_written = 0, gems_written;
	unsigned char phase_number, json_diff_mapping[4] = {0xFF, 0xFF, 0xFF, 0xFF}, next_diff, power;
	char *phase_names[8] = {"INVALID", "intro", "verse", "prechorus", "CHORUS", "bridge", "solo", "outro"};
	char *diff_names[4] = {"ChartEasy", "ChartNormal", "ChartHard", "ChartExtreme"};
	double beat_pos;	//The calculated position of an item measured in beats (phases and notes)
	unsigned char note, ghost, accent, first_beat_missing_phase = 0;

	//Validate parameters
	if(!sp || (track >= sp->tracks))
	{
		eof_log("\tError saving:  Invalid parameters", 1);
		return 0;	//Return failure
	}
	if(sp->track[track]->track_behavior != EOF_DRUM_TRACK_BEHAVIOR)
	{	//Only allow drum tracks to export to this format
		eof_log("\tNot a drum track.  Aborting export", 1);
		return 0;	//Return failure
	}
	if(!eof_get_track_size(sp, track))
	{	//Target track has no notes
		eof_log("\tEmpty track.  Aborting export", 1);
		return 0;
	}
	if(sp->beats < 2)
	{	//Project doesn't have at least two beats
		eof_log("\tNot enough beats.  Aborting export", 1);
		return 0;
	}

	eof_log("eof_export_smashdrums() entered", 1);
	(void) replace_filename(eof_temp_filename, destpath, "", sizeof(eof_temp_filename));	//Obtain the destination folder path

	//Determine which track difficulties will export as which JSON difficulties, since the highest defined difficulty must export as expert
	for(ctr = 4, next_diff = 4; ctr > 0; ctr--)
	{	//For each of the usable track difficulties in reverse order
		if(eof_get_track_diff_size(sp, track, ctr - 1))
		{	//If this track has any notes that will export
			json_diff_mapping[next_diff - 1] = ctr - 1;	//Map this as the next highest difficult to export to JSON
			next_diff--;
		}
	}
	if(json_diff_mapping[3] == 0xFF)
	{	//If the track difficulty that would export as ChartExtreme was not found
		eof_log("\tCould not identify the difficulty to export as ChartExtreme.  Aborting export", 1);
		return 0;
	}

	//Write audio.ogg
	if(eof_silence_loaded)
	{
		allegro_message("SmashDrums:  No chart audio is loaded so the export folder will be missing audio.");
		eof_log("No chart audio is loaded so the export folder will be missing audio.", 2);
	}
	else
	{
		(void) replace_filename(eof_temp_filename, eof_temp_filename, "audio.ogg", (int) sizeof(eof_temp_filename));
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tWriting \"%s\"", eof_temp_filename);
		eof_log(eof_log_string, 2);
		if(!eof_conditionally_copy_file(eof_loaded_ogg_name, eof_temp_filename))
		{
			allegro_message("Could not export audio!\n%s", eof_temp_filename);
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Could not export audio!\n%s", eof_temp_filename);
			eof_log(eof_log_string, 2);

			return 0;	//Return failure
		}
	}

	//Write album art
	album_art_filename[0] = '\0';	//Empty this string
	(void) replace_filename(eof_temp_filename, eof_temp_filename, art_name, sizeof(eof_temp_filename));	//Build the path to the album art file to write to the export folder
	if(!exists(eof_temp_filename))
	{	//If cover.png does not exist in the export folder
		eof_log("\tAlbum art not found in export folder, checking project folder.", 2);
		eof_check_for_immerrock_album_art(eof_song_path, album_art_filename, sizeof(album_art_filename) - 1, "png", 1);	//Check for any suitable files in the project folder, preferring PNG format
		if(exists(album_art_filename))
		{	//If a JPG, PNG or TIFF file with a base filename of "Album", "Cover", "Label" or "Image" exist in the project folder
			if(!ustricmp(get_extension(album_art_filename), "png"))
			{	//If the album art file with a .png extension was found in the project folder
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tWriting \"%s\"", eof_temp_filename);
				eof_log(eof_log_string, 2);
				(void) eof_conditionally_copy_file(album_art_filename, eof_temp_filename);	//Copy the album art to the export folder if a file of the same size doesn't already exist
			}
			else
			{	//The album art exists in another format
				if(exists(eof_ffmpeg_executable_path))
				{	//If FFMPEG is linked, use it to convert the image
					(void) eof_ffmpeg_convert_file(album_art_filename, eof_temp_filename);
				}
				else
				{
					allegro_message("Album art was found but is not in the correct format.  Convert it to PNG format or use \"File>Link to>FFMPEG\" to have EOF use FFMPEG to convert it.");
				}
			}
		}
		else
			eof_log("\tAlbum art not found in project folder.", 2);
	}
	else
		eof_log("\tAlbum art already exists in export folder, skipping copy", 2);

	//Write preview.wav if it doesn't exist in the export folder and it has been created for the project
	(void) replace_filename(temp_string, eof_song_path, preview_name, sizeof(temp_string));		//Build the path to the preview.ogg filename in the project folder
	(void) replace_filename(eof_temp_filename, eof_temp_filename, preview_name, (int) sizeof(eof_temp_filename));	//Build the path to the preview.wav filename to write to the export folder
	(void) replace_extension(eof_temp_filename, eof_temp_filename, "wav", (int) sizeof(eof_temp_filename));
	if(exists(temp_string) && !exists(eof_temp_filename))
	{	//If preview.ogg exists in the project folder and preview.wav does not exist in the export folder
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tCreating \"%s\" from \"%s\"", eof_temp_filename, temp_string);
		eof_log(eof_log_string, 2);
		buffer = eof_buffer_file(temp_string, 0, 0);	//Buffer the OGG file to memory
		if(buffer)
		{	//If the OGG file was buffered
			temp_ogg = alogg_create_ogg_from_buffer(buffer, file_size_ex(temp_string));	//Load the OGG file
			if(temp_ogg)
			{	//If the OGG file was loaded
				preview = alogg_create_sample_from_ogg(temp_ogg);		//Load in SAMPLE format
				alogg_destroy_ogg(temp_ogg);

				if(preview)
				{	//If the audio was loaded in SAMPLE format
					if(!save_wav(eof_temp_filename, preview) || !exists(eof_temp_filename))
					{	//If the wave file was not created
						eof_log("!Failed to convert preview audio to WAV", 1);
						allegro_message("Failed to create preview audio.  preview.wav will need to be created in the export folder manually");
					}
					destroy_sample(preview);
				}
			}
			free(buffer);
		}
	}

	//Write chart JSON
	(void) replace_filename(eof_temp_filename, eof_temp_filename, "meta.json", (int) sizeof(eof_temp_filename));
	fp = pack_fopen(eof_temp_filename, "w");
	if(!fp)
	{
		eof_log("\tError saving:  Cannot open meta.json for writing", 1);
		return 0;	//Return failure
	}
	else
	{	//JSON file was opened for writing
		//Write metadata
		(void) pack_fputs("{\n", fp);
		(void) snprintf(temp_string, sizeof(temp_string) - 1, "	\"NameArtist\": \"%s\",\n", eof_check_string(sp->tags->artist) ? sp->tags->artist : "UNDEFINED");
		(void) pack_fputs(temp_string, fp);		//Write artist name
		(void) snprintf(temp_string, sizeof(temp_string) - 1, "	\"NameSong\": \"%s\",\n", eof_check_string(sp->tags->title) ? sp->tags->title : "UNDEFINED");
		(void) pack_fputs(temp_string, fp);		//Write song title
		(void) snprintf(temp_string, sizeof(temp_string) - 1, "	\"NameCharter\": \"%s\",\n", eof_check_string(sp->tags->frettist) ? sp->tags->frettist : "UNDEFINED");
		(void) pack_fputs(temp_string, fp);		//Write chart author
		(void) pack_fputs("	\"FilePath\": \"\",\n", fp);			//Write empty file path (unused)
		(void) pack_fputs("	\"SongOffsetSeconds\": 0.0,\n", fp);	//Write 0 song offset (unused)

		//Write anchors
		(void) pack_fputs("	\"SongTiming\": [\n", fp);
		for(ctr = 0; ctr < sp->beats; ctr++)
		{	//For each beat in the project
			if((ctr + 1 >= sp->beats) || eof_beat_is_anchor(sp, ctr))
			{	//If this is the last beat in the project or if it is an anchor (the first beat is always an anchor)
				(void) pack_fputs("		{\n", fp);
				(void) snprintf(temp_string, sizeof(temp_string) - 1, "			\"beat\": %lu,\n", ctr);
				(void) pack_fputs(temp_string, fp);		//Write beat number
				(void) snprintf(temp_string, sizeof(temp_string) - 1, "			\"timer\": %f\n", sp->beat[ctr]->fpos / 1000.0);
				(void) pack_fputs(temp_string, fp);		//Write beat position in seconds
				if(ctr + 1 < sp->beats)
				{	//If there will be another anchor written (the last beat is always written), place a comma at the end of the anchor definition
					(void) pack_fputs("		},\n", fp);
				}
				else
				{	//The last anchor definition won't have a comma after it because no others will follow
					(void) pack_fputs("		}\n", fp);
				}
			}
		}
		(void) pack_fputs("	],\n", fp);

		//Count song phases
		for(ctr = 0, phase_count = 0, last_phase_match = ULONG_MAX; ctr < sp->text_events; ctr++)
		{	//For each text event in the project
			if(!sp->text_event[ctr]->track || (sp->text_event[ctr]->track == track))
			{	//If the text event has no associated track or is specific to the track being exported
				if(sp->text_event[ctr]->pos != last_phase_match)
				{	//If this text event isn't in the same position as the last one that was found to be a valid phase instance
					if(eof_text_event_is_smashdrums_phase(sp->text_event[ctr]->text))
					{	//If this text event is a usable phase (section) type in Smash Drums
						if(!phase_count && (sp->text_event[ctr]->pos != 0))
						{	//If this is the first applicable section marker, but it is not on the first beat
							first_beat_missing_phase = 1;	//Remember this so an intro phase can be inserted at the first beat
							phase_count++;			//And add that to the count
						}
						phase_count++;	//Keep count
						last_phase_match = sp->text_event[ctr]->pos;
					}
				}
			}
		}

		//Write song phases
		(void) pack_fputs("	\"SongPhases\": [\n", fp);
		phases_written = 0;
		if(first_beat_missing_phase)
		{	//If there was not a phase defined at the first beat, insert one
			(void) pack_fputs("		{\n", fp);
			(void) pack_fputs("			\"beat\": 0.0,\n", fp);
			(void) pack_fputs("			\"phase\": 1,\n", fp);
			(void) pack_fputs("			\"power\": 1,\n", fp);
			(void) pack_fputs("			\"phaseName\": \"Intro\"\n", fp);
			phases_written++;	//This counts as the first phase written to file
			if(phases_written < phase_count)
			{	//If another phase will be written, add a comma after this phase ends
				(void) pack_fputs("		},\n", fp);
			}
			else
			{	//This is the last phase
				(void) pack_fputs("		}\n", fp);
			}
		}

		//Write all manually defined phases
		for(ctr = 0, last_phase_match = ULONG_MAX; ctr < sp->text_events; ctr++)
		{	//For each text event in the project
			if(!sp->text_event[ctr]->track || (sp->text_event[ctr]->track == track))
			{	//If the text event has no associated track or is specific to the track being exported
				if(sp->text_event[ctr]->pos != last_phase_match)
				{	//If this text event isn't in the same position as the last one that was found to be a valid phase instance
					phase_number = eof_text_event_is_smashdrums_phase(sp->text_event[ctr]->text);
					if(phase_number)
					{	//If this text event is a usable phase (section) type in Smash Drums
						power = eof_find_smashdrums_phase_power_definition_at_pos(sp, track, sp->text_event[ctr]->pos, NULL);	//Find the defined power level of this phase, if any
						if(power > 100)
						{	//If no valid power level was found
							power = 100;	//Default at 100%
						}
						phases_written++;	//Keep count
						last_phase_match = sp->text_event[ctr]->pos;
						(void) pack_fputs("		{\n", fp);
						if(sp->text_event[ctr]->flags & EOF_EVENT_FLAG_FLOATING_POS)
						{	//If this text event is defined at an arbitrary timstamp instead of on a beat marker,
							beat_pos = eof_get_beatpos_abs(sp, sp->text_event[ctr]->pos);	//Calculate this event's position measured in beats
							(void) snprintf(temp_string, sizeof(temp_string) - 1, "			\"beat\": %f,\n", beat_pos);
						}
						else
						{	//This text event is defined on a beat marker
							(void) snprintf(temp_string, sizeof(temp_string) - 1, "			\"beat\": %lu.0,\n", sp->text_event[ctr]->pos);
						}
						(void) pack_fputs(temp_string, fp);
						(void) snprintf(temp_string, sizeof(temp_string) - 1, "			\"phase\": %u,\n", phase_number);
						(void) pack_fputs(temp_string, fp);
						(void) snprintf(temp_string, sizeof(temp_string) - 1, "			\"power\": %f,\n", (double)power / 100.0);
						(void) pack_fputs(temp_string, fp);
						(void) snprintf(temp_string, sizeof(temp_string) - 1, "			\"phaseName\": \"%s\"\n", phase_names[phase_number]);
						(void) pack_fputs(temp_string, fp);
						if(phases_written < phase_count)
						{	//If another phase will be written, add a comma after this phase ends
							(void) pack_fputs("		},\n", fp);
						}
						else
						{	//This is the last phase
							(void) pack_fputs("		}\n", fp);
						}
					}
				}//If this text event isn't in the same position as the last one that was found to be a valid phase instance
			}//If the text event has no associated track or is specific to the track being exported
		}//For each text event in the project
		(void) pack_fputs("	],\n", fp);

		//Write notes
		for(ctr = 0; ctr < 4; ctr++)
		{	//For each of the four JSON difficulties
			if(diffs_written)
			{	//If a difficulty definition was written in this JSON already
				(void) pack_fputs(",\n", fp);	//Follow that definition with a comma and a newline to separate it from the following definition
			}
			if(json_diff_mapping[ctr] != 0xFF)
			{	//If this JSON difficulty was found to have notes that will export
				gems_written = 0;	//Reset this count, for tracking when a comma needs to be added between gem definitions
				(void) snprintf(temp_string, sizeof(temp_string) - 1, "	\"%s\": [\n", diff_names[ctr]);
				(void) pack_fputs(temp_string, fp);		//Write start of difficulty
				for(notectr = 0; notectr < eof_get_track_size(sp, track); notectr++)
				{	//For each note in the track being exported
					if(eof_get_note_type(sp, track, notectr) == json_diff_mapping[ctr])
					{	//If this note is in the track difficulty being exported to this JSON difficulty
						beat_pos = eof_get_beatpos_abs(sp, eof_get_note_pos(sp, track, notectr));		//Calculate this note's position measured in beats
						eof_remap_smashdrums_note_masks(sp, track, notectr, &note, &ghost, &accent);	//Determine the Smash Drums bitmasks for this note
						for(lanectr = 0, bitmask = 1; lanectr < 6; lanectr++, bitmask <<= 1)
						{	//For each of the 6 gem types used in Smash Drums
							unsigned strength = 1;	//The default hit strength

							if(note & bitmask)
							{	//If this gem type is used in this note
								if(ghost & bitmask)
									strength = 0;	//The gem is a ghost gem
								else if(accent & bitmask)
									strength = 2;	//The gem is an accent gem

								if(gems_written)
								{	//If a gem definition was written in this JSON difficulty already
									(void) pack_fputs(",\n", fp);	//Follow that definition with a comma and a newline to separate it from the following definition
								}
								(void) pack_fputs("		{\n", fp);
								(void) snprintf(temp_string, sizeof(temp_string) - 1, "			\"Beat\": %f,\n", beat_pos);
								(void) pack_fputs(temp_string, fp);		//Write gem position
								(void) snprintf(temp_string, sizeof(temp_string) - 1, "			\"Strength\": %u,\n", strength);
								(void) pack_fputs(temp_string, fp);		//Write gem strength
								(void) snprintf(temp_string, sizeof(temp_string) - 1, "			\"Id\": %lu\n", lanectr);
								(void) pack_fputs(temp_string, fp);		//Write gem ID (type)
								(void) pack_fputs("		}", fp);
								gems_written++;	//Track how many gems have been exported
							}
						}
					}
				}//For each note in the track being exported
				(void) pack_fputs("\n	]", fp);	//End of note data
				diffs_written++;	//Track how many JSON difficulties have been exported
			}//If this JSON difficulty was found to have notes that will export
			else
			{	//This JSON track difficulty is empty
				(void) snprintf(temp_string, sizeof(temp_string) - 1, "	\"%s\": []", diff_names[ctr]);	//Write an empty difficulty (extreme will always have content, so since this is not the last difficulty being exported, append a comma)
				(void) pack_fputs(temp_string, fp);
				diffs_written++;	//Track how many JSON difficulties have been exported
			}
		}//For each of the four JSON difficulties
		(void) pack_fputs("\n", fp);	//End of track definitions
		(void) pack_fputs("}", fp);	//End of JSON

		pack_fclose(fp);
	}//JSON file was opened for writing

	//Compress the exported files as needed by Smash Drums
	#ifdef ALLEGRO_WINDOWS
		if(eof_check_string(sp->tags->artist) && eof_check_string(sp->tags->title))
		{	//If the artist name and song title are defined
			(void) replace_filename(eof_etext2, eof_temp_filename, "", (int) sizeof(eof_etext2));										//Build the path to the export folder
			eof_etext2[strlen(eof_etext2) - 1] = '\0';																			//Remove the final folder separator from this path
			(void) snprintf(eof_etext, sizeof(eof_etext) - 1, "%s - %s.zip", sp->tags->artist, sp->tags->title);							//Build the relative name for the output zip file
			(void) replace_filename(eof_temp_filename2, eof_temp_filename, eof_etext, (int) sizeof(eof_temp_filename2));				//Build the full path for the output zip file
			(void) snprintf(temp_string, sizeof(temp_string) - 1, "tar -cavf \"%s\" -C \"%s\" meta.json audio.ogg", eof_temp_filename2, eof_etext2);	//Buld the beginning of the tar command, use -C to have it add files with just their relative path instead of recreating the full folder structure

			(void) replace_filename(eof_temp_filename, eof_temp_filename, "cover.png", (int) sizeof(eof_temp_filename));
			if(exists(eof_temp_filename))
			{	//If album art was exported
				eof_strncat(temp_string, " cover.png", sizeof(temp_string));		//Append the album art file name
			}

			(void) replace_filename(eof_temp_filename, eof_temp_filename, "preview.wav", (int) sizeof(eof_temp_filename));
			if(exists(eof_temp_filename))
			{	//If preview audio was exported
				eof_strncat(temp_string, " preview.wav", sizeof(temp_string));	//Append the preview audio file name
			}
		}
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tCalling tar as follows:  %s", temp_string);
		eof_log(eof_log_string, 1);
		(void) eof_system(temp_string);	//Call the command
		(void) replace_extension(eof_temp_filename, eof_temp_filename2, "indies", (int) sizeof(eof_temp_filename));		//Build the path to rename the .zip file to .indies
		(void) rename(eof_temp_filename2, eof_temp_filename);	//Rename the zip file
	#endif

	eof_log("eof_export_smashdrums() completed", 1);
	return 1;		//Return success
}

unsigned char eof_find_smashdrums_phase_power_definition_at_pos(EOF_SONG *sp, unsigned long track, unsigned long pos, unsigned long *eventindex)
{
	unsigned long ctr;
	char *str;
	long power;

	if(!sp || !track || (track > sp->tracks))
		return 0xFF;	//Invalid parameters

	for(ctr = 0; ctr < sp->text_events; ctr++)
	{	//For each text event in the project
		if(!sp->text_event[ctr]->track || (sp->text_event[ctr]->track == track))
		{	//If the text event has no associated track or is specific to the track being examined
			if(sp->text_event[ctr]->pos > pos)
				break;	//If this and all remaining text events are after the target position, stop checking events

			if(sp->text_event[ctr]->pos == pos)
			{	//If the text event is at the specified position
				str = strcasestr_spec(sp->text_event[ctr]->text, "power=");
				if(str)
				{	//If this text event is formatted as expected for a phase power definition
					power = atol(str);	//Read the number expected to occur after the equal sign

					if(!power && !eof_string_is_zero(str))
					{	//If atol() returned zero and the string isn't meant to represent zero, it failed to be parsed
						return 0xFF;	//Return failure
					}
					if(power > 100)
						return 0xFF;	//Invalid power level

					if(eventindex)
					{	//If the calling function wanted the index of the matching event
						*eventindex = ctr;
					}
					return power;
				}
			}
		}
	}

	return 0xFF;	//No power definition found
}
