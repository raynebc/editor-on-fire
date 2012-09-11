/* ans.c
 *
 * This file is part of the Allegro GUI Un-uglification Project.
 * It emulates the look of the NeXTstep widget set.
 *
 * Joao Neves <miauz@clix.pt>
 */


#include <allegro.h>
#include "ans.h"
#include "agupitrn.h"

#ifdef USEMEMWATCH
#include "../memwatch.h"
#endif


/*----------------------------------------------------------------------*/
/* Recommend fg/bg colors                                               */
/*----------------------------------------------------------------------*/


int ans_fg_color;
int ans_bg_color;
int ans_mg_color;


/*----------------------------------------------------------------------*/
/* Helpers                                                              */
/*----------------------------------------------------------------------*/


static int white, lgrey, dgrey, black, disabled;


#define REDRAW(d)                                               \
do {                                                            \
    scare_mouse_area(d->x, d->y, d->x + d->w, d->y + d->h);     \
    object_message(d, MSG_DRAW, 0);                             \
    unscare_mouse();                                            \
} while (0)


static void ns_bevel(BITMAP *bmp, int x, int y, int w, int h, int state)
{
    w--;
    h--;
    switch (state) {
	case 0:		/* normal */
	    rectfill(bmp, x, y, x + w, y + h, lgrey);
	    rect(bmp, x, y, x + w, y + h, black);
	    line(bmp, x, y, x + w - 1, y, white);
	    line(bmp, x, y, x, y + h - 1, white);
	    line(bmp, x + 1, y + h - 1, x + w - 1, y + h - 1, dgrey);
	    line(bmp, x + w - 1, y + 1, x + w - 1, y + h - 1, dgrey);
	    break;

	case 1:		/* pressed */
	    rectfill(bmp, x, y, x + w, y + h, white);
	    line(bmp, x, y, x, y + h - 1, black);
	    line(bmp, x, y, x + w - 1, y, black);
	    break;

	case 2:		/* focused */
	    rectfill(bmp, x, y, x + w, y + h, white);
	    rect(bmp, x, y, x + w, y + h, black);
	    line(bmp, x, y, x + w - 1, y, white);
	    line(bmp, x, y, x, y + h - 1, white);
	    line(bmp, x + 1, y + h - 1, x + w - 1, y + h - 1, dgrey);
	    line(bmp, x + w - 1, y + 1, x + w - 1, y + h - 1, dgrey);
	    break;
    }
}


static void ns_check(BITMAP *bmp, int x, int y)
{
    line(bmp, x + 4, y + 6, x + 4, y + 10, white);
    line(bmp, x + 4, y + 10, x + 12, y + 2, white);
    line(bmp, x + 5, y + 6, x + 5, y + 7, black);
    line(bmp, x + 5, y + 8, x + 6, y + 7, dgrey);
    line(bmp, x + 4, y + 11, x + 12, y + 3, black);
    line(bmp, x + 5, y + 11, x + 12, y + 4, dgrey);
}


static BITMAP *generate_bitmap(AL_CONST char *templat[], int w, int h)
{
    BITMAP *bmp = create_bitmap(w, h);
    int x, y;

    for (y = 0; y < h; y++)
	for (x = 0; x < w; x++)
	    switch (templat[y][x]) {
		case '.': putpixel(bmp, x, y, white); break;
		case 'X': putpixel(bmp, x, y, black); break;
		case 'a': putpixel(bmp, x, y, lgrey); break;
		case 'x': putpixel(bmp, x, y, dgrey); break;
		case ' ': putpixel(bmp, x, y, lgrey); break;
	    }

    return bmp;
}


/*----------------------------------------------------------------------*/
/* NeXTstep-lookalike procs  						*/
/*----------------------------------------------------------------------*/


int d_ans_box_proc(int msg, DIALOG *d, int c)
{
    (void) c;

    if (msg == MSG_DRAW) {
	ns_bevel(gui_get_screen(), d->x, d->y, d->w, d->h, 0);
    }

    return D_O_K;
}


