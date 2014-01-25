#include <allegro.h>
#include "main.h"
#include "menu/edit.h"
#include "menu/track.h"	//For the tech view enable/disable functions
#include "dialog.h"
#include "editor.h"
#include "rs.h"
#include "undo.h"
#include "utility.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

char * eof_undo_filename[EOF_MAX_UNDO] = {0};
int eof_undo_type[EOF_MAX_UNDO] = {0};
int eof_undo_last_type = 0;
int eof_undo_current_index = 0;
int eof_undo_count = 0;
int eof_redo_count = 0;
int eof_redo_type = 0;
int eof_undo_states_initialized = 0;

int eof_undo_load_state(const char * fn)
{
	EOF_SONG * sp = NULL;
	PACKFILE * fp = NULL;
	char rheader[16] = {0};
	int old_eof_silence_loaded = eof_silence_loaded;	//Retain this value, since it is destroyed by eof_destroy_song()

 	eof_log("eof_undo_load_state() entered", 1);

	if(fn == NULL)
	{
		return 0;
	}
	fp = pack_fopen(fn, "r");
	if(!fp)
	{
		return 0;
	}
	if(pack_fread(rheader, 16, fp) != 16)
	{
		return 0;	//Return error if 16 bytes cannot be read
	}
	sp = eof_create_song();		//Initialize an empty chart
	if(sp == NULL)
	{
		return 0;
	}
	if(!eof_load_song_pf(sp, fp))
	{	//If loading the undo state fails
		allegro_message("Failed to perform undo");
		return 0;	//Return failure
	}
	if(eof_song)
	{
		eof_destroy_song(eof_song);	//Destroy the chart that is open
	}
	eof_song = sp;	//Replacing it with the loaded undo state
	eof_silence_loaded = old_eof_silence_loaded;	//Restore the status of whether chart audio is loaded

	(void) pack_fclose(fp);
	return 1;
}

void eof_undo_reset(void)
{
 	eof_log("eof_undo_reset() entered", 1);

	eof_undo_current_index = 0;
	eof_undo_count = 0;
	eof_redo_count = 0;
}

int eof_undo_add(int type)
{
	char fn[1024] = {0}, temp[1024] = {0};
	unsigned long ctr;

 	eof_log("eof_undo_add() entered", 1);

	if(eof_undo_states_initialized == -1)
	{	//The undo filename array couldn't be initialized previously
		return 0;
	}

	if(!eof_undo_states_initialized)
	{	//Initialize the undo filename array
		for(ctr = 0; ctr < EOF_MAX_UNDO; ctr++)
		{	//For each undo slot
			(void) snprintf(fn, sizeof(fn) - 1, "eof%03u-%03lu.undo", eof_log_id, ctr);	//Build the undo filename in the format of "eof#-#.undo", where the first number is the EOF ID
			eof_undo_filename[ctr] = malloc(sizeof(fn)+1);
			if(eof_undo_filename[ctr] == NULL)
			{
				allegro_message("Error initializing undo system.  Undo disabled");
				eof_undo_states_initialized = -1;
				return 0;
			}
			strcpy(eof_undo_filename[ctr], fn);	//Save the undo filename to the array
		}
		eof_undo_states_initialized = 1;
	}

	if((type == EOF_UNDO_TYPE_NOTE_LENGTH) && (eof_undo_last_type == EOF_UNDO_TYPE_NOTE_LENGTH))
	{
		return 0;
	}
	if((type == EOF_UNDO_TYPE_LYRIC_NOTE) && (eof_undo_last_type == EOF_UNDO_TYPE_LYRIC_NOTE))
	{
		return 0;
	}
	if((type == EOF_UNDO_TYPE_RECORD) && (eof_undo_last_type == EOF_UNDO_TYPE_RECORD))
	{
		return 0;
	}
	if((type == EOF_UNDO_TYPE_TEMPO_ADJUST) && (eof_undo_last_type == EOF_UNDO_TYPE_TEMPO_ADJUST))
	{
		return 0;
	}
	if(type == EOF_UNDO_TYPE_SILENCE)
	{
		(void) snprintf(fn, sizeof(fn) - 1, "%s.ogg", eof_undo_filename[eof_undo_current_index]);
		(void) eof_copy_file(eof_loaded_ogg_name, fn);
	}
	eof_undo_last_type = type;
	(void) eof_save_song(eof_song, eof_undo_filename[eof_undo_current_index]);
	eof_undo_type[eof_undo_current_index] = type;
	if(eof_recovery)
	{	//If this EOF instance is maintaining auto-recovery files
		PACKFILE *fp = pack_fopen("eof.recover", "w");	//Open the recovery definition file for writing
		if(fp)
		{	//If the file opened
			(void) append_filename(temp, eof_song_path, eof_loaded_song_name, 1024);	//Construct the full path to the project file
			(void) pack_fputs(eof_undo_filename[eof_undo_current_index], fp);			//Write the undo file path
			(void) pack_fputs("\n", fp);												//Write a newline character
			(void) pack_fputs(temp, fp);												//Write the project path
			(void) pack_fclose(fp);
		}
	}
	eof_undo_current_index++;
	if(eof_undo_current_index >= EOF_MAX_UNDO)
	{
		eof_undo_current_index = 0;
	}
	eof_undo_count++;
	if(eof_undo_count >= EOF_MAX_UNDO)
	{
		eof_undo_count = EOF_MAX_UNDO;
	}
	eof_redo_count = 0;
	return 1;
}

