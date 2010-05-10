/*
 * This file is part of the Allegro GUI Un-uglification Project.  If you
 * are looking for NeXTstep header files, you are looking in the wrong place.
 */

#ifndef _agup_included_ans_h
#define _agup_included_ans_h

extern int ans_fg_color;
extern int ans_bg_color;

void ans_init(void);
void ans_shutdown(void);

int d_ans_box_proc(int, DIALOG *, int);
int d_ans_shadow_box_proc(int, DIALOG *, int);
int d_ans_button_proc(int, DIALOG *, int);
int d_ans_push_proc(int, DIALOG *, int);
int d_ans_check_proc(int, DIALOG *, int);
int d_ans_radio_proc(int, DIALOG *, int);
int d_ans_icon_proc(int, DIALOG *, int);
int d_ans_edit_proc(int, DIALOG *, int);
int d_ans_list_proc(int, DIALOG *, int);
int d_ans_text_list_proc(int, DIALOG *, int);
int d_ans_textbox_proc(int, DIALOG *, int);
int d_ans_slider_proc(int, DIALOG *, int);
int d_ans_menu_proc(int, DIALOG *, int);
int d_ans_window_proc(int, DIALOG *, int);

extern AL_CONST struct AGUP_THEME *ans_theme;

#endif