int d_ans_shadow_box_proc(int msg, DIALOG *d, int c)
{
    return d_ans_box_proc(msg, d, c);
}


int d_ans_button_proc(int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW) {
	BITMAP *bmp = gui_get_screen();
	int s = 0;
	int fg = black;

	if (d->flags & D_SELECTED) {
	    s = 1;
	}
	if (d->flags & D_DISABLED) {
	    fg = disabled;
	}

	ns_bevel(bmp, d->x, d->y, d->w, d->h, s);

	if (d->dp) {
	    if (d->flags & D_SELECTED) {
		gui_textout_ex(bmp, (char *)d->dp, d->x + d->w / 2 + 1,
			    d->y + d->h / 2 - text_height(font) / 2 + 1, fg, -1,
			    TRUE);
	    }
	    else {
		gui_textout_ex(bmp, (char *)d->dp, d->x + d->w / 2,
			    d->y + d->h / 2 - text_height(font) / 2, fg, -1, TRUE);
	    }
	}

	return D_O_K;
    }

    return d_button_proc(msg, d, c);
}


int d_ans_push_proc(int msg, DIALOG *d, int c)
{
    int ret = D_O_K;

    d->flags |= D_EXIT;

    ret |= d_ans_button_proc(msg, d, c);

    if (ret & D_CLOSE) {
	ret &= ~D_CLOSE;
	REDRAW(d);

	if (d->dp3)
	    ret |= ((int (*)(DIALOG *)) d->dp3) (d);
    }

    return ret;
}


int d_ans_check_proc(int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW) {
	BITMAP *bmp = gui_get_screen();
	int fg = black;
	ns_bevel(bmp, d->x, d->y + d->h / 2 - 8, 16, 16, 0);

	if (d->flags & D_SELECTED) {
	    ns_check(bmp, d->x, d->y + d->h / 2 - 8);
	}
	if (d->flags & D_DISABLED) {
	    fg = disabled;
	}

	if (d->dp) {
	    gui_textout_ex(bmp, (char *)d->dp,
			d->x + 20, d->y + d->h / 2 - text_height(font) / 2,
			fg, -1, FALSE);
	}

	return D_O_K;
    }

    return d_button_proc(msg, d, c);
}


static AL_CONST char *radio_up[] =
{
    /* 15 x 15 */
    "     xxxxx     ",
    "   xxXXXXXxx   ",
    "  xXX       x  ",
    " xXx           ",
    " xX            ",
    "xX            .",
    "xX            .",
    "xX            .",
    "xX            .",
    "xX            .",
    " xx          . ",
    " x           . ",
    "  x         .  ",
    "    .     ..   ",
    "     .....     "
};


static AL_CONST char *radio_down[] =
{
    /* 15 x 15 */
    "     xxxxx     ",
    "   xxXXXXXxx   ",
    "  xXX     x x  ",
    " xXx  ....     ",
    " xX ........   ",
    "xX  ........  .",
    "xX .......... .",
    "xX .......... .",
    "xX .......... .",
    "xX .......... .",
    " xx ........ . ",
    " x  ........ . ",
    "  x   ....  .  ",
    "    .     ..   ",
    "     .....     "
};


static AL_CONST char *handle[] =
{
    /* 6 x 6 */
    " xXXX ",
    "xXxxxx",
    "Xxx   ",
    "Xx  ..",
    "Xx ...",
    " x .. "
};


static AL_CONST char *menu_arrow[] =
{
    /* 7 x 7 */
    "Xx     ",
    "X xx   ",
    "X   xx ",
    "X     .",
    "X   .. ",
    "X ..   ",
    "X.     "
};


static AL_CONST char *menu_arrow_sel[] =
{
    /* 7 x 7 */
    "Xx.....",
    "X.xx...",
    "X...xx.",
    "X.....a",
    "X...aa.",
    "X.aa...",
    "Xa....."
};


