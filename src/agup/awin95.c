/* awin95.c
 * 
 * This file is part of the Allegro GUI Un-uglification Project.
 * It emulates the default look of the Win95 widget set.
 * 
 * Widgets created by Robert J Ohannessian.
 * 
 * Window frames adapted from DIME by Sven Sandberg, by Peter Wang.
 *
 * Many changes by David A. Capello.
 */


#include <allegro.h>
#include "awin95.h"
#include "agupitrn.h"


/*----------------------------------------------------------------------*/
/* Recommend fg/bg colors						*/
/*----------------------------------------------------------------------*/


int awin95_fg_color;
int awin95_bg_color;
int awin95_mg_color;


/*----------------------------------------------------------------------*/
/* Helpers              						*/
/*----------------------------------------------------------------------*/


static int white, highlight, normal, gray, black;
static int hshadow, nshadow, blue;

static BITMAP *checked_bmp = NULL;


#define REDRAW(d)						\
do {								\
    scare_mouse_area(d->x, d->y, d->x + d->w, d->y + d->h);	\
    object_message(d, MSG_DRAW, 0);				\
    unscare_mouse();						\
} while (0)


static void draw_base(BITMAP *bmp, DIALOG *d)
{
    rectfill(bmp, d->x, d->y, d->x+d->w-1, d->y+d->h-1, normal);
}


static BITMAP *generate_bitmap(AL_CONST char *templat[], int w, int h)
{
    BITMAP *bmp = create_bitmap(w, h);
    int x, y;

    for (y = 0; y < h; y++) for (x = 0; x < w; x++) 
	switch (templat[y][x]) {
	    case '.': putpixel(bmp, x, y, white); break;
	    case 'X': putpixel(bmp, x, y, black); break;
	    case 'x': putpixel(bmp, x, y, nshadow); break;
	    case 'a': putpixel(bmp, x, y, hshadow); break;
	    case ' ': putpixel(bmp, x, y, bitmap_mask_color(bmp)); break;
	}
    
    return bmp;
}


static volatile int awin95_time_toggle = 0;

static void awin95_timer(void)
{
    awin95_time_toggle ^= 1;
}
END_OF_STATIC_FUNCTION(awin95_timer);


/*----------------------------------------------------------------------*/
/* Win95-lookalike procs							*/
/*----------------------------------------------------------------------*/


static INLINE void rectedge(BITMAP * bmp, int x1, int y1, int x2, int y2, int fg, int bg)
{
    hline(bmp, x1, y1, x2 - 1, fg);
    vline(bmp, x1, y1 + 1, y2 - 1, fg);
    hline(bmp, x1, y2, x2, bg);
    vline(bmp, x2, y1, y2 - 1, bg);
}


static INLINE void dotted_rect(int x1, int y1, int x2, int y2, int fg, int bg)
{
    BITMAP *bmp = gui_get_screen();
    int x = ((x1 + y1) & 1) ? 1 : 0;
    int c;

    /* two loops to avoid bank switches */
    for (c = x1; c <= x2; c++)
	putpixel(bmp, c, y1, (((c + y1) & 1) == x) ? fg : bg);
    for (c = x1; c <= x2; c++)
	putpixel(bmp, c, y2, (((c + y2) & 1) == x) ? fg : bg);

    for (c = y1 + 1; c < y2; c++) {
	putpixel(bmp, x1, c, (((c + x1) & 1) == x) ? fg : bg);
	putpixel(bmp, x2, c, (((c + x2) & 1) == x) ? fg : bg);
    }
}


static INLINE void dotted_rect_wh(int x, int y, int w, int h, int fg, int bg)
{
    dotted_rect(x, y, x+w-1, y+h-1, fg, bg);
}


static INLINE void rectfillwh(BITMAP *bmp, int x, int y, int w, int h, int color)
{
    rectfill(bmp, x, y, x+w-1, y+h-1, color);
}


