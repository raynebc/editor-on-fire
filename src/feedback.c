/*

Up/down arrow: Move forward and backward.
Left/right arrow: Change note spacement.
12345: Place notes.
Space: Play song.

I have Finnish keyboard so these might be different for you:
2 keys next to backspace: Change BPM. With shift, changes 0,01 instead of normal 1.
Key next to right shift: Change playback rate.

Shift + up: Choose notes.
Ctrl + up/down: Move selected notes forward/backward. If no notes is selected, move everything from ahead.
Ctrl + left/right: Same as above, but makes yellow blue, blue orange and so on. Kinda dangerous though, dB doesn't have Ctrl-z (EOF has?) so that might ruin everything.
Ctrl-c / Ctrl-v: copy/paste.

*/

#include <allegro.h>
#include <stdio.h>
#include <ctype.h>
#include "song.h"
#include "menu/edit.h"
#include "menu/song.h"
#include "menu/note.h"
#include "beat.h"
#include "main.h"
#include "dialog.h"
#include "player.h"
#include "editor.h"
#include "feedback.h"
#include "undo.h"
#include "foflc/Lyric_storage.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

int         eof_feedback_step = 0;
int         eof_feedback_selecting = 0;
int         eof_feedback_selecting_start_pos = 0;
int         eof_feedback_selecting_end_pos = 0;
EOF_NOTE *  eof_feedback_new_note = NULL;
int         eof_feedback_note[5] = {0};

int eof_feedback_any_note(void)
{
	eof_log("eof_feedback_any_note() entered", 1);

	int i;

	for(i = 0; i < 5; i++)
	{
		if(eof_feedback_note[i])
		{
			return 1;
		}
	}
	return 0;
}

