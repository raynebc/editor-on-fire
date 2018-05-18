#include <allegro.h>
#include <ctype.h>
#include <sys/stat.h>
#include "utility.h"
#include "main.h"	//For logging
#include "foflc/RS_parse.h"	//For rs_lyric_substitute_char_extended()

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

unsigned short * eof_ucode_table = NULL;

int eof_chdir(const char * dir)
{
	#ifdef ALLEGRO_WINDOWS
		wchar_t wdir[1024] = {0};

		if(dir == NULL)
			return -1;
		(void) uconvert(dir, U_UTF8, (char *)(&wdir[0]), U_UNICODE, 2048);
		return _wchdir(wdir);
	#else
		if(dir == NULL)
			return -1;
		return chdir(dir);
	#endif
}

int eof_mkdir(const char * dir)
{
	#ifdef ALLEGRO_WINDOWS
		wchar_t wdir[1024] = {0};

		if(dir == NULL)
			return -1;
		(void) uconvert(dir, U_UTF8, (char *)(&wdir[0]), U_UNICODE, 2048);
		return _wmkdir(wdir);
	#else
		if(dir == NULL)
			return -1;
		return mkdir(dir, 0777);
	#endif
}

int eof_system(const char * command)
{
	#ifdef ALLEGRO_WINDOWS
		wchar_t wcommand[1024] = {0};

		if(command == NULL)
			return -1;
		(void) uconvert(command, U_UTF8, (char *)(&wcommand[0]), U_UNICODE, 2048);
		return _wsystem(wcommand);
	#else
		if(command == NULL)
			return -1;
		return system(command);
	#endif
}

void * eof_buffer_file(const char * fn, char appendnull)
{
// 	eof_log("eof_buffer_file() entered");

	void * data = NULL;
	PACKFILE * fp = NULL;
	size_t filesize, buffersize;

	if(fn == NULL)
		return NULL;
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tBuffering file:  \"%s\"", fn);
	eof_log(eof_log_string, 1);
	fp = pack_fopen(fn, "r");
	if(fp == NULL)
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tCannot open specified file:  \"%s\"", strerror(errno));	//Get the Operating System's reason for the failure
		eof_log(eof_log_string, 1);
		return NULL;
	}
	filesize = buffersize = file_size_ex(fn);
	if(appendnull)
	{	//If adding an extra NULL byte of padding to the end of the buffer
		buffersize++;	//Allocate an extra byte for the padding
	}
	data = (char *)malloc(buffersize);
	if(data == NULL)
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tCannot allocate %lu bytes of memory.", (unsigned long)buffersize);
		eof_log(eof_log_string, 1);
		return NULL;
	}

	(void) pack_fread(data, (long)filesize, fp);
	if(appendnull)
	{	//If adding an extra NULL byte of padding to the end of the buffer
		((char *)data)[buffersize - 1] = 0;	//Write a 0 byte at the end of the buffer
	}
	(void) pack_fclose(fp);
	return data;
}

int eof_copy_file(const char * src, const char * dest)
{
	PACKFILE * src_fp = NULL;
	PACKFILE * dest_fp = NULL;
	void *ptr = NULL;	//Used to buffer memory
	unsigned long src_size = 0;
	unsigned long i;

 	eof_log("eof_copy_file() entered", 1);

	if((src == NULL) || (dest == NULL))
		return 0;
	if(!ustricmp(src,dest))		//Special case:  The input and output file are the same
		return 0;				//Return success without copying any files

	src_size = (unsigned long)file_size_ex(src);
	if(src_size > LONG_MAX)
		return 0;	//Unable to validate I/O due to Allegro's usage of signed values
	src_fp = pack_fopen(src, "r");
	if(!src_fp)
	{
		return 0;
	}
	dest_fp = pack_fopen(dest, "w");
	if(!dest_fp)
	{
		(void) pack_fclose(src_fp);
		return 0;
	}
//Attempt to buffer the input file into memory for faster read and write
	ptr = malloc((size_t)src_size);
	if(ptr != NULL)
	{	//If a buffer large enough to store the input file was created
		long long_src_size = src_size;
		if((pack_fread(ptr, long_src_size, src_fp) != long_src_size) || (pack_fwrite(ptr, long_src_size, dest_fp) != long_src_size))
		{	//If there was an error reading from file or writing from memory
			free(ptr);	//Release buffer
			return 0;	//Return error
		}
		free(ptr);	//Release buffer
	}
	else
	{	//Otherwise copy the slow way (one byte at a time)
		for(i = 0; i < src_size; i++)
		{
			(void) pack_putc(pack_getc(src_fp), dest_fp);
		}
	}
	(void) pack_fclose(src_fp);
	(void) pack_fclose(dest_fp);
	return 1;
}

int eof_check_string(char * tp)
{
	int i;

	if(tp == NULL)
		return 0;

	for(i = 0; tp[i] != '\0'; i++)
	{	//For each character in the string until the NULL terminator is reached
		if(tp[i] != ' ')
		{
			return 1;
		}
	}
	return 0;
}

