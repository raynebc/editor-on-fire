#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chartconvert.h"

#define ALLEGRO_USE_CONSOLE
#include <allegro.h>
#ifdef ALLEGRO_WINDOWS
	#include <winalleg.h>
#endif

#ifdef CCDEBUG
#include "..\memwatch.h"
#endif

char *midi_track_name[NUM_MIDI_TRACKS] = {"INVALID", "PART GUITAR", "PART GUITAR COOP", "PART BASS", "PART DRUMS", "PART RHYTHM", "PART KEYS", "PART GUITAR GHL", "PART BASS GHL"};
struct MIDIevent *midi_track_events[NUM_MIDI_TRACKS] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
char events_reallocated[NUM_MIDI_TRACKS] = {0};
clock_t start, end;

#ifdef ALLEGRO_WINDOWS
	char **utf_argv;
	wchar_t **utf_internal_argv;
	int utf_argc = 0;

	int build_utf8_argument_list(int argc, char **argv)
	{
		int i;
		wchar_t * cl = GetCommandLineW();
		utf_internal_argv = CommandLineToArgvW(cl, &utf_argc);
		utf_argv = malloc(utf_argc * sizeof(char *));

		for(i = 0; i < utf_argc; i++)
		{
			utf_argv[i] = malloc(1024 * sizeof(char));
			if(utf_argv[i] == NULL)
			{	//If the allocation failed
				while(i > 0)
				{	//Free all the previously allocated argument strings
					free(utf_argv[i - 1]);
					i--;
				}
				free(utf_argv);
				return 1;	//Return failure
			}
			memset(utf_argv[i], 0, 1024);
			(void) uconvert((char *)utf_internal_argv[i], U_UNICODE, utf_argv[i], U_UTF8, 4096);
		}

		return 0;	//Success
	}

	void destroy_utf8_argument_list(void)
	{
		if(utf_argv)
		{
			int ctr;
			for(ctr = 0; ctr < utf_argc; ctr++)
			{	//For each of the recreated command line argument strings
				free(utf_argv[ctr]);
			}
			free(utf_argv);
			utf_argv = NULL;
			utf_argc = 0;
		}
	}
#endif

int main(int argc, char *argv[])
{
	struct FeedbackChart *chart;
	char output_filename[PATH_WIDTH * 2] = {0};	//Allow for Unicode characters, which are two bytes each
	int ctr;
	char par_output_filename = 0, par_overwrite = 0, par_invalid = 0;
	char **effective_argv = NULL;


#ifdef ALLEGRO_WINDOWS
	if(build_utf8_argument_list(argc, argv))
	{	//If the argument list fails to be converted into Unicode
		(void) puts("Failed to parse command line arguments");
		par_invalid = 1;
	}
	else
	{
		effective_argv = utf_argv;
	}
#else
	effective_argv = argv;
#endif

	///Process parameters
	allegro_init();
	if(argc < 2)
		par_invalid = 1;	//Insufficient parameter count
	for(ctr = 2; (ctr < argc) && !par_invalid; ctr++)
	{	//For each parameter after the first, unless a problem with the parameters has been found
		if(effective_argv[ctr][0] != '-')
		{	//If the parameter doesn't begin with a hyphen, this is expected to be the output filename
			if(par_output_filename)
			{
				(void) puts("Cannot specify more than one output filename.");
				par_invalid = 1;
			}
			ustrncpy(output_filename, effective_argv[ctr], PATH_WIDTH);	//Use it verbatim
			par_output_filename = 1;
		}
		else
		{
			if(!strcmpi(effective_argv[ctr], "-o"))
			{	//If this is the overwrite switch
				par_overwrite = 1;
			}
			else
			{
				(void) printf("Unknown parameter \"%s\".\n", effective_argv[ctr]);
				par_invalid = 1;
			}
		}
	}
	if(par_invalid)
	{
		(void) puts("Usage:  chartconvert {input_filename.chart} [output_filename.mid] [-o]");
		(void) puts("\tIf no output filename, [input_filename].mid is assumed.");
		(void) puts("\tIf output filename exists, it isn't overwritten unless -o is specified.");
		#ifdef ALLEGRO_WINDOWS
			destroy_utf8_argument_list();
		#endif
		return 1;
	}
	if(!exists(effective_argv[1]))
	{
		(void) printf("Input file \"%s\" does not exist.  Aborting.\n", effective_argv[1]);
		#ifdef ALLEGRO_WINDOWS
			destroy_utf8_argument_list();
		#endif
		return 2;
	}
	if(!par_output_filename)
	{	//If a filename wasn't specified, input_filename.mid will be assumed
		(void) replace_extension(output_filename,  effective_argv[1], "mid", PATH_WIDTH);	//Use the input filename with a .mid extension
	}
	if(exists(output_filename) && !par_overwrite)
	{	//If the output file exists and the user didn't specify to overwrite it
		(void) printf("Output file \"%s\" already exists.  Aborting.\n", output_filename);
		#ifdef ALLEGRO_WINDOWS
			destroy_utf8_argument_list();
		#endif
		return 3;
	}


	///Import
	(void) printf("Importing file \"%s\".\n", effective_argv[1]);
	start = clock();
	chart = import_feedback(effective_argv[1]);
	if(!chart)
	{
		(void) puts("Import failed.  Aborting.");
		destroy_utf8_argument_list();
		#ifdef ALLEGRO_WINDOWS
			destroy_utf8_argument_list();
		#endif
		return 4;
	}
	#ifdef CCDEBUG
		(void) puts("\tFile imported.");
	#endif


	///Process
	(void) puts("Sorting items.");
	sort_chart(chart);
	(void) puts("Detecting HOPOs.");
	set_hopo_status(chart);
	(void) puts("Condensing slider phrases.");
	set_slider_status(chart);


	///Export
	(void) printf("Exporting file \"%s\".\n", output_filename);
	if(export_midi(output_filename, chart))
	{
		(void) puts("Export failed.  Aborting.");
		destroy_feedback_chart(chart);
		(void) delete_file(output_filename);
		(void) delete_file("temp");
		#ifdef ALLEGRO_WINDOWS
			destroy_utf8_argument_list();
		#endif
		return 5;
	}
	end = clock();
	#ifdef CCDEBUG
		(void) puts("\tFile exported.");
	#endif


	///Clean up
	(void) delete_file("temp");
	destroy_feedback_chart(chart);

#ifdef ALLEGRO_WINDOWS
	destroy_utf8_argument_list();
#endif

	(void) printf("Converted in %f seconds.\n", ((double)end - start) / (double) CLOCKS_PER_SEC);
    return 0;
}

struct FeedbackChart *import_feedback(const char *filename)
{
	PACKFILE *inf;
	char *filebuffer, *linebuffer;
	char linebuffer2[1024] = {0};
	unsigned long linectr = 0, index, index2;
	char BOM[] = {0xEF, 0xBB, 0xBF};
	char error = 0;
	unsigned char currentsection = 0;	//Will be set to 1 for [Song], 2 for [SyncTrack], 3 for [Events], 4 for an instrument section or 0xFF for an unrecognized section
	char songparsed = 0, syncparsed = 0, eventsparsed = 0;	//Tracks whether each of these sections has already been parsed
	char *substring, *substring2;		//Used for string searches
	char *string1 = NULL, *string2 = NULL;	//Used to hold strings parsed with Read_db_string()
	unsigned long gemcount = 0;			//Used to count and log the number of gems parsed for each instrument section
	unsigned long eventcount = 0;		//Used to count and log the number of events parsed
	int errorstatus = 0;				//Records any error code issued by parse_long_int()
	unsigned long A = 0, B = 0, C = 0;	//The first, second and third integer values read from the current line of the file
	char anchortype = 0;				//The anchor type being read in [SyncTrack]
	char notetype = 0;					//The note type being read in the instrument track

//Feedback chart structure variables
	struct FeedbackChart *chart;
	struct dbAnchor *curanchor = NULL;		//Conductor for the anchor linked list
	struct dbText *curevent = NULL;			//Conductor for the text event linked list
	struct dbNote *curnote = NULL;			//Conductor for the current instrument track's note linked list
	struct dbTrack *curtrack = NULL;		//Conductor for the instrument track linked list, which contains a linked list of notes
	void *temp = NULL;						//Temporary pointer used for storing newly-allocated memory

	//Open file
	if(!filename)
	{
		(void) puts("\tLogic error:  Invalid input file.");
		return NULL;	//Invalid parameter
	}
	inf = pack_fopen(filename, "rt");
	if(!inf)
	{	//If the file failed to be opened
		(void) printf("\tError opening input file:  %s", strerror(errno));	//Get the Operating System's reason for the failure
		return NULL;
	}

	//Allocate and initialize Feedback structure
	chart = (struct FeedbackChart *)malloc(sizeof(struct FeedbackChart));
	if(!chart)
	{	//If the memory couldn't be allocated
		(void) printf("\tError allocating memory to store parsed chart content.");
		return NULL;	//Return error
	}
	memset(chart, 0, sizeof(struct FeedbackChart));
	filebuffer = (char *) buffer_file(filename, 1);
	if(!filebuffer)
	{
		(void) puts("\tError buffering input file into memory.");
		error = 1;
	}
	#ifdef CCDEBUG
		(void) printf("\tBuffered input file \"%s\" into memory.\n", filename);
	#endif