static void win95_bevel(BITMAP *bmp, int x, int y, int w, int h, int state)
{
    switch (state) {
	case 0: /* normal */
	    hline(bmp, x, y, x+w-1, white);
	    vline(bmp, x, y, y+h-1, white);
	    hline(bmp, x+1, y+h-2, x+w-2, nshadow);
	    vline(bmp, x+w-2, y+1, y+h-2, nshadow);
	    hline(bmp, x, y+h-1, x+w-1, black);
	    vline(bmp, x+w-1, y, y+h-1, black);
	    break;

	case 1: /* highlight */
	    hline(bmp, x+1, y+1, x+w-2, white);
	    vline(bmp, x+1, y+1, y+h-2, white);
	    hline(bmp, x+2, y+h-2, x+w-2, nshadow);
	    vline(bmp, x+w-2, y+2, y+h-2, nshadow);
	    rect(bmp, x, y, x+w-1, y+h-1, black);
	    break;
	    
	case 2: /* pressed */
	    rect(bmp, x, y, x+w-1, y+h-1, black);
	    rect(bmp, x+1, y+1, x+w-2, y+h-2, nshadow);
	    break;
	    
	case 3: /* highlight and pressed */
	    rect(bmp, x, y, x+w-1, y+h-1, black);
	    rect(bmp, x+1, y+1, x+w-2, y+h-2, nshadow);
	    break;

	case 4: /* focused for text box and slider */
	    rect(bmp, x, y, x+w-1, y+h-1, black);
	    hline(bmp, x+1, y+1, x+w-2, nshadow);
	    vline(bmp, x+1, y+1, y+h-2, nshadow);
	    hline(bmp, x+2, y+h-2, x+w-2, white);
	    vline(bmp, x+w-2, y+2, y+h-2, white);
	    break;
    }
}


static void win95_check_bevel(BITMAP *bmp, int x, int y, int w, int h, int state)
{
    hline(bmp, x, y, x+w-2, nshadow);
    vline(bmp, x, y, y+h-2, nshadow);
    hline(bmp, x+1,y+1,x+w-3, black);
    vline(bmp, x+1,y+1,y+h-3, black);
    hline(bmp, x+1, y+h-2, x+w-2, hshadow);
    vline(bmp, x+w-2, y+1, y+h-2, hshadow);
    hline(bmp, x,   y+h-1, x+w-1, white);
    vline(bmp, x+w-1, y,   y+h-1, white);

    /* pressed */
    if (state & 4)
	rectfillwh(bmp, x+2, y+2, w-4, h-4, nshadow);
    else
	rectfillwh(bmp, x+2, y+2, w-4, h-4, white);
}


static void win95_text_bevel(BITMAP *bmp, int x, int y, int w, int h)
{
    hline(bmp, x, y, x+w-2, nshadow);
    vline(bmp, x, y, y+h-2, nshadow);
    hline(bmp, x+1,y+1,x+w-3, black);
    vline(bmp, x+1,y+1,y+h-3, black);
    hline(bmp, x+1, y+h-2, x+w-2, hshadow);
    vline(bmp, x+w-2, y+1, y+h-2, hshadow);
    hline(bmp, x,   y+h-1, x+w-1, white);
    vline(bmp, x+w-1, y,   y+h-1, white);
}


static void win95_box(BITMAP *bmp, int x, int y, int w, int h, int pressed)
{
    if (!pressed) {
	rectedge(bmp, x,     y,     x + w - 1, y + h - 1, highlight, black);
	rectedge(bmp, x + 1, y + 1, x + w - 2, y + h - 2, white, gray);
	rectfill(bmp, x + 2, y + 2, x + w - 3, y + h - 3, normal);
    }
    else {
	rectedge(bmp, x,     y,     x + w - 1, y + h - 1, gray, white);
	rectedge(bmp, x + 1, y + 1, x + w - 2, y + h - 2, black, highlight);
	rectfill(bmp, x + 2, y + 2, x + w - 3, y + h - 3, normal);
    }
}


static void win95_menu_box(BITMAP *bmp, int x, int y, int w, int h, int state)
{
    rectfillwh(bmp, x+1, y+1, w-1, h-1, normal);
    win95_bevel(bmp, x, y, w, h, state);
}


static void win95_icon(BITMAP *bmp, int x, int y, int w, int h, int state)
{
    rectfillwh(bmp, x, y, w, h, normal);

    switch (state) {
	case 1: /* highligh */
	    hline(bmp, x, y, x + w - 2, white);
	    vline(bmp, x, y, y + h - 2, white);
	    hline(bmp, x, y + h - 1, x + w - 1, nshadow);
	    vline(bmp, x + w - 1, y, y + h - 1, nshadow);
	    break;
	case 2: /* pressed */
	case 3:
	    hline(bmp, x, y, x + w - 2, nshadow);
	    vline(bmp, x, y, y + h - 2, nshadow);
	    hline(bmp, x, y + h - 1, x + w - 1, white);
	    vline(bmp, x + w - 1, y, y + h - 1, white);
	    break;
    }
}


