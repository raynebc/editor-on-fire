#ifndef EOF_SMASHDRUMS_H
#define EOF_SMASHDRUMS_H

unsigned char eof_text_event_is_smashdrums_phase(char *str);
	//Returns the matching Smash Drums phase number (section type, ie. "verse") of the given string
	//The value will be between 1 and 7 inclusive for a match or 0 for a non-match
int eof_remap_smashdrums_note_masks(EOF_SONG *sp, unsigned long track, unsigned long notenum, unsigned char *notemask, unsigned char *ghostmask, unsigned char *accentmask);
	//Remaps the EOF note bitmask into the bitmask used for Smash Drums export (bass, snare, non hi hat cymbal, tom, hi hat, clapfire)
	//The ghost and accent bitmasks are remapped to match
	//Returns zero on error
int eof_export_smashdrums(EOF_SONG *sp, unsigned long track, char *destpath);
	//Exports the specified drum track in Smash Drums JSON format to the specified destination path
	//Also exports the chart audio in OGG format, cover art (if present in PNG format or if FFMPEG is linked to convert it), preview audio in WAV format if present
	//Returns zero on error

unsigned char eof_find_smashdrums_phase_power_definition_at_pos(EOF_SONG *sp, unsigned long track, unsigned long pos, unsigned long *eventindex);
	//Searches all text events at the specified position applicable to the specified track in the specified project
	//If one is found to define a phase power level (formatted as "Power=#", where # is a value from 0 through 100 inclusive)
	//  it is returned.  If eventindex is not NULL, the index of the matching text event is returned through it
	//If no such text event is found, or upon error 0xFF is returned

#endif
