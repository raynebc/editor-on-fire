#ifndef EOF_UTILITY_H
#define EOF_UTILITY_H

#include "song.h"

typedef struct
{
	char *ID;		//The metadata field, ie. IART for artist name
	char *value;	//The corresponding data for this field
	unsigned long value_array_size;	//The length of the value buffer, to avoid data overflow
} EOF_AUDIO_METADATA;

int eof_folder_exists(const char * dir);	//Returns nonzero if the specified file path exists and is a directory
int eof_chdir(const char * dir);		//Changes the current working directory of the program.  Returns nonzero on error
int eof_mkdir(const char * dir);
int eof_system(const char * command);	//Runs the specified system command, after reformatting the command string accordingly

void *eof_buffer_file(const char * fn, char appendnull, char discardbom);
	//Loads the specified file into a memory buffer and returns the buffer, or NULL upon error
	//If appendnull is nonzero, an extra 0 byte is written at the end of the buffer, potentially necessary if buffering a file that will be read as text, ensuring the buffer is NULL terminated
	//If discardbom is nonzero, any EF BB BF, FE FF or FF FE byte order mark sequence at the beginning of the file is discarded and not included in the buffer
int eof_copy_file(const char * src, const char * dest);	//Copies the source file to the destination file.  Returns 0 upon error
int eof_conditionally_copy_file(const char * src, const char * dest);
	//Performs eof_copy_file() if the two specified files are of a different size
int eof_file_compare(char *file1, char *file2);	//Returns zero if the two files are the same

int eof_check_string(char * tp);	//Returns nonzero if the string contains at least one non space ' ' character before it terminates

//Unicode font handling stuff
extern unsigned short * eof_ucode_table;						//Stores Unicode mappings for the 256 extended ASCII characters, some of which are set to 0x20 for nonprintable characters
void eof_allocate_ucode_table(void);							//Populates the contents of eof_ucode_table[]
void eof_free_ucode_table(void);								//Frees eof_ucode_table[]
int eof_convert_from_extended_ascii(char * buffer, int size);	//Convert a string from 8-bit ASCII to the current format
int eof_convert_to_extended_ascii(char * buffer, int size);		//Convert a string from the current format to 8-bit ASCII
int eof_lookup_extended_ascii_code(int character);
	//Accepts an input UTF-8 character and returns the matching extended ASCII code from eof_ucode_table[] if it exists
	//Returns 0 if there is no match or upon error
int rs_lyric_substitute_char_utf8(int character, int function);
	//Converts the input UTF-8 character into extended ASCII and uses rs_lyric_substitute_char_extended() to search for an applicable normal ASCII substitution
	//  for example, replacing an accented A character with a non-accented version.
	//If function is 0, substitutions are sought for any accented Latin character
	//If function is 1, substitutions are only sought for characters that Rocksmith doesn't support
	//If no substitution is found, the input character is returned unchanged

int eof_string_has_non_ascii(char *str);	//Returns nonzero if any characters in the UTF-8 encoded string have non ASCII characters (any character valued over 127)
int eof_string_has_non_alphanumeric(char *str);	//Returns nonzero if any characters in the UTF-8 encoded string have non alphanumeric ASCII characters
void eof_sanitize_string(char *str);		//Replaces any non-printable or non ASCII (characters numbered higher than 127) characters with spaces
void eof_build_sanitized_filename_string(char *input, char *output);
	//Builds a copy of the input string that filters out all characters that aren't allowed in filenames ( \ / : * ? " < > | )
	//If the input name ends in whitespace, it is removed from the output string as that is not valid at least in Windows
	//Do not pass more than one folder's or file's name as the path separators will be removed from a folder\name file path as a result
	//input and output must be non-overlapping arrays
int eof_char_is_hex(int c);
	//Returns nonzero if c is a numerical digit or an upper/lower case letter A, B, C, D, E or F
int eof_string_is_hexadecimal(char *string);
	//Returns nonzero if all characters in the provided string are hexadecimal characters
	//Returns 0 on error

int eof_is_illegal_filename_character(char c);	//Returns nonzero if the specified character is not legal for use in a filename in Windows

int eof_remake_color(int hexrgb);	//Accepts a color in hexadecimal RGB format and returns the result of makecol() passed each relevant byte

PACKFILE *eof_pack_fopen_retry(const char *filename, const char *mode, unsigned count);
	//Attempts to open the specified file in the specified mode up to [count] number of times, with a 1ms delay between each attempt
	//If any attempts succeed, the PACKFILE handle is returned, otherwise if all attempts fail, NULL is returned

int eof_number_is_power_of_two(unsigned long value);
	//Returns nonzero if the specified value is any power of two from 2^0 through 2^31

int eof_parse_last_folder_name(const char *filename, char *buffer, unsigned long bufferlen);
	//Parses the last folder name in the given filename, which is expected to contain at least one folder name, one folder separator and one filename (3 characters)
	//The output buffer is expected to be able to store the specified number of CHARACTERS, which may be two bytes each if the folder name has Unicode characters
	//Returns 0 on error

int eof_byte_to_binary_string(unsigned char value, char *buffer);
	//Places a binary string representation of the given 8 bit value into the given buffer, which must be able to store at least 9 bytes
	//Leading zeroes are dropped
	//Returns nonzero on error

void eof_check_and_log_lyric_line_errors(EOF_SONG *sp, char force);
	//A debugging function that detects errors with lyric lines and if any are found, or if force is nonzero,
	// logs all lyric line timings and the index number and text of each line's lyrics

extern char *eof_os_clipboard;	//A memory buffer that is recreated during each call to eof_get_clipboard()
extern int eof_gas_clipboard;		//For fun
int eof_get_clipboard(void);
	//In Windows, attempts to read text from the OS clipboard into os_clipboard.txt in EOF's program folder
	//For other Operating Systems, attempts to read the content of os_clipboard.txt as created by eof_set_clipboard()
	//If that succeeds, the contents of os_clipboard.txt are buffered into eof_os_clipboard[], the Byte Order Mark is removed if present and the buffer is NULL-terminated
	//Returns nonzero on error
int eof_set_clipboard(char *text);
	//Deletes and recreates os_clipboard.txt to include the specified text
	//In Windows, then attempts to write the specified text to the OS clipboard
	//Returns nonzero on error

unsigned long eof_pack_read_vlv(PACKFILE *fp, unsigned long *byte_counter);
	//Reads and returns a variable length value from the specified PACKFILE handle, returns ULONG_MAX on error
	//If byte_counter is not NULL, it is incremented for each byte that is read from the PACKFILE

int eof_packfile_search_phrase(PACKFILE *inf, const char *phrase, unsigned long phraselen, unsigned long *bytes_read);
	//Searches from the current file position of inf for the first match of the specified array of characters
	//phrase is an array of characters to find, and phraselen is the number of characters defined in the array
	//If a match is found, the file position is left at the first byte after that first match and 1 is returned
	//If inf or phrase are NULL or if an I/O error occurs, -1 is returned
	//If the file is parsed but no match is found, 0 is returned
	//If bytes_read is not NULL, it will store the number of bytes that were read in the PACKFILE stream by this function if the function does not return in error

unsigned long eof_find_wav_metadata(char *filename, EOF_AUDIO_METADATA *metadata, unsigned long metadata_count);
	//Search for the specified list of RIFF format metadata in the specified WAV file
	//metadata_count is the size of the metadata array and the number of individual pieces of metadata to search for
	//Returns the number of specified metadata items that were found, or 0 on error

#endif
