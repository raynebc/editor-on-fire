#include <allegro.h>
#include "song.h"
#include "event.h"

EOF_SONG * eof_load_notes_legacy(PACKFILE * fp, char version)
{
	EOF_SONG * sp = NULL;
	EOF_NOTE * new_note = NULL;
	int i, j;
	char a;
	int b, c, t, tl;
	unsigned long lastppqn = 500000;
	char buffer[1024];

	unsigned long eof_old_max_notes=0;
	unsigned long eof_max_notes=0;
	unsigned eof_beat_flag_anchor=0;
	unsigned eof_tracks_max=0;
	unsigned eof_note_amazing=0;

	//All EOF project formats before revision 'H' stored 5 legacy and 1 vocal track
	sp = eof_create_song_populated();	//Create a new chart with 5 legacy tracks and 1 vocal track
	if(!sp)
	{
		return NULL;
	}
	switch(version)
	{
		case 'G':
		{
			eof_old_max_notes = 65536;	//This was the value of EOF_OLD_MAX_NOTES
			pack_fread(sp->tags->artist, 256, fp);
			pack_fread(sp->tags->title, 256, fp);
			sp->beat[0]->ppqn = 500000;
			sp->beats = 1;
			for(i = 0; i < 4; i++)
			{
				a = pack_getc(fp);
				if(a)
				{
					pack_fread(buffer, 256, fp);
					for(j = 0; j < eof_old_max_notes; j++)
					{
						a = pack_getc(fp);
						if(a)
						{
							new_note = eof_legacy_track_add_note(sp->legacy_track[i]);
							new_note->type = pack_getc(fp);
							new_note->note = pack_getc(fp);
							new_note->pos = pack_igetl(fp);
							new_note->length = pack_igetw(fp);
							new_note->flags = pack_getc(fp);
						}
					}
				}
			}
			return sp;
		}
		case '1':
		{
			eof_old_max_notes = 65536;	//This was the value of EOF_OLD_MAX_NOTES
			pack_fread(sp->tags->artist, 256, fp);
			pack_fread(sp->tags->title, 256, fp);
			pack_fread(sp->tags->frettist, 256, fp);
			sp->tags->ogg[0].midi_offset = pack_igetl(fp);
			eof_song_add_beat(sp);
			sp->beat[0]->ppqn = pack_igetl(fp);
			sp->beats = 1;
			for(i = 0; i < 4; i++)
			{
				a = pack_getc(fp);
				if(a)
				{
					pack_fread(buffer, 256, fp);
					for(j = 0; j < eof_old_max_notes; j++)
					{
						a = pack_getc(fp);
						if(a)
						{
							new_note = eof_legacy_track_add_note(sp->legacy_track[i]);
							new_note->type = pack_getc(fp);
							new_note->note = pack_getc(fp);
							new_note->pos = pack_igetl(fp);
							new_note->length = pack_igetw(fp);
							new_note->flags = pack_getc(fp);
						}
					}
				}
			}
			return sp;
		}
		case '2':
		{
			eof_old_max_notes = 65536;	//This was the value of EOF_OLD_MAX_NOTES
			eof_beat_flag_anchor = 1;	//This was the value of EOF_BEAT_FLAG_ANCHOR
			eof_tracks_max = 5;			//This was the value of EOF_TRACKS_MAX
			eof_max_notes = 32768;		//This was the value of EOF_MAX_NOTES
			pack_fread(sp->tags->artist, 256, fp);
			pack_fread(sp->tags->title, 256, fp);
			pack_fread(sp->tags->frettist, 256, fp);
			sp->tags->ogg[0].midi_offset = pack_igetl(fp);

			/* read beat info */
			b = pack_igetl(fp);
			eof_song_resize_beats(sp, b);
			for(i = 0; i < b; i++)
			{
				sp->beat[i]->ppqn = pack_igetl(fp);
				if(sp->beat[i]->ppqn != lastppqn)
				{
					sp->beat[i]->flags = eof_beat_flag_anchor;
				}
				else
				{
					sp->beat[i]->flags = 0;
				}
				lastppqn = sp->beat[i]->ppqn;
				sp->beat[i]->pos = pack_igetl(fp);
			}

			/* read tracks */
			for(i = 0; i < eof_tracks_max; i++)
			{
				a = pack_getc(fp);
				if(a)
				{
					pack_fread(buffer, 256, fp);
					for(j = 0; j < eof_old_max_notes; j++)
					{
						a = pack_getc(fp);
						if(j < eof_max_notes)
						{
							if(a)
							{
								new_note = eof_legacy_track_add_note(sp->legacy_track[i]);
								new_note->type = pack_getc(fp);
								new_note->note = pack_getc(fp);
								new_note->pos = pack_igetl(fp);
								new_note->length = pack_igetw(fp);
								new_note->flags = pack_getc(fp);
							}
						}
						else
						{
							if(a)
							{
								pack_getc(fp);
								pack_getc(fp);
								pack_igetl(fp);
								pack_igetw(fp);
								pack_getc(fp);
							}
						}
					}
				}
			}
			return sp;
		}
		case '3':
		{
			eof_tracks_max = 5;			//This was the value of EOF_TRACKS_MAX
			eof_old_max_notes = 65536;	//This was the value of EOF_OLD_MAX_NOTES
			pack_fread(sp->tags->artist, 256, fp);
			pack_fread(sp->tags->title, 256, fp);
			pack_fread(sp->tags->frettist, 256, fp);
			sp->tags->ogg[0].midi_offset = pack_igetl(fp);

			/* read beat info */
			b = pack_igetl(fp);
			eof_song_resize_beats(sp, b);
			for(i = 0; i < b; i++)
			{
				sp->beat[i]->ppqn = pack_igetl(fp);
				sp->beat[i]->pos = pack_igetl(fp);
				sp->beat[i]->flags = pack_igetl(fp);
			}

			/* read tracks */
			for(i = 0; i < eof_tracks_max; i++)
			{
				a = pack_getc(fp);
				if(a)
				{
					pack_fread(buffer, 256, fp);
					for(j = 0; j < eof_old_max_notes; j++)
					{
						a = pack_getc(fp);
						if(a)
						{
							new_note = eof_legacy_track_add_note(sp->legacy_track[i]);
							new_note->type = pack_getc(fp);
							new_note->note = pack_getc(fp);
							new_note->pos = pack_igetl(fp);
							new_note->length = pack_igetw(fp);
							new_note->flags = pack_getc(fp);
						}
					}
				}
			}
			return sp;
		}
		case '4':
		{
			eof_tracks_max = 5;			//This was the value of EOF_TRACKS_MAX
			eof_max_notes = 32768;		//This was the value of EOF_MAX_NOTES
			pack_fread(sp->tags->artist, 256, fp);
			pack_fread(sp->tags->title, 256, fp);
			pack_fread(sp->tags->frettist, 256, fp);
			sp->tags->ogg[0].midi_offset = pack_igetl(fp);

			/* read beat info */
			b = pack_igetl(fp);
			eof_song_resize_beats(sp, b);
			for(i = 0; i < b; i++)
			{
				sp->beat[i]->ppqn = pack_igetl(fp);
				sp->beat[i]->pos = pack_igetl(fp);
				sp->beat[i]->flags = pack_igetl(fp);
			}

			/* read tracks */
			for(i = 0; i < eof_tracks_max; i++)
			{
				a = pack_getc(fp);
				if(a)
				{
					pack_fread(buffer, 256, fp);
					for(j = 0; j < eof_max_notes; j++)
					{
						a = pack_getc(fp);
						if(a)
						{
							new_note = eof_legacy_track_add_note(sp->legacy_track[i]);
							new_note->type = pack_getc(fp);
							new_note->note = pack_getc(fp);
							new_note->pos = pack_igetl(fp);
							new_note->length = pack_igetw(fp);
							new_note->flags = pack_getc(fp);
						}
					}
				}
			}
			return sp;
		}
		case '5':
		{
			eof_tracks_max = 5;			//This was the value of EOF_TRACKS_MAX
			eof_max_notes = 32768;		//This was the value of EOF_MAX_NOTES
			pack_fread(sp->tags->artist, 256, fp);
			pack_fread(sp->tags->title, 256, fp);
			pack_fread(sp->tags->frettist, 256, fp);
			sp->tags->ogg[0].midi_offset = pack_igetl(fp);

			/* read beat info */
			b = pack_igetl(fp);
			eof_song_resize_beats(sp, b);
			for(i = 0; i < b; i++)
			{
				sp->beat[i]->ppqn = pack_igetl(fp);
				sp->beat[i]->pos = pack_igetl(fp);
				sp->beat[i]->flags = pack_igetl(fp);
			}

			/* read events info */
			t = pack_igetl(fp);
			for(i = 0; i < t; i++)
			{
				pack_fread(buffer, 256, fp);
				b = pack_igetl(fp);
				eof_add_text_event(sp, b, buffer);
			}

			/* read tracks */
			for(i = 0; i < eof_tracks_max; i++)
			{
				a = pack_getc(fp);
				if(a)
				{
					pack_fread(buffer, 256, fp);
					for(j = 0; j < eof_max_notes; j++)
					{
						a = pack_getc(fp);
						if(a)
						{
							new_note = eof_legacy_track_add_note(sp->legacy_track[i]);
							new_note->type = pack_getc(fp);
							new_note->note = pack_getc(fp);
							new_note->pos = pack_igetl(fp);
							new_note->length = pack_igetw(fp);
							new_note->flags = pack_getc(fp);
						}
					}
				}
			}
			return sp;
		}
		case '6':
		{
			eof_tracks_max = 5;			//This was the value of EOF_TRACKS_MAX
			eof_max_notes = 32768;		//This was the value of EOF_MAX_NOTES
			eof_note_amazing = 3;		//This was the value of EOF_NOTE_AMAZING
			pack_fread(sp->tags->artist, 256, fp);
			pack_fread(sp->tags->title, 256, fp);
			pack_fread(sp->tags->frettist, 256, fp);
			sp->tags->ogg[0].midi_offset = pack_igetl(fp);

			/* read beat info */
			b = pack_igetl(fp);
			eof_song_resize_beats(sp, b);
			for(i = 0; i < b; i++)
			{
				sp->beat[i]->ppqn = pack_igetl(fp);
				sp->beat[i]->pos = pack_igetl(fp);
				sp->beat[i]->flags = pack_igetl(fp);
//				allegro_message("beat[%d]\nppqn = %d\npos = %d", i, sp->beat[i]->ppqn, sp->beat[i]->pos);
			}

			/* read events info */
			t = pack_igetl(fp);
			for(i = 0; i < t; i++)
			{
				pack_fread(buffer, 256, fp);
				b = pack_igetl(fp);
				eof_add_text_event(sp, b, buffer);
			}

			/* read tracks */
			for(i = 0; i < eof_tracks_max; i++)
			{
				a = pack_getc(fp);
				if(a)
				{
					pack_fread(buffer, 256, fp);
					for(j = 0; j < eof_max_notes; j++)
					{
						a = pack_getc(fp);
						if(a)
						{
							new_note = eof_legacy_track_add_note(sp->legacy_track[i]);
							new_note->type = pack_getc(fp);
							new_note->note = pack_getc(fp);
							new_note->pos = pack_igetl(fp);
							new_note->length = pack_igetw(fp);
							new_note->flags = pack_getc(fp);
						}
					}
				}
			}

			/* read bookmarks */
			for(i = 0; i < 10; i++)
			{
				sp->bookmark_pos[i] = pack_igetl(fp);
			}

			/* read fret catalog */
			sp->catalog->entries = pack_igetl(fp);
			for(i = 0; i < sp->catalog->entries; i++)
			{
				sp->catalog->entry[i].track = 1;	//All EOF project formats before revision 'H' numbered tracks starting from 0 instead of 1, correct for this
				sp->catalog->entry[i].start_pos = pack_igetl(fp);
				sp->catalog->entry[i].end_pos = pack_igetl(fp);
				sp->catalog->entry[i].type = eof_note_amazing;
			}
			return sp;
		}
		case '7':
		{
			eof_tracks_max = 5;			//This was the value of EOF_TRACKS_MAX
			eof_max_notes = 32768;		//This was the value of EOF_MAX_NOTES
			eof_note_amazing = 3;		//This was the value of EOF_NOTE_AMAZING
			pack_fread(sp->tags->artist, 256, fp);
			pack_fread(sp->tags->title, 256, fp);
			pack_fread(sp->tags->frettist, 256, fp);
			sp->tags->ogg[0].midi_offset = pack_igetl(fp);
			sp->tags->ini_settings = pack_igetw(fp);
			for(i = 0; i < sp->tags->ini_settings; i++)
			{
				pack_fread(sp->tags->ini_setting[i], 512, fp);
			}

			/* read beat info */
			b = pack_igetl(fp);
			eof_song_resize_beats(sp, b);
			for(i = 0; i < b; i++)
			{
				sp->beat[i]->ppqn = pack_igetl(fp);
				sp->beat[i]->pos = pack_igetl(fp);
				sp->beat[i]->flags = pack_igetl(fp);
			}

			/* read events info */
			t = pack_igetl(fp);
			for(i = 0; i < t; i++)
			{
				pack_fread(buffer, 256, fp);
				b = pack_igetl(fp);
				eof_add_text_event(sp, b, buffer);
			}

			/* read tracks */
			for(i = 0; i < eof_tracks_max; i++)
			{
				a = pack_getc(fp);
				if(a)
				{
					pack_fread(buffer, 256, fp);
					for(j = 0; j < eof_max_notes; j++)
					{
						a = pack_getc(fp);
						if(a)
						{
							new_note = eof_legacy_track_add_note(sp->legacy_track[i]);
							new_note->type = pack_getc(fp);
							new_note->note = pack_getc(fp);
							new_note->pos = pack_igetl(fp);
							new_note->length = pack_igetw(fp);
							new_note->flags = pack_getc(fp);
						}
					}
				}
			}

			/* read bookmarks */
			for(i = 0; i < 10; i++)
			{
				sp->bookmark_pos[i] = pack_igetl(fp);
			}

			/* read fret catalog */
			sp->catalog->entries = pack_igetl(fp);
			for(i = 0; i < sp->catalog->entries; i++)
			{
				sp->catalog->entry[i].track = 1;	//All EOF project formats before revision 'H' numbered tracks starting from 0 instead of 1, correct for this
				sp->catalog->entry[i].start_pos = pack_igetl(fp);
				sp->catalog->entry[i].end_pos = pack_igetl(fp);
				sp->catalog->entry[i].type = eof_note_amazing;
			}
			return sp;
		}
		case '8':
		{
			eof_tracks_max = 5;			//This was the value of EOF_TRACKS_MAX
			eof_max_notes = 32768;		//This was the value of EOF_MAX_NOTES
			eof_note_amazing = 3;		//This was the value of EOF_NOTE_AMAZING
			pack_fread(sp->tags->artist, 256, fp);
			pack_fread(sp->tags->title, 256, fp);
			pack_fread(sp->tags->frettist, 256, fp);
			sp->tags->ogg[0].midi_offset = pack_igetl(fp);
			sp->tags->ini_settings = pack_igetw(fp);
			for(i = 0; i < sp->tags->ini_settings; i++)
			{
				pack_fread(sp->tags->ini_setting[i], 512, fp);
			}

			/* read beat info */
			b = pack_igetl(fp);
			eof_song_resize_beats(sp, b);
			for(i = 0; i < b; i++)
			{
				sp->beat[i]->ppqn = pack_igetl(fp);
				sp->beat[i]->pos = pack_igetl(fp);
				sp->beat[i]->flags = pack_igetl(fp);
			}

			/* read events info */
			t = pack_igetl(fp);
			for(i = 0; i < t; i++)
			{
				pack_fread(buffer, 256, fp);
				b = pack_igetl(fp);
				eof_add_text_event(sp, b, buffer);
			}

			/* read tracks */
			for(i = 0; i < eof_tracks_max; i++)
			{
				a = pack_getc(fp);
				if(a)
				{
					pack_fread(buffer, 256, fp);
					for(j = 0; j < eof_max_notes; j++)
					{
						a = pack_getc(fp);
						if(a)
						{
							new_note = eof_legacy_track_add_note(sp->legacy_track[i]);
							new_note->type = pack_getc(fp);
							new_note->note = pack_getc(fp);
							new_note->pos = pack_igetl(fp);
							new_note->length = pack_igetw(fp);
							new_note->flags = pack_getc(fp);
						}
					}
				}
			}

			/* read bookmarks */
			for(i = 0; i < 10; i++)
			{
				sp->bookmark_pos[i] = pack_igetl(fp);
			}

			/* read fret catalog */
			sp->catalog->entries = pack_igetl(fp);
			for(i = 0; i < sp->catalog->entries; i++)
			{
				sp->catalog->entry[i].track = pack_igetl(fp)+1;	//All EOF project formats before revision 'H' numbered tracks starting from 0 instead of 1, correct for this
				sp->catalog->entry[i].start_pos = pack_igetl(fp);
				sp->catalog->entry[i].end_pos = pack_igetl(fp);
				sp->catalog->entry[i].type = eof_note_amazing;
			}
			return sp;
		}
		case '9':
		{
			eof_tracks_max = 5;			//This was the value of EOF_TRACKS_MAX
			eof_note_amazing = 3;		//This was the value of EOF_NOTE_AMAZING
			pack_fread(sp->tags->artist, 256, fp);
			pack_fread(sp->tags->title, 256, fp);
			pack_fread(sp->tags->frettist, 256, fp);
			sp->tags->ogg[0].midi_offset = pack_igetl(fp);
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

			/* read tracks */
			for(i = 0; i < eof_tracks_max; i++)
			{
				b = pack_igetl(fp);
//				eof_legacy_track_resize(sp->legacy_track[i], b);
				eof_track_resize(sp, i + 1,b);
				for(j = 0; j < b; j++)
				{
					sp->legacy_track[i]->note[j]->type = pack_getc(fp);
					sp->legacy_track[i]->note[j]->note = pack_getc(fp);
					sp->legacy_track[i]->note[j]->pos = pack_igetl(fp);
					sp->legacy_track[i]->note[j]->length = pack_igetw(fp);
					sp->legacy_track[i]->note[j]->flags = pack_getc(fp);
				}
			}

			/* read bookmarks */
			for(i = 0; i < 10; i++)
			{
				sp->bookmark_pos[i] = pack_igetl(fp);
			}

			/* read fret catalog */
			sp->catalog->entries = pack_igetl(fp);
			for(i = 0; i < sp->catalog->entries; i++)
			{
				sp->catalog->entry[i].track = pack_igetl(fp)+1;	//All EOF project formats before revision 'H' numbered tracks starting from 0 instead of 1, correct for this
				sp->catalog->entry[i].start_pos = pack_igetl(fp);
				sp->catalog->entry[i].end_pos = pack_igetl(fp);
				sp->catalog->entry[i].type = eof_note_amazing;
			}
			return sp;
		}
		case 'A':
		{
			eof_tracks_max = 5;			//This was the value of EOF_TRACKS_MAX
			pack_fread(sp->tags->artist, 256, fp);
			pack_fread(sp->tags->title, 256, fp);
			pack_fread(sp->tags->frettist, 256, fp);
			sp->tags->ogg[0].midi_offset = pack_igetl(fp);
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

			/* read tracks */
			for(i = 0; i < eof_tracks_max; i++)
			{
				b = pack_igetl(fp);
//				eof_legacy_track_resize(sp->legacy_track[i], b);
				eof_track_resize(sp, i + 1,b);
				for(j = 0; j < b; j++)
				{
					sp->legacy_track[i]->note[j]->type = pack_getc(fp);
					sp->legacy_track[i]->note[j]->note = pack_getc(fp);
					sp->legacy_track[i]->note[j]->pos = pack_igetl(fp);
					sp->legacy_track[i]->note[j]->length = pack_igetw(fp);
					sp->legacy_track[i]->note[j]->flags = pack_getc(fp);
				}
			}

			/* read bookmarks */
			for(i = 0; i < 10; i++)
			{
				sp->bookmark_pos[i] = pack_igetl(fp);
			}

			/* read fret catalog */
			sp->catalog->entries = pack_igetl(fp);
			for(i = 0; i < sp->catalog->entries; i++)
			{
				sp->catalog->entry[i].track = pack_getc(fp)+1;	//All EOF project formats before revision 'H' numbered tracks starting from 0 instead of 1, correct for this
				sp->catalog->entry[i].type = pack_getc(fp);
				sp->catalog->entry[i].start_pos = pack_igetl(fp);
				sp->catalog->entry[i].end_pos = pack_igetl(fp);
			}
			return sp;
		}
		case 'B':
		{
			eof_tracks_max = 5;			//This was the value of EOF_TRACKS_MAX

			/* read song info */
			pack_fread(sp->tags->artist, 256, fp);
			pack_fread(sp->tags->title, 256, fp);
			pack_fread(sp->tags->frettist, 256, fp);

			/* read OGG data */
			sp->tags->oggs = pack_igetw(fp);
			for(i = 0; i < sp->tags->oggs; i++)
			{
				pack_fread(sp->tags->ogg[i].filename, 256, fp);
				sp->tags->ogg[i].midi_offset = pack_igetl(fp);
			}

			/* read file revision number */
			sp->tags->revision = pack_igetl(fp);

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

			/* read tracks */
			for(i = 0; i < eof_tracks_max; i++)
			{

				/* read solo sections */
				sp->legacy_track[i]->solos = pack_igetw(fp);
				for(j = 0; j < sp->legacy_track[i]->solos; j++)
				{
					sp->legacy_track[i]->solo[j].start_pos = pack_igetl(fp);
					sp->legacy_track[i]->solo[j].end_pos = pack_igetl(fp);
				}

				/* read star power sections */
				sp->legacy_track[i]->star_power_paths = pack_igetw(fp);
				for(j = 0; j < sp->legacy_track[i]->star_power_paths; j++)
				{
					sp->legacy_track[i]->star_power_path[j].start_pos = pack_igetl(fp);
					sp->legacy_track[i]->star_power_path[j].end_pos = pack_igetl(fp);
				}

				/* read notes */
				b = pack_igetl(fp);
//				eof_legacy_track_resize(sp->legacy_track[i], b);
				eof_track_resize(sp, i + 1,b);
				for(j = 0; j < b; j++)
				{
					sp->legacy_track[i]->note[j]->type = pack_getc(fp);
					sp->legacy_track[i]->note[j]->note = pack_getc(fp);
					sp->legacy_track[i]->note[j]->pos = pack_igetl(fp);
					sp->legacy_track[i]->note[j]->length = pack_igetw(fp);
					sp->legacy_track[i]->note[j]->flags = pack_getc(fp);
				}
			}

			/* read bookmarks */
			for(i = 0; i < 10; i++)
			{
				sp->bookmark_pos[i] = pack_igetl(fp);
			}

			/* read fret catalog */
			sp->catalog->entries = pack_igetl(fp);
			for(i = 0; i < sp->catalog->entries; i++)
			{
				sp->catalog->entry[i].track = pack_getc(fp)+1;	//All EOF project formats before revision 'H' numbered tracks starting from 0 instead of 1, correct for this
				sp->catalog->entry[i].type = pack_getc(fp);
				sp->catalog->entry[i].start_pos = pack_igetl(fp);
				sp->catalog->entry[i].end_pos = pack_igetl(fp);
			}
			return sp;
		}
		case 'C':
		{
			eof_tracks_max = 5;			//This was the value of EOF_TRACKS_MAX

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

			/* read tracks */
			for(i = 0; i < eof_tracks_max; i++)
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
//				eof_legacy_track_resize(sp->legacy_track[i], b);
				eof_track_resize(sp, i + 1,b);
				for(j = 0; j < b; j++)
				{
					sp->legacy_track[i]->note[j]->type = pack_getc(fp);
					sp->legacy_track[i]->note[j]->note = pack_getc(fp);
					sp->legacy_track[i]->note[j]->pos = pack_igetl(fp);
					sp->legacy_track[i]->note[j]->length = pack_igetw(fp);
					sp->legacy_track[i]->note[j]->flags = pack_getc(fp);
				}
			}

			/* read bookmarks */
			for(i = 0; i < 10; i++)
			{
				sp->bookmark_pos[i] = pack_igetl(fp);
			}

			/* read fret catalog */
			sp->catalog->entries = pack_igetl(fp);
			for(i = 0; i < sp->catalog->entries; i++)
			{
				sp->catalog->entry[i].track = pack_getc(fp)+1;	//All EOF project formats before revision 'H' numbered tracks starting from 0 instead of 1, correct for this
				sp->catalog->entry[i].type = pack_getc(fp);
				sp->catalog->entry[i].start_pos = pack_igetl(fp);
				sp->catalog->entry[i].end_pos = pack_igetl(fp);
			}
			return sp;
		}
		case 'D':
		{
			eof_tracks_max = 5;			//This was the value of EOF_TRACKS_MAX

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

			/* read tracks */
			for(i = 0; i < eof_tracks_max; i++)
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
//				eof_legacy_track_resize(sp->legacy_track[i], b);
				eof_track_resize(sp, i + 1,b);
				for(j = 0; j < b; j++)
				{
					sp->legacy_track[i]->note[j]->type = pack_getc(fp);
					sp->legacy_track[i]->note[j]->note = pack_getc(fp);
					sp->legacy_track[i]->note[j]->pos = pack_igetl(fp);
					sp->legacy_track[i]->note[j]->length = pack_igetw(fp);
					sp->legacy_track[i]->note[j]->flags = pack_getc(fp);
				}
			}

			/* read lyric track */
			b = pack_igetl(fp);
//			eof_vocal_track_resize(sp->vocal_track[0], b);
			eof_track_resize(sp, EOF_TRACK_VOCALS,b);
			for(j = 0; j < b; j++)
			{
				sp->vocal_track[0]->lyric[j]->pos = pack_igetl(fp);
				c = pack_igetw(fp);
				pack_fread(sp->vocal_track[0]->lyric[j]->text, c, fp);
				sp->vocal_track[0]->lyric[j]->text[c] = '\0';
			}
			sp->vocal_track[0]->lines = pack_igetl(fp);
			for(j = 0; j < sp->vocal_track[0]->lines; j++)
			{
				sp->vocal_track[0]->line[j].start_pos = pack_igetl(fp);
				sp->vocal_track[0]->line[j].end_pos = pack_igetl(fp);
				sp->vocal_track[0]->line[j].flags = 0;
			}

			/* read bookmarks */
			for(i = 0; i < 10; i++)
			{
				sp->bookmark_pos[i] = pack_igetl(fp);
			}

			/* read fret catalog */
			sp->catalog->entries = pack_igetl(fp);
			for(i = 0; i < sp->catalog->entries; i++)
			{
				sp->catalog->entry[i].track = pack_getc(fp)+1;	//All EOF project formats before revision 'H' numbered tracks starting from 0 instead of 1, correct for this
				sp->catalog->entry[i].type = pack_getc(fp);
				sp->catalog->entry[i].start_pos = pack_igetl(fp);
				sp->catalog->entry[i].end_pos = pack_igetl(fp);
			}
			return sp;
		}
		case 'E':
		{
			eof_tracks_max = 5;			//This was the value of EOF_TRACKS_MAX

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

			/* read tracks */
			for(i = 0; i < eof_tracks_max; i++)
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
//				eof_legacy_track_resize(sp->legacy_track[i], b);
				eof_track_resize(sp, i + 1,b);
				for(j = 0; j < b; j++)
				{
					sp->legacy_track[i]->note[j]->type = pack_getc(fp);
					sp->legacy_track[i]->note[j]->note = pack_getc(fp);
					sp->legacy_track[i]->note[j]->pos = pack_igetl(fp);
					sp->legacy_track[i]->note[j]->length = pack_igetl(fp);
					sp->legacy_track[i]->note[j]->flags = pack_getc(fp);
				}
			}

			/* read lyric track */
			b = pack_igetl(fp);
