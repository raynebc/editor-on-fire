#include <allegro.h>
#include "modules/ocd3d.h"
#include "main.h"
#include "note.h"

int eof_note_count_colors(EOF_NOTE * np)
{
	int count = 0;

	if(np->note & 1)
	{
		count++;
	}
	if(np->note & 2)
	{
		count++;
	}
	if(np->note & 4)
	{
		count++;
	}
	if(np->note & 8)
	{
		count++;
	}
	if(np->note & 16)
	{
		count++;
	}
	return count;
}

void eof_note_create(EOF_NOTE * np, char g, char y, char r, char b, char p, char o, int pos, int length)
{
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

void eof_note_create2(EOF_NOTE * np, unsigned long bitmask, unsigned long pos, long length)
{
	np->note = bitmask;
	np->pos = pos;
	np->length = length;
}

int eof_adjust_notes(int offset)
{
	unsigned long i, j;
//	unsigned long tracknum = eof_song->track[eof_selected_track]->tracknum;

	for(i = 0; i < EOF_LEGACY_TRACKS_MAX; i++)
	{
		for(j = 0; j < eof_song->legacy_track[i]->notes; j++)
		{
			eof_song->legacy_track[i]->note[j]->pos += offset;
		}
		for(j = 0; j < eof_song->legacy_track[i]->solos; j++)
		{
			eof_song->legacy_track[i]->solo[j].start_pos += offset;
			eof_song->legacy_track[i]->solo[j].end_pos += offset;
		}
		for(j = 0; j < eof_song->legacy_track[i]->star_power_paths; j++)
		{
			eof_song->legacy_track[i]->star_power_path[j].start_pos += offset;
			eof_song->legacy_track[i]->star_power_path[j].end_pos += offset;
		}
	}
	for(j = 0; j < eof_song->vocal_track[0]->lyrics; j++)
	{
		eof_song->vocal_track[0]->lyric[j]->pos += offset;
	}
	for(j = 0; j < eof_song->vocal_track[0]->lines; j++)
	{
		eof_song->vocal_track[0]->line[j].start_pos += offset;
		eof_song->vocal_track[0]->line[j].end_pos += offset;
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
	int position;	//This is the position for the specified window's piano roll and is based on the passed window pointer
	int leftcoord;	//This is the position of the left end of the piano roll
	int pos;		//This is the position of the specified window's piano roll, scaled by the current zoom level
	int npos;
	int ychart[EOF_MAX_FRETS];
	int pcol = p == 1 ? eof_color_white : p == 2 ? makecol(224, 255, 224) : 0;
	int dcol = eof_color_white;
	int dcol2 = dcol;
	int colors[EOF_MAX_FRETS] = {eof_color_green,eof_color_red,eof_color_yellow,eof_color_blue,eof_color_purple,eof_color_orange};	//Each of the fret colors
	int ncol = eof_color_silver;	//Note color defaults to silver unless the note is not star power
	int ctr,ctr2;
	unsigned int mask;	//Used to mask out colors in the for loop
	int radius,dotsize;
	char iscymbal;		//Used to track whether the specified note is marked as a cymbal
	int x,y;
	unsigned long numlanes, tracknum;
	EOF_NOTE * np = NULL;

//Validate parameters
	if(window == NULL)
		return 1;	//Error, signal to stop rendering
	if(track != 0)
	{	//Render an existing note
		if(track >= eof_song->tracks)
			return 1;	//Error, signal to stop rendering
		tracknum = eof_song->track[track]->tracknum;
		if((eof_song->track[track]->track_format != EOF_LEGACY_TRACK_FORMAT) || (notenum >= eof_song->legacy_track[tracknum]->notes))
			return 1;	//Error, signal to stop rendering
		np = eof_song->legacy_track[tracknum]->note[notenum];	//Store the pointer to this note
	}
	else
	{	//Render the pen note
		np = &eof_pen_note;
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
		npos = 20 + (np->pos) / eof_zoom;
	}
	else
	{
		npos = 20 - ((pos - leftcoord)) + np->pos / eof_zoom;
	}

//Determine if the entire note would clip.  If so, return without attempting to render
	if(npos - eof_screen_layout.note_size > window->screen->w)	//If the note would render entirely to the right of the visible area
		return 1;	//Return status:  Clipping to the right of the viewing window

	if((npos < 0) && (npos + np->length / eof_zoom < 0))	//If the note and its tail would render entirely to the left of the visible area
		return -1;	//Return status:  Clipping to the left of the viewing window

	if(np->flags & EOF_NOTE_FLAG_CRAZY)
		dcol = eof_color_black;	//"Crazy" notes render with a black dot in the center

//Since Expert+ double bass notation uses the same flag as crazy status, override the dot color for PART DRUMS
	if(eof_selected_track == EOF_TRACK_DRUM)
		dcol = eof_color_white;

	if(track != 0)
	{	//If rendering an existing note
		numlanes = eof_count_track_lanes(track);	//Count the number of lanes in that note's track
	}
	else
	{
		numlanes = eof_count_track_lanes(eof_selected_track);	//Count the number of lanes in the active track
	}
	if(eof_inverted_notes)
	{
		for(ctr = 0, ctr2 = 0; ctr < EOF_MAX_FRETS; ctr++)
		{	//Store the fretboard lane positions in reverse order, with respect to the number of lanes in use
			if(EOF_MAX_FRETS - ctr <= numlanes)
			{	//If this lane is used in the chart, store it in ychart[] in reverse order
				ychart[ctr2++] = eof_screen_layout.note_y[EOF_MAX_FRETS - 1 - ctr];
			}
		}
	}
	else
	{
		for(ctr = 0; ctr < EOF_MAX_FRETS; ctr++)
		{	//Store the fretboard lane positions in normal order
			ychart[ctr] = eof_screen_layout.note_y[ctr];
		}
	}

	if(np->flags & EOF_NOTE_FLAG_F_HOPO)				//If this note is forced as HOPO on
	{
		radius=eof_screen_layout.hopo_note_size;		//Draw the note in the defined HOPO on size
		dotsize=eof_screen_layout.hopo_note_dot_size;
	}
	else if(np->flags & EOF_NOTE_FLAG_NO_HOPO)			//Or if this note is forced as HOPO off
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
	{	//Render for each of the available fret colors
		iscymbal = 0;
		x = npos;											//Store this to make the code more readable
		y = EOF_EDITOR_RENDER_OFFSET + 15 + ychart[ctr];	//Store this to make the code more readable

		if(np->note & mask)
		{
			if(!(np->flags & EOF_NOTE_FLAG_SP))
			{	//If the note is not star power
				ncol = colors[ctr];	//Assign the appropriate fret color
			}

			if((eof_selected_track == EOF_TRACK_DRUM))
			{	//Drum track specific dot color logic
				if((np->type == EOF_NOTE_AMAZING) && (np->flags & EOF_NOTE_FLAG_DBASS) && (mask == 1))
					dcol2 = eof_color_red;	//If this is an Expert+ bass drum note, render it with a red dot
				else if(((np->flags & EOF_NOTE_FLAG_Y_CYMBAL) && (mask == 4)) || ((np->flags & EOF_NOTE_FLAG_B_CYMBAL) && (mask == 8)) || ((np->flags & EOF_NOTE_FLAG_G_CYMBAL) && (mask == 16)))
				{	//If this drum note is marked as a yellow, blue or green cymbal
					iscymbal = 1;
				}
				else
					dcol2 = dcol;			//Otherwise render with the expected dot color
			}
			else
				dcol2 = dcol;			//Otherwise render with the expected dot color

			rectfill(window->screen, x, y - eof_screen_layout.note_tail_size, x + np->length / eof_zoom, y + eof_screen_layout.note_tail_size, ncol);
			if(p)
			{
				rect(window->screen, x, y - eof_screen_layout.note_tail_size, x + np->length / eof_zoom, y + eof_screen_layout.note_tail_size, pcol);
			}

			if(!iscymbal)
			{	//If this note is not a cymbal, render note as a circle
				circlefill(window->screen, x, y, radius, ncol);
				circlefill(window->screen, x, y, dotsize, dcol2);
				if(p)
				{
					circle(window->screen, x, y, radius, pcol);
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
		}
		else if((eof_hover_note >= 0) && (p == 3))
		{
			rect(window->screen, x, y - eof_screen_layout.note_tail_size, x + np->length / eof_zoom, y + eof_screen_layout.note_tail_size, eof_color_gray);
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
	}

	return 0;	//Return status:  Note was not clipped in its entirety
}

int eof_lyric_draw(EOF_LYRIC * np, int p, EOF_WINDOW *window)
{
	EOF_LYRIC *nextnp=NULL;	//Used to find the would-be X coordinate of the next lyric in the lyric lane
	unsigned long notenum = 0;
	char notpen = 0;	//Is set to nonzero if the passed lyric is determined to already be defined
	int position;	//This is the position for the specified window's piano roll and is based on the passed window pointer
	int leftcoord;	//This is the position of the left end of the piano roll
	int pos;		//This is the position of the specified window's piano roll, scaled by the current zoom level
	int npos;		//Stores the X coordinate at which to draw lyric #notenum
	int X2;			//Stores the X coordinate at which lyric #notenum+1 would be drawn (to set the clip rectangle)
	int stringend;	//Stores the coordinate position of the end of the lyric string, used for clipping logic
	int nplace;
	int note_y;
	int native = 0;
	int pcol = p == 1 ? eof_color_white : p == 2 ? makecol(224, 255, 224) : 0;
	int dcol = eof_color_white;
	int ncol = 0;
	EOF_LYRIC_LINE *lyricline;	//The line that this lyric is found to be in (if any) so the correct background color can be determined
	int bgcol = eof_color_black;	//Unless the text is found to be in a lyric phrase, it will render with a black background
	int endpos;		//This will store the effective end position for the lyric's rendering (taking lyric text, note rectangles and vocal slides into account)

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
	{
		ncol = native ? eof_color_red : eof_color_green;
		if(np->note != EOF_LYRIC_PERCUSSION)
		{
			rectfill(window->screen, npos, note_y, npos + np->length / eof_zoom, note_y + eof_screen_layout.vocal_tail_size - 1, ncol);

			int sliderect[8] = {0};	//An array of 4 vertices, used to draw a diagonal rectangle
			EOF_LYRIC *np2=NULL;	//Stores a pointer to the next lyric
			int nplace2 = 0;
			int native2 = 0;
			int note_y2 = 0;	//Used to store the y coordinate of the next lyric
			int npos2 = 0;		//Stores the X coordinate of the next lyric

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
	}
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
	int pos = eof_music_pos / eof_zoom_3d;
	int npos;
	int xchart[5] = {48, 48 + 56, 48 + 56 * 2, 48 + 56 * 3, 48 + 56 * 4};
	int bx = 48;
	int point[8];
	int rz, ez;
	int ctr;
	unsigned int mask;	//Used to mask out colors in the for loop
	unsigned int notes[EOF_MAX_FRETS] = {EOF_IMAGE_NOTE_GREEN, EOF_IMAGE_NOTE_RED, EOF_IMAGE_NOTE_YELLOW, EOF_IMAGE_NOTE_BLUE, EOF_IMAGE_NOTE_PURPLE, EOF_IMAGE_NOTE_ORANGE};
	unsigned int notes_hit[EOF_MAX_FRETS] = {EOF_IMAGE_NOTE_GREEN_HIT, EOF_IMAGE_NOTE_RED_HIT, EOF_IMAGE_NOTE_YELLOW_HIT, EOF_IMAGE_NOTE_BLUE_HIT, EOF_IMAGE_NOTE_PURPLE_HIT, EOF_IMAGE_NOTE_ORANGE_HIT};
	unsigned int hopo_notes[EOF_MAX_FRETS] = {EOF_IMAGE_NOTE_HGREEN, EOF_IMAGE_NOTE_HRED, EOF_IMAGE_NOTE_HYELLOW, EOF_IMAGE_NOTE_HBLUE, EOF_IMAGE_NOTE_HPURPLE, EOF_IMAGE_NOTE_HORANGE};
	unsigned int hopo_notes_hit[EOF_MAX_FRETS] = {EOF_IMAGE_NOTE_HGREEN_HIT, EOF_IMAGE_NOTE_HRED_HIT, EOF_IMAGE_NOTE_HYELLOW_HIT, EOF_IMAGE_NOTE_HBLUE_HIT, EOF_IMAGE_NOTE_HPURPLE_HIT, EOF_IMAGE_NOTE_HORANGE_HIT};
	unsigned int cymbals[EOF_MAX_FRETS] = {EOF_IMAGE_NOTE_GREEN, EOF_IMAGE_NOTE_RED, EOF_IMAGE_NOTE_YELLOW_CYMBAL, EOF_IMAGE_NOTE_BLUE_CYMBAL, EOF_IMAGE_NOTE_PURPLE_CYMBAL, EOF_IMAGE_NOTE_ORANGE};
	unsigned int cymbals_hit[EOF_MAX_FRETS] = {EOF_IMAGE_NOTE_GREEN_HIT, EOF_IMAGE_NOTE_RED_HIT, EOF_IMAGE_NOTE_YELLOW_CYMBAL_HIT, EOF_IMAGE_NOTE_BLUE_CYMBAL_HIT, EOF_IMAGE_NOTE_PURPLE_CYMBAL_HIT, EOF_IMAGE_NOTE_ORANGE_HIT};
	unsigned long numlanes, tracknum;
	EOF_NOTE * np = NULL;

//Validate parameters
	tracknum = eof_song->track[track]->tracknum;
	if((track == 0) || (track >= eof_song->tracks) || (eof_song->track[track]->track_format != EOF_LEGACY_TRACK_FORMAT) || (notenum >= eof_song->legacy_track[tracknum]->notes))
	{	//If an invalid track or note number was passsed
		return -1;	//Error, signal to stop rendering (3D window renders last note to first)
	}
	np = eof_song->legacy_track[tracknum]->note[notenum];	//Store the pointer to this note
	numlanes = eof_count_track_lanes(track);

	npos = -pos - 6 + np->pos / eof_zoom_3d + eof_av_delay / eof_zoom_3d;
	if(npos + np->length / eof_zoom_3d < -100)
	{				//If the note would render entirely before the visible area
		return -1;	//Return status:  Clipping before the viewing window
	}
	else if(npos > 600)
	{				//If the note would render entirely after the visible area
		return 1;	//Return status:  Clipping after the viewing window
	}
	if(track == EOF_TRACK_DRUM)
	{
		if(eof_lefty_mode)
		{
			xchart[0] = 48 + 56 * 3;
			xchart[1] = 48 + 56 * 2;
			xchart[2] = 48 + 56;
			xchart[3] = 48;
			xchart[4] = 48;
		}
		if(np->note & 1)
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

			if(np->flags & EOF_NOTE_FLAG_SP)			//If this bass drum note is star power, render it in silver
				polygon(eof_window_3d->screen, 4, point, p ? eof_color_white : eof_color_silver);
			else if(np->flags & EOF_NOTE_FLAG_DBASS)	//Or if it is double bass, render it in red
				polygon(eof_window_3d->screen, 4, point, p ? makecol(255, 192, 192) : eof_color_red);
			else										//Otherwise render it in green
				polygon(eof_window_3d->screen, 4, point, p ? makecol(192, 255, 192) : eof_color_green);
		}
		for(ctr=1,mask=2;ctr<EOF_MAX_FRETS;ctr++,mask=mask<<1)
		{	//Render for each of the available fret colors after 1 (bass drum)
			if(np->note & mask)
			{
				if(((np->flags & EOF_NOTE_FLAG_Y_CYMBAL) && (mask == 4)) || ((np->flags & EOF_NOTE_FLAG_B_CYMBAL) && (mask == 8)) || ((np->flags & EOF_NOTE_FLAG_G_CYMBAL) && (mask == 16)))
				{	//If this is a cymbal note, render with the cymbal image
					if(np->flags & EOF_NOTE_FLAG_SP)
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
					if(np->flags & EOF_NOTE_FLAG_SP)
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
	}
	else
	{
		if(eof_lefty_mode)
		{
			xchart[0] = 48 + 56 * 4;
			xchart[1] = 48 + 56 * 3;
			xchart[2] = 48 + 56 * 2;
			xchart[3] = 48 + 56;
			xchart[4] = 48;
		}
		for(ctr=0,mask=1;ctr<EOF_MAX_FRETS;ctr++,mask=mask<<1)
		{	//Render for each of the available fret colors
			if(np->note & mask)
			{
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

					if(np->flags & EOF_NOTE_FLAG_SP)			//If this open bass note is star power, render it in silver
						polygon(eof_window_3d->screen, 4, point, p ? eof_color_white : eof_color_silver);
					else										//Otherwise render it in orange
						polygon(eof_window_3d->screen, 4, point, p ? makecol(255, 192, 0) : eof_color_orange);
				}
				else if(mask < 32)
				{	//For now, lane 6 is not rendered except for open bass
					if(np->flags & EOF_NOTE_FLAG_HOPO)
					{	//If this is a HOPO note
						if(np->flags & EOF_NOTE_FLAG_SP)
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
						if(np->flags & EOF_NOTE_FLAG_SP)
						{	//If this is an SP note
							ocd3d_draw_bitmap(eof_window_3d->screen, p ? eof_image[EOF_IMAGE_NOTE_WHITE_HIT] : eof_image[EOF_IMAGE_NOTE_WHITE], xchart[ctr] - 24, 200 - 48, npos);
						}
						else
						{
							ocd3d_draw_bitmap(eof_window_3d->screen, p ? eof_image[notes_hit[ctr]] : eof_image[notes[ctr]], xchart[ctr] - 24, 200 - 48, npos);
						}
					}
				}
			}
		}
	}

	return 0;	//Return status:  Note was not clipped in its entirety
}

int eof_note_tail_draw_3d(unsigned long track, unsigned long notenum, int p)
{
	long pos = eof_music_pos / eof_zoom_3d;
	long npos;
	int xchart[5] = {48, 48 + 56, 48 + 56 * 2, 48 + 56 * 3, 48 + 56 * 4};
	int point[8];
	int rz, ez;
	unsigned long numlanes, tracknum, ctr, mask;
	EOF_NOTE * np = NULL;
	int colortable[EOF_MAX_FRETS][2] = {{makecol(192, 255, 192), eof_color_green}, {makecol(255, 192, 192), eof_color_red}, {makecol(255, 255, 192), eof_color_yellow}, {makecol(192, 192, 255), eof_color_blue}, {makecol(255, 192, 255), eof_color_purple}, {makecol(255, 192, 0), eof_color_orange}};

//Validate parameters
	tracknum = eof_song->track[track]->tracknum;
	if((track == 0) || (track >= eof_song->tracks) || (eof_song->track[track]->track_format != EOF_LEGACY_TRACK_FORMAT) || (notenum >= eof_song->legacy_track[tracknum]->notes))
	{	//If an invalid track or note number was passsed
		return -1;	//Error, signal to stop rendering (3D window renders last note to first)
	}
	np = eof_song->legacy_track[tracknum]->note[notenum];	//Store the pointer to this note
	numlanes = eof_count_track_lanes(track);

	npos = -pos - 6 + (np->pos + eof_av_delay) / eof_zoom_3d;
	if(npos + np->length / eof_zoom_3d < -100)
	{
		return -1;
	}
	else if(npos > 600)
	{
		return 1;
	}

	if(eof_lefty_mode)
	{
		xchart[0] = 48 + 56 * 4;
		xchart[1] = 48 + 56 * 3;
		xchart[2] = 48 + 56 * 2;
		xchart[3] = 48 + 56;
		xchart[4] = 48;
	}

	if((eof_selected_track == EOF_TRACK_DRUM) || (np->length <= 10))
		return 0;	//Don't render tails for drum notes or notes that aren't over 10ms long

	rz = npos < -100 ? -100 : npos + 10;
	ez = npos + np->length / eof_zoom_3d > 600 ? 600 : npos + np->length / eof_zoom_3d + 6;
	for(ctr=0,mask=1; ctr < EOF_MAX_FRETS; ctr++,mask=mask<<1)
	{	//For each of the available frets
		if(np->note & mask)
		{	//If this lane has a gem to render
			if(ctr < 5)
			{	//Logic to render lanes 1 through 5
				point[0] = ocd3d_project_x(xchart[ctr] - 10, rz);
				point[1] = ocd3d_project_y(200, rz);
				point[2] = ocd3d_project_x(xchart[ctr] - 10, ez);
				point[3] = ocd3d_project_y(200, ez);
				point[4] = ocd3d_project_x(xchart[ctr] + 10, ez);
				point[5] = ocd3d_project_y(200, ez);
				point[6] = ocd3d_project_x(xchart[ctr] + 10, rz);
				point[7] = ocd3d_project_y(200, rz);
				polygon(eof_window_3d->screen, 4, point, np->flags & EOF_NOTE_FLAG_SP ? (p ? eof_color_white : eof_color_silver) : (p ? colortable[ctr][0] : colortable[ctr][1]));
			}
			else if((ctr == 5) && (track == EOF_TRACK_BASS) && eof_open_bass_enabled())
			{	//Logic to render open bass strum notes (a rectangle covering the width of rendering of frets 2, 3 and 4
				point[0] = ocd3d_project_x(xchart[1] - 10, rz);
				point[1] = ocd3d_project_y(200, rz);
				point[2] = ocd3d_project_x(xchart[1] - 10, ez);
				point[3] = ocd3d_project_y(200, ez);
				point[4] = ocd3d_project_x(xchart[3] + 10, ez);
				point[5] = ocd3d_project_y(200, ez);
				point[6] = ocd3d_project_x(xchart[3] + 10, rz);
				point[7] = ocd3d_project_y(200, rz);
				polygon(eof_window_3d->screen, 4, point, np->flags & EOF_NOTE_FLAG_SP ? (p ? eof_color_white : eof_color_silver) : (p ? colortable[ctr][0] : colortable[ctr][1]));
			}
		}
	}
	return 0;
}

EOF_LYRIC_LINE *eof_find_lyric_line(unsigned long lyricnum)
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
		return 0;

	for(ctr = 0; ctr < eof_song->vocal_track[0]->lyrics; ctr++)
	{	//For each lyric
		if(np == eof_song->vocal_track[0]->lyric[ctr])
			return ctr;
	}

	return 0;
}
