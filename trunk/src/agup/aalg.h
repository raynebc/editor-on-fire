#ifndef _agup_included_aalg_h
#define _agup_included_aalg_h

void aalg_init(void);
void aalg_shutdown(void);

int d_aalg_box_proc(int, DIALOG *, int);
int d_aalg_shadow_box_proc(int, DIALOG *, int);
int d_aalg_button_proc(int, DIALOG *, int);
int d_aalg_push_proc(int, DIALOG *, int);
int d_aalg_check_proc(int, DIALOG *, int);
int d_aalg_radio_proc(int, DIALOG *, int);
int d_aalg_icon_proc(int, DIALOG *, int);
int d_aalg_edit_proc(int, DIALOG *, int);
int d_aalg_list_proc(int, DIALOG *, int);
int d_aalg_text_list_proc(int, DIALOG *, int);
int d_aalg_textbox_proc(int, DIALOG *, int);
int d_aalg_slider_proc(int, DIALOG *, int);
int d_aalg_menu_proc(int, DIALOG *, int);
int d_aalg_window_proc(int, DIALOG *, int);

extern AL_CONST struct AGUP_THEME *aalg_theme;

#endif
