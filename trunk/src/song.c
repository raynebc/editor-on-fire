#include <allegro.h>
#include <time.h>
#include "main.h"
#include "editor.h"
#include "beat.h"
#include "song.h"
#include "legacy.h"
#include "mix.h"
#include "undo.h"
#include "utility.h"
#include "menu/file.h"
#include "menu/song.h"

EOF_TRACK_ENTRY eof_default_tracks[EOF_TRACKS_MAX + 1] =
{
	{0},
	{EOF_LEGACY_TRACK_FORMAT, EOF_GUITAR_TRACK_BEHAVIOR, EOF_TRACK_GUITAR, 0, "PART GUITAR", 0, 0},
	{EOF_LEGACY_TRACK_FORMAT, EOF_GUITAR_TRACK_BEHAVIOR, EOF_TRACK_BASS, 0, "PART BASS", 0, 0},
	{EOF_LEGACY_TRACK_FORMAT, EOF_GUITAR_TRACK_BEHAVIOR, EOF_TRACK_GUITAR_COOP, 0, "PART GUITAR COOP", 0, 0},
	{EOF_LEGACY_TRACK_FORMAT, EOF_GUITAR_TRACK_BEHAVIOR, EOF_TRACK_RHYTHM, 0, "PART RHYTHM", 0, 0},
	{EOF_LEGACY_TRACK_FORMAT, EOF_DRUM_TRACK_BEHAVIOR, EOF_TRACK_DRUM, 0, "PART DRUMS", 0, 0},
	{EOF_VOCAL_TRACK_FORMAT, EOF_VOCAL_TRACK_BEHAVIOR, EOF_TRACK_VOCALS, 0, "PART VOCALS", 0, 0},
	{EOF_LEGACY_TRACK_FORMAT, EOF_KEYS_TRACK_BEHAVIOR, EOF_TRACK_KEYS, 0, "PART KEYS", 0, 0},
	{EOF_PRO_GUITAR_TRACK_FORMAT, EOF_PRO_GUITAR_TRACK_BEHAVIOR, EOF_TRACK_PRO_BASS, 0, "PART REAL_BASS", 0, 0},
	{EOF_PRO_GUITAR_TRACK_FORMAT, EOF_PRO_GUITAR_TRACK_BEHAVIOR, EOF_TRACK_PRO_GUITAR, 0, "PART REAL_GUITAR", 0, 0},

	//This pro format is not supported yet, but the entry describes the track's details
	{EOF_PRO_KEYS_TRACK_FORMAT, EOF_PRO_KEYS_TRACK_BEHAVIOR, EOF_TRACK_PRO_KEYS, 0, "PART REAL_KEYS", 0, 0}
};	//These entries describe the tracks that EOF should present by default
	//These entries are indexed the same as the track type in the new EOF project format

EOF_TRACK_ENTRY eof_midi_tracks[EOF_TRACKS_MAX + 11] =
{
	{0},
	{EOF_LEGACY_TRACK_FORMAT, EOF_GUITAR_TRACK_BEHAVIOR, EOF_TRACK_GUITAR, 0, "PART GUITAR", 0, 0},
	{EOF_LEGACY_TRACK_FORMAT, EOF_GUITAR_TRACK_BEHAVIOR, EOF_TRACK_BASS, 0, "PART BASS", 0, 0},
	{EOF_LEGACY_TRACK_FORMAT, EOF_GUITAR_TRACK_BEHAVIOR, EOF_TRACK_GUITAR_COOP, 0, "PART GUITAR COOP", 0, 0},
	{EOF_LEGACY_TRACK_FORMAT, EOF_GUITAR_TRACK_BEHAVIOR, EOF_TRACK_RHYTHM, 0, "PART RHYTHM", 0, 0},
	{EOF_LEGACY_TRACK_FORMAT, EOF_DRUM_TRACK_BEHAVIOR, EOF_TRACK_DRUM, 0, "PART DRUMS", 0, 0},
	{EOF_VOCAL_TRACK_FORMAT, EOF_VOCAL_TRACK_BEHAVIOR, EOF_TRACK_VOCALS, 0, "PART VOCALS", 0, 0},
	{EOF_LEGACY_TRACK_FORMAT, EOF_KEYS_TRACK_BEHAVIOR, EOF_TRACK_KEYS, 0, "PART KEYS", 0, 0},
	{EOF_PRO_GUITAR_TRACK_FORMAT, EOF_PRO_GUITAR_TRACK_BEHAVIOR, EOF_TRACK_PRO_BASS, 0, "PART REAL_BASS", 0, 0},
	{EOF_PRO_GUITAR_TRACK_FORMAT, EOF_PRO_GUITAR_TRACK_BEHAVIOR, EOF_TRACK_PRO_GUITAR, 0, "PART REAL_GUITAR", 0, 0},

	//These tracks are not supported for import yet, but these entries describe the tracks' details
	{EOF_PRO_KEYS_TRACK_FORMAT, EOF_PRO_KEYS_TRACK_BEHAVIOR, EOF_TRACK_PRO_KEYS, 0, "PART REAL_KEYS_X", 0, 0},
	{EOF_PRO_KEYS_TRACK_FORMAT, EOF_PRO_KEYS_TRACK_BEHAVIOR, EOF_TRACK_PRO_KEYS, 0, "PART REAL_KEYS_H", 0, 0},
	{EOF_PRO_KEYS_TRACK_FORMAT, EOF_PRO_KEYS_TRACK_BEHAVIOR, EOF_TRACK_PRO_KEYS, 0, "PART REAL_KEYS_M", 0, 0},
	{EOF_PRO_KEYS_TRACK_FORMAT, EOF_PRO_KEYS_TRACK_BEHAVIOR, EOF_TRACK_PRO_KEYS, 0, "PART REAL_KEYS_E", 0, 0},
	{EOF_PRO_GUITAR_TRACK_FORMAT, EOF_PRO_GUITAR_TRACK_BEHAVIOR, EOF_TRACK_PRO_GUITAR, 0, "PART REAL_GUITAR_22", 0, 0},
	{EOF_VOCAL_TRACK_FORMAT, EOF_VOCAL_TRACK_BEHAVIOR, EOF_TRACK_VOCALS, 0, "HARM1", 0, 0},
	{EOF_VOCAL_TRACK_FORMAT, EOF_VOCAL_TRACK_BEHAVIOR, EOF_TRACK_VOCALS, 0, "HARM2", 0, 0},
	{EOF_VOCAL_TRACK_FORMAT, EOF_VOCAL_TRACK_BEHAVIOR, EOF_TRACK_VOCALS, 0, "HARM3", 0, 0},
	{EOF_VOCAL_TRACK_FORMAT, EOF_VOCAL_TRACK_BEHAVIOR, EOF_TRACK_VOCALS, 0, "PART HARM1", 0, 0},
	{EOF_VOCAL_TRACK_FORMAT, EOF_VOCAL_TRACK_BEHAVIOR, EOF_TRACK_VOCALS, 0, "PART HARM2", 0, 0},
	{EOF_VOCAL_TRACK_FORMAT, EOF_VOCAL_TRACK_BEHAVIOR, EOF_TRACK_VOCALS, 0, "PART HARM3", 0, 0}
};	//These entries describe tracks in the RB3 MIDI file format and their associated EOF track type macros, for use during import/export
	//One complication here is that the pro keys charting gets one MIDI track for each difficulty
	//Another is that the pro guitar could have a separate track for use with the Squier guitar (22 frets instead of 17)
	//Another is that although the best way to offer harmony charting would be in PART VOCALS, each harmony gets its own MIDI track
	//For the purpose of MIDI import, it will be expected that eof_midi_tracks[track_type].track_format is correct for the given track,
	// so eof_midi_tracks[EOF_TRACK_PRO_GUITAR].track_format references the correct behavior for the track


/* sort all notes according to position */
int eof_song_qsort_legacy_notes(const void * e1, const void * e2)
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
	EOF_SONG * sp = NULL;
	unsigned long i;

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
	sp->resolution = 0;
	sp->tracks = 0;
	sp->legacy_tracks = 0;
	sp->vocal_tracks = 0;
	sp->pro_guitar_tracks = 0;
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
	unsigned long ctr;

	if(sp == NULL)
		return;

	for(ctr= sp->tracks-1; ctr > 0; ctr--)
	{
		eof_song_delete_track(sp, ctr);
	}

	for(ctr=0; ctr < sp->beats; ctr++)
	{	//For each entry in the beat array
		free(sp->beat[ctr]);
	}

	for(ctr=0; ctr < sp->text_events; ctr++)
	{	//For each entry in the text event array
		free(sp->text_event[ctr]);
	}

	free(sp->catalog);
	free(sp);
}

