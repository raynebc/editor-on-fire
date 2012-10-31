#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "XML_parse.h"
#include "Lyric_storage.h"

#ifdef USEMEMWATCH
#ifdef EOF_BUILD	//In the EOF code base, memwatch.h is at the root
#include "../memwatch.h"
#else
#include "memwatch.h"
#endif
#endif

void Export_RS(FILE *outf)
{
	struct Lyric_Line *curline=NULL;	//Conductor of the lyric line linked list
	struct Lyric_Piece *temp=NULL;		//A conductor for the lyric pieces list

	assert_wrapper(outf != NULL);	//This must not be NULL
	assert_wrapper(Lyrics.piececount != 0);	//This function is not to be called with an empty Lyrics structure

	if(Lyrics.verbose)	printf("\nExporting Rocksmith XML lyrics to file \"%s\"\n",Lyrics.outfilename);

//Write the beginning lines of the XML file
	fputs_err("<?xml version='1.0' encoding='UTF-8'?>\n",outf);
	if(fprintf(outf,"<vocals count=\"%lu\">\n", Lyrics.piececount) < 0)
	{
		printf("Error writing to XML file: %s\nAborting\n",strerror(errno));
		exit_wrapper(2);
	}

//Export the lyric pieces
	if(Lyrics.verbose)	puts("Writing lyrics");
	curline=Lyrics.lines;	//Point lyric line conductor to first line of lyrics
	while(curline != NULL)	//For each line of lyrics
	{
		temp=curline->pieces;	//Starting with the first piece of lyric in this line

		if(Lyrics.verbose)	printf("\tLyric line: ");

		while(temp != NULL)				//For each piece of lyric in this line
		{
			if(fprintf(outf,"  <vocal time=\"%.3f\" note=\"%u\" length=\"%.3f\" lyric=\"%s\"/>\n", (double)temp->start / 1000.0, temp->pitch, (double)temp->duration / 1000.0, temp->lyric) < 0)
			{
				printf("Error exporting lyric %lu\t%lu\ttext\t%s: %s\nAborting\n",temp->start,temp->duration,temp->lyric,strerror(errno));
				exit_wrapper(2);
			}

			if(Lyrics.verbose)	printf("'%s'",temp->lyric);

			temp=temp->next;	//Advance to next lyric piece as normal
		}//end while(temp != NULL)

		curline=curline->next;	//Advance to next line of lyrics
		if(Lyrics.verbose)	putchar('\n');
	}

//Close the <vocals> XML tag
	fputs_err("</vocals>",outf);

	if(Lyrics.verbose)	printf("\nRocksmith XML export complete.  %lu lyrics written",Lyrics.piececount);
}
