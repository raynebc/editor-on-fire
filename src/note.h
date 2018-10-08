#ifndef NOTE_H
#define NOTE_H

#include "song.h"

#define EOF_MAX_FRETS 6
#define EOF_LYRIC_PITCH_MIN 36
#define EOF_LYRIC_PITCH_MAX 84
#define EOF_LYRIC_PERCUSSION 96

extern char eof_last_tab_notation[65];


unsigned long eof_note_count_colors(EOF_SONG *sp, unsigned long track, unsigned long note);
	//Uses eof_note_count_colors_bitmask() to determine the number of gems the specified note defines being present
unsigned long eof_note_count_colors_bitmask(unsigned long notemask);
	//Performs bit masking to determine the number of gems the specified note bitmask defines being present
	//Lanes higher than 7 are checked for the sake of Feedback import, files for which can use lane 8 for open strum notes
unsigned long eof_note_count_rs_lanes(EOF_SONG *sp, unsigned long track, unsigned long note, char target);
	//Returns the number of gems the specified note contains that can be exported for the target game
	//If target is 1, then Rocksmith 1 authoring rules are also followed and lanes that are string muted or ghosted are not counted
	//If target is 2, then Rocksmith 2 authoring rules are followed and string muted gems are counted, but ghosted gems still are not
	//If (target & 4) is nonzero, then ghosted gems are also counted
	//Returns 0 on error
int eof_adjust_notes(int offset);
	//Applies the given additive offset to all notes, lyrics, bookmarks, catalog entries, phrases, etc.
int eof_note_draw(unsigned long track, unsigned long notenum, int p, EOF_WINDOW *window);
	//Renders the note to the specified window, unless it would be outside the viewable area
	//If p is nonzero, the note is rendered as highlighted (selected/hovered)
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

BITMAP *eof_create_fret_number_bitmap(EOF_PRO_GUITAR_NOTE *note, char *text, unsigned char stringnum, unsigned long padding, int textcol, int fillcol, FONT *font);
	//Used to create a bordered rectangle bitmap with the specified string number, for use in the editor or 3D window, returns NULL on error
	//The specified font is used, allowing the mono-spaced symbol and regular fonts to be used interchangeably
	//If note is NULL, a bitmap containing the string in the text pointer is used instead
void eof_get_note_notation(char *buffer, unsigned long track, unsigned long note, unsigned char sanitycheck);
	//Used to store notations (ie. "PM" for palm mute) for the specified note into the buffer, which should be able to hold at least 65 characters just to guarantee an overflow isn't possible
	//If sanitycheck is nonzero and the specified note is a pro guitar note, the validity of any pitched/unpitched slide technique it has is checked
	// and if the end position is not valid, a question mark is appended after the slide notation character
int eof_note_compare(EOF_SONG *sp, unsigned long track1, unsigned long note1, unsigned long track2, unsigned long note2, char thorough);
	//Compares the two notes and returns 0 under the following conditions:
	//1. The track is a legacy format track and both notes have the same bitmask
	//2. The track is a vocal format track and both lyrics have the same pitch
	//3. The track is a pro guitar format track and both notes have the same bitmask (legacy bitmasks are not compared) and active frets have matching values
	//If the thorough parameter is nonzero, the notes' flags and extended flags must also be identical, and their lengths must be within 3ms of each other for them to match
	// *The highlight status is not required to match
	//If the notes do not match, or are from differently formatted tracks, 1 is returned
	//If the thorough parameter is greater than 0, string mute status is also compared if the notes are pro guitar notes
	//If the thorough parameter is greater than 1, ghost status is also compared if the notes are pro guitar notes
	//If the thorough parameter is greater than 2, bend strength and pitched/unpitched slide end positions are also compared if the notes are pro guitar notes
	//-1 is returned on error
int eof_note_compare_simple(EOF_SONG *sp, unsigned long track, unsigned long note1, unsigned long note2);
	//Compares two notes in the same track by invoking eof_note_compare, with the option of not comparing note lengths and flags
int eof_pro_guitar_note_compare(EOF_PRO_GUITAR_TRACK *tp1, unsigned long note1, EOF_PRO_GUITAR_TRACK *tp2, unsigned long note2, char thorough);
	//Compares two pro guitar notes and returns 0 if both notes have the same bitmask (legacy bitmasks are not compared) and active frets have matching values
	//If the notes do not match, or are from differently formatted tracks, 1 is returned
	//If the thorough parameter is greater than 0, string mute status is also compared
	//If the thorough parameter is greater than 1, ghost status is also compared
	//If the thorough parameter is greater than 2, bend strength and pitched/unpitched slide end positions are also compared
	//-1 is returned on error
int eof_pro_guitar_note_compare_fingerings(EOF_PRO_GUITAR_NOTE *np1, EOF_PRO_GUITAR_NOTE *np2);
	//Compares the fingering between the specified notes and returns 0 if both use the same strings and define the same fingering for all used strings
	//The muting status is not taken into account in the comparison if the specified finger is the same in both notes
	//-1 is returned on error
char eof_build_note_name(EOF_SONG *sp, unsigned long track, unsigned long note, char *buffer);
	//Copies the note name into the buffer, which is assumed to be large enough to store the name
	//If the note is named manually, that name is used, otherwise chord detection is used to build the name
	//Returns 0 on error or if the note has no manually assigned or detected name
	//Returns 1 if the name was manually assigned
	//Returns 2 if the name was detected
