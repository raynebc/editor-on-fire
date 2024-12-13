#ifndef EOF_MENU_NATIVE_H
#define EOF_MENU_NATIVE_H

#include <allegro.h>

#define EOF_MAX_NATIVE_MENUS 256
#define EOF_MAX_MENU_ITEMS  1024

bool eof_set_up_native_menus(MENU * mp);
void eof_update_native_menus(void);
void eof_handle_native_menu_clicks(void);

#endif
