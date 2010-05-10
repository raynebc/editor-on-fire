/* examp.c
 *
 * This file is part of the Allegro GUI Un-uglification Project.
 * It is a cheap demonstration of how pretty we are.
 *
 * Peter Wang <tjaden@users.sourceforge.net>
 */


#include <allegro.h>
#include <string.h>
#include <stdio.h>
#include "agup.h"

/*----------------------------------------------------------------------*/
/* Main dialog stuff                                                    */
/*----------------------------------------------------------------------*/


extern DIALOG main_dlg[];
extern MENU menu_theme[];
extern MENU menu_font[];

FONT *oldfont, *newfont;


int select_theme(void)
{
    int c;
    AGUP_THEME const *theme = agup_theme_by_name (active_menu->dp);

    if (!theme)
       return D_O_K;

    agup_shutdown();
    agup_init(theme);

    /* Clear all theme menu selections. */
    for (c=0; menu_theme[c].proc; c++)
        menu_theme[c].flags &=~ D_SELECTED;
    /* Select the current theme. */
    active_menu->flags |= D_SELECTED;

    /* Set the dialog color (useful for the Allegro theme only). */
    gui_fg_color = agup_fg_color;
    gui_bg_color = agup_bg_color;
    set_dialog_color(main_dlg, gui_fg_color, gui_bg_color);

    return D_REDRAW;
}


int select_font(void)
{
    int i;
    /* Clear all theme menu selections. */
    for (i = 0; menu_font[i].proc; i++)
        menu_font[i].flags &=~ D_SELECTED;
    /* Select the current theme. */
    active_menu->flags |= D_SELECTED;

    if (!strcmp (active_menu->text, "&Allegro"))
        font = oldfont;
    else
        font = newfont;

    return D_REDRAW;
}


int file_selector(void)
{
    char path[1024] = "";
    file_select_ex("Testing...", path, NULL, sizeof path, 0, 0);
    return D_REDRAW;
}


int demo_windows(void)
{
    DIALOG dlg[] =
    {
        { d_agup_window_proc, 0,   0,   300, 200, agup_fg_color, agup_bg_color, 0, 0, 0, 0, (void *)"A window",   NULL, NULL },
        { d_ctext_proc,       160, 100, 0,   0,   agup_fg_color, agup_bg_color, 0, 0, 0, 0, (void *)"Press ESC.", NULL, NULL },
        { NULL,               0,   0,   0,   0,   0,             0,             0, 0, 0, 0, NULL,                 NULL, NULL }
    };

    centre_dialog(dlg);
    do_dialog(dlg, -1);

    return D_REDRAW;
}


int surprise()
{
    alert("You pushed my button!", "Isn't this a useful feature?", 0,
          "&Yes", "&Definitely", 'y', 'd');

    return D_O_K;
}


int quit_proc(int msg, DIALOG *d, int c)
{
    (void)d;
    (void)c;

    return (((msg == MSG_KEY) || (msg == MSG_XCHAR && c == KEY_F10 << 8))
            && alert(0, "Quit?", 0, "Ok", "Cancel", 0, 0) == 1 ? D_CLOSE : D_O_K);
}


char *lister(int index, int *list_size)
{
    static char items[10][32] = {"box", "text", "input", "button", "menu",
       "selection", "slider", "scrollbar", "list", "window"};
    if (index >= 0) {
        return items[index];
    }
    else {
        index = 10;
        *list_size = index;
    }

    return NULL;
}


int toggle_focus(void)
{
    gui_mouse_focus ^= 1;
    if (gui_mouse_focus)
        active_menu->flags |= D_SELECTED;
    else
        active_menu->flags &= ~D_SELECTED;
    return D_O_K;
}


char version[1024];
char sel[10];
char sel2[10];
char buf[101] = "101";
char buf2[101] = "dalmations";
char buf3[1024] = "   AGUP\n"
                  "__________\n"
                  "\n"
                  " Allegro\n"
                  "   GUI\n"
                  " Un-ugli-\n"
                  " fication\n"
                  " Project\n"
                  "\n\n"
                  " Widget\n"
                  "  Sets\n"
                  "__________\n"
                  "\n"
                  "- Allegro\n"
                  "- GTK\n"
                  "- BeOS\n"
                  "- Photon\n"
                  "- Win95\n"
                  "- NeXTStep\n"
                  "- ASE\n"
                  "\n\n"
                  "    By\n"
                  "__________\n"
                  "\n"
                  "Shawn Hargreaves\n\n"
                  "Peter Wang\n\n"
                  "Elias Pschernig\n\n"
                  "Eric Botcazou\n\n"
                  "Robert Ohannessian\n\n"
                  "Joao Neves\n\n"
                  "David A. Capello\n\n"
                  "__________";

