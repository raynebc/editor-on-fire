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
#include "undo.h"
#include "utility.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

unsigned long crc32_lookup[256] = {0};	//A lookup table to improve checksum calculation performance
char crc32_lookup_initialized = 0;	//Is set to nonzero when the lookup table is created
char eof_gh_skip_size_def = 0;	//Is set to nonzero if it's determined that the size field in GH headers is omitted
char magicnumber[] = {0x1C,0x08,0x02,0x04,0x10,0x04,0x08,0x0C,0x0C,0x08,0x02,0x04,0x14,0x02,0x04,0x0C,0x10,0x10,0x0C,0x00};	//The magic number is expected 8 bytes into the QB header
unsigned char eof_gh_unicode_encoding_detected;	//Is set to nonzero if determined that the lyric/section text is in Unicode (UTF-16) format
char eof_song_aux_swap = 0;	//Set to nonzero if user opts to import the _song_aux_ sections into PART RHYTHM (ie. in QB format GH charts that have _song_aux_ notes but no _song_rhythmcoop_ notes)
							//or if PART RHYTHM is populated but PART BASS is empty, the user is prompted to import aux to bass instead
unsigned long eof_song_aux_track = EOF_TRACK_KEYS;	//The effective track to which the aux guitar track's notes and star power paths are to be imported

int eof_gh_accent_prompt = 0;	//When the first accented note is parsed, EOF will prompt user whether the chart is from Warriors of Rock,
								// which defines the bits in a different order than Smash Hits
int eof_gh_import_ghost_drum_notice = 0;	//Will be set to nonzero if the user was notified that ghost notes were detected and highlighted during GH import
int eof_gh_import_threshold_prompt = 0;		//When the imported file is determined to be in GH3/GHA format, tracks whether the user opts to use 66/192 or 100/192 quarter notes as the HOPO threshold
int eof_gh_import_gh3_style_sections = 0;	//Will be set to nonzero if GH3 format sections are detected from whichever file is used to import section names, since they are defined differently than in newer games

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

gh_section eof_gh_sp_battle_sections_qb[EOF_NUM_GH_SP_SECTIONS_QB] =
{
	{"_expert_starbattlemode", EOF_TRACK_GUITAR, 0},
	{"_rhythm_expert_starbattlemode", EOF_TRACK_BASS, 0},
	{"_guitarcoop_expert_starbattlemode", EOF_TRACK_GUITAR_COOP, 0},
	{"_rhythmcoop_expert_starbattlemode", EOF_TRACK_RHYTHM, 0},
	{"_drum_expert_starbattlemode", EOF_TRACK_DRUM, 0},
	{"_aux_expert_starbattlemode", EOF_TRACK_KEYS, 0}
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

	reflected_bitmask = 1UL << (numbits - 1);
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
				crc32_lookup[x] = (crc32_lookup[x] << 1) ^ ((crc32_lookup[x] & (1UL << 31)) ? polynomial : 0);
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
	unsigned char checksumarray[4] = {0};

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
		else if(seektype == 2)
		{	//Seek past the last byte of the match
			fb->index++;
		}
		else if(seektype == 0)
		{	//Restore original buffer position
			fb->index = originalpos;
		}
		return 1;	//Return match (if seektype was not 1, the buffer position is left at the byte following the match
	}

	fb->index = originalpos;	//Restore the original buffer position
	return 0;	//Return no match
}

unsigned long eof_filebuffer_count_instances(filebuffer *fb, const void *bytes, size_t searchlen)
{
	unsigned long originalpos, count = 0;

	if(!fb || !bytes || !searchlen)
		return 0;	//Invalid parameters

	originalpos = fb->index;
	fb->index = 0;
	while(eof_filebuffer_find_bytes(fb, bytes, searchlen, 2) == 1)
	{	//For each match that is found
		count++;
	}
	fb->index = originalpos;	//Restore the buffer position to what it was before the searching

	return count;
}

int eof_gh_read_instrument_section_note(filebuffer *fb, EOF_SONG *sp, gh_section *target, char forcestrum)
{
	unsigned long numnotes = 0, dword = 0, ctr, notesize = 0;
	unsigned int length = 0, isexpertplus;
	unsigned char notemask = 0, accentmask = 0, fixednotemask;
	EOF_NOTE *newnote = NULL, *lastnote = NULL;

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
		int ghost = 0;
		unsigned char ghostmask;

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
///If the size field is omitted, it's not yet known whether the expected size is 8 or 9 bytes
		ghostmask = 0;
		if(notesize == 9)
		{	//A variation of the note section is one that contains an extra byte of data (for drum notes it appears to define ghost status)
			if(eof_filebuffer_get_byte(fb, &ghostmask))	//Read the extra data byte
			{	//If there was an error reading the next 1 byte value
				eof_log("\t\tError:  Could not read ghost status byte", 1);
				return -1;
			}
		}

#ifdef GH_IMPORT_DEBUG
		if(ghostmask)
		{	//If this note has any ghost gems
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  \t\tNote %lu position = %lu  length = %u  bitmask = %u (%08lu) accent = %08lu ghost = %08lu", ctr, dword, length, notemask, eof_char_to_binary(notemask), eof_char_to_binary(accentmask), eof_char_to_binary(ghostmask));
		}
		else
		{
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  \t\tNote %lu position = %lu  length = %u  bitmask = %u (%08lu) accent = %08lu", ctr, dword, length, notemask, eof_char_to_binary(notemask), eof_char_to_binary(accentmask));
		}
		eof_log(eof_log_string, 1);
#endif
		isexpertplus = 0;	//Reset this condition
		if(target->tracknum == EOF_TRACK_DRUM)
		{	//In Guitar Hero, lane 6 is bass drum, lane 1 is the right-most lane (ie. 6)
			unsigned long tracknum = sp->track[EOF_TRACK_DRUM]->tracknum;

			notemask |= ghostmask;	//Add any defined ghost gems to the note
			if(ghostmask)
			{	//If this note has any ghost gems
				ghost = 1;
			}

			fixednotemask = notemask;
			fixednotemask &= ~1;	//Clear lane 1 gem
			fixednotemask &= ~32;	//Clear lane 6 gem
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
		else if(target->tracknum == EOF_TRACK_DRUM)
		{	//If this is a drum track
			if((newnote->type == EOF_NOTE_AMAZING) && (length >= 140))
			{	//If this is an expert difficulty note that is at least 140ms long, it should be treated as a drum roll
				int phrasetype = EOF_TREMOLO_SECTION;	//Assume a normal drum roll (one lane)
				unsigned long lastnoteindex = eof_get_track_size(sp, EOF_TRACK_DRUM) - 1;

				if(eof_note_count_colors(sp, EOF_TRACK_DRUM, lastnoteindex) > 1)
				{	//If the new drum note contains gems on more than one lane
					phrasetype = EOF_TRILL_SECTION;		//Consider it a special drum roll (multiple lanes)
				}
				(void) eof_track_add_section(sp, EOF_TRACK_DRUM, phrasetype, 0xFF, dword, dword + length, 0, NULL);
				newnote->flags |= EOF_NOTE_FLAG_CRAZY;	//Mark it as crazy, both so it can overlap other notes and so that it will export with sustain
			}
			if(accentmask)
			{	//If there are any accent status bits
				if(!eof_gh_accent_prompt)
				{	//If the user wasn't prompted about which game this chart is from
					if(alert("Is this chart from any of the following GH games?", "\"Warriors of Rock\", \"Band Hero\", \"Guitar Hero 5\"", "(These use a different accent notation than Smash Hits)", "&Yes", "&No", 'y', 'n') == 1)
					{	//If user indicates the chart is from Warriors of Rock
						eof_gh_accent_prompt = 1;
					}
					else
					{
						eof_gh_accent_prompt = 2;
					}
				}
				if(eof_gh_accent_prompt == 1)
				{	//Warriors of Rock rules for interpreting the accent bitmask
					if(accentmask & 31)
					{	//Bits 0 through 4 of the accent mask are used to indicate accent status for drum gems
						if(accentmask & 1)
						{	//The least significant bit is assumed to be accent status for lane 6
							accentmask &= ~1;	//Clear the bit for lane 1
							accentmask |= 32;	//Set the bit for lane 6
						}
						accentmask &= newnote->note;	//Filter out any bits from the accent mask that don't have gems
						newnote->accent = accentmask;
					}
				}
				else
				{	//Smash Hits rules for interpreting the accent bitmask
					if(accentmask & 15)
					{	//Bits 0 through 3 of the accent mask are used to indicate accent status for drum gems (lanes other than bass drum) in Guitar Hero Smash Hits
						accentmask &= 15;						//Store only the applicable bits (lane 2 through 6) into the note's accent mask
						accentmask <<= 1;						//Shift left one bit to reflect that the first accent bit defines accent for snare, ie. lane 2 instead of bass, ie. lane 1
						if(notemask & 32)
						{	//If the accented note has any gems on lane 6, it's assumed that it inherits the accented status automatically since the game doesn't seem
							// to use a bit to define accent status individually for this lane
							accentmask |= 32;
						}

						accentmask &= newnote->note;			//Filter out any bits from the accent mask that don't have gems
						newnote->accent = accentmask;
					}
				}
			}
			if(ghost)
			{	//If the ghost note was converted into a snare
				newnote->flags |= EOF_NOTE_FLAG_HIGHLIGHT;	//Highlight the note

				if(!eof_gh_import_ghost_drum_notice)
				{
					allegro_message("Detected ghost notes will be highlighted.");
					eof_gh_import_ghost_drum_notice = 1;
				}
			}
		}//If this is a drum track
		if(isexpertplus)
		{	//If this note was determined to be an expert+ drum note
			newnote->flags |= EOF_DRUM_NOTE_FLAG_DBASS;	//Set the double bass flag bit
		}

		//Apply disjointed status if appropriate
		if(lastnote && (lastnote->pos == newnote->pos) && (lastnote->length != newnote->length))
		{	//If there was a previous note, it started at the same time as this note and has a different length
			lastnote->eflags |= EOF_NOTE_EFLAG_DISJOINTED;	//Apply disjointed status to both notes
			newnote->eflags |= EOF_NOTE_EFLAG_DISJOINTED;
		}

		lastnote = newnote;
	}//For each note in the section
	return 1;
}

