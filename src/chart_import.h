#ifndef EOF_CHART_IMPORT_H
#define EOF_CHART_IMPORT_H

#include <allegro.h>
#include "song.h"

//A linked list storing anchors, each of which is a Set Tempo, Anchor or Time Signature event
struct dbAnchor
{
	unsigned long chartpos;	//The defined chart position of the anchor
	unsigned long BPM;	//The tempo of this beat times 1000 (0 for no change from previous anchor)
	unsigned char TSN;	//The numerator of this beat's time signature (0 for no change from previous anchor)
	unsigned char TSD;	//The denominator of this beat's time signature (4 is assumed if the .chart file is pre-Moonscraper and doesn't define this value, 0 for no change from previous anchor)
	unsigned long usec;	//The real time position of this anchor in microseconds (millionths of a second) (0 if not an anchor)
	struct dbAnchor *next;	//Pointer to the next anchor in the list
};

//A linked list strong text events
struct dbText
{
	unsigned long chartpos;
	char *text;
	struct dbText *next;
};

//A linked list storing notes for an instrument track
struct dbNote
{
	unsigned long chartpos;
	unsigned char gemcolor;
		//This is 0-4 for a note definition, 5 for inverted HOPO notation, 6 is for slider notation,
		// 7 is for open strum, 'S' for a start of solo section, 'E' for an end of solo section
		// '0' for a Player 1 section, '1' for a Player 2 section or '2' for a Star Power section
	char is_gem;	//Set to nonzero if the note is a gem instead of a status marker (is valued from 0-4, 7 or 8)
	unsigned long duration;
	struct dbNote *next;
};

//A linked list of instrument tracks, each of which stores a linked list of notes
struct dbTrack
{
	char *trackname;
	char tracktype;	//1 for guitar, 2 for lead guitar, 3 for for bass, 4 for drums, 5 for vocal rhythm, 6 for rhythm, 7 for keyboard, 0 for other
	char difftype;	//1 for easy, 2 for medium, 3 for hard, 4 for expert
	char isguitar;	//Nonzero if it is a guitar track, set to 2 if it is a GHL guitar track
	char isbass;	//Nonzero if it is a bass track, set to 2 if it is a GHL bass track
	char isdrums;	//Nonzero if it is a drums track, 1 reflects the normal drum track and 2 reflects the double drums track
	struct dbNote *notes;
	struct dbTrack *next;
};

//The structure representing the contents of a Feedback chart.  Relevant tags are stored, as are linked lists for all
//anchors, text events and instrument tracks
struct FeedbackChart
{
//Tags from the [Song] section
	char *name;
	char *artist;
	char *charter;
	double offset;		//The chart's offset in SECONDS
	unsigned long resolution;	//Is expected to be 192.  If it's missing in the .chart file, 192 could probably be assumed
	char *audiofile;			//The MP3/OGG file specified in the chart file as the audio track
	unsigned long linesprocessed;	//The number of lines in the file that were processed
	unsigned long tracksloaded;	//The number of instrument tracks that were loaded

	char guitartypes;	//Tracks the presence of 6 lane guitar (0 = no guitar parts, 1 = 5 lane guitar parts only, 2 = 6 lane guitar parts only, 3 = 5 and 6 lane guitar parts)
	char basstypes;		//Tracks the presence of 6 lane bass (0 = no bass parts, 1 = 5 lane bass parts only, 2 = 6 lane bass parts only, 3 = 5 and 6 lane bass parts)

	struct dbAnchor *anchors;	//Linked list of anchors
	struct dbText *events;		//Linked list of text events
	struct dbTrack *tracks;		//Linked list of note tracks

	unsigned long chart_length;	//The highest chart position used in the imported chart (including note lengths)
};

int Read_db_string(char *source, char **str1, char **str2);
	//Scans the source string for a valid dB tag: text = text	or	text = "text"
	//The text to the left of the equal sign is returned through str1 as a new string, with whitespace truncated
	//The text to the right of the equal sign is returned through str2 as a new string, with whitespace truncated
	//If the first non whitespace character encountered after the equal sign is a quotation mark, all characters after
	//that quotation mark up to the next are returned through str2
	//Nonzero is returned upon success, or zero is returned if source did not contain two sets of non whitespace characters
	//separated by an equal sign character, or if the closing quotation mark is missing.
int eof_validate_db_track_diff_string(char *diffstring, struct dbTrack *track);
	//Compares the input string against all supported track difficulty names, setting values in *track (ie. tracktype, isguitar, isdrums) accordingly
	//track->tracktype will be set to 0 if the string is a known instrument difficulty but isn't to be imported
	//Returns 0 on error or if the string doesn't reflect a supported name
struct dbTrack *Validate_db_instrument(char *buffer);
	//Validates that buffer contains a valid dB instrument track name enclosed in brackets []
	//buffer is expected to point to the opening bracket
	//If it is valid, a dbTrack structure is allocated and initialized:
	//(track name is allocated, tracktype and difftype are set and the linked lists are set to NULL)
	//The track structure is returned, otherwise NULL is returned if the string did not contain a valid
	//track name.  buffer[] is modified to remove any whitespace after the closing bracket
void DestroyFeedbackChart(struct FeedbackChart *ptr, char freestruct);
	//Releases memory used by the CONTENTS of the passed FeedbackChart, including that of the contained linked lists
	//If freestruct is nonzero, the passed FeedbackChart structure's memory is also freed
struct FeedbackChart *ImportFeedback(const char *filename, int *error);
	//Imports the specified file and returns a populated FeedbackChart structure if the import succeeds
	//NULL is returned if import fails
	//In the event of memory allocation, file I/O or file syntax error, assert_wrapper or exit_wrapper may be
	//invoked, so use jumpcode=setjmp(jumpbuffer); and the setjmp logic to catch that in the function that
	//calls this function
	//If import fails and error is NOT NULL, a return error is passed through it
unsigned long FindLongestLineLength_ALLEGRO(const char *filename, char exit_on_empty);
	//An adaptation of FoFLC's FindLongestLineLength() function that uses Allegro's file I/O routines in order to
	//avoid problems with the standard C functions being unable to handle filenames with special characters
	//Returns 0 on error
void eof_chart_import_process_note_markers(EOF_SONG *sp, unsigned long track, unsigned char difficulty);
	//Processes and deletes toggle HOPO markers and slider markers from the specified track difficulty to remove them and help an imported track from avoid exceeding EOF_MAX_NOTES
EOF_SONG * eof_import_chart(const char * fn);
	//Invokes ImportFeedback() and transfers the chart data to EOF
void sort_chart(struct FeedbackChart *chart);
	//Performs a bubble sort of the specified chart's linked lists, to ensure markers are correctly processed
	//Bubble sort isn't very efficient, but all notes are supposed to be defined in order anyway so it shouldn't matter for properly made charts

#endif
