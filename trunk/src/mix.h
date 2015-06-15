#ifndef EOF_MIX_H
#define EOF_MIX_H

#define EOF_MIX_MAX_CHANNELS 8
#define EOF_MAX_VOCAL_TONES 256

typedef struct
{

	SAMPLE * sp;
	long pos;
	double fpos;	//Since the sample increment is not 1.0 when the chart audio's sample rate is not 44.1KHz, a floating point position needs to be tracked
	char playing;
	int volume;	//A percent value from 0 to 100
	double multiplier;	//This is the value sqrt(volume/100.0), which must be multiplied to the voice's amplitude to adjust for the specified volume

} EOF_MIX_VOICE;

extern int  eof_mix_speed;
extern char eof_mix_claps_enabled;
extern char eof_mix_metronome_enabled;
extern char eof_mix_claps_note;
extern char eof_mix_vocal_tones_enabled;
extern char eof_mix_midi_tones_enabled;	//Tracks whether MIDI tones are enabled
extern char eof_mix_percussion_enabled;	//Tracks whether vocal percussion cues are enabled
extern EOF_MIX_VOICE eof_voice[EOF_MIX_MAX_CHANNELS];
extern int eof_chart_volume;				//Stores the volume level for the chart audio, specified as a percentage
extern double eof_chart_volume_multiplier;	//This is the value sqrt(volume/100.0), which must be multiplied to the voice's amplitude to adjust for the specified volume
extern int eof_clap_volume;					//Stores the volume level for the clap, specified as a percentage
extern int eof_tick_volume;					//Stores the volume level for the tick cue, specified as a percentage
extern int eof_tone_volume;					//Stores the volume level for the tone cue, specified as a percentage
extern int eof_percussion_volume;			//Stores the volume level for the vocal percussion cue, specified as a percentage
extern int eof_clap_for_mutes;				//Specifies whether fully string muted notes trigger the clap sound cue
extern int eof_selected_percussion_cue;		//The user selected percussion sound (cowbell by default), corresponds to the radio button in the eof_audio_cues_dialog[] array
extern SAMPLE *eof_sound_chosen_percussion;	//The user-selected percussion sound
extern SAMPLE *eof_sound_cowbell;
extern SAMPLE *eof_sound_tambourine1;
extern SAMPLE *eof_sound_tambourine2;
extern SAMPLE *eof_sound_tambourine3;
extern SAMPLE *eof_sound_triangle1;
extern SAMPLE *eof_sound_triangle2;
extern SAMPLE *eof_sound_woodblock1;
extern SAMPLE *eof_sound_woodblock2;
extern SAMPLE *eof_sound_woodblock3;
extern SAMPLE *eof_sound_woodblock4;
extern SAMPLE *eof_sound_woodblock5;
extern SAMPLE *eof_sound_woodblock6;
extern SAMPLE *eof_sound_woodblock7;
extern SAMPLE *eof_sound_woodblock8;
extern SAMPLE *eof_sound_woodblock9;
extern SAMPLE *eof_sound_woodblock10;
extern SAMPLE *eof_sound_clap1;
extern SAMPLE *eof_sound_clap2;
extern SAMPLE *eof_sound_clap3;
extern SAMPLE *eof_sound_clap4;
extern SAMPLE *eof_sound_grid_snap;

void eof_mix_callback_stereo(void * buf, int length);	//Buffer callback function for alogg, optimized for stereo samples
void eof_mix_callback_mono(void * buf, int length);	//Buffer callback function for alogg, optimized for mono samples
unsigned long eof_mix_msec_to_sample(unsigned long msec, int freq);

void eof_mix_init(void);	//Inits variables and loads cues (clap, metronome tick, vocal tones)
void eof_mix_exit(void);	//Releases memory used by audio cues
void eof_mix_start_helper(void);	//Finds the next clap, tick and vocal tone timestamps?
void eof_mix_start(int speed);	//Prepares variables for chart playback
void eof_mix_seek(int pos);	//Performs a seek and updates the position of the next of each audio cue
SAMPLE *eof_mix_load_ogg_sample(char *fn);	//Loads the specified OGG sample from a dat file, returning it in SAMPLE format

void eof_mix_find_claps(void);		//Populates counters and arrays for the sound cues based on the active track difficulty's contents
void eof_mix_play_note(int note);	//Plays the vocal tone for the specified note, if available
void eof_midi_play_note_ex(int note, unsigned char channel, unsigned char patch);
	//Sends MIDI commands to stop a note on the specified channel, change to the "clean guitar" MIDI instrument and plays the note
	//If patch is nonzero, MIDI commands to set it as the active instrument number are sent
void eof_play_pro_guitar_note_midi(EOF_SONG *sp, unsigned long track, unsigned long note);	//Immediately plays all the MIDI tones in the specified pro guitar note
void eof_play_queued_midi_tones(void);	//Advances through and plays MIDI tones queued by eof_mix_find_claps(), similarly to how the OGG callback function plays other cues
int eof_lookup_midi_tone(EOF_SONG *sp, unsigned long track, unsigned long note);	//Returns the appropriate MIDI instrument number to use for the specified note, or 0 on error

void eof_set_seek_position(int pos);
	//Updates variables to set the audio and seek position to the specified timestamp in ms

#endif
