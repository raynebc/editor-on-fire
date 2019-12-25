#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "Lyric_storage.h"
#include "Midi_parse.h"
#include "UStar_parse.h"

#ifdef S_SPLINT_S
//Ensure Splint checks the code for EOF's code base
#define EOF_BUILD
#endif

#ifdef USEMEMWATCH
#ifdef EOF_BUILD	//In the EOF code base, memwatch.h is at the root
#include "../memwatch.h"
#else
#include "memwatch.h"
#endif
#endif

double Weighted_Mean_Tempo(void)
{
	long double counter=0;			//The sum that will equal the weighted mean
	long double weightedvalue=0;	//The weighted value divided by the number of tempo changes for the current tempo
	struct Tempo_change *ptr=NULL;	//Conductor for the tempomap linked list
	double test=0.0;

//Count the number of tempo changes
	if(MIDIstruct.hchunk.tempomap == NULL)
		return 120.0;	//return default tempo

	if(MIDIstruct.hchunk.tempomap->next == NULL)
	{
		test=(MIDIstruct.hchunk.tempomap->BPM);	//return that tempo
		return test;
	}

	assert_wrapper(MIDIstruct.endtime != 0);	//The run time of the MIDI cannot be 0 ms

//For each tempo change, find the weighted value of each tempo, divide it by the number of tempo changes and add it to our sum
	for(ptr=MIDIstruct.hchunk.tempomap; ptr->next!=NULL; ptr=ptr->next)
	{
		weightedvalue=(long double)ptr->BPM * ((long double)ptr->next->realtime - (long double)ptr->realtime);	//Weighted value
		weightedvalue/=(long double)MIDIstruct.endtime;
		counter+=weightedvalue;		//Add to weighted mean
	}

//Process the last tempo change specially because it involves the timestamp of the end of the MIDI
	weightedvalue=(long double)ptr->BPM * ((long double)MIDIstruct.endtime - (long double)ptr->realtime);
	weightedvalue/=(long double)MIDIstruct.endtime;
	counter+=weightedvalue;	//Add to weighted mean.  Now the weighted mean has been calculated

	return (double)counter;
}

double Mean_Timediff_Tempo(void)
{
	long double counter=0;
	struct Lyric_Piece *current=NULL,*next=NULL;
	struct Lyric_Line *templine=NULL;

	if(Lyrics.piececount < 2)	//If there is only one lyric piece
		return 120.0;			//return the default tempo

	templine=Lyrics.lines;
	assert_wrapper(templine != NULL);	//Should not be possible if piececount > 0
	current=Lyrics.lines->pieces;		//Start with the first lyric
	while(current != NULL)
	{
	//Find next lyric, or break if there are none
		if(current->next != NULL)	//If there's another lyric on this line
			next=current->next;
		else
		{
			if(templine->next != NULL)	//If there's another line of lyrics
			{
				templine=templine->next;
				next=templine->pieces;
				if(next == NULL)
					break;		//Break from while loop
			}
			else
				break;			//Break from while loop
		}

		assert_wrapper(next != NULL);
		counter+=next->start - current->start;	//Add the difference between this timestamp and the next
		current=next;	//Focus on next lyric
	}

	counter/=Lyrics.piececount-1;	//Divide by the number of lyric transitions to calculate the mean
	return (double)counter;
}

