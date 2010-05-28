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

int eof_lyric_draw_truncate(int notenum, int p);
	//If notenum < eof_song->vocal_track->lyrics, renders the text to the lyric lane
	//If notenum < eof_song->vocal_track->lyrics-1, the X coordinate of the next lyric's position in the lane is used
	//to clip the drawing of lyric #notenum, performing effective truncate logic
	//Returns nonzero if the x position that the text was written to was outside of the clip region for the lyric lane,
	//so the calling function is able to stop rendering lyrics that won't be seen
EOF_LYRIC_LINE *FindLyricLine_p(EOF_LYRIC * lp);
EOF_LYRIC_LINE *FindLyricLine(int lyricnum);
	//Returns a pointer to whichever line the specified lyric exists in, or NULL if it is not in a line

#endif
