#ifdef EOF_BUILD
#include <allegro.h>
#include "main.h"
#include "song.h"
#include "tuning.h"
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

EOF_SONG *eof_import_gp(const char * fn)
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
				puts("Error allocating memory");
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
		puts("Error allocating memory");
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
		bytemask = pack_getc(inf);	//Read the measure bitmask
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
							eof_gp_debug_log(inf, "\tDownstroke speed:  ");
							byte = pack_getc(inf);
							printf("%u\n", (byte & 0xFF));
							eof_gp_debug_log(inf, "\tUpstroke speed:  ");
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
	return (EOF_SONG *)1;	///This is just a placeholder value until this logic is fully implemented
}

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
int main(int argc, char *argv[])
{
	EOF_SONG *ptr = NULL;

	if(argc < 2)
	{
		puts("Must pass the GP file as a parameter");
		return 1;
	}

	ptr = eof_import_gp(argv[1]);
	if(ptr == NULL)
	{	//Failed to reach end of parsing
		return 2;
	}

	return 0;	//Return success
}
#endif