void Export_UStar(FILE *outf)
{
	double tempo=0.0;	//Will hold a value based on the weighted mean of the loaded tempo if a MIDI was
						//loaded, otherwise will use an estimate based on the starting timestamps
	double stepping=0.0;	//The number of milliseconds in one quarter beat
	double x=0.0,y=0.0,z=0.0;
	long int start=0,dur=0;
	unsigned long int ctr=0,ctr2=0;
	struct Lyric_Piece *current=NULL,*next=NULL,*nextnext=NULL;
	char replace=0;		//boolean: Current lyric piece has + that needs to be replaced with ~
	char *temp=NULL;	//Used for string combination and tag output
	char unknownstr[]="Unknown";	//String written if a tag is empty
	char *tempostring=NULL;	//Used to store parsed tempo string (with a comma replacing the decimal point)
	unsigned long gap=0;//This is the #GAP tag in the UltraStar file, functionally the same as
						//the delay parameter in a FoF chart.  Since the UltraStar file will be
						//using this delay, references to lyric timestamps will be negatively
						//offset by this value.
//	char incomplete=0;	//Boolean:  Required UltraStar information needs to be entered into the
						//			exported file
	int errornumber=0;
	unsigned char pitch_char='F';	//By default, freestyle will be assumed.  If Lyrics.pitch_tracking is nonzero, then appropriate styles will be set
	char pitch_num=MINPITCH;		//By default, generic pitch value 36 (C1) is assumed
	char newline=1;		//Boolean:  If this is true, the prefixed space is skipped (for preventing a leading space
						//	for the first lyric in a new line of lyrics.  This should be set each time an end of
						//	line is written, as well as before writing the first lyric piece.  This is reset after
						//	the writing of each lyric entry.
	long int linetime=0;	//Converted unsigned long value of numerical string representing the timestamp of a line of lyric's first lyric (for relative timing)
	int rawpitch=0;			//Due to differences in UltraStar and MIDI pitch numbering, a number must be subtracted before export, which could make the pitch negative
	unsigned long length=0;


	assert_wrapper(outf != NULL);			//This must not be NULL
	assert_wrapper(Lyrics.piececount != 0);	//This function is not to be called with an empty Lyrics structure

	if(Lyrics.lines == NULL)
	{
			(void) puts("Error: Empty list of lyrics during export\nAborting");
			exit_wrapper(1);
	}

	if(Lyrics.verbose)	printf("\nExporting UltraStar lyrics to file \"%s\"\n\n",Lyrics.outfilename);

	if(Lyrics.pitch_tracking == 0)
		(void) puts("\a! NOTE: No pitch variation found in input lyrics.  Exporting as Freestyle");

	assert_wrapper((Lyrics.lines != NULL) && (Lyrics.lines->pieces != NULL));
	gap=Lyrics.lines->pieces->start;	//Load the #GAP value (timestamp of first lyric)

	if(Lyrics.verbose)	printf("GAP value of %lu detected\n",gap);

	if(Lyrics.explicittempo > 1.0)	//If user specified a tempo via command line
	{
		tempo=Lyrics.explicittempo;
		stepping=60000.0/(tempo*4);	//The stepping is the realtime length of one quarter beat
		printf("Tempo of %.2fBPM given, quarter beat precision is %fms.\nSkipping tempo estimation\n",tempo,stepping);
	}
	else
	{
	//Create an estimated mean tempo
		if(Lyrics.in_format==MIDI_FORMAT)
		{
			tempo=Weighted_Mean_Tempo();
			if(Lyrics.verbose)	printf("Weighted mean tempo is %fBPM\n",tempo);
		}
		else	//importing script or VL format
		{
			x=Mean_Timediff_Tempo();
			if(Lyrics.verbose>=2)	printf("Average time difference between starting timestamps is %fms\n",x);
			tempo=60000.0/x;	//Convert to tempo
			if(Lyrics.verbose>=2)	printf("Estimated tempo is %fBPM\n",tempo);
		}

	//Add tempo to itself until it would exceed 400BPM, to increase resolution
		for(x=tempo;tempo+x<400.0;tempo+=x);
		stepping=60000.0/(tempo*4);
		if(Lyrics.verbose>=2)	printf("Multiplied tempo is %fBPM, precision is %fms\n",tempo,stepping);

	//If brute forcing is enabled, find the most accurate tempo and compare with estimated tempo
		if(Lyrics.brute)
		{
			if(Lyrics.verbose>=2)	printf("\tEstimated tempo's mean time skew is %fms\n",CalculateTimeDiff(tempo));
			tempo=BruteForceTempo(200.0,400.0);
			stepping=60000.0/(tempo*4);	//Recalculate stepping, since the tempo has changed
			if(Lyrics.verbose>=2)
			{
				printf("Brute forced tempo= %fBPM, precision= %fms\n",tempo,stepping);
				printf("\tBrute forced mean time skew= %fms\n",CalculateTimeDiff(tempo));
			}
		}
	}

	if(Lyrics.verbose)	printf("Tempo of %.2fBPM selected\n\nWriting tags\n",tempo);

//Write tag information
	if(Lyrics.Title == NULL)
	{
		temp=unknownstr;
//		incomplete=1;
	}
	else
		temp=Lyrics.Title;

	if(fprintf(outf,"#TITLE:%s\n",temp) < 0)
		errornumber=errno;

	if(Lyrics.Artist == NULL)
	{
		temp=unknownstr;
//		incomplete=1;
	}
	else
		temp=Lyrics.Artist;

	if(fprintf(outf,"#ARTIST:%s\n",temp) < 0)
		errornumber=errno;

	fputs_err("#MP3:\n",outf);
//	incomplete=1;	//There is no checking for MP3 yet

	if(Lyrics.relative)						//If relative UltraStar export was specified
		fputs_err("#RELATIVE:yes\n",outf);	//Write the relative timing indicator

	tempostring=ConvertTempoToString(tempo);
	if(tempostring==NULL)
	{
		(void) puts("Error parsing tempo during UltraStar export\nAborting");
		exit_wrapper(2);
	}
	else
	{
		if(fprintf(outf,"#BPM:%s\n",tempostring) < 0)
			errornumber=errno;
		free(tempostring);	//We don't need this string anymore
	}

	if(fprintf(outf,"#GAP:%lu\n",gap) < 0)
		errornumber=errno;

	if(errornumber != 0)
	{
		printf("Error writing UltraStar tags to output file: %s\nAborting\n",strerror(errornumber));
		exit_wrapper(3);
	}

	if(Lyrics.verbose)	(void) puts("Writing Lyrics");

//Process lyric pieces
	for(ctr=1;ctr<=Lyrics.piececount;ctr++)	//For each lyric entry
	{
		current=FindLyricNumber(ctr);
		next=FindLyricNumber(ctr+1);

		if(current == NULL)
		{
			(void) puts("Error: Unexpected end of lyrics during export\nAborting.");
			exit_wrapper(4);
			return;	//Redundant return statement to satisfy cppcheck about checking current for NULL
		}

	//Set the appropriate pitch character
		if(Lyrics.pitch_tracking)
		{
			if(current->freestyle || (current->pitch == PITCHLESS))	//Export pitchless lyrics as freestyle
				pitch_char='F';		//This lyric piece had a # character
			else if(current->overdrive)
				pitch_char='*';		//This lyric was stored in the Lyrics structure denoted as Overdrive
			else
				pitch_char=':';		//If input lyrics have normal pitch, use this character for exported lyric lines
		}

	//If the current lyric piece is nothing but a + and whitespace, replace the + with ~
		assert_wrapper(current->lyric != NULL);
		length = (unsigned long)strlen(current->lyric);	//Save this value, which will be used several times
		for(ctr2=0,replace=1;ctr2<length;ctr2++)
			if((current->lyric[ctr2] != '+') && !isspace((unsigned char)current->lyric[ctr2]))
				replace=0;

	//If the current lyric piece is "+-", replace the + with ~
		if(!strcmp(current->lyric,"+-"))
			replace=1;

		if(replace)
		{
			for(ctr2=0;ctr2<length;ctr2++)
				if(current->lyric[ctr2] == '+')
					current->lyric[ctr2]='~';
				else
					printf("'%c' != '+'\n",current->lyric[ctr2]);
		}

		dur=0;	//reset to 0

		if(next == NULL)	//We're processing the last lyric piece
		{
			x=((double)current->start - gap) / stepping;
			start=(int)x;
			z=(double)current->duration / stepping;
			if(z-(int)z > 0.5)	//if this duration can round up
				dur=(int)z+1;	//round up
			else
				dur=(int)z;		//round down

			if(Lyrics.verbose)	printf("\"%s\"\tstart=%.2f\tdur=%.2f\n",current->lyric,x,z);
		}
		else
		{
			if(current->start > next->start)
			{
				printf("Error: Lyrics out of order\nAborting\n");
				exit_wrapper(5);
			}

		//find starting beat # and duration of current and next lyric piece
			x=((double)current->start - gap) / stepping;
			y=((double)next->start - gap) / stepping;

			if(Lyrics.verbose>=2)	printf("\"%s\"\tstart=%.2f\tnext start=%.2f\tdur=%.2f\n",current->lyric,x,y,(double)current->duration/stepping);

			if((int)(x + 0.5) == (int)(y +0.5))
			{		//this and next lyric start on same beat (considering rounding)
				if(y-(int)y > 0.5)	//if y would round up
				{
					nextnext=FindLyricNumber(ctr+2);	//get next lyric
					if((nextnext==NULL) || ((int)((nextnext->start - gap) / stepping) - (int)y > 1))
					{		//next lyric can round up without overlapping with the next lyric
						start=(int)x;	//round down for this lyric
						y=((int)y)+1.0;		//round next lyric up for remaining logic
						dur=1;	//we have determined that the next lyric starts on the next beat
					}
					else	//next lyric cannot round up, join current lyric with next lyric
					{
						temp=DuplicateString(current->lyric);	//Copy current string
						if((current->groupswithnext == 0) && (isspace((unsigned char)temp[strlen(temp)-1]) == 0))
						//If this piece does not group with the next and there's no space between the this lyric and the next
							temp=ResizedAppend(temp," ",1);	//Append a space after current lyric piece
						temp=ResizedAppend(temp,next->lyric,1);	//Reallocate temp to append next lyric piece
						free(next->lyric);	//Release string for next lyric
						next->lyric=temp;	//Replace with newly-created string

						next->start=current->start;	//Replace next lyric's timestamp with this one's

						next->duration+=current->duration;	//Add this lyric's duration to next's
						next->pitch=current->pitch;	//Replace next lyric's pitch with this one's
						continue;	//start at next iteration of loop
					}
				}//end if(y-(int)y > 0.5)
			}else
			{
				start=(int)(x+0.5);	//round up
				y=(double)((int)(y+0.5));	//round y up as well
			}

		//find duration in beats
			if(dur == 0)	//if we didn't determine a duration value yet because there was no special case
			{
				z=(double)current->duration / stepping;

				if(start + (int)z > (int)y)	//remember to round y up for comparison
				{	//This condition should have been caught with the syllable combining logic earlier
					(void) puts("Error: Lyrics out of order\nAborting");
					exit_wrapper(6);
				}

				if((z-(int)z > 0.5) && (start + (int)z + 1 <= (int)y))	//can round up without ovarlapping with next lyric
					dur=(int)z+1;	//round up
				else
					dur=(int)z;	//round down

				if(dur==0)	//1 is the shortest duration allowed.  Checking performed earlier guarantees this will
					dur=1;	//not cause an overlap with the next lyric
			}
		}//end if(next == NULL) else ...

		if(Lyrics.verbose>=2)	printf("\trounded start=%ld\trounded duration=%ld\n",start,dur);

		//write lyric timing and pitch to file, prefixing with : or F as appropriate based on pitch presence
		if(Lyrics.pitch_tracking)	//Write default pitch MINPITCH unless Lyrics.pitch_tracking is enabled
			pitch_num=current->pitch;

		rawpitch=pitch_num;	//Store the pitch into a signed int variable
		if((unsigned char)rawpitch != PITCHLESS)	//If this lyric has a pitch
			rawpitch-=24;			//Remap it to UltraStar numbering
		else
			rawpitch=PITCHLESS;	//Store it as the positive value representing a pitchless lyric

		if(fprintf(outf,"%c %ld %ld %d ",pitch_char,start-linetime,dur,rawpitch) < 0)
			errornumber=errno;
		if(!newline && !current->prev->groupswithnext)	//If this piece doesn't group with previous piece and isn't the first lyric in this line
		{
			if(Lyrics.verbose>=2)	printf("\tNo grouping between \"%s\" and \"%s\"\n",(current->prev)->lyric,current->lyric);
			fputc_err(' ',outf);	//An extra space prefix is necessary for non-grouped pieces
		}

		newline=0;	//Reset this condition

		if(fprintf(outf,"%s\n",current->lyric) < 0)
			errornumber=errno;

		if(errornumber != 0)
		{
			printf("Error writing UltraStar lyrics to output file: %s\nAborting\n",strerror(errornumber));
			exit_wrapper(7);
		}

		if((current->next == NULL) && (next != NULL))
		{	//If this is the last lyric piece in this line, and there's another line
			if(fprintf(outf,"- %ld\n",start+dur-linetime) < 0)	//write end of line
			{
				printf("Error writing end of lyric line: %s\nAborting\n",strerror(errno));
				exit_wrapper(8);
			}
			if(Lyrics.relative)		//Only if relative UltraStar export was specified,
				linetime=start+dur;	//Store absolute timestamp of the line break, which will cause the timestamps to be written as relative
			newline=1;	//This will prevent the first lyric piece in the next line from having a leading space
		}
	}//end for(ctr=1;ctr<Lyrics.piececount;ctr++)

	fputs_err("E\n",outf);

//	if(incomplete)	//For now this condition is always true, so comment this condition out to satisfy cppcheck
		(void) puts("\a! Needed information needs to be manually entered into the exported file");

	if(Lyrics.verbose)	printf("\nUltraStar export complete.  %lu lyrics written",Lyrics.piececount);
}

