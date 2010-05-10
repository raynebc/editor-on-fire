#ifndef WFSEL_H
#define WFSEL_H

#define NCDFS_MAX_FILTERS 32

typedef struct
{
	
	char extension[256];
	char description[256];
	
} NCDFS_FILTER;

typedef struct
{
	
	NCDFS_FILTER filter[NCDFS_MAX_FILTERS];
	int filters;
	int default_filter;
	
} NCDFS_FILTER_LIST;

extern int  ncdfs_use_allegro;

NCDFS_FILTER_LIST * ncdfs_filter_list_create(void);
int ncdfs_filter_list_add(NCDFS_FILTER_LIST * lp, char * ext, char * desc, int df);
char * ncd_file_select(int type, char * initial, const char * title, NCDFS_FILTER_LIST * lp);
char * ncd_folder_select(char * title);

#endif
