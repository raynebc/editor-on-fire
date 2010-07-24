#include <allegro.h>
#include <sys/stat.h>
#include "utility.h"

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
	void * data;
	PACKFILE * fp;

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
	PACKFILE * src_fp;
	PACKFILE * dest_fp;
	void *ptr;	//Used to buffer memory
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

//	for(i = 0; i < ustrlen(tp); i++)
	for(i = 0; tp[i] != '\0'; i++)
	{	//For each character in the string until the NULL terminator is reached
		if(tp[i] != ' ')
		{
			return 1;
		}
	}
	return 0;
}
