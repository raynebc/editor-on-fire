/* agtk.c
 *
 * This file is part of the Allegro GUI Un-uglification Project.
 * It emulates the default look of the GTK widget set.
 *
 * Peter Wang <tjaden@users.sourceforge.net>
 */


#include <allegro.h>
#include "agtk.h"
#include "agupitrn.h"

#ifdef USEMEMWATCH
#include "../memwatch.h"
#endif


/*----------------------------------------------------------------------*/
/* Recommend fg/bg colors						*/
/*----------------------------------------------------------------------*/


int agtk_fg_color;
int agtk_bg_color;
int agtk_mg_color;


/*----------------------------------------------------------------------*/
/* Helpers              						*/
/*----------------------------------------------------------------------*/


static int white, highlight, normal, pressed, hshadow, nshadow, black;
static int blue, yellow;


#define REDRAW(d)						\
do {								\
    scare_mouse_area(d->x, d->y, d->x + d->w, d->y + d->h);	\
    object_message(d, MSG_DRAW, 0);				\
    unscare_mouse();						\
} while (0)


static void draw_base(BITMAP *bmp, DIALOG *d)
{
    if (d->flags & D_GOTFOCUS) {
	rectfill(bmp, d->x+1, d->y+1, d->x+d->w-2, d->y+d->h-2, highlight);
	rect(bmp, d->x, d->y, d->x+d->w-1, d->y+d->h-1, black);
    }
    else
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


/*----------------------------------------------------------------------*/
/* GTK-lookalike procs							*/
/*----------------------------------------------------------------------*/


static INLINE void rectfillwh(BITMAP *bmp, int x, int y, int w, int h, int color)
{
    rectfill(bmp, x, y, x+w-1, y+h-1, color);
}


static void gtk_bevel(BITMAP *bmp, int x, int y, int w, int h, int state)
{
    switch (state) {
	case 0: /* normal */
	    hline(bmp, x, y, x+w-1, white);
	    vline(bmp, x, y, y+h-1, white);
	    hline(bmp, x+2, y+h-2, x+w-2, nshadow);
	    vline(bmp, x+w-2, y+2, y+h-2, nshadow);
	    hline(bmp, x, y+h-1, x+w-1, black);
	    vline(bmp, x+w-1, y, y+h-1, black);
	    break;

	case 1: /* highlight */
	    hline(bmp, x, y, x+w-1, white);
	    vline(bmp, x, y, y+h-1, white);
	    hline(bmp, x+2, y+h-2, x+w-2, hshadow);
	    vline(bmp, x+w-2, y+2, y+h-2, hshadow);
	    hline(bmp, x, y+h-1, x+w-1, black);
	    vline(bmp, x+w-1, y, y+h-1, black);
	    break;

	case 2: /* pressed */
	    hline(bmp, x, y, x+w-1, nshadow);
	    vline(bmp, x, y, y+h-1, nshadow);
	    hline(bmp, x+1, y+1, x+w-2, black);
	    vline(bmp, x+1, y+1, y+h-2, black);
	    hline(bmp, x+1, y+h-1, x+w-1, white);
	    vline(bmp, x+w-1, y+1, y+h-2, white);
	    hline(bmp, x+2, y+h-2, x+w-2, pressed);
	    vline(bmp, x+w-2, y+2, y+h-2, pressed);
	    break;

	case 3: /* focused for text box and slider */
	    rect(bmp, x, y, x+w-1, y+h-1, black);
	    hline(bmp, x+1, y+1, x+w-2, nshadow);
	    vline(bmp, x+1, y+1, y+h-2, nshadow);
	    hline(bmp, x+2, y+h-2, x+w-2, white);
	    vline(bmp, x+w-2, y+2, y+h-2, white);
	    break;
    }
}


static void gtk_box(BITMAP *bmp, int x, int y, int w, int h, int state, int border)
{
    if (border) {
	rect(bmp, x, y, x+w-1, y+h-1, black);
	x++, y++, w -= 2, h -= 2;
    }

    switch (state) {
	case 0: /* normal */
	    rectfillwh(bmp, x+1, y+1, w-1, h-1, normal);
	    break;
	case 1: /* highlight */
	    rectfillwh(bmp, x+1, y+1, w-1, h-1, highlight);
	    break;
	case 2: /* pressed */
	    rectfillwh(bmp, x+2, y+2, w-2, h-2, pressed);
	    break;
    }

    gtk_bevel(bmp, x, y, w, h, state);
}


int d_agtk_box_proc(int msg, DIALOG *d, int c)
{
    (void)c;
    if (msg == MSG_DRAW)
	gtk_box(gui_get_screen(), d->x, d->y, d->w, d->h, 0, 0);
    return D_O_K;
}


int d_agtk_shadow_box_proc(int msg, DIALOG *d, int c)
{
    (void)c;
    if (msg == MSG_DRAW)
	gtk_box(gui_get_screen(), d->x, d->y, d->w, d->h, 0, 1);
    return D_O_K;
}


int d_agtk_button_proc(int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW) {
    BITMAP *bmp = gui_get_screen();
	int x;
	if (d->flags & D_SELECTED)
	    x = 2;
	else if (d->flags & D_GOTFOCUS)
	    x = 1;
	else
	    x = 0;
	gtk_box(bmp, d->x, d->y, d->w, d->h, x, d->flags & D_GOTFOCUS);

	if (d->dp) {
	    gui_textout_ex(bmp, (char *)d->dp,
			d->x+d->w/2, d->y+d->h/2-text_height(font)/2,
			(d->flags & D_DISABLED) ? nshadow : black, -1, TRUE);
	}
	return D_O_K;
    }

    return d_button_proc(msg, d, c);
}


