#include <allegro.h>
#include <assert.h>
#include "modules/ocd3d.h"
#include "main.h"
#include "bf.h"	//For eof_pro_guitar_note_lookup_string_fingering()
#include "dr.h"
#include "note.h"
#include "rs.h"	//For eof_note_has_high_chord_density()
#include "tuning.h"
#include "menu/track.h"	//For tech view functions

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

char eof_last_tab_notation[65] = {0};	//Used to store a copy of the results of the last call to eof_get_note_notation(), so repeated notation can be summarized

unsigned long eof_note_count_colors(EOF_SONG *sp, unsigned long track, unsigned long note)
{
//	eof_log("eof_note_count_colors() entered", 3);

	if(!sp)
	{
		return 0;
	}

	return eof_note_count_colors_bitmask(eof_get_note_note(sp, track, note));
}

unsigned long eof_note_count_colors_bitmask(unsigned long notemask)
{
	unsigned long count = 0;

	if(notemask & 1)
	{
		count++;
	}
	if(notemask & 2)
	{
		count++;
	}
	if(notemask & 4)
	{
		count++;
	}
	if(notemask & 8)
	{
		count++;
	}
	if(notemask & 16)
	{
		count++;
	}
	if(notemask & 32)
	{
		count++;
	}
	if(notemask & 64)
	{
		count++;
	}
	if(notemask & 128)
	{
		count++;
	}
	return count;
}