/* read keys only available to editor (use after song is loaded */
void eof_editor_logic_feedback(void)
{
	eof_log("eof_editor_logic_feedback() entered", 1);

	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;
	long fbeat = 0;
	int fcpos[32];
	int fppos[32];
	int fsnap_divider[32] = {1, 2, 3, 4, 6, 8, 12}; // divide beat into this many portions
	int i;
	int npos;
	double bpm = 120.0;
	unsigned long cppqn = eof_song->beat[fbeat]->ppqn;

	fbeat = eof_get_beat(eof_song, eof_music_pos - eof_av_delay);
	if(fbeat >= 0)
	{
		bpm = (double)60000000.0 / (double)eof_song->beat[fbeat]->ppqn;
	}

	if(key[KEY_SLASH])
	{
		if(eof_music_paused)
		{
			eof_menu_edit_playback_speed_helper_faster();
		}
		key[KEY_SLASH] = 0;
	}

	if(key[KEY_M])
	{
		eof_menu_edit_metronome();
		key[KEY_M] = 0;
	}
	if(key[KEY_C] && !KEY_EITHER_CTRL)
	{
		eof_menu_edit_claps();
		key[KEY_C] = 0;
	}
	if(KEY_EITHER_CTRL && key[KEY_C])
	{
		eof_menu_edit_copy();
		key[KEY_C] = 0;
	}
	if(KEY_EITHER_CTRL && key[KEY_V])
	{
		eof_menu_edit_paste();
		key[KEY_V] = 0;
	}

	/* play/pause music */
	if(key[KEY_SPACE] && eof_song_loaded)
	{
		if(KEY_EITHER_SHIFT)
		{
			eof_catalog_play();
		}
		else
		{
			if(eof_music_catalog_playback)
			{
				eof_music_catalog_playback = 0;
				eof_music_catalog_pos = eof_song->catalog->entry[eof_selected_catalog_entry].start_pos + eof_av_delay;
				eof_stop_midi();
				alogg_stop_ogg(eof_music_track);
				alogg_seek_abs_msecs_ogg(eof_music_track, eof_music_pos);
			}
			else
			{
				eof_music_play();
				if(eof_music_paused)
				{
					eof_menu_song_seek_rewind();
				}
			}
		}
		key[KEY_SPACE] = 0;
	}

	if(!eof_music_paused)
	{
		for(i = 0; i < eof_song->legacy_track[tracknum]->notes; i++)
		{
			if(eof_song->legacy_track[tracknum]->note[i]->type == eof_note_type)
			{
				npos = eof_song->legacy_track[tracknum]->note[i]->pos;
				if((eof_music_pos - eof_av_delay > npos) && (eof_music_pos - eof_av_delay < npos + (eof_song->legacy_track[tracknum]->note[i]->length > 100 ? eof_song->legacy_track[tracknum]->note[i]->length : 100)))
				{
					if(eof_hover_note != i)
					{
						eof_hover_note = i;
					}
					break;
				}
			}
		}
		return;
	}

	/* change quantization */
	if(key[KEY_LEFT])
	{
		if(KEY_EITHER_CTRL)
		{
			eof_menu_note_transpose_down();
		}
		else
		{
			eof_snap_mode--;
			if(eof_snap_mode < 1)
			{
				eof_snap_mode = 7;
			}
			eof_music_pos = eof_song->beat[fbeat]->pos + eof_av_delay;
			eof_feedback_step = 0;
		}
		key[KEY_LEFT] = 0;
	}
	if(key[KEY_RIGHT])
	{
		if(KEY_EITHER_CTRL)
		{
			eof_menu_note_transpose_up();
		}
		else
		{
			eof_snap_mode++;
			if(eof_snap_mode > 7)
			{
				eof_snap_mode = 1;
			}
			eof_music_pos = eof_song->beat[fbeat]->pos + eof_av_delay;
			eof_feedback_step = 0;
		}
		key[KEY_RIGHT] = 0;
	}

	/* change BPM */
	if(key[KEY_MINUS])
	{
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		eof_menu_edit_cut(fbeat + 1, 1, 0.0);
		if(KEY_EITHER_SHIFT)
		{
			bpm -= 0.1;
		}
		else
		{
			bpm -= 1.0;
		}
		for(i = fbeat; i < EOF_MAX_BEATS; i++)
		{
			if(eof_song->beat[i]->ppqn == cppqn)
			{
				eof_song->beat[i]->ppqn = (double)60000000.0 / bpm;
			}

			/* break when we reach the end of the portion to change */
			else
			{
				break;
			}
		}
		eof_song->beat[fbeat]->flags = EOF_BEAT_FLAG_ANCHOR;
		eof_calculate_beats(eof_song);
		eof_menu_edit_cut_paste(fbeat + 1, 1, 0.0);
		key[KEY_MINUS] = 0;
	}
	if(key[KEY_EQUALS])
	{
		eof_prepare_undo(EOF_UNDO_TYPE_NONE);
		eof_menu_edit_cut(fbeat + 1, 1, 0.0);
		if(KEY_EITHER_SHIFT)
		{
			bpm += 0.1;
		}
		else
		{
			bpm += 1.0;
		}
		for(i = fbeat; i < EOF_MAX_BEATS; i++)
		{
			if(eof_song->beat[i]->ppqn == cppqn)
			{
				eof_song->beat[i]->ppqn = (double)60000000.0 / bpm;
			}

			/* break when we reach the end of the portion to change */
			else
			{
				break;
			}
		}
		eof_song->beat[fbeat]->flags = EOF_BEAT_FLAG_ANCHOR;
		eof_calculate_beats(eof_song);
		eof_menu_edit_cut_paste(fbeat + 1, 1, 0.0);
		key[KEY_EQUALS] = 0;
	}

	if(eof_snap_mode == 0)
	{
		eof_snap_mode = 1;
	}
	if(eof_music_pos - eof_av_delay < eof_song->beat[0]->pos)
	{
		eof_music_pos = eof_song->beat[0]->pos + eof_av_delay;
	}

	/* measure the length of the beat */
	eof_snap.length = eof_song->beat[fbeat + 1]->pos - eof_song->beat[fbeat]->pos;

	/* find snap positions */
	for(i = 0; i < fsnap_divider[eof_snap_mode - 1]; i++)
	{
		fcpos[i] = eof_song->beat[fbeat]->pos + ((float)eof_snap.length / (float)fsnap_divider[eof_snap_mode - 1]) * ((float)(i)) + eof_av_delay;
	}
	if(fbeat > 0)
	{
		for(i = 0; i < fsnap_divider[eof_snap_mode - 1]; i++)
		{
			fppos[i] = eof_song->beat[fbeat - 1]->pos + ((float)eof_snap.length / (float)fsnap_divider[eof_snap_mode - 1]) * ((float)(i)) + eof_av_delay;
		}
	}

	eof_hover_note = -1;
	for(i = 0; i < eof_song->legacy_track[tracknum]->notes; i++)
	{
		if((eof_song->legacy_track[tracknum]->note[i]->type == eof_note_type) && (eof_song->legacy_track[tracknum]->note[i]->pos > eof_music_pos - eof_av_delay - 5) && (eof_song->legacy_track[tracknum]->note[i]->pos < eof_music_pos - eof_av_delay + 5))
		{
			eof_hover_note = i;
			break;
		}
	}
	if(!KEY_EITHER_SHIFT)
	{
		eof_feedback_selecting = 0;
	}
	if(key[KEY_UP])
	{
		if(KEY_EITHER_CTRL)
		{
			eof_menu_note_push_back();
		}
		else
		{
			if(KEY_EITHER_SHIFT)
			{
				if(!eof_feedback_selecting)
				{
					eof_feedback_selecting = 1;
					eof_feedback_selecting_start_pos = eof_music_pos - eof_av_delay;
				}
				else
				{
					eof_feedback_selecting_end_pos = eof_music_pos - eof_av_delay;
				}
			}
			if(eof_snap_mode == 1)
			{
				eof_music_pos = eof_song->beat[fbeat + 1]->pos + eof_av_delay;
			}
			else
			{
				if(eof_feedback_step < fsnap_divider[eof_snap_mode - 1])
				{
					eof_feedback_step++;
					if(eof_feedback_step >= fsnap_divider[eof_snap_mode - 1])
					{
						eof_music_pos = eof_song->beat[fbeat + 1]->pos + eof_av_delay;
						eof_feedback_step = 0;
					}
					else
					{
						eof_music_pos = fcpos[eof_feedback_step];
					}
				}
			}
			if(KEY_EITHER_SHIFT)
			{
				if(eof_feedback_selecting)
				{
					eof_feedback_selecting = 1;
					eof_feedback_selecting_end_pos = eof_music_pos - eof_av_delay;
				}
			}
			if(eof_feedback_any_note() && eof_feedback_new_note)
			{
				eof_feedback_new_note->length = eof_music_pos - eof_av_delay - eof_feedback_new_note->pos;
			}
		}
		key[KEY_UP] = 0;
	}
	memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
	for(i = 0; i < eof_song->legacy_track[tracknum]->notes; i++)
	{
		if((eof_song->legacy_track[tracknum]->note[i]->type == eof_note_type) && (eof_song->legacy_track[tracknum]->note[i]->pos >= eof_feedback_selecting_start_pos) && (eof_song->legacy_track[tracknum]->note[i]->pos <= eof_feedback_selecting_end_pos))
		{
			eof_selection.multi[i] = 1;
		}
	}
	if(key[KEY_DOWN])
	{
		if(KEY_EITHER_CTRL)
		{
			eof_menu_note_push_up();
		}
		else if(eof_music_pos > eof_song->beat[0]->pos + eof_av_delay)
		{
			if(eof_snap_mode == 1)
			{
				if(fbeat > 0)
				{
					eof_music_pos = eof_song->beat[fbeat - 1]->pos + eof_av_delay;
				}
			}
			else
			{
				if(eof_feedback_step > 0)
				{
					eof_feedback_step--;
					eof_music_pos = fcpos[eof_feedback_step];
				}
				else
				{
					eof_feedback_step = fsnap_divider[eof_snap_mode - 1] - 1;
					eof_music_pos = fppos[eof_feedback_step];
				}
			}
			if(eof_feedback_any_note() && eof_feedback_new_note)
			{
				eof_feedback_new_note->length = eof_music_pos - eof_av_delay - eof_feedback_new_note->pos;
				if(eof_feedback_new_note->length < 1)
				{
					eof_feedback_new_note->length = 1;
				}
			}
		}
		key[KEY_DOWN] = 0;
	}
	eof_hover_note = -1;
	for(i = 0; i < eof_song->legacy_track[tracknum]->notes; i++)
	{
		if((eof_song->legacy_track[tracknum]->note[i]->type == eof_note_type) && (eof_song->legacy_track[tracknum]->note[i]->pos > eof_music_pos - eof_av_delay - 5) && (eof_song->legacy_track[tracknum]->note[i]->pos < eof_music_pos - eof_av_delay + 5))
		{
			eof_hover_note = i;
			break;
		}
	}
	if(key[KEY_1])
	{
		if(eof_hover_note < 0)
		{
			if(!eof_feedback_new_note)
			{
				eof_feedback_new_note = eof_legacy_track_add_note(eof_song->legacy_track[tracknum]);
				eof_legacy_track_note_create(eof_feedback_new_note, 1, 0, 0, 0, 0, 0, eof_music_pos - eof_av_delay, 1);
				eof_feedback_new_note->type = eof_note_type;
				eof_selection.current_pos = eof_feedback_new_note->pos;
				eof_selection.track = eof_selected_track;
				memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
				eof_sort_notes(eof_song);
				eof_fixup_notes(eof_song);
				eof_determine_phrase_status();
				eof_selection.multi[eof_selection.current] = 1;
			}
			eof_feedback_note[0] = 1;
		}
		else if(eof_feedback_note[0] == 0)
		{
			eof_feedback_new_note = eof_song->legacy_track[tracknum]->note[eof_hover_note];
			eof_song->legacy_track[tracknum]->note[eof_hover_note]->note ^= 1;
			eof_feedback_note[0] = 2;
		}
	}
	else
	{
		eof_feedback_note[0] = 0;
	}
	if(key[KEY_2])
	{
		if(eof_hover_note < 0)
		{
			if(!eof_feedback_new_note)
			{
				eof_feedback_new_note = eof_legacy_track_add_note(eof_song->legacy_track[tracknum]);
				eof_legacy_track_note_create(eof_feedback_new_note, 0, 1, 0, 0, 0, 0, eof_music_pos - eof_av_delay, 1);
				eof_feedback_new_note->type = eof_note_type;
				eof_selection.current_pos = eof_feedback_new_note->pos;
				eof_selection.track = eof_selected_track;
				memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
				eof_sort_notes(eof_song);
				eof_fixup_notes(eof_song);
				eof_determine_phrase_status();
				eof_selection.multi[eof_selection.current] = 1;
			}
			eof_feedback_note[1] = 1;
		}
		else if(eof_feedback_note[1] == 0)
		{
			eof_feedback_new_note = eof_song->legacy_track[tracknum]->note[eof_hover_note];
			eof_song->legacy_track[tracknum]->note[i]->note ^= 2;
			eof_feedback_note[1] = 2;
		}
	}
	else
	{
		eof_feedback_note[1] = 0;
	}
	if(key[KEY_3])
	{
		if(eof_hover_note < 0)
		{
			if(!eof_feedback_new_note)
			{
				eof_feedback_new_note = eof_legacy_track_add_note(eof_song->legacy_track[tracknum]);
				eof_legacy_track_note_create(eof_feedback_new_note, 0, 0, 1, 0, 0, 0, eof_music_pos - eof_av_delay, 1);
				eof_feedback_new_note->type = eof_note_type;
				eof_selection.current_pos = eof_feedback_new_note->pos;
				eof_selection.track = eof_selected_track;
				memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
				eof_sort_notes(eof_song);
				eof_fixup_notes(eof_song);
				eof_determine_phrase_status();
				eof_selection.multi[eof_selection.current] = 1;
			}
			eof_feedback_note[2] = 1;
		}
		else if(eof_feedback_note[2] == 0)
		{
			eof_feedback_new_note = eof_song->legacy_track[tracknum]->note[eof_hover_note];
			eof_song->legacy_track[tracknum]->note[i]->note ^= 4;
			eof_feedback_note[2] = 2;
		}
	}
	else
	{
		eof_feedback_note[2] = 0;
	}
	if(key[KEY_4])
	{
		if(eof_hover_note < 0)
		{
			if(!eof_feedback_new_note)
			{
				eof_feedback_new_note = eof_legacy_track_add_note(eof_song->legacy_track[tracknum]);
				eof_legacy_track_note_create(eof_feedback_new_note, 0, 0, 0, 1, 0, 0, eof_music_pos - eof_av_delay, 1);
				eof_feedback_new_note->type = eof_note_type;
				eof_selection.current_pos = eof_feedback_new_note->pos;
				eof_selection.track = eof_selected_track;
				memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
				eof_sort_notes(eof_song);
				eof_fixup_notes(eof_song);
				eof_determine_phrase_status();
				eof_selection.multi[eof_selection.current] = 1;
			}
			eof_feedback_note[3] = 1;
		}
		else if(eof_feedback_note[3] == 0)
		{
			eof_feedback_new_note = eof_song->legacy_track[tracknum]->note[eof_hover_note];
			eof_song->legacy_track[tracknum]->note[i]->note ^= 8;
			eof_feedback_note[3] = 2;
		}
	}
	else
	{
		eof_feedback_note[3] = 0;
	}
	if(key[KEY_5])
	{
		if(eof_hover_note < 0)
		{
			if(!eof_feedback_new_note)
			{
				eof_feedback_new_note = eof_legacy_track_add_note(eof_song->legacy_track[tracknum]);
				eof_legacy_track_note_create(eof_feedback_new_note, 0, 0, 0, 0, 1, 0, eof_music_pos - eof_av_delay, 1);
				eof_feedback_new_note->type = eof_note_type;
				eof_selection.current_pos = eof_feedback_new_note->pos;
				eof_selection.track = eof_selected_track;
				memset(eof_selection.multi, 0, sizeof(eof_selection.multi));	//Clear the selected notes array
				eof_sort_notes(eof_song);
				eof_fixup_notes(eof_song);
				eof_determine_phrase_status();
				eof_selection.multi[eof_selection.current] = 1;
			}
			eof_feedback_note[4] = 1;
		}
		else if(eof_feedback_note[4] == 0)
		{
			eof_feedback_new_note = eof_song->legacy_track[tracknum]->note[eof_hover_note];
			eof_song->legacy_track[tracknum]->note[eof_hover_note]->note ^= 16;
			eof_feedback_note[4] = 2;
		}
	}
	else
	{
		eof_feedback_note[4] = 0;
	}

	/* not placing notes */
	if(!key[KEY_1] && !key[KEY_2] && !key[KEY_3] && !key[KEY_4] && !key[KEY_5])
	{
		eof_feedback_new_note = NULL;
	}

	if((eof_hover_note >= 0) && !eof_song->legacy_track[tracknum]->note[eof_hover_note]->note)
	{
		eof_track_delete_note(eof_song, eof_selected_track, eof_hover_note);
		eof_sort_notes(eof_song);
		eof_fixup_notes(eof_song);
		eof_determine_phrase_status();
		eof_selection.current = EOF_MAX_NOTES - 1;
		eof_selection.multi[eof_hover_note] = 0;
	}
}

