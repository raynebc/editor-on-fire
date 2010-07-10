#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <ctype.h>
#include "Lyric_storage.h"
#include "Midi_parse.h"

#ifdef USEMEMWATCH
#include <memwatch.h>
#endif

//
//GLOBAL VARIABLE DEFINITIONS
//
struct _MIDISTRUCT_ MIDIstruct;
struct _MIDI_LYRIC_STRUCT_ MIDI_Lyrics;

struct Tempo_change DEFAULTTEMPO={0,0,120.0,NULL};
	//Until a Set Tempo event is encountered in a track, 120BPM will be assumed


void InitMIDI(void)
{
	if(Lyrics.verbose)	puts("Initializing MIDI variables");

//Initialize global variables
	MIDIstruct.BPM=(double)120.0;								//Default BPM is assumed until MPQN is defined
	MIDIstruct.MPQN_defined=0;									//Default MPQN is assumed until one is defined
	MIDIstruct.hchunk.tempomap=MIDIstruct.hchunk.curtempo=NULL;	//Empty linked list of tempo information
	MIDI_Lyrics.head=MIDI_Lyrics.tail=NULL;						//Empty MIDI Lyric linked list
	MIDIstruct.trackswritten=0;
	MIDIstruct.diff_lo=MIDIstruct.diff_hi=0;
	MIDIstruct.mixedtrack=0;
	MIDIstruct.miditype=0;
	Lyrics.last_pitch=0;
}

void ReadMIDIHeader(FILE *inf,char suppress_errors)
{
	unsigned short ctr=0;
	unsigned char failure=0;
	char header[5]={0};	//Used to read header


	assert_wrapper(inf != NULL);	//This must not be NULL

	fread_err(header,4,1,inf);
	header[4]='\0';				//Terminate buffer to create a proper string, which is now expected to be "MThd"

	if(strcmp(header,"MThd") != 0)
	{
		if(strcmp(header,"RIFF") == 0)	//If this is a RIFF-MIDI (RMIDI) header
		{
			if(Lyrics.verbose)	puts("Parsing RIFF-MIDI header");
			MIDIstruct.miditype=1;		//Track that this MIDI file is within a RIFF header
			fseek_err(inf,20,SEEK_SET);	//Seek to byte 20, which is the beginning of the MIDI header in an RMIDI file
			fread_err(header,4,1,inf);	//Read 4 bytes, which are expected to be the MIDI header
			if(strcmp(header,"MThd") != 0)
				failure=1;
		}
		else if(strcmp(header,"RBSF") == 0)	//If this is a Rock Band Audition (RBA) header
		{
			if(Lyrics.verbose)	puts("Parsing RBA header");
			if(SearchPhrase(inf,0,NULL,"MThd",4,1) == 1)	//Search for and seek to MIDI header
			{
				MIDIstruct.miditype=2;	//Track that this MIDI is within a RBA file
				fread_err(header,4,1,inf);	//Read 4 bytes, which are already known to be the MIDI header
			}
			else
				failure=1;	//RBA file with no MIDI
		}
		else
			failure=1;
	}

	if(failure)
	{
		if(!suppress_errors)
			puts("Error: Incorrect or missing MIDI header\nAborting");
		exit_wrapper(1);
	}

	ReadDWORDBE(inf,&MIDIstruct.hchunk.chunksize);
	if((unsigned long)MIDIstruct.hchunk.chunksize != (unsigned long)6)
	{
		if(!suppress_errors)
			puts("Error: Incorrect chunk size in MIDI header\nAborting");
		exit_wrapper(2);
	}
	ReadWORDBE(inf,&MIDIstruct.hchunk.formattype);
	if(MIDIstruct.hchunk.formattype > 2)
	{
		if(!suppress_errors)
			puts("Error: Incorrect MIDI type in MIDI header\nAborting");
		exit_wrapper(3);
	}
	ReadWORDBE(inf,&MIDIstruct.hchunk.numtracks);
	ReadWORDBE(inf,&MIDIstruct.hchunk.division);
	if((MIDIstruct.hchunk.division & 0x8000) != 0)
	{
		if(!suppress_errors)
			puts("Error: frames per second time division is not supported\nAborting.");
		exit_wrapper(4);
	}
	MIDIstruct.hchunk.tracks=(struct Track_chunk *)calloc_err(1,sizeof(struct Track_chunk)*MIDIstruct.hchunk.numtracks);	//Allocate an array of track headers and init to 0

	for(ctr=0;ctr<MIDIstruct.hchunk.numtracks;ctr++)	//Initialize tracks array
		(MIDIstruct.hchunk.tracks[ctr]).tracknum=ctr;

	if(Lyrics.verbose>=2)	printf("Start of MIDI\nMIDI format=%u\tNumber of tracks=%u\nTime division=%u\n\n",MIDIstruct.hchunk.formattype,MIDIstruct.hchunk.numtracks,MIDIstruct.hchunk.division);
}

int ReadTrackHeader(FILE *inf,struct Track_chunk *tchunk)
{
	int c=0;	//Used for testing end of MIDI
	char header[5]={0};

	assert_wrapper((inf != NULL) && (tchunk != NULL));	//These must not be NULL

//Reset time and delta counters at the beginning of each track
	MIDIstruct.deltacounter=0;
	MIDIstruct.realtime=0;

	MIDIstruct.hchunk.curtempo=&DEFAULTTEMPO;
		//Until tempo is defined, 120BPM will be assumed.  The linked list header is still
		//NULL, so when the first Set Tempo is encountered, the default tempo will be scrapped

//Attempt to read one more byte, if EOF was triggered, we reached the end of the MIDI
	c=fgetc(inf);
	if(c==EOF)	//On failure to read, this function returns error instead of exiting
		return 1;

	header[0]=c;	//If we're here, there's a track header, and the first byte of it has been read
	fread_err(&(header[1]),3,1,inf);	//Read the other 3 bytes of the header
	header[4]='\0';	//Terminate string, which is now expected to be "MTrk"
	if(strcmp(header,"MTrk") != 0)
		return -1;	//Return invalid track header

	ReadDWORDBE(inf,&(tchunk->chunksize));

	return 0;	//Return end of track (not end of file)
}

int ReadVarLength(FILE *inf, unsigned long *ptr)
{	//Read variable length value into *ptr performing bit shifting accordingly.  Returns zero on success
	unsigned char c=0;		//used for input
	unsigned ctr=0;			//counter
	int readcount=0;		//Counter for the number of bytes read for this VLV
	unsigned long sum=0;	//unencoded variable length value

	assert_wrapper((inf != NULL) && (ptr != NULL));	//These must not be NULL

	for(ctr=1;ctr<5;ctr++)
	{
		c=fgetc_err(inf);
		readcount++;

		sum<<=7;			//Shift the ongoing sum
		sum+=(c & 0x7F);	//Add the value read, masking out the MSB, as it is for continuation and not data

		if((c & 0x80) == 0)	//If the continuation bit is not set,
		{
			*ptr=sum;	//there is no more data to read, save sum
			return readcount;	//return number of bytes read
		}
	}

	return 0;	//If we got here, we read 4 bytes and the continuation bit was still set.  This indicates an
				//invalid variable length value.  Return error
}

void WriteVarLength(FILE *outf, unsigned long value)
{	//Write variable length value to output file.  Returns zero on success, or 1 if value is too large to be written in a 4 byte VLV
	unsigned char buffer[4]={0};	//Store each byte in this (buffer[0] is MSB)
	unsigned char numbytes=1;		//Incremented for each continuation bit, this is how many bytes to write to file
	unsigned char ctr=0;
	unsigned long temp=value;

	assert_wrapper(outf != NULL);	//This must not be NULL

	if(value > 0x0FFFFFFFL)		//0x0FFFFFFF is the largest possible value that can be represented as a 4 byte VLV
	{
		printf("Error: Invalid variable length value write (%0lX) attempted\nAborting\n",value);
		exit_wrapper(1);
	}

	for(ctr=4;ctr>0;ctr--)
	{
		buffer[ctr-1] |= (char)(temp & 0x7F);	//Store low 7 bits in array
		if((temp>=128) && (ctr>1))		//If any higher bits are set (not possible in the last iteration)
		{
			buffer[ctr-2]=128;	//Set continuation bit in previous array element
			numbytes++;			//Increment number of bytes to write
		}
		temp = temp>>7;			//Shift right 7 bits
	}

	for(ctr=numbytes;ctr>0;ctr--)	//For each byte we need to write
	{
		assert_wrapper((ctr > 0) && (ctr<5));	//Prevent indexing outside of buffer[]
		fwrite_err(&(buffer[4-ctr]),1,1,outf);
	}
}

char *ReadMetaEventString(FILE *inf,long int length)
{
	char *buffer=NULL;

	assert_wrapper(inf != NULL);	//This must not be NULL

	if(length == 0)
	{
		if(Lyrics.verbose)	puts("Ignoring empty Meta Event string");
		return NULL;	//If the MIDI describes a string with 0 characters, ignore it
	}

	buffer=(char *)malloc_err((size_t)length+1);	//Allocate an array large enough to hold the event including null char

	fread_err(buffer,(size_t)length,1,inf);
	buffer[length]='\0';	//Null terminate the string
	return buffer;
}