static BITMAP *radio_down_bmp, *radio_up_bmp;
static BITMAP *handle_bmp;
static BITMAP *menu_arrow_bmp, *menu_arrow_sel_bmp;


int d_ans_radio_proc(int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW) {
	BITMAP *bmp = gui_get_screen();
	int fg = black;
	draw_sprite(bmp,
		    (d->flags & D_SELECTED) ? radio_down_bmp : radio_up_bmp,
		    d->x, d->y + d->h / 2 - 7);
	if (d->flags & D_DISABLED) {
	    fg = disabled;
	}

	if (d->dp) {
	    gui_textout_ex(bmp, (char *)d->dp,
			d->x + 20, d->y + d->h / 2 - text_height(font) / 2,
			fg, -1, FALSE);
	}

	return D_O_K;
    }

    return d_radio_proc(msg, d, c);
}


int d_ans_icon_proc(int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW) {
	BITMAP *bmp = gui_get_screen();
	BITMAP *img = (BITMAP *)d->dp;
	int x = 0;

	if (d->flags & D_SELECTED) {
	    x = 1;
	}

	ns_bevel(bmp, d->x, d->y, d->w, d->h, x);

	stretch_sprite(bmp, img, d->x + 2, d->y + 2, d->w - 4, d->h - 4);

	return D_O_K;
    }

    return d_button_proc(msg, d, c);
}


int d_ans_edit_proc(int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW) {
	BITMAP *bmp = gui_get_screen();
	int l, x, b, f, p, w;
	int fg = (d->flags & D_DISABLED) ? disabled : black;
	char *s = (char *)d->dp;
	char buf[16];
	int fonth;

        agup_edit_adjust_position (d);

	fonth = text_height(font);

	l = ustrlen(s);
	/* set cursor pos */
	if (d->d2 >= l) {
	    d->d2 = l;
	    usetc(buf + usetc(buf, ' '), 0);
	    x = text_length(font, buf) + 2;
	}
	else
	    x = 2;

	b = 0;			/* num of chars to be blitted */
	/* get the part of the string to be blitted */
	for (p = d->d2; p >= 0; p--) {
	    usetc(buf + usetc(buf, ugetat(s, p)), 0);
	    x += text_length(font, buf);
	    b++;

	    if (x > d->w - 4)
		break;
	}

	/* see if length of text is too wide */
	if (x <= d->w - 2) {
	    b = l;
	    p = 0;
	}
	else {
	    b--;
	    p = d->d2 - b + 1;
	    b = d->d2;
	}

	rectfill(bmp, d->x, d->y, d->x + d->w - 1, d->y + fonth + 5, lgrey);
	rect(bmp, d->x, d->y, d->x + d->w - 1, d->y + fonth + 5, white);
	line(bmp, d->x, d->y, d->x + d->w - 2, d->y, dgrey);
	line(bmp, d->x, d->y, d->x, d->y + fonth + 4, dgrey);
	line(bmp, d->x + 1, d->y + 1, d->x + d->w - 3, d->y + 1, dgrey);
	line(bmp, d->x + 1, d->y + 1, d->x + 1, d->y + fonth + 3, dgrey);

	rectfill(bmp, d->x + 2, d->y + 2, d->x + d->w - 3, d->y + fonth + 3, white);
	for (x = 4; p <= b; p++) {
	    f = ugetat(s, p);
	    usetc(buf + usetc(buf, (f) ? f : ' '), 0);
	    w = text_length(font, buf);
	    f = ((p == d->d2) && (d->flags & D_GOTFOCUS));
	    textout_ex(bmp, font, buf, d->x + x, d->y + 4, fg, white);
	    if (f)
		vline(bmp, d->x + x - 1, d->y + 3, d->y + fonth + 3, black);
	    if ((x += w) + w > d->w - 4)
		break;
	}

        agup_edit_restore_position (d);

	return D_O_K;
    }

    return d_agup_adjusted_edit_proc(msg, d, c);
}


typedef char *(*getfuncptr) (int, int *);


