/* abitmap.c
 *
 * This file is part of the Allegro GUI Un-uglification Project.
 * It is a general bitmap theme, everything is a bitmap.
 *
 * One major drawback of AGUP is, it's almost impossible to create a new
 * theme - something which normally is wanted for use in a game's menu
 * though. With this bitmap theme, there's limited possibility for creating
 * own themes now - as long as all of the theme consists of simple bitmapped
 * buttons. The file abitmap.txt contains instructions for how to create
 * such a theme from a simple Allegro configuration file and some bitmaps.
 *
 * Elias Pschernig <elias@users.sourceforge.net>
 *
 */

#include <allegro.h>
#include <allegro/internal/aintern.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "agup.h"
#include "agupitrn.h"

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#ifdef USEMEMWATCH
#include "../memwatch.h"
#endif

/* internally recognized bitmap elements */
enum B_ELEMENT
{
    B_BOX,
    B_BUTTON,
    B_ICON,
    B_LIST,
    B_TEXTBOX,
    B_SCROLLER,
    B_SLIDERV,
    B_SLIDERH,
    B_CHECK,
    B_RADIO,
    B_EDIT,
    B_CLEAR,
    B_TEXT,
    B_MENU,
    B_MENUBAR,
    B_WINDOW,

    B_BUTTON_DOWN,
    B_ICON_DOWN,
    B_CHECK_DOWN,
    B_RADIO_DOWN,

    B_LIST_ITEM,
    B_CHECK_ITEM,
    B_RADIO_ITEM,
    B_CURSOR,

    B_CHECK_ITEM_DOWN,
    B_RADIO_ITEM_DOWN,
    B_MENU_CHECK,
    B_MENU_SUB,
    B_MENU_SEP,

    B_SCROLLER_HANDLE,
    B_SLIDERV_HANDLE,
    B_SLIDERH_HANDLE,

    B_MENU_ITEM,
    B_MENUBAR_ITEM,

    B_ELEMENTS_COUNT
};

/* data for a single GUI bitmap */
struct ABMAP
{
    BITMAP *bmp;
    enum
    {
        ABMAP_TILE_H = 0,
        ABMAP_TILE_V = 0,
        ABMAP_STRETCH_H = 1,
        ABMAP_STRETCH_V = 2,
        ABMAP_CENTER_H = 4,
        ABMAP_CENTER_V = 8,
        ABMAP_ALIGN_H = 16,
        ABMAP_ALIGN_V = 32
    }
    flags;
    int bl, bt, br, bb;
    int color;
};

struct AGUP_BITMAP_THEME
{
    /* possible theme adjustments */
    int textpushx, textpushy;
    int fg_color, bg_color, mg_color;

    /* 3 bitmaps for everything, 0 = normal, 1 = highlight, 2 = disabled */
    struct ABMAP bitmaps[B_ELEMENTS_COUNT][3];
};

/* AGUP registration */

struct AGUP_BITMAP_THEME_LIST
{
    char *path;
    DATAFILE *datafile;
    AGUP_THEME theme;
};

static struct AGUP_BITMAP_THEME_LIST **themes = NULL;
static int themes_count = 0;

static struct AGUP_BITMAP_THEME global_theme;
static struct AGUP_BITMAP_THEME *theme = &global_theme; /* theme currently used */

static BITMAP *none; /* used for a hack to make a textbox transparent */

/* information to load a single bitmap. */
struct BLOAD
{
    char const *name;
    enum B_ELEMENT elem;
};

struct BLOAD bload[] = {
    {"box", B_BOX}, /* NOT masked */
    {" button", B_BUTTON}, /* NOT masked */
    {"  check", B_CHECK_ITEM}, /* may be masked */
    {"   radio", B_RADIO_ITEM}, /* may be masked */
    {"  icon", B_ICON}, /* NOT masked */
    {"  scroller", B_SCROLLER_HANDLE}, /* may be masked */
    {"   sliderh", B_SLIDERH_HANDLE}, /* may be masked */
    {"   sliderv", B_SLIDERV_HANDLE}, /* may masked */
    {" buttonsel", B_BUTTON_DOWN},  /* NOT masked */
    {"  iconsel", B_ICON_DOWN}, /* NOT masked */
    {"  scroll", B_SCROLLER}, /* NOT masked */
    {"   slidev", B_SLIDERV}, /* NOT masked */
    {"   slideh", B_SLIDERH}, /* NOT masked */
    {"   cursor", B_CURSOR}, /* may be masked */
    {"   menusep", B_MENU_SEP}, /* may be masked */
    {"  checked", B_CHECK_ITEM_DOWN}, /* may be masked */
    {"   radiosel", B_RADIO_ITEM_DOWN}, /* may be masked */
    {"   menucheck", B_MENU_CHECK}, /* may be masked */
    {"   menusub", B_MENU_SUB}, /* may be masked */
    {"  edit", B_EDIT}, /* NOT masked */
    {" list", B_LIST}, /* NOT masked */
    {" textbox", B_TEXTBOX}, /* NOT masked */
    {" menu", B_MENU}, /* NOT masked */
    {"  menuitem", B_MENU_ITEM}, /* NOT masked */
    {"  menubar", B_MENUBAR}, /* NOT masked */
    {"   menubaritem", B_MENUBAR_ITEM},     /* NOT masked */
    {" window", B_WINDOW}, /* NOT masked */
    {" clear", B_CLEAR}, /* NOT masked */
    {"  checkback", B_CHECK}, /* NOT masked */
    {"   checkbacksel", B_CHECK_DOWN}, /* NOT masked */
    {"  radioback", B_RADIO}, /* NOT masked */
    {"   radiobacksel", B_RADIO_DOWN}, /* NOT masked */
    {"  text", B_TEXT}, /* NOT masked */
    {"   listitem", B_LIST_ITEM}, /* NOT masked */
    {NULL, B_ELEMENTS_COUNT}
};

