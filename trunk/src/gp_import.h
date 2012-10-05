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
	struct eof_guitar_pro_struct
	{
		unsigned long numtracks;		//The number of tracks loaded from the guitar pro file
		char **names;					//An array of strings, representing the native name of each loaded track
		EOF_PRO_GUITAR_TRACK **track;	//An array of pro guitar track pointers, representing the imported note data of each loaded track
	};

	struct eof_gp_time_signature
	{
		unsigned char num, den;		//The 8 bit numerator and denominator defined in guitar pro time signatures
	};

	void eof_gp_debug_log(PACKFILE *inf, char *text);
		//Does nothing in the EOF build
	EOF_SONG *eof_import_gp(const char * fn);
		//Currently parses through the specified GP file and outputs its details to stdout
	struct eof_guitar_pro_struct *eof_load_gp(const char * fn, char *undo_made);
		//Parses the specified guitar pro file, returning a structure of track information and a populated pro guitar track for each
		//Returns NULL on error
		//NOTE:  Beats are added to the current project if there aren't as many as defined in the GP file.
		//If the user opts to import the GP file's time signatures, an undo state will be made if undo_made is not NULL, and the referenced memory will be set to nonzero
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


