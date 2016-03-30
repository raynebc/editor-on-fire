#ifndef EOF_MENU_CONTEXT_H
#define EOF_MENU_CONTEXT_H

extern MENU eof_right_click_menu_normal[];
extern MENU eof_right_click_menu_note[];
extern MENU eof_right_click_menu_full_screen_3d_view[];

void eof_prepare_context_menu(void);
int eof_disable_full_screen_3d(void);	//Turns off full screen 3D view
int eof_enable_full_screen_3d(void);	//Turns on full screen 3D view

#endif