int d_awin95_box_proc(int msg, DIALOG *d, int c)
{
    (void)c;
    if (msg == MSG_DRAW)
	win95_box(gui_get_screen(), d->x, d->y, d->w, d->h, 0);
    return D_O_K;
}



int d_awin95_shadow_box_proc(int msg, DIALOG *d, int c)
{
    return d_awin95_box_proc(msg, d, c);
}



int d_awin95_button_proc(int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW) {
	BITMAP *bmp = gui_get_screen();
	int f = 0, s = 0;

	if (d->flags & D_SELECTED) {
	    s = 1;
	    rect(bmp, d->x, d->y, d->x + d->w - 1, d->y + d->h - 1, black);
	    rect(bmp, d->x + 1, d->y + 1, d->x + d->w - 2, d->y + d->h - 2, gray);
	}
	else {
	    if (d->flags & D_GOTFOCUS) {
		f = 1;
		rect(bmp, d->x, d->y, d->x + d->w - 1, d->y + d->h - 1, black);
	    }

	    rectedge(bmp, d->x + f, d->y + f, d->x + d->w - f - 1, d->y + d->h - f - 1, white, black);
	    rectedge(bmp, d->x + f + 1, d->y + f + 1, d->x + d->w - f - 2, d->y + d->h - f - 2, highlight, gray);
	}

	rectfill(bmp, d->x + f + 2, d->y + f + 2, d->x + d->w - f - 3, d->y + d->h - f - 3, normal);

	if (d->dp) {
	    gui_textout_ex(bmp, (char *)d->dp,
			d->x + d->w / 2 + s,
			d->y + d->h / 2 - text_height(font) / 2 + s,
			(d->flags & D_DISABLED) ? gray : black, -1, TRUE);
	}

	if (d->flags & D_GOTFOCUS)
	    dotted_rect(d->x + 4, d->y + 4, d->x + d->w - 5, d->y + d->h - 5, black, normal);

	return D_O_K;
    }

    return d_button_proc(msg, d, c);
}


int d_awin95_push_proc(int msg, DIALOG *d, int c)
{
    int ret = D_O_K;

    d->flags |= D_EXIT;

    ret |= d_awin95_button_proc(msg, d, c);

    if (ret & D_CLOSE) {
	ret &= ~D_CLOSE;
	REDRAW(d);

	if (d->dp3)
	    ret |= ((int (*)(DIALOG *))d->dp3)(d);
    }

    return ret;
}


int d_awin95_check_proc(int msg, DIALOG *d, int c)
{
    if (msg == MSG_LRELEASE || msg == MSG_LPRESS)
	d->flags |= D_DIRTY;

    if (msg == MSG_DRAW) {
	BITMAP *bmp = gui_get_screen();
	int x = 0;
	if ((d->flags & D_GOTMOUSE) && (mouse_b & 1))
	    x |= 4;
	if (d->flags & D_SELECTED)
	    x |= 2;
	if (d->flags & D_GOTFOCUS)
	    x |= 1;

	draw_base(bmp, d);
	win95_check_bevel(bmp, d->x+2, d->y+d->h/2-6, 13, 13, x);

	/* Draw X */
	if (x & 2)
	    draw_sprite(bmp, checked_bmp, d->x+4, d->y+d->h/2-4);

	if (d->dp) {
	    gui_textout_ex(bmp, (char *)d->dp, d->x+18, d->y+d->h/2-text_height(font)/2,
			(d->flags & D_DISABLED) ? gray : black, -1, FALSE);
	}

	if (x & 1)
//	    dotted_rect_wh(d->x+16, d->y+d->h/2-text_height(font)/2 - 2, text_length(font, (char *)d->dp) + 4, text_height(font) + 4, black, normal);
	    dotted_rect_wh(d->x+16, d->y+d->h/2-text_height(font)/2, text_length(font, (char *)d->dp) + 4, text_height(font), black, normal);

	return D_O_K;
    }

    return d_button_proc(msg, d, c);
}


static AL_CONST char *checked[] =
{
    /* 9 x 9 */
    "         ",
    "       X ",
    "      XX ",
    " X   XXX ",
    " XX XXX  ",
    " XXXXX   ",
    "  XXX    ",
    "   X     ",
    "         "
};


