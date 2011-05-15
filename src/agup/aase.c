/* aase.c
 *
 * This file is part of the Allegro GUI Un-uglification Project.
 * It emulates the look of the allegro-sprite-editor widget set.
 *
 * David A. Capello <dacap@users.sourceforge.net>
 * http://ase.sourceforge.net
 */

#include <allegro.h>

#include "aase.h"
#include "agupitrn.h"

#ifdef USEMEMWATCH
#include "../memwatch.h"
#endif


int aase_fg_color;
int aase_mg_color;
int aase_bg_color;



static void ase_rectedge(BITMAP *bmp, int x1, int y1, int x2, int y2, int fg, int bg);
static void ase_rectdotted(BITMAP *bmp, int x1, int y1, int x2, int y2, int fg, int bg);
static void ase_rectgouraud(BITMAP *bmp, int x1, int y1, int x2, int y2, int top, int bottom);
static void ase_rectbox(BITMAP *bmp, int x1, int y1, int x2, int y2, int flags);



void aase_init(void)
{
  aase_fg_color = makecol(0,   0,   0);
  aase_bg_color = makecol(255, 255, 255);
  aase_mg_color = makecol(128, 128, 128);
}



void aase_shutdown(void)
{
}



static void ase_rectedge(BITMAP *bmp, int x1, int y1, int x2, int y2, int fg, int bg)
{
  hline(bmp, x1, y1,     x2 - 1, fg);
  vline(bmp, x1, y1 + 1, y2 - 1, fg);
  hline(bmp, x1, y2,     x2,     bg);
  vline(bmp, x2, y1,     y2 - 1, bg);
}



static void ase_rectdotted(BITMAP *bmp, int x1, int y1, int x2, int y2, int fg, int bg)
{
  int x = ((x1+y1) & 1) ? 1 : 0;
  int c;

  /* two loops to avoid bank switches */
  for (c=x1; c<=x2; c++)
    putpixel(bmp, c, y1, (((c+y1) & 1) == x) ? fg : bg);
  for (c=x1; c<=x2; c++)
    putpixel(bmp, c, y2, (((c+y2) & 1) == x) ? fg : bg);

  for (c=y1+1; c<y2; c++) {
    putpixel(bmp, x1, c, (((c+x1) & 1) == x) ? fg : bg);
    putpixel(bmp, x2, c, (((c+x2) & 1) == x) ? fg : bg);
  }
}



static void ase_rectgouraud(BITMAP *bmp, int x1, int y1, int x2, int y2, int top, int bottom)
{
  int gray, y;

  for (y=y1; y<=y2; y++) {
    gray = top + (bottom - top) * (y - y1) / (y2 - y1);

    hline(bmp, x1, y, x2, makecol(gray, gray, gray));
  }
}



static void ase_rectbox(BITMAP *bmp, int x1, int y1, int x2, int y2, int flags)
{
  int bg, fg, top, bottom, aux;

  if (flags & D_DISABLED) {
    bg = makecol(192, 192, 192);
    fg = makecol(128, 128, 128);
    top = 255;
    bottom = 128;
  }
  else {
    bg = makecol(255, 255, 255);
    fg = makecol(0, 0, 0);
    top = 255;
    bottom = (flags & D_GOTFOCUS)? 224: 194;
  }

  if (flags & D_SELECTED) {
    aux = top;
    top = bottom;
    bottom = aux;
  }

  rect(bmp, x1, y1, x2, y2, fg);
  ase_rectedge(bmp, x1+1, y1+1, x2-1, y2-1,
    (flags & D_SELECTED)? makecol(128, 128, 128): makecol(255, 255, 255),
    (flags & D_SELECTED)? makecol(255, 255, 255): makecol(128, 128, 128));

  ase_rectgouraud(bmp, x1+2, y1+2, x2-2, y2-2, top, bottom);

  if (flags & D_GOTFOCUS) {
    if (flags & D_SELECTED)
      ase_rectdotted(bmp, x1+3, y1+3, x2-2, y2-2, fg, bg);
    else
      ase_rectdotted(bmp, x1+2, y1+2, x2-3, y2-3, fg, bg);
  }
}



