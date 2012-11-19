#include <allegro.h>
#include <stdio.h>
#include "control.h"
#include "main.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

void eof_read_controller(EOF_CONTROLLER * cp)
{
//	eof_log("eof_read_controller() entered");

	int i;

	if(!cp)
	{
		return;
	}
	poll_joystick();
	for(i = 0; i < EOF_CONTROLLER_MAX_BUTTONS; i++)
	{
		switch(cp->button[i].type)
		{
			case EOF_CONTROLLER_BUTTON_TYPE_KEY:
			{
				if(key[cp->button[i].key])
				{
					cp->button[i].held++;
					if(cp->button[i].held == 1)
					{
						cp->button[i].pressed = 1;
					}
					else
					{
						cp->button[i].pressed = 0;
					}
				}
				else
				{
					if(cp->button[i].held)
					{
						cp->button[i].pressed = 0;
						cp->button[i].released = 1;
						cp->button[i].held = 0;
					}
					else
					{
						cp->button[i].pressed = 0;
						cp->button[i].released = 0;
					}
				}
				break;
			}
			case EOF_CONTROLLER_BUTTON_TYPE_JOYBUTTON:
			{
				if(joy[cp->button[i].joy].button[cp->button[i].key].b)
				{
					cp->button[i].held++;
					if(cp->button[i].held == 1)
					{
						cp->button[i].pressed = 1;
					}
					else
					{
						cp->button[i].pressed = 0;
					}
				}
				else
				{
					if(cp->button[i].held)
					{
						cp->button[i].pressed = 0;
						cp->button[i].released = 1;
						cp->button[i].held = 0;
					}
					else
					{
						cp->button[i].pressed = 0;
						cp->button[i].released = 0;
					}
				}
				break;
			}
			case EOF_CONTROLLER_BUTTON_TYPE_JOYAXIS:
			{
				if(cp->button[i].d == 0)
				{
					if(joy[cp->button[i].joy].stick[cp->button[i].index].axis[cp->button[i].key].d1)
					{
						cp->button[i].held++;
						if(cp->button[i].held == 1)
						{
							cp->button[i].pressed = 1;
						}
						else
						{
							cp->button[i].pressed = 0;
						}
					}
					else
					{
						if(cp->button[i].held)
						{
							cp->button[i].pressed = 0;
							cp->button[i].released = 1;
							cp->button[i].held = 0;
						}
						else
						{
							cp->button[i].pressed = 0;
							cp->button[i].released = 0;
						}
					}
				}
				else
				{
					if(joy[cp->button[i].joy].stick[cp->button[i].index].axis[cp->button[i].key].d2)
					{
						cp->button[i].held++;
						if(cp->button[i].held == 1)
						{
							cp->button[i].pressed = 1;
						}
						else
						{
							cp->button[i].pressed = 0;
						}
					}
					else
					{
						if(cp->button[i].held)
						{
							cp->button[i].pressed = 0;
							cp->button[i].released = 1;
							cp->button[i].held = 0;
						}
						else
						{
							cp->button[i].pressed = 0;
							cp->button[i].released = 0;
						}
					}
				}
				break;
			}
		}
	}
}

int eof_controller_set_button(EOF_CONTROLLER_BUTTON * bp)
{
	int i, j, k;
	int count = 0;
	char ignore[4][8][2] = {{{0}}};
	char ignoreb[4][32] = {{0}};

	eof_log("eof_controller_set_button() entered", 1);

	if(!bp)
	{
		return 0;
	}
	while(!key[KEY_ESC])
	{
		/* scan keyboard keys first */
		for(i = 0; i < KEY_MAX; i++)
		{
			if((i != KEY_SPACE) && key[i])
			{
				bp->type = EOF_CONTROLLER_BUTTON_TYPE_KEY;
				bp->key = i;
				ustrcpy(bp->name, scancode_to_name(i));
				return 1;
			}
		}

		/* scan joysticks */
		poll_joystick();
		for(i = 0; i < num_joysticks; i++)
		{

			/* scan buttons */
			for(j = 0; j < joy[i].num_buttons; j++)
			{
				if(joy[i].button[j].b)
				{
					if(count == 0)
					{
						ignoreb[i][j] = 1;
					}
					else if(!ignoreb[i][j])
					{
						bp->type = EOF_CONTROLLER_BUTTON_TYPE_JOYBUTTON;
						bp->joy = i;
						bp->key = j;
						sprintf(bp->name, "Joy %d %s", i, joy[i].button[j].name);
						return 1;
					}
				}
				else
				{
					ignoreb[i][j] = 0;
				}
			}

			/* scan sticks */
			for(j = 0; j < joy[i].num_sticks; j++)
			{
				for(k = 0; k < joy[i].stick[j].num_axis; k++)
				{
					if(joy[i].stick[j].axis[k].d1)
					{
						if(count == 0)
						{
							ignore[i][j][k] = 1;
						}
						else if(!ignore[i][j][k])
						{
							bp->type = EOF_CONTROLLER_BUTTON_TYPE_JOYAXIS;
							bp->joy = i;
							bp->index = j;
							bp->key = k;
							bp->d = 0;
							sprintf(bp->name, "Joy %d %s Axis (-)", i, joy[i].stick[j].axis[k].name);
							return 1;
						}
					}
					else if(joy[i].stick[j].axis[k].d2)
					{
						if(count == 0)
						{
							ignore[i][j][k] = 1;
						}
						else if(!ignore[i][j][k])
						{
							bp->type = EOF_CONTROLLER_BUTTON_TYPE_JOYAXIS;
							bp->joy = i;
							bp->index = j;
							bp->key = k;
							bp->d = 1;
							sprintf(bp->name, "Joy %d %s Axis (+)", i, joy[i].stick[j].axis[k].name);
							return 1;
						}
					}
				}
			}
		}
		count++;
		rest(10);
	}
	if(key[KEY_ESC])
	{	//If user pressed Escape, undefine this controller button
		ustrcpy(bp->name, "(none)");
		bp->type=bp->joy=bp->index=bp->d=bp->key=bp->held=bp->pressed=bp->released = 0;
	}
	return 0;
}

