#ifndef EOF_UTILITY_H
#define EOF_UTILITY_H

int eof_chdir(const char * dir);
int eof_mkdir(const char * dir);
int eof_system(const char * command);

void * eof_buffer_file(char * fn);
int eof_copy_file(char * src, char * dest);

int eof_check_string(char * tp);

#endif