int eof_file_compare(char *file1, char *file2)
{
	uint64_t filesize,ctr;
	int data1,data2;
	PACKFILE *fp1 = NULL,*fp2 = NULL;
	char result = 0;	//The result is assumed to "files identical" until found otherwise

 	eof_log("eof_file_compare() entered", 1);

	if((file1 == NULL) || (file2 == NULL))
	{
		return 2;	//Return error
	}

	filesize = file_size_ex(file1);	//Get length of file1
	if(filesize != file_size_ex(file2))
	{	//If file1 and file2 are different lengths
		return 1;	//Return files don't match
	}

	fp1 = pack_fopen(file1, "r");
	if(fp1 == NULL)
	{
		return 2;	//Return error
	}
	fp2 = pack_fopen(file2, "r");
	if(fp2 == NULL)
	{
		(void) pack_fclose(fp1);
		return 2;	//Return error
	}

	for(ctr = 0;ctr < filesize; ctr++)
	{	//For each byte in the files
		data1 = pack_getc(fp1);	//Read one byte from each
		data2 = pack_getc(fp2);
		if((data1 == EOF) || (data2 == EOF))
		{	//If EOF was reached unexpectedly
			break;	//Exit loop
		}
		if(data1 != data2)
		{
			result = 1;	//Store a "non identical" result
			break;		//Exit loop
		}
	}
	(void) pack_fclose(fp1);
	(void) pack_fclose(fp2);

	return result;
}

/* allocate and set ucode table for 8-bit ASCII conversion */
void eof_allocate_ucode_table(void)
{
	int i;

	if(!eof_ucode_table)
	{
		eof_ucode_table = malloc(sizeof(unsigned short) * 256);
		if(eof_ucode_table)
		{
			//Most of the extended ASCII characters map directly to the first 256 Unicode characters
			for(i = 0; i < 256; i++)
			{
				eof_ucode_table[i] = i;
			}

			//However characters 128 - 159 have unique mappings
			eof_ucode_table[128] = 0x20AC;	//Euro sign
			eof_ucode_table[129] = 0x20;	//Unassigned, map to space character
			eof_ucode_table[130] = 0x201A;	//Single Low-9 Quotation Mark
			eof_ucode_table[131] = 0x192;	//Latin Small Letter F With Hook
			eof_ucode_table[132] = 0x201E;	//Double Low-9 Quotation Mark
			eof_ucode_table[133] = 0x2026;	//Horizontal Ellipsis
			eof_ucode_table[134] = 0x2020;	//Dagger
			eof_ucode_table[135] = 0x2021;	//Double Dagger
			eof_ucode_table[136] = 0x2C6;	//Modifier Letter Circumflex Accent
			eof_ucode_table[137] = 0x2030;	//Per Mille Sign
			eof_ucode_table[138] = 0x160;	//Latin Capital Letter S With Caron
			eof_ucode_table[139] = 0x2039;	//Single Left-Pointing Angle Quotation Mark
			eof_ucode_table[140] = 0x152;	//Latin Capital Ligature OE
			eof_ucode_table[141] = 0x20;	//Unassigned, map to space character
			eof_ucode_table[142] = 0x17D;	//Latin Capital Letter Z With Caron
			eof_ucode_table[143] = 0x20;	//Unassigned, map to space character
			eof_ucode_table[144] = 0x20;	//Unassigned, map to space character
			eof_ucode_table[145] = 0x2018;	//Left Single Quotation Mark
			eof_ucode_table[146] = 0x2019;	//Right Single Quotation Mark
			eof_ucode_table[147] = 0x201C;	//Left Double Quotation Mark
			eof_ucode_table[148] = 0x201D;	//Right Double Quotation Mark
			eof_ucode_table[149] = 0x2022;	//Bullet
			eof_ucode_table[150] = 0x2013;	//En Dash
			eof_ucode_table[151] = 0x2014;	//Em Dash
			eof_ucode_table[152] = 0x2DC;	//Small Tilde
			eof_ucode_table[153] = 0x2122;	//Trade Mark Sign
			eof_ucode_table[154] = 0x161;	//Latin Small Letter S With Caron
			eof_ucode_table[155] = 0x203A;	//Single Right-Pointing Angle Quotation Mark
			eof_ucode_table[156] = 0x153;	//Latin Small Ligature OE
			eof_ucode_table[157] = 0x20;	//Unassigned, map to space character
			eof_ucode_table[158] = 0x17E;	//Latin Small Letter Z With Caron
			eof_ucode_table[159] = 0x178;	//Latin Capital Letter Y With Diaeresis

			set_ucodepage(eof_ucode_table, NULL);
		}
	}
}

void eof_free_ucode_table(void)
{
	if(eof_ucode_table)
	{
		free(eof_ucode_table);
		eof_ucode_table = NULL;
	}
}

