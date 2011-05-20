#include <allegro.h>
#include <stdio.h>
#include "foflc/Lyric_storage.h"
#include "foflc/Midi_parse.h"
#include "foflc/Script_parse.h"
#include "foflc/VL_parse.h"
#include "foflc/UStar_parse.h"
#include "foflc/LRC_parse.h"
#include "foflc/ID3_parse.h"
#include "foflc/SRT_parse.h"
#include "song.h"
#include "main.h"
#include "lc_import.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif


int EOF_IMPORT_VIA_LC(EOF_VOCAL_TRACK *tp, struct Lyric_Format **lp, int format, char *inputfilename, char *string2)
{
	eof_log("EOF_IMPORT_VIA_LC() entered", 1);

	char * returnedfn = NULL;	//Return string from dialog window
	FILE *inf;	  //Used to open the input file
	struct Lyric_Format *detectionlist;
	unsigned long i;

//Validate parameters
	if((tp == NULL) || (inputfilename == NULL))
		return 0;	//Return failure

	if((format == 0) && (lp == NULL))
		return 0;	//Return failure

//Perform detection logic
	InitLyrics();	//Initialize all variables in the Lyrics structure
	InitMIDI();		//Initialize all variables in the MIDI structure
	if(format == 0)		//Auto-detect lyric format
	{
		detectionlist=DetectLyricFormat(inputfilename);
		if(detectionlist == NULL)
			return 0;	//Return invalid lyric file

		if(detectionlist->format == PITCHED_LYRIC_FORMAT)
		{	//If the detection format is Pitched Lyrics, the user must specify the corresponding Vocal Rhythm MIDI
			*lp=detectionlist;	//Return the detected lyric information via the lp pointer
			return -1;			//Return prompt for user selection
		}

		if(detectionlist->next != NULL)
		{	//If there was more MIDI track with lyrics, the user must specify which track to import
			*lp=detectionlist;	//Return the detected lyric information via the lp pointer
			return -2;			//Return prompt for user selection
		}

		Lyrics.in_format=detectionlist->format;		//Format to import
		if(detectionlist->track != NULL)
			Lyrics.inputtrack=detectionlist->track;	//Track to import from
		DestroyLyricFormatList(detectionlist);		//Deallocate the linked list returned by DetectLyricFormat()
		detectionlist=NULL;
	}
	else			//Import specific format
	{
		Lyrics.in_format=format;

		if(Lyrics.in_format == KAR_FORMAT)
		{	//If this is a format for which string2 (pitched file or track name) must be specified
			if(string2 == NULL)	//If the track name to import is not given
				return 0;	//Return failure

			Lyrics.inputtrack=DuplicateString(string2);	//Make a duplicate, so its de-allocation won't affect calling function
		}
		else if(Lyrics.in_format == PITCHED_LYRIC_FORMAT)
		{	//If importing Pitched Lyrics, user must provide the Vocal Rhythm MIDI
			returnedfn = ncd_file_select(0, eof_filename, "Select Vocal Rhythm MIDI", eof_filter_midi_files);
			eof_clear_input();
			if(!returnedfn)
				return 0;	//Return error or user canceled
		}
	}

	Lyrics.infilename=DuplicateString(inputfilename);	//Make a duplicate, so it's de-allocation won't affect calling function

//Import lyrics
	switch(Lyrics.in_format)
	{
		case SCRIPT_FORMAT:	//Load script.txt format file as input
			inf=fopen_err(Lyrics.infilename,"rt");	//Script is a text format
			Script_Load(inf);
		break;

		case VL_FORMAT:	//Load VL format file as input
			inf=fopen_err(Lyrics.infilename,"rb");	//VL is a binary format
			VL_Load(inf);
		break;

		case MIDI_FORMAT:	//Load MIDI format file as input
			if(string2 == NULL)	//If no track name was given
				Lyrics.inputtrack=DuplicateString("PART VOCALS");	//Default to PART VOCALS
			else
				Lyrics.inputtrack=DuplicateString(string2);	//Make a duplicate, so its de-allocation won't affect calling function

			inf=fopen_err(Lyrics.infilename,"rb");	//MIDI is a binary format
			Parse_Song_Ini(Lyrics.infilename,1,1);	//Load ALL tags from song.ini first, as the delay tag will affect timestamps
			MIDI_Load(inf,Lyric_handler,0);		//Call MIDI_Load, specifying the new KAR-compatible Lyric Event handler
		break;

		case USTAR_FORMAT:	//Load UltraStar format file as input
			inf=fopen_err(Lyrics.infilename,"rt");	//UltraStar is a text format
			UStar_Load(inf);
		break;

		case LRC_FORMAT:	//Load LRC format file as input
			inf=fopen_err(Lyrics.infilename,"rt");	//LRC is a text format
			LRC_Load(inf);
		break;

		case VRHYTHM_FORMAT:	//Load vocal rhythm (MIDI) and pitched lyrics
//			Lyrics.inputtrack=string2;
			inf=fopen_err(returnedfn,"rb");	//Vrhythm is a binary format
			VRhythm_Load(inputfilename,returnedfn,inf);
		break;

		case PITCHED_LYRIC_FORMAT:
//			Lyrics.inputtrack=string2;
			inf=fopen_err(returnedfn,"rb");	//Vrhythm is a binary format
			VRhythm_Load(eof_filename,returnedfn,inf);
		break;

		case KAR_FORMAT:	//Load KAR MIDI file
			inf=fopen_err(Lyrics.infilename,"rb");	//KAR is a binary format
			MIDI_Load(inf,Lyric_handler,0);	//Call MIDI_Load, specifying the new KAR-compatible Lyric Event handler
		break;

		case SKAR_FORMAT:	//Load SKAR MIDI file
			inf=fopen_err(Lyrics.infilename,"rb");	//KAR is a binary format
			Lyrics.inputtrack=DuplicateString("Words");
			MIDI_Load(inf,SKAR_handler,0);	//Call MIDI_Load, specifying the Simple Karaoke Event handler
			EndLyricLine();	//KAR files do not mark the end of the last line of lyrics
		break;

		case ID3_FORMAT:	//Load MP3 ID3 tag
			inf=fopen_err(Lyrics.infilename,"rb");	//MP3 is a binary format
			ID3_Load(inf);
		break;

		case SRT_FORMAT:	//Load SRT file
			inf=fopen_err(Lyrics.infilename,"rt");	//SRT is a text format
			SRT_Load(inf);
		break;

		default:
		return 0;	//Return failure
	}//switch(Lyrics.in_format)

	free(Lyrics.infilename);
	Lyrics.infilename = NULL;

//Validate imported lyrics
	if((Lyrics.piececount == 0) || (MIDI_Lyrics.head != NULL))	//If the imported MIDI track had no valid lyrics or otherwise was incorrectly formatted
	{
		ReleaseMemory(1);	//Release memory allocated during lyric import
		fclose_err(inf);
		return 0;	  //Return no EOF lyric structure
	}

	PostProcessLyrics();	//Perform validation of pointers, counters, etc.

	RemapPitches();		//Ensure pitches are within the correct range (except for pitchless lyrics)

	//Delete any existing lyrics and lines
	for(i = 0; i < tp->lyrics; i++)
	{
		free(tp->lyric[i]);
	}
	tp->lyrics = 0;
	tp->lines = 0;

	if(EOF_TRANSFER_FROM_LC(tp,&Lyrics) != 0)	//Pass the Lyrics global variable by reference
	{
		ReleaseMemory(1);	//Release memory allocated during lyric import
		return 0;		//Return error (failed to import into EOF lyric structure)
	}

	fclose_err(inf);	//Ensure this file gets closed
	inf=NULL;
	ReleaseMemory(1);	//Release memory allocated during lyric import
	return 1;	 		//Return finished EOF lyric structure
}