	//Parse the file
	while(!error)
	{	//Until an error occurs
		linectr++;
		index = 0;	//Reset index counter to beginning

		//Read the next line from the input file
		if(linectr == 1)
		{	//The first line being read from the input file
			linebuffer = ustrtok(filebuffer, "\r\n");	//Initialize the tokenization and get the first tokenized line

			//Skip Byte Order Mark if one is found on the first line
			if((linebuffer[0] == BOM[0]) && (linebuffer[1] == BOM[1]) && (linebuffer[2] == BOM[2]))
			{
				#ifdef CCDEBUG
					(void) printf("\tSkipping byte order mark on line %lu.\n", linectr);
				#endif
				linebuffer[0] = linebuffer[1] = linebuffer[2] = ' ';	//Overwrite the BOM with spaces
			}
		}
		else
		{	//Subsequent lines
			linebuffer = ustrtok(NULL, "\r\n");	//Return the next tokenized line
		}
		if(!linebuffer)	//If a tokenized line of the file was not obtained
			break;

		//Skip leading whitespace
		while(linebuffer[index] != '\0')
		{
			if((linebuffer[index] != '\n') && (isspace((unsigned char)linebuffer[index])))
				index++;	//If this character is whitespace, skip to next character
			else
				break;
		}

		if((linebuffer[index] == '\0') || (linebuffer[index] == '{'))
		{	//If this line was empty, or contained characters we're ignoring
			continue;							//Skip ahead to the next line
		}

		//Process section header
		if(linebuffer[index]=='[')
		{	//If the line begins an open bracket, it identifies the start of a section
			substring2 = strchr(linebuffer,']');		//Find first closing bracket
			if(substring2 == NULL)
			{
				(void) printf("\tImport failed on line #%lu:  Malformed section header (no closing bracket).\n", linectr);
				error = 2;
				break;	//Exit while loop
			}

			if(currentsection != 0)				//If a section is already being parsed
			{
				(void) printf("\tImport failed on line #%lu:  Malformed file (section not closed before another begins).\n", linectr);
				error = 3;
				break;	//Exit while loop
			}

			substring = strcasestr_spec(linebuffer, "Song");	//Case insensitive search, returning pointer to after the match
			if(substring && (substring <= substring2))
			{	//If this line contained "Song" followed by "]"
				if(songparsed)
				{	//If a section with this name was already parsed
					(void) printf("\tImport failed on line #%lu:  Malformed file ([Song] section defined more than once).\n", linectr);
					error = 4;
					break;	//Exit while loop
				}
				songparsed = 1;
				currentsection = 1;	//Track that the [Song] section is being parsed
				#ifdef CCDEBUG
					(void) printf("\tIdentified [Song] section on line %lu.\n", linectr);
				#endif
			}
			else
			{	//Not a [Song] section
				substring = strcasestr_spec(linebuffer, "SyncTrack");
				if(substring && (substring <= substring2))
				{	//If this line contained "SyncTrack" followed by "]"
					if(syncparsed)
					{	//If a section with this name was already parsed
						(void) printf("\tImport failed on line #%lu:  Malformed file ([SyncTrack] section defined more than once).\n", linectr);
						error = 5;
						break;	//Exit while loop
					}
					syncparsed = 1;
					currentsection = 2;	//Track that the [SyncTrack] section is being parsed
					#ifdef CCDEBUG
						(void) printf("\tIdentified [SyncTrack] section on line %lu.\n", linectr);
					#endif
				}
				else
				{	//Not a [SyncTrack] section
					substring = strcasestr_spec(linebuffer, "Events");
					if(substring && (substring <= substring2))
					{	//If this line contained "Events" followed by "]"
						if(eventsparsed != 0)
						{	//If a section with this name was already parsed
							(void) printf("\tImport failed on line #%lu:  Malformed file ([Events] section defined more than once).\n", linectr);
							error = 6;
							break;	//Exit while loop
						}
						eventsparsed = 1;
						currentsection = 3;	//Track that the [Events] section is being parsed
						#ifdef CCDEBUG
							(void) printf("\tIdentified [Events] section on line %lu.\n", linectr);
						#endif
					}
					else
					{	//This is an instrument section
						gemcount = 0;	//Reset this counter
						temp = (void *)validate_db_instrument(linebuffer);
						if(temp == NULL)					//Not a valid Feedback instrument section name
						{
							(void) printf("Invalid instrument section \"%s\" on line #%lu.  Ignoring.\n", linebuffer, linectr);
							currentsection = 0xFF;	//This section will be ignored
						}
						else
						{	//This is a valid Feedback instrument section name
							currentsection = 4;

							//Create and insert instrument link in the instrument list
							if(chart->tracks == NULL)						//If the list is empty
							{
								chart->tracks = (struct dbTrack *)temp;		//Point head of list to this link
								curtrack = chart->tracks;					//Point conductor to this link
							}
							else
							{
								curtrack->next = (struct dbTrack *)temp;	//Conductor points forward to this link
								curtrack = curtrack->next;					//Point conductor to this link
							}

							//Initialize instrument link
							curnote = NULL;			//Reset the conductor for the track's note list

							chart->guitartypes |= curtrack->isguitar;	//Cumulatively track the presence of 5 and 6 lane guitar tracks
							chart->basstypes |= curtrack->isbass;		//Ditto for 5 and 6 lane bass tracks
							#ifdef CCDEBUG
								(void) printf("\tIdentified [%s] section on line %lu.\n", curtrack->trackname, linectr);
							#endif
						}
					}
				}//Not a [SyncTrack] section
			}//Not a [Song] section

			continue;				//Skip ahead to the next line
		}//If the line begins an open bracket, it identifies the start of a section

		//Process end of section
		if(linebuffer[index] == '}')	//If the line begins with a closing curly brace, it is the end of the current section
		{
			if(currentsection == 0)				//If no section is being parsed
			{
				(void) printf("Import failed on line #%lu:  Malformed file (unexpected closing curly brace).\n", linectr);
				error = 7;
				break;	//Exit while loop
			}
			else if(currentsection == 3)
			{
				(void) printf("\t\tParsed events section:  %lu events.\n", eventcount);
			}
			else if(currentsection == 4)
			{	//If this was an instrument track
				(void) printf("\t\tParsed instrument section \"%s\":  %lu gems.\n", curtrack->trackname, gemcount);
			}
			currentsection = 0;
			continue;							//Skip ahead to the next line
		}

		//Process normal line input
		if(currentsection != 0xFF)
		{	//If this section was identified and is to be processed
			substring = strchr(linebuffer, '=');		//Any line within the section is expected to contain an equal sign
			if(substring == NULL)
			{
				(void) printf("Import failed on line #%lu:  Malformed item (missing equal sign).\n", linectr);
				error = 8;
				break;	//Exit while loop
			}
		}

		//Process [Song]
		if(currentsection == 1)
		{
			if(read_db_string(linebuffer, &string1, &string2))
			{	//If a valid definition of (string) = (string) or (string) = "(string)" was found
				if(strcasecmp(string1,"Offset") == 0)
				{	//This is the offset tag
					chart->offset = atof(string2);
					#ifdef CCDEBUG
						(void) printf("\t\tChart defines an offset of %fs.\n", chart->offset);
					#endif
				}
				else if(!strcasecmp(string1, "Resolution"))
				{	//This is the resolution tag
					index2 = 0;	//Use this as an index for string2
					chart->resolution = (unsigned long)parse_long_int(string2, &index2, &errorstatus);	//Parse string2 as a number
					if(errorstatus)
					{	//If parse_long_int() failed
						(void) printf("Import failed on line #%lu:  Invalid chart resolution.\n", linectr);
						error = 9;
						break;	//Exit while loop
					}
					#ifdef CCDEBUG
						(void) printf("\t\tChart defines a resolution of %lu.\n", chart->resolution);
					#endif
				}

				free(string1);	//These strings are no longer used
				free(string2);
				string1 = string2 = NULL;
			}
		}//Process [Song]

		//Process [SyncTrack]
		else if(currentsection == 2)
		{	//# = ID # is expected
			//Load first number
			A = (unsigned long)parse_long_int(linebuffer, &index, &errorstatus);
			if(errorstatus)
			{	//If parse_long_int() failed
				(void) printf("Feedback import failed on line #%lu:  Invalid sync item position.\n", linectr);
				error = 10;
				break;	//Exit while loop
			}

			//Skip equal sign
			index = 0;	//Reset index, to begin parsing from the equal sign that was previously found
			if(substring[index++] != '=')	//Check the character and increment index
			{
				(void) printf("Import failed on line #%lu:  Logic error (lost equal sign).\n", linectr);
				error = 11;
				break;	//Exit while loop
			}

			//Skip whitespace and parse to event ID
			while(substring[index] != '\0')
			{
				if((substring[index] != '\n') && (isspace((unsigned char)substring[index])))
					index++;	//If this character is whitespace, skip to next character
				else
					break;
			}

			//Identify sync item type
			anchortype = toupper(substring[index]);	//Store as uppercase
			if((anchortype == 'A') || (anchortype == 'B'))
				index++;	//Advance to next whitespace character
			else if(anchortype == 'T')
			{
				if(substring[index + 1] != 'S')		//If the next character doesn't complete the anchor type
				{
					(void) printf("Import failed on line #%lu:  Invalid sync item type \"T%c\".\n", linectr, substring[index + 1]);
					error = 12;
					break;	//Exit while loop
				}
				index += 2;	//This anchor type is two characters instead of one
			}
			else
			{
				(void) printf("Import failed on line #%lu:  Invalid sync item type \"%c\".\n", linectr, anchortype);
				error = 12;
				break;	//Exit while loop
			}

		//Load second number
			B = (unsigned long)parse_long_int(substring, &index, &errorstatus);
			if(errorstatus)
			{	//If parse_long_int() failed
				(void) printf("Import failed on line #%lu:  Invalid sync item parameter.\n", linectr);
				error = 13;
				break;	//Exit while loop
			}

			//If this is a time signature change, check for a second parameter, which Moonscraper uses to define the denominator (the power to which 2 is raised)
			if(anchortype == 'T')
			{
				C = (unsigned long)parse_long_int(substring, &index, &errorstatus);
				if(errorstatus)
				{	//If another number was not read on this line
					C = 2;	//Assume 4 (2^2 == 4)
					errorstatus = 0;	//Reset this so it isn't nonzero after the next successful call to parse_long_int
				}
				C = 1 << C;
			}

			//If this anchor event has the same chart time as the last, just write this event's information with the last's
			if(curanchor && (curanchor->chartpos == A))
			{
				if(anchortype == 'A')		//If this is an Anchor event
				{
					curanchor->usec = B;	//Store the anchor realtime position
					#ifdef CCDEBUG
						(void) printf("\t\tAnchor:  Delta pos:  %lu,  Realtime pos:  %lu.\n", A, B);
					#endif
				}
				else if(anchortype == 'B')	//If this is a Tempo event
				{
					curanchor->BPM = B;		//Store the tempo
					#ifdef CCDEBUG
						(void) printf("\t\tTempo change:  Delta pos:  %lu,  %lu.%lu BPM.\n", A, (B / 1000), (B % 1000));
					#endif
				}
				else if(anchortype == 'T')	//If this is a Time Signature event
				{
					curanchor->TSN = B;		//Store the Time Signature
					curanchor->TSD = C;
					#ifdef CCDEBUG
						(void) printf("\t\tTime signature change:  Delta pos:  %lu,  TS %lu/%lu.\n", A, B, C);
					#endif
				}
				else
				{
					(void) printf("Import failed on line #%lu:  Invalid sync item type \"%c\".\n", linectr, anchortype);
					error = 14;
					break;	//Exit while loop
				}
			}
			else
			{
				//Create and insert anchor link into the anchor list
				temp = malloc(sizeof(struct dbAnchor));		//Allocate memory
				if(!temp)
				{
					(void) puts("Failed to allocate memory.");
					error = 15;
					break;	//Exit while loop
				}
				memset(temp, 0, sizeof(struct dbAnchor));
				if(chart->anchors == NULL)					//If the list is empty
				{
					chart->anchors = (struct dbAnchor *)temp;	//Point head of list to this link
					curanchor = chart->anchors;					//Point conductor to this link
				}
				else
				{
					if(curanchor == NULL)
					{
						(void) printf("Import failed on line #%lu:  Logic error (lost anchor linked list conductor).\n", linectr);
						error = 16;
						break;	//Exit while loop
					}
					curanchor->next = (struct dbAnchor *)temp;	//Conductor points forward to this link
					curanchor = curanchor->next;					//Point conductor to this link
				}

				//Initialize anchor link
				curanchor->chartpos = A;	//The first number read is the chart position
				switch(anchortype)
				{
					case 'A':					//If this was an anchor event
						curanchor->usec = B;	//Store the anchor's timestamp in microseconds
						#ifdef CCDEBUG
							(void) printf("\t\tAnchor:  Delta pos:  %lu,  Realtime pos:  %lu.\n", A, B);
						#endif
					break;

					case 'B':				//If this was a tempo event
						curanchor->BPM = B;	//The second number represents 1000 times the tempo
						#ifdef CCDEBUG
							(void) printf("\t\tTempo change:  Delta pos:  %lu,  %lu.%lu BPM.\n", A, (B / 1000), (B % 1000));
						#endif
					break;

					case 'T':				//If this was a time signature event
						curanchor->TSN = B;	//Store the numerator of the time signature
						curanchor->TSD = C;	//Store the denominator of the time signature
						#ifdef CCDEBUG
							(void) printf("\t\tTime signature change:  Delta pos:  %lu,  TS %lu/%lu.\n", A, B, C);
						#endif
					break;

					default:
						(void) printf("Import failed on line #%lu:  Invalid sync item type \"%c\".\n", linectr, anchortype);
						error = 17;
						break;	//Exit while loop
				}
			}
		}//Process [SyncTrack]

		//Process [Events]
		else if(currentsection == 3)
		{	//# = E "(STRING)" is expected
			//Load first number
			A = parse_long_int(linebuffer, &index, &errorstatus);
			if(errorstatus)
			{	//If parse_long_int() failed
				(void) printf("Import failed on line #%lu:  Invalid event time position.\n", linectr);
				error = 18;
				break;	//Exit while loop
			}

			//Skip whitespace and parse to equal sign
			index = 0;	//Reset index, to parse an int from after the equal sign
			while(substring[index] != '\0')
			{
				if((substring[index] != '\n') && (isspace((unsigned char)substring[index])))
					index++;	//If this character is whitespace, skip to next character
				else
					break;
			}

			//Skip equal sign
			if(substring[index++] != '=')	//Check the character and increment index
			{
				(void) printf("Import failed on line #%lu:  Malformed event item (missing equal sign).\n", linectr);
				error = 19;
				break;	//Exit while loop
			}

			//Skip whitespace and parse to event ID
			while(substring[index] != '\0')
			{
				if((substring[index] != '\n') && (isspace((unsigned char)substring[index])))
					index++;	//If this character is whitespace, skip to next character
				else
					break;
			}

			if(substring[index++] == 'E')
			{	//Check if this is a text event indicator (and increment index)
				//Seek to opening quotation mark
				while((substring[index] != '\0') && (substring[index] != '"'))
					index++;

				if(substring[index++] != '"')
				{	//Check if this was a null character instead of quotation mark (and increment index)
					(void) printf("Import failed on line #%lu:  Malformed event item (missing open quotation mark).\n", linectr);
					error = 20;
					break;	//Exit while loop
				}

				//Load string by copying all characters to the second buffer (up to the next quotation mark)
				linebuffer2[0] = '\0';	//Truncate string
				index2 = 0;			//Reset linebuffer2's index
				while(substring[index] != '"')
				{	//For all characters up to the next quotation mark
					if(substring[index] == '\0')
					{	//If a null character is reached unexpectedly
						(void) printf("Import failed on line #%lu:  Malformed event item (missing close quotation mark).\n", linectr);
						error = 21;
						break;	//Exit while loop
					}
					linebuffer2[index2++] = substring[index++];	//Copy the character to the second buffer, incrementing both indexes
				}
				linebuffer2[index2] = '\0';	//Truncate the second buffer to form a complete string

				//Create and insert event link into event list
				temp = malloc(sizeof(struct dbText));		//Allocate memory
				if(!temp)
				{
					(void) puts("Failed to allocate memory.");
					error = 22;
					break;	//Exit while loop
				}
				memset(temp, 0, sizeof(struct dbText));
				if(chart->events == NULL)					//If the list is empty
				{
					chart->events = (struct dbText *)temp;	//Point head of list to this link
					curevent = chart->events;				//Point conductor to this link
				}
				else
				{
					if(curevent == NULL)
					{
						(void) puts("Logic error:  Event link corrupt.");
						error = 23;
						break;	//Exit while loop
					}
					curevent->next = (struct dbText *)temp;	//Conductor points forward to this link
					curevent = curevent->next;				//Point conductor to this link
				}

				//Initialize event link- Duplicate linebuffer2 into a newly created dbText link, adding it to the list
				curevent->chartpos = A;							//The first number read is the chart position
				curevent->text = duplicate_string(linebuffer2);	//Copy linebuffer2 to new string and store in list
				eventcount++;
				#ifdef CCDEBUG
///					(void) printf("\t\tEvent:  Delta pos:  %lu, \"%s\"\n", A, curevent->text);
				#endif
			}
			else
			{	//This is not a recognized event entry
				(void) printf("\tIgnoring unrecognized event on line #%lu:  \"%s\"", linectr, linebuffer);
			}
		}//Process [Events]

	//Process instrument tracks
		else if(currentsection == 4)
		{	//"# = N # #" or "# = S # #" is expected
			//Load first number
			A = parse_long_int(linebuffer, &index, &errorstatus);
			if(errorstatus)
			{	//If parse_long_int() failed
				(void) printf("Import failed on line #%lu:  Invalid gem item position.\n", linectr);
				error = 24;
				break;	//Exit while loop
			}

		//Skip whitespace and parse to equal sign
			index = 0;	//Reset index, to parse an int from after the equal sign
			while(substring[index] != '\0')
			{
				if((substring[index] != '\n') && (isspace((unsigned char)substring[index])))
					index++;	//If this character is whitespace, skip to next character
				else
					break;
			}

		//Skip equal sign
			if(substring[index++] != '=')
			{	//Check the character and increment index
				(void) printf("Import failed on line #%lu:  Malformed gem item (missing equal sign).\n", linectr);
				error = 25;
				break;	//Exit while loop
			}

		//Skip whitespace and parse to event ID
			while(substring[index] != '\0')
			{
				if((substring[index] != '\n') && (isspace((unsigned char)substring[index])))
					index++;	//If this character is whitespace, skip to next character
				else
					break;
			}

			notetype = 0;	//By default, assume this is going to be a note definition
			switch(substring[index])
			{
				case 'S':			//Player/overdrive section
					notetype = 1;	//This is a section marker
					index++; 		//Increment index past N or S identifier
				break;

				case 'E':		//Text event
					if(strstr(&substring[index + 1], "*"))
					{	//If the E is followed by an asterisk
						notetype = 2;	//This is a toggle HOPO marker
					}
					else if(strstr(&substring[index + 1], "= E O"))
					{	//If the E is followed by the letter O
						char *check = strcasestr_spec(substring, "= E O");	//Obtain the next character after this sequence of characters
						if(check && (*check == '\0'))
						{	//If that's the sequence this line ended in
							notetype=3;	//This is an open strum marker
						}
						else
							continue;	//Otherwise ignore this event
					}
					else if(strcasestr_spec(substring, "= E soloend"))
					{
						notetype = 'E';	//This is a Clone Hero end of solo marker
					}
					else if(strcasestr_spec(substring, "= E solo"))
					{
						notetype = 'S';	//This is a Clone Hero start of solo marker
					}
					else
					{
						continue;
					}
				break;

				case 'N':		//Note indicator, and increment index
					index++; 	//Increment index past N or S identifier
				break;

				default:
					(void) printf("Import failed on line #%lu:  Invalid gem item type \"%c\".\n", linectr, substring[index]);
					error = 26;
				break;	//Exit while loop
			}

			if(notetype == 2)
			{	//If this is a toggle HOPO marker, treat it as a note authored as "N 5 0"
				notetype = 0;
				B = 5;
				C = 0;
			}
			else if(notetype == 3)
			{	//If this is an open strum marker, treat it as a note authored as "N 7 0"
				notetype = 0;
				B = 7;
				C = 0;
			}
			else if(notetype == 'S')
			{	//If this is a Clone Hero start of solo marker
				notetype = 0;
				B = 'S';
				C = 0;
			}
			else if(notetype == 'E')
			{	//If this is a Clone Hero end of solo marker
				notetype = 0;
				B = 'E';
				C = 0;
			}
			else
			{	//Otherwise parse the second and third parameters
				//Load second number
				B = parse_long_int(substring, &index, &errorstatus);
				if(errorstatus)
				{	//If parse_long_int() failed
					(void) printf("Import failed on line #%lu:  Invalid gem item parameter 1.\n", linectr);
					error = 27;
					break;	//Exit while loop
				}

			//Load third number
				C = parse_long_int(substring, &index, &errorstatus);
				if(errorstatus)
				{	//If parse_long_int() failed
					(void) printf("Import failed on line #%lu:  Invalid gem item parameter 2.\n", linectr);
					error = 28;
					break;	//Exit while loop
				}
			}

			if(notetype)
			{	//This was a player section marker
				if(B > 4)
				{	//Only values of 0, 1 or 2 are valid for player section markers, 3 and 4 are unknown but will be kept and ignored for now during transfer
					(void) printf("Import failed on line #%lu:  Invalid section marker #%lu.\n", linectr, B);
					error = 32;
					break;	//Exit while loop
				}
				B += '0';	//Store 0 as '0', 1 as '1' or 2 as '2', ...
			}

			if(curnote && (curnote->chartpos == A) && (curnote->gemcolor == B))
			{	//If this is a duplicate of the previous gem
				(void) printf("\t\tOmitting duplicate item at pos %lu.\n", A);
				continue;
			}

		//Create a note link and add it to the current Note list
			if(curtrack == NULL)
			{	//If the instrument track linked list is not initialized
				(void) printf("Import failed on line #%lu:  Malformed file (gem defined outside instrument track).\n", linectr);
				error = 29;
				break;	//Exit while loop
			}
			temp = malloc(sizeof(struct dbNote));		//Allocate memory
			if(!temp)
			{
				(void) puts("Failed to allocate memory.");
				error = 30;
				break;	//Exit while loop
			}
			memset(temp, 0, sizeof(struct dbNote));
			if(curtrack->notes == NULL)					//If the list is empty
			{
				curtrack->notes = (struct dbNote *)temp;	//Point head of list to this link
				curnote = curtrack->notes;					//Point conductor to this link
			}
			else
			{
				if(curnote == NULL)
				{
					(void) puts("Logic error:  Note link corrupt.");
					error = 31;
					break;	//Exit while loop
				}
				curnote->next = (struct dbNote *)temp;	//Conductor points forward to this link
				curnote = curnote->next;				//Point conductor to this link
			}

		//Initialize note link
			curnote->chartpos = A;	//The first number read is the chart position
			curnote->gemcolor = B;
			if((curnote->gemcolor <= 8) && (curnote->gemcolor != 5) && (curnote->gemcolor != 6))
			{	//If this gem is value 0-4, 7 or 8
				curnote->is_gem = 1;
			}
			curnote->duration = C;			//The third number read is the note duration
			if(curnote->duration == 0)		//Enforce a minimum note/marker duration of 1 delta
				curnote->duration = 1;
			gemcount++;						//Track the number of gems parsed and stored into the list
		}//Process instrument tracks

	//Error: Content in file outside of a defined section
		else if(currentsection != 0xFF)
		{	//If this isn't a section that is being ignored
			(void) printf("Import failed on line #%lu:  Malformed file (item defined outside of track).\n", linectr);
			error = 33;
			break;	//exit while loop
		}
	}//Until an error occurs