/* Given two sizes, return an offset <= 0, so when texturing the area of size1
 * with a texture of size size2, the center will be aligned.
 */
static inline int
centered_offset (int size1, int size2)
{
    int center1, center2, o;

    if (!size1 || !size2)
        return 0;
    center1 = size1 / 2;
    center2 = size2 / 2;
    o = (center1 - center2) % size2;
    if (o > 0)
        o -= size2;
    return o;
}

static inline void
masked_non_stretched_blit (BITMAP *s, BITMAP *d, int sx, int sy, int w, int h,
                           int dx, int dy, int _, int __)
{
    (void) _;
    (void) __;
    masked_blit (s, d, sx, sy, dx, dy, w, h);
}

enum COLUMN_TYPE
{
    COLUMN_CENTER = 1,
    COLUMN_STRETCH,
    COLUMN_LEFT,
    COLUMN_MIDDLE,
    COLUMN_RIGHT
};
/* Draw a column of pattern pat into the bitmap bmp inside the given rectangle.
 */
static inline void
blit_column (BITMAP *bmp, struct ABMAP *pat, int x, int y, int w, int h, enum COLUMN_TYPE f)
{
    int ct = bmp->ct, cb = bmp->cb;
    int oy;
    int j;
    int pat_h = pat->bmp->h - pat->bt - pat->bb;

    void (*bfunc) (BITMAP *, BITMAP *, int, int, int, int, int, int, int,
                   int) = masked_non_stretched_blit;
    int bx = 0;
    int bw = pat->bmp->w;
    int x2 = x;
    int w2;

    if (pat->flags & ABMAP_ALIGN_V)
        oy = (y / pat_h) * pat_h - y;
    else
        oy = centered_offset (h, pat_h);

    if (pat_h < 1)
        return;

    if (f == COLUMN_MIDDLE)
        bx = pat->bl;
    if (f == COLUMN_RIGHT)
        bx = pat->bmp->w - pat->br;

    if (f == COLUMN_LEFT)
        bw = pat->bl;
    if (f == COLUMN_MIDDLE)
        bw = w;
    if (f == COLUMN_RIGHT)
        bw = pat->br;

    if (f == COLUMN_CENTER)
        x2 = x + w / 2 - pat->bmp->w / 2;
    if (f == COLUMN_RIGHT)
        x2 = x + w - pat->br;

    w2 = bw;

    if (f == COLUMN_STRETCH)
    {
        w2 = w;
        bfunc = masked_stretch_blit;
    }

    if (pat->flags & ABMAP_CENTER_V)
    {
        bfunc (pat->bmp, bmp, bx, 0, bw, pat->bmp->h, x2,
               y + h / 2 - pat->bmp->h / 2, w2, pat->bmp->h);
    }
    else if (pat->flags & ABMAP_STRETCH_V)
    {
        masked_stretch_blit (pat->bmp, bmp, bx, 0, bw, pat->bmp->h, x2, y, w2,
                             h);
    }
    else
    {
        /* top */
        if (pat->bt)
        {
            bmp->ct = MAX (ct, y);
            bmp->cb = MIN (cb, MIN (y + h, y + pat->bt));
            bfunc (pat->bmp, bmp, bx, 0, bw, pat->bt, x2, y, w2, pat->bt);
        }
        /* middle */
        bmp->ct = MAX (ct, MIN (y + h, y + pat->bt));
        bmp->cb = MIN (cb, MAX (y, y + h - pat->bb));
        for (j = oy + y; j < y + h; j += pat->bmp->h - pat->bt - pat->bb)
        {
            bfunc (pat->bmp, bmp, bx, pat->bt, bw, pat_h, x2, j, w2, pat_h);
        }
        /* bottom */
        if (pat->bt)
        {
            bmp->ct = MAX (ct, MAX (y, y + h - pat->bb));
            bmp->cb = MIN (cb, y + h);
            bfunc (pat->bmp, bmp, bx, pat->bmp->h - pat->bb, bw, pat->bb, x2,
                   y + h - pat->bb, w2, pat->bb);
        }
    }
    bmp->ct = ct;
    bmp->cb = cb;
}

/* Draw the pattern pat into the specified rectangle, following the contained
 * tiling, stretching, centering and bordering constraints.
 *
 * Borders align depending on their type:
 *
 * NW N NE
 * W  C  E
 * SW S SW
 *
 * The remaining texture is aligned with its center to the center, except if the
 * align flag is set. In this case, it will be aligned NW to the screen.
 */
