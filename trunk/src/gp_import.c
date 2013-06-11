#ifdef S_SPLINT_S
//Ensure Splint checks the code for EOF's code base
#define EOF_BUILD
#endif

#ifdef EOF_BUILD
#include <allegro.h>
#include <assert.h>
#include "beat.h"
#include "chart_import.h"	//For FindLongestLineLength_ALLEGRO()
#include "main.h"
#include "midi.h"
#include "rs.h"
#include "song.h"
#include "tuning.h"
#include "undo.h"
#endif

#include <math.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gp_import.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

#ifndef EOF_BUILD
char *eof_note_names_flat[12];		//The name of musical note 0 (0 semitones above A) to 11 (11 semitones above A)
char **eof_note_names = eof_note_names_flat;
#endif

void pack_ReadWORDLE(PACKFILE *inf,unsigned *data)
{
	unsigned char buffer[2]={0};

	if(inf)
	{
		if((pack_fread(buffer, 2, inf) == 2) && data)
		{	//If the data was successfully read, and data isn't NULL, store the value
			*data = ((unsigned short)buffer[0] | ((unsigned short)buffer[1]<<8));	//Convert to 2 byte integer
		}
	}
}

void pack_ReadDWORDLE(PACKFILE *inf,unsigned long *data)
{
	unsigned char buffer[4]={0};

	if(inf)
	{
		if((pack_fread(buffer, 4, inf) == 4) && data)
		{	//If the data was successfully read, and data isn't NULL, store the value
			*data=((unsigned long)buffer[0]) | ((unsigned long)buffer[1]<<8) | ((unsigned long)buffer[2]<<16) | ((unsigned long)buffer[3]<<24);	//Convert to 4 byte integer
		}
	}
}

int eof_read_gp_string(PACKFILE *inf, unsigned *length, char *buffer, char readfieldlength)
{
	unsigned int len = 0;

	if(!inf || !buffer)
		return 0;	//Return error

	if(readfieldlength)
		pack_ReadDWORDLE(inf, NULL);	//Read the 4 byte field length that prefixes the string length

	len = pack_getc(inf);	//Read the string length
	if(len == EOF)
		return 0;	//Return error
	if(length)
	{	//If the calling function passed a non NULL pointer
		*length = len;	//Return the string length through it
	}
	(void) pack_fread(buffer, (long)len, inf);	//Read the string
	buffer[len] = '\0';	//Terminate the string

	return 1;	//Return success
}

#ifndef EOF_BUILD
	//Standalone parse utility
void eof_gp_debug_log(FILE *inf, char *text)
{
	if(inf && text)
	{	//If these are both not NULL
		printf("0x%lX:\t%s", ftell(inf), text);
	}
}
#else
void eof_gp_debug_log(PACKFILE *inf, char *text)
{
	if(inf)
	{	//Read inf to get rid of a warning about it being unused
		eof_log(text, 1);
	}
}
#endif

int eof_gp_parse_bend(PACKFILE *inf)
{
	unsigned word;
	unsigned long height, points, ctr, dword;

	if(!inf)
		return 1;	//Return error

	eof_gp_debug_log(inf, "\tBend:  ");
	word = pack_getc(inf);	//Read bend type
	if(word == 1)
	{
		(void) puts("Bend");
	}
	else if(word == 2)
	{
		(void) puts("Bend and release");
	}
	else if(word == 3)
	{
		(void) puts("Bend, release and bend");
	}
	else if(word == 4)
	{
		(void) puts("Pre bend");
	}
	else if(word == 5)
	{
		(void) puts("Pre bend and release");
	}
	else if(word == 6)
	{
		(void) puts("Tremolo dip");
	}
	else if(word == 7)
	{
		(void) puts("Tremolo dive");
	}
	else if(word == 8)
	{
		(void) puts("Tremolo release up");
	}
	else if(word == 9)
	{
		(void) puts("Tremolo inverted dip");
	}
	else if(word == 10)
	{
		(void) puts("Tremolo return");
	}
	else if(word == 11)
	{
		(void) puts("Tremolo release down");
	}
	eof_gp_debug_log(inf, "\t\tHeight:  ");
	pack_ReadDWORDLE(inf, &height);	//Read bend height
	printf("%lu cents\n", height);
	eof_gp_debug_log(inf, "\t\tNumber of points:  ");
	pack_ReadDWORDLE(inf, &points);	//Read number of bend points
	printf("%lu points\n", points);
	for(ctr = 0; ctr < points; ctr++)
	{	//For each point in the bend
		if(pack_feof(inf))
		{	//If the end of file was reached unexpectedly
			(void) puts("\aEnd of file reached unexpectedly");
			return 1;	//Return error
		}
		eof_gp_debug_log(inf, "\t\t\tTime relative to previous point:  ");
		pack_ReadDWORDLE(inf, &dword);
		printf("%lu sixtieths\n", dword);
		eof_gp_debug_log(inf, "\t\t\tVertical position:  ");
		pack_ReadDWORDLE(inf, &dword);
		printf("%ld * 25 cents\n", (long)dword);
		eof_gp_debug_log(inf, "\t\t\tVibrato type:  ");
		word = pack_getc(inf);
		if(!word)
		{
			(void) puts("none");
		}
		else if(word == 1)
		{
			(void) puts("fast");
		}
		else if(word == 2)
		{
			(void) puts("average");
		}
		else if(word == 3)
		{
			(void) puts("slow");
		}
	}
	return 0;	//Return success
}

#ifndef EOF_BUILD
	//Standalone parse utility

