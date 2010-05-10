#ifndef _script_parse_h_
#define _script_parse_h_

#include <stdio.h>

void Export_Script(FILE *outf);	//Exports the Lyric structure to specified file in script.txt format
void Script_Load(FILE *inf);	//Perform all code necessary to load a script.txt format file

#endif //#ifndef _script_parse_h_
