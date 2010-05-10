#ifndef EOF_SONG_H
#define EOF_SONG_H

#define EOF_OLD_MAX_NOTES     65536
#define EOF_MAX_NOTES         32768
#define EOF_MAX_LYRICS        32768
#define EOF_MAX_LYRIC_LINES    4096
#define EOF_MAX_TRACKS            5
#define EOF_MAX_CATALOG_ENTRIES 256
#define EOF_MAX_INI_SETTINGS     32

#define EOF_NOTE_SUPAEASY 0
#define EOF_NOTE_EASY     1
#define EOF_NOTE_MEDIUM   2
#define EOF_NOTE_AMAZING  3
#define EOF_NOTE_SPECIAL  4

#define EOF_TRACK_GUITAR      0
#define EOF_TRACK_BASS        1
#define EOF_TRACK_GUITAR_COOP 2
#define EOF_TRACK_RHYTHM      3
#define EOF_TRACK_DRUM        4
#define EOF_TRACK_VOCALS      5

#define EOF_NOTE_FLAG_HOPO     1
#define EOF_NOTE_FLAG_SP       2
#define EOF_NOTE_FLAG_CRAZY    4
#define EOF_NOTE_FLAG_F_HOPO   8
#define EOF_NOTE_FLAG_NO_HOPO 16

#define EOF_MAX_BEATS 32768

#define EOF_MAX_SOLOS      32

#define EOF_MAX_STAR_POWER 32

#define EOF_BEAT_FLAG_ANCHOR      1
#define EOF_BEAT_FLAG_EVENTS      2
#define EOF_BEAT_FLAG_START_4_4   4
#define EOF_BEAT_FLAG_START_3_4   8
#define EOF_BEAT_FLAG_START_5_4  16
#define EOF_BEAT_FLAG_START_6_4  32

#define EOF_LYRIC_LINE_FLAG_OVERDRIVE 1

#define EOF_MAX_TEXT_EVENTS 4096

typedef struct
{
	
	char          type;
	char          note;
	unsigned long pos;
	long          length;
	char          flags;
	
} EOF_NOTE;

typedef struct
{
	
	char          type;
	char          note;
	short         endbeat;
	short         beat;       // which beat this note was copied from
	unsigned long pos;
	long          length;
	float         porpos;     // position of note within the beat (100.0 = full beat)
	float         porendpos;
	char          active;
	
} EOF_EXTENDED_NOTE;

typedef struct
{
	
	char          note;
	char          text[256];
	unsigned long pos;
	long          length;
	
} EOF_LYRIC;

typedef struct
{
	
	char          note;
	char          text[256];
	unsigned long pos;
	long          length;
	
	short         beat;       // which beat this note was copied from
	short         endbeat;    // which beat this note was copied from
	float         porpos;     // position of note within the beat (100.0 = full beat)
	float         porendpos;
	
} EOF_EXTENDED_LYRIC;

typedef struct
{
	
	int start_pos;
	int end_pos;
	int flags;
	
} EOF_LYRIC_LINE;

typedef struct
{
	
	int start_pos;
	int end_pos;
	
} EOF_SOLO_ENTRY;

typedef struct
{
	
	int start_pos;
	int end_pos;
	
} EOF_STAR_POWER_ENTRY;

typedef struct
{
	
	EOF_NOTE * note[EOF_MAX_NOTES];
	unsigned long notes;
	
	/* solos */
	EOF_SOLO_ENTRY solo[EOF_MAX_SOLOS];
	short solos;
	
	/* star power */
	EOF_STAR_POWER_ENTRY star_power_path[EOF_MAX_STAR_POWER];
	short star_power_paths;
	
} EOF_TRACK;

typedef struct
{
	
	EOF_LYRIC * lyric[EOF_MAX_LYRICS];
	unsigned long lyrics;
	
	/* lyric lines */
	EOF_LYRIC_LINE line[EOF_MAX_LYRIC_LINES];
	short lines;
	
} EOF_VOCAL_TRACK;

typedef struct
{

	unsigned long ppqn;
	unsigned long pos;
	unsigned long flags;
	
	double fpos;

} EOF_BEAT_MARKER;

typedef struct
{
	
	char filename[256];
	int  midi_offset;
	
} EOF_OGG_INFO;

