/* abeos.c
 *
 * This file is part of the Allegro GUI Un-uglification Project.
 * It emulates the look of the original BeOS GUI (the GUI you don't like
 * if you don't like yellow).
 *
 * Elias Pschernig <elias@users.sourceforge.net>
 *
 * Note1: Auto-formatted with "cindent -bli0 -cli2 -i2 -cbi0 -npro -nut"
 * Note2: Not as nicely coded as the gtk/win95/photon ones, instead most
 *        of the code is stuffed into one big function :)
 */

#include <allegro.h>
#include "abeos.h"
#include "agupitrn.h"

#ifdef USEMEMWATCH
#include "../memwatch.h"
#endif

/*----------------------------------------------------------------------*/
/* Recommend fg/mg/bg colors                                            */
/*----------------------------------------------------------------------*/

int abeos_fg_color;
int abeos_mg_color;
int abeos_bg_color;

/*----------------------------------------------------------------------*/
/* Helpers                                                              */
/*----------------------------------------------------------------------*/

/* some (lots of) colors; it seems like the BeOS people tried to never
 * use the same shade of grey twice - or I was somehow fooled by a sort
 * of gamma correction or anti-aliasing on my BeOS box.
 */
static int normal[5];
static int pushed[5];
static int highlight[5];
static int disabled[5];
static int frame[5];
static int frame_highlight[5];
static int frame2[5];
static int frame2_highlight[5];
static int disabled_pushed[5];
static int scroll[5];
static int scroll_hl[5];
static int scroll_dis[5];
static int slider[5];
static int slider_dis[5];
static int menu[5];
static int shadow_color;
static int edge_color;
static int white;

/* some bitmaps */
static BITMAP *radio[6];
static BITMAP *check[6];
static BITMAP *sliderhandle[2];
static BITMAP *arrow[2];

/* Blinking cursor idea copied over from awin95.c */
static volatile int abeos_time_toggle = 0;

static void
abeos_timer (void)
{
  abeos_time_toggle ^= 1;
}

END_OF_STATIC_FUNCTION (abeos_timer);

/* used to pass different parameters to the below function */
struct BDATA
{
  union
  {
    char const *text;
    struct BSCROLL
    {
      int total, visible, pos;
    } scroll;
  } u;
};

struct datapal
{
  char c;
  int r, g, b;
};

/* supported widgets */
enum ABEOS_WIDGET_TYPE
{
  ABEOS_PUSH_BUTTON,
  ABEOS_CHECK_BUTTON,
  ABEOS_RADIO_BUTTON,
  ABEOS_FRAME,                  /* list/textbox frame, with auto-scrollbar */
  ABEOS_SCROLLBAR_HANDLE,
  ABEOS_SCROLLBAR_BACK,
  ABEOS_SLIDER,
  ABEOS_MENU_FRAME,
  ABEOS_EDITBOX_FRAME,
  ABEOS_MENUBAR,
  ABEOS_WINDOW_FRAME
};

/* One big bad function to draw the BeOS look-alike widgets. Most widgets look
 * like their real counterparts. Sometimes outer borders are missing, because
 * the Allegro GUI only allows smaller dimensions. The yellow highlight color
 * was added by me, to somehow bring in that BeOS feeling :)
 *
 */
