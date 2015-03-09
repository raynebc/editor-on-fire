#ifndef EOF_BR_H
#define EOF_BF_H

#include "song.h"

struct bf_section
{
	unsigned long long indkey;	//The 8 byte index key identifier
	unsigned long long objkey;	//The 8 byte object key identifier
	unsigned long address;		//The file address at which this section begins
	char encountered;			//Is set to nonzero the first time it is encountered among ZOBJ sections to track whether the section is a parent or child object
};

struct bf_string
{
	unsigned long long indkey;	//The 8 byte index key identifier
	unsigned long offset;		//The file offset from the beginning of the string table at which this section begins
	char *string;				//The string itself
};

void pack_ReadDWORDBE(PACKFILE *inf, void *data);
	//Read a big endian ordered set of 4 bytes from the specified file.  If data isn't NULL, the value is stored into it.
void pack_ReadQWORDBE(PACKFILE *inf, void *data);
	//Read a big endian ordered set of 8 bytes from the specified file.  If data isn't NULL, the value is stored into it.

char *eof_lookup_bf_string_key(struct bf_string *ptr, unsigned long arraysize, unsigned long long key);
	//Looks for the specified key in the specified bf_string array, returning the pointer to the first matching string, or NULL if there is no match

int eof_dword_to_binary_string(unsigned long dword, char *buffer);
	//Places a binary string representation of the given 32 bit value into the given buffer, which must be able to store at least 33 bytes
	//Returns nonzero on error

EOF_SONG *eof_load_bf(char * fn);
	//Parses the specified Bandfuse RIFF file and returns its content in a pro guitar track structure

#endif
