#include "beat.h"
#include "gh_import.h"
#include "main.h"
#include "midi.h"	//For eof_apply_ts()
#include "utility.h"
#include "foflc/Lyric_storage.h"	//For strcasestr_spec()

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

unsigned long crc32_lookup[256] = {0};	//A lookup table to improve checksum calculation performance
char crc32_lookup_initialized = 0;	//Is set to nonzero when the lookup table is created
char eof_gh_skip_size_def = 0;	//Is set to nonzero if it's determined that the size field in GH headers is omitted

#define GH_IMPORT_DEBUG

#define EOF_NUM_GH_INSTRUMENT_SECTIONS_NOTE 12
gh_section eof_gh_instrument_sections_note[EOF_NUM_GH_INSTRUMENT_SECTIONS_NOTE] =
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

#define EOF_NUM_GH_INSTRUMENT_SECTIONS_QB 24
gh_section eof_gh_instrument_sections_qb[EOF_NUM_GH_INSTRUMENT_SECTIONS_QB] =
{
	{"_song_easy", EOF_TRACK_GUITAR, EOF_NOTE_SUPAEASY},
	{"_song_medium", EOF_TRACK_GUITAR, EOF_NOTE_EASY},
	{"_song_hard", EOF_TRACK_GUITAR, EOF_NOTE_MEDIUM},
	{"_song_expert", EOF_TRACK_GUITAR, EOF_NOTE_AMAZING},
	{"_song_rhythm_easy", EOF_TRACK_BASS, EOF_NOTE_SUPAEASY},
	{"_song_rhythm_medium", EOF_TRACK_BASS, EOF_NOTE_EASY},
	{"_song_rhythm_hard", EOF_TRACK_BASS, EOF_NOTE_MEDIUM},
	{"_song_rhythm_expert", EOF_TRACK_BASS, EOF_NOTE_AMAZING},
	{"_song_guitarcoop_easy", EOF_TRACK_GUITAR_COOP, EOF_NOTE_SUPAEASY},
	{"_song_guitarcoop_medium", EOF_TRACK_GUITAR_COOP, EOF_NOTE_EASY},
	{"_song_guitarcoop_hard", EOF_TRACK_GUITAR_COOP, EOF_NOTE_MEDIUM},
	{"_song_guitarcoop_expert", EOF_TRACK_GUITAR_COOP, EOF_NOTE_AMAZING},
	{"_song_rhythmcoop_easy", EOF_TRACK_RHYTHM, EOF_NOTE_SUPAEASY},
	{"_song_rhythmcoop_medium", EOF_TRACK_RHYTHM, EOF_NOTE_EASY},
	{"_song_rhythmcoop_hard", EOF_TRACK_RHYTHM, EOF_NOTE_MEDIUM},
	{"_song_rhythmcoop_expert", EOF_TRACK_RHYTHM, EOF_NOTE_AMAZING},
	{"_song_drum_easy", EOF_TRACK_DRUM, EOF_NOTE_SUPAEASY},
	{"_song_drum_medium", EOF_TRACK_DRUM, EOF_NOTE_EASY},
	{"_song_drum_hard", EOF_TRACK_DRUM, EOF_NOTE_MEDIUM},
	{"_song_drum_expert", EOF_TRACK_DRUM, EOF_NOTE_AMAZING},
	{"_song_aux_easy", EOF_TRACK_KEYS, EOF_NOTE_SUPAEASY},
	{"_song_aux_medium", EOF_TRACK_KEYS, EOF_NOTE_EASY},
	{"_song_aux_hard", EOF_TRACK_KEYS, EOF_NOTE_MEDIUM},
	{"_song_aux_expert", EOF_TRACK_KEYS, EOF_NOTE_AMAZING}
};	//QB format GH files prefix each section name with the song name, so these will be treated as section name suffixes

#define EOF_NUM_GH_SP_SECTIONS_NOTE 3
gh_section eof_gh_sp_sections_note[EOF_NUM_GH_SP_SECTIONS_NOTE] =
{
	{"guitarexpertstarpower", EOF_TRACK_GUITAR, 0},
	{"bassexpertstarpower", EOF_TRACK_BASS, 0},
	{"drumsexpertstarpower", EOF_TRACK_DRUM, 0}
};

#define EOF_NUM_GH_SP_SECTIONS_QB 6
gh_section eof_gh_sp_sections_qb[EOF_NUM_GH_SP_SECTIONS_QB] =
{
	{"_expert_star", EOF_TRACK_GUITAR, 0},
	{"_rhythm_expert_star", EOF_TRACK_BASS, 0},
	{"_guitarcoop_expert_star", EOF_TRACK_GUITAR_COOP, 0},
	{"_rhythmcoop_expert_star", EOF_TRACK_RHYTHM, 0},
	{"_drum_expert_star", EOF_TRACK_DRUM, 0},
	{"_aux_expert_star", EOF_TRACK_KEYS, 0}
};

#define EOF_NUM_GH_TAP_SECTIONS_NOTE 2
gh_section eof_gh_tap_sections_note[EOF_NUM_GH_TAP_SECTIONS_NOTE] =
{
	{"guitarexperttapping", EOF_TRACK_GUITAR, 0},
	{"bassexperttapping", EOF_TRACK_BASS, 0}
};

#define EOF_NUM_GH_TAP_SECTIONS_QB 4
gh_section eof_gh_tap_sections_qb[EOF_NUM_GH_TAP_SECTIONS_QB] =
{
	{"_expert_tapping", EOF_TRACK_GUITAR, 0},
	{"_rhythm_expert_tapping", EOF_TRACK_BASS, 0},
	{"_guitarcoop_expert_tapping", EOF_TRACK_GUITAR_COOP, 0},
	{"_rhythmcoop_expert_tapping", EOF_TRACK_RHYTHM, 0}
};

struct QBlyric
{
	unsigned long checksum;
	char *text;
	struct QBlyric *next;
};	//QB format Guitar Hero files store lyric information in different sections.  One contains timestamps paired with lyric checksums,
	//another contains the actual lyrics with their respective checksums.  A linked list will be built to store the latter section's entries

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

unsigned long eof_crc32(const char *string)
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

unsigned long eof_gh_checksum(const char *string)
{
	unsigned long checksum = 0;

	if(string)
	{
		checksum = eof_crc32(string) ^ 0xFFFFFFFF;
#ifdef GH_IMPORT_DEBUG
		snprintf(eof_log_string, sizeof(eof_log_string), "\tGH:  Checksum of \"%s\" is 0x%08lX", string, checksum);
		eof_log(eof_log_string, 1);
#endif
	}
	return checksum;
}

int eof_filebuffer_find_checksum(filebuffer *fb, unsigned long checksum)
{
	unsigned char checksumarray[4];

	checksumarray[0] = checksum >> 24;	//Store the checksum into an array to simplify the search logic
	checksumarray[1] = (checksum & 0xFF0000) >> 16;
	checksumarray[2] = (checksum & 0xFF00) >> 8;
	checksumarray[3] = (checksum & 0xFF);

	return (eof_filebuffer_find_bytes(fb, checksumarray, 4, 2) != 1);	//Seek to the byte that follows the checksum, convert the return value to return 0 on match
}