static void ns_draw_scrollable_frame(DIALOG *d, int listsize, int offset, int height)
{
	BITMAP *bmp = gui_get_screen();
    int i, len;
//    int c;	//Unused
    int xx, yy;

    /* draw frame */
    hline(bmp, d->x, d->y, d->x + d->w - 1, dgrey);
    vline(bmp, d->x, d->y, d->y + d->h - 1, dgrey);

    hline(bmp, d->x + 1, d->y + 1, d->x + d->w - 2, black);
    vline(bmp, d->x + 1, d->y + 1, d->y + d->h - 2, black);
    hline(bmp, d->x + 2, d->y + d->h - 2, d->x + d->w - 2, black);
    vline(bmp, d->x + d->w - 2, d->y + 2, d->y + d->h - 3, black);

    hline(bmp, d->x + 1, d->y + d->h - 1, d->x + d->w - 1, white);
    vline(bmp, d->x + d->w - 1, d->y + 1, d->y + d->h - 2, white);

    /* possibly draw scrollbar */
    if (listsize > height) {
//	if (d->flags & D_GOTFOCUS)
//	    c = 1;
//	else if (d->flags & D_SELECTED)
//	    c = 2;
//	else
//	    c = 0;

	xx = d->x + d->w - 11;
	yy = d->y;
	len = (((d->h - 2) * offset) + listsize / 2) / listsize;
	rectfill(bmp, xx - 2, yy + 1, xx - 2 + 12 - 1, yy + d->h - 1 - 1, lgrey);
	vline(bmp, xx - 2, yy + 1, yy + d->h - 1 - 1, black);

	i = ((d->h - 5) * height + listsize / 2) / listsize;
	xx = d->x + d->w - 12;
	yy = d->y + 2;

	if (offset > 0) {
	    len = (((d->h - 5) * offset) + listsize / 2) / listsize;
	    rectfill(bmp, xx, yy, xx + 10, yy + len, lgrey);
	    yy += len;
	}
	if (yy + i < d->y + d->h - 3) {
	    ns_bevel(bmp, xx, yy-1, 11, i+1, 0);
	    if (i > 10) {
		draw_sprite(bmp, handle_bmp, xx + 11 / 2 - 3, yy + i / 2 - 3);
	    }
	    yy += i + 1;
	}
	else {
	    ns_bevel(bmp, xx, yy-1, 11, d->h  - yy + d->y, 0);
	    if (i > 10) {
		draw_sprite(bmp, handle_bmp, xx + 11 / 2 - 3, yy + i / 2 - 3);
	    }
	}
    }
}


int d_ans_list_proc(int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW) {
	BITMAP *bmp = gui_get_screen();
	int height, listsize, i, len, bar, x, y, w;
	int fg, bg;
//	int fg_color;	//Unused
	char *sel = (char *)d->dp2;
	char s[1024];

	rectfill(bmp, d->x, d->y, d->x + d->w - 1, d->y + d->h - 1, lgrey);

	(*(getfuncptr) d->dp) (-1, &listsize);
	height = (d->h - 4) / text_height(font);
	bar = (listsize > height);
	w = (bar ? d->w - 14 : d->w - 3);
//	fg_color = (d->flags & D_DISABLED) ? disabled : d->fg;

	/* draw box contents */
	for (i = 0; i < height; i++) {
	    if (d->d2 + i < listsize) {
		if (d->d2 + i == d->d1) {
		    fg = black;
		    bg = white;
		}
		else if ((sel) && (sel[d->d2 + i])) {
		    fg = white;
		    bg = dgrey;
		}
		else {
		    fg = black;
		    bg = lgrey;
		}
		usetc(s, 0);
		ustrncat(s, (*(getfuncptr) d->dp) (i + d->d2, NULL),
			 sizeof(s) - ucwidth(0));
		x = d->x + 2;
		y = d->y + 2 + i * text_height(font);
		rectfill(bmp, x, y, x + 7, y + text_height(font) - 1, bg);
		x += 8;
		len = ustrlen(s);
		while (text_length(font, s) >=
		       MAX(d->w - 1 - (bar ? 22 : 10), 1)) {
		    len--;
		    usetat(s, len, 0);
		}
		textout_ex(bmp, font, s, x, y, fg, bg);
		x += text_length(font, s);

		if (x <= d->x + w)
		    rectfill(bmp, x, y, d->x+w, y+text_height(font)-1, bg);
	    }
	    else {
		rectfill(bmp, d->x + 2, d->y + 2 + i * text_height(font),
			 d->x + w, d->y + 1 + (i + 1) * text_height(font),
			 lgrey);
	    }
	}

	if (d->y + 2 + i * text_height(font) <= d->y + d->h - 3)
	    rectfill(bmp, d->x + 2, d->y + 2 + i * text_height(font),
		     d->x + w, d->y + d->h - 3, lgrey);

	/* draw frame, maybe with scrollbar */
	ns_draw_scrollable_frame(d, listsize, d->d2, height);

	return D_O_K;
    }

    return d_list_proc(msg, d, c);
}


