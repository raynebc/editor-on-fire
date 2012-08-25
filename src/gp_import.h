#ifndef EOF_GP_IMPORT_H
#define EOF_GP_IMPORT_H

#ifndef EOF_BUILD
	//Compile standalone parse utility, use macros to make the file compile without Allegro
	#define PACKFILE FILE
	#define pack_fread(x, y, z) fread(x, 1, y, z)
	#define pack_fopen fopen
	#define pack_fclose fclose
	#define pack_getc getc
	#define pack_fseek(x, y) fseek(x, y, SEEK_CUR)
	#define EOF_SONG char
	#define pack_feof feof
	void eof_gp_debug_log(FILE *inf, char *text);		//Prints the current file position and the specified string to the console
	extern char *eof_note_names[12];
#else
	void eof_gp_debug_log(PACKFILE *inf, char *text);	//Does nothing in the EOF build
	EOF_SONG *eof_import_gp(const char * fn);	//Currently parses through the specified GP file and outputs its details to stdout
#endif

void pack_ReadWORDLE(PACKFILE *inf, unsigned *data);
	//Read a little endian format 2 byte integer from the specified file.  If data isn't NULL, the value is stored into it.
void pack_ReadDWORDLE(PACKFILE *inf, unsigned long *data);
	//Read a little endian format 4 byte integer from the specified file.  If data isn't NULL, the value is stored into it.
int eof_read_gp_string(PACKFILE *inf, unsigned *length, char *buffer, char readfieldlength);
	//Read a string, prefixed by a one-byte string length, from the specified file
	//If readfieldlength is nonzero, a 4 byte field length that prefixes the string length is also read
	//If length is not NULL, the string length is returned through it.
	//Buffer must be able to store at least 256 bytes to avoid overflowing.
void eof_gp_parse_bend(PACKFILE *inf);
	//Parses the bend at the current file position, outputting debug logging appropriately

#endif


