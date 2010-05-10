#include <allegro.h>
#include "main.h"
#include "gui.h"

static char* split_around_tab(const char *s, char **tok1, char **tok2)
{
	char *buf, *last;
	char tmp[16];

	buf = ustrdup(s);
	*tok1 = ustrtok_r(buf, uconvert_ascii("\t", tmp), &last);
	*tok2 = ustrtok_r(NULL, empty_string, &last);

	return buf;
}

void eof_draw_menu(int x, int y, int w, int h)
{
	rectfill(screen, x, y, w, h, makecol(224, 224, 224));
}

void eof_draw_menu_item(MENU * m, int x, int y, int w, int h, int bar, int sel)
{
	int fg, bg;
	char *buf, *tok1, *tok2;
	int my;

	if(m->flags & D_DISABLED)
	{
		if(sel)
		{
			fg = gui_mg_color;
			bg = makecol(0, 64, 192);
		}
		else
		{
			fg = makecol(160, 160, 160);
			bg = makecol(224, 224, 224);
		}
	}
	else
	{
		if(sel)
		{
			fg = eof_color_white;
			bg = makecol(0, 64, 192);
		}
		else
		{
			fg = makecol(64, 64, 64);
			bg = makecol(224, 224, 224);
		}
	}

	rectfill(screen, x, y, x+w-1, y+text_height(font)+3, bg);

	if(ugetc(m->text))
	{
		buf = split_around_tab(m->text, &tok1, &tok2);
		gui_textout_ex(screen, tok1, x+8, y+1, fg, bg, FALSE);
		if(tok2)
		{
			gui_textout_ex(screen, tok2, x+w-gui_strlen(tok2)-10, y+1, fg, bg, FALSE);
		}

		if((m->child) && (!bar))
		{
			my = y + text_height(font)/2;
			hline(screen, x+w-8, my+1, x+w-4, fg);
			hline(screen, x+w-8, my+0, x+w-5, fg);
			hline(screen, x+w-8, my-1, x+w-6, fg);
			hline(screen, x+w-8, my-2, x+w-7, fg);
			putpixel(screen, x+w-8, my-3, fg);
			hline(screen, x+w-8, my+2, x+w-5, fg);
			hline(screen, x+w-8, my+3, x+w-6, fg);
			hline(screen, x+w-8, my+4, x+w-7, fg);
			putpixel(screen, x+w-8, my+5, fg);
		}

		free(buf);
	}
   else
   {
		hline(screen, x, y+text_height(font)/2+2, x+w, fg);
	}

	if(m->flags & D_SELECTED)
	{
		line(screen, x+1, y+text_height(font)/2+1, x+3, y+text_height(font)+1, fg);
		line(screen, x+3, y+text_height(font)+1, x+6, y+2, fg);
	}
}

void eof_gui_initialize(void)
{
//	gui_menu_draw_menu = eof_draw_menu;
//	gui_menu_draw_menu_item = eof_draw_menu_item;
}
