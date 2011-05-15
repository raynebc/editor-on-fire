/* agup.c
 *
 * This file is part of the Allegro GUI Un-uglification Project.
 * It provides "theming" ability.
 *
 * Peter Wang <tjaden@users.sourceforge.net>
 */


#include <allegro.h>
#include "agup.h"
#include "agupitrn.h"

#ifdef USEMEMWATCH
#include "../memwatch.h"
#endif


#define MAX_THEMES 256
AL_CONST struct AGUP_THEME *agup_theme;
int agup_themes_count = 0;
struct AGUP_THEME const *agup_themes_list[MAX_THEMES];

int agup_fg_color;
int agup_bg_color;
int agup_mg_color;

static int backup_gui_bg_color;
static int backup_gui_mg_color;
static int backup_gui_fg_color;

static int agup_edit_recursive = 0;



static void
agup_themes_list_init (void)
{
    static int once = 0;
    if (once)
       return;
    once++;
    agup_themes_list[agup_themes_count++] = agtk_theme;
    agup_themes_list[agup_themes_count++] = aphoton_theme;
    agup_themes_list[agup_themes_count++] = awin95_theme;
    agup_themes_list[agup_themes_count++] = aase_theme;
    agup_themes_list[agup_themes_count++] = abeos_theme;
    agup_themes_list[agup_themes_count++] = ans_theme;
    agup_themes_list[agup_themes_count++] = aalg_theme;
}

/* Hack to allow the 3 pixel border around AGUP's edit box. */
int d_agup_adjusted_edit_proc(int msg, DIALOG *d, int c)
{
    int ret;
    d->x += 3;
    d->y += 3;
    agup_edit_recursive++;
    ret = d_edit_proc(msg, d, c);
    agup_edit_recursive--;
    d->x -= 3;
    d->y -= 3;
    return ret;
}


/* Handles recursive calls of d_agup_edit_proc with the above hack. */
void agup_edit_adjust_position (DIALOG *d)
{
    if (agup_edit_recursive)
    {
        d->x -= 3;
        d->y -= 3;
    }
}


/* Handles recursive calls of d_agup_edit_proc with the above hack. */
void agup_edit_restore_position (DIALOG *d)
{
    if (agup_edit_recursive)
    {
        d->x += 3;
        d->y += 3;
    }
}


int
d_agup_common_text_proc (int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW)
    {
        gui_textout_ex (gui_get_screen(), d->dp, d->x, d->y,
            *(agup_theme->fg_color), *(agup_theme->bg_color), FALSE);
        return D_O_K;
    }
    return d_text_proc (msg, d, c);
}

int
d_agup_common_ctext_proc (int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW)
    {
        gui_textout_ex (gui_get_screen(), d->dp, d->x + d->w / 2, d->y,
            *(agup_theme->fg_color), *(agup_theme->bg_color), TRUE);
        return D_O_K;
    }
    return d_ctext_proc (msg, d, c);
}

int
d_agup_common_rtext_proc (int msg, DIALOG *d, int c)
{
    if (msg == MSG_DRAW)
    {
        gui_textout_ex (gui_get_screen(), d->dp, d->x + d->w - gui_strlen(d->dp),
            d->y, *(agup_theme->fg_color), *(agup_theme->bg_color), FALSE);
        return D_O_K;
    }
    return d_rtext_proc (msg, d, c);
}


void agup_init(AL_CONST AGUP_THEME *thm)
{
    if ((agup_theme = thm)) {
        backup_gui_bg_color = gui_bg_color;
        backup_gui_mg_color = gui_mg_color;
        backup_gui_fg_color = gui_fg_color;
        agup_theme->init();
        agup_fg_color = *(agup_theme->fg_color);
        agup_bg_color = *(agup_theme->bg_color);
        agup_mg_color = *(agup_theme->mg_color);
    }
}


void agup_shutdown(void)
{
    if (agup_theme) {
        agup_theme->shutdown();
        agup_theme = NULL;
        gui_bg_color =  backup_gui_bg_color;
        gui_mg_color =  backup_gui_mg_color;
        gui_fg_color =  backup_gui_fg_color;
    }
}


AGUP_THEME const *agup_theme_by_name (char const *name)
{
    int i;
    agup_themes_list_init ();
    for (i = 0; i < agup_themes_count; i++)
    {
        if (!ustrcmp (agup_themes_list[i]->name, name))
        {
            return agup_themes_list[i];
        }
    }
    return NULL;
}


#define MAKE_WRAPPER(wrapper, proc)                                     \
int wrapper(int msg, DIALOG *d, int c)                                  \
{                                                                       \
    return ((agup_theme) && (agup_theme->proc)) ? agup_theme->proc(msg, d, c) : D_O_K; \
}

MAKE_WRAPPER(d_agup_box_proc, box_proc);
MAKE_WRAPPER(d_agup_shadow_box_proc, shadow_box_proc);
MAKE_WRAPPER(d_agup_button_proc, button_proc);
MAKE_WRAPPER(d_agup_push_proc, push_proc);
MAKE_WRAPPER(d_agup_check_proc, check_proc);
MAKE_WRAPPER(d_agup_radio_proc, radio_proc);
MAKE_WRAPPER(d_agup_icon_proc, icon_proc);
MAKE_WRAPPER(d_agup_edit_proc, edit_proc);
MAKE_WRAPPER(d_agup_list_proc, list_proc);
MAKE_WRAPPER(d_agup_text_list_proc, text_list_proc);
MAKE_WRAPPER(d_agup_textbox_proc, textbox_proc);
MAKE_WRAPPER(d_agup_slider_proc, slider_proc);
MAKE_WRAPPER(d_agup_menu_proc, menu_proc);
MAKE_WRAPPER(d_agup_window_proc, window_proc);
MAKE_WRAPPER(d_agup_clear_proc, clear_proc);
MAKE_WRAPPER(d_agup_text_proc, text_proc);
MAKE_WRAPPER(d_agup_ctext_proc, ctext_proc);
MAKE_WRAPPER(d_agup_rtext_proc, rtext_proc);