MENU menu12[] =
{
    /* text             proc              child     flags             dp */
    { "Submenu",        0,                0,        0,                0 },
    { "",               0,                0,        0,                0 },
    { "Checked",        0,                0,        D_SELECTED,       0 },
    { "Disabled",       0,                0,        D_DISABLED,       0 },
    { NULL,             0,                0,        0,                0 },
};


MENU menu1[] =
{
    /* text             proc              child     flags     dp */
    { "&File selector", file_selector,    0,        0,        0 },
    { "&Window frames", demo_windows,     0,        0,        0 },
    { "Test &1",        0,                0,        0,        0 },
    { "Test &2",        0,                menu12,   0,        0 },
    { "Test &3\thi",    0,                0,        0,        0 },
    { NULL,             0,                0,        0,        0 },
};


MENU menu_theme[] =
{
    /* text              proc                 child    flags              dp */
    { "&GTK",            select_theme,        0,        D_SELECTED,       "GTK" },
    { "&Photon",         select_theme,        0,        0,                "Photon" },
    { "&Win95",          select_theme,        0,        0,                "Win95" },
    { "&ASE",            select_theme,        0,        0,                "ASE" },
    { "&BeOS",           select_theme,        0,        0,                "BeOS" },
    { "&NeXTStep",       select_theme,        0,        0,                "NeXTStep" },
    { "A&llegro",        select_theme,        0,        0,                "Allegro" },
    { "Bl&ue",           select_theme,        0,        D_DISABLED,       "Blue" },
    { "&Fleur de Lis",   select_theme,        0,        D_DISABLED,       "Fleur de Lis" },
    { "&Mud",            select_theme,        0,        D_DISABLED,       "Mud" },
    { "",                NULL,                0,        0,                NULL },
    { "&Focus Follow Mouse", toggle_focus,    0,        0,                NULL},
    { NULL,              0,                   0,        0,                NULL},
};


MENU menu_font[] =
{
    /* text              proc                 child    flags              dp */
    { "&Allegro",        select_font,         0,        0,                NULL },
    { "&Clean",          select_font,         0,        D_SELECTED,       NULL },
    { NULL,                 0,                0,        0,                NULL },
};


MENU menubar[] =
{
    /* text             proc      child             flags     dp */
    { "&Menu 1",        0,        menu1,            0,        0 },
    { "&Theme",         0,        menu_theme,       0,        0 },
    { "&Font",          0,        menu_font,        0,        0 },
    { NULL,             0,        0,                0,        0 },
};


#define DIS D_DISABLED
#define SEL D_SELECTED


DIALOG main_dlg[] =
{
    /* proc                   x     y     w    h   fg    bg    key  flags    d1    d2  dp                    dp2    dp3 */
    { d_agup_clear_proc,      0,     0, 320, 240,  0,    0,    0,   0,       0,    0,  0,                    0,     0 },

    /* First column */
    { d_agup_check_proc,        5,  25, 100,  12,  0,    0,    0,   SEL,     0,    0,  (void *)"check box",  0,     0 },
    { d_agup_radio_proc,        5,  50, 100,  12,  0,    0,    0,   SEL,     1,    0,  (void *)"radio 1",    0,     0 },
    { d_agup_radio_proc,        5,  75, 100,  12,  0,    0,    0,   0,       1,    0,  (void *)"radio 2",    0,     0 },
    { d_agup_shadow_box_proc,   5, 100, 100,  20,  0,    0,    0,   0,       0,    0,  0,                    0,     0 },
    { d_agup_slider_proc,       5, 125, 100,  12,  0,    0,    0,   0,       100, 50,  0,                    0,     0 },
    { d_agup_edit_proc,         5, 150, 100,  14,  0,    0,    0,   0,       100,  0,  buf2,                 0,     0 },
    { d_agup_list_proc,         5, 175, 100,  50,  0,    0,    0,   0,       2,    0,  lister,               sel2,  0 },

    /* Buttons */
    { d_agup_button_proc,     110,  25, 100,  20,  0,    0,    0,   0,       0,    0,  (void *)"button",     0,     0 },
    { d_agup_push_proc,       110,  50, 100,  20,  0,    0,    0,   0,       0,    0,  (void *)"button",     0,     surprise },
    { d_agup_button_proc,     215,  25, 100,  20,  0,    0,    0,   DIS,     0,    0,  (void *)"button",     0,     0 },
    { d_agup_button_proc,     215,  50, 100,  20,  0,    0,    0,   DIS|SEL, 0,    0,  (void *)"button",     0,     0 },

    /* Misc */
    #define ICON_DIALOG 12
    { d_agup_icon_proc,       240,  75,  50,  50,  0,    0,    0,   0,       2,    2,  0,                    0,     0 },
    { d_agup_text_proc,       240, 130,  75,  12,  0,    0,    0,   0,       0,    0,  (void *)"A",          0,     0 },
    { d_agup_ctext_proc,      240, 150,  75,  12,  0,    0,    0,   0,       0,    0,  (void *)"B",          0,     0 },
    { d_agup_rtext_proc,      240, 170,  75,  12,  0,    0,    0,   0,       0,    0,  (void *)"C",          0,     0 },
    { d_agup_textbox_proc,    110,  75, 100, 150,  0,    0,    0,   0,       0,    0,  buf3,                 0,     0 },
    { d_agup_slider_proc,     215,  75,  12, 100,  0,    0,    0,   0,       100, 50,  0,                    0,     0 },
    { d_agup_box_proc,        215, 185, 100,  20,  0,    0,    0,   0,       0,    0,  0,                    0,     0 },
    { d_agup_slider_proc,     215, 213, 100,  12,  0,    0,    0,   DIS,     100,  0,  0,                    0,     0 },
    { d_agup_menu_proc,         5,   5, 310,  12,  0,    0,    0,   0,       0,    0,  menubar,              0,     0 },
    { quit_proc,              0,     0,   0,   0,  0,    0,    27,  0,       0,    0,  0,                    0,     0 },
    { d_agup_text_proc,       0,   232, 320,   8,  0,    0,    0,   0,       0,    0,  version,              0,     0 },
    { d_yield_proc,           0,     0,   0,   0,  0,    0,    0,   0,       0,    0,  0,                    0,     0 },
    { NULL,                   0,     0,   0,   0,  0,    0,    0,   0,       0,    0,  0,                    0,     0 }
};



