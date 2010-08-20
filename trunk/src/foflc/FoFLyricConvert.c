#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <setjmp.h>
#include <assert.h>
#include "Lyric_storage.h"
#include "VL_parse.h"
#include "Midi_parse.h"
#include "Script_parse.h"
#include "UStar_parse.h"
#include "LRC_parse.h"
#include "ID3_parse.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

//To enable memwatch, add memwatch.c to the build/project, uncomment the USEMEMWATCH #define in Lyric_storage.h and pass these parameters to the compiler:
//-DMEMWATCH -DMEMWATCH_STDIO


static void Input_failed(int errnum,char *argument);
	//If the command line parameters are not correct, this function is called.  If a generic error (unrecognized
	//argument) is passed to errnum, argument will be a reference to argv[errnum]
static void DisplaySyntax(void);	//Displays program syntax
static void DisplayHelp(void);		//Lists verbose program help


int main(int argc, char *argv[])
{
//Command line syntax:	FoFLyricConvert [-offset #] [-nohyphens [1|2|3]] [-noplus] [-marklines] [-nolyrics]
//						[-grouping {word | line}] [-verbose | debug] [-filter [...]] [-quick] [-startstamp #]
//						[-bpm #] [-brute] [-help] [-srcoffset #] [-intrack "TRACKNAME"] [-outtrack "TRACKNAME"]
//						[-notenames] [-relative] [-nopitch] [-nosrctag [...]] [-nofstyle]
//						{  {-detect infile} | {-id3tag infile} |
//						   {-in [FORMAT] infile (lyr | ID)} {-out FORMAT (srcfile) outfile (lyrics)(vrhythmID)}  }
	FILE *inf=NULL,*outf=NULL,*srcfile=NULL,*pitchedlyrics=NULL;
		//Input and output file pointers (srcfile is only used to open source midi/mp3 for certain exports, pitchedlyrics is only used during vrhythm export)
	char offset_defined=0;			//Boolean:  Offset value has been defined as a command line parameter
	char srcoffset_defined=0;		//Boolean:  Source offset value has been defined as a command line parameter
	char midi_based_import=0;		//Boolean:  The import format is MIDI based (KAR, MIDI or Vrhythm)
	char midi_based_export=0;		//Boolean:  The export format is MIDI based (KAR, MIDI or Vrhythm)
	int ctr=0;
	char c=0;							//Temporary variable
	size_t x=0,y=0;						//Temporary variables
	char *correct_extension=NULL;		//Used to validate the file extension
	char *temp=NULL;					//Used to validate the file extension
	char *vrhythmid=NULL;				//A command line parameter used during vrhythm export containing the instrument and difficulty ID (ie. D4 for Expert drums)
	struct Lyric_Line *lineptr=NULL;	//Used for applying the delay from a source MIDI's song.ini file
	struct Lyric_Piece *curpiece=NULL;	//Used for applying the delay from a source MIDI's song.ini file
	struct Lyric_Format *detectionlist=NULL;	//List of formats returned by DetectedLyricFormat()
	char *detectfile=NULL;				//This is set to the filename to detect if the "-detect" parameter is specified, which will interrupt the regular program control
	struct _LYRICSSTRUCT_ *backuplyric=NULL;	//Used to keep a copy of all settings in the Lyrics structure while lyric detection is performed
	unsigned char ID3omit=0;			//The user specified to omit individual source ID3 frames (1) or all source frames (0xFF) by using -nosrctag
	int ctr2=0,noarg1=0,noarg2=0,noarg3=0;	//Used to pre-determine the availability of subparameters three ahead of the current parameter
											//Values of noarg#: 0=The subparameter exists and does not begin with hyphen
											//0xF0=The parameter exists, but begins with hyphen
											//0xFF=The parameter doesn't exist

	if(argc < 2)	//User didn't provide any parameters
	{				//Display syntax and terminate
		DisplaySyntax();
		return 0;
	}

	InitLyrics();		//Initialize all variables in the Lyrics structure
	InitMIDI();			//Initialize all variables in the MIDI structure
	backuplyric=malloc_err(sizeof(struct _LYRICSSTRUCT_));	//Allocate memory to back up the Lyrics structure

	for(ctr=1;ctr<argc;ctr++)	//For each specified parameter (skipping executable name)
	{
//Pre-check the next 3 parameters to see if they are valid for use as subparameters
		for(ctr2=1;ctr2<4;ctr2++)
		{
			if(argc < ctr+ctr2+1)
				noarg3=0xFF;
			else if((argv[ctr+ctr2])[0] == '-')
				noarg3=0xF0;
			else
				noarg3=0;

		//If the 3rd parameter to ctr wasn't being examined, store the status in the appropriate variable
			if(ctr2==1)
				noarg1=noarg3;
			else if(ctr2==2)
				noarg2=noarg3;
		}

//Handle -in parameter, which must be immediately followed by script, vl or midi and then by the input filename
//	For vocal rhythm import, the syntax is -in vrhythm srcrhythm input_lyrics
		if(strcasecmp(argv[ctr],"-in") == 0)
		{
			if(noarg1 != 0)		//If there's not at least one more parameter, or if the next one begins with hyphen
				Input_failed(noarg1,NULL);
			if(Lyrics.infilename != NULL)	//If the user already defined the input file parameters
				Input_failed(0xEF,NULL);

			if(noarg2 == 0)	//If the user provided two parameters
			{	//The first parameter is the format, the second is the input file
				Lyrics.infilename=argv[ctr+2];		//Store this filename
				if(strcasecmp(argv[ctr+1],"script") == 0)
					Lyrics.in_format=SCRIPT_FORMAT;
				else if(strcasecmp(argv[ctr+1],"vl") == 0)
					Lyrics.in_format=VL_FORMAT;
				else if(strcasecmp(argv[ctr+1],"midi") == 0)
				{
					Lyrics.in_format=MIDI_FORMAT;
					midi_based_import=1;
				}
				else if(strcasecmp(argv[ctr+1],"ustar") == 0)
					Lyrics.in_format=USTAR_FORMAT;
				else if(strcasecmp(argv[ctr+1],"lrc") == 0)
					Lyrics.in_format=LRC_FORMAT;
				else if(strcasecmp(argv[ctr+1],"vrhythm") == 0)
				{	//-in vrhythm infile {lyrics | ID}
					Lyrics.in_format=VRHYTHM_FORMAT;
					midi_based_import=1;

					if(noarg3)
					{
						if(noarg3==0xF0)
							noarg3=0xF1;	//Correct error number
						Input_failed(noarg3,NULL);
					}

					Lyrics.srcrhythmname=argv[ctr+2];	//Store source rhythm midi file's name (it's after "-in vrhythm")
														//Used to import tags from song.ini if it exists

					Lyrics.inputtrack=AnalyzeVrhythmID(argv[ctr+3]);	//Test parameter 3 for being an ID, if so, load the parameters
					if(Lyrics.inputtrack)	//If it was a valid ID
						Lyrics.nolyrics=2;	//Implicitly enable the nolyrics behavior
					else					//If it was not a valid ID, store parameter 3 as a filename
						Lyrics.srclyrname=argv[ctr+3];		//Store the input pitched lyric file's name

					ctr++;		//Seek past one additional parameter because this format included one more than the others
				}
				else if(strcasecmp(argv[ctr+1],"kar") == 0)
				{
					Lyrics.in_format=KAR_FORMAT;
					midi_based_import=1;
				}
				else if(strcasecmp(argv[ctr+1],"skar") == 0)
				{
					Lyrics.in_format=SKAR_FORMAT;
					midi_based_import=1;
				}
				else if(strcasecmp(argv[ctr+1],"id3") == 0)
					Lyrics.in_format=ID3_FORMAT;
				else
					Input_failed(ctr+1,NULL);

				ctr+=2;		//seek past these parameters because we processed them
			}
			else
			{	//The next parameter is the input file, the format was not specified and will be detected
				Lyrics.infilename=argv[ctr+1];		//Store this filename
				ctr++;		//seek past this parameter because we processed it
			}
		}

//Handle -out parameter, which must be immediately followed by script,vl or midi, followed by the source midi (if midi is
//the ouput file type and the input type is not MIDI), followed by the output file name
		else if(strcasecmp(argv[ctr],"-out") == 0)
		{
			if(noarg2 != 0)		//If there are not at least two more parameters, or the second one begins with hyphen
				Input_failed(noarg2,NULL);

			if(Lyrics.out_format != 0)	//If the user already defined the output file parameters
				Input_failed(0xEF,NULL);

			if(strcasecmp(argv[ctr+1],"script") == 0)
			{
				Lyrics.out_format=SCRIPT_FORMAT;
				Lyrics.outfilename=argv[ctr+2];
				correct_extension=DuplicateString(".txt");	//.txt is the correct extension for this file format
			}
			else if(strcasecmp(argv[ctr+1],"vl") == 0)
			{
				Lyrics.out_format=VL_FORMAT;
				Lyrics.outfilename=argv[ctr+2];
				correct_extension=DuplicateString(".vl");	//.vl is the correct extension for this file format
			}
			else if(strcasecmp(argv[ctr+1],"midi") == 0)	//-out midi [sourcemidi] outputfile
			{	//if output format is midi, the next parameter must be a source midi file, then output file
				//Ensure that the parameter after "midi" is read as a filename (not beginning with hyphen)
				//Check if there is are one or two filenames given with the -out midi parameter
				if(noarg3 == 0)	//If there is a third parameter to -out that doesn't begin with a hyphen
				{	//The command line is -out midi filename1 filename2, filename1 is used as the source, filename2 is the output file
					Lyrics.srcfilename=argv[ctr+2];	//Store source midi file's name
					Lyrics.outfilename=argv[ctr+3];	//Store output file's name
					ctr++;	//seek past one additional parameter compared to if the source midi was specified in -in
				}
				else
				{	//The command line is -out midi filename, filename is used as the output file
					Lyrics.outfilename=argv[ctr+2];	//Store output file's name
				}

				Lyrics.out_format=MIDI_FORMAT;
				correct_extension=DuplicateString(".mid");	//.mid is the correct extension for this file format
				midi_based_export=1;
			}
			else if(strcasecmp(argv[ctr+1],"ustar") == 0)
			{
				Lyrics.out_format=USTAR_FORMAT;
				Lyrics.outfilename=argv[ctr+2];
				correct_extension=DuplicateString(".txt");	//.txt is the correct extension for this file format
			}
			else if(strcasecmp(argv[ctr+1],"lrc") == 0)
			{	//Simple LRC format
				Lyrics.out_format=LRC_FORMAT;
				Lyrics.outfilename=argv[ctr+2];
				correct_extension=DuplicateString(".lrc");	//.lrc is the correct extension for this file format
			}
			else if(strcasecmp(argv[ctr+1],"elrc") == 0)
			{	//Extended LRC format
				Lyrics.out_format=ELRC_FORMAT;
				Lyrics.outfilename=argv[ctr+2];
				correct_extension=DuplicateString(".lrc");	//.lrc is the correct extension for this file format
			}
			else if(strcasecmp(argv[ctr+1],"vrhythm") == 0)	//-out vrhythm [sourcemidi] outputrythm pitchedlyrics rhythmID
			{	//Vocal Rhythm format
				if(argc < ctr+4+1)	//Importing this format requires at least 4 parameters to -out
					Input_failed(0xFF,NULL);
				//Ensure that the next two parameters don't start with a hyphen, because no new parameters are expected
				if(noarg3 || ((argv[ctr+4])[0] == '-'))
					Input_failed(0xF0,NULL);

				if((argc > ctr+5) && ((argv[ctr+5])[0] != '-'))	//If there is a fifth parameter to -out that doesn't begin with a hyphen
				{	//The command line is -out vrhythm sourcemidi outputrhythm pitchedlyrics rhythmID
					Lyrics.srcfilename=argv[ctr+2];	//Store the source midi file's name
					Lyrics.outfilename=argv[ctr+3];	//Store output rhythm midi file name (it's after "-in vrhythm")
					Lyrics.dstlyrname=argv[ctr+4];	//Store the output pitched lyric file's name
					vrhythmid=argv[ctr+5];			//Store the vocal rhythm track/difficulty's ID
					ctr+=3;							//Seek past three additional parameters because this format included three more than normal
				}
				else
				{	//The command line is -out vrhythm outputrhythm pitchedlyrics rhythmID
					Lyrics.outfilename=argv[ctr+2];	//Store output rhythm midi file name (it's after "-in vrhythm")
					Lyrics.dstlyrname=argv[ctr+3];	//Store the output pitched lyric file's name
					vrhythmid=argv[ctr+4];			//Store the vocal rhythm track/difficulty's ID
					ctr+=2;							//Seek past two additional parameters because this format included two more than normal
				}

				Lyrics.out_format=VRHYTHM_FORMAT;
				correct_extension=DuplicateString(".mid");	//.mid is the correct extension for the output MIDI
				midi_based_export=1;

				//Validate instrument/difficulty identifier
				if(!isalpha(vrhythmid[0]))	//A letter is expected as the first character of the instrument/difficulty identifier
					Input_failed(0xE9,NULL);

				//Validate extension of output pitched lyric filename
				temp=strrchr(Lyrics.dstlyrname,'.');	//Find last instance of a period
				if(temp != NULL)
					if(strcasecmp(temp,".txt") != 0)	//Compare the filename starting with the last period with the correct extension
						temp=NULL;

				if(temp == NULL)	//If the output filename didn't end with a period, the correct extension and then a null terminator
					printf("\a! Warning: Output pitch file (\"%s\") does not have the correct file extension (.txt)\n",Lyrics.dstlyrname);
			}
			else if(strcasecmp(argv[ctr+1],"skar") == 0)	//-out skar [sourcemidi] output
			{	//Soft Karaoke format
				//Check if there is are one or two filenames given with the -out midi parameter
				if(noarg3 == 0)	//If there is a third parameter to -out that doesn't begin with a hyphen
				{	//The command line is -out skar filename1 filename2, filename1 is used as the source, filename2 is the output file
					Lyrics.srcfilename=argv[ctr+2];	//Store source midi file's name
					Lyrics.outfilename=argv[ctr+3];	//Store output file's name
					ctr++;	//seek past one additional parameter compared to if the source midi was specified in -in
				}
				else
				{	//The command line is -out midi filename, filename is used as the output file
					Lyrics.outfilename=argv[ctr+2];	//Store output file's name
				}

				Lyrics.out_format=SKAR_FORMAT;
				correct_extension=DuplicateString(".kar");	//.kar is the correct extension for this file format
				midi_based_export=1;
			}
			else if(strcasecmp(argv[ctr+1],"kar") == 0)	//-out kar [sourcemidi] output
			{	//Unofficial KAR format
				//Check if there is are one or two filenames given with the -out midi parameter
				if(noarg3 == 0)	//If there is a third parameter to -out that doesn't begin with a hyphen
				{	//The command line is -out skar filename1 filename2, filename1 is used as the source, filename2 is the output file
					Lyrics.srcfilename=argv[ctr+2];	//Store source midi file's name
					Lyrics.outfilename=argv[ctr+3];	//Store output file's name
					ctr++;	//seek past one additional parameter compared to if the source midi was specified in -in
				}
				else
				{	//The command line is -out midi filename, filename is used as the output file
					Lyrics.outfilename=argv[ctr+2];	//Store output file's name
				}

				Lyrics.out_format=KAR_FORMAT;
				correct_extension=DuplicateString(".kar");	//.kar is the correct extension for this file format
				midi_based_export=1;
			}
			else if(strcasecmp(argv[ctr+1],"id3") == 0)	//-out id3 sourcemp3 output
			{
				if(Lyrics.in_format==ID3_FORMAT)
				{	//Check for abbreviated ID3 export syntax if the import format was also ID3
					if(noarg3 == 0)
					{	//If there are at least 3 parameters remaining, and the third parameter to -out doesn't begin
						//with a hyphen, parse normal ID3 export syntax
						Lyrics.srcfilename=argv[ctr+2];		//Store the source MP3 filename
						Lyrics.outfilename=argv[ctr+3];	//Store the output MP3 filename
						ctr++;	//seek past one additional parameter since this export requires one extra
					}
					else
					{	//Parse the abbreviated sytax (use the input file as the source file)
						Lyrics.srcfilename=Lyrics.infilename;	//Store the source MP3 filename
						Lyrics.outfilename=argv[ctr+2];		//Store the output MP3 filename
					}
				}
				else
				{	//Parse normal ID3 export syntax
					if(noarg3)	//If there's not a third parameter or it begins with hyphen
						Input_failed(noarg3,NULL);

					Lyrics.srcfilename=argv[ctr+2];	//Store the source MP3 filename
					Lyrics.outfilename=argv[ctr+3];	//Store output file's name
					ctr++;	//seek past one additional parameter since this export requires one extra
				}
				Lyrics.out_format=ID3_FORMAT;
				correct_extension=DuplicateString(".mp3");	//.mp3 is the correct extension for this file format
			}
			else	//Any other specified output format is invalid
				Input_failed(ctr+1,NULL);

			//Ensure that the parameter after the format was a filename (doesn't start with hyphen)
			ctr+=2;		//seek past these parameters because we processed them
		}

//Handle -offset parameter
		else if(strcasecmp(argv[ctr],"-offset") == 0)
		{	//The parameter to offset is allowed to begin with a hyphen (negative offset)
			if(argc < ctr+1+1)	//If there's not at least one more parameter
				Input_failed(0xFF,NULL);

			if(offset_defined != 0)	//If this parameter was already defined
				Input_failed(0xEF,NULL);

			if(Lyrics.startstampspecified)	//If user defined a custom starting timestamp
				Input_failed(0xED,NULL);

			if(strcasecmp(argv[ctr+1],"0") != 0)
			{	//Only accept the specified offset if it is not zero
				assert_wrapper(argv[ctr+1] != NULL);	//atol() may crash the program if it is passed NULL

				Lyrics.realoffset=atol(argv[ctr+1]);
				if(Lyrics.realoffset == 0)	//if atol() couldn't convert string to integer
					Input_failed(0xFD,NULL);
			}
			else
				Lyrics.realoffset=0;	//User actually specified "-offset 0" on the command line

			Lyrics.offsetoverride=1;	//This offset will override anything specified in the input file
			ctr++;		//seek past next parameter because we processed it
			offset_defined=1;
		}

//Handle -srcoffset parameter
		else if(strcasecmp(argv[ctr],"-srcoffset") == 0)
		{	//The parameter to srcoffset is allowed to begin with a hyphen (negative offset)
			if(argc < ctr+1+1)	//If there's not at least one more parameter
				Input_failed(0xFF,NULL);

			if(srcoffset_defined != 0)	//If this parameter was already defined
				Input_failed(0xEF,NULL);

			if(strcasecmp(argv[ctr+1],"0") != 0)
			{	//Only accept the specified offset if it is not zero
				assert_wrapper(argv[ctr+1] != NULL);	//atol() may crash the program if it is passed NULL

				Lyrics.srcrealoffset=atol(argv[ctr+1]);
				if(Lyrics.srcrealoffset == 0)	//if atol() couldn't convert string to integer
					Input_failed(0xFD,NULL);
			}
			else
				Lyrics.srcrealoffset=0;	//User actually specified "-offset 0" on the command line

			Lyrics.srcoffsetoverride=1;	//This offset will override anything specified in the input file
			ctr++;		//seek past next parameter because we processed it
			srcoffset_defined=1;
		}

//Handle -nohyphens parameter
		else if(strcasecmp(argv[ctr],"-nohyphens") == 0)
		{	//If there's not at least one more parameter, or the next parameter begins with a hyphen, assume option 3 (suppress all hyphens)
			if(noarg1)
				Lyrics.nohyphens=3;
			else
			{
				if(Lyrics.nohyphens != 0)	//If this parameter was already defined
					Input_failed(0xEF,NULL);

				if(strcasecmp(argv[ctr+1],"0") != 0)
				{	//Only accept the specified nohyphens value if it is not zero
					Lyrics.nohyphens=(char)(atoi(argv[ctr+1]) & 0xFF);	//Enforce capping at an 8 bit value
					if(Lyrics.nohyphens == 0)	//if atoi() couldn't convert string to integer
						Input_failed(0xFD,NULL);
					if(Lyrics.nohyphens > 3)
						Input_failed(0xFC,NULL);
				}
				else	//If the user specified "-nohyphens 0"
					Input_failed(0xFC,NULL);

				ctr++;		//seek past next parameter because we processed it
			}
		}

//Handle -grouping parameter
		else if(strcasecmp(argv[ctr],"-grouping") == 0)
		{
			if(argc < ctr+1+1)	//If there's not at least one more parameter
				Input_failed(0xFF,NULL);

			if(Lyrics.grouping != 0)	//If this parameter was already defined
				Input_failed(0xEF,NULL);

			if(strcasecmp(argv[ctr+1],"word") == 0)
				Lyrics.grouping=1;
			else if(strcasecmp(argv[ctr+1],"line") == 0)
				Lyrics.grouping=2;
			else
				Input_failed(ctr+1,NULL);

			ctr++;		//seek past next parameter because we processed it
		}

//Handle -noplus parameter
		else if(strcasecmp(argv[ctr],"-noplus") == 0)
		{
			if(Lyrics.noplus != 0)	//If this parameter was already defined
				Input_failed(0xEF,NULL);

			Lyrics.noplus=1;
		}

//Handle -verbose parameter
		else if(strcasecmp(argv[ctr],"-verbose") == 0)
		{
			if(Lyrics.verbose != 0)
				Input_failed(0xE7,NULL);		//Can only define verbose/debug output parameter once
			Lyrics.verbose=1;
		}

//Handle -debug parameter
		else if(strcasecmp(argv[ctr],"-debug") == 0)
		{
			if(Lyrics.verbose != 0)
				Input_failed(0xE7,NULL);		//Can only define verbose/debug output parameter once
			Lyrics.verbose=2;
		}

//Handle -filter parameter
		else if(strcasecmp(argv[ctr],"-filter") == 0)
		{
			if(Lyrics.filter != NULL)
				Input_failed(0xEF,NULL);		//Can only define this parameter once

			if(noarg1 == 0)
			{	//If there's at least one more parameter and it doesn't begin with a hyphen
				Lyrics.filter=argv[ctr+1];		//Use it as the filter list
				//Validate custom filter list
				y=strlen(Lyrics.filter);
				assert_wrapper(y > 0);	//If a parameter has 0 characters, the OS' command line parser is glitching
				for(x=0;x<y;x++)
				{
					c=Lyrics.filter[x];
					if(isalnum((unsigned char)c) || isspace((unsigned char)c) || (c == '-') || (c == '+') || (c== '='))
						Input_failed(0xEE,NULL);
				}
				ctr++;		//seek past next parameter because we processed it
			}
			else	//Assume the default filter list
			{
				if(Lyrics.verbose)	puts("No filter string provided.  Default filter \"^%#/\" is assumed");
				Lyrics.filter=DuplicateString("^%#/");
				Lyrics.defaultfilter=1;	//Remember to deallocate this at end of program
			}
		}

//Handle -quick parameter
//	This parameter is only effectively used during MIDI import
		else if(strcasecmp(argv[ctr],"-quick") == 0)
		{
				if(Lyrics.quick != 0)
					Input_failed(0xEF,NULL);	//Can only define this parameter once

				Lyrics.quick=1;
		}

//Handle -bpm parameter
//	This parameter is only allowed for UltraStar export
		else if(strcasecmp(argv[ctr],"-bpm") == 0)
		{
			if(Lyrics.explicittempo > 1.0)	//If already set to a tempo
				Input_failed(0xEF,NULL);	//Can only define this parameter once

			if(noarg1)	//If there's not at least one more parameter, or it begins with a hyphen
				Input_failed(noarg1,NULL);

			assert_wrapper(argv[ctr+1] != NULL);	//atol() may crash the program if it is passed NULL

			Lyrics.explicittempo=atof(argv[ctr+1]);
			if(Lyrics.explicittempo < 2.0)	//if atol() didn't set an acceptable tempo or couldn't convert string
				Input_failed(0xFD,NULL);
			if(atof(argv[ctr+1]) < 0)		//Supplied tempo cannot be negative
				Input_failed(ctr+1,NULL);

			ctr++;		//seek past next parameter because we processed it
		}

//Handle -startstamp parameter
		else if(strcasecmp(argv[ctr],"-startstamp") == 0)
		{
			if(argc < ctr+1+1)	//If there's not at least one more parameter
				Input_failed(0xFF,NULL);

			if(Lyrics.startstampspecified != 0)	//If this parameter was already defined
				Input_failed(0xEF,NULL);

			if(offset_defined != 0)		//If user defined a custom starting timestamp offset
				Input_failed(0xED,NULL);

			if(strcasecmp(argv[ctr+1],"0") != 0)
			{	//Only accept the specified offset if it is not zero
				assert_wrapper(argv[ctr+1] != NULL);	//atol() may crash the program if it is passed NULL

				Lyrics.startstamp=(unsigned long)atol(argv[ctr+1]);
				if(Lyrics.startstamp == 0)	//if atol() couldn't convert string to integer
					Input_failed(0xFD,NULL);
				if(atol(argv[ctr+1]) < 0)	//Supplied starting timestamp cannot be negative
					Input_failed(ctr+1,NULL);
			}
			else
				Lyrics.startstamp=0;	//User actually specified "-startstamp 0" on the command line

			Lyrics.startstampspecified=1;	//This first timestamp will be overridden and the other timestamps will be offsetted
			Lyrics.offsetoverride=1;
			ctr++;		//seek past next parameter because we processed it
		}

//Handle -brute parameter
//	This parameter is only used for UltraStar export
		else if(strcasecmp(argv[ctr],"-brute") == 0)
		{
			if(Lyrics.brute != 0)	//If this parameter was already defined
				Input_failed(0xEF,NULL);

			Lyrics.brute = 1;
		}

//Handle -marklines parameter
		else if(strcasecmp(argv[ctr],"-marklines") == 0)
		{
			if(Lyrics.marklines != 0)	//If this parameter was already defined
				Input_failed(0xEF,NULL);

			Lyrics.marklines = 1;
		}

//Handle -help parameter
		else if(strcasecmp(argv[ctr],"-help") == 0)
			DisplayHelp();

//Handle -intrack parameter
		else if(strcasecmp(argv[ctr],"-intrack") == 0)
		{
			if(Lyrics.inputtrack != NULL)	//If this parameter was already defined
				Input_failed(0xEF,NULL);

			if(noarg1)	//If there's not at least one more parameter, or it begins with a hyphen
			{
				if(noarg1==0xF0)
					noarg1=0xF9;	//Correct error number
				Input_failed(noarg1,NULL);
			}

			Lyrics.inputtrack=DuplicateString(argv[ctr+1]);
			ctr++;		//seek past next parameter because we processed it
		}

//Handle -outtrack parameter
		else if(strcasecmp(argv[ctr],"-outtrack") == 0)
		{
			if(Lyrics.outputtrack != NULL)	//If this parameter was already defined
				Input_failed(0xEF,NULL);

			if(noarg1)	//If there's not at least one more parameter, or it begins with a hyphen
			{
				if(noarg1==0xF0)
					noarg1=0xF9;	//Correct error
				Input_failed(noarg1,NULL);
			}

			Lyrics.outputtrack=DuplicateString(argv[ctr+1]);
			ctr++;		//seek past next parameter because we processed it
		}

//Handle -nolyrics parameter
		else if(strcasecmp(argv[ctr],"-nolyrics") == 0)
		{
			if(Lyrics.nolyrics == 1)	//If this parameter was already explicitly defined (not implicitly by importing vrhythm without lyrics)
				Input_failed(0xEF,NULL);

			Lyrics.nolyrics=1;
		}

//Handle -notenames parameter
		else if(strcasecmp(argv[ctr],"-notenames") == 0)
		{
			if(Lyrics.notenames)	//If this parameter was already defined
				Input_failed(0xEF,NULL);

			Lyrics.notenames=1;
		}

//Handle -relative parameter
		else if(strcasecmp(argv[ctr],"-relative") == 0)
		{
			if(Lyrics.relative)	//If this parameter was already defined
				Input_failed(0xEF,NULL);

			Lyrics.relative=1;
		}

//Handle -nopitch parameter
		else if(strcasecmp(argv[ctr],"-nopitch") == 0)
		{
			if(Lyrics.nopitch)	//If this parameter was already defined
				Input_failed(0xEF,NULL);

			Lyrics.nopitch=1;
		}
//Handle -detect parameter
		else if(strcasecmp(argv[ctr],"-detect") == 0)
		{
			if(argc < ctr+1+1)	//If there's not at least one more parameter
				Input_failed(0xFF,NULL);
			if(detectfile != NULL)	//If this is already defined
				Input_failed(0xEF,NULL);

			detectfile=argv[ctr+1];
			ctr++;		//seek past next parameter because we processed it
		}

//Handle -id3tag parameter
		else if(strcasecmp(argv[ctr],"-id3tag") == 0)
		{
			if(argc < ctr+1+1)	//If there's not at least one more parameter
				Input_failed(0xFF,NULL);

			DisplayID3Tag(argv[ctr+1]);
			return 0;
		}

//Handle -nosrctag parameter
		else if(strcasecmp(argv[ctr],"-nosrctag") == 0)
		{
			if(noarg1 == 0)	//If there's at least one more parameter that doesn't begin with a hyphen
			{	//Process all parameters to -nosrctag.  Parameters end when they run out or if one begins with a hyphen
				while((ctr + 1 < argc) && ((argv[ctr+1])[0] != '-'))
				{	//While there's at least one more parameter and it doesn't begin with a hyphen
					if(ID3omit == 0xFF)	//If user already omitted all frames
						Input_failed(0xFB,NULL);
					Lyrics.nosrctag=AddOmitID3framelist(Lyrics.nosrctag,argv[ctr+1]);	//Linked list create/append with specified ID
					ID3omit=1;	//User specified to omit one or more individual frames
					ctr++;		//seek past next parameter because we processed it
				}
			}
			else	//No parameters to -nosrctag were given
			{
				if(ID3omit == 1)	//If user already omitted individual frames
					Input_failed(0xFB,NULL);
				if(ID3omit == 0xFF)	//If user already omitted all frames
					Input_failed(0xFA,NULL);

				Lyrics.nosrctag=AddOmitID3framelist(Lyrics.nosrctag,"*");	//Linked list create/append with wildcard
				ID3omit=0xFF;	//User specified to omit all source ID3 frames
			}
		}

//Handle -nofstyle parameter
		else if(strcasecmp(argv[ctr],"-nofstyle") == 0)
		{
			if(Lyrics.nofstyle)	//If this parameter was already defined
				Input_failed(0xEF,NULL);

			Lyrics.nofstyle=1;
		}

//Anything else is not a supported command line parameter
		else
			Input_failed(ctr,argv[ctr]);	//Pass the argument, so that it can be pointed out to the user
	}//end for(ctr=1;ctr<argc;ctr++)

//Output command line
	if(Lyrics.verbose)
	{
		puts(PROGVERSION);	//Output program version
		printf("Invoked with the following command line:\n");
		for(ctr=0;ctr<argc;ctr++)
		{	//For each parameter (including the program name)
			assert_wrapper(argv[ctr] != NULL);	//If this condition is true, then the command line interpreter has failed,

			if(strchr(argv[ctr],' '))		//If the parameter contains whitespace
				putchar('"');				//Begin the parameter with a quotation mark
			printf("%s",argv[ctr]);			//Display the parameter
			if(strchr(argv[ctr],' '))		//If the parameter contains whitespace
				putchar('"');				//End the parameter with a quotation mark
			if(ctr+1<argc)	putchar(' ');	//Display a space if there's another parameter
		}
		putchar('\n');
		putchar('\n');
	}

//Perform lyric detection if user opted to detect a file's lyric format instead of performing a conversion
	if(detectfile != NULL)
	{
		detectionlist=DetectLyricFormat(detectfile);
		printf("\nDetected lyric format(s) of file \"%s\":\n",detectfile);
		EnumerateFormatDetectionList(detectionlist);
		DestroyLyricFormatList(detectionlist);
		return 0;	//Return successful program completion
	}

//Perform lyric detection if user specifid an input file but not its format
	if(!Lyrics.in_format && Lyrics.infilename)	//If user specified an input file but not its format
	{
		if(Lyrics.verbose)	printf("Auto detecting the lyric format(s) of file \"%s\"\n",Lyrics.infilename);

	//Back up the Lyrics structure, perform lyric detection and restore the Lyrics structure
		memcpy(backuplyric,&Lyrics,sizeof(struct _LYRICSSTRUCT_));	//Back up the Lyrics structure before performing detection, which will wipe it out
		detectionlist=DetectLyricFormat(Lyrics.infilename);
		memcpy(&Lyrics,backuplyric,sizeof(struct _LYRICSSTRUCT_));	//Restore the Lyrics structure as it was created by parsing the command line

		if(detectionlist == NULL)
		{
			puts("No valid lyrics were detected\nExiting");
			DestroyLyricFormatList(detectionlist);
			exit_wrapper(1);
		}

		if(detectionlist->next != NULL)
		{
			puts("Multiple detected lyric format(s):");
			EnumerateFormatDetectionList(detectionlist);
			DestroyLyricFormatList(detectionlist);
			puts("\nCall the program again, specifying the desired format to import\nExiting");
			exit_wrapper(1);
		}

		if(detectionlist->format == PITCHED_LYRIC_FORMAT)
		{
			puts("Pitched lyric file detected.  Call the program again, specifying parameters vrhythm import\nExiting");
			DestroyLyricFormatList(detectionlist);
			exit_wrapper(1);
		}

	//Apply the detected format as the import format
		if(Lyrics.verbose)	printf("Selecting \"%s\" format for import\n",LYRICFORMATNAMES[detectionlist->format]);
		Lyrics.in_format=detectionlist->format;
		DestroyLyricFormatList(detectionlist);
		detectionlist=NULL;
	}

//Verify that the required parameters were defined
	if((Lyrics.in_format==0) || (Lyrics.out_format==0))
		Input_failed(0xFF,NULL);

//Verify that no conflicting parameters were defined
	if(Lyrics.quick && ((Lyrics.in_format != MIDI_FORMAT) && (Lyrics.in_format != KAR_FORMAT)))
		Input_failed(0xEC,NULL);	//The quick parameter is only valid for MIDI import
	if((Lyrics.explicittempo > 1.0) && ((Lyrics.out_format != USTAR_FORMAT) && !midi_based_export))
		Input_failed(0xEB,NULL);	//The BPM parameter is only valid for UltraStar and MIDI based exports
	if(Lyrics.brute && (Lyrics.out_format != USTAR_FORMAT))
		Input_failed(0xEA,NULL);	//The brute parameter is only valid for UltraStar export
	if(Lyrics.marklines && (Lyrics.out_format != SCRIPT_FORMAT))
		Input_failed(0xE0,NULL);	//The marklines parameter is only valid for Script export
	if((Lyrics.nolyrics == 2) && ((Lyrics.in_format != KAR_FORMAT) || (Lyrics.in_format != MIDI_FORMAT)))
		Input_failed(0xE8,NULL);	//The nolyrics parameter can only be specified explicitly for MIDI/KAR import
	if(Lyrics.quick && (!midi_based_import && !midi_based_export))
		Input_failed(0xE6,NULL);	//The quick parameter is only valid for MIDI, KAR or Vrhythm import/export
	if(srcoffset_defined && (Lyrics.srcfilename == NULL))
		Input_failed(0xE5,NULL);	//The srcoffset parameter is only valid when a source file is provided
	if(Lyrics.inputtrack && (!midi_based_import || Lyrics.in_format == SKAR_FORMAT))
		Input_failed(0xE4,NULL);	//The intrack parameter is only valid for MIDI or KAR import (not for SKAR import)
	if(Lyrics.outputtrack && !midi_based_export)
		Input_failed(0xE3,NULL);	//The outtrack parameter is only valid for MIDI or KAR export
	if(Lyrics.notenames && (Lyrics.out_format != VRHYTHM_FORMAT))
		Input_failed(0xE2,NULL);	//The notenames parameter is only valid for Vrhythm export
	if(Lyrics.relative && (Lyrics.out_format != USTAR_FORMAT))
		Input_failed(0xE1,NULL);	//The relative parameter is only valid for UltraStar export
	if((Lyrics.outputtrack != NULL) && (Lyrics.out_format == SKAR_FORMAT))
		Input_failed(0xD0,NULL);	//The outputtrack parameter may not be used for SKAR export, which requires pre-determined track names
	if(Lyrics.nofstyle && (Lyrics.out_format != MIDI_FORMAT))
		Input_failed(0xF8,NULL);	//The nofstyle parameter is only valid for MIDI export

//Display informational messages regarding parameters/formats
	if((Lyrics.grouping == 2) && Lyrics.marklines)	//User specified both marklines and line grouping
	{
		puts("Marklines is disabled when line grouping is specified");
		Lyrics.marklines=0;
	}
	if(Lyrics.grouping && ((Lyrics.out_format == MIDI_FORMAT) || (Lyrics.out_format == USTAR_FORMAT)))
		puts("Warning: Grouped lyrics will lose some timing and pitch information");

//If output file is MIDI based, validate the source midi file name if given
	if(midi_based_export)
	{	//verify that the source MIDI file was specified
		assert_wrapper(Lyrics.outfilename != NULL);	//It should not be possible for this to be NULL

		//If a source MIDI file was given, verify that it is not the same file as the output file
		if((Lyrics.srcfilename != NULL) && (strcasecmp(Lyrics.srcfilename,Lyrics.outfilename) == 0))
			Input_failed(0xFE,NULL); //special case:  Imported and exported MIDI file cannot be the same because one is read into the other
	}

//Check if the specified output filename has the correct file extension.  If not, present a warning to the user
	assert_wrapper(correct_extension != NULL);
	temp=strrchr(Lyrics.outfilename,'.');	//Find last instance of a period
	if(temp != NULL)
		if(strcasecmp(temp,correct_extension) != 0)	//Compare the filename starting with the last period with the correct extension
			temp=NULL;

	if(temp == NULL)	//If the output filename didn't end with a period, the correct extension and then a null terminator
		printf("\a! Warning: Output file does not have the correct file extension (%s)\n",correct_extension);

	free(correct_extension);	//Free this, as it is not referenced anymore

//Open the input file and import the specified format
	fflush_err(stdout);
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
			if(Lyrics.inputtrack == NULL)	//If no input track name was specified via command line
				Lyrics.inputtrack=DuplicateString("PART VOCALS");	//Default to PART VOCALS

			inf=fopen_err(Lyrics.infilename,"rb");	//MIDI is a binary format
			Parse_Song_Ini(Lyrics.infilename,1,1);	//Load ALL tags from song.ini first, as the delay tag will affect timestamps
			MIDI_Load(inf,Lyric_handler,0);	//Call MIDI_Load, specifying the new KAR-compatible Lyric Event handler
		break;

		case USTAR_FORMAT:	//Load UltraStar format file as input
			inf=fopen_err(Lyrics.infilename,"rt");	//UltraStar is a text format
			UStar_Load(inf);
		break;

		case LRC_FORMAT:	//Load LRC format file as input
		case ELRC_FORMAT:
			inf=fopen_err(Lyrics.infilename,"rt");	//LRC is a text format
			LRC_Load(inf);
		break;

		case VRHYTHM_FORMAT:	//Load vocal rhythm (MIDI) and pitched lyrics
			inf=fopen_err(Lyrics.infilename,"rb");	//Vrhythm is a binary format
			VRhythm_Load(Lyrics.srclyrname,Lyrics.srcrhythmname,inf);
		break;

		case KAR_FORMAT:	//Load KAR MIDI file
		case SKAR_FORMAT:
			inf=fopen_err(Lyrics.infilename,"rb");	//KAR is a binary format
			if(!Lyrics.inputtrack || (Lyrics.in_format==SKAR_FORMAT) || !strcasecmp(Lyrics.inputtrack,"Words"))	//If user specified no input track, specified SKAR import or specified an input track of "Words"
			{	//Perform official "Soft Karaoke" KAR logic (load lyrics based on Text events in a track called "Words")
				if(Lyrics.inputtrack != NULL)
					free(Lyrics.inputtrack);
				Lyrics.inputtrack=DuplicateString("Words");
				if(Lyrics.verbose)	puts("Using Soft Karaoke import logic");
				Lyrics.in_format=SKAR_FORMAT;
				MIDI_Load(inf,SKAR_handler,0);	//Call MIDI_Load, specifying the Simple Karaoke Event handler
			}
			else
			{	//Perform KAR logic to load lyrics based off of Note On/Off events
				assert_wrapper(Lyrics.inputtrack != NULL);
				if(Lyrics.verbose)	puts("Using RB/KAR import logic");
				MIDI_Load(inf,Lyric_handler,0);	//Call MIDI_Load, specifying the new KAR-compatible Lyric Event handler
			}
		break;

		case ID3_FORMAT:	//Load MP3 file
			inf=fopen_err(Lyrics.infilename,"rb");	//MP3 is a binary format
			ID3_Load(inf);
		break;

		default:
			puts("Unexpected error in import switch\nAborting");
			exit_wrapper(2);
		break;
	}

	fflush_err(stdout);
	fclose_err(inf);	//Ensure this file is closed
	inf=NULL;
	assert_wrapper(Lyrics.line_on == 0);	//Import functions are expected to ensure this condition