int d_agtk_push_proc(int msg, DIALOG *d, int c)
{
    int ret = D_O_K;

    d->flags |= D_EXIT;

    ret |= d_agtk_button_proc(msg, d, c);

    if (ret & D_CLOSE) {
	ret &= ~D_CLOSE;
	REDRAW(d);

	if (d->dp3)
	    ret |= ((int (*)(DIALOG *))d->dp3)(d);
    }

    return ret;
}


int d_agtk_check_proc(int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW) {
	BITMAP *bmp = gui_get_screen();
	draw_base(bmp, d);
	gtk_bevel(bmp, d->x+3, d->y+d->h/2-5, 10, 10, (d->flags & D_SELECTED) ? 2 : 1);
	if (d->dp) {
	    gui_textout_ex(bmp, (char *)d->dp,
			d->x+18, d->y+d->h/2-text_height(font)/2,
			(d->flags & D_DISABLED) ? nshadow : black, -1, FALSE);
	}
	return D_O_K;
    }

    return d_button_proc(msg, d, c);
}


static AL_CONST char *radio_down[] =
{
    /* 11 x 11 */
    "     x     ",
    "    xxx    ",
    "   xxXxx   ",
    "  xxX Xxx  ",
    " xxX   Xxx ",
    "xxX     Xxx",
    " ..a   a.. ",
    "  ..a a..  ",
    "   ..a..   ",
    "    ...    ",
    "     .     "
};


static AL_CONST char *radio_up[] =
{
    /* 11 x 11 */
    "     .     ",
    "    ...    ",
    "   .. ..   ",
    "  ..   ..  ",
    " ..     .. ",
    "..       ..",
    " Xxx   xxX ",
    "  Xxx xxX  ",
    "   XxxxX   ",
    "    XxX    ",
    "     X     "
};


static BITMAP *radio_down_bmp, *radio_up_bmp;


/* d1 used for group number. */
int d_agtk_radio_proc(int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW) {
	BITMAP *bmp = gui_get_screen();
	draw_base(bmp, d);
	draw_sprite(bmp, (d->flags & D_SELECTED) ? radio_down_bmp : radio_up_bmp, d->x+3, d->y+d->h/2-5);

	if (d->dp) {
	    gui_textout_ex(bmp, (char *)d->dp,
			d->x+18, d->y+d->h/2-text_height(font)/2,
			(d->flags & D_DISABLED) ? nshadow : black, -1, FALSE);
	}
	return D_O_K;
    }

    return d_radio_proc(msg, d, c);
}


int d_agtk_icon_proc(int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW) {
	BITMAP *bmp = gui_get_screen();
	BITMAP *img = (BITMAP *)d->dp;
	int x;

	if (d->flags & D_SELECTED)
	    x = 2;
	else if (d->flags & D_GOTFOCUS)
	    x = 1;
	else
	    x = 0;
	gtk_box(bmp, d->x, d->y, d->w, d->h, x, d->flags & D_GOTFOCUS);

	stretch_sprite(bmp, img, d->x+2, d->y+2, d->w-4, d->h-4);
	return D_O_K;
    }

    return d_agtk_button_proc(msg, d, c);
}


