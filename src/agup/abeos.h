/*
 * This file is part of the Allegro GUI Un-uglification Project.  If you
 * are looking for BeOS header files, you are looking in the wrong place.
 */

#ifndef _agup_included_abeos_h
#define _agup_included_abeos_h

extern int abeos_fg_color;
extern int abeos_bg_color;

void abeos_init(void);
void abeos_shutdown(void);

int d_abeos_box_proc(int, DIALOG *, int);
int d_abeos_shadow_box_proc(int, DIALOG *, int);
int d_abeos_button_proc(int, DIALOG *, int);
int d_abeos_push_proc(int, DIALOG *, int);
int d_abeos_check_proc(int, DIALOG *, int);
int d_abeos_radio_proc(int, DIALOG *, int);
int d_abeos_icon_proc(int, DIALOG *, int);
int d_abeos_edit_proc(int, DIALOG *, int);
int d_abeos_list_proc(int, DIALOG *, int);
int d_abeos_text_list_proc(int, DIALOG *, int);
int d_abeos_textbox_proc(int, DIALOG *, int);
int d_abeos_slider_proc(int, DIALOG *, int);
int d_abeos_menu_proc(int, DIALOG *, int);
int d_abeos_window_proc(int, DIALOG *, int);

extern AL_CONST struct AGUP_THEME *abeos_theme;

#endif
