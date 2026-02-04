#include <allegro.h>
#include <ctype.h>
#include <sys/stat.h>
#include "utility.h"
#include "main.h"	//For logging
#include "mix.h"
#include "foflc/RS_parse.h"	//For rs_lyric_substitute_char_extended()
#include "menu/file.h"		//For eof_menu_file_link_ffmpeg()
#include "modules/g-idle.h"	//For Idle()

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

unsigned short * eof_ucode_table = NULL;
char *eof_os_clipboard = NULL;
int eof_gas_clipboard = 0;

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

int eof_folder_exists(const char * dir)
{
	char *rebuilt_path = NULL;
	const char *path;
	int aret = 0, length, endchar;
	int match = 1;

	if(!dir)
		return 0;	//Invalid parameter

	//Rebuild the string if necessary, to ensure it does not end in a file separator
	path = dir;		//By default, use the specified string as-is
	length = ustrlen(dir);	//Read the number of characters (Unicode aware) in the input string
	if(length == 0)
		return 0;	//Invalid parameter (empty string)
	endchar = ugetat(dir, length - 1);	//Find the character at the end of the input string

	if((endchar == '\\') || (endchar == '/'))
	{	//If the input string ends in a file separator
		rebuilt_path = malloc(length + 2);	//Allocate enough space to copy the input string, plus a two byte NULL terminator (instead of one byte, in case UTF-16 is in effect for some reason)
		if(!rebuilt_path)
			return 0;	//Couldn't allocate memory

		(void) ustrncpy(rebuilt_path, dir, length);		//Copy the string, including the file separator at the end, so its truncation can be verified while debugging
		(void) usetat(rebuilt_path, length - 1, '\0');	//Terminate the string, removing the trailing file separator
		path = rebuilt_path;	//Use this new string as the path to verify
	}

	if(!file_exists(path, FA_ALL, &aret))
		match = 0;	//File path does not exist

	if(!(aret & FA_DIREC))
		match = 0;	//The matching file path is not a directory

	if(rebuilt_path)			//If a string was allocated to rebuild the input path
		free(rebuilt_path);		//Free it now

	return match;
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

void *eof_buffer_file(const char * fn, char appendnull, char discardbom)
{
// 	eof_log("eof_buffer_file() entered");

	void * data = NULL, * dest = NULL;
	unsigned char * cdata = NULL;
	PACKFILE * fp = NULL;
	size_t filesize, buffersize, bytes_read = 0;
	char bom_present = 0;

	if(fn == NULL)
		return NULL;
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tBuffering file:  \"%s\"", fn);
	eof_log(eof_log_string, 2);
	fp = pack_fopen(fn, "r");
	if(fp == NULL)
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tCannot open specified file \"%s\":  \"%s\"", fn, strerror(errno));	//Get the Operating System's reason for the failure
		eof_log(eof_log_string, 1);
		return NULL;
	}
	filesize = buffersize = file_size_ex(fn);
	if(appendnull)
	{	//If adding an extra NULL byte of padding to the end of the buffer
		buffersize++;	//Allocate an extra byte for the padding
	}
	data = dest = malloc(buffersize);
	cdata = (unsigned char *)data;
	if(data == NULL)
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tCannot allocate %lu bytes of memory.", (unsigned long)buffersize);
		eof_log(eof_log_string, 1);
		(void) pack_fclose(fp);
		return NULL;
	}

	//Byte order mark detection logic
	if(discardbom && (filesize > 2))
	{	//If the byte order mark is being checked for and this file is large enough to have it and at least one byte of data
		if(pack_fread(data, 2, fp) == 2)
		{	//If the first two bytes of the file could be read
			bytes_read = 2;
			if((cdata[0] == 0xFE) && (cdata[1] == 0xFF))
			{
				bom_present = 1;	//UTF-16BE
			}
			else if((cdata[0] == 0xFF) && (cdata[1] == 0xFE))
			{
				bom_present = 2;	//UTF-16LE
			}
			else if((cdata[0] == 0xEF) && (cdata[1] == 0xBB))
			{	//The first two bytes of the three byte UTF-8 BOM
				if(pack_fread(cdata+2, 1, fp) == 1)
				{	//If the third byte of the file could be read
					bytes_read++;
					if(cdata[2] == 0xBF)
					{
						bom_present = 1;	//UTF-8
					}
				}
			}
		}
		if(bom_present)
		{	//If the byte order mark was identified, do not include it in the buffered data
			eof_log("\t\tDiscarding BOM", 1);
			cdata[0] = cdata[1] = cdata[2] = 0;	//The buffer is at least 3 bytes large if a BOM was detected, overwrite the BOM with zeroes
			dest = data;	//The rest of the data will be read into the beginning of the data buffer
		}
		else
		{	//There was no BOM, any bytes that were read will be kept in the data buffer
			dest = cdata + bytes_read;	//Advance the destination pointer by the number of bytes being kept
		}
	}

	(void) pack_fread(dest, (long)filesize - bytes_read, fp);	//Read the remainder of the file, minus however many bytes were discarded for the byte order mark
	if(appendnull)
	{	//If adding an extra NULL byte of padding to the end of the buffer
		((char *)data)[buffersize - 1 - bytes_read] = 0;	//Write a 0 byte at the end of the buffer, also accounting for how many bytes with discarded for the byte order mark
	}
	(void) pack_fclose(fp);
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tFile buffered to address 0x%08lX", (unsigned long)data);
	eof_log(eof_log_string, 2);
	return data;
}

