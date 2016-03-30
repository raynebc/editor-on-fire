#ifndef GH_IMPORT_H
#define GH_IMPORT_H

#include <allegro.h>
#include "song.h"

#define EOF_GH_CRC32(x) (eof_crc32(x) ^ 0xFFFFFFFF)
	//GH checksums take a 32 bit CRC of the string text, and XOR the result by 0xFFFFFFFF

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
extern EOF_SONG *eof_sections_list_all_ptr;	//eof_sections_list_all() lists the text events in this chart

EOF_SONG * eof_import_gh(const char * fn);
	//Imports the specified Guitar Hero file
EOF_SONG * eof_import_gh_note(const char * fn);
	//Imports the specified Guitar Hero file, parsing as a NOTE format (ie. GH5) file
EOF_SONG * eof_import_gh_qb(const char *fn);
	//Imports the specified Guitar Hero file, parsing as a QB format (ie. GHWT) file

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
int eof_filebuffer_memcpy(filebuffer *fb, void *ptr, size_t num);
	//Copies num bytes from the buffered file into ptr
	//Returns EOF on error, otherwise returns 0
int eof_filebuffer_read_unicode_chars(filebuffer *fb, char *ptr, unsigned long num);
	//Reads the specified number of Unicode characters from the buffered file, storing as ASCII characters into ptr
	//Returns EOF on error, otherwise returns 0
unsigned long eof_crc32_reflect(unsigned long value, unsigned numbits);
	//Returns the passed value with the (numbits) number of low order bits reflected (swapped)
unsigned long eof_crc32(const char *string);
	//Returns the CRC-32 checksum of the specified string, or 0 on error
unsigned long eof_gh_checksum(const char *string);
	//Returns the GH checksum for the string (CRC32 XOR'd by 0xFFFFFFFF)
	//If string is NULL, then a value of 0 is returned
int eof_filebuffer_find_bytes(filebuffer *fb, const void *bytes, size_t searchlen, unsigned char seektype);
	//Searches from the current buffer position for the first match of the sequence of byte values
	//bytes is an array of byte values to find, and searchlen is the number of bytes defined in the array
	//If no match is found, the buffer position is returned to what it was when the function was called
	//If a match is found, the buffer position is set based on the value of seektype and 1 is returned
	//If seektype is 1, the buffer position is set to the first byte of the match
	//If seektype is 2, the buffer position is set to the first byte that follows the match
	//If an error occurs, -1 is returned
	//If the buffer is parsed but no match is found, 0 is returned
int eof_filebuffer_find_checksum(filebuffer *fb, unsigned long checksum);
	//Looks for the 4 byte checksum in the buffered file starting at the current position
	//If the checksum is found, the position is set to the byte that follows it and zero is returned
	//If the checksum is not found, the position is left unchanged and nonzero is returned
int eof_gh_read_instrument_section_note(filebuffer *buffer, EOF_SONG *sp, gh_section *target, char forcestrum);
	//Searches for the target instrument section in the buffered file (NOTE format GH file)
	//If the section is not found, 0 is returned
	//If an error is detected, -1 is returned
	//If it is found, it is parsed and notes are added accordingly to the passed EOF_SONG structure
	//If forcestrum is nonzero, all non HOPO gems for guitar tracks are marked as explicit HOPO OFF notes (as they are played in GH)
int eof_gh_read_sp_section_note(filebuffer *fb, EOF_SONG *sp, gh_section *target);
	//Searches for the target section in the buffered file (NOTE format GH file)
	//If the section is not found, 0 is returned
	//If an error is detected, -1 is returned
	//If it is found, it is parsed and the star power sections are added accordingly to the passed EOF_SONG structure
void eof_process_gh_lyric_phrases(EOF_SONG *sp);
	//Determines appropriate ending positions for each lyric phrase, since GH files only define a start position for each
	//The lyrics and lyric lines are expected to already be sorted by timestamp
int eof_gh_read_vocals_note(filebuffer *fb, EOF_SONG *sp);
	//Searches the buffered file for vocal data and loads them into the specified EOF_SONG structure  (NOTE format GH file)
	//If vocals are not found, 0 is returned
	//If an error is detected, -1 is returned
int eof_gh_read_tap_section_note(filebuffer *fb, EOF_SONG *sp, gh_section *target);
	//Searches for the target section in the buffered file (NOTE format GH file)
	//If the section is not found, 0 is returned
	//If an error is detected, -1 is returned
	//If it is found, it is parsed and the tap sections are added accordingly to the passed EOF_SONG structure as slider sections

