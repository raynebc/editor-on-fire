#include <allegro.h>
#include "../agup/agup.h"
#include "../menu/main.h"

#ifdef USEMEMWATCH
#include "../memwatch.h"
#endif

DIALOG eof_main_dialog[] =
{
	/* (proc)         (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)           (dp2) (dp3) */
	{ d_agup_menu_proc, 0, 0, 1024, 8, 0, 0, 0, 0, 0, 0, eof_main_menu, NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};
