#ifndef _srt_parse_h_
#define _srt_parse_h_

#include <stdio.h>

#define SRTTIMESTAMPMAXFIELDLENGTH 3
	//Used to define the max length of the hours, minutes, seconds and millisecond fields in SRT timestamps

void SRT_Load(FILE *inf);
	//Loads the SRT formatted file into the Lyrics structure
char *SeekNextSRTTimestamp(char *ptr);
	//find next occurence of a timestamp in hh:mm:ss,mmm format, return pointer to beginning of timestamp, or NULL if no timestamp is found
	//If ptr is passed as NULL, NULL is returned
unsigned long ConvertSRTTimestamp(char **ptr,int *errorstatus);
	//Accepts pointer to a timestamp in hh:mm:ss,mmm format, returns conversion in milliseconds
	//ptr is advanced to first character after end of timestamp
	//If errorstatus is NOT NULL, a nonzero number is instead stored into it and 0 is returned upon parse error
	//otherwise, program exits upon parse error
int Write_SRT_Timestamp(FILE *outf,unsigned long timestamp);
	//Writes an SRT style timestamp to the specified file (hh:mm:ss,mmm)
	//Returns zero on success
void Export_SRT(FILE *outf);
	//Exports the Lyric structure to specified file in SRT subtitle format

#endif //#ifndef _srt_parse_h_
