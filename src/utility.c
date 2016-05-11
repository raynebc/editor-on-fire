#include <allegro.h>
#include <sys/stat.h>
#include "utility.h"
#include "main.h"	//For logging

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
	fp = pack_fopen(fn, "r");
	if(fp == NULL)
	{
		return NULL;
	}
	filesize = buffersize = file_size_ex(fn);
	if(appendnull)
	{	//If adding an extra NULL byte of padding to the end of the buffer
		buffersize++;	//Allocate an extra byte for the padding
	}
	data = (char *)malloc(buffersize);
	if(data == NULL)
		return NULL;

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
			for(i = 0; i < 256; i++)
			{
				eof_ucode_table[i] = i;
			}
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
int eof_convert_extended_ascii(char * buffer, int size)
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
	do_uconvert(workbuffer, U_ASCII_CP, buffer, U_CURRENT, size);
	free(workbuffer);
	return 1;
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
