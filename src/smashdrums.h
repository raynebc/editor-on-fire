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

#endif
