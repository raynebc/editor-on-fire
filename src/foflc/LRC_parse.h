#ifndef _lrc_parse_h_
#define _lrc_parse_h_

#define LRCTIMESTAMPMAXFIELDLENGTH 3
	//Used to define the max length of the minutes, seconds and hundredths/thousandths fields in LRC timestamps

void LRC_Load(FILE *inf);
	//Loads the LRC or ELRC formatted file into the Lyrics structure
char *SeekNextLRCTimestamp(char *ptr);
	//find next occurence of a timestamp in [xx:yy:zz[z]] format, return pointer to beginning of timestamp, or NULL if no timestamp is found
	//If ptr is passed as NULL, NULL is returned
unsigned long ConvertLRCTimestamp(char **ptr,int *errorstatus);
	//Accepts pointer to a timestamp in [xx:yy:zz[z]] format, returns conversion in milliseconds
	//ptr is advanced to first character after end of timestamp
	//If errorstatus is NOT NULL, a nonzero number is instead stored into it and 0 is returned upon parse error
	//otherwise, program exits upon parse error
void WriteLRCTimestamp(FILE *outf,char openchar,char closechar,unsigned long time);
	//Accepts the time given in milliseconds and writes a timestamp to specified FILE stream, using the specified characters at
	//the beginning and end of the timestamp: ie. <##:##.##> or [##:##.##]
	//If openchar or closechar are zero, that character is not written
void Export_LRC(FILE *outf);
	//Exports the Lyric structure to specified file in simple or extended LRC format (based on the value of Lyrics.out_format)
	//Or if Lyrics.out_format is ILRC_FORMAT, lyrics export in a format that is essentially LRC with slightly different formatting and no metadata tags, for use with the game Immerrock
void Export_QRC(FILE *outf);
	//Exports the Lyric structure to the specified file in QRC format (an ELRC variant popular in China)

#endif //#ifndef _lrc_parse_h_