int eof_copy_file(const char * src, const char * dest)
{
	PACKFILE * src_fp = NULL;
	PACKFILE * dest_fp = NULL;
	void *ptr = NULL;	//Used to buffer memory
	unsigned long src_size = 0;
	unsigned long i;
	long long_src_size;

 	eof_log("eof_copy_file() entered", 1);

	if((src == NULL) || (dest == NULL))
		return 0;
	if(!ustricmp(src,dest))		//Special case:  The input and output file are the same
		return 0;				//Return success without copying any files

	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tCopying file \"%s\" to \"%s\"", src, dest);
	eof_log(eof_log_string, 1);

	src_size = (unsigned long)file_size_ex(src);
	long_src_size = src_size;
	if(src_size > LONG_MAX)
		return 0;	//Unable to validate I/O due to Allegro's usage of signed values
	src_fp = pack_fopen(src, "r");
	if(!src_fp)
	{
		eof_log("\tFailed to open source file for reading", 1);
		return 0;
	}
	dest_fp = pack_fopen(dest, "w");
	if(!dest_fp)
	{
		eof_log("\tFailed to open destination file for writing", 1);
		(void) pack_fclose(src_fp);
		return 0;
	}
//Attempt to buffer the input file into memory for faster read and write
	ptr = malloc((size_t)src_size);
	if(ptr != NULL)
	{	//If a buffer large enough to store the input file was created
		long bytes_read, bytes_written;
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tBuffer copying %ld bytes", long_src_size);
		eof_log(eof_log_string, 1);
		bytes_read = pack_fread(ptr, long_src_size, src_fp);
		if(bytes_read != long_src_size)
		{
			eof_log("\t\tError reading to memory", 1);
			free(ptr);	//Release buffer
			return 0;	//Return error
		}
		bytes_written = pack_fwrite(ptr, long_src_size, dest_fp);
		free(ptr);	//Release buffer
		if(bytes_written != long_src_size)
		{
			eof_log("\t\tError writing to disk", 1);
			return 0;	//Return error
		}
	}
	else
	{	//Otherwise copy the slow way (one byte at a time)
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tByte by byte copying %ld bytes", long_src_size);
		eof_log(eof_log_string, 1);
		for(i = 0; i < src_size; i++)
		{
			int byte = pack_getc(src_fp);
			if(byte == EOF)
			{
				eof_log("\t\tError reading source file", 1);
				free(ptr);	//Release buffer
				return 0;	//Return error
			}
			if(pack_putc(byte, dest_fp) == EOF)
			{
				eof_log("\t\tError writing destination file", 1);
				free(ptr);	//Release buffer
				return 0;	//Return error
			}
		}
	}
	(void) pack_fclose(src_fp);
	(void) pack_fclose(dest_fp);
	eof_log("\tCopy successful", 1);
	return 1;
}

int eof_conditionally_copy_file(const char * src, const char * dest)
{
	if(!src || !dest)
		return 0;	//Invalid parameters

	eof_log("eof_conditionally_copy_file() entered", 1);
	if(file_size_ex(src) == file_size_ex(dest))
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tSkipping overwrite of file \"%s\" with source file of same size.", dest);
		eof_log(eof_log_string, 1);
		return 2;	//Copy operation skipped
	}

	return eof_copy_file(src, dest);
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

