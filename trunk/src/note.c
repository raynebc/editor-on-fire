#include <allegro.h>
#include "modules/ocd3d.h"
#include "main.h"
#include "note.h"

unsigned long eof_note_count_colors(EOF_SONG *sp, unsigned long track, unsigned long note)
{
	eof_log("eof_note_count_colors() entered");

	if(!sp)
	{
		return 0;
	}

	unsigned long count = 0;
	unsigned long notemask = eof_get_note_note(sp, track, note);

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
	return count;
}

void eof_legacy_track_note_create(EOF_NOTE * np, char g, char y, char r, char b, char p, char o, unsigned long pos, long length)
{
	eof_log("eof_legacy_track_note_create() entered");

	if(!np)
	{
		return;
	}
	np->note = 0;
	if(g)
	{
		np->note |= 1;
	}
	if(y)
	{
		np->note |= 2;
	}
	if(r)
	{
		np->note |= 4;
	}
	if(b)
	{
		np->note |= 8;
	}
	if(p)
	{
		np->note |= 16;
	}
	if(o)
	{
		np->note |= 32;
	}
	np->pos = pos;
	np->length = length;
}

void eof_legacy_track_note_create2(EOF_NOTE * np, unsigned long bitmask, unsigned long pos, long length)
{
	eof_log("eof_legacy_track_note_create2() entered");

	if(np)
	{
		np->note = bitmask;
		np->pos = pos;
		np->length = length;
	}
}

