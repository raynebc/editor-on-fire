#include <allegro.h>
#include "aalg.h"
#include "agupitrn.h"

int d_aalg_push_proc(int msg, DIALOG *d, int c)
{
   int ret = D_O_K;

   d->flags |= D_EXIT;

   ret |= d_button_proc(msg, d, c);

   if (ret & D_CLOSE) {
      ret &= ~D_CLOSE;
      ret |= D_REDRAWME;

      if (d->dp3)
      ret |= ((int (*)(DIALOG *))d->dp3)(d);
   }

   return ret;
}

int d_aalg_window_proc(int msg, DIALOG *d, int c)
{
   (void)c;

   if (msg == MSG_DRAW) {
      d_shadow_box_proc(msg, d, c);

      if (d->dp) {
	 BITMAP *bmp = gui_get_screen();
	 int cl = bmp->cl, ct = bmp->ct, cr = bmp->cr, cb = bmp->cb;
	 set_clip_rect(bmp, d->x, d->y, d->x+d->w-1, d->y+d->h-1);

	 textout_ex(bmp, font, (char *)d->dp, d->x+6, d->y+6, gui_fg_color, gui_bg_color);

	 set_clip_rect(bmp, cl, ct, cr, cb);
      }
   }

   return D_O_K;
}


static struct AGUP_THEME the_theme =
{
    "Allegro",
    NULL,
    NULL,
    NULL,
    aalg_init,
    aalg_shutdown,
    d_box_proc,
    d_shadow_box_proc,
    d_button_proc,
    d_aalg_push_proc,
    d_check_proc,
    d_radio_proc,
    d_icon_proc,
    d_edit_proc,
    d_list_proc,
    d_text_list_proc,
    d_textbox_proc,
    d_slider_proc,
    d_menu_proc,
    d_aalg_window_proc,
    d_clear_proc,
    d_text_proc,
    d_ctext_proc,
    d_rtext_proc
};

void aalg_init(void)
{
    gui_menu_draw_menu = NULL;
    gui_menu_draw_menu_item = NULL;
    the_theme.fg_color = &gui_fg_color;
    the_theme.bg_color = &gui_bg_color;
    the_theme.mg_color = &gui_mg_color;
}

void aalg_shutdown(void)
{

}

AL_CONST struct AGUP_THEME *aalg_theme = &the_theme;
