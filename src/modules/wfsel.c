#include <allegro.h>
#ifdef ALLEGRO_WINDOWS
	#define UNICODE
	#include <winalleg.h>
	#include <shlobj.h>
#endif

#ifdef ALLEGRO_LEGACY
	#include <a5alleg.h>
	#include <allegro5/allegro_native_dialog.h>
#endif

#include "wfsel.h"

#ifdef USEMEMWATCH
#include "../memwatch.h"
#endif

char ncdfs_internal_return_path[4096] = {0};
char ncdfs_internal_return_folder[4096] = {0};

/* use Windows Unicode format then convert to UTF-8 */
#ifdef ALLEGRO_WINDOWS
	wchar_t ncdfs_internal_windows_return_path[4096];
	wchar_t ncdfs_internal_windows_return_folder[4096];
	wchar_t ncdfs_internal_windows_title[4096];
	wchar_t ncdfs_internal_windows_initial[4096];
	wchar_t ncdfs_internal_windows_filter[4096];
#endif

int  ncdfs_use_allegro = 0;

NCDFS_FILTER_LIST * ncdfs_filter_list_create(void)
{
	NCDFS_FILTER_LIST * flist;

	flist = malloc(sizeof(NCDFS_FILTER_LIST));
	if(flist)
	{
		flist->filters = 0;
	}
	return flist;
}

int ncdfs_filter_list_add(NCDFS_FILTER_LIST * lp, char * ext, char * desc, int df)
{
	ustrcpy(lp->filter[lp->filters].extension, ext);
	ustrcpy(lp->filter[lp->filters].description, desc);
	if(df)
	{
		lp->default_filter = lp->filters;
	}
	lp->filters++;
	return 1;
}

/*

	example filter string

	"Bitmaps\0*.bmp\0All Files\0*.*\0\0"

*/

static int ncd_file_select_allegro_want_all(NCDFS_FILTER_LIST * lp)
{
	int i;

	for(i = 0; i < lp->filters; i++)
	{
		if(!strcmp(lp->filter[i].extension, "*"))
		{
			return 1;
		}
	}
	return 0;
}

#ifdef ALLEGRO_LEGACY

	char * ncd_file_select_a5(int type, char * initial, const char * title, NCDFS_FILTER_LIST * lp)
	{
		ALLEGRO_FILECHOOSER * fcp = NULL;
		char pattern[1024] = {0};
		int i, c;
		int flags = 0;

		if(lp)
		{
			for(i = 0; i < lp->filters; i++)
			{
				if(i > 0)
				{
					strcat(pattern, ";");
				}
				strcat(pattern, "*.");
				strcat(pattern, lp->filter[i].extension);
			}
		}

		if(type == 0)
		{
			flags |= ALLEGRO_FILECHOOSER_FILE_MUST_EXIST;
		}
		else if(type == 1)
		{
			flags |= ALLEGRO_FILECHOOSER_SAVE;
		}
		else if(type == 2)
		{
			flags |= ALLEGRO_FILECHOOSER_FILE_MUST_EXIST;
			flags |= ALLEGRO_FILECHOOSER_FOLDER;
		}
		fcp = al_create_native_file_dialog(initial, title, pattern, flags);
		if(fcp)
		{
			al_show_native_file_dialog(all_get_display(), fcp);
			c = al_get_native_file_dialog_count(fcp);
			if(c > 0)
			{
				strcpy(ncdfs_internal_return_path, al_get_native_file_dialog_path(fcp, 0));
			}
			al_destroy_native_file_dialog(fcp);
			if(c > 0)
			{
				return ncdfs_internal_return_path;
			}
		}
		return NULL;
	}

#endif

char * ncd_file_select_allegro(int type, char * initial, const char * title, NCDFS_FILTER_LIST * lp)
{
	char realfilter[1024] = {0};
	int i, j, epos;
	char * filter;

	if(!initial)
	{
//		ustrcpy(ncdfs_internal_return_path, ncdfs_last_path);
	}
	else
	{
		ustrcpy(ncdfs_internal_return_path, initial);
	}

	/* build filter string */
	if(!lp)
	{
		filter = NULL;
	}
	else
	{
		if(ncd_file_select_allegro_want_all(lp))
		{
			filter = NULL;
		}
		else
		{
			ustrcpy(realfilter, "");
			epos = 0;
			for(i = 0; i < lp->filters; i++)
			{

				for(j = 0; j < ustrlen(lp->filter[i].extension); j++)
				{
					realfilter[epos] = lp->filter[i].extension[j];
					epos++;
				}

				if(i < lp->filters - 1)
				{
					realfilter[epos] = ';';
					epos++;
				}

				realfilter[epos] = '\0';
				epos++;
			}
			filter = realfilter;
		}
	}
	if(file_select_ex(title, ncdfs_internal_return_path, filter, 1024, 320, 240))
	{
		return ncdfs_internal_return_path;
	}
	return NULL;
}

