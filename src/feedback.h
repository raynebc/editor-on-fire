#ifndef EOF_FEEDBACK_H
#define EOF_FEEDBACK_H

#include <allegro.h>
#ifdef ALLEGRO_WINDOWS
	#include <winalleg.h>
#endif

int eof_feedback_any_note(void);
void eof_editor_logic_feedback(void);

#endif
