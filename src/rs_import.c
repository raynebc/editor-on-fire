#include <allegro.h>
#include "chart_import.h"
#include "foflc/Lyric_storage.h"	//For FindLongestLineLength()
#include "foflc/RS_parse.h"	//For XML parsing functions
#include "main.h"
#include "rs.h"
#include "rs_import.h"
#include "undo.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

#define RS_IMPORT_DEBUG

#define EOF_RS_PHRASE_IMPORT_LIMIT 200
#define EOF_RS_EVENT_IMPORT_LIMIT 200
#define EOF_RS_CHORD_TEMPLATE_IMPORT_LIMIT 400

int eof_parse_chord_template(char *name, size_t size, char *finger, char *frets, unsigned char *note, unsigned char *numstrings, unsigned long linectr, char *input)
{
	unsigned long ctr, bitmask;
	long output;
	int success_count;

	if(!name || !size || !finger || !frets || !note || !input)
	{
		eof_log("Invalid parameters sent to eof_parse_chord_template()", 1);
		return 1;	//Return error
	}

	//Read chord name
	if(!parse_xml_attribute_text(name, size, "chordName", input))
	{	//If the chord name could not be read
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading chord name on line #%lu.  Aborting", linectr);
		eof_log(eof_log_string, 1);
		return 2;	//Return error
	}

	//Read chord fingering
	success_count = 0;
	success_count += parse_xml_attribute_number("finger0", input, &output);	//Read the fingering for string 0
	finger[0] = output;	//Store the tuning
	success_count += parse_xml_attribute_number("finger1", input, &output);	//Read the fingering for string 1
	finger[1] = output;	//Store the tuning
	success_count += parse_xml_attribute_number("finger2", input, &output);	//Read the fingering for string 2
	finger[2] = output;	//Store the tuning
	success_count += parse_xml_attribute_number("finger3", input, &output);	//Read the fingering for string 3
	finger[3] = output;	//Store the tuning
	success_count += parse_xml_attribute_number("finger4", input, &output);	//Read the fingering for string 4
	finger[4] = output;	//Store the tuning
	success_count += parse_xml_attribute_number("finger5", input, &output);	//Read the fingering for string 5
	finger[5] = output;	//Store the tuning
	if(success_count < 4)
	{	//Rocksmith doesn't support arrangements using less than 4 strings
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading chord fingering on line #%lu.  Aborting", linectr);
		eof_log(eof_log_string, 1);
		return 3;	//Return error
	}
	if((success_count == 4) && !parse_xml_attribute_number("finger4", input, &output) && !parse_xml_attribute_number("finger5", input, &output))
	{	//If only four string fingerings were read, this must be a 4 string bass arrangement
		if(numstrings)
		{	//If the calling function wanted to track the number of strings read from the templates
			*numstrings = 4;	//Update the string count
		}
	}
	for(ctr = 0; ctr < 6; ctr++)
	{	//For each of the 6 supported strings
		if(finger[ctr] < 0)
		{	//If the fingering was given as a negative number, it is unused
			finger[ctr] = 0;	//Convert to EOF's notation for an unused/undefined finger
		}
	}

	//Read chord fretting
	success_count = 0;
	success_count += parse_xml_attribute_number("fret0", input, &output);	//Read the fretting for string 0
	frets[0] = output;	//Store the tuning
	success_count += parse_xml_attribute_number("fret1", input, &output);	//Read the fretting for string 1
	frets[1] = output;	//Store the tuning
	success_count += parse_xml_attribute_number("fret2", input, &output);	//Read the fretting for string 2
	frets[2] = output;	//Store the tuning
	success_count += parse_xml_attribute_number("fret3", input, &output);	//Read the fretting for string 3
	frets[3] = output;	//Store the tuning
	success_count += parse_xml_attribute_number("fret4", input, &output);	//Read the fretting for string 4
	frets[4] = output;	//Store the tuning
	success_count += parse_xml_attribute_number("fret5", input, &output);	//Read the fretting for string 5
	frets[5] = output;	//Store the tuning
	if(success_count < 4)
	{	//Rocksmith doesn't support arrangements using less than 4 strings
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading chord fretting on line #%lu.  Aborting", linectr);
		eof_log(eof_log_string, 1);
		return 4;	//Return error
	}
	for(ctr = 0, *note = 0, bitmask = 1; ctr < 6; ctr++, bitmask <<= 1)
	{	//For each of the 6 supported strings
		if(frets[ctr] < 0)
		{	//If the fingering was given as a negative number, it is unused
			frets[ctr] = 0;	//Convert to EOF's notation for an unused/undefined finger
		}
		else
		{	//Otherwise the string is used
			*note |= bitmask;	//Set the mask bit for this string
		}
	}

	return 0;	//Return success
}

