#include <allegro.h>
#include <assert.h>
#include "agup/agup.h"
#include "beat.h"
#include "dialog.h"
#include "event.h"	//For eof_song_add_text_event()
#include "gh_import.h"
#include "ini_import.h"	//For eof_import_ini()
#include "main.h"
#include "midi.h"	//For eof_apply_ts()
#include "utility.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

unsigned long crc32_lookup[256] = {0};	//A lookup table to improve checksum calculation performance
char crc32_lookup_initialized = 0;	//Is set to nonzero when the lookup table is created
char eof_gh_skip_size_def = 0;	//Is set to nonzero if it's determined that the size field in GH headers is omitted
char magicnumber[] = {0x1C,0x08,0x02,0x04,0x10,0x04,0x08,0x0C,0x0C,0x08,0x02,0x04,0x14,0x02,0x04,0x0C,0x10,0x10,0x0C,0x00};	//The magic number is expected 8 bytes into the QB header
unsigned char eof_gh_unicode_encoding_detected;	//Is set to nonzero if determined that the lyric/section text is in Unicode format

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

#define EOF_NUM_GH_SP_SECTIONS_NOTE 4
gh_section eof_gh_sp_sections_note[EOF_NUM_GH_SP_SECTIONS_NOTE] =
{
	{"guitarexpertstarpower", EOF_TRACK_GUITAR, 0},
	{"bassexpertstarpower", EOF_TRACK_BASS, 0},
	{"drumsexpertstarpower", EOF_TRACK_DRUM, 0},
	{"vocalstarpower", EOF_TRACK_VOCALS, 0}
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

DIALOG eof_all_sections_dialog[] =
{
	/* (proc)                    (x)  (y)  (w)  (h)  (fg) (bg) (key) (flags) (d1) (d2) (dp)                            (dp2) (dp3) */
	{ d_agup_window_proc,         0,   48,  500, 234, 2,   23,  0,    0,      0,   0,   "Is this the right language?", NULL, NULL },
	{ d_agup_list_proc,           12,  84,  475, 140, 2,   23,  0,    0,      0,   0,   (void *)eof_sections_list_all, NULL, NULL },
	{ d_agup_button_proc,         12,  237, 50, 28,  2,   23,  'Y',  D_EXIT, 0,   0,    "&Yes",                        NULL, NULL },
	{ d_agup_button_proc,         95,  237, 50, 28,  2,   23,  'N',  D_EXIT, 0,   0,    "&No",                         NULL, NULL },
	{ d_agup_button_proc,         178, 237, 50, 28,  2,   23,  'S',  D_EXIT, 0,   0,    "&Skip",                       NULL, NULL },
	{ NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

void eof_destroy_qblyric_list(struct QBlyric *head)
{
	struct QBlyric *ptr = head;

	while(ptr != NULL)
	{	//While there are links remaining in the list
		free(ptr->text);
		head = ptr->next;	//The new head link will be the next link
		free(ptr);			//Free the head link
		ptr = head;			//Advance to the new head link
	}
}

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
	{
		free(buffer);
		return NULL;	//Could not get size of file
	}
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
	if((size_t)fb->index + 1 >= fb->size)	//This read operation would run outside the buffer
		return EOF;
	*ptr = fb->buffer[fb->index++];

	return 0;
}

int eof_filebuffer_get_word(filebuffer *fb, unsigned int *ptr)
{
	if(!fb || !ptr)
		return EOF;
	if((size_t)fb->index + 2 >= fb->size)	//This read operation would run outside the buffer
		return EOF;
	*ptr = (fb->buffer[fb->index++] << 8);	//Read high byte value
	*ptr += fb->buffer[fb->index++];

	return 0;
}

int eof_filebuffer_memcpy(filebuffer *fb, void *ptr, size_t num)
{
	if(!fb || !ptr)
		return EOF;
	if((size_t)fb->index + num >= fb->size)	//This read operation would run outside the buffer
		return EOF;
	memcpy(ptr, &fb->buffer[fb->index], num);	//Read data
	fb->index += num;

	return 0;
}

int eof_filebuffer_get_dword(filebuffer *fb, unsigned long *ptr)
{
	if(!fb || !ptr)
		return EOF;
	if((size_t)fb->index + 4 >= fb->size)	//This read operation would run outside the buffer
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
		if((size_t)fb->index + 2 >= fb->size)	//Reading the next Unicode character would run outside the buffer
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

unsigned long eof_crc32_reflect(unsigned long value, unsigned numbits)
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
				crc32_lookup[x] = (crc32_lookup[x] << 1) ^ ((crc32_lookup[x] & (1 << 31)) ? polynomial : 0);
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
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  Checksum of \"%s\" is 0x%08lX", string, checksum);
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

	return (eof_filebuffer_find_bytes(fb, checksumarray, sizeof(checksumarray), 2) != 1);	//Seek to the byte that follows the checksum, convert the return value to return 0 on match
}

int eof_filebuffer_find_bytes(filebuffer *fb, const void *bytes, size_t searchlen, unsigned char seektype)
{
	unsigned long originalpos, matchpos = 0, index = 0;
	unsigned char *target, success = 0;

	if(!fb || !bytes)
		return -1;	//Return error
	if(!searchlen)
		return 0;	//There will be no matches to an empty search

	originalpos = fb->index;
	target = (unsigned char *)bytes;
	while((size_t)fb->index < fb->size)
	{	//While the end of buffer hasn't been reached
		if(fb->buffer[fb->index] == target[index])
		{	//If the next byte of the target has been matched
			if(index == 0)
			{	//The match was the first byte of the target
				matchpos = fb->index;
			}
			index++;	//Next pass of the loop will look for the next byte in the target
			if((size_t)index == searchlen)
			{	//If all bytes have been matched
				success = 1;
				break;
			}
		}
		else
		{	//The byte did not match
			if(index != 0)
			{	//If a partial match had been found
				fb->index = matchpos;	//return the buffer index to the start of that match, the index will increment to the next byte at the end of the loop
			}
			index = 0;	//Reset the search index to look for a match to the target's first byte
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

int eof_gh_read_instrument_section_note(filebuffer *fb, EOF_SONG *sp, gh_section *target, char forcestrum)
{
	unsigned long numnotes, dword, ctr, notesize = 0;
	unsigned int length, isexpertplus;
	unsigned char notemask, accentmask, fixednotemask;
	EOF_NOTE *newnote = NULL;

	if(!fb || !sp || !target)
		return -1;

#ifdef GH_IMPORT_DEBUG
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  Looking for section \"%s\"", target->name);
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
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tNumber of notes = %lu", numnotes);
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
		if(eof_filebuffer_get_byte(fb, &accentmask))	//Read the note accent bitmask
		{	//If there was an error reading the next 1 byte value
			eof_log("\t\tError:  Could not read note accent bitmask", 1);
			return -1;
		}
#ifdef GH_IMPORT_DEBUG
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  \t\tNote %lu position = %lu  length = %u  bitmask = %u (%08lu) accent = %08lu", ctr, dword, length, notemask, eof_char_to_binary(notemask), eof_char_to_binary(accentmask));
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
			if((notemask & 64) && !(notemask & 32))
			{	//If bit 6 is set, but bit 5 is not
				isexpertplus = 1;		//Consider this to be double bass
				fixednotemask |= 1;		//Set the lane 1 (bass drum gem)
			}
			notemask = fixednotemask;
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
			else if(forcestrum)
			{	//If the note is not a HOPO, mark it as a forced non HOPO (strum required), but only if the user opted to do so
				newnote->flags |= EOF_NOTE_FLAG_NO_HOPO;
			}
		}
		else if((target->tracknum == EOF_TRACK_DRUM) && (target->diffnum == EOF_NOTE_AMAZING) && (length >= 110))
		{	//If this is the expert drum track, check for long drum notes, which indicate that it should be treated as a drum roll
			int phrasetype = EOF_TREMOLO_SECTION;	//Assume a normal drum roll (one lane)
			unsigned long lastnote = eof_get_track_size(sp, EOF_TRACK_DRUM) - 1;

			if(eof_note_count_colors(sp, EOF_TRACK_DRUM, lastnote) > 1)
			{	//If the new drum note contains gems on more than one lane
				phrasetype = EOF_TRILL_SECTION;		//Consider it a special drum roll (multiple lanes)
			}
			(void) eof_track_add_section(sp, EOF_TRACK_DRUM, phrasetype, 0, dword, dword + length, 0, NULL);
		}
		if(isexpertplus)
		{	//If this note was determined to be an expert+ drum note
			newnote->flags |= EOF_DRUM_NOTE_FLAG_DBASS;	//Set the double bass flag bit
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
	}//For each note in the section
	return 1;
}

int eof_gh_read_sp_section_note(filebuffer *fb, EOF_SONG *sp, gh_section *target)
{
	unsigned long numphrases, phrasesize, dword, ctr;
	unsigned int length;

	if(!fb || !sp || !target)
		return -1;

#ifdef GH_IMPORT_DEBUG
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  Looking for section \"%s\"", target->name);
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
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tNumber of phrases = %lu", numphrases);
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
#ifdef GH_IMPORT_DEBUG
		else
		{	//The section was added
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  Star power section added:  %lums to %lums", dword, dword + length);
			eof_log(eof_log_string, 1);
		}
#endif
	}
	return 1;
}

int eof_gh_read_tap_section_note(filebuffer *fb, EOF_SONG *sp, gh_section *target)
{
	unsigned long numphrases, phrasesize, dword, ctr, length;

	if(!fb || !sp || !target)
		return -1;

#ifdef GH_IMPORT_DEBUG
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  Looking for section \"%s\"", target->name);
	eof_log(eof_log_string, 1);
#endif
	if(eof_filebuffer_find_checksum(fb, eof_gh_checksum(target->name)))	//Seek one byte past the target header
	{	//If the target section couldn't be found
		eof_log("\t\tCould not find section", 1);
		return 0;
	}
	if(eof_filebuffer_get_dword(fb, &numphrases))	//Read the number of tap phrases in the section
	{	//If there was an error reading the next 4 byte value
		eof_log("\t\tError:  Could not read number of phrases", 1);
		return -1;
	}
#ifdef GH_IMPORT_DEBUG
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tNumber of phrases = %lu", numphrases);
	eof_log(eof_log_string, 1);
