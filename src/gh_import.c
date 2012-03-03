#include "beat.h"
#include "gh_import.h"
#include "main.h"
#include "midi.h"	//For eof_apply_ts()
#include "utility.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

unsigned long crc32_lookup[256] = {0};	//A lookup table to improve checksum calculation performance
char crc32_lookup_initialized = 0;	//Is set to nonzero when the lookup table is created
char eof_gh_skip_size_def = 0;	//Is set to nonzero if it's determined that the size field in GH headers is omitted

#define GH_IMPORT_DEBUG

#define EOF_NUM_GH_INSTRUMENT_SECTIONS 12
gh_section eof_gh_instrument_sections[EOF_NUM_GH_INSTRUMENT_SECTIONS] =
{
	{"guitareasyinstrument", EOF_TRACK_GUITAR, EOF_NOTE_SUPAEASY},
	{"guitarmediuminstrument", EOF_TRACK_GUITAR, EOF_NOTE_EASY},
	{"guitarhardinstrument", EOF_TRACK_GUITAR, EOF_NOTE_MEDIUM},
	{"guitarexpertinstrument", EOF_TRACK_GUITAR, EOF_NOTE_AMAZING},
	{"basseasyinstrument", EOF_TRACK_BASS, EOF_NOTE_SUPAEASY},
	{"bassmediuminstrument", EOF_TRACK_BASS, EOF_NOTE_EASY},
	{"basshardinstrument", EOF_TRACK_BASS, EOF_NOTE_MEDIUM},
	{"bassexpertinstrument", EOF_TRACK_BASS, EOF_NOTE_AMAZING},
	{"drumseasyinstrument", EOF_TRACK_DRUM, EOF_NOTE_SUPAEASY},
	{"drumsmediuminstrument", EOF_TRACK_DRUM, EOF_NOTE_EASY},
	{"drumshardinstrument", EOF_TRACK_DRUM, EOF_NOTE_MEDIUM},
	{"drumsexpertinstrument", EOF_TRACK_DRUM, EOF_NOTE_AMAZING}
};

#define EOF_NUM_GH_SP_SECTIONS 3
gh_section eof_gh_sp_sections[EOF_NUM_GH_SP_SECTIONS] =
{
	{"guitarexpertstarpower", EOF_TRACK_GUITAR, 0},
	{"bassexpertstarpower", EOF_TRACK_BASS, 0},
	{"drumsexpertstarpower", EOF_TRACK_DRUM, 0}
};

#define EOF_NUM_GH_TAP_SECTIONS 2
gh_section eof_gh_tap_sections[EOF_NUM_GH_TAP_SECTIONS] =
{
	{"guitarexperttapping", EOF_TRACK_GUITAR, 0},
	{"bassexperttapping", EOF_TRACK_BASS, 0}
};

filebuffer *eof_filebuffer_load(const char * fn)
{
	filebuffer *buffer = NULL;

	if(fn == NULL)
		return NULL;	//Invalid file name
	buffer = malloc(sizeof(filebuffer));
	if(buffer == NULL)
		return NULL;	//Could not allocate memory
	buffer->size = file_size_ex(fn);
	if(buffer->size == 0)
		return NULL;	//Could not get size of file
	buffer->buffer = eof_buffer_file(fn, 0);
	if(buffer->buffer == NULL)
	{
		free(buffer);
		return NULL;	//Could not allocate memory
	}
	buffer->index = 0;	//Set index to beginning of buffer

	return buffer;
}

void eof_filebuffer_close(filebuffer *fb)
{
	if(fb != NULL)
	{
		if(fb->buffer != NULL)
		{
			free(fb->buffer);
		}
		free(fb);
	}
}

int eof_filebuffer_get_byte(filebuffer *fb, unsigned char *ptr)
{
	if(!fb || !ptr)
		return EOF;
	if(fb->index + 1 >= fb->size)	//This read operation would run outside the buffer
		return EOF;
	*ptr = fb->buffer[fb->index++];

	return 0;
}

int eof_filebuffer_get_word(filebuffer *fb, unsigned int *ptr)
{
	if(!fb || !ptr)
		return EOF;
	if(fb->index + 2 >= fb->size)	//This read operation would run outside the buffer
		return EOF;
	*ptr = (fb->buffer[fb->index++] << 8);	//Read high byte value
	*ptr += fb->buffer[fb->index++];

	return 0;
}

int eof_filebuffer_memcpy(filebuffer *fb, void *ptr, size_t num)
{
	if(!fb || !ptr)
		return EOF;
	if(fb->index + num >= fb->size)	//This read operation would run outside the buffer
		return EOF;
	memcpy(ptr, &fb->buffer[fb->index], num);	//Read data
	fb->index += num;

	return 0;
}

int eof_filebuffer_get_dword(filebuffer *fb, unsigned long *ptr)
{
	if(!fb || !ptr)
		return EOF;
	if(fb->index + 4 >= fb->size)	//This read operation would run outside the buffer
		return EOF;
	*ptr = (fb->buffer[fb->index++] << 24);	//Read high byte value
	*ptr += (fb->buffer[fb->index++] << 16);
	*ptr += (fb->buffer[fb->index++] << 8);
	*ptr += fb->buffer[fb->index++];

	return 0;
}

int eof_filebuffer_read_unicode_chars(filebuffer *fb, char *ptr, unsigned long num)
{
	char byte1, byte2;
	unsigned long index = 0, ctr;

	if(!fb || !ptr)
		return EOF;

	for(ctr = 0; ctr < num; ctr++)
	{	//For each of the requested Unicode characters
		if(fb->index + 2 >= fb->size)	//Reading the next Unicode character would run outside the buffer
			return EOF;

		byte1 = fb->buffer[fb->index++];
		byte2 = fb->buffer[fb->index++];

		if(byte1 != 0)
		{	//For the English character set, the high byte of the Unicode character is 0
			return EOF;	//Unsupported Unicode language
		}
		ptr[index++] = byte2;
	}

	return 0;
}