unsigned long TrackEventProcessor(FILE *inf,FILE *outf,unsigned char break_on,char ismeta,int (*event_handler)(struct TEPstruct *data),char callbefore,struct Track_chunk *tchunk,char suppress_errors)
{	//Process events within a track, documenting to stdout.  If event number break_on is read and ismeta equals 0,
	//program returns with value 0.  If meta event type break_on is read and ismeta equals 1, program returns with
	//value of 0.  Otherwise the number of processed events is returned
	//This function takes a pointer to a function that uses a TEPstruct struct ptr. as a parameter and returns an
	//integer run status:
	//	-1 upon error, 0 upon event ignored, 1 upon event successfully handled
	//If callbefore is nonzero, the handler is called before the event is parsed further than its status byte,
	//otherwise it is called after the event is parsed and read into the TEPstruct.  If the function pointer is
	//NULL, no handler is called.
	struct TEPstruct vars={0,0,0,0,0,NULL,NULL,0,0,0,0,0,0,{0},NULL,NULL,0,0,0};		//Storage unit for all event processor variables
//	unsigned long ctr;			//Used for parsing lyric/text events
	long int deltalength=0;
	static unsigned char buffered=0;	//Stores the condition that one extra byte was read from file (ie. Running status)
										//So that instead of performing a costly fseek to rewind one byte, this status
										//indicates that the buffer variable below is storing the extra byte
	static unsigned char buffer=0;		//The extra byte that was read from the previous access.  Where fread is used,
										//in this function, the status of buffered should be checked to see whether one
										//less byte should be read
	unsigned char buffer2=0;			//Used to read the byte after a buffered byte above

	assert_wrapper((inf != NULL) && (tchunk != NULL));	//These must not be NULL (event handler is allowed to be NULL)
	//outf will only be checked for NULL within MIDI_Build_handler, where it is used

//Declare and initialize internal variables
	MIDIstruct.deltacounter=0;	//Should reset to 0 at beginning of each track
	MIDIstruct.absdelta=0;
	vars.lasteventtype=0;
	vars.lastwritteneventtype=0;
	vars.processed=0;
	vars.delta=0;
	vars.inf=inf;
	vars.outf=outf;
	if(MIDIstruct.MPQN_defined == 0)
		vars.current_MPQN=500000;//By definition of MIDI spec, 120BPM is assumed until a MPQN is defined explicitly
	vars.trackname=NULL;
	vars.tracknum=tchunk->tracknum;

	if(Lyrics.verbose)	puts("\n\tStart of track");

	while(1)
	{
		vars.buffer=NULL;		//If this is not NULL at the end of the loop, it will be de-allocated
		vars.runningstatus=0;	//Running status is not considered in effect until it is found

		vars.startindex=ftell_err(inf);	//Store file index of this event's delta value
		vars.endindex=0;		//0 until we have read the index of the next delta value

		if(Lyrics.verbose>=2)	printf("Delta file pos=0x%lX\t",vars.startindex);

//Expected input is the variable length delta value
		deltalength=ReadVarLength(inf,&(vars.delta));
		if(deltalength == 0)
		{
			if(!suppress_errors)
				puts("Error: Invalid variable length value encountered\nAborting");
			exit_wrapper(1);
		}

		MIDIstruct.deltacounter+=vars.delta;	//Add this to our ongoing counters
		MIDIstruct.absdelta+=vars.delta;
		vars.eventindex=vars.startindex+deltalength;	//Add the number of bytes read for the delta to find the file position

//v2.32	Fixed a bug in the logging that calculated the realtime incorrectly (using the wrong tempo)
//		if(Lyrics.verbose>=2)	printf("Delta time=%lu\tReal time=%fms \tEvent's file pos=0x%lX\t",vars.delta,(MIDIstruct.realtime+ (double)MIDIstruct.deltacounter / (double)MIDIstruct.hchunk.division * ((double)60000.0 / MIDIstruct.BPM)),vars.eventindex);
		if(Lyrics.verbose>=2)
				printf("Delta time=%lu\tReal time=%fms \tEvent's file pos=0x%lX\t",vars.delta,ConvertToRealTime(MIDIstruct.absdelta,0.0),vars.eventindex);	//Use the absolute delta counter

//Expected input is an event type (4 bits) and midi channel (4 bits)
		vars.eventtype=fgetc_err(inf);

//Check whether it is a running status byte, if so, we have to rewind one byte so the parameters are readable
		if(vars.eventtype < 0x80)	//All events have to have bit 7 set, if not, it's running status
		{
	//Validity checking for running status
			if((vars.lasteventtype>>4) == 0)
			{
				if(!suppress_errors)
					puts("Error: Running status encountered with no preceeding event\nAborting");
				exit_wrapper(2);
			}
			if((vars.lasteventtype>>4) >= 0xF)
			{	//running status is illegal for meta and sysex events
				if(!suppress_errors)
					puts("MIDI violation: Running status encountered for a Meta or Sysex event\nAborting");
				exit_wrapper(3);
			}
			else
			{
				if(Lyrics.verbose>=2)	printf(" (Running status): ");

				buffered=1;
				buffer=vars.eventtype;	//Store this byte and prevent the need to seek backward one byte

				vars.runningstatus=1;
				vars.eventtype=vars.lasteventtype;
			}
		}

//If this is the event upon which we intend to return before parsing, do so now
		if(((vars.eventtype>>4) == break_on) && (ismeta == 0))
			return 0;	//Return with found status

//If we specified to call a handler routine before parsing, do so now
		if((event_handler != NULL) && (callbefore != 0))
			(*event_handler)(&vars);

//Read in the MIDI event parameters
		if(vars.eventtype < 0xF0)
		{	//If it's not a meta or SysEx event, read two bytes to find the parameters
			if(buffered == 0)	//If a byte wasn't "un read" from further above, ie in the Running Status detection
				fread_err(vars.parameters,2,1,inf);	//Read two bytes normally
			else				//Otherwise use buffer as the first byte, and read the second byte
			{
				buffer2=fgetc_err(inf);			//Read one byte
				vars.parameters[1]=buffer2;		//Store it into parameters array
				vars.parameters[0]=buffer;		//Store pre-buffered byte into parameters array
				buffered=buffer=0;				//Clear the buffered status
			}
		}

//Identify event and output to stdout
		switch(vars.eventtype>>4)	//Shift out the controller number, just look at the event
		{
			case 0x8:	//Note off
				if(Lyrics.verbose>=2)	printf("Event: Note off (Channel=%d): Note #=%d, Velocity=%d\n",vars.eventtype&0xF,vars.parameters[0],vars.parameters[1]);
			break;

			case 0x9:	//Note on
				if(Lyrics.verbose>=2)
				{
					printf("Event: Note on (Channel=%d): Note #=%d, Velocity=%d",vars.eventtype&0xF,vars.parameters[0],vars.parameters[1]);
					if(vars.parameters[1] == 0)	//Note on with Velocity of 0 is equivalent to a Note off
						puts("\t(NOTE OFF)");
					else
						putchar('\n');
				}
			break;

			case 0xA:	//Note Aftertouch
				if(Lyrics.verbose>=2)	printf("Event: Note Aftertouch (Channel=%d): Note #=%d\tAmount=%d\n",vars.eventtype&0xF,vars.parameters[0],vars.parameters[1]);
			break;

			case 0xB:	//Controller
				if(Lyrics.verbose>=2)	printf("Event: Controller (Channel=%d): Controller type=%d\tValue=%d\n",vars.eventtype&0xF,vars.parameters[0],vars.parameters[1]);
			break;

			case 0xC:	//Program Change
//This event only takes one parameter, rewind file pointer by one byte
				fseek_err(inf,-1,SEEK_CUR);

				if(Lyrics.verbose>=2)	printf("Event: Program Change (Channel=%d): Program number=%d\n",vars.eventtype&0xF,vars.parameters[0]);
			break;

			case 0xD:	//Channel Aftertouch
//This event only takes one parameter, rewind file pointer by one byte
				fseek_err(inf,-1,SEEK_CUR);

				if(Lyrics.verbose>=2)	printf("Event: Channel Aftertouch (Channel=%d): Amount=%d\n",vars.eventtype&0xF,vars.parameters[0]);
			break;

			case 0xE:	//Pitch Bend
				if(Lyrics.verbose>=2)	printf("Event: Channel Aftertouch (Channel=%d): Pitch value=%d\n",vars.eventtype&0xF,((vars.parameters[1]&0x7F)<<7)+(vars.parameters[0]&0x7F));
			break;
			case 0xF:	//Meta Event (only if the lower half of eventtype = 0xF)
//Events 0xF0 and 0xF7 are SysEx events, 0xFF is a meta event
				if((vars.eventtype & 0xF) == 0xF)	//If it's a meta event
				{
//Read the meta event type and create a separate switch for the meta event type
					vars.m_eventtype=fgetc_err(inf);

//If this is the meta event we intend to return from function upon reading, do so now
					if((vars.m_eventtype == break_on) && (ismeta == 1))
						return 0;	//Return with found status

//Read the variable length "length" parameter
					if(ReadVarLength(inf,&vars.length) == 0)
					{
						if(!suppress_errors)
							printf("Error parsing Meta event: %s\nAborting\n",strerror(errno));
						exit_wrapper(4);
					}

					switch(vars.m_eventtype)
					{
						case 0x0:	//Sequence number, the length parameter must always be 2
							if(vars.length != 2)
							{
								if(!suppress_errors)
									printf("Error: Invalid sequence number meta event length (Delta time is at file position 0x%lX)\nAborting\n",vars.startindex);
								exit_wrapper(5);
							}
							fread_err(vars.parameters,2,1,inf);

							if(Lyrics.verbose)	printf("Meta Event: Sequence Number=%u\n",*((unsigned short *)vars.parameters));
						break;

						case 0x1:	//Text event
							vars.buffer=ReadMetaEventString(inf,vars.length);

							if(vars.buffer != NULL)	//If there was a string read from MIDI
							{	//Count the number of text/lyric events that don't begin with an open bracket ([)
								if(Lyrics.verbose)	printf("Meta Event: Text Event=\"%s\"\tLength=%d\n",vars.buffer,strlen(vars.buffer));
							}
						break;

						case 0x2:	//Copyright notice
							vars.buffer=ReadMetaEventString(inf,vars.length);
							if(Lyrics.verbose && vars.buffer)	printf("Meta Event: Copyright Notice=\"%s\"\tLength=%d\n",vars.buffer,strlen(vars.buffer));
						break;

						case 0x3:	//Sequence/Track name
							vars.trackname=ReadMetaEventString(inf,vars.length);
							if(vars.trackname != NULL)	//Only if the string wasn't empty
							{
								if(tchunk->trackname != NULL)	//If there is already a string here
									free(tchunk->trackname);	//release it before overwriting

								tchunk->trackname=vars.trackname;	//Store this string in the track header struct
								if(Lyrics.verbose)	printf("Meta Event: Track Name=\"%s\"\tLength=%d\n",vars.trackname,strlen(vars.trackname));

							//Special processing for quick processing
								if((vars.tracknum!=0) && (event_handler == NULL) && Lyrics.quick)	//If we're only processing for tempo changes and track names (track 0 must be processed completely)
									return vars.processed;

								if((vars.trackname != NULL) && (Lyrics.inputtrack != NULL))	//Only allow the rest of the track to be skipped if both the track name and the input track are defined
								{															//and they match.  Track 0 is forced to process
									if((vars.tracknum!=0) && Lyrics.quick && (strcasecmp(vars.trackname,Lyrics.inputtrack) != 0))
									{	//Not allowed to skip track 0
										if(Lyrics.verbose>=2)	printf("Quick processing specified and this is not \"%s\".  Skipping rest of track\n",Lyrics.inputtrack);
										return vars.processed;
									}
								}
							}
						break;

						case 0x4:	//Instrument Name
							vars.buffer=ReadMetaEventString(inf,vars.length);
							if((Lyrics.verbose>=2) && vars.buffer)	printf("Meta Event: Instrument Name=\"%s\"\tLength=%d\n",vars.buffer,strlen(vars.buffer));
						break;

						case 0x5:	//Lyrics
							vars.buffer=ReadMetaEventString(inf,vars.length);

							if(vars.buffer != NULL)	//If there was a string read from MIDI
							{	//Count the number of text/lyric events that don't begin with an open bracket ([)
								if(Lyrics.verbose)	printf("Meta Event: Lyric=\"%s\"\tLength=%d\n",vars.buffer,strlen(vars.buffer));
							}
						break;

						case 0x6:	//Marker
							vars.buffer=ReadMetaEventString(inf,vars.length);
							if((Lyrics.verbose>=2) && vars.buffer)	printf("Meta Event: Marker=\"%s\"\tLength=%d\n",vars.buffer,strlen(vars.buffer));
						break;

						case 0x7:	//Cue Point
							vars.buffer=ReadMetaEventString(inf,vars.length);
							if((Lyrics.verbose>=2)&& vars.buffer)	printf("Meta Event: Cue Point=\"%s\"\tLength=%d\n",vars.buffer,strlen(vars.buffer));
						break;

						case 0x20:	//MIDI channel prefix
							if(vars.length != 1)
							{
								if(!suppress_errors)
									printf("Error: Invalid MIDI channel prefix meta event length (Delta time is at file position 0x%lX)\nAborting\n",vars.startindex);
								exit_wrapper(6);
							}
							vars.parameters[0]=fgetc_err(inf);

							if(Lyrics.verbose>=2)	printf("Meta Event: MIDI channel prefix: %d\n",vars.parameters[0]);
						break;

						case 0x21:	//UNOFFICIAL MIDI EVENT: MIDI port prefix
							if(vars.length != 1)
							{
								if(!suppress_errors)
									printf("Error: Invalid MIDI port prefix meta event length (Delta time is at file position 0x%lX\nAborting\n",vars.startindex);
								exit_wrapper(7);
							}
							vars.parameters[0]=fgetc_err(inf);

							if(Lyrics.verbose>=2)	printf("Meta Event: MIDI port prefix: %d\n",vars.parameters[0]);
						break;

//Upon reading the end of track midi event, return from function
						case 0x2F:	//End of Track
							if(vars.length != 0)
							{
								if(!suppress_errors)
									printf("Error: Invalid End of Track meta event length (Delta time is at file position 0x%lX)\nAborting\n",vars.startindex);
								exit_wrapper(8);
							}
							vars.processed++;		//This counts as an event, increment the counter
							if(Lyrics.verbose)
							{
								printf("End of track.  %lu events processed\n",vars.processed);
								if(tchunk->textcount || tchunk->notecount)
								{
									if(tchunk->textcount)
										printf("(%lu viable lyric/text events found)",tchunk->textcount);
									if(tchunk->notecount)
										printf("\t(%lu Note On events found)",tchunk->textcount);
									putchar('\n');
								}
							}

							vars.endindex=ftell_err(inf);	//Points to either end of file or start of next track header
							if((event_handler != NULL) && (callbefore == 0))
								(*event_handler)(&vars);	//Call event handler if one is designated

							if(MIDIstruct.hchunk.tempomap == NULL)	//If a tempo was never set
							{
								if(Lyrics.verbose)	puts("No tempo defined.  120BPM is assumed");
								MIDIstruct.hchunk.tempomap=&DEFAULTTEMPO;	//Define tempo linked list to be a single link with a tempo of 120BPM
							}

							if((tchunk->trackname != NULL) && (Lyrics.inputtrack != NULL))
								if(strcasecmp(tchunk->trackname,Lyrics.inputtrack) == 0)
									MIDIstruct.endtime=MIDIstruct.realtime+((double)MIDIstruct.deltacounter / (double)MIDIstruct.hchunk.division * ((double)60000.0 / MIDIstruct.BPM));

							if(vars.trackname != NULL)	//If there's a track name being remembered
								vars.trackname=NULL;	//forget it now that the end of the track has been reached

							return vars.processed;
						break;

						case 0x51:	//Set Tempo
							if(vars.length != 3)
							{
								if(!suppress_errors)
									printf("Error: Invalid Set Tempo Meta event length (Delta time is at file position 0x%lX)\nAborting\n",vars.startindex);
								exit_wrapper(9);
							}
							fread_err(vars.parameters,3,1,inf);

							if(vars.tracknum != 0)
							{
								if(!suppress_errors)
									printf("Error: Set Tempo event encountered outside of track 0 (in track # %lu, delta time is at file position 0x%lX)\nAborting\n",vars.tracknum,vars.startindex);
								exit_wrapper(10);
							}
							vars.current_MPQN=(vars.parameters[0]<<16) | (vars.parameters[1]<<8) | vars.parameters[2];	//Convert MPQN (Microseconds per quarter note)
							MIDIstruct.MPQN_defined=1;
//Convert cumulative delta time to real time, add to our real time clock
							MIDIstruct.realtime+=(double)MIDIstruct.deltacounter / (double)MIDIstruct.hchunk.division * ((double)60000.0 / MIDIstruct.BPM);
							MIDIstruct.BPM=(double)60000000.0/(double)vars.current_MPQN;	//Calculate new tempo

//Track the information in our tempo linked list by adding a link
							if(MIDIstruct.hchunk.tempomap == NULL)
							{	//Special condition:  Empty list (this is the first Tempo Change)
								MIDIstruct.hchunk.tempomap=(struct Tempo_change *)malloc_err(sizeof(struct Tempo_change));
								MIDIstruct.hchunk.curtempo=MIDIstruct.hchunk.tempomap;	//Conductor points at start of list
							}
							else	//A link exists already
							{
								MIDIstruct.hchunk.curtempo->next=(struct Tempo_change *)malloc_err(sizeof(struct Tempo_change));
								MIDIstruct.hchunk.curtempo=MIDIstruct.hchunk.curtempo->next;	//Conductor points at new link
							}

							MIDIstruct.hchunk.curtempo->next=NULL;		//No further BPM changes found yet
							MIDIstruct.hchunk.curtempo->delta=MIDIstruct.deltacounter;
							MIDIstruct.hchunk.curtempo->realtime=MIDIstruct.realtime;
							MIDIstruct.hchunk.curtempo->BPM=MIDIstruct.BPM;

							MIDIstruct.deltacounter=0;		//Reset cumulative delta time to 0
							if(Lyrics.verbose>=2)	printf("Meta Event: Set Tempo=%lu MPQN (%f BPM)\n",vars.current_MPQN,(double)60000000.0/(double)vars.current_MPQN);
						break;

						case 0x54:	//SMPTE Offset
							if(vars.length != 5)
							{
								if(!suppress_errors)
									printf("Error: Invalid SMPTE Offset meta event length (Delta time is at file position 0x%lX)\nAborting\n",vars.startindex);
								exit_wrapper(11);
							}
							fseek_err(inf,5,SEEK_CUR);		//Skip this event

							if(Lyrics.verbose>=2)	puts("Meta Event: SMPTE Offset\t(Ignoring)");
						break;

						case 0x58:	//Time Signature
							if(vars.length != 4)
							{
								if(!suppress_errors)
									printf("Error: Invalid Time Signature meta event length (Delta time is at file position 0x%lX)\nAborting\n",vars.startindex);
								exit_wrapper(12);
							}
							fread_err(vars.parameters,4,1,inf);

							if(Lyrics.verbose)	printf("Meta Event: Time Signature: Num=%u, Den=(2^)%u, Metro=%u, 32nds=%u\n",vars.parameters[0],vars.parameters[1],vars.parameters[2],vars.parameters[3]);
						break;

						case 0x59:	//Key Signature
							if(vars.length != 2)
							{
								if(!suppress_errors)
									printf("Error: Invalid Key Signature meta event length (Delta time is at file position 0x%lX)\nAborting\n",vars.startindex);
								exit_wrapper(13);
							}
							fread_err(vars.parameters,2,1,inf);

							if(Lyrics.verbose>=2)	printf("Meta Event: Key Signature: Key=%d, scale=%d\n",(char)vars.parameters[0],vars.parameters[1]);
						break;

						case 0x7F:	//Sequencer-specific
							vars.buffer=(char *)malloc_err((size_t)vars.length+1);
							fread_err(vars.buffer,(size_t)vars.length,1,inf);

							vars.buffer[vars.length]='\0';	//Null terminate the string
						break;

						default:
							if(!suppress_errors)
								printf("Error: Unsupported Meta event 0x%X (Delta time is at file position 0x%lX)\nAborting\n",vars.m_eventtype,vars.startindex);
							exit_wrapper(14);
						break;
					}//End switch(vars.m_eventtype)
				}//End if((vars.eventtype & 0xF) == 0xF)
				else if( ((vars.eventtype & 0xF) == 0) || ((vars.eventtype & 0xF) == 0x7))
				{	//This is a SysEx event
//If this is the meta event we intend to return from function upon reading, do so now
					if((vars.m_eventtype == break_on) && (ismeta == 1))
						return 0;	//Return with found status

//Read the variable length "length" parameter
					if(ReadVarLength(inf,&vars.length) == 0)
					{
						if(!suppress_errors)
							printf("Error parsing SysEx event: %s\nAborting\n",strerror(errno));
						exit_wrapper(15);
					}
					fseek_err(inf,vars.length,SEEK_CUR);

					if(Lyrics.verbose)	printf("SysEx Event encountered.  Skipping %lu bytes\n",vars.length);
				}
				else
				{
					if(!suppress_errors)
						printf("Error: Invalid MIDI event (Delta time is at file position 0x%lX)\nAborting\n",vars.startindex);
					exit_wrapper(16);
				}
			break;//case 0xF

			default:
				if(!suppress_errors)
					printf("Error: Unsupported MIDI event 0x%X, file position 0x%lX\nAborting\n",(vars.eventtype>>4),ftell_err(inf));
				exit_wrapper(17);
			break;
		}//End switch(vars.eventtype>>4)

		vars.processed++;	//If we reached the end of the loop, we processed an event

//! I have noticed a trend in some Rock Band MIDIs that will nest Meta events inside a running status, expecting
//	the running status to be valid after the Meta event is parsed.  Technically, this violates the MIDI standard.
//	This conditional statement will attempt to continue a running status in this scenario
		if((vars.eventtype >> 4) != 0xF)		//If this event wasn't a Meta/Sysex event
			vars.lasteventtype=vars.eventtype;	//Remember this in case Running Status is encountered

		vars.endindex=ftell_err(inf);	//Store current file index

//Before finishing out this iteration of the loop, call the event handler if specified
		if((event_handler != NULL) && (callbefore == 0))
			(*event_handler)(&vars);

		if(vars.buffer != NULL)		//If we allocated memory to hold data
		{
			free(vars.buffer);	//release it before moving to the next event
			vars.buffer=NULL;	//Reset value to NULL
		}
	}//End while(1)
}

int Lyricless_handler(struct TEPstruct *data)
{	//This handler will expect proper MIDI notes with no overlapping/nesting
	char placeholder[]="*";
	double time=0.0;
	unsigned char eventtype=0;

	static unsigned long lastlyrictime=0;
		//Retains the timestamp in ms of the last Note on
	static unsigned char lyric_note_num=0;
		//For the sake of handling problematic input files, this will be used to handle overlapping notes

	if(Lyrics.reinit)
	{	//Re-initialize static variables
		Lyrics.reinit=0;
		lastlyrictime=0;
		lyric_note_num=0;
	}

	assert_wrapper(data != NULL);	//This must not be NULL

	eventtype=data->eventtype;//This value may be interpreted differently for Note On w/ velocity=0

	//After ensuring that the track name string is populated,
	//Check to make sure this is within the specified MIDI track.  If not, return without doing anything
	if(data->trackname == NULL)
		return 0;
	assert_wrapper(Lyrics.inputtrack != NULL);
	if(strcasecmp(data->trackname,Lyrics.inputtrack) != 0)
		return 0;

//Special case:  A Note On event with a velocity of 0 must be treated as a Note Off event
	if(((data->eventtype>>4) == 0x9) && (data->parameters[1] == 0))
		eventtype=0x80;	//Treat data->eventtype into Note Off, leave it as Note On in the original TEPstruct

//Handle Note On event
	if((eventtype>>4) == 0x9)
	{	//If note on in the vocal track
		//This Note On is associated with the previous Lyric event, store the note number
		if((data->parameters[0] < MIDIstruct.diff_lo) || (data->parameters[0] > MIDIstruct.diff_hi))
		{
			MIDIstruct.mixedtrack=1;	//Note that this track contains more than just vocal rhythm
			return 0;			//This note is outside the target difficulty, ignore it
		}

		lyric_note_num=data->parameters[0];
	//Configure variables to track this event
		if(Lyrics.lyric_on == 0)
		{	//Only if no notes are currently on
			lastlyrictime=(unsigned long)ConvertToRealTime(MIDIstruct.deltacounter,MIDIstruct.realtime);
				//Store timestamp of this Note On event
			Lyrics.lyric_on=1;
			lyric_note_num=data->parameters[0];		//Will only process a Note Off for this note number
		}
		else
			return 0;	//Don't perform processing for nested notes

		return 1;
	}

//Handle Note Off event
	if((eventtype>>4) == 0x8)
	{	//If note off in the vocal track
		//Ensure that the lyric has had a Note On event already
		if((data->parameters[0] < MIDIstruct.diff_lo) || (data->parameters[0] > MIDIstruct.diff_hi))
			return 0;	//If this note is outside the target difficulty, ignore it

		if(Lyrics.lyric_on == 0)
		{
			printf("Error: Lyric note off detected without note on (Delta time for the event is at file position %lX)\nAborting\n",data->startindex);
			exit_wrapper(1);
		}
		if(data->parameters[0] != lyric_note_num )	//If this Note Off doesn't correspond to the note number we're expecting
			return 0;								//ignore it
	//Add the lyric piece
		time=ConvertToRealTime(MIDIstruct.deltacounter,MIDIstruct.realtime);	//End offset of lyric piece in realtime
		AddLyricPiece(placeholder,lastlyrictime,(unsigned long)time,lyric_note_num,0);	//Add the lyric placeholder to the lyric storage by providing the lyric string, the start time
			//in milliseconds and the end time in millieseconds.  Store the pitch specified by the Note number
		if(Lyrics.verbose>=2)	printf("Added lyric placeholder %lu: Start=%lu\tEnd=%lu\n",Lyrics.piececount,lastlyrictime,(unsigned long)time);

  		return 1;
	}

	return 0;
}

