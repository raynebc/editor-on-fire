#ifndef _script_parse_h_
#define _script_parse_h_

#include <stdio.h>

void Export_Script(FILE *outf);
	//Exports the Lyric structure to specified file in script.txt format
	//If Lyrics.plain is nonzero, only the lyric text itself and no other tags/timing/etc. is exported
void Script_Load(FILE *inf);	//Perform all code necessary to load a script.txt format file

#endif //#ifndef _script_parse_h_