int eof_filebuffer_find_bytes(filebuffer *fb, const void *bytes, unsigned long searchlen, unsigned char seektype)
{
	unsigned long originalpos, matchpos = 0, index = 0;
	unsigned char *target, success = 0;

	if(!fb || !bytes)
		return -1;	//Return error
	if(!searchlen)
		return 0;	//There will be no matches to an empty search

	originalpos = fb->index;
	target = (unsigned char *)bytes;
	while(fb->index < fb->size)
	{	//While the end of buffer hasn't been reached
		if(fb->buffer[fb->index] == target[index])
		{	//If the next byte of the target has been matched
			if(index == 0)
			{	//The match was the first byte of the target
				matchpos = fb->index;
			}
			index++;	//Next pass of the loop will look for the next byte in the target
			if(index == searchlen)
			{	//If all bytes have been matched
				success = 1;
				break;
			}
		}
		else
		{	//The byte did not match
			index = 0;	//Reset the index to look for a match to the target's first byte
		}
		fb->index++;	//Increment to next byte in the buffer
	}

	if(success)
	{	//If a match was found
		if(seektype == 1)
		{	//Seek to beginning of match
			fb->index = matchpos;
		}
		else
		{	//Otherwise seek past the last byte of the match
			fb->index++;
		}
		return 1;	//Return match (if seektype was not 1, the buffer position is left at the byte following the match
	}

	fb->index = originalpos;	//Restore the original buffer position
	return 0;	//Return no match
}

int eof_gh_read_instrument_section_note(filebuffer *fb, EOF_SONG *sp, gh_section *target)
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
	if(eof_filebuffer_find_checksum(fb, eof_gh_checksum(target->name)))	//Seek one byte past the target header
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