int eof_undo_apply(void)
{
	char fn[1024] = {0};
	char title[256] = {0};
	unsigned long ctr;
	EOF_PRO_GUITAR_TRACK *tp = NULL;
	char tech_view_status[EOF_PRO_GUITAR_TRACKS_MAX] = {0};	//Tracks whether or not tech view was in effect for each of the pro guitar tracks, so this view's status can be restored after the undo

 	eof_log("eof_undo_apply() entered", 1);

	if(eof_undo_count > 0)
	{
		//Determine whether each pro guitar track was in tech view
		for(ctr = 0; ctr < EOF_PRO_GUITAR_TRACKS_MAX; ctr++)
		{	//For each pro guitar track in the project
			tp = eof_song->pro_guitar_track[ctr];
			if(tp->note == tp->technote)
			{	//If tech view was in effect for this track
				tech_view_status[ctr] = 1;
			}
		}

		strncpy(title, eof_song->tags->title, sizeof(title) - 1);	//Backup the song title field, since if it changes as part of the undo, the Rocksmith WAV file should be deleted

		(void) snprintf(fn, sizeof(fn) - 1, "eof%03u.redo", eof_log_id);	//Include EOF's log ID in the redo name to almost guarantee it is uniquely named
		(void) eof_save_song(eof_song, fn);
		eof_redo_type = 0;
		eof_undo_current_index--;
		if(eof_undo_current_index < 0)
		{
			eof_undo_current_index = EOF_MAX_UNDO - 1;
		}
		(void) eof_undo_load_state(eof_undo_filename[eof_undo_current_index]);
		if(eof_undo_type[eof_undo_current_index] == EOF_UNDO_TYPE_NOTE_SEL)
		{
			(void) eof_menu_edit_deselect_all();
		}
		if(eof_undo_type[eof_undo_current_index] == EOF_UNDO_TYPE_SILENCE)
		{
			(void) snprintf(fn, sizeof(fn) - 1, "eof%03u.redo.ogg", eof_log_id);	//Include EOF's log ID in the redo name to almost guarantee it is uniquely named
			(void) eof_copy_file(eof_loaded_ogg_name, fn);
			(void) snprintf(fn, sizeof(fn) - 1, "%s.ogg", eof_undo_filename[eof_undo_current_index]);
			(void) eof_copy_file(fn, eof_loaded_ogg_name);
			(void) eof_load_ogg(eof_loaded_ogg_name, 0);
			eof_delete_rocksmith_wav();		//Delete the Rocksmith WAV file since changing silence will require a new WAV file to be written
			eof_fix_waveform_graph();
			eof_fix_spectrogram();
			eof_redo_type = EOF_UNDO_TYPE_SILENCE;
		}
		if(strcmp(title, eof_song->tags->title))
		{	//If the song title changed as part of the undo, delete the Rocksmith WAV file, since changing the title will cause a new WAV file to be written
			eof_delete_rocksmith_wav();
		}
		eof_undo_count--;
		eof_redo_count = 1;
		eof_change_count--;
		if(eof_change_count == 0)
		{
			eof_changes = 0;
		}
		else
		{
			eof_changes = 1;
		}
		eof_undo_last_type = 0;

		eof_init_after_load(1);	//Perform various cleanup
		(void) eof_detect_difficulties(eof_song, eof_selected_track);
		eof_select_beat(eof_selected_beat);
		eof_fix_catalog_selection();
		eof_fix_window_title();
		eof_scale_fretboard(0);	//Recalculate the 2D screen positioning based on the current track

		//Restore tech view for each pro guitar track that had it in use before the undo operation
		for(ctr = 0; ctr < EOF_PRO_GUITAR_TRACKS_MAX; ctr++)
		{	//For each pro guitar track in the project
			tp = eof_song->pro_guitar_track[ctr];
			if(tech_view_status[ctr])
			{	//If tech view was in effect for this track
				eof_menu_track_enable_tech_view(tp);
			}
		}

		return 1;
	}
	return 0;
}

