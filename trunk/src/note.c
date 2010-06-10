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

void eof_note_create(EOF_NOTE * np, char g, char y, char r, char b, char p, int pos, int length)
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
	np->pos = pos;
	np->length = length;
}

int eof_adjust_notes(int offset)
{
	int i, j;

	for(i = 0; i < EOF_MAX_TRACKS; i++)
	{
		for(j = 0; j < eof_song->track[i]->notes; j++)
		{
			eof_song->track[i]->note[j]->pos += offset;
		}
		for(j = 0; j < eof_song->track[i]->solos; j++)
		{
			eof_song->track[i]->solo[j].start_pos += offset;
			eof_song->track[i]->solo[j].end_pos += offset;
		}
		for(j = 0; j < eof_song->track[i]->star_power_paths; j++)
		{
			eof_song->track[i]->star_power_path[j].start_pos += offset;
			eof_song->track[i]->star_power_path[j].end_pos += offset;
		}
	}
	for(j = 0; j < eof_song->vocal_track->lyrics; j++)
	{
		eof_song->vocal_track->lyric[j]->pos += offset;
	}
	for(j = 0; j < eof_song->vocal_track->lines; j++)
	{
		eof_song->vocal_track->line[j].start_pos += offset;
		eof_song->vocal_track->line[j].end_pos += offset;
	}
	for(i = 0; i < eof_song->catalog->entries; i++)
	{
		eof_song->catalog->entry[i].start_pos += offset;
		eof_song->catalog->entry[i].end_pos += offset;
	}
	for(i = 0; i < 10; i++)
	{
		if(eof_song->bookmark_pos[i] != 0)
		{
			eof_song->bookmark_pos[i] += offset;
		}
	}
	return 1;
}

void eof_note_draw(EOF_NOTE * np, int p)
{
	int pos = eof_music_pos / eof_zoom;
	int npos;
	int ychart[5] = {20, 40, 60, 80, 100};
	int pcol = p == 1 ? makecol(255, 255, 255) : p == 2 ? makecol(224, 255, 224) : 0;
	int dcol = (np->flags & EOF_NOTE_FLAG_CRAZY) ? makecol(0, 0, 0) : makecol(255, 255, 255);
	int ncol = 0;

	if(p == 3)
	{
		pcol = eof_color_white;
		dcol = eof_color_white;
	}

	if(eof_inverted_notes)
	{
		ychart[0] = eof_screen_layout.note_y[4];
		ychart[1] = eof_screen_layout.note_y[3];
		ychart[2] = eof_screen_layout.note_y[2];
		ychart[3] = eof_screen_layout.note_y[1];
		ychart[4] = eof_screen_layout.note_y[0];
	}
	else
	{
		ychart[0] = eof_screen_layout.note_y[0];
		ychart[1] = eof_screen_layout.note_y[1];
		ychart[2] = eof_screen_layout.note_y[2];
		ychart[3] = eof_screen_layout.note_y[3];
		ychart[4] = eof_screen_layout.note_y[4];
	}

	if(pos < 300)
	{
		npos = 20 + (np->pos) / eof_zoom;
	}
	else
	{
		npos = 20 - ((pos - 300)) + np->pos / eof_zoom;
	}

	if(np->flags & EOF_NOTE_FLAG_SP)
	{
		ncol = makecol(192, 192, 192);
	}
	if(p == 3)
	{
		vline(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[2] - eof_screen_layout.note_marker_size, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[2] + eof_screen_layout.note_marker_size, makecol(128, 128, 128));
		if(np->note & 1)
		{
			if(!(np->flags & EOF_NOTE_FLAG_SP))
			{
				ncol = eof_color_green;
			}
			rectfill(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[0] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[0] + eof_screen_layout.note_tail_size, ncol);
			rect(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[0] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[0] + eof_screen_layout.note_tail_size, pcol);
			circlefill(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[0], eof_screen_layout.note_size, ncol);
			circlefill(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[0], eof_screen_layout.note_dot_size, dcol);
			circle(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[0], eof_screen_layout.note_size, pcol);
		}
		else if(eof_hover_note >= 0)
		{
			rect(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[0] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[0] + eof_screen_layout.note_tail_size, eof_color_gray);
			circle(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[0], eof_screen_layout.note_size, eof_color_gray);
		}
		if(np->note & 2)
		{
			if(!(np->flags & EOF_NOTE_FLAG_SP))
			{
				ncol = eof_color_red;
			}
			rectfill(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[1] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[1] + eof_screen_layout.note_tail_size, ncol);
			rect(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[1] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[1] + eof_screen_layout.note_tail_size, pcol);
			circlefill(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[1], eof_screen_layout.note_size, ncol);
			circlefill(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[1], eof_screen_layout.note_dot_size, dcol);
			circle(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[1], eof_screen_layout.note_size, pcol);
		}
		else if(eof_hover_note >= 0)
		{
			rect(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[1] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[1] + eof_screen_layout.note_tail_size, eof_color_gray);
			circle(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[1], eof_screen_layout.note_size, eof_color_gray);
		}
		if(np->note & 4)
		{
			if(!(np->flags & EOF_NOTE_FLAG_SP))
			{
				ncol = eof_color_yellow;
			}
			rectfill(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[2] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[2] + eof_screen_layout.note_tail_size, ncol);
			rect(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[2] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[2] + eof_screen_layout.note_tail_size, pcol);
			circlefill(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[2], eof_screen_layout.note_size, ncol);
			circlefill(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[2], eof_screen_layout.note_dot_size, dcol);
			circle(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[2], eof_screen_layout.note_size, pcol);
		}
		else if(eof_hover_note >= 0)
		{
			rect(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[2] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[2] + eof_screen_layout.note_tail_size, eof_color_gray);
			circle(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[2], eof_screen_layout.note_size, eof_color_gray);
		}
		if(np->note & 8)
		{
			if(!(np->flags & EOF_NOTE_FLAG_SP))
			{
				ncol = eof_color_blue;
			}
			rectfill(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[3] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[3] + eof_screen_layout.note_tail_size, ncol);
			rect(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[3] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[3] + eof_screen_layout.note_tail_size, pcol);
			circlefill(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[3], eof_screen_layout.note_size, ncol);
			circlefill(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[3], eof_screen_layout.note_dot_size, dcol);
			circle(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[3], eof_screen_layout.note_size, pcol);
		}
		else if(eof_hover_note >= 0)
		{
			rect(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[3] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[3] + eof_screen_layout.note_tail_size, eof_color_gray);
			circle(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[3], eof_screen_layout.note_size, eof_color_gray);
		}
		if(np->note & 16)
		{
			if(!(np->flags & EOF_NOTE_FLAG_SP))
			{
				ncol = eof_color_purple;
			}
			rectfill(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[4] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[4] + eof_screen_layout.note_tail_size, ncol);
			rect(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[4] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[4] + eof_screen_layout.note_tail_size, pcol);
			circlefill(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[4], eof_screen_layout.note_size, ncol);
			circlefill(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[4], eof_screen_layout.note_dot_size, dcol);
			circle(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[4], eof_screen_layout.note_size, pcol);
		}
		else if(eof_hover_note >= 0)
		{
			rect(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[4] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[4] + eof_screen_layout.note_tail_size, eof_color_gray);
			circle(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[4], eof_screen_layout.note_size, eof_color_gray);
		}
	}
	else
	{
		vline(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[2] - eof_screen_layout.note_marker_size, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[2] + eof_screen_layout.note_marker_size, makecol(128, 128, 128));
		if(np->note & 1)
		{
			if(!(np->flags & EOF_NOTE_FLAG_SP))
			{
				ncol = eof_color_green;
			}
			rectfill(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[0] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[0] + eof_screen_layout.note_tail_size, ncol);
			if(p)
			{
				rect(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[0] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[0] + eof_screen_layout.note_tail_size, pcol);
			}
			circlefill(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[0], eof_screen_layout.note_size, ncol);
			circlefill(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[0], eof_screen_layout.note_dot_size, dcol);
			if(p)
			{
				circle(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[0], eof_screen_layout.note_size, pcol);
			}
		}
		if(np->note & 2)
		{
			if(!(np->flags & EOF_NOTE_FLAG_SP))
			{
				ncol = eof_color_red;
			}
			rectfill(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[1] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[1] + eof_screen_layout.note_tail_size, ncol);
			if(p)
			{
				rect(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[1] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[1] + eof_screen_layout.note_tail_size, pcol);
			}
			circlefill(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[1], eof_screen_layout.note_size, ncol);
			circlefill(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[1], eof_screen_layout.note_dot_size, dcol);
			if(p)
			{
				circle(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[1], eof_screen_layout.note_size, pcol);
			}
		}
		if(np->note & 4)
		{
			if(!(np->flags & EOF_NOTE_FLAG_SP))
			{
				ncol = eof_color_yellow;
			}
			rectfill(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[2] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[2] + eof_screen_layout.note_tail_size, ncol);
			if(p)
			{
				rect(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[2] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[2] + eof_screen_layout.note_tail_size, pcol);
			}
			circlefill(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[2], eof_screen_layout.note_size, ncol);
			circlefill(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[2], eof_screen_layout.note_dot_size, dcol);
			if(p)
			{
				circle(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[2], eof_screen_layout.note_size, pcol);
			}
		}
		if(np->note & 8)
		{
			if(!(np->flags & EOF_NOTE_FLAG_SP))
			{
				ncol = eof_color_blue;
			}
			rectfill(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[3] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[3] + eof_screen_layout.note_tail_size, ncol);
			if(p)
			{
				rect(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[3] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[3] + eof_screen_layout.note_tail_size, pcol);
			}
			circlefill(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[3], eof_screen_layout.note_size, ncol);
			circlefill(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[3], eof_screen_layout.note_dot_size, dcol);
			if(p)
			{
				circle(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[3], eof_screen_layout.note_size, pcol);
			}
		}
		if(np->note & 16)
		{
			if(!(np->flags & EOF_NOTE_FLAG_SP))
			{
				ncol = eof_color_purple;
			}
			rectfill(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[4] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[4] + eof_screen_layout.note_tail_size, ncol);
			if(p)
			{
				rect(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[4] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[4] + eof_screen_layout.note_tail_size, pcol);
			}
			circlefill(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[4], eof_screen_layout.note_size, ncol);
			circlefill(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[4], eof_screen_layout.note_dot_size, dcol);
			if(p)
			{
				circle(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[4], eof_screen_layout.note_size, pcol);
			}
		}
	}
}