//If there were no lyrics imported, display the detected format(s) in the input file and exit program without opening a file for writing
	if(Lyrics.piececount == 0)
	{
	//Back up the Lyrics structure, perform lyric detection and restore the Lyrics structure (to ensure Lyric structure gets properly freed
		memcpy(backuplyric,&Lyrics,sizeof(struct _LYRICSSTRUCT_));	//Back up the Lyrics structure before performing detection, which will wipe it out
		detectionlist=DetectLyricFormat(Lyrics.infilename);
		memcpy(&Lyrics,backuplyric,sizeof(struct _LYRICSSTRUCT_));	//Restore the Lyrics structure as it was created by parsing the command line
		printf("Requested import did not load any lyrics\nDetected lyric format(s) of file \"%s\":\n",Lyrics.infilename);
		EnumerateFormatDetectionList(detectionlist);
		DestroyLyricFormatList(detectionlist);
		puts("Exiting");
		free(backuplyric);
		backuplyric=NULL;
		ReleaseMemory(1);
		return 0;
	}

//If importing KAR or MIDI and the MIDI_Lyrics linked list isn't empty, exit program
	if(MIDI_Lyrics.head != NULL)
	{
		printf("Invalid input KAR or MIDI file:  At least one lyric's (\"%s\") note off event is missing.\nAborting\n",MIDI_Lyrics.head->lyric);
		exit_wrapper(3);
	}

	if(Lyrics.verbose)	printf("%lu lines of lyrics loaded.\n",Lyrics.linecount);
	PostProcessLyrics();	//Perform hyphen and grouping validation/handling

	if(Lyrics.pitch_tracking && (Lyrics.out_format == MIDI_FORMAT) && CheckPitches(NULL,NULL))
	{	//Only perform input pitch validation and remapping if the import lyrics had pitch and being exported to MIDI
		puts("\aWarning: Input vocal pitches are outside Harmonix's defined range of [36,84]\nCorrecting");
		RemapPitches();		//Verify vocal pitches are correct, remap them if necessary
	}