char *ConvertTempoToString(double tempo)
{	//Takes an input double, parses it and returns a string representation in %.2f representation, with the
	//decimal replaced by a comma.  The value is truncated, the thousanths value is not rounded up.
	char string[15]={0};	//Max precision will be 3 digits integer part, a decimal, and 6 digits decimal part
	char *temp=NULL;		//The final string will be duplicated into an allocated array which will be returned
	unsigned ctr=0;

	if(!(tempo > 0.0) || !(tempo < 1000.0))
		return NULL;	//return error

	if(snprintf(string, sizeof(string) - 1,"%.6f",tempo) < 0)	//Use snprintf so that rounding errors don't occur, and to prevent a buffer overflow
		return NULL;	//return error

	for(ctr=0;string[ctr]!='\0';ctr++)	//parse string to convert decimal character to comma
		if(string[ctr] == '.')
			string[ctr] = ',';

	for(ctr=0;string[ctr]!='\0';ctr++)	//parse string to truncate to hundredths place
		if((string[ctr] == ',') && (string[ctr+1] != '\0') && (string[ctr+2] != '\0'))	//If there are two digits following the comma
			string[ctr+3]='\0';	//truncate the string after those two digits

	temp=DuplicateString(string);

	return temp;
}

double ConvertStringToTempo(char *tempo)
{
	unsigned long ctr=0;
	double x=0.0;

	assert_wrapper(tempo != NULL);

//Parse the string to convert comma to decimal point
	for(ctr=(unsigned long)strlen(tempo);ctr>0;ctr--)
	{
		if(tempo[ctr-1] == ',')
		{
			tempo[ctr-1] = '.';
			break;
		}
	}

	x=atof(tempo);

	if(x < 1.0)
	{
		(void) puts("Error processing source UltraStar file's tempo\nAborting");
		exit_wrapper(1);
	}

	return x;
}

