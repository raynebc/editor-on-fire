/*
 * This file is part of the Allegro GUI Un-uglification Project.
 */

#ifndef _agup_included_agupitrn_h
#define _agup_included_agupitrn_h

struct AGUP_THEME {
    char *name;
    int *fg_color;
    int *bg_color;
    int *mg_color;
    void (*init)(void);
    void (*shutdown)(void);
    int (*box_proc)(int, DIALOG *, int);
    int (*shadow_box_proc)(int, DIALOG *, int);
    int (*button_proc)(int, DIALOG *, int);
    int (*push_proc)(int, DIALOG *, int);
    int (*check_proc)(int, DIALOG *, int);
    int (*radio_proc)(int, DIALOG *, int);
    int (*icon_proc)(int, DIALOG *, int);
    int (*edit_proc)(int, DIALOG *, int);
    int (*list_proc)(int, DIALOG *, int);
    int (*text_list_proc)(int, DIALOG *, int);
    int (*textbox_proc)(int, DIALOG *, int);
    int (*slider_proc)(int, DIALOG *, int);
    int (*menu_proc)(int, DIALOG *, int);
    int (*window_proc)(int, DIALOG *, int);
    int (*clear_proc)(int, DIALOG *, int);
    int (*text_proc)(int, DIALOG *, int);
    int (*ctext_proc)(int, DIALOG *, int);
    int (*rtext_proc)(int, DIALOG *, int);
};

extern AL_CONST struct AGUP_THEME *agup_theme;
extern int agup_themes_count;
extern struct AGUP_THEME const *agup_themes_list[];

int d_agup_adjusted_edit_proc(int msg, DIALOG *d, int c);
void agup_edit_adjust_position (DIALOG *d);
void agup_edit_restore_position (DIALOG *d);

int d_agup_common_text_proc (int msg, DIALOG *d, int c);
int d_agup_common_ctext_proc (int msg, DIALOG *d, int c);
int d_agup_common_rtext_proc (int msg, DIALOG *d, int c);

#include <allegro/internal/aintern.h>

#endif