void
beos_box (BITMAP * bmp, int x, int y, int w, int h, int flags,
          enum ABEOS_WIDGET_TYPE what, struct BDATA *bdata)
{
  switch (what)
  {
    case ABEOS_PUSH_BUTTON:    /* button */
    {
      int *c = normal;
      int tc = abeos_fg_color;
      int tx = w / 2;
      int ty = h / 2 - text_height (font) / 2;
      int center = 1;

      if (flags & D_GOTFOCUS)
        c = highlight;
      if (flags & D_DISABLED)
      {
        c = disabled;
        tc = abeos_mg_color;
      }
      if (flags & D_SELECTED)
      {
        c = pushed;
        tc = white;
      }
      if (flags & D_SELECTED && flags & D_DISABLED)
        c = disabled_pushed;

      hline (bmp, x + 1, y + 1, x + w - 2, c[2]);
      vline (bmp, x + 1, y + 2, y + h - 2, c[2]);

      hline (bmp, x + 2, y + 2, x + w - 3, c[0]);
      vline (bmp, x + 2, y + 3, y + h - 3, c[0]);
      hline (bmp, x + 3, y + 3, x + w - 4, c[1]);
      vline (bmp, x + 3, y + 4, y + h - 4, c[1]);

      rectfill (bmp, x + 4, y + 4, x + w - 4, y + h - 4, c[2]);

      hline (bmp, x + 3, y + h - 3, x + w - 3, c[3]);
      vline (bmp, x + w - 3, y + 3, y + h - 4, c[3]);
      hline (bmp, x + 2, y + h - 2, x + w - 2, c[4]);
      vline (bmp, x + w - 2, y + 2, y + h - 3, c[4]);

      hline (bmp, x + 1, y, x + w - 2, shadow_color);
      vline (bmp, x, y + 1, y + h - 2, shadow_color);
      hline (bmp, x + 1, y + h - 1, x + w - 2, shadow_color);
      vline (bmp, x + w - 1, y + 1, y + h - 2, shadow_color);

      putpixel (bmp, x, y, edge_color);
      putpixel (bmp, x + w - 1, y, edge_color);
      putpixel (bmp, x, y + h - 1, edge_color);
      putpixel (bmp, x + w - 1, y + h - 1, edge_color);

      if (bdata)
        gui_textout_ex (bmp, (char *) bdata->u.text, x + tx, y + ty, tc, -1, center);
      break;
    }
    case ABEOS_CHECK_BUTTON:
    case ABEOS_RADIO_BUTTON:
    {
      int tc = abeos_fg_color;
      int tx = w / 2;
      int ty = h / 2 - text_height (font) / 2;
      int center = 1;
      BITMAP **b = check;
      int o = 0;

      if (what == ABEOS_RADIO_BUTTON)
        b = radio;

      if (flags & D_SELECTED)
        o |= 1;
      if (flags & D_GOTFOCUS)
        o |= 2;

      if (flags & D_DISABLED)
      {
        tc = abeos_mg_color;
        o |= 4;
        o &= ~2;
      }

      rectfill (bmp, x, y, x + w - 1, y + h - 1, normal[2]);
      draw_sprite (bmp, b[o], x + 2, y + ty - 2);
      tx = 20;
      center = 0;

      gui_textout_ex (bmp, (char *) bdata->u.text, x + tx, y + ty, tc, -1, center);
      break;
    }
    case ABEOS_FRAME:
    {
      int *c = frame;

      if (flags & D_GOTFOCUS)
        c = frame_highlight;

      rect (bmp, x, y, x + w - 1, y + h - 1, c[0]);
      rect (bmp, x + 1, y + 1, x + w - 3, y + h - 3, c[1]);
      vline (bmp, x + w - 2, y + 1, y + h - 3, c[2]);
      hline (bmp, x + 1, y + h - 2, x + w - 2, c[2]);

      if (bdata)
      {
        int pos = bdata->u.scroll.pos;
        int total = bdata->u.scroll.total;
        int visible = bdata->u.scroll.visible;
        if (total > visible)
        {
          int off, off2;

          if (pos < 0)
            pos = 0;

          off = (h - 6) * pos / total;
          off2 = ((h - 6) * (pos + visible) + total - 1) / total;

          vline (bmp, x + w - 13, y + 2, y + h - 4, shadow_color);

          beos_box (bmp, x + w - 12, y + 2, 9, h - 5, flags,
                    ABEOS_SCROLLBAR_BACK, NULL);
          beos_box (bmp, x + w - 12, y + 2 + off, 9, off2 - off + 1,
                    flags, ABEOS_SCROLLBAR_HANDLE, NULL);

          if (off2 + 1 < h - 6)
            hline (bmp, x + w - 12, y + 3 + off2, x + w - 4, shadow_color);
        }
      }
      break;
    }
    case ABEOS_SCROLLBAR_HANDLE:
    {
      int *c = scroll;
      int i;

      if (flags & D_GOTFOCUS)
        c = scroll_hl;

      if (flags & D_DISABLED)
        c = scroll_dis;

      hline (bmp, x, y, x + w - 3, c[0]);
      vline (bmp, x, y + 1, y + h - 3, c[0]);
      rectfill (bmp, x + 1, y + 1, x + w - 2, y + h - 2, c[1]);
      putpixel (bmp, x + w - 2, y, c[1]);
      putpixel (bmp, x, y + h - 2, c[1]);
      hline (bmp, x, y + h - 1, x + w - 1, c[3]);
      vline (bmp, x + w - 1, y, y + h - 2, c[3]);

      for (i = 0; i < 3; i++)
      {
        int mx = x + w / 2 - 2;
        int my = y + h / 2 - 2 + (i - 1) * 8;
        if (my >= y + 2 && my + 4 < y + h - 2)
        {
          hline (bmp, mx, my, mx + 3, c[0]);
          vline (bmp, mx, my + 1, my + 3, c[0]);
          hline (bmp, mx, my + 4, mx + 4, c[3]);
          vline (bmp, mx + 4, my, my + 3, c[3]);
          rectfill (bmp, mx + 1, my + 1, mx + 3, my + 3, c[1]);
        }
      }
      break;
    }
    case ABEOS_SCROLLBAR_BACK:
    {
      int *c = scroll;

      if (flags & D_DISABLED)
        c = scroll_dis;

      hline (bmp, x, y, x + w - 2, c[3]);
      hline (bmp, x, y + 1, x + w - 2, c[3]);

      vline (bmp, x, y + 2, y + h - 1, c[3]);
      vline (bmp, x + 1, y + 2, y + h - 1, c[3]);

      rectfill (bmp, x + 2, y + 2, x + w - 2, y + h - 1, c[2]);

      vline (bmp, x + w - 1, y, y + h - 1, c[1]);
      break;
    }
    case ABEOS_SLIDER:
    {
      int vert = h > w;
      int *c = slider;
      int l, t, r, b;

      if (flags & D_DISABLED)
        c = slider_dis;

      if (vert)
      {
        l = x + (w - 6) / 2;
        r = l + 5;

        rectfill (bmp, x, y, l - 1, y + h - 1, abeos_bg_color);
        rectfill (bmp, r + 1, y, x + w - 1, y + h - 1, abeos_bg_color);

        for (t = y + h - 2; t >= y; t -= 20)
        {
          hline (bmp, x, t, l - 1, c[0]);
          hline (bmp, x, t + 1, l - 1, c[1]);
          hline (bmp, r + 1, t, x + w - 1, c[0]);
          hline (bmp, r + 1, t + 1, x + w - 1, c[1]);
        }

        t = y;
        b = y + h - 1;

      }
      else
      {
        t = y + (h - 6) / 2;
        b = t + 5;

        rectfill (bmp, x, y, x + w - 1, t - 1, abeos_bg_color);
        rectfill (bmp, x, b + 1, x + w - 1, y + h - 1, abeos_bg_color);

        for (l = x; l + 1 < x + w; l += 20)
        {
          vline (bmp, l, y, t - 1, c[0]);
          vline (bmp, l + 1, y, t - 1, c[1]);
          vline (bmp, l, b + 1, y + h - 1, c[0]);
          vline (bmp, l + 1, b + 1, y + h - 1, c[1]);
        }

        l = x;
        r = x + w - 1;
      }

      hline (bmp, l, t, r, c[0]);
      vline (bmp, l, t + 1, b, c[0]);
      hline (bmp, l + 1, b, r, c[1]);
      vline (bmp, r, t + 1, b - 1, c[1]);

      if (bdata)
      {
        int pos = bdata->u.scroll.pos;
        int total = bdata->u.scroll.total;
        int visible = bdata->u.scroll.visible;
        BITMAP *handle =
          flags & D_GOTFOCUS ? sliderhandle[1] : sliderhandle[0];

        if (total > visible)
        {
          int off;
          int a = vert ? h - 12 : w - 12;

          if (pos < 0)
            pos = 0;

          off = a * pos / total;

          if (vert)
          {
            rectfill (bmp, l + 1, y + h - off - 6, r - 1, b - 1, c[3]);
            rectfill (bmp, l + 1, t + 1, r - 1, y + h - off - 7, c[4]);
            draw_sprite (bmp, handle, x + (w - 13) / 2, t + h - off - 12);
          }
          else
          {
            rectfill (bmp, l + 1, t + 1, x + off + 6, b - 1, c[3]);
            rectfill (bmp, x + off + 7, t + 1, r - 1, b - 1, c[4]);
            draw_sprite (bmp, handle, l + off, y + (h - 13) / 2);
          }
        }
      }
      break;
    }
    case ABEOS_MENU_FRAME:
    {
      int *c = menu;

      rect (bmp, x, y, x + w - 1, y + h - 1, c[0]);

      hline (bmp, x + 1, y + 1, x + w - 3, c[1]);
      vline (bmp, x + 1, y + 2, y + h - 2, c[1]);

      //rectfill (bmp, x + 2, y + 2, x + w - 3, y + h - 3, c[2]);
      putpixel (bmp, x + 1, y + h - 2, c[2]);

      hline (bmp, x + 1, y + h - 2, x + w - 2, c[3]);
      vline (bmp, x + w - 2, y, y + h - 3, c[3]);
      break;
    }
    case ABEOS_EDITBOX_FRAME:
    {
      int *c = frame2;
      if (flags & D_GOTFOCUS)
        c = frame2_highlight;

      hline (bmp, x, y, x + w - 1, c[0]);
      vline (bmp, x, y + 1, y + h - 1, c[0]);

      hline (bmp, x + 1, y + h - 1, x + w - 1, c[4]);
      vline (bmp, x + w - 1, y + 1, y + h - 2, c[4]);

      hline (bmp, x + 1, y + 1, x + w - 2, c[1]);
      vline (bmp, x + 1, y + 2, y + h - 2, c[1]);

      hline (bmp, x + 2, y + h - 2, x + w - 2, c[3]);
      vline (bmp, x + w - 2, y + 2, y + h - 3, c[3]);
      break;
    }

      /* menu bar */
    case ABEOS_MENUBAR:
    {
      int *c = menu;

      rect (bmp, x, y, x + w - 1, y + h - 1, c[3]);

      hline (bmp, x + 1, y + 1, x + w - 3, c[1]);
      vline (bmp, x + 1, y + 2, y + h - 2, c[1]);

      rectfill (bmp, x + 2, y + 2, x + w - 3, y + h - 3, c[2]);
      putpixel (bmp, x + 1, y + h - 2, c[2]);

      hline (bmp, x + 1, y + h - 2, x + w - 2, c[3]);
      vline (bmp, x + w - 2, y, y + h - 3, c[3]);
      break;
    }
    case ABEOS_WINDOW_FRAME:
    {
      int *c = highlight;
      int tw, th;

      th = text_height (font) + 6;

      tw = bdata ? text_length (font, (char *) bdata->u.text) + 16 : 16;

      /* 6 lines for outer frame */
      hline (bmp, x, y, x + tw - 1, normal[4]);
      vline (bmp, x, y, y + h - 2, normal[4]);
      hline (bmp, x, y + h - 1, x + w - 1, shadow_color);
      vline (bmp, x + tw - 1, y + 1, y + th - 1, shadow_color);
      hline (bmp, x + tw, y + th - 1, x + w - 2, normal[4]);
      vline (bmp, x + w - 1, y + th - 1, y + h - 2, shadow_color);

      /* Yellow BeOS window head */
      hline (bmp, x + 1, y + 1, x + tw - 2, c[0]);
      vline (bmp, x + 1, y + 2, y + th - 1, c[0]);
      vline (bmp, x + tw - 2, y + 2, y + th - 1, c[4]);

      rectfill (bmp, x + 2, y + 2, x + tw - 3, y + th - 1, c[2]);

      if (bdata)
        textout_ex (bmp, font, (char *) bdata->u.text, x + 6, y + 4, abeos_fg_color, -1);

      /* window frame */
      vline (bmp, x + 1, y + th, y + h - 3, normal[1]);
      hline (bmp, x + tw - 2, y + th, x + w - 3, normal[1]);
      hline (bmp, x + 2, y + th, x + tw - 3, normal[2]);
      hline (bmp, x + 1, y + h - 2, x + w - 2, normal[4]);
      vline (bmp, x + w - 2, y + th, y + h - 3, normal[1]);

      rectfill (bmp, x + 2, y + th + 1, x + w - 3, y + h - 3, normal[2]);
      break;
    }
  }
}