	//Cleanup
	if(error)
	{	//If the input file did not import
		destroy_feedback_chart(chart);
		chart = NULL;
	}
	free(filebuffer);

	return chart;
}

void *buffer_file(const char * fn, char appendnull)
{
	void *data = NULL;
	PACKFILE *fp = NULL;
	size_t filesize, buffersize;

	if(fn == NULL)
	{
		(void) puts("\tLogic error.  Aborting.");
		return NULL;	//Invalid parameter
	}
	fp = pack_fopen(fn, "r");
	if(fp == NULL)
	{	//If the file failed to be opened
		(void) puts("Error opening input file.  Aborting.");
		return NULL;	//Couldn't open file
	}
	filesize = buffersize = file_size_ex(fn);
	if(appendnull)
	{	//If adding an extra NULL byte of padding to the end of the buffer
		buffersize++;	//Allocate an extra byte for the padding
	}
	data = (char *)malloc(buffersize);
	if(data == NULL)
	{
		(void) puts("Cannot allocate memory for buffer.");
		(void) pack_fclose(fp);
		return NULL;
	}
	(void) pack_fread(data, (long)filesize, fp);
	if(appendnull)
	{	//If adding an extra NULL byte of padding to the end of the buffer
		((char *)data)[buffersize - 1] = 0;	//Write a 0 byte at the end of the buffer
	}
	(void) pack_fclose(fp);

	return data;
}

char *duplicate_string(const char *str)
{
	char *ptr = NULL;
	size_t size=0;

	if(str == NULL)
		return NULL;

	size = strlen(str);
	ptr = malloc(size + 1);
	if(!ptr)
		return NULL;

	strcpy(ptr, str);
	return ptr;
}

void destroy_feedback_chart(struct FeedbackChart *ptr)
{
	struct dbAnchor *anchorptr;		//Conductor for the anchors linked list
	struct dbText *eventptr;		//Conductor for the events linked list
	struct dbTrack *trackptr;		//Conductor for the tracks linked list
	struct dbNote *noteptr;			//Conductor for the notes linked lists
	unsigned long track_ctr;

	if(!ptr)
	{
		return;
	}

//Empty anchors list
	while(ptr->anchors != NULL)
	{
		anchorptr = ptr->anchors->next;	//Store link to next anchor
		free(ptr->anchors);				//Free current anchor
		ptr->anchors = anchorptr;		//Point to next anchor
	}

//Empty events list
	while(ptr->events != NULL)
	{
		eventptr = ptr->events->next;	//Store link to next event
		free(ptr->events->text);		//Free event text
		free(ptr->events);				//Free current event
		ptr->events = eventptr;			//Point to next event
	}

//Empty tracks list
	while(ptr->tracks != NULL)
	{
		trackptr = ptr->tracks->next;	//Store link to next instrument track

		while(ptr->tracks->notes != NULL)
		{
			noteptr = ptr->tracks->notes->next;	//Store link to next note
			free(ptr->tracks->notes);			//Free current note
			ptr->tracks->notes = noteptr;		//Point to next note
		}

		free(ptr->tracks->trackname);	//Free track name
		free(ptr->tracks);				//Free current track
		ptr->tracks = trackptr;			//Point to next track
	}

//Empty MIDI event linked lists
	for(track_ctr = 1; track_ctr < NUM_MIDI_TRACKS; track_ctr++)
	{	//For each of the instrument tracks that may have been built
		struct MIDIevent *ptr, *nextptr;

		if(events_reallocated[track_ctr])
		{	//If the entire linked list was rebuilt into a single allocated chunk of memory
			ptr = midi_track_events[track_ctr];
			while(ptr != NULL)
			{	//While there are events imported for this instrument track
				if(ptr->dp)
				{	//If there was memory allocated for this event
					free(ptr->dp);		//Free it
				}
				ptr = ptr->next;
			}
			free(midi_track_events[track_ctr]);	//Only one call to free() is needed for the links
		}
		else
		{	//Otherwise each link needs to be freed individually
			ptr = midi_track_events[track_ctr];
			while(ptr != NULL)
			{	//While there are events imported for this instrument track
				nextptr = ptr->next;	//Store link to next event
				if(ptr->dp)
				{	//If there was memory allocated for this event
					free(ptr->dp);		//Free it
				}
				free(ptr);				//Free current event
				ptr = nextptr;			//Point to next event
			}
		}
	}

	free(ptr);
}

char *strcasestr_spec(char *str1,const char *str2)
{	//Performs a case INSENSITIVE search of str2 in str1, returning the character AFTER the match in str1 if it exists, else NULL
	char *temp1=NULL;	//Used for string matching
	const char *temp2=NULL;

	while(*str1 != '\0')	//Until end of str1 is reached
	{
		if(tolower(*str1) == tolower(*str2))	//If the first character in both strings match
		{
			for(temp1=str1,temp2=str2;((*temp1 != '\0') && (*temp2 != '\0'));temp1++,temp2++)
			{	//Compare remaining characters in str2 with those starting at current index into str1
				if(tolower(*temp1) != tolower(*temp2))
					break;	//string mismatch, break loop and keep looking
			}
			if((*temp2) == '\0')	//If all characters in str2 were matched
				return temp1;		//Return pointer to character after the match in str1
		}
		str1++;	//Increment index into string
	}

	return NULL;	//No match was found, return NULL
}

struct dbTrack *validate_db_instrument(char *buffer)
{
	unsigned long index = 0;			//Used to index into buffer
	char *endbracket = NULL;			//The pointer to the end bracket
	char *diffstring = NULL;			//Used to find the difficulty substring
	char *retstring = NULL;				//Used to create the instrument track string
	struct dbTrack *track = NULL;		//Used to create the structure that is returned
	char difftype = 0;					//Used to track the difficulty of the track

	if(buffer == NULL)
		return NULL;	//Return error

//Validate the opening bracket, to which buffer is expected to point
	if(buffer[0] != '[')	//If the opening bracket is missing
		return NULL;	//Return error

//Validate the presence of the closing bracket
	endbracket = strchr(buffer, ']');
	if(endbracket == NULL)
		return NULL;	//Return error

//Verify that no non whitespace characters exist after the closing bracket
	index = 1;	//Reset index to point to the character after the closing bracket
	while(endbracket[index] != '\0')
		if(!isspace(endbracket[index++]))	//Check if this character isn't whitespace (and increment index)
			return NULL;			//If it isn't whitespace, return error

//Truncate whitespace after closing bracket
	endbracket[1] = '\0';	//Write a NULL character after the closing bracket

//Verify that a valid diffulty is specified, seeking past the opening bracket pointed to by buffer[0]
	//Test for Easy
	diffstring = strcasestr_spec(&buffer[1], "Easy");
	if(diffstring == NULL)
	{
	//Test for Medium
		diffstring = strcasestr_spec(&buffer[1], "Medium");
		if(diffstring == NULL)
		{
	//Test for Hard
			diffstring = strcasestr_spec(&buffer[1], "Hard");
			if(diffstring == NULL)
			{
	//Test for Expert
				diffstring = strcasestr_spec(&buffer[1], "Expert");
				if(diffstring == NULL)	//If none of the four valid difficulty strings were found
					return NULL;	//Return error

				difftype=4;	//Track that this is an Expert difficulty
			}
			else
				difftype=3;	//Track that this is a Hard difficulty
		}
		else
			difftype=2;	//Track that this is a Medium difficulty
	}
	else
		difftype=1;	//Track that this is is an Easy difficulty

//Create and initialize the instrument structure
	track = malloc(sizeof(struct dbTrack));		//Allocate memory
	if(!track)
	{	//If the memory couldn't be allocated
		(void) printf("\tError allocating memory to store parsed instrument track content.");
		return NULL;	//Return error
	}
	memset(track, 0, sizeof(struct dbTrack));

//At this point, diffstring points to the character AFTER the matching difficulty string.  Verify that a valid instrument is specified
	if(!validate_db_track_diff_string(diffstring, track))
	{	//If the instrument difficulty is not recognized
		free(track);
		return NULL;	//Return error
	}

//Create a new string containing the instrument name, minus the brackets
	retstring = duplicate_string(&buffer[1]);
	if(!retstring)
	{
		free(track);
		return NULL;	//Return error
	}
	retstring[strlen(retstring)-1] = '\0';	//Truncate the trailing bracket

	track->trackname = retstring;			//Store the instrument track name
	track->difftype = difftype;
	return track;
}

int validate_db_track_diff_string(char *diffstring, struct dbTrack *track)
{
	char *ptr;

	if(!diffstring || !track)
		return 0;	//Invalid parameters

	//Initialize chart variables
	track->tracktype = 0;	//Unless specified otherwise, this track will not import
	track->isguitar = track->isdrums = track->isbass = 0;

	//Check for supported instrument difficulties
	ptr = strcasestr_spec(diffstring, "Single]");
	if(ptr && (ptr[0] == '\0'))
	{	//If the string ends in "Single]"
		track->tracktype = 1;	//Track that this is a "Guitar" track
		track->isguitar = 1;
		return 1;	//Return success
	}

	ptr = strcasestr_spec(diffstring, "DoubleGuitar]");
	if(ptr && (ptr[0] == '\0'))
	{	//If the string ends in "DoubleGuitar]"
		track->tracktype = 2;	//Track that this is a "Lead Guitar" track
		track->isguitar = 1;
		return 1;	//Return success
	}

	ptr = strcasestr_spec(diffstring, "DoubleBass]");
	if(ptr && (ptr[0] == '\0'))
	{	//If the string ends in "DoubleBass]"
		track->tracktype = 3;	//Track that this is a "Bass" track
		track->isbass = 1;
		return 1;	//Return success
	}

	ptr = strcasestr_spec(diffstring, "Drums]");
	if(ptr && (ptr[0] == '\0'))
	{	//If the string ends in "Drums]"
		track->tracktype = 4;	//Track that this is a "Drums" track
		track->isdrums = 1;
		return 1;	//Return success
	}

	ptr = strcasestr_spec(diffstring, "DoubleRhythm]");
	if(ptr && (ptr[0] == '\0'))
	{	//If the string ends in "DoubleRhythm]"
		track->tracktype = 5;	//Track that this is a "Rhythm" track
		track->isguitar = 1;
		return 1;	//Return success
	}

	ptr = strcasestr_spec(diffstring, "Keyboard]");
	if(ptr && (ptr[0] == '\0'))
	{	//If the string ends in "Keyboard]"
		track->tracktype = 6;	//Track that this is a keyboard track
		return 1;	//Return success
	}

	//Check for 6 lane tracks, authored by MoonScraper for use in Clone Hero
	ptr = strcasestr_spec(diffstring, "GHLGuitar]");
	if(ptr && (ptr[0] == '\0'))
	{	//If the string ends in "GHLGuitar]"
		track->tracktype = 7;	//Track that this is a "Guitar" track
		track->isguitar = 2;	//This signals that this is a 6 lane guitar track
		return 1;	//Return success
	}

	ptr = strcasestr_spec(diffstring, "GHLBass]");
	if(ptr && (ptr[0] == '\0'))
	{	//If the string ends in "GHLBass]"
		track->tracktype = 8;	//Track that this is a "Bass" track
		track->isbass = 2;		//This signals that this is a 6 lane bass track
		return 1;	//Return success
	}

	//Check for other instrument difficulties that are valid but not supported
	ptr = strcasestr_spec(diffstring, "SingleGuitar]");
	if(ptr && (ptr[0] == '\0'))
	{	//If the string ends in "SingleGuitar]"
		track->isguitar = 1;
		return 1;	//Return success
	}

	ptr = strcasestr_spec(diffstring, "SingleRhythm]");
	if(ptr && (ptr[0] == '\0'))
	{	//If the string ends in "SingleRhythm]"
		track->isguitar = 1;
		return 1;	//Return success
	}

	ptr = strcasestr_spec(diffstring, "DoubleDrums]");
	if(ptr && (ptr[0] == '\0'))
	{	//If the string ends in "DoubleDrums]"
		track->isdrums = 2;	//This will be imported into the Phase Shift drum track instead of the normal drum track
		return 1;	//Return success
	}

	ptr = strcasestr_spec(diffstring, "Vocals]");
	if(ptr && (ptr[0] == '\0'))
	{	//If the string ends in "Vocals]"
		return 1;	//Return success
	}

	ptr = strcasestr_spec(diffstring, "EnhancedGuitar]");
	if(ptr && (ptr[0] == '\0'))
	{	//If the string ends in "EnhancedGuitar]"
		track->isguitar = 1;	//EnhancedGuitar is a guitar track, but it won't be imported
		return 1;	//Return success
	}

	ptr = strcasestr_spec(diffstring, "CoopLead]");
	if(ptr && (ptr[0] == '\0'))
	{	//If the string ends in "CoopLead]"
		track->isguitar = 1;	//CoopLead is a guitar track, but it won't be imported
		return 1;	//Return success
	}

	ptr = strcasestr_spec(diffstring, "CoopBass]");
	if(ptr && (ptr[0] == '\0'))
	{	//If the string ends in "CoopBass]"
		track->isguitar = 1;	//CoopBass is a guitar track, but it won't be imported
		return 1;	//Return success
	}

	ptr = strcasestr_spec(diffstring, "10KeyGuitar]");
	if(ptr && (ptr[0] == '\0'))
	{	//If the string ends in "10KeyGuitar]"
		track->isguitar = 1;	//10KeyGuitar is a guitar track, but it won't be imported
		return 1;	//Return success
	}

	return 0;	//Return failure
}