int eof_gh_read_sp_section_note(filebuffer *fb, EOF_SONG *sp, gh_section *target)
{
	unsigned long numphrases = 0, phrasesize = 0, dword = 0, ctr;
	unsigned int length = 0;

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
		//The section was added
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  Star power section added:  %lums to %lums", dword, dword + length);
		eof_log(eof_log_string, 1);
#endif
	}
	return 1;
}

int eof_gh_read_tap_section_note(filebuffer *fb, EOF_SONG *sp, gh_section *target)
{
	unsigned long numphrases = 0, phrasesize = 0, dword = 0, ctr, length = 0;

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
		if(!eof_track_add_section(sp, target->tracknum, EOF_SLIDER_SECTION, 0xFF, dword, dword + length, 0, NULL))
		{	//If there was an error adding the section
			eof_log("\t\tError:  Could not add tap section", 1);
			return -1;
		}
	}
	return 1;
}

void eof_process_gh_lyric_text_u(EOF_SONG *sp)
{
	unsigned long ctr, ctr2, length, prevlength, index;
	char *string, *prevstring, buffer[EOF_MAX_LYRIC_LENGTH+1] = {0};

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
			if(voxstart < tp->line[ctr].start_pos)
				continue;	//If this lyric is before this line, skip it

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
	unsigned long ctr, ctr2, numvox = 0, voxsize = 0, voxstart = 0, numlyrics = 0, lyricsize = 0, lyricstart = 0, tracknum, phrasesize = 0, numphrases = 0, phrasestart = 0;
	char *lyricbuffer = NULL, matched;
	unsigned int voxlength = 0;
	unsigned char voxpitch = 0;
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
			(void) eof_convert_from_extended_ascii(lyricbuffer, (lyricsize + 2) * 4);
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
				char *name = eof_get_note_name(sp, EOF_TRACK_VOCALS, ctr2);
				if(!name || (name[0] != '+'))
					continue;	//If this lyric pitch already has text assigned to it, skip it

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
		(void) eof_vocal_track_add_line(tp, phrasestart, phrasestart, 0xFF);	//Add the phrase with a temporary end position
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
				(void) ustrncpy(sp->tags->ini_setting[sp->tags->ini_settings], "drum_fallback_blue = True", EOF_INI_LENGTH - 1);
				sp->tags->ini_settings++;
			}
		}
	}

	if(sp)
	{	//If a GH file was loaded
		unsigned long tracknum;
		unsigned long ctr;

		eof_gh_import_sp_cleanup(sp);		//Shorten any star power phrases that end on a note's start position, so the latter isn't included in the phrase (GH uses this logic)
		eof_gh_import_slider_cleanup(sp);	//Likewise, shorten slider (tap) phrases

		if(!eof_disable_backups)
		{	//If the user did not disable automatic backups
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

//Update path variables
		(void) ustrcpy(eof_filename, fn);
		(void) replace_filename(eof_song_path, fn, "", 1024);
		(void) replace_filename(eof_last_eof_path, eof_filename, "", 1024);
		(void) ustrcpy(eof_loaded_song_name, get_filename(eof_filename));
		(void) replace_extension(eof_loaded_song_name, eof_loaded_song_name, "eof", 1024);

//Load guitar.ogg automatically if it's present, otherwise prompt user to browse for audio
		ogg_profile_name = sp->tags->ogg[0].filename;	//Store the pointer to the OGG profile filename to be updated by eof_load_ogg()
		if(!eof_load_ogg(oggfn, 2))	//If user does not provide audio, fail over to using silent audio
		{
			eof_destroy_song(sp);
			eof_free_ucode_table();
			return NULL;
		}
		eof_music_length = alogg_get_length_msecs_ogg_ul(eof_music_track);
		eof_truncate_chart(sp);	//Update the chart length before performing lyric cleanup
		eof_vocal_track_fixup_lyrics(sp, EOF_TRACK_VOCALS, 0);	//Clean up the lyrics
		eof_log("\tGH import completed", 1);
	}//If a GH file was loaded
	eof_free_ucode_table();
	return sp;
}

