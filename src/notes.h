#ifndef EOF_NOTES_H
#define EOF_NOTES_H

#include <allegro.h>
#include "main.h"
#include "song.h"

//This structure is passed to eof_expand_notes_window_macro() so that it can alter Notes panel settings when relevant macros are read
typedef struct
{
	//Pointers/paths
	char filename[1024];	//The full path to the text file defining the content for this panel
	char *text;				//The pointer to the memory buffer containing the above text file
	EOF_WINDOW *window;		//The window to which the panel will render

	//Print controls
	int xpos, ypos;	//The current output coordinates of text being printed to the Notes panel
	int color;		//The text color in use
	int bgcolor;	//The background color used for that text
	int allowempty;	//A %EMPTY% macro was parsed, allow an empty output line to print to the Notes panel

	//Parsing controls
	int flush;		//Set to nonzero if a %FLUSH% macro is parsed, signaling eof_expand_notes_window_text() to output the current content of the output buffer
					// and then resume parsing the current line of text
	int contentprinted;	//Set to nonzero if content was printed for a line, such as by using %FLUSH% even if the output buffer is empty when the end of line is parsed
	int symbol;		//After a flush, this character will be printed to the Notes panel using the symbol font (ie. to print guitar tab characters)
	int endline;	//After a flush, a nonzero value for this variable will end processing of the current line
	int endpanel;	//After a flush, a nonzero value for this variable will end processing of the entire panel

} EOF_TEXT_PANEL;

EOF_TEXT_PANEL *eof_create_text_panel(char *filename, int builtin);
	//Creates a text panel and buffers the specified filename into its text variable
	//If builtin is nonzero, the function will attempt to recover the specified file from eof.dat if it is missing from the CWD (which is expected to be EOF's program folder)
void eof_destroy_text_panel(EOF_TEXT_PANEL *panel);
	//Frees panel->text and frees panel
int eof_expand_notes_window_text(char *src_buffer, char *dest_buffer, unsigned long dest_buffer_size, EOF_TEXT_PANEL *panel);
	//Parses the content of the source buffer, expanding macros with eof_expand_notes_window_macro() and populating the destination buffer of the given size
	//The destination buffer is guaranteed to be NULL terminated
	//The controls structure is passed to eof_expand_notes_window_macro(), which alters it appropriately when relevant control macros are parsed
	//If a macro is not recognized, the destination buffer is populated with a message indicating the first macro in the input buffer that wasn't recognized
	//Returns zero on error, and writes any failed macro text conversion
	//Returns 1 on a normal conversion
int eof_expand_notes_window_macro(char *macro, char *dest_buffer, unsigned long dest_buffer_size, EOF_TEXT_PANEL *panel);
	//Matches the macro against supported macro strings
	//If a match is found, the corresponding text is generated and written to the destination buffer
	//The controls structure is altered appropriately when relevant control macros are parsed
	//Returns 1 if a normal macro is expanded to the destination buffer, or if a control macro was processed
	//Returns 2 if the macro is a conditional test that evaluates as false, which signals that the all content from the macro until the end of the line or next %ENDIF% instance is to be skipped
	//Returns 3 if the macro is a conditional test that evaluates as true, in which case the destination buffer is emptied
	//Returns zero on error or if no matching string is found for the macro
int eof_read_macro_color(char *string, int *color);
	//Accepts a string and performs a comparison against known color names
	//If a match is found, *color is set to the appropriate Allegro color
	//If no match is found, but the string is composed of 6 hexadecimal characters,
	//  it is treated as an RGB color code and that color is created and returned through *color
	//Returns 0 if no color was determined or upon error
int eof_read_macro_number(char *string, unsigned long *number);
	//Accepts a string and converts the decimal number representation and stores it into *number
	//Returns 0 if no number was parsed or upon error
int eof_read_macro_gem_designations(char *string, unsigned char *bitmask, unsigned char *tomsmask, unsigned char *cymbalsmask);
	//Accepts a string and converts the gem designations into bitmasks
	//For non GHL tracks:  'G' = lane 1, 'R' = lane 2, 'Y' = lane 3, 'B' = lane 4, 'O' = lane 5, 'P' = lane 6, 'S' = open strum
	//For GHL tracks:  "B1" = lane 1, "B2" = lane 2, "B3" = lane 3, "W1" = lane 4, "W2" = lane 5, "W3" = lane 6, 'S' = open strum
	//For drum tracks:  "T3" = lane 3 tom, "T4" = lane 4 tom, "T5" = lane 5 tom, "C3" = lane 3 cymbal, "C4" = lane 4 cymbal, "C5" = lane 5 cymbal
	//  The toms and cymbals bitmasks are set accordingly if a drum track is active, otherwise *toms and *cymbals are reset to 0 to indicate they
	//  needn't be met as comparison criteria.  Tom and cymbal designations are additive, ie. a designation of 3T5 will be specify any drum chord
	//  with a cymbal or tom on lane 3 and a tom on lane 5.
	//Lane numbers can be designated by number instead of color:  '1' = lane 1, '2' = lane 2, '3' = lane 3, '4' = lane 4, '5' = lane 5, '6' = lane 6
	//  which would be more convenient for use with pro guitar tracks.  '6' will not refer to open strum notes.
	//For both types of tracks, an open strum is returned as value 255
	//If the string contains an open strum designation in addition to other gems, open strum takes precedence and 255 is returned through *bitmask
void eof_render_text_panel(EOF_TEXT_PANEL *panel, int opaque);
	//Processes the specified panel's text file and renders it to the panel's configured EOF_WINDOW instance
	//If opaque is nonzero, the specified panel's window is cleared to dark gray and a window border is drawn
	//Set opaque to zero in the event that full screen 3D preview is in use, so that it doesn't obscure the 3D preview

unsigned long eof_count_num_notes_with_gem_count(unsigned long gemcount);
	//Examines all notes in the active track difficulty
	//Returns the number of those notes that have the specified number of gems (ie. 1 for single notes or >1 for chords)
	//For the sake of the Notes panel, legacy guitar track open notes do not count as having a gem
	//  calling this function with a gem count of 0 will count the open notes in the active legacy guitar track
unsigned long eof_count_num_notes_with_gem_designation(unsigned char gems, unsigned char toms, unsigned char cymbals);
	//Examines all notes in the active track difficulty
	//Returns the number of those that have the specified note gem bitmask (as created by eof_read_macro_gem_designations() )
	//If the track is a drum track, the note must have the specified toms and the specified cymbals to be counted as a match
	//A gem bitmask of 255 indicates an open note
unsigned long eof_count_track_num_notes_with_flag(unsigned long flags);
	//Examines all notes in the active track
	//Returns the number of those that have the specified flag(s)

unsigned long eof_notes_panel_count_section_stats(unsigned long sectiontype, unsigned long *minptr, unsigned long *maxptr);
	//Examines the number of notes in the active track's sections of the specified type (ie. EOF_SOLO_SECTION or EOF_SP_SECTION)
	//The note count in the section with the fewest notes is returned via *min if minptr is not NULL
	//The note count in the section with the most notes is returned via *max if maxptr is not NULL
	//Returns the total number of section notes in the active track, or 0 on error

#endif