int EOF_TRANSFER_FROM_LC(EOF_VOCAL_TRACK * tp, struct _LYRICSSTRUCT_ * lp)
{
	eof_log("\tImporting lyrics\nEOF_TRANSFER_FROM_LC() entered", 1);

	struct Lyric_Line *curline;	//Conductor of the lyric line linked list
	struct Lyric_Piece *curpiece;	//Conductor of the lyric piece linked list
	EOF_LYRIC *temp;		//Pointer returned by eof_vocal_track_add_lyric()
	unsigned long start=0;	//Used to track the start position of each line
	char startfound=0;		//Used to help skip adding vocal percussion notes to lyric lines
	char overdrive=0;		//Used to track the overdrive status of a lyric line

	if((tp == NULL) || (lp == NULL))
		return -1;	//Return error (invalid structure pointers)

	curline=lp->lines;	//Point line conductor to first lineof lyrics in the Lyrics structure
	while(curline != NULL)
	{	//For each line of lyrics
		startfound =  0;
		overdrive = 0;

		curpiece=curline->pieces;	//Point lyric conductor to first lyric in this line
		while(curpiece != NULL)
		{	//For each lyric in this line
			if((startfound == 0) && (curpiece->pitch != VOCALPERCUSSION))
			{
				startfound = 1;
				start=curpiece->start;	//Store the timestamp of the first non vocal percussion note in this line
			}

			if(curpiece->overdrive)				//If this lyric is overdrive
				overdrive = 1;

			temp=eof_vocal_track_add_lyric(tp);	//Add a new lyric to EOF structure
			if(temp == NULL)
				return -1;	//Return error (out of memory)

			if(curpiece->pitch == PITCHLESS)
				temp->note=0;				//Store with EOF's notation of a non defined pitch
			else
				temp->note=(char)curpiece->pitch;	//curpiece->pitch is an unsigned char between the values of 0 and 127

			temp->pos=curpiece->start;
			temp->length=(long)curpiece->duration;	//curpiece->duration is an unsigned long value, hopefully this won't cause problems
			if(ustrlen(curpiece->lyric) > EOF_MAX_LYRIC_LENGTH)
				return -1;	//Return error (lyric too large to store in EOF's array)
			if(curpiece->pitch == VOCALPERCUSSION)
				ustrcpy(temp->text, "");	//Copy an empty string for a vocal percussion note
			else
				ustrcpy(temp->text,curpiece->lyric);

			if((curpiece->next == NULL) && startfound)
			{	//If this was the last lyric for this line, and at least one non vocal percussion note was found
				eof_vocal_track_add_line(tp,start,curpiece->start + curpiece->duration);	//Add the lyric line definition to the EOF structure

				if(overdrive && (tp->lines > 0))	//If this line had any overdriven lyrics
					tp->line[tp->lines-1].flags |= EOF_LYRIC_LINE_FLAG_OVERDRIVE;	//Mark the line for overdrive
			}

			curpiece=curpiece->next;	//Point to next lyric in the line
		}

		curline=curline->next;	//Point to next line of lyrics
	}

	eof_log("\tLyrics imported", 1);

	return 0;	//Return success
}