static void
plot (BITMAP * bmp, char data[13][14], struct datapal *pal)
{
  int x, y, i;
  for (y = 0; y < bmp->h; y++)
  {
    for (x = 0; x < bmp->w; x++)
    {
      for (i = 0; pal[i].c; i++)
      {
        if (pal[i].c == data[y][x])
        {
          putpixel (bmp, x, y, makecol (pal[i].r, pal[i].g, pal[i].b));
          break;
        }
      }
    }
  }
}


/*----------------------------------------------------------------------*/
/* BeOS-lookalike procs                                                 */
/*----------------------------------------------------------------------*/


int
d_abeos_box_proc (int msg, DIALOG * d, int c)
{
  (void) c;
  if (msg == MSG_DRAW)
    beos_box (gui_get_screen(), d->x, d->y, d->w, d->h, d->flags, ABEOS_PUSH_BUTTON,
               NULL);

  return D_O_K;
}


int
d_abeos_shadow_box_proc (int msg, DIALOG * d, int c)
{
  (void) c;
  if (msg == MSG_DRAW)
    beos_box (gui_get_screen(), d->x, d->y, d->w, d->h, d->flags, ABEOS_PUSH_BUTTON,
              NULL);

  return D_O_K;
}


int
d_abeos_button_proc (int msg, DIALOG * d, int c)
{
  if (msg == MSG_DRAW)
  {
    struct BDATA bdata;
    bdata.u.text = (char *) d->dp;
    beos_box (gui_get_screen(), d->x, d->y, d->w, d->h, d->flags, ABEOS_PUSH_BUTTON,
              &bdata);
    return D_O_K;
  }
  return d_button_proc (msg, d, c);
}