struct FeedbackChart *ImportFeedback(char *filename, int *error)
{
	eof_log("ImportFeedback() entered", 1);

	PACKFILE *inf=NULL;
	char songparsed=0,syncparsed=0,eventsparsed=0;
		//Flags to indicate whether each of the mentioned sections had already been parsed
	char currentsection=0;					//Will be set to 1 for [Song], 2 for [SyncTrack], 3 for [Events] or 4 for an instrument section
	unsigned long maxlinelength=0;			//I will count the length of the longest line (including NULL char/newline) in the
	char *buffer=NULL,*buffer2=NULL;		//Will be an array large enough to hold the largest line of text from input file
	unsigned long index=0,index2=0;			//Indexes for buffer and buffer2, respectively
	char *substring=NULL,*substring2=NULL;	//Used with strstr() to find tag strings in the input file
	unsigned long A=0,B=0,C=0;				//The first, second and third integer values read from the current line of the file
	int errorstatus=0;						//Passed to ParseLongInt()
	char anchortype=0;						//The achor type being read in [SyncTrack]
	char notetype=0;						//The note type being read in the instrument track
	char *string1=NULL,*string2=NULL;		//Used to hold strings parsed with Read_dB_string()

//Feedback chart structure variables
	struct FeedbackChart *chart=NULL;
	struct dBAnchor *curanchor=NULL;		//Conductor for the anchor linked list
	struct dbText *curevent=NULL;			//Conductor for the text event linked list
	struct dbNote *curnote=NULL;			//Conductor for the current instrument track's note linked list
	struct dbTrack *curtrack=NULL;			//Conductor for the instrument track linked list, which contains a linked list of notes
	void *temp=NULL;						//Temporary pointer used for storing newly-allocated memory
	static struct FeedbackChart emptychart;	//A static struct has all members auto-initialized to 0/NULL
	static struct dBAnchor emptyanchor;
	static struct dbText emptytext;
	static struct dbNote emptynote;

//Open file in text mode
	if(!filename)
	{
		return NULL;
	}
	inf=pack_fopen(filename,"rt");
	if(inf == NULL)
	{
		if(error)
			*error=1;
		snprintf(eof_log_string, sizeof(eof_log_string), "\tError loading:  Cannot open input .chart file:  \"%s\"", strerror(errno));	//Get the Operating System's reason for the failure
		eof_log(eof_log_string, 1);
		return NULL;
	}

//Initialize chart structure
	chart=(struct FeedbackChart *)malloc_err(sizeof(struct FeedbackChart));	//Allocate memory
	*chart=emptychart;		//Reliably set all member variables to 0/NULL
	chart->resolution=192;	//Default this to 192

//Allocate memory buffers large enough to hold any line in this file
	maxlinelength=FindLongestLineLength_ALLEGRO(filename,1);
	buffer=(char *)malloc_err(maxlinelength);
	buffer2=(char *)malloc_err(maxlinelength);

//Read first line of text, capping it to prevent buffer overflow
	if(!pack_fgets(buffer,maxlinelength,inf))
	{	//I/O error
		snprintf(eof_log_string, sizeof(eof_log_string), "Feedback import failed on line #%lu:  Unable to read from file:  \"%s\"", chart->linesprocessed, strerror(errno));
		eof_log(eof_log_string, 1);
		if(error)
			*error=2;
		pack_fclose(inf);
		free(buffer);
		free(buffer2);
		return NULL;
	}

//Parse the contents of the file
	while(!pack_feof(inf))		//Until end of file is reached
	{
		chart->linesprocessed++;	//Track which line number is being parsed

//Skip leading whitespace
		index=0;	//Reset index counter to beginning
		while(buffer[index] != '\0')
		{
			if((buffer[index] != '\n') && (isspace((unsigned char)buffer[index])))
				index++;	//If this character is whitespace, skip to next character
			else
				break;
		}

		if((buffer[index] == '\n') || (buffer[index] == '\r') || (buffer[index] == '\0') || (buffer[index] == '{'))
		{	//If this line was empty, or contained characters we're ignoring
			pack_fgets(buffer,maxlinelength,inf);	//Read next line of text, so the EOF condition can be checked, don't exit on EOF
			continue;							//Skip ahead to the next line
		}

//Process section header
		if(buffer[index]=='[')	//If the line begins an open bracket, it identifies the start of a section
		{
			substring2=strchr(buffer,']');		//Find first closing bracket
			if(substring2 == NULL)
			{
				snprintf(eof_log_string, sizeof(eof_log_string), "Feedback import failed on line #%lu:  Malformed section header (no closing bracket)", chart->linesprocessed);
				eof_log(eof_log_string, 1);
				DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
				if(error)
					*error=3;
				return NULL;					//Malformed section header, return error
			}

			if(currentsection != 0)				//If a section is already being parsed
			{
				snprintf(eof_log_string, sizeof(eof_log_string), "Feedback import failed on line #%lu:  Malformed file (section not closed before another begins)", chart->linesprocessed);
				eof_log(eof_log_string, 1);
				DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
				if(error)
					*error=4;
				return NULL;					//Malformed file, return error
			}

			substring=strcasestr_spec(buffer,"Song");	//Case insensitive search, returning pointer to after the match
			if(substring && (substring <= substring2))	//If this line contained "Song" followed by "]"
			{
				if(songparsed != 0)					//If a section with this name was already parsed
				{
					snprintf(eof_log_string, sizeof(eof_log_string), "Feedback import failed on line #%lu:  Malformed file ([Song] section defined more than once)", chart->linesprocessed);
					eof_log(eof_log_string, 1);
					DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
					if(error)
						*error=5;
					return NULL;					//Multiple [song] sections, return error
				}
				songparsed=1;
				currentsection=1;	//Track that we're parsing [Song]
			}
			else
			{
				substring=strcasestr_spec(buffer,"SyncTrack");
				if(substring && (substring <= substring2))	//If this line contained "SyncTrack" followed by "]"
				{
					if(syncparsed != 0)					//If a section with this name was already parsed
					{
						snprintf(eof_log_string, sizeof(eof_log_string), "Feedback import failed on line #%lu:  Malformed file ([SyncTrack] section defined more than once)", chart->linesprocessed);
						eof_log(eof_log_string, 1);
						DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
						if(error)
							*error=6;
						return NULL;					//Multiple [SyncTrack] sections, return error
					}
					syncparsed=1;
					currentsection=2;	//Track that we're parsing [SyncTrack]
				}
				else
				{
					substring=strcasestr_spec(buffer,"Events");
					if(substring && (substring <= substring2))	//If this line contained "Events" followed by "]"
					{
						if(eventsparsed != 0)				//If a section with this name was already parsed
						{
							snprintf(eof_log_string, sizeof(eof_log_string), "Feedback import failed on line #%lu:  Malformed file ([Events] section defined more than once)", chart->linesprocessed);
							eof_log(eof_log_string, 1);
							DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
							if(error)
								*error=7;
							return NULL;					//Multiple [Events] sections, return error
						}
						eventsparsed=1;
						currentsection=3;	//Track that we're parsing [Events]
					}
					else
					{	//This is an instrument section
						temp=(void *)Validate_dB_instrument(buffer);
						if(temp == NULL)					//Not a valid Feedback instrument section name
						{
							snprintf(eof_log_string, sizeof(eof_log_string), "Feedback import failed on line #%lu:  Invalid instrument section \"%s\"", chart->linesprocessed, buffer);
							eof_log(eof_log_string, 1);
							DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
							if(error)
								*error=8;
							return NULL;					//Invalid instrument section, return error
						}
						currentsection=4;
						chart->tracksloaded++;	//Keep track of how many instrument tracks are loaded

					//Create and insert instrument link in the instrument list
						if(chart->tracks == NULL)					//If the list is empty
						{
							chart->tracks=(struct dbTrack *)temp;	//Point head of list to this link
							curtrack=chart->tracks;					//Point conductor to this link
						}
						else
						{
							curtrack->next=(struct dbTrack *)temp;	//Conductor points forward to this link
							curtrack=curtrack->next;				//Point conductor to this link
						}

					//Initialize instrument link
						curnote=NULL;			//Reset the conductor for the track's note list
					}
				}
			}

			pack_fgets(buffer,maxlinelength,inf);	//Read next line of text, so the EOF condition can be checked, don't exit on EOF
			continue;				//Skip ahead to the next line
		}//If the line begins an open bracket...

//Process end of section
		if(buffer[index]=='}')	//If the line begins with a closing curly brace, it is the end of the current section
		{
			if(currentsection == 0)				//If no section is being parsed
			{
				snprintf(eof_log_string, sizeof(eof_log_string), "Feedback import failed on line #%lu:  Malformed file (unexpected closing curly brace)", chart->linesprocessed);
				eof_log(eof_log_string, 1);
				DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
				if(error)
					*error=9;
				return NULL;					//Malformed file, return error
			}
			currentsection=0;
			pack_fgets(buffer,maxlinelength,inf);	//Read next line of text, so the EOF condition can be checked, don't exit on EOF
			continue;							//Skip ahead to the next line
		}

//Process normal line input
		substring=strchr(buffer,'=');		//Any line within the section is expected to contain an equal sign
		if(substring == NULL)
		{
			snprintf(eof_log_string, sizeof(eof_log_string), "Feedback import failed on line #%lu:  Malformed item (missing equal sign)", chart->linesprocessed);
			eof_log(eof_log_string, 1);
			DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
			if(error)
				*error=10;
			return NULL;					//Invalid section entry, return error
		}

	//Process [Song]
		if(currentsection == 1)
		{
			if(Read_dB_string(buffer,&string1,&string2))
			{	//If a valid definition of (string) = (string) or (string) = "(string)" was found
				if(strcasecmp(string1,"Name") == 0)
					chart->name=string2;	//Save the song name tag
				else if(strcasecmp(string1,"Artist") == 0)
					chart->artist=string2;	//Save the song artist tag
				else if(strcasecmp(string1,"Charter") == 0)
					chart->charter=string2;	//Save the chart editor tag
				else if(strcasecmp(string1,"Offset") == 0)
				{
					chart->offset=atof(string2);
					free(string2);	//This string is no longer used
					string2 = NULL;
				}
				else if(strcasecmp(string1,"Resolution") == 0)
				{
					index2=0;	//Use this as an index for string2
					chart->resolution=(unsigned long)ParseLongInt(string2,&index2,chart->linesprocessed,&errorstatus);	//Parse string2 as a number
					if(errorstatus)						//If ParseLongInt() failed
					{
						snprintf(eof_log_string, sizeof(eof_log_string), "Feedback import failed on line #%lu:  Invalid chart resolution", chart->linesprocessed);
						eof_log(eof_log_string, 1);
						DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
						if(error)
							*error=11;
						return NULL;					//Invalid number, return error
					}
					free(string2);	//This string is no longer used
					string2 = NULL;
				}
				else if(strcasecmp(string1,"MusicStream") == 0)
					chart->audiofile=string2;	//Save the name of the audio file for the chart
				else
				{	//No recognized tag was found
					free(string2);
					string2 = NULL;
				}
				free(string1);	//This string is no longer used
				string1 = NULL;
			}
		}

	//Process [SyncTrack]
		else if(currentsection == 2)
		{	//# = ID # is expected
		//Load first number
			A=(unsigned long)ParseLongInt(buffer,&index,chart->linesprocessed,&errorstatus);
			if(errorstatus)						//If ParseLongInt() failed
			{
				snprintf(eof_log_string, sizeof(eof_log_string), "Feedback import failed on line #%lu:  Invalid sync item position", chart->linesprocessed);
				eof_log(eof_log_string, 1);
				DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
				if(error)
					*error=12;
				return NULL;					//Invalid number, return error
			}

		//Skip whitespace and parse to equal sign
			index=0;	//Reset index, to parse an int from after the equal sign
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
				snprintf(eof_log_string, sizeof(eof_log_string), "Feedback import failed on line #%lu:  Malformed sync item (missing equal sign)", chart->linesprocessed);
				eof_log(eof_log_string, 1);
				DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
				if(error)
					*error=13;
				return NULL;				//Invalid SyncTrack entry, return error
			}

		//Skip whitespace and parse to event ID
			while(substring[index] != '\0')
			{
				if((substring[index] != '\n') && (isspace((unsigned char)substring[index])))
					index++;	//If this character is whitespace, skip to next character
				else
					break;
			}

			anchortype=toupper(substring[index]);	//Store as uppercase
			if((anchortype == 'A') || (anchortype == 'B'))
				index++;	//Advance to next whitespace character
			else if(anchortype == 'T')
			{
				if(substring[index+1] != 'S')		//If the next character doesn't complete the anchor type
				{
					snprintf(eof_log_string, sizeof(eof_log_string), "Feedback import failed on line #%lu:  Invalid sync item type \"T%c\"", chart->linesprocessed, substring[index+1]);
					eof_log(eof_log_string, 1);
					DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
					if(error)
						*error=14;
					return NULL;					//Invalid anchor type, return error
				}
				index+=2;	//This anchor type is two characters instead of one
			}
			else
			{
				snprintf(eof_log_string, sizeof(eof_log_string), "Feedback import failed on line #%lu:  Invalid sync item type \"%c\"", chart->linesprocessed, anchortype);
				eof_log(eof_log_string, 1);
				DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
				if(error)
					*error=15;
				return NULL;					//Invalid anchor type, return error
			}

		//Load second number
			B=(unsigned long)ParseLongInt(substring,&index,chart->linesprocessed,&errorstatus);
			if(errorstatus)						//If ParseLongInt() failed
			{
				snprintf(eof_log_string, sizeof(eof_log_string), "Feedback import failed on line #%lu:  Invalid sync item parameter", chart->linesprocessed);
				eof_log(eof_log_string, 1);
				DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
				if(error)
					*error=16;
				return NULL;					//Invalid number, return error
			}

		//If this anchor event has the same chart time as the last, just write this event's information with the last's
			if(curanchor && (curanchor->chartpos == A))
			{
				if(anchortype == 'A')		//If this is an Anchor event
					curanchor->usec=B;		//Store the anchor realtime position
				else if(anchortype == 'B')	//If this is a Tempo event
					curanchor->BPM=B;		//Store the tempo
				else if(anchortype == 'T')	//If this is a Time Signature event
					curanchor->TS=B;		//Store the Time Signature
				else
				{
					snprintf(eof_log_string, sizeof(eof_log_string), "Feedback import failed on line #%lu:  Invalid sync item type \"%c\"", chart->linesprocessed, anchortype);
					eof_log(eof_log_string, 1);
					DestroyFeedbackChart(chart,1);
					if(error)
						*error=17;
					return NULL;			//Invalid anchor type, return error
				}
			}
			else
			{
		//Create and insert anchor link into the anchor list
				temp=malloc_err(sizeof(struct dBAnchor));	//Allocate memory
				*((struct dBAnchor *)temp)=emptyanchor;		//Reliably set all member variables to 0/NULL
				if(chart->anchors == NULL)					//If the list is empty
				{
					chart->anchors=(struct dBAnchor *)temp;	//Point head of list to this link
					curanchor=chart->anchors;				//Point conductor to this link
				}
				else
				{
					curanchor->next=(struct dBAnchor *)temp;	//Conductor points forward to this link
					curanchor=curanchor->next;					//Point conductor to this link
				}

		//Initialize anchor link
				curanchor->chartpos=A;	//The first number read is the chart position
				switch(anchortype)
				{
					case 'B':				//If this was a tempo event
						curanchor->BPM=B;	//The second number represents 1000 times the tempo
					break;

					case 'T':				//If this was a time signature event
						curanchor->TS=B;	//Store the numerator of the time signature
					break;

					case 'A':				//If this was an anchor event
						curanchor->usec=B;	//Store the anchor's timestamp in microseconds
					break;

					default:
						snprintf(eof_log_string, sizeof(eof_log_string), "Feedback import failed on line #%lu:  Invalid sync item type \"%c\"", chart->linesprocessed, anchortype);
						eof_log(eof_log_string, 1);
						DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
						if(error)
							*error=17;
					return NULL;			//Invalid anchor type, return error
				}
			}
		}

