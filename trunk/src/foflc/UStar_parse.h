#ifndef _ustar_parse_h_
#define _ustar_parse_h_

//
//TEMPO MANIPULATION FUNCTIONS
//
double Weighted_Mean_Tempo(void);
	//For use with source MIDI lyrics
	//Reads the tempo list in the MIDI structure, returning the weighted mean tempo
double Mean_Timediff_Tempo(void);
	//For use with tempo-less MIDI lyrics (VL, script.txt)
	//Calculates the mean of the difference between all starting lyric timestamps
	//and returns an estimated tempo
char *ConvertTempoToString(double tempo);
	//Takes input tempo and returns a string representation as "###,##" using snprintf
	//There are no leading zeros for the integer part, and the decimal part is rounded at the hundredths place
	//NULL is returned on error
double ConvertStringToTempo(char *tempo);
	//Takes input string of a tempo (represented as "###,##") and returns in numerical format
double BruteForceTempo(double start,double end);
	//Returns the tempo from start to end with the lowest time diff. for conversion from realtime to quarterbeats


void Export_UStar(FILE *outf);
	//Export the loaded lyrics in beat-timed format
void UStar_Load(FILE *inf);
	//Perform all code necessary to load an UltraStar format file
char *ReadUStarTag(char *str);
	//Reads a UltraStar format tag, returning a newly allocated string by reference.  Since there is no end of tag
	//delimiter, this function truncates '\r' and '\n' from the end of the string if it exists.
double CalculateTimeDiff(double tempo);
	//Calculates the mean time diff. for lyric timing conversion from realtime to quarterbeats of the specified tempo
void RemapPitches(void);
	//Expects all Lyrics to have been parsed and stored into the Lyrics structure.  If all pitches are within the
	//range of [36,84], they are left as-is.  Otherwise, pitches that are out of ranged are raised/lowered by one
	//or more octaves until they fall into the correct range
int CheckPitches(unsigned char *pitchmin,unsigned char *pitchmax);
	//Expects all Lyrics to have been parsed and stored into the Lyrics structure.  If all pitches are within the
	//range of [36,84], zero is returned.  Otherwise, nonzero is returned, which indicates remapping will be
	//necessary for the pitches to be suitable for use in Rock Band/FoF
	//If pitchmin and pitchmax are not NULL, the minimum and maximum pitches found are stored in the referenced variables

#endif //#ifndef _ustar_parse_h_
