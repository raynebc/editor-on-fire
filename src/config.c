#include <allegro.h>
#include <ctype.h>	//For isdigit(), isspace()
#include <math.h>	//For sqrt()
#include "config.h"
#include "main.h"
#include "mix.h"
#include "menu/song.h"	//For eof_set_percussion_cue()
#include "tuning.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

static void set_default_controller_config(void);	//Applies the default controller settings, and overwrites settings from a loaded configuration where applicable

void set_default_controller_config(void)
{
	eof_log("set_default_controller_config() entered", 1);

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
	eof_guitar.button[7].type = EOF_CONTROLLER_BUTTON_TYPE_KEY;
	eof_guitar.button[7].key = KEY_F6;
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
	char gp_drum_mappings[1024] = {0};
	int num_default_settings, ctr;
	const char **default_settings = NULL;
	char string[EOF_INI_LENGTH] = {0};

	eof_log("eof_load_config() entered", 1);

	if(!fn)
	{
		return;
	}
	set_default_controller_config();
	set_config_file(fn);

	/* read configuration */
	eof_av_delay = get_config_int("config", "av_delay", 300);
	eof_midi_tone_delay = get_config_int("config", "eof_midi_tone_delay", 0);
	eof_midi_synth_instrument_guitar = get_config_int("config", "eof_midi_synth_instrument_guitar", 28);
	eof_midi_synth_instrument_guitar_muted = get_config_int("config", "eof_midi_synth_instrument_guitar_muted", 29);
	eof_midi_synth_instrument_guitar_harm = get_config_int("config", "eof_midi_synth_instrument_guitar_harm", 32);
	eof_midi_synth_instrument_bass = get_config_int("config", "eof_midi_synth_instrument_bass", 34);
	eof_scroll_seek_percent = get_config_int("config", "eof_scroll_seek_percent", 5);
	if((eof_scroll_seek_percent < 1) || (eof_scroll_seek_percent > 500))
	{
		eof_scroll_seek_percent = 5;
	}
	eof_buffer_size = get_config_int("config", "buffer_size", 6144);
	eof_smooth_pos = get_config_int("config", "smooth_playback", 1);
	eof_disable_windows = get_config_int("config", "disable_windows_fs", 0);
	ncdfs_use_allegro = eof_disable_windows;
	eof_disable_vsync = get_config_int("config", "disable_vsync", 1);
	eof_cpu_saver = get_config_int("config", "cpu_saver", 0);
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
	eof_input_mode = get_config_int("preferences", "input_mode", EOF_INPUT_REX);
	eof_ogg_setting = get_config_int("preferences", "ogg_quality", 1);
	eof_use_ts = get_config_int("preferences", "use_ts", 1);
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
	eof_enable_notes_panel = get_config_int("preferences", "eof_enable_notes_panel", 0);
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
	eof_clap_for_mutes = get_config_int("preferences", "eof_clap_for_mutes", 1);
	eof_clap_for_ghosts = get_config_int("preferences", "eof_clap_for_ghosts", 1);
	eof_multi_pitch_metronome = get_config_int("preferences", "eof_multi_pitch_metronome", 1);
	eof_set_percussion_cue(eof_selected_percussion_cue);
	eof_paste_erase_overlap = get_config_int("preferences", "eof_paste_erase_overlap", 0);
	eof_write_fof_files = get_config_int("preferences", "eof_write_fof_files", 1);
	eof_write_gh_files = get_config_int("preferences", "eof_write_gh_files", 0);
	eof_write_rb_files = get_config_int("preferences", "eof_write_rb_files", 0);
	eof_write_music_midi = get_config_int("preferences", "eof_write_music_midi", 0);
	eof_write_rs_files = get_config_int("preferences", "eof_write_rs_files", 0);
	eof_write_rs2_files = get_config_int("preferences", "eof_write_rs2_files", 1);
	eof_write_immerrock_files = get_config_int("preferences", "eof_write_immerrock_files", 0);
	eof_abridged_rs2_export = get_config_int("preferences", "eof_abridged_rs2_export", 1);
	eof_rs2_export_extended_ascii_lyrics = get_config_int("preferences", "eof_rs2_export_extended_ascii_lyrics", 0);
	eof_disable_ini_difference_warnings = get_config_int("preferences", "eof_disable_ini_difference_warnings", 0);
	eof_write_bf_files = get_config_int("preferences", "eof_write_bf_files", 0);
	eof_write_lrc_files = get_config_int("preferences", "eof_write_lrc_files", 0);
	eof_force_pro_drum_midi_notation = get_config_int("preferences", "eof_force_pro_drum_midi_notation", 1);
	eof_inverted_chords_slash = get_config_int("preferences", "eof_inverted_chords_slash", 0);
	eof_click_changes_dialog_focus = get_config_int("preferences", "eof_click_changes_dialog_focus", 1);
	if(eof_click_changes_dialog_focus)
		gui_mouse_focus = 0;
	else
		gui_mouse_focus = 1;
	eof_stop_playback_leave_focus = get_config_int("preferences", "eof_stop_playback_leave_focus", 1);
	eof_enable_logging = get_config_int("preferences", "eof_enable_logging", 1);
	eof_log_level = get_config_int("preferences", "eof_log_level", 1);
	if(eof_enable_logging && !eof_log_level)
	{	//If logging is enabled, but the logging level was zero for some reason
		eof_log_level = 1;
	}
	if((eof_log_level < 0) || (eof_log_level > 3))
	{	//If the logging level is invalid
		eof_log_level = 1;
	}
	eof_2d_render_top_option = get_config_int("preferences", "eof_2d_render_top_option", 5);
	if((eof_2d_render_top_option < 5) || (eof_2d_render_top_option > 10))
	{	//If eof_2d_render_top_option is invalid
		eof_2d_render_top_option = 5;	//Reset to default of displaying note names
	}
	eof_color_set = get_config_int("preferences", "eof_color_set", 0);
	if(eof_color_set >= EOF_NUM_COLOR_SETS)
	{	//Bounds check this value
		eof_color_set = 0;
	}
	eof_add_new_notes_to_selection = get_config_int("preferences", "eof_add_new_notes_to_selection", 0);
	eof_drum_modifiers_affect_all_difficulties = get_config_int("preferences", "eof_drum_modifiers_affect_all_difficulties", 1);
	eof_fb_seek_controls = get_config_int("preferences", "eof_fb_seek_controls", 0);
	eof_min_note_length = get_config_int("preferences", "eof_min_note_length", 0);
	eof_chord_density_threshold = get_config_int("preferences", "eof_chord_density_threshold", 10000);
	if(eof_chord_density_threshold > 99999)
	{	//If eof_chord_density_threshold is invalid (more than 5 digits)
		eof_chord_density_threshold = 10000;
	}
	eof_new_note_length_1ms = get_config_int("preferences", "eof_new_note_length_1ms", 0);
	eof_use_fof_difficulty_naming = get_config_int("preferences", "eof_use_fof_difficulty_naming", 0);
	if(eof_use_fof_difficulty_naming)
	{
		eof_note_type_name = eof_note_type_name_fof;
	}
	eof_new_note_forced_strum = get_config_int("preferences", "eof_new_note_forced_strum", 0);
	eof_gp_import_preference_1 = get_config_int("preferences", "eof_gp_import_preference_1", 0);
	eof_gp_import_truncate_short_notes = get_config_int("preferences", "eof_gp_import_truncate_short_notes", 1);
	eof_gp_import_truncate_short_chords = get_config_int("preferences", "eof_gp_import_truncate_short_chords", 1);
	eof_gp_import_replaces_track = get_config_int("preferences", "eof_gp_import_replaces_track", 1);
	eof_gp_import_replaces_track = get_config_int("preferences", "eof_gp_import_nat_harmonics_only", 0);
	eof_render_3d_rs_chords = get_config_int("preferences", "eof_render_3d_rs_chords", 0);
	eof_render_2d_rs_piano_roll = get_config_int("preferences", "eof_render_2d_rs_piano_roll", 0);
	eof_disable_backups = get_config_int("preferences", "eof_disable_backups", 0);
	eof_min_note_distance = get_config_int("preferences", "eof_min_note_distance", 3);
	eof_enable_open_strums_by_default = get_config_int("preferences", "eof_enable_open_strums_by_default", 0);
	if(eof_min_note_distance > 999)
	{	//If the minimum note distance is invalid (more than 3 digits)
		eof_min_note_distance = 3;	//Reset it to default
	}
	eof_min_note_distance_intervals = get_config_int("preferences", "eof_min_note_distance_intervals", 0);
	if(eof_min_note_distance_intervals > 2)
	{	//If the minimum note distance setting isn't ms, 1/# measure or 1/# beat
		eof_min_note_distance_intervals = 0;	//Reset it to ms
	}
	eof_enforce_chord_density = get_config_int("preferences", "eof_enforce_chord_density", 0);
	eof_imports_recall_last_path = get_config_int("preferences", "eof_imports_recall_last_path", 0);
	eof_rewind_at_end = get_config_int("preferences", "eof_rewind_at_end", 1);
	eof_disable_rs_wav = get_config_int("preferences", "eof_disable_rs_wav", 0);
	eof_display_seek_pos_in_seconds = get_config_int("preferences", "eof_display_seek_pos_in_seconds", 0);
	eof_note_tails_clickable = get_config_int("preferences", "eof_note_tails_clickable", 0);
	eof_render_grid_lines = get_config_int("preferences", "eof_render_grid_lines", 0);
	eof_render_bass_drum_in_lane = get_config_int("preferences", "eof_render_bass_drum_in_lane", 0);
	eof_vanish_y = get_config_int("preferences", "eof_vanish_y", 0);
	if((eof_vanish_y < -500) || (eof_vanish_y > 260))
	{	//Correct the value if it is out of bounds
		eof_vanish_y = 0;
	}
	eof_playback_time_stretch = get_config_int("preferences", "eof_playback_time_stretch", 1);
	eof_auto_complete_fingering = get_config_int("preferences", "eof_auto_complete_fingering", 1);
	eof_rbn_export_slider_hopo = get_config_int("preferences", "eof_rbn_export_slider_hopo", 0);
	eof_imports_drop_mid_beat_tempos = get_config_int("preferences", "eof_db_import_drop_mid_beat_tempos", 0);	//This variable is replaced by eof_imports_drop_mid_beat_tempos
	if(!eof_imports_drop_mid_beat_tempos)
	{	//If the deprecated eof_db_import_drop_mid_beat_tempos variable was not read, look for the variable's replacement
		eof_imports_drop_mid_beat_tempos = get_config_int("preferences", "eof_imports_drop_mid_beat_tempos", 0);
	}
	eof_render_mid_beat_tempos_blue = get_config_int("preferences", "eof_render_mid_beat_tempos_blue", 1);
	eof_disable_ini_export = get_config_int("preferences", "eof_disable_ini_export", 0);
	eof_gh_import_sustain_threshold_prompt = get_config_int("preferences", "eof_gh_import_sustain_threshold_prompt", 0);
	eof_rs_import_all_handshapes = get_config_int("preferences", "eof_rs_import_all_handshapes", 0);
	eof_rs2_export_version_8 = get_config_int("preferences", "eof_rs2_export_version_8", 0);
	eof_midi_export_enhanced_open_marker = get_config_int("preferences", "eof_midi_export_enhanced_open_marker", 0);
	eof_db_import_suppress_5nc_conversion = get_config_int("preferences", "eof_db_import_suppress_5nc_conversion", 0);
	eof_dont_auto_name_double_stops = get_config_int("preferences", "eof_dont_auto_name_double_stops", 0);
	eof_section_auto_adjust = get_config_int("preferences", "eof_section_auto_adjust", 1);
	eof_technote_auto_adjust = get_config_int("preferences", "eof_technote_auto_adjust", 1);
	eof_top_of_2d_pane_cycle_count_2 = get_config_int("preferences", "eof_top_of_2d_pane_cycle_count_2", 0);
	eof_warn_missing_bass_fhps = get_config_int("preferences", "eof_warn_missing_bass_fhps", 1);
	eof_fingering_checks_include_mutes = get_config_int("preferences", "eof_fingering_checks_include_mutes", 0);
	eof_ghl_conversion_swaps_bw_gems = get_config_int("preferences", "eof_ghl_conversion_swaps_bw_gems", 0);
	eof_3d_hopo_scale_size = get_config_int("preferences", "eof_3d_hopo_scale_size", 75);
	if((eof_3d_hopo_scale_size < 10) || (eof_3d_hopo_scale_size > 200))
	{
		eof_3d_hopo_scale_size = 75;
	}
	eof_prefer_midi_friendly_grid_snapping = get_config_int("preferences", "eof_prefer_midi_friendly_grid_snapping", 1);
	eof_dont_auto_edit_new_lyrics = get_config_int("preferences", "eof_dont_auto_edit_new_lyrics", 0);
	eof_dont_redraw_on_exit_prompt = get_config_int("preferences", "eof_dont_redraw_on_exit_prompt", 0);

	/* read display settings */
	eof_screen_layout.mode = get_config_int("display", "display_mode", 0);
	eof_screen_width = get_config_int("display", "eof_screen_width", 640);
	eof_screen_height = get_config_int("display", "eof_screen_height", 480);
	if((eof_screen_layout.mode < 0) || (eof_screen_layout.mode > 3))
	{
		eof_screen_layout.mode = 0;
	}
	eof_note_auto_adjust = get_config_int("display", "note_auto_adjust", 1);
	eof_soft_cursor = get_config_int("display", "software_mouse", 0);
	eof_desktop = get_config_int("display", "true_color", 1);

	/* read paths */
	(void) ustrcpy(eof_fof_executable_path, get_config_string("paths", "fof_path", ""));
	(void) ustrcpy(eof_fof_executable_name, get_filename(eof_fof_executable_path));
	(void) ustrcpy(eof_fof_songs_path, get_config_string("paths", "fof_songs_path", ""));
	(void) ustrcpy(eof_ps_executable_path, get_config_string("paths", "ps_path", ""));
	(void) ustrcpy(eof_ps_executable_name, get_filename(eof_ps_executable_path));
	(void) ustrcpy(eof_rs_to_tab_executable_path, get_config_string("paths", "rs_to_tab_path", ""));
	(void) ustrcpy(eof_ps_songs_path, get_config_string("paths", "ps_songs_path", ""));
	(void) ustrcpy(eof_songs_path, get_config_string("paths", "songs_path", ""));
	(void) ustrcpy(eof_last_eof_path, get_config_string("paths", "eof_path", ""));
	(void) ustrcpy(eof_last_ogg_path, get_config_string("paths", "ogg_path", ""));
	(void) ustrcpy(eof_last_midi_path, get_config_string("paths", "midi_path", ""));
	(void) ustrcpy(eof_last_dr_path, get_config_string("paths", "dr_path", ""));
	(void) ustrcpy(eof_last_db_path, get_config_string("paths", "db_path", ""));
	(void) ustrcpy(eof_last_gh_path, get_config_string("paths", "gh_path", ""));
	(void) ustrcpy(eof_last_ghl_path, get_config_string("paths", "ghl_path", ""));
	(void) ustrcpy(eof_last_lyric_path, get_config_string("paths", "lyric_path", ""));
	(void) ustrcpy(eof_last_gp_path, get_config_string("paths", "gp_path", ""));
	(void) ustrcpy(eof_last_rs_path, get_config_string("paths", "rs_path", ""));
	(void) ustrcpy(eof_last_sonic_visualiser_path, get_config_string("paths", "sonic_visualiser_path", ""));
	(void) ustrcpy(eof_last_bf_path, get_config_string("paths", "bf_path", ""));
	(void) ustrcpy(eof_last_browsed_notes_panel_path, get_config_string("paths", "eof_last_browsed_notes_panel_path", ""));
	(void) ustrcpy(eof_current_notes_panel_path, get_config_string("paths", "eof_current_notes_panel_path", ""));

	/* read editor settings */
	(void) ustrcpy(eof_last_frettist, get_config_string("editor", "frettist", ""));
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
	if((eof_zoom_3d < 1) || (eof_zoom_3d > 5))
	{
		eof_zoom_3d = 5;
	}
	eof_hopo_view = get_config_int("editor", "hopo_view", EOF_HOPO_MANUAL);
	if((eof_hopo_view < 0) || (eof_hopo_view >= EOF_NUM_HOPO_MODES))
	{
		eof_hopo_view = EOF_HOPO_RF;
	}

	/* read waveform graph colors */
	eof_color_waveform_trough_raw = get_config_hex("colors", "eof_color_waveform_trough", 0x007C00);	//The RGB equivalent of makecol(0, 124, 0)
	eof_color_waveform_peak_raw = get_config_hex("colors", "eof_color_waveform_peak", 0x00BE00);		//The RGB equivalent of makecol(0, 190, 0)
	eof_color_waveform_rms_raw = get_config_hex("colors", "eof_color_waveform_rms", 0xBE0000);			//The RGB equivalent of makecol(190, 0, 0)

	/* read waveform settings */
	eof_waveform_renderlocation = get_config_int("waveform", "eof_waveform_renderlocation", 0);
	eof_waveform_renderleftchannel = get_config_int("waveform", "eof_waveform_renderleftchannel", 1);
	eof_waveform_renderrightchannel = get_config_int("waveform", "eof_waveform_renderrightchannel", 0);
	eof_waveform_renderscale_enabled = get_config_int("waveform", "eof_waveform_renderscale_enabled", 0);
	eof_waveform_renderscale = get_config_int("waveform", "eof_waveform_renderscale", 100);
	if((eof_waveform_renderscale < 10) || (eof_waveform_renderscale > 999))
		eof_waveform_renderscale = 100;	//Bounds check

	/* read highlight colors */
	eof_color_highlight1_raw = get_config_hex("colors", "eof_color_highlight1", 0xFFFF00UL);	//The RGB equivalent of makecol(255, 255, 0), AKA yellow
	eof_color_highlight2_raw = get_config_hex("colors", "eof_color_highlight2", 0x00FFFFUL);	//The RGB equivalent of makecol(0, 255, 255), AKA cyan

	if(exists(fn))
	{	//Only try to load the controller buttons if the config file exists, otherwise the defaults will be erased
		eof_controller_load_config(&eof_guitar, "guitar");
		eof_controller_read_button_names(&eof_guitar);
		eof_controller_load_config(&eof_drums, "drums");
		eof_controller_read_button_names(&eof_drums);
	}

	/* other */
	eof_display_flats = get_config_int("other", "eof_display_flats", 0);
	if(eof_display_flats)
	{
		eof_note_names = eof_note_names_flat;
		eof_slash_note_names = eof_slash_note_names_flat;
	}
	eof_4_fret_range = get_config_int("other", "eof_4_fret_range", 1);
	eof_5_fret_range = get_config_int("other", "eof_5_fret_range", 0);
	eof_6_fret_range = get_config_int("other", "eof_6_fret_range", 0);

	(void) ustrcpy(gp_drum_mappings, get_config_string("other", "gp_drum_import_lane_1", "35,36"));
	if(!eof_parse_gp_drum_mappings(gp_drum_import_lane_1, gp_drum_mappings, EOF_GP_DRUM_MAPPING_COUNT))
	{	//If the drum mappings couldn't be parsed and stored
		unsigned char default_mapping[] = {35,36,0};
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Error processing gp_drum_import_lane_1 from the config file, resetting to defaults.");
		eof_log(eof_log_string, 1);
		allegro_message("%s", eof_log_string);
		memcpy(gp_drum_import_lane_1, default_mapping, sizeof(default_mapping));
	}

	(void) ustrcpy(gp_drum_mappings, get_config_string("other", "gp_drum_import_lane_2", "38,40"));
	if(!eof_parse_gp_drum_mappings(gp_drum_import_lane_2, gp_drum_mappings, EOF_GP_DRUM_MAPPING_COUNT))
	{	//If the drum mappings couldn't be parsed and stored
		unsigned char default_mapping[] = {38,40,0};
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Error processing gp_drum_import_lane_2 from the config file, resetting to defaults.");
		eof_log(eof_log_string, 1);
		allegro_message("%s", eof_log_string);
		memcpy(gp_drum_import_lane_2, default_mapping, sizeof(default_mapping));
	}
	(void) ustrcpy(gp_drum_mappings, get_config_string("other", "gp_drum_import_lane_2_rimshot", "37"));
	if(!eof_parse_gp_drum_mappings(gp_drum_import_lane_2_rimshot, gp_drum_mappings, EOF_GP_DRUM_MAPPING_COUNT))
	{	//If the drum mappings couldn't be parsed and stored
		unsigned char default_mapping[] = {37,0};
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Error processing gp_drum_import_lane_2_rimshot from the config file, resetting to defaults.");
		eof_log(eof_log_string, 1);
		allegro_message("%s", eof_log_string);
		memcpy(gp_drum_import_lane_2_rimshot, default_mapping, sizeof(default_mapping));
	}

	(void) ustrcpy(gp_drum_mappings, get_config_string("other", "gp_drum_import_lane_3", "47,50"));
	if(!eof_parse_gp_drum_mappings(gp_drum_import_lane_3, gp_drum_mappings, EOF_GP_DRUM_MAPPING_COUNT))
	{	//If the drum mappings couldn't be parsed and stored
		unsigned char default_mapping[] = {47,50,0};
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Error processing gp_drum_import_lane_3 from the config file, resetting to defaults.");
		eof_log(eof_log_string, 1);
		allegro_message("%s", eof_log_string);
		memcpy(gp_drum_import_lane_3, default_mapping, sizeof(default_mapping));
	}
	(void) ustrcpy(gp_drum_mappings, get_config_string("other", "gp_drum_import_lane_3_cymbal", "42,54,92"));
	if(!eof_parse_gp_drum_mappings(gp_drum_import_lane_3_cymbal, gp_drum_mappings, EOF_GP_DRUM_MAPPING_COUNT))
	{	//If the drum mappings couldn't be parsed and stored
		unsigned char default_mapping[] = {42,54,92,0};
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Error processing gp_drum_import_lane_3_cymbal from the config file, resetting to defaults.");
		eof_log(eof_log_string, 1);
		allegro_message("%s", eof_log_string);
		memcpy(gp_drum_import_lane_3_cymbal, default_mapping, sizeof(default_mapping));
	}
	(void) ustrcpy(gp_drum_mappings, get_config_string("other", "gp_drum_import_lane_3_hi_hat_pedal", "44"));
	if(!eof_parse_gp_drum_mappings(gp_drum_import_lane_3_hi_hat_pedal, gp_drum_mappings, EOF_GP_DRUM_MAPPING_COUNT))
	{	//If the drum mappings couldn't be parsed and stored
		unsigned char default_mapping[] = {44,0};
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Error processing gp_drum_import_lane_3_hi_hat_pedal from the config file, resetting to defaults.");
		eof_log(eof_log_string, 1);
		allegro_message("%s", eof_log_string);
		memcpy(gp_drum_import_lane_3_hi_hat_pedal, default_mapping, sizeof(default_mapping));
	}
	(void) ustrcpy(gp_drum_mappings, get_config_string("other", "gp_drum_import_lane_3_hi_hat_open", "46"));
	if(!eof_parse_gp_drum_mappings(gp_drum_import_lane_3_hi_hat_open, gp_drum_mappings, EOF_GP_DRUM_MAPPING_COUNT))
	{	//If the drum mappings couldn't be parsed and stored
		unsigned char default_mapping[] = {46,0};
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Error processing gp_drum_import_lane_3_hi_hat_open from the config file, resetting to defaults.");
		eof_log(eof_log_string, 1);
		allegro_message("%s", eof_log_string);
		memcpy(gp_drum_import_lane_3_hi_hat_open, default_mapping, sizeof(default_mapping));
	}

	(void) ustrcpy(gp_drum_mappings, get_config_string("other", "gp_drum_import_lane_4", "45,48"));
	if(!eof_parse_gp_drum_mappings(gp_drum_import_lane_4, gp_drum_mappings, EOF_GP_DRUM_MAPPING_COUNT))
	{	//If the drum mappings couldn't be parsed and stored
		unsigned char default_mapping[] = {45,48,0};
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Error processing gp_drum_import_lane_4 from the config file, resetting to defaults.");
		eof_log(eof_log_string, 1);
		allegro_message("%s", eof_log_string);
		memcpy(gp_drum_import_lane_4, default_mapping, sizeof(default_mapping));
	}
	(void) ustrcpy(gp_drum_mappings, get_config_string("other", "gp_drum_import_lane_4_cymbal", "51,53,56,59"));
	if(!eof_parse_gp_drum_mappings(gp_drum_import_lane_4_cymbal, gp_drum_mappings, EOF_GP_DRUM_MAPPING_COUNT))
	{	//If the drum mappings couldn't be parsed and stored
		unsigned char default_mapping[] = {51,53,56,59,0};
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Error processing gp_drum_import_lane_4_cymbal from the config file, resetting to defaults.");
		eof_log(eof_log_string, 1);
		allegro_message("%s", eof_log_string);
		memcpy(gp_drum_import_lane_4_cymbal, default_mapping, sizeof(default_mapping));
	}

	(void) ustrcpy(gp_drum_mappings, get_config_string("other", "gp_drum_import_lane_5", "41,43"));
	if(!eof_parse_gp_drum_mappings(gp_drum_import_lane_5, gp_drum_mappings, EOF_GP_DRUM_MAPPING_COUNT))
	{	//If the drum mappings couldn't be parsed and stored
		unsigned char default_mapping[] = {41,43,0};
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Error processing gp_drum_import_lane_5 from the config file, resetting to defaults.");
		eof_log(eof_log_string, 1);
		allegro_message("%s", eof_log_string);
		memcpy(gp_drum_import_lane_5, default_mapping, sizeof(default_mapping));
	}
	(void) ustrcpy(gp_drum_mappings, get_config_string("other", "gp_drum_import_lane_5_cymbal", "49,52,55,57"));
	if(!eof_parse_gp_drum_mappings(gp_drum_import_lane_5_cymbal, gp_drum_mappings, EOF_GP_DRUM_MAPPING_COUNT))
	{	//If the drum mappings couldn't be parsed and stored
		unsigned char default_mapping[] = {49,52,55,57,0};
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Error processing gp_drum_import_lane_5_cymbal from the config file, resetting to defaults.");
		eof_log(eof_log_string, 1);
		allegro_message("%s", eof_log_string);
		memcpy(gp_drum_import_lane_5_cymbal, default_mapping, sizeof(default_mapping));
	}

	(void) ustrcpy(gp_drum_mappings, get_config_string("other", "gp_drum_import_lane_6", "0"));
	if(!eof_parse_gp_drum_mappings(gp_drum_import_lane_6, gp_drum_mappings, EOF_GP_DRUM_MAPPING_COUNT))
	{	//If the drum mappings couldn't be parsed and stored
		unsigned char default_mapping[] = {0};
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Error processing gp_drum_import_lane_6 from the config file, resetting to defaults.");
		eof_log(eof_log_string, 1);
		allegro_message("%s", eof_log_string);
		memcpy(gp_drum_import_lane_6, default_mapping, sizeof(default_mapping));
	}

	(void) strncpy(eof_lyric_gap_multiplier_string, get_config_string("other", "eof_lyric_gap_multiplier", "0.0"), sizeof(eof_lyric_gap_multiplier_string) - 1);
	eof_lyric_gap_multiplier = atof(eof_lyric_gap_multiplier_string);	//Retain the original string in memory and use a double floating point conversion of it
	if(eof_lyric_gap_multiplier < 0.0)
	{
		strncpy(eof_lyric_gap_multiplier_string, "0.0", sizeof(eof_lyric_gap_multiplier_string) - 1);
		eof_lyric_gap_multiplier = 0.0;	//Bounds check
	}

	eof_song_folder_prompt = get_config_int("other", "eof_song_folder_prompt", 0);

	drums_rock_remap_lane_1 = get_config_int("other", "drums_rock_remap_lane_1", 4);
	if(drums_rock_remap_lane_1 > 6)
		drums_rock_remap_lane_1 = 4;	//If the defined value is invalid, reset to default
	drums_rock_remap_lane_2 = get_config_int("other", "drums_rock_remap_lane_2", 3);
	if(drums_rock_remap_lane_2 > 6)
		drums_rock_remap_lane_2 = 3;	//If the defined value is invalid, reset to default
	drums_rock_remap_lane_3 = get_config_int("other", "drums_rock_remap_lane_3", 1);
	if(drums_rock_remap_lane_3 > 6)
		drums_rock_remap_lane_3 = 1;	//If the defined value is invalid, reset to default
	drums_rock_remap_lane_3_cymbal = get_config_int("other", "drums_rock_remap_lane_3_cymbal", 5);
	if(drums_rock_remap_lane_3_cymbal > 6)
		drums_rock_remap_lane_3_cymbal = 5;	//If the defined value is invalid, reset to default
	drums_rock_remap_lane_4 = get_config_int("other", "drums_rock_remap_lane_4", 1);
	if(drums_rock_remap_lane_4 > 6)
		drums_rock_remap_lane_4 = 1;	//If the defined value is invalid, reset to default
	drums_rock_remap_lane_4_cymbal = get_config_int("other", "drums_rock_remap_lane_4_cymbal", 5);
	if(drums_rock_remap_lane_4_cymbal > 6)
		drums_rock_remap_lane_4_cymbal = 5;	//If the defined value is invalid, reset to default
	drums_rock_remap_lane_5 = get_config_int("other", "drums_rock_remap_lane_5", 6);
	if(drums_rock_remap_lane_5 > 6)
		drums_rock_remap_lane_5 = 6;	//If the defined value is invalid, reset to default
	drums_rock_remap_lane_5_cymbal = get_config_int("other", "drums_rock_remap_lane_5_cymbal", 2);
	if(drums_rock_remap_lane_5_cymbal > 6)
		drums_rock_remap_lane_5_cymbal = 2;	//If the defined value is invalid, reset to default
	drums_rock_remap_lane_6 = get_config_int("other", "drums_rock_remap_lane_6", 2);
	if(drums_rock_remap_lane_6 > 6)
		drums_rock_remap_lane_6 = 2;	//If the defined value is invalid, reset to default

	//Convert MIDI tones to zero numbering
	if(eof_midi_synth_instrument_guitar > 0)
		eof_midi_synth_instrument_guitar--;
	if(eof_midi_synth_instrument_guitar_muted > 0)
		eof_midi_synth_instrument_guitar_muted--;
	if(eof_midi_synth_instrument_guitar_harm > 0)
		eof_midi_synth_instrument_guitar_harm--;
	if(eof_midi_synth_instrument_bass > 0)
		eof_midi_synth_instrument_bass--;

	//Read default INI settings
	num_default_settings = list_config_entries("default_ini_settings", &default_settings);
	for(ctr = 0; ctr < num_default_settings; ctr++)
	{	//For each default INI setting found in the config file
		(void) snprintf(string, sizeof(string), "%s = %s", default_settings[ctr], get_config_string("default_ini_settings", default_settings[ctr], ""));	//Rebuild the config string since Allegro breaks it apart
		(void) eof_default_ini_add(string, 0);	//Add the rebuilt string to the default INI entries
	}
	free_config_entries(&default_settings);
}

