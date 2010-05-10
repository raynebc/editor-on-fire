#ifndef NOTE_H
#define NOTE_H

#include "song.h"

int eof_note_count_colors(EOF_NOTE * np);
void eof_note_create(EOF_NOTE * np, char g, char y, char r, char b, char p, int pos, int length);
void eof_detect_notes(void);
int eof_adjust_notes(int offset);
void eof_note_draw(EOF_NOTE * np, int p);
void eof_lyric_draw(EOF_LYRIC * np, int p);
void eof_note_draw_3d(EOF_NOTE * np, int p);
int eof_note_tail_draw_3d(EOF_NOTE * np, int p);
void eof_note_draw_catalog(EOF_NOTE * np, int p);
void eof_lyric_draw_catalog(EOF_LYRIC * np, int p);

#endif