void eof_build_sanitized_filename_string(char *input, char *output)
{
	char *forbidden = "\\/:*?\"<>|";	//These are the characters that cannot be used in filenames in Windows
	unsigned long ctr, index = 0;

	if(!input || !output || (input == output))
		return;	//Invalid parameters

	output[0] = '\0';	//Terminate the output string

	//Filter out invalid characters
	for(ctr = 0; input[ctr] != '\0'; ctr++)
	{	//For each character in the input string
		int character = ugetat(input, ctr);
		if(ustrchr(forbidden, character) == NULL)
		{	//If this character is not one of the ones unable to be used in a filename
			uinsert(output, index++, character);	//Append it to the output string
		}
	}
	uinsert(output, index, '\0');	//Terminate the output string

	//Remove trailing whitespace, which are not valid in folder or file names
	while(index > 1)
	{	//While there's at least one character in the string
		if(isspace(ugetat(output, index - 1)))
		{	//If the last character in the string is any whitespace character
			uinsert(output, index - 1, '\0');	//Truncate it off the output string
			index--;
		}
		else
			break;
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

PACKFILE *eof_pack_fopen_retry(const char *filename, const char *mode, unsigned count)
{
	PACKFILE *fp;

	if(!filename || !mode)
		return NULL;	//Invalid parameters

	fp = pack_fopen(filename, mode);
	while(fp == NULL)
	{
		count--;
		if(!count)
		{	//If the number of retries have been exhausted
			return NULL;
		}
		Idle(1);	//Brief wait before retry
		fp = pack_fopen(filename, mode);
	}

	return fp;
}

int eof_number_is_power_of_two(unsigned long value)
{
	unsigned long mask, count;

	for(count = 0, mask = 1; count < 32; count++, mask <<= 1)
	{
		if(value == mask)
			return 1;
	}

	return 0;
}

int eof_parse_last_folder_name(const char *filename, char *buffer, unsigned long bufferlen)
{
	unsigned long index1 = 0, index2 = 0;	//The array indexes of the beginning and end of the folder name to be parsed, based on the presence of folder separators
	unsigned long stringlen, ctr;
	int thischar;

	if(!filename || !buffer || !bufferlen)
		return 0;	//Invalid parameters

	//Find the location of the last folder separator in the filename
	stringlen = ustrlen(filename);
	if(stringlen < 3)
		return 0;	//Error, at least one folder name, one separator and one filename is required as input
	for(ctr = stringlen; ctr > 0; ctr --)
	{	//For each character in the filename starting from the end and working backward
		thischar = ugetat(filename, ctr - 1);	//Examine the character
		if((thischar == '/') || (thischar == '\\'))
		{	//If this is a forward or backward slash, consider it a folder separator
			if(ctr < 2)		//There is no character preceding this separator
				return 0;	//Error
			index2 = ctr - 2;	//This is the index marking the end of the folder name
			ctr--;	//Back up one character to prepare for the next loop
			break;
		}
	}

	//Find the location of the folder separator preceding the last separator in the filename, if any
	//If no separator is found to precede the folder name begin parsed, index1 remains initialized at 0 to reflect the beginning of the input string
	for(; ctr > 0; ctr--)
	{	//For each remaining character in the filename until the characters have run out
		thischar = ugetat(filename, ctr - 1);	//Examine the character
		if((thischar == '/') || (thischar == '\\'))
		{	//If this is a forward or backward slash, consider it a folder separator
			index1 = ctr;	//The character after this separator is considered the beginning of the target file name
			break;
		}
	}
	thischar = ugetat(filename, index1);
	if((thischar == '/') || (thischar == '\\'))
	{	//If the parsed folder name is a separator, the input was valid, ie. had consecutive folder separators
		return 0;	//Error
	}

	//Validate the length of the output buffer and write the folder name to it
	if(index2 - index1 + 1 >= bufferlen)
	{	//If the output buffer isn't large enough to store the folder name
		return 0;	//Error
	}
	for(ctr = 0; index1 + ctr <= index2; ctr++)
	{	//For each character from the first to the last in the identified folder name
		usetat(buffer, ctr, ugetat(filename, index1 + ctr));	//Copy it to the output buffer
	}
	usetat(buffer, ctr, '\0');	//Terminate the string

	return 1;	//Success
}

int eof_byte_to_binary_string(unsigned char value, char *buffer)
{
	unsigned long index, ctr, bitmask;

	if(!buffer)
		return -1;	//Return error

	for(ctr = 0, index = 0, bitmask = 0x80; ctr < 8; ctr++, bitmask >>= 1)
	{	//For each bit in the 8 bit value, from most significant to least significant
		if(value & bitmask)
		{	//If this bit is set
			buffer[index++] = '1';	//Append a 1 to the string
		}
		else if(index)
		{	//If the bit isn't set, but a previous bit was
			buffer[index++] = '0';	//Append a 0 to the string
		}
	}
	if(!index)
	{	//If no bits in value were set
		buffer[index++] = '0';	//Append a 0 to the string
	}
	buffer[index] = '\0';	//Terminate the string
	return 0;	//Return success
}

void eof_check_and_log_lyric_line_errors(EOF_SONG *sp, char force)
{
	unsigned long ctr, ctr2;
	EOF_VOCAL_TRACK *tp;
	char buffer[1024], buffer2[50] = {0}, error = 0, unsorted = 0;
	unsigned long lastend = 0, lastlyricindex, matches;

	if(!sp)
		return;

	tp = sp->vocal_track[0];
	for(ctr = 0; ctr < tp->lines; ctr++)
	{	//For each lyric line
		if(tp->line[ctr].start_pos >= tp->line[ctr].end_pos)
		{
			error = 1;
			break;
		}
		if(ctr && (tp->line[ctr].start_pos <= lastend))
		{
			error = 2;
			break;
		}
		if(tp->line[ctr].difficulty != 255)
		{
			error = 3;
			break;
		}
		matches = lastlyricindex = 0;	//Reset these
		for(ctr2 = 0; ctr2 < tp->lyrics; ctr2++)
		{	//For each lyric
			if((tp->lyric[ctr2]->pos >= tp->line[ctr].start_pos) && (tp->lyric[ctr2]->pos <= tp->line[ctr].end_pos))
			{	//If this lyric is within the scope of the lyric line
				if(matches && (ctr2 > lastlyricindex + 1))
				{
					error = 4;
					break;
				}
				matches++;	//Keep track of how many lyrics were found to be in this line
				lastlyricindex = ctr2;	//Keep track of the last lyric index found to be in this line
			}
		}
		lastend = tp->line[ctr].end_pos;
		if(!matches)
		{	//No lyrics were in this line
			error = 5;
		}
		if(error)
			break;
	}

	if(error || force)
	{	//If any errors were found, or the calling function is forcing the lyrics to be written to log
		if(error)
			eof_log("\tLyric errors found", 1);
		else
			eof_log("\tLyric dump", 1);

		for(ctr = 0; ctr < tp->lines;)
		{	//For each lyric line
			if(tp->line[ctr].start_pos >= tp->line[ctr].end_pos)
			{
				eof_log("\t\t!Lyric line timings corrupt.  Repairing", 1);
				if(tp->line[ctr].start_pos >= tp->line[ctr].end_pos)
					tp->line[ctr].end_pos++;		//If the start and end positions are the same, make the line 1ms longer to resolve the problem
				else
				{	//The line's start position is after the end position, swap those timings
					unsigned long temp = tp->line[ctr].end_pos;
					tp->line[ctr].end_pos = tp->line[ctr].start_pos;
					tp->line[ctr].start_pos = temp;
				}
			}
			if(ctr && (tp->line[ctr].start_pos <= lastend))
			{
				eof_log("\t\t!Lyric line overlaps previous line.  Repairing", 1);
				tp->line[ctr].start_pos = lastend + 1;	//Make the lyric line begin 1ms after the end of the previous line
			}
			if(tp->line[ctr].difficulty != 255)
			{
				eof_log("\t\t!Lyric line is the wrong difficulty.  Repairing", 1);
				tp->line[ctr].difficulty = 255;
			}
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tLine #%lu:  Diff %u, %lums to %lums contains lyric numbers:", ctr, tp->line[ctr].difficulty, tp->line[ctr].start_pos, tp->line[ctr].end_pos);
			eof_log(eof_log_string, 1);
			buffer[0] = '\0';	//Empty the buffer
			matches = lastlyricindex = 0;	//Reset these
			for(ctr2 = 0; ctr2 < tp->lyrics; ctr2++)
			{	//For each lyric
				if((tp->lyric[ctr2]->pos >= tp->line[ctr].start_pos) && (tp->lyric[ctr2]->pos <= tp->line[ctr].end_pos))
				{	//If this lyric is within the scope of the lyric line
					if(matches && (ctr2 > lastlyricindex + 1))
					{
						eof_log("\t\t\t!Lyrics out of order.", 1);
						unsorted = 1;
					}
					snprintf(buffer2, sizeof(buffer2) - 1, "%lu (%s) ", ctr2, tp->lyric[ctr2]->text);
					strncat(buffer, buffer2, sizeof(buffer) - 1);	//Append data about this lyric to the buffer
					matches++;	//Keep track of how many lyrics were found to be in this line
					lastlyricindex = ctr2;	//Keep track of the last lyric index found to be in this line
				}
			}
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t%s", buffer);
			eof_log(eof_log_string, 1);

			lastend = tp->line[ctr].end_pos;
			if(!matches)
			{	//No lyrics were in this line, do not increment ctr and instead process the next line which will now become line #ctr
				eof_log("\t\t!Lyric line is empty.  Repairing", 1);
				eof_vocal_track_delete_line(tp, ctr);		//Delete the empty line
			}
			else
				ctr++;	//Process the next lyric line on the next loop
		}

		if(unsorted)
		{	//If the lyrics were out of chronological order
			eof_log("\t\t\tRepairing.", 1);
			eof_track_sort_notes(sp, EOF_TRACK_VOCALS);
		}

		if(error)
			allegro_message("Lyric error detected (%u) and repaired, check EOF log.", error);
	}
	else
		eof_log("\tLyrics OK", 1);
}

int eof_get_clipboard(void)
{
	eof_log("eof_get_clipboard() entered", 1);
	if(eof_validate_temp_folder())
	{	//Ensure the correct working directory and presence of the temporary folder
		eof_log("\tCould not validate working directory and temp folder", 1);
		eof_log_cwd();
		return -1;
	}

	#ifdef ALLEGRO_WINDOWS
	//Since logic to read the Windows clipboard has been added, recreate os_clipboard.txt
		(void) delete_file("os_clipboard.txt");
		if(exists("os_clipboard.txt"))
		{
			eof_log("!Failed to delete os_clipboard.txt", 1);
			return -1;
		}
///		(void) eof_system("powershell -WindowStyle Minimized -command \"Get-Clipboard\" > os_clipboard.txt");	//The command below is better
		(void) eof_system("powershell -WindowStyle Minimized -command \"Get-Clipboard | Out-File -Encoding utf8 -NoNewline -FilePath \"os_clipboard.txt\"\"");
		if(!exists("os_clipboard.txt"))
		{
			eof_log("!Failed to create os_clipboard.txt", 1);
			return -1;
		}
	#else
	//Otherwise for other Operating Systems, use os_clipboard.txt as a clipboard that is internal to EOF
		if(!exists("os_clipboard.txt"))
		{
			eof_log("!Failed to find os_clipboard.txt", 1);
			return -1;
		}
	#endif

	//Buffer the contents of os_clipboard.txt into eof_os_clipboard[]
	if(eof_os_clipboard)
		free(eof_os_clipboard);	//Release any previous clipboard buffer
	eof_os_clipboard = eof_buffer_file("os_clipboard.txt", 1, 1);	//Read the clipboard file to memory, discard the BOM if present and append a NULL terminator
	if(!eof_os_clipboard)
	{
		eof_log("!Failed to buffer os_clipboard.txt to memory", 1);
		return -1;
	}
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tRead from OS clipboard:  \"%s\"", eof_os_clipboard);
	eof_log(eof_log_string, 2);
	if(eof_gas_clipboard)
		play_sample(eof_sound_gas3, 255.0 * (eof_tone_volume / 100.0), 127, 1000 + eof_audio_fine_tune, 0);	//Play this sound clip upon success, if enabled

	return 0;		//Success
}

int eof_set_clipboard(char *text)
{
	int retval = 0;
	PACKFILE * fp;

	if(!text)
		return -1;	//Abort if there is no text

	eof_log("eof_set_clipboard() entered", 1);
	if(eof_validate_temp_folder())
	{	//Ensure the correct working directory and presence of the temporary folder
		eof_log("\tCould not validate working directory and temp folder", 1);
		eof_log_cwd();
		return -1;
	}

	(void) delete_file("os_clipboard.txt");
	if(exists("os_clipboard.txt"))
	{
		eof_log("!Failed to delete os_clipboard.txt", 1);
		return -1;
	}
	fp = pack_fopen("os_clipboard.txt", "w");	//Try to open the file for writing
	if(fp)
	{
		(void) pack_fputs(text, fp);	//Write specified text to the file
		(void) pack_fclose(fp);
	}
	if(!exists("os_clipboard.txt"))
	{
		eof_log("!Failed to create os_clipboard.txt", 1);
		return -1;
	}

	#ifdef ALLEGRO_WINDOWS
	//For Windows, use a native command line utility to populate the clipboard
		char syscommand[1024];

///		(void) snprintf(syscommand, sizeof(syscommand) - 1, "echo %s|clip", text);	//This method has problems with special characters because they would need to escaped, feed the text file into clip.exe instead
		(void) snprintf(syscommand, sizeof(syscommand) - 1, "clip < os_clipboard.txt");
		retval = eof_system(syscommand);
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tWrote to OS clipboard:  \"%s\"", text);
		eof_log(eof_log_string, 2);
		if(!retval && eof_gas_clipboard)
			play_sample(eof_sound_gas1, 255.0 * (eof_tone_volume / 100.0), 127, 1000 + eof_audio_fine_tune, 0);	//Play this sound clip upon success, if enabled
		return retval;
	#else
	//Otherwise for other Operating Systems, just use os_clipboard.txt as a clipboard that is internal to EOF
		return retval;
	#endif
}