unsigned long eof_crc32_reflect(unsigned long value, int numbits)
{
	unsigned long retval = 0, bitmask = 1, reflected_bitmask, x;

	reflected_bitmask = 1 << (numbits - 1);
	for(x = 0; x < numbits; x++, bitmask <<= 1, reflected_bitmask >>= 1)
	{	//For each of the bits to reflect
		if(value & bitmask)
		{	//If this bit is set
			retval |= reflected_bitmask;	//Set the reflected bit
		}
	}
	return retval;
}

unsigned long eof_crc32(char *string)
{
	unsigned long retval = 0xFFFFFFFF, length;
	unsigned char *ptr = (unsigned char *)string;

	if(string == NULL)
		return 0;

	if(!crc32_lookup_initialized)
	{	//Create lookup table
		unsigned long polynomial = 0x04c11db7;	//This is the polynomial defined in Ethernet specification
		int x, y;

		for(x = 0; x <= 255; x++)
		{	//For each lookup table entry
			crc32_lookup[x] = eof_crc32_reflect(x, 8) << 24;
			for(y = 0; y < 8; y++)
			{
				crc32_lookup[x] = (crc32_lookup[x] << 1) ^ (crc32_lookup[x] & (1 << 31) ? polynomial : 0);
			}
			crc32_lookup[x] = eof_crc32_reflect(crc32_lookup[x], 32);
		}
		crc32_lookup_initialized = 1;
	}

	length = ustrlen(string);
	while(length--)
	{	//For each byte in the target string
		retval = (retval >> 8) ^ crc32_lookup[(retval & 0xFF) ^ *ptr++];
	}

	return (retval ^ 0xFFFFFFFF);	//XOR result with the starting value and return it
}

int eof_filebuffer_find_checksum(filebuffer *fb, unsigned long checksum)
{
	unsigned long original_index, index1, index2;
	unsigned char checksumarray[4];

	if((fb == NULL) || (fb->buffer == NULL) || (fb->index + 5 >= fb->size))
		return 1;	//If the buffer is invalid or there are not 5 bytes left in the buffer from the current position

	original_index = fb->index;	//Back up this position
	checksumarray[0] = checksum >> 24;	//Store the checksum into an array to simplify the search logic
	checksumarray[1] = (checksum & 0xFF0000) >> 16;
	checksumarray[2] = (checksum & 0xFF00) >> 8;
	checksumarray[3] = (checksum & 0xFF);
	while(fb->index < fb->size)
	{	//While the end of buffer hasn't been reached
		if(fb->buffer[fb->index] == checksumarray[0])
		{	//If the high order byte matches
			for(index1 = fb->index + 1, index2 = 1; index2 < 4; index1++, index2++)
			{	//For the next 3 bytes
				if(fb->buffer[index1] != checksumarray[index2])
					break;	//If the byte doesn't match, break look and keep looking
			}
			if(index2 >= 4)
			{	//The 4 bytes matched, return success
				fb->index = index1;	//Position the index to the byte after the search hit
				return 0;	//Return success
			}
		}
		fb->index++;	//Increment index into file buffer
	}
	fb->index = original_index;	//Restore the original file position
	return 1;	//Return search failure
}

