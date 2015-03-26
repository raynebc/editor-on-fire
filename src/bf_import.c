#include <allegro.h>
#include "bf_import.h"
#include "main.h"
#include "midi.h"	//For eof_apply_ts()
#include "undo.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

//Define this value to > 1 in order to lessen the amount of logging written to file
#define BF_IMPORT_LOGGING_LEVEL 1

void pack_ReadDWORDBE(PACKFILE *inf, void *data)
{
	unsigned char buffer[4] = {0};
	unsigned long *ptr = data;

	if(inf)
	{
		if((pack_fread(buffer, 4, inf) == 4) && data)
		{	//If the data was successfully read, and data isn't NULL, store the value
			*ptr=((unsigned long)buffer[0]<<24) | ((unsigned long)buffer[1]<<16) | ((unsigned long)buffer[2]<<8) | ((unsigned long)buffer[3]);	//Convert to 4 byte integer
		}
	}
}

void pack_ReadQWORDBE(PACKFILE *inf, void *data)
{
	unsigned char buffer[8] = {0};
	unsigned long long *ptr = data;

	if(inf)
	{
		if((pack_fread(buffer, 8, inf) == 8) && data)
		{	//If the data was successfully read, and data isn't NULL, store the value
			*ptr=((unsigned long long)buffer[0]<<56) | ((unsigned long long)buffer[1]<<48) | ((unsigned long long)buffer[2]<<40) | ((unsigned long long)buffer[3]<<32) | ((unsigned long long)buffer[4]<<24) | ((unsigned long long)buffer[5]<<16) | ((unsigned long long)buffer[6]<<8) | ((unsigned long long)buffer[7]);	//Convert to 8 byte integer
		}
	}
}

int eof_bf_qsort_time_changes(const void * e1, const void * e2)
{
	struct bf_timing_change * thing1 = (struct bf_timing_change *)e1;
	struct bf_timing_change * thing2 = (struct bf_timing_change *)e2;

	//Sort by timestamp
	if(thing1->realtimems < thing2->realtimems)
	{
		return -1;
	}
	else if(thing1->realtimems > thing2->realtimems)
	{
		return 1;
	}

	//Sort by item type (time signature changes before tempo changes)
	if(!thing1->type)
	{	//If the first item is a time signature
		return -1;
	}
	if(!thing2->type)
	{	//If the second item is a time signature
		return 1;
	}

	//They are equal
	return 0;
}

char *eof_lookup_bf_string_key(struct bf_string *ptr, unsigned long arraysize, unsigned long long key)
{
	unsigned long ctr;
	char *result = NULL;

	for(ctr = 0; ctr < arraysize; ctr++)
	{	//For each string in the array
		if(ptr[ctr].indkey == key)
		{	//If the string has the target key
			result = ptr[ctr].string;
			break;
		}
	}

	return result;
}

int eof_dword_to_binary_string(unsigned long dword, char *buffer)
{
	unsigned long index, ctr, bitmask;

	if(!buffer)
		return -1;	//Return error

	for(ctr = 0, index = 0, bitmask = 0x80000000; ctr < 32; ctr++, bitmask >>= 1)
	{	//For each bit in the 32 bit value, from most significant to least significant
		if(dword & bitmask)
		{	//If this bit is set
			buffer[index++] = '1';	//Append a 1 to the string
		}
		else if(index)
		{	//If the bit isn't set, but a previous bit was
			buffer[index++] = '0';	//Append a 0 to the string
		}
	}
	if(!index)
	{	//If no bits in dword were set
		buffer[index++] = '0';	//Append a 0 to the string
	}
	buffer[index] = '\0';	//Terminate the string
	return 0;	//Return success
}

