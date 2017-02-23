#ifndef EOF_UTILITY_H
#define EOF_UTILITY_H

int eof_chdir(const char * dir);
int eof_mkdir(const char * dir);
int eof_system(const char * command);	//Runs the specified system command, after reformatting the command string accordingly

void * eof_buffer_file(const char * fn, char appendnull);
	//Loads the specified file into a memory buffer and returns the buffer, or NULL upon error
	//If appendnull is nonzero, an extra 0 byte is written at the end of the buffer, potentially necessary if buffering a file that will be read as text, ensuring the buffer is NULL terminated
int eof_copy_file(const char * src, const char * dest);	//Copies the source file to the destination file.  Returns 0 upon error
int eof_file_compare(char *file1, char *file2);	//Returns zero if the two files are the same

int eof_check_string(char * tp);	//Returns nonzero if the string contains at least one non space ' ' character before it terminates

void eof_allocate_ucode_table(void);
void eof_free_ucode_table(void);
int eof_convert_extended_ascii(char * buffer, int size);

int eof_string_has_non_ascii(char *str);	//Returns nonzero if any characters in the string have non ASCII characters (any character valued over 127)
void eof_sanitize_string(char *str);		//Replaces any non-printable or non ASCII (characters numbered higher than 127) characters with spaces

int eof_is_illegal_filename_character(char c);	//Returns nonzero if the specified character is not legal for use in a filename in Windows

int eof_remake_color(int hexrgb);	//Accepts a color in hexadecimal RGB format and returns the result of makecol() passed each relevant byte

#endif