static int work_around_fsel_bug = 0;
/* The AGUP d_agup_edit_proc isn't exactly compatible to Allegro's stock
 * d_edit_proc, which has no border at all. Therefore we resort to this little
 * hack.
 */
int d_hackish_edit_proc (int msg, DIALOG *d, int c)
{
    if (msg == MSG_START && !work_around_fsel_bug)
    {
        /* Adjust position/dimension so it is the same as AGUP's. */
        d->y -= 3;
        d->h += 6;
        /* The Allegro GUI has a bug where it repeatedely sends MSG_START to a
         * custom GUI procedure. We need to work around that.
         */
        work_around_fsel_bug = 1;
    }
    if (msg == MSG_END && work_around_fsel_bug)
    {
        d->y += 3;
        d->h -= 6;
        work_around_fsel_bug = 0;
    }
    return d_agup_edit_proc (msg, d, c);
}


void do_main_dialog()
{
    BITMAP *icon;

    agup_init(agtk_theme);

    /* Make Allegro's dialogs also use AGUP. See the Allegro docs for
     * details.
     */
    gui_fg_color = agup_fg_color;
    gui_bg_color = agup_bg_color;
    gui_mg_color = agup_mg_color;
    gui_shadow_box_proc = d_agup_shadow_box_proc;
    gui_button_proc = d_agup_button_proc;
    gui_ctext_proc = d_agup_ctext_proc;
    gui_text_list_proc = d_agup_text_list_proc;
    /* The file selector has a very small edit proc. This is a small hack
     * to make the edit box enlarge itself.
     */
    gui_edit_proc = d_hackish_edit_proc;

    if (agup_load_bitmap_theme ("themes/blue/agup.cfg", NULL))
        menu_theme[7].flags = 0;
    if (agup_load_bitmap_theme ("themes/fdl/agup.cfg", NULL))
        menu_theme[8].flags = 0;
    if (agup_load_bitmap_theme ("themes/mud/agup.cfg", NULL))
        menu_theme[9].flags = 0;

    icon = load_bitmap("alex.pcx", 0);
    main_dlg[ICON_DIALOG].dp = icon;

    set_dialog_color(main_dlg, gui_fg_color, gui_bg_color);

    gui_mouse_focus = 0;
    do_dialog(main_dlg, -1);

    destroy_bitmap(icon);
    agup_shutdown();
}


/*----------------------------------------------------------------------*/
/* Main                                                                 */
/*----------------------------------------------------------------------*/

void closebutton (void)
{
    simulate_keypress ((KEY_ESC << 8) | scancode_to_ascii (KEY_ESC));
}

int init()
{
    allegro_init();
    install_timer();
    install_keyboard();
    install_mouse();

    set_window_title("Widgets test");
    set_color_depth(desktop_color_depth());
    if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, 320, 240, 0, 0) < 0) {
        if (set_gfx_mode(GFX_SAFE, 320, 240, 0, 0) < 0) {
            allegro_message("Could not set graphics mode.\n");
            return -1;
        }
    }

    set_close_button_callback (closebutton);

	snprintf (version, sizeof(version), "AGUP %s, Allegro %s", AGUP_VERSION, ALLEGRO_VERSION_STR);

    return 0;
}


int main(void)
{
    if (init() < 0)
        return 1;
    enable_hardware_cursor();
    show_mouse(screen);

    oldfont = font;
    font = newfont = load_font ("clean8.pcx", NULL, NULL);
    do_main_dialog();
    font = oldfont;
    return 0;
}
END_OF_MAIN()
