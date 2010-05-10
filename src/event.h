#ifndef EOF_EVENT_H
#define EOF_EVENT_H

#include "song.h"

extern EOF_TEXT_EVENT eof_text_event[EOF_MAX_TEXT_EVENTS];
extern char eof_event_list_text[EOF_MAX_TEXT_EVENTS][256];
extern int eof_text_events;

void eof_add_text_event(EOF_SONG * sp, int beat, char * text);
void eof_move_text_events(EOF_SONG * sp, int beat, int offset);

#endif