double BruteForceTempo(double start,double end)
{
	double ctr=0.0,temp=0.0;
	double candidate=0.0;			//The current best tempo
	double candidate_accuracy=0.0;	//The mean time difference for lyric timestamps in ms

	assert_wrapper((start > 0) && (end > 0));	//Tempos that are 0 or negative are not valid
	assert_wrapper(start<=end);

	for(ctr=start;ctr<=end;ctr+=0.01)	//Check all increments from start to end
	{
		temp=CalculateTimeDiff(ctr);	//Find the mean time difference using this tempo

		if((candidate < 1.0) || (temp < candidate_accuracy))
		{	//This is the first tempo we've checked or it beats the current candidate
			candidate=ctr;
			candidate_accuracy=temp;
		}
	}

	return candidate;
}

double CalculateTimeDiff(double tempo)
{
	struct Lyric_Piece *lyrptr;		//Conductor for lyric piece list
	struct Lyric_Line *lineptr;		//Conductor for lyric line list
	unsigned long ctr=0,num=0;
	long double sum=0;				//Stores the sum of the time differences
	double stepping;				//The precision of the tempo

//error checking: Linked lists must be populated
	assert_wrapper(Lyrics.lines != NULL);
	assert_wrapper(Lyrics.lines->pieces != NULL);
	assert_wrapper(tempo > 0);	//Tempos that are 0 or negative are not valid

//init conductors
	lineptr=Lyrics.lines;	//point to the first line of lyrics
	lyrptr=lineptr->pieces;	//point to first lyric piece in the line

	stepping=60000.0/(tempo*4);	//The stepping is in quarter beats

//process each lyric piece
	while(lyrptr != NULL)
	{
		num=(unsigned long)((double)lyrptr->start / stepping);	//Convert to rounded down starting quarterbeat
		sum+=lyrptr->start - (num * stepping);		//Find the time difference from realtime and add it to sum

		ctr++;	//processed another lyric piece

		if(lyrptr->next != NULL)	//point to next lyric piece
			lyrptr=lyrptr->next;
		else				//end of line of lyrics reached
			if(lineptr->next != NULL)
			{
				lineptr=lineptr->next;	//point to next line of lyrics
				lyrptr=lineptr->pieces;	//point to first lyric piece in this line
			}
			else			//end of lyrics reached
				break;
	}

	return (double)(sum / (double)ctr);	//return the mean
}

