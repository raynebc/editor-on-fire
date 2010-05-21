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
	int delay;

} EOF_CONTROLLER;

void eof_read_controller(EOF_CONTROLLER * cp);	//Polls for controller/keyboard input and updates the applicable button statuses in the passed controller structure
int eof_controller_set_button(EOF_CONTROLLER_BUTTON * bp);	//Scans for controller/keyboard input and stores the first detected button/key press ID in the passed button structure
void eof_controller_read_button_names(EOF_CONTROLLER * cp);	//Populates the passed controller structure with the Allegro-defined names for all butons/keys defined for the controller
void eof_controller_save_config(EOF_CONTROLLER * cp, PACKFILE * fp);	//Saves the controller's configuration to file
void eof_controller_load_config(EOF_CONTROLLER * cp, PACKFILE * fp);	//Loads the controller's configuration from file

#endif