int d_ans_text_list_proc(int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW)
	return d_ans_list_proc(msg, d, c);
    return d_text_list_proc(msg, d, c);
}


int d_ans_textbox_proc(int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW) {
	int height, bar;
	int fg_color = (d->flags & D_DISABLED) ? disabled : black;

	height = (d->h - 8) / text_height(font);

	/* tell the object to sort of draw, but only calculate the listsize */
	_draw_textbox((char *)d->dp, &d->d1, 0,	/* DONT DRAW anything */
		      d->d2, !(d->flags & D_SELECTED), 8, d->x, d->y,
		      d->w, d->h, (d->flags & D_DISABLED), 0, 0, 0);

	if (d->d1 > height) {
	    bar = 11;
	}
	else {
	    bar = 0;
	    d->d2 = 0;
	}

	/* now do the actual drawing */
	_draw_textbox((char *)d->dp, &d->d1, 1, d->d2,
		      !(d->flags & D_SELECTED), 8, d->x, d->y, d->w - bar, d->h,
		      (d->flags & D_DISABLED), fg_color, white, ans_mg_color);

	/* draw the frame around */
	ns_draw_scrollable_frame(d, d->d1, d->d2, height);
	return D_O_K;
    }

    return d_textbox_proc(msg, d, c);
}


int d_ans_slider_proc(int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW) {
	BITMAP *bmp = gui_get_screen();
	int vert = TRUE;	/* flag: is slider vertical? */
	int slp;		/* slider position */
	int irange;
//	int ismark, inmark = 1;	//Unused
	fixed slratio, slmax, slpos;

	/* check for slider direction */
	if (d->h < d->w)
	    vert = FALSE;

	irange = (vert ? d->h - 2 : d->w - 2);
//	ismark = irange / inmark;
	slmax = itofix(irange - 19);
	slratio = slmax / (d->d1);
	slpos = slratio * d->d2;
	slp = 1 + fixtoi(slpos);

	rectfill(bmp, d->x, d->y, d->x + d->w - 1, d->y + d->h - 1, dgrey);
	line(bmp, d->x, d->y, d->x, d->y + d->h - 1, black);
	line(bmp, d->x, d->y, d->x + d->w - 1, d->y, black);
	line(bmp, d->x, d->y + d->h - 1, d->x + d->w - 1, d->y + d->h - 1, white);
	line(bmp, d->x + d->w - 1, d->y, d->x + d->w - 1, d->y + d->h - 1, white);

	if (vert) {
	    ns_bevel(bmp, d->x + 1, d->y + d->h - slp - 19, d->w - 2, 10, 0);
	    ns_bevel(bmp, d->x + 1, d->y + d->h - slp - 10, d->w - 2, 10, 0);
	    line(bmp, d->x + 2, d->y + d->h - slp - 19, d->x + 2,
		 d->y + d->h - slp - 12, white);
	    line(bmp, d->x + 2, d->y + d->h - slp - 10, d->x + 2,
		 d->y + d->h - slp - 3, white);
	    putpixel(bmp, d->x + d->w - 3, d->y + d->h - slp - 10, lgrey);
	    putpixel(bmp, d->x + 1, d->y + d->h - slp - 2, dgrey);
	}
	else {
	    ns_bevel(bmp, d->x + slp, d->y + 1, 10, d->h - 2, 0);
	    ns_bevel(bmp, d->x + slp + 9, d->y + 1, 10, d->h - 2, 0);
	    line(bmp, d->x + slp + 1, d->y + 2, d->x + slp + 1,
		 d->y + d->h - 2 - 2, white);
	    putpixel(bmp, d->x + slp + 9, d->y + 1 + d->h - 2 - 2, lgrey);
	    putpixel(bmp, d->x + slp + 9 + 8, d->y + 1, dgrey);
	}
	return D_O_K;
    }

    return d_slider_proc(msg, d, c);
}