#endif
	fb->index += 4;	//Seek past the next 4 bytes, which is a checksum for the game-specific sp section subheader
	if(!eof_gh_skip_size_def)
	{	//If the size field is expected
		if(eof_filebuffer_get_dword(fb, &phrasesize))	//Read the size of the tap entry
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

void eof_process_gh_lyric_text_u(EOF_SONG *sp)
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
			if((ugetat(string, ctr2) == '=') && (ctr > 0))
			{	//If this character is an equal sign, and there's a previous lyric, drop the character and append a hyphen to the previous lyric
				prevstring = eof_get_note_name(sp, EOF_TRACK_VOCALS, ctr - 1);	//Get the string for the previous lyric
				if(prevstring != NULL)
				{	//If there wasn't an error getting that string
					prevlength = ustrlen(prevstring);
					if((ugetat(prevstring, prevlength - 1) != '-') && (ugetat(prevstring, prevlength - 1) != '='))
					{	//If the previous lyric doesn't already end in a hyphen or equal sign
						if(prevlength + 1 < EOF_MAX_LYRIC_LENGTH)
						{	//Bounds check
							(void) uinsert(prevstring, prevlength, '-'); // append a hyphen
						}
					}
				}
			}
			else if((ugetat(string, ctr2) == '-') && (ctr2 + 1 == length))
			{	//If this is the last character in the string and it is a hyphen
				(void) usetat(buffer, index, '='); //Convert it to the equivalent RB notation
				index++;
			}
			else
			{	//This character isn't an equal sign or a hyphen at the end of the string
				(void) usetat(buffer, index, ugetat(string, ctr2)); //Copy it as-is
				index++;
			}
		}
		(void) usetat(buffer, index, '\0');
		index++;
		(void) usetat(buffer, index, '\0');
		index++;
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
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tAltering lyric phrase at %lu ms to be %lu ms - %lu ms", tp->line[ctr].start_pos, phrasestart, phraseend);
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
	char *lyricbuffer = NULL, matched;
	unsigned int voxlength;
	unsigned char voxpitch;
	EOF_LYRIC *ptr = NULL;
	EOF_VOCAL_TRACK * tp = NULL;
	char vocalsectionfound = 0;

	if(!fb || !sp)
		return -1;

	tracknum = sp->track[EOF_TRACK_VOCALS]->tracknum;
#ifdef GH_IMPORT_DEBUG
	eof_log("\tGH:  Searching for vocals", 1);
#endif
	fb->index = 0;	//Seek to the beginning of the file buffer
	while(!vocalsectionfound)
	{	//For some files, there are multiple instances of the vocals section checksum, find the appropriate one
		if(eof_filebuffer_find_checksum(fb, eof_gh_checksum("vocals")))	//Seek one byte past the target header
		{	//If the target section couldn't be found
			eof_log("\t\tCould not find section", 1);
			return 0;
		}
	#ifdef GH_IMPORT_DEBUG
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tChecking next vocal section at file position 0x%lX", fb->index - 4);
		eof_log(eof_log_string, 1);
	#endif
		if(eof_filebuffer_get_dword(fb, &numvox))	//Read the number of vox notes in the section
		{	//If there was an error reading the next 4 byte value
			eof_log("\t\tError:  Could not read number of vox notes", 1);
			return -1;
		}
	#ifdef GH_IMPORT_DEBUG
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tNumber of vox notes = %lu", numvox);
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
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tError:  Vox note size is given as %lu, not 7", voxsize);
				eof_log(eof_log_string, 1);
				if(eof_filebuffer_get_dword(fb, &voxsize) || (voxsize != 7))	//Read the next dword, as at least on GH file put the value of 7 one dword later than expected
				{	//If there was an error reading the next 4 byte value, or it wasn't 7
					eof_log("\t\tError:  Could not determine vox note size", 1);
					continue;
				}
			}
		}
		else
		{
			eof_log("\t\tSkipping field size", 1);
		}
		vocalsectionfound = 1;	//The expected values were parsed
	}
	for(ctr = 0; ctr < numvox; ctr++)
	{	//For each vox note in the section
		if(eof_filebuffer_get_dword(fb, &voxstart))	//Read the vox note position
		{	//If there was an error reading the next 4 byte value
			eof_log("\t\tError:  Could not read vox note position", 1);
			return -1;
		}
		if(eof_filebuffer_get_word(fb, &voxlength))	//Read the vox note length
		{	//If there was an error reading the next 2 byte value
			eof_log("\t\tError:  Could not read vox note length", 1);
			return -1;
		}
		if(eof_filebuffer_get_byte(fb, &voxpitch))	//Read the vox note pitch
		{	//If there was an error reading the next 1 byte value
			eof_log("\t\tError:  Could not read vox note pitch", 1);
			return -1;
		}
#ifdef GH_IMPORT_DEBUG
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tVocal note:  Position = %lu, Length = %u, Pitch = %u", voxstart, voxlength, voxpitch);
		eof_log(eof_log_string, 1);
#endif
		if(voxpitch == 2)
		{	//This pitch indicates a pitch shift bridging the previous and next lyrics
			char *name = eof_get_note_name(sp, EOF_TRACK_VOCALS, eof_get_track_size(sp, EOF_TRACK_VOCALS) - 1);
			if(name)
			{
				unsigned long length = ustrlen(name);
				if((name[length - 1] != '+') && (length < EOF_MAX_LYRIC_LENGTH))
				{	//If the previous lyric doesn't already end in a pitch shift character, and there's enough space to add a character
					name[length] = '+';			//Append a + character
					name[length + 1] = '\0';	//Re-terminate the string
				}
			}
		}
		else
		{
			if(voxpitch == 26)
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
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tNumber of lyrics = %lu", numlyrics);
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
		if(lyricsize == 68)
		{	//The Unicode version of GH string entries are 68 bytes each (32 Unicode characters plus one 4 byte timestamp)
			eof_gh_unicode_encoding_detected = 1;
			eof_log("\t\tUnicode text detected", 1);
		}
		else if(lyricsize != 36)
		{	//Each lyric entry is expected to be 36 bytes (for ASCII) or 68 bytes (for Unicode) long
			eof_log("\t\tError:  Lyric size is neither 36 nor 68", 1);
			return -1;
		}
	}
	else
	{
		if((size_t)fb->index + 5 >= fb->size)
		{	//Corrupted file
			eof_log("\t\tError:  Unexpected end of file", 1);
			return -1;
		}
		if((fb->buffer[fb->index + 4] == 0) && (fb->buffer[fb->index + 5] != 0))
		{	//Looking beyond the first timestamp, if the first two bytes of lyric text appear to be Unicode
			eof_gh_unicode_encoding_detected = 1;
			lyricsize = 68;	//The Unicode version of GH string entries are 68 bytes each (32 Unicode characters plus one 4 byte timestamp)
			eof_log("\t\tUnicode text detected", 1);
		}
		else
		{	//Otherwise assume a size of 36 bytes
			lyricsize = 36;
			eof_log("\t\tSkipping field size", 1);
		}
	}
	lyricbuffer = (char *)malloc((size_t)(lyricsize + 2) * 4); // leave enough room for unicode conversion (4x the memory)
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
			free(lyricbuffer);
			return -1;
		}
		if(!eof_gh_unicode_encoding_detected)
		{	//If these are not Unicode strings, just do a normal memory copy
			if(eof_filebuffer_memcpy(fb, lyricbuffer, (size_t)lyricsize - 4))	//Read 4 bytes less to account for the entry's position
			{	//If there was an error reading the lyric entry
				eof_log("\t\tError:  Could not read lyric text", 1);
				free(lyricbuffer);
				return -1;
			}
			//successfully read, convert to unicode for storage */
			else
			{
				(void) eof_convert_extended_ascii(lyricbuffer, (lyricsize + 2) * 4);
			}
		}
		else
		{
			if(eof_filebuffer_read_unicode_chars(fb, lyricbuffer, 32))
			{	//If there was an error reading 32 Unicode characters (68 bytes)
				eof_log("\t\tError:  Could not read lyric text", 1);
				free(lyricbuffer);
				return -1;
			}
		}
		lyricbuffer[lyricsize] = '\0';
		lyricbuffer[lyricsize + 1] = '\0';	//Ensure the last two bytes are NULL characters, in case this string is treated as Unicode
#ifdef GH_IMPORT_DEBUG
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tAdding lyric text:  Position = %lu, Lyric = \"%s\"",lyricstart, lyricbuffer);
		eof_log(eof_log_string, 1);
#endif

		matched = 0;
		for(ctr2 = 0; ctr2 < numvox; ctr2++)
		{	//For each vox note that was loaded earlier
			if(eof_get_note_pos(sp, EOF_TRACK_VOCALS, ctr2) == lyricstart)
			{	//If the position of this vox note matches the position of the lyric entry
#ifdef GH_IMPORT_DEBUG
				eof_log("\t\t\tFound matching vocal data", 1);
#endif
				eof_set_note_name(sp, EOF_TRACK_VOCALS, ctr2, lyricbuffer);	//Apply the lyric text
				matched = 1;
				break;
			}
		}
		if(!matched)
		{	//If there was not a lyric position that matched, one will have to be guessed
			unsigned long thispos = 0, nextpos;
			eof_log("\t\tThere is a bug in this NOTE file (no defined vocal pitch has a matching position).  Estimating the appropriate pitch to match with.", 1);
			for(ctr2 = 0; ctr2 < eof_get_track_size(sp, EOF_TRACK_VOCALS); ctr2++)
			{	//For each lyric pitch in the EOF_SONG structure
				if(eof_get_note_name(sp, EOF_TRACK_VOCALS, ctr2) && (eof_get_note_name(sp, EOF_TRACK_VOCALS, ctr2)[0] == '+'))
				{	//If this lyric pitch has no text assigned to it yet
					thispos = eof_get_note_pos(sp, EOF_TRACK_VOCALS, ctr2);
					if(thispos > lyricstart)
					{	//If this lyric pitch is defined before the lyric text
						eof_set_note_name(sp, EOF_TRACK_VOCALS, ctr2, lyricbuffer);	//Update the text on this lyric
						matched = 1;
						break;
					}
					if(ctr2 + 1 < eof_get_track_size(sp, EOF_TRACK_VOCALS))
					{	//If there is another lyric pitch that follows this one
						nextpos = eof_get_note_pos(sp, EOF_TRACK_VOCALS, ctr2 + 1);
						if(nextpos <= lyricstart)
						{	//If that lyric pitch is closer to this unmatched lyric text
							continue;	//Skip this lyric pitch because it's not a suitable match
						}
						else
						{	//That next lyric pitch is after this unmatched lyric text, find which pitch is closer
							if((lyricstart - thispos < nextpos - lyricstart))
							{	//If the earlier pitch is closer to the unmatched lyric text then the later pitch
								eof_set_note_name(sp, EOF_TRACK_VOCALS, ctr2, lyricbuffer);	//Update the text on this lyric
								matched = 1;
								break;
							}
							else
							{	//The later pitch is closer to the unmatched lyric text
								eof_set_note_name(sp, EOF_TRACK_VOCALS, ctr2 + 1, lyricbuffer);	//Update the text on this lyric
								thispos = nextpos;
								matched = 1;
								break;
							}
						}
					}
					else
					{	//There is not another lyric pitch that follows this one
						eof_set_note_name(sp, EOF_TRACK_VOCALS, ctr2, lyricbuffer);	//Update the text on this lyric
						matched = 1;
						break;
					}
				}//If this lyric pitch has no text assigned to it yet
			}//For each lyric pitch in the EOF_SONG structure
#ifdef GH_IMPORT_DEBUG
			if(matched)
			{	//If a match was determined
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tDecided match:  Text = \"%s\"\tPosition = %lu", lyricbuffer, thispos);
				eof_log(eof_log_string, 1);
			}
