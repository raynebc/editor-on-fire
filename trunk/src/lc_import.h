#ifndef LC_IMPORT_H
#define LC_IMPORT_H

#include "song.h"

int EOF_IMPORT_VIA_LC(EOF_VOCAL_TRACK *tp, struct Lyric_Format **lp, int format, char *inputfilename, char *string2);
int EOF_EXPORT_TO_LC(EOF_VOCAL_TRACK * tp,char *outputfilename,char *string2,int format);

#endif
