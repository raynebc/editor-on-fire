#include <allegro.h>
#include <allegro5/allegro5.h>
#include <allegro5/allegro_native_dialog.h>
#include "native.h"

static ALLEGRO_MENU * native_menu[EOF_MAX_NATIVE_MENUS] = {NULL};
static MENU * a4_menu[EOF_MAX_NATIVE_MENUS] = {NULL};
static int current_menu = 0;

static const char * get_menu_text(const char * in, char * out)
{
	int i;
	int c = 0;

	if(!in)
	{
		return NULL;
	}
	for(i = 0; i < strlen(in); i++)
	{
		if(in[i] != '&')
		{
			out[c] = in[i];
			c++;
			out[c] = 0;
		}
	}
	return out;
}

static bool add_menu(MENU * mp)
{
	int this_menu = current_menu;
	char buf[256];
	int flags;

	native_menu[this_menu] = al_create_menu();
	if(!native_menu[this_menu])
	{
		return false;
	}
	a4_menu[this_menu] = mp;
	current_menu++;
	while(mp)
	{
		if(mp->child)
		{
			if(!add_menu(mp))
			{
				return false;
			}
		}
		else
		{
			flags = 0;
			if(mp->flags & D_SELECTED)
			{
				flags = ALLEGRO_MENU_ITEM_CHECKED;
			}
			if(mp->flags & D_DISABLED)
			{
				flags = ALLEGRO_MENU_ITEM_DISABLED;
			}
			al_append_menu_item(native_menu[this_menu], get_menu_text(mp->text, buf), this_menu, flags, NULL, NULL);
		}
		mp++;
	}
	return true;
}

static void destroy_native_menus(void)
{
	int i;

	for(i = current_menu - 1; i >= 0; i--)
	{
		al_destroy_menu(native_menu[i]);
	}
	current_menu = 0;
}

bool eof_set_up_native_menus(MENU * mp)
{
	int i = 0;

	if(!add_menu(mp))
	{
		destroy_native_menus();
		return false;
	}
	return true;
}

void eof_update_native_menus(MENU * mp)
{
}
