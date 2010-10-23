#ifndef _lrc_parse_h_
#define _lrc_parse_h_

#define LRCTIMESTAMPMAXFIELDLENGTH 2
	//Used to define the max length of the minutes, seconds and hundredths fields in LRC timestamps

void LRC_Load(FILE *inf);
	//Loads the LRC or ELRC formatted file into the Lyrics structure
char *SeekNextLRCTimestamp(char *ptr);
	//find next occurence of a timestamp in [xx:yy:zz] format, return pointer to beginning of timestamp, or NULL if no timestamp is found
	//If ptr is passed as NULL, NULL is returned
unsigned long ConvertLRCTimestamp(char **ptr,int *errorstatus);
	//Accepts pointer to a timestamp in [xx:yy:zz] format, returns conversion in milliseconds
	//ptr is advanced to first character after end of timestamp
	//If errorstatus is NOT NULL, a nonzero number is instead stored into it and 0 is returned upon parse error
	//otherwise, program exits upon parse error
void WriteLRCTimestamp(FILE *outf,char openchar,char closechar,unsigned long time);
	//Accepts the time given in milliseconds and writes a timestamp to specified FILE stream, using the specified characters at
	//the beginning and end of the timestamp: ie. <##:##.##> or [##:##.##]
void Export_LRC(FILE *outf);
	//Exports the Lyric structure to specified file in simple or extended LRC format (based on the value of Lyrics.out_format)

#endif //#ifndef _lrc_parse_h_