void UStar_Load(FILE *inf)
{
	unsigned long ctr=0;	//Equal to length of string (+1 for null terminator)
	size_t maxlinelength;	//I will count the length of the longest line (including NULL char/newline) in the
							//input file so I can create a buffer large enough to read any line into
	char *buffer;			//Will be an array large enough to hold the largest line of text from input file
	char *substring=NULL;	//Used with strstr() to find tag strings in the input file
	unsigned long index=0;	//Used to index within a line of text
	long int starttime=0;		//Converted unsigned long value of numerical string representing a lyric's timestamp in quarterbeats
	long int duration=0;		//Converted unsigned long value of numerical string representing a lyric's duration
	long int linetime=0;	//Converted unsigned long value of numerical string representing the timestamp of a line of lyric's first lyric (for relative timing)
	int rawpitch=0;			//The (potentially) negative pitch is read into this variable, so it can be properly bounds checked
	unsigned char pitch=0;	//Converted unsigned char value of numerical string representing a lyric's pitch
	unsigned long processedctr=0;	//The current line number being processed in the text file
	char tagsprocessed=0;		//If nonzero, a line not beginning in '#' was read, at which point all requisite
								//tags were expected to have been read.  If > 1, tags have been processed.
	double BPM=0.0;			//The tempo defined in the file
	char *tempstr=NULL;		//Used for temporary strings
	double stepping=0.0;	//The number of milliseconds in one quarter beat
	char replace=0;			//Used for ~ to + character conversion
	char relative_timing=0;	//Boolean: If nonzero, there was a #RELATIVE tag in the input file specifying to use relative
							//timing instead of absolute timing.  Relative timing causes the timestamp to reset to 0 at
							//the beginning of each line of lyrics in the UltraStar file (reset at each linebreak)
	unsigned long length=0;
	char BOM[]={0xEF,0xBB,0xBF};
	int readerrordetected = 0;

	assert_wrapper(inf != NULL);	//This must not be NULL

//Find the length of the longest line
	maxlinelength=FindLongestLineLength(inf,1);

//Allocate memory buffer large enough to hold any line in this file
	buffer=(char *)malloc_err(maxlinelength);

//Load each line and parse it
	(void) fgets_err(buffer, (int)maxlinelength,inf);	//Read first line of text, capping it to prevent buffer overflow

//Skip Byte Order Mark if one is found on the first line
	if((buffer[0] == BOM[0]) && (buffer[1] == BOM[1]) && (buffer[2] == BOM[2]))
	{
		if(Lyrics.verbose)	(void) puts("Skipping byte order mark on line 1");
		buffer[0] = buffer[1] = buffer[2] = ' ';	//Overwrite the BOM with spaces
	}

	if(Lyrics.verbose)	printf("\nImporting UltraStar lyrics from file \"%s\"\n\n",Lyrics.infilename);

//Look for required tags
	Lyrics.TitleStringID=DuplicateString("#TITLE");
	Lyrics.ArtistStringID=DuplicateString("#ARTIST");
	Lyrics.OffsetStringID=DuplicateString("#GAP");

	processedctr=0;			//This will be set to 1 at the beginning of the main while loop
	while(!feof(inf) && !readerrordetected)		//Until end of file is reached or fgets() returns an I/O error
	{
		processedctr++;

//Skip leading whitespace
		index=0;	//Reset index counter to beginning
		while(1)
			if((buffer[index] != '\n') && (isspace((unsigned char)buffer[index])))
				index++;	//If this character is whitespace, skip to next character
			else
				break;		//Only break out of this loop if end of line or non whitespace is reached

		if((buffer[index] == '\r') || (buffer[index] == '\n'))	//If it was a blank line
		{
			if(Lyrics.verbose)	printf("Ignoring blank line (line #%lu) in input file\n",processedctr);
			if(fgets(buffer, (int)maxlinelength,inf) == NULL)	//Read next line of text, so the EOF condition can be checked, don't exit on EOF
				readerrordetected = 1;
			continue;	//Skip processing for a blank line
		}

//Look for end of UltraStar file
		if(buffer[index]=='E')	//If 'E' is encountered at the start of any line
			break;		//Stop processing lines

// **Add checking to require all tags to be placed before the lyric timings, this is consistent with UltraStar documentation
		if(buffer[index]=='#')	//If the line begins with a pound symbol
		{
			if(ParseTag(':','\0',buffer,1) <= 0)	//Look for tags, content starts after ':' char and extends to end of line.  Negatize the offset
			{	//If the title, artist and offset tags are not found in the line
	//Look for BPM tag	(#BPM:)
				substring=strcasestr_spec(buffer,"#BPM:");
				if(substring != NULL)
				{
					if(BPM > 1.0)
						printf("Warning in line #%lu: Extra BPM tag in Ultrastar file.  Ignoring\n",processedctr);
					else
					{
						tempstr=ReadUStarTag(substring);
						BPM=ConvertStringToTempo(tempstr);
						if(!(BPM > 1.0))	//Write condition to avoid testing for equality, which GCC will warn about for floating point comparisons
						{
							printf("Error in line #%lu: Tempo must be greater than 1BPM\nAborting\n",processedctr);
							exit_wrapper(1);
						}

						if(Lyrics.explicittempo < 1.0)	//If user did not specify an export tempo via command line
							Lyrics.explicittempo=BPM;	//Save this tempo in case export format uses a BPM value

						if(Lyrics.verbose)	printf("Loaded Ultrastar tag: \"BPM= %f\"\n",BPM);
						free(tempstr);	//This string is no longer needed
					}
				}
				else
					if(strcasestr_spec(buffer,"#RELATIVE"))	//If there is a case-insensitive match for the relative timing string
						if(strcasestr_spec(buffer,"yes"))	//And the string "yes" occurs in the same line
							relative_timing=1;				//Activate relative timing
			}
			if(fgets(buffer, (int)maxlinelength,inf) == NULL)	//Read next line of text, so the EOF condition can be checked, don't exit on EOF
				readerrordetected = 1;
			continue;	//Restart at next iteration of loop
		}//end if(buffer[index]=='#')
		else	//This line does not contain a tag
			if(tagsprocessed == 0)	//Time to validate that all tags have been read
			{
				if((Lyrics.Title == NULL) || (Lyrics.Artist == NULL) || (BPM < 1.0) || (!Lyrics.offsetoverride && (Lyrics.Offset == NULL)))	//Ignore the unprocessed offset tag if it is being overridden via command line
					(void) puts("\aWarning: One of the required tags (Title, Artist, BPM, GAP) is missing");

				stepping=60000.0/(BPM*4);	//The stepping is the realtime length of one quarter beat
				tagsprocessed=1;	//Tags were verified
			}

//Identify pitch style or end of line
		Lyrics.overdrive_on=0;	//Reset overdrive to off until a golden note lyric is read
		Lyrics.freestyle_on=0;	//Reset freestyle to off until a freestyle lyric is read

		switch(buffer[index])
		{
			case '*':	//Golden note
				Lyrics.overdrive_on=1;	//Cause AddLyricPiece() to mark the following note as a golden note
			break;

			case ':':	//Regular vocal note (do nothing, but prevent the default case from being reached)
			break;

			case 'F':	//Freestyle note
			case 'f':
				Lyrics.freestyle_on=1;	//Cause AddLyricPiece() to mark the following note as freestyle
			break;

			case '-':	//End of line
				EndLyricLine();
				if(relative_timing)
				{	//If relative timing is in effect, validate that the line break has one or two timestamps (REQUIRED)
					if(FindNextNumber(buffer,&index) == 0)	//If there was not at least one number
					{
						printf("Error: UltraStar violation: Line break has no defined timestamp in a Relative timed file (line #%lu)\nAborting\n",processedctr);
						exit_wrapper(2);
					}
					starttime=ParseLongInt(buffer,&index,processedctr,NULL);	//Parse the line break's relative timestamp
					if(FindNextNumber(buffer,&index))	//If there was a second number, it is the timestamp to use, so store its index
						linetime+=ParseLongInt(buffer,&index,processedctr,NULL);	//Parse the line break's relative timestamp and add it to find the line break's absolute timestamp
					else
						linetime+=starttime;	//Otherwise add the number that was already read to find the line break's absolute timestamp
				}

				if(fgets(buffer, (int)maxlinelength,inf) == NULL)	//Read next line of text, so the EOF condition can be checked, don't exit on EOF
					readerrordetected = 1;
			continue;

			default:
				printf("Error: Invalid line type '%c' in line #%lu\nAborting\n",buffer[index],processedctr);
				exit_wrapper(3);
			break;
		}

		index++;	//Seek past the pitch style/endline character

		if(Lyrics.line_on != 1)	//If we're at this point, there should be a line of lyrics in progress
			CreateLyricLine();

//Parse starting offset (which is in quarter beats)
		starttime=ParseLongInt(buffer,&index,processedctr,NULL);

//Perform relative timing conversion if applicable
		if(relative_timing)
			starttime+=linetime;	//Adjust the timing accordingly (timestamp absolute time + lyric relative time = lyric absolute time)

		starttime=(long int)(((double)starttime * stepping)+0.5);	//Convert from quarterbeats to milliseconds (round up)

//Ensure that the lyric timestamp is not negative once the offset is taken into account
		if(starttime-Lyrics.realoffset < 0)
		{
			printf("Error: Offsetted lyric has negative timestamp\nAborting\n");
			exit_wrapper(4);
		}

//Parse duration
		duration=ParseLongInt(buffer,&index,processedctr,NULL);
		duration=(long int)(((double)duration * stepping)+0.5);		//Convert from quarterbeats to milliseconds (round up)
		if(!duration)
		{	//If the duration had the invalid value of 0, set it to the minimum length of 1
			printf("Warning:  Invalid lyric duration on line #%lu\nCorrecting\n",processedctr);
			duration = 1;
		}

//Parse pitch
		rawpitch=ParseLongInt(buffer,&index,processedctr,NULL);
		if((unsigned char)rawpitch != PITCHLESS)
		{	//If this lyric has a pitch
			rawpitch+=24;	//UltraStar pitches map note C1 as 0 instead of 24, remap to match the MIDI note standard
			if((rawpitch<0) || (rawpitch>127))
			{
				printf("Error: Pitch %d in the UltraStar file cannot be converted to another pitch numbering system\nAborting\n",rawpitch-24);
				exit_wrapper(5);
			}
			pitch=(unsigned char)rawpitch & 0xFF;	//Store the remapped pitch
		}
		else
		{
			pitch=rawpitch;	//Store the lyric as pitchless
		}

//Track for pitch changes, enabling Lyrics.pitch_tracking if applicable
		if((Lyrics.last_pitch != 0) && (Lyrics.last_pitch != pitch))	//There's a pitch change
			Lyrics.pitch_tracking=1;
		Lyrics.last_pitch=pitch;	//Consider this the last defined pitch

//At least one whitespace character is expected between pitch and lyric
		if(!isspace((unsigned char)buffer[index]))
		{
			printf("Error: Whitespace expected after pitch in line #%lu\nAborting\n",processedctr);
			exit_wrapper(6);
		}
		index++;	//Advance to next character

	//Truncate newline and carriage return characters from the end of the string
		while(1)
		{
			if((buffer[strlen(buffer)-1]=='\n') || (buffer[strlen(buffer)-1]=='\r'))
				buffer[strlen(buffer)-1]='\0';
			else
				break;
		}

		if(!isspace((unsigned char)buffer[index]))
		{	//If there is NOT a second whitespace character between pitch and lyric, this piece groups with previous lyric
			if(Lyrics.curline->piececount > 0)
			{	//And only if there was a previous lyric in this line
				if(Lyrics.verbose>=2)	printf("\t\tLyric piece \"%s\" groups with previous lyric \"%s\"\n",&(buffer[index]),Lyrics.lastpiece->lyric);

				assert_wrapper(Lyrics.lastpiece != NULL);
				if(Lyrics.lastpiece->trailspace == 0)	//And only if there was not trailing whitespace on the previous lyric
					Lyrics.lastpiece->groupswithnext=1;	//The previous lyric piece will group with this one
			}
		}

//Find the next non whitespace character
		while(buffer[index] != '\0')
		{
			if(isspace((unsigned char)buffer[index]) == 0)
				break;

			index++;
		}

//Replace UltraStar's pitch shift indicator (~) with that of Rock Band (+)
	//If the current lyric piece is nothing but a ~ and whitespace, replace the ~ with +
		length = (unsigned long)strlen(&buffer[index]);
		for(ctr=0,replace=1;ctr<length;ctr++)
			if((buffer[index+ctr] != '~') && !isspace((unsigned char)buffer[index+ctr]))
				replace=0;

		if(replace)
		{
			for(ctr=0;ctr<length;ctr++)
				if(buffer[index+ctr] == '~')
					buffer[index+ctr]='+';
		}

//Add lyric.  At this point, buffer[index] points to text string to add.  If there was nothing but whitespace after
//the pitch, AddLyricPiece() will receive a string of 0 characters, and it will ignore it
		AddLyricPiece(&(buffer[index]),starttime,starttime+duration,pitch,0);	//Add lyric (remapped to Rock Band's pitch system)

		if(fgets(buffer, (int)maxlinelength,inf) == NULL)	//Read next line of text, so the EOF condition can be checked, don't exit on EOF
			readerrordetected = 1;
	}//while(!feof(inf) && !readerrordetected)

	if(feof(inf))	//If end of file was reached before a line beginning with E
		(void) puts("Warning: UltraStar file did not properly end with a line beginning with 'E'");

	ForceEndLyricLine();

	free(buffer);	//No longer needed, release the memory before exiting function

	if(Lyrics.verbose)	printf("\nUltraStar import complete.  %lu lyrics loaded\n\n",Lyrics.piececount);
}

