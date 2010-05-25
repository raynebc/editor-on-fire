#include <allegro.h>
#include <stdio.h>
#include "../agup/agup.h"

int eof_verified_edit_proc(int msg, DIALOG *d, int c)
{
	int i;
	char * string = NULL;
	int key_list[32] = {KEY_BACKSPACE, KEY_DEL, KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN, KEY_ESC};
	int always_allowed = 0;
	
	if(msg == MSG_CHAR)
	{
		
		for(i = 0; i < 7; i++)
		{
			if(c >> 8 == key_list[i])
			{
				always_allowed = 1;
				break;
			}
		}
		
		if(!always_allowed)
		{
			/* see if key is an allowed key */
			string = (char *)(d->dp2);
			for(i = 0; i < strlen(string); i++)
			{
				if(string[i] == (c & 0xff))
				{
					break;
				}
			}
			if(i < strlen(string))
			{
				return d_agup_edit_proc(msg, d, c);
			}
			else
			{
				return D_USED_CHAR;
			}
		}
	}
	return d_agup_edit_proc(msg, d, c);
}