EOF_SONG * eof_import_gh_note(const char * fn)
{
	EOF_SONG * sp;
	filebuffer *fb;
	unsigned long dword = 0, ctr, ctr2, numbeats = 0, numsigs = 0, lastfretbar = 0, lastsig = 0;
	unsigned char tsnum = 0, tsden = 0;
	char forcestrum = 0;
	char ts_warned = 0;

	eof_log("eof_import_gh_note() entered", 1);
	eof_log("Attempting to import NOTE format Guitar Hero chart", 1);

	eof_gh_accent_prompt = 0;	//Reset this condition
	eof_gh_import_ghost_drum_notice = 0;	//Reset this condition

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
					if(!ts_warned)
					{	//If the user hasn't been warned about this yet
						allegro_message("Warning:  Mid beat time signature detected.  Skipping");
						ts_warned = 1;
					}
					eof_log("\t\tWarning:  Mid beat time signature detected.  Skipping", 1);
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
	unsigned long dword = 0, arraysize, size = 0, ctr;

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
	unsigned long size = 0, offset = 0;

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
	unsigned long numnotes, dword = 0, ctr, ctr2, arraysize, *arrayptr = NULL;
	unsigned int length = 0, isexpertplus;
	unsigned char notemask = 0, accentmask = 0, fixednotemask;
	EOF_NOTE *newnote = NULL, *lastnote = NULL;
	char buffer[101] = {0};
	int destination_track;
	int gh3_format;		//Specifies whether the notes are determined to be defined as 3 dwords each instead of 2
	unsigned long fbindex, lastpos = 0, headersize;
	double threshold = 66.0 / 192.0;

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
		assert(arrayptr != NULL);	//Unneeded assertion to resolve a false positive in Splint
		headersize = eof_gh_read_array_header(fb, arrayptr[ctr], qbindex);	//Process the array header (get size and seek to first data value)
		if(headersize % 2)
		{	//The value in numnotes is the number of dwords used to define this note array (each note should be 2 dwords in size)
			if(headersize % 3)
			{	//However in GH3/GHA format, each note is defined with 3 dwords instead
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Error:  Invalid note array size (%lu)", headersize);
				eof_log(eof_log_string, 1);
				return -1;
			}
		}

		//Offer to import the song_aux notes into the rhythm or bass tracks if thy are empty (they are all imported before the aux track)
		destination_track = target->tracknum;	//By default, use eof_gh_instrument_sections_qb[] to derive the track associated with the section name
		if(strcasestr_spec(target->name, "_song_aux_"))
		{	//If this is one of the auxiliary instrument sections
			if(!eof_song_aux_swap)
			{	//If the user hasn't been prompted to re-target this track
				if(!eof_get_track_size(sp, EOF_TRACK_RHYTHM))
				{	//If the rhythm track is empty
					if(alert(NULL, "Import the auxiliary track into PART RHYTHM instead of PART KEYS?", NULL, "&Yes", "&No", 'y', 'n') == 1)
					{	//If user opts to change the destination track for the auxiliary section
						eof_song_aux_swap = 1;
						eof_song_aux_track = EOF_TRACK_RHYTHM;
						eof_log("\tImporting aux guitar track to rhythm.", 1);
					}
					else
					{
						eof_song_aux_swap = 2;
					}
				}
				else if(!eof_get_track_size(sp, EOF_TRACK_BASS))
				{	//If the rhythm track is populated but the bass track is empty
					if(alert(NULL, "Import the auxiliary track into PART BASS instead of PART KEYS?", NULL, "&Yes", "&No", 'y', 'n') == 1)
					{	//If user opts to change the destination track for the auxiliary section
						eof_song_aux_swap = 1;
						eof_song_aux_track = EOF_TRACK_BASS;
						eof_log("\tImporting aux guitar track to bass.", 1);
					}
					else
					{
						eof_song_aux_swap = 2;
					}
				}
			}
			if(eof_song_aux_swap)
			{	//If the user opted to redirect the aux notes
				destination_track = eof_song_aux_track;
			}
		}

		//Pre-parse the content to detect whether it's GH3/GHA format, where each note is defined as 3 dwords:  timestamp, duration, bitmask
		fbindex = fb->index;	//Record this to rewind after the check
		gh3_format = 1;			//Unless the notes don't follow the expected scheme, assume GH3/GHA format
		for(ctr2 = 0; ctr2 < headersize / 3; ctr2++)
		{	//For each note in the section (in this format, each note is defined with 3 qwords instead of 2)
			if(eof_filebuffer_get_dword(fb, &dword))	//Read the note position
			{	//If there was an error reading the next 4 byte value
				gh3_format = 0;
				break;
			}
			if(ctr2 && (dword < lastpos))
			{	//If this isn't the first note parsed, and the timestamp is smaller then the previous timestamp
				gh3_format = 0;
				break;
			}
			lastpos = dword;	//Remember this timestamp for comparison with the next timestamp
			if(eof_filebuffer_get_dword(fb, &dword))	//Read the note duration
			{	//If there was an error reading the next 4 byte value
				gh3_format = 0;
				break;
			}
			if(eof_filebuffer_get_dword(fb, &dword))	//Read the note bitmask
			{	//If there was an error reading the next 4 byte value
				gh3_format = 0;
				break;
			}
			if(dword > 63)
			{	//If the note bitmask is larger than expected (higher than 6 set bits)
				gh3_format = 0;
				break;
			}
		}
		fb->index = fbindex;	//Restore the file buffer index to parse beginning from the first note
		if(gh3_format)
		{	//This format uses 3 dwords to define each note
			numnotes = headersize / 3;
			if(!eof_gh_import_threshold_prompt)
			{	//If the user wasn't prompted to select a HOPO threshold yet during this import
				eof_gh_import_threshold_prompt = alert("GH3/GHA charts can have one of two HOPO thresholds.", "Which should EOF use?", NULL, "66/192 qnote", "100/192 qnote", 0, 0);
			}
			if(eof_gh_import_threshold_prompt == 2)
			{	//If the user selected the 100/192 threshold now or earlier in the import
				threshold = 100.0 / 192.0;
			}

#ifdef GH_IMPORT_DEBUG
			eof_log("\tGH3/GHA note format detected.", 1);
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tUser selected %s HOPO threshold.", ((eof_gh_import_threshold_prompt == 1) ? "66/192" : "100/192"));
			eof_log(eof_log_string, 1);
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tNumber of notes = %lu", numnotes);
			eof_log(eof_log_string, 1);
#endif
		}
		else
		{	//This format uses 2 dwords to define each note
			numnotes = headersize / 2;	//Determine the number of notes that are defined, assuming it's not GH3/GHA format
#ifdef GH_IMPORT_DEBUG
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  \tNumber of notes = %lu", numnotes);
			eof_log(eof_log_string, 1);
#endif
		}

		///Parse the note content in the detected format
		for(ctr2 = 0; ctr2 < numnotes; ctr2++)
		{	//For each note in the section
			if(gh3_format)
			{
				unsigned long pos = 0;

				if(eof_filebuffer_get_dword(fb, &pos))	//Read the note position
				{	//If there was an error reading the next 4 byte value
					eof_log("\t\tError:  Could not read note position", 1);
					free(arrayptr);
					return -1;
				}
				if(eof_filebuffer_get_dword(fb, &dword))	//Read the note length
				{	//If there was an error reading the next 4 byte value
					eof_log("\t\tError:  Could not read note length", 1);
					free(arrayptr);
					return -1;
				}
				length = dword;	//The logic below expects this variable to hold the length
				if(eof_filebuffer_get_dword(fb, &dword))	//Read the note bitmask
				{	//If there was an error reading the next 4 byte value
					eof_log("\t\tError:  Could not read note bitmask", 1);
					free(arrayptr);
					return -1;
				}
				notemask = dword;	//The logic below expects this variable to hold the note bitmask
				dword = pos;	//The logic below expects this variable to hold the note position
				accentmask = 0;	//Unused
			}
			else
			{
				if(eof_filebuffer_get_dword(fb, &dword))	//Read the note position
				{	//If there was an error reading the next 4 byte value
					eof_log("\t\tError:  Could not read note position", 1);
					free(arrayptr);
					return -1;
				}
				if(eof_filebuffer_get_byte(fb, &accentmask))	//Read the note accent bitmask
				{	//If there was an error reading the next 1 byte value
					eof_log("\t\tError:  Could not read note accent bitmask", 1);
					free(arrayptr);
					return -1;
				}
				if(eof_filebuffer_get_byte(fb, &notemask))	//Read the note bitmask
				{	//If there was an error reading the next 1 byte value
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
			}
#ifdef GH_IMPORT_DEBUG
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  \t\tNote %lu position = %lu  length = %u  bitmask = %u (%08lu) accent = %08lu", ctr2+1, dword, length, notemask, eof_char_to_binary(notemask), eof_char_to_binary(accentmask));
			eof_log(eof_log_string, 1);
#endif
			isexpertplus = 0;	//Reset this condition
			if(destination_track == EOF_TRACK_DRUM)
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
				if((accentmask & 32) && !(notemask & 32))
				{	//If bit 5 of the accent mask is set, but bit 5 of the note mask is not
					isexpertplus = 1;		//Consider this to be double bass
					fixednotemask |= 1;	//Set the lane 1 (bass drum gem)
				}
				notemask = fixednotemask;
			}
			newnote = (EOF_NOTE *)eof_track_add_create_note(sp, destination_track, (notemask & 0x3F), dword, length, target->diffnum, NULL);
			if(newnote == NULL)
			{	//If there was an error adding a new note
				eof_log("\t\tError:  Could not add note", 1);
				free(arrayptr);
				return -1;
			}
			if(sp->track[destination_track]->track_behavior == EOF_GUITAR_TRACK_BEHAVIOR)
			{	//If this is a guitar track, check the accent mask and HOPO status
				if(gh3_format)
				{	//GH3/GHA charts used note proximity and a toggle HOPO marker
					if(notemask & 32)
					{	//Bit 5 is a toggle HOPO status
						newnote->flags ^= EOF_NOTE_FLAG_F_HOPO;	//Toggle the forced HOPO flag
						newnote->flags ^= EOF_NOTE_FLAG_HOPO;
						newnote->note &= 31;	//Clear all bits above 5, as they aren't used in this game
					}
				}
				else
				{	//Other charts explicitly define HOPO on/off
					if(notemask & 0x40)
					{	//Bit 6 is the HOPO status
						newnote->flags |= EOF_NOTE_FLAG_F_HOPO;
						newnote->flags |= EOF_NOTE_FLAG_HOPO;
					}
					else if(forcestrum)
					{	//If the note is not a HOPO, mark it as a forced non HOPO (strum required), but only if the user opted to do so
						newnote->flags |= EOF_NOTE_FLAG_NO_HOPO;
					}
					if((accentmask != 0xF) && !gh3_format)
					{	//"Crazy" guitar/bass notes have an accent mask that isn't 0xF, and the chart wasn't determined to be in GH3/GHA format
						newnote->flags |= EOF_NOTE_FLAG_CRAZY;	//Set the crazy flag bit
					}
				}
			}
			else if(destination_track == EOF_TRACK_DRUM)
			{	//If this is a drum track
				if((newnote->type == EOF_NOTE_AMAZING) && (length >= 140))
				{	//If this is an expert difficulty note that is at least 140ms long, it should be treated as a drum roll
					int phrasetype = EOF_TREMOLO_SECTION;	//Assume a normal drum roll (one lane)
					unsigned long lastnoteindex = eof_get_track_size(sp, EOF_TRACK_DRUM) - 1;

					if(eof_note_count_colors(sp, EOF_TRACK_DRUM, lastnoteindex) > 1)
					{	//If the new drum note contains gems on more than one lane
						phrasetype = EOF_TRILL_SECTION;		//Consider it a special drum roll (multiple lanes)
					}
					(void) eof_track_add_section(sp, EOF_TRACK_DRUM, phrasetype, 0xFF, dword, dword + length, 0, NULL);
					newnote->flags |= EOF_NOTE_FLAG_CRAZY;	//Mark it as crazy, both so it can overlap other notes and so that it will export with sustain
				}
				if(accentmask & 31)
				{	//Bits 0 through 4 of the accent mask are used to indicate accent status for drum gems (lanes other than bass drum) in Guitar Hero Smash Hits
					accentmask &= 31;						//Store only the applicable bits (lane 2 through 6) into the note's accent mask
					accentmask <<= 1;						//Shift left one bit to reflect that the first accent bit defines accent for snare, ie. lane 2 instead of bass, ie. lane 1
					if(notemask & 32)
					{	//If the accented note has any gems on lane 6, it's assumed that it inherits the accented status automatically since the game doesn't seem
						// to use a bit to define accent status individually for this lane
						accentmask |= 32;
					}

					accentmask &= newnote->note;			//Filter out any bits from the accent mask that don't have gems
					newnote->accent = accentmask;
				}
			}
			if(isexpertplus)
			{	//If this note was determined to be an expert+ drum note
				newnote->flags |= EOF_DRUM_NOTE_FLAG_DBASS;	//Set the double bass flag bit
			}

			//Apply disjointed status if appropriate
			if(lastnote && (lastnote->pos == newnote->pos) && (lastnote->length != newnote->length))
			{	//If there was a previous note, it started at the same time as this note and has a different length
				lastnote->eflags |= EOF_NOTE_EFLAG_DISJOINTED;	//Apply disjointed status to both notes
				newnote->eflags |= EOF_NOTE_EFLAG_DISJOINTED;
			}

			lastnote = newnote;
			if(newnote->pos > eof_chart_length)
				eof_chart_length = newnote->pos;	//Keep this variable up to date so the HOPO logic below can work (eof_get_beat() requires eof_chart_length to be correct)
		}//For each note in the section

		eof_track_sort_notes(sp, destination_track);

		//Apply auto HOPO statuses if necessary
		if(gh3_format)
		{	//If the notes were in GH3/GHA format, HOPO status is based on proximity to the previous note
			unsigned long tracksize = eof_get_track_size(sp, destination_track);
			eof_track_sort_notes(sp, destination_track);
			for(ctr2 = 0; ctr2 < tracksize; ctr2++)
			{	//For each of the imported notes
				unsigned long flags = eof_get_note_flags(sp, destination_track, ctr2), flags2;

				if(eof_get_note_type(sp, destination_track, ctr2) != target->diffnum)	//If this isn't one of the notes that was imported for this section's track difficulty
					continue;	//Skip it

				flags2 = flags & ~(EOF_NOTE_FLAG_F_HOPO | EOF_NOTE_FLAG_NO_HOPO);	//Clear these flags so eof_note_is_gh3_hopo() won't use them as the deciding factor
				eof_set_note_flags(sp, destination_track, ctr2, flags2);
				if(eof_note_is_gh3_hopo(sp, destination_track, ctr2, threshold))
				{	//If this note meets the criteria for an auto HOPO (is a single gem not matching the previous note and is within the user specified threshold distance)
					flags ^= EOF_NOTE_FLAG_F_HOPO;	//Toggle the forced HOPO flag
					flags ^= EOF_NOTE_FLAG_HOPO;
				}
				else
				{
					flags &= ~EOF_NOTE_FLAG_HOPO;	//Clear HOPO flag
				}
				if(forcestrum && !(flags & EOF_NOTE_FLAG_F_HOPO))
				{	//If the note was not ultimately a HOPO, and the user opted to import forced strum status
					flags |= EOF_NOTE_FLAG_NO_HOPO;
				}
				eof_set_note_flags(sp, destination_track, ctr2, flags);	//Update the note's flags
			}
		}
	}//For each 1D array of note data

	if(arraysize)
	{	//If memory was allocated by eof_gh_process_section_header()
		free(arrayptr);
	}
	return 1;
}