//Source MIDI logic
//Handle the timing for MIDI based exports based on the value of Lyrics.srcfilename
	if(midi_based_export)
	{	//If outputting to a MIDI based export format, use either a default tempo or the tempos from the source midi
		outf=fopen_err(Lyrics.outfilename,"wb");	//These are binary formats

		ReleaseMIDI();	//Disregard contents of input MIDI if applicable
		InitMIDI();

		if(Lyrics.Offset != NULL)	//Disregard any current offset value
		{
			Lyrics.realoffset=0;
			free(Lyrics.Offset);
			Lyrics.Offset=NULL;
		}

		if(Lyrics.out_format == VRHYTHM_FORMAT)
		{
			Lyrics.outputtrack=AnalyzeVrhythmID(vrhythmid);		//Determine the output MIDI track (instrument) name and note range (difficulty), so the output track can be omitted from the source MIDI copy
			if(Lyrics.outputtrack == NULL)	//Validate return value
			{
				printf("Error: Invalid output vocal rhythm identifier \"%s\"\nAborting\n",vrhythmid);
				exit_wrapper(4);
			}
		}
		if(Lyrics.out_format == MIDI_FORMAT)
			if(Lyrics.outputtrack == NULL)							//If no custom output track name was provided
				Lyrics.outputtrack=DuplicateString("PART VOCALS");	//Write track name as PART VOCALS by default

		if(Lyrics.srcfilename != NULL)	//If a source MIDI was provided for export
		{
			srcfile=fopen_err(Lyrics.srcfilename,"rb");	//This is the source midi to read from

			if(Lyrics.verbose)	printf("Loading timing from specified source file \"%s\"\n",Lyrics.srcfilename);
			Lyrics.quick=1;								//Should be fine to skip everything except loading basic track info
			MIDI_Load(srcfile,NULL,0);					//Call MIDI_Load with no handler (just load MIDI info) Lyric structure is NOT re-init'd- it's already populated

			if(!Lyrics.srcoffsetoverride)				//If the offset for the source file is not being manually specified
			{
				if(Lyrics.verbose)	puts("Loading source MIDI's song.ini to obtain offset, all other tags will be ignored");
				Parse_Song_Ini(Lyrics.srcfilename,1,0);	//Load ONLY the offset tag from the song.ini file, it will be stored negative from the contents of song.ini, so add to timestamps instead of subtract
			}
			else
				Lyrics.realoffset=Lyrics.srcrealoffset;	//Otherwise use the offset specified via command line

			if(Lyrics.realoffset != 0)
			{	//If there is an offset to apply to the source timing
				if(Lyrics.verbose)	printf("Applying additive offset of %ld from source MIDI file's song.ini\n",Lyrics.realoffset);
				for(lineptr=Lyrics.lines;lineptr!=NULL;lineptr=lineptr->next)	//For each line of lyrics
				{
					for(curpiece=lineptr->pieces;curpiece!=NULL;curpiece=curpiece->next)	//For each lyric in the line
					{
						if((long)curpiece->start < Lyrics.realoffset)
						{
							printf("Error: Offset in source MIDI is larger than lyric timestamp.\nAborting\n");
							exit_wrapper(5);
						}
						if(Lyrics.verbose>=2)	printf("\tLyric \"%s\"\tRealtime: %lu\t->\t%lu\n",curpiece->lyric,curpiece->start,curpiece->start+Lyrics.realoffset);
						curpiece->start+=Lyrics.realoffset;
					}
				}
			}

			if(Lyrics.verbose>=2)	putchar('\n');
			Copy_Source_MIDI(srcfile,outf);	//Copy all tracks besides destination vocal track to output file
			fclose_err(srcfile);
			srcfile=NULL;
		}
		else											//Assume default tempo
			Write_Default_Track_Zero(outf);				//Write up to the end of track 0 for a default MIDI and configure MIDI variables
	}