unsigned long eof_note_count_rs_lanes(EOF_SONG *sp, unsigned long track, unsigned long note, char target)
{
	unsigned long ctr, bitmask, tracknum, count = 0, notenote;
	EOF_PRO_GUITAR_TRACK *tp;

	if(!sp || !track || (track >= sp->tracks))
		return 0;	//Invalid parameters

	notenote = eof_get_note_note(sp, track, note);
	if(sp->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
	{	//If the specified track is not a pro guitar track
		return eof_note_count_colors_bitmask(notenote);
	}

	tracknum = sp->track[track]->tracknum;
	tp = sp->pro_guitar_track[tracknum];	//Simplify
	if(note >= tp->notes)
	{	//If the specified note is higher than the number of notes in the track
		return 0;	//Invalid parameters
	}
	for(ctr = 0, bitmask = 1; ctr < 6; ctr++, bitmask <<= 1)
	{	//For each of the 6 supported strings
		if(tp->note[note]->note & bitmask)
		{	//If this string is used
			if(!(tp->note[note]->ghost & bitmask) || (target & 4))
			{	//If the string is not ghosted, or if the target parameter allows ghost gems to be counted
				if(((tp->note[note]->frets[ctr] & 0x80) == 0) || (target & 2))
				{	//If the note is not string muted, or the target game is Rocksmith 2 (which supports string mutes)
					count++;	//Increment counter
				}
			}
		}
	}

	return count;
}

int eof_adjust_notes(unsigned long track, int offset)
{
	unsigned long i, j, tracknum;
	EOF_PHRASE_SECTION *phraseptr = NULL;
	EOF_PRO_GUITAR_TRACK *tp;
	unsigned long neg_offset = 0;

	eof_log("eof_adjust_notes() entered", 1);

	if(!eof_song || (track < 1) || ((track != ULONG_MAX) && (track >= eof_song->tracks)))
		return 0;	//Invalid parameters

	if(track == ULONG_MAX)
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tOffsetting all content by %dms", offset);
	}
	else
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tOffsetting \"%s\" content by %dms", eof_song->track[track]->name, offset);
	}
	eof_log(eof_log_string, 2);

	if(offset < 0)
	{	//If content is being made earlier
		neg_offset = -offset;	//Store the absolute value to use as a check that nothing is to be made a negative timestamp
	}
	for(i = 1; i < eof_song->tracks; i++)
	{	//For each track
		char restore_tech_view = 0;		//If tech view is in effect, it is temporarily disabled until after the notes have been stored

		if((track != ULONG_MAX) && (track != i))	//If this track isn't in the scope of the operation
			continue;								//Skip it

		restore_tech_view = eof_menu_track_get_tech_view_state(eof_song, i);
		eof_menu_track_set_tech_view_state(eof_song, i, 0);	//Disable tech view if applicable

		if(track == ULONG_MAX)
		{
			(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tProcessing track \"%s\"", eof_song->track[i]->name);
			eof_log(eof_log_string, 2);
		}

		//Offset the regular notes
		for(j = 0; j < eof_get_track_size(eof_song, i); j++)
		{	//For each note in the track
			if(eof_get_note_pos(eof_song, i, j) < neg_offset)
			{
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tCan't offset note at %lums to be before 0s", eof_get_note_pos(eof_song, i, j));
				eof_log(eof_log_string, 1);
				return 0;	//Negative timestamps not allowed
			}

			eof_set_note_pos(eof_song, i, j, eof_get_note_pos(eof_song, i, j) + offset);	//Add the offset to the note's position
		}
		//Offset the tech notes
		if(eof_song->track[i]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If this is a pro guitar track
			eof_menu_track_set_tech_view_state(eof_song, i, 1);	//Enable tech view if applicable
			for(j = 0; j < eof_get_track_size(eof_song, i); j++)
			{	//For each note in the track
				if(eof_get_note_pos(eof_song, i, j) < neg_offset)
				{
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tCan't offset tech note at %lums to be before 0s", eof_get_note_pos(eof_song, i, j));
					eof_log(eof_log_string, 1);
					return 0;	//Negative timestamps not allowed
				}

				eof_set_note_pos(eof_song, i, j, eof_get_note_pos(eof_song, i, j) + offset);	//Add the offset to the note's position
			}
			eof_menu_track_set_tech_view_state(eof_song, i, restore_tech_view);	//Restore the track's original tech view state
		}

		for(j = 0; j < eof_get_num_solos(eof_song, i); j++)
		{	//For each solo section in the track
			phraseptr = eof_get_solo(eof_song, i, j);

			if(phraseptr->start_pos < neg_offset)
			{
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tCan't offset solo at %lums to be before 0s", phraseptr->start_pos);
				eof_log(eof_log_string, 1);
				return 0;	//Negative timestamps not allowed
			}

			phraseptr->start_pos += offset;
			phraseptr->end_pos += offset;
		}
		for(j = 0; j < eof_get_num_star_power_paths(eof_song, i); j++)
		{	//For each star power path in the track
			phraseptr = eof_get_star_power_path(eof_song, i, j);

			if(phraseptr->start_pos < neg_offset)
			{
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tCan't offset star power at %lums to be before 0s", phraseptr->start_pos);
				eof_log(eof_log_string, 1);
				return 0;	//Negative timestamps not allowed
			}

			phraseptr->start_pos += offset;
			phraseptr->end_pos += offset;
		}
		for(j = 0; j < eof_get_num_trills(eof_song, i); j++)
		{	//For each trill phrase in the track
			phraseptr = eof_get_trill(eof_song, i, j);

			if(phraseptr->start_pos < neg_offset)
			{
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tCan't offset trill at %lums to be before 0s", phraseptr->start_pos);
				eof_log(eof_log_string, 1);
				return 0;	//Negative timestamps not allowed
			}

			phraseptr->start_pos += offset;
			phraseptr->end_pos += offset;
		}
		for(j = 0; j < eof_get_num_tremolos(eof_song, i); j++)
		{	//For each tremolo phrase in the track
			phraseptr = eof_get_tremolo(eof_song, i, j);

			if(phraseptr->start_pos < neg_offset)
			{
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tCan't offset tremolo at %lums to be before 0s", phraseptr->start_pos);
				eof_log(eof_log_string, 1);
				return 0;	//Negative timestamps not allowed
			}

			phraseptr->start_pos += offset;
			phraseptr->end_pos += offset;
		}
		for(j = 0; j < eof_get_num_arpeggios(eof_song, i); j++)
		{	//For each arpeggio phrase in the track
			phraseptr = eof_get_arpeggio(eof_song, i, j);

			if(phraseptr->start_pos < neg_offset)
			{
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tCan't offset arpeggio at %lums to be before 0s", phraseptr->start_pos);
				eof_log(eof_log_string, 1);
				return 0;	//Negative timestamps not allowed
			}

			phraseptr->start_pos += offset;
			phraseptr->end_pos += offset;
		}
		for(j = 0; j < eof_get_num_sliders(eof_song, i); j++)
		{	//For each slider phrase in the track
			phraseptr = eof_get_slider(eof_song, i, j);

			if(phraseptr->start_pos < neg_offset)
			{
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tCan't offset slider at %lums to be before 0s", phraseptr->start_pos);
				eof_log(eof_log_string, 1);
				return 0;	//Negative timestamps not allowed
			}

			phraseptr->start_pos += offset;
			phraseptr->end_pos += offset;
		}
		for(j = 0; j < eof_get_num_lyric_sections(eof_song, i); j++)
		{	//For each lyric phrase in the track
			phraseptr = eof_get_lyric_section(eof_song, i, j);

			if(phraseptr->start_pos < neg_offset)
			{
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tCan't offset lyric phrase at %lums to be before 0s", phraseptr->start_pos);
				eof_log(eof_log_string, 1);
				return 0;	//Negative timestamps not allowed
			}

			phraseptr->start_pos += offset;
			phraseptr->end_pos += offset;
		}

		if(eof_song->track[i]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
			continue;	//If this isn't a pro guitar track, skip the logic below to adjust pro guitar specific items

		tracknum = eof_song->track[i]->tracknum;
		tp = eof_song->pro_guitar_track[tracknum];

		for(j = 0; j < tp->handpositions; j++)
		{	//For each fret hand position in the track (only change the start_pos variable, end_pos stores the fret position and not a timestamp)
			if(tp->handposition[j].start_pos < neg_offset)
			{
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tCan't offset hand position at %lums to be before 0s", tp->handposition[j].start_pos);
				eof_log(eof_log_string, 1);
				return 0;	//Negative timestamps not allowed
			}

			tp->handposition[j].start_pos += offset;
		}
		for(j = 0; j < tp->popupmessages; j++)
		{	//For each popup message in the track
			if(tp->popupmessage[j].start_pos < neg_offset)
			{
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tCan't offset popup message at %lums to be before 0s", tp->popupmessage[j].start_pos);
				eof_log(eof_log_string, 1);
				return 0;	//Negative timestamps not allowed
			}

			tp->popupmessage[j].start_pos += offset;
			tp->popupmessage[j].end_pos += offset;
		}
		for(j = 0; j < tp->tonechanges; j++)
		{	//For each tone change in the track (only change the start_pos variable, end_pos is unused)
			if(tp->tonechange[j].start_pos < neg_offset)
			{
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tCan't offset tone change at %lums to be before 0s", tp->tonechange[j].start_pos);
				eof_log(eof_log_string, 1);
				return 0;	//Negative timestamps not allowed
			}

			tp->tonechange[j].start_pos += offset;
		}
	}

	if(track == ULONG_MAX)
	{	//If the entire chart is being altered
		//Update fret catalog entries
		for(i = 0; i < eof_song->catalog->entries; i++)
		{
			if(eof_song->catalog->entry[i].start_pos < neg_offset)
			{
				(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tCan't offset catalog entry at %lums to be before 0s", eof_song->catalog->entry[i].start_pos);
				eof_log(eof_log_string, 1);
				return 0;	//Negative timestamps not allowed
			}

			eof_song->catalog->entry[i].start_pos += offset;
			eof_song->catalog->entry[i].end_pos += offset;
		}
		//Update bookmarks
		for(i = 0; i < EOF_MAX_BOOKMARK_ENTRIES; i++)
		{
			if(eof_song->bookmark_pos[i] != 0)
			{
				if(eof_song->bookmark_pos[i] < neg_offset)
				{
					(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\t\tCan't offset bookmark at %lums to be before 0s", eof_song->bookmark_pos[i]);
					eof_log(eof_log_string, 1);
					return 0;	//Negative timestamps not allowed
				}

				eof_song->bookmark_pos[i] += offset;
			}
		}
	}
	return 1;
}

int eof_note_draw(unsigned long track, unsigned long notenum, int p, EOF_WINDOW *window)
{
	long position;	//This is the position for the specified window's piano roll and is based on the passed window pointer
	long leftcoord;	//This is the position of the left end of the piano roll
	long pos;		//This is the position of the specified window's piano roll, scaled by the current zoom level
	long npos;
	int pcol = p == 2 ? makecol(224, 255, 224) : 0;
	int dcol = eof_color_white;
	int tcol = eof_color_black;	//This color is used as the fret text color (for pro guitar notes)
	int dcol2 = dcol;
	int ncol = eof_color_silver;	//Note color defaults to silver unless the note is not star power
	int lcol = makecol(128, 128, 128);	//The color used to draw the vertical line over the note's position
	unsigned long ctr;
	unsigned long mask;	//Used to mask out colors in the for loop
	int radius,dotsize;
	char iscymbal;		//Used to track whether the specified note is marked as a cymbal
	long x,y;
	unsigned long numlanes, tracknum=0, numlanes2;
	char notation[65] = {0};	//Used to store tab style notation for various note statuses
	char *nameptr = NULL;		//This points to the display name string for the note
	char samename[] = "/";		//This is what a repeated note name will display as
	char samenameauto[] = "[/]";	//This is what a repeated note for an non manually-named note will display as
	char notename[EOF_NAME_LENGTH+1] = {0}, prevnotename[EOF_NAME_LENGTH+1] = {0}, namefound;	//Used for name display
	static int lastx2 = 0;		//Tracks the right edge of the last non-summarized tab notation rendering

	//These variables are used to store the common note data, regardless of whether the note is legacy or pro guitar format
	unsigned long notepos = 0;
	long notelength = 0;
	unsigned long noteflags = 0;
	unsigned long notenote = 0;
	unsigned char notetype = 0;
	unsigned char ghost = 0;
	long length;
	int half_string_space = eof_screen_layout.string_space / 2;

	EOF_PRO_GUITAR_TRACK *tp = NULL;

//Validate parameters
	if(window == NULL)
	{
		return 1;	//Error, signal to stop rendering
	}
	if(track != 0)
	{	//Render an existing note
		if(track >= eof_song->tracks)
			return 1;	//Error, signal to stop rendering
		if(notenum >= eof_get_track_size(eof_song, track))
			return 1;	//Invalid note number, signal to stop rendering

		tracknum = eof_song->track[track]->tracknum;
		if((eof_song->track[track]->track_format != EOF_LEGACY_TRACK_FORMAT) && (eof_song->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
			return 1;	//Invalid track format, signal to stop rendering
		notepos = eof_get_note_pos(eof_song, track, notenum);
		notelength = eof_get_note_length(eof_song, track, notenum);
		noteflags = eof_get_note_flags(eof_song, track, notenum);
		notenote = eof_get_note_note(eof_song, track, notenum);
		notetype = eof_get_note_type(eof_song, track, notenum);
		ghost = eof_get_note_ghost(eof_song, track, notenum);

		if((eof_song->track[track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT) && ((eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT) || eof_legacy_view))
		{	//If the catalog entry is a pro guitar note and the active track is not, or the user specified to display pro guitar notes as legacy notes
			if(eof_song->pro_guitar_track[tracknum]->note[notenum]->legacymask != 0)
			{	//If the user defined how this pro guitar note would transcribe to a legacy track
				notenote = eof_song->pro_guitar_track[tracknum]->note[notenum]->legacymask;
			}
			else
			{
				notenote &= 31;	//Mask out lane 6 (this is how it would be treated when notes were created via a paste operation)
				if(!notenote)
				{	//Unless this clears the note entirely
					notenote = 31;	//In which case it becomes a chord that uses all lanes
				}
			}
		}
	}
	else
	{	//Render the pen note
		notepos = eof_pen_note.pos;
		notelength = eof_pen_note.length;
		noteflags = eof_pen_note.flags;
		notenote = eof_pen_note.note;
		notetype = eof_note_type;	//The pen note should reflect the active difficulty, ie. so the pen note can show an expert+ bass drum note's red center dot
		if(eof_hover_note >= 0)
		{
			noteflags = eof_get_note_flags(eof_song, eof_selected_track, eof_hover_note);	//The pen note should reflect the hover note's flags, ie. so the pen note can show an expert+ bass drum note's red center dot
		}
	}

	if(window == eof_window_info)
	{	//If rendering to the fret catalog
		position = eof_music_catalog_pos;
		leftcoord = 140;
	}
	else
	{	//Otherwise assume it's being rendered to the editor window
		position = eof_music_pos.value;
		leftcoord = 300;
	}
	pos = position / eof_zoom;
	if(pos < leftcoord)
	{	//Scroll the left edge of the piano roll based on the roll's position
		npos = 20 + notepos / eof_zoom;
	}
	else
	{
		npos = 20 - ((pos - leftcoord)) + notepos / (unsigned)eof_zoom;
	}
	x = npos;	//Store this to make the code more readable

//Determine if the entire note would clip.  If so, return without attempting to render
	if(npos - eof_screen_layout.note_size > window->screen->w)	//If the note would render entirely to the right of the visible area
		return 1;	//Return status:  Clipping to the right of the viewing window

	length = notelength / eof_zoom;
	if(npos + length < 0)	//If the note and its tail would render entirely to the left of the visible area
		return -1;	//Return status:  Clipping to the left of the viewing window

	if(noteflags & EOF_NOTE_FLAG_CRAZY)
	{
		dcol = eof_color_black;	//"Crazy" notes render with a black dot in the center
		tcol = eof_color_white;	//In which case, render the fret number (pro guitar) with the opposite color
		if(eof_fingering_view)
			tcol = eof_color_red;	//Unless fingering view is in effect, in which case render text in red to contrast better over the blue background color
	}

	if(track != 0)
	{	//If rendering an existing note
		numlanes = eof_count_track_lanes(eof_song, eof_selected_track);	//Count the number of lanes in the active track
		numlanes2 = eof_count_track_lanes(eof_song, track);	//Count the number of lanes in that note's track
		if(numlanes > numlanes2)
		{	//Special case (ie. viewing an open bass guitar catalog entry when any other legacy track is active)
			numlanes = numlanes2;	//Use the number of lanes in the active track
		}
	}
	else
	{	//If rendering the pen note
		numlanes = eof_count_track_lanes(eof_song, eof_selected_track);	//Count the number of lanes in the active track
	}
	if(noteflags & EOF_NOTE_FLAG_F_HOPO)				//If this note is forced as HOPO on
	{
		radius=eof_screen_layout.hopo_note_size;		//Draw the note in the defined HOPO on size
		dotsize=eof_screen_layout.hopo_note_dot_size;
	}
	else if(noteflags & EOF_NOTE_FLAG_NO_HOPO)			//Or if this note is forced as HOPO off
	{
		radius=eof_screen_layout.anti_hopo_note_size;	//Draw the note in the defined HOPO off size
		dotsize=eof_screen_layout.anti_hopo_note_dot_size;
	}
	else
	{
		radius=eof_screen_layout.note_size;				//Otherwise, draw the note at its standard size
		dotsize=eof_screen_layout.note_dot_size;
	}

	if(track && eof_render_grid_lines && (eof_snap_mode != EOF_SNAP_OFF) && !eof_is_grid_snap_position(notepos))
	{	//If an existing note is being rendered, grid lines are being displayed, a grid snap setting is set and the note in question is not on a grid snap position
		lcol = eof_color_red;	//Render the note's vertical line in red so it will contrast with the grid lines' yellow color
	}
	vline(window->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[2] - eof_screen_layout.note_marker_size, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[2] + eof_screen_layout.note_marker_size, lcol);
	if(p == 3)
	{
		dcol = eof_color_white;
	}

	//Render tab notation
	if(track != 0)
	{	//If rendering an existing note instead of the pen note
		if(eof_song->track[track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
		{	//If a pro guitar track is being rendered
			tp = eof_song->pro_guitar_track[eof_song->track[track]->tracknum];
			if(tp->note == tp->technote)
			{	//If tech view is in effect, render the tab notation for the note and nothing else
				BITMAP *fretbmp;
				int textcol;
				int bgcol;

				eof_get_note_notation(notation, track, notenum, 0);	//Get the tab playing notation for this note, disabling sanity checks for slides
				if(notation[0] == '\0')
				{	//If the note has no notations
					notation[0] = ' ';	//Insert a space to make the box that is drawn a little wider
					notation[1] = '\0';
				}
				for(ctr = 0, mask = 1; ctr < numlanes; ctr++, mask = mask << 1)
				{	//Render for each of the available fret lanes
					if(!(notenote & mask))
						continue;	//If this lane is not populated, skip it

					textcol = eof_color_red;	//Unless the technote overlaps at least one normal note or is highlighted, it will render in red
					bgcol = eof_color_black;	//Unless the technote begins at the same timestamp as a normal note, the background will be black
					if(p)
					{	//If the tech note is highlighted
						textcol = eof_color_white;
					}
					else
					{	//Otherwise determine what color to render it in
						char retval = eof_pro_guitar_tech_note_overlaps_a_note(tp, notenum, mask, NULL);	//Determine if the tech note overlaps any regular notes

						if(retval == 1)
						{	//If the technote overlaps with and starts at the same timestamp as a regular note on this lane
							textcol = eof_color_blue;
							bgcol = eof_color_white;	//Blue on white background should be more readable
						}
						else if(retval == 2)
						{	//If the technote overlaps with a regular note on this lane
							textcol = eof_color_green;
						}
					}

					fretbmp = eof_create_fret_number_bitmap(NULL, notation, 0, 2, textcol, bgcol, eof_symbol_font);	//Build a bordered bitmap for the technique, allow 2 pixels for padding
					if(fretbmp != NULL)
					{	//Render the bitmap in place of the note and then destroy the bitmap
						y = EOF_EDITOR_RENDER_OFFSET + 15 + ychart[ctr];	//Store this to make the code more readable
						draw_sprite(window->screen, fretbmp, x - (fretbmp->w/2), y - (text_height(font)/2));	//Fudge (x,y) to make it print centered over the gem
					}
					destroy_bitmap(fretbmp);
				}//Render for each of the available fret lanes
				return 0;	//Return status:  Note was not clipped in its entirety
			}//If tech view is in effect, render the tab notation for the note and nothing else
		}//If a pro guitar track is being rendered

		//Render tab notations before the note, so that the former doesn't render a solid background over the latter
		eof_get_note_notation(notation, track, notenum, 1);	//Get the tab playing notation for this note
		if(notation[0] != '\0')
		{	//If the note has notations
			if(!strcmp(notation, eof_last_tab_notation))
			{	//If the notation is identical to the previously rendered note's tab notation
				notation[0] = '.';	//Replace the notation string with a period
				notation[1] = '\0';
			}
			else
			{	//Otherwise keep a copy of this string to be compared with the next note
				strncpy(eof_last_tab_notation, notation, sizeof(eof_last_tab_notation) - 1);
			}
		}
		else
		{
			eof_last_tab_notation[0] = '\0';	//Otherwise erase this string, only consecutive notes can have their notation summarized
		}
		if(notation[0] == '.')
		{	//If the notation is being summarized, try to ensure it doesn't render on top of the full notation
			if(x < lastx2)
			{	//If this summarized notation would render on top of the fully notation
				notation[0] = '\0';	//Empty the string to avoid this and to ensure the full notation remains visible
			}
		}
		if(notation[0] != '\0')
		{	//If the notation hasn't been suppressed
			int half_notation_length = text_length(eof_symbol_font, notation) / 2;

			if(x - half_notation_length < 0)
			{	//If the notation would render off-screen, render the notation in left alignment
				textout_ex(window->screen, eof_symbol_font, notation, x, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 6, eof_color_red, eof_color_black);
				lastx2 = x + half_notation_length + half_notation_length + 2;	//Store the x coordinate position of the end of this notation rendering, add an extra couple pixels of padding
			}
			else
			{	//Otherwise, center the notation below the note
				textout_centre_ex(window->screen, eof_symbol_font, notation, x, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 6, eof_color_red, eof_color_black);
				lastx2 = x + half_notation_length + 2;	//Store the x coordinate position of the end of this notation rendering, add an extra couple pixels of padding
			}
		}
	}//If rendering an existing note instead of the pen note, draw tab notation

	//Render note, tail, slide indicator, etc.
	for(ctr=0,mask=1;ctr<numlanes;ctr++,mask=mask<<1)
	{	//Render for each of the available fret lanes
		iscymbal = 0;
		y = EOF_EDITOR_RENDER_OFFSET + 15 + ychart[ctr];	//Store this to make the code more readable

		if(notenote & mask)
		{	//If this lane is populated
			if((mask == 32) && !eof_lane_six_enabled(track))
			{	//If this is a gem on lane 6 and this track doesn't have 6 active lanes
				if(track)
				{	//If this is the pen note, it could only have been set to lane 6 if that lane was enabled
					continue;	//Skip rendering this note
				}
			}
			if(!(noteflags & EOF_NOTE_FLAG_SP))
			{	//If the note is not star power
				if(tp && (eof_color_set == EOF_COLORS_BF))
				{	//If a pro guitar track is active (tp will have been set above) and the Bandfuse color set is in use, override the color based on the gem's fingering
					if(tp->note[notenum]->flags & EOF_PRO_GUITAR_NOTE_FLAG_TAP)
					{	//If the note is tapped
						ncol = makecol(51, 51, 51);	//Draw the note in black
					}
					else
					{
						unsigned fingering = eof_pro_guitar_note_lookup_string_fingering(tp, notenum, ctr, 6);	//Look up this gem's fingering (or return 6 if cannot be determined)
						if(fingering < 6)
						{	//If the finger was determined
							ncol = eof_colors[fingering].color;	//Use the appropriate color
						}
						else
						{	//Otherwise use the default silvering coloring
							noteflags |= EOF_NOTE_FLAG_SP;	//And trigger the selection of the appropriate corresponding border color
						}
					}
				}
				else if(!track && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT) && (eof_color_set == EOF_COLORS_BF))
				{	//If the pen note is being drawn, the active track is a pro guitar track and the Bandfuse color set is in use
					ncol = eof_color_silver;		//Force the pen note to draw in silver (undefined fingering)
					noteflags |= EOF_NOTE_FLAG_SP;	//And trigger the selection of the appropriate corresponding border color
				}
				else if(track && (mask & eof_get_note_accent(eof_song, track, notenum)))
				{	//If the note is accented
					ncol = makecol(51, 51, 51);	//Draw the note in black
				}
				else
				{
					ncol = eof_colors[ctr].color;	//Assign the appropriate fret color
				}
			}
			if(p)
			{	//For mouse over/highlight lyrics, render note with its defined contrasting border color
				if(!(noteflags & EOF_NOTE_FLAG_SP))
				{	//If the note is not star power
					pcol = eof_colors[ctr].border;	//Use the note's normal border color
				}
				else
				{
					pcol = eof_color_red;	//Draw the border in red, which contrasts with silver much better
				}
			}

			if(eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
			{	//Drum track specific dot color logic
				if((notetype == EOF_NOTE_AMAZING) && (noteflags & EOF_DRUM_NOTE_FLAG_DBASS) && (mask == 1))
				{	//If this is an Expert+ bass drum note
					if(!eof_song->tags->double_bass_drum_disabled)	//And the user hasn't disabled expert+ bass drum notes
						dcol2 = eof_color_red;	//render it with a red dot
					else
						dcol2 = eof_color_blue;	//render it with a blue dot
				}
				else if(eof_note_is_cymbal_bitmask(mask, noteflags))
				{	//If this drum note is marked as a yellow, blue or green cymbal
					iscymbal = 1;
				}
				else
					dcol2 = dcol;			//Otherwise render with the expected dot color
			}
			else
				dcol2 = dcol;			//Otherwise render with the expected dot color

			//Render ghost note status background
			if(ghost & mask)
			{	//If this lane is ghosted
				long ghostlength = length;	//By default, draw this to match the length of the note
				if(ghostlength < 5)
				{	//But if the note is shorter than this
					ghostlength = 5;	//Pad it to this minimum so it's more visible
				}
				rectfill(window->screen, x, y - half_string_space, x + ghostlength, y + half_string_space, eof_color_white);	//Fill the background for this lane in white for the duration of the gem
			}

			//Render note tail
			if((notetype == EOF_NOTE_SPECIAL) || !((eof_song->track[eof_selected_track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR) && eof_hide_drum_tails))
			{	//If this is a BRE note or it is otherwise not a drum note that will have its tail hidden due to the "Hide drum note tails" user option,
				rectfill(window->screen, x, y - eof_screen_layout.note_tail_size, x + length, y + eof_screen_layout.note_tail_size, ncol);	//Draw the note tail
				if(p)
				{	//If this note is moused over
					rect(window->screen, x, y - eof_screen_layout.note_tail_size, x + length, y + eof_screen_layout.note_tail_size, pcol);	//Draw a border around the rectangle
				}
			}

			//Render pro guitar note slide if applicable
			if((track != 0) && (eof_song->track[track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT) && ((noteflags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP) || (noteflags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN) || (noteflags & EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE)))
			{	//If rendering an existing pro guitar note that slides up or down or is an unpitched slide
				long x2;			//Used for slide note rendering
				unsigned long notepos2;		//Used for slide note rendering
				int sliderect[8] = {0};		//An array of 4 vertices, used to draw a diagonal rectangle
				int slidecolor = eof_color_dark_purple;	//By default, pro guitar slides are drawn in purple
				char up = 0;		//Will be assumed to be a down slide by default

				if(noteflags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_REVERSE)
					slidecolor = eof_color_white;	//If it's a reverse slide though, draw in white
				if(noteflags & EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE)
					slidecolor = eof_color_black;	//If it's an unpitched slide, draw in black

				if(noteflags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP)
					up = 1;
				else if(noteflags & EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE)
				{
					unsigned char lowestfret = eof_get_lowest_fret_value(eof_song, track, notenum);	//Determine the fret value of the lowest fretted string
					if(lowestfret < eof_song->pro_guitar_track[tracknum]->note[notenum]->unpitchend)
					{	//If the unpitched slide goes higher than this position
						up = 1;
					}
				}

				notepos2 = notepos + notelength;	//Find the position of the end of the note
				if(pos < leftcoord)
				{
					x2 = 20 + notepos2 / eof_zoom;
				}
				else
				{
					x2 = 20 - ((pos - leftcoord)) + notepos2 / eof_zoom;
				}

				//Define the slide rectangle coordinates in clockwise order
				#define EOF_PRO_GUITAR_SLIDE_LINE_THICKNESS 1
				sliderect[0] = x;	//X1 (X coordinate of the left end of the slide)
				if(up)
				{	//If this note slides up, start the slide line at the bottom of this note
					sliderect[1] = y + radius - EOF_PRO_GUITAR_SLIDE_LINE_THICKNESS;	//Y1 (Y coordinate of the left end of the slide)
				}
				else
				{	//Otherwise start the slide line at the top of this note
					sliderect[1] = y - radius - EOF_PRO_GUITAR_SLIDE_LINE_THICKNESS;	//Y1 (Y coordinate of the left end of the slide)
				}
				sliderect[2] = x2;	//X2 (X coordinate of the right end of the slide)
				if(up)
				{	//If this note slides up, end the slide line at the top of the next note
					sliderect[3] = y - radius - EOF_PRO_GUITAR_SLIDE_LINE_THICKNESS;	//Y2 (Y coordinate of the right end of the slide)
				}
				else
				{	//Otherwise end the slide line at the bottom of the next note
					sliderect[3] = y + radius - EOF_PRO_GUITAR_SLIDE_LINE_THICKNESS;	//Y2 (Y coordinate of the right end of the slide)
				}
				sliderect[4] = x2;	//X3 (X coordinate of the right end of the slide)
				sliderect[5] = sliderect[3] + (2 * EOF_PRO_GUITAR_SLIDE_LINE_THICKNESS);	//Y3 (the specified number of pixels below Y2)
				sliderect[6] = x;	//X4 (X coordinate of the left end of the slide)
				sliderect[7] = sliderect[1] + (2 * EOF_PRO_GUITAR_SLIDE_LINE_THICKNESS);	//Y4 (the specified number of pixels below Y1)

				if((sliderect[0] < window->w) && (sliderect[2] >= 0))
				{	//If the left end of the polygon doesn't render off the right edge of the editor window and the right end of the polygon doesn't render off the left edge
					polygon(window->screen, 4, sliderect, slidecolor);		//Render the 4 point polygon in the appropriate color
				}
			}//If rendering an existing pro guitar note that slides up or down or is an unpitched slide

			//Render note head
			if(!iscymbal)
			{	//If this note is not a cymbal, render note as a circle
				circlefill(window->screen, x, y, radius, ncol);
				circlefill(window->screen, x, y, dotsize, dcol2);
				if(p)
				{
					circle(window->screen, x, y, radius, pcol);
				}

				if(!eof_legacy_view && (track > 0) && (notenote & mask) && (eof_song->track[track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT) && (eof_song->track[eof_selected_track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
				{	//If legacy view is disabled, this is a pro guitar note and a pro guitar track is active, perform pro guitar specific rendering
					BITMAP *fretbmp;

					if(!eof_fingering_view)
					{	//Render the fret number over the center of the note (but only if the active track is a pro guitar track)
						fretbmp = eof_create_fret_number_bitmap(eof_song->pro_guitar_track[tracknum]->note[notenum], NULL, ctr, 2, tcol, dcol, font);	//Allow 2 pixels for padding
					}
					else
					{	//Render the finger number instead
						unsigned fingernum = eof_song->pro_guitar_track[tracknum]->note[notenum]->finger[ctr];	//By default, the string's fingering will display as-is

						if(eof_pro_guitar_note_fingering_valid(eof_song->pro_guitar_track[tracknum], notenum, 0) != 1)
						{	//If the fingering is not deemed valid for this note
							dcol = eof_color_light_red;
							if(noteflags & EOF_NOTE_FLAG_CRAZY)
								tcol = eof_color_blue;	//Change text color to blue to contrast better
						}
						else
						{
							dcol = eof_color_even_lighter_blue;
						}
						if(!fingernum)
							snprintf(notation, 2, " ");		//Fingering not defined for this string
						else
						{
							if(fingernum == 5)
								fingernum = 0;	//Convert from EOF's numbering (5 = thumb) to Rocksmith's numbering (0 = thumb)
							snprintf(notation, 2, "%u", fingernum);
						}
						fretbmp = eof_create_fret_number_bitmap(NULL, notation, ctr, 2, tcol, dcol, font);
					}
					if(fretbmp != NULL)
					{	//Render the bitmap on top of the 2D note and then destroy the bitmap
						draw_sprite(window->screen, fretbmp, x - (fretbmp->w/2), y - (text_height(font)/2));	//Fudge (x,y) to make it print centered over the gem
						destroy_bitmap(fretbmp);
					}
				}
			}
			else
			{	//Otherwise render it as a triangle
				#ifdef ALLEGRO_MACOSX
				{	//The triangle() function is broken in at least OS X High Sierra, call polygon() instead
					int points[6];

					points[0] = x;
					points[1] = y-radius;
					points[2] = x+radius;
					points[3] = y+radius;
					points[4] = x-radius;
					points[5] = y+radius;
					polygon(window->screen, 3, points, ncol);
				}
				#else
					triangle(window->screen, x, y-radius, x+radius, y+radius, x-radius, y+radius, ncol);
				#endif
				if(p)
				{	//Draw a non filled rectangle along the border of the filled triangle
					line(window->screen, x, y-radius, x+radius, y+radius, pcol);
					line(window->screen, x+radius, y+radius, x-radius, y+radius, pcol);
					line(window->screen, x-radius, y+radius, x, y-radius, pcol);
				}
				if(track == EOF_TRACK_DRUM_PS)
				{	//If rendering a note in the PS drum track
					if(	((mask == 4) && (noteflags & EOF_DRUM_NOTE_FLAG_Y_COMBO)) ||
						((mask == 8) && (noteflags & EOF_DRUM_NOTE_FLAG_B_COMBO)) ||
						((mask == 16) && (noteflags & EOF_DRUM_NOTE_FLAG_G_COMBO)))
					{	//If the note is a tom/cymbal combo
						circlefill(window->screen, x, y + dotsize / 2, dotsize * 1.5, pcol);	//Draw a large dot in the center of the triangle
					}
				}
				if(noteflags & EOF_NOTE_FLAG_CRAZY)
				{	//If the cymbal has crazy status
					circlefill(window->screen, x, y + dotsize / 2, dotsize - 1, eof_color_black);	//Draw a dot in the center of the triangle
				}
			}
		}//If this lane is populated
		else if((eof_hover_note >= 0) && (p == 3))
		{	//If this note is currently being moused over
			rect(window->screen, x, y - eof_screen_layout.note_tail_size, x + length, y + eof_screen_layout.note_tail_size, eof_color_gray);

			//Determine if the hovered note is marked as a yellow, blue or green cymbal
			iscymbal = eof_note_is_cymbal(eof_song, eof_selected_track, eof_hover_note);
			if(!iscymbal)
			{	//If this note is not a cymbal, draw a non filled circle over the note
				circle(window->screen, x, y, radius, eof_color_gray);
			}
			else
			{	//Draw a non filled rectangle along the border of the filled triangle
				line(window->screen, x, y-radius, x+radius, y+radius, eof_color_gray);
				line(window->screen, x+radius, y+radius, x-radius, y+radius, eof_color_gray);
				line(window->screen, x-radius, y+radius, x, y-radius, eof_color_gray);
			}
		}
	}//Render for each of the available fret lanes

	if((track == 0) || eof_hide_note_names)
		return 0;	//If the pen note is being drawn, or the user opted to hide note names, skip the logic below
	if((window != eof_window_info) && (eof_2d_render_top_option != 5))
		return 0;	//If not rendering to the fret catalog or to the 2D window with the option to display note names, skip the logic below

	notename[0] = prevnotename[0] = '\0';	//Empty these strings
	namefound = eof_build_note_name(eof_song, track, notenum, notename);
	if(!namefound)
		return 0;	//If the note does not have a name, skip the logic below

	//Otherwise prepare it for rendering
	(void) eof_build_note_name(eof_song, track, eof_get_prev_note_type_num(eof_song, track, notenum), prevnotename);	//Get the previous note's name
	if(!ustrcmp(notename, prevnotename))
	{	//If this note and the previous one have the same name (case sensitive)
		if(namefound == 1)
		{	//If the name for this note was manually assigned
			nameptr = samename;	//Display this note's name as "/" to indicate a repeat of the last note
		}
		else
		{	//The name for this note was detected
			nameptr = samenameauto;	//Display this note's name as "[/]" to indicate a repeat of the last note
		}
	}
	else
	{	//This note doesn't have the same name as the previous note
		if(namefound == 1)
		{	//If the name for this note was manually assigned
			nameptr = notename;	//Display the note name as-is
		}
		else
		{	//The name for this note was detected
			(void) snprintf(prevnotename, sizeof(notename) - 1, "[%s]", notename);	//Rebuild the note name to be enclosed in brackets
			nameptr = prevnotename;
		}
	}
	if(eof_track_is_drums_rock_mode(eof_song, track))
	{	//Special case:  Drums Rock mode is enabled
		nameptr = notename;	//Display the note name as-is since it instead defines the drum roll count
	}
	if(window == eof_window_info)
	{	//If rendering to the info window
		textout_centre_ex(window->screen, font, nameptr, x, EOF_EDITOR_RENDER_OFFSET + 10, eof_color_white, -1);
	}
	else
	{	//If rendering to either editor window
		textout_centre_ex(window->screen, font, nameptr, x, 25 + 5, eof_color_white, -1);
	}

	return 0;	//Return status:  Note was not clipped in its entirety
}

int eof_lyric_draw(EOF_LYRIC * np, int p, EOF_WINDOW *window)
{
	EOF_LYRIC *nextnp=NULL;	//Used to find the would-be X coordinate of the next lyric in the lyric lane
	unsigned long notenum = 0;
	char notpen = 0;	//Is set to nonzero if the passed lyric is determined to already be defined
	long position;	//This is the position for the specified window's piano roll and is based on the passed window pointer
	long leftcoord;	//This is the position of the left end of the piano roll
	long pos;		//This is the position of the specified window's piano roll, scaled by the current zoom level
	long npos;		//Stores the X coordinate at which to draw lyric #notenum
	long X2;		//Stores the X coordinate at which lyric #notenum+1 would be drawn (to set the clip rectangle)
	long stringend;	//Stores the coordinate position of the end of the lyric string, used for clipping logic
	long nplace;
	long note_y;
	long native = 0;
	int pcol = p == 1 ? eof_color_blue : p == 2 ? makecol(224, 255, 224) : 0;
	int ncol = 0;
	EOF_PHRASE_SECTION *lyricline;	//The line that this lyric is found to be in (if any) so the correct background color can be determined
	int bgcol = eof_color_black;	//Unless the text is found to be in a lyric phrase, it will render with a black background
	long endpos;		//This will store the effective end position for the lyric's rendering (taking lyric text, note rectangles and vocal slides into account)

//Validate parameters
	if((np == NULL) || (window == NULL))	//If this is not a valid lyric or window pointer
		return 1;			//Stop rendering

	if(window == eof_window_info)
	{	//If rendering to the fret catalog
		position = eof_music_catalog_pos;
		leftcoord = 140;
	}
	else
	{	//Otherwise assume it's being rendered to the editor window
		position = eof_music_pos.value;
		leftcoord = 300;
	}
	pos = position / eof_zoom;
	if(pos < leftcoord)
	{
		npos = 20 + (np->pos) / eof_zoom;
	}
	else
	{
		npos = 20 - ((pos - leftcoord)) + np->pos / eof_zoom;
	}

//Determine if the entire note would clip right of the viewable area.  If so, return without attempting to render
	if(npos > window->screen->w)	//If the lyric would render beginning beyond EOF's right window border
		return 1;

	X2=window->screen->w;		//The default X2 coordinate for the clipping rectangle
	notenum = eof_find_lyric_number(np);	//Find which lyric number this is
	if(notenum || (eof_song->vocal_track[0]->lyric[0] == np))
	{	//If the passed lyric is already defined (not the pen lyric)
		notpen = 1;
		lyricline=eof_find_lyric_line(notenum);	//Find which line this lyric is in
		if(lyricline != NULL)
		{
			if((lyricline->flags) & EOF_LYRIC_LINE_FLAG_OVERDRIVE)	//If the overdrive flag is set
				bgcol=makecol(64, 128, 64);	//Make dark green the text's background color
			else
				bgcol=makecol(0, 0, 127);	//Make dark blue the text's background color
		}

		if(notenum < eof_song->vocal_track[0]->lyrics-1)		//If there is another lyric
		{							//Find its would-be X coordinate (use as X2 for clipping rect)
			nextnp=eof_song->vocal_track[0]->lyric[notenum+1];
			if(pos < leftcoord)
				X2 = 20 + (nextnp->pos) / eof_zoom;
			else
				X2 = 20 - ((pos - leftcoord)) + nextnp->pos / eof_zoom;
		}
	}

//Determine if the entire note would clip left of the viewable area.  If so, return without attempting to render
	endpos = np->pos + np->length;	//The normal end of the lyric is based on its rectangle
	if(notpen && (notenum + 1 < eof_song->vocal_track[0]->lyrics) && (eof_song->vocal_track[0]->lyric[notenum + 1]->text[0] == '+'))
	{	//If there is another lyric and it is a pitch shift
		endpos = eof_song->vocal_track[0]->lyric[notenum + 1]->pos;
	}
	//Map the timestamp to a screen coordinate
	if(pos < leftcoord)
	{
		endpos = 20 + endpos / eof_zoom;
	}
	else
	{
		endpos = 20 - ((pos - leftcoord)) + endpos / eof_zoom;
	}
	//Map the end of the lyric text to a screen coordinate, compare with the other and keep the higher of the two
	stringend = npos + text_length(font, np->text);	//Get the would be end position of the rendered string
	if(stringend > endpos)
	{	//If the lyric text is longer on screen than the lyric rectangle or vocal slide
		endpos = stringend;	//Use it for the clipping logic
	}
	if(endpos < 0)
		return -1;	//Return status:  Clipping to the left of the viewing window

	if(p == 3)
	{
		pcol = eof_color_white;
	}

	nplace = np->note - eof_vocals_offset;
	if(nplace < 0)
	{
		native = -1;
	}
	else if(nplace >= eof_screen_layout.vocal_view_size)
	{
		native = 1;
	}
	while(nplace < 0)
	{
		nplace += eof_screen_layout.vocal_view_size;
	}
	if(native < 0)
	{
		note_y = EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.vocal_y;
	}
	else if(native > 0)
	{
		note_y = EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.vocal_y - (eof_screen_layout.vocal_view_size + 1) * eof_screen_layout.vocal_tail_size;
	}
	else
	{
		note_y = EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.vocal_y - (nplace % eof_screen_layout.vocal_view_size + 1) * eof_screen_layout.vocal_tail_size;
	}

//Rewritten logic to remove duplicated code and render pitchless lyrics at the bottom of the piano roll in gray
	vline(window->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.vocal_y - ((eof_screen_layout.vocal_view_size + 2) * eof_screen_layout.vocal_tail_size) / 2 - eof_screen_layout.note_marker_size, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.vocal_y - ((eof_screen_layout.vocal_view_size + 2) * eof_screen_layout.vocal_tail_size) / 2 + eof_screen_layout.note_marker_size, makecol(128, 128, 128));
	if((np->note != 0) && !eof_is_freestyle(np->text))
	{	//If this lyric is not pitchless/freestyle
		ncol = native ? eof_color_red : eof_color_green;
		if(np->note != EOF_LYRIC_PERCUSSION)
		{
			int sliderect[8] = {0};	//An array of 4 vertices, used to draw a diagonal rectangle
			EOF_LYRIC *np2=NULL;	//Stores a pointer to the next lyric
			long nplace2 = 0;
			long native2 = 0;
			long note_y2 = 0;	//Used to store the y coordinate of the next lyric
			long npos2 = 0;		//Stores the X coordinate of the next lyric

			rectfill(window->screen, npos, note_y, npos + np->length / eof_zoom, note_y + eof_screen_layout.vocal_tail_size - 1, ncol);

			if(notpen && (notenum + 1 < eof_song->vocal_track[0]->lyrics) && (eof_song->vocal_track[0]->lyric[notenum + 1]->text[0] == '+'))
			{	//If there's another lyric, and it begins with a plus sign, it's a pitch shift, draw a vocal slide polygon
				long leftcoord2;

				np2=eof_song->vocal_track[0]->lyric[notenum+1];
				sliderect[0]=npos + np->length / eof_zoom;	//X1 (X coordinate of the end of this lyric's rectangle)
				sliderect[1]=note_y;						//Y1 (Y coordinate of the bottom of this lyric's rectangle)
				if(window == eof_window_info)
				{	//If rendering to the fret catalog
					leftcoord2 = 140;
				}
				else
				{	//Otherwise assume it's being rendered to the editor window
					leftcoord2 = 300;
				}

				if(pos < 300)
				{
					npos2 = 20 + (np2->pos) / eof_zoom;
				}
				else
				{
					npos2 = 20 - ((pos - leftcoord2)) + np2->pos / eof_zoom;
				}
				sliderect[2]=npos2;	//X2 (X coordinate of next lyric's rectangle)

				nplace2 = np2->note - eof_vocals_offset;
				if(nplace2 < 0)
				{
					native2 = -1;
				}
				else if(nplace2 >= eof_screen_layout.vocal_view_size)
				{
					native2 = 1;
				}
				while(nplace2 < 0)
				{
					nplace2 += eof_screen_layout.vocal_view_size;
				}
				if(native2 < 0)
				{
					note_y2 = EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.vocal_y;
				}
				else if(native2 > 0)
				{
					note_y2 = EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.vocal_y - (eof_screen_layout.vocal_view_size + 1) * eof_screen_layout.vocal_tail_size;
				}
				else
				{
					note_y2 = EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.vocal_y - (nplace2 % eof_screen_layout.vocal_view_size + 1) * eof_screen_layout.vocal_tail_size;
				}
				sliderect[3]=note_y2;	//Y2 (Y coordinate of next lyric's rectangle)

				sliderect[6]=sliderect[0];	//X3 (X coordinate of the end of this lyric's rectangle)
				sliderect[7]=sliderect[1] + eof_screen_layout.vocal_tail_size - 1;	//Y3 (Y coordinate of the bottom of this lyric's rectangle)
				sliderect[4]=sliderect[2];	//X4 (X coordinate of the next lyric's rectangle)
				sliderect[5]=sliderect[3] + eof_screen_layout.vocal_tail_size - 1;	//Y4 (Y coordinate of the bottom of next lyric's rectangle)

				if((sliderect[0] < window->w) && (sliderect[2] >= 0))
				{	//If the left end of the polygon doesn't render off the right edge of the editor window and the right end of the polygon doesn't render off the left edge
					polygon(window->screen, 4, sliderect, eof_color_dark_purple);	//Render the 4 point polygon in dark purple
				}
			}
		}
		else
		{	//Render a vocal percussion note as a fret note in the middle lane
			circlefill(window->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[2] + eof_screen_layout.string_space / 2, eof_screen_layout.note_size, eof_color_white);
			circlefill(window->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[2] + eof_screen_layout.string_space / 2, eof_screen_layout.note_size-2, eof_color_black);
		}
	}//If this lyric is not pitchless/freestyle
	else
	{	//If the lyric is pitchless or freestyle, render with gray
		rectfill(window->screen, npos, note_y, npos + np->length / eof_zoom, note_y + eof_screen_layout.vocal_tail_size - 1, makecol(128, 128, 128));
	}

	if(p)
	{	//If the lyric should be highlighted (ie. is selected or is being moused over)
		if(np->note == EOF_LYRIC_PERCUSSION)
		{	//Highlight a vocal percussion note
			circlefill(window->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[2] + eof_screen_layout.string_space / 2, eof_screen_layout.note_size, eof_color_white);
			circlefill(window->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[2] + eof_screen_layout.string_space / 2, eof_screen_layout.note_size-2, eof_color_gray);
		}
		else
		{	//Render a regular vocal note
			rect(window->screen, npos, note_y, npos + np->length / eof_zoom, note_y + eof_screen_layout.vocal_tail_size - 1, pcol);
		}
	}

//Rewritten logic checks the next lyric to set an appropriate clipping rectangle for truncation purposes, and use the determined background color (so phrase marking rectangle isn't erased by text)
	set_clip_rect(window->screen, 0, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.lyric_y + 1, X2-2, window->screen->h);	//Alteration: Provide at least two pixels of clearance for the edge of the clip rectangle

	if(p == 3)
		textprintf_ex(window->screen, font, npos, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.lyric_y, eof_color_white, bgcol, "%s", np->text);
	else
		textprintf_ex(window->screen, font, npos, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.lyric_y, p ? eof_color_green : eof_color_white, bgcol, "%s", np->text);

	set_clip_rect(window->screen, 0, 0, window->screen->w, window->screen->h);	//Restore original clipping rectangle

	return 0;	//Return status:  Note was not clipped in its entirety
}

int eof_note_draw_3d(unsigned long track, unsigned long notenum, int p)
{
	long npos;
	int bx = 48;
	int point[8];
	long rz, ez;
	unsigned long ctr;
	unsigned long mask;	//Used to mask out colors in the for loop
	unsigned long numlanes, tracknum;
	long xoffset = 0;	//This will be used to draw bitmaps half a lane further left when the bass drum isn't rendering in a lane and drum gems render centered over fret lines instead of between them
	int drawline = 0;	//Set to nonzero if a bass drum style line is to be drawn for the gem
	int drawlineonly = 1;	//Set to zero if the note could render as more than just a bass drum style line (ie. drum notes or open chords)
	int linecol = eof_color_red;		//The color of the line to be drawn
	int offset_y_3d = 0;	//If full height 3D preview is in effect, y coordinates will be shifted down by one panel height, otherwise will shift by 0

	//These variables are used for the name rendering logic
	long x3d, y3d, z3d;			//The coordinate at which to draw the name string (right aligned)
	char *nameptr = NULL;		//This points to the display name string for the note
	char samename[] = "/";		//This is what a repeated note name will display as
	char samenameauto[] = "[/]";	//This is what a repeated note for an non manually-named note will display as
	char notename[EOF_NAME_LENGTH+1] = {0}, prevnotename[EOF_NAME_LENGTH+1] = {0}, namefound;	//Used for name display

	//These variables are used to store the common note data, regardless of whether the note is legacy or pro guitar format
	unsigned long notepos = 0;
	long notelength = 0;
	unsigned long noteflags = 0;
	unsigned long notenote = 0;
	unsigned imagenum = 0;	//Used to store the appropriate image index to use for rendering the specified note

	int render_bass_drum_in_lane = 0;	//Tracks whether this is a drum track and bass drums are to be rendered as a gem in a lane instead of a line

	//Validate parameters
	if((track == 0) || (track >= eof_song->tracks) || ((eof_song->track[track]->track_format != EOF_LEGACY_TRACK_FORMAT) && (eof_song->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)) || (notenum >= eof_get_track_size(eof_song, track)))
	{	//If an invalid track or note number was passed
		return -1;	//Error, signal to stop rendering (3D window renders last note to first)
	}
	tracknum = eof_song->track[track]->tracknum;
	notepos = eof_get_note_pos(eof_song, track, notenum);
	notelength = eof_get_note_length(eof_song, track, notenum);
	noteflags = eof_get_note_flags(eof_song, track, notenum);
	notenote = eof_get_note_note(eof_song, track, notenum);

	if(eof_full_height_3d_preview)
	{
		offset_y_3d = eof_screen_height / 2;	//Y coordinates will be lowered by one panel height so that the bottom of the 3D fret board is at the bottom of the program window
	}
	if((eof_song->track[track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT) && ((eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT) || eof_legacy_view))
	{	//If the catalog entry is a pro guitar note and the active track is not, or the user specified to display pro guitar notes as legacy notes
		if(eof_song->pro_guitar_track[tracknum]->note[notenum]->legacymask != 0)
		{	//If the user defined how this pro guitar note would transcribe to a legacy track
			notenote = eof_song->pro_guitar_track[tracknum]->note[notenum]->legacymask;
		}
		else
		{
			notenote &= 31;	//Mask out lane 6 (this is how it would be treated when notes were created via a paste operation)
		}
	}

	npos = (long)(notepos + eof_av_delay - eof_music_pos.value) / eof_zoom_3d  - 6;
	if(npos + notelength / eof_zoom_3d < eof_3d_min_depth)
	{				//If the note would render entirely before the visible area
		return -1;	//Return status:  Clipping before the viewing window
	}
	else if(npos > eof_3d_max_depth)
	{				//If the note would render entirely after the visible area
		return 1;	//Return status:  Clipping after the viewing window
	}

	numlanes = eof_count_track_lanes(eof_song, track);	//Count the number of lanes in that note's track
	if(eof_track_is_legacy_guitar(eof_song, track))
	{	//Special case:  Legacy guitar tracks can use a sixth lane but their 3D representation still only draws 5 lanes
		numlanes = 5;
	}

	if(((eof_song->track[track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR) && eof_render_bass_drum_in_lane) || eof_track_is_drums_rock_mode(eof_song, track))
		render_bass_drum_in_lane = 1;

	if((eof_song->track[track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR) && !render_bass_drum_in_lane)
	{	//If this is a drum track and the bass drum isn't being rendered in its own lane
		xoffset = (56.0 * (4.0 / (numlanes-1))) / 2;	//This value is half of the 3D lane's width
	}

	if(eof_track_is_drums_rock_mode(eof_song, track))
	{	//Special case:  Drums Rock mode will alter the note mask to  contain no more than 2 gems
		notenote = eof_convert_drums_rock_note_mask(eof_song, track, notenum);
	}

	//Check whether a line is to be rendered for this note
	if((eof_song->track[track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR) && (notenote & 1) && !render_bass_drum_in_lane)
	{	//If this is a drum track, it includes a bass drum gem is being drawn and bass drum isn't being rendered in its own lane
		drawline = 1;
		drawlineonly = 0;	//Drum notes can have other gems render on top of the line
		notenote--;		//Remove the bass drum gem so it isn't rendered in the for loop below
		if(noteflags & EOF_NOTE_FLAG_SP)			//If this bass drum note is star power, render it in silver
			linecol = p ? eof_color_white : eof_color_silver;
		else if(noteflags & EOF_DRUM_NOTE_FLAG_DBASS)
		{	//Or if it is double bass
			if(!eof_song->tags->double_bass_drum_disabled)	//If the user has not disabled expert+ bass drum notes
				linecol = p ? makecol(255, 192, 192) : eof_color_red;	//Render it in red
			else
				linecol = p ? makecol(192, 192, 255) : eof_color_blue;	//Render it in blue
		}
		else										//Otherwise render it in the standard lane one color for the current color set
			linecol = p ? eof_colors[0].hit : eof_colors[0].color;
	}
	else if(eof_render_3d_rs_chords && (eof_note_count_rs_lanes(eof_song, track, notenum, 2) >= 2) && (eof_song->track[track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT) &&
			((noteflags & EOF_PRO_GUITAR_NOTE_FLAG_HD) || (eof_note_has_high_chord_density(eof_song, track, notenum, 2))))
	{	//If the user has opted to 3D render Rocksmith style chords, and this is a pro guitar chord that either has high density due to being explicitly defined as such or automatically due to other means
		long prevnote = eof_track_fixup_previous_note(eof_song, track, notenum);

		if((noteflags & EOF_PRO_GUITAR_NOTE_FLAG_HD) || (!(noteflags & EOF_PRO_GUITAR_NOTE_FLAG_SPLIT) && !(eof_get_note_eflags(eof_song, track, notenum) & EOF_PRO_GUITAR_NOTE_EFLAG_CHORDIFY)))
		{	//If this chord has "hi dens" status, or if it isn't marked with split or chordify statuses
			drawline = 1;
			if(!prevnote)
			{	//If there was a previous note
				EOF_PRO_GUITAR_TRACK * tp = eof_song->pro_guitar_track[eof_song->track[track]->tracknum];	//Simplify

				if(strcmp(eof_get_note_name(eof_song, track, prevnote), eof_get_note_name(eof_song, track, notenum)))
				{	//If the previous note had a different name defined
					drawline = 0;
				}
				if(eof_pro_guitar_note_compare_fingerings(tp->note[prevnote], tp->note[notenum]))
				{	//If the previous note had a different chord fingering defined
					drawline = 0;
				}
			}
			if(noteflags & EOF_PRO_GUITAR_NOTE_FLAG_HD)
			{	//The high density status overrides all other conditions
				drawline = 1;
			}
			if(drawline)
			{	//If conditions were met to render this chord as a repeat line
				linecol = p ? eof_color_cyan : eof_color_dark_cyan;
			}
		}
	}
	else if(!eof_track_is_ghl_mode(eof_song, track) && (notenote & 32) && eof_track_is_legacy_guitar(eof_song, track))
	{	//If this is NOT a GHL style track, is a legacy guitar track and contains a gem on lane 6 (renders similarly to a bass drum note)
		if(eof_open_strum_enabled(track))
		{	//If open strum is enabled for the track
			drawline = 1;
			drawlineonly = 0;	//Open chords can have other gems render on top of the line
			notenote -= 32;	//Remove the lane 6 gem so it doesn't render in the for loop below
			if(noteflags & EOF_NOTE_FLAG_SP)			//If this open bass note is star power, render it in silver
				linecol = p ? eof_color_white : eof_color_silver;
			else										//Otherwise render it in the standard lane six color for the current color set
				linecol = p ? eof_colors[5].hit : eof_colors[5].color;
		}
	}
	else if(eof_track_is_ghl_mode(eof_song, track) && (noteflags & EOF_GUITAR_NOTE_FLAG_GHL_OPEN))
	{	//If this is a GHL style track and the note is an open note
		drawline = 1;
		if(noteflags & EOF_NOTE_FLAG_SP)			//If this open note has star power, render it in silver
			linecol = p ? eof_color_white : eof_color_silver;
	}

	//Render the line if applicable
	if(drawline)
	{	//If rendering a line
		rz = npos;
		ez = npos + 14;
		point[0] = ocd3d_project_x(bx - 10, rz);
		point[1] = ocd3d_project_y(200 + offset_y_3d, rz);
		point[2] = ocd3d_project_x(bx - 10, ez);
		point[3] = ocd3d_project_y(200 + offset_y_3d, ez);
		point[4] = ocd3d_project_x(bx + 232, ez);
		point[5] = point[3];
		point[6] = ocd3d_project_x(bx + 232, rz);
		point[7] = point[1];

		if((point[0] != -65536) && (point[1] != -65536) && (point[2] != -65536) && (point[3] != -65536) && (point[4] != -65536) && (point[6] != -65536))
		{	//If none of the coordinate projections failed
			polygon(eof_window_3d->screen, 4, point, linecol);
		}
		if(noteflags & EOF_NOTE_FLAG_HOPO)
		{	//If the note is a HOPO, ie. open note, render a smaller black rectangle inside the first
			rz = npos + 5;
			ez = npos + 9;
			point[0] = ocd3d_project_x(bx - 10 + 15, rz);
			point[1] = ocd3d_project_y(200 + offset_y_3d, rz);
			point[2] = ocd3d_project_x(bx - 10 + 15, ez);
			point[3] = ocd3d_project_y(200 + offset_y_3d, ez);
			point[4] = ocd3d_project_x(bx + 232 - 15, ez);
			point[5] = point[3];
			point[6] = ocd3d_project_x(bx + 232 - 15, rz);
			point[7] = point[1];

			if((point[0] != -65536) && (point[1] != -65536) && (point[2] != -65536) && (point[3] != -65536) && (point[4] != -65536) && (point[6] != -65536))
			{	//If none of the coordinate projections failed
				polygon(eof_window_3d->screen, 4, point, eof_color_black);
			}
		}
	}

	//Render any gems using bitmaps if applicable
	for(ctr = 0, mask = 1; ctr < eof_count_track_lanes(eof_song, track); ctr++, mask <<= 1)
	{	//For each lane used in this note
		if(drawline && drawlineonly)	//If this note rendered as a line and could not render with any other gems
			break;				//Don't render any

		//Determine if this gem is to be drawn as a 3D rectangle instead of a bitmap
		if(!(notenote & mask))
			continue;	//If this lane is not used, skip it

		if((mask == 32) && !eof_track_is_ghl_mode(eof_song, track) && eof_track_is_legacy_guitar(eof_song, track) && !eof_open_strum_enabled(track))
			continue;	//If this is a lane 6 gem in a legacy track, GHL mode is not enabled and open strum is not enabled, skip rendering this gem

		#define EOF_HALF_3D_IMAGE_WIDTH 24
		#define EOF_3D_IMAGE_HEIGHT 48

		//The full size (non HOPO) GHL gems are larger than the normal 5 lane gems
		#define EOF_GHL_HALF_3D_IMAGE_WIDTH 36
		#define EOF_GHL_3D_IMAGE_HEIGHT 72

		unsigned lanenum = ctr;	//For non GHL modes, each fret generally get its own lane
		unsigned long half_image_width;
		unsigned long image_height;
		double height_scale = 1.0;

		if(eof_track_is_ghl_mode(eof_song, track))
		{	//If rendering a Guitar Hero Live style track
			lanenum = ctr % 3;	//Gems 1 through 3 use the same lanes as gems 4 through 6, open notes are rendered with the previous drawline logic

			if(noteflags & EOF_NOTE_FLAG_SP)
			{	//If this is a SP note
				if(ctr < 3)
				{	//The first three lanes are black notes
					unsigned long barremask = mask | (mask << 3);	//This represents the note mask of the gem and a gem 3 lanes higher

					if((notenote & barremask) == barremask)
					{	//If this is to be rendered as a barre note
						if(noteflags & EOF_GUITAR_NOTE_FLAG_IS_SLIDER)
						{	//Barre SP GHL slider
							imagenum = EOF_IMAGE_NOTE_GHL_BARRE_SP_SLIDER;
						}
						else if(noteflags & EOF_NOTE_FLAG_HOPO)
						{	//If this is a HOPO note
							imagenum = p ? EOF_IMAGE_NOTE_GHL_BARRE_SP_HOPO_HIT : EOF_IMAGE_NOTE_GHL_BARRE_SP_HOPO;
						}
						else
						{	//This is not a HOPO note
							imagenum = p ? EOF_IMAGE_NOTE_GHL_BARRE_SP_HIT : EOF_IMAGE_NOTE_GHL_BARRE_SP;
						}
						notenote &= ~barremask;		//Prevent drawing another gem for the other gem in the barre, since the barre note graphic already represents both
					}
					else if(noteflags & EOF_GUITAR_NOTE_FLAG_IS_SLIDER)
					{	//Black SP GHL slider
						imagenum = EOF_IMAGE_NOTE_GHL_BLACK_SP_SLIDER;
					}
					else if(noteflags & EOF_NOTE_FLAG_HOPO)
					{	//If this is a HOPO note
						imagenum = p ? EOF_IMAGE_NOTE_GHL_BLACK_SP_HOPO_HIT : EOF_IMAGE_NOTE_GHL_BLACK_SP_HOPO;
					}
					else
					{	//This is not a HOPO note
						imagenum = p ? EOF_IMAGE_NOTE_GHL_BLACK_SP_HIT : EOF_IMAGE_NOTE_GHL_BLACK_SP;
					}
				}
				else
				{	//The next three lanes are white notes
					if(noteflags & EOF_GUITAR_NOTE_FLAG_IS_SLIDER)
					{	//White SP GHL slider
						imagenum = EOF_IMAGE_NOTE_GHL_WHITE_SP_SLIDER;
					}
					else if(noteflags & EOF_NOTE_FLAG_HOPO)
					{	//If this is a HOPO note
						imagenum = p ? EOF_IMAGE_NOTE_GHL_WHITE_SP_HOPO_HIT : EOF_IMAGE_NOTE_GHL_WHITE_SP_HOPO;
					}
					else
					{	//This is not a HOPO note
						imagenum = p ? EOF_IMAGE_NOTE_GHL_WHITE_SP_HIT : EOF_IMAGE_NOTE_GHL_WHITE_SP;
					}
				}
			}
			else
			{	//This is not a SP note
				unsigned long barremask = mask | (mask << 3);	//This represents the note mask of the gem and a gem 3 lanes higher

				if((ctr < 3) && ((notenote & barremask) == barremask))
				{	//If this is to be rendered as a barre note
					if(noteflags & EOF_GUITAR_NOTE_FLAG_IS_SLIDER)
					{	//Barre GHL slider
						imagenum = EOF_IMAGE_NOTE_GHL_BARRE_SLIDER;
					}
					else if(noteflags & EOF_NOTE_FLAG_HOPO)
					{	//If this is a HOPO note
						imagenum = p ? EOF_IMAGE_NOTE_GHL_BARRE_HOPO_HIT : EOF_IMAGE_NOTE_GHL_BARRE_HOPO;
					}
					else
					{	//This is not a HOPO note
						imagenum = p ? EOF_IMAGE_NOTE_GHL_BARRE_HIT : EOF_IMAGE_NOTE_GHL_BARRE;
					}
					notenote &= ~barremask;		//Prevent drawing another gem for the other gem in the barre, since the barre note graphic already represents both
				}
				else
				{	//Render a normal, non SP note
					if(noteflags & EOF_GUITAR_NOTE_FLAG_IS_SLIDER)
					{	//Slider
						imagenum = eof_colors[ctr].slider3d;
					}
					else if(noteflags & EOF_NOTE_FLAG_HOPO)
					{	//If this is a HOPO note
						imagenum = p ? eof_colors[ctr].hoponotehit3d : eof_colors[ctr].hoponote3d;
					}
					else
					{	//This is not a HOPO note
						imagenum = p ? eof_colors[ctr].notehit3d : eof_colors[ctr].note3d;
					}
				}
			}
		}//If rendering a Guitar Hero Live style track
		else if(eof_song->track[track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
		{	//If rendering a drum note
			if(eof_track_is_drums_rock_mode(eof_song, track))
			{	//Drums Rock mode render logic
				if((mask == 2) || (mask == 16))
				{	//Lanes 2 (blue) and 5 (green) always display as cymbals
					imagenum = p ? eof_colors[ctr].cymbalhit3d : eof_colors[ctr].cymbal3d;
				}
				else
				{	//All other lanes always display as non cymbals
					imagenum = p ? eof_colors[ctr].notehit3d : eof_colors[ctr].note3d;
				}
			}
			else
			{	//Normal drum render logic
				if(((noteflags & EOF_DRUM_NOTE_FLAG_Y_CYMBAL) && (mask == 4)) || ((noteflags & EOF_DRUM_NOTE_FLAG_B_CYMBAL) && (mask == 8)) || ((noteflags & EOF_DRUM_NOTE_FLAG_G_CYMBAL) && (mask == 16)))
				{	//If this is a cymbal note, render with the cymbal image
					if(noteflags & EOF_NOTE_FLAG_SP)
					{	//If this cymbal note is star power, render it in silver
						imagenum = p ? EOF_IMAGE_NOTE_WHITE_CYMBAL_HIT : EOF_IMAGE_NOTE_WHITE_CYMBAL;
					}
					else
					{	//Otherwise render in the appropriate color
						imagenum = p ? eof_colors[ctr].cymbalhit3d : eof_colors[ctr].cymbal3d;
					}
				}
				else
				{	//Otherwise render with the standard note image
					if(noteflags & EOF_NOTE_FLAG_SP)
					{	//If this drum note is star power, render it in silver
						imagenum = p ? EOF_IMAGE_NOTE_WHITE_HIT: EOF_IMAGE_NOTE_WHITE;
					}
					else
					{	//Otherwise render in the appropriate color
						imagenum = p ? eof_colors[ctr].notehit3d : eof_colors[ctr].note3d;
					}
				}
			}
		}//If rendering a drum note
		else if(track == EOF_TRACK_DANCE)
		{	//If rendering a dance note
			imagenum = p ? eof_colors[ctr].arrowhit3d : eof_colors[ctr].arrow3d;
		}
		else
		{	//If rendering a normal note
			unsigned long color = ctr;	//By default, the color will be determined by the gem's lane number

			imagenum = 0;
			if((eof_song->track[track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT) && (eof_color_set == EOF_COLORS_BF))
			{	//If a pro guitar track is active and the Bandfuse color set is in use, override the color based on the gem's fingering
				EOF_PRO_GUITAR_TRACK *tp = eof_song->pro_guitar_track[tracknum];

				if(tp->note[notenum]->flags & EOF_PRO_GUITAR_NOTE_FLAG_TAP)
				{	//If the note is tapped
					imagenum = p ? EOF_IMAGE_NOTE_BLACK_HIT : EOF_IMAGE_NOTE_BLACK;	//Draw the note in black
				}
				else
				{
					unsigned fingering = eof_pro_guitar_note_lookup_string_fingering(tp, notenum, ctr, 6);	//Look up this gem's fingering (or return 6 if cannot be determined)

					if(fingering < 6)
					{	//If the finger was determined
						color = fingering;	//Use the fingering's appropriate color
					}
					else
					{	//Otherwise use the default silvering coloring
						noteflags |= EOF_NOTE_FLAG_SP;	//And trigger the selection of the appropriate corresponding border color
					}
				}
			}

			if(!imagenum)
			{	//If the appropriate 3D image wasn't determined yet
				if(eof_track_is_legacy_guitar(eof_song, track) && (noteflags & EOF_GUITAR_NOTE_FLAG_IS_SLIDER))
				{	//If the specified note is a slider note
					if(noteflags & EOF_NOTE_FLAG_SP)
					{	//If it is also an SP note
						imagenum = EOF_IMAGE_NOTE_WHITE_SLIDER;
					}
					else
					{
						imagenum = eof_colors[color].slider3d;
					}
				}
				else if(noteflags & EOF_NOTE_FLAG_HOPO)
				{	//If this is a HOPO note
					if(noteflags & EOF_NOTE_FLAG_SP)
					{	//If this is also a SP note
						imagenum = p ? EOF_IMAGE_NOTE_HWHITE_HIT : EOF_IMAGE_NOTE_HWHITE;
					}
					else
					{
						imagenum = p ? eof_colors[color].hoponotehit3d : eof_colors[color].hoponote3d;
					}
				}
				else
				{	//This is not a HOPO note
					if(noteflags & EOF_NOTE_FLAG_SP)
					{	//If this is an SP note
						imagenum = p ? EOF_IMAGE_NOTE_WHITE_HIT : EOF_IMAGE_NOTE_WHITE;
					}
					else
					{
						imagenum = p ? eof_colors[color].notehit3d : eof_colors[color].note3d;
					}
				}
			}//If the appropriate 3D image wasn't determined yet
		}//If rendering a normal note

		//Render the note
		if(eof_full_height_3d_preview)
		{	//If full height 3D preview is in effect, stretch the gems to double height to make them look less squished
			BITMAP **dest_bitmap;	//This will refer to either stretch_bitmap or hopo_stretch_bitmap accordingly

			//Re-use either the normal or HOPO size bitmaps from a previous call to eof_note_draw_3d() where possible to reduce overhead
			if(noteflags & EOF_NOTE_FLAG_HOPO)
			{	//If the note being rendered is a HOPO note
				dest_bitmap = &eof_hopo_stretch_bitmap;
			}
			else
			{
				dest_bitmap = &eof_stretch_bitmap;
			}
			height_scale = (double)eof_window_3d->screen->h / eof_window_3d->screen->w;
			image_height = ((double)eof_image[imagenum]->h * height_scale) + 0.5;	//Scale the image height based on the 3D window dimensions to try to retain a correct aspect ratio

			if(eof_full_screen_3d)
			{	//The full screen 3D feature makes the notes look even more squished
				image_height *= 2;	//Make the notes even taller
			}
			half_image_width = eof_image[imagenum]->w / 2;
			if(((*dest_bitmap) == NULL) || ((*dest_bitmap)->w != eof_image[imagenum]->w) || ((*dest_bitmap)->h != image_height))
			{	//If the temporary bitmap must be recreated to store the image
				if(*dest_bitmap != NULL)
					destroy_bitmap(*dest_bitmap);
				*dest_bitmap = create_bitmap(eof_image[imagenum]->w, image_height);
				eof_log("\t\tRebuilding 3D stretched image cache", 3);
			}

			if(*dest_bitmap)
			{	//If the bitmap was created or can be reused
				stretch_blit(eof_image[imagenum], *dest_bitmap, 0, 0, eof_image[imagenum]->w, eof_image[imagenum]->h, 0, 0, (*dest_bitmap)->w, (*dest_bitmap)->h);
				ocd3d_draw_bitmap(eof_window_3d->screen, *dest_bitmap, xchart[lanenum] - half_image_width - xoffset, 200 - image_height + offset_y_3d, npos);
			}
		}
		else
		{	//Normal logic
			image_height = eof_image[imagenum]->h;
			half_image_width = eof_image[imagenum]->w / 2;
			ocd3d_draw_bitmap(eof_window_3d->screen, eof_image[imagenum], xchart[lanenum] - half_image_width - xoffset, 200 - image_height + offset_y_3d, npos);
		}

		if(!eof_legacy_view && (notenote & mask) && (eof_song->track[track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
		{	//If legacy view is disabled and this is a pro guitar note, render the fret number over the center of the note
			BITMAP *fretbmp = eof_create_fret_number_bitmap(eof_song->pro_guitar_track[tracknum]->note[notenum], NULL, ctr, 8, eof_color_white, eof_color_black, font);	//Allow one extra character's width for padding
			if(fretbmp != NULL)
			{	//Render the bitmap on top of the 3D note and then destroy the bitmap
				if(eof_full_height_3d_preview)
				{	//If the fret number bitmap should be scaled also
					BITMAP *scaledbmp = create_bitmap(fretbmp->w, (double)fretbmp->h * height_scale);
					if(scaledbmp)
					{	//If the scaled bitmap was created
						stretch_blit(fretbmp, scaledbmp, 0, 0, fretbmp->w, fretbmp->h, 0, 0, scaledbmp->w, scaledbmp->h);
						destroy_bitmap(fretbmp);
						fretbmp = scaledbmp;
					}
				}
				ocd3d_draw_bitmap(eof_window_3d->screen, fretbmp, xchart[lanenum] - 8, 200 - (image_height / 2) + offset_y_3d, npos);
				destroy_bitmap(fretbmp);
			}
		}
		else if(track == EOF_TRACK_DRUM_PS)
		{	//If this was a note in the Phase Shift drum track
			if(	((mask == 4) && (noteflags & EOF_DRUM_NOTE_FLAG_Y_COMBO)) ||
				((mask == 8) && (noteflags & EOF_DRUM_NOTE_FLAG_B_COMBO)) ||
				((mask == 16) && (noteflags & EOF_DRUM_NOTE_FLAG_G_COMBO)))
			{	//If the gem just drawn is a tom/cymbal combo
				int x, y, x2;
				x = ocd3d_project_x(xchart[lanenum] - xoffset, npos);
				y = ocd3d_project_y(200 - (image_height / 4) + offset_y_3d, npos);
				x2 = ocd3d_project_x(xchart[lanenum] + (xchart[1] - xchart[0]) - xoffset, npos);	//The x coordinate one lane over
				if((x != -65536) && (y != -65536))
				{	//If none of the coordinate projections failed
					circlefill(eof_window_3d->screen, x, y, (x2 - x) / 6, eof_color_black);	//Draw a large dot in the center of the cymbal's 3D image
				}
			}
		}
	}//For each lane used in this note

	if(eof_hide_note_names)
		return 0;	//If the user opted to hide note names, skip the logic below

	notename[0] = prevnotename[0] = '\0';	//Empty these strings
	namefound = eof_build_note_name(eof_song, track, notenum, notename);
	if(!namefound)
		return 0;	//If the note has no name, return from function early

	//Otherwise prepare the name for rendering
	(void) eof_build_note_name(eof_song, track, eof_get_prev_note_type_num(eof_song, track, notenum), prevnotename);	//Get the previous note's name
	if(!ustrcmp(notename, prevnotename))
	{	//If this note and the previous one have the same name
		if(namefound == 1)
		{	//If the name for this note was manually assigned
			nameptr = samename;	//Display this note's name as "/" to indicate a repeat of the last note
		}
		else
		{	//The name for this note was detected
			nameptr = samenameauto;	//Display this note's name as "[/]" to indicate a repeat of the last note
		}
	}
	else
	{	//This note doesn't have the same name as the previous note
		if(namefound == 1)
		{	//If the name for this note was manually assigned
			nameptr = notename;	//Display the note name as-is
		}
		else
		{	//The name for this note was detected
			(void) snprintf(prevnotename, sizeof(prevnotename) - 1, "[%s]", notename);	//Rebuild the note name to be enclosed in brackets
			nameptr = prevnotename;
		}
	}
	if(eof_track_is_drums_rock_mode(eof_song, track))
	{	//Special case:  Drums Rock mode is enabled
		nameptr = notename;	//Display the note name as-is since it instead defines the drum roll count
	}
	z3d = npos + 6 + text_height(font);	//Restore the 6 that was subtracted earlier when finding npos, and add the font's height to have the text line up with the note's z position
	z3d = z3d < eof_3d_min_depth ? eof_3d_min_depth : z3d;
	x3d = ocd3d_project_x(20 - 4, z3d);
	y3d = ocd3d_project_y(200 + offset_y_3d, z3d);
	textout_right_ex(eof_window_3d->screen, font, nameptr, x3d, y3d, eof_color_white, -1);

	return 0;	//Return status:  Note was not clipped in its entirety
}

int eof_note_tail_draw_3d(unsigned long track, unsigned long notenum, int p)
{
	long npos;
	int point[8];
	int rz, ez;
	unsigned long numlanes, ctr, mask, tracknum;
	int offset_y_3d = 0;	//If full height 3D preview is in effect, y coordinates will be shifted down by one panel height, otherwise will shift by 0

	//These variables are used to store the common note data, regardless of whether the note is legacy or pro guitar format
	unsigned long notepos = 0;
	long notelength = 0;
	unsigned long noteflags = 0;
	unsigned long notenote = 0;

//Validate parameters
	if((track == 0) || (track >= eof_song->tracks) || ((eof_song->track[track]->track_format != EOF_LEGACY_TRACK_FORMAT) && (eof_song->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)) || (notenum >= eof_get_track_size(eof_song, track)))
	{	//If an invalid track or note number was passsed
		return -1;	//Error, signal to stop rendering (3D window renders last note to first)
	}
	notepos = eof_get_note_pos(eof_song, track, notenum);
	notelength = eof_get_note_length(eof_song, track, notenum);
	noteflags = eof_get_note_flags(eof_song, track, notenum);
	notenote = eof_get_note_note(eof_song, track, notenum);

	if(eof_song->track[track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
		return 0;	//Don't render tails for drum notes
	if(eof_track_is_ghl_mode(eof_song, track) && (noteflags & EOF_GUITAR_NOTE_FLAG_GHL_OPEN))
		return 0;	//Don't render tails on GHL open notes

	if(eof_full_height_3d_preview)
	{
		offset_y_3d = eof_screen_height / 2;	//Y coordinates will be lowered by one panel height so that the bottom of the 3D fret board is at the bottom of the program window
	}
	if(eof_render_3d_rs_chords && (eof_song->track[track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT) && (eof_note_count_colors(eof_song, track, notenum) > 1))
	{	//If the user has opted to 3D render Rocksmith style chords, and this is a pro guitar/bass chord
		if(!eof_get_rs_techniques(eof_song, track, notenum, 0, NULL, 2, 1))
		{	//If the chord does not contain any techniques that would require chordNotes to be written (to display with a sustain in RS2)
			return 0;	//Don't render the tail
		}
	}

	tracknum = eof_song->track[track]->tracknum;
	if((eof_song->track[track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT) && ((eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT) || eof_legacy_view))
	{	//If the catalog entry is a pro guitar note and the active track is not, or the user specified to display pro guitar notes as legacy notes
		if(eof_song->pro_guitar_track[tracknum]->note[notenum]->legacymask != 0)
		{	//If the user defined how this pro guitar note would transcribe to a legacy track
			notenote = eof_song->pro_guitar_track[tracknum]->note[notenum]->legacymask;
		}
		else
		{
			notenote &= 31;	//Mask out lane 6 (this is how it would be treated when notes were created via a paste operation)
		}
	}

	npos = (long)(notepos + eof_av_delay - eof_music_pos.value) / eof_zoom_3d  - 6;
	if(npos + notelength / eof_zoom_3d < eof_3d_min_depth)
	{
		return -1;
	}
	else if(npos > eof_3d_max_depth)
	{
		return 1;
	}

	//Determine the width of the fret lanes
	numlanes = eof_count_track_lanes(eof_song, track);	//Count the number of lanes in that note's track
	if(eof_track_is_ghl_mode(eof_song, track))
	{	//Special case:  Guitar Hero Live style tracks display with 3 lanes
		numlanes = 3;
	}
	else if(eof_open_strum_enabled(track))
	{	//Special case:  5 lane guitar/bass tracks can use a sixth lane but its 3D representation still only draws 5 lanes
		numlanes = 5;
	}
	rz = npos < eof_3d_min_depth ? eof_3d_min_depth : npos + 6;
	ez = npos + notelength / eof_zoom_3d > eof_3d_max_depth ? eof_3d_max_depth : npos + notelength / eof_zoom_3d + 6;
	for(ctr=0,mask=1; ctr < eof_count_track_lanes(eof_song, track); ctr++,mask=mask<<1)
	{	//For each of the lanes in this track
		unsigned long nextnotenum;
		long npos2, rz2;
		unsigned long notepos2, nextnotenote, ctr2, mask2;		//Used for slide note rendering
		unsigned lanenum, lanenum2;

		assert(ctr < EOF_MAX_FRETS);	//Put an assertion here to resolve a false positive with Coverity
		if(!(notenote & mask))
			continue;	//If this lane does not have a gem to render, skip it

		lanenum = ctr;	//By default, each gem gets its own lane number
		if(!eof_track_is_ghl_mode(eof_song, track) && (ctr == 5) && eof_track_is_legacy_guitar(eof_song, track))
		{	//If this is NOT a GHL style track and if drawing the tail of a gem on lane 6 of a legacy guitar track
			if(eof_open_strum_enabled(track))
			{	//And open strum notes are enabled, render open strum notes (a rectangle covering the width of rendering of frets 2, 3 and 4
				point[0] = ocd3d_project_x(xchart[1] - 10, rz);
				point[1] = ocd3d_project_y(200 + offset_y_3d, rz);
				point[2] = ocd3d_project_x(xchart[1] - 10, ez);
				point[3] = ocd3d_project_y(200 + offset_y_3d, ez);
				point[4] = ocd3d_project_x(xchart[3] + 10, ez);
				point[5] = point[3];
				point[6] = ocd3d_project_x(xchart[3] + 10, rz);
				point[7] = point[1];
				polygon(eof_window_3d->screen, 4, point, (noteflags & EOF_NOTE_FLAG_SP) ? (p ? eof_color_white : eof_color_silver) : (p ? eof_colors[ctr].hit : eof_colors[ctr].color));
			}
		}
		else
		{	//Logic to render lanes 1 through 6
			unsigned long color = ctr;	//By default, the color will be determined by the gem's lane number

			if((eof_song->track[track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT) && (eof_color_set == EOF_COLORS_BF))
			{	//If a pro guitar track is active and the Bandfuse color set is in use, override the color based on the gem's fingering
				unsigned fingering = eof_pro_guitar_note_lookup_string_fingering(eof_song->pro_guitar_track[tracknum], notenum, ctr, 6);	//Look up this gem's fingering (or return 6 if cannot be determined)

				if(fingering < 6)
				{	//If the finger was determined
					color = fingering;	//Use the fingering's appropriate color
				}
				else
				{	//Otherwise use the default silvering coloring
					noteflags |= EOF_NOTE_FLAG_SP;	//And trigger the selection of the appropriate corresponding border color
				}
			}

			if(eof_track_is_ghl_mode(eof_song, track))
			{	//Special case:  Guitar Hero Live style tracks display with 3 lanes
				if(ctr < 3)
				{	//If drawing a gem for one of the first three lanes
					unsigned long barremask = mask | (mask << 3);	//This represents the note mask of the gem and a gem 3 lanes higher

					if((notenote & barremask) == barremask)
					{	//If this is to be rendered as a barre note
						notenote &= ~barremask;		//Skip drawing a tail for the matching black GHL note, since the barre represents both the relevant black and white gems
					}
				}

				lanenum = ctr % 3;	//Gems 1 through 3 use the same lanes as gems 4 through 6
			}
			point[0] = ocd3d_project_x(xchart[lanenum] - 10, rz);
			point[1] = ocd3d_project_y(200 + offset_y_3d, rz);
			point[2] = ocd3d_project_x(xchart[lanenum] - 10, ez);
			point[3] = ocd3d_project_y(200 + offset_y_3d, ez);
			point[4] = ocd3d_project_x(xchart[lanenum] + 10, ez);
			point[5] = point[3];
			point[6] = ocd3d_project_x(xchart[lanenum] + 10, rz);
			point[7] = point[1];
			polygon(eof_window_3d->screen, 4, point, (noteflags & EOF_NOTE_FLAG_SP) ? (p ? eof_color_white : eof_color_silver) : (p ? eof_colors[color].hit : eof_colors[color].color));
		}

		//Render pro guitar note slide if applicable
		if((track != 0) && (eof_song->track[track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT) && ((noteflags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP) || (noteflags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN) || (noteflags & EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE)))
		{	//If rendering an existing pro guitar track that slides up or down or is an unpitched slide
			unsigned long halflanewidth = (56.0 * (4.0 / (numlanes-1))) / 2;
			int slidecolor = eof_color_dark_purple;	//By default, pro guitar slides are drawn in purple
			char up = 0;		//Will be assumed to be a down slide by default

			if(noteflags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_REVERSE)
				slidecolor = eof_color_white;	//If it's a reverse slide though, draw in white
			if(noteflags & EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE)
				slidecolor = eof_color_black;	//If it's an unpitched slide, draw in black

			if(noteflags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP)
				up = 1;
			else if(noteflags & EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE)
			{
				unsigned char lowestfret = eof_get_lowest_fret_value(eof_song, track, notenum);	//Determine the fret value of the lowest fretted string
				if(lowestfret < eof_song->pro_guitar_track[eof_song->track[track]->tracknum]->note[notenum]->unpitchend)
				{	//If the unpitched slide goes higher than this position
					up = 1;
				}
			}

			notepos2 = notepos + notelength;	//Find the position of the end of the note
			npos2 = (long)(notepos2 + eof_av_delay - eof_music_pos.value) / eof_zoom_3d  - 6;
			rz2 = npos2 < eof_3d_min_depth ? eof_3d_min_depth : npos2 + 10;

			//Define the slide rectangle coordinates in clockwise order
			#define EOF_PRO_GUITAR_SLIDE_LINE_THICKNESS_3D 4
			if(up)
			{	//If this note slides up (3D view from left to right), start the slide line at the left of this note
				point[0] = ocd3d_project_x(xchart[ctr] - halflanewidth, rz);	//X1 (X coordinate of the front end of the slide)
			}
			else
			{	//Otherwise start the slide line at the right of this note
				point[0] = ocd3d_project_x(xchart[ctr] + halflanewidth, rz);	//X1 (X coordinate of the front end of the slide)
			}
			point[1] = ocd3d_project_y(200 + offset_y_3d, rz);	//Y1 (Y coordinate of the front end of the slide)
			if(up)
			{	//If this note slides up (3D view from left to right), end the slide line at the right of the next note
				point[2] = ocd3d_project_x(xchart[ctr] + halflanewidth, rz2);	//X2 (X coordinate of the back end of the slide)
			}
			else
			{	//Otherwise end the slide line at the left of the next note
				point[2] = ocd3d_project_x(xchart[ctr] - halflanewidth, rz2);	//X2 (X coordinate of the back end of the slide)
			}
			point[3] = ocd3d_project_y(200 + offset_y_3d, rz2);	//Y2 (Y coordinate of the back end of the slide
			point[4] = point[2] + (2 * EOF_PRO_GUITAR_SLIDE_LINE_THICKNESS_3D);	//X3 (the specified number of pixels right of X2)
			point[5] = point[3];	//Y3 (Y coordinate of the back end of the slide)
			point[6] = point[0] + (2 * EOF_PRO_GUITAR_SLIDE_LINE_THICKNESS_3D);	//X4 (the specified number of pixels right of X1)
			point[7] = point[1];	//Y4 (Y coordinate of the front end of the slide)
			polygon(eof_window_3d->screen, 4, point, slidecolor);	//Render the 4 point polygon in the appropriate color
		}//If rendering an existing pro guitar track that slides up or down or is an unpitched slide

		//Render slider note slide if applicable
		if(((eof_song->track[track]->track_behavior != EOF_GUITAR_TRACK_BEHAVIOR) && (track != EOF_TRACK_KEYS)) || !(noteflags & EOF_GUITAR_NOTE_FLAG_IS_SLIDER))
			continue;	//If the note isn't in a guitar/keys track or isn't in a slider, skip the remaining logic that renders slider phrases
		if(eof_track_fixup_next_note(eof_song, track, notenum) < 0)
			continue;	//If there isn't another note in this difficulty, don't render a slider phrase for a single note

		nextnotenum = eof_track_fixup_next_note(eof_song, track, notenum);
		if(!(eof_get_note_flags(eof_song, track, nextnotenum) & EOF_GUITAR_NOTE_FLAG_IS_SLIDER))
			continue;	//If that next note is not also a slider note, skip the logic to render the slider phrase

		//Otherwise draw a dark purple line between this note and the next
		nextnotenote = eof_get_note_note(eof_song, track, nextnotenum);
		for(ctr2=0,mask2=1; ctr2 < eof_count_track_lanes(eof_song, track); ctr2++,mask2=mask2<<1)
		{
			if(nextnotenote & mask2)
			{	//If this lane is populated for the next note
				break;
			}
		}

		notepos2 = eof_get_note_pos(eof_song, track, nextnotenum);	//Find the position of the next note
		npos2 = (long)(notepos2 + eof_av_delay - eof_music_pos.value) / eof_zoom_3d  - 6;
		rz2 = npos2 < eof_3d_min_depth ? eof_3d_min_depth : npos2 + 10;

		if(eof_track_is_ghl_mode(eof_song, track))
		{	//Special case:  Guitar Hero Live style tracks display with 3 lanes
			lanenum2 = ctr2 % 3;	//Gems 1 through 3 use the same lanes as gems 4 through 6
		}
		else
		{
			lanenum2 = ctr2;		//Otherwise each gem uses its own lane
		}

		//Define the slide rectangle coordinates in clockwise order
		point[0] = ocd3d_project_x(xchart[lanenum], rz);	//X1 (X coordinate of the front end of the slide): The X position of this note
		point[1] = ocd3d_project_y(200 + offset_y_3d, rz);				//Y1 (Y coordinate of the front end of the slide): The Y position of this note
		point[2] = ocd3d_project_x(xchart[lanenum2], rz2);	//X2 (X coordinate of the back end of the slide): The X position of the next note
		point[3] = ocd3d_project_y(200 + offset_y_3d, rz2);				//Y2 (Y coordinate of the back end of the slide): The Y position of the next note

		point[4] = point[2] + (2 * EOF_PRO_GUITAR_SLIDE_LINE_THICKNESS_3D);	//X3 (the specified number of pixels right of X2)
		point[5] = point[3];							//Y3 (Y coordinate of the back end of the slide)
		point[6] = point[0] + (2 * EOF_PRO_GUITAR_SLIDE_LINE_THICKNESS_3D);	//X4 (the specified number of pixels right of X1)
		point[7] = point[1];							//Y4 (Y coordinate of the front end of the slide)
		polygon(eof_window_3d->screen, 4, point, eof_color_dark_purple);	//Render the 4 point polygon in dark purple
	}//For each of the lanes in this track
	return 0;
}

EOF_PHRASE_SECTION *eof_find_lyric_line(unsigned long lyricnum)
{
	unsigned long linectr;
	unsigned long lyricpos;

	if(eof_song == NULL)
		return NULL;
	if(lyricnum >= eof_song->vocal_track[0]->lyrics)
	{
		return NULL;
	}
	lyricpos=eof_song->vocal_track[0]->lyric[lyricnum]->pos;

	for(linectr=0;linectr<eof_song->vocal_track[0]->lines;linectr++)
	{
		if((eof_song->vocal_track[0]->line[linectr].start_pos <= lyricpos) && (eof_song->vocal_track[0]->line[linectr].end_pos >= lyricpos))
			return &(eof_song->vocal_track[0]->line[linectr]);	//Line found, return it
	}

	return NULL;	//No such line found
}

unsigned long eof_find_lyric_number(EOF_LYRIC * np)
{
	unsigned long ctr;

	if(np == NULL)
	{
		return 0;
	}
	for(ctr = 0; ctr < eof_song->vocal_track[0]->lyrics; ctr++)
	{	//For each lyric
		if(np == eof_song->vocal_track[0]->lyric[ctr])
			return ctr;
	}

	return 0;
}

BITMAP *eof_create_fret_number_bitmap(EOF_PRO_GUITAR_NOTE *note, char *text, unsigned char stringnum, unsigned long padding, int textcol, int fillcol, FONT *font)
{
	BITMAP *fretbmp = NULL;
	int height, width;
	char fretstring[10] = {0};

	if(!note && !text)
		return NULL;	//Invalid parameters
	if(!font)
		return NULL;

	if(note != NULL)
	{	//If the specified note's fretting will be rendered to the bitmap
		if(note->frets[stringnum] & 0x80)
		{	//This is a muted fret
			(void) snprintf(fretstring, sizeof(fretstring) - 1, "X");
		}
		else
		{	//This is a non muted fret
			if(note->ghost & (1 << stringnum))
			{	//This is a ghosted note
				(void) snprintf(fretstring, sizeof(fretstring) - 1,"(%u)", note->frets[stringnum]);
				if(note->eflags & EOF_PRO_GUITAR_NOTE_EFLAG_GHOST_HS)
				{	//If this note has ghost handshape status
					fillcol = eof_color_red;
					textcol = eof_color_blue;
				}
			}
			else
			{	//This is a normal note
				(void) snprintf(fretstring, sizeof(fretstring) - 1,"%u", note->frets[stringnum]);
			}
		}
		text = fretstring;	//This string will be rendered to the bitmap
	}

	width = text_length(font, text) + padding + 1;	//The font in use doesn't look centered, so pad the left by one pixel
	height = text_height(font);
	fretbmp = create_bitmap(width, height);
	if(fretbmp != NULL)
	{	//Render the fret number on top of the 3D note
		clear_to_color(fretbmp, fillcol);
		rect(fretbmp, 0, 0, width - 1, height - 1, textcol);	//Draw a border along the edge of this bitmap
		textprintf_ex(fretbmp, font, (padding / 2.0) + 1, 0, textcol, -1, "%s", text);	//Center the text between the padding (including one extra pixel for left padding), rounding to the right if the padding is an odd value
	}

	return fretbmp;
}

void eof_get_note_notation(char *buffer, unsigned long track, unsigned long note, unsigned char sanitycheck)
{
	unsigned long index = 0, flags = 0, eflags = 0, number = 0;
	char buffer2[5] = {0};
	int ret;

	if(!track || (track >= eof_song->tracks) || (buffer == NULL) || ((eof_song->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT) && (eof_song->track[track]->track_format != EOF_LEGACY_TRACK_FORMAT)))
	{
		return;	//If this is an invalid track number, the buffer is NULL or the specified track isn't a pro guitar or legacy track, return
	}
	if(note >= eof_get_track_size(eof_song, track))
	{
		return;
	}
	flags = eof_get_note_flags(eof_song, track, note);
	eflags = eof_get_note_eflags(eof_song, track, note);

	if(eflags & EOF_NOTE_EFLAG_SP_DEPLOY)
	{	//If the note has SP deploy status
		buffer[index++] = 'm';	//In the symbols font, m is the star character
	}
	if(eof_song->track[track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
	{	//Check pro guitar statuses
		unsigned long tracknum = eof_song->track[track]->tracknum, index2;
		EOF_PRO_GUITAR_TRACK *tp = eof_song->pro_guitar_track[tracknum];
		EOF_PRO_GUITAR_NOTE *np = tp->note[note];
		unsigned char lowestfret = eof_pro_guitar_note_lowest_fret(tp, note);	//Determine the lowest used fret value

		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_HO)
		{
			buffer[index++] = 'h';
		}
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_PO)
		{
			buffer[index++] = 'p';
		}
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_TAP)
		{
			buffer[index++] = 'T';
		}
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_BEND)
		{
			if(eflags & EOF_PRO_GUITAR_NOTE_EFLAG_PRE_BEND)
			{	//If the note has pre-bend status
				buffer[index++] = 'l';	//In the symbols font, l is the pre-bend character
			}
			else
			{
				buffer[index++] = 'a';	//In the symbols font, a is the bend character
			}
			if(flags & EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION)
			{	//If the note defines the strength of its bend
				unsigned char bendstrength = np->bendstrength & 0x7F;	//Mask out the MSB
				if(np->bendstrength & 0x80)
				{	//If the bend strength is defined in quarter steps
					(void) snprintf(buffer2, sizeof(buffer2) - 1, "%d", bendstrength / 2);	//Build a string out of the bend strength converted to half steps
					for(index2 = 0; buffer2[index2] != '\0'; index2++)
					{	//For each character in the string
						buffer[index++] = buffer2[index2];	//Append it to the notation string
					}
					if(bendstrength % 2)
					{	//Write any remaining quarter bend value as a decimal
						buffer[index++] = '.';
						buffer[index++] = '5';
					}
				}
				else
				{	//The bend strength is defined in half steps
					(void) snprintf(buffer2, sizeof(buffer2) - 1, "%d", bendstrength);	//Build a string out of the bend strength
					for(index2 = 0; buffer2[index2] != '\0'; index2++)
					{	//For each character in the string
						buffer[index++] = buffer2[index2];	//Append it to the notation string
					}
				}
			}
		}
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_HARMONIC)
		{
			buffer[index++] = 'b';	//In the symbols font, b is the harmonic character
		}
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_VIBRATO)
		{
			buffer[index++] = 'V';
		}
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP)
		{
			buffer[index++] = '/';
			if(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_REVERSE)	//If the slide is reversed
				buffer[index++] = '/';							//Double the slide indicator
		}
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN)
		{
			buffer[index++] = '\\';
			if(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_REVERSE)	//If the slide is reversed
				buffer[index++] = '\\';							//Double the slide indicator
		}
		if(((flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP) || (flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN)) && (flags & EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION))
		{	//If the note slides up or down and defines the ending fret for the slide
			if(((np->slideend == lowestfret) || !lowestfret) && sanitycheck)
			{	//If the slide is not valid (it ends on the same fret it starts on or the note/chord is played open) and sanity checking is enabled
				buffer[index++] = '?';	//Place this character to alert the user
			}
			else
			{	//The slide is valid
				(void) snprintf(buffer2, sizeof(buffer2) - 1, "%u", np->slideend);	//Build a string out of the ending fret value
				for(index2 = 0; buffer2[index2] != '\0'; index2++)
				{	//For each character in the string
					buffer[index++] = buffer2[index2];	//Append it to the notation string
				}
			}
		}
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE)
		{
			buffer[index++] = 'P';
			buffer[index++] = 'M';
		}
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE)
		{
			buffer[index++] = 'X';
		}
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_DOWN_STRUM)
		{
			buffer[index++] = 'c';	//In the symbols font, c is the down strum character
		}
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_UP_STRUM)
		{
			buffer[index++] = 'd';	//In the symbols font, d is the up strum character
		}
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_MID_STRUM)
		{
			buffer[index++] = 'M';
		}
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_POP)
		{
			buffer[index++] = 'P';
		}
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLAP)
		{
			buffer[index++] = 'S';
		}
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_ACCENT)
		{
			buffer[index++] = '>';
		}
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_P_HARMONIC)
		{
			buffer[index++] = 'f';	//In the symbols font, f is the pinch harmonic character
		}
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_LINKNEXT)
		{
			buffer[index++] = 'g';	//In the symbols font, g is the linknext indicator
		}
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE)
		{
			if(((lowestfret == np->unpitchend) || !lowestfret) && sanitycheck)
			{	//If the unpitched slide is not valid (it ends on the same fret it starts on or the note/chord is played open) and sanity checking is enabled
				buffer[index++] = '?';	//Place this character to alert the user
			}
			else
			{	//The slide is valid
				if(lowestfret > np->unpitchend)
				{	//If the unpitched slide goes lower than this position
					buffer[index++] = 'j';	//In the symbols font, j is the unpitched slide down indicator
				}
				else if(lowestfret < np->unpitchend)
				{	//If the unpitched slide goes higher than this position
					buffer[index++] = 'i';	//In the symbols font, i is the unpitched slide up indicator
				}
				(void) snprintf(buffer2, sizeof(buffer2) - 1, "%u", np->unpitchend);	//Build a string out of the ending fret value
				for(index2 = 0; buffer2[index2] != '\0'; index2++)
				{	//For each character in the string
					buffer[index++] = buffer2[index2];	//Append it to the notation string
				}
			}
		}
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_HD)
		{
			buffer[index++] = '|';
		}
		if(flags & EOF_PRO_GUITAR_NOTE_FLAG_SPLIT)
		{
			buffer[index++] = '<';
			buffer[index++] = '-';
			buffer[index++] = '>';
		}
		if(np->eflags & EOF_PRO_GUITAR_NOTE_EFLAG_IGNORE)
		{
			buffer[index++] = 'I';
		}
		if(np->eflags & EOF_PRO_GUITAR_NOTE_EFLAG_SUSTAIN)
		{
			buffer[index++] = 's';
		}
		if(np->eflags & EOF_PRO_GUITAR_NOTE_EFLAG_STOP)
		{
			buffer[index++] = 'k';	//In the symbols font, k is the stop status indicator
		}
		if(np->eflags & EOF_PRO_GUITAR_NOTE_EFLAG_GHOST_HS)
		{
			buffer[index++] = '(';
			buffer[index++] = 'G';
			buffer[index++] = ')';
		}
		if((tp->note != tp->technote) && eof_pro_guitar_note_has_tech_note(tp, note, NULL))
		{	//If tech view is not in effect (this function isn't being used to generate the string being displayed in the tech not box), display an asterisk on notes that have an overlapping tech note
			buffer[index++] = '*';
			if(!(flags & EOF_PRO_GUITAR_NOTE_FLAG_BEND))
			{	//If the note isn't already explicitly marked with bend status
				if(eof_pro_guitar_note_bitmask_has_bend_tech_note(tp, note, 0xFF, NULL))
				{	//If any bend tech notes overlap this note
					buffer[index++] = '(';
					buffer[index++] = 'a';	//In the symbols font, a is the bend character
					buffer[index++] = ')';
				}
			}
		}
		if(np->eflags & EOF_PRO_GUITAR_NOTE_EFLAG_CHORDIFY)
		{
			buffer[index++] = 'C';
		}
		if(np->eflags & EOF_PRO_GUITAR_NOTE_EFLAG_FINGERLESS)
		{
			buffer[index++] = 'F';
		}
	}//Check pro guitar statuses
	else if((track == EOF_TRACK_DRUM) || (track == EOF_TRACK_DRUM_PS))
	{	//Check drum statuses
		if(flags & EOF_DRUM_NOTE_FLAG_Y_HI_HAT_OPEN)
		{
			buffer[index++] = 'O';
		}
		if(flags & EOF_DRUM_NOTE_FLAG_Y_HI_HAT_PEDAL)
		{
			buffer[index++] = 'P';
		}
		if(flags & EOF_DRUM_NOTE_FLAG_Y_SIZZLE)
		{
			buffer[index++] = 'S';
		}
		if(flags & EOF_DRUM_NOTE_FLAG_R_RIMSHOT)
		{
			buffer[index++] = 'R';
		}
		if(flags & EOF_DRUM_NOTE_FLAG_FLAM)
		{
			buffer[index++] = 'F';
		}
		if(eof_get_note_accent(eof_song, track, note))
		{	//If any of the lanes on this note are accented
			buffer[index++] = '>';
		}
		if(eof_get_note_ghost(eof_song, track, note))
		{	//If any of the lanes on this note are ghosted
			buffer[index++] = 'G';
		}
	}
	else if(eof_track_is_legacy_guitar(eof_song, track))
	{	//Check legacy guitar statuses
		if(flags & EOF_GUITAR_NOTE_FLAG_IS_SLIDER)
		{	//A guitar/bass note inside a slider phrase
			buffer[index++] = 'S';
		}
		if(flags & EOF_GUITAR_NOTE_FLAG_GHL_OPEN)
		{	//An GHL open note
			buffer[index++] = 'O';
		}
		else if(!eof_track_is_ghl_mode(eof_song, track) && (eof_get_note_note(eof_song, track, note) & 32))
		{	//An open note/chord in a non GHL track
			buffer[index++] = 'O';
		}
		if(flags & EOF_NOTE_FLAG_F_HOPO)
		{	//If the note is a forced HOPO
			buffer[index++] = 'h';
		}
		else if(flags & EOF_NOTE_FLAG_HOPO)
		{	//Otherwise if the note is still a HOPO (was determined to be a HOPO based on the current "Preview HOPO" setting)
			buffer[index++] = '(';
			buffer[index++] = 'h';
			buffer[index++] = ')';
		}
	}

	if((eof_song->track[track]->track_format == EOF_LEGACY_TRACK_FORMAT) && (eflags & EOF_NOTE_EFLAG_DISJOINTED))
	{	//If the note is a legacy note and has disjointed status
		buffer[index++] = 'D';
	}

	if(flags & EOF_NOTE_FLAG_IS_TRILL)
	{
		if(eof_song->track[track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
		{	//If this is a drum track, white the notation for special drum roll
			//Special case:  A note marked as a drum roll with a name of "0" will have drum roll status suppressed during Drums Rock export
			if(eof_track_is_drums_rock_mode(eof_song, track))
			{	//If Drums Rock mode is enabled for this track
				ret = eof_get_note_name_as_number(eof_song, track, note, &number);
				if(ret && !number)
				{	//If the name for this note is defined as the number 0
					buffer[index++] = '!';
				}
			}

			buffer[index++] = 'S';
			buffer[index++] = 'D';
			buffer[index++] = 'R';
		}
		else
		{	//Otherwise assume a guitar track, write the notation for a trill
			buffer[index++] = 'T';
			buffer[index++] = 'R';
		}
	}

	if(flags & EOF_NOTE_FLAG_IS_TREMOLO)
	{
		if(eof_song->track[track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
		{	//If this is a drum track, white the notation for drum roll
			//Special case:  A note marked as a drum roll with a name of "0" will have drum roll status suppressed during Drums Rock export
			if(eof_track_is_drums_rock_mode(eof_song, track))
			{	//If Drums Rock mode is enabled for this track
				ret = eof_get_note_name_as_number(eof_song, track, note, &number);
				if(ret && !number)
				{	//If the name for this note is defined as the number 0
					buffer[index++] = '!';
				}
			}

			buffer[index++] = 'D';
			buffer[index++] = 'R';
		}
		else
		{	//Otherwise assume a guitar track, write the notation for a tremolo
			buffer[index++] = 'e';	//In the symbols font, e is the tremolo character
		}
	}

	buffer[index] = '\0';
}

int eof_note_compare(EOF_SONG *sp, unsigned long track1, unsigned long note1, unsigned long track2, unsigned long note2, char thorough)
{
	unsigned long tracknum, tracknum2;
	unsigned long note1note, note2note;
	unsigned long flags, flags2, eflags, eflags2;
	unsigned char accent1, accent2, ghost1, ghost2;
	long length, length2;

	//Validate parameters
	if(!track1 || !track2 || !sp || (track1 >= sp->tracks) || (track2 >= sp->tracks) || (note1 >= eof_get_track_size(sp, track1)) || (note2 >= eof_get_track_size(sp, track2)))
	{	//If an invalid track or note number was passed
		return -1;	//Return error
	}

	if(sp->track[track1]->track_format != sp->track[track2]->track_format)	//If the two tracks are not the same format
		return 1;	//Return not equal

	note1note = eof_get_note_note(sp, track1, note1);
	note2note = eof_get_note_note(sp, track2, note2);

	if(thorough)
	{	//If the note lengths and flags are also to be compared
		flags = eof_get_note_flags(sp, track1, note1);
		flags2 = eof_get_note_flags(sp, track2, note2);
		flags &= ~EOF_NOTE_FLAG_HIGHLIGHT;	//Do not compare highlight status
		flags2 &= ~EOF_NOTE_FLAG_HIGHLIGHT;
		eflags = eof_get_note_eflags(sp, track1, note1);
		eflags2 = eof_get_note_eflags(sp, track2, note2);
		accent1 = eof_get_note_accent(sp, track1, note1);
		accent2 = eof_get_note_accent(sp, track2, note2);
		ghost1 = eof_get_note_ghost(sp, track1, note1);
		ghost2 = eof_get_note_ghost(sp, track2, note2);
		if(flags != flags2)
		{	//If the flags don't match
			return 1;	//Return not equal
		}
		if(eflags != eflags2)
		{	//If the extended flags don't match
			return 1;	//Return not equal
		}
		if(accent1 != accent2)
		{	//If the accent bitmasks don't match
			return 1;	//Return not equal
		}
		if(ghost1 != ghost2)
		{	//If the ghost bitmasks don't match
			return 1;	//Return not equal
		}
		length = eof_get_note_length(sp, track1, note1);
		length2 = eof_get_note_length(sp, track2, note2);
		if(length > length2)
		{	//If the first note is longer
			if(length2 + 3 < length)
			{	//And it's by more than 3ms
				return 1;	//Return not equal
			}
		}
		else
		{	//If the second note is longer
			if(length + 3 < length2)
			{	//And it's by more than 3ms
				return 1;	//Return not equal
			}
		}
	}

	switch(sp->track[track1]->track_format)
	{
		case EOF_LEGACY_TRACK_FORMAT:
			if(note1note == note2note)
			{	//If both notes' bitmasks match
				return 0;	//Return equal
			}
		break;

		case EOF_VOCAL_TRACK_FORMAT:
			if(note1note == note2note)
			{	//If both lyrics' pitches match
				return 0;	//Return equal
			}
		break;

		case EOF_PRO_GUITAR_TRACK_FORMAT:
			tracknum = sp->track[track1]->tracknum;
			tracknum2 = sp->track[track2]->tracknum;
		return eof_pro_guitar_note_compare(sp->pro_guitar_track[tracknum], note1, sp->pro_guitar_track[tracknum2], note2, thorough);

		default:
		break;
	}

	return 1;	//Return not equal
}

int eof_note_compare_simple(EOF_SONG *sp, unsigned long track, unsigned long note1, unsigned long note2)
{
	return eof_note_compare(sp, track, note1, track, note2, 0);
}

int eof_pro_guitar_note_compare(EOF_PRO_GUITAR_TRACK *tp1, unsigned long note1, EOF_PRO_GUITAR_TRACK *tp2, unsigned long note2, char thorough)
{
	unsigned long ctr, bitmask, note, flags1, flags2;
	unsigned long bendflags = EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION | EOF_PRO_GUITAR_NOTE_FLAG_BEND;
	unsigned long slideflags = EOF_PRO_GUITAR_NOTE_FLAG_RS_NOTATION | EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP | EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN;

	if(!tp1 || !tp2 || (note1 >= tp1->notes) || (note2 >= tp2->notes))
		return -1;	//Invalid parameters

	note = tp1->note[note1]->note;	//Cache this for easier access
	if(note != tp2->note[note2]->note)
		return 1;	//The note bitmasks don't match
	if((thorough > 1) && (tp1->note[note1]->ghost != tp2->note[note2]->ghost))	//If the ghost masks are to be compared, but they don't match
		return 1;	//The ghost masks don't match
	if(thorough > 2)
	{	//If bends and slides are to be compared
		flags1 = tp1->note[note1]->flags;
		flags2 = tp2->note[note2]->flags;
		if((flags1 & bendflags) != (flags2 & bendflags))	//If one note bends and has a defined bend strength and the other doesn't
			return 1;	//The bend statuses don't match
		if(flags1 & bendflags)
		{	//If both notes bend and have a defined bend strength
			if(tp1->note[note1]->bendstrength != tp2->note[note2]->bendstrength)
				return 1;	//The bend strengths don't match
		}
		if((flags1 & slideflags) != (flags2 & slideflags))	//If one note slides and has a defined end position and the other doesn't
			return 1;	//The slide statuses don't match
		if(flags1 & slideflags)
		{	//If both notes slide and have a defined end of slide position
			if(tp1->note[note1]->slideend != tp2->note[note2]->slideend)
				return 1;	//The slide end positions don't match
		}
		if((flags1 & EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE) != (flags2 & EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE))	//If one note unpitched slides and the other doesn't
			return 1;	//The unpitched slide statuses don't match
		if(flags1 & EOF_PRO_GUITAR_NOTE_FLAG_UNPITCH_SLIDE)
		{	//If both notes have an unpitched slide
			if(tp1->note[note1]->unpitchend != tp2->note[note2]->unpitchend)
				return 1;	//The unpitched slide end positions don't match
		}
	}

	for(ctr = 0, bitmask = 1; ctr < 6; ctr ++, bitmask <<= 1)
	{	//For each of the 6 supported strings
		if(note & bitmask)
		{	//If this string is used
			if((tp1->note[note1]->frets[ctr] & 0x7F) != (tp2->note[note2]->frets[ctr] & 0x7F))
			{	//If this string's fret value (when masking out the mute status) isn't the same for both notes
				return 1;	//Return not equal
			}
			if(thorough && ((tp1->note[note1]->frets[ctr] & 0x80) != (tp2->note[note2]->frets[ctr] & 0x80)))
			{	//If the mute status is to be compared, but they don't match
				return 1;	//Return not equal
			}
		}
	}

	return 0;	//Return equal
}

int eof_pro_guitar_note_compare_fingerings(EOF_PRO_GUITAR_NOTE *np1, EOF_PRO_GUITAR_NOTE *np2)
{
	unsigned long ctr, bitmask;

	if(!np1 || !np2)
		return -1;	//Invalid parameters

	if(np1->note != np2->note)	//If the notes don't use the same strings
		return 1;				//Return not equal

	for(ctr = 0, bitmask = 1; ctr < 6; ctr ++, bitmask <<= 1)
	{	//For each of the 6 supported strings
		if(np1->note & bitmask)
		{	//If this string is used
			if(np1->finger[ctr] != np2->finger[ctr])
			{	//If the fingering isn't identical between both notes
				return 1;	//Return not equal
			}
		}
	}
	return 0;	//Return equal
}

unsigned char eof_pro_guitar_note_lowest_fret_np(EOF_PRO_GUITAR_NOTE *np)
{
	unsigned long ctr, bitmask;
	unsigned char fret = 0xFF;

	if(!np)
		return 0;

	for(ctr = 0, bitmask = 1; ctr < 6; ctr++, bitmask <<= 1)
	{	//For each of the 6 supported strings
		if((np->note & bitmask) && (np->frets[ctr] & 0x7F) && (np->frets[ctr] != 0xFF))
		{	//If this string is used, isn't played open (masking out the mute bit) and isn't played muted with no fret specified
			if((np->frets[ctr] & 0x7F) < fret)
			{	//If this string has a lower fret value than the one being stored
				fret = np->frets[ctr] & 0x7F;
			}
		}
	}
	if(fret == 0xFF)
	{	//If all strings had a fret of 0
		return 0;
	}
	return fret;
}

unsigned char eof_pro_guitar_note_lowest_fret(EOF_PRO_GUITAR_TRACK *tp, unsigned long note)
{
	if(!tp || (note >= tp->notes))
		return 0;

	return eof_pro_guitar_note_lowest_fret_np(tp->note[note]);
}

unsigned char eof_pro_guitar_note_highest_fret(EOF_PRO_GUITAR_TRACK *tp, unsigned long note)
{
	unsigned long ctr, bitmask;
	unsigned char fret = 0;

	if(!tp || (note >= tp->notes))
		return 0;

	for(ctr = 0, bitmask = 1; ctr < tp->numstrings; ctr++, bitmask <<= 1)
	{	//For each string this track supports
		if((tp->note[note]->note & bitmask) && (tp->note[note]->frets[ctr] & 0x7F) && (tp->note[note]->frets[ctr] != 0xFF))
		{	//If this string is used, isn't played open (masking out the mute bit) and isn't played muted with no fret specified
			if((tp->note[note]->frets[ctr] & 0x7F) > fret)
			{	//If this string has a higher fret value than the one being stored
				fret = tp->note[note]->frets[ctr] & 0x7F;
			}
		}
	}
	return fret;
}

unsigned char eof_pro_guitar_note_is_barre_chord(EOF_PRO_GUITAR_TRACK *tp, unsigned long note)
{
	unsigned long ctr, bitmask, frettedstringcount = 0;
	unsigned char lowest;

	if(!tp || (note >= tp->notes))
		return 0;	//Invalid parameters

	for(ctr = 0, bitmask = 1; ctr < tp->numstrings; ctr++, bitmask <<= 1)
	{	//For each string this track supports
		if((tp->note[note]->note & bitmask) && (tp->note[note]->frets[ctr] != 0))
		{	//If this string is used and not played open (muted string is OK)
			frettedstringcount++;
		}
	}
	if(frettedstringcount < 2)
	{	//If less than two strings are played non-open
		return 0;	//This is not a barre chord
	}
	lowest = eof_pro_guitar_note_lowest_fret(tp, note);	//Get the lowest used fret in this chord

	//Determine if the lowest used fret is used on multiple, non-contiguous strings
	//Find the first string using this fret
	for(ctr = 0, bitmask = 1; ctr < tp->numstrings; ctr++, bitmask <<= 1)
	{	//For each string this track supports
		if((tp->note[note]->note & bitmask) && (tp->note[note]->frets[ctr] == lowest))
		{	//If this string plays the chord's lowest fret number
			break;
		}
	}
	for(; ctr < tp->numstrings; ctr++, bitmask <<= 1)
	{	//For each remaining string this track supports
		if((tp->note[note]->note & bitmask) && (tp->note[note]->frets[ctr] == 0))
		{	//If this string is played open
			return 0;	//This is not a barre chord
		}
		if((tp->note[note]->note & bitmask) && (tp->note[note]->frets[ctr] != lowest))
		{	//If this string is used and does not play the chord's lowest fret
			break;
		}
	}
	for(; ctr < tp->numstrings; ctr++, bitmask <<= 1)
	{	//For each remaining string this track supports
		if((tp->note[note]->note & bitmask) && (tp->note[note]->frets[ctr] == lowest))
		{	//If this string plays the chord's lowest fret number
			return 1;	//This is a barre chord
		}
	}

	return 0;	//This is not a barre chord
}

unsigned char eof_pro_guitar_note_is_double_stop(EOF_PRO_GUITAR_TRACK *tp, unsigned long note)
{
	unsigned long ctr, bitmask, frettedstringcount = 0;
	char contiguous = 0;

	if(!tp || (note >= tp->notes))
		return 0;	//Invalid parameters

	for(ctr = 0, bitmask = 1; ctr < tp->numstrings; ctr++, bitmask <<= 1)
	{	//For each string this track supports
		if(tp->note[note]->note & bitmask)
		{	//If this string is used
			contiguous = 0;	//Reset this status
			frettedstringcount++;
			if(tp->note[note]->note & (bitmask >> 1))
			{	//If the previous string was used
				contiguous = 1;
			}
		}
	}
	if((frettedstringcount != 2) || !contiguous)
	{	//If less than two strings are played non-open, or they were not contiguous strings
		return 0;	//This is not a double stop
	}

	return 1;	//This is a double stop
}

char eof_pro_guitar_note_is_open_chord(EOF_PRO_GUITAR_TRACK *tp, unsigned long note)
{
	unsigned long ctr, bitmask, opencount = 0, frettedcount = 0, highestfret = 0;

	if(!tp || (note >= tp->notes))
		return 0;	//Invalid parameters

	for(ctr = 0, bitmask = 1; ctr < tp->numstrings; ctr++, bitmask <<= 1)
	{	//For each string this track supports
		if((tp->note[note]->note & bitmask) && !(tp->note[note]->frets[ctr] & 0x80))
		{	//If the string is played and is not muted
			if(tp->note[note]->frets[ctr] != 0)
			{	//If this string is fretted
				frettedcount++;	//Track how many fretted strings are played
				if(tp->note[note]->frets[ctr] > highestfret)
					highestfret = tp->note[note]->frets[ctr];	//Track the highest used fret
			}
			else
			{	//The string is open
				opencount++;	//Track how many open strings are played
			}
		}
	}

	if(opencount && frettedcount && (highestfret < 4))
	{	//If there's at least one open string, at least one fretted string, and no strings fretted higher than fret 3
		return 1;	//This is an open chord
	}

	return 0;	//This is not an open chord
}

char eof_build_note_name(EOF_SONG *sp, unsigned long track, unsigned long note, char *buffer)
{
	char *name;
	int scale = 0, chord = 0, isslash = 0, bassnote = 0;
	unsigned long tracknum;

	if((sp == NULL) || !track || (track >= sp->tracks) || (buffer == NULL))
		return 0;	//Invalid parameters

	name = eof_get_note_name(sp, track, note);	//Check if the note was manually assigned a name
	if(name && (name[0] != '\0'))
	{	//If it has a name
		(void) ustrcpy(buffer, name);	//Copy the name
		return 1;
	}

	if(sp->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return 0;	//If this isn't a pro guitar/bass track, chord detection can't be used


	tracknum = sp->track[track]->tracknum;
	if(eof_lookup_chord(sp->pro_guitar_track[tracknum], track, note, &scale, &chord, &isslash, &bassnote, 0, 0))
	{	//If the chord lookup found a match
		if(!isslash)
		{	//If it's a normal chord
			(void) snprintf(buffer, EOF_NAME_LENGTH, "%s%s", eof_note_names[scale], eof_chord_names[chord].chordname);
		}
		else
		{	//If it's a slash chord
			(void) snprintf(buffer, EOF_NAME_LENGTH, "%s%s%s", eof_note_names[scale], eof_chord_names[chord].chordname, eof_slash_note_names[bassnote]);
		}
		buffer[EOF_NAME_LENGTH] = '\0';	//Ensure this buffer is truncated
		return 2;
	}

	return 0;	//Return no name found/detected
}

void eof_build_trill_phrases(EOF_PRO_GUITAR_TRACK *tp)
{
	unsigned long ctr, startpos, endpos, count;

	if(!tp)
		return;

	for(ctr = 0; ctr < tp->notes; ctr++)
	{	//For each note in the track
		if(!(tp->note[ctr]->flags & EOF_NOTE_FLAG_IS_TRILL))
			continue;	//If this note does not have trill status, skip it

		startpos = tp->note[ctr]->pos;	//Mark the start of this phrase
		endpos = startpos + tp->note[ctr]->length;	//Initialize the end position of the phrase
		while(++ctr < tp->notes)
		{	//For the consecutive remaining notes in the track
			if(tp->note[ctr]->flags & EOF_NOTE_FLAG_IS_TRILL)
			{	//And the next note is also marked as a trill
				endpos = tp->note[ctr]->pos + tp->note[ctr]->length;	//Update the end position of the phrase
			}
			else
			{
				break;	//Break from while loop.  This note isn't a trill so the next pass doesn't need to check it either
			}
		}
		if(endpos > startpos + 1)
		{	//As long as the phrase is determined to be longer than 1ms
			endpos--;	//Shorten the phrase so that a consecutive note isn't incorrectly included in the phrase just because it starts where the earlier note ended
		}
		count = tp->trills;
		if(count < EOF_MAX_PHRASES)
		{	//If the track can store the trill section
			tp->trill[count].start_pos = startpos;
			tp->trill[count].end_pos = endpos;
			tp->trill[count].flags = 0;
			tp->trill[count].name[0] = '\0';
			tp->trills++;
		}
	}
}

void eof_build_tremolo_phrases(EOF_PRO_GUITAR_TRACK *tp, unsigned char diff)
{
	unsigned long ctr, startpos, endpos, count;

	if(!tp)
		return;

	for(ctr = 0; ctr < tp->notes; ctr++)
	{	//For each note in the track
		if((diff != 0xFF) && (tp->note[ctr]->type != diff))
			continue;	//If this phrase doesn't apply to all difficulties or this note is not in the target difficulty, skip this note
		if(!(tp->note[ctr]->flags & EOF_NOTE_FLAG_IS_TREMOLO))
			continue;	//If this note is not marked as being in a tremolo, skip it

		startpos = tp->note[ctr]->pos;	//Mark the start of this phrase
		endpos = startpos + tp->note[ctr]->length;	//Initialize the end position of the phrase
		while(++ctr < tp->notes)
		{	//For the consecutive remaining notes in the track
			if(tp->note[ctr]->flags & EOF_NOTE_FLAG_IS_TREMOLO)
			{	//And the next note is also marked as a tremolo
				endpos = tp->note[ctr]->pos + tp->note[ctr]->length;	//Update the end position of the phrase
			}
			else
			{
				break;	//Break from while loop.  This note isn't a tremolo so the next pass doesn't need to check it either
			}
		}
		if(endpos > startpos + 1)
		{	//As long as the phrase is determined to be longer than 1ms
			endpos--;	//Shorten the phrase so that a consecutive note isn't incorrectly included in the phrase just because it starts where the earlier note ended
		}
		count = tp->tremolos;
		if(count < EOF_MAX_PHRASES)
		{	//If the track can store the tremolo section
			tp->tremolo[count].start_pos = startpos;
			tp->tremolo[count].end_pos = endpos;
			tp->tremolo[count].flags = 0;
			tp->tremolo[count].name[0] = '\0';
			tp->tremolo[count].difficulty = diff;	//Define the tremolo's difficulty as specified
			tp->tremolos++;
		}
	}//For each note in the track
}

int eof_note_convert_ghl_authoring(EOF_SONG *sp, unsigned long track, unsigned long note)
{
	EOF_LEGACY_TRACK *tp;
	int warning = 0;
	unsigned char blackgems, whitegems;

	if((sp == NULL) || !track || (track >= sp->tracks))
		return 0;
	if(!eof_track_is_legacy_guitar(sp, track))
		return 0;		//Do not allow this function to run on a non legacy guitar behavior track
	if(note >= eof_get_track_size(sp, track))
		return 0;

	tp = sp->legacy_track[sp->track[track]->tracknum];	//Simplify
	if(eof_track_is_ghl_mode(sp, track))
	{	//If the specified note's track is a GHL track, convert to GHL authoring
		if(tp->note[note]->note == 31)
		{	//If this note is a 5 lane chord
			if(eof_ghl_conversion_swaps_bw_gems)
			{	//If the user enabled the preference to swap black and white GHL gems during this conversion
				tp->note[note]->note = 4;	//Convert to a lane 3 gem (black 3)
			}
			else
			{
				tp->note[note]->note = 32;	//Otherwise convert to a lane 6 gem (white 3) instead
			}
		}
		else if(tp->note[note]->note == 32)
		{	//If this note is a lane 6 gem (Represents an open note in non GHL tracks)
			tp->note[note]->flags |= EOF_GUITAR_NOTE_FLAG_GHL_OPEN;	//Apply the GHL open note flag
		}
		else if(eof_ghl_conversion_swaps_bw_gems)
		{	//If the user enabled the preference to swap black and white GHL gems during this conversion
			blackgems = tp->note[note]->note & 7;			//Lanes 1-3 will be remapped to lanes 4-6 to reflect the white GHL gems
			whitegems = (tp->note[note]->note >> 3) & 3;	//Lanes 4-5 will be remapped to lanes 1-2 to reflect black GHL gems
			tp->note[note]->note &= ~63;					//Clear lanes 1-6
			tp->note[note]->note |= (blackgems << 3);		//Add the remapped gems from lanes 1-3
			tp->note[note]->note |= whitegems;				//Add the remapped gems from lanes 4-5
		}
	}
	else
	{	//Convert from GHL authoring
		if(tp->note[note]->flags & EOF_GUITAR_NOTE_FLAG_GHL_OPEN)
		{	//If this note has the GHL open note status
			tp->note[note]->flags &= ~EOF_GUITAR_NOTE_FLAG_GHL_OPEN;	//Remove that status
			tp->note[note]->note = 32;				//Convert to a lane 6 gem (open note)
		}
		else if((tp->note[note]->note & 32) && !eof_ghl_conversion_swaps_bw_gems)
		{	//If this note has a lane 6 (white 3) gem and it is be converted to a 5 lane chord as per the default logic
			if(tp->note[note]->note != 32)
			{	//If it also has a gem on any other lane
				tp->note[note]->flags |= EOF_NOTE_FLAG_HIGHLIGHT;	//Highlight the note
				warning = 1;
			}
			tp->note[note]->note = 31;	//Convert to a 5 lane chord
		}
		else if((tp->note[note]->note & 4) && eof_ghl_conversion_swaps_bw_gems)
		{	//If this note has a lane 3 (black 3) gem and it is to be converted to a 5 lane chord (user enabled the preference to swap black and white gems during GHL conversion)
			if(tp->note[note]->note != 4)
			{	//If it also has a gem on any other lane
				tp->note[note]->flags |= EOF_NOTE_FLAG_HIGHLIGHT;	//Highlight the note
				warning = 1;
			}
			tp->note[note]->note = 31;	//Otherwise convert to a 5 lane chord
		}
		else if(eof_ghl_conversion_swaps_bw_gems)
		{	//If the user enabled the preference to swap black and white GHL gems during this conversion
			blackgems = tp->note[note]->note & 3;			//Black gems 1-2 will be remapped to lanes 4-6
			whitegems = (tp->note[note]->note >> 3) & 7;	//White gems 1-3 will be remapped to lanes 1-3 to reflect black GHL gems
			tp->note[note]->note &= ~63;					//Clear lanes 1-6
			tp->note[note]->note |= (blackgems << 3);		//Add the remapped gems from lanes 1-2
			tp->note[note]->note |= whitegems;				//Add the remapped gems from lanes 3-5
		}
	}

	return warning;
}

int eof_legacy_guitar_note_is_open(EOF_SONG *sp, unsigned long track, unsigned long note)
{
	if((sp == NULL) || !track || (track >= sp->tracks))
		return 0;
	if(!eof_track_is_legacy_guitar(sp, track))
		return 0;		//Do not allow this function to run on a non legacy guitar behavior track
	if(note >= eof_get_track_size(sp, track))
		return 0;

	if(eof_track_is_ghl_mode(sp, track))
	{	//If the specified note's track is a GHL track
		if(eof_get_note_flags(sp, track, note) & EOF_GUITAR_NOTE_FLAG_GHL_OPEN)
			return 1;	//A note that has this flag is considered an open note
	}
	else
	{	//The specified note is in a non GHL track
		if(eof_get_note_note(sp, track, note) & 32)
			return 1;	//A note containing a gem on lane 6 is considered an open note/chord
	}

	return 0;
}