static AL_CONST char *radio_down[] =
{
    /* 12 x 12 */
    "    xxxx    ",
    "  xxXXXXxx  ",
    " xXX....XX. ",
    " xX......a. ",
    "xX...XX...a.",
    "xX..XXXX..a.",
    "xX..XXXX..a.",
    "xX...XX...a.",
    " xX......a. ",
    " xaa....aa. ",
    "  ..aaaa..  ",
    "    ....    "
};


static AL_CONST char *radio_up[] =
{
    /* 12 x 12 */
    "    xxxx    ",
    "  xxXXXXxx  ",
    " xXX....XX. ",
    " xX......a. ",
    "xX........a.",
    "xX........a.",
    "xX........a.",
    "xX........a.",
    " xX......a. ",
    " xaa....aa. ",
    "  ..aaaa..  ",
    "    ....    "
};


static AL_CONST char *radio_pressed[] =
{
    /* 12 x 12 */
    "    xxxx    ",
    "  xxXXXXxx  ",
    " xXXaaaaXX. ",
    " xXaaaaaaa. ",
    "xXaaaXXaaaa.",
    "xXaaXXXXaaa.",
    "xXaaXXXXaaa.",
    "xXaaaXXaaaa.",
    " xXaaaaaaa. ",
    " xaaaaaaaa. ",
    "  ..aaaa..  ",
    "    ....    "
};

static AL_CONST char *scroll_arrow[] =
{
    /* 5 x 3 */
    "  X  ",
    " XXX ",
    "XXXXX"
};


static BITMAP *radio_down_bmp, *radio_up_bmp, *radio_pressed_bmp, *scroll_arrow_bmp;


/* d1 used for group number. */
int d_awin95_radio_proc(int msg, DIALOG *d, int c)
{
    if (msg == MSG_LRELEASE || msg == MSG_LPRESS)
	d->flags |= D_DIRTY;
    if (msg == MSG_DRAW) {
	BITMAP *bmp = gui_get_screen();
	draw_base(bmp, d);
	draw_sprite(bmp,
		    (d->flags & D_SELECTED)
		    ? (((d->flags & D_GOTMOUSE) && (mouse_b & 1))
		       ? radio_pressed_bmp : radio_down_bmp)
		    : radio_up_bmp, d->x+3, d->y+d->h/2-6);

	if (d->flags & D_GOTFOCUS)
//	    dotted_rect_wh(d->x+16, d->y+d->h/2-text_height(font)/2 - 2, text_length(font, (char *)d->dp) + 4, text_height(font) + 4, black, normal);
	    dotted_rect_wh(d->x+16, d->y+d->h/2-text_height(font)/2, text_length(font, (char *)d->dp) + 4, text_height(font), black, normal);

	if (d->dp) {
	    gui_textout_ex(bmp, (char *)d->dp, d->x+18, d->y+d->h/2-text_height(font)/2,
			(d->flags & D_DISABLED) ? gray : black, -1, FALSE);
	}
	return D_O_K;
    }

    return d_radio_proc(msg, d, c);
}


int d_awin95_icon_proc(int msg, DIALOG *d, int c)
{
    if (msg == MSG_LRELEASE || msg == MSG_LPRESS)
	d->flags |= D_DIRTY;

    if (msg == MSG_DRAW) {
	BITMAP *bmp = gui_get_screen();
	BITMAP *img = (BITMAP *)d->dp;
	int x = 0;
	
	if (d->flags & D_SELECTED)
	    x |= 2;
	if (d->flags & D_GOTFOCUS)
	    x |= 1;

	win95_icon(bmp, d->x, d->y, d->w, d->h, x);

	stretch_sprite(bmp, img, d->x+2, d->y+2, d->w-4, d->h-4);
	return D_O_K;
    }
    
    return d_awin95_button_proc(msg, d, c);
}


