#include <allegro.h>
#include <a5alleg.h>
#include <allegro5/allegro5.h>
#include <allegro5/allegro_native_dialog.h>
#include "native.h"
#include "../dialog.h"

/* Each Allegro 5 menu ID is associated with all of this data. We use this data
   to keep the Allegro 5 menu state synchronized with its Allegro 4 counterpart.
	 */
typedef struct
{

	/* Data we use to know which Allegro 5 menu and item each A4 item is
	   associated with. */
	MENU * menu_item;

	/* Remember old values so we can compare to update values in user menus. */
	char * name;
	int flags;

} A4_MENU_ITEM_DATA;

static ALLEGRO_MENU * native_menu[EOF_MAX_NATIVE_MENUS] = {NULL};
static int native_menu_items[EOF_MAX_NATIVE_MENUS] = {0};
static A4_MENU_ITEM_DATA * a4_menu_item_data = NULL;

static int current_menu = 0;
static int current_id = 1;

static ALLEGRO_EVENT_QUEUE * event_queue = NULL;

static const char * get_menu_text(const char * in, char * out)
{
	int i;
	int c = 0;

	if(!in || !strlen(in))
	{
		strcpy(out, "placeholder");
		return out;
	}
	#ifdef ALLEGRO_WINDOWS
		strcpy(out, in);
	#else
		for(i = 0; i < strlen(in); i++)
		{
			if(in[i] == '\t')
			{
				break;
			}
			else if(in[i] != '&')
			{
				out[c] = in[i];
				c++;
				out[c] = 0;
			}
		}
	#endif
	return out;
}

static bool index_a4_menu_item(int id, MENU * mp)
{
	a4_menu_item_data[id].menu_item = mp;
	a4_menu_item_data[id].name = mp->text ? malloc(strlen(mp->text) + 1) : NULL;
	if(a4_menu_item_data[id].name)
	{
		strcpy(a4_menu_item_data[id].name, mp->text);
	}
	a4_menu_item_data[id].flags = mp->flags;

	return true;
}

static bool update_a4_menu_item(int id, const char * caption, int flags)
{
	if(a4_menu_item_data[id].name)
	{
		free(a4_menu_item_data[id].name);
	}
	a4_menu_item_data[id].name = caption ? malloc(strlen(caption) + 1) : NULL;
	if(a4_menu_item_data[id].name)
	{
		strcpy(a4_menu_item_data[id].name, caption);
	}
	a4_menu_item_data[id].flags = flags;
	return true;
}

static bool add_menu(MENU * mp, MENU * parent, int parent_pos)
{
	int flags;
	int this_menu = current_menu;

	if(current_menu >= EOF_MAX_NATIVE_MENUS)
	{
		return false;
	}

	native_menu[current_menu] = al_create_menu();
	if(!native_menu[current_menu])
	{
		return false;
	}
	current_menu++;

	while(mp && mp->text)
	{
		/* Allegro 4 menu items are also menus if they have a child. Add new native
	     menu if we have a child. */
		if(mp->child)
		{
			if(!add_menu(mp->child, mp, this_menu))
			{
				return false;
			}
		}
		/* Add menu item to current menu. */
		else if (mp->proc)
		{
			flags = 0;
			if(mp->flags & D_SELECTED)
			{
				flags = ALLEGRO_MENU_ITEM_CHECKED;
			}
			if(mp->flags & D_DISABLED)
			{
				flags |= ALLEGRO_MENU_ITEM_DISABLED;
			}
			if(mp->flags & D_USER)
			{
				flags |= ALLEGRO_MENU_ITEM_CHECKBOX;
			}
			al_append_menu_item(native_menu[this_menu], mp->text, current_id, flags, NULL, NULL);
			index_a4_menu_item(current_id, mp);
			native_menu_items[this_menu]++;
			current_id++;
		}
		else /* A separator */
			al_append_menu_item(native_menu[this_menu], NULL, 0, 0, NULL, NULL);

		mp++;
	}

	/* Add menu item for submenu. */
	if(parent && parent_pos >= 0)
	{
		flags = 0;
		if(parent->flags & D_SELECTED)
		{
			flags = ALLEGRO_MENU_ITEM_CHECKED;
		}
		if(parent->flags & D_DISABLED)
		{
			flags = ALLEGRO_MENU_ITEM_DISABLED;
		}
		if(parent->text)
		{
			al_append_menu_item(native_menu[parent_pos], parent->text, current_id, flags, NULL, native_menu[this_menu]);
			index_a4_menu_item(current_id, parent);
			native_menu_items[parent_pos]++;
			current_id++;
		}
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
}

bool eof_set_up_native_menus(MENU * mp)
{
	int i = 0;
	int pos = 0;

	event_queue = al_create_event_queue();
	if(!event_queue)
	{
		return false;
	}
	a4_menu_item_data = malloc(sizeof(A4_MENU_ITEM_DATA) * EOF_MAX_MENU_ITEMS);
	if(!a4_menu_item_data)
	{
		return false;
	}
	if(!add_menu(mp, NULL, -1))
	{
		destroy_native_menus();
		return false;
	}
	al_register_event_source(event_queue, al_get_default_menu_event_source());
	al_set_display_menu(all_get_display(), native_menu[0]);
	return true;
}

static bool caption_changed(const char * mcap, const char * nmcap)
{
	if(mcap && !nmcap)
	{
		return true;
	}
	else if(nmcap && !mcap)
	{
		return true;
	}
	else if(mcap && nmcap && strcmp(mcap, nmcap))
	{
		return true;
	}
	return false;
}

static bool update_native_menu(int m)
{
	ALLEGRO_MENU * mp = NULL;
	int index = -1;
	int i;
	bool update = false;
	int new_flags = 0;

	if(caption_changed(a4_menu_item_data[m].name, a4_menu_item_data[m].menu_item->text))
	{
		al_set_menu_item_caption(native_menu[0], m, a4_menu_item_data[m].menu_item->text ? a4_menu_item_data[m].menu_item->text : "blank");
		update = true;
	}
	if((a4_menu_item_data[m].flags & ~D_USER) != (a4_menu_item_data[m].menu_item->flags & ~D_USER))
	{
		if(a4_menu_item_data[m].menu_item->flags & D_DISABLED)
		{
			new_flags |= ALLEGRO_MENU_ITEM_DISABLED;
		}
		if(a4_menu_item_data[m].menu_item->flags & D_SELECTED)
		{
			new_flags |= ALLEGRO_MENU_ITEM_CHECKED;
		}
		al_set_menu_item_flags(native_menu[0], m, new_flags);
		update = true;
	}
	if(update)
	{
		update_a4_menu_item(m, a4_menu_item_data[m].menu_item->text, a4_menu_item_data[m].menu_item->flags);
	}
	return true;
}

void eof_update_native_menus(void)
{
	int i, j;
	const char * caption;

	for(i = 1; i < current_id; i++)
	{
		update_native_menu(i);
	}
}

static void call_menu_proc(int id)
{
	if(a4_menu_item_data[id].menu_item->proc)
	{
		a4_menu_item_data[id].menu_item->proc();
	}
}

void eof_handle_native_menu_clicks(void)
{
	ALLEGRO_EVENT event;

	while(!al_event_queue_is_empty(event_queue))
	{
		al_get_next_event(event_queue, &event);
		if(event.type == ALLEGRO_EVENT_MENU_CLICK)
		{
			call_menu_proc(event.user.data1);
			eof_prepare_menus();
		}
	}
}
