#ifndef EOF_CONTROLLER_H
#define EOF_CONTROLLER_H

#define EOF_CONTROLLER_BUTTON_TYPE_KEY       0
#define EOF_CONTROLLER_BUTTON_TYPE_JOYBUTTON 1
#define EOF_CONTROLLER_BUTTON_TYPE_JOYAXIS   2

#define EOF_CONTROLLER_MAX_BUTTONS 16

typedef struct
{
	
	char type;
	int joy, index, d;
	int key;
	char name[32];
	
	unsigned long held;
	char pressed, released;
	
} EOF_CONTROLLER_BUTTON;

typedef struct
{
	
	EOF_CONTROLLER_BUTTON button[EOF_CONTROLLER_MAX_BUTTONS];
	
} EOF_CONTROLLER;

void eof_read_controller(EOF_CONTROLLER * cp);
int eof_controller_set_button(EOF_CONTROLLER_BUTTON * bp);
void eof_controller_read_button_names(EOF_CONTROLLER * cp);
void eof_controller_save_config(EOF_CONTROLLER * cp, PACKFILE * fp);
void eof_controller_load_config(EOF_CONTROLLER * cp, PACKFILE * fp);

#endif