void MIDI_Load(FILE *inf,int (*event_handler)(struct TEPstruct *data),char suppress_errors)
{
	struct Track_chunk temp={0,0,NULL,0,0,0,0,0};	//Used to count the track headers
	unsigned long ctr=0;		//The currently parsed MIDI track number
	size_t temp2=0;
	int error=0;				//error returned from ReadTrackHeader
	struct Track_chunk *newtrack=NULL;	//Used to reallocate tracks[] array in the event of a damaged MIDI
	unsigned long ctr2=0;				//Used to rebuild tracks[] array in the event of a damaged MIDI

	assert_wrapper(inf != NULL);	//A filename must have been passed to this function

	if(Lyrics.verbose)
	{
		if(event_handler != NULL)	//An event handler was passed, and lyrics will be loaded
			printf("\nImporting MIDI lyrics from file \"%s\"\n\n",Lyrics.infilename);
		else	//Only timing and track information is being loaded
			printf("\nImporting MIDI timing from file\n\n");
	}

	rewind_err(inf);		//Rewind to beginning of file
	ReadMIDIHeader(inf,0);	//Load and validate the MIDI header

	if(Lyrics.verbose>=2)	printf("MIDI header indicates %u tracks\n",MIDIstruct.hchunk.numtracks);

	temp2=ftell_err(inf);

//Do a manual count of the tracks headers in the file and store each track's location
	while(1)
	{
		error=ReadTrackHeader(inf,&temp);	//Read the next track
		if(error)	//If end of file or invalid track header is read
			break;	//exit loop

		if(ctr+1 > MIDIstruct.hchunk.numtracks)
		{	//If this is one more track than was defined in the MIDI header
			puts("\aWarning: Input MIDI file is damaged (too many tracks are defined).  Correcting...");

			//Reallocate tracks array
			newtrack=(struct Track_chunk *)calloc_err(1,sizeof(struct Track_chunk)*(ctr+1));	//Re-allocate an array of track headers one larger than the current array and init to 0
			for(ctr2=0;ctr2<ctr+1;ctr2++)	//Initialize track numbers
				(newtrack[ctr2]).tracknum=ctr2;

			memcpy(newtrack,MIDIstruct.hchunk.tracks,sizeof(struct Track_chunk)*MIDIstruct.hchunk.numtracks);	//Copy existing tracks array content into new array
			free(MIDIstruct.hchunk.tracks);	//release old array
			MIDIstruct.hchunk.tracks=newtrack;	//store new array
			newtrack=NULL;
			MIDIstruct.hchunk.numtracks++;	//Increment track count
		}

		if(Lyrics.verbose>=2)	printf("MIDI track %lu begins at byte 0x%lX\n",ctr,(unsigned long)temp2);
		(MIDIstruct.hchunk.tracks[ctr]).fileposition=temp2;	//Store file position for this track
		ctr++;		//Increment track counter
		fseek_err(inf,temp.chunksize,SEEK_CUR);		//Fast forward to next track header
		temp2=ftell_err(inf);
	}

	if(ctr == 0)	//If there were no tracks in the input MIDI
	{
		puts("Error: No tracks detected in the specified MIDI file\nAborting");
		exit_wrapper(1);
	}

	if(ctr < MIDIstruct.hchunk.numtracks)
	{	//If too few tracks were defined
		if(error == -1)	//If it was due to an error in the MIDI
		{
			printf("Error: Incorrect track header at byte 0x%lX\nAborting\n",ftell_err(inf)-4);
			exit_wrapper(1);	//Abort
		}
		else
			puts("\aWarning: Input MIDI file is damaged (too few tracks are defined)");
	}
	else if(ctr > MIDIstruct.hchunk.numtracks)
	{	//This shouldn't happen, as the logic above would resize the tracks array and increment MIDIstruct.hchunk.numtracks
		puts("Logic error: Corrected track count failed.\nAborting");
		exit_wrapper(2);
	}

//Override the number of tracks specified in the MIDI file because we validated it ourselves
	MIDIstruct.hchunk.numtracks=ctr;
	ctr=0;			//reset counter

//v2.32	Updated this logic, as the first track header won't be at byte 14 if the MIDI is embedded within a RIFF or RBA file
//	fseek_err(inf,14,SEEK_SET);		//Rewind to the first track header (always at byte 14)
	fseek_err(inf,(MIDIstruct.hchunk.tracks[0]).fileposition,SEEK_SET);	//Rewind to first track header

//We are expecting the track header
	while((ctr < MIDIstruct.hchunk.numtracks) && (ReadTrackHeader(inf,&(MIDIstruct.hchunk.tracks[ctr])) == 0))
	{	//For each track, starting at zero
		TrackEventProcessor(inf,NULL,0x1,0,event_handler,0,&(MIDIstruct.hchunk.tracks[ctr]),suppress_errors);
		ctr++;

		if(Lyrics.quick && (ctr<MIDIstruct.hchunk.numtracks))	//If track processing is allowed to stop early
		{	//Only skip to next track if there is another one
			if(Lyrics.verbose>=2)	printf("Skipping to track at byte 0x%lX\n",(MIDIstruct.hchunk.tracks[ctr]).fileposition);

			fseek_err(inf,(MIDIstruct.hchunk.tracks[ctr]).fileposition,SEEK_SET);		//Seek to next track manually if there is one
		}
	}

	if(MIDIstruct.unfinalizedlyric)
	{	//If there is an unfinalized, pitchless lyric
		if(Lyrics.verbose)	printf("Forcefully closing lyric \"%s\"\n",MIDIstruct.unfinalizedlyric);
		AddMIDILyric(MIDIstruct.unfinalizedlyric,MIDIstruct.unfinalizedlyrictime,PITCHLESS,MIDIstruct.unfinalizedoverdrive_on,MIDIstruct.unfinalizedgroupswithnext);
			//Recover lyric with timestamp, overdrive status and grouping status.  Add it to the MIDI lyric list and finalize it
		EndMIDILyric(PITCHLESS,MIDIstruct.unfinalizedlyrictime+1);	//Formally end it
		GetLyricPiece();	//Remove the now completed lyric from the MIDI lyric list and store in the Lyrics list
		EndLyricLine();		//End the lyric line gracefully
	}

	ForceEndLyricLine();
	if(Lyrics.verbose)
	{
		if(event_handler != NULL)
			printf("MIDI import complete.  %lu lyrics loaded\n\n",Lyrics.piececount);
		else
			puts("MIDI parse complete\n");
	}
}

void ReleaseMIDI(void)
{
	struct Track_chunk *trackstemp=NULL;	//Used for deallocation at end of import
	unsigned long ctr=0;					//Used for deallocation at end of import
	struct Tempo_change *tempotemp=NULL,*tempotemp2=NULL;	//Used for deallocation at end of import

	if(Lyrics.verbose >= 2)	puts("Cleaning up MIDI structure");

//Release memory outside of Lyrics structure that was used for import
	//De-allocate MIDI track array and track name strings
	trackstemp=MIDIstruct.hchunk.tracks;
	if(trackstemp != NULL)
	{
		for(ctr=0;ctr<MIDIstruct.hchunk.numtracks;ctr++)
			if((trackstemp[ctr]).trackname != NULL)
				free((trackstemp[ctr]).trackname);

		free(trackstemp);
		MIDIstruct.hchunk.tracks=NULL;
	}
	MIDIstruct.hchunk.numtracks=0;

	//De-allocate tempo change linked list
	tempotemp=MIDIstruct.hchunk.tempomap;
	if(tempotemp == &DEFAULTTEMPO)	//If there were no set tempo events read from the last MIDI file
		return;

	while(tempotemp != NULL)
	{
		assert_wrapper(tempotemp != &DEFAULTTEMPO);
		tempotemp2=tempotemp->next;	//Save pointer to next link
		free(tempotemp);			//Deallocate link
		tempotemp=tempotemp2;		//Continue for next link (if it exists)
	}
	MIDIstruct.hchunk.tempomap=NULL;
	MIDIstruct.hchunk.curtempo=NULL;
}

double ConvertToRealTime(unsigned long absolutedelta,double starttime)
{	//This function expects the Tempo linked list to have been fully populated by reading Track 0.  absolutedelta
	//is a delta counter that will be converted to realtime starting at the tempo change defined in starttime.
	//If starttime is 0, all tempo changes are checked starting with the first, otherwise tempo changes are
	//checked starting with the one specified
	struct Tempo_change *temp=MIDIstruct.hchunk.tempomap;	//Point to first link in list
	double temptimer=starttime;	//Will be used to seek to appropriate beginning tempo change
	unsigned long tempdelta=absolutedelta;
	double tempBPM=0.0;			//Stores BPM of current tempo change

	if(temp == NULL)
	{
//v2.32	Alter behavior of this function to be aware of the default tempo of 120BPM
//		puts("Error: Invalid linked list when converting to real time\nAborting");
//		exit_wrapper(1);
		tempBPM=120.0;
	}
	else
	{
		while((temp->next != NULL) && (temptimer >= (temp->next)->realtime))	//Seek to tempo change indicated by starttime
		{
			tempdelta-=temp->delta; //subtract this tempo change's delta
			temp=temp->next;
		}

		temptimer=temp->realtime;	//Start with the timestamp of the starting tempo change
		tempBPM=temp->BPM;

	//Find the real time of the specified delta time, which is relative from the defined starting timestamp
		while((temp->next != NULL))
		{		//Traverse tempo changes until tempdelta is insufficient to reach the next tempo change
				//or until there are no further tempo changes
			if(tempdelta >= (temp->next)->delta)	//If the delta time is enough to reach next tempo change
			{
				temp=temp->next;			//Advance to next tempo change
				temptimer=temp->realtime;	//Set timer
				tempdelta-=temp->delta;		//Subtract the tempo change's delta time
				tempBPM=temp->BPM;			//Set BPM
			}
			else
				break;	//break from loop
		}
	}

//At this point, we have reached the tempo change that absolutedelta resides within
//tempdelta is now relative to this tempo change, find the realtime of tempdelta
	temptimer+=(double)tempdelta / (double)MIDIstruct.hchunk.division * ((double)60000.0 / tempBPM);

	return temptimer;
}

void CopyTrack(FILE *inf,unsigned short tracknum,FILE *outf)	//Copies the specified MIDI track to the output file stream.  Expects the MIDI
{								//structure to have been populated by parsing the input MIDI.
	char header[5]={0};			//Array used to read in the track header string for validation
	unsigned long chunksize=0;	//Used to read in the track size for validation
//	char *buffer;				//Array used to read an entire track into memory and then copy the track to file

	assert_wrapper((inf != NULL) && (outf != NULL));	//These must not be NULL

//Validate track number from which to copy
	if(tracknum >= MIDIstruct.hchunk.numtracks)	//MIDI track numbers count starting from 0
	{
		printf("Error copying track #%u.  The MIDI's last track is #%u\nAborting",tracknum,MIDIstruct.hchunk.numtracks-1);
		exit_wrapper(1);
	}

//Seek to the header of the specified track
	fseek_err(inf,(MIDIstruct.hchunk.tracks[tracknum]).fileposition,SEEK_SET);

//Check that the file stream pointer is pointing to the start of a MIDI track by loading the header
	fread_err(header,4,1,inf);
	header[4]='\0';				//Terminate buffer to create a string, which is now expected to be "MTrk"

	if(strcmp(header,"MTrk") != 0)
	{
		puts("Error: Incorrect track header during copy\nAborting");
		exit_wrapper(2);
	}

//At this point, validation of the track header succeeded, validate the track size before continuing
	ReadDWORDBE(inf,&chunksize);

	if(chunksize != (MIDIstruct.hchunk.tracks[tracknum]).chunksize)	//Compare to the size for this track number listed in the MIDI structure
	{
		puts("Error: Non-matching track size encountered during copy\nAborting");
		exit_wrapper(3);
	}

//At this point, the input FILE stream has been shown to begin on a track header and be the expected length, so copy it to memory
	//The MIDI header has been read, so write it back to the output file
	fputs_err(header,outf);			//Write "MTrk"
	WriteDWORDBE(outf,chunksize);	//Write the track chunk size

	if(BlockCopy(inf,outf,chunksize) != 0)
	{
		puts("Error: Unable to copy MIDI track\nAborting");
		exit_wrapper(4);
	}
}

unsigned long ConvertToDeltaTime(unsigned long starttime)
{	//Uses the Tempo Changes list to calculate the absolute delta time of the specified realtime
	struct Tempo_change *temp=MIDIstruct.hchunk.tempomap;	//Begin with first tempo change
	unsigned long deltacounter=0;		//Begin with delta of first tempo change
	double temptime=0.0;

	assert_wrapper(temp != NULL);	//Ensure the tempomap is populated

	deltacounter=temp->delta;		//Begin with delta of first tempo change

//Seek to the latest tempo change at or before the specified real time value, adding delta time values
	while((temp->next != NULL) && (starttime >= (temp->next)->realtime))	//For each timestamp after the first
	{	//If the starttime timestamp is equal to or greater than this timestamp,
		temp=temp->next;		//Advance to that time stamp
		deltacounter+=temp->delta;	//Add the delta value for the time stamp to our counter
	}

//Deltacounter is now the delta time of the latest tempo change the specified timestamp can reach
	temptime=starttime - temp->realtime;	//Find the relative timestamp from this tempo change

//temptime is the amount of time we need to find a delta for, and add to deltacounter
//By using NewCreature's formula:	realtime = (delta / divisions) * (60000.0 / bpm)
//The formula for delta is:		delta = realtime * divisions * bpm / 60000
	deltacounter+=(unsigned long)((temptime * (double)MIDIstruct.hchunk.division * temp->BPM / (double)60000.0 + (double)0.5));
		//Add .5 so that the delta counter is rounded to the nearest 1

	return deltacounter;
}

void Copy_Source_MIDI(FILE *inf,FILE *outf)
{	//Copy all tracks besides destination vocal track to output file, return the number of tracks written
	//If the import format is MIDI, the source vocal track will be excluded from export as well
	unsigned short ctr=0,ctr2=0;
	char buffer[14]={0};		//Used to read in the MIDI file header
	long chunkfileposition=0;	//store this so we can write the track chunk size at the end
	long endchunkfileposition=0;//store this to allow seeking back to end of the rebuilt track to allow next track to be written
	long chunkfilesize=0;

	assert_wrapper((inf != NULL) && (outf != NULL) && (Lyrics.outputtrack != NULL));	//These must not be NULL

	if(Lyrics.verbose)	puts("Copying tracks from source MIDI");

//Read+write the file header
	rewind_err(inf);					//Rewind to beginning of file
	fread_err(buffer,14,1,inf);		//Read MIDI header
	fwrite_err(buffer,14,1,outf);	//Write MIDI header

//Copy tracks to output file except PART VOCALS and except for the vocal rhythm notes if applicable
	for(ctr=0;ctr<MIDIstruct.hchunk.numtracks;ctr++)	//For each track
	{
		if(Lyrics.verbose)
		{
			if((MIDIstruct.hchunk.tracks[ctr]).trackname != NULL)
				printf("\tTrack %u (\"%s\"): ",MIDIstruct.trackswritten,(MIDIstruct.hchunk.tracks[ctr]).trackname);
			else
				printf("\tTrack %u: ",ctr);
		}

		//Track 0 will often have no track name defined, all un-named tracks must be copied explicitly
		if((MIDIstruct.hchunk.tracks[ctr]).trackname != NULL)	//If this track has a name
			if(!strcasecmp((MIDIstruct.hchunk.tracks[ctr]).trackname,Lyrics.outputtrack) || ((Lyrics.out_format != VRHYTHM_FORMAT) && ((Lyrics.inputtrack != NULL) && !strcasecmp((MIDIstruct.hchunk.tracks[ctr]).trackname,Lyrics.inputtrack))))
			{	//If this track has the same name as the track to which output vocals will be written, or if it has the same name as the input vocal track (vrhythm import excluded)
				if(Lyrics.verbose)	puts("Omitting existing input/output track");
				continue;
			}

		if((ctr==0) || (Lyrics.in_format != VRHYTHM_FORMAT) || ((MIDIstruct.hchunk.tracks[ctr]).trackname == NULL))
		{	//Use original copy logic for track 0, any track without a name or any non vocal rhythm import
			if((MIDIstruct.hchunk.tracks[ctr]).trackname && (Lyrics.out_format == SKAR_FORMAT) && (!strcasecmp((MIDIstruct.hchunk.tracks[ctr]).trackname,"Soft Karaoke") || !strcasecmp((MIDIstruct.hchunk.tracks[ctr]).trackname,"Words")))
			{	//Do not copy tracks named "Soft Karaoke" or "Words" from source MIDI file during Soft Karaoke export
				if(Lyrics.verbose)	printf("Omitting existing input track \"%s\"\n",(MIDIstruct.hchunk.tracks[ctr]).trackname);
				continue;
			}

			if(Lyrics.verbose)	puts("Copying...");
			CopyTrack(inf,ctr,outf);	//Copy to output file
			MIDIstruct.trackswritten++;		//Remember how many tracks were copied
		}
		else
		{		//Perform more complex logic to remove the vocal rhythm notes from the exported MIDI
			assert_wrapper(Lyrics.inputtrack != NULL);	//If Vrhythm import was performed, Lyrics.inputtrack must have already been set

			if(strcasecmp((MIDIstruct.hchunk.tracks[ctr]).trackname,Lyrics.inputtrack) != 0)
			{	//If this MIDI track is different from the track that contains the imported vocal rhythm
				if(Lyrics.verbose)	puts("Copying...");
				CopyTrack(inf,ctr,outf);	//Copy the entire track to output file
				MIDIstruct.trackswritten++;
			}
			else
			{	//This MIDI track contains vocal rhythm notes
				if(MIDIstruct.mixedtrack == 0)	//If this track contained no normal instrument notes
				{
					if(Lyrics.verbose)	puts("Contains only vocal rhythm.  Omitting...");
					continue;
				}

				//This track will have to be rebuilt to not contain vocal rhythm notes
				if(Lyrics.verbose)	puts("Contains instrument and vocal rhythm notes.  Rebuilding...");
				fseek_err(inf,(MIDIstruct.hchunk.tracks[ctr]).fileposition+8,SEEK_SET);	//Seek just beyond the track header (8 bytes)
			//Write the track header
				chunkfileposition=Write_MIDI_Track_Header(outf);

			//Write track events (which should include the track name and end of track events)
				TrackEventProcessor(inf,outf,(unsigned char)0x1,(char)0,MIDI_Build_handler,(char)0,&(MIDIstruct.hchunk.tracks[ctr]),0);	//Rebuild the MIDI track to exclude vocal rhythm notes
				MIDIstruct.trackswritten++;
				endchunkfileposition=ftell_err(outf);	//Save this to allow us to rewind to finish writing the track

			//Rewind to the track chunk size location and write the correct value
				chunkfilesize=endchunkfileposition-chunkfileposition-4;	//# of bytes from start to end of the track chunk (- chunk size variable).  The null padding will be omitted
				fseek_err(outf,chunkfileposition,SEEK_SET);			//Rewind to where the chunk size is supposed to be written

				for(ctr2=0;ctr2<4;ctr2++)							//Write chunk size in reverse order
					fwrite_err(((unsigned char *)(&chunkfilesize))+3-ctr2,1,1,outf);

				fseek_err(outf,endchunkfileposition,SEEK_SET);		//Seek to end of current track
			}//else
		}//else
	}//for(ctr=0;ctr<MIDIstruct.hchunk.numtracks;ctr++)
}