char * ncd_file_select(int type, char * initial, const char * title, NCDFS_FILTER_LIST * lp)
{
	#ifdef ALLEGRO_LEGACY

		return ncd_file_select_a5(type, initial, title, lp);

	#else

		#ifdef ALLEGRO_WINDOWS

			char realfilter[1024] = {0}; // fill this buffer with U_UNICODE characters and cast to wchar_t *
			int i, j, l, epos, cl = 0;
			OPENFILENAME ofn;
			HWND allegro_window;
			memset(&ofn,0,sizeof(ofn));
			ofn.lStructSize=sizeof(OPENFILENAME);
			allegro_window = win_get_window();
			if(ncdfs_use_allegro)
			{
				return ncd_file_select_allegro(type, initial, title, lp);
			}

			if(!initial)
			{
				ustrcpy(ncdfs_internal_return_path, "");
			}
			else
			{
				ustrcpy(ncdfs_internal_return_path, initial);
			}

			/* build filter string */
			if(!lp)
			{
			}
			else
			{
				epos = 0;
				for(i = 0; i < lp->filters; i++)
				{
					set_uformat(U_UTF8);
					l = ustrlen(lp->filter[i].description); // need to get the length of the string in characters

					/* append U_UNICODE characters one at a time */
					for(j = 0; j < l; j++)
					{
						set_uformat(U_UNICODE);
						cl = usetc(&realfilter[epos], lp->filter[i].description[j]);
						epos += cl;
					}

					cl = usetc(&realfilter[epos], '\0');
					epos += cl;
					cl = usetc(&realfilter[epos], '*');
					epos += cl;
					cl = usetc(&realfilter[epos], '.');
					epos += cl;


					set_uformat(U_UTF8); // lp->filter fields are in U_UTF8
					l = ustrlen(lp->filter[i].extension);
					for(j = 0; j < l; j++)
					{
						set_uformat(U_UNICODE);
						if(lp->filter[i].extension[j] == ';')
						{
							cl = usetc(&realfilter[epos], lp->filter[i].extension[j]);
							epos += cl;
							cl = usetc(&realfilter[epos], '*');
							epos += cl;
							cl = usetc(&realfilter[epos], '.');
							epos += cl;
						}
						else
						{
							cl = usetc(&realfilter[epos], lp->filter[i].extension[j]);
							epos += cl;
						}
					}

					cl = usetc(&realfilter[epos], '\0');
					epos += cl;
				}
				cl = usetc(&realfilter[epos], '\0');
				epos += cl;
				set_uformat(U_UTF8); // go back to U_UTF8
			}
			ncdfs_internal_return_path[0] = 0;
			uconvert("", U_UTF8, (char *)(&ncdfs_internal_windows_return_path[0]), U_UNICODE, 4096);
			uconvert(title, U_UTF8, (char *)(&ncdfs_internal_windows_title[0]), U_UNICODE, 4096);
			uconvert(initial ? initial : "", U_UTF8, (char *)(&ncdfs_internal_windows_initial[0]), U_UNICODE, 4096);
	//		uconvert(realfilter ? realfilter : "", U_UTF8, (char *)(&ncdfs_internal_windows_filter[0]), U_UNICODE, 4096);

			ofn.lStructSize = sizeof(OPENFILENAME);
			ofn.hwndOwner = allegro_window;
			ofn.hInstance = 0;
			ofn.lpstrCustomFilter = NULL;
			ofn.nFilterIndex = 0;
			ofn.lpstrFile = ncdfs_internal_windows_return_path; // need to clear this buffer before using
			ofn.nMaxFile = 1024;
			ofn.lpstrTitle = ncdfs_internal_windows_title;
			ofn.lpstrFileTitle = NULL;
			ofn.lpstrInitialDir = ncdfs_internal_windows_initial;
			ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

			ofn.lpstrFilter = (wchar_t *)realfilter;

			if(type == 0)
			{
				if(GetOpenFileName(&ofn) != 0)
				{
					uconvert((char *)ncdfs_internal_windows_return_path, U_UNICODE, ncdfs_internal_return_path, U_UTF8, 4096);
					return ncdfs_internal_return_path;
				}
				else
				{
					return NULL;
				}
			}
			else
			{
				if(GetSaveFileName(&ofn) != 0)
				{
					uconvert((char *)ncdfs_internal_windows_return_path, U_UNICODE, ncdfs_internal_return_path, U_UTF8, 4096);
					return ncdfs_internal_return_path;
				}
				else
				{
					return NULL;
				}
			}
			return NULL;

		#else

			return ncd_file_select_allegro(type, initial, title, lp);

		#endif

	#endif
}

char * ncd_folder_select_allegro(char * title)
{
	if(file_select_ex(title, ncdfs_internal_return_folder, "", 1024, 320, 240))
	{
		return ncdfs_internal_return_folder;
	}
	return NULL;
}

char * ncd_folder_select(char * title)
{

	#ifdef ALLEGRO_LEGACY
		char * retpath = ncd_file_select_a5(2, NULL, title, NULL);

		if(retpath)
		{
			put_backslash(retpath);
		}
		return retpath;

	#else

		#ifdef ALLEGRO_WINDOWS

			HWND allegro_window = win_get_window();
			BROWSEINFO folderinfo;
			LPCITEMIDLIST pidl;
			if(ncdfs_use_allegro)
			{
				return ncd_folder_select_allegro(title);
			}

			uconvert(title, U_UTF8, (char *)(&ncdfs_internal_windows_title[0]), U_UNICODE, 4096);

			folderinfo.hwndOwner = allegro_window;
			folderinfo.pidlRoot = NULL;
			folderinfo.pszDisplayName = ncdfs_internal_windows_return_folder;
			folderinfo.lpszTitle = ncdfs_internal_windows_title;
			folderinfo.ulFlags = 0;
			folderinfo.lpfn = NULL;

			pidl = SHBrowseForFolder(&folderinfo);
			if(pidl)
			{
				SHGetPathFromIDList(pidl, ncdfs_internal_windows_return_folder);
				uconvert((char *)ncdfs_internal_windows_return_folder, U_UNICODE, ncdfs_internal_return_folder, U_UTF8, 4096);
				ustrcat(ncdfs_internal_return_folder, "\\");
				return ncdfs_internal_return_folder;
			}
			return NULL;

		#else

			return ncd_folder_select_allegro(title);

		#endif

	#endif
}
