#ifndef EOF_MIX_H
#define EOF_MIX_H

#define EOF_MIX_MAX_CHANNELS 8
#define EOF_MAX_VOCAL_TONES 256

typedef struct
{

	SAMPLE * sp;
	double pos;
	char playing;
	int volume;	//A percent value from 0 to 100

} EOF_MIX_VOICE;

//extern int  eof_mix_buffer_size;
extern int  eof_mix_speed;
extern char eof_mix_claps_enabled;
extern char eof_mix_metronome_enabled;
extern char eof_mix_claps_note;
extern char eof_mix_vocal_tones_enabled;
extern char eof_mix_midi_tones_enabled;	//Tracks whether MIDI tones are enabled
extern char eof_mix_percussion_enabled;	//Tracks whether vocal percussion cues are enabled
extern EOF_MIX_VOICE eof_voice[EOF_MIX_MAX_CHANNELS];
extern int eof_clap_volume;	//Stores the volume level for the clap, specified as a percentage
extern int eof_tick_volume;	//Stores the volume level for the tick cue, specified as a percentage
extern int eof_tone_volume;	//Stores the volume level for the tone cue, specified as a percentage

void eof_mix_callback(void * buf, int length);	//Buffer callback function for alogg
unsigned long eof_mix_msec_to_sample(unsigned long msec, int freq);

void eof_mix_init(void);	//Inits variables and loads cues (clap, metronome tick, vocal tones)
void eof_mix_exit(void);	//Releases memory used by audio cues
void eof_mix_start_helper(void);	//Finds the next clap, tick and vocal tone timestamps?
void eof_mix_start(unsigned long start, int speed);	//Prepares variables for chart playback
void eof_mix_seek(int pos);	//Performs a seek and updates the position of the next of each audio cue
//void eof_mix_poll(void);
//void eof_mix_stop(void);

void eof_mix_find_claps(void);	//Populates clap, metronome, note_pos and note_note arrays based on the chart's contents
//void eof_mix_find_vocal_claps(void);	//Undefined function
void eof_mix_play_note(int note);	//Plays the vocal tone for the specified note, if available
void eof_midi_play_note(int note);	//Sends a Note On MIDI command

#endif