int read_db_string(char *source, char **str1, char **str2)
{
	unsigned long srcindex;	//Index into source string
	unsigned long index;	//Index into destination strings
	char *string1, *string2;	//Working strings
	char findquote = 0;	//Boolean:	The element to the right of the equal sign began with a quotation mark
						//			str2 will stop filling with characters when the next quotation mark is read

	if(!source || !str1 || !str2)
	{
		return 0;	//return error
	}

//Allocate memory for strings
	string1 = malloc(strlen(source) + 1);
	string2 = malloc(strlen(source) + 2);
	if(!string1 || !string2)
	{	//If either allocation failed
		if(string1)
			free(string1);
		if(string2)
			free(string2);
		return 0;	//Return error
	}

//Parse the string to the left of the expected equal sign into string1
	index = 0;
	for(srcindex = 0; source[srcindex] != '='; srcindex++)	//Parse characters until equal sign is found
	{
		if(source[srcindex] == '\0')	//If the string ended unexpectedly
		{
			free(string1);
			free(string2);
			return 0;		//return error
		}

		string1[index++] = source[srcindex];	//Append character to string1 and increment index into string
	}
	string1[index] = '\0';	//Truncate string1
	srcindex++;		//Seek past the equal sign
	string1 = truncate_string(string1, 1);	//Re-allocate string to remove leading and trailing whitespace

//Skip leading whitespace
	while(source[srcindex] != '\0')
	{
		if(!isspace(source[srcindex]))	//Character is not whitespace and not NULL character
		{
			if(source[srcindex] == '"')	//If the first character after the = is found to be a quotation mark
			{
				srcindex++;			//Seek past the quotation mark
				findquote=1;		//Expect another quotation mark to end the string
			}

			break;	//Exit while loop
		}
		srcindex++;	//Increment to look at next character
	}

//Parse the string to the right of the equal sign into string2
	index = 0;
	while(source[srcindex] != '\0')	//There was nothing but whitespace after the equal sign
	{
		if(findquote && (source[srcindex] == '"'))	//If we should stop at this quotation mark
			break;
		string2[index++] = source[srcindex++];	//Append character to string1 and increment both indexes
	}

	string2[index] = '\0';	//Truncate string2

	if(index)	//If there were characters copied to string2
		string2 = truncate_string(string2, 1);	//Re-allocate string to remove the trailing whitespace

	*str1 = string1;	//Return string1 through pointer
	*str2 = string2;	//Return string2 through pointer
	return 1;			//Return success
}

long int parse_long_int(char *buffer, unsigned long *startindex, int *errorstatus)
{
	unsigned long index;
	char buffer2[11] = {0};		//An array large enough to hold a 10 digit numerical string
	unsigned long index2 = 0;	//index within buffer2
	long value = 0;				//The converted long int value of the numerical string

	if(!buffer || !startindex)
	{
		if(errorstatus != NULL)
			*errorstatus = 1;	//Invalid parameters
		return 0;
	}
	index = *startindex;	//Dereference starting index for ease of use

	//Ignore leading whitespace, seeking to first numerical character (leading 0's are discarded)
	while(buffer[index] != '\0')
	{
		if((buffer[index] == '0') && (isdigit((unsigned char)buffer[index + 1])))	//If this is a leading zero
			index++;					//skip it
		else
		{
			if(isdigit((unsigned char)buffer[index]))	//If this is a numerical character (but not a leading 0)
				break;			//exit loop

			if(buffer[index] == '-')					//If this is a negative sign
				break;			//exit loop

			if(isspace((unsigned char)buffer[index]))	//If this is a white space character
				index++;		//skip it
			else
			{
				if(errorstatus != NULL)
					*errorstatus = 2;	//Invalid character
				return 0;
			}
		}
	}

//Parse the numerical characters into buffer2
	if(buffer[index] == '\0')
	{
		if(errorstatus != NULL)
			*errorstatus = 3;	//Unexpected end of line encountered
		return 0;
	}

	while(isspace((unsigned char)buffer[index]) == 0)
	{	//Until whitespace or NULL character is found
		if(isdigit((unsigned char)buffer[index]))	//If this is a numerical character
			buffer2[index2++]=buffer[index];	//Store it, move index2 forward
		else if(buffer[index] == '-')			//if this is a negative sign
			buffer2[index2++]=buffer[index];	//Store it, move index2 forward
		else
		{	//is not a numerical character
			if(buffer[index]=='\0')	//If the end of the string in buffer is reached
				break;

			if(errorstatus != NULL)
				*errorstatus = 4;	//Non-numerical character
			return 0;
		}

		if(index2 > 10)
		{	//We only are accounting for a numerical string 10 characters long
			if(errorstatus != NULL)
				*errorstatus = 5;	//Number too long
			return 0;
		}

		index++;	//Look at next character
	}
	buffer2[index2]='\0';		//Null terminate parsed numerical string
	(*startindex)=index;		//Pass our indexed position of buffer back into the passed variable

//Convert numerical string to numerical value
	if(buffer2[0] == '0')	//If the number is actually zero
		value = 0;
	else
	{
		value = atol(buffer2);	//Convert string to int
		if(value == 0)
		{
			if(errorstatus != NULL)
				*errorstatus = 6;	//Number conversion error
			return 0;
		}
	}

	return value;
}

char *truncate_string(char *str, char dealloc)
{
	unsigned long ctr = 0;
	char *newstr = NULL;
	char *originalstr = str;

	if(!str)
		return NULL;	//Invalid parameter

//Truncate leading whitespace
	while(str[0] != '\0')					//Until NULL terminator is reached
		if(isspace((unsigned char)str[0]) == 0)	//If this is a non whitespace character
			break;					//break from loop
		else
			str++;					//Drop whitespace from start of string

//Parse the string backwards to truncate trailing whitespace
//Note, isspace() seems to detect certain characters (ie. accented chars) incorrectly
//unless the input character is casted to unsigned char?
	for(ctr = (unsigned long)strlen(str); ctr > 0; ctr--)
		if(isspace((unsigned char)str[ctr - 1]))	//If this character is whitespace
			str[ctr - 1]='\0';		//truncate string to drop the whitespace
		else		//If this isn't whitespace
			break;	//Stop parsing

	newstr = duplicate_string(str);
	if(dealloc)
		free(originalstr);
	return newstr;	//Return new string
}

int write_word_be(unsigned short data, PACKFILE *outf)
{
	if(!outf)
		return 1;	//Error

	if(pack_putc(data >> 8, outf) == EOF)	//If the most significant byte could not be written
		return 1;	//Error
	if(pack_putc(data & 0xFF, outf) == EOF)	//If the least significant byte could not be written
		return 1;	//Error

	return 0;	//Success
}

int write_dword_be(unsigned long data, PACKFILE *outf)
{
	if(!outf)
		return 1;	//Error

	if(pack_putc((data >> 24) & 0xFF, outf) == EOF)	//If the most significant byte could not be written
		return 1;	//Error
	if(pack_putc((data >> 16) & 0xFF, outf) == EOF)	//If the next most significant byte could not be written
		return 1;	//Error
	if(pack_putc((data >> 8) & 0xFF, outf) == EOF)	//If the next most significant byte could not be written
		return 1;	//Error
	if(pack_putc(data & 0xFF, outf) == EOF)		//If the least significant byte could not be written
		return 1;	//Error

	return 0;	//Success
}

int write_var_length(unsigned long data, PACKFILE *outf)
{
	unsigned long buffer;
	buffer = data & 0x7F;

	if(!outf)
		return 1;	//Error
	if(data > 0x0FFFFFFF)
		return 1;	//Error, MIDI cannot write a variable length value larger than 0x0FFFFFFF

	while((data >>= 7))
	{
		buffer <<= 8;
		buffer |= 0x80;
		buffer += (data & 0x7F);
	}

	while(1)
	{
		if(pack_putc(buffer, outf) == EOF)	//If the next byte could not be written
			return 1;		//Error

		if(buffer & 0x80)
		{
			buffer >>= 8;
		}
		else
		{
			break;
		}
	}

	return 0;	//Success
}

