#include <allegro.h>
#include "window.h"
#include "main.h"	//For logging

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

EOF_WINDOW * eof_window_create(int x, int y, int w, int h, BITMAP * bp)
{
 	eof_log("eof_window_create() entered", 1);

	EOF_WINDOW * wp = NULL;
	if(bp == NULL)
		return NULL;

	wp = malloc(sizeof(EOF_WINDOW));
	if(wp)
	{
		wp->x = x;
		wp->y = y;
		wp->w = w;
		wp->h = h;
		wp->screen = create_sub_bitmap(bp, x, y, w, h);
	}
	return wp;
}

void eof_window_destroy(EOF_WINDOW * wp)
{
 	eof_log("eof_window_destroy() entered", 1);

	if(wp)
	{
		destroy_bitmap(wp->screen);
		free(wp);
	}
}
