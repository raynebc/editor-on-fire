#ifndef EOF_MIX_H
#define EOF_MIX_H

#define EOF_MIX_MAX_CHANNELS 8

typedef struct
{
	
	SAMPLE * sp;
	double pos;
	char playing;
	
} EOF_MIX_VOICE;

//extern int  eof_mix_buffer_size;
extern int  eof_mix_speed;
extern char eof_mix_claps_enabled;
extern char eof_mix_metronome_enabled;
extern char eof_mix_claps_note;
extern char eof_mix_vocal_tones_enabled;

unsigned long eof_mix_msec_to_sample(unsigned long msec, int freq);

void eof_mix_init(void);
void eof_mix_exit(void);
void eof_mix_start_helper(void);
void eof_mix_start(unsigned long start, int speed);
void eof_mix_seek(int pos);
//void eof_mix_poll(void);
//void eof_mix_stop(void);

void eof_mix_find_claps(void);
void eof_mix_find_vocal_claps(void);
void eof_mix_play_note(int note);

#endif
