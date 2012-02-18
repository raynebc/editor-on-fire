#ifndef GH_IMPORT_H
#define GH_IMPORT_H

#include <allegro.h>
#include "song.h"

#define EOF_GH_CRC32(x) (eof_crc32(x) ^ 0xFFFFFFFF)

typedef struct
{
	unsigned char *buffer;
	size_t size;
	unsigned long index;
} filebuffer;	//This structure will be used for buffered file I/O

typedef struct
{
	char *name;
	int tracknum;
	int diffnum;
} gh_section;	//This structure will be used to associate a guitar hero section with a specific track and difficulty (if applicable)

extern unsigned long crc32_lookup[256];
extern char crc32_lookup_initialized;

EOF_SONG * eof_import_gh(const char * fn);
	//Imports the specified Guitar Hero file

filebuffer *eof_filebuffer_load(const char * fn);
	//Initializes a filebuffer struct, loads the specified file into memory and returns the struct
	//Returns NULL on error
void eof_filebuffer_close(filebuffer *fb);
	//Frees the memory buffer and the filebuffer structure
int eof_filebuffer_get_byte(filebuffer *fb, unsigned char *ptr);
	//Reads the next 1 byte value into ptr
	//Returns EOF on error, otherwise returns 0
int eof_filebuffer_get_word(filebuffer *fb, unsigned int *ptr);
	//Reads the next 2 byte value into ptr, in big endian fashion
	//Returns EOF on error, otherwise returns 0
int eof_filebuffer_get_dword(filebuffer *fb, unsigned long *ptr);
	//Returns the next 4 byte value into ptr, in big endian fashion
	//Returns EOF on error, otherwise returns 0
unsigned long eof_crc32_reflect(unsigned long value, int numbits);
	//Returns the passed value with the (numbits) number of low order bits reflected (swapped)
unsigned long eof_crc32(char *string);
	//Returns the CRC-32 checksum of the specified string, or 0 on error
int eof_filebuffer_find_checksum(filebuffer *fb, unsigned long checksum);
	//Looks for the 4 byte checksum in the buffered file starting at the current position
	//If the checksum is found, the position is set to the byte that follows it and zero is returned
	//If the checksum is not found, the position is left unchanged and nonzero is returned
int eof_gh_read_instrument_section(filebuffer *buffer, EOF_SONG *sp, gh_section *target);
	//Searches for the target instrument section in the buffered file
	//If the section is not found, 0 is returned
	//If an error is detected, -1 is returned
	//If it is found, it is parsed and notes are added accordingly to the passed EOF_SONG structure
int eof_gh_read_sp_section(filebuffer *fb, EOF_SONG *sp, gh_section *target);
	//Searches for the target section in the buffered file
	//If the section is not found, 0 is returned
	//If an error is detected, -1 is returned
	//If it is found, it is parsed and the star power sections are added accordingly to the passed EOF_SONG structure

int eof_gh_read_tap_section(filebuffer *fb, EOF_SONG *sp, gh_section *target);
	//Searches for the target section in the buffered file
	//If the section is not found, 0 is returned
	//If an error is detected, -1 is returned
	//If it is found, it is parsed and the tap sections are added accordingly to the passed EOF_SONG structure as slider sections

#endif