EOF_SONG * eof_load_song(const char * fn)
{
	PACKFILE * fp = NULL;
	EOF_SONG * sp = NULL;
	char header[16] = {'E', 'O', 'F', 'S', 'O', 'N', 'H', 0};	//This header represents the current project format
	char rheader[16];
	short i;

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

EOF_NOTE * eof_legacy_track_add_note(EOF_LEGACY_TRACK * tp)
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

void eof_legacy_track_delete_note(EOF_LEGACY_TRACK * tp, unsigned long note)
{
	unsigned long i;

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

void eof_legacy_track_sort_notes(EOF_LEGACY_TRACK * tp)
{
	qsort(tp->note, tp->notes, sizeof(EOF_NOTE *), eof_song_qsort_legacy_notes);
}

long eof_fixup_next_legacy_note(EOF_LEGACY_TRACK * tp, unsigned long note)
{
	long i;

	for(i = note + 1; i < tp->notes; i++)
	{
		if(tp->note[i]->type == tp->note[note]->type)
		{
			return i;
		}
	}
	return -1;
}

void eof_legacy_track_fixup_notes(EOF_LEGACY_TRACK * tp, int sel)
{
	unsigned long i;
	long next;

	if(!sel)
	{
		if(eof_selection.current < tp->notes)
		{
			eof_selection.multi[eof_selection.current] = 0;
		}
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	for(i = tp->notes; i > 0; i--)
	{
		/* fix selections */
		if((tp->note[i-1]->type == eof_note_type) && (tp->note[i-1]->pos == eof_selection.current_pos))
		{
			eof_selection.current = i-1;
		}
		if((tp->note[i-1]->type == eof_note_type) && (tp->note[i-1]->pos == eof_selection.last_pos))
		{
			eof_selection.last = i-1;
		}

		/* delete certain notes */
		if((tp->note[i-1]->note == 0) || ((tp->note[i-1]->type < 0) || (tp->note[i-1]->type > 4)) || (tp->note[i-1]->pos < eof_song->tags->ogg[eof_selected_ogg].midi_offset) || (tp->note[i-1]->pos >= eof_music_length))
		{
			eof_legacy_track_delete_note(tp, i-1);
		}

		else
		{
			/* make sure there are no 0-length notes */
			if(tp->note[i-1]->length <= 0)
			{
				tp->note[i-1]->length = 1;
			}

			/* make sure note doesn't extend past end of song */
			if(tp->note[i-1]->pos + tp->note[i-1]->length >= eof_music_length)
			{
				tp->note[i-1]->length = eof_music_length - tp->note[i-1]->pos;
			}

			/* compare this note to the next one of the same type
			   to make sure they don't overlap */
			next = eof_fixup_next_legacy_note(tp, i-1);
			if(next >= 0)
			{
				if(tp->note[i-1]->pos == tp->note[next]->pos)
				{
					tp->note[i-1]->note |= tp->note[next]->note;
					eof_legacy_track_delete_note(tp, next);
				}
				else if(tp->note[i-1]->pos + tp->note[i-1]->length > tp->note[next]->pos - 1)
				{
					if(!(tp->note[i-1]->flags & EOF_NOTE_FLAG_CRAZY) || (tp->note[i-1]->note & tp->note[next]->note))
					{
						tp->note[i-1]->length = tp->note[next]->pos - tp->note[i-1]->pos - 1;
					}
				}
			}
		}
	}
	if(eof_open_bass_enabled() && (tp == eof_song->legacy_track[eof_song->track[EOF_TRACK_BASS]->tracknum]))
	{	//If open bass strumming is enabled, and this is the bass guitar track, check to ensure that open bass doesn't conflict with other notes/HOPOs/statuses
		for(i = 0; i < tp->notes; i++)
		{	//For each note in the track
			if(tp->note[i]->note & 32)
			{	//If this note contains open bass (lane 6)
				tp->note[i]->note = 32;							//Clear all lanes except lane 6
				tp->note[i]->flags &= (~EOF_NOTE_FLAG_F_HOPO);	//Clear the forced HOPO on flag
				tp->note[i]->flags &= (~EOF_NOTE_FLAG_NO_HOPO);	//Clear the forced HOPO off flag
				tp->note[i]->flags &= (~EOF_NOTE_FLAG_CRAZY);	//Clear the crazy status
			}
			else if((tp->note[i]->note & 1) && (tp->note[i]->flags & EOF_NOTE_FLAG_F_HOPO))
			{	//If this note contains a gem on lane 1 and the note has the forced HOPO on status
				tp->note[i]->flags &= (~EOF_NOTE_FLAG_F_HOPO);	//Clear the forced HOPO on flag
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
	if(eof_song && (tp->parent->track_behavior == EOF_DRUM_TRACK_BEHAVIOR))
	{	//If the track being cleaned is a drum track
		for(i = 0; i < tp->notes; i++)
		{	//For each note in the drum track
			if(eof_check_flags_at_legacy_note_pos(tp,i,EOF_NOTE_FLAG_G_CYMBAL))
			{	//If any notes at this position are marked as a green cymbal
				eof_set_flags_at_legacy_note_pos(tp,i,EOF_NOTE_FLAG_G_CYMBAL,1);	//Mark all notes at this position as green cymbal
			}
			else
			{
				eof_set_flags_at_legacy_note_pos(tp,i,EOF_NOTE_FLAG_G_CYMBAL,0);	//Mark all notes at this position as green drum
			}
			if(eof_check_flags_at_legacy_note_pos(tp,i,EOF_NOTE_FLAG_Y_CYMBAL))
			{	//If any notes at this position are marked as a yellow cymbal
				eof_set_flags_at_legacy_note_pos(tp,i,EOF_NOTE_FLAG_Y_CYMBAL,1);	//Mark all notes at this position as yellow cymbal
			}
			else
			{
				eof_set_flags_at_legacy_note_pos(tp,i,EOF_NOTE_FLAG_Y_CYMBAL,0);	//Mark all notes at this position as yellow drum
			}
			if(eof_check_flags_at_legacy_note_pos(tp,i,EOF_NOTE_FLAG_B_CYMBAL))
			{	//If any notes at this position are marked as a blue cymbal
				eof_set_flags_at_legacy_note_pos(tp,i,EOF_NOTE_FLAG_B_CYMBAL,1);	//Mark all notes at this position as blue cymbal
			}
			else
			{
				eof_set_flags_at_legacy_note_pos(tp,i,EOF_NOTE_FLAG_B_CYMBAL,0);	//Mark all notes at this position as blue drum
			}
		}
	}
}

void eof_legacy_track_add_star_power(EOF_LEGACY_TRACK * tp, unsigned long start_pos, unsigned long end_pos)
{
	if(tp->star_power_paths < EOF_MAX_PHRASES)
	{	//If the maximum number of star power phrases for this track hasn't already been defined
		tp->star_power_path[tp->star_power_paths].start_pos = start_pos;
		tp->star_power_path[tp->star_power_paths].end_pos = end_pos;
		tp->star_power_paths++;
	}
}

void eof_legacy_track_delete_star_power(EOF_LEGACY_TRACK * tp, unsigned long index)
{
	unsigned long i;

	if(index >= tp->star_power_paths)
		return;

	if(tp->star_power_path[index].name != NULL)
	{	//If the section has a name
		free(tp->star_power_path[index].name);	//Free it
	}

	for(i = index; i < tp->star_power_paths - 1; i++)
	{
		memcpy(&tp->star_power_path[i], &tp->star_power_path[i + 1], sizeof(EOF_PHRASE_SECTION));
	}
	tp->star_power_paths--;
}

void eof_legacy_track_add_solo(EOF_LEGACY_TRACK * tp, unsigned long start_pos, unsigned long end_pos)
{
	if(tp->solos < EOF_MAX_PHRASES)
	{	//If the maximum number of solo phrases for this track hasn't already been defined
		tp->solo[tp->solos].start_pos = start_pos;
		tp->solo[tp->solos].end_pos = end_pos;
		tp->solos++;
	}
}

void eof_legacy_track_delete_solo(EOF_LEGACY_TRACK * tp, unsigned long index)
{
	unsigned long i;

	if(index >= tp->solos)
		return;

	if(tp->solo[index].name != NULL)
	{	//If the section has a name
		free(tp->solo[index].name);	//Free it
	}

	for(i = index; i < tp->solos - 1; i++)
	{
		memcpy(&tp->solo[i], &tp->solo[i + 1], sizeof(EOF_PHRASE_SECTION));
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

void eof_vocal_track_delete_lyric(EOF_VOCAL_TRACK * tp, unsigned long lyric)
{
	unsigned long i;

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

long eof_fixup_next_lyric(EOF_VOCAL_TRACK * tp, unsigned long lyric)
{
	long i;

	for(i = lyric + 1; i < tp->lyrics; i++)
	{
		return i;
	}
	return -1;
}

void eof_vocal_track_fixup_lyrics(EOF_VOCAL_TRACK * tp, int sel)
{
	unsigned long i, j;
	int lc;
	long next;

	if(!sel)
	{
		if(eof_selection.current < tp->lyrics)
		{
			eof_selection.multi[eof_selection.current] = 0;
		}
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	for(i = tp->lyrics; i > 0; i--)
	{
		/* fix selections */
		if(tp->lyric[i-1]->pos == eof_selection.current_pos)
		{
			eof_selection.current = i-1;
		}
		if(tp->lyric[i-1]->pos == eof_selection.last_pos)
		{
			eof_selection.last = i-1;
		}

		/* delete certain notes */
		if((tp->lyric[i-1]->pos < eof_song->tags->ogg[eof_selected_ogg].midi_offset) || (tp->lyric[i-1]->pos >= eof_music_length))
		{
			eof_vocal_track_delete_lyric(tp, i-1);
		}

		else
		{
			/* make sure there are no 0-length notes */
			if(tp->lyric[i-1]->length <= 0)
			{
				tp->lyric[i-1]->length = 1;
			}

			/* make sure note doesn't extend past end of song */
			if(tp->lyric[i-1]->pos + tp->lyric[i-1]->length >= eof_music_length)
			{
				tp->lyric[i-1]->length = eof_music_length - tp->lyric[i-1]->pos;
			}

			/* compare this note to the next one of the same type
			   to make sure they don't overlap */
			next = eof_fixup_next_lyric(tp, i-1);
			if(next >= 0)
			{
				if(tp->lyric[i-1]->pos == tp->lyric[next]->pos)
				{
					eof_vocal_track_delete_lyric(tp, next);
				}
				else if(tp->lyric[i-1]->pos + tp->lyric[i-1]->length >= tp->lyric[next]->pos - 1)
				{	//If this lyric does not end at least 1ms before the next lyric starts
					tp->lyric[i-1]->length = tp->lyric[next]->pos - tp->lyric[i-1]->pos - 1 - 1;	//Subtract one more to ensure padding
					if(tp->lyric[i-1]->length <= 0)
						tp->lyric[i-1]->length = 1;	//Ensure that this doesn't cause the length to be invalid
				}
			}
		}

		/* validate lyric text, ie. freestyle marker */
		eof_fix_lyric(tp,i-1);
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
	for(i = tp->lines; i > 0; i--)
	{
		lc = 0;
		for(j = 0; j < tp->lyrics; j++)
		{
			if((tp->lyric[j]->pos >= tp->line[i-1].start_pos) && (tp->lyric[j]->pos <= tp->line[i-1].end_pos))
			{
				lc++;
			}
		}
		if(lc <= 0)
		{
			eof_vocal_track_delete_line(tp, i-1);
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

void eof_vocal_track_delete_line(EOF_VOCAL_TRACK * tp, unsigned long index)
{
	unsigned long i;

	if(tp->lines == 0)	//If there are no lyric line phrases to delete
		return;			//Cancel this to avoid problems

	for(i = index; i < tp->lines - 1; i++)
	{
		memcpy(&tp->line[i], &tp->line[i + 1], sizeof(EOF_PHRASE_SECTION));
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

void eof_song_delete_beat(EOF_SONG * sp, unsigned long beat)
{
	unsigned long i;

	free(sp->beat[beat]);
	for(i = beat; i < sp->beats - 1; i++)
	{
		sp->beat[i] = sp->beat[i + 1];
	}
	sp->beats--;
}

int eof_song_resize_beats(EOF_SONG * sp, unsigned long beats)
{
	unsigned long i;
	unsigned long oldbeats = sp->beats;
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

EOF_TEXT_EVENT * eof_song_add_text_event(EOF_SONG * sp, unsigned long beat, char * text)
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

void eof_song_delete_text_event(EOF_SONG * sp, unsigned long event)
{
	unsigned long i;
	free(sp->text_event[event]);
	for(i = event; i < sp->text_events - 1; i++)
	{
		sp->text_event[i] = sp->text_event[i + 1];
	}
	sp->text_events--;
}

void eof_song_move_text_events(EOF_SONG * sp, unsigned long beat, int offset)
{
	unsigned long i;

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

int eof_song_resize_text_events(EOF_SONG * sp, unsigned long events)
{
	unsigned long i;
	unsigned long oldevents = sp->text_events;
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
	unsigned long i, j;

	if(eof_selection.current < eof_get_track_size(eof_song, eof_selected_track))
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

	for(j = 1; j < eof_song->tracks; j++)
	{
		eof_track_fixup_notes(eof_song, j, j == eof_selected_track);
	}
}

void eof_sort_notes(void)
{
	unsigned long j;

	for(j = 1; j < eof_song->tracks; j++)
	{
		eof_track_sort_notes(eof_song, j);
	}
}

void eof_detect_difficulties(EOF_SONG * sp)
{
	unsigned long i;

	memset(eof_note_difficulties, 0, sizeof(int) * 4);
	eof_note_type_name[0][0] = ' ';
	eof_note_type_name[1][0] = ' ';
	eof_note_type_name[2][0] = ' ';
	eof_note_type_name[3][0] = ' ';
	eof_note_type_name[4][0] = ' ';
	eof_vocal_tab_name[0][0] = ' ';

	for(i = 0; i < eof_get_track_size(sp, eof_selected_track); i++)
	{
		if(sp->track[eof_selected_track]->track_format == EOF_VOCAL_TRACK_FORMAT)
		{
			if(sp->vocal_track[sp->track[eof_selected_track]->tracknum]->lyrics)
			{
				eof_vocal_tab_name[0][0] = '*';
				break;
			}
		}
		else
		{
			if((eof_get_note_type(sp, eof_selected_track, i) >= 0) && (eof_get_note_type(sp, eof_selected_track, i) < 5))
			{
				eof_note_difficulties[(int)eof_get_note_type(sp, eof_selected_track, i)] = 1;
				eof_note_type_name[(int)eof_get_note_type(sp, eof_selected_track, i)][0] = '*';
			}
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
long eof_song_tick_to_msec(EOF_SONG * sp, unsigned long track, unsigned long tick)
{
	unsigned long beat; // which beat 'tick' lies in
	double curpos = sp->tags->ogg[eof_selected_ogg].midi_offset;
	double portion;
	unsigned long i;

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
long eof_song_msec_to_tick(EOF_SONG * sp, unsigned long track, unsigned long msec)
{
	long beat; // which beat 'msec' lies in
	long beat_tick;
	long portion;
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


char eof_check_flags_at_legacy_note_pos(EOF_LEGACY_TRACK *tp,unsigned notenum,unsigned long flag)
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

void eof_set_flags_at_legacy_note_pos(EOF_LEGACY_TRACK *tp,unsigned notenum,unsigned long flag,char operation)
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

int eof_load_song_string_pf(char *const buffer, PACKFILE *fp, const unsigned long buffersize)
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

int eof_song_add_track(EOF_SONG * sp, EOF_TRACK_ENTRY * trackdetails)
{
	EOF_LEGACY_TRACK *ptr = NULL;
	EOF_VOCAL_TRACK *ptr2 = NULL;
	EOF_TRACK_ENTRY *ptr3 = NULL;
	EOF_PRO_GUITAR_TRACK *ptr4 = NULL;
	unsigned long count=0;

	if((sp == NULL) || (trackdetails == NULL))
		return 0;	//Return error

	if(sp->tracks <= EOF_TRACKS_MAX)
	{
		ptr3 = malloc(sizeof(EOF_TRACK_ENTRY));
		if(ptr3 == NULL)
			return 0;	//Return error

		//Insert new track structure in the appropriate track type array
		switch(trackdetails->track_format)
		{
			case EOF_LEGACY_TRACK_FORMAT:
				count = sp->legacy_tracks;
				ptr = malloc(sizeof(EOF_LEGACY_TRACK));
				if(ptr == NULL)
					return 0;	//Return error
				ptr->notes = 0;
				ptr->solos = 0;
				ptr->star_power_paths = 0;
				ptr->trills = 0;
				ptr->tremolos = 0;
				if(trackdetails->flags & EOF_TRACK_FLAG_OPEN_STRUM)
				{	//Open strum is tracked as a sixth lane
					ptr->numlanes = 6;
				}
				else
				{	//Otherwise, all legacy tracks will be 5 lanes by default
					ptr->numlanes = 5;
				}
				ptr->parent = ptr3;
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
				ptr2->star_power_paths = 0;
				ptr2->parent = ptr3;
				sp->vocal_track[sp->vocal_tracks] = ptr2;
				sp->vocal_tracks++;
			break;
			case EOF_PRO_KEYS_TRACK_FORMAT:
			break;
			case EOF_PRO_GUITAR_TRACK_FORMAT:
				count = sp->pro_guitar_tracks;
				ptr4 = malloc(sizeof(EOF_PRO_GUITAR_TRACK));
				if(ptr4 == NULL)
					return 0;	//Return error
				ptr4->numfrets = 17;	//By default, assume a 17 fret guitar (ie. Mustang controller)
				ptr4->numstrings = 6;	//By default, assume a 6 string guitar
				ptr4->tuning = NULL;	//(not implemented yet)
				ptr4->notes = 0;
				ptr4->solos = 0;
				ptr4->star_power_paths = 0;
				ptr4->arpeggios = 0;
				ptr4->trills = 0;
				ptr4->tremolos = 0;
				ptr4->parent = ptr3;
				sp->pro_guitar_track[sp->pro_guitar_tracks] = ptr4;
				sp->pro_guitar_tracks++;
			break;
			case EOF_PRO_VARIABLE_LEGACY_TRACK_FORMAT:
			break;
			default:
			return 0;	//Return error
		}

		//Insert new track structure in the main track array and copy details
		ptr3->tracknum = count;
		ptr3->track_format = trackdetails->track_format;
		ptr3->track_behavior = trackdetails->track_behavior;
		ptr3->track_type = trackdetails->track_type;
		ustrcpy(ptr3->name,trackdetails->name);
		ptr3->difficulty = trackdetails->difficulty;
		ptr3->flags = trackdetails->flags;
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
	switch(sp->track[track]->track_format)
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
			if((sp->track[track]->tracknum >= sp->pro_guitar_tracks) || (sp->pro_guitar_track[sp->track[track]->tracknum] == NULL))
				return 0;	//Cannot remove a pro guitar track that doesn't exist
			if(sp->pro_guitar_track[sp->track[track]->tracknum]->tuning)
			{	//If this track has a defined tuning array, free the array
				free(sp->pro_guitar_track[sp->track[track]->tracknum]->tuning);
			}
			free(sp->pro_guitar_track[sp->track[track]->tracknum]);
			for(ctr = sp->track[track]->tracknum; ctr + 1 < sp->pro_guitar_tracks; ctr++)
			{
				sp->pro_guitar_track[ctr] = sp->pro_guitar_track[ctr + 1];
			}
			sp->pro_guitar_tracks--;
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
	sp->tracks--;
	if(sp->tracks == 1)
	{	//If only the dummy track[0] entry remains
		sp->tracks = 0;	//Drop it so sp->tracks equaling 0 can reflect that there are no tracks
	}
	return 1;	//Return success
}

int eof_load_song_pf(EOF_SONG * sp, PACKFILE * fp)
{
	unsigned char inputc;
	unsigned long inputl,count,ctr,ctr2,bitmask;
	unsigned long track_count,track_ctr;
	unsigned long section_type_count,section_type_ctr,section_type,section_count,section_ctr,section_start,section_end;
	unsigned long custom_data_count,custom_data_ctr,custom_data_size;
	EOF_TRACK_ENTRY temp={0};
	char name[EOF_NAME_LENGTH+1];	//Used to load note/section names

	#define EOFNUMINISTRINGTYPES 12
	char *const inistringbuffer[EOFNUMINISTRINGTYPES]={NULL,NULL,sp->tags->artist,sp->tags->title,sp->tags->frettist,NULL,sp->tags->year,sp->tags->loading_text,NULL,NULL,NULL,NULL};
	unsigned long const inistringbuffersize[12]={0,0,256,256,256,0,32,512,0,0,0,0};
		//Store the buffer information of each of the 12 INI strings to simplify the loading code
		//This buffer can be updated without redesigning the entire load function, just add logic for loading the new string type

	#define EOFNUMINIBOOLEANTYPES 6
	char *const inibooleanbuffer[EOFNUMINIBOOLEANTYPES]={NULL,&sp->tags->lyrics,&sp->tags->eighth_note_hopo,NULL,NULL,NULL};
		//Store the pointers to each of the 5 boolean type INI settings (number 0 is reserved) to simplify the loading code

	#define EOFNUMININUMBERTYPES 5
	unsigned long *const ininumberbuffer[EOFNUMININUMBERTYPES]={NULL,NULL,NULL,NULL,NULL};
		//Store the pointers to each of the 5 number type INI settings (number 0 is reserved) to simplify the loading code


	if((sp == NULL) || (fp == NULL))
		return 0;	//Return failure

	/* read chart properties */
	sp->tags->revision = pack_igetl(fp);	//Read project revision number
	inputc = pack_getc(fp);					//Read timing format
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
		if(((inputc & 0x7F) < EOFNUMINIBOOLEANTYPES) && (inibooleanbuffer[(inputc & 0x7F)] != NULL))
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
			eof_load_song_string_pf(sp->tags->ogg[sp->tags->oggs].description,fp,0);	//Read the OGG profile description string
			sp->tags->ogg[sp->tags->oggs].midi_offset = pack_igetl(fp);	//Read the profile's MIDI delay
			pack_igetl(fp);									//Read the OGG profile flags (not supported yet)
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
		eof_load_song_string_pf(temp.name,fp,sizeof(temp.name));	//Read the track name
		temp.track_format = pack_getc(fp);		//Read the track format
		temp.track_behavior = pack_getc(fp);	//Read the track behavior
		temp.track_type = pack_getc(fp);		//Read the track type
		temp.difficulty = pack_getc(fp);		//Read the track difficulty level
		temp.flags = pack_igetl(fp);			//Read the track flags
		pack_igetw(fp);					//Read the track compliance flags (not supported yet)
		temp.tracknum=0;	//Ignored

		if(track_ctr != 0)
		{	//Add track to project
			if(eof_song_add_track(sp, &temp) == 0)	//Add the track
				return 0;	//Return error upon failure
		}
		switch(temp.track_format)
		{	//Perform the appropriate logic to load this format of track
			case 0:	//The global track only has section data
			break;
			case EOF_LEGACY_TRACK_FORMAT:	//Legacy (non pro guitar, non pro bass, non pro keys, pro or non pro drums)
				sp->legacy_track[sp->legacy_tracks-1]->numlanes = pack_getc(fp);	//Read the number of lanes/keys/etc. used in this track
				count = pack_igetl(fp);	//Read the number of notes in this track
				if(count > EOF_MAX_NOTES)
				{
					allegro_message("Error: Unsupported number of notes in track %lu.  Aborting",track_ctr);
					return 0;
				}
				eof_track_resize(sp, sp->tracks-1,count);	//Resize the note array
				for(ctr=0; ctr<count; ctr++)
				{	//For each note in this track
					eof_load_song_string_pf(sp->legacy_track[sp->legacy_tracks-1]->note[ctr]->name,fp,EOF_NAME_LENGTH);	//Read the note's name
					sp->legacy_track[sp->legacy_tracks-1]->note[ctr]->type = pack_getc(fp);		//Read the note's difficulty
					sp->legacy_track[sp->legacy_tracks-1]->note[ctr]->note = pack_getc(fp);		//Read note bitflags
					sp->legacy_track[sp->legacy_tracks-1]->note[ctr]->pos = pack_igetl(fp);		//Read note position
					sp->legacy_track[sp->legacy_tracks-1]->note[ctr]->length = pack_igetl(fp);	//Read note length
					sp->legacy_track[sp->legacy_tracks-1]->note[ctr]->flags = pack_igetl(fp);	//Read note flags
				}
			break;
			case EOF_VOCAL_TRACK_FORMAT:	//Vocal
				pack_getc(fp);	//Read the tone set number assigned to this track (not supported yet)
				count = pack_igetl(fp);	//Read the number of notes in this track
				if(count > EOF_MAX_LYRICS)
				{
					allegro_message("Error: Unsupported number of lyrics in track %lu.  Aborting",track_ctr);
					return 0;
				}
				eof_track_resize(sp, sp->tracks-1,count);	//Resize the lyrics array
				for(ctr=0; ctr<count; ctr++)
				{	//For each lyric in this track
					eof_load_song_string_pf(sp->vocal_track[sp->vocal_tracks-1]->lyric[ctr]->text,fp,EOF_MAX_LYRIC_LENGTH);	//Read the lyric text
					pack_getc(fp);	//Read lyric set number (not supported yet)
					sp->vocal_track[sp->vocal_tracks-1]->lyric[ctr]->note = pack_getc(fp);		//Read lyric pitch
					sp->vocal_track[sp->vocal_tracks-1]->lyric[ctr]->pos = pack_igetl(fp);		//Read lyric position
					sp->vocal_track[sp->vocal_tracks-1]->lyric[ctr]->length = pack_igetl(fp);	//Read lyric length
					pack_igetl(fp);	//Read lyric flags (not supported yet)
				}
			break;
			case EOF_PRO_KEYS_TRACK_FORMAT:	//Pro Keys
				allegro_message("Error: Pro Keys not supported yet.  Aborting");
			return 0;
			case EOF_PRO_GUITAR_TRACK_FORMAT:	//Pro Guitar/Bass
				sp->pro_guitar_track[sp->pro_guitar_tracks-1]->numfrets = pack_getc(fp);	//Read the number of frets used in this track
				count = pack_getc(fp);	//Read the number of strings used in this track
				if(count > 8)
				{
					allegro_message("Error: Unsupported number of strings in track %lu.  Aborting",track_ctr);
					return 0;
				}
				sp->pro_guitar_track[sp->pro_guitar_tracks-1]->numstrings = count;
				for(ctr=0; ctr < sp->pro_guitar_track[sp->pro_guitar_tracks-1]->numstrings; ctr++)
				{	//For each string
					pack_getc(fp);	//Read the string's tuning (not supported yet)
				}
				count = pack_igetl(fp);	//Read the number of notes in this track
				if(count > EOF_MAX_NOTES)
				{
					allegro_message("Error: Unsupported number of notes in track %lu.  Aborting",track_ctr);
					return 0;
				}
				eof_track_resize(sp, sp->tracks-1,count);	//Resize the note array
				for(ctr=0; ctr<count; ctr++)
				{	//For each note in this track
					eof_load_song_string_pf(sp->pro_guitar_track[sp->pro_guitar_tracks-1]->note[ctr]->name,fp,EOF_NAME_LENGTH);	//Read the note's name
					pack_getc(fp);																	//Read the chord's number (not supported yet)
					sp->pro_guitar_track[sp->pro_guitar_tracks-1]->note[ctr]->type = pack_getc(fp);	//Read the note's difficulty
					sp->pro_guitar_track[sp->pro_guitar_tracks-1]->note[ctr]->note = pack_getc(fp);	//Read note bitflags
					pack_getc(fp);	//Read ghost bitflags (not supported yet)
					for(ctr2=0, bitmask=1; ctr2 < 16; ctr2++, bitmask <<= 1)
					{	//For each of the 16 bits in the note bitflag
						if(sp->pro_guitar_track[sp->pro_guitar_tracks-1]->note[ctr]->note & bitmask)
						{	//If this bit is set
							sp->pro_guitar_track[sp->pro_guitar_tracks-1]->note[ctr]->frets[ctr2] = pack_getc(fp);	//Read this string's fret value
						}
						else
						{	//Write a default value of 0
							sp->pro_guitar_track[sp->pro_guitar_tracks-1]->note[ctr]->frets[ctr2] = 0;
						}
					}
					sp->pro_guitar_track[sp->pro_guitar_tracks-1]->note[ctr]->legacymask = pack_getc(fp);	//Read the legacy note bitmask
					sp->pro_guitar_track[sp->pro_guitar_tracks-1]->note[ctr]->pos = pack_igetl(fp);			//Read note position
					sp->pro_guitar_track[sp->pro_guitar_tracks-1]->note[ctr]->length = pack_igetl(fp);		//Read note length
					sp->pro_guitar_track[sp->pro_guitar_tracks-1]->note[ctr]->flags = pack_igetl(fp);		//Read note flags
				}
			break;
			case EOF_PRO_VARIABLE_LEGACY_TRACK_FORMAT:	//Variable Lane Legacy
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
				eof_load_song_string_pf(name,fp,EOF_NAME_LENGTH+1);	//Parse past the section name
				inputc = pack_getc(fp);								//Read the section's associated difficulty
				section_start = pack_igetl(fp);						//Read the start timestamp of the section
				section_end = pack_igetl(fp);						//Read the end timestamp of the section
				inputl = pack_igetl(fp);							//Read the section flags

				//Perform the appropriate logic to load this type of section
				switch(track_ctr)
				{
					case 0:		//Sections defined in track 0 are global
						switch(section_type)
						{
							case EOF_BOOKMARK_SECTION:		//Bookmark section
								eof_track_add_section(sp,0,EOF_BOOKMARK_SECTION,0,section_start,section_end,inputl,NULL);
							break;
							case EOF_FRET_CATALOG_SECTION:	//Fret Catalog section
								eof_track_add_section(sp,0,EOF_FRET_CATALOG_SECTION,inputc,section_start,section_end,inputl,name);	//For fret catalog sections, the flag represents the associated track number
							break;
						}
					break;

					default:	//Read track-specific sections
						eof_track_add_section(sp,track_ctr,section_type,inputc,section_start,section_end,inputl,name);
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

int eof_track_add_section(EOF_SONG * sp, unsigned long track, unsigned long sectiontype, char difficulty, unsigned long start, unsigned long end, unsigned long flags, char *name)
{
	unsigned long count,tracknum;	//Used to de-obfuscate the track handling

	if((sp == NULL) || ((track != 0) && (track >= sp->tracks)))
		return 0;	//Return error

	if(track == 0)
	{	//Allow global sections to be added
		switch(sectiontype)
		{
			case EOF_BOOKMARK_SECTION:
				if(end < EOF_MAX_BOOKMARK_ENTRIES)
				{	//If EOF can store this bookmark
					sp->bookmark_pos[end] = start;
				}
			return 1;
			case EOF_FRET_CATALOG_SECTION:
				if(sp->catalog->entries < EOF_MAX_CATALOG_ENTRIES)
				{
					sp->catalog->entry[sp->catalog->entries].track = flags;		//For now, EOF still numbers tracks starting from number 0
					sp->catalog->entry[sp->catalog->entries].type = difficulty;	//Store the fret catalog section's associated difficulty
					sp->catalog->entry[sp->catalog->entries].start_pos = start;
					sp->catalog->entry[sp->catalog->entries].end_pos = end;
					if(name == NULL)
					{
						sp->catalog->entry[sp->catalog->entries].name[0] = '\0';
					}
					else
					{
						ustrcpy(sp->catalog->entry[sp->catalog->entries].name, name);
					}
					sp->catalog->entries++;
				}
			return 1;
			default:	//Unknown global section type
			return 0;	//Return error
		}
	}

	tracknum = sp->track[track]->tracknum;
	switch(sectiontype)
	{	//Perform the appropriate logic to add this type of section
		case EOF_SOLO_SECTION:
			switch(sp->track[track]->track_format)
			{	//Solos are allowed for any track EXCEPT vocal tracks
				case EOF_LEGACY_TRACK_FORMAT:
					count = sp->legacy_track[tracknum]->solos;
					if(count < EOF_MAX_PHRASES)
					{	//If EOF can store the solo section
						sp->legacy_track[tracknum]->solo[count].start_pos = start;
						sp->legacy_track[tracknum]->solo[count].end_pos = end;
						sp->legacy_track[tracknum]->solo[count].flags = 0;
						if(name == NULL)
						{
							sp->legacy_track[tracknum]->solo[count].name[0] = '\0';
						}
						else
						{
							ustrcpy(sp->legacy_track[tracknum]->solo[count].name, name);
						}
						sp->legacy_track[tracknum]->solos++;
					}
				return 1;
				case EOF_PRO_GUITAR_TRACK_FORMAT:
					count = sp->pro_guitar_track[tracknum]->solos;
					if(count < EOF_MAX_PHRASES)
					{	//If EOF can store the solo section
						sp->pro_guitar_track[tracknum]->solo[count].start_pos = start;
						sp->pro_guitar_track[tracknum]->solo[count].end_pos = end;
						sp->pro_guitar_track[tracknum]->solo[count].flags = 0;
						if(name == NULL)
						{
							sp->pro_guitar_track[tracknum]->solo[count].name[0] = '\0';
						}
						else
						{
							ustrcpy(sp->pro_guitar_track[tracknum]->solo[count].name, name);
						}
						sp->pro_guitar_track[tracknum]->solos++;
					}
				return 1;
			}
		break;
		case EOF_SP_SECTION:
			switch(sp->track[track]->track_format)
			{	//Star power is valid for any track
				case EOF_LEGACY_TRACK_FORMAT:
					count = sp->legacy_track[tracknum]->star_power_paths;
					if(count < EOF_MAX_PHRASES)
					{	//If EOF can store the star power section
						sp->legacy_track[tracknum]->star_power_path[count].start_pos = start;
						sp->legacy_track[tracknum]->star_power_path[count].end_pos = end;
						sp->legacy_track[tracknum]->star_power_path[count].flags = 0;
						if(name == NULL)
						{
							sp->legacy_track[tracknum]->star_power_path[count].name[0] = '\0';
						}
						else
						{
							ustrcpy(sp->legacy_track[tracknum]->star_power_path[count].name, name);
						}
						sp->legacy_track[tracknum]->star_power_paths++;
					}
				return 1;
				case EOF_VOCAL_TRACK_FORMAT:
					count = sp->vocal_track[tracknum]->star_power_paths;
					if(count < EOF_MAX_PHRASES)
					{	//If EOF can store the star power section
						sp->vocal_track[tracknum]->star_power_path[count].start_pos = start;
						sp->vocal_track[tracknum]->star_power_path[count].end_pos = end;
						sp->vocal_track[tracknum]->star_power_path[count].flags = 0;
						if(name == NULL)
						{
							sp->vocal_track[tracknum]->star_power_path[count].name[0] = '\0';
						}
						else
						{
							ustrcpy(sp->vocal_track[tracknum]->star_power_path[count].name, name);
						}
						sp->vocal_track[tracknum]->star_power_paths++;
					}
				return 1;
				case EOF_PRO_GUITAR_TRACK_FORMAT:
					count = sp->pro_guitar_track[tracknum]->star_power_paths;
					if(count < EOF_MAX_PHRASES)
					{	//If EOF can store the star power section
						sp->pro_guitar_track[tracknum]->star_power_path[count].start_pos = start;
						sp->pro_guitar_track[tracknum]->star_power_path[count].end_pos = end;
						sp->pro_guitar_track[tracknum]->star_power_path[count].flags = 0;
						if(name == NULL)
						{
							sp->pro_guitar_track[tracknum]->star_power_path[count].name[0] = '\0';
						}
						else
						{
							ustrcpy(sp->pro_guitar_track[tracknum]->star_power_path[count].name, name);
						}
						sp->pro_guitar_track[tracknum]->star_power_paths++;
					}
				return 1;
			}
		break;
		case EOF_LYRIC_PHRASE_SECTION:	//Lyric Phrase section
			switch(sp->track[track]->track_format)
			{	//Lyric phrases are only valid for vocal tracks
				case EOF_VOCAL_TRACK_FORMAT:
					count = sp->vocal_track[tracknum]->lines;
					if(count < EOF_MAX_LYRIC_LINES)
					{	//If EOF can store the lyric phrase
						sp->vocal_track[tracknum]->line[count].start_pos = start;
						sp->vocal_track[tracknum]->line[count].end_pos = end;
						sp->vocal_track[tracknum]->line[count].flags = flags;
						sp->vocal_track[tracknum]->line[count].name[0] = '\0';
						sp->vocal_track[tracknum]->lines++;
					}
				return 1;
			}
		break;
		case EOF_YELLOW_TOM_SECTION:	//Yellow Tom section (not supported yet)
			if(sp->track[track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
			{	//Tom sections are only valid for drum tracks
			}
		break;
		case EOF_BLUE_TOM_SECTION:	//Blue Tom section (not supported yet)
			if(sp->track[track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
			{	//Tom sections are only valid for drum tracks
			}
		break;
		case EOF_GREEN_TOM_SECTION:	//Green Tom section (not supported yet)
			if(sp->track[track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR)
			{	//Tom sections are only valid for drum tracks
			}
		break;
		case EOF_TRILL_SECTION:
			if((sp->track[track]->track_behavior == EOF_GUITAR_TRACK_BEHAVIOR) || (sp->track[track]->track_behavior == EOF_PRO_GUITAR_TRACK_BEHAVIOR) || (sp->track[track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR))
			{	//Only legacy/pro guitar/bass/drum type tracks are able to use this type of section
				switch(sp->track[track]->track_format)
				{
					case EOF_LEGACY_TRACK_FORMAT:
						count = sp->legacy_track[tracknum]->trills;
						sp->legacy_track[tracknum]->trill[count].start_pos = start;
						sp->legacy_track[tracknum]->trill[count].end_pos = end;
						sp->legacy_track[tracknum]->trill[count].flags = 0;
						if(name == NULL)
						{
							sp->legacy_track[tracknum]->trill[count].name[0] = '\0';
						}
						else
						{
							ustrcpy(sp->legacy_track[tracknum]->trill[count].name, name);
						}
						sp->legacy_track[tracknum]->trills++;
					return 1;

					case EOF_PRO_GUITAR_TRACK_FORMAT:
						count = sp->pro_guitar_track[tracknum]->trills;
						sp->pro_guitar_track[tracknum]->trill[count].start_pos = start;
						sp->pro_guitar_track[tracknum]->trill[count].end_pos = end;
						sp->pro_guitar_track[tracknum]->trill[count].flags = 0;
						if(name == NULL)
						{
							sp->pro_guitar_track[tracknum]->trill[count].name[0] = '\0';
						}
						else
						{
							ustrcpy(sp->pro_guitar_track[tracknum]->trill[count].name, name);
						}
						sp->pro_guitar_track[tracknum]->trills++;
					return 1;
				}
			}
		break;
		case EOF_ARPEGGIO_SECTION:	//Arpeggio section
			if(sp->track[track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
			{
				count = sp->pro_guitar_track[tracknum]->arpeggios;
				sp->pro_guitar_track[tracknum]->arpeggio[count].start_pos = start;
				sp->pro_guitar_track[tracknum]->arpeggio[count].end_pos = end;
				sp->pro_guitar_track[tracknum]->arpeggio[count].flags = 0;
				if(name == NULL)
				{
					sp->pro_guitar_track[tracknum]->arpeggio[count].name[0] = '\0';
				}
				else
				{
					ustrcpy(sp->pro_guitar_track[tracknum]->arpeggio[count].name, name);
				}
				sp->pro_guitar_track[tracknum]->arpeggios++;
				return 1;
			}
		break;
		case EOF_TRAINER_SECTION:	//Pro trainer section (not supported yet)
		break;
		case EOF_CUSTOM_MIDI_NOTE_SECTION:	//Custom MIDI note section (not supported yet)
		break;
		case EOF_PREVIEW_SECTION:	//Preview audio section (not supported yet)
		break;
		case EOF_TREMOLO_SECTION:
			if((sp->track[track]->track_behavior == EOF_GUITAR_TRACK_BEHAVIOR) || (sp->track[track]->track_behavior == EOF_PRO_GUITAR_TRACK_BEHAVIOR) || (sp->track[track]->track_behavior == EOF_DRUM_TRACK_BEHAVIOR))
			{	//Only legacy/pro guitar/bass/drum type tracks are able to use this type of section
				switch(sp->track[track]->track_format)
				{
					case EOF_LEGACY_TRACK_FORMAT:
						count = sp->legacy_track[tracknum]->tremolos;
						sp->legacy_track[tracknum]->tremolo[count].start_pos = start;
						sp->legacy_track[tracknum]->tremolo[count].end_pos = end;
						sp->legacy_track[tracknum]->tremolo[count].flags = 0;
						if(name == NULL)
						{
							sp->legacy_track[tracknum]->tremolo[count].name[0] = '\0';
						}
						else
						{
							ustrcpy(sp->legacy_track[tracknum]->tremolo[count].name, name);
						}
						sp->legacy_track[tracknum]->tremolos++;
					return 1;

					case EOF_PRO_GUITAR_TRACK_FORMAT:
						count = sp->pro_guitar_track[tracknum]->tremolos;
						sp->pro_guitar_track[tracknum]->tremolo[count].start_pos = start;
						sp->pro_guitar_track[tracknum]->tremolo[count].end_pos = end;
						sp->pro_guitar_track[tracknum]->tremolo[count].flags = 0;
						if(name == NULL)
						{
							sp->pro_guitar_track[tracknum]->tremolo[count].name[0] = '\0';
						}
						else
						{
							ustrcpy(sp->pro_guitar_track[tracknum]->tremolo[count].name, name);
						}
						sp->pro_guitar_track[tracknum]->tremolos++;
					return 1;
				}
			}
		break;
	}
	return 0;	//Return error
}

int eof_save_song_string_pf(char *buffer, PACKFILE *fp)
{
	unsigned long length=0,ctr;

	if(fp == NULL)
		return 1;	//Return error

	if(buffer != NULL)
	{	//If the string isn't NULL
		length = ustrsize(buffer);	//Gets its length in bytes (allowing for Unicode string support)
	}

	pack_iputw(length, fp);	//Write string length
	for(ctr=0; ctr < length; ctr++)
	{
		pack_putc(buffer[ctr], fp);
	}

	return 0;	//Return success
}

int eof_save_song(EOF_SONG * sp, const char * fn)
{
	PACKFILE * fp;
	char header[16] = {'E', 'O', 'F', 'S', 'O', 'N', 'H', 0};
	unsigned long count,ctr,ctr2,tracknum;
	unsigned long track_count,track_ctr,bookmark_count,bitmask;
	char has_solos,has_star_power,has_bookmarks,has_catalog,has_lyric_phrases,has_arpeggios,has_trills,has_tremolos;

	#define EOFNUMINISTRINGTYPES 12
	char *const inistringbuffer[EOFNUMINISTRINGTYPES]={NULL,NULL,sp->tags->artist,sp->tags->title,sp->tags->frettist,NULL,sp->tags->year,sp->tags->loading_text,NULL,NULL,NULL,NULL};
		//Store the buffer information of each of the 12 INI strings to simplify the loading code
		//This buffer can be updated without redesigning the entire load function, just add logic for loading the new string type

	#define EOFNUMINIBOOLEANTYPES 6
	char *const inibooleanbuffer[EOFNUMINIBOOLEANTYPES]={NULL,&sp->tags->lyrics,&sp->tags->eighth_note_hopo,NULL,NULL,NULL};
		//Store the pointers to each of the 5 boolean type INI settings (number 0 is reserved) to simplify the loading code

	#define EOFNUMININUMBERTYPES 5
	unsigned long *const ininumberbuffer[EOFNUMININUMBERTYPES]={NULL,NULL,NULL,NULL,NULL};
		//Store the pointers to each of the 5 number type INI settings (number 0 is reserved) to simplify the loading code


	if((sp == NULL) || (fn == NULL))
	{
		return 0;	//Return error
	}

	/* write file header */
	fp = pack_fopen(fn, "w");
	if(!fp)
	{
		return 0;	//Return error
	}
	pack_fwrite(header, 16, fp);

	/* write chart properties */
	pack_iputl(sp->tags->revision, fp);			//Write project revision number
	pack_putc(0, fp);							//Write timing format
	pack_iputl(EOF_DEFAULT_TIME_DIVISION, fp);	//Write time division (not supported yet)

	/* write song properties */
	//Count the number of INI strings to write (including custom strings)
	count = sp->tags->ini_settings;
	for(ctr=0; ctr < EOFNUMINISTRINGTYPES; ctr++)
	{
		if((inistringbuffer[ctr] != NULL) && (inistringbuffer[ctr][0] != '\0'))
		{	//If this native INI string is populated
			count++;
		}
	}
	pack_iputw(count, fp);	//Write the number of INI strings
	for(ctr=0; ctr < EOFNUMINISTRINGTYPES; ctr++)
	{	//For each built-in INI string
		if((inistringbuffer[ctr] != NULL) && (inistringbuffer[ctr][0] != '\0'))
		{	//If this native INI string is populated
			pack_putc(ctr, fp);	//Write the type of INI string
			eof_save_song_string_pf(inistringbuffer[ctr], fp);	//Write the string
		}
	}
	for(ctr=0; ctr < sp->tags->ini_settings; ctr++)
	{	//For each custom INI string
		pack_putc(0, fp);	//Write the "custom" INI string type
		eof_save_song_string_pf(sp->tags->ini_setting[ctr], fp);	//Write the string
	}
	//Count the number of INI booleans to write
	count = 0;
	for(ctr=0; ctr < EOFNUMINIBOOLEANTYPES; ctr++)
	{
		if((inibooleanbuffer[ctr] != NULL) && (*inibooleanbuffer[ctr] != 0))
		{
			count++;
		}
	}
	pack_iputw(count, fp);	//Write the number of INI booleans
	for(ctr=0; ctr < EOFNUMINIBOOLEANTYPES; ctr++)
	{	//For each boolean value
		if((inibooleanbuffer[ctr] != NULL) && (*inibooleanbuffer[ctr] != 0))
		{	//If this boolean value is nonzero
			pack_putc(0x80 + ctr, fp);	//Write the type of INI boolean in the lower 7 bits with the MSB set to represent TRUE
		}
	}
	//Count the number of INI numbers to write
	count = 0;
	for(ctr=0; ctr < EOFNUMININUMBERTYPES; ctr++)
	{
		if(ininumberbuffer[ctr] != NULL)
		{
			count++;
		}
	}
	pack_iputw(0, fp);	//Write the number of INI numbers
	for(ctr=0; ctr < EOFNUMININUMBERTYPES; ctr++)
	{
		if(ininumberbuffer[ctr] != NULL)
		{
			pack_putc(ctr, fp);	//Write the type of INI number
			pack_iputl(*ininumberbuffer[ctr], fp);	//Write the INI number
		}
	}

	/* write chart data */
	pack_iputw(sp->tags->oggs, fp);	//Write the number of OGG profiles
	for(ctr=0; ctr < sp->tags->oggs; ctr++)
	{	//For each OGG profile in the project
		eof_save_song_string_pf(sp->tags->ogg[ctr].filename, fp);	//Write the OGG filename string
		eof_save_song_string_pf(NULL, fp);	//Write an empty original audio file name string (not supported yet)
		eof_save_song_string_pf(sp->tags->ogg[ctr].description, fp);	//Write an OGG profile description string
		pack_iputl(sp->tags->ogg[ctr].midi_offset, fp);	//Write the profile's MIDI delay
		pack_iputl(0, fp);	//Write the profile's flags (not supported yet)
	}
	pack_iputl(sp->beats, fp);	//Write the number of beats
	for(ctr=0; ctr < sp->beats; ctr++)
	{	//For each beat in the project
		pack_iputl(sp->beat[ctr]->ppqn, fp);	//Write the beat's tempo
		pack_iputl(sp->beat[ctr]->pos, fp);		//Write the beat's position (milliseconds or delta ticks)
		pack_iputl(sp->beat[ctr]->flags, fp);	//Write the beat's flags
		pack_putc(0xFF, fp);	//Write the beat's key signature (not supported yet)
	}
	pack_iputl(sp->text_events, fp);	//Write the number of text events
	for(ctr=0; ctr < sp->text_events; ctr++)
	{	//For each text event in the project
		eof_save_song_string_pf(sp->text_event[ctr]->text, fp);	//Write the text event string
		pack_iputl(sp->text_event[ctr]->beat, fp);	//Write the text event's associated beat number
		pack_iputl(0, fp);	//Write the text event's associated track number (not supported yet)
	}


//	pack_iputl(0, fp);	//Write an empty custom data block (not currently in use)
	pack_iputl(1, fp);			//Write a debug custom data block
	pack_iputl(4, fp);
	pack_iputl(0xFFFFFFFF, fp);


	/* write track data */
	//Count the number of bookmarks
	for(ctr=0,bookmark_count=0,has_bookmarks=0; ctr < EOF_MAX_BOOKMARK_ENTRIES; ctr++)
	{
		if(sp->bookmark_pos[ctr] > 0)
		{	//If this bookmark exists
			bookmark_count++;
			has_bookmarks = 1;
		}
	}
	//Count the number of catalog entries
	if((sp->catalog != NULL) && (sp->catalog->entries > 0))
	{
		has_catalog = 1;
	}
	else
	{
		has_catalog = 0;
	}
	//Determine how many tracks need to be written
	track_count = sp->tracks;
	if((track_count == 0) && (bookmark_count || sp->catalog->entries))
	{	//Ensure that a global track is written if necessary to accommodate bookmarks and catalog entries
		track_count = 1;
	}
	pack_iputl(track_count, fp);	//Write the number of tracks
	for(track_ctr=0; track_ctr < track_count; track_ctr++)
	{	//For each track in the project
		if(sp->track[track_ctr] != NULL)
		{	//Skip NULL tracks, such as track 0
			eof_save_song_string_pf(sp->track[track_ctr]->name, fp);	//Write track name string
		}
		else
		{
			eof_save_song_string_pf(NULL, fp);	//Write empty string
		}
		//Write global track
		if(track_ctr == 0)
		{
			pack_putc(0, fp);	//Write track format (global)
			pack_putc(0, fp);	//Write track behavior (not used)
			pack_putc(0, fp);	//Write track type (not used)
			pack_putc(0, fp);	//Write track difficulty (not used)
			pack_iputl(0, fp);	//Write global track flags (not supported yet)
			pack_iputw(0xFFFF, fp);	//Write compliance flags (not used)
			//Write global track section type chunk
			pack_iputw(has_bookmarks + has_catalog, fp);	//Write number of section types
			if(has_bookmarks)
			{	//Write bookmarks
				pack_iputw(EOF_BOOKMARK_SECTION, fp);	//Write bookmark section type
				pack_iputl(bookmark_count, fp);			//Write number of bookmarks
				for(ctr=0; ctr < EOF_MAX_BOOKMARK_ENTRIES; ctr++)
				{	//For each bookmark in the project
					if(sp->bookmark_pos[ctr] > 0)
					{
						eof_save_song_string_pf(NULL, fp);		//Write an empty section name string (not supported yet)
						pack_putc(0xFF, fp);					//Write an associated difficulty of "all difficulties"
						pack_iputl(sp->bookmark_pos[ctr], fp);	//Write the bookmark's position
						pack_iputl(ctr, fp);					//Write end position (bookmark number)
						pack_iputl(0, fp);						//Write section flags (not used)
					}
				}
			}
			if(has_catalog)
			{	//Write fret catalog
				pack_iputw(EOF_FRET_CATALOG_SECTION, fp);	//Write fret catalog section type
				pack_iputl(sp->catalog->entries, fp);		//Write number of catalog entries
				for(ctr=0; ctr < sp->catalog->entries; ctr++)
				{	//For each fret catalog entry in the project
					eof_save_song_string_pf(sp->catalog->entry[ctr].name, fp);	//Write the section name string
					pack_putc(sp->catalog->entry[ctr].type, fp);				//Write the associated difficulty
					pack_iputl(sp->catalog->entry[ctr].start_pos, fp);			//Write the catalog entry's position
					pack_iputl(sp->catalog->entry[ctr].end_pos, fp);			//Write the catalog entry's end position
					pack_iputl(sp->catalog->entry[ctr].track, fp);				//Write the flags (associated track number)
				}
			}
		}
		//Write other tracks
		else
		{
			pack_putc(sp->track[track_ctr]->track_format, fp);		//Write track format
			pack_putc(sp->track[track_ctr]->track_behavior, fp);	//Write track behavior
			pack_putc(sp->track[track_ctr]->track_type, fp);		//Write track type
			pack_putc(sp->track[track_ctr]->difficulty, fp);		//Write track difficulty
			pack_iputl(sp->track[track_ctr]->flags, fp);			//Write track flags
			pack_iputw(0, fp);	//Write track compliance flags (not supported yet)

			tracknum = sp->track[track_ctr]->tracknum;
			has_solos = has_star_power = has_lyric_phrases = has_arpeggios = has_trills = has_tremolos = 0;
			switch(sp->track[track_ctr]->track_format)
			{	//Perform the appropriate logic to write this format of track
				case EOF_LEGACY_TRACK_FORMAT:	//Legacy (non pro guitar, non pro bass, non pro keys, pro or non pro drums)
					pack_putc(sp->legacy_track[tracknum]->numlanes, fp);	//Write the number of lanes/keys/etc. used in this track
					pack_iputl(sp->legacy_track[tracknum]->notes, fp);		//Write the number of notes in this track
					for(ctr=0; ctr < sp->legacy_track[tracknum]->notes; ctr++)
					{	//For each note in this track
						eof_save_song_string_pf(sp->legacy_track[tracknum]->note[ctr]->name, fp);	//Write the note's name
						pack_putc(sp->legacy_track[tracknum]->note[ctr]->type, fp);		//Write the note's difficulty
						pack_putc(sp->legacy_track[tracknum]->note[ctr]->note, fp);		//Write the note's bitflags
						pack_iputl(sp->legacy_track[tracknum]->note[ctr]->pos, fp);		//Write the note's position
						pack_iputl(sp->legacy_track[tracknum]->note[ctr]->length, fp);	//Write the note's length
						pack_iputl(sp->legacy_track[tracknum]->note[ctr]->flags, fp);	//Write the note's flags
					}
					//Write the section type chunk
					if(sp->legacy_track[tracknum]->solos)
					{
						has_solos = 1;
					}
					if(sp->legacy_track[tracknum]->star_power_paths)
					{
						has_star_power = 1;
					}
					if(sp->legacy_track[tracknum]->trills)
					{
						has_trills = 1;
					}
					if(sp->legacy_track[tracknum]->tremolos)
					{
						has_tremolos = 1;
					}
					pack_iputw(has_solos + has_star_power + has_trills + has_tremolos, fp);	//Write number of section types
					if(has_solos)
					{	//Write solo sections
						pack_iputw(EOF_SOLO_SECTION, fp);	//Write solo section type
						pack_iputl(sp->legacy_track[tracknum]->solos, fp);	//Write number of solo sections for this track
						for(ctr=0; ctr < sp->legacy_track[tracknum]->solos; ctr++)
						{	//For each solo section in the track
							eof_save_song_string_pf(NULL, fp);		//Write an empty section name string (not supported yet)
							pack_putc(0xFF, fp);					//Write an associated difficulty of "all difficulties"
							pack_iputl(sp->legacy_track[tracknum]->solo[ctr].start_pos, fp);	//Write the solo's position
							pack_iputl(sp->legacy_track[tracknum]->solo[ctr].end_pos, fp);		//Write the solo's end position
							pack_iputl(0, fp);						//Write section flags (not used)
						}
					}
					if(has_star_power)
					{	//Write star power sections
						pack_iputw(EOF_SP_SECTION, fp);		//Write star power section type
						pack_iputl(sp->legacy_track[tracknum]->star_power_paths, fp);	//Write number of star power sections for this track
						for(ctr=0; ctr < sp->legacy_track[tracknum]->star_power_paths; ctr++)
						{	//For each star power section in the track
							eof_save_song_string_pf(NULL, fp);		//Write an empty section name string (not supported yet)
							pack_putc(0xFF, fp);					//Write an associated difficulty of "all difficulties"
							pack_iputl(sp->legacy_track[tracknum]->star_power_path[ctr].start_pos, fp);	//Write the SP phrase's position
							pack_iputl(sp->legacy_track[tracknum]->star_power_path[ctr].end_pos, fp);	//Write the SP phrase's end position
							pack_iputl(0, fp);						//Write section flags (not used)
						}
					}
					if(has_trills)
					{	//Write trill sections
						pack_iputw(EOF_TRILL_SECTION, fp);		//Write trill section type
						pack_iputl(sp->legacy_track[tracknum]->trills, fp);	//Write number of trill sections for this track
						for(ctr=0; ctr < sp->legacy_track[tracknum]->trills; ctr++)
						{	//For each trill section in the track
							eof_save_song_string_pf(NULL, fp);		//Write an empty section name string (not supported yet)
							pack_putc(0xFF, fp);					//Write an associated difficulty of "all difficulties"
							pack_iputl(sp->legacy_track[tracknum]->trill[ctr].start_pos, fp);	//Write the trill phrase's position
							pack_iputl(sp->legacy_track[tracknum]->trill[ctr].end_pos, fp);		//Write the trill phrase's end position
							pack_iputl(0, fp);						//Write section flags (not used)
						}
					}
					if(has_tremolos)
					{	//Write tremolo sections
						pack_iputw(EOF_TREMOLO_SECTION, fp);		//Write tremolo section type
						pack_iputl(sp->legacy_track[tracknum]->tremolos, fp);	//Write number of tremolo sections for this track
						for(ctr=0; ctr < sp->legacy_track[tracknum]->tremolos; ctr++)
						{	//For each tremolo section in the track
							eof_save_song_string_pf(NULL, fp);		//Write an empty section name string (not supported yet)
							pack_putc(0xFF, fp);					//Write an associated difficulty of "all difficulties"
							pack_iputl(sp->legacy_track[tracknum]->tremolo[ctr].start_pos, fp);	//Write the tremolo phrase's position
							pack_iputl(sp->legacy_track[tracknum]->tremolo[ctr].end_pos, fp);		//Write the tremolo phrase's end position
							pack_iputl(0, fp);						//Write section flags (not used)
						}
					}
				break;
				case EOF_VOCAL_TRACK_FORMAT:	//Vocal
					pack_putc(0, fp);	//Write the tone set number assigned to this track (not supported yet)
					pack_iputl(sp->vocal_track[tracknum]->lyrics, fp);	//Write the number of lyrics in this track
					for(ctr=0; ctr < sp->vocal_track[tracknum]->lyrics; ctr++)
					{	//For each lyric in this track
						eof_save_song_string_pf(sp->vocal_track[tracknum]->lyric[ctr]->text, fp);	//Write the lyric string
						pack_putc(0, fp);	//Write lyric set number (not supported yet)
						pack_putc(sp->vocal_track[tracknum]->lyric[ctr]->note, fp);		//Write the lyric pitch
						pack_iputl(sp->vocal_track[tracknum]->lyric[ctr]->pos, fp);		//Write the lyric position
						pack_iputl(sp->vocal_track[tracknum]->lyric[ctr]->length, fp);	//Write the lyric length
						pack_iputl(0, fp);	//Write the lyric flags (not supported yet)
					}
					//Write the section type chunk
					if(sp->vocal_track[tracknum]->lines)
					{
						has_lyric_phrases = 1;
					}
					if(sp->vocal_track[tracknum]->star_power_paths)
					{
						has_star_power = 1;
					}
					pack_iputw(has_lyric_phrases + has_star_power, fp);	//Write number of section types
					if(has_lyric_phrases)
					{	//Write lyric phrases
						pack_iputw(EOF_LYRIC_PHRASE_SECTION, fp);	//Write lyric phrase section type
						pack_iputl(sp->vocal_track[tracknum]->lines, fp);	//Write number of star power sections for this track
						for(ctr=0; ctr < sp->vocal_track[tracknum]->lines; ctr++)
						{	//For each lyric phrase in the track
							eof_save_song_string_pf(NULL, fp);		//Write an empty section name string (not supported yet)
							pack_putc(0, fp);						//Write the associated difficulty (lyric set) (not supported yet)
							pack_iputl(sp->vocal_track[tracknum]->line[ctr].start_pos, fp);	//Write the lyric phrase's position
							pack_iputl(sp->vocal_track[tracknum]->line[ctr].end_pos, fp);	//Write the lyric phrase's end position
							pack_iputl(0, fp);						//Write section flags (not used)
						}
					}
					if(has_star_power)
					{	//Write star power sections
						pack_iputw(EOF_SP_SECTION, fp);		//Write star power section type
						pack_iputl(sp->vocal_track[tracknum]->star_power_paths, fp);	//Write number of star power sections for this track
						for(ctr=0; ctr < sp->vocal_track[tracknum]->star_power_paths; ctr++)
						{	//For each solo section in the track
							eof_save_song_string_pf(NULL, fp);		//Write an empty section name string (not supported yet)
							pack_putc(0xFF, fp);					//Write an associated difficulty of "all difficulties"
							pack_iputl(sp->vocal_track[tracknum]->star_power_path[ctr].start_pos, fp);	//Write the SP phrase's position
							pack_iputl(sp->vocal_track[tracknum]->star_power_path[ctr].end_pos, fp);	//Write the SP phrase's end position
							pack_iputl(0, fp);						//Write section flags (not used)
						}
					}
				break;
				case EOF_PRO_KEYS_TRACK_FORMAT:	//Pro Keys
					allegro_message("Error: Pro Keys not supported yet.  Aborting");
				return 0;
				case EOF_PRO_GUITAR_TRACK_FORMAT:	//Pro Guitar/Bass
					pack_putc(sp->pro_guitar_track[tracknum]->numfrets, fp);	//Write the number of frets used in this track
					pack_putc(sp->pro_guitar_track[tracknum]->numstrings, fp);	//Write the number of strings used in this track
					for(ctr=0; ctr < sp->pro_guitar_track[tracknum]->numstrings; ctr++)
					{	//For each string
						if(sp->pro_guitar_track[tracknum]->tuning != NULL)
						{	//If the tuning array is defined
							pack_putc(sp->pro_guitar_track[tracknum]->tuning[ctr], fp);	//Write this string's tuning value
						}
						else
						{
							pack_putc(0, fp);	//Otherwise write 0 (undefined tuning)
						}
					}
					pack_iputl(sp->pro_guitar_track[tracknum]->notes, fp);					//Write the number of notes in this track
					for(ctr=0; ctr < sp->pro_guitar_track[tracknum]->notes; ctr++)
					{	//For each note in this track
						eof_save_song_string_pf(sp->pro_guitar_track[tracknum]->note[ctr]->name, fp);	//Write the note's name
						pack_putc(0, fp);													//Write the chord's number (not supported yet)
						pack_putc(sp->pro_guitar_track[tracknum]->note[ctr]->type, fp);		//Write the note's difficulty
						pack_putc(sp->pro_guitar_track[tracknum]->note[ctr]->note, fp);		//Write the note's bitflags
						pack_putc(0, fp);	//Write the note's ghost bitflags (not supported yet)
						for(ctr2=0, bitmask=1; ctr2 < 16; ctr2++, bitmask <<= 1)
						{	//For each of the 16 bits in the note bitflag
							if(sp->pro_guitar_track[tracknum]->note[ctr]->note & bitmask)
							{	//If this bit is set
								pack_putc(sp->pro_guitar_track[tracknum]->note[ctr]->frets[ctr2], fp);	//Write this string's fret value
							}
						}
						pack_putc(sp->pro_guitar_track[tracknum]->note[ctr]->legacymask, fp);	//Write the legacy note bitmask
						pack_iputl(sp->pro_guitar_track[tracknum]->note[ctr]->pos, fp);			//Write the note's position
						pack_iputl(sp->pro_guitar_track[tracknum]->note[ctr]->length, fp);		//Write the note's length
						pack_iputl(sp->pro_guitar_track[tracknum]->note[ctr]->flags, fp);		//Write the note's flags
					}
					//Write the section type chunk
					if(sp->pro_guitar_track[tracknum]->solos)
					{
						has_solos = 1;
					}
					if(sp->pro_guitar_track[tracknum]->star_power_paths)
					{
						has_star_power = 1;
					}
					if(sp->pro_guitar_track[tracknum]->arpeggios)
					{
						has_arpeggios = 1;
					}
					if(sp->pro_guitar_track[tracknum]->trills)
					{
						has_trills = 1;
					}
					if(sp->pro_guitar_track[tracknum]->tremolos)
					{
						has_tremolos = 1;
					}
					pack_iputw(has_solos + has_star_power + has_arpeggios + has_trills + has_tremolos, fp);		//Write the number of section types
					if(has_solos)
					{	//Write solo sections
						pack_iputw(EOF_SOLO_SECTION, fp);			//Write solo section type
						pack_iputl(sp->pro_guitar_track[tracknum]->solos, fp);	//Write number of solo sections for this track
						for(ctr=0; ctr < sp->pro_guitar_track[tracknum]->solos; ctr++)
						{	//For each solo section in the track
							eof_save_song_string_pf(NULL, fp);		//Write an empty section name string (not supported yet)
							pack_putc(0xFF, fp);					//Write an associated difficulty of "all difficulties"
							pack_iputl(sp->pro_guitar_track[tracknum]->solo[ctr].start_pos, fp);	//Write the solo's position
							pack_iputl(sp->pro_guitar_track[tracknum]->solo[ctr].end_pos, fp);		//Write the solo's end position
							pack_iputl(0, fp);						//Write section flags (not used)
						}
					}
					if(has_star_power)
					{	//Write star power sections
						pack_iputw(EOF_SP_SECTION, fp);				//Write star power section type
						pack_iputl(sp->pro_guitar_track[tracknum]->star_power_paths, fp);	//Write number of star power sections for this track
						for(ctr=0; ctr < sp->pro_guitar_track[tracknum]->star_power_paths; ctr++)
						{	//For each star power section in the track
							eof_save_song_string_pf(NULL, fp);		//Write an empty section name string (not supported yet)
							pack_putc(0xFF, fp);					//Write an associated difficulty of "all difficulties"
							pack_iputl(sp->pro_guitar_track[tracknum]->star_power_path[ctr].start_pos, fp);	//Write the SP phrase's position
							pack_iputl(sp->pro_guitar_track[tracknum]->star_power_path[ctr].end_pos, fp);	//Write the SP phrase's end position
							pack_iputl(0, fp);						//Write section flags (not used)
						}
					}
					if(has_arpeggios)
					{	//Write arpeggio section
						pack_iputw(EOF_ARPEGGIO_SECTION, fp);		//Write arpeggio section type
						pack_iputl(sp->pro_guitar_track[tracknum]->arpeggios, fp);	//Write number of arpeggio sections for this track
						for(ctr=0; ctr < sp->pro_guitar_track[tracknum]->arpeggios; ctr++)
						{	//For each arpegio section in the track
							eof_save_song_string_pf(NULL, fp);		//Write an empty section name string (not supported yet)
							pack_putc(0xFF, fp);					//Write an associated difficulty of "all difficulties"
							pack_iputl(sp->pro_guitar_track[tracknum]->arpeggio[ctr].start_pos, fp);	//Write the arpeggio phrase's position
							pack_iputl(sp->pro_guitar_track[tracknum]->arpeggio[ctr].end_pos, fp);		//Write the arpeggio phrase's end position
							pack_iputl(0, fp);						//Write section flags (not used)
						}
					}
					if(has_trills)
					{	//Write trill sections
						pack_iputw(EOF_TRILL_SECTION, fp);		//Write trill section type
						pack_iputl(sp->pro_guitar_track[tracknum]->trills, fp);	//Write number of trill sections for this track
						for(ctr=0; ctr < sp->pro_guitar_track[tracknum]->trills; ctr++)
						{	//For each trill section in the track
							eof_save_song_string_pf(NULL, fp);		//Write an empty section name string (not supported yet)
							pack_putc(0xFF, fp);					//Write an associated difficulty of "all difficulties"
							pack_iputl(sp->pro_guitar_track[tracknum]->trill[ctr].start_pos, fp);	//Write the trill phrase's position
							pack_iputl(sp->pro_guitar_track[tracknum]->trill[ctr].end_pos, fp);		//Write the trill phrase's end position
							pack_iputl(0, fp);						//Write section flags (not used)
						}
					}
					if(has_tremolos)
					{	//Write tremolo sections
						pack_iputw(EOF_TREMOLO_SECTION, fp);		//Write tremolo section type
						pack_iputl(sp->pro_guitar_track[tracknum]->tremolos, fp);	//Write number of tremolo sections for this track
						for(ctr=0; ctr < sp->pro_guitar_track[tracknum]->tremolos; ctr++)
						{	//For each tremolo section in the track
							eof_save_song_string_pf(NULL, fp);		//Write an empty section name string (not supported yet)
							pack_putc(0xFF, fp);					//Write an associated difficulty of "all difficulties"
							pack_iputl(sp->pro_guitar_track[tracknum]->tremolo[ctr].start_pos, fp);	//Write the tremolo phrase's position
							pack_iputl(sp->pro_guitar_track[tracknum]->tremolo[ctr].end_pos, fp);		//Write the tremolo phrase's end position
							pack_iputl(0, fp);						//Write section flags (not used)
						}
					}
				break;//Pro Guitar/Bass
				case EOF_PRO_VARIABLE_LEGACY_TRACK_FORMAT:	//Variable Lane Legacy
					allegro_message("Error: Variable lane not supported yet.  Aborting");
				return 0;
				default://Unknown track type
					allegro_message("Error: Unsupported track type.  Aborting");
				return 0;
			}
		}
//		pack_iputl(0, fp);	//Write an empty custom data block (not currently in use)
		pack_iputl(1, fp);			//Write a debug custom data block
		pack_iputl(4, fp);
		pack_iputl(0xFFFFFFFF, fp);
	}

	pack_fclose(fp);
	return 1;	//Return success
}

EOF_SONG * eof_create_song_populated(void)
{
	EOF_SONG * sp = NULL;
	unsigned long ctr;

	/* create empty song */
	sp = eof_create_song();
	if(sp != NULL)
	{
		for(ctr = 1; ctr < EOF_TRACKS_MAX; ctr++)
		{	//For each track in the eof_default_tracks[] array
			if(eof_song_add_track(sp,&eof_default_tracks[ctr]) == 0)
				return NULL;
		}
	}

	return sp;
}

unsigned long eof_count_track_lanes(EOF_SONG *sp, unsigned long track)
{
	if((track == 0) || (track > EOF_TRACKS_MAX) || (sp == NULL))
		return 5;	//Return default value if the specified track doesn't exist

	if(sp->track[track]->track_format == EOF_LEGACY_TRACK_FORMAT)
	{	//If this is a legacy track, return the number of lanes it uses
		return sp->legacy_track[eof_song->track[track]->tracknum]->numlanes;
	}
	else if(sp->track[track]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT)
	{
		if(eof_legacy_view)
		{	//If the legacy view is in effect, force pro guitar tracks to render as 5 lane tracks
			return 5;
		}
		else
		{
			return sp->pro_guitar_track[eof_song->track[track]->tracknum]->numstrings;
		}
	}
	else
	{	//Otherwise return 5, as so far, other track formats don't store this information
		return 5;
	}
}

inline int eof_open_bass_enabled(void)
{
	return (eof_song->track[EOF_TRACK_BASS]->flags & EOF_TRACK_FLAG_OPEN_STRUM);
}

EOF_PRO_GUITAR_NOTE *eof_pro_guitar_track_add_note(EOF_PRO_GUITAR_TRACK *tp)
{
	if(tp == NULL)
		return NULL;

	if(tp->notes < EOF_MAX_NOTES)
	{
		tp->note[tp->notes] = malloc(sizeof(EOF_PRO_GUITAR_NOTE));
		if(tp->note[tp->notes])
		{
			memset(tp->note[tp->notes], 0, sizeof(EOF_PRO_GUITAR_NOTE));
			tp->notes++;
			return tp->note[tp->notes - 1];
		}
	}
	return NULL;
}

unsigned long eof_get_track_size(EOF_SONG *sp, unsigned long track)
{
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return 0;
	tracknum = sp->track[track]->tracknum;

	switch(sp->track[track]->track_format)
	{
		case EOF_LEGACY_TRACK_FORMAT:
		return sp->legacy_track[tracknum]->notes;

		case EOF_VOCAL_TRACK_FORMAT:
		return sp->vocal_track[tracknum]->lyrics;

		case EOF_PRO_GUITAR_TRACK_FORMAT:
		return sp->pro_guitar_track[tracknum]->notes;
	}

	return 0;
}

void *eof_track_add_note(EOF_SONG *sp, unsigned long track)
{
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return NULL;
	tracknum = sp->track[track]->tracknum;

	switch(sp->track[track]->track_format)
	{
		case EOF_LEGACY_TRACK_FORMAT:
		return eof_legacy_track_add_note(sp->legacy_track[tracknum]);

		case EOF_VOCAL_TRACK_FORMAT:
		return eof_vocal_track_add_lyric(sp->vocal_track[tracknum]);

		case EOF_PRO_GUITAR_TRACK_FORMAT:
		return eof_pro_guitar_track_add_note(sp->pro_guitar_track[tracknum]);
	}

	return NULL;
}

void eof_track_delete_note(EOF_SONG *sp, unsigned long track, unsigned long note)
{
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return;
	tracknum = sp->track[track]->tracknum;

	if(note < eof_get_track_size(sp, track))
	{
		switch(sp->track[track]->track_format)
		{
			case EOF_LEGACY_TRACK_FORMAT:
				eof_legacy_track_delete_note(sp->legacy_track[tracknum], note);
			break;

			case EOF_VOCAL_TRACK_FORMAT:
				eof_vocal_track_delete_lyric(sp->vocal_track[tracknum], note);
			break;

			case EOF_PRO_GUITAR_TRACK_FORMAT:
				eof_pro_guitar_track_delete_note(sp->pro_guitar_track[tracknum], note);
			break;
		}
	}
}

void eof_track_resize(EOF_SONG *sp, unsigned long track, unsigned long size)
{
	unsigned long i, oldsize;

	if((sp == NULL) || (track >= sp->tracks))
		return;
	oldsize = eof_get_track_size(sp, track);

	if(size > oldsize)
	{	//If this track is being grown
		for(i=oldsize; i < size; i++)
		{
			eof_track_add_note(sp, track);
		}
	}
	else if(size < oldsize)
	{	//If this track is being shrunk
		for(i=oldsize; i > size; i--)
		{	//Delete notes from the end of the array
			eof_track_delete_note(sp, track, i-1);
		}
	}
}

char eof_get_note_type(EOF_SONG *sp, unsigned long track, unsigned long note)
{
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return 0xFF;	//Return error
	tracknum = sp->track[track]->tracknum;

	switch(sp->track[track]->track_format)
	{
		case EOF_LEGACY_TRACK_FORMAT:
			if(note < sp->legacy_track[tracknum]->notes)
			{
				return sp->legacy_track[tracknum]->note[note]->type;
			}
		break;

		case EOF_VOCAL_TRACK_FORMAT:
			if(note < sp->vocal_track[tracknum]->lyrics)
			{
				return sp->vocal_track[tracknum]->lyric[note]->type;
			}
		break;

		case EOF_PRO_GUITAR_TRACK_FORMAT:
			if(note < sp->pro_guitar_track[tracknum]->notes)
			{
				return sp->pro_guitar_track[tracknum]->note[note]->type;
			}
		break;
	}

	return 0xFF;	//Return error
}

unsigned long eof_get_note_pos(EOF_SONG *sp, unsigned long track, unsigned long note)
{
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return 0;	//Return error
	tracknum = sp->track[track]->tracknum;

	switch(sp->track[track]->track_format)
	{
		case EOF_LEGACY_TRACK_FORMAT:
			if(note < sp->legacy_track[tracknum]->notes)
			{
				return sp->legacy_track[tracknum]->note[note]->pos;
			}
		break;

		case EOF_VOCAL_TRACK_FORMAT:
			if(note < sp->vocal_track[tracknum]->lyrics)
			{
				return sp->vocal_track[tracknum]->lyric[note]->pos;
			}
		break;

		case EOF_PRO_GUITAR_TRACK_FORMAT:
			if(note < sp->pro_guitar_track[tracknum]->notes)
			{
				return sp->pro_guitar_track[tracknum]->note[note]->pos;
			}
		break;
	}

	return 0;	//Return error
}

long eof_get_note_length(EOF_SONG *sp, unsigned long track, unsigned long note)
{
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return 0;	//Return error
	tracknum = sp->track[track]->tracknum;

	switch(sp->track[track]->track_format)
	{
		case EOF_LEGACY_TRACK_FORMAT:
			if(note < sp->legacy_track[tracknum]->notes)
			{
				return sp->legacy_track[tracknum]->note[note]->length;
			}
		break;

		case EOF_VOCAL_TRACK_FORMAT:
			if(note < sp->vocal_track[tracknum]->lyrics)
			{
				return sp->vocal_track[tracknum]->lyric[note]->length;
			}
		break;

		case EOF_PRO_GUITAR_TRACK_FORMAT:
			if(note < sp->pro_guitar_track[tracknum]->notes)
			{
				return sp->pro_guitar_track[tracknum]->note[note]->length;
			}
		break;
	}

	return 0;	//Return error
}

void eof_set_note_length(EOF_SONG *sp, unsigned long track, unsigned long note, long length)
{
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return;
	tracknum = sp->track[track]->tracknum;

	switch(sp->track[track]->track_format)
	{
		case EOF_LEGACY_TRACK_FORMAT:
			if(note < sp->legacy_track[tracknum]->notes)
			{
				sp->legacy_track[tracknum]->note[note]->length = length;
			}
		break;

		case EOF_VOCAL_TRACK_FORMAT:
			if(note < sp->vocal_track[tracknum]->lyrics)
			{
				sp->vocal_track[tracknum]->lyric[note]->length = length;
			}
		break;

		case EOF_PRO_GUITAR_TRACK_FORMAT:
			if(note < sp->pro_guitar_track[tracknum]->notes)
			{
				sp->pro_guitar_track[tracknum]->note[note]->length = length;
			}
		break;
	}
}

unsigned long eof_get_note_flags(EOF_SONG *sp, unsigned long track, unsigned long note)
{
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return 0;	//Return error
	tracknum = sp->track[track]->tracknum;

	switch(sp->track[track]->track_format)
	{
		case EOF_LEGACY_TRACK_FORMAT:
			if(note < sp->legacy_track[tracknum]->notes)
			{
				return sp->legacy_track[tracknum]->note[note]->flags;
			}
		break;

		case EOF_VOCAL_TRACK_FORMAT:
			if(note < sp->vocal_track[tracknum]->lyrics)
			{
				return sp->vocal_track[tracknum]->lyric[note]->flags;
			}
		break;

		case EOF_PRO_GUITAR_TRACK_FORMAT:
			if(note < sp->pro_guitar_track[tracknum]->notes)
			{
				return sp->pro_guitar_track[tracknum]->note[note]->flags;
			}
		break;
	}

	return 0;	//Return error
}

unsigned long eof_get_note_note(EOF_SONG *sp, unsigned long track, unsigned long note)
{
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return 0;	//Return error
	tracknum = sp->track[track]->tracknum;

	switch(sp->track[track]->track_format)
	{
		case EOF_LEGACY_TRACK_FORMAT:
			if(note < sp->legacy_track[tracknum]->notes)
			{
				return sp->legacy_track[tracknum]->note[note]->note;
			}
		break;

		case EOF_VOCAL_TRACK_FORMAT:
			if(note < sp->vocal_track[tracknum]->lyrics)
			{
				return sp->vocal_track[tracknum]->lyric[note]->note;
			}
		break;

		case EOF_PRO_GUITAR_TRACK_FORMAT:
			if(note < sp->pro_guitar_track[tracknum]->notes)
			{
				return sp->pro_guitar_track[tracknum]->note[note]->note;
			}
		break;
	}

	return 0;	//Return error
}

void *eof_track_add_create_note(EOF_SONG *sp, unsigned long track, unsigned long note, unsigned long pos, long length, char type, char *text)
{
	void *new_note = NULL;
	EOF_NOTE *ptr = NULL;
	EOF_LYRIC *ptr2 = NULL;
	EOF_PRO_GUITAR_NOTE *ptr3 = NULL;

	new_note = eof_track_add_note(sp, track);
	if(new_note != NULL)
	{
		if(length < 1)
		{
			length = 1;	//1 is the minimum length of any note/lyric
		}
		switch(sp->track[track]->track_format)
		{
			case EOF_LEGACY_TRACK_FORMAT:
				ptr = (EOF_NOTE *)new_note;
				ptr->type = type;
				ptr->note = note;
				ptr->midi_pos = 0;	//Not implemented yet
				ptr->midi_length = 0;	//Not implemented yet
				ptr->pos = pos;
				ptr->length = length;
				ptr->flags = 0;
				if(text != NULL)
				{
					ustrncpy(ptr->name, text, EOF_NAME_LENGTH);
				}
				else
				{
					ptr->name[0] = '\0';	//Empty the string
				}
				if(!((eof_count_track_lanes(sp, track) > 5) && (track != EOF_TRACK_BASS)))
				{	//If the track storing the new note does not have six lanes (with the exclusion of the bass track's open strum lane)
					note &= ~32;	//Clear lane 6
				}
				if(sp->track[track]->track_behavior == EOF_KEYS_TRACK_BEHAVIOR)
				{	//In a keys track, all lanes are forced to be "crazy" and be allowed to overlap other lanes
					ptr->flags |= EOF_NOTE_FLAG_CRAZY;	//Set the crazy flag bit
				}
			return ptr;

			case EOF_VOCAL_TRACK_FORMAT:
				ptr2 = (EOF_LYRIC *)new_note;
				ptr2->type = type;
				ptr2->note = note;
				if(text != NULL)
				{
					ustrncpy(ptr2->text, text, EOF_MAX_LYRIC_LENGTH);
				}
				else
				{
					ptr2->text[0] = '\0';	//Empty the string
				}
				ptr2->midi_pos = 0;	//Not implemented yet
				ptr2->midi_length = 0;	//Not implemented yet
				ptr2->pos = pos;
				ptr2->length = length;
				ptr2->flags = 0;
			return ptr2;

			case EOF_PRO_GUITAR_TRACK_FORMAT:
				ptr3 = (EOF_PRO_GUITAR_NOTE *)new_note;
				ptr3->number = 0;	//Not implemented yet
				if(text != NULL)
				{
					ustrncpy(ptr3->name, text, EOF_NAME_LENGTH);
				}
				else
				{
					ptr3->name[0] = '\0';	//Empty the string
				}
				ptr3->type = type;
				ptr3->note = note;
				memset(ptr3->frets, 0, 16);	//Initialize all frets to open
				ptr3->midi_pos = 0;	//Not implemented yet
				ptr3->midi_length = 0;	//Not implemented yet
				ptr3->pos = pos;
				ptr3->length = length;
				ptr3->flags = 0;
				if(eof_legacy_view)
				{	//If legacy view is in effect, initialize the legacy bitmask to whichever lanes (1-5) created the note
					ptr3->legacymask = note & 31;
				}
				else
				{	//Otherwise initialize the legacy bitmask to zero
					ptr3->legacymask = 0;
				}
			return ptr3;
		}
	}

	return NULL;	//Return error
}

void *eof_track_add_create_note2(EOF_SONG *sp, unsigned long track, EOF_NOTE *note)
{
	if(note == NULL)
	{
		return NULL;
	}

	return eof_track_add_create_note(sp, track, note->note, note->pos, note->length, note->type, note->name);
}

unsigned long eof_get_num_solos(EOF_SONG *sp, unsigned long track)
{
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return 0;	//Return error
	tracknum = sp->track[track]->tracknum;

	switch(sp->track[track]->track_format)
	{
		case EOF_LEGACY_TRACK_FORMAT:
		return sp->legacy_track[tracknum]->solos;

		case EOF_PRO_GUITAR_TRACK_FORMAT:
		return sp->pro_guitar_track[tracknum]->solos;
	}

	return 0;	//Return error
}

EOF_PHRASE_SECTION *eof_get_solo(EOF_SONG *sp, unsigned long track, unsigned long solonum)
{
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return NULL;	//Return error
	tracknum = sp->track[track]->tracknum;

	switch(sp->track[track]->track_format)
	{
		case EOF_LEGACY_TRACK_FORMAT:
			if(solonum < EOF_MAX_PHRASES)
			{
				return &sp->legacy_track[tracknum]->solo[solonum];
			}
		break;


		case EOF_PRO_GUITAR_TRACK_FORMAT:
			if(solonum < EOF_MAX_PHRASES)
			{
				return &sp->pro_guitar_track[tracknum]->solo[solonum];
			}
		break;
	}

	return NULL;	//Return error
}

void eof_set_note_pos(EOF_SONG *sp, unsigned long track, unsigned long note, unsigned long pos)
{
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return;
	tracknum = sp->track[track]->tracknum;

	switch(sp->track[track]->track_format)
	{
		case EOF_LEGACY_TRACK_FORMAT:
			if(note < sp->legacy_track[tracknum]->notes)
			{
				sp->legacy_track[tracknum]->note[note]->pos = pos;
			}
		break;

		case EOF_VOCAL_TRACK_FORMAT:
			if(note < sp->vocal_track[tracknum]->lyrics)
			{
				sp->vocal_track[tracknum]->lyric[note]->pos = pos;
			}
		break;

		case EOF_PRO_GUITAR_TRACK_FORMAT:
			if(note < sp->pro_guitar_track[tracknum]->notes)
			{
				sp->pro_guitar_track[tracknum]->note[note]->pos = pos;
			}
		break;
	}
}

void eof_set_note_note(EOF_SONG *sp, unsigned long track, unsigned long note, unsigned long value)
{
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return;
	tracknum = sp->track[track]->tracknum;

	switch(sp->track[track]->track_format)
	{
		case EOF_LEGACY_TRACK_FORMAT:
			if(note < sp->legacy_track[tracknum]->notes)
			{
				sp->legacy_track[tracknum]->note[note]->note = value;
			}
		break;

		case EOF_VOCAL_TRACK_FORMAT:
			if(note < sp->vocal_track[tracknum]->lyrics)
			{
				sp->vocal_track[tracknum]->lyric[note]->note = value;
			}
		break;

		case EOF_PRO_GUITAR_TRACK_FORMAT:
			if(note < sp->pro_guitar_track[tracknum]->notes)
			{
				sp->pro_guitar_track[tracknum]->note[note]->note = value;
			}
		break;
	}
}

void eof_track_sort_notes(EOF_SONG *sp, unsigned long track)
{
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks) || (track == 0))
		return;
	tracknum = sp->track[track]->tracknum;

	switch(sp->track[track]->track_format)
	{
		case EOF_LEGACY_TRACK_FORMAT:
			eof_legacy_track_sort_notes(sp->legacy_track[tracknum]);
		break;

		case EOF_VOCAL_TRACK_FORMAT:
			eof_vocal_track_sort_lyrics(sp->vocal_track[tracknum]);
		break;

		case EOF_PRO_GUITAR_TRACK_FORMAT:
			eof_pro_guitar_track_sort_notes(sp->pro_guitar_track[tracknum]);
		break;
	}
}

void eof_track_fixup_notes(EOF_SONG *sp, unsigned long track, int sel)
{
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return;
	tracknum = sp->track[track]->tracknum;

	switch(sp->track[track]->track_format)
	{
		case EOF_LEGACY_TRACK_FORMAT:
			eof_legacy_track_fixup_notes(sp->legacy_track[tracknum], sel);
		break;

		case EOF_VOCAL_TRACK_FORMAT:
			eof_vocal_track_fixup_lyrics(sp->vocal_track[tracknum], sel);
		break;

		case EOF_PRO_GUITAR_TRACK_FORMAT:
			eof_pro_guitar_track_fixup_notes(sp->pro_guitar_track[tracknum], sel);
		break;
	}
}

void eof_pro_guitar_track_sort_notes(EOF_PRO_GUITAR_TRACK * tp)
{
	qsort(tp->note, tp->notes, sizeof(EOF_PRO_GUITAR_NOTE *), eof_song_qsort_pro_guitar_notes);
}

/* sort all notes according to position */
int eof_song_qsort_pro_guitar_notes(const void * e1, const void * e2)
{
    EOF_PRO_GUITAR_NOTE ** thing1 = (EOF_PRO_GUITAR_NOTE **)e1;
    EOF_PRO_GUITAR_NOTE ** thing2 = (EOF_PRO_GUITAR_NOTE **)e2;

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

void eof_pro_guitar_track_delete_note(EOF_PRO_GUITAR_TRACK * tp, unsigned long note)
{
	unsigned long i;

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

long eof_fixup_next_pro_guitar_note(EOF_PRO_GUITAR_TRACK * tp, unsigned long note)
{
	long i;

	for(i = note + 1; i < tp->notes; i++)
	{
		if(tp->note[i]->type == tp->note[note]->type)
		{
			return i;
		}
	}
	return -1;
}

long eof_get_prev_note_type_num(EOF_SONG *sp, unsigned long track, unsigned long note)
{
	long i;

	for(i = note - 1; i >= 0; i--)
	{	//For each note before the specified note number, starting with the one immediately before it
		if(eof_get_note_type(sp, track, note) == eof_get_note_type(sp, track, i))
		{	//If this note has the same type/difficulty
			return i;
		}
	}
	return -1;	//Return note not found
}

void eof_pro_guitar_track_fixup_notes(EOF_PRO_GUITAR_TRACK * tp, int sel)
{
	unsigned long i, ctr, bitmask;
	unsigned char fretvalue;
	long next;
	char allmuted;	//Used to track whether all used strings are string muted

	if(!sel)
	{
		if(eof_selection.current < tp->notes)
		{
			eof_selection.multi[eof_selection.current] = 0;
		}
		eof_selection.current = EOF_MAX_NOTES - 1;
	}
	for(i = tp->notes; i > 0; i--)
	{	//For each note in the track
		/* ensure notes within an arpeggio phrase are marked as "crazy" */
		for(ctr = 0; ctr < tp->arpeggios; ctr++)
		{	//For each arpeggio section in this track
			if((tp->note[i-1]->pos >= tp->arpeggio[ctr].start_pos) && (tp->note[i-1]->pos <= tp->arpeggio[ctr].end_pos))
			{	//If this note is in an arpeggio phrase
				tp->note[i-1]->flags |= EOF_NOTE_FLAG_CRAZY;	//Set the crazy flag bit
				break;
			}
		}

		/* fix selections */
		if((tp->note[i-1]->type == eof_note_type) && (tp->note[i-1]->pos == eof_selection.current_pos))
		{
			eof_selection.current = i-1;
		}
		if((tp->note[i-1]->type == eof_note_type) && (tp->note[i-1]->pos == eof_selection.last_pos))
		{
			eof_selection.last = i-1;
		}

		/* delete certain notes */
		if((tp->note[i-1]->note == 0) || ((tp->note[i-1]->type < 0) || (tp->note[i-1]->type > 4)) || (tp->note[i-1]->pos < eof_song->tags->ogg[eof_selected_ogg].midi_offset) || (tp->note[i-1]->pos >= eof_music_length))
		{	//If the note is not valid
			eof_pro_guitar_track_delete_note(tp, i-1);
		}
		else
		{	//If the note is valid, perform other cleanup
			/* make sure there are no 0-length notes */
			if(tp->note[i-1]->length <= 0)
			{
				tp->note[i-1]->length = 1;
			}

			/* make sure note doesn't extend past end of song */
			if(tp->note[i-1]->pos + tp->note[i-1]->length >= eof_music_length)
			{
				tp->note[i-1]->length = eof_music_length - tp->note[i-1]->pos;
			}

			/* compare this note to the next one of the same type
			   to make sure they don't overlap */
			next = eof_fixup_next_pro_guitar_note(tp, i-1);
			if(next >= 0)
			{	//If there is another note in this track
				if(tp->note[i-1]->pos == tp->note[next]->pos)
				{	//If this note and the next are at the same position
					tp->note[i-1]->note |= tp->note[next]->note;	//Merge the two notes' bitmasks
					for(ctr = 0, bitmask = 1; ctr < 6; ctr++, bitmask<<=1)
					{	//For each of the next note's 6 usable strings
						if(tp->note[next]->note & bitmask)
						{	//If this string is used
							tp->note[i-1]->frets[ctr] = tp->note[next]->frets[ctr];	//Overwrite this note's fret value on this string with that of the next note
						}
					}
					eof_pro_guitar_track_delete_note(tp, next);
				}
				else if(tp->note[i-1]->pos + tp->note[i-1]->length > tp->note[next]->pos - 1)
				{	//If this note overlaps the next note
					if(!(tp->note[i-1]->flags & EOF_NOTE_FLAG_CRAZY) || (tp->note[i-1]->note & tp->note[next]->note))
					{	//If this note isn't a "crazy" note or is overlapping the next note on the same lanes it uses
						tp->note[i-1]->length = tp->note[next]->pos - tp->note[i-1]->pos - 1;	//Alter the length to reach the next note
					}
				}
			}

			/* make sure that there aren't any invalid fret values, and inspect the mute flag status */
			for(ctr = 0, allmuted = 1, bitmask = 1; ctr < 6; ctr++, bitmask<<=1)
			{	//For each of the 6 usable strings
				fretvalue = tp->note[i-1]->frets[ctr];
				if(fretvalue != 0xFF)
				{	//Track whether the all used strings in this note/chord are muted
					if(tp->note[i-1]->note & bitmask)
					{	//If this string is used in the note
						allmuted = 0;
					}
					if(fretvalue > tp->numfrets)
					{	//If this fret value is invalid
						tp->note[i-1]->frets[ctr] = 0;	//Revert to default fret value of 0
					}
				}
			}

			/* correct the mute flag status if necessary */
			if(!allmuted)
			{	//If any used strings in this note/chord weren't string muted
				tp->note[i-1]->flags &= (~EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE);	//Clear the string mute flag
			}
			else if(!(tp->note[i-1]->flags & EOF_PRO_GUITAR_NOTE_FLAG_PALM_MUTE))
			{	//If all strings are muted and the user didn't specify a palm mute
				tp->note[i-1]->flags |= EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE;		//Set the string mute flag
			}

			/* ensure that a note isn't both ghosted AND string muted/hammer on */
			if(((tp->note[i-1]->flags & EOF_PRO_GUITAR_NOTE_FLAG_STRING_MUTE) || (tp->note[i-1]->flags & EOF_PRO_GUITAR_NOTE_FLAG_HO)) && tp->note[i-1]->ghost)
			{	//If this note is string muted and any strings are ghosted
				tp->note[i-1]->ghost = 0;	//Remove ghost status from all strings
			}
		}//If the note is valid, perform other cleanup
	}//For each note in the track

	for(i = 0; i < tp->tremolos; i++)
	{	//For each tremolo section in the track
		for(ctr = 0; ctr < tp->notes; ctr++)
		{	//For each note in the track
			if((tp->note[ctr]->pos >= tp->tremolo[i].start_pos) && (tp->note[ctr]->pos <= tp->tremolo[i].end_pos))
			{	//If the note is within a tremolo phrase
				tp->note[ctr]->flags &= ~(EOF_PRO_GUITAR_NOTE_FLAG_HO);	//Clear the hammer on flag, which RB3 charts set needlessly
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
}

unsigned long eof_get_num_star_power_paths(EOF_SONG *sp, unsigned long track)
{
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return 0;	//Return error
	tracknum = sp->track[track]->tracknum;

	switch(sp->track[track]->track_format)
	{
		case EOF_LEGACY_TRACK_FORMAT:
		return sp->legacy_track[tracknum]->star_power_paths;

		case EOF_PRO_GUITAR_TRACK_FORMAT:
		return sp->pro_guitar_track[tracknum]->star_power_paths;
	}

	return 0;	//Return error
}

EOF_PHRASE_SECTION *eof_get_star_power_path(EOF_SONG *sp, unsigned long track, unsigned long pathnum)
{
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return NULL;	//Return error
	tracknum = sp->track[track]->tracknum;

	switch(sp->track[track]->track_format)
	{
		case EOF_LEGACY_TRACK_FORMAT:
			if(pathnum < EOF_MAX_PHRASES)
			{
				return &sp->legacy_track[tracknum]->star_power_path[pathnum];
			}
		break;

		case EOF_VOCAL_TRACK_FORMAT:
		return NULL;	//Vocal star power is not implemented yet

		case EOF_PRO_GUITAR_TRACK_FORMAT:
			if(pathnum < EOF_MAX_PHRASES)
			{
				return &sp->pro_guitar_track[tracknum]->star_power_path[pathnum];
			}
		break;
	}

	return NULL;	//Return error
}

void eof_set_num_solos(EOF_SONG *sp, unsigned long track, unsigned long number)
{
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return;
	tracknum = sp->track[track]->tracknum;

	switch(sp->track[track]->track_format)
	{
		case EOF_LEGACY_TRACK_FORMAT:
			sp->legacy_track[tracknum]->solos = number;
		break;

		case EOF_PRO_GUITAR_TRACK_FORMAT:
			sp->pro_guitar_track[tracknum]->solos = number;
		break;
	}
}

void eof_set_num_star_power_paths(EOF_SONG *sp, unsigned long track, unsigned long number)
{
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return;
	tracknum = sp->track[track]->tracknum;

	switch(sp->track[track]->track_format)
	{
		case EOF_LEGACY_TRACK_FORMAT:
			sp->legacy_track[tracknum]->star_power_paths = number;
		break;

		case EOF_VOCAL_TRACK_FORMAT:
			sp->vocal_track[tracknum]->star_power_paths = number;
		break;

		case EOF_PRO_GUITAR_TRACK_FORMAT:
			sp->pro_guitar_track[tracknum]->star_power_paths = number;
		break;
	}
}

long eof_track_fixup_next_note(EOF_SONG *sp, unsigned long track, unsigned long note)
{
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return -1;
	tracknum = sp->track[track]->tracknum;

	switch(sp->track[track]->track_format)
	{
		case EOF_LEGACY_TRACK_FORMAT:
		return eof_fixup_next_legacy_note(sp->legacy_track[tracknum], note);

		case EOF_VOCAL_TRACK_FORMAT:
		return eof_fixup_next_lyric(sp->vocal_track[tracknum], note);

		case EOF_PRO_GUITAR_TRACK_FORMAT:
		return eof_fixup_next_pro_guitar_note(sp->pro_guitar_track[tracknum], note);
	}

	return -1;
}

void eof_track_find_crazy_notes(EOF_SONG *sp, unsigned long track)
{
	unsigned long i;
	long next;

	if((sp == NULL) || (track >= sp->tracks))
		return;

	if(sp->track[track]->track_format == EOF_VOCAL_TRACK_FORMAT)
		return;	//Vocal tracks don't have the capability to have "crazy" notes

	for(i = 0; i < eof_get_track_size(sp, track); i++)
	{	//For each note in the track
		next = eof_track_fixup_next_note(sp, track, i);
		if(next >= 0)
		{
			if(eof_get_note_pos(sp, track, i) + eof_get_note_length(sp, track, i) > eof_get_note_pos(sp, track, next))
			{
				eof_set_note_flags(sp, track, i, eof_get_note_flags(sp, track, i) | EOF_NOTE_FLAG_CRAZY);
			}
		}
	}
}

void eof_set_note_flags(EOF_SONG *sp, unsigned long track, unsigned long note, unsigned long flags)
{
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return;
	tracknum = sp->track[track]->tracknum;

	switch(sp->track[track]->track_format)
	{
		case EOF_LEGACY_TRACK_FORMAT:
			if(note < sp->legacy_track[tracknum]->notes)
			{
				sp->legacy_track[tracknum]->note[note]->flags = flags;
			}
		break;

		case EOF_VOCAL_TRACK_FORMAT:
			if(note < sp->vocal_track[tracknum]->lyrics)
			{
				sp->vocal_track[tracknum]->lyric[note]->flags = flags;
			}
		break;

		case EOF_PRO_GUITAR_TRACK_FORMAT:
			if(note < sp->pro_guitar_track[tracknum]->notes)
			{
				sp->pro_guitar_track[tracknum]->note[note]->flags = flags;
			}
		break;
	}
}

void eof_set_note_type(EOF_SONG *sp, unsigned long track, unsigned long note, char type)
{
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return;
	tracknum = sp->track[track]->tracknum;

	switch(sp->track[track]->track_format)
	{
		case EOF_LEGACY_TRACK_FORMAT:
			if(note < sp->legacy_track[tracknum]->notes)
			{
				sp->legacy_track[tracknum]->note[note]->type = type;
			}
		break;

		case EOF_VOCAL_TRACK_FORMAT:
			if(note < sp->vocal_track[tracknum]->lyrics)
			{
				sp->vocal_track[tracknum]->lyric[note]->type = type;
			}
		break;

		case EOF_PRO_GUITAR_TRACK_FORMAT:
			if(note < sp->pro_guitar_track[tracknum]->notes)
			{
				sp->pro_guitar_track[tracknum]->note[note]->type = type;
			}
		break;
	}
}

void eof_track_delete_star_power_path(EOF_SONG *sp, unsigned long track, unsigned long pathnum)
{
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return;
	tracknum = sp->track[track]->tracknum;

	switch(sp->track[track]->track_format)
	{
		case EOF_LEGACY_TRACK_FORMAT:
			if(pathnum < sp->legacy_track[tracknum]->star_power_paths)
			{
				eof_legacy_track_delete_star_power(sp->legacy_track[tracknum], pathnum);
			}
		break;

		case EOF_VOCAL_TRACK_FORMAT:
			if(pathnum < sp->vocal_track[tracknum]->star_power_paths)
			{
				eof_vocal_track_delete_star_power(sp->vocal_track[tracknum], pathnum);
			}
		break;

		case EOF_PRO_GUITAR_TRACK_FORMAT:
			if(pathnum < sp->pro_guitar_track[tracknum]->star_power_paths)
			{
				eof_pro_guitar_track_delete_star_power(sp->pro_guitar_track[tracknum], pathnum);
			}
		break;
	}
}

void eof_vocal_track_delete_star_power(EOF_VOCAL_TRACK * tp, unsigned long index)
{
	unsigned long i;

	if(index >= tp->star_power_paths)
		return;

	if(tp->star_power_path[index].name != NULL)
	{	//If the section has a name
		free(tp->star_power_path[index].name);	//Free it
	}

	for(i = index; i < tp->star_power_paths - 1; i++)
	{
		memcpy(&tp->star_power_path[i], &tp->star_power_path[i + 1], sizeof(EOF_PHRASE_SECTION));
	}
	tp->star_power_paths--;
}

void eof_pro_guitar_track_delete_star_power(EOF_PRO_GUITAR_TRACK * tp, unsigned long index)
{
	unsigned long i;

	if(index >= tp->star_power_paths)
		return;

	if(tp->star_power_path[index].name != NULL)
	{	//If the section has a name
		free(tp->star_power_path[index].name);	//Free it
	}

	for(i = index; i < tp->star_power_paths - 1; i++)
	{
		memcpy(&tp->star_power_path[i], &tp->star_power_path[i + 1], sizeof(EOF_PHRASE_SECTION));
	}
	tp->star_power_paths--;
}

void eof_track_add_star_power_path(EOF_SONG *sp, unsigned long track, unsigned long start_pos, unsigned long end_pos)
{
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return;
	tracknum = sp->track[track]->tracknum;

	switch(sp->track[track]->track_format)
	{
		case EOF_LEGACY_TRACK_FORMAT:
			eof_legacy_track_add_star_power(sp->legacy_track[tracknum], start_pos, end_pos);
		break;

		case EOF_VOCAL_TRACK_FORMAT:
			eof_vocal_track_add_star_power(sp->vocal_track[tracknum], start_pos, end_pos);
		break;

		case EOF_PRO_GUITAR_TRACK_FORMAT:
			eof_pro_guitar_track_add_star_power(sp->pro_guitar_track[tracknum], start_pos, end_pos);
		break;
	}
}

void eof_vocal_track_add_star_power(EOF_VOCAL_TRACK * tp, unsigned long start_pos, unsigned long end_pos)
{
	if(tp->star_power_paths < EOF_MAX_PHRASES)
	{	//If the maximum number of star power phrases for this track hasn't already been defined
		tp->star_power_path[tp->star_power_paths].start_pos = start_pos;
		tp->star_power_path[tp->star_power_paths].end_pos = end_pos;
		tp->star_power_paths++;
	}
}

void eof_pro_guitar_track_add_star_power(EOF_PRO_GUITAR_TRACK * tp, unsigned long start_pos, unsigned long end_pos)
{
	if(tp->star_power_paths < EOF_MAX_PHRASES)
	{	//If the maximum number of star power phrases for this track hasn't already been defined
		tp->star_power_path[tp->star_power_paths].start_pos = start_pos;
		tp->star_power_path[tp->star_power_paths].end_pos = end_pos;
		tp->star_power_paths++;
	}
}

void eof_track_delete_solo(EOF_SONG *sp, unsigned long track, unsigned long pathnum)
{
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return;
	tracknum = sp->track[track]->tracknum;

	switch(sp->track[track]->track_format)
	{
		case EOF_LEGACY_TRACK_FORMAT:
			eof_legacy_track_delete_solo(sp->legacy_track[tracknum], pathnum);
		break;

		case EOF_PRO_GUITAR_TRACK_FORMAT:
			eof_pro_guitar_track_delete_solo(sp->pro_guitar_track[tracknum], pathnum);
		break;
	}
}

void eof_pro_guitar_track_delete_solo(EOF_PRO_GUITAR_TRACK * tp, unsigned long index)
{
	unsigned long i;

	if(index >= tp->solos)
		return;

	if(tp->solo[index].name != NULL)
	{	//If the section has a name
		free(tp->solo[index].name);	//Free it
	}

	for(i = index; i < tp->solos - 1; i++)
	{
		memcpy(&tp->solo[i], &tp->solo[i + 1], sizeof(EOF_PHRASE_SECTION));
	}
	tp->solos--;
}

void eof_track_add_solo(EOF_SONG *sp, unsigned long track, unsigned long start_pos, unsigned long end_pos)
{
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return;
	tracknum = sp->track[track]->tracknum;

	switch(sp->track[track]->track_format)
	{
		case EOF_LEGACY_TRACK_FORMAT:
			eof_legacy_track_add_solo(sp->legacy_track[tracknum], start_pos, end_pos);
		break;

		case EOF_PRO_GUITAR_TRACK_FORMAT:
			eof_pro_guitar_track_add_solo(sp->pro_guitar_track[tracknum], start_pos, end_pos);
		break;
	}
}

void eof_pro_guitar_track_add_solo(EOF_PRO_GUITAR_TRACK * tp, unsigned long start_pos, unsigned long end_pos)
{
	if(tp->solos < EOF_MAX_PHRASES)
	{	//If the maximum number of solo phrases for this track hasn't already been defined
		tp->solo[tp->solos].start_pos = start_pos;
		tp->solo[tp->solos].end_pos = end_pos;
		tp->solos++;
	}
}

void eof_note_set_tail_pos(EOF_SONG *sp, unsigned long track, unsigned long note, unsigned long pos)
{
	if((sp == NULL) || (track >= sp->tracks) || (note >= eof_get_track_size(sp, track)))
		return;

	eof_set_note_length(sp, track, note, pos - eof_get_note_pos(sp, track, note));
}

unsigned long eof_get_used_lanes(unsigned long track, unsigned long startpos, unsigned long endpos, char type)
{
	unsigned long ctr, bitmask = 0;

	for(ctr = 0; ctr < eof_get_track_size(eof_song, track); ctr++)
	{	//For each note in the specified track
		if((eof_get_note_type(eof_song, track, ctr) == type) && (eof_get_note_pos(eof_song, track, ctr) >= startpos) && (eof_get_note_pos(eof_song, track, ctr) <= endpos))
		{	//If the note is in the specified difficulty and is within the specified time range
			bitmask |= eof_get_note_note(eof_song, track, ctr);	//Add its bitmask to the result
		}
	}

	return bitmask;
}

unsigned long eof_get_num_trills(EOF_SONG *sp, unsigned long track)
{
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return 0;	//Return error
	tracknum = sp->track[track]->tracknum;

	switch(sp->track[track]->track_format)
	{
		case EOF_LEGACY_TRACK_FORMAT:
		return sp->legacy_track[tracknum]->trills;

		case EOF_PRO_GUITAR_TRACK_FORMAT:
		return sp->pro_guitar_track[tracknum]->trills;
	}

	return 0;	//Return error
}

unsigned long eof_get_num_tremolos(EOF_SONG *sp, unsigned long track)
{
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return 0;	//Return error
	tracknum = sp->track[track]->tracknum;

	switch(sp->track[track]->track_format)
	{
		case EOF_LEGACY_TRACK_FORMAT:
		return sp->legacy_track[tracknum]->tremolos;

		case EOF_PRO_GUITAR_TRACK_FORMAT:
		return sp->pro_guitar_track[tracknum]->tremolos;
	}

	return 0;	//Return error
}

EOF_PHRASE_SECTION *eof_get_trill(EOF_SONG *sp, unsigned long track, unsigned long index)
{
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return NULL;	//Return error
	tracknum = sp->track[track]->tracknum;

	switch(sp->track[track]->track_format)
	{
		case EOF_LEGACY_TRACK_FORMAT:
			if(index < EOF_MAX_PHRASES)
			{
				return &sp->legacy_track[tracknum]->trill[index];
			}
		break;


		case EOF_PRO_GUITAR_TRACK_FORMAT:
			if(index < EOF_MAX_PHRASES)
			{
				return &sp->pro_guitar_track[tracknum]->trill[index];
			}
		break;
	}

	return NULL;	//Return error
}

EOF_PHRASE_SECTION *eof_get_tremolo(EOF_SONG *sp, unsigned long track, unsigned long index)
{
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return NULL;	//Return error
	tracknum = sp->track[track]->tracknum;

	switch(sp->track[track]->track_format)
	{
		case EOF_LEGACY_TRACK_FORMAT:
			if(index < EOF_MAX_PHRASES)
			{
				return &sp->legacy_track[tracknum]->tremolo[index];
			}
		break;


		case EOF_PRO_GUITAR_TRACK_FORMAT:
			if(index < EOF_MAX_PHRASES)
			{
				return &sp->pro_guitar_track[tracknum]->tremolo[index];
			}
		break;
	}

	return NULL;	//Return error
}

void eof_track_delete_trill(EOF_SONG *sp, unsigned long track, unsigned long index)
{
	unsigned long ctr;
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return;
	tracknum = sp->track[track]->tracknum;

	switch(sp->track[track]->track_format)
	{
		case EOF_LEGACY_TRACK_FORMAT:
			if(index < sp->legacy_track[tracknum]->trills)
			{
				if(sp->legacy_track[tracknum]->trill[index].name != NULL)
				{	//If the section has a name
					free(sp->legacy_track[tracknum]->trill[index].name);	//Free it
				}
				for(ctr = index; ctr < sp->legacy_track[tracknum]->trills; ctr++)
				{
					memcpy(&sp->legacy_track[tracknum]->trill[ctr], &sp->legacy_track[tracknum]->trill[ctr + 1], sizeof(EOF_PHRASE_SECTION));
				}
				sp->legacy_track[tracknum]->trills--;
			}
		break;

		case EOF_PRO_GUITAR_TRACK_FORMAT:
			if(index < sp->pro_guitar_track[tracknum]->trills)
			{
				if(sp->pro_guitar_track[tracknum]->trill[index].name != NULL)
				{	//If the section has a name
					free(sp->pro_guitar_track[tracknum]->trill[index].name);	//Free it
				}
				for(ctr = index; ctr < sp->pro_guitar_track[tracknum]->trills; ctr++)
				{
					memcpy(&sp->pro_guitar_track[tracknum]->trill[ctr], &sp->pro_guitar_track[tracknum]->trill[ctr + 1], sizeof(EOF_PHRASE_SECTION));
				}
				sp->pro_guitar_track[tracknum]->trills--;
			}
		break;
	}
}

void eof_track_delete_tremolo(EOF_SONG *sp, unsigned long track, unsigned long index)
{
	unsigned long ctr;
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return;
	tracknum = sp->track[track]->tracknum;

	switch(sp->track[track]->track_format)
	{
		case EOF_LEGACY_TRACK_FORMAT:
			if(index < sp->legacy_track[tracknum]->tremolos)
			{
				if(sp->legacy_track[tracknum]->tremolo[index].name != NULL)
				{	//If the section has a name
					free(sp->legacy_track[tracknum]->tremolo[index].name);	//Free it
				}
				for(ctr = index; ctr < sp->legacy_track[tracknum]->tremolos; ctr++)
				{
					memcpy(&sp->legacy_track[tracknum]->tremolo[ctr], &sp->legacy_track[tracknum]->tremolo[ctr + 1], sizeof(EOF_PHRASE_SECTION));
				}
				sp->legacy_track[tracknum]->tremolos--;
			}
		break;

		case EOF_PRO_GUITAR_TRACK_FORMAT:
			if(index < sp->pro_guitar_track[tracknum]->tremolos)
			{
				if(sp->pro_guitar_track[tracknum]->tremolo[index].name != NULL)
				{	//If the section has a name
					free(sp->pro_guitar_track[tracknum]->tremolo[index].name);	//Free it
				}
				for(ctr = index; ctr < sp->pro_guitar_track[tracknum]->tremolos; ctr++)
				{
					memcpy(&sp->pro_guitar_track[tracknum]->tremolo[ctr], &sp->pro_guitar_track[tracknum]->tremolo[ctr + 1], sizeof(EOF_PHRASE_SECTION));
				}
				sp->pro_guitar_track[tracknum]->tremolos--;
			}
		break;
	}
}

void eof_set_num_trills(EOF_SONG *sp, unsigned long track, unsigned long number)
{
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return;
	tracknum = sp->track[track]->tracknum;

	switch(sp->track[track]->track_format)
	{
		case EOF_LEGACY_TRACK_FORMAT:
			sp->legacy_track[tracknum]->trills = number;
		break;

		case EOF_PRO_GUITAR_TRACK_FORMAT:
			sp->pro_guitar_track[tracknum]->trills = number;
		break;
	}
}

void eof_set_num_tremolos(EOF_SONG *sp, unsigned long track, unsigned long number)
{
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return;
	tracknum = sp->track[track]->tracknum;

	switch(sp->track[track]->track_format)
	{
		case EOF_LEGACY_TRACK_FORMAT:
			sp->legacy_track[tracknum]->tremolos = number;
		break;

		case EOF_PRO_GUITAR_TRACK_FORMAT:
			sp->pro_guitar_track[tracknum]->tremolos = number;
		break;
	}
}

void eof_set_pro_guitar_fret_number(char function, unsigned long fretvalue)
{
	unsigned long ctr, ctr2, bitmask, tracknum;
	char undo_made = 0;
	unsigned char oldfretvalue = 0, newfretvalue = 0;

	if(eof_song->track[eof_selected_track]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT)
		return;	//Do not allow this function to run unless a pro guitar/bass track is active

	tracknum = eof_song->track[eof_selected_track]->tracknum;
	for(ctr = 0; ctr < eof_song->pro_guitar_track[tracknum]->notes; ctr++)
	{	//For each note in the active pro guitar track
		if((eof_selection.track == eof_selected_track) && eof_selection.multi[ctr] && (eof_song->pro_guitar_track[tracknum]->note[ctr]->type == eof_note_type))
		{	//If the note is selected and is in the active difficulty
			for(ctr2 = 0, bitmask = 1; ctr2 < 6; ctr2++, bitmask<<=1)
			{	//For each of the 6 usable strings
				if((eof_song->pro_guitar_track[tracknum]->note[ctr]->note & bitmask) && (eof_pro_guitar_fret_bitmask & bitmask))
				{	//If this string is in use, and this string is enabled for fret shortcut manipulation
					oldfretvalue = eof_song->pro_guitar_track[tracknum]->note[ctr]->frets[ctr2];
					newfretvalue = oldfretvalue;
					switch(function)
					{
						case 0:	//Set fret value
							newfretvalue = fretvalue;
						break;

						case 1:	//Increment fret value
							if(oldfretvalue != 0xFF)	//Don't increment a muted note
								newfretvalue++;
						break;

						case 2:	//Decrement fret value
							if(oldfretvalue > 0)	//Don't decrement an open note
								newfretvalue--;
						break;
					}
					if((newfretvalue <= eof_song->pro_guitar_track[tracknum]->numfrets) || (newfretvalue == 0xFF))
					{	//Only set the fret value if it is valid
						if(!undo_made && (newfretvalue != oldfretvalue))
						{	//Make an undo state before making the first change
							eof_prepare_undo(EOF_UNDO_TYPE_NONE);
							undo_made = 1;
						}
						eof_song->pro_guitar_track[tracknum]->note[ctr]->frets[ctr2] = newfretvalue;	//Update the string's fret value
					}
				}
			}
		}
	}
}

char *eof_get_note_name(EOF_SONG *sp, unsigned long track, unsigned long note)
{
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return NULL;	//Return error
	tracknum = sp->track[track]->tracknum;

	switch(sp->track[track]->track_format)
	{
		case EOF_LEGACY_TRACK_FORMAT:
			if(note < sp->legacy_track[tracknum]->notes)
			{
				return sp->legacy_track[tracknum]->note[note]->name;
			}
		break;

		case EOF_VOCAL_TRACK_FORMAT:
			if(note < sp->vocal_track[tracknum]->lyrics)
			{
				return sp->vocal_track[tracknum]->lyric[note]->text;
			}
		break;

		case EOF_PRO_GUITAR_TRACK_FORMAT:
			if(note < sp->pro_guitar_track[tracknum]->notes)
			{
				return sp->pro_guitar_track[tracknum]->note[note]->name;
			}
		break;
	}

	return NULL;	//Return error
}

void *eof_copy_note(EOF_SONG *sp, unsigned long sourcetrack, unsigned long sourcenote, unsigned long desttrack, unsigned long pos, long length, char type)
{
	unsigned long sourcetracknum, desttracknum, newnotenum;
	unsigned long note, flags;
	char *text;
	void *result = NULL;

	//Validate parameters
	if((sp == NULL) || (sourcetrack >= sp->tracks) || (desttrack >= sp->tracks) || (sourcenote >= eof_get_track_size(sp, sourcetrack)))
		return NULL;

	//Don't allow copying instrument track notes to PART VOCALS and vice versa
	if(((sourcetrack == EOF_TRACK_VOCALS) && (desttrack != EOF_TRACK_VOCALS)) || ((sourcetrack != EOF_TRACK_VOCALS) && (desttrack == EOF_TRACK_VOCALS)))
		return NULL;

	sourcetracknum = sp->track[sourcetrack]->tracknum;
	desttracknum = sp->track[desttrack]->tracknum;

	note = eof_get_note_note(sp, sourcetrack, sourcenote);
	text = eof_get_note_name(sp, sourcetrack, sourcenote);
	flags = eof_get_note_flags(sp, sourcetrack, sourcenote);

	if(desttrack == EOF_TRACK_VOCALS)
	{	//If copying from PART VOCALS
		return eof_track_add_create_note(sp, desttrack, note, pos, length, type, text);
	}
	else
	{	//If copying from a non vocal track
		if((sp->track[sourcetrack]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT) && (sp->track[desttrack]->track_format != EOF_PRO_GUITAR_TRACK_FORMAT))
		{	//If copying from a pro guitar track to a non pro guitar track
			if(eof_song->pro_guitar_track[sourcetracknum]->note[sourcenote]->legacymask != 0)
			{	//If the user defined how this pro guitar note would transcribe to a legacy track
				note = eof_song->pro_guitar_track[sourcetracknum]->note[sourcenote]->legacymask;	//Use that bitmask
			}
		}

		result = eof_track_add_create_note(sp, desttrack, note, pos, length, type, text);
		if(result)
		{	//If the note was successfully created
			newnotenum = eof_get_track_size(sp, desttrack) - 1;		//The index of the new note
			eof_set_note_flags(sp, desttrack, newnotenum, flags);	//Copy the souce note's flags to the newly created note
			if((sp->track[sourcetrack]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT) && (sp->track[desttrack]->track_format == EOF_PRO_GUITAR_TRACK_FORMAT))
			{	//If the note was copied from a pro guitar track and pasted to a pro guitar track
				memcpy(sp->pro_guitar_track[desttracknum]->note[newnotenum]->frets, eof_song->pro_guitar_track[sourcetracknum]->note[sourcenote]->frets, 6);	//Copy the six usable string fret values from the source note to the newly created note
			}
		}
	}//If copying from a non vocal track

	return result;
}

unsigned long eof_get_num_arpeggios(EOF_SONG *sp, unsigned long track)
{
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return 0;	//Return error
	tracknum = sp->track[track]->tracknum;

	switch(sp->track[track]->track_format)
	{
		case EOF_PRO_GUITAR_TRACK_FORMAT:
		return sp->pro_guitar_track[tracknum]->arpeggios;
	}

	return 0;	//Return error
}

EOF_PHRASE_SECTION *eof_get_arpeggio(EOF_SONG *sp, unsigned long track, unsigned long index)
{
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return NULL;	//Return error
	tracknum = sp->track[track]->tracknum;

	switch(sp->track[track]->track_format)
	{
		case EOF_PRO_GUITAR_TRACK_FORMAT:
			if(index < EOF_MAX_PHRASES)
			{
				return &sp->pro_guitar_track[tracknum]->arpeggio[index];
			}
		break;
	}

	return NULL;	//Return error
}

//#define EOF_CREATE_IMAGE_SEQUENCE_SHOW_FPS_ONLY
int eof_create_image_sequence(void)
{
	unsigned long framectr = 0, refreshctr = 0, lastpollctr = 0;
	unsigned long remainder = 0;
	char windowtitle[101] = {0};
	float fps = 0.0;
	clock_t starttime, curtime, endtime, lastpolltime = 0;
	char original_eof_desktop = eof_desktop;

	#ifndef EOF_CREATE_IMAGE_SEQUENCE_SHOW_FPS_ONLY
	int err;
	char filename[20] = {0};

	/* check to make sure \sequence folder exists */
	ustrcpy(eof_temp_filename, eof_song_path);
	replace_filename(eof_temp_filename, eof_temp_filename, "", sizeof(eof_temp_filename));
	put_backslash(eof_temp_filename);
	ustrcat(eof_temp_filename, "sequence");
	if(!file_exists(eof_temp_filename, FA_DIREC | FA_HIDDEN, NULL))
	{	//If this folder doesn't already exist
		err = eof_mkdir(eof_temp_filename);
		if(err)
		{	//If the folder could not be created
			allegro_message("Could not create folder!\n%s", eof_temp_filename);
			return 1;
		}
	}
	else if(alert(NULL, "Overwrite contents of existing \\sequence folder?", NULL, "&Yes", "&No", 'y', 'n') != 1)
	{	//If user declined to overwrite the contents of the folder
		return 1;
	}
	put_backslash(eof_temp_filename);	//eof_temp_filename is now the path of the \sequence folder
	#endif

//Change to 8 bit color mode
	eof_desktop = 0;
	eof_apply_display_settings(eof_screen_layout.mode);

	alogg_seek_abs_msecs_ogg(eof_music_track, 0);
	eof_music_actual_pos = alogg_get_pos_msecs_ogg(eof_music_track);
	eof_music_pos = eof_music_actual_pos + eof_av_delay;
	clear_to_color(eof_screen, makecol(224, 224, 224));
	blit(eof_image[EOF_IMAGE_MENU_FULL], eof_screen, 0, 0, 0, 0, eof_screen->w, eof_screen->h);
	starttime = clock();	//Get the start time of the image sequence export
	while(eof_music_pos <  eof_music_actual_length)
	{
		if(key[KEY_ESC])
			break;

		if(refreshctr >= 10)
		{
		//Update EOF's window title to provide a status
			curtime = clock();	//Get the current time
			fps = (float)(framectr - lastpollctr) / ((float)(curtime - lastpolltime) / (float)CLOCKS_PER_SEC);	//Find the number of FPS rendered since the last poll
			snprintf(windowtitle, sizeof(windowtitle)-1, "Exporting image sequence: %.2f%% (%.2fFPS) - Press Esc to cancel",(float)eof_music_pos/(float)eof_music_actual_length * 100.0, fps);
			set_window_title(windowtitle);
			refreshctr -= 10;
			lastpolltime = curtime;
			lastpollctr = framectr;
		}

		//Render the screen
		eof_find_lyric_preview_lines();
		if(eof_vocals_selected)
		{
			eof_render_vocal_editor_window();
			eof_render_lyric_window();
		}
		else
		{
			eof_render_editor_window();
			eof_render_3d_window();
		}
		eof_render_note_window();

		#ifndef EOF_CREATE_IMAGE_SEQUENCE_SHOW_FPS_ONLY
	//Export the image for this frame
		snprintf(filename, sizeof(filename), "%08lu.pcx",framectr);
		replace_filename(eof_temp_filename, eof_temp_filename, filename, sizeof(eof_temp_filename));
		save_pcx(eof_temp_filename, eof_screen, NULL);	//Pass a NULL palette
		#endif

	//Seek one frame (1/30 second) further into the audio, tracking for rounding errors
		#define EOF_IMAGE_SEQUENCE_FPS 30
		framectr++;
		refreshctr++;
		eof_music_pos += 1000 / EOF_IMAGE_SEQUENCE_FPS;
		remainder += 1000 % EOF_IMAGE_SEQUENCE_FPS;	//Track the remainder
		while(remainder >= EOF_IMAGE_SEQUENCE_FPS)
		{
			eof_music_pos++;
			remainder -= EOF_IMAGE_SEQUENCE_FPS;
		}
	}
	endtime = clock();	//Get the start time of the image sequence export

	#ifdef EOF_CREATE_IMAGE_SEQUENCE_SHOW_FPS_ONLY
	fps = (float)framectr / ((float)(endtime - starttime) / (float)CLOCKS_PER_SEC);	//Find the average FPS
	snprintf(windowtitle, sizeof(windowtitle)-1, "Average render rate was %.2fFPS",fps);
	allegro_message("%s", windowtitle);
	#endif

//Restore original display settings
	eof_desktop = original_eof_desktop;
	eof_apply_display_settings(eof_screen_layout.mode);

	eof_fix_window_title();
	return 1;
}

unsigned long eof_get_num_lyric_sections(EOF_SONG *sp, unsigned long track)
{
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return 0;	//Return error
	tracknum = sp->track[track]->tracknum;

	switch(sp->track[track]->track_format)
	{
		case EOF_VOCAL_TRACK_FORMAT:
		return sp->vocal_track[tracknum]->lines;
	}

	return 0;	//Return error
}

EOF_PHRASE_SECTION *eof_get_lyric_section(EOF_SONG *sp, unsigned long track, unsigned long sectionnum)
{
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return NULL;	//Return error
	tracknum = sp->track[track]->tracknum;

	switch(sp->track[track]->track_format)
	{
		case EOF_VOCAL_TRACK_FORMAT:
			if(sectionnum < EOF_MAX_LYRIC_LINES)
			{
				return &sp->vocal_track[tracknum]->line[sectionnum];
			}
		break;
	}

	return NULL;	//Return error
}

void eof_adjust_note_length(EOF_SONG * sp, unsigned long track, unsigned long amount, int dir)
{
	unsigned long i, undo_made = 0, adjustment, notepos, notelength;
	long b, next_note;

	if((sp == NULL) || (track >= sp->tracks))
		return;

	adjustment = amount;	//This would be the amount to adjust the note by
	for(i = 0; i < eof_get_track_size(eof_song, eof_selected_track); i++)
	{	//For each note in the track
		notepos = eof_get_note_pos(sp, track, i);
		notelength = eof_get_note_length(eof_song, track, i);

		if((eof_selection.track == eof_selected_track) && eof_selection.multi[i] && (eof_get_note_type(eof_song, eof_selected_track, i) == eof_note_type))
		{	//If the note is selected and in the active instrument difficulty
			if(amount == 0)
			{	//If adjusting the note's length by the grid snap value, find the grid snap length for the note
				b = eof_get_beat(eof_song, notepos + notelength - 1);
				if(b >= 0)
				{
					eof_snap_logic(&eof_tail_snap, eof_song->beat[b]->pos);
				}
				else
				{
					eof_snap_logic(&eof_tail_snap, notepos + notelength - 1);
				}
				eof_snap_length_logic(&eof_tail_snap);
				adjustment = eof_tail_snap.length;
			}
			if(dir < 0)
			{	//If the note length is to be decreased
				if(notelength < 2)
				{	//If the note cannot have its length decreased
					continue;	//Skip adjusting this note
				}
				if(!undo_made)
				{	//Ensure an undo state was made before decreasing the length
					eof_prepare_undo(EOF_UNDO_TYPE_NOTE_LENGTH);
					undo_made = 1;
				}
				eof_set_note_length(eof_song, eof_selected_track, i, notelength - adjustment);
			}
			else
			{	//If the note length is to be increased
				next_note = eof_track_fixup_next_note(sp, track, i);	//Get the index of the next note in the active instrument difficulty
				if(next_note > 0)
				{	//Check if the increase would be canceled due to being unable to overlap the next note
					if((notepos + notelength + 1 >= eof_get_note_pos(sp, track, next_note)) && !(eof_get_note_flags(sp, track, i) & EOF_NOTE_FLAG_CRAZY))
					{	//If this note cannot increase its length because it would overlap the next and the note isn't "crazy"
						continue;	//Skip adjusting this note
					}
				}
				if(!undo_made)
				{	//Ensure an undo state was made before increasing the length
					eof_prepare_undo(EOF_UNDO_TYPE_NOTE_LENGTH);
					undo_made = 1;
				}
				eof_set_note_length(eof_song, eof_selected_track, i, notelength + adjustment);
			}
			if(amount == 0)
			{	//If adjusting the note's length by the grid snap value, snap the tail's end position
				notelength = eof_get_note_length(eof_song, eof_selected_track, i);
				if(notelength > 1)
				{	//If the note's length, after the adjustment, is over 1
					eof_snap_logic(&eof_tail_snap, notepos + notelength);
					eof_note_set_tail_pos(eof_song, eof_selected_track, i, eof_tail_snap.pos);
				}
			}
		}
	}//For each note in the track
	eof_track_fixup_notes(eof_song, eof_selected_track, 1);
}

void eof_set_num_arpeggios(EOF_SONG *sp, unsigned long track, unsigned long number)
{
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks))
		return;
	tracknum = sp->track[track]->tracknum;

	switch(sp->track[track]->track_format)
	{
		case EOF_PRO_GUITAR_TRACK_FORMAT:
			sp->pro_guitar_track[tracknum]->arpeggios = number;
		break;
	}
}

void eof_set_note_name(EOF_SONG *sp, unsigned long track, unsigned long note, char *name)
{
	unsigned long tracknum;

	if((sp == NULL) || (track >= sp->tracks) || (name == NULL))
		return;
	tracknum = sp->track[track]->tracknum;

	switch(sp->track[track]->track_format)
	{
		case EOF_LEGACY_TRACK_FORMAT:
			if(note < sp->legacy_track[tracknum]->notes)
			{
				ustrncpy(sp->legacy_track[tracknum]->note[note]->name, name, EOF_NAME_LENGTH);
			}
		break;

		case EOF_VOCAL_TRACK_FORMAT:
			if(note < sp->vocal_track[tracknum]->lyrics)
			{
				ustrncpy(sp->vocal_track[tracknum]->lyric[note]->text, name, EOF_MAX_LYRIC_LENGTH);
			}
		break;

		case EOF_PRO_GUITAR_TRACK_FORMAT:
			if(note < sp->pro_guitar_track[tracknum]->notes)
			{
				ustrncpy(sp->pro_guitar_track[tracknum]->note[note]->name, name, EOF_NAME_LENGTH);
			}
		break;
	}
}