static void
abitmap_draw_bmp (BITMAP *bmp, struct ABMAP *pat, int x, int y, int w, int h)
{
    int i;

    int cl = bmp->cl, ct = bmp->ct, cr = bmp->cr, cb = bmp->cb;
    int pat_w = pat->bmp->w - pat->bl - pat->br;

    if (w < 1 || h < 1 || pat_w < 1)
        return;

    set_clip_rect (bmp, MAX (cl, x), MAX (ct, y), MIN (cr, x + w) - 1,
              MIN (cb, y + h) - 1);

    if (pat->flags & ABMAP_CENTER_H)
    {
        blit_column (bmp, pat, x, y, w, h, COLUMN_CENTER);
    }
    else if (pat->flags & ABMAP_STRETCH_H)
    {
        blit_column (bmp, pat, x, y, w, h, COLUMN_STRETCH);
    }
    else
    {
        int ox;
        if (pat->flags & ABMAP_ALIGN_H)
            ox = (x / pat_w) * pat_w - x;
        else
            ox = centered_offset (w, pat_w);

        /* left */
        if (pat->bl)
        {
            /* Intersect with previous clipping range. */
            bmp->cl = MAX (cl, x);
            bmp->cr = MIN (cr, MIN (x + w, x + pat->bl));

            blit_column (bmp, pat, x, y, w, h, COLUMN_LEFT);
        }
        /* middle */
        bmp->cl = MAX (cl, MIN (x + w, x + pat->bl));
        bmp->cr = MIN (cr, MAX (x, x + w - pat->br));
        for (i = ox + x; i < x + w; i += pat_w)
        {
            blit_column (bmp, pat, i, y, pat_w, h, COLUMN_MIDDLE);
        }
        /* right */
        if (pat->br)
        {
            bmp->cl = MAX (cl, MAX (x, x + w - pat->br));
            bmp->cr = MIN (cr, x + w);

            blit_column (bmp, pat, x, y, w, h, COLUMN_RIGHT);
        }
    }

    bmp->cl = cl;
    bmp->ct = ct;
    bmp->cr = cr;
    bmp->cb = cb;
}

static inline int
get_state (DIALOG *d)
{
    if (d->flags & D_DISABLED)
        return 2;
    if (d->flags & D_GOTFOCUS)
        return 1;
    return 0;
}

static void
abitmap_draw_box (DIALOG *d, int what)
{
    int s = get_state (d);
    abitmap_draw_bmp (gui_get_screen(), &theme->bitmaps[what][s], d->x, d->y, d->w, d->h);
}

static void
abitmap_draw_text (DIALOG *d, int what, int offx, int offy, int push,
    int halign, int valign)
{
    int s = get_state (d);

    int x = d->x + offx;
    int y = d->y + offy;
    int w = gui_strlen (d->dp);
    int h = text_height (font);
    if (halign == 1)
        x = d->x + (d->w - offx) / 2 - w / 2;
    if (halign == 2)
        x = d->x + (d->w - offx) - w;
    if (valign == 1)
        y = d->y + (d->h - offy) / 2 - h / 2;
    if (valign == 2)
        y = d->y + (d->h - offy) - h;

    if (push && (d->flags & D_SELECTED))
    {
        x += theme->textpushx;
        y += theme->textpushy;
    }

    gui_textout_ex(gui_get_screen(), d->dp, x, y, theme->bitmaps[what][s].color, -1, FALSE);
}

static void
abitmap_draw_area (DIALOG *d, int what, int offx, int offy, int aw, int ah,
    int halign, int valign)
{
    int s = get_state (d);

    int x = d->x + offx;
    int y = d->y + offy;
    int w = aw;
    int h = ah;
    if (halign == 1)
        x = d->x + (d->w - offx) / 2 - w / 2;
    if (halign == 2)
        x = d->x + (d->w - offx) - w;
    if (valign == 1)
        y = d->y + (d->h - offy) / 2 - h / 2;
    if (valign == 2)
        y = d->y + (d->h - offy) - h;

    abitmap_draw_bmp (gui_get_screen(), &theme->bitmaps[what][s], x, y, w, h);
}

static void
abitmap_draw_scroller (DIALOG *d, int pos, int total, int visible)
{
    int s = get_state (d);
    int min_h = theme->bitmaps[B_SCROLLER_HANDLE][s].bmp->h;
    int bar_h = d->h * visible / total;
    int bar_p;

    if (bar_h < min_h)
        bar_h = min_h;

    bar_p = (d->h - bar_h) * pos / (total - visible);

    abitmap_draw_area (d, B_SCROLLER, 0, 0, 12, d->h, 2, 0);

    abitmap_draw_area (d, B_SCROLLER_HANDLE, 0, bar_p, 12, bar_h, 2, 0);
}

static void
abitmap_draw_slider (DIALOG *d)
{
    int s = get_state (d);
    int w = d->w;
    int h = d->h;
    int pos = d->d2;
    int total = d->d1;
    int visible_size = w > h ? w : h;
    int bar_size = ((w > h) ? theme->bitmaps[B_SLIDERH_HANDLE][s].bmp->w :
        theme->bitmaps[B_SLIDERV_HANDLE][s].bmp->h);
    int bar_p = (visible_size - bar_size) * pos / total;

    if (w > h)
    {
        abitmap_draw_box (d, B_SLIDERH);
        abitmap_draw_area (d, B_SLIDERH_HANDLE, bar_p, 0, bar_size, h, 0, 0);
    }
    else
    {
        abitmap_draw_box (d, B_SLIDERV);
        abitmap_draw_area (d, B_SLIDERV_HANDLE, 0, h - bar_size - bar_p, w, bar_size, 0, 0);
    }
}


/*----------------------------------------------------------------------*/
/* Bitmapped theme procs                                                */
/*----------------------------------------------------------------------*/

int
d_abitmap_box_proc (int msg, DIALOG *d, int c)
{
    (void) c;
    if (msg == MSG_DRAW)
        abitmap_draw_box (d, B_BOX);
    return D_O_K;
}

int
d_abitmap_shadow_box_proc (int msg, DIALOG *d, int c)
{
    (void) c;
    if (msg == MSG_DRAW)
        abitmap_draw_box (d, B_BOX);
    return D_O_K;
}