int d_awin95_edit_proc(int msg, DIALOG *d, int c)
{
    static int old_tick = 0;
    int fonth = text_height(font);

    if (old_tick != awin95_time_toggle) {
	old_tick = awin95_time_toggle;
	d->flags |= D_DIRTY;
    }
    if (msg == MSG_DRAW) {
	BITMAP *bmp = gui_get_screen();
	int l, x, b, f, p, w;
	int fg = (d->flags & D_DISABLED) ? hshadow : black;
	char *s = (char *)d->dp;
	char buf[16];

	agup_edit_adjust_position (d);

	l = ustrlen(s);
	/* set cursor pos */
	if (d->d2 >= l) {
	    d->d2 = l;
	    usetc(buf+usetc(buf, ' '), 0);
	    x = text_length(font, buf) + 2;
	}
	else
	{
	    x = 2;
    }
	
	b = 0;	  /* num of chars to be blitted */
	/* get the part of the string to be blitted */
	for (p = d->d2; p >= 0; p--) {
	    usetc(buf+usetc(buf, ugetat(s, p)), 0);
	    x += text_length(font, buf);
	    b++;
	    if (x > d->w-4)
		break;
	}

	/* see if length of text is too wide */
	if (x <= d->w-2) {
	    b = l; 
	    if(text_length(font, s) <= d->w - 4)
	    {
	    	p = 0;
    	}
	}
	else {
	    b--;
	    p = d->d2-b+1; 
	    b = d->d2; 
	}
	if(p < 0)
	{
		p = 0;
	}

	win95_text_bevel(bmp, d->x, d->y, d->w, fonth+6);
	rectfill(bmp, d->x+2, d->y+2, d->x+d->w-3, d->y+fonth+3, white);

	for (x = 4; p<=b; p++) {
	    f = ugetat(s, p);
	    usetc(buf+usetc(buf, (f) ? f : ' '), 0);
	    w = text_length(font, buf);
	    f = ((p == d->d2) && (d->flags & D_GOTFOCUS));
	    textout_ex(bmp, font, buf, d->x+x, d->y+4, fg, white);
	    if (f && awin95_time_toggle)
		vline(bmp, d->x+x-1, d->y+3, d->y+text_height(font)+3, black);
	    if ((x += w) + w > d->w - 4)
		break;
	}

        agup_edit_restore_position (d);

	return D_O_K;
    }

    if (msg == MSG_LOSTFOCUS || msg == MSG_LOSTMOUSE)
	return D_WANTFOCUS;

    return d_agup_adjusted_edit_proc(msg, d, c);
}


typedef char *(*getfuncptr)(int, int *);


static void win95_draw_scrollable_frame(DIALOG *d, int listsize, int offset, int height)
{
    BITMAP *bmp = gui_get_screen();
    int i, len;
    int xx, yy;

    /* draw frame */
    hline(bmp, d->x, d->y, d->x+d->w-1, nshadow);
    vline(bmp, d->x, d->y, d->y+d->h-1, nshadow);
    hline(bmp, d->x+1, d->y+1, d->x+d->w-2, black);
    vline(bmp, d->x+1, d->y+1, d->y+d->h-2, black);
    hline(bmp, d->x+2, d->y+d->h-2, d->x+d->w-2, normal);
    vline(bmp, d->x+d->w-2, d->y+2, d->y+d->h-3, normal);
    hline(bmp, d->x+1, d->y+d->h-1, d->x+d->w-1, white);
    vline(bmp, d->x+d->w-1, d->y+1, d->y+d->h-2, white);

    /* possibly draw scrollbar */
    if (listsize > height) {
	xx = d->x+d->w-11;
	yy = d->y;
	len = (((d->h-2) * offset) + listsize/2) / listsize;
	rectfillwh(bmp, xx-2, yy+2+11, 11, d->h-4-11, highlight);

	i = ((d->h-5) * height + listsize/2) / listsize;
	xx = d->x+d->w-11;
	yy = d->y+2;

	if (offset > 0) {
	    len = (((d->h-5) * offset) + listsize/2) / listsize;
	    rectfill(bmp, xx-2, yy, xx+10, yy+len+1, highlight);
	    yy += len;
	}
	if (yy+i < d->y+d->h-3) {
	    win95_box(bmp, xx-2, yy, 12, i, 0);
	    yy += i+1;
	}
	else
	    win95_box(bmp, xx-2, yy, 12, d->h-3-yy+d->y, 0);
    }
}