void eof_save_config(char * fn)
{
	char gp_drum_mappings[1024] = {0};
	int num_default_settings, ctr;
	unsigned long ctr2;
	const char **default_settings = NULL;
	char name[EOF_INI_LENGTH] = {0}, value[EOF_INI_LENGTH] = {0};

	eof_log("eof_save_config() entered", 1);

	if(!fn)
	{
		return;
	}
	if(eof_log_level > 2)
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Writing to config file:  %s", fn);
		eof_log(eof_log_string, 3);
	}
	set_config_file(fn);

	/* write configuration */
	eof_log("\tWriting INI entries", 3);
	set_config_int("config", "av_delay", eof_av_delay);
	set_config_int("config", "eof_midi_tone_delay", eof_midi_tone_delay);
	set_config_int("config", "eof_midi_synth_instrument_guitar", eof_midi_synth_instrument_guitar + 1);	//Add one to convert back to conventional numbering
	set_config_int("config", "eof_midi_synth_instrument_guitar_muted", eof_midi_synth_instrument_guitar_muted + 1);
	set_config_int("config", "eof_midi_synth_instrument_guitar_harm", eof_midi_synth_instrument_guitar_harm + 1);
	set_config_int("config", "eof_midi_synth_instrument_bass", eof_midi_synth_instrument_bass + 1);
	set_config_int("config", "eof_scroll_seek_percent", eof_scroll_seek_percent);
	set_config_int("config", "buffer_size", eof_buffer_size);
	set_config_int("config", "smooth_playback", eof_smooth_pos);
	set_config_int("config", "disable_windows_fs", eof_disable_windows);
	set_config_int("config", "disable_vsync", eof_disable_vsync);
	set_config_int("config", "cpu_saver", eof_cpu_saver);
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
	set_config_int("preferences", "eof_enable_notes_panel", eof_enable_notes_panel);
	set_config_int("preferences", "eof_chart_volume", eof_chart_volume);
	set_config_int("preferences", "eof_clap_volume", eof_clap_volume);
	set_config_int("preferences", "eof_tick_volume", eof_tick_volume);
	set_config_int("preferences", "eof_tone_volume", eof_tone_volume);
	set_config_int("preferences", "eof_percussion_volume", eof_percussion_volume);
	set_config_int("preferences", "eof_selected_percussion_cue", eof_selected_percussion_cue);
	set_config_int("preferences", "eof_clap_for_mutes", eof_clap_for_mutes);
	set_config_int("preferences", "eof_clap_for_ghosts", eof_clap_for_ghosts);
	set_config_int("preferences", "eof_multi_pitch_metronome", eof_multi_pitch_metronome);
	set_config_int("preferences", "eof_paste_erase_overlap", eof_paste_erase_overlap);
	set_config_int("preferences", "eof_write_fof_files", eof_write_fof_files);
	set_config_int("preferences", "eof_write_gh_files", eof_write_gh_files);
	set_config_int("preferences", "eof_write_rb_files", eof_write_rb_files);
	set_config_int("preferences", "eof_write_music_midi", eof_write_music_midi);
	set_config_int("preferences", "eof_write_rs_files", eof_write_rs_files);
	set_config_int("preferences", "eof_write_rs2_files", eof_write_rs2_files);
	set_config_int("preferences", "eof_write_immerrock_files", eof_write_immerrock_files);
	set_config_int("preferences", "eof_abridged_rs2_export", eof_abridged_rs2_export);
	set_config_int("preferences", "eof_rs2_export_extended_ascii_lyrics", eof_rs2_export_extended_ascii_lyrics);
	set_config_int("preferences", "eof_disable_ini_difference_warnings", eof_disable_ini_difference_warnings);
	set_config_int("preferences", "eof_write_bf_files", eof_write_bf_files);
	set_config_int("preferences", "eof_write_lrc_files", eof_write_lrc_files);
	set_config_int("preferences", "eof_force_pro_drum_midi_notation", eof_force_pro_drum_midi_notation);
	set_config_int("preferences", "eof_inverted_chords_slash", eof_inverted_chords_slash);
	set_config_int("preferences", "eof_click_changes_dialog_focus", eof_click_changes_dialog_focus);
	set_config_int("preferences", "eof_stop_playback_leave_focus", eof_stop_playback_leave_focus);
	set_config_int("preferences", "eof_enable_logging", eof_enable_logging);
	set_config_int("preferences", "eof_log_level", eof_log_level);
	set_config_int("preferences", "eof_color_set", eof_color_set);
	set_config_int("preferences", "eof_2d_render_top_option", eof_2d_render_top_option);
	set_config_int("preferences", "eof_add_new_notes_to_selection", eof_add_new_notes_to_selection);
	set_config_int("preferences", "eof_drum_modifiers_affect_all_difficulties", eof_drum_modifiers_affect_all_difficulties);
	set_config_int("preferences", "eof_fb_seek_controls", eof_fb_seek_controls);
	set_config_int("preferences", "eof_new_note_length_1ms", eof_new_note_length_1ms);
	set_config_int("preferences", "eof_new_note_forced_strum", eof_new_note_forced_strum);
	set_config_int("preferences", "eof_use_fof_difficulty_naming", eof_use_fof_difficulty_naming);
	set_config_int("preferences", "eof_gp_import_preference_1", eof_gp_import_preference_1);
	set_config_int("preferences", "eof_gp_import_truncate_short_notes", eof_gp_import_truncate_short_notes);
	set_config_int("preferences", "eof_gp_import_truncate_short_chords", eof_gp_import_truncate_short_chords);
	set_config_int("preferences", "eof_gp_import_replaces_track", eof_gp_import_replaces_track);
	set_config_int("preferences", "eof_gp_import_nat_harmonics_only", eof_gp_import_nat_harmonics_only);
	set_config_int("preferences", "eof_render_3d_rs_chords", eof_render_3d_rs_chords);
	set_config_int("preferences", "eof_render_2d_rs_piano_roll", eof_render_2d_rs_piano_roll);
	set_config_int("preferences", "eof_disable_backups", eof_disable_backups);
	set_config_int("preferences", "eof_enable_open_strums_by_default", eof_enable_open_strums_by_default);
	set_config_int("preferences", "eof_min_note_length", eof_min_note_length);
	set_config_int("preferences", "eof_enforce_chord_density", eof_enforce_chord_density);
	set_config_int("preferences", "eof_chord_density_threshold", eof_chord_density_threshold);
	set_config_int("preferences", "eof_min_note_distance", eof_min_note_distance);
	set_config_int("preferences", "eof_min_note_distance_intervals", eof_min_note_distance_intervals);
	set_config_int("preferences", "eof_render_bass_drum_in_lane", eof_render_bass_drum_in_lane);
	set_config_int("preferences", "eof_vanish_y", eof_vanish_y);
	set_config_int("preferences", "eof_playback_time_stretch", eof_playback_time_stretch);
	set_config_int("preferences", "eof_imports_recall_last_path", eof_imports_recall_last_path);
	set_config_int("preferences", "eof_rewind_at_end", eof_rewind_at_end);
	set_config_int("preferences", "eof_disable_rs_wav", eof_disable_rs_wav);
	set_config_int("preferences", "eof_display_seek_pos_in_seconds", eof_display_seek_pos_in_seconds);
	set_config_int("preferences", "eof_note_tails_clickable", eof_note_tails_clickable);
	set_config_int("preferences", "eof_render_grid_lines", eof_render_grid_lines);
	set_config_int("preferences", "eof_auto_complete_fingering", eof_auto_complete_fingering);
	set_config_int("preferences", "eof_rbn_export_slider_hopo", eof_rbn_export_slider_hopo);
	set_config_int("preferences", "eof_imports_drop_mid_beat_tempos", eof_imports_drop_mid_beat_tempos);
	set_config_int("preferences", "eof_render_mid_beat_tempos_blue", eof_render_mid_beat_tempos_blue);
	set_config_int("preferences", "eof_disable_ini_export", eof_disable_ini_export);
	set_config_int("preferences", "eof_gh_import_sustain_threshold_prompt", eof_gh_import_sustain_threshold_prompt);
	set_config_int("preferences", "eof_rs_import_all_handshapes", eof_rs_import_all_handshapes);
	set_config_int("preferences", "eof_rs2_export_version_8", eof_rs2_export_version_8);
	set_config_int("preferences", "eof_midi_export_enhanced_open_marker", eof_midi_export_enhanced_open_marker);
	set_config_int("preferences", "eof_db_import_suppress_5nc_conversion", eof_db_import_suppress_5nc_conversion);
	set_config_int("preferences", "eof_dont_auto_name_double_stops", eof_dont_auto_name_double_stops);
	set_config_int("preferences", "eof_section_auto_adjust", eof_section_auto_adjust);
	set_config_int("preferences", "eof_technote_auto_adjust", eof_technote_auto_adjust);
	set_config_int("preferences", "eof_top_of_2d_pane_cycle_count_2", eof_top_of_2d_pane_cycle_count_2);
	set_config_int("preferences", "eof_warn_missing_bass_fhps", eof_warn_missing_bass_fhps);
	set_config_int("preferences", "eof_fingering_checks_include_mutes", eof_fingering_checks_include_mutes);
	set_config_int("preferences", "eof_ghl_conversion_swaps_bw_gems", eof_ghl_conversion_swaps_bw_gems);
	set_config_int("preferences", "eof_3d_hopo_scale_size", eof_3d_hopo_scale_size);
	set_config_int("preferences", "eof_prefer_midi_friendly_grid_snapping", eof_prefer_midi_friendly_grid_snapping);
	set_config_int("preferences", "eof_dont_auto_edit_new_lyrics", eof_dont_auto_edit_new_lyrics);
	set_config_int("preferences", "eof_dont_redraw_on_exit_prompt", eof_dont_redraw_on_exit_prompt);

	/* write display settings */
	set_config_int("display", "display_mode", eof_screen_layout.mode);
	set_config_int("display", "eof_screen_width", eof_screen_width);
	set_config_int("display", "eof_screen_height", eof_screen_height);
	set_config_int("display", "note_auto_adjust", eof_note_auto_adjust);
	set_config_int("display", "software_mouse", eof_soft_cursor);
	set_config_int("display", "true_color", eof_desktop);

	/* write paths */
	set_config_string("paths", "fof_path", eof_fof_executable_path);
	set_config_string("paths", "fof_songs_path", eof_fof_songs_path);
	set_config_string("paths", "ps_path", eof_ps_executable_path);
	set_config_string("paths", "ps_songs_path", eof_ps_songs_path);
	set_config_string("paths", "rs_to_tab_path", eof_rs_to_tab_executable_path);
	set_config_string("paths", "songs_path", eof_songs_path);
	set_config_string("paths", "eof_path", eof_last_eof_path);
	set_config_string("paths", "ogg_path", eof_last_ogg_path);
	set_config_string("paths", "midi_path", eof_last_midi_path);
	set_config_string("paths", "dr_path", eof_last_dr_path);
	set_config_string("paths", "db_path", eof_last_db_path);
	set_config_string("paths", "gh_path", eof_last_gh_path);
	set_config_string("paths", "ghl_path", eof_last_ghl_path);
	set_config_string("paths", "lyric_path", eof_last_lyric_path);
	set_config_string("paths", "gp_path", eof_last_gp_path);
	set_config_string("paths", "rs_path", eof_last_rs_path);
	set_config_string("paths", "sonic_visualiser_path", eof_last_sonic_visualiser_path);
	set_config_string("paths", "bf_path", eof_last_bf_path);
	set_config_string("paths", "eof_last_browsed_notes_panel_path", eof_last_browsed_notes_panel_path);
	set_config_string("paths", "eof_current_notes_panel_path", eof_current_notes_panel_path);

	/* write editor settings */
	set_config_string("editor", "frettist", eof_last_frettist);
	set_config_int("editor", "snap_mode", eof_snap_mode);
	set_config_int("editor", "snap_interval", eof_snap_interval);
	set_config_int("editor", "snap_measure", eof_custom_snap_measure);
	set_config_int("editor", "preview_speed", eof_zoom_3d);
	set_config_int("editor", "hopo_view", eof_hopo_view);

	/* write waveform graph colors (raw format to avoid color corruption on subsequent launches of EOF) */
	set_config_hex("colors", "eof_color_waveform_trough", eof_color_waveform_trough_raw);
	set_config_hex("colors", "eof_color_waveform_peak", eof_color_waveform_peak_raw);
	set_config_hex("colors", "eof_color_waveform_rms", eof_color_waveform_rms_raw);

	/* write waveform graph settings */
	set_config_int("waveform", "eof_waveform_renderlocation", eof_waveform_renderlocation);
	set_config_int("waveform", "eof_waveform_renderleftchannel", eof_waveform_renderleftchannel);
	set_config_int("waveform", "eof_waveform_renderrightchannel", eof_waveform_renderrightchannel);
	set_config_int("waveform", "eof_waveform_renderscale_enabled", eof_waveform_renderscale_enabled);
	set_config_int("waveform", "eof_waveform_renderscale", eof_waveform_renderscale);

	/* write highlight colors (raw format to avoid color corruption on subsequent launches of EOF) */
	set_config_hex("colors", "eof_color_highlight1", eof_color_highlight1_raw);
	set_config_hex("colors", "eof_color_highlight2", eof_color_highlight2_raw);

	eof_log("\tSaving controller configs", 3);
	eof_controller_save_config(&eof_guitar, "guitar");
	eof_controller_save_config(&eof_drums, "drums");

	/* write other settings */
	set_config_int("other", "eof_display_flats", eof_display_flats);
	set_config_int("other", "eof_4_fret_range", eof_4_fret_range);
	set_config_int("other", "eof_5_fret_range", eof_5_fret_range);
	set_config_int("other", "eof_6_fret_range", eof_6_fret_range);

	eof_log("\tWriting GP drum import mappings", 3);
	eof_build_gp_drum_mapping_string(gp_drum_mappings, sizeof(gp_drum_mappings) - 1, gp_drum_import_lane_1);
	set_config_string("other", "gp_drum_import_lane_1", gp_drum_mappings);
	eof_build_gp_drum_mapping_string(gp_drum_mappings, sizeof(gp_drum_mappings) - 1, gp_drum_import_lane_2);
	set_config_string("other", "gp_drum_import_lane_2", gp_drum_mappings);
	eof_build_gp_drum_mapping_string(gp_drum_mappings, sizeof(gp_drum_mappings) - 1, gp_drum_import_lane_2_rimshot);
	set_config_string("other", "gp_drum_import_lane_2_rimshot", gp_drum_mappings);
	eof_build_gp_drum_mapping_string(gp_drum_mappings, sizeof(gp_drum_mappings) - 1, gp_drum_import_lane_3);
	set_config_string("other", "gp_drum_import_lane_3", gp_drum_mappings);
	eof_build_gp_drum_mapping_string(gp_drum_mappings, sizeof(gp_drum_mappings) - 1, gp_drum_import_lane_3_cymbal);
	set_config_string("other", "gp_drum_import_lane_3_cymbal", gp_drum_mappings);
	eof_build_gp_drum_mapping_string(gp_drum_mappings, sizeof(gp_drum_mappings) - 1, gp_drum_import_lane_3_hi_hat_pedal);
	set_config_string("other", "gp_drum_import_lane_3_hi_hat_pedal", gp_drum_mappings);
	eof_build_gp_drum_mapping_string(gp_drum_mappings, sizeof(gp_drum_mappings) - 1, gp_drum_import_lane_3_hi_hat_open);
	set_config_string("other", "gp_drum_import_lane_3_hi_hat_open", gp_drum_mappings);
	eof_build_gp_drum_mapping_string(gp_drum_mappings, sizeof(gp_drum_mappings) - 1, gp_drum_import_lane_4);
	set_config_string("other", "gp_drum_import_lane_4", gp_drum_mappings);
	eof_build_gp_drum_mapping_string(gp_drum_mappings, sizeof(gp_drum_mappings) - 1, gp_drum_import_lane_4_cymbal);
	set_config_string("other", "gp_drum_import_lane_4_cymbal", gp_drum_mappings);
	eof_build_gp_drum_mapping_string(gp_drum_mappings, sizeof(gp_drum_mappings) - 1, gp_drum_import_lane_5);
	set_config_string("other", "gp_drum_import_lane_5", gp_drum_mappings);
	eof_build_gp_drum_mapping_string(gp_drum_mappings, sizeof(gp_drum_mappings) - 1, gp_drum_import_lane_5_cymbal);
	set_config_string("other", "gp_drum_import_lane_5_cymbal", gp_drum_mappings);
	eof_build_gp_drum_mapping_string(gp_drum_mappings, sizeof(gp_drum_mappings) - 1, gp_drum_import_lane_6);
	set_config_string("other", "gp_drum_import_lane_6", gp_drum_mappings);

	set_config_string("other", "eof_lyric_gap_multiplier", eof_lyric_gap_multiplier_string);
	set_config_int("other", "eof_song_folder_prompt", eof_song_folder_prompt);

	set_config_int("other", "drums_rock_remap_lane_1", drums_rock_remap_lane_1);
	set_config_int("other", "drums_rock_remap_lane_2", drums_rock_remap_lane_2);
	set_config_int("other", "drums_rock_remap_lane_3", drums_rock_remap_lane_3);
	set_config_int("other", "drums_rock_remap_lane_3_cymbal", drums_rock_remap_lane_3_cymbal);
	set_config_int("other", "drums_rock_remap_lane_4", drums_rock_remap_lane_4);
	set_config_int("other", "drums_rock_remap_lane_4_cymbal", drums_rock_remap_lane_4_cymbal);
	set_config_int("other", "drums_rock_remap_lane_5", drums_rock_remap_lane_5);
	set_config_int("other", "drums_rock_remap_lane_5_cymbal", drums_rock_remap_lane_5_cymbal);
	set_config_int("other", "drums_rock_remap_lane_6", drums_rock_remap_lane_6);

	//Delete existing default INI settings from config file
	eof_log("\tRewriting default INI settings", 3);
	num_default_settings = list_config_entries("default_ini_settings", &default_settings);
	for(ctr = 0; ctr < num_default_settings; ctr++)
	{	//For each default INI setting found in the config file
		set_config_string("default_ini_settings", default_settings[ctr], NULL);
	}
	free_config_entries(&default_settings);

	//Write default INI settings
	for(ctr2 = 0; ctr2 < eof_default_ini_settings; ctr2++)
	{	//For each default INI setting stored in memory
		if(!eof_parse_config_entry_name(name, sizeof(name), value, sizeof(value),eof_default_ini_setting[ctr2]))
		{	//If the setting was successfully parsed into a name and a value
			set_config_string("default_ini_settings", name, value);	//Write them to the config file
		}
	}

	flush_config_file();
	eof_log("\teof_save_config() completed", 3);
}