unsigned long eof_pack_read_vlv(PACKFILE *fp, unsigned long *byte_counter)
{
	unsigned long value = 0;
	int byte;

	if(!fp)
		return ULONG_MAX;

	byte = pack_getc(fp);
	if(byte_counter)
		*byte_counter = *byte_counter + 1;
	if(byte == EOF)
		return ULONG_MAX;
	value = byte & 0x7F;

	while(byte & 0x80)
	{
		value <<= 7;
		byte = pack_getc(fp);
		if(byte_counter)
			*byte_counter = *byte_counter + 1;
		if(byte == EOF)
			return ULONG_MAX;
		value += (byte & 0x7F);
	}

	return value;
}

int eof_packfile_search_phrase(PACKFILE *inf, const char *phrase, unsigned long phraselen, unsigned long *bytes_read)
{
	unsigned char c = 0;
	int ret;
	unsigned long ctr = 0;		//Used to index into the phrase[] array, beginning with the first character
	unsigned char success = 0;
	unsigned long read_count = 0;

//Validate input
	if(!inf || !phrase)	//These input parameters are not allowed to be NULL
		return -1;

	if(!phraselen)
		return 0;	//There will be no matches to an empty search array

//Initialize for search
	errno=0;
	ret = pack_getc(inf);		//Read the first character of the file
	read_count++;
	if(ret == EOF)
		return -1;
	c = ret;

//Perform search
	while(!pack_feof(inf))	//While end of file hasn't been reached
	{
	//Check if the next character in the search phrase has been matched
		if(c == phrase[ctr])
		{	//The next character was matched
			ctr++;	//Advance to the next character in search array
			if(ctr == phraselen)
			{	//If all characters have been matched
				success=1;
				break;
			}
		}
		else	//Character did not match
			ctr=0;			//Ensure that the first character in the search array is looked for

		ret = pack_getc(inf);	//Read the next character of the file
		read_count++;
		if(ret == EOF)
			return -1;
		c = ret;
	}

	if(pack_ferror(inf))		//If there was an I/O error
		return -1;

	if(bytes_read)
		*bytes_read = read_count;	//If the calling function wanted to know how many bytes were read, return that information by reference

//Return match/non match
	if(success)
	{
		return 1;	//Return match
	}

	return 0;	//Return no match
}

