#ifndef EOF_SM_H
#define EOF_SM_H

int eof_import_stepmania(char * fn);
	//Accepts a path to a .sm or .ssc file, parsing it to import notes into the dance track
	//If the tempo map is not locked, it will be overwritten by the tempos defined in the imported file
	//Returns 1 on error, or 2 on user cancellation

int eof_read_stepmania_tempo(char **strptr, double *beat, double *tempo);
	//Accepts a pointer to a position in the line buffer, reading the next tempo definition
	//*strptr is updated to reflect the next position in the line to continue parsing the next tempo change
	//If beat is nonzero, the parsed beat number is returned through it
	//If tempo is nonzero, the parsed tempo is returned through it
	//Returns nonzero if a valid tempo change was parsed

#endif