int eof_gh_read_instrument_section(filebuffer *fb, EOF_SONG *sp, gh_section *target)
{
	unsigned long numnotes, dword, ctr, notesize;
	unsigned int length;
	unsigned char notemask, accentmask, fixednotemask;
	EOF_NOTE *newnote = NULL;

	if(!fb || !sp || !target)
		return 0;

#ifdef GH_IMPORT_DEBUG
	snprintf(eof_log_string, sizeof(eof_log_string), "\tGH:  Looking for section \"%s\"", target->name);
	eof_log(eof_log_string, 1);
#endif
	if(eof_filebuffer_find_checksum(fb, EOF_GH_CRC32(target->name)))	//Seek one byte past the target header
	{	//If the target section couldn't be found
		eof_log("\t\tCould not find section", 1);
		return 0;
	}
	if(eof_filebuffer_get_dword(fb, &numnotes))	//Read the number of notes in the section
	{	//If there was an error reading the next 4 byte value
		eof_log("\t\tError:  Could not read number of notes", 1);
		return -1;
	}
#ifdef GH_IMPORT_DEBUG
	snprintf(eof_log_string, sizeof(eof_log_string), "\t\tNumber of notes = %lu", numnotes);
	eof_log(eof_log_string, 1);
#endif
	fb->index += 4;	//Seek past the next 4 bytes, which is a checksum for the game-specific note section subheader
	if(!eof_gh_skip_size_def)
	{	//If the size field is expected
		if(eof_filebuffer_get_dword(fb, &notesize))	//Read the size of the note entry
		{	//If there was an error reading the next 4 byte value
			eof_log("\t\tError:  Could not read note size", 1);
			return -1;
		}
		if((notesize != 8) && (notesize != 9))
		{	//Each note is expected to be 8 or 9 bytes long
			eof_log("\t\tError:  Note size is not 8 or 9", 1);
			return -1;
		}
	}
	else
	{
		eof_log("\t\tSkipping field size", 1);
	}

	for(ctr = 0; ctr < numnotes; ctr++)
	{	//For each note in the section
		if(eof_filebuffer_get_dword(fb, &dword))	//Read the note position
		{	//If there was an error reading the next 4 byte value
			eof_log("\t\tError:  Could not read note position", 1);
			return -1;
		}
		if(eof_filebuffer_get_word(fb, &length))	//Read the note length
		{	//If there was an error reading the next 2 byte value
			eof_log("\t\tError:  Could not read note length", 1);
			return -1;
		}
		if(eof_filebuffer_get_byte(fb, &notemask))	//Read the note bitmask
		{	//If there was an error reading the next 1 byte value
			eof_log("\t\tError:  Could not read note bitmask", 1);
			return -1;
		}
		if(target->tracknum == EOF_TRACK_DRUM)
		{	//In Guitar Hero, lane 6 is bass drum, lane 1 is the right-most lane (ie. 6)
			unsigned long tracknum = sp->track[EOF_TRACK_DRUM]->tracknum;

			fixednotemask = notemask;
			fixednotemask &= ~1;	//Clear this gem
			fixednotemask &= ~32;	//Clear this gem
			if(notemask & 32)
			{	//If lane 6 is populated, convert it to RB's bass drum gem
				fixednotemask |= 1;		//Set the lane 1 (bass drum) gem
			}
			if(notemask & 1)
			{	//If lane 1 is populated, convert it to lane 6
				fixednotemask |= 32;	//Set the lane 6 gem
				sp->track[EOF_TRACK_DRUM]->flags |= EOF_TRACK_FLAG_SIX_LANES;	//Ensure "five lane" drums is enabled for the track
				sp->legacy_track[tracknum]->numlanes = 6;
			}
			notemask = fixednotemask;
		}
		if(eof_filebuffer_get_byte(fb, &accentmask))	//Read the note accent bitmask
		{	//If there was an error reading the next 1 byte value
			eof_log("\t\tError:  Could not read note accent bitmask", 1);
			return -1;
		}
		newnote = (EOF_NOTE *)eof_track_add_create_note(sp, target->tracknum, (notemask & 0x3F), dword, length, target->diffnum, NULL);
		if(newnote == NULL)
		{	//If there was an error adding a new note
			eof_log("\t\tError:  Could not add note", 1);
			return -1;
		}
		if((target->tracknum == EOF_TRACK_GUITAR) || (target->tracknum == EOF_TRACK_BASS))
		{	//If this is a guitar track, check the accent mask and HOPO status
			if(accentmask != 0x1F)
			{	//"Crazy" guitar/bass notes have an accent mask that isn't 0x1F
				newnote->flags |= EOF_NOTE_FLAG_CRAZY;	//Set the crazy flag bit
			}
			if(notemask & 0x40)
			{	//Bit 6 is the HOPO status
				newnote->flags |= EOF_NOTE_FLAG_F_HOPO;
				newnote->flags |= EOF_NOTE_FLAG_HOPO;
			}
			else
			{	//If the note is not a HOPO, mark it as a forced non HOPO (strum required)
				newnote->flags |= EOF_NOTE_FLAG_NO_HOPO;
			}
		}
///If the size field is omitted, it's not yet known whether the expected size is 8 or 9 bytes
		if(notesize == 9)
		{	//A variation of the note section is one that contains an extra byte of unknown data
			if(eof_filebuffer_get_byte(fb, &notemask))	//Read the extra data byte
			{	//If there was an error reading the next 1 byte value
				eof_log("\t\tError:  Could not read unknown data byte", 1);
				return -1;
			}
		}
	}
	return 1;
}

int eof_gh_read_sp_section(filebuffer *fb, EOF_SONG *sp, gh_section *target)
{
	unsigned long numphrases, phrasesize, dword, ctr;
	unsigned int length;

	if(!fb || !sp || !target)
		return 0;

#ifdef GH_IMPORT_DEBUG
	snprintf(eof_log_string, sizeof(eof_log_string), "\tGH:  Looking for section \"%s\"", target->name);
	eof_log(eof_log_string, 1);
#endif
	if(eof_filebuffer_find_checksum(fb, EOF_GH_CRC32(target->name)))	//Seek one byte past the target header
	{	//If the target section couldn't be found
		eof_log("\t\tCould not find section", 1);
		return 0;
	}
	if(eof_filebuffer_get_dword(fb, &numphrases))	//Read the number of sp phrases in the section
	{	//If there was an error reading the next 4 byte value
		eof_log("\t\tError:  Could not read number of phrases", 1);
		return -1;
	}
#ifdef GH_IMPORT_DEBUG
	snprintf(eof_log_string, sizeof(eof_log_string), "\t\tNumber of phrases = %lu", numphrases);
	eof_log(eof_log_string, 1);
#endif
	fb->index += 4;	//Seek past the next 4 bytes, which is a checksum for the game-specific sp section subheader
	if(!eof_gh_skip_size_def)
	{	//If the size field is expected
		if(eof_filebuffer_get_dword(fb, &phrasesize))	//Read the size of the star power entry
		{	//If there was an error reading the next 4 byte value
			eof_log("\t\tError:  Could not read note size", 1);
			return -1;
		}
		if(phrasesize != 6)
		{	//Each star power phrase is expected to be 6 bytes long
			eof_log("\t\tError:  Phrase size is not 6", 1);
			return -1;
		}
	}
	else
	{
		eof_log("\t\tSkipping field size", 1);
	}
	for(ctr = 0; ctr < numphrases; ctr++)
	{	//For each star power phrase in the section
		if(eof_filebuffer_get_dword(fb, &dword))	//Read the phrase position
		{	//If there was an error reading the next 4 byte value
			eof_log("\t\tError:  Could not read phrase position", 1);
			return -1;
		}
		if(eof_filebuffer_get_word(fb, &length))	//Read the phrase length
		{	//If there was an error reading the next 2 byte value
			eof_log("\t\tError:  Could not read phrase length", 1);
			return -1;
		}
		if(!eof_track_add_section(sp, target->tracknum, EOF_SP_SECTION, 0, dword, dword + length, 0, NULL))
		{	//If there was an error adding the section
			eof_log("\t\tError:  Could not add sp section", 1);
			return -1;
		}
	}
	return 1;
}