EOF_SONG *eof_load_bf(char * fn)
{
	EOF_SONG * sp = NULL;
	char header[5] = {0};		//Used to read header
	#define BF_IMPORT_BUFFER_SIZE 2048
	char buffer[BF_IMPORT_BUFFER_SIZE + 1] = {0};	//Used to read strings
	char *lang, lang_english[] = "English", lang_japanese[] = "Japanese", lang_german[] = "German", lang_italian[] = "Italian", lang_spanish[] = "Spanish", lang_french[] = "French", *string;
	unsigned long sectionctr, ctr, ctr2, dword = 0, dword2 = 0, dword3 = 0, dword4, dword5, dword6, dword7, dword8, numstbentries, offset;
	unsigned long fileadd = 0;				//Use this to track the address within the input file, since Allegro's file I/O routines don't offer a way to determine this
	unsigned long long qword, qword2, qword3;
	int word;
	PACKFILE *inf = NULL;
	struct bf_section *sectiondata = NULL;	//Will be used to store details of each section
	unsigned long numsections = 0;			//The number of sections stored in the above array
	#define BF_IMPORT_STRING_NUM 1000
	struct bf_string *stringdata = NULL;	//Will be used to store strings and their key identifiers
	unsigned long numstrings = 0;			//The number of strings stored in the above array
	char parent;							//Tracks whether a parsed ZOBJ section is a parent or child object
	char nfn[1024] = {0};
	#define BF_IMPORT_TIME_CHANGE_NUM 400
	struct bf_timing_change changes[BF_IMPORT_TIME_CHANGE_NUM] = {{0,0.0,0,0.0,0,0.0,0,0}};	//Used to store all time signature and tempo changes
	unsigned long numchanges = 0;				//The number of changes stored in the above array
	unsigned curnum, curden, lastnum, lastden;	//Used to build the tempo map
	double curbeatlen, curtime;					//Used to build the tempo map
	unsigned long curtimems;					//Used to build the tempo map
	unsigned long lastitem = 0;					//Used to track the realtime position of the last item in the chart, used to build the tempo map
	unsigned long start, end, temp;				//Used to correct lyric line positions

	eof_log("\tImporting Bandfuse file", 1);
	eof_log("eof_load_bf() entered", 1);

	//Initialize pointers and handles
	if(!fn)
	{
		return NULL;
	}
	inf = pack_fopen(fn, "rb");	//Open file in binary mode
	if(!inf)
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError loading:  Cannot open input xml file:  \"%s\"", strerror(errno));	//Get the Operating System's reason for the failure
		eof_log(eof_log_string, 1);
		return NULL;
	}
	sp = eof_create_song_populated();
	if(!sp)
	{
		(void) pack_fclose(inf);
		return NULL;
	}

	//Parse RIFF header
	pack_ReadDWORDBE(inf, header);	//Read the header string and reverse the byte order
	pack_ReadDWORDBE(inf, &dword);	//Read the RIFF chunk size
	fileadd += 8;	//Update file address
	if((strcmp(header,"RIFF") != 0) || !dword)
	{	//Doesn't match expected header or a valid chunk size wasn't read
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tInvalid RIFF header \"%s\" (size %lu).  Aborting.", header, dword);
		eof_log(eof_log_string, 1);
		(void) pack_fclose(inf);
		eof_destroy_song(sp);
		return NULL;
	}
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tRIFF chunk size = %lu bytes.", dword);
	eof_log(eof_log_string, 1);

	//Parse INDX header
	dword = 0;
	pack_ReadDWORDBE(inf, header);	//Read the header string and reverse the byte order
	pack_ReadDWORDBE(inf, &dword);	//Read the INDX chunk size
	pack_ReadDWORDBE(inf, &numsections);	//Read the number of sections
	pack_ReadDWORDBE(inf, &dword2);	//Read the value expected to be 0
	fileadd += 16;	//Update file address
	if((strcmp(header,"INDX") != 0) || !dword || !numsections || (dword2 != 4))
	{	//Doesn't match expected header or a valid values weren't read
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tInvalid INDX header \"%s\" (size = %lu, # of sections  = %lu, constant value = %lu).  Aborting.", header, dword, numsections, dword2);
		eof_log(eof_log_string, 1);
		(void) pack_fclose(inf);
		eof_destroy_song(sp);
		return NULL;
	}
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tINDX chunk size = %lu bytes, # of sections = %lu.", dword, numsections);
	eof_log(eof_log_string, 1);

	//Build an array to store the section information into
	sectiondata = malloc(sizeof(struct bf_section) * numsections);	//Allocate the memory
	if(!sectiondata)
	{	//If the memory was not allocated
		eof_log("\tError allocating memory for sections.  Aborting", 1);
		(void) pack_fclose(inf);
		eof_destroy_song(sp);
		return NULL;
	}
	memset(sectiondata, 0, sizeof(struct bf_section) * numsections);	//Set the memory to all 0s

	//Build an array to store the strings into
	stringdata = malloc(sizeof(struct bf_string) * BF_IMPORT_STRING_NUM);	//Allocate the memory
	if(!stringdata)
	{	//If the memory was not allocated
		eof_log("\tError allocating memory for strings.  Aborting", 1);
		(void) pack_fclose(inf);
		free(sectiondata);
		eof_destroy_song(sp);
		return NULL;
	}
	memset(stringdata, 0, sizeof(struct bf_string) * BF_IMPORT_STRING_NUM);	//Set the memory to all 0s

	//Parse section information
	for(sectionctr = 0; sectionctr < numsections; sectionctr++)
	{	//For each section in the INDX header
		qword = dword = 0;
		pack_ReadQWORDBE(inf, &qword);	//Read the section's index key identifier
		sectiondata[sectionctr].indkey = qword;
		pack_ReadDWORDBE(inf, &dword);	//Read the section's file address
		sectiondata[sectionctr].address = dword;
		pack_ReadDWORDBE(inf, NULL);	//Read and ignore 4 bytes of padding
		if(!qword || !dword)
		{	//If valid values weren't read
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tUnable to read INDX section #%lu (Index key ID = 0x%016I64X, address = 0x%lX).  Aborting.", sectionctr, qword, dword);
			eof_log(eof_log_string, 1);
			(void) pack_fclose(inf);
			free(sectiondata);
			free(stringdata);
			eof_destroy_song(sp);
			return NULL;
		}
		fileadd += 16;	//Update file address
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tSection #%lu:\tIndex key = 0x%016I64X, address = 0x%lX", sectionctr, qword, dword);
		eof_log(eof_log_string, BF_IMPORT_LOGGING_LEVEL);
	}

	//Parse sections
	for(sectionctr = 0; sectionctr < numsections; sectionctr++)
	{	//For each section defined in the INDX header
		dword = dword2 = dword3 = dword4 = qword = qword2 = qword3 = 0;
		pack_ReadDWORDBE(inf, header);	//Read the header string and reverse the byte order

		if(!strcmp(header,"STbl"))
		{	//If this is an STbl header
			fileadd += 4;	//Update file address
			pack_ReadDWORDBE(inf, &dword);	//Read the STbl chunk size
			pack_ReadQWORDBE(inf, &qword);	//Read the index key
			pack_ReadQWORDBE(inf, &qword2);	//Read the folder key
			pack_ReadQWORDBE(inf, &qword3);	//Read the language key
			pack_ReadQWORDBE(inf, NULL);	//Read and ignore 8 bytes of padding
			pack_ReadDWORDBE(inf, &numstbentries);	//Read the number of string entries
			pack_ReadDWORDBE(inf, &dword2);	//Read the value expected to be 12
			pack_ReadDWORDBE(inf, &dword3);	//Read the string table size
			pack_ReadDWORDBE(inf, &dword4);	//Read the number of bytes until the beginning of the string table (including this dword, so this value will never be zero)
			fileadd += 52;	//Update file address
			if(!dword || !qword || !qword2 || !qword3 || !numstbentries || (dword2 != 12) || !dword3 || !dword4)
			{	//If valid values weren't read
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tInvalid STbl header \"%s\" (size = %lu, index key = 0x%016I64X, folder key = 0x%016I64X, language key = 0x%016I64X, number of entries = %lu, table size = %lu, bytes until table = %lu.  Aborting.", header, dword, qword, qword2, qword3, numstbentries, dword3, dword4);
				eof_log(eof_log_string, 1);
				(void) pack_fclose(inf);
				free(sectiondata);
				free(stringdata);
				eof_destroy_song(sp);
				return NULL;
			}
			switch(qword3)
			{
				case 0x4B1B8838F44C207C:
					lang = lang_english;
				break;
				case 0xBBC4BF723FCD64B4:
					lang = lang_japanese;
				break;
				case 0x750CDF9C21C7F0A0:
					lang = lang_german;
				break;
				case 0x3A07E146FF1CCA40:
					lang = lang_italian;
				break;
				case 0xE612EF05D38C0E52:
					lang = lang_spanish;
				break;
				case 0x58A752C7A10CF8A0:
					lang = lang_french;
				break;
				default:
					lang = "Unknown";
			}
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tSection #%lu (STbl) chunk size = %lu, index key = 0x%016I64X, folder key = 0x%016I64X, language key = 0x%016I64X (%s), number of entries = %lu, table size = %lu, bytes until table = %lu", sectionctr, dword, qword, qword2, qword3, lang, numstbentries, dword3, dword4);
			eof_log(eof_log_string, 1);

			//Parse STbl entries
			for(ctr = 0; ctr < numstbentries; ctr++)
			{	//For each entry in the string table
				pack_ReadQWORDBE(inf, &qword);	//Read the entry key (is allowed to be all 0s for an empty entry)
				pack_ReadDWORDBE(inf, &dword);	//Read the offset (from the start of the string table) to this entry's string (is allowed to be all 0s for an empty entry)
				pack_ReadDWORDBE(inf, NULL);	//Read and ignore 4 bytes of padding
				fileadd += 16;	//Update file address
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tEntry #%lu:\tIndex key = 0x%016I64X, offset from string table start = %lu", ctr, qword, dword);
				eof_log(eof_log_string, 2);
				if(lang == lang_english)
				{	//If this is an English language string table
					char duplicate = 0;
					if(numstrings < BF_IMPORT_STRING_NUM)
					{	//If the string table is large enough to add this to the string array
						for(ctr2 = 0; ctr2 < numstrings; ctr2++)
						{	//For each existing entry in the string array
							if(stringdata[ctr2].indkey == qword)
							{	//If the key already exists in the list
								duplicate = 1;
								break;
							}
						}
						if(!duplicate)
						{	//If an array entry with this key didn't already exist, add it
							stringdata[numstrings].indkey = qword;
							stringdata[numstrings].offset = dword;
							numstrings++;
						}
					}
				}
			}

			//Parse strings
			offset = 0;	//Use this to track the current byte offset into the string table
			for(ctr = 0; ctr < numstbentries; ctr++)
			{	//For each entry in the string table
				dword = 0;	//Use this to count the string's size in bytes
				do{
						word = pack_getc(inf);	//Read the next byte
						fileadd++;	//Update file address
						if(word == EOF)
						{	//If the byte couldn't be read
							eof_log("\t\tError reading string.  Aborting", 1);
							(void) pack_fclose(inf);
							free(sectiondata);
							for(ctr = 0; ctr < numstrings; free(stringdata[ctr].string), ctr++);	//Free the memory used to store each string
							free(stringdata);
							eof_destroy_song(sp);
							return NULL;
						}
						if(dword >= BF_IMPORT_BUFFER_SIZE)
						{	//If the string is too large for the buffer
							eof_log("\t\tString too large to store.  Aborting", 1);
							(void) pack_fclose(inf);
							free(sectiondata);
							for(ctr = 0; ctr < numstrings; free(stringdata[ctr].string), ctr++);	//Free the memory used to store each string
							free(stringdata);
							eof_destroy_song(sp);
							return NULL;
						}
						buffer[dword] = word;	//Store the byte into the buffer
						dword++;				//Track the string's size
				}while(word != '\0');	//Read all bytes into the buffer until a NULL terminator is reached
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tString #%lu:\tIndex key = 0x%016I64X, string table offset = %lu,\t\"%s\"", ctr, stringdata[ctr].indkey, offset, buffer);
				eof_log(eof_log_string, 1);
				if(lang == lang_english)
				{	//If this is an English language string table
					for(ctr2 = 0; ctr2 < numstrings; ctr2++)
					{	//For each entry in the string array
						if(stringdata[ctr2].offset == offset)
						{	//If the string just read is at an offset at which a string is expected
							if(stringdata[ctr2].string == NULL)
							{	//If a string wasn't read for this offset yet
								size_t size = ustrsize(buffer) + 1;	//Get the length of the string in bytes (UTF-8 aware size) and add 1 byte for NULL
								stringdata[ctr2].string = malloc(size);	//Allocate memory for the string
								if(!stringdata[ctr2].string)
								{	//If the memory couldn't be allocated
									eof_log("\t\tError storing string into array.  Aborting", 1);
									(void) pack_fclose(inf);
									free(sectiondata);
									for(ctr = 0; ctr < numstrings; free(stringdata[ctr].string), ctr++);	//Free the memory used to store each string
									free(stringdata);
									eof_destroy_song(sp);
									return NULL;
								}
								(void) ustrzcpy(stringdata[ctr2].string, size, buffer);
							}
						}
					}
				}
				offset += dword;	//Advance the offset byte counter by the size of the string (including NULL terminator)
			}

			//STbl sections have a variable amount of padding, just skip the remaining bytes until the next section begins
			if(sectionctr + 1 < numsections)
			{	//If there's another section
				while(fileadd < sectiondata[sectionctr + 1].address)
				{	//Until the next section is reached
					word = pack_getc(inf);	//Read the next byte
					fileadd++;	//Update file address
					if(word == EOF)
					{	//If the byte couldn't be read
						eof_log("\t\tError reading padding.  Aborting", 1);
						(void) pack_fclose(inf);
						free(sectiondata);
						for(ctr = 0; ctr < numstrings; free(stringdata[ctr].string), ctr++);	//Free the memory used to store each string
						free(stringdata);
						eof_destroy_song(sp);
						return NULL;
					}
				}
			}
		}//If this is an STbl header
		else if(!strcmp(header,"ZOBJ"))
		{	//If this is a ZOBJ header
			unsigned long zobjsize;

			fileadd += 4;	//Update file address
			dword = qword = qword2 = qword3 = 0;
			pack_ReadDWORDBE(inf, &zobjsize);	//Read the ZOBJ chunk size (expected to contain a bare minimum of the four qwords below)
			pack_ReadQWORDBE(inf, &qword);	//Read the index key
			pack_ReadQWORDBE(inf, &qword2);	//Read the object key
			pack_ReadQWORDBE(inf, &qword3);	//Read the type string key
			pack_ReadQWORDBE(inf, NULL);	//Read and ignore 8 bytes of padding
			fileadd += 36;	//Update file address
			if((zobjsize < 32) || !qword || !qword2 || !qword3)
			{	//If valid values weren't read
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tUnable to read ZOBJ section (index key = 0x%016I64X, object key = 0x%016I64X, type string key = 0x%016I64X.  Aborting.", qword, qword2, qword3);
				eof_log(eof_log_string, 1);
				(void) pack_fclose(inf);
				free(sectiondata);
				for(ctr = 0; ctr < numstrings; free(stringdata[ctr].string), ctr++);	//Free the memory used to store each string
				free(stringdata);
				eof_destroy_song(sp);
				return NULL;
			}

			//Identify whether this is a parent or child ZOBJ section
			parent = 0;
			for(ctr = 0; ctr < sectionctr; ctr++)
			{	//For each section in the array BEFORE the one being parsed
				if(sectiondata[ctr].objkey == qword2)
				{	//If the section has the same object key as this ZOBJ
					if(!sectiondata[ctr].encountered)
					{	//If this is the first the the key was referenced in the ZOBJ sections
						parent = 1;							//It is a parent object
						sectiondata[ctr].encountered = 1;	//Track that this key was referenced already (all future references to it are for child objects)
					}
					break;
				}
			}
			sectiondata[ctr].objkey = qword2;	//Store the object key in the array so it's associated with this ZOBJ section
			string = eof_lookup_bf_string_key(stringdata, numstrings, qword);	//Identify the string name of this ZOBJ section
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tSection #%lu (ZOBJ) chunk size = %lu, index key = 0x%016I64X (\"%s\", %s), object key = 0x%016I64X, type string key = 0x%016I64X", sectionctr, zobjsize, qword, string ? string : "[UNKNOWN KEY]", parent ? "Parent" : "Child", qword2, qword3);
			eof_log(eof_log_string, 1);

			//Parse the ZOBJ section
			if(zobjsize < 64)
			{	//If this object isn't large enough to be a section we're interested in
				string = NULL;	//Don't bother parsing it
			}
			if(string)
			{	//If the name of the string was identified
				char *ptr;
				unsigned long entryid, entrysize, entrycount, startms, endms;
				float start, end, tempo;

				pack_ReadDWORDBE(inf, &entryid);	//Read entry ID
				pack_ReadDWORDBE(inf, &entrysize);	//Read entry size
				pack_ReadDWORDBE(inf, &entrycount);	//Read entry count
				pack_ReadDWORDBE(inf, &dword);		//Read the value expected to be 4
				fileadd += 16;	//Update file address
				zobjsize -= 16;	//Take into account the 16 bytes read from this object

				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tEntry ID:  %lu\tEntry size = %lu\tEntry count = %lu", entryid, entrysize, entrycount);
				eof_log(eof_log_string, 1);

				if((entryid == 0) && (entrysize == 16))
				{	//Time signature object
					for(ctr = 0; ctr < entrycount; ctr++)
					{	//For each tempo in this object
						pack_ReadDWORDBE(inf, &start);	//Read offset 1
						pack_ReadDWORDBE(inf, &end);	//Read offset 2
						pack_ReadDWORDBE(inf, &dword);	//Read numerator
						pack_ReadDWORDBE(inf, &dword2);	//Read denominator
						fileadd += 16;	//Update file address
						if(numchanges < BF_IMPORT_TIME_CHANGE_NUM)
						{	//If this time signature change can be stored
							changes[numchanges].type = 0;	//Denote this entry is a time signature change
							changes[numchanges].realtime = start;
							changes[numchanges].realtimems = start + 0.5;
							changes[numchanges].TS_num = dword;
							changes[numchanges].TS_den = dword2;
							numchanges++;
						}
						if(start + 0.5 > lastitem)		//Keep track of the timestamp of the last imported item
							lastitem = start + 0.5;
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tTime signature change #%2lu offset 1 = %f, offset 2 = %f, num = %lu, den = %lu", ctr, start, end, dword, dword2);
						eof_log(eof_log_string, 1);
					}
				}
				else if((entryid == 1) && (entrysize == 12))
				{	//Tempo object
					for(ctr = 0; ctr < entrycount; ctr++)
					{	//For each tempo in this object
						pack_ReadDWORDBE(inf, &start);	//Read tempo start time
						pack_ReadDWORDBE(inf, &end);	//Read tempo end time
						pack_ReadDWORDBE(inf, &tempo);	//Read tempo
						fileadd += 12;	//Update file address
						if(numchanges < BF_IMPORT_TIME_CHANGE_NUM)
						{	//If this tempo change can be stored
							changes[numchanges].type = 1;	//Denote this entry is a tempo change
							changes[numchanges].realtime = start;
							changes[numchanges].realtimems = start + 0.5;
							changes[numchanges].realtime2 = end;
							changes[numchanges].realtime2ms = end + 0.5;
							changes[numchanges].BPM = tempo;
							numchanges++;
						}
						if(start + 0.5 > lastitem)		//Keep track of the timestamp of the last imported item
							lastitem = start + 0.5;
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tTempo change #%2lu start = %13fms, end = %13fms, tempo = %fBPM", ctr, start, end, tempo);
						eof_log(eof_log_string, 1);
					}
				}
				else if((entryid == 6) && (entrysize == 24))
				{	//Chord object
					for(ctr = 0; ctr < entrycount; ctr++)
					{	//For each chord name in this object
						pack_ReadDWORDBE(inf, &start);	//Read chord start time
						pack_ReadDWORDBE(inf, &end);	//Read chord end time
						pack_ReadQWORDBE(inf, &qword);	//Read chord name string index key
						pack_ReadQWORDBE(inf, NULL);	//Read and ignore 8 bytes of padding
						fileadd += 24;			//Update file address
						startms = start + 0.5;	//Round to nearest millisecond
						endms = end + 0.5;		//Round to nearest millisecond
						ptr = eof_lookup_bf_string_key(stringdata, numstrings, qword);	//Identify the string for this chord name
						if(ptr)
						{	//If the name was found
							(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tChord #%lu start = %lums, end = %lums, name = \"%s\"", ctr, startms, endms, ptr);
							eof_log(eof_log_string, 1);
						}
					}
				}
				else if((entryid == 8) && (entrysize == 32))
				{	//Vox object
					EOF_LYRIC *lp;
					char *type;

					for(ctr = 0; ctr < entrycount; ctr++)
					{	//For each lyric in this object
						pack_ReadDWORDBE(inf, &start);	//Read lyric start time
						pack_ReadDWORDBE(inf, &end);	//Read lyric end time
						pack_ReadDWORDBE(inf, &dword);	//Read lyric pitch
						pack_ReadDWORDBE(inf, NULL);	//Read and ignore 4 bytes of padding
						pack_ReadQWORDBE(inf, &qword);	//Read lyric string index key
						pack_ReadDWORDBE(inf, &dword2);	//Read lyric type (1 = normal, 2 = unpitched, 3 = extended (ie. pitch shift))
						pack_ReadDWORDBE(inf, NULL);	//Read and ignore 4 bytes of padding
						fileadd += 32;			//Update file address
						startms = start + 0.5;	//Round to nearest millisecond
						endms = end + 0.5;		//Round to nearest millisecond
						ptr = eof_lookup_bf_string_key(stringdata, numstrings, qword);	//Identify the string for this lyric
						if(ptr)
						{	//If the name was found
							if(dword2 == 1)
							{
								type = "normal";
							}
							else if(dword2 == 2)
							{
								type = "unpitched";
							}
							else
							{
								type = "extended";
							}
							(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tLyric #%lu start = %lums, end = %lums, name = \"%s\", type = %lu (%s)", ctr, startms, endms, ptr, dword2, type);
							eof_log(eof_log_string, 1);
							lp = eof_vocal_track_add_lyric(sp->vocal_track[0]);	//Create a lyric structure
							if(!lp)
							{	//If the lyric couldn't be added
								eof_log("\t\tUnable to add lyric.  Aborting.", 1);
								(void) pack_fclose(inf);
								free(sectiondata);
								for(ctr = 0; ctr < numstrings; free(stringdata[ctr].string), ctr++);	//Free the memory used to store each string
								free(stringdata);
								eof_destroy_song(sp);
								return NULL;
							}
							(void) ustrncpy(lp->text, ptr, EOF_MAX_LYRIC_LENGTH);	//Copy the lyric text into the lyric structure
							lp->pos = startms;
							lp->length = endms - startms + 0.5;	//Round to nearest millisecond
							if(dword2 == 2)
							{	//If this lyric is pitchless
								lp->note = 0;	//Explicitly set it as such
							}
							else
							{
								lp->note = (dword & 0xFF00) >> 8;	//Otherwise set the defined pitch
							}
						}
					}
				}
				else if((entryid == 10) && (entrysize == 8))
				{	//voxpushphrase object
					for(ctr = 0; ctr < entrycount; ctr++)
					{	//For each lyric line in this object
						pack_ReadDWORDBE(inf, &start);	//Read lyric line start time
						pack_ReadDWORDBE(inf, &end);	//Read lyric line end time
						fileadd += 8;			//Update file address
						startms = start + 0.5;	//Round to nearest millisecond
						endms = end + 0.5;		//Round to nearest millisecond
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tLyric line #%lu start = %lums, end = %lums", ctr, startms, endms);
						eof_log(eof_log_string, 1);
///						(void) eof_vocal_track_add_line(sp->vocal_track[0], startms, endms);	//Add the lyric line
					}
				}
				else if((entryid == 11) && (entrysize == 64))
				{	//Tab object
					float amount;
					EOF_PRO_GUITAR_TRACK *tp = NULL;	//A pointer to appropriate destination pro guitar track referenced by this ZOBJ object
					EOF_PRO_GUITAR_NOTE *np = NULL;		//A pointer to the most recently created note
					EOF_PRO_GUITAR_NOTE *npp = NULL;	//A pointer to the previously created note
					unsigned char curdiff;				//The difficulty referenced by this ZOBJ object
					unsigned long flags;
					char *tech;
					unsigned long prevfret = 0;			//Used to track the fret value of the previous note, for HO/PO tracking

					//Determine the track difficulty this ZOBJ section refers to
					if(strstr(string, "bss_"))
					{	//If this is one of the bass arrangements
						tp = sp->pro_guitar_track[sp->track[EOF_TRACK_PRO_BASS]->tracknum];
						tp->parent->numdiffs = 5;
					}
					else
					{	//Otherwise assume guitar arrangement
						tp = sp->pro_guitar_track[sp->track[EOF_TRACK_PRO_GUITAR]->tracknum];
						tp->parent->numdiffs = 6;
					}
					tp->parent->flags |= EOF_TRACK_FLAG_UNLIMITED_DIFFS;	//Remove the difficulty limit for this track
					if(strstr(string, "bss_adv"))
					{	//This is level 5 difficulty for bass
						curdiff = 4;
					}
					else if(strstr(string, "_jam"))
					{
						curdiff = 0;
					}
					else if(strstr(string, "_nov"))
					{
						curdiff = 1;
					}
					else if(strstr(string, "_beg"))
					{
						curdiff = 2;
					}
					else if(strstr(string, "_int"))
					{
						curdiff = 3;
					}
					else if(strstr(string, "_rhy"))
					{	//This is level 5 difficulty for guitar (rhythm)
						curdiff = 4;
					}
					else if(strstr(string, "_adv"))
					{	//This is level 5 difficulty for guitar (lead)
						curdiff = 5;
					}
					else
					{
						curdiff = 6;	//Unknown
						tp->parent->numdiffs = 7;
					}

					for(ctr = 0; ctr < entrycount; ctr++)
					{	//For each note in this object
						//Read note data
						pack_ReadDWORDBE(inf, &start);	//Read note start time
						pack_ReadDWORDBE(inf, &end);	//Read note end time
						pack_ReadDWORDBE(inf, &dword);	//Read note string number
						pack_ReadDWORDBE(inf, &dword2);	//Read note fret number
						pack_ReadDWORDBE(inf, &dword3);	//Read note finger number
						pack_ReadDWORDBE(inf, &dword4);	//Read note statuses
						pack_ReadDWORDBE(inf, &dword5);	//Read other statuses (least significant byte is bend type)
						pack_ReadDWORDBE(inf, &amount);	//Read the slide/trill/bend amount (in steps)
						pack_ReadDWORDBE(inf, &dword6);	//Read other statuses (least significant bit indicates tremolo)
						pack_ReadDWORDBE(inf, NULL);	//Read and ignore 4 bytes of unknown use
						pack_ReadQWORDBE(inf, NULL);	//Read the value expected to be 0x427c0000427c0000
						pack_ReadDWORDBE(inf, NULL);	//Read and ignore 4 bytes of unknown use
						pack_ReadDWORDBE(inf, NULL);	//Read and ignore 4 bytes of unknown use
						pack_ReadDWORDBE(inf, &dword7);	//Read other statuses (byte 0 indicates linknext, byte 1 indicates palm mute, byte 2 indicates tremolo)
						pack_ReadDWORDBE(inf, &dword8);	//Read pop/slap status (1 indicates slap, 2 indicates pop)
						fileadd += 64;	//Update file address
						startms = start + 0.5;	//Round to nearest millisecond
						endms = end + 0.5;		//Round to nearest millisecond
						if(endms > lastitem)	//Keep track of the timestamp of the last imported item
							lastitem = endms;
						if(dword4 == 1)
						{
							tech = "Mute";
						}
						else if(dword4 == 2)
						{
							tech = "Slide";
						}
						else if(dword4 == 3)
						{
							tech = "UUp";
						}
						else if(dword4 == 4)
						{
							tech = "UDown";
						}
						else if(dword4 == 9)
						{
							tech = "Trill";
						}
						else if(dword4 == 10)
						{
							tech = "Harm";
						}
						else if(dword4 == 11)
						{
							tech = "PHarm";
						}
						else if(amount != 0.0)
						{
							tech = "Bend";
						}
						else
						{
							tech = "";
						}
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tNote #%4lu start = %6lums, end = %6lums, string = %lu, fret = %2lu, finger = %lu, statuses = %lu (%5s), statuses2 = 0x%lX, slide/bend/trill amount = %9f, statuses3 = 0x%lX, statuses4 = 0x%08lX, pop/slap = %s", ctr, startms, endms, dword, dword2, dword3, dword4, tech, dword5, amount, dword6, dword7, (!dword8 ? "neither" : (dword == 1 ? "slap" : "pop")));
						eof_log(eof_log_string, 1);

						//Create note
						np = eof_pro_guitar_track_add_note(tp);	//Allocate, initialize and add the new note to the note array
						if(!np)
						{
							eof_log("\tError allocating memory.  Aborting", 1);
							(void) pack_fclose(inf);
							free(sectiondata);
							for(ctr = 0; ctr < numstrings; free(stringdata[ctr].string), ctr++);	//Free the memory used to store each string
							free(stringdata);
							eof_destroy_song(sp);
							return NULL;
						}
						np->type = curdiff;
						dword = tp->numstrings - dword;	//Convert the string numbering
						np->note = 1 << dword;
						np->frets[dword] = dword2;
						np->pos = startms;
						flags = 0;
						if(end > start)
						{
							np->length = end - start + 0.5;
						}
						else
						{
							np->length = 1;
						}
						switch(dword4)
						{
							case 1:	//String mute
								flags |= EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE;
							break;
							case 2:	//Slide to next note
								flags |= EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION;
								if(amount >= 0.0)
								{
									flags |= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP;
									np->slideend = dword2 + ((amount / 0.5) + 0.5);
								}
								else
								{
									flags |= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN;
									np->slideend = dword2 + ((amount / 0.5) + 0.5);
								}
							break;
							case 3:	//Unpitched slide up
								flags |= EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE;
								if(amount > 0.0)
								{
									np->unpitchend = (dword2 + (amount / 0.5)) + 0.5;
								}
								if(dword2 + 1 > tp->numfrets)
								{	//If this track doesn't have enough frets to notate this
									tp->numfrets = dword2 + 1;	//Increase its limit
								}
							break;
							case 4:	//Unpitched slide down
								flags |= EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE;
								if(amount < 0.0)
								{	//This floating point value is expected to be negative
									if(dword2 + amount > 0.0)
									{	//Prevent integer underflow
										np->unpitchend = (dword2 + (amount / 0.5)) + 0.5;
									}
								}
							break;
							case 9:	//Trill
								flags |= EOF_NOTE_FLAG_IS_TRILL;	//Set this flag as a placeholder, the trill phrase will have to be created after the ZOBJ section is imported
							break;
							case 10:	//Natural harmonic
								flags |= EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC;
							break;
							case 11:	//Pinch harmonic
								flags |= EOF_PRO_GUITAR_NOTE_FLAG_P_HARMONIC;
							break;
						}
						if(dword5)
						{	//If this note bends
							flags |= EOF_PRO_GUITAR_NOTE_FLAG_BEND;
							if((dword5 == 1) && (amount > 0.0))
							{	//Normal bend
								flags |= EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION;
								np->bendstrength = (amount / 0.5) + 0.5;	//Round bend strength to the nearest number of half steps
							}
						}
						if(dword6)
						{	//If this note has vibrato
							flags |= EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO;
						}
						if((dword7 >> 24) & 0xFF)
						{	//If this note links to the next note
							flags |= EOF_PRO_GUITAR_NOTE_FLAG_LINKNEXT;
						}
						if((dword7 >> 16) & 0xFF)
						{	//If this note is palm muted
							flags |= EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE;
						}
						if((dword7 >> 8) & 0xFF)
						{	//If this note uses tremolo picking
							flags |= EOF_NOTE_FLAG_IS_TREMOLO;	//Set this flag as a placeholder, the tremolo phrase will have to be created after the ZOBJ section is imported
						}
						if(((dword8 >> 24) & 0xFF) == 1)
						{	//If this note uses slap technique
							flags |= EOF_PRO_GUITAR_NOTE_FLAG_SLAP;
						}
						if(((dword8 >> 24) & 0xFF) == 2)
						{	//If this note uses pop technique
							flags |= EOF_PRO_GUITAR_NOTE_FLAG_POP;
						}

						//Track HO/POs
						if(npp && (npp->flags & EOF_PRO_GUITAR_NOTE_FLAG_LINKNEXT))
						{	//If there was a previous note and it was marked as linked with the current note
							if(!(npp->flags & (EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP | EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN | EOF_PRO_GUITAR_NOTE_FLAG_BEND | EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE)))
							{	//If the previous note doesn't slide or bend
								if(!(np->flags & (EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP | EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN | EOF_PRO_GUITAR_NOTE_FLAG_BEND | EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE)))
								{	//If the current note doesn't slide or bend either, assume the linked status refers to hammer on or pull off
									npp->flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_LINKNEXT;	//Remove the linknext flag from the previous note
									if(dword2 > prevfret)
									{	//If this note's fret is higher
										flags |= EOF_PRO_GUITAR_NOTE_FLAG_HO;	//It's a hammer on
									}
									else
									{
										flags |= EOF_PRO_GUITAR_NOTE_FLAG_PO;	//It's a pull off
									}
								}
							}
						}

						np->flags = flags;
						if(dword2 > tp->numfrets)
							tp->numfrets = dword2;	//Track the highest used fret number
						npp = np;			//Keep track of the previously created note
						prevfret = dword2;	//And its fret value
					}//For each note in this object
					eof_build_trill_phrases(tp);	//Add trill phrases to encompass the notes that have the trill flag set
					eof_build_tremolo_phrases(tp, curdiff);	//Add tremolo phrases and define them to apply to the track difficulty that this tab object contained notes for

					//Apply string muting for notes
					for(ctr = 0; ctr < tp->notes; ctr++)
					{	//For each note in the track
						unsigned long bitmask;

						if(tp->note[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE)
						{	//If the string mute status was set during import
							for(ctr2 = 0, bitmask = 1; ctr2 < 6; ctr2++, bitmask <<= 1)
							{	//For each of the 6 supported strings
								if(tp->note[ctr]->note & bitmask)
								{	//If this string is used
									tp->note[ctr]->frets[ctr2] |= 0x80;	//Set the mute bit
								}
							}
						}
					}
				}//Tab object
				else
				{	//Unknown section type
					string = NULL;	//Set to NULL so the logic below seeks beyond the section
				}
			}

			if(!string)
			{	//If the ZOBJ section was not parsed, skip to the end of the section
				for(ctr = 0; ctr < zobjsize - 32; ctr++)
				{	//For each of the remaining bytes in the ZOBJ chunk
					word = pack_getc(inf);	//Skip to next byte
					fileadd++;	//Update file address
					if(word == EOF)
					{	//If the byte couldn't be read
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tError reading remaining %lu bytes of ZOBJ object.  Aborting", zobjsize - 32);
						eof_log(eof_log_string, BF_IMPORT_LOGGING_LEVEL);
						(void) pack_fclose(inf);
						free(sectiondata);
						for(ctr = 0; ctr < numstrings; free(stringdata[ctr].string), ctr++);	//Free the memory used to store each string
						free(stringdata);
						eof_destroy_song(sp);
						return NULL;
					}
				}
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tSeeked ahead %lu bytes to end of unrecognized/ignored ZOBJ object.", zobjsize - 32);
				eof_log(eof_log_string, BF_IMPORT_LOGGING_LEVEL);
			}
		}//If this is a ZOBJ header
		else
		{	//Unrecognized header
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tUnrecognized section header \"%s\" at file address 0x%lX.  Aborting.", header, fileadd);
			eof_log(eof_log_string, 1);
			(void) pack_fclose(inf);
			free(sectiondata);
			for(ctr = 0; ctr < numstrings; free(stringdata[ctr].string), ctr++);	//Free the memory used to store each string
			free(stringdata);
			eof_destroy_song(sp);
			return NULL;
		}
	}//For each section defined in the INDX header

	//Cleanup
	(void) pack_fclose(inf);
	free(sectiondata);
	for(ctr = 0; ctr < numstrings; free(stringdata[ctr].string), ctr++);	//Free the memory used to store each string
	free(stringdata);

	//Correct the lyric phrases, as the Bandfuse format effectively only defines rough line break positions
	eof_track_sort_notes(sp, EOF_TRACK_VOCALS);
	start = 0;	//The lyric lines will begin at the start of the chart
	for(ctr = 0; ctr < sp->vocal_track[0]->lines; ctr++)
	{	//For each lyric line that was added
		EOF_PHRASE_SECTION *llp = &sp->vocal_track[0]->line[ctr];	//A pointer to the lyric line being checked

		//Extend the lyric line up to this line break
		end = llp->start_pos;
		temp = llp->end_pos;	//Retain the "end" position of the line break
		llp->start_pos = start;
		llp->end_pos = end;
		start = temp;			//The next lyric line will begin at the end of this line break

		//Re-position the beginning of the lyric line to match the start position of the first lyric in that line
		for(ctr2 = 0; ctr2 < sp->vocal_track[0]->lyrics; ctr2++)
		{	//For each lyric that was imported
			EOF_LYRIC *lp = sp->vocal_track[0]->lyric[ctr2];			//A pointer to the lyric being checked

			if(lp->pos < llp->start_pos)	//If the lyric begins before the lyric line
				continue;					//Skip it
			if(lp->pos > llp->end_pos)		//If the lyric begins after the end of the lyric line
				break;						//All the rest of the lyrics will as well, break from lyric loop

			//This lyric falls within the lyric line
			llp->start_pos = lp->pos;		//Adjust the lyric line to begin at the start of this lyric
		}

///This logic doesn't sufficiently clean up the lines, base the start and end of lines on the last lyric in the line and the lyric that follows it
		//Re-position the end of the lyric line to match the last lyric in that line
		for(ctr2 = 0; ctr2 < sp->vocal_track[0]->lyrics; ctr2++)
		{	//For each lyric that was imported
			EOF_LYRIC *lp = sp->vocal_track[0]->lyric[ctr2];			//A pointer to the lyric being checked

			if(lp->pos < llp->start_pos)	//If the lyric begins before the lyric line
				continue;					//Skip it
			if(lp->pos > llp->end_pos)		//If the lyric begins after the end of the lyric line
				break;						//All the rest of the lyrics will as well, break from lyric loop

			//This lyric falls within the lyric line
			llp->end_pos = lp->pos + lp->length;	//Adjust the lyric line to end at the end of this lyric
		}

		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tLyric line #%lu corrected to:  start = %lums, end = %lums", ctr, sp->vocal_track[0]->line[ctr].start_pos, sp->vocal_track[0]->line[ctr].end_pos);
		eof_log(eof_log_string, 1);
	}

	//Build the tempo map, using the timestamps instead of the tempo changes, since the Bandfuse format doesn't use very accurate tempo values
	qsort(changes, (size_t)numchanges, sizeof(struct bf_timing_change), eof_bf_qsort_time_changes);	//Sort the time signature and tempo changes by timestamp
	curnum = curden = lastnum = lastden = 4;	//Until a time signature change is reached, 4/4 is assumed
	curbeatlen = 500.0;		//Until a tempo change is reached, 120BPM is assumed (the corresponding beat length is 500ms)
	curtime = 0.0;
	curtimems = 0;
	while(curtimems <= lastitem)
	{	//Add new beats until enough have been added to encompass all imported items
		if(eof_song_add_beat(sp) == NULL)	//Add a new beat
		{	//Or return failure if that doesn't succeed
			eof_destroy_song(sp);
			eof_log("Error adding beats.  Aborting", 1);
			return NULL;
		}

		//Find the tempo and time signature in effect at this beat
		for(ctr = 0; ctr < numchanges; ctr++)
		{	//For each imported time signature and tempo change
			if(curtimems >= changes[ctr].realtimems)
			{	//If the change is at or before the current beat time
				if(!changes[ctr].type)
				{	//If this is a time signature change
					curnum = changes[ctr].TS_num;
					curden = changes[ctr].TS_den;
				}
				else
				{	//This is a tempo change
					unsigned numbeats;	//The approximate number of beats between this tempo change and the next
					double tempbeatlen = 60000.0 / (changes[ctr].BPM * curden / 4.0);	//Determine the beat length as specified by the given BPM value (taking the time signature into account)
					numbeats = (changes[ctr].realtime2ms - changes[ctr].realtimems) / tempbeatlen + 0.5;	//Determine the approximate number of beats between this tempo change and the next
					if(numbeats)
					{	//If the start and end positions are far enough apart to denote at least one beat
						curbeatlen = ((double)changes[ctr].realtime2ms - changes[ctr].realtimems) / numbeats;	//Get a beat length that more accurately reflects the chart
					}
					else
					{	//The final defined tempo change defines both positions with the same value
						curbeatlen = tempbeatlen;	//Just base the beat length on the tempo
					}
				}
			}
		}

		//Set the timestamps and time signature for this beat as applicable
		sp->beat[sp->beats - 1]->fpos = curtime;	//Set this beat's timestamp
		sp->beat[sp->beats - 1]->pos = curtimems;
		if(eof_use_ts && ((lastnum != curnum) || (lastden != curden) || (sp->beats - 1 == 0)))
		{	//If the user opted to import TS changes, and this time signature is different than the last beat's time signature (or this is the first beat)
			(void) eof_apply_ts(curnum, curden, sp->beats - 1, sp, 0);	//Set the TS flags for this beat
		}

		//Advance the time variables
		curtime += curbeatlen;
		curtimems = curtime + 0.5;
	}
	eof_calculate_tempo_map(sp);	//Rebuild the tempo changes based on the beat timestamps

///Debug readout of string array
//	eof_log("English string list", 1);
//	for(ctr = 0; ctr < numstrings; ctr++)
//	{	//For each string in the array
//		if(stringdata[ctr].string)
//		{	//If this entry has a string
//			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tString #%lu:\tIndex key = 0x%016I64X, string = \"%s\"", ctr, stringdata[ctr].indkey, stringdata[ctr].string);
//			eof_log(eof_log_string, BF_IMPORT_LOGGING_LEVEL);
//		}
//	}

//Load guitar.ogg automatically if it's present, otherwise prompt user to browse for audio
	(void) replace_filename(eof_song_path, fn, "", 1024);
	(void) append_filename(nfn, eof_song_path, "guitar.ogg", 1024);
	if(!eof_load_ogg(nfn, 1))	//If user does not provide audio, fail over to using silent audio
	{
		eof_destroy_song(sp);
		return NULL;
	}
	eof_song_loaded = 1;
	eof_chart_length = alogg_get_length_msecs_ogg(eof_music_track);
	eof_log("\tImport complete", 1);

	return sp;
}