void eof_build_gp_drum_mapping_string(char *destination, size_t size, unsigned char *mapping_list)
{
	unsigned long ctr, writecount = 0;
	char str[10] = {0};

	if(!destination || !mapping_list || (size < 2))
		return;	//Invalid parameters

	destination[0] = '\0';	//Empty the string
	for(ctr = 0; ctr < EOF_GP_DRUM_MAPPING_COUNT; ctr++)
	{	//For each element in the drum mapping list
		if(mapping_list[ctr] == 0)
			break;	//All defined mappings have been checked

		if(writecount)
		{	//If this isn't the first number written
			strncat(destination, ",", size - 1);	//Separate with a comma
		}
		(void) snprintf(str, sizeof(str) - 1, "%d", mapping_list[ctr]);	//Convert the number to ASCII
		strncat(destination, str, size - 1);						//Append it to the destination string
		writecount++;
	}
	if(!writecount)
	{	//If the mapping list is empty, write a string of "0"
		destination[0] = '0';
		destination[1] = '\0';
	}
}

unsigned long eof_parse_gp_drum_mappings(unsigned char *destination, char *input, unsigned countlimit)
{
	unsigned long number = 0, digit, index, parsecount = 0;
	char ch, parsed = 0;

	if(!destination || !input || !countlimit)
		return 0;	//Invalid parameters

	for(index = 0; input[index] != '\0'; index++)
	{	//For each character in the input string
		ch = input[index];	//Simplify

		if(isdigit(ch))
		{	//If this is a numerical character
			digit = ch - '0';	//Convert to numerical value
			number *= 10;		//Another digit means the previously read digits are worth ten times as much
			number += digit;	//Add to the value of this digit to the number being parsed
			parsed = 1;			//Track that a number has been parsed
		}
		else if((input[index] == ',') || (input[index] == ' '))
		{	//This is a separator, store the parsed number into the destination array
			if(number > 255)
				return 0;	//Input number too large
			if(parsecount >= countlimit)
				return 0;	//Too many input numbers

			destination[parsecount] = number;
			parsecount++;
			number = parsed = 0;
		}
		else
		{
			return 0;	//Invalid input
		}
	}

	//Store any number that was parsed but not followed by a delimiter other than the NULL terminator
	if(parsed)
	{	//If a number has been parsed
		if(number > 255)
			return 0;	//Input number too large
		if(parsecount >= countlimit)
			return 0;	//Too many input numbers

		destination[parsecount] = number;
		parsecount++;
	}

	//If there is space in the destination buffer, append a 0 value to indicate the end of the number list
	if(parsecount  < countlimit)
		destination[parsecount] = 0;

	return parsecount;	//Return number of parsed entries
}