int export_midi(const char *filename, struct FeedbackChart *chart)
{
	PACKFILE *outf;
	PACKFILE *tempf;	//Each track must state its size, but Allegro file I/O functions don't allow seeking, so track data must be written to a temporary file so its size can be determined
	char temp_filename[PATH_WIDTH] = {0};
	struct dbTrack *track_ptr;
	struct dbAnchor *anchor_ptr;
	struct dbText *event_ptr;
	struct dbNote *note_ptr;
	struct MIDIevent *mevent_ptr;
	unsigned long track_ctr = 0, lastdelta;
	unsigned long lyric_events = 0, non_lyric_events = 0;
	int error = 0;
	unsigned char phase_shift_sysex_phrase[8] = {'P','S','\0',0,0,0,0,0xF7};	//This is used to write Sysex messages for features supported in Phase Shift (ie. open strum)

	if(!filename ||  !chart)
		return 1;	//Invalid parameters

	///Open file
	if(!filename)
	{
		(void) puts("\tLogic error:  Invalid input file.");
		return 1;	//Invalid parameter
	}
	if(!chart)
	{
		(void) puts("\tLogic error:  Invalid input chart structure.");
		return 2;	//Invalid parameter
	}
	outf = pack_fopen(filename, "wb");
	if(!outf)
	{	//If the file failed to be opened
		(void) printf("\tError opening output file:  %s", strerror(errno));	//Get the Operating System's reason for the failure
		return 3;
	}

	///Count the number of tracks to write
	for(track_ctr = 0, track_ptr = chart->tracks; track_ptr != NULL; track_ptr = track_ptr->next)
	{	//For each track in the linked list
		if(track_ptr->tracktype)	//If this was a track that was identified as one to import
			track_ctr++;
	}
	track_ctr++;	//Add one for the mandatory tempo track

	//Count the number of lyric events and non lyric events
	if(chart->events)
	{	//If there are events to write
		for(event_ptr = chart->events; event_ptr != NULL; event_ptr = event_ptr->next)
		{	//For each event in the linked list
			if(!ustricmp(event_ptr->text, "phrase_start"))
			{	//If this is a start of lyric phrase
				lyric_events++;
			}
			else if(!ustricmp(event_ptr->text, "phrase_end"))
			{	//If this is an end of lyric phrase
				lyric_events++;
			}
			else if(ustrstr(event_ptr->text, "lyric "))
			{	//If this is a lyric
				lyric_events++;
			}
			else
			{	//This is a non lyric event
				non_lyric_events++;
			}
		}
	}
	if(lyric_events)
	{	//If there are any lyric events to write
		track_ctr++;	//Add one for the PART VOCALS track
	}
	if(non_lyric_events)
	{	//If there are non lyric events to write
		track_ctr++;	//Add one for the EVENTS track
	}

	///Write MIDI header
	if(pack_fputs("MThd", outf) == EOF)					//Write the MIDI header chunk ID
		error = 4;
	error |= write_dword_be(6, outf);					//Write the MIDI header chunk size
	error |= write_word_be(1, outf);					//Write a MIDI format of 1
	error |= write_word_be(track_ctr, outf);			//Write the number of tracks
	error |= write_word_be(chart->resolution, outf);	//Write the time division (number of ticks per quarter note)
	if(error)
	{	//If any of that failed to be written
		(void) puts("\tI/O error writing MIDI file header.");
		pack_fclose(outf);
		return 4;
	}

	///Create tempo track
	replace_filename(temp_filename, filename, "temp", PATH_WIDTH);	//A file named "temp" will be used to create track data at the output path
	tempf = pack_fopen(temp_filename, "wb");
	if(!tempf)
	{	//If the file failed to be opened
		(void) printf("\tError opening temp file:  %s", strerror(errno));	//Get the Operating System's reason for the failure
		error = 5;
	}
	error |= write_var_length(0, tempf);
	error |= write_string_meta_event(3, "TEMPO", tempf);
	if(error)
	{	//If the name of the tempo track failed to be written
		(void) puts("\tError writing tempo track name.");
		error = 6;
	}
	for(anchor_ptr = chart->anchors, lastdelta = 0; (anchor_ptr != NULL) && !error; anchor_ptr = anchor_ptr->next)
	{	//For each anchor in the linked list, unless an error has occurred
		int identified = 0;
		if(anchor_ptr->BPM)
		{	//This is a tempo change
			unsigned long ppqn;
			double BPM = (double) anchor_ptr->BPM / 1000.0;		//Feedback format defines the tempo as thousandths of a BPM

			ppqn = (60000000.0 / BPM) + 0.5;					//Convert BPM to ppqn, rounding up
			error |= write_var_length(anchor_ptr->chartpos - lastdelta, tempf);	//Write this tempo change's relative delta time
			error |= (pack_putc(0xFF, tempf) == EOF);						//Write Meta Event 0x51 (Set Tempo)
			error |= (pack_putc(0x51, tempf) == EOF);
			error |= (pack_putc(0x03, tempf) == EOF);						//Write event length of 3
			error |= (pack_putc((ppqn & 0xFF0000) >> 16, tempf) == EOF);	//Write high order byte of ppqn
			error |= (pack_putc((ppqn & 0xFF00) >> 8, tempf) == EOF);		//Write middle byte of ppqn
			error |= (pack_putc((ppqn & 0xFF), tempf) == EOF);				//Write low order byte of ppqn
			lastdelta = anchor_ptr->chartpos;	//Track the last delta time written because MIDI defines event timings as relative
			identified = 1;
		}
		if((anchor_ptr->TSN) || (anchor_ptr->TSD))
		{	//This is a time signature change
			unsigned long den;

			for(den = 0; den <= 8; den++)
			{	//Convert the denominator into the power of two required to write into the MIDI event
				if(anchor_ptr->TSD >> den == 1)	//if 2 to the power of i is the denominator
					break;
			}
			if(anchor_ptr->TSD >> den != 1)
			{	//If the loop ended before the appropriate value was found
				(void) printf("\t\tWARNING:  Invalid time signature %u/%u detected, skipping.\n", anchor_ptr->TSN, anchor_ptr->TSD);
				continue;
			}

			error |= write_var_length(anchor_ptr->chartpos - lastdelta, tempf);	//Write this time signature change's relative delta time
			error |= (pack_putc(0xFF, tempf) == EOF);				//Write Meta Event 0x58 (Time Signature)
			error |= (pack_putc(0x58, tempf) == EOF);
			error |= (pack_putc(0x04, tempf) == EOF);				//Write event length of 4
			error |= (pack_putc(anchor_ptr->TSN, tempf) == EOF);	//Write the numerator
			error |= (pack_putc(den, tempf) == EOF);				//Write the denominator
			error |= (pack_putc(24, tempf) == EOF);					//Write the metronome interval (not used by EOF)
			error |= (pack_putc(8, tempf) == EOF);					//Write the number of 32nd notes per 24 ticks (not used by EOF)
			lastdelta = anchor_ptr->chartpos;	//Track the last delta time written because MIDI defines event timings as relative
			identified = 1;
		}
		if(anchor_ptr->usec)
		{	//This is an anchor (defining a realtime position for a delta position)
			identified = 1;
			continue;	///Ignore these for now, they shouldn't be relevant to the beat timings defined by tempo and time signature changes
		}
		if(!identified )
		{	//If this anchor type is unknown
			(void) puts("\tLogic error:  Corrupted anchor list");
			error = 7;
			break;
		}

		if(error)
		{
			(void) puts("\t\tI/O error writing tempo track.");
		}
	}//For each anchor in the linked list, unless an error has occurred
	error |= (write_var_length(0, tempf) == EOF);	//Write delta time
	error |= (pack_putc(0xFF, tempf) == EOF);		//Write Meta Event 0x2F (End Track)
	error |= (pack_putc(0x2F, tempf) == EOF);
	error |= (pack_putc(0x00, tempf) == EOF);		//Write padding

	if(tempf)
		(void) pack_fclose(tempf);	//Close the temporary file
	tempf = NULL;

	if(error)
	{	//If the tempo track failed to be created
		(void) pack_fclose(outf);
		return error;
	}

	///Write tempo track
	#ifdef CCDEBUG
		(void) puts("\tWriting tempo track.");
	#endif
	if(dump_midi_track("temp", outf))
	{	//If the file failed to be written
		(void) puts("\t\tError writing tempo track.");
		(void) pack_fclose(outf);
		return 7;
	}

	///Create events track
	if(non_lyric_events)
	{	//If there is at least one non lyric event to write
		tempf = pack_fopen(temp_filename, "wb");
		if(!tempf)
		{	//If the file failed to be opened
			(void) printf("\tError opening temp file:  %s", strerror(errno));	//Get the Operating System's reason for the failure
			error = 8;
		}
		error |= write_var_length(0, tempf);
		error |= write_string_meta_event(3, "EVENTS", tempf);
		if(error)
		{	//If the name of the events track failed to be written
			(void) puts("\tError writing event track name.");
			error = 9;
		}
		for(event_ptr = chart->events, lastdelta = 0; (event_ptr != NULL) && !error; event_ptr = event_ptr->next)
		{	//For each event in the linked list, unless an error has occurred
			if(!ustricmp(event_ptr->text, "phrase_start"))
			{	//If this is a start of lyric phrase
				continue;	//Skip it
			}
			else if(!ustricmp(event_ptr->text, "phrase_end"))
			{	//If this is an end of lyric phrase
				continue;	//Skip it
			}
			else if(ustrstr(event_ptr->text, "lyric "))
			{	//If this is a lyric
				continue;	//Skip it
			}

			error |= write_var_length(event_ptr->chartpos - lastdelta, tempf);	//Write this text event's relative delta time
			error |= write_string_meta_event(1, event_ptr->text, tempf);

			if(error)
			{
				(void) puts("\t\tI/O error writing event track.");
			}
			lastdelta = event_ptr->chartpos;	//Track the last delta time written because MIDI defines event timings as relative
		}//For each event in the linked list, unless an error has occurred
		error |= (write_var_length(0, tempf) == EOF);	//Write delta time
		error |= (pack_putc(0xFF, tempf) == EOF);		//Write Meta Event 0x2F (End Track)
		error |= (pack_putc(0x2F, tempf) == EOF);
		error |= (pack_putc(0x00, tempf) == EOF);		//Write padding

		if(tempf)
			(void) pack_fclose(tempf);	//Close the temporary file
		tempf = NULL;

		if(error)
		{	//If the events track failed to be created
			(void) pack_fclose(outf);
			return error;
		}

	///Write events track
		#ifdef CCDEBUG
			(void) puts("\tWriting event track.");
		#endif
		if(dump_midi_track("temp", outf))
		{	//If the file failed to be written
			(void) puts("\t\tError writing event track.");
			(void) pack_fclose(outf);
			return 10;
		}
	}//If there is at least one non lyric event to write

	///Create vocal track
	///Create events track
	if(lyric_events)
	{	//If there is at least one lyric event to write
		tempf = pack_fopen(temp_filename, "wb");
		if(!tempf)
		{	//If the file failed to be opened
			(void) printf("\tError opening temp file:  %s", strerror(errno));	//Get the Operating System's reason for the failure
			error = 8;
		}
		error |= write_var_length(0, tempf);
		error |= write_string_meta_event(3, "PART VOCALS", tempf);
		if(error)
		{	//If the name of the events track failed to be written
			(void) puts("\tError writing vocal track name.");
			error = 9;
		}
		for(event_ptr = chart->events, lastdelta = 0; (event_ptr != NULL) && !error; event_ptr = event_ptr->next)
		{	//For each event in the linked list, unless an error has occurred
			if(!ustricmp(event_ptr->text, "phrase_start"))
			{	//If this is a start of lyric phrase, start a marker with note 105
				error |= write_var_length(event_ptr->chartpos - lastdelta, tempf);	//Write this text event's relative delta time
				error |= (pack_putc(0x90, tempf) == EOF);		//Write note on event, channel 0
				error |= (pack_putc(105, tempf) == EOF);		//Write note 105
				error |= (pack_putc(100, tempf) == EOF);		//Write note velocity
			}
			else if(!ustricmp(event_ptr->text, "phrase_end"))
			{	//If this is an end of lyric phrase, end the marker
				error |= write_var_length(event_ptr->chartpos - lastdelta, tempf);	//Write this text event's relative delta time
				error |= (pack_putc(0x80, tempf) == EOF);		//Write note off event, channel 0
				error |= (pack_putc(105, tempf) == EOF);		//Write note 105
				error |= (pack_putc(100, tempf) == EOF);		//Write note velocity
			}
			else if(ustrstr(event_ptr->text, "lyric "))
			{	//If this is a lyric
				char *lyric = strcasestr_spec(event_ptr->text, "lyric ");	//Find the beginning of the lyric text (the first character after "lyric ")
				if(lyric)
				{	//As long as it was found
					error |= write_var_length(event_ptr->chartpos - lastdelta, tempf);	//Write this text event's relative delta time
					error |= write_string_meta_event(5, lyric, tempf);
				}
			}
			else
			{	//This is a non lyric event
				continue;	//Skip it
			}

			if(error)
			{
				(void) puts("\t\tI/O error writing vocal track.");
			}
			lastdelta = event_ptr->chartpos;	//Track the last delta time written because MIDI defines event timings as relative
		}//For each event in the linked list, unless an error has occurred
		error |= (write_var_length(0, tempf) == EOF);	//Write delta time
		error |= (pack_putc(0xFF, tempf) == EOF);		//Write Meta Event 0x2F (End Track)
		error |= (pack_putc(0x2F, tempf) == EOF);
		error |= (pack_putc(0x00, tempf) == EOF);		//Write padding

		if(tempf)
			(void) pack_fclose(tempf);	//Close the temporary file
		tempf = NULL;

		if(error)
		{	//If the events track failed to be created
			(void) pack_fclose(outf);
			return error;
		}

	///Write vocal track
		#ifdef CCDEBUG
			(void) puts("\tWriting vocal track.");
		#endif
		if(dump_midi_track("temp", outf))
		{	//If the file failed to be written
			(void) puts("\t\tError writing vocal track.");
			(void) pack_fclose(outf);
			return 11;
		}
	}//If there is at least one lyric event to write

	///Generate instrument track events
	for(track_ctr = 1, track_ptr = chart->tracks; (track_ptr != NULL) && !error; track_ctr++, track_ptr = track_ptr->next)
	{	//For each track in the linked list, unless an error has occurred
		int midi_note_offset = 60, is_ghl = 0;
		unsigned long ch_solo_pos = 0;	//Tracks the start position of the last Clone Hero solo event
		char ch_solo_on = 0;			//Tracks whether a Clone Hero solo event is in progress
		unsigned long slider_count = 0, open_strum_count = 0;	//Debugging

		if(!track_ptr->tracktype)	//If this was not a track that was identified as one to import
			continue;				//Skip it
		if(track_ptr->tracktype >= NUM_MIDI_TRACKS)
		{
			(void) puts("\tLogic error:  Invalid instrument track type.");
			error = 12;
			break;
		}

		#ifdef CCDEBUG
			(void) printf("\tBuilding events for track \"%s\".\n", track_ptr->trackname);
		#endif

		switch(track_ptr->difftype)
		{
			case 1:	//Easy difficulty
				midi_note_offset = 60;	//Lane 1 is note 60
				break;
			case 2:	//Medium difficulty
				midi_note_offset = 72;	//Lane 1 is note 72
				break;
			case 3:	//Hard difficulty
				midi_note_offset = 84;	//Lane 1 is note 84
				break;
			case 4:	//Expert difficulty
				midi_note_offset = 96;	//Lane 1 is note 96
				break;
		}
		if((track_ctr == 7) || (track_ctr == 8))
		{	//If the track being exported is a GHL track
			is_ghl = 1;	//Note this
			midi_note_offset--;	//And reflect that the first lane in this track format is one MIDI note lower, and open notes are one note lower still
		}

		///Write MIDI events to the track's linked list
		for(note_ptr = track_ptr->notes; note_ptr != NULL && !error; note_ptr = note_ptr->next)
		{	//For each note in the linked list, unless an error has occurred
			unsigned char gemcolor = note_ptr->gemcolor;	//Simplify
			unsigned long startpos, endpos;
			int vel = 100;	//Use a velocity of 100 for notes

			startpos = note_ptr->chartpos;	//Simplify
			endpos = startpos + note_ptr->duration;

			if(gemcolor == '2')
			{	//Star power notation
				struct dbNote *ptr;

				///Look ahead to see if the star power phrase ends at another note's start position.  If so, the marker needs to be shortened to NOT include that note
				if(note_ptr->duration > 1)
				{	//If the star power marker is at least 2 ticks long
					for(ptr = note_ptr->next; ptr != NULL; ptr = ptr->next)
					{	//For notes defined after the star power marker
						if(ptr->chartpos > endpos)
						{	//If this and all remaining notes are outside the scope of the star power marker
							break;	//Stop processing the notes following the star power marker
						}
						if(ptr->chartpos == endpos)
						{	//If this note starts at the star power marker's end position
							endpos--;	//Shorten the marker by one delta tick to ensure the note is outside its scope when written to MIDI
							break;	//Stop processing the notes following the star power marker
						}
					}
				}
				error |= add_midi_event(track_ptr, startpos, 0x90, 116, vel, 0);	//Write a marker with note 116
				error |= add_midi_event(track_ptr, endpos, 0x80, 116, vel, 0);
			}
			else if(gemcolor == 'S')
			{	//Clone Hero solo start notation
				ch_solo_pos = startpos;	//Store this until the solo end notation is parsed
				ch_solo_on = 1;
			}
			else if(gemcolor == 'E')
			{	//Clone Hero solo end notation
				if(ch_solo_on)
				{	//If the start of the solo was defined
					///Write midi events for solo marker
					error |= add_midi_event(track_ptr, ch_solo_pos, 0x90, 103, vel, 0);	//Write a marker with note 103, ranging from the 'S' start marker to this 'E' end marker
					error |= add_midi_event(track_ptr, startpos, 0x80, 103, vel, 0);
					ch_solo_on = 0;
				}
			}
			else if(gemcolor == 5)
			{	//Toggle HOPO marker (this was already processed by set_hopo_status() )
			}
			else if(gemcolor == 6)
			{	//Slider marker (this was already processed by set_slider_status(), the sliders will be written later on)
			}
			else if(gemcolor == 7)
			{	//Open strum
				if(!track_ptr->isdrums)
				{	//If this isn't a drum track
					if(is_ghl)
					{	//GHL tracks define open notes as one note below lane 1
						error |= add_midi_event(track_ptr, startpos, 0x90, midi_note_offset - 1, vel, 0);	//Write as 1 note below lane 1
						error |= add_midi_event(track_ptr, endpos, 0x80, midi_note_offset - 1, vel, 0);
					}
					else
					{	//Other tracks use a lane 1 gem and a Sysex marker
						error |= add_midi_event(track_ptr, startpos, 0x90, midi_note_offset + 0, vel, 0);	//Write a gem for lane 1
						error |= add_midi_event(track_ptr, endpos, 0x80, midi_note_offset + 0, vel, 0);
						phase_shift_sysex_phrase[3] = 0;	//Store the Sysex message ID (0 = phrase marker)
						phase_shift_sysex_phrase[4] = track_ptr->difftype - 1;	//Store the difficulty ID (0 = Easy, 1 = Medium, 2 = Hard, 3 = Expert)
						phase_shift_sysex_phrase[5] = 1;	//Store the phrase ID (1 = Open Strum Bass)
						phase_shift_sysex_phrase[6] = 1;	//Store the phrase status (1 = Phrase start)
						error |= add_midi_sysex_event(track_ptr, startpos, 8, phase_shift_sysex_phrase, 1);	//Write the custom open bass phrase start marker
						phase_shift_sysex_phrase[6] = 0;	//Store the phrase status (0 = Phrase stop)
						error |= add_midi_sysex_event(track_ptr, endpos, 8, phase_shift_sysex_phrase, 0);	//Write the custom open bass phrase stop marker
						open_strum_count++;
					}
				}
			}
			else
			{	//A note instead of a marker
				int note = 0;

				if(is_ghl)
				{	//If this is a GHL track, there are special mappings
					if(gemcolor == 0)
						note = 3;		//W1 is written as lane 4
					else if(gemcolor == 1)
						note = 4;		//W2 is written as lane 5
					else if(gemcolor == 2)
						note = 5;		//W3 is written as lane 6
					else if(gemcolor == 3)
						note = 0;		//B1 is written as lane 1
					else if(gemcolor == 4)
						note = 1;		//B2 is written as lane 2
					else if(gemcolor == 8)
						note = 2;		//B3 is written as lane 3
				}
				else
				{	//Normal lane numbering
					note = gemcolor;
				}

				error |= add_midi_event(track_ptr, startpos, 0x90, midi_note_offset + note, vel, 0);	//Write a gem for the associated lane
				error |= add_midi_event(track_ptr, endpos, 0x80, midi_note_offset + note, vel, 0);
			}

			//Write forced HOPO marker if applicable
			if(note_ptr->is_hopo)
			{	//Write a forced HOPO marker
				if(is_ghl)
				{	//If this is a GHL track, forced HOPO is written one note above lane 6
					error |= add_midi_event(track_ptr, startpos, 0x90, midi_note_offset + 6, vel, 0);
					error |= add_midi_event(track_ptr, endpos, 0x80, midi_note_offset + 6, vel, 0);
				}
				else
				{	//Otherwise it is written one note above lane 5
					error |= add_midi_event(track_ptr, startpos, 0x90, midi_note_offset + 5, vel, 0);
					error |= add_midi_event(track_ptr, endpos, 0x80, midi_note_offset + 5, vel, 0);
				}
			}
			else
			{	//Write a forced strum marker
				if(is_ghl)
				{	//If this is a GHL track, forced strum is written two notes above lane 6
					error |= add_midi_event(track_ptr, startpos, 0x90, midi_note_offset + 7, vel, 0);
					error |= add_midi_event(track_ptr, endpos, 0x80, midi_note_offset + 7, vel, 0);
				}
				else
				{	//Otherwise it is written two notes above lane 5
					error |= add_midi_event(track_ptr, startpos, 0x90, midi_note_offset + 6, vel, 0);
					error |= add_midi_event(track_ptr, endpos, 0x80, midi_note_offset + 6, vel, 0);
				}
			}
		}//For each note in the linked list, unless an error has occurred

		//Write slider phrases
		if(!track_ptr->isdrums)
		{	//If this isn't a drum track
			unsigned long start, end;
			char slider_on = 0;

			for(note_ptr = track_ptr->notes; note_ptr != NULL && !error; note_ptr = note_ptr->next)
			{	//For each note in the linked list, unless an error has occurred
				if(!note_ptr->is_gem)
				{	//If this is a status marker instead of a note
					continue;	//Skip it
				}
				if(note_ptr->is_slider)
				{	//If this note is a slider note
					if(!slider_on)
					{	//If a set of contiguous slider notes is not already being tracked
						slider_on = 1;
						start = note_ptr->chartpos;			//Track the position of the first of potential contiguous slider notes
						end = start + note_ptr->duration;	//And the end position
					}
					else
					{	//There are other slider notes in this contiguous set
						end = note_ptr->chartpos + note_ptr->duration;	//Update the end position
					}
				}
				else
				{	//This is not a slider note
					if(slider_on)
					{	//But one or more slider notes were just parsed
						phase_shift_sysex_phrase[3] = 0;	//Store the Sysex message ID (0 = phrase marker)
						phase_shift_sysex_phrase[4] = 0xFF;	//Store the difficulty ID (0xFF = all difficulties)
						phase_shift_sysex_phrase[5] = 4;	//Store the phrase ID (4 = slider)
						phase_shift_sysex_phrase[6] = 1;	//Store the phrase status (1 = Phrase start)
						error |= add_midi_sysex_event(track_ptr, start, 8, phase_shift_sysex_phrase, 1);	//Write the custom slider start marker
						phase_shift_sysex_phrase[6] = 0;	//Store the phrase status (0 = Phrase stop)
						error |= add_midi_sysex_event(track_ptr, end, 8, phase_shift_sysex_phrase, 0);	//Write the custom slider stop marker
						slider_count++;
						slider_on = 0;	//Track that a run of slider notes is not in progress
					}
				}
			}
		}

		#ifdef CCDEBUG
			if(slider_count)
				(void) printf("\t\t%lu slider Sysex phrases written.\n", slider_count);
			if(open_strum_count)
				(void) printf("\t\t%lu open strum Sysex phrases written.\n", open_strum_count);
		#endif
	}//For each track in the linked list, unless an error has occurred

	#ifdef CCDEBUG
		(void) puts("\tSorting MIDI events");
	#endif
	qsort_midi_events(chart);	//Sort all of the events that were added to the lists in midi_track_events[]

	///Create and write instrument tracks
	for(track_ctr = 1; track_ctr < NUM_MIDI_TRACKS; track_ctr++)
	{	//For each of the instrument tracks that may have been built
		if(midi_track_events[track_ctr] == NULL)	//If there was no content imported for this instrument track
			continue;	//Skip it

		#ifdef CCDEBUG
			(void) printf("\tWriting MIDI events for track \"%s\".\n", midi_track_name[track_ctr]);
		#endif

		//Write track name
		tempf = pack_fopen(temp_filename, "wb");
		if(!tempf)
		{	//If the file failed to be opened
			(void) printf("\tError opening temp file:  %s", strerror(errno));	//Get the Operating System's reason for the failure
			error = 13;
			break;
		}
		error |= write_var_length(0, tempf);
		error |= write_string_meta_event(3, midi_track_name[track_ctr], tempf);
		if(error)
		{	//If the name of the instrument track failed to be written
			(void) puts("\tError writing instrument track name.");
			error = 14;
			break;
		}

		//Write the events from the track's linked list
		for(mevent_ptr = midi_track_events[track_ctr], lastdelta = 0; mevent_ptr != NULL; mevent_ptr = mevent_ptr->next)
		{	//For each MIDI event built for this instrument track
			error |= write_var_length(mevent_ptr->pos - lastdelta, tempf);	//Write delta time

			if(mevent_ptr->type == 0x01)
			{	//Text event
				error |= write_string_meta_event(1, mevent_ptr->dp, tempf);		//Write the string
			}
			else if(mevent_ptr->type == 0xF0)
			{	//Sysex message
				error |= (pack_putc(0xF0, tempf) == EOF);			//Write Sysex event type
				error |= write_var_length(mevent_ptr->note, tempf);	//Write the Sysex message's size
				if(pack_fwrite(mevent_ptr->dp, mevent_ptr->note, tempf) != mevent_ptr->note)
				{	//If the Sysex message's data couldn't be written
					error = 1;	//I/O error
				}
			}
			else
			{	//Note on/off
				error |= (pack_putc(mevent_ptr->type + mevent_ptr->channel, tempf) == EOF);	//Write event type (upper nibble) and channel number (lower nibble)
				error |= (pack_putc(mevent_ptr->note, tempf) == EOF);	//Write note number
				error |= (pack_putc(mevent_ptr->velocity, tempf) == EOF);	//Write note velocity
			}

			lastdelta = mevent_ptr->pos;	//Store this event's absolute delta time so relative times can be determined
		}

		//Write end of track
		error |= (write_var_length(0, tempf) == EOF);	//Write delta time
		error |= (pack_putc(0xFF, tempf) == EOF);		//Write Meta Event 0x2F (End Track)
		error |= (pack_putc(0x2F, tempf) == EOF);
		error |= (pack_putc(0x00, tempf) == EOF);		//Write padding

		if(tempf)
			(void) pack_fclose(tempf);	//Close the temporary file
		tempf = NULL;

		//Write track to MIDI file being exported
		#ifdef CCDEBUG
			(void) printf("\tWriting track \"%s\".\n", midi_track_name[track_ctr]);
		#endif
		if(dump_midi_track("temp", outf))
		{	//If the file failed to be written
			(void) puts("\t\tError writing track.");
			(void) pack_fclose(outf);
			return 15;
		}
	}

	if(tempf)
		(void) pack_fclose(tempf);	//Close the temporary file
	tempf = NULL;

	if(error)
	{	//If an instrument track failed to be created
		(void) pack_fclose(outf);
		return error;
	}

	///Cleanup
	(void) pack_fclose(outf);

	return 0;	//Return success
}

