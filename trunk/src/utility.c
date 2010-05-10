#include <allegro.h>
#include <sys/stat.h>
#include "utility.h"

int eof_chdir(const char * dir)
{
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

	fp = pack_fopen(fn, "r");
	if(fp == NULL)
	{
		return NULL;
	}
	data = (char *)malloc(file_size_ex(fn));
	pack_fread(data, file_size_ex(fn), fp);
	pack_fclose(fp);
	return data;
}

int eof_copy_file(char * src, char * dest)
{
	PACKFILE * src_fp;
	PACKFILE * dest_fp;
	unsigned long src_size = 0;
	int i;
	
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
	for(i = 0; i < src_size; i++)
	{
		pack_putc(pack_getc(src_fp), dest_fp);
	}
	pack_fclose(src_fp);
	pack_fclose(dest_fp);
	return 1;
}

int eof_check_string(char * tp)
{
	int i;
	
	for(i = 0; i < ustrlen(tp); i++)
	{
		if(tp[i] != ' ')
		{
			return 1;
		}
	}
	return 0;
}