int
d_abeos_push_proc (int msg, DIALOG * d, int c)
{
  int ret = D_O_K;
  d->flags |= D_EXIT;
  ret |= d_abeos_button_proc (msg, d, c);
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


int
d_abeos_check_proc (int msg, DIALOG * d, int c)
{
  if (msg == MSG_DRAW)
  {
    struct BDATA bdata;
    bdata.u.text = (char *) d->dp;
    beos_box (gui_get_screen(), d->x, d->y, d->w, d->h, d->flags, ABEOS_CHECK_BUTTON,
              &bdata);
    return D_O_K;
  }
  return d_button_proc (msg, d, c);
}


int
d_abeos_radio_proc (int msg, DIALOG * d, int c)
{
  if (msg == MSG_DRAW)
  {
    struct BDATA bdata;
    bdata.u.text = (char *) d->dp;
    beos_box (gui_get_screen(), d->x, d->y, d->w, d->h, d->flags, ABEOS_RADIO_BUTTON,
              &bdata);
    return D_O_K;
  }
  return d_radio_proc (msg, d, c);
}


int
d_abeos_icon_proc (int msg, DIALOG * d, int c)
{
  if (msg == MSG_DRAW)
  {
    BITMAP *bmp = gui_get_screen();
    beos_box (bmp, d->x, d->y, d->w, d->h, d->flags, ABEOS_PUSH_BUTTON,
              NULL);
    stretch_sprite (bmp, (BITMAP *) d->dp, d->x + 2, d->y + 2, d->w - 4,
                    d->h - 4);
    return D_O_K;
  }
  return d_abeos_button_proc (msg, d, c);
}


int
d_abeos_edit_proc (int msg, DIALOG * d, int c)
{
  static int old_tick = 0;

  if (old_tick != abeos_time_toggle)
  {
    old_tick = abeos_time_toggle;
    d->flags |= D_DIRTY;
  }

  if (msg == MSG_DRAW)
  {
    BITMAP *bmp = gui_get_screen();
    int l, x, b, f, p, w;
    int fg = (d->flags & D_DISABLED) ? abeos_mg_color : abeos_fg_color;
    char *s = (char *) d->dp;
    char buf[16];

    agup_edit_adjust_position (d);

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

    b = 0;                      /* num of chars to be blitted */
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

    rectfill (bmp, d->x + 2, d->y + 2, d->x + d->w - 3,
              d->y + text_height (font) + 3, white);

    for (x = 4; p <= b; p++)
    {
      f = ugetat (s, p);
      usetc (buf + usetc (buf, (f) ? f : ' '), 0);
      w = text_length (font, buf);
      f = ((p == d->d2) && (d->flags & D_GOTFOCUS));

      textout_ex (bmp, font, buf, d->x + x, d->y + 4, fg, -1);

      if (f && abeos_time_toggle)
        vline (bmp, d->x + x - 1, d->y + 3,
               d->y + text_height (font) + 3, abeos_fg_color);
      if ((x += w) + w > d->w - 4)
        break;
    }

    beos_box (bmp, d->x, d->y, d->w, text_height (font) + 8, d->flags,
              ABEOS_EDITBOX_FRAME, NULL);

    agup_edit_restore_position (d);

    return D_O_K;
  }

  if (msg == MSG_LOSTFOCUS || msg == MSG_LOSTMOUSE)
    return D_WANTFOCUS;

  return d_agup_adjusted_edit_proc (msg, d, c);
}


int
d_abeos_list_proc (int msg, DIALOG * d, int c)
{
  if (msg == MSG_DRAW)
  {
	BITMAP *bmp = gui_get_screen();
    int height, size, i, len, bar, x, y, w;
    int fg, bg;
    char *sel = (char *) d->dp2;
    char s[1024];

    (*(char *(*)(int, int *)) d->dp) (-1, &size);
    height = (d->h - 4) / text_height (font);
    bar = (size > height);
    w = (bar ? d->w - 15 : d->w - 5);

    /* draw box contents */
    for (i = 0; i < height; i++)
    {
      if (d->d2 + i < size)
      {
        if (d->d2 + i == d->d1)
        {
          fg = abeos_fg_color;
          bg = normal[3];
        }
        else if ((sel) && (sel[d->d2 + i]))
        {
          fg = abeos_fg_color;
          bg = normal[3];
        }
        else
        {
          fg = abeos_fg_color;
          bg = normal[0];
        }
        ustrzcpy (s, sizeof (s),
                  (*(char *(*)(int, int *)) d->dp) (i + d->d2, NULL));
        x = d->x + 2;
        y = d->y + 2 + i * text_height (font);
        rectfill (bmp, x, y, x + 7, y + text_height (font) - 1, bg);
        x += 8;
        len = ustrlen (s);
        while (text_length (font, s) >= MAX (d->w - 1 - (bar ? 22 : 11), 1))
        {
          len--;
          usetat (s, len, 0);
        }
        textout_ex (bmp, font, s, x, y, fg, bg);
        x += text_length (font, s);
        if (x <= d->x + 1 + w)
          rectfill (bmp, x, y, d->x + 1 + w,
                    y + text_height (font) - 1, bg);
      }
      else
      {
        rectfill (bmp, d->x + 2, d->y + 2 + i * text_height (font),
                  d->x + 1 + w, d->y + 1 + (i + 1) * text_height (font),
                  normal[0]);
      }
    }

    if (d->y + 2 + i * text_height (font) <= d->y + d->h - 3)
      rectfill (bmp, d->x + 2, d->y + 2 + i * text_height (font),
                d->x + 1 + w, d->y + d->h - 4, normal[0]);

    /* draw the frame around */
    {
      struct BDATA bdata;
      bdata.u.scroll.total = size;
      bdata.u.scroll.pos = d->d2;
      bdata.u.scroll.visible = height;
      beos_box (bmp, d->x, d->y, d->w, d->h, d->flags, ABEOS_FRAME,
                &bdata);
    }

    return D_O_K;
  }
  return d_list_proc (msg, d, c);
}


int
d_abeos_text_list_proc (int msg, DIALOG * d, int c)
{
  if (msg == MSG_DRAW)
    return d_abeos_list_proc (msg, d, c);

  return d_text_list_proc (msg, d, c);
}


int
d_abeos_textbox_proc (int msg, DIALOG * d, int c)
{
  int height, bar = 1;

  if (msg == MSG_DRAW)
  {

    height = (d->h - 8) / text_height (font);

    /* tell the object to sort of draw, but only calculate the listsize */
    _draw_textbox ((char *) d->dp, &d->d1, 0,   /* DONT DRAW anything */
                   d->d2, !(d->flags & D_SELECTED), 8,
                   d->x, d->y, d->w, d->h, (d->flags & D_DISABLED), 0, 0, 0);

    if (d->d1 > height)
      bar = 11;
    else
      d->d2 = 0;

    /* now do the actual drawing */
    _draw_textbox ((char *) d->dp, &d->d1, 1, d->d2,
                   !(d->flags & D_SELECTED), 8,
                   d->x, d->y, d->w - bar, d->h,
                   (d->flags & D_DISABLED), abeos_fg_color, white,
                   abeos_mg_color);

    /* draw the frame around */
    {
      struct BDATA bdata;
      bdata.u.scroll.total = d->d1;
      bdata.u.scroll.pos = d->d2;
      bdata.u.scroll.visible = height;
      beos_box (gui_get_screen(), d->x, d->y, d->w, d->h, d->flags, ABEOS_FRAME,
                &bdata);
    }

    return D_O_K;
  }

  return d_textbox_proc (msg, d, c);
}


int
d_abeos_slider_proc (int msg, DIALOG * d, int c)
{

  if (msg == MSG_DRAW)
  {
    struct BDATA bdata;
    bdata.u.scroll.total = d->d1;
    bdata.u.scroll.pos = d->d2;
    bdata.u.scroll.visible = 1;
    beos_box (gui_get_screen(), d->x, d->y, d->w, d->h, d->flags, ABEOS_SLIDER, &bdata);
    return D_O_K;
  }

  return d_slider_proc (msg, d, c);
}


int
d_abeos_clear_proc (int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW)
    {
        clear_to_color (gui_get_screen(), abeos_bg_color);
        return D_O_K;
    }
    return d_clear_proc (msg, d, c);
}