void eof_controller_read_button_names(EOF_CONTROLLER * cp)
{
	int i;

	eof_log("eof_controller_read_button_names() entered", 1);

	if(!cp)
	{
		return;
	}
	for(i = 0; i < EOF_CONTROLLER_MAX_BUTTONS; i++)
	{
		switch(cp->button[i].type)
		{
			case EOF_CONTROLLER_BUTTON_TYPE_KEY:
			{
				ustrcpy(cp->button[i].name, scancode_to_name(cp->button[i].key));
				break;
			}
			case EOF_CONTROLLER_BUTTON_TYPE_JOYBUTTON:
			{
				sprintf(cp->button[i].name, "Joy %d %s", cp->button[i].joy, joy[cp->button[i].joy].button[cp->button[i].key].name);
				break;
			}
			case EOF_CONTROLLER_BUTTON_TYPE_JOYAXIS:
			{
				sprintf(cp->button[i].name, "Joy %d %s Axis (%s)", cp->button[i].joy, joy[cp->button[i].joy].stick[cp->button[i].index].axis[cp->button[i].key].name, cp->button[i].d == 0 ? "-" : "+");
				break;
			}
		}
	}
}

void eof_controller_save_config(EOF_CONTROLLER * cp, char * name)
{
	int i;
	char string[256] = {0};

	eof_log("eof_controller_save_config() entered", 1);

	if(!cp || !name)
	{
		return;
	}
	for(i = 0; i < EOF_CONTROLLER_MAX_BUTTONS; i++)
	{
		sprintf(string, "button_%d_type", i);
		set_config_int(name, string, cp->button[i].type);
		switch(cp->button[i].type)
		{
			case EOF_CONTROLLER_BUTTON_TYPE_KEY:
			{
				sprintf(string, "button_%d_key", i);
				set_config_int(name, string, cp->button[i].key);
				break;
			}
			case EOF_CONTROLLER_BUTTON_TYPE_JOYBUTTON:
			{
				sprintf(string, "button_%d_controller", i);
				set_config_int(name, string, cp->button[i].joy);
				sprintf(string, "button_%d_key", i);
				set_config_int(name, string, cp->button[i].key);
				break;
			}
			case EOF_CONTROLLER_BUTTON_TYPE_JOYAXIS:
			{
				sprintf(string, "button_%d_controller", i);
				set_config_int(name, string, cp->button[i].joy);
				sprintf(string, "button_%d_axis", i);
				set_config_int(name, string, cp->button[i].index);
				sprintf(string, "button_%d_direction", i);
				set_config_int(name, string, cp->button[i].d);
				sprintf(string, "button_%d_key", i);
				set_config_int(name, string, cp->button[i].key);
				break;
			}
		}
	}
	set_config_int(name, "delay", cp->delay);
}

void eof_controller_load_config(EOF_CONTROLLER * cp, char * name)
{
	int i;
	char string[256] = {0};

	eof_log("eof_controller_load_config() entered", 1);

	if(!cp || !name)
	{
		return;
	}
	for(i = 0; i < EOF_CONTROLLER_MAX_BUTTONS; i++)
	{
		sprintf(string, "button_%d_type", i);
		cp->button[i].type = get_config_int(name, string, 0);
		switch(cp->button[i].type)
		{
			case EOF_CONTROLLER_BUTTON_TYPE_KEY:
			{
				sprintf(string, "button_%d_key", i);
				cp->button[i].key = get_config_int(name, string, 0);
				break;
			}
			case EOF_CONTROLLER_BUTTON_TYPE_JOYBUTTON:
			{
				sprintf(string, "button_%d_controller", i);
				cp->button[i].joy = get_config_int(name, string, 0);
				sprintf(string, "button_%d_key", i);
				cp->button[i].key = get_config_int(name, string, 0);
				break;
			}
			case EOF_CONTROLLER_BUTTON_TYPE_JOYAXIS:
			{
				sprintf(string, "button_%d_controller", i);
				cp->button[i].joy = get_config_int(name, string, 0);
				sprintf(string, "button_%d_axis", i);
				cp->button[i].index = get_config_int(name, string, 0);
				sprintf(string, "button_%d_direction", i);
				cp->button[i].d = get_config_int(name, string, 0);
				sprintf(string, "button_%d_key", i);
				cp->button[i].key = get_config_int(name, string, 0);
				break;
			}
		}
	}
	cp->delay = get_config_int(name, "delay", 0);
}
