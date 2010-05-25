#include <allegro.h>
#include "../agup/agup.h"

int eof_verified_edit_box(int msg, DIALOG *d, int c)
{
	int i;
	char * string = NULL;
	
	if(msg == MSG_CHAR)
	{
		
		/* see if key is an allowed key */
		string = (char *)(d->dp2);
		for(i = 0; i < strlen(string); i++)
		{
			if(string[i] == c)
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
			return D_O_K;
		}
	}
	return d_agup_edit_proc(msg, d, c);
}