int d_agtk_edit_proc(int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW) {
	BITMAP *bmp = gui_get_screen();
	int l, x, b, f, p, w;
	int fg = (d->flags & D_DISABLED) ? hshadow : black;
	char *s = (char *)d->dp;
	char buf[16];
	int fonth;

	agup_edit_adjust_position (d);

	fonth = text_height(font);

	l = ustrlen(s);
	/* set cursor pos */
	if (d->d2 >= l) {
	    d->d2 = l;
	    usetc(buf+usetc(buf, ' '), 0);
	    x = text_length(font, buf) + 2;
	}
	else
	    x = 2;

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
	    p = 0;
	}
	else {
	    b--;
	    p = d->d2-b+1;
	    b = d->d2;
	}

	if (d->flags & D_GOTFOCUS)
	    gtk_bevel(bmp, d->x, d->y, d->w, fonth+6, 3);
	else
	    gtk_bevel(bmp, d->x, d->y, d->w, fonth+6, 2);

	rectfill(bmp, d->x+2, d->y+2, d->x+d->w-3, d->y+fonth+3, white);
	for (x = 4; p<=b; p++) {
	    f = ugetat(s, p);
	    usetc(buf+usetc(buf, (f) ? f : ' '), 0);
	    w = text_length(font, buf);
	    f = ((p == d->d2) && (d->flags & D_GOTFOCUS));
	    textout_ex(bmp, font, buf, d->x+x, d->y+4, fg, white);
	    if (f)
		vline(bmp, d->x+x-1, d->y+3, d->y+fonth+3, black);
	    if ((x += w) + w > d->w - 4)
		break;
	}

        agup_edit_restore_position (d);

	return D_O_K;
    }

    return d_agup_adjusted_edit_proc(msg, d, c);
}


typedef char *(*getfuncptr)(int, int *);


static void gtk_draw_scrollable_frame(DIALOG *d, int listsize, int offset, int height)
{
    BITMAP *bmp = gui_get_screen();
    int i, len, c;
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
	if (d->flags & D_GOTFOCUS)
	    c = 1;
	else if (d->flags & D_SELECTED)
	    c = 2;
	else
	    c = 0;

	xx = d->x+d->w-11;
	yy = d->y;
	len = (((d->h-2) * offset) + listsize/2) / listsize;
	gtk_box(bmp, xx-2, yy, 12, d->h-1, 2, 0);

	i = ((d->h-5) * height + listsize/2) / listsize;
	xx = d->x+d->w-11;
	yy = d->y+2;

	if (offset > 0) {
	    len = (((d->h-5) * offset) + listsize/2) / listsize;
	    rectfill(bmp, xx, yy, xx+8, yy+len, pressed);
	    yy += len;
	}
	if (yy+i < d->y+d->h-3) {
	    gtk_box(bmp, xx, yy, 8, i, c, 0);
	    yy += i+1;
	}
	else
	    gtk_box(bmp, xx, yy, 8, d->h-3-yy+d->y, c, 0);
    }
}


int d_agtk_list_proc(int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW) {
	BITMAP *bmp = gui_get_screen();
	int height, listsize, i, len, bar, x, y, w;
	int fg, bg;
//	int fg_color;	//Unused
	char *sel = (char *)d->dp2;
	char s[1024];

	(*(getfuncptr)d->dp)(-1, &listsize);
	height = (d->h-4) / text_height(font);
	bar = (listsize > height);
	w = (bar ? d->w-14 : d->w-3);
//	fg_color = (d->flags & D_DISABLED) ? agtk_mg_color : d->fg;

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
#if 0
		/* GTK puts a yellow box around the currently selected
		 * item, but it's quite ugly when we emulate it, so
		 * I'm disabling it. --pw
		 */
		if (d->d2+i == d->d1)
		    rect(bmp, x-text_length(font, s)-8, y, d->x+w, y+text_height(font)-1, yellow);
#endif
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
	gtk_draw_scrollable_frame(d, listsize, d->d2, height);

	return D_O_K;
    }

    return d_list_proc(msg, d, c);
}


int d_agtk_text_list_proc(int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW)
	return d_agtk_list_proc(msg, d, c);
    return d_text_list_proc(msg, d, c);
}


int d_agtk_textbox_proc(int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW) {
	int height, bar;
	int fg_color = (d->flags & D_DISABLED) ? agtk_mg_color : black;

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
		      fg_color, white, agtk_mg_color);

	/* draw the frame around */
	gtk_draw_scrollable_frame(d, d->d1, d->d2, height);
	return D_O_K;
    }

    return d_textbox_proc(msg, d, c);
}


int d_agtk_slider_proc(int msg, DIALOG *d, int c)
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
	gtk_box(bmp, d->x, d->y, d->w, d->h, 2, 0);

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

	gtk_box(bmp, slx, sly, slw, slh, (d->flags & D_GOTFOCUS) ? 1 : 0, 0);

	return D_O_K;
    }

    return d_slider_proc(msg, d, c);
}


int
d_agtk_clear_proc (int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW)
    {
        clear_to_color (gui_get_screen(), agtk_bg_color);
        return D_O_K;
    }
    return d_clear_proc (msg, d, c);
}


