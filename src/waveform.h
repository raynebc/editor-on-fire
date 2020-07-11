#ifndef EOF_WAVEFORM_H
#define EOF_WAVEFORM_H

#include <alogg.h>

struct waveformslice
{
	unsigned min;	//The trough (lowest) amplitude for the samples
	unsigned peak;	//The peak amplitude for the samples
	double rms;		//The root mean square for the samples
};

struct waveformchanneldata
{
	struct waveformslice *slices;	//The waveform data for this channel
	unsigned maxamp;				//The highest amplitude of samples in this channel
	unsigned long maxampoffset;		//The difference between the zero amplitude and the channel's maximum amplitude (cached to optimize rendering)
	unsigned long yaxis;			//The y coordinate representing the y axis the channel's graph will render to
	unsigned long height;			//The height of this channel's graph
	unsigned long halfheight;		//One half of the channel's graph height (cached in eof_render_waveform() to avoid recalculating for each line)
};

struct wavestruct
{
	char *oggfilename;
	ALOGG_OGG *oggstruct;
	unsigned long slicelength;		//The length of one slice of the graph in milliseconds
	unsigned long slicesize;		//The number of samples in one slice of the graph
	unsigned long numslices;		//The number of waveform structures in the arrays below
	unsigned int zeroamp;			//The amplitude representing a 0 amplitude for this waveform (32768 for 16 bit audio samples, 128 for 8 bit audio samples)
	char is_stereo;					//This OGG has two audio channels

	struct waveformchanneldata left;	//The amplitude and graph data for the audio's left channel
	struct waveformchanneldata right;	//The amplitude and graph data for the audio's right channel (if applicable)
};

extern struct wavestruct *eof_waveform;		//Stores the waveform data
extern char eof_display_waveform;			//Specifies whether the waveform display is enabled
extern char eof_waveform_renderlocation;	//Specifies where and how high the graph will render (0 = fretboard area, 1 = editor window)
extern char eof_waveform_renderleftchannel;	//Specifies whether the left channel's graph should render
extern char eof_waveform_renderrightchannel;//Specifies whether the right channel's graph should render

void eof_destroy_waveform(struct wavestruct *ptr);
	//frees memory used by the specified waveform structure
int eof_waveform_slice_mean(struct waveformslice *left, struct waveformslice *right, struct wavestruct *waveform, unsigned long slicestart, unsigned long num);
	//performs a mathematical mean on the specified waveform slice data, returning the results via left and right if they aren't NULL, which will hold the values for the left and right channels, respectively.
	//slice numbering begins with 0
	//returns nonzero on error
int eof_render_waveform(struct wavestruct *waveform);
	//Renders the left channel waveform into the editor window, taking the zoom level into account
	//Returns nonzero on failure
void eof_render_waveform_line(unsigned int zeroamp, struct waveformchanneldata *channel, unsigned amp, unsigned long x, int color);
	//Given the amplitude and the channel and zero amplitude of the waveform to process, draws the vertical line for the channel's waveform originating from point x and the channel's defined y axis coordinate
	//waveform and channel are assumed to not be NULL, so it's the calling function's responsibility to ensure the pointers are valid

struct wavestruct *eof_create_waveform_old(char *oggfilename, unsigned long slicelength);
struct wavestruct *eof_create_waveform_new(char *oggfilename, unsigned long slicelength);
	//Decompresses the specified OGG file into memory and creates waveform data
	//slicelength is the length of one waveform graph slice in milliseconds
	//The correct number of samples are used to represent each column (slice) of the graph
	//The waveform data is returned, otherwise NULL is returned upon error

int eof_decode_one_ogg_buffer(ALOGG_OGG *ogg, SAMPLE *sample);
	//Erases the sample buffer and decodes enough samples from the specified OGG to fill the buffer
	//16 bit samples are expected, and ogg and sample are expected to both be the same number of channels (mono or stereo)
	//Returns 0 on error

int eof_decode_ogg_samples(ALOGG_OGG *ogg, SAMPLE *sample, unsigned long count);
	//Decodes the specified number of samples of the input OGG to the given PCM sample buffer, or however many samples can be decoded until end of file
	//16 bit samples are expected, and ogg and sample are expected to both be the same number of channels (mono or stereo)
	//Returns 0 on error

int eof_process_next_waveform_slice_old(struct wavestruct *waveform, SAMPLE *audio, unsigned long slicenum);
int eof_process_next_waveform_slice_new(struct wavestruct *waveform, SAMPLE *audio, unsigned long slicenum);
	//Processes waveform->slicesize number of audio samples, or if there are not enough, the remainder of the samples, storing the peak amplitude and RMS into waveform->slices[slicenum]
	//If the audio is stereo, the data for the right channel is likewise processed and stored into waveform->slices2[slicenum]
	//Returns 0 on success, 1 when all samples are exhausted or -1 on error

void eof_dump_waveform_data(struct wavestruct *waveform, char *filename);
	//A debugging function for writing the provided waveform structure's data to disk

#endif
