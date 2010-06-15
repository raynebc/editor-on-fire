#ifndef NOTE_H
#define NOTE_H

#include "song.h"

int eof_note_count_colors(EOF_NOTE * np);	//Performs bit masking to determine the number of gems the note defines being present
void eof_note_create(EOF_NOTE * np, char g, char y, char r, char b, char p, int pos, int length);
	//Creates a note by by storing the specified on/off status of the green, yellow, red, blue and purple gem colors
	//as well as the position and length into the given EOF_NOTE structure, whose contents are overwritten
	//The note type and flags are not altered
//void eof_detect_notes(void);	//Undefined function
int eof_adjust_notes(int offset);	//Applies the given additive offset to all notes, lyrics, bookmarks, catalog entries and solo/star power/lyric phrases
void eof_note_draw(EOF_NOTE * np, int p);	//Renders the note to the piano roll area
void eof_lyric_draw(EOF_LYRIC * np, int p);	//Renders the lyric to the piano roll area
void eof_note_draw_3d(EOF_NOTE * np, int p);	//Renders the note to the 3D preview area
int eof_note_tail_draw_3d(EOF_NOTE * np, int p);	//Renders the note's tail to the 3D preview area
void eof_note_draw_catalog(EOF_NOTE * np, int p);	//Renders the note to the fret catalog area
void eof_lyric_draw_catalog(EOF_LYRIC * np, int p);	//Renders the lyric to the fret catalog area

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