int eof_gh_read_tap_section(filebuffer *fb, EOF_SONG *sp, gh_section *target)
{
	unsigned long numphrases, phrasesize, dword, ctr, length;

	if(!fb || !sp || !target)
		return 0;

#ifdef GH_IMPORT_DEBUG
	snprintf(eof_log_string, sizeof(eof_log_string), "\tGH:  Looking for section \"%s\"", target->name);
	eof_log(eof_log_string, 1);
#endif
	if(eof_filebuffer_find_checksum(fb, EOF_GH_CRC32(target->name)))	//Seek one byte past the target header
	{	//If the target section couldn't be found
		eof_log("\t\tCould not find section", 1);
		return 0;
	}
	if(eof_filebuffer_get_dword(fb, &numphrases))	//Read the number of sp phrases in the section
	{	//If there was an error reading the next 4 byte value
		eof_log("\t\tError:  Could not read number of phrases", 1);
		return -1;
	}
#ifdef GH_IMPORT_DEBUG
	snprintf(eof_log_string, sizeof(eof_log_string), "\t\tNumber of phrases = %lu", numphrases);
	eof_log(eof_log_string, 1);
#endif
	fb->index += 4;	//Seek past the next 4 bytes, which is a checksum for the game-specific sp section subheader
	if(!eof_gh_skip_size_def)
	{	//If the size field is expected
		if(eof_filebuffer_get_dword(fb, &phrasesize))	//Read the size of the star power entry
		{	//If there was an error reading the next 4 byte value
			eof_log("\t\tError:  Could not read note size", 1);
			return -1;
		}
		if(phrasesize != 8)
		{	//Each tap phrase is expected to be 8 bytes long
			eof_log("\t\tError:  Phrase size is not 8", 1);
			return -1;
		}
	}
	else
	{
		eof_log("\t\tSkipping field size", 1);
	}
	for(ctr = 0; ctr < numphrases; ctr++)
	{	//For each tap phrase in the section
		if(eof_filebuffer_get_dword(fb, &dword))	//Read the phrase position
		{	//If there was an error reading the next 4 byte value
			eof_log("\t\tError:  Could not read phrase position", 1);
			return -1;
		}
		if(eof_filebuffer_get_dword(fb, &length))	//Read the phrase length
		{	//If there was an error reading the next 4 byte value
			eof_log("\t\tError:  Could not read phrase length", 1);
			return -1;
		}
		if(!eof_track_add_section(sp, target->tracknum, EOF_SLIDER_SECTION, 0, dword, dword + length, 0, NULL))
		{	//If there was an error adding the section
			eof_log("\t\tError:  Could not add tap section", 1);
			return -1;
		}
	}
	return 1;
}