#endif
		}//If there was not a lyric position that matched
		if(!matched)
		{	//If there was still no match found, add it as a pitchless lyric
			(void) eof_track_add_create_note(sp, EOF_TRACK_VOCALS, 0, lyricstart, 10, 0, lyricbuffer);
		}
	}//For each lyric in the section
	eof_process_gh_lyric_text_u(sp);	//Filter and convert hyphenating characters where appropriate

	fb->index = 0;	//Seek to the beginning of the file buffer
	if(eof_filebuffer_find_checksum(fb, eof_gh_checksum("vocalphrase")))	//Seek one byte past the target header
	{	//If the target section couldn't be found
		eof_log("\t\tCould not find section", 1);
		free(lyricbuffer);
		return 0;
	}
	if(eof_filebuffer_get_dword(fb, &numphrases))	//Read the number of lyric phrases in the section
	{	//If there was an error reading the next 4 byte value
		eof_log("\t\tError:  Could not read number of lyric phrases", 1);
		free(lyricbuffer);
		return -1;
	}
#ifdef GH_IMPORT_DEBUG
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tNumber of lyric phrases = %lu", numphrases);
	eof_log(eof_log_string, 1);
#endif
	fb->index += 4;	//Seek past the next 4 bytes, which is a checksum for the game-specific lyric phrase section subheader
	if(!eof_gh_skip_size_def)
	{	//If the size field is expected
		if(eof_filebuffer_get_dword(fb, &phrasesize))	//Read the size of the lyric phrase entry
		{	//If there was an error reading the next 4 byte value
			eof_log("\t\tError:  Could not read lyric phrase size", 1);
			free(lyricbuffer);
			return -1;
		}
		if(phrasesize != 4)
		{	//Each lyric entry is expected to be 4 bytes long
			eof_log("\t\tError:  Lyric phrase size is not 4", 1);
			free(lyricbuffer);
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
			free(lyricbuffer);
			return -1;
		}
#ifdef GH_IMPORT_DEBUG
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tLyric phrase at %lu ms", phrasestart);
	eof_log(eof_log_string, 1);
#endif
		(void) eof_vocal_track_add_line(tp, phrasestart, phrasestart);	//Add the phrase with a temporary end position
	}
	eof_process_gh_lyric_phrases(sp);	//Create proper end positions for each lyric phrase

	free(lyricbuffer);
	return 1;
}

EOF_SONG * eof_import_gh(const char * fn)
{
	EOF_SONG * sp;
	char oggfn[1024] = {0}, backup_filename[1024] = {0}, inifn[1024] = {0};

	eof_log("\tImporting Guitar Hero chart", 1);
	eof_log("eof_import_gh() entered", 1);

	//Build the paths to guitar.ogg and song.ini ahead of time, as fn can be clobbered by browsing for an external section names file during GH import below
	(void) replace_filename(oggfn, fn, "guitar.ogg", 1024);
	(void) replace_filename(inifn, fn, "song.ini", 1024);
	eof_allocate_ucode_table();
	eof_gh_unicode_encoding_detected = 0;	//Reset this condition
	sp = eof_import_gh_note(fn);	//Attempt to load as a "NOTE" format GH file
	if(!sp)
	{	//If that failed
		sp = eof_import_gh_qb(fn);	//Attempt to load as a "QB" format GH file
		if(sp && eof_get_track_size(sp, EOF_TRACK_DRUM))
		{	//If there were any drum gems imported from this QB format GH file, add a "drum_fallback_blue = True" INI tag so Phase Shift knows how to downchart to 4 lanes
			if(sp->tags->ini_settings < EOF_MAX_INI_SETTINGS)
			{	//If this INI setting can be stored
				(void) ustrcpy(sp->tags->ini_setting[sp->tags->ini_settings]," drum_fallback_blue = True");
				sp->tags->ini_settings++;
			}
		}
	}

	if(sp)
	{	//If a GH file was loaded
		unsigned long tracknum;
		unsigned long ctr;

		/* backup "song.ini" if it exists in the folder with the imported file
		as it will be overwritten upon save */
		if(exists(inifn))
		{
			/* do not overwrite an existing backup, this prevents the original backed up song.ini from
			being overwritten if the user imports the MIDI again */
			(void) replace_filename(backup_filename, inifn, "song.ini.backup", 1024);
			if(!exists(backup_filename))
			{
				(void) eof_copy_file(eof_temp_filename, backup_filename);
			}
		}

		/* read INI file */
		(void) eof_import_ini(sp, inifn, 0);

		if(eof_get_track_size(sp, EOF_TRACK_DRUM))
		{	//If there were any drum gems imported, ensure the fifth drum lane is enabled, as all GH drum charts are this style of track
			tracknum = sp->track[EOF_TRACK_DRUM]->tracknum;
			sp->track[EOF_TRACK_DRUM]->flags |= EOF_TRACK_FLAG_SIX_LANES;	//Set the five lane drum track flag
			sp->legacy_track[tracknum]->numlanes = 6;						//Set the lane count
		}

		for(ctr = 0; ctr < eof_get_track_size(sp, EOF_TRACK_BASS); ctr++)
		{	//For each bass guitar note
			if(eof_get_note_note(sp, EOF_TRACK_BASS, ctr) & 32)
			{	//If the the note has a gem in lane six
				tracknum = sp->track[EOF_TRACK_BASS]->tracknum;
				sp->track[EOF_TRACK_BASS]->flags |= EOF_TRACK_FLAG_SIX_LANES;	//Set the flag for six lane bass
				sp->legacy_track[tracknum]->numlanes = 6;	//Set the number of lanes
				break;
			}
		}

//Load an audio file
		if(!eof_load_ogg(oggfn, 1))	//If user does not provide audio, fail over to using silent audio
		{
			eof_destroy_song(sp);
			eof_free_ucode_table();
			return NULL;
		}
		eof_music_length = alogg_get_length_msecs_ogg(eof_music_track);
		eof_truncate_chart(sp);	//Update the chart length before performing lyric cleanup
		eof_vocal_track_fixup_lyrics(sp, EOF_TRACK_VOCALS, 0);	//Clean up the lyrics
		eof_log("\tGH import completed", 1);
	}
	eof_free_ucode_table();
	return sp;
}

EOF_SONG * eof_import_gh_note(const char * fn)
{
	EOF_SONG * sp;
	filebuffer *fb;
	unsigned long dword, ctr, ctr2, numbeats, numsigs, lastfretbar = 0, lastsig = 0;
	unsigned char tsnum, tsden;
	char forcestrum = 0;

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
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  Number of fretbars = %lu", numbeats);
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
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  Fretbar at %lums", dword);
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
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  Number of time signatures = %lu", numsigs);
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
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  Time signature at %lums (%d/%d)", dword, tsnum, tsden);
			eof_log(eof_log_string, 1);
#endif
			for(ctr2 = 0; ctr2 < sp->beats; ctr2++)
			{	//For each beat in the song
				if(dword == sp->beat[ctr2]->pos)
				{	//If this time signature is positioned at this beat marker
					(void) eof_apply_ts(tsnum,tsden,ctr2,sp,0);	//Apply the signature
					break;
				}
				else if(dword < sp->beat[ctr2]->pos)
				{	//Otherwise if this time signature's position has been surpassed by a beat
					allegro_message("Error:  Mid beat time signature detected.  Skipping");
					break;
				}
			}

			lastsig = dword;
		}
	}//If the user opted to import TS changes

	eof_clear_input();
	if(alert(NULL, "Import the chart's original HOPO OFF notation?", NULL, "&Yes", "&No", 'y', 'n') == 1)
	{	//If user opts to have all non HOPO notes marked for forced strumming
		forcestrum = 1;
	}

//Read instrument tracks
	for(ctr = 0; ctr < EOF_NUM_GH_INSTRUMENT_SECTIONS_NOTE; ctr++)
	{	//For each known guitar hero instrument difficulty section
		fb->index = 0;	//Rewind to beginning of file buffer
		(void) eof_gh_read_instrument_section_note(fb, sp, &eof_gh_instrument_sections_note[ctr], forcestrum);	//Import notes from the section
	}

//Read vocal track
	(void) eof_gh_read_vocals_note(fb, sp);

//Read star power sections
	for(ctr = 0; ctr < EOF_NUM_GH_SP_SECTIONS_NOTE; ctr++)
	{	//For each known guitar hero star power section
		fb->index = 0;	//Rewind to beginning of file buffer
		(void) eof_gh_read_sp_section_note(fb, sp, &eof_gh_sp_sections_note[ctr]);	//Import star power section
	}

//Read tap (slider) sections
	for(ctr = 0; ctr < EOF_NUM_GH_TAP_SECTIONS_NOTE; ctr++)
	{	//For each known guitar hero tap section
		fb->index = 0;	//Rewind to beginning of file buffer
		(void) eof_gh_read_tap_section_note(fb, sp, &eof_gh_tap_sections_note[ctr]);	//Import tap section
	}

//Read sections
	(void) eof_gh_read_sections_note(fb, sp);
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
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Error:  Failed to locate \"%s\" header.", sectionname);
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
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  Data is offset to position 0x%08lX within the QB file", dword);
		eof_log(eof_log_string, 1);
		fb->index = qbindex + dword;	//Seek to the appropriate buffer position
	}
	if(eof_filebuffer_get_dword(fb, &dword))
	{	//If there was an error reading the next 4 byte value
		eof_log("Error:  Could not read section's storage chunk type", 1);
		return 0;
	}