struct QBlyric *eof_gh_read_section_names(filebuffer *fb);
	//Searches the buffered file for the next set of section markers (from current buffer position),
	//returning a linked list of section name and checksum pairs, to be used for QB or NOTE GH import
	//NULL is returned on error
int eof_gh_read_sections_note(filebuffer *fb, EOF_SONG *sp);
	//Searches the buffered file for section markers and loads them into the specified EOF_SONG structure (NOTE format GH file)
	//Each language of sections are loaded individually and presented to the user so that the user can select which language to import
	//If sections are not found, or the user cancels loading sections, 0 is returned
	//If an error is detected, -1 is returned
int eof_gh_read_sections_qb(filebuffer *fb, EOF_SONG *sp);
	//Searches the buffered file for section markers and loads them into the specified EOF_SONG structure (QB format GH file)
	//If sections are not found, 0 is returned
	//If an error is detected, -1 is returned
	//If the user cancels the loading of sections, -2 is returned

int eof_gh_read_instrument_section_qb(filebuffer *fb, EOF_SONG *sp, const char *songname, gh_section *target, unsigned long qbindex, char forcestrum);
	//Searches for the target instrument section in the buffered file (QB format GH file)
	//songname is a string representing the song's name, which is a prefix for each section name
	//If the section is not found, 0 is returned
	//If an error is detected, -1 is returned
	//If it is found, it is parsed and notes are added accordingly to the passed EOF_SONG structure
	//If forcestrum is nonzero, all non HOPO gems for guitar tracks are marked as explicit HOPO OFF notes (as they are played in GH)
int eof_gh_read_sp_section_qb(filebuffer *fb, EOF_SONG *sp, const char *songname, gh_section *target, unsigned long qbindex);
	//Searches for the target section in the buffered file (QB format GH file)
	//songname is a string representing the song's name, which is a prefix for each section name
	//If the section is not found, 0 is returned
	//If an error is detected, -1 is returned
	//If it is found, it is parsed and the star power sections are added accordingly to the passed EOF_SONG structure
int eof_gh_read_tap_section_qb(filebuffer *fb, EOF_SONG *sp, const char *songname, gh_section *target, unsigned long qbindex);
	//Searches for the target section in the buffered file (QB format GH file)
	//songname is a string representing the song's name, which is a prefix for each section name
	//If the section is not found, 0 is returned
	//If an error is detected, -1 is returned
	//If it is found, it is parsed and the tap sections are added accordingly to the passed EOF_SONG structure as slider sections
int eof_gh_read_vocals_qb(filebuffer *fb, EOF_SONG *sp, const char *songname, unsigned long qbindex);
	//Searches the buffered file for vocal data and loads them into the specified EOF_SONG structure  (QB format GH file)
	//songname is a string representing the song's name, which is a prefix for each section name
	//If vocals are not found, 0 is returned
	//If an error is detected, -1 is returned

unsigned long eof_gh_process_section_header(filebuffer *fb, const char *sectionname, unsigned long **arrayptr, unsigned long qbindex);
	//Generates the checksum for the passed section name and attempts to find it in the buffered file
	//qbindex is expected to be the buffer position of the QB header (which is required to seek within the QB data)
	//If found, *arrayptr is assigned allocated memory and populated with the offsets for each 1D array in this section
	//On success, the number of 1D arrays referenced in arrayptr[] is returned
	//On error, 0 is returned, the memory allocated for *arrayptr is automatically released and *arrayptr is set to NULL
unsigned long eof_gh_read_array_header(filebuffer *fb, unsigned long qbpos, unsigned long qbindex);
	//Seeks the buffer to the specified QB position and parses the array size and data offset
	//qbindex is expected to be the buffer position of the QB header (which is required to seek within the QB data)
	//On success, the buffer is seeked to the first data position of the array and the array size (in dwords) is returned
	//Returns 0 on error

unsigned long eof_char_to_binary(unsigned char input);
	//A function for debugging purposes that accepts a 1 byte value and returns a binary representation in decimal format (ie. 0xFF is returned as 11111111)
char *eof_sections_list_all(int index, int * size);
	//A list box function to display the text events in the EOF_SONG structure pointed at by eof_sections_list_all_ptr

#endif
