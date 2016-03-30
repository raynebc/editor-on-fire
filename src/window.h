#ifndef EOF_WINDOW_H
#define EOF_WINDOW_H

#include <allegro.h>

typedef struct
{

	int x, y;
	int w, h;
	BITMAP * screen;

} EOF_WINDOW;

EOF_WINDOW * eof_window_create(int x, int y, int w, int h, BITMAP * bp);
void eof_window_destroy(EOF_WINDOW * wp);

#endif