int
d_abitmap_button_proc (int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW)
    {
        int b = d->flags & D_SELECTED ? B_BUTTON_DOWN : B_BUTTON;
        abitmap_draw_box (d, b);
        abitmap_draw_text (d, b, 0, 0, d->flags & D_SELECTED, 1, 1);
        return D_O_K;
    }
    return d_button_proc (msg, d, c);
}

int
d_abitmap_check_proc (int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW)
    {
        int b = d->flags & D_SELECTED ? B_CHECK_DOWN : B_CHECK;
        int bi = d->flags & D_SELECTED ? B_CHECK_ITEM_DOWN : B_CHECK_ITEM;
        abitmap_draw_box (d, b);
        abitmap_draw_area (d, bi, 0, 0, 12, 12, 0, 1);
        abitmap_draw_text (d, b, 14, 0, 0, 0, 1);
        return D_O_K;
    }
    return d_button_proc (msg, d, c);
}

int
d_abitmap_radio_proc (int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW)
    {
        int b = d->flags & D_SELECTED ? B_RADIO_DOWN : B_RADIO;
        int bi = d->flags & D_SELECTED ? B_RADIO_ITEM_DOWN : B_RADIO_ITEM;
        abitmap_draw_box (d, b);
        abitmap_draw_area (d, bi, 0, 0, 12, 12, 0, 1);
        abitmap_draw_text (d, b, 14, 0, 0, 0, 1);
        return D_O_K;
    }
    return d_radio_proc (msg, d, c);
}

int
d_abitmap_icon_proc (int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW)
    {
        int b = d->flags & D_SELECTED ? B_ICON_DOWN : B_ICON;
        int px = 0, py = 0;
        abitmap_draw_box (d, b);
        if (d->flags & D_SELECTED)
        {
            px = theme->textpushx;
            py = theme->textpushy;
        }
        stretch_sprite (gui_get_screen(), d->dp, d->x + 2 + px, d->y + 2 + py,
            d->w - 4 - px, d->h - 4 - py);
        return D_O_K;
    }
    return d_icon_proc (msg, d, c);
}

int
d_abitmap_edit_proc (int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW)
    {
		BITMAP *bmp = gui_get_screen();
        int l, x, b, f, p, w;
        char *s = (char *) d->dp;
        char buf[16];           /* Buffer to hold one char at a time. */
        int c = 0;
        int fg;

        agup_edit_adjust_position (d);

        if (d->flags & D_GOTFOCUS)
            c = 1;
        if (d->flags & D_DISABLED)
            c = 2;

        abitmap_draw_area (d, B_EDIT, 0, 0, d->w, text_height (font) + 8, 0, 0);

        l = ustrlen (s);
        /* set cursor pos */
        if (d->d2 >= l)
        {
            d->d2 = l;
            usetc (buf + usetc (buf, ' '), 0);
            x = text_length (font, buf) + 2;
        }
        else
            x = 2;

        b = 0;                  /* num of chars to be blitted */
        /* get the part of the string to be blitted */
        for (p = d->d2; p >= 0; p--)
        {
            usetc (buf + usetc (buf, ugetat (s, p)), 0);
            x += text_length (font, buf);
            b++;
            if (x > d->w - 4)
                break;
        }

        /* see if length of text is too wide */
        if (x <= d->w - 2)
        {
            b = l;
            p = 0;
        }
        else
        {
            b--;
            p = d->d2 - b + 1;
            b = d->d2;
        }

        for (x = 4; p <= b; p++)
        {
            f = ugetat (s, p);
            usetc (buf + usetc (buf, (f) ? f : ' '), 0);
            w = text_length (font, buf);

            if (((p == d->d2) && (d->flags & D_GOTFOCUS)))
            {
                abitmap_draw_bmp (bmp, &theme->bitmaps[B_CURSOR][c],
                    d->x + x, d->y, w, text_height (font) + 4);
                fg =  theme->bitmaps[B_CURSOR][c].color;
            }
            else
                fg =  theme->bitmaps[B_EDIT][c].color;

            textout_ex (bmp, font, buf, d->x + x, d->y + 4, fg, -1);

            if ((x += w) + w > d->w - 4)
                break;
        }

        agup_edit_restore_position (d);

        return D_O_K;
    }

    if (msg == MSG_LOSTFOCUS || msg == MSG_LOSTMOUSE)
        return D_WANTFOCUS;

    return d_agup_adjusted_edit_proc (msg, d, c);
}

int
d_abitmap_list_proc (int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW)
    {
		BITMAP *bmp = gui_get_screen();
        int height, size, i, len, bar, x, y;
        char *sel = d->dp2;
        char s[1024];
        int c = 0;

        if (d->flags & D_GOTFOCUS)
            c = 1;
        if (d->flags & D_DISABLED)
            c = 2;

        (*(char *(*)(int, int *)) d->dp) (-1, &size);
        height = (d->h - 4) / text_height (font);
        bar = (size > height);

        abitmap_draw_area (d, B_LIST, 0, 0, bar ? d->w - 12 : d->w, d->h, 0, 0);
        if (bar)
            abitmap_draw_scroller (d, d->d2, size, height);

        /* draw the contents */
        for (i = 0; i < height; i++)
        {
            if (d->d2 + i < size)
            {
                int fg = theme->bitmaps[B_LIST][c].color;

                x = d->x + 2;
                y = d->y + 2 + i * text_height (font);

                if (d->d2 + i == d->d1 || ((sel) && (sel[d->d2 + i])))
                {
                    abitmap_draw_area (d, B_LIST_ITEM, 0, y - d->y,
                        bar ? d->w - 12 : d->w, text_height (font), 0, 0);
                    fg = theme->bitmaps[B_LIST_ITEM][c].color;
                }
                ustrzcpy (s, sizeof (s),
                          (*(char *(*)(int, int *)) d->dp) (i + d->d2, NULL));

                x += 8;
                len = ustrlen (s);
                while (text_length (font, s) >=
                       MAX (d->w - 1 - (bar ? 22 : 11), 1))
                {
                    len--;
                    usetat (s, len, 0);
                }
                textout_ex (bmp, font, s, x, y, fg, -1);
                x += text_length (font, s);

            }
        }
        return D_O_K;
    }
    return d_list_proc (msg, d, c);
}