int dump_midi_track(const char *inputfile, PACKFILE *outf)
{
	unsigned long track_length;
	PACKFILE *inf = NULL;
	char trackheader[8] = {'M', 'T', 'r', 'k', 0, 0, 0, 0};
	unsigned long i;
	int retval = 0;

	if((inputfile == NULL) || (outf == NULL))
	{
		return 1;	//Invalid parameters
	}
	track_length = (unsigned long)file_size_ex(inputfile);
	inf = pack_fopen(inputfile, "r");	//Open input file for reading
	if(!inf)
	{	//If the file failed to be opened
		(void) printf("\t\tError opening temporary file:  %s", strerror(errno));	//Get the Operating System's reason for the failure
		return 2;	//Return failure
	}

	if(pack_fwrite(trackheader, 4, outf) != 4)
	{	//If the output track header couldn't be written
		retval = 4;	//I/O error
	}
	if(pack_mputl(track_length, outf) == EOF)
	{	//If the output track length couldn't be written
		retval = 4;	//I/O error
	}
	for(i = 0; i < track_length; i++)
	{	//For each byte in the input file
		int c = pack_getc(inf);

		if(c == EOF)
		{	//If the byte couldn't be read from the input file
			(void) puts("\t\tError reading temporary file.");
			retval = 3;
			break;
		}
		if(pack_putc(c, outf) == EOF)
		{	//If the byte couldn't be written to the output file
			(void) puts("\t\tError writing to output file.");
			retval = 4;	//I/O error
			break;
		}
	}
	(void) pack_fclose(inf);

	return retval;
}

void sort_chart(struct FeedbackChart *chart)
{
	char sorted;
	struct dbAnchor *anchor_ptr, *prev_anchor_ptr;
	struct dbText *text_ptr, *prev_text_ptr;
	struct dbTrack *track_ptr;
	struct dbNote *note_ptr, *prev_note_ptr;

	if(!chart)
		return;	//Invalid parameter

	//Sort anchors
	do{
		sorted = 1;
		for(anchor_ptr = chart->anchors, prev_anchor_ptr = NULL; anchor_ptr != NULL; anchor_ptr = anchor_ptr->next)
		{	//For each anchor in the linked list
			if(anchor_ptr->next && (anchor_ptr->next->chartpos < anchor_ptr->chartpos))
			{	//If there's another anchor and it should sort before this one
				struct dbAnchor *this_ptr = anchor_ptr, *next_ptr = anchor_ptr->next;

				#ifdef CCDEBUG
					(void) printf("\tSorting anchor (Delta pos = %lu, tempo = %lu, TS = %u/%u, usec = %lu) before anchor (Delta pos = %lu, tempo = %lu, TS = %u/%u, usec = %lu).\n", anchor_ptr->chartpos, anchor_ptr->BPM, anchor_ptr->TSN, anchor_ptr->TSD, anchor_ptr->usec, anchor_ptr->next->chartpos, anchor_ptr->next->BPM, anchor_ptr->next->TSN, anchor_ptr->next->TSD, anchor_ptr->next->usec);
				#endif

				this_ptr->next = next_ptr->next;
				next_ptr->next = this_ptr;
				if(prev_anchor_ptr == NULL)
				{	//If the head link of the list was swapped
					chart->anchors = next_ptr;			//Update the linked list pointer in the chart structure
				}
				else
				{
					prev_anchor_ptr->next = next_ptr;	//Point the previous link forward to the earlier of the two links just sorted
				}
				sorted= 0;
				break;	//Restart the for loop
			}
			prev_anchor_ptr = anchor_ptr;
		}
	}while(!sorted);

	//Sort events
	do{
		sorted = 1;
		for(text_ptr = chart->events, prev_text_ptr = NULL; text_ptr != NULL; text_ptr = text_ptr->next)
		{	//For each text event in the linked list
			if(text_ptr->next && (text_ptr->next->chartpos < text_ptr->chartpos))
			{	//If there's another text event and it should sort before this one
				struct dbText *this_ptr = text_ptr, *next_ptr = text_ptr->next;

				#ifdef CCDEBUG
					(void) printf("\tSorting text event (Delta pos = %lu, \"%s\") before text event (Delta pos = %lu, \"%s\".\n", text_ptr->chartpos, text_ptr->text, text_ptr->next->chartpos, text_ptr->next->text);
				#endif

				this_ptr->next = next_ptr->next;
				next_ptr->next = this_ptr;
				if(prev_text_ptr == NULL)
				{	//If the head link of the list was swapped
					chart->events = next_ptr;	//Update the linked list pointer in the chart structure
				}
				else
				{
					prev_text_ptr = next_ptr;	//Point the previous link forward to the earlier of the two links just sorted
				}
				sorted= 0;
				break;	//Restart the for loop
			}
			prev_text_ptr = text_ptr;
		}
	}while(!sorted);

	//Sort track content
	for(track_ptr = chart->tracks; track_ptr != NULL; track_ptr = track_ptr->next)
	{	//For each track
		#ifdef CCDEBUG
			(void) printf("\tSorting track %s.\n", track_ptr->trackname);
		#endif
		//Sort notes
		do{
			sorted = 1;
			for(note_ptr = track_ptr->notes, prev_note_ptr = NULL; note_ptr != NULL; note_ptr = note_ptr->next)
			{	//For each note in the linked list
				if(note_ptr->next)
				{	//If there's another note and it should sort before this one
					int swap = 0;

					if(note_ptr->next->chartpos < note_ptr->chartpos)
					{	//If the next note occurs before this note
						swap = 1;
					}
					if((note_ptr->next->chartpos == note_ptr->chartpos) && (note_ptr->next->gemcolor > note_ptr->gemcolor))
					{	//If the two notes have the same position, but the next one has a higher lane (to help reliably sort toggle HOPO markers before the gems they affect, suiting a singly linked list)
						swap = 1;
					}
					if(swap)
					{
						struct dbNote *this_ptr = note_ptr, *next_ptr = note_ptr->next;

						this_ptr->next = next_ptr->next;
						next_ptr->next = this_ptr;
						if(prev_note_ptr == NULL)
						{	//If the head link of the list was swapped
							track_ptr->notes = next_ptr;	//Update the linked list pointer in the track structure
						}
						else
						{
							prev_note_ptr->next = next_ptr;	//Point the previous link forward to the earlier of the two links just sorted
						}
						sorted = 0;
						break;	//Restart the for loop
					}
				}
				prev_note_ptr = note_ptr;
			}
		}while(!sorted);
	}
}

