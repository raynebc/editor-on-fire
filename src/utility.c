#include <allegro.h>
#include <sys/stat.h>
#include "utility.h"
#include "main.h"	//For logging

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

int eof_chdir(const char * dir)
{
	if(dir == NULL)
		return -1;
	#ifdef ALLEGRO_WINDOWS
		wchar_t wdir[1024] = {0};
		uconvert(dir, U_UTF8, (char *)(&wdir[0]), U_UNICODE, 2048);
		return _wchdir(wdir);
	#else
		return chdir(dir);
	#endif
	return -1;
}

int eof_mkdir(const char * dir)
{
	if(dir == NULL)
		return -1;
	#ifdef ALLEGRO_WINDOWS
		wchar_t wdir[1024] = {0};
		uconvert(dir, U_UTF8, (char *)(&wdir[0]), U_UNICODE, 2048);
		return _wmkdir(wdir);
	#else
		return mkdir(dir, 0777);
	#endif
	return -1;
}

int eof_system(const char * command)
{
	if(command == NULL)
		return -1;
	#ifdef ALLEGRO_WINDOWS
		wchar_t wcommand[1024] = {0};
		uconvert(command, U_UTF8, (char *)(&wcommand[0]), U_UNICODE, 2048);
		return _wsystem(wcommand);
	#else
		return system(command);
	#endif
	return -1;
}

void * eof_buffer_file(char * fn)
{
// 	eof_log("eof_buffer_file() entered");

	void * data = NULL;
	PACKFILE * fp = NULL;

	if(fn == NULL)
		return NULL;
	fp = pack_fopen(fn, "r");
	if(fp == NULL)
	{
		return NULL;
	}
	data = (char *)malloc(file_size_ex(fn));
	if(data == NULL)
		return NULL;

	pack_fread(data, file_size_ex(fn), fp);
	pack_fclose(fp);
	return data;
}

int eof_copy_file(char * src, char * dest)
{
 	eof_log("eof_copy_file() entered", 1);

	PACKFILE * src_fp = NULL;
	PACKFILE * dest_fp = NULL;
	void *ptr = NULL;	//Used to buffer memory
	unsigned long src_size = 0;
	int i;

	if((src == NULL) || (dest == NULL))
		return 0;
	src_size = file_size_ex(src);
	src_fp = pack_fopen(src, "r");
	if(!src_fp)
	{
		return 0;
	}
	dest_fp = pack_fopen(dest, "w");
	if(!dest_fp)
	{
		pack_fclose(src_fp);
		return 0;
	}
//Attempt to buffer the input file into memory for faster read and write
	ptr=malloc(src_size);
	if(ptr != NULL)
	{	//If a buffer large enough to store the input file was created
		if((pack_fread(ptr, src_size, src_fp) != src_size) || (pack_fwrite(ptr, src_size, dest_fp) != src_size))
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
			pack_putc(pack_getc(src_fp), dest_fp);
		}
	}
	pack_fclose(src_fp);
	pack_fclose(dest_fp);
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
 	eof_log("eof_file_compare() entered", 1);

	uint64_t filesize,ctr;
	int data1,data2;
	PACKFILE *fp1 = NULL,*fp2 = NULL;
	char result = 0;	//The result is assumed to "files identical" until found otherwise

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
		pack_fclose(fp1);
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
	pack_fclose(fp1);
	pack_fclose(fp2);

	return result;
}