int
d_abitmap_text_list_proc (int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW)
        return d_abitmap_list_proc (msg, d, c);

    return d_text_list_proc (msg, d, c);
}

int
d_abitmap_textbox_proc (int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW)
    {
        int s = get_state (d);
        int height, bar = 1;
        int mode, x_anchor, y_anchor;
        BITMAP *pattern;
        height = (d->h - 8) / text_height (font);

        /* hack to make _draw_textbox draw transparent text */
        mode = _drawing_mode;
        pattern = _drawing_pattern;
        x_anchor = _drawing_x_anchor;
        y_anchor = _drawing_y_anchor;
        drawing_mode (DRAW_MODE_MASKED_PATTERN, none, 0, 0);

        /* tell the object to sort of draw, but only calculate the listsize */
        _draw_textbox ((char *) d->dp, &d->d1, 0,       /* DONT DRAW anything */
                       d->d2, !(d->flags & D_SELECTED), 8,
                       d->x, d->y, d->w - 2, d->h - 2,
                       (d->flags & D_DISABLED), 0, 0, 0);

        if (d->d1 > height)
            bar = 11;
        else
            d->d2 = 0;

        abitmap_draw_area (d, B_TEXTBOX, 0, 0, bar == 11 ? d->w - 12 : d->w, d->h, 0, 0);
        if (bar == 11)
            abitmap_draw_scroller (d, d->d2, d->d1 + 1, height);

        /* now do the actual drawing */
        _draw_textbox ((char *) d->dp, &d->d1, 1, d->d2,
                       !(d->flags & D_SELECTED), 8,
                       d->x, d->y, d->w - 2 - bar, d->h - 2,
                       (d->flags & D_DISABLED), theme->bitmaps[B_TEXTBOX][s].color,
                       -1, theme->bitmaps[B_TEXTBOX][s].color);

        _drawing_mode = mode;
        _drawing_pattern = pattern;
        _drawing_x_anchor = x_anchor;
        _drawing_y_anchor = y_anchor;

        return D_O_K;
    }

    return d_textbox_proc (msg, d, c);
}

int
d_abitmap_slider_proc (int msg, DIALOG *d, int c)
{

    if (msg == MSG_DRAW)
    {
        abitmap_draw_slider (d);

        return D_O_K;
    }

    return d_slider_proc (msg, d, c);
}

int
d_abitmap_clear_proc (int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW)
    {
        int s = get_state (d);
        abitmap_draw_bmp (gui_get_screen(), &theme->bitmaps[B_CLEAR][s], 0, 0, SCREEN_W, SCREEN_H);
        return D_O_K;
    }
    return d_clear_proc (msg, d, c);
}

int
d_abitmap_text_proc (int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW)
    {
        abitmap_draw_text (d, B_TEXT, 0, 0, 0, 0, 0);
        return D_O_K;
    }
    return d_text_proc (msg, d, c);
}

int
d_abitmap_ctext_proc (int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW)
    {
        abitmap_draw_text (d, B_TEXT, 0, 0, 0, 1, 0);
        return D_O_K;
    }
    return d_ctext_proc (msg, d, c);
}

int
d_abitmap_rtext_proc (int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW)
    {
        abitmap_draw_text (d, B_TEXT, 0, 0, 0, 2, 0);
        return D_O_K;
    }
    return d_rtext_proc (msg, d, c);
}

int
d_abitmap_push_proc (int msg, DIALOG *d, int c)
{
    int ret = D_O_K;

    d->flags |= D_EXIT;
    ret |= d_abitmap_button_proc (msg, d, c);
    if (ret & D_CLOSE)
    {
        ret &= ~D_CLOSE;

        scare_mouse_area (d->x, d->y, d->x + d->w, d->y + d->h);
        object_message (d, MSG_DRAW, 0);
        unscare_mouse ();

        if (d->dp3)
            ret |= ((int (*)(DIALOG *)) d->dp3) (d);
    }
    return ret;
}

/*----------------------------------------------------------------------*/
/* Menus                                                                */
/*----------------------------------------------------------------------*/

static int last_x, last_y, last_w, last_h, menu_done;

static void
abitmap_draw_menu (int x, int y, int w, int h)
{
    /* Idea taken from aphoton.c, because we want a menu bar to look
     * differently from a popup menu:
     * Ugly hack needed because we lack the 'bar' member
     * of the underlying MENU_INFO structure from which
     * the arguments of the functions are passed.
     *
     * Since I just read this and couldn't make any sense out of it, here is
     * what happens. In Allegro, we first call draw_menu with the dimensions
     * of the whole menu, then call draw_menu_item for all the menu items, each
     * only getting their own dimensions. This is not ok if we need the relative
     * position of the menu item - so we just don't do anything here, but save
     * the position, and then use it in subsequent draw_menu_item calls. It's
     * even needed if all we want is to draw a menu bar different from a menu,
     * because this info is only present in draw_menu_item.
     */
    last_x = x;
    last_y = y;
    last_w = w;
    last_h = h;
    menu_done = FALSE;
}