EOF_PRO_GUITAR_TRACK *eof_load_rs(char * fn)
{
	char *buffer = NULL, *buffer2 = NULL;		//Will be an array large enough to hold the largest line of text from input file
	EOF_PRO_GUITAR_TRACK *tp = NULL;
	size_t maxlinelength;
	unsigned long linectr = 1, tagctr;
	PACKFILE *inf = NULL;
	char *ptr, *ptr2, tag[256];
	char error = 0, warning = 0;
	char *phraselist[EOF_RS_PHRASE_IMPORT_LIMIT] = {0};	//Will store the list of phrase names
	unsigned long phraselist_count = 0;	//Keeps count of how many phrases have been imported
	EOF_TEXT_EVENT *eventlist[EOF_RS_EVENT_IMPORT_LIMIT] = {0};	//Will store the list of imported RS phrases, RS sections and RS events
	unsigned long eventlist_count = 0;	//Keeps count of how many events have been imported
	EOF_PRO_GUITAR_NOTE *chordlist[EOF_RS_CHORD_TEMPLATE_IMPORT_LIMIT] = {0};	//Will store the list of chord templates
	unsigned long chordlist_count = 0;	//Keeps count of how many chord templates have been imported
	unsigned long beat_count = 0;		//Keeps count of which ebeat is being parsed
	unsigned long note_count = 0;		//Keeps count of how many notes have been imported
	unsigned long tech_note_count = 0;	//Keeps count of how many tech notes have been imported
	char strum_dir = 0;					//Tracks whether any chords were marked as up strums
	unsigned long ctr, ctr2, ctr3;

	if(!eof_song || !eof_song_loaded)
		return NULL;	//For now, don't do anything unless a project is active

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return NULL;	//Don't do anything unless the active track is a pro guitar/bass track

	eof_log("\tImporting Rocksmith file", 1);
	eof_log("eof_load_rs() entered", 1);

	//Initialize pointers and handles
	if(!fn)
	{
		return NULL;
	}
	inf = pack_fopen(fn, "rt");	//Open file in text mode
	if(!inf)
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError loading:  Cannot open input xml file:  \"%s\"", strerror(errno));	//Get the Operating System's reason for the failure
		eof_log(eof_log_string, 1);
		return NULL;
	}

	//Allocate memory buffers large enough to hold any line in this file
	maxlinelength = (size_t)FindLongestLineLength_ALLEGRO(fn, 0);
	if(!maxlinelength)
	{
		eof_log("\tError finding the largest line in the file.  Aborting", 1);
		(void) pack_fclose(inf);
		return NULL;
	}
	buffer = (char *)malloc(maxlinelength);
	buffer2 = (char *)malloc(maxlinelength);
	if(!buffer || !buffer2)
	{
		if(buffer)
		{	//If the first buffer was allocated
			free(buffer);
		}
		if(buffer2)
		{	//If the second buffer was allocated, not sure why this would happen if the first failed
			free(buffer2);
		}
		eof_log("\tError allocating memory.  Aborting", 1);
		(void) pack_fclose(inf);
		return NULL;
	}

	//Allocate and initialize a pro guitar structure
	tp = malloc(sizeof(EOF_PRO_GUITAR_TRACK));
	if(!tp)
	{
		eof_log("\tError allocating memory.  Aborting", 1);
		(void) pack_fclose(inf);
		free(buffer);
		free(buffer2);
		return NULL;
	}
	memset(tp, 0, sizeof(EOF_PRO_GUITAR_TRACK));	//Initialize memory block to 0 to avoid crashes when not explicitly setting counters that were newly added to the pro guitar structure
	tp->numstrings = 6;	//The number of strings that will be used in the arrangement is unknown, default to 6
	tp->numfrets = 22;	//The number of frets that will be used in the arrangement is unknown, default to 22
	tp->note = tp->pgnote;	//Put the regular pro guitar note array into effect
	tp->parent = eof_song->pro_guitar_track[eof_song->track[eof_selected_track]->tracknum]->parent;	//Initialize the parent so that the alternate track name can be set if appropriate

	//Read first line of text, capping it to prevent buffer overflow
	if(!pack_fgets(buffer, (int)maxlinelength, inf))
	{	//I/O error
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Rocksmith import failed on line #%lu:  Unable to read from file:  \"%s\"", linectr, strerror(errno));
		eof_log(eof_log_string, 1);
		error = 1;
	}

	//Parse the contents of the file
	while(!error && !pack_feof(inf))
	{	//Until there was an error reading from the file or end of file is reached
		#ifdef RS_IMPORT_DEBUG
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tProcessing line #%lu", linectr);
			eof_log(eof_log_string, 1);
		#endif

		//Separate the line into the opening XML tag (buffer) and the content between the opening and closing tag (buffer2)
		ptr = strcasestr_spec(buffer, ">");
		if(ptr)
		{	//If this line contained an XML tag
			strcpy(buffer2, ptr);	//Copy the portion of the buffer beginning after the opening tag
			ptr2 = strchr(buffer2, '<');
			if(ptr2)
			{	//If this line contains a closing XML tag
				*ptr2 = '\0';	//Drop it from buffer2
			}
			else
			{	//Otherwise empty buffer2
				buffer2[0] = '\0';
			}
			*ptr = '\0';	//Drop everything after the first tag from buffer
		}
		else
		{	//This line had no XML, skip it
			(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text, so the EOF condition can be checked
			linectr++;
			continue;
		}

		if(strcasestr_spec(buffer, "<scoreUrl>"))
		{	//If this is a scoreUrl tag (Go PlayAlong XML file)
			eof_log("This appears to be a Go PlayAlong XML file.  Aborting", 1);
			allegro_message("This appears to be a Go PlayAlong XML file.  Use \"File>Guitar Pro Import\" instead.");
			error = 1;
		}
		else if(strcasestr_spec(buffer, "<song>") || strcasestr_spec(buffer, "<song version"))
		{	//If this is the song tag
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);	//Make an undo state because the tempo map will be overwritten
			for(ctr = eof_song->text_events; ctr > 0; ctr--)
			{	//For each of the project's text events (in reverse order)
				if(eof_song->text_event[ctr - 1]->track == eof_selected_track)
				{	//If the text event is assigned to the track being replaced
					eof_song_delete_text_event(eof_song, ctr - 1);	//Delete it
				}
			}
		}
		else if(strcasestr_spec(buffer, "<vocals "))
		{	//If this is the vocals tag
			allegro_message("This is a lyric file, not a guitar or bass arrangement.\nUse File>Lyric Import to load this file.");
			eof_log("\tError:  This is a lyric file, not a guitar or bass arrangement.", 1);
			free(buffer);
			free(buffer2);
			free(tp);
			return NULL;
		}
		else if(strcasestr_spec(buffer, "<title>"))
		{	//If this is the title tag
			shrink_xml_text(tag, sizeof(tag), buffer2);	//Convert any escape sequences in the tag content to normal text
			strncpy(eof_song->tags->title, tag, sizeof(eof_song->tags->title) - 1);
		}
		else if(strcasestr_spec(buffer, "<arrangement>"))
		{	//If this is the arrangement tag
			char match = 0;

			shrink_xml_text(tag, sizeof(tag), buffer2);	//Convert any escape sequences in the tag content to normal text
			for(ctr = 1; ctr < EOF_TRACKS_MAX; ctr++)
			{	//Compare the arrangement name against EOF's natively supported tracks
				if(!ustricmp(tag, eof_midi_tracks[ctr].name))
				{	//If any native track name matches the arrangement name from this tag
					match = 1;
					break;
				}
			}
			if(!match)
			{	//If this arrangement name isn't one of EOF's native track names
				tp->parent->flags |= EOF_TRACK_FLAG_ALT_NAME;
				strncpy(tp->parent->altname, tag, sizeof(tp->parent->altname) - 1);

				//If the arrangement name matches one of the Rocksmith arrangement types
				if(!ustricmp(tag, "Combo"))
				{
					tp->arrangement = 1;
				}
				else if(!ustricmp(tag, "Rhythm"))
				{
					tp->arrangement = 2;
				}
				else if(!ustricmp(tag, "Lead"))
				{
					tp->arrangement = 3;
				}
				else if(!ustricmp(tag, "Bass"))
				{
					tp->arrangement = 4;
				}
			}
		}
		else if(strcasestr_spec(buffer, "<tuning"))
		{	//If this is the tuning tag
			int success_count = 0;
			long output;

			success_count += parse_xml_attribute_number("string0", buffer, &output);	//Read the value of string 0's tuning from the opening tag
			tp->tuning[0] = output;	//Store the tuning
			success_count += parse_xml_attribute_number("string1", buffer, &output);	//Read the value of string 1's tuning from the opening tag
			tp->tuning[1] = output;	//Store the tuning
			success_count += parse_xml_attribute_number("string2", buffer, &output);	//Read the value of string 2's tuning from the opening tag
			tp->tuning[2] = output;	//Store the tuning
			success_count += parse_xml_attribute_number("string3", buffer, &output);	//Read the value of string 3's tuning from the opening tag
			tp->tuning[3] = output;	//Store the tuning
			success_count += parse_xml_attribute_number("string4", buffer, &output);	//Read the value of string 4's tuning from the opening tag
			tp->tuning[4] = output;	//Store the tuning
			success_count += parse_xml_attribute_number("string5", buffer, &output);	//Read the value of string 5's tuning from the opening tag
			tp->tuning[5] = output;	//Store the tuning

			if(success_count != 6)
			{	//If 6 string tunings were not read
				eof_log("Error reading tuning values from XML.  Aborting", 1);
				error = 1;
				break;
			}
		}
		else if(strcasestr_spec(buffer, "<capo>"))
		{	//If this is the capo tag (RS2)
			long capo;
			if((buffer2[0] != '\0') && (buffer2[0] != '0'))
			{	//If a nonzero capo is defined
				capo = atol(buffer2);
				if(capo > 0)
				{	//If the number was successfully read
					tp->capo = capo;
				}
			}
		}
		else if(strcasestr_spec(buffer, "<artistName>"))
		{	//If this is the artistName tag
			shrink_xml_text(tag, sizeof(tag), buffer2);	//Convert any escape sequences in the tag content to normal text
			strncpy(eof_song->tags->artist, tag, sizeof(eof_song->tags->artist) - 1);
		}
		else if(strcasestr_spec(buffer, "<albumName>"))
		{	//If this is the albumName tag
			shrink_xml_text(tag, sizeof(tag), buffer2);	//Convert any escape sequences in the tag content to normal text
			strncpy(eof_song->tags->album, tag, sizeof(eof_song->tags->album) - 1);
		}
		else if(strcasestr_spec(buffer, "<albumYear>"))
		{	//If this is the albumYear tag
			shrink_xml_text(tag, sizeof(tag), buffer2);	//Convert any escape sequences in the tag content to normal text
			strncpy(eof_song->tags->year, tag, sizeof(eof_song->tags->year) - 1);
		}
		else if(strcasestr_spec(buffer, "<phrases") && !strstr(buffer, "/>"))
		{	//If this is the phrases tag and it isn't empty
			#ifdef RS_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tProcessing <phrases> tag on line #%lu", linectr);
				eof_log(eof_log_string, 1);
			#endif

			(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
			linectr++;
			while(!error || !pack_feof(inf))
			{	//Until there was an error reading from the file or end of file is reached
				if(strcasestr_spec(buffer, "</phrases"))
				{	//If this is the end of the phrases tag
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t%lu phrases loaded", phraselist_count);
					eof_log(eof_log_string, 1);
					break;	//Break from loop
				}
				if(phraselist_count < EOF_RS_PHRASE_IMPORT_LIMIT)
				{	//If another phrase name can be stored
					if(!parse_xml_attribute_text(tag, sizeof(tag), "name", buffer))
					{	//If the phrase name could not be read
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading phrase name on line #%lu.  Aborting", linectr);
						eof_log(eof_log_string, 1);
						error = 1;
						break;	//Break from inner loop
					}
					phraselist[phraselist_count] = malloc(strlen(tag) + 1);	//Allocate memory to store the phrase name
					if(!phraselist[phraselist_count])
					{
						eof_log("\tError allocating memory.  Aborting", 1);
						error = 1;
						break;	//Break from inner loop
					}
					strcpy(phraselist[phraselist_count], tag);
					phraselist_count++;
				}
				else
				{	//Otherwise the phrase limit has been exceeded
					eof_log("\t\tError:  Phrase limit exceeded.  Aborting", 1);
					error = 1;
					break;	//Break from inner loop
				}

				(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
				linectr++;	//Increment line counter
			}
			if(error)
				break;	//Break from outer loop
		}//If this is the phrases tag and it isn't empty
		else if(strcasestr_spec(buffer, "<phraseIterations") && !strstr(buffer, "/>"))
		{	//If this is the phraseIterations tag and it isn't empty
			long timestamp, id;
			unsigned long phraseitctr = 0;

			#ifdef RS_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tProcessing <phraseIterations> tag on line #%lu", linectr);
				eof_log(eof_log_string, 1);
			#endif

			(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
			linectr++;
			while(!error || !pack_feof(inf))
			{	//Until there was an error reading from the file or end of file is reached
				if(strcasestr_spec(buffer, "</phraseIterations"))
				{	//If this is the end of the phraseIterations tag
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t%lu phrase iterations loaded", phraseitctr);
					eof_log(eof_log_string, 1);
					break;	//Break from loop
				}
				if(strcasestr_spec(buffer, "<heroLevels"))
				{	//If this is the start of a heroLevels tag, ignore all lines of XML until the end of the tag is reached
					#ifdef RS_IMPORT_DEBUG
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tIgnoring <heroLevels> tag on line #%lu", linectr);
						eof_log(eof_log_string, 1);
					#endif
				}
				else if(strcasestr_spec(buffer, "<heroLevel "))
				{	//Ignore a heroLevel tag
				}
				else if(strcasestr_spec(buffer, "</heroLevels"))
				{	//Ignore the end of a heroLevels tag
				}
				else if(strcasestr_spec(buffer, "</phraseIteration>"))
				{	//Ignore the end of a phraseIteration tag, which would occur after a heroLevels tag
				}
				else
				{
					if(eventlist_count < EOF_RS_EVENT_IMPORT_LIMIT)
					{	//If another text event can be stored
						if(!parse_xml_rs_timestamp("time", buffer, &timestamp))
						{	//If the timestamp was not readable
							(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading timestamp on line #%lu.  Aborting", linectr);
							eof_log(eof_log_string, 1);
							error = 1;
							break;	//Break from inner loop
						}
						if(!parse_xml_attribute_number("phraseId", buffer, &id))
						{	//If the phrase ID was not readable
							(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading phrase ID on line #%lu.  Aborting", linectr);
							eof_log(eof_log_string, 1);
							error = 1;
							break;	//Break from inner loop
						}
						if(id >= phraselist_count)
						{	//If this phrase ID was not defined in the phrase tag
							(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError:  Invalid phrase ID on line #%lu.  Aborting", linectr);
							eof_log(eof_log_string, 1);
							error = 1;
							break;	//Break from inner loop
						}
						eventlist[eventlist_count] = malloc(sizeof(EOF_TEXT_EVENT));	//Allocate memory to store the text event
						if(!eventlist[eventlist_count])
						{
							eof_log("\tError allocating memory.  Aborting", 1);
							error = 1;
							break;	//Break from inner loop
						}
						strncpy(eventlist[eventlist_count]->text, phraselist[id], sizeof(eventlist[eventlist_count]->text) - 1);	//Copy the phrase name
						eventlist[eventlist_count]->track = eof_selected_track;
						eventlist[eventlist_count]->flags = EOF_EVENT_FLAG_RS_PHRASE;
						eventlist[eventlist_count]->beat = timestamp;	//Store the real timestamp, it will need to be converted to the beat number later
						phraseitctr++;
						eventlist_count++;
					}
					else
					{	//Otherwise the text event limit has been exceeded
						eof_log("\t\tError:  Text event limit exceeded.  Aborting", 1);
						error = 1;
						break;	//Break from inner loop
					}
				}

				(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
				linectr++;	//Increment line counter
			}
			if(error)
				break;	//Break from outer loop
		}//If this is the phraseIterations tag and it isn't empty
		else if(strcasestr_spec(buffer, "<chordTemplates") && !strstr(buffer, "/>"))
		{	//If this is the chordTemplates tag and it isn't empty
			char finger[8] = {0};
			char frets[8] = {0};
			unsigned char note;

			#ifdef RS_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tProcessing <chordTemplates> tag on line #%lu", linectr);
				eof_log(eof_log_string, 1);
			#endif

			(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
			linectr++;
			while(!error || !pack_feof(inf))
			{	//Until there was an error reading from the file or end of file is reached
				if(strcasestr_spec(buffer, "</chordTemplates"))
				{	//If this is the end of the chordTemplates tag
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t%lu chord templates loaded", chordlist_count);
					eof_log(eof_log_string, 1);
					break;	//Break from loop
				}
				if(chordlist_count < EOF_RS_CHORD_TEMPLATE_IMPORT_LIMIT)
				{	//If another chord can be stored
					if(eof_parse_chord_template(tag, sizeof(tag), finger, frets, &note, &(tp->numstrings), linectr, buffer))
					{	//If there was an error reading the chord template
						error = 1;
						break;
					}
					//Track the highest used fret number
					for(ctr = 0; ctr < 6; ctr++)
					{	//For each of the 6 supported strings
						if(frets[ctr] >= tp->capo)
							frets[ctr] -= tp->capo;		//Apply the capo if applicable
						if(frets[ctr] > tp->numfrets)
							tp->numfrets = frets[ctr];	//Track the highest used fret number
					}

					//Add chord template to list
					chordlist[chordlist_count] = malloc(sizeof(EOF_PRO_GUITAR_NOTE));
					if(!chordlist[chordlist_count])
					{
						eof_log("\tError allocating memory.  Aborting", 1);
						error = 1;
						break;	//Break from inner loop
					}
					memset(chordlist[chordlist_count], 0, sizeof(EOF_PRO_GUITAR_NOTE));	//Initialize memory block to 0
					strncpy(chordlist[chordlist_count]->name, tag, sizeof(chordlist[chordlist_count]->name) - 1);	//Store the chord name
					memcpy(chordlist[chordlist_count]->finger, finger, 8);	//Store the finger array
					memcpy(chordlist[chordlist_count]->frets, frets, 8);	//Store the fret array
					chordlist[chordlist_count]->note = note;	//Store the note mask
					chordlist_count++;
				}//If another chord can be stored
				else
				{	//Otherwise the chord limit has been exceeded
					eof_log("\t\tError:  Chord template limit exceeded.  Aborting", 1);
					error = 1;
					break;	//Break from inner loop
				}

				(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
				linectr++;	//Increment line counter
			}//Until there was an error reading from the file or end of file is reached
			if(error)
				break;	//Break from outer loop
		}//If this is the chordTemplates tag and it isn't empty
		else if(strcasestr_spec(buffer, "<ebeats") && !strstr(buffer, "/>"))
		{	//If this is the ebeats tag and it isn't empty
			long output;

			#ifdef RS_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tProcessing <ebeats> tag on line #%lu", linectr);
				eof_log(eof_log_string, 1);
			#endif

			(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
			linectr++;
			while(!error || !pack_feof(inf))
			{	//Until there was an error reading from the file or end of file is reached
				if(strcasestr_spec(buffer, "</ebeats"))
				{	//If this is the end of the ebeats tag
					for(ctr = eof_song->beats; ctr > beat_count; ctr--)
					{	//For each of the remaining beats in the project (which weren't initialized), in reverse order
						eof_song_delete_beat(eof_song, ctr - 1);	//Delete it.  eof_truncate_chart() will be run by the calling function to add beats as necessary
					}
					break;	//Break from loop
				}
				if(!parse_xml_rs_timestamp("time", buffer, &output))
				{	//If the timestamp was not readable
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading timestamp on line #%lu.  Aborting", linectr);
					eof_log(eof_log_string, 1);
					error = 1;
					break;	//Break from inner loop
				}
				if(beat_count >= eof_song->beats)
				{	//If a beat has to be added to the project
					if(!eof_song_add_beat(eof_song))
					{	//If there was an error adding a beat
						eof_log("\tError adding beat.  Aborting", 1);
						error = 1;
						break;	//Break from inner loop
					}
				}
				eof_song->beat[beat_count]->pos = output;			//Store the integer timestamp
				eof_song->beat[beat_count]->fpos = (double)output;	//Store the floating point timestamp
				if(!beat_count)
				{	//If this is the first beat, update the chart's MIDI delay
					eof_song->tags->ogg[eof_selected_ogg].midi_offset = output;
				}
				beat_count++;

				(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
				linectr++;	//Increment line counter
			}//Until there was an error reading from the file or end of file is reached
			if(error)
				break;	//Break from outer loop

			eof_calculate_tempo_map(eof_song);	//Determine all tempo changes based on the beats' timestamps
			if(!beat_count)
			{	//If no beats were parsed
				eof_log("No valid beat tags found.  Aborting", 1);
				error = 1;
				break;
			}
		}//If this is the ebeats tag and it isn't empty
		else if(strcasestr_spec(buffer, "<controls") && !strstr(buffer, "/>"))
		{	//If this is the controls tag and it isn't empty
			long output;

			#ifdef RS_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tProcessing <controls> tag on line #%lu", linectr);
				eof_log(eof_log_string, 1);
			#endif

			(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
			linectr++;
			while(!error || !pack_feof(inf))
			{	//Until there was an error reading from the file or end of file is reached
				if(strcasestr_spec(buffer, "</controls"))
				{	//If this is the end of the controls tag
					break;	//Break from loop
				}
				if(!parse_xml_rs_timestamp("time", buffer, &output))
				{	//If the timestamp was not readable
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading timestamp on line #%lu.  Aborting", linectr);
					eof_log(eof_log_string, 1);
					error = 1;
					break;	//Break from inner loop
				}
				ptr = strcasestr_spec(buffer, "ShowMessageBox(");
				if(ptr)
				{	//If this is a start of a popup message
					if(tp->popupmessages < EOF_MAX_PHRASES)
					{	//If another popup message can be stored
						ptr = strcasestr_spec(ptr, ",");	//The message text begins after the first comma in the XML tag
						if(!ptr)
						{
							(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading start of control message on line #%lu.  Aborting", linectr);
							eof_log(eof_log_string, 1);
							error = 1;
							break;	//Break from inner loop
						}
						ptr2 = strchr(ptr, ')');	//The message text ends at the first closing parenthesis after the start of the message
						if(!ptr2)
						{
							(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading end of control message on line #%lu.  Aborting", linectr);
							eof_log(eof_log_string, 1);
							error = 1;
							break;	//Break from inner loop
						}
						*ptr2 = '\0';	//Truncate the buffer
						tp->popupmessage[tp->popupmessages].start_pos = output;	//Store the starting timestamp
						tp->popupmessage[tp->popupmessages].end_pos = 0;		//Use 0 to indicate an un-ended popup message
						strncpy(tp->popupmessage[tp->popupmessages].name, ptr, sizeof(tp->popupmessage[tp->popupmessages].name) - 1);	//Store the message text
						tp->popupmessages++;
					}
				}
				else if((ptr = strcasestr_spec(buffer, "ClearAllMessageBoxes()")) && ptr)
				{	//If this is the end of any popup messages that are not ended yet
					for(ctr = 0; ctr < tp->popupmessages; ctr++)
					{	//For each popup message that has been imported so far
						if(!tp->popupmessage[ctr].end_pos)
						{	//If the message hasn't been ended
							tp->popupmessage[ctr].end_pos = output;		//Store the ending timestamp
						}
					}
				}
				else if((ptr = strcasestr_spec(buffer, "CDlcTone(")) && ptr)
				{	//if this is a tone change
					if(tp->tonechanges < EOF_MAX_PHRASES)
					{	//If another tone change can be stored
						if(!ptr)
						{
							(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading start of control message on line #%lu.  Aborting", linectr);
							eof_log(eof_log_string, 1);
							error = 1;
							break;	//Break from inner loop
						}
						ptr2 = strchr(ptr, ')');	//The tone key name ends at the first closing parenthesis after the start of the name
						if(!ptr2)
						{
							(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading end of control message on line #%lu.  Aborting", linectr);
							eof_log(eof_log_string, 1);
							error = 1;
							break;	//Break from inner loop
						}
						*ptr2 = '\0';	//Truncate the buffer
						tp->tonechange[tp->tonechanges].start_pos = output;		//Store the starting timestamp
						tp->tonechange[tp->tonechanges].end_pos = 0;			//Tone changes don't use an end position
						strncpy(tp->tonechange[tp->tonechanges].name, ptr, sizeof(tp->tonechange[tp->tonechanges].name) - 1);	//Store the message text
						tp->tonechanges++;
					}
				}
				else
				{
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError:  Unrecognized control message on line #%lu.  Aborting", linectr);
					eof_log(eof_log_string, 1);
					error = 1;
					break;	//Break from inner loop
				}

				(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
				linectr++;	//Increment line counter
			}//Until there was an error reading from the file or end of file is reached
			if(error)
				break;	//Break from outer loop

			for(ctr = 0; ctr < tp->popupmessages; ctr++)
			{	//For each popup message that was imported
				if(!tp->popupmessage[ctr].end_pos)
				{	//If the message hasn't been ended
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tWarning:  Control message #%lu was not ended.  Correcting", ctr);
					eof_log(eof_log_string, 1);
					tp->popupmessage[ctr].end_pos = tp->popupmessage[ctr].start_pos + 1;
				}
			}
		}//If this is the controls tag and it isn't empty
		else if(strcasestr_spec(buffer, "<tonebase"))
		{	//If this is the tonebase tag (RS2)
			shrink_xml_text(tag, sizeof(tag), buffer2);	//Convert any escape sequences in the tag content to normal text
			strncpy(tp->defaulttone, tag, sizeof(tp->defaulttone) - 1);
		}
		else if(strcasestr_spec(buffer, "<tones") && !strstr(buffer, "/>"))
		{	//If this is the tones tag (RS2) and it isn't empty
			long output;
			unsigned long tonectr = 0;

			#ifdef RS_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tProcessing <tones> tag on line #%lu", linectr);
				eof_log(eof_log_string, 1);
			#endif

			(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
			linectr++;
			while(!error || !pack_feof(inf))
			{	//Until there was an error reading from the file or end of file is reached
				if(strcasestr_spec(buffer, "</tones"))
				{	//If this is the end of the tones tag
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t%lu tones loaded", tonectr);
					eof_log(eof_log_string, 1);
					break;	//Break from loop
				}
				if(tonectr < EOF_MAX_PHRASES)
				{	//If another tone change can be stored
					if(!parse_xml_rs_timestamp("time", buffer, &output))
					{	//If the timestamp was not readable
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading timestamp on line #%lu.  Aborting", linectr);
						eof_log(eof_log_string, 1);
						error = 1;
						break;	//Break from inner loop
					}
					if(!parse_xml_attribute_text(tag, sizeof(tag), "name", buffer))
					{	//If the section name could not be read
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading tone name on line #%lu.  Aborting", linectr);
						eof_log(eof_log_string, 1);
						error = 1;
						break;	//Break from inner loop
					}
					tp->tonechange[tp->tonechanges].start_pos = output;		//Store the starting timestamp
					strncpy(tp->tonechange[tp->tonechanges].name, tag, sizeof(tp->tonechange[tp->tonechanges].name) - 1);	//Store the tone name
					tp->tonechanges++;
				}
				else
				{	//Otherwise the tone change limit has been exceeded
					eof_log("\t\tError:  Tone change limit exceeded.  Aborting", 1);
					error = 1;
					break;	//Break from inner loop
				}

				(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
				linectr++;	//Increment line counter
			}
			if(error)
				break;	//Break from outer loop
		}//If this is the tones tag (RS2) and it isn't empty
		else if(strcasestr_spec(buffer, "<sections") && !strstr(buffer, "/>"))
		{	//If this is the sections tag and it isn't empty
			long output;
			unsigned long sectionctr = 0;

			#ifdef RS_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tProcessing <sections> tag on line #%lu", linectr);
				eof_log(eof_log_string, 1);
			#endif

			(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
			linectr++;
			while(!error || !pack_feof(inf))
			{	//Until there was an error reading from the file or end of file is reached
				if(strcasestr_spec(buffer, "</sections"))
				{	//If this is the end of the sections tag
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t%lu sections loaded", sectionctr);
					eof_log(eof_log_string, 1);
					break;	//Break from loop
				}
				if(eventlist_count < EOF_RS_EVENT_IMPORT_LIMIT)
				{	//If another text event can be stored
					if(!parse_xml_attribute_text(tag, sizeof(tag), "name", buffer))
					{	//If the section name could not be read
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading section name on line #%lu.  Aborting", linectr);
						eof_log(eof_log_string, 1);
						error = 1;
						break;	//Break from inner loop
					}
					if(!eof_rs_section_text_valid(tag))
					{	//If this isn't a valid name for a RS section
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tWarning:  Invalid section name on line #%lu.", linectr);
						eof_log(eof_log_string, 1);
					}
					if(!parse_xml_rs_timestamp("startTime", buffer, &output))
					{	//If the timestamp was not readable
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading timestamp on line #%lu.  Aborting", linectr);
						eof_log(eof_log_string, 1);
						error = 1;
						break;	//Break from inner loop
					}
					eventlist[eventlist_count] = malloc(sizeof(EOF_TEXT_EVENT));	//Allocate memory to store the text event
					if(!eventlist[eventlist_count])
					{
						eof_log("\tError allocating memory.  Aborting", 1);
						error = 1;
						break;	//Break from inner loop
					}
					strncpy(eventlist[eventlist_count]->text, tag, sizeof(eventlist[eventlist_count]->text) - 1);	//Copy the section name
					eventlist[eventlist_count]->track = eof_selected_track;
					eventlist[eventlist_count]->flags = EOF_EVENT_FLAG_RS_SECTION;
					eventlist[eventlist_count]->beat = output;	//Store the real timestamp, it will need to be converted to the beat number later
					sectionctr++;
					eventlist_count++;
				}
				else
				{	//Otherwise the text event limit has been exceeded
					eof_log("\t\tError:  Text event limit exceeded.  Aborting", 1);
					error = 1;
					break;	//Break from inner loop
				}

				(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
				linectr++;	//Increment line counter
			}
			if(error)
				break;	//Break from outer loop
		}//If this is the sections tag and it isn't empty
		else if(strcasestr_spec(buffer, "<events") && !strstr(buffer, "/>"))
		{	//If this is the events tag and it isn't empty
			long output;
			unsigned long eventctr = 0;

			#ifdef RS_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tProcessing <events> tag on line #%lu", linectr);
				eof_log(eof_log_string, 1);
			#endif

			(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
			linectr++;
			while(!error || !pack_feof(inf))
			{	//Until there was an error reading from the file or end of file is reached
				if(strcasestr_spec(buffer, "</events"))
				{	//If this is the end of the events tag
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t%lu events loaded", eventctr);
					eof_log(eof_log_string, 1);
					break;	//Break from loop
				}
				if(eventlist_count < EOF_RS_EVENT_IMPORT_LIMIT)
				{	//If another text event can be stored
					if(!parse_xml_rs_timestamp("time", buffer, &output))
					{	//If the timestamp was not readable
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading timestamp on line #%lu.  Aborting", linectr);
						eof_log(eof_log_string, 1);
						error = 1;
						break;	//Break from inner loop
					}
					if(!parse_xml_attribute_text(tag, sizeof(tag), "code", buffer))
					{	//If the event code could not be read
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading event code on line #%lu.  Aborting", linectr);
						eof_log(eof_log_string, 1);
						error = 1;
						break;	//Break from inner loop
					}
					eventlist[eventlist_count] = malloc(sizeof(EOF_TEXT_EVENT));	//Allocate memory to store the text event
					if(!eventlist[eventlist_count])
					{
						eof_log("\tError allocating memory.  Aborting", 1);
						error = 1;
						break;	//Break from inner loop
					}
					strncpy(eventlist[eventlist_count]->text, tag, sizeof(eventlist[eventlist_count]->text) - 1);	//Copy the event code
					eventlist[eventlist_count]->track = eof_selected_track;
					eventlist[eventlist_count]->flags = EOF_EVENT_FLAG_RS_EVENT;
					eventlist[eventlist_count]->beat = output;	//Store the real timestamp, it will need to be converted to the beat number later
					eventctr++;
					eventlist_count++;
				}
				else
				{	//Otherwise the text event limit has been exceeded
					eof_log("\t\tError:  Text event limit exceeded.  Aborting", 1);
					error = 1;
					break;	//Break from inner loop
				}

				(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
				linectr++;	//Increment line counter
			}
			if(error)
				break;	//Break from outer loop
		}//If this is the events tag and it isn't empty
		else if(strcasestr_spec(buffer, "<levels") && !strstr(buffer, "/>"))
		{	//If this is the levels tag and it isn't empty
			long curdiff, time, bend, fret, hammeron, harmonic, palmmute, pluck, pulloff, slap, slideto, string, sustain, tremolo;
			long linknext, accent, mute, pinchharmonic, slideunpitchto, tap, vibrato, step;
			unsigned long flags;
			EOF_PRO_GUITAR_NOTE *np = NULL, *tnp = NULL;

			#ifdef RS_IMPORT_DEBUG
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tProcessing <levels> tag on line #%lu", linectr);
				eof_log(eof_log_string, 1);
			#endif

			(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
			linectr++;
			while(!error || !pack_feof(inf))
			{	//Until there was an error reading from the file or end of file is reached
				if(strcasestr_spec(buffer, "</levels"))
				{	//If this is the end of the levels tag
					break;	//Break from loop
				}

				//Read level tag
				ptr = strcasestr_spec(buffer, "<level difficulty");
				if(ptr)
				{	//If this is a level tag

					#ifdef RS_IMPORT_DEBUG
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\tProcessing <level> tag on line #%lu", linectr);
						eof_log(eof_log_string, 1);
					#endif

					if(!parse_xml_attribute_number("difficulty", buffer, &curdiff))
					{	//If the difficulty number was not readable
						(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading difficulty number on line #%lu.  Aborting", linectr);
						eof_log(eof_log_string, 1);
						error = 1;
						break;	//Break from inner loop
					}
					(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
					linectr++;
					while(!error || !pack_feof(inf))
					{	//Until there was an error reading from the file or end of file is reached
						if(strcasestr_spec(buffer, "</level>"))
						{	//If this is the end of the level tag
							break;	//Break from loop
						}

						//Read notes tag
						if(strcasestr_spec(buffer, "<notes") && !strstr(buffer, "/>"))
						{	//If this is the notes tag and it isn't empty
							#ifdef RS_IMPORT_DEBUG
								(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\tProcessing <notes> tag on line #%lu", linectr);
								eof_log(eof_log_string, 1);
							#endif

							tagctr = 0;
							(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
							linectr++;
							while(!error || !pack_feof(inf))
							{	//Until there was an error reading from the file or end of file is reached
								if(strcasestr_spec(buffer, "</notes"))
								{	//If this is the end of the notes tag
									#ifdef RS_IMPORT_DEBUG
										(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\t\tAdded %lu notes", tagctr);
										eof_log(eof_log_string, 1);
									#endif
									break;	//Break from loop
								}

								//Read note tag
								ptr = strcasestr_spec(buffer, "<note ");
								if(ptr)
								{	//If this is a note tag
									if(note_count < EOF_MAX_NOTES)
									{	//If another note can be stored
										bend = fret = hammeron = harmonic = palmmute = pulloff = string = tremolo = flags = linknext = accent = mute = pinchharmonic = tap = vibrato = 0;	//Init all of these to undefined
										pluck = slap = slideto = slideunpitchto = -1;	//Init these to Rocksmith's undefined value

										//Read note attributes
										if(!parse_xml_rs_timestamp("time", buffer, &time))
										{	//If the timestamp was not readable
											(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading start timestamp on line #%lu.  Aborting", linectr);
											eof_log(eof_log_string, 1);
											error = 1;
											break;	//Break from inner loop
										}
										(void) parse_xml_attribute_number("bend", buffer, &bend);
										(void) parse_xml_attribute_number("fret", buffer, &fret);
										if(fret >= tp->capo)
											fret -= tp->capo;	//Apply the capo if applicable
										(void) parse_xml_attribute_number("hammerOn", buffer, &hammeron);
										(void) parse_xml_attribute_number("harmonic", buffer, &harmonic);
										(void) parse_xml_attribute_number("palmMute", buffer, &palmmute);
										(void) parse_xml_attribute_number("pluck", buffer, &pluck);
										(void) parse_xml_attribute_number("pullOff", buffer, &pulloff);
										(void) parse_xml_attribute_number("slap", buffer, &slap);
										(void) parse_xml_attribute_number("slideTo", buffer, &slideto);
										if(slideto >= tp->capo)
											slideto -= tp->capo;	//Apply the capo if applicable
										(void) parse_xml_attribute_number("string", buffer, &string);
										if(!parse_xml_rs_timestamp("sustain", buffer, &sustain))
										{	//If the timestamp was not readable
											(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading sustain time on line #%lu.  Aborting", linectr);
											eof_log(eof_log_string, 1);
											error = 1;
											break;	//Break from inner loop
										}
										(void) parse_xml_attribute_number("tremolo", buffer, &tremolo);

										//Read RS2 note attributes
										(void) parse_xml_attribute_number("linkNext", buffer, &linknext);
										(void) parse_xml_attribute_number("accent", buffer, &accent);
										(void) parse_xml_attribute_number("mute", buffer, &mute);
										(void) parse_xml_attribute_number("harmonicPinch", buffer, &pinchharmonic);
										(void) parse_xml_attribute_number("slideUnpitchTo", buffer, &slideunpitchto);
										if(slideunpitchto >= tp->capo)
											slideunpitchto -= tp->capo;	//Apply the capo if applicable
										(void) parse_xml_attribute_number("tap", buffer, &tap);
										(void) parse_xml_attribute_number("vibrato", buffer, &vibrato);

										//Add note and set attributes
										if((string >= 0) && (string < 6))
										{	//As long as the string number is valid
											np = eof_pro_guitar_track_add_note(tp);	//Allocate, initialize and add the new note to the note array
											if(!np)
											{
												eof_log("\tError allocating memory.  Aborting", 1);
												error = 1;
												break;	//Break from inner loop
											}
											np->type = curdiff;
											np->note = 1 << (unsigned long) string;
											np->frets[(unsigned long) string] = fret;
											np->pos = time;
											np->length = sustain;
											if(bend)
											{
												flags |= EOF_PRO_GUITAR_NOTE_FLAG_BEND;
												flags |= EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION;
												np->bendstrength = bend;
											}
											if(hammeron)
												flags |= EOF_PRO_GUITAR_NOTE_FLAG_HO;
											if(harmonic)
												flags |= EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC;
											if(palmmute)
												flags |= EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE;
											if(pulloff)
												flags |= EOF_PRO_GUITAR_NOTE_FLAG_PO;
											if(tremolo)
												flags |= EOF_NOTE_FLAG_IS_TREMOLO;
											if(pluck > 0)
												flags |= EOF_PRO_GUITAR_NOTE_FLAG_POP;
											if(slap > 0)
												flags |= EOF_PRO_GUITAR_NOTE_FLAG_SLAP;
											if(slideto > 0)
											{
												flags |= EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION;
												np->slideend = slideto;
												if(slideto > fret)
												{	//The slide ends above the starting fret
													flags |= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP;
												}
												else
												{	//The slide ends below the starting fret
													flags |= EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN;
												}
											}
											if(linknext)
												flags |= EOF_PRO_GUITAR_NOTE_FLAG_LINKNEXT;
											if(accent)
												flags |= EOF_PRO_GUITAR_NOTE_FLAG_ACCENT;
                                            if(mute)
												flags |= EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE;
                                            if(pinchharmonic)
												flags |= EOF_PRO_GUITAR_NOTE_FLAG_P_HARMONIC;
                                            if(slideunpitchto > 0)
											{
												flags |= EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE;
												np->unpitchend = slideunpitchto;
											}
											if(tap)
												flags |= EOF_PRO_GUITAR_NOTE_FLAG_TAP;
                                            if(vibrato)
												flags |= EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO;
											np->flags = flags;
											note_count++;
											tagctr++;

											if(fret > tp->numfrets)
												tp->numfrets = fret;	//Track the highest used fret number
										}//As long as the string number is valid
									}//If another note can be stored
								}//If this is a note tag

								//Read bendValue tag
								ptr = strcasestr_spec(buffer, "<bendValue ");
								if(ptr && np)
								{	//If this is a bendValue tag and a note was read
									if(tech_note_count < EOF_MAX_NOTES)
									{	//If another tech note can be stored
										if(!parse_xml_rs_timestamp("time", buffer, &time))
										{	//If the timestamp was not readable
											(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading start timestamp on line #%lu.  Aborting", linectr);
											eof_log(eof_log_string, 1);
											error = 1;
											break;	//Break from inner loop
										}
										if(!parse_xml_rs_timestamp("step", buffer, &step))
										{	//If the bend strength was not readable
											(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading bend strength on line #%lu.  Aborting", linectr);
											eof_log(eof_log_string, 1);
											error = 1;
											break;	//Break from inner loop
										}
										tnp = eof_pro_guitar_track_add_tech_note(tp);
										if(!tnp)
										{
											eof_log("\tError allocating memory.  Aborting", 1);
											error = 1;
											break;	//Break from inner loop
										}
										tnp->type = np->type;	//This bend tech note will use the same difficulty and string as the note it is applied to
										tnp->note = np->note;
										tnp->pos = time;
										tnp->length = 1;
										tnp->flags |= EOF_PRO_GUITAR_NOTE_FLAG_BEND;
										tnp->flags |= EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION;
										if(step % 1000)
										{	//If the bend strength was defined in quarter steps
											tnp->bendstrength = 0x80 | (step / 500);
										}
										else
										{	//The bend strength was defined in half steps
											tnp->bendstrength = step / 1000;
										}
										tech_note_count++;
									}
								}//If this is a bendValue tag

								(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
								linectr++;
							}//Until there was an error reading from the file or end of file is reached
							if(error)
								break;	//Break from inner loop
						}//If this is a notes tag
						else if(strcasestr_spec(buffer, "<chords") && !strstr(buffer, "/>"))
						{	//If this is a chords tag and it isn't empty
							long time, id;
							unsigned long flags;

							#ifdef RS_IMPORT_DEBUG
								(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\tProcessing <chords> tag on line #%lu", linectr);
								eof_log(eof_log_string, 1);
							#endif

							tagctr = 0;
							(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
							linectr++;
							while(!error || !pack_feof(inf))
							{	//Until there was an error reading from the file or end of file is reached
								if(strcasestr_spec(buffer, "</chords"))
								{	//If this is the end of the events tag
									#ifdef RS_IMPORT_DEBUG
										(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\t\tAdded %lu chords", tagctr);
										eof_log(eof_log_string, 1);
									#endif
									break;	//Break from loop
								}

								//Read chord tag
								ptr = strcasestr_spec(buffer, "<chord ");
								if(ptr)
								{	//If this is a chord tag
									if(note_count < EOF_MAX_NOTES)
									{	//If another chord can be stored
										flags = 0;

										//Read chord attributes
										if(!parse_xml_rs_timestamp("time", buffer, &time))
										{	//If the timestamp was not readable
											(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading timestamp on line #%lu.  Aborting", linectr);
											eof_log(eof_log_string, 1);
											error = 1;
											break;	//Break from inner loop
										}
										if(!parse_xml_attribute_number("chordId", buffer, &id))
										{	//If the chord ID was not readable
											(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading chord ID on line #%lu.  Aborting", linectr);
											eof_log(eof_log_string, 1);
											error = 1;
											break;	//Break from inner loop
										}
										if(id >= chordlist_count)
										{	//If this chord ID was not defined in the chordTemplates tag
											(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError:  Invalid chord ID on line #%lu.  Aborting", linectr);
											eof_log(eof_log_string, 1);
											error = 1;
											break;	//Break from inner loop
										}
										if(!parse_xml_attribute_text(tag, sizeof(tag), "strum", buffer))
										{	//If the strum direction could not be read
											(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading strum direction on line #%lu.  Aborting", linectr);
											eof_log(eof_log_string, 1);
											error = 1;
											break;	//Break from inner loop
										}

										//Add chord and set attributes
										np = eof_pro_guitar_track_add_note(tp);	//Allocate, initialize and add the new note to the note array
										if(!np)
										{
											eof_log("\tError allocating memory.  Aborting", 1);
											error = 1;
											break;	//Break from inner loop
										}
										memcpy(np, chordlist[id], sizeof(EOF_PRO_GUITAR_NOTE));	//Copy the chord template into the new note
										np->pos = time;
										if(strcasestr_spec(tag, "up"))
										{	//If the strum direction is marked as up
											flags |= EOF_PRO_GUITAR_NOTE_FLAG_UP_STRUM;
											strum_dir = 1;	//Track that this arrangement defines chord strum directions
										}
										np->flags = flags;
										np->type = curdiff;
										note_count++;
										tagctr++;
									}
								}//If this is a chord tag

								(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
								linectr++;	//Increment line counter
							}
							if(error)
								break;	//Break from inner loop
						}//If this is a chords tag and it isn't empty
						else if(strcasestr_spec(buffer, "<anchors") && !strstr(buffer, "/>"))
						{	//If this is an anchors tag and it isn't empty
							long time, fret;

							#ifdef RS_IMPORT_DEBUG
								(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\tProcessing <anchors> tag on line #%lu", linectr);
								eof_log(eof_log_string, 1);
							#endif

							tagctr = 0;
							(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
							linectr++;
							while(!error || !pack_feof(inf))
							{	//Until there was an error reading from the file or end of file is reached
								if(strcasestr_spec(buffer, "</anchors"))
								{	//If this is the end of the anchors tag
									#ifdef RS_IMPORT_DEBUG
										(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\t\t\t\tAdded %lu anchors", tagctr);
										eof_log(eof_log_string, 1);
									#endif
									break;	//Break from loop
								}

								//Read anchor tag
								ptr = strcasestr_spec(buffer, "<anchor ");
								if(ptr)
								{	//If this is an anchor tag
									if(tp->handpositions < EOF_MAX_NOTES)
									{	//If another chord can be stored
										if(!parse_xml_rs_timestamp("time", buffer, &time))
										{	//If the timestamp was not readable
											(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading timestamp on line #%lu.  Aborting", linectr);
											eof_log(eof_log_string, 1);
											error = 1;
											break;	//Break from inner loop
										}
										if(!parse_xml_attribute_number("fret", buffer, &fret))
										{	//If the fret number was not readable
											(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError reading fret number on line #%lu.  Aborting", linectr);
											eof_log(eof_log_string, 1);
											error = 1;
											break;	//Break from inner loop
										}
										if(fret > 19)
										{	//If the anchor is not valid, log it and warn the user
											///When RS2 import is implemented, this check will need to take the capo position into account
											(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tIgnoring invalid anchor (fret %ld) at position %ld on line #%lu", fret, time, linectr);
											eof_log(eof_log_string, 1);
											if(!(warning & 1))
											{	//If the user wasn't warned about this error yet
												allegro_message("Warning:  This arrangement contains at least one invalid fret hand position (higher than fret 19).\n  Offending positions were dropped");
												warning |= 1;
											}
										}
										else
										{	//Otherwise add it to the track
											tp->handposition[tp->handpositions].start_pos = time;
											tp->handposition[tp->handpositions].end_pos = fret;
											tp->handposition[tp->handpositions].difficulty = curdiff;
											tp->handpositions++;
										}
									}//If another chord can be stored
								}//If this is an anchor tag

								(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
								linectr++;	//Increment line counter
								tagctr++;
							}
							if(error)
								break;	//Break from inner loop
						}//If this is an anchors tag and it isn't empty

						(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
						linectr++;
					}//Until there was an error reading from the file or end of file is reached
					if(error)
						break;	//Break from outer loop
				}//If this is a level tag

				(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
				linectr++;
			}//Until there was an error reading from the file or end of file is reached
		}//If this is the levels tag and it isn't empty

		(void) pack_fgets(buffer, (int)maxlinelength, inf);	//Read next line of text
		linectr++;	//Increment line counter
	}//Until there was an error reading from the file or end of file is reached

	if(!error)
	{	//If import succeeded
#ifdef RS_IMPORT_DEBUG
		eof_log("\tParsing complete.  Finishing up", 1);
#endif

		//Create tremolo phrases
		for(ctr3 = 0; ctr3 < 256; ctr3++)
		{	//For each of the 256 possible difficulties
			for(ctr2 = 0; ctr2 < tp->notes; ctr2++)
			{	//For each note in the track
				if(tp->note[ctr2]->type == ctr3)
				{	//If the note is in the difficulty being parsed
					if(tp->note[ctr2]->flags & EOF_NOTE_FLAG_IS_TREMOLO)
					{	//If this note is marked as being in a tremolo
						unsigned long startpos, endpos, count;

						startpos = tp->note[ctr2]->pos;	//Mark the start of this phrase
						endpos = startpos + tp->note[ctr2]->length;	//Initialize the end position of the phrase
						while(++ctr2 < tp->notes)
						{	//For the consecutive remaining notes in the track
							if(tp->note[ctr2]->type == ctr3)
							{	//If the note is in the difficulty being parsed
								if(tp->note[ctr2]->flags & EOF_NOTE_FLAG_IS_TREMOLO)
								{	//And the next note is also marked as a tremolo
									endpos = tp->note[ctr2]->pos + tp->note[ctr2]->length;	//Update the end position of the phrase
								}
								else
								{
									break;	//Break from while loop.  This note isn't a tremolo so the next pass doesn't need to check it either
								}
							}
						}
						count = tp->tremolos;
						if(tp->tremolos < EOF_MAX_PHRASES)
						{	//If the track can store the tremolo section
							tp->tremolo[count].start_pos = startpos;
							tp->tremolo[count].end_pos = endpos;
							tp->tremolo[count].flags = 0;
							tp->tremolo[count].difficulty = ctr3;	//Tremolo phrases are difficulty-specific in Rocksmith
							tp->tremolo[count].name[0] = '\0';
							tp->tremolos++;
						}
					}
				}
			}
		}

		//Apply strum directions
		eof_clear_input();
		if(strum_dir && (alert("At least one chord was marked as strum up.", "Would you like to to mark all non strum-up chords as strum down?", NULL, "&Yes", "&No", 'y', 'n') == 1))
		{	//If there were any up strum chords, and user opts to mark all others as down strum chords
			unsigned long bitmask, count;

			for(ctr = 0; ctr < tp->notes; ctr++)
			{	//For each note in the track
				for(ctr2 = 0, count = 0, bitmask = 1; ctr2 < 6; ctr2++, bitmask <<= 1)
				{	//For each of the 6 supported strings
					if(tp->note[ctr]->note & bitmask)
					{	//If this string is used
						count++;	//Increment counter
					}
				}
				if((count > 1) && !(tp->note[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_UP_STRUM))
				{	//If this note is a chord that isn't already marked to strum upward
					tp->note[ctr]->flags |= EOF_PRO_GUITAR_NOTE_FLAG_DOWN_STRUM;	//Mark it to strum downward
				}
			}
		}

		//Derive chord lengths from single note lengths
		eof_pro_guitar_track_sort_notes(tp);
		for(ctr = 0; ctr < tp->notes; ctr++)
		{	//For each note in the track
			if(eof_note_count_colors_bitmask(tp->note[ctr]->note) > 1)
			{	//If the note is a chord, look for single notes at the same position in other difficulties
				for(ctr2 = 0; ctr2 < tp->notes; ctr2++)
				{	//For each note in the track
					if(tp->note[ctr2]->pos > tp->note[ctr]->pos)
					{	//If the note is beyond the note from the outer loop
						break;	//None of the remaining notes will be at the appropriate position
					}
					else if(tp->note[ctr2]->pos == tp->note[ctr]->pos)
					{	//If the notes are at the same position
						if(tp->note[ctr2]->length > tp->note[ctr]->length)
						{	//If the note in the inner loop is longer (ie. a single note)
							tp->note[ctr]->length = tp->note[ctr2]->length;	//Assign its length to the chord from the outer loop
						}
					}
				}
			}
		}
	}//If import succeeded

	//Cleanup
	for(ctr = 0; ctr < phraselist_count; ctr++)
	{	//For each phrase name that was stored
		free(phraselist[ctr]);	//Free it
	}
	phraselist_count = 0;
	for(ctr = 0; ctr < eventlist_count; ctr++)
	{	//For each text event that was stored
		if(!error)
		{	//If import succeeded
			for(ctr2 = 0; ctr2 < eof_song->beats; ctr2++)
			{	//For each beat in the project
				if((ctr2 + 1 >= eof_song->beats) || (eventlist[ctr]->beat < eof_song->beat[ctr2 + 1]->pos))
				{	//If this text event falls before the next beat, or if there isn't another beat
					(void) eof_song_add_text_event(eof_song, ctr2, eventlist[ctr]->text, eof_selected_track, eventlist[ctr]->flags, 0);	//Add the event to this beat
					break;
				}
			}
		}
		free(eventlist[ctr]);	//Free the text event, since it's been added to the project
	}
	eof_beat_stats_cached = 0;	//Mark the cached beat stats as not current
	eventlist_count = 0;
	for(ctr = 0; ctr < chordlist_count; ctr++)
	{	//For each chord template that was stored
		free(chordlist[ctr]);	//Free it
	}
	chordlist_count = 0;
	(void) pack_fclose(inf);
	free(buffer);
	free(buffer2);

	if(!error)
	{	//If import succeeded
		eof_sort_events(eof_song);
		eof_pro_guitar_track_sort_fret_hand_positions(tp);

		return tp;
	}
	else
	{
		free(tp);
		return NULL;
	}
}
