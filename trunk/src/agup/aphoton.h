/*
 * This file is part of the Allegro GUI Un-uglification Project.  If you
 * are looking for Photon header files, you are looking in the wrong place.
 */

#ifndef _agup_included_aphoton_h
#define _agup_included_aphoton_h

extern int aphoton_fg_color;
extern int aphoton_bg_color;

void aphoton_init(void);
void aphoton_shutdown(void);

int d_aphoton_box_proc(int, DIALOG *, int);
int d_aphoton_shadow_box_proc(int, DIALOG *, int);
int d_aphoton_button_proc(int, DIALOG *, int);
int d_aphoton_push_proc(int, DIALOG *, int);
int d_aphoton_check_proc(int, DIALOG *, int);
int d_aphoton_radio_proc(int, DIALOG *, int);
int d_aphoton_icon_proc(int, DIALOG *, int);
int d_aphoton_edit_proc(int, DIALOG *, int);
int d_aphoton_list_proc(int, DIALOG *, int);
int d_aphoton_text_list_proc(int, DIALOG *, int);
int d_aphoton_textbox_proc(int, DIALOG *, int);
int d_aphoton_slider_proc(int, DIALOG *, int);
int d_aphoton_menu_proc(int, DIALOG *, int);
int d_aphoton_window_proc(int, DIALOG *, int);

extern AL_CONST struct AGUP_THEME *aphoton_theme;

#endif
