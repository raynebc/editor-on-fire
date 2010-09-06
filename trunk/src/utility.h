#ifndef EOF_UTILITY_H
#define EOF_UTILITY_H

int eof_chdir(const char * dir);
int eof_mkdir(const char * dir);
int eof_system(const char * command);

void * eof_buffer_file(char * fn);	//Loads the specified file into a memory buffer and returns the buffer, or NULL upon error
int eof_copy_file(char * src, char * dest);	//Copies the source file to the destination file.  Returns 0 upon error

int eof_check_string(char * tp);	//Returns nonzero if the string contains any non space ' ' characters and is at least one character long

#endif