int
d_ans_clear_proc (int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW)
    {
        clear_to_color (gui_get_screen(), ans_bg_color);
        return D_O_K;
    }
    return d_clear_proc (msg, d, c);
}


/*----------------------------------------------------------------------*/
/* Menus                                                                */
/*----------------------------------------------------------------------*/


/* These menus don't look much like NeXTstep menus.  Unfortunately, it's
   probably about as good as we can do without reimplementing menus
   for ourselves.  */


static void ns_draw_menu(int x, int y, int w, int h)
{
    ns_bevel(gui_get_screen(), x, y, w, h, 0);
}


static void ns_draw_menu_item(MENU * m, int x, int y, int w, int h, int bar, int sel)
{
    BITMAP *bmp = gui_get_screen();
    int fg, bg;
    int i, j;
    char buf[256], *tok;

    if (m->flags & D_DISABLED) {
	fg = disabled;
	bg = lgrey;
    }
    else {
	fg = black;
	bg = (sel) ? white : lgrey;
    }

    if (sel && (!(m->flags & D_DISABLED))) {
	ns_bevel(bmp, x, y, w, h, 2);
    }
    else {
	ns_bevel(bmp, x, y, w, h, 0);
    }

    if (ugetc(m->text)) {
	i = 0;
	j = ugetc(m->text);

	while ((j) && (j != '\t')) {
	    i += usetc(buf + i, j);
	    j = ugetc(m->text + i);
	}

	usetc(buf + i, 0);

	gui_textout_ex(bmp, buf, x + 8, y + 1, fg, bg, FALSE);

	if (j == '\t') {
	    tok = m->text + i + uwidth(m->text + i);
	    gui_textout_ex(bmp, tok, x + w - gui_strlen(tok) - 10, y + 1, fg, bg,
			FALSE);
	}

	if ((m->child) && (!bar)) {
	    if (sel) {
		draw_sprite(bmp, menu_arrow_sel_bmp, x + w - 12,
			    y + (h - menu_arrow_sel_bmp->h) / 2);
	    }
	    else {
		draw_sprite(bmp, menu_arrow_bmp, x + w - 12,
			    y + (h - menu_arrow_bmp->h) / 2);
	    }
	}
    }
    else {
	hline(bmp, x + 4, y + text_height(font) / 2 + 2, x + w - 4, dgrey);
	hline(bmp, x + 4, y + text_height(font) / 2 + 3, x + w - 4, white);
    }

    if (m->flags & D_SELECTED) {
	line(bmp,
	     x + 1, y + text_height(font) / 2 + 1,
	     x + 3, y + text_height(font) + 1, fg);
	line(bmp, x + 3, y + text_height(font) + 1, x + 6, y + 2, fg);
    }
}


int d_ans_menu_proc(int msg, DIALOG *d, int c)
{
    return d_menu_proc(msg, d, c);
}


/*----------------------------------------------------------------------*/
/* Window frames							*/
/*----------------------------------------------------------------------*/