//Open the output file for text/binary writing and export to the specified format
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
			pitchedlyrics=fopen_err(Lyrics.dstlyrname,"wt");	//Pitched lyrics is a text format
			Export_Vrhythm(outf,pitchedlyrics,vrhythmid);
			fflush_err(pitchedlyrics);	//Commit any pending pitched lyric writes to file
			fclose_err(pitchedlyrics);	//Close pitched lyric file
			pitchedlyrics=NULL;
		break;

		case SKAR_FORMAT:	//Export as Soft Karaoke.  Default export track is "Words"
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

		case ID3_FORMAT:	//Export as MP3 with embedded lyrics
			srcfile=fopen_err(Lyrics.srcfilename,"rb");	//This is the source MP3 to read from
			outf=fopen_err(Lyrics.outfilename,"wb");	//MP3 is a binary format
			Export_ID3(srcfile,outf);
		break;

		default:
			puts("Unexpected error in export switch\nAborting");
			exit_wrapper(4);
		break;
	}

	if(midi_based_export)
	{	//Update the MIDI header to reflect the number of MIDI tracks written to file for all applicable export formats
		fseek_err(outf,10,SEEK_SET);		//The number of tracks is 10 bytes in from the start of the file header
		fputc_err(MIDIstruct.trackswritten>>8,outf);
		fputc_err(MIDIstruct.trackswritten&0xFF,outf);
	}

