/*
 * This file is part of the Allegro GUI Un-uglification Project.  If you
 * are looking for GTK header files, you are looking in the wrong place.
 */

#ifndef _agup_included_agtk_h
#define _agup_included_agtk_h

extern int agtk_fg_color;
extern int agtk_bg_color;

void agtk_init(void);
void agtk_shutdown(void);

int d_agtk_box_proc(int, DIALOG *, int);
int d_agtk_shadow_box_proc(int, DIALOG *, int);
int d_agtk_button_proc(int, DIALOG *, int);
int d_agtk_push_proc(int, DIALOG *, int);
int d_agtk_check_proc(int, DIALOG *, int);
int d_agtk_radio_proc(int, DIALOG *, int);
int d_agtk_icon_proc(int, DIALOG *, int);
int d_agtk_edit_proc(int, DIALOG *, int);
int d_agtk_list_proc(int, DIALOG *, int);
int d_agtk_text_list_proc(int, DIALOG *, int);
int d_agtk_textbox_proc(int, DIALOG *, int);
int d_agtk_slider_proc(int, DIALOG *, int);
int d_agtk_menu_proc(int, DIALOG *, int);
int d_agtk_window_proc(int, DIALOG *, int);

extern AL_CONST struct AGUP_THEME *agtk_theme;

#endif