//			eof_vocal_track_resize(sp->vocal_track[0], b);
			eof_track_resize(sp, EOF_TRACK_VOCALS,b);
			for(j = 0; j < b; j++)
			{
				sp->vocal_track[0]->lyric[j]->note = pack_getc(fp);
				sp->vocal_track[0]->lyric[j]->pos = pack_igetl(fp);
				sp->vocal_track[0]->lyric[j]->length = pack_igetl(fp);
				c = pack_igetw(fp);
				pack_fread(sp->vocal_track[0]->lyric[j]->text, c, fp);
				sp->vocal_track[0]->lyric[j]->text[c] = '\0';
			}
			sp->vocal_track[0]->lines = pack_igetl(fp);
			for(j = 0; j < sp->vocal_track[0]->lines; j++)
			{
				sp->vocal_track[0]->line[j].start_pos = pack_igetl(fp);
				sp->vocal_track[0]->line[j].end_pos = pack_igetl(fp);
				sp->vocal_track[0]->line[j].flags = 0;
			}

			/* read bookmarks */
			for(i = 0; i < 10; i++)
			{
				sp->bookmark_pos[i] = pack_igetl(fp);
			}

			/* read fret catalog */
			sp->catalog->entries = pack_igetl(fp);
			for(i = 0; i < sp->catalog->entries; i++)
			{
				sp->catalog->entry[i].track = pack_getc(fp)+1;	//All EOF project formats before revision 'H' numbered tracks starting from 0 instead of 1, correct for this
				sp->catalog->entry[i].type = pack_getc(fp);
				sp->catalog->entry[i].start_pos = pack_igetl(fp);
				sp->catalog->entry[i].end_pos = pack_igetl(fp);
			}
			return sp;
		}
		case 'F':
		{
			eof_tracks_max = 5;			//This was the value of EOF_TRACKS_MAX

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

			/* read tracks */
			for(i = 0; i < eof_tracks_max; i++)
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
//				eof_legacy_track_resize(sp->legacy_track[i], b);
				eof_track_resize(sp, i + 1,b);
				for(j = 0; j < b; j++)
				{
					sp->legacy_track[i]->note[j]->type = pack_getc(fp);
					sp->legacy_track[i]->note[j]->note = pack_getc(fp);
					sp->legacy_track[i]->note[j]->pos = pack_igetl(fp);
					sp->legacy_track[i]->note[j]->length = pack_igetl(fp);
					sp->legacy_track[i]->note[j]->flags = pack_getc(fp);
				}
			}

			/* read lyric track */
			b = pack_igetl(fp);
