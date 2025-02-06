#ifndef EOF_DR_H
#define EOF_DR_H

#include "song.h"

int eof_export_drums_rock_track_diff(EOF_SONG * sp, unsigned long track, unsigned char diff, char *destpath);
	//Exports Drums Rock files for the specified drum track difficulty in a subfolder of the project folder
	//destpath will be the folder level at which each difficulty's file folder will be written

unsigned char eof_convert_drums_rock_note_mask(EOF_SONG *sp, unsigned long track, unsigned long note);
	//For the specified note , filters it down to a maximum of two gems (or single gems for any notes in a drum roll) suitable to export to Drums Rock format
	//Expects eof_determine_phrase_status() to have been called for the applicable track to set drum roll flags for the note
	//If the track has the EOF_TRACK_FLAG_DRUMS_ROCK_REMAP flag enabled, the note bitmask will be remapped based on the drums_rock_remap_lane_* variables

unsigned long eof_get_note_name_as_number(EOF_SONG * sp, unsigned long track, unsigned long notenum);
	//Examines the specified note's name, if defined, and returns it as an unsigned long
	//Returns 0 if the number could not be parsed or if the name is not defined

int eof_check_drums_rock_track(EOF_SONG * sp, unsigned long track);
	//Performs various Drums Rock related quality checks on the specified track and prompts the user whether to cancel save and seek to/highlight issues
	// If the user opts to cancel project save, nonzero is returned, otherwise zero is returned

#endif // header guard