char *ReadUStarTag(char *str)
{
	char *temp;
	unsigned long length=0;	//strlen(temp) is used several times

	assert_wrapper(str != NULL);	//This must not be NULL

	temp=DuplicateString(str);	//Duplicate string into temp

	while(1)
	{
		length = (unsigned long)strlen(temp);

		if(length == 0)	//string is empty
		{
			(void) puts("Error processing UltraStar tag\nAborting");
			exit_wrapper(1);
		}

		if((temp[length-1] == '\r') || (temp[length-1] == '\n'))	//If last character is carriage return or new line
			temp[length-1]='\0';					//truncate it
		else
			return temp;		//Otherwise return the string
	}
}

void RemapPitches(void)
{
	unsigned char pitchmin=0,pitchmax=0;
	int diff=0;
	struct Lyric_Piece *temp=NULL;
	struct Lyric_Line *templine=NULL;
	unsigned int minoctave=0,maxoctave=0;

	if(!CheckPitches(&pitchmin,&pitchmax))
		return;	//Return without performing remapping if all pitches are in the correct range

//Determine the pitch that begins on the lowest octave of 12 usable notes
	minoctave=(MINPITCH/12)*12;
#if (MINPITCH % 12 != 0)
	minoctave+=12;		//The first usable full octave is the first octave above MINPITCH
#endif

//Determine the pitch that begins AFTER the highest octave of 12 usable notes
	maxoctave=(MAXPITCH/12)*12-12;	//The pitch of C in the octave below the maximum pitch
	// cppcheck-suppress constArgument
	assert_wrapper(maxoctave>minoctave);	//This better be the case or it's an invalid MINPITCH and MAXPITCH definition (MAXPITCH must be at least 12 higher than MINPITCH)

	diff=minoctave - ((pitchmin/12)*12);	//Find the difference between the lowest used octave and the lowest usable octave

	if((diff <= -12) || (diff >= 12))	//Only if the minimum pitch is at least one octave higher or lower than the lowest usable octave can all pitches transpose down or up by one or more octaves
	{
		if(Lyrics.verbose)	printf("Transposing imported pitches by %d octaves to be no less than the minimum pitch of %d\n",diff/12,MINPITCH);

		for(templine=Lyrics.lines;templine!=NULL;templine=templine->next)	//For each line of lyrics
		{
			for(temp=templine->pieces;temp!=NULL;temp=temp->next)	//For each lyric in the line
			{
				if(temp->pitch == PITCHLESS)	//If this lyric piece has no defined pitch
					continue;					//Disregard it

				if(Lyrics.verbose>=2)	printf("\tRemapping lyric \"%s\"'s pitch of %u to ",temp->lyric,temp->pitch);
				temp->pitch=temp->pitch + diff;	//Transpose the note by the appropriate number of octaves
				if(Lyrics.verbose>=2)	printf("%u\n",temp->pitch);
			}
		}

		if(!CheckPitches(&pitchmin,&pitchmax))	//If the pitches now fall into the range of [MINPITCH,MAXPITCH]
			return;
	}

	assert_wrapper(pitchmin >= MINPITCH);	//The transposing above (if applied) was expected to leave this condition true
	if(Lyrics.verbose)	printf("Constricting pitch octaves to leave them no greater than %d\n",MAXPITCH);

//If the pitches still exceed the defined maximum (MAXPITCH)
	for(templine=Lyrics.lines;templine!=NULL;templine=templine->next)	//For each line of lyrics
	{
		for(temp=templine->pieces;temp!=NULL;temp=temp->next)	//For each lyric in the line
		{
			if(temp->pitch == PITCHLESS)	//If this lyric piece has no defined pitch
				continue;					//Disregard it

			if(temp->pitch > MAXPITCH)	//If the note is above accepted pitch range
			{				//Transpose the note to the highest exportable octave
				if(Lyrics.verbose)	printf("\tRemapping lyric \"%s\"'s pitch of %u to ",temp->lyric,temp->pitch);
				temp->pitch=(temp->pitch % 12)+maxoctave;	//Lower the octave of the note
				if(Lyrics.verbose)	printf("%u\n",temp->pitch);
			}
		}
	}

	if(CheckPitches(&pitchmin,&pitchmax) != 0)	//If the pitches still don't fall into the appropriate range
	{
		(void) puts("Error: Failed to remap pitches\nAborting");
		exit_wrapper(1);
	}
}