#ifdef GH_IMPORT_DEBUG
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  \tSection storage chunk type is 0x%08lX", dword);
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
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  \t1D array at QB index 0x%08lX (file position 0x%lX)", fb->index - qbindex - 12, fb->index - 12);
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
					*arrayptr = NULL;
					return 0;
				}
				if(fb->index - qbindex != dword)
				{	//If the data doesn't begin after the padding
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  2D array data is offset to position 0x%08lX within the QB file", dword);
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
					*arrayptr = NULL;
					return 0;
				}
/*
#ifdef GH_IMPORT_DEBUG
				if(dword == 0x00010c00)
				{	//2D array
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  \t\t2D array at QB index 0x%08lX (file position 0x%lX)", (*arrayptr)[ctr], (*arrayptr)[ctr] + qbindex);
				}
				else
				{	//Alternate 2D array
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  \t\tData at QB index 0x%08lX (file position 0x%lX)", (*arrayptr)[ctr], (*arrayptr)[ctr] + qbindex);
				}
				eof_log(eof_log_string, 1);
#endif
*/
			}
		}
	}
	else
	{	//Unsupported type
		eof_log("\t\tError:  Unsupported section storage chunk type", 1);
		free(*arrayptr);
		*arrayptr = NULL;
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

int eof_gh_read_instrument_section_qb(filebuffer *fb, EOF_SONG *sp, const char *songname, gh_section *target, unsigned long qbindex, char forcestrum)
{
	unsigned long numnotes, dword, ctr, ctr2, arraysize, *arrayptr;
	unsigned int length, notemask, fixednotemask, isexpertplus;
	EOF_NOTE *newnote = NULL;
	char buffer[101] = {0};

	if(!fb || !sp || !target || !songname)
		return -1;

	(void) snprintf(buffer, sizeof(buffer) - 1, "%s%s", songname, target->name);
#ifdef GH_IMPORT_DEBUG
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  Looking for section \"%s\"", buffer);
	eof_log(eof_log_string, 1);
#endif
	arraysize = eof_gh_process_section_header(fb, buffer, &arrayptr, qbindex);	//Parse the location of the 1D arrays of section data
	for(ctr = 0; ctr < arraysize; ctr++)
	{	//For each 1D array of note data
		numnotes = eof_gh_read_array_header(fb, arrayptr[ctr], qbindex);	//Process the array header (get size and seek to first data value)
		if(numnotes % 2)
		{	//The value in numnotes is the number of dwords used to define this note array (each note should be 2 dwords in size)
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Error:  Invalid note array size (%lu)", numnotes);
			eof_log(eof_log_string, 1);
			return -1;
		}
		numnotes /= 2;	//Determine the number of notes that are defined
#ifdef GH_IMPORT_DEBUG
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  \tNumber of notes = %lu", numnotes);
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
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  \t\tNote %lu position = %lu  length = %u  bitmask = %u (%08lu %08lu)", ctr2+1, dword, length, notemask, eof_char_to_binary(notemask >> 8), eof_char_to_binary(notemask & 0xFF));
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
				else if(forcestrum)
				{	//If the note is not a HOPO, mark it as a forced non HOPO (strum required), but only if the user opted to do so
					newnote->flags |= EOF_NOTE_FLAG_NO_HOPO;
				}
				if((notemask >> 8) != 0xF)
				{	//"Crazy" guitar/bass notes have a high order note bitmask that isn't 0xF
					newnote->flags |= EOF_NOTE_FLAG_CRAZY;	//Set the crazy flag bit
				}
			}
			else if((target->tracknum == EOF_TRACK_DRUM) && (length >= 110))
			{	//If this is a drum track, check for long drum notes, which indicate that it should be treated as a drum roll
				int phrasetype = EOF_TREMOLO_SECTION;	//Assume a normal drum roll (one lane)
				unsigned long lastnote = eof_get_track_size(sp, EOF_TRACK_DRUM) - 1;

				if(eof_note_count_colors(sp, EOF_TRACK_DRUM, lastnote) > 1)
				{	//If the new drum note contains gems on more than one lane
					phrasetype = EOF_TRILL_SECTION;		//Consider it a special drum roll (multiple lanes)
				}
				(void) eof_track_add_section(eof_song, EOF_TRACK_DRUM, phrasetype, 0, dword, dword + length, 0, NULL);
			}
			if(isexpertplus)
			{	//If this note was determined to be an expert+ drum note
				newnote->flags |= EOF_DRUM_NOTE_FLAG_DBASS;	//Set the double bass flag bit
			}
		}
	}
	if(arraysize)
	{	//If memory was allocated by eof_gh_process_section_header()
		free(arrayptr);
	}
	return 1;
}

int eof_gh_read_sp_section_qb(filebuffer *fb, EOF_SONG *sp, const char *songname, gh_section *target, unsigned long qbindex)
{
	unsigned long numphrases, dword, length, ctr, ctr2, arraysize, *arrayptr;
	char buffer[101] = {0};

	if(!fb || !sp || !target || !songname)
		return -1;

	(void) snprintf(buffer, sizeof(buffer) - 1, "%s%s", songname, target->name);
#ifdef GH_IMPORT_DEBUG
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  Looking for section \"%s\"", buffer);
	eof_log(eof_log_string, 1);
#endif
	arraysize = eof_gh_process_section_header(fb, buffer, &arrayptr, qbindex);	//Parse the location of the 1D arrays of section data
	for(ctr = 0; ctr < arraysize; ctr++)
	{	//For each 1D array of star power data
		numphrases = eof_gh_read_array_header(fb, arrayptr[ctr], qbindex);	//Process the array header (get size and seek to first data value)
		if(numphrases % 3)
		{	//The value in numphrases is the number of dwords used to define this star power array (each phrase should be 3 dwords in size)
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Error:  Invalid star power array size (%lu)", numphrases);
			eof_log(eof_log_string, 1);
			free(arrayptr);
			return -1;
		}
		numphrases /= 3;	//Determine the number of phrases that are defined
#ifdef GH_IMPORT_DEBUG
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  \tNumber of phrases = %lu", numphrases);
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
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  \t\tPhrase %lu position = %lu  length = %lu", ctr2+1, dword, length);
			eof_log(eof_log_string, 1);
#endif
			if(!eof_track_add_section(sp, target->tracknum, EOF_SP_SECTION, 0, dword, dword + length, 0, NULL))
			{	//If there was an error adding the section
				eof_log("\t\tError:  Could not add sp section", 1);
				free(arrayptr);
				return -1;
			}
#ifdef GH_IMPORT_DEBUG
			else
			{	//The section was added
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  \t\tStar power section added:  %lums to %lums", dword, dword + length);
				eof_log(eof_log_string, 1);
			}
#endif
		}
	}
	if(arraysize)
	{	//If memory was allocated by eof_gh_process_section_header()
		free(arrayptr);
	}
	return 1;
}

int eof_gh_read_tap_section_qb(filebuffer *fb, EOF_SONG *sp, const char *songname, gh_section *target, unsigned long qbindex)
{
	unsigned long numphrases, dword, length, ctr, ctr2, arraysize, *arrayptr;
	char buffer[101] = {0};

	if(!fb || !sp || !target || !songname)
		return -1;

	(void) snprintf(buffer, sizeof(buffer) - 1, "%s%s", songname, target->name);
#ifdef GH_IMPORT_DEBUG
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  Looking for section \"%s\"", buffer);
	eof_log(eof_log_string, 1);
#endif
	arraysize = eof_gh_process_section_header(fb, buffer, &arrayptr, qbindex);	//Parse the location of the 1D arrays of section data
	for(ctr = 0; ctr < arraysize; ctr++)
	{	//For each 1D array of tap phrase data
		numphrases = eof_gh_read_array_header(fb, arrayptr[ctr], qbindex);	//Process the array header (get size and seek to first data value)
		if(numphrases % 3)
		{	//The value in numphrases is the number of dwords used to define this tap array (each phrase should be 3 dwords in size)
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Error:  Invalid tap array size (%lu)", numphrases);
			eof_log(eof_log_string, 1);
			free(arrayptr);
			return -1;
		}
		numphrases /= 3;	//Determine the number of phrases that are defined
#ifdef GH_IMPORT_DEBUG
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  \tNumber of phrases = %lu", numphrases);
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
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  \t\tPhrase %lu position = %lu  length = %lu", ctr2+1, dword, length);
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
	if(arraysize)
	{	//If memory was allocated by eof_gh_process_section_header()
		free(arrayptr);
	}
	return 1;
}

int eof_gh_read_vocals_qb(filebuffer *fb, EOF_SONG *sp, const char *songname, unsigned long qbindex)
{
	unsigned long ctr, ctr2, numvox, voxstart, voxlength, voxpitch, numphrases, phrasestart, arraysize, *arrayptr, dword, tracknum;
	EOF_LYRIC *ptr = NULL;
	EOF_VOCAL_TRACK * tp = NULL;
	char buffer[201], matched;
	struct QBlyric *head = NULL, *tail = NULL, *linkptr = NULL;	//Used to maintain the linked list matching lyric text with checksums
	unsigned char lyricid[] = {0x20, 0x22, 0x5C, 0x4C};	//This hex sequence is within each lyric entry between the lyric text and its checksum
	char *newtext = NULL;
	unsigned long checksum, length;

	if(!fb || !sp || !songname)
		return -1;

//Read vocal note positions and pitches
	(void) snprintf(buffer, sizeof(buffer) - 1, "%s_song_vocals", songname);
#ifdef GH_IMPORT_DEBUG
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  Looking for section \"%s\"", buffer);
	eof_log(eof_log_string, 1);
#endif
	fb->index = 0;	//Seek to the beginning of the file buffer
	arraysize = eof_gh_process_section_header(fb, buffer, &arrayptr, qbindex);	//Parse the location of the 1D arrays of section data
	for(ctr = 0; ctr < arraysize; ctr++)
	{	//For each 1D array of voxnote data
		numvox = eof_gh_read_array_header(fb, arrayptr[ctr], qbindex);	//Process the array header (get size and seek to first data value)
		if(numvox % 3)
		{	//The value in numvox is the number of dwords used to define this vox array (each vox note should be 3 dwords in size)
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Error:  Invalid vox note array size (%lu)", numvox);
			eof_log(eof_log_string, 1);
			free(arrayptr);
			return -1;
		}
		numvox /= 3;	//Determine the number of vox notes that are defined
#ifdef GH_IMPORT_DEBUG
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  \tNumber of vox notes = %lu", numvox);
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
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tVocal note %lu:  Position = %lu, Length = %lu, Pitch = %lu", ctr2+1, voxstart, voxlength, voxpitch);
			eof_log(eof_log_string, 1);
#endif
			if(voxpitch == 2)
			{	//This pitch indicates a pitch shift bridging the previous and next lyrics
				char *name = eof_get_note_name(sp, EOF_TRACK_VOCALS, eof_get_track_size(sp, EOF_TRACK_VOCALS) - 1);
				if(name)
				{
					unsigned long length2 = ustrlen(name);
					if((name[length2 - 1] != '+') && (length2 < EOF_MAX_LYRIC_LENGTH))
					{	//If the previous lyric doesn't already end in a pitch shift character, and there's enough space to add a character
						name[length2] = '+';			//Append a + character
						name[length2 + 1] = '\0';	//Re-terminate the string
					}
				}
			}
			else
			{
				if(voxpitch == 26)
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
			}
		}//For each vox note in the section
	}//For each 1D array of star power data
	if(arraysize)
	{	//If memory was allocated by eof_gh_process_section_header()
		free(arrayptr);
		arrayptr = NULL;
	}
	eof_track_sort_notes(sp, EOF_TRACK_VOCALS);