int d_awin95_list_proc(int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW) {
	BITMAP *bmp = gui_get_screen();
	int height, listsize, i, len, bar, x, y, w;
	int fg_color, fg, bg;
	char *sel = (char *)d->dp2;
	char s[1024];

	(*(getfuncptr)d->dp)(-1, &listsize);
	height = (d->h-4) / text_height(font);
	bar = (listsize > height);
	w = (bar ? d->w-14 : d->w-3);
	fg_color = (d->flags & D_DISABLED) ? awin95_mg_color : d->fg;

	/* draw box contents */
	for (i=0; i<height; i++) {
	    if (d->d2+i < listsize) {
		if (d->d2+i == d->d1) {
		    fg = white;
		    bg = blue;
		}
		else if ((sel) && (sel[d->d2+i])) {
		    fg = white;
		    bg = blue;
		}
		else {
		    fg = black;
		    bg = white;
		}
		usetc(s, 0);
		ustrncat(s, (*(getfuncptr)d->dp)(i+d->d2, NULL), sizeof(s)-ucwidth(0));
		x = d->x + 2;
		y = d->y + 2 + i*text_height(font);
		rectfill(bmp, x, y, x+7, y+text_height(font)-1, bg);
		x += 8;
		len = ustrlen(s);
		while (text_length(font, s) >= MAX(d->w - 1 - (bar ? 22 : 10), 1)) {
		    len--;
		    usetat(s, len, 0);
		}
		textout_ex(bmp, font, s, x, y, fg, bg);
		x += text_length(font, s);
		if (x <= d->x+w)
		    rectfill(bmp, x, y, d->x+w, y+text_height(font)-1, bg);
	    }
	    else {
		rectfill(bmp, d->x+2,  d->y+2+i*text_height(font),
			 d->x+w, d->y+1+(i+1)*text_height(font), white);
	    }
	}

	if (d->y+2+i*text_height(font) <= d->y+d->h-3)
	    rectfill(bmp, d->x+2, d->y+2+i*text_height(font),
		     d->x+w, d->y+d->h-3, white);

	/* draw frame, maybe with scrollbar */
	win95_draw_scrollable_frame(d, listsize, d->d2, height);

	return D_O_K;
    }


    return d_list_proc(msg, d, c);
}


int d_awin95_text_list_proc(int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW)
	return d_awin95_list_proc(msg, d, c);
    return d_text_list_proc(msg, d, c);
}


int d_awin95_textbox_proc(int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW) {
	int height, bar;
	int fg_color = (d->flags & D_DISABLED) ? awin95_mg_color : black;

	height = (d->h-8) / text_height(font);

	/* tell the object to sort of draw, but only calculate the listsize */
	_draw_textbox((char *)d->dp, &d->d1, 
		      0, /* DONT DRAW anything */
		      d->d2, !(d->flags & D_SELECTED), 8,
		      d->x, d->y, d->w, d->h,
		      (d->flags & D_DISABLED),
		      0, 0, 0);

	if (d->d1 > height) {
	    bar = 11;
	}
	else {
	    bar = 0;
	    d->d2 = 0;
	}

	/* now do the actual drawing */
	_draw_textbox((char *)d->dp, &d->d1, 1, d->d2,
		      !(d->flags & D_SELECTED), 8,
		      d->x, d->y, d->w-bar, d->h,
		      (d->flags & D_DISABLED),
		      fg_color, white, awin95_mg_color);

	/* draw the frame around */
	win95_draw_scrollable_frame(d, d->d1, d->d2, height);
	return D_O_K;
    }
    
    return d_textbox_proc(msg, d, c);
}


int d_awin95_slider_proc(int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW) {
	BITMAP *bmp = gui_get_screen();
	int vert = TRUE;    /* flag: is slider vertical? */
	int hh = 32;        /* handle height (width for horizontal sliders) */
	int slp;            /* slider position */
	int irange;
	int slx, sly, slh, slw;
	fixed slratio, slmax, slpos;

	/* check for slider direction */
	if (d->h < d->w)
	    vert = FALSE;

	irange = (vert) ? d->h : d->w;
	slmax = itofix(irange-hh);
	slratio = slmax / (d->d1);
	slpos = slratio * d->d2;
	slp = fixtoi(slpos);

	/* draw background */
	win95_box(bmp, d->x, d->y, d->w, d->h, 1);
	
	/* now draw the handle */
	if (vert) {
	    slx = d->x+2;
	    sly = d->y+2+(d->h)-(hh+slp);
	    slw = d->w-1-3;
	    slh = hh-1-3;
	} else {
	    slx = d->x+2+slp;
	    sly = d->y+2;
	    slw = hh-1-3;
	    slh = d->h-1-3;
	}
	    
	win95_box(bmp, slx, sly, slw, slh, 0);

	return D_O_K;
    }
    
    return d_slider_proc(msg, d, c);
}