unsigned long eof_find_wav_metadata(char *filename, EOF_AUDIO_METADATA *metadata, unsigned long metadata_count)
{
	PACKFILE *fp = NULL;
	unsigned long found = 0;

	if(!filename || !metadata || !metadata_count)
		return 0;	//Invalid parameters

	eof_log("eof_find_wav_metadata() entered", 1);
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tParsing for metadata from \"%s\"", filename);
	eof_log(eof_log_string, 1);
	fp = pack_fopen(filename, "r");
	if(!fp)
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError loading:  Cannot open input .wav file:  \"%s\"", strerror(errno));	//Get the Operating System's reason for the failure
		eof_log(eof_log_string, 1);
		return 0;
	}

	while(eof_packfile_search_phrase(fp, "LIST", 4, NULL) > 0)
	{	//For each LIST chunk that is found in the file
		unsigned long listchunksize = pack_igetl(fp);	//Read the LIST chunk size
		unsigned long listchunkpos = 0;				//Used to keep track of the file position within the LIST chunk

		if(pack_ferror(fp))
			return 0;		//If the LIST chunk size was not read, return error

		while(listchunkpos < listchunksize)
		{	//Until all content in this LIST chunk has been parsed
			unsigned long bytes_read = 0;

			if((eof_packfile_search_phrase(fp, "INFO", 4, &bytes_read) > 0) && (listchunkpos + bytes_read < listchunksize))
			{	//If an INFO sub chunk header was read, and it is within the scope of the current LIST chunk
				listchunkpos += bytes_read;	//Update this position

				while(listchunkpos < listchunksize)
				{	//Until all content in this LIST chunk has been parsed
					char text_id[5] = {0};
					unsigned long text_size, ctr;
					char *text;

					//Skip any variable amount of padding that occurs before the next piece of metadata
					do{
						text_id[0] = pack_getc(fp);
						listchunkpos++;	//Update this position
						if(text_id[0] == EOF)
						{	//If the byte was not read
							listchunkpos = ULONG_MAX;		//Set a condition to end file parsing
							break;
						}
					}while(text_id[0] == 0);
					if(listchunkpos == ULONG_MAX)
						break;

					//Read metadata ID
					if(pack_fread(&text_id[1], 3, fp) != 3)
					{	//If the rest of the text identifier was not read
						listchunkpos = ULONG_MAX;		//Set a condition to end file parsing
						break;
					}
					listchunkpos += 3;	//Update this position
					if(listchunkpos >= listchunksize)
					{	//If the end of the LIST chunk has been reached
						break;
					}
					text_size = pack_igetl(fp);	//Read the number of bytes this metadata item contains, including NULL terminator
					if(text_size < 2)
						return 0;	//The string must be long enough for at least one character and a NULL terminator, otherwise it's invalid
					listchunkpos += 4;	//Update this position

					//Read metadata value
					if(text_size > 1000)
					{
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tSkipping large metadata field:  %s (%lu bytes)", text_id, text_size);
						eof_log(eof_log_string, 1);
						pack_fseek(fp, text_size);
						listchunkpos += text_size;	//Update this position
						if(pack_ferror(fp))
							return 0;			//Return error if there was an I/O error
					}
					else
					{
						text = malloc(text_size);
						if(!text)
							return 0;	//If memory couldn't be allocated, return error
						if(pack_fread(text, text_size, fp) != text_size)
						{	//If the text was not read
							listchunkpos = ULONG_MAX;		//Set a condition to end file parsing
							break;
						}
						listchunkpos += text_size;	//Update this position
						text[text_size - 1] = '\0';	//Guarantee this string is terminated
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tFound metadata:  %s = \"%s\"", text_id, text);
						eof_log(eof_log_string, 1);

						//Store metadata value if it's wanted by the calling function
						for(ctr = 0; ctr < metadata_count; ctr++)
						{	//For each piece of metadata sought by the calling function
							if(!stricmp(metadata[ctr].ID, text_id))
							{	//If the one that was just read is sought
								if(metadata[ctr].value_array_size < 2)
								{	//The buffer isn't large enough to store any valid data
									free(text);
									return 0;	//Return error
								}
								strncpy(metadata[ctr].value, text, metadata[ctr].value_array_size - 1);	//Copy the metadata text, truncate if necessary
								metadata[ctr].value[metadata[ctr].value_array_size - 1] = '\0';	//Guarantee this string is terminated
								found++;		//Count how many sought pieces of metadata were found
								break;
							}
						}
						free(text);	//Release the metadata text buffer
					}
				}//Until all content in this LIST chunk has been parsed
			}//If an INFO sub chunk header was read, and it is within the scope of the current LIST chunk
		}//Until all content in this LIST chunk has been parsed
	}//For each LIST chunk that is found in the file

	(void) pack_fclose(fp);
	return found;
}