unsigned long eof_gh_read_sp_section_qb(filebuffer *fb, EOF_SONG *sp, const char *songname, gh_section *target, unsigned long qbindex, int count_only)
{
	unsigned long numphrases, dword = 0, length = 0, ctr, ctr2, arraysize, *arrayptr = NULL, destination_track;
	char buffer[101] = {0};

	if(!fb || !sp || !target || !songname)
		return ULONG_MAX;

	destination_track = target->tracknum;	//By default, the star power phrases will import to their default tracks
	if(strcasestr_spec(target->name, "_aux_expert_star") && eof_song_aux_swap)
	{	//However if an aux guitar star power section is being imported, and the user redirected the aux guitar track to another track
		destination_track = eof_song_aux_track;	//Redirect the star power sections to match
		eof_log("\tRedirecting aux star power sections to match redirected notes.", 1);
	}

	(void) snprintf(buffer, sizeof(buffer) - 1, "%s%s", songname, target->name);
#ifdef GH_IMPORT_DEBUG
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  Looking for section \"%s\"", buffer);
	eof_log(eof_log_string, 1);
#endif
	arraysize = eof_gh_process_section_header(fb, buffer, &arrayptr, qbindex);	//Parse the location of the 1D arrays of section data

	if(count_only)
	{
		if(arraysize)
		{	//If memory was allocated by eof_gh_process_section_header()
			free(arrayptr);
		}
		return arraysize;	//If the calling function only wanted to count the number of SP phrases in the specified section name
	}

	for(ctr = 0; ctr < arraysize; ctr++)
	{	//For each 1D array of star power data
		assert(arrayptr != NULL);	//Unneeded check to resolve false positive in Splint
		numphrases = eof_gh_read_array_header(fb, arrayptr[ctr], qbindex);	//Process the array header (get size and seek to first data value)
		if(numphrases % 3)
		{	//The value in numphrases is the number of dwords used to define this star power array (each phrase should be 3 dwords in size)
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Error:  Invalid star power array size (%lu)", numphrases);
			eof_log(eof_log_string, 1);
			free(arrayptr);
			return ULONG_MAX;
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
				return ULONG_MAX;
			}
			if(eof_filebuffer_get_dword(fb, &length))	//Read the phrase length
			{	//If there was an error reading the next 4 byte value
				eof_log("\t\tError:  Could not read phrase length", 1);
				free(arrayptr);
				return ULONG_MAX;
			}
			fb->index += 4;	//Skip the field indicating the number of notes contained within the star power section
#ifdef GH_IMPORT_DEBUG
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  \t\tPhrase %lu position = %lu  length = %lu", ctr2+1, dword, length);
			eof_log(eof_log_string, 1);
#endif
			if(!eof_track_add_section(sp, destination_track, EOF_SP_SECTION, 0, dword, dword + length, 0, NULL))
			{	//If there was an error adding the section
				eof_log("\t\tError:  Could not add sp section", 1);
				free(arrayptr);
				return ULONG_MAX;
			}

#ifdef GH_IMPORT_DEBUG
			//The section was added
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  \t\tStar power section added:  %lums to %lums", dword, dword + length);
			eof_log(eof_log_string, 1);
#endif
		}
	}//For each 1D array of star power data
	if(arraysize)
	{	//If memory was allocated by eof_gh_process_section_header()
		free(arrayptr);
	}
	return arraysize;
}

int eof_gh_read_tap_section_qb(filebuffer *fb, EOF_SONG *sp, const char *songname, gh_section *target, unsigned long qbindex)
{
	unsigned long numphrases, dword = 0, length = 0, ctr, ctr2, arraysize, *arrayptr = NULL;
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
		assert(arrayptr != NULL);	//Unneeded check to resolve a false positive with Coverity
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
			if(!eof_track_add_section(sp, target->tracknum, EOF_SLIDER_SECTION, 0xFF, dword, dword + length, 0, NULL))
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
	unsigned long ctr, ctr2, numvox, voxstart = 0, voxlength = 0, voxpitch = 0, numphrases, phrasestart = 0, arraysize, *arrayptr = NULL, dword = 0, tracknum;
	EOF_LYRIC *ptr = NULL;
	EOF_VOCAL_TRACK * tp = NULL;
	char buffer[201], matched;
	struct QBlyric *head = NULL, *tail = NULL, *linkptr = NULL;	//Used to maintain the linked list matching lyric text with checksums
	unsigned char lyricid[] = {0x20, 0x22, 0x5C, 0x4C};	//This hex sequence is within each lyric entry between the lyric text and its checksum
	char *newtext = NULL;
	unsigned long checksum = 0, length;

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
		assert(arrayptr != NULL);	//Unneeded check to resolve a false positive with Coverity
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
	}//For each 1D array of voxnote data
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
		assert(arrayptr != NULL);	//Unneeded check to resolve a false positive with Coverity
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
			if(linkptr->checksum != checksum)
				continue;	//If this checksum does not match the one in the list, skip it

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
					char *name = eof_get_note_name(sp, EOF_TRACK_VOCALS, ctr2);

					if(!name || (name[0] != '+'))
						continue;	//If this lyric pitch already has text assigned to it, skip it

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
		assert(arrayptr != NULL);	//Unneeded check to resolve a false positive with Coverity
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
			(void) eof_vocal_track_add_line(tp, phrasestart, phrasestart, 0xFF);	//Add the phrase with a temporary end position
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
	char filename[101] = {0}, songname[101] = {0}, buffer[101], forcestrum = 0;
	unsigned char byte = 0;
	unsigned long index, ctr, ctr2, ctr3, arraysize, *arrayptr = NULL, numbeats, numsigs, tsnum = 0, tsden = 0, dword = 0, lastfretbar = 0, lastsig = 0;
	unsigned long qbindex;	//Will store the file index of the QB header
	char ts_warned = 0;
	unsigned long sp_count, sp_battle_count;
	int import_sp = 1, import_battle_sp = 1, sp_has_conflict = 0, sp_conflicts[EOF_NUM_GH_SP_SECTIONS_QB];

	eof_log("eof_import_gh_qb() entered", 1);
	eof_log("Attempting to import QB format Guitar Hero chart", 1);

//Load the GH file into memory
	fb = eof_filebuffer_load(fn);
	if(fb == NULL)
	{
		eof_log("Error:  Failed to buffer GH file", 1);
		return NULL;
	}

	eof_song_aux_swap = 0;	//Reset these conditions
	eof_gh_import_threshold_prompt = 0;
	eof_gh_import_gh3_style_sections = 0;
	eof_song_aux_track = EOF_TRACK_KEYS;

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
		assert(arrayptr != NULL);	//Unneeded check to resolve a false positive with Coverity
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
						if(!ts_warned)
						{	//If the user hasn't been warned about this yet
							allegro_message("Warning:  Mid beat time signature detected.  Skipping");
							ts_warned = 1;
						}
						eof_log("\t\tWarning:  Mid beat time signature detected.  Skipping", 1);
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

//Check whether the chart has both normal star power AND star power battle sections
#ifdef GH_IMPORT_DEBUG
		eof_log("\tGH:  *Counting star power phrases.", 1);