int eof_gh_read_vocals(filebuffer *fb, EOF_SONG *sp)
{
	unsigned long ctr, ctr2, numvox, voxsize, voxstart, numlyrics, lyricsize, lyricstart, tracknum, phrasesize, numphrases, phrasestart, phraseend, prevphrase;
	unsigned long index1, index2;
	char *lyricbuffer = NULL, *lyricptr = NULL, *prevlyricptr = NULL;
	unsigned int voxlength;
	unsigned char voxpitch;
	unsigned char unicode_encoding = 0;	//Is set to nonzero if determined that the lyric text is in Unicode format
	EOF_LYRIC *ptr = NULL;
	EOF_VOCAL_TRACK * tp = NULL;

	if(!fb || !sp)
		return -1;

	tracknum = sp->track[EOF_TRACK_VOCALS]->tracknum;
#ifdef GH_IMPORT_DEBUG
	eof_log("\tGH:  Searching for vocals", 1);
#endif
	fb->index = 0;	//Seek to the beginning of the file buffer
	if(eof_filebuffer_find_checksum(fb, EOF_GH_CRC32("vocals")))	//Seek one byte past the target header
	{	//If the target section couldn't be found
		eof_log("\t\tCould not find section", 1);
		return 0;
	}
	if(eof_filebuffer_get_dword(fb, &numvox))	//Read the number of vox notes in the section
	{	//If there was an error reading the next 4 byte value
		eof_log("\t\tError:  Could not read number of vox notes", 1);
		return -1;
	}
#ifdef GH_IMPORT_DEBUG
	snprintf(eof_log_string, sizeof(eof_log_string), "\t\tNumber of vox notes = %lu", numvox);
	eof_log(eof_log_string, 1);
#endif
	fb->index += 4;	//Seek past the next 4 bytes, which is a checksum for the game-specific vox note section subheader
	if(!eof_gh_skip_size_def)
	{	//If the size field is expected
		if(eof_filebuffer_get_dword(fb, &voxsize))	//Read the size of the vox note entry
		{	//If there was an error reading the next 4 byte value
			eof_log("\t\tError:  Could not read vox note size", 1);
			return -1;
		}
		if(voxsize != 7)
		{	//Each vox note entry is expected to be 7 bytes long
			eof_log("\t\tError:  Vox note size is not 7", 1);
			return -1;
		}
	}
	else
	{
		eof_log("\t\tSkipping field size", 1);
	}
	for(ctr = 0; ctr < numvox; ctr++)
	{	//For each vox note in the section
		if(eof_filebuffer_get_dword(fb, &voxstart))	//Read the vox note position
		{	//If there was an error reading the next 4 byte value
			eof_log("\t\tError:  Could not vox note position", 1);
			return -1;
		}
		if(eof_filebuffer_get_word(fb, &voxlength))	//Read the vox note length
		{	//If there was an error reading the next 2 byte value
			eof_log("\t\tError:  Could not vox note length", 1);
			return -1;
		}
		if(eof_filebuffer_get_byte(fb, &voxpitch))	//Read the vox note pitch
		{	//If there was an error reading the next 1 byte value
			eof_log("\t\tError:  Could not vox note pitch", 1);
			return -1;
		}
#ifdef GH_IMPORT_DEBUG
		snprintf(eof_log_string, sizeof(eof_log_string), "\t\t\tVocal note:  Position = %lu, Length = %u, Pitch = %u", voxstart, voxlength, voxpitch);
		eof_log(eof_log_string, 1);
#endif
		if((voxpitch == 26) || (voxpitch == 2))
		{	//If this vox note is pitchless
			voxpitch = 0;	//Remap to EOF's pitchless value
		}
		else
		{	//Otherwise ensure it's within range
			while(voxpitch < 36)
			{	//Ensure the pitch isn't less than the RB minimum of 36
				voxpitch += 12;
			}
			while(voxpitch > 84)
			{	//Ensure the pitch isn't greater than the RB maximum of 84
				voxpitch -= 12;
			}
		}
		ptr = eof_track_add_create_note(sp, EOF_TRACK_VOCALS, voxpitch, voxstart, voxlength, 0, "+");	//Use "+" as a place holder because if the text is not defined, it should be considered a pitch shift

		if(!ptr)
		{	//If there was an error adding the lyric
			eof_log("\t\tError:  Could not add lyric section", 1);
			return -1;
		}
	}

	eof_track_sort_notes(sp, EOF_TRACK_VOCALS);
	fb->index = 0;	//Seek to the beginning of the file buffer
	if(eof_filebuffer_find_checksum(fb, EOF_GH_CRC32("vocallyrics")))	//Seek one byte past the target header
	{	//If the target section couldn't be found
		eof_log("\t\tCould not find section", 1);
		return 0;
	}
	if(eof_filebuffer_get_dword(fb, &numlyrics))	//Read the number of lyrics in the section
	{	//If there was an error reading the next 4 byte value
		eof_log("\t\tError:  Could not read number of lyrics", 1);
		return -1;
	}
#ifdef GH_IMPORT_DEBUG
	snprintf(eof_log_string, sizeof(eof_log_string), "\t\tNumber of lyrics = %lu", numlyrics);
	eof_log(eof_log_string, 1);
#endif
	fb->index += 4;	//Seek past the next 4 bytes, which is a checksum for the game-specific lyric section subheader
	if(!eof_gh_skip_size_def)
	{	//If the size field is expected
		if(eof_filebuffer_get_dword(fb, &lyricsize))	//Read the size of the lyric entry
		{	//If there was an error reading the next 4 byte value
			eof_log("\t\tError:  Could not read lyric size", 1);
			return -1;
		}
		if(lyricsize != 36)
		{	//Each lyric entry is expected to be 36 bytes long
			eof_log("\t\tError:  Lyric size is not 36", 1);
			return -1;
		}
	}
	else
	{
		if(fb->index + 5 >= fb->size)
		{	//Corrupted file
			eof_log("\t\tError:  Unexpected end of file", 1);
			return -1;
		}
		if((fb->buffer[fb->index + 4] == 0) && (fb->buffer[fb->index + 5] != 0))
		{	//Looking beyond the first timestamp, if the first two bytes of lyric text appear to be Unicode
			unicode_encoding = 1;
			lyricsize = 68;	//The Unicode version of GH strings entriesare 68 bytes each (32 Unicode characters plus one 4 byte timestamp)
			eof_log("\t\tUnicode text detected", 1);
		}
		else
		{	//Otherwise assume a size of 36 bytes
			lyricsize = 36;
			eof_log("\t\tSkipping field size", 1);
		}
	}
	lyricbuffer = (char *)malloc(lyricsize + 2);
	if(!lyricbuffer)
	{
		eof_log("\t\tError:  Could not allocate memory for lyric buffer", 1);
		return -1;
	}
	for(ctr = 0; ctr < numlyrics; ctr++)
	{	//For each lyric in the section
		if(eof_filebuffer_get_dword(fb, &lyricstart))	//Read the lyric start position
		{	//If there was an error reading the next 4 byte value
			eof_log("\t\tError:  Could not read lyric position", 1);
			return -1;
		}
		if(!unicode_encoding)
		{	//If these are not Unicode strings, just do a normal memory copy
			if(eof_filebuffer_memcpy(fb, lyricbuffer, lyricsize - 4))	//Read 4 bytes less to account for the entry's position
			{	//If there was an error reading the lyric entry
				eof_log("\t\tError:  Could not read lyric text", 1);
				return -1;
			}
		}
		else
		{
			if(eof_filebuffer_read_unicode_chars(fb, lyricbuffer, 32))
			{	//If there was an error reading 32 Unicode characters (68 bytes)
				eof_log("\t\tError:  Could not read lyric text", 1);
				return -1;
			}
		}
		lyricbuffer[lyricsize] = '\0';
		lyricbuffer[lyricsize + 1] = '\0';	//Ensure the last two bytes are NULL characters, in case this string is treated as Unicode
#ifdef GH_IMPORT_DEBUG
		snprintf(eof_log_string, sizeof(eof_log_string), "\t\t\tAdding lyric text:  Position = %lu, Lyric = \"%s\"",lyricstart, lyricbuffer);
		eof_log(eof_log_string, 1);
#endif

		for(ctr2 = 0; ctr2 < numvox; ctr2++)
		{	//For each vox note that was loaded earlier
			if(eof_get_note_pos(sp, EOF_TRACK_VOCALS, ctr2) == lyricstart)
			{	//If the position of this vox note matches the position of the lyric entry
#ifdef GH_IMPORT_DEBUG
				eof_log("\t\t\tFound matching vocal data", 1);
#endif
				lyricptr = sp->vocal_track[tracknum]->lyric[ctr2]->text;	//Get the string address for this lyric
				index1 = index2 = 0;	//Prepare to filter the equal sign out of the lyric text
				while(lyricbuffer[index1] != '\0')
				{	//For each character in the lyric
					if((lyricbuffer[index1] == '=') && (ctr2 > 0))
					{	//If this character is an equal sign, remove it and append a hyphen to the previous lyric (the equivalent RB notation)
						unsigned long length;
						prevlyricptr = sp->vocal_track[tracknum]->lyric[ctr2 - 1]->text;	//Get the string address for the previous lyric
						length = ustrlen(prevlyricptr);
						if((prevlyricptr[length - 1] != '-') && (prevlyricptr[length - 1] != '='))
						{	//If the previous lyric doesn't already end in a hyphen or equal sign
							if(length + 1 < EOF_MAX_LYRIC_LENGTH)
							{	//Bounds check
								prevlyricptr[length] = '-';			//Append a hyphen
								prevlyricptr[length + 1] = '\0';	//Re-terminate the string
							}
						}
					}
					else if((lyricbuffer[index1] == '-') && (lyricbuffer[index1 + 1] == '\0'))
					{	//If this is the last character in the string and it is a hyphen,
						lyricptr[index2++] = '=';	//Convert it to the equivalent RB notation
					}
					else
					{	//If this character isn't an equal sign
						lyricptr[index2++] = lyricbuffer[index1];	//Copy it into the final lyric string
					}
					index1++;	//Point to next character in the lyric
				}
				lyricptr[index2] = '\0';	//Ensure the string is terminated
				lyricptr[EOF_MAX_LYRIC_LENGTH - 1] = '\0';	//Ensure the last two bytes of the lyric text are NULL (Unicode terminator)
				lyricptr[EOF_MAX_LYRIC_LENGTH] = '\0';
				break;
			}
		}
#ifdef GH_IMPORT_DEBUG
		if(ctr2 >= numvox)
		{	//If the lyric entry didn't match the position of any vocal note
			eof_log("\t\t\t!Did not find matching vocal data", 1);
		}
#endif
	}//For each lyric in the section

	fb->index = 0;	//Seek to the beginning of the file buffer
	if(eof_filebuffer_find_checksum(fb, EOF_GH_CRC32("vocalphrase")))	//Seek one byte past the target header
	{	//If the target section couldn't be found
		eof_log("\t\tCould not find section", 1);
		return 0;
	}
	if(eof_filebuffer_get_dword(fb, &numphrases))	//Read the number of lyric phrases in the section
	{	//If there was an error reading the next 4 byte value
		eof_log("\t\tError:  Could not read number of lyric phrases", 1);
		return -1;
	}
#ifdef GH_IMPORT_DEBUG
	snprintf(eof_log_string, sizeof(eof_log_string), "\t\tNumber of lyric phrases = %lu", numphrases);
	eof_log(eof_log_string, 1);
#endif
	fb->index += 4;	//Seek past the next 4 bytes, which is a checksum for the game-specific lyric phrase section subheader
	if(!eof_gh_skip_size_def)
	{	//If the size field is expected
		if(eof_filebuffer_get_dword(fb, &phrasesize))	//Read the size of the lyric phrase entry
		{	//If there was an error reading the next 4 byte value
			eof_log("\t\tError:  Could not read lyric phrase size", 1);
			return -1;
		}
		if(phrasesize != 4)
		{	//Each lyric entry is expected to be 4 bytes long
			eof_log("\t\tError:  Lyric phrase size is not 4", 1);
			return -1;
		}
	}
	else
	{
		eof_log("\t\tSkipping field size", 1);
	}
	tp = sp->vocal_track[tracknum];		//Store the pointer to the vocal track to simplify things
	for(ctr = 0; ctr < numphrases; ctr++)
	{	//For each lyric phrase in the section
		if(eof_filebuffer_get_dword(fb, &phrasestart))	//Read the lyric start position
		{	//If there was an error reading the next 4 byte value
			eof_log("\t\tError:  Could not read lyric phrase position", 1);
			return -1;
		}
#ifdef GH_IMPORT_DEBUG
	snprintf(eof_log_string, sizeof(eof_log_string), "\t\tLyric phrase at %lu ms", phrasestart);
	eof_log(eof_log_string, 1);
#endif
		eof_vocal_track_add_line(tp, phrasestart, phrasestart);	//Add the phrase with a temporary end position
	}

	///Note:  This logic assumes the lyric lines are sorted in ascending order by timestamp
	///The lyrics themselves were previously sorted and can be expected to still be so
	prevphrase = 0;
	for(ctr = 0; ctr < tp->lines; ctr++)
	{	//For each lyric phrase that was added
#ifdef GH_IMPORT_DEBUG
		if((ctr > 0) && (tp->line[ctr].start_pos <= prevphrase))
		{	//If this phrase doesn't come after the previous phrase
			allegro_message("Error:  GH lyric phrases are not in order!");
		}
#endif
		prevphrase = tp->line[ctr].start_pos;
		phrasestart = phraseend = 0;
		for(ctr2 = 0; ctr2 < numvox; ctr2++)
		{	//For each vox note that was loaded earlier
			voxstart = eof_get_note_pos(sp, EOF_TRACK_VOCALS, ctr2);	//Store this position
			if(voxstart >= tp->line[ctr].start_pos)
			{	//If this lyric is at or after this line
				if(ctr + 1 < tp->lines)
				{	//If there is another line of lyrics
					if(voxstart < tp->line[ctr + 1].start_pos)
					{	//If this lyric is before the next lyric line
#ifdef GH_IMPORT_DEBUG
						if(tp->line[ctr].start_pos >= tp->line[ctr + 1].start_pos)
						{
							allegro_message("Error:  GH lyric phrases are not in order!");
						}
#endif
						phraseend = voxstart + eof_get_note_length(sp, EOF_TRACK_VOCALS, ctr2);		//Set the phrase end to the end of this lyric

						if(!phrasestart)
						{	//If this is the first lyric that is positioned at or after the start of this phrase
							phrasestart = voxstart;
						}
					}
					else
					{	//This lyric is in the next line
						break;
					}
				}
				else
				{	//This is the final line of lyrics
					phraseend = voxstart + eof_get_note_length(sp, EOF_TRACK_VOCALS, ctr2);		//Set the phrase end to the end of this lyric
				}
			}
		}
		if(phrasestart || phraseend)
		{	//If any lyrics fell within the phrase, adjust the phrase's timing
#ifdef GH_IMPORT_DEBUG
			snprintf(eof_log_string, sizeof(eof_log_string), "\t\tAltering lyric phrase at %lu ms to be %lu ms - %lu ms", tp->line[ctr].start_pos, phrasestart, phraseend);
			eof_log(eof_log_string, 1);
#endif
			tp->line[ctr].start_pos = phrasestart;	//Update the starting position of the lyric phrase
			tp->line[ctr].end_pos = phraseend;		//Update the ending position of the lyric phrase
		}
	}

	free(lyricbuffer);
	return 1;
}

