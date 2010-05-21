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
	PACKFILE * fp;

	set_default_config();

	fp = pack_fopen(fn, "r");
	if(fp)
	{
		eof_av_delay = pack_igetl(fp);
		eof_buffer_size = pack_igetl(fp);
		pack_fread(eof_fof_executable_path, 1024, fp);
		ustrcpy(eof_fof_executable_name, get_filename(eof_fof_executable_path));
		pack_fread(eof_fof_songs_path, 1024, fp);
		eof_inverted_notes = pack_igetl(fp);
		if(eof_inverted_notes != 0 && eof_inverted_notes != 1)
		{
			eof_inverted_notes = 0;
		}
		eof_smooth_pos = pack_igetl(fp);
		if(eof_smooth_pos != 0 && eof_smooth_pos != 1)
		{
			eof_smooth_pos = 1;
		}
		eof_lefty_mode = pack_igetl(fp);
		if(eof_lefty_mode != 0 && eof_lefty_mode != 1)
		{
			eof_lefty_mode = 0;
		}
		eof_input_mode = pack_igetl(fp);
		if(eof_input_mode < 0 || eof_input_mode > 6)
		{
			eof_input_mode = EOF_INPUT_PIANO_ROLL;
		}
		eof_anchor_all_beats = pack_igetl(fp);
		eof_anchor_all_beats = 1;
		eof_windowed = pack_igetl(fp);
		if(eof_windowed != 0 && eof_windowed != 1)
		{
			eof_windowed = 1;
		}
		eof_disable_windows = pack_igetl(fp);
		if(eof_disable_windows != 0 && eof_disable_windows != 1)
		{
			eof_disable_windows = 0;
		}
		ncdfs_use_allegro = eof_disable_windows;
		eof_ogg_setting = pack_igetl(fp);
		if(eof_ogg_setting < 0 || eof_ogg_setting >= 6)
		{
			eof_ogg_setting = 1;
		}
		eof_disable_vsync = pack_igetl(fp);
		if(eof_disable_vsync != 0 && eof_disable_vsync != 1)
		{
			eof_disable_vsync = 0;
		}
		pack_fread(eof_songs_path, 1024, fp);
		eof_controller_load_config(&eof_guitar, fp);
		eof_controller_read_button_names(&eof_guitar);
		eof_controller_load_config(&eof_drums, fp);
		eof_controller_read_button_names(&eof_drums);
		pack_fread(eof_last_eof_path, 1024, fp);
		pack_fread(eof_last_ogg_path, 1024, fp);
		eof_cpu_saver = pack_getc(fp);
		if(eof_cpu_saver < 0 || eof_cpu_saver > 10)
		{
			eof_cpu_saver = 0;
		}
		eof_screen_layout.mode = pack_getc(fp);
		if(eof_screen_layout.mode < 0 || eof_screen_layout.mode > 2)
		{
			eof_screen_layout.mode = 0;
		}
		eof_supports_mp3 = pack_getc(fp);
		eof_note_auto_adjust = pack_igetl(fp);
		pack_fread(eof_last_frettist, 256, fp);
		eof_soft_cursor = pack_getc(fp);
		eof_desktop = pack_getc(fp);

		/* edit settings */
		eof_snap_mode = pack_igetl(fp);
		if(eof_snap_mode < 0 || eof_snap_mode > EOF_SNAP_CUSTOM)
		{
			eof_snap_mode = 0;
		}
		eof_snap_interval = pack_igetl(fp);
		if(eof_snap_interval < 0 || eof_snap_interval > 15)
		{
			eof_snap_interval = 1;
		}
		eof_zoom_3d = pack_igetl(fp);
		if(eof_zoom_3d < 2 || eof_zoom_3d > 5)
		{
			eof_zoom_3d = 5;
		}
		eof_hopo_view = pack_igetl(fp);
		if(eof_hopo_view < 0 || eof_hopo_view > 2)
		{
			eof_hopo_view = EOF_HOPO_RF;
		}
		eof_guitar.delay = pack_igetl(fp);
		eof_drums.delay = pack_igetl(fp);
		pack_fclose(fp);
	}
}

void eof_save_config(char * fn)
{
	PACKFILE * fp;

	fp = pack_fopen(fn, "w");
	if(fp)
	{
		pack_iputl(eof_av_delay, fp);
		pack_iputl(eof_buffer_size, fp);
		pack_fwrite(eof_fof_executable_path, 1024, fp);
		pack_fwrite(eof_fof_songs_path, 1024, fp);
		pack_iputl(eof_inverted_notes, fp);
		pack_iputl(eof_smooth_pos, fp);
		pack_iputl(eof_lefty_mode, fp);
		pack_iputl(eof_input_mode, fp);
		pack_iputl(eof_anchor_all_beats, fp);
		pack_iputl(eof_windowed, fp);
		pack_iputl(eof_disable_windows, fp);
		pack_iputl(eof_ogg_setting, fp);
		pack_iputl(eof_disable_vsync, fp);
		pack_fwrite(eof_songs_path, 1024, fp);
		eof_controller_save_config(&eof_guitar, fp);
		eof_controller_save_config(&eof_drums, fp);
		pack_fwrite(eof_last_eof_path, 1024, fp);
		pack_fwrite(eof_last_ogg_path, 1024, fp);
		pack_putc(eof_cpu_saver, fp);
		pack_putc(eof_screen_layout.mode, fp);
		pack_putc(eof_supports_mp3, fp);
		pack_iputl(eof_note_auto_adjust, fp);
		pack_fwrite(eof_last_frettist, 256, fp);
		pack_putc(eof_soft_cursor, fp);
		pack_putc(eof_desktop, fp);

		/* edit settings */
		pack_iputl(eof_snap_mode, fp);
		pack_iputl(eof_snap_interval, fp);
		pack_iputl(eof_zoom_3d, fp);
		pack_iputl(eof_hopo_view, fp);

		pack_iputl(eof_guitar.delay, fp);
		pack_iputl(eof_drums.delay, fp);

		pack_fclose(fp);
	}
}

