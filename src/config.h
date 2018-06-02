#ifndef EOF_CONFIG_H
#define EOF_CONFIG_H

void eof_load_config(char * fn);	//Loads EOF settings from the specified configuration file
void eof_save_config(char * fn);	//Saves EOF settings to the specified configuration file

void eof_build_gp_drum_mapping_string(char *destination, size_t size, unsigned char *mapping_list);
	//Populates destination with a string representation of the numbers in mapping_list
	//If the mapping_list array is empty (begins with a value of 0), "0" is written to the destination string
	//size is the size of the destination array, used to prevent buffer overflow
unsigned long eof_parse_gp_drum_mappings(unsigned char *destination, char *input, unsigned countlimit);
	//Reads the input string, parsing ASCII representations of numbers (each comma or space delimited)
	//and stores the binary conversions into the destination array.
	//Up to countlimit number of values are added to the destination array
	//After all values have been parsed, the last stored value in the destination array is followed by a zero value
	// to indicate that there are no more entries, as long as countlimit indicates there is room for this padding
	//Returns the number of parsed values stored into the destination, or zero on error
int eof_lookup_drum_mapping(unsigned char *mapping_list, unsigned char value);
	//Checks the elements in the specified drum mapping list for the specified value
	//Elements are checked starting from index 0 until the first zero element value is reached
	//If the specified value is found, nonzero is returned

#endif