unsigned char eof_pro_guitar_note_lowest_fret_np(EOF_PRO_GUITAR_NOTE *np);
	//Returns the lowest fret number used in the specified note
	//If all strings are played open or are string muted with no fret specified, 0 is returned
unsigned char eof_pro_guitar_note_lowest_fret(EOF_PRO_GUITAR_TRACK *tp, unsigned long note);
	//Same as eof_pro_guitar_note_lowest_fret_np() but with a track pointer and note index as parameters
unsigned char eof_pro_guitar_note_highest_fret(EOF_PRO_GUITAR_TRACK *tp, unsigned long note);
	//Returns the highest fret number used in the specified note.
	//If all strings are played open or are string muted with no fret specified, 0 is returned
unsigned char eof_pro_guitar_note_is_barre_chord(EOF_PRO_GUITAR_TRACK *tp, unsigned long note);
	//Returns nonzero if the specified note is a barre chord (the lowest used fret is played on multiple non-contiguous strings, with no strings played open between the lowest fret instances)
unsigned char eof_pro_guitar_note_is_double_stop(EOF_PRO_GUITAR_TRACK *tp, unsigned long note);
	//Returns nonzero if the specified note is a double stop (only two contiguous strings played)
char eof_pro_guitar_note_is_open_chord(EOF_PRO_GUITAR_TRACK *tp, unsigned long note);
	//Returns nonzero if the specified note is an open chord (at least one open string, at least one fretted string, and no strings fretted higher than fret 3)

void eof_build_trill_phrases(EOF_PRO_GUITAR_TRACK *tp);
	//Creates trill phrases for the specified track by checking which notes have the EOF_NOTE_FLAG_IS_TRILL flag set
void eof_build_tremolo_phrases(EOF_PRO_GUITAR_TRACK *tp, unsigned char diff);
	//Creates tremolo phrases for the specified track by checking which notes have the EOF_NOTE_FLAG_IS_TREMOLO flag set
	//Diff refers to which difficulty of notes should be examined, or 0xFF to refer to all difficulties
	//The created phrases apply to the corresponding difficulty as well (ie. Rocksmith authoring), or to all difficulties (ie. Rock Band authoring)

int eof_note_convert_ghl_authoring(EOF_SONG *sp, unsigned long track, unsigned long note);
	//Translates the specified note to suit whether the track it's in has GHL mode enabled
	//If the track has GHL mode enabled:
	//  1.  A 5-lane chord will be converted to a lane 6 gem
	//  2.  A lane 6 single note (5 lane guitar's open note) will have EOF_GUITAR_NOTE_FLAG_GHL_OPEN status applied
	//If the track has GHL mode disabled:
	//  1.  A note with a lane 6 gem (lane 3 white GHL gem) will be converted to a 5 lane chord (lane 6 is exclusively used for open notes when GHL mode is not in effect)
	//  2.  A note with EOF_GUITAR_NOTE_FLAG_GHL_OPEN status has that flag removed and is changed to a lane 6 single note
	//Converting to non GHL mode has the potential to lose authoring if the note is a chord that contains a lane 3 white gem
	//  If this is the case for the specified note, its highlighting flag is set and nonzero is returned
	//Only alters notes that are in a legacy, guitar behavior track

int eof_legacy_guitar_note_is_open(EOF_SONG *sp, unsigned long track, unsigned long note);
	//If the specified note is in a legacy guitar/bass track, returns nonzero if either of these conditions are true:
	//  1.  The note is in a GHL mode track and has EOF_GUITAR_NOTE_FLAG_GHL_OPEN status
	//  2.  The note is in a non GHL mode track and contains a gem on lane 6

int eof_note_is_last_longest_gem(EOF_SONG *sp, unsigned long track, unsigned long note);
	//Returns nonzero if the note is the longest gem at its position (ie. for identifying the longest gem in a disjointed chord)
	//Returns nonzero if the note does not have disjointed status, as it is the longest note at its position
	//In the event of a tie between this gem and a later one being the same length, zero is returned
	//This allows SP pathing logic to only score SP bonus for one representative gem in a disjointed chord
	//Assumes all notes are sorted
	//Return zero on error

unsigned long eof_note_count_gems_extending_to_pos(EOF_SONG *sp, unsigned long track, unsigned long note, unsigned long pos);
	//Returns the number of notes starting at the specified note's timestamp which extend to or beyond the specified position
	//Returns 1 if the specified note does not have disjointed status
	//Assumes all notes are sorted
	//Returns 0 on error

int eof_note_is_last_in_sp_phrase(EOF_SONG *sp, unsigned long track, unsigned long note);
	//Returns nonzero if the specified note has star power and either of the following is true:
	// 1.  There is not another note that follows it
	// 2.  There is another note that follows it, and it does not have star power

unsigned long eof_translate_track_diff_note_index(EOF_SONG *sp, unsigned long track, unsigned char diff, unsigned long index);
	//Accepts an index specifying the note number within a track difficulty and returns the real index for that note in the track's note[] array
	//Return ULONG_MAX if the index is invalid or upon error

#endif
