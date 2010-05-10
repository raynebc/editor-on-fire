#include <allegro.h>
#include "window.h"

EOF_WINDOW * eof_window_create(int x, int y, int w, int h, BITMAP * bp)
{
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
	destroy_bitmap(wp->screen);
	free(wp);
}