int eof_string_is_zero(char *string)
{
	unsigned long ctr;
	char decimalparsed = 0;

	if(!string)
		return 0;

	for(ctr = 0; string[ctr] != '\0'; ctr++)
	{
		if((string[ctr] != '0') && (string[ctr] != '.'))
			return 0;	//If there are any characters other than 0 or a decimal point, this is not a valid representation of the number 0
		if(string[ctr] == '.')
		{
			if(decimalparsed)
				return 0;	//A decimal point occurring twice is not valid
			else
				if(ctr == 0)
					return 0;	//A digit must occur before a decimal point
				else
					decimalparsed = 1;
		}
		else
			if(decimalparsed == 1)
				decimalparsed = 2;	//Track whether digits were read after a decimal point
	}
	if(ctr == 0)
		return 0;	//No digits were read
	if(decimalparsed == 1)
		return 0;	//A decimal point followed by no digits is not valid

	return 1;	//At least one zero and optionally a decimal point and one or more zeros were read
}

void eof_build_mmssms_string(unsigned long mspos, unsigned *min, unsigned *sec, unsigned *ms, char *string)
{
	unsigned ism, iss, isms;

	ism = (mspos / 1000) / 60;
	iss = (mspos / 1000) % 60;
	isms = (mspos % 1000);

	if(min)
		*min = ism;	//If the calling function wanted to store the minutes, do so
	if(sec)
		*sec = iss;	//If the calling function wanted to store the seconds, do so
	if(ms)
		*ms = isms;	//If the calling function wanted to store the milliseconds, do so

	if(string)
	{	//If the calling function wants the formatted string
		if(ism > 99)
		{	//The timestamp is too large to display with only two digits
			(void) sprintf(string, ">=100min");
		}
		else
		{
			(void) sprintf(string, "%02u:%02u.%03u", ism, iss, isms);
		}
	}
}