void Export_MIDI(FILE *outf)
{	//A MIDI track is written with lyric and note events based on the contents of the Lyrics structure
	unsigned long reldelta=0;	//The relative delta compared to this delta time and the last delta time
								//Stores a relative delta for a start of a lyric piece
	unsigned long thisdelta=0;	//The absolute delta time of the current lyric/line start
	unsigned long lastdelta=0;	//We have to store the last absolute delta of the end of a lyric so we can keep
								//track of relative delta times between events
	unsigned long lyrdelta=0;	//The relative delta value to use for a lyric piece.  If a new line is started, this is 0
	unsigned long extradelta=0;	//To account for the delta time that would otherwise be missed when not writing Note Off for PITCHLESS lyrics
	struct Lyric_Line *curline=NULL;	//A conductor for the lyric lines list
	struct Lyric_Piece *temp=NULL;		//A conductor for the lyric pieces list
	struct Lyric_Piece *temp2=NULL;		//A temporary pointer used to handle overdrive
	char line_marked=0;			//Boolean:  The current line of lyrics has been marked with a Note 105 on event
	char overdrive=0;			//Boolean:	Overdrive is enabled (a Note On has been created)
	unsigned int channelnum=((MIDIstruct.hchunk.numtracks-1) & 0xF); //This is the channel number to write events to (0-15)
	long chunkfileposition=0;	//store this so we can write the track chunk size at the end
	long chunkfilesize=0;
	unsigned char pitch=LYRIC_NOTE_ON;	//This will be set to LYRIC_NOTE_ON if Lyrics.pitch_detection
										//is false, otherwise the input lyric pitches will be used
	char *tempstr=NULL;
	char runningstatus=0;	//Used for running status:  Since all non Meta/SysEx events written in this function are
							//Note On events, this boolean can be passed to Write_MIDI_Note() and set after the first call,
							//implementing running status

	assert_wrapper(outf != NULL);	//This must not be NULL (inf is allowed to in order to control how the output file is written)
	assert_wrapper(Lyrics.outputtrack != NULL);	//This must be set during or before the source MIDI logic
	assert_wrapper((Lyrics.out_format == MIDI_FORMAT) || (Lyrics.out_format == KAR_FORMAT));	//This export function only handles these two formats
	assert_wrapper(Lyrics.piececount != 0);	//This function is not to be called with an empty Lyrics structure

	if(Lyrics.verbose)	printf("\nExporting MIDI lyrics to file \"%s\"\n",Lyrics.outfilename);

	if(!Lyrics.pitch_tracking)
		puts("\a! NOTE: No pitch variation found in input lyrics.  Exporting as Freestyle");

	if(Lyrics.verbose)	printf("\tTrack %u (\"%s\"): Building...\n",MIDIstruct.trackswritten,Lyrics.outputtrack);

//Write the output lyric track
 //Write the track header
	chunkfileposition=Write_MIDI_Track_Header(outf);

 //Write track name (meta event 3) with a delta of 0
	WriteMIDIString(outf,0,TRACK_NAME_MIDI_STRING,Lyrics.outputtrack);

 //Before writing the abosolute first lyric event, embed various pieces of information
	WriteMIDIString(outf,0,SEQSPEC_MIDI_STRING,PROGVERSION);

	if(Lyrics.Title != NULL)
	{
		tempstr=Append("Title=",Lyrics.Title);		//Write Title tag as a single string
		WriteMIDIString(outf,0,SEQSPEC_MIDI_STRING,tempstr);
		free(tempstr);
	}
	if(Lyrics.Artist != NULL)
	{
		tempstr=Append("Artist=",Lyrics.Artist);	//Write Artist tag as a single string
		WriteMIDIString(outf,0,SEQSPEC_MIDI_STRING,tempstr);
		free(tempstr);
	}
	if(Lyrics.Album != NULL)
	{
		tempstr=Append("Album=",Lyrics.Album);		//Write Album tag as a single string
		WriteMIDIString(outf,0,SEQSPEC_MIDI_STRING,tempstr);
		free(tempstr);
	}
	if(Lyrics.Editor != NULL)
	{
		tempstr=Append("Editor=",Lyrics.Editor);	//Write Editor tag as a single string
		WriteMIDIString(outf,0,SEQSPEC_MIDI_STRING,tempstr);
		free(tempstr);
	}

 //Write the lyrics
 	overdrive=0;
	curline=Lyrics.lines;	//Point conductor to first line of lyrics
	while(curline != NULL)
	{	//For each line of lyrics
		line_marked=0;		//The first lyric in the line will mark the start of the line as well
		temp=curline->pieces;	//Starting with the first piece of lyric in this line
		while(temp != NULL)
		{	//For each piece of lyric in the line
			thisdelta=ConvertToDeltaTime(temp->start);//Delta is the start absolute delta time of this lyric
			assert_wrapper(thisdelta>=lastdelta);	//There's been some serious malfunction if this is false
			reldelta=thisdelta-lastdelta;	//Find the delta value relative to the last lyric event
			lyrdelta=reldelta;	//Unless we find a new line is starting this is the lyric's delta value

	//Perform RB MIDI specific phrase marking logic
			if(Lyrics.out_format == MIDI_FORMAT)
			{
				if(line_marked == 0)	//If we need to mark the start of the line of lyrics
				{
					lyrdelta=0;	//The lyric's delta will be 0, following the line start event

		//Write Note 105 On event (line start)
				//Write delta value of this start of line (equal to the delta of the first lyric in a line)
					WriteVarLength(outf,reldelta+extradelta);
					extradelta=0;	//Ensure this is reset since the appropriate delta time was written
					if(Lyrics.verbose >= 2)	putchar('\n');
					Write_MIDI_Note(105,channelnum,MIDI_NOTE_ON,outf,runningstatus);
					runningstatus=1;	//Any note written after the first can be running status
					line_marked=1;	//This will remain 1 until the start of the next line
				}

		//Handle Note 116 On event (Overdrive start)
				if(temp->overdrive && !overdrive)
				{	//If this lyric is overdrive, and an overdrive marker isn't in progress
					WriteVarLength(outf,lyrdelta+extradelta);
					extradelta=0;	//Ensure this is reset since the appropriate delta time was written
					Write_MIDI_Note(116,channelnum,MIDI_NOTE_ON,outf,runningstatus);
					runningstatus=1;	//Any note written after the first can be running status
					overdrive=1;
					lyrdelta=0;	//The lyric's delta will be 0, following the overdrive start event
				}
			}

	//Write lyric
			//If the lyric is freestyle, append a pound symbol (#), as per Rock Band convention, unless the user is prohibiting this behavior
			if((Lyrics.out_format == MIDI_FORMAT) && !Lyrics.nofstyle)	//If the export format is RB MIDI
				if(temp->freestyle || !Lyrics.pitch_tracking)		//If freestyle is explicitly or implicitly defined
					if(temp->lyric[strlen(temp->lyric)-1] != '#')	//And the lyric doesn't already end in a # char
						temp->lyric=ResizedAppend(temp->lyric,"#",1);	//Re-allocate string to have a # char appended

		//Perform unofficial KAR grouping logic (append whitespace to lyric if it does not group with the next lyric)
			if((Lyrics.out_format == KAR_FORMAT) && !temp->groupswithnext && (temp->next != NULL))
				temp->lyric=ResizedAppend(temp->lyric," ",1);	//Resize string to include a space at the end

			WriteMIDIString(outf,lyrdelta+extradelta,LYRIC_MIDI_STRING,temp->lyric);
			extradelta=0;	//Ensure this is reset since the appropriate delta time was written

		//Perform unofficial KAR line break logic
			if((Lyrics.out_format == KAR_FORMAT) && (temp->next == NULL))	//If this was the last lyric in the line
				WriteMIDIString(outf,0,LYRIC_MIDI_STRING,"\n");		//Write a newline character lyric event 0 deltas away from the line's last lyric

			lastdelta=ConvertToDeltaTime(temp->start+temp->duration);	//lastdelta=abs delta of end of lyric, this must be calculated even if Note On/Off is not being written for the lyric

		//Write Note On and Off events unless -nopitch was specified and there was no defined pitch for the lyric
			if(!(Lyrics.nopitch && (temp->pitch == PITCHLESS)))
			{	//If the condition to skip writing the Note events (nopitch specified and pitchless lyric) is NOT true, write Note On and Off
			//Write note on 0 deltas away from the lyric event
				if(Lyrics.pitch_tracking)
					pitch=temp->pitch;

				if(temp->pitch == PITCHLESS)	//If a pitchless lyric is to be written
					pitch=LYRIC_NOTE_ON;		//Change it to the generic pitch specified in lyric_storage.h

				WriteVarLength(outf,0+extradelta);
				extradelta=0;	//Ensure this is reset since the appropriate delta time was written
				Write_MIDI_Note(pitch,channelnum,MIDI_NOTE_ON,outf,runningstatus);
				runningstatus=1;	//Any note written after the first can be running status

			//Write note off (note #LYRIC_NOTE_ON off) the correct number of deltas away from the note on event
				if(lastdelta < thisdelta)
				{
					printf("Unexpected error: temp->start=%lu\ttemp->duration=%lu\tduration delta=%lu\nAborting\n",temp->start,temp->duration,lastdelta);
					exit_wrapper(1);
				}
				WriteVarLength(outf,lastdelta-thisdelta);

				Write_MIDI_Note(pitch,channelnum,MIDI_NOTE_OFF,outf,runningstatus);
				runningstatus=1;	//Any note written after the first can be running status
			}
			else	//If the Note On and Off events were not written, the delta time for where the Note Off would have gone needs to be accounted for
				extradelta=lastdelta-thisdelta;	//Track this difference to add to the next delta time that is written

	//Perform RB MIDI specific phrase marking logic
			if(Lyrics.out_format == MIDI_FORMAT)
			{
		//Handle Note 116 Off event (Overdrive end)
				if(overdrive)
				{	//If an overdrive phrase is in progress
					temp2=temp->next;								//temp2 points to the next lyric of this line
					if((temp2 == NULL) && (curline->next != NULL))	//If there's no next piece in this line, but there's another line
						temp2=curline->next->pieces;				//temp2 points to the first lyric of the next line, otherwise it would be NULL

					if(!temp2 || !temp2->overdrive)
					{	//If there are no more lyrics, or the next one doesn't use overdrive, turn overdrive off
						WriteVarLength(outf,0+extradelta);	//Write delta value of 0, as this should end at the same time as the Note off for the last lyric in this phrase that used overdrive
						extradelta=0;	//Ensure this is reset since the appropriate delta time was written
						Write_MIDI_Note(116,channelnum,MIDI_NOTE_OFF,outf,runningstatus);
						runningstatus=1;	//Any note written after the first can be running status
						overdrive=0;
					}
				}
			}

			temp=temp->next;	//Advance to next lyric piece in this line
		}//while(temp != NULL) 	(For each piece of lyric in the line)

//Perform RB MIDI specific phrase marking logic
		if(Lyrics.out_format == MIDI_FORMAT)
		{
	//Write Note 105 Off event (line end)
	//Write delta value 0 (line ends 0 deltas away from the Note Off event for the last lyric)
			WriteVarLength(outf,0+extradelta);
			extradelta=0;	//Ensure this is reset since the appropriate delta time was written
			Write_MIDI_Note(105,channelnum,MIDI_NOTE_OFF,outf,runningstatus);
			runningstatus=1;	//Any note written after the first can be running status
		}

		curline=curline->next;	//Advance to next line of lyrics
	}//while(Lyrics.curline != NULL)

//Write end of track event with a delta of 0 and a null padding byte
	WriteDWORDBE(outf,0x00FF2F00UL);	//Write 4 bytes: 0, 0xFF, 0x2F and 0

	chunkfilesize=ftell_err(outf)-chunkfileposition-4;	//# of bytes from start to end of the track chunk (- chunk size variable)

//Rewind to the track chunk size location and write the correct value
	fseek_err(outf,chunkfileposition,SEEK_SET);			//Rewind to where the chunk size is supposed to be written

	WriteDWORDBE(outf,chunkfilesize);	//Write chunk size in BE format

	MIDIstruct.trackswritten++;	//One additional track has been written to the output file
	if(Lyrics.verbose)	printf("\nMIDI export complete.  %lu lyrics written\n",Lyrics.piececount);
}

void WriteMIDIString(FILE *outf,unsigned long delta,int stringtype, const char *str)
{
	assert_wrapper((stringtype == 0x1) || (stringtype == 0x3) || (stringtype == 0x5) || (stringtype == 0x7F));
	assert_wrapper((outf != NULL) && (str != NULL));	//These must not be NULL

	if(Lyrics.verbose>=2)
	{
		printf("\tWriting MIDI string type %d: ",stringtype);
		if(!strcmp(str,"\n"))	//If the string is just a newline character (used for line breaks in KAR export)
			puts("(Newline character)");
		else
			printf("\"%s\"\n",str);
	}

	//write a delta time, write Meta Event type, length of the string and the string text itself
	WriteVarLength(outf,delta);
	fputc_err(0xFF,outf);
	fputc_err(stringtype,outf);
	WriteVarLength(outf,strlen(str));
	fputs_err(str,outf);
}

void Parse_Song_Ini(char *midipath,char loadoffset,char loadothertags)
{
	FILE *inf=NULL;
	char *tempstr=NULL;		//Temporary string
	char *fullpath=NULL;	//The final path to song.ini
	char *buffer=NULL;		//Will be an array large enough to hold the largest line of text from input file
	unsigned long maxlinelength=0;	//I will count the length of the longest line (including NULL char/newline) in the
					//input file so I can create a buffer large enough to read any line into

	if(midipath == NULL)
	{
		puts("Error: Invalid string passed to Parse_Song_Ini()\nAborting");
		exit_wrapper(1);
	}

	if(Lyrics.verbose>=2)	puts("Creating song.ini path string");

//Make a working copy of the MIDI path string
	tempstr=DuplicateString(midipath);

	if(Lyrics.verbose>=2)
	{
		printf("MIDI path string is \"%s\"\n",tempstr);
		puts("Truncating song.ini path string");
	}

//Truncate the MIDI's filename to leave the path to its parent folder
	buffer=strrchr(tempstr,'\\');	//Temporarily store the address of the last backslash in the file path
	if(buffer != NULL)				//There are folders specified in the path to the MIDI
	{
		buffer[1]='\0';				//Place a NULL after the last backslash

		if(Lyrics.verbose)	printf("Appending \"song.ini\" to folder path \"%s\"\n",tempstr);

//Allocate new string to store file path to song.ini
		fullpath=ResizedAppend(tempstr,"song.ini",0);
	}
	else
		fullpath=DuplicateString("song.ini");

	if(Lyrics.verbose)	printf("Attempting to open file: \"%s\"\n",fullpath);

//Attempt to open song.ini to see if it exists
	inf=fopen(fullpath,"r");	//Try to open the file, but don't exit it it cannot be opened

	free(tempstr);	//No longer needed

	if(inf == NULL)		//file doesn't exist
	{
		if(Lyrics.verbose)	printf("! Unable to open file \"%s\"\n",fullpath);
		free(fullpath);	//No longer needed
		return;		//return from function
	}

	free(fullpath);	//No longer needed

//Find the length of the longest line
	maxlinelength=FindLongestLineLength(inf,0);
	if(!maxlinelength)
	{
		puts("\aWarning: Song.ini is empty");
		fclose_err(inf);
		return;		//Do not process song.ini if it is empty
	}

//Allocate memory buffer large enough to hold any line in this file
	buffer=(char *)malloc_err(maxlinelength);

//Release memory for and reset tag identifier strings
	if(Lyrics.TitleStringID != NULL)
		free(Lyrics.TitleStringID);
	if(Lyrics.ArtistStringID != NULL)
		free(Lyrics.ArtistStringID);
	if(Lyrics.AlbumStringID != NULL)
		free(Lyrics.AlbumStringID);
	if(Lyrics.EditorStringID != NULL)
		free(Lyrics.EditorStringID);
	if(Lyrics.OffsetStringID != NULL)
		free(Lyrics.OffsetStringID);

	Lyrics.TitleStringID=NULL;
	Lyrics.ArtistStringID=NULL;
	Lyrics.AlbumStringID=NULL;
	Lyrics.EditorStringID=NULL;
	Lyrics.OffsetStringID=NULL;

	if(Lyrics.verbose)	puts("Beginning song.ini parse");

	if(loadothertags)	//Load tags that aren't the offset tag
	{
		Lyrics.TitleStringID=DuplicateString("name");
		Lyrics.ArtistStringID=DuplicateString("artist");
		Lyrics.AlbumStringID=DuplicateString("album");
		Lyrics.EditorStringID=DuplicateString("frets");
	}

	if(loadoffset)	//Load the offset tag
		Lyrics.OffsetStringID=DuplicateString("delay");

	while(fgets(buffer,maxlinelength,inf) != NULL)		//Read lines until end of file is reached, don't exit on EOF
		ParseTag('=','\0',buffer,1);	//Look for tags, content starts after '=' char and extends to end of line.  Negatize the offset

	free(buffer);	//No longer needed, release the memory before exiting function
	fclose_err(inf);
}

