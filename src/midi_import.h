#ifndef EOF_MIDI_IMPORT_H
#define EOF_MIDI_IMPORT_H

#include "song.h"
#include "midi.h"
#include "foflc/Midi_parse.h"

#define EOF_MAX_IMPORT_MIDI_TRACKS 32
#define EOF_MAX_MIDI_TEXT_SIZE 255
#define EOF_IMPORT_MAX_EVENTS EOF_MAX_MIDI_EVENTS

extern unsigned char eof_midi_import_drum_accent_velocity;
extern unsigned char eof_midi_import_drum_ghost_velocity;

EOF_SONG * eof_import_midi(const char * fn);
double eof_ConvertToRealTime(unsigned long absolutedelta, struct Tempo_change *anchorlist, EOF_MIDI_TS_LIST *tslist, unsigned long timedivision, unsigned long offset, unsigned int *gridsnap);
	//Converts and returns the converted realtime of the specified delta time based on the anchors, time signatures and the MIDI's time division
	//The specified offset should represent a chart delay, and is added to the returned realtime
	//If gridnsap is not NULL, the variable it references is set to nonzero if the specified delta time is determined to be a grid snap position,
	// otherwise it is set to zero, so this condition can be enforced after import to defeat rounding errors
unsigned long eof_parse_var_len(unsigned char * data, unsigned long pos, unsigned long * bytes_used);
	//Parses a variable length value from data[pos], returning it and adding the size (in bytes) of the variable length value to bytes_used
	//bytes_used is NOT initialized to zero within this function, the calling function must set it appropriately
unsigned long eof_repair_midi_import_grid_snap(void);
	//Parses all notes/lyrics in the active project and processes those that have the EOF_NOTE_TFLAG_RESNAP temporary flag set during MIDI import
	//For such notes that are not currently grid snapped, they are each moved to the nearest grid snap of any size and the temporary flag is cleared
	//Currently, this is called after eof_import_midi() and eof_init_after_load() have completed
	//Returns zero on error
int eof_song_check_unsnapped_chords(EOF_SONG *sp, unsigned long track, int function, int timing, unsigned threshold);
	//Checks the positions for the notes in one or more instrument track in the specified chart to determine whether they should merge
	//If track is zero, all instrument tracks are checked, otherwise only the specified track is checked
	//If timing is zero, the MIDI position (ie. set during MIDI import, discarded during save) is checked
	//If timing is nonzero, the millisecond position is checked
	//If none of the notes in the project have a nonzero MIDI position, but the MIDI position was chosen for the operation, a warning is displayed and the function returns
	//If any two notes in a track difficulty are within the threshold of timing units of each other but not at the same position,
	// they are regarded as authoring errors where they were probably meant to be a chord
	//If function is zero, if there are such notes they are highlighted
	//If function is nononzero, if there are any such notes, the user is prompted about whether the later gems in the unsnapped chord should be re-aligned with the earlier gem
	//If any notes were deemed to be unsnapped chords, nonzero is returned
	//This is to be called at the end of eof_import_midi() before any timing changes such as those made by eof_repair_midi_import_grid_snap() occur

#endif
