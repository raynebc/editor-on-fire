#include <allegro.h>
#include "../agup/agup.h"
#include "../main.h"
#include "../dialog.h"
#include "../utility.h"
#include "help.h"

MENU eof_help_menu[] =
{
    {"&Manual", eof_menu_help_manual, NULL, 0, NULL},
    {"&Tutorial", eof_menu_help_tutorial, NULL, 0, NULL},
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
   { d_agup_textbox_proc,   8,  32,  624, 412 - 8,  2,   23,  0,    0,      0,   0,   eof_help_text, NULL, NULL },
   { d_agup_button_proc, 8,  444, 624,  28, 2,   23,  '\r',    D_EXIT, 0,   0,   "Okay",               NULL, NULL },
   { NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

int eof_menu_help_keys(void)
{
	PACKFILE * fp;
	
	fp = pack_fopen("keys.txt", "r");
	pack_fread(eof_help_text, file_size_ex("keys.txt"), fp);
	pack_fclose(fp);
	
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
	return 1;
}

int eof_menu_help_manual(void)
{
	#ifdef ALLEGRO_WINDOWS
		eof_system("start manual\\index.htm");
	#else
		#ifdef ALLEGRO_MACOSX
			eof_system("open manual/index.htm");
		#else
			eof_system("xdg-open manual/index.htm");
		#endif
	#endif
	return 1;
}

int eof_menu_help_tutorial(void)
{
	#ifdef ALLEGRO_WINDOWS
		eof_system("start tutorial\\index.htm");
	#else
		#ifdef ALLEGRO_MACOSX
			eof_system("open tutorial/index.htm");
		#else
			eof_system("xdg-open tutorial/index.htm");
		#endif
	#endif
	return 1;
}

int eof_menu_help_about(void)
{
	eof_cursor_visible = 0;
	eof_pen_visible = 0;
	eof_render();
	alert(EOF_VERSION_STRING, NULL, EOF_COPYRIGHT_STRING, "Okay", NULL, KEY_ENTER, 0);
	eof_clear_input();
	eof_show_mouse(NULL);
	eof_cursor_visible = 1;
	eof_pen_visible = 1;
	return 1;
}
