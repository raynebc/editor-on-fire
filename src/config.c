#include <allegro.h>
#include "main.h"
#include "config.h"
#include "mix.h"

static void set_default_config(void);	//Applies the default settings, and overwrites settings from a loaded configuration where applicable

void set_default_config(void)
{
	eof_guitar.button[0].type = EOF_CONTROLLER_BUTTON_TYPE_KEY;
	eof_guitar.button[0].key = KEY_ENTER;
	eof_guitar.button[1].type = EOF_CONTROLLER_BUTTON_TYPE_KEY;
	eof_guitar.button[1].key = KEY_BACKSLASH;
	eof_guitar.button[2].type = EOF_CONTROLLER_BUTTON_TYPE_KEY;
	eof_guitar.button[2].key = KEY_F1;
	eof_guitar.button[3].type = EOF_CONTROLLER_BUTTON_TYPE_KEY;
	eof_guitar.button[3].key = KEY_F2;
	eof_guitar.button[4].type = EOF_CONTROLLER_BUTTON_TYPE_KEY;
	eof_guitar.button[4].key = KEY_F3;
	eof_guitar.button[5].type = EOF_CONTROLLER_BUTTON_TYPE_KEY;
	eof_guitar.button[5].key = KEY_F4;
	eof_guitar.button[6].type = EOF_CONTROLLER_BUTTON_TYPE_KEY;
	eof_guitar.button[6].key = KEY_F5;
	eof_controller_read_button_names(&eof_guitar);
	eof_guitar.delay = 0;

	eof_drums.button[0].type = EOF_CONTROLLER_BUTTON_TYPE_KEY;
	eof_drums.button[0].key = KEY_1;
	eof_drums.button[1].type = EOF_CONTROLLER_BUTTON_TYPE_KEY;
	eof_drums.button[1].key = KEY_2;
	eof_drums.button[2].type = EOF_CONTROLLER_BUTTON_TYPE_KEY;
	eof_drums.button[2].key = KEY_3;
	eof_drums.button[3].type = EOF_CONTROLLER_BUTTON_TYPE_KEY;
	eof_drums.button[3].key = KEY_4;
	eof_drums.button[4].type = EOF_CONTROLLER_BUTTON_TYPE_KEY;
	eof_drums.button[4].key = KEY_5;
	eof_controller_read_button_names(&eof_drums);
	eof_drums.delay = 0;
}