void eof_lyric_draw(EOF_LYRIC * np, int p)
{
	int pos = eof_music_pos / eof_zoom;
	int npos;
	int nplace;
	int note_y;
	int native = 0;
	int ychart[5] = {20, 40, 60, 80, 100};
	int pcol = p == 1 ? makecol(255, 255, 255) : p == 2 ? makecol(224, 255, 224) : 0;
	int dcol = makecol(255, 255, 255);
	int ncol = 0;
	
	EOF_LYRIC_LINE *lyricline;	//The line that this lyric is found to be in (if any) so the correct background color can be determined
	int bgcol = eof_color_black;	//Unless the text is found to be in a lyric phrase, it will render with a black background

	if(p == 3)
	{
		pcol = eof_color_white;
		dcol = eof_color_white;
	}

	lyricline=FindLyricLine_p(np);	//Find which line this lyric is in
	if(lyricline != NULL)
	{
		if((lyricline->flags) & EOF_LYRIC_LINE_FLAG_OVERDRIVE)	//If the overdrive flag is set
			bgcol=makecol(64, 128, 64);	//Make dark green the text's background color
		else
			bgcol=makecol(0, 0, 127);	//Make dark blue the text's background colo
	}
	
	ychart[0] = eof_screen_layout.note_y[0];
	ychart[1] = eof_screen_layout.note_y[1];
	ychart[2] = eof_screen_layout.note_y[2];
	ychart[3] = eof_screen_layout.note_y[3];
	ychart[4] = eof_screen_layout.note_y[4];

	if(pos < 300)
	{
		npos = 20 + (np->pos) / eof_zoom;
	}
	else
	{
		npos = 20 - ((pos - 300)) + np->pos / eof_zoom;
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
	vline(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.vocal_y - ((eof_screen_layout.vocal_view_size + 2) * eof_screen_layout.vocal_tail_size) / 2 - eof_screen_layout.note_marker_size, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.vocal_y - ((eof_screen_layout.vocal_view_size + 2) * eof_screen_layout.vocal_tail_size) / 2 + eof_screen_layout.note_marker_size, makecol(128, 128, 128));
	if(np->note != 0)
	{
		ncol = native ? eof_color_red : eof_color_green;
		rectfill(eof_window_editor->screen, npos, note_y, npos + np->length / eof_zoom, note_y + eof_screen_layout.vocal_tail_size - 1, ncol);
		if(p)
		{
			rect(eof_window_editor->screen, npos, note_y, npos + np->length / eof_zoom, note_y + eof_screen_layout.vocal_tail_size - 1, pcol);
		}
	}
	else	//If the lyric is pitchless, render with gray
		rectfill(eof_window_editor->screen, npos, note_y, npos + np->length / eof_zoom, note_y + eof_screen_layout.vocal_tail_size - 1, makecol(128, 128, 128));

	if(p == 3)
	{
		set_clip_rect(eof_window_editor->screen, 0, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.lyric_y + 1, eof_window_editor->screen->w, eof_window_editor->screen->h);
		textprintf_ex(eof_window_editor->screen, font, npos, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.lyric_y, eof_color_white, bgcol, "%s", np->text);
		set_clip_rect(eof_window_editor->screen, 0, 0, eof_window_editor->screen->w, eof_window_editor->screen->h);
	}
	else
	{
		set_clip_rect(eof_window_editor->screen, 0, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.lyric_y + 1, eof_window_editor->screen->w, eof_window_editor->screen->h);
		textprintf_ex(eof_window_editor->screen, font, npos, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.lyric_y, p ? eof_color_green : eof_color_white, bgcol, "%s", np->text);
		set_clip_rect(eof_window_editor->screen, 0, 0, eof_window_editor->screen->w, eof_window_editor->screen->h);
	}
}

int eof_lyric_draw_truncate(int notenum, int p)
{
	EOF_LYRIC *np=NULL;
	EOF_LYRIC *nextnp=NULL;	//Used to find the would-be X coordinate of the next lyric in the lyric lane

	int pos = eof_music_pos / eof_zoom;
	int npos;		//Stores the X coordinate at which to draw lyric #notenum
	int X2;			//Stores the X coordinate at which lyric #notenum+1 would be drawn (to set the clip rectangle)
	int nplace;
	int note_y;
	int native = 0;
	int ychart[5] = {20, 40, 60, 80, 100};
	int pcol = p == 1 ? makecol(255, 255, 255) : p == 2 ? makecol(224, 255, 224) : 0;
	int dcol = makecol(255, 255, 255);
	int ncol = 0;

	EOF_LYRIC_LINE *lyricline;	//The line that this lyric is found to be in (if any) so the correct background color can be determined
	int bgcol = eof_color_black;	//Unless the text is found to be in a lyric phrase, it will render with a black background

	if(notenum >= eof_song->vocal_track->lyrics)	//If this is outside the bounds of EOF's defined lyrics
		return 1;	//Stop rendering

	lyricline=FindLyricLine(notenum);	//Find which line this lyric is in
	if(lyricline != NULL)
	{
		if((lyricline->flags) & EOF_LYRIC_LINE_FLAG_OVERDRIVE)	//If the overdrive flag is set
			bgcol=makecol(64, 128, 64);	//Make dark green the text's background color
		else
			bgcol=makecol(0, 0, 127);	//Make dark blue the text's background colo
	}

	np=eof_song->vocal_track->lyric[notenum];

	if(notenum < eof_song->vocal_track->lyrics-1)		//If there is another lyric
	{							//Find its would-be X coordinate (use as X2 for clipping rect)
		nextnp=eof_song->vocal_track->lyric[notenum+1];
		if(pos < 300)
			X2 = 20 + (nextnp->pos) / eof_zoom;
		else
			X2 = 20 - ((pos - 300)) + nextnp->pos / eof_zoom;
	}
	else
		X2=eof_window_editor->screen->w;		//The default X2 coordinate for the clipping rectangle


	if(p == 3)
	{
		pcol = eof_color_white;
		dcol = eof_color_white;
	}

	ychart[0] = eof_screen_layout.note_y[0];
	ychart[1] = eof_screen_layout.note_y[1];
	ychart[2] = eof_screen_layout.note_y[2];
	ychart[3] = eof_screen_layout.note_y[3];
	ychart[4] = eof_screen_layout.note_y[4];

	if(pos < 300)
	{
		npos = 20 + (np->pos) / eof_zoom;
	}
	else
	{
		npos = 20 - ((pos - 300)) + np->pos / eof_zoom;
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
	vline(eof_window_editor->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.vocal_y - ((eof_screen_layout.vocal_view_size + 2) * eof_screen_layout.vocal_tail_size) / 2 - eof_screen_layout.note_marker_size, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.vocal_y - ((eof_screen_layout.vocal_view_size + 2) * eof_screen_layout.vocal_tail_size) / 2 + eof_screen_layout.note_marker_size, makecol(128, 128, 128));
	if(np->note != 0)
	{
		ncol = native ? eof_color_red : eof_color_green;
		rectfill(eof_window_editor->screen, npos, note_y, npos + np->length / eof_zoom, note_y + eof_screen_layout.vocal_tail_size - 1, ncol);
		if(p)
		{
			rect(eof_window_editor->screen, npos, note_y, npos + np->length / eof_zoom, note_y + eof_screen_layout.vocal_tail_size - 1, pcol);
		}
	}
	else	//If the lyric is pitchless, render with gray
		rectfill(eof_window_editor->screen, npos, note_y, npos + np->length / eof_zoom, note_y + eof_screen_layout.vocal_tail_size - 1, makecol(128, 128, 128));


//Rewritten logic checks the next lyric to set an appropriate clipping rectangle for truncation purposes, and use the determined background color (so phrase marking rectangle isn't erased by text)
	set_clip_rect(eof_window_editor->screen, 0, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.lyric_y + 1, X2-2, eof_window_editor->screen->h);	//Alteration: Provide at least two pixels of clearance for the edge of the clip rectangle

	if(p == 3)
		textprintf_ex(eof_window_editor->screen, font, npos, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.lyric_y, eof_color_white, bgcol, "%s", np->text);
	else
		textprintf_ex(eof_window_editor->screen, font, npos, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.lyric_y, p ? eof_color_green : eof_color_white, bgcol, "%s", np->text);

	set_clip_rect(eof_window_editor->screen, 0, 0, eof_window_editor->screen->w, eof_window_editor->screen->h);	//Restore original clipping rectangle

//Rewritten logic returns nonzero if NO pixels were written to the left of the EOF window's X2 coordinate
	if(npos > eof_window_editor->screen->w)	//If text was rendered beginning beyond EOF's right window border
		return 1;
	else
		return 0;
}

void eof_note_draw_3d(EOF_NOTE * np, int p)
{
	int pos = eof_music_pos / eof_zoom_3d;
	int npos;
	int xchart[5] = {48, 48 + 56, 48 + 56 * 2, 48 + 56 * 3, 48 + 56 * 4};
	int bx = 48;
	int point[8];
	int rz, ez;

	npos = -pos - 6 + np->pos / eof_zoom_3d + eof_av_delay / eof_zoom_3d;
	if(npos + np->length / eof_zoom_3d < -100 || npos > 600)
	{
		return;
	}
	if(eof_selected_track == EOF_TRACK_DRUM)
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
			polygon(eof_window_3d->screen, 4, point, p ? makecol(192, 255, 192) : eof_color_green);
		}
		if(np->note & 2)
		{
			ocd3d_draw_bitmap(eof_window_3d->screen, p ? eof_image[EOF_IMAGE_NOTE_RED_HIT] : eof_image[EOF_IMAGE_NOTE_RED], xchart[0] - 24 + 28, 200 - 48, npos);
		}
		if(np->note & 4)
		{
			ocd3d_draw_bitmap(eof_window_3d->screen, p ? eof_image[EOF_IMAGE_NOTE_YELLOW_HIT] : eof_image[EOF_IMAGE_NOTE_YELLOW], xchart[1] - 24 + 28, 200 - 48, npos);
		}
		if(np->note & 8)
		{
			ocd3d_draw_bitmap(eof_window_3d->screen, p ? eof_image[EOF_IMAGE_NOTE_BLUE_HIT] : eof_image[EOF_IMAGE_NOTE_BLUE], xchart[2] - 24 + 28, 200 - 48, npos);
		}
		if(np->note & 16)
		{
			ocd3d_draw_bitmap(eof_window_3d->screen, p ? eof_image[EOF_IMAGE_NOTE_PURPLE_HIT] : eof_image[EOF_IMAGE_NOTE_PURPLE], xchart[3] - 24 + 28, 200 - 48, npos);
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
		if(np->note & 1)
		{
			if(np->flags & EOF_NOTE_FLAG_HOPO)
			{
				if(np->flags & EOF_NOTE_FLAG_SP)
				{
					ocd3d_draw_bitmap(eof_window_3d->screen, p ? eof_image[EOF_IMAGE_NOTE_HWHITE_HIT] : eof_image[EOF_IMAGE_NOTE_HWHITE], xchart[0] - 24, 200 - 48, npos);
				}
				else
				{
					ocd3d_draw_bitmap(eof_window_3d->screen, p ? eof_image[EOF_IMAGE_NOTE_HGREEN_HIT] : eof_image[EOF_IMAGE_NOTE_HGREEN], xchart[0] - 24, 200 - 48, npos);
				}
			}
			else
			{
				if(np->flags & EOF_NOTE_FLAG_SP)
				{
					ocd3d_draw_bitmap(eof_window_3d->screen, p ? eof_image[EOF_IMAGE_NOTE_WHITE_HIT] : eof_image[EOF_IMAGE_NOTE_WHITE], xchart[0] - 24, 200 - 48, npos);
				}
				else
				{
					ocd3d_draw_bitmap(eof_window_3d->screen, p ? eof_image[EOF_IMAGE_NOTE_GREEN_HIT] : eof_image[EOF_IMAGE_NOTE_GREEN], xchart[0] - 24, 200 - 48, npos);
				}
			}
		}
		if(np->note & 2)
		{
			if(np->flags & EOF_NOTE_FLAG_HOPO)
			{
				if(np->flags & EOF_NOTE_FLAG_SP)
				{
					ocd3d_draw_bitmap(eof_window_3d->screen, p ? eof_image[EOF_IMAGE_NOTE_HWHITE_HIT] : eof_image[EOF_IMAGE_NOTE_HWHITE], xchart[1] - 24, 200 - 48, npos);
				}
				else
				{
					ocd3d_draw_bitmap(eof_window_3d->screen, p ? eof_image[EOF_IMAGE_NOTE_HRED_HIT] : eof_image[EOF_IMAGE_NOTE_HRED], xchart[1] - 24, 200 - 48, npos);
				}
			}
			else
			{
				if(np->flags & EOF_NOTE_FLAG_SP)
				{
					ocd3d_draw_bitmap(eof_window_3d->screen, p ? eof_image[EOF_IMAGE_NOTE_WHITE_HIT] : eof_image[EOF_IMAGE_NOTE_WHITE], xchart[1] - 24, 200 - 48, npos);
				}
				else
				{
					ocd3d_draw_bitmap(eof_window_3d->screen, p ? eof_image[EOF_IMAGE_NOTE_RED_HIT] : eof_image[EOF_IMAGE_NOTE_RED], xchart[1] - 24, 200 - 48, npos);
				}
			}
		}
		if(np->note & 4)
		{
			if(np->flags & EOF_NOTE_FLAG_HOPO)
			{
				if(np->flags & EOF_NOTE_FLAG_SP)
				{
					ocd3d_draw_bitmap(eof_window_3d->screen, p ? eof_image[EOF_IMAGE_NOTE_HWHITE_HIT] : eof_image[EOF_IMAGE_NOTE_HWHITE], xchart[2] - 24, 200 - 48, npos);
				}
				else
				{
					ocd3d_draw_bitmap(eof_window_3d->screen, p ? eof_image[EOF_IMAGE_NOTE_HYELLOW_HIT] : eof_image[EOF_IMAGE_NOTE_HYELLOW], xchart[2] - 24, 200 - 48, npos);
				}
			}
			else
			{
				if(np->flags & EOF_NOTE_FLAG_SP)
				{
					ocd3d_draw_bitmap(eof_window_3d->screen, p ? eof_image[EOF_IMAGE_NOTE_WHITE_HIT] : eof_image[EOF_IMAGE_NOTE_WHITE], xchart[2] - 24, 200 - 48, npos);
				}
				else
				{
					ocd3d_draw_bitmap(eof_window_3d->screen, p ? eof_image[EOF_IMAGE_NOTE_YELLOW_HIT] : eof_image[EOF_IMAGE_NOTE_YELLOW], xchart[2] - 24, 200 - 48, npos);
				}
			}
		}
		if(np->note & 8)
		{
			if(np->flags & EOF_NOTE_FLAG_HOPO)
			{
				if(np->flags & EOF_NOTE_FLAG_SP)
				{
					ocd3d_draw_bitmap(eof_window_3d->screen, p ? eof_image[EOF_IMAGE_NOTE_HWHITE_HIT] : eof_image[EOF_IMAGE_NOTE_HWHITE], xchart[3] - 24, 200 - 48, npos);
				}
				else
				{
					ocd3d_draw_bitmap(eof_window_3d->screen, p ? eof_image[EOF_IMAGE_NOTE_HBLUE_HIT] : eof_image[EOF_IMAGE_NOTE_HBLUE], xchart[3] - 24, 200 - 48, npos);
				}
			}
			else
			{
				if(np->flags & EOF_NOTE_FLAG_SP)
				{
					ocd3d_draw_bitmap(eof_window_3d->screen, p ? eof_image[EOF_IMAGE_NOTE_WHITE_HIT] : eof_image[EOF_IMAGE_NOTE_WHITE], xchart[3] - 24, 200 - 48, npos);
				}
				else
				{
					ocd3d_draw_bitmap(eof_window_3d->screen, p ? eof_image[EOF_IMAGE_NOTE_BLUE_HIT] : eof_image[EOF_IMAGE_NOTE_BLUE], xchart[3] - 24, 200 - 48, npos);
				}
			}
		}
		if(np->note & 16)
		{
			if(np->flags & EOF_NOTE_FLAG_HOPO)
			{
				if(np->flags & EOF_NOTE_FLAG_SP)
				{
					ocd3d_draw_bitmap(eof_window_3d->screen, p ? eof_image[EOF_IMAGE_NOTE_HWHITE_HIT] : eof_image[EOF_IMAGE_NOTE_HWHITE], xchart[4] - 24, 200 - 48, npos);
				}
				else
				{
					ocd3d_draw_bitmap(eof_window_3d->screen, p ? eof_image[EOF_IMAGE_NOTE_HPURPLE_HIT] : eof_image[EOF_IMAGE_NOTE_HPURPLE], xchart[4] - 24, 200 - 48, npos);
				}
			}
			else
			{
				if(np->flags & EOF_NOTE_FLAG_SP)
				{
					ocd3d_draw_bitmap(eof_window_3d->screen, p ? eof_image[EOF_IMAGE_NOTE_WHITE_HIT] : eof_image[EOF_IMAGE_NOTE_WHITE], xchart[4] - 24, 200 - 48, npos);
				}
				else
				{
					ocd3d_draw_bitmap(eof_window_3d->screen, p ? eof_image[EOF_IMAGE_NOTE_PURPLE_HIT] : eof_image[EOF_IMAGE_NOTE_PURPLE], xchart[4] - 24, 200 - 48, npos);
				}
			}
		}
	}
}

int eof_note_tail_draw_3d(EOF_NOTE * np, int p)
{
	int pos = eof_music_pos / eof_zoom_3d;
	int npos;
	int xchart[5] = {48, 48 + 56, 48 + 56 * 2, 48 + 56 * 3, 48 + 56 * 4};
	int point[8];
	int rz, ez;

	if(eof_lefty_mode)
	{
		xchart[0] = 48 + 56 * 4;
		xchart[1] = 48 + 56 * 3;
		xchart[2] = 48 + 56 * 2;
		xchart[3] = 48 + 56;
		xchart[4] = 48;
	}

	npos = -pos - 6 + np->pos / eof_zoom_3d + eof_av_delay / eof_zoom_3d;
	if(npos + np->length / eof_zoom_3d < -100)
	{
		return -1;
	}
	else if(npos > 600)
	{
		return 1;
	}
	if(eof_selected_track != EOF_TRACK_DRUM)
	{
		if(np->note & 1)
		{
			rz = npos < -100 ? -100 : npos + 10;
			ez = npos + np->length / eof_zoom_3d > 600 ? 600 : npos + np->length / eof_zoom_3d + 6;
			point[0] = ocd3d_project_x(xchart[0] - 10, rz);
			point[1] = ocd3d_project_y(200, rz);
			point[2] = ocd3d_project_x(xchart[0] - 10, ez);
			point[3] = ocd3d_project_y(200, ez);
			point[4] = ocd3d_project_x(xchart[0] + 10, ez);
			point[5] = ocd3d_project_y(200, ez);
			point[6] = ocd3d_project_x(xchart[0] + 10, rz);
			point[7] = ocd3d_project_y(200, rz);
			if(np->length > 10)
			{
				polygon(eof_window_3d->screen, 4, point, np->flags & EOF_NOTE_FLAG_SP ? (p ? eof_color_white : makecol(192, 192, 192)) : (p ? makecol(192, 255, 192) : eof_color_green));
			}
		}
		if(np->note & 2)
		{
			rz = npos < -100 ? -100 : npos + 10;
			ez = npos + np->length / eof_zoom_3d > 600 ? 600 : npos + np->length / eof_zoom_3d + 6;
			point[0] = ocd3d_project_x(xchart[1] - 10, rz);
			point[1] = ocd3d_project_y(200, rz);
			point[2] = ocd3d_project_x(xchart[1] - 10, ez);
			point[3] = ocd3d_project_y(200, ez);
			point[4] = ocd3d_project_x(xchart[1] + 10, ez);
			point[5] = ocd3d_project_y(200, ez);
			point[6] = ocd3d_project_x(xchart[1] + 10, rz);
			point[7] = ocd3d_project_y(200, rz);
			if(np->length > 10)
			{
				polygon(eof_window_3d->screen, 4, point, np->flags & EOF_NOTE_FLAG_SP ? (p ? eof_color_white : makecol(192, 192, 192)) : (p ? makecol(255, 192, 192) : eof_color_red));
			}
		}
		if(np->note & 4)
		{
			rz = npos < -100 ? -100 : npos + 10;
			ez = npos + np->length / eof_zoom_3d > 600 ? 600 : npos + np->length / eof_zoom_3d + 6;
			point[0] = ocd3d_project_x(xchart[2] - 10, rz);
			point[1] = ocd3d_project_y(200, rz);
			point[2] = ocd3d_project_x(xchart[2] - 10, ez);
			point[3] = ocd3d_project_y(200, ez);
			point[4] = ocd3d_project_x(xchart[2] + 10, ez);
			point[5] = ocd3d_project_y(200, ez);
			point[6] = ocd3d_project_x(xchart[2] + 10, rz);
			point[7] = ocd3d_project_y(200, rz);
			if(np->length > 10)
			{
				polygon(eof_window_3d->screen, 4, point, np->flags & EOF_NOTE_FLAG_SP ? (p ? eof_color_white : makecol(192, 192, 192)) : (p ? makecol(255, 255, 192) : eof_color_yellow));
			}
		}
		if(np->note & 8)
		{
			rz = npos < -100 ? -100 : npos + 10;
			ez = npos + np->length / eof_zoom_3d > 600 ? 600 : npos + np->length / eof_zoom_3d + 6;
			point[0] = ocd3d_project_x(xchart[3] - 10, rz);
			point[1] = ocd3d_project_y(200, rz);
			point[2] = ocd3d_project_x(xchart[3] - 10, ez);
			point[3] = ocd3d_project_y(200, ez);
			point[4] = ocd3d_project_x(xchart[3] + 10, ez);
			point[5] = ocd3d_project_y(200, ez);
			point[6] = ocd3d_project_x(xchart[3] + 10, rz);
			point[7] = ocd3d_project_y(200, rz);
			if(np->length > 10)
			{
				polygon(eof_window_3d->screen, 4, point, np->flags & EOF_NOTE_FLAG_SP ? (p ? eof_color_white : makecol(192, 192, 192)) : (p ? makecol(192, 192, 255) : eof_color_blue));
			}
		}
		if(np->note & 16)
		{
			rz = npos < -100 ? -100 : npos + 10;
			ez = npos + np->length / eof_zoom_3d > 600 ? 600 : npos + np->length / eof_zoom_3d + 6;
			point[0] = ocd3d_project_x(xchart[4] - 10, rz);
			point[1] = ocd3d_project_y(200, rz);
			point[2] = ocd3d_project_x(xchart[4] - 10, ez);
			point[3] = ocd3d_project_y(200, ez);
			point[4] = ocd3d_project_x(xchart[4] + 10, ez);
			point[5] = ocd3d_project_y(200, ez);
			point[6] = ocd3d_project_x(xchart[4] + 10, rz);
			point[7] = ocd3d_project_y(200, rz);
			if(np->length > 10)
			{
				polygon(eof_window_3d->screen, 4, point, np->flags & EOF_NOTE_FLAG_SP ? (p ? eof_color_white : makecol(192, 192, 192)) : (p ? makecol(255, 192, 255) : eof_color_purple));
			}
		}
	}
	return 0;
}

void eof_note_draw_catalog(EOF_NOTE * np, int p)
{
	int pos = eof_music_catalog_pos / eof_zoom;
	int npos;
	int ychart[5] = {20, 40, 60, 80, 100};
	int pcol = p == 1 ? makecol(255, 255, 255) : p == 2 ? makecol(224, 255, 224) : 0;

	if(p == 3)
	{
		pcol = eof_color_white;
	}

	if(eof_inverted_notes)
	{
		ychart[0] = eof_screen_layout.note_y[4];
		ychart[1] = eof_screen_layout.note_y[3];
		ychart[2] = eof_screen_layout.note_y[2];
		ychart[3] = eof_screen_layout.note_y[1];
		ychart[4] = eof_screen_layout.note_y[0];
	}
	else
	{
		ychart[0] = eof_screen_layout.note_y[0];
		ychart[1] = eof_screen_layout.note_y[1];
		ychart[2] = eof_screen_layout.note_y[2];
		ychart[3] = eof_screen_layout.note_y[3];
		ychart[4] = eof_screen_layout.note_y[4];
	}

	if(pos < 140)
	{
		npos = 20 + (np->pos) / eof_zoom;
	}
	else
	{
		npos = 20 - ((pos - 140)) + np->pos / eof_zoom;
	}

	if(eof_selected_track == EOF_TRACK_DRUM && eof_input_mode != EOF_INPUT_PIANO_ROLL && eof_input_mode != EOF_INPUT_REX)
	{
		vline(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[2] - eof_screen_layout.note_marker_size, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[2] + eof_screen_layout.note_marker_size, makecol(128, 128, 128));
		if(np->note & 1)
		{
			rectfill(eof_window_note->screen, npos - eof_screen_layout.note_kick_size, EOF_EDITOR_RENDER_OFFSET + 30, npos + eof_screen_layout.note_kick_size, EOF_EDITOR_RENDER_OFFSET + 120, eof_color_green);
			if(p)
			{
				rect(eof_window_note->screen, npos - eof_screen_layout.note_kick_size, EOF_EDITOR_RENDER_OFFSET + 30, npos + eof_screen_layout.note_kick_size, EOF_EDITOR_RENDER_OFFSET + 120, pcol);
			}
		}
		if(np->note & 2)
		{
			circlefill(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[0] + 10, eof_screen_layout.note_size, eof_color_red);
			circlefill(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[0] + 10, 2, makecol(255, 255, 255));
			if(p)
			{
				circle(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[0] + 10, eof_screen_layout.note_size, pcol);
			}
		}
		if(np->note & 4)
		{
			circlefill(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[1] + 10, eof_screen_layout.note_size, eof_color_yellow);
			circlefill(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[1] + 10, 2, makecol(255, 255, 255));
			if(p)
			{
				circle(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[1] + 10, eof_screen_layout.note_size, pcol);
			}
		}
		if(np->note & 8)
		{
			circlefill(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[2] + 10, eof_screen_layout.note_size, eof_color_blue);
			circlefill(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[2] + 10, 2, makecol(255, 255, 255));
			if(p)
			{
				circle(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[2] + 10, eof_screen_layout.note_size, pcol);
			}
		}
		if(np->note & 16)
		{
			circlefill(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[3] + 10, eof_screen_layout.note_size, eof_color_purple);
			circlefill(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[3] + 10, 2, makecol(255, 255, 255));
			if(p)
			{
				circle(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[3] + 10, eof_screen_layout.note_size, pcol);
			}
		}
	}
	else
	{

		if(p == 3)
		{
			vline(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[2] - eof_screen_layout.note_marker_size, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[2] + eof_screen_layout.note_marker_size, makecol(128, 128, 128));
			if(np->note & 1)
			{
				rectfill(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[0] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[0] + eof_screen_layout.note_tail_size, eof_color_green);
				rect(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[0] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[0] + eof_screen_layout.note_tail_size, pcol);
				circlefill(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[0], eof_screen_layout.note_size, eof_color_green);
				circlefill(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[0], eof_screen_layout.note_dot_size, makecol(255, 255, 255));
				circle(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[0], eof_screen_layout.note_size, pcol);
			}
			else if(eof_hover_note >= 0)
			{
				rect(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[0] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[0] + eof_screen_layout.note_tail_size, eof_color_gray);
				circle(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[0], eof_screen_layout.note_size, eof_color_gray);
			}
			if(np->note & 2)
			{
				rectfill(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[1] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[1] + eof_screen_layout.note_tail_size, eof_color_red);
				rect(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[1] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[1] + eof_screen_layout.note_tail_size, pcol);
				circlefill(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[1], eof_screen_layout.note_size, eof_color_red);
				circlefill(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[1], eof_screen_layout.note_dot_size, makecol(255, 255, 255));
				circle(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[1], eof_screen_layout.note_size, pcol);
			}
			else if(eof_hover_note >= 0)
			{
				rect(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[1] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[1] + eof_screen_layout.note_tail_size, eof_color_gray);
				circle(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[1], eof_screen_layout.note_size, eof_color_gray);
			}
			if(np->note & 4)
			{
				rectfill(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[2] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[2] + eof_screen_layout.note_tail_size, eof_color_yellow);
				rect(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[2] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[2] + eof_screen_layout.note_tail_size, pcol);
				circlefill(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[2], eof_screen_layout.note_size, eof_color_yellow);
				circlefill(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[2], eof_screen_layout.note_dot_size, makecol(255, 255, 255));
				circle(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[2], eof_screen_layout.note_size, pcol);
			}
			else if(eof_hover_note >= 0)
			{
				rect(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[2] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[2] + eof_screen_layout.note_tail_size, eof_color_gray);
				circle(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[2], eof_screen_layout.note_size, eof_color_gray);
			}
			if(np->note & 8)
			{
				rectfill(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[3] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[3] + eof_screen_layout.note_tail_size, eof_color_blue);
				rect(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[3] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[3] + eof_screen_layout.note_tail_size, pcol);
				circlefill(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[3], eof_screen_layout.note_size, eof_color_blue);
				circlefill(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[3], eof_screen_layout.note_dot_size, makecol(255, 255, 255));
				circle(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[3], eof_screen_layout.note_size, pcol);
			}
			else if(eof_hover_note >= 0)
			{
				rect(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[3] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[3] + eof_screen_layout.note_tail_size, eof_color_gray);
				circle(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[3], eof_screen_layout.note_size, eof_color_gray);
			}
			if(np->note & 16)
			{
				rectfill(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[4] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[4] + eof_screen_layout.note_tail_size, eof_color_purple);
				rect(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[4] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[4] + eof_screen_layout.note_tail_size, pcol);
				circlefill(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[4], eof_screen_layout.note_size, eof_color_purple);
				circlefill(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[4], eof_screen_layout.note_dot_size, makecol(255, 255, 255));
				circle(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[4], eof_screen_layout.note_size, pcol);
			}
			else if(eof_hover_note >= 0)
			{
				rect(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[4] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[4] + eof_screen_layout.note_tail_size, eof_color_gray);
				circle(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[4], eof_screen_layout.note_size, eof_color_gray);
			}
		}
		else
		{
			vline(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[2] - eof_screen_layout.note_marker_size, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.note_y[2] + eof_screen_layout.note_marker_size, makecol(128, 128, 128));
			if(np->note & 1)
			{
				rectfill(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[0] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[0] + eof_screen_layout.note_tail_size, eof_color_green);
				if(p)
				{
					rect(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[0] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[0] + eof_screen_layout.note_tail_size, pcol);
				}
				circlefill(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[0], eof_screen_layout.note_size, eof_color_green);
				circlefill(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[0], eof_screen_layout.note_dot_size, makecol(255, 255, 255));
				if(p)
				{
					circle(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[0], eof_screen_layout.note_size, pcol);
				}
			}
			if(np->note & 2)
			{
				rectfill(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[1] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[1] + eof_screen_layout.note_tail_size, eof_color_red);
				if(p)
				{
					rect(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[1] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[1] + eof_screen_layout.note_tail_size, pcol);
				}
				circlefill(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[1], eof_screen_layout.note_size, eof_color_red);
				circlefill(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[1], eof_screen_layout.note_dot_size, makecol(255, 255, 255));
				if(p)
				{
					circle(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[1], eof_screen_layout.note_size, pcol);
				}
			}
			if(np->note & 4)
			{
				rectfill(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[2] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[2] + eof_screen_layout.note_tail_size, eof_color_yellow);
				if(p)
				{
					rect(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[2] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[2] + eof_screen_layout.note_tail_size, pcol);
				}
				circlefill(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[2], eof_screen_layout.note_size, eof_color_yellow);
				circlefill(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[2], eof_screen_layout.note_dot_size, makecol(255, 255, 255));
				if(p)
				{
					circle(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[2], eof_screen_layout.note_size, pcol);
				}
			}
			if(np->note & 8)
			{
				rectfill(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[3] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[3] + eof_screen_layout.note_tail_size, eof_color_blue);
				if(p)
				{
					rect(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[3] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[3] + eof_screen_layout.note_tail_size, pcol);
				}
				circlefill(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[3], eof_screen_layout.note_size, eof_color_blue);
				circlefill(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[3], eof_screen_layout.note_dot_size, makecol(255, 255, 255));
				if(p)
				{
					circle(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[3], eof_screen_layout.note_size, pcol);
				}
			}
			if(np->note & 16)
			{
				rectfill(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[4] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[4] + eof_screen_layout.note_tail_size, eof_color_purple);
				if(p)
				{
					rect(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[4] - eof_screen_layout.note_tail_size, npos + np->length / eof_zoom, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[4] + eof_screen_layout.note_tail_size, pcol);
				}
				circlefill(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[4], eof_screen_layout.note_size, eof_color_purple);
				circlefill(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[4], eof_screen_layout.note_dot_size, makecol(255, 255, 255));
				if(p)
				{
					circle(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + ychart[4], eof_screen_layout.note_size, pcol);
				}
			}
		}
	}
}

void eof_lyric_draw_catalog(EOF_LYRIC * np, int p)
{
	int pos = eof_music_catalog_pos / eof_zoom;
	int npos;
	int nplace;
	int note_y;
	int native = 0;
	int ychart[5] = {20, 40, 60, 80, 100};
	int pcol = p == 1 ? makecol(255, 255, 255) : p == 2 ? makecol(224, 255, 224) : 0;
	int dcol = makecol(255, 255, 255);
	int ncol = 0;

	if(p == 3)
	{
		pcol = eof_color_white;
		dcol = eof_color_white;
	}

	ychart[0] = eof_screen_layout.note_y[0];
	ychart[1] = eof_screen_layout.note_y[1];
	ychart[2] = eof_screen_layout.note_y[2];
	ychart[3] = eof_screen_layout.note_y[3];
	ychart[4] = eof_screen_layout.note_y[4];

	if(pos < 140)
	{
		npos = 20 + (np->pos) / eof_zoom;
	}
	else
	{
		npos = 20 - ((pos - 140)) + np->pos / eof_zoom;
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
	if(p == 3)
	{
		vline(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.vocal_y - ((eof_screen_layout.vocal_view_size + 2) * eof_screen_layout.vocal_tail_size) / 2 - eof_screen_layout.note_marker_size, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.vocal_y - ((eof_screen_layout.vocal_view_size + 2) * eof_screen_layout.vocal_tail_size) / 2 + eof_screen_layout.note_marker_size, makecol(128, 128, 128));
		if(np->note != 0)
		{
			ncol = native ? eof_color_red : eof_color_green;
			rectfill(eof_window_note->screen, npos, note_y, npos + np->length / eof_zoom, note_y + eof_screen_layout.vocal_tail_size - 1, ncol);
			if(p)
			{
				rect(eof_window_note->screen, npos, note_y, npos + np->length / eof_zoom, note_y + eof_screen_layout.vocal_tail_size - 1, pcol);
			}
		}
		set_clip_rect(eof_window_note->screen, 0, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.lyric_y + 1, eof_window_note->screen->w, eof_window_note->screen->h);
		textprintf_ex(eof_window_note->screen, font, npos, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.lyric_y, eof_color_white, eof_color_black, "%s", np->text);
		set_clip_rect(eof_window_note->screen, 0, 0, eof_window_note->screen->w, eof_window_note->screen->h);
	}
	else
	{
		vline(eof_window_note->screen, npos, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.vocal_y - ((eof_screen_layout.vocal_view_size + 2) * eof_screen_layout.vocal_tail_size) / 2 - eof_screen_layout.note_marker_size, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.vocal_y - ((eof_screen_layout.vocal_view_size + 2) * eof_screen_layout.vocal_tail_size) / 2 + eof_screen_layout.note_marker_size, makecol(128, 128, 128));
		if(np->note != 0)
		{
			ncol = native ? eof_color_red : eof_color_green;
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
			rectfill(eof_window_note->screen, npos, note_y, npos + np->length / eof_zoom, note_y + eof_screen_layout.vocal_tail_size - 1, ncol);
			if(p)
			{
				rect(eof_window_note->screen, npos, note_y, npos + np->length / eof_zoom, note_y + eof_screen_layout.vocal_tail_size - 1, pcol);
			}
		}
		set_clip_rect(eof_window_note->screen, 0, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.lyric_y + 1, eof_window_note->screen->w, eof_window_note->screen->h);
		textprintf_ex(eof_window_note->screen, font, npos, EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.lyric_y, p ? eof_color_green : eof_color_white, eof_color_black, "%s", np->text);
		set_clip_rect(eof_window_note->screen, 0, 0, eof_window_note->screen->w, eof_window_note->screen->h);
	}
}

EOF_LYRIC_LINE *FindLyricLine_p(EOF_LYRIC * lp)
{
	int linectr;
	unsigned long lyricpos;

	if(eof_song == NULL)
		return NULL;
	lyricpos=lp->pos;

	for(linectr=0;linectr<eof_song->vocal_track->lines;linectr++)
	{
		if((eof_song->vocal_track->line[linectr].start_pos <= lyricpos) && (eof_song->vocal_track->line[linectr].end_pos >= lyricpos))
			return &(eof_song->vocal_track->line[linectr]);	//Line found, return it
	}

	return NULL;	//No such line found
}

EOF_LYRIC_LINE *FindLyricLine(int lyricnum)
{
	int linectr;
	unsigned long lyricpos;

	if(eof_song == NULL)
		return NULL;
	if(lyricnum >= eof_song->vocal_track->lyrics)
	{
		return NULL;
	}
	lyricpos=eof_song->vocal_track->lyric[lyricnum]->pos;

	for(linectr=0;linectr<eof_song->vocal_track->lines;linectr++)
	{
		if((eof_song->vocal_track->line[linectr].start_pos <= lyricpos) && (eof_song->vocal_track->line[linectr].end_pos >= lyricpos))
			return &(eof_song->vocal_track->line[linectr]);	//Line found, return it
	}

	return NULL;	//No such line found
}