void VRhythm_Load(char *srclyrname,char *srcmidiname,FILE *inf)
{
	FILE *inputlyrics=NULL;
	char buffer[11]={0};	//Load max of 10 characters from input lyrics file
	char *temp=NULL,*temp2=NULL;
	char trackID=0;
	char diffID=0;
	unsigned ctr=0;
	unsigned long ctr2=0;
	unsigned char note1=0,note2=0,splitlogic=0;	//Used to add line breaks during lyricless vrhythm import

//Validate parameters
	if(((!Lyrics.nolyrics && (srclyrname == NULL))) || (inf == NULL) || (srcmidiname == NULL))
	{	//Allow srclyrname to be NULL if nolyrics is in effect
		puts("Error: Null parameter passed to VRhythm_Load\nAborting");
		exit_wrapper(1);
	}

	if(Lyrics.verbose)	printf("Importing vocal rhythm from file \"%s\"\n\n",Lyrics.infilename);

	if(!Lyrics.nolyrics)	//If regular pitched lyric logic is in effect
	{	//Open input pitched lyric file to read vrhythm ID
		inputlyrics=fopen_err(srclyrname,"rt");

	//Read first line, which should indicate the target track and difficulty in a format such as "midi = D4" (Drums>Expert)
		fgets_err(buffer,11,inputlyrics);	//Read first line (or first 10 bytes) of file into buffer
		buffer[10]='\0';					//Ensure the string is terminated

	//Validate and process first line
		temp=strcasestr_spec(buffer,"midi");	//Get the address of the character after a match for "midi", if it exists
		if(temp != NULL)
			temp2=strchr(temp,'=');				//Look for an equal sign in the portion of the string that followed the match for "midi", if applicable
		if(!temp || !temp2)						//If first line does not contain "midi" followed by an equal sign
		{
			printf("Error: Invalid vocal rhythm track identifier \"%s\" in input lyric file \"%s\"\nAborting\n",buffer,srclyrname);
			exit_wrapper(2);
		}
		temp2++;	//Seek past equal sign
		while(isspace(temp2[0]))
			temp2++;	//Seek to first non whitespace character (should be the instrument track identifier)

		trackID=tolower(temp2[0]);	//Store the track identifier in lowercase
		diffID=temp2[1];			//Store the difficulty identifier

		assert_wrapper(Lyrics.inputtrack == NULL);	//This string must not already be set if vrhythm import is being performed
		Lyrics.inputtrack=AnalyzeVrhythmID(temp2);	//Set MIDIstruct.diff_lo and MIDIstruct.diff_hi and store MIDI track name to load vrhythm from
		if(Lyrics.inputtrack == NULL)	//Validate return value
		{
			printf("Error: Invalid input vocal rhythm identifier \"%s\"\nAborting\n",temp2);
			exit_wrapper(3);
		}
	}//if(!Lyrics.nolyrics)
	else	//Perform lyricless vrhythm import logic
	{
		assert_wrapper(Lyrics.inputtrack != NULL);	//Ensure that AnalyzeVrhythmID() was used to validate the vrhythm ID
		assert_wrapper(MIDIstruct.diff_lo && MIDIstruct.diff_hi);	//Ensure these aren't zero
	}

//Load delay and other tags from song.ini
	Parse_Song_Ini(srcmidiname,1,1);	//Load ALL tags from song.ini first, as the delay tag will affect timestamps

//Load MIDI information
	Lyrics.quick=1;			//We can skip processing tracks besides track 0 and "PART VOCALS" (which will be ignored anyway)
	if(Lyrics.verbose>=2)	puts("Importing source MIDI (quick processing)");
	MIDI_Load(inf,NULL,0);	//Call MIDI_Load with no handler (just load MIDI info such as track names)

	if(Lyrics.verbose)	printf("\nSearching for MIDI track \"%s\", difficulty %c\n",Lyrics.inputtrack,diffID);

//Search for the appropriate MIDI track
	for(ctr=0;ctr<MIDIstruct.hchunk.numtracks;ctr++)	//For each MIDI track
	{
		if((MIDIstruct.hchunk.tracks[ctr]).trackname == NULL)	//If this track name doesn't exist (ie. Track 0)
			continue;	//Skip to next track

		if(!strcasecmp((MIDIstruct.hchunk.tracks[ctr]).trackname,Lyrics.inputtrack))
			break;

		if((trackID == 'd') && (!strcasecmp((MIDIstruct.hchunk.tracks[ctr]).trackname,"PART DRUM")))
		{	//If we're looking for the drum track, also compare it to "PART DRUM".  If a match is found, exit loop
			free(Lyrics.inputtrack);
			Lyrics.inputtrack=DuplicateString("PART DRUM");	//Reassign "PART DRUM" as the target MIDI track
			break;
		}
	}

	if(ctr >= MIDIstruct.hchunk.numtracks)	//The specified track doesn't exist in the input MIDI
	{
		printf("Error: Track \"%s\" is missing from input MIDI\nAborting\n",Lyrics.inputtrack);
		exit_wrapper(4);
	}

//Seek to the appropriate MIDI track and past the track header (header is 8 bytes long)
	fseek_err(inf,(MIDIstruct.hchunk.tracks[ctr]).fileposition+8,SEEK_SET);

//Initialize lyric line
	CreateLyricLine();

//Process MIDI track
	Lyrics.quick=0;		//This must be reset otherwise the Event Processor will skip everything but PART VOCALS
	ctr2=TrackEventProcessor(inf,NULL,0x1,0,Lyricless_handler,0,&(MIDIstruct.hchunk.tracks[ctr]),0);
		//Process all events in the track with the Lyricless event handler

	if(Lyrics.verbose)	printf("\nProcessed %lu events in vocal rhythm file\n",ctr2);

//Finalize lyric line
	EndLyricLine();

	if(Lyrics.verbose)	printf("Vocal rhythm import complete.  %lu vocal rhythm notes loaded\n\n\n",Lyrics.piececount);

//Load pitched lyrics
	if(Lyrics.nolyrics == 0)
	{
		PitchedLyric_Load(inputlyrics);
		fclose_err(inputlyrics);
	}
	else
	{
		if(Lyrics.verbose)	puts("Vrhythm ID supplied instead of a pitched lyric file, skipping pitched lyric import\nPerforming lyric line splitting logic");

//Perform logic for line splitting based on fret number changes in notes
		Lyrics.curline=Lyrics.lines;	//Point conductor to first (only) line of lyrics
		while(Lyrics.curline != NULL)
		{
			Lyrics.curline->curpiece=Lyrics.curline->pieces;			//Point conductor to first lyric in the line
			if(Lyrics.verbose)	printf("\tChecking line beginning at timestamp %lu\n",Lyrics.curline->start);

			if(Lyrics.curline->curpiece == NULL)						//If the line is now empty
				return;													//return

			note1=Lyrics.curline->curpiece->pitch;						//Store the note number of the first entry in this line
			if(Lyrics.verbose)	printf("\t\tFirst fret in line is #%u\n",note1-MIDIstruct.diff_lo+1);
			Lyrics.curline->curpiece=Lyrics.curline->curpiece->next;	//Point to next lyric
			if(Lyrics.curline->curpiece == NULL)						//If that was the only remaining lyric in the line
				return;													//return

			note2=Lyrics.curline->curpiece->pitch;						//Store the note number of the second entry in this line
			if(Lyrics.verbose)	printf("\t\tSecond fret in line is #%u\n",note2-MIDIstruct.diff_lo+1);

			if(note1 != note2)											//If the two vrhythm notes were different fret numbers
				splitlogic=1;											//enable line splitting logic
			else
				splitlogic=0;											//otherwise turn it off

			Lyrics.curline->curpiece=Lyrics.curline->curpiece->next;	//Point to next lyric
			while(Lyrics.curline->curpiece != NULL)
			{	//While there are lyrics to process
				if(Lyrics.curline->curpiece->pitch != note2)			//If the vrhythm note fret number just changed
				{
					if(Lyrics.verbose)	printf("\t\tSplitting line at timestamp %lu\n",Lyrics.curline->curpiece->start);

					Lyrics.curline=InsertLyricLineBreak(Lyrics.curline,Lyrics.curline->curpiece);	//Insert a line break
					break;																			//break to outer loop to reprocess this lyric as the first in its line
				}

				Lyrics.curline->curpiece=Lyrics.curline->curpiece->next;	//Iterate to next lyric
			}

			//All lyrics are processed, iterate to next line, if one should exist
			if(Lyrics.curline->curpiece == NULL)		//Conditionally iterate to next line, since if a split occurred,
				Lyrics.curline=Lyrics.curline->next;	//curline points to the new line already
		}//while(Lyrics.curline != NULL)

		RecountLineVars(Lyrics.lines);
	}//else
}

void PitchedLyric_Load(FILE *inf)
{
	unsigned long maxlinelength=0;	//I will count the length of the longest line (including NULL char/newline) in the
									//input file so I can create a buffer large enough to read any line into
	char *buffer=NULL;				//Will be an array large enough to hold the largest line of text from input file
	unsigned long index=0;			//Used to index within a line of text
	long int pitch=0;				//Converted unsigned long value of numerical string representing a lyric's pitch
	unsigned long processedctr=0;	//The current line number being processed in the text file
	unsigned long splitctr=0;		//The number of lyrics processed since the last line split
	struct Lyric_Piece *lyrptr=NULL;//Conductor for Lyric piece linked lists
	unsigned long lyricctr=0;

	char notename=0;					//Used for parsing note names instead of pitch numbers
	char noteissharp=0;
	char noteisflat=0;
	int octavenum=0;
	char negativeoctave=0;
	char invalidnote=0;

	assert_wrapper(inf != NULL);	//This must not be NULL

	Lyrics.curline=Lyrics.lines;	//Reset the internal line linked list conductor to first line
	assert_wrapper(Lyrics.curline != NULL);	//Ensure Lyrics structure is populated

	lyrptr=Lyrics.curline->pieces;	//Set Lyric piece conductor to first lyric piece
	Lyrics.overdrive_on=0;	//Reset overdrive to off until a golden note lyric is read
	Lyrics.freestyle_on=0;	//Reset freestyle to off until a freestyle lyric is read
	Lyrics.curline->piececount=Lyrics.curline->start=Lyrics.curline->duration=0;	//Reset these values

//Find the length of the longest line
	maxlinelength=FindLongestLineLength(inf,1);

//Allocate memory buffer large enough to hold any line in this file
	buffer=(char *)malloc_err(maxlinelength);

//Load each line and parse it
	fgets_err(buffer,maxlinelength,inf);	//Read and ignore first line of text, as it is the vrhythm track identifier

	assert_wrapper(Lyrics.srclyrname != NULL);
	if(Lyrics.verbose)	printf("\nImporting pitched lyrics from file \"%s\"\n\n",Lyrics.srclyrname);

	processedctr=1;			//This will be set to 2 at the beginning of the main loop, denoting starting at line 2
	while(!feof(inf))		//Until end of file is reached
	{
		if(fgets(buffer,maxlinelength,inf) == NULL)	//Read next line of text, capping it to prevent buffer overflow, don't exit on EOF
			break;	//If NULL is returned, EOF was reached and no bytes were read from file

		processedctr++;
//Skip leading whitespace
		index=0;	//Reset index counter to beginning
		while(1)
		{
			if((buffer[index] != '\n') && (isspace((unsigned char)buffer[index])))
				index++;	//If this character is whitespace, skip to next character
			else
				break;		//Only break out of this loop if end of line or non whitespace is reached
		}

		if((buffer[index] == '\r') || (buffer[index] == '\n'))	//If it was a blank line
		{
			if(Lyrics.verbose)	printf("Ignoring blank line (line #%lu) in input file\n",processedctr);
			fgets(buffer,maxlinelength,inf);	//Read next line of text, so the EOF condition can be checked, don't exit on EOF
			continue;	//Skip processing for a blank line
		}

//Verify that the last characters of the string are not newline or carriage return
		while(1)
		{
			if((buffer[strlen(buffer)-1]=='\n') || (buffer[strlen(buffer)-1]=='\r'))
				buffer[strlen(buffer)-1]='\0';
			else
				break;
		}

//Process line of input
		assert_wrapper(buffer != NULL);

		if(isdigit(buffer[index]) || (buffer[index] == '#') || isalpha(buffer[index]))
		{	//If this line of input begins with a number, # character or letter
			if(isdigit(buffer[index]))
			{	//Parse pitch
				Lyrics.freestyle_on=0;	//Freestyle is disabled
				pitch=(unsigned char)(ParseLongInt(buffer,&index,processedctr,NULL) & 0xFF);	//Force pitch as an 8-bit value

				//Track for pitch changes, enabling Lyrics.pitch_tracking if applicable
				if((Lyrics.last_pitch != 0) && (Lyrics.last_pitch != pitch))	//There's a pitch change
					Lyrics.pitch_tracking=1;
				Lyrics.last_pitch=pitch;	//Consider this the last defined pitch
			}
			else if(buffer[index] == '#')
			{	//Treat as pitchless
				pitch=PITCHLESS;
				Lyrics.freestyle_on=1;	//Enable freestyle on this lyric
				//Find the next non pound character
				while(buffer[index] != '\0')
				{
					if(buffer[index] != '#')
						break;

					index++;
				}
			}
			else
			{	//If the line starts with a letter, parse it as a note number (ie. A#, B5, C#2, etc)
				notename=noteissharp=noteisflat=negativeoctave=invalidnote=0;	//Init note name variables
				octavenum=4;		//The middle octave is assumed unless the octave is defined

				notename=toupper(buffer[index++]);
				if(buffer[index] == '#')
				{	//If this note name has a sharp symbol
					noteissharp=1;
					index++;		//Seek past sharp symbol
				}
				else if(toupper(buffer[index]) == 'B')
				{	//If this note name has a flat symbol
					noteisflat=1;
					index++;		//Seek past flat symbol
				}

				if(buffer[index] == '-')
				{	//Check for octave -1
					negativeoctave=1;
					index++;
				}

				if(isdigit(buffer[index]))
				{
					octavenum=buffer[index++]-'0';	//Record octave number

					if((toupper(buffer[index]) == 'B') || (buffer[index] == '#'))
					{	//If a sharp or flat indicator followed the octave number
						printf("Note name syntax error in line %lu of pitched lyric file.  Sharp or flat indicator must precede octave number\nAborting\n",processedctr);
						exit_wrapper(1);
					}
				}

				//Validate note (C-1 is the lowest MIDI pitch allowed, value 0, and G9 is the highest MIDI pitch allowed, value 127)
				if((notename < 'A') || (notename > 'G'))
					invalidnote=1;
				else if(negativeoctave)
				{
//v2.32	This logic broke the ability to import valid notes A-1 through B-1
//					if(notename < 'C')	//The lowest valid note on octave -1 is C
//						invalidnote=1;
					if(octavenum != 1)	//The only valid negative octave is -1
						invalidnote=1;
				}
				else if((octavenum == 9) && (notename == 'G') && noteissharp)	//G#9 is just above the highest valid note
					invalidnote=1;
				else if((octavenum == -1) && (notename == 'C') && noteisflat)	//Cb-1 is just below the lowest valid note
					invalidnote=1;

				if(invalidnote)
				{
					printf("Error: Invalid note name in line %lu during pitched lyric import\nAborting\n",processedctr);
					exit_wrapper(1);
				}

				//Convert note name to numerical pitch
				pitch=0;
				if(!negativeoctave)				//If the specified octave is not -1
					pitch+=(octavenum*12)+12;	//Add value of C in the given octave
				switch(notename)				//Add value of note in current octave
				{
					case 'B':
						pitch+=11;
					break;

					case 'A':
						pitch+=9;
					break;

					case 'G':
						pitch+=7;
					break;

					case 'F':
						pitch+=5;
					break;

					case 'E':
						pitch+=4;
					break;

					case 'D':
						pitch+=2;
					break;

					default:
					break;
				}
				if(noteissharp)
					pitch++;					//Add value of note being sharp
				else if (noteisflat)
					pitch--;					//Subtract value of note being flat

				//Track for pitch changes, enabling Lyrics.pitch_tracking if applicable
				if((Lyrics.last_pitch != 0) && (Lyrics.last_pitch != pitch))	//There's a pitch change
					Lyrics.pitch_tracking=1;
				Lyrics.last_pitch=pitch;	//Consider this the last defined pitch
			}//end pitch parsing

			if(((pitch < 0) || (pitch > 127)) && (pitch != PITCHLESS))
			{
				printf("Error: Invalid pitch (%ld) detected in line %lu during pitched lyric import\nAborting\n",pitch,processedctr);
				exit_wrapper(2);
			}

			//Find the next non whitespace character
			while(buffer[index] != '\0')
			{
				if(isspace((unsigned char)buffer[index]) == 0)
					break;

				index++;
			}

			//Verify that there are enough lyric entries created during vocal rhythm import to load this lyric
			if(lyrptr == NULL)
			{
				printf("\aWarning: Not enough notes in imported vocal rhythm file to insert lyric \"%s\" with pitch %u.\n",&(buffer[index]),(unsigned char)pitch);
				puts("Pitched lyric import incomplete.  Examine the output file to determine the necessary corrections to input files");
				break;	//Break from while loop to stop reading input file
			}

			lyricctr++;
			if(Lyrics.verbose>=2)
			{
				printf("Processing pitched lyric #%lu:\t\"%s\"\tStart=%lums\t",lyricctr,buffer,lyrptr->start);
				if(pitch == PITCHLESS)
					puts("No pitch");
				else
					printf("Pitch=%ld\n",pitch);
			}

			//Copy lyric string and pitch into the existing lyric entry
			free(lyrptr->lyric);	//free lyric placeholder

			lyrptr->lyric=TruncateString(&(buffer[index]),0);		//Copy lyric text, removing leading and trailing whitespace from lyric, deallocate old string and save new string

			if(lyrptr->lyric[strlen(lyrptr->lyric)-1] == '-')		//If the lyric ends in a hyphen
				lyrptr->groupswithnext=1;	//This lyric will group with the next lyric

			lyrptr->pitch=pitch;			//Write lyric pitch
			if(Lyrics.overdrive_on)			//Write vocal style if applicable
				lyrptr->overdrive=1;

			if(Lyrics.freestyle_on)
				lyrptr->freestyle=1;

			lyrptr=lyrptr->next;	//Advance to next lyric piece
			splitctr++;				//Increment this counter
		}//end if(isdigit(buffer[index]) || (buffer[index] == '#') || isalpha(buffer[index]))
		else if(buffer[index]=='-')	//If this line of input begins with a phrase marker
		{
			if(strchr(buffer,'*') != NULL)	//If the line of input contains an asterisk
				Lyrics.overdrive_on=1;		//Enable lyric overdrive
			else
				Lyrics.overdrive_on=0;		//Otherwise disable overdrive

			//Verify that there are enough lyric entries created during vocal rhythm import to load this lyric
			if(lyrptr == NULL)
			{
				puts("\aUnexpected end of vocal rhythm notes.\n\tDo not end the pitched lyric file with a line beginning with a hyphen.");
				break;	//Break from while loop to stop reading input file
			}

			if(splitctr != 0)	//Split the linked list into two different lines, but only if lyrics have been processed since beginning of file or last line split
			{
				if(Lyrics.verbose>=2)	puts("\tInserting line break");
				Lyrics.curline=InsertLyricLineBreak(Lyrics.curline,lyrptr);	//Insert a line break in front of current lyric
			}
			else if(Lyrics.verbose)
				puts("Empty line phrase encountered.  Ignoring");
		}
		else
		{
			printf("Error: Invalid input \"%s\" in line %lu during pitched lyric import\nAborting\n",&(buffer[index]),processedctr);
			exit_wrapper(3);
		}
	}//while(!feof(inf))

	free(buffer);	//No longer needed, release the memory before exiting function

	if(Lyrics.verbose>=2)	puts("Rebuilding Lyric line structures");
	RecountLineVars(Lyrics.lines);	//Repair the piececount and duration variables that would have been made incorrect by inserting line breaks

	if(Lyrics.verbose)	printf("Pitched lyric import complete.  %lu lyric pieces loaded\n\n",lyricctr);
}