/*----------------------------------------------------------------------*/
/* Menus                                                                */
/*----------------------------------------------------------------------*/


static int last_x, last_y, last_w, last_h, menu_done;


static void
abeos_draw_menu (int x, int y, int w, int h)
{
  /* Idea taken from aphoton.c, because in BeOS a menu bar looks
   * different from a popup menu as well:
   * Ugly hack needed because we lack the 'bar' member
   * of the underlying MENU_INFO structure from which
   * the arguments of the functions are passed.
   */
  last_x = x;
  last_y = y;
  last_w = w;
  last_h = h;
  menu_done = FALSE;
}


static void
abeos_draw_menu_item (MENU * m, int x, int y, int w, int h, int bar, int sel)
{
  BITMAP *bmp = gui_get_screen();
  int fg, bg;
  int i, j;
  char buf[256], *tok;

  if (!menu_done)
  {
    if (bar)
      beos_box (bmp, last_x, last_y, last_w, last_h, 0, ABEOS_MENUBAR,
                NULL);
    else
      beos_box (bmp, last_x, last_y, last_w, last_h, 0, ABEOS_MENU_FRAME,
                NULL);
    menu_done = TRUE;
  }

  if (m->flags & D_DISABLED)
  {
    fg = abeos_mg_color;
    bg = menu[2];
  }
  else
  {
    fg = abeos_fg_color;
    bg = (sel) ? menu[4] : menu[2];
  }

  rectfill (bmp, x, y, x + w - 1, y + h - 1, bg);

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

    gui_textout_ex (bmp, buf, x + 8, y + 1, fg, bg, FALSE);

    if (j == '\t')
    {
      tok = m->text + i + uwidth (m->text + i);
      gui_textout_ex (bmp, tok, x + w - gui_strlen (tok) - 10, y + 1,
                      fg, bg, FALSE);
    }

    if ((m->child) && (!bar))
      draw_sprite (bmp, arrow[sel ? 1 : 0], x + w - 12, y + h / 2 - 7);
  }
  else
  {
    /* menu separator */
    hline (bmp, x, y + text_height (font) / 2 - 1, x + w - 1, menu[4]);
    hline (bmp, x, y + text_height (font) / 2, x + w - 1, menu[1]);
  }

  if (m->flags & D_SELECTED)
  {
    line (bmp, x + 1, y + text_height (font) / 2 + 1, x + 3,
          y + text_height (font) + 1, fg);
    line (bmp, x + 3, y + text_height (font) + 1, x + 6, y + 2, fg);
  }

}