/*----------------------------------------------------------------------*/
/* Menus                                                                */
/*----------------------------------------------------------------------*/


/* These menus don't look much like win95 menus.  Unfortunately, it's
   probably about as good as we can do without reimplementing menus
   for ourselves.  */


static AL_CONST char *menu_arrow[] =
{
    /* 8 x 7 */
    "XX      ",
    "XXXX    ",
    "XXXXXX  ",
    "XXXXXXXX",
    "XXXXXX  ",
    "XXXX    ",
    "XX      "
};


static BITMAP *menu_arrow_bmp;


static void win95_draw_menu(int x, int y, int w, int h)
{
    win95_menu_box(gui_get_screen(), x, y, w, h, 0);
}


static void win95_draw_menu_item(MENU *m, int x, int y, int w, int h, int bar, int sel)
{
    BITMAP *bmp = gui_get_screen();
    int fg, bg;
    int i, j;
    char buf[256], *tok;
    
    if (m->flags & D_DISABLED) {
	fg = nshadow;
	bg = normal;
    }
    else {
	fg = (sel && !bar) ? normal : black;
	bg = (sel && !bar) ? blue : normal;
    }

    rectfill(bmp, x, y, x+w-1, y+h-1, bg);

   if (bar && sel) {
	hline(bmp, x, y, x + w - 1, white);
	vline(bmp, x, y, y + h - 1, white);
	hline(bmp, x + 1, y + h - 1, x + w - 1, nshadow);
	vline(bmp, x + w - 1, y + 1, y + h - 1, nshadow);
    }

    if (ugetc(m->text)) {
	i = 0;
	j = ugetc(m->text);

	while ((j) && (j != '\t')) {
	    i += usetc(buf+i, j);
	    j = ugetc(m->text+i);
	}

	usetc(buf+i, 0);

	gui_textout_ex(bmp, buf, x+8, y+1, fg, bg, FALSE);

	if (j == '\t') {
	    tok = m->text+i + uwidth(m->text+i);
	    gui_textout_ex(bmp, tok, x+w-gui_strlen(tok)-10, y+1, fg, bg, FALSE);
	}

	if ((m->child) && (!bar))
	    draw_sprite(bmp, menu_arrow_bmp, x+w-12, y+(h-menu_arrow_bmp->h)/2);
    }
    else {
	hline(bmp, x+4, y+text_height(font)/2+2, x+w-4, nshadow);
	hline(bmp, x+4, y+text_height(font)/2+3, x+w-4, highlight);
    }

    if (m->flags & D_SELECTED) {
	line(bmp, x+1, y+text_height(font)/2+1, x+3, y+text_height(font)+1, fg);
	line(bmp, x+3, y+text_height(font)+1, x+6, y+2, fg);
    }
}


int d_awin95_menu_proc(int msg, DIALOG *d, int c)
{
    return d_menu_proc(msg, d, c);
}


/*----------------------------------------------------------------------*/
/* Window frames						 	*/
/*----------------------------------------------------------------------*/

enum {
    border_thickness = 3,
    internal_border_thickness = 3,
};


static void fill_textout(BITMAP *bmp, FONT *f, AL_CONST char *text,
			 int x, int y, int w, int fg, int bg)
{
    int text_w, text_h;
    int cl, ct, cr, cb;

    text_w = text_length(f, text);
    text_h = text_height(f);

    if (is_screen_bitmap(bmp))
	scare_mouse_area(x, y, x+w-1, y+text_h-1);

    cl = bmp->cl, ct = bmp->ct, cr = bmp->cr, cb = bmp->cb;
    set_clip_rect(bmp, x, y, x+w-1, y+text_h-1);

    textout_ex(bmp, f, text, x, y, fg, bg);
    if (text_w < w)
	rectfill(bmp, x + text_w, y, x+w-1, y+text_h-1, bg);

    set_clip_rect(bmp, cl, ct, cr, cb);

    if (is_screen_bitmap(bmp))
	unscare_mouse();
}