EOF_SONG * eof_import_gh(const char * fn)
{
	EOF_SONG * sp;
	filebuffer *fb;
	unsigned long dword, ctr, ctr2, numbeats, numsigs, lastfretbar = 0, lastsig = 0, lastppqn = 0;
	unsigned char tsnum, tsden;
	char oggfn[1024] = {0};

	eof_log("\tImporting Guitar Hero chart", 1);
	eof_log("eof_import_gh() entered", 1);

//Load the GH file into memory
	fb = eof_filebuffer_load(fn);
	if(fb == NULL)
	{
		eof_log("Error:  Failed to buffer GH file", 1);
		return NULL;
	}

//Process the fretbar section to create the tempo map
	eof_gh_skip_size_def = 0;	//Reset this condition
	if(eof_filebuffer_find_checksum(fb, EOF_GH_CRC32("fretbar")))	//Seek one byte past the "fretbar" header
	{	//If the "fretbar" section couldn't be found
		eof_log("Error:  Failed to locate \"fretbar\" header", 1);
		eof_filebuffer_close(fb);
		return NULL;
	}
	if(eof_filebuffer_get_dword(fb, &numbeats))	//Read the number of fretbars
	{	//If there was an error reading the next 4 byte value
		eof_log("Error:  Could not read number of fretbars", 1);
		eof_filebuffer_close(fb);
		return NULL;
	}
#ifdef GH_IMPORT_DEBUG
		snprintf(eof_log_string, sizeof(eof_log_string), "\tGH:  Number of fretbars = %lu", numbeats);
		eof_log(eof_log_string, 1);
#endif
	if(numbeats < 3)
	{
		eof_log("Error:  Invalid number of fretbars", 1);
		eof_filebuffer_close(fb);
		return NULL;
	}
	fb->index += 4;	//Seek past the next 4 bytes, which is a checksum for the game-specific fretbar subheader
	if(eof_filebuffer_get_dword(fb, &dword))	//Read the size of the fretbar value
	{	//If there was an error reading the next 4 byte value
		eof_log("Error:  Could not read fretbar size", 1);
		eof_filebuffer_close(fb);
		return NULL;
	}
	if(dword == 0)
	{	//I've noticed that some files skip defining the size and go immediately to defining the first fretbar position
		eof_gh_skip_size_def = 1;	//Note that the size field in the header is omitted
		eof_log("\t\tThe size fields for this file appear to be omitted", 1);
		fb->index -= 4;	//Rewind 4 bytes so that the fretbar loop below reads the first position correctly
	}
	else if(dword != 4)
	{	//Otherwise each fretbar is expected to be 4 bytes long
		eof_log("Error:  Fretbar size is not 4", 1);
		eof_filebuffer_close(fb);
		return NULL;
	}
	sp = eof_create_song_populated();
	if(sp == NULL)
	{	//Couldn't create new song
		eof_log("Error:  Could not create new chart", 1);
		eof_filebuffer_close(fb);
		return NULL;
	}
	if(!eof_song_resize_beats(sp, numbeats))	//Add the number of beats specified in the chart file
	{	//Couldn't resize beat array
		eof_log("Error:  Could not resize beat array", 1);
		eof_filebuffer_close(fb);
		eof_destroy_song(sp);
		return NULL;
	}
	for(ctr = 0; ctr < numbeats; ctr++)
	{	//For each fretbar in the chart file
		if(eof_filebuffer_get_dword(fb, &dword))
		{	//If there was an error reading the next 4 byte value
			eof_log("Error:  Could not read fretbar", 1);
			eof_destroy_song(sp);
			eof_filebuffer_close(fb);
			return NULL;
		}
#ifdef GH_IMPORT_DEBUG
		snprintf(eof_log_string, sizeof(eof_log_string), "\tGH:  Fretbar at %lums", dword);
		eof_log(eof_log_string, 1);
#endif
		if(ctr && (dword <= lastfretbar))
		{	//If this beat doesn't come after the previous beat
			allegro_message("Warning:  Corrupt fretbar position");
			numbeats = ctr;	//Update the number of beats
			if(numbeats < 3)
			{	//If too few beats were usable
				eof_destroy_song(sp);
				eof_filebuffer_close(fb);
				return NULL;
			}
		}
		else
		{
			sp->beat[ctr]->pos = sp->beat[ctr]->fpos = dword;	//Set the timestamp position of this beat
			lastfretbar = dword;
		}
	}
	for(ctr = 0; ctr < numbeats - 1; ctr++)
	{	//For each beat in the chart
		sp->beat[ctr]->ppqn = 1000 * (sp->beat[ctr + 1]->pos - sp->beat[ctr]->pos);	//Calculate the tempo of the beat by getting its length
		if(!lastppqn || (lastppqn != sp->beat[ctr]->ppqn))
		{	//If the tempo is being changed at this beat, or this is the first beat
#ifdef GH_IMPORT_DEBUG
		snprintf(eof_log_string, sizeof(eof_log_string), "\tGH:  Tempo change at %lums (%fBPM)", sp->beat[ctr]->pos, 60000000.0 / sp->beat[ctr]->ppqn);
		eof_log(eof_log_string, 1);
#endif
			sp->beat[ctr]->flags |= EOF_BEAT_FLAG_ANCHOR;	//Set the anchor flag
			lastppqn = sp->beat[ctr]->ppqn;
		}
	}
	sp->beat[numbeats - 1]->ppqn = sp->beat[numbeats - 2]->ppqn;	//The last beat's tempo is the same as the previous beat's

	if(eof_use_ts)
	{	//If the user opted to import TS changes
//Process the timesig section to load time signatures
		fb->index = 0;	//Seek to the beginning of the file buffer
		if(eof_filebuffer_find_checksum(fb, EOF_GH_CRC32("timesig")))	//Seek one byte past the "fretbar" header
		{	//If the "fretbar" section couldn't be found
			eof_log("Error:  Failed to locate \"timesig\" header", 1);
			eof_destroy_song(sp);
			eof_filebuffer_close(fb);
			return NULL;
		}
		if(eof_filebuffer_get_dword(fb, &numsigs))	//Read the number of time signatures
		{	//If there was an error reading the next 4 byte value
			eof_log("Error:  Could not read number of time signatures", 1);
			eof_destroy_song(sp);
			eof_filebuffer_close(fb);
			return NULL;
		}
#ifdef GH_IMPORT_DEBUG
			snprintf(eof_log_string, sizeof(eof_log_string), "\tGH:  Number of time signatures = %lu", numsigs);
			eof_log(eof_log_string, 1);
#endif
		fb->index += 4;	//Seek past the next 4 bytes, which is a checksum for the game-specific timesig subheader
		if(!eof_gh_skip_size_def)
		{	//If the size field is expected
			if(eof_filebuffer_get_dword(fb, &dword))	//Read the size of the time signature value
			{	//If there was an error reading the next 4 byte value
				eof_log("Error:  Could not read time signature size", 1);
				eof_destroy_song(sp);
				eof_filebuffer_close(fb);
				return NULL;
			}
			if(dword == 0)
			{	//I've noticed that some files skip defining the size and go immediately to defining the first timesig position
				eof_gh_skip_size_def = 1;	//Note that the size field in the header is omitted
				eof_log("\t\tThe size fields for this file appear to be omitted", 1);
				fb->index -= 4;	//Rewind 4 bytes so that the timesig loop below reads the first position correctly
			}
			else if(dword != 6)
			{	//Otherwise each time signature is expected to be 6 bytes long
				eof_log("Error:  Time signature size is not 6", 1);
				eof_destroy_song(sp);
				eof_filebuffer_close(fb);
				return NULL;
			}
		}
		else
		{
			eof_log("\t\tSkipping field size", 1);
		}
		for(ctr = 0; ctr < numsigs; ctr++)
		{	//For each time signature in the chart file
			if(eof_filebuffer_get_dword(fb, &dword))
			{	//If there was an error reading the next 4 byte value
				eof_log("Error:  Could not read time signature position", 1);
				eof_destroy_song(sp);
				eof_filebuffer_close(fb);
				return NULL;
			}
			if(ctr && (dword <= lastsig))
			{	//If this beat doesn't come after the previous beat
				eof_log("Error:  Corrupted time signature", 1);
				eof_destroy_song(sp);
				eof_filebuffer_close(fb);
				return NULL;
			}
			if(eof_filebuffer_get_byte(fb, &tsnum) || eof_filebuffer_get_byte(fb, &tsden))
			{	//If there was an error reading the next 2 bytes (numerator and denominator
				eof_log("Error:  Could not read time signature", 1);
				eof_destroy_song(sp);
				eof_filebuffer_close(fb);
				return NULL;
			}
#ifdef GH_IMPORT_DEBUG
			snprintf(eof_log_string, sizeof(eof_log_string), "\tGH:  Time signature at %lums (%d/%d)", dword, tsnum, tsden);
			eof_log(eof_log_string, 1);
#endif
			for(ctr2 = 0; ctr2 < sp->beats; ctr2++)
			{	//For each beat in the song
				if(dword == sp->beat[ctr2]->pos)
				{	//If this time signature is positioned at this beat marker
					eof_apply_ts(tsnum,tsden,ctr2,sp,0);	//Apply the signature
					break;
				}
				else if(dword < sp->beat[ctr2]->pos)
				{	//Otherwise if this time signature's position has been surpassed by a beat
					allegro_message("Error:  Mid beat time signature detected.  Skipping");
				}
			}

			lastsig = dword;
		}
	}//If the user opted to import TS changes

//Read instrument tracks
	for(ctr = 0; ctr < EOF_NUM_GH_INSTRUMENT_SECTIONS; ctr++)
	{	//For each known guitar hero instrument difficulty section
		fb->index = 0;	//Rewind to beginning of file buffer
		eof_gh_read_instrument_section(fb, sp, &eof_gh_instrument_sections[ctr]);	//Import notes from the section
	}

//Read star power sections
	for(ctr = 0; ctr < EOF_NUM_GH_SP_SECTIONS; ctr++)
	{	//For each known guitar hero star power section
		fb->index = 0;	//Rewind to beginning of file buffer
		eof_gh_read_sp_section(fb, sp, &eof_gh_sp_sections[ctr]);	//Import star power section
	}

//Read tap (slider) sections
	for(ctr = 0; ctr < EOF_NUM_GH_TAP_SECTIONS; ctr++)
	{	//For each known guitar hero tap section
		fb->index = 0;	//Rewind to beginning of file buffer
		eof_gh_read_tap_section(fb, sp, &eof_gh_tap_sections[ctr]);	//Import tap section
	}

//Read vocal track
	eof_gh_read_vocals(fb, sp);

//Load an audio file
	replace_filename(oggfn, fn, "guitar.ogg", 1024);	//Try to load guitar.ogg in the GH file's folder by default
	if(!eof_load_ogg(oggfn))
	{
		eof_destroy_song(sp);
		return NULL;
	}
	eof_music_length = alogg_get_length_msecs_ogg(eof_music_track);
	eof_music_actual_length = eof_music_length;

	eof_vocal_track_fixup_lyrics(sp, EOF_TRACK_VOCALS, 0);	//Clean up the lyrics

	eof_filebuffer_close(fb);	//Close the file buffer
	eof_log("\tGH import completed", 1);
	return sp;
}
