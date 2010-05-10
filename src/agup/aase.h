/*
 * This file is part of the Allegro GUI Un-uglification Project.  If you
 * are looking for ASE source code, you are looking in the wrong place.
 */

#ifndef _agup_included_aase_h
#define _agup_included_aase_h

extern int aase_fg_color;
extern int aase_bg_color;

void aase_init(void);
void aase_shutdown(void);

int d_aase_box_proc(int, DIALOG *, int);
int d_aase_shadow_box_proc(int, DIALOG *, int);
int d_aase_button_proc(int, DIALOG *, int);
int d_aase_push_proc(int, DIALOG *, int);
int d_aase_check_proc(int, DIALOG *, int);
int d_aase_radio_proc(int, DIALOG *, int);
int d_aase_icon_proc(int, DIALOG *, int);
int d_aase_edit_proc(int, DIALOG *, int);
int d_aase_list_proc(int, DIALOG *, int);
int d_aase_text_list_proc(int, DIALOG *, int);
int d_aase_textbox_proc(int, DIALOG *, int);
int d_aase_slider_proc(int, DIALOG *, int);
int d_aase_menu_proc(int, DIALOG *, int);
int d_aase_window_proc(int, DIALOG *, int);

extern AL_CONST struct AGUP_THEME *aase_theme;

#endif