static void draw_window_title(DIALOG *d, AL_CONST char *text)
{
    BITMAP *bmp = gui_get_screen();
    int height = text_height(font);

    rectfill(bmp,
	     d->x + border_thickness + 1,
	     d->y + border_thickness + 1,
	     d->x + d->w - border_thickness - 2,
	     d->y + border_thickness + 1 + internal_border_thickness + height + internal_border_thickness - 1,
	     blue);
    fill_textout(bmp, font, text,
		 d->x + border_thickness + 1 + internal_border_thickness,
		 d->y + border_thickness + 1 + internal_border_thickness,
		 d->w - 2 * (border_thickness + 1 + internal_border_thickness),
		 white, blue);
    hline(bmp, 
	  d->x + border_thickness, d->y + border_thickness,
	  d->x + d->w - border_thickness - 2, 
	  nshadow);
    vline(bmp,
	  d->x + border_thickness,
	  d->y + border_thickness,
	  d->y + border_thickness + 2 * internal_border_thickness + height, 
	  nshadow);
    hline(bmp,
	  d->x + border_thickness,
	  d->y + border_thickness + 2 * internal_border_thickness + height + 1,
	  d->x + d->w - border_thickness - 1,
	  highlight);
    vline(bmp,
	  d->x + d->w - border_thickness - 1,
	  d->y + border_thickness,
	  d->y + border_thickness + 2 * internal_border_thickness + height,
	  highlight);
}


int d_awin95_window_proc(int msg, DIALOG *d, int c)
{
   (void)c;

   if (msg == MSG_DRAW) {
	win95_box(gui_get_screen(), d->x, d->y, d->w, d->h, 0);
	draw_window_title(d, d->dp ? (AL_CONST char *)d->dp : empty_string);
   }

   return D_O_K;
}


/*----------------------------------------------------------------------*/
/* Initialisation                                                       */
/*----------------------------------------------------------------------*/


void awin95_init(void)
{
    white     = makecol(255, 255, 255);
    highlight = makecol(223, 223, 223);
    normal    = makecol(227, 227, 227);
    gray      = makecol(127, 127, 127);
    black     = makecol(0, 0, 0);

    hshadow = makecol(0xa5, 0xa5, 0xa5);
    nshadow = makecol(0x95, 0x95, 0x95);
    blue = makecol(49, 106, 197);
    
    awin95_fg_color = black;
    awin95_bg_color = normal;
    awin95_mg_color = nshadow;

    radio_up_bmp = generate_bitmap(radio_up, 12, 12);
    radio_down_bmp = generate_bitmap(radio_down, 12, 12);
    radio_pressed_bmp = generate_bitmap(radio_pressed, 12, 12);
    checked_bmp = generate_bitmap(checked, 9, 9);
    scroll_arrow_bmp = generate_bitmap(scroll_arrow, 5, 3);

    menu_arrow_bmp = generate_bitmap(menu_arrow, 8, 7);
    gui_menu_draw_menu = win95_draw_menu;
    gui_menu_draw_menu_item = win95_draw_menu_item;

    LOCK_VARIABLE(awin95_time_toggle);
    LOCK_FUNCTION(awin95_timer);

    install_int(awin95_timer, 500);
}


void awin95_shutdown(void)
{
    destroy_bitmap(menu_arrow_bmp);
    gui_menu_draw_menu_item = NULL;
    gui_menu_draw_menu = NULL;

    destroy_bitmap(radio_down_bmp);
    destroy_bitmap(radio_up_bmp);

    remove_int(awin95_timer);
}


int
d_awin95_clear_proc (int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW)
    {
        clear_to_color (gui_get_screen(), awin95_bg_color);
        return D_O_K;
    }
    return d_clear_proc (msg, d, c);
}


/*----------------------------------------------------------------------*/
/* Theme                                                                */
/*----------------------------------------------------------------------*/

static struct AGUP_THEME the_theme =
{
    "Win95",
    &awin95_fg_color,
    &awin95_bg_color,
    &awin95_mg_color,
    awin95_init,
    awin95_shutdown,
    d_awin95_box_proc,
    d_awin95_shadow_box_proc,
    d_awin95_button_proc,
    d_awin95_push_proc,
    d_awin95_check_proc,
    d_awin95_radio_proc,
    d_awin95_icon_proc,
    d_awin95_edit_proc,
    d_awin95_list_proc,
    d_awin95_text_list_proc,
    d_awin95_textbox_proc,
    d_awin95_slider_proc,
    d_awin95_menu_proc,
    d_awin95_window_proc,
    d_awin95_clear_proc,
    d_agup_common_text_proc,
    d_agup_common_ctext_proc,
    d_agup_common_rtext_proc
};


AL_CONST struct AGUP_THEME *awin95_theme = &the_theme;
