#include <allegro.h>
#include <math.h>
#include "config.h"
#include "main.h"
#include "mix.h"
#include "menu/song.h"	//For eof_set_percussion_cue()

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

static void set_default_config(void);	//Applies the default controller settings, and overwrites settings from a loaded configuration where applicable

void set_default_config(void)
{
	eof_log("set_default_config() entered", 1);

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
	eof_log("eof_load_config() entered", 1);

	if(!fn)
	{
		return;
	}
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
	eof_supports_oggcat = get_config_int("config", "oggcat_support", 0);
	eof_audio_fine_tune = get_config_int("config", "fine_tune", 0);
	eof_global_volume = get_config_int("config", "volume", 255);
	if(eof_global_volume < 0 || eof_global_volume > 255)
	{
		eof_global_volume = 255;
	}

	/* read preferences */
	eof_inverted_notes = get_config_int("preferences", "invert_notes", 0);
	eof_lefty_mode = get_config_int("preferences", "lefty", 0);
	eof_input_mode = get_config_int("preferences", "input_mode", EOF_INPUT_PIANO_ROLL);
	eof_ogg_setting = get_config_int("preferences", "ogg_quality", 1);
	eof_use_ts = get_config_int("preferences", "use_ts", 0);
	if(!eof_use_ts)
	{
		eof_log("\t! TS import/export is currently disabled", 1);
	}
	eof_zoom = get_config_int("preferences", "eof_zoom", 10);
	eof_selected_track = get_config_int("preferences", "eof_selected_track", EOF_TRACK_GUITAR);
	eof_note_type = get_config_int("preferences", "eof_note_type", EOF_NOTE_AMAZING);
	eof_hide_drum_tails = get_config_int("preferences", "hide_drum_tails", 0);
	eof_hide_note_names = get_config_int("preferences", "eof_hide_note_names", 0);
	eof_disable_sound_processing = get_config_int("preferences", "eof_disable_sound_processing", 0);
	eof_disable_3d_rendering = get_config_int("preferences", "eof_disable_3d_rendering", 0);
	eof_disable_2d_rendering = get_config_int("preferences", "eof_disable_2d_rendering", 0);
	eof_disable_info_panel = get_config_int("preferences", "eof_disable_info_panel", 0);
	eof_chart_volume = get_config_int("preferences", "eof_chart_volume", 100);
	if((eof_chart_volume < 0) || (eof_chart_volume > 100))
	{	//Correct the value if it is out of bounds
		eof_chart_volume = 100;
	}
	eof_chart_volume_multiplier = sqrt(eof_chart_volume/100.0);	//Store this math so it only needs to be performed once
	eof_clap_volume = get_config_int("preferences", "eof_clap_volume", 100);
	if((eof_clap_volume < 0) || (eof_clap_volume > 100))
	{	//Correct the value if it is out of bounds
		eof_clap_volume = 100;
	}
	eof_tick_volume = get_config_int("preferences", "eof_tick_volume", 100);
	if((eof_tick_volume < 0) || (eof_tick_volume > 100))
	{	//Correct the value if it is out of bounds
		eof_tick_volume = 100;
	}
	eof_tone_volume = get_config_int("preferences", "eof_tone_volume", 100);
	if((eof_tone_volume < 0) || (eof_tone_volume > 100))
	{	//Correct the value if it is out of bounds
		eof_tone_volume = 100;
	}
	eof_percussion_volume = get_config_int("preferences", "eof_percussion_volume", 100);
	if((eof_percussion_volume < 0) || (eof_percussion_volume > 100))
	{	//Correct the value if it is out of bounds
		eof_percussion_volume = 100;
	}
	eof_selected_percussion_cue = get_config_int("preferences", "eof_selected_percussion_cue", 17);
	eof_set_percussion_cue(eof_selected_percussion_cue);
	eof_paste_erase_overlap = get_config_int("preferences", "eof_paste_erase_overlap", 0);
	eof_write_rbn_midis = get_config_int("preferences", "eof_write_rbn_midis", 0);
	eof_inverted_chords_slash = get_config_int("preferences", "eof_inverted_chords_slash", 0);
	enable_logging = get_config_int("preferences", "enable_logging", 1);
	eof_use_rb_colors = get_config_int("preferences", "eof_use_rb_colors", 1);
	eof_add_new_notes_to_selection = get_config_int("preferences", "eof_add_new_notes_to_selection", 0);
	eof_drum_modifiers_affect_all_difficulties = get_config_int("preferences", "eof_drum_modifiers_affect_all_difficulties", 1);

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
	ustrcpy(eof_ps_executable_path, get_config_string("paths", "ps_path", ""));
	ustrcpy(eof_ps_executable_name, get_filename(eof_ps_executable_path));
	ustrcpy(eof_ps_songs_path, get_config_string("paths", "ps_songs_path", ""));
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
	eof_hopo_view = get_config_int("editor", "hopo_view", EOF_HOPO_MANUAL);
	if((eof_hopo_view < 0) || (eof_hopo_view >= EOF_NUM_HOPO_MODES))
	{
		eof_hopo_view = EOF_HOPO_RF;
	}

	/* read waveform graph colors */
	eof_color_waveform_trough = get_config_hex("colors", "eof_color_waveform_trough", 0x007C00);	//The RGB equivalent of makecol(0, 124, 0)
	eof_color_waveform_peak = get_config_hex("colors", "eof_color_waveform_peak", 0x00BE00);		//The RGB equivalent of makecol(0, 190, 0)
	eof_color_waveform_rms = get_config_hex("colors", "eof_color_waveform_rms", 0xBE0000);			//The RGB equivalent of makecol(190, 0, 0)

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
	eof_log("eof_save_config() entered", 1);

	if(!fn)
	{
		return;
	}
	set_config_file(fn);

	/* write configuration */
	set_config_int("config", "av_delay", eof_av_delay);
	set_config_int("config", "buffer_size", eof_buffer_size);
	set_config_int("config", "smooth_playback", eof_smooth_pos);
	set_config_int("config", "disable_windows_fs", eof_disable_windows);
	set_config_int("config", "disable_vsync", eof_disable_vsync);
	set_config_int("config", "cpu_saver", eof_cpu_saver);;
	set_config_int("config", "mp3_support", eof_supports_mp3);
	set_config_int("config", "oggcat_support", eof_supports_oggcat);
	set_config_int("config", "fine_tune", eof_audio_fine_tune);
	set_config_int("config", "volume", eof_global_volume);

	/* write preferences */
	set_config_int("preferences", "invert_notes", eof_inverted_notes);
	set_config_int("preferences", "lefty", eof_lefty_mode);
	set_config_int("preferences", "input_mode", eof_input_mode);
	set_config_int("preferences", "ogg_quality", eof_ogg_setting);
	set_config_int("preferences", "use_ts", eof_use_ts);
	set_config_int("preferences", "eof_zoom", eof_zoom);
	set_config_int("preferences", "eof_selected_track", eof_selected_track);
	set_config_int("preferences", "eof_note_type", eof_note_type);
	set_config_int("preferences", "hide_drum_tails", eof_hide_drum_tails);
	set_config_int("preferences", "eof_hide_note_names", eof_hide_note_names);
	set_config_int("preferences", "eof_disable_sound_processing", eof_disable_sound_processing);
	set_config_int("preferences", "eof_disable_3d_rendering", eof_disable_3d_rendering);
	set_config_int("preferences", "eof_disable_2d_rendering", eof_disable_2d_rendering);
	set_config_int("preferences", "eof_disable_info_panel", eof_disable_info_panel);
	set_config_int("preferences", "eof_chart_volume", eof_chart_volume);
	set_config_int("preferences", "eof_clap_volume", eof_clap_volume);
	set_config_int("preferences", "eof_tick_volume", eof_tick_volume);
	set_config_int("preferences", "eof_tone_volume", eof_tone_volume);
	set_config_int("preferences", "eof_percussion_volume", eof_percussion_volume);
	set_config_int("preferences", "eof_selected_percussion_cue", eof_selected_percussion_cue);
	set_config_int("preferences", "eof_paste_erase_overlap", eof_paste_erase_overlap);
	set_config_int("preferences", "eof_write_rbn_midis", eof_write_rbn_midis);
	set_config_int("preferences", "eof_inverted_chords_slash", eof_inverted_chords_slash);
	set_config_int("preferences", "enable_logging", enable_logging);
	set_config_int("preferences", "eof_use_rb_colors", eof_use_rb_colors);
	set_config_int("preferences", "eof_add_new_notes_to_selection", eof_add_new_notes_to_selection);
	set_config_int("preferences", "eof_drum_modifiers_affect_all_difficulties", eof_drum_modifiers_affect_all_difficulties);

	/* write display settings */
	set_config_int("display", "display_mode", eof_screen_layout.mode);
	set_config_int("display", "note_auto_adjust", eof_note_auto_adjust);
	set_config_int("display", "software_mouse", eof_soft_cursor);
	set_config_int("display", "true_color", eof_desktop);

	/* write paths */
	set_config_string("paths", "fof_path", eof_fof_executable_path);
	set_config_string("paths", "fof_songs_path", eof_fof_songs_path);
	set_config_string("paths", "ps_path", eof_ps_executable_path);
	set_config_string("paths", "ps_songs_path", eof_ps_songs_path);
	set_config_string("paths", "songs_path", eof_songs_path);
	set_config_string("paths", "eof_path", eof_last_eof_path);
	set_config_string("paths", "ogg_path", eof_last_ogg_path);

	/* write editor settings */
	set_config_string("editor", "frettist", eof_last_frettist);
	set_config_int("editor", "snap_mode", eof_snap_mode);
	set_config_int("editor", "snap_interval", eof_snap_interval);
	set_config_int("editor", "snap_measure", eof_custom_snap_measure);
	set_config_int("editor", "preview_speed", eof_zoom_3d);
	set_config_int("editor", "hopo_view", eof_hopo_view);

	/* write waveform graph colors */
	set_config_hex("colors", "eof_color_waveform_trough", eof_color_waveform_trough);
	set_config_hex("colors", "eof_color_waveform_peak", eof_color_waveform_peak);
	set_config_hex("colors", "eof_color_waveform_rms", eof_color_waveform_rms);

	eof_controller_save_config(&eof_guitar, "guitar");
	eof_controller_save_config(&eof_drums, "drums");
}