static void
abitmap_draw_menu_item (MENU * m, int x, int y, int w, int h, int bar,
                        int sel)
{
	BITMAP *bmp = gui_get_screen();
    int i, j;
    char buf[256], *tok;
    int c;
    int fg;
    int x2 = x, y2 = y, w2 = w, h2 = h;

    if (m->flags & D_DISABLED)
        c = 2;
    else
        c = 0;
    if (sel)
        c = 1;

    if (bar)
    {
        fg = theme->bitmaps[B_MENUBAR_ITEM][c].color;
        y2 -= 1;
        h2 += 3;
    }
    else
    {
        fg = theme->bitmaps[B_MENU_ITEM][c].color;
        x2 -= 1;
        w2 += 3;
    }

    /* See the previous function. */
    if (!menu_done)
    {
        /* Draw the menu background. */
        if (bar)
            abitmap_draw_bmp (bmp, &theme->bitmaps[B_MENUBAR][c], last_x, last_y, last_w,
last_h);
        else
            abitmap_draw_bmp (bmp, &theme->bitmaps[B_MENU][c], last_x, last_y, last_w,
last_h);
        menu_done = TRUE;
    }

    abitmap_draw_bmp (bmp, &theme->bitmaps[bar ? B_MENUBAR_ITEM :
        B_MENU_ITEM][c],    x2, y2, w2, h2);

    if (m->flags & D_SELECTED)
    {
        /* checked menu item */
        abitmap_draw_bmp (bmp, &theme->bitmaps[B_MENU_CHECK][c], x, y, 12, h);
    }

    if (ugetc (m->text))
    {
        i = 0;
        j = ugetc (m->text);

        while ((j) && (j != '\t'))
        {
            i += usetc (buf + i, j);
            j = ugetc (m->text + i);
        }

        usetc (buf + i, 0);

        gui_textout_ex (bmp, buf, x + 8, y + 1, fg, -1, FALSE);

        if (j == '\t')
        {
            tok = m->text + i + uwidth (m->text + i);
            gui_textout_ex (bmp, tok, x + w - gui_strlen (tok) - 10, y + 1,
                         fg, -1, FALSE);
        }

        if ((m->child) && (!bar))
        {
            abitmap_draw_bmp (bmp, &theme->bitmaps[B_MENU_SUB][c], x + w - 12, y, 12, h);
        }
    }
    else
    {
        /* menu separator */
        abitmap_draw_bmp (bmp, &theme->bitmaps[B_MENU_SEP][c], x, y, w, h);
    }
}

int
d_abitmap_menu_proc (int msg, DIALOG *d, int c)
{
    return d_menu_proc (msg, d, c);
}

/*----------------------------------------------------------------------*/
/* Window frames                                                        */
/*----------------------------------------------------------------------*/

int
d_abitmap_window_proc (int msg, DIALOG *d, int c)
{
    (void) c;

    if (msg == MSG_DRAW)
    {
        abitmap_draw_box (d, B_WINDOW);
        abitmap_draw_text (d, B_WINDOW, 0, 0, 0, 1, 0);
    }

    return D_O_K;
}

/*----------------------------------------------------------------------*/
/* Initialization                                                       */
/*----------------------------------------------------------------------*/

static BITMAP **used_bitmaps = NULL;
static char **used_bitmaps_names = NULL;
static int used_bitmaps_count = 0;
static DATAFILE *used_dat = NULL;

static void
used_bitmap (BITMAP *bmp, char const *name)
{
    used_bitmaps = realloc (used_bitmaps,
        (used_bitmaps_count + 1) * sizeof *used_bitmaps);
    used_bitmaps[used_bitmaps_count] = bmp;
    used_bitmaps_names = realloc (used_bitmaps_names,
        (used_bitmaps_count + 1) * sizeof *used_bitmaps_names);
    used_bitmaps_names[used_bitmaps_count] = name ? strdup (name) : NULL;
    used_bitmaps_count++;
}

static BITMAP *
find_used_bitmap (char const *name)
{
    int i;
    for (i = 0; i < used_bitmaps_count; i++)
    {
        if (used_bitmaps_names[i] && !strcmp (name, used_bitmaps_names[i]))
            return used_bitmaps[i];
    }
    return NULL;
}

static void
used_bitmaps_cleanup (void)
{
    int i;
    for (i = 0; i < used_bitmaps_count; i++)
    {
        destroy_bitmap (used_bitmaps[i]);
        free (used_bitmaps_names[i]);
    }
    free (used_bitmaps);
    free (used_bitmaps_names);
    if (used_dat)
        unload_datafile (used_dat);
    used_bitmaps_count = 0;
    used_bitmaps = NULL;
    used_bitmaps_names = NULL;
    used_dat = NULL;
}

/* Finds agup.cfg and add its contents to Allegro's current config. If agup.cfg
 * is found inside a datafile, the datafile is returned (and loaded if
 * necessary).
 */
static DATAFILE *
set_theme_config (char const *path, DATAFILE *datafile)
{
    DATAFILE *dat = NULL;

    if (path)
    {
        char *ext = get_extension (path);
        char str[1024 * 6];

        if (!strcmp (ext, "dat"))
        {
            dat = load_datafile (path);
            if (!dat)
                dat = datafile;
        }
        else if (!strcmp (ext, "cfg"))
            set_config_file (path);
        else
        {
            replace_extension (str, path, "dat", 1024 * 6);
            dat = load_datafile (str);
            if (!dat)
            {
                replace_extension (str, path, "cfg", 1024 * 6);
                set_config_file (str);
            }
        }
    }
    else
        dat = datafile;

    if (dat)
    {
        override_config_data (dat->dat, dat->size);
    }

    return dat;
}