/* convert a string from 8-bit ASCII to the current format */
int eof_convert_from_extended_ascii(char * buffer, int size)
{
	char * workbuffer = NULL;

	if(!buffer)
		return 0;	//Invalid parameters

	if(!eof_ucode_table)
	{
		return 0;
	}
	workbuffer = malloc((size_t)size);
	if(!workbuffer)
	{
		return 0;
	}
	strncpy(workbuffer, buffer, (size_t)size - 1);
	do_uconvert(workbuffer, U_ASCII_CP, buffer, U_CURRENT, size);
	free(workbuffer);
	return 1;
}

/* convert a string from the current format to 8-bit ASCII */
int eof_convert_to_extended_ascii(char * buffer, int size)
{
	char * workbuffer = NULL;

	if(!buffer)
		return 0;	//Invalid parameters

	if(!eof_ucode_table)
	{
		return 0;
	}
	workbuffer = malloc((size_t)size);
	if(!workbuffer)
	{
		return 0;
	}
	memcpy(workbuffer, buffer, (size_t)size);
	do_uconvert(workbuffer, U_CURRENT, buffer, U_ASCII_CP, size);
	free(workbuffer);
	return 1;
}

int rs_lyric_substitute_char_utf8(int character, int function)
{
	char string[6] = {0};

	(void) usetat(string, 0, character);	//Build a terminated string containing just the input character
	(void) usetat(string, 1, '\0');
	if(eof_convert_to_extended_ascii(string, (int)sizeof(string)))
	{	//If the string was converted
		int substitution = rs_lyric_substitute_char_extended((unsigned char)string[0], function);	//Check for a substitution of the given kind

		if(substitution != string[0])
		{	//If a suitable ASCII substitution was found
			return substitution;
		}
	}

	return character;	//Return the input character unchanged
}

int eof_lookup_extended_ascii_code(int character)
{
	unsigned ctr;

	if(!eof_ucode_table)
	{
		return 0;	//Error
	}

	for(ctr = 0; ctr < 256; ctr++)
	{	//For each entry in eof_ucode_table[]
		if(eof_ucode_table[ctr] == character)
			return ctr;
	}

	return 0;	//No match
}

int eof_string_has_non_ascii(char *str)
{
	unsigned long ctr, length;
	int val;

	if(!str)
		return 0;

	for(ctr = 0, length = ustrlen(str); ctr < length; ctr++)
	{	//For each character of the string
		val = ugetat(str, ctr);
		if(val > 127)
		{	//If the character is not normal ASCII (value 0 through 127)
			return 1;
		}
	}
	return 0;
}

int eof_string_has_non_alphanumeric(char *str)
{
	unsigned long ctr, length;
	int val;

	if(!str)
		return 0;

	for(ctr = 0, length = ustrlen(str); ctr < length; ctr++)
	{	//For each character of the string
		val = ugetat(str, ctr);
		if(!isalnum(val))
		{	//If the character isn't an alphanumeric ASCII character
			return 1;
		}
	}
	return 0;
}

void eof_sanitize_string(char *str)
{
	unsigned char ch;
	unsigned long index = 0;

	if(!str)
		return;

	while(str[index] != '\0')
	{	//Until the end of the string is reached
		ch = str[index];
		if((ch > 127) || !isprint(ch))
		{	//If this character is deemed invalid
			str[index] = ' ';	//Replace with a space
		}
		index++;
	}
}

int eof_is_illegal_filename_character(char c)
{
	switch(c)
	{
		case '\\':
		case '/':
		case ':':
		case '*':
		case '?':
		case '"':
		case '<':
		case '>':
		case '|':
		return 1;	//Characters that are illegal in Windows file names

		default:
		break;
	}

	return 0;	//Valid for use in a filename
}

int eof_remake_color(int hexrgb)
{
	int r, g, b;

	r = (hexrgb >> 16) & 0xFF;
	g = (hexrgb >> 8) & 0xFF;
	b = hexrgb & 0xFF;

	return makecol(r, g, b);
}

int eof_char_is_hex(int c)
{
	int upper;

	if(isdigit(c))		//Numerical digits are valid hexadecimal characters
		return 1;
	if(!isalpha(c))		//Otherwise they must be alphabetical
		return 0;

	upper = toupper(c);
	if((upper == 'A') || (upper == 'B') || (upper == 'C') || (upper == 'D') || (upper == 'E') || (upper == 'F'))
		return 1;	//And be A, B, C, D, E or F

	return 0;		//No other characters are valid hexadecimal characters
}

int eof_string_is_hexadecimal(char *string)
{
	unsigned long index;

	if(!string)
		return 0;	//Invalid parameter

	for(index = 0; string[index] != '\0'; index++)
	{	//For each character in the string
		if(!eof_char_is_hex(string[index]))
			return 0;
	}

	return 1;	//No non-hexadecimal characters were encountered
}