int
d_abeos_menu_proc (int msg, DIALOG * d, int c)
{
  return d_menu_proc (msg, d, c);
}


/*----------------------------------------------------------------------*/
/* Window frames                                                        */
/*----------------------------------------------------------------------*/


int
d_abeos_window_proc (int msg, DIALOG * d, int c)
{

  (void) c;

  if (msg == MSG_DRAW)
  {
    struct BDATA bdata;
    bdata.u.text = (char *) d->dp;
    beos_box (gui_get_screen(), d->x, d->y, d->w, d->h, d->flags, ABEOS_WINDOW_FRAME,
              d->dp ? &bdata : NULL);
  }


  return D_O_K;
}


/*----------------------------------------------------------------------*/
/* Initialization                                                       */
/*----------------------------------------------------------------------*/


char checkdata[13][14] = {
  ".............",
  ".xxxxxxxxxxx|",
  ".x|||||||||_|",
  ".x|||||||||_|",
  ".x|||||||||_|",
  ".x|||||||||_|",
  ".x|||||||||_|",
  ".x|||||||||_|",
  ".x|||||||||_|",
  ".x|||||||||_|",
  ".x|||||||||_|",
  ".x__________|",
  ".||||||||||||"
};


char checkoverlay[13][14] = {
  "             ",
  "             ",
  "             ",
  "   oo   oo   ",
  "   ooo ooo   ",
  "    ooooo    ",
  "     ooo     ",
  "    ooooo    ",
  "   ooo ooo   ",
  "   oo   oo   ",
  "             ",
  "             ",
  "             "
};


char checkoverlay2[13][14] = {
  "xxxxxxxxxxxxx",
  "x           x",
  "x           x",
  "x           x",
  "x           x",
  "x           x",
  "x           x",
  "x           x",
  "x           x",
  "x           x",
  "x           x",
  "x           x",
  "xxxxxxxxxxxxx"
};


char radiodata[13][14] = {
  "////.....////",
  "//..xxxxx..//",
  "/.xx|||||x_|/",
  "/.x|||||||_|/",
  ".x|||||||||_|",
  ".x|||||||||_|",
  ".x|||||||||_|",
  ".x|||||||||_|",
  ".x|||||||||_|",
  "/.x|||||||_|/",
  "/.__|||||__|/",
  "//||_____||//",
  "////|||||////"
};


char radiooverlay[13][14] = {
  "             ",
  "             ",
  "    IIIII    ",
  "   IooooII   ",
  "  Ioo|oooII  ",
  "  Io|ooooII  ",
  "  IooooooII  ",
  "  IoooooIII  ",
  "  IIoooIIII  ",
  "   IIIIIII   ",
  "    IIIII    ",
  "             ",
  "             "
};


char radiooverlay2[13][14] = {
  "    xxxxx    ",
  "  xx     xx  ",
  " x         x ",
  " x         x ",
  "x           x",
  "x           x",
  "x           x",
  "x           x",
  "x           x",
  " x         x ",
  " x         x ",
  "  xx     xx  ",
  "    xxxxx    "
};


char sliderdata[13][14] = {
  " ----------  ",
  "|XXXXXXXXXX` ",
  "|XXXXXXXXX+- ",
  "|XXXXXXXXi+- ",
  "|XXXXXXXXi+- ",
  "|XXXXXXXXi+- ",
  "|XXXXXXXXi+- ",
  "|XXXXXXXXi+- ",
  "|XXXXXXXXi+- ",
  "|XXiiiiiii+- ",
  "|X+++++++++- ",
  " `---------` ",
  "             "
};


char arrowdata[13][14] = {
  "             ",
  "             ",
  " ,,          ",
  " ,|,,        ",
  " ,|||,,      ",
  " ,|__||,,    ",
  " ,|____|;,,  ",
  " ,|__;;,,    ",
  " ,|;;,,      ",
  " ,;,,        ",
  " ,,          ",
  "             ",
  "             "
};