int eof_lookup_drum_mapping(unsigned char *mapping_list, unsigned char value)
{
	unsigned long ctr;

	if(!mapping_list || !value)
		return 0;

	for(ctr = 0; ctr < EOF_GP_DRUM_MAPPING_COUNT; ctr++)
	{	//For each element in the drum mapping list
		if(mapping_list[ctr] == 0)
			break;	//All defined mappings have been checked
		if(mapping_list[ctr] == value)
			return 1;	//A match was found
	}

	return 0;	//No match was found
}

int eof_parse_config_entry_name(char *name, size_t name_size, char *value, size_t value_size, const char *string)
{
	size_t s_index = 0, n_index = 0, v_index = 0;
	char name_started = 0, name_ended = 0;

	if(!name || !value || !string || !name_size || !value_size)
		return 1;	//Error:  Invalid parameters

	while(string[s_index] != '\0')
	{	//Until the end of the source string is reached
		int this_char = string[s_index];

		s_index++;
		if(n_index + 1 >= name_size)
		{	//If there is only one byte left in the name buffer
			name[n_index] = '\0';	//Terminate it
			value[0] = '\0';		//Empty the value buffer
			return 2;	//Error:  Insufficient buffer size
		}
		if(isspace(this_char))
		{	//If the next character in the source string is whitespace
			if(!name_started)
			{	//If there hasn't been any non-whitespace read yet
				continue;	//This is leading whitespace, skip it
			}

			name_ended = 1;	//This marks the expected end of the entry name, and the next expected character is an equal sign
		}
		else if(this_char == '=')
		{	//If the next character in the source string is an equal sign
			if(!name_started)
			{	//If the config entry name hasn't been read yet
				name[0] = '\0';		//Empty the name buffer
				value[0] = '\0';	//Empty the value buffer
				return 3;	//Error:  Malformed entry
			}

			//If this point is reached, the config entry name was parsed, remove trailing whitespace
			while(isspace(name[n_index]) && (n_index > 0))
			{	//If the last character in the name buffer is a space and there's at least one character before it
				n_index--;	//Truncate the space character from the buffer
			}
			name[n_index] = '\0';	//Terminate the name buffer

			//Copy the remainder of the source string into the value buffer
			for(;string[s_index] != '\0' && isspace(string[s_index]);s_index++);	//Skip any whitespace after the equal sign
			while(string[s_index] != '\0')
			{	//Until the end of the source string is reached
				value[v_index] = string[s_index];	//Append the character to the value buffer
				v_index++;
				s_index++;
			}
			return 0;	//Return success
		}
		else
		{	//The next character is non whitespace and isn't an equal sign
			if(name_ended)
			{	//If there was already text and a white space before the equal sign
				name[0] = '\0';		//Empty the name buffer
				value[0] = '\0';	//Empty the value buffer
				return 3;	//Error:  Malformed entry
			}

			name_started = 1;	//Track that a non whitespace character was read
			name[n_index] = this_char;	//Append the character to the name buffer
			n_index++;
		}
	}

	//If this point is reached, there was no equal sign in the source string
	name[0] = '\0';		//Empty the name buffer
	value[0] = '\0';	//Empty the value buffer
	return 3;	//Error:  Malformed entry
}

