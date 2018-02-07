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
	{"", NULL, NULL, 0, NULL},
	{"&Keys\tF1", eof_menu_help_keys, NULL, 0, NULL},
	{"", NULL, NULL, 0, NULL},
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
	eof_log_cwd();

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
	eof_log_cwd();

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
	eof_log_cwd();

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
	eof_log_cwd();

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