#endif
	for(ctr = 0; ctr < EOF_NUM_GH_SP_SECTIONS_QB; ctr++)
	{	//For each of the known SP and SP battle section names
		unsigned long retval;

		sp_count = sp_battle_count = 0;	//Reset these
		fb->index = 0;	//Rewind to beginning of file buffer so the search checks the entire buffered file
		retval = eof_gh_read_sp_section_qb(fb, sp, songname, &eof_gh_sp_sections_qb[ctr], qbindex, 1);	//Count star power phrases
		if(retval != ULONG_MAX)
		{	//If the count was successful
#ifdef GH_IMPORT_DEBUG
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  \t%s%s:  %lu Star power phrases", songname, eof_gh_sp_sections_qb[ctr].name, retval);
			eof_log(eof_log_string, 1);
#endif
			sp_count += retval;	//Add to the star power phrase counter
		}

		fb->index = 0;	//Rewind to beginning of file buffer so the search checks the entire buffered file
		retval = eof_gh_read_sp_section_qb(fb, sp, songname, &eof_gh_sp_battle_sections_qb[ctr], qbindex, 1);	//Count star power battle phrases
		if(retval != ULONG_MAX)
		{	//If the count was successful
#ifdef GH_IMPORT_DEBUG
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  \t%s%s:  %lu Star power battle phrases", songname, eof_gh_sp_sections_qb[ctr].name, retval);
			eof_log(eof_log_string, 1);
#endif
			sp_battle_count += retval;	//Add to the star power battle phrase counter
		}
		if(sp_count && sp_battle_count)
		{	//If this track has both types of SP phrases
			sp_conflicts[ctr] = 1;	//Track this
			sp_has_conflict = 1;
		}
		else
		{	//This track had only one SP phrase type or none at all
			sp_conflicts[ctr] = 0;
		}
	}
	if(sp_has_conflict)
	{
		int ret = alert3("At least one track has both normal AND battle SP phrases.", "Import which type when both are present?", NULL, "Normal", "Battle", "Both", 0, 0, 0);
		if(ret == 1)
		{
			import_battle_sp = 0;	//Don't import battle SP phrases when both SP types are present
		}
		else if(ret == 2)
		{
			import_sp = 0;	//Don't import normal SP phrases when both SP types are present
		}
	}

//Read star power sections
#ifdef GH_IMPORT_DEBUG
		eof_log("\tGH:  *Importing star power phrases.", 1);
#endif
	for(ctr = 0; ctr < EOF_NUM_GH_SP_SECTIONS_QB; ctr++)
	{	//For each known guitar hero star power section
		if(sp_conflicts[ctr] && !import_sp)
		{	//If this track has SP AND battle SP phrases, and the user opted to import only battle phrases in this scenario
#ifdef GH_IMPORT_DEBUG
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  User opted to skip %s%s phrase", songname, eof_gh_sp_sections_qb[ctr].name);
			eof_log(eof_log_string, 1);
#endif
			continue;	//Skip this section
		}
		fb->index = 0;	//Rewind to beginning of file buffer so the search checks the entire buffered file
		(void) eof_gh_read_sp_section_qb(fb, sp, songname, &eof_gh_sp_sections_qb[ctr], qbindex, 0);	//Import star power section
	}

//Read star power battle sections
	for(ctr = 0; ctr < EOF_NUM_GH_SP_SECTIONS_QB; ctr++)
	{	//For each known guitar hero star power battle section
		if(sp_conflicts[ctr] && !import_battle_sp)
		{	//If this track has SP AND battle SP phrases, and the user opted to import only normal SP phrases in this scenario
#ifdef GH_IMPORT_DEBUG
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGH:  User opted to skip %s%s phrase", songname, eof_gh_sp_sections_qb[ctr].name);
			eof_log(eof_log_string, 1);
#endif
			continue;	//Skip this section
		}
		fb->index = 0;	//Rewind to beginning of file buffer so the search checks the entire buffered file
		(void) eof_gh_read_sp_section_qb(fb, sp, songname, &eof_gh_sp_battle_sections_qb[ctr], qbindex, 0);	//Import star power battle section
	}

//Read tap (slider) sections
	for(ctr = 0; ctr < EOF_NUM_GH_TAP_SECTIONS_QB; ctr++)
	{	//For each known guitar hero tap section
		fb->index = 0;	//Rewind to beginning of file buffer
		(void) eof_gh_read_tap_section_qb(fb, sp, songname, &eof_gh_tap_sections_qb[ctr], qbindex);	//Import tap section
	}

//Read sections
	(void) eof_gh_read_sections_qb(fb, sp, 0);

//Read vocal track
	(void) eof_gh_read_vocals_qb(fb, sp, songname, qbindex);
	eof_filebuffer_close(fb);	//Close the file buffer

	return sp;
}

struct QBlyric *eof_gh_read_section_names(filebuffer *fb)
{
	unsigned long checksum = 0, index2, nameindex = 0, ctr;
	unsigned char sectionid_ASCII[] = {0x22, 0x0D, 0x0A};		//This hex sequence is between each section name entry for ASCII text encoded GH files
	unsigned char sectionid_UNI[] = {0x00, 0x22, 0x00, 0x0A};	//This hex sequence is between each section name entry for ASCII text encoded GH files
	unsigned char sectionid_GH3[] = {0x00, 0x20, 0x03, 0x00};	//This hex sequence precedes each section name entry in GH3 format chart files
	unsigned char *sectionid, char_size = 1;
	char addsection;
	size_t section_id_size;
	unsigned char quote_rewind = 0;	//The number of bytes before a section name's opening quote mark its checknum exists
	char *buffer = NULL, checksumbuff[9] = {0}, checksumbuffuni[18] = {0}, *name;
	struct QBlyric *head = NULL, *tail = NULL, *linkptr = NULL, *temp;	//Used to maintain the linked list matching section names with checksums
	char abnormal_markers = 0;	//Normally, section names don't begin with "\L", but some files have them with this prefix
	unsigned long lastseekstartpos;
		//Some GH files put all languages together instead of in separate parts, this is used to store the position before a seek so if a duplicate checksum (same
		//section as a previous one in another language) is found, the seek position is restored and function returns, so next call can parse the next language
	unsigned long lastfoundpos;	//Stores the last found instance of the section ID, in case it turns out not to be a section and should be skipped
	unsigned long gh3_count = 0, ascii_count = 0, uni_count = 0;

	eof_log("eof_gh_read_section_names() entered", 1);

	if(!fb)
		return NULL;

	gh3_count = eof_filebuffer_count_instances(fb, sectionid_GH3, 4);		//Count how many times the GH3 section associated byte sequence is found
	ascii_count = eof_filebuffer_count_instances(fb, sectionid_ASCII, 3);	//Count how many times the ASCII section associated byte sequence is found
	uni_count = eof_filebuffer_count_instances(fb, sectionid_UNI, 4);		//Count how many times the Unicode section associated byte sequence is found

	if((gh3_count > ascii_count) && (gh3_count > uni_count))
	{	//If the hex string associated with GH3 charts is most prevalent
		section_id_size = sizeof(sectionid_GH3);
		sectionid = sectionid_GH3;
		eof_gh_import_gh3_style_sections = 1;		//Different logic will be used to parse the section names
		eof_log("\tDetected GH3 format sections", 1);
	}
	else if(eof_gh_unicode_encoding_detected)
	{	//If Unicode encoding was found when importing lyrics, the section names will also be in Unicode
		section_id_size = sizeof(sectionid_UNI);
		sectionid = sectionid_UNI;
		quote_rewind = 18;
		char_size = 2;	//A Unicode character is two bytes
		eof_log("\tDetected UTF-16 sections", 1);
	}
	else
	{	//ASCII encoding is assumed
		section_id_size = sizeof(sectionid_ASCII);
		sectionid = sectionid_ASCII;
		quote_rewind = 9;
		char_size = 1;
		eof_log("\tDetected ASCII sections", 1);
	}

//Find the section names and checksums
	lastseekstartpos = fb->index;	//Back up the buffer position before each seek
	while(eof_filebuffer_find_bytes(fb, sectionid, section_id_size, 1) > 0)
	{	//While there are section name entries
		lastfoundpos = fb->index;
		addsection = 0;	//Reset this status
		if(eof_gh_import_gh3_style_sections)
		{	//GH3 section parsing logic
			nameindex = 0;	//This is the index into the buffer that will point ahead of any leading gibberish the section name string may contain
			fb->index += 4;	//Seek past the section header
			if(eof_filebuffer_get_dword(fb, &checksum) == EOF)	//Read the checksum into a buffer
			{
				eof_log("\t\tError:  Could not read section name checksum", 1);
				eof_destroy_qblyric_list(head);
				return NULL;
			}

			//Check if this checksum was already read (one language of sections has been parsed), and if so, revert the buffer position to before the last seek operation, return section list
			for(temp = head; temp != NULL; temp = temp->next)
			{	//For each section name parsed in this function call
				if(temp->checksum == checksum)
				{	//If the checksum just read already exists in the linked list
					fb->index = lastseekstartpos;	//Revert to position before the most recent seek
					return head;	//Stop reading sections
				}
			}

			fb->index += 4;	//Seek past 4 bytes of unknown data

			//Parse the section name string
			fb->index += 8;	//Seek past 8 bytes of unknown data
			for(index2 = 0; fb->buffer[fb->index + index2] != '\0'; index2++);	//Count the number of characters in this string
			if(index2)
			{	//If there is at least one character (if there aren't any, it probably isn't a practice section)
				buffer = malloc((size_t)index2 + 1);		//This buffer will be large enough to contain the string and a NULL terminator
				if(buffer == NULL)
				{	//If the memory couldn't be allocated
					eof_log("\t\tError:  Cannot allocate memory", 1);
					eof_destroy_qblyric_list(head);
					return NULL;
				}
				memset(buffer, 0, (size_t)index2);		//Fill with 0s to satisfy Splint
				if(eof_filebuffer_memcpy(fb, buffer, (size_t)index2) == EOF)	//Read the section name string into a buffer
				{
					eof_log("\t\tError:  Could not read section name text", 1);
					free(buffer);
					eof_destroy_qblyric_list(head);
					return NULL;
				}
				buffer[index2] = '\0';	//Terminate the string
				addsection = 1;	//Criteria have been met to add this section
			}
		}
		else
		{	//Non GH3 section parsing logic
			for(index2 = char_size; (index2 <= fb->index) && (fb->buffer[fb->index - index2 + char_size - 1] != '\"'); index2 += char_size);	//Find the opening quotation mark for this string
			if((index2 > fb->index) || (index2 < 1))
			{	//If the opening quotation mark wasn't found or if there was some kind of logic error (ie. overflow)
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
						checksumbuff[ctr] = checksumbuffuni[(2 * ctr) + 1];	//Keep the second byte (non-zero byte) of each UTF-16 Unicode character, converting to ASCII format
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
				memset(buffer, 0, (size_t)index2);		//Fill with 0s to satisfy Splint
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
					addsection = 1;	//Criteria have been met to add this section
				}//If this string is at least two characters long
			}//If there is enough buffered data before this position to allow for an 8 character checksum and a space character
		}//Non GH3 section parsing logic
		lastseekstartpos = fb->index;	//Back up the buffer position before each seek