int eof_adjust_notes(int offset)
{
	eof_log("eof_adjust_notes() entered");

	unsigned long i, j;
	EOF_PHRASE_SECTION *soloptr = NULL;
	EOF_PHRASE_SECTION *starpowerptr= NULL;
	unsigned long tracknum;

	for(i = 1; i < eof_song->tracks; i++)
	{	//For each track
		for(j = 0; j < eof_get_track_size(eof_song, i); j++)
		{	//For each note in the track
			eof_set_note_pos(eof_song, i, j, eof_get_note_pos(eof_song, i, j) + offset);	//Add the offset to the note's position
		}
		for(j = 0; j < eof_get_num_solos(eof_song, i); j++)
		{	//For each solo section in the track
			soloptr = eof_get_solo(eof_song, i, j);
			soloptr->start_pos += offset;
			soloptr->end_pos += offset;
		}
		for(j = 0; j < eof_get_num_star_power_paths(eof_song, i); j++)
		{	//For each star power path in the track
			starpowerptr = eof_get_star_power_path(eof_song, i, j);
			starpowerptr->start_pos += offset;
			starpowerptr->end_pos += offset;
		}
		if(eof_song->track[i]->track_format == EOF_VOCAL_TRACK_FORMAT)
		{	//If this is a vocal track
			tracknum = eof_song->track[i]->tracknum;
			for(j = 0; j < eof_song->vocal_track[tracknum]->lines; j++)
			{	//For each lyric phrase in the track
				eof_song->vocal_track[tracknum]->line[j].start_pos += offset;
				eof_song->vocal_track[tracknum]->line[j].end_pos += offset;
			}
		}
	}
	for(i = 0; i < eof_song->catalog->entries; i++)
	{
		eof_song->catalog->entry[i].start_pos += offset;
		eof_song->catalog->entry[i].end_pos += offset;
	}
	for(i = 0; i < EOF_MAX_BOOKMARK_ENTRIES; i++)
	{
		if(eof_song->bookmark_pos[i] != 0)
		{
			eof_song->bookmark_pos[i] += offset;
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
	int pcol = p == 1 ? eof_color_white : p == 2 ? makecol(224, 255, 224) : 0;
	int dcol = eof_color_white;
	int tcol = eof_color_black;	//This color is used as the fret text color (for pro guitar notes)
	int dcol2 = dcol;
	int colors[EOF_MAX_FRETS] = {eof_color_green,eof_color_red,eof_color_yellow,eof_color_blue,eof_color_purple,eof_color_orange};	//Each of the fret colors
	int ncol = eof_color_silver;	//Note color defaults to silver unless the note is not star power
	unsigned long ctr;
	unsigned long mask;	//Used to mask out colors in the for loop
	int radius,dotsize;
	char iscymbal;		//Used to track whether the specified note is marked as a cymbal
	long x,y;
	unsigned long numlanes, tracknum=0, numlanes2;
	char notation[11];	//Used to store tab style notation for pro guitar notes
	char *nameptr = NULL;		//This points to the display name string for the note
	char *nameptrprev = NULL;	//This points to the display name string for the previous note (if applicable)
	char samename[] = "/";		//This is what a repeated note name will display as

	//These variables are used to store the common note data, regardless of whether the note is legacy or pro guitar format
	unsigned long notepos = 0;
	long notelength = 0;
	unsigned long noteflags = 0;
	unsigned long notenote = 0;
	char notetype = 0;


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
	}
	else
	{	//Render the pen note
		notepos = eof_pen_note.pos;
		notelength = eof_pen_note.length;
		noteflags = eof_pen_note.flags;
		notenote = eof_pen_note.note;
		notetype = eof_pen_note.type;
	}

	if(window == eof_window_note)
	{	//If rendering to the fret catalog
		position = eof_music_catalog_pos;
		leftcoord = 140;
	}
	else
	{	//Otherwise assume it's being rendered to the editor window
		position = eof_music_pos;
		leftcoord = 300;
	}
	pos = position / eof_zoom;
	if(pos < leftcoord)
	{	//Scroll the left edge of the piano roll based on the roll's position
		npos = 20 + (notepos) / eof_zoom;
	}
	else
	{
		npos = 20 - ((pos - leftcoord)) + notepos / eof_zoom;
	}
	x = npos;	//Store this to make the code more readable

//Determine if the entire note would clip.  If so, return without attempting to render
	if(npos - eof_screen_layout.note_size > window->screen->w)	//If the note would render entirely to the right of the visible area
		return 1;	//Return status:  Clipping to the right of the viewing window

	if((npos < 0) && (npos + notelength / eof_zoom < 0))	//If the note and its tail would render entirely to the left of the visible area
		return -1;	//Return status:  Clipping to the left of the viewing window

	if(noteflags & EOF_NOTE_FLAG_CRAZY)
	{
		dcol = eof_color_black;	//"Crazy" notes render with a black dot in the center
		tcol = eof_color_white;	//In which case, render the fret number (pro guitar) with the opposite color
	}

//Since Expert+ double bass notation uses the same flag as crazy status, override the dot color for PART DRUMS
	if(eof_selected_track == EOF_TRACK_DRUM)
		dcol = eof_color_white;

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
	{
		numlanes = eof_count_track_lanes(eof_song, eof_selected_track);	//Count the number of lanes in the active track
	}
//	eof_set_2D_lane_positions(track);	//Update the ychart[] array (this call should be unnecessary since eof_render_editor_window_common() and eof_render_note_window() update the ychart[] array)
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

	vline(window->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[2] - eof_screen_layout.note_marker_size, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[2] + eof_screen_layout.note_marker_size, makecol(128, 128, 128));
	if(p == 3)
	{
		pcol = eof_color_white;
		dcol = eof_color_white;
	}

	for(ctr=0,mask=1;ctr<numlanes;ctr++,mask=mask<<1)
	{	//Render for each of the available fret lanes
		iscymbal = 0;
		y = EOF_EDITOR_RENDER_OFFSET + 15 + ychart[ctr];	//Store this to make the code more readable

		if(notenote & mask)
		{	//If this lane is populated
			if(!(noteflags & EOF_NOTE_FLAG_SP))
			{	//If the note is not star power
				ncol = colors[ctr];	//Assign the appropriate fret color
			}

			if((eof_selected_track == EOF_TRACK_DRUM))
			{	//Drum track specific dot color logic
				if((notetype == EOF_NOTE_AMAZING) && (noteflags & EOF_NOTE_FLAG_DBASS) && (mask == 1))
					dcol2 = eof_color_red;	//If this is an Expert+ bass drum note, render it with a red dot
				else if(((noteflags & EOF_NOTE_FLAG_Y_CYMBAL) && (mask == 4)) || ((noteflags & EOF_NOTE_FLAG_B_CYMBAL) && (mask == 8)) || ((noteflags & EOF_NOTE_FLAG_G_CYMBAL) && (mask == 16)))
				{	//If this drum note is marked as a yellow, blue or green cymbal
					iscymbal = 1;
				}
				else
					dcol2 = dcol;			//Otherwise render with the expected dot color
			}
			else
				dcol2 = dcol;			//Otherwise render with the expected dot color

			rectfill(window->screen, x, y - eof_screen_layout.note_tail_size, x + notelength / eof_zoom, y + eof_screen_layout.note_tail_size, ncol);	//Draw the note tail
			if(p)
			{	//If this note is moused over
				rect(window->screen, x, y - eof_screen_layout.note_tail_size, x + notelength / eof_zoom, y + eof_screen_layout.note_tail_size, pcol);	//Draw a border around the rectangle
			}

			//Render pro guitar note slide if applicable
			if((track != 0) && (eof_song->track[track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT) && ((noteflags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP) || (noteflags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN)))
			{	//If rendering an existing pro guitar note that slides up or down
				long x2;			//Used for slide note rendering
				unsigned long notepos2;		//Used for slide note rendering
				int sliderect[8];		//An array of 4 vertices, used to draw a diagonal rectangle

				notepos2 = notepos + notelength;	//Find the position of the end of the note
				if(pos < leftcoord)
				{
					x2 = 20 + (notepos2) / eof_zoom;
				}
				else
				{
					x2 = 20 - ((pos - leftcoord)) + notepos2 / eof_zoom;
				}

				//Define the slide rectangle coordinates in clockwise order
				#define EOF_PRO_GUITAR_SLIDE_LINE_THICKNESS 1
				sliderect[0] = x;	//X1 (X coordinate of the left end of the slide)
				if(noteflags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP)
				{	//If this note slides up, start the slide line at the bottom of this note
					sliderect[1] = y + radius - EOF_PRO_GUITAR_SLIDE_LINE_THICKNESS;	//Y1 (Y coordinate of the left end of the slide)
				}
				else
				{	//Otherwise start the slide line at the top of this note
					sliderect[1] = y - radius - EOF_PRO_GUITAR_SLIDE_LINE_THICKNESS;	//Y1 (Y coordinate of the left end of the slide)
				}
				sliderect[2] = x2;	//X2 (X coordinate of the right end of the slide)
				if(noteflags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP)
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
					polygon(window->screen, 4, sliderect, makecol(128, 0, 128));		//Render the 4 point polygon in purple
				}
			}//If rendering an existing pro guitar track that slides up or down

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
					//Render the fret number over the center of the note (but only if the active track is a pro guitar track)
					BITMAP *fretbmp = eof_create_fret_number_bitmap(eof_song->pro_guitar_track[tracknum]->note[notenum], ctr, 2, tcol, dcol);	//Allow 2 pixels for padding
					if(fretbmp != NULL)
					{	//Render the bitmap on top of the 3D note and then destroy the bitmap
						draw_sprite(window->screen, fretbmp, x - (fretbmp->w/2), y - (text_height(font)/2));	//Fudge (x,y) to make it print centered over the gem
						destroy_bitmap(fretbmp);
					}
				}
			}
			else
			{	//Otherwise render it as a triangle
				triangle(window->screen, x, y-radius, x+radius, y+radius, x-radius, y+radius, ncol);
				if(p)
				{	//Draw a non filled rectangle along the border of the filled triangle
					line(window->screen, x, y-radius, x+radius, y+radius, pcol);
					line(window->screen, x+radius, y+radius, x-radius, y+radius, pcol);
					line(window->screen, x-radius, y+radius, x, y-radius, pcol);
				}
			}
		}//If this lane is populated
		else if((eof_hover_note >= 0) && (p == 3))
		{
			rect(window->screen, x, y - eof_screen_layout.note_tail_size, x + notelength / eof_zoom, y + eof_screen_layout.note_tail_size, eof_color_gray);
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

	if(track != 0)
	{	//If rendering an existing note instead of the pen note
		//Render tab notations
		eof_get_note_tab_notation(notation, track, notenum);	//Get the tab playing notation for this note
		textprintf_centre_ex(window->screen, eof_mono_font, x, EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 3, eof_color_red, -1, notation);

		//Render note names
		nameptr = eof_get_note_name(eof_song, track, notenum);
		if((nameptr != NULL) && (nameptr[0] != '\0'))
		{	//If this note has a defined name
			nameptrprev = eof_get_note_name(eof_song, track, eof_get_prev_note_type_num(eof_song, track, notenum));	//Get the previous note's (in the same difficulty's) name
			if(nameptrprev && (!ustricmp(nameptr, nameptrprev)))
			{	//If there was a previous note, and it has the same name as this note's name
				nameptr = samename;	//Display this note's name as "/" to indicate a repeat of the last note
			}
			if(window == eof_window_editor)
			{	//If rendering to the editor window
				textprintf_centre_ex(window->screen, font, x, 25 + 5, eof_color_white, -1, nameptr);
			}
			else
			{	//If rendering to the note window
				textprintf_centre_ex(window->screen, font, x, EOF_EDITOR_RENDER_OFFSET + 10, eof_color_white, -1, nameptr);
			}
		}
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
	int pcol = p == 1 ? eof_color_white : p == 2 ? makecol(224, 255, 224) : 0;
	int dcol = eof_color_white;
	int ncol = 0;
	EOF_PHRASE_SECTION *lyricline;	//The line that this lyric is found to be in (if any) so the correct background color can be determined
	int bgcol = eof_color_black;	//Unless the text is found to be in a lyric phrase, it will render with a black background
	long endpos;		//This will store the effective end position for the lyric's rendering (taking lyric text, note rectangles and vocal slides into account)

//Validate parameters
	if((np == NULL) || (window == NULL))	//If this is not a valid lyric or window pointer
		return 1;			//Stop rendering

	if(window == eof_window_note)
	{	//If rendering to the fret catalog
		position = eof_music_catalog_pos;
		leftcoord = 140;
	}
	else
	{	//Otherwise assume it's being rendered to the editor window
		position = eof_music_pos;
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
				bgcol=makecol(0, 0, 127);	//Make dark blue the text's background colo
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
		dcol = eof_color_white;
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
			rectfill(window->screen, npos, note_y, npos + np->length / eof_zoom, note_y + eof_screen_layout.vocal_tail_size - 1, ncol);

			int sliderect[8] = {0};	//An array of 4 vertices, used to draw a diagonal rectangle
			EOF_LYRIC *np2=NULL;	//Stores a pointer to the next lyric
			long nplace2 = 0;
			long native2 = 0;
			long note_y2 = 0;	//Used to store the y coordinate of the next lyric
			long npos2 = 0;		//Stores the X coordinate of the next lyric

			if(notpen && (notenum + 1 < eof_song->vocal_track[0]->lyrics) && (eof_song->vocal_track[0]->lyric[notenum + 1]->text[0] == '+'))
			{	//If there's another lyric, and it begins with a plus sign, it's a pitch shift, draw a vocal slide polygon
				np2=eof_song->vocal_track[0]->lyric[notenum+1];
				sliderect[0]=npos + np->length / eof_zoom;	//X1 (X coordinate of the end of this lyric's rectangle)
				sliderect[1]=note_y;						//Y1 (Y coordinate of the bottom of this lyric's rectangle)

				if(pos < 300)
				{
					npos2 = 20 + (np2->pos) / eof_zoom;
				}
				else
				{
					npos2 = 20 - ((pos - 300)) + np2->pos / eof_zoom;
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
					polygon(window->screen, 4, sliderect, makecol(128, 0, 128));	//Render the 4 point polygon in purple
				}
			}
		}
		else
		{	//Render a vocal percussion note as a fret note in the middle lane
			circlefill(window->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[2] + eof_screen_layout.string_space / 2, eof_screen_layout.note_size, eof_color_white);
			circlefill(window->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[2] + eof_screen_layout.string_space / 2, eof_screen_layout.note_size-2, eof_color_black);
		}

		if(p)
		{
			if(np->note == EOF_LYRIC_PERCUSSION)
			{	//Render a vocal percussion note as a fret note in the middle lane
				circlefill(window->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[2] + eof_screen_layout.string_space / 2, eof_screen_layout.note_size, eof_color_white);
				circlefill(window->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[2] + eof_screen_layout.string_space / 2, eof_screen_layout.note_size-2, eof_color_black);
			}
			else
			{	//Render a regular vocal note
				rect(window->screen, npos, note_y, npos + np->length / eof_zoom, note_y + eof_screen_layout.vocal_tail_size - 1, pcol);
			}
		}
	}//If this lyric is not pitchless/freestyle
	else	//If the lyric is pitchless or freestyle, render with gray
		rectfill(window->screen, npos, note_y, npos + np->length / eof_zoom, note_y + eof_screen_layout.vocal_tail_size - 1, makecol(128, 128, 128));

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
	unsigned int notes[EOF_MAX_FRETS] = {EOF_IMAGE_NOTE_GREEN, EOF_IMAGE_NOTE_RED, EOF_IMAGE_NOTE_YELLOW, EOF_IMAGE_NOTE_BLUE, EOF_IMAGE_NOTE_PURPLE, EOF_IMAGE_NOTE_ORANGE};
	unsigned int notes_hit[EOF_MAX_FRETS] = {EOF_IMAGE_NOTE_GREEN_HIT, EOF_IMAGE_NOTE_RED_HIT, EOF_IMAGE_NOTE_YELLOW_HIT, EOF_IMAGE_NOTE_BLUE_HIT, EOF_IMAGE_NOTE_PURPLE_HIT, EOF_IMAGE_NOTE_ORANGE_HIT};
	unsigned int hopo_notes[EOF_MAX_FRETS] = {EOF_IMAGE_NOTE_HGREEN, EOF_IMAGE_NOTE_HRED, EOF_IMAGE_NOTE_HYELLOW, EOF_IMAGE_NOTE_HBLUE, EOF_IMAGE_NOTE_HPURPLE, EOF_IMAGE_NOTE_HORANGE};
	unsigned int hopo_notes_hit[EOF_MAX_FRETS] = {EOF_IMAGE_NOTE_HGREEN_HIT, EOF_IMAGE_NOTE_HRED_HIT, EOF_IMAGE_NOTE_HYELLOW_HIT, EOF_IMAGE_NOTE_HBLUE_HIT, EOF_IMAGE_NOTE_HPURPLE_HIT, EOF_IMAGE_NOTE_HORANGE_HIT};
	unsigned int cymbals[EOF_MAX_FRETS] = {EOF_IMAGE_NOTE_GREEN, EOF_IMAGE_NOTE_RED, EOF_IMAGE_NOTE_YELLOW_CYMBAL, EOF_IMAGE_NOTE_BLUE_CYMBAL, EOF_IMAGE_NOTE_PURPLE_CYMBAL, EOF_IMAGE_NOTE_ORANGE};
	unsigned int cymbals_hit[EOF_MAX_FRETS] = {EOF_IMAGE_NOTE_GREEN_HIT, EOF_IMAGE_NOTE_RED_HIT, EOF_IMAGE_NOTE_YELLOW_CYMBAL_HIT, EOF_IMAGE_NOTE_BLUE_CYMBAL_HIT, EOF_IMAGE_NOTE_PURPLE_CYMBAL_HIT, EOF_IMAGE_NOTE_ORANGE_HIT};
	unsigned int arrows[EOF_MAX_FRETS] = {EOF_IMAGE_NOTE_GREEN_ARROW, EOF_IMAGE_NOTE_RED_ARROW, EOF_IMAGE_NOTE_YELLOW_ARROW, EOF_IMAGE_NOTE_BLUE_ARROW, EOF_IMAGE_NOTE_PURPLE, EOF_IMAGE_NOTE_ORANGE};
	unsigned int arrows_hit[EOF_MAX_FRETS] = {EOF_IMAGE_NOTE_GREEN_ARROW_HIT, EOF_IMAGE_NOTE_RED_ARROW_HIT, EOF_IMAGE_NOTE_YELLOW_ARROW_HIT, EOF_IMAGE_NOTE_BLUE_ARROW_HIT, EOF_IMAGE_NOTE_PURPLE_HIT, EOF_IMAGE_NOTE_ORANGE_HIT};
	unsigned long numlanes, tracknum;

	//These variables are used for the name rendering logic
	char *nameptr = NULL;		//This points to the display name string for the note
	char *nameptrprev = NULL;	//This points to the display name string for the previous note (if applicable)
	char samename[] = "/";		//This is what a repeated note name will display as
	long x3d, y3d, z3d;			//The coordinate at which to draw the name string (right aligned)

	//These variables are used to store the common note data, regardless of whether the note is legacy or pro guitar format
	unsigned long notepos = 0;
	long notelength = 0;
	unsigned long noteflags = 0;
	unsigned long notenote = 0;
	char notetype = 0;

//Validate parameters
	tracknum = eof_song->track[track]->tracknum;
	if((track == 0) || (track >= eof_song->tracks) || ((eof_song->track[track]->track_format != EOF_LEGACY_TRACK_FORMAT) && (eof_song->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)) || (notenum >= eof_get_track_size(eof_song, track)))
	{	//If an invalid track or note number was passsed
		return -1;	//Error, signal to stop rendering (3D window renders last note to first)
	}
	notepos = eof_get_note_pos(eof_song, track, notenum);
	notelength = eof_get_note_length(eof_song, track, notenum);
	noteflags = eof_get_note_flags(eof_song, track, notenum);
	notenote = eof_get_note_note(eof_song, track, notenum);
	notetype = eof_get_note_type(eof_song, track, notenum);

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

	npos = (long)(notepos + eof_av_delay - eof_music_pos) / eof_zoom_3d  - 6;
	if(npos + notelength / eof_zoom_3d < -100)
	{				//If the note would render entirely before the visible area
		return -1;	//Return status:  Clipping before the viewing window
	}
	else if(npos > 600)
	{				//If the note would render entirely after the visible area
		return 1;	//Return status:  Clipping after the viewing window
	}

	numlanes = eof_count_track_lanes(eof_song, track);	//Count the number of lanes in that note's track
	if(eof_selected_track == EOF_TRACK_BASS)
	{	//Special case:  The bass track can use a sixth lane but its 3D representation still only draws 5 lanes
		numlanes = 5;
	}

//	eof_set_3D_lane_positions(track);	//Update the xchart[] array (should be unnecessary since eof_render_3d_window() updates the xchart[] array)
	if(eof_song->track[track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
	{	//If this is a drum track
		if(notenote & 1)
		{
			rz = npos;
			ez = npos + 14;
			point[0] = ocd3d_project_x(bx - 10, rz);
			point[1] = ocd3d_project_y(200, rz);
			point[2] = ocd3d_project_x(bx - 10, ez);
			point[3] = ocd3d_project_y(200, ez);
			point[4] = ocd3d_project_x(bx + 232, ez);
			point[5] = ocd3d_project_y(200, ez);
			point[6] = ocd3d_project_x(bx + 232, rz);
			point[7] = ocd3d_project_y(200, rz);

			if(noteflags & EOF_NOTE_FLAG_SP)			//If this bass drum note is star power, render it in silver
				polygon(eof_window_3d->screen, 4, point, p ? eof_color_white : eof_color_silver);
			else if(noteflags & EOF_NOTE_FLAG_DBASS)	//Or if it is double bass, render it in red
				polygon(eof_window_3d->screen, 4, point, p ? makecol(255, 192, 192) : eof_color_red);
			else										//Otherwise render it in green
				polygon(eof_window_3d->screen, 4, point, p ? makecol(192, 255, 192) : eof_color_green);
		}
		for(ctr=1,mask=2;ctr<numlanes;ctr++,mask=mask<<1)
		{	//Render for each of the available fret colors after 1 (bass drum)
			if(notenote & mask)
			{
				if(((noteflags & EOF_NOTE_FLAG_Y_CYMBAL) && (mask == 4)) || ((noteflags & EOF_NOTE_FLAG_B_CYMBAL) && (mask == 8)) || ((noteflags & EOF_NOTE_FLAG_G_CYMBAL) && (mask == 16)))
				{	//If this is a cymbal note, render with the cymbal image
					if(noteflags & EOF_NOTE_FLAG_SP)
					{	//If this cymbal note is star power, render it in silver
						ocd3d_draw_bitmap(eof_window_3d->screen, p ? eof_image[EOF_IMAGE_NOTE_WHITE_CYMBAL_HIT] : eof_image[EOF_IMAGE_NOTE_WHITE_CYMBAL], xchart[ctr-1] - 24 + 28, 200 - 48, npos);
					}
					else
					{	//Otherwise render in the appropriate color
						ocd3d_draw_bitmap(eof_window_3d->screen, p ? eof_image[cymbals_hit[ctr]] : eof_image[cymbals[ctr]], xchart[ctr-1] - 24 + 28, 200 - 48, npos);
					}
				}
				else
				{	//Otherwise render with the standard note image
					if(noteflags & EOF_NOTE_FLAG_SP)
					{	//If this drum note is star power, render it in silver
						ocd3d_draw_bitmap(eof_window_3d->screen, p ? eof_image[EOF_IMAGE_NOTE_WHITE_HIT] : eof_image[EOF_IMAGE_NOTE_WHITE], xchart[ctr-1] - 24 + 28, 200 - 48, npos);
					}
					else
					{	//Otherwise render in the appropriate color
						ocd3d_draw_bitmap(eof_window_3d->screen, p ? eof_image[notes_hit[ctr]] : eof_image[notes[ctr]], xchart[ctr-1] - 24 + 28, 200 - 48, npos);
					}
				}
			}
		}
	}//If this is a drum track
	else
	{	//This is a non drum track
		for(ctr=0,mask=1;ctr<eof_count_track_lanes(eof_song, track);ctr++,mask=mask<<1)
		{	//Render for each of the available fret lanes (count the active track's lanes to work around numlanes not being equal to 6 for open bass)
			if(notenote & mask)
			{	//If this lane is used
				if((mask == 32) && (track == EOF_TRACK_BASS) && eof_open_bass_enabled())
				{	//Lane 6 for the bass track (if enabled) renders similarly to a bass drum note
					rz = npos;
					ez = npos + 14;
					point[0] = ocd3d_project_x(bx - 10, rz);
					point[1] = ocd3d_project_y(200, rz);
					point[2] = ocd3d_project_x(bx - 10, ez);
					point[3] = ocd3d_project_y(200, ez);
					point[4] = ocd3d_project_x(bx + 232, ez);
					point[5] = ocd3d_project_y(200, ez);
					point[6] = ocd3d_project_x(bx + 232, rz);
					point[7] = ocd3d_project_y(200, rz);

					if(noteflags & EOF_NOTE_FLAG_SP)			//If this open bass note is star power, render it in silver
						polygon(eof_window_3d->screen, 4, point, p ? eof_color_white : eof_color_silver);
					else										//Otherwise render it in orange
						polygon(eof_window_3d->screen, 4, point, p ? makecol(255, 192, 0) : eof_color_orange);
				}
				else
				{
					if(track != EOF_TRACK_DANCE)
					{	//If this is not a dance track
						if(noteflags & EOF_NOTE_FLAG_HOPO)
						{	//If this is a HOPO note
							if(noteflags & EOF_NOTE_FLAG_SP)
							{	//If this is also a SP note
								ocd3d_draw_bitmap(eof_window_3d->screen, p ? eof_image[EOF_IMAGE_NOTE_HWHITE_HIT] : eof_image[EOF_IMAGE_NOTE_HWHITE], xchart[ctr] - 24, 200 - 48, npos);
							}
							else
							{
								ocd3d_draw_bitmap(eof_window_3d->screen, p ? eof_image[hopo_notes_hit[ctr]] : eof_image[hopo_notes[ctr]], xchart[ctr] - 24, 200 - 48, npos);
							}
						}
						else
						{
							if(noteflags & EOF_NOTE_FLAG_SP)
							{	//If this is an SP note
								ocd3d_draw_bitmap(eof_window_3d->screen, p ? eof_image[EOF_IMAGE_NOTE_WHITE_HIT] : eof_image[EOF_IMAGE_NOTE_WHITE], xchart[ctr] - 24, 200 - 48, npos);
							}
							else
							{
								ocd3d_draw_bitmap(eof_window_3d->screen, p ? eof_image[notes_hit[ctr]] : eof_image[notes[ctr]], xchart[ctr] - 24, 200 - 48, npos);
							}
						}

						if(!eof_legacy_view && (notenote & mask) && (eof_song->track[track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
						{	//If legacy view is disabled and this is a pro guitar note, render the fret number over the center of the note
							BITMAP *fretbmp = eof_create_fret_number_bitmap(eof_song->pro_guitar_track[tracknum]->note[notenum], ctr, 8, eof_color_white, eof_color_black);	//Allow one extra character's width for padding
							if(fretbmp != NULL)
							{	//Render the bitmap on top of the 3D note and then destroy the bitmap
								ocd3d_draw_bitmap(eof_window_3d->screen, fretbmp, xchart[ctr] - 8, 200 - 24, npos);
								destroy_bitmap(fretbmp);
							}
						}
					}//If this is not a dance track
					else
					{	//This is a dance track
						ocd3d_draw_bitmap(eof_window_3d->screen, p ? eof_image[arrows_hit[ctr]] : eof_image[arrows[ctr]], xchart[ctr] - 24, 200 - 48, npos);
					}
				}
			}//If this lane is used
		}//Render for each of the available fret lanes
	}//This is a non drum track

	//Render note names
	nameptr = eof_get_note_name(eof_song, track, notenum);
	if((nameptr != NULL) && (nameptr[0] != '\0'))
	{	//If this note has a defined name
		nameptrprev = eof_get_note_name(eof_song, track, eof_get_prev_note_type_num(eof_song, track, notenum));	//Get the previous note's (in the same difficulty's) name
		if(nameptrprev && (!ustricmp(nameptr, nameptrprev)))
		{	//If there was a previous note, and it has the same name as this note's name
			nameptr = samename;	//Display this note's name as "/" to indicate a repeat of the last note
		}
		z3d = npos + 6 + text_height(font);	//Restore the 6 that was subtracted earlier when finding npos, and add the font's height to have the text line up with the note's z position
		z3d = z3d < -100 ? -100 : z3d;
		x3d = ocd3d_project_x(20 - 4, z3d);
		y3d = ocd3d_project_y(200, z3d);
		textprintf_right_ex(eof_window_3d->screen, font, x3d, y3d, eof_color_white, -1, nameptr);
	}

	return 0;	//Return status:  Note was not clipped in its entirety
}

int eof_note_tail_draw_3d(unsigned long track, unsigned long notenum, int p)
{
	long npos;
	int point[8];
	int rz, ez;
	unsigned long numlanes, ctr, mask, tracknum;
	int colortable[EOF_MAX_FRETS][2] = {{makecol(192, 255, 192), eof_color_green}, {makecol(255, 192, 192), eof_color_red}, {makecol(255, 255, 192), eof_color_yellow}, {makecol(192, 192, 255), eof_color_blue}, {makecol(255, 192, 255), eof_color_purple}, {makecol(255, 192, 0), eof_color_orange}};

	//These variables are used to store the common note data, regardless of whether the note is legacy or pro guitar format
	unsigned long notepos = 0;
	long notelength = 0;
	unsigned long noteflags = 0;
	unsigned long notenote = 0;
	char notetype = 0;

//Validate parameters
	if((track == 0) || (track >= eof_song->tracks) || ((eof_song->track[track]->track_format != EOF_LEGACY_TRACK_FORMAT) && (eof_song->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)) || (notenum >= eof_get_track_size(eof_song, track)))
	{	//If an invalid track or note number was passsed
		return -1;	//Error, signal to stop rendering (3D window renders last note to first)
	}
	notepos = eof_get_note_pos(eof_song, track, notenum);
	notelength = eof_get_note_length(eof_song, track, notenum);
	noteflags = eof_get_note_flags(eof_song, track, notenum);
	notenote = eof_get_note_note(eof_song, track, notenum);
	notetype = eof_get_note_type(eof_song, track, notenum);

	if((eof_selected_track == EOF_TRACK_DRUM) || (notelength <= 10))
		return 0;	//Don't render tails for drum notes or notes that aren't over 10ms long

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

	npos = (long)(notepos + eof_av_delay - eof_music_pos) / eof_zoom_3d  - 6;
	if(npos + notelength / eof_zoom_3d < -100)
	{
		return -1;
	}
	else if(npos > 600)
	{
		return 1;
	}

	//Determine the width of the fret lanes
	numlanes = eof_count_track_lanes(eof_song, track);	//Count the number of lanes in that note's track
	if(eof_selected_track == EOF_TRACK_BASS)
	{	//Special case:  The bass track can use a sixth lane but its 3D representation still only draws 5 lanes
		numlanes = 5;
	}
//	eof_set_3D_lane_positions(track);	//Update the xchart[] array (should be unnecessary since eof_render_3d_window() updates the xchart[] array)
	rz = npos < -100 ? -100 : npos + 10;
	ez = npos + notelength / eof_zoom_3d > 600 ? 600 : npos + notelength / eof_zoom_3d + 6;
	for(ctr=0,mask=1; ctr < eof_count_track_lanes(eof_song, track); ctr++,mask=mask<<1)
	{	//For each of the lanes in this track
		if(notenote & mask)
		{	//If this lane has a gem to render
			if((ctr == 5) && (track == EOF_TRACK_BASS) && eof_open_bass_enabled())
			{	//Logic to render open bass strum notes (a rectangle covering the width of rendering of frets 2, 3 and 4
				point[0] = ocd3d_project_x(xchart[1] - 10, rz);
				point[1] = ocd3d_project_y(200, rz);
				point[2] = ocd3d_project_x(xchart[1] - 10, ez);
				point[3] = ocd3d_project_y(200, ez);
				point[4] = ocd3d_project_x(xchart[3] + 10, ez);
				point[5] = ocd3d_project_y(200, ez);
				point[6] = ocd3d_project_x(xchart[3] + 10, rz);
				point[7] = ocd3d_project_y(200, rz);
				polygon(eof_window_3d->screen, 4, point, noteflags & EOF_NOTE_FLAG_SP ? (p ? eof_color_white : eof_color_silver) : (p ? colortable[ctr][0] : colortable[ctr][1]));
			}
			else
			{	//Logic to render lanes 1 through 6
				if(!((ctr == 5) && (track == EOF_TRACK_BASS)))
				{	//If this is not a hidden open bass note
					point[0] = ocd3d_project_x(xchart[ctr] - 10, rz);
					point[1] = ocd3d_project_y(200, rz);
					point[2] = ocd3d_project_x(xchart[ctr] - 10, ez);
					point[3] = ocd3d_project_y(200, ez);
					point[4] = ocd3d_project_x(xchart[ctr] + 10, ez);
					point[5] = ocd3d_project_y(200, ez);
					point[6] = ocd3d_project_x(xchart[ctr] + 10, rz);
					point[7] = ocd3d_project_y(200, rz);
					polygon(eof_window_3d->screen, 4, point, noteflags & EOF_NOTE_FLAG_SP ? (p ? eof_color_white : eof_color_silver) : (p ? colortable[ctr][0] : colortable[ctr][1]));
				}
			}

			//Render pro guitar note slide if applicable
			if((track != 0) && (eof_song->track[track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT) && ((noteflags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP) || (noteflags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN)))
			{	//If rendering an existing pro guitar track that slides up or down
				long npos2, rz2;
				unsigned long notepos2;		//Used for slide note rendering
				unsigned long halflanewidth = (56.0 * (4.0 / (numlanes-1))) / 2;

				notepos2 = notepos + notelength;	//Find the position of the end of the note
				npos2 = (long)(notepos2 + eof_av_delay - eof_music_pos) / eof_zoom_3d  - 6;
				rz2 = npos2 < -100 ? -100 : npos2 + 10;

				//Define the slide rectangle coordinates in clockwise order
				#define EOF_PRO_GUITAR_SLIDE_LINE_THICKNESS_3D 4
				if(noteflags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP)
				{	//If this note slides up (3D view from left to right), start the slide line at the left of this note
					point[0] = ocd3d_project_x(xchart[ctr] - halflanewidth, rz);	//X1 (X coordinate of the front end of the slide)
				}
				else
				{	//Otherwise start the slide line at the right of this note
					point[0] = ocd3d_project_x(xchart[ctr] + halflanewidth, rz);	//X1 (X coordinate of the front end of the slide)
				}
				point[1] = ocd3d_project_y(200, rz);	//Y1 (Y coordinate of the front end of the slide)
				if(noteflags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP)
				{	//If this note slides up (3D view from left to right), end the slide line at the right of the next note
					point[2] = ocd3d_project_x(xchart[ctr] + halflanewidth, rz2);	//X2 (X coordinate of the back end of the slide)
				}
				else
				{	//Otherwise end the slide line at the left of the next note
					point[2] = ocd3d_project_x(xchart[ctr] - halflanewidth, rz2);	//X2 (X coordinate of the back end of the slide)
				}
				point[3] = ocd3d_project_y(200, rz2);	//Y2 (Y coordinate of the back end of the slide
				point[4] = point[2] + (2 * EOF_PRO_GUITAR_SLIDE_LINE_THICKNESS_3D);	//X3 (the specified number of pixels right of X2)
				point[5] = point[3];	//Y3 (Y coordinate of the back end of the slide)
				point[6] = point[0] + (2 * EOF_PRO_GUITAR_SLIDE_LINE_THICKNESS_3D);	//X4 (the specified number of pixels right of X1)
				point[7] = point[1];	//Y4 (Y coordinate of the front end of the slide)
				polygon(eof_window_3d->screen, 4, point, makecol(128, 0, 128));	//Render the 4 point polygon in purple
			}//If rendering an existing pro guitar track that slides up or down
		}//If this lane has a gem to render
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

BITMAP *eof_create_fret_number_bitmap(EOF_PRO_GUITAR_NOTE *note, unsigned char stringnum, unsigned long padding, int textcol, int fillcol)
{
	BITMAP *fretbmp = NULL;
	int height, width;
	char fretstring[10] = {0};

	if(note != NULL)
	{
		if(note->frets[stringnum] == 0xFF)
		{	//This is a muted fret
			snprintf(fretstring,sizeof(fretstring),"X");
		}
		else
		{	//This is a non muted fret
			if(note->ghost & (1 << stringnum))
			{	//This is a ghosted note
				snprintf(fretstring,sizeof(fretstring),"(%d)",note->frets[stringnum]);
			}
			else
			{	//This is a normal note
				snprintf(fretstring,sizeof(fretstring),"%d",note->frets[stringnum]);
			}
		}

		width = text_length(font,fretstring) + padding + 1;	//The font in use doesn't look centered, so pad the left by one pixel
		height = text_height(font);
		fretbmp = create_bitmap(width,height);
		if(fretbmp != NULL)
		{	//Render the fret number on top of the 3D note
			clear_to_color(fretbmp, fillcol);
			rect(fretbmp, 0, 0, width-1, height-1, textcol);	//Draw a border along the edge of this bitmap
			textprintf_ex(fretbmp, font, (padding/2.0) + 1, 0, textcol, -1, "%s", fretstring);	//Center the text between the padding (including one extra pixel for left padding), rounding to the right if the padding is an odd value
		}
	}

	return fretbmp;
}

void eof_get_note_tab_notation(char *buffer, unsigned long track, unsigned long note)
{
	unsigned long index = 0, tracknum, flags = 0, prevnoteflags = 0;
	long prevnotenum;

	if((track >= eof_song->tracks) || (buffer == NULL) || ((eof_song->track[track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT) && (eof_song->track[track]->track_format != EOF_LEGACY_TRACK_FORMAT)))
	{
		return;	//If this is an invalid track number, the buffer is NULL or the specified track isn't a pro guitar or legacy track, return
	}
	tracknum = eof_song->track[track]->tracknum;
	if(note >= eof_get_track_size(eof_song, track))
	{
		return;
	}
	flags = eof_get_note_flags(eof_song, track, note);

	prevnotenum = eof_get_prev_note_type_num(eof_song, track, note);	//Get the index of the previous note in this track difficulty
	if(prevnotenum >= 0)
	{	//If there is a previous note in this track difficulty
		prevnoteflags = eof_get_note_flags(eof_song, track, prevnotenum);	//Store its flags
	}

	if(flags & EOF_PRO_GUITAR_NOTE_FLAG_HO)
	{
		buffer[index++] = 'H';
	}
	else if(flags & EOF_PRO_GUITAR_NOTE_FLAG_PO)
	{
		buffer[index++] = 'P';
	}
	else if(flags & EOF_PRO_GUITAR_NOTE_FLAG_TAP)
	{
		buffer[index++] = 'T';
	}

	if(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_UP)
	{
		buffer[index++] = '/';
	}
	else if(flags & EOF_PRO_GUITAR_NOTE_FLAG_SLIDE_DOWN)
	{
		buffer[index++] = '\\';
	}

	if(flags & EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE)
	{
		if((prevnotenum >= 0) && (prevnoteflags & EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE))
		{	//If there is a previous note that was also a palm mute
			buffer[index++] = '-';	//Write a palm mute continuation character
		}
		else
		{
			buffer[index++] = 'P';
			buffer[index++] = 'M';
		}
	}
	else if(flags & EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE)
	{
		buffer[index++] = 'X';
	}

	if(flags & EOF_NOTE_FLAG_IS_TRILL)
	{
		if((prevnotenum >= 0) && (prevnoteflags & EOF_NOTE_FLAG_IS_TRILL))
		{	//If there is a previous note that was also in a trill
			buffer[index++] = '~';	//Write a trill continuation character
		}
		else
		{
			if(eof_song->track[track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
			{	//If this is a drum track, white the notation for special drum roll
				buffer[index++] = 'S';
				buffer[index++] = 'D';
				buffer[index++] = 'R';
			}
			else
			{	//Otherwise assume a guitar track, write the notation for a trill
				buffer[index++] = 't';
				buffer[index++] = 'r';
			}
		}
	}

	if(flags & EOF_NOTE_FLAG_IS_TREMOLO)
	{
		if((prevnotenum >= 0) && (prevnoteflags & EOF_NOTE_FLAG_IS_TREMOLO))
		{	//If there is a previous note that was also in a tremolo
			buffer[index++] = '-';	//Write a tremolo continuation character
		}
		else
		{
			if(eof_song->track[track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
			{	//If this is a drum track, white the notation for drum roll
				buffer[index++] = 'D';
				buffer[index++] = 'R';
			}
			else
			{	//Otherwise assume a guitar track, write the notation for a tremolo
				buffer[index++] = 'T';
				buffer[index++] = 'P';
			}
		}
	}

	buffer[index] = '\0';
}

int eof_note_compare(unsigned long track, unsigned long note1, unsigned long note2)
{
	unsigned long tracknum, ctr, bitmask;
	unsigned long note1note, note2note;

	//Validate parameters
	if((track == 0) || (track >= eof_song->tracks) || (note1 >= eof_get_track_size(eof_song, track)) || (note2 >= eof_get_track_size(eof_song, track)))
	{	//If an invalid track or note number was passed
		return -1;	//Return error
	}

	note1note = eof_get_note_note(eof_song, track, note1);
	note2note = eof_get_note_note(eof_song, track, note2);

	tracknum = eof_song->track[track]->tracknum;
	switch(eof_song->track[track]->track_format)
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
			if(note1note == note2note)
			{	//If both note's bitmasks match
				for(ctr = 0, bitmask = 1; ctr < 16; ctr ++, bitmask <<= 1)
				{	//For each of the 16 bits in the note bitmask
					if(note1note & bitmask)
					{	//If this bit is set
						if(eof_song->pro_guitar_track[tracknum]->note[note1]->frets[ctr] != eof_song->pro_guitar_track[tracknum]->note[note2]->frets[ctr])
						{	//If this string's fret value isn't the same for both notes
							return 1;	//Return not equal
						}
					}
				}
				return 0;	//Return equal
			}
		break;
	}

	return 1;	//Return not equal
}