struct datapal pal[] = {
  {'`', 0, 0, 0},
  {'x', 96, 96, 96},
  {'-', 120, 120, 120},
  {',', 128, 128, 128},
  {'+', 144, 144, 144},
  {';', 152, 152, 152},
  {'.', 184, 184, 184},
  {'i', 200, 200, 200},
  {'/', 235, 235, 235},
  {'X', 240, 240, 240},
  {'_', 216, 216, 216},
  {'|', 255, 255, 255},
  {'o', 0, 0, 229},
  {'I', 0, 0, 154},
  {0, 0, 0, 0}
};

struct datapal pal_hl[] = {
  {'/', 235, 235, 235},
  {'.', 184, 184, 184},
  {'x', 175, 123, 0},
  {'|', 250, 250, 80},
  {'_', 255, 203, 0},
  {'o', 0, 0, 229},
  {'I', 0, 0, 154},
  {'-', 120, 120, 120},
  {'+', 175, 123, 0},
  {'i', 255, 203, 0},
  {'X', 250, 250, 80},
  {'`', 0, 0, 0},
  {0, 0, 0, 0}
};

struct datapal pal_dis[] = {
  {'/', 235, 235, 235},
  {'.', 216, 216, 216},
  {'x', 152, 152, 152},
  {'|', 239, 239, 239},
  {'_', 184, 184, 184},
  {'o', 156, 156, 244},
  {'I', 102, 102, 152},
  {'-', 120, 120, 120},
  {'+', 144, 144, 144},
  {'i', 200, 200, 200},
  {'X', 240, 240, 240},
  {'`', 0, 0, 0},
  {';', 152, 152, 152},
  {',', 128, 128, 128},
  {0, 0, 0, 0}
};

void
abeos_init (void)
{
  abeos_fg_color = makecol (0, 0, 0);
  abeos_mg_color = makecol (184, 184, 184);
  abeos_bg_color = makecol (235, 235, 235);

  normal[0] = makecol (255, 255, 255);
  normal[1] = makecol (255, 255, 255);
  normal[2] = makecol (232, 232, 232);
  normal[3] = makecol (216, 216, 216);
  normal[4] = makecol (152, 152, 152);

  disabled[0] = makecol (255, 255, 255);
  disabled[1] = makecol (255, 255, 255);
  disabled[2] = makecol (232, 232, 232);
  disabled[3] = makecol (216, 216, 216);
  disabled[4] = makecol (216, 216, 216);

  highlight[0] = makecol (255, 255, 80);
  highlight[1] = makecol (255, 255, 80);
  highlight[2] = makecol (255, 203, 0);
  highlight[3] = makecol (175, 123, 0);
  highlight[4] = makecol (175, 123, 0);

  pushed[0] = makecol (0, 0, 0);
  pushed[1] = makecol (0, 0, 0);
  pushed[2] = makecol (23, 23, 23);
  pushed[3] = makecol (39, 39, 39);
  pushed[4] = makecol (103, 103, 103);

  frame[0] = makecol (152, 152, 152);
  frame[1] = makecol (144, 144, 144);
  frame[2] = makecol (235, 235, 235);
  frame[3] = makecol (216, 216, 216);
  frame[4] = makecol (0, 0, 129);

  frame_highlight[0] = makecol (152, 152, 152);
  frame_highlight[1] = makecol (0, 0, 229);
  frame_highlight[2] = makecol (235, 235, 235);
  frame_highlight[3] = makecol (216, 216, 216);
  frame_highlight[4] = makecol (0, 0, 129);

  frame2[0] = makecol (184, 184, 184);
  frame2[1] = makecol (96, 96, 96);
  frame2[2] = makecol (255, 255, 80);
  frame2[3] = makecol (135, 135, 135);
  frame2[4] = makecol (255, 255, 255);

  frame2_highlight[0] = makecol (184, 184, 184);
  frame2_highlight[1] = makecol (0, 0, 229);
  frame2_highlight[2] = makecol (255, 255, 80);
  frame2_highlight[3] = makecol (0, 0, 229);
  frame2_highlight[4] = makecol (255, 255, 255);

  disabled_pushed[0] = makecol (239, 239, 239);
  disabled_pushed[1] = makecol (239, 239, 239);
  disabled_pushed[2] = makecol (184, 184, 184);
  disabled_pushed[3] = makecol (152, 152, 152);
  disabled_pushed[4] = makecol (152, 152, 152);

  scroll[0] = makecol (255, 255, 255);
  scroll[1] = makecol (216, 216, 216);
  scroll[2] = makecol (200, 200, 200);
  scroll[3] = makecol (184, 184, 184);
  scroll[4] = makecol (96, 96, 96);

  scroll_hl[0] = makecol (255, 255, 80);
  scroll_hl[1] = makecol (255, 203, 0);
  scroll_hl[2] = makecol (200, 200, 200);
  scroll_hl[3] = makecol (175, 123, 0);
  scroll_hl[4] = makecol (96, 96, 96);

  scroll_dis[0] = makecol (255, 255, 255);
  scroll_dis[1] = makecol (240, 240, 240);
  scroll_dis[2] = makecol (240, 240, 240);
  scroll_dis[3] = makecol (216, 216, 216);
  scroll_dis[4] = makecol (216, 216, 216);

  slider[0] = makecol (144, 144, 144);
  slider[1] = makecol (255, 255, 255);
  slider[2] = makecol (0, 0, 0);
  slider[3] = makecol (255, 203, 0);
  slider[4] = makecol (152, 152, 255);

  slider_dis[0] = makecol (144, 144, 144);
  slider_dis[1] = makecol (255, 255, 255);
  slider_dis[2] = makecol (0, 0, 0);
  slider_dis[3] = makecol (255, 203, 100);
  slider_dis[4] = makecol (152, 152, 200);

  menu[0] = makecol (100, 100, 100);
  menu[1] = makecol (239, 239, 239);
  menu[2] = makecol (216, 216, 216);
  menu[3] = makecol (152, 152, 152);
  menu[4] = makecol (184, 184, 184);

  shadow_color = makecol (96, 96, 96);
  edge_color = makecol (216, 216, 216);
  white = makecol (255, 255, 255);

  check[0] = create_bitmap (13, 13);
  plot (check[0], checkdata, pal);
  check[1] = create_bitmap (13, 13);
  plot (check[1], checkdata, pal);
  plot (check[1], checkoverlay, pal);
  check[2] = create_bitmap (13, 13);
  plot (check[2], checkdata, pal_hl);
  plot (check[2], checkoverlay2, pal);
  check[3] = create_bitmap (13, 13);
  plot (check[3], checkdata, pal_hl);
  plot (check[3], checkoverlay, pal);
  plot (check[3], checkoverlay2, pal);
  check[4] = create_bitmap (13, 13);
  plot (check[4], checkdata, pal_dis);
  check[5] = create_bitmap (13, 13);
  plot (check[5], checkdata, pal_dis);
  plot (check[5], checkoverlay, pal_dis);

  radio[0] = create_bitmap (13, 13);
  plot (radio[0], radiodata, pal);
  radio[1] = create_bitmap (13, 13);
  plot (radio[1], radiodata, pal);
  plot (radio[1], radiooverlay, pal);
  radio[2] = create_bitmap (13, 13);
  plot (radio[2], radiodata, pal_hl);
  plot (radio[2], radiooverlay2, pal);
  radio[3] = create_bitmap (13, 13);
  plot (radio[3], radiodata, pal_hl);
  plot (radio[3], radiooverlay, pal);
  plot (radio[3], radiooverlay2, pal);
  radio[4] = create_bitmap (13, 13);
  plot (radio[4], radiodata, pal_dis);
  radio[5] = create_bitmap (13, 13);
  plot (radio[5], radiodata, pal_dis);
  plot (radio[5], radiooverlay, pal_dis);

  sliderhandle[0] = create_bitmap (13, 13);
  clear_to_color (sliderhandle[0], bitmap_mask_color (sliderhandle[0]));
  plot (sliderhandle[0], sliderdata, pal);
  sliderhandle[1] = create_bitmap (13, 13);
  clear_to_color (sliderhandle[1], bitmap_mask_color (sliderhandle[1]));
  plot (sliderhandle[1], sliderdata, pal_hl);

  arrow[0] = create_bitmap (13, 13);
  clear_to_color (arrow[0], bitmap_mask_color (arrow[0]));
  plot (arrow[0], arrowdata, pal);
  arrow[1] = create_bitmap (13, 13);
  clear_to_color (arrow[1], bitmap_mask_color (arrow[1]));
  plot (arrow[1], arrowdata, pal_dis);

  gui_menu_draw_menu = abeos_draw_menu;
  gui_menu_draw_menu_item = abeos_draw_menu_item;

  LOCK_VARIABLE (abeos_time_toggle);
  LOCK_FUNCTION (abeos_timer);

  install_int (abeos_timer, 500);
}