int write_string_meta_event(unsigned char type, const char *str, PACKFILE *outf)
{
	unsigned long length;
	int error = 0;

	if(!str || !outf)
		return 1;	//Invalid parameters

	length = ustrlen(str);
	error |= (pack_putc(0xFF, outf) == EOF);
	error |= (pack_putc(type, outf) == EOF);
	error |= write_var_length(length, outf);
	if(error)
	{	//if any of the above failed to be written
		return 2;
	}

	if(pack_fwrite(str, length, outf) != length)
	{	//If the string failed to be written
		return 3;
	}

	return 0;	//Return success
}

struct MIDIevent *append_midi_event(struct dbTrack *track)
{
	static struct MIDIevent *head = NULL;		//Keeps track of the end of the linked list between calls to avoid having to iterate to the end every time
	static struct MIDIevent *conductor = NULL;
	struct MIDIevent *event;

	if(!track)
		return NULL;	//Invalid parameter

	//Find the head of the track's MIDI event linked list if it isn't set yet
	if(track->events == NULL)
	{	//If this track doesn't have a linked list
		track->events = midi_track_events[track->tracktype];	//Point it to the linked list shared among the four difficulties of this instrument, if it is established
	}

	//Find the end of the track's MIDI event linked list if the static pointers aren't caching the correct pointers
	if(head != track->events)
	{	//If this is not the same track that had an event added in the previous call to this function
		for(conductor = track->events; (conductor != NULL) && (conductor->next != NULL); conductor = conductor->next);	//Obtain the last link in the list, if there are any
	}

	//Create a new MIDI event link and append it to the track's MIDI event linked list
	event = malloc(sizeof(struct MIDIevent));
	memset(event, 0, sizeof(struct MIDIevent));
	if(!event)
	{
		(void) puts("Failed to allocate memory.");
		return NULL;
	}
	if(!conductor)
	{	//If the linked list is empty
		track->events = event;	//This is the head of the list
		midi_track_events[track->tracktype] = event;	//It is also the head of the linked list to be used for the other difficulties of this instrument
	}
	else
	{	//The linked list is populated
		conductor->next = event;
	}
	conductor = event;		//Remember this is the end of the linked list for the next call to this function
	head = track->events;	//Remember this is the linked list that is being operated on for the next call to this function

	return event;
}

int add_midi_event(struct dbTrack *track, unsigned long pos, int type, int note, int velocity, int channel)
{
	struct MIDIevent *event;

	if(!track)
		return 1;	//Invalid parameter

	event = append_midi_event(track);	//Append a new event to the track's MDII event linked list
	if(!event)
		return 2;	//Allocation failed

	//Initialize the new MIDI event link
	event->pos = pos;
	event->type = type;
	event->note = note;
	event->velocity = velocity;
	event->channel = channel;

	if((type & 0xF0) == 0x80)
	{	//If this is a note off event
		event->off = 1;
	}
	else if((type & 0xF0) == 0x90)
	{	//If this is a note on event
		event->on = 1;
	}

	return 0;
}

int add_midi_lyric_event(struct dbTrack *track, unsigned long pos, char *text)
{
	struct MIDIevent *event;

	if(!track || !text)
		return 1;	//Invalid parameter

	event = append_midi_event(track);	//Append a new event to the track's MDII event linked list
	if(!event)
		return 2;	//Allocation failed

	//Initialize the new MIDI event link
	event->pos = pos;
	event->type = 0x05;
	event->dp = duplicate_string(text);	//Store a newly allocated copy of the string into the link
	if(!event->dp)
	{
		(void) puts("Failed to allocate memory.");
		return 2;
	}

	return 0;
}

int add_midi_text_event(struct dbTrack *track, unsigned long pos, char *text)
{
	struct MIDIevent *event;

	if(!track || !text)
		return 1;	//Invalid parameter

	event = append_midi_event(track);	//Append a new event to the track's MDII event linked list
	if(!event)
		return 2;	//Allocation failed

	//Initialize the new MIDI event link
	event->pos = pos;
	event->type = 0x01;
	event->dp = duplicate_string(text);	//Store a newly allocated copy of the string into the link
	if(!event->dp)
	{
		(void) puts("Failed to allocate memory.");
		return 3;
	}

	return 0;
}

int add_midi_sysex_event(struct dbTrack *track, unsigned long pos, int size, void *data, char sysexon)
{
	struct MIDIevent *event;
	void *datacopy;

	if(!track || !size || !data)
		return 1;	//Invalid parameter

	event = append_midi_event(track);	//Append a new event to the track's MDII event linked list
	if(!event)
		return 2;	//Allocation failed
	datacopy = malloc((size_t)size);
	if(!datacopy)
	{
		(void) puts("Failed to allocate memory.");
		return 3;	//Allocation failed
	}
	memcpy(datacopy, data, (size_t)size);	//Copy the input data into the new buffer

	//Initialize the new MIDI event link
	event->pos = pos;
	event->type = 0xF0;
	event->note = size;		//Store the size of the Sysex message in the note variable
	event->dp = datacopy;	//Store the newly allocated copy of the Sysex data
	event->sysexon = sysexon;

	return 0;
}

int should_swap_events(struct MIDIevent *this_ptr, struct MIDIevent *next_ptr)
{
	if(!this_ptr || !next_ptr)
	{
		return 0;	//Invalid parameters
	}

	//Chronological order takes precedence in sorting
	if(next_ptr->pos < this_ptr->pos)
	{	//If the next event occurs before this event
		return 1;
	}
	if(this_ptr->pos < next_ptr->pos)
	{	//This event occurs before the next event
		return 0;	//This is the correct order
	}

	//Logical order of custom Sysex markers:  Phrase on, note on, note off, Phrase off
	if((this_ptr->type == 0xF0) && (next_ptr->type == 0x90))
	{	//If this event is a Sysex event and the next is a note on event
		if(!this_ptr->sysexon)
		{	//If this event is a phrase off Sysex marker or some other Sysex message
			return 1;
		}
		else
		{	//This event is a phrase on Sysex marker and the next event is a note on
			return 0;	//This is the correct order
		}
	}
	if((this_ptr->type == 0x90) && (next_ptr->type == 0xF0))
	{	//If this event is a note on event and the next is a Sysex event
		if(next_ptr->sysexon)
		{	//If the next event is a Sysex phrase on marker
			return 1;
		}
		else
		{	//This event is a note on and the next event is a phrase off Sysex marker
			return 0;	//This is the correct order
		}
	}
	if((this_ptr->type == 0x80) && (next_ptr->type == 0xF0))
	{	//If this event is a note off event and the next is a Sysex event
		if(next_ptr->sysexon)
		{	//If the next event is a Sysex phrase on marker
			return 1;
		}
		else
		{	//This event is a note off event and the next event is a phrase off Sysex marker
			return 0;	//This is the correct order
		}
	}
	if((this_ptr->type == 0xF0) && (next_ptr->type == 0x80))
	{	//If this event is a Sysex event and the next is a note off event
		if(!this_ptr->sysexon)
		{	//If this event is a phrase off Sysex marker or some other Sysex message
			return 1;
		}
		else
		{	//This event is a phrase on Sysex marker and the next event is a note off
			return 0;	//This is the correct order
		}
	}

	//Logical order of lyric markings:  Overdrive on, solo on, lyric on, lyric, lyric pitch, ..., lyric off, solo off, overdrive off
	if((this_ptr->type == 0x90) && (next_ptr->type == 0x90))
	{	//If this event and the next are both note on events
		if(next_ptr->note == 116)
		{	//If the next event is overdrive
			return 1;
		}
		if(this_ptr->note == 116)
		{	//If this event is overdrive
			return 0;	//This is the correct order
		}
		if(next_ptr->note == 105)
		{	//If the next event is a lyric phrase on marker
			return 1;
		}
		if(this_ptr->note == 105)
		{	//If this event is a lyric phrase on marker
			return 0;	//This is the correct order
		}

		//If two note on events occur at the same time for the same note, but one of them has a velocity of 0 (note off), sort the note off to be first
		if(this_ptr->note == next_ptr->note)
		{	//If both note on events are for the same note
			if(this_ptr->velocity && !next_ptr->velocity)
			{	//If this event is a note on and the next event is a note off
				return 1;
			}
			else if(!this_ptr->velocity && next_ptr->velocity)
			{	//This event is a note off and the next event is a note on
				return 0;	//This is the correct order
			}
		}
	}

	//Put lyric events first, except for a lyric phrase on marker
	if((this_ptr->type == 0x05) && (next_ptr->type == 0x90))
	{	//If this event is a lyric event and the next event is a note on
		if((next_ptr->note == 105) || (next_ptr->note == 106))
		{	//If the next event is a lyric phrase start
			return 1;
		}
		else
		{	//This event is a lyric and the next event is not a phrase start
			return 0;	//This is the correct order
		}
	}
	if((this_ptr->type == 0x90) && (next_ptr->type == 0x05))
	{	//If this event is a note on and the next event is a lyric event
		if((this_ptr->note == 105) || (this_ptr->note == 106))
		{	//If this event is a lyric phrase start
			return 0;	//This is the correct order
		}
		else
		{
			return 1;
		}
	}

	///Add sorting of pro drum markers when Clone Hero supports it

	//Put notes first and then markers, will numerically sort in this order:  lyric pitch, lyric off, overdrive off
	if(this_ptr->note > next_ptr->note)
	{	//If this event's note is higher than the next event's note
		return 1;
	}
	else if(this_ptr->note < next_ptr->note)
	{	//If this event's note is lower than the next event's note
		return 0;	//This is the correct order
	}

	//To avoid strange overlap problems, ensure that if a note on and a note off occur at the same time, the note off is written first
	if(this_ptr->note == next_ptr->note)
	{	//If both events are for the same note
		if((this_ptr->type == 0x90) && (next_ptr->type == 0x80))
		{	//If this event is a note on and the next event is a note off
			return 1;
		}
	}

	return 0;	//Do not swap, the two events are in the correct order
}

void insertion_sort_midi_events(struct FeedbackChart *chart)
{
	unsigned long track_ctr;
	struct MIDIevent *sorted_list, *conductor, *event_ptr, *prev_event_ptr, *event_tail = NULL;
	int inserted;
	struct dbTrack *track_ptr;
	unsigned long pre_count, post_count;

	#ifdef CCDEBUG
		clock_t start, end;
	#endif

	if(!chart)
		return;	//Invalid parameter

	#ifdef CCDEBUG
		start = clock();
	#endif

	//Sort the events
	for(track_ctr = 1; track_ctr < NUM_MIDI_TRACKS; track_ctr++)
	{	//For each of the instrument tracks that may have been built
		if(midi_track_events[track_ctr] == NULL)	//If there is no MIDI data for this track
			continue;	//Skip it

		#ifdef CCDEBUG
			(void) printf("\t\tSorting midi track \"%s\".\n", midi_track_name[track_ctr]);
		#endif

		for(event_ptr = midi_track_events[track_ctr], pre_count = 0; event_ptr != NULL; event_ptr = event_ptr->next, pre_count++);	//Count the number of events before sort

		sorted_list = NULL;
		while(midi_track_events[track_ctr])
		{	//While there are unsorted events
			//Extract the head link from the unsorted list
			event_ptr = midi_track_events[track_ctr];		//Remove the first event from the list
			midi_track_events[track_ctr] = event_ptr->next;
			inserted = 0;

			//Sort it into the sorted list
			conductor = NULL;	//Initialize this to NULL in case the fast check to append the extracted link is used and the for loop is skipped
			prev_event_ptr = event_tail;	//Likewise point this to the current end of the list
			if(!sorted_list)
			{	//If the sorted listed is empty
				sorted_list = event_ptr;	//The extracted link is its head link
				event_ptr->next = NULL;		//And there are no links after it yet
			}
			else
			{	//The link will be added to a list with one or more items
				//Optimize this sort to check for suitability of appending to the list before comparing with the middle links in the list
				if(should_swap_events(event_tail, event_ptr))
				{	//If the link should insert before at least the last link in the list
					for(conductor = sorted_list, prev_event_ptr = NULL; conductor != NULL; conductor = conductor->next)
					{	//For each event in the sorted list
						if(should_swap_events(conductor, event_ptr))
						{	//If the extracted link should insert in front of the conductor
							if(!prev_event_ptr)
							{	//If the extracted link is to be inserted in front of all links in the sorted list
								event_ptr->next = sorted_list;	//It is the new head link
								sorted_list = event_ptr;
							}
							else
							{	//The extracted link is to be inserted in the middle of the sorted list
								event_ptr->next = prev_event_ptr->next;
								prev_event_ptr->next = event_ptr;
							}
							inserted = 1;
							break;
						}
						prev_event_ptr = conductor;
					}//For each event in the sorted list
				}
				if(!inserted)
				{	//If the extracted link did not insert in front of any of the already sorted links
					if((conductor == NULL) && prev_event_ptr)
					{	//If the end of the sorted list is known as expected
						prev_event_ptr->next = event_ptr;	//It is the new tail link
						event_ptr->next = NULL;				//And there are no links after it
					}
					else
					{
						(void) puts("\t\tLogic error:  Lost end of sorted list.");
					}
				}
			}
			if(event_ptr->next == NULL)
			{	//If the extracted link is the last link in the sorted list
				event_tail = event_ptr;		//It's the tail link
			}
		}

		midi_track_events[track_ctr] = sorted_list;	//Replace the now empty global list with the sorted list
		for(event_ptr = midi_track_events[track_ctr], post_count = 0; event_ptr != NULL; event_ptr = event_ptr->next, post_count++);	//Count the number of events after sort
		if(pre_count != post_count)
		{	//If the number of events changed during sort
			(void) printf("\t\tLogic error:  MIDI event list corrupted during sort (event count before:  %lu, after:  %lu).\n", pre_count, post_count);
		}
	}//For each of the instrument tracks that may have been built

	//Update the linked list pointers in the chart's track difficulties
	for(track_ptr = chart->tracks; track_ptr != NULL; track_ptr = track_ptr->next)
	{	//For each track in the chart
		if(track_ptr->tracktype < NUM_MIDI_TRACKS)
		{	//Bounds check
			track_ptr->events = midi_track_events[track_ptr->tracktype];	//Point the track to the current head of the associated instrument's event linked list
		}
	}

	#ifdef CCDEBUG
		end = clock();
		(void) printf("\t\tSort completed in %f seconds.\n", ((double)end - start) / (double)CLOCKS_PER_SEC);
	#endif
}

