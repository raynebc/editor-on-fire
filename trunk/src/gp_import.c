#ifdef EOF_BUILD
#include <allegro.h>
#include <assert.h>
#include "beat.h"
#include "main.h"
#include "midi.h"
#include "song.h"
#include "tuning.h"
#include "undo.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "gp_import.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

#ifndef EOF_BUILD
char *eof_note_names[12] =	{"A","Bb","B","C","Db","D","Eb","E","F","Gb","G","Ab"};		//The name of musical note 0 (0 semitones above A) to 11 (11 semitones above A)
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
	pack_fread(buffer, len, inf);	//Read the string
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
}
#endif

void eof_gp_parse_bend(PACKFILE *inf)
{
	unsigned word;
	unsigned long height, points, ctr, dword;

	if(!inf)
		return;

	eof_gp_debug_log(inf, "\tBend:  ");
	word = pack_getc(inf);	//Read bend type
	if(word == 1)
	{
		puts("Bend");
	}
	else if(word == 2)
	{
		puts("Bend and release");
	}
	else if(word == 3)
	{
		puts("Bend, release and bend");
	}
	else if(word == 4)
	{
		puts("Pre bend");
	}
	else if(word == 5)
	{
		puts("Pre bend and release");
	}
	else if(word == 6)
	{
		puts("Tremolo dip");
	}
	else if(word == 7)
	{
		puts("Tremolo dive");
	}
	else if(word == 8)
	{
		puts("Tremolo release up");
	}
	else if(word == 9)
	{
		puts("Tremolo inverted dip");
	}
	else if(word == 10)
	{
		puts("Tremolo return");
	}
	else if(word == 11)
	{
		puts("Tremolo release down");
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
			puts("\aEnd of file reached unexpectedly");
			abort();
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
			puts("none");
		}
		else if(word == 1)
		{
			puts("fast");
		}
		else if(word == 2)
		{
			puts("average");
		}
		else if(word == 3)
		{
			puts("slow");
		}
	}
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
	eof_read_gp_string(inf, &word, buffer, 0);	//Read file version string
	puts(buffer);
	eof_gp_debug_log(inf, "(skipping ");
	printf("%d bytes of padding)\n", 30 - word);
	pack_fseek(inf, 30 - word);	//Skip the padding that follows the version string
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
		puts("File format version not supported");
		pack_fclose(inf);
		return NULL;
	}

	eof_gp_debug_log(inf, "Title:  ");
	eof_read_gp_string(inf, NULL, buffer, 1);	//Read title string
	puts(buffer);
	eof_gp_debug_log(inf, "Subtitle:  ");
	eof_read_gp_string(inf, NULL, buffer, 1);	//Read subtitle string
	puts(buffer);
	eof_gp_debug_log(inf, "Artist:  ");
	eof_read_gp_string(inf, NULL, buffer, 1);	//Read artist string
	puts(buffer);
	eof_gp_debug_log(inf, "Album:  ");
	eof_read_gp_string(inf, NULL, buffer, 1);	//Read album string
	puts(buffer);

	if(fileversion >= 500)
	{	//The words field only exists in version 5.x or higher versions of the format
		eof_gp_debug_log(inf, "Lyricist:  ");
		eof_read_gp_string(inf, NULL, buffer, 1);	//Read words string
		puts(buffer);
	}

	eof_gp_debug_log(inf, "Composer:  ");
	eof_read_gp_string(inf, NULL, buffer, 1);	//Read music string
	puts(buffer);
	eof_gp_debug_log(inf, "Copyright:  ");
	eof_read_gp_string(inf, NULL, buffer, 1);	//Read copyright string
	puts(buffer);
	eof_gp_debug_log(inf, "Transcriber:  ");
	eof_read_gp_string(inf, NULL, buffer, 1);	//Read tab string
	puts(buffer);
	eof_gp_debug_log(inf, "Instructions:  ");
	eof_read_gp_string(inf, NULL, buffer, 1);	//Read instructions string
	puts(buffer);
	eof_gp_debug_log(inf, "Number of notice entries:  ");
	pack_ReadDWORDLE(inf, &dword);	//Read the number of notice entries
	printf("%lu\n", dword);
	while(dword > 0)
	{	//Read each notice entry
		eof_gp_debug_log(inf, "Notice:  ");
		eof_read_gp_string(inf, NULL, buffer, 1);	//Read notice string
		puts(buffer);
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
			buffer2 = malloc(dword + 1);	//Allocate a buffer large enough for the lyric string (plus a NULL terminator)
			if(!buffer2)
			{
				puts("Error allocating memory (9)");
				pack_fclose(inf);
				return 0;
			}
			pack_fread(buffer2, dword, inf);	//Read the lyric string
			buffer2[dword] = '\0';		//Terminate the string
			puts(buffer2);
			free(buffer2);	//Free the buffer
		}
	}

	if(fileversion > 500)
	{	//The volume/equalization settings only exist in versions newer than 5.0 of the format
		puts("\tVolume/equalization settings:");
		eof_gp_debug_log(inf, "Master volume:  ");
		pack_ReadDWORDLE(inf, &dword);	//Read the master volume
		printf("%lu\n", dword);
		eof_gp_debug_log(inf, "(skipping 4 bytes of unknown data)\n");
		pack_fseek(inf, 4);		//Unknown data
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
		puts("\tPage setup settings:");
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
		eof_gp_debug_log(inf, "Header/footer fields bitmask:   ");
		pack_ReadWORDLE(inf, &word);	//Read the enabled header/footer fields bitmask
		printf("%u\n", word);
		puts("\tHeader/footer strings:");
		eof_gp_debug_log(inf, "\tTitle ");
		eof_read_gp_string(inf, NULL, buffer, 1);	//Read the title header/footer string
		printf("(%s):  %s\n", (word & 1) ? "enabled" : "disabled", buffer);
		eof_gp_debug_log(inf, "\tSubtitle ");
		eof_read_gp_string(inf, NULL, buffer, 1);	//Read the subtitle header/footer string
		printf("(%s):  %s\n", (word & 2) ? "enabled" : "disabled", buffer);
		eof_gp_debug_log(inf, "\tArtist ");
		eof_read_gp_string(inf, NULL, buffer, 1);	//Read the artist header/footer string
		printf("(%s):  %s\n", (word & 4) ? "enabled" : "disabled", buffer);
		eof_gp_debug_log(inf, "\tAlbum ");
		eof_read_gp_string(inf, NULL, buffer, 1);	//Read the album header/footer string
		printf("(%s):  %s\n", (word & 8) ? "enabled" : "disabled", buffer);
		eof_gp_debug_log(inf, "\tLyricist ");
		eof_read_gp_string(inf, NULL, buffer, 1);	//Read the words (lyricist) header/footer string
		printf("(%s):  %s\n", (word & 16) ? "enabled" : "disabled", buffer);
		eof_gp_debug_log(inf, "\tComposer ");
		eof_read_gp_string(inf, NULL, buffer, 1);	//Read the music (composer) header/footer string
		printf("(%s):  %s\n", (word & 32) ? "enabled" : "disabled", buffer);
		eof_gp_debug_log(inf, "\tLyricist and Composer ");
		eof_read_gp_string(inf, NULL, buffer, 1);	//Read the words & music header/footer string
		printf("(%s):  %s\n", (word & 64) ? "enabled" : "disabled", buffer);
		eof_gp_debug_log(inf, "\tCopyright 1 ");
		eof_read_gp_string(inf, NULL, buffer, 1);	//Read the Copyright line 1 header/footer string
		printf("(%s):  %s\n", (word & 128) ? "enabled" : "disabled", buffer);
		eof_gp_debug_log(inf, "\tCopyright 2 ");
		eof_read_gp_string(inf, NULL, buffer, 1);	//Read the Copyright line 2 header/footer string
		printf("(%s):  %s\n", (word & 128) ? "enabled" : "disabled", buffer);
		eof_gp_debug_log(inf, "\tPage number ");
		eof_read_gp_string(inf, NULL, buffer, 1);	//Read the page number header/footer string
		printf("(%s):  %s\n", (word & 256) ? "enabled" : "disabled", buffer);
	}

	eof_gp_debug_log(inf, "Tempo:  ");
	if(fileversion >= 500)
	{	//The tempo string only exists in version 5.x or newer of the format
		eof_read_gp_string(inf, NULL, buffer, 1);	//Read the tempo string
	}
	pack_ReadDWORDLE(inf, &dword);	//Read the tempo
	printf("%luBPM", dword);
	if((fileversion >= 500) && (buffer[0] != '\0'))
	{	//The tempo string only exists in version 5.x or newer of the format
		printf(" (%s)", buffer);
	}
	putchar('\n');

	if(fileversion > 500)
	{	//There is a byte of unknown data/padding here in versions newer than 5.0 of the format
		eof_gp_debug_log(inf, "(skipping 1 byte of unknown data)\n");
		pack_fseek(inf, 1);		//Unknown data
	}

	if(fileversion >= 400)
	{	//Versions 4.0 and newer of the format store key and octave information here
		eof_gp_debug_log(inf, "Key:  ");
		byte = pack_getc(inf);	//Read the key
		printf("%d (%u %s)\n", byte, abs(byte), (byte < 0) ? "flats" : "sharps");
		eof_gp_debug_log(inf, "(skipping 3 bytes of unknown data)\n");
		pack_fseek(inf, 3);		//Unknown data
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
		pack_getc(inf);					//Read the volume
		pack_getc(inf);					//Read the pan value
		pack_getc(inf);					//Read the chorus value
		pack_getc(inf);					//Read the reverb value
		pack_getc(inf);					//Read the phaser value
		pack_getc(inf);					//Read the tremolo value
		pack_ReadWORDLE(inf, NULL);		//Read two bytes of unknown data/padding
	}

	if(fileversion >= 500)
	{	//Versions 5.0 and newer of the format store musical directional symbols and a master reverb setting here
		puts("\tMusical symbols:");
		eof_gp_debug_log(inf, "\"Coda\" symbol is ");
		pack_ReadWORDLE(inf, &word);
		if(word == 0xFFFF)
		{
			puts("unused");
		}
		else
		{
			printf("at beat #%u\n", word);
		}
		eof_gp_debug_log(inf, "\"Double coda\" symbol is ");
		pack_ReadWORDLE(inf, &word);
		if(word == 0xFFFF)
		{
			puts("unused");
		}
		else
		{
			printf("at beat #%u\n", word);
		}
		eof_gp_debug_log(inf, "\"Segno\" symbol is ");
		pack_ReadWORDLE(inf, &word);
		if(word == 0xFFFF)
		{
			puts("unused");
		}
		else
		{
			printf("at beat #%u\n", word);
		}
		eof_gp_debug_log(inf, "\"Segno segno\" symbol is ");
		pack_ReadWORDLE(inf, &word);
		if(word == 0xFFFF)
		{
			puts("unused");
		}
		else
		{
			printf("at beat #%u\n", word);
		}
		eof_gp_debug_log(inf, "\"Fine\" symbol is ");
		pack_ReadWORDLE(inf, &word);
		if(word == 0xFFFF)
		{
			puts("unused");
		}
		else
		{
			printf("at beat #%u\n", word);
		}
		eof_gp_debug_log(inf, "\"Da capo\" symbol is ");
		pack_ReadWORDLE(inf, &word);
		if(word == 0xFFFF)
		{
			puts("unused");
		}
		else
		{
			printf("at beat #%u\n", word);
		}
		eof_gp_debug_log(inf, "\"Da capo al coda\" symbol is ");
		pack_ReadWORDLE(inf, &word);
		if(word == 0xFFFF)
		{
			puts("unused");
		}
		else
		{
			printf("at beat #%u\n", word);
		}
		eof_gp_debug_log(inf, "\"Da capo al double coda\" symbol is ");
		pack_ReadWORDLE(inf, &word);
		if(word == 0xFFFF)
		{
			puts("unused");
		}
		else
		{
			printf("at beat #%u\n", word);
		}
		eof_gp_debug_log(inf, "\"Da capo al fine\" symbol is ");
		pack_ReadWORDLE(inf, &word);
		if(word == 0xFFFF)
		{
			puts("unused");
		}
		else
		{
			printf("at beat #%u\n", word);
		}
		eof_gp_debug_log(inf, "\"Da segno\" symbol is ");
		pack_ReadWORDLE(inf, &word);
		if(word == 0xFFFF)
		{
			puts("unused");
		}
		else
		{
			printf("at beat #%u\n", word);
		}
		eof_gp_debug_log(inf, "\"Da segno al coda\" symbol is ");
		pack_ReadWORDLE(inf, &word);
		if(word == 0xFFFF)
		{
			puts("unused");
		}
		else
		{
			printf("at beat #%u\n", word);
		}
		eof_gp_debug_log(inf, "\"Da segno al double coda\" symbol is ");
		pack_ReadWORDLE(inf, &word);
		if(word == 0xFFFF)
		{
			puts("unused");
		}
		else
		{
			printf("at beat #%u\n", word);
		}
		eof_gp_debug_log(inf, "\"Da segno al fine\" symbol is ");
		pack_ReadWORDLE(inf, &word);
		if(word == 0xFFFF)
		{
			puts("unused");
		}
		else
		{
			printf("at beat #%u\n", word);
		}
		eof_gp_debug_log(inf, "\"Da segno segno\" symbol is ");
		pack_ReadWORDLE(inf, &word);
		if(word == 0xFFFF)
		{
			puts("unused");
		}
		else
		{
			printf("at beat #%u\n", word);
		}
		eof_gp_debug_log(inf, "\"Da segno segno al coda\" symbol is ");
		pack_ReadWORDLE(inf, &word);
		if(word == 0xFFFF)
		{
			puts("unused");
		}
		else
		{
			printf("at beat #%u\n", word);
		}
		eof_gp_debug_log(inf, "\"Da segno segno al double coda\" symbol is ");
		pack_ReadWORDLE(inf, &word);
		if(word == 0xFFFF)
		{
			puts("unused");
		}
		else
		{
			printf("at beat #%u\n", word);
		}
		eof_gp_debug_log(inf, "\"Da segno segno al fine\" symbol is ");
		pack_ReadWORDLE(inf, &word);
		if(word == 0xFFFF)
		{
			puts("unused");
		}
		else
		{
			printf("at beat #%u\n", word);
		}
		eof_gp_debug_log(inf, "\"Da coda\" symbol is ");
		pack_ReadWORDLE(inf, &word);
		if(word == 0xFFFF)
		{
			puts("unused");
		}
		else
		{
			printf("at beat #%u\n", word);
		}
		eof_gp_debug_log(inf, "\"Da double coda\" symbol is ");
		pack_ReadWORDLE(inf, &word);
		if(word == 0xFFFF)
		{
			puts("unused");
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
		puts("Error allocating memory (10)");
		pack_fclose(inf);
		return NULL;
	}

	for(ctr = 0; ctr < measures; ctr++)
	{	//For each measure
		printf("\tStart of definition for measure #%lu\n", ctr + 1);
		eof_gp_debug_log(inf, "\tMeasure bitmask:  ");
		bytemask = pack_getc(inf);	//Read the measure bitmask
		printf("%u\n", (bytemask & 0xFF));
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
		if(bytemask & 32)
		{	//New section
			eof_gp_debug_log(inf, "\tNew section:  ");
			eof_read_gp_string(inf, NULL, buffer, 1);	//Read section string
			printf("%s (coloring:  ", buffer);
			word = pack_getc(inf);
			printf("R = %u, ", word);
			word = pack_getc(inf);
			printf("G = %u, ", word);
			word = pack_getc(inf);
			printf("B = %u)\n", word);
			pack_getc(inf);	//Read unused value
		}
		if(bytemask & 64)
		{	//Key signature change
			eof_gp_debug_log(inf, "\tNew key signature:  ");
			byte = pack_getc(inf);	//Read the key
			printf("%d (%u %s, ", byte, abs(byte), (byte < 0) ? "flats" : "sharps");
			byte = pack_getc(inf);	//Read the major/minor byte
			printf("%s)\n", !byte ? "major" : "minor");
		}
		if((fileversion >= 500) && ((bytemask & 1) || (bytemask & 2)))
		{	//If either a new TS numerator or denominator was set, read the beam by eight notes values (only for version 5.x and higher of the format.  3.x/4.x are known to not have this info)
			char byte1, byte2, byte3, byte4;
			eof_gp_debug_log(inf, "\tBeam eight notes by:  ");
			byte1 = pack_getc(inf);
			byte2 = pack_getc(inf);
			byte3 = pack_getc(inf);
			byte4 = pack_getc(inf);
			printf("%d + %d + %d + %d = %d\n", byte1, byte2, byte3, byte4, byte1 + byte2 + byte3 + byte4);
		}
		if(bytemask & 8)
		{	//End of repeat
			eof_gp_debug_log(inf, "\tEnd of repeat:  ");
			word = pack_getc(inf);	//Read number of repeats
			printf("%u repeats\n", word);
		}
		if(bytemask & 128)
		{	//Double bar
			puts("\t\t(Double bar)");
		}

		if(fileversion >= 500)
		{	//Versions 5.0 and newer of the format store unknown data/padding here
			if(bytemask & 16)
			{	//Number of alternate ending
				eof_gp_debug_log(inf, "\tNumber of alternate ending:  ");
				word = pack_getc(inf);	//Read alternate ending number
				printf("%u\n", word);
			}
			else
			{
				eof_gp_debug_log(inf, "\t(skipping 1 byte of unused alternate ending data)\n");
				pack_getc(inf);			//Unknown data
			}
			eof_gp_debug_log(inf, "\tTriplet feel:  ");
			byte = pack_getc(inf);	//Read triplet feel value
			printf("%s\n", !byte ? "none" : ((byte == 1) ? "Triplet 8th" : "Triplet 16th"));
			eof_gp_debug_log(inf, "\t(skipping 1 byte of unknown data)\n");
			pack_getc(inf);			//Unknown data
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
			puts("\t\t\t(Is a drum track)");
		}
		if(bytemask & 2)
		{
			puts("\t\t\t(Is a 12 string guitar track)");
		}
		if(bytemask & 4)
		{
			puts("\t\t\t(Is a banjo track)");
		}
		if(bytemask & 16)
		{
			puts("\t\t\t(Is marked for solo playback)");
		}
		if(bytemask & 32)
		{
			puts("\t\t\t(Is marked for muted playback)");
		}
		if(bytemask & 64)
		{
			puts("\t\t\t(Is marked for RSE playback)");
		}
		if(bytemask & 128)
		{
			puts("\t\t\t(Is set to have the tuning displayed)");
		}
		eof_gp_debug_log(inf, "\tTrack name string:  ");
		eof_read_gp_string(inf, &word, buffer, 0);	//Read track name string
		puts(buffer);
		eof_gp_debug_log(inf, "\t(skipping ");
		printf("%d bytes of padding)\n", 40 - word);
		pack_fseek(inf, 40 - word);			//Skip the padding that follows the track name string
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
			puts("(none)");
		}
		eof_gp_debug_log(inf, "\tTrack color:  ");
		word = pack_getc(inf);
		printf("R = %u, ", word);
		word = pack_getc(inf);
		printf("G = %u, ", word);
		word = pack_getc(inf);
		printf("B = %u\n", word);
		pack_getc(inf);	//Read unused value

		if(fileversion > 500)
		{
			eof_gp_debug_log(inf, "\tTrack properties 1 bitmask:  ");
			bytemask = pack_getc(inf);
			printf("%u\n", (bytemask & 0xFF));
			eof_gp_debug_log(inf, "\tTrack properties 2 bitmask:  ");
			bytemask2 = pack_getc(inf);
			printf("%u\n", (bytemask2 & 0xFF));
			eof_gp_debug_log(inf, "\t(skipping 1 byte of unknown data)\n");
			pack_getc(inf);			//Unknown data
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
			pack_fseek(inf, 31);		//Unknown data
			eof_gp_debug_log(inf, "\tSelected sound bank option:  ");
			word = pack_getc(inf);
			printf("%u\n", word);
			eof_gp_debug_log(inf, "\t(skipping 7 bytes of unknown data)\n");
			pack_fseek(inf, 7);		//Unknown data
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
			eof_read_gp_string(inf, NULL, buffer, 1);	//Read track instrument effect 1
			puts(buffer);
			eof_gp_debug_log(inf, "\tTrack instrument effect 2:  ");
			eof_read_gp_string(inf, NULL, buffer, 1);	//Read track instrument effect 2
			puts(buffer);
		}
		else if(fileversion == 500)
		{
			eof_gp_debug_log(inf, "\t(skipping 45 bytes of unknown data)\n");
			pack_fseek(inf, 45);		//Unknown data
		}
	}//For each track

	if(fileversion >= 500)
	{
		eof_gp_debug_log(inf, "\t(skipping 1 byte of unknown data)\n");
		pack_getc(inf);
	}

	puts("\tStart of beat definitions:");
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
						puts("\t\t(Dotted note)");
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
							eof_read_gp_string(inf, NULL, buffer, 1);	//Read chord name
							puts(buffer);
							eof_gp_debug_log(inf, "\t\tDiagram begins at fret #");
							pack_ReadDWORDLE(inf, &dword);	//Read the diagram fret position
							printf("%lu\n", dword);
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
						}//Chord diagram format 0, ie. GP3
						else if(word == 1)
						{	//Chord diagram format 1, ie. GP4
							eof_gp_debug_log(inf, "\t\tDisplay as ");
							word = pack_getc(inf);	//Read sharp/flat indicator
							printf("%s\n", !word ? "flat" : "sharp");
							eof_gp_debug_log(inf, "\t\t(skipping 3 bytes of unknown data)\n");
							pack_fseek(inf, 3);		//Unknown data
							eof_gp_debug_log(inf, "\t\tChord root:  ");
							word = pack_getc(inf);	//Read chord root
							printf("%u\n", word);
							if(fileversion / 100 == 3)
							{	//If it is a GP 3.x file
								eof_gp_debug_log(inf, "\t\t(skipping 3 bytes of unknown data)\n");
								pack_fseek(inf, 3);		//Unknown data
							}
							eof_gp_debug_log(inf, "\t\tChord type:  ");
							word = pack_getc(inf);	//Read chord type
							printf("%u\n", word);
							if(fileversion / 100 == 3)
							{	//If it is a GP 3.x file
								eof_gp_debug_log(inf, "\t\t(skipping 3 bytes of unknown data)\n");
								pack_fseek(inf, 3);		//Unknown data
							}
							eof_gp_debug_log(inf, "\t\t9th/11th/13th option:  ");
							word = pack_getc(inf);
							printf("%u\n", word);
							if(fileversion / 100 == 3)
							{	//If it is a GP 3.x file
								eof_gp_debug_log(inf, "\t\t(skipping 3 bytes of unknown data)\n");
								pack_fseek(inf, 3);		//Unknown data
							}
							eof_gp_debug_log(inf, "\t\tBass note:  ");
							pack_ReadDWORDLE(inf, &dword);	//Read lowest note played in string
							printf("%lu (%s)\n", dword, eof_note_names[(dword + 3) % 12]);
							eof_gp_debug_log(inf, "\t\t+/- option:  ");
							word = pack_getc(inf);
							printf("%u\n", word);
							eof_gp_debug_log(inf, "\t\t(skipping 4 bytes of unknown data)\n");
							pack_fseek(inf, 4);		//Unknown data
							eof_gp_debug_log(inf, "\t\tChord name:  ");
							word = pack_getc(inf);	//Read chord name string length
							pack_fread(buffer, 20, inf);	//Read chord name (which is padded to 20 bytes)
							buffer[word] = '\0';	//Ensure string is terminated to be the right length
							puts(buffer);
							eof_gp_debug_log(inf, "\t\t(skipping 2 bytes of unknown data)\n");
							pack_fseek(inf, 2);		//Unknown data
							eof_gp_debug_log(inf, "\t\tTonality of the fifth:  ");
							byte = pack_getc(inf);
							printf("%s\n", !byte ? "perfect" : ((byte == 1) ? "augmented" : "diminished"));
							if(fileversion / 100 == 3)
							{	//If it is a GP 3.x file
								eof_gp_debug_log(inf, "\t\t(skipping 3 bytes of unknown data)\n");
								pack_fseek(inf, 3);		//Unknown data
							}
							eof_gp_debug_log(inf, "\t\tTonality of the ninth:  ");
							byte = pack_getc(inf);
							printf("%s\n", !byte ? "perfect" : ((byte == 1) ? "augmented" : "diminished"));
							if(fileversion / 100 == 3)
							{	//If it is a GP 3.x file
								eof_gp_debug_log(inf, "\t\t(skipping 3 bytes of unknown data)\n");
								pack_fseek(inf, 3);		//Unknown data
							}
							eof_gp_debug_log(inf, "\t\tTonality of the eleventh:  ");
							byte = pack_getc(inf);
							printf("%s\n", !byte ? "perfect" : ((byte == 1) ? "augmented" : "diminished"));
							if(fileversion / 100 == 3)
							{	//If it is a GP 3.x file
								eof_gp_debug_log(inf, "\t\t(skipping 3 bytes of unknown data)\n");
								pack_fseek(inf, 3);		//Unknown data
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
									pack_getc(inf);
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
									pack_getc(inf);
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
									pack_getc(inf);
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
							pack_getc(inf);	//Unknown data
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
									pack_getc(inf);		//Skip this padding
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
						eof_read_gp_string(inf, NULL, buffer, 1);	//Read beat text string
						puts(buffer);
					}
					if(bytemask & 8)
					{	//Beat has effects
						unsigned char byte1 = 0, byte2 = 0;

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
								puts("\t\tRasguedo");
							}
						}
						if(byte1 & 1)
						{	//Vibrato
							puts("\t\t(Vibrato)");
						}
						if(byte1 & 2)
						{
							puts("\t\t(Wide vibrato)");
						}
						if(byte1 & 4)
						{
							puts("\t\t(Natural harmonic)");
						}
						if(byte1 & 8)
						{
							puts("\t\t(Artificial harmonic)");
						}
						if(byte1 & 32)
						{	//Tapping/popping/slapping
							eof_gp_debug_log(inf, "\tString effect:  ");
							byte = pack_getc(inf);	//Read tapping/popping/slapping indicator
							if(byte == 0)
							{
								puts("Tremolo");
							}
							else if(byte == 1)
							{
								puts("Tapping");
							}
							else if(byte == 2)
							{
								puts("Slapping");
							}
							else if(byte == 3)
							{
								puts("Popping");
							}
							else
							{
								puts("Unknown");
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
							eof_gp_parse_bend(inf);
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

						puts("\t\tBeat mix table change:");
						eof_gp_debug_log(inf, "\t\tNew instrument number:  ");
							byte = pack_getc(inf);
						if(byte == -1)
						{
							puts("(no change)");
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
							pack_fseek(inf, 4);		//Unknown data
						}
						eof_gp_debug_log(inf, "\t\tNew volume:  ");
						byte = pack_getc(inf);
						if(byte == -1)
						{
							puts("(no change)");
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
							puts("(no change)");
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
							puts("(no change)");
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
							puts("(no change)");
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
							puts("(no change)");
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
							puts("(no change)");
						}
						else
						{
							tremolo_change = 1;
							printf("%d\n", byte);
						}
						if(fileversion >= 500)
						{	//These fields are only in version 5.x files
							eof_gp_debug_log(inf, "\t\tTempo text string:  ");
							eof_read_gp_string(inf, NULL, buffer, 1);	//Read the tempo text string
							puts(buffer);
						}
						eof_gp_debug_log(inf, "\t\tNew tempo:  ");
						pack_ReadDWORDLE(inf, &dword);
						if((long)dword == -1)
						{
							puts("(no change)");
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
							pack_fseek(inf, 1);		//Unknown data
						}
						if(fileversion > 500)
						{	//These strings are only in versions newer than 5.0 of the format
							eof_gp_debug_log(inf, "\t\tEffect 2 string:  ");
							eof_read_gp_string(inf, NULL, buffer, 1);	//Read the Effect 2 string
							puts(buffer);
							eof_gp_debug_log(inf, "\t\tEffect 1 string:  ");
							eof_read_gp_string(inf, NULL, buffer, 1);	//Read the Effect 1 string
							puts(buffer);
						}
					}//Beat has mix table change
					eof_gp_debug_log(inf, "\tUsed strings bitmask:  ");
					usedstrings = pack_getc(inf);
					printf("%u\n", (usedstrings & 0xFF));
					unsigned bitmask;
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
								pack_fseek(inf, 8);		//Unknown data
							}
							if(fileversion >= 500)
							{	//This padding isn't in version 3.x and 4.x files
								eof_gp_debug_log(inf, "\t\t(skipping 1 byte of unknown data)\n");
								pack_fseek(inf, 1);		//Unknown data
							}
							if(bytemask & 8)
							{	//Note effects
								char byte1 = 0, byte2 = 0;
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
									eof_gp_parse_bend(inf);
								}
								if(byte1 & 2)
								{
									puts("\t\t\t\t(Hammer on/pull off from current note)");
								}
								if(byte1 & 4)
								{
									puts("\t\t\t\t(Slide from current note)");
								}
								if(byte1 & 8)
								{
									puts("\t\t\t\t(Let ring)");
								}
								if(byte1 & 16)
								{	//Grace note
									puts("\t\t\t\t(Grace note)");
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
											puts("none");
										}
										else if(byte == 1)
										{
											puts("slide");
										}
										else if(byte == 2)
										{
											puts("bend");
										}
										else if(byte == 3)
										{
											puts("hammer");
										}
									}
									else
									{	//The purpose of this field in 4.x or older files is unknown
										eof_gp_debug_log(inf, "\t\t(skipping 1 byte of unknown data)\n");
										pack_fseek(inf, 1);		//Unknown data
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
											puts("\t\t\t\t\t(dead note)");
										}
										printf("\t\t\t\t\t%s\n", (byte & 2) ? "(on the beat)" : "(before the beat)");
									}
								}
								if(byte2 & 1)
								{
									puts("\t\t\t\t(Note played staccato");
								}
								if(byte2 & 2)
								{
									puts("\t\t\t\t(Palm mute");
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
										puts("\t\t\t\t\t(Artificial harmonic)");
										eof_gp_debug_log(inf, "\t\t\t\tHarmonic note:  ");
										byte = pack_getc(inf);	//Read harmonic note
										printf("%s", eof_note_names[(byte + 3) % 12]);
										byte = pack_getc(inf);	//Read sharp/flat status
										if(byte == -1)
										{
											putchar('b');
										}
										else if(byte == 1)
										{
											putchar('#');
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
										putchar('\n');
									}
									else if(byte == 3)
									{	//Tapped harmonic
										puts("\t\t\t\t\t(Tapped harmonic)");
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
									puts("\t\t\t\t(Vibrato)");
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
							pack_fseek(inf, 1);	//Unknown data
						}
					}
				}//For each beat
			}//For each voice
			if(fileversion >= 500)
			{
				eof_gp_debug_log(inf, "\t(skipping 1 byte of unknown data)\n");
				pack_fseek(inf, 1);		//Unknown data
			}
		}//For each track
	}//For each measure


	pack_fclose(inf);
	free(strings);
	puts("\nSuccess");
	return (EOF_SONG *)1;
}

