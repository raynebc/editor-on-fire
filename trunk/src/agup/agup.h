/*
 * This file is part of the Allegro GUI Un-uglification Project.
 */

#ifndef _agup_included_agup_h
#define _agup_included_agup_h

#define AGUP_VERSION "1.0"

typedef struct AGUP_THEME AGUP_THEME;

extern int agup_fg_color;
extern int agup_bg_color;
extern int agup_mg_color;

void agup_init(AL_CONST AGUP_THEME *);
void agup_shutdown(void);
AGUP_THEME *agup_load_bitmap_theme (char const *path, DATAFILE *datafile);
AL_CONST AGUP_THEME *agup_theme_by_name (char const *name);

int d_agup_box_proc(int, DIALOG *, int);
int d_agup_shadow_box_proc(int, DIALOG *, int);
int d_agup_button_proc(int, DIALOG *, int);
int d_agup_push_proc(int, DIALOG *, int);
int d_agup_check_proc(int, DIALOG *, int);
int d_agup_radio_proc(int, DIALOG *, int);
int d_agup_icon_proc(int, DIALOG *, int);
int d_agup_edit_proc(int, DIALOG *, int);
int d_agup_list_proc(int, DIALOG *, int);
int d_agup_text_list_proc(int, DIALOG *, int);
int d_agup_textbox_proc(int, DIALOG *, int);
int d_agup_slider_proc(int, DIALOG *, int);
int d_agup_menu_proc(int, DIALOG *, int);
int d_agup_window_proc(int, DIALOG *, int);
int d_agup_clear_proc(int, DIALOG *, int);
int d_agup_text_proc(int, DIALOG *, int);
int d_agup_ctext_proc(int, DIALOG *, int);
int d_agup_rtext_proc(int, DIALOG *, int);

extern AL_CONST AGUP_THEME *aalg_theme;
extern AL_CONST AGUP_THEME *aase_theme;
extern AL_CONST AGUP_THEME *abeos_theme;
extern AL_CONST AGUP_THEME *agtk_theme;
extern AL_CONST AGUP_THEME *ans_theme;
extern AL_CONST AGUP_THEME *aphoton_theme;
extern AL_CONST AGUP_THEME *awin95_theme;

#endif
