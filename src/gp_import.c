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
#include "foflc/RS_parse.h"	//For shrink_xml_text()
#include "menu/track.h"	//For tech view functions
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
char *eof_note_names_flat[12] =	{"A","Bb","B","C","Db","D","Eb","E","F","Gb","G","Ab"};		//The name of each scale in order from 0 (0 semitones above A) to 11 (11 semitones above A)
char **eof_note_names = eof_note_names_flat;
#endif

void pack_ReadWORDLE(PACKFILE *inf, unsigned *data)
{
	unsigned char buffer[2] = {0};

	if(inf)
	{
		if((pack_fread(buffer, 2, inf) == 2) && data)
		{	//If the data was successfully read, and data isn't NULL, store the value
			*data = ((unsigned short)buffer[0] | ((unsigned short)buffer[1]<<8));	//Convert to 2 byte integer
		}
	}
}

void pack_ReadDWORDLE(PACKFILE *inf, unsigned long *data)
{
	unsigned char buffer[4] = {0};

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
	if((int)len == EOF)
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
		printf("0x%lX:\t%s", (unsigned long)ftell(inf), text);
	}
}
#else
void eof_gp_debug_log(PACKFILE *inf, char *text)
{
	if(!inf || !text)
	{	//Read these variables so Splint doesn't complain about them being unused
		eof_log("Logging error", 1);
	}
}
#endif

int eof_gp_parse_bend(PACKFILE *inf, struct guitar_pro_bend *bp)
{
	unsigned word;
	unsigned long height = 0, points = 0, ctr, dword = 0, dword2 = 0;

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
	printf("%lu * 2 cents\n", height);
	eof_gp_debug_log(inf, "\t\tNumber of points:  ");
	pack_ReadDWORDLE(inf, &points);	//Read number of bend points
	printf("%lu points\n", points);
	if(bp)
	{	//If the calling function wanted to retrieve the bend height
		bp->bendpoints = points;
		if(bp->bendpoints > 30)
		{	//Only store the first 30 points
			bp->bendpoints = 30;
		}
	}
	if(points >= 100)
	{	//Compare the bend point count against an arbitrarily large number to satisfy Coverity
		eof_gp_debug_log(inf, "\t\t\tToo many bend points, aborting.");
		return 1;
	}
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
		pack_ReadDWORDLE(inf, &dword2);
		printf("%ld * 2 cents\n", (long)dword2);
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
		if(bp && (ctr < 30))
		{	//If the calling function wanted to retrieve the bend height, store the first 30 definitions
			bp->bendpos[ctr] = dword;
			bp->bendheight[ctr] = dword2 / 25;	//Store the value in quarter steps
			if(!height && bp->bendheight[ctr])
			{	//If the bend did not have a summarized height
				height = dword2;	//Store that bend value as the height, which will be converted to quarter steps after the loop completes
			}
		}
	}//For each point in the bend
	if(bp)
	{	//If the calling function wanted to retrieve the bend height
		bp->summaryheight = height / 25;	//Store the value in quarter steps (100 cents in a half step, and height is the bend measured in increments of 2 cents)
	}
	return 0;	//Return success
}