//Read lyric text
#ifdef GH_IMPORT_DEBUG
	eof_log("\tGH:  Looking for lyrics", 1);
#endif
	fb->index = 0;	//Seek to the beginning of the file buffer
	while((eof_filebuffer_find_bytes(fb, lyricid, sizeof(lyricid), 1) > 0) && (fb->index > 8) && ((size_t)fb->index + 5 < fb->size))
	{	//While there are lyric entries
		fb->index -= 8;	//Rewind 8 bytes, where the ASCII format 32 bit checksum is expected
		if(eof_filebuffer_memcpy(fb, buffer, 8) == EOF)	//Read the checksum into a buffer
		{
			eof_log("\t\tError:  Could not read lyric text checksum", 1);
			return -1;
		}
		buffer[8] = '\0';	//Null terminate the buffer
		(void) sscanf(buffer, "%8lX", &checksum);	//Convert the hex string to unsigned decimal format (sscanf is width limited to prevent buffer overflow)
		fb->index += 4;	//Skip ahead 4 bytes, where the lyric text is expected
		for(length = 0; (size_t)fb->index + length < fb->size; length++)	//Count the length of the lyric text
		{	//Until end of buffer is reached
			if(fb->buffer[fb->index + length] == '\"')
			{	//If this is the end of the text string
				break;	//Exit the loop
			}
		}
		if((size_t)fb->index + length >= fb->size)
		{
			eof_log("\t\tError:  Malformed lyric text", 1);
			return -1;
		}
		if((size_t)length + 1 > sizeof(buffer))
		{	//If the buffer isn't large enough to store this string and its NULL terminator
			allegro_message("Error:  QB lyric buffer too small, aborting");
			return -1;
		}
		if(eof_filebuffer_memcpy(fb, buffer, (size_t)length) == EOF)	//Read the lyric string into a buffer
		{
			eof_log("\t\tError:  Could not read lyric text", 1);
			return -1;
		}
		buffer[length] = '\0';	//Null terminate the buffer
#ifdef GH_IMPORT_DEBUG
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  Found lyric:  Checksum = 0x%08lX\tText = \"%s\"", checksum, buffer);
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
	}//While there are lyric entries

//Read lyric text positions
	fb->index = 0;	//Seek to the beginning of the file buffer
	(void) snprintf(buffer, sizeof(buffer) - 1, "%s_lyrics", songname);
#ifdef GH_IMPORT_DEBUG
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  Looking for section \"%s\"", buffer);
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
#ifdef GH_IMPORT_DEBUG
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  Found lyric checksum position:  Checksum = 0x%08lX\tPosition = %lu", checksum, voxstart);
		eof_log(eof_log_string, 1);
#endif
		matched = 0;
		for(linkptr = head; (linkptr != NULL) && !matched; linkptr = linkptr->next)
		{	//For each link in the lyric checksum list (until a match has been made)
			if(linkptr->checksum == checksum)
			{	//If this checksum matches the one in the list (until a match has been made)
				for(ctr2 = 0; ctr2 < eof_get_track_size(sp, EOF_TRACK_VOCALS); ctr2++)
				{	//For each lyric pitch in the EOF_SONG structure
					if(eof_get_note_pos(sp, EOF_TRACK_VOCALS, ctr2) == voxstart)
					{	//If this lyric has a matching timestamp
#ifdef GH_IMPORT_DEBUG
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tMatched lyric position:  Text = \"%s\"\tPosition = %lu", linkptr->text, voxstart);
						eof_log(eof_log_string, 1);
#endif
						eof_set_note_name(sp, EOF_TRACK_VOCALS, ctr2, linkptr->text);	//Update the text on this lyric
						matched = 1;
						break;
					}
				}
				if(!matched)
				{	//If there was not a lyric position that matched, one will have to be guessed
					unsigned long thispos = 0, nextpos;
					eof_log("\t\tThere is a bug in this QB file (no defined vocal pitch has a matching position).  Estimating the appropriate pitch to match with.", 1);
					for(ctr2 = 0; ctr2 < eof_get_track_size(sp, EOF_TRACK_VOCALS); ctr2++)
					{	//For each lyric pitch in the EOF_SONG structure
						if(eof_get_note_name(sp, EOF_TRACK_VOCALS, ctr2) && (eof_get_note_name(sp, EOF_TRACK_VOCALS, ctr2)[0] == '+'))
						{	//If this lyric pitch has no text assigned to it yet
							thispos = eof_get_note_pos(sp, EOF_TRACK_VOCALS, ctr2);
							if(thispos > voxstart)
							{	//If this lyric pitch is defined before the lyric text
								eof_set_note_name(sp, EOF_TRACK_VOCALS, ctr2, linkptr->text);	//Update the text on this lyric
								matched = 1;
								break;
							}
							if(ctr2 + 1 < eof_get_track_size(sp, EOF_TRACK_VOCALS))
							{	//If there is another lyric pitch that follows this one
								nextpos = eof_get_note_pos(sp, EOF_TRACK_VOCALS, ctr2 + 1);
								if(nextpos <= voxstart)
								{	//If that lyric pitch is closer to this unmatched lyric text
									continue;	//Skip this lyric pitch becauase it's not a suitable match
								}
								else
								{	//That next lyric pitch is after this unmatched lyric text, find which pitch is closer
									if((voxstart - thispos < nextpos - voxstart))
									{	//If the earlier pitch is closer to the unmatched lyric text then the later pitch
										eof_set_note_name(sp, EOF_TRACK_VOCALS, ctr2, linkptr->text);	//Update the text on this lyric
										matched = 1;
										break;
									}
									else
									{	//The later pitch is closer to the unmatched lyric text
										eof_set_note_name(sp, EOF_TRACK_VOCALS, ctr2 + 1, linkptr->text);	//Update the text on this lyric
										matched = 1;
										break;
									}
								}
							}
							else
							{	//There is not another lyric pitch that follows this one
								eof_set_note_name(sp, EOF_TRACK_VOCALS, ctr2, linkptr->text);	//Update the text on this lyric
								matched = 1;
								break;
							}
						}//If this lyric pitch has no text assigned to it yet
					}//For each lyric pitch in the EOF_SONG structure
#ifdef GH_IMPORT_DEBUG
					if(thispos)
					{	//If a match was determined
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tDecided match:  Text = \"%s\"\tPosition = %lu", linkptr->text, thispos);
						eof_log(eof_log_string, 1);
					}
#endif
				}//If there was not a lyric position that matched
				if(!matched)
				{	//If there was still no match found, add it as a pitchless lyric
					(void) eof_track_add_create_note(sp, EOF_TRACK_VOCALS, 0, voxstart, 10, 0, linkptr->text);
					break;
				}
			}//If this checksum matches the one in the list
		}//For each link in the lyric checksum list (until a match has been made)
	}//For each block of lyric text data
	if(arraysize)
	{	//If memory was allocated by eof_gh_process_section_header()
		free(arrayptr);
		arrayptr = NULL;
	}
	eof_process_gh_lyric_text_u(sp);	//Filter and convert hyphenating characters where appropriate

//Read lyric phrases
	fb->index = 0;	//Seek to the beginning of the file buffer
	tracknum = sp->track[EOF_TRACK_VOCALS]->tracknum;
	tp = sp->vocal_track[tracknum];		//Store the pointer to the vocal track to simplify things
	(void) snprintf(buffer, sizeof(buffer) - 1, "%s_vocals_phrases", songname);
#ifdef GH_IMPORT_DEBUG
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  Looking for section \"%s\"", buffer);
	eof_log(eof_log_string, 1);
#endif
	arraysize = eof_gh_process_section_header(fb, buffer, &arrayptr, qbindex);	//Parse the location of the 1D arrays of section data
	for(ctr = 0; ctr < arraysize; ctr++)
	{	//For each 1D array of voxnote data
		assert(arrayptr != NULL);	//Put an assertion here to resolve a false positive with Coverity
		numphrases = eof_gh_read_array_header(fb, arrayptr[ctr], qbindex);	//Process the array header (get size and seek to first data value)
		if(numphrases % 2)
		{	//The value in numphrases is the number of dwords used to define this vocal phrases array (each phrase should be 2 dwords in size)
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Error:  Invalid vocal phrase note array size (%lu)", numphrases);
			eof_log(eof_log_string, 1);
			free(arrayptr);	//Even though arrayptr could not be NULL if arraysize was nonzero and the for loop was entered, add this check to avoid a false positive with Coverity
			return -1;
		}
		numphrases /= 2;	//Determine the number of vocal phrases that are defined
#ifdef GH_IMPORT_DEBUG
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  \tNumber of vocal phrases = %lu", numphrases);
		eof_log(eof_log_string, 1);
#endif
		for(ctr2 = 0; ctr2 < numphrases; ctr2++)
		{	//For each vocal phrase in the section
			if(eof_filebuffer_get_dword(fb, &phrasestart))	//Read the phrase position
			{	//If there was an error reading the next 4 byte value
				eof_log("\t\tError:  Could not vocal phrase position", 1);
				free(arrayptr);	//Even though arrayptr could not be NULL if arraysize was nonzero and the for loop was entered, add this check to avoid a false positive with Coverity
				return -1;
			}
			fb->index += 4;	//Skip 4 bytes of uninteresting data
#ifdef GH_IMPORT_DEBUG
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tLyric phrase at %lu ms", phrasestart);
			eof_log(eof_log_string, 1);
#endif
			(void) eof_vocal_track_add_line(tp, phrasestart, phrasestart);	//Add the phrase with a temporary end position
		}
	}
	eof_process_gh_lyric_phrases(sp);	//Create proper end positions for each lyric phrase

