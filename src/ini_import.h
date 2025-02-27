#ifndef EOF_INI_IMPORT_H
#define EOF_INI_IMPORT_H

extern char eof_ini_pro_drum_tag_present;		//Is set to nonzero if eof_import_ini() finds the "pro_drums = True" tag (to influence MIDI import)
extern char eof_ini_star_power_tag_present;		//Is set to nonzero if eof_import_ini() finds the "multiplier_note = 116" tag (to influence MIDI import)
extern char eof_ini_sysex_open_bass_present;	//Is set to nonzero if eof_import_ini() finds the "sysex_open_bass = True" tag (to influence MIDI import)

int eof_import_ini(EOF_SONG * sp, char * fn, int function);
	//If function is zero, any tag found in the INI file overwrites the corresponding property or tag in the specified project without prompt
	//Otherwise if a tag in the file is found to be different from what's defined in the project, the user is notified that the INI file was
	//externally altered and prompted whether or not to merge the file's changes into the project.  If the user declines, this function
	//returns without processing the remainder of the file.
	//At this time, only file imports call this with a function value of 0

int eof_compare_set_ini_string_field(char *dest, char *src, int *function, char *tag, size_t buffersize);
	//If *function is nonzero, the destination string is compared against the source string (case insensitive)
	//	and if not identical, prompts the user whether or not to replace the existing setting.  If user
	//	allows, the source string is copied to the destination and zero is returned.
	//	If user denies, the string is not altered and nonzero is returned, signaling to eof_ini_import() to cancel
	//	the processing of song.ini.  If user allows, an undo state is made and *function is set to zero
	//If *function is zero, the source string is copied to the destination without prompting and zero is returned
	//If the strings are identical, zero is returned regardless of the value of function
	//tag is the display name of the tag being checked, used for logging
	//buffersize is the size of the destination buffer, to prevent buffer overflows

int eof_compare_set_ini_boolean(char *status, char original, char *string, int *function, char *tag);
	//The string is compared against "True" or "1" to determine a boolean status which is returned
	//through the status pointer.  The "original" parameter should be the boolean value to compare the
	//string against.  If the determined string boolean status does not match original, and *function is
	//nonzero, the user is prompted whether or not to replace the existing setting.  If user denies, nonzero is
	//returned, signaling to eof_ini_import() to cancel the processing of song.ini.  If user allows, an undo
	//state is made and *function is set to zero.
	//This function does not directly alter the project's contents, the calling function must determine what
	//to do with the information returned through status
	//tag is the display name of the tag being checked, used for logging

int eof_compare_set_ini_string_setting(EOF_SONG *sp, char *tag, char *value, int *function, char *logtag);
	//If *function is nonzero, the specified tag value is compared against all custom INI settings stored in
	//	the specified project.  If the specified value does not match (case insensitive) that of the
	//	project's existing INI setting, or if the specified INI tag was not already in the project, the user
	//	is prompted whether or not to replace the existing INI setting.  If user denies, the project is not
	//	altered, and nonzero is returned, signaling to eof_ini_import() to cancel the processing of song.ini.
	//	If user allows, an undo state is made and *function is set to zero.
	//If *function is zero, the specified tag value either replaces the existing tag value in the project, or
	//	it is added to the project as a new INI setting.
	//logtag is the display name of the tag being checked, used for logging

int eof_compare_set_ini_pro_guitar_tuning(EOF_PRO_GUITAR_TRACK *tp, char *string, int *function);
	//If *function is nonzero, and the tuning defined in the string does not match the existing tuning for
	//	the specified pro guitar track, the user is prompted whether or not to replace the existing
	//	tuning.  If user denies, the track is not altered, and nonzero is returned, signaling to
	//	eof_ini_import() to cancel the processing of song.ini.  If user allows, an undo state is made and
	//	*function is set to zero.
	//Otherwise the tuning is updated in the specified pro guitar track
	//If *function is zero, the tunings and string counts in the INI tags override those in the passed pro guitar structures

int eof_compare_set_ini_integer(long *value, long original, char *string, int *function, char *tag);
	//The string is converted into numerical format with atoi() and returned through the value pointer.
	//The "original" parameter should be the value to compare the source string against.  If the converted
	//numerical value does not match original, and *function is nonzero, the user is prompted whether or not
	//to replace the existing value.  If user denies, nonzero is returned, signaling to eof_ini_import() to
	//cancel the processing of song.ini.  If user allows, an undo state is made and *function is set to zero.
	//This function does not directly alter the project's contents, the calling function must determine what
	//to do with the information returned through value
	//tag is the display name of the tag being checked, used for logging

char *eof_find_ini_setting_tag(EOF_SONG *sp, unsigned long *index, char *tag);
	//Searches the specified project for the specified INI setting containing "[tag] ="
	//If found, the setting number is returned through index and the pointer to the character after the equal sign in the INI setting's string is returned
	//If no such setting was found in the project, NULL is returned

int eof_is_unicode(const char *filename);
	//If *function is nonzero, the filename is UNICODE
#endif