int CheckPitches(unsigned char *pitchmin,unsigned char *pitchmax)
{
	unsigned char pitchmin_local=0,pitchmax_local=0;
	char firstpitchfound = 0;	//This will be set to 1 when the first pitched lyric is found
	struct Lyric_Piece *temp=NULL;
	struct Lyric_Line *templine=NULL;

//Find the minimum and maximum pitch in the input lyrics
	assert_wrapper((Lyrics.lines != NULL) && (Lyrics.lines->pieces != NULL));

	for(templine=Lyrics.lines;templine!=NULL;templine=templine->next)	//For each line of lyrics
	{
		for(temp=templine->pieces;temp!=NULL;temp=temp->next)			//For each lyric in the line
		{
			if(temp->pitch == PITCHLESS)	//If this lyric piece has no defined pitch
				continue;					//Disregard it

			#ifdef EOF_BUILD
			if(temp->pitch == VOCALPERCUSSION)	//If this lyric piece is vocal percussion
				continue;						//Disregard it
			#endif

			if(!firstpitchfound)
			{	//If this is the first pitched lyric
				pitchmin_local=pitchmax_local=temp->pitch;	//Initialize the min and max pitch variables
				firstpitchfound = 1;
			}
			else
			{
				if(temp->pitch < pitchmin_local)	//Track the lowest pitch
					pitchmin_local=temp->pitch;

				if(temp->pitch > pitchmax_local)	//Track the highest pitch
					pitchmax_local=temp->pitch;
			}
		}
	}

	if(Lyrics.verbose)	printf("Input lyric pitch range is [%u,%u]\n",pitchmin_local,pitchmax_local);

//If non-NULL pointers were passed, save the minimum and/or maximum pitch values found
	if(pitchmin != NULL)
		*pitchmin=pitchmin_local;
	if(pitchmax != NULL)
		*pitchmax=pitchmax_local;

//If the pitches are within the target range,
	if((pitchmin_local >= MINPITCH) && (pitchmax_local <= MAXPITCH))
		return 0;	//return success

//Special case:  All lyrics were pitchless (ie. VL/Script/LRC import)
	if((pitchmin_local == PITCHLESS) && (pitchmax_local == PITCHLESS))
		return 0;	//return success

	return 1;		//otherwise return error
}