int d_ans_window_proc(int msg, DIALOG *d, int c)
{
    (void)c;

    if (msg == MSG_DRAW) {
	BITMAP *bmp = gui_get_screen();
	rectfill(bmp, d->x, d->y, d->x + d->w - 1, d->y + d->h - 1, lgrey);
	rect(bmp, d->x, d->y, d->x + d->w - 1, d->y + d->h - 1, black);
	rectfill(bmp, d->x, d->y, d->x + d->w - 1, d->y + text_height(font) + 6 + 1, black);

	hline(bmp, d->x + 1, d->y + text_height(font) + 6, d->x + d->w - 1 - 2, dgrey);
	vline(bmp, d->x + d->w - 1 - 1, d->y + 2, d->y + text_height(font) + 6, dgrey);
	hline(bmp, d->x + 1, d->y + 1, d->x + d->w - 2, lgrey);
	vline(bmp, d->x + 1, d->y + 1, d->y + text_height(font) + 6, lgrey);

	if (d->dp) {
	    int cl = bmp->cl, ct = bmp->ct;
	    int cr = bmp->cr, cb = bmp->cb;
	    set_clip_rect(bmp, d->x, d->y, d->x + d->w - 1, d->y + d->h - 1);

	    textout_centre_ex(bmp, font, (char *)d->dp, d->x + d->w / 2, d->y + 3 + 2, white, -1);

	    set_clip_rect(bmp, cl, ct, cr, cb);
	}
    }

    return D_O_K;
}


/*----------------------------------------------------------------------*/
/* Initialisation                                                       */
/*----------------------------------------------------------------------*/


void ans_init(void)
{
    white = makecol(0xff, 0xff, 0xff);
    lgrey = makecol(0xbd, 0xbd, 0xbd);
    dgrey = makecol(0x7b, 0x7b, 0x7b);
    black = makecol(0, 0, 0);
    disabled = makecol(0x7f, 0x7f, 0x7f);

    ans_fg_color = black;
    ans_bg_color = lgrey;
    ans_mg_color = disabled;

    radio_up_bmp = generate_bitmap(radio_up, 15, 15);
    radio_down_bmp = generate_bitmap(radio_down, 15, 15);
    handle_bmp = generate_bitmap(handle, 6, 6);
    menu_arrow_bmp = generate_bitmap(menu_arrow, 7, 7);
    menu_arrow_sel_bmp = generate_bitmap(menu_arrow_sel, 7, 7);

    gui_menu_draw_menu = ns_draw_menu;
    gui_menu_draw_menu_item = ns_draw_menu_item;
}

void ans_shutdown(void)
{
    destroy_bitmap(radio_up_bmp);
    destroy_bitmap(radio_down_bmp);
    destroy_bitmap(handle_bmp);
    destroy_bitmap(menu_arrow_bmp);
    destroy_bitmap(menu_arrow_sel_bmp);
    gui_menu_draw_menu_item = NULL;
    gui_menu_draw_menu = NULL;
}

/*----------------------------------------------------------------------*/
/* Theme                                                                */
/*----------------------------------------------------------------------*/

static struct AGUP_THEME the_theme = {
    "NeXTStep",
    &ans_fg_color,
    &ans_bg_color,
    &ans_mg_color,
    ans_init,
    ans_shutdown,
    d_ans_box_proc,
    d_ans_shadow_box_proc,
    d_ans_button_proc,
    d_ans_push_proc,
    d_ans_check_proc,
    d_ans_radio_proc,
    d_ans_icon_proc,
    d_ans_edit_proc,
    d_ans_list_proc,
    d_ans_text_list_proc,
    d_ans_textbox_proc,
    d_ans_slider_proc,
    d_ans_menu_proc,
    d_ans_window_proc,
    d_ans_clear_proc,
    d_agup_common_text_proc,
    d_agup_common_ctext_proc,
    d_agup_common_rtext_proc
};


AL_CONST struct AGUP_THEME *ans_theme = &the_theme;

/*
                white
        #bdbdbd light grey
        #7b7b7b dark grey
        #7f7f7f disabled
                black
*/