	//Process [Events]
		else if(currentsection == 3)
		{	//# = E "(STRING)" is expected
		//Load first number
			A=ParseLongInt(buffer,&index,chart->linesprocessed,&errorstatus);
			if(errorstatus)						//If ParseLongInt() failed
			{
				snprintf(eof_log_string, sizeof(eof_log_string), "Feedback import failed on line #%lu:  Invalid event time position", chart->linesprocessed);
				eof_log(eof_log_string, 1);
				DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
				if(error)
					*error=18;
				return NULL;					//Invalid number, return error
			}

		//Skip whitespace and parse to equal sign
			index=0;	//Reset index, to parse an int from after the equal sign
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
				snprintf(eof_log_string, sizeof(eof_log_string), "Feedback import failed on line #%lu:  Malformed event item (missing equal sign)", chart->linesprocessed);
				eof_log(eof_log_string, 1);
				DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
				if(error)
					*error=19;
				return NULL;				//Invalid Event entry, return error
			}

		//Skip whitespace and parse to event ID
			while(substring[index] != '\0')
			{
				if((substring[index] != '\n') && (isspace((unsigned char)substring[index])))
					index++;	//If this character is whitespace, skip to next character
				else
					break;
			}

			if(substring[index++] != 'E')		//Check if this isn't a "text event" indicator (and increment index)
			{
				snprintf(eof_log_string, sizeof(eof_log_string), "Feedback import failed on line #%lu:  Invalid event item type \"%c\"", chart->linesprocessed, substring[index - 1]);
				eof_log(eof_log_string, 1);
				DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
				if(error)
					*error=20;
				return NULL;					//Invalid Event entry, return error
			}

