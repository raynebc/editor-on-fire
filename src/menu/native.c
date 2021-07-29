#include <allegro.h>
#include <allegro5/allegro5.h>
#include <allegro5/allegro_native_dialog.h>
#include "native.h"

static ALLEGRO_MENU * native_menu[EOF_MAX_NATIVE_MENUS] = {NULL};
static MENU * a4_menu[EOF_MAX_NATIVE_MENUS] = {NULL};
static int current_menu = 0;

bool eof_set_up_native_menus(MENU * mp)
{
	return false;
}

void eof_update_native_menus(MENU * mp)
{
}