int eof_validate_default_ini_entry(const char *entry, int silent)
{
	char name1[EOF_INI_LENGTH], name2[EOF_INI_LENGTH], value[EOF_INI_LENGTH];
	unsigned long ctr;
	int retval;

	retval = eof_parse_config_entry_name(name1, sizeof(name1), value, sizeof(value), entry);
	if(retval)
	{	//If the provided string's variable name couldn't be parsed
		if(retval == 3)
		{
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Error adding default INI entry (malformed):  %s", entry);
			eof_log(eof_log_string, 1);
		}
		else
		{
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Error code %d adding default INI entry:  %s", retval, entry);
			eof_log(eof_log_string, 1);
		}
		if(!silent)
		{	//If errors are to be displayed to the user
			allegro_message("%s", eof_log_string);
		}
		return 1;	//Return input string error
	}

	//Compare with all existing default INI entries to ensure the variable isn't already defined
	for(ctr = 0; ctr < eof_default_ini_settings; ctr++)
	{	//For each already defined INI entry
		if(eof_parse_config_entry_name(name2, sizeof(name2), value, sizeof(value), eof_default_ini_setting[ctr]))
		{	//If the entry's variable name couldn't be parsed
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Error parsing existing default INI entry:  %s", eof_default_ini_setting[ctr]);
			eof_log(eof_log_string, 1);
			if(!silent)
			{	//If errors are to be displayed to the user
				allegro_message("%s", eof_log_string);
			}
			return 2;	//Return logic error
		}

		if(!ustricmp(name1, name2))
		{	//If the existing entry's variable name is the same as the provided string's variable name
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Skipped adding default INI entry because it is already defined:  %s", entry);
			eof_log(eof_log_string, 1);
			if(!silent)
			{	//If errors are to be displayed to the user
				allegro_message("%s", eof_log_string);
			}
			return 3;	//Return entry already defined
		}
	}

	if(eof_default_ini_settings >= EOF_MAX_INI_SETTINGS)
	{	//If the default INI settings array can't store another entry
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Skipped adding default INI entry because the maximum number of entries are already defined:  %s", entry);
		eof_log(eof_log_string, 1);
		if(!silent)
		{	//If errors are to be displayed to the user
			allegro_message("%s", eof_log_string);
		}
		return 4;
	}

	return 0;	//Valid
}

int eof_default_ini_add(const char *entry, int silent)
{
	int retval;

	retval = eof_validate_default_ini_entry(entry, silent);
	if(retval)
	{	//If the text string failed validation
		return retval;
	}

	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tAdded default INI entry:  %s", entry);
	eof_log(eof_log_string, 1);
	(void) ustrncpy(eof_default_ini_setting[eof_default_ini_settings], entry, EOF_INI_LENGTH - 1);
	eof_default_ini_settings++;

	return 0;	//Return success
}