static BITMAP *
find_theme_bitmap (DATAFILE *datafile, char const *name)
{
    char str[1024 * 6];
    char const *prefix = get_config_string ("agup.cfg", "prefix", "");
    char const *suffix = get_config_string ("agup.cfg", "suffix", "");
    snprintf (str, sizeof str, "%s%s%s", prefix, name, suffix);

    if (datafile)
    {
        DATAFILE *d = find_datafile_object (datafile, str);
        if (d)
            return d->dat;
        else
            TRACE ("Bitmap %s not found in theme datafile.\n", str);
        return NULL;
    }
    else
    {
        BITMAP *bmp = find_used_bitmap (str);
        if (!bmp)
        {
            bmp = load_bitmap (str, NULL);
            if (bmp)
                used_bitmap (bmp, str);
            else
                TRACE ("bitmap %s not found.\n", str);
        }
        return bmp;
    }
}

static int get_color_string (char const *str, char const *def)
{
     char const *s = get_config_string ("agup.cfg", str, def);
     int c32 = strtol (s, NULL, 0);
     return makecol (c32 >> 16, (c32 >> 8) & 255, c32 & 255);
}

/* See README for details.
 */
void
abitmap_init (char const *path, DATAFILE *datafile)
{
    int i;
    int prev[10];
    int cv;
    char str[1024];
    DATAFILE *dat;

    cv = get_color_conversion ();
    set_color_conversion (COLORCONV_TOTAL | COLORCONV_KEEP_TRANS);

    used_bitmaps_cleanup ();

    push_config_state ();
    dat = set_theme_config (path, datafile);
    if (dat != datafile)
        used_dat = dat;

    theme->fg_color = get_color_string ("fg", "0x000000");
    theme->bg_color = get_color_string ("bg", "0xffffff");
    theme->mg_color = get_color_string ("mg", "0x808080");

    theme->textpushx = get_config_int ("agup.cfg", "px", 0);
    theme->textpushy = get_config_int ("agup.cfg", "py", 0);

    for (i = 0; bload[i].name; i++)
    {
        int j;
        int s;

        char const *suffix[] = { "", "_hl", "_dis" };

        for (s = 0; bload[i].name[s] == ' '; s++);

        for (j = 0; j < 3; j++)
        {
            char **argv = NULL;
            int argc = 0;
            BITMAP *bmp = NULL;
            int got_color = 0;

            theme->bitmaps[bload[i].elem][j].bmp = NULL;
            theme->bitmaps[bload[i].elem][j].flags = 0;
            theme->bitmaps[bload[i].elem][j].bl = 0;
            theme->bitmaps[bload[i].elem][j].br = 0;
            theme->bitmaps[bload[i].elem][j].bt = 0;
            theme->bitmaps[bload[i].elem][j].bb = 0;

            sprintf (str, "%s%s", bload[i].name + s, suffix[j]);
            argv = get_config_argv ("agup.cfg", str, &argc);

            TRACE ("%s: ", str);

            if (argc)
            {
                bmp = find_theme_bitmap (dat, argv[0]);
            }

            if (bmp)
            {
                int a;

                theme->bitmaps[bload[i].elem][j].bmp = bmp;

                for (a = 1; a < argc; a++)
                {
                    if (!strcmp (argv[a], "stretch"))
                        theme->bitmaps[bload[i].elem][j].flags |=
                            ABMAP_STRETCH_H | ABMAP_STRETCH_V;
                    else if (!strcmp (argv[a], "center"))
                        theme->bitmaps[bload[i].elem][j].flags |=
                            ABMAP_CENTER_H | ABMAP_CENTER_V;
                    else if (!strcmp (argv[a], "stretchh"))
                        theme->bitmaps[bload[i].elem][j].flags |= ABMAP_STRETCH_H;
                    else if (!strcmp (argv[a], "centerh"))
                        theme->bitmaps[bload[i].elem][j].flags |= ABMAP_CENTER_H;
                    else if (!strcmp (argv[a], "alignh"))
                        theme->bitmaps[bload[i].elem][j].flags |= ABMAP_ALIGN_H;
                    else if (!strcmp (argv[a], "stretchv"))
                        theme->bitmaps[bload[i].elem][j].flags |= ABMAP_STRETCH_V;
                    else if (!strcmp (argv[a], "centerv"))
                        theme->bitmaps[bload[i].elem][j].flags |= ABMAP_CENTER_V;
                    else if (!strcmp (argv[a], "alignv"))
                        theme->bitmaps[bload[i].elem][j].flags |= ABMAP_ALIGN_V;
                    else if (!strcmp (argv[a], "cut"))
                    {
                        int cx, cy, cw, ch;
                        a++;
                        if (a < argc)
                        {
                            cx = strtol (argv[a], NULL, 10);
                            a++;
                            if (a < argc)
                            {
                                cy = strtol (argv[a], NULL, 10);
                                a++;
                                if (a < argc)
                                {
                                    cw = strtol (argv[a], NULL, 10);
                                    a++;
                                    if (a < argc)
                                    {
                                        BITMAP *sub;
                                        ch = strtol (argv[a], NULL, 10);
                                        if (cw <= 0)
                                            cw += bmp->w;
                                        if (ch <= 0)
                                            ch += bmp->h;
                                        sub = create_sub_bitmap (bmp, cx, cy, cw, ch);
                                        used_bitmap (sub, NULL);
                                        theme->bitmaps[bload[i].elem][j].bmp = sub;
                                    }
                                }
                            }
                        }
                    }
                    else if (!strcmp (argv[a], "border"))
                    {
                        a++;
                        if (a < argc)
                        {
                            theme->bitmaps[bload[i].elem][j].bl = strtol (argv[a], NULL, 10);
                            a++;
                            if (a < argc)
                            {
                                theme->bitmaps[bload[i].elem][j].br =
                                    strtol (argv[a], NULL, 10);
                                a++;
                                if (a < argc)
                                {
                                    theme->bitmaps[bload[i].elem][j].bt =
                                        strtol (argv[a], NULL, 10);
                                    a++;
                                    if (a < argc)
                                    {
                                        theme->bitmaps[bload[i].elem][j].bb =
                                            strtol (argv[a], NULL, 10);
                                    }
                                }
                            }
                        }
                    }
                    else if (!strcmp (argv[a], "color"))
                    {
                        a++;
                        if (a < argc)
                        {
                            int rgb32 = strtol (argv[a], NULL, 0);

                            theme->bitmaps[bload[i].elem][j].color =
                                makecol (rgb32 >> 16, (rgb32 >> 8) & 255,
                                         rgb32 & 255);
                            got_color = 1;
                            TRACE ("color %06x ", rgb32);
                        }
                    }
                }
                TRACE ("loaded successfully.\n");
            }
            else
                /* If everything fails, try to inherit from another bitmap. */
            {
                if (j)
                {
                    theme->bitmaps[bload[i].elem][j] =
                        theme->bitmaps[bload[i].elem][0];
                    TRACE ("inherited from normal state.\n");
                }
                else if (s)
                {
                    theme->bitmaps[bload[i].elem][0] =
                        theme->bitmaps[bload[prev[s - 1]].elem][0];
                    TRACE ("inherited from parent.\n");
                }
                else
                {
                    BITMAP *bmp;

                    /* This shouldn't happen. Build a replacement bitmap. */
                    bmp = create_bitmap (32, 32);
                    clear_to_color (bmp, theme->bg_color);
                    used_bitmap (bmp, "replacement");
                    theme->bitmaps[bload[i].elem][j].bmp = bmp;
                    TRACE ("Fatal: AGUP bitmap not present!\n");
                }
            }

            if (!got_color)
            {
                /* Inherit color from parent element, independent from bitmap inheritance. */
                if (s)
                {
                    theme->bitmaps[bload[i].elem][j].color =
                        theme->bitmaps[bload[prev[s - 1]].elem][j].color;
                }
                else
                {
                    theme->bitmaps[bload[i].elem][j].color = theme->fg_color;
                }
            }

            prev[s] = i;
        }
    }

    gui_menu_draw_menu = abitmap_draw_menu;
    gui_menu_draw_menu_item = abitmap_draw_menu_item;
    none = create_bitmap (32, 32);
    used_bitmap (none, "none");
    clear_to_color (none, bitmap_mask_color (none));

    pop_config_state ();
    set_color_conversion (cv);

}

