#ifndef EOF_DR_H
#define EOF_DR_H

#include "song.h"

int eof_export_drums_rock_track_diff(EOF_SONG * sp, unsigned long track, unsigned char diff, char *destpath);
	//Exports Drums Rock files for the specified drum track difficulty in a folder with multiple files
	//destpath will be the folder level at which each difficulty's file folder will be written

unsigned char eof_convert_drums_rock_note_mask(EOF_SONG *sp, unsigned long track, unsigned long note);
	//For the specified note , filters it down to a maximum of two gems (or single gems for any notes in a drum roll) suitable to export to Drums Rock format
	//Expects eof_determine_phrase_status() to have been called for the applicable track to set drum roll flags for the note
	//If the track has the EOF_TRACK_FLAG_DRUMS_ROCK_REMAP flag enabled, the note bitmask will be remapped based on the drums_rock_remap_lane_* variables

unsigned long eof_get_note_name_as_number(EOF_SONG * sp, unsigned long track, unsigned long notenum, unsigned long *number);
	//Examines the specified note's name, if defined, and stores it into the pointer
	//Returns 0 if the number could not be parsed, is not defined or upon error

int eof_check_drums_rock_track(EOF_SONG * sp, unsigned long track);
	//Performs various Drums Rock related quality checks on the specified track and prompts the user whether to cancel save and seek to/highlight issues
	// If the user opts to cancel project save, nonzero is returned, otherwise zero is returned

int eof_import_drums_rock_track_diff(char * fn);
	//Accepts a path to a CSV file defining Drums Rock notes, parsing it to import notes into the active drum track difficulty
	//If the specified file can't be parsed, notes.csv is attempted to be loaded if it is present at the same folder level
	//If info.csv exists at the same folder level, it is parsed to verify difficulty level and optionally import song metadata
	//Does not import notes if a drum track is not active
	//Returns 1 on error, or 2 on user cancellation

void eof_build_sanitized_drums_rock_string(char *input, char *output);
	//Writes a copy of the input string to the output string, removing all commas, which are not supported by Drums Rock in the song or artist name fields in info.csv
	//Any double quotation marks (") are written as "" as is appropriate for CSV files
	//input and output must be different, non-overlapping pointers

#endif // header guard

