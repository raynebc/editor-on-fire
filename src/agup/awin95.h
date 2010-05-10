/*
 * This file is part of the Allegro GUI Un-uglification Project.  If you
 * are looking for Windows source code, you are looking in the wrong place.
 */

#ifndef _agup_included_awin95_h
#define _agup_included_awin95_h

extern int awin95_fg_color;
extern int awin95_bg_color;

void awin95_init(void);
void awin95_shutdown(void);

int d_awin95_box_proc(int, DIALOG *, int);
int d_awin95_shadow_box_proc(int, DIALOG *, int);
int d_awin95_button_proc(int, DIALOG *, int);
int d_awin95_push_proc(int, DIALOG *, int);
int d_awin95_check_proc(int, DIALOG *, int);
int d_awin95_radio_proc(int, DIALOG *, int);
int d_awin95_icon_proc(int, DIALOG *, int);
int d_awin95_edit_proc(int, DIALOG *, int);
int d_awin95_list_proc(int, DIALOG *, int);
int d_awin95_text_list_proc(int, DIALOG *, int);
int d_awin95_textbox_proc(int, DIALOG *, int);
int d_awin95_slider_proc(int, DIALOG *, int);
int d_awin95_menu_proc(int, DIALOG *, int);
int d_awin95_window_proc(int, DIALOG *, int);

extern AL_CONST struct AGUP_THEME *awin95_theme;

#endif