int MIDI_Build_handler(struct TEPstruct *data)
{
	static unsigned long counter=0;
		//This counter will retain the delta from the previous call, just in case the last event parsed was a note that should be omitted from the exported MIDI
		//Such a delta value will be added with the delta time of the current call to keep all events in the correct timing.
	static char skippedpreviousevent=0;
		//This is a flag that indicates whether the last event was skipped.  This should be tracked because if the following event uses running status,
		//the Note On/Off event may need to be manually stated to avoid writing the events incorrectly.  It would be necessary IF the skipped event
		//does not use running status, but the following event does.
	char *buffer=NULL;	//Used to store event
	unsigned long eventlength=0;			//The number of bytes to read into the buffer
//	static unsigned char lasteventtype=0;	//Used for running status
//	unsigned char velocity;					//Used for converting Note Off events to Note On

	if(Lyrics.reinit)
	{	//Re-initialize static variables
		Lyrics.reinit=0;
		counter=0;
		skippedpreviousevent=0;
	}

	assert_wrapper(data != NULL);		//This must not be NULL
	assert_wrapper(data->outf != NULL);	//This must not be NULL
	assert_wrapper(data->endindex > data->eventindex);	//The event being copied cannot be <= 0 bytes long

	eventlength=data->endindex-data->eventindex;

	counter+=data->delta;		//Store this event's delta time
	if(((data->eventtype>>4) == 0x9) || ((data->eventtype>>4) == 0x8))						//If this is a Note on or off event
		if((data->parameters[0] >= MIDIstruct.diff_lo) && (data->parameters[0] <= MIDIstruct.diff_hi))	//If this note is within the range of notes to omit
		{
			if(!data->runningstatus)
				skippedpreviousevent=1;
			return 0;																		//return event ignored
		}

//If this point is reached, the event is not a note on/off for the vocal rhythm difficulty, copy the event into the output file
	WriteVarLength(data->outf,counter);	//Write the delta time

	if( (((data->eventtype>>4)==MIDI_NOTE_ON)||((data->eventtype>>4)==MIDI_NOTE_OFF)) && data->runningstatus && skippedpreviousevent)
	{	//If this is a Note on or off event using running status that depended on the previous event that was ommitted
		if(Lyrics.verbose)	puts("Rebuilding Note On/Off event");
		if(data->lastwritteneventtype != MIDI_NOTE_ON)	//If the last written MIDI event wasn't a Note On
			Write_MIDI_Note(data->parameters[0],data->eventtype&0xF,data->eventtype>>4,data->outf,0);	//Write the note normally as a Note On event
		else
			Write_MIDI_Note(data->parameters[0],data->eventtype&0xF,data->eventtype>>4,data->outf,1);	//Write the note, omitting the status byte (running status)

		data->lastwritteneventtype=MIDI_NOTE_ON;	//Write_MIDI_Note() writes Note On events only
	}
	else
	{	//Copy the event into memory buffer
		fseek_err(data->inf,data->eventindex,SEEK_SET);
		buffer=ReadMetaEventString(data->inf,eventlength);	//Read event into an allocated buffer

		//Write the memory buffer to the ouput file
		if((data->eventtype < 0xF0) && ((data->eventtype>>4) == data->lastwritteneventtype) && !data->runningstatus)
		{	//If this event is not a Meta/SysEx event as is the same as the last such event that was written
			//and this event isn't already running status, convert to running status by omitting event status byte
			if(Lyrics.verbose >= 2)	printf("\tEvent type 0x%X at file position 0x%lX converted to running status\n",data->eventtype>>4,data->eventindex);
			fwrite_err(&(buffer[1]),eventlength-1,1,data->outf);	//Write buffered event (except for the first byte)
		}
		else	//Otherwise write the buffered event normally
			fwrite_err(buffer,eventlength,1,data->outf);

		free(buffer);	//De-allocate memory buffer

		//Store running status information
		if(data->eventtype < 0xF0)	//If it's not a meta or SysEx event
			data->lastwritteneventtype=data->eventtype>>4;	//Store this event type for running status purposes
	}

	skippedpreviousevent=0;	//reset this condition
	counter=0;	//reset delta time counter
	return 1;	//Return event handled
}

void Write_MIDI_Note(unsigned int notenum,unsigned int channelnum,unsigned int notestatus,FILE *outf,char skipstatusbyte)
{
	unsigned char velocity=MIDI_VELOCITY;	//Note on events will be written with this velocity

	assert_wrapper(notenum < 128);
	assert_wrapper((notestatus == MIDI_NOTE_OFF) || (notestatus == MIDI_NOTE_ON));
	assert_wrapper((outf != NULL) && (channelnum < 16));

	if(notestatus == MIDI_NOTE_OFF)	//If this is a Note Off event
	{
		notestatus=MIDI_NOTE_ON;	//Change variables to write it as a Note On
		velocity=0;					//With a velocity of 0 (Note Off, optimized for running status)
	}

	//Write event type MIDI_NOTE_ON, the appropriate channel number and two parameters (note #, velocity)
	if(skipstatusbyte == 0)	//If not specified to skip writing this status byte
		fputc_err((notestatus<<4)|channelnum,outf);

	if(Lyrics.verbose >= 2)	printf("\t\tWriting MIDI Note %d %s%s",notenum,velocity ? "On" : "Off",skipstatusbyte ? " (Running status)\n" : "\n");

	fputc_err(notenum,outf);
	fputc_err(velocity,outf);
}

int SKAR_handler(struct TEPstruct *data)
{
	unsigned char eventtype=0;
	char *buffer=NULL;
	char groupswithnext=0;
	static unsigned long lastlyrictime=0;	//Retains the timestamp in ms of the last Lyric/Text event
	static unsigned char titlecount=0;		//Counts how many @T (Title information) tags have been encountered
											//The first is treated as the song title
											//The second is treated as the song artist

	if(Lyrics.reinit)
	{	//Re-initialize static variables
		Lyrics.reinit=0;
		lastlyrictime=0;
		titlecount=0;
	}

	assert_wrapper(data != NULL);	//This must not be NULL

	eventtype=data->eventtype;//This value may be interpreted differently for Note On w/ velocity=0

	//After ensuring that the track name string is populated,
	//Check to make sure this is within the appropriate vocal track.  If not, return without doing anything
	if(data->trackname == NULL)
		return 0;

	assert_wrapper((data->trackname != NULL) && (Lyrics.inputtrack != NULL));
	if(strcasecmp(data->trackname,Lyrics.inputtrack) != 0)
		return 0;

//Handle Lyric or text event
	if((eventtype==0xFF) && ((data->m_eventtype==0x5) || (data->m_eventtype==0x1)))
	{	//Lyric or Text event in the appropriate vocal track
		if(data->buffer == NULL)			//If the lyric string is NULL, skip it
			return 0;

	//Handle KAR control events
		if(data->buffer[0] == '@')
		{
			if(strlen(data->buffer) < 3)	//A valid KAR control is @, the control identifier and at least one more character
				return 0;

			if(Lyrics.verbose)	printf("\tKAR control event: \"%s\"\n",data->buffer);

			if(toupper(data->buffer[1]) == 'T')	//Title KAR event
			{
				if(titlecount<1)		//First instance, treat as song title
					SetTag(&(data->buffer[2]),'n',0);	//Store Title tag
				else if(titlecount<2)	//Second instance, treat as artist
					SetTag(&(data->buffer[2]),'s',0);	//Store Artist tag

				titlecount++;

				if(titlecount <= 2)	//If the tag was processed
					return 1;
			}

			return 0;
		}

	//Handle new line markers \ and /
		if((data->buffer[0] == '\\') || (data->buffer[0] == '/'))
		{
			if(Lyrics.line_on)	//If there is a line of lyrics in progress
				EndLyricLine();	//Close it

			CreateLyricLine();	//Start a new line
			buffer=DuplicateString(data->buffer+1);	//Copy the rest of the lyric that follows the \ or / character
		}
		else
		{
			if(!Lyrics.line_on)		//If a lyric was given with no line started
			{
				printf("Error: KAR lyric defined (\"%s\") with no line started\nAborting\n",data->buffer);
				exit_wrapper(1);
			}

			buffer=DuplicateString(data->buffer);
		}

		assert_wrapper(Lyrics.curline != NULL);
		lastlyrictime=(unsigned long)ConvertToRealTime(MIDIstruct.deltacounter,MIDIstruct.realtime);	//Get realtime for start of this lyric

//Handle whitespace at the beginning of any parsed lyric piece as a signal that the piece will not group with previous piece
		if(isspace(buffer[0]))
		{
			if(Lyrics.curline->pieces != NULL)	//If there was a previous lyric piece on this line
			{
				assert_wrapper(Lyrics.lastpiece != NULL);
				Lyrics.lastpiece->groupswithnext=0;	//Ensure it is set to not group with this lyric piece
				Lyrics.lastpiece->duration=1;		//Ensure it has a generic duration of 1ms
			}
		}
		else
		{	//If there was a previous lyric in this line
			if(Lyrics.curline->piececount > 0)
			{
				assert_wrapper(Lyrics.lastpiece != NULL);
				if(Lyrics.lastpiece->groupswithnext)	//If this piece groups with the previous piece
					Lyrics.lastpiece->duration=lastlyrictime-Lyrics.realoffset-Lyrics.lastpiece->start;	//Extend previous piece's length to reach this piece, take the current offset into account
			}
		}

		if(isspace(buffer[strlen(buffer)-1]))	//If the lyric ends in a space
			groupswithnext=0;
		else
			groupswithnext=1;

		AddLyricPiece(buffer,lastlyrictime,lastlyrictime+1,PITCHLESS,groupswithnext);	//Add lyric with no defined pitch and generic duration of 1ms

		if(Lyrics.verbose)	printf("\tAdded lyric piece \"%s\"\n",buffer);

		free(buffer);
		return 1;
	}//Lyric or Text event in the appropriate vocal track

	return 0;
}