void qsort_midi_events(struct FeedbackChart *chart)
{
	unsigned long track_ctr;
	struct MIDIevent *sorted_list, *event_ptr, *next_ptr;
	struct dbTrack *track_ptr;
	unsigned long pre_count, index;

	#ifdef CCDEBUG
		clock_t start, end;
	#endif

	if(!chart)
		return;	//Invalid parameter

	#ifdef CCDEBUG
		start = clock();
	#endif

	//Sort the events
	for(track_ctr = 1; track_ctr < NUM_MIDI_TRACKS; track_ctr++)
	{	//For each of the instrument tracks that may have been built
		if(midi_track_events[track_ctr] == NULL)	//If there is no MIDI data for this track
			continue;	//Skip it

		#ifdef CCDEBUG
			(void) printf("\t\tSorting midi track \"%s\".\n", midi_track_name[track_ctr]);
		#endif

		for(event_ptr = midi_track_events[track_ctr], pre_count = 0; event_ptr != NULL; event_ptr = event_ptr->next, pre_count++);	//Count the number of events before sort
		if(pre_count < 2)	//If there aren't at least two links in the list
			return;			//There's no need to sort it

		sorted_list = malloc(sizeof(struct MIDIevent) * pre_count);	//Allocate enough memory to build a static array of event structures for this track
		if(sorted_list)
		{	//If it was allocated
			//Build the event array and sort it
			for(event_ptr = midi_track_events[track_ctr], index = 0; event_ptr != NULL; event_ptr = event_ptr->next, index++)
			{	//For each event in the list
				memcpy(&sorted_list[index], event_ptr, sizeof(struct MIDIevent));	//Copy it to the static array, the allocated dp arrays will be kept as-is
			}
			qsort(sorted_list, (size_t)pre_count, sizeof(struct MIDIevent), qsort_midi_events_comparitor);	//Quicksort the static array

			//Destroy the old linked list
			event_ptr = midi_track_events[track_ctr];
			while(event_ptr != NULL)
			{
				next_ptr = event_ptr->next;	//Store the address of the next link
				free(event_ptr);			//Free this link
				event_ptr = next_ptr;		//Point to next link
			}

			//Updated the sorted array's pointers to make it a proper linked list
			for(index = 0; index + 1 < pre_count; index++)
			{	//For each element in the event array before the last one
				sorted_list[index].next = &sorted_list[index + 1];
			}
			sorted_list[pre_count - 1].next = NULL;	//The final link in the list always points forward to NULL

			midi_track_events[track_ctr] = sorted_list;	//Replace the now empty global list with the sorted list
			events_reallocated[track_ctr] = 1;	//This track will have different cleanup behavior in destroy_feedback_chart()
		}
		else
		{
			(void) puts("\t\t\tMemory allocation failed, falling back to slower sort (deal with it).");
			insertion_sort_midi_events(chart);
			return;
		}
	}//For each of the instrument tracks that may have been built

	//Update the linked list pointers in the chart's track difficulties
	for(track_ptr = chart->tracks; track_ptr != NULL; track_ptr = track_ptr->next)
	{	//For each track in the chart
		if(track_ptr->tracktype < NUM_MIDI_TRACKS)
		{	//Bounds check
			track_ptr->events = midi_track_events[track_ptr->tracktype];	//Point the track to the current head of the associated instrument's event linked list
		}
	}

	#ifdef CCDEBUG
		end = clock();
		(void) printf("\t\tSort completed in %f seconds.\n", ((double)end - start) / (double)CLOCKS_PER_SEC);
	#endif
}

int qsort_midi_events_comparitor(const void * e1, const void * e2)
{
	const struct MIDIevent *this_ptr = e1;
	const struct MIDIevent *next_ptr = e2;

	if(!this_ptr || !next_ptr)
	{
		return 0;	//Invalid parameters
	}

	//Chronological order takes precedence in sorting
	if(next_ptr->pos < this_ptr->pos)
	{	//If the next event occurs before this event
		return 1;
	}
	if(this_ptr->pos < next_ptr->pos)
	{	//This event occurs before the next event
		return -1;	//This is the correct order
	}

	//Logical order of custom Sysex markers:  Phrase on, note on, note off, Phrase off
	if((this_ptr->type == 0xF0) && (next_ptr->type == 0x90))
	{	//If this event is a Sysex event and the next is a note on event
		if(!this_ptr->sysexon)
		{	//If this event is a phrase off Sysex marker or some other Sysex message
			return 1;
		}
		else
		{	//This event is a phrase on Sysex marker and the next event is a note on
			return -1;	//This is the correct order
		}
	}
	if((this_ptr->type == 0x90) && (next_ptr->type == 0xF0))
	{	//If this event is a note on event and the next is a Sysex event
		if(next_ptr->sysexon)
		{	//If the next event is a Sysex phrase on marker
			return 1;
		}
		else
		{	//This event is a note on and the next event is a phrase off Sysex marker
			return -1;	//This is the correct order
		}
	}
	if((this_ptr->type == 0x80) && (next_ptr->type == 0xF0))
	{	//If this event is a note off event and the next is a Sysex event
		if(next_ptr->sysexon)
		{	//If the next event is a Sysex phrase on marker
			return 1;
		}
		else
		{	//This event is a note off event and the next event is a phrase off Sysex marker
			return -1;	//This is the correct order
		}
	}
	if((this_ptr->type == 0xF0) && (next_ptr->type == 0x80))
	{	//If this event is a Sysex event and the next is a note off event
		if(!this_ptr->sysexon)
		{	//If this event is a phrase off Sysex marker or some other Sysex message
			return 1;
		}
		else
		{	//This event is a phrase on Sysex marker and the next event is a note off
			return -1;	//This is the correct order
		}
	}

	//Logical order of lyric markings:  Overdrive on, solo on, lyric on, lyric, lyric pitch, ..., lyric off, solo off, overdrive off
	if((this_ptr->type == 0x90) && (next_ptr->type == 0x90))
	{	//If this event and the next are both note on events
		if(next_ptr->note == 116)
		{	//If the next event is overdrive
			return 1;
		}
		if(this_ptr->note == 116)
		{	//If this event is overdrive
			return -1;	//This is the correct order
		}
		if(next_ptr->note == 105)
		{	//If the next event is a lyric phrase on marker
			return 1;
		}
		if(this_ptr->note == 105)
		{	//If this event is a lyric phrase on marker
			return -1;	//This is the correct order
		}

		//If two note on events occur at the same time for the same note, but one of them has a velocity of 0 (note off), sort the note off to be first
		if(this_ptr->note == next_ptr->note)
		{	//If both note on events are for the same note
			if(this_ptr->velocity && !next_ptr->velocity)
			{	//If this event is a note on and the next event is a note off
				return 1;
			}
			else if(!this_ptr->velocity && next_ptr->velocity)
			{	//This event is a note off and the next event is a note on
				return -1;	//This is the correct order
			}
		}
	}

	//Put lyric events first, except for a lyric phrase on marker
	if((this_ptr->type == 0x05) && (next_ptr->type == 0x90))
	{	//If this event is a lyric event and the next event is a note on
		if((next_ptr->note == 105) || (next_ptr->note == 106))
		{	//If the next event is a lyric phrase start
			return 1;
		}
		else
		{	//This event is a lyric and the next event is not a phrase start
			return -1;	//This is the correct order
		}
	}
	if((this_ptr->type == 0x90) && (next_ptr->type == 0x05))
	{	//If this event is a note on and the next event is a lyric event
		if((this_ptr->note == 105) || (this_ptr->note == 106))
		{	//If this event is a lyric phrase start
			return -1;	//This is the correct order
		}
		else
		{
			return 1;
		}
	}

	///Add sorting of pro drum markers when Clone Hero supports it

	//Put notes first and then markers, will numerically sort in this order:  lyric pitch, lyric off, overdrive off
	if(this_ptr->note > next_ptr->note)
	{	//If this event's note is higher than the next event's note
		return 1;
	}
	else if(this_ptr->note < next_ptr->note)
	{	//If this event's note is lower than the next event's note
		return -1;	//This is the correct order
	}

	//To avoid strange overlap problems, ensure that if a note on and a note off occur at the same time, the note off is written first
	if(this_ptr->note == next_ptr->note)
	{	//If both events are for the same note
		if((this_ptr->type == 0x90) && (next_ptr->type == 0x80))
		{	//If this event is a note on and the next event is a note off
			return 1;
		}
		else if((this_ptr->type == 0x80) && (next_ptr->type == 0x90))
		{	//If this event is a note off and the next event is a note on
			return -1;	//This is the correct order
		}
	}

	return 0;	//If no sort order has been determined by now, the sorting priority for these events is considered equal
}

void set_hopo_status(struct FeedbackChart *chart)
{
	struct dbTrack *track_ptr;
	struct dbNote *prev_note_ptr, *note_ptr, *next_note_ptr;
	unsigned long threshold, start, end;
	unsigned long prev_note_is_chord = 0, this_note_is_chord = 0;

	if(!chart)
		return;	//Invalid parameters

	threshold = (chart->resolution * 66) / 192;	//This is the distance at which a note is forced strum by default (unless the toggle HOPO marker is used)
	for(track_ptr = chart->tracks; track_ptr != NULL; track_ptr = track_ptr->next)
	{	//For each track in the chart
		//Initialize HOPO status based on threshold
		for(note_ptr = track_ptr->notes, prev_note_ptr = NULL; note_ptr != NULL; note_ptr = note_ptr->next)
		{	//For each note in the track
			if(prev_note_ptr && (prev_note_ptr->chartpos != note_ptr->chartpos))
			{	//If there was a previous gem and it was at a different position
				this_note_is_chord = 0;	//Reset this status for the current note
			}
			if(!note_ptr->is_gem)
			{	//If this is a status marker instead of a note
				if(note_ptr->next && (note_ptr->next->chartpos > note_ptr->chartpos))
				{	//If the next gem is at a different position
					prev_note_is_chord = this_note_is_chord;	//Track this
				}
				continue;	//Skip it and don't remember it for comparison
			}
			if(note_ptr->next && note_ptr->next->is_gem && (note_ptr->next->chartpos == note_ptr->chartpos))
			{	//If there is a next gem at the same position, this is a chord and cannot be a HOPO without use of the toggle HOPO marker
				this_note_is_chord = 1;	//Track this for HOPO detection (chords cannot become HOPOs due to proximity threshold alone)
			}
			if(!this_note_is_chord && prev_note_ptr)
			{	//If this note is a single gem, and there was a previous note
				if(note_ptr->chartpos - prev_note_ptr->chartpos < threshold)
				{	//If this note is within the HOPO threshold of the previous note
					if(prev_note_is_chord)
					{	//If the previous note is a chord, any single gem within the HOPO threshold qualifies as an auto HOPO
						note_ptr->is_hopo = 1;
					}
					else if(prev_note_ptr->gemcolor != note_ptr->gemcolor)
					{	//If the previous note is a single gem, but it isn't the same gem as this note
						note_ptr->is_hopo = 1;
					}
				}
			}

			if(note_ptr->next && (note_ptr->next->chartpos > note_ptr->chartpos))
			{	//If the next gem is at a different position
				prev_note_is_chord = this_note_is_chord;	//Track this
			}
			prev_note_ptr = note_ptr;	//Remember this note to compare against the next note
		}

		//Invert the HOPO status for any note within the scope of a toggle HOPO marker
		for(note_ptr = track_ptr->notes; note_ptr != NULL; note_ptr = note_ptr->next)
		{	//For each note in the track
			if(note_ptr->gemcolor == 5)
			{	//If this is a toggle HOPO marker
				start = note_ptr->chartpos;			//Obtain the scope of the marker
				end = start + note_ptr->duration;

				for(next_note_ptr = track_ptr->notes; next_note_ptr != NULL; next_note_ptr = next_note_ptr->next)
				{	//For each of the notes in the track (rewind to beginning of list because the affected note(s) may be defined before the toggle HOPO marker)
					if(next_note_ptr->chartpos > end)
					{	//If this and all remaining notes in the track are beyond the scope of the marker
						break;	//Exit inner for loop
					}
					if(!next_note_ptr->is_gem)
					{	//If this is a status marker instead of a note
						continue;	//Skip it
					}
					if(next_note_ptr->chartpos >= start)
					{	//If this note is within the scope of the marker
						if(next_note_ptr->is_hopo)
						{	//If the note was an automatic HOPO (due to threshold)
							next_note_ptr->is_hopo = 0;	//It isn't anymore
						}
						else
						{	//Otherwise it becomes a HOPO
							next_note_ptr->is_hopo = 1;
						}
					}
				}//For each of the remaining notes in the track
			}//If this is a toggle HOPO marker
		}//For each note in the track
	}
}

void set_slider_status(struct FeedbackChart *chart)
{
	struct dbTrack *track_ptr;
	struct dbNote *note_ptr, *note_ptr_2;
	unsigned long start, end;

	if(!chart)
		return;	//Invalid parameters

	for(track_ptr = chart->tracks; track_ptr != NULL; track_ptr = track_ptr->next)
	{	//For each track in the chart
		#ifdef CCDEBUG
			(void) printf("\t%s\n", track_ptr->trackname);
		#endif
		for(note_ptr = track_ptr->notes; note_ptr != NULL; note_ptr = note_ptr->next)
		{	//For each note in the track
			if(note_ptr->gemcolor == 6)
			{	//If this note is a slider marker
				start = note_ptr->chartpos;
				end = start + note_ptr->duration;

				for(note_ptr_2 = track_ptr->notes; note_ptr_2 != NULL; note_ptr_2 = note_ptr_2->next)
				{	//Re-parse from the beginning of the list, because it can't be assumed that the slider marker was defined before applicable notes
					if((note_ptr_2->gemcolor > 8) || (note_ptr_2->gemcolor == 5) || (note_ptr_2->gemcolor == 6))
					{	//If this is a status marker instead of a note
						continue;	//Skip it
					}
					if(note_ptr_2->chartpos > end)
					{	//If this and all remaining notes are after the slider marker
						break;	//Stop looking for notes in this slider marker's scope
					}
					if(note_ptr_2->chartpos >= start)
					{	//If this note is within the slider marker's scope
						note_ptr_2->is_slider = 1;
					}
				}
			}
		}
	}
}
