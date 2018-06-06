#include <allegro.h>
#include "window.h"
#include "main.h"	//For logging

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

EOF_WINDOW * eof_window_create(int x, int y, int w, int h, BITMAP * bp)
{
	EOF_WINDOW * wp = NULL;
	if(bp == NULL)
		return NULL;

 	eof_log("eof_window_create() entered", 1);

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

EOF_WINDOW *eof_coordinates_identify_window(int x, int y, char *name)
{
	EOF_WINDOW *window_list[4] = {eof_window_editor, eof_window_3d, eof_window_notes, eof_window_info};	//Order the info window last because it takes the entire bottom of the program window
	char *window_name[4] = {"eof_window_editor", "eof_window_3d", "eof_window_notes", "eof_window_info"};
	unsigned long ctr;

	if(!name)
		return NULL;	//Invalid parameter

	for(ctr = 0; ctr < 4; ctr++)
	{	//For each of the windows being checked
		if(window_list[ctr] == NULL)
			return NULL;	//Logic error

		if((x >= window_list[ctr]->x) && (x <= window_list[ctr]->x + window_list[ctr]->w))
		{	//If the x coordinate is within the boundaries of this window
			if((y >= window_list[ctr]->y) && (y <= window_list[ctr]->y + window_list[ctr]->h))
			{	//If the y coordinate is within the boundaries of this window
				strcpy(name, window_name[ctr]);
				return window_list[ctr];
			}
		}
	}

	return NULL;	//Window not found
}