void eof_redo_apply(void)
{
	char fn[1024] = {0};
	char title[256] = {0};
	unsigned long ctr;
	EOF_PRO_GUITAR_TRACK *tp = NULL;
	char tech_view_status[EOF_PRO_GUITAR_TRACKS_MAX] = {0};	//Tracks whether or not tech view was in effect for each of the pro guitar tracks, so this view's status can be restored after the redo

 	eof_log("eof_redo_apply() entered", 1);

	if(eof_redo_count > 0)
	{
		//Determine whether each pro guitar track was in tech view
		for(ctr = 0; ctr < EOF_PRO_GUITAR_TRACKS_MAX; ctr++)
		{	//For each pro guitar track in the project
			tp = eof_song->pro_guitar_track[ctr];
			if(tp->note == tp->technote)
			{	//If tech view was in effect for this track
				tech_view_status[ctr] = 1;
			}
		}

		strncpy(title, eof_song->tags->title, sizeof(title) - 1);	//Backup the song title field, since if it changes as part of the redo, the Rocksmith WAV file should be deleted

		(void) eof_save_song(eof_song, eof_undo_filename[eof_undo_current_index]);
		eof_undo_current_index++;
		if(eof_undo_current_index >= EOF_MAX_UNDO)
		{
			eof_undo_current_index = 0;
		}
		(void) snprintf(fn, sizeof(fn) - 1, "eof%03u.redo", eof_log_id);	//Get the name of this EOF instance's redo file
		(void) eof_undo_load_state(fn);	//And load it
		if(eof_redo_type == EOF_UNDO_TYPE_SILENCE)
		{
			(void) snprintf(fn, sizeof(fn) - 1, "eof%03u.redo.ogg", eof_log_id);	//Get the name of this EOF instance's redo OGG
			(void) eof_copy_file(fn, eof_loaded_ogg_name);	//And save the current audio to that filename
			(void) eof_load_ogg(eof_loaded_ogg_name, 0);
			eof_delete_rocksmith_wav();		//Delete the Rocksmith WAV file since changing silence will require a new WAV file to be written
			eof_fix_waveform_graph();
			eof_fix_spectrogram();
		}
		if(strcmp(title, eof_song->tags->title))
		{	//If the song title changed as part of the redo, delete the Rocksmith WAV file, since changing the title will cause a new WAV file to be written
			eof_delete_rocksmith_wav();
		}
		eof_undo_count++;
		if(eof_undo_count >= EOF_MAX_UNDO)
		{
			eof_undo_count = EOF_MAX_UNDO;
		}
		eof_redo_count = 0;
		eof_change_count++;
		if(eof_change_count == 0)
		{
			eof_changes = 0;
		}
		else
		{
			eof_changes = 1;
		}

		eof_init_after_load(1);	//Perform various cleanup
		(void) eof_detect_difficulties(eof_song, eof_selected_track);
		eof_select_beat(eof_selected_beat);
		eof_fix_catalog_selection();
		eof_fix_window_title();
		eof_scale_fretboard(0);	//Recalculate the 2D screen positioning based on the current track

		//Restore tech view for each pro guitar track that had it in use before the redo operation
		for(ctr = 0; ctr < EOF_PRO_GUITAR_TRACKS_MAX; ctr++)
		{	//For each pro guitar track in the project
			tp = eof_song->pro_guitar_track[ctr];
			if(tech_view_status[ctr])
			{	//If tech view was in effect for this track
				eof_menu_track_enable_tech_view(tp);
			}
		}
	}
}

void eof_destroy_undo(void)
{
	unsigned long ctr;

	if(eof_undo_states_initialized > 0)
	{
		for(ctr = 0; ctr < EOF_MAX_UNDO; ctr++)
		{	//For each undo slot
			if(eof_undo_filename[ctr] != NULL)
			{
				free(eof_undo_filename[ctr]);
				eof_undo_filename[ctr] = NULL;
			}
		}
	}
}
