#ifndef EOF_RS_IMPORT_H
#define EOF_RS_IMPORT_H

#include "song.h"

typedef struct
{
	unsigned long start;
	unsigned long stop;
	unsigned long size;			//The size of the gap in ms
	unsigned long capacity;		//The number of technotes that will be inserted into this gap during the final application of the solution
	unsigned long population;	//The number of technotes that have been inserted into this gap during the final application of the solution
} EOF_TECHNOTE_GAP;

int eof_parse_chord_template(char *name, size_t size, char *finger, char *frets, unsigned char *note, unsigned char *isarp, unsigned long linectr, char *input);
	//Reads a Rocksmith XML format chord template from the input string into the destination variables:
	//	The name attribute is read into name[] which is an array at least size number of bytes in size
	//	The chord fingering is read into finger[] which is an array at least 6 bytes in size
	//	The fretting is read into frets[] which is an array at least 6 bytes in size
	//	The corresponding note bitmask is stored into *note
	//  If the parsed chord's display name contains "-arp", *isarp is set to nonzero if isarp is not NULL
	//Returns zero on success
	//Upon error, the specific cause for failure is logged regarding line number linectr and nonzero is returned

EOF_PRO_GUITAR_NOTE *eof_rs_import_note_tag_data(char *buffer, int function, EOF_PRO_GUITAR_TRACK *tp, unsigned long linectr, unsigned char curdiff);
	//Parses the note, chordnote or bendvalue XML tag contained in the specified array,
	// creating a pro guitar note structure as defined by the tag
	//If function is nonzero, the note is appended to the specified track structure
	// bendvalue tags result in the note being added to the technote set, otherwise the note is added to the normal note set
	//If a bendvalue tag is parsed, the difficulty and string number of the last normal note to be parsed by this function is applied to the created tech note
	//The created note is returned, or NULL is returned on error
	//linectr is cited as the XML line number in error logging
	//curdiff is the difficulty level to assign to any new notes that are appended to the track

unsigned long eof_evaluate_rs_import_gap_solution(EOF_TECHNOTE_GAP *gaps, unsigned long numgaps, unsigned *solution, unsigned long solutionsize);
	//Calculates the amount of space before and after each technote and other technotes or obstacles
	//The smallest of those amounts is returned as the value of the solution, or ULONG_MAX is returned on error
	//A larger solution is desired as it indicates all technotes are as sparsely spaced as possible

char eof_rs_import_process_chordnotes(EOF_PRO_GUITAR_TRACK *tp, EOF_PRO_GUITAR_NOTE *np, EOF_PRO_GUITAR_NOTE **chordnote, unsigned long chordnotectr, unsigned long numtechnotes);
	//Accepts a pointer to the chord (np) being parsed, an array (chordnote) containing the chordnote data and the number (chordnotectr) of chordnotes in that array
	// and adds optimally-spaced technotes over the duration of the specified chord
	//numtechnotes is expected to be the number of technotes that have already been added to the specified note (ie. bend tech notes)
	//This function frees the pro guitar note structures in the chordnote array and resets the array's pointers to NULL
	//Returns nonzero on error

EOF_PRO_GUITAR_TRACK *eof_load_rs(char * fn);
	//Parses the specified Rocksmith XML file and returns its content in a pro guitar track structure

#endif
