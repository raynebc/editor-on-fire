#include <allegro.h>
#include "main.h"
#include "editor.h"
#include "beat.h"
#include "song.h"
#include "legacy.h"

/* sort all notes according to position */
int eof_song_qsort_notes(const void * e1, const void * e2)
{
    EOF_NOTE ** thing1 = (EOF_NOTE **)e1;
    EOF_NOTE ** thing2 = (EOF_NOTE **)e2;

//	allegro_message("%d\n%d", (*thing1)->pos, (*thing2)->pos);
    if((*thing1)->pos < (*thing2)->pos)
	{
        return -1;
    }
    if((*thing1)->pos > (*thing2)->pos)
    {
        return 1;
    }

    // they are equal...
    return 0;
}


int eof_song_qsort_events(const void * e1, const void * e2)
{
	EOF_TEXT_EVENT ** thing1 = (EOF_TEXT_EVENT **)e1;
	EOF_TEXT_EVENT ** thing2 = (EOF_TEXT_EVENT **)e2;

	if((*thing1)->beat < (*thing2)->beat)
	{
		return -1;
	}
	else if((*thing1)->beat == (*thing2)->beat)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

int eof_song_qsort_lyrics(const void * e1, const void * e2)
{
    EOF_LYRIC ** thing1 = (EOF_LYRIC **)e1;
    EOF_LYRIC ** thing2 = (EOF_LYRIC **)e2;

    if((*thing1)->pos < (*thing2)->pos)
	{
        return -1;
    }
    if((*thing1)->pos > (*thing2)->pos)
    {
        return 1;
    }

    // they are equal...
    return 0;
}


EOF_SONG * eof_create_song(void)
{
	EOF_SONG * sp;
	int i;

	sp = malloc(sizeof(EOF_SONG));
	if(!sp)
	{
		return NULL;
	}
	sp->tags = malloc(sizeof(EOF_SONG_TAGS));
	if(!sp->tags)
	{
		return NULL;
	}
	ustrcpy(sp->tags->artist, "");
	ustrcpy(sp->tags->title, "");
	ustrcpy(sp->tags->frettist, "");
	ustrcpy(sp->tags->year, "");
	ustrcpy(sp->tags->loading_text, "");
	sp->tags->lyrics = 0;
	sp->tags->eighth_note_hopo = 0;
	sp->tags->ini_settings = 0;
	sp->tags->ogg[0].midi_offset = 0;
	sp->tags->ogg[0].modified = 0;
	ustrcpy(sp->tags->ogg[0].filename, "guitar.ogg");
	sp->tags->oggs = 1;
	sp->tags->revision = 0;
	sp->tracks = 0;
	sp->legacy_tracks = 0;
	sp->vocal_tracks = 0;
	sp->beats = 0;
	sp->text_events = 0;
	sp->catalog = malloc(sizeof(EOF_CATALOG));
	if(!sp->catalog)
	{
		return NULL;
	}
	sp->catalog->entries = 0;
	for(i = 0; i < EOF_MAX_BOOKMARK_ENTRIES; i++)
	{
		sp->bookmark_pos[i] = 0;
	}
	return sp;
}

void eof_destroy_song(EOF_SONG * sp)
{
	free(sp);
}

int eof_save_song(EOF_SONG * sp, const char * fn)
{
	PACKFILE * fp;
	int i, j, tl;
	char header[16] = {'E', 'O', 'F', 'S', 'O', 'N', 'F', 0};

	fp = pack_fopen(fn, "w");
	if(!fp)
	{
		return 0;
	}

	pack_fwrite(header, 16, fp);

	/* write revision number */
	pack_iputl(sp->tags->revision, fp);

	/* write song info */
	tl = ustrlen(sp->tags->artist);
	pack_iputw(tl, fp);
	pack_fwrite(sp->tags->artist, tl, fp);
	tl = ustrlen(sp->tags->title);
	pack_iputw(tl, fp);
	pack_fwrite(sp->tags->title, tl, fp);
	tl = ustrlen(sp->tags->frettist);
	pack_iputw(tl, fp);
	pack_fwrite(sp->tags->frettist, tl, fp);
	tl = ustrlen(sp->tags->year);
	pack_iputw(tl, fp);
	pack_fwrite(sp->tags->year, tl, fp);
	tl = ustrlen(sp->tags->loading_text);
	pack_iputw(tl, fp);
	pack_fwrite(sp->tags->loading_text, tl, fp);
	pack_putc(sp->tags->lyrics, fp);
	pack_putc(sp->tags->eighth_note_hopo, fp);

	/* write OGG data */
	pack_iputw(sp->tags->oggs, fp);
	for(i = 0; i < sp->tags->oggs; i++)
	{
		pack_fwrite(sp->tags->ogg[i].filename, 256, fp);
		pack_iputl(sp->tags->ogg[i].midi_offset, fp);
	}

	/* write INI settings */
	pack_iputw(sp->tags->ini_settings, fp);
	for(i = 0; i < sp->tags->ini_settings; i++)
	{
		pack_fwrite(sp->tags->ini_setting[i], 512, fp);
	}

	/* write beat info */
	pack_iputl(sp->beats, fp);
	for(i = 0; i < sp->beats; i++)
	{
		pack_iputl(sp->beat[i]->ppqn, fp);
		pack_iputl(sp->beat[i]->pos, fp);
		pack_iputl(sp->beat[i]->flags, fp);
	}

	/* write events info */
	pack_iputl(sp->text_events, fp);
	for(i = 0; i < sp->text_events; i++)
	{
		pack_fwrite(sp->text_event[i]->text, 256, fp);
		pack_iputl(sp->text_event[i]->beat, fp);
	}

	/* write legacy tracks */
	for(i = 0; i < sp->legacy_tracks; i++)
	{
		/* write solo sections */
		pack_iputw(sp->legacy_track[i]->solos, fp);
		for(j = 0; j < sp->legacy_track[i]->solos; j++)
		{
			pack_iputl(sp->legacy_track[i]->solo[j].start_pos, fp);
			pack_iputl(sp->legacy_track[i]->solo[j].end_pos, fp);
		}

		/* write star power sections */
		pack_iputw(sp->legacy_track[i]->star_power_paths, fp);
		for(j = 0; j < sp->legacy_track[i]->star_power_paths; j++)
		{
			pack_iputl(sp->legacy_track[i]->star_power_path[j].start_pos, fp);
			pack_iputl(sp->legacy_track[i]->star_power_path[j].end_pos, fp);
		}

		/* write notes */
		pack_iputl(sp->legacy_track[i]->notes, fp);
		for(j = 0; j < sp->legacy_track[i]->notes; j++)
		{
			pack_putc(sp->legacy_track[i]->note[j]->type, fp);
			pack_putc(sp->legacy_track[i]->note[j]->note, fp);
			pack_iputl(sp->legacy_track[i]->note[j]->pos, fp);
			pack_iputl(sp->legacy_track[i]->note[j]->length, fp);
			pack_putc(sp->legacy_track[i]->note[j]->flags, fp);
		}
	}

	/* write lyric tracks */
	for(i = 0; i < sp->vocal_tracks; i++)
	{
		pack_iputl(sp->vocal_track[i]->lyrics, fp);
		for(j = 0; j < sp->vocal_track[i]->lyrics; j++)
		{
			pack_putc(sp->vocal_track[i]->lyric[j]->note, fp);
			pack_iputl(sp->vocal_track[i]->lyric[j]->pos, fp);
			pack_iputl(sp->vocal_track[i]->lyric[j]->length, fp);
			pack_iputw(ustrlen(sp->vocal_track[i]->lyric[j]->text), fp);
			pack_fwrite(sp->vocal_track[i]->lyric[j]->text, ustrlen(sp->vocal_track[i]->lyric[j]->text), fp);
		}
		pack_iputl(sp->vocal_track[i]->lines, fp);
		for(j = 0; j < sp->vocal_track[i]->lines; j++)
		{
			pack_iputl(sp->vocal_track[i]->line[j].start_pos, fp);
			pack_iputl(sp->vocal_track[i]->line[j].end_pos, fp);
			pack_iputl(sp->vocal_track[i]->line[j].flags, fp);
		}
	}

	/* write bookmarks */
	for(i = 0; i < EOF_MAX_BOOKMARK_ENTRIES; i++)
	{
		pack_iputl(sp->bookmark_pos[i], fp);
	}

	/* write fret catalog */
	pack_iputl(sp->catalog->entries, fp);
	for(i = 0; i < sp->catalog->entries; i++)
	{
		pack_putc(sp->catalog->entry[i].track, fp);
		pack_putc(sp->catalog->entry[i].type, fp);
		pack_iputl(sp->catalog->entry[i].start_pos, fp);
		pack_iputl(sp->catalog->entry[i].end_pos, fp);
	}
	pack_fclose(fp);
	return 1;
}

int eof_load_song_pf(EOF_SONG * sp, PACKFILE * fp)
{
	int i, j, b, c, tl;

	/* read file revision number */
	sp->tags->revision = pack_igetl(fp);

	/* read song info */
	tl = pack_igetw(fp);
	pack_fread(sp->tags->artist, tl, fp);
	sp->tags->artist[tl] = 0;
	tl = pack_igetw(fp);
	pack_fread(sp->tags->title, tl, fp);
	sp->tags->title[tl] = 0;
	tl = pack_igetw(fp);
	pack_fread(sp->tags->frettist, tl, fp);
	sp->tags->frettist[tl] = 0;
	tl = pack_igetw(fp);
	pack_fread(sp->tags->year, tl, fp);
	sp->tags->year[tl] = 0;
	tl = pack_igetw(fp);
	pack_fread(sp->tags->loading_text, tl, fp);
	sp->tags->loading_text[tl] = 0;
	sp->tags->lyrics = pack_getc(fp);
	sp->tags->eighth_note_hopo = pack_getc(fp);

	/* read OGG data */
	sp->tags->oggs = pack_igetw(fp);
	for(i = 0; i < sp->tags->oggs; i++)
	{
		pack_fread(sp->tags->ogg[i].filename, 256, fp);
		sp->tags->ogg[i].midi_offset = pack_igetl(fp);
	}

	/* read INI settings */
	sp->tags->ini_settings = pack_igetw(fp);
	for(i = 0; i < sp->tags->ini_settings; i++)
	{
		pack_fread(sp->tags->ini_setting[i], 512, fp);
	}

	/* read beat info */
	b = pack_igetl(fp);
	if(!eof_song_resize_beats(sp, b))
	{
		return 0;
	}
	for(i = 0; i < b; i++)
	{
		sp->beat[i]->ppqn = pack_igetl(fp);
		sp->beat[i]->pos = pack_igetl(fp);
		sp->beat[i]->fpos = sp->beat[i]->pos;
		sp->beat[i]->flags = pack_igetl(fp);
	}

	/* read events info */
	b = pack_igetl(fp);
	eof_song_resize_text_events(sp, b);
	for(i = 0; i < b; i++)
	{
		pack_fread(sp->text_event[i]->text, 256, fp);
		sp->text_event[i]->beat = pack_igetl(fp);
	}

	/* read legacy tracks */
	for(i = 0; i < sp->legacy_tracks; i++)
	{
		/* read solo sections */
		sp->legacy_track[i]->solos = pack_igetw(fp);
		for(j = 0; j < sp->legacy_track[i]->solos; j++)
		{
			sp->legacy_track[i]->solo[j].start_pos = pack_igetl(fp);
			sp->legacy_track[i]->solo[j].end_pos = pack_igetl(fp);
		}
		if(sp->legacy_track[i]->solos < 0)
		{
			sp->legacy_track[i]->solos = 0;
		}

		/* read star power sections */
		sp->legacy_track[i]->star_power_paths = pack_igetw(fp);
		for(j = 0; j < sp->legacy_track[i]->star_power_paths; j++)
		{
			sp->legacy_track[i]->star_power_path[j].start_pos = pack_igetl(fp);
			sp->legacy_track[i]->star_power_path[j].end_pos = pack_igetl(fp);
		}
		if(sp->legacy_track[i]->star_power_paths < 0)
		{
			sp->legacy_track[i]->star_power_paths = 0;
		}

		/* read notes */
		b = pack_igetl(fp);
		eof_legacy_track_resize(sp->legacy_track[i], b);
		for(j = 0; j < b; j++)
		{
			sp->legacy_track[i]->note[j]->type = pack_getc(fp);
			sp->legacy_track[i]->note[j]->note = pack_getc(fp);
			sp->legacy_track[i]->note[j]->pos = pack_igetl(fp);
			sp->legacy_track[i]->note[j]->length = pack_igetl(fp);
			sp->legacy_track[i]->note[j]->flags = pack_getc(fp);
		}
	}

	/* read lyric tracks */
	for(i = 0; i < sp->vocal_tracks; i++)
	{
		b = pack_igetl(fp);
		eof_vocal_track_resize(sp->vocal_track[i], b);
		for(j = 0; j < b; j++)
		{
			sp->vocal_track[i]->lyric[j]->note = pack_getc(fp);
			sp->vocal_track[i]->lyric[j]->pos = pack_igetl(fp);
			sp->vocal_track[i]->lyric[j]->length = pack_igetl(fp);
			c = pack_igetw(fp);
			pack_fread(sp->vocal_track[i]->lyric[j]->text, c, fp);
			sp->vocal_track[i]->lyric[j]->text[c] = '\0';
		}
		sp->vocal_track[i]->lines = pack_igetl(fp);
		for(j = 0; j < sp->vocal_track[i]->lines; j++)
		{
			sp->vocal_track[i]->line[j].start_pos = pack_igetl(fp);
			sp->vocal_track[i]->line[j].end_pos = pack_igetl(fp);
			sp->vocal_track[i]->line[j].flags = pack_igetl(fp);
		}
	}

	/* read bookmarks */
	for(i = 0; i < EOF_MAX_BOOKMARK_ENTRIES; i++)
	{
		sp->bookmark_pos[i] = pack_igetl(fp);
	}

	/* read fret catalog */
	sp->catalog->entries = pack_igetl(fp);
	for(i = 0; i < sp->catalog->entries; i++)
	{
		sp->catalog->entry[i].track = pack_getc(fp);
		sp->catalog->entry[i].type = pack_getc(fp);
		sp->catalog->entry[i].start_pos = pack_igetl(fp);
		sp->catalog->entry[i].end_pos = pack_igetl(fp);
	}

	return 1;
}

EOF_SONG * eof_load_song(const char * fn)
{
	PACKFILE * fp;
	EOF_SONG * sp;
	char header[16] = {'E', 'O', 'F', 'S', 'O', 'N', 'H', 0};	//This header represents the current project format
	char rheader[16];
	int i;

	fp = pack_fopen(fn, "r");
	if(!fp)
	{
		return 0;
	}
	pack_fread(rheader, 16, fp);
	if(!ustricmp(rheader, header))
	{
		sp = eof_create_song();
		if(!sp)
		{
			return NULL;
		}
		if(!eof_load_song_pf(sp, fp))
		{
			return NULL;
		}
	}
	else
	{
		sp = eof_load_notes_legacy(fp, rheader[6]);
	}
	pack_fclose(fp);

	/* select correct OGG */
	if(sp)
	{
		for(i = 0; i < sp->tags->oggs; i++)
		{
			if(sp->tags->ogg[i].midi_offset == sp->beat[0]->pos)
			{
				eof_selected_ogg = i;
				break;
			}
		}
	}

	return sp;
}

EOF_NOTE * eof_track_add_note(EOF_LEGACY_TRACK * tp)
{
	if(tp->notes < EOF_MAX_NOTES)
	{
		tp->note[tp->notes] = malloc(sizeof(EOF_NOTE));
		if(tp->note[tp->notes])
		{
			memset(tp->note[tp->notes], 0, sizeof(EOF_NOTE));
			tp->notes++;
			return tp->note[tp->notes - 1];
		}
	}
	return NULL;
}

void eof_track_delete_note(EOF_LEGACY_TRACK * tp, int note)
{
	int i;

	if(note < tp->notes)
	{
		free(tp->note[note]);
		for(i = note; i < tp->notes - 1; i++)
		{
			tp->note[i] = tp->note[i + 1];
		}
		tp->notes--;
	}
}

void eof_track_sort_notes(EOF_LEGACY_TRACK * tp)
{
	qsort(tp->note, tp->notes, sizeof(EOF_NOTE *), eof_song_qsort_notes);
}

int eof_fixup_next_note(EOF_LEGACY_TRACK * tp, int note)
{
	int i;

	for(i = note + 1; i < tp->notes; i++)
	{
		if(tp->note[i]->type == tp->note[note]->type)
		{
			return i;
		}
	}
	return -1;
}

/* find and mark crazy notes, use during MIDI import */
void eof_track_find_crazy_notes(EOF_LEGACY_TRACK * tp)
{
	int i;
	int next;

	for(i = 0; i < tp->notes; i++)
	{
		next = eof_fixup_next_note(tp, i);
		if(next >= 0)
		{
			if(tp->note[i]->pos + tp->note[i]->length > tp->note[next]->pos)
			{
				tp->note[i]->flags |= EOF_NOTE_FLAG_CRAZY;
			}
		}
	}
}

void eof_track_fixup_notes(EOF_LEGACY_TRACK * tp, int sel)
{
	int i;
	int next;

	if(!sel)
	{
		if(eof_selection.current < tp->notes)
		{
			eof_selection.multi[eof_selection.current] = 0;
		}
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	for(i = tp->notes - 1; i >= 0; i--)
	{

		/* fix selections */
		if((tp->note[i]->type == eof_note_type) && (tp->note[i]->pos == eof_selection.current_pos))
		{
			eof_selection.current = i;
		}
		if((tp->note[i]->type == eof_note_type) && (tp->note[i]->pos == eof_selection.last_pos))
		{
			eof_selection.last = i;
		}

		/* delete certain notes */
		if((tp->note[i]->note == 0) || ((tp->note[i]->type < 0) || (tp->note[i]->type > 4)) || (tp->note[i]->pos < eof_song->tags->ogg[eof_selected_ogg].midi_offset) || (tp->note[i]->pos >= eof_music_length))
		{
			eof_track_delete_note(tp, i);
		}

		else
		{
			/* make sure there are no 0-length notes */
			if(tp->note[i]->length <= 0)
			{
				tp->note[i]->length = 1;
			}

			/* make sure note doesn't extend past end of song */
			if(tp->note[i]->pos + tp->note[i]->length >= eof_music_length)
			{
				tp->note[i]->length = eof_music_length - tp->note[i]->pos;
			}

			/* compare this note to the next one of the same type
			   to make sure they don't overlap */
			next = eof_fixup_next_note(tp, i);
			if(next >= 0)
			{
				if(tp->note[i]->pos == tp->note[next]->pos)
				{
					tp->note[i]->note |= tp->note[next]->note;
					eof_track_delete_note(tp, next);
				}
				else if(tp->note[i]->pos + tp->note[i]->length > tp->note[next]->pos - 1)
				{
					if(!(tp->note[i]->flags & EOF_NOTE_FLAG_CRAZY) || (tp->note[i]->note & tp->note[next]->note))
					{
						tp->note[i]->length = tp->note[next]->pos - tp->note[i]->pos - 1;
					}
				}
			}
		}
	}
	if(!sel)
	{
		if(eof_selection.current < tp->notes)
		{
			eof_selection.multi[eof_selection.current] = 1;
		}
	}

//Cleanup for pro drum notation
	if(eof_song && (eof_song->legacy_track[EOF_TRACK_DRUM] == tp))
	{	//If the track being cleaned is PART DRUMS
		for(i = 0; i < tp->notes; i++)
		{	//For each note in the drum track
			if(eof_check_flags_at_note_pos(tp,i,EOF_NOTE_FLAG_G_CYMBAL))
			{	//If any notes at this position are marked as a green cymbal
				eof_set_flags_at_note_pos(tp,i,EOF_NOTE_FLAG_G_CYMBAL,1);	//Mark all notes at this position as green cymbal
			}
			else
			{
				eof_set_flags_at_note_pos(tp,i,EOF_NOTE_FLAG_G_CYMBAL,0);	//Mark all notes at this position as green drum
			}
			if(eof_check_flags_at_note_pos(tp,i,EOF_NOTE_FLAG_Y_CYMBAL))
			{	//If any notes at this position are marked as a yellow cymbal
				eof_set_flags_at_note_pos(tp,i,EOF_NOTE_FLAG_Y_CYMBAL,1);	//Mark all notes at this position as yellow cymbal
			}
			else
			{
				eof_set_flags_at_note_pos(tp,i,EOF_NOTE_FLAG_Y_CYMBAL,0);	//Mark all notes at this position as yellow drum
			}
			if(eof_check_flags_at_note_pos(tp,i,EOF_NOTE_FLAG_B_CYMBAL))
			{	//If any notes at this position are marked as a blue cymbal
				eof_set_flags_at_note_pos(tp,i,EOF_NOTE_FLAG_B_CYMBAL,1);	//Mark all notes at this position as blue cymbal
			}
			else
			{
				eof_set_flags_at_note_pos(tp,i,EOF_NOTE_FLAG_B_CYMBAL,0);	//Mark all notes at this position as blue drum
			}
		}
	}
}

void eof_legacy_track_resize(EOF_LEGACY_TRACK * tp, int notes)
{
	int i;
	int oldnotes = tp->notes;
	if(notes > oldnotes)
	{
		for(i = oldnotes; i < notes; i++)
		{
			eof_track_add_note(tp);
		}
	}
	else if(notes < oldnotes)
	{
		for(i = notes; i < oldnotes; i++)
		{
			free(tp->note[i]);
			tp->notes--;
		}
	}
}

void eof_track_add_star_power(EOF_LEGACY_TRACK * tp, unsigned long start_pos, unsigned long end_pos)
{
	if(tp->star_power_paths < EOF_MAX_STAR_POWER)
	{	//If the maximum number of star power phrases for this track hasn't already been defined
		tp->star_power_path[tp->star_power_paths].start_pos = start_pos;
		tp->star_power_path[tp->star_power_paths].end_pos = end_pos;
		tp->star_power_paths++;
	}
}

void eof_track_delete_star_power(EOF_LEGACY_TRACK * tp, int index)
{
	int i;

	for(i = index; i < tp->star_power_paths - 1; i++)
	{
		memcpy(&tp->star_power_path[i], &tp->star_power_path[i + 1], sizeof(EOF_STAR_POWER_ENTRY));
	}
	tp->star_power_paths--;
}

void eof_track_add_solo(EOF_LEGACY_TRACK * tp, unsigned long start_pos, unsigned long end_pos)
{
	if(tp->solos < EOF_MAX_SOLOS)
	{	//If the maximum number of solo phrases for this track hasn't already been defined
		tp->solo[tp->solos].start_pos = start_pos;
		tp->solo[tp->solos].end_pos = end_pos;
		tp->solos++;
	}
}

void eof_track_delete_solo(EOF_LEGACY_TRACK * tp, int index)
{
	int i;

	for(i = index; i < tp->solos - 1; i++)
	{
		memcpy(&tp->solo[i], &tp->solo[i + 1], sizeof(EOF_SOLO_ENTRY));
	}
	tp->solos--;
}

EOF_LYRIC * eof_vocal_track_add_lyric(EOF_VOCAL_TRACK * tp)
{
	if(tp->lyrics < EOF_MAX_LYRICS)
	{
		tp->lyric[tp->lyrics] = malloc(sizeof(EOF_LYRIC));
		if(tp->lyric[tp->lyrics])
		{
			memset(tp->lyric[tp->lyrics], 0, sizeof(EOF_LYRIC));
			tp->lyrics++;
			return tp->lyric[tp->lyrics - 1];
		}
	}
	return NULL;
}

void eof_vocal_track_delete_lyric(EOF_VOCAL_TRACK * tp, int lyric)
{
	int i;

	if(lyric < tp->lyrics)
	{
		free(tp->lyric[lyric]);
		for(i = lyric; i < tp->lyrics - 1; i++)
		{
			tp->lyric[i] = tp->lyric[i + 1];
		}
		tp->lyrics--;
	}
}

void eof_vocal_track_sort_lyrics(EOF_VOCAL_TRACK * tp)
{
	qsort(tp->lyric, tp->lyrics, sizeof(EOF_LYRIC *), eof_song_qsort_lyrics);
}

int eof_fixup_next_lyric(EOF_VOCAL_TRACK * tp, int lyric)
{
	int i;

	for(i = lyric + 1; i < tp->lyrics; i++)
	{
		return i;
	}
	return -1;
}

void eof_vocal_track_fixup_lyrics(EOF_VOCAL_TRACK * tp, int sel)
{
	int i, j;
	int lc;
	int next;

	if(!sel)
	{
		if(eof_selection.current < tp->lyrics)
		{
			eof_selection.multi[eof_selection.current] = 0;
		}
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	for(i = tp->lyrics - 1; i >= 0; i--)
	{

		/* fix selections */
		if(tp->lyric[i]->pos == eof_selection.current_pos)
		{
			eof_selection.current = i;
		}
		if(tp->lyric[i]->pos == eof_selection.last_pos)
		{
			eof_selection.last = i;
		}

		/* delete certain notes */
		if((tp->lyric[i]->pos < eof_song->tags->ogg[eof_selected_ogg].midi_offset) || (tp->lyric[i]->pos >= eof_music_length))
		{
			eof_vocal_track_delete_lyric(tp, i);
		}

		else
		{

			/* make sure there are no 0-length notes */
			if(tp->lyric[i]->length <= 0)
			{
				tp->lyric[i]->length = 1;
			}

			/* make sure note doesn't extend past end of song */
			if(tp->lyric[i]->pos + tp->lyric[i]->length >= eof_music_length)
			{
				tp->lyric[i]->length = eof_music_length - tp->lyric[i]->pos;
			}

			/* compare this note to the next one of the same type
			   to make sure they don't overlap */
			next = eof_fixup_next_lyric(tp, i);
			if(next >= 0)
			{
				if(tp->lyric[i]->pos == tp->lyric[next]->pos)
				{
					eof_vocal_track_delete_lyric(tp, next);
				}
				else if(tp->lyric[i]->pos + tp->lyric[i]->length >= tp->lyric[next]->pos - 1)
				{	//If this lyric does not end at least 1ms before the next lyric starts
					tp->lyric[i]->length = tp->lyric[next]->pos - tp->lyric[i]->pos - 1 - 1;	//Subtract one more to ensure padding
					if(tp->lyric[i]->length <= 0)
						tp->lyric[i]->length = 1;	//Ensure that this doesn't cause the length to be invalid
				}
			}
		}

		/* validate lyric text, ie. freestyle marker */
		eof_fix_lyric(tp,i);
	}

	/* make sure no lines overlap */
	for(i = 0; i < tp->lines; i++)
	{
		for(j = 0; j < tp->lines; j++)
		{
			if(j != i)
			{
				if((tp->line[i].end_pos >= tp->line[j].start_pos) && (tp->line[i].start_pos <= tp->line[j].end_pos))
				{
					tp->line[i].end_pos = tp->line[j].start_pos - 1;
				}
			}
		}
	}
	/* delete empty lines */
	for(i = tp->lines - 1; i >= 0; i--)
	{
		lc = 0;
		for(j = 0; j < tp->lyrics; j++)
		{
			if((tp->lyric[j]->pos >= tp->line[i].start_pos) && (tp->lyric[j]->pos <= tp->line[i].end_pos))
			{
				lc++;
			}
		}
		if(lc <= 0)
		{
			eof_vocal_track_delete_line(tp, i);
		}
	}
	if(!sel)
	{
		if(eof_selection.current < tp->lyrics)
		{
			eof_selection.multi[eof_selection.current] = 1;
		}
	}
}

void eof_vocal_track_resize(EOF_VOCAL_TRACK * tp, int lyrics)
{
	int i;
	int oldlyrics = tp->lyrics;
	if(lyrics > oldlyrics)
	{
		for(i = oldlyrics; i < lyrics; i++)
		{
			eof_vocal_track_add_lyric(tp);
		}
	}
	else if(lyrics < oldlyrics)
	{
		for(i = lyrics; i < oldlyrics; i++)
		{
			free(tp->lyric[i]);
			tp->lyrics--;
		}
	}
}

void eof_vocal_track_add_line(EOF_VOCAL_TRACK * tp, unsigned long start_pos, unsigned long end_pos)
{
	if(tp->lines < EOF_MAX_LYRIC_LINES)
	{	//If the maximum number of lyric phrases hasn't already been defined
		tp->line[tp->lines].start_pos = start_pos;
		tp->line[tp->lines].end_pos = end_pos;
		tp->line[tp->lines].flags = 0;	//Ensure that a blank flag status is initialized
		tp->lines++;
	}
}

void eof_vocal_track_delete_line(EOF_VOCAL_TRACK * tp, int index)
{
	int i;

	if(tp->lines == 0)	//If there are no lyric line phrases to delete
		return;			//Cancel this to avoid problems

	for(i = index; i < tp->lines - 1; i++)
	{
		memcpy(&tp->line[i], &tp->line[i + 1], sizeof(EOF_LYRIC_LINE));
	}
	tp->lines--;
}

EOF_BEAT_MARKER * eof_song_add_beat(EOF_SONG * sp)
{
	if((sp == NULL) || (sp->beat == NULL))
		return NULL;

	if(sp->beats < EOF_MAX_BEATS)
	{	//If the maximum number of beats hasn't already been defined
		sp->beat[sp->beats] = malloc(sizeof(EOF_BEAT_MARKER));
		if(sp->beat[sp->beats] != NULL)
		{
			sp->beat[sp->beats]->pos = 0;
			sp->beat[sp->beats]->ppqn = 500000;
			sp->beat[sp->beats]->flags = 0;
			sp->beat[sp->beats]->midi_pos = 0;
			sp->beat[sp->beats]->fpos = 0.0;
			sp->beats++;
			return sp->beat[sp->beats - 1];
		}
	}

	return NULL;
}

void eof_song_delete_beat(EOF_SONG * sp, int beat)
{
	int i;

	free(sp->beat[beat]);
	for(i = beat; i < sp->beats - 1; i++)
	{
		sp->beat[i] = sp->beat[i + 1];
	}
	sp->beats--;
}

int eof_song_resize_beats(EOF_SONG * sp, int beats)
{
	int i;
	int oldbeats = sp->beats;
	if(beats > oldbeats)
	{
		for(i = oldbeats; i < beats; i++)
		{
			if(!eof_song_add_beat(sp))
			{
				return 0;	//Return failure
			}
		}
	}
	else if(beats < oldbeats)
	{
		for(i = beats; i < oldbeats; i++)
		{
			free(sp->beat[i]);
			sp->beats--;
		}
	}
	return 1;	//Return success
}

EOF_TEXT_EVENT * eof_song_add_text_event(EOF_SONG * sp, int beat, char * text)
{
	if(sp->text_events < EOF_MAX_TEXT_EVENTS)
	{	//If the maximum number of text events hasn't already been defined
		sp->text_event[sp->text_events] = malloc(sizeof(EOF_TEXT_EVENT));
		if(sp->text_event[sp->text_events])
		{
			ustrcpy(sp->text_event[sp->text_events]->text, text);
			sp->text_event[sp->text_events]->beat = beat;
			sp->text_events++;
		}
		else
		{
			return NULL;	//Return failure
		}
	}
	return sp->text_event[sp->text_events-1];	//Return successfully created text event
}

void eof_song_delete_text_event(EOF_SONG * sp, int event)
{
	int i;
	free(sp->text_event[event]);
	for(i = event; i < sp->text_events - 1; i++)
	{
		sp->text_event[i] = sp->text_event[i + 1];
	}
	sp->text_events--;
}

void eof_song_move_text_events(EOF_SONG * sp, int beat, int offset)
{
	int i;

	for(i = 0; i < sp->text_events; i++)
	{
		if(sp->text_event[i]->beat >= beat)
		{
			sp->beat[sp->text_event[i]->beat]->flags = sp->beat[sp->text_event[i]->beat]->flags & EOF_BEAT_FLAG_ANCHOR;
			sp->text_event[i]->beat += offset;
			sp->beat[sp->text_event[i]->beat]->flags |= EOF_BEAT_FLAG_EVENTS;
		}
	}
}

int eof_song_resize_text_events(EOF_SONG * sp, int events)
{
	int i;
	int oldevents = sp->text_events;
	if(events > oldevents)
	{
		for(i = oldevents; i < events; i++)
		{
			if(!eof_song_add_text_event(sp, 0, ""))
			{
				return 0;	//Return failure
			}
		}
	}
	else if(events < oldevents)
	{
		for(i = events; i < oldevents; i++)
		{
			free(sp->text_event[i]);
			sp->text_events--;
		}
	}
	return 1;	//Return succes
}

void eof_sort_events(void)
{
	qsort(eof_song->text_event, eof_song->text_events, sizeof(EOF_TEXT_EVENT *), eof_song_qsort_events);
}

/* make sure notes don't overlap */
void eof_fixup_notes(void)
{
	int i, j;

	if(eof_selection.current < eof_song->legacy_track[eof_song->track[eof_selected_track]->tracknum]->notes)
	{
		eof_selection.multi[eof_selection.current] = 0;
	}
	eof_selection.current = EOF_MAX_NOTES - 1;

	/* fix beats */
	if(eof_song->beat[0]->pos != eof_song->tags->ogg[eof_selected_ogg].midi_offset)
	{
		for(i = 0; i < eof_song->beats; i++)
		{
			eof_song->beat[i]->pos += eof_song->tags->ogg[eof_selected_ogg].midi_offset - eof_song->beat[0]->pos;
		}
	}

	for(j = 0; j < EOF_LEGACY_TRACKS_MAX; j++)
	{
		eof_track_fixup_notes(eof_song->legacy_track[j], j == eof_selected_track);
	}
}

void eof_sort_notes(void)
{
	int j;

	for(j = 0; j < EOF_LEGACY_TRACKS_MAX; j++)
	{
		eof_track_sort_notes(eof_song->legacy_track[j]);
	}
}

void eof_detect_difficulties(EOF_SONG * sp)
{
	int i;

	memset(eof_note_difficulties, 0, sizeof(int) * 4);
	eof_note_type_name[0][0] = ' ';
	eof_note_type_name[1][0] = ' ';
	eof_note_type_name[2][0] = ' ';
	eof_note_type_name[3][0] = ' ';
	eof_note_type_name[4][0] = ' ';
	eof_vocal_tab_name[0][0] = ' ';
	if(eof_selected_track != EOF_TRACK_VOCALS)
	{
		for(i = 0; i < sp->legacy_track[sp->track[eof_selected_track]->tracknum]->notes; i++)
		{
			if((sp->legacy_track[sp->track[eof_selected_track]->tracknum]->note[i]->type >= 0) && (sp->legacy_track[sp->track[eof_selected_track]->tracknum]->note[i]->type < 5))
			{
				eof_note_difficulties[(int)sp->legacy_track[sp->track[eof_selected_track]->tracknum]->note[i]->type] = 1;
				eof_note_type_name[(int)sp->legacy_track[sp->track[eof_selected_track]->tracknum]->note[i]->type][0] = '*';
			}
		}
	}
	else
	{
		if(sp->vocal_track[0]->lyrics)
		{
			eof_vocal_tab_name[0][0] = '*';
		}
	}
}

int eof_lyric_is_freestyle(EOF_VOCAL_TRACK * tp, unsigned long lyricnumber)
{
	if((tp == NULL) || (lyricnumber >= tp->lyrics))
		return -1;	//Return error

	return eof_is_freestyle(tp->lyric[lyricnumber]->text);
}

int eof_is_freestyle(char *ptr)
{
	unsigned long ctr=0;
	char c=0;

	if(ptr == NULL)
		return -1;	//Return error

	for(ctr=0;ctr<EOF_MAX_LYRIC_LENGTH;ctr++)
	{
		c=ptr[ctr];

		if(c == '\0')	//End of string
			break;

		if((c == '#') || (c == '^'))
			return 1;	//Return is freestyle
	}

	return 0;	//Return not freestyle
}

void eof_set_freestyle(char *ptr, char status)
{
	unsigned long ctr=0,ctr2=0;
	char c=0,style=0;

	if(ptr == NULL)
		return;	//Return if input is invalid

	for(ctr=0;ctr<EOF_MAX_LYRIC_LENGTH;ctr++)
	{
		c=ptr[ctr];

		if((c != '#') && (c != '^'))	//If this is not a freestyle character
		{
			ptr[ctr2]=c;				//keep it, otherwise it will be overwritten by the rest of the lyric text
			if(c == '\0')				//If the end of the string was just parsed
				break;					//Exit from loop
			ctr2++;						//Increment destination index into lyric string
		}
		else
		{
			if(!style)
				style=c;				//Remember this lyric's first freestyle character
		}
	}

	if(ctr == EOF_MAX_LYRIC_LENGTH)
	{	//If the lyric is not properly NULL terminated
		ptr[EOF_MAX_LYRIC_LENGTH]='\0';	//Truncate the string at its max length
		ctr2=EOF_MAX_LYRIC_LENGTH;
	}

//At this point, ctr2 is the NULL terminator's index into the string
	if(status)
	{
		if(!style)			//If the original string didn't have a particular freestyle marker
			style='#';		//Default to using '#'

		if(ctr2 < EOF_MAX_LYRIC_LENGTH)	//If there is room to append the pound character
		{
			ptr[ctr2]=style;
			ptr[ctr2+1]='\0';
		}
		else if(ctr2 > 0)		//If one byte of the string can be truncated to write the pound character
		{
			ptr[ctr2-1]=style;
		}
		else					//special case: EOF_MAX_LYRIC_LENGTH is 0 for some reason
			return;				//Don't do anything
	}
}

void eof_fix_lyric(EOF_VOCAL_TRACK * tp, unsigned long lyricnumber)
{
	int result=0;

	if((tp == NULL) || (lyricnumber >= tp->lyrics))
		return;	//Return if input is invalid

	result=eof_lyric_is_freestyle(tp,lyricnumber);
	if(result >= 0)												//As long as there wasn't an error with the lyric
		eof_set_freestyle(tp->lyric[lyricnumber]->text,result);	//Rewrite lyric (if the NULL was missing, the string will be corrected)
}

void eof_toggle_freestyle(EOF_VOCAL_TRACK * tp, unsigned long lyricnumber)
{
	if((tp == NULL) || (lyricnumber >= tp->lyrics))
		return;	//Return if input is invalid

	if(eof_lyric_is_freestyle(tp,lyricnumber))				//If the lyric is freestyle
		eof_set_freestyle(tp->lyric[lyricnumber]->text,0);	//Remove its freestyle character(s)
	else													//Otherwise
		eof_set_freestyle(tp->lyric[lyricnumber]->text,1);	//Rewrite as freestyle
}

/* function to convert a MIDI-style tick to msec time */
int eof_song_tick_to_msec(EOF_SONG * sp, int track, unsigned long tick)
{
	int beat; // which beat 'tick' lies in
	double curpos = sp->tags->ogg[eof_selected_ogg].midi_offset;
	double portion;
	int i;

	beat = tick / sp->resolution;
	portion = (double)((tick % sp->resolution)) / (double)(sp->resolution);

	/* calculate position up to the beat 'tick' lies in */
	for(i = 0; i < beat; i++)
	{
		curpos += (double)60000.0 / ((double)60000000.0 / (double)sp->beat[i]->ppqn);
	}

	/* add the time from the beat marker to 'tick' */
	curpos += ((double)60000.0 / ((double)60000000.0 / (double)sp->beat[beat]->ppqn)) * portion;

	return curpos + 0.5;
}

/* function to convert msec time to a MIDI-style tick */
int eof_song_msec_to_tick(EOF_SONG * sp, int track, unsigned long msec)
{
	int beat; // which beat 'msec' lies in
	int beat_tick;
	int portion;
	double beat_start, beat_end, beat_length;

	/* figure out which beat we are in */
	beat = eof_get_beat(sp, msec);
	if(beat < 0)
	{
		return -1;
	}

	/* get the tick for the beat we are in */
	beat_tick = beat * sp->resolution;

	/* find which tick of the beat is closest to 'msec' */
	beat_start = sp->beat[beat]->fpos;
	beat_end = beat_start + (double)60000.0 / ((double)60000000.0 / (double)sp->beat[beat]->ppqn);
	beat_length = beat_end - beat_start;
	portion = (((double)msec - beat_start) / beat_length) * ((double)sp->resolution) + 0.5;

	return beat_tick + portion;
}


char eof_check_flags_at_note_pos(EOF_LEGACY_TRACK *tp,unsigned notenum,char flag)
{
	unsigned long ctr,ctr2;
	char match = 0;

	if((tp == NULL) || (notenum >= tp->notes))
		return 0;

//Find the first note at the specified note's position
	for(ctr = 0; ctr < tp->notes; ctr++)
	{	//For each note in the track
		if(tp->note[ctr]->pos == tp->note[notenum]->pos)
		{	//If the note is after the specified note's position
			match = 1;
			break;
		}
	}

//Check all notes at its position for the presence of the specified flag
	if(match)
	{
		match = 0;
		for(ctr2 = ctr; ctr2 < tp->notes; ctr2++)
		{	//For each note starting with the one found above
			if(tp->note[ctr2]->pos > tp->note[notenum]->pos)
				break;	//If there are no more notes at that position, stop looking
			if(tp->note[ctr2]->flags & flag)
			{	//If the note has the specified flag
				match = 1;
				break;
			}
		}
	}

	return match;
}

void eof_set_flags_at_note_pos(EOF_LEGACY_TRACK *tp,unsigned notenum,char flag,char operation)
{
	unsigned long ctr,ctr2;
	char match = 0;

	if((tp == NULL) || (notenum >= tp->notes))
		return;

//Find the first note at the specified note's position
	for(ctr = 0; ctr < tp->notes; ctr++)
	{	//For each note in the track
		if(tp->note[ctr]->pos == tp->note[notenum]->pos)
		{	//If the note is after the specified note's position
			match = 1;
			break;
		}
	}

//Check all notes at its position for the presence of the specified flag
	if(match)
	{
		for(ctr2 = ctr; ctr2 < tp->notes; ctr2++)
		{	//For each note starting with the one found above
			if(tp->note[ctr2]->pos > tp->note[notenum]->pos)
				break;	//If there are no more notes at that position, stop looking
			if(operation == 0)
			{	//If the calling function indicated to clear the flag
				tp->note[ctr2]->flags &= (~flag);
			}
			else if(operation == 1)
			{	//If the calling function indicated to set the flag
				tp->note[ctr2]->flags |= flag;
			}
			else if(operation == 2)
			{	//If the calling function indicated to toggle the flag
				tp->note[ctr2]->flags ^= flag;
			}
		}
	}
}

int eof_load_song_string_pf(char *buffer, PACKFILE *fp, unsigned long buffersize)
{
	unsigned long ctr=0,stringsize=0;
	int inputc=0;

	if((fp == NULL) || ((buffer == NULL) && (buffersize != 0)))
		return 1;	//Return error

	stringsize = pack_igetw(fp);

	for(ctr = 0; ctr < stringsize; ctr++)
	{
		inputc = pack_getc(fp);
		if(inputc == EOF)		//If the end of file was reached
			break;			//stop reading characters
		if(ctr + 1 < buffersize)	//If the buffer can accommodate this character
			buffer[ctr] = inputc;	//store it
	}

	if(buffersize != 0)
	{	//Skip the termination of the buffer if none was presented
		if(ctr < buffersize)
			buffer[ctr] = '\0';		//NULL terminate the buffer
		else
			buffer[buffersize-1] = '\0';	//NULL terminate the end of the buffer
	}

	return 0;	//Return success
}

int eof_song_add_track(EOF_SONG * sp, int trackformat)
{
	EOF_LEGACY_TRACK *ptr = NULL;
	EOF_VOCAL_TRACK *ptr2 = NULL;
	EOF_TRACK_ENTRY *ptr3 = NULL;
	unsigned long count=0;

	if(sp == NULL)
		return 0;	//Return error

	if(sp->tracks <= EOF_TRACKS_MAX)
	{
		ptr3 = malloc(sizeof(EOF_TRACK_ENTRY));
		if(ptr3 == NULL)
			return 0;	//Return error

		//Insert new track structure in the appropriate track type array
		switch(trackformat)
		{
			case EOF_LEGACY_TRACK_FORMAT:
				count = sp->legacy_tracks;
				ptr = malloc(sizeof(EOF_LEGACY_TRACK));
				if(ptr == NULL)
					return 0;	//Return error
				ptr->notes = 0;
				ptr->solos = 0;
				ptr->star_power_paths = 0;
				sp->legacy_track[sp->legacy_tracks] = ptr;
				sp->legacy_tracks++;
			break;
			case EOF_VOCAL_TRACK_FORMAT:
				count = sp->vocal_tracks;
				ptr2 = malloc(sizeof(EOF_VOCAL_TRACK));
				if(ptr2 == NULL)
					return 0;	//Return error
				ptr2->lyrics = 0;
				ptr2->lines = 0;
				sp->vocal_track[sp->vocal_tracks] = ptr2;
				sp->vocal_tracks++;
			break;
			case EOF_PRO_KEYS_TRACK_FORMAT:
			break;
			case EOF_PRO_GUITAR_TRACK_FORMAT:
			break;
			case EOF_PRO_VARIABLE_LEGACY_TRACK_FORMAT:
			break;
			default:
			return 0;	//Return error
		}

		//Insert new track structure in the main track array
		ptr3->tracknum = count;
		ptr3->trackformat = trackformat;
		if(sp->tracks == 0)
		{	//If this is the first track being added, ensure that sp->track[0] is inserted
			sp->track[0] = NULL;
			sp->tracks++;
		}
		sp->track[sp->tracks] = ptr3;
		sp->tracks++;
	}
	return 1;	//Return success
}

int eof_song_delete_track(EOF_SONG * sp, unsigned long track)
{
	unsigned long ctr;

	if((sp == NULL) || (track >= sp->tracks) || (sp->track[track] == NULL))
		return 0;	//Return error

	//Remove the track from the appropriate track type array
	switch(sp->track[track]->trackformat)
	{
		case EOF_LEGACY_TRACK_FORMAT:
			if((sp->track[track]->tracknum >= sp->legacy_tracks) || (sp->legacy_track[sp->track[track]->tracknum] == NULL))
				return 0;	//Cannot remove a legacy track that doesn't exist
			free(sp->legacy_track[sp->track[track]->tracknum]);
			for(ctr = sp->track[track]->tracknum; ctr + 1 < sp->legacy_tracks; ctr++)
			{
				sp->legacy_track[ctr] = sp->legacy_track[ctr + 1];
			}
			sp->legacy_tracks--;
		break;
		case EOF_VOCAL_TRACK_FORMAT:
			if((sp->track[track]->tracknum >= sp->vocal_tracks) || (sp->vocal_track[sp->track[track]->tracknum] == NULL))
				return 0;	//Cannot remove a vocal track that doesn't exist
			free(sp->vocal_track[sp->track[track]->tracknum]);
			for(ctr = sp->track[track]->tracknum; ctr + 1 < sp->vocal_tracks; ctr++)
			{
				sp->vocal_track[ctr] = sp->vocal_track[ctr + 1];
			}
			sp->vocal_tracks--;
		break;
		case EOF_PRO_KEYS_TRACK_FORMAT:
		break;
		case EOF_PRO_GUITAR_TRACK_FORMAT:
		break;
		case EOF_PRO_VARIABLE_LEGACY_TRACK_FORMAT:
		break;
	}

	//Remove the track from the main track array
	free(sp->track[track]);
	for(ctr = track; ctr + 1 < sp->tracks; ctr++)
	{
		sp->track[ctr] = sp->track[ctr + 1];
	}
	return 1;	//Return success
}

int eof_load_song_pf_new(EOF_SONG * sp, PACKFILE * fp)
{
	unsigned char inputc;
	unsigned long inputl,count,ctr;
	unsigned long track_count,track_ctr,track_format,track_behavior,track_type;
	unsigned long section_type_count,section_type_ctr,section_type,section_count,section_ctr,section_start,section_end;
	unsigned long custom_data_count,custom_data_ctr,custom_data_size;
	unsigned long bookmarkctr = 0;

	#define EOFNUMINISTRINGTYPES 12
	char *inistringbuffer[EOFNUMINISTRINGTYPES]={NULL,NULL,sp->tags->artist,sp->tags->title,sp->tags->frettist,NULL,sp->tags->year,sp->tags->loading_text,NULL,NULL,NULL,NULL};
	unsigned long inistringbuffersize[12]={0,0,256,256,256,0,32,512,0,0,0,0};
		//Store the buffer information of each of the 12 INI strings to simplify the loading code
		//This buffer can be updated without redesigning the entire load function, just add logic for loading the new string type

	#define EOFNUMINIBOOLEANTYPES 6
	char *inibooleanbuffer[EOFNUMINIBOOLEANTYPES]={NULL,&sp->tags->lyrics,&sp->tags->eighth_note_hopo,NULL,NULL,NULL};
		//Store the pointers to each of the 5 boolean type INI settings (number 0 is reserved) to simplify the loading code

	#define EOFNUMININUMBERTYPES 5
	unsigned long *ininumberbuffer[EOFNUMININUMBERTYPES]={NULL,NULL,NULL,NULL,NULL};
		//Store the pointers to each of the 5 number type INI settings (number 0 is reserved) to simplify the loading code


	if((sp == NULL) || (fp == NULL))
		return 0;	//Return failure

	/* read chart properties */
	sp->tags->revision = pack_igetl(fp);	//Read file revision number
	inputc = pack_getc(fp);			//Read timing format
	if(inputc == 1)
	{
		allegro_message("Error: Delta timing is not yet supported");
		return 0;	//Return failure
	}
	pack_igetl(fp);		//Read time division (not supported yet)

	/* read song properties */
	count = pack_igetw(fp);			//Read the number of INI strings
	for(ctr=0; ctr<count; ctr++)
	{	//For each INI string in the project
		inputc = pack_getc(fp);		//Read the type of INI string
		if((inputc < EOFNUMINISTRINGTYPES) && (inistringbuffer[inputc] != NULL))
		{	//If EOF can store the INI setting natively
			eof_load_song_string_pf(inistringbuffer[inputc],fp,inistringbuffersize[inputc]);
		}
		else
		{	//If it is not natively supported or is otherwise a custom INI setting
			if(sp->tags->ini_settings < EOF_MAX_INI_SETTINGS)
			{	//If this INI setting can be stored
				eof_load_song_string_pf(sp->tags->ini_setting[sp->tags->ini_settings],fp,sizeof(sp->tags->ini_setting[0]));
				sp->tags->ini_settings++;
			}
		}
	}

	count = pack_igetw(fp);			//Read the number of INI booleans
	for(ctr=0; ctr<count; ctr++)
	{	//For each INI boolean in the project
		inputc = pack_getc(fp);		//Read the type of INI boolean
		if(((inputc & 0x7F) < EOFNUMINIBOOLEANTYPES) && (inibooleanbuffer[inputc] != NULL))
		{	//Mask out the true/false bit to determine if it is a supported boolean setting
			*inibooleanbuffer[(inputc & 0x7F)] = (inputc & 0xF0);	//Store the true/false status into the appropriate project variable
		}
	}

	count = pack_igetw(fp);			//Read the number of INI numbers
	for(ctr=0; ctr<count; ctr++)
	{	//For each INI number in the project
		inputc = pack_getc(fp);		//Read the type of INI number
		inputl = pack_igetl(fp);	//Read the INI number
		if((inputc < EOFNUMININUMBERTYPES) && (ininumberbuffer[inputc] != NULL))
		{	//If EOF can store the INI number natively
			*ininumberbuffer[inputc] = inputl;
		}
	}

	/* read chart data */
	count = pack_igetw(fp);			//Read the number of OGG profiles
	sp->tags->oggs = 0;
	for(ctr=0; ctr<count; ctr++)
	{	//For each OGG profile in the project
		if(sp->tags->oggs < EOF_MAX_OGGS)
		{	//IF EOF can store this OGG profile
			eof_load_song_string_pf(sp->tags->ogg[sp->tags->oggs].filename,fp,256);	//Read the OGG filename
			eof_load_song_string_pf(NULL,fp,0);				//Parse past the original audio file name (not supported yet)
			eof_load_song_string_pf(NULL,fp,0);				//Parse past the OGG profile comments string (not supported yet)
			sp->tags->ogg[sp->tags->oggs].midi_offset = pack_igetl(fp);	//Read the profile's MIDI delay
			pack_getc(fp);									//Read the OGG profile flags (not supported yet)
			sp->tags->oggs++;
		}
	}

	count = pack_igetl(fp);					//Read the number of beats
	if(!eof_song_resize_beats(sp, count))	//Resize the beat array accordingly
	{
		return 0;	//Return error upon failure to do so
	}
	for(ctr=0; ctr<count; ctr++)
	{	//For each beat in the project
		sp->beat[ctr]->ppqn = pack_igetl(fp);		//Read the beat's tempo
		sp->beat[ctr]->pos = pack_igetl(fp);		//Read the beat's position (milliseconds or delta ticks)
		sp->beat[ctr]->fpos = sp->beat[ctr]->pos;	//For now, assume the position is in milliseconds and copy to the fpos variable as-is
		sp->beat[ctr]->flags = pack_igetl(fp);		//Read the beat's flags
		pack_getc(fp);								//Read the beat's key signature (not supported yet)
	}

	count = pack_igetl(fp);				//Read the number of text events
	if(!eof_song_resize_text_events(sp, count))	//Resize the text event array accordingly
	{
		return 0;	//Return error upon failure to do so
	}
	for(ctr=0; ctr<count; ctr++)
	{	//For each text event in the project
		eof_load_song_string_pf(sp->text_event[ctr]->text,fp,256);	//Read the text event string
		sp->text_event[ctr]->beat = pack_igetl(fp);	//Read the text event's beat number
		pack_igetl(fp);	//Read the text event's associated track number (not supported yet)
	}

	custom_data_count = pack_igetl(fp);		//Read the number of custom data blocks
	for(custom_data_ctr=0; custom_data_ctr<custom_data_count; custom_data_ctr++)
	{	//For each custom data block in the project
		custom_data_size = pack_igetl(fp);	//Read the size of the custom data block
		for(ctr=0; ctr<custom_data_size; ctr++)
		{	//For each byte in the custom data block
			pack_getc(fp);	//Read the data (not supported yet)
		}
	}

	/* read track data */
	track_count = pack_igetl(fp);		//Read the number of tracks
	for(track_ctr=0; track_ctr<track_count; track_ctr++)
	{	//For each track in the project
		eof_load_song_string_pf(NULL,fp,0);				//Parse past the track name (not supported yet)
		track_format = pack_getc(fp);	//Read the track format
		track_behavior = pack_getc(fp);	//Read the track behavior
		track_type = pack_getc(fp);		//Read the track type
		pack_getc(fp);					//Read the track difficulty level (not supported yet)
		pack_igetl(fp);					//Read the track flags (not supported yet)
		pack_igetw(fp);					//Read the track compliance flags (not supported yet)

		switch(track_format)
		{	//Perform the appropriate logic to load this format of track
			case 1:	//Legacy (non pro guitar, non pro bass, non pro keys, pro or non pro drums)
				if(eof_song_add_track(sp, EOF_LEGACY_TRACK_FORMAT) == 0)	//Add a new legacy track
					return 0;	//Return error upon failure
				sp->track[sp->tracks-1]->trackbehavior = track_behavior;
				sp->track[sp->tracks-1]->tracktype = track_type;
				count = pack_igetl(fp);	//Read the number of notes in this track
				if(count > EOF_MAX_NOTES)
				{
					allegro_message("Error: Unsupported number of notes in track %lu.  Aborting",track_ctr);
					return 0;
				}
				eof_legacy_track_resize(sp->legacy_track[sp->legacy_tracks-1],count);	//Resize the note array
				for(ctr=0; ctr<count; ctr++)
				{	//For each note in this track
					eof_load_song_string_pf(NULL,fp,0);		//Parse past the note name (not supported yet)
					sp->legacy_track[sp->legacy_tracks-1]->note[ctr]->type = pack_getc(fp);		//Read the note's difficulty
					sp->legacy_track[sp->legacy_tracks-1]->note[ctr]->note = pack_getc(fp);		//Read note bitflags
					sp->legacy_track[sp->legacy_tracks-1]->note[ctr]->pos = pack_igetl(fp);		//Read note position
					sp->legacy_track[sp->legacy_tracks-1]->note[ctr]->length = pack_igetl(fp);	//Read note length
					sp->legacy_track[sp->legacy_tracks-1]->note[ctr]->flags = pack_igetw(fp);	//Read note flags
				}
			break;
			case 2:	//Vocal
				if(eof_song_add_track(sp, EOF_VOCAL_TRACK_FORMAT) == 0)	//Add a new vocal track
					return 0;	//Return error upon failure
				sp->track[sp->tracks-1]->trackbehavior = track_behavior;
				sp->track[sp->tracks-1]->tracktype = track_type;
				pack_getc(fp);	//Read the tone set number assigned to this track (not supported yet)
				count = pack_igetl(fp);	//Read the number of notes in this track
				if(count > EOF_MAX_LYRICS)
				{
					allegro_message("Error: Unsupported number of lyrics in track %lu.  Aborting",track_ctr);
					return 0;
				}
				eof_vocal_track_resize(sp->vocal_track[sp->vocal_tracks-1],count);	//Resize the lyrics array
				for(ctr=0; ctr<count; ctr++)
				{	//For each lyric in this track
					eof_load_song_string_pf(sp->vocal_track[sp->vocal_tracks-1]->lyric[ctr]->text,fp,EOF_MAX_LYRIC_LENGTH);
					pack_getc(fp);	//Read lyric set number (not supported yet)
					sp->vocal_track[sp->vocal_tracks-1]->lyric[ctr]->note = pack_getc(fp);		//Read lyric pitch
					sp->vocal_track[sp->vocal_tracks-1]->lyric[ctr]->pos = pack_igetl(fp);		//Read lyric position
					sp->vocal_track[sp->vocal_tracks-1]->lyric[ctr]->length = pack_igetl(fp);	//Read lyric length
					pack_igetw(fp);	//Read lyric flags (not supported yet)
				}
			break;
			case 3:	//Pro Guitar/Bass
				allegro_message("Error: Pro Guitar/Bass not supported yet.  Aborting");
			return 0;
			case 4:	//Pro Keys
				allegro_message("Error: Pro Keys not supported yet.  Aborting");
			return 0;
			case 5:	//Variable Lane Legacy
				allegro_message("Error: Variable lane not supported yet.  Aborting");
			return 0;
			default://Unknown track type
				allegro_message("Error: Unsupported track type.  Aborting");
			return 0;
		}

		section_type_count = pack_igetw(fp);	//Read the number of types of sections defined for this track
		for(section_type_ctr=0; section_type_ctr<section_type_count; section_type_ctr++)
		{	//For each type of section in this track
			section_type = pack_igetw(fp);		//Read the type of section this is
			section_count = pack_igetl(fp);		//Read the number of instances of this type of section there is
			for(section_ctr=0; section_ctr<section_count; section_ctr++)
			{	//For each instance of the specified section
				eof_load_song_string_pf(NULL,fp,0);				//Parse past the section name (not supported yet)
				inputc = pack_getc(fp);			//Read the section's associated difficulty
				section_start = pack_igetl(fp);	//Read the start timestamp of the section
				section_end = pack_igetl(fp);	//Read the end timestamp of the section
				inputl = pack_igetw(fp);		//Read the section flags (not supported yet)

				//Perform the appropriate logic to load this type of section
				switch(track_ctr)
				{
					case 0:		//Read sections that are global
						switch(section_type)
						{
							case 3:	//Bookmark section
								eof_song_add_section(sp,0,EOF_BOOKMARK_SECTION,0,section_start,0,bookmarkctr);
								bookmarkctr++;
							break;
							case 4:	//Fret Catalog section
								eof_song_add_section(sp,0,EOF_FRET_CATALOG_SECTION,inputc,section_start,section_end,track_ctr-1);	//For now, EOF still numbers tracks starting from number 0 (ie EOF_TRACK_GUITAR)
							break;
						}
					break;

					default:	//Read track-specific sections
						eof_song_add_section(sp,track_ctr,section_type,inputc,section_start,section_end,inputl);
					break;
				}
			}
		}//For each type of section in this track

		custom_data_count = pack_igetl(fp);		//Read the number of custom data blocks
		for(custom_data_ctr=0; custom_data_ctr<custom_data_count; custom_data_ctr++)
		{	//For each custom data block in the project
			custom_data_size = pack_igetl(fp);	//Read the size of the custom data block
			for(ctr=0; ctr<custom_data_size; ctr++)
			{	//For each byte in the custom data block
				pack_getc(fp);	//Read the data (not supported yet)
			}
		}
	}//For each track in the project
	return 1;	//Return success
}

int eof_song_add_section(EOF_SONG * sp, unsigned long track, unsigned long sectiontype, char difficulty, unsigned long start, unsigned long stop, unsigned long flags)
{
	if((sp == NULL) || (track >= sp->tracks))
		return 0;	//Return error

	if(track == 0)
	{	//Allow global sections to be added
		switch(sectiontype)
		{
			case EOF_BOOKMARK_SECTION:
				if(flags < EOF_MAX_BOOKMARK_ENTRIES)
				{	//If EOF can store this bookmark
					sp->bookmark_pos[flags] = start;
				}
			return 1;
			case EOF_FRET_CATALOG_SECTION:
				sp->catalog->entry[sp->catalog->entries].track = flags;		//For now, EOF still numbers tracks starting from number 0
				sp->catalog->entry[sp->catalog->entries].type = difficulty;			//Store the fret catalog section's associated difficulty
				sp->catalog->entry[sp->catalog->entries].start_pos = start;
				sp->catalog->entry[sp->catalog->entries].end_pos = stop;
			return 1;
			default:	//Unknown global section type
			return 0;	//Return error
		}
	}

	switch(sectiontype)
	{	//Perform the appropriate logic to add this type of section
		case 1:	//Solo section
			if(sp->track[track]->trackbehavior != 3)
			{	//Solo sections are not valid for vocal tracks
			}
		break;
		case 2:	//Star Power section
		break;
		case 5:	//Lyric Phrase section
			if(sp->track[track]->trackbehavior == 3)
			{	//Lyric phrases are only valid for vocal tracks
			}
		break;
		case 6:	//Yellow Tom section (not supported yet)
			if(sp->track[track]->trackbehavior == 2)
			{	//Tom sections are only valid for drum tracks
			}
		break;
		case 7:	//Blue Tom section (not supported yet)
			if(sp->track[track]->trackbehavior == 2)
			{	//Tom sections are only valid for drum tracks
			}
		break;
		case 8:	//Green Tom section (not supported yet)
			if(sp->track[track]->trackbehavior == 2)
			{	//Tom sections are only valid for drum tracks
			}
		break;
	}
	return 0;	//Return error
}
