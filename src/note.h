#ifndef NOTE_H
#define NOTE_H

#include "song.h"

#define EOF_MAX_FRETS 6
#define EOF_LYRIC_PERCUSSION 96


unsigned long eof_note_count_colors(EOF_SONG *sp, unsigned long track, unsigned long note);
	//Performs bit masking to determine the number of gems the specified note defines being present
unsigned long eof_note_count_colors_bitmask(unsigned long notemask);
	//Performs bit masking to determine the number of gems the specified note bitmask defines being present
void eof_legacy_track_note_create(EOF_NOTE * np, char g, char y, char r, char b, char p, char o, unsigned long pos, long length);
	//Initializes a note by by storing the specified on/off status of the green, yellow, red, blue and purple gem colors
	//as well as the position and length into the given EOF_NOTE structure, whose contents are overwritten
	//The note type and flags are not altered
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
EOF_PHRASE_SECTION *eof_find_lyric_line(unsigned long lyricnum);
	//Returns a pointer to whichever line the specified lyric exists in, or NULL if it is not in a line
unsigned long eof_find_lyric_number(EOF_LYRIC * np);
	//Finds the lyric in the lyric[] array and returns its index, or 0 on error or lyric not found
	//error checking can be achieved by testing if(!returnval && (lyric[returnval] != np))

BITMAP *eof_create_fret_number_bitmap(EOF_PRO_GUITAR_NOTE *note, unsigned char stringnum, unsigned long padding, int textcol, int fillcol);
	//Used to create a bordered rectangle bitmap with the specified string number, for use in the editor or 3D window, returns NULL on error
void eof_get_note_notation(char *buffer, unsigned long track, unsigned long note);
	//Used to store notations (ie. "PM" for palm mute) for the specified note into the buffer, which should be able to hold at least 30 characters
int eof_note_compare(EOF_SONG *sp, unsigned long track1, unsigned long note1, unsigned long track2, unsigned long note2);
	//Compares the two notes and returns 0 under the following conditions:
	//1. The track is a legacy format track and both notes have the same bitmask
	//2. The track is a vocal format track and both lyrics have the same pitch
	//3. The track is a pro guitar format track and both notes have the same bitmask (legacy bitmasks are not compared) and active frets have matching values
	//If the notes do not match, or are from differently formatted tracks, 1 is returned
	//-1 is returned on error
int eof_note_compare_simple(EOF_SONG *sp, unsigned long track, unsigned long note1, unsigned long note2);
	//Compares two notes in the same track by invoking eof_note_compare
int eof_pro_guitar_note_compare(EOF_PRO_GUITAR_TRACK *tp1, unsigned long note1, EOF_PRO_GUITAR_TRACK *tp2, unsigned long note2);
	//Compares two pro guitar notes and returns 0 if both notes have the same bitmask (legacy bitmasks are not compared) and active frets have matching values
	//If the notes do not match, or are from differently formatted tracks, 1 is returned
	//-1 is returned on error
char eof_build_note_name(EOF_SONG *sp, unsigned long track, unsigned long note, char *buffer);
	//Copies the note name into the buffer, which is assumed to be large enough to store the name
	//If the note is named manually, that name is used, otherwise chord detection is used to build the name
	//Returns 0 on error or if the note has no manually assigned or detected name
	//Returns 1 if the name was manually assigned
	//Returns 2 if the name was detected
unsigned char eof_pro_guitar_note_lowest_fret(EOF_PRO_GUITAR_TRACK *tp, unsigned long note);
	//Returns the lowest fret number used in the specified note.
	//If all strings are played open or are string muted with no fret specified, 0 is returned
unsigned char eof_pro_guitar_note_highest_fret(EOF_PRO_GUITAR_TRACK *tp, unsigned long note);
	//Returns the highest fret number used in the specified note.
	//If all strings are played open or are string muted with no fret specified, 0 is returned
unsigned char eof_pro_guitar_note_is_barre_chord(EOF_PRO_GUITAR_TRACK *tp, unsigned long note);
	//Returns nonzero if the specified note is a barre chord (the lowest used fret is played on multiple non-contiguous strings, with no strings played open between the lowest fret instances)

#endif
