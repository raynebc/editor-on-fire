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
EOF_WINDOW *eof_coordinates_identify_window(int x, int y, char *name);
	//Inspects the coordinates of each of the standard EOF_WINDOW instances (piano roll, info panel, 3D preview) to identify within which, if any, the specified screen coordinates are
	//If the coordinates are in a particular window, that window's pointer is returned and the window's name is written to *name
	//Returns NULL if a corresponding window instance is not found, or upon error

#endif