//Commit any pending writes to file and close input and output files
	if(Lyrics.verbose)	puts("\nCleaning up");
	if(Lyrics.verbose>=2)	puts("\tClosing files");
	if(srcfile != NULL)
	{
		fclose_err(srcfile);
		srcfile=NULL;
	}
	fflush_err(outf);
	fclose_err(outf);
	outf=NULL;
	fflush_err(stdout);

	free(backuplyric);
	backuplyric=NULL;
	ReleaseMemory(1);
	if(Lyrics.verbose)	puts("Success!");
	return 0;	//return success
}

void DisplaySyntax(void)
{
	printf("\n%s syntax:\nFoFLyricConvert [-offset #][-nohyphens [1|2|3]][-noplus][-marklines][-nolyrics]\n",PROGVERSION);
	puts(" [-grouping {word|line}][-verbose|debug][-filter [...]][-quick][-startstamp #]");
	puts(" [-bpm #][-brute][-help][-srcoffset #][-intrack TRACKNAME][-outtrack TRACKNAME]");
	puts(" [-notenames][-relative][-nopitch][-nosrctag [...]][-nofstyle]");
	puts(" { {-detect infile} | {-id3tag infile} |");
	puts("   {{-in [FORMAT] infile (lyr|ID)} {-out FORMAT (srcfile) outfile (lyr)(ID)}} }");
	puts("\nItems in {} are required, [] are optional, () are required depending on FORMAT");
}