int main(int argc, char *argv[])
{
	EOF_SONG *ptr = NULL;

	if(argc < 2)
	{
		puts("Must pass the GP file as a parameter");
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

struct eof_guitar_pro_struct *eof_load_gp(const char * fn, char *undo_made)
{
	char buffer[256], byte, *buffer2, bytemask, usedstrings;
	unsigned word, fileversion;
	unsigned long dword, ctr, ctr2, ctr3, ctr4, tracks, measures, *strings, beats, barres;
	PACKFILE *inf;
	struct eof_guitar_pro_struct *gp;
	struct eof_gp_time_signature *tsarray;	//Stores all time signatures defined in the file
	EOF_PRO_GUITAR_NOTE **np;	//Will store the last created note for each track (for handling tie notes)
	char *hopo;	//Will store the fret value of the previous note marked as HO/PO (in GP, if note #N is marked for this, note #N+1 is the one that is a HO or PO), otherwise -1, for each track

	eof_log("\tImporting Guitar Pro file", 1);
	eof_log("eof_import_gp() entered", 1);


//Initialize pointers and handles
	if(!fn)
	{
		return NULL;
	}
	inf = pack_fopen(fn, "rb");
	if(!inf)
	{
		return NULL;
	}
	gp = malloc(sizeof(struct eof_guitar_pro_struct));
	if(!gp)
	{
		eof_log("Error allocating memory (1)", 1);
		pack_fclose(inf);
		return NULL;
	}


//Read file version string
	eof_read_gp_string(inf, &word, buffer, 0);	//Read file version string
	pack_fseek(inf, 30 - word);			//Skip the padding that follows the version string
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
		pack_fclose(inf);
		free(gp);
		return NULL;
	}
#ifdef GP_IMPORT_DEBUG
	snprintf(eof_log_string, sizeof(eof_log_string), "\tParsing version %u guitar pro file", fileversion);
	eof_log(eof_log_string, 1);
#endif


//Read past various ignored information
	eof_read_gp_string(inf, NULL, buffer, 1);	//Read title string
	eof_read_gp_string(inf, NULL, buffer, 1);	//Read subtitle string
	eof_read_gp_string(inf, NULL, buffer, 1);	//Read artist string
	eof_read_gp_string(inf, NULL, buffer, 1);	//Read album string
	if(fileversion >= 500)
	{	//The words field only exists in version 5.x or higher versions of the format
		eof_read_gp_string(inf, NULL, buffer, 1);	//Read words string
	}
	eof_read_gp_string(inf, NULL, buffer, 1);	//Read music string
	eof_read_gp_string(inf, NULL, buffer, 1);	//Read copyright string
	eof_read_gp_string(inf, NULL, buffer, 1);	//Read tab string
	eof_read_gp_string(inf, NULL, buffer, 1);	//Read instructions string
	pack_ReadDWORDLE(inf, &dword);				//Read the number of notice entries
	while(dword > 0)
	{	//Read each notice entry
		eof_read_gp_string(inf, NULL, buffer, 1);	//Read notice string
		dword--;
	}
	if(fileversion < 500)
	{	//The shuffle rhythm feel field only exists here in version 4.x or older of the format
		byte = pack_getc(inf);
	}
	if(fileversion >= 400)
	{	//The lyrics fields only exist in version 4.x or newer of the format
		pack_ReadDWORDLE(inf, NULL);	//Read the lyrics' associated track number
		for(ctr = 0; ctr < 5; ctr++)
		{
			pack_ReadDWORDLE(inf, &dword);	//Read the start from bar #
			pack_ReadDWORDLE(inf, &dword);	//Read the lyric string length
			buffer2 = malloc(dword + 1);	//Allocate a buffer large enough for the lyric string (plus a NULL terminator)
			if(!buffer2)
			{
				eof_log("Error allocating memory (2)", 1);
				pack_fclose(inf);
				free(gp);
				return NULL;
			}
			pack_fread(buffer2, dword, inf);	//Read the lyric string
			buffer2[dword] = '\0';				//Terminate the string
			free(buffer2);						//Free the buffer
		}
	}
	if(fileversion > 500)
	{	//The volume/equalization settings only exist in versions newer than 5.0 of the format
		pack_ReadDWORDLE(inf, &dword);	//Read the master volume
		pack_fseek(inf, 4);			//Unknown data
		byte = pack_getc(inf);		//32Hz band lowered
		byte = pack_getc(inf);		//60Hz band lowered
		byte = pack_getc(inf);		//125Hz band lowered
		byte = pack_getc(inf);		//250Hz band lowered
		byte = pack_getc(inf);		//500Hz band lowered
		byte = pack_getc(inf);		//1KHz band lowered
		byte = pack_getc(inf);		//2KHz band lowered
		byte = pack_getc(inf);		//4KHz band lowered
		byte = pack_getc(inf);		//8KHz band lowered
		byte = pack_getc(inf);		//16KHz band lowered
		byte = pack_getc(inf);		//Gain lowered
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
		eof_read_gp_string(inf, NULL, buffer, 1);	//Read the title header/footer string
		eof_read_gp_string(inf, NULL, buffer, 1);	//Read the subtitle header/footer string
		eof_read_gp_string(inf, NULL, buffer, 1);	//Read the artist header/footer string
		eof_read_gp_string(inf, NULL, buffer, 1);	//Read the album header/footer string
		eof_read_gp_string(inf, NULL, buffer, 1);	//Read the words (lyricist) header/footer string
		eof_read_gp_string(inf, NULL, buffer, 1);	//Read the music (composer) header/footer string
		eof_read_gp_string(inf, NULL, buffer, 1);	//Read the words & music header/footer string
		eof_read_gp_string(inf, NULL, buffer, 1);	//Read the Copyright line 1 header/footer string
		eof_read_gp_string(inf, NULL, buffer, 1);	//Read the Copyright line 2 header/footer string
		eof_read_gp_string(inf, NULL, buffer, 1);	//Read the page number header/footer string
	}
	if(fileversion >= 500)
	{	//The tempo string only exists in version 5.x or newer of the format
		eof_read_gp_string(inf, NULL, buffer, 1);	//Read the tempo string
	}
	pack_ReadDWORDLE(inf, &dword);	//Read the tempo
	if(fileversion > 500)
	{	//There is a byte of unknown data/padding here in versions newer than 5.0 of the format
		pack_fseek(inf, 1);		//Unknown data
	}
	if(fileversion >= 400)
	{	//Versions 4.0 and newer of the format store key and octave information here
		byte = pack_getc(inf);	//Read the key
		pack_fseek(inf, 3);		//Unknown data
		word = pack_getc(inf);
	}
	else
	{	//Older versions stored only key information here
		pack_ReadDWORDLE(inf, &dword);	//Read the key
	}
	for(ctr = 0; ctr < 64; ctr++)
	{
		pack_ReadDWORDLE(inf, NULL);	//Read the instrument patch number
		pack_getc(inf);					//Read the volume
		pack_getc(inf);					//Read the pan value
		pack_getc(inf);					//Read the chorus value
		pack_getc(inf);					//Read the reverb value
		pack_getc(inf);					//Read the phaser value
		pack_getc(inf);					//Read the tremolo value
		pack_ReadWORDLE(inf, NULL);		//Read two bytes of unknown data/padding
	}
	if(fileversion >= 500)
	{	//Versions 5.0 and newer of the format store musical directional symbols and a master reverb setting here
		pack_ReadWORDLE(inf, &word);	//Coda symbol position
		pack_ReadWORDLE(inf, &word);	//Double coda symbol position
		pack_ReadWORDLE(inf, &word);	//Segno symbol position
		pack_ReadWORDLE(inf, &word);	//Segno segno symbol position
		pack_ReadWORDLE(inf, &word);	//Fine symbol position
		pack_ReadWORDLE(inf, &word);	//Da capo symbol position
		pack_ReadWORDLE(inf, &word);	//Da capo al coda symbol position
		pack_ReadWORDLE(inf, &word);	//Da capo al double coda symbol position
		pack_ReadWORDLE(inf, &word);	//Da capo al fine symbol position
		pack_ReadWORDLE(inf, &word);	//Da segno symbol position
		pack_ReadWORDLE(inf, &word);	//Da segno symbol position
		pack_ReadWORDLE(inf, &word);	//Da segno al double coda symbol position
		pack_ReadWORDLE(inf, &word);	//Da segno al fine symbol position
		pack_ReadWORDLE(inf, &word);	//Da segno segno symbol position
		pack_ReadWORDLE(inf, &word);	//Da segno segno al coda symbol position
		pack_ReadWORDLE(inf, &word);	//Da segno segno al double coda symbol position
		pack_ReadWORDLE(inf, &word);	//Da segno segno al fine symbol position
		pack_ReadWORDLE(inf, &word);	//Da coda symbol position
		pack_ReadWORDLE(inf, &word);	//Da double coda symbol position
		pack_ReadDWORDLE(inf, &dword);	//Read the master reverb value
	}


//Read track data
	pack_ReadDWORDLE(inf, &measures);	//Read the number of measures
#ifdef GP_IMPORT_DEBUG
	snprintf(eof_log_string, sizeof(eof_log_string), "\tNumber of measures: %lu", measures);
	eof_log(eof_log_string, 1);
#endif
	tsarray = malloc(sizeof(struct eof_gp_time_signature) * measures);	//Allocate memory to store the time signature effective for each measure
	if(!tsarray)
	{
		eof_log("Error allocating memory (3)", 1);
		pack_fclose(inf);
		free(gp);
		return NULL;
	}
	pack_ReadDWORDLE(inf, &tracks);	//Read the number of tracks
#ifdef GP_IMPORT_DEBUG
	snprintf(eof_log_string, sizeof(eof_log_string), "\tNumber of tracks: %lu", tracks);
	eof_log(eof_log_string, 1);
#endif
	gp->numtracks = tracks;
	gp->names = malloc(sizeof(char *) * tracks);	//Allocate memory for track name strings
	np = malloc(sizeof(EOF_PRO_GUITAR_NOTE *) * tracks);	//Allocate memory for the array of last created notes
	hopo = malloc(sizeof(char) * tracks);	//Allocate memory for storing HOPO information
	if(!gp->names || !np || !hopo)
	{
		eof_log("Error allocating memory (4)", 1);
		pack_fclose(inf);
		free(gp);
		free(tsarray);
		return NULL;
	}
	memset(np, 0, sizeof(EOF_PRO_GUITAR_NOTE *) * tracks);	//Set all last created note pointers to NULL
	memset(hopo, -1, sizeof(char) * tracks);	//Set all tracks to have no HOPO status
	gp->track = malloc(sizeof(EOF_PRO_GUITAR_TRACK *) * tracks);	//Allocate memory for pro guitar track pointers
	if(!gp->track)
	{
		eof_log("Error allocating memory (5)", 1);
		pack_fclose(inf);
		free(gp->names);
		free(np);
		free(hopo);
		free(gp);
		free(tsarray);
		return NULL;
	}
	for(ctr = 0; ctr < tracks; ctr++)
	{	//Initialize each pro guitar track
		gp->track[ctr] = malloc(sizeof(EOF_PRO_GUITAR_TRACK));
		if(!gp->track[ctr])
		{
			eof_log("Error allocating memory (6)", 1);
			pack_fclose(inf);
			free(gp->names);
			while(ctr > 0)
			{	//Free all previously allocated track structures
				free(gp->track[ctr - 1]);
				ctr--;
			}
			free(gp->track);	//Free array of track pointers
			free(np);
			free(hopo);
			free(gp);
			free(tsarray);
			return NULL;
		}
		gp->track[ctr]->arpeggios = 0;
		gp->track[ctr]->notes = 0;
		gp->track[ctr]->numfrets = 22;
		gp->track[ctr]->solos = 0;
		gp->track[ctr]->star_power_paths = 0;
		gp->track[ctr]->trills = 0;
		gp->track[ctr]->tremolos = 0;
		gp->track[ctr]->parent = NULL;
	}


//Read measure data
	unsigned char curnum = 4, curden = 4;	//Stores the current time signature (4/4 assumed until one is explicitly defined)
	unsigned long totalbeats = 0;			//Count the total number of beats in the Guitar Pro file's transcription
	//Allocate memory for an array to track the number of strings for each track
	strings = malloc(sizeof(unsigned long) * tracks);
	if(!strings)
	{
		eof_log("Error allocating memory (7)", 1);
		pack_fclose(inf);
		free(gp->names);
		for(ctr = 0; ctr < tracks; ctr++)
		{	//Free all previously allocated track structures
			free(gp->track[ctr]);
		}
		free(np);
		free(hopo);
		free(gp);
		free(tsarray);
		return NULL;
	}
#ifdef GP_IMPORT_DEBUG
	eof_log("\tParsing measure data", 1);
#endif
	for(ctr = 0; ctr < measures; ctr++)
	{	//For each measure
		bytemask = pack_getc(inf);	//Read the measure bitmask
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
			snprintf(eof_log_string, sizeof(eof_log_string), "\t\tTS change at measure %lu:  %u/%u", ctr + 1, curnum, curden);
			eof_log(eof_log_string, 1);
		}
#endif
		if(bytemask & 32)
		{	//New section
			eof_read_gp_string(inf, NULL, buffer, 1);	//Read section string
			word = pack_getc(inf);						//Read section string color (Red intensity)
			word = pack_getc(inf);						//Read section string color (Green intensity)
			word = pack_getc(inf);						//Read section string color (Blue intensity)
			pack_getc(inf);								//Read unused value
		}
		if(bytemask & 64)
		{	//Key signature change
			byte = pack_getc(inf);	//Read the key
			byte = pack_getc(inf);	//Read the major/minor byte
		}
		if((fileversion >= 500) && ((bytemask & 1) || (bytemask & 2)))
		{	//If either a new TS numerator or denominator was set, read the beam by eight notes values (only for version 5.x and higher of the format.  3.x/4.x are known to not have this info)
			pack_getc(inf);
			pack_getc(inf);
			pack_getc(inf);
			pack_getc(inf);
		}
		if(bytemask & 8)
		{	//End of repeat
			word = pack_getc(inf);	//Read number of repeats
		}
		if(bytemask & 128)
		{	//Double bar
		}
		if(fileversion >= 500)
		{	//Versions 5.0 and newer of the format store unknown data/padding here
			if(bytemask & 16)
			{	//Number of alternate ending
				word = pack_getc(inf);	//Read alternate ending number
			}
			else
			{
				pack_getc(inf);		//Unknown data
			}
			byte = pack_getc(inf);	//Read triplet feel value
			pack_getc(inf);			//Unknown data
		}
		tsarray[ctr].num = curnum;	//Store this measure's time signature for future reference
		tsarray[ctr].den = curden;
		totalbeats += curnum;		//Add the number of beats in this measure to the ongoing counter
	}//For each measure
	if(eof_song->beats < totalbeats + 2)
	{	//If there will be beats appended to the project to encompass the guitar pro file's tracks
#ifdef GP_IMPORT_DEBUG
		snprintf(eof_log_string, sizeof(eof_log_string), "\tAppending %lu beats to project so that guitar pro transcriptions will fit", totalbeats + 2 - eof_song->beats);
		eof_log(eof_log_string, 1);
#endif
		while(eof_song->beats < totalbeats + 2)
		{	//Until the loaded project has enough beats to contain the Guitar Pro transcriptions, and two extra to allow room for processing beat lengths later
			if(!eof_song_add_beat(eof_song))
			{
				eof_log("Error allocating memory (8)", 1);
				pack_fclose(inf);
				free(gp->names);
				for(ctr = 0; ctr < tracks; ctr++)
				{	//Free all previously allocated track structures
					free(gp->track[ctr]);
				}
				free(np);
				free(hopo);
				free(gp);
				free(tsarray);
				free(strings);
				return NULL;
			}
			eof_song->beat[eof_song->beats - 1]->ppqn = eof_song->beat[eof_song->beats - 2]->ppqn;	//Match the tempo of the previously last beat
			double beat_length = (double)60000.0 / ((double)60000000.0 / (double)eof_song->beat[eof_song->beats - 2]->ppqn);	//Get the length of the previously last beat
			eof_song->beat[eof_song->beats - 1]->fpos = eof_song->beat[eof_song->beats - 2]->fpos + beat_length;
			eof_song->beat[eof_song->beats - 1]->pos = eof_song->beat[eof_song->beats - 1]->fpos + 0.5;	//Round up
		}
		eof_chart_length = eof_song->beat[eof_song->beats - 1]->pos;	//Alter the chart length so that the full transcription will display
	}
	if(!eof_song->tags->tempo_map_locked)
	{	//If the active project's tempo map isn't locked
		eof_clear_input();
		if(alert(NULL, "Import Guitar Pro file's time signatures?", NULL, "&Yes", "&No", 'y', 'n') == 1)
		{	//If user opts to import the TS changes into the active project
			if(undo_made)
			{	//If calling function wants to track an undo state being made if time signatures are imported into the project
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				*undo_made = 1;
			}
			curnum = curden = 0;
			unsigned long beatctr = 0;
			for(ctr = 0; ctr < measures; ctr++)
			{	//For each measure in the GP file
				for(ctr2 = 0; ctr2 < tsarray[ctr].num; ctr2++, beatctr++)
				{	//For each beat in this measure
					if(!ctr2 && ((tsarray[ctr].num != curnum) || (tsarray[ctr].den != curden)))
					{	//If this is a time signature change on the first beat of the measure
						eof_apply_ts(tsarray[ctr].num, tsarray[ctr].den, beatctr, eof_song, 0);	//Apply the change to the active project
					}
					else
					{	//Otherwise clear all beat flags except those that aren't TS related
						unsigned long flags = eof_song->beat[beatctr]->flags & EOF_BEAT_FLAG_ANCHOR;
						flags |= eof_song->beat[beatctr]->flags & EOF_BEAT_FLAG_EVENTS;
						eof_song->beat[beatctr]->flags = flags;
					}
				}
				curnum = tsarray[ctr].num;
				curden = tsarray[ctr].den;
			}
		}
	}


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

		eof_read_gp_string(inf, &word, buffer, 0);		//Read track name string
		gp->names[ctr] = malloc(sizeof(buffer) + 1);	//Allocate memory to store track name string into guitar pro structure
		if(!gp->names[ctr])
		{
			eof_log("Error allocating memory (9)", 1);
			pack_fclose(inf);
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
			free(np);
			free(hopo);
			free(gp);
			free(tsarray);
			free(strings);
			return NULL;
		}
		strcpy(gp->names[ctr], buffer);
#ifdef GP_IMPORT_DEBUG
		snprintf(eof_log_string, sizeof(eof_log_string), "\t\tTrack #%lu: %s", ctr + 1, buffer);
		eof_log(eof_log_string, 1);
#endif
		pack_fseek(inf, 40 - word);					//Skip the padding that follows the track name string
		pack_ReadDWORDLE(inf, &strings[ctr]);		//Read the number of strings in this track
		gp->track[ctr]->numstrings = strings[ctr];
		if(strings[ctr] > 6)
		{	//EOF will cap it at 6 strings
			gp->track[ctr]->numstrings = 6;
		}
#ifdef GP_IMPORT_DEBUG
		snprintf(eof_log_string, sizeof(eof_log_string), "\t\t\t%lu strings", strings[ctr]);
		eof_log(eof_log_string, 1);
#endif
		for(ctr2 = 0; ctr2 < 7; ctr2++)
		{	//For each of the 7 possible usable strings
			if(ctr2 < strings[ctr])
			{	//If this string is used
				pack_ReadDWORDLE(inf, &dword);	//Read the tuning for this string
#ifdef GP_IMPORT_DEBUG
				snprintf(eof_log_string, sizeof(eof_log_string), "\t\t\tTuning for string #%lu: MIDI note %lu (%s)", ctr2 + 1, dword, eof_note_names[(dword + 3) % 12]);
				eof_log(eof_log_string, 1);
#endif
				if(ctr2 < 6)
				{	//Don't store the tuning for more than six strings, as EOF doesn't support 7
					gp->track[ctr]->tuning[gp->track[ctr]->numstrings - 1 - ctr2] = (dword + 3) % 12;	//Store the raw note value in number of half steps above A, this will have to be converted to the track and string count specific relative value once mapped to an EOF instrument track (Guitar Pro stores the tuning in string order starting from #1, reversed from EOF)
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
		snprintf(eof_log_string, sizeof(eof_log_string), "\t\t\tNumber of frets: %lu", dword);
		eof_log(eof_log_string, 1);
#endif
		gp->track[ctr]->numfrets = dword;
		pack_ReadDWORDLE(inf, &dword);	//Read the capo position for this track
		word = pack_getc(inf);			//Track color (Red intensity)
		word = pack_getc(inf);			//Track color (Green intensity)
		word = pack_getc(inf);			//Track color (Blue intensity)
		pack_getc(inf);					//Read unused value
		if(fileversion > 500)
		{
			bytemask = pack_getc(inf);	//Track properties 1 bitmask
			pack_getc(inf);				//Track properties 2 bitmask
			pack_getc(inf);				//Unknown data
			word = pack_getc(inf);		//MIDI bank
			word = pack_getc(inf);		//Human playing
			word = pack_getc(inf);		//Auto accentuation on the beat
			pack_fseek(inf, 31);		//Unknown data
			word = pack_getc(inf);		//Selected sound bank option
			pack_fseek(inf, 7);			//Unknown data
			byte = pack_getc(inf);		//Low frequency band lowered
			byte = pack_getc(inf);		//Mid frequency band lowered
			byte = pack_getc(inf);		//High frequency band lowered
			byte = pack_getc(inf);		//Gain lowered
			eof_read_gp_string(inf, NULL, buffer, 1);	//Read track instrument effect 1
			eof_read_gp_string(inf, NULL, buffer, 1);	//Read track instrument effect 2
		}
		else if(fileversion == 500)
		{
			pack_fseek(inf, 45);		//Unknown data
		}
	}//For each track
	if(fileversion >= 500)
	{
		pack_getc(inf);	//Unknown data
	}


//Read note data
#ifdef GP_IMPORT_DEBUG
	eof_log("\tParsing note data", 1);
#endif
#define TARGET_VOICE 0
	unsigned long curbeat = 0;		//Tracks the current beat number for the current measure
	double gp_durations[] = {1.0, 0.5, 0.25, 0.125, 0.0625, 0.03125, 0.015625};	//The duration of each note type in terms of one measure (whole note, half, 4th, 8th, 12th, 32nd, 64th)
	double note_duration;			//Tracks the note's duration as a percentage of the current measure
	double measure_position;		//Tracks the current position as a percentage within the current measure
	unsigned long flags;			//Tracks the flags for the current note
	char new_note;					//Tracks whether a new note is to be created
	char tie_note;					//Tracks whether a note is a tie note
	for(ctr = 0; ctr < measures; ctr++)
	{	//For each measure
		curnum = tsarray[ctr].num;	//Obtain the time signature for this measure
		curden = tsarray[ctr].den;
		for(ctr2 = 0; ctr2 < tracks; ctr2++)
		{	//For each track
			unsigned voice, maxvoices = 1;
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
					new_note = 0;	//Assume no new note is to be added unless a normal/muted note is parsed
					tie_note = 0;	//Assume a note isn't a tie note unless found otherwise
					flags = 0;
					bytemask = pack_getc(inf);	//Read beat bitmask
					if(bytemask & 64)
					{	//Beat is a rest
						word = pack_getc(inf);	//Rest beat type (empty/rest)
						new_note = 0;
					}
					byte = pack_getc(inf);		//Read beat duration
					assert((byte >= -2) && (byte <= 4));	//These are the expected duration values
					note_duration = gp_durations[byte + 2];	//Get this note's duration in measures
					if(bytemask & 1)
					{	//Dotted note
						note_duration *= 1.5;	//A dotted note is one and a half times as long as normal
					}
					if(bytemask & 32)
					{	//Beat is an N-tuplet
						pack_ReadDWORDLE(inf, &dword);	//Number of notes played within the "tuplet" (ie. 3 = triplet)
						note_duration = 1.0 / (double)curnum / (double)dword;	//A tuplet is one beat divided into equally-long parts (ie. a triplet is 1/3 of one beat)
					}
					if(bytemask & 2)
					{	//Beat has a chord diagram
						word = pack_getc(inf);	//Read chord diagram format
						if(word == 0)
						{	//Chord diagram format 0, ie. GP3
							eof_read_gp_string(inf, NULL, buffer, 1);	//Read chord name
							pack_ReadDWORDLE(inf, &dword);				//Read the diagram fret position
							for(ctr4 = 0; ctr4 < strings[ctr2]; ctr4++)
							{	//For each string defined in the track
								pack_ReadDWORDLE(inf, &dword);			//Read the fret played on this string
							}
						}//Chord diagram format 0, ie. GP3
						else if(word == 1)
						{	//Chord diagram format 1, ie. GP4
							word = pack_getc(inf);		//Read sharp/flat indicator
							pack_fseek(inf, 3);			//Unknown data
							word = pack_getc(inf);		//Read chord root
							if(fileversion / 100 == 3)
							{	//If it is a GP 3.x file
								pack_fseek(inf, 3);		//Unknown data
							}
							word = pack_getc(inf);		//Read chord type
							if(fileversion / 100 == 3)
							{	//If it is a GP 3.x file
								pack_fseek(inf, 3);		//Unknown data
							}
							word = pack_getc(inf);		//9th/11th/13th option
							if(fileversion / 100 == 3)
							{	//If it is a GP 3.x file
								pack_fseek(inf, 3);		//Unknown data
							}
							pack_ReadDWORDLE(inf, &dword);	//Read bass note (lowest note played in string)
							word = pack_getc(inf);			//+/- option
							pack_fseek(inf, 4);				//Unknown data
							word = pack_getc(inf);			//Read chord name string length
							pack_fread(buffer, 20, inf);	//Read chord name (which is padded to 20 bytes)
							buffer[word] = '\0';			//Ensure string is terminated to be the right length
							pack_fseek(inf, 2);				//Unknown data
							byte = pack_getc(inf);			//Tonality of the fifth
							if(fileversion / 100 == 3)
							{	//If it is a GP 3.x file
								pack_fseek(inf, 3);			//Unknown data
							}
							byte = pack_getc(inf);			//Tonality of the ninth
							if(fileversion / 100 == 3)
							{	//If it is a GP 3.x file
								pack_fseek(inf, 3);			//Unknown data
							}
							byte = pack_getc(inf);			//Tonality of the eleventh
							if(fileversion / 100 == 3)
							{	//If it is a GP 3.x file
								pack_fseek(inf, 3);			//Unknown data
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
							barres = pack_getc(inf);	//Read the number of barres in this chord
							for(ctr4 = 0; ctr4 < 5; ctr4++)
							{	//For each of the 5 possible barres
								if(ctr4 < barres)
								{	//If this barre is defined
									word = pack_getc(inf);	//Read the barre position
								}
								else
								{
									pack_getc(inf);
								}
							}
							for(ctr4 = 0; ctr4 < 5; ctr4++)
							{	//For each of the 5 possible barres
								if(ctr4 < barres)
								{		//If this barre is defined
									word = pack_getc(inf);	//Read the barre start string
								}
								else
								{
									pack_getc(inf);
								}
							}
							for(ctr4 = 0; ctr4 < 5; ctr4++)
							{	//For each of the 5 possible barres
								if(ctr4 < barres)
								{	//If this barre is defined
									word = pack_getc(inf);	//Read the barre stop string
								}
								else
								{
									pack_getc(inf);
								}
							}
							byte = pack_getc(inf);	//Chord includes first interval?
							byte = pack_getc(inf);	//Chord includes third interval?
							byte = pack_getc(inf);	//Chord includes fifth interval?
							byte = pack_getc(inf);	//Chord includes seventh interval?
							byte = pack_getc(inf);	//Chord includes ninth interval?
							byte = pack_getc(inf);	//Chord includes eleventh interval?
							byte = pack_getc(inf);	//Chord includes thirteenth interval?
							pack_getc(inf);			//Unknown data
							for(ctr4 = 0; ctr4 < 7; ctr4++)
							{	//For each of the 7 possible usable strings
								if(ctr4 < strings[ctr2])
								{	//If this string is used in the track
									byte = pack_getc(inf);	//Finger # used to play string
								}
								else
								{
									pack_getc(inf);		//Skip this padding
								}
							}
							byte = pack_getc(inf);	//Chord fingering displayed?
						}//Chord diagram format 1, ie. GP4
					}//Beat has a chord diagram
					if(bytemask & 4)
					{	//Beat has text
						eof_read_gp_string(inf, NULL, buffer, 1);	//Read beat text string
					}
					if(bytemask & 8)
					{	//Beat has effects
						unsigned char byte1 = 0, byte2 = 0;

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
							{	//Tremolo
								flags |= EOF_NOTE_FLAG_IS_TREMOLO;
							}
							else if(byte == 1)
							{	//Tapping
								flags |= EOF_PRO_GUITAR_NOTE_FLAG_TAP;
							}
							else if(byte == 2)
							{	//Slapping
							}
							else if(byte == 3)
							{	//Popping
							}
							else
							{	//Unknown
							}
							if(fileversion < 400)
							{
								pack_ReadDWORDLE(inf, &dword);	//String effect value
							}
						}
						if(byte2 & 4)
						{	//Tremolo bar
							eof_gp_parse_bend(inf);
						}
						if(byte1 & 64)
						{	//Stroke (strum) effect
							unsigned char upspeed;
							upspeed = pack_getc(inf);	//Up strum speed
							pack_getc(inf);				//Down strum speed
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

						byte = pack_getc(inf);	//New instrument number
						if(fileversion >= 500)
						{	//These fields are only in version 5.x files
							pack_ReadDWORDLE(inf, &dword);	//RSE related number
							pack_ReadDWORDLE(inf, &dword);	//RSE related number
							pack_ReadDWORDLE(inf, &dword);	//RSE related number
							pack_fseek(inf, 4);				//Unknown data
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
							eof_read_gp_string(inf, NULL, buffer, 1);	//Read the tempo text string
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
							byte = pack_getc(inf);	//New volume change transition
						}
						if(pan_change)
						{	//This field only exists if a new pan value was defined
							byte = pack_getc(inf);	//New pan change transition
						}
						if(chorus_change)
						{	//This field only exists if a new  chorus value was defined
							byte = pack_getc(inf);	//New chorus change transition
						}
						if(reverb_change)
						{	//This field only exists if a new reverb value was defined
							byte = pack_getc(inf);	//New reverb change transition
						}
						if(phaser_change)
						{	//This field only exists if a new phaser value was defined
							byte = pack_getc(inf);	//New phaser change transition
						}
						if(tremolo_change)
						{	//This field only exists if a new tremolo value was defined
							byte = pack_getc(inf);	//New tremolo change transition
						}
						if(tempo_change)
						{	//These fields only exists if a new tempo was defined
							byte = pack_getc(inf);	//New tempo change transition
							if(fileversion > 500)
							{	//This field only exists in versions newer than 5.0 of the format
								byte = pack_getc(inf);	//Tempo text string hidden
							}
						}
						if(fileversion >= 400)
						{	//This field is not in version 3.0 files, assume 4.x or higher
							byte = pack_getc(inf);	//Mix table change applied tracks bitmask
						}
						if(fileversion >= 500)
						{	//This unknown byte is only in version 5.x files
							pack_fseek(inf, 1);		//Unknown data
						}
						if(fileversion > 500)
						{	//These strings are only in versions newer than 5.0 of the format
							eof_read_gp_string(inf, NULL, buffer, 1);	//Read the Effect 2 string
							eof_read_gp_string(inf, NULL, buffer, 1);	//Read the Effect 1 string
						}
					}//Beat has mix table change
					usedstrings = pack_getc(inf);	//Used strings bitmask
					char ghost = 0;	//Track the ghost status for notes
					unsigned bitmask;
					unsigned char frets[7];		//Store fret values for each string
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
									if(np[ctr2])
									{	//If there is a previously created note, alter its length
										tie_note = 1;
										unsigned long beat_position = (measure_position + note_duration) * curnum;
										double partial_beat_position = (measure_position + note_duration) * curnum - beat_position;	//How far into this beat the note ends
										beat_position += curbeat;	//Add the number of beats into the track the current measure is
										double beat_length = eof_song->beat[beat_position + 1]->fpos - eof_song->beat[beat_position]->fpos;
										np[ctr2]->length = eof_song->beat[beat_position]->fpos + (beat_length * partial_beat_position) - np[ctr2]->pos + 0.5;	//Define the length of this note
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
								byte = pack_getc(inf);	//Time independent duration value
								byte = pack_getc(inf);	//Time independent duration values
							}
							if(bytemask & 16)
							{	//Note dynamic
								word = pack_getc(inf) - 1;	//Get the dynamic value and remap its values from 0 to 7
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
								byte = pack_getc(inf);	//Right hand fingering
							}
							if((bytemask & 1) && (fileversion >= 500))
							{	//Time independent duration (for versions of the format 5.x or newer)
								pack_fseek(inf, 8);		//Unknown data
							}
							if(fileversion >= 500)
							{	//This padding isn't in version 3.x and 4.x files
								pack_fseek(inf, 1);		//Unknown data
							}
							if(bytemask & 8)
							{	//Note effects
								char byte1 = 0, byte2 = 0;
								byte1 = pack_getc(inf);	//Note effect bitmask
								if(fileversion >= 400)
								{	//Version 4.0 and higher of the file format has a second note effect bitmask
									byte2 = pack_getc(inf);	//Note effect 2 bitmask
								}
								if(byte1 & 1)
								{	//Bend
									eof_gp_parse_bend(inf);
									flags |= EOF_PRO_GUITAR_NOTE_FLAG_BEND;
								}
								if(byte1 & 2)
								{	//Hammer on/pull off from current note (next note gets the HO/PO status)
									hopo[ctr2] = frets[ctr4] & 0x8F;	//Store the fret value (masking out the MSB ghost bit) so that the next note can be determined as either a HO or a PO
								}
								if(byte1 & 4)
								{	//Slide from current note (GP3 format indicator)
									flags |= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP | EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN;	//The slide direction is unknown and will be corrected later
								}
								if(byte1 & 8)
								{	//Let ring
								}
								if(byte1 & 16)
								{	//Grace note
									byte = pack_getc(inf);	//Grace note fret number
									word = (pack_getc(inf) - 1) % 8;	//Get the grace note dynamic value and remap its values from 0 to 7
									if(fileversion >= 500)
									{	//If the file version is 5.x or higher (this byte verified not to be in 3.0 and 4.06 files)
										byte = pack_getc(inf);	//Grace note transition type
									}
									else
									{	//The purpose of this field in 4.x or older files is unknown
										pack_fseek(inf, 1);		//Unknown data
									}
									byte = pack_getc(inf);	//Grace note duration
									if(fileversion >= 500)
									{	//If the file version is 5.x or higher (this byte verified not to be in 3.0 and 4.06 files)
										byte = pack_getc(inf);	//Grace note position
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
									byte = pack_getc(inf);	//Tremolo picking speed
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
										flags |= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP | EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN;	//The slide direction is unknown and will be corrected later
									}
								}
								if(byte2 & 16)
								{	//Harmonic
									flags |= EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC;
									byte = pack_getc(inf);	//Harmonic type
									if(byte == 2)
									{	//Artificial harmonic
										byte = pack_getc(inf);	//Read harmonic note
										byte = pack_getc(inf);	//Read sharp/flat status
										byte = pack_getc(inf);	//Read octave status
									}
									else if(byte == 3)
									{	//Tapped harmonic
										byte = pack_getc(inf);	//Right hand fret
									}
								}
								if(byte2 & 32)
								{	//Trill
									byte = pack_getc(inf);	//Trill with fret
									byte = pack_getc(inf);	//Trill duration
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
							pack_fseek(inf, 1);	//Unknown data
						}
					}
					if(new_note)
					{	//If a new note is to be created
						np[ctr2] = eof_pro_guitar_track_add_note(gp->track[ctr2]);	//Add a new note to the current track
						if(!np[ctr2])
						{
							eof_log("Error allocating memory (10)", 1);
							pack_fclose(inf);
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
							free(np);
							free(hopo);
							free(gp);
							free(tsarray);
							free(strings);
							return NULL;
						}
						np[ctr2]->flags = flags;
						for(ctr4 = 0; ctr4 < gp->track[ctr2]->numstrings; ctr4++)
						{	//For each of this track's supported strings
							np[ctr2]->frets[ctr4] = frets[strings[ctr2] - 1 - ctr4];	//Re-map from GP's string numbering to EOF's
						}
						np[ctr2]->legacymask = 0;
						np[ctr2]->midi_length = 0;
						np[ctr2]->midi_pos = 0;
						np[ctr2]->name[0] = '\0';
						np[ctr2]->note = usedstrings >> (7 - strings[ctr2]);	//Guitar pro's note bitmask reflects string 7 being the LSB
						np[ctr2]->ghost = ghost >> (7 - strings[ctr2]);	//Likewise translate the ghost bit mask
						np[ctr2]->type = EOF_NOTE_AMAZING;

//Determine the correct timestamp position and duration
						unsigned long beat_position = measure_position * curnum;			//How many whole beats into the current measure the position is
						double partial_beat_position = measure_position * curnum - beat_position;	//How far into this beat the note begins
						beat_position += curbeat;	//Add the number of beats into the track the current measure is
						double beat_length = eof_song->beat[beat_position + 1]->fpos - eof_song->beat[beat_position]->fpos;
						np[ctr2]->pos = eof_song->beat[beat_position]->fpos + (beat_length * partial_beat_position);	//Define the position of this note

						beat_position = (measure_position + note_duration) * curnum;
						partial_beat_position = (measure_position + note_duration) * curnum - beat_position;	//How far into this beat the note ends
						beat_position += curbeat;	//Add the number of beats into the track the current measure is
						beat_length = eof_song->beat[beat_position + 1]->fpos - eof_song->beat[beat_position]->fpos;
						np[ctr2]->length = eof_song->beat[beat_position]->fpos + (beat_length * partial_beat_position) - np[ctr2]->pos + 0.5;	//Define the length of this note
					}//If a new note is to be created
					else if(np[ctr2] && tie_note)
					{	//Otherwise if this was a tie note
						np[ctr2]->flags |= flags;	//Apply this tie note's flags to the previous note
					}
					measure_position += note_duration;	//Update the measure position
				}//For each beat
			}//For each voice
			if(fileversion >= 500)
			{
				pack_fseek(inf, 1);		//Unknown data
			}
		}//For each track
		curbeat += curnum;	//Add this measure's number of beats to the beat counter
	}//For each measure


//Create trill phrases
	unsigned long count;
	unsigned long startpos, endpos;
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
					gp->track[ctr]->tremolos++;
				}
			}
		}
	}


//Correct slide directions
	unsigned char startfret, endfret, bitmask ;
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
						endfret = gp->track[ctr]->note[ctr2 + 1]->frets[ctr3];	//Record the fret number used on the next note
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
						break;
					}
				}
			}
		}
	}


//Clean up
	pack_fclose(inf);
	free(strings);
	free(tsarray);
	free(np);
	free(hopo);
	puts("\nSuccess");
	return gp;
}

#endif