//Cleanup
	if(arraysize)
	{	//If memory was allocated by eof_gh_process_section_header()
		free(arrayptr);
	}
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
	char filename[101], songname[101], buffer[101], forcestrum = 0;
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
			if((size_t)index >= sizeof(filename))
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
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  Internal file name is \"%s\"", filename);
			eof_log(eof_log_string, 1);
#endif
			break;
		}
		else
		{
			eof_log("Error:  Failed to locate internal file name", 1);
			eof_filebuffer_close(fb);
			return NULL;
		}
	}
//Parse the song name
	for(index = 0; ((size_t)index < sizeof(filename) - 1) && (filename[index] != '\0') && (filename[index] != '.'); index++)
	{	//For each character in the file name until the end of the string or a period is reached (and while a buffer overflow won't occur)
		songname[index] = filename[index];	//Copy the character
	}
	songname[index] = '\0';	//Terminate the string
#ifdef GH_IMPORT_DEBUG
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  Internal song name is \"%s\"", songname);
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
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  QB header located at file position 0x%lX", qbindex);
	eof_log(eof_log_string, 1);
#endif

//Process the fretbar section to create the tempo map
	(void) snprintf(buffer, sizeof(buffer) - 1, "%s_fretbars", songname);	//Build the section name
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
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  \tNumber of fretbars = %lu", numbeats);
		eof_log(eof_log_string, 1);