void eof_load_config(char * fn)
{
	set_default_config();

	set_config_file(fn);

	/* read configuration */
	eof_av_delay = get_config_int("config", "av_delay", 250);
	eof_buffer_size = get_config_int("config", "buffer_size", 4096);
	eof_smooth_pos = get_config_int("config", "smooth_playback", 1);
	eof_disable_windows = get_config_int("config", "disable_windows_fs", 0);
	ncdfs_use_allegro = eof_disable_windows;
	eof_disable_vsync = get_config_int("config", "disable_vsync", 0);
	eof_cpu_saver = get_config_int("config", "cpu_saver", 0);;
	if((eof_cpu_saver < 0) || (eof_cpu_saver > 10))
	{
		eof_cpu_saver = 0;
	}
	eof_supports_mp3 = get_config_int("config", "mp3_support", 0);
	eof_supports_silence = get_config_int("config", "silence_support", 0);
	eof_audio_fine_tune = get_config_int("config", "fine_tune", 0);

	/* read preferences */
	eof_inverted_notes = get_config_int("preferences", "invert_notes", 0);
	eof_lefty_mode = get_config_int("preferences", "lefty", 0);
	eof_input_mode = get_config_int("preferences", "input_mode", EOF_INPUT_PIANO_ROLL);
	eof_ogg_setting = get_config_int("preferences", "ogg_quality", 1);

	/* read display settings */
	eof_screen_layout.mode = get_config_int("display", "display_mode", 0);
	if((eof_screen_layout.mode < 0) || (eof_screen_layout.mode > 2))
	{
		eof_screen_layout.mode = 0;
	}
	eof_note_auto_adjust = get_config_int("display", "note_auto_adjust", 1);
	eof_soft_cursor = get_config_int("display", "software_mouse", 0);
	eof_desktop = get_config_int("display", "true_color", 1);

	/* read paths */
	ustrcpy(eof_fof_executable_path, get_config_string("paths", "fof_path", ""));
	ustrcpy(eof_fof_executable_name, get_filename(eof_fof_executable_path));
	ustrcpy(eof_fof_songs_path, get_config_string("paths", "fof_songs_path", ""));
	ustrcpy(eof_songs_path, get_config_string("paths", "songs_path", ""));
	ustrcpy(eof_last_eof_path, get_config_string("paths", "eof_path", ""));
	ustrcpy(eof_last_ogg_path, get_config_string("paths", "ogg_path", ""));

	/* read editor settings */
	ustrcpy(eof_last_frettist, get_config_string("editor", "frettist", ""));
	eof_snap_mode = get_config_int("editor", "snap_mode", 0);
	if((eof_snap_mode < 0) || (eof_snap_mode > EOF_SNAP_CUSTOM))
	{
		eof_snap_mode = 0;
	}
	eof_snap_interval = get_config_int("editor", "snap_interval", 1);
	if((eof_snap_interval < 1) || (eof_snap_interval > EOF_MAX_GRID_SNAP_INTERVALS))
	{
		eof_snap_interval = 1;
	}
	eof_custom_snap_measure = get_config_int("editor", "snap_measure", 0);
	eof_zoom_3d = get_config_int("editor", "preview_speed", 5);
	if((eof_zoom_3d < 2) || (eof_zoom_3d > 5))
	{
		eof_zoom_3d = 5;
	}
	eof_hopo_view = get_config_int("editor", "hopo_view", EOF_HOPO_RF);
	if((eof_hopo_view < 0) || (eof_hopo_view > 2))
	{
		eof_hopo_view = EOF_HOPO_RF;
	}

	if(exists(fn))
	{	//Only try to load the controller buttons if the config file exists, otherwise the defaults will be erased
		eof_controller_load_config(&eof_guitar, "guitar");
		eof_controller_read_button_names(&eof_guitar);
		eof_controller_load_config(&eof_drums, "drums");
		eof_controller_read_button_names(&eof_drums);
	}
}

void eof_save_config(char * fn)
{
	set_config_file(fn);

	/* write configuration */
	set_config_int("config", "av_delay", eof_av_delay);
	set_config_int("config", "buffer_size", eof_buffer_size);
	set_config_int("config", "smooth_playback", eof_smooth_pos);
	set_config_int("config", "disable_windows_fs", eof_disable_windows);
	set_config_int("config", "disable_vsync", eof_disable_vsync);
	set_config_int("config", "cpu_saver", eof_cpu_saver);;
	set_config_int("config", "mp3_support", eof_supports_mp3);
	set_config_int("config", "silence_support", eof_supports_silence);
	set_config_int("config", "fine_tune", eof_audio_fine_tune);

	/* write preferences */
	set_config_int("preferences", "invert_notes", eof_inverted_notes);
	set_config_int("preferences", "lefty", eof_lefty_mode);
	set_config_int("preferences", "input_mode", eof_input_mode);
	set_config_int("preferences", "ogg_quality", eof_ogg_setting);

	/* write display settings */
	set_config_int("display", "display_mode", eof_screen_layout.mode);
	set_config_int("display", "note_auto_adjust", eof_note_auto_adjust);
	set_config_int("display", "software_mouse", eof_soft_cursor);
	set_config_int("display", "true_color", eof_desktop);

	/* write paths */
	set_config_string("paths", "fof_path", eof_fof_executable_path);
	set_config_string("paths", "fof_songs_path", eof_fof_songs_path);
	set_config_string("paths", "songs_path", eof_songs_path);
	set_config_string("paths", "eof_path", eof_last_eof_path);
	set_config_string("paths", "ogg_path", eof_last_ogg_path);

	/* read editor settings */
	set_config_string("editor", "frettist", eof_last_frettist);
	set_config_int("editor", "snap_mode", eof_snap_mode);
	set_config_int("editor", "snap_interval", eof_snap_interval);
	set_config_int("editor", "snap_measure", eof_custom_snap_measure);
	set_config_int("editor", "preview_speed", eof_zoom_3d);
	set_config_int("editor", "hopo_view", eof_hopo_view);

	eof_controller_save_config(&eof_guitar, "guitar");
	eof_controller_save_config(&eof_drums, "drums");
}

