#include <allegro.h>
#include <stdio.h>
#include "../agup/agup.h"

#ifdef USEMEMWATCH
#include "../memwatch.h"
#endif

int eof_verified_edit_proc(int msg, DIALOG *d, int c)
{
	int i;
	char * string = NULL;
	int key_list[32] = {KEY_BACKSPACE, KEY_DEL, KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN, KEY_ESC, KEY_ENTER};
	int match = 0;

	if(msg == MSG_CHAR)
	{
		for(i = 0; i < 8; i++)
		{
			if(c >> 8 == key_list[i])			//If the input is permanently allowed
			{
				return d_agup_edit_proc(msg, d, c);	//Immediately allow the input character to be returned
			}
		}

		/* see if key is an allowed key */
		string = (char *)(d->dp2);
		if(string == NULL)	//If the accepted characters list is NULL for some reason
			match = 1;	//Implicitly accept the input character instead of allowing a crash
		else
		{
			for(i = 0; string[i] != '\0'; i++)	//Search all characters of the accepted characters list
			{
				if(string[i] == (c & 0xff))
				{
					match = 1;
					break;
				}
			}
		}

		if(!match)			//If there was no match
			return D_USED_CHAR;	//Drop the character
	}
	return d_agup_edit_proc(msg, d, c);	//Allow the input character to be returned
}