		//Seek to opening quotation mark
			while((substring[index] != '\0') && (substring[index] != '"'))
				index++;

			if(substring[index++] != '"')		//Check if this was a null character instead of quotation mark (and increment index)
			{
				snprintf(eof_log_string, sizeof(eof_log_string), "Feedback import failed on line #%lu:  Malformed event item (missing open quotation mark)", chart->linesprocessed);
				eof_log(eof_log_string, 1);
				DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
				if(error)
					*error=21;
				return NULL;					//Invalid Event entry, return error
			}

		//Load string by copying all characters to the second buffer (up to the next quotation mark)
			buffer2[0]='\0';	//Truncate string
			index2=0;			//Reset buffer2's index
			while(substring[index] != '"')			//For all characters up to the next quotation mark
			{
				if(substring[index] == '\0')		//If a null character is reached unexpectedly
				{
					snprintf(eof_log_string, sizeof(eof_log_string), "Feedback import failed on line #%lu:  Malformed event item (missing close quotation mark)", chart->linesprocessed);
					eof_log(eof_log_string, 1);
					DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
					if(error)
						*error=22;
					return NULL;					//Invalid Event entry, return error
				}
				buffer2[index2++]=substring[index++];	//Copy the character to the second buffer, incrementing both indexes
			}
			buffer2[index2]='\0';	//Truncate the second buffer to form a complete string