int Lyric_handler(struct TEPstruct *data)
{	//This is the new KAR_pitch_handler logic that is designed to handle both RB and KAR MIDIs
	static char groupswithnext=0;			//Grouping status for KAR import logic, configured when a Lyric/Text event is read
	double time=0.0;
	unsigned char eventtype=0;
	struct MIDI_Lyric_Piece *Lyric_Piece=NULL;	//Used to retrieve from the Notes list

	static unsigned char lyric_note_num=0xFF;
		//Some incorrectly-prepared RB ripped MIDIs nest note events within the Note On and Note Off event that
		//pertain to a lyric's starting and ending.  This static variable will be used to keep track, between
		//calls to Lyric_handler, of which MIDI note number corresponds to the next lyric.  0xFF is not a valid
		//MIDI number, so it will be the value of this variable if a new Note # is being expected
	static char end_line_after_piece=0;
		//KAR files often have a lyric, another lyric of "\r" and then the Note events for the first lyric, so
		//the boolean status of there having been a line break needs to be maintained until the lyric is added to list
	static unsigned long lastlyrictime=0;
		//Formerly in the TEPstruct, this is being used locally to store the starting timestamp of the Lyric event being processed
	static unsigned long lastnotetime=0;
		//Used to store the real time positions of the most recent Note On event (for lyricless logic)
	static char *lastlyric=NULL;
		//Formerly in the TEPstruct, this is being used locally to store the text of a parsed lyric that hasn't been added to the Notes list yet
	static char overdrive_on=0;
		//Used to track whether overdrive is enabled (For RB MIDI import), based on Note On/Off #116

	if(Lyrics.reinit)
	{	//Re-initialize static variables
		Lyrics.reinit=0;
		groupswithnext=0;
		lyric_note_num=0xFF;
		end_line_after_piece=0;
		lastlyrictime=0;
		lastnotetime=0;
		lastlyric=NULL;
		overdrive_on=0;
	}

	assert_wrapper(data != NULL);	//This must not be NULL
	assert_wrapper(Lyrics.inputtrack != NULL);

	eventtype=data->eventtype;//This value may be interpreted differently for Note On w/ velocity=0

	//After ensuring that the track name string is populated,
	//Check to make sure this is within the appropriate vocal track.  If not, return without doing anything
	if(data->trackname == NULL)
		return 0;
	if(strcasecmp(data->trackname,Lyrics.inputtrack) != 0)
		return 0;

//Special case:  A Note On event with a velocity of 0 must be treated as a Note Off event
	if(((data->eventtype>>4) == 0x9) && (data->parameters[1] == 0))
		eventtype=0x80;	//Treat data->eventtype into Note Off, leave it as Note On in the original TEPstruct

//Handle Lyric/Text event
	if((eventtype==0xFF) && ((data->m_eventtype==0x5) || (data->m_eventtype==0x1)))
	{	//Lyric or Text event in the appropriate vocal track
		if(data->buffer == NULL)			//If the lyric string is NULL, skip it
			return 0;

		if(Lyrics.nolyrics)	//If lyricless logic is in effect
		{
			if(Lyrics.verbose)	printf("Lyricless logic in effect.  Ignoring lyric \"%s\".\n",data->buffer);
			return 0;	//Ignore and return
		}

	//Perform RB MIDI specific Lyric/Text logic
		if(Lyrics.in_format == MIDI_FORMAT)
		{
			if((data->buffer)[0] == '[')	//If the text/lyric event begins with bracket
				return 0;					//Ignore it and return
		}
	//Perform KAR specific Lyric/Text logic
		else
		{
			//Handle presence of "\r" or "\n" as a line break
				if(strchr(data->buffer,'\r') || strchr(data->buffer,'\n'))
				{
					if(Lyrics.line_on  || (lastlyric != NULL))		//Only end a line if one is open or a Lyric is in progress
					{
						if(lastlyric != NULL)		//If there is an unfinished Lyric start
							end_line_after_piece=1;		//This line break will come after the unfinished Lyric
						else
						{
							if(MIDI_Lyrics.head != NULL)		//If there are unfinished lyrics
								AddMIDILyric(NULL,0,0xFF,0,0);	//Append a line break to the list
							else								//Otherwise, the line ends normally
								EndLyricLine();
						}

						return 1;
					}

					return 0;
				}
		}//if(Lyrics.in_format == MIDI_FORMAT)

	//Ensure that a new lyric event is expected (previous lyric piece has been parsed or added to the Notes list)
		if(lastlyric != NULL)
		{	//A new lyric event was encountered without the last lyric piece having been ended with Note Off
			//Special case is that a preceeding Note On was just accepted for this lyric event above, allow it
			AddMIDILyric(lastlyric,lastlyrictime,PITCHLESS,overdrive_on,groupswithnext);	//Add lyric of style "pitchless" to the MIDI lyric list
			EndMIDILyric(PITCHLESS,lastlyrictime+1);										//End the pitchless lyric with a duration of 1ms
			while(GetLyricPiece());	//Remove and store all completed MIDI lyrics from the front of the list
			lastlyric=DuplicateString(data->buffer);	//Forget lastlyric string, allocate a new one
			MIDIstruct.unfinalizedlyric=lastlyric;		//Store this to handle the special case of the last lyric being pitchless
			lastlyrictime=(unsigned long)ConvertToRealTime(MIDIstruct.deltacounter,MIDIstruct.realtime);
			MIDIstruct.unfinalizedlyrictime=lastlyrictime;	//Store this to handle the special case of the last lyric being pitchless
			return 1;
		}

	//Perform KAR specific grouping logic for this new lyric
		if(Lyrics.in_format == KAR_FORMAT)
		{	//Handle whitespace at the beginning of the parsed lyric piece as a signal that the piece will not group with previous piece
			if(isspace((Lyric_Piece->lyric)[0]))
				if(Lyrics.curline->pieces != NULL)	//If there was a previous lyric piece on this line
					Lyrics.lastpiece->groupswithnext=0;	//Ensure it is set to not group with this lyric piece

			if(isspace((Lyric_Piece->lyric)[strlen(Lyric_Piece->lyric)-1]))	//If the lyric ends in a space
				groupswithnext=0;
			else
				groupswithnext=1;
		}

		lastlyric=DuplicateString(data->buffer);	//Forget lastlyric string, allocate a new one

	//Configure variables to track this event
		lastlyrictime=(unsigned long)ConvertToRealTime(MIDIstruct.deltacounter,MIDIstruct.realtime);

		MIDIstruct.unfinalizedlyric=lastlyric;					//Store this to handle the special case of the last lyric being pitchless
		MIDIstruct.unfinalizedlyrictime=lastlyrictime;			//Store this to handle the special case of the last lyric being pitchless
		MIDIstruct.unfinalizedoverdrive_on=overdrive_on;		//Store this to handle the special case of the last lyric being pitchless
		MIDIstruct.unfinalizedgroupswithnext=groupswithnext;	//Store this to handle the special case of the last lyric being pitchless

//Special case:  The lyric piece was defined by having the Note On event before its Lyric event.
		if(lyric_note_num != 0xFF)
		{	//If a Note on preceeded this Lyric event, perform following logic and add Lyric to Notes list at end of this function
			if(data->delta == 0)	//and this Lyric event has a delta of 0
				printf("Warning: Lyric defined improperly (Lyric defined after pitch %u) in source MIDI.  Correcting..\n",lyric_note_num);
			else	//A Note On that does not match the timing of an adjacent Lyric event is ignored
			{
				printf("Warning: Extraneous Note (%u) On event discarded\n",lyric_note_num);
				lyric_note_num = 0xFF;	//Expecting a new Note on event
			}
		}
		else	//Still expecting a Note On
			return 1;
	}

//Handle Note On event
	if((eventtype>>4) == 0x9)
	{
	//Perform RB MIDI specific Note on logic
		if(Lyrics.in_format == MIDI_FORMAT)
		{
			if((data->parameters[0] == 105) || (data->parameters[0] == 106))	//Note on event to mark a new line of lyrics
			{
				if(Lyrics.line_on == 0)		//Only create a new line if one isn't already open
					CreateLyricLine();
				return 1;
			}
			else if(data->parameters[0] == 116)	//Note on event to mark Overdrive
			{
				overdrive_on=1;	//Track this locally to ensure that it is correctly applied to lyrics in the Notes list
				return 1;
			}
		}

		if(lyric_note_num != 0xFF)	//If a Note on was already encountered before reaching a Lyric event
		{
			if(Lyrics.verbose)
				puts("Warning: Note nested improperly in source MIDI.  Ignoring..");

			return 0;
		}

		if(Lyrics.nolyrics)	//If lyricless logic is in effect, enforce the disabling of chords
			if(MIDI_Lyrics.head != NULL)	//If a note is already populated in the MIDI lyric list
			{
				puts("!\tWarning: Chords encountered in imported vocal track, omitting all notes except the first in the chord");
				return 0;	//Ignore this note, it isn't valid
			}

		lastnotetime=(unsigned long)ConvertToRealTime(MIDIstruct.deltacounter,MIDIstruct.realtime);

		lyric_note_num=data->parameters[0];	//Store Note number
		if(lastlyric != NULL)
		{	//If a Lyric was parsed and a corresponding Note # was expected, perform following logic and add Lyric to Notes list at end of this function
			if(data->delta != 0)
				printf("\a Warning: Note on for a lyric piece has a delta != 0 (Delta time for the event is at file position %lX)\n",data->startindex);
		}
		else	//If no lyric was parsed yet, return depending on whether lyricless logic is in effect
		{
			if(Lyrics.nolyrics == 0)	//If the nolyrics parameter wasn't specified
				return 1;				//return, as the lyric is expected to be parsed
		}
	}//Note On event

//Handle Note Off event
	if((eventtype>>4) == 0x8)
	{	//If note off in the vocal track
		time=ConvertToRealTime(MIDIstruct.deltacounter,MIDIstruct.realtime);	//End offset of lyric piece in realtime

	//Perform RB Midi specific Note off logic
		if(Lyrics.in_format == MIDI_FORMAT)
		{
			if((data->parameters[0] == 105) || (data->parameters[0] == 106))	//Note off event to mark end of line of lyrics
			{
				if(Lyrics.line_on  || (lastlyric != NULL))		//Only end a line if one is open or a Lyric is in progress
				{
					if(lastlyric != NULL)		//If there is an unfinished Lyric start
						end_line_after_piece=1;		//This line break will come after the unfinished Lyric
					else
					{
						if(MIDI_Lyrics.head != NULL)		//If there are unfinished lyrics
							AddMIDILyric(NULL,0,0xFF,0,0);	//Append a line break to the list
						else								//Otherwise, the line ends normally
							EndLyricLine();
					}

					return 1;
				}

				return 0;
			}
			else if(data->parameters[0] == 116)	//Note off event to mark end of Overdrive
			{
				overdrive_on=0;	//Track this locally to ensure that it is correctly applied to lyrics in the Notes list
				return 1;
			}
		}

		Lyric_Piece=EndMIDILyric(data->parameters[0],time);			//Append timestamp to the appropriate Lyric entry, remove and return it if the first entry is complete

		if(Lyric_Piece == NULL)	//If the first Lyric in the list was not completed
		{
			if(data->parameters[0] == lyric_note_num)	//If this Note Off matches a Note on that has no lyric yet
				lyric_note_num=0xFF;					//Reset this status

			return 0;			//ignore the event
		}

		if(Lyric_Piece->lyric == NULL)
		{
			puts("Error: Unexpected NULL string\nAborting");
			exit_wrapper(3);
		}

		assert_wrapper((Lyric_Piece->pitch == PITCHLESS) || (Lyric_Piece->pitch < 128));	//Valid MIDI notes are 0-127, or PITCHLESS

		while(GetLyricPiece());	//Remove and store all completed MIDI lyrics from the front of the list

  		return 1;
	}//Note Off event

//Lyricless logic
	if(Lyrics.nolyrics && (lyric_note_num != 0xFF))
	{	//If a Note On event was parsed, and lyricless logic is being performed
		lastlyric=DuplicateString("*");
		AddMIDILyric(lastlyric,lastnotetime,lyric_note_num,overdrive_on,0);
		MIDIstruct.unfinalizedlyric=NULL;

		lastlyric=NULL;	//Reset this status
		lyric_note_num=0xFF;	//Reset this status

		return 1;
	}

//Normal lyric logic
	if((lastlyric != NULL) && (lyric_note_num != 0xFF))
	{	//If a Lyric and Note On with the same timing have been parsed
		AddMIDILyric(lastlyric,lastlyrictime,lyric_note_num,overdrive_on,groupswithnext);
		MIDIstruct.unfinalizedlyric=NULL;

		if(end_line_after_piece)
		{
			AddMIDILyric(NULL,0,0xFF,0,0);	//Append a line break to the list
			end_line_after_piece=0;			//Reset this condition
		}

		lastlyric=NULL;	//Reset this status
		lyric_note_num=0xFF;	//Reset this status

		return 1;
	}

	return 0;	//Nothing was done
}

void AddMIDILyric(char *str,unsigned long start,unsigned char pitch,char isoverdrive,char groupswithnext)
{	//Adds the Lyric and Note On information to the linked list
	struct MIDI_Lyric_Piece *temp=NULL;

	if((MIDI_Lyrics.head != NULL) && (MIDI_Lyrics.tail != NULL) && (MIDI_Lyrics.tail->lyric == NULL))
		return;		//If the last item appended was a line break, return instead of appending another consecutive line break

	temp=(struct MIDI_Lyric_Piece *)malloc_err(sizeof (struct MIDI_Lyric_Piece));
	temp->lyric=str;	//If str is NULL, it will be handled like a line break
	temp->start=start;
	temp->end=0;		//Set to 0 until the appropriate Note Off is found
	temp->next=NULL;	//Point new link forward to nothing
	temp->groupswithnext=groupswithnext;
	temp->overdrive_on=isoverdrive;

	if(str == NULL)			//If a line break is being added
		temp->pitch=0xFF;	//Ensure an invalid pitch is written
	else
		temp->pitch=pitch;

//Insert into linked list
	if(MIDI_Lyrics.head == NULL)
	{	//List is empty
		MIDI_Lyrics.head=temp;	//Point head to new link
		temp->prev=NULL;		//Point new link back to nothing
	}
	else
	{
		temp->prev=MIDI_Lyrics.tail;	//Point new link back to last link
		assert_wrapper(MIDI_Lyrics.tail != NULL);
		MIDI_Lyrics.tail->next=temp;	//Point last link forward to new link
	}

	MIDI_Lyrics.tail=temp;		//Point tail to new link

	if(Lyrics.verbose>=2)
	{
		if(str == NULL)
			puts("\tAppending line break to Notes list");
		else
		{
			printf("\tAppending lyric \"%s\" with ",str);
			if(pitch == PITCHLESS)
				printf("NO pitch");
			else
				printf("pitch %u",pitch);

			if(Lyrics.in_format == KAR_FORMAT)
			{	//groupswithnext is only set for KAR import
				printf("\tKAR grouping: ");
				if(groupswithnext)
					printf("YES");
				else
					printf("NO");
			}
			putchar('\n');
		}
	}
}

struct MIDI_Lyric_Piece *EndMIDILyric(unsigned char pitch,unsigned long end)
{	//Searches for the first incomplete (missing end timestamp) MIDI Lyric list entry with a matching pitch and writes the ending timestamp
	//The address of the matching Lyric is returned by reference, otherwise NULL is returned
	//This allows an indefinite number of Lyrics to be appended and when they are ended, they will be returned in the proper order (First In, First Out)
	struct MIDI_Lyric_Piece *temp=NULL;

//Search the list for the FIRST instance of the Note number
	temp=MIDI_Lyrics.head;
	while(temp != NULL)
	{
		if((temp->pitch == pitch) && (temp->end == 0))	//If this is an incomplete entry with a matching pitch
			break;
		else
			temp=temp->next;
	}

	if(temp == NULL)
		return NULL;	//No matching note number found, ignore

//Append ending timestamp
	temp->end=end;
	if(Lyrics.verbose>=2)
	{
		printf("\tEnding lyric \"%s\" with ",temp->lyric);
		if(temp->pitch == PITCHLESS)
			puts("NO pitch");
		else
			printf("pitch %u\n",temp->pitch);
	}

	return temp;
}

int GetLyricPiece(void)
{
	struct MIDI_Lyric_Piece *temp=MIDI_Lyrics.head;

	if((temp != NULL) && (temp->end != 0))
	{	//If first link exists and is completed
		assert_wrapper(temp->prev == NULL);	//If the head link doesn't point back to nothing, abort program
		MIDI_Lyrics.head=temp->next;

		if(temp->next == NULL)	//Removing only link in list
			MIDI_Lyrics.tail=NULL;
		else
			(temp->next)->prev=NULL;

		temp->prev=temp->next=NULL;

//Ensure a line of lyrics is open
		if(Lyrics.line_on == 0)		//Only create a new line if one isn't already open
			CreateLyricLine();

//Special case: The first entry of Notes list is now a line break
		if((MIDI_Lyrics.head != NULL) && (MIDI_Lyrics.head->lyric == NULL))
		{
			EndLyricLine();		//End the line gracefully

		//Remove from the notes list
			temp=MIDI_Lyrics.head;	//Store this pointer
			MIDI_Lyrics.head=temp->next;
			if(temp->next == NULL)	//Remove tail of list
				MIDI_Lyrics.tail=NULL;
			else
				(temp->next)->prev=NULL;

			free(temp);	//Deallocate memory used by the MIDI_Lyric piece
			return 2;	//Return line break stored
		}

//Configure overdrive based on the retrieved Lyric's overdrive status
		if(temp->overdrive_on)
			Lyrics.overdrive_on=1;
		else
			Lyrics.overdrive_on=0;

		AddLyricPiece(temp->lyric,temp->start,temp->end,temp->pitch,temp->groupswithnext);

//Track for pitch changes, enabling Lyrics.pitch_tracking if applicable
		if((Lyrics.last_pitch != 0) && (Lyrics.last_pitch != temp->pitch))	//There's a pitch change
			Lyrics.pitch_tracking=1;
		Lyrics.last_pitch=temp->pitch;	//Consider this the last defined pitch

//Reset variables
		free(temp->lyric);
		temp->lyric=NULL;	//This is expected to be NULL on the next Lyric event
		free(temp);			//Deallocate memory used by the MIDI_Lyric piece
	}
	else
		return 0;	//No MIDI lyric stored

	return 1;	//Return MIDI lyric stored
}

char *AnalyzeVrhythmID(const char *buffer)
{
	char *trackname=NULL;	//This is the target input/output MIDI track name that will be returned

	if(buffer == NULL)
		return NULL;

	switch(tolower(buffer[0]))
	{	//Identify the specified track name
		case 'g':	//PART GUITAR
			trackname=DuplicateString("PART GUITAR");
		break;

		case 'b':	//PART BASS
			trackname=DuplicateString("PART BASS");
		break;

		case 'c':	//PART GUITAR COOP
			trackname=DuplicateString("PART GUITAR COOP");
		break;

		case 'r':	//PART RHYTHM
			trackname=DuplicateString("PART RHYTHM");
		break;

		case 'd':	//PART DRUMS	(account for the possibility of the track being called "PART DRUM"
			trackname=DuplicateString("PART DRUMS");
		break;

		default:
			return NULL;
		break;
	}

	assert_wrapper(trackname != NULL);

	switch(buffer[1])
	{	//Store the range of notes that represent this difficulty
		case '1':	//Supaeasy (notes 60-64)
			MIDIstruct.diff_lo=60;
			MIDIstruct.diff_hi=64;
		break;

		case '2':	//Easy (notes 72-76)
			MIDIstruct.diff_lo=72;
			MIDIstruct.diff_hi=76;
		break;

		case '3':	//Medium (notes 84-88)
			MIDIstruct.diff_lo=84;
			MIDIstruct.diff_hi=88;
		break;

		case '4':	//Amazing (notes 96-100)
			MIDIstruct.diff_lo=96;
			MIDIstruct.diff_hi=100;
		break;

		default:
			free(trackname);
			return NULL;
		break;
	}

	return trackname;
}

long int Write_MIDI_Track_Header(FILE *outf)
{
	long int fp=0;
	char buffer[4]={0};		//Null Data to write as the chunk size

	assert_wrapper(outf != NULL);

	fputc_err('M',outf);
	fputc_err('T',outf);
	fputc_err('r',outf);
	fputc_err('k',outf);
	fp=ftell_err(outf);				//We'll calculate the chunk size after writing the events
	fwrite_err(buffer,4,1,outf);	//write dummy information for the chunk size, we'll correct it later

	return fp;
}

