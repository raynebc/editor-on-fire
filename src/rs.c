#include <allegro.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include "agup/agup.h"
#include "beat.h"
#include "chart_import.h"	//For FindLongestLineLength_ALLEGRO()
#include "dialog.h"
#include "event.h"
#include "main.h"
#include "midi.h"
#include "mix.h"	//For eof_set_seek_position()
#include "rs.h"
#include "rs_import.h"	//For eof_parse_chord_template()
#include "song.h"	//For eof_pro_guitar_track_delete_hand_position()
#include "tuning.h"	//For eof_lookup_tuned_note()
#include "undo.h"
#include "foflc/RS_parse.h"	//For expand_xml_text()
#include "menu/beat.h"	//For eof_rocksmith_phrase_dialog_add()
#include "menu/track.h"	//For eof_fret_hand_position_list_dialog_undo_made and eof_fret_hand_position_list_dialog[]

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

EOF_CHORD_SHAPE eof_chord_shape[EOF_MAX_CHORD_SHAPES];	//An array storing chord shape definitions
unsigned long num_eof_chord_shapes = 0;	//Defines how many chord shapes are defined in eof_chord_shape[]

EOF_RS_PREDEFINED_SECTION eof_rs_predefined_sections[EOF_NUM_RS_PREDEFINED_SECTIONS] =
{
	{"noguitar", "No Guitar"},
	{"intro", "Intro"},
	{"preverse", "Pre Verse"},
	{"verse", "Verse"},
	{"postvs", "Post Verse"},
	{"modverse", "Modulated Verse"},
	{"prechorus", "Pre Chorus"},
	{"chorus", "Chorus"},
	{"postchorus", "Post Chorus"},
	{"modchorus", "Modulated Chorus"},
	{"hook", "Hook"},
	{"prebrdg", "Pre Bridge"},
	{"bridge", "Bridge"},
	{"postbrdg", "Post Bridge"},
	{"solo", "Solo"},
	{"outro", "Outro"},
	{"ambient", "Ambient"},
	{"breakdown", "Breakdown"},
	{"interlude", "Interlude"},
	{"transition", "Transition"},
	{"riff", "Riff"},
	{"fadein", "Fade In"},
	{"fadeout", "Fade Out"},
	{"buildup", "Buildup"},
	{"variation", "Variation"},
	{"head", "Head"},
	{"modbridge", "Modulated Bridge"},
	{"melody", "Melody"},
	{"vamp", "Vamp"},
	{"silence", "Silence"}
};

EOF_RS_PREDEFINED_SECTION eof_rs_predefined_events[EOF_NUM_RS_PREDEFINED_EVENTS] =
{
	{"B0", "High pitch tick (B0)"},
	{"B1", "Low pitch tick (B1)"},
	{"E1", "Crowd happy (E1)"},
	{"E3", "Crowd wild (E3)"}
};

unsigned char *eof_fret_range_tolerances = NULL;	//A dynamically allocated array that defines the fretting hand's range for each fret on the guitar neck, numbered where fret 1's range is defined at eof_fret_range_tolerances[1]

char *eof_rs_arrangement_names[5] = {"Undefined", "Combo", "Rhythm", "Lead", "Bass"};	//Indexes 1 through 4 represent the valid arrangement names for Rocksmith arrangements