		//Create and insert event link into event list
			temp=malloc_err(sizeof(struct dbText));		//Allocate memory
			*((struct dbText *)temp)=emptytext;			//Reliably set all member variables to 0/NULL
			if(chart->events == NULL)					//If the list is empty
			{
				chart->events=(struct dbText *)temp;	//Point head of list to this link
				curevent=chart->events;					//Point conductor to this link
			}
			else
			{
				curevent->next=(struct dbText *)temp;	//Conductor points forward to this link
				curevent=curevent->next;				//Point conductor to this link
			}

		//Initialize event link- Duplicate buffer2 into a newly created dbText link, adding it to the list
			curevent->chartpos=A;						//The first number read is the chart position
			curevent->text=DuplicateString(buffer2);	//Copy buffer2 to new string and store in list
		}

	//Process instrument tracks
		else if(currentsection == 4)
		{	//"# = N # #" or "# = S # #" is expected
		//Load first number
			A=ParseLongInt(buffer,&index,chart->linesprocessed,&errorstatus);
			if(errorstatus)						//If ParseLongInt() failed
			{
				snprintf(eof_log_string, sizeof(eof_log_string), "Feedback import failed on line #%lu:  Invalid gem item position", chart->linesprocessed);
				eof_log(eof_log_string, 1);
				DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
				if(error)
					*error=23;
				return NULL;					//Invalid number, return error
			}

		//Skip whitespace and parse to equal sign
			index=0;	//Reset index, to parse an int from after the equal sign
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
				snprintf(eof_log_string, sizeof(eof_log_string), "Feedback import failed on line #%lu:  Malformed gem item (missing equal sign)", chart->linesprocessed);
				eof_log(eof_log_string, 1);
				DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
				if(error)
					*error=24;
				return NULL;				//Invalid instrument entry, return error
			}

		//Skip whitespace and parse to event ID
			while(substring[index] != '\0')
			{
				if((substring[index] != '\n') && (isspace((unsigned char)substring[index])))
					index++;	//If this character is whitespace, skip to next character
				else
					break;
			}

			notetype=0;	//By default, assume this is going to be a note definition
			switch(substring[index])
			{
				case 'S':		//Player/overdrive section
					notetype=1;	//This is a section marker
					index++; 	//Increment index past N or S identifier
				break;

				case 'E':		//Text event, skip it
					pack_fgets(buffer,maxlinelength,inf);	//Read next line of text, so the EOF condition can be checked, don't exit on EOF
				continue;

				case 'N':		//Note indicator, and increment index
					index++; 	//Increment index past N or S identifier
				break;

				default:
					snprintf(eof_log_string, sizeof(eof_log_string), "Feedback import failed on line #%lu:  Invalid gem item type \"%c\"", chart->linesprocessed, substring[index]);
					eof_log(eof_log_string, 1);
					DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
					if(error)
						*error=25;
				return NULL;		//Invalid instrument entry, return error
			}

		//Load second number
			B=ParseLongInt(substring,&index,chart->linesprocessed,&errorstatus);
			if(errorstatus)						//If ParseLongInt() failed
			{
				snprintf(eof_log_string, sizeof(eof_log_string), "Feedback import failed on line #%lu:  Invalid gem item parameter 1", chart->linesprocessed);
				eof_log(eof_log_string, 1);
				DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
				if(error)
					*error=26;
				return NULL;					//Invalid number, return error
			}

		//Load third number
			C=ParseLongInt(substring,&index,chart->linesprocessed,&errorstatus);
			if(errorstatus)						//If ParseLongInt() failed
			{
				snprintf(eof_log_string, sizeof(eof_log_string), "Feedback import failed on line #%lu:  Invalid gem item parameter 2", chart->linesprocessed);
				eof_log(eof_log_string, 1);
				DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
				if(error)
					*error=27;
				return NULL;					//Invalid number, return error
			}

		//Create a note link and add it to the current Note list
			if(curtrack == NULL)				//If the instrument track linked list is not initialized
			{
				snprintf(eof_log_string, sizeof(eof_log_string), "Feedback import failed on line #%lu:  Malformed file (gem defined outside instrument track)", chart->linesprocessed);
				eof_log(eof_log_string, 1);
				DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
				if(error)
					*error=28;
				return NULL;					//Malformed file, return error
			}

			temp=malloc_err(sizeof(struct dbNote));		//Allocate memory
			*((struct dbNote *)temp)=emptynote;			//Reliably set all member variables to 0/NULL
			if(curtrack->notes == NULL)					//If the list is empty
			{
				curtrack->notes=(struct dbNote *)temp;	//Point head of list to this link
				curnote=curtrack->notes;				//Point conductor to this link
			}
			else
			{
				curnote->next=(struct dbNote *)temp;	//Conductor points forward to this link
				curnote=curnote->next;					//Point conductor to this link
			}

		//Initialize note link
			curnote->chartpos=A;	//The first number read is the chart position

			if(!notetype)				//This was a note definition
				curnote->gemcolor=B;	//The second number read is the gem color
			else						//This was a player section marker
			{
				if(B > 4)							//Only values of 0, 1 or 2 are valid for player section markers, 3 and 4 are unknown but will be kept and ignored for now during transfer to EOF
				{
					snprintf(eof_log_string, sizeof(eof_log_string), "Feedback import failed on line #%lu:  Invalid section marker #%lu", chart->linesprocessed, B);
					eof_log(eof_log_string, 1);
					DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
					if(error)
						*error=29;
					return NULL;					//Invalid player section marker, return error
				}
				curnote->gemcolor='0'+B;	//Store 0 as '0', 1 as '1' or 2 as '2', ...
			}
			curnote->duration=C;			//The third number read is the note duration
		}

	//Error: Content in file outside of a defined section
		else
		{
			snprintf(eof_log_string, sizeof(eof_log_string), "Feedback import failed on line #%lu:  Malformed file (item defined outside of track)", chart->linesprocessed);
			eof_log(eof_log_string, 1);
			DestroyFeedbackChart(chart,1);	//Destroy the chart and its contents
			if(error)
				*error=30;
			return NULL;					//Malformed file, return error
		}

		pack_fgets(buffer,maxlinelength,inf);	//Read next line of text, so the EOF condition can be checked, don't exit on EOF
	}//Until end of file is reached

	if(error)
		*error=0;

	free(buffer);
	free(buffer2);
	pack_fclose(inf);

	return chart;
}