typedef struct
{
	
	char          artist[256];
	char          title[256];
	char          frettist[256];
	char          year[32];
	char          loading_text[512];
	char          lyrics;
	char          eighth_note_hopo;
	
	EOF_OGG_INFO  ogg[8];
	short         oggs;
	
	char          ini_setting[EOF_MAX_INI_SETTINGS][512];
	short         ini_settings;
	
	unsigned long revision;
	
} EOF_SONG_TAGS;

typedef struct
{
	
	char track;
	char type;
	int start_pos;
	int end_pos;
	
} EOF_CATALOG_ENTRY;

typedef struct
{
	
	char text[256];
	int beat;
	
} EOF_TEXT_EVENT;

typedef struct
{
	
	EOF_CATALOG_ENTRY entry[EOF_MAX_CATALOG_ENTRIES];
	int entries;
	
} EOF_CATALOG;

typedef struct
{
	
	/* song info */
	EOF_SONG_TAGS * tags;
	
	/* track data */
	EOF_TRACK       * track[EOF_MAX_TRACKS];
	EOF_VOCAL_TRACK * vocal_track;
	
	EOF_BEAT_MARKER * beat[EOF_MAX_BEATS];
	int beats;
	
	EOF_TEXT_EVENT * text_event[EOF_MAX_TEXT_EVENTS];
	int text_events;
	
	/* miscellaneous */
	int bookmark_pos[10];
	EOF_CATALOG * catalog;
		
} EOF_SONG;

EOF_SONG * eof_create_song(void);
void eof_destroy_song(EOF_SONG * sp);
int eof_load_song_pf(EOF_SONG * sp, PACKFILE * fp);
EOF_SONG * eof_load_song(const char * fn);
int eof_save_song(EOF_SONG * sp, const char * fn);

EOF_NOTE * eof_track_add_note(EOF_TRACK * tp);
void eof_track_delete_note(EOF_TRACK * tp, int note);
void eof_track_sort_notes(EOF_TRACK * tp);
int eof_fixup_next_note(EOF_TRACK * tp, int note);
void eof_track_find_crazy_notes(EOF_TRACK * tp);
void eof_track_fixup_notes(EOF_TRACK * tp, int sel);
void eof_track_resize(EOF_TRACK * tp, int notes);
void eof_track_add_star_power(EOF_TRACK * tp, unsigned long start_pos, unsigned long end_pos);
void eof_track_delete_star_power(EOF_TRACK * tp, int index);
void eof_track_add_solo(EOF_TRACK * tp, unsigned long start_pos, unsigned long end_pos);
void eof_track_delete_solo(EOF_TRACK * tp, int index);

EOF_LYRIC * eof_vocal_track_add_lyric(EOF_VOCAL_TRACK * tp);
void eof_vocal_track_delete_lyric(EOF_VOCAL_TRACK * tp, int lyric);
void eof_vocal_track_sort_lyrics(EOF_VOCAL_TRACK * tp);
int eof_fixup_next_lyric(EOF_VOCAL_TRACK * tp, int lyric);
void eof_vocal_track_fixup_lyrics(EOF_VOCAL_TRACK * tp, int sel);
void eof_vocal_track_resize(EOF_VOCAL_TRACK * tp, int lyrics);
void eof_vocal_track_add_line(EOF_VOCAL_TRACK * tp, unsigned long start_pos, unsigned long end_pos);
void eof_vocal_track_delete_line(EOF_VOCAL_TRACK * tp, int index);

EOF_BEAT_MARKER * eof_song_add_beat(EOF_SONG * sp);
void eof_song_delete_beat(EOF_SONG * sp, int beat);
int eof_song_resize_beats(EOF_SONG * sp, int beats);

void eof_song_add_text_event(EOF_SONG * sp, int beat, char * text);
void eof_song_delete_text_event(EOF_SONG * sp, int event);
void eof_song_move_text_events(EOF_SONG * sp, int beat, int offset);
void eof_song_resize_text_events(EOF_SONG * sp, int events);
void eof_sort_events(void);

void eof_sort_notes(void);
void eof_fixup_notes(void);
void eof_detect_difficulties(EOF_SONG * sp);

#endif
