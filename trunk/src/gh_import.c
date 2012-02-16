#include "beat.h"
#include "gh_import.h"
#include "main.h"
#include "midi.h"	//For eof_apply_ts()
#include "utility.h"

unsigned long crc32_lookup[256] = {0};	//A lookup table to improve checksum calculation performance
char crc32_lookup_initialized = 0;	//Is set to nonzero when the lookup table is created

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
	unsigned char notemask, accentmask;
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
		}
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

#define GH_IMPORT_DEBUG
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
	if(dword != 4)
	{	//Each fretbar is expected to be 4 bytes long
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
		if(ctr && (dword <= lastfretbar))
		{	//If this beat doesn't come after the previous beat
			eof_log("Error:  Corrupted fretbar", 1);
			eof_destroy_song(sp);
			eof_filebuffer_close(fb);
			return NULL;
		}
#ifdef GH_IMPORT_DEBUG
		snprintf(eof_log_string, sizeof(eof_log_string), "\tGH:  Fretbar at %lums", dword);
		eof_log(eof_log_string, 1);
#endif
		sp->beat[ctr]->pos = sp->beat[ctr]->fpos = dword;	//Set the timestamp position of this beat
		lastfretbar = dword;
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
		if(eof_filebuffer_get_dword(fb, &dword))	//Read the size of the time signature value
		{	//If there was an error reading the next 4 byte value
			eof_log("Error:  Could not read time signature size", 1);
			eof_destroy_song(sp);
			eof_filebuffer_close(fb);
			return NULL;
		}
		if(dword != 6)
		{	//Each time signature is expected to be 6 bytes long
			eof_log("Error:  Time signature size is not 6", 1);
			eof_destroy_song(sp);
			eof_filebuffer_close(fb);
			return NULL;
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

	eof_filebuffer_close(fb);	//Close the file buffer

//Load an audio file
	replace_filename(oggfn, fn, "guitar.ogg", 1024);	//Try to load guitar.ogg in the GH file's folder by default
	if(!eof_load_ogg(oggfn))
	{
		eof_destroy_song(sp);
		return NULL;
	}
	eof_music_length = alogg_get_length_msecs_ogg(eof_music_track);
	eof_music_actual_length = eof_music_length;

	eof_log("\tGH import completed", 1);
	return sp;
}