int Read_dB_string(char *source,char **str1, char **str2)
{
	//Scans the source string for a valid dB tag: text = text	or	text = "text"
	//The text to the left of the equal sign is returned through str1 as a new string, with whitespace truncated
	//The text to the right of the equal sign is returned through str2 as a new string, with whitespace truncated
	//If the first non whitespace character encountered after the equal sign is a quotation mark, all characters after
	//that quotation mark up to the next are returned through str2
	//Nonzero is returned upon success, or zero is returned if source did not contain two sets of non whitespace characters
	//separated by an equal sign character, or if the closing quotation mark is missing.

//	eof_log("Read_dB_string() entered");

	unsigned long srcindex;	//Index into source string
	unsigned long index;	//Index into destination strings
	char *string1,*string2;	//Working strings
	char findquote=0;	//Boolean:	The element to the right of the equal sign began with a quotation mark
				//		str2 will stop filling with characters when the next quotation mark is read

	if(!source || !str1 || !str2)
	{
		return 0;	//return error
	}

//Allocate memory for strings
	string1=malloc_err(strlen(source)+1);
	string2=malloc_err(strlen(source)+2);

//Parse the string to the left of the expected equal sign into string1
	index=0;
	for(srcindex=0;source[srcindex] != '=';srcindex++)	//Parse characters until equal sign is found
	{
		if(source[srcindex] == '\0')	//If the string ended unexpectedly
		{
			free(string1);
			free(string2);
			return 0;		//return error
		}

		string1[index++]=source[srcindex];	//Append character to string1 and increment index into string
	}
	string1[index]='\0';	//Truncate string1
	srcindex++;		//Seek past the equal sign
	string1=TruncateString(string1,1);	//Re-allocate string to remove leading and trailing whitespace

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
	index=0;
	while(source[srcindex] != '\0')	//There was nothing but whitespace after the equal sign
	{
		if(findquote && (source[srcindex] == '"'))	//If we should stop at this quotation mark
			break;
		string2[index++]=source[srcindex++];	//Append character to string1 and increment both indexes
	}

	string2[index]='\0';	//Truncate string2

	if(index)	//If there were characters copied to string2
		string2=TruncateString(string2,1);	//Re-allocate string to remove the trailing whitespace

	*str1=string1;		//Return string1 through pointer
	*str2=string2;		//Return string2 through pointer
	return 1;		//Return success
}

struct dbTrack *Validate_dB_instrument(char *buffer)
{
	//Validates that buffer contains a valid dB instrument track name enclosed in brackets []
	//buffer is expected to point to the opening bracket
	//If it is valid, a dbTrack structure is allocated and initialized:
	//(track name is allocated, tracktype and difftype are set and the linked lists are set to NULL)
	//The track structure is returned, otherwise NULL is returned if the string did not contain a valid
	//track name.  buffer[] is modified to remove any whitespace after the closing bracket

	eof_log("Validate_dB_instrument() entered", 1);

	unsigned long index=0;	//Used to index into buffer
	char *endbracket=NULL;	//The pointer to the end bracket
	char *diffstring=NULL;	//Used to find the difficulty substring
	char *inststring=NULL;	//Used to find the instrument substring
	char *retstring=NULL;	//Used to create the instrument track string
	struct dbTrack *chart=NULL;		//Used to create the structure that is returned
	static struct dbTrack emptychart;	//Static structures have all members auto initialize to 0/NULL
	char tracktype=0,difftype=0;	//Used to track the instrument type and difficulty of the track, based on the name
	char isguitar=0,isdrums=0;		//Used to track secondary/tertiary guitar/drum tracks

