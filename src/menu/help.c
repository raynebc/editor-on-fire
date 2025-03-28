#include <allegro.h>
#include "../agup/agup.h"
#include "../main.h"
#include "../dialog.h"
#include "../utility.h"
#include "help.h"

#ifdef USEMEMWATCH
#include "../memwatch.h"
#endif

MENU eof_help_menu[] =
{
	{"&Manual", eof_menu_help_manual, NULL, 0, NULL},
	{"&Tutorial", eof_menu_help_tutorial, NULL, 0, NULL},
	{"&Vocals Tutorial", eof_menu_help_vocals_tutorial, NULL, 0, NULL},
	{"&Pro Guitar Tutorial", eof_menu_help_pro_guitar_tutorial, NULL, 0, NULL},
	{"&Immerrock Tutorial", eof_menu_help_immerrock_tutorial, NULL, 0, NULL},
	{"", NULL, NULL, 0, NULL},
	{"Reset &Display\tShift+F5", eof_reset_display, NULL, 0, NULL},
	{"Reset audi&O", eof_reset_audio, NULL, 0, NULL},
	{"", NULL, NULL, 0, NULL},
	{"&Keys\tF1", eof_menu_help_keys, NULL, 0, NULL},
	{"&About", eof_menu_help_about, NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

DIALOG eof_help_dialog[] =
{
	/* (proc)         (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)           (dp2) (dp3) */
	{ d_agup_window_proc,    0,  0,  640, 480, 2,   23,  0,    0,      0,   0,   "EOF Help",               NULL, NULL },
	{ d_agup_text_proc,   288,  8,  128, 8,  2,   23,  0,    0,      0,   0,   "", NULL, NULL },
	{ d_agup_textbox_proc,   8,  32,  624, 412 - 8,  2,   23,  0,    0,      0,   0,   "", NULL, NULL },
	{ d_agup_button_proc, 8,  444, 624,  28, 2,   23,  '\r',    D_EXIT, 0,   0,   "Okay",               NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

int eof_menu_help_keys(void)
{
	if(eof_validate_temp_folder())
	{	//Ensure the correct working directory and presence of the temporary folder
		eof_log("\tCould not validate working directory and temp folder", 1);
		eof_log_cwd();
		return 1;
	}
	eof_help_text = eof_buffer_file("keys.txt", 1);	//Buffer the help file into memory, appending a NULL terminator
	if(eof_help_text == NULL)
	{	//Could not buffer file
		allegro_message("Error reading keys.txt");
		return 1;
	}

	eof_help_dialog[2].dp = eof_help_text;	//Use the buffered file for the text box
	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	eof_color_dialog(eof_help_dialog, gui_fg_color, gui_bg_color);
	centre_dialog(eof_help_dialog);
	if(eof_popup_dialog(eof_help_dialog, 0) == 1)
	{
	}
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	free(eof_help_text);
	eof_help_text = NULL;
	return 1;
}

int eof_menu_help_manual(void)
{
	if(eof_validate_temp_folder())
	{	//Ensure the correct working directory and presence of the temporary folder
		eof_log("\tCould not validate working directory and temp folder", 1);
		eof_log_cwd();
		return 1;
	}

	#ifdef ALLEGRO_WINDOWS
		(void) eof_system("start manual\\index.htm");
	#else
		#ifdef ALLEGRO_MACOSX
			(void) eof_system("open manual/index.htm");
		#else
			(void) eof_system("xdg-open manual/index.htm");
		#endif
	#endif
	return 1;
}

int eof_menu_help_tutorial(void)
{
	if(eof_validate_temp_folder())
	{	//Ensure the correct working directory and presence of the temporary folder
		eof_log("\tCould not validate working directory and temp folder", 1);
		eof_log_cwd();
		return 1;
	}

	#ifdef ALLEGRO_WINDOWS
		(void) eof_system("start tutorial\\index.htm");
	#else
		#ifdef ALLEGRO_MACOSX
			(void) eof_system("open tutorial/index.htm");
		#else
			(void) eof_system("xdg-open tutorial/index.htm");
		#endif
	#endif
	return 1;
}

int eof_menu_help_vocals_tutorial(void)
{
	if(eof_validate_temp_folder())
	{	//Ensure the correct working directory and presence of the temporary folder
		eof_log("\tCould not validate working directory and temp folder", 1);
		eof_log_cwd();
		return 1;
	}

	#ifdef ALLEGRO_WINDOWS
		(void) eof_system("start vocaltutorial\\index.htm");
	#else
		#ifdef ALLEGRO_MACOSX
			(void) eof_system("open vocaltutorial/index.htm");
		#else
			(void) eof_system("xdg-open vocaltutorial/index.htm");
		#endif
	#endif
	return 1;
}

int eof_menu_help_pro_guitar_tutorial(void)
{
	if(eof_validate_temp_folder())
	{	//Ensure the correct working directory and presence of the temporary folder
		eof_log("\tCould not validate working directory and temp folder", 1);
		eof_log_cwd();
		return 1;
	}

	#ifdef ALLEGRO_WINDOWS
		(void) eof_system("start proguitartutorial\\index.htm");
	#else
		#ifdef ALLEGRO_MACOSX
			(void) eof_system("open proguitartutorial/index.htm");
		#else
			(void) eof_system("xdg-open proguitartutorial/index.htm");
		#endif
	#endif
	return 1;
}

int eof_menu_help_immerrock_tutorial(void)
{
	if(eof_validate_temp_folder())
	{	//Ensure the correct working directory and presence of the temporary folder
		eof_log("\tCould not validate working directory and temp folder", 1);
		eof_log_cwd();
		return 1;
	}

	#ifdef ALLEGRO_WINDOWS
		(void) eof_system("start immerrocktutorial\\index.htm");
	#else
		#ifdef ALLEGRO_MACOSX
			(void) eof_system("open immerrocktutorial/index.htm");
		#else
			(void) eof_system("xdg-open immerrocktutorial/index.htm");
		#endif
	#endif
	return 1;
}

int eof_menu_help_about(void)
{
	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	(void) alert(EOF_VERSION_STRING, NULL, EOF_COPYRIGHT_STRING, "Okay", NULL, KEY_ENTER, 0);
	eof_clear_input();
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	return 1;
}

int eof_reset_display(void)
{
	(void) eof_set_display_mode(eof_screen_width, eof_screen_height);

	//Update coordinate related items
	eof_scale_fretboard(0);			//Recalculate the 2D screen positioning based on the current track
	eof_set_2D_lane_positions(0);	//Update ychart[] by force just in case the display window size was changed
	eof_set_3D_lane_positions(0);	//Update xchart[] by force just in case the display window size was changed

	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	eof_show_mouse(screen);
	eof_render();

	return D_O_K;
}

int eof_reset_audio(void)
{
	remove_sound();

	if(install_sound(DIGI_AUTODETECT, MIDI_AUTODETECT, NULL))
	{	//If Allegro failed to initialize the sound AND midi
		if(install_sound(DIGI_AUTODETECT, MIDI_NONE, NULL))
		{
			allegro_message("Can't set up sound!  Error: %s",allegro_error);
			return 0;
		}
		eof_midi_initialized = 0;	//Couldn't set up MIDI
	}
	else
	{
		install_timer();	//Needed to use midi_out()
		eof_midi_initialized = 1;
	}

	eof_close_menu = 1;			//Force the main menu to close, as this function had a tendency to get hung in the menu logic when activated by keyboard
	return D_O_K;
}