void Input_failed(int errnum,char *argument)
{
	puts("\n");	//Add leading newlines
	if(errnum == 0xFF)
		puts("Error: Incorrect number of parameters");
	else if(errnum == 0xFE)
		puts("Error: Cannot import and export the same file");
	else if(errnum == 0xFD)
		puts("Error: Invalid numerical value");
	else if(errnum == 0xFC)
		puts("Error: The optional nohyphens flag must be 1, 2 or 3");
	else if(errnum == 0xFB)
		puts("Error: The \"nosrctag\" parameter can't be used for both wildcard and single ID3 frame omission");
	else if(errnum == 0xFA)
		puts("Error: The \"nosrctag\" parameter can't be used as a wildcard multiple times");
	else if(errnum == 0xF9)
		puts("Error: Track name expected");
	else if(errnum == 0xF8)
		puts("Error: The \"nofstyle\" parameter can only be used for MIDI export");
	else if(errnum == 0xF1)
		puts("Error: Subparameters were expected instead of a hyphen");
	else if(errnum == 0xF0)
		puts("Error: File name expected");
	else if(errnum == 0xEF)
		puts("Error: Each parameter can only be defined once");
	else if(errnum == 0xEE)
		puts("Error: Invalid character in filter string");
	else if(errnum == 0xED)
		puts("Error: The \"startstamp\" and \"offset\" parameters cannot be used simultaneously");
	else if(errnum == 0xEC)
		puts("Error: The \"quick\" parameter can only be used for MIDI import");
	else if(errnum == 0xEB)
		puts("Error: The \"bpm\" parameter can only be used for UltraStar or MIDI based export");
	else if(errnum == 0xEA)
		puts("Error: The \"brute\" parameter can only be used for UltraStar export");
	else if(errnum == 0xE0)
		puts("Error: The \"marklines\" parameter can only be used for Script export");
	else if(errnum == 0xE9)
		puts("Error: Vocal rhythm instrument+difficulty identifier is expected");
	else if(errnum == 0xE8)
		puts("Error: The \"nolyrics\" parameter can only be specified explicitly for MIDI/KAR import");
	else if(errnum == 0xE7)
		puts("Error: The \"verbose\" and \"debug\" parameters may not be used simultaneously");
	else if(errnum == 0xE6)
		puts("Error: The \"quick\" parameter is only valid for MIDI, KAR or Vrhythm import/export");
	else if(errnum == 0xE5)
		puts("Error: The \"srcoffset\" parameter is only valid when a source MIDI is provided");
	else if(errnum == 0xE4)
		puts("Error: The \"intrack\" parameter is only valid for MIDI or KAR import");
	else if(errnum == 0xE3)
		puts("Error: The \"outtrack\" parameter is only valid for MIDI or KAR export");
	else if(errnum == 0xE2)
		puts("Error: The \"notenames\" parameter is only valid for Vrhythm export");
	else if(errnum == 0xE1)
		puts("Error: The \"relative\" parameter is only valid for UltraStar export");
	else if(errnum == 0xD0)
		puts("Error: The \"outtrack\" parameter may not be used for Soft Karaoke export");
	else
	{
		printf("Parameter %d is incorrect\n",errnum);
		if(argument != NULL)	printf("(%s)\n",argument);
		puts("");
	}

	DisplaySyntax();
	puts("\nUse FoFLyricConvert -help for detailed help");

	exit_wrapper(1);
}