#endif
		if(numbeats < 3)
		{
			eof_log("Error:  Invalid number of fretbars", 1);
			eof_filebuffer_close(fb);
			eof_destroy_song(sp);
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
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  \t\tFretbar %lu position = %lu", ctr2+1, dword);
				eof_log(eof_log_string, 1);
#endif
			}
		}//For each beat in the array
	}//For each 1D array of fretbar data
	if(arraysize)
	{	//If memory was allocated by eof_gh_process_section_header()
		free(arrayptr);	//Free the memory used to store the 1D arrays of section data
		arrayptr = NULL;
	}

	if(eof_use_ts)
	{	//If the user opted to import TS changes
//Process the timesig section to load time signatures
		fb->index = 0;	//Seek to the beginning of the file buffer
		(void) snprintf(buffer, sizeof(buffer) - 1, "%s_timesig", songname);	//Build the section name
		arraysize = eof_gh_process_section_header(fb, buffer, &arrayptr, qbindex);	//Parse the location of the 1D arrays of section data
		for(ctr = 0; ctr < arraysize; ctr++)
		{	//For each 1D array of timesig data
			assert(arrayptr != NULL);	//Put an assertion here to resolve a false positive with Coverity
			numsigs = eof_gh_read_array_header(fb, arrayptr[ctr], qbindex);	//Process the array header (get size and seek to first data value)
			if(numsigs % 3)
			{	//The value in numsigs is the number of dwords used to define this timesig array (each signature should be 3 dwords in size)
					eof_log("Error:  Invalid time signature array size", 1);
					eof_destroy_song(sp);
					eof_filebuffer_close(fb);
					free(arrayptr);	//Even though arrayptr could not be NULL if arraysize was nonzero and the for loop was entered, add this check to resolve a false positive with Coverity
					return NULL;
			}
			numsigs /= 3;	//Determine the number of time signatures that are defined
#ifdef GH_IMPORT_DEBUG
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  \tNumber of timesigs = %lu", numsigs);
			eof_log(eof_log_string, 1);
#endif
			for(ctr2 = 0; ctr2 < numsigs; ctr2++)
			{	//For each time signature in the array
				if(eof_filebuffer_get_dword(fb, &dword))
				{	//If there was an error reading the next 4 byte value
					eof_log("Error:  Could not read time signature position", 1);
					eof_destroy_song(sp);
					eof_filebuffer_close(fb);
					free(arrayptr);	//Even though arrayptr could not be NULL if arraysize was nonzero and the for loop was entered, add this check to resolve a false positive with Coverity
					return NULL;
				}
				if(ctr2 && (dword <= lastsig))
				{	//If this beat doesn't come after the previous beat
					eof_log("Error:  Corrupted time signature", 1);
					eof_destroy_song(sp);
					eof_filebuffer_close(fb);
					free(arrayptr);	//Even though arrayptr could not be NULL if arraysize was nonzero and the for loop was entered, add this check to resolve a false positive with Coverity
					return NULL;
				}
				if(eof_filebuffer_get_dword(fb, &tsnum) || eof_filebuffer_get_dword(fb, &tsden))
				{	//If there was an error reading the next 8 bytes (numerator and denominator)
					eof_log("Error:  Could not read time signature", 1);
					eof_destroy_song(sp);
					eof_filebuffer_close(fb);
					free(arrayptr);	//Even though arrayptr could not be NULL if arraysize was nonzero and the for loop was entered, add this check to resolve a false positive with Coverity
					return NULL;
				}
#ifdef GH_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  \t\tTime signature %lu position = %lu (%lu/%lu)", ctr2, dword, tsnum, tsden);
				eof_log(eof_log_string, 1);
#endif
				for(ctr3 = 0; ctr3 < sp->beats; ctr3++)
				{	//For each beat in the song
					if(dword == sp->beat[ctr3]->pos)
					{	//If this time signature is positioned at this beat marker
						(void) eof_apply_ts(tsnum,tsden,ctr3,sp,0);	//Apply the signature
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
		if(arraysize)
		{	//If memory was allocated by eof_gh_process_section_header()
			free(arrayptr);	//Free the memory used to store the 1D arrays of section data
		}
	}//If the user opted to import TS changes
	eof_calculate_tempo_map(sp);	//Build the tempo map based on the beat time stamps

	eof_clear_input();
	if(alert(NULL, "Import the chart's original HOPO OFF notation?", NULL, "&Yes", "&No", 'y', 'n') == 1)
	{	//If user opts to have all non HOPO notes marked for forced strumming
		forcestrum = 1;
	}

//Read instrument tracks
	for(ctr = 0; ctr < EOF_NUM_GH_INSTRUMENT_SECTIONS_QB; ctr++)
	{	//For each known guitar hero instrument difficulty section
		fb->index = 0;	//Rewind to beginning of file buffer
		(void) eof_gh_read_instrument_section_qb(fb, sp, songname, &eof_gh_instrument_sections_qb[ctr], qbindex, forcestrum);	//Import notes from the section
	}

//Read star power sections
	for(ctr = 0; ctr < EOF_NUM_GH_SP_SECTIONS_QB; ctr++)
	{	//For each known guitar hero star power section
		fb->index = 0;	//Rewind to beginning of file buffer
		(void) eof_gh_read_sp_section_qb(fb, sp, songname, &eof_gh_sp_sections_qb[ctr], qbindex);	//Import star power section
	}

//Read tap (slider) sections
	for(ctr = 0; ctr < EOF_NUM_GH_TAP_SECTIONS_QB; ctr++)
	{	//For each known guitar hero tap section
		fb->index = 0;	//Rewind to beginning of file buffer
		(void) eof_gh_read_tap_section_qb(fb, sp, songname, &eof_gh_tap_sections_qb[ctr], qbindex);	//Import tap section
	}

//Read sections
	(void) eof_gh_read_sections_qb(fb, sp);

//Read vocal track
	(void) eof_gh_read_vocals_qb(fb, sp, songname, qbindex);
	eof_filebuffer_close(fb);	//Close the file buffer

	return sp;
}

struct QBlyric *eof_gh_read_section_names(filebuffer *fb)
{
	unsigned long checksum, index2, nameindex, ctr;
	unsigned char sectionid_ASCII[] = {0x22, 0x0D, 0x0A};		//This hex sequence is between each section name entry for ASCII text encoded GH files
	unsigned char sectionid_UNI[] = {0x00, 0x22, 0x00, 0x0A};	//This hex sequence is between each section name entry for ASCII text encoded GH files
	unsigned char *sectionid, char_size;
	size_t section_id_size;
	unsigned char quote_rewind;	//The number of bytes before a section name's opening quote mark its checknum exists
	char *buffer, checksumbuff[9], checksumbuffuni[18], *name;
	struct QBlyric *head = NULL, *tail = NULL, *linkptr = NULL, *temp;	//Used to maintain the linked list matching section names with checksums
	char abnormal_markers = 0;	//Normally, section names don't begin with "\L", but some files have them with this prefix
	unsigned long lastseekstartpos;
		//Some GH files put all languages together instead of in separate parts, this is used to store the position before a seek so if a duplicate checksum (same
		//section as a previous one in another language) is found, the seek position is restored and function returns, so next call can parse the next language
	unsigned long lastfoundpos;	//Stores the last found instance of the section ID, in case it turns out not to be a section and should be skipped

	eof_log("eof_gh_read_section_names() entered", 1);

	if(!fb)
		return NULL;

	if(eof_gh_unicode_encoding_detected)
	{	//If Unicode encoding was found when importing lyrics, the section names will also be in Unicode
		section_id_size = sizeof(sectionid_UNI);
		sectionid = sectionid_UNI;
		quote_rewind = 18;
		char_size = 2;	//A Unicode character is two bytes
	}
	else
	{	//ASCII encoding is assumed
		section_id_size = sizeof(sectionid_ASCII);
		sectionid = sectionid_ASCII;
		quote_rewind = 9;
		char_size = 1;
	}

//Find the section names and checksums
	lastseekstartpos = fb->index;	//Back up the buffer position before each seek
	while(eof_filebuffer_find_bytes(fb, sectionid, section_id_size, 1) > 0)
	{	//While there are section name entries
		lastfoundpos = fb->index;
		for(index2 = char_size; (index2 <= fb->index) && (fb->buffer[fb->index - index2 + char_size - 1] != '\"'); index2 += char_size);	//Find the opening quotation mark for this string
		if(index2 > fb->index)
		{	//If the opening quotation mark wasn't found
			eof_destroy_qblyric_list(head);
			return NULL;
		}
		if(fb->index - index2 >= quote_rewind)
		{	//If there is enough buffered data before this position to allow for an 8 character checksum and a space character
			//Find and parse the checksum for this section name
			fb->index = fb->index - index2 - quote_rewind;	//Seek to the position of the section name checksum
			if(eof_gh_unicode_encoding_detected)
			{	//If this GH file is in Unicode, convert the letters to ASCII by skipping the leading 0 byte of each
				if(eof_filebuffer_memcpy(fb, checksumbuffuni, 16) == EOF)	//Read the Unicode checksum into a buffer
				{
					eof_log("\t\tError:  Could not read Unicode section name checksum", 1);
					eof_destroy_qblyric_list(head);
					return NULL;
				}
				for(ctr = 0; ctr < 8; ctr++)
				{	//For each of the 8 Unicode characters
					checksumbuff[ctr] = checksumbuffuni[(2 * ctr) + 1];	//Keep the second byte of each Unicode character
				}
			}
			else
			{
				if(eof_filebuffer_memcpy(fb, checksumbuff, 8) == EOF)	//Read the checksum into a buffer
				{
					eof_log("\t\tError:  Could not read section name checksum", 1);
					eof_destroy_qblyric_list(head);
					return NULL;
				}
			}
			checksumbuff[8] = '\0';	//Null terminate the buffer
			(void) sscanf(checksumbuff, "%8lX", &checksum);	//Convert the hex string to unsigned decimal format (sscanf is width limited to prevent buffer overflow)
			if(fb->buffer[fb->index + eof_gh_unicode_encoding_detected] != ' ')
			{	//If the expected space character is not found at this position
				eof_log("\t\tError:  Malformed section name checksum, skipping.", 1);
				fb->index = lastfoundpos + 1;	//Seek just beyond the last search hit so the next one can be found
				continue;	//Look for next section
			}
			fb->index += (2 * char_size);	//Seek past the space character and opening quotation mark

			//Check if this checksum was already read (one language of sections has been parsed), and if so, revert the buffer position to before the last seek operation, return section list
			for(temp = head; temp != NULL; temp = temp->next)
			{	//For each section name parsed in this function call
				if(temp->checksum == checksum)
				{	//If the checksum just read already exists in the linked list
					fb->index = lastseekstartpos;	//Revert to position before the most recent seek
					return head;	//Stop reading sections
				}
			}

			//Parse the section name string
			buffer = malloc((size_t)index2);		//This buffer will be large enough to contain the string
			if(buffer == NULL)
			{	//If the memory couldn't be allocated
				eof_log("\t\tError:  Cannot allocate memory", 1);
				eof_destroy_qblyric_list(head);
				return NULL;
			}
			if(eof_filebuffer_memcpy(fb, buffer, (size_t)index2 - 1) == EOF)	//Read the section name string into a buffer
			{
				eof_log("\t\tError:  Could not read section name text", 1);
				free(buffer);
				eof_destroy_qblyric_list(head);
				return NULL;
			}
			if(eof_gh_unicode_encoding_detected)
			{	//If this GH file is in Unicode, convert the letters to ASCII by skipping the leading 0 byte of each
				for(ctr = 0; ctr < index2 / char_size; ctr++)
				{	//For each of the Unicode characters
					buffer[ctr] = buffer[(ctr * char_size) + 1];	//Keep the second byte of each Unicode character
				}
				buffer[ctr - 1] = '\0';	//Terminate the string
			}
			else
			{
				buffer[index2 - 1] = '\0';	//Terminate the string
			}
			nameindex = 0;	//This is the index into the buffer that will point ahead of any leading gibberish the section name string may contain
			if(strlen(buffer) >= 2)
			{	//If this string is at least two characters long
				if(buffer[nameindex] == '\\')
				{	//If this string begins with a backslash
					nameindex++;
					if(buffer[nameindex] == 'L')
					{	//If it follows with an 'L' character, this is probably a lyric entry string
						abnormal_markers = 1;	//A few QB files prefix section names with "\L", requiring all such strings to be parsed in case they are sections
						nameindex++;
						if(buffer[nameindex] == '=')
						{	//If it follows with a '=' character, this is definitely a lyric entry string
							free(buffer);
							fb->index++;	//Seek past the closing quotation mark in this section name to allow the next loop iteration to look for the next section name
							continue;		//Skip it
						}
						nameindex--;	//Rewind one character
					}
					nameindex++;	//Otherwise skip the character that follows the backslash (expected to be the 'u' character)
				}
				if(buffer[nameindex] == '[')
				{	//If there is an open bracket
					nameindex++;	//Skip past it
					while(buffer[nameindex] != ']')
					{	//Until the closing bracket is found
						if(buffer[nameindex] == '\0')
						{	//If the end of the string is reached unexpectedly
							eof_log("\t\tError:  Malformed section name string", 1);
							free(buffer);
							eof_destroy_qblyric_list(head);
							return NULL;
						}
						nameindex++;
					}
					nameindex++;	//Skip past the closing bracket
				}
				name = malloc(strlen(&buffer[nameindex]) + 1);	//Allocate a new buffer large enough to store the important part of the section name string
				if(!name)
				{	//If the memory couldn't be allocated
					eof_log("\t\tError:  Cannot allocate memory", 1);
					free(buffer);
					eof_destroy_qblyric_list(head);
					return NULL;
				}
				strcpy(name, &buffer[nameindex]);	//Copy the clean section name into the new buffer

#ifdef GH_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tPotential section name = \"%s\"\tchecksum = 0x%08lX", name, checksum);
				eof_log(eof_log_string, 1);
#endif

				//Store the section name and checksum pair in the linked list
				linkptr = malloc(sizeof(struct QBlyric));	//Allocate a new link, initialize it and insert it into the linked list
				if(!linkptr)
				{
					eof_log("\t\tError:  Cannot allocate memory", 1);
					free(buffer);
					free(name);
					eof_destroy_qblyric_list(head);
					return NULL;
				}
				linkptr->checksum = checksum;
				linkptr->text = name;
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
			}//If this string is at least two characters long

			fb->index++;	//Seek past the closing quotation mark in this section name to allow the next loop iteration to look for the next section name
			free(buffer);	//Free the temporary buffer that stored the raw section name string
			buffer = NULL;
			if(!abnormal_markers && ((size_t)fb->index + 4 < fb->size))
			{	//If there are at least four more bytes in the buffer (and no strings beginning with "\L" were parsed)
				if((fb->buffer[fb->index] == 0x0D) && (fb->buffer[fb->index + 1] == 0x0A) && (fb->buffer[fb->index + 2] == 0x0D) && (fb->buffer[fb->index + 3] == 0x0A))
				{	//If those bytes are 0x0D, 0x0A, 0x0D, 0x0A
					break;	//This marks the formal end of the section names
				}
			}
		}//If there is enough buffered data before this position to allow for an 8 character checksum and a space character
		lastseekstartpos = fb->index;	//Back up the buffer position before each seek
	}//While there are section name entries

	return head;
}

EOF_SONG *eof_sections_list_all_ptr;
char *eof_sections_list_all(int index, int * size)
{
	if(index < 0)
	{	//Signal to return the list count
		if(!eof_sections_list_all_ptr)
		{
			*size = 0;
		}
		else
		{
			*size = eof_sections_list_all_ptr->text_events;
		}
	}
	else
	{	//Return the specified list item
		return eof_sections_list_all_ptr->text_event[index]->text;
	}
	return NULL;
}

int eof_gh_read_sections_note(filebuffer *fb, EOF_SONG *sp)
{
	unsigned long numsections, checksum, dword, ctr, lastsectionpos = 0;
	char matched, sectionsfound = 0;
	int prompt;
	struct QBlyric *head = NULL, *linkptr = NULL;	//Used to maintain the linked list matching section names with checksums

	if(!fb || !sp)
		return -1;

	eof_log("eof_gh_read_sections_note() entered", 1);
	fb->index = 0;	//Rewind to beginning of file buffer

	while(1)
	{	//Until the user accepts a language of section names
		fb->index = lastsectionpos;	//Seek back to the position that was reached by the last search for section names
		head = eof_gh_read_section_names(fb);	//Read the section names and their checksums into a linked list
		if(head)
		{	//Section names were found
			lastsectionpos = fb->index;	//Store the current buffer position, which will be lost when seeking to the section positions below

		//Find the section positions
			fb->index = 0;  //Rewind to beginning of file buffer
			if(eof_filebuffer_find_checksum(fb, eof_gh_checksum("guitarmarkers")))	//Seek one byte past the "guitarmarkers" header
			{	//If the "guitarmarkers" section couldn't be found
				eof_log("Error:  Failed to locate \"guitarmarkers\" header", 1);
				eof_destroy_qblyric_list(head);
				return -1;
			}
			if(eof_filebuffer_get_dword(fb, &numsections))	//Read the number of sections
			{	//If there was an error reading the next 4 byte value
				eof_log("Error:  Could not read number of sections", 1);
				eof_destroy_qblyric_list(head);
				return -1;
			}
		#ifdef GH_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tNumber of sections = %lu", numsections);
				eof_log(eof_log_string, 1);
		#endif
			fb->index += 4;	//Seek past the next 4 bytes, which is a checksum for the game-specific guitarmarkers subheader
			if(eof_filebuffer_get_dword(fb, &dword))	//Read the size of the section
			{	//If there was an error reading the next 4 byte value
				eof_log("Error:  Could not read section size", 1);
				eof_destroy_qblyric_list(head);
				return -1;
			}
			if(dword != 8)
			{	//Each section is expected to be 8 bytes long
				eof_log("Error:  Section size is not 8", 1);
				eof_destroy_qblyric_list(head);
				return -1;
			}
			for(ctr = 0; ctr < numsections; ctr++)
			{	//For each section in the chart file
				if(eof_filebuffer_get_dword(fb, &dword))
				{	//If there was an error reading the next 4 byte value
					eof_log("Error:  Could not read section timestamp", 1);
					eof_destroy_qblyric_list(head);
					return -1;
				}
				if(eof_filebuffer_get_dword(fb, &checksum))
				{	//If there was an error reading the next 4 byte value
					eof_log("Error:  Could not read section checksum", 1);
					eof_destroy_qblyric_list(head);
					return -1;
				}

		//Match this section position with a name
				matched = 0;
				for(linkptr = head; (linkptr != NULL) && !matched; linkptr = linkptr->next)
				{	//For each link in the sections checksum list (until a match has been made)
					if(linkptr->checksum == checksum)
					{	//If this checksum matches the one in the list
						long beatnum;

						eof_chart_length = dword;	//Satisfy eof_get_beat() by ensuring this variable isn't smaller than the looked up timestamp
						beatnum = eof_get_beat(sp, dword);	//Get the beat immediately at or before this section
						if(beatnum >= 0)
						{	//If there is such a beat
							char buffer2[256] = {0};

		#ifdef GH_IMPORT_DEBUG
							(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tSection:  Position = %lums, checksum = 0x%08lX: %s", dword, checksum, linkptr->text);
							eof_log(eof_log_string, 1);
		#endif
							(void) snprintf(buffer2, sizeof(buffer2) - 1, "[section %s]", linkptr->text);	//Alter the section name formatting
							(void) eof_song_add_text_event(sp, beatnum, buffer2, 0, 0, 0);	//Add the text event
						}
						break;
					}
				}
			}//For each section in the chart file

		//Cleanup
			eof_destroy_qblyric_list(head);
		}//Section names were found

	//Prompt user
		if(sp->text_events)
		{	//If there were practice sections loaded
			sectionsfound = 1;	//At least one practice section was found in this file
			eof_sections_list_all_ptr = sp;	//eof_sections_list_all() will list the events from this temporary chart
			eof_color_dialog(eof_all_sections_dialog, gui_fg_color, gui_bg_color);
			centre_dialog(eof_all_sections_dialog);
			prompt = eof_popup_dialog(eof_all_sections_dialog, 0);
			if(prompt == 2)
			{	//User opted to keep the loaded practice sections
				return 1;	//Return success
			}
			for(ctr = sp->text_events; ctr > 0; ctr--)
			{	//For each text event (in reverse order)
				eof_song_delete_text_event(sp, ctr-1);	//Delete it
			}
			if(prompt == 4)
			{	//User opted to skip the loading of practice sections
				return 0;	//Return cancellation
			}
		}
		else
		{	//There were no sections loaded during this loop iteration
			if(sectionsfound)
			{	//If a different iteration found sections, alert the user and seek to beginning of buffer
				allegro_message("There are no other languages detected");
				lastsectionpos = 0;	//The next loop iteration will rewind to beginning of file buffer (so the next pass can load the first language of sections again)
			}
			else
			{	//There are no sections found in this file
				break;	//Break from loop and return
			}
		}
	}//Until the user accepts a language of section names

	return 0;	//Return cancellation/no sections
}

int eof_gh_read_sections_qb(filebuffer *fb, EOF_SONG *sp)
{
	unsigned long checksum, dword, ctr, findpos, findpos2, lastsectionpos = 0;
	char sectionsfound = 0, validated, found;
	int prompt;
	struct QBlyric *head = NULL, *linkptr = NULL;	//Used to maintain the linked list matching section names with checksums
	filebuffer *sections_file;
	int done = 0, retval = 0;

	if(!fb || !sp)
		return -1;

	eof_log("eof_gh_read_sections_qb() entered", 1);
	sections_file = fb;	//By default, check for sections in the main chart file
	sections_file->index = 0;	//Rewind to beginning of file buffer

	while(!done)
	{	//Until the user accepts a language of section names
		sections_file->index = lastsectionpos;				//Seek back to the position that was reached by the last search for section names
		head = eof_gh_read_section_names(sections_file);	//Read the section names and their checksums into a linked list
		if(head)
		{	//Section names were found
			lastsectionpos = sections_file->index;	//Store the current buffer position within the file being checked for section names, which will be lost when seeking to the section positions below

		//Find the section timestamps
			for(linkptr = head; linkptr != NULL; linkptr = linkptr->next)
			{	//For each link in the sections checksum list
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH: \tSearching for position of section \"%s\" (checksum 0x%08lX)", linkptr->text, linkptr->checksum);
				eof_log(eof_log_string, 1);
				sections_file->index = 0;	//Rewind to beginning of chart file buffer
				found = 0;	//Reset this boolean condition
				while(1)
				{	//Search for each instance of the section string checksum
					if(eof_filebuffer_find_checksum(sections_file, linkptr->checksum))	//Find the section string checksum in the buffer
					{	//If the checksum wasn't found
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tCouldn't find position data for section \"%s\", it is probably a lyric", linkptr->text);
						eof_log(eof_log_string, 1);
						break;	//Skip looking for this section's timestamp
					}
					findpos = sections_file->index;	//Store the section string checksum match position
					validated = 0;		//Reset this boolean condition
					if(!eof_filebuffer_get_dword(sections_file, &dword) && (dword == 0) && (sections_file->index >= 20))
					{	//If the dword following the string checksum was successfully read, the value was 0, and the buffer can be rewound at least 20 bytes
						sections_file->index -= 20;	//Rewind 5 dwords
						if(!eof_filebuffer_get_dword(sections_file, &dword) && (dword == 0x00201C00))
						{	//If the 3rd dword before the string checksum was succesfully read and the value was 0x00201C00 (new section header)
							validated = 1;	//Consider this to be the appropriate entry matching the section string checksum with its section checksum
						}
					}
					if(validated)
					{
						if(!eof_filebuffer_get_dword(sections_file, &checksum))
						{	//If the checksum for the practice section could be read
							fb->index = 0;	//Rewind to beginning of chart file buffer (the external file, if used, contains its own section name and checksum, and a referring checksum used in the main chart file)
							while(1)
							{	//Search for each instance of the practice section checksum
								if(eof_filebuffer_find_checksum(fb, checksum))	//Find the practice section checksum in the buffer
								{	//If the practice section checksum was not found
									break;	//Exit to next outer loop to continue looking for other instances of the section string checksum
								}
								findpos2 = fb->index;	//Store the practice section checksum match position
								validated = 0;	//Reset this boolean condition
								if(!eof_filebuffer_get_dword(fb, &dword) && (dword == 0) && (fb->index >= 16))
								{	//If the dword following the string checksum was successfully read, the value was 0, and the buffer can be rewound at least 16 bytes
									fb->index -= 16;	//Rewind 4 dwords
									if(!eof_filebuffer_get_dword(fb, &dword) && (dword == 0x00011A00) && (fb->index >= 12))
									{	//If the 3rd dword before the string checksum was succesfully read, the value was 0x00011A00 (alternate 2D array section header) and the buffer can be rewound at least 24 bytes
										fb->index -= 12;	//Rewind 3 dwords
										validated = 1;
									}
								}
								if(validated)
								{
									if(!eof_filebuffer_get_dword(fb, &dword))	//Read the timestamp
									{	//If the timestamp was successfully read
										long beatnum;

										eof_chart_length = dword;	//Satisfy eof_get_beat() by ensuring this variable isn't smaller than the looked up timestamp
										beatnum = eof_get_beat(sp, dword);	//Get the beat immediately at or before this section
										if(beatnum >= 0)
										{	//If there is such a beat
											char buffer2[256] = {0};

#ifdef GH_IMPORT_DEBUG
											(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tSection:  Position = %lums, checksum = 0x%08lX: %s", dword, checksum, linkptr->text);
											eof_log(eof_log_string, 1);
#endif
											(void) snprintf(buffer2, sizeof(buffer2) - 1, "[section %s]", linkptr->text);	//Alter the section name formatting
											(void) eof_song_add_text_event(sp, beatnum, buffer2, 0, 0, 0);	//Add the text event
										}
										found = 1;
										break;	//Break from practice section search loop
									}
								}
								fb->index = findpos2;	//Restore the buffer position for the next iteration of this loop
							}
						}//If the checksum for the practice section could be read
					}
					if(found)
					{	//If the practice section's timestamp was imported
						break;	//Break from section string search loop
					}
					fb->index = findpos;	//Restore the buffer position for the next iteration of this loop
				}//Search for each instance of the section string checksum
			}//For each link in the sections checksum list

		//Cleanup
			linkptr = head;
			while(linkptr != NULL)
			{	//While there are links remaining in the list
				free(linkptr->text);
				head = linkptr->next;	//The new head link will be the next link
				free(linkptr);			//Free the head link
				linkptr = head;			//Advance to the new head link
			}
		}//Section names were found

	//Prompt user
		if(sp->text_events)
		{	//If there were practice sections loaded
			sectionsfound = 1;	//At least one practice section was found in this file
			eof_sections_list_all_ptr = sp;	//eof_sections_list_all() will list the events from this temporary chart
			eof_color_dialog(eof_all_sections_dialog, gui_fg_color, gui_bg_color);
			centre_dialog(eof_all_sections_dialog);
			prompt = eof_popup_dialog(eof_all_sections_dialog, 0);
			if(prompt == 2)
			{	//User opted to keep the loaded practice sections
				if(sections_file != fb)
				{	//If an external section names file was buffered
					eof_filebuffer_close(sections_file);	//Close that file buffer
				}
				retval = 1;	//Return success
				done = 1;	//Exit loop
			}
			for(ctr = sp->text_events; ctr > 0; ctr--)
			{	//For each text event (in reverse order)
				eof_song_delete_text_event(sp, ctr-1);	//Delete it
			}
			if(prompt == 4)
			{	//User opted to skip the loading of practice sections
				if(sections_file != fb)
				{	//If an external section names file was buffered
					eof_filebuffer_close(sections_file);	//Close that file buffer
				}
				retval = -2;	//Return cancellation
				done = 1;		//Exit loop
			}
		}
		else
		{	//There were no sections loaded during this loop iteration
			if(sectionsfound)
			{	//If a different iteration found sections, alert the user and seek to beginning of buffer
				allegro_message("There are no other languages detected");
				lastsectionpos = 0;	//The next loop iteration will rewind to beginning of section name file buffer (so the next pass can load the first language of sections again)
			}
			else
			{	//There are no sections found in this file
				eof_log("\tGH: No sections found in chart file", 1);
				if(sections_file != fb)
				{	//If an external section names file was buffered
					eof_filebuffer_close(sections_file);	//Close that file buffer
				}
				while(1)
				{	//Prompt user about browsing for an external file with section names until explicitly declined
					char * sectionfn;

					eof_clear_input();
					if(alert("No section names were found.", "Specify another file PAK or TXT file to try?", NULL, "&Yes", "&No", 'y', 'n') != 1)
					{	//If user opts not to try looking for section names in another file
						eof_show_mouse(NULL);
						eof_cursor_visible = 1;
						eof_pen_visible = 1;
						retval = 0;	//Return no sections
						done = 1;	//Exit outer loop
						break;		//Break from inner loop
					}
					eof_clear_input();

					eof_cursor_visible = 0;
					eof_pen_visible = 0;
					eof_render();
					eof_clear_input();
					sectionfn = ncd_file_select(0, eof_last_eof_path, "Import GH section name file", eof_filter_gh_files);
					eof_clear_input();
					if(sectionfn)
					{	//If the user selected a file
						eof_show_mouse(NULL);
						eof_cursor_visible = 1;
						eof_pen_visible = 1;
						sections_file = eof_filebuffer_load(sectionfn);
						if(sections_file == NULL)
						{	//Section names file failed to buffer
							allegro_message("Error:  Failed to buffer section name file");
						}
						else
						{	//Section names file buffered successfully
							eof_log("\tGH: Searching for sections in user-specified external file", 1);
							lastsectionpos = 0;	//The next loop iteration will rewind to beginning of section name file buffer (so the next pass can load the first language of sections again)
							break;	//Break from while loop
						}//Section names file buffered successfully
					}
				}//Prompt user about browsing for an external file with section names until explicitly declined
			}//There are no sections found in this file
		}//There were no sections loaded during this loop iteration
	}//Until the user accepts a language of section names

	return retval;
}