EOF_SONG *parse_gp(const char * fn)
{
	char buffer[256], byte, *buffer2, bytemask, bytemask2, usedstrings;
	unsigned word, fileversion;
	unsigned long dword, ctr, ctr2, ctr3, ctr4, tracks, measures, *strings, beats, barres;
	char *note_dynamics[8] = {"ppp", "pp", "p", "mp", "mf", "f", "ff", "fff"};
	PACKFILE *inf;

	if(!fn)
	{
		return NULL;
	}
	inf = pack_fopen(fn, "rb");
	if(!inf)
	{
		return NULL;
	}

	eof_gp_debug_log(inf, "File version string:  ");
	(void) eof_read_gp_string(inf, &word, buffer, 0);	//Read file version string
	(void) puts(buffer);
	eof_gp_debug_log(inf, "(skipping ");
	printf("%d bytes of padding)\n", 30 - word);
	(void) pack_fseek(inf, 30 - word);	//Skip the padding that follows the version string
	if(!strcmp(buffer, "FICHIER GUITARE PRO v1.01"))
	{
		fileversion = 101;
	}
	else if(!strcmp(buffer, "FICHIER GUITARE PRO v1.02"))
	{
		fileversion = 102;
	}
	else if(!strcmp(buffer, "FICHIER GUITARE PRO v1.03"))
	{
		fileversion = 103;
	}
	else if(!strcmp(buffer, "FICHIER GUITARE PRO v1.04"))
	{
		fileversion = 104;
	}
	else if(!strcmp(buffer, "FICHIER GUITAR PRO v2.20"))
	{
		fileversion = 220;
	}
	else if(!strcmp(buffer, "FICHIER GUITAR PRO v2.21"))
	{
		fileversion = 221;
	}
	else if(!strcmp(buffer, "FICHIER GUITAR PRO v3.00"))
	{
		fileversion = 300;
	}
	else if(!strcmp(buffer, "FICHIER GUITAR PRO v4.00"))
	{
		fileversion = 400;
	}
	else if(!strcmp(buffer, "FICHIER GUITAR PRO v4.06"))
	{
		fileversion = 406;
	}
	else if(!strcmp(buffer, "FICHIER GUITAR PRO L4.06"))
	{
		fileversion = 406;
	}
	else if(!strcmp(buffer, "FICHIER GUITAR PRO v5.00"))
	{
		fileversion = 500;
	}
	else if(!strcmp(buffer, "FICHIER GUITAR PRO v5.10"))
	{
		fileversion = 510;
	}
	else
	{
		(void) puts("File format version not supported");
		(void) pack_fclose(inf);
		return NULL;
	}

	eof_gp_debug_log(inf, "Title:  ");
	(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read title string
	(void) puts(buffer);
	eof_gp_debug_log(inf, "Subtitle:  ");
	(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read subtitle string
	(void) puts(buffer);
	eof_gp_debug_log(inf, "Artist:  ");
	(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read artist string
	(void) puts(buffer);
	eof_gp_debug_log(inf, "Album:  ");
	(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read album string
	(void) puts(buffer);

	if(fileversion >= 500)
	{	//The words field only exists in version 5.x or higher versions of the format
		eof_gp_debug_log(inf, "Lyricist:  ");
		(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read words string
		(void) puts(buffer);
	}

	eof_gp_debug_log(inf, "Composer:  ");
	(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read music string
	(void) puts(buffer);
	eof_gp_debug_log(inf, "Copyright:  ");
	(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read copyright string
	(void) puts(buffer);
	eof_gp_debug_log(inf, "Transcriber:  ");
	(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read tab string
	(void) puts(buffer);
	eof_gp_debug_log(inf, "Instructions:  ");
	(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read instructions string
	(void) puts(buffer);
	eof_gp_debug_log(inf, "Number of notice entries:  ");
	pack_ReadDWORDLE(inf, &dword);	//Read the number of notice entries
	printf("%lu\n", dword);
	while(dword > 0)
	{	//Read each notice entry
		eof_gp_debug_log(inf, "Notice:  ");
		(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read notice string
		(void) puts(buffer);
		dword--;
	}

	if(fileversion < 500)
	{	//The shuffle rhythm feel field only exists here in version 4.x or older of the format
		eof_gp_debug_log(inf, "Shuffle rhythm feel:  ");
		byte = pack_getc(inf);
		printf("%d\n", byte);
	}

	if(fileversion >= 400)
	{	//The lyrics fields only exist in version 4.x or newer of the format
		pack_ReadDWORDLE(inf, NULL);	//Read the lyrics' associated track number
		for(ctr = 0; ctr < 5; ctr++)
		{
			eof_gp_debug_log(inf, "Lyric");
			pack_ReadDWORDLE(inf, &dword);	//Read the start from bar #
			printf(" (start from bar #%ld):  ", dword);
			pack_ReadDWORDLE(inf, &dword);	//Read the lyric string length
			buffer2 = malloc((size_t)dword + 1);	//Allocate a buffer large enough for the lyric string (plus a NULL terminator)
			if(!buffer2)
			{
				(void) puts("Error allocating memory (9)");
				(void) pack_fclose(inf);
				return 0;
			}
			(void) pack_fread(buffer2, (size_t)dword, inf);	//Read the lyric string
			buffer2[dword] = '\0';		//Terminate the string
			(void) puts(buffer2);
			free(buffer2);	//Free the buffer
		}
	}

	if(fileversion > 500)
	{	//The volume/equalization settings only exist in versions newer than 5.0 of the format
		(void) puts("\tVolume/equalization settings:");
		eof_gp_debug_log(inf, "Master volume:  ");
		pack_ReadDWORDLE(inf, &dword);	//Read the master volume
		printf("%lu\n", dword);
		eof_gp_debug_log(inf, "(skipping 4 bytes of unknown data)\n");
		(void) pack_fseek(inf, 4);		//Unknown data
		eof_gp_debug_log(inf, "32Hz band lowered ");
		byte = pack_getc(inf);
		printf("%d increments of .1dB\n", byte);
		eof_gp_debug_log(inf, "60Hz band lowered ");
		byte = pack_getc(inf);
		printf("%d increments of .1dB\n", byte);
		eof_gp_debug_log(inf, "125Hz band lowered ");
		byte = pack_getc(inf);
		printf("%d increments of .1dB\n", byte);
		eof_gp_debug_log(inf, "250Hz band lowered ");
		byte = pack_getc(inf);
		printf("%d increments of .1dB\n", byte);
		eof_gp_debug_log(inf, "500Hz band lowered ");
		byte = pack_getc(inf);
		printf("%d increments of .1dB\n", byte);
		eof_gp_debug_log(inf, "1KHz band lowered ");
		byte = pack_getc(inf);
		printf("%d increments of .1dB\n", byte);
		eof_gp_debug_log(inf, "2KHz band lowered ");
		byte = pack_getc(inf);
		printf("%d increments of .1dB\n", byte);
		eof_gp_debug_log(inf, "4KHz band lowered ");
		byte = pack_getc(inf);
		printf("%d increments of .1dB\n", byte);
		eof_gp_debug_log(inf, "8KHz band lowered ");
		byte = pack_getc(inf);
		printf("%d increments of .1dB\n", byte);
		eof_gp_debug_log(inf, "16KHz band lowered ");
		byte = pack_getc(inf);
		printf("%d increments of .1dB\n", byte);
		eof_gp_debug_log(inf, "Gain lowered ");
		byte = pack_getc(inf);
		printf("%d increments of .1dB\n", byte);
	}

	if(fileversion >= 500)
	{	//The page setup settings only exist in version 5.x or newer of the format
		(void) puts("\tPage setup settings:");
		eof_gp_debug_log(inf, "Page format:  ");
		pack_ReadDWORDLE(inf, &dword);	//Read the page format length
		printf("%lumm", dword);
		pack_ReadDWORDLE(inf, &dword);	//Read the page format width
		printf(" x %lumm\n", dword);
		eof_gp_debug_log(inf, "Margins:  Left:  ");
		pack_ReadDWORDLE(inf, &dword);	//Read the left margin
		printf("%lumm, ", dword);
		pack_ReadDWORDLE(inf, &dword);	//Read the right margin
		printf("Right: %lumm, ", dword);
		pack_ReadDWORDLE(inf, &dword);	//Read the top margin
		printf("Top: %lumm, ", dword);
		pack_ReadDWORDLE(inf, &dword);	//Read the bottom margin
		printf("Bottom: %lumm\n", dword);
		eof_gp_debug_log(inf, "Score size:  ");
		pack_ReadDWORDLE(inf, &dword);	//Read the score size
		printf("%lu%%\n", dword);
		eof_gp_debug_log(inf, "Header/footer fields bitmask:  ");
		pack_ReadWORDLE(inf, &word);	//Read the enabled header/footer fields bitmask
		printf("%u\n", word);
		(void) puts("\tHeader/footer strings:");
		eof_gp_debug_log(inf, "\tTitle ");
		(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read the title header/footer string
		printf("(%s):  %s\n", (word & 1) ? "enabled" : "disabled", buffer);
		eof_gp_debug_log(inf, "\tSubtitle ");
		(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read the subtitle header/footer string
		printf("(%s):  %s\n", (word & 2) ? "enabled" : "disabled", buffer);
		eof_gp_debug_log(inf, "\tArtist ");
		(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read the artist header/footer string
		printf("(%s):  %s\n", (word & 4) ? "enabled" : "disabled", buffer);
		eof_gp_debug_log(inf, "\tAlbum ");
		(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read the album header/footer string
		printf("(%s):  %s\n", (word & 8) ? "enabled" : "disabled", buffer);
		eof_gp_debug_log(inf, "\tLyricist ");
		(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read the words (lyricist) header/footer string
		printf("(%s):  %s\n", (word & 16) ? "enabled" : "disabled", buffer);
		eof_gp_debug_log(inf, "\tComposer ");
		(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read the music (composer) header/footer string
		printf("(%s):  %s\n", (word & 32) ? "enabled" : "disabled", buffer);
		eof_gp_debug_log(inf, "\tLyricist and Composer ");
		(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read the words & music header/footer string
		printf("(%s):  %s\n", (word & 64) ? "enabled" : "disabled", buffer);
		eof_gp_debug_log(inf, "\tCopyright 1 ");
		(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read the Copyright line 1 header/footer string
		printf("(%s):  %s\n", (word & 128) ? "enabled" : "disabled", buffer);
		eof_gp_debug_log(inf, "\tCopyright 2 ");
		(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read the Copyright line 2 header/footer string
		printf("(%s):  %s\n", (word & 128) ? "enabled" : "disabled", buffer);
		eof_gp_debug_log(inf, "\tPage number ");
		(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read the page number header/footer string
		printf("(%s):  %s\n", (word & 256) ? "enabled" : "disabled", buffer);
	}

	eof_gp_debug_log(inf, "Tempo:  ");
	if(fileversion >= 500)
	{	//The tempo string only exists in version 5.x or newer of the format
		(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read the tempo string
	}
	pack_ReadDWORDLE(inf, &dword);	//Read the tempo
	printf("%luBPM", dword);
	if((fileversion >= 500) && (buffer[0] != '\0'))
	{	//The tempo string only exists in version 5.x or newer of the format
		printf(" (%s)", buffer);
	}
	(void) putchar('\n');

	if(fileversion > 500)
	{	//There is a byte of unknown data/padding here in versions newer than 5.0 of the format
		eof_gp_debug_log(inf, "(skipping 1 byte of unknown data)\n");
		(void) pack_fseek(inf, 1);		//Unknown data
	}

	if(fileversion >= 400)
	{	//Versions 4.0 and newer of the format store key and octave information here
		eof_gp_debug_log(inf, "Key:  ");
		byte = pack_getc(inf);	//Read the key
		printf("%d (%u %s)\n", byte, abs(byte), (byte < 0) ? "flats" : "sharps");
		eof_gp_debug_log(inf, "(skipping 3 bytes of unknown data)\n");
		(void) pack_fseek(inf, 3);		//Unknown data
		eof_gp_debug_log(inf, "Transpose:  ");
		word = pack_getc(inf);
//		pack_ReadDWORDLE(inf, &dword);	//Read the transpose field
		printf("%u\n", word);
	}
	else
	{	//Older versions stored only key information here
		pack_ReadDWORDLE(inf, &dword);	//Read the key
		byte = (char)dword;				//Store as signed integer value
		printf("%d (%u %s)\n", byte, abs(byte), (byte < 0) ? "flats" : "sharps");
	}

	eof_gp_debug_log(inf, "Track data (64 chunks of data)\n");
	for(ctr = 0; ctr < 64; ctr++)
	{
		pack_ReadDWORDLE(inf, NULL);	//Read the instrument patch number
		(void) pack_getc(inf);			//Read the volume
		(void) pack_getc(inf);			//Read the pan value
		(void) pack_getc(inf);			//Read the chorus value
		(void) pack_getc(inf);			//Read the reverb value
		(void) pack_getc(inf);			//Read the phaser value
		(void) pack_getc(inf);			//Read the tremolo value
		pack_ReadWORDLE(inf, NULL);		//Read two bytes of unknown data/padding
	}

	if(fileversion >= 500)
	{	//Versions 5.0 and newer of the format store musical directional symbols and a master reverb setting here
		(void) puts("\tMusical symbols:");
		eof_gp_debug_log(inf, "\"Coda\" symbol is ");
		pack_ReadWORDLE(inf, &word);
		if(word == 0xFFFF)
		{
			(void) puts("unused");
		}
		else
		{
			printf("at beat #%u\n", word);
		}
		eof_gp_debug_log(inf, "\"Double coda\" symbol is ");
		pack_ReadWORDLE(inf, &word);
		if(word == 0xFFFF)
		{
			(void) puts("unused");
		}
		else
		{
			printf("at beat #%u\n", word);
		}
		eof_gp_debug_log(inf, "\"Segno\" symbol is ");
		pack_ReadWORDLE(inf, &word);
		if(word == 0xFFFF)
		{
			(void) puts("unused");
		}
		else
		{
			printf("at beat #%u\n", word);
		}
		eof_gp_debug_log(inf, "\"Segno segno\" symbol is ");
		pack_ReadWORDLE(inf, &word);
		if(word == 0xFFFF)
		{
			(void) puts("unused");
		}
		else
		{
			printf("at beat #%u\n", word);
		}
		eof_gp_debug_log(inf, "\"Fine\" symbol is ");
		pack_ReadWORDLE(inf, &word);
		if(word == 0xFFFF)
		{
			(void) puts("unused");
		}
		else
		{
			printf("at beat #%u\n", word);
		}
		eof_gp_debug_log(inf, "\"Da capo\" symbol is ");
		pack_ReadWORDLE(inf, &word);
		if(word == 0xFFFF)
		{
			(void) puts("unused");
		}
		else
		{
			printf("at beat #%u\n", word);
		}
		eof_gp_debug_log(inf, "\"Da capo al coda\" symbol is ");
		pack_ReadWORDLE(inf, &word);
		if(word == 0xFFFF)
		{
			(void) puts("unused");
		}
		else
		{
			printf("at beat #%u\n", word);
		}
		eof_gp_debug_log(inf, "\"Da capo al double coda\" symbol is ");
		pack_ReadWORDLE(inf, &word);
		if(word == 0xFFFF)
		{
			(void) puts("unused");
		}
		else
		{
			printf("at beat #%u\n", word);
		}
		eof_gp_debug_log(inf, "\"Da capo al fine\" symbol is ");
		pack_ReadWORDLE(inf, &word);
		if(word == 0xFFFF)
		{
			(void) puts("unused");
		}
		else
		{
			printf("at beat #%u\n", word);
		}
		eof_gp_debug_log(inf, "\"Da segno\" symbol is ");
		pack_ReadWORDLE(inf, &word);
		if(word == 0xFFFF)
		{
			(void) puts("unused");
		}
		else
		{
			printf("at beat #%u\n", word);
		}
		eof_gp_debug_log(inf, "\"Da segno al coda\" symbol is ");
		pack_ReadWORDLE(inf, &word);
		if(word == 0xFFFF)
		{
			(void) puts("unused");
		}
		else
		{
			printf("at beat #%u\n", word);
		}
		eof_gp_debug_log(inf, "\"Da segno al double coda\" symbol is ");
		pack_ReadWORDLE(inf, &word);
		if(word == 0xFFFF)
		{
			(void) puts("unused");
		}
		else
		{
			printf("at beat #%u\n", word);
		}
		eof_gp_debug_log(inf, "\"Da segno al fine\" symbol is ");
		pack_ReadWORDLE(inf, &word);
		if(word == 0xFFFF)
		{
			(void) puts("unused");
		}
		else
		{
			printf("at beat #%u\n", word);
		}
		eof_gp_debug_log(inf, "\"Da segno segno\" symbol is ");
		pack_ReadWORDLE(inf, &word);
		if(word == 0xFFFF)
		{
			(void) puts("unused");
		}
		else
		{
			printf("at beat #%u\n", word);
		}
		eof_gp_debug_log(inf, "\"Da segno segno al coda\" symbol is ");
		pack_ReadWORDLE(inf, &word);
		if(word == 0xFFFF)
		{
			(void) puts("unused");
		}
		else
		{
			printf("at beat #%u\n", word);
		}
		eof_gp_debug_log(inf, "\"Da segno segno al double coda\" symbol is ");
		pack_ReadWORDLE(inf, &word);
		if(word == 0xFFFF)
		{
			(void) puts("unused");
		}
		else
		{
			printf("at beat #%u\n", word);
		}
		eof_gp_debug_log(inf, "\"Da segno segno al fine\" symbol is ");
		pack_ReadWORDLE(inf, &word);
		if(word == 0xFFFF)
		{
			(void) puts("unused");
		}
		else
		{
			printf("at beat #%u\n", word);
		}
		eof_gp_debug_log(inf, "\"Da coda\" symbol is ");
		pack_ReadWORDLE(inf, &word);
		if(word == 0xFFFF)
		{
			(void) puts("unused");
		}
		else
		{
			printf("at beat #%u\n", word);
		}
		eof_gp_debug_log(inf, "\"Da double coda\" symbol is ");
		pack_ReadWORDLE(inf, &word);
		if(word == 0xFFFF)
		{
			(void) puts("unused");
		}
		else
		{
			printf("at beat #%u\n", word);
		}
		eof_gp_debug_log(inf, "Master reverb:  ");
		pack_ReadDWORDLE(inf, &dword);	//Read the master reverb value
		printf("%ld\n", dword);
	}

	eof_gp_debug_log(inf, "Number of measures:  ");
	pack_ReadDWORDLE(inf, &measures);	//Read the number of measures
	printf("%ld\n", measures);
	eof_gp_debug_log(inf, "Number of tracks:  ");
	pack_ReadDWORDLE(inf, &tracks);	//Read the number of tracks
	printf("%ld\n", tracks);

	//Allocate memory for an array to track the number of strings for each track
	strings = malloc(sizeof(unsigned long) * tracks);
	if(!strings)
	{
		(void) puts("Error allocating memory (10)");
		(void) pack_fclose(inf);
		return NULL;
	}

	for(ctr = 0; ctr < measures; ctr++)
	{	//For each measure
		printf("\tStart of definition for measure #%lu\n", ctr + 1);
		eof_gp_debug_log(inf, "\tMeasure bitmask:  ");
		bytemask = pack_getc(inf);	//Read the measure bitmask
		printf("%u\n", (bytemask & 0xFF));
		if(fileversion < 300)
		{	//Versions of the format older than 3.0
			if(bytemask & 1)
			{	//Start of repeat
				eof_gp_debug_log(inf, "\tStart of repeat\n");
			}
			if(bytemask & 2)
			{	//End of repeat
				eof_gp_debug_log(inf, "\tEnd of repeat:  ");
				word = pack_getc(inf);	//Read number of repeats
				printf("%u repeats\n", word);
			}
			if(bytemask & 4)
			{	//Number of alternate ending
				eof_gp_debug_log(inf, "\tNumber of alternate ending:  ");
				word = pack_getc(inf);	//Read alternate ending number
				printf("%u\n", word);
			}
		}
		else
		{	//Versions of the format 3.0 and newer
			if(bytemask & 1)
			{	//Time signature change (numerator)
				eof_gp_debug_log(inf, "\tTS numerator:  ");
				word = pack_getc(inf);
				printf("%u\n", word);
			}
			if(bytemask & 2)
			{	//Time signature change (denominator)
				eof_gp_debug_log(inf, "\tTS denominator:  ");
				word = pack_getc(inf);
				printf("%u\n", word);
			}
			if(bytemask & 4)
			{	//Start of repeat
				eof_gp_debug_log(inf, "\tStart of repeat\n");
			}
			if(bytemask & 8)
			{	//End of repeat
				eof_gp_debug_log(inf, "\tEnd of repeat:  ");
				word = pack_getc(inf);	//Read number of repeats
				if(fileversion >= 500)
				{	//Version 5 of the format has slightly different counting for repeats
					word--;
				}
				printf("%u repeats\n", word);
			}
			if(fileversion < 500)
			{	//Versions 3 and 4 define the alternate ending next, followed by the section definition
				if(bytemask & 16)
				{	//Number of alternate ending
					eof_gp_debug_log(inf, "\tNumber of alternate ending:  ");
					word = pack_getc(inf);	//Read alternate ending number
					printf("%u\n", word);
				}
				if(bytemask & 32)
				{	//New section
					eof_gp_debug_log(inf, "\tNew section:  ");
					(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read section string
					printf("%s (coloring:  ", buffer);
					word = pack_getc(inf);
					printf("R = %u, ", word);
					word = pack_getc(inf);
					printf("G = %u, ", word);
					word = pack_getc(inf);
					printf("B = %u)\n", word);
					(void) pack_getc(inf);	//Read unused value
				}
			}
			else
			{	//Version 5 defines these two items in the opposite order
				if(bytemask & 32)
				{	//New section
					eof_gp_debug_log(inf, "\tNew section:  ");
					(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read section string
					printf("%s (coloring:  ", buffer);
					word = pack_getc(inf);
					printf("R = %u, ", word);
					word = pack_getc(inf);
					printf("G = %u, ", word);
					word = pack_getc(inf);
					printf("B = %u)\n", word);
					(void) pack_getc(inf);	//Read unused value
				}
				if(bytemask & 16)
				{	//Number of alternate ending
					eof_gp_debug_log(inf, "\tNumber of alternate ending:  ");
					word = pack_getc(inf);	//Read alternate ending number
					printf("%u\n", word);
				}
			}
			if(bytemask & 64)
			{	//Key signature change
				eof_gp_debug_log(inf, "\tNew key signature:  ");
				byte = pack_getc(inf);	//Read the key
				printf("%d (%u %s, ", byte, abs(byte), (byte < 0) ? "flats" : "sharps");
				byte = pack_getc(inf);	//Read the major/minor byte
				printf("%s)\n", !byte ? "major" : "minor");
			}
			if(fileversion >= 500)
			{	//Version 5 of the format defines additional things here
				if((bytemask & 1) || (bytemask & 2))
				{	//If either a new TS numerator or denominator was set, read the beam by eight notes values
					char byte1, byte2, byte3, byte4;
					eof_gp_debug_log(inf, "\tBeam eight notes by:  ");
					byte1 = pack_getc(inf);
					byte2 = pack_getc(inf);
					byte3 = pack_getc(inf);
					byte4 = pack_getc(inf);
					printf("%d + %d + %d + %d = %d\n", byte1, byte2, byte3, byte4, byte1 + byte2 + byte3 + byte4);
				}
				if(!(bytemask & 16))
				{	//If a GP5 file doesn't define an alternate ending here, ignore a byte of padding
					eof_gp_debug_log(inf, "\t(skipping 1 byte of unused alternate ending data)\n");
					(void) pack_getc(inf);			//Unknown data
				}
				eof_gp_debug_log(inf, "\tTriplet feel:  ");
				byte = pack_getc(inf);	//Read triplet feel value
				printf("%s\n", !byte ? "none" : ((byte == 1) ? "Triplet 8th" : "Triplet 16th"));
				eof_gp_debug_log(inf, "\t(skipping 1 byte of unknown data)\n");
				(void) pack_getc(inf);	//Unknown data/padding
			}
		}//Versions of the format 3.0 and newer
		if(bytemask & 128)
		{	//Double bar
			(void) puts("\t\t(Double bar)");
		}
	}//For each measure

	for(ctr = 0; ctr < tracks; ctr++)
	{	//For each track
		printf("\tStart of definition for track #%lu\n", ctr + 1);
		eof_gp_debug_log(inf, "\tTrack bitmask:  ");
		bytemask = pack_getc(inf);	//Read the track bitmask
		printf("%u\n", (bytemask & 0xFF));
		if(bytemask & 1)
		{
			(void) puts("\t\t\t(Is a drum track)");
		}
		if(bytemask & 2)
		{
			(void) puts("\t\t\t(Is a 12 string guitar track)");
		}
		if(bytemask & 4)
		{
			(void) puts("\t\t\t(Is a banjo track)");
		}
		if(bytemask & 16)
		{
			(void) puts("\t\t\t(Is marked for solo playback)");
		}
		if(bytemask & 32)
		{
			(void) puts("\t\t\t(Is marked for muted playback)");
		}
		if(bytemask & 64)
		{
			(void) puts("\t\t\t(Is marked for RSE playback)");
		}
		if(bytemask & 128)
		{
			(void) puts("\t\t\t(Is set to have the tuning displayed)");
		}
		eof_gp_debug_log(inf, "\tTrack name string:  ");
		(void) eof_read_gp_string(inf, &word, buffer, 0);	//Read track name string
		(void) puts(buffer);
		eof_gp_debug_log(inf, "\t(skipping ");
		printf("%d bytes of padding)\n", 40 - word);
		(void) pack_fseek(inf, 40 - word);			//Skip the padding that follows the track name string
		eof_gp_debug_log(inf, "\tNumber of strings:  ");
		pack_ReadDWORDLE(inf, &strings[ctr]);	//Read the number of strings in this track
		printf("%lu\n", strings[ctr]);
		for(ctr2 = 0; ctr2 < 7; ctr2++)
		{	//For each of the 7 possible usable strings
			if(ctr2 < strings[ctr])
			{	//If this string is used
				eof_gp_debug_log(inf, "\tTuning for string #");
				pack_ReadDWORDLE(inf, &dword);	//Read the tuning for this string
				printf("%lu:  MIDI note %lu (%s)\n", ctr2 + 1, dword, eof_note_names[(dword + 3) % 12]);
			}
			else
			{
				eof_gp_debug_log(inf, "\t(skipping definition for unused string)\n");
				pack_ReadDWORDLE(inf, NULL);	//Skip this padding
			}
		}
		eof_gp_debug_log(inf, "\tMIDI port:  ");
		pack_ReadDWORDLE(inf, &dword);	//Read the MIDI port used for this track
		printf("%lu\n", dword);
		eof_gp_debug_log(inf, "\tMIDI channel:  ");
		pack_ReadDWORDLE(inf, &dword);	//Read the MIDI channel used for this track
		printf("%lu\n", dword);
		eof_gp_debug_log(inf, "\tEffects MIDI channel:  ");
		pack_ReadDWORDLE(inf, &dword);	//Read the MIDI channel used for this track's effects
		printf("%lu\n", dword);
		eof_gp_debug_log(inf, "\tNumber of frets:  ");
		pack_ReadDWORDLE(inf, &dword);	//Read the number of frets used for this track
		printf("%lu\n", dword);
		eof_gp_debug_log(inf, "\tCapo:  ");
		pack_ReadDWORDLE(inf, &dword);	//Read the capo position for this track
		if(dword)
		{	//If there is a capo
			printf("Fret %lu\n", dword);
		}
		else
		{	//There is no capo
			(void) puts("(none)");
		}
		eof_gp_debug_log(inf, "\tTrack color:  ");
		word = pack_getc(inf);
		printf("R = %u, ", word);
		word = pack_getc(inf);
		printf("G = %u, ", word);
		word = pack_getc(inf);
		printf("B = %u\n", word);
		(void) pack_getc(inf);	//Read unused value

		if(fileversion > 500)
		{
			eof_gp_debug_log(inf, "\tTrack properties 1 bitmask:  ");
			bytemask = pack_getc(inf);
			printf("%u\n", (bytemask & 0xFF));
			eof_gp_debug_log(inf, "\tTrack properties 2 bitmask:  ");
			bytemask2 = pack_getc(inf);
			printf("%u\n", (bytemask2 & 0xFF));
			eof_gp_debug_log(inf, "\t(skipping 1 byte of unknown data)\n");
			(void) pack_getc(inf);			//Unknown data
			eof_gp_debug_log(inf, "\tMIDI bank:  ");
			word = pack_getc(inf);
			printf("%u\n", word);
			eof_gp_debug_log(inf, "\tHuman playing value:  ");
			word = pack_getc(inf);
			printf("%u\n", word);
			eof_gp_debug_log(inf, "\tAuto accentuation on the beat:  ");
			word = pack_getc(inf);
			printf("%u\n", word);
			eof_gp_debug_log(inf, "\t(skipping 31 bytes of unknown data)\n");
			(void) pack_fseek(inf, 31);		//Unknown data
			eof_gp_debug_log(inf, "\tSelected sound bank option:  ");
			word = pack_getc(inf);
			printf("%u\n", word);
			eof_gp_debug_log(inf, "\t(skipping 7 bytes of unknown data)\n");
			(void) pack_fseek(inf, 7);		//Unknown data
			eof_gp_debug_log(inf, "\tLow frequency band lowered ");
			byte = pack_getc(inf);
			printf("%d increments of .1dB\n", byte);
			eof_gp_debug_log(inf, "\tMid frequency band lowered ");
			byte = pack_getc(inf);
			printf("%d increments of .1dB\n", byte);
			eof_gp_debug_log(inf, "\tHigh frequency band lowered ");
			byte = pack_getc(inf);
			printf("%d increments of .1dB\n", byte);
			eof_gp_debug_log(inf, "\tGain lowered ");
			byte = pack_getc(inf);
			printf("%d increments of .1dB\n", byte);
			eof_gp_debug_log(inf, "\tTrack instrument effect 1:  ");
			(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read track instrument effect 1
			(void) puts(buffer);
			eof_gp_debug_log(inf, "\tTrack instrument effect 2:  ");
			(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read track instrument effect 2
			(void) puts(buffer);
		}
		else if(fileversion == 500)
		{
			eof_gp_debug_log(inf, "\t(skipping 45 bytes of unknown data)\n");
			(void) pack_fseek(inf, 45);		//Unknown data
		}
	}//For each track

	if(fileversion >= 500)
	{
		eof_gp_debug_log(inf, "\t(skipping 1 byte of unknown data)\n");
		(void) pack_getc(inf);
	}

	(void) puts("\tStart of beat definitions:");
	for(ctr = 0; ctr < measures; ctr++)
	{	//For each measure
		printf("\t-> Measure # %lu\n", ctr + 1);
		for(ctr2 = 0; ctr2 < tracks; ctr2++)
		{	//For each track
			unsigned voice, maxvoices = 1;
			if(fileversion >= 500)
			{	//Version 5.0 and higher of the format stores two "voices" (lead and bass) for each track
				maxvoices = 2;
			}
			for(voice = 0; voice < maxvoices; voice++)
			{	//For each voice
				printf("\t-> M#%lu -> Track # %lu (voice %u)\n", ctr + 1, ctr2 + 1, voice + 1);
				eof_gp_debug_log(inf, "\tNumber of beats:  ");
				pack_ReadDWORDLE(inf, &beats);
				printf("%lu\n", beats);
				for(ctr3 = 0; ctr3 < beats; ctr3++)
				{	//For each beat
					unsigned bitmask;

					printf("\t-> M#%lu -> T#%lu -> Beat # %lu\n", ctr + 1, ctr2 + 1, ctr3 + 1);
					eof_gp_debug_log(inf, "\tBeat bitmask:  ");
					bytemask = pack_getc(inf);	//Read beat bitmask
					printf("%u\n", (bytemask & 0xFF));
					if(bytemask & 64)
					{	//Beat is a rest
						eof_gp_debug_log(inf, "\t(Rest beat type:  ");
						word = pack_getc(inf);
						printf("%s)\n", !word ? "empty" : "rest");
					}
					eof_gp_debug_log(inf, "\tBeat duration:  ");
					byte = pack_getc(inf);	//Read beat duration
					printf("%d\n", byte);
					if(bytemask & 1)
					{	//Dotted note
						(void) puts("\t\t(Dotted note)");
					}
					if(bytemask & 32)
					{	//Beat is an N-tuplet
						eof_gp_debug_log(inf, "\t(N-tuplet:  ");
						pack_ReadDWORDLE(inf, &dword);
						printf("%lu)\n", dword);
					}
					if(bytemask & 2)
					{	//Beat has a chord diagram
						eof_gp_debug_log(inf, "\t(Chord diagram, ");
						word = pack_getc(inf);	//Read chord diagram format
						printf("format %u)\n", word);
						if(word == 0)
						{	//Chord diagram format 0, ie. GP3
							eof_gp_debug_log(inf, "\t\tChord name:  ");
							(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read chord name
							(void) puts(buffer);
							eof_gp_debug_log(inf, "\t\tDiagram begins at fret #");
							pack_ReadDWORDLE(inf, &dword);	//Read the diagram fret position
							printf("%lu\n", dword);
							if(dword)
							{	//If the diagram is properly defined
								for(ctr4 = 0; ctr4 < strings[ctr2]; ctr4++)
								{	//For each string defined in the track
									eof_gp_debug_log(inf, "\t\tString #");
									pack_ReadDWORDLE(inf, &dword);	//Read the fret played on this string
									if((long)dword == -1)
									{	//String not played
										printf("%lu:  X\n", ctr4 + 1);
									}
									else
									{
										printf("%lu:  %lu\n", ctr4 + 1, dword);
									}
								}
							}
						}//Chord diagram format 0, ie. GP3
						else if(word == 1)
						{	//Chord diagram format 1, ie. GP4
							eof_gp_debug_log(inf, "\t\tDisplay as ");
							word = pack_getc(inf);	//Read sharp/flat indicator
							printf("%s\n", !word ? "flat" : "sharp");
							eof_gp_debug_log(inf, "\t\t(skipping 3 bytes of unknown data)\n");
							(void) pack_fseek(inf, 3);		//Unknown data
							eof_gp_debug_log(inf, "\t\tChord root:  ");
							word = pack_getc(inf);	//Read chord root
							printf("%u\n", word);
							if(fileversion / 100 == 3)
							{	//If it is a GP 3.x file
								eof_gp_debug_log(inf, "\t\t(skipping 3 bytes of unknown data)\n");
								(void) pack_fseek(inf, 3);		//Unknown data
							}
							eof_gp_debug_log(inf, "\t\tChord type:  ");
							word = pack_getc(inf);	//Read chord type
							printf("%u\n", word);
							if(fileversion / 100 == 3)
							{	//If it is a GP 3.x file
								eof_gp_debug_log(inf, "\t\t(skipping 3 bytes of unknown data)\n");
								(void) pack_fseek(inf, 3);		//Unknown data
							}
							eof_gp_debug_log(inf, "\t\t9th/11th/13th option:  ");
							word = pack_getc(inf);
							printf("%u\n", word);
							if(fileversion / 100 == 3)
							{	//If it is a GP 3.x file
								eof_gp_debug_log(inf, "\t\t(skipping 3 bytes of unknown data)\n");
								(void) pack_fseek(inf, 3);		//Unknown data
							}
							eof_gp_debug_log(inf, "\t\tBass note:  ");
							pack_ReadDWORDLE(inf, &dword);	//Read lowest note played in string
							printf("%lu (%s)\n", dword, eof_note_names[(dword + 3) % 12]);
							eof_gp_debug_log(inf, "\t\t+/- option:  ");
							word = pack_getc(inf);
							printf("%u\n", word);
							eof_gp_debug_log(inf, "\t\t(skipping 4 bytes of unknown data)\n");
							(void) pack_fseek(inf, 4);		//Unknown data
							eof_gp_debug_log(inf, "\t\tChord name:  ");
							word = pack_getc(inf);	//Read chord name string length
							(void) pack_fread(buffer, 20, inf);	//Read chord name (which is padded to 20 bytes)
							buffer[word] = '\0';	//Ensure string is terminated to be the right length
							(void) puts(buffer);
							eof_gp_debug_log(inf, "\t\t(skipping 2 bytes of unknown data)\n");
							(void) pack_fseek(inf, 2);		//Unknown data
							eof_gp_debug_log(inf, "\t\tTonality of the fifth:  ");
							byte = pack_getc(inf);
							printf("%s\n", !byte ? "perfect" : ((byte == 1) ? "augmented" : "diminished"));
							if(fileversion / 100 == 3)
							{	//If it is a GP 3.x file
								eof_gp_debug_log(inf, "\t\t(skipping 3 bytes of unknown data)\n");
								(void) pack_fseek(inf, 3);		//Unknown data
							}
							eof_gp_debug_log(inf, "\t\tTonality of the ninth:  ");
							byte = pack_getc(inf);
							printf("%s\n", !byte ? "perfect" : ((byte == 1) ? "augmented" : "diminished"));
							if(fileversion / 100 == 3)
							{	//If it is a GP 3.x file
								eof_gp_debug_log(inf, "\t\t(skipping 3 bytes of unknown data)\n");
								(void) pack_fseek(inf, 3);		//Unknown data
							}
							eof_gp_debug_log(inf, "\t\tTonality of the eleventh:  ");
							byte = pack_getc(inf);
							printf("%s\n", !byte ? "perfect" : ((byte == 1) ? "augmented" : "diminished"));
							if(fileversion / 100 == 3)
							{	//If it is a GP 3.x file
								eof_gp_debug_log(inf, "\t\t(skipping 3 bytes of unknown data)\n");
								(void) pack_fseek(inf, 3);		//Unknown data
							}
							eof_gp_debug_log(inf, "\t\tBase fret for diagram:  ");
							pack_ReadDWORDLE(inf, &dword);
							printf("%lu\n", dword);
							for(ctr4 = 0; ctr4 < 7; ctr4++)
							{	//For each of the 7 possible usable strings
								if(ctr4 < strings[ctr2])
								{	//If this string is used in the track
									eof_gp_debug_log(inf, "\t\tString #");
									pack_ReadDWORDLE(inf, &dword);
									if((long)dword == -1)
									{	//String not used in diagram
										printf("%lu:  (String unused)\n", ctr4 + 1);
									}
									else
									{
										printf("%lu:  Fret #%lu\n", ctr4 + 1, dword);
									}
								}
								else
								{
									eof_gp_debug_log(inf, "\t\t(skipping definition for unused string)\n");
									pack_ReadDWORDLE(inf, NULL);	//Skip this padding
								}
							}
							barres = pack_getc(inf);	//Read the number of barres in this chord
							for(ctr4 = 0; ctr4 < 5; ctr4++)
							{	//For each of the 5 possible barres
								if(ctr4 < barres)
								{	//If this barre is defined
									eof_gp_debug_log(inf, "\t\tBarre #");
									word = pack_getc(inf);	//Read the barre position
									printf("%lu:  Fret #%u\n", ctr4 + 1, word);
								}
								else
								{
									eof_gp_debug_log(inf, "\t\t(skipping fret definition for undefined barre)\n");
									(void) pack_getc(inf);
								}
							}
							for(ctr4 = 0; ctr4 < 5; ctr4++)
							{	//For each of the 5 possible barres
								if(ctr4 < barres)
								{		//If this barre is defined
									eof_gp_debug_log(inf, "\t\tBarre #");
									word = pack_getc(inf);	//Read the barre start string
									printf("%lu starts at string #%u\n", ctr4 + 1, word);
								}
								else
								{
									eof_gp_debug_log(inf, "\t\t(skipping start definition for undefined barre)\n");
									(void) pack_getc(inf);
								}
							}
							for(ctr4 = 0; ctr4 < 5; ctr4++)
							{	//For each of the 5 possible barres
								if(ctr4 < barres)
								{	//If this barre is defined
									eof_gp_debug_log(inf, "\t\tBarre #");
									word = pack_getc(inf);	//Read the barre stop string
									printf("%lu ends at string #%u\n", ctr4 + 1, word);
								}
								else
								{
									eof_gp_debug_log(inf, "\t\t(skipping stop definition for undefined barre)\n");
									(void) pack_getc(inf);
								}
							}
							eof_gp_debug_log(inf, "\t\tChord includes first interval:  ");
							byte = pack_getc(inf);
							printf("%s\n", byte ? "no" : "yes");	//These booleans define whether the interval is EXCLUDED
							eof_gp_debug_log(inf, "\t\tChord includes third interval:  ");
							byte = pack_getc(inf);
							printf("%s\n", byte ? "no" : "yes");
							eof_gp_debug_log(inf, "\t\tChord includes fifth interval:  ");
							byte = pack_getc(inf);
							printf("%s\n", byte ? "no" : "yes");
							eof_gp_debug_log(inf, "\t\tChord includes seventh interval:  ");
							byte = pack_getc(inf);
							printf("%s\n", byte ? "no" : "yes");
							eof_gp_debug_log(inf, "\t\tChord includes ninth interval:  ");
							byte = pack_getc(inf);
							printf("%s\n", byte ? "no" : "yes");
							eof_gp_debug_log(inf, "\t\tChord includes eleventh interval:  ");
							byte = pack_getc(inf);
							printf("%s\n", byte ? "no" : "yes");
							eof_gp_debug_log(inf, "\t\tChord includes thirteenth interval:  ");
							byte = pack_getc(inf);
							printf("%s\n", byte ? "no" : "yes");
							eof_gp_debug_log(inf, "\t\t(skipping 1 byte of unknown data)\n");
							(void) pack_getc(inf);	//Unknown data
							for(ctr4 = 0; ctr4 < 7; ctr4++)
							{	//For each of the 7 possible usable strings
								if(ctr4 < strings[ctr2])
								{	//If this string is used in the track
									eof_gp_debug_log(inf, "\t\tString #");
									byte = pack_getc(inf);
									printf("%lu is played with finger %d\n", ctr4 + 1, byte);
								}
								else
								{
									eof_gp_debug_log(inf, "\t\t(skipping definition for unused string)\n");
									(void) pack_getc(inf);		//Skip this padding
								}
							}
							eof_gp_debug_log(inf, "\t\tChord fingering displayed:  ");
							byte = pack_getc(inf);
							printf("%s\n", !byte ? "no" : "yes");
						}//Chord diagram format 1, ie. GP4
					}//Beat has a chord diagram
					if(bytemask & 4)
					{	//Beat has text
						eof_gp_debug_log(inf, "\tBeat text:  ");
						(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read beat text string
						(void) puts(buffer);
					}
					if(bytemask & 8)
					{	//Beat has effects
						unsigned char byte1, byte2 = 0;

						eof_gp_debug_log(inf, "\tBeat effects bitmask:  ");
						byte1 = pack_getc(inf);	//Read beat effects 1 bitmask
						printf("%u\n", (byte1 & 0xFF));
						if(fileversion >= 400)
						{	//Versions 4.0 and higher of the format have a second beat effects bitmask
							eof_gp_debug_log(inf, "\tExtended beat effects bitmask:  ");
							byte2 = pack_getc(inf);
							printf("%u\n", (byte2 & 0xFF));
							if(byte2 & 1)
							{
								(void) puts("\t\tRasguedo");
							}
						}
						if(byte1 & 1)
						{	//Vibrato
							(void) puts("\t\t(Vibrato)");
						}
						if(byte1 & 2)
						{
							(void) puts("\t\t(Wide vibrato)");
						}
						if(byte1 & 4)
						{
							(void) puts("\t\t(Natural harmonic)");
						}
						if(byte1 & 8)
						{
							(void) puts("\t\t(Artificial harmonic)");
						}
						if(byte1 & 32)
						{	//Tapping/popping/slapping
							eof_gp_debug_log(inf, "\tString effect:  ");
							byte = pack_getc(inf);	//Read tapping/popping/slapping indicator
							if(byte == 0)
							{
								(void) puts("Tremolo bar");
							}
							else if(byte == 1)
							{
								(void) puts("Tapping");
							}
							else if(byte == 2)
							{
								(void) puts("Slapping");
							}
							else if(byte == 3)
							{
								(void) puts("Popping");
							}
							else
							{
								(void) puts("Unknown");
							}
							if(fileversion < 400)
							{
								eof_gp_debug_log(inf, "\t\tString effect value:  ");
								pack_ReadDWORDLE(inf, &dword);
								printf("%lu\n", dword);
							}
						}
						if(byte2 & 4)
						{	//Tremolo bar
							if(eof_gp_parse_bend(inf))
							{	//If there was an error parsing the bend
								(void) puts("Error parsing bend");
								(void) pack_fclose(inf);
								free(strings);
								return NULL;
							}
						}
						if(byte1 & 64)
						{	//Stroke effect
							eof_gp_debug_log(inf, "\tUpstroke speed:  ");
							byte = pack_getc(inf);
							printf("%u\n", (byte & 0xFF));
							eof_gp_debug_log(inf, "\tDownstroke speed:  ");
							byte = pack_getc(inf);
							printf("%u\n", (byte & 0xFF));
						}
						if(byte2 & 2)
						{	//Pickstroke effect
							eof_gp_debug_log(inf, "\tPickstroke effect:  ");
							byte = pack_getc(inf);
							printf("%s\n", !byte ? "none" : ((byte == 1) ? "up" : "down"));
						}
					}//Beat has effects
					if(bytemask & 16)
					{	//Beat has mix table change
						char volume_change = 0, pan_change = 0, chorus_change = 0, reverb_change = 0, phaser_change = 0, tremolo_change = 0, tempo_change = 0;

						(void) puts("\t\tBeat mix table change:");
						eof_gp_debug_log(inf, "\t\tNew instrument number:  ");
							byte = pack_getc(inf);
						if(byte == -1)
						{
							(void) puts("(no change)");
						}
						else
						{
							printf("%d\n", byte);
						}
						if(fileversion >= 500)
						{	//These fields are only in version 5.x files
							eof_gp_debug_log(inf, "\t\tRSE related number:  ");
							pack_ReadDWORDLE(inf, &dword);
							printf("%lu\n", dword);
							eof_gp_debug_log(inf, "\t\tRSE related number:  ");
							pack_ReadDWORDLE(inf, &dword);
							printf("%lu\n", dword);
							eof_gp_debug_log(inf, "\t\tRSE related number:  ");
							pack_ReadDWORDLE(inf, &dword);
							printf("%lu\n", dword);
							eof_gp_debug_log(inf, "\t\t(skipping 4 bytes of unknown data)\n");
							(void) pack_fseek(inf, 4);		//Unknown data
						}
						eof_gp_debug_log(inf, "\t\tNew volume:  ");
						byte = pack_getc(inf);
						if(byte == -1)
						{
							(void) puts("(no change)");
						}
						else
						{
							volume_change = 1;
							printf("%d\n", byte);
						}
						eof_gp_debug_log(inf, "\t\tNew pan value:  ");
						byte = pack_getc(inf);
						if(byte == -1)
						{
							(void) puts("(no change)");
						}
						else
						{
							pan_change = 1;
							printf("%d\n", byte);
						}
						eof_gp_debug_log(inf, "\t\tNew chorus value:  ");
						byte = pack_getc(inf);
						if(byte == -1)
						{
							(void) puts("(no change)");
						}
						else
						{
							chorus_change = 1;
							printf("%d\n", byte);
						}
						eof_gp_debug_log(inf, "\t\tNew reverb value:  ");
						byte = pack_getc(inf);
						if(byte == -1)
						{
							(void) puts("(no change)");
						}
						else
						{
							reverb_change = 1;
							printf("%d\n", byte);
						}
						eof_gp_debug_log(inf, "\t\tNew phaser value:  ");
						byte = pack_getc(inf);
						if(byte == -1)
						{
							(void) puts("(no change)");
						}
						else
						{
							phaser_change = 1;
							printf("%d\n", byte);
						}
						eof_gp_debug_log(inf, "\t\tNew tremolo value:  ");
						byte = pack_getc(inf);
						if(byte == -1)
						{
							(void) puts("(no change)");
						}
						else
						{
							tremolo_change = 1;
							printf("%d\n", byte);
						}
						if(fileversion >= 500)
						{	//These fields are only in version 5.x files
							eof_gp_debug_log(inf, "\t\tTempo text string:  ");
							(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read the tempo text string
							(void) puts(buffer);
						}
						eof_gp_debug_log(inf, "\t\tNew tempo:  ");
						pack_ReadDWORDLE(inf, &dword);
						if((long)dword == -1)
						{
							(void) puts("(no change)");
						}
						else
						{
							tempo_change = 1;
							printf("%ld\n", (long)dword);
						}
						if(volume_change)
						{	//This field only exists if a new volume was defined
							eof_gp_debug_log(inf, "\t\tNew volume change transition:  ");
							byte = pack_getc(inf);
							printf("%d bars\n", byte);
						}
						if(pan_change)
						{	//This field only exists if a new pan value was defined
							eof_gp_debug_log(inf, "\t\tNew pan change transition:  ");
							byte = pack_getc(inf);
							printf("%d bars\n", byte);
						}
						if(chorus_change)
						{	//This field only exists if a new  chorus value was defined
							eof_gp_debug_log(inf, "\t\tNew chorus change transition:  ");
							byte = pack_getc(inf);
							printf("%d bars\n", byte);
						}
						if(reverb_change)
						{	//This field only exists if a new reverb value was defined
							eof_gp_debug_log(inf, "\t\tNew reverb change transition:  ");
							byte = pack_getc(inf);
							printf("%d bars\n", byte);
						}
						if(phaser_change)
						{	//This field only exists if a new phaser value was defined
							eof_gp_debug_log(inf, "\t\tNew phaser change transition:  ");
							byte = pack_getc(inf);
							printf("%d bars\n", byte);
						}
						if(tremolo_change)
						{	//This field only exists if a new tremolo value was defined
							eof_gp_debug_log(inf, "\t\tNew tremolo change transition:  ");
							byte = pack_getc(inf);
							printf("%d bars\n", byte);
						}
						if(tempo_change)
						{	//These fields only exists if a new tempo was defined
							eof_gp_debug_log(inf, "\t\tNew tempo change transition:  ");
							byte = pack_getc(inf);
							printf("%d bars\n", byte);
							if(fileversion > 500)
							{	//This field only exists in versions newer than 5.0 of the format
								eof_gp_debug_log(inf, "\t\tTempo text string hidden:  ");
								byte = pack_getc(inf);
								printf("%s\n", !byte ? "no" : "yes");
							}
						}
						if(fileversion >= 400)
						{	//This field is not in version 3.0 files, assume 4.x or higher
							eof_gp_debug_log(inf, "\t\tMix table change applied tracks bitmask:  ");
							byte = pack_getc(inf);
							printf("%u\n", (byte & 0xFF));
						}
						if(fileversion >= 500)
						{	//This unknown byte is only in version 5.x files
							eof_gp_debug_log(inf, "\t\t(skipping 1 byte of unknown data)\n");
							(void) pack_fseek(inf, 1);		//Unknown data
						}
						if(fileversion > 500)
						{	//These strings are only in versions newer than 5.0 of the format
							eof_gp_debug_log(inf, "\t\tEffect 2 string:  ");
							(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read the Effect 2 string
							(void) puts(buffer);
							eof_gp_debug_log(inf, "\t\tEffect 1 string:  ");
							(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read the Effect 1 string
							(void) puts(buffer);
						}
					}//Beat has mix table change
					eof_gp_debug_log(inf, "\tUsed strings bitmask:  ");
					usedstrings = pack_getc(inf);
					printf("%u\n", (usedstrings & 0xFF));
					for(ctr4 = 0, bitmask = 64; ctr4 < 7; ctr4++, bitmask>>=1)
					{	//For each of the 7 possible usable strings
						if(bitmask & usedstrings)
						{	//If this string is used
							printf("\t\t\tString %lu:\n", ctr4 + 1);
							eof_gp_debug_log(inf, "\t\tNote bitmask:  ");
							bytemask = pack_getc(inf);
							printf("%u\n", (bytemask & 0xFF));
							if(bytemask & 32)
							{	//Note type is defined
								eof_gp_debug_log(inf, "\t\tNote type:  ");
								byte = pack_getc(inf);
								printf("%s\n", (byte == 1) ? "normal" : ((byte == 2) ? "tie" : "dead"));
							}
							if((bytemask & 1) && (fileversion < 500))
							{	//Time independent duration (for versions of the format older than 5.x)
								eof_gp_debug_log(inf, "\t\tTime independent duration values:  ");
								byte = pack_getc(inf);
								printf("%u ", (byte & 0xFF));
								byte = pack_getc(inf);
								printf("%u\n", (byte & 0xFF));
							}
							if(bytemask & 16)
							{	//Note dynamic
								eof_gp_debug_log(inf, "\t\tNote dynamic:  ");
								word = pack_getc(inf) - 1;	//Get the dynamic value and remap its values from 0 to 7
								printf("%s\n", note_dynamics[word % 8]);
							}
							if(bytemask & 32)
							{	//Note type is defined
								eof_gp_debug_log(inf, "\t\tFret number:  ");
								byte = pack_getc(inf);
								printf("%u\n", (byte & 0xFF));
							}
							if(bytemask & 128)
							{	//Right/left hand fingering
								eof_gp_debug_log(inf, "\t\tLeft hand fingering:  ");
								byte = pack_getc(inf);
								printf("%d\n", byte);
								eof_gp_debug_log(inf, "\t\tRight hand fingering:  ");
								byte = pack_getc(inf);
								printf("%d\n", byte);
							}
							if((bytemask & 1) && (fileversion >= 500))
							{	//Time independent duration (for versions of the format 5.x or newer)
								eof_gp_debug_log(inf, "\t\t(skipping 8 bytes of unknown time independent duration data)\n");
								(void) pack_fseek(inf, 8);		//Unknown data
							}
							if(fileversion >= 500)
							{	//This padding isn't in version 3.x and 4.x files
								eof_gp_debug_log(inf, "\t\t(skipping 1 byte of unknown data)\n");
								(void) pack_fseek(inf, 1);		//Unknown data
							}
							if(bytemask & 8)
							{	//Note effects
								char byte1, byte2 = 0;
								eof_gp_debug_log(inf, "\t\tNote effect bitmask:  ");
								byte1 = pack_getc(inf);
								printf("%u\n", (byte1 & 0xFF));
								if(fileversion >= 400)
								{	//Version 4.0 and higher of the file format has a second note effect bitmask
									eof_gp_debug_log(inf, "\t\tNote effect 2 bitmask:  ");
									byte2 = pack_getc(inf);
									printf("%u\n", (byte2 & 0xFF));
								}
								if(byte1 & 1)
								{	//Bend
									if(eof_gp_parse_bend(inf))
									{	//If there was an error parsing the bend
										(void) puts("Error parsing bend");
										(void) pack_fclose(inf);
										free(strings);
										return NULL;
									}
								}
								if(byte1 & 2)
								{
									(void) puts("\t\t\t\t(Hammer on/pull off from current note)");
								}
								if(byte1 & 4)
								{
									(void) puts("\t\t\t\t(Slide from current note)");
								}
								if(byte1 & 8)
								{
									(void) puts("\t\t\t\t(Let ring)");
								}
								if(byte1 & 16)
								{	//Grace note
									(void) puts("\t\t\t\t(Grace note)");
									eof_gp_debug_log(inf, "\t\t\tGrace note fret number:  ");
									byte = pack_getc(inf);
									printf("%u\n", (byte & 0xFF));
									eof_gp_debug_log(inf, "\t\t\tGrace note dynamic:  ");
									word = (pack_getc(inf) - 1) % 8;	//Get the dynamic value and remap its values from 0 to 7
									printf("%s\n", note_dynamics[word]);
									if(fileversion >= 500)
									{	//If the file version is 5.x or higher (this byte verified not to be in 3.0 and 4.06 files)
										eof_gp_debug_log(inf, "\t\t\tGrace note transition type:  ");
										byte = pack_getc(inf);
										if(!byte)
										{
											(void) puts("none");
										}
										else if(byte == 1)
										{
											(void) puts("slide");
										}
										else if(byte == 2)
										{
											(void) puts("bend");
										}
										else if(byte == 3)
										{
											(void) puts("hammer");
										}
									}
									else
									{	//The purpose of this field in 4.x or older files is unknown
										eof_gp_debug_log(inf, "\t\t(skipping 1 byte of unknown data)\n");
										(void) pack_fseek(inf, 1);		//Unknown data
									}
									eof_gp_debug_log(inf, "\t\t\tGrace note duration:  ");
									byte = pack_getc(inf);
									printf("%u\n", (byte & 0xFF));
									if(fileversion >= 500)
									{	//If the file version is 5.x or higher (this byte verified not to be in 3.0 and 4.06 files)
										eof_gp_debug_log(inf, "\t\t\tGrace note position:  ");
										byte = pack_getc(inf);
										printf("%u\n", (byte & 0xFF));
										if(byte & 1)
										{
											(void) puts("\t\t\t\t\t(dead note)");
										}
										printf("\t\t\t\t\t%s\n", (byte & 2) ? "(on the beat)" : "(before the beat)");
									}
								}
								if(byte2 & 1)
								{
									(void) puts("\t\t\t\t(Note played staccato");
								}
								if(byte2 & 2)
								{
									(void) puts("\t\t\t\t(Palm mute");
								}
								if(byte2 & 4)
								{	//Tremolo picking
									eof_gp_debug_log(inf, "\t\t\tTremolo picking speed:  ");
									byte = pack_getc(inf);
									printf("%u\n", (byte & 0xFF));
								}
								if(byte2 & 8)
								{	//Slide
									eof_gp_debug_log(inf, "\t\t\tSlide type:  ");
									byte = pack_getc(inf);
									printf("%u\n", (byte & 0xFF));
								}
								if(byte2 & 16)
								{	//Harmonic
									eof_gp_debug_log(inf, "\t\t\tHarmonic type:  ");
									byte = pack_getc(inf);
									printf("%u\n", (byte & 0xFF));
									if(byte == 2)
									{	//Artificial harmonic
										(void) puts("\t\t\t\t\t(Artificial harmonic)");
										eof_gp_debug_log(inf, "\t\t\t\tHarmonic note:  ");
										byte = pack_getc(inf);	//Read harmonic note
										printf("%s", eof_note_names[(byte + 3) % 12]);
										byte = pack_getc(inf);	//Read sharp/flat status
										if(byte == -1)
										{
											(void) putchar('b');
										}
										else if(byte == 1)
										{
											(void) putchar('#');
										}
										byte = pack_getc(inf);	//Read octave status
										if(byte == 0)
										{
											printf(" loco");
										}
										else if(byte == 1)
										{
											printf(" 8va");
										}
										else if(byte == 2)
										{
											printf(" 15ma");
										}
										(void) putchar('\n');
									}
									else if(byte == 3)
									{	//Tapped harmonic
										(void) puts("\t\t\t\t\t(Tapped harmonic)");
										eof_gp_debug_log(inf, "\t\t\t\tRight hand fret:  ");
										byte = pack_getc(inf);
										printf("%u\n", (byte & 0xFF));
									}
								}
								if(byte2 & 32)
								{	//Trill
									eof_gp_debug_log(inf, "\t\t\tTrill with fret:  ");
									byte = pack_getc(inf);
									printf("%u\n", (byte & 0xFF));
									eof_gp_debug_log(inf, "\t\t\tTrill duration:  ");
									byte = pack_getc(inf);
									printf("%u\n", (byte & 0xFF));
								}
								if(byte2 & 64)
								{
									(void) puts("\t\t\t\t(Vibrato)");
								}
							}//Note effects
						}//If this string is used
					}//For each of the 7 possible usable strings
					if(fileversion >= 500)
					{	//Version 5.0 and higher of the file format stores a note transpose mask and unknown data here
						eof_gp_debug_log(inf, "\tTranspose bitmask:  ");
						pack_ReadWORDLE(inf, &word);
						printf("%u\n", word);
						if(word & 0x800)
						{	//If bit 11 of the transpose bitmask was set, there is an additional byte of unknown data
							eof_gp_debug_log(inf, "\t\t(skipping 1 byte of unknown transpose data)\n");
							(void) pack_fseek(inf, 1);	//Unknown data
						}
					}
				}//For each beat
			}//For each voice
			if(fileversion >= 500)
			{
				eof_gp_debug_log(inf, "\t(skipping 1 byte of unknown data)\n");
				(void) pack_fseek(inf, 1);		//Unknown data
			}
		}//For each track
	}//For each measure


	(void) pack_fclose(inf);
	free(strings);
	(void) puts("\nSuccess");
	return (EOF_SONG *)1;
}

int main(int argc, char *argv[])
{
	EOF_SONG *ptr = NULL;

	if(argc < 2)
	{
		(void) puts("Must pass the GP file as a parameter");
		return 1;
	}

	ptr = parse_gp(argv[1]);
	if(ptr == NULL)
	{	//Failed to reach end of parsing
		return 2;
	}

	return 0;	//Return success
}

#else

#define GP_IMPORT_DEBUG
#define TARGET_VOICE 0

struct eof_guitar_pro_struct *eof_load_gp(const char * fn, char *undo_made)
{
	char buffer[256], *buffer2, buffer3[256], byte, bytemask, usedstrings, *ptr;
	unsigned word, fileversion;
	unsigned long dword, ctr, ctr2, ctr3, ctr4, tracks, measures, *strings, beats;
	PACKFILE *inf = NULL, *inf2;	//The GPA import logic will open the file handle for the Guitar Pro file in inf if applicable
	struct eof_guitar_pro_struct *gp;
	struct eof_gp_measure *tsarray;	//Stores all time signatures defined in the file
	EOF_PRO_GUITAR_NOTE **np;	//Will store the last created note for each track (for handling tie notes)
	char *hopo;	//Will store the fret value of the previous note marked as HO/PO (in GP, if note #N is marked for this, note #N+1 is the one that is a HO or PO), otherwise -1, for each track
	char user_warned = 0;	//Used to track user warnings about the file being corrupt
	char string_warning = 0;	//Used to track a user warning about the string count for a track being higher than what EOF supports
	char (*nonshiftslide)[7];		//Used to track, per GP track, whether the previous note on each string was a non shift slide (suppress the note after the slide)
	unsigned long curbeat = 0;		//Tracks the current beat number for the current measure
	double gp_durations[] = {1.0, 0.5, 0.25, 0.125, 0.0625, 0.03125, 0.015625};	//The duration of each note type in terms of one whole note (whole note, half, 4th, 8th, 12th, 32nd, 64th)
	double note_duration;			//Tracks the note's duration as a percentage of the current measure
	double measure_position;		//Tracks the current position as a percentage within the current measure
	unsigned long flags;			//Tracks the flags for the current note
	char new_note;					//Tracks whether a new note is to be created
	char tie_note;					//Tracks whether a note is a tie note
	unsigned char finger[7];		//Store left (fretting hand) finger values for each string
	unsigned long count;
	unsigned long startpos, endpos;
	unsigned char curnum = 4, curden = 4;	//Stores the current time signature (4/4 assumed until one is explicitly defined)
	unsigned long totalbeats = 0;			//Count the total number of beats in the Guitar Pro file's transcription
	unsigned long beatctr = 0;
	unsigned char startfret, endfret, bitmask;
	char *rssectionname;
	unsigned char start_of_repeat, num_of_repeats;
	char import_ts = 0;		//Will be set to nonzero if user opts to import time signatures
	char note_is_short = 0;	//Will be set to nonzero if the note being parsed is shorter than a quarter note
	char parse_gpa = 0;		//Will be set to nonzero if the specified file is detected to be XML, in which case, the Go PlayAlong file will be parsed
	size_t maxlinelength;
	unsigned long linectr = 2, num_sync_points = 0;
	struct eof_gpa_sync_point *sync_points = NULL;
	char error = 0;

	eof_log("\tImporting Guitar Pro file", 1);
	eof_log("eof_load_gp() entered", 1);

	if(!fn)
	{
		return NULL;
	}


//First, parse the input file to see if it is a Go PlayAlong XML file
	//Allocate memory buffers large enough to hold any line in this file
	inf2 = pack_fopen(fn, "rt");
	if(!inf2)
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError loading:  Cannot open input GP file:  \"%s\"", strerror(errno));	//Get the Operating System's reason for the failure
		eof_log(eof_log_string, 1);
		return NULL;
	}
	maxlinelength = (size_t)FindLongestLineLength_ALLEGRO(fn, 0);
	if(!maxlinelength)
	{
		eof_log("\tError finding the largest line in the file.  Aborting", 1);
		(void) pack_fclose(inf2);
		return NULL;
	}
	buffer2 = (char *)malloc(maxlinelength);
	if(!buffer2)
	{
		eof_log("\tError allocating memory (1).  Aborting", 1);
		(void) pack_fclose(inf2);
		return NULL;
	}
	if(pack_fgets(buffer2, (int)maxlinelength, inf2))
	{	//If the line was read
		if(strcasestr_spec(buffer2, "<?xml"))
		{	//If the file is determined to be XML based
			if(eof_song->tags->tempo_map_locked)
			{	//If the user has locked the tempo map
				eof_clear_input();
				key[KEY_Y] = 0;
				key[KEY_N] = 0;
				if(alert(NULL, "The tempo map must be unlocked in order to import a Go PlayAlong file.  Continue?", NULL, "&Yes", "&No", 'y', 'n') != 1)
				{	//If the user does not opt to unlock the tempo map
					eof_log("\tUser cancellation.  Aborting", 1);
					(void) pack_fclose(inf2);
					return NULL;
				}
				eof_song->tags->tempo_map_locked = 0;	//Unlock the tempo map
			}
			parse_gpa = 1;
		}
	}


//Parse the XML file if applicable
	if(parse_gpa)
	{	//If the input file was determined to be an XML file
		eof_log("\tParsing Go PlayAlong file", 1);
		if(!pack_fgets(buffer2, (int)maxlinelength, inf2))
		{	//I/O error
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tGo PlayAlong import failed on line #%lu:  Unable to read from file:  \"%s\"", linectr, strerror(errno));
			eof_log(eof_log_string, 1);
			error = 1;
		}
		while(!error && !pack_feof(inf2))
		{	//Until there was an error reading from the file or end of file is reached
			ptr = strcasestr_spec(buffer2, "<scoreUrl>");	//Find the beginning of the GP file name, if present on this line
			if(ptr)
			{	//If the GP file name was present, parse it into a buffer
				for(ctr = 0; ctr < 256; ctr++)
				{	//For each character buffer3[] can hold
					if((*ptr == '<') || (*ptr == '\0'))
					{	//If end of string or end of XML tag are reached
						break;
					}
					buffer3[ctr] = *ptr;
					ptr++;
				}
				buffer3[ctr] = '\0';	//Terminate the string
				(void) replace_filename(eof_temp_filename, fn, "", 1024);
				strncat(eof_temp_filename, buffer3, 1024);	//Build the path to the GP file
				inf = pack_fopen(eof_temp_filename, "rb");
				if(!inf)
				{
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tError loading:  Cannot open GP file (%s) specified in XML file:  \"%s\"", eof_temp_filename, strerror(errno));	//Get the Operating System's reason for the failure
					eof_log(eof_log_string, 1);
					error = 1;
				}
			}

			ptr = strcasestr_spec(buffer2, "<sync>");
			if(ptr)
			{	//If the sync tag is present
				//Read the number of sync entries
				for(ctr = 0; ctr < 20; ctr++)
				{	//Read up to 20 characters for this number
					if(*ptr == '#')
					{	//If the end of the number has been reached
						if(ctr < 1)
						{	//If no characters were read for this number
							eof_log("\t\tMalformed sync point.  Aborting", 1);
							error = 1;
							break;
						}
						break;	//Otherwise stop parsing this number
					}
					else if(isdigit(*ptr))
					{	//If this character is a valid number
						buffer3[ctr] = *ptr;	//Append it to the buffer
						ptr++;
					}
					else
					{	//Malformed sync tag
						eof_log("\t\tMalformed sync point.  Aborting", 1);
						error = 1;
						break;
					}
				}//Read up to 20 characters for this number
				if(ctr >= 20)
				{	//If the number was too long to parse
					eof_log("\t\tMalformed sync point.  Aborting", 1);
					error = 1;
					break;
				}
				buffer3[ctr] = '\0';	//Terminate the string
				num_sync_points = atol(buffer3);	//Convert to integer value

				//Allocate array for sync points
				if(num_sync_points)
				{	//If there is a valid number of sync entries
					sync_points = malloc(sizeof(struct eof_gpa_sync_point) * num_sync_points);
					if(!sync_points)
					{
						eof_log("\t\tError allocating memory (2).  Aborting", 1);
						error = 1;
					}
				}

				//Store sync points into array
				for(ctr = 0; ctr < num_sync_points; ctr++)
				{	//For each expected sync point
					if(!eof_get_next_gpa_sync_point(&ptr, &sync_points[ctr]))
					{	//If the sync point was not read
						eof_log("\t\tError parsing sync tag.  Aborting", 1);
						error = 1;
						break;
					}
					if(ctr && ((sync_points[ctr].measure + sync_points[ctr].pos_in_measure <= sync_points[ctr - 1].measure + sync_points[ctr - 1].pos_in_measure) || (sync_points[ctr].realtime_pos <= sync_points[ctr - 1].realtime_pos)))
					{	//If there was a previous sync point, and this sync point isn't a later timestamp or measure position
						eof_log("\t\tSync points out of order.  Aborting", 1);
						error = 1;
						break;
					}
				}
			}//If the sync tag is present

			(void) pack_fgets(buffer2, (int)maxlinelength, inf2);	//Read next line of text
			linectr++;	//Increment line counter
		}//Until there was an error reading from the file or end of file is reached
		if(error)
		{
			if(inf)
				pack_fclose(inf);
			pack_fclose(inf2);
			if(sync_points)
				free(sync_points);
			free(buffer2);
			return NULL;
		}
	}
	pack_fclose(inf2);
	if(buffer2)
		free(buffer2);


//Initialize pointers and handles
	if(!inf)
	{	//If the input GP file wasn't opened for reading by the GPA parse logic earlier
		inf = pack_fopen(fn, "rb");
	}
	if(!inf)
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError loading:  Cannot open input GP file:  \"%s\"", strerror(errno));	//Get the Operating System's reason for the failure
		eof_log(eof_log_string, 1);
		if(sync_points)
			free(sync_points);
		return NULL;
	}
	gp = malloc(sizeof(struct eof_guitar_pro_struct));
	if(!gp)
	{
		eof_log("Error allocating memory (3)", 1);
		(void) pack_fclose(inf);
		if(sync_points)
			free(sync_points);
		return NULL;
	}


//Read file version string
	(void) eof_read_gp_string(inf, &word, buffer, 0);	//Read file version string
	(void) pack_fseek(inf, 30 - word);			//Skip the padding that follows the version string
	if(!strcmp(buffer, "FICHIER GUITARE PRO v1.01"))
	{
		fileversion = 101;
	}
	else if(!strcmp(buffer, "FICHIER GUITARE PRO v1.02"))
	{
		fileversion = 102;
	}
	else if(!strcmp(buffer, "FICHIER GUITARE PRO v1.03"))
	{
		fileversion = 103;
	}
	else if(!strcmp(buffer, "FICHIER GUITARE PRO v1.04"))
	{
		fileversion = 104;
	}
	else if(!strcmp(buffer, "FICHIER GUITAR PRO v2.20"))
	{
		fileversion = 220;
	}
	else if(!strcmp(buffer, "FICHIER GUITAR PRO v2.21"))
	{
		fileversion = 221;
	}
	else if(!strcmp(buffer, "FICHIER GUITAR PRO v3.00"))
	{
		fileversion = 300;
	}
	else if(!strcmp(buffer, "FICHIER GUITAR PRO v4.00"))
	{
		fileversion = 400;
	}
	else if(!strcmp(buffer, "FICHIER GUITAR PRO v4.06"))
	{
		fileversion = 406;
	}
	else if(!strcmp(buffer, "FICHIER GUITAR PRO L4.06"))
	{
		fileversion = 406;
	}
	else if(!strcmp(buffer, "FICHIER GUITAR PRO v5.00"))
	{
		fileversion = 500;
	}
	else if(!strcmp(buffer, "FICHIER GUITAR PRO v5.10"))
	{
		fileversion = 510;
	}
	else
	{
		allegro_message("File format version not supported");
		eof_log("File format version not supported", 1);
		(void) pack_fclose(inf);
		free(gp);
		if(sync_points)
			free(sync_points);
		return NULL;
	}
#ifdef GP_IMPORT_DEBUG
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tParsing version %u guitar pro file", fileversion);
	eof_log(eof_log_string, 1);
#endif


//Read past various ignored information
	(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read title string
	(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read subtitle string
	(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read artist string
	(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read album string
	if(fileversion >= 500)
	{	//The words field only exists in version 5.x or higher versions of the format
		(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read words string
	}
	(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read music string
	(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read copyright string
	(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read tab string
	(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read instructions string
	pack_ReadDWORDLE(inf, &dword);						//Read the number of notice entries
	while(dword > 0)
	{	//Read each notice entry
		(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read notice string
		dword--;
	}
	if(fileversion < 500)
	{	//The shuffle rhythm feel field only exists here in version 4.x or older of the format
		(void) pack_getc(inf);
	}
	if(fileversion >= 400)
	{	//The lyrics fields only exist in version 4.x or newer of the format
		pack_ReadDWORDLE(inf, NULL);	//Read the lyrics' associated track number
		for(ctr = 0; ctr < 5; ctr++)
		{
			pack_ReadDWORDLE(inf, &dword);	//Read the start from bar #
			pack_ReadDWORDLE(inf, &dword);	//Read the lyric string length
			buffer2 = malloc((size_t)dword + 1);	//Allocate a buffer large enough for the lyric string (plus a NULL terminator)
			if(!buffer2)
			{
				eof_log("Error allocating memory (4)", 1);
				(void) pack_fclose(inf);
				free(gp);
				if(sync_points)
					free(sync_points);
				return NULL;
			}
			(void) pack_fread(buffer2, dword, inf);	//Read the lyric string
			buffer2[dword] = '\0';				//Terminate the string
			free(buffer2);						//Free the buffer
		}
	}
	if(fileversion > 500)
	{	//The volume/equalization settings only exist in versions newer than 5.0 of the format
		pack_ReadDWORDLE(inf, &dword);	//Read the master volume
		(void) pack_fseek(inf, 4);			//Unknown data
		(void) pack_getc(inf);		//32Hz band lowered
		(void) pack_getc(inf);		//60Hz band lowered
		(void) pack_getc(inf);		//125Hz band lowered
		(void) pack_getc(inf);		//250Hz band lowered
		(void) pack_getc(inf);		//500Hz band lowered
		(void) pack_getc(inf);		//1KHz band lowered
		(void) pack_getc(inf);		//2KHz band lowered
		(void) pack_getc(inf);		//4KHz band lowered
		(void) pack_getc(inf);		//8KHz band lowered
		(void) pack_getc(inf);		//16KHz band lowered
		(void) pack_getc(inf);		//Gain lowered
	}
	if(fileversion >= 500)
	{	//The page setup settings only exist in version 5.x or newer of the format
		pack_ReadDWORDLE(inf, &dword);	//Read the page format length
		pack_ReadDWORDLE(inf, &dword);	//Read the page format width
		pack_ReadDWORDLE(inf, &dword);	//Read the left margin
		pack_ReadDWORDLE(inf, &dword);	//Read the right margin
		pack_ReadDWORDLE(inf, &dword);	//Read the top margin
		pack_ReadDWORDLE(inf, &dword);	//Read the bottom margin
		pack_ReadDWORDLE(inf, &dword);	//Read the score size
		pack_ReadWORDLE(inf, &word);	//Read the enabled header/footer fields bitmask
		(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read the title header/footer string
		(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read the subtitle header/footer string
		(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read the artist header/footer string
		(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read the album header/footer string
		(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read the words (lyricist) header/footer string
		(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read the music (composer) header/footer string
		(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read the words & music header/footer string
		(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read the Copyright line 1 header/footer string
		(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read the Copyright line 2 header/footer string
		(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read the page number header/footer string
	}
	if(fileversion >= 500)
	{	//The tempo string only exists in version 5.x or newer of the format
		(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read the tempo string
	}
	pack_ReadDWORDLE(inf, &dword);	//Read the tempo
	if(fileversion > 500)
	{	//There is a byte of unknown data/padding here in versions newer than 5.0 of the format
		(void) pack_fseek(inf, 1);		//Unknown data
	}
	if(fileversion >= 400)
	{	//Versions 4.0 and newer of the format store key and octave information here
		(void) pack_getc(inf);			//Read the key
		(void) pack_fseek(inf, 3);		//Unknown data
		(void) pack_getc(inf);
	}
	else
	{	//Older versions stored only key information here
		pack_ReadDWORDLE(inf, &dword);	//Read the key
	}
	for(ctr = 0; ctr < 64; ctr++)
	{
		pack_ReadDWORDLE(inf, NULL);	//Read the instrument patch number
		(void) pack_getc(inf);			//Read the volume
		(void) pack_getc(inf);			//Read the pan value
		(void) pack_getc(inf);			//Read the chorus value
		(void) pack_getc(inf);			//Read the reverb value
		(void) pack_getc(inf);			//Read the phaser value
		(void) pack_getc(inf);			//Read the tremolo value
		pack_ReadWORDLE(inf, NULL);		//Read two bytes of unknown data/padding
	}
	if(fileversion >= 500)
	{	//Versions 5.0 and newer of the format store musical directional symbols and a master reverb setting here
		pack_ReadWORDLE(inf, &word);	//Coda symbol position
		gp->symbols[EOF_CODA_SYMBOL] = word - 1;	//Store the measure number this symbol appears on
#ifdef GP_IMPORT_DEBUG
		if(word != 0xFFFF)
		{
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\"Coda\" symbol is at measure #%u", word);
			eof_log(eof_log_string, 1);
		}
#endif
		pack_ReadWORDLE(inf, &word);	//Double coda symbol position
		gp->symbols[EOF_DOUBLE_CODA_SYMBOL] = word - 1;	//Store the measure number this symbol appears on
#ifdef GP_IMPORT_DEBUG
		if(word != 0xFFFF)
		{
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\"Double Coda\" symbol is at measure #%u", word);
			eof_log(eof_log_string, 1);
		}
#endif
		pack_ReadWORDLE(inf, &word);	//Segno symbol position
		gp->symbols[EOF_SEGNO_SYMBOL] = word - 1;	//Store the measure number this symbol appears on
#ifdef GP_IMPORT_DEBUG
		if(word != 0xFFFF)
		{
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\"Segno\" symbol is at measure #%u", word);
			eof_log(eof_log_string, 1);
		}
#endif
		pack_ReadWORDLE(inf, &word);	//Segno segno symbol position
		gp->symbols[EOF_SEGNO_SEGNO_SYMBOL] = word - 1;	//Store the measure number this symbol appears on
#ifdef GP_IMPORT_DEBUG
		if(word != 0xFFFF)
		{
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\"Segno Segno\" symbol is at measure #%u", word);
			eof_log(eof_log_string, 1);
		}
#endif
		pack_ReadWORDLE(inf, &word);	//Fine symbol position
		gp->symbols[EOF_FINE_SYMBOL] = word - 1;	//Store the measure number this symbol appears on
#ifdef GP_IMPORT_DEBUG
		if(word != 0xFFFF)
		{
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\"Fine\" symbol is at measure #%u", word);
			eof_log(eof_log_string, 1);
		}
#endif
		pack_ReadWORDLE(inf, &word);	//Da capo symbol position
		gp->symbols[EOF_DA_CAPO_SYMBOL] = word - 1;	//Store the measure number this symbol appears on
#ifdef GP_IMPORT_DEBUG
		if(word != 0xFFFF)
		{
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\"Da Capo\" symbol is at measure #%u", word);
			eof_log(eof_log_string, 1);
		}
#endif
		pack_ReadWORDLE(inf, &word);	//Da capo al coda symbol position
		gp->symbols[EOF_DA_CAPO_AL_CODA_SYMBOL] = word - 1;	//Store the measure number this symbol appears on
#ifdef GP_IMPORT_DEBUG
		if(word != 0xFFFF)
		{
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\"Da Capo al Coda\" symbol is at measure #%u", word);
			eof_log(eof_log_string, 1);
		}
#endif
		pack_ReadWORDLE(inf, &word);	//Da capo al double coda symbol position
		gp->symbols[EOF_DA_CAPO_AL_DOUBLE_CODA_SYMBOL] = word - 1;	//Store the measure number this symbol appears on
#ifdef GP_IMPORT_DEBUG
		if(word != 0xFFFF)
		{
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\"Da Capo al double Coda\" symbol is at measure #%u", word);
			eof_log(eof_log_string, 1);
		}
#endif
		pack_ReadWORDLE(inf, &word);	//Da capo al fine symbol position
		gp->symbols[EOF_DA_CAPO_AL_FINE_SYMBOL] = word - 1;	//Store the measure number this symbol appears on
#ifdef GP_IMPORT_DEBUG
		if(word != 0xFFFF)
		{
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\"Da Capo al Fine\" symbol is at measure #%u", word);
			eof_log(eof_log_string, 1);
		}
#endif
		pack_ReadWORDLE(inf, &word);	//Da segno symbol position
		gp->symbols[EOF_DA_SEGNO_SYMBOL] = word - 1;	//Store the measure number this symbol appears on
#ifdef GP_IMPORT_DEBUG
		if(word != 0xFFFF)
		{
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\"Da Segno\" symbol is at measure #%u", word);
			eof_log(eof_log_string, 1);
		}
#endif
		pack_ReadWORDLE(inf, &word);	//Da segno al coda symbol position
		gp->symbols[EOF_DA_SEGNO_AL_CODA_SYMBOL] = word - 1;	//Store the measure number this symbol appears on
#ifdef GP_IMPORT_DEBUG
		if(word != 0xFFFF)
		{
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\"Da Segno al Coda\" symbol is at measure #%u", word);
			eof_log(eof_log_string, 1);
		}
#endif
		pack_ReadWORDLE(inf, &word);	//Da segno al double coda symbol position
		gp->symbols[EOF_DA_SEGNO_AL_DOUBLE_CODA_SYMBOL] = word - 1;	//Store the measure number this symbol appears on
#ifdef GP_IMPORT_DEBUG
		if(word != 0xFFFF)
		{
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\"Da Segno al double Coda\" symbol is at measure #%u", word);
			eof_log(eof_log_string, 1);
		}
#endif
		pack_ReadWORDLE(inf, &word);	//Da segno al fine symbol position
		gp->symbols[EOF_DA_SEGNO_AL_FINE_SYMBOL] = word - 1;	//Store the measure number this symbol appears on
#ifdef GP_IMPORT_DEBUG
		if(word != 0xFFFF)
		{
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\"Da Segno al Fine\" symbol is at measure #%u", word);
			eof_log(eof_log_string, 1);
		}
#endif
		pack_ReadWORDLE(inf, &word);	//Da segno segno symbol position
		gp->symbols[EOF_DA_SEGNO_SEGNO_SYMBOL] = word - 1;	//Store the measure number this symbol appears on
#ifdef GP_IMPORT_DEBUG
		if(word != 0xFFFF)
		{
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\"Da Segno Segno\" symbol is at measure #%u", word);
			eof_log(eof_log_string, 1);
		}
#endif
		pack_ReadWORDLE(inf, &word);	//Da segno segno al coda symbol position
		gp->symbols[EOF_DA_SEGNO_SEGNO_AL_CODA_SYMBOL] = word - 1;	//Store the measure number this symbol appears on
#ifdef GP_IMPORT_DEBUG
		if(word != 0xFFFF)
		{
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\"Da Segno Segno al Coda\" symbol is at measure #%u", word);
			eof_log(eof_log_string, 1);
		}
#endif
		pack_ReadWORDLE(inf, &word);	//Da segno segno al double coda symbol position
		gp->symbols[EOF_DA_SEGNO_SEGNO_AL_DOUBLE_CODA_SYMBOL] = word - 1;	//Store the measure number this symbol appears on
#ifdef GP_IMPORT_DEBUG
		if(word != 0xFFFF)
		{
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\"Da Segno Segno al double Coda\" symbol is at measure #%u", word);
			eof_log(eof_log_string, 1);
		}
#endif
		pack_ReadWORDLE(inf, &word);	//Da segno segno al fine symbol position
		gp->symbols[EOF_DA_SEGNO_SEGNO_AL_FINE_SYMBOL] = word - 1;	//Store the measure number this symbol appears on
#ifdef GP_IMPORT_DEBUG
		if(word != 0xFFFF)
		{
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\"Da Segno Segno al Fine\" symbol is at measure #%u", word);
			eof_log(eof_log_string, 1);
		}
#endif
		pack_ReadWORDLE(inf, &word);	//Da coda symbol position
		gp->symbols[EOF_DA_CODA_SYMBOL] = word - 1;	//Store the measure number this symbol appears on
#ifdef GP_IMPORT_DEBUG
		if(word != 0xFFFF)
		{
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\"Da Coda\" symbol is at measure #%u", word);
			eof_log(eof_log_string, 1);
		}
#endif
		pack_ReadWORDLE(inf, &word);	//Da double coda symbol position
		gp->symbols[EOF_DA_DOUBLE_CODA_SYMBOL] = word - 1;	//Store the measure number this symbol appears on
#ifdef GP_IMPORT_DEBUG
		if(word != 0xFFFF)
		{
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\"Da double Coda\" symbol is at measure #%u", word);
			eof_log(eof_log_string, 1);
		}
#endif
		pack_ReadDWORDLE(inf, &dword);	//Read the master reverb value

		//Validate symbols
		if(gp->symbols[EOF_CODA_SYMBOL] == 0xFFFF)
		{	//If no Coda symbol was placed
			gp->symbols[EOF_DA_CAPO_AL_CODA_SYMBOL] = gp->symbols[EOF_DA_SEGNO_AL_CODA_SYMBOL] = gp->symbols[EOF_DA_SEGNO_SEGNO_AL_CODA_SYMBOL] = gp->symbols[EOF_DA_CODA_SYMBOL] = 0xFFFF;	//These symbols aren't valid
		}
		if(gp->symbols[EOF_DOUBLE_CODA_SYMBOL] == 0xFFFF)
		{	//If no Double Coda symbol was placed
			gp->symbols[EOF_DA_CAPO_AL_DOUBLE_CODA_SYMBOL] = gp->symbols[EOF_DA_SEGNO_AL_DOUBLE_CODA_SYMBOL] = gp->symbols[EOF_DA_SEGNO_SEGNO_AL_DOUBLE_CODA_SYMBOL] = gp->symbols[EOF_DA_DOUBLE_CODA_SYMBOL] = 0xFFFF;	//These symbols aren't valid
		}
		if(gp->symbols[EOF_SEGNO_SYMBOL] == 0xFFFF)
		{	//If no Segno symbol was placed
			gp->symbols[EOF_DA_SEGNO_SYMBOL] = gp->symbols[EOF_DA_SEGNO_AL_CODA_SYMBOL] = gp->symbols[EOF_DA_SEGNO_AL_DOUBLE_CODA_SYMBOL] = gp->symbols[EOF_DA_SEGNO_AL_FINE_SYMBOL] = 0xFFFF;	//These symbols aren't valid
		}
		if(gp->symbols[EOF_SEGNO_SEGNO_SYMBOL] == 0xFFFF)
		{	//If no Segno Segno symbol was placed
			gp->symbols[EOF_DA_SEGNO_SEGNO_SYMBOL] = gp->symbols[EOF_DA_SEGNO_SEGNO_AL_CODA_SYMBOL] = gp->symbols[EOF_DA_SEGNO_SEGNO_AL_DOUBLE_CODA_SYMBOL] = gp->symbols[EOF_DA_SEGNO_SEGNO_AL_FINE_SYMBOL] = 0xFFFF;	//These symbols aren't valid
		}
		if(gp->symbols[EOF_FINE_SYMBOL] == 0xFFFF)
		{	//If no Fine symbol was placed
			gp->symbols[EOF_DA_CAPO_AL_FINE_SYMBOL] = gp->symbols[EOF_DA_SEGNO_AL_FINE_SYMBOL] = gp->symbols[EOF_DA_SEGNO_SEGNO_AL_FINE_SYMBOL] = 0xFFFF;	//These symbols aren't valid
		}
	}


//Read track data
	pack_ReadDWORDLE(inf, &measures);	//Read the number of measures
#ifdef GP_IMPORT_DEBUG
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tNumber of measures: %lu", measures);
	eof_log(eof_log_string, 1);
#endif
	tsarray = malloc(sizeof(struct eof_gp_measure) * measures);	//Allocate memory to store the time signature effective for each measure
	if(!tsarray)
	{
		eof_log("Error allocating memory (5)", 1);
		(void) pack_fclose(inf);
		free(gp);
		if(sync_points)
			free(sync_points);
		return NULL;
	}
	gp->measure = tsarray;		//Store this array into the Guitar Pro structure
	gp->measures = measures;	//Store the measure count as well
	pack_ReadDWORDLE(inf, &tracks);	//Read the number of tracks
#ifdef GP_IMPORT_DEBUG
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tNumber of tracks: %lu", tracks);
	eof_log(eof_log_string, 1);
#endif
	gp->numtracks = tracks;
	gp->names = malloc(sizeof(char *) * tracks);			//Allocate memory for track name strings
	np = malloc(sizeof(EOF_PRO_GUITAR_NOTE *) * tracks);	//Allocate memory for the array of last created notes
	hopo = malloc(sizeof(char) * tracks);					//Allocate memory for storing HOPO information
	nonshiftslide = malloc(7 * sizeof(char) * tracks);		//Allocate a 7 byte array for each track to store string slide information
	gp->capos = malloc(sizeof(unsigned long) * tracks);		//Allocate memory to store the capo position of each track
	if(!gp->names || !np || !hopo || !nonshiftslide || !gp->capos)
	{
		eof_log("Error allocating memory (6)", 1);
		(void) pack_fclose(inf);
		free(gp);
		free(tsarray);
		if(sync_points)
			free(sync_points);
		return NULL;
	}
	memset(np, 0, sizeof(EOF_PRO_GUITAR_NOTE *) * tracks);				//Set all last created note pointers to NULL
	memset(hopo, -1, sizeof(char) * tracks);							//Set all tracks to have no HOPO status
	memset(nonshiftslide, 0, sizeof(char) * 7 * tracks);				//Clear all string slide statuses
	memset(gp->capos, 0, sizeof(unsigned long) * tracks);				//Initialize all capo positions to 0
	gp->track = malloc(sizeof(EOF_PRO_GUITAR_TRACK *) * tracks);		//Allocate memory for pro guitar track pointers
	gp->text_events = 0;
	if(!gp->track )
	{
		eof_log("Error allocating memory (7)", 1);
		(void) pack_fclose(inf);
		free(gp->track);
		free(gp->names);
		free(np);
		free(hopo);
		free(nonshiftslide);
		free(gp->capos);
		free(gp);
		free(tsarray);
		if(sync_points)
			free(sync_points);
		return NULL;
	}
	for(ctr = 0; ctr < tracks; ctr++)
	{	//Initialize each pro guitar track
		gp->track[ctr] = malloc(sizeof(EOF_PRO_GUITAR_TRACK));
		if(!gp->track[ctr])
		{
			eof_log("Error allocating memory (8)", 1);
			(void) pack_fclose(inf);
			free(gp->names);
			free(gp->track[ctr]);
			while(ctr > 0)
			{	//Free all previously allocated track structures
				free(gp->track[ctr - 1]);
				ctr--;
			}
			free(gp->track);	//Free array of track pointers
			free(np);
			free(hopo);
			free(nonshiftslide);
			free(gp->capos);
			free(gp);
			free(tsarray);
			if(sync_points)
				free(sync_points);
			return NULL;
		}
		memset(gp->track[ctr], 0, sizeof(EOF_PRO_GUITAR_TRACK));	//Initialize memory block to 0 to avoid crashes when not explicitly setting counters that were newly added to the pro guitar structure
		gp->track[ctr]->numfrets = 22;
		gp->track[ctr]->parent = NULL;
	}


//Read measure data
	//Allocate memory for an array to track the number of strings for each track
	strings = malloc(sizeof(unsigned long) * tracks);
	if(!strings)
	{
		eof_log("Error allocating memory (9)", 1);
		(void) pack_fclose(inf);
		free(gp->names);
		for(ctr = 0; ctr < tracks; ctr++)
		{	//Free all previously allocated track structures
			free(gp->track[ctr]);
		}
		free(gp->track);
		free(np);
		free(hopo);
		free(nonshiftslide);
		free(gp->capos);
		free(gp);
		free(tsarray);
		if(sync_points)
			free(sync_points);
		return NULL;
	}
#ifdef GP_IMPORT_DEBUG
	eof_log("\tParsing measure data", 1);
#endif
	for(ctr = 0; ctr < measures; ctr++)
	{	//For each measure
		bytemask = pack_getc(inf);	//Read the measure bitmask
		start_of_repeat = num_of_repeats = 0;	//Reset these values
		if(fileversion < 300)
		{	//Versions of the format older than 3.0
			if(bytemask & 1)
			{	//Start of repeat
				start_of_repeat = 1;	//Track that this measure is the start of a repeat
			}
			if(bytemask & 2)
			{	//End of repeat
				num_of_repeats = pack_getc(inf);	//Read number of repeats
			}
			if(bytemask & 4)
			{	//Number of alternate ending
				(void) pack_getc(inf);	//Read alternate ending number
			}
		}
		else
		{	//Versions of the format 3.0 and newer
			if(bytemask & 1)
			{	//Time signature change (numerator)
				curnum = pack_getc(inf);
			}
			if(bytemask & 2)
			{	//Time signature change (denominator)
				curden = pack_getc(inf);
			}
#ifdef GP_IMPORT_DEBUG
			if(bytemask & 3)
			{	//If the TS numerator or denominator were changed
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tTS change at measure %lu:  %u/%u", ctr + 1, curnum, curden);
				eof_log(eof_log_string, 1);
			}
#endif
			if(bytemask & 4)
			{	//Start of repeat
				start_of_repeat = 1;	//Track that this measure is the start of a repeat
			}
			if(bytemask & 8)
			{	//End of repeat
				num_of_repeats = pack_getc(inf);	//Read number of repeats
				if(fileversion >= 500)
				{	//Version 5 of the format has slightly different counting for repeats
					num_of_repeats--;
				}
			}
			if(fileversion < 500)
			{	//Versions 3 and 4 define the alternate ending next, followed by the section definition
				if(bytemask & 16)
				{	//Number of alternate ending
					(void) pack_getc(inf);	//Read alternate ending number
				}
				if(bytemask & 32)
				{	//New section
					(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read section string
					if(gp->text_events < EOF_MAX_TEXT_EVENTS)
					{	//If the maximum number of text events hasn't already been defined
#ifdef GP_IMPORT_DEBUG
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tSection marker found at measure #%lu:  \"%s\"", ctr + 1, buffer);
						eof_log(eof_log_string, 1);
#endif
						gp->text_event[gp->text_events] = malloc(sizeof(EOF_TEXT_EVENT));
						if(!gp->text_event[gp->text_events])
						{
							eof_log("Error allocating memory (10)", 1);
							(void) pack_fclose(inf);
							free(gp->names);
							for(ctr = 0; ctr < tracks; ctr++)
							{	//Free all previously allocated track structures
								free(gp->track[ctr]);
							}
							for(ctr = 0; ctr < gp->text_events; ctr++)
							{	//Free all allocated text events
								free(gp->text_event[ctr]);
							}
							free(gp->track);
							free(np);
							free(hopo);
							free(nonshiftslide);
							free(gp->capos);
							free(gp);
							free(tsarray);
							if(sync_points)
								free(sync_points);
							return NULL;
						}
						gp->text_event[gp->text_events]->beat = ctr;	//For now, store the measure number, it will need to be converted to the beat number later
						gp->text_event[gp->text_events]->track = 0;
						rssectionname = eof_rs_section_text_valid(buffer);	//Determine whether this is a valid Rocksmith section name
						if(eof_gp_import_preference_1 || !rssectionname)
						{	//If the user preference is to import all section markers as RS phrases, or this section marker isn't validly named for a RS section anyway
							(void) ustrncpy(gp->text_event[gp->text_events]->text, buffer, 255);
							gp->text_event[gp->text_events]->flags = EOF_EVENT_FLAG_RS_PHRASE;	//Ensure this will be detected as a RS phrase
						}
						else
						{	//Otherwise this section marker is valid as a RS section, then import it with the section's native name
							(void) ustrncpy(gp->text_event[gp->text_events]->text, rssectionname, 255);
							gp->text_event[gp->text_events]->flags = EOF_EVENT_FLAG_RS_SECTION;	//Ensure this will be detected as a RS section
						}
						gp->text_event[gp->text_events]->is_temporary = 0;	//This will be used to track whether the measure number was converted to the proper beat number below
						gp->text_events++;
					}
					(void) pack_getc(inf);								//Read section string color (Red intensity)
					(void) pack_getc(inf);								//Read section string color (Green intensity)
					(void) pack_getc(inf);								//Read section string color (Blue intensity)
					(void) pack_getc(inf);								//Read unused value
				}//New section
			}
			else
			{	//Version 5 defines the section definition next, followed by the alternate ending
				if(bytemask & 32)
				{	//New section
					(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read section string
					if(gp->text_events < EOF_MAX_TEXT_EVENTS)
					{	//If the maximum number of text events hasn't already been defined
#ifdef GP_IMPORT_DEBUG
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tSection marker found at measure #%lu:  \"%s\"", ctr + 1, buffer);
						eof_log(eof_log_string, 1);
#endif
						gp->text_event[gp->text_events] = malloc(sizeof(EOF_TEXT_EVENT));
						if(!gp->text_event[gp->text_events])
						{
							eof_log("Error allocating memory (11)", 1);
							(void) pack_fclose(inf);
							free(gp->names);
							for(ctr = 0; ctr < tracks; ctr++)
							{	//Free all previously allocated track structures
								free(gp->track[ctr]);
							}
							for(ctr = 0; ctr < gp->text_events; ctr++)
							{	//Free all allocated text events
								free(gp->text_event[ctr]);
							}
							free(gp->track);
							free(np);
							free(hopo);
							free(nonshiftslide);
							free(gp->capos);
							free(gp);
							free(tsarray);
							if(sync_points)
								free(sync_points);
							return NULL;
						}
						gp->text_event[gp->text_events]->beat = ctr;	//For now, store the measure number, it will need to be converted to the beat number later
						gp->text_event[gp->text_events]->track = 0;
						rssectionname = eof_rs_section_text_valid(buffer);	//Determine whether this is a valid Rocksmith section name
						if(eof_gp_import_preference_1 || !rssectionname)
						{	//If the user preference is to import all section markers as RS phrases, or this section marker isn't validly named for a RS section anyway
							(void) ustrncpy(gp->text_event[gp->text_events]->text, buffer, 255);
							gp->text_event[gp->text_events]->flags = EOF_EVENT_FLAG_RS_PHRASE;	//Ensure this will be detected as a RS phrase
						}
						else
						{	//Otherwise this section marker is valid as a RS section, then import it with the section's native name
							(void) ustrncpy(gp->text_event[gp->text_events]->text, rssectionname, 255);
							gp->text_event[gp->text_events]->flags = EOF_EVENT_FLAG_RS_SECTION;	//Ensure this will be detected as a RS section
						}
						gp->text_event[gp->text_events]->is_temporary = 0;	//This will be used to track whether the measure number was converted to the proper beat number below
						gp->text_events++;
					}
					(void) pack_getc(inf);								//Read section string color (Red intensity)
					(void) pack_getc(inf);								//Read section string color (Green intensity)
					(void) pack_getc(inf);								//Read section string color (Blue intensity)
					(void) pack_getc(inf);								//Read unused value
				}
				if(bytemask & 16)
				{	//Number of alternate ending
					word = pack_getc(inf);	//Read alternate ending number
				}
			}
			if(bytemask & 64)
			{	//Key signature change
				(void) pack_getc(inf);	//Read the key
				(void) pack_getc(inf);	//Read the major/minor byte
			}
		}//Versions of the format 3.0 and newer
		if(bytemask & 128)
		{	//Double bar
		}
		if(fileversion >= 500)
		{	//Version 5 of the format defines additional things here
			if((bytemask & 1) || (bytemask & 2))
			{	//If either a new TS numerator or denominator was set, read the beam by eight notes values
				(void) pack_getc(inf);
				(void) pack_getc(inf);
				(void) pack_getc(inf);
				(void) pack_getc(inf);
			}
			if(!(bytemask & 16))
			{	//If a GP5 file doesn't define an alternate ending here, ignore a byte of padding
				(void) pack_getc(inf);		//Unknown data
			}
			(void) pack_getc(inf);		//Read triplet feel value
			(void) pack_getc(inf);		//Unknown data
		}
		tsarray[ctr].num = curnum;	//Store this measure's time signature for future reference
		tsarray[ctr].den = curden;
		tsarray[ctr].start_of_repeat = start_of_repeat;
		if(ctr == 0)
		{	//If this is the first measure
			tsarray[ctr].start_of_repeat = 1;	//It is a start of repeat by default
		}
		tsarray[ctr].num_of_repeats = num_of_repeats;
		totalbeats += curnum;		//Add the number of beats in this measure to the ongoing counter
	}//For each measure
	if(eof_song->beats < totalbeats + 2)
	{	//If there will be beats appended to the project to encompass the guitar pro file's tracks
#ifdef GP_IMPORT_DEBUG
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tTranscription uses %lu beats, but project only has %lu beats.  Appending %lu beats to project so that guitar pro transcriptions and 2 bytes of padding will fit.", totalbeats, eof_song->beats, totalbeats + 2 - eof_song->beats);
		eof_log(eof_log_string, 1);
#endif
		while(eof_song->beats < totalbeats + 2)
		{	//Until the loaded project has enough beats to contain the Guitar Pro transcriptions, and two extra to allow room for processing beat lengths later
			double beat_length;

			if(!eof_song_add_beat(eof_song))
			{
				eof_log("Error allocating memory (12)", 1);
				(void) pack_fclose(inf);
				free(gp->names);
				for(ctr = 0; ctr < tracks; ctr++)
				{	//Free all previously allocated track structures
					free(gp->track[ctr]);
				}
				for(ctr = 0; ctr < gp->text_events; ctr++)
				{	//Free all allocated text events
					free(gp->text_event[ctr]);
				}
				free(gp->track);
				free(np);
				free(hopo);
				free(nonshiftslide);
				free(gp->capos);
				free(gp);
				free(tsarray);
				free(strings);
				if(sync_points)
					free(sync_points);
				return NULL;
			}
			eof_song->beat[eof_song->beats - 1]->ppqn = eof_song->beat[eof_song->beats - 2]->ppqn;	//Match the tempo of the previously last beat
			beat_length = (double)60000.0 / ((double)60000000.0 / (double)eof_song->beat[eof_song->beats - 2]->ppqn);	//Get the length of the previously last beat
			eof_song->beat[eof_song->beats - 1]->fpos = eof_song->beat[eof_song->beats - 2]->fpos + beat_length;
			eof_song->beat[eof_song->beats - 1]->pos = eof_song->beat[eof_song->beats - 1]->fpos + 0.5;	//Round up
		}
		eof_chart_length = eof_song->beat[eof_song->beats - 1]->pos;	//Alter the chart length so that the full transcription will display
	}

	eof_clear_input();
	if(!sync_points)
	{	//Skip prompting to import time signatures if importing a Go PlayAlong file
		if(eof_use_ts && !eof_song->tags->tempo_map_locked)
		{	//If user has enabled the preference to import time signatures, and the project's tempo map isn't locked (skip the prompt if importing a Go PlayAlong file)
			eof_clear_input();
			key[KEY_Y] = 0;
			key[KEY_N] = 0;
			if(alert(NULL, "Import Guitar Pro file's time signatures?", NULL, "&Yes", "&No", 'y', 'n') == 1)
			{	//If the user opts to import those from this Guitar Pro file into the active project
				import_ts = 1;
				if(undo_made && (*undo_made == 0))
				{	//If calling function wants to track an undo state being made if time signatures are imported into the project
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					*undo_made = 1;
				}
			}
		}
		else
		{	//TS change importing is being skipped
			allegro_message("To allow time signatures to be imported, ensure the \"Import/Export TS\" preference is enabled and the tempo map isn't locked");
		}
	}
	curnum = curden = 0;
	for(ctr = 0; ctr < measures; ctr++)
	{	//For each measure in the GP file
		for(ctr2 = 0; ctr2 < gp->text_events; ctr2++)
		{	//For each section marker that was imported
			if((gp->text_event[ctr2]->beat == ctr) && (!gp->text_event[ctr2]->is_temporary))
			{	//If the section marker was defined on this measure (and it hasn't been converted to use beat numbering yet
				gp->text_event[ctr2]->beat = beatctr;
				gp->text_event[ctr2]->is_temporary = 1;	//Track that this event has been converted to beat numbering
			}
		}
		for(ctr2 = 0; ctr2 < tsarray[ctr].num; ctr2++, beatctr++)
		{	//For each beat in this measure, count the beats
			if(import_ts || sync_points)
			{	//If the user opted to replace the active project's TS changes, or if a Go PlayAlong file is being imported
				if(!ctr2 && ((tsarray[ctr].num != curnum) || (tsarray[ctr].den != curden)))
				{	//If this is a time signature change on the first beat of the measure
					(void) eof_apply_ts(tsarray[ctr].num, tsarray[ctr].den, beatctr, eof_song, 0);	//Apply the change to the active project
				}
				else
				{	//Otherwise clear all beat flags except those that aren't TS related
					unsigned long flags = eof_song->beat[beatctr]->flags & (EOF_BEAT_FLAG_ANCHOR | EOF_BEAT_FLAG_EVENTS | EOF_BEAT_FLAG_KEY_SIG);	//Keep these flags as-is
					eof_song->beat[beatctr]->flags = flags;
				}
			}
		}
		curnum = tsarray[ctr].num;
		curden = tsarray[ctr].den;
	}


//Apply Go PlayAlong timings now if applicable
	if(sync_points)
	{	//If synchronization data was imported from the input Go PlayAlong file
		double curpos = 0.0, beat_length = 500;	//By default, assume 120BPM
		eof_process_beat_statistics(eof_song, eof_selected_track);	//Find the measure numbering for all beats
		for(ctr = 0; ctr < eof_song->beats; ctr++)
		{	//For each beat in the project
			measure_position = (double)eof_song->beat[ctr]->beat_within_measure / (double)eof_song->beat[ctr]->num_beats_in_measure;	//Find this beat's position in the measure, as a value between 0 and 1
			for(ctr2 = 0; ctr2 < num_sync_points; ctr2++)
			{	//For each sync point from the Go PlayAlong file
				if(sync_points[ctr2].measure + 1 == eof_song->beat[ctr]->measurenum)
				{	//If the sync point belongs in the same measure as this beat (GPA files number measures starting with 0)
					if(fabs(measure_position - sync_points[ctr2].pos_in_measure) < (1.0 / (double)eof_song->beat[ctr]->num_beats_in_measure) * 0.05)
					{	//If this sync point is close enough (within 5% of the measure's length) to this beat to be considered tied to the beat
						eof_song->beat[ctr]->fpos = eof_song->beat[ctr]->pos = sync_points[ctr2].realtime_pos;	//Apply the timestamp
						curpos = sync_points[ctr2].realtime_pos;			//Update the ongoing position variable
						beat_length = sync_points[ctr2].beat_length;		//Update the beat length variable
						break;	//Exit sync point loop
					}
				}
			}
			eof_song->beat[ctr]->fpos = curpos;							//Apply the current position to this beat
			eof_song->beat[ctr]->pos = eof_song->beat[ctr]->fpos + 0.5;	//Round up
			curpos += beat_length;
		}
		eof_song->tags->ogg[eof_selected_ogg].midi_offset = eof_song->beat[0]->pos;
		eof_calculate_tempo_map(eof_song);	//Update the tempo and anchor status of all beats
		free(sync_points);
	}//If synchronization data was imported from the input Go PlayAlong file


//Read track data
#ifdef GP_IMPORT_DEBUG
	eof_log("\tParsing track data", 1);
#endif
	for(ctr = 0; ctr < tracks; ctr++)
	{	//For each track
		bytemask = pack_getc(inf);	//Read the track bitmask
		if(bytemask & 1)
		{	//Is a drum track
		}
		if(bytemask & 2)
		{	//Is a 12 string guitar track
		}
		if(bytemask & 4)
		{	//Is a banjo track
		}
		if(bytemask & 16)
		{	//Is marked for solo playback
		}
		if(bytemask & 32)
		{	//Is marked for muted playback
		}
		if(bytemask & 64)
		{	//Is marked for RSE playback
		}
		if(bytemask & 128)
		{	//Is set to have the tuning displayed
		}

		(void) eof_read_gp_string(inf, &word, buffer, 0);		//Read track name string
		gp->names[ctr] = malloc(sizeof(buffer) + 1);	//Allocate memory to store track name string into guitar pro structure
		if(!gp->names[ctr])
		{
			eof_log("Error allocating memory (13)", 1);
			(void) pack_fclose(inf);
			while(ctr > 0)
			{	//Free the previous track name strings
				free(gp->names[ctr - 1]);
				ctr--;
			}
			free(gp->names);
			for(ctr = 0; ctr < tracks; ctr++)
			{	//Free all previously allocated track structures
				free(gp->track[ctr]);
			}
			for(ctr = 0; ctr < gp->text_events; ctr++)
			{	//Free all allocated text events
				free(gp->text_event[ctr]);
			}
			free(gp->track);
			free(np);
			free(hopo);
			free(nonshiftslide);
			free(gp->capos);
			free(gp);
			free(tsarray);
			free(strings);
			return NULL;
		}
		strcpy(gp->names[ctr], buffer);
#ifdef GP_IMPORT_DEBUG
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tTrack #%lu: %s", ctr + 1, buffer);
		eof_log(eof_log_string, 1);
#endif
		(void) pack_fseek(inf, 40 - word);					//Skip the padding that follows the track name string
		pack_ReadDWORDLE(inf, &strings[ctr]);		//Read the number of strings in this track
		gp->track[ctr]->numstrings = strings[ctr];
		if(strings[ctr] > 6)
		{	//Warn that EOF will not import more than 6 strings
			gp->track[ctr]->numstrings = 6;
			if(!string_warning)
			{
				allegro_message("Warning:  At least one track uses more than 6 strings, string 7 will be dropped during import");
				string_warning = 1;
			}
		}
#ifdef GP_IMPORT_DEBUG
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t%lu strings", strings[ctr]);
		eof_log(eof_log_string, 1);
#endif
		for(ctr2 = 0; ctr2 < 7; ctr2++)
		{	//For each of the 7 possible usable strings
			if(ctr2 < strings[ctr])
			{	//If this string is used
				pack_ReadDWORDLE(inf, &dword);	//Read the tuning for this string
#ifdef GP_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tTuning for string #%lu: MIDI note %lu (%s)", ctr2 + 1, dword, eof_note_names[(dword + 3) % 12]);
				eof_log(eof_log_string, 1);
#endif
				if(ctr2 < 6)
				{	//Don't store the tuning for more than six strings, as EOF doesn't support 7
					gp->track[ctr]->tuning[gp->track[ctr]->numstrings - 1 - ctr2] = dword;	//Store the absolute MIDI note, this will have to be converted to the track and string count specific relative value once mapped to an EOF instrument track (Guitar Pro stores the tuning in string order starting from #1, reversed from EOF)
				}
			}
			else
			{
				pack_ReadDWORDLE(inf, NULL);	//Skip this padding
			}
		}
		pack_ReadDWORDLE(inf, &dword);	//Read the MIDI port used for this track
		pack_ReadDWORDLE(inf, &dword);	//Read the MIDI channel used for this track
		pack_ReadDWORDLE(inf, &dword);	//Read the MIDI channel used for this track's effects
		pack_ReadDWORDLE(inf, &dword);	//Read the number of frets used for this track
#ifdef GP_IMPORT_DEBUG
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tNumber of frets: %lu", dword);
		eof_log(eof_log_string, 1);
#endif
		gp->track[ctr]->numfrets = dword;
		pack_ReadDWORDLE(inf, &(gp->capos[ctr]));	//Read the capo position for this track
#ifdef GP_IMPORT_DEBUG
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tCapo position: %lu", gp->capos[ctr]);
		eof_log(eof_log_string, 1);
#endif
		(void) pack_getc(inf);						//Track color (Red intensity)
		(void) pack_getc(inf);						//Track color (Green intensity)
		(void) pack_getc(inf);						//Track color (Blue intensity)
		(void) pack_getc(inf);						//Read unused value
		if(fileversion > 500)
		{
			bytemask = pack_getc(inf);			//Track properties 1 bitmask
			(void) pack_getc(inf);				//Track properties 2 bitmask
			(void) pack_getc(inf);				//Unknown data
			(void) pack_getc(inf);				//MIDI bank
			(void) pack_getc(inf);				//Human playing
			(void) pack_getc(inf);				//Auto accentuation on the beat
			(void) pack_fseek(inf, 31);			//Unknown data
			(void) pack_getc(inf);				//Selected sound bank option
			(void) pack_fseek(inf, 7);			//Unknown data
			(void) pack_getc(inf);				//Low frequency band lowered
			(void) pack_getc(inf);				//Mid frequency band lowered
			(void) pack_getc(inf);				//High frequency band lowered
			(void) pack_getc(inf);				//Gain lowered
			(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read track instrument effect 1
			(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read track instrument effect 2
		}
		else if(fileversion == 500)
		{
			(void) pack_fseek(inf, 45);		//Unknown data
		}
	}//For each track
	if(fileversion >= 500)
	{
		(void) pack_getc(inf);	//Unknown data
	}


//Read note data
#ifdef GP_IMPORT_DEBUG
	eof_log("\tParsing note data", 1);
#endif

	for(ctr = 0; ctr < measures; ctr++)
	{	//For each measure
		curnum = tsarray[ctr].num;	//Obtain the time signature for this measure
		curden = tsarray[ctr].den;

#ifdef GP_IMPORT_DEBUG
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tMeasure #%lu", ctr + 1);
		eof_log(eof_log_string, 1);
#endif

		for(ctr2 = 0; ctr2 < tracks; ctr2++)
		{	//For each track
			unsigned voice, maxvoices = 1;
#ifdef GP_IMPORT_DEBUG
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tTrack #%lu", ctr2 + 1);
			eof_log(eof_log_string, 1);
#endif
			if(fileversion >= 500)
			{	//Version 5.0 and higher of the format stores two "voices" (lead and bass) for each track
				maxvoices = 2;
			}
			for(voice = 0; voice < maxvoices; voice++)
			{	//For each voice
				measure_position = 0.0;
				pack_ReadDWORDLE(inf, &beats);	//Read number of "beats" (which are more accurately considered notes)
				for(ctr3 = 0; ctr3 < beats; ctr3++)
				{	//For each "beat"
					char ghost = 0;	//Track the ghost status for notes
					unsigned bitmask;
					unsigned char frets[7];		//Store fret values for each string

					new_note = 0;	//Assume no new note is to be added unless a normal/muted note is parsed
					tie_note = 0;	//Assume a note isn't a tie note unless found otherwise
					flags = 0;
					memset(finger, 0, sizeof(finger));	//Clear the finger array
					bytemask = pack_getc(inf);	//Read beat bitmask
					if(bytemask & 64)
					{	//Beat is a rest
						(void) pack_getc(inf);	//Rest beat type (empty/rest)
						new_note = 0;
					}
					byte = pack_getc(inf);		//Read beat duration
					if((byte < -2) || (byte > 4))
					{	//If it's an invalid note length
						if(!user_warned)
						{
							allegro_message("Warning:  This file has an invalid note length of %d and is probably corrupt", byte);
						}
						user_warned = 1;
						byte = 4;
					}
					note_duration = gp_durations[byte + 2] * (double)curden / (double)curnum;	//Get this note's duration in measures (accounting for the time signature)
					if(bytemask & 32)
					{	//Beat is an N-tuplet
						pack_ReadDWORDLE(inf, &dword);	//Number of notes played within the "tuplet" (ie. 3 = triplet)
						switch(dword)
						{	//The length of each note in an N-tuplet depends on N
							case 6:
								note_duration /= 6.0 / 4.0;	//A sextuplet is 6 notes in the span of 4 of that note
							break;
							case 7:
								note_duration /= 7.0 / 4.0;	//A septuplet is 7 notes in the span of 4 of that note
							break;
							case 9:
								note_duration /= 9.0 / 8.0;	//A nonuplet is 9 notes in the span of 8 of that note
							break;
							case 10:
								note_duration /= 10.0 / 8.0;	//A decuplet is 10 notes in the span of 8 of that note
							break;
							case 11:
								note_duration /= 11.0 / 8.0;	//An undecuplet is 11 notes in the span of 8 of that note
							break;
							case 12:
								note_duration /= 12.0 / 8.0;	//A dodecuplet is 12 notes in the span of 8 of that note
							break;
							case 13:
								note_duration /= 13.0 / 8.0;	//A triskaidekatuplet(?) is 13 notes in the span of 8 of that note
							break;
							default:	//Otherwise assume the tuplet is n notes in the span of (n-1) of that note
								note_duration /= (double)dword / ((double)dword - 1.0);
							break;
						}
					}//(a triplet of quarter notes is 3 notes in the span of two quarter notes) (a quintuplet of eighth notes is 5 notes in the span of 4 eighth notes)
					if(bytemask & 1)
					{	//Dotted note
						note_duration *= 1.5;	//A dotted note is one and a half times as long as normal
					}
					if(note_duration / ((double)curden / (double)curnum) < 0.25)
					{	//If the note (after undoing the scaling for the time signature) is shorter than a quarter note
						note_is_short = 1;
					}
					else
					{
						note_is_short = 0;
					}

					if(bytemask & 2)
					{	//Beat has a chord diagram
						word = pack_getc(inf);	//Read chord diagram format
						if(word == 0)
						{	//Chord diagram format 0, ie. GP3
							(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read chord name
							pack_ReadDWORDLE(inf, &dword);				//Read the diagram fret position
							if(dword)
							{	//If the diagram is properly defined
								for(ctr4 = 0; ctr4 < strings[ctr2]; ctr4++)
								{	//For each string defined in the track
									pack_ReadDWORDLE(inf, &dword);			//Read the fret played on this string
								}
							}
						}//Chord diagram format 0, ie. GP3
						else if(word == 1)
						{	//Chord diagram format 1, ie. GP4
							(void) pack_getc(inf);		//Read sharp/flat indicator
							(void) pack_fseek(inf, 3);			//Unknown data
							(void) pack_getc(inf);		//Read chord root
							if(fileversion / 100 == 3)
							{	//If it is a GP 3.x file
								(void) pack_fseek(inf, 3);		//Unknown data
							}
							(void) pack_getc(inf);		//Read chord type
							if(fileversion / 100 == 3)
							{	//If it is a GP 3.x file
								(void) pack_fseek(inf, 3);		//Unknown data
							}
							(void) pack_getc(inf);		//9th/11th/13th option
							if(fileversion / 100 == 3)
							{	//If it is a GP 3.x file
								(void) pack_fseek(inf, 3);		//Unknown data
							}
							pack_ReadDWORDLE(inf, &dword);	//Read bass note (lowest note played in string)
							(void) pack_getc(inf);					//+/- option
							(void) pack_fseek(inf, 4);				//Unknown data
							word = pack_getc(inf);			//Read chord name string length
							(void) pack_fread(buffer, 20, inf);	//Read chord name (which is padded to 20 bytes)
							buffer[word] = '\0';			//Ensure string is terminated to be the right length
							(void) pack_fseek(inf, 2);				//Unknown data
							(void) pack_getc(inf);					//Tonality of the fifth
							if(fileversion / 100 == 3)
							{	//If it is a GP 3.x file
								(void) pack_fseek(inf, 3);			//Unknown data
							}
							(void) pack_getc(inf);					//Tonality of the ninth
							if(fileversion / 100 == 3)
							{	//If it is a GP 3.x file
								(void) pack_fseek(inf, 3);			//Unknown data
							}
							(void) pack_getc(inf);					//Tonality of the eleventh
							if(fileversion / 100 == 3)
							{	//If it is a GP 3.x file
								(void) pack_fseek(inf, 3);			//Unknown data
							}
							pack_ReadDWORDLE(inf, &dword);	//Base fret for diagram
							for(ctr4 = 0; ctr4 < 7; ctr4++)
							{	//For each of the 7 possible usable strings
								if(ctr4 < strings[ctr2])
								{	//If this string is used in the track
									pack_ReadDWORDLE(inf, &dword);	//Fret value played (-1 means unused in chord)
								}
								else
								{
									pack_ReadDWORDLE(inf, NULL);	//Skip this padding
								}
							}
							(void) pack_getc(inf);	//Read the number of barres in this chord
							for(ctr4 = 0; ctr4 < 5; ctr4++)
							{	//For each of the 5 possible barres
								(void) pack_getc(inf);	//Read the barre position/padding
							}
							for(ctr4 = 0; ctr4 < 5; ctr4++)
							{	//For each of the 5 possible barres
								(void) pack_getc(inf);	//Read the barre start string/padding
							}
							for(ctr4 = 0; ctr4 < 5; ctr4++)
							{	//For each of the 5 possible barres
								(void) pack_getc(inf);	//Read the barre stop string/padding
							}
							(void) pack_getc(inf);		//Chord includes first interval?
							(void) pack_getc(inf);		//Chord includes third interval?
							(void) pack_getc(inf);		//Chord includes fifth interval?
							(void) pack_getc(inf);		//Chord includes seventh interval?
							(void) pack_getc(inf);		//Chord includes ninth interval?
							(void) pack_getc(inf);		//Chord includes eleventh interval?
							(void) pack_getc(inf);		//Chord includes thirteenth interval?
							(void) pack_getc(inf);		//Unknown data
							for(ctr4 = 0; ctr4 < 7; ctr4++)
							{	//For each of the 7 possible usable strings
								if(ctr4 < strings[ctr2])
								{	//If this string is used in the track
									byte = pack_getc(inf);	//Finger # used to play string
									if(byte == 0)
									{	//Thumb, remap to EOF's definition of thumb = 5
										byte = 5;
									}
									if(byte > 0)
									{	//If the finger number is defined
										finger[ctr4] = byte;	//Store into the finger array
									}
								}
								else
								{
									(void) pack_getc(inf);		//Skip this padding
								}
							}
							(void) pack_getc(inf);	//Chord fingering displayed?
						}//Chord diagram format 1, ie. GP4
					}//Beat has a chord diagram
					if(bytemask & 4)
					{	//Beat has text
						(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read beat text string
						rssectionname = eof_rs_section_text_valid(buffer);	//Determine whether this is a valid Rocksmith section name
						if(gp->text_events < EOF_MAX_TEXT_EVENTS)
						{	//If the maximum number of text events hasn't already been defined
#ifdef GP_IMPORT_DEBUG
							(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tBeat text found at beat #%lu:  \"%s\"", curbeat, buffer);
							eof_log(eof_log_string, 1);
#endif
							gp->text_event[gp->text_events] = malloc(sizeof(EOF_TEXT_EVENT));
							if(!gp->text_event[gp->text_events])
							{
								eof_log("Error allocating memory (14)", 1);
								(void) pack_fclose(inf);
								free(gp->names);
								for(ctr = 0; ctr < tracks; ctr++)
								{	//Free all previously allocated track structures
									free(gp->track[ctr]);
								}
								for(ctr = 0; ctr < gp->text_events; ctr++)
								{	//Free all allocated text events
									free(gp->text_event[ctr]);
								}
								free(gp->track);
								free(np);
								free(hopo);
								free(nonshiftslide);
								free(gp->capos);
								free(gp);
								free(tsarray);
								return NULL;
							}
							gp->text_event[gp->text_events]->beat = curbeat;
							gp->text_event[gp->text_events]->track = 0;
							gp->text_event[gp->text_events]->is_temporary = 1;	//Track that the event's beat number has already been determined
							if(rssectionname)
							{	//If this beat text matches a valid Rocksmith section name, import it with the section's native name
								(void) ustrncpy(gp->text_event[gp->text_events]->text, rssectionname, 255);
								gp->text_event[gp->text_events]->flags = EOF_EVENT_FLAG_RS_SECTION;	//Ensure this will be detected as a RS section
								gp->text_events++;
							}
							else if(!eof_gp_import_preference_1)
							{	//If the user preference to discard beat text that doesn't match a RS section isn't enabled, import it as a RS phrase
								(void) ustrncpy(gp->text_event[gp->text_events]->text, buffer, 255);	//Copy the beat text as-is
								gp->text_event[gp->text_events]->flags = EOF_EVENT_FLAG_RS_PHRASE;	//Ensure this will be detected as a RS phrase
								gp->text_events++;
							}
							else
							{	//Otherwise discard this beat text
								free(gp->text_event[gp->text_events]);	//Free the memory allocated to store this text event
							}
						}
					}//Beat has text
					if(bytemask & 8)
					{	//Beat has effects
						unsigned char byte1, byte2 = 0;

						byte1 = pack_getc(inf);	//Read beat effects 1 bitmask
						if(fileversion >= 400)
						{	//Versions 4.0 and higher of the format have a second beat effects bitmask
							byte2 = pack_getc(inf);	//Extended beat effects bitmask
							if(byte2 & 1)
							{	//Rasguedo
							}
						}
						if(byte1 & 1)
						{	//Vibrato
							flags |= EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO;
						}
						if(byte1 & 2)
						{	//Wide vibrato
							flags |= EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO;
						}
						if(byte1 & 4)
						{	//Natural harmonic
							flags |= EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC;
						}
						if(byte1 & 8)
						{	//Artificial harmonic
							flags |= EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC;
						}
						if(byte1 & 32)
						{	//Tapping/popping/slapping
							byte = pack_getc(inf);	//Read tapping/popping/slapping indicator
							if(byte == 0)
							{	//Tremolo bar
							}
							else if(byte == 1)
							{	//Tapping
								flags |= EOF_PRO_GUITAR_NOTE_FLAG_TAP;
							}
							else if(byte == 2)
							{	//Slapping
								flags |= EOF_PRO_GUITAR_NOTE_FLAG_SLAP;
							}
							else if(byte == 3)
							{	//Popping
								flags |= EOF_PRO_GUITAR_NOTE_FLAG_POP;
							}
							if(fileversion < 400)
							{
								pack_ReadDWORDLE(inf, &dword);	//String effect value
							}
						}
						if(byte2 & 4)
						{	//Tremolo bar
							if(eof_gp_parse_bend(inf))
							{	//If there was an error parsing the bend
								allegro_message("Error parsing bend, file is corrupt");
								(void) pack_fclose(inf);
								while(ctr > 0)
								{	//Free the previous track name strings
									free(gp->names[ctr - 1]);
									ctr--;
								}
								free(gp->names);
								for(ctr = 0; ctr < tracks; ctr++)
								{	//Free all previously allocated track structures
									free(gp->track[ctr]);
								}
								for(ctr = 0; ctr < gp->text_events; ctr++)
								{	//Free all allocated text events
									free(gp->text_event[ctr]);
								}
								free(gp->track);
								free(np);
								free(hopo);
								free(nonshiftslide);
								free(gp->capos);
								free(gp);
								free(tsarray);
								free(strings);
								return NULL;
							}
						}
						if(byte1 & 64)
						{	//Stroke (strum) effect
							unsigned char upspeed;
							upspeed = pack_getc(inf);	//Up strum speed
							(void) pack_getc(inf);				//Down strum speed
							if(!upspeed)
							{	//Strum down
								flags |= EOF_PRO_GUITAR_NOTE_FLAG_DOWN_STRUM;
							}
							else
							{	//Strum up
								flags |= EOF_PRO_GUITAR_NOTE_FLAG_UP_STRUM;
							}
						}
						if(byte2 & 2)
						{	//Pickstroke effect
							byte = pack_getc(inf);	//Pickstroke effect (up/down)
							if(byte == 1)
							{	//Upward pick
								flags |= EOF_PRO_GUITAR_NOTE_FLAG_UP_STRUM;
							}
							else if(byte == 2)
							{	//Downward pick
								flags |= EOF_PRO_GUITAR_NOTE_FLAG_DOWN_STRUM;
							}
						}
					}//Beat has effects
					if(bytemask & 16)
					{	//Beat has mix table change
						char volume_change = 0, pan_change = 0, chorus_change = 0, reverb_change = 0, phaser_change = 0, tremolo_change = 0, tempo_change = 0;

						(void) pack_getc(inf);	//New instrument number
						if(fileversion >= 500)
						{	//These fields are only in version 5.x files
							pack_ReadDWORDLE(inf, &dword);	//RSE related number
							pack_ReadDWORDLE(inf, &dword);	//RSE related number
							pack_ReadDWORDLE(inf, &dword);	//RSE related number
							(void) pack_fseek(inf, 4);				//Unknown data
						}
						byte = pack_getc(inf);	//New volume
						if(byte == -1)
						{	//No change
						}
						else
						{
							volume_change = 1;
						}
						byte = pack_getc(inf);	//New pan value
						if(byte == -1)
						{	//No change
						}
						else
						{
							pan_change = 1;
						}
						byte = pack_getc(inf);	//New chorus value
						if(byte == -1)
						{	//No change
						}
						else
						{
							chorus_change = 1;
						}
						byte = pack_getc(inf);	//New reverb value
						if(byte == -1)
						{	//No change
						}
						else
						{
							reverb_change = 1;
						}
						byte = pack_getc(inf);	//New phaser value
						if(byte == -1)
						{	//No change
						}
						else
						{
							phaser_change = 1;
						}
						byte = pack_getc(inf);	//New tremolo value
						if(byte == -1)
						{	//No change
						}
						else
						{
							tremolo_change = 1;
						}
						if(fileversion >= 500)
						{	//These fields are only in version 5.x files
							(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read the tempo text string
						}
						pack_ReadDWORDLE(inf, &dword);	//New tempo
						if((long)dword == -1)
						{	//No change
						}
						else
						{
							tempo_change = 1;
						}
						if(volume_change)
						{	//This field only exists if a new volume was defined
							(void) pack_getc(inf);	//New volume change transition
						}
						if(pan_change)
						{	//This field only exists if a new pan value was defined
							(void) pack_getc(inf);	//New pan change transition
						}
						if(chorus_change)
						{	//This field only exists if a new  chorus value was defined
							(void) pack_getc(inf);	//New chorus change transition
						}
						if(reverb_change)
						{	//This field only exists if a new reverb value was defined
							(void) pack_getc(inf);	//New reverb change transition
						}
						if(phaser_change)
						{	//This field only exists if a new phaser value was defined
							(void) pack_getc(inf);	//New phaser change transition
						}
						if(tremolo_change)
						{	//This field only exists if a new tremolo value was defined
							(void) pack_getc(inf);	//New tremolo change transition
						}
						if(tempo_change)
						{	//These fields only exists if a new tempo was defined
							(void) pack_getc(inf);	//New tempo change transition
							if(fileversion > 500)
							{	//This field only exists in versions newer than 5.0 of the format
								(void) pack_getc(inf);	//Tempo text string hidden
							}
						}
						if(fileversion >= 400)
						{	//This field is not in version 3.0 files, assume 4.x or higher
							(void) pack_getc(inf);	//Mix table change applied tracks bitmask
						}
						if(fileversion >= 500)
						{	//This unknown byte is only in version 5.x files
							(void) pack_fseek(inf, 1);		//Unknown data
						}
						if(fileversion > 500)
						{	//These strings are only in versions newer than 5.0 of the format
							(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read the Effect 2 string
							(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read the Effect 1 string
						}
					}//Beat has mix table change
					usedstrings = pack_getc(inf);	//Used strings bitmask
					for(ctr4 = 0, bitmask = 64; ctr4 < 7; ctr4++, bitmask>>=1)
					{	//For each of the 7 possible usable strings
						if(bitmask & usedstrings)
						{	//If this string is used
							bytemask = pack_getc(inf);	//Note bitmask
							if(bytemask & 32)
							{	//Note type is defined
								byte = pack_getc(inf);	//Note type (1 = normal, 2 = tie, 3 = dead (muted))
								if(byte == 1)
								{	//Normal note
									frets[ctr4] = 0;	//Ensure this is cleared
									new_note = 1;
								}
								else if(byte == 2)
								{	//If this string is playing a tied note (it is still ringing from a previously played note)
									if(voice == TARGET_VOICE)
									{	//If this is the voice that is being imported
										if(np[ctr2] && (np[ctr2]->note && bitmask))
										{	//If there is a previously created note, and it used this string, alter its length
											unsigned long beat_position, oldlength;
											double partial_beat_position, beat_length;

											tie_note = 1;
											beat_position = (measure_position + note_duration) * curnum;
											partial_beat_position = (measure_position + note_duration) * curnum - beat_position;	//How far into this beat the note ends
											beat_position += curbeat;	//Add the number of beats into the track the current measure is
											beat_length = eof_song->beat[beat_position + 1]->fpos - eof_song->beat[beat_position]->fpos;
											oldlength = np[ctr2]->length;
											np[ctr2]->length = eof_song->beat[beat_position]->fpos + (beat_length * partial_beat_position) - np[ctr2]->pos + 0.5;	//Define the length of this note

#ifdef GP_IMPORT_DEBUG
											(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tTie note:  Note starting at %lums lengthened from %lums to %lums", np[ctr2]->pos, oldlength, np[ctr2]->length);
											eof_log(eof_log_string, 1);
#endif

											usedstrings &= ~bitmask;	//Clear the bit used to indicate the tie note's string as being played, since overlapping guitar notes isn't supported in Rock Band or Rocksmith
										}
									}
								}
								else if(byte == 3)
								{	//If this string is muted
									frets[ctr4] = 128;	//Set the MSB, which is how EOF tracks muted status
									new_note = 1;
								}
							}
							if(bytemask & 4)
							{	//Ghost note
								ghost |= bitmask;	//Mark this string as being ghosted
							}
							if((bytemask & 1) && (fileversion < 500))
							{	//Time independent duration (for versions of the format older than 5.x)
								(void) pack_getc(inf);	//Time independent duration value
								(void) pack_getc(inf);	//Time independent duration values
							}
							if(bytemask & 16)
							{	//Note dynamic
								(void) pack_getc(inf);	//Get the dynamic value
							}
							if(bytemask & 32)
							{	//Note type is defined
								byte = pack_getc(inf);	//Fret number
								frets[ctr4] |= byte;	//OR this value, so that the muted status can be kept if it is set
							}
							if(hopo[ctr2] >= 0)
							{	//If the previous note was marked as leading into a hammer on or pull off with the next (this) note
								if(byte < hopo[ctr2])
								{	//If this note is a lower fret than the previous note
									flags |= EOF_PRO_GUITAR_NOTE_FLAG_PO;	//This note is a pull off
								}
								else
								{	//Otherwise this note is a hammer on
									flags |= EOF_PRO_GUITAR_NOTE_FLAG_HO;
								}
							}
							hopo[ctr2] = -1;	//Reset this status before it is checked if this note is marked (to signal the next note being HO/PO)
							if(bytemask & 128)
							{	//Right/left hand fingering
								byte = pack_getc(inf);	//Left hand fingering
								if(byte == 0)
								{	//Thumb, remap to EOF's definition of thumb = 5
									byte = 5;
								}
								if(byte > 0)
								{	//If the finger number is defined
									finger[ctr4] = byte;	//Store into the finger array
								}
								(void) pack_getc(inf);	//Right hand fingering
							}
							if((bytemask & 1) && (fileversion >= 500))
							{	//Time independent duration (for versions of the format 5.x or newer)
								(void) pack_fseek(inf, 8);		//Unknown data
							}
							if(fileversion >= 500)
							{	//This padding isn't in version 3.x and 4.x files
								(void) pack_fseek(inf, 1);		//Unknown data
							}
							if(bytemask & 8)
							{	//Note effects
								char byte1, byte2 = 0;
								byte1 = pack_getc(inf);	//Note effect bitmask
								if(fileversion >= 400)
								{	//Version 4.0 and higher of the file format has a second note effect bitmask
									byte2 = pack_getc(inf);	//Note effect 2 bitmask
								}
								if(byte1 & 1)
								{	//Bend
									if(eof_gp_parse_bend(inf))
									{	//If there was an error parsing the bend
										allegro_message("Error parsing bend, file is corrupt");
										(void) pack_fclose(inf);
										while(ctr > 0)
										{	//Free the previous track name strings
											free(gp->names[ctr - 1]);
											ctr--;
										}
										free(gp->names);
										for(ctr = 0; ctr < tracks; ctr++)
										{	//Free all previously allocated track structures
											free(gp->track[ctr]);
										}
										for(ctr = 0; ctr < gp->text_events; ctr++)
										{	//Free all allocated text events
											free(gp->text_event[ctr]);
										}
										free(gp->track);
										free(np);
										free(hopo);
										free(nonshiftslide);
										free(gp->capos);
										free(gp);
										free(tsarray);
										free(strings);
										return NULL;
									}
									flags |= EOF_PRO_GUITAR_NOTE_FLAG_BEND;
								}
								if(byte1 & 2)
								{	//Hammer on/pull off from current note (next note gets the HO/PO status)
									hopo[ctr2] = frets[ctr4] & 0x8F;	//Store the fret value (masking out the MSB ghost bit) so that the next note can be determined as either a HO or a PO
								}
								if(byte1 & 4)
								{	//Slide from current note (GP3 format indicator)
									flags |= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP | EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN;	//The slide direction is unknown and will be corrected later
									nonshiftslide[ctr2][ctr4] = 1;	//Track that the next note on this string for this track is to be removed (after slide directions are determined) because it only defines the end of the slide
								}
								if(byte1 & 8)
								{	//Let ring
								}
								if(byte1 & 16)
								{	//Grace note
									(void) pack_getc(inf);	//Grace note fret number
									(void) pack_getc(inf);	//Grace note dynamic value
									if(fileversion >= 500)
									{	//If the file version is 5.x or higher (this byte verified not to be in 3.0 and 4.06 files)
										(void) pack_getc(inf);	//Grace note transition type
									}
									else
									{	//The purpose of this field in 4.x or older files is unknown
										(void) pack_fseek(inf, 1);		//Unknown data
									}
									(void) pack_getc(inf);	//Grace note duration
									if(fileversion >= 500)
									{	//If the file version is 5.x or higher (this byte verified not to be in 3.0 and 4.06 files)
										(void) pack_getc(inf);	//Grace note position
									}
								}
								if(byte2 & 1)
								{	//Note played staccato
								}
								if(byte2 & 2)
								{	//Palm mute
									flags |= EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE;
								}
								if(byte2 & 4)
								{	//Tremolo picking
									(void) pack_getc(inf);	//Tremolo picking speed
									flags |= EOF_NOTE_FLAG_IS_TREMOLO;
								}
								if(byte2 & 8)
								{	//Slide
									byte = pack_getc(inf);	//Slide type
									if(byte & 4)
									{	//This note slides out and downwards
										flags |= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN;
									}
									else if(byte & 8)
									{	//This note slides out and upwards
										flags |= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP;
									}
									else
									{
										if(byte & 2)
										{	//If this is a legato slide
											nonshiftslide[ctr2][ctr4] = 1;	//Track that the next note on this string for this track is to be removed (after slide directions are determined) because it only defines the end of the slide
										}
										flags |= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP | EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN;	//The slide direction is unknown and will be corrected later
									}
								}
								if(byte2 & 16)
								{	//Harmonic
									flags |= EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC;
									byte = pack_getc(inf);	//Harmonic type
									if(byte == 2)
									{	//Artificial harmonic
										(void) pack_getc(inf);	//Read harmonic note
										(void) pack_getc(inf);	//Read sharp/flat status
										(void) pack_getc(inf);	//Read octave status
									}
									else if(byte == 3)
									{	//Tapped harmonic
										(void) pack_getc(inf);	//Right hand fret
									}
								}
								if(byte2 & 32)
								{	//Trill
									(void) pack_getc(inf);	//Trill with fret
									(void) pack_getc(inf);	//Trill duration
									flags |= EOF_NOTE_FLAG_IS_TRILL;
///It might be necessary to insert notes here for the trill phrase
								}
								if(byte2 & 64)
								{	//Vibrato
									flags |= EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO;
								}
							}//Note effects
						}//If this string is used
					}//For each of the 7 possible usable strings
					if(fileversion >= 500)
					{	//Version 5.0 and higher of the file format stores a note transpose mask and unknown data here
						pack_ReadWORDLE(inf, &word);	//Transpose bitmask
						if(word & 0x800)
						{	//If bit 11 of the transpose bitmask was set, there is an additional byte of unknown data
							(void) pack_fseek(inf, 1);	//Unknown data
						}
					}
					if(voice == TARGET_VOICE)
					{	//If this is the voice that is being imported
						if(new_note)
						{	//If a new note is to be created
							unsigned long beat_position;
							double partial_beat_position, beat_length;

							np[ctr2] = eof_pro_guitar_track_add_note(gp->track[ctr2]);	//Add a new note to the current track
							if(!np[ctr2])
							{
								eof_log("Error allocating memory (15)", 1);
								(void) pack_fclose(inf);
								while(ctr > 0)
								{	//Free the previous track name strings
									free(gp->names[ctr - 1]);
									ctr--;
								}
								free(gp->names);
								for(ctr = 0; ctr < tracks; ctr++)
								{	//Free all previously allocated track structures
									free(gp->track[ctr]);
								}
								for(ctr = 0; ctr < gp->text_events; ctr++)
								{	//Free all allocated text events
									free(gp->text_event[ctr]);
								}
								free(gp->track);
								free(np);
								free(hopo);
								free(nonshiftslide);
								free(gp->capos);
								free(gp);
								free(tsarray);
								free(strings);
								return NULL;
							}
							np[ctr2]->flags = flags;
							for(ctr4 = 0; ctr4 < strings[ctr2]; ctr4++)
							{	//For each of this track's natively supported strings
								unsigned int convertednum = strings[ctr2] - 1 - ctr4;	//Re-map from GP's string numbering to EOF's (EOF stores 16 fret values per note, it just only uses 6 by default)
								if(strings[ctr2] > 6)
								{	//If this is a 7 string Guitar Pro track
									convertednum--;	//Remap so that string 7 is ignored and the other 6 are read
								}
								np[ctr2]->frets[ctr4] = frets[convertednum] + gp->capos[ctr2];	//Copy the fret number for this string, adding this track's capo position (if any)
								np[ctr2]->finger[ctr4] = finger[convertednum];	//Copy the finger number used to fret the string (is nonzero if defined)
							}
							np[ctr2]->legacymask = 0;
							np[ctr2]->midi_length = 0;
							np[ctr2]->midi_pos = 0;
							np[ctr2]->name[0] = '\0';
							if(strings[ctr2] > 6)
							{	//If this is a 7 string track
								np[ctr2]->note = usedstrings >> 1;	//Shift out string 7 to leave the first 6 strings
								np[ctr2]->ghost = ghost >> 1;		//Likewise translate the ghost bit mask
							}
							else
							{
								np[ctr2]->note = usedstrings >> (7 - strings[ctr2]);	//Guitar pro's note bitmask reflects string 7 being the LSB
								np[ctr2]->ghost = ghost >> (7 - strings[ctr2]);			//Likewise translate the ghost bit mask
							}
							np[ctr2]->type = eof_note_type;

	//Determine the correct timestamp position and duration
							beat_position = measure_position * curnum;								//How many whole beats into the current measure the position is
							partial_beat_position = (measure_position * curnum) - beat_position;	//How far into this beat the note begins
							beat_position += curbeat;	//Add the number of beats into the track the current measure is
							beat_length = eof_song->beat[beat_position + 1]->fpos - eof_song->beat[beat_position]->fpos;
							np[ctr2]->pos = eof_song->beat[beat_position]->fpos + (beat_length * partial_beat_position) + 0.5;	//Define the position of this note (rounded up)

							beat_position = (measure_position + note_duration) * curnum;
							partial_beat_position = (measure_position + note_duration) * curnum - beat_position;	//How far into this beat the note ends
							beat_position += curbeat;	//Add the number of beats into the track the current measure is
							beat_length = eof_song->beat[beat_position + 1]->fpos - eof_song->beat[beat_position]->fpos;
							np[ctr2]->length = eof_song->beat[beat_position]->fpos + (beat_length * partial_beat_position) - np[ctr2]->pos + 0.5;	//Define the length of this note

							if(note_is_short && eof_gp_import_truncate_short_notes)
							{	//If this note is shorter than a quarter note, and the preference to drop the note's sustain in this circumstance is enabled
								if(!(np[ctr2]->flags & EOF_PRO_GUITAR_NOTE_FLAG_BEND) && !(np[ctr2]->flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP) && !(np[ctr2]->flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN) && !(np[ctr2]->flags & EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO))
								{	//If this note doesn't have bend, slide or vibrato status
									np[ctr2]->length = 1;	//Remove the note's sustain
								}
							}

#ifdef GP_IMPORT_DEBUG
							(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tNote #%lu:  Start: %lums\tLength: %lums\tFrets+capo: ", gp->track[ctr2]->notes - 1, np[ctr2]->pos, np[ctr2]->length);
							for(ctr4 = 0, bitmask = 1; ctr4 < strings[ctr2]; ctr4++, bitmask <<= 1)
							{	//For each of this track's natively supported strings
								char temp[10];
								if(np[ctr2]->note & bitmask)
								{	//If this string is used
									if(np[ctr2]->frets[ctr4] & 0x80)
									{	//If this string is muted
										(void) snprintf(temp, sizeof(temp) - 1, "X");
									}
									else
									{
										(void) snprintf(temp, sizeof(temp) - 1, "%u", np[ctr2]->frets[ctr4]);
									}
								}
								else
								{
									(void) snprintf(temp, sizeof(temp) - 1, "_");
								}
								if(ctr4 + 1 < strings[ctr2])
								{	//If there is another string
									(void) strncat(temp, ", ", sizeof(temp) - 1);
								}
								(void) strncat(eof_log_string, temp, sizeof(eof_log_string) - 1);
							}
							eof_log(eof_log_string, 1);
#endif

							for(ctr4 = 0; ctr4 < strings[ctr2]; ctr4++)
							{	//For each of this track's natively supported strings
								if(nonshiftslide[ctr2][ctr4] == 1)
								{	//If the next note on this track is to be removed (after slide directions are determined)
									nonshiftslide[ctr2][ctr4] = 2;	//Indicate that the slide note is being added, the next note on this string will see 2 as the signal to mark itself for removal (set its length as 0)
								}
								else if(nonshiftslide[ctr2][ctr4] == 2)
								{	//If this is the note that will need to be removed
									np[ctr2]->flags |= EOF_NOTE_FLAG_HOPO;	//Use this flag to indicate that the note will need to be removed
									nonshiftslide[ctr2][ctr4] = 0;	//Mark that the slide has been handled
								}
							}
						}//If a new note is to be created
						else if(np[ctr2] && tie_note)
						{	//Otherwise if this was a tie note
							np[ctr2]->flags |= flags;	//Apply this tie note's flags to the previous note
						}
					}//If this is the voice that is being imported
					measure_position += note_duration;	//Update the measure position
				}//For each beat
			}//For each voice
			if(fileversion >= 500)
			{
				(void) pack_fseek(inf, 1);		//Unknown data
			}
		}//For each track
		curbeat += curnum;	//Add this measure's number of beats to the beat counter
	}//For each measure

//Correct slide directions
	for(ctr = 0; ctr < gp->numtracks; ctr++)
	{	//For each imported track
		for(ctr2 = 0; ctr2 < gp->track[ctr]->notes; ctr2++)
		{	//For each note in the track
			if((gp->track[ctr]->note[ctr2]->flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP) && (gp->track[ctr]->note[ctr2]->flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN) && (ctr2 + 1 < gp->track[ctr]->notes))
			{	//If this note is marked as being an undetermined direction slide, and there's a note that follows
				startfret = 0;
				for(ctr3 = 0, bitmask = 1; ctr3 < 7; ctr3++, bitmask<<=1)
				{	//For each of the 7 strings the GP format allows for
					if((bitmask & gp->track[ctr]->note[ctr2]->note) && (bitmask & gp->track[ctr]->note[ctr2 + 1]->note))
					{	//If this string is used on this note as well as the next
						startfret = gp->track[ctr]->note[ctr2]->frets[ctr3];	//Record the fret number used on this note
						if(gp->track[ctr]->note[ctr2 + 1]->frets[ctr3] & 0x80)
						{	//If the end of the slide is defined by a mute note
							endfret =  0;	//Force it to be a downward slide, the end fret of the slide will be undefined
						}
						else
						{
							endfret = gp->track[ctr]->note[ctr2 + 1]->frets[ctr3];	//Record the fret number used on the next note
						}
						if(startfret == endfret)
						{	//If both strings use the same fret
							continue;	//Check the next string instead (this one might be being played open)
						}
						if(startfret > endfret)
						{	//This is a downward slide
							gp->track[ctr]->note[ctr2]->flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP;	//Clear the slide up flag
						}
						else
						{	//This is an upward slide
							gp->track[ctr]->note[ctr2]->flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN;	//Clear the slide down flag
						}
						if(gp->track[ctr]->note[ctr2]->slideend == 0)
						{	//If the slide ending hasn't been defined yet
							gp->track[ctr]->note[ctr2]->slideend = endfret;
							gp->track[ctr]->note[ctr2]->flags |= EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION;	//Indicate that the note has the slide ending defined
						}
						break;
					}
				}
			}
		}
	}

//Delete notes that were marked to be removed (have a length of 0) because they were only present to define the end fret number for a slide
	for(ctr = 0; ctr < gp->numtracks; ctr++)
	{	//For each imported track
		for(ctr2 = gp->track[ctr]->notes; ctr2 > 0; ctr2--)
		{	//For each note in the track (in reverse order)
			if(gp->track[ctr]->note[ctr2 - 1]->length == 0)
			{	//If this note has been given a length of 0
				eof_pro_guitar_track_delete_note(gp->track[ctr], ctr2 - 1);	//Remove it
			}
		}
	}

//Unwrap repeat sections if necessary
	for(ctr = 0; ctr < gp->numtracks; ctr++)
	{	//For each imported track
		if(eof_unwrap_gp_track(gp, ctr, import_ts))
		{	//If the track failed to unwrap
			allegro_message("Warning:  Failed to unwrap repeats for track #%lu (%s).", ctr + 1, gp->names[ctr]);
		}
		import_ts = 0;	//Only unwrap the time signatures on the first pass
	}

//Create trill phrases
	for(ctr = 0; ctr < gp->numtracks; ctr++)
	{	//For each imported track
		for(ctr2 = 0; ctr2 < gp->track[ctr]->notes; ctr2++)
		{	//For each note in the track
			if(gp->track[ctr]->note[ctr2]->flags & EOF_NOTE_FLAG_IS_TRILL)
			{	//If this note is marked as being in a trill
				startpos = gp->track[ctr]->note[ctr2]->pos;	//Mark the start of this phrase
				endpos = startpos + gp->track[ctr]->note[ctr2]->length;	//Initialize the end position of the phrase
				while(++ctr2 < gp->track[ctr]->notes)
				{	//For the consecutive remaining notes in the track
					if(gp->track[ctr]->note[ctr2]->flags & EOF_NOTE_FLAG_IS_TRILL)
					{	//And the next note is also marked as a trill
						endpos = gp->track[ctr]->note[ctr2]->pos + gp->track[ctr]->note[ctr2]->length;	//Update the end position of the phrase
					}
					else
					{
						break;	//Break from while loop.  This note isn't a trill so the next pass doesn't need to check it either
					}
				}
				count = gp->track[ctr]->trills;
				if(count < EOF_MAX_PHRASES)
				{	//If the track can store the trill section
					gp->track[ctr]->trill[count].start_pos = startpos;
					gp->track[ctr]->trill[count].end_pos = endpos;
					gp->track[ctr]->trill[count].flags = 0;
					gp->track[ctr]->trill[count].name[0] = '\0';
					gp->track[ctr]->trills++;
				}
			}
		}
	}

//Create tremolo phrases
	for(ctr = 0; ctr < gp->numtracks; ctr++)
	{	//For each imported track
		for(ctr2 = 0; ctr2 < gp->track[ctr]->notes; ctr2++)
		{	//For each note in the track
			if(gp->track[ctr]->note[ctr2]->flags & EOF_NOTE_FLAG_IS_TREMOLO)
			{	//If this note is marked as being in a tremolo
				startpos = gp->track[ctr]->note[ctr2]->pos;	//Mark the start of this phrase
				endpos = startpos + gp->track[ctr]->note[ctr2]->length;	//Initialize the end position of the phrase
				while(++ctr2 < gp->track[ctr]->notes)
				{	//For the consecutive remaining notes in the track
					if(gp->track[ctr]->note[ctr2]->flags & EOF_NOTE_FLAG_IS_TREMOLO)
					{	//And the next note is also marked as a tremolo
						endpos = gp->track[ctr]->note[ctr2]->pos + gp->track[ctr]->note[ctr2]->length;	//Update the end position of the phrase
					}
					else
					{
						break;	//Break from while loop.  This note isn't a tremolo so the next pass doesn't need to check it either
					}
				}
				count = gp->track[ctr]->tremolos;
				if(count < EOF_MAX_PHRASES)
				{	//If the track can store the tremolo section
					gp->track[ctr]->tremolo[count].start_pos = startpos;
					gp->track[ctr]->tremolo[count].end_pos = endpos;
					gp->track[ctr]->tremolo[count].flags = 0;
					gp->track[ctr]->tremolo[count].name[0] = '\0';
					if(eof_song->track[eof_selected_track]->flags & EOF_TRACK_FLAG_UNLIMITED_DIFFS)
					{	//If the active project's active track already had the difficulty limit removed
						gp->track[ctr]->tremolo[count].difficulty = eof_note_type;	//The tremolo will be made specific to the active track difficulty
					}
					else
					{	//Otherwise it will apply to all track difficulties
						gp->track[ctr]->tremolo[count].difficulty = 0xFF;
					}
					gp->track[ctr]->tremolos++;
				}
			}
		}
	}

//Validate any imported note/chord fingerings and duplicate defined fingerings to matching notes
	for(ctr = 0; ctr < gp->numtracks; ctr++)
	{	//For each imported track
		eof_pro_guitar_track_fix_fingerings(gp->track[ctr], NULL);	//Erase partial note fingerings, replicate valid finger definitions to matching notes without finger definitions, don't make any additional undo states for corrections to the imported track
	}

//Clean up
	(void) pack_fclose(inf);
	free(strings);
	free(tsarray);
	free(np);
	free(hopo);
	free(nonshiftslide);
	(void) puts("\nSuccess");
	return gp;
}

int eof_unwrap_gp_track(struct eof_guitar_pro_struct *gp, unsigned long track, char import_ts)
{
	unsigned long ctr, currentmeasure, beatctr, last_start_of_repeat;
	unsigned char has_repeats = 0, has_symbols = 0;
	unsigned char *working_num_of_repeats;	//Will store a copy of gp->measure[]'s repeat information so that the information in gp isn't destroyed
	EOF_PRO_GUITAR_TRACK *tp;		//Stores the resulting track that is inserted into gp
	unsigned long *measuremap;		//Will be used to refer to a dynamically built array storing the beat number at which each measure begins
	EOF_TEXT_EVENT * newevent[EOF_MAX_TEXT_EVENTS];	//Will be used to rebuild the text events array to the appropriate unwrapped beat positions, if the first track is being unwrapped
	unsigned long newevents = 0;		//The number of events stored in the above array
	char unwrapevents = 0;		//Will track whether text events are to be unwrapped
	unsigned int working_symbols[19];	//Will store a copy of gp->symbols[] so that the information ingp isn't destroyed

	eof_log("eof_unwrap_gp_track() entered", 1);

	if(!gp || (track >= gp->numtracks))
	{
		eof_log("\tInvalid parameters", 1);
		return 1;	//Invalid parameters
	}

	gp->coda_activated = gp->double_coda_activated = gp->fine_activated = 0;	//Reset these statuses

	//First parse the measures[] array to determine if any end of repeats are present
	for(ctr = 0; ctr < gp->measures; ctr++)
	{	//For each measure in the GP file
		if(gp->measure[ctr].num_of_repeats)
		{	//If this measure is an end of repeat
			has_repeats = 1;
			break;
		}
	}
	//Check to see if any navigational symbols were placed
	for(ctr = 5; ctr < 19; ctr++)
	{	//For each of the navigational symbols (except the first 5, which are labels and not branches)
		if(gp->symbols[ctr] != 0xFFFF)
		{	//If this symbol is placed in the composition
			has_symbols = 1;
			break;
		}
	}
	if(!has_repeats && !has_symbols)
	{	//If this GP file has no repeats and no branching
		eof_log("\tNo repeats or navigational symbols detected", 1);
		return 0;	//Return without altering the gp structure
	}

	//Create a working copy of the array tracking the number of repeats left for each measure
	working_num_of_repeats = malloc(sizeof(unsigned char) * gp->measures);
	if(!working_num_of_repeats)
	{	//Error allocating memory
		eof_log("\tError allocating memory to unwrap GP track (1)", 1);
		return 1;
	}
	for(ctr = 0; ctr < gp->measures; ctr++)
	{	//For each measure in the GP file
		working_num_of_repeats[ctr] = gp->measure[ctr].num_of_repeats;	//Copy the number of repeats
	}

	//Create a working copy of the array tracking the navigational symbols
	memcpy(working_symbols, gp->symbols, sizeof(working_symbols));

	//Create a new pro guitar track structure to unwrap into
	tp = malloc(sizeof(EOF_PRO_GUITAR_TRACK));
	if(!tp)
	{
		eof_log("\tError allocating memory to unwrap GP track (2)", 1);
		free(working_num_of_repeats);
		return 2;
	}
	memset(tp, 0, sizeof(EOF_PRO_GUITAR_TRACK));	//Initialize memory block to 0 to avoid crashes when not explicitly setting counters that were newly added to the pro guitar structure
	tp->numfrets = gp->track[track]->numfrets;
	tp->numstrings = gp->track[track]->numstrings;
	tp->parent = NULL;
	memcpy(tp->tuning, gp->track[track]->tuning, sizeof(char) * EOF_TUNING_LENGTH);

	//Build an array tracking the beat number marking the start of each measure
	measuremap = malloc(sizeof(unsigned long) * gp->measures);	//Allocate enough memory to store this information for each measure imported from the gp file
	if(!measuremap)
	{
		eof_log("\tError allocating memory to unwrap GP track (3)", 1);
		free(tp);
		free(working_num_of_repeats);
		return 3;
	}
	memset(measuremap, 0, sizeof(unsigned long) * gp->measures);	//Initialize this memory to 0
	for(ctr = 0, beatctr = 0; ctr < gp->measures; ctr++)
	{	//For each measure in the GP file
		measuremap[ctr] = beatctr;		//Store the beat number at which this measure begins
		beatctr += gp->measure[ctr].num;	//Add the number of beats in this measure to the ongoing counter
	}

	//If the time signatures are to be imported, clear any existing signatures placed in the active project
	if(import_ts)
	{
		for(ctr = 0; ctr < eof_song->beats; ctr++)
		{	//For each beat in the project, clear all beat flags except those that aren't TS related
			eof_song->beat[ctr]->flags &= (EOF_BEAT_FLAG_ANCHOR | EOF_BEAT_FLAG_EVENTS | EOF_BEAT_FLAG_KEY_SIG);	//Keep these flags as-is
		}
	}

	//If track 0 is being unwrapped, this function call will have the text events unwrapped
	if(!track && gp->text_events)
	{	//Only bother if there's at least one text event
		unwrapevents = 1;
	}

	//Parse the measure[] array again, unwrapping the beats and their content into the new pro guitar track
	currentmeasure = 0;		//Start with the first measure
	beatctr = 0;			//Reset this counter, which will track the number of beats unwrapped into the new pro guitar track
	last_start_of_repeat = 0;	//The first measure is a start of repeat by default
	while(currentmeasure < gp->measures)
	{	//Continue until all repeats of all measures have been processed
		if(gp->measure[currentmeasure].start_of_repeat)
		{	//If this measure is the start of a repeat
#ifdef GP_IMPORT_DEBUG
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tTracking start of repeat at measure #%lu", currentmeasure + 1);
			eof_log(eof_log_string, 1);
#endif
			last_start_of_repeat = currentmeasure;	//Track it as the most recent start of repeat
		}

		//Ensure the active project has enough beats to hold the next unwrapped measure
		if(eof_song->beats < beatctr + gp->measure[currentmeasure].num + 2)
		{
#ifdef GP_IMPORT_DEBUG
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tUnwrapping measure #%lu requires %lu beats, but project only has %lu beats.  Appending %lu beats to the project to allow for this measure and two beats of padding.", currentmeasure + 1, beatctr + gp->measure[currentmeasure].num, eof_song->beats, beatctr + gp->measure[currentmeasure].num + 2 - eof_song->beats);
			eof_log(eof_log_string, 1);
#endif
		}
		while(eof_song->beats < beatctr + gp->measure[currentmeasure].num + 2)
		{	//Until the loaded project has enough beats to contain the unwrapped measure, and two extra to allow room for processing beat lengths later
			double beat_length;

			if(!eof_song_add_beat(eof_song))
			{
				eof_log("\tError allocating memory to unwrap GP track (4)", 1);
				free(measuremap);
				free(tp);
				free(working_num_of_repeats);
				for(ctr = 0; ctr < newevents; ctr++)
				{	//For each unwrapped text event
					free(newevent[ctr]);	//Free it
				}
				return 4;
			}
			eof_song->beat[eof_song->beats - 1]->ppqn = eof_song->beat[eof_song->beats - 2]->ppqn;	//Match the tempo of the previously last beat
			beat_length = (double)60000.0 / ((double)60000000.0 / (double)eof_song->beat[eof_song->beats - 2]->ppqn);	//Get the length of the previously last beat
			eof_song->beat[eof_song->beats - 1]->fpos = eof_song->beat[eof_song->beats - 2]->fpos + beat_length;
			eof_song->beat[eof_song->beats - 1]->pos = eof_song->beat[eof_song->beats - 1]->fpos + 0.5;	//Round up
		}
		eof_chart_length = eof_song->beat[eof_song->beats - 1]->pos;	//Alter the chart length so that the full transcription will display

		//Copy the contents in this measure to the new pro guitar track
#ifdef GP_IMPORT_DEBUG
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tUnwrapping %d beats from measure #%lu (starts at beat %lu) from original track to beat %lu in new track", gp->measure[currentmeasure].num, currentmeasure + 1, measuremap[currentmeasure], beatctr);
		eof_log(eof_log_string, 1);
#endif
		if(!eof_copy_notes_in_beat_range(gp->track[track], measuremap[currentmeasure], gp->measure[currentmeasure].num, tp, beatctr))
		{	//If there was an error unwrapping the repeat into the new pro guitar track
			eof_log("\tError unwrapping beats", 1);
			free(measuremap);
			free(tp);
			free(working_num_of_repeats);
			for(ctr = 0; ctr < newevents; ctr++)
			{	//For each unwrapped text event
				free(newevent[ctr]);	//Free it
			}
			return 5;
		}

		//Update the time signature if necessary
		if(import_ts)
		{
			if(!currentmeasure || (gp->measure[currentmeasure].num != gp->measure[currentmeasure - 1].num) || (gp->measure[currentmeasure].den != gp->measure[currentmeasure - 1].den))
			{	//If this is the first measure or this measure's time signature is different from that of the previous measure
				(void) eof_apply_ts(gp->measure[currentmeasure].num, gp->measure[currentmeasure].den, beatctr, eof_song, 0);	//Apply the change to the active project
			}
		}

		//If this measure has a text event, and text events are being unwrapped, copy it to the new text event array
		if(unwrapevents)
		{	//If text events are being unwrapped
			for(ctr = 0; ctr < gp->text_events; ctr++)
			{	//For each text event that was imported from the gp file
				if(gp->text_event[ctr]->beat == measuremap[currentmeasure])
				{	//If the text event is positioned at this measure
					if(newevents < EOF_MAX_TEXT_EVENTS)
					{	//If the maximum number of text events hasn't already been defined
						newevent[newevents] = malloc(sizeof(EOF_TEXT_EVENT));	//Allocate space for the text event
						if(!newevent[newevents])
						{
							eof_log("\tError allocating memory to unwrap GP track (5)", 1);
							free(measuremap);
							free(tp);
							free(working_num_of_repeats);
							for(ctr = 0; ctr < newevents; ctr++)
							{	//For each unwrapped text event
								free(newevent[ctr]);	//Free it
							}
							return 6;
						}
						memcpy(newevent[newevents], gp->text_event[ctr], sizeof(EOF_TEXT_EVENT));	//Copy the text event
						newevent[newevents]->beat = beatctr;	//Correct the beat number
						newevents++;
					}
					break;
				}
			}
		}

		beatctr += gp->measure[currentmeasure].num;	//Add the number of beats contained in this measure to the ongoing counter

		//Either increment to the next measure or seek to the beginning of a repeat/symbol
		if(working_num_of_repeats[currentmeasure])
		{	//If this measure has an end of repeat with any repeats left, seek to the appropriate measure
#ifdef GP_IMPORT_DEBUG
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tEnd of repeat (%d repeats left) -> Seeking to measure #%lu", working_num_of_repeats[currentmeasure] - 1, last_start_of_repeat + 1);
			eof_log(eof_log_string, 1);
#endif
			working_num_of_repeats[currentmeasure]--;	//Decrement the number of repeats left for this marker
			currentmeasure = last_start_of_repeat;	//Jump to the last start of repeat that was encountered
		}
		else
		{
			working_num_of_repeats[currentmeasure] = gp->measure[currentmeasure].num_of_repeats;	//Restore the repeat count of this measure, in case a navigation symbol causes it to be unwrapped again later

			if((working_symbols[EOF_FINE_SYMBOL] == currentmeasure) && gp->fine_activated)
			{	//If the "Fine" symbol was reached after a "da ... al fine" symbol, this marks the end of the composition
#ifdef GP_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\"Fine\" reached, ending unwrap process");
				eof_log(eof_log_string, 1);
#endif
				break;
			}
			else if(working_symbols[EOF_DA_CAPO_SYMBOL] == currentmeasure)
			{	//If the "Da Capo" symbol was reached, seek back to first measure
#ifdef GP_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\"Da Capo\" reached, seeking to first measure");
				eof_log(eof_log_string, 1);
#endif
				currentmeasure = 0;
				working_symbols[EOF_DA_CAPO_SYMBOL] = 0xFFFF;	//This symbol is now destroyed
			}
			else if(working_symbols[EOF_DA_CAPO_AL_CODA_SYMBOL] == currentmeasure)
			{	//If the "Da Capo al Coda" symbol was reached, seek to the first measure and expect a "Da Coda" symbol to follow
#ifdef GP_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\"Da Capo al Coda\" reached, seeking to first measure");
				eof_log(eof_log_string, 1);
#endif
				currentmeasure = 0;
				gp->coda_activated = 1;	//Perform a seek when the "Da Coda" symbol is reached
				working_symbols[EOF_DA_CAPO_AL_CODA_SYMBOL] = 0xFFFF;	//This symbol is now destroyed
			}
			else if(working_symbols[EOF_DA_CAPO_AL_DOUBLE_CODA_SYMBOL] == currentmeasure)
			{	//If the "Da Capo al Double Coda" symbol was reached, seek to the first measure and expect a "Da Coda" symbol to follow
#ifdef GP_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\"Da Capo al Double Coda\" reached, seeking to first measure");
				eof_log(eof_log_string, 1);
#endif
				currentmeasure = 0;
				gp->double_coda_activated = 1;	//Perform a seek when the "Da Double Coda" symbol is reached
				working_symbols[EOF_DA_CAPO_AL_DOUBLE_CODA_SYMBOL] = 0xFFFF;	//This symbol is now destroyed
			}
			else if(working_symbols[EOF_DA_CAPO_AL_FINE_SYMBOL] == currentmeasure)
			{	//If the "Da Capo al Fine" symbol was reached, seek to the first measure and expect a "Fine" symbol to follow
#ifdef GP_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\"Da Capo al Fine\" reached, seeking to first measure");
				eof_log(eof_log_string, 1);
#endif
				currentmeasure = 0;
				gp->fine_activated = 1;	//End unwrapping when the "Fine" symbol is reached
				working_symbols[EOF_DA_CAPO_AL_FINE_SYMBOL] = 0xFFFF;	//This symbol is now destroyed
			}
			else if(working_symbols[EOF_DA_SEGNO_SYMBOL] == currentmeasure)
			{	//If the "Da Segno" symbol was reached, seek to the measure containing the Segno symbol
#ifdef GP_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\"Da Segno\" reached, seeking to measure #%u", working_symbols[EOF_SEGNO_SYMBOL] + 1);
				eof_log(eof_log_string, 1);
#endif
				currentmeasure = working_symbols[EOF_SEGNO_SYMBOL];
				working_symbols[EOF_DA_SEGNO_SYMBOL] = 0xFFFF;	//This symbol is now destroyed
			}
			else if(working_symbols[EOF_DA_SEGNO_AL_CODA_SYMBOL] == currentmeasure)
			{	//If the "Da Segno al Coda" symbol was reached, seek to the measure containing the Segno symbol and expect a "Da Coda" symbol to follow
#ifdef GP_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\"Da Segno al Coda\" reached, seeking to measure #%u", working_symbols[EOF_SEGNO_SYMBOL] + 1);
				eof_log(eof_log_string, 1);
#endif
				currentmeasure = working_symbols[EOF_SEGNO_SYMBOL];
				gp->coda_activated = 1;	//Seek to "Coda" symbol when the "Da Coda" symbol is reached
				working_symbols[EOF_DA_SEGNO_AL_CODA_SYMBOL] = 0xFFFF;	//This symbol is now destroyed
			}
			else if(working_symbols[EOF_DA_SEGNO_AL_DOUBLE_CODA_SYMBOL] == currentmeasure)
			{	//If the "Da Segno al Double Coda" symbol was reached, seek to the measure containing the Segno symbol and expect a "Da Double Coda" symbol to follow
#ifdef GP_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\"Da Segno al Double Coda\" reached, seeking to measure #%u", working_symbols[EOF_SEGNO_SYMBOL] + 1);
				eof_log(eof_log_string, 1);
#endif
				currentmeasure = working_symbols[EOF_SEGNO_SYMBOL];
				gp->double_coda_activated = 1;	//Seek to "Double Coda" symbol when the "Da Double Coda" symbol is reached
				working_symbols[EOF_DA_SEGNO_AL_DOUBLE_CODA_SYMBOL] = 0xFFFF;	//This symbol is now destroyed
			}
			else if(working_symbols[EOF_DA_SEGNO_AL_FINE_SYMBOL] == currentmeasure)
			{	//If the "Da Segno al Fine" symbol was reached, seek to the measure containing the Segno symbol and expect a "Fine" symbol to follow
#ifdef GP_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\"Da Segno al Fine\" reached, seeking to measure #%u", working_symbols[EOF_SEGNO_SYMBOL] + 1);
				eof_log(eof_log_string, 1);
#endif
				currentmeasure = working_symbols[EOF_SEGNO_SYMBOL];
				gp->fine_activated = 1;	//End unwrapping when the "Fine" symbol is reached
				working_symbols[EOF_DA_SEGNO_AL_FINE_SYMBOL] = 0xFFFF;	//This symbol is now destroyed
			}
			else if(working_symbols[EOF_DA_SEGNO_SEGNO_SYMBOL] == currentmeasure)
			{	//If the "Da Segno Segno" symbol was reached, seek to the measure containing the Segno Segno symbol
#ifdef GP_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\"Da Segno Segno\" reached, seeking to measure #%u", working_symbols[EOF_SEGNO_SEGNO_SYMBOL] + 1);
				eof_log(eof_log_string, 1);
#endif
				currentmeasure = working_symbols[EOF_SEGNO_SEGNO_SYMBOL];
				working_symbols[EOF_DA_SEGNO_SEGNO_SYMBOL] = 0xFFFF;	//This symbol is now destroyed
			}
			else if(working_symbols[EOF_DA_SEGNO_SEGNO_AL_CODA_SYMBOL] == currentmeasure)
			{	//If the "Da Segno Segno al Coda" symbol was reached, seek to the measure containing the Segno Segno symbol and expect a "Da Coda" symbol to follow
#ifdef GP_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\"Da Segno Segno al Coda\" reached, seeking to measure #%u", working_symbols[EOF_SEGNO_SEGNO_SYMBOL] + 1);
				eof_log(eof_log_string, 1);
#endif
				currentmeasure = working_symbols[EOF_SEGNO_SEGNO_SYMBOL];
				gp->coda_activated = 1;	//Seek to "Coda" symbol when the "Da Coda" symbol is reached
				working_symbols[EOF_DA_SEGNO_SEGNO_AL_CODA_SYMBOL] = 0xFFFF;	//This symbol is now destroyed
			}
			else if(working_symbols[EOF_DA_SEGNO_SEGNO_AL_DOUBLE_CODA_SYMBOL] == currentmeasure)
			{	//If the "Da Segno Segno al Double Coda" symbol was reached, seek to the measure containing the Segno Segno symbol and expect a "Da Double Coda" symbol to follow
#ifdef GP_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\"Da Segno Segno al Double Coda\" reached, seeking to measure #%u", working_symbols[EOF_SEGNO_SEGNO_SYMBOL] + 1);
				eof_log(eof_log_string, 1);
#endif
				currentmeasure = working_symbols[EOF_SEGNO_SEGNO_SYMBOL];
				gp->double_coda_activated = 1;	//Seek to "Double Coda" symbol when the "Da Double Coda" symbol is reached
				working_symbols[EOF_DA_SEGNO_SEGNO_AL_DOUBLE_CODA_SYMBOL] = 0xFFFF;	//This symbol is now destroyed
			}
			else if(working_symbols[EOF_DA_SEGNO_SEGNO_AL_FINE_SYMBOL] == currentmeasure)
			{	//If the "Da Segno Segno al Fine" symbol was reached, seek to the measure containing the Segno Segno symbol and expect a "Fine" symbol to follow
#ifdef GP_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\"Da Segno Segno al Fine\" reached, seeking to measure #%u", working_symbols[EOF_SEGNO_SEGNO_SYMBOL] + 1);
				eof_log(eof_log_string, 1);
#endif
				currentmeasure = working_symbols[EOF_SEGNO_SEGNO_SYMBOL];
				gp->fine_activated = 1;	//End unwrapping when the "Fine" symbol is reached
				working_symbols[EOF_DA_SEGNO_SEGNO_AL_FINE_SYMBOL] = 0xFFFF;	//This symbol is now destroyed
			}
			else if((working_symbols[EOF_DA_CODA_SYMBOL] == currentmeasure) && gp->coda_activated)
			{	//If the "Da Coda" symbol was reached after a "Da ... al Coda" symbol, seek to the measure containing the Coda symbol
#ifdef GP_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\"Da Coda\" reached, seeking to measure #%u", working_symbols[EOF_CODA_SYMBOL] + 1);
				eof_log(eof_log_string, 1);
#endif
				currentmeasure = working_symbols[EOF_CODA_SYMBOL];
				gp->coda_activated = 0;	//Reset this status
				working_symbols[EOF_DA_CODA_SYMBOL] = 0xFFFF;	//This symbol is now destroyed
			}
			else if((working_symbols[EOF_DA_DOUBLE_CODA_SYMBOL] == currentmeasure) && gp->double_coda_activated)
			{	//If the "Da Double Coda" symbol was reached after a "Da ... al Double Coda" symbol, seek to the measure containing the Double Coda symbol
#ifdef GP_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\"Da Double Coda\" reached, seeking to measure #%u", working_symbols[EOF_DOUBLE_CODA_SYMBOL] + 1);
				eof_log(eof_log_string, 1);
#endif
				currentmeasure = working_symbols[EOF_DOUBLE_CODA_SYMBOL];
				gp->double_coda_activated = 0;	//Reset this status
				working_symbols[EOF_DA_DOUBLE_CODA_SYMBOL] = 0xFFFF;	//This symbol is now destroyed
			}
			else
			{
#ifdef GP_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tContinuing to next measure");
				eof_log(eof_log_string, 1);
#endif
				currentmeasure++;	//Otherwise continue to the next measure
			}
		}
	}//Continue until all repeats of all measures have been processed

	//Replace the target pro guitar track with the new one
	for(ctr = 0; ctr < gp->track[track]->notes; ctr++)
	{	//For each note in the target track
		free(gp->track[track]->note[ctr]);	//Free its memory
	}
	free(gp->track[track]);	//Free the pro guitar track
	gp->track[track] = tp;	//Insert the new pro guitar track into the array

	//Cleanup
	free(measuremap);
	free(working_num_of_repeats);
	if(unwrapevents)
	{	//If text events were unwrapped
		for(ctr = 0; ctr < gp->text_events; ctr++)
		{	//For each of the wrapped text events
			free(gp->text_event[ctr]);	//Free it
		}
		memcpy(gp->text_event, newevent, sizeof(newevent));	//Clone the unwrapped text event array
		gp->text_events = newevents;	//Update the events counter
	}

	eof_log("\tGP track successfully unwrapped", 1);
	return 0;
}

char eof_copy_notes_in_beat_range(EOF_PRO_GUITAR_TRACK *source, unsigned long startbeat, unsigned long numbeats, EOF_PRO_GUITAR_TRACK *dest, unsigned long destbeat)
{
	unsigned long ctr;
	long beatnum, endbeatnum, newpos, newend;
	float notepos, noteendpos;

	eof_log("eof_copy_notes_in_beat_range() entered", 1);

	if(!source || !dest || (startbeat + numbeats >= eof_song->beats) || (destbeat + numbeats >= eof_song->beats))
	{
		eof_log("\tInvalid parameters", 1);
		return 0;	//Return error
	}

	for(ctr = 0; ctr < source->notes; ctr++)
	{	//For each note in the source track
		if((source->note[ctr]->pos >= eof_song->beat[startbeat]->pos) && (source->note[ctr]->pos < eof_song->beat[startbeat + numbeats]->pos))
		{	//If this note is positioned within the target range of beats
			beatnum = eof_get_beat(eof_song, source->note[ctr]->pos);					//Find which beat this note is within
			endbeatnum = eof_get_beat(eof_song, source->note[ctr]->pos + source->note[ctr]->length);	//Find which beat this note ends within
			if((beatnum >= 0) && (endbeatnum >= 0))
			{	//The beat positions were found
				if(eof_song->beats < destbeat + endbeatnum - startbeat + 2)
				{
#ifdef GP_IMPORT_DEBUG
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tUnwrapping note #%lu requires %lu beats, but project only has %lu beats.  Appending %lu beats to the project to allow for this note and 2 bytes of padding.", ctr, destbeat + endbeatnum - startbeat, eof_song->beats, destbeat + endbeatnum - startbeat + 2 - eof_song->beats);
					eof_log(eof_log_string, 1);
#endif
					while(eof_song->beats < destbeat + endbeatnum - startbeat + 2)
					{	//Until the loaded project has enough beats to contain the unwrapped note, and two extra to allow room for processing beat lengths later
						double beat_length;

						if(!eof_song_add_beat(eof_song))
						{
							eof_log("\tError allocating memory to unwrap GP track (6)", 1);
							return 0;	//Return error
						}
						eof_song->beat[eof_song->beats - 1]->ppqn = eof_song->beat[eof_song->beats - 2]->ppqn;	//Match the tempo of the previously last beat
						beat_length = (double)60000.0 / ((double)60000000.0 / (double)eof_song->beat[eof_song->beats - 2]->ppqn);	//Get the length of the previously last beat
						eof_song->beat[eof_song->beats - 1]->fpos = eof_song->beat[eof_song->beats - 2]->fpos + beat_length;
						eof_song->beat[eof_song->beats - 1]->pos = eof_song->beat[eof_song->beats - 1]->fpos + 0.5;	//Round up
					}
					eof_chart_length = eof_song->beat[eof_song->beats - 1]->pos;	//Alter the chart length so that the full transcription will display
				}

				notepos = eof_get_porpos(source->note[ctr]->pos);									//Get the note's position as a percentage within its beat
				noteendpos = eof_get_porpos(source->note[ctr]->pos + source->note[ctr]->length);	//Get the note end's end position as a percentage within its beat
				newpos = eof_put_porpos(destbeat + beatnum - startbeat, notepos, 0.0);				//Get the position for the copied note
				newend = eof_put_porpos(destbeat + endbeatnum - startbeat, noteendpos, 0.0);		//Get the end position for the copied note
				if((newpos < 0) || (newend < 0))
				{	//If the positioning for the copied note couldn't be determined
					eof_log("\tError finding position for unwrapped note", 1);
					return 0;	//Return error
				}
				if(dest->notes >= EOF_MAX_NOTES)
				{	//If the track can't hold any more notes
					eof_log("\tNote limit exceeded", 1);
					return 0;	//Return error
				}
				dest->note[dest->notes] = malloc(sizeof(EOF_PRO_GUITAR_NOTE));	//Allocate memory for the copied note
				if(!dest->note[dest->notes])
				{
					eof_log("\tError allocating memory", 1);
					return 0;	//Return error
				}
				memcpy(dest->note[dest->notes], source->note[ctr], sizeof(EOF_PRO_GUITAR_NOTE));	//Copy the note
				dest->note[dest->notes]->pos = newpos;				//Set the unwrapped note's position
				dest->note[dest->notes]->length = newend - newpos;	//Set its length
				dest->notes++;	//Increment the destination track's note counter
			}
		}
	}

	return 1;	//Return success
}

int eof_get_next_gpa_sync_point(char **buffer, struct eof_gpa_sync_point *ptr)
{
	unsigned long ctr, index;
	char buffer2[21];
	double value;

	if(!buffer || !(*buffer) || !ptr)
		return 0;	//Invalid parameters

	//Examine the character at the specified position to see if it needs to be skipped
	if(**buffer == '#')
		(*buffer)++;	//Skip the timestamp delimiter
	else if(!isdigit(**buffer))
		return 0;	//Unexpected character was encountered

	//Read a set of 4 delimited numbers
	for(ctr = 0; ctr < 4; ctr++)
	{	//For each of the 4 expected numbers in the timestamp
		//Read the number and convert to floating point
		for(index = 0; index < 20; index++)
		{	//Read up to 20 characters for this number
			if(**buffer == '\0')
				return 0;	//The end of the string was unexpectedly reached
			if((**buffer == '<') || (**buffer == '#'))
			{	//If the end of the timestamp is reached
				if(ctr < 3)
				{	//If four input fields haven't been read yet
					return 0;	//Malformed timestamp
				}
				break;	//Otherwise stop parsing this number
			}
			if(**buffer == ';')
			{	//If the end of one of the first 3 numbers is reached
				if(index < 1)
				{	//If no characters were read for this number
					return 0;	//Malformed timestamp
				}
				(*buffer)++;	//Increment to next character to read
				break;	//Otherwise stop parsing this number
			}
			if(!isdigit(**buffer) && (**buffer != '.'))
			{	//If the next character isn't a number or a period
				return 0;	//Malformed timestamp
			}
			buffer2[index] = **buffer;	//Read the next character
			(*buffer)++;	//Increment to next character to read
		}
		if(index >= 20)
		{	//If the number field was too long to parse
			return 0;	//Malformed timestamp
		}
		buffer2[index] = '\0';	//Terminate the string
		value = atof(buffer2);	//Convert to floating point

		//Store the value into the appropriate structure element
		if(!ctr)
		{
			ptr->realtime_pos = value + 0.5;	//Round up to nearest ms
		}
		else if(ctr == 1)
		{
			ptr->measure = value + 0.5;	//Round up to nearest number to avoid precision loss
		}
		else if(ctr == 2)
		{
			ptr->pos_in_measure = value;
		}
		else
		{	//ctr is 3
			ptr->beat_length = value;
		}
	}//For each of the 4 expected numbers in the timestamp

	return 1;	//Return success
}

#endif