void
abeos_shutdown (void)
{
  gui_menu_draw_menu_item = NULL;
  gui_menu_draw_menu = NULL;

  destroy_bitmap (check[0]);
  destroy_bitmap (check[1]);
  destroy_bitmap (check[2]);
  destroy_bitmap (check[3]);
  destroy_bitmap (check[4]);
  destroy_bitmap (check[5]);
  destroy_bitmap (radio[0]);
  destroy_bitmap (radio[1]);
  destroy_bitmap (radio[2]);
  destroy_bitmap (radio[3]);
  destroy_bitmap (radio[4]);
  destroy_bitmap (radio[5]);
  destroy_bitmap (sliderhandle[0]);
  destroy_bitmap (sliderhandle[1]);
  destroy_bitmap (arrow[0]);
  destroy_bitmap (arrow[1]);

  remove_int (abeos_timer);
}


/*----------------------------------------------------------------------*/
/* Theme                                                                */
/*----------------------------------------------------------------------*/

static struct AGUP_THEME the_theme = {
  "BeOS",
  &abeos_fg_color,
  &abeos_bg_color,
  &abeos_mg_color,
  abeos_init,
  abeos_shutdown,
  d_abeos_box_proc,
  d_abeos_shadow_box_proc,
  d_abeos_button_proc,
  d_abeos_push_proc,
  d_abeos_check_proc,
  d_abeos_radio_proc,
  d_abeos_icon_proc,
  d_abeos_edit_proc,
  d_abeos_list_proc,
  d_abeos_text_list_proc,
  d_abeos_textbox_proc,
  d_abeos_slider_proc,
  d_abeos_menu_proc,
  d_abeos_window_proc,
  d_abeos_clear_proc,
  d_agup_common_text_proc,
  d_agup_common_ctext_proc,
  d_agup_common_rtext_proc
};


AL_CONST struct AGUP_THEME *abeos_theme = &the_theme;



/*
 * Local Variables:
 * c-basic-offset: 2
 * End:
 */
