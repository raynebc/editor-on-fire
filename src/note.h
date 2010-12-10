#ifndef NOTE_H
#define NOTE_H

#include "song.h"

#define EOF_MAX_FRETS 6
#define EOF_LYRIC_PERCUSSION 96


int eof_note_count_colors(EOF_NOTE * np);	//Performs bit masking to determine the number of gems the note defines being present
void eof_note_create(EOF_NOTE * np, char g, char y, char r, char b, char p, char o, int pos, int length);
	//Creates a note by by storing the specified on/off status of the green, yellow, red, blue and purple gem colors
	//as well as the position and length into the given EOF_NOTE structure, whose contents are overwritten
	//The note type and flags are not altered
void eof_note_create2(EOF_NOTE * np, unsigned long bitmask, unsigned long pos, long length);
	//A simplified version of eof_note_create that accepts the fret status as a bitflag instead of individual characters
int eof_adjust_notes(int offset);	//Applies the given additive offset to all notes, lyrics, bookmarks, catalog entries and solo/star power/lyric phrases
int eof_note_draw(unsigned long track, unsigned long notenum, int p, EOF_WINDOW *window);
	//Renders the note to the specified window, unless it would be outside the viewable area
	//If track is nonzero, the pre-existing note is rendered, otherwise the eof_pen_note is rendered
	//The position specified should be eof_music_catalog_pos when rendering the fret catalog, or eof_music_pos when rendering the editor window
	//If the note is 100% clipped, nonzero is returned
	//-1 Clipped completely to the left of the viewing window
	//1 Clipped completely to the right of the viewing window
int eof_note_draw_3d(unsigned long track, unsigned long notenum, int p);
	//Renders the note to the 3D preview area
	//If the note is 100% clipped, nonzero is returned
	//-1 Clipped completely before the viewing window
	//1 Clipped completely after the viewing window
int eof_note_tail_draw_3d(unsigned long track, unsigned long notenum, int p);
	//Renders the note's tail to the 3D preview area
	//If the note is 100% clipped, nonzero is returned
	//-1 Clipped completely before the viewing window
	//1 Clipped completely after of the viewing window
int eof_lyric_draw(EOF_LYRIC * np, int p, EOF_WINDOW *window);
	//Renders the lyric to the specified window
	//If the lyric is 100% clipped, nonzero is returned
	//-1 Clipped completely to the left of the viewing window
	//1 Clipped completely to the right of the viewing window
EOF_LYRIC_LINE *eof_find_lyric_line(unsigned long lyricnum);
	//Returns a pointer to whichever line the specified lyric exists in, or NULL if it is not in a line
unsigned long eof_find_lyric_number(EOF_LYRIC * np);
	//Finds the lyric in the lyric[] array and returns its index, or 0 on error or lyric not found
	//error checking can be achieved by testing if(!returnval && (lyric[returnval] != np))


#endif