int d_aase_box_proc(int msg, DIALOG *d, int c)
{
  (void)c;

  if (msg == MSG_DRAW)
    ase_rectbox(gui_get_screen(), d->x, d->y, d->x+d->w-1, d->y+d->h-1, d->flags);

  return D_O_K;
}



int d_aase_shadow_box_proc(int msg, DIALOG *d, int c)
{
  return d_aase_box_proc(msg, d, c);
}



int d_aase_button_proc(int msg, DIALOG *d, int c)
{
  if (msg == MSG_DRAW) {
    ase_rectbox(gui_get_screen(), d->x, d->y, d->x+d->w-1, d->y+d->h-1, d->flags);

    if (d->dp) {
      gui_textout_ex(gui_get_screen(), (char *)d->dp,
        ((d->flags & D_SELECTED)? 1: 0) + d->x+d->w/2,
        ((d->flags & D_SELECTED)? 1: 0) + d->y+d->h/2-text_height(font)/2,
        ((d->flags & D_DISABLED)? makecol(128, 128, 128): makecol(0, 0, 0)), -1,
        TRUE);
    }

    return D_O_K;
  }

  return d_button_proc(msg, d, c);
}



int d_aase_push_proc(int msg, DIALOG *d, int c)
{
  int ret = D_O_K;

  d->flags |= D_EXIT;

  ret |= d_aase_button_proc(msg, d, c);

  if (ret & D_CLOSE) {
    ret &= ~D_CLOSE;
    scare_mouse_area(d->x, d->y, d->x + d->w, d->y + d->h);
    object_message(d, MSG_DRAW, 0);
    unscare_mouse();

    if (d->dp3)
      ret |= ((int (*)(DIALOG *))d->dp3)(d);
  }

  return ret;
}



int d_aase_check_proc(int msg, DIALOG *d, int c)
{
  if (msg == MSG_DRAW) {
    int y = d->y+d->h/2;
    int size = 12;

    ase_rectbox(gui_get_screen(), d->x, y-size/2, d->x+size-1, y-size/2+size-1, d->flags);

    gui_textout_ex(gui_get_screen(), (char *)d->dp,
      d->x+size+4, y-text_height(font)/2,
      ((d->flags & D_DISABLED)? makecol(128, 128, 128): makecol(0, 0, 0)), -1,
      FALSE);

    return D_O_K;
  }

  return d_button_proc(msg, d, c);
}



int d_aase_radio_proc(int msg, DIALOG *d, int c)
{
  if (msg == MSG_DRAW) {
    int y = d->y+d->h/2;
    int size = 12;

    ase_rectbox(gui_get_screen(), d->x, y-size/2, d->x+size-1, y-size/2+size-1, d->flags);

    gui_textout_ex(gui_get_screen(), (char *)d->dp,
      d->x+size+4, y-text_height(font)/2,
      ((d->flags & D_DISABLED)? makecol(128, 128, 128): makecol(0, 0, 0)), -1,
      FALSE);

    return D_O_K;
  }

  return d_radio_proc(msg, d, c);
}



int d_aase_icon_proc(int msg, DIALOG *d, int c)
{
  return d_icon_proc(msg, d, c);
}