int EOF_EXPORT_TO_LC(EOF_VOCAL_TRACK * tp,char *outputfilename,char *string2,int format)
{
	eof_log("EOF_EXPORT_TO_LC() entered", 1);

	unsigned long linectr=0,lyrctr=0,lastlyrtime=0,linestart=0,lineend=0;
	unsigned char pitch=0;
	FILE *outf=NULL;			//Used to open output file
	FILE *pitchedlyrics=NULL;	//Used to open output pitched lyric fle
	char *vrhythmid=NULL;

	if((tp == NULL) || (outputfilename == NULL))
		return -1;	//Return failure

//Initialize variables
	InitLyrics();	//Initialize all variables in the Lyrics structure
	InitMIDI();		//Initialize all variables in the MIDI structure

//Set export-specific settigns
	if(format == SCRIPT_FORMAT)
	{
		Lyrics.grouping=2;	//Enable line grouping for script.txt export
		Lyrics.nohyphens=3;	//Disable hyphen output
		Lyrics.noplus=1;	//Disable plus output
		Lyrics.filter=DuplicateString("^=%#/");	//Use default filter list
	}

//Import lyrics from EOF structure
	lyrctr=0;		//Begin indexing into lyrics from the very first
	lastlyrtime=0;	//First lyric is expected to be greater than or equal to this timestamp
	for(linectr=0;linectr<(unsigned long)tp->lines;linectr++)
	{	//For each line of lyrics in the EOF structure
		if(linestart > lineend)	//If the line starts after it ends
			return -1;			//Return failure

		if(lyrctr < tp->lyrics)	//If there are lyrics remaining
			CreateLyricLine();	//Initialize new line of lyrics

		linestart=(tp->line[linectr]).start_pos;
		lineend=(tp->line[linectr]).end_pos;

		if((tp->line[linectr]).flags & EOF_LYRIC_LINE_FLAG_OVERDRIVE)	//If this line is overdrive
			Lyrics.overdrive_on=1;
		else
			Lyrics.overdrive_on=0;

		while(lyrctr < tp->lyrics)
		{	//For each lyric
			if((tp->lyric[lyrctr])->pos < lastlyrtime)	//If this lyric precedes the previous lyric
				return -1;				//Return failure
			if((tp->lyric[lyrctr])->pos < linestart)		//If this lyric precedes the beginning of the line
				return -1;				//Return failure
			if((tp->lyric[lyrctr])->pos > lineend)		//If this lyric is placed beyond the end of this line
				break;					//Break from this while loop to have another line created

			pitch=(tp->lyric[lyrctr])->note;			//Store the lyric's pitch
			if((tp->lyric[lyrctr])->note == 0)			//Remap EOF's pitchless value to FLC's pitchless value
				pitch=PITCHLESS;

			if(!Lyrics.line_on)		//If a lyric line is not in progress
				CreateLyricLine();	//Force one to be before adding the next lyric
			AddLyricPiece((tp->lyric[lyrctr])->text,(tp->lyric[lyrctr])->pos,(tp->lyric[lyrctr])->pos+(tp->lyric[lyrctr])->length,pitch,0);
				//Add the lyric to the Lyrics structure

//			assert_wrapper(Lyrics.lastpiece != NULL);	//If an empty string was not added, allow export to continue
			if((Lyrics.lastpiece != NULL) && (Lyrics.lastpiece->lyric[strlen(Lyrics.lastpiece->lyric)-1] == '-'))	//If the piece that was just added ended in a hyphen
				Lyrics.lastpiece->groupswithnext=1;	//Set its grouping status

			lyrctr++;	//Advance to next lyric
		}

		EndLyricLine();	//End the current line of lyrics
	}

	if(Lyrics.piececount == 0)	//No lyrics imported
	{
		ReleaseMemory(1);
		return 0;		//Return no lyrics found
	}

//Load chart tags
	if(eof_song->tags->artist[0] != '\0')
		Lyrics.Artist=DuplicateString(eof_song->tags->artist);
	if(eof_song->tags->title[0] != '\0')
		Lyrics.Title=DuplicateString(eof_song->tags->title);
	if(eof_song->tags->frettist[0] != '\0')
		Lyrics.Editor=DuplicateString(eof_song->tags->frettist);

	PostProcessLyrics();	//Perform hyphen and grouping validation/handling

	//If the export format is MIDI-based, write a MIDI file header and a MIDI track (track 0) specifying a tempo of 120BPM
	if((Lyrics.out_format==MIDI_FORMAT) || (Lyrics.out_format==VRHYTHM_FORMAT) || (Lyrics.out_format==SKAR_FORMAT) || (Lyrics.out_format==KAR_FORMAT))
	{
		outf=fopen_err(Lyrics.outfilename,"wb");	//These are binary formats
		Write_Default_Track_Zero(outf);
	}

//Export lyrics
	Lyrics.outfilename=outputfilename;
	Lyrics.out_format=format;
	switch(Lyrics.out_format)
	{
		case SCRIPT_FORMAT:	//Export as script.txt format file
			outf=fopen_err(Lyrics.outfilename,"wt");	//Script.txt is a text format
			Export_Script(outf);
		break;

		case VL_FORMAT:	//Export as VL format file
			outf=fopen_err(Lyrics.outfilename,"wb");	//VL is a binary format
			Export_VL(outf);
		break;

		case MIDI_FORMAT:	//Export as MIDI format file.  Default export track is "PART VOCALS"
			if(string2 == NULL)						//If a destination track name wasn't given
				Lyrics.outputtrack=DuplicateString("PART VOCALS");	//Write track name as PART VOCALS by default
			else
				Lyrics.outputtrack=DuplicateString(string2);
			Export_MIDI(outf);
		break;

		case USTAR_FORMAT:	//Export as UltraStar format file
			outf=fopen_err(Lyrics.outfilename,"wt");	//UltraStar is a text format
			Export_UStar(outf);
		break;

		case LRC_FORMAT:	//Export as simple LRC
		case ELRC_FORMAT:	//Export as extended LRC
			outf=fopen_err(Lyrics.outfilename,"wt");	//LRC is a text format
			Export_LRC(outf);
		break;

		case VRHYTHM_FORMAT:	//Export as Vocal Rhythm (MIDI and text file)
			if(string2 == NULL)	//If a pitched lyric file wasn't given
				return -1;	//Return failure

			pitchedlyrics=fopen_err(string2,"wt");	//Pitched lyrics is a text format
			vrhythmid=DuplicateString("G4");
			Export_Vrhythm(outf,pitchedlyrics,vrhythmid);
			fflush_err(pitchedlyrics);	//Commit any pending pitched lyric writes to file
			fclose_err(pitchedlyrics);	//Close pitched lyric file
		break;

		case SKAR_FORMAT:	//Export as Soft Karaoke.  Default export track is "Words"
			if(string2 == NULL)						//If a destination track name wasn't given
				Lyrics.outputtrack=DuplicateString("Words");	//Write track name as "Words" by default
			else
				Lyrics.outputtrack=DuplicateString(string2);
			Export_SKAR(outf);
		break;

		case KAR_FORMAT:	//Export as unofficial KAR.  Default export track is "Melody"
			if(Lyrics.outputtrack == NULL)
			{
				puts("\aNo ouput track name for KAR file was given.  A track named \"Melody\" will be used by default");
				Lyrics.outputtrack=DuplicateString("Melody");
			}
			Export_MIDI(outf);
		break;

		default:
			puts("Unexpected error in export switch\nAborting");
			exit_wrapper(4);
		break;
	}

	if((Lyrics.out_format==MIDI_FORMAT) || (Lyrics.out_format==VRHYTHM_FORMAT) || (Lyrics.out_format==SKAR_FORMAT) || (Lyrics.out_format==KAR_FORMAT))
	{	//Update the MIDI header to reflect the number of MIDI tracks written to file for all applicable export formats
		fseek_err(outf,10,SEEK_SET);		//The number of tracks is 10 bytes in from the start of the file header
		fputc_err(MIDIstruct.trackswritten>>8,outf);
		fputc_err(MIDIstruct.trackswritten&0xFF,outf);
	}

//Cleanup
	fclose_err(outf);
//	fflush_err(stdout);	//For Allegro programs, fflush(stdout) seems to fail, so this function calls exit()

	ReleaseMemory(1);
	return 1;	//Return success
}
