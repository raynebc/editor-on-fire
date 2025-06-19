#include <allegro.h>
#include <stdio.h>
#include "../agup/agup.h"
#include "../main.h"		//For declaration of eof_click_changes_dialog_focus
#include "../utility.h"	//For clipboard logic
#include "proc.h"

#ifdef USEMEMWATCH
#include "../memwatch.h"
#endif

#define EOF_MSG_BUTTONFOCUS D_USER
//Allegro's GUI system reserves bit flags 0 through 6 for its own use, the others are available for
//program use.  Define a bitflag for use to filter dialog focus changing events that use the mouse
//click from those that don't

int eof_verified_edit_proc(int msg, DIALOG *d, int c)
{
	int i, j;
	char * string = NULL;
	#define KEY_LIST_SIZE 9
	unsigned key_list[32] = {KEY_BACKSPACE, KEY_DEL, KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN, KEY_ESC, KEY_ENTER, KEY_TAB};
	int match = 0;
	unsigned c2 = (unsigned)c;	//Force cast this to unsigned because Splint is incapable of avoiding a false positive detecting it as negative despite assertions proving otherwise
	static char tabused = 0;	//Tracks whether the tab key was pressed but not completely processed yet

	if(msg == MSG_CHAR)
	{
		char do_paste = 0;
		char do_copy = 0;

		string = (char *)(d->dp2);

		if(c2 >> 8 == KEY_TAB)
			tabused = 1;

		for(i = 0; i < KEY_LIST_SIZE; i++)
		{	//Check each of the pre-defined allowable keys
			if(c2 >> 8 == key_list[i])			//If the input is permanently allowed
			{
				return d_agup_edit_proc(msg, d, c2);	//Immediately allow the input character to be returned
			}
		}

		//Paste input
		if((c2 >> 8 == KEY_V) && (KEY_EITHER_CTRL))
			do_paste = 1;	//CTRL+V was detected
		if((c2 >> 8 == KEY_INSERT) && (KEY_EITHER_SHIFT))
			do_paste = 1;	//SHIFT+Insert was detected
		if((c2 >> 8 == KEY_C) && (KEY_EITHER_CTRL))
			do_copy = 1;	//CTRL+C was detected
		if((c2 >> 8 == KEY_INSERT) && (KEY_EITHER_CTRL))
			do_copy = 1;	//CTRL+Insert was detected

		if(do_paste)
		{	//If either paste combination was detected
			if((eof_get_clipboard() == 0) && eof_os_clipboard)
			{	//If the clipboard was read and buffered
				for(i = 0; eof_os_clipboard[i] != '\0'; i++)
				{	//For each character read from the clipboard
					if(ustrlen(d->dp) < d->d1)
					{	//If the string can add one more character before its limit is reached
						for(j = 0; !string || (string[j] != '\0'); j++)	//Search all characters of the accepted characters list, if one is defined
						{
							if(!string || (string[j] == (eof_os_clipboard[i])))
							{	//If the list of acceptable characters is undefined, or if this character from the clipboard is acceptable
								uinsert(d->dp, d->d2++, eof_os_clipboard[i]);	//Insert the next character from the clipboard, advance the cursor in the input field by one character
								break;
							}
						}
					}
					else
						break;	//No more characters can be pasted
				}
				(void) object_message(d, MSG_DRAW, 0);	//Redraw the text input field
			}
			return D_USED_CHAR;	//Drop the keypress that triggered the paste
		}

		//Copy input
		if(do_copy)
		{
			(void) eof_set_clipboard(d->dp);	//Copy the contents of this input field to the clipboard
			return D_USED_CHAR;	//Drop the keypress that triggered the copy
		}

		//Normal character input
		/* see if key is an allowed key */
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
	if(msg == MSG_WANTFOCUS)
	{	//If this field wants focus
		if(!eof_click_changes_dialog_focus)
		{	//If the user preference to require a mouse click to change field focus is not enabled
			return d_agup_edit_proc(msg,d,c);	//Allow the dialog system to carry out normal logic
		}
		if(tabused)
		{	//If this dialog message was preceded by a tab keypress
			tabused = 0;
			return d_agup_edit_proc(msg,d,c);	//Allow the dialog system to carry out normal logic
		}
		if(d->flags & EOF_MSG_BUTTONFOCUS)
		{	//If a custom button focus flag was set by the click event
			d->flags &= ~EOF_MSG_BUTTONFOCUS;	//Clear it
			return D_WANTFOCUS;	//And have the dialog system switch input focus
		}
		return D_O_K;	//Otherwise deny it, because simply mousing over it can cause this message
	}
	if(msg == MSG_CLICK)
	{	//If this field is explicitly clicked on
		d->flags |= EOF_MSG_BUTTONFOCUS;	//Set this custom flag to indicate a mouse click initiated the WANTFOCUS message
		return d_agup_edit_proc(msg,d,c);	//Allow the dialog system to carry out the click processing
	}

	return d_agup_edit_proc(msg, d, c);	//Allow the input character to be returned
}

int eof_focus_controlled_edit_proc(int msg, DIALOG *d, int c)
{
	static char tabused = 0;	//Tracks whether the tab key was pressed but not completely processed yet

	if(msg == MSG_WANTFOCUS)
	{	//If this field wants focus
		if(!eof_click_changes_dialog_focus)
		{	//If the user preference to require a mouse click to change field focus is not enabled
			return d_agup_edit_proc(msg,d,c);	//Allow the dialog system to carry out normal logic
		}
		if(tabused)
		{	//If this dialog message was preceded by a tab keypress
			tabused = 0;
			return d_agup_edit_proc(msg,d,c);	//Allow the dialog system to carry out normal logic
		}
		if(d->flags & EOF_MSG_BUTTONFOCUS)
		{	//If a custom button focus flag was set by the click event
			d->flags &= ~EOF_MSG_BUTTONFOCUS;	//Clear it
			return D_WANTFOCUS;	//And have the dialog system switch input focus
		}
		return D_O_K;	//Otherwise deny it, because simply mousing over it can cause this message
	}
	if(msg == MSG_CLICK)
	{	//If this field is explicitly clicked on
		d->flags |= EOF_MSG_BUTTONFOCUS;	//Set this custom flag to indicate a mouse click initiated the WANTFOCUS message
		return d_agup_edit_proc(msg,d,c);	//Allow the dialog system to carry out the click processing
	}

	return d_agup_edit_proc(msg, d, c);	//Allow the input character to be returned
}

int eof_edit_proc(int msg, DIALOG *d, int c)
{
	unsigned long i;
	unsigned int c2 = c;	//Cast this to satisfy Splint

	if(msg == MSG_CHAR)
	{
		char do_paste = 0;
		char do_copy = 0;

		if((c2 >> 8 == KEY_V) && (KEY_EITHER_CTRL))
			do_paste = 1;	//CTRL+V was detected
		if((c2 >> 8 == KEY_INSERT) && (KEY_EITHER_SHIFT))
			do_paste = 1;	//SHIFT+Insert was detected
		if((c2 >> 8 == KEY_C) && (KEY_EITHER_CTRL))
			do_copy = 1;	//CTRL+C was detected
		if((c2 >> 8 == KEY_INSERT) && (KEY_EITHER_CTRL))
			do_copy = 1;	//CTRL+Insert was detected

		if(do_paste)
		{	//If either paste combination was detected
			if((eof_get_clipboard() == 0) && eof_os_clipboard)
			{	//If the clipboard was read and buffered
				for(i = 0; eof_os_clipboard[i] != '\0'; i++)
				{	//For each character read from the clipboard
///					simulate_ukeypress(eof_os_clipboard[i], 0);	//Add it to the keyboard buffer to be handled by d_agup_edit_proc()	//This doesn't work, characters are processed out of order
///					d_agup_edit_proc(MSG_CHAR, d, eof_os_clipboard[i]);	//Pass it as a character input message to ensure they are processed in the correct order	//This doesn't work either
					if(ustrlen(d->dp) < d->d1)
					{	//If the string can add one more character before its limit is reached
						uinsert(d->dp, d->d2++, eof_os_clipboard[i]);	//Insert the next character from the clipboard, advance the cursor in the input field by one character
					}
					else
						break;	//No more characters can be pasted
				}
				(void) object_message(d, MSG_DRAW, 0);	//Redraw the text input field
			}
			return D_USED_CHAR;	//Drop the keypress that triggered the paste
		}
		if(do_copy)
		{
			(void) eof_set_clipboard(d->dp);	//Copy the contents of this input field to the clipboard
			return D_USED_CHAR;	//Drop the keypress that triggered the copy
		}
	}

	return d_agup_edit_proc(msg, d, c);	//Allow the input character to be processed normally
}