int d_aase_edit_proc(int msg, DIALOG *d, int c)
{
  int ret;

  if (msg == MSG_START) {
    /* this is mainly for `file_select', because the size that
       it put to the `gui_edit_proc' is very small */
    d->h = MAX(d->h, 3 + text_height(font) + 3);
  }
  else if (msg == MSG_DRAW) {
    int f, l, p, w, x, fg, b, scroll;
    char buf[16];
    char *s;

    agup_edit_adjust_position (d);

    if (!(d->flags & D_INTERNAL))
      d->w -= 6;

    s = (char *)d->dp;
    l = ustrlen(s);
    if (d->d2 > l)
       d->d2 = l;

    /* calculate maximal number of displayable characters */
    if (d->d2 == l)  {
       usetc(buf+usetc(buf, ' '), 0);
       x = text_length(font, buf);
    }
    else
       x = 0;

    b = 0;

    for (p=d->d2; p>=0; p--) {
       usetc(buf+usetc(buf, ugetat(s, p)), 0);
       x += text_length(font, buf);
       b++;
       if (x > d->w)
         break;
    }

    if (x <= d->w) {
       b = l;
       scroll = FALSE;
    }
    else {
       b--;
       scroll = TRUE;
    }

    d->w += 6;

    ase_rectedge(gui_get_screen(), d->x,   d->y,   d->x+d->w-1, d->y+d->h-1,
      makecol(0, 0, 0),
      makecol(255, 255, 255));

    ase_rectedge(gui_get_screen(), d->x+1, d->y+1, d->x+d->w-2, d->y+d->h-2,
      makecol(128, 128, 128),
      makecol(192, 192, 192));

    ase_rectgouraud(gui_get_screen(), d->x+2, d->y+2, d->x+d->w-3, d->y+d->h-3,
      (d->flags & D_DISABLED)? 128: 224, 255);

    fg = (d->flags & D_DISABLED) ? aase_mg_color : d->fg;
    x = 3;

    if (scroll) {
       p = d->d2-b+1;
       b = d->d2;
    }
    else
       p = 0;

    for (; p<=b; p++) {
       f = ugetat(s, p);
       usetc(buf+usetc(buf, (f) ? f : ' '), 0);
       w = text_length(font, buf);
       if (x+w > d->w-2)
          break;
       f = ((p == d->d2) && (d->flags & D_GOTFOCUS));
       textout_ex(gui_get_screen(), font, buf, d->x+x, d->y+d->h/2-text_height(font)/2,
            (f) ? d->bg : fg, (f) ? fg : -1);
       x += w;
    }

    if (d->flags & D_INTERNAL)
      d->w -= 6;

    agup_edit_restore_position (d);

    return D_O_K;
  }

  d->w -= 6;
  d->flags |= D_INTERNAL;

  ret = d_agup_adjusted_edit_proc(msg, d, c);

  d->flags &= ~D_INTERNAL;
  d->w += 6;

  return ret;
}



int d_aase_list_proc(int msg, DIALOG *d, int c)
{
  return d_list_proc(msg, d, c);
}



int d_aase_text_list_proc(int msg, DIALOG *d, int c)
{
  return d_text_list_proc(msg, d, c);
}



int d_aase_textbox_proc(int msg, DIALOG *d, int c)
{
  return d_textbox_proc(msg, d, c);
}



int d_aase_slider_proc(int msg, DIALOG *d, int c)
{
  return d_slider_proc(msg, d, c);
}



int d_aase_menu_proc(int msg, DIALOG *d, int c)
{
  return d_menu_proc(msg, d, c);
}



int d_aase_window_proc(int msg, DIALOG *d, int c)
{
  (void)c;

  if (msg == MSG_DRAW) {
    BITMAP *bmp = gui_get_screen();
    ase_rectbox(bmp, d->x, d->y, d->x+d->w-1, d->y+d->h-1, d->flags);

    if (d->dp) {
      int cl = bmp->cl, ct = bmp->ct, cr = bmp->cr, cb = bmp->cb;
      set_clip_rect(bmp, d->x, d->y, d->x+d->w-1, d->y+d->h-1);

      textout_centre_ex(bmp, font, (char *)d->dp, d->x+d->w/2, d->y+4,
         makecol(0, 0, 0), -1);

      set_clip_rect(bmp, cl, ct, cr, cb);
    }
  }

  return D_O_K;
}



int
d_aase_clear_proc (int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW)
    {
        clear_to_color (gui_get_screen(), aase_bg_color);
        return D_O_K;
    }
    return d_clear_proc (msg, d, c);
}



static struct AGUP_THEME the_theme =
{
  "ASE",
  &aase_fg_color,
  &aase_bg_color,
  &aase_mg_color,
  aase_init,
  aase_shutdown,
  d_aase_box_proc,
  d_aase_shadow_box_proc,
  d_aase_button_proc,
  d_aase_push_proc,
  d_aase_check_proc,
  d_aase_radio_proc,
  d_aase_icon_proc,
  d_aase_edit_proc,
  d_aase_list_proc,
  d_aase_text_list_proc,
  d_aase_textbox_proc,
  d_aase_slider_proc,
  d_aase_menu_proc,
  d_aase_window_proc,
  d_aase_clear_proc,
  d_agup_common_text_proc,
  d_agup_common_ctext_proc,
  d_agup_common_rtext_proc
};

AL_CONST struct AGUP_THEME *aase_theme = &the_theme;


