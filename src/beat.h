#ifndef EOF_BEAT_H
#define EOF_BEAT_H

//extern EOF_BEAT_MARKER eof_beat[EOF_MAX_BEATS];
//extern int eof_beats;

int eof_get_beat(unsigned long pos);
int eof_get_beat_length(int beat);
int eof_find_previous_anchor(int cbeat);
int eof_find_next_anchor(int cbeat);
int eof_beat_is_anchor(int cbeat);
int eof_beat_is_moveable(int cbeat);
int eof_beat_is_bpm_change(int cbeat);
void eof_calculate_beats(void);
void eof_change_bpm(int cbeat, unsigned long ppqn);
void eof_realign_beats(int cbeat);
void eof_recalculate_beats(int cbeat);

#endif