//			eof_vocal_track_resize(sp->vocal_track[0], b);
			eof_track_resize(sp, EOF_TRACK_VOCALS,b);
			for(j = 0; j < b; j++)
			{
				sp->vocal_track[0]->lyric[j]->note = pack_getc(fp);
				sp->vocal_track[0]->lyric[j]->pos = pack_igetl(fp);
				sp->vocal_track[0]->lyric[j]->length = pack_igetl(fp);
				c = pack_igetw(fp);
				pack_fread(sp->vocal_track[0]->lyric[j]->text, c, fp);
				sp->vocal_track[0]->lyric[j]->text[c] = '\0';
			}
			sp->vocal_track[0]->lines = pack_igetl(fp);
			for(j = 0; j < sp->vocal_track[0]->lines; j++)
			{
				sp->vocal_track[0]->line[j].start_pos = pack_igetl(fp);
				sp->vocal_track[0]->line[j].end_pos = pack_igetl(fp);
				sp->vocal_track[0]->line[j].flags = pack_igetl(fp);
			}

			/* read bookmarks */
			for(i = 0; i < 10; i++)
			{
				sp->bookmark_pos[i] = pack_igetl(fp);
			}

			/* read fret catalog */
			sp->catalog->entries = pack_igetl(fp);
			for(i = 0; i < sp->catalog->entries; i++)
			{
				sp->catalog->entry[i].track = pack_getc(fp)+1;	//All EOF project formats before revision 'H' numbered tracks starting from 0 instead of 1, correct for this
				sp->catalog->entry[i].type = pack_getc(fp);
				sp->catalog->entry[i].start_pos = pack_igetl(fp);
				sp->catalog->entry[i].end_pos = pack_igetl(fp);
			}
			return sp;
		}
	}
	if(sp)
	{
		eof_destroy_song(sp);
	}
	return NULL;
}