void Export_Vrhythm(FILE *outmidi,FILE *outlyric,char *vrhythmid)
{
	long int chunkfileposition=0,chunkfilesize=0,fp=0;
	struct Lyric_Line *curline=NULL;	//A conductor for the lyric lines list
	struct Lyric_Piece *temp=NULL;		//A conductor for the lyric pieces list
	char line_marked=0;			//Boolean:  The current line of lyrics has been marked with a Note 105 on event
	unsigned long reldelta=0;	//The relative delta compared to this delta time and the last delta time
								//Stores a relative delta for a start of a lyric piece
	unsigned long thisdelta=0;	//The absolute delta time of the current lyric/line start
	unsigned long lastdelta=0;	//We have to store the last absolute delta of the end of a lyric so we can keep
								//track of relative delta times between events
	unsigned long lyrdelta=0;	//The relative delta value to use for a lyric piece.  If a new line is started, this is 0
	unsigned int channelnum=1;	//This is the channel number to write events to (channel 1 for track 1)
	char isfirstinline=0;		//Boolean:  The lyric is the first one that occurs in the current line of lyrics (will be written as fret 2)
	int errornumber=0;
	char *notestring=NULL;		//Used to store the note name if the notenames feature is being used
	char runningstatus=0;		//Used for running status:  Since all non Meta/SysEx events written in this function are
								//Note On events, this boolean can be passed to Write_MIDI_Note() and set after the first call,
								//implementing running status

	assert_wrapper((outmidi != NULL) && (outlyric != NULL) && (vrhythmid != NULL));	//These must not be NULL
	assert_wrapper(Lyrics.outputtrack != NULL);		//This is now set at the beginning of the source MIDI logic
	assert_wrapper(Lyrics.piececount != 0);	//This function is not to be called with an empty Lyrics structure

	//Write instrument/difficulty identifier
	if(fprintf(outlyric,"midi = %s\n",vrhythmid) < 0)
	{
		printf("Error writing to output lyric file: %s\nAborting\n",strerror(errno));
		exit_wrapper(2);
	}

	if(Lyrics.verbose)		printf("\tTrack %u (\"%s\"): Building...\n",MIDIstruct.trackswritten,Lyrics.outputtrack);

//Write header for vrhythm track
	chunkfileposition=Write_MIDI_Track_Header(outmidi);

//Write track name
	WriteMIDIString(outmidi,0,TRACK_NAME_MIDI_STRING,Lyrics.outputtrack);

//Before writing the abosolute first lyric event, embed the program version
	WriteMIDIString(outmidi,0,SEQSPEC_MIDI_STRING,PROGVERSION);

//Write Note On and Off events for each Lyric piece
	curline=Lyrics.lines;	//Point conductor to first line of lyrics

	//Special case:  The first lyric has overdrive, insert a line break so that an overdrive phrase can be defined
	if(curline->pieces->overdrive)	//If the lyric is in overdrive
		fputs_err("-*\n",outlyric);	//Write a line break, denoting overdrive

	while(curline != NULL)
	{	//For each line of lyrics
		line_marked=0;		//The first lyric in the line will mark the start of the line as well
		isfirstinline=1;		//The next lyric will be written as fret 2, as it's the first in this line of lyrics
		temp=curline->pieces;	//Starting with the first piece of lyric in this line
		while(temp != NULL)
		{	//For each piece of lyric
//Perform MIDI logic
			thisdelta=ConvertToDeltaTime(temp->start);//Delta is the start absolute delta time of this lyric
			reldelta=thisdelta-lastdelta;	//Find the delta value relative to the last lyric event
			lyrdelta=reldelta;	//Unless we find a new line is starting this is the lyric's delta value

			if(line_marked == 0)	//If we need to mark the start of the line of lyrics
			{
				lyrdelta=0;	//The lyric's delta will be 0, following the line start event

	//Write Note 105 On event (line start)
			//Write delta value of this start of line (equal to the delta of the first lyric in a line)
				WriteVarLength(outmidi,reldelta);
				Write_MIDI_Note(105,channelnum,MIDI_NOTE_ON,outmidi,runningstatus);
				runningstatus=1;	//Any note written after the first can be running status
				line_marked=1;	//This will remain 1 until the start of the next line
			}

	//Write vocal rhythm note on
			WriteVarLength(outmidi,lyrdelta);	//Write delta time
			Write_MIDI_Note(MIDIstruct.diff_lo+isfirstinline,channelnum,MIDI_NOTE_ON,outmidi,runningstatus);	//Write as fret 1 note (or fret 2 if it's the first lyric in the line)
			runningstatus=1;	//Any note written after the first can be running status

			if(Lyrics.verbose)
				printf("\t\tWriting vrhythm note for lyric: '%s'\tRealtime: %lums (written as %fms)\tDuration: %lu\n",temp->lyric,temp->start-Lyrics.realoffset,ConvertToRealTime(thisdelta,0.0),temp->duration);

	//Write vocal rhythm note off the correct number of deltas away from the note on event
			lastdelta=ConvertToDeltaTime(temp->start+temp->duration);	//lastdelta=abs delta of end of lyric
			if(lastdelta < thisdelta)
			{
				printf("Unexpected error: temp->start=%lu\ttemp->duration=%lu\tduration delta=%lu\nAborting\n",temp->start,temp->duration,lastdelta);
				exit_wrapper(1);
			}

			WriteVarLength(outmidi,lastdelta-thisdelta);	//Write delta time
			Write_MIDI_Note(MIDIstruct.diff_lo+isfirstinline,channelnum,MIDI_NOTE_OFF,outmidi,runningstatus);	//Write fret 1 note off (or fret 2 if it's the first lyric in the line)
			runningstatus=1;	//Any note written after the first can be running status

//Perform pitched lyric logic
			if((Lyrics.pitch_tracking == 0) || (temp->pitch == PITCHLESS) || (temp->freestyle))	//If there was no pitch variation among all lyrics, this lyric has no defined pitch or the lyric is freestyle
				fputs_err("# ",outlyric);	//Write freestyle pitch indicator
			else
			{
				if(Lyrics.notenames)		//If user requested to have the notes written by name
				{
					notestring=ConvertNoteNum(temp->pitch);	//Create a string containing the note's name
					if(fprintf(outlyric,"%s ",notestring)<0)
						errornumber=errno;
					free(notestring);
					notestring=NULL;
				}
				else						//Write note number instead
					if(fprintf(outlyric,"%u ",temp->pitch)<0)	//Write pitch
						errornumber=errno;
			}

			if(fprintf(outlyric,"%s\n",temp->lyric)<0)		//Write lyric
				errornumber=errno;

			if(errornumber)
			{
				printf("Error writing to pitched lyric file: %s\nAborting\n",strerror(errornumber));
				exit_wrapper(3);
			}

			temp=temp->next;	//Advance to next lyric piece in this line
			isfirstinline=0;	//All other lyrics in this line are not the first
		}//while(temp != NULL)

	//Write Note 105 Off event (line end)
	//Write delta value 0 (line ends 0 deltas away from the Note Off event for the last lyric)
		WriteVarLength(outmidi,0);
		Write_MIDI_Note(105,channelnum,MIDI_NOTE_OFF,outmidi,runningstatus);
		runningstatus=1;	//Any note written after the first can be running status

//Perform pitched lyric line break logic
		if(curline->next != NULL)	//If there's another line of lyrics
		{
			assert_wrapper(curline->next->pieces != NULL);
			if(curline->next->pieces->overdrive)	//If the first lyric of the next line is overdrive
				fputs_err("-*\n",outlyric);	//Write line break with overdrive indicator
			else
				fputs_err("-\n",outlyric);	//Write line break without overdrive indicator
		}

		curline=curline->next;	//Advance to next line of lyrics
	}//while(curline != NULL)

	//Write end of track event with a delta of 0 and a null padding byte
	WriteDWORDBE(outmidi,0x00FF2F00UL);	//Write 4 bytes: 0, 0xFF, 0x2F and 0

	//Write the correct track chunk size in track 1's header
	fp=ftell_err(outmidi);									//Save the current file position
	chunkfilesize=fp-chunkfileposition-4;
	fseek_err(outmidi,chunkfileposition,SEEK_SET);			//Rewind to where the chunk size is supposed to be written

	WriteDWORDBE(outmidi,chunkfilesize);	//Write track chunk size
	fseek_err(outmidi,fp,SEEK_SET);			//Return to original file position

	MIDIstruct.trackswritten++;			//Increment the counter to reflect the vocal rhythm rack having been written
	if(Lyrics.verbose)	printf("\nVrhythm export complete.  %lu lyrics written\n",Lyrics.piececount);
}

void Write_Default_Track_Zero(FILE *outmidi)
{	//Writes a MIDI file header specifying 1 track, writes a track for track 0 and sets MIDIstruct.hchunk.division and MIDIstruct.hchunk.tempomap
	long int chunkfileposition=0,chunkfilesize=0,fp=0;
	struct Tempo_change *tempo=NULL;	//Used to write a custom tempo if one was provided

	assert_wrapper(outmidi != NULL);

	if(Lyrics.verbose)	puts("No source MIDI was provided.  Creating MIDI track 0");

	//Write MIDI header
	fputc_err('M',outmidi);
	fputc_err('T',outmidi);
	fputc_err('h',outmidi);
	fputc_err('d',outmidi);

	WriteDWORDBE(outmidi,6);	//Write header chunk size
	WriteWORDBE(outmidi,1);		//Write MIDI format type
	WriteWORDBE(outmidi,1);		//Write # of tracks being written by this function (1)
	MIDIstruct.trackswritten=1;
	WriteWORDBE(outmidi,0x1E0);	//Write this as the time division, as it is commonly used in Rock Band MIDIs

	MIDIstruct.hchunk.division=0x1E0;			//Store the time division in the MIDI structure for use by the delta<->realtime functions

	chunkfileposition=Write_MIDI_Track_Header(outmidi);	//Write header for track 0

	fputc_err(0,outmidi);					//Write a delta time of 0

	if(Lyrics.explicittempo < 2.0)			//If no tempo was manually provided via command line
	{
		Write_Tempo_Change(120.0,outmidi);			//Write a tempo change of 120BPM
		MIDIstruct.hchunk.tempomap=&DEFAULTTEMPO;	//Store a default tempo of 120BPM
	}
	else
	{	//Create and intialize a tempo change structure
		tempo=(struct Tempo_change *)calloc_err(1,sizeof(struct Tempo_change));	//Allocate and init to 0
		tempo->BPM=Lyrics.explicittempo;
		//Write tempo change to MIDI and store in tempo map
		Write_Tempo_Change(Lyrics.explicittempo,outmidi);
		MIDIstruct.hchunk.tempomap=tempo;
	}

	//Write end of track event with a delta of 0 and a null padding byte
	WriteDWORDBE(outmidi,0x00FF2F00UL);	//Write 4 bytes: 0, 0xFF, 0x2F and 0

	//Write track chunk size for track 0
	fp=ftell_err(outmidi);									//Save the current file position
	chunkfilesize=fp-chunkfileposition-4;
	fseek_err(outmidi,chunkfileposition,SEEK_SET);			//Rewind to where the chunk size is supposed to be written

	WriteDWORDBE(outmidi,chunkfilesize);	//Write track chunk size
	fseek_err(outmidi,fp,SEEK_SET);			//Return to original file position
}

void Write_Tempo_Change(double BPM,FILE *outf)
{	//Writes Meta event 0x51, specifying the MPQN (milliseconds per quarter note), where MPQN = MICROSECONDS_PER_MINUTE/BPM = 60000000/BPM
	unsigned long mpqn=0;

	assert_wrapper(outf != NULL);
	assert_wrapper(BPM > 0);	//BPM that is zero or negative is not valid

	if(Lyrics.verbose)	printf("\t\tWriting tempo change of %f\n",BPM);

	WriteWORDBE(outf,0xFF51);	//Write meta event type 0x51
	fputc_err(3,outf);			//Write length field specifying 3 bytes

	mpqn=(unsigned long)((60000000.0/BPM)+0.5);	//Calculate MPQN, rounding up to the nearest whole number

	//Write MPQN as a 3 byte value in big endian format
	fputc_err((mpqn >> 16) & 0xFF,outf);	//Write the most significant byte first
	fputc_err((mpqn >> 8) & 0xFF,outf);		//Write the next most significant byte
	fputc_err(mpqn & 0xFF,outf);			//Write the least significant byte last
}

void Export_SKAR(FILE *outf)
{	//"Words" is written with lyric and note events based on the contents of the Lyrics structure
	unsigned long reldelta=0;	//The relative delta compared to this delta time and the last delta time
					//Stores a relative delta for a start of a lyric piece
	unsigned long thisdelta=0;	//The absolute delta time of the current lyric/line start
	unsigned long lastdelta=0;	//We have to store the last absolute delta of the start of a lyric so we can keep
					//track of relative delta times between events
	struct Lyric_Line *curline=NULL;	//A conductor for the lyric lines list
	struct Lyric_Piece *temp=NULL;		//A conductor for the lyric pieces list
	long chunkfileposition=0;	//store this so we can write the track chunk size at the end
	long tempfileposition=0;	//store this so we can return to the end of the first track that is written

	long chunkfilesize=0;
	char *tempstr=NULL;

	assert_wrapper(outf != NULL);			//This must not be NULL (inf is allowed to in order to control how the output file is written)
	assert_wrapper(Lyrics.piececount != 0);	//This function is not to be called with an empty Lyrics structure

	if(Lyrics.verbose)
	{
		printf("\nExporting SKAR lyrics to file \"%s\"\n",Lyrics.outfilename);
		printf("\tTrack %u (\"Soft Karaoke\"): Building...\n",MIDIstruct.trackswritten);
	}

//Write Track 1 (Soft Karaoke)
 //Write the track header
	chunkfileposition=Write_MIDI_Track_Header(outf);

 //Write track name with a delta of 0
	WriteMIDIString(outf,0,TRACK_NAME_MIDI_STRING,"Soft Karaoke");

 //Write Soft Karaoke tags
	WriteMIDIString(outf,0,TEXT_MIDI_STRING,"@KMIDI KARAOKE FILE");
	WriteMIDIString(outf,0,TEXT_MIDI_STRING,"@V0100");
	tempstr=Append("@I ",PROGVERSION);
	WriteMIDIString(outf,0,TEXT_MIDI_STRING,tempstr);		//Write FoFLyricConvert version as informational tag
	free(tempstr);
	if(Lyrics.Editor != NULL)
	{
		tempstr=Append("@I lyrics by ",Lyrics.Editor);
		WriteMIDIString(outf,0,TEXT_MIDI_STRING,tempstr);	//Write editor as informational tag
		free(tempstr);
	}

 //Write end of track event with a delta of 0 and a null padding byte
	WriteDWORDBE(outf,0x00FF2F00UL);	//Write 4 bytes: 0, 0xFF, 0x2F and 0
	tempfileposition=ftell_err(outf);		//Save the current file position
//This call to ftell can be eliminated since it is made above
//	chunkfilesize=ftell_err(outf)-chunkfileposition-4;	//# of bytes from start to end of the track chunk (- chunk size variable)
	chunkfilesize=tempfileposition-chunkfileposition-4;	//# of bytes from start to end of the track chunk (- chunk size variable)

 //Rewind to the track chunk size location and write the correct value
	fseek_err(outf,chunkfileposition,SEEK_SET);	//Rewind to where the chunk size is supposed to be written
	WriteDWORDBE(outf,chunkfilesize);		//Write chunk size in BE format
	fseek_err(outf,tempfileposition,SEEK_SET);	//Return to end of the "Soft Karaoke" track
	MIDIstruct.trackswritten++;			//One additional track has been written to the output file

	if(Lyrics.verbose)	printf("\tTrack %u (\"Words\"): Building...\n",MIDIstruct.trackswritten);

//Write Track 2 (Words)
 //Write the track header
	chunkfileposition=Write_MIDI_Track_Header(outf);

 //Write track name with a delta of 0
	WriteMIDIString(outf,0,TRACK_NAME_MIDI_STRING,"Words");

 //Write Soft Karaoke tags
	WriteMIDIString(outf,0,TEXT_MIDI_STRING,"@LENGL");		//Write the language tag as English
	if(Lyrics.Title != NULL)
	{
		tempstr=Append("@T",Lyrics.Title);	//Write song title tag
		WriteMIDIString(outf,0,TEXT_MIDI_STRING,tempstr);
		free(tempstr);
	}
	if(Lyrics.Artist != NULL)
	{
		tempstr=Append("@T",Lyrics.Artist);	//Write song artist tag
		WriteMIDIString(outf,0,TEXT_MIDI_STRING,tempstr);
		free(tempstr);
	}

 //Write the lyrics
	curline=Lyrics.lines;	//Point conductor to first line of lyrics
	while(curline != NULL)
	{	//For each line of lyrics
		temp=curline->pieces;	//Starting with the first piece of lyric in this line
		while(temp != NULL)
		{	//For each piece of lyric in the line
			thisdelta=ConvertToDeltaTime(temp->start);//Delta is the absolute delta time of this lyric's timestamp
			assert_wrapper(thisdelta>=lastdelta);	//There's been some serious malfunction if this is false
			reldelta=thisdelta-lastdelta;	//Find the delta value relative to the last lyric event
			lastdelta=thisdelta;			//Store the absolute delta time of this event to use for next lyric

	//Prefix the first lyric of each line with a backslash
			if(temp==curline->pieces)	//If this is the first lyric in the line
			{
				tempstr=Append("\\",temp->lyric);
				free(temp->lyric);
				temp->lyric=tempstr;
			}

	//Append whitespace to lyric if it does not group with the next lyric, and there's another lyric in this line
			if(!temp->groupswithnext && (temp->next != NULL))
				temp->lyric=ResizedAppend(temp->lyric," ",1);	//Resize string to include a space at the end

	//Write lyric (as a Text event)
			WriteMIDIString(outf,reldelta,TEXT_MIDI_STRING,temp->lyric);

			temp=temp->next;	//Advance to next lyric piece in this line
		}//while(temp != NULL)

		curline=curline->next;	//Advance to next line of lyrics
	}//while(Lyrics.curline != NULL)

//Write end of track event with a delta of 0 and a null padding byte
	WriteDWORDBE(outf,0x00FF2F00UL);	//Write 4 bytes: 0, 0xFF, 0x2F and 0

	chunkfilesize=ftell_err(outf)-chunkfileposition-4;	//# of bytes from start to end of the track chunk (- chunk size variable)

//Rewind to the track chunk size location and write the correct value
	fseek_err(outf,chunkfileposition,SEEK_SET);			//Rewind to where the chunk size is supposed to be written

	WriteDWORDBE(outf,chunkfilesize);	//Write chunk size in BE format

	MIDIstruct.trackswritten++;	//One additional track has been written to the output file
	if(Lyrics.verbose)	printf("\nSoft Karaoke export complete.  %lu lyrics written\n",Lyrics.piececount);
}

int MIDI_Stats(struct TEPstruct *data)
{
	unsigned char eventtype=0;
	unsigned long ctr=0;
	char whitespace=0;

	assert_wrapper(data != NULL);	//This must not be NULL

//Special case:  A Note On event with a velocity of 0 must be treated as a Note Off event
	eventtype=data->eventtype;//This value may be interpreted differently for Note On w/ velocity=0
	if(((data->eventtype>>4) == 0x9) && (data->parameters[1] == 0))
		eventtype=0x80;	//Treat data->eventtype into Note Off, leave it as Note On in the original TEPstruct

	if(eventtype>>4 == 0x9)	//Track Note On
	{
		(MIDIstruct.hchunk.tracks[data->tracknum]).notecount++;	//One more Note On was read
		return 1;
	}

	if(data->buffer == NULL)
		return -1;	//Text/lyric events can't be statistically tracked if not passed to this handler

	if((eventtype == 0xFF) && ((data->m_eventtype == 0x1) || (data->m_eventtype == 0x5)))	//Track text/lyric events
	{
		for(ctr=0;data->buffer[ctr]!='\0';ctr++)
		{
			if(data->buffer[ctr]=='[')		//If the first non whitespace character in the string is an open bracket
				break;						//Stop parsing string, don't count this in our statistics
			if((data->buffer[ctr]=='@') && (data->m_eventtype == 0x1))	//If this is a text event, and the first non whitespace character in the string is an @ symbol (ie. Soft Karaoke tag)
				break;						//Stop parsing string, don't count this in our statistics

			if(!isspace(data->buffer[ctr]))	//Otherwise, if it isn't a whitespace character
			{
				if(data->m_eventtype == 0x1)	//If this is a text event
					(MIDIstruct.hchunk.tracks[data->tracknum]).textcount++;	//One more text event was read
				else
					(MIDIstruct.hchunk.tracks[data->tracknum]).lyrcount++;		//One more lyric event was read
				break;
			}
			else
				whitespace=1;	//Whitespace was found
		}
		if(whitespace)	//If this text/lyric event had any leading/trailing whitespace
			(MIDIstruct.hchunk.tracks[data->tracknum]).spacecount++;	//Track this
	}
	return 1;
}