#ifndef EOF_BUILD
{	//Standalone parse utility

EOF_SONG *parse_gp(const char * fn)
{
	char buffer[256], byte, *buffer2, bytemask, bytemask2, usedstrings;
	unsigned word, fileversion;
	unsigned long dword, ctr, ctr2, ctr3, ctr4, tracks, measures, *strings, beats, barres;
	char *note_dynamics[8] = {"ppp", "pp", "p", "mp", "mf", "f", "ff", "fff"};
	PACKFILE *inf;
	char *musical_symbols[19] = {"Coda", "Double Coda", "Segno", "Segno Segno", "Fine", "Da Capo", "Da Capo al Coda", "Da Capo al double Coda", "Da Capo al Fine", "Da Segno", "Da Segno al Coda", "Da Segno al double Coda", "Da Segno al Fine", "Da Segno Segno", "Da Segno Segno al Coda", "Da Segno Segno al double Coda", "Da Segno Segno al Fine", "Da Coda", "Da double Coda"};

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
	printf("%u bytes of padding)\n", 30 - word);
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
			printf(" (start from bar #%lu):  ", dword);
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
		printf("%d (%d %s)\n", byte, abs(byte), (byte < 0) ? "flats" : "sharps");
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
		printf("%d (%d %s)\n", byte, abs(byte), (byte < 0) ? "flats" : "sharps");
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
	{	//Versions 5.0 and newer of the format store 19 musical directional symbols and a master reverb setting here
		(void) puts("\tMusical symbols:");
		for(ctr = 0; ctr < 19; ctr++)
		{	//For each of the musical directional symbols
			eof_gp_debug_log(inf, "\"");	//Print file position
			printf("%s\" symbol is ", musical_symbols[ctr]);
			pack_ReadWORDLE(inf, &word);
			if(word == 0xFFFF)
			{
				(void) puts("unused");
			}
			else
			{
				printf("at beat #%u\n", word);
			}
		}
		eof_gp_debug_log(inf, "Master reverb:  ");
		pack_ReadDWORDLE(inf, &dword);	//Read the master reverb value
		printf("%lu\n", dword);
	}

	eof_gp_debug_log(inf, "Number of measures:  ");
	pack_ReadDWORDLE(inf, &measures);	//Read the number of measures
	printf("%lu\n", measures);
	eof_gp_debug_log(inf, "Number of tracks:  ");
	pack_ReadDWORDLE(inf, &tracks);	//Read the number of tracks
	printf("%lu\n", tracks);

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
		printf("%d\n", (bytemask & 0xFF));
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
			{	//Versions 3 and 4 define the alternate ending next, followed by the section definition, then the key signature
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
				if(bytemask & 64)
				{	//Key signature change
					eof_gp_debug_log(inf, "\tNew key signature:  ");
					byte = pack_getc(inf);	//Read the key
					printf("%d (%d %s, ", byte, abs(byte), (byte < 0) ? "flats" : "sharps");
					byte = pack_getc(inf);	//Read the major/minor byte
					printf("%s)\n", !byte ? "major" : "minor");
				}
			}
			else
			{	//Version 5 and newer define these items in a different order and some other items afterward
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
				if(bytemask & 64)
				{	//Key signature change
					eof_gp_debug_log(inf, "\tNew key signature:  ");
					byte = pack_getc(inf);	//Read the key
					printf("%d (%d %s, ", byte, abs(byte), (byte < 0) ? "flats" : "sharps");
					byte = pack_getc(inf);	//Read the major/minor byte
					printf("%s)\n", !byte ? "major" : "minor");
				}
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
				if(bytemask & 16)
				{	//Number of alternate ending
					eof_gp_debug_log(inf, "\tNumber of alternate ending:  ");
					word = pack_getc(inf);	//Read alternate ending number
					printf("%u\n", word);
				}
				else
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
		printf("%d\n", (bytemask & 0xFF));
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
		printf("%u bytes of padding)\n", 40 - word);
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
			printf("%d\n", (bytemask & 0xFF));
			eof_gp_debug_log(inf, "\tTrack properties 2 bitmask:  ");
			bytemask2 = pack_getc(inf);
			printf("%d\n", (bytemask2 & 0xFF));
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
					printf("%d\n", (bytemask & 0xFF));
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
						printf("%d\n", (byte1 & 0xFF));
						if(fileversion >= 400)
						{	//Versions 4.0 and higher of the format have a second beat effects bitmask
							eof_gp_debug_log(inf, "\tExtended beat effects bitmask:  ");
							byte2 = pack_getc(inf);
							printf("%d\n", (byte2 & 0xFF));
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
							if(eof_gp_parse_bend(inf, NULL))
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
							printf("%d\n", (byte & 0xFF));
							eof_gp_debug_log(inf, "\tDownstroke speed:  ");
							byte = pack_getc(inf);
							printf("%d\n", (byte & 0xFF));
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
							printf("%d\n", (byte & 0xFF));
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
					printf("%d\n", (usedstrings & 0xFF));
					for(ctr4 = 0, bitmask = 64; ctr4 < 7; ctr4++, bitmask>>=1)
					{	//For each of the 7 possible usable strings
						if(bitmask & usedstrings)
						{	//If this string is used
							printf("\t\t\tString %lu:\n", ctr4 + 1);
							eof_gp_debug_log(inf, "\t\tNote bitmask:  ");
							bytemask = pack_getc(inf);
							printf("%d\n", (bytemask & 0xFF));
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
								printf("%d ", (byte & 0xFF));
								byte = pack_getc(inf);
								printf("%d\n", (byte & 0xFF));
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
								printf("%d\n", (byte & 0xFF));
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
								printf("%d\n", (byte1 & 0xFF));
								if(fileversion >= 400)
								{	//Version 4.0 and higher of the file format has a second note effect bitmask
									eof_gp_debug_log(inf, "\t\tNote effect 2 bitmask:  ");
									byte2 = pack_getc(inf);
									printf("%d\n", (byte2 & 0xFF));
								}
								if(byte1 & 1)
								{	//Bend
									if(eof_gp_parse_bend(inf, NULL))
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
									printf("%d\n", (byte & 0xFF));
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
									printf("%d\n", (byte & 0xFF));
									if(fileversion >= 500)
									{	//If the file version is 5.x or higher (this byte verified not to be in 3.0 and 4.06 files)
										eof_gp_debug_log(inf, "\t\t\tGrace note position:  ");
										byte = pack_getc(inf);
										printf("%d\n", (byte & 0xFF));
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
									printf("%d\n", (byte & 0xFF));
								}
								if(byte2 & 8)
								{	//Slide
									eof_gp_debug_log(inf, "\t\t\tSlide type:  ");
									byte = pack_getc(inf);
									printf("%d\n", (byte & 0xFF));
								}
								if(byte2 & 16)
								{	//Harmonic
									eof_gp_debug_log(inf, "\t\t\tHarmonic type:  ");
									byte = pack_getc(inf);
									printf("%d\n", (byte & 0xFF));
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
										printf("%d\n", (byte & 0xFF));
									}
								}
								if(byte2 & 32)
								{	//Trill
									eof_gp_debug_log(inf, "\t\t\tTrill with fret:  ");
									byte = pack_getc(inf);
									printf("%d\n", (byte & 0xFF));
									eof_gp_debug_log(inf, "\t\t\tTrill duration:  ");
									byte = pack_getc(inf);
									printf("%d\n", (byte & 0xFF));
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

}
#else

#define GP_IMPORT_DEBUG

struct eof_guitar_pro_struct *eof_load_gp(const char * fn, char *undo_made)
{
	#define EOF_GP_IMPORT_BUFFER_SIZE 256
	char buffer[EOF_GP_IMPORT_BUFFER_SIZE + 1] = {0}, *buffer2, buffer3[EOF_GP_IMPORT_BUFFER_SIZE + 1] = {0}, buffer4[EOF_GP_IMPORT_BUFFER_SIZE + 1] = {0}, byte, bytemask, *ptr, patches[64] = {0};
	unsigned char usedstrings, definedstrings;
	unsigned char usedtie;	//Tracks which strings in an imported note were tie notes
	unsigned word = 0, fileversion;
	unsigned long dword = 0, ctr, ctr2, ctr3, ctr4, ctr5, tracks = 0, measures = 0, *strings, beats = 0;
	PACKFILE *inf = NULL, *inf2;	//The GPA import logic will open the file handle for the Guitar Pro file in inf if applicable
	struct eof_guitar_pro_struct *gp = NULL;
	struct eof_gp_measure *tsarray;	//Stores measure information relating to time signatures, alternate endings and navigational symbols
	EOF_PRO_GUITAR_NOTE **np;	//Will store the last created note for each track (for handling tie and grace notes)
	char *hopo;			//Will store the fret value of the previous note marked as HO/PO (in GP, if note #N is marked for this, note #N+1 is the one that is a HO or PO), otherwise -1, for each track
	unsigned long *hopobeatnum;	//Will store the beat (note) number for which each track's last ho/po notation was defined, to ensure that the status is properly applied to the following note
	unsigned long *hopomeasurenum;	//Likewise tracks which measure each track's last ho/po notation was defined, to allow tracking for these statuses between different measures
	char *durations;	//Will store the last imported note duration for each track (to handle triplet feel notation)
	double *note_durations;	//Will store the effective duration (in terms of a whole measure) for the last imported note in each track
	char tripletfeel = 0;	//The current triplet feel notation in effect (0 = none, 1 = 8th note, 2 = 16th note)
	char user_warned = 0;	//Used to track user warnings about the file being corrupt
	char string_warning = 0;	//Used to track a user warning about the string count for a track being higher than what EOF supports
	char drop_7 = 0;			//Used to track whether string 7 is being dropped during import, if any tracks have 7 strings
	char drop_1 = 0;			//Used to track whether string 1 is being dropped during import, if any tracks have 7 strings
	unsigned long curbeat = 0;		//Tracks the current beat number for the current measure
	double gp_durations[] = {1.0, 0.5, 0.25, 0.125, 0.0625, 0.03125, 0.015625};	//The duration of each note type in terms of one whole note (whole note, half, 4th, 8th, 16th, 32nd, 64th)
	double note_duration;			//Tracks the note's duration as an amount of the current measure
	double measure_position;		//Tracks the current position as an amount within the current measure
	unsigned long allflags;			//Tracks the flags for the current note
	unsigned long flags;			//Tracks the flags for the current string while individual strings are being parsed, after which this variable stores the combined flags of all gems in the note
	unsigned long tflags;			//Tracks any temporary flags marked for a note
	unsigned long tieflags;			//Tracks the flags for the tie gems only in the current note (so that tie chords containing a different string than the connecting note won't apply linknext status if it's not appropriate)
	unsigned char bendstrength;		//Tracks the note's bend strength if applicable
	struct guitar_pro_bend bendstruct = {0, 0, {0}, {0}};	//Stores data about the bend being parsed
	double laststartpos = 0, lastendpos = 0;	//Stores the start and end position of the last normal or tie note to be parsed, so bend point data can be used to create tech notes
	double lastgracestartpos = 0, lastgraceendpos = 0;	//Stores the start and end position of the last grace note to be parsed
	unsigned char graceonbeat = 0;	//Tracks whether the currently-parsed grace note is on the beat instead of before it
	unsigned char gracetrans = 0;	//Tracks the grace note's transition type
	char new_note;					//Tracks whether a new note is to be created
	char tie_note;					//Tracks whether a note is a tie note
	char rest_note;					//Tracks whether a note is a rest note
	unsigned char finger[7];		//Store left (fretting hand) finger values for each string
	unsigned char curnum = 4, curden = 4;	//Stores the current time signature (4/4 assumed until one is explicitly defined)
	unsigned long totalbeats = 0;			//Count the total number of beats in the Guitar Pro file's transcription
	unsigned long beatctr = 0;
	unsigned char startfret, endfret, bitmask;
	char *rssectionname;
	unsigned char start_of_repeat, num_of_repeats;
	char import_ts = 0;		//Will be set to nonzero if user opts to import time signatures
	char note_is_short = 0;	//Will be set to nonzero if the note being parsed should have its sustain dropped (ie. shorter than a quarter note or is played staccato), pending techniques that overrule this
	char parse_gpa = 0;		//Will be set to nonzero if the specified file is detected to be XML, in which case, the Go PlayAlong file will be parsed
	size_t maxlinelength;
	unsigned long linectr = 2, num_sync_points = 0, raw_num_sync_points = 0;
	struct eof_gpa_sync_point *sync_points = NULL, temp_sync_point = {0, 0, 0, 0.0, 0.0, 0.0, 0.0, 0};
	char error = 0;
	char *musical_symbols[19] = {"Coda", "Double Coda", "Segno", "Segno Segno", "Fine", "Da Capo", "Da Capo al Coda", "Da Capo al double Coda", "Da Capo al Fine", "Da Segno", "Da Segno al Coda", "Da Segno al double Coda", "Da Segno al Fine", "Da Segno Segno", "Da Segno Segno al Coda", "Da Segno Segno al double Coda", "Da Segno Segno al Fine", "Da Coda", "Da double Coda"};
	unsigned char unpitchend;	//Tracks the end position for imported notes that slide in/out with no formal slide definition
	const char *gpfile;	//Will be set to point to the name of the GP file to import, since when importing a GPA file, fn points to the XML file and not the GP file
	unsigned long skipbeatsourcectr = 0;
		//During GPA import, timings can be configured so that beats start at a negative position (before the start of the audio) if the first sync point is not at measure 1
		//This counter is an offset indicating how many beats of content are being omitted from the imported track, so source beat #N is imported to beat #(N-skipbeatsourcectr) in the project
	char importnote = 0;	//A boolean variable tracking whether each note is at or after 0ms and will import
	unsigned slide_in_from_warned = 0;	//Tracks whether the user has been warned that slide in from above/below notes were encountered

	eof_log("\tImporting Guitar Pro file", 1);
	eof_log("eof_load_gp() entered", 1);

	if(!fn)
	{
		return NULL;
	}
	gpfile = fn;	//Unless fn is found to point to a Go PlayAlong XML file, fn is assumed to be the path to a Guitar Pro file


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
		if(strcasestr_spec(buffer2, "<?xml") || strcasestr_spec(buffer2, "<song"))
		{	//If the file is determined to be XML based
			if(eof_song->tags->tempo_map_locked)
			{	//If the user has locked the tempo map
				eof_clear_input();
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
		char identity = 0;	//Set to 1 if any Go PlayAlong tags are found, 2 if a Rocksmith phrase tag is found

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
				identity = 1;
				for(ctr = 0; ctr < EOF_GP_IMPORT_BUFFER_SIZE; ctr++)
				{	//For each character buffer3[] can hold
					if((*ptr == '<') || (*ptr == '\0'))
					{	//If end of string or end of XML tag are reached
						break;
					}
					buffer3[ctr] = *ptr;
					ptr++;
				}
				buffer3[ctr] = '\0';	//Terminate the string
				shrink_xml_text(buffer4, sizeof(buffer4), buffer3);	//Convert any escape sequences in the file name to regular characters
				(void) replace_filename(eof_temp_filename, fn, "", 1024);
				strncat(eof_temp_filename, buffer4, 1024 - strlen(eof_temp_filename) - 1);	//Build the path to the GP file
				inf = pack_fopen(eof_temp_filename, "rb");
				if(!inf)
				{
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tError loading:  Cannot open GP file (%s) specified in XML file:  \"%s\"", eof_temp_filename, strerror(errno));	//Get the Operating System's reason for the failure
					eof_log(eof_log_string, 1);

					//Retry building the file path without converting escape sequences, in case Go PlayAlong did not correctly encode letters like ampersand
					eof_log("\t\tRetrying without converting escape sequences in the GP5 file name", 1);
					(void) replace_filename(eof_temp_filename, fn, "", 1024);
					strncat(eof_temp_filename, buffer3, 1024 - strlen(eof_temp_filename) - 1);	//Build the path to the GP file, with the unaltered scoreUrl file name from the XML file
					inf = pack_fopen(eof_temp_filename, "rb");
					if(!inf)
					{
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tError loading:  Cannot open GP file (%s) specified in XML file:  \"%s\"", eof_temp_filename, strerror(errno));	//Get the Operating System's reason for the failure
						eof_log(eof_log_string, 1);

						error = 1;
					}
				}
				gpfile = eof_temp_filename;	//Point gpfile to the name of the GP file to parse
			}

			ptr = strcasestr_spec(buffer2, "<sync>");
			if(ptr)
			{	//If the sync tag is present
				identity = 1;
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
				raw_num_sync_points = atol(buffer3);	//Convert to integer value

				//Allocate array for sync points
				if(raw_num_sync_points)
				{	//If there is a valid number of sync entries
					sync_points = malloc(sizeof(struct eof_gpa_sync_point) * raw_num_sync_points);
					if(!sync_points)
					{
						eof_log("\t\tError allocating memory (2).  Aborting", 1);
						error = 1;
					}
					else
					{
						memset(sync_points, 0, sizeof(struct eof_gpa_sync_point) * raw_num_sync_points);	//Fill with 0s to satisfy Splint
					}
				}

				//Store sync points into array
				for(ctr = 0; ctr < raw_num_sync_points; ctr++)
				{	//For each expected sync point
					if(!eof_get_next_gpa_sync_point(&ptr, &temp_sync_point))
					{	//If the sync point was not read
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tError parsing sync point #%lu.  Aborting", ctr);
						eof_log(eof_log_string, 1);
						error = 1;
						break;
					}
///FOR DEBUGGING
//					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tSync point #%lu (time = %lums, measure = %f)", ctr, temp_sync_point.realtime_pos, temp_sync_point.measure + 1.0 + temp_sync_point.pos_in_measure);
//					eof_log(eof_log_string, 1);
					assert(sync_points != NULL);	//Unneeded check to resolve false positive in Splint
					if(temp_sync_point.is_negative)
					{	//If this sync point has a negative timestamp
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tSync point #%lu (time = %lums, measure = %f) is before 0s, it will be skipped.", ctr, temp_sync_point.realtime_pos, temp_sync_point.measure + 1.0 + temp_sync_point.pos_in_measure);
						eof_log(eof_log_string, 1);
					}
					else if(num_sync_points && ((temp_sync_point.measure + temp_sync_point.pos_in_measure <= sync_points[num_sync_points - 1].measure + sync_points[num_sync_points - 1].pos_in_measure) || (temp_sync_point.realtime_pos <= sync_points[num_sync_points - 1].realtime_pos)))
					{	//If there was a previous sync point added, and this sync point isn't a later timestamp or measure position
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tSync point #%lu (time = %lums, measure = %f) is out of chronological order, it will be skipped.", ctr, temp_sync_point.realtime_pos, temp_sync_point.measure + 1.0 + temp_sync_point.pos_in_measure);
						eof_log(eof_log_string, 1);
					}
					else
					{	//This sync point is valid, add it to the array
						memcpy(&sync_points[num_sync_points], &temp_sync_point, sizeof(temp_sync_point));	//Copy the structure to the array
						num_sync_points++;	//Maintain this as the count of valid sync points
					}
				}
			}//If the sync tag is present

			ptr = strcasestr_spec(buffer2, "<phrases");
			if(ptr)
			{	//If a phrases tag (Rocksmith XML file) is present
				identity = 2;
				eof_log("\t\tError:  This appears to be a Rocksmith XML file instead of a Go PlayAlong XML file.  Aborting", 1);
				error = 1;
			}

			(void) pack_fgets(buffer2, (int)maxlinelength, inf2);	//Read next line of text
			linectr++;	//Increment line counter
		}//Until there was an error reading from the file or end of file is reached
		if(error)
		{
			switch(identity)
			{
				case 0:	//Unknown XML file
					allegro_message("Error:  The format of this XML file is unknown.  Aborting");
				break;
				case 1:	//Go PlayAlong XML file
					allegro_message("Error:  Malformed Go PlayAlong XML file.  Aborting");
				break;
				case 2:	//Rocksmith XML file
					allegro_message("This appears to be a Rocksmith XML file.  Use \"File>Rocksmith Import\" instead.");
				break;
				default:
				break;
			}
			if(inf)
				(void) pack_fclose(inf);
			(void) pack_fclose(inf2);
			if(sync_points)
				free(sync_points);
			free(buffer2);
			return NULL;
		}
	}//If the input file was determined to be an XML file
	(void) pack_fclose(inf2);
	if(buffer2)
		free(buffer2);


//Initialize pointers and handles
	if(!inf)
	{	//If the input GP file wasn't opened for reading by the GPA parse logic earlier
		inf = pack_fopen(gpfile, "rb");
	}
	//In the past, the WebTabPlayer website corrupted GP files in some ways, including by inserting a BOM at the beginning of the file, check for this and ignore it if necessary
	if(inf)
	{	//If the file was opened
		char reopen = 1;	//If the BOM sequence is not read, the file will have to be reopened, since Allegro can't simply rewind it

		if(pack_getc(inf) == 0xEF)
		{	//The first byte in a BOM sequence
			if(pack_getc(inf) == 0xBB)
			{	//The second byte in a BOM sequence
				if(pack_getc(inf) == 0xBF)
				{	//The third byte in a BOM sequence
					eof_log("Warning:  This GP file is corrupted (begins with a Byte Order Mark sequence), attempting to recover", 1);
					reopen = 0;	//Don't re-open the file
					//If this sequence of bytes was not read, re-open the file for reading since Allegro can't simply rewind it
				}
			}
		}
		if(reopen)
		{
			(void) pack_fclose(inf);
			inf = pack_fopen(gpfile, "rb");
		}
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
	memset(gp, 0, sizeof(struct eof_guitar_pro_struct));	//Fill with 0s to satisfy Splint


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
		allegro_message("File format version not supported\n(make sure it's GP5 format or older)");
		eof_log("File format version not supported", 1);
		(void) pack_fclose(inf);
		free(gp);
		if(sync_points)
			free(sync_points);
		return NULL;
	}
	gp->fileversion = fileversion;	//Store this so that the unwrapping function can handle differing formatting for alternate endings
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
	if(dword > 1000)
	{	//Compare the notice entry count against an arbitrarily large number to satisfy Coverity
		eof_log("\t\t\tToo many notice entries, aborting.", 1);
		(void) pack_fclose(inf);
		free(gp);
		if(sync_points)
			free(sync_points);
		return NULL;
	}
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

		//The tempo string only exists in version 5.x or newer of the format
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
		pack_ReadDWORDLE(inf, &dword);	//Read the instrument patch number
		patches[ctr] = dword;			//Store the instrument patch for later reference
		(void) pack_getc(inf);			//Read the volume
		(void) pack_getc(inf);			//Read the pan value
		(void) pack_getc(inf);			//Read the chorus value
		(void) pack_getc(inf);			//Read the reverb value
		(void) pack_getc(inf);			//Read the phaser value
		(void) pack_getc(inf);			//Read the tremolo value
		pack_ReadWORDLE(inf, NULL);		//Read two bytes of unknown data/padding
	}
	if(fileversion >= 500)
	{	//Versions 5.0 and newer of the format store 19 musical directional symbols and a master reverb setting here
		for(ctr = 0; ctr < 19; ctr++)
		{	//For each of the musical directional symbols
			pack_ReadWORDLE(inf, &word);	//Read symbol position
			gp->symbols[ctr] = word;		//Store the measure number this symbol appears on
			if(word != 0xFFFF)
			{	//If the symbol's use is defined
				gp->symbols[ctr]--;			//Change the numbering to reflect the first measure being #0

#ifdef GP_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\"%s\" symbol is at measure #%u", musical_symbols[ctr], word);
				eof_log(eof_log_string, 1);
#endif
			}
		}

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
	else
	{	//Older versions of the GP format do not define the placement of these symbols
		for(ctr = 0; ctr < 19; ctr++)
		{	//For each of the musical directional symbols
			gp->symbols[ctr] = 0xFFFF;		//Track that this symbol is not in use
		}
	}


//Read track data
	pack_ReadDWORDLE(inf, &measures);	//Read the number of measures
#ifdef GP_IMPORT_DEBUG
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tNumber of measures: %lu", measures);
	eof_log(eof_log_string, 1);
#endif
	if(measures > 5000)
	{	//Compare the measure point count against an arbitrarily large number to satisfy Coverity
		eof_log("\t\t\tToo many measures, aborting.", 1);
		(void) pack_fclose(inf);
		free(gp);
		if(sync_points)
			free(sync_points);
		return NULL;
	}
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
	memset(tsarray, 0, sizeof(struct eof_gp_measure) * measures);	//Fill with 0s to satisfy Splint
	gp->measure = tsarray;		//Store this array into the Guitar Pro structure
	gp->measures = measures;	//Store the measure count as well
	pack_ReadDWORDLE(inf, &tracks);	//Read the number of tracks
#ifdef GP_IMPORT_DEBUG
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tNumber of tracks: %lu", tracks);
	eof_log(eof_log_string, 1);
#endif
	if(tracks >= 100)
	{	//Compare the track count against an arbitrarily large number to satisfy Coverity
		eof_log("\t\t\tToo many tracks, aborting.", 1);
		(void) pack_fclose(inf);
		free(gp);
		if(sync_points)
			free(sync_points);
		return NULL;
	}
	gp->numtracks = tracks;
	gp->names = malloc(sizeof(char *) * tracks);			//Allocate memory for track name strings
	gp->instrument_types = malloc(sizeof(char) * tracks);	//Allocate memory for the track instrument types
	np = malloc(sizeof(EOF_PRO_GUITAR_NOTE *) * tracks);	//Allocate memory for the array of last created notes
	hopo = malloc(sizeof(char) * tracks);					//Allocate memory for storing HOPO information
	hopobeatnum = malloc(sizeof(unsigned long) * tracks);	//Allocate memory for storing HOPO information
	hopomeasurenum = malloc(sizeof(unsigned long) * tracks);	//Allocate memory for storing HOPO information
	durations = malloc(sizeof(char) * tracks);				//Allocate memory for storing note lengths
	note_durations = malloc(sizeof(double) * tracks);		//Allocate memory for storing note durations
	if(!gp->names || !gp->instrument_types || !np || !hopo || !hopobeatnum || !hopomeasurenum || !durations || !note_durations)
	{
		eof_log("Error allocating memory (6)", 1);
		(void) pack_fclose(inf);
		if(gp->names)
			free(gp->names);
		if(gp->instrument_types)
			free(gp->instrument_types);
		if(np)
			free(np);
		if(hopo)
			free(hopo);
		if(hopobeatnum)
			free(hopobeatnum);
		if(hopomeasurenum)
			free(hopomeasurenum);
		if(durations)
			free(durations);
		if(note_durations)
			free(note_durations);
		free(gp);
		free(tsarray);
		if(sync_points)
			free(sync_points);
		return NULL;
	}
	memset(np, 0, sizeof(EOF_PRO_GUITAR_NOTE *) * tracks);				//Set all last created note pointers to NULL
	memset(hopo, -1, sizeof(char) * tracks);							//Set all tracks to have no HOPO status
	memset(hopobeatnum, 0, sizeof(unsigned long) * tracks);
	memset(hopomeasurenum, 0, sizeof(unsigned long) * tracks);
	memset(durations, 0, sizeof(char) * tracks);
	memset(note_durations, 0, sizeof(double) * tracks);
	memset(gp->instrument_types, 0, sizeof(char) * tracks);				//Set the instrument type for all tracks to undefined
	gp->track = malloc(sizeof(EOF_PRO_GUITAR_TRACK *) * tracks);		//Allocate memory for pro guitar track pointers
	gp->text_events = 0;
	if(!gp->track )
	{
		eof_log("Error allocating memory (7)", 1);
		(void) pack_fclose(inf);
		free(gp->names);
		free(gp->instrument_types);
		free(np);
		free(hopo);
		free(hopobeatnum);
		free(hopomeasurenum);
		free(durations);
		free(note_durations);
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
			free(gp->instrument_types);
			free(gp->track[ctr]);
			while(ctr > 0)
			{	//Free all previously allocated track structures
				free(gp->track[ctr - 1]);
				ctr--;
			}
			free(gp->track);	//Free array of track pointers
			free(np);
			free(hopo);
			free(hopobeatnum);
			free(hopomeasurenum);
			free(durations);
			free(note_durations);
			free(gp);
			free(tsarray);
			if(sync_points)
				free(sync_points);
			return NULL;
		}
		memset(gp->track[ctr], 0, sizeof(EOF_PRO_GUITAR_TRACK));	//Initialize memory block to 0 to avoid crashes when not explicitly setting counters that were newly added to the pro guitar structure
		gp->track[ctr]->numfrets = 22;
		gp->track[ctr]->note = gp->track[ctr]->pgnote;	//Put the regular pro guitar note array into effect
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
		free(gp->instrument_types);
		for(ctr = 0; ctr < tracks; ctr++)
		{	//Free all previously allocated track structures
			if(gp->track[ctr])
			{	//Redundant NULL check to satisfy Splint
				free(gp->track[ctr]);
			}
		}
		free(gp->track);
		free(np);
		free(hopo);
		free(hopobeatnum);
		free(hopomeasurenum);
		free(durations);
		free(note_durations);
		free(gp);
		free(tsarray);
		if(sync_points)
			free(sync_points);
		return NULL;
	}
	memset(strings, 0, sizeof(unsigned long) * tracks);	//Fill with 0s to satisfy Splint
#ifdef GP_IMPORT_DEBUG
	eof_log("\tParsing measure data", 1);
#endif
	for(ctr = 0; ctr < measures; ctr++)
	{	//For each measure
		int alt_endings = 0;
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
				alt_endings = pack_getc(inf);	//Read alternate ending number
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
			{	//Versions 3 and 4 define the alternate ending next, followed by the section definition, then the key signature
				if(bytemask & 16)
				{	//Number of alternate ending
					alt_endings = pack_getc(inf);	//Read alternate ending number
				}
				if(bytemask & 32)
				{	//New section
					(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read section string
#ifdef GP_IMPORT_DEBUG
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tSection marker found at measure #%lu:  \"%s\"", ctr + 1, buffer);
					eof_log(eof_log_string, 1);
#endif
					if((buffer[0] != '\0') && (gp->text_events < EOF_MAX_TEXT_EVENTS))
					{	//If the section marker has any text in its name and the maximum number of text events hasn't already been defined
						gp->text_event[gp->text_events] = malloc(sizeof(EOF_TEXT_EVENT));
						if(!gp->text_event[gp->text_events])
						{
							eof_log("Error allocating memory (10)", 1);
							(void) pack_fclose(inf);
							free(gp->names);
							free(gp->instrument_types);
							for(ctr = 0; ctr < tracks; ctr++)
							{	//Free all previously allocated track structures
								if(gp->track[ctr])
								{	//Redundant NULL check to satisfy Splint
									free(gp->track[ctr]);
								}
							}
							for(ctr = 0; ctr < gp->text_events; ctr++)
							{	//Free all allocated text events
								free(gp->text_event[ctr]);
							}
							free(gp->track);
							free(np);
							free(hopo);
							free(hopobeatnum);
							free(hopomeasurenum);
							free(durations);
							free(note_durations);
							free(gp);
							free(tsarray);
							if(sync_points)
								free(sync_points);
							return NULL;
						}
						gp->text_event[gp->text_events]->pos = ctr;	//For now, store the measure number, it will need to be converted to the beat number later
						gp->text_event[gp->text_events]->track = 0;
						rssectionname = eof_rs_section_text_valid(buffer);	//Determine whether this is a valid Rocksmith section name
						if(eof_gp_import_preference_1 || !rssectionname)
						{	//If the user preference is to import all section markers as RS phrases, or this section marker isn't validly named for a RS section anyway
							(void) ustrcpy(gp->text_event[gp->text_events]->text, buffer);
							gp->text_event[gp->text_events]->flags = EOF_EVENT_FLAG_RS_PHRASE;	//Ensure this will be detected as a RS phrase
						}
						else
						{	//Otherwise this section marker is valid as a RS section, then import it with the section's native name
							(void) ustrcpy(gp->text_event[gp->text_events]->text, rssectionname);
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
				if(bytemask & 64)
				{	//Key signature change
					(void) pack_getc(inf);	//Read the key
					(void) pack_getc(inf);	//Read the major/minor byte
				}
			}
			else
			{	//Version 5 and newer define these items in a different order and some other items afterward
				if(bytemask & 32)
				{	//New section
					(void) eof_read_gp_string(inf, NULL, buffer, 1);	//Read section string
#ifdef GP_IMPORT_DEBUG
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tSection marker found at measure #%lu:  \"%s\"", ctr + 1, buffer);
					eof_log(eof_log_string, 1);
#endif
					if((buffer[0] != '\0') && (gp->text_events < EOF_MAX_TEXT_EVENTS))
					{	//If the section marker has any text in its name and the maximum number of text events hasn't already been defined
						gp->text_event[gp->text_events] = malloc(sizeof(EOF_TEXT_EVENT));
						if(!gp->text_event[gp->text_events])
						{
							eof_log("Error allocating memory (11)", 1);
							(void) pack_fclose(inf);
							free(gp->names);
							free(gp->instrument_types);
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
							free(hopobeatnum);
							free(hopomeasurenum);
							free(durations);
							free(note_durations);
							free(gp);
							free(tsarray);
							if(sync_points)
								free(sync_points);
							return NULL;
						}
						memset(gp->text_event[gp->text_events], 0, sizeof(EOF_TEXT_EVENT));	//Fill with 0s to satisfy Splint
						gp->text_event[gp->text_events]->pos = ctr;	//For now, store the measure number, it will need to be converted to the beat number later
						gp->text_event[gp->text_events]->track = 0;
						rssectionname = eof_rs_section_text_valid(buffer);	//Determine whether this is a valid Rocksmith section name
						if(eof_gp_import_preference_1 || !rssectionname)
						{	//If the user preference is to import all section markers as RS phrases, or this section marker isn't validly named for a RS section anyway
							(void) ustrcpy(gp->text_event[gp->text_events]->text, buffer);
							gp->text_event[gp->text_events]->flags = EOF_EVENT_FLAG_RS_PHRASE;	//Ensure this will be detected as a RS phrase
						}
						else
						{	//Otherwise this section marker is valid as a RS section, then import it with the section's native name
							(void) ustrcpy(gp->text_event[gp->text_events]->text, rssectionname);
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
				if(bytemask & 64)
				{	//Key signature change
					(void) pack_getc(inf);	//Read the key
					(void) pack_getc(inf);	//Read the major/minor byte
				}
				if((bytemask & 1) || (bytemask & 2))
				{	//If either a new TS numerator or denominator was set, read the beam by eight notes values
					(void) pack_getc(inf);
					(void) pack_getc(inf);
					(void) pack_getc(inf);
					(void) pack_getc(inf);
				}
				if(bytemask & 16)
				{	//Number of alternate ending
					alt_endings = pack_getc(inf);	//Read alternate ending number
				}
				else
				{	//If a GP5 file doesn't define an alternate ending here, ignore a byte of padding
					(void) pack_getc(inf);
				}
				tripletfeel = pack_getc(inf);		//Read triplet feel value
				(void) pack_getc(inf);		//Unknown data
			}//Version 5 and newer define these items in a different order and some other items afterward
		}//Versions of the format 3.0 and newer
		if(bytemask & 128)
		{	//Double bar
		}
		tsarray[ctr].alt_endings = alt_endings;
		tsarray[ctr].num = curnum;	//Store this measure's time signature for future reference
		tsarray[ctr].den = curden;
		tsarray[ctr].start_of_repeat = start_of_repeat;
		tsarray[ctr].tripletfeel = tripletfeel;
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
			if(eof_song_append_beats(eof_song, 1))
				continue;	//If the beat was appended, add more as needed

			//Otherwise if the beat was not appended
			eof_log("Error allocating memory (12)", 1);
			(void) pack_fclose(inf);
			free(gp->names);
			free(gp->instrument_types);
			for(ctr = 0; ctr < tracks; ctr++)
			{	//Free all previously allocated track structures
				if(gp->track[ctr])
				{	//Redundant NULL check to satisfy Splint
					free(gp->track[ctr]);
				}
			}
			for(ctr = 0; ctr < gp->text_events; ctr++)
			{	//Free all allocated text events
				free(gp->text_event[ctr]);
			}
			free(gp->track);
			free(np);
			free(hopo);
			free(hopobeatnum);
			free(hopomeasurenum);
			free(durations);
			free(note_durations);
			free(gp);
			free(tsarray);
			free(strings);
			if(sync_points)
				free(sync_points);
			return NULL;
		}
		eof_chart_length = eof_song->beat[eof_song->beats - 1]->pos;	//Alter the chart length so that the full transcription will display
	}
	eof_clear_input();
	if(!sync_points)
	{	//Skip prompting to import time signatures if importing a Go PlayAlong file
		if(eof_use_ts && !eof_song->tags->tempo_map_locked)
		{	//If user has enabled the preference to import time signatures, and the project's tempo map isn't locked (skip the prompt if importing a Go PlayAlong file)
			eof_clear_input();
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
	else
	{	//Make an undo state before processing Go PlayAlong file, which will overwrite the tempo map
		if(undo_made && (*undo_made == 0))
		{	//If calling function wants to track an undo state being made if time signatures are imported into the project
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			*undo_made = 1;
		}
	}
	curnum = curden = 0;
	for(ctr = 0; ctr < measures; ctr++)
	{	//For each measure in the GP file
		for(ctr2 = 0; ctr2 < gp->text_events; ctr2++)
		{	//For each section marker that was imported
			if((gp->text_event[ctr2]->pos == ctr) && (!gp->text_event[ctr2]->is_temporary))
			{	//If the section marker was defined on this measure (and it hasn't been converted to use beat numbering yet
				gp->text_event[ctr2]->pos = beatctr;
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
					flags = eof_song->beat[beatctr]->flags & (EOF_BEAT_FLAG_ANCHOR | EOF_BEAT_FLAG_EVENTS | EOF_BEAT_FLAG_KEY_SIG);	//Keep these flags as-is
					eof_song->beat[beatctr]->flags = flags;
				}
			}
		}
		curnum = tsarray[ctr].num;
		curden = tsarray[ctr].den;
	}

	if(import_ts || sync_points)
	{	//If the user opted to replace the active project's TS changes, or if a Go PlayAlong file is being imported
		eof_calculate_beats(eof_song);	//Rebuild the tempo map to reflect any time signature changes
	}

//Correct Go PlayAlong timings by rebuilding the quarter note lengths
 	if(sync_points)
	{	//If synchronization data was imported from the input Go PlayAlong file
		double fbeatctr, qnotectr;
		unsigned char *num;	//Will point to an array with the TS numerator of every measure
		unsigned char *den;	//Will point to an array with the TS denominator of every measure
		unsigned long nummeasures;	//The number of entries in the above arrays
		double curpos = 0.0, beat_length = 500.0, last_qnote_length = 500.0;	//By default, assume 120BPM at 4/4 meter
		char mid_beat;	//Tracks whether the sync point applied to the beat was mid-beat
		unsigned long firstsyncpointbeat = 0;	//This will track the first beat to have its position set by a sync point

		eof_log("Unwrapping beats", 1);
		(void) eof_unwrap_gp_track(gp, 0, 1, 1);	//Unwrap all measures in the GP file, applying time signatures where appropriate.  No notes are imported yet, but allow text events to be unwrapped
		eof_process_beat_statistics(eof_song, eof_selected_track);		//Find the measure numbering for all beats

		//Build a temporary tsarray[], since GPA import needs one reflecting unwrapped measures, and the GP import that follows needs one reflecting the original wrapped measures
		nummeasures = eof_song->beat[eof_song->beats - 1]->measurenum;	//Derive the number of measures based off the last unwrapped beat's measure number
		num = (unsigned char *)malloc(sizeof(unsigned char) * nummeasures);	//Allocate memory for the arrays
		den = (unsigned char *)malloc(sizeof(unsigned char) * nummeasures);
		if(!num || !den)
		{
			if(num)
				free(num);
			if(den)
				free(den);
			eof_log("Error allocating memory (13)", 1);
			(void) pack_fclose(inf);
			free(gp->names);
			free(gp->instrument_types);
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
			free(hopobeatnum);
			free(hopomeasurenum);
			free(durations);
			free(note_durations);
			free(gp);
			free(tsarray);
			free(strings);
			free(sync_points);
			return NULL;
		}
		memset(num, 0, sizeof(char) * nummeasures);	//Fill with 0s to satisfy Splint
		memset(den, 0, sizeof(char) * nummeasures);
		for(ctr = 0; ctr < nummeasures; ctr++)
		{	//For each unwrapped measure
			for(ctr2 = 0; ctr2 < eof_song->beats; ctr2++)
			{	//For each beat in the project
				if(eof_song->beat[ctr2]->measurenum == ctr + 1)
				{	//If this beat is in the targeted measure
					num[ctr] = eof_song->beat[ctr2]->num_beats_in_measure;	//Populate the TS arrays with the measure information
					den[ctr] = eof_song->beat[ctr2]->beat_unit;
					break;
				}
			}
		}

		eof_log("Correcting sync point timings", 1);
		for(ctr = 0; ctr < num_sync_points; ctr++)
		{	//For each sync point from the Go PlayAlong file
			if(ctr + 1 >= num_sync_points)
			{	//If this is the last sync point, retain the original quarter note length specified in the XML file
				sync_points[ctr].real_qnote_length = sync_points[ctr].qnote_length;
				qnotectr = 0;
			}
			else
			{	//Otherwise base the quarter note length on the amount of time and number of beats between this next sync point and the next
				if(sync_points[ctr + 1].measure > sync_points[ctr].measure)
				{	//If the next sync point ends in a different measure
					fbeatctr = (1.0 - sync_points[ctr].pos_in_measure) * num[sync_points[ctr].measure];	//Initialize the counter to the number of beats between this sync point and the end of the measure it's in
					qnotectr = fbeatctr / ((double)den[sync_points[ctr].measure] / 4.0);	//Convert the beat counter to the number of quarter notes
					for(ctr2 = sync_points[ctr].measure + 1; ctr2 < sync_points[ctr + 1].measure; ctr2++)
					{	//For each remaining measure until the one the sync point is in
						fbeatctr += num[ctr2];	//Add this measure's number of beats to the counter
						qnotectr += num[ctr2] / ((double)den[ctr2] / 4.0);	//Add this measure's number of quarter notes to the counter
					}
					fbeatctr += sync_points[ctr + 1].pos_in_measure * num[sync_points[ctr + 1].measure];	//Add the number of beats into the measure the later sync point is
					qnotectr += (sync_points[ctr + 1].pos_in_measure * num[sync_points[ctr + 1].measure]) / ((double)den[sync_points[ctr + 1].measure] / 4.0);	//Add the number of quarter notes into the measure the later sync point is
				}
				else
				{	//This sync point and the next end in the same measure
					fbeatctr = sync_points[ctr + 1].pos_in_measure - sync_points[ctr].pos_in_measure;	//Get the distance between them in measures
					fbeatctr *= num[sync_points[ctr].measure];	//Convert this value to the number of beats between the sync points
					qnotectr = fbeatctr / ((double)den[sync_points[ctr].measure] / 4.0);	//Convert the beat counter to the number of quarter notes
				}
				sync_points[ctr].beat_length = ((double)sync_points[ctr + 1].realtime_pos - sync_points[ctr].realtime_pos) / fbeatctr;	//Get the beat length
				sync_points[ctr].real_qnote_length = (sync_points[ctr + 1].realtime_pos - sync_points[ctr].realtime_pos) / qnotectr;	//Get the quarter note length (the distance between this sync point and the next divided by the number of quarter notes between them)
			}
#ifdef GP_IMPORT_DEBUG
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tSync point #%lu:  Pos:  %lums\tMeasure:  %f\tQuarter note length:  %fms\t# of qnotes to next sync point:  %f\tCorrected length:  %fms", ctr, sync_points[ctr].realtime_pos, sync_points[ctr].measure + 1.0 + sync_points[ctr].pos_in_measure, sync_points[ctr].qnote_length, qnotectr, sync_points[ctr].real_qnote_length);
			eof_log(eof_log_string, 1);
#endif
		}//For each sync point from the Go PlayAlong file

//Apply Go PlayAlong timings now if applicable
		eof_log("Applying sync point timings", 1);
		for(ctr = 0; ctr < eof_song->beats; ctr++)
		{	//For each beat in the project
			mid_beat = 0;	//Reset this condition
			measure_position = (double)eof_song->beat[ctr]->beat_within_measure / (double)eof_song->beat[ctr]->num_beats_in_measure;	//Find this beat's position in the measure, as a value between 0 and 1
			for(ctr2 = 0; ctr2 < num_sync_points; ctr2++)
			{	//For each sync point from the Go PlayAlong file
				if(sync_points[ctr2].processed)
					continue;	//If the sync point was already handled, skip it

				if(sync_points[ctr2].measure + 1 == eof_song->beat[ctr]->measurenum)
				{	//If the sync point belongs in the same measure as this beat (GPA files number measures starting with 0)
					if(fabs(measure_position - sync_points[ctr2].pos_in_measure) < (1.0 / (double)eof_song->beat[ctr]->num_beats_in_measure) * 0.05)
					{	//If this sync point is close enough (within 5% of the beat's length) to this beat to be considered tied to the beat
#ifdef GP_IMPORT_DEBUG
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tSync point #%lu (On-beat):  Pos:  %lums\tMeasure:  %f\tQuarter note length:  %fms\tCorrected length:  %fms", ctr2, sync_points[ctr2].realtime_pos, sync_points[ctr2].measure + 1.0 + sync_points[ctr2].pos_in_measure, sync_points[ctr2].qnote_length, sync_points[ctr2].real_qnote_length);
						eof_log(eof_log_string, 1);
#endif
						eof_song->beat[ctr]->fpos = eof_song->beat[ctr]->pos = sync_points[ctr2].realtime_pos;	//Apply the timestamp
						curpos = sync_points[ctr2].realtime_pos;			//Update the ongoing position variable
						last_qnote_length = sync_points[ctr2].real_qnote_length;
						sync_points[ctr2].processed = 1;
						if(!ctr2)
						{	//If this is the first sync point
							firstsyncpointbeat = ctr;	//Track which beat was the first to be affected by any sync points
						}
						break;	//Exit sync point loop
					}
				}
				if((double)sync_points[ctr2].measure + 1.0 + sync_points[ctr2].pos_in_measure < (double)eof_song->beat[ctr]->measurenum + measure_position)
				{	//If the beat is beyond the sync point, the sync point is not within 5% of a beat line and is located between beats
					double temp;
#ifdef GP_IMPORT_DEBUG
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tSync point #%lu (Mid-beat):  Pos:  %lums\tMeasure:  %f\tQuarter note length:  %fms\tCorrected length:  %fms", ctr2, sync_points[ctr2].realtime_pos, sync_points[ctr2].measure + 1.0 + sync_points[ctr2].pos_in_measure, sync_points[ctr2].qnote_length, sync_points[ctr2].real_qnote_length);
					eof_log(eof_log_string, 1);
#endif
					//This is the first beat that surpassed this sync point, find out how far into last beat the sync point is, and use the beat length to derive the position of the next beat
					mid_beat = 1;
					temp = (double)eof_song->beat[ctr]->measurenum + measure_position - ((double)sync_points[ctr2].measure + 1.0 + sync_points[ctr2].pos_in_measure);	//The number of measures between this beat and the sync point before it
					temp *= (double)num[sync_points[ctr2].measure];	//The number of beats between this beat and the sync point before it
					curpos = sync_points[ctr2].realtime_pos + (temp * sync_points[ctr2].real_qnote_length);
					last_qnote_length = sync_points[ctr2].real_qnote_length;
					sync_points[ctr2].processed = 1;	//Mark this sync point as processed, but don't break from loop, so that if there are multiple sync points within the span of one beat, only the last one is used to alter beat timings
					if(!ctr2)
					{	//If this is the first sync point
						firstsyncpointbeat = ctr;	//Track which beat was the first to be affected by any sync points
					}
					break;	//Exit sync point loop
				}
				else
				{	//Otherwise if the beat is before the sync point
					break;
				}
			}//For each sync point from the Go PlayAlong file
			eof_song->beat[ctr]->fpos = curpos;							//Apply the current position to this beat
			eof_song->beat[ctr]->pos = eof_song->beat[ctr]->fpos + 0.5;	//Round up
			if(mid_beat)
			{	//If a mid beat sync point was processed
				ctr--;	//Rewind one beat in case the next sync point is also at or before this beat (happens if the user defines multiple sync points within the span of one beat)
			}
			else
			{	//Otherwise advance the position variable
				beat_length = last_qnote_length / ((double)eof_song->beat[ctr]->beat_unit / 4.0);
				curpos += beat_length;
			}
#ifdef GP_IMPORT_DEBUG
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tBeat #%lu set at pos = %lu, fpos = %f", ctr, eof_song->beat[ctr]->pos, eof_song->beat[ctr]->fpos);
			eof_log(eof_log_string, 1);
			if((ctr > 0) && (eof_song->beat[ctr - 1]->fpos >= eof_song->beat[ctr]->fpos))
			{	//If this beat isn't after the previous beat
				eof_log("\t\t\tError:  Invalid beat position.", 1);
			}
#endif
		}//For each beat in the project
		if(firstsyncpointbeat != 0)
		{	//If the first sync point wasn't placed at the first beat
			if(eof_song->beats > firstsyncpointbeat + 1)
			{	//If there are at least two beats positioned by the sync points
#ifdef GP_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tFirst sync point is not at measure 1.0.  Positioning the %lu preceding beats using the first sync point's quarter note length of %fms", firstsyncpointbeat, sync_points[0].real_qnote_length);
				eof_log(eof_log_string, 1);
#endif
				skipbeatsourcectr = firstsyncpointbeat;	//Store the number of beats that are being omitted from import
				for(ctr = firstsyncpointbeat; ctr > 0; ctr--)
				{	//For each beat before the first beat that was affected by any sync points, in reverse order
					beat_length = sync_points[0].real_qnote_length / ((double)eof_song->beat[ctr - 1]->beat_unit / 4.0);	//Determine the length of this beat based on the time signature in effect
					if(eof_song->beat[ctr]->fpos >= beat_length)
					{	//If going back one beat length doesn't result in a negative timestamp
						eof_song->beat[ctr - 1]->fpos = eof_song->beat[ctr]->fpos - beat_length;	//Place this beat one beat length behind the one that follows
						eof_song->beat[ctr - 1]->pos = eof_song->beat[ctr - 1]->fpos + 0.5;			//Round up
						skipbeatsourcectr--;	//Keep track of the number of beats that haven't been verified to be at or after 0ms
#ifdef GP_IMPORT_DEBUG
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tBeat #%lu reset to pos = %lu, fpos = %f", ctr - 1, eof_song->beat[ctr - 1]->pos, eof_song->beat[ctr - 1]->fpos);
						eof_log(eof_log_string, 1);
#endif
					}
					else
					{	//This beat is positioned before 0ms
						break;
					}
				}
				//Remove the appropriate number of beats from the front of the beat array so that beat 0 is the first one with a positive timestamp
				if(skipbeatsourcectr)
				{	//If any beats were positioned before 0ms
					allegro_message("Warning:  This Go PlayAlong file is synchronized in a way that puts one or more beats before the start of the audio, these beats will be omitted from the import.");
#ifdef GP_IMPORT_DEBUG
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t%lu beats are positioned before 0ms, they will be omitted from import", skipbeatsourcectr);
					eof_log(eof_log_string, 1);
#endif
					for(ctr = 0; ctr < skipbeatsourcectr; ctr++)
					{	//For each of the beats that need to be omitted during import since they are before 0ms
						unsigned tsnum = 0, tsden = 0;

						(void) eof_get_ts(eof_song, &tsnum, &tsden, 1);			//Store the time signature change at this beat, if there is one
						eof_song->beat[1]->flags = eof_song->beat[0]->flags;	//Retain the deleted beat's flags (ie. time signatures)
						if(tsnum)
						{	//If the now-first beat had a time signature change
							(void) eof_apply_ts(tsnum, tsden, 1, eof_song, 0);	//Re-apply it now so it isn't lost
						}
						eof_song_delete_beat(eof_song, 0);						//Delete the first beat in the project
					}
					//Re-correct the placement of any section markers that were previously imported
					for(ctr2 = 0; ctr2 < gp->text_events; ctr2++)
					{	//For each section marker that was imported
						if(gp->text_event[ctr2]->pos >= skipbeatsourcectr)
						{	//If the event can be moved back the appropriate number of beats
							gp->text_event[ctr2]->pos -= skipbeatsourcectr;	//Do so
						}
					}
					eof_cleanup_beat_flags(eof_song);	//Rebuild the beat flags so the correct beats are marked as having text events
				}
			}//If there are at least two beats positioned by the sync points
		}//If the first sync point wasn't placed at the first beat
		eof_song->tags->ogg[0].midi_offset = eof_song->beat[0]->pos;
		eof_song->tags->accurate_ts = 1;	//Enable the accurate TS option, since comparable logic was used to calculate sync point positions
		eof_calculate_tempo_map(eof_song);	//Update the tempo and anchor status of all beats
		free(sync_points);
		free(num);
		free(den);
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
			gp->instrument_types[ctr] = 3;	//Note that this is a drum track
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
		if(buffer[0] == '\0')
		{	//If the track has no name
			(void) snprintf(buffer, sizeof(buffer) - 1, "Track #%lu", ctr + 1);
		}
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
			free(gp->instrument_types);
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
			free(hopobeatnum);
			free(hopomeasurenum);
			free(durations);
			free(note_durations);
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
		(void) pack_fseek(inf, 40 - word);			//Skip the padding that follows the track name string
		pack_ReadDWORDLE(inf, &strings[ctr]);		//Read the number of strings in this track
		gp->track[ctr]->numstrings = strings[ctr];
#ifdef GP_IMPORT_DEBUG
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t%lu strings", strings[ctr]);
		eof_log(eof_log_string, 1);
#endif
		if(strings[ctr] > 7)
		{	//7 is the highest number of strings Guitar Pro supports for any track
			eof_log("Invalid string count", 1);
			(void) pack_fclose(inf);
			for(ctr = 0; ctr < tracks; ctr++)
			{	//Free the previous track name strings
				free(gp->names[ctr]);
			}
			free(gp->names);
			free(gp->instrument_types);
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
			free(hopobeatnum);
			free(hopomeasurenum);
			free(durations);
			free(note_durations);
			free(gp);
			free(tsarray);
			free(strings);
			return NULL;
		}
		if((gp->instrument_types[ctr] != 3) && (strings[ctr] > 6))
		{	//If this isn't a drum track, warn user that EOF will not import more than 6 strings if applicable
			gp->track[ctr]->numstrings = 6;
			if(!string_warning)
			{
				eof_clear_input();
				if(alert("Warning:  At least one track uses 7 strings.", "Only 6 can be used.  Drop which string?", "(If you want to use the standard 6 strings, drop string 7)", "&1 (thinnest)", "&7 (thickest)", '1', '7') == 2)
				{	//If the user opts to drop string 7
					drop_7 = 1;
				}
				else
				{
					drop_1 = 1;	//String 1 is being dropped
				}
				string_warning = 1;
			}
		}
		for(ctr2 = 0; ctr2 < 7; ctr2++)
		{	//For each of the 7 possible usable strings
			if(ctr2 < strings[ctr])
			{	//If this string is used
				pack_ReadDWORDLE(inf, &dword);	//Read the tuning for this string
#ifdef GP_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tTuning for string #%lu: MIDI note %lu (%s)", ctr2 + 1, dword, eof_note_names[(dword + 3) % 12]);
				eof_log(eof_log_string, 1);
#endif
				if(!drop_1 && (ctr2 < 6))
				{	//If the user isn't importing string 7, don't store the tuning for anything after the first six strings
					gp->track[ctr]->tuning[gp->track[ctr]->numstrings - 1 - ctr2] = dword;	//Store the absolute MIDI note, this will have to be converted to the track and string count specific relative value once mapped to an EOF instrument track (Guitar Pro stores the tuning in string order starting from #1, reversed from EOF)
				}
				else if(drop_1 && (ctr2 > 0))
				{	//If the user is importing string 7, don't import the first string
					gp->track[ctr]->tuning[gp->track[ctr]->numstrings - 1 - ctr2 + 1] = dword;	//Store the tuning for each string 1 index later, since string 2 is imported as string 1, etc.
				}
			}
			else
			{
				pack_ReadDWORDLE(inf, NULL);	//Skip this padding
			}
		}
		if(eof_track_is_drop_tuned(gp->track[ctr]))
		{	//If this track has a drop tuning
			gp->track[ctr]->ignore_tuning = 0;
		}
		pack_ReadDWORDLE(inf, &dword);	//Read the MIDI port used for this track
		pack_ReadDWORDLE(inf, &dword);	//Read the MIDI channel used for this track
		if((dword > 0) && (dword < 64))
		{	//Bounds check the value of dword
			int patchnum = patches[dword - 1];

			if((patchnum >=24) && (patchnum <= 31))
			{	//These are the defined guitar instrument numbers in Guitar Pro
				assert(gp->instrument_types != NULL);	//Unneeded check to resolve a false positive in Splint
				gp->instrument_types[ctr] = 1;
			}
			else if((patchnum >= 32) && (patchnum <= 39))
			{	//These are the defined bass guitar instrument numbers in Guitar Pro
				assert(gp->instrument_types != NULL);	//Unneeded check to resolve a false positive in Splint
				gp->instrument_types[ctr] = 2;
			}
		}
		pack_ReadDWORDLE(inf, &dword);	//Read the MIDI channel used for this track's effects
		pack_ReadDWORDLE(inf, &dword);	//Read the number of frets used for this track
#ifdef GP_IMPORT_DEBUG
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tNumber of frets: %lu", dword);
		eof_log(eof_log_string, 1);
#endif
		gp->track[ctr]->numfrets = dword;
		pack_ReadDWORDLE(inf, &dword);	//Read the capo position for this track
		gp->track[ctr]->capo = dword;	//Store it into the guitar pro structure
#ifdef GP_IMPORT_DEBUG
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tCapo position: %lu", dword);
		eof_log(eof_log_string, 1);
#endif
		(void) pack_getc(inf);						//Track color (Red intensity)
		(void) pack_getc(inf);						//Track color (Green intensity)
		(void) pack_getc(inf);						//Track color (Blue intensity)
		(void) pack_getc(inf);						//Read unused value
		if(fileversion > 500)
		{
			(void) pack_getc(inf);				//Track properties 1 bitmask
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
		if(tripletfeel != tsarray[ctr].tripletfeel)
		{	//If the triplet feel is not the same as the previous measure
			memset(durations, 0, sizeof(char) * tracks);	//Reset the triplet feel tracking
		}
		tripletfeel = tsarray[ctr].tripletfeel;	//Obtain the triplet feel for this measure

#ifdef GP_IMPORT_DEBUG
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tMeasure #%lu", ctr + 1);
		eof_log(eof_log_string, 1);
#endif

		for(ctr2 = 0; ctr2 < tracks; ctr2++)
		{	//For each track
			unsigned voice, maxvoices = 1;
			char effective_drop_7 = drop_7;	//By default, this will reflect the user's choice regarding 7 string guitar tracks

			if(gp->instrument_types[ctr2] == 3)
			{	//If this track is a drum track
				effective_drop_7 = 1;	//It is assumed that only the first 6 strings encode drum notes
			}
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
				if(beats > 1000)
				{	//Compare the beat count against an arbitrarily large number to satisfy Coverity
					eof_log("\t\t\tToo many beats (notes) in this measure, aborting.", 1);
					(void) pack_fclose(inf);
					for(ctr = 0; ctr < tracks; ctr++)
					{	//Free the previous track name strings
						free(gp->names[ctr]);
					}
					free(gp->names);
					free(gp->instrument_types);
					for(ctr = 0; ctr < tracks; ctr++)
					{	//Free all previously allocated track structures
						for(ctr2 = 0; ctr2 < gp->track[ctr]->notes; ctr2++)
						{	//Free all notes in this track
							free(gp->track[ctr]->note[ctr2]);
						}
						for(ctr2 = 0; ctr2 < gp->track[ctr]->technotes; ctr2++)
						{	//Free all tech notes in this track
							free(gp->track[ctr]->technote[ctr2]);
						}
						free(gp->track[ctr]);
					}
					for(ctr = 0; ctr < gp->text_events; ctr++)
					{	//Free all allocated text events
						free(gp->text_event[ctr]);
					}
					free(gp->track);
					free(np);
					free(hopo);
					free(hopobeatnum);
					free(hopomeasurenum);
					free(durations);
					free(note_durations);
					free(gp);
					free(tsarray);
					free(strings);
					return NULL;
				}
				for(ctr3 = 0; ctr3 < beats; ctr3++)
				{	//For each "beat" (note)
					unsigned char ghost = 0;	//Track the ghost status for notes
					unsigned char mute = 0;		//Track the mute status for notes
					unsigned char grace = 0;	//Track the note mask for grace notes
					unsigned char frets[7] = {0};		//Store fret values for each string
					unsigned char gracefrets[7] = {0};	//Store fret values for each string (for tracking grace notes)
					unsigned long beat_position;
					double partial_beat_position, beat_length;
					char notebends = 0;	//Tracks whether any bend points were parsed for the note, since they may be applied as tech notes instead of toward the regular note
					char isquarterorlonger = 0, isaltered = 0;	//Boolean statuses used to more accurately track whether the "GP import truncates short notes" should take effect
					char istuplet = 0;		//Set to nonzero if the note is explicitly in a tuplet (ie. triplet), which will cause any "triplet feel" notation to be ignored

					unpitchend = 0;	//Assume no unpitched slide unless one is defined
					new_note = 0;	//Assume no new note is to be added unless a normal/muted note is parsed
					tie_note = 0;	//Assume a note isn't a tie note unless found otherwise
					rest_note = 0;	//Assume a note isn't a rest note unless found otherwise
					bendstruct.bendpoints = 0;	//Assume the note has no bend points unless any are parsed
					tieflags = allflags = tflags = 0;
					bendstrength = 0;
					note_is_short = 0;
					memset(finger, 0, sizeof(finger));	//Clear the finger array
					bytemask = pack_getc(inf);	//Read beat bitmask
					if(bytemask & 64)
					{	//Beat is a rest
						(void) pack_getc(inf);	//Rest beat type (empty/rest)
						new_note = 0;
						rest_note = 1;
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
					if(byte <= 0)
					{	//If this is a quarter note or longer
						isquarterorlonger = 1;	//Track this
					}
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
							case 0:	//Invalid tuplet duration
								eof_log("\t\t\tWarning:  GP file defines an invalid \"0 tuplet\" notation.  Ignoring this.", 1);
							break;
							default:	//Otherwise assume the tuplet is n notes in the span of (n-1) of that note
								note_duration /= (double)dword / ((double)dword - 1.0);
							break;
						}
						isaltered = 1;	//A note notated as a quarter note may now no longer be as long as one
						istuplet = 1;	//Tuplet timing will override triplet feel logic where applicable
					}//Beat is an N-tuplet
					if(bytemask & 1)
					{	//Dotted note
						note_duration *= 1.5;	//A dotted note is one and a half times as long as normal
					}
					if(note_duration > 1.0)
					{	//Some GP files have encoding errors where an empty measure is written as a whole note rest, even when that is musically inaccurate (ie. non 4/4 time signature)
						note_duration = 1.0;	//Force the maximum length of a note to be 1 measure.  GP must correctly write tie notes to define longer notes
						eof_log("\t\t\tWarning:  GP file defines an invalid note duration, capping to 1 measure long.", 1);
					}
					if(tripletfeel && (byte == (durations[ctr2] & 0x3F)) && (byte == tripletfeel) && !istuplet)
					{	//If triplet feel is in effect, the note is the same length as the previously imported note for this track (masking out the MSB used to track whether that previous note was a rest and the second MSB used to track whether the previous note was dotted)
						//is the length of any configured triplet feel notation and is not explicitly in a tuplet set of notes, apply triplet feel modifications
						if(np[ctr2] && !(durations[ctr2] & 0x80))
						{	//If a previous note was imported and not a rest note
							np[ctr2]->length *= (4.0 / 3.0);	//Update the previous note to reflect triplet feel (it is made the length of two triplets)
						}
						measure_position += (note_durations[ctr2] / 3.0);	//Advance the measure position by an extra third of the previously imported note's length
						note_duration *= (2.0 / 3.0);		//Update the current note to reflect triplet feel (it is made the length of one triplet)
						durations[ctr2] = 0;	//Reset the triplet feel notation to require two notes
					}
					else if(tripletfeel && (durations[ctr2] & 0x40) && ((durations[ctr2] & 0x3F) + 1 == byte) && (byte == tripletfeel))
					{	//If triplet feel is in effect, the previous note was a dotted note, that dot notation added a length equivalent to the current note and the current note is the triplet feel value
						if(np[ctr2] && !(durations[ctr2] & 0x80))
						{	//If a previous note was imported and not a rest note
							double baselength, dottedlength;
							baselength = np[ctr2]->length * 2.0 / 3.0;		//Get the length of the previous note that was not affected by the dotted notation
							dottedlength = baselength / 2.0;				//Get the amount of the previous note that was increased by the dotted notation
							dottedlength *= (4.0 / 3.0);					//Update that dotted notation's length to to reflect triplet feel (it is made the length of two triplets)
							np[ctr2]->length = baselength + dottedlength;	//Update the previous note's length to reflect that the dotted portion is made the length of two triplets due to triplet feel
						}
						measure_position += (note_durations[ctr2] / 9.0);	//Advance the measure position by an extra third of the length that was added to the previously imported note by its dotted notation
						note_duration *= (2.0 / 3.0);		//Update the current note to reflect triplet feel (it is made the length of one triplet)
						durations[ctr2] = 0;	//Reset the triplet feel notation to require two notes
					}
					else
					{	//Otherwise just track the duration of the note being imported
						if(istuplet)
						{	//If this note was in a tuplet
							durations[ctr2] = 0;	//Reset the triplet feel notation to ignore this note and require two other ones
						}
						else
						{
							durations[ctr2] = byte;	//Track the duration of the note being imported for triplet feel
						}
						if(rest_note)
						{	//If this is a rest note
							durations[ctr2] |= 0x80;	//Set the MSB of this value to ensure EOF doesn't try to modify a note duration for a rest regarding triplet feel, since no note would have been created to represent a rest
						}
						if(bytemask & 1)
						{	//If the note was dotted
							durations[ctr2] |= 0x40;	//Set the second most significant bit of this value to track that the note is dotted, to track a special case where a dotted note can trigger triplet feel with the next note
						}
						note_durations[ctr2] = note_duration;
					}
					if(note_duration / ((double)curden / (double)curnum) < 0.25)
					{	//If the note (after undoing the scaling for the time signature) is shorter than a quarter note
						if(!(!isaltered && isquarterorlonger))
						{	//If the note was encoded as a quarter note and wasn't part of a tuplet, avoid math rounding errors causing the above logic to detect as shorter than a quarter note
							note_is_short = 1;
						}
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
							(void) pack_getc(inf);			//Read sharp/flat indicator
							(void) pack_fseek(inf, 3);		//Unknown data
							(void) pack_getc(inf);			//Read chord root
							if(fileversion / 100 == 3)
							{	//If it is a GP 3.x file
								(void) pack_fseek(inf, 3);	//Unknown data
							}
							(void) pack_getc(inf);			//Read chord type
							if(fileversion / 100 == 3)
							{	//If it is a GP 3.x file
								(void) pack_fseek(inf, 3);	//Unknown data
							}
							(void) pack_getc(inf);			//9th/11th/13th option
							if(fileversion / 100 == 3)
							{	//If it is a GP 3.x file
								(void) pack_fseek(inf, 3);	//Unknown data
							}
							pack_ReadDWORDLE(inf, &dword);	//Read bass note (lowest note played in string)
							(void) pack_getc(inf);			//+/- option
							(void) pack_fseek(inf, 4);		//Unknown data
							word = pack_getc(inf);			//Read chord name string length
							(void) pack_fread(buffer, 20, inf);	//Read chord name (which is padded to 20 bytes)
							buffer[word] = '\0';			//Ensure string is terminated to be the right length
							(void) pack_fseek(inf, 2);		//Unknown data
							(void) pack_getc(inf);			//Tonality of the fifth
							if(fileversion / 100 == 3)
							{	//If it is a GP 3.x file
								(void) pack_fseek(inf, 3);	//Unknown data
							}
							(void) pack_getc(inf);			//Tonality of the ninth
							if(fileversion / 100 == 3)
							{	//If it is a GP 3.x file
								(void) pack_fseek(inf, 3);	//Unknown data
							}
							(void) pack_getc(inf);			//Tonality of the eleventh
							if(fileversion / 100 == 3)
							{	//If it is a GP 3.x file
								(void) pack_fseek(inf, 3);	//Unknown data
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
							if(curbeat >= skipbeatsourcectr)
							{	//If this beat's content is being imported
								gp->text_event[gp->text_events] = malloc(sizeof(EOF_TEXT_EVENT));
								if(!gp->text_event[gp->text_events])
								{
									eof_log("Error allocating memory (14)", 1);
									(void) pack_fclose(inf);
									for(ctr = 0; ctr < tracks; ctr++)
									{	//Free the previous track name strings
										free(gp->names[ctr]);
									}
									free(gp->names);
									free(gp->instrument_types);
									for(ctr = 0; ctr < tracks; ctr++)
									{	//Free all previously allocated track structures
										for(ctr2 = 0; ctr2 < gp->track[ctr]->notes; ctr2++)
										{	//Free all notes in this track
											free(gp->track[ctr]->note[ctr2]);
										}
										for(ctr2 = 0; ctr2 < gp->track[ctr]->technotes; ctr2++)
										{	//Free all tech notes in this track
											free(gp->track[ctr]->technote[ctr2]);
										}
										free(gp->track[ctr]);
									}
									for(ctr = 0; ctr < gp->text_events; ctr++)
									{	//Free all allocated text events
										free(gp->text_event[ctr]);
									}
									free(gp->track);
									free(np);
									free(hopo);
									free(hopobeatnum);
									free(hopomeasurenum);
									free(durations);
									free(note_durations);
									free(gp);
									free(tsarray);
									free(strings);
									return NULL;
								}
								gp->text_event[gp->text_events]->pos = curbeat - skipbeatsourcectr;
								gp->text_event[gp->text_events]->track = 0;
								gp->text_event[gp->text_events]->is_temporary = 1;	//Track that the event's beat number has already been determined
								if(rssectionname)
								{	//If this beat text matches a valid Rocksmith section name, import it with the section's native name
									(void) ustrcpy(gp->text_event[gp->text_events]->text, rssectionname);
									gp->text_event[gp->text_events]->flags = EOF_EVENT_FLAG_RS_SECTION;	//Ensure this will be detected as a RS section
									gp->text_events++;
								}
								else if(!eof_gp_import_preference_1)
								{	//If the user preference to discard beat text that doesn't match a RS section isn't enabled, import it as a RS phrase
									(void) ustrcpy(gp->text_event[gp->text_events]->text, buffer);	//Copy the beat text as-is
									gp->text_event[gp->text_events]->flags = EOF_EVENT_FLAG_RS_PHRASE;	//Ensure this will be detected as a RS phrase
									gp->text_events++;
								}
								else
								{	//Otherwise discard this beat text
									free(gp->text_event[gp->text_events]);	//Free the memory allocated to store this text event
								}
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
							allflags |= EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO;
						}
						if(byte1 & 2)
						{	//Wide vibrato
							allflags |= EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO;
						}
						if(byte1 & 4)
						{	//Natural harmonic
							allflags |= EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC;
						}
						if(byte1 & 8)
						{	//Artificial harmonic
							if(!eof_gp_import_nat_harmonics_only)
							{	//If the user didn't opt to ignore harmonic status except for natural harmonics
								allflags |= EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC;
							}
						}
						if(byte1 & 32)
						{	//Tapping/popping/slapping
							byte = pack_getc(inf);	//Read tapping/popping/slapping indicator
							if(byte == 0)
							{	//Tremolo bar
							}
							else if(byte == 1)
							{	//Tapping
								allflags |= EOF_PRO_GUITAR_NOTE_FLAG_TAP;
							}
							else if(byte == 2)
							{	//Slapping
								allflags |= EOF_PRO_GUITAR_NOTE_FLAG_SLAP;
							}
							else if(byte == 3)
							{	//Popping
								allflags |= EOF_PRO_GUITAR_NOTE_FLAG_POP;
							}
							if(fileversion < 400)
							{
								pack_ReadDWORDLE(inf, &dword);	//String effect value
							}
						}
						if(byte2 & 4)
						{	//Tremolo bar
							if(eof_gp_parse_bend(inf, NULL))
							{	//If there was an error parsing the bend
								allegro_message("Error parsing bend, file is corrupt");
								(void) pack_fclose(inf);
								for(ctr = 0; ctr < tracks; ctr++)
								{	//Free the previous track name strings
									free(gp->names[ctr]);
								}
								free(gp->names);
								free(gp->instrument_types);
								for(ctr = 0; ctr < tracks; ctr++)
								{	//Free all previously allocated track structures
									for(ctr2 = 0; ctr2 < gp->track[ctr]->notes; ctr2++)
									{	//Free all notes in this track
										free(gp->track[ctr]->note[ctr2]);
									}
									for(ctr2 = 0; ctr2 < gp->track[ctr]->technotes; ctr2++)
									{	//Free all tech notes in this track
										free(gp->track[ctr]->technote[ctr2]);
									}
									free(gp->track[ctr]);
								}
								for(ctr = 0; ctr < gp->text_events; ctr++)
								{	//Free all allocated text events
									free(gp->text_event[ctr]);
								}
								free(gp->track);
								free(np);
								free(hopo);
								free(hopobeatnum);
								free(hopomeasurenum);
								free(durations);
								free(note_durations);
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
								allflags |= EOF_PRO_GUITAR_NOTE_FLAG_DOWN_STRUM;
							}
							else
							{	//Strum up
								allflags |= EOF_PRO_GUITAR_NOTE_FLAG_UP_STRUM;
							}
						}
						if(byte2 & 2)
						{	//Pickstroke effect
							byte = pack_getc(inf);	//Pickstroke effect (up/down)
							if(byte == 1)
							{	//Upward pick
								allflags |= EOF_PRO_GUITAR_NOTE_FLAG_UP_STRUM;
							}
							else if(byte == 2)
							{	//Downward pick
								allflags |= EOF_PRO_GUITAR_NOTE_FLAG_DOWN_STRUM;
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

					//Determine whether the note that was just parsed will be imported, and if so, its realtime start and end positions
					beat_position = measure_position * curnum;								//How many whole beats into the current measure the position is
					partial_beat_position = (measure_position * curnum) - beat_position;	//How far into this beat the note begins
					beat_position += curbeat;	//Add the number of beats into the track the current measure is

					if(beat_position >= skipbeatsourcectr)
					{	//If this beat's content is being imported
						importnote = 1;
						beat_position -= skipbeatsourcectr;	//Offset by the number of beats being skipped
						beat_length = eof_song->beat[beat_position + 1]->fpos - eof_song->beat[beat_position]->fpos;
						laststartpos = eof_song->beat[beat_position]->fpos + (beat_length * partial_beat_position);

						//Determine the realtime end position of the note that was just parsed
						beat_position = (measure_position + note_duration) * curnum;
						partial_beat_position = (measure_position + note_duration) * curnum - beat_position;	//How far into this beat the note ends
						beat_position += curbeat;	//Add the number of beats into the track the current measure is
						beat_position -= skipbeatsourcectr;	//Offset by the number of beats being skipped

						while(beat_position + 1 >= eof_song->beats)
						{	//If there aren't enough beats in the project for some reason, add enough to continue
							if(eof_song_append_beats(eof_song, 1))
								continue;	//If a beat was appended to the project, add more as needed

							//Otherwise if a beat couldn't be appended to the project
							eof_log("Error allocating memory (15)", 1);
							(void) pack_fclose(inf);
							for(ctr = 0; ctr < tracks; ctr++)
							{	//Free the previous track name strings
								free(gp->names[ctr]);
							}
							free(gp->names);
							free(gp->instrument_types);
							for(ctr = 0; ctr < tracks; ctr++)
							{	//Free all previously allocated track structures
								for(ctr2 = 0; ctr2 < gp->track[ctr]->notes; ctr2++)
								{	//Free all notes in this track
									free(gp->track[ctr]->note[ctr2]);
								}
								for(ctr2 = 0; ctr2 < gp->track[ctr]->technotes; ctr2++)
								{	//Free all tech notes in this track
									free(gp->track[ctr]->technote[ctr2]);
								}
								free(gp->track[ctr]);
							}
							for(ctr = 0; ctr < gp->text_events; ctr++)
							{	//Free all allocated text events
								free(gp->text_event[ctr]);
							}
							free(gp->track);
							free(np);
							free(hopo);
							free(hopobeatnum);
							free(hopomeasurenum);
							free(durations);
							free(note_durations);
							free(gp);
							free(tsarray);
							free(strings);
							return NULL;
						}//If there aren't enough beats in the project for some reason, add enough to continue

						beat_length = eof_song->beat[beat_position + 1]->fpos - eof_song->beat[beat_position]->fpos;
						lastendpos = eof_song->beat[beat_position]->fpos + (beat_length * partial_beat_position);
					}
					else
					{	//If this beat's content is NOT being imported because its timestamp is before 0ms
						importnote = 0;
					}

					usedstrings = pack_getc(inf);		//Used strings bitmask
					definedstrings = 0;	//Reset this bitmask, which will reflect which strings this note actually uses for playable notes, instead of what combination it and all other note effects (like grace notes) are in use
					usedtie = 0;	//Reset this bitmask
					for(ctr4 = 0, bitmask = 64; ctr4 < 7; ctr4++, bitmask >>= 1)
					{	//For each of the 7 possible usable strings
						char thisgemtype = 0;	//Tracks the note type for this string's gem (0 = undefined, 1 = normal, 2 = tie, 3 = dead (muted))
						flags = 0;

						if(!(bitmask & usedstrings))
							continue;	//If this string is not used, skip it

						bytemask = pack_getc(inf);	//Note bitmask
						if(bytemask & 32)
						{	//Note type is defined
							definedstrings |= bitmask;
							byte = pack_getc(inf);	//Note type (1 = normal, 2 = tie, 3 = dead (muted))
							thisgemtype = byte;
							if(byte == 1)
							{	//Normal note
								frets[ctr4] = 0;	//Ensure this is cleared
								new_note = 1;
								mute |= 2;	//Set bit 1
							}
							else if(byte == 2)
							{	//If this string is playing a tied note (it is still ringing from a previously played note)
								if(np[ctr2] && (np[ctr2]->note && bitmask))
								{	//If there is a previously created note, and it used this string, alter its length
									tie_note = 1;
									usedtie |= bitmask;	//Track that this string is a tie note
								}
							}
							else if(byte == 3)
							{	//If this string is muted
								frets[ctr4] = 0x80;	//Set the MSB, which is how EOF tracks muted status
								new_note = 1;
								mute |= 1;	//Set bit 0
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
							if(thisgemtype == 2)
							{	//If this gem is a tie note, recall the last fretting of this string in this track, since overlapping tie notes may prevent a simple check of the previous note from having the desired fret value
								unsigned int convertednum = strings[ctr2] - 1 - ctr4;	//Re-map from GP's string numbering to EOF's (EOF stores 8 fret values per note, it just only uses 6 by default)

								if((strings[ctr2] > 6) && effective_drop_7)
								{	//If this is a 7 string Guitar Pro track and the user opted to drop string 7 instead of string 1
									convertednum--;	//Remap so that string 7 is ignored and the other 6 are read
								}
								for(ctr5 = gp->track[ctr2]->notes; ctr5 > 0; ctr5--)
								{	//For each previous note created for this track
									if(gp->track[ctr2]->note[ctr5 - 1]->note & (1 << convertednum))
									{	//If the note has a gem on this string
										frets[ctr4] = gp->track[ctr2]->note[ctr5 - 1]->frets[convertednum];	//Copy the fret number for this string
										break;
									}
								}
							}
							else
							{	//Assign the defined fret value
								frets[ctr4] |= byte;	//OR this value, so that the muted status can be kept if it is set
							}
						}
						if((hopo[ctr2] >= 0) && ((ctr3 > hopobeatnum[ctr2]) || (ctr > hopomeasurenum[ctr2])))
						{	//If the previous note was marked as leading into a hammer on or pull off with the next (this) note
							if(byte < hopo[ctr2])
							{	//If this note is a lower fret than the previous note
								flags |= EOF_PRO_GUITAR_NOTE_FLAG_PO;	//This note is a pull off
							}
							else
							{	//Otherwise this note is a hammer on
								flags |= EOF_PRO_GUITAR_NOTE_FLAG_HO;
							}
							hopo[ctr2] = -1;	//Reset this status before it is checked if this note is marked (to signal the next note being HO/PO)
						}

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
								unsigned int convertednum = strings[ctr2] - 1 - ctr4;	//Re-map from GP's string numbering to EOF's (EOF stores 8 fret values per note, it just only uses 6 by default)

								if((strings[ctr2] > 6) && effective_drop_7)
								{	//If this is a 7 string Guitar Pro track and the user opted to drop string 7 instead of string 1
									convertednum--;	//Remap so that string 7 is ignored and the other 6 are read
								}
								if(eof_gp_parse_bend(inf, &bendstruct) || (bendstruct.bendpoints > 30))
								{	//If there was an error parsing the bend, or if bendpoints wasn't capped at 30 (a redundant check to satisfy a false positive in Coverity)
									allegro_message("Error parsing bend, file is corrupt");
									(void) pack_fclose(inf);
									for(ctr = 0; ctr < tracks; ctr++)
									{	//Free the previous track name strings
										free(gp->names[ctr]);
									}
									free(gp->names);
									free(gp->instrument_types);
									for(ctr = 0; ctr < tracks; ctr++)
									{	//Free all previously allocated track structures
										for(ctr2 = 0; ctr2 < gp->track[ctr]->notes; ctr2++)
										{	//Free all notes in this track
											free(gp->track[ctr]->note[ctr2]);
										}
										for(ctr2 = 0; ctr2 < gp->track[ctr]->technotes; ctr2++)
										{	//Free all tech notes in this track
											free(gp->track[ctr]->technote[ctr2]);
										}
										free(gp->track[ctr]);
									}
									for(ctr = 0; ctr < gp->text_events; ctr++)
									{	//Free all allocated text events
										free(gp->text_event[ctr]);
									}
									free(gp->track);
									free(np);
									free(hopo);
									free(hopobeatnum);
									free(hopomeasurenum);
									free(durations);
									free(note_durations);
									free(gp);
									free(tsarray);
									free(strings);
									return NULL;
								}
								//Don't mark entire chords with bend status, Guitar Pro defines which strings are to be bent
								if(eof_note_count_colors_bitmask(usedstrings) == 1)
								{	//If the note being parsed is not a chord
									flags |= EOF_PRO_GUITAR_NOTE_FLAG_BEND;
									notebends = 1;
								}
								if(bendstruct.summaryheight > 0)
								{	//If the GP file defines the bend of being at least one quarter step
									bendstrength = bendstruct.summaryheight;
									bendstrength |= 0x80;	//Mark the MSB to indicate this value is measured in quarter steps
									flags |= EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION;	//Indicate that the note has the bend height defined
								}
								if(convertednum <= 6)
								{	//Only if the converted string number is valid
									//Remove redundant bendpoints, any consecutive duplicate bend heights at the end of the bend have no effect and should be removed to avoid causing problems in Rocksmith
									for(ctr5 = bendstruct.bendpoints; ctr5 > 1; ctr5--)
									{	//For each bendpoint after the first, in reverse order
										if(bendstruct.bendheight[ctr5 - 1] == bendstruct.bendheight[ctr5 - 2])
										{	//If the bendpoint is the same height as the point before it
											bendstruct.bendpoints--;	//Discard it
										}
										else
											break;
									}
									for(ctr5 = 0; ctr5 < bendstruct.bendpoints; ctr5++)
									{	//For each bend point that was parsed
										EOF_PRO_GUITAR_NOTE *pgnp;
										unsigned long length;

										pgnp = eof_pro_guitar_track_add_tech_note(gp->track[ctr2]);	//Add a new tech note to the current track
										if(!pgnp)
										{
											eof_log("Error allocating memory (16)", 1);
											(void) pack_fclose(inf);
											for(ctr = 0; ctr < tracks; ctr++)
											{	//Free the previous track name strings
												free(gp->names[ctr]);
											}
											free(gp->names);
											free(gp->instrument_types);
											for(ctr = 0; ctr < tracks; ctr++)
											{	//Free all previously allocated track structures
												for(ctr2 = 0; ctr2 < gp->track[ctr]->notes; ctr2++)
												{	//Free all notes in this track
													free(gp->track[ctr]->note[ctr2]);
												}
												for(ctr2 = 0; ctr2 < gp->track[ctr]->technotes; ctr2++)
												{	//Free all tech notes in this track
													free(gp->track[ctr]->technote[ctr2]);
												}
												free(gp->track[ctr]);
											}
											for(ctr = 0; ctr < gp->text_events; ctr++)
											{	//Free all allocated text events
												free(gp->text_event[ctr]);
											}
											free(gp->track);
											free(np);
											free(hopo);
											free(hopobeatnum);
											free(hopomeasurenum);
											free(durations);
											free(note_durations);
											free(gp);
											free(tsarray);
											free(strings);
											return NULL;
										}
										pgnp->flags |= EOF_PRO_GUITAR_NOTE_FLAG_BEND;	//Set the bend flag
										notebends = 1;
										pgnp->flags |= EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION;	//Indicate that a bend strength is defined
										pgnp->bendstrength = 0x80 + bendstruct.bendheight[ctr5];	//Store the bend strength and set the MSB to indicate the value is in quarter steps
										pgnp->note = 1 << convertednum;	//Determine the note bitmask of this string
										if(bendstruct.bendpos[ctr5] == 60)
										{	//If this bend point is at the end of the note
											unsigned effective_min_note_distance;

											if(eof_min_note_distance_intervals)
											{	//Per beat/measure min. note distance could be too drastic of a padding to add to bend notes when the next note position isn't known yet
												effective_min_note_distance = 0;
											}
											else
											{
												effective_min_note_distance = eof_min_note_distance;
											}
											length = lastendpos - laststartpos - effective_min_note_distance;	//Take the minimum padding between notes into account to ensure it doesn't overlap the next note
										}
										else
										{	//Otherwise use the normal note length
											length = lastendpos - laststartpos;
										}
										pgnp->pos = laststartpos + ((length * (double)bendstruct.bendpos[ctr5]) / 60.0) + 0.5;	//Determine the position of the bend point, rounded to nearest ms
										pgnp->length = 1;
										pgnp->type = voice;		//Store lead voice notes in difficulty 0, bass voice notes in difficulty 1
									}//For each bend point that was parsed
								}//Only if the converted string number is valid
							}//Bend
							if(byte1 & 2)
							{	//Hammer on/pull off from current note (next note gets the HO/PO status)
								hopo[ctr2] = frets[ctr4] & 0x7F;	//Store the fret value (masking out the MSB ghost bit) so that the next note can be determined as either a HO or a PO
								hopobeatnum[ctr2] = ctr3;			//Track which beat (note) number has the HO/PO status
								hopomeasurenum[ctr2] = ctr;			//Track which measure number has the HO/PO status
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
								double grace_durations[] = {0.015625, 0.03125, 0.0625};	//The duration of each grace note length in terms of one whole note (64th, 32nd, 16th)
								double grace_duration, grace_position = measure_position;
								unsigned char dur;
								gracetrans = graceonbeat = 0;

								grace |= bitmask;	//Mark this string as having a grace note
								gracefrets[ctr4] = pack_getc(inf);	//Grace note fret number

								(void) pack_getc(inf);	//Grace note dynamic value
								if(fileversion >= 500)
								{	//If the file version is 5.x or higher (this byte verified not to be in 3.0 and 4.06 files)
									gracetrans = pack_getc(inf);	//Grace note transition type
								}
								else
								{	//The purpose of this field in 4.x or older files is unknown
									(void) pack_fseek(inf, 1);		//Unknown data
								}
								dur = pack_getc(inf) - 1;	//Grace note duration
								if(fileversion >= 500)
								{	//If the file version is 5.x or higher (this byte verified not to be in 3.0 and 4.06 files)
									graceonbeat = pack_getc(inf);	//Grace note position
									if(graceonbeat & 1)
									{	//If this grace note is string muted
										gracefrets[ctr4] |= 0x80;	//Set the mute bit
									}
									graceonbeat &= 2;	//Mask out everything except bit 1, which indicates whether the note is "on the beat" instead of before it
								}
								if(dur < 3)
								{	//If the defined duration is valid
									grace_duration = grace_durations[dur] * (double)curden / (double)curnum;	//Get this grace note's duration in measures (accounting for the time signature)
									if(!curbeat && measure_position < grace_duration)
									{	//If this grace note is positioned before the beginning of the chart (ie. a before the beat grace note on a note at the beginning of measure 1)
										grace = 0;	//Ignore this grace note
									}
									else
									{	//This grace note has a valid position
										if(!graceonbeat)
										{	//if the grace note is before the beat
											grace_position = measure_position - grace_duration;	//Reposition it accordingly
											tflags = EOF_NOTE_TFLAG_GRACE;	//Recognized this as a flam note if it is imported as a percussion track
										}
										beat_position = grace_position * curnum;								//How many whole beats into the current measure the position is
										partial_beat_position = (grace_position * curnum) - beat_position;	//How far into this beat the grace note begins
										beat_position += curbeat;	//Add the number of beats into the track the current measure is
										if(beat_position >= skipbeatsourcectr)
										{	//If this beat's content is being imported
											beat_position -= skipbeatsourcectr;	//Offset by the number of beats being skipped
											beat_length = eof_song->beat[beat_position + 1]->fpos - eof_song->beat[beat_position]->fpos;
											lastgracestartpos = eof_song->beat[beat_position]->fpos + (beat_length * partial_beat_position);

											//Determine the realtime end position of the note that was just parsed
											beat_position = (grace_position + grace_duration) * curnum;
											partial_beat_position = (grace_position + grace_duration) * curnum - beat_position;	//How far into this beat the grace note ends
											beat_position += curbeat;	//Add the number of beats into the track the current measure is
											beat_position -= skipbeatsourcectr;	//Offset by the number of beats being skipped
											beat_length = eof_song->beat[beat_position + 1]->fpos - eof_song->beat[beat_position]->fpos;
											lastgraceendpos = eof_song->beat[beat_position]->fpos + (beat_length * partial_beat_position);
										}
									}
								}//If the defined duration is valid
							}//Grace note
							if(byte2 & 1)
							{	//Note played staccato
								note_is_short = 1;
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
								if(fileversion < 500)
								{	//If the GP file is older than version 5
									flags &= ~(EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP | EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN);	//GP4 and older formats have a tendency to mark unpitched slides as "slide from note" as well, remove the redundant status set earlier
									if(byte == - 2)
									{	//Slide in from above
										flags |= EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE;
										flags |= EOF_NOTE_FLAG_HIGHLIGHT;
										if(!unpitchend || (frets[ctr4] < unpitchend))
										{	//Track the lowest fret value for the slide end position
											unpitchend = frets[ctr4];	//Set the end position of this slide at the authored note
										}
										frets[ctr4]++;				//Set the beginning of this slide one fret higher
										slide_in_from_warned++;		//Track that such a slide in was encountered
									}
									else if(byte == -1)
									{	//Slide in from below
										if(frets[ctr4] > 1 )
										{	//Don't allow this unless sliding into a fret higher than 1
											flags |= EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE;
											flags |= EOF_NOTE_FLAG_HIGHLIGHT;
											if(!unpitchend || (frets[ctr4] < unpitchend))
											{	//Track the lowest fret value for the slide end position
												unpitchend = frets[ctr4];	//Set the end position of this slide at the authored note
											}
											frets[ctr4]--;				//Set the beginning of this slide one fret lower
											slide_in_from_warned++;		//Track that such a slide in was encountered
										}
									}
									else if(byte == 1)
									{	//Shift slide
										flags |= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP | EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN;	//The slide direction is unknown and will be corrected later
									}
									else if(byte == 2)
									{	//If this is a legato slide
										flags |= EOF_PRO_GUITAR_NOTE_FLAG_LINKNEXT;	//Mark the current note with linknext status to accurately describe how to play it in Rocksmith
										flags |= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP | EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN;	//The slide direction is unknown and will be corrected later
									}
									else if(byte == 3)
									{	//Slide out and downward
										if(frets[ctr4] > 1 )
										{	//Don't apply a downward slide unless the note is at fret 2 or higher
											flags |= EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE;
											if(!unpitchend || (frets[ctr4] < unpitchend))
											{	//Track the lowest fret value for the slide end position
												unpitchend = frets[ctr4] - 1;
											}
										}
									}
									else if(byte == 4)
									{	//Slide out and upward
										flags |= EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE;
										if(!unpitchend || (frets[ctr4] < unpitchend))
										{	//Track the lowest fret value for the slide end position
											unpitchend = frets[ctr4] + 1;
										}
									}
								}
								else
								{	//Version 5 or newer GP file
									if(byte & 4)
									{	//This note slides out and downwards
										if(frets[ctr4] > 1 )
										{	//Don't apply a downward slide unless the note is at fret 2 or higher
											flags |= EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE;
											if(!unpitchend || (frets[ctr4] < unpitchend))
											{	//Track the lowest fret value for the slide end position
												unpitchend = frets[ctr4] - 1;
											}
										}
									}
									else if(byte & 8)
									{	//This note slides out and upwards
										flags |= EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE;
										if(!unpitchend || (frets[ctr4] < unpitchend))
										{	//Track the lowest fret value for the slide end position
											unpitchend = frets[ctr4] + 1;
										}
									}
									else if(byte & 16)
									{	//This note slides in from below
										if(frets[ctr4] > 1 )
										{	//Don't allow this unless sliding into a fret higher than 1
											flags |= EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE;
											flags |= EOF_NOTE_FLAG_HIGHLIGHT;
											if(!unpitchend || (frets[ctr4] < unpitchend))
											{	//Track the lowest fret value for the slide end position
												unpitchend = frets[ctr4];	//Set the end position of this slide at the authored note
											}
											frets[ctr4]--;				//Set the beginning of this slide one fret lower
											slide_in_from_warned++;		//Track that such a slide in was encountered
										}
									}
									else if(byte & 32)
									{	//This note slides in from above
										flags |= EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE;
										flags |= EOF_NOTE_FLAG_HIGHLIGHT;
										if(!unpitchend || (frets[ctr4] < unpitchend))
										{	//Track the lowest fret value for the slide end position
											unpitchend = frets[ctr4];	//Set the end position of this slide at the authored note
										}
										frets[ctr4]++;				//Set the beginning of this slide one fret higher
										slide_in_from_warned++;		//Track that such a slide in was encountered
									}
									else
									{
										if(byte & 2)
										{	//If this is a legato slide
											flags |= EOF_PRO_GUITAR_NOTE_FLAG_LINKNEXT;	//Mark the current note with linknext status to accurately describe how to play it in Rocksmith
										}
										flags |= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP | EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN;	//The slide direction is unknown and will be corrected later
									}
								}//Version 5 or newer GP file
								if(slide_in_from_warned == 1)
								{	//If this is the first slide in from above/below technique encountered, warn user
									allegro_message("Imported slide in from above/below notes will be highlighted, as Rocksmith does not directly support this technique.");
									slide_in_from_warned++;	//Change the value so this prompt isn't immediately triggered on the next loop iteration
								}
							}//Slide
							if(byte2 & 16)
							{	//Harmonic
								byte = pack_getc(inf);	//Harmonic type
								if(byte == 1)
								{	//Natural harmonic
									flags |= EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC;
								}
								else if(byte == 4)
								{	//Pinch harmonic
									flags |= EOF_PRO_GUITAR_NOTE_FLAG_P_HARMONIC;
								}
								else if(byte == 5)
								{	//"Semi" harmonic (similar to a pinch harmonic)
									flags |= EOF_PRO_GUITAR_NOTE_FLAG_P_HARMONIC;
								}
								else
								{	//Other harmonic types will only be applied to imported notes if the user didn't enable the preference to ignore them
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
									if(!eof_gp_import_nat_harmonics_only)
									{	//If the user opted to ignore harmonic status except for natural harmonics
										flags |= EOF_PRO_GUITAR_NOTE_FLAG_P_HARMONIC;
									}
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
						if(bytemask & 64)
						{	//Heavy accented or accented note
							flags |= EOF_PRO_GUITAR_NOTE_FLAG_ACCENT;
						}
						if((fileversion >= 500) && (bytemask & 2))
						{	//Heavy accented note (GP5 or higher only)
							flags |= EOF_PRO_GUITAR_NOTE_FLAG_ACCENT;
						}
						if((thisgemtype == 2) || tie_note)
						{	//If the note on this string was a tie note or the entire note itself was a tie
							tieflags |= flags;	//Add this string's flags to the bitmask tracking flags for this note's tie gems
						}
						allflags |= flags;	//Add this string's flags to the bitmask tracking flags for the entire note
					}//For each of the 7 possible usable strings
					flags = allflags;	//Beyond this point, the flags variable is expected to reflect the combined flags for all gems in this note
					if(fileversion >= 500)
					{	//Version 5.0 and higher of the file format stores a note transpose mask and unknown data here
						pack_ReadWORDLE(inf, &word);	//Transpose bitmask
						if(word & 0x800)
						{	//If bit 11 of the transpose bitmask was set, there is an additional byte of unknown data
							(void) pack_fseek(inf, 1);	//Unknown data
						}
					}
					if(importnote)
					{	//If this note is being imported
						if(tie_note)
						{	//If this note had one or more tie gems
							char newtech = 0;	//Is set to nonzero if the tie note is determined to add techniques to the note it is extending
							unsigned long dwbitmask;	//A double word bitmask, since 32 bits need to be tested

							for(ctr4 = 0, dwbitmask = 1; ctr4 < 32; ctr4++, dwbitmask <<= 1)
							{	//For each bit in the flags bitmask
								if(!(np[ctr2]->flags & dwbitmask) && (tieflags & dwbitmask))
								{	//If the previous note's flags bit was clear and this note's tie flags bit is not (only checking the flags on the tie gems)
									if(tieflags & ~(EOF_PRO_GUITAR_NOTE_FLAG_UP_STRUM | EOF_PRO_GUITAR_NOTE_FLAG_DOWN_STRUM))
									{	//If the flags were different when disregarding strum direction
										newtech = 1;	//This tie note adds a status and should import as a linked note
										break;
									}
								}
							}
							if(!newtech)
							{	//If the tie note doesn't enable a technique not in use by the previous note, alter the previous note's length to include the tie note
								long oldlength;
								unsigned int convertedtie = usedtie >> (7 - strings[ctr2]);	//Re-map from GP's string numbering to EOF's

								if(strings[ctr2] > 6)
								{	//If this is a 7 string track
									if(effective_drop_7)
									{	//The user opted to drop string 7 instead of string 1
										convertedtie |= usedtie >> 1;	//Shift out string 7 to leave the first 6 strings (merge the bitmask so that on-beat grace notes combine with the note they affect)
									}
									else
									{	//The user opted to drop string 1
										convertedtie |= usedtie & 63;	//Mask out string 1 (merge the bitmask so that on-beat grace notes combine with the note they affect)
									}
								}
								else
								{	//This track has less than 7 strings
									convertedtie |= usedtie >> (7 - strings[ctr2]);	//Guitar pro's note bitmask reflects string 7 being the LSB (merge the bitmask so that on-beat grace notes combine with the note they affect)
								}

								//Search backward for correct note to alter
								eof_pro_guitar_track_sort_notes(gp->track[ctr2]);	//Sort the track to ensure that before-the-beat grace notes are before the notes that they affect
								for(ctr4 = gp->track[ctr2]->notes; ctr4 > 0; ctr4--)
								{	//For each imported note in this track, in reverse order
									if(gp->track[ctr2]->note[ctr4 - 1]->note & convertedtie)
									{	//If the note uses any of the same gems as the tie note
										oldlength = gp->track[ctr2]->note[ctr4 - 1]->length;
										gp->track[ctr2]->note[ctr4 - 1]->length = lastendpos - gp->track[ctr2]->note[ctr4 - 1]->pos + 0.5;	//Round up to nearest millisecond

#ifdef GP_IMPORT_DEBUG
										(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\tTie note:  Note starting at %lums lengthened from %ldms to %ldms", gp->track[ctr2]->note[ctr4 - 1]->pos, oldlength, gp->track[ctr2]->note[ctr4 - 1]->length);
										eof_log(eof_log_string, 1);
#endif
										break;
									}
								}

								definedstrings &= ~usedtie;
								usedstrings &= ~usedtie;	//Clear the bits used to indicate the tie notes' strings as being played, since overlapping guitar notes isn't supported in Rock Band or Rocksmith
															//This line had to be changed to not run when a tie note creates a new note, because the note bitmask needs to be intact in that condition
							}
							else
							{	//Otherwise create a new note, since the tie note changes the techniques in use
								tie_note = 0;	//Reset this to prevent this note's flags and bend strength from overriding those of the previous note
								new_note = 1;	//Set this to nonzero, the if() block below will create the note
								np[ctr2]->flags |= EOF_PRO_GUITAR_NOTE_FLAG_LINKNEXT;	//Set the linknext flag on the previous note
							}
						}//If this was defined as a tie note

						if(new_note)
						{	//If a new note is to be created
							char truncate = 0;	//Is set to nonzero if any conditions are met that should cause the note's sustain to be removed

							np[ctr2] = eof_pro_guitar_track_add_note(gp->track[ctr2]);	//Add a new note to the current track
							if(!np[ctr2])
							{
								eof_log("Error allocating memory (17)", 1);
								(void) pack_fclose(inf);
								for(ctr = 0; ctr < tracks; ctr++)
								{	//Free the previous track name strings
									free(gp->names[ctr]);
								}
								free(gp->names);
								free(gp->instrument_types);
								for(ctr = 0; ctr < tracks; ctr++)
								{	//Free all previously allocated track structures
									for(ctr2 = 0; ctr2 < gp->track[ctr]->notes; ctr2++)
									{	//Free all notes in this track
										free(gp->track[ctr]->note[ctr2]);
									}
									for(ctr2 = 0; ctr2 < gp->track[ctr]->technotes; ctr2++)
									{	//Free all tech notes in this track
										free(gp->track[ctr]->technote[ctr2]);
									}
									free(gp->track[ctr]);
								}
								for(ctr = 0; ctr < gp->text_events; ctr++)
								{	//Free all allocated text events
									free(gp->text_event[ctr]);
								}
								free(gp->track);
								free(np);
								free(hopo);
								free(hopobeatnum);
								free(hopomeasurenum);
								free(durations);
								free(note_durations);
								free(gp);
								free(tsarray);
								free(strings);
								return NULL;
							}
							np[ctr2]->flags = flags;
							for(ctr4 = 0; ctr4 < strings[ctr2]; ctr4++)
							{	//For each of this track's natively supported strings
								unsigned int convertednum = strings[ctr2] - 1 - ctr4;	//Re-map from GP's string numbering to EOF's (EOF stores 8 fret values per note, it just only uses 6 by default)
								if((strings[ctr2] > 6) && effective_drop_7)
								{	//If this is a 7 string Guitar Pro track and the user opted to drop string 7 instead of string 1
									convertednum--;	//Remap so that string 7 is ignored and the other 6 are read
								}
								if((convertednum > 6) || (ctr4 > 5))
								{	//If convertednum became an unexpectedly large value (ie. integer underflow) or six strings have already been processed
									break;	//Stop translating fretting and fingering data
								}
								np[ctr2]->frets[ctr4] = frets[convertednum];	//Copy the fret number for this string
								np[ctr2]->finger[ctr4] = finger[convertednum];	//Copy the finger number used to fret the string (is nonzero if defined)
							}
							if(flags & EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE)
							{	//If the note had an unpitched slide
								np[ctr2]->unpitchend = unpitchend;	//Apply the end position that was previously determined
								if(unpitchend > gp->track[ctr2]->numfrets)
								{	//If this unpitched slide requires the fret limit to be increased
									gp->track[ctr2]->numfrets++;		//Make it so
								}
							}
							np[ctr2]->legacymask = 0;
							np[ctr2]->midi_length = 0;
							np[ctr2]->midi_pos = 0;
							np[ctr2]->name[0] = '\0';
							if(strings[ctr2] > 6)
							{	//If this is a 7 string track
								if(effective_drop_7)
								{	//The user opted to drop string 7 instead of string 1
									np[ctr2]->note = definedstrings >> 1;	//Shift out string 7 to leave the first 6 strings
									np[ctr2]->ghost = ghost >> 1;		//Likewise translate the ghost bit mask
								}
								else
								{	//The user opted to drop string 1
									np[ctr2]->note = definedstrings & 63;	//Mask out string 1
									np[ctr2]->ghost = ghost & 63;		//Likewise mask out string 1 of the ghost bit mask
								}
							}
							else
							{	//This track has less than 7 strings
								np[ctr2]->note = definedstrings >> (7 - strings[ctr2]);	//Guitar pro's note bitmask reflects string 7 being the LSB
								np[ctr2]->ghost = ghost >> (7 - strings[ctr2]);			//Likewise translate the ghost bit mask
							}
							np[ctr2]->type = voice;					//Store lead voice notes in difficulty 0, bass voice notes in difficulty 1
							np[ctr2]->bendstrength = bendstrength;	//Apply the note's bend strength if applicable

							//Set the start position and length of the note
							np[ctr2]->pos = laststartpos + 0.5;	//Round up to nearest millisecond
							np[ctr2]->length = lastendpos - laststartpos + 0.5;	//Round up to nearest millisecond

							if(note_is_short)
							{	//If this note is shorter than a quarter note
								if(!(np[ctr2]->flags & EOF_NOTE_FLAG_IS_TREMOLO))
								{	//If this note doesn't have tremolo status
									if((eof_note_count_colors_bitmask(np[ctr2]->note) == 1) && eof_gp_import_truncate_short_notes)
									{	//If this is a single note and the preference to drop the note's sustain in this circumstance is enabled
										truncate = 1;
									}
									else if((eof_note_count_colors_bitmask(np[ctr2]->note) > 1) && eof_gp_import_truncate_short_chords)
									{	//If this is a chord and the preference to drop the note's sustain in this circumstance is enabled
										truncate = 1;
									}
								}
							}
							if((mute == 1) || (np[ctr2]->flags & EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE))
							{	//If this note is entirely string muted (only bit 0 of the mute variable was set, ie. no non-muted notes) or is palm muted
								truncate = 1;
							}
							if(truncate)
							{	//If the above conditions were triggered
								if(!(notebends) && !(np[ctr2]->flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP) && !(np[ctr2]->flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN) && !(np[ctr2]->flags & EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO) && !(np[ctr2]->flags & EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE))
								{	//If this note doesn't have bend, slide, vibrato or unpitched slide status
									np[ctr2]->length = 1;	//Remove the note's sustain
								}
							}

#ifdef GP_IMPORT_DEBUG
							(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tNote #%lu:  Start: %lums\tLength: %ldms\tFrets: ", gp->track[ctr2]->notes - 1, np[ctr2]->pos, np[ctr2]->length);
							assert(strings[ctr2] < 8);	//Redundant assertion to resolve a false positive in Coverity
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
									(void) strncat(temp, ", ", sizeof(temp) - strlen(temp) - 1);
								}
								(void) strncat(eof_log_string, temp, sizeof(eof_log_string) - strlen(eof_log_string) - 1);
							}
							eof_log(eof_log_string, 1);
#endif
						}//If a new note is to be created
						else if(np[ctr2] && tie_note)
						{	//Otherwise if this was a tie note
							np[ctr2]->flags |= flags;	//Apply this tie note's flags to the previous note
							if(bendstrength > np[ctr2]->bendstrength)
							{	//If this tie note defined a stronger bend than the original note
								np[ctr2]->bendstrength = bendstrength;	//Apply this tie note's bend strength
							}
						}

						if(grace)
						{	//If a new grace note is to be created
							EOF_PRO_GUITAR_NOTE *gnp;

							gnp = eof_pro_guitar_track_add_note(gp->track[ctr2]);	//Add a new note to the current track
							if(!gnp)
							{
								eof_log("Error allocating memory (18)", 1);
								(void) pack_fclose(inf);
								for(ctr = 0; ctr < tracks; ctr++)
								{	//Free the previous track name strings
									free(gp->names[ctr]);
								}
								free(gp->names);
								free(gp->instrument_types);
								for(ctr = 0; ctr < tracks; ctr++)
								{	//Free all previously allocated track structures
									for(ctr2 = 0; ctr2 < gp->track[ctr]->notes; ctr2++)
									{	//Free all notes in this track
										free(gp->track[ctr]->note[ctr2]);
									}
									for(ctr2 = 0; ctr2 < gp->track[ctr]->technotes; ctr2++)
									{	//Free all tech notes in this track
										free(gp->track[ctr]->technote[ctr2]);
									}
									free(gp->track[ctr]);
								}
								for(ctr = 0; ctr < gp->text_events; ctr++)
								{	//Free all allocated text events
									free(gp->text_event[ctr]);
								}
								free(gp->track);
								free(np);
								free(hopo);
								free(hopobeatnum);
								free(hopomeasurenum);
								free(durations);
								free(note_durations);
								free(gp);
								free(tsarray);
								free(strings);
								return NULL;
							}
							gnp->tflags = tflags;	//Track the grace note status for the sake of being able to treat as flam notation for percussion tracks
							if(strings[ctr2] > 6)
							{	//If this is a 7 string track
								if(effective_drop_7)
								{	//The user opted to drop string 7 instead of string 1
									gnp->note |= grace >> 1;	//Shift out string 7 to leave the first 6 strings (merge the bitmask so that on-beat grace notes combine with the note they affect)
								}
								else
								{	//The user opted to drop string 1
									gnp->note |= grace & 63;	//Mask out string 1 (merge the bitmask so that on-beat grace notes combine with the note they affect)
								}
							}
							else
							{	//This track has less than 7 strings
								gnp->note |= grace >> (7 - strings[ctr2]);	//Guitar pro's note bitmask reflects string 7 being the LSB (merge the bitmask so that on-beat grace notes combine with the note they affect)
							}
							gnp->type = voice;	//Store lead voice notes in difficulty 0, bass voice notes in difficulty 1
							for(ctr4 = 0, bitmask = 1; ctr4 < strings[ctr2]; ctr4++, bitmask <<= 1)
							{	//For each of this track's natively supported strings
								unsigned int convertednum = strings[ctr2] - 1 - ctr4;	//Re-map from GP's string numbering to EOF's (EOF stores 8 fret values per note, it just only uses 6 by default)
								if((strings[ctr2] > 6) && effective_drop_7)
								{	//If this is a 7 string Guitar Pro track and the user opted to drop string 7 instead of string 1
									convertednum--;	//Remap so that string 7 is ignored and the other 6 are read
								}
								if((convertednum > 6) || (ctr4 > 5))
								{	//If convertednum became an unexpectedly large value (ie. integer underflow) or six strings have already been processed
									break;	//Stop translating fretting and fingering data
								}
								if(!graceonbeat || gracefrets[convertednum])
								{	//If this isn't an on-beat grace note (or if it is, but the fret value is nonzero)
									//If an on-beat grace note is applied to a string that has no normal note, GP authors an open string to accommodate the grace note
									//and the grace note defines an open fret for the string that the normal note uses.  This check ensures that the open
									//notes don't override the real fret values
									gnp->frets[ctr4] = gracefrets[convertednum];	//Copy the fret number for this string
									gnp->finger[ctr4] = finger[convertednum];		//Copy the finger number used to fret the note the grace note is applied to
								}
								if(graceonbeat || !(gnp->note & bitmask))
									continue;	//If this is an on beat grace note or if this string isn't used by the grace note, skip it

								if(gracetrans == 1)
								{	//Grace note slides into normal note
									gnp->flags |= EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION;	//Define the grace note's end of slide position
									gnp->slideend = np[ctr2]->frets[ctr4];				//The end of slide position is the fret position of the note the grace note affects
									gnp->flags |= EOF_PRO_GUITAR_NOTE_FLAG_LINKNEXT;	//Add linknext status to the grace note so it appears nicely as a slide-in in Rocksmith
									if(gnp->frets[ctr4] > np[ctr2]->frets[ctr4])
									{	//If the grace note's fret value is higher than the normal note's
										gnp->flags |= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN;	//The grace note slides down
									}
									else
									{	//The grace note's fret value is lower than the normal note's
										gnp->flags |= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP;	//The grace note slides up
									}
								}
								else if(gracetrans == 2)
								{	//Grace note bends to normal note
									if(gnp->frets[ctr4] < np[ctr2]->frets[ctr4])
									{	//This only makes sense if the grace note's fret value is lower than the normal note's
										gnp->flags |= EOF_PRO_GUITAR_NOTE_FLAG_BEND;		//The grace note bends up to the next note
										gnp->flags |= EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION;	//Define the grace note's end of slide position
										gnp->bendstrength = np[ctr2]->frets[ctr4] - gnp->frets[ctr4];	//Set the bend strength to the number of half steps needed to reach the normal note
									}
								}
								else if(gracetrans == 3)
								{	//Grace note is hammered onto or pulled off to perform the note it precedes
									if(gnp->frets[ctr4] > np[ctr2]->frets[ctr4])
									{	//If the grace note's fret value is higher than the normal note's
										np[ctr2]->flags |= EOF_PRO_GUITAR_NOTE_FLAG_PO;	//The grace note is pulled off of
									}
									else
									{	//The grace note's fret value is lower than the normal note's
										np[ctr2]->flags |= EOF_PRO_GUITAR_NOTE_FLAG_HO;	//The grace note is hammered on from
									}
								}
							}//For each of this track's natively supported strings

							//Set the start position and length of the note
							gnp->pos = lastgracestartpos + 0.5;	//Round up to nearest millisecond
							gnp->length = lastgraceendpos - lastgracestartpos + 0.5;	//Round up to nearest millisecond

							if(graceonbeat)
							{	//If this grace note displaces the note it is associated with
								np[ctr2]->pos += gnp->length;			//Delay the note by the length of the grace note
								if(np[ctr2]->length >= gnp->length)
									np[ctr2]->length -= gnp->length;	//Shorten the note by the same length if possible
							}

							if((gracetrans != 1) && (gracetrans != 2))
							{	//If this grace note didn't transition with a bend or slide technique
								if((eof_note_count_colors_bitmask(gnp->note) == 1) && eof_gp_import_truncate_short_notes)
								{	//If this grace note is a single note and the preference to drop the note's sustain in this circumstance is enabled
									gnp->length = 1;
								}
								else if((eof_note_count_colors_bitmask(gnp->note) > 1) && eof_gp_import_truncate_short_chords)
								{	//If this grace note is a chord and the preference to drop the note's sustain in this circumstance is enabled
									gnp->length = 1;
								}
							}
#ifdef GP_IMPORT_DEBUG
							(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tGrace note #%lu:  Start: %lums\tLength: %ldms\tFrets: ", gp->track[ctr2]->notes - 1, gnp->pos, gnp->length);
							assert(strings[ctr2] < 8);	//Redundant assertion to resolve a false positive in Coverity
							for(ctr4 = 0, bitmask = 1; ctr4 < strings[ctr2]; ctr4++, bitmask <<= 1)
							{	//For each of this track's natively supported strings
								char temp[10];
								if(gnp->note & bitmask)
								{	//If this string is used
									(void) snprintf(temp, sizeof(temp) - 1, "%u", gnp->frets[ctr4]);
								}
								else
								{
									(void) snprintf(temp, sizeof(temp) - 1, "_");
								}
								if(ctr4 + 1 < strings[ctr2])
								{	//If there is another string
									(void) strncat(temp, ", ", sizeof(temp) - strlen(temp) - 1);
								}
								(void) strncat(eof_log_string, temp, sizeof(eof_log_string) - strlen(eof_log_string) - 1);
							}
							eof_log(eof_log_string, 1);
#endif
						}//If a new grace note is to be created
					}//If this note is being imported
					measure_position += note_duration;	//Update the measure position
				}//For each "beat" (note)
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
			if(!(gp->track[ctr]->note[ctr2]->flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP) || !(gp->track[ctr]->note[ctr2]->flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN) || (ctr2 + 1 >= gp->track[ctr]->notes))
				continue;	//If this not isn't marked as being an undetermined direction of slide, or there's no note that follows, skip it

			startfret = 0;
			for(ctr3 = 0, bitmask = 1; ctr3 < 7; ctr3++, bitmask<<=1)
			{	//For each of the 7 strings the GP format allows for
				if(!(bitmask & gp->track[ctr]->note[ctr2]->note) || !(bitmask & gp->track[ctr]->note[ctr2 + 1]->note))
					continue;	//If this string is not used for both this note and the next, skip it

				startfret = eof_pro_guitar_note_lowest_fret(gp->track[ctr], ctr2);	//Record the lowest used fret on this note
				if(gp->track[ctr]->note[ctr2 + 1]->frets[ctr3] & 0x80)
				{	//If the end of the slide is defined by a mute note
					endfret =  0;	//Force it to be a downward slide, the end fret of the slide will be undefined
				}
				else
				{
					endfret = eof_pro_guitar_note_lowest_fret(gp->track[ctr], ctr2 + 1);	//Record the lowest used fret on the next note
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
			}//For each of the 7 strings the GP format allows for
			if(!(gp->track[ctr]->note[ctr2]->flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP) || !(gp->track[ctr]->note[ctr2]->flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN))
				continue;	//If either of the up/down slide flags aren't set, skip this note

			//Otherwise if both are set, the next note didn't use any of the same strings as the slide note
			//Base the slide direction on the lowest fret value of that next note
			startfret = eof_pro_guitar_note_lowest_fret(gp->track[ctr], ctr2);
			endfret = eof_pro_guitar_note_lowest_fret(gp->track[ctr], ctr2 + 1);
			if(startfret == endfret)
				continue;	//If the slide and the following note start at the same fret position, skip this note as a slide direction can't be determined

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
		}//For each note in the track
	}//For each imported track

//Remove string muted notes from chords that contain at least one non muted note
	for(ctr = 0; ctr < gp->numtracks; ctr++)
	{	//For each imported track
		for(ctr2 = gp->track[ctr]->notes; ctr2 > 0; ctr2--)
		{	//For each note in the track (in reverse order)
			if(!eof_is_partially_string_muted(gp->track[ctr], ctr2 - 1))
				continue;	//If this note doesn't have at least one muted and one non muted gem, skip it

			for(ctr3 = 0, bitmask = 1; ctr3 < 6; ctr3++, bitmask <<= 1)
			{	//For each of the 6 supported strings
				if(ctr3 < gp->track[ctr]->numstrings)
				{	//If this is a string used in the track
					if(gp->track[ctr]->note[ctr2 - 1]->note & bitmask)
					{	//If this is a string used in the note
						if(gp->track[ctr]->note[ctr2 - 1]->frets[ctr3] & 0x80)
						{	//If this string is muted
							gp->track[ctr]->note[ctr2 - 1]->note &= ~bitmask;	//Clear this string's gem
							gp->track[ctr]->note[ctr2 - 1]->frets[ctr3] = 0;	//Erase the fret value
						}
					}
				}
			}
		}
	}

//Unwrap repeat sections if necessary
	for(ctr = 0; ctr < gp->numtracks; ctr++)
	{	//For each imported track
		if(eof_unwrap_gp_track(gp, ctr, import_ts, 0))
		{	//If the track failed to unwrap
			allegro_message("Warning:  Failed to unwrap repeats for track #%lu (%s).", ctr + 1, gp->names[ctr]);
		}
		import_ts = 0;	//Only unwrap the time signatures on the first pass
	}

//Create trill phrases
	for(ctr = 0; ctr < gp->numtracks; ctr++)
	{	//For each imported track
		eof_build_trill_phrases(gp->track[ctr]);	//Add trill phrases to encompass the notes that have the trill flag set
	}

//Create tremolo phrases
	for(ctr = 0; ctr < gp->numtracks; ctr++)
	{	//For each imported track
		if((eof_song->track[eof_selected_track]->flags & EOF_TRACK_FLAG_UNLIMITED_DIFFS) || !eof_gp_import_replaces_track)
		{	//If the active project's active track already had the difficulty limit removed, or if the user preference is to import the GP file into the active track difficulty instead of replacing the whole track
			eof_build_tremolo_phrases(gp->track[ctr], 0);	//Add tremolo phrases defined in the lead voice
			eof_build_tremolo_phrases(gp->track[ctr], 1);	//Add tremolo phrases defined in the rhythm voice
		}
		else
		{	//Otherwise it will apply to all track difficulties
			eof_build_tremolo_phrases(gp->track[ctr], 0xFF);	//Add tremolo phrases and define them to apply to all track difficulties
		}
	}//For each imported track

//Validate any imported note/chord fingerings and duplicate defined fingerings to matching notes
	for(ctr = 0; ctr < gp->numtracks; ctr++)
	{	//For each imported track
		eof_pro_guitar_track_fix_fingerings(gp->track[ctr], NULL, 0);	//Erase partial note fingerings, replicate valid finger definitions (within imported track only) to matching notes without finger definitions, don't make any additional undo states for corrections to the imported track
	}

//Clean up
	(void) pack_fclose(inf);
	free(strings);
	free(tsarray);
	free(np);
	free(hopo);
	free(hopobeatnum);
	free(hopomeasurenum);
	free(durations);
	free(note_durations);
	(void) puts("\nSuccess");
	return gp;
}

int eof_unwrap_gp_track(struct eof_guitar_pro_struct *gp, unsigned long track, char import_ts, char beats_only)
{
	unsigned long ctr, currentmeasure, beatctr, last_start_of_repeat;
	unsigned char has_repeats = 0, has_symbols = 0;
	unsigned char *working_num_of_repeats = NULL;	//Will store a copy of gp->measure[]'s repeat information so that the information in gp isn't destroyed
	EOF_PRO_GUITAR_TRACK *tp;		//Stores the resulting unwrapped track that is inserted into gp
	unsigned long *measuremap;		//Will be used to refer to a dynamically built array storing the beat number at which each measure begins
	EOF_TEXT_EVENT * newevent[EOF_MAX_TEXT_EVENTS] = {0};	//Will be used to rebuild the text events array to the appropriate unwrapped beat positions, if the first track is being unwrapped
	unsigned long newevents = 0;		//The number of events stored in the above array
	char unwrapevents = 0;		//Will track whether text events are to be unwrapped
	unsigned int working_symbols[19];	//Will store a copy of gp->symbols[] so that the information ingp isn't destroyed
	unsigned int curr_repeat = 0;
	char in_alt_ending;
	unsigned char curnum = 4, curden = 4;	//Tracks the time signature of the most recently unwrapped measure
	EOF_SONG *dsp = NULL;		//A new working song structure needs to be created because unwrapping time signature changes would corrupt the wrapped beat map and note timings

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

	//Create and initialize a working song structure to store the unwrapped beat map
	dsp = eof_create_song();
	if(!dsp)
	{
		eof_log("\tFailed to create destination song structure", 1);
		return 2;	//Return failure
	}

	//Create a working copy of the array tracking the number of repeats left for each measure
	working_num_of_repeats = malloc(sizeof(unsigned char) * gp->measures);
	if(!working_num_of_repeats)
	{	//Error allocating memory
		eof_log("\tError allocating memory to unwrap GP track (1)", 1);
		eof_destroy_song(dsp);	//Destroy working project
		return 1;
	}
	memset(working_num_of_repeats, 0, sizeof(unsigned char) * gp->measures);	//Fill with 0s to satisfy Splint
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
		eof_destroy_song(dsp);	//Destroy working project
		return 2;
	}
	memset(tp, 0, sizeof(EOF_PRO_GUITAR_TRACK));	//Initialize memory block to 0 to avoid crashes when not explicitly setting counters that were newly added to the pro guitar structure
	tp->numfrets = gp->track[track]->numfrets;
	tp->numstrings = gp->track[track]->numstrings;
	tp->note = tp->pgnote;	//Put the regular pro guitar note array into effect
	tp->parent = NULL;
	memcpy(tp->tuning, gp->track[track]->tuning, sizeof(char) * EOF_TUNING_LENGTH);

	//Build an array tracking the beat number marking the start of each measure
	measuremap = malloc(sizeof(unsigned long) * gp->measures);	//Allocate enough memory to store this information for each measure imported from the gp file
	if(!measuremap)
	{
		eof_log("\tError allocating memory to unwrap GP track (3)", 1);
		free(tp);
		free(working_num_of_repeats);
		eof_destroy_song(dsp);	//Destroy working project
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

	//Clone the active project's beat map
	for(ctr = 0; ctr < eof_song->beats; ctr++)
	{	//For each beat in the active project
		if(ctr >= dsp->beats)
		{	//If the working project doesn't have enough beats
			if(!eof_song_add_beat(dsp))
			{	//If another beat couldn't be added
				eof_log("\tError allocating memory to unwrap GP track (4)", 1);
				free(tp);
				free(working_num_of_repeats);
				free(measuremap);
				eof_destroy_song(dsp);	//Destroy working project
				return 4;
			}
		}
		memcpy(dsp->beat[ctr], eof_song->beat[ctr], sizeof(EOF_BEAT_MARKER));
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
			if(last_start_of_repeat != currentmeasure)
			{	//If this measure begins a new start of repeat
				curr_repeat = 0;	//Reset the repeat counter
				last_start_of_repeat = currentmeasure;	//Track this measure as the most recent start of repeat
			}
		}

		in_alt_ending = 0;	//Reset this status
		if((gp->fileversion < 400) || (gp->fileversion >= 500))
		{	//Versions 3.x and older and 5.x of the GP format defined alternate endings as a bitmask
			if(gp->measure[currentmeasure].alt_endings & (1 << curr_repeat))
			{	//If the alternate ending value matches the current repeat count
				in_alt_ending = 1;
			}
		}
		else
		{	//Version 4.x stores a number indicating the highest pass that plays the measure?
			if(gp->measure[currentmeasure].alt_endings >= curr_repeat + 1)
			{
				in_alt_ending = 1;
			}
		}
		if(!gp->measure[currentmeasure].alt_endings || in_alt_ending)
		{	//If this measure isn't part of an alternate ending, or if it is an alternate ending and the repeat count applies to it
			//Ensure the working project has enough beats to hold the next unwrapped measure
			if(dsp->beats < beatctr + gp->measure[currentmeasure].num + 2)
			{
#ifdef GP_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tUnwrapping measure #%lu requires %lu beats, but project only has %lu beats.  Appending %lu beats to the project to allow for this measure and two beats of padding.", currentmeasure + 1, beatctr + gp->measure[currentmeasure].num, dsp->beats, beatctr + gp->measure[currentmeasure].num + 2 - dsp->beats);
				eof_log(eof_log_string, 1);
#endif
			}
			while(dsp->beats < beatctr + gp->measure[currentmeasure].num + 2)
			{	//Until the working project has enough beats to contain the unwrapped measure, and two extra to allow room for processing beat lengths later
				if(!eof_song_append_beats(dsp, 1))
				{
					eof_log("\tError allocating memory to unwrap GP track (5)", 1);
					free(measuremap);
					free(tp);
					free(working_num_of_repeats);
					for(ctr = 0; ctr < newevents; ctr++)
					{	//For each unwrapped text event
						free(newevent[ctr]);	//Free it
					}
					eof_destroy_song(dsp);	//Destroy working project
					return 5;
				}
			}

			//Update the time signature if necessary
			if(import_ts)
			{
				if(!beatctr || (gp->measure[currentmeasure].num != curnum) || (gp->measure[currentmeasure].den != curden))
				{	//If this is the first beat being unwrapped or this measure's time signature is different from that of the previously unwrapped measure
					curnum = gp->measure[currentmeasure].num;
					curden = gp->measure[currentmeasure].den;
					(void) eof_apply_ts(curnum, curden, beatctr, dsp, 0);	//Apply the change to the working project
				}
			}

			//Copy the contents in this measure to the new pro guitar track
			if(!beats_only)
			{	//If the track content besides just the beats and time signatures is being unwrapped
				unsigned long oldnotecount;
#ifdef GP_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tUnwrapping %u beats from measure #%lu (starts at beat %lu) from original track to beat %lu in new track", gp->measure[currentmeasure].num, currentmeasure + 1, measuremap[currentmeasure], beatctr);
				eof_log(eof_log_string, 1);
#endif
				oldnotecount = tp->notes;
				if(!eof_copy_notes_in_beat_range(eof_song, gp->track[track], measuremap[currentmeasure], gp->measure[currentmeasure].num, dsp, tp, beatctr))
				{	//If there was an error unwrapping the repeat into the new pro guitar track
					eof_log("\tError unwrapping beats (normal notes)", 1);
					free(measuremap);
					for(ctr = 0; ctr < tp->notes; ctr++)
					{	//For each note that had been allocated for the unwrapped track
						free(tp->note[ctr]);	//Free its memory
					}
					free(tp);
					free(working_num_of_repeats);
					for(ctr = 0; ctr < newevents; ctr++)
					{	//For each unwrapped text event
						free(newevent[ctr]);	//Free it
					}
					eof_destroy_song(dsp);	//Destroy working project
					return 6;
				}
				if(tp->notes > oldnotecount)
				{	//If notes were unwrapped
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t%lu notes unwrapped", tp->notes - oldnotecount);
					eof_log(eof_log_string, 1);
				}
				eof_menu_pro_guitar_track_set_tech_view_state(gp->track[track], 1);	//Enable tech view for source track
				eof_menu_pro_guitar_track_set_tech_view_state(tp, 1);				//Enable tech view for destination track
				oldnotecount = tp->notes;
				if(!eof_copy_notes_in_beat_range(eof_song, gp->track[track], measuremap[currentmeasure], gp->measure[currentmeasure].num, dsp, tp, beatctr))
				{	//If there was an error unwrapping tech notes within the repeat into the new pro guitar track
					eof_log("\tError unwrapping beats (tech notes)", 1);
					free(measuremap);
					for(ctr = 0; ctr < tp->notes; ctr++)
					{	//For each note that had been allocated for the unwrapped track
						free(tp->note[ctr]);	//Free its memory
					}
					free(tp);
					free(working_num_of_repeats);
					for(ctr = 0; ctr < newevents; ctr++)
					{	//For each unwrapped text event
						free(newevent[ctr]);	//Free it
					}
					eof_menu_pro_guitar_track_set_tech_view_state(gp->track[track], 0);	//Disable tech view for source track
					eof_destroy_song(dsp);	//Destroy working project
					return 7;
				}
				if(tp->notes > oldnotecount)
				{	//If notes were unwrapped
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t%lu tech notes unwrapped", tp->notes - oldnotecount);
					eof_log(eof_log_string, 1);
				}
				eof_menu_pro_guitar_track_set_tech_view_state(gp->track[track], 0);	//Disable tech view for source track
				eof_menu_pro_guitar_track_set_tech_view_state(tp, 0);				//disable tech view for destination track
			}

			//If this measure has a text event, and text events are being unwrapped, copy it to the new text event array
			if(unwrapevents && !beats_only)
			{	//If text events are being unwrapped
				for(ctr = 0; ctr < gp->text_events; ctr++)
				{	//For each text event that was imported from the gp file
					if(gp->text_event[ctr]->pos != measuremap[currentmeasure])
						continue;	//If the text event is not positioned at this measure, skip it

					if(newevents >= EOF_MAX_TEXT_EVENTS)
						continue;	//If the max number of text events has already been defined, skip adding this one

					newevent[newevents] = malloc(sizeof(EOF_TEXT_EVENT));	//Allocate space for the text event
					if(!newevent[newevents])
					{
						eof_log("\tError allocating memory to unwrap GP track (5)", 1);
						free(measuremap);
						for(ctr = 0; ctr < tp->notes; ctr++)
						{	//For each note that had been allocated for the unwrapped track
							free(tp->note[ctr]);	//Free its memory
						}
						free(tp);
						free(working_num_of_repeats);
						for(ctr = 0; ctr < newevents; ctr++)
						{	//For each unwrapped text event
							free(newevent[ctr]);	//Free it
						}
						eof_destroy_song(dsp);	//Destroy working project
						return 8;
					}
					memcpy(newevent[newevents], gp->text_event[ctr], sizeof(EOF_TEXT_EVENT));	//Copy the text event
					newevent[newevents]->pos = beatctr;	//Correct the beat number
					newevents++;
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
				if(!in_alt_ending)
				{	//As long as this measure isn't used in an alternate ending (depending on how the GP file is authored, multiple alternate endings may rely on the same end of repeat)
					working_num_of_repeats[currentmeasure]--;	//Decrement the number of repeats left for this marker
				}
				currentmeasure = last_start_of_repeat;	//Jump to the last start of repeat that was encountered
				curr_repeat++;
			}
			else
			{	//This measure is not an end of repeat with any repeats left
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
			}//This measure is not an end of repeat with any repeats left
		}//If this measure isn't part of an alternate ending, or if it is an alternate ending and the repeat count applies to it
		else
		{	//This measure is the beginning of a different alternate ending, seek until the next relevant measure is reached
			unsigned char curr_alt_ending;

#ifdef GP_IMPORT_DEBUG
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tMeasure #%lu is the start of another alternate ending (mask = %u), skipping to the end of the alternate ending", currentmeasure + 1, gp->measure[currentmeasure].alt_endings);
			eof_log(eof_log_string, 1);
#endif
			curr_alt_ending = gp->measure[currentmeasure].alt_endings;	//Remember the alternate ending number being skipped
			while(currentmeasure < gp->measures)
			{	//While there are more measures
				//Based on Guitar Pro's behavior, even when a time signature change is in an inactive alternate ending, the composition's meter takes it into effect
				if(import_ts)
				{
					if(!currentmeasure || (gp->measure[currentmeasure].num != gp->measure[currentmeasure - 1].num) || (gp->measure[currentmeasure].den != gp->measure[currentmeasure - 1].den))
					{	//If this is the first measure or this measure's time signature is different from that of the previous measure
						curnum = gp->measure[currentmeasure].num;
						curden = gp->measure[currentmeasure].den;
						(void) eof_apply_ts(curnum, curden, beatctr, dsp, 0);	//Apply the change to the working project
					}
				}
				if(gp->measure[currentmeasure].num_of_repeats)
				{	//If this is the first end of repeat that was reached, the scope of the alternate ending is over
					currentmeasure++;	//Go beyond the end of repeat to the next measure
					break;
				}
				if(gp->measure[currentmeasure].alt_endings && (curr_alt_ending != gp->measure[currentmeasure].alt_endings))
				{	//If this is the beginning of another alternate ending
					break;
				}
				currentmeasure++;	//Iterate to the next measure
			}
		}//This measure is not the current alternate ending, seek until the correct alternate ending is found
	}//Continue until all repeats of all measures have been processed

	//Replace the target pro guitar track with the new one
	for(ctr = 0; ctr < gp->track[track]->notes; ctr++)
	{	//For each note in the target track
		free(gp->track[track]->note[ctr]);	//Free its memory
	}
	for(ctr = 0; ctr < gp->track[track]->technotes; ctr++)
	{	//For each tech note in the target track
		free(gp->track[track]->technote[ctr]);	//Free its memory
	}
	free(gp->track[track]);	//Free the pro guitar track
	gp->track[track] = tp;	//Insert the new pro guitar track into the array

	//Replace the active project's beat map with that of the working project
	for(ctr = 0; ctr < dsp->beats; ctr++)
	{	//For each beat in the working project
		if(ctr >= eof_song->beats)
		{	//If the active project doesn't have enough beats
			if(!eof_song_add_beat(eof_song))
			{	//If another beat couldn't be added
				eof_log("\tError allocating memory to unwrap GP track (9)", 1);
				free(tp);
				free(working_num_of_repeats);
				free(measuremap);
				eof_destroy_song(dsp);	//Destroy working project
				return 9;
			}
		}
		memcpy(eof_song->beat[ctr], dsp->beat[ctr], sizeof(EOF_BEAT_MARKER));
	}
	eof_chart_length = eof_song->beat[eof_song->beats - 1]->pos;	//Alter the chart length so that the full transcription will display

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
	eof_destroy_song(dsp);	//Destroy working project

	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tGP track successfully unwrapped (%lu notes in total)", gp->track[track]->notes);
	eof_log(eof_log_string, 1);
	eof_log("\tGP track successfully unwrapped", 1);
	return 0;
}

char eof_copy_notes_in_beat_range(EOF_SONG *ssp, EOF_PRO_GUITAR_TRACK *source, unsigned long startbeat, unsigned long numbeats, EOF_SONG *dsp, EOF_PRO_GUITAR_TRACK *dest, unsigned long destbeat)
{
	unsigned long ctr;
	unsigned long beatnum, endbeatnum;
	long newpos, newend;
	double notepos, noteendpos;

	eof_log("eof_copy_notes_in_beat_range() entered", 2);

	if(!ssp || !source || !dsp || !dest || (startbeat + numbeats >= ssp->beats) || (destbeat + numbeats >= dsp->beats))
	{
		eof_log("\tInvalid parameters", 1);
		return 0;	//Return error
	}

	for(ctr = 0; ctr < source->notes; ctr++)
	{	//For each note in the source track
		if(source->note[ctr]->pos >= ssp->beat[startbeat + numbeats]->pos)
			break;		//If this note (and all remaining notes) are after the target range of beats, exit loop
		if(source->note[ctr]->pos < ssp->beat[startbeat]->pos)
			continue;	//If this note is before or after the target range of beats, skip it

		beatnum = eof_get_beat(ssp, source->note[ctr]->pos);					//Find which beat this note is within
		endbeatnum = eof_get_beat(ssp, source->note[ctr]->pos + source->note[ctr]->length);	//Find which beat this note ends within
		if(!eof_beat_num_valid(ssp, beatnum) || !eof_beat_num_valid(ssp, endbeatnum))
			continue;	//If the beat positions were not found, skip this note

		if(dsp->beats < destbeat + endbeatnum - startbeat + 2)
		{
#ifdef GP_IMPORT_DEBUG
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tUnwrapping note #%lu requires %lu beats, but project only has %lu beats.  Appending %lu beats to the project to allow for this note and 2 bytes of padding.", ctr, destbeat + endbeatnum - startbeat, dsp->beats, destbeat + endbeatnum - startbeat + 2 - dsp->beats);
			eof_log(eof_log_string, 1);
#endif
			while(dsp->beats < destbeat + endbeatnum - startbeat + 2)
			{	//Until the destination project has enough beats to contain the unwrapped note, and two extra to allow room for processing beat lengths later
				if(!eof_song_append_beats(dsp, 1))
				{
					eof_log("\tError allocating memory to unwrap GP track (6)", 1);
					return 0;	//Return error
				}
			}
			eof_chart_length = dsp->beat[dsp->beats - 1]->pos;	//Alter the chart length so that the full transcription will display
		}

		notepos = eof_get_porpos_sp(ssp, source->note[ctr]->pos);									//Get the note's position as a percentage within its beat
		noteendpos = eof_get_porpos_sp(ssp, source->note[ctr]->pos + source->note[ctr]->length);	//Get the note end's end position as a percentage within its beat
		newpos = eof_put_porpos_sp(dsp, destbeat + beatnum - startbeat, notepos, 0.0);				//Get the position for the copied note
		newend = eof_put_porpos_sp(dsp, destbeat + endbeatnum - startbeat, noteendpos, 0.0);		//Get the end position for the copied note
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
	}//For each note in the source track

	return 1;	//Return success
}

int eof_get_next_gpa_sync_point(char **buffer, struct eof_gpa_sync_point *ptr)
{
	unsigned long ctr, index;
	char buffer2[41], byte;
	double value;

	if(!buffer || !(*buffer) || !ptr)
		return 0;	//Invalid parameters

	//Examine the character at the specified position to see if it needs to be skipped
	byte = **buffer;
	if(byte == '#')
	{
		(*buffer)++;	//Skip the timestamp delimiter
		byte = **buffer;
	}
	if(!isdigit(byte) && (byte != '-'))
		return 0;	//Unexpected character was encountered

	//Read a set of 4 delimited numbers
	for(ctr = 0; ctr < 4; ctr++)
	{	//For each of the 4 expected numbers in the timestamp
		//Read the number and convert to floating point
		for(index = 0; index < 40; index++)
		{	//Read up to 40 characters for this number
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
			if(!isdigit(**buffer) && (**buffer != '.') && (byte != '-'))
			{	//If the next character isn't a number, a period or a hyphen
				return 0;	//Malformed timestamp
			}
			buffer2[index] = **buffer;	//Read the next character
			(*buffer)++;	//Increment to next character to read
		}
		if(index >= 40)
		{	//If the number field was too long to parse
			return 0;	//Malformed timestamp
		}
		buffer2[index] = '\0';	//Terminate the string
		value = atof(buffer2);	//Convert to floating point

		//Store the value into the appropriate structure element
		if(!ctr)
		{
			if(value < 0)
			{	//Sync points with negative timestamps will ultimately be ignored
				ptr->is_negative = 1;
				ptr->realtime_pos = 0;
			}
			else
			{
				ptr->is_negative = 0;
				ptr->realtime_pos = value + 0.5;	//Round up to nearest ms
			}
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
			ptr->qnote_length = value;
			ptr->real_qnote_length = value;
		}
	}//For each of the 4 expected numbers in the timestamp

	//Initialize some other member variables
	ptr->beat_length = 0.0;
	ptr->processed = 0;
	return 1;	//Return success
}

#endif