		if(addsection)
		{	//If the information for a section was parsed
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

			free(buffer);	//Free the temporary buffer that stored the raw section name string
			buffer = NULL;
		}//If this string is at least two characters long

		if(!eof_gh_import_gh3_style_sections)
		{	//Non GH3 section parsing logic
			fb->index++;	//Seek past the closing quotation mark in this section name to allow the next loop iteration to look for the next section name
			if(!abnormal_markers && ((size_t)fb->index + 4 < fb->size))
			{	//If there are at least four more bytes in the buffer (and no strings beginning with "\L" were parsed)
				if((fb->buffer[fb->index] == 0x0D) && (fb->buffer[fb->index + 1] == 0x0A) && (fb->buffer[fb->index + 2] == 0x0D) && (fb->buffer[fb->index + 3] == 0x0A))
				{	//If those bytes are 0x0D, 0x0A, 0x0D, 0x0A
					break;	//This marks the formal end of the section names
				}
			}
			}
	}//While there are section name entries

	return head;
}

EOF_SONG *eof_sections_list_all_ptr;
char *eof_sections_list_all(int index, int * size)
{
	if(index < 0)
	{	//Signal to return the list count
		if(!size)
			return NULL;

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
	unsigned long numsections = 0, checksum = 0, dword = 0, ctr, lastsectionpos = 0;
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
					unsigned long beatnum;

					if(linkptr->checksum != checksum)
						continue;	//If this checksum does not match the one in the list, skip it

					eof_chart_length = dword;	//Satisfy eof_get_beat() by ensuring this variable isn't smaller than the looked up timestamp
					beatnum = eof_get_beat(sp, dword);	//Get the beat immediately at or before this section
					if(eof_beat_num_valid(sp, beatnum))
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

int eof_gh_read_sections_qb(filebuffer *fb, EOF_SONG *sp, char undo)
{
	unsigned long checksum = 0, dword = 0, ctr, findpos, findpos2, lastsectionpos = 0;
	char sectionsfound = 0, validated, found;
	int prompt;
	struct QBlyric *head = NULL, *linkptr = NULL;	//Used to maintain the linked list matching section names with checksums
	filebuffer *sections_file;
	int done = 0, retval = 0;
	char undo_made = 0;

	if(!fb || !sp)
		return -1;

	eof_log("eof_gh_read_sections_qb() entered", 1);
	sections_file = fb;	//By default, check for section names in the same file as the section timestamps
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
				fb->index = 0;	//Rewind to beginning of chart file buffer
				found = 0;	//Reset this boolean condition
				while(1)
				{	//Search for each instance of the section string checksum
					if(eof_filebuffer_find_checksum(fb, linkptr->checksum))	//Find the section string checksum in the buffer
					{	//If the checksum wasn't found
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tCouldn't find position data for section \"%s\", it is probably a lyric", linkptr->text);
						eof_log(eof_log_string, 1);
						break;	//Skip looking for this section's timestamp
					}
					findpos = fb->index;	//Store the section string checksum match position
					validated = 0;		//Reset this boolean condition
					if(eof_gh_import_gh3_style_sections)
					{	//GH3 format section names
						if(!eof_filebuffer_get_dword(fb, &dword) && (dword == 0) && (fb->index >= 24))
						{	//If the dword following the string checksum was successfully read, the value was 0, and the buffer can be rewound at least 24 bytes
							fb->index -= 24;	//Rewind 6 dwords, where the timestamp is expected to be
							if(!eof_filebuffer_get_dword(fb, &dword))	//Read the timestamp
							{	//If the timestamp was successfully read
								unsigned long beatnum;

								if(eof_chart_length < dword)
								{	//Satisfy eof_get_beat() by ensuring this variable isn't smaller than the looked up timestamp
									eof_chart_length = dword;
								}
								beatnum = eof_get_beat(sp, dword);	//Get the beat immediately at or before this section
								if(eof_beat_num_valid(sp, beatnum))
								{	//If there is such a beat
									char buffer2[256] = {0};

#ifdef GH_IMPORT_DEBUG
									(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tSection:  Position = %lums, checksum = 0x%08lX: %s", dword, checksum, linkptr->text);
									eof_log(eof_log_string, 1);
#endif
									(void) snprintf(buffer2, sizeof(buffer2) - 1, "[section %s]", linkptr->text);	//Alter the section name formatting
									if(undo && !undo_made)
									{	//Make a back up before adding the first section (but only if the calling function specified to create an undo state)
										eof_prepare_undo(EOF_UNDO_TYPE_NONE);
										undo_made = 1;
									}
									(void) eof_song_add_text_event(sp, beatnum, buffer2, 0, 0, 0);	//Add the text event
								}
								found = 1;
								break;	//Break from practice section search loop
							}
						}
					}
					else
					{	//Other GH games' section name format
						if(!eof_filebuffer_get_dword(fb, &dword) && (dword == 0) && (fb->index >= 20))
						{	//If the dword following the string checksum was successfully read, the value was 0, and the buffer can be rewound at least 20 bytes
							fb->index -= 20;	//Rewind 5 dwords
							if(!eof_filebuffer_get_dword(fb, &dword) && (dword == 0x00201C00))
							{	//If the 3rd dword before the string checksum was succesfully read and the value was 0x00201C00 (new section header)
								validated = 1;	//Consider this to be the appropriate entry matching the section string checksum with its section checksum
							}
						}

						if(validated)
						{
							if(!eof_filebuffer_get_dword(fb, &checksum))
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
											unsigned long beatnum;

											eof_chart_length = dword;	//Satisfy eof_get_beat() by ensuring this variable isn't smaller than the looked up timestamp
											beatnum = eof_get_beat(sp, dword);	//Get the beat immediately at or before this section
											if(eof_beat_num_valid(sp, beatnum))
											{	//If there is such a beat
												char buffer2[256] = {0};

#ifdef GH_IMPORT_DEBUG
												(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tSection:  Position = %lums, checksum = 0x%08lX: %s", dword, checksum, linkptr->text);
												eof_log(eof_log_string, 1);
#endif
												(void) snprintf(buffer2, sizeof(buffer2) - 1, "[section %s]", linkptr->text);	//Alter the section name formatting
												if(undo && !undo_made)
												{	//Make a back up before adding the first section (but only if the calling function specified to create an undo state)
													eof_prepare_undo(EOF_UNDO_TYPE_NONE);
													undo_made = 1;
												}
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
					}//Other GH games' section name format
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
			else
			{
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
					if(!sectionfn)
						continue;	//If the user didn't select a file, try again

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
				}//Prompt user about browsing for an external file with section names until explicitly declined
			}//There are no sections found in this file
		}//There were no sections loaded during this loop iteration
	}//Until the user accepts a language of section names

	return retval;
}

int eof_import_array_txt(const char *filename)
{
	char *buffer, *buffer2, *line;
	int failed = 0, format = 0, gh3_format;
	unsigned long ctr = 0, ctr2, linesread = 0, tracknum;
	long position, lastposition = 0, note, fixednote, data, length, accent;
	EOF_NOTE *newnote, *lastnote = NULL;
	double threshold = 66.0 / 192.0;
	unsigned long toggle_hopo_count = 0;

	if(!filename || !eof_song)
		return 1;	//No project or invalid parameter

	eof_gh_accent_prompt = 0;	//Reset this condition

	///Load file into memory buffer
	buffer = (char *)eof_buffer_file(filename, 1);	//Buffer the file into memory, adding a NULL terminator at the end of the buffer
	if(!buffer)
		return 2;	//Failed to buffer file into memory

	///Create an extra copy of the buffer, so the input file can be parsed twice
	buffer2 = DuplicateString(buffer);
	if(!buffer2)
	{
		free(buffer);
		return 3;	//Failed to allocate memory
	}

	///Pre-parse file contents to determine whether all lines have a valid number and what data is being represented
	for(ctr = 0; !failed; ctr++)
	{	//Until an error has occurred
		if(!ctr)
		{	//If this is the first line being parsed
			line = ustrtok(buffer2, "\r\n");	//Initialize the tokenization and get first tokenized line
		}
		else
		{
			line = ustrtok(NULL, "\r\n");	//Return the next tokenized line
		}

		if(!line)	//If a tokenized line of the file was not obtained
			break;
		if(line[0] == '\0')	//If this line is empty
			continue;	//Skip it

		if(line[0] == '0')
		{	//If this number is a zero
			position = 0;
		}
		else
		{
			position = atol(line);	//Convert the string into a number
			if(position < 0)
			{	//If the number is an invalid position
				failed = 4;	//Invalid data
				break;
			}
			if(position == 0)
			{	//If atol() couldn't convert the number
				failed = 5;	//Invalid data
				break;
			}
		}

		if((ctr > 0) && (position <= lastposition))
		{	//If a number is not larger than the previous line's number
			format = 1;	//This is note data and not beat position data
		}
		linesread++;	//Count how many numbers were read
		lastposition = position;
	}//Until an error has occurred
	if(format && ((linesread % 2) && (linesread % 3)))
	{	//If this was determined to be note data, but the number of lines isn't divisible by two (post GH3/GHA format) or three (GH3/GHA format)
		failed = 6;	//Invalid data, each note is defined in two lines (one with a position, the other with its composition)
	}

	///If it is determined to be note data, pre-parse to determine whether it's GH3/GHA format (3 numbers define each note) or newer format (2 numbers define each note)
	strcpy(buffer2, buffer);	//Replace buffer2 with a clean copy of buffer to re-tokenize it
	gh3_format = 1;	//Unless the notes don't follow the expected scheme, assume GH3/GHA format
	for(ctr = 0; !failed; ctr++)
	{	//Until an error has occurred
		if(!ctr)
		{	//If this is the first line being parsed
			line = ustrtok(buffer2, "\r\n");	//Initialize the tokenization and get first tokenized line
		}
		else
		{
			line = ustrtok(NULL, "\r\n");	//Return the next tokenized line
		}

		if(!line)	//If a tokenized line of the file was not obtained
			break;
		if(line[0] == '\0')	//If this line is empty
			continue;	//Skip it

		if(line[0] == '0')
		{	//If this number is a zero
			position = 0;
		}
		else
		{
			position = atol(line);	//Convert the string into a number
		}
		if(ctr && (position < lastposition))
		{	//If this isn't the first note parsed, and the timestamp is smaller then the previous timestamp
			gh3_format = 0;
			break;
		}
		lastposition = position;	//Remember this timestamp for comparison with the next timestamp
		line = ustrtok(NULL, "\r\n");	//Read the note duration
		if(!line || (line[0] == '\0'))
		{	//If the line couldn't be read of the line is empty
			gh3_format = 0;
			break;
		}
		line = ustrtok(NULL, "\r\n");	//Read the note bitmask
		if(!line || (line[0] == '\0'))
		{	//If the line couldn't be read of the line is empty
			gh3_format = 0;
			break;
		}
		note = atol(line);	//Convert the bitmask to integer format
		if(note > 63)
		{	//If the note bitmask is larger than expected (higher than 6 set bits)
			gh3_format = 0;
			break;
		}
	}

	if(failed)
	{	//If any numbers failed to be parsed or the data is invalid
		free(buffer);
		free(buffer2);

		return failed;
	}
	eof_prepare_undo(EOF_UNDO_TYPE_NONE);

	if(!format)
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tImporting %lu beat times.", linesread);
		eof_log(eof_log_string, 1);
	}
	else
	{
		if(gh3_format)
		{
			int selection;

			selection = alert("GH3/GHA charts can have one of two HOPO thresholds.", "Which should EOF use?", NULL, "66/192 qnote", "100/192 qnote", 0, 0);
			if(selection == 2)
			{	//If the user selected the 100/192 threshold
				threshold = 100.0 / 192.0;
			}
#ifdef GH_IMPORT_DEBUG
			eof_log("\tGH3/GHA note format detected.", 1);
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tUser selected %s HOPO threshold.", ((selection == 1) ? "66/192" : "100/192"));
			eof_log(eof_log_string, 1);
#endif
		}
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tImporting %lu note definitions.", linesread / 2);
		eof_log(eof_log_string, 1);
	}

	///Parse file contents as either beat positions or note data
	for(ctr = 0; !failed; ctr++)
	{	//Until an error has occurred
		if(!ctr)
		{	//If this is the first line being parsed
			line = ustrtok(buffer, "\r\n");	//Initialize the tokenization and get first tokenized line
		}
		else
		{
			line = ustrtok(NULL, "\r\n");	//Return the next tokenized line
		}

		if(!line)	//If a tokenized line of the file was not obtained
			break;
		if(line[0] == '\0')	//If this line is empty
			continue;	//Skip it

		//Read the position
		if(line[0] == '0')
		{	//If this number is a zero
			position = 0;
		}
		else
		{
			position = atol(line);	//Convert the string into a number
		}

		if(!format)
		{	//If this is a beat position, update the project's beat map
			if(ctr >= eof_song->beats)
			{	//If there aren't enough beats in the project to store this beat position
				EOF_BEAT_MARKER *new_beat = eof_song_add_beat(eof_song);	//Add a beat

				if(!new_beat)
				{	//If a new beat couldn't be added
					failed = 7;
					break;
				}
			}
			eof_song->beat[ctr]->pos = eof_song->beat[ctr]->fpos = position;
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tBeat %lu:  %ldms.", ctr, position);
			eof_log(eof_log_string, 1);
		}
		else
		{	//This is note data, read the next line to get the note's composition
			line = ustrtok(NULL, "\r\n");	//Return the next tokenized line
			if(!line || (line[0] == '\0') || (line[0] == '0'))
			{	//If the line couldn't be read, is empty or is a zero
				failed = 8;	//Invalid data
				break;
			}
			data = atol(line);
			if(data == 0)
			{	//If atol() couldn't convert the number
				failed = 9;	//Invalid data
				break;
			}
			if(gh3_format)
			{	//The line that was just read and one more are expected to define the note
				length = data;	//That line was the duration

				line = ustrtok(NULL, "\r\n");	//Return the next tokenized line
				if(!line || (line[0] == '\0') || (line[0] == '0'))
				{	//If the line couldn't be read, is empty or is a zero
					failed = 8;	//Invalid data
					break;
				}
				data = atol(line);
				if(data == 0)
				{	//If atol() couldn't convert the number
					failed = 9;	//Invalid data
					break;
				}

				note = data & 63;	//The third line is the note bitmask
				fixednote = note & 31;	//Only the lower 5 bits define gems, bit 6 defines the toggle HOPO marker
				accent = 0;			//Unused
			}
			else
			{	//The line that was just read completes the note's definition
				length = data & 0xFFFF;	//The two least significant bytes are the length of the note
				data >>= 16;		//Shift the data right two bytes, discarding the length
				note = data & 63;	//The next byte defines the gems used, although only the first 6 bits pertain to gems
				fixednote = note;
				accent = data >> 8;	//The most significant byte defines the accent bitmask
			}

			newnote = eof_track_add_create_note(eof_song, eof_selected_track, 0, position, length, eof_note_type, NULL);	//Add the note to the active track difficulty
			if(!newnote)
			{	//If the note couldn't be added
				failed = 10;
				break;
			}
			if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
			{	//If a drum track is active, interpret as drum note data
				fixednote &= ~1;	//Clear lane 1 gem
				fixednote &= ~32;	//Clear lane 6 gem
				if(data & 32)
				{	//If lane 6 is populated, convert it to RB's bass drum gem
					fixednote |= 1;		//Set the lane 1 (bass drum) gem
				}
				if(data & 1)
				{	//If lane 1 is populated, convert it to lane 6
					fixednote |= 32;
					eof_song->track[eof_selected_track]->flags |= EOF_TRACK_FLAG_SIX_LANES;	//Ensure "five lane" drums is enabled for the track
					tracknum = eof_song->track[eof_selected_track]->tracknum;	//Simplify
					eof_song->legacy_track[tracknum]->numlanes = 6;
				}
				if((data & 64) && !(data & 32))
				{	//If bit 6 is set, but bit 5 is not
					fixednote |= 1;	//Consider it a double bass drum note
					newnote->flags |= EOF_DRUM_NOTE_FLAG_DBASS;	//Set the double bass flag bit
				}
				else if((accent & 32) && !(note & 32))
				{	//If bit 5 of the accent mask is set, but bit 5 of the note mask is not
					fixednote |= 1;	//Consider it a double bass drum note
					newnote->flags |= EOF_DRUM_NOTE_FLAG_DBASS;	//Set the double bass flag bit
				}

				if((newnote->type == EOF_NOTE_AMAZING) && (length >= 140))
				{	//If this is an expert difficulty note that is at least 140ms long, it should be treated as a drum roll
					int phrasetype = EOF_TREMOLO_SECTION;	//Assume a normal drum roll (one lane)
					unsigned long lastnoteindex = eof_get_track_size(eof_song, EOF_TRACK_DRUM) - 1;

					if(eof_note_count_colors(eof_song, EOF_TRACK_DRUM, lastnoteindex) > 1)
					{	//If the new drum note contains gems on more than one lane
						phrasetype = EOF_TRILL_SECTION;		//Consider it a special drum roll (multiple lanes)
					}
					(void) eof_track_add_section(eof_song, EOF_TRACK_DRUM, phrasetype, 0xFF, position, position + length, 0, NULL);
					newnote->flags |= EOF_NOTE_FLAG_CRAZY;	//Mark it as crazy, both so it can overlap other notes and so that it will export with sustain
				}

				//If there is accent data, prompt user about which game it's from to account for different accent definition behavior as with GH import
				if(accent)
				{
					if(!eof_gh_accent_prompt)
					{	//If the user wasn't prompted about which game this chart is from
						if(alert("Is this chart from any of the following GH games?", "\"Warriors of Rock\", \"Band Hero\", \"Guitar Hero 5\"", "(These use a different accent notation than Smash Hits)", "&Yes", "&No", 'y', 'n') == 1)
						{	//If user indicates the chart is from Warriors of Rock
							eof_gh_accent_prompt = 1;
						}
						else
						{
							eof_gh_accent_prompt = 2;
						}
					}
					if(eof_gh_accent_prompt == 1)
					{	//Warriors of Rock rules for interpreting the accent bitmask
						if(accent & 31)
						{	//Bits 0 through 4 of the accent mask are used to indicate accent status for drum gems
							if(accent & 1)
							{	//The least significant bit is assumed to be accent status for lane 6
								accent &= ~1;	//Clear the bit for lane 1
								accent |= 32;	//Set the bit for lane 6
							}
							accent &= fixednote;			//Filter out any bits from the accent mask that don't have gems
							newnote->accent = accent;
						}
					}
					else
					{	//Smash Hits rules for interpreting the accent bitmask
						if(accent & 15)
						{	//Bits 0 through 3 of the accent mask are used to indicate accent status for drum gems (lanes other than bass drum) in Guitar Hero Smash Hits
							accent &= 15;						//Store only the applicable bits (lane 2 through 6) into the note's accent mask
							accent <<= 1;						//Shift left one bit to reflect that the first accent bit defines accent for snare, ie. lane 2 instead of bass, ie. lane 1
							if(fixednote & 32)
							{	//If the accented note has any gems on lane 6, it's assumed that it inherits the accented status automatically since the game doesn't seem
								// to use a bit to define accent status individually for this lane
								accent |= 32;
							}

							accent &= fixednote;			//Filter out any bits from the accent mask that don't have gems
							newnote->accent = accent;
						}
					}
				}
			}//If a drum track is active, interpret as drum note data
			else
			{	//Interpret as guitar/bass note data
				if(gh3_format)
				{	//GH3/GHA charts used note proximity and a toggle HOPO marker
					if(note & 32)
					{	//Bit 5 of the note bitmask is a toggle HOPO status
						newnote->flags ^= EOF_NOTE_FLAG_F_HOPO;	//Toggle the forced HOPO flag
						newnote->flags ^= EOF_NOTE_FLAG_HOPO;
						toggle_hopo_count++;	//Keep track of how many of these there are
					}
				}
				else
				{	//Other charts explicitly define HOPO on/off
					if(data & 64)
					{	//Bit 6 is the HOPO bit
						newnote->flags |= (EOF_NOTE_FLAG_F_HOPO | EOF_NOTE_FLAG_HOPO);
					}
					else
					{
						newnote->flags |= EOF_NOTE_FLAG_NO_HOPO;
					}
				}
			}
			newnote->note = fixednote;	//Update the new note's bitmask

			//Apply disjointed status if appropriate
			if(lastnote && (lastnote->pos == newnote->pos) && (lastnote->length != newnote->length))
			{	//If there was a previous note, it started at the same time as this note and has a different length
				lastnote->eflags |= EOF_NOTE_EFLAG_DISJOINTED;	//Apply disjointed status to both notes
				newnote->eflags |= EOF_NOTE_EFLAG_DISJOINTED;
			}

			lastnote = newnote;
		}//This is note data, read the next line to get the note's composition
	}//Until an error has occurred

	if(!failed)
	{	//If the import succeeded
		if(!format)
		{	//If beat timings were imported
			eof_calculate_tempo_map(eof_song);	//Update the tempos of the project based on the imported timestamps
			eof_chart_length = eof_song->beat[eof_song->beats - 1]->pos;	//The position of the last beat is the new chart length
		}
		else
		{	//If notes were imported
			unsigned long lastpos;

			eof_track_sort_notes(eof_song, eof_selected_track);

			//Apply auto HOPO statuses if necessary
			if(gh3_format)
			{	//If the notes were in GH3/GHA format, HOPO status is based on proximity to the previous note
				unsigned long tracksize = eof_get_track_size(eof_song, eof_selected_track);

				eof_track_sort_notes(eof_song, eof_selected_track);
				for(ctr2 = 0; ctr2 < tracksize; ctr2++)
				{	//For each of the imported notes
					unsigned long flags = eof_get_note_flags(eof_song, eof_selected_track, ctr2), flags2;

					if(eof_get_note_type(eof_song, eof_selected_track, ctr2) != eof_note_type)	//If this isn't one of the notes that was imported into the active difficulty
						continue;	//Skip it

					flags2 = flags & ~(EOF_NOTE_FLAG_F_HOPO | EOF_NOTE_FLAG_NO_HOPO);	//Clear these flags so eof_note_is_gh3_hopo() won't use them as the deciding factor
					eof_set_note_flags(eof_song, eof_selected_track, ctr2, flags2);
					if(eof_note_is_gh3_hopo(eof_song, eof_selected_track, ctr2, threshold))
					{	//If this note meets the criteria for an auto HOPO (is a single gem not matching the previous note and is within the user specified threshold distance)
						flags ^= EOF_NOTE_FLAG_F_HOPO;	//Toggle the forced HOPO flag
						flags ^= EOF_NOTE_FLAG_HOPO;
					}
					else
					{
						flags &= ~EOF_NOTE_FLAG_HOPO;	//Clear HOPO flag
					}
					if(!(flags & EOF_NOTE_FLAG_F_HOPO))
					{	//If the note was not ultimately a HOPO, and the user opted to import forced strum status
						flags |= EOF_NOTE_FLAG_NO_HOPO;	//Set forced non HOPO
					}
					eof_set_note_flags(eof_song, eof_selected_track, ctr2, flags);	//Update the note's flags
				}

#ifdef GH_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t%lu toggle HOPO statuses encountered.", toggle_hopo_count);
				eof_log(eof_log_string, 1);
#endif
			}

			lastpos = eof_get_note_pos(eof_song, eof_selected_track, eof_get_track_size(eof_song, eof_selected_track) - 1);
			if(eof_chart_length < lastpos)
			{	//If the length of the project is to be extended due to new notes
				eof_chart_length = lastpos;
				eof_truncate_chart(eof_song);	//Add enough beats to encompass the notes
			}
			eof_track_find_crazy_notes(eof_song, eof_selected_track, 1);	//Mark overlapping notes with crazy status, but not notes that start at the exact same timestamp (will be given disjointed status or merge into chords as appropriate)
			(void) eof_detect_difficulties(eof_song, eof_selected_track);	//Update arrays for note set population and highlighting

			//Update piano roll and 3D preview lane coordinates in case the lane count changed
			eof_set_3D_lane_positions(0);	//Update xchart[] by force
			eof_scale_fretboard(0);			//Recalculate the 2D screen positioning based on the current track

			eof_track_fixup_notes(eof_song, eof_selected_track, 1);
		}//If notes were imported
	}//If the import succeeded

	///Free memory
	free(buffer);
	free(buffer2);

	return failed;	//Return whatever success/failure status has accumulated
}

void eof_gh_import_sp_cleanup(EOF_SONG *sp)
{
	unsigned long track, path, numpaths, note, numnotes;
	EOF_PHRASE_SECTION *ptr;

	if(!sp)
		return;	//Invalid parameter

	for(track = 1; track < sp->tracks; track++)
	{	//For each track in the project
		numpaths = eof_get_num_star_power_paths(sp, track);

		for(path = 0; path < numpaths; path++)
		{	//For each star power path in the track
			ptr = eof_get_star_power_path(sp, track, path);
			if(ptr && (ptr->end_pos - ptr->start_pos > 1))
			{	//If the phrase was found, and its length is at least 2ms
				numnotes = eof_get_track_size(sp, track);
				for(note = 0; note < numnotes; note++)
				{	//For each note in the track
					if(eof_get_note_pos(sp, track, note) == ptr->end_pos)
					{	//If the note begins at the end position of the star power path
						ptr->end_pos--;	//Truncate the path by 1ms
						break;	//Stop checking notes for this star power path
					}
				}
			}
		}
	}
}

void eof_gh_import_slider_cleanup(EOF_SONG *sp)
{
	unsigned long track, path, numpaths, note, numnotes;
	EOF_PHRASE_SECTION *ptr;

	if(!sp)
		return;	//Invalid parameter

	for(track = 1; track < sp->tracks; track++)
	{	//For each track in the project
		numpaths = eof_get_num_sliders(sp, track);

		for(path = 0; path < numpaths; path++)
		{	//For each slider path in the track
			ptr = eof_get_slider(sp, track, path);
			if(ptr && (ptr->end_pos - ptr->start_pos > 1))
			{	//If the phrase was found, and its length is at least 2ms
				numnotes = eof_get_track_size(sp, track);
				for(note = 0; note < numnotes; note++)
				{	//For each note in the track
					if(eof_get_note_pos(sp, track, note) == ptr->end_pos)
					{	//If the note begins at the end position of the slider
						ptr->end_pos--;	//Truncate the path by 1ms
						break;	//Stop checking notes for this slider
					}
				}
			}
		}
	}
}
