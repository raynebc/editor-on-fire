#ifndef EOF_CONFIG_H
#define EOF_CONFIG_H

void eof_load_config(char * fn);	//Loads EOF settings from the specified configuration file
void eof_save_config(char * fn);	//Saves EOF settings to the specified configuration file

void eof_build_gp_drum_mapping_string(char *destination, size_t size, unsigned char *mapping_list);
	//Populates destination with a string representation of the numbers in mapping_list
	//If the mapping_list array is empty (begins with a value of 0), "0" is written to the destination string
	//size is the size of the destination array, used to prevent buffer overflow
unsigned long eof_parse_number_list(unsigned char *destination, char *input, unsigned countlimit);
	//Reads the input string, parsing ASCII representations of numbers (each comma or space delimited)
	//and stores the binary conversions into the destination array.
	//Up to countlimit number of values are added to the destination array
	//Returns the number of parsed values stored into the destination, or zero on error
unsigned long eof_parse_gp_drum_mappings(unsigned char *destination, char *input, unsigned countlimit);
	//Uses eof_parse_number_list() to parse the numbers in the input list and store them in the destination buffer
	//After all values have been parsed, the last stored value in the destination array is followed by a zero value
	// to indicate that there are no more entries, as long as countlimit indicates there is room for this padding
	//Returns the number of parsed values stored into the destination, or zero on error
int eof_lookup_drum_mapping(unsigned char *mapping_list, unsigned char value);
	//Checks the elements in the specified drum mapping list for the specified value
	//Elements are checked starting from index 0 until the first zero element value is reached
	//If the specified value is found, nonzero is returned

int eof_parse_config_entry_name(char *name, size_t name_size, char *value, size_t value_size, const char *string);
	//Parses the name and value of the config entry from the given string, removing leading whitespace from each and trailing whitespace from the name
	//Returns nonzero on error, such as if the config string is not in the proper format of "[name] = [text]"
int eof_validate_default_ini_entry(const char *entry, int silent);
	//Returns nonzero if the specified text is not valid (ie. a duplicate of an existing default INI entry or it is malformed)
	//If the formatting is valid, uses eof_parse_config_entry_name() to ensure the entry isn't already stored in eof_default_ini_setting[]
	// and returns nonzero if found to be a duplicate.
	//If silent is zero, dialog messages are presented to tell the user why an entry fails validation
int eof_default_ini_add(const char *entry, int silent);
	//Uses eof_validate_default_ini_entry() to ensure the proposed entry is valid, and appends it if it is
	//Returns nonzero on error, such as if the entry is already defined or the entry is malformed
	//If silent is zero, dialog messages are presented to tell the user why an entry fails to be added (ie. duplicate/malformed)

#endif
