#include <allegro.h>
#include "main.h"
#include "menu/edit.h"
#include "dialog.h"
#include "undo.h"
#include "editor.h"
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

	pack_fclose(fp);
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
			snprintf(fn, sizeof(fn), "eof%03u-%03lu.undo", eof_log_id, ctr);	//Build the undo filename in the format of "eof#-#.undo", where the first number is the EOF ID
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
		sprintf(fn, "%s.ogg", eof_undo_filename[eof_undo_current_index]);
		eof_copy_file(eof_loaded_ogg_name, fn);
	}
	eof_undo_last_type = type;
	eof_save_song(eof_song, eof_undo_filename[eof_undo_current_index]);
	eof_undo_type[eof_undo_current_index] = type;
	if(eof_recovery)
	{	//If this EOF instance is maintaining auto-recovery files
		PACKFILE *fp = pack_fopen("eof.recover", "w");	//Open the recovery definition file for writing
		if(fp)
		{	//If the file opened
			append_filename(temp, eof_song_path, eof_loaded_song_name, 1024);	//Construct the full path to the project file
			pack_fputs(eof_undo_filename[eof_undo_current_index], fp);			//Write the undo file path
			pack_fputs("\n", fp);												//Write a newline character
			pack_fputs(temp, fp);												//Write the project path
			pack_fclose(fp);
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

 	eof_log("eof_undo_apply() entered", 1);

	if(eof_undo_count > 0)
	{
		snprintf(fn, sizeof(fn), "eof%03u.redo", eof_log_id);	//Include EOF's log ID in the redo name to almost guarantee it is uniquely named
		eof_save_song(eof_song, fn);
		eof_redo_type = 0;
		eof_undo_current_index--;
		if(eof_undo_current_index < 0)
		{
			eof_undo_current_index = EOF_MAX_UNDO - 1;
		}
		eof_undo_load_state(eof_undo_filename[eof_undo_current_index]);
		if(eof_undo_type[eof_undo_current_index] == EOF_UNDO_TYPE_NOTE_SEL)
		{
			eof_menu_edit_deselect_all();
		}
		if(eof_undo_type[eof_undo_current_index] == EOF_UNDO_TYPE_SILENCE)
		{
			snprintf(fn, sizeof(fn), "eof%03u.redo.ogg", eof_log_id);	//Include EOF's log ID in the redo name to almost guarantee it is uniquely named
			eof_copy_file(eof_loaded_ogg_name, fn);
			sprintf(fn, "%s.ogg", eof_undo_filename[eof_undo_current_index]);
			eof_copy_file(fn, eof_loaded_ogg_name);
			eof_load_ogg(eof_loaded_ogg_name, 0);
			eof_fix_waveform_graph();
			eof_redo_type = EOF_UNDO_TYPE_SILENCE;
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
		eof_detect_difficulties(eof_song);
		eof_select_beat(eof_selected_beat);
		eof_fix_catalog_selection();
		eof_fix_window_title();
		eof_scale_fretboard(0);	//Recalculate the 2D screen positioning based on the current track
		return 1;
	}
	return 0;
}

void eof_redo_apply(void)
{
	char fn[1024] = {0};

 	eof_log("eof_redo_apply() entered", 1);

	if(eof_redo_count > 0)
	{
		eof_save_song(eof_song, eof_undo_filename[eof_undo_current_index]);
		eof_undo_current_index++;
		if(eof_undo_current_index >= EOF_MAX_UNDO)
		{
			eof_undo_current_index = 0;
		}
		snprintf(fn, sizeof(fn), "eof%03u.redo", eof_log_id);	//Get the name of this EOF instance's redo file
		eof_undo_load_state(fn);	//And load it
		if(eof_redo_type == EOF_UNDO_TYPE_SILENCE)
		{
			snprintf(fn, sizeof(fn), "eof%03u.redo.ogg", eof_log_id);	//Get the name of this EOF instance's redo OGG
			eof_copy_file(fn, eof_loaded_ogg_name);	//And save the current audio to that filename
			eof_load_ogg(eof_loaded_ogg_name, 0);
			eof_fix_waveform_graph();
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
		eof_detect_difficulties(eof_song);
		eof_select_beat(eof_selected_beat);
		eof_fix_catalog_selection();
		eof_fix_window_title();
		eof_scale_fretboard(0);	//Recalculate the 2D screen positioning based on the current track
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