	if(buffer == NULL)
		return NULL;	//Return error

//Validate the opening bracket, to which buffer is expected to point
	if(buffer[0] != '[')	//If the opening bracket is missing
		return NULL;	//Return error

//Validate the presence of the closing bracket
	endbracket=strchr(buffer,']');
	if(endbracket == NULL)
		return NULL;	//Return error

//Verify that no non whitespace characters exist after the closing bracket
	index=1;	//Reset index to point to the character after the closing bracket
	while(endbracket[index] != '\0')
		if(!isspace(endbracket[index++]))	//Check if this character isn't whitespace (and increment index)
			return NULL;			//If it isn't whitespace, return error

//Truncate whitespace after closing bracket
	endbracket[1]='\0';	//Write a NULL character after the closing bracket

//Verify that a valid diffulty is specified, seeking past the opening bracket pointed to by buffer[0]
	//Test for Easy
	diffstring=strcasestr_spec(&buffer[1],"Easy");
	if(diffstring == NULL)
	{
	//Test for Medium
		diffstring=strcasestr_spec(&buffer[1],"Medium");
		if(diffstring == NULL)
		{
	//Test for Hard
			diffstring=strcasestr_spec(&buffer[1],"Hard");
			if(diffstring == NULL)
			{
	//Test for Expert
				diffstring=strcasestr_spec(&buffer[1],"Expert");
				if(diffstring == NULL)	//If none of the four valid difficulty strings were found
					return NULL;	//Return error
				else
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

//At this point, diffstring points to the character AFTER the matching difficulty string.  Verify that a valid instrument is specified
	//Test for Single (Guitar)
		inststring=strcasestr_spec(diffstring,"Single]");
		if(inststring == NULL)
		{
	//Test for DoubleGuitar (Lead Guitar)
			inststring=strcasestr_spec(diffstring,"DoubleGuitar]");
			if(inststring == NULL)
			{
	//Test for DoubleBass (Bass)
				inststring=strcasestr_spec(diffstring,"DoubleBass]");
				if(inststring == NULL)
				{
	//Test for EnhancedGuitar
					inststring=strcasestr_spec(diffstring,"EnhancedGuitar]");
					if(inststring == NULL)
					{
	//Test for CoopLead
						inststring=strcasestr_spec(diffstring,"CoopLead]");
						if(inststring == NULL)
						{
	//Test for CoopBass
							inststring=strcasestr_spec(diffstring,"CoopBass]");
							if(inststring == NULL)
							{
	//Test for 10KeyGuitar
								inststring=strcasestr_spec(diffstring,"10KeyGuitar]");
								if(inststring == NULL)
								{
	//Test for Drums
									inststring=strcasestr_spec(diffstring,"Drums]");
									if(inststring == NULL)
									{
	//Test for DoubleDrums (Expert+ drums)
										inststring=strcasestr_spec(diffstring,"DoubleDrums]");
										if(inststring == NULL)
										{
	//Test for Vocals (Vocal Rhythm)
											inststring=strcasestr_spec(diffstring,"Vocals]");
											if(inststring == NULL)
											{
	//Test for Keyboard
												inststring=strcasestr_spec(diffstring,"Keyboard]");
												if(inststring == NULL)	//If none of the valid instrument names were found
												{
	//Test for SingleGuitar (Guitar)
													inststring=strcasestr_spec(diffstring,"SingleGuitar]");	//This is another track name that can represent "Single"
													if(inststring == NULL)
													{
	//Test for SingleRhythm (Bass)
														inststring=strcasestr_spec(diffstring,"SingleRhythm]");	//This is another track name that can represent "DoubleBass"
														if(inststring == NULL)
														{
	//Test for DoubleRhythm (Rhythm)
															inststring=strcasestr_spec(diffstring,"DoubleRhythm]");
															if(inststring == NULL)
															{		//If none of the valid instrument names were found
																return NULL;	//Return error
															}
															else
															{
																tracktype=6;	//Track that this is a "Rhythm" track
																isguitar=1;
															}
														}
														else
														{
															tracktype=3;	//Track that this is a "Bass" track
															isguitar=1;
														}
													}
													else
													{
														tracktype=1;	//Track that this is a "Guitar" track
														isguitar=1;
													}
												}
											}
											else
												tracktype=5;	//Track that this is a "Vocals" track
										}
										else
											isdrums=1;	//DoubleDrums is a drums track
									}
									else
									{
										tracktype=4;	//Track that this is a "Drums" track
										isdrums=1;
									}
								}
								else
									isguitar=1;	//10KeyGuitar is a guitar track
							}
							else
								isguitar=1;	//CoopBass is a guitar track
						}
						else
							isguitar=1;	//CoopLead is a guitar track
					}
					else
						isguitar=1;		//EnhancedGuitar is a guitar track
				}
				else
				{
					tracktype=3;	//Track that this is a "Bass" track
					isguitar=1;
				}
			}
			else
			{
				tracktype=2;	//Track that this is a "Lead Guitar" track
				isguitar=1;
			}
		}
		else
		{
			tracktype=1;	//Track that this is a "Guitar" track
			isguitar=1;
		}

//Validate that the character immediately after the instrument substring is the NULL terminator
	if(inststring[0] != '\0')
		return NULL;

//Create a new string containing the instrument name, minus the brackets
	retstring=DuplicateString(&buffer[1]);
	retstring[strlen(retstring)-1]='\0';	//Truncate the trailing bracket

//Create and initialize the instrument structure
	chart=malloc_err(sizeof(struct dbTrack));	//Allocate memory
	*chart=emptychart;							//Reliably set all member variables to 0/NULL
	chart->trackname=retstring;					//Store the instrument track name
	chart->tracktype=tracktype;
	chart->difftype=difftype;
	return chart;
}

void DestroyFeedbackChart(struct FeedbackChart *ptr, char freestruct)
{
	eof_log("DestroyFeedbackChart() entered", 1);

	struct dBAnchor *anchorptr;	//Conductor for the anchors linked list
	struct dbText *eventptr;	//Conductor for the events linked list
	struct dbTrack *trackptr;	//Conductor for the tracks linked list
	struct dbNote *noteptr;	//Conductor for the notes linked lists

	if(!ptr)
	{
		return;
	}

//Free and re-init tags
	if(ptr->name)
	{
		free(ptr->name);
		ptr->name=NULL;
	}
	if(ptr->artist)
	{
		free(ptr->artist);
		ptr->artist=NULL;
	}
	if(ptr->charter)
	{
		free(ptr->charter);
		ptr->charter=NULL;
	}
	if(ptr->audiofile)
	{
		free(ptr->audiofile);
		ptr->audiofile=NULL;
	}

//Re-init variables
	ptr->offset=ptr->resolution=ptr->linesprocessed=ptr->tracksloaded=0;

//Empty anchors list
	while(ptr->anchors != NULL)
	{
		anchorptr=ptr->anchors->next;	//Store link to next anchor
		free(ptr->anchors);		//Free current anchor
		ptr->anchors=anchorptr;	//Point to next anchor
	}

//Empty events list
	while(ptr->events != NULL)
	{
		eventptr=ptr->events->next;	//Store link to next event
		free(ptr->events->text);	//Free event text
		free(ptr->events);		//Free current event
		ptr->events=eventptr;		//Point to next event
	}

//Empty tracks list
	while(ptr->tracks != NULL)
	{
		trackptr=ptr->tracks->next;	//Store link to next instrument track

		while(ptr->tracks->notes != NULL)
		{
			noteptr=ptr->tracks->notes->next;	//Store link to next note
			free(ptr->tracks->notes);		//Free current note
			ptr->tracks->notes=noteptr;		//Point to next note
		}

		free(ptr->tracks->trackname);	//Free track name
		free(ptr->tracks);		//Free current track
		ptr->tracks=trackptr;		//Point to next track
	}

//Optionally free the passed Feedback chart structure itself
	if(freestruct)
		free(ptr);
}

unsigned long FindLongestLineLength_ALLEGRO(char *filename,char exit_on_empty)
{
	unsigned long maxlinelength=0;
	unsigned long ctr=0;
	int inputchar=0;
	PACKFILE *inf=NULL;

	assert_wrapper(filename != NULL);	//This must not be NULL
	inf = pack_fopen(filename, "rt");
	if(inf == NULL)
		exit_wrapper(1);	//File open failed

	do{
		ctr=0;			//Reset line length counter
		do{
			inputchar=pack_getc(inf);	//get a character, do not exit on EOF
			ctr++;					//increment line length counter
		}while((inputchar != EOF) && (inputchar != '\n'));//Repeat until end of file or newline character is read

		if(ctr > maxlinelength)		//If this line was the longest yet,
			maxlinelength=ctr;	//Store its length
	}while(inputchar != EOF);	//Repeat until end of file is reached

	if(maxlinelength < 2)		//If the input file contained nothing but empty lines or no text at all
	{
		if(!exit_on_empty)
		{
			pack_fclose(inf);
			return 0;
		}
		else
		{
			puts("Error: File is empty\nAborting");
			exit_wrapper(2);
		}
	}
	maxlinelength++;		//Must increment this to account for newline character

	pack_fclose(inf);
	return maxlinelength;
}
