#ifndef EOF_FEEDBACK_H
#define EOF_FEEDBACK_H

int eof_feedback_any_note(void);
void eof_editor_logic_feedback(void);

//A linked list storing anchors, each of which is a Set Tempo, Anchor or Time Signature event
struct dBAnchor
{
	unsigned long chartpos;	//The defined chart position of the anchor
	unsigned long BPM;	//The tempo of this beat times 1000 (0 for no change from previous anchor)
	unsigned char TS;	//The numerator of this beat's time signature (#/4) (0 for no change from previous anchor)
	unsigned long usec;	//The real time position of this anchor in microseconds (millionths of a second) (0 if not an anchor)
	struct dBAnchor *next;	//Pointer to the next anchor in the list
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
	unsigned char gemcolor;	//This is 0-4 for a note definition, '0' for a Player 1 section, '1' for a Player 2 section or '2' for a Star Power section
	unsigned long duration;
	struct dbNote *next;
};

//A linked list of instrument tracks, each of which stores a linked list of notes
struct dbTrack
{
	char *trackname;
	char tracktype;	//1 for guitar, 2 for lead guitar, 3 for for bass, 4 for drums, 5 for vocal rhythm, 0 for other
	char difftype;	//1 for easy, 2 for medium, 3 for hard, 4 for expert
	char isguitar;	//Nonzero if it is a guitar track, regardless of which one it is
	char isdrums;	//Nonzero if it is a drums track, regardless of which one it is
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
	unsigned long resolution;	//Is expected to be 192.  If it's missing in the .char file, 192 could probably be assumed
	char *audiofile;			//The MP3/OGG file specified in the chart file as the audio track
	unsigned long linesprocessed;	//The number of lines in the file that were processed
	unsigned long tracksloaded;	//The number of instrument tracks that were loaded

	struct dBAnchor *anchors;	//Linked list of anchors
	struct dbText *events;		//Linked list of text events
	struct dbTrack *tracks;		//Linked list of note tracks
};

int Read_dB_string(char *source,char **str1, char **str2);
	//Scans the source string for a valid dB tag: text = text	or	text = "text"
	//The text to the left of the equal sign is returned through str1 as a new string, with whitespace truncated
	//The text to the right of the equal sign is returned through str2 as a new string, with whitespace truncated
	//If the first non whitespace character encountered after the equal sign is a quotation mark, all characters after
	//that quotation mark up to the next are returned through str2
	//Nonzero is returned upon success, or zero is returned if source did not contain two sets of non whitespace characters
	//separated by an equal sign character, or if the closing quotation mark is missing.

struct dbTrack *Validate_dB_instrument(char *buffer);
	//Validates that buffer contains a valid dB instrument track name enclosed in brackets []
	//buffer is expected to point to the opening bracket
	//If it is valid, a dbTrack structure is allocated and initialized:
	//(track name is allocated, tracktype and difftype are set and the linked lists are set to NULL)
	//The track strcture is returned, otherwise NULL is returned if the string did not contain a valid
	//track name.  buffer[] is modified to remove any whitespace after the closing bracket

void DestroyFeedbackChart(struct FeedbackChart *ptr, char freestruct);
	//Releases memory used by the CONTENTS of the passed FeedbackChart, including that of the contained linked lists
	//If freestruct is nonzero, the passed FeedbackChart structure's memory is also freed

struct FeedbackChart *ImportFeedback(char *filename, int *error);
	//Imports the specified file and returns a populated FeedbackChart structure if the import succeeds
	//NULL is returned if import fails
	//In the event of memory allocation, file I/O or file syntax error, assert_wrapper or exit_wrapper may be
	//invoked, so use jumpcode=setjmp(jumpbuffer); and the setjmp logic to catch that in the function that
	//calls this function
	//If import fails and error is NOT NULL, a return error is passed through it

#endif