/*----------------------------------------------------------------------*/
/* Menus                                                                */
/*----------------------------------------------------------------------*/


/* These menus don't look much like GTK menus.  Unfortunately, it's
   probably about as good as we can do without reimplementing menus
   for ourselves.  */


static AL_CONST char *menu_arrow[] =
{
    /* 8 x 7 */
    ".a      ",
    ". aa    ",
    ".   aa  ",
    ".     aa",
    ".   xX  ",
    ". xX    ",
    "xX      "
};


static BITMAP *menu_arrow_bmp;


static void gtk_draw_menu(int x, int y, int w, int h)
{
    gtk_box(gui_get_screen(), x, y, w, h, 0, 1);
}


static void gtk_draw_menu_item(MENU *m, int x, int y, int w, int h, int bar, int sel)
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
	fg = black;
	bg = (sel) ? highlight : normal;
    }

    rectfill(bmp, x+1, y+1, x+w-3, y+h-4, bg);

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


int d_agtk_menu_proc(int msg, DIALOG *d, int c)
{
    return d_menu_proc(msg, d, c);
}


/*----------------------------------------------------------------------*/
/* Window frames							*/
/*----------------------------------------------------------------------*/


int d_agtk_window_proc(int msg, DIALOG *d, int c)
{
    (void)c;

    if (msg == MSG_DRAW) {
	BITMAP *bmp = gui_get_screen();
	gtk_box(bmp, d->x, d->y, d->w, d->h, 0, 1);

	if (d->dp) {
	    int cl = bmp->cl, ct = bmp->ct, cr = bmp->cr, cb = bmp->cb;
	    set_clip_rect(bmp, d->x, d->y, d->x+d->w-1, d->y+d->h-1);

	    textout_ex(bmp, font, (char *)d->dp, d->x+6, d->y+6, black, normal);

	    set_clip_rect(bmp, cl, ct, cr, cb);
	}
    }

    return D_O_K;
}


/*----------------------------------------------------------------------*/
/* Initialisation                                                       */
/*----------------------------------------------------------------------*/


void agtk_init(void)
{
    white = makecol(0xff, 0xff, 0xff);
    highlight = makecol(0xee, 0xea, 0xee);
    normal = makecol(0xd6, 0xd6, 0xd6);
    pressed = makecol(0xc5, 0xc2, 0xc5);
    hshadow = makecol(0xa5, 0xa5, 0xa5);
    nshadow = makecol(0x95, 0x95, 0x95);
    black = makecol(0, 0, 0);
    blue = makecol(0, 0, 0x9c);
    yellow = makecol(0xff, 0xff, 0x62);

    agtk_fg_color = black;
    agtk_bg_color = normal;
    agtk_mg_color = nshadow;

    radio_up_bmp = generate_bitmap(radio_up, 11, 11);
    radio_down_bmp = generate_bitmap(radio_down, 11, 11);

    menu_arrow_bmp = generate_bitmap(menu_arrow, 8, 7);
    gui_menu_draw_menu = gtk_draw_menu;
    gui_menu_draw_menu_item = gtk_draw_menu_item;
}


void agtk_shutdown(void)
{
    destroy_bitmap(menu_arrow_bmp);
    gui_menu_draw_menu_item = NULL;
    gui_menu_draw_menu = NULL;

    destroy_bitmap(radio_down_bmp);
    destroy_bitmap(radio_up_bmp);
}


/*----------------------------------------------------------------------*/
/* Theme                                                                */
/*----------------------------------------------------------------------*/

static struct AGUP_THEME the_theme =
{
    "GTK",
    &agtk_fg_color,
    &agtk_bg_color,
    &agtk_mg_color,
    agtk_init,
    agtk_shutdown,
    d_agtk_box_proc,
    d_agtk_shadow_box_proc,
    d_agtk_button_proc,
    d_agtk_push_proc,
    d_agtk_check_proc,
    d_agtk_radio_proc,
    d_agtk_icon_proc,
    d_agtk_edit_proc,
    d_agtk_list_proc,
    d_agtk_text_list_proc,
    d_agtk_textbox_proc,
    d_agtk_slider_proc,
    d_agtk_menu_proc,
    d_agtk_window_proc,
    d_agtk_clear_proc,
    d_agup_common_text_proc,
    d_agup_common_ctext_proc,
    d_agup_common_rtext_proc
};


AL_CONST struct AGUP_THEME *agtk_theme = &the_theme;


/*
		white
	#eeeaee hilight
	#d6d6d6 normal
	#c5c2c5 clicked
	#a5a5a5 hi-shadow
	#959595 shadow
		black
*/