int eof_gh_read_sp_section_note(filebuffer *fb, EOF_SONG *sp, gh_section *target)
{
	unsigned long numphrases, phrasesize, dword, ctr;
	unsigned int length;

	if(!fb || !sp || !target)
		return 0;

#ifdef GH_IMPORT_DEBUG
	snprintf(eof_log_string, sizeof(eof_log_string), "\tGH:  Looking for section \"%s\"", target->name);
	eof_log(eof_log_string, 1);
#endif
	if(eof_filebuffer_find_checksum(fb, eof_gh_checksum(target->name)))	//Seek one byte past the target header
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

int eof_gh_read_tap_section_note(filebuffer *fb, EOF_SONG *sp, gh_section *target)
{
	unsigned long numphrases, phrasesize, dword, ctr, length;

	if(!fb || !sp || !target)
		return 0;

#ifdef GH_IMPORT_DEBUG
	snprintf(eof_log_string, sizeof(eof_log_string), "\tGH:  Looking for section \"%s\"", target->name);
	eof_log(eof_log_string, 1);
#endif
	if(eof_filebuffer_find_checksum(fb, eof_gh_checksum(target->name)))	//Seek one byte past the target header
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

void eof_process_gh_lyric_text(EOF_SONG *sp)
{
	unsigned long ctr, ctr2, length, prevlength, index;
	char *string, *prevstring, buffer[EOF_MAX_LYRIC_LENGTH+1];

	for(ctr = 0; ctr < eof_get_track_size(sp, EOF_TRACK_VOCALS); ctr++)
	{	//For each lyric
		index = 0;	//Reset the index into the buffer
		string = eof_get_note_name(sp, EOF_TRACK_VOCALS, ctr);
		if(string == NULL)
			return;	//If there is some kind of error
		length = ustrlen(string);

		for(ctr2 = 0; ctr2 < length; ctr2++)
		{	//For each character in the the lyric's text
			if((string[ctr2] == '=') && (ctr > 0))
			{	//If this character is an equal sign, and there's a previous lyric, drop the character and append a hyphen to the previous lyric
				prevstring = eof_get_note_name(sp, EOF_TRACK_VOCALS, ctr - 1);	//Get the string for the previous lyric
				if(prevstring != NULL)
				{	//If there wasn't an error getting that string
					prevlength = ustrlen(prevstring);
					if((prevstring[prevlength - 1] != '-') && (prevstring[prevlength - 1] != '='))
					{	//If the previous lyric doesn't already end in a hyphen or equal sign
						if(prevlength + 1 < EOF_MAX_LYRIC_LENGTH)
						{	//Bounds check
							prevstring[prevlength] = '-';		//Append a hyphen
							prevstring[prevlength + 1] = '\0';	//Re-terminate the string
						}
					}
				}
			}
			else if((string[ctr2] == '-') && (ctr2 + 1 == length))
			{	//If this is the last character in the string and it is a hyphen
				buffer[index++] = '=';	//Convert it to the equivalent RB notation
			}
			else
			{	//This character isn't an equal sign or a hyphen at the end of the string
				buffer[index++] = string[ctr2];	//Copy it as-is
			}
		}
		buffer[index] = '\0';				//Terminate the finalized string
		buffer[EOF_MAX_LYRIC_LENGTH - 1] = '\0';	//Ensure the last two bytes of the lyric text are NULL (Unicode terminator)
		buffer[EOF_MAX_LYRIC_LENGTH] = '\0';
		eof_set_note_name(sp, EOF_TRACK_VOCALS, ctr, buffer);	//Update the string on the lyric
	}
}

void eof_process_gh_lyric_phrases(EOF_SONG *sp)
{
	unsigned long ctr, ctr2, tracknum, phrasestart, phraseend, voxstart, prevphrase = 0;
	EOF_VOCAL_TRACK * tp = NULL;

	if(sp == NULL)
		return;

	tracknum = sp->track[EOF_TRACK_VOCALS]->tracknum;
	tp = sp->vocal_track[tracknum];		//Store the pointer to the vocal track to simplify things

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
		for(ctr2 = 0; ctr2 < eof_get_track_size(sp, EOF_TRACK_VOCALS); ctr2++)
		{	//For each lyric
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
}

int eof_gh_read_vocals_note(filebuffer *fb, EOF_SONG *sp)
{
	unsigned long ctr, ctr2, numvox, voxsize, voxstart, numlyrics, lyricsize, lyricstart, tracknum, phrasesize, numphrases, phrasestart;
	char *lyricbuffer = NULL;
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
	if(eof_filebuffer_find_checksum(fb, eof_gh_checksum("vocals")))	//Seek one byte past the target header
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
	if(eof_filebuffer_find_checksum(fb, eof_gh_checksum("vocallyrics")))	//Seek one byte past the target header
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
				eof_set_note_name(sp, EOF_TRACK_VOCALS, ctr2, lyricbuffer);	//Apply the lyric text
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
	eof_process_gh_lyric_text(sp);	//Filter and convert hyphenating characters where appropriate

	fb->index = 0;	//Seek to the beginning of the file buffer
	if(eof_filebuffer_find_checksum(fb, eof_gh_checksum("vocalphrase")))	//Seek one byte past the target header
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
	eof_process_gh_lyric_phrases(sp);	//Create proper end positions for each lyric phrase

	free(lyricbuffer);
	return 1;
}

EOF_SONG * eof_import_gh(const char * fn)
{
	EOF_SONG * sp;
	char oggfn[1024] = {0};

	eof_log("\tImporting Guitar Hero chart", 1);
	eof_log("eof_import_gh() entered", 1);

	sp = eof_import_gh_note(fn);	//Attempt to load as a "NOTE" format GH file
	if(!sp)
	{	//If that failed
		sp = eof_import_gh_qb(fn);	//Attempt to load as a "QB" format GH file
	}
	if(sp)
	{	//If a GH file was loaded
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
		eof_log("\tGH import completed", 1);
	}
	return sp;
}

EOF_SONG * eof_import_gh_note(const char * fn)
{
	EOF_SONG * sp;
	filebuffer *fb;
	unsigned long dword, ctr, ctr2, numbeats, numsigs, lastfretbar = 0, lastsig = 0;
	unsigned char tsnum, tsden;

//Load the GH file into memory
	fb = eof_filebuffer_load(fn);
	if(fb == NULL)
	{
		eof_log("Error:  Failed to buffer GH file", 1);
		return NULL;
	}

//Process the fretbar section to create the tempo map
	eof_gh_skip_size_def = 0;	//Reset this condition
	if(eof_filebuffer_find_checksum(fb, eof_gh_checksum("fretbar")))	//Seek one byte past the "fretbar" header
	{	//If the "fretbar" section couldn't be found
		eof_filebuffer_close(fb);
		eof_log("Error:  Failed to locate \"fretbar\" header, this is not a \"NOTE\" format Guitar Hero file", 1);
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

	eof_calculate_tempo_map(sp);	//Build the tempo map based on the beat time stamps
	if(eof_use_ts)
	{	//If the user opted to import TS changes
//Process the timesig section to load time signatures
		fb->index = 0;	//Seek to the beginning of the file buffer
		if(eof_filebuffer_find_checksum(fb, eof_gh_checksum("timesig")))	//Seek one byte past the "fretbar" header
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
			{	//If this signature doesn't come after the previous signature
				eof_log("Error:  Corrupted time signature", 1);
				eof_destroy_song(sp);
				eof_filebuffer_close(fb);
				return NULL;
			}
			if(eof_filebuffer_get_byte(fb, &tsnum) || eof_filebuffer_get_byte(fb, &tsden))
			{	//If there was an error reading the next 2 bytes (numerator and denominator)
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
	for(ctr = 0; ctr < EOF_NUM_GH_INSTRUMENT_SECTIONS_NOTE; ctr++)
	{	//For each known guitar hero instrument difficulty section
		fb->index = 0;	//Rewind to beginning of file buffer
		eof_gh_read_instrument_section_note(fb, sp, &eof_gh_instrument_sections_note[ctr]);	//Import notes from the section
	}

//Read star power sections
	for(ctr = 0; ctr < EOF_NUM_GH_SP_SECTIONS_NOTE; ctr++)
	{	//For each known guitar hero star power section
		fb->index = 0;	//Rewind to beginning of file buffer
		eof_gh_read_sp_section_note(fb, sp, &eof_gh_sp_sections_note[ctr]);	//Import star power section
	}

//Read tap (slider) sections
	for(ctr = 0; ctr < EOF_NUM_GH_TAP_SECTIONS_NOTE; ctr++)
	{	//For each known guitar hero tap section
		fb->index = 0;	//Rewind to beginning of file buffer
		eof_gh_read_tap_section_note(fb, sp, &eof_gh_tap_sections_note[ctr]);	//Import tap section
	}

//Read vocal track
	eof_gh_read_vocals_note(fb, sp);
	eof_filebuffer_close(fb);	//Close the file buffer

	return sp;
}

unsigned long eof_gh_process_section_header(filebuffer *fb, const char *sectionname, unsigned long **arrayptr, unsigned long qbindex)
{
	unsigned long dword, arraysize, size, ctr;

	if(!fb || !sectionname || !arrayptr)
		return 0;	//Return error

	if(eof_filebuffer_find_checksum(fb, eof_gh_checksum(sectionname)))	//Seek one byte past the requested section header
	{	//If the section couldn't be found
		snprintf(eof_log_string, sizeof(eof_log_string), "Error:  Failed to locate \"%s\" header.", sectionname);
		eof_log(eof_log_string, 1);
		return 0;
	}
	fb->index += 4;	//Seek past the next 4 bytes, which is a checksum for the file name
	if(eof_filebuffer_get_dword(fb, &dword))
	{	//If there was an error reading the next 4 byte value
		eof_log("Error:  Could not read section's data offset", 1);
		return 0;
	}
	fb->index += 4;	//Seek past the next 4 bytes, which is padding
	if(fb->index - qbindex != dword)
	{	//If the data doesn't begin after the padding
		snprintf(eof_log_string, sizeof(eof_log_string), "\tGH:  Data is offset to position 0x%08lX within the QB file", dword);
		eof_log(eof_log_string, 1);
		fb->index = qbindex + dword;	//Seek to the appropriate buffer position
	}
	if(eof_filebuffer_get_dword(fb, &dword))
	{	//If there was an error reading the next 4 byte value
		eof_log("Error:  Could not read section's storage chunk type", 1);
		return 0;
	}
#ifdef GH_IMPORT_DEBUG
	snprintf(eof_log_string, sizeof(eof_log_string), "\tGH:  \tSection storage chunk type is 0x%08lX", dword);
	eof_log(eof_log_string, 1);
	if(dword == 0x00010000)
	{	//Empty section
		eof_log("\tGH:  \t\tEmpty section", 1);
	}
#endif
	if(eof_filebuffer_get_dword(fb, &size))
	{	//If there was an error reading the next 4 byte value
		eof_log("Error:  Could not read the size of this storage chunk", 1);
		return 0;
	}
	if(dword == 0x00010000)
	{	//Empty section
		arraysize = 0;
	}
	else if((dword == 0x00010A00) || (dword == 0x00010c00) || (dword == 0x00010100))
	{
//Allocate array to store 1D array offsets
		if(dword == 0x00010100)
		{	//A 1D array
			arraysize = 1;
		}
		else
		{	//A 2D array
			arraysize = size;
		}
		*arrayptr = (unsigned long *)malloc(sizeof(unsigned long) * arraysize);	//Allocate storage for the offsets pointing to 1D arrays of data
		if(!(*arrayptr))
		{
			eof_log("Error:  Could not allocate memory", 1);
			return 0;
		}

//Store 1D array offsets
		if(dword == 0x00010100)
		{	//A 1D array
			(*arrayptr)[0] = fb->index - qbindex - 8;	//Store the QB position of this 1D array's header
#ifdef GH_IMPORT_DEBUG
			snprintf(eof_log_string, sizeof(eof_log_string), "\tGH:  \t1D array at QB index 0x%08lX (file position 0x%lX)", fb->index - qbindex - 12, fb->index - 12);
			eof_log(eof_log_string, 1);
#endif
		}
		else
		{	//A 2D array
			if(arraysize > 1)
			{	//If there is more than one element in the 2D array, the next 4 bytes is the offset to the first element
				if(eof_filebuffer_get_dword(fb, &dword))
				{	//If there was an error reading the next 4 byte value
					eof_log("Error:  Could not read the offset to the 2D array data", 1);
					free(*arrayptr);
					return 0;
				}
				if(fb->index - qbindex != dword)
				{	//If the data doesn't begin after the padding
					snprintf(eof_log_string, sizeof(eof_log_string), "\tGH:  2D array data is offset to position 0x%08lX within the QB file", dword);
					eof_log(eof_log_string, 1);
					fb->index = qbindex + dword;	//Seek to the appropriate buffer position
				}
			}
			for(ctr = 0; ctr < arraysize; ctr++)
			{	//For each 1D array offset
				if(eof_filebuffer_get_dword(fb, &((*arrayptr)[ctr])))
				{	//If there was an error reading the next 4 byte value
					eof_log("Error:  Could not read the offset of this 1D array", 1);
					free(*arrayptr);
					return 0;
				}
#ifdef GH_IMPORT_DEBUG
				if(dword == 0x00010c00)
				{	//2D array
					snprintf(eof_log_string, sizeof(eof_log_string), "\tGH:  \t\t1D array at QB index 0x%08lX (file position 0x%lX)", (*arrayptr)[ctr], (*arrayptr)[ctr] + qbindex);
				}
				else
				{	//Alternate 2D array
					snprintf(eof_log_string, sizeof(eof_log_string), "\tGH:  \t\tData at QB index 0x%08lX (file position 0x%lX)", (*arrayptr)[ctr], (*arrayptr)[ctr] + qbindex);
				}
				eof_log(eof_log_string, 1);
#endif
			}
		}
	}
	else
	{	//Unsupported type
		eof_log("\t\tError:  Unsupported section storage chunk type", 1);
		free(*arrayptr);
		return 0;
	}

	return arraysize;
}

unsigned long eof_gh_read_array_header(filebuffer *fb, unsigned long qbpos, unsigned long qbindex)
{
	unsigned long size, offset;

	if(!fb)
		return 0;

	fb->index = qbindex + qbpos + 4;	//Seek four bytes past the specified array header
	if(eof_filebuffer_get_dword(fb, &size))	//Read the size of the array
	{	//If there was an error reading the next 4 byte value
		eof_log("Error:  Could not read array size", 1);
		return 0;
	}
	if(eof_filebuffer_get_dword(fb, &offset))	//Read the offset of the array data
	{	//If there was an error reading the next 4 byte value
		eof_log("Error:  Could not read data offset", 1);
		return 0;
	}
	fb->index = qbindex + offset;	//Seek to the specified buffer position (the first element of data in the array)

	return size;
}

unsigned long eof_char_to_binary(unsigned char input)
{
	unsigned char ctr, bitmask;
	unsigned long retval = 0;

	for(ctr = 0, bitmask = 128; ctr < 8; ctr++, bitmask >>= 1)
	{	//For each of the 8 bits in the input byte, starting with the MSB
		retval *= 10;	//Multiply by 10 to shift the return value left one digit
		if(input & bitmask)
		{	//If this bit is set
			retval++;	//Set the one's place of the return value
		}
	}
	return retval;
}

int eof_gh_read_instrument_section_qb(filebuffer *fb, EOF_SONG *sp, const char *songname, gh_section *target, unsigned long qbindex)
{
	unsigned long numnotes, dword, ctr, ctr2, arraysize, *arrayptr;
	unsigned int length, notemask, fixednotemask, isexpertplus;
	EOF_NOTE *newnote = NULL;
	char buffer[101];

	if(!fb || !sp || !target || !songname)
		return 0;

	snprintf(buffer, sizeof(buffer), "%s%s", songname, target->name);
#ifdef GH_IMPORT_DEBUG
	snprintf(eof_log_string, sizeof(eof_log_string), "\tGH:  Looking for section \"%s\"", buffer);
	eof_log(eof_log_string, 1);
#endif
	arraysize = eof_gh_process_section_header(fb, buffer, &arrayptr, qbindex);	//Parse the location of the 1D arrays of section data
	for(ctr = 0; ctr < arraysize; ctr++)
	{	//For each 1D array of note data
		numnotes = eof_gh_read_array_header(fb, arrayptr[ctr], qbindex);	//Process the array header (get size and seek to first data value)
		if(numnotes % 2)
		{	//The value in numnotes is the number of dwords used to define this star power array (each note should be 2 dwords in size)
			snprintf(eof_log_string, sizeof(eof_log_string), "Error:  Invalid note array size (%lu)", numnotes);
			eof_log(eof_log_string, 1);
			return -1;
		}
		numnotes /= 2;	//Determine the number of notes that are defined
#ifdef GH_IMPORT_DEBUG
		snprintf(eof_log_string, sizeof(eof_log_string), "\tGH:  \tNumber of notes = %lu", numnotes);
		eof_log(eof_log_string, 1);
#endif

		for(ctr2 = 0; ctr2 < numnotes; ctr2++)
		{	//For each note in the section
			if(eof_filebuffer_get_dword(fb, &dword))	//Read the note position
			{	//If there was an error reading the next 4 byte value
				eof_log("\t\tError:  Could not read note position", 1);
				free(arrayptr);
				return -1;
			}
			if(eof_filebuffer_get_word(fb, &notemask))	//Read the note bitmask
			{	//If there was an error reading the next 2 byte value
				eof_log("\t\tError:  Could not read note bitmask", 1);
				free(arrayptr);
				return -1;
			}
			if(eof_filebuffer_get_word(fb, &length))	//Read the note length
			{	//If there was an error reading the next 2 byte value
				eof_log("\t\tError:  Could not read note length", 1);
				free(arrayptr);
				return -1;
			}
#ifdef GH_IMPORT_DEBUG
			snprintf(eof_log_string, sizeof(eof_log_string), "\tGH:  \t\tNote %lu position = %lu  length = %u  bitmask = %u (%08lu %08lu)", ctr2+1, dword, length, notemask, eof_char_to_binary(notemask >> 8), eof_char_to_binary(notemask & 0xFF));
			eof_log(eof_log_string, 1);
#endif
			isexpertplus = 0;	//Reset this condition
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
				if((notemask & 8192) && !(notemask & 32))
				{	//If bit 13 is set, but bit 5 is not
					isexpertplus = 1;		//Consider this to be double bass
					fixednotemask |= 1;	//Set the lane 1 (bass drum gem)
				}
				notemask = fixednotemask;
			}
			newnote = (EOF_NOTE *)eof_track_add_create_note(sp, target->tracknum, (notemask & 0x3F), dword, length, target->diffnum, NULL);
			if(newnote == NULL)
			{	//If there was an error adding a new note
				eof_log("\t\tError:  Could not add note", 1);
				free(arrayptr);
				return -1;
			}
			if((target->tracknum == EOF_TRACK_GUITAR) || (target->tracknum == EOF_TRACK_BASS))
			{	//If this is a guitar track, check the accent mask and HOPO status
				if(notemask & 0x40)
				{	//Bit 6 is the HOPO status
					newnote->flags |= EOF_NOTE_FLAG_F_HOPO;
					newnote->flags |= EOF_NOTE_FLAG_HOPO;
				}
				else
				{	//If the note is not a HOPO, mark it as a forced non HOPO (strum required)
					newnote->flags |= EOF_NOTE_FLAG_NO_HOPO;
				}
				if((notemask >> 8) != 0xF)
				{	//"Crazy" guitar/bass notes have a high order note bitmask that isn't 0xF
					newnote->flags |= EOF_NOTE_FLAG_CRAZY;	//Set the crazy flag bit
				}
			}
			if(isexpertplus)
			{	//If this note was determined to be an expert+ drum note
				newnote->flags |= EOF_NOTE_FLAG_DBASS;	//Set the double bass flag bit
			}
		}
	}
	free(arrayptr);
	return 1;
}

int eof_gh_read_sp_section_qb(filebuffer *fb, EOF_SONG *sp, const char *songname, gh_section *target, unsigned long qbindex)
{
	unsigned long numphrases, dword, length, ctr, ctr2, arraysize, *arrayptr;
	char buffer[101];

	if(!fb || !sp || !target || !songname)
		return 0;

	snprintf(buffer, sizeof(buffer), "%s%s", songname, target->name);
#ifdef GH_IMPORT_DEBUG
	snprintf(eof_log_string, sizeof(eof_log_string), "\tGH:  Looking for section \"%s\"", buffer);
	eof_log(eof_log_string, 1);
#endif
	arraysize = eof_gh_process_section_header(fb, buffer, &arrayptr, qbindex);	//Parse the location of the 1D arrays of section data
	for(ctr = 0; ctr < arraysize; ctr++)
	{	//For each 1D array of star power data
		numphrases = eof_gh_read_array_header(fb, arrayptr[ctr], qbindex);	//Process the array header (get size and seek to first data value)
		if(numphrases % 3)
		{	//The value in numphrases is the number of dwords used to define this star power array (each phrase should be 3 dwords in size)
			snprintf(eof_log_string, sizeof(eof_log_string), "Error:  Invalid star power array size (%lu)", numphrases);
			eof_log(eof_log_string, 1);
			free(arrayptr);
			return -1;
		}
		numphrases /= 3;	//Determine the number of phrases that are defined
#ifdef GH_IMPORT_DEBUG
		snprintf(eof_log_string, sizeof(eof_log_string), "\tGH:  \tNumber of phrases = %lu", numphrases);
		eof_log(eof_log_string, 1);
#endif
		for(ctr2 = 0; ctr2 < numphrases; ctr2++)
		{	//For each phrase in the section
			if(eof_filebuffer_get_dword(fb, &dword))	//Read the phrase position
			{	//If there was an error reading the next 4 byte value
				eof_log("\t\tError:  Could not read phrase position", 1);
				free(arrayptr);
				return -1;
			}
			if(eof_filebuffer_get_dword(fb, &length))	//Read the phrase length
			{	//If there was an error reading the next 4 byte value
				eof_log("\t\tError:  Could not read phrase length", 1);
				free(arrayptr);
				return -1;
			}
			fb->index += 4;	//Skip the field indicating the number of notes contained within the star power section
#ifdef GH_IMPORT_DEBUG
			snprintf(eof_log_string, sizeof(eof_log_string), "\tGH:  \t\tPhrase %lu position = %lu  length = %lu", ctr2+1, dword, length);
			eof_log(eof_log_string, 1);
#endif
			if(!eof_track_add_section(sp, target->tracknum, EOF_SP_SECTION, 0, dword, dword + length, 0, NULL))
			{	//If there was an error adding the section
				eof_log("\t\tError:  Could not add sp section", 1);
				free(arrayptr);
				return -1;
			}
		}
	}
	free(arrayptr);
	return 1;
}

int eof_gh_read_tap_section_qb(filebuffer *fb, EOF_SONG *sp, const char *songname, gh_section *target, unsigned long qbindex)
{
	unsigned long numphrases, dword, length, ctr, ctr2, arraysize, *arrayptr;
	char buffer[101];

	if(!fb || !sp || !target || !songname)
		return 0;

	snprintf(buffer, sizeof(buffer), "%s%s", songname, target->name);
#ifdef GH_IMPORT_DEBUG
	snprintf(eof_log_string, sizeof(eof_log_string), "\tGH:  Looking for section \"%s\"", buffer);
	eof_log(eof_log_string, 1);
#endif
	arraysize = eof_gh_process_section_header(fb, buffer, &arrayptr, qbindex);	//Parse the location of the 1D arrays of section data
	for(ctr = 0; ctr < arraysize; ctr++)
	{	//For each 1D array of tap phrase data
		numphrases = eof_gh_read_array_header(fb, arrayptr[ctr], qbindex);	//Process the array header (get size and seek to first data value)
		if(numphrases % 3)
		{	//The value in numphrases is the number of dwords used to define this tap array (each phrase should be 3 dwords in size)
			snprintf(eof_log_string, sizeof(eof_log_string), "Error:  Invalid tap array size (%lu)", numphrases);
			eof_log(eof_log_string, 1);
			free(arrayptr);
			return -1;
		}
		numphrases /= 3;	//Determine the number of phrases that are defined
#ifdef GH_IMPORT_DEBUG
		snprintf(eof_log_string, sizeof(eof_log_string), "\tGH:  \tNumber of phrases = %lu", numphrases);
		eof_log(eof_log_string, 1);
#endif
		for(ctr2 = 0; ctr2 < numphrases; ctr2++)
		{	//For each phrase in the section
			if(eof_filebuffer_get_dword(fb, &dword))	//Read the phrase position
			{	//If there was an error reading the next 4 byte value
				eof_log("\t\tError:  Could not read phrase position", 1);
				free(arrayptr);
				return -1;
			}
			if(eof_filebuffer_get_dword(fb, &length))	//Read the phrase length
			{	//If there was an error reading the next 4 byte value
				eof_log("\t\tError:  Could not read phrase length", 1);
				free(arrayptr);
				return -1;
			}
			fb->index += 4;	//Skip the unknown data
#ifdef GH_IMPORT_DEBUG
			snprintf(eof_log_string, sizeof(eof_log_string), "\tGH:  \t\tPhrase %lu position = %lu  length = %lu", ctr2+1, dword, length);
			eof_log(eof_log_string, 1);
#endif
			if(!eof_track_add_section(sp, target->tracknum, EOF_SLIDER_SECTION, 0, dword, dword + length, 0, NULL))
			{	//If there was an error adding the section
				eof_log("\t\tError:  Could not add tap section", 1);
				free(arrayptr);
				return -1;
			}
		}
	}
	free(arrayptr);
	return 1;
}

int eof_gh_read_vocals_qb(filebuffer *fb, EOF_SONG *sp, const char *songname, unsigned long qbindex)
{
	unsigned long ctr, ctr2, numvox, voxstart, voxlength, voxpitch, numphrases, phrasestart, arraysize, *arrayptr, dword, tracknum;
//	unsigned long index1, index2, dword, numlyrics, lyricsize, lyricstart, tracknum, phrasesize, , phraseend, prevphrase, voxsize;
//	char *lyricbuffer = NULL, *lyricptr = NULL, *prevlyricptr = NULL;
//	unsigned char unicode_encoding = 0;	//Is set to nonzero if determined that the lyric text is in Unicode format
	EOF_LYRIC *ptr = NULL;
	EOF_VOCAL_TRACK * tp = NULL;
	char buffer[101], matched;

	if(!fb || !sp || !songname)
		return 0;

//Read vocal note positions and pitches
	snprintf(buffer, sizeof(buffer), "%s_song_vocals", songname);
#ifdef GH_IMPORT_DEBUG
	snprintf(eof_log_string, sizeof(eof_log_string), "\tGH:  Looking for section \"%s\"", buffer);
	eof_log(eof_log_string, 1);
#endif
	arraysize = eof_gh_process_section_header(fb, buffer, &arrayptr, qbindex);	//Parse the location of the 1D arrays of section data
	for(ctr = 0; ctr < arraysize; ctr++)
	{	//For each 1D array of voxnote data
		numvox = eof_gh_read_array_header(fb, arrayptr[ctr], qbindex);	//Process the array header (get size and seek to first data value)
		if(numvox % 3)
		{	//The value in numvox is the number of dwords used to define this vox array (each vox note should be 3 dwords in size)
			snprintf(eof_log_string, sizeof(eof_log_string), "Error:  Invalid vox note array size (%lu)", numvox);
			eof_log(eof_log_string, 1);
			free(arrayptr);
			return -1;
		}
		numvox /= 3;	//Determine the number of vox notes that are defined
#ifdef GH_IMPORT_DEBUG
		snprintf(eof_log_string, sizeof(eof_log_string), "\tGH:  \tNumber of vox notes = %lu", numvox);
		eof_log(eof_log_string, 1);
#endif
		for(ctr2 = 0; ctr2 < numvox; ctr2++)
		{	//For each vox note in the section
			if(eof_filebuffer_get_dword(fb, &voxstart))	//Read the vox note position
			{	//If there was an error reading the next 4 byte value
				eof_log("\t\tError:  Could not vox note position", 1);
				free(arrayptr);
				return -1;
			}
			if(eof_filebuffer_get_dword(fb, &voxlength))	//Read the vox note length
			{	//If there was an error reading the next 4 byte value
				eof_log("\t\tError:  Could not vox note length", 1);
				free(arrayptr);
				return -1;
			}
			if(eof_filebuffer_get_dword(fb, &voxpitch))	//Read the vox note pitch
			{	//If there was an error reading the next 4 byte value
				eof_log("\t\tError:  Could not vox note pitch", 1);
				free(arrayptr);
				return -1;
			}
#ifdef GH_IMPORT_DEBUG
			snprintf(eof_log_string, sizeof(eof_log_string), "\t\t\tVocal note %lu:  Position = %lu, Length = %lu, Pitch = %lu", ctr2+1, voxstart, voxlength, voxpitch);
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
			ptr = eof_track_add_create_note(sp, EOF_TRACK_VOCALS, voxpitch, voxstart, voxlength, 0, "+");

			if(!ptr)
			{	//If there was an error adding the lyric
				eof_log("\t\tError:  Could not add lyric", 1);
				free(arrayptr);
				return -1;
			}
		}//For each vox note in the section
	}//For each 1D array of star power data
	free(arrayptr);
	eof_track_sort_notes(sp, EOF_TRACK_VOCALS);

//Read lyric text
	struct QBlyric *head = NULL, *tail = NULL, *linkptr = NULL;	//Used to maintain the linked list matching lyric text with checksums
	unsigned char lyricid[] = {0x20, 0x22, 0x5C, 0x4C};	//This hex sequence is within each lyric entry between the lyric text and its checksum
	char *newtext = NULL;
	unsigned long checksum, length;

	fb->index = 0;	//Seek to the beginning of the file buffer
	while((eof_filebuffer_find_bytes(fb, lyricid, sizeof(lyricid), 1) > 0) && (fb->index > 8) && (fb->index + 5 < fb->size))
	{	//While there are lyric entries
		fb->index -= 8;	//Rewind 8 bytes, where the ASCII format 32 bit checksum is expected
		if(eof_filebuffer_memcpy(fb, buffer, 8) == EOF)	//Read the checksum into a buffer
		{
			eof_log("\t\tError:  Could not read lyric text checksum", 1);
			return -1;
		}
		buffer[8] = '\0';	//Null terminate the buffer
		sscanf(buffer, "%lX", &checksum);	//Convert the hex string to unsigned decimal format
		fb->index += 4;	//Skip ahead 5 bytes, where the lyric text is expected
		for(length = 0; fb->index + length < fb->size; length++)	//Count the length of the lyric text
		{	//Until end of buffer is reached
			if(fb->buffer[fb->index + length] == '\"')
			{	//If this is the end of the text string
				break;	//Exit the loop
			}
		}
		if(fb->index + length >= fb->size)
		{
			eof_log("\t\tError:  Malformed lyric text", 1);
			return -1;
		}
		if(eof_filebuffer_memcpy(fb, buffer, length) == EOF)	//Read the lyric string into a buffer
		{
			eof_log("\t\tError:  Could not read lyric text", 1);
			return -1;
		}
		buffer[length] = '\0';	//Null terminate the buffer
#ifdef GH_IMPORT_DEBUG
		snprintf(eof_log_string, sizeof(eof_log_string), "\tGH:  Found lyric:  Checksum = 0x%08lX\tText = \"%s\"", checksum, buffer);
		eof_log(eof_log_string, 1);
#endif
		linkptr = malloc(sizeof(struct QBlyric));	//Allocate a new link, initialize it and insert it into the linked list
		newtext = malloc(strlen(buffer) + 1);
		if(!linkptr || !newtext)
		{
			eof_log("\t\tError:  Cannot allocate memory", 1);
			return -1;
		}
		linkptr->checksum = checksum;
		strcpy(newtext, buffer);
		linkptr->text = newtext;
		linkptr->next = NULL;
		if(head == NULL)
		{	//If the list is empty
			head = linkptr;	//The new link is now the first link in the list
		}
		else if(tail != NULL)
		{	//If there is already a link at the end of the list
			tail->next = linkptr;	//Point it forward to the new link
		}
		tail = linkptr;	//The new link is the new tail of the list
	}

//Read lyric text positions
	fb->index = 0;	//Seek to the beginning of the file buffer
	snprintf(buffer, sizeof(buffer), "%s_lyrics", songname);
#ifdef GH_IMPORT_DEBUG
	snprintf(eof_log_string, sizeof(eof_log_string), "\tGH:  Looking for section \"%s\"", buffer);
	eof_log(eof_log_string, 1);
#endif
	arraysize = eof_gh_process_section_header(fb, buffer, &arrayptr, qbindex);	//Parse the location of the 1D arrays of section data
	for(ctr = 0; ctr < arraysize; ctr++)
	{	//For each block of lyric text data
		fb->index = arrayptr[ctr] + qbindex + 4;	//Seek four bytes past the beginning of this block
		if(eof_filebuffer_get_dword(fb, &dword))	//Read the offset of the start of data
		{	//If there was an error reading the next 4 byte value
			eof_log("Error:  Could not read timestamp data offset", 1);
			return -1;
		}
		fb->index = qbindex + dword + 8;	//Seek to the specified buffer position and four bytes further (past a header and an uninteresting checksum)
		if(eof_filebuffer_get_dword(fb, &voxstart))	//Read the timestamp of this lyric
		{	//If there was an error reading the next 4 byte value
			eof_log("Error:  Could not read lyric timestamp", 1);
			return -1;
		}
		if(eof_filebuffer_get_dword(fb, &dword))	//Read the offset of the lyric checksum
		{	//If there was an error reading the next 4 byte value
			eof_log("Error:  Could not read checksum data offset", 1);
			return -1;
		}
		fb->index = qbindex + dword + 8;	//Seek to the specified buffer position and four bytes further (past a header and an uninteresting checksum)
		if(eof_filebuffer_get_dword(fb, &checksum))	//Read the checksum of this lyric
		{	//If there was an error reading the next 4 byte value
			eof_log("Error:  Could not read lyric checksum", 1);
			return -1;
		}
		matched = 0;
		for(linkptr = head; (linkptr != NULL) && !matched; linkptr = linkptr->next)
		{	//For each link in the lyric checksum list (until a match has been made)
			if(linkptr->checksum == checksum)
			{	//If this checksum matches the one in the list
				for(ctr2 = 0; ctr2 < eof_get_track_size(sp, EOF_TRACK_VOCALS); ctr2++)
				{	//For each lyric in the EOF_SONG structure
					if(eof_get_note_pos(sp, EOF_TRACK_VOCALS, ctr2) == voxstart)
					{	//If this lyric has a matching timestamp
						eof_set_note_name(sp, EOF_TRACK_VOCALS, ctr2, linkptr->text);	//Update the text on this lyric
						matched = 1;
						break;
					}
				}
			}
		}
	}
	free(arrayptr);
	eof_process_gh_lyric_text(sp);	//Filter and convert hyphenating characters where appropriate

//Read lyric phrases
	fb->index = 0;	//Seek to the beginning of the file buffer
	tracknum = sp->track[EOF_TRACK_VOCALS]->tracknum;
	tp = sp->vocal_track[tracknum];		//Store the pointer to the vocal track to simplify things
	snprintf(buffer, sizeof(buffer), "%s_vocals_phrases", songname);
#ifdef GH_IMPORT_DEBUG
	snprintf(eof_log_string, sizeof(eof_log_string), "\tGH:  Looking for section \"%s\"", buffer);
	eof_log(eof_log_string, 1);
#endif
	arraysize = eof_gh_process_section_header(fb, buffer, &arrayptr, qbindex);	//Parse the location of the 1D arrays of section data
	for(ctr = 0; ctr < arraysize; ctr++)
	{	//For each 1D array of voxnote data
		numphrases = eof_gh_read_array_header(fb, arrayptr[ctr], qbindex);	//Process the array header (get size and seek to first data value)
		if(numphrases % 2)
		{	//The value in numphrases is the number of dwords used to define this vocal phrases array (each phrase should be 2 dwords in size)
			snprintf(eof_log_string, sizeof(eof_log_string), "Error:  Invalid vocal phrase note array size (%lu)", numvox);
			eof_log(eof_log_string, 1);
			free(arrayptr);
			return -1;
		}
		numphrases /= 2;	//Determine the number of vocal phrases that are defined
#ifdef GH_IMPORT_DEBUG
		snprintf(eof_log_string, sizeof(eof_log_string), "\tGH:  \tNumber of vocal phrases = %lu", numphrases);
		eof_log(eof_log_string, 1);
#endif
		for(ctr2 = 0; ctr2 < numphrases; ctr2++)
		{	//For each vocal phrase in the section
			if(eof_filebuffer_get_dword(fb, &phrasestart))	//Read the phrase position
			{	//If there was an error reading the next 4 byte value
				eof_log("\t\tError:  Could not vocal phrase position", 1);
				free(arrayptr);
				return -1;
			}
			fb->index += 4;	//Skip 4 bytes of uninteresting data
#ifdef GH_IMPORT_DEBUG
			snprintf(eof_log_string, sizeof(eof_log_string), "\t\tLyric phrase at %lu ms", phrasestart);
			eof_log(eof_log_string, 1);
#endif
			eof_vocal_track_add_line(tp, phrasestart, phrasestart);	//Add the phrase with a temporary end position
		}
	}
	eof_process_gh_lyric_phrases(sp);	//Create proper end positions for each lyric phrase

//Cleanup
	linkptr = head;
	while(linkptr != NULL)
	{	//While there are links remaining in the list
		free(linkptr->text);
		head = linkptr->next;	//The new head link will be the next link
		free(linkptr);			//Free the head link
		linkptr = head;			//Advance to the new head link
	}

	return 1;
}


EOF_SONG * eof_import_gh_qb(const char *fn)
{
	EOF_SONG * sp;
	filebuffer *fb;
	char filename[101], songname[21], buffer[101];
	char magicnumber[] = {0x1C,0x08,0x02,0x04,0x10,0x04,0x08,0x0C,0x0C,0x08,0x02,0x04,0x14,0x02,0x04,0x0C,0x10,0x10,0x0C,0x00};	//The magic number is expected 8 bytes into the QB header
	unsigned char byte;
	unsigned long index, ctr, ctr2, ctr3, arraysize, *arrayptr, numbeats, numsigs, tsnum, tsden, dword, lastfretbar = 0, lastsig = 0;
	unsigned long qbindex;	//Will store the file index of the QB header

	eof_log("Attempting to import QB format Guitar Hero chart", 1);
	eof_log("eof_import_gh_qb() entered", 1);

//Load the GH file into memory
	fb = eof_filebuffer_load(fn);
	if(fb == NULL)
	{
		eof_log("Error:  Failed to buffer GH file", 1);
		return NULL;
	}

//Find the file name (stored within the PAK header)
	while(1)
	{	//Until the file name has been found or end of file was reached
		if(eof_filebuffer_find_bytes(fb, "songs/", strlen("songs/"), 2) < 1)	//Try to find the string "songs/"
		{	//If the string couldn't be found in the file
			fb->index = 0;	//Rewind and try to find it with a backslash instead of a forward slash
			if(eof_filebuffer_find_bytes(fb, "songs\\", strlen("songs\\"), 2) < 1)	//Try to find the string "songs\"
			{
				fb->index = 0;	//Rewind and try to find the DLC based file name
				if(eof_filebuffer_find_bytes(fb, "dlc/output/", strlen("dlc/output/"), 2) < 1)	//Try to find the string "dlc/output/"
				{
					fb->index = 0;	//Rewind and try to find it with backslashes instead of forward slashes
					if(eof_filebuffer_find_bytes(fb, "dlc\\output\\", strlen("dlc\\output\\"), 2) < 1)	//Try to find the string "dlc\output\"
					{
						eof_log("Error:  Failed to locate internal file name", 1);
						eof_filebuffer_close(fb);
						return NULL;
					}
				}
			}
		}
//Parse the file name into a buffer
		index = 0;
		while(1)
		{	//Until the string has been parsed
			if(eof_filebuffer_get_byte(fb, &byte))
			{	//If there was an error reading the next 1 byte value
				eof_log("Error:  Could not parse file name", 1);
				eof_filebuffer_close(fb);
				return NULL;
			}
			if(byte == '\0')
			{	//If this was the end of the file name string
				filename[index] = '\0';	//Terminate the buffered string
				break;
			}
			if(index >= sizeof(filename))
			{	//If the string name would overflow the file name buffer
				eof_log("Error:  File name too long", 1);
				eof_filebuffer_close(fb);
				return NULL;
			}
			filename[index++] = byte;	//Store the parsed character
		}
//Validate the file name
		if(strstr(filename, ".mid."))
		{	//If this appears to be the correct file name
#ifdef GH_IMPORT_DEBUG
			snprintf(eof_log_string, sizeof(eof_log_string), "\tGH:  Internal file name is \"%s\"", filename);
			eof_log(eof_log_string, 1);
#endif
			break;
		}
	}
//Parse the song name
	for(index = 0; (filename[index] != '\0') && (filename[index] != '.'); index++)
	{	//For each character in the file name until the end of the string or a period is reached
		songname[index] = filename[index];	//Copy the character
	}
	songname[index] = '\0';	//Terminate the string
#ifdef GH_IMPORT_DEBUG
	snprintf(eof_log_string, sizeof(eof_log_string), "\tGH:  Internal song name is \"%s\"", songname);
	eof_log(eof_log_string, 1);
#endif

//Find the QB file header (8 byte file size followed by the 20 byte "magic number")
	fb->index = 0;	//Seek to the beginning of the file buffer
	if(eof_filebuffer_find_bytes(fb, magicnumber, sizeof(magicnumber), 1) < 1)	//Try to find the magic number
	{	//If the magic number couldn't be found in the file
		eof_log("Error:  Failed to locate magic number", 1);
		eof_filebuffer_close(fb);
		return NULL;
	}
	qbindex = fb->index - 8;	//The encapsulated QB file begins 8 bytes before the magic number
#ifdef GH_IMPORT_DEBUG
	snprintf(eof_log_string, sizeof(eof_log_string), "\tGH:  QB header located at file position 0x%lX", qbindex);
	eof_log(eof_log_string, 1);
#endif

//Process the fretbar section to create the tempo map
	snprintf(buffer, sizeof(buffer), "%s_fretbars", songname);	//Build the section name
	arraysize = eof_gh_process_section_header(fb, buffer, &arrayptr, qbindex);	//Parse the location of the 1D arrays of section data
	sp = eof_create_song_populated();
	if(sp == NULL)
	{	//Couldn't create new song
		eof_log("Error:  Could not create new chart", 1);
		eof_filebuffer_close(fb);
		free(arrayptr);
		return NULL;
	}
	for(ctr = 0; ctr < arraysize; ctr++)
	{	//For each 1D array of fretbar data
		numbeats = eof_gh_read_array_header(fb, arrayptr[ctr], qbindex);	//Process the array header (get size and seek to first data value)
#ifdef GH_IMPORT_DEBUG
		snprintf(eof_log_string, sizeof(eof_log_string), "\tGH:  \tNumber of fretbars = %lu", numbeats);
		eof_log(eof_log_string, 1);
#endif
		if(numbeats < 3)
		{
			eof_log("Error:  Invalid number of fretbars", 1);
			eof_filebuffer_close(fb);
			free(arrayptr);
			return NULL;
		}
		for(ctr2 = 0; ctr2 < numbeats; ctr2++)
		{	//For each beat in the array
			if(eof_song_add_beat(sp) == NULL)
			{	//If a new beat could not be added
				eof_log("Error:  Could not add beat", 1);
				eof_filebuffer_close(fb);
				eof_destroy_song(sp);
				free(arrayptr);
				return NULL;
			}
			if(eof_filebuffer_get_dword(fb, &dword))
			{	//If there was an error reading the next 4 byte value
				eof_log("Error:  Could not read fretbar position", 1);
				eof_destroy_song(sp);
				eof_filebuffer_close(fb);
				free(arrayptr);
				return NULL;
			}
			if(ctr2 && (dword <= lastfretbar))
			{	//If this beat doesn't come after the previous beat
				allegro_message("Warning:  Corrupt fretbar position");
				numbeats = ctr2;	//Update the number of beats
				if(numbeats < 3)
				{	//If too few beats were usable
					eof_destroy_song(sp);
					eof_filebuffer_close(fb);
					free(arrayptr);
					return NULL;
				}
			}
			else
			{
				sp->beat[ctr2]->pos = sp->beat[ctr2]->fpos = dword;	//Set the timestamp position of this beat
				lastfretbar = dword;
#ifdef GH_IMPORT_DEBUG
				snprintf(eof_log_string, sizeof(eof_log_string), "\tGH:  \t\tFretbar %lu position = %lu", ctr2+1, dword);
				eof_log(eof_log_string, 1);
#endif
			}
		}//For each beat in the array
	}//For each 1D array of fretbar data
	free(arrayptr);	//Free the memory used to store the 1D arrays of section data

	if(eof_use_ts)
	{	//If the user opted to import TS changes
//Process the timesig section to load time signatures
		fb->index = 0;	//Seek to the beginning of the file buffer
		snprintf(buffer, sizeof(buffer), "%s_timesig", songname);	//Build the section name
		arraysize = eof_gh_process_section_header(fb, buffer, &arrayptr, qbindex);	//Parse the location of the 1D arrays of section data
		for(ctr = 0; ctr < arraysize; ctr++)
		{	//For each 1D array of timesig data
			numsigs = eof_gh_read_array_header(fb, arrayptr[ctr], qbindex);	//Process the array header (get size and seek to first data value)
			if(numsigs % 3)
			{	//The value in numsigs is the number of dwords used to define this timesig array (each signature should be 3 dwords in size)
					eof_log("Error:  Invalid time signature array size", 1);
					eof_destroy_song(sp);
					eof_filebuffer_close(fb);
					free(arrayptr);
					return NULL;
			}
			numsigs /= 3;	//Determine the number of time signatures that are defined
#ifdef GH_IMPORT_DEBUG
			snprintf(eof_log_string, sizeof(eof_log_string), "\tGH:  \tNumber of timesigs = %lu", numsigs);
			eof_log(eof_log_string, 1);
#endif
			for(ctr2 = 0; ctr2 < numsigs; ctr2++)
			{	//For each time signature in the array
				if(eof_filebuffer_get_dword(fb, &dword))
				{	//If there was an error reading the next 4 byte value
					eof_log("Error:  Could not read time signature position", 1);
					eof_destroy_song(sp);
					eof_filebuffer_close(fb);
					free(arrayptr);
					return NULL;
				}
				if(ctr2 && (dword <= lastsig))
				{	//If this beat doesn't come after the previous beat
					eof_log("Error:  Corrupted time signature", 1);
					eof_destroy_song(sp);
					eof_filebuffer_close(fb);
					free(arrayptr);
					return NULL;
				}
				if(eof_filebuffer_get_dword(fb, &tsnum) || eof_filebuffer_get_dword(fb, &tsden))
				{	//If there was an error reading the next 8 bytes (numerator and denominator)
					eof_log("Error:  Could not read time signature", 1);
					eof_destroy_song(sp);
					eof_filebuffer_close(fb);
					free(arrayptr);
					return NULL;
				}
#ifdef GH_IMPORT_DEBUG
				snprintf(eof_log_string, sizeof(eof_log_string), "\tGH:  \t\tTime signature %lu position = %lu (%lu/%lu)", ctr2, dword, tsnum, tsden);
				eof_log(eof_log_string, 1);
#endif
				for(ctr3 = 0; ctr3 < sp->beats; ctr3++)
				{	//For each beat in the song
					if(dword == sp->beat[ctr3]->pos)
					{	//If this time signature is positioned at this beat marker
						eof_apply_ts(tsnum,tsden,ctr3,sp,0);	//Apply the signature
						break;
					}
					else if(dword < sp->beat[ctr3]->pos)
					{	//Otherwise if this time signature's position has been surpassed by a beat
						allegro_message("Error:  Mid beat time signature detected.  Skipping");
					}
				}

				lastsig = dword;
			}//For each time signature in the array
		}//For each 1D array of timesig data
	}//If the user opted to import TS changes
	free(arrayptr);	//Free the memory used to store the 1D arrays of section data
	eof_calculate_tempo_map(sp);	//Build the tempo map based on the beat time stamps

//Read instrument tracks
	for(ctr = 0; ctr < EOF_NUM_GH_INSTRUMENT_SECTIONS_QB; ctr++)
	{	//For each known guitar hero instrument difficulty section
		fb->index = 0;	//Rewind to beginning of file buffer
		eof_gh_read_instrument_section_qb(fb, sp, songname, &eof_gh_instrument_sections_qb[ctr], qbindex);	//Import notes from the section
	}

//Read star power sections
	for(ctr = 0; ctr < EOF_NUM_GH_SP_SECTIONS_QB; ctr++)
	{	//For each known guitar hero star power section
		fb->index = 0;	//Rewind to beginning of file buffer
		eof_gh_read_sp_section_qb(fb, sp, songname, &eof_gh_sp_sections_qb[ctr], qbindex);	//Import star power section
	}

//Read tap (slider) sections
	for(ctr = 0; ctr < EOF_NUM_GH_TAP_SECTIONS_QB; ctr++)
	{	//For each known guitar hero tap section
		fb->index = 0;	//Rewind to beginning of file buffer
		eof_gh_read_tap_section_qb(fb, sp, songname, &eof_gh_tap_sections_qb[ctr], qbindex);	//Import tap section
	}

//Read vocal track
	eof_gh_read_vocals_qb(fb, sp, songname, qbindex);
	eof_filebuffer_close(fb);	//Close the file buffer

	return sp;
}