int eof_ffmpeg_convert_audio(char *input, char *output)
{
	int retval;
	char tempstr[1024] = {0};
	char syscommand[1024] = {0};

	eof_log("eof_ffmpeg_convert_audio() entered", 1);

	//Ensure FFMPEG is linked
	if(!exists(eof_ffmpeg_executable_path))
	{	//If the path to FFMPEG is not defined or otherwise doesn't refer to a valid file
		retval = alert3("FFMPEG is not properly linked", "First download the program", "Then use \"File>Link to FFMPEG\" to browse to the program", "OK", "Download", "Link", 0, 0, 0);
		if(retval == 2)
		{	//If the user opted to download the program
			(void) eof_system("start https://www.ffmpeg.org/download.html");
		}
		else if(retval == 3)
		{	//If the user opted to link the program
			(void) eof_menu_file_link_ffmpeg();
		}
		if(!exists(eof_ffmpeg_executable_path))
			return 1;	//If FFMPEG is still not linked, return failure
	}

	//Validate parameters
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tInput file:  %s", input);
	eof_log(eof_log_string, 1);
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tOutput file:  %s", output);
	eof_log(eof_log_string, 1);
	if(!exists(input))
	{
		eof_log("\t\tInput file does not exist", 1);
		return 2;	//Return failure
	}
	if(exists(output))
	{
		eof_log("\t\tOutput file will be overwritten", 1);
		(void) delete_file(output);
		if(exists(output))
		{
			eof_log("\t\tFailed to delete existing output file", 1);
			return 3;	//Return failure
		}
	}

	//Call program
	(void) ustrcpy(syscommand, eof_ffmpeg_executable_path);
	(void) uszprintf(tempstr, (int) sizeof(tempstr) - 1, " -i \"%s\" -y \"%s\"", input, output);
	(void) ustrcat(syscommand, tempstr);
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tCalling FFMPEG as follows:  %s", syscommand);
	eof_log(eof_log_string, 1);
	(void) eof_system(syscommand);

	if(exists(output))
		return 0;	//The output file was written.  Return success

	return 4;		//Return failure
}