/* Frees all resources used by the bitmap theme. */
void
abitmap_shutdown (void)
{
    gui_menu_draw_menu_item = NULL;
    gui_menu_draw_menu = NULL;
    used_bitmaps_cleanup ();
}

/* AGUP theme init function. */
static void
init (void)
{
    int i;
    for (i = 0; i < themes_count; i++)
    {
        if (&themes[i]->theme == agup_theme)
            abitmap_init (themes[i]->path, themes[i]->datafile);
    }
}

/* AGUP theme shutdown function. */
static void
shutdown (void)
{
    abitmap_shutdown ();
}

static struct AGUP_THEME theme_template = {
    NULL,
    &global_theme.fg_color,
    &global_theme.bg_color,
    &global_theme.mg_color,
    init,
    shutdown,
    d_abitmap_box_proc,
    d_abitmap_shadow_box_proc,
    d_abitmap_button_proc,
    d_abitmap_push_proc,
    d_abitmap_check_proc,
    d_abitmap_radio_proc,
    d_abitmap_icon_proc,
    d_abitmap_edit_proc,
    d_abitmap_list_proc,
    d_abitmap_text_list_proc,
    d_abitmap_textbox_proc,
    d_abitmap_slider_proc,
    d_abitmap_menu_proc,
    d_abitmap_window_proc,
    d_abitmap_clear_proc,
    d_abitmap_text_proc,
    d_abitmap_ctext_proc,
    d_abitmap_rtext_proc
};

struct AGUP_THEME *agup_load_bitmap_theme (char const *path, DATAFILE *datafile)
{
    int i = themes_count;
    char const *box;
    DATAFILE *dat;
    char *name;

    push_config_state ();
    dat = set_theme_config (path, datafile);
    if (dat != datafile)
        unload_datafile (dat);
    box = get_config_string ("agup.cfg", "box", NULL);
    name = strdup (get_config_string ("agup.cfg", "name", "unnamed"));
    pop_config_state ();

    if (!box)
    {
        free (name);
        return NULL;
    }

    themes_count++;
    themes = realloc (themes, themes_count * sizeof *themes);
    themes[i] = calloc (1, sizeof *themes[i]);
    themes[i]->path = strdup (path);
    themes[i]->datafile = datafile;
    /* Copy the template. */
    themes[i]->theme = theme_template;
    themes[i]->theme.name = name;

    agup_themes_list[agup_themes_count] = &themes[i]->theme;
    agup_themes_count++;

    return &themes[i]->theme;
}