void DisplayHelp(void)
{
//Describe general syntax and required parameters
	DisplaySyntax();
	puts("-The use of detect will halt a conversion specified by the other parameters");
	puts("-Valid FORMATs are script,vl,midi,ustar,lrc,elrc,vrhythm,kar,skar or id3");
	puts("-infile is the name of the input file, whose FORMAT is optional");
	puts("-outfile is the name of the file to create in the FORMAT preceding it");
	puts("-If the input format is vrhythm, a source rhythm MIDI file (infile)");
	puts(" containing the vocal timings must be specified, followed by the input pitched");
	puts(" lyrics file or vrhythm track ID.  Both files are unique to this format, ie:");
	puts("\tFoFLyricConvert -in vrhythm timing.mid lyrics.txt -out ustar vox.txt");
	puts("-(srcfile) may be used for MIDI based export formats for merging the lryics");
	puts(" with an existing file, keeping its timing intact.  For example:");
	puts("\tFoFLyricConvert -in vl myfile.vl -out midi notes.mid withlyrics.mid");
	puts("-If the output format is vrhythm, an output vocal rhythm file, pitched lyric");
	puts(" file and instrument/difficulty identifier must be specified, for example:");
	puts("\tFoFLyricConvert -in vl song.vl -out vrhythm rhythm.mid pitches.txt G4");
	printf("\nPress Enter for next screen");
	fflush_err(stdin);
	fgetc_err(stdin);	//wait for keypress

//Describe optional parameters
	puts("\n\n\tOPTIONAL PARAMETERS:");
	puts("-The offset parameter will subtract # milliseconds from the");
	puts(" beginning of all lyric timestamps.  Provide your song's delay value with this");
	puts("-The nohyphens flag will control how trailing hyphens are handled:");
	puts(" If 1 or 3, hyphens are not inserted.  If 2 or 3, hyphens in input are ignored");
	puts("-The noplus parameter will remove + lyric events from source");
	puts("-The marklines parameter retains line markings for script export");
	puts("-The nolyrics parameter loads lyrics from a MIDI or KAR input file, ignoring");
	puts(" the presence or absence of lyric text, just loading pitches and durations");
	puts("-The grouping parameter will control output lyric separation");
	puts("-Specify verbose or debug to log information to the screen");
	puts("-The filter parameter allows a defined set of characters to be");
	puts(" removed from end of lyric pieces.  If a character set isn't defined, ^=%#/ is");
	puts(" assumed.  The filter CAN'T contain space, alphanumerical, + or - characters");
	puts("-The quick parameter speeds up MIDI import");
	puts("-The startstamp parameter can be a substitute for offset by");
	puts(" overriding the starting timestamp, offsetting all following lyric timestamps");
	puts("-The bpm allows a custom tempo for exported ustar or MIDI based files");
	puts("-The brute parameter finds the best tempo for Ultrastar export");
	puts("-The srcoffset parameter allows the lyric timing for the source");
	puts(" file to be offset manually, such as to override what is read from song.ini");
	printf("\nPress Enter for next screen");
	fflush_err(stdin);
	fgetc_err(stdin);	//wait for keypress

//Describe notes
	puts("\n\n\tOPTIONAL PARAMETERS (cont'd):");
	puts("-The intrack and outtrack parameters allow the import or export MIDI track to");
	puts(" be manually defined (ie. \"PART HARM1\" for RB:Beatles MIDIs).");
	puts("-The notenames parameter causes note names to be written instead of note");
	puts(" numbers during Vrhythm export");
	puts("-The nosrctag parameter allows one or more ID3 frames from the source MP3 file");
	puts(" to be omitted from the output MP3 file.  For example: \"-nosrctag APIC TDAT\"");
	puts(" omits the Attached Picture and Date ID3 frames, if they exist in the source");
	puts(" MP3.  The artist, title and/or album frames are omitted from the source MP3 if");
	puts(" defined in the input file.  The SYLT frame is always omitted for ID3 export.");
	puts("\n\tNOTES:");
	puts("* Input (including the source midi) and output files may not be the same");
	puts("* Filenames may not begin with a hyphen (ie. \"-notes.mid\" is not accepted)");
	puts("* Parameters containing whitespace (ie. filenames) must be enclosed in");
	puts("  quotation marks");

	exit_wrapper(1);
}