int eof_is_string_muted(EOF_SONG *sp, unsigned long track, unsigned long note)
{
	unsigned long ctr, bitmask;
	EOF_PRO_GUITAR_TRACK *tp;
	int allmuted = 1;	//Will be set to nonzero if any used strings aren't fret hand muted

	if(!sp || (track >= sp->tracks) || (note >= eof_get_track_size(sp, track)) || (sp->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return 0;	//Return error

	tp = sp->pro_guitar_track[sp->track[track]->tracknum];
	for(ctr = 0, bitmask = 1; ctr < 6; ctr++, bitmask <<= 1)
	{	//For each of the 6 supported strings
		if(ctr < tp->numstrings)
		{	//If this is a string used in the track
			if(tp->note[note]->note & bitmask)
			{	//If this is a string used in the note
				if((tp->note[note]->frets[ctr] & 0x80) == 0)
				{	//If this string is not fret hand muted
					allmuted = 0;
					break;
				}
			}
		}
	}

	return allmuted;	//Return nonzero if all of the used strings were fret hand muted
}

int eof_is_partially_ghosted(EOF_SONG *sp, unsigned long track, unsigned long note)
{
	unsigned long ctr, bitmask;
	EOF_PRO_GUITAR_TRACK *tp;
	char ghosted = 0, nonghosted = 0;	//Tracks the number of gems in this note that are ghosted and non ghosted

	if(!sp || (track >= sp->tracks) || (note >= eof_get_track_size(sp, track)) || (sp->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return 0;	//Return error

	tp = sp->pro_guitar_track[sp->track[track]->tracknum];
	for(ctr = 0, bitmask = 1; ctr < 6; ctr++, bitmask <<= 1)
	{	//For each of the 6 supported strings
		if(ctr < tp->numstrings)
		{	//If this is a string used in the track
			if(tp->note[note]->note & bitmask)
			{	//If this is a string used in the note
				if(tp->note[note]->ghost & bitmask)
				{	//If this string is ghosted
					ghosted++;
				}
				else
				{	//This string is not ghosted
					nonghosted++;
				}
			}
		}
	}

	return (ghosted && nonghosted);	//Return nonzero if the note contained at least one ghosted gem AND one non ghosted gem
}

int eof_is_partially_string_muted(EOF_PRO_GUITAR_TRACK *tp, unsigned long note)
{
	unsigned long ctr, bitmask;
	char muted = 0, nonmuted = 0;	//Tracks the number of gems in this note that are muted and non muted

	if(!tp || (note >= tp->notes))
		return 0;	//Return error

	for(ctr = 0, bitmask = 1; ctr < 6; ctr++, bitmask <<= 1)
	{	//For each of the 6 supported strings
		if(ctr < tp->numstrings)
		{	//If this is a string used in the track
			if(tp->note[note]->note & bitmask)
			{	//If this is a string used in the note
				if(tp->note[note]->frets[ctr] & 0x80)
				{	//If this string is muted
					muted++;
				}
				else
				{	//This string is not muted
					nonmuted++;
				}
			}
		}
	}

	return (muted && nonmuted);	//Return nonzero if the note contained at least one ghosted gem AND one non ghosted gem
}

int eof_tflag_is_arpeggio(unsigned long tflag)
{
	if((tflag & EOF_NOTE_TFLAG_ARP) && !(tflag & EOF_NOTE_TFLAG_HAND))
		return 1;	//Return 1 if the flag indicates the note is in an arpeggio/handshape phrase, but it is not a handshape phrase

	return 0;
}

unsigned long eof_build_chord_list(EOF_SONG *sp, unsigned long track, unsigned long **results, char target)
{
	unsigned long ctr, ctr2, unique_count = 0;
	EOF_PRO_GUITAR_NOTE **notelist;	//An array large enough to hold a pointer to every note in the track
	EOF_PRO_GUITAR_TRACK *tp;
	char match;

	eof_log("eof_rs_build_chord_list() entered", 1);

	if(!results)
		return 0;	//Return error

	if(!sp || (track >= sp->tracks) || (sp->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
	{
		*results = NULL;
		return 0;	//Invalid parameters
	}

	target |= 4;	//Allow ghosted notes to be counted for chords, since this is required for arpeggios

	//Duplicate the track's note array
	notelist = malloc(sizeof(EOF_PRO_GUITAR_NOTE *) * EOF_MAX_NOTES);	//Allocate memory to duplicate the note[] array
	if(!notelist)
	{
		*results = NULL;
		return 0;	//Return error
	}
	tp = sp->pro_guitar_track[sp->track[track]->tracknum];
	memcpy(notelist, tp->note, sizeof(EOF_PRO_GUITAR_NOTE *) * EOF_MAX_NOTES);	//Copy the note array

	//Overwrite each pointer in the duplicate note array that isn't a unique chord with NULL
	for(ctr = 0; ctr < tp->notes; ctr++)
	{	//For each note in the track
		if(eof_note_count_rs_lanes(sp, track, ctr, target) > 1)
		{	//If this note is a valid chord based on the target
			if(!(tp->note[ctr]->tflags & EOF_NOTE_TFLAG_IGNORE) || (tp->note[ctr]->tflags & EOF_NOTE_TFLAG_ARP) || (tp->note[ctr]->tflags & EOF_NOTE_TFLAG_GHOST_HS))
			{	//If this note is not ignored, is a chord within an arpeggio/handshape or is a temporary chord created created due to ghost handshape status
				match = 0;
				for(ctr2 = ctr + 1; ctr2 < tp->notes; ctr2++)
				{	//For each note in the track that follows this note
					if((eof_note_count_rs_lanes(sp, track, ctr2, target) > 1) && !eof_note_compare_simple(sp, track, ctr, ctr2))
					{	//If this note matches one that follows it, and that later note is a valid chord for the target Rocksmith game
						if(!eof_pro_guitar_note_compare_fingerings(tp->note[ctr], tp->note[ctr2]))
						{	//If this note has identical fingering to the one that follows it
							if(!(tp->note[ctr2]->tflags & EOF_NOTE_TFLAG_IGNORE) || (tp->note[ctr2]->tflags & EOF_NOTE_TFLAG_GHOST_HS))
							{	//If the note is not ignored or if the note was created due to ghost handshape status
								if(!strcmp(tp->note[ctr]->name, tp->note[ctr2]->name))
								{	//If both notes have the same name (or both have no defined name)
									if(!(target & 2))
									{	//If the target is Rocksmith 1
										notelist[ctr] = NULL;	//Eliminate this note from the list
										match = 1;	//Note that this chord matched one of the others
										break;
									}
									if(eof_tflag_is_arpeggio(tp->note[ctr]->tflags) == eof_tflag_is_arpeggio(tp->note[ctr2]->tflags))
									{	//If both notes have the same arpeggio status (both are in an arpeggio and not a handshape), or at least one is otherwise
										notelist[ctr] = NULL;	//Eliminate this note from the list
										match = 1;	//Note that this chord matched one of the others
										break;
									}
								}
							}
						}
					}
				}
				if(!match)
				{	//If this chord didn't match any of the other notes
					unique_count++;	//Increment unique chord counter
				}
			}//If this note is not ignored, is a chord within an arpeggio or is a temporary chord created created due to ghost handshape status
			else
			{
				notelist[ctr] = NULL;	//Eliminate this note from the list since it's not a unique chord
			}
		}//If this note is a valid chord based on the target
		else
		{	//This not is not a chord
			notelist[ctr] = NULL;	//Eliminate this note from the list since it's not a chord
		}
	}//For each note in the track

	if(!unique_count)
	{	//If there were no chords
		*results = NULL;	//Return empty result set
		free(notelist);
		return 0;
	}

	//Allocate and build an array with the note numbers representing the unique chords
	*results = malloc(sizeof(unsigned long) * unique_count);	//Allocate enough memory to store the note number of each unique chord
	if(*results == NULL)
	{
		free(notelist);
		return 0;
	}
	for(ctr = 0, ctr2 = 0; ctr < tp->notes; ctr++)
	{	//For each note in the track
		if(notelist[ctr] != NULL)
		{	//If this was a unique chord
			if(ctr2 < unique_count)
			{	//Bounds check
				(*results)[ctr2++] = ctr;	//Append the note number
			}
		}
	}

	//Cleanup and return results
	free(notelist);
	return unique_count;
}

unsigned long eof_build_section_list(EOF_SONG *sp, unsigned long **results, unsigned long track)
{
	unsigned long ctr, ctr2, unique_count = 0;
	EOF_TEXT_EVENT **eventlist;	//An array large enough to hold a pointer to every text event in the chart
	char match;

	eof_log("eof_build_section_list() entered", 1);

	if(!results)
		return 0;	//Return error

	if(!sp)
	{
		*results = NULL;
		return 0;	//Return error
	}

	//Duplicate the chart's text events array
	eventlist = malloc(sizeof(EOF_TEXT_EVENT *) * EOF_MAX_TEXT_EVENTS);
	if(!eventlist)
	{
		*results = NULL;
		return 0;	//Return error
	}
	memcpy(eventlist, sp->text_event, sizeof(EOF_TEXT_EVENT *) * EOF_MAX_TEXT_EVENTS);	//Copy the event array

	//In the case of beats that contain multiple sections, only keep ones that are cached in the beat statistics
	eof_process_beat_statistics(sp, track);	//Rebuild beat stats from the perspective of the track being examined
	for(ctr = 0; ctr < sp->text_events; ctr++)
	{	//For each text event in the chart
		match = 0;
		for(ctr2 = 0; ctr2 < sp->beats; ctr2++)
		{	//For each beat in the chart
			if(sp->beat[ctr2]->contained_section_event == ctr)
			{	//If the beat's statistics indicate this section is used
				match = 1;	//Note that this section is to be kept
				break;
			}
		}
		if(!match)
		{	//If none of the beat stats used this section
			eventlist[ctr] = NULL;	//Eliminate this section from the list since only 1 section per beat will be exported
		}
	}

	//Overwrite each pointer in the duplicate event array that isn't a unique section marker with NULL and count the unique events
	for(ctr = 0; ctr < sp->text_events; ctr++)
	{	//For each text event in the chart
		if(eventlist[ctr] != NULL)
		{	//If this section hasn't been eliminated yet
			if(eof_is_section_marker(sp->text_event[ctr], track))
			{	//If the text event's string or flags indicate a section marker (from the perspective of the specified track)
				match = 0;
				for(ctr2 = ctr + 1; ctr2 < sp->text_events; ctr2++)
				{	//For each event in the chart that follows this event
					if(eventlist[ctr2] != NULL)
					{	//If the event wasn't already eliminated
						if(eof_is_section_marker(sp->text_event[ctr2], track) &&!ustricmp(sp->text_event[ctr]->text, sp->text_event[ctr2]->text))
						{	//If this event is also a section event from the perspective of the track being examined, and its text matches
							eventlist[ctr] = NULL;	//Eliminate this event from the list
							match = 1;	//Note that this section matched one of the others
							break;
						}
					}
				}
				if(!match)
				{	//If this section marker didn't match any of the other events
					unique_count++;	//Increment unique section counter
				}
			}
			else
			{	//This event is not a section marker
				eventlist[ctr] = NULL;	//Eliminate this note from the list since it's not a chord
			}
		}
	}

	if(!unique_count)
	{	//If there were no section markers
		*results = NULL;	//Return empty result set
		free(eventlist);
		return 0;
	}

	//Allocate and build an array with the event numbers representing the unique section markers
	*results = malloc(sizeof(unsigned long) * unique_count);	//Allocate enough memory to store the event number of each unique section marker
	if(*results == NULL)
	{
		free(eventlist);
		return 0;
	}
	for(ctr = 0, ctr2 = 0; ctr < sp->text_events; ctr++)
	{	//For each event in the chart
		if(eventlist[ctr] != NULL)
		{	//If this was a unique section marker
			if(ctr2 < unique_count)
			{	//Bounds check
				(*results)[ctr2++] = ctr;	//Append the event number
			}
		}
	}

	//Cleanup and return results
	free(eventlist);
	return unique_count;
}

int eof_song_qsort_control_events(const void * e1, const void * e2)
{
	EOF_RS_CONTROL * thing1 = (EOF_RS_CONTROL *)e1;
	EOF_RS_CONTROL * thing2 = (EOF_RS_CONTROL *)e2;

	//Sort by timestamp
	if(thing1->pos < thing2->pos)
	{
		return -1;
	}
	else if(thing1->pos > thing2->pos)
	{
		return 1;
	}

	//They are equal
	return 0;
}

int eof_export_rocksmith_1_track(EOF_SONG * sp, char * fn, unsigned long track, unsigned short *user_warned)
{
	PACKFILE * fp;
	char buffer[600] = {0}, buffer2[600] = {0};
	time_t seconds;		//Will store the current time in seconds
	struct tm *caltime;	//Will store the current time in calendar format
	unsigned long ctr, ctr2, ctr3, ctr4, ctr5, numsections, stringnum, bitmask, numsinglenotes, numchords, *chordlist = NULL, chordlistsize, xml_end, numevents = 0;
	EOF_PRO_GUITAR_TRACK *tp;
	char *arrangement_name;	//This will point to the track's native name unless it has an alternate name defined
	unsigned numdifficulties;
	unsigned char bre_populated = 0;
	unsigned beatspermeasure = 4, beatcounter = 0;
	long displayedmeasure, measurenum = 0;
	char standard[] = {0,0,0,0,0,0};
	char standardbass[] = {0,0,0,0};
	char eb[] = {-1,-1,-1,-1,-1,-1};
	char dropd[] = {-2,0,0,0,0,0};
	char openg[] = {-2,-2,0,0,0,-2};
	char *tuning;
	char isebtuning = 1;	//Will track whether all strings are tuned to -1
	char notename[EOF_NAME_LENGTH+1] = {0};	//String large enough to hold any chord name supported by EOF
	int scale = 0, chord = 0, isslash = 0, bassnote = 0;	//Used for power chord detection
	int standard_tuning = 0, non_standard_chords = 0, barre_chords = 0, power_chords = 0, notenum, dropd_tuning = 1, dropd_power_chords = 0, open_chords = 0, double_stops = 0, palm_mutes = 0, harmonics = 0, hopo = 0, tremolo = 0, slides = 0, bends = 0, tapping = 0, vibrato = 0, slappop = 0, octaves = 0, fifths_and_octaves = 0;	//Used for technique detection
	char is_bass = 0;	//Is set to nonzero if the specified track is to be considered a bass guitar track
	unsigned long chordid = 0, handshapectr = 0, handshapeloop;
	unsigned long handshapestart = 0, handshapeend = 0;
	long nextnote;
	unsigned long originalbeatcount;	//If beats are padded to reach the beginning of the next measure (for DDC), this will track the project's original number of beats
	char restore_tech_view = 0;			//If tech view is in effect, it is temporarily disabled so that the correct notes are exported

	eof_log("eof_export_rocksmith_1_track() entered", 1);

	if(!sp || !fn || !sp->beats || (track >= sp->tracks) || (sp->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT) || !user_warned)
	{
		eof_log("\tError saving:  Invalid parameters", 1);
		return 0;	//Return failure
	}

	tp = sp->pro_guitar_track[sp->track[track]->tracknum];
	restore_tech_view = eof_menu_track_get_tech_view_state(sp, track);
	eof_menu_track_set_tech_view_state(sp, track, 0);	//Disable tech view if applicable
	if(eof_get_highest_fret(sp, track, 0) > 22)
	{	//If the track being exported uses any frets higher than 22
		if((*user_warned & 2) == 0)
		{	//If the user wasn't alerted about this issue yet
			allegro_message("Warning:  At least one track (\"%s\") uses a fret higher than 22.  This will cause Rocksmith 1 to crash.", sp->track[track]->name);
			*user_warned |= 2;
		}
	}
	for(ctr = 0; ctr < eof_get_track_size(sp, track); ctr++)
	{	//For each note in the track
		unsigned char slideend;

		if(eof_note_count_rs_lanes(sp, track, ctr, 1) == 1)
		{	//If the note will export as a single note
			if(eof_get_note_flags(sp, track, ctr) & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP)
			{	//If the note slides up
				slideend = tp->note[ctr]->slideend + tp->capo;	//Obtain the end position of the slide, take the capo position into account
				if(!(eof_get_note_flags(sp, track, ctr) & EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION))
				{	//If this slide's end position is not defined
					slideend = eof_get_highest_fret_value(sp, track, ctr) + 1;	//Assume a 1 fret slide
				}
				if(slideend >= 22)
				{	//If the slide goes to or above fret 22
					if((*user_warned & 8) == 0)
					{	//If the user wasn't alerted about this issue yet
						eof_seek_and_render_position(track, tp->note[ctr]->type, tp->note[ctr]->pos);	//Show the offending note
						allegro_message("Warning:  At least one note slides to or above fret 22.  This will cause Rocksmith 1 to crash.");
						*user_warned |= 8;
					}
					break;
				}
			}
		}
	}

	//Count the number of populated difficulties in the track
	(void) eof_detect_difficulties(sp, track);	//Update eof_track_diff_populated_status[] to reflect all populated difficulties for this track
	if((sp->track[track]->flags & EOF_TRACK_FLAG_UNLIMITED_DIFFS) == 0)
	{	//If the track is using the traditional 5 difficulty system
		if(eof_track_diff_populated_status[4])
		{	//If the BRE difficulty is populated
			bre_populated = 1;	//Track that it was
		}
		eof_track_diff_populated_status[4] = 0;	//Ensure that the BRE difficulty is not exported
	}
	for(ctr = 0, numdifficulties = 0; ctr < 256; ctr++)
	{	//For each possible difficulty
		if(eof_track_diff_populated_status[ctr])
		{	//If this difficulty is populated
			numdifficulties++;	//Increment this counter
		}
	}
	if(!numdifficulties)
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Cannot export track \"%s\"in Rocksmith format, it has no populated difficulties", sp->track[track]->name);
		eof_log(eof_log_string, 1);
		if(bre_populated && ((*user_warned & 1024) == 0))
		{	//If the BRE difficulty was the only one populated, warn that it is being omitted (unless the user was already warned of this)
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Warning:  Track \"%s\" only has notes in the BRE difficulty.\nThese are not exported in Rocksmith format unless you remove the difficulty limit (Track>Rocksmith>Remove difficulty limit).", sp->track[track]->name);
			allegro_message("%s", eof_log_string);
			eof_log(eof_log_string, 1);
			*user_warned |= 1024;
		}
		eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view if applicable
		return 0;	//Return failure
	}

	//Identify the first note of each arpeggio phrase
	for(ctr2 = 0; ctr2 < tp->arpeggios; ctr2++)
	{	//For each arpeggio/handshape section in the track
		for(ctr = 0; ctr < tp->notes; ctr++)
		{	//For each note in the active pro guitar track
			if(!(tp->note[ctr]->tflags & EOF_NOTE_TFLAG_IGNORE) && (tp->note[ctr]->pos >= tp->arpeggio[ctr2].start_pos) && (tp->note[ctr]->pos <= tp->arpeggio[ctr2].end_pos) && (tp->note[ctr]->type == tp->arpeggio[ctr2].difficulty))
			{	//If the note is isn't already ignored and is within the arpeggio/handshape phrase
				tp->note[ctr]->tflags |= EOF_NOTE_TFLAG_ARP_FIRST;	//Mark this note as being in an arpeggio/handshape phrase
				break;
			}
		}
	}

	//Update target file name and open it for writing
	if((sp->track[track]->flags & EOF_TRACK_FLAG_ALT_NAME) && (sp->track[track]->altname[0] != '\0'))
	{	//If the track has an alternate name
		arrangement_name = sp->track[track]->altname;
	}
	else
	{	//Otherwise use the track's native name
		arrangement_name = sp->track[track]->name;
	}
	(void) snprintf(buffer, 600, "%s.xml", arrangement_name);
	(void) replace_filename(fn, fn, buffer, 1024);
	fp = pack_fopen(fn, "w");
	if(!fp)
	{
		eof_log("\tError saving:  Cannot open file for writing", 1);
		eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view if applicable
		return 0;	//Return failure
	}

	//Update the track's arrangement name
	if(tp->arrangement)
	{	//If the track's arrangement type has been defined
		arrangement_name = eof_rs_arrangement_names[tp->arrangement];	//Use the corresponding string in the XML file
	}

	//Get the smaller of the chart length and the music length, this will be used to write the songlength tag
	xml_end = eof_music_length;
	if(eof_silence_loaded || (eof_chart_length < eof_music_length))
	{	//If the chart length is shorter than the music length, or there is no chart audio loaded
		xml_end = eof_chart_length;	//Use the chart's length instead
	}

	//Write the beginning of the XML file
	(void) pack_fputs("<?xml version='1.0' encoding='UTF-8'?>\n", fp);
	(void) pack_fputs("<song version=\"4\">\n", fp);
	(void) pack_fputs("<!-- " EOF_VERSION_STRING " -->\n", fp);	//Write EOF's version in an XML comment
	expand_xml_text(buffer2, sizeof(buffer2) - 1, sp->tags->title, 64, 0);	//Expand XML special characters into escaped sequences if necessary, and check against the maximum supported length of this field
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <title>%s</title>\n", buffer2);
	(void) pack_fputs(buffer, fp);
	expand_xml_text(buffer2, sizeof(buffer2) - 1, arrangement_name, 32, 0);	//Expand XML special characters into escaped sequences if necessary, and check against the maximum supported length of this field
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <arrangement>%s</arrangement>\n", buffer2);
	(void) pack_fputs(buffer, fp);
	(void) pack_fputs("  <part>1</part>\n", fp);
	(void) pack_fputs("  <offset>0.000</offset>\n", fp);
	eof_truncate_chart(sp);	//Update the chart length
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <songLength>%.3f</songLength>\n", (double)(xml_end - 1) / 1000.0);	//Make sure the song length is not longer than the actual audio, or the chart won't reach an end in-game
	(void) pack_fputs(buffer, fp);
	seconds = time(NULL);
	caltime = localtime(&seconds);
	if(caltime)
	{	//If the calendar time could be determined
		(void) snprintf(buffer, sizeof(buffer) - 1, "  <lastConversionDateTime>%d-%d-%d %d:%02d</lastConversionDateTime>\n", caltime->tm_mon + 1, caltime->tm_mday, caltime->tm_year % 100, caltime->tm_hour, caltime->tm_min);
	}
	else
	{
		(void) snprintf(buffer, sizeof(buffer) - 1, "  <lastConversionDateTime>UNKNOWN</lastConversionDateTime>\n");
	}
	(void) pack_fputs(buffer, fp);

	//Write additional tags to pass song information to the Rocksmith toolkit
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <startBeat>%.3f</startBeat>\n", sp->beat[0]->fpos / 1000.0);	//The position of the first beat
	(void) pack_fputs(buffer, fp);
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <averageTempo>%.3f</averageTempo>\n", 60000.0 / ((sp->beat[sp->beats - 1]->fpos - sp->beat[0]->fpos) / sp->beats));	//The average tempo (60000ms / the average beat length in ms)
	(void) pack_fputs(buffer, fp);
	tuning = tp->tuning;	//By default, use the track's original tuning array
	for(ctr = 0; ctr < 6; ctr++)
	{	//For each string EOF supports
		if(ctr >= tp->numstrings)
		{	//If the track doesn't use this string
			tp->tuning[ctr] = 0;	//Ensure the tuning is cleared accordingly
		}
	}
	for(ctr = 0; ctr < tp->numstrings; ctr++)
	{	//For each string in this track
		if(tp->tuning[ctr] != -1)
		{	//If this string isn't tuned a half step down
			isebtuning = 0;
			break;
		}
	}
	is_bass = eof_track_is_bass_arrangement(tp, track);
	if(isebtuning && !(is_bass && (tp->numstrings > 4)))
	{	//If all strings were tuned down a half step (except for bass tracks with more than 4 strings, since in those cases, the lowest string is not tuned to E)
		tuning = eb;	//Remap 4 or 5 string Eb tuning as {-1,-1,-1,-1,-1,-1}
	}
	if(memcmp(tuning, standard, 6) && memcmp(tuning, standardbass, 4) && memcmp(tuning, eb, 6) && memcmp(tuning, dropd, 6) && memcmp(tuning, openg, 6))
	{	//If the track's tuning doesn't match any supported by Rocksmith
		if((*user_warned & 2048) == 0)
		{	//If the user wasn't warned about this yet
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Warning:  This track (%s) uses a tuning that isn't known to be supported in Rocksmith 1.  Tuning and note recognition may not work as expected in-game", sp->track[track]->name);
			allegro_message("Warning:  This track (%s) uses a tuning that isn't one known to be supported in Rocksmith 1.\nTuning and note recognition may not work as expected in-game", sp->track[track]->name);
			eof_log(eof_log_string, 1);
			*user_warned |= 2048;
		}
	}
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <tuning string0=\"%d\" string1=\"%d\" string2=\"%d\" string3=\"%d\" string4=\"%d\" string5=\"%d\" />\n", tuning[0], tuning[1], tuning[2], tuning[3], tuning[4], tuning[5]);
	(void) pack_fputs(buffer, fp);
	expand_xml_text(buffer2, sizeof(buffer2) - 1, sp->tags->artist, 256, 0);	//Replace any special characters in the artist song property with escape sequences if necessary
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <artistName>%s</artistName>\n", buffer2);
	(void) pack_fputs(buffer, fp);
	expand_xml_text(buffer2, sizeof(buffer2) - 1, sp->tags->album, 256, 0);	//Replace any special characters in the album song property with escape sequences if necessary
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <albumName>%s</albumName>\n", buffer2);
	(void) pack_fputs(buffer, fp);
	expand_xml_text(buffer2, sizeof(buffer2) - 1, sp->tags->year, 32, 0);	//Replace any special characters in the year song property with escape sequences if necessary
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <albumYear>%s</albumYear>\n", buffer2);
	(void) pack_fputs(buffer, fp);

	//Determine arrangement properties
	if(!memcmp(tuning, standard, 6))
	{	//All unused strings had their tuning set to 0, so if all bytes of this array are 0, the track is in standard tuning
		standard_tuning = 1;
	}
	notenum = eof_lookup_tuned_note(tp, track, 0, tp->tuning[0]);	//Look up the open note the lowest string plays
	notenum %= 12;	//Ensure the value is in the range of [0,11]
	if(notenum == 5)
	{	//If the lowest string is tuned to D
		for(ctr = 1; ctr < 6; ctr++)
		{	//For the other 5 strings
			if(tp->tuning[ctr] != 0)
			{	//If the string is not in standard tuning
				dropd_tuning = 0;
			}
		}
	}
	else
	{	//The lowest string is not tuned to D
		dropd_tuning = 0;
	}
	eof_determine_phrase_status(sp, track);	//Update the tremolo status of each note
	for(ctr = 0; ctr < tp->notes; ctr++)
	{	//For each note in the track
		if(eof_note_count_rs_lanes(sp, track, ctr, 1) > 1)
		{	//If the note will export as a chord (more than one non ghosted/muted gem)
			if(!non_standard_chords && !eof_build_note_name(sp, track, ctr, notename))
			{	//If the chord has no defined or detected name (only if this condition hasn't been found already)
				non_standard_chords = 1;
			}
			if(!barre_chords && eof_pro_guitar_note_is_barre_chord(tp, ctr))
			{	//If the chord is a barre chord (only if this condition hasn't been found already)
				barre_chords = 1;
			}
			if(!power_chords && eof_lookup_chord(tp, track, ctr, &scale, &chord, &isslash, &bassnote, 0, 0))
			{	//If the chord lookup found a match (only if this condition hasn't been found already)
				if(chord == 27)
				{	//27 is the index of the power chord formula in eof_chord_names[]
					power_chords = 1;
					if(dropd_tuning)
					{	//If this track is in drop d tuning
						dropd_power_chords = 1;
					}
				}
			}
			if(!open_chords)
			{	//Only if no open chords have been found already
				for(ctr2 = 0, bitmask = 1; ctr2 < 6; ctr2++, bitmask <<= 1)
				{	//For each of the 6 supported strings
					if((tp->note[ctr]->note & bitmask) && (tp->note[ctr]->frets[ctr2] == 0))
					{	//If this string is used and played open
						open_chords = 1;
					}
				}
			}
			if(!double_stops && eof_pro_guitar_note_is_double_stop(tp, ctr))
			{	//If the chord is a double stop (only if this condition hasn't been found already)
				double_stops = 1;
			}
			if(is_bass)
			{	//If the arrangement being exported is bass
				int thisnote, lastnote = -1, failed = 0;
				for(ctr2 = 0, bitmask = 1; ctr2 < 6; ctr2++, bitmask <<= 1)
				{	//For each of the 6 supported strings
					if((tp->note[ctr]->note & bitmask) && !(tp->note[ctr]->frets[ctr2] & 0x80))
					{	//If this string is played and not string muted
						thisnote = eof_lookup_played_note(tp, track, ctr2, tp->note[ctr]->frets[ctr2]);	//Determine what note is being played
						if((lastnote >= 0) && (lastnote != thisnote))
						{	//If this string's played note doesn't match the note played by previous strings
							failed = 1;
							break;
						}
						lastnote = thisnote;
					}
				}
				if(!failed)
				{	//If non muted strings in the chord played the same note
					octaves = 1;
				}
			}
		}//If the note is a chord (more than one non ghosted gem)
		if(tp->note[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE)
		{	//If the note is palm muted
			palm_mutes = 1;
		}
		if(tp->note[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC)
		{	//If the note is a harmonic
			harmonics = 1;
		}
		if((tp->note[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_HO) || (tp->note[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_PO))
		{	//If the note is a hammer on or pull off
			hopo = 1;
		}
		if(tp->note[ctr]->flags & EOF_NOTE_FLAG_IS_TREMOLO)
		{	//If the note is played with tremolo
			tremolo = 1;
		}
		if((tp->note[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP) || (tp->note[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN))
		{	//If the note slides up or down
			slides = 1;
		}
		if(tp->note[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_BEND)
		{	//If the note is bent
			bends = 1;
		}
		if(tp->note[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_TAP)
		{	//If the note is tapped
			tapping = 1;
		}
		if(tp->note[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO)
		{	//If the note is played with vibrato
			vibrato = 1;
		}
		if((tp->note[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_SLAP) || (tp->note[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_POP))
		{	//If the note is played by slapping or popping
			slappop = 1;
		}
	}//For each note in the track
	if(is_bass)
	{	//If the arrangement being exported is bass
		if(double_stops || octaves)
		{	//If either of these techniques were detected
			fifths_and_octaves = 1;
		}
		double_stops = 0;
	}
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <arrangementProperties represent=\"1\" standardTuning=\"%d\" nonStandardChords=\"%d\" barreChords=\"%d\" powerChords=\"%d\" dropDPower=\"%d\" openChords=\"%d\" fingerPicking=\"0\" pickDirection=\"0\" doubleStops=\"%d\" palmMutes=\"%d\" harmonics=\"%d\" pinchHarmonics=\"0\" hopo=\"%d\" tremolo=\"%d\" slides=\"%d\" unpitchedSlides=\"0\" bends=\"%d\" tapping=\"%d\" vibrato=\"%d\" fretHandMutes=\"0\" slapPop=\"%d\" twoFingerPicking=\"0\" fifthsAndOctaves=\"%d\" syncopation=\"0\" bassPick=\"0\" />\n", standard_tuning, non_standard_chords, barre_chords, power_chords, dropd_power_chords, open_chords, double_stops, palm_mutes, harmonics, hopo, tremolo, slides, bends, tapping, vibrato, slappop, fifths_and_octaves);
	(void) pack_fputs(buffer, fp);

	//Write the phrases and do other setup common to both Rocksmith exports
	originalbeatcount = sp->beats;	//Store the original beat count
	if(!eof_rs_export_common(sp, track, fp, user_warned))
	{	//If there was an error adding temporary phrases, sections, beats tot he project and writing the phrases to file
		eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view if applicable
		(void) pack_fclose(fp);
		return 0;	//Return error
	}

	//Write some unknown information
	(void) pack_fputs("  <linkedDiffs count=\"0\"/>\n", fp);
	(void) pack_fputs("  <phraseProperties count=\"0\"/>\n", fp);

	if(sp->tags->rs_chord_technique_export)
	{	//If the user opted to export chord techniques to the Rocksmith XML files
		//Check for chords with techniques, and for each, add a temporary single note with those techniques at the same position
		//Rocksmith will render the single note on top of the chord box so that the player can see which techniques are indicated
		eof_determine_phrase_status(sp, track);	//Update the tremolo status of each note
		for(ctr = tp->notes; ctr > 0; ctr--)
		{	//For each note in the track, in reverse order
			if(eof_note_count_rs_lanes(sp, track, ctr - 1, 1) > 1)
			{	//If this note will export as a chord (at least two non ghosted/muted gems)
				unsigned long target = EOF_PRO_GUITAR_NOTE_FLAG_BEND | EOF_PRO_GUITAR_NOTE_FLAG_HO | EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC | EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE | EOF_PRO_GUITAR_NOTE_FLAG_POP | EOF_PRO_GUITAR_NOTE_FLAG_SLAP | EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP | EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN | EOF_NOTE_FLAG_IS_TREMOLO;	//A list of all statuses to try to notate for chords
				EOF_PRO_GUITAR_NOTE *new_note;

				if(tp->note[ctr - 1]->flags & target)
				{	//If this note has any of the statuses that can be displayed in Rocksmith for single notes
					for(ctr2 = 0, bitmask = 1; ctr2 < 6; ctr2++, bitmask <<= 1)
					{	//For each of the 6 supported strings
						if(tp->note[ctr - 1]->note & bitmask)
						{	//If this string is used
							new_note = eof_track_add_create_note(sp, track, bitmask, tp->note[ctr - 1]->pos, tp->note[ctr - 1]->length, tp->note[ctr - 1]->type, NULL);	//Initialize a new single note at this position
							if(new_note)
							{	//If the new note was created
								new_note->flags = tp->note[ctr - 1]->flags;					//Clone the flags
								new_note->eflags = tp->note[ctr]->eflags;					//Clone the extended flags
								new_note->tflags |= EOF_NOTE_TFLAG_TEMP;					//Mark the note as temporary
								new_note->bendstrength = tp->note[ctr - 1]->bendstrength;	//Copy the bend strength
								new_note->slideend = tp->note[ctr - 1]->slideend;			//And the slide end position
								new_note->frets[ctr2] = tp->note[ctr - 1]->frets[ctr2];		//And this string's fret value
							}
							break;
						}
					}
				}
			}
		}
		eof_track_sort_notes(sp, track);	//Re-sort the notes
	}//If the user opted to export chord techniques to the Rocksmith XML files

	//Write chord templates
	chordlistsize = eof_build_chord_list(sp, track, &chordlist, 1);	//Build a list of all unique chords in the track
	if(!chordlistsize)
	{	//If there were no chords, write an empty chord template tag
		(void) pack_fputs("  <chordTemplates count=\"0\"/>\n", fp);
	}
	else
	{	//There were chords
		long fret0 = 0, fret1 = 0, fret2 = 0, fret3 = 0, fret4 = 0, fret5 = 0;	//Will store the fret number played on each string (-1 means the string is not played)
		long *fret[6] = {&fret0, &fret1, &fret2, &fret3, &fret4, &fret5};	//Allow the fret numbers to be accessed via array
		char *fingerunused = "-1";
		char *finger0 = NULL, *finger1 = NULL, *finger2 = NULL, *finger3 = NULL, *finger4 = NULL, *finger5 = NULL;	//Each will be set to either fingerunknown or fingerunused
		char **finger[6] = {&finger0, &finger1, &finger2, &finger3, &finger4, &finger5};	//Allow the finger strings to be accessed via array
		char finger0def[2] = "0", finger1def[2] = "1", finger2def[2] = "2", finger3def[2] = "3", finger4def[2] = "4", finger5def[2] = "5";	//Static strings for building manually-defined finger information
		char *fingerdef[6] = {finger0def, finger1def, finger2def, finger3def, finger4def, finger5def};	//Allow the fingerdef strings to be accessed via array
		unsigned long shapenum = 0;
		EOF_PRO_GUITAR_NOTE temp = {{0}, 0, 0, 0, {0}, {0}, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};	//Will have a matching chord shape definition's fingering applied to
		unsigned char *effective_fingering;	//Will point to either a note's own finger array or one of that of the temp pro guitar note structure above

		(void) snprintf(buffer, sizeof(buffer) - 1, "  <chordTemplates count=\"%lu\">\n", chordlistsize);
		(void) pack_fputs(buffer, fp);
		for(ctr = 0; ctr < chordlistsize; ctr++)
		{	//For each of the entries in the unique chord list
			notename[0] = '\0';	//Empty the note name string
			assert(chordlist != NULL);	//Unneeded check to resolve a false positive in Splint
			(void) eof_build_note_name(sp, track, chordlist[ctr], notename);	//Build the note name (if it exists) into notename[]

			effective_fingering = tp->note[chordlist[ctr]]->finger;	//By default, use the chord list entry's finger array
			memcpy(temp.frets, tp->note[chordlist[ctr]]->frets, 6);	//Clone the fretting of the chord into the temporary note
			temp.note = tp->note[chordlist[ctr]]->note;				//Clone the note mask
			if(eof_pro_guitar_note_fingering_valid(tp, chordlist[ctr], 0) != 1)
			{	//If the fingering for the note is not fully defined
				if(eof_lookup_chord_shape(tp->note[chordlist[ctr]], &shapenum, 0))
				{	//If a fingering for the chord can be found in the chord shape definitions
					eof_apply_chord_shape_definition(&temp, shapenum);	//Apply the matching chord shape definition's fingering
					effective_fingering = temp.finger;	//Use the matching chord shape definition's finger definitions
				}
			}

			for(ctr2 = 0, bitmask = 1; ctr2 < 6; ctr2++, bitmask <<= 1)
			{	//For each of the 6 supported strings
				if((eof_get_note_note(sp, track, chordlist[ctr]) & bitmask) && (ctr2 < tp->numstrings) && ((tp->note[chordlist[ctr]]->frets[ctr2] & 0x80) == 0))
				{	//If the chord list entry uses this string (verifying that the string number is supported by the track) and the string is not fret hand muted (ghost notes must be allowed so that arpeggio shapes can export)
					*(fret[ctr2]) = tp->note[chordlist[ctr]]->frets[ctr2] & 0x7F;	//Retrieve the fret played on this string (masking out the muting bit)
					*(fret[ctr2]) += tp->capo;	//Apply the capo position
					if(effective_fingering[ctr2])
					{	//If the fingering for this string is defined
						char *str = fingerdef[ctr2];	//Simplify logic below
						str[0] = '0' + effective_fingering[ctr2];	//Convert decimal to ASCII
						str[1] = '\0';	//Truncate string
						if(str[0] == '5')
						{	//If this fingering specifies the thumb
							str[0] = '0';	//Convert from EOF's numbering (5 = thumb) to Rocksmith's numbering (0 = thumb)
						}
						*(finger[ctr2]) = str;
					}
					else
					{	//The fingering is not defined, regardless of whether the string is open or fretted
						*(finger[ctr2]) = fingerunused;		//Write a -1, this will allow the XML to compile even if the chord's fingering is incomplete/undefined
					}
				}
				else
				{	//The chord list entry does not use this string
					*(fret[ctr2]) = -1;
					*(finger[ctr2]) = fingerunused;
				}
			}

			expand_xml_text(buffer2, sizeof(buffer2) - 1, notename, 32, 1);	//Expand XML special characters into escaped sequences if necessary, and check against the maximum supported length of this field.  Filter out characters suspected of causing the game to crash (allow forward slash).
			(void) snprintf(buffer, sizeof(buffer) - 1, "    <chordTemplate chordName=\"%s\" finger0=\"%s\" finger1=\"%s\" finger2=\"%s\" finger3=\"%s\" finger4=\"%s\" finger5=\"%s\" fret0=\"%ld\" fret1=\"%ld\" fret2=\"%ld\" fret3=\"%ld\" fret4=\"%ld\" fret5=\"%ld\"/>\n", buffer2, finger0, finger1, finger2, finger3, finger4, finger5, fret0, fret1, fret2, fret3, fret4, fret5);
			(void) pack_fputs(buffer, fp);
		}//For each of the entries in the unique chord list
		(void) pack_fputs("  </chordTemplates>\n", fp);
	}//There were chords

	//Write some unknown information
	(void) pack_fputs("  <fretHandMuteTemplates count=\"0\"/>\n", fp);

	//Write the beat timings
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <ebeats count=\"%lu\">\n", sp->beats);
	(void) pack_fputs(buffer, fp);
	for(ctr = 0; ctr < sp->beats; ctr++)
	{	//For each beat in the chart
		if(eof_get_ts(sp,&beatspermeasure,NULL,ctr) == 1)
		{	//If this beat has a time signature change
			beatcounter = 0;
		}
		if(!beatcounter)
		{	//If this is the first beat in a measure
			measurenum++;
			displayedmeasure = measurenum;
		}
		else
		{	//Otherwise the measure is displayed as -1 to indicate no change from the previous beat's measure number
			displayedmeasure = -1;
		}
		(void) snprintf(buffer, sizeof(buffer) - 1, "    <ebeat time=\"%.3f\" measure=\"%ld\"/>\n", sp->beat[ctr]->fpos / 1000.0, displayedmeasure);
		(void) pack_fputs(buffer, fp);
		beatcounter++;
		if(beatcounter >= beatspermeasure)
		{
			beatcounter = 0;
		}
	}
	(void) pack_fputs("  </ebeats>\n", fp);

	//Restore the original number of beats in the project in case any were added for DDC
	(void) eof_song_resize_beats(sp, originalbeatcount);

	//Write message boxes for the loading text song property (if defined) and each user defined popup message
	if(sp->tags->loading_text[0] != '\0')
	{	//If the loading text is defined
		char expanded_text[513];	//A string to expand the user defined text into, long enough for the text length limit of 512 + 1 more character for NULL termination
		(void) strftime(expanded_text, sizeof(expanded_text), sp->tags->loading_text, caltime);	//Expand any user defined calendar date/time tokens
		expand_xml_text(buffer2, sizeof(buffer2) - 1, expanded_text, 512, 0);	//Expand XML special characters into escaped sequences if necessary, and check against the maximum supported length of this field

		(void) eof_track_add_section(eof_song, track, EOF_RS_POPUP_MESSAGE, 0, 5100, 10100, 1, buffer2);	//Insert the expanded text as a popup message, setting the flag to nonzero to mark is as temporary
		eof_track_pro_guitar_sort_popup_messages(tp);	//Sort the popup messages
	}
	if(tp->popupmessages || tp->tonechanges)
	{	//If at least one popup message or tone change is to be written
		unsigned long count, controlctr = 0;
		size_t stringlen;
		EOF_RS_CONTROL *controls = NULL;

		//Allocate memory for a list of control events
		eof_track_pro_guitar_sort_tone_changes(tp);	//Re-sort the tone changes
		count = tp->popupmessages * 2;	//Each popup message needs one control message to display and one to clear
		count += tp->tonechanges;		//Each tone change needs one control message
		(void) snprintf(buffer, sizeof(buffer) - 1, "  <controls count=\"%lu\">\n", count);
		(void) pack_fputs(buffer, fp);
		controls = malloc(sizeof(EOF_RS_CONTROL) * count);	//Allocate memory for a list of Rocksmith control events
		if(!controls)
		{
			eof_log("\tError saving:  Cannot allocate memory for control list", 1);
			eof_rs_export_cleanup(sp, track);
			eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view if applicable
			if(chordlist)
			{	//If the chord list was built
				free(chordlist);
			}
			(void) pack_fclose(fp);
			return 0;	//Return failure
		}
		memset(controls, 0, sizeof(EOF_RS_CONTROL) * count);	//Fill with 0s to satisfy Splint

		//Build the list of control events
		for(ctr = 0; ctr < tp->popupmessages; ctr++)
		{	//For each popup message
			//Add the popup message display control to the list
			expand_xml_text(buffer2, sizeof(buffer2) - 1, tp->popupmessage[ctr].name, EOF_SECTION_NAME_LENGTH, 0);	//Expand XML special characters into escaped sequences if necessary, and check against the maximum supported length of this field
			stringlen = (size_t)snprintf(NULL, 0, "    <control time=\"%.3f\" code=\"ShowMessageBox(hint%lu, %s)\"/>\n", tp->popupmessage[ctr].start_pos / 1000.0, ctr + 1, buffer2) + 1;	//Find the number of characters needed to store this string
			controls[controlctr].str = malloc(stringlen + 1);	//Allocate memory to build the string
			if(!controls[controlctr].str)
			{
				eof_log("\tError saving:  Cannot allocate memory for control event", 1);
				while(controlctr > 0)
				{	//Free previously allocated strings
					free(controls[controlctr - 1].str);
					controlctr--;
				}
				free(controls);
				eof_rs_export_cleanup(sp, track);
				eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view if applicable
				if(chordlist)
				{	//If the chord list was built
					free(chordlist);
				}
				(void) pack_fclose(fp);
				return 0;	//Return failure
			}
			(void) snprintf(controls[controlctr].str, stringlen, "    <control time=\"%.3f\" code=\"ShowMessageBox(hint%lu, %s)\"/>\n", tp->popupmessage[ctr].start_pos / 1000.0, ctr + 1, buffer2);
			controls[controlctr].pos = tp->popupmessage[ctr].start_pos;
			controlctr++;

			//Add the clear message control to the list
			stringlen = (size_t)snprintf(NULL, 0, "    <control time=\"%.3f\" code=\"ClearAllMessageBoxes()\"/>\n", tp->popupmessage[ctr].end_pos / 1000.0) + 1;	//Find the number of characters needed to store this string
			controls[controlctr].str = malloc(stringlen + 1);	//Allocate memory to build the string
			if(!controls[controlctr].str)
			{
				eof_log("\tError saving:  Cannot allocate memory for control event", 1);
				while(controlctr > 0)
				{	//Free previously allocated strings
					free(controls[controlctr - 1].str);
					controlctr--;
				}
				free(controls);
				eof_rs_export_cleanup(sp, track);
				eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view if applicable
				if(chordlist)
				{	//If the chord list was built
					free(chordlist);
				}
				(void) pack_fclose(fp);
				return 0;	//Return failure
			}
			(void) snprintf(controls[controlctr].str, stringlen, "    <control time=\"%.3f\" code=\"ClearAllMessageBoxes()\"/>\n", tp->popupmessage[ctr].end_pos / 1000.0);
			controls[controlctr].pos = tp->popupmessage[ctr].end_pos;
			controlctr++;
		}
		for(ctr = 0; ctr < tp->tonechanges; ctr++)
		{	//For each tone change
			//Add the tone change control to the list
			stringlen = (size_t)snprintf(NULL, 0, "    <control time=\"%.3f\" code=\"CDlcTone(%s)\"/>\n", tp->tonechange[ctr].start_pos / 1000.0, tp->tonechange[ctr].name) + 1;	//Find the number of characters needed to store this string
			controls[controlctr].str = malloc(stringlen + 1);	//Allocate memory to build the string
			if(!controls[controlctr].str)
			{
				eof_log("\tError saving:  Cannot allocate memory for control event", 1);
				while(controlctr > 0)
				{	//Free previously allocated strings
					free(controls[controlctr - 1].str);
					controlctr--;
				}
				free(controls);
				eof_rs_export_cleanup(sp, track);
				eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view if applicable
				if(chordlist)
				{	//If the chord list was built
					free(chordlist);
				}
				(void) pack_fclose(fp);
				return 0;	//Return failure
			}
			(void) snprintf(controls[controlctr].str, stringlen, "    <control time=\"%.3f\" code=\"CDlcTone(%s)\"/>\n", tp->tonechange[ctr].start_pos / 1000.0, tp->tonechange[ctr].name);
			controls[controlctr].pos = tp->tonechange[ctr].start_pos;
			controlctr++;
		}

		//Sort, write and free the list of control events
		qsort(controls, (size_t)count, sizeof(EOF_RS_CONTROL), eof_song_qsort_control_events);
		for(ctr = 0; ctr < count; ctr++)
		{	//For each control event
			(void) pack_fputs(controls[ctr].str, fp);	//Write the control event string
			free(controls[ctr].str);	//Free the string
		}
		free(controls);	//Free the array

		(void) pack_fputs("  </controls>\n", fp);

		//Remove any loading text popup that was inserted into the track
		for(ctr = 0; ctr < tp->popupmessages; ctr++)
		{	//For each popup message
			if(tp->popupmessage[ctr].flags)
			{	//If the flags field was made nonzero
				eof_track_pro_guitar_delete_popup_message(tp, ctr);	//Delete this temporary popup message
				break;
			}
		}
	}//If at least one popup message or tone change is to be written

	//Write sections
	for(ctr = 0, numsections = 0; ctr < sp->beats; ctr++)
	{	//For each beat in the chart
		if(sp->beat[ctr]->contained_rs_section_event >= 0)
		{	//If this beat has a Rocksmith section
			numsections++;	//Update Rocksmith section instance counter
		}
	}
	if(numsections)
	{	//If there is at least one Rocksmith section defined in the chart (which should be the case since default ones were inserted earlier if there weren't any)
		(void) snprintf(buffer, sizeof(buffer) - 1, "  <sections count=\"%lu\">\n", numsections);
		(void) pack_fputs(buffer, fp);
		for(ctr = 0; ctr < sp->beats; ctr++)
		{	//For each beat in the chart
			if(sp->beat[ctr]->contained_rs_section_event >= 0)
			{	//If this beat has a Rocksmith section
				expand_xml_text(buffer2, sizeof(buffer2) - 1, sp->text_event[sp->beat[ctr]->contained_rs_section_event]->text, 32, 0);	//Expand XML special characters into escaped sequences if necessary, and check against the maximum supported length of this field
				(void) snprintf(buffer, sizeof(buffer) - 1, "    <section name=\"%s\" number=\"%d\" startTime=\"%.3f\"/>\n", buffer2, sp->beat[ctr]->contained_rs_section_event_instance_number, sp->beat[ctr]->fpos / 1000.0);
				(void) pack_fputs(buffer, fp);
			}
		}
		(void) pack_fputs("  </sections>\n", fp);
	}
	else
	{
		allegro_message("Error:  Default RS sections that were added are missing.  Skipping writing the <sections> tag.");
		eof_log("Error:  Default RS sections that were added are missing.  Skipping writing the <sections> tag.", 1);
	}

	//Write events
	for(ctr = 0, numevents = 0; ctr < sp->text_events; ctr++)
	{	//For each event in the chart
		if(sp->text_event[ctr]->flags & EOF_EVENT_FLAG_RS_EVENT)
		{	//If the event is marked as a Rocksmith event
			if(!sp->text_event[ctr]->track || (sp->text_event[ctr]->track  == track))
			{	//If the event applies to the specified track
				numevents++;
			}
		}
	}
	if(numevents)
	{	//If there is at least one Rocksmith event defined in the chart
		(void) snprintf(buffer, sizeof(buffer) - 1, "  <events count=\"%lu\">\n", numevents);
		(void) pack_fputs(buffer, fp);
		for(ctr = 0, numevents = 0; ctr < sp->text_events; ctr++)
		{	//For each event in the chart
			if(sp->text_event[ctr]->flags & EOF_EVENT_FLAG_RS_EVENT)
			{	//If the event is marked as a Rocksmith event
				if(!sp->text_event[ctr]->track || (sp->text_event[ctr]->track  == track))
				{	//If the event applies to the specified track
					expand_xml_text(buffer2, sizeof(buffer2) - 1, sp->text_event[ctr]->text, 256, 0);	//Expand XML special characters into escaped sequences if necessary, and check against the maximum supported length of this field
					(void) snprintf(buffer, sizeof(buffer) - 1, "    <event time=\"%.3f\" code=\"%s\"/>\n", sp->beat[sp->text_event[ctr]->beat]->fpos / 1000.0, buffer2);
					(void) pack_fputs(buffer, fp);
				}
			}
		}
		(void) pack_fputs("  </events>\n", fp);
	}
	else
	{	//Otherwise write an empty events tag
		(void) pack_fputs("  <events count=\"0\"/>\n", fp);
	}

	//Write note difficulties
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <levels count=\"%u\">\n", numdifficulties);
	(void) pack_fputs(buffer, fp);
	for(ctr = 0, ctr2 = 0; ctr < 256; ctr++)
	{	//For each of the possible difficulties
		if(eof_track_diff_populated_status[ctr])
		{	//If this difficulty is populated
			unsigned long anchorcount;
			char anchorsgenerated = 0;	//Tracks whether anchors were automatically generated and will need to be deleted after export

			//Count the number of single notes and chords in this difficulty
			for(ctr3 = 0, numsinglenotes = 0, numchords = 0; ctr3 < tp->notes; ctr3++)
			{	//For each note in the track
				if(eof_get_note_type(sp, track, ctr3) == ctr)
				{	//If the note is in this difficulty
					unsigned long lanecount = eof_note_count_rs_lanes(sp, track, ctr3, 1);	//Count the number of non ghosted/muted gems for this note
					if(lanecount == 1)
					{	//If the note has only one gem
						numsinglenotes++;	//Increment counter
					}
					else if(lanecount > 1)
					{	//If the note has multiple gems
						numchords++;	//Increment counter
					}
				}
			}

			//Write single notes
			(void) snprintf(buffer, sizeof(buffer) - 1, "    <level difficulty=\"%lu\">\n", ctr2);
			(void) pack_fputs(buffer, fp);
			ctr2++;	//Increment the populated difficulty level number
			if(numsinglenotes)
			{	//If there's at least one single note in this difficulty
				(void) snprintf(buffer, sizeof(buffer) - 1, "      <notes count=\"%lu\">\n", numsinglenotes);
				(void) pack_fputs(buffer, fp);
				for(ctr3 = 0; ctr3 < tp->notes; ctr3++)
				{	//For each note in the track
					if((eof_get_note_type(sp, track, ctr3) == ctr) && (eof_note_count_rs_lanes(sp, track, ctr3, 1) == 1))
					{	//If this note is in this difficulty and will export as a single note (only one gem has non ghosted/muted status)
						for(stringnum = 0, bitmask = 1; stringnum < tp->numstrings; stringnum++, bitmask <<= 1)
						{	//For each string used in this track
							if((eof_get_note_note(sp, track, ctr3) & bitmask) && ((tp->note[ctr3]->frets[stringnum] & 0x80) == 0) && !(tp->note[ctr3]->ghost & bitmask))
							{	//If this string is used in this note, it is not fret hand muted and it is not ghosted
								unsigned long flags = eof_get_note_flags(sp, track, ctr3);

								if((flags & EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE) == 0)
								{	//At this point, it doesn't seem Rocksmith supports string muted notes
									EOF_RS_TECHNIQUES tech = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 ,0};
									unsigned long notepos;
									unsigned long fret;				//The fret number used for this string

									(void) eof_get_rs_techniques(sp, track, ctr3, stringnum, &tech, 1, 0);	//Determine techniques used by this note (run the RS1 check to ensure a pop/slap note isn't allowed to also have bend/slide technique)
									notepos = eof_get_note_pos(sp, track, ctr3);
									fret = tp->note[ctr3]->frets[stringnum] & 0x7F;	//Get the fret value for this string (mask out the muting bit)
									fret += tp->capo;	//Apply the capo position
									if(!eof_pro_guitar_note_lowest_fret(tp, ctr3))
									{	//If this note contains no fretted strings
										if(tech.bend || (tech.slideto >= 0))
										{	//If it is also marked as a bend or slide note, omit these statuses because they're invalid for open notes
											tech.bend = tech.bendstrength_h = tech.bendstrength_q = 0;
											tech.slideto = -1;
											if((*user_warned & 4) == 0)
											{	//If the user wasn't alerted that one or more open notes have these statuses improperly applied
												eof_seek_and_render_position(track, eof_get_note_type(sp, track, ctr3), eof_get_note_pos(sp, track, ctr3));
												allegro_message("Warning:  At least one open note is marked with bend or slide status.\nThis is not supported, so these statuses are being omitted for such notes.");
												*user_warned |= 4;
											}
										}
									}
									(void) snprintf(buffer, sizeof(buffer) - 1, "        <note time=\"%.3f\" bend=\"%lu\" fret=\"%lu\" hammerOn=\"%d\" harmonic=\"%d\" hopo=\"%d\" ignore=\"0\" palmMute=\"%d\" pluck=\"%d\" pullOff=\"%d\" slap=\"%d\" slideTo=\"%ld\" string=\"%lu\" sustain=\"%.3f\" tremolo=\"%d\"/>\n", (double)notepos / 1000.0, tech.bendstrength_h, fret, tech.hammeron, tech.harmonic, tech.hopo, tech.palmmute, tech.pop, tech.pulloff, tech.slap, tech.slideto, stringnum, (double)tech.length / 1000.0, tech.tremolo);
									(void) pack_fputs(buffer, fp);
									break;	//Only one note entry is valid for each single note, so break from loop
								}//If the note isn't string muted
							}//If this string is used in this note
						}//For each string used in this track
					}//If this note is in this difficulty and is a single note (and not a chord)
				}//For each note in the track
				(void) pack_fputs("      </notes>\n", fp);
			}//If there's at least one single note in this difficulty
			else
			{	//There are no single notes in this difficulty, write an empty notes tag
				(void) pack_fputs("      <notes count=\"0\"/>\n", fp);
			}

			//Write chords
			if(numchords)
			{	//If there's at least one chord in this difficulty
				char *upstrum = "up";
				char *downstrum = "down";
				char *direction;	//Will point to either upstrum or downstrum as appropriate
				double notepos;
				char highdensity;		//Any chord within the threshold proximity of an identical chord has the highDensity boolean property set to true

				(void) snprintf(buffer, sizeof(buffer) - 1, "      <chords count=\"%lu\">\n", numchords);
				(void) pack_fputs(buffer, fp);
				for(ctr3 = 0; ctr3 < tp->notes; ctr3++)
				{	//For each note in the track
					if((eof_get_note_type(sp, track, ctr3) == ctr) && (eof_note_count_rs_lanes(sp, track, ctr3, 1) > 1))
					{	//If this note is in this difficulty and will export as a chord (at least two non ghosted/muted gems)
						for(ctr4 = 0; ctr4 < chordlistsize; ctr4++)
						{	//For each of the entries in the unique chord list
							assert(chordlist != NULL);	//Unneeded check to resolve a false positive in Splint
							if(!eof_note_compare_simple(sp, track, ctr3, chordlist[ctr4]))
							{	//If this note matches a chord list entry
								if(!eof_pro_guitar_note_compare_fingerings(tp->note[ctr3], tp->note[chordlist[ctr4]]))
								{	//If this note has identical fingering to chord list entry
									if(!strcmp(tp->note[ctr3]->name, tp->note[chordlist[ctr4]]->name))
									{	//If the chord names match
										chordid = ctr4;	//Store the chord list entry number
										break;
									}
								}
							}
						}
						if(ctr4 >= chordlistsize)
						{	//If the chord couldn't be found
							eof_seek_and_render_position(track, tp->note[ctr3]->type, tp->note[ctr3]->pos);
							allegro_message("Error:  Couldn't match chord with chord template while exporting chords.  Aborting Rocksmith 1 export.");
							eof_log("Error:  Couldn't match chord with chord template while exporting chords.  Aborting Rocksmith 1 export.", 1);
							if(chordlist)
							{	//If the chord list was built
								free(chordlist);
							}
							eof_rs_export_cleanup(sp, track);
							eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view if applicable
							(void) pack_fclose(fp);
							return 0;	//Return error
						}
						if(tp->note[ctr3]->flags & EOF_PRO_GUITAR_NOTE_FLAG_UP_STRUM)
						{	//If this note explicitly strums up
							direction = upstrum;	//Set the direction string to match
						}
						else
						{	//Otherwise the direction defaults to down
							direction = downstrum;
						}
						highdensity = eof_note_has_high_chord_density(sp, track, ctr3, 1);	//Determine whether the chord will export with high density
						notepos = (double)tp->note[ctr3]->pos / 1000.0;
						(void) snprintf(buffer, sizeof(buffer) - 1, "        <chord time=\"%.3f\" chordId=\"%lu\" highDensity=\"%d\" ignore=\"0\" strum=\"%s\"/>\n", notepos, chordid, highdensity, direction);
						(void) pack_fputs(buffer, fp);
					}//If this note is in this difficulty and is a chord
				}//For each note in the track
				(void) pack_fputs("      </chords>\n", fp);
			}
			else
			{	//There are no chords in this difficulty, write an empty chords tag
				(void) pack_fputs("      <chords count=\"0\"/>\n", fp);
			}

			//Write other stuff
			(void) pack_fputs("      <fretHandMutes count=\"0\"/>\n", fp);

			//Write anchors (fret hand positions)
			for(ctr3 = 0, anchorcount = 0; ctr3 < tp->handpositions; ctr3++)
			{	//For each hand position defined in the track
				if(tp->handposition[ctr3].difficulty == ctr)
				{	//If the hand position is in this difficulty
					anchorcount++;
				}
			}
			if(!anchorcount)
			{	//If there are no anchors in this track difficulty, automatically generate them
				if((*user_warned & 1) == 0)
				{	//If the user wasn't alerted that one or more track difficulties have no fret hand positions defined
					allegro_message("Warning:  At least one track difficulty has no fret hand positions defined.  They will be created automatically.");
					*user_warned |= 1;
				}
				eof_fret_hand_position_list_dialog_undo_made = 1;	//Ensure no undo state is written during export
				eof_generate_efficient_hand_positions(sp, track, ctr, 0, 0);	//Generate the fret hand positions for the track difficulty being currently written (use a static fret range tolerance of 4 for all frets)
				anchorsgenerated = 1;
			}
			for(ctr3 = 0, anchorcount = 0; ctr3 < tp->handpositions; ctr3++)	//Re-count the hand positions
			{	//For each hand position defined in the track
				if(tp->handposition[ctr3].difficulty == ctr)
				{	//If the hand position is in this difficulty
					anchorcount++;
				}
			}
			if(anchorcount)
			{	//If there's at least one anchor in this difficulty
				(void) snprintf(buffer, sizeof(buffer) - 1, "      <anchors count=\"%lu\">\n", anchorcount);
				(void) pack_fputs(buffer, fp);
				for(ctr3 = 0, anchorcount = 0; ctr3 < tp->handpositions; ctr3++)
				{	//For each hand position defined in the track
					if(tp->handposition[ctr3].difficulty == ctr)
					{	//If the hand position is in this difficulty
						unsigned long fret = tp->handposition[ctr3].end_pos + tp->capo;	//Apply the capo position
						(void) snprintf(buffer, sizeof(buffer) - 1, "        <anchor time=\"%.3f\" fret=\"%lu\"/>\n", (double)tp->handposition[ctr3].start_pos / 1000.0, fret);
						(void) pack_fputs(buffer, fp);
					}
				}
				(void) pack_fputs("      </anchors>\n", fp);
			}
			else
			{	//There are no anchors in this difficulty, write an empty anchors tag
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Error:  Failed to automatically generate fret hand positions for level %lu of\n\"%s\" during MIDI export.", ctr2, fn);
				allegro_message("%s", eof_log_string);
				eof_log(eof_log_string, 1);
				(void) pack_fputs("      <anchors count=\"0\"/>\n", fp);
			}
			if(anchorsgenerated)
			{	//If anchors were automatically generated for this track difficulty, remove them now
				for(ctr3 = tp->handpositions; ctr3 > 0; ctr3--)
				{	//For each hand position defined in the track, in reverse order
					if(tp->handposition[ctr3 - 1].difficulty == ctr)
					{	//If the hand position is in this difficulty
						eof_pro_guitar_track_delete_hand_position(tp, ctr3 - 1);	//Delete the hand position
					}
				}
			}

			//Count/write the hand shapes
			for(handshapeloop = 0, handshapectr = 0; handshapeloop < 2; handshapeloop++)
			{	//On first pass, reset the handshape counter and count the number of handshapes.  On second pass, write handshapes.
				if(handshapeloop)
				{	//If this is the second pass, write the opening of the handshapes tag
					if(!handshapectr)
					{	//If there were no handshapes
						(void) pack_fputs("      <handShapes count=\"0\"/>\n", fp);
						break;	//Exit loop
					}
					(void) snprintf(buffer, sizeof(buffer) - 1, "      <handShapes count=\"%lu\">\n", handshapectr);
					(void) pack_fputs(buffer, fp);
				}

				for(ctr3 = 0; ctr3 < tp->notes; ctr3++)
				{	//For each note in the track
					if((eof_get_note_type(sp, track, ctr3) == ctr) && ((eof_note_count_rs_lanes(sp, track, ctr3, 1) > 1) || ((eof_note_count_rs_lanes(sp, track, ctr3, 4) > 1) && tp->note[ctr3]->tflags & EOF_NOTE_TFLAG_ARP_FIRST)))
					{	//If this note is in this difficulty and will export as a chord (at least two non ghosted/muted gems) or an arpeggio/handshape (at least two non muted notes)
						unsigned long chordnum = ctr3;	//Store a copy of this note number because ctr3 will be manipulated below

						//Find this chord's ID
						for(ctr4 = 0; ctr4 < chordlistsize; ctr4++)
						{	//For each of the entries in the unique chord list
							assert(chordlist != NULL);	//Unneeded check to resolve a false positive in Splint
							if(!eof_note_compare_simple(sp, track, ctr3, chordlist[ctr4]))
							{	//If this note matches a chord list entry
								if(!eof_pro_guitar_note_compare_fingerings(tp->note[ctr3], tp->note[chordlist[ctr4]]))
								{	//If this note has identical fingering to chord list entry
									if(!strcmp(tp->note[ctr3]->name, tp->note[chordlist[ctr4]]->name))
									{	//If the chord names match
										chordid = ctr4;	//Store the chord list entry number
										break;
									}
								}
							}
						}
						if(ctr4 >= chordlistsize)
						{	//If the chord couldn't be found
							eof_seek_and_render_position(track, tp->note[ctr3]->type, tp->note[ctr3]->pos);
							allegro_message("Error:  Couldn't match chord with chord template while writing handshapes.  Aborting Rocksmith 1 export.");
							eof_log("Error:  Couldn't match chord with chord template while writing handshapes.  Aborting Rocksmith 1 export.", 1);
							if(chordlist)
							{	//If the chord list was built
								free(chordlist);
							}
							eof_rs_export_cleanup(sp, track);	//Remove all temporary notes that were added
							eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view if applicable
							(void) pack_fclose(fp);
							return 0;	//Return error
						}
						handshapestart = eof_get_note_pos(sp, track, ctr3);	//Store this chord's start position (in seconds)

						//If this chord is at the beginning of an arpeggio phrase, skip the rest of the notes in that phrase
						for(ctr5 = 0; ctr5 < tp->arpeggios; ctr5++)
						{	//For each arpeggio phrase in the track
							if(((tp->note[ctr3]->pos + 10 >= tp->arpeggio[ctr5].start_pos) && (tp->note[ctr3]->pos <= tp->arpeggio[ctr5].start_pos + 10)) && (tp->note[ctr3]->type == tp->arpeggio[ctr5].difficulty))
							{	//If this chord's start position is within 10ms of an arpeggio phrase in this track difficulty
								while(1)
								{
									nextnote = eof_track_fixup_next_note(sp, track, ctr3);
									if((nextnote >= 0) && (tp->note[nextnote]->pos <= tp->arpeggio[ctr5].end_pos))
									{	//If there is another note and it is in the same arpeggio phrase
										ctr3 = nextnote;	//Iterate to that note, and check subsequent notes to see if they are also in the phrase
									}
									else
									{	//The next note (if any) is not in the arpeggio phrase
										break;	//Break from while loop
									}
								}
								break;	//Break from for loop
							}
						}

						//Examine subsequent notes to see if they match this chord
						while(1)
						{
							nextnote = eof_track_fixup_next_note(sp, track, ctr3);
							if((nextnote >= 0) && !eof_note_compare_simple(sp, track, chordnum, nextnote) && !eof_is_partially_ghosted(sp, track, nextnote) && !eof_pro_guitar_note_compare_fingerings(tp->note[chordnum], tp->note[nextnote]))
							{	//If there is another note, it matches this chord, it is not partially ghosted (an arpeggio) and it has the same fingering
								ctr3 = nextnote;	//Iterate to that note, and check subsequent notes to see if they match
							}
							else
							{	//The next note (if any) is not a repeat of this note
								handshapeend = eof_get_note_pos(sp, track, ctr3) + eof_get_note_length(sp, track, ctr3);	//End the hand shape at the end of this chord

								if((handshapeend - handshapestart < 56) && (handshapestart + 56 < eof_get_note_pos(sp, track, nextnote)))
								{	//If the hand shape would be shorter than 56ms, and the next note is further than 56ms away
									handshapeend = eof_get_note_pos(sp, track, ctr3) + 56;	//Pad the hand shape to 56ms
								}
								break;	//Break from while loop
							}
						}

						if(!handshapeloop)
						{	//If this is the first pass
							handshapectr++;	//One more hand shape has been counted
						}
						else
						{	//Second pass, pad and write the handshape tag
							(void) snprintf(buffer, sizeof(buffer) - 1, "        <handShape chordId=\"%lu\" endTime=\"%.3f\" startTime=\"%.3f\"/>\n", chordid, (double)handshapeend / 1000.0, (double)handshapestart / 1000.0);
							(void) pack_fputs(buffer, fp);
						}
					}//If this note is in this difficulty and will export as a chord (at least two non ghosted/muted gems)
				}//For each note in the track
			}//On first pass, reset the handshape counter and count the number of handshapes.  On second pass, write handshapes.
			if(handshapectr)
			{	//If there were handshapes
				(void) pack_fputs("      </handShapes>\n", fp);
			}

			//Write closing level tag
			(void) pack_fputs("    </level>\n", fp);
		}//If this difficulty is populated
	}//For each of the available difficulties
	(void) pack_fputs("  </levels>\n", fp);
	(void) pack_fputs("</song>\n", fp);
	(void) pack_fclose(fp);

	//Cleanup
	if(chordlist)
	{	//If the chord list was built
		free(chordlist);
	}
	//Remove all temporary text events that were added
	for(ctr = sp->text_events; ctr > 0; ctr--)
	{	//For each text event (in reverse order)
		if(sp->text_event[ctr - 1]->is_temporary)
		{	//If this text event has been marked as temporary
			eof_song_delete_text_event(sp, ctr - 1);	//Delete it
		}
	}
	eof_sort_events(sp);	//Re-sort events
	eof_rs_export_cleanup(sp, track);	//Remove all temporary notes that were added
	eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view if applicable

	return 1;	//Return success
}

int eof_export_rocksmith_2_track(EOF_SONG * sp, char * fn, unsigned long track, unsigned short *user_warned)
{
	PACKFILE * fp;
	char buffer[600] = {0}, buffer2[512] = {0};
	time_t seconds;		//Will store the current time in seconds
	struct tm *caltime;	//Will store the current time in calendar format
	unsigned long ctr, ctr2, ctr3, ctr4, ctr5, handshapeloop, numsections, stringnum, bitmask, numsinglenotes, numchords, *chordlist = NULL, chordlistsize, xml_end, numevents = 0;
	EOF_PRO_GUITAR_TRACK *tp;
	char *arrangement_name;	//This will point to the track's native name unless it has an alternate name defined
	unsigned numdifficulties;
	unsigned char bre_populated = 0;
	unsigned beatspermeasure = 4, beatcounter = 0;
	long displayedmeasure, measurenum = 0;
	char standard[] = {0,0,0,0,0,0};
	char tuning_all_same = 1;	//Set to nonzero if any strings are found to be tuned a different amount of half steps from standard
	char tuning[6] = {0};
	char notename[EOF_NAME_LENGTH+1] = {0};	//String large enough to hold any chord name supported by EOF
	int scale = 0, chord = 0, isslash = 0, bassnote = 0;	//Used for power chord detection
	int standard_tuning = 0, non_standard_chords = 0, barre_chords = 0, power_chords = 0, notenum, dropd_tuning = 1, dropd_power_chords = 0, open_chords = 0, double_stops = 0, palm_mutes = 0, harmonics = 0, hopo = 0, tremolo = 0, slides = 0, bends = 0, tapping = 0, vibrato = 0, slappop = 0, octaves = 0, fifths_and_octaves = 0, sustains = 0, pinch= 0;	//Used for technique detection
	int is_lead = 0, is_rhythm = 0, is_bass = 0;	//Is set to nonzero if the specified track is to be considered any of these arrangement types
	unsigned long chordid = 0, handshapectr = 0;
	unsigned long handshapestart = 0, handshapeend = 0;
	long nextnote, prevnote;
	unsigned long originalbeatcount;	//If beats are padded to reach the beginning of the next measure (for DDC), this will track the project's original number of beats
	EOF_RS_TECHNIQUES tech = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 ,0};
	EOF_PRO_GUITAR_NOTE *new_note;
	char restore_tech_view = 0;			//If tech view is in effect, it is temporarily disabled so that the correct notes are exported
	char match;		//Used for testing whether partially ghosted chords are inside of arpeggio phrases

	eof_log("eof_export_rocksmith_2_track() entered", 1);

	if(!sp || !fn || !sp->beats || (track >= sp->tracks) || (sp->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT) || !user_warned)
	{
		eof_log("\tError saving:  Invalid parameters", 1);
		return 0;	//Return failure
	}

	tp = sp->pro_guitar_track[sp->track[track]->tracknum];
	restore_tech_view = eof_menu_track_get_tech_view_state(sp, track);
	eof_menu_track_set_tech_view_state(sp, track, 0);	//Disable tech view if applicable
	if(eof_get_highest_fret(sp, track, 0) > 24)
	{	//If the track being exported uses any frets higher than 24
		if((*user_warned & 256) == 0)
		{	//If the user wasn't alerted about this issue yet
			allegro_message("Warning:  At least one track (\"%s\") uses a fret higher than 24.  These won't display correctly in Rocksmith 2.", sp->track[track]->name);
			*user_warned |= 256;
		}
	}
	for(ctr = 0; ctr < eof_get_track_size(sp, track); ctr++)
	{	//For each note in the track
		unsigned char slideend;
		unsigned long flags = eof_get_note_flags(sp, track, ctr);

		if((flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP) || (flags & EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE))
		{	//If the note slides up or unpitched slides up
			slideend = tp->note[ctr]->slideend + tp->capo;	//Obtain the end position of the slide, take the capo position into account
			if((flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP) && !(flags & EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION))
			{	//If this pitched slide's end position is not defined
				slideend = eof_get_highest_fret_value(sp, track, ctr) + 1;	//Assume a 1 fret slide
			}
			if((slideend >= 22) || ((tp->note[ctr]->unpitchend + tp->capo) >= 22))
			{	//If either type of slide goes to or above fret 22 (after taking the capo position into account)
				if((*user_warned & 8) == 0)
				{	//If the user wasn't alerted about this issue yet
					eof_seek_and_render_position(track, tp->note[ctr]->type, tp->note[ctr]->pos);	//Show the offending note
					allegro_message("Warning:  At least one note slides to or above fret 22.  This could cause Rocksmith 2 to crash.");
					*user_warned |= 8;
				}
				break;
			}
		}
	}

	//Count the number of populated difficulties in the track
	(void) eof_detect_difficulties(sp, track);	//Update eof_track_diff_populated_status[] to reflect all populated difficulties for this track
	if((sp->track[track]->flags & EOF_TRACK_FLAG_UNLIMITED_DIFFS) == 0)
	{	//If the track is using the traditional 5 difficulty system
		if(eof_track_diff_populated_status[4])
		{	//If the BRE difficulty is populated
			bre_populated = 1;	//Track that it was
		}
		eof_track_diff_populated_status[4] = 0;	//Ensure that the BRE difficulty is not exported
	}
	for(ctr = 0, numdifficulties = 0; ctr < 256; ctr++)
	{	//For each possible difficulty
		if(eof_track_diff_populated_status[ctr])
		{	//If this difficulty is populated
			numdifficulties++;	//Increment this counter
		}
	}
	if(!numdifficulties)
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Cannot export track \"%s\"in Rocksmith format, it has no populated difficulties", sp->track[track]->name);
		eof_log(eof_log_string, 1);
		if(bre_populated && ((*user_warned & 1024) == 0))
		{	//If the BRE difficulty was the only one populated, warn that it is being omitted (unless the user was already warned of this)
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Warning:  Track \"%s\" only has notes in the BRE difficulty.\nThese are not exported in Rocksmith format unless you remove the difficulty limit (Track>Rocksmith>Remove difficulty limit).", sp->track[track]->name);
			allegro_message("%s", eof_log_string);
			eof_log(eof_log_string, 1);
			*user_warned |= 1024;
		}
		eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view if applicable
		return 0;	//Return failure
	}

	//Update target file name and open it for writing
	if((sp->track[track]->flags & EOF_TRACK_FLAG_ALT_NAME) && (sp->track[track]->altname[0] != '\0'))
	{	//If the track has an alternate name
		arrangement_name = sp->track[track]->altname;
	}
	else
	{	//Otherwise use the track's native name
		arrangement_name = sp->track[track]->name;
	}
	(void) snprintf(buffer, 600, "%s_RS2.xml", arrangement_name);
	(void) replace_filename(fn, fn, buffer, 1024);
	fp = pack_fopen(fn, "w");
	if(!fp)
	{
		eof_log("\tError saving:  Cannot open file for writing", 1);
		eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view if applicable
		return 0;	//Return failure
	}

	//Update the track's arrangement name
	if(tp->arrangement)
	{	//If the track's arrangement type has been defined
		arrangement_name = eof_rs_arrangement_names[tp->arrangement];	//Use the corresponding string in the XML file
	}

	//Get the smaller of the chart length and the music length, this will be used to write the songlength tag
	xml_end = eof_music_length;
	if(eof_silence_loaded || (eof_chart_length < eof_music_length))
	{	//If the chart length is shorter than the music length, or there is no chart audio loaded
		xml_end = eof_chart_length;	//Use the chart's length instead
	}

	//Write the beginning of the XML file
	(void) pack_fputs("<?xml version='1.0' encoding='UTF-8'?>\n", fp);
	(void) pack_fputs("<song version=\"7\">\n", fp);
	(void) pack_fputs("<!-- " EOF_VERSION_STRING " -->\n", fp);	//Write EOF's version in an XML comment
	expand_xml_text(buffer2, sizeof(buffer2) - 1, sp->tags->title, 64, 0);	//Expand XML special characters into escaped sequences if necessary, and check against the maximum supported length of this field
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <title>%s</title>\n", buffer2);
	(void) pack_fputs(buffer, fp);
	expand_xml_text(buffer2, sizeof(buffer2) - 1, arrangement_name, 32, 0);	//Expand XML special characters into escaped sequences if necessary, and check against the maximum supported length of this field
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <arrangement>%s</arrangement>\n", buffer2);
	(void) pack_fputs(buffer, fp);
	(void) pack_fputs("  <part>1</part>\n", fp);
	(void) pack_fputs("  <offset>0.000</offset>\n", fp);
	(void) pack_fputs("  <centOffset>0</centOffset>\n", fp);
	eof_truncate_chart(sp);	//Update the chart length
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <songLength>%.3f</songLength>\n", (double)(xml_end - 1) / 1000.0);	//Make sure the song length is not longer than the actual audio, or the chart won't reach an end in-game
	(void) pack_fputs(buffer, fp);
	seconds = time(NULL);
	caltime = localtime(&seconds);
	if(caltime)
	{	//If the calendar time could be determined
		(void) snprintf(buffer, sizeof(buffer) - 1, "  <lastConversionDateTime>%d-%d-%d %d:%02d</lastConversionDateTime>\n", caltime->tm_mon + 1, caltime->tm_mday, caltime->tm_year % 100, caltime->tm_hour, caltime->tm_min);
	}
	else
	{
		(void) snprintf(buffer, sizeof(buffer) - 1, "  <lastConversionDateTime>UNKNOWN</lastConversionDateTime>\n");
	}
	(void) pack_fputs(buffer, fp);

	//Write additional tags to pass song information to the Rocksmith toolkit
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <startBeat>%.3f</startBeat>\n", sp->beat[0]->fpos / 1000.0);	//The position of the first beat
	(void) pack_fputs(buffer, fp);
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <averageTempo>%.3f</averageTempo>\n", 60000.0 / ((sp->beat[sp->beats - 1]->fpos - sp->beat[0]->fpos) / sp->beats));	//The average tempo (60000ms / the average beat length in ms)
	(void) pack_fputs(buffer, fp);
	for(ctr = 0; ctr < 6; ctr++)
	{	//For each string EOF supports
		if(ctr >= tp->numstrings)
		{	//If the track doesn't use this string
			tp->tuning[ctr] = 0;	//Ensure the tuning is cleared accordingly
		}
	}
	is_bass = eof_track_is_bass_arrangement(tp, track);
	if(tp->arrangement == 2)
	{	//Rhythm arrangement
		is_rhythm = 1;
	}
	else if(tp->arrangement == 3)
	{	//Lead arrangement
		is_lead = 1;
	}
	for(ctr = 0; ctr < 6; ctr++)
	{	//For each of the 6 supported strings
		if(ctr < tp->numstrings)
		{	//If this string is used in the track
			if(ctr && (tp->tuning[ctr - 1] != tp->tuning[ctr]))
			{	//If this isn't the first string, and this string's tuning value doesn't match the previous one
				tuning_all_same = 0;
			}
			tuning[ctr] = tp->tuning[ctr];
		}
		else
		{
			if(ctr && tuning_all_same)
			{	//If ctr is greater than 0 as expected and all the strings were tuned the same amount from standard
				tuning[ctr] = tuning[ctr - 1];	//Give this unused string a matching tuning value
			}
		}
	}
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <tuning string0=\"%d\" string1=\"%d\" string2=\"%d\" string3=\"%d\" string4=\"%d\" string5=\"%d\" />\n", tuning[0], tuning[1], tuning[2], tuning[3], tuning[4], tuning[5]);
	(void) pack_fputs(buffer, fp);
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <capo>%u</capo>\n", tp->capo);
	(void) pack_fputs(buffer, fp);
	expand_xml_text(buffer2, sizeof(buffer2) - 1, sp->tags->artist, 256, 0);	//Replace any special characters in the artist song property with escape sequences if necessary
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <artistName>%s</artistName>\n", buffer2);
	(void) pack_fputs(buffer, fp);
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <artistNameSort>%s</artistNameSort>\n", buffer2);
	(void) pack_fputs(buffer, fp);
	expand_xml_text(buffer2, sizeof(buffer2) - 1, sp->tags->album, 256, 0);	//Replace any special characters in the album song property with escape sequences if necessary
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <albumName>%s</albumName>\n", buffer2);
	(void) pack_fputs(buffer, fp);
	expand_xml_text(buffer2, sizeof(buffer2) - 1, sp->tags->year, 32, 0);	//Replace any special characters in the year song property with escape sequences if necessary
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <albumYear>%s</albumYear>\n", buffer2);
	(void) pack_fputs(buffer, fp);
	(void) pack_fputs("  <crowdSpeed>1</crowdSpeed>\n", fp);

	//Determine arrangement properties
	if(!memcmp(tuning, standard, 6))
	{	//All unused strings had their tuning set to 0, so if all bytes of this array are 0, the track is in standard tuning
		standard_tuning = 1;
	}
	notenum = eof_lookup_tuned_note(tp, track, 0, tp->tuning[0]);	//Look up the open note the lowest string plays
	notenum %= 12;	//Ensure the value is in the range of [0,11]
	if(notenum == 5)
	{	//If the lowest string is tuned to D
		for(ctr = 1; ctr < 6; ctr++)
		{	//For the other 5 strings
			if(tp->tuning[ctr] != 0)
			{	//If the string is not in standard tuning
				dropd_tuning = 0;
			}
		}
	}
	else
	{	//The lowest string is not tuned to D
		dropd_tuning = 0;
	}
	eof_determine_phrase_status(sp, track);	//Update the tremolo status of each note
	for(ctr = 0; ctr < tp->notes; ctr++)
	{	//For each note in the track
		if(eof_note_count_rs_lanes(sp, track, ctr, 2) > 1)
		{	//If the note will export as a chord (more than one non ghosted/muted gem)
			if(!non_standard_chords && !eof_build_note_name(sp, track, ctr, notename))
			{	//If the chord has no defined or detected name (only if this condition hasn't been found already)
				non_standard_chords = 1;
			}
			if(!barre_chords && eof_pro_guitar_note_is_barre_chord(tp, ctr))
			{	//If the chord is a barre chord (only if this condition hasn't been found already)
				barre_chords = 1;
			}
			if(!power_chords && eof_lookup_chord(tp, track, ctr, &scale, &chord, &isslash, &bassnote, 0, 0))
			{	//If the chord lookup found a match (only if this condition hasn't been found already)
				if(chord == 27)
				{	//27 is the index of the power chord formula in eof_chord_names[]
					power_chords = 1;
					if(dropd_tuning)
					{	//If this track is in drop d tuning
						dropd_power_chords = 1;
					}
				}
			}
			if(!open_chords)
			{	//Only if no open chords have been found already
				for(ctr2 = 0, bitmask = 1; ctr2 < 6; ctr2++, bitmask <<= 1)
				{	//For each of the 6 supported strings
					if((tp->note[ctr]->note & bitmask) && (tp->note[ctr]->frets[ctr2] == 0))
					{	//If this string is used and played open
						open_chords = 1;
					}
				}
			}
			if(!double_stops && eof_pro_guitar_note_is_double_stop(tp, ctr))
			{	//If the chord is a double stop (only if this condition hasn't been found already)
				double_stops = 1;
			}
			if(is_bass)
			{	//If the arrangement being exported is bass
				int thisnote, lastnote = -1, failed = 0;
				for(ctr2 = 0, bitmask = 1; ctr2 < 6; ctr2++, bitmask <<= 1)
				{	//For each of the 6 supported strings
					if((tp->note[ctr]->note & bitmask) && !(tp->note[ctr]->frets[ctr2] & 0x80))
					{	//If this string is played and not string muted
						thisnote = eof_lookup_played_note(tp, track, ctr2, tp->note[ctr]->frets[ctr2]);	//Determine what note is being played
						if((lastnote >= 0) && (lastnote != thisnote))
						{	//If this string's played note doesn't match the note played by previous strings
							failed = 1;
							break;
						}
						lastnote = thisnote;
					}
				}
				if(!failed)
				{	//If non muted strings in the chord played the same note
					octaves = 1;
				}
			}
		}//If the note is a chord (more than one non ghosted gem)
		else
		{	//If it will export as a single note
			if(tp->note[ctr]->length > 1)
			{	//If the note is a sustain
				sustains = 1;
			}
		}
		if(tp->note[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE)
		{	//If the note is palm muted
			palm_mutes = 1;
		}
		if(tp->note[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC)
		{	//If the note is a harmonic
			harmonics = 1;
		}
		if((tp->note[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_HO) || (tp->note[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_PO))
		{	//If the note is a hammer on or pull off
			hopo = 1;
		}
		if(tp->note[ctr]->flags & EOF_NOTE_FLAG_IS_TREMOLO)
		{	//If the note is played with tremolo
			tremolo = 1;
		}
		if((tp->note[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP) || (tp->note[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN))
		{	//If the note slides up or down
			slides = 1;
		}
		if(tp->note[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_BEND)
		{	//If the note is bent
			bends = 1;
		}
		if(tp->note[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_TAP)
		{	//If the note is tapped
			tapping = 1;
		}
		if(tp->note[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO)
		{	//If the note is played with vibrato
			vibrato = 1;
		}
		if((tp->note[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_SLAP) || (tp->note[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_POP))
		{	//If the note is played by slapping or popping
			slappop = 1;
		}
		if(tp->note[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_P_HARMONIC)
		{	//If the note is played with pinch harmonic
			pinch = 1;
		}
	}//For each note in the track
	if(is_bass)
	{	//If the arrangement being exported is bass
		if(double_stops || octaves)
		{	//If either of these techniques were detected
			fifths_and_octaves = 1;
		}
		double_stops = 0;
	}
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <arrangementProperties represent=\"1\" bonusArr=\"0\" standardTuning=\"%d\" nonStandardChords=\"%d\" barreChords=\"%d\" powerChords=\"%d\" dropDPower=\"%d\" openChords=\"%d\" fingerPicking=\"0\" pickDirection=\"0\" doubleStops=\"%d\" palmMutes=\"%d\" harmonics=\"%d\" pinchHarmonics=\"%d\" hopo=\"%d\" tremolo=\"%d\" slides=\"%d\" unpitchedSlides=\"0\" bends=\"%d\" tapping=\"%d\" vibrato=\"%d\" fretHandMutes=\"0\" slapPop=\"%d\" twoFingerPicking=\"0\" fifthsAndOctaves=\"%d\" syncopation=\"0\" bassPick=\"0\" sustain=\"%d\" pathLead=\"%d\" pathRhythm=\"%d\" pathBass=\"%d\" />\n", standard_tuning, non_standard_chords, barre_chords, power_chords, dropd_power_chords, open_chords, double_stops, palm_mutes, harmonics, pinch, hopo, tremolo, slides, bends, tapping, vibrato, slappop, fifths_and_octaves, sustains, is_lead, is_rhythm, is_bass);
	(void) pack_fputs(buffer, fp);

	//Write the phrases and do other setup common to both Rocksmith exports
	originalbeatcount = sp->beats;	//Store the original beat count
	if(!eof_rs_export_common(sp, track, fp, user_warned))
	{	//If there was an error adding temporary phrases, sections, beats tot he project and writing the phrases to file
		eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view if applicable
		(void) pack_fclose(fp);
		return 0;	//Return error
	}

	//Write some unknown information
	(void) pack_fputs("  <newLinkedDiffs count=\"0\"/>\n", fp);
	(void) pack_fputs("  <linkedDiffs count=\"0\"/>\n", fp);
	(void) pack_fputs("  <phraseProperties count=\"0\"/>\n", fp);

	//Identify chords that follow a note with linkNext status, which will need to be broken into single notes so that they display correctly in-game
	for(ctr = 0; ctr < tp->notes; ctr++)
	{	//For each note in the active pro guitar track
		if(eof_note_count_rs_lanes(sp, track, ctr, 2) > 1)
		{	//If this note would export as a chord
			prevnote = eof_track_fixup_previous_note(sp, track, ctr);	//Identify the previous note in this track difficulty, if there is one
			if(prevnote >= 0)
			{	//If there is a previous note, check to see if it contains linkNext status
				char linked = 0;	//Will be set to nonzero if the previous note had any strings with linkNext status applied (including tech notes)

				for(ctr2 = 0, bitmask = 1; ctr2 < 6; ctr2++, bitmask <<= 1)
				{	//For each of the 6 supported strings
					(void) eof_get_rs_techniques(sp, track, prevnote, ctr2, &tech, 2, 1);	//Obtain all techniques that apply to this string
					if(tech.linknext)
					{	//If the linknext status is applied
						linked = 1;
						break;
					}
				}
				if(linked)
				{	//If the previous note has linkNext status applied to any string
					tp->note[ctr]->tflags |= EOF_NOTE_TFLAG_IGNORE;	//Mark this chord to be ignored by the chord count/export logic and exported as single notes
					for(ctr3 = 0, bitmask = 1; ctr3 < 6; ctr3++, bitmask <<= 1)
					{	//For each of the 6 supported strings
						if((tp->note[ctr]->note & bitmask) && !(tp->note[ctr]->ghost & bitmask))
						{	//If this string is used and is not ghosted
							(void) eof_get_rs_techniques(sp, track, ctr, ctr3, &tech, 2, 1);		//Get the end position of any pitched/unpitched slide this chord's gem has
							new_note = eof_track_add_create_note(sp, track, bitmask, tp->note[ctr]->pos, tp->note[ctr]->length, tp->note[ctr]->type, NULL);	//Initialize a new single note at this position
							if(new_note)
							{	//If the new note was created
								new_note->flags = tp->note[ctr]->flags;					//Clone the flags
								new_note->eflags = tp->note[ctr]->eflags;				//Clone the extended flags
								new_note->tflags |= EOF_NOTE_TFLAG_TEMP;				//Mark the note as temporary
								new_note->bendstrength = tp->note[ctr]->bendstrength;	//Copy the bend strength
								new_note->frets[ctr3] = tp->note[ctr]->frets[ctr3];		//And this string's fret value
								if(tp->note[ctr]->slideend && (tech.slideto >= 0))
								{	//If this note has slide technique and a valid slide end position was found
									new_note->slideend = tech.slideto - tp->capo;		//Apply the correct end position for this gem (removing the capo position which will be reapplied later if in use)
								}
								if(tp->note[ctr]->unpitchend && (tech.unpitchedslideto >= 0))
								{	//If this note has unpitched slide technique and a valid unpitched slide end position was found
									new_note->unpitchend = tech.unpitchedslideto - tp->capo;	//Apply the correct end position for this gem (removing the capo position which will be reapplied later if in use)
								}
							}
							else
							{
								allegro_message("Error:  Couldn't expand linked chords into single notes.  Aborting Rocksmith 2 export.");
								eof_log("Error:  Couldn't expand linked chords into single notes.  Aborting Rocksmith 2 export.", 1);
								eof_rs_export_cleanup(sp, track);	//Remove all temporary notes that were added and remove ignore status from notes
								eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view if applicable
								(void) pack_fclose(fp);
								return 0;	//Return error
							}
						}//If this string is used and is not ghosted
					}//For each of the 6 supported strings
				}//If the previous note has linkNext status applied to any string
			}//If there is a previous note and it has the linkNext status applied to it
		}//If this note would export as a chord
	}//For each note in the active pro guitar track
	eof_track_sort_notes(sp, track);	//Re-sort the notes

	//Identify chords that have the split status.  These will export as single notes instead of as chords.
	for(ctr = 0; ctr < tp->notes; ctr++)
	{	//For each note in the active pro guitar track
		if(eof_note_count_rs_lanes(sp, track, ctr, 2) > 1)
		{	//If this note would export as a chord
			if(tp->note[ctr]->flags & EOF_PRO_GUITAR_NOTE_FLAG_SPLIT)
			{	//If this chord has split status
				for(ctr3 = 0, bitmask = 1; ctr3 < 6; ctr3++, bitmask <<= 1)
				{	//For each of the 6 supported strings
					if((tp->note[ctr]->note & bitmask) && !(tp->note[ctr]->ghost & bitmask))
					{	//If this string is used and is not ghosted
						(void) eof_get_rs_techniques(sp, track, ctr, ctr3, &tech, 2, 1);		//Get the end position of any pitched/unpitched slide this chord's gem has
						new_note = eof_copy_note(sp, track, ctr, track, tp->note[ctr]->pos, tp->note[ctr]->length, tp->note[ctr]->type);	//Clone the note with the updated length
						if(new_note)
						{	//If the new note was created
							new_note->tflags |= EOF_NOTE_TFLAG_TEMP;		//Mark the note as temporary
							new_note->note = bitmask;						//Turn the cloned chord into a single note on the appropriate string
							if(tp->note[ctr]->slideend && (tech.slideto >= 0))
							{	//If this note has slide technique and a valid slide end position was found
								new_note->slideend = tech.slideto - tp->capo;		//Apply the correct end position for this gem (removing the capo position which will be reapplied later if in use)
							}
							if(tp->note[ctr]->unpitchend && (tech.unpitchedslideto >= 0))
							{	//If this note has unpitched slide technique and a valid unpitched slide end position was found
								new_note->unpitchend = tech.unpitchedslideto - tp->capo;	//Apply the correct end position for this gem (removing the capo position which will be reapplied later if in use)
							}
						}
						else
						{
							allegro_message("Error:  Couldn't expand a split status chord into single notes.  Aborting Rocksmith 2 export.");
							eof_log("Error:  Couldn't expand a split status chord into single notes.  Aborting Rocksmith 2 export.", 1);
							eof_rs_export_cleanup(sp, track);	//Remove all temporary notes that were added and remove ignore status from notes
							eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view if applicable
							(void) pack_fclose(fp);
							return 0;	//Return error
						}
					}//If this string is used and is not ghosted
				}//For each of the 6 supported strings
				tp->note[ctr]->tflags |= EOF_NOTE_TFLAG_IGNORE;	//Mark this chord to be ignored by the chord count/export logic as it will be exported as single notes
			}//If this chord has split status
		}//If this note would export as a chord
	}//For each note in the active pro guitar track
	eof_track_sort_notes(sp, track);	//Re-sort the notes

	//Identify gems within chords that will combine with adjacent single notes due to linknext status during chordnote export
	//Mark single notes as ignored where appropriate
	for(ctr3 = 0; ctr3 < tp->notes; ctr3++)
	{	//For each note in the track
		if(eof_is_partially_ghosted(sp, track, ctr3) && (tp->note[ctr3]->tflags & EOF_NOTE_TFLAG_TWIN))
		{	//If the chord is partially ghosted and has this flag set, it has a clone without ghost gems that should instead be used for chord tag export
			continue;
		}
		if(eof_note_count_rs_lanes(sp, track, ctr3, 2) > 1)
		{	//If this note will export as a chord (at least two non ghosted gems)
			if(!(tp->note[ctr3]->tflags & EOF_NOTE_TFLAG_IGNORE) || (tp->note[ctr3]->tflags & EOF_NOTE_TFLAG_COMBINE))
			{	//If this chord wasn't split into single notes or converted into a non ghosted chord and is being ignored, or if it is marked for combination
				for(stringnum = 0, bitmask = 1; stringnum < tp->numstrings; stringnum++, bitmask <<= 1)
				{	//For each string used in this track, write chordNote tags
					if((eof_get_note_note(sp, track, ctr3) & bitmask) && !(tp->note[ctr3]->ghost & bitmask))
					{	//If this string is used in this note and it is not ghosted
						long originallength = tp->note[ctr3]->length;	//Back up the original length of this note because it may be altered before export

						(void) eof_rs_combine_linknext_logic(sp, track, ctr3, stringnum);
						tp->note[ctr3]->length = originallength;	//Restore the original length to the chord
					}
				}
			}
		}
	}

	//Identify notes that are inside handshape phrases, handshapes for which will be treated differently than arpeggio phrases
	for(ctr = 0; ctr < tp->notes; ctr++)
	{	//For each note in the active pro guitar track
		for(ctr2 = 0; ctr2 < tp->arpeggios; ctr2++)
		{	//For each arpeggio section in the track
			if((tp->note[ctr]->pos >= tp->arpeggio[ctr2].start_pos) && (tp->note[ctr]->pos <= tp->arpeggio[ctr2].end_pos) && (tp->note[ctr]->type == tp->arpeggio[ctr2].difficulty))
			{	//If the note is within the arpeggio phrase
				if(tp->arpeggio[ctr2].flags & EOF_RS_ARP_HANDSHAPE)
				{	//If this arpeggio is specified as exporting as a normal handshape instead of an arpeggio one
					tp->note[ctr]->tflags |= EOF_NOTE_TFLAG_HAND;	//Mark this chord as being in a handshape phrase
				}
			}
		}
	}

	//Identify notes that are inside arpeggio/handshape phrases
	//Those which are chords inside arpeggio phrases will need to be broken into single notes so that they display correctly in-game
	for(ctr2 = 0; ctr2 < tp->arpeggios; ctr2++)
	{	//For each arpeggio phrase in the track
		unsigned long tflags = EOF_NOTE_TFLAG_ARP;	//The first note in each arpeggio phrase gets both of these flags, the other notes in the phrase just get the latter
		unsigned long tflags2 = EOF_NOTE_TFLAG_ARP_FIRST;	//The first chord in each arpeggio gets this flag set

		for(ctr = 0; ctr < tp->notes; ctr++)
		{	//For each note in the active pro guitar track
			if((tp->note[ctr]->pos >= tp->arpeggio[ctr2].start_pos) && (tp->note[ctr]->pos <= tp->arpeggio[ctr2].end_pos) && (tp->note[ctr]->type == tp->arpeggio[ctr2].difficulty))
			{	//If the note is within the arpeggio/handshape phrase
				tp->note[ctr]->tflags |= tflags;	//Mark this note as being in an arpeggio phrase
				if(eof_note_count_rs_lanes(sp, track, ctr, 2) > 1)
				{	//If this note would export as a chord
					tp->note[ctr]->tflags |= tflags2;		//Mark this note as being the first in an arpeggio phrase if applicable
					tflags2 &= ~EOF_NOTE_TFLAG_ARP_FIRST;	//Clear this flag so that chords other than the first one in this phrase don't receive it
				}
				if(!(tp->note[ctr]->tflags & EOF_NOTE_TFLAG_IGNORE))
				{	//If this note isn't already ignored
					if(!(tp->arpeggio[ctr2].flags & EOF_RS_ARP_HANDSHAPE))
					{	//If this is NOT a handshape phrase
						if(eof_note_count_rs_lanes(sp, track, ctr, 2) > 1)
						{	//If this note would export as a chord
							if(!(tp->arpeggio[ctr2].flags & EOF_RS_ARP_HANDSHAPE))
							{	//If this was an arpeggio phrase instead of a handshape phrase
								tp->note[ctr]->tflags |= EOF_NOTE_TFLAG_IGNORE;	//Mark this chord to be ignored by the chord count/export logic and exported as single notes
							}
							for(ctr3 = 0, bitmask = 1; ctr3 < 6; ctr3++, bitmask <<= 1)
							{	//For each of the 6 supported strings
								if((tp->note[ctr]->note & bitmask) && !(tp->note[ctr]->ghost & bitmask))
								{	//If this string is used and is not ghosted
									long originallength = tp->note[ctr]->length;	//Back up the original length of this note because it may be altered by eof_rs_combine_linknext_logic()
									int linknextoff;

									linknextoff = eof_rs_combine_linknext_logic(sp, track, ctr, ctr3);	//Run combination logic to set the chord's length to that which this string's chordnote will export, if applicable
									new_note = eof_copy_note(sp, track, ctr, track, tp->note[ctr]->pos, tp->note[ctr]->length, tp->note[ctr]->type);	//Clone the note with the updated length
									tp->note[ctr]->length = originallength;	//Restore the chord's original length if it had changed
									if(new_note)
									{	//If the new note was created
										new_note->tflags |= EOF_NOTE_TFLAG_TEMP;				//Mark the note as temporary
										new_note->note = bitmask;								//Turn the cloned chord into a single note on the appropriate string
										if(linknextoff)
										{	//If the chordnote corresponding to this single note will have its linknext status forced off during export
											new_note->flags &= ~EOF_PRO_GUITAR_NOTE_FLAG_LINKNEXT;	//This single note should export the same way
											new_note->tflags |= EOF_NOTE_TFLAG_NO_LN;			//Explicitly signal to export logic that this single note will not export with linknext, even due to tech notes
										}
									}
									else
									{
										allegro_message("Error:  Couldn't expand an arpeggio chord into single notes.  Aborting Rocksmith 2 export.");
										eof_log("Error:  Couldn't expand an arpeggio chord into single notes.  Aborting Rocksmith 2 export.", 1);
										eof_rs_export_cleanup(sp, track);	//Remove all temporary notes that were added and remove ignore status from notes
										eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view if applicable
										(void) pack_fclose(fp);
										return 0;	//Return error
									}
								}//If this string is used and is not ghosted
							}//For each of the 6 supported strings
						}//If this note would export as a chord
					}//If this is NOT a handshape phrase
				}//If this note isn't already ignored
			}//If the note is within the arpeggio/handshape phrase
		}//For each note in the active pro guitar track
	}//For each arpeggio/handshape section in the track
	eof_track_sort_notes(sp, track);	//Re-sort the notes

	//Identify partially ghosted chords, those outside arpeggio phrases will need to be temporarily replaced with variations of the chords without the ghost notes
	//Those inside arpeggio phrases will be copied without ghost notes.  The original's chord template will be used for handshape tag export and the copy will be used for chord tag export
	for(ctr = 0; ctr < tp->notes; ctr++)
	{	//For each note in the active pro guitar track
		if((eof_note_count_rs_lanes(sp, track, ctr, 4) > 1) && !(tp->note[ctr]->tflags & EOF_NOTE_TFLAG_IGNORE))
		{	//If this note contains multiple gems and isn't already ignored
			if(eof_is_partially_ghosted(sp, track, ctr))
			{	//If it is partially ghosted
				char failed = 0;
				EOF_PRO_GUITAR_NOTE *new_note2;
				for(ctr2 = 0, match = 0; ctr2 < tp->arpeggios; ctr2++)
				{	//For each arpeggio section in the track
					if((tp->note[ctr]->pos >= tp->arpeggio[ctr2].start_pos) && (tp->note[ctr]->pos <= tp->arpeggio[ctr2].end_pos) && (tp->note[ctr]->type == tp->arpeggio[ctr2].difficulty))
					{	//If the note is within the arpeggio phrase
						match = 1;	//Track that it was inside an arpeggio phrase
						break;	//Exit inner for loop
					}
				}
				if(!match)
				{	//If the partially ghosted chord wasn't in an arpeggio phrase, mark it as ignored
					tp->note[ctr]->tflags |= EOF_NOTE_TFLAG_IGNORE;	//Mark this chord to be ignored by the chord count/export logic and exported as the newly built chord below
				}
				//Insert a temporary chord that is the same except without the ghost gems
				new_note = eof_copy_note(sp, track, ctr, track, tp->note[ctr]->pos, tp->note[ctr]->length, tp->note[ctr]->type);
				if(new_note)
				{	//If the new note was created
					new_note->tflags |= EOF_NOTE_TFLAG_TEMP;				//Mark the note as temporary
					new_note->ghost = 0;									//No ghost gems will be retained by this temporary note
					new_note->note = 0;										//Clear the new note's note mask, it will be rebuilt below
					for(ctr3 = 0, bitmask = 1; ctr3 < 6; ctr3++, bitmask <<= 1)
					{	//For each of the 6 supported strings
						if((tp->note[ctr]->note & bitmask) && !(tp->note[ctr]->ghost & bitmask))
						{	//If this string is used and not ghosted
							new_note->note |= bitmask;	//Set this bit in the mask
						}
					}
					if(match)
					{	//If the partially ghosted chord was inside an arpeggio phrase, denote that the original and clone are both involved with this cloning process
						tp->note[ctr]->tflags |= EOF_NOTE_TFLAG_TWIN;	//The original (includes ghost gems) is NOT allowed to be have its chord tag export
						new_note->tflags |= EOF_NOTE_TFLAG_TWIN;		//The clone (does not include ghost gems) is NOT allowed to have its chord template invoked during handshape tag export
					}
					if(tp->note[ctr]->eflags & EOF_PRO_GUITAR_NOTE_EFLAG_GHOST_HS)
					{	//If the chord is marked with ghost handshape status, create a copy of the chord with all ghosted gems as normal gems
						new_note2 = eof_copy_note(sp, track, ctr, track, tp->note[ctr]->pos, tp->note[ctr]->length, tp->note[ctr]->type);
						if(new_note2)
						{	//If the new note was created
							new_note2->ghost = 0;							//Clear all ghost status from this chord
							new_note2->tflags |= EOF_NOTE_TFLAG_TEMP;		//Mark the note as temporary
							new_note2->tflags |= EOF_NOTE_TFLAG_IGNORE;		//Mark it as ignored, it will only be used in the unique chord building logic and handshape tag export
							new_note2->tflags |= EOF_NOTE_TFLAG_GHOST_HS;	//Mark that the note will only be partially used during the export
						}
						else
						{
							failed = 1;
						}
					}
				}
				else
				{
					failed = 1;
				}
				if(failed)
				{
					allegro_message("Error:  Couldn't expand a non arpeggio partially ghosted chord into non ghosted chord(s).  Aborting Rocksmith 2 export.");
					eof_log("Error:  Couldn't expand a non arpeggio partially ghosted chord into non ghosted chord(s).  Aborting Rocksmith 2 export.", 1);
					eof_rs_export_cleanup(sp, track);	//Remove all temporary notes that were added and remove ignore status from notes
					eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view if applicable
					(void) pack_fclose(fp);
					return 0;	//Return error
				}
			}//If it is partially ghosted
		}//If this note contains multiple gems and isn't already ignored
	}
	eof_track_sort_notes(sp, track);	//Re-sort the notes

	//Write chord templates
	chordlistsize = eof_build_chord_list(sp, track, &chordlist, 2);	//Build a list of all unique chords in the track
	if(!chordlistsize)
	{	//If there were no chords, write an empty chord template tag
		(void) pack_fputs("  <chordTemplates count=\"0\"/>\n", fp);
	}
	else
	{	//There were chords
		long fret0 = 0, fret1 = 0, fret2 = 0, fret3 = 0, fret4 = 0, fret5 = 0;	//Will store the fret number played on each string (-1 means the string is not played)
		long *fret[6] = {&fret0, &fret1, &fret2, &fret3, &fret4, &fret5};	//Allow the fret numbers to be accessed via array
		char *fingerunused = "-1";
		char *finger0 = NULL, *finger1 = NULL, *finger2 = NULL, *finger3 = NULL, *finger4 = NULL, *finger5 = NULL;	//Each will be set to either fingerunknown or fingerunused
		char **finger[6] = {&finger0, &finger1, &finger2, &finger3, &finger4, &finger5};	//Allow the finger strings to be accessed via array
		char finger0def[2] = "0", finger1def[2] = "1", finger2def[2] = "2", finger3def[2] = "3", finger4def[2] = "4", finger5def[2] = "5";	//Static strings for building manually-defined finger information
		char *fingerdef[6] = {finger0def, finger1def, finger2def, finger3def, finger4def, finger5def};	//Allow the fingerdef strings to be accessed via array
		unsigned long shapenum = 0;
		EOF_PRO_GUITAR_NOTE temp = {{0}, 0, 0, 0, {0}, {0}, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};	//Will have a matching chord shape definition's fingering applied to
		unsigned char *effective_fingering;	//Will point to either a note's own finger array or one of that of the temp pro guitar note structure above
		char arp[] = "-arp", no_arp[] = "";	//The suffix applied to the chord template's display name, depending on whether the template is for an arpeggio
		char *suffix;	//Will point to either arp[] or no_arp[]

		(void) snprintf(buffer, sizeof(buffer) - 1, "  <chordTemplates count=\"%lu\">\n", chordlistsize);
		(void) pack_fputs(buffer, fp);
		for(ctr = 0; ctr < chordlistsize; ctr++)
		{	//For each of the entries in the unique chord list
			notename[0] = '\0';	//Empty the note name string
			assert(chordlist != NULL);	//Unneeded check to resolve a false positive in Splint
			(void) eof_build_note_name(sp, track, chordlist[ctr], notename);	//Build the note name (if it exists) into notename[]

			effective_fingering = tp->note[chordlist[ctr]]->finger;	//By default, use the chord list entry's finger array
			memcpy(temp.frets, tp->note[chordlist[ctr]]->frets, 6);	//Clone the fretting of the chord into the temporary note
			temp.note = tp->note[chordlist[ctr]]->note;				//Clone the note mask
			if(eof_pro_guitar_note_fingering_valid(tp, chordlist[ctr], 0) != 1)
			{	//If the fingering for the note is not fully defined
				if(eof_lookup_chord_shape(tp->note[chordlist[ctr]], &shapenum, 0))
				{	//If a fingering for the chord can be found in the chord shape definitions
					eof_apply_chord_shape_definition(&temp, shapenum);	//Apply the matching chord shape definition's fingering
					effective_fingering = temp.finger;	//Use the matching chord shape definition's finger definitions
				}
			}

			for(ctr2 = 0, bitmask = 1; ctr2 < 6; ctr2++, bitmask <<= 1)
			{	//For each of the 6 supported strings
				if((eof_get_note_note(sp, track, chordlist[ctr]) & bitmask) && (ctr2 < tp->numstrings))
				{	//If the chord list entry uses this string (verifying that the string number is supported by the track)
					if(tp->note[chordlist[ctr]]->frets[ctr2] == 0xFF)
					{	//If this is a string mute with no defined fret number
						*(fret[ctr2]) = 0;	//Assume muted open note
					}
					else
					{	//Otherwise use the defined fret number
						*(fret[ctr2]) = tp->note[chordlist[ctr]]->frets[ctr2] & 0x7F;	//Retrieve the fret played on this string (masking out the muting bit)
					}
					if(*(fret[ctr2]))
					{	//If this string isn't played open
						*(fret[ctr2]) += tp->capo;	//Apply the capo position
					}
					if(effective_fingering[ctr2])
					{	//If the fingering for this string is defined
						char *str = fingerdef[ctr2];	//Simplify logic below
						str[0] = '0' + effective_fingering[ctr2];	//Convert decimal to ASCII
						str[1] = '\0';	//Truncate string
						if(str[0] == '5')
						{	//If this fingering specifies the thumb
							str[0] = '0';	//Convert from EOF's numbering (5 = thumb) to Rocksmith's numbering (0 = thumb)
						}
						*(finger[ctr2]) = str;
					}
					else
					{	//The fingering is not defined, regardless of whether the string is open or fretted
						*(finger[ctr2]) = fingerunused;		//Write a -1, this will allow the XML to compile even if the chord's fingering is incomplete/undefined
					}
				}
				else
				{	//The chord list entry does not use this string
					*(fret[ctr2]) = -1;
					*(finger[ctr2]) = fingerunused;
				}
			}

			if(eof_is_partially_ghosted(sp, track, chordlist[ctr]))
			{	//If the chord list entry is partially ghosted (for arpeggio or handshape notation)
				if(!(tp->note[chordlist[ctr]]->tflags & EOF_NOTE_TFLAG_HAND))
				{	//If the chord is in an arpeggio phrase instead of a handshape phrase
					suffix = arp;	//Apply the "-arp" suffix to the chord template's display name
				}
				else
				{	//Otherwise the chord is in a handshape phrase
					suffix = no_arp;
				}
			}
			else
			{
				suffix = no_arp;	//This chord template is not for an arpeggio chord, apply no suffix
			}
			expand_xml_text(buffer2, sizeof(buffer2) - 1, notename, 32 - 4, 1);	//Expand XML special characters into escaped sequences if necessary, and check against the maximum supported length of this field (reserve 4 characters for the "-arp" suffix).  Filter out characters suspected of causing the game to crash (allow forward slash).
			(void) snprintf(buffer, sizeof(buffer) - 1, "    <chordTemplate chordName=\"%s\" displayName=\"%s%s\" finger0=\"%s\" finger1=\"%s\" finger2=\"%s\" finger3=\"%s\" finger4=\"%s\" finger5=\"%s\" fret0=\"%ld\" fret1=\"%ld\" fret2=\"%ld\" fret3=\"%ld\" fret4=\"%ld\" fret5=\"%ld\"/>\n", buffer2, buffer2, suffix, finger0, finger1, finger2, finger3, finger4, finger5, fret0, fret1, fret2, fret3, fret4, fret5);
			(void) pack_fputs(buffer, fp);
		}//For each of the entries in the unique chord list
		(void) pack_fputs("  </chordTemplates>\n", fp);
	}//There were chords

	//Write some unknown information
	(void) pack_fputs("  <fretHandMuteTemplates count=\"0\"/>\n", fp);

	//Write the beat timings
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <ebeats count=\"%lu\">\n", sp->beats);
	(void) pack_fputs(buffer, fp);
	for(ctr = 0; ctr < sp->beats; ctr++)
	{	//For each beat in the chart
		if(eof_get_ts(sp,&beatspermeasure,NULL,ctr) == 1)
		{	//If this beat has a time signature change
			beatcounter = 0;
		}
		if(!beatcounter)
		{	//If this is the first beat in a measure
			measurenum++;
			displayedmeasure = measurenum;
		}
		else
		{	//Otherwise the measure is displayed as -1 to indicate no change from the previous beat's measure number
			displayedmeasure = -1;
		}
		(void) snprintf(buffer, sizeof(buffer) - 1, "    <ebeat time=\"%.3f\" measure=\"%ld\"/>\n", sp->beat[ctr]->fpos / 1000.0, displayedmeasure);
		(void) pack_fputs(buffer, fp);
		beatcounter++;
		if(beatcounter >= beatspermeasure)
		{
			beatcounter = 0;
		}
	}
	(void) pack_fputs("  </ebeats>\n", fp);

	//Restore the original number of beats in the project in case any were added for DDC
	(void) eof_song_resize_beats(sp, originalbeatcount);

	//Write tone changes
	//Build and count the size of the list of unique tone names used, and empty the default tone string if it is not valid
	eof_track_pro_guitar_sort_tone_changes(tp);	//Re-sort the tone changes
	eof_track_rebuild_rs_tone_names_list_strings(track, 0);	//Build the tone names list without the (D) default tone name suffix
	if((tp->defaulttone[0] != '\0') && (eof_track_rs_tone_names_list_strings_num > 1))
	{	//If the default tone is valid and at least two different tone names are referenced among the tone changes
		unsigned long tonecount = 0;
		char *temp;
		char *effective_tone;	//The last tone exported

		//Make sure the default tone is placed at the beginning of the eof_track_rs_tone_names_list_strings[] list
		for(ctr = 1; ctr < eof_track_rs_tone_names_list_strings_num; ctr++)
		{	//For each unique tone name after the first
			if(!strcmp(eof_track_rs_tone_names_list_strings[ctr], tp->defaulttone))
			{	//If this is the track's default tone
				temp = eof_track_rs_tone_names_list_strings[0];	//Store the pointer to the first tone name in the list
				eof_track_rs_tone_names_list_strings[0] = eof_track_rs_tone_names_list_strings[ctr];	//Replace it with this one, which is the default tone
				eof_track_rs_tone_names_list_strings[ctr] = temp;	//And swap the previously first tone name in its place
				break;
			}
		}

		//Write the declarations for the default tone and the first two tones
		(void) snprintf(buffer, sizeof(buffer) - 1, "  <tonebase>%s</tonebase>\n", eof_track_rs_tone_names_list_strings[0]);
		(void) pack_fputs(buffer, fp);
		(void) snprintf(buffer, sizeof(buffer) - 1, "  <tonea>%s</tonea>\n", eof_track_rs_tone_names_list_strings[0]);
		(void) pack_fputs(buffer, fp);
		(void) snprintf(buffer, sizeof(buffer) - 1, "  <toneb>%s</toneb>\n", eof_track_rs_tone_names_list_strings[1]);
		(void) pack_fputs(buffer, fp);

		//Write the third tone declaration if applicable
		if(eof_track_rs_tone_names_list_strings_num > 2)
		{	//If there is a third tone name
			(void) snprintf(buffer, sizeof(buffer) - 1, "  <tonec>%s</tonec>\n", eof_track_rs_tone_names_list_strings[2]);
			(void) pack_fputs(buffer, fp);
		}

		//Write the fourth tone declaration if applicable
		if(eof_track_rs_tone_names_list_strings_num > 3)
		{	//If there is a fourth tone name
			(void) snprintf(buffer, sizeof(buffer) - 1, "  <toned>%s</toned>\n", eof_track_rs_tone_names_list_strings[3]);
			(void) pack_fputs(buffer, fp);
		}

		//Count how many tone changes are valid to export
		effective_tone = tp->defaulttone;	//The default tone is automatically in effect at the start of the track
		for(ctr = 0; ctr < tp->tonechanges; ctr++)
		{	//For each tone change in the track
			for(ctr2 = 0; (ctr2 < eof_track_rs_tone_names_list_strings_num) && (ctr2 < 4); ctr2++)
			{	//For the first four unique tone names
				if(!strcmp(tp->tonechange[ctr].name, eof_track_rs_tone_names_list_strings[ctr2]))
				{	//If the tone change applies one of the four valid tone names
					if(strcmp(tp->tonechange[ctr].name, effective_tone))
					{	//If the tone being changed to isn't already in effect
						tonecount++;
						effective_tone = tp->tonechange[ctr].name;	//Track the tone that is in effect
					}
					break;	//Break from inner loop
				}
			}
		}

		//Write the tone changes that are valid to export
		(void) snprintf(buffer, sizeof(buffer) - 1, "  <tones count=\"%lu\">\n", tonecount);
		(void) pack_fputs(buffer, fp);
		effective_tone = tp->defaulttone;	//The default tone is automatically in effect at the start of the track
		for(ctr = 0; ctr < tp->tonechanges; ctr++)
		{	//For each tone change in the track
			for(ctr2 = 0; (ctr2 < eof_track_rs_tone_names_list_strings_num) && (ctr2 < 4); ctr2++)
			{	//For the first four unique tone names
				if(!strcmp(tp->tonechange[ctr].name, eof_track_rs_tone_names_list_strings[ctr2]))
				{	//If the tone change applies one of the four valid tone names
					if(strcmp(tp->tonechange[ctr].name, effective_tone))
					{	//If the tone being changed to isn't already in effect
						(void) snprintf(buffer, sizeof(buffer) - 1, "    <tone time=\"%.3f\" name=\"%s\"/>\n", tp->tonechange[ctr].start_pos / 1000.0, tp->tonechange[ctr].name);
						(void) pack_fputs(buffer, fp);
						effective_tone = tp->tonechange[ctr].name;	//Track the tone that is in effect
					}
					break;	//Break from inner loop
				}
			}
		}
		(void) pack_fputs("  </tones>\n", fp);
	}//If the default tone is valid and at least two different tone names are referenced among the tone changes
	eof_track_destroy_rs_tone_names_list_strings();

	//Write sections
	for(ctr = 0, numsections = 0; ctr < sp->beats; ctr++)
	{	//For each beat in the chart
		if(sp->beat[ctr]->contained_rs_section_event >= 0)
		{	//If this beat has a Rocksmith section
			numsections++;	//Update Rocksmith section instance counter
		}
	}
	if(numsections)
	{	//If there is at least one Rocksmith section defined in the chart (which should be the case since default ones were inserted earlier if there weren't any)
		(void) snprintf(buffer, sizeof(buffer) - 1, "  <sections count=\"%lu\">\n", numsections);
		(void) pack_fputs(buffer, fp);
		for(ctr = 0; ctr < sp->beats; ctr++)
		{	//For each beat in the chart
			if(sp->beat[ctr]->contained_rs_section_event >= 0)
			{	//If this beat has a Rocksmith section
				expand_xml_text(buffer2, sizeof(buffer2) - 1, sp->text_event[sp->beat[ctr]->contained_rs_section_event]->text, 32, 0);	//Expand XML special characters into escaped sequences if necessary, and check against the maximum supported length of this field
				(void) snprintf(buffer, sizeof(buffer) - 1, "    <section name=\"%s\" number=\"%d\" startTime=\"%.3f\"/>\n", buffer2, sp->beat[ctr]->contained_rs_section_event_instance_number, sp->beat[ctr]->fpos / 1000.0);
				(void) pack_fputs(buffer, fp);
			}
		}
		(void) pack_fputs("  </sections>\n", fp);
	}
	else
	{
		allegro_message("Error:  Default RS sections that were added are missing.  Skipping writing the <sections> tag.");
		eof_log("Error:  Default RS sections that were added are missing.  Skipping writing the <sections> tag.", 1);
	}

	//Add temporary events for time signature changes
	for(ctr = 0; ctr < sp->beats; ctr++)
	{	//For each beat
		char buffer3[16];
		char buffer4[30];

		if(eof_song->beat[ctr]->contains_ts_change && eof_get_ts_text(ctr, buffer3))
		{	//If this beat has a time signature defined
			(void) uszprintf(buffer4, (int) sizeof(buffer4), "TS:%s", buffer3);		//Build a string to mark this change
			(void) eof_song_add_text_event(sp, ctr, buffer4, track, EOF_EVENT_FLAG_RS_EVENT, 1);		//Add it as a temporary event at the change's beat number
		}
	}
	eof_sort_events(sp);	//Re-sort

	//Write events
	for(ctr = 0, numevents = 0; ctr < sp->text_events; ctr++)
	{	//For each event in the chart
		if(sp->text_event[ctr]->flags & EOF_EVENT_FLAG_RS_EVENT)
		{	//If the event is marked as a Rocksmith event
			if(!sp->text_event[ctr]->track || (sp->text_event[ctr]->track  == track))
			{	//If the event applies to the specified track
				numevents++;
			}
		}
	}
	if(numevents)
	{	//If there is at least one Rocksmith event defined in the chart
		(void) snprintf(buffer, sizeof(buffer) - 1, "  <events count=\"%lu\">\n", numevents);
		(void) pack_fputs(buffer, fp);
		for(ctr = 0, numevents = 0; ctr < sp->text_events; ctr++)
		{	//For each event in the chart
			if(sp->text_event[ctr]->flags & EOF_EVENT_FLAG_RS_EVENT)
			{	//If the event is marked as a Rocksmith event
				if(!sp->text_event[ctr]->track || (sp->text_event[ctr]->track  == track))
				{	//If the event applies to the specified track
					expand_xml_text(buffer2, sizeof(buffer2) - 1, sp->text_event[ctr]->text, 256, 0);	//Expand XML special characters into escaped sequences if necessary, and check against the maximum supported length of this field
					(void) snprintf(buffer, sizeof(buffer) - 1, "    <event time=\"%.3f\" code=\"%s\"/>\n", sp->beat[sp->text_event[ctr]->beat]->fpos / 1000.0, buffer2);
					(void) pack_fputs(buffer, fp);
				}
			}
		}
		(void) pack_fputs("  </events>\n", fp);
	}
	else
	{	//Otherwise write an empty events tag
		(void) pack_fputs("  <events count=\"0\"/>\n", fp);
	}

	//Remove all temporary text events that were added for time signatures
	for(ctr = sp->text_events; ctr > 0; ctr--)
	{	//For each text event (in reverse order)
		if(sp->text_event[ctr-1]->is_temporary)
		{	//If this text event has been marked as temporary
			eof_song_delete_text_event(sp, ctr-1);	//Delete it
		}
	}
	eof_sort_events(sp);	//Re-sort

	//Write some unknown information
	(void) pack_fputs("  <transcriptionTrack difficulty=\"-1\">\n", fp);
	(void) pack_fputs("      <notes count=\"0\"/>\n", fp);
	(void) pack_fputs("      <chords count=\"0\"/>\n", fp);
	(void) pack_fputs("      <anchors count=\"0\"/>\n", fp);
	(void) pack_fputs("      <handShapes count=\"0\"/>\n", fp);
	(void) pack_fputs("  </transcriptionTrack>\n", fp);

	//Write note difficulties
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <levels count=\"%u\">\n", numdifficulties);
	(void) pack_fputs(buffer, fp);
	for(ctr = 0, ctr2 = 0; ctr < 256; ctr++)
	{	//For each of the possible difficulties
		if(eof_track_diff_populated_status[ctr])
		{	//If this difficulty is populated
			unsigned long anchorcount;
			char anchorsgenerated = 0;	//Tracks whether anchors were automatically generated and will need to be deleted after export

			//Count the number of single notes and chords in this difficulty
			for(ctr3 = 0, numsinglenotes = 0, numchords = 0; ctr3 < tp->notes; ctr3++)
			{	//For each note in the track
				if((eof_get_note_type(sp, track, ctr3) == ctr) && !(tp->note[ctr3]->tflags & EOF_NOTE_TFLAG_IGNORE))
				{	//If the note is in this difficulty and isn't being ignored
					unsigned long lanecount;
					if(eof_is_partially_ghosted(sp, track, ctr3) && (tp->note[ctr3]->tflags & EOF_NOTE_TFLAG_TWIN))
					{	//If the chord is partially ghosted and has this flag set, it has a clone without ghost gems that should instead be used for chord tag export
						continue;
					}
					lanecount = eof_note_count_rs_lanes(sp, track, ctr3, 2);	//Count the number of non ghosted gems for this note
					if(lanecount == 1)
					{	//If the note has only one gem
						numsinglenotes++;	//Increment counter
					}
					else if(lanecount > 1)
					{	//If the note has multiple gems
						numchords++;	//Increment counter
					}
				}
			}

			//Write single notes
			(void) snprintf(buffer, sizeof(buffer) - 1, "    <level difficulty=\"%lu\">\n", ctr2);
			(void) pack_fputs(buffer, fp);
			ctr2++;	//Increment the populated difficulty level number
			if(numsinglenotes)
			{	//If there's at least one single note in this difficulty
				(void) snprintf(buffer, sizeof(buffer) - 1, "      <notes count=\"%lu\">\n", numsinglenotes);
				(void) pack_fputs(buffer, fp);
				for(ctr3 = 0; ctr3 < tp->notes; ctr3++)
				{	//For each note in the track
					if(eof_is_partially_ghosted(sp, track, ctr3) && (tp->note[ctr3]->tflags & EOF_NOTE_TFLAG_TWIN))
					{	//If the chord is partially ghosted and has this flag set, it has a clone without ghost gems that should instead be used for chord tag export
						continue;
					}
					if(!(tp->note[ctr3]->tflags & EOF_NOTE_TFLAG_IGNORE) && (eof_get_note_type(sp, track, ctr3) == ctr) && (eof_note_count_rs_lanes(sp, track, ctr3, 2) == 1))
					{	//If this note is not ignored, is in this difficulty and will export as a single note (only one gem has non ghosted status)
						for(stringnum = 0, bitmask = 1; stringnum < tp->numstrings; stringnum++, bitmask <<= 1)
						{	//For each string used in this track
							if((eof_get_note_note(sp, track, ctr3) & bitmask) && !(tp->note[ctr3]->ghost & bitmask))
							{	//If this string is used in this note and it is not ghosted
								long originallength = tp->note[ctr3]->length;	//Back up the original length of this note because it may be altered before export

								(void) eof_rs_combine_linknext_logic(sp, track, ctr3, stringnum);
								eof_rs2_export_note_string_to_xml(sp, track, ctr3, stringnum, 0, 0, fp);	//Write this note's XML tag
								tp->note[ctr3]->length = originallength;	//Restore the original length to the note
								break;	//Only one note entry is valid for each single note, so break from loop
							}//If this string is used in this note and it is not ghosted
						}//For each string used in this track
					}//If this note is not ignored, is in this difficulty and will export as a single note (only one gem has non ghosted status)
				}//For each note in the track
				(void) pack_fputs("      </notes>\n", fp);
			}//If there's at least one single note in this difficulty
			else
			{	//There are no single notes in this difficulty, write an empty notes tag
				(void) pack_fputs("      <notes count=\"0\"/>\n", fp);
			}

			//Write chords
			if(numchords)
			{	//If there's at least one chord in this difficulty
				unsigned long flags;
				unsigned long lastchordid = 0;	//Stores the previous written chord's ID, so that when the ID changes, chordNote subtags can be forced to be written
				char *upstrum = "up";
				char *downstrum = "down";
				char *direction;		//Will point to either upstrum or downstrum as appropriate
				unsigned long notepos;
				char highdensity;		//Various criteria determine whether the highDensity boolean property is set to true

				(void) snprintf(buffer, sizeof(buffer) - 1, "      <chords count=\"%lu\">\n", numchords);
				(void) pack_fputs(buffer, fp);
				for(ctr3 = 0; ctr3 < tp->notes; ctr3++)
				{	//For each note in the track
					if(eof_is_partially_ghosted(sp, track, ctr3) && (tp->note[ctr3]->tflags & EOF_NOTE_TFLAG_TWIN))
					{	//If the chord is partially ghosted and has this flag set, it has a clone without ghost gems that should instead be used for chord tag export
						continue;
					}
					if((eof_get_note_type(sp, track, ctr3) == ctr) && (eof_note_count_rs_lanes(sp, track, ctr3, 2) > 1))
					{	//If this note is in this difficulty and will export as a chord (at least two non ghosted gems)
						if(!(tp->note[ctr3]->tflags & EOF_NOTE_TFLAG_IGNORE) || (tp->note[ctr3]->tflags & EOF_NOTE_TFLAG_COMBINE))
						{	//If this chord wasn't split into single notes or converted into a non ghosted chord and is being ignored, or if it is marked for combination
							for(ctr4 = 0; ctr4 < chordlistsize; ctr4++)
							{	//For each of the entries in the unique chord list
								assert(chordlist != NULL);	//Unneeded check to resolve a false positive in Splint
								if(!eof_note_compare_simple(sp, track, ctr3, chordlist[ctr4]))
								{	//If this note matches a chord list entry
									if(!eof_pro_guitar_note_compare_fingerings(tp->note[ctr3], tp->note[chordlist[ctr4]]))
									{	//If this note has identical fingering to chord list entry
										if(!strcmp(tp->note[ctr3]->name, tp->note[chordlist[ctr4]]->name))
										{	//If the chord names match
											chordid = ctr4;	//Store the chord list entry number
											break;
										}
									}
								}
							}
							if(ctr4 >= chordlistsize)
							{	//If the chord couldn't be found
								eof_seek_and_render_position(track, tp->note[ctr3]->type, tp->note[ctr3]->pos);
								allegro_message("Error:  Couldn't match chord with chord template while exporting chords.  Aborting Rocksmith 2 export.");
								eof_log("Error:  Couldn't match chord with chord template while exporting chords.  Aborting Rocksmith 2 export.", 1);
								if(chordlist)
								{	//If the chord list was built
									free(chordlist);
								}
								eof_rs_export_cleanup(sp, track);	//Remove all temporary notes that were added and remove ignore status from notes
								eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view if applicable
								(void) pack_fclose(fp);
								return 0;	//Return error
							}
							flags = tp->note[ctr3]->flags;	//Simplify
							if(flags & EOF_PRO_GUITAR_NOTE_FLAG_UP_STRUM)
							{	//If this note explicitly strums up
								direction = upstrum;	//Set the direction string to match
							}
							else
							{	//Otherwise the direction defaults to down
								direction = downstrum;
							}
							(void) eof_get_rs_techniques(sp, track, ctr3, 0, &tech, 2, 0);			//Determine techniques used by this chord (do not include applicable technote's techniques to the chord tag itself, they will apply to chordNotes instead)
							highdensity = eof_note_has_high_chord_density(sp, track, ctr3, 2);		//Determine whether the chord will export with high density
							notepos = tp->note[ctr3]->pos;
							if(highdensity != 2)
							{	//If the chord isn't fully string muted with no fingering defined and following a chord, allow a chord ID change to mark as low density
								if(chordid != lastchordid)
								{	//If this chord's ID is different from that of the previous chord or meets the normal criteria for a low density chord
									highdensity = 0;	//Ensure the chord tag is written to reflect low density
								}
							}
							if(highdensity > 1)
							{	//Force highdensity to a true/false value
								highdensity = 1;
							}
							if(flags & EOF_PRO_GUITAR_NOTE_FLAG_HD)
							{	//Chord is explicitly specified to be high density
								highdensity = 1;
							}
							(void) snprintf(buffer, sizeof(buffer) - 1, "        <chord time=\"%.3f\" linkNext=\"%d\" accent=\"%d\" chordId=\"%lu\" fretHandMute=\"%d\" highDensity=\"%d\" ignore=\"%d\" palmMute=\"%d\" hopo=\"%d\" strum=\"%s\">\n", (double)notepos / 1000.0, tech.linknext, tech.accent, chordid, tech.stringmute, highdensity, tech.ignore, tech.palmmute, tech.hopo, direction);
							(void) pack_fputs(buffer, fp);
							//Write chordnote tags
							for(stringnum = 0, bitmask = 1; stringnum < tp->numstrings; stringnum++, bitmask <<= 1)
							{	//For each string used in this track, write chordNote tags
								if((eof_get_note_note(sp, track, ctr3) & bitmask) && !(tp->note[ctr3]->ghost & bitmask))
								{	//If this string is used in this note and it is not ghosted
									long originallength = tp->note[ctr3]->length;	//Back up the original length of this note because it may be altered before export

									(void) eof_rs_combine_linknext_logic(sp, track, ctr3, stringnum);

									assert(chordlist != NULL);	//Unneeded check to resolve a false positive in Splint
									eof_rs2_export_note_string_to_xml(sp, track, ctr3, stringnum, 1, chordlist[chordid], fp);	//Write this chordNote's XML tag

									tp->note[ctr3]->length = originallength;	//Restore the original length to the chord
								}//If this string is used in this note and it is not ghosted
							}//For each string used in this track
							(void) pack_fputs("        </chord>\n", fp);
							lastchordid = chordid;
						}//If this chord wasn't split into single notes or converted into a non ghosted chord and is being ignored
					}//If this note is in this difficulty and will export as a chord (at least two non ghosted gems)
				}//For each note in the track
				(void) pack_fputs("      </chords>\n", fp);
			}
			else
			{	//There are no chords in this difficulty, write an empty chords tag
				(void) pack_fputs("      <chords count=\"0\"/>\n", fp);
			}

			//Write other stuff
			(void) pack_fputs("      <fretHandMutes count=\"0\"/>\n", fp);

			//Write anchors (fret hand positions)
			for(ctr3 = 0, anchorcount = 0; ctr3 < tp->handpositions; ctr3++)
			{	//For each hand position defined in the track
				if(tp->handposition[ctr3].difficulty == ctr)
				{	//If the hand position is in this difficulty
					anchorcount++;
				}
			}
			if(!anchorcount)
			{	//If there are no anchors in this track difficulty, automatically generate them
				if((*user_warned & 1) == 0)
				{	//If the user wasn't alerted that one or more track difficulties have no fret hand positions defined
					allegro_message("Warning:  At least one track difficulty has no fret hand positions defined.  They will be created automatically.");
					*user_warned |= 1;
				}
				eof_fret_hand_position_list_dialog_undo_made = 1;	//Ensure no undo state is written during export
				eof_generate_efficient_hand_positions(sp, track, ctr, 0, 0);	//Generate the fret hand positions for the track difficulty being currently written (use a static fret range tolerance of 4 for all frets)
				anchorsgenerated = 1;
			}
			for(ctr3 = 0, anchorcount = 0; ctr3 < tp->handpositions; ctr3++)	//Re-count the hand positions
			{	//For each hand position defined in the track
				if(tp->handposition[ctr3].difficulty == ctr)
				{	//If the hand position is in this difficulty
					anchorcount++;
				}
			}
			if(anchorcount)
			{	//If there's at least one anchor in this difficulty
				(void) snprintf(buffer, sizeof(buffer) - 1, "      <anchors count=\"%lu\">\n", anchorcount);
				(void) pack_fputs(buffer, fp);
				for(ctr3 = 0; ctr3 < tp->handpositions; ctr3++)
				{	//For each hand position defined in the track
					if(tp->handposition[ctr3].difficulty == ctr)
					{	//If the hand position is in this difficulty
						unsigned long highest, nextanchorpos, width = 4, fret;

						nextanchorpos = tp->note[tp->notes - 1]->pos + 1;	//In case there are no other anchors, initialize this to reflect covering all remaining notes
						for(ctr4 = ctr3 + 1; ctr4 < tp->handpositions; ctr4++)
						{	//For the remainder of the fret hand positions
							if(tp->handposition[ctr4].difficulty == ctr)
							{	//If the hand position is in this difficulty
								nextanchorpos = tp->handposition[ctr4].start_pos;	//Track its position
								break;
							}
						}
						highest = eof_get_highest_fret_in_time_range(sp, track, ctr, tp->handposition[ctr3].start_pos, nextanchorpos - 1);	//Find the highest fret number used within the scope of this fret hand position
						if(highest > tp->handposition[ctr3].end_pos + 3)
						{	//If any notes within the scope of this fret hand position require the anchor width to be increased beyond 4 frets
							width = highest - tp->handposition[ctr3].end_pos + 1;	//Determine the minimum needed width
						}
						fret = tp->handposition[ctr3].end_pos + tp->capo;	//Apply the capo position
						(void) snprintf(buffer, sizeof(buffer) - 1, "        <anchor time=\"%.3f\" fret=\"%lu\" width=\"%lu.000\"/>\n", (double)tp->handposition[ctr3].start_pos / 1000.0, fret, width);
						(void) pack_fputs(buffer, fp);
					}
				}
				(void) pack_fputs("      </anchors>\n", fp);
			}
			else
			{	//There are no anchors in this difficulty, write an empty anchors tag
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Error:  Failed to automatically generate fret hand positions for level %lu of\n\"%s\" during MIDI export.", ctr2, fn);
				allegro_message("%s", eof_log_string);
				eof_log(eof_log_string, 1);
				(void) pack_fputs("      <anchors count=\"0\"/>\n", fp);
			}
			if(anchorsgenerated)
			{	//If anchors were automatically generated for this track difficulty, remove them now
				for(ctr3 = tp->handpositions; ctr3 > 0; ctr3--)
				{	//For each hand position defined in the track, in reverse order
					if(tp->handposition[ctr3 - 1].difficulty == ctr)
					{	//If the hand position is in this difficulty
						eof_pro_guitar_track_delete_hand_position(tp, ctr3 - 1);	//Delete the hand position
					}
				}
			}

			//Count/write the handshapes
			for(handshapeloop = 0, handshapectr = 0; handshapeloop < 2; handshapeloop++)
			{	//On first pass, reset the handshape counter and count the number of handshapes.  On second pass, write handshapes.
				if(handshapeloop)
				{	//If this is the second pass, write the opening of the handshapes tag
					if(!handshapectr)
					{	//If there were no handshapes
						(void) pack_fputs("      <handShapes count=\"0\"/>\n", fp);
						break;	//Exit loop
					}
					(void) snprintf(buffer, sizeof(buffer) - 1, "      <handShapes count=\"%lu\">\n", handshapectr);
					(void) pack_fputs(buffer, fp);
				}

				for(ctr3 = 0; ctr3 < tp->notes; ctr3++)
				{	//For each note in the track
					if(!eof_is_partially_ghosted(sp, track, ctr3) && (tp->note[ctr3]->tflags & EOF_NOTE_TFLAG_TWIN))
					{	//If the chord is NOT partially ghosted and has this flag set, it has a clone with ghost gems that should instead be used for handshape tag export
						continue;
					}
					if(!(tp->note[ctr3]->tflags & EOF_NOTE_TFLAG_IGNORE) || (tp->note[ctr3]->tflags & EOF_NOTE_TFLAG_ARP_FIRST))
					{	//If this note is not ignored or if it is the base chord for an arpeggio/handshape phrase
						if((eof_get_note_type(sp, track, ctr3) == ctr) && ((eof_note_count_rs_lanes(sp, track, ctr3, 2) > 1) || eof_is_partially_ghosted(sp, track, ctr3) || (tp->note[ctr3]->tflags & EOF_NOTE_TFLAG_ARP_FIRST)))
						{	//If this note is in this difficulty and will export as a chord (at least two non ghosted gems) or an arpeggio/handshape or is the first in an arpeggio/handshape phrase
							unsigned long chordnum = ctr3;	//Store a copy of this note number because ctr3 will be manipulated below
							unsigned long sourcenote = ctr3;	//The note number to match against the chord list

							//Find this chord's ID
							chordid = chordlistsize;	//Initialize this to an invalid value
							if((tp->note[ctr3]->tflags & EOF_NOTE_TFLAG_TEMP) && (tp->note[ctr3]->eflags & EOF_PRO_GUITAR_NOTE_EFLAG_GHOST_HS))
							{	//If this was a temporary note created due to ghost handshape status, find the note representing the whole handshape
								for(ctr4 = 0; ctr4 < tp->notes; ctr4++)
								{	//For each note in the track
									if((tp->note[ctr4]->type == tp->note[ctr3]->type) && (tp->note[ctr4]->pos == tp->note[ctr3]->pos) && (tp->note[ctr4]->tflags & EOF_NOTE_TFLAG_GHOST_HS))
									{	//If it's the matching chord
										sourcenote = ctr4;
										break;
									}
								}
							}
							for(ctr4 = 0; ctr4 < chordlistsize; ctr4++)
							{	//For each of the entries in the unique chord list
								assert(chordlist != NULL);	//Unneeded check to resolve a false positive in Splint
								if(!eof_note_compare_simple(sp, track, sourcenote, chordlist[ctr4]))
								{	//If this note matches a chord list entry
									if(!eof_pro_guitar_note_compare_fingerings(tp->note[sourcenote], tp->note[chordlist[ctr4]]))
									{	//If this note has identical fingering to chord list entry
										if(!strcmp(tp->note[sourcenote]->name, tp->note[chordlist[ctr4]]->name))
										{	//If the chord names match
											if(eof_tflag_is_arpeggio(tp->note[sourcenote]->tflags) == eof_tflag_is_arpeggio(tp->note[chordlist[ctr4]]->tflags))
											{	//If this note's arpeggio status (is in an arpeggio and not a handshape, or in no arpeggio/handshape at all) is the same as that of the chord list entry
												chordid = ctr4;	//Store the chord list entry number
												break;
											}
										}
									}
								}
							}
							if((ctr4 >= chordlistsize) || (chordid >= chordlistsize))
							{	//If the chord couldn't be found
								eof_seek_and_render_position(track, tp->note[ctr3]->type, tp->note[ctr3]->pos);
								allegro_message("Error:  Couldn't match chord with chord template while writing handshapes.  Aborting Rocksmith 2 export.");
								eof_log("Error:  Couldn't match chord with chord template while writing handshapes.  Aborting Rocksmith 2 export.", 1);
								if(chordlist)
								{	//If the chord list was built
									free(chordlist);
								}
								eof_rs_export_cleanup(sp, track);	//Remove all temporary notes that were added and remove ignore status from notes
								eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view if applicable
								(void) pack_fclose(fp);
								return 0;	//Return error
							}
							handshapestart = eof_get_note_pos(sp, track, ctr3);	//Use this chord's start position, unless the loop below finds it is inside an arpeggio/handshape
							handshapeend = handshapestart;	//Set a condition that can be checked later to determine if an arpeggio/handshape phrase was parsed

							//If this chord is at the beginning of an arpeggio/handshape phrase, skip the rest of the notes in that phrase
							for(ctr5 = 0; ctr5 < tp->arpeggios; ctr5++)
							{	//For each arpeggio/handshape phrase in the track
								if(((tp->note[ctr3]->pos >= tp->arpeggio[ctr5].start_pos) && (tp->note[ctr3]->pos <= tp->arpeggio[ctr5].end_pos)) && (tp->note[ctr3]->type == tp->arpeggio[ctr5].difficulty))
								{	//If this chord's start position is within an arpeggio/handshape phrase in this track difficulty
									handshapestart = tp->arpeggio[ctr5].start_pos;	//This arpeggio/handshape phrase will define the start and stop positions for the exported handshape
									handshapeend = tp->arpeggio[ctr5].end_pos;
									while(1)
									{
										nextnote = eof_track_fixup_next_note(sp, track, ctr3);
										if((nextnote >= 0) && (tp->note[nextnote]->pos <= tp->arpeggio[ctr5].end_pos))
										{	//If there is another note and it is in the same arpeggio phrase
											ctr3 = nextnote;	//Iterate to that note, and check subsequent notes to see if they are also in the phrase
										}
										else
										{	//The next note (if any) is not in the arpeggio phrase
											break;	//Break from while loop
										}
									}
									break;	//Break from for loop
								}
							}

							//Examine subsequent notes to see if they match this chord
							if(handshapeend == handshapestart)
							{	//If the chord wasn't inside of an arpeggio/handshape phrase, generate the handshape tag automatically
								while(1)
								{
									nextnote = eof_track_fixup_next_note(sp, track, ctr3);
									if((nextnote >= 0) && (eof_note_count_rs_lanes(sp, track, nextnote, 2) > 1))
									{	//If there is another note and it is a chord
										if(!eof_note_has_high_chord_density(sp, track, nextnote, 2))
										{	//If the next note is low density (including if it has any techniques requiring a new handshape tag such as sliding)
											handshapeend = eof_get_note_pos(sp, track, ctr3) + eof_get_note_length(sp, track, ctr3);	//End the hand shape at the end of this chord
											break;	//Break from while loop
										}
									}
									if((nextnote >= 0) && (tp->note[nextnote]->tflags & EOF_NOTE_TFLAG_ARP_FIRST))
									{	//If there is another note, and it is the first note in a arpeggio/handshape phrase
										handshapeend = eof_get_note_pos(sp, track, ctr3) + eof_get_note_length(sp, track, ctr3);	//End the hand shape at the end of this chord
										break;
									}
									if((nextnote >= 0) && (eof_note_count_rs_lanes(sp, track, nextnote, 2) > 1) && eof_is_string_muted(sp, track, nextnote) && (eof_pro_guitar_note_fingering_valid(tp, nextnote, 1) == 2))
									{	//If there is another note, and it is fully string muted and has no fingering defined even for muted strings
										ctr3 = nextnote;	//Iterate to that note, and check subsequent notes to see if they match
									}
									else if((nextnote >= 0) && (!eof_note_compare_simple(sp, track, chordnum, nextnote) || eof_is_string_muted(sp, track, nextnote)) && !eof_is_partially_ghosted(sp, track, nextnote) && !eof_pro_guitar_note_compare_fingerings(tp->note[chordnum], tp->note[nextnote]))
									{	//If there is another note, it either matches this chord or is completely string muted, it is not partially ghosted (an arpeggio) and it has the same fingering
										if(eof_is_partially_ghosted(sp, track, chordnum))
										{	//If the handshape being written was for an arpeggio, and the next note isn't
											handshapeend = eof_get_note_pos(sp, track, ctr3) + eof_get_note_length(sp, track, ctr3);	//End the hand shape at the end of the arpeggio's last note
											break;	//Break from while loop
										}
										ctr3 = nextnote;	//Iterate to that note, and check subsequent notes to see if they match
									}
									else
									{	//The next note (if any) is not a repeat of this note and is not completely string muted
										handshapeend = eof_get_note_pos(sp, track, ctr3) + eof_get_note_length(sp, track, ctr3);	//End the hand shape at the end of this chord
										break;	//Break from while loop
									}
								}
							}

							if(!handshapeloop)
							{	//If this is the first pass
								handshapectr++;	//One more hand shape has been counted
							}
							else
							{	//Second pass, pad and write the handshape tag
								if(handshapeend - handshapestart < 56)
								{	//If the handshape is shorter than 56ms, see if it can be padded to 56ms
									nextnote = ctr3;
									do{	//Find the next note that isn't ignored, if any
										nextnote = eof_track_fixup_next_note(sp, track, nextnote);
									}while((nextnote >= 0) && (tp->note[nextnote]->tflags & EOF_NOTE_TFLAG_IGNORE));
									if((nextnote < 0) || (handshapestart + 56 < eof_get_note_pos(sp, track, nextnote)))
									{	//If no notes follow this chord, or if there's at least 56ms of gap between this chord and the next note, the handshape can be lengthened without any threat of overlapping another handshape tag
										handshapeend = handshapestart + 56;
									}
								}

								//Write this hand shape
								(void) snprintf(buffer, sizeof(buffer) - 1, "        <handShape chordId=\"%lu\" endTime=\"%.3f\" startTime=\"%.3f\"/>\n", chordid, (double)handshapeend / 1000.0, (double)handshapestart / 1000.0);
								(void) pack_fputs(buffer, fp);
							}
						}//If this note is in this difficulty and will export as a chord (at least two non ghosted gems) or an arpeggio/handshape or is in an arpeggio/handshape phrase
					}//If this note is not ignored or if it is the base chord for an arpeggio/handshape phrase
				}//For each note in the track
			}//On first pass, count the number of handshapes.  On second pass, write handshapes.
			if(handshapectr)
			{	//If there were handshapes
				(void) pack_fputs("      </handShapes>\n", fp);
			}

			//Write closing level tag
			(void) pack_fputs("    </level>\n", fp);
		}//If this difficulty is populated
	}//For each of the available difficulties
	(void) pack_fputs("  </levels>\n", fp);
	(void) pack_fputs("</song>\n", fp);
	(void) pack_fclose(fp);

	eof_rs_export_cleanup(sp, track);	//Remove all temporary notes that were added and remove ignore and arpeggio status from notes

	//Cleanup
	if(chordlist)
	{	//If the chord list was built
		free(chordlist);
	}
	//Remove all temporary text events that were added
	for(ctr = sp->text_events; ctr > 0; ctr--)
	{	//For each text event (in reverse order)
		if(sp->text_event[ctr - 1]->is_temporary)
		{	//If this text event has been marked as temporary
			eof_song_delete_text_event(sp, ctr - 1);	//Delete it
		}
	}
	eof_sort_events(sp);	//Re-sort events
	eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view if applicable

	return 1;	//Return success
}

void eof_pro_guitar_track_fix_fingerings(EOF_PRO_GUITAR_TRACK *tp, char *undo_made)
{
	unsigned long ctr2, ctr3;
	unsigned char *array;	//Points to the finger array being replicated to matching notes
	int retval;

	if(!tp)
		return;	//Invalid parameter

	for(ctr2 = 0; ctr2 < tp->notes; ctr2++)
	{	//For each note in the track (outer loop)
		retval = eof_pro_guitar_note_fingering_valid(tp, ctr2, 0);
		if(retval == 1)
		{	//If the note's fingering was complete
			if(eof_note_count_colors_bitmask(tp->note[ctr2]->note) > 1)
			{	//If this note is a chord
				array = tp->note[ctr2]->finger;
				for(ctr3 = 0; ctr3 < tp->notes; ctr3++)
				{	//For each note in the track (inner loop)
					if((ctr2 != ctr3) && (eof_pro_guitar_note_compare(tp, ctr2, tp, ctr3, 0) == 0))
					{	//If this note matches the note being examined in the outer loop, and we're not comparing the note to itself
						if(eof_pro_guitar_note_fingering_valid(tp, ctr3, 0) != 1)
						{	//If the fingering of the inner loop's note is invalid/undefined
							if(undo_made && !(*undo_made))
							{	//If an undo hasn't been made yet
								eof_prepare_undo(EOF_UNDO_TYPE_NONE);
								*undo_made = 1;
							}
							memcpy(tp->note[ctr3]->finger, array, 8);	//Overwrite it with the current finger array
						}
						else
						{	//The inner loop's note has a valid fingering array defined
							array = tp->note[ctr3]->finger;	//Use this finger array for remaining matching notes in the track
						}
					}
				}//For each note in the track (inner loop)
			}
		}
		else if(retval == 0)
		{	//If the note's fingering was defined, but invalid
			if(undo_made && !(*undo_made))
			{	//If an undo hasn't been made yet
				eof_prepare_undo(EOF_UNDO_TYPE_NONE);
				*undo_made = 1;
			}
			memset(tp->note[ctr2]->finger, 0, 8);	//Clear it
		}
	}
}

int eof_pro_guitar_note_fingering_valid(EOF_PRO_GUITAR_TRACK *tp, unsigned long note, char count_mutes)
{
	unsigned long ctr, bitmask;
	char string_finger_defined = 0, string_finger_undefined = 0, all_strings_open = 1;

	if(!tp || (note >= tp->notes))
		return 0;	//Invalid parameters

	for(ctr = 0, bitmask = 1; ctr < tp->numstrings; ctr++, bitmask <<= 1)
	{	//For each string supported by this track
		if(tp->note[note]->note & bitmask)
		{	//If this string is used
			if(!(tp->note[note]->frets[ctr] & 0x80) || count_mutes)
			{	//If the string isn't muted, or if the calling function wants muted strings inspected
				if(tp->note[note]->frets[ctr] != 0)
				{	//If the string isn't being played open, it requires a fingering
					all_strings_open = 0;	//Track that the note used at least one fretted string
					if(tp->note[note]->finger[ctr] != 0)
					{	//If this string has a finger definition
						string_finger_defined = 1;	//Track that a string was defined
					}
					else
					{	//This string does not have a finger definition
						string_finger_undefined = 1;	//Track that a string was undefined
					}
				}
				else
				{	//If the string is being played open, it must not have a finger defined
					if(tp->note[note]->finger[ctr] != 0)
					{	//If this string has a finger definition
						string_finger_defined = string_finger_undefined = 1;	//Set an error condition
						break;
					}
				}
			}
		}
	}

	if(all_strings_open && !string_finger_defined)
	{	//If the note only had open or muted strings played, and no fingering was defined, this is valid
		return 1;	//Return fingering valid
	}
	if(string_finger_defined && string_finger_undefined)
	{	//If a note only had partial finger definition
		return 0;	//Return fingering invalid
	}
	if(string_finger_defined)
	{	//If the finger definition was complete
		return 1;	//Return fingering valid
	}
	return 2;	//Return fingering undefined
}

void eof_song_fix_fingerings(EOF_SONG *sp, char *undo_made)
{
	unsigned long ctr;
	char restore_tech_view;

	if(!sp)
		return;	//Invalid parameter

	for(ctr = 1; ctr < sp->tracks; ctr++)
	{	//For each track (skipping the NULL global track 0)
		if(sp->track[ctr]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If this is a pro guitar track
			restore_tech_view = eof_menu_track_get_tech_view_state(sp, ctr);
			eof_menu_track_set_tech_view_state(sp, ctr, 0); //Disable tech view if applicable
			eof_pro_guitar_track_fix_fingerings(sp->pro_guitar_track[sp->track[ctr]->tracknum], undo_made);	//Correct and complete note fingering where possible, performing an undo state before making changes
			eof_menu_track_set_tech_view_state(sp, ctr, restore_tech_view); //Re-enable tech view if applicable
		}
	}
}

void eof_generate_efficient_hand_positions_logic(EOF_SONG *sp, unsigned long track, char difficulty, char warnuser, char dynamic, unsigned long startnote, unsigned long stopnote, char function)
{
	unsigned long ctr, ctr2, tracknum, count, bitmask, beatctr, startpos = 0, endpos, shapenum = 0, prevnote = 0;
	unsigned long effectivestart, effectivestop;	//The start and stop timestamps of the affected range of notes
	EOF_PRO_GUITAR_TRACK *tp;
	unsigned char current_low, current_high, last_anchor = 0;
	EOF_PRO_GUITAR_NOTE *next_position = NULL;	//Tracks the note at which the next fret hand position will be placed
	EOF_PRO_GUITAR_NOTE *np, temp = {{0}, 0, 0, 0, {0}, {0}, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, temp2 = {{0}, 0, 0, 0, {0}, {0}, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};	//Used to track FHPs influenced by fingering or by slides
	char force_change, started = 0;
	char restore_tech_view = 0;		//If tech view is in effect, it is temporarily disabled until after the fret hand positions are generated
	char all = 0;	//Is set to nonzero if the values passed for startnote and stopnote are equal, indicating all existing fret hand positions are to be replaced
	char *warning, warning1[] = "Existing fret hand positions for the active track difficulty will be removed.", warning2[] = "Existing fret hand positions for the selected range of notes will be removed.";
	unsigned limit = 21;	//Rocksmith 2's fret hand position limit is 21
	long lastnoteslide = 0;	//Tracks the number of frets the last processed note slid (for adding FHPs to track when the fret hand slides)
	long nextnote;

	if(!sp || (track >= sp->tracks) || (sp->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return;	//Invalid parameters

	if(eof_write_rs_files || eof_write_rb_files)
	{	//If Rocksmith 1 or Rock Band export is enabled
		limit = 19;	//The highest fret hand position change is 19 instead
	}

	//Set up
	tracknum = sp->track[track]->tracknum;
	tp = sp->pro_guitar_track[tracknum];
	if((tp->notes == 0) || (startnote >= tp->notes) || (stopnote >= tp->notes) || (startnote > stopnote))
	{	//If there are no notes or the specified start or stop note numbers are not valid
		return;	//Invalid parameters
	}
	restore_tech_view = eof_menu_track_get_tech_view_state(sp, track);
	eof_menu_track_set_tech_view_state(sp, track, 0);	//Disable tech view for the specified pro guitar track if applicable
	if(startnote == stopnote)
	{	//If fret hand positions for the entire track difficulty are to be generated
		all = 1;
		startnote = 0;
		stopnote = tp->notes - 1;	//The last note processed in the fret hand position loop is the last note in this track
		warning = warning1;
	}
	else
	{
		warning = warning2;
	}
	effectivestart = tp->note[startnote]->pos;
	effectivestop = tp->note[stopnote]->pos;

	//Remove any existing fret hand positions in the specified scope
	for(ctr = tp->handpositions; ctr > 0; ctr--)
	{	//For each existing hand positions in this track (in reverse order)
		if(tp->handposition[ctr - 1].difficulty == difficulty)
		{	//If this hand position is defined for the specified difficulty
			if(all || ((tp->handposition[ctr - 1].start_pos >= tp->note[startnote]->pos) && (tp->handposition[ctr - 1].start_pos <= tp->note[stopnote]->pos)))
			{	//If this fret hand position is in the affected range of this function
				if(warnuser && function)
				{	//Skip the warning if the logic to delete FHPs in the specified range was called explicitly
					eof_clear_input();
					if(alert(NULL, warning, "Continue?", "&Yes", "&No", 'y', 'n') != 1)
					{	//If the user does not opt to remove the existing hand positions
						eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view for the second piano roll's track if applicable
						return;
					}
				}
				warnuser = 0;
				if(!eof_fret_hand_position_list_dialog_undo_made)
				{	//If an undo state hasn't been made yet since launching this dialog
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					eof_fret_hand_position_list_dialog_undo_made = 1;
				}
				eof_pro_guitar_track_delete_hand_position(tp, ctr - 1);	//Delete the hand position
			}
		}
	}
	eof_pro_guitar_track_sort_fret_hand_positions(tp);	//Sort the positions

	if(!function)
	{	//If this function was only called to delete fret hand positions
		return;
	}

	//Count the number of notes in the specified track difficulty and allocate arrays large enough to store the lowest and highest fret number used in each
	for(ctr = 0, count = 0; ctr < tp->notes; ctr++)
	{	//For each note in the track
		if(tp->note[ctr]->type == difficulty)
		{	//If it is in the specified difficulty
			count++;	//Increment this counter
		}
	}

	if(!count)
	{	//If this track difficulty has no notes
		eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view for the second piano roll's track if applicable
		return;	//Exit function
	}

	eof_build_fret_range_tolerances(tp, difficulty, dynamic);	//Allocate and build eof_fret_range_tolerances[], using the calling function's chosen option regarding tolerances
	if(!eof_fret_range_tolerances)
	{	//eof_fret_range_tolerances[] wasn't built
		eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view for the second piano roll's track if applicable
		return;
	}

	//Initialize low and high fret variables for the generation of hand positions
	if(all)
	{	//If the entire track difficulty's fret hand positions are being generated
		current_low = current_high = 0;	//Reset these
	}
	else
	{	//If fret hand positions for a selection of notes is being generated
		current_low = current_high = eof_pro_guitar_track_find_effective_fret_hand_position(tp, eof_note_type, tp->note[startnote]->pos);	//Set these variables to the value of the fret hand position in effect at the position of the first selected note, if any
	}

	//Iterate through this track difficulty's notes and determine efficient hand positions
	for(ctr = startnote; ctr <= stopnote; ctr++)
	{	//For each note in the track that is in the target range
		if((tp->note[ctr]->type == difficulty) && !(tp->note[ctr]->tflags & EOF_NOTE_TFLAG_TEMP))
		{	//If it is in the specified difficulty and isn't marked as a temporary note (a single note temporarily inserted to allow chord techniques to appear in Rocksmith 1)
			if(!next_position)
			{	//If this is the first note since the last hand position, or there was no hand position placed yet
				next_position = tp->note[ctr];	//Store its address
			}
			force_change = 0;	//Reset this condition

			//Determine if this note is inside an arpeggio/handshape phrase.  The note at the beginning of such a phrase will trigger a change, but other notes inside the phrase will not
			for(ctr2 = 0; ctr2 < tp->arpeggios; ctr2++)
			{	//For each arpeggio/handshape phrase in the track
				if(((tp->note[ctr]->pos >= tp->arpeggio[ctr2].start_pos) && (tp->note[ctr]->pos <= tp->arpeggio[ctr2].end_pos)) && (tp->note[ctr]->type == tp->arpeggio[ctr2].difficulty))
				{	//If this note's start position is within an arpeggio/handshape phrase in this track difficulty
					force_change = 3;
					break;	//Break from for loop
				}
			}

			//Determine if this note is a chord that uses the index finger, which will trigger a fret hand position change (if this chord's fingering is incomplete, perform a chord shape lookup)
			if(!force_change)
			{	//If a fret hand change wasn't already determined necessary
				np = tp->note[ctr];	//Unless the chord's fingering is incomplete, the note's current fingering will be used to determine whether the index finger triggers a position change
				if((eof_note_count_colors(eof_song, track, ctr) > 1) && !eof_is_string_muted(eof_song, track, ctr))
				{	//If this note is a chord that isn't completely string muted
					if(eof_pro_guitar_note_fingering_valid(tp, ctr, 0) != 1)
					{	//If the fingering for the note is not fully defined
						if(eof_lookup_chord_shape(np, &shapenum, 0))
						{	//If a fingering for the chord can be found in the chord shape definitions
							memcpy(temp.frets, np->frets, 6);	//Clone the fretting of the original note into the temporary note
							temp.note = np->note;				//Clone the note mask
							eof_apply_chord_shape_definition(&temp, shapenum);	//Apply the matching chord shape definition's fingering to the temporary note
							np = &temp;	//Check the temporary note for use of the index finger, instead of the original note
						}
					}
				}
				for(ctr2 = 0, bitmask = 1; ctr2 < 6; ctr2++, bitmask <<= 1)
				{	//For each of the 6 supported strings
					if((np->note & bitmask) && (np->finger[ctr2] == 1))
					{	//If this note uses this string, and the string is defined as being fretted by the index finger
						force_change = 1;
						break;
					}
				}
			}

			//Determine if this note follows a slide and a fret hand position should be added due to the slide
			if(!force_change && lastnoteslide)
			{	//If this note didn't already trigger a fret hand position change and the last note had a slide
				if(lastnoteslide < 0)
				{	//If the slide note went down
					unsigned long lastnoteslide_abs = -lastnoteslide;
					if((current_low >= lastnoteslide_abs) && (current_low - lastnoteslide <= eof_pro_guitar_note_lowest_fret(tp, ctr)))
					{	//If the current fret hand position can be moved the same number of frets as the slide and still be valid for this note
						force_change = 2;
					}
				}
				else
				{	//If the slide note went up
					if(current_low + lastnoteslide <= eof_pro_guitar_note_lowest_fret(tp, ctr))
					{	//If the current fret hand position can be moved the same number of frets as the slide and still be valid for this note
						force_change = 2;
					}
				}
			}

			if(force_change || !eof_note_can_be_played_within_fret_tolerance(tp, ctr, &current_low, &current_high))
			{	//If a position change was determined to be necessary based on fingering/sliding or arpeggio/handshape phrasing, or this note can't be included with previous notes within a single fret hand position
				if(current_low + tp->capo > limit)
				{	//Ensure the fret hand position is capped at the appropriate limit based on the game exports enabled
					current_low = limit - tp->capo;
				}
				if(force_change)
				{
					if(next_position != tp->note[ctr])
					{	//If the fret hand position for previous notes has not been placed yet, write it first
						if(current_low && (current_low != last_anchor))
						{	//As long as the hand position being written is valid and is is different from the previous one
							if(!eof_fret_hand_position_list_dialog_undo_made)
							{	//If an undo state hasn't been made yet since launching this dialog
								eof_prepare_undo(EOF_UNDO_TYPE_NONE);
								eof_fret_hand_position_list_dialog_undo_made = 1;
							}
							if(eof_pro_guitar_track_find_effective_fret_hand_position(tp, difficulty, next_position->pos) != current_low)
							{	//If the desired fret hand position isn't already in effect
								if(eof_track_add_section(sp, track, EOF_FRET_HAND_POS_SECTION, difficulty, next_position->pos, current_low, 0, NULL))
								{	//Add the fret hand position for this forced position change, if successful
									last_anchor = current_low;	//Track the current anchor
									eof_pro_guitar_track_sort_fret_hand_positions(tp);	//Sort fret hand positions
								}
							}
						}
						if(force_change == 1)
						{	//If the position change is due to fingering
							next_position = tp->note[ctr];	//The fret hand position for the current note will be written next
						}
						else if(force_change == 2)
						{	//If the position change is due to sliding
							next_position = &temp2;	//The fret hand position for the end of the slide will be written next
						}
						else if(force_change == 3)
						{	//If the position change is due to arpeggio/handshape phrasing
							next_position = tp->note[ctr];	//The fret hand position for the current note will be written next
						}
					}
					//Now that the previous notes' position is in place, update the high and low fret tracking for the current note
					current_low = eof_pro_guitar_note_lowest_fret(tp, ctr);	//Track this note's high and low frets
					current_high = eof_pro_guitar_note_highest_fret(tp, ctr);
					if((current_low && (current_low != last_anchor)) || (force_change == 3))
					{	//As long as the hand position being written is valid and is is different from the previous one, or if arpeggio/handshape phrasing is forcing a change regardless of the previous position
						if(!eof_fret_hand_position_list_dialog_undo_made)
						{	//If an undo state hasn't been made yet since launching this dialog
							eof_prepare_undo(EOF_UNDO_TYPE_NONE);
							eof_fret_hand_position_list_dialog_undo_made = 1;
						}
						if(current_low + tp->capo > limit)
						{	//Ensure the fret hand position is capped at the appropriate limit based on the game exports enabled
							current_low = limit - tp->capo;
						}
						if(!current_low)
						{	//If the range of affected notes had only open notes
							if(!last_anchor)
							{	//If no fret hand positions were placed yet
								current_low = 1;	//Place the fret hand position at fret 1
							}
							else
							{	//Otherwise the last placed fret hand position remains in effect
								current_low = last_anchor;
							}
						}
						if(eof_pro_guitar_track_find_effective_fret_hand_position(tp, difficulty, next_position->pos) != current_low)
						{	//If the desired fret hand position isn't already in effect
							if(eof_track_add_section(sp, track, EOF_FRET_HAND_POS_SECTION, difficulty, next_position->pos, current_low, 0, NULL))
							{	//Add the fret hand position for this forced position change, if successful
								last_anchor = current_low;	//Track the current anchor
								eof_pro_guitar_track_sort_fret_hand_positions(tp);	//Sort fret hand positions
							}
						}
					}
					//If necessary, seek to end of arpeggio/handshape phrase so the next FHP written is beyond it
					if(force_change == 3)
					{	//If the position change is due to arpeggio/handshape phrasing
						for(ctr2 = 0; ctr2 < tp->arpeggios; ctr2++)
						{	//For each arpeggio/handshape phrase in the track
							if(((tp->note[ctr]->pos >= tp->arpeggio[ctr2].start_pos) && (tp->note[ctr]->pos <= tp->arpeggio[ctr2].end_pos)) && (tp->note[ctr]->type == tp->arpeggio[ctr2].difficulty))
							{	//If this is the arpeggio/handshape phrase the note was in
								while(1)
								{
									nextnote = eof_track_fixup_next_note(sp, track, ctr);
									if(nextnote >= 0)
									{	//If there is another note in this track difficulty
										if(tp->note[nextnote]->pos <= tp->arpeggio[ctr2].end_pos)
										{	//And it is inside the same arpeggio/handshape phrase
											ctr = nextnote;	//Iterate to that note
										}
										else
										{	//It is the first note that is after the arpeggio/handshape phrase
											break;	//Break from while loop
										}
									}
									else
										break;	//Break from while loop
								}
								break;	//Break from for loop
							}
						}
					}
					next_position = NULL;	//This note's position will not receive another hand position, the next loop iteration will look for any necessary position changes starting with the next note's location
				}
				else
				{	//If the position change is being placed on a previous note due to this note going out of fret tolerance
					if(current_low && (current_low != last_anchor))
					{	//As long as the hand position being written is valid and is is different from the previous one
						if(!eof_fret_hand_position_list_dialog_undo_made)
						{	//If an undo state hasn't been made yet since launching this dialog
							eof_prepare_undo(EOF_UNDO_TYPE_NONE);
							eof_fret_hand_position_list_dialog_undo_made = 1;
						}
						if(eof_pro_guitar_track_find_effective_fret_hand_position(tp, difficulty, next_position->pos) != current_low)
						{	//If the desired fret hand position isn't already in effect
							if(eof_track_add_section(sp, track, EOF_FRET_HAND_POS_SECTION, difficulty, next_position->pos, current_low, 0, NULL))
							{	//Add the fret hand position for this forced position change, if successful
								last_anchor = current_low;	//Track the current anchor
								eof_pro_guitar_track_sort_fret_hand_positions(tp);	//Sort fret hand positions
							}
						}
					}
					next_position = tp->note[ctr];	//The fret hand position for the current note will be written next
				}
				if(force_change != 3)
				{	//If a FHP wasn't placed due to a handshape/arpeggio phrase
					current_low = eof_pro_guitar_note_lowest_fret(tp, ctr);	//Track this note's high and low frets
					current_high = eof_pro_guitar_note_highest_fret(tp, ctr);
				}
			}//If a position change was determined to be necessary based on fingering/sliding or arpeggio/handshape phrasing, or this note can't be included with previous notes within a single fret hand position
			else if((ctr != prevnote) && !eof_note_compare_simple(sp, track, ctr, prevnote))
			{	//Otherwise if there was a previous note and this note is a repeat of that note
				if(next_position)
				{
					unsigned char lastlow, lasthigh, thislow, thishigh;
					lastlow = eof_pro_guitar_track_find_effective_fret_hand_position(tp, difficulty, next_position->pos);
					if(lastlow)
					{	//If there is already a fret hand position in effect at this note's position
						lasthigh = lastlow + eof_fret_range_tolerances[lastlow] - 1;	//Recreate the fret range of the fret hand position in question
						thislow = (lastlow < current_low) ? lastlow : current_low;		//Target the lesser of the previous FHP's low fret value and the current note range's low fret value
						thishigh = (lasthigh > current_high) ? lasthigh : current_high;	//Target the greater of the previous FHP's highest valid fret and the current note range's high fret value
						if(eof_note_can_be_played_within_fret_tolerance(tp, ctr, &thislow, &thishigh))
						{	//If the pending range of notes can still fit within the effective fret hand position's range
							next_position = NULL;	//Prevent a FHP change from taking place on this note when the same previous note wasn't targeted for one, as that wouldn't make sense
						}
					}
				}
			}

			//Track the number of frets this note slides
			lastnoteslide = 0;
			np = tp->note[ctr];	//Simplify
			if(np->flags & (EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP | EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN | EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE))
			{	//If the note that was just processed has slide technique
				memcpy(&temp2, tp->note[ctr], sizeof(temp2));	//Clone the note
				temp2.pos = temp2.pos + temp2.length;	//Change the clone's start position (the position where a fret hand position was be placed) to the end of the note
				if((np->flags & EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION) && (np->slideend || np->unpitchend))
				{	//If the end of slide position is defined
					if(np->flags & (EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP | EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN))
					{	//If the note was a pitched slide
						lastnoteslide = np->slideend - eof_pro_guitar_note_lowest_fret(tp, ctr);
					}
					else
					{	//If the note was an unpitched slide
						lastnoteslide = np->unpitchend - eof_pro_guitar_note_lowest_fret(tp, ctr);
					}
				}
			}
			prevnote = ctr;	//Track the last processed note number
		}//If it is in the specified difficulty and isn't marked as a temporary note (a single note inserted to allow chord techniques to appear in Rocksmith)
	}//For each note in the track that is in the target range

	//The last one or more notes examined need to have their hand position placed
	if(!current_low)
	{	//If the range of affected notes had only open notes
		if(!last_anchor)
		{	//If no fret hand positions were placed
			current_low = 1;	//Place the fret hand position at fret 1
		}
		else
		{	//Otherwise the last placed fret hand position remains in effect
			current_low = last_anchor;
		}
	}
	else if(current_low + tp->capo > limit)
	{	//Ensure the fret hand position is capped at the appropriate limit based on the game exports enabled
		current_low = limit - tp->capo;
	}
	if((current_low != last_anchor) && next_position)
	{	//If the last parsed note requires a position change
		if(!eof_fret_hand_position_list_dialog_undo_made)
		{	//If an undo state hasn't been made yet since launching this dialog
			eof_prepare_undo(EOF_UNDO_TYPE_NONE);
			eof_fret_hand_position_list_dialog_undo_made = 1;
		}
		if(eof_pro_guitar_track_find_effective_fret_hand_position(tp, difficulty, next_position->pos) != current_low)
		{	//If the desired fret hand position isn't already in effect
			(void) eof_track_add_section(sp, track, EOF_FRET_HAND_POS_SECTION, difficulty, next_position->pos, current_low, 0, NULL);	//Add the best determined fret hand position
			eof_pro_guitar_track_sort_fret_hand_positions(tp);	//Sort fret hand positions
		}
	}

	//Ensure that a fret hand position is defined in each phrase, at or before its first note
	for(beatctr = 0; beatctr < sp->beats; beatctr++)
	{	//For each beat in the project
		if((sp->beat[beatctr]->contained_section_event >= 0) || ((beatctr + 1 >= sp->beats) && started))
		{	//If this beat has a section event (RS phrase) or a phrase is in progress and this is the last beat, it marks the end of any current phrase and the potential start of another
			if(started)
			{	//If the first phrase marker has been encountered, this beat marks the end of a phrase
				endpos = sp->beat[beatctr]->pos - 1;	//Track this as the end position of the phrase
				if((startpos >= effectivestart) && (startpos <= effectivestop))
				{	//If this phrase begins in the portion of the chart affected by this function
					(void) eof_enforce_rs_phrase_begin_with_fret_hand_position(sp, track, difficulty, startpos, endpos, &eof_fret_hand_position_list_dialog_undo_made, 0);	//Add a fret hand position to the beginning of the phrase
				}
			}//If the first phrase marker has been encountered, this beat marks the end of a phrase

			started = 1;	//Track that a phrase has been encountered
			startpos = eof_song->beat[beatctr]->pos;	//Track the starting position of the phrase
		}//If this beat has a section event (RS phrase) or a phrase is in progress and this is the last beat, it marks the end of any current phrase and the potential start of another
	}//For each beat in the project

	//Clean up
	free(eof_fret_range_tolerances);
	eof_fret_range_tolerances = NULL;	//Clear this array so that the next call to eof_build_fret_range_tolerances() rebuilds it accordingly
	eof_pro_guitar_track_sort_fret_hand_positions(tp);	//Sort the positions
	eof_pro_guitar_track_fixup_hand_positions(sp, track);	//Cleanup fret hand positions to remove any instances of multiple FHPs at the same timestamp
	eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view for the second piano roll's track if applicable
	eof_render();
}

void eof_generate_efficient_hand_positions(EOF_SONG *sp, unsigned long track, char difficulty, char warnuser, char dynamic)
{
	eof_generate_efficient_hand_positions_logic(sp, track, difficulty, warnuser, dynamic, 0, 0, 1);	//Generate fret hand positions for the entire track
}

int eof_generate_hand_positions_current_track_difficulty(void)
{
	int junk = 0;
	unsigned long diffindex = 0;

	if(!eof_song || (eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return 0;	//Invalid parameters

	eof_fret_hand_position_list_dialog_undo_made = 0;	//Reset this condition
	eof_generate_efficient_hand_positions(eof_song, eof_selected_track, eof_note_type, 1, 0);	//Warn the user if existing hand positions will be deleted (use a static fret range tolerance of 4 for all frets, since it's assumed the author is charting for Rocksmith if they use this function)

	(void) eof_pro_guitar_track_find_effective_fret_hand_position_definition(eof_song->pro_guitar_track[eof_song->track[eof_selected_track]->tracknum], eof_note_type, eof_music_pos - eof_av_delay, NULL, &diffindex, 0);
		//Obtain the fret hand position change now in effect at the current seek position
	eof_fret_hand_position_list_dialog[1].d1 = diffindex;	//Pre-select it in the list
	(void) dialog_message(eof_fret_hand_position_list_dialog, MSG_START, 0, &junk);	//Re-initialize the dialog
	(void) dialog_message(eof_fret_hand_position_list_dialog, MSG_DRAW, 0, &junk);	//Redraw dialog
	return D_REDRAW;
}

int eof_generate_efficient_hand_positions_for_selected_notes(void)
{
	unsigned long sel_start = 0, sel_end = 0;

	eof_fret_hand_position_list_dialog_undo_made = 0;	//Reset this condition
	if(eof_get_selected_note_range(&sel_start, &sel_end, 0) > 1)
	{	//If multiple notes are selected
		eof_generate_efficient_hand_positions_logic(eof_song, eof_selected_track, eof_note_type, 1, 0, sel_start, sel_end, 1);	//Generate fret hand positions for the range of the chart from the first to last selected note
	}

	return 1;
}

int eof_delete_hand_positions_for_selected_notes(void)
{
	unsigned long sel_start = 0, sel_end = 0;

	eof_fret_hand_position_list_dialog_undo_made = 0;	//Reset this condition
	if(eof_get_selected_note_range(&sel_start, &sel_end, 0) > 1)
	{	//If multiple notes are selected
		eof_generate_efficient_hand_positions_logic(eof_song, eof_selected_track, eof_note_type, 1, 0, sel_start, sel_end, 0);	//Delete fret hand positions for the range of the chart from the first to last selected note
	}

	return 1;
}
int eof_note_can_be_played_within_fret_tolerance(EOF_PRO_GUITAR_TRACK *tp, unsigned long note, unsigned char *current_low, unsigned char *current_high)
{
	unsigned char effective_lowest = 0, effective_highest = 0;	//Stores the cumulative highest and lowest fret values with the input range and the next note for tolerance testing
	long next;

	if(!tp || !current_low || !current_high || (note >= tp->notes) || (*current_low > *current_high) || (*current_high > tp->numfrets) || !eof_fret_range_tolerances)
		return 0;	//Invalid parameters

	while(1)
	{
		effective_lowest = eof_pro_guitar_note_lowest_fret(tp, note);	//Find the highest and lowest fret used in the note
		effective_highest = eof_pro_guitar_note_highest_fret(tp, note);
		if(!effective_lowest && (note + 1 < tp->notes))
		{	//If only open strings are played in the note
			next = eof_fixup_next_pro_guitar_note(tp, note);	//Determine if there's another note in this track difficulty
			if(next >= 0)
			{	//If there was another note
				note = next;	//Examine it and recheck its highest and lowest frets (so that position changes can be made for further ahead notes)
				continue;
			}
		}
		break;
	}

	if(!(*current_low))
	{	//If there's no hand position in effect yet, init the currently tracked high and low frets with this note's
		*current_low = effective_lowest;
		*current_high = effective_highest;
		return 1;	//Return note can be played without an additional hand position
	}

	if(eof_pro_guitar_note_is_barre_chord(tp, note))
	{	//If this note is a barre chord
		if(*current_low != effective_lowest)
		{	//If the ongoing lowest fret value is not at lowest used fret in this barre chord (where the index finger will need to be to play the chord)
			return 0;	//Return note cannot be played without an additional hand position
		}
	}
	if(!effective_lowest)
	{	//If this note didn't have a low fret value
		effective_lowest = *current_low;	//Keep the ongoing lowest fret value
	}
	else if((*current_low != 0) && (*current_low < effective_lowest))
	{	//Otherwise keep the ongoing lowest fret value only if it is defined and is lower than this note's lowest fret value
		effective_lowest = *current_low;
	}
	if(*current_high > effective_highest)
	{	//Obtain the higher of these two values
		effective_highest = *current_high;
	}

	if(effective_highest - effective_lowest + 1 > eof_fret_range_tolerances[effective_lowest])
	{	//If this note can't be played at the same hand position as the one already in effect
		return 0;	//Return note cannot be played without an additional hand position
	}

	*current_low = effective_lowest;	//Update the effective highest and lowest used frets
	*current_high = effective_highest;
	return 1;	//Return note can be played without an additional hand position
}

void eof_build_fret_range_tolerances(EOF_PRO_GUITAR_TRACK *tp, unsigned char difficulty, char dynamic)
{
	unsigned long ctr;
	unsigned char lowest, highest, range;

	if(!tp)
	{	//Invalid parameters
		eof_fret_range_tolerances = NULL;
		return;
	}

	if(eof_fret_range_tolerances)
	{	//If this array was previously built
		free(eof_fret_range_tolerances);	//Release its memory as it will be rebuilt to suit this track difficulty
	}
	eof_fret_range_tolerances = malloc((size_t)tp->numfrets + 1);	//Allocate memory for an array large enough to specify the fret hand's range for each fret starting the numbering with 1 instead of 0
	if(!eof_fret_range_tolerances)
	{	//Couldn't allocate memory
		return;
	}

	//Initialize the tolerance of each fret to 4
	memset(eof_fret_range_tolerances, 4, (size_t)tp->numfrets + 1);	//Set a default range of 4 frets for the entire guitar neck

	if(!dynamic)
	{	//If the tolerances aren't being built from the specified track, return with all tolerances initialized to 4
		return;
	}

	//Find the range of each fret as per the notes in the track
	for(ctr = 0; ctr < tp->notes; ctr++)
	{	//For each note in the specified track
		if(tp->note[ctr]->type == difficulty)
		{	//If it is in the specified difficulty
			lowest = eof_pro_guitar_note_lowest_fret(tp, ctr);	//Get the lowest and highest fret used by this note
			highest = eof_pro_guitar_note_highest_fret(tp, ctr);

			range = highest - lowest + 1;	//Determine the range used by this note, and assume it's range of frets is playable since that's how the chord is defined for play
			if(eof_fret_range_tolerances[lowest] < range)
			{	//If the current fret range for this fret position is lower than this chord uses
				eof_fret_range_tolerances[lowest] = range;	//Update the range to reflect that this chord is playable
			}
		}
	}

	//Update the array so that any range that is valid for a lower fret number is valid for a higher fret number
	range = eof_fret_range_tolerances[1];	//Start with the range of fret 1
	for(ctr = 2; ctr < tp->numfrets + 1; ctr++)
	{	//For each of the frets in the array, starting with the second
		if(eof_fret_range_tolerances[ctr] < range)
		{	//If this fret's defined range is lower than a lower (longer fret)
			eof_fret_range_tolerances[ctr] = range;	//Update it
		}
		range = eof_fret_range_tolerances[ctr];	//Track the current fret's range
	}
}

unsigned char eof_pro_guitar_track_find_effective_fret_hand_position(EOF_PRO_GUITAR_TRACK *tp, unsigned char difficulty, unsigned long position)
{
	unsigned long ctr;
	unsigned char effective = 0;

	if(!tp)
		return 0;

	for(ctr = 0; ctr < tp->handpositions; ctr++)
	{	//For each hand position in the track
		if(tp->handposition[ctr].difficulty == difficulty)
		{	//If the hand position is in the specified difficulty
			if(tp->handposition[ctr].start_pos <= position)
			{	//If the hand position is at or before the specified timestamp
				effective = tp->handposition[ctr].end_pos;	//Track its fret number
			}
			else
			{	//This hand position is beyond the specified timestamp
				return effective;	//Return the last hand position that was found (if any) in this track difficulty
			}
		}
	}

	return effective;	//Return the last hand position definition that was found (if any) in this track difficulty
}

EOF_PHRASE_SECTION *eof_pro_guitar_track_find_effective_fret_hand_position_definition(EOF_PRO_GUITAR_TRACK *tp, unsigned char difficulty, unsigned long position, unsigned long *index, unsigned long *diffindex, char function)
{
	unsigned long ctr, ctr2 = 0;
	EOF_PHRASE_SECTION *ptr = NULL;

	if(!tp)
		return 0;	//Invalid parameters

	for(ctr = 0; ctr < tp->handpositions; ctr++)
	{	//For each hand position in the track
		if(tp->handposition[ctr].difficulty == difficulty)
		{	//If the hand position is in the specified difficulty
			if(tp->handposition[ctr].start_pos <= position)
			{	//If the hand position is at or before the specified timestamp
				if(function && (tp->handposition[ctr].start_pos != position))
				{	//If the calling function only wanted to identify the fret hand position exactly at the target time stamp
					continue;	//Skip to next position if this one isn't at the target time
				}
				ptr = &tp->handposition[ctr];	//Store its address
				if(index)
				{	//If index isn't NULL
					*index = ctr;		//Track this fret hand position definition number
				}
				if(diffindex)
				{	//If diffindex isn't NULL
					*diffindex = ctr2;	//Track the position number this is within the target difficulty
				}
			}
			else
			{	//This hand position is beyond the specified timestamp
				return ptr;		//Return any hand position address that has been found
			}
			ctr2++;
		}
	}

	return ptr;	//Return any hand position address that has been found
}

char *eof_rs_section_text_valid(char *string)
{
	unsigned long ctr;

	if(!string)
		return NULL;	//Return error

	for(ctr = 0; ctr < EOF_NUM_RS_PREDEFINED_SECTIONS; ctr++)
	{	//For each pre-defined Rocksmith section
		if(!ustricmp(eof_rs_predefined_sections[ctr].string, string) || !ustricmp(eof_rs_predefined_sections[ctr].displayname, string))
		{	//If the string matches this Rocksmith section entry's native or display name
			return eof_rs_predefined_sections[ctr].string;	//Return match
		}
	}
	return NULL;	//Return no match
}

int eof_rs_event_text_valid(char *string)
{
	unsigned long ctr;

	if(!string)
		return 0;	//Return error

	for(ctr = 0; ctr < EOF_NUM_RS_PREDEFINED_EVENTS; ctr++)
	{	//For each pre-defined Rocksmith event
		if(!ustrcmp(eof_rs_predefined_events[ctr].string, string))
		{	//If the string matches this Rocksmith event entry
			return 1;	//Return match
		}
	}
	return 0;	//Return no match
}

unsigned long eof_get_rs_section_instance_number(EOF_SONG *sp, unsigned long track, unsigned long event)
{
	unsigned long ctr, count = 1;

	if(!sp || (event >= sp->text_events) || !(sp->text_event[event]->flags & EOF_EVENT_FLAG_RS_SECTION))
		return 0;	//If the parameters are invalid, or the specified text event is not a Rocksmith section
	if(sp->text_event[event]->track && (sp->text_event[event]->track != track))
		return 0;	//If the specified event is assigned to a track other than the one specified

	for(ctr = 0; ctr < event; ctr++)
	{	//For each text event in the chart that is before the specified event
		if(sp->text_event[ctr]->flags & EOF_EVENT_FLAG_RS_SECTION)
		{	//If the text event is marked as a Rocksmith section
			if(!sp->text_event[ctr]->track || (sp->text_event[ctr]->track == track))
			{	//If the text event is not track specific or is assigned to the specified track
				if(!ustrcmp(sp->text_event[ctr]->text, sp->text_event[event]->text))
				{	//If the text event's text matches
					count++;	//Increment the instance counter
				}
			}
		}
	}

	return count;
}

void eof_get_rocksmith_wav_path(char *buffer, const char *parent_folder, size_t num)
{
	(void) replace_filename(buffer, parent_folder, "", (int)num - 1);	//Obtain the destination path

	if(!eof_song_loaded)
		return;	//Don't perform this action unless a chart is loaded

	//Build target WAV file name
	put_backslash(buffer);
	if(eof_song->tags->title[0] != '\0')
	{	//If the chart has a defined song title
		(void) ustrncat(buffer, eof_song->tags->title, (int)num - 1);
	}
	else
	{	//Otherwise default to "guitar"
		(void) ustrncat(buffer, "guitar", (int)num - 1);
	}
	(void) ustrncat(buffer, ".wav", (int)num - 1);
	buffer[num - 1] = '\0';	//Ensure the finalized string is terminated
}

void eof_delete_rocksmith_wav(void)
{
	char checkfn[1024] = {0};

	if(!eof_song_loaded)
		return;	//Don't perform this action unless a chart is loaded

	eof_get_rocksmith_wav_path(checkfn, eof_song_path, sizeof(checkfn));	//Build the path to the WAV file written for Rocksmith during save
	if(!exists(checkfn))
	{	//If the path based on the song title does not exist, delete guitar.wav because this is the name it will use if the song title has characters invalid for a filename
		(void) replace_filename(checkfn, eof_song_path, "guitar.wav", 1024);
	}
	if(exists(checkfn))
	{	//If the target file exists
		(void) delete_file(checkfn);
		if(exists(checkfn))
		{	//If the file still exists after the attempted deletion
			allegro_message("Warning:  Couldn't delete the Rocksmith WAV file.  Please delete it manually so it will be recreated during the next save.");
		}
	}
}

char eof_compare_time_range_with_previous_or_next_difficulty(EOF_SONG *sp, unsigned long track, unsigned long start, unsigned long stop, unsigned char diff, char compareto)
{
	unsigned long ctr2, ctr3, thispos, thispos2;
	unsigned char note_found;
	unsigned char comparediff, thisdiff, populated = 0;
	char restore_tech_view = 0;		//If tech view is in effect, it is temporarily disabled until after the secondary piano roll has been rendered

	if(!sp || (track >= sp->tracks) || (start > stop))
		return 0;	//Invalid parameters
	if(!diff && (compareto < 0))
		return 0;	//There is no difficulty before the first difficulty
	if((diff == 255) && (compareto >= 0))
		return 0;	//There is no difficulty after the last difficulty

	if(compareto < 0)
	{	//Compare the specified difficulty with the previous difficulty
		comparediff = diff - 1;
		if(((sp->track[track]->flags & EOF_TRACK_FLAG_UNLIMITED_DIFFS) == 0) && (comparediff == 4))
		{	//If the track is using the traditional 5 difficulty system and the difficulty previous to the being examined is the BRE difficulty
			comparediff--;	//Compare to the previous difficulty
		}
	}
	else
	{	//Compare the specified difficulty with the next difficulty
		comparediff = diff + 1;
		if(((sp->track[track]->flags & EOF_TRACK_FLAG_UNLIMITED_DIFFS) == 0) && (comparediff == 4))
		{	//If the track is using the traditional 5 difficulty system and the difficulty next to the being examined is the BRE difficulty
			comparediff++;	//Compare to the next difficulty
		}
	}

	restore_tech_view = eof_menu_track_get_tech_view_state(sp, track);
	eof_menu_track_set_tech_view_state(sp, track, 0);	//Disable tech view if applicable

	//First pass:  Compare notes in the specified difficulty with those in the comparing difficulty
	for(ctr2 = 0; ctr2 < eof_get_track_size(sp, track); ctr2++)
	{	//For each note in the track
		thispos = eof_get_note_pos(sp, track, ctr2);	//Get this note's position
		if(thispos > stop)
		{	//If this note (and all remaining notes, since they are expected to remain sorted) is beyond the specified range, break from loop
			break;
		}

		if(thispos >= start)
		{	//If this note is at or after the start of the specified range, check its difficulty
			thisdiff = eof_get_note_type(sp, track, ctr2);	//Get this note's difficulty
			if(thisdiff == diff)
			{	//If this note is in the difficulty being examined
				populated = 1;	//Track that there was at least 1 note in the specified difficulty of the phrase
				//Compare this note to the one at the same position in the comparing difficulty, if there is one
				note_found = 0;	//This condition will be set if this note is found in the comparing difficulty
				if(compareto < 0)
				{	//If comparing to the previous difficulty, parse notes backwards
					for(ctr3 = ctr2; ctr3 > 0; ctr3--)
					{	//For each previous note in the track, for performance reasons going backward from the one being examined in the outer loop (which has a higher note number when sorted)
						thispos2 = eof_get_note_pos(sp, track, ctr3 - 1);	//Get this note's position
						thisdiff = eof_get_note_type(sp, track, ctr3 - 1);	//Get this note's difficulty
						if(thispos2 < thispos)
						{	//If this note and all previous ones are before the one being examined in the outer loop
							break;
						}
						if((thispos == thispos2) && (thisdiff == comparediff))
						{	//If this note is at the same position and one difficulty lower than the one being examined in the outer loop
							note_found = 1;	//Track that a note at the same position was found in the previous difficulty
							if(eof_note_compare(sp, track, ctr2, track, ctr3 - 1, 1))
							{	//If the two notes don't match (including lengths and flags)
								eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view if applicable
								return 1;	//Return difference found
							}
							break;
						}
					}
				}
				else
				{	//Comparing to the next difficulty, parse notes forwards
					for(ctr3 = ctr2 + 1; ctr3 < eof_get_track_size(sp, track); ctr3++)
					{	//For each remaining note in the track
						thispos2 = eof_get_note_pos(sp, track, ctr3);	//Get this note's position
						thisdiff = eof_get_note_type(sp, track, ctr3);	//Get this note's difficulty
						if(thispos2 > thispos)
						{	//If this note and all subsequent ones are after the one being examined in the outer loop
							break;
						}
						if((thispos == thispos2) && (thisdiff == comparediff))
						{	//If this note is at the same position and one difficulty lower than the one being examined in the outer loop
							note_found = 1;	//Track that a note at the same position was found in the previous difficulty
							if(eof_note_compare(sp, track, ctr2, track, ctr3 - 1, 1))
							{	//If the two notes don't match (including lengths and flags)
								eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view if applicable
								return 1;	//Return difference found
							}
							break;
						}
					}
				}
				if(!note_found)
				{	//If this note has no note at the same position in the previous difficulty
					eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view if applicable
					return 1;	//Return difference found
				}
			}//If this note is in the difficulty being examined
		}//If this note is at or after the start of the specified range, check its difficulty
	}//For each note in the track

	if(!populated)
	{	//If no notes were contained within the time range in the specified difficulty
		eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view if applicable
		return -1;	//Return empty time range
	}

	//Second pass:  Compare notes in the comparing difficulty with those in the specified difficulty
	for(ctr2 = 0; ctr2 < eof_get_track_size(sp, track); ctr2++)
	{	//For each note in the track
		thispos = eof_get_note_pos(sp, track, ctr2);	//Get this note's position
		if(thispos > stop)
		{	//If this note (and all remaining notes, since they are expected to remain sorted) is beyond the specified range, break from loop
			break;
		}

		if(thispos >= start)
		{	//If this note is at or after the start of the specified range, check its difficulty
			thisdiff = eof_get_note_type(sp, track, ctr2);	//Get this note's difficulty
			if(thisdiff == comparediff)
			{	//If this note is in the difficulty being compared
				//Compare this note to the one at the same position in the examined difficulty, if there is one
				note_found = 0;	//This condition will be set if this note is found in the examined difficulty
				if(compareto < 0)
				{	//If comparing the comparing difficulty with the next, parse notes forwards
					for(ctr3 = ctr2 + 1; ctr3 < eof_get_track_size(sp, track); ctr3++)
					{	//For each remaining note in the track
						thispos2 = eof_get_note_pos(sp, track, ctr3);	//Get this note's position
						thisdiff = eof_get_note_type(sp, track, ctr3);	//Get this note's difficulty
						if(thispos2 > thispos)
						{	//If this note and all subsequent ones are after the one being examined in the outer loop
							break;
						}
						if((thispos == thispos2) && (thisdiff == diff))
						{	//If this note is in the specified difficulty and at the same position as the one being examined in the outer loop
							note_found = 1;	//Track that a note at the same position was found in the previous difficulty
							if(eof_note_compare(sp, track, ctr2, track, ctr3 - 1, 1))
							{	//If the two notes don't match (including lengths and flags)
								eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view if applicable
								return 1;	//Return difference found
							}
							break;
						}
					}
				}
				else
				{	//Comparing the comparing difficulty with the previous, parse notes backwards
					for(ctr3 = ctr2; ctr3 > 0; ctr3--)
					{	//For each previous note in the track, for performance reasons going backward from the one being examined in the outer loop (which has a higher note number when sorted)
						thispos2 = eof_get_note_pos(sp, track, ctr3 - 1);	//Get this note's position
						thisdiff = eof_get_note_type(sp, track, ctr3 - 1);	//Get this note's difficulty
						if(thispos2 < thispos)
						{	//If this note and all previous ones are before the one being examined in the outer loop
							break;
						}
						if((thispos == thispos2) && (thisdiff == diff))
						{	//If this note is in the specified difficulty and at the same position as the one being examined in the outer loop
							note_found = 1;	//Track that a note at the same position was found in the previous difficulty
							if(eof_note_compare(sp, track, ctr2, track, ctr3 - 1, 1))
							{	//If the two notes don't match (including lengths and flags)
								eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view if applicable
								return 1;	//Return difference found
							}
							break;
						}
					}
				}
				if(!note_found)
				{	//If this note has no note at the same position in the previous difficulty
					eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view if applicable
					return 1;	//Return difference found
				}
			}//If this note is in the difficulty being compared
		}//If this note is at or after the start of the specified range, check its difficulty
	}//For each note in the track
	eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view if applicable

	return 0;	//Return no difference found
}

unsigned char eof_find_fully_leveled_rs_difficulty_in_time_range(EOF_SONG *sp, unsigned long track, unsigned long start, unsigned long stop, unsigned char relative)
{
	unsigned char reldiff, fullyleveleddiff = 0;
	unsigned long ctr;
	char restore_tech_view = 0;		//If tech view is in effect, it is temporarily disabled until after the secondary piano roll has been rendered

	if(!sp || (track >= sp->tracks) || (start > stop))
		return 0;	//Invalid parameters

	restore_tech_view = eof_menu_track_get_tech_view_state(sp, track);
	eof_menu_track_set_tech_view_state(sp, track, 0);	//Disable tech view if applicable
	(void) eof_detect_difficulties(sp, track);	//Update eof_track_diff_populated_status[] to reflect all populated difficulties for this track
	if((sp->track[track]->flags & EOF_TRACK_FLAG_UNLIMITED_DIFFS) == 0)
	{	//If the track is using the traditional 5 difficulty system
		eof_track_diff_populated_status[4] = 0;	//Ensure that the BRE difficulty is ignored
	}
	for(ctr = 0; ctr < 256; ctr++)
	{	//For each of the possible difficulties
		if(eof_track_diff_populated_status[ctr])
		{	//If this difficulty isn't empty
			if(eof_compare_time_range_with_previous_or_next_difficulty(sp, track, start, stop, ctr, -1) > 0)
			{	//If this difficulty isn't empty and had more notes than the previous or any of the notes within the phrase were different than those in the previous difficulty
				fullyleveleddiff = ctr;			//Track the lowest difficulty number that represents the fully leveled time range of notes
			}
		}
	}//For each of the possible difficulties
	eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view if applicable

	if(!relative)
	{	//If the resulting difficulty number is not to be converted to Rocksmith's relative difficulty number system
		return fullyleveleddiff;
	}

	//Remap fullyleveleddiff to the relative difficulty
	for(ctr = 0, reldiff = 0; ctr < 256; ctr++)
	{	//For each of the possible difficulties
		if(((sp->track[track]->flags & EOF_TRACK_FLAG_UNLIMITED_DIFFS) == 0) && (ctr == 4))
		{	//If the track is using the traditional 5 difficulty system and the difficulty being examined is the BRE difficulty
			continue;	//Don't process the BRE difficulty, it will not be exported
		}
		if(fullyleveleddiff == ctr)
		{	//If the corresponding relative difficulty has been found
			return reldiff;	//Return it
		}
		if(eof_track_diff_populated_status[ctr])
		{	//If the track is populated
			reldiff++;
		}
	}
	return 0;	//Error
}

int eof_check_rs_sections_have_phrases(EOF_SONG *sp, unsigned long track)
{
	unsigned long ctr, old_text_events;
	char user_prompted = 0;

	if(!sp || (track >= sp->tracks))
		return 1;	//Invalid parameters
	if(!eof_get_track_size(sp, track))
		return 0;	//Empty track

	eof_process_beat_statistics(sp, track);	//Cache section name information into the beat structures (from the perspective of the specified track)
	for(ctr = 0; ctr < sp->beats; ctr++)
	{	//For each beat
		if(sp->beat[ctr]->contained_rs_section_event >= 0)
		{	//If this beat contains a RS section
			if(sp->beat[ctr]->contained_section_event < 0)
			{	//But it doesn't contain a RS phrase
				unsigned char original_eof_2d_render_top_option = eof_2d_render_top_option;	//Back up the user's preference

				eof_selected_beat = ctr;					//Change the selected beat
				if(eof_2d_render_top_option != 36)
				{	//If the piano roll isn't already displaying both RS sections and phrases
					eof_2d_render_top_option = 35;					//Change the user preference to render RS sections at the top of the piano roll
				}
				eof_seek_and_render_position(track, eof_note_type, sp->beat[ctr]->pos);	//Render the track so the user can see where the correction needs to be made, along with the RS section in question
				eof_clear_input();
				if(!user_prompted && alert("At least one Rocksmith section doesn't have a Rocksmith phrase at the same position.", "This can cause the chart's sections to work incorrectly", "Would you like to place Rocksmith phrases to correct this?", "&Yes", "&No", 'y', 'n') != 1)
				{	//If the user hasn't already answered this prompt, and doesn't opt to correct the issue
					eof_2d_render_top_option = original_eof_2d_render_top_option;	//Restore the user's preference
					return 2;	//Return user cancellation
				}
				eof_2d_render_top_option = original_eof_2d_render_top_option;	//Restore the user's preference
				user_prompted = 1;

				while(1)
				{
					old_text_events = sp->text_events;				//Remember how many text events there were before launching dialog
					(void) eof_rocksmith_phrase_dialog_add();			//Launch dialog for user to add a Rocksmith phrase
					if(old_text_events == sp->text_events)
					{	//If the number of text events defined in the chart didn't change, the user canceled
						return 2;	//Return user cancellation
					}
					eof_process_beat_statistics(sp, track);	//Rebuild beat statistics to check if user added a Rocksmith phrase
					if(sp->beat[ctr]->contained_section_event < 0)
					{	//If user added a text event, but it wasn't a Rocksmith phrase
						eof_clear_input();
						if(alert("You didn't add a Rocksmith phrase.", NULL, "Do you want to continue adding RS phrases for RS sections that are missing them?", "&Yes", "&No", 'y', 'n') != 1)
						{	//If the user doesn't opt to finish correcting the issue
							return 2;	//Return user cancellation
						}
					}
					else
					{	//User added the missing RS phrase
						break;
					}
				}
			}
		}
	}

	return 0;	//Return completion
}

unsigned long eof_find_effective_rs_phrase(unsigned long position)
{
	unsigned long ctr, effective = 0;

	for(ctr = 0; ctr < eof_song->beats; ctr++)
	{	//For each beat in the chart
		if(eof_song->beat[ctr]->contained_section_event >= 0)
		{	//If this beat has a section event (RS phrase)
			if(eof_song->beat[ctr]->pos <= position)
			{	//If this phrase is at or before the position being checked
				effective++;
			}
			else
			{	//If this phrase is after the position being checked
				break;
			}
		}
	}

	if(effective)
	{	//If the specified phrase was found
		return effective - 1;	//Return its number
	}

	return 0;	//Return no phrase found
}

int eof_time_range_is_populated(EOF_SONG *sp, unsigned long track, unsigned long start, unsigned long stop, unsigned char diff)
{
	unsigned long ctr2, thispos;
	unsigned char thisdiff;
	char restore_tech_view = 0;		//If tech view is in effect, it is temporarily disabled until after the secondary piano roll has been rendered
	int retval = 0;

	if(!sp || (track >= sp->tracks) || (start > stop))
		return 0;	//Invalid parameters

	restore_tech_view = eof_menu_track_get_tech_view_state(sp, track);
	eof_menu_track_set_tech_view_state(sp, track, 0);	//Disable tech view if applicable

	for(ctr2 = 0; ctr2 < eof_get_track_size(sp, track); ctr2++)
	{	//For each note in the track
		thispos = eof_get_note_pos(sp, track, ctr2);	//Get this note's position
		if(thispos > stop)
		{	//If this note (and all remaining notes, since they are expected to remain sorted) is beyond the specified range, break from loop
			break;
		}
		if(thispos >= start)
		{	//If this note is at or after the start of the specified range, check its difficulty
			thisdiff = eof_get_note_type(sp, track, ctr2);	//Get this note's difficulty
			if(thisdiff == diff)
			{	//If this note is in the difficulty being examined
				retval = 1;	//Set the return value to indicate the specified time range of the specified difficulty is populated
				break;
			}
		}
	}
	eof_menu_track_set_tech_view_state(sp, track, restore_tech_view);	//Re-enable tech view if applicable

	return retval;	//Return not populated
}

int eof_note_has_high_chord_density(EOF_SONG *sp, unsigned long track, unsigned long note, char target)
{
	long prev;
	unsigned long handshapestatus;
	EOF_PRO_GUITAR_TRACK *tp;
	EOF_RS_TECHNIQUES tech = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 ,0};

	if((sp == NULL) || (track >= sp->tracks))
		return 0;	//Error

	if(sp->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 0;	//Note is not a pro guitar/bass note
	if(eof_note_count_rs_lanes(sp, track, note, target) < 2)
		return 0;	//Note is not a chord from the perspective of the target game (RS1 ignored string muted notes)

	if(eof_get_note_flags(sp, track, note) & EOF_NOTE_FLAG_CRAZY)
		return 0;	//Chord is marked with crazy status, which forces it to export as low density

	prev = eof_track_fixup_previous_note(sp, track, note);
	if(prev < 0)
		return 0;	//No earlier note

	tp = sp->pro_guitar_track[sp->track[track]->tracknum];
	if(target == 2)
	{	//Additional checks for Rocksmith 2
		if(eof_get_rs_techniques(sp, track, note, 0, NULL, 2, 1))
		{	//If this chord uses any techniques that require writing a low density chord
			return 0;
		}

		(void) eof_get_rs_techniques(sp, track, prev, 0, &tech, 2, 0);	//Get techniques used by the previous note
		if((eof_note_count_rs_lanes(sp, track, note, target) > 1) && (tech.slideto >= 0))
		{	//If the previous note was a chord slide
			return 0;
		}

		if(eof_is_string_muted(sp, track, note) && (eof_pro_guitar_note_fingering_valid(tp, note, 1) == 2) && (eof_note_count_rs_lanes(sp, track, prev, target) > 1))
		{	//If this chord is entirely string muted, has no fingering defined and the previous note was a chord
			return 2;	//Export this chord as high density so that it is added to the same hand shape tag (return 2 to signal to RS2 export that this chord should be included in the previous chord's handshape)
		}
	}

	handshapestatus = eof_note_number_within_handshape(sp, track, note);
	if(handshapestatus)
	{	//If the chord is in a handshape
		if(handshapestatus == 1)
			return 0;	//Chord is the first note in any handshape
	}

	if(eof_note_compare(sp, track, note, track, prev, 0))
		return 0;	//Chord does not match the previous note (ignoring note flags and lengths)

	if(eof_get_note_pos(sp, track, note) > eof_get_note_pos(sp, track, prev) + eof_get_note_length(sp, track, prev) + eof_chord_density_threshold)
		return 0;	//Chord is not within the configured threshold distance from the previous note

	return 1;	//All criteria passed, chord is high density
}

unsigned long eof_note_number_within_handshape(EOF_SONG *sp, unsigned long track, unsigned long note)
{
	unsigned long ctr, ctr2, count;
	EOF_PRO_GUITAR_TRACK *tp;

	if((sp == NULL) || (track >= sp->tracks))
		return 0;	//Invalid parameters
	tp = sp->pro_guitar_track[sp->track[track]->tracknum];
	if(note >= tp->notes)
		return 0;	//Invalid parameters

	for(ctr = 0; ctr < tp->arpeggios; ctr++)
	{	//For each arpeggio/handshape phrase
		if(tp->arpeggio[ctr].flags & EOF_RS_ARP_HANDSHAPE)
		{	//If this is a handshape phrase
			for(ctr2 = 0, count = 0; ctr2 < tp->notes; ctr2++)
			{	//For each note in the track
				if(tp->note[ctr2]->pos > tp->arpeggio[ctr].end_pos)
				{	//If this and all remaining notes are beyond the end of the handshape phrase being checked
					break;	//Break from inner loop
				}
				if(tp->note[ctr2]->pos >= tp->arpeggio[ctr].start_pos)
				{	//If this note is in the handshape phrase
					count++;		//Track how many such notes were in the phrase
					if(ctr2 == note)
					{	//If this note is the one specified by the calling function
						return count;	//Return which note instance in the handshape this note is
					}
				}
			}
		}
	}

	return 0;	//The note was not found in any handshape phrases
}

int eof_enforce_rs_phrase_begin_with_fret_hand_position(EOF_SONG *sp, unsigned long track, unsigned char diff, unsigned long startpos, unsigned long endpos, char *undo_made, char check_only)
{
	unsigned long ctr, firstnotepos = 0, tracknum;
	int found = 0;
	unsigned char position;
	EOF_PRO_GUITAR_TRACK *tp;

	if(!sp || (track >= sp->tracks) || (sp->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return 0;	//Invalid parameters
	tracknum = sp->track[track]->tracknum;
	tp = sp->pro_guitar_track[tracknum];

	//Find the position of the first note in the phrase
	found = 0;	//Reset this condition
	for(ctr = 0; ctr < tp->notes; ctr++)
	{	//For each note in the track
		if(tp->note[ctr]->type == diff)
		{	//If the note is in the active difficulty
			if((tp->note[ctr]->pos >= startpos) && (tp->note[ctr]->pos <= endpos))
			{	//If this is the first note in the phrase
				firstnotepos = tp->note[ctr]->pos;	//Note the position
				found = 1;
				break;
			}
		}
	}

	//Determine if the necessary fret hand position was set between the start of the phrase and the first note
	if(found)
	{	//If the phrase has a note in it
		found = 0;	//Reset this condition
		for(ctr = 0; ctr < tp->handpositions; ctr++)
		{	//For each hand position in the track
			if(tp->handposition[ctr].difficulty == diff)
			{	//If the hand position is in the active difficulty
				if((tp->handposition[ctr].start_pos >= startpos) && (tp->handposition[ctr].start_pos <= firstnotepos))
				{	//If the hand position is defined anywhere between the start of the phrase and the start of the first note in that phrase
					found = 1;
					break;
				}
			}
		}
		if(!found && !check_only)
		{	//If a hand position needs to be added to the difficulty, and the calling function intends for the position to be added
			position = eof_pro_guitar_track_find_effective_fret_hand_position(tp, diff, firstnotepos);
			if(position)
			{	//If a fret hand position was is in effect (placed anywhere earlier in the difficulty)
				if(undo_made && !(*undo_made))
				{	//If an undo state needs to be made
					eof_prepare_undo(EOF_UNDO_TYPE_NONE);
					*undo_made = 1;
				}
				//Place that fret hand position for the active difficulty
				(void) eof_track_add_section(sp, track, EOF_FRET_HAND_POS_SECTION, diff, startpos, position, 0, NULL);
				eof_pro_guitar_track_sort_fret_hand_positions(tp);	//Sort the positions
			}
		}
	}

	return found;
}

int eof_lookup_chord_shape(EOF_PRO_GUITAR_NOTE *np, unsigned long *shapenum, unsigned long skipctr)
{
	unsigned long ctr, ctr2, bitmask;
	EOF_PRO_GUITAR_NOTE template;
	unsigned char lowest = 0;	//Tracks the lowest fret value in the note
	char nonmatch;

	if(!np)
		return 0;	//Invalid parameter

	//Prepare a copy of the target note that ignores open and muted strings and moves the note's shape to the lowest fret position and so that its lowest fretted string is lane 1
	//This will be used to easily compare against the chord shape definitions, which have been transformed the same way
	template = *np;
	for(ctr = 0, ctr2 = 0, bitmask = 1; ctr < 6; ctr++, bitmask <<= 1)
	{	//For each of the 6 supported strings
		if(template.note & bitmask)
		{	//If this string is used
			if((template.frets[ctr] == 0) || (template.frets[ctr] & 0x80))
			{	//If this string is played open or muted
				template.note &= ~bitmask;	//Clear this string on the template note
			}
			else
			{	//Otherwise the string is fretted
				ctr2++;	//Count how many strings are fretted
				if(!lowest || (template.frets[ctr] < lowest))
				{
					lowest = template.frets[ctr];	//Track the lowest fret value in the note
				}
			}
		}
	}
	if(ctr2 < 1)
	{	//If no strings are fretted, there cannot be a chord shape
		return 0;
	}
	for(ctr = 0; ctr < 6; ctr++)
	{	//For each of the 6 supported strings, lower the fretted notes by the same amount so shape is moved to fret 1
		if(template.frets[ctr] != 0)
		{	//If this note is fretted
			template.frets[ctr] -= (lowest - 1);	//Transpose the shape to the first fret
		}
	}
	while((template.note & 1) == 0)
	{	//Until the shape has been moved to occupy the lowest string
		for(ctr = 0; ctr < 5; ctr++)
		{	//For each of the first 5 supported strings
			template.frets[ctr] = template.frets[ctr + 1];		//Transpose the fretted note down one string
			template.finger[ctr] = template.finger[ctr + 1];	//Transpose the finger definition for the string
		}
		template.frets[5] = 0;
		template.finger[5] = 0;
		template.note >>= 1;	//Transpose the note mask
	}

	//Look for a chord shape definition that matches the template on any string position
	for(ctr = 0; ctr < num_eof_chord_shapes; ctr++)
	{	//For each chord shape definition
		//Compare the working copy note against the chord shape definition
		if(template.note == eof_chord_shape[ctr].note)
		{	//If the note bitmask matches that of the chord shape definition
			nonmatch = 0;	//Reset this status
			for(ctr2 = 0, bitmask = 1; ctr2 < 6; ctr2++, bitmask <<= 1)
			{	//For each of the 6 supported strings
				if(template.note & bitmask)
				{	//If this string is fretted
					if(template.frets[ctr2] != eof_chord_shape[ctr].frets[ctr2])
					{	//If the fret value doesn't match the chord shape definition
						nonmatch = 1;
						break;
					}
				}
			}
			if(!nonmatch)
			{	//If the note matches the chord shape definition
				if(!skipctr)
				{	//If no more matches were to be skipped before returning the match
					if(shapenum)
					{	//If calling function wanted to receive the matching definition number
						*shapenum = ctr;
					}
					return 1;	//Return match found
				}
				skipctr--;
			}
		}//If the note bitmask matches that of the chord shape definition
	}//For each chord shape definition

	return 0;	//No chord shape match found
}

void eof_apply_chord_shape_definition(EOF_PRO_GUITAR_NOTE *np, unsigned long shapenum)
{
	unsigned long ctr, transpose, bitmask;

	if(!np || (shapenum >= num_eof_chord_shapes))
		return;	//Invalid parameters

	//Find the lowest fretted string in the target note, this is where the chord shape begins
	//The specified chord shape definition will be transposed the appropriate number of strings
	for(transpose = 0, bitmask = 1; transpose < 6; transpose++, bitmask <<= 1)
	{	//For each of the 6 supported strings
		np->finger[transpose] = 0;	//Erase any existing fingering for this string
		if(np->note & bitmask)
		{	//If this string is used
			if((np->frets[transpose] != 0) && !(np->frets[transpose] & 0x80))
			{	//If this string is not played open or muted
				break;	//This string has the lowest fretted note
			}
		}
	}
	if(transpose >= 6)
		return;	//Error

	//Apply the chord shape's fingering, transosing the definition by the number of strings found necessary in the prior loop
	for(ctr = 0; ctr + transpose < 6; ctr++)
	{	//For the remainder of the 6 strings after any needed transposing is accounted for
		np->finger[ctr + transpose] = eof_chord_shape[shapenum].finger[ctr];	//Apply the defined fingering
	}
}

void eof_load_chord_shape_definitions(char *fn)
{
	char *buffer = NULL;	//Will be an array large enough to hold the largest line of text from input file
	PACKFILE *inf = NULL;
	size_t maxlinelength, length;
	unsigned long linectr = 1, ctr, bitmask;
	char finger[8] = {0};
	char frets[8] = {0};
	char name[51] = {0};
	unsigned char note = 0, lowestfret;
	char error = 0;

	eof_log("\tImporting chord shape definitions", 1);
	eof_log("eof_load_chord_shape_definitions() entered", 1);

	if(!fn)
		return;	//Invalid parameter

	inf = pack_fopen(fn, "rt");	//Open file in text mode
	if(!inf)
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tError loading:  Cannot open input chord shape definitions file:  \"%s\"", strerror(errno));	//Get the Operating System's reason for the failure
		eof_log(eof_log_string, 1);
		return;
	}

	//Allocate memory buffers large enough to hold any line in this file
	maxlinelength = (size_t)FindLongestLineLength_ALLEGRO(fn, 0);
	if(!maxlinelength)
	{
		eof_log("\tError finding the largest line in the file.  Aborting", 1);
		(void) pack_fclose(inf);
		return;
	}
	buffer = (char *)malloc(maxlinelength);
	if(!buffer)
	{
		eof_log("\tError allocating memory.  Aborting", 1);
		(void) pack_fclose(inf);
		return;
	}

	//Read first line of text, capping it to prevent buffer overflow
	if(!pack_fgets(buffer, (int)maxlinelength, inf))
	{	//I/O error
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Chord shape definitions import failed on line #%lu:  Unable to read from file:  \"%s\"", linectr, strerror(errno));
		eof_log(eof_log_string, 1);
		error = 1;
	}

	//Parse the contents of the file
	while(!error)
	{	//Until there was an error reading from the file
		if(num_eof_chord_shapes < EOF_MAX_CHORD_SHAPES)
		{	//If another chord shape definition can be stored
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tProcessing line #%lu", linectr);
			eof_log(eof_log_string, 2);

			//Load chord shape definition
			if(strcasestr_spec(buffer, "<chordTemplate"))
			{	//If this line contains a chord template tag (which defines a chord shape)
				if(eof_parse_chord_template(name, sizeof(name), finger, frets, &note, NULL, NULL, linectr, buffer))
				{	//If there was an error reading the chord template
					error = 1;
				}

				if(eof_note_count_colors_bitmask(note) < 2)
				{	//If not at least two strings are used in the definition
					eof_log("\t\tSkipping non chord definition", 2);
				}
				else
				{	//The chord shape is valid
					//Move the shape so that it begins on fret 1 and its lowest fretted string is lane 1
					for(ctr = 0, bitmask = 1, lowestfret = 0; ctr < 6; ctr++, bitmask <<= 1)
					{	//For each of the 6 supported strings
						if(note & bitmask)
						{	//If this string is used
							if(frets[ctr] == 0)
							{	//If this string is played open
								note &= ~bitmask;	//Clear this string from the note mask
							}
							else if(!lowestfret || (frets[ctr] < lowestfret))
							{
								lowestfret = frets[ctr];	//Track the lowest fret value in the note
							}
						}
					}
					for(ctr = 0; ctr < 6; ctr++)
					{	//For each of the 6 supported strings
						if(frets[ctr] >= lowestfret)
						{
							frets[ctr] -= (lowestfret - 1);	//Transpose any fretted strings to the first fret
						}
					}
					while((note & 1) == 0)
					{	//Until the shape has been moved to occupy the lowest string
						for(ctr = 0; ctr < 5; ctr++)
						{	//For each of the first 5 supported strings
							frets[ctr] = frets[ctr + 1];	//Transpose the fretted note down one string
							finger[ctr] = finger[ctr + 1];	//Transpose the finger definition for the string
						}
						frets[5] = 0;
						finger[5] = 0;
						note >>= 1;	//Transpose the note mask
					}

					//Add to list
					length = strlen(name);
					eof_chord_shape[num_eof_chord_shapes].name = malloc(length + 1);	//Allocate memory to store the shape name
					if(!eof_chord_shape[num_eof_chord_shapes].name)
					{
						eof_log("\tError allocating memory.  Aborting", 1);
						error = 1;
					}
					else
					{	//Memory was allocated
						memset(eof_chord_shape[num_eof_chord_shapes].name, 0, length + 1);	//Initialize memory block to 0
						strncpy(eof_chord_shape[num_eof_chord_shapes].name, name, strlen(name) + 1);
						memcpy(eof_chord_shape[num_eof_chord_shapes].finger, finger, 8);	//Store the finger array
						memcpy(eof_chord_shape[num_eof_chord_shapes].frets, frets, 8);		//Store the fret array
						eof_chord_shape[num_eof_chord_shapes].note = note;			//Store the note mask
						num_eof_chord_shapes++;
						eof_log("\t\tChord shape definition loaded", 2);
					}
				}//The chord shape is valid
			}//If this line contains a chord template tag (which defines a chord shape)
		}//If another chord shape definition can be stored
		else
		{	//The chord shape definition list is full
			error = 1;
		}

		//Use this method of checking for EOF instead of pack_feof(), otherwise the last line cannot be read and the definitions file doesn't use a closing tag that can be ignored
		if(!pack_fgets(buffer, (int)maxlinelength, inf))
		{	//If another line cannot be read from the file
			break;	//Exit loop
		}
		linectr++;	//Increment line counter
	}//Until there was an error reading from the file

	if(error)
	{	//If import did not complete successfully
		eof_destroy_shape_definitions();	//Destroy any imported shapes
	}

	//Cleanup
	(void) pack_fclose(inf);
	free(buffer);
}

void eof_destroy_shape_definitions(void)
{
	unsigned long ctr;

	for(ctr = 0; ctr < num_eof_chord_shapes; ctr++)
	{	//For each chord shape that was imported
		free(eof_chord_shape[ctr].name);	//Release the memory allocated to store the name
		eof_chord_shape[ctr].name = NULL;
	}
	num_eof_chord_shapes = 0;
}

unsigned long eof_get_highest_fret_in_time_range(EOF_SONG *sp, unsigned long track, unsigned char difficulty, unsigned long start, unsigned long stop)
{
	unsigned long highest = 0, temp, ctr, tracknum;
	EOF_PRO_GUITAR_TRACK *tp;

	if(!sp || (track >= sp->tracks) || (sp->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT) || (stop < start))
		return 0;	//Invalid parameters

	tracknum = sp->track[track]->tracknum;
	tp = sp->pro_guitar_track[tracknum];
	for(ctr = 0; ctr < tp->notes; ctr++)
	{	//For each note in the track
		if((tp->note[ctr]->pos >= start) && (tp->note[ctr]->pos <= stop))
		{	//If the note is within the specified time range
			if(tp->note[ctr]->type == difficulty)
			{	//If the note is in the specified track difficulty
				temp = eof_get_highest_fret_value(sp, track, ctr);	//Determine its highest used fret
				if(temp > highest)
				{	//If it's the highest fret encountered so far
					highest = temp;	//Track this value
				}
			}
		}
		else if(tp->note[ctr]->pos > stop)
		{	//If this and all remaining notes are beyond the specified time range
			break;
		}
	}

	return highest;
}

unsigned long eof_get_rs_techniques(EOF_SONG *sp, unsigned long track, unsigned long notenum, unsigned long stringnum, EOF_RS_TECHNIQUES *ptr, char target, char checktechnotes)
{
	unsigned long tracknum, flags, eflags, techflags = 0, techeflags = 0, technote_num = 0, ctr, bitmask, notepos, stop_tech_note_position = 0;
	long techslideto = -1, techunpitchedslideto = -1;	//These will track the first slide
	EOF_PRO_GUITAR_TRACK *tp;
	unsigned char lowestfret;
	long fret, slidediff = 0, unpitchedslidediff = 0;
	char techbends = 0, thistechbends, has_stop = 0;

	if((sp == NULL) || (track >= sp->tracks) || (sp->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return 0;	//Invalid parameters
	tracknum = sp->track[track]->tracknum;
	tp = sp->pro_guitar_track[tracknum];
	if((notenum >= tp->notes) || (stringnum > 5))
		return 0;	//Invalid parameter

	notepos = eof_get_note_pos(sp, track, notenum);
	flags = eof_get_note_flags(sp, track, notenum);
	eflags = eof_get_note_eflags(sp, track, notenum);
	bitmask = 1 << stringnum;
	if(tp->note[notenum]->frets[stringnum] != 0xFF)
	{	//If this gem has a defined fret value
		fret = tp->note[notenum]->frets[stringnum] & 0x7F;	//Get the fret value for this string (mask out the muting bit)
	}
	else
	{
		fret = 0;
	}
	lowestfret = eof_pro_guitar_note_lowest_fret(tp, notenum);	//Determine the lowest used fret in this note

	//If applicable, track the techniques for any tech notes that affect the specified string of the note
	if(checktechnotes && eof_pro_guitar_note_bitmask_has_tech_note(tp, notenum, bitmask, &technote_num))
	{	//If tech notes are to be taken into account and the specified string of the note has at least one tech note applied to it
		unsigned long thistechflags, notelen;
		long nextnote;

		notelen = eof_get_note_length(sp, track, notenum);
		nextnote = eof_fixup_next_pro_guitar_note(tp, notenum);
		if(nextnote > 0)
		{	//If there was a next note
			if(notepos + notelen == tp->pgnote[nextnote]->pos)
			{	//And this note extends all the way to it with no gap in between (this note has linkNext status)
				notelen--;	//Shorten the effective note length to ensure that a tech note at the next note's position is detected as affecting that note instead of this one
			}
		}
		for(ctr = technote_num; ctr < tp->technotes; ctr++)
		{	//For all tech notes, starting with the first one that applies to this note
			if(tp->technote[ctr]->pos > notepos + tp->note[notenum]->length)
			{	//If this tech note (and all those that follow) are after the end position of this note
				break;	//Break from loop, no more overlapping notes will be found
			}
			if((tp->technote[ctr]->type == tp->pgnote[notenum]->type) && (bitmask & tp->technote[ctr]->note))
			{	//If the tech note is in the same difficulty as the specified note and uses the same string
				if((tp->technote[ctr]->pos >= notepos) && (tp->technote[ctr]->pos <= notepos + tp->note[notenum]->length))
				{	//If this tech note overlaps with the specified note
					thistechflags = tp->technote[ctr]->flags;
					techflags |= thistechflags;
					techeflags |= tp->technote[ctr]->eflags;
					thistechbends = 0;	//Reset this value

					//Track the slide to position of the last tech note affecting the specified note
					if((thistechflags & EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION) == 0)
					{	//If this tech note doesn't have definitions for bend strength or the ending fret for slides
						if(thistechflags & EOF_PRO_GUITAR_NOTE_FLAG_BEND)
						{	//If this note bends
							thistechbends = 1;	//Assume a 1 half step bend
						}
						if(thistechflags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP)
						{	//If this tech note has slide up technique
							techslideto = fret + 1;		//Assume a 1 fret slide until logic is added for the author to add this information
							techslideto += tp->capo;	//Apply the capo position, so the slide ending is on the correct fret in-game
						}
						else if(thistechflags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN)
						{	//If this tech note has slide down technique
							techslideto = fret - 1;		//Assume a 1 fret slide until logic is added for the author to add this information
							techslideto += tp->capo;	//Apply the capo position, so the slide ending is on the correct fret in-game
						}
					}
					else
					{	//This tech note defines the bend strength and ending fret for slides
						if(thistechflags & EOF_PRO_GUITAR_NOTE_FLAG_BEND)
						{	//If this note bends
							if(tp->technote[ctr]->bendstrength & 0x80)
							{	//If this bend strength is defined in quarter steps
								thistechbends = ((tp->technote[ctr]->bendstrength & 0x7F) + 1) / 2;	//Obtain the defined bend strength in quarter steps (mask out the MSB), add 1 and divide by 2 to round up to the nearest number of half steps
							}
							else
							{	//The bend strength is defined in half steps
								thistechbends = tp->technote[ctr]->bendstrength;	//Obtain the defined bend strength in half steps
							}
						}
						if((thistechflags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP) || (thistechflags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN))
						{	//If this tech note slides
							lowestfret = fret;	//This individual string's note will slide regardless of what the chord's lowest fretted string is
							if(lowestfret != tp->technote[ctr]->slideend)
							{	//If the tech note's slide doesn't end at the position the lowest fretted string in the tech note is already at
								slidediff = tp->technote[ctr]->slideend - lowestfret;	//Determine how many frets this slide travels
								techslideto = fret + slidediff + tp->capo;	//Get the correct ending fret for this string's slide, applying the capo position
							}
						}
					}
					if((thistechflags & EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE) && tp->technote[ctr]->unpitchend)
					{	//If this note has an unpitched slide and the user has defined the ending fret of the slide
						if(lowestfret != tp->technote[ctr]->unpitchend)
						{	//Don't allow the unpitched slide if it slides to the same fret this note/chord is already at
							unpitchedslidediff = tp->technote[ctr]->unpitchend - lowestfret;	//Determine how many frets this slide travels
							techunpitchedslideto = fret + unpitchedslidediff + tp->capo;	//Get the correct ending fret for this string's slide, applying the capo position
						}
					}
					if((techeflags & EOF_PRO_GUITAR_NOTE_EFLAG_STOP) && !has_stop)
					{	//If this tech note has stop status, and is the first such tech note found so far
						stop_tech_note_position = tp->technote[ctr]->pos;	//Record its position
						has_stop = 1;	//Track that a stop tech note has been found, only the first one on a note will take effect
					}
					if(thistechbends > techbends)
					{	//If this tech note's bend was the strongest found for this note so far
						techbends = thistechbends;	//Track the strongest tech note bend (in halfsteps)
					}
				}//If this tech note overlaps with the specified note
			}//If the tech note is in the same difficulty as the specified note and uses the same string
		}//For all tech notes, starting with the first one that applies to this note
	}//If tech notes are to be taken into account and the specified string of the note has at least one tech note applied to it

	if(ptr)
	{	//If the calling function passed a techniques structure
		char keeplength = 0;	//Set to nonzero if the note's techniques require the sustain to be kept

		memset(ptr, 0, sizeof(EOF_RS_TECHNIQUES));	//Force this structure to fill with zeroes to avoid scenarios where two identical structures fail memory comparison because of differences in the values of padding between variables
		ptr->length = eof_get_note_length(sp, track, notenum);

		ptr->bendstrength_h = ptr->bendstrength_q = ptr->bend = 0;	//Initialize these to default values
		ptr->slideto = ptr->unpitchedslideto = -1;

		if(has_stop)
		{	//If a stop tech note was encountered, override the length of the affected string
			if((stop_tech_note_position >= notepos) && (stop_tech_note_position - notepos < ptr->length))
			{	//Validate that the stop tech note's recorded position is at/after the beginning of the affected note and earlier than the end of that note
				ptr->length = stop_tech_note_position - notepos;
			}
		}
		if((flags & EOF_PRO_GUITAR_NOTE_FLAG_BEND) || (flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP) || (flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN) || (flags & EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE) || (flags & EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO))
		{	//If this note has bend, vibrato, slide (bend and slide notes are required to have a length > 0 or Rocksmith will crash), unpitched slide or sustain status
			keeplength = 1;
		}
		if(fret)
		{	//If this string is fretted (open notes don't have slide or bend attributes written)
			if((flags & EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION) == 0)
			{	//If this note doesn't have definitions for bend strength or the ending fret for slides
				if(flags & EOF_PRO_GUITAR_NOTE_FLAG_BEND)
				{	//If this note bends
					ptr->bend = 1;
					ptr->bendstrength_h = 1;	//Assume a 1 half step bend
					ptr->bendstrength_q = 2;
				}
				if(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP)
				{	//If this note slides up and the user hasn't defined the ending fret of the slide
					ptr->slideto = fret + 1;	//Assume a 1 fret slide until logic is added for the author to add this information
					ptr->slideto += tp->capo;	//Apply the capo position, so the slide ending is on the correct fret in-game
				}
				else if(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN)
				{	//If this note slides down and the user hasn't defined the ending fret of the slide
					ptr->slideto = fret - 1;	//Assume a 1 fret slide until logic is added for the author to add this information
					ptr->slideto += tp->capo;	//Apply the capo position, so the slide ending is on the correct fret in-game
				}
			}
			else
			{	//This note defines the bend strength and ending fret for slides
				if(flags & EOF_PRO_GUITAR_NOTE_FLAG_BEND)
				{	//If this note bends
					if(tp->note[notenum]->bendstrength & 0x80)
					{	//If this bend strength is defined in quarter steps
						ptr->bendstrength_q = (tp->note[notenum]->bendstrength & 0x7F);	//Obtain the defined bend strength in quarter steps (mask out the MSB)
						ptr->bendstrength_h = (ptr->bendstrength_q + 1) / 2;			//Obtain the defined bend strength rounded up to the nearest half step
					}
					else
					{	//The bend strength is defined in half steps
						ptr->bendstrength_h = tp->note[notenum]->bendstrength;	//Obtain the defined bend strength in half steps
						ptr->bendstrength_q = ptr->bendstrength_h * 2;			//Obtain the defined bend strength in quarter steps
					}
					ptr->bend = ptr->bendstrength_h;	//Obtain the strength of this bend rounded up to the nearest half step
				}
				if((flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP) || (flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN))
				{	//If this note slides
					if(tp->note[notenum]->slideend && (lowestfret != tp->note[notenum]->slideend))
					{	//If the note's slide is properly defined and doesn't end at the position the lowest fretted string in the note is already at
						slidediff = tp->note[notenum]->slideend - lowestfret;	//Determine how many frets this slide travels
						ptr->slideto = fret + slidediff + tp->capo;	//Get the correct ending fret for this string's slide, applying the capo position
					}
				}
			}
			if((flags & EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE) && tp->note[notenum]->unpitchend)
			{	//If this note has an unpitched slide and the user has defined the ending fret of the slide
				if(tp->note[notenum]->unpitchend && (lowestfret != tp->note[notenum]->unpitchend))
				{	//Don't allow the unpitched slide if its end fret isn't properly defined or if it slides to the same fret this note/chord is already at
					unpitchedslidediff = tp->note[notenum]->unpitchend - lowestfret;	//Determine how many frets this slide travels
					ptr->unpitchedslideto = fret + unpitchedslidediff + tp->capo;	//Get the correct ending fret for this string's slide, applying the capo position
					if(target == 2)
					{	//If the target game is Rocksmith 2
						ptr->slideto = -1;	//Don't allow the note to export as both an unpitched and a pitched slide at the same time
						techslideto = -1;	//Prevent the code block below from allowing a tech note to override this constraint
					}
				}
			}
			if((target == 2) && checktechnotes)
			{	//If the target game is Rocksmith 2 and tech notes are being taken into account
				if(techbends)
				{	//If there was at least one bend tech note found affecting the specified string
					ptr->bend = techbends;
				}
				if(techslideto >= 0)
				{	//If a tech note had a defined slide
					ptr->slideto = techslideto;	//It overrides any slide defined on the normal note
				}
				if(techunpitchedslideto >= 0)
				{	//If a tech note had a defined unpitched slide
					ptr->unpitchedslideto = techunpitchedslideto;	//It overrides any unpitched slide defined on the normal note
				}
			}
		}//If this string is fretted (open notes don't have slide or bend attributes written)

		//Determine note statuses
		flags |= techflags;		//Mix the normal and tech notes' flags
		eflags |= techeflags;	//Mix the normal and tech notes' extended flags
		if(!fret)
		{	//If the string is played open
			ptr->hammeron = 0;	//An open string can't be played by hammering onto a fret
		}
		else
		{
			ptr->hammeron = (flags & EOF_PRO_GUITAR_NOTE_FLAG_HO) ? 1 : 0;
		}
		ptr->pulloff = (flags & EOF_PRO_GUITAR_NOTE_FLAG_PO) ? 1 : 0;
		ptr->harmonic = (flags & EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC) ? 1 : 0;
		ptr->hopo = (ptr->hammeron | ptr->pulloff) ? 1 : 0;
		ptr->palmmute = (flags & EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE) ? 1 : 0;
		ptr->tremolo = (flags & EOF_NOTE_FLAG_IS_TREMOLO) ? 1 : 0;
		ptr->pop = (flags & EOF_PRO_GUITAR_NOTE_FLAG_POP) ? 1 : -1;
		ptr->slap = (flags & EOF_PRO_GUITAR_NOTE_FLAG_SLAP) ? 1 : -1;
		ptr->accent = (flags & EOF_PRO_GUITAR_NOTE_FLAG_ACCENT) ? 1 : 0;
		ptr->pinchharmonic = (flags & EOF_PRO_GUITAR_NOTE_FLAG_P_HARMONIC) ? 1 : 0;
		ptr->stringmute = (flags & EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE) ? 1 : 0;
		ptr->tap = (flags & EOF_PRO_GUITAR_NOTE_FLAG_TAP) ? 1 : 0;
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO)
		{	//If the note has vibrato technique
			ptr->vibrato = 80;	//The default vibrato speed is assumed
		}
		else
		{	//The note has no vibrato technique
			ptr->vibrato = 0;
		}
		ptr->linknext = (flags & EOF_PRO_GUITAR_NOTE_FLAG_LINKNEXT) ? 1 : 0;
		if((ptr->pop > 0) || (ptr->slap > 0))
		{	//If the note has pop or slap notation
			if(target == 1)
			{	//In Rocksmith 1, a slap/pop note cannot also be a slide/bend, because slap/pop notes require a sustain of 0 and slide/bend requires a nonzero sustain
				ptr->slideto = -1;	//Avoid allowing a 0 length slide note from crashing the game
				ptr->bend = ptr->bendstrength_h = ptr->bendstrength_q = 0;	//Avoid allowing a 0 length bend note from crashing the game
				ptr->length = 0;	//Remove all sustain for the note, otherwise Rocksmith 1 won't display the pop/slap sustain technique
			}
		}
		if((eflags & EOF_PRO_GUITAR_NOTE_EFLAG_IGNORE) && (target == 2))
		{	//If the note's extended flags indicate the ignore status is applied and Rocksmith 2 export is in effect
			ptr->ignore = 1;
		}
		else
		{
			ptr->ignore = 0;
		}
		if((eflags & EOF_PRO_GUITAR_NOTE_EFLAG_SUSTAIN) && (target == 2))
		{	//If the note's extended flags indicate the sustain status is applied and Rocksmith 2 export is in effect
			ptr->sustain = 1;
			keeplength = 1;
		}
		else
		{
			ptr->sustain = 0;
		}
		if(!keeplength)
		{	//If the note doesn't need to keep its sustain
			if((ptr->length == 1) && !((target == 2) && (eflags & EOF_PRO_GUITAR_NOTE_EFLAG_SUSTAIN) && (flags & EOF_PRO_GUITAR_NOTE_FLAG_LINKNEXT)))
			{	//Only if this note has the absolute minimum possible length and does not have the sustain or linknext status applied and the target is Rocksmith 2
				ptr->length = 0;	//Convert a 1ms note to a length of 0 so that it doesn't display as a sustain note in-game
			}
			if(ptr->palmmute || ptr->stringmute)
			{	//If the note is palm or string muted
				ptr->length = 0;	//Remove its sustain
			}
		}
	}//If the calling function passed a techniques structure

	//Make a bitmask reflecting only the techniques this note (or any applicable tech notes) has that require a chordNote subtag to be written
	flags &= (	EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP | EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN | EOF_PRO_GUITAR_NOTE_FLAG_BEND | EOF_PRO_GUITAR_NOTE_FLAG_HO | EOF_PRO_GUITAR_NOTE_FLAG_PO |
				EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC | EOF_NOTE_FLAG_IS_TREMOLO | EOF_PRO_GUITAR_NOTE_FLAG_POP | EOF_PRO_GUITAR_NOTE_FLAG_SLAP | EOF_PRO_GUITAR_NOTE_FLAG_P_HARMONIC |
				EOF_PRO_GUITAR_NOTE_FLAG_TAP | EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO | EOF_PRO_GUITAR_NOTE_FLAG_LINKNEXT | EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE);
	if((target == 2) && checktechnotes && (eof_pro_guitar_note_bitmask_has_tech_note(tp, notenum, tp->note[notenum]->note, NULL)))
	{	//If the target is Rocksmith 2, technotes are to be observed and any tech notes apply to at least one string of this note
		flags |= 1;	//Ensure the return value is nonzero, to reflect that the specified note will need chordNote tags written during export if the note is a chord
	}

	if((target == 2) && (eflags & EOF_PRO_GUITAR_NOTE_EFLAG_SUSTAIN))
	{	//If the target is Rocksmith 2 and this note has the sustain status applied
		flags |= EOF_PRO_GUITAR_NOTE_EFLAG_SUSTAIN;	//Ensure flags is nonzero so the calling function knows the a chord with such status exports with chordNote tags
	}

	return flags;
}

void eof_rs_export_cleanup(EOF_SONG * sp, unsigned long track)
{
	unsigned long ctr;
	EOF_PRO_GUITAR_TRACK *tp;

	if(!sp || (track >= sp->tracks) || (sp->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		return;	//Invalid parameters
	tp = sp->pro_guitar_track[sp->track[track]->tracknum];

	//Remove all temporary notes that were added and remove the ignore status from all notes
	for(ctr = tp->notes; ctr > 0; ctr--)
	{	//For each note in the track, in reverse order
		tp->note[ctr - 1]->tflags &= ~EOF_NOTE_TFLAG_IGNORE;	//Clear the ignore flag
		tp->note[ctr - 1]->tflags &= ~EOF_NOTE_TFLAG_ARP;		//Clear the arpeggio flag
		tp->note[ctr - 1]->tflags &= ~EOF_NOTE_TFLAG_HAND;		//Clear the handshape flag
		tp->note[ctr - 1]->tflags &= ~EOF_NOTE_TFLAG_ARP_FIRST;	//Clear the first in arpeggio flag
		tp->note[ctr - 1]->tflags &= ~EOF_NOTE_TFLAG_TWIN;		//Clear the partial ghosted chord twin flag
		tp->note[ctr - 1]->tflags &= ~EOF_NOTE_TFLAG_COMBINE;	//Clear the note/chordnote combine flag
		tp->note[ctr - 1]->tflags &= ~EOF_NOTE_TFLAG_NO_LN;		//Clear the ignore linknext flag
		if(tp->note[ctr - 1]->tflags & EOF_NOTE_TFLAG_TEMP)
		{	//If this is a temporary note that was added to split up an arpeggio's chord into single notes
			eof_track_delete_note(sp, track, ctr - 1);	//Delete it
		}
	}
	eof_track_sort_notes(sp, track);	//Re-sort the notes
}

void eof_rs2_export_note_string_to_xml(EOF_SONG * sp, unsigned long track, unsigned long notenum, unsigned long stringnum, char ischordnote, unsigned long fingering, PACKFILE *fp)
{
	EOF_PRO_GUITAR_TRACK *tp;
	unsigned long fret;			//The fret number used for the specified string of the note
	char tagend[2] = "/";		//If a bendValues subtag is to be written, this string is emptied so that the note/chordNote tag doesn't end in the same line
	unsigned long flags, notepos, notelen, ctr, bitmask;
	EOF_RS_TECHNIQUES tech = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 ,0};
	unsigned char *finger = NULL;
	long fingernum;
	char *tagstring, notestring[] = "note", chordnotestring[] = "chordNote", *indentlevel, noindent[] = "", indent[] = "  ", buffer[600] = {0};

	//Validate parameters and initialize some variables
	if(!sp || !fp || (track >= sp->tracks) || (sp->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT) || (stringnum > 5))
		return;	//Invalid parameters
	tp = sp->pro_guitar_track[sp->track[track]->tracknum];
	if(notenum >= tp->notes)
		return;	//Invalid parameter
	flags = tp->note[notenum]->flags;
	notepos = tp->note[notenum]->pos;
	notelen = tp->note[notenum]->length;
	bitmask = 1 << stringnum;
	if(ischordnote)
	{	//If a chordNote is being exported
		finger = tp->note[fingering]->finger;	//Point to this specified note's finger array
		tagstring = chordnotestring;
		indentlevel = indent;	//chordNote tags are indented two spaces more than note tags
	}
	else
	{	//A single note is being exported
		tagstring = notestring;
		indentlevel = noindent;
	}

	(void) eof_get_rs_techniques(sp, track, notenum, stringnum, &tech, 2, 1);	//Determine techniques used by this note/chordNote
	if(tp->note[notenum]->frets[stringnum] == 0xFF)
	{	//If this is a string mute with no defined fret number
		fret = 0;	//Assume muted open note
	}
	else
	{	//Otherwise use the defined fret number
		fret = tp->note[notenum]->frets[stringnum] & 0x7F;	//Get the fret value for this string (mask out the muting bit)
	}
	if(fret)
	{	//If this string isn't played open
		fret += tp->capo;	//Apply the capo position
	}
	if(tp->note[notenum]->frets[stringnum] & 0x80)
	{	//If the note/chordNote is string muted
		tech.stringmute = 1;	//Ensure the note/chordNote indicates this string is muted
	}
	else
	{	//It is not string muted
		tech.stringmute = 0;
	}

	//If a chordNote is being exported, determine if its sustain needs to be dropped
	if(ischordnote)
	{	//If a chordNote is being written
		if(!tech.tremolo && !tech.bend && !tech.vibrato && (tech.slideto < 0) && (tech.unpitchedslideto < 0))
		{	//If the chordNote does not have tremolo, bend, vibrato, slide or unpitched slide (all of which need to keep their sustain)
			if(!((fret == 0) && ((flags & EOF_PRO_GUITAR_NOTE_FLAG_BEND) || (flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP) || (flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN) || (flags & EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE) || (flags & EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO))))
			{	//If the chordNote is not fretted, it needs to keep its sustain if the fretted notes in the chord have bend, vibrato, slide or unpitched slide status
				if(!tech.sustain && !tech.linknext)
				{	//If the chordNote has the sustain or linknext status applied, it needs to keep its sustain
					tech.length = 0;	//Otherwise force the chordNote to have no sustain
				}
			}
		}
	}

	if(tech.bend)
	{	//If the note/chordNote is a bend, the note tag must not end on the same line as it will have a bendValues subtag
		tagend[0] = '\0';	//Drop the / from the string
	}

	//Determine the fingering to be exported for this note/chordNote
	if(finger && finger[stringnum])
	{	//If a chordNote is being exported, and the chordNote's string is fretted
		fingernum = finger[stringnum];
		if(fingernum == 5)
		{	//If this fingering specifies the thumb
			fingernum = 0;	//Convert from EOF's numbering (5 = thumb) to Rocksmith's numbering (0 = thumb)
		}
	}
	else
	{	//This string is played open
		fingernum = -1;
	}

	//If the EOF_NOTE_TFLAG_NO_LN flag is set, force the linknext status to be disregarded for this note
	if(tp->note[notenum]->tflags & EOF_NOTE_TFLAG_NO_LN)
		tech.linknext = 0;

	//Write the note/chordNote tag
	(void) snprintf(buffer, sizeof(buffer) - 1, "        %s<%s time=\"%.3f\" linkNext=\"%d\" accent=\"%d\" bend=\"%d\" fret=\"%lu\" hammerOn=\"%d\" harmonic=\"%d\" hopo=\"%d\" ignore=\"%d\" leftHand=\"%ld\" mute=\"%d\" palmMute=\"%d\" pluck=\"%d\" pullOff=\"%d\" slap=\"%d\" slideTo=\"%ld\" string=\"%lu\" sustain=\"%.3f\" tremolo=\"%d\" harmonicPinch=\"%d\" pickDirection=\"0\" rightHand=\"-1\" slideUnpitchTo=\"%ld\" tap=\"%d\" vibrato=\"%d\"%s>\n", indentlevel, tagstring, (double)notepos / 1000.0, tech.linknext, tech.accent, tech.bend, fret, tech.hammeron, tech.harmonic, tech.hopo, tech.ignore, fingernum, tech.stringmute, tech.palmmute, tech.pop, tech.pulloff, tech.slap, tech.slideto, stringnum, (double)tech.length / 1000.0, tech.tremolo, tech.pinchharmonic, tech.unpitchedslideto, tech.tap, tech.vibrato, tagend);
	(void) pack_fputs(buffer, fp);
	if(tech.bend)
	{	//If the note is a bend, write the bendValues subtag and close the note tag
		unsigned long bendpoints, firstbend = 0, bendstrength_q;	//Used to parse any bend tech notes that may affect the exported note
		long nextnote;

		bendpoints = eof_pro_guitar_note_bitmask_has_bend_tech_note(tp, notenum, bitmask, &firstbend);	//Count how many bend tech notes overlap this note on the specified string
		if(!bendpoints)
		{	//If there are none
			(void) snprintf(buffer, sizeof(buffer) - 1, "          %s<bendValues count=\"1\">\n", indentlevel);
			(void) pack_fputs(buffer, fp);
			(void) snprintf(buffer, sizeof(buffer) - 1, "            %s<bendValue time=\"%.3f\" step=\"%.3f\"/>\n", indentlevel, (((double)notepos + ((double)tech.length / 3.0)) / 1000.0), (double)tech.bendstrength_q / 2.0);	//Write a bend point 1/3 into the note
			(void) pack_fputs(buffer, fp);
		}
		else
		{	//If there's at least one bend tech note that overlaps the note being exported
			(void) snprintf(buffer, sizeof(buffer) - 1, "          %s<bendValues count=\"%lu\">\n", indentlevel, bendpoints);
			(void) pack_fputs(buffer, fp);
			nextnote = eof_fixup_next_pro_guitar_note(tp, notenum);
			if(nextnote > 0)
			{	//If there was a next note
				if(notepos + notelen == tp->pgnote[nextnote]->pos)
				{	//And this note extends all the way to it with no gap in between (this note has linkNext status)
					notelen--;	//Shorten the effective note length to ensure that a tech note at the next note's position is detected as affecting that note instead of this one
				}
			}
			for(ctr = firstbend; ctr < tp->technotes; ctr++)
			{	//For all tech notes, starting with the first applicable bend tech note
				if(tp->technote[ctr]->pos > notepos + notelen)
				{	//If this tech note (and all those that follow) are after the end position of this note
					break;	//Break from loop, no more overlapping notes will be found
				}
				if((tp->technote[ctr]->type == tp->pgnote[notenum]->type) && (bitmask & tp->technote[ctr]->note))
				{	//If the tech note is in the same difficulty as the pro guitar single note being exported and uses the same string
					if((tp->technote[ctr]->pos >= notepos) && (tp->technote[ctr]->pos <= notepos + notelen))
					{	//If this tech note overlaps with the specified pro guitar regular note
						flags = tp->technote[ctr]->flags;
						if((flags & EOF_PRO_GUITAR_NOTE_FLAG_BEND) && (flags & EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION))
						{	//If the tech note is a bend and has a bend strength defined
							if(tp->technote[ctr]->bendstrength & 0x80)
							{	//If this bend strength is defined in quarter steps
								bendstrength_q = tp->technote[ctr]->bendstrength & 0x7F;	//Obtain the defined bend strength in quarter steps (mask out the MSB)
							}
							else
							{	//The bend strength is defined in half steps
								bendstrength_q = tp->technote[ctr]->bendstrength * 2;		//Obtain the defined bend strength in quarter steps
							}
							(void) snprintf(buffer, sizeof(buffer) - 1, "            %s<bendValue time=\"%.3f\" step=\"%.3f\"/>\n", indentlevel, ((double)tp->technote[ctr]->pos / 1000.0), (double)bendstrength_q / 2.0);	//Write the bend point at the specified position within the note
							(void) pack_fputs(buffer, fp);
						}
					}
				}
			}
		}
		(void) snprintf(buffer, sizeof(buffer) - 1, "          %s</bendValues>\n", indentlevel);
		(void) pack_fputs(buffer, fp);
		(void) snprintf(buffer, sizeof(buffer) - 1, "        %s</%s>\n", indentlevel, tagstring);
		(void) pack_fputs(buffer, fp);
	}//If the note is a bend, write the bendValues subtag and close the note tag
}

int eof_rs_export_common(EOF_SONG * sp, unsigned long track, PACKFILE *fp, unsigned short *user_warned)
{
	EOF_PRO_GUITAR_TRACK *tp;
	unsigned long *sectionlist = NULL, sectionlistsize, ctr, ctr2, numsections, phraseid = 0;
	char end_phrase_found = 0;	//Will track if there was a manually defined END phrase
	char buffer[200] = {0}, buffer2[50] = {0};
	long startbeat;	//This will indicate the first beat containing a note in the track
	long endbeat;	//This will indicate the first beat after the exported track's last note

	if(!sp || !fp || (track >= sp->tracks) || (sp->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT) || !user_warned)
		return 0;	//Invalid parameters

	tp = sp->pro_guitar_track[sp->track[track]->tracknum];

	//Check if any RS phrases need to be added
	startbeat = eof_get_beat(sp, tp->note[0]->pos);	//Find the beat containing the track's first note
	if(startbeat < 0)
	{	//If the beat couldn't be found
		startbeat = 0;	//Set this to the first beat
	}
	endbeat = eof_get_beat(sp, tp->note[tp->notes - 1]->pos + tp->note[tp->notes - 1]->length);	//Find the beat containing the end of the track's last note
	if((endbeat < 0) || (endbeat + 1 >= sp->beats))
	{	//If the beat couldn't be found, or the extreme last beat in the track has the last note
		endbeat = sp->beats - 1;	//Set this to the last beat
	}
	else
	{
		endbeat++;	//Otherwise set it to the first beat that follows the end of the last note
	}
	eof_process_beat_statistics(sp, track);	//Cache section name information into the beat structures (from the perspective of the specified track)
	if(!eof_song_contains_event(sp, "COUNT", track, EOF_EVENT_FLAG_RS_PHRASE, 1) && !eof_song_contains_event(sp, "COUNT", 0, EOF_EVENT_FLAG_RS_PHRASE, 1))
	{	//If the user did not define a COUNT phrase that applies to either the track being exported or all tracks
		if((sp->beat[0]->contained_section_event >= 0) && ((*user_warned & 16) == 0))
		{	//If there is already a phrase defined on the first beat, and the user wasn't warned of this problem yet
			allegro_message("Warning:  There is no COUNT phrase, but the first beat marker already has a phrase.\nYou should move that phrase because only one phrase per beat is exported.");
			*user_warned |= 16;
		}
		eof_log("\t! Adding missing COUNT phrase", 1);
		(void) eof_song_add_text_event(sp, 0, "COUNT", 0, EOF_EVENT_FLAG_RS_PHRASE, 1);	//Add it as a temporary event at the first beat
	}
	for(ctr = 0; ctr < sp->beats; ctr++)
	{	//For each beat
		if((sp->beat[ctr]->contained_section_event >= 0) && !ustricmp(sp->text_event[sp->beat[ctr]->contained_section_event]->text, "END"))
		{	//If this beat contains an "END" RS phrase
			for(ctr2 = ctr + 1; ctr2 < sp->beats; ctr2++)
			{	//For each remaining beat
				if(((sp->beat[ctr2]->contained_section_event >= 0) || (sp->beat[ctr2]->contained_rs_section_event >= 0)) && ((*user_warned & 32) == 0))
				{	//If the beat contains an RS phrase or RS section, and the user wasn't warned of this problem yet
					unsigned char original_eof_2d_render_top_option = eof_2d_render_top_option;	//Back up the user's preference

					eof_2d_render_top_option = 36;	//Change the user preference to display RS phrases and sections
					eof_selected_beat = ctr;		//Select it
					eof_seek_and_render_position(track, eof_note_type, sp->beat[ctr]->pos);	//Show the offending END phrase
					allegro_message("Warning:  Beat #%lu contains an END phrase, but there's at least one more phrase or section after it.\nThis will cause dynamic difficulty and/or riff repeater to not work correctly.", ctr);
					eof_2d_render_top_option = original_eof_2d_render_top_option;	//Restore the user's preference
					*user_warned |= 32;
					break;
				}
			}
			end_phrase_found = 1;
			break;
		}
	}
	if(!end_phrase_found)
	{	//If the user did not define a END phrase
		if((sp->beat[endbeat]->contained_section_event >= 0) && ((*user_warned & 512) == 0))
		{	//If there is already a phrase defined on the beat following the last note, and the user wasn't warned of this problem yet
			unsigned char original_eof_2d_render_top_option = eof_2d_render_top_option;	//Back up the user's preference

			eof_2d_render_top_option = 36;	//Change the user preference to display RS phrases and sections
			eof_selected_beat = endbeat;		//Select it
			eof_seek_and_render_position(track, eof_note_type, sp->beat[endbeat]->pos);	//Show where the END phrase should go
			allegro_message("Warning:  There is no END phrase, but the beat marker after the last note in \"%s\" already has a phrase.\nYou should move that phrase because only one phrase per beat is exported.", sp->track[track]->name);
			eof_2d_render_top_option = original_eof_2d_render_top_option;	//Restore the user's preference
			*user_warned |= 512;
		}
		eof_log("\t! Adding missing END phrase", 1);
		(void) eof_song_add_text_event(sp, endbeat, "END", 0, EOF_EVENT_FLAG_RS_PHRASE, 1);	//Add it as a temporary event at the last beat
	}

	//Check if any RS sections need to be added
	eof_sort_events(sp);	//Re-sort events
	eof_process_beat_statistics(sp, track);	//Cache section name information into the beat structures (from the perspective of the specified track)
	if(!eof_song_contains_event(sp, "intro", track, EOF_EVENT_FLAG_RS_SECTION, 1) && !eof_song_contains_event(sp, "intro", 0, EOF_EVENT_FLAG_RS_SECTION, 1))
	{	//If the user did not define an intro RS section that applies to either the track being exported or all tracks
		if((sp->beat[startbeat]->contained_rs_section_event >= 0) && ((*user_warned & 64) == 0))
		{	//If there is already a RS section defined on the first beat containing a note, and the user wasn't warned of this problem yet
			allegro_message("Warning:  There is no intro RS section, but the beat marker before the first note already has a section.\nYou should move that section because only one section per beat is exported.");
			*user_warned |= 64;
		}
		eof_log("\t! Adding missing intro RS section", 1);
		(void) eof_song_add_text_event(sp, startbeat, "intro", 0, EOF_EVENT_FLAG_RS_SECTION, 1);	//Add a temporary one
	}
	if(!eof_song_contains_event(sp, "noguitar", track, EOF_EVENT_FLAG_RS_SECTION, 1) && !eof_song_contains_event(sp, "noguitar", 0, EOF_EVENT_FLAG_RS_SECTION, 1))
	{	//If the user did not define a noguitar RS section that applies to either the track being exported or all tracks
		if((sp->beat[endbeat]->contained_rs_section_event >= 0) && ((*user_warned & 128) == 0))
		{	//If there is already a RS section defined on the first beat after the last note, and the user wasn't warned of this problem yet
			allegro_message("Warning:  There is no noguitar RS section, but the beat marker after the last note already has a section.\nYou should move that section because only one section per beat is exported.");
			*user_warned |= 128;
		}
		eof_log("\t! Adding missing noguitar RS section", 1);
		(void) eof_song_add_text_event(sp, endbeat, "noguitar", 0, EOF_EVENT_FLAG_RS_SECTION, 1);	//Add a temporary one
	}

	//Check if any beats need to be added
	//DDC prefers when the XML pads partially complete measures by adding beats to finish the measure and then going one beat into the next measure
	if(sp->beat[endbeat]->beat_within_measure)
	{	//If the first beat after the last note in this track isn't the first beat in a measure
		ctr = sp->beat[endbeat]->num_beats_in_measure - sp->beat[endbeat]->beat_within_measure;	//This is how many beats need to be after endbeat
		if(endbeat + ctr > sp->beats)
		{	//If the project doesn't have enough beats to accommodate this padding
			(void) eof_song_append_beats(sp, ctr);	//Append them to the end of the project
		}
	}

	//Write the phrases
	eof_sort_events(sp);	//Re-sort events
	eof_process_beat_statistics(sp, track);	//Rebuild beat stats
	for(ctr = 0, numsections = 0; ctr < sp->beats; ctr++)
	{	//For each beat in the chart
		if(sp->beat[ctr]->contained_section_event >= 0)
		{	//If this beat has a section event (RS phrase)
			numsections++;	//Update section marker instance counter
		}
	}
	sectionlistsize = eof_build_section_list(sp, &sectionlist, track);	//Build a list of all unique section markers (Rocksmith phrases) in the chart (from the perspective of the track being exported)
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <phrases count=\"%lu\">\n", sectionlistsize);	//Write the number of unique phrases
	(void) pack_fputs(buffer, fp);
	for(ctr = 0; ctr < sectionlistsize; ctr++)
	{	//For each of the entries in the unique section (RS phrase) list
		char * currentphrase = NULL;
		unsigned long startpos = 0, endpos = 0;		//Track the start and end position of the each instance of the phrase
		unsigned char maxdiff, ongoingmaxdiff = 0;	//Track the highest fully leveled difficulty used among all phraseinstances
		char started = 0;

		//Determine the highest maxdifficulty present among all instances of this phrase
		assert(sectionlist != NULL);	//Unneeded check to resolve a false positive in Splint
		for(ctr2 = 0; ctr2 < sp->beats; ctr2++)
		{	//For each beat
			if((sp->beat[ctr2]->contained_section_event >= 0) || ((ctr2 + 1 >= eof_song->beats) && started))
			{	//If this beat contains a section event (Rocksmith phrase) or a phrase is in progress and this is the last beat, it marks the end of any current phrase and the potential start of another
				started = 0;
				endpos = sp->beat[ctr2]->pos - 1;	//Track this as the end position of the previous phrase marker
				if(currentphrase)
				{	//If the first instance of the phrase was already encountered
					if(!ustricmp(currentphrase, sp->text_event[sectionlist[ctr]]->text))
					{	//If the phrase that just ended is an instance of the phrase being written
						maxdiff = eof_find_fully_leveled_rs_difficulty_in_time_range(sp, track, startpos, endpos, 1);	//Find the maxdifficulty value for this phrase instance, converted to relative numbering
						if(maxdiff > ongoingmaxdiff)
						{	//If that phrase instance had a higher maxdifficulty than the other instances checked so far
							ongoingmaxdiff = maxdiff;	//Track it
						}
					}
				}
				started = 1;
				startpos = sp->beat[ctr2]->pos;	//Track the starting position
				currentphrase = sp->text_event[eof_song->beat[ctr2]->contained_section_event]->text;	//Track which phrase is being examined
			}
		}

		//Write the phrase definition using the highest difficulty found among all instances of the phrase
		expand_xml_text(buffer2, sizeof(buffer2) - 1, sp->text_event[sectionlist[ctr]]->text, 32, 2);	//Expand XML special characters into escaped sequences if necessary, and check against the maximum supported length of this field.  Filter out characters suspected of causing the game to crash (do not allow forward slash).
		(void) snprintf(buffer, sizeof(buffer) - 1, "    <phrase disparity=\"0\" ignore=\"0\" maxDifficulty=\"%u\" name=\"%s\" solo=\"0\"/>\n", ongoingmaxdiff, buffer2);
		(void) pack_fputs(buffer, fp);
	}//For each of the entries in the unique section (RS phrase) list
	(void) pack_fputs("  </phrases>\n", fp);
	(void) snprintf(buffer, sizeof(buffer) - 1, "  <phraseIterations count=\"%lu\">\n", numsections);	//Write the number of phrase instances
	(void) pack_fputs(buffer, fp);
	for(ctr = 0; ctr < sp->beats; ctr++)
	{	//For each beat in the chart
		if(sp->beat[ctr]->contained_section_event >= 0)
		{	//If this beat has a section event
			for(ctr2 = 0; ctr2 < sectionlistsize; ctr2++)
			{	//For each of the entries in the unique section list
				assert(sectionlist != NULL);	//Unneeded check to resolve a false positive in Splint
				if(!ustricmp(sp->text_event[sp->beat[ctr]->contained_section_event]->text, sp->text_event[sectionlist[ctr2]]->text))
				{	//If this event matches a section marker entry
					phraseid = ctr2;
					break;
				}
			}
			if(ctr2 >= sectionlistsize)
			{	//If the section couldn't be found
				allegro_message("Error:  Couldn't find section in unique section list.  Aborting Rocksmith export.");
				eof_log("Error:  Couldn't find section in unique section list.  Aborting Rocksmith export.", 1);
				free(sectionlist);
				return 0;	//Return error
			}
			(void) snprintf(buffer, sizeof(buffer) - 1, "    <phraseIteration time=\"%.3f\" phraseId=\"%lu\"/>\n", sp->beat[ctr]->fpos / 1000.0, phraseid);
			(void) pack_fputs(buffer, fp);
		}
	}
	(void) pack_fputs("  </phraseIterations>\n", fp);
	if(sectionlistsize)
	{	//If there were any entries in the unique section list
		free(sectionlist);	//Free the list now
	}

	return 1;	//Return success
}

int eof_rs_combine_linknext_logic(EOF_SONG * sp, unsigned long track, unsigned long notenum, unsigned long stringnum)
{
	EOF_PRO_GUITAR_TRACK *tp;
	EOF_RS_TECHNIQUES tech = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 ,0}, tech2 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 ,0};
	int retval = 0;

	if(!sp || (track >= sp->tracks) || (sp->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT) || (stringnum > 5))
		return 0;		//Invalid parameters

	tp = sp->pro_guitar_track[sp->track[track]->tracknum];
	if(!(tp->note[notenum]->note & (1 << stringnum)))	//If the specified chord doesn't have a gem on this string
		return 0;		//Invalid paramter

	(void) eof_get_rs_techniques(sp, track, notenum, stringnum, &tech, 2, 1);		//Determine techniques used by this gem (including applicable technotes)
	(void) eof_get_rs_techniques(sp, track, notenum, stringnum, &tech2, 2, 1);

	while(tech2.linknext)
	{	//While linknext may have the effect of combining a single note's sustain with the chordnote
		long target, target2;

		target = notenum;
		while(1)
		{	//Find the first of the remaining notes that is not ignored (unless it is marked for combination) and is at/after the end position of the the chordnote, if any
			target = eof_fixup_next_pro_guitar_note(tp, target);
			if(target <= 0)
				break;	//Out of notes to check, exit loop
			if((tp->note[target]->tflags & EOF_NOTE_TFLAG_IGNORE) && !(tp->note[target]->tflags & EOF_NOTE_TFLAG_COMBINE))
				continue;	//If this note is ignored and not marked for combination, skip it
			if(tp->note[target]->pos >= tp->note[notenum]->pos + tp->note[notenum]->length)
				break;	//If this note is at/after the end position of the chordnote, exit loop
		}

		if((target > 0) && (tp->note[target]->pos != tp->note[notenum]->pos))
		{	//If such a note was found
			EOF_PRO_GUITAR_NOTE *np1 = tp->note[target];	//Simplify

			for(target2 = target; (target2 > 0) && (tp->note[target2]->pos == np1->pos); target2 = eof_fixup_next_pro_guitar_note(tp, target2))
			{	//For all remaining notes at that timestamp
				EOF_PRO_GUITAR_NOTE *np2 = tp->note[target2];	//Simplify
				tech2.linknext = 0;	//Set the condition to break from the while loop

				if((np2->tflags & EOF_NOTE_TFLAG_IGNORE) && !(np2->tflags & EOF_NOTE_TFLAG_COMBINE))
					continue;	//If this single note is already being ignored, and isn't marked for combination, skip it

				if(np2->note == (1 << stringnum))
				{	//If it is a single note on the same string as the chordnote being exported
					(void) eof_get_rs_techniques(sp, track, target2, stringnum, &tech2, 2, 1);		//Determine techniques used by this single note (including applicable technotes)
					tech2.linknext = tech.linknext;	//Disregard any differences with the linknext status
					tech2.length = tech.length;		//And length
					tech2.sustain = tech.sustain;	//And sustain
					if(!memcmp(&tech, &tech2, sizeof(EOF_RS_TECHNIQUES)) && (tp->note[notenum]->frets[stringnum] == np2->frets[stringnum]))
					{	//If the used techniques and fret numbers were otherwise identical, combine this single note's sustain with that of the chordnote and don't export the single note
						tp->note[notenum]->length = np2->pos + np2->length - tp->note[notenum]->pos;	//Adjust length of chordnote
						np2->tflags |= EOF_NOTE_TFLAG_IGNORE;	//Mark the single note as ignored
						np2->tflags |= EOF_NOTE_TFLAG_COMBINE;	//Flag that the note is going to be combined with a chordnote during export
						(void) eof_get_rs_techniques(sp, track, target2, stringnum, &tech2, 2, 1);	//Allow linknext on the merged note to cause the while loop to re-run
						if(!tech2.linknext)
						{	//If the merged note was not linked to the next one
							tp->note[notenum]->tflags |= EOF_NOTE_TFLAG_NO_LN;	//Mark that the linknext status for the chordnote is to not be exported
							retval = 1;	//Signal to calling function that any linknext for this chordnote should be forced off during export
						}
					}
					else
					{
						tech2.linknext = 0;	//Set the condition to break from the while loop
					}
					break;	//Break from for loop to allow for checking of notes at a later timestamp
				}
			}
		}
		else
		{
			tech2.linknext = 0;	//Break from while loop
		}
	}

	return retval;
}
