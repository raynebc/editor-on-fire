#include <allegro.h>
#include "window.h"
#include "main.h"	//For logging

EOF_WINDOW * eof_window_create(int x, int y, int w, int h, BITMAP * bp)
{
 	eof_log("eof_window_create() entered");

	EOF_WINDOW * wp;

	wp = malloc(sizeof(EOF_WINDOW));
	wp->x = x;
	wp->y = y;
	wp->w = w;
	wp->h = h;
	wp->screen = create_sub_bitmap(bp, x, y, w, h);
	return wp;
}

void eof_window_destroy(EOF_WINDOW * wp)
{
 	eof_log("eof_window_destroy() entered");

	destroy_bitmap(wp->screen);
	free(wp);
}
