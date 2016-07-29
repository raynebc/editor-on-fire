#include <math.h>	//For sqrt()
#include <allegro.h>
#include "utility.h"
#include "waveform.h"
#include "song.h"
#include "main.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

struct wavestruct *eof_waveform = NULL;	//Stores the waveform data
char eof_display_waveform = 0;			//Specifies whether the waveform display is enabled
char eof_waveform_renderlocation = 0;		//Specifies where and how high the graph will render (0 = fretboard area, 1 = editor window)
char eof_waveform_renderleftchannel = 1;	//Specifies whether the left channel's graph should render
char eof_waveform_renderrightchannel = 0;	//Specifies whether the right channel's graph should render

int eof_waveform_slice_mean(struct waveformslice *left, struct waveformslice *right, struct wavestruct *waveform, unsigned long slicestart, unsigned long num)
{
// 	eof_log("eof_waveform_slice_mean() entered");

	unsigned long ctr, ctr2;
	struct waveformslice *results;
	struct waveformslice *channel = NULL;

//Validate parameters
	if(waveform == NULL)
		return 1;

	if((left == NULL) && (right == NULL))
		return 0;	//Return without doing anything

//Calculate mean data
	for(ctr = 0; ctr < 2; ctr++)
	{	//For each possible channel
		if((ctr == 0) && (left != NULL) && (waveform->left.slices))
		{	//If this is the left channel, its processing is enabled and there is waveform data for this channel
			channel = waveform->left.slices;	//Point to left channel data
			results = left;
		}
		else if((ctr == 1) && (right != NULL) && (waveform->right.slices))
		{	//Or if this is the right channel, its processing is enabled and there is waveform data for this channel
			channel = waveform->right.slices;	//Point to right channel data
			results = right;
		}
		else
		{	//Otherwise skip this channel
			continue;
		}
		results->min = results->peak = results->rms = 0;	//Reset this channel's result data

		for(ctr2 = 0; ctr2 < num; ctr2++)
		{	//For each requested waveform slice, add the data
			if(slicestart + ctr2 + 1 > waveform->numslices)	//If the next slice to process doesn't exist
			{
				if(ctr2 == 0)	//If the first amplitude is unavailable,
					results->min = results ->peak = results->rms = waveform->zeroamp;	//Set it to a 0dB amplitude

				break;					//exit this loop
			}

			results->min += channel[slicestart + ctr2].min;
			results->peak += channel[slicestart + ctr2].peak;
			results->rms += channel[slicestart + ctr2].rms;
		}

		if(ctr2 > 1)
		{	//If at least two slices' data was added, calculate the mean, round up
			results->min = ((double)results->min / ctr2 + 0.5);
			results->peak = ((double)results->peak / ctr2 + 0.5);
			results->rms = ((double)results->rms / ctr2 );
		}
	}

	return 0;	//Return success
}

void eof_destroy_waveform(struct wavestruct *ptr)
{
 	eof_log("eof_destroy_waveform() entered", 1);

	if(ptr)
	{
		if(ptr->oggfilename)
			free(ptr->oggfilename);
		if(ptr->left.slices)
			free(ptr->left.slices);
		if(ptr->right.slices)
			free(ptr->right.slices);
		free(ptr);
	}
}

int eof_render_waveform(struct wavestruct *waveform)
{
// 	eof_log("eof_render_waveform() entered");

	unsigned long x, startslice, startpixel, ctr;
	struct waveformslice left = {0, 0, 0.0}, right = {0, 0, 0.0};
	unsigned long ycoord1, ycoord2;	//Stores the Y coordinates of graph 1's and 2's Y axis
	unsigned long height;		//Stores the heigth of the fretboard area
	unsigned long top, bottom;	//Stores the top and bottom coordinates for the area the graph will render to
	char numgraphs;		//Stores the number of channels to render
	unsigned long pos = eof_music_pos / eof_zoom;
	int render_right_channel = 0;	//This caches the condition of the user having enabled the right channel to render AND the audio is stereo
	unsigned int zeroamp;
	struct waveformchanneldata *leftchannel;
	struct waveformchanneldata *rightchannel = NULL;

//validate input
	if(!eof_song_loaded || !waveform)
		return 1;	//Return error
	if(!waveform->left.slices)
		return 1;	//Return error if the waveform graph has no left/mono channel data
	if(waveform->is_stereo && !waveform->right.slices)
		return 1;	//Return error if the stereo waveform graph has no right channel data

//determine how many channels will be graphed
	if(waveform->is_stereo)
	{	//Take both channels into account
		numgraphs = eof_waveform_renderleftchannel + eof_waveform_renderrightchannel;
	}
	else
	{	//Take only the first channel into account
		numgraphs = eof_waveform_renderleftchannel;
	}
	if(numgraphs == 0)	//If user specified not to render either channel, or to render the right channel on a mono audio file
		return 0;

//determine timestamp of the left visible edge of the piano roll, which will be in ms, the same as the length of each waveform slice
	startslice = eof_determine_piano_roll_left_edge();

//determine which pixel is the left visible edge of the piano roll
	if(pos < 300)
	{
		startpixel = 20;
	}
	else if(pos < 320)
	{
		startpixel = 320 - pos;
	}
	else
	{
		startpixel = 0;
	}

//determine the top and bottom boundary for the graphing area
	if(eof_waveform_renderlocation == 0)
	{	//Render one or both channels' graphs into the fretboard area
		if(eof_selected_track == EOF_TRACK_VOCALS)
		{	//Set the top boundary 1 pixel below the lyric lane (at the top of the fretboard area in the vocal editor)
			top = EOF_EDITOR_RENDER_OFFSET + 15 + eof_screen_layout.lyric_y + 1 + 16 + 1;
		}
		else
		{	//Set the top boundary to the top of the fretboard area
			top = EOF_EDITOR_RENDER_OFFSET + 25;
		}

		bottom = EOF_EDITOR_RENDER_OFFSET + eof_screen_layout.fretboard_h - 1;	//Set the bottom boundary to the bottom of the fretboard area
	}
	else
	{	//Render one or both channels' graphs into the editor window
		top = 32;												//Set the top of the editor window
		bottom = eof_screen_layout.scrollbar_y - 1;				//Set the bottom boundary to just above the scroll bar
	}

//determine the y axis location and graph height of each channel's graph
	height = (bottom - top) / numgraphs;
	ycoord1 = top + (height / 2);	//The first graph will render with respect to the top of the graphing area
	if(numgraphs == 1)
	{
		if(eof_waveform_renderleftchannel)
		{	//If only rendering the left channel
			waveform->left.height = height;
			waveform->left.halfheight = height / 2;
			waveform->left.yaxis = ycoord1;
		}
		else
		{	//If only rendering the right channel
			waveform->right.height = height;
			waveform->right.halfheight = height / 2;
			waveform->right.yaxis = ycoord1;
		}
	}
	else if(numgraphs == 2)
	{
		ycoord2 = bottom - (height / 2);	//This graph will take 1/2 the entire graphing area, oriented at the bottom

		waveform->left.height = height;
		waveform->left.yaxis = ycoord1;
		waveform->right.height = height;
		waveform->right.yaxis = ycoord2;
		waveform->left.halfheight = waveform->right.halfheight = height / 2;
	}
	else
	{	//Do not render anything unless it's the graph for 1 or 2 channels
		return 1;
	}

//render graph from left to right, one pixel at a time (each pixel represents eof_zoom number of milliseconds of audio)
	zeroamp = waveform->zeroamp;			//Cache this for each channel so it doesn't have to be dereferenced more often than necessary
	leftchannel = &waveform->left;	//Cache this so waveform doesn't have to be dereferenced more often than necessary
	if(eof_waveform_renderrightchannel && waveform->is_stereo)
	{
		render_right_channel = 1;
		rightchannel = &waveform->right;
	}
	for(x = startpixel, ctr = 0; x < eof_window_editor->w; x++, ctr += eof_zoom)
	{	//for each pixel in the piano roll's visible width
		if(eof_waveform_slice_mean(&left, &right, waveform, startslice + ctr, eof_zoom) != 0)
			continue;	//If the processing was not successful, skip this iteration

		if(eof_waveform_renderleftchannel)
		{	//If the left channel rendering is enabled
			if(waveform->left.maxampoffset)
			{	//Additional check to avoid divide by zero on account of silent chart audio
				if(left.peak != zeroamp)	//If there was a nonzero left peak amplitude, scale it to the channel's maximum amplitude and scale again to half the fretboard's height and render it in green
				{
					if(left.peak < zeroamp)	//If the peak is a negative amplitude
					{	//Render it after the minimum amplitude to ensure it is visible
						eof_render_waveform_line(zeroamp, leftchannel, left.min, x, eof_color_waveform_trough);	//Render the minimum amplitude (default color is dark green)
						eof_render_waveform_line(zeroamp, leftchannel, left.rms, x, eof_color_waveform_rms);	//Render the root mean square amplitude (default color is red)
						eof_render_waveform_line(zeroamp, leftchannel, left.peak, x, eof_color_waveform_peak);	//Render the peak amplitude (default color is green)
					}
					else
					{	//Otherwise render it first
						eof_render_waveform_line(zeroamp, leftchannel, left.peak, x, eof_color_waveform_peak);	//Render the peak amplitude
						eof_render_waveform_line(zeroamp, leftchannel, left.rms, x, eof_color_waveform_rms);	//Render the root mean square
						eof_render_waveform_line(zeroamp, leftchannel, left.min, x, eof_color_waveform_trough);	//Render the minimum amplitude
					}
				}
			}
		}

		if(render_right_channel)
		{	//If the right channel rendering is enabled and the audio file is in stereo
			if(waveform->right.maxampoffset)
			{	//Additional check to avoid divide by zero on account of silent chart audio
				if(right.peak != zeroamp)	//If there was a nonzero right peak amplitude, scale it to the channel's maximum amplitude and scale again to half the fretboard's height and render it in green
				{
					if(right.peak < zeroamp)	//If the peak is a negative amplitude
					{	//Render it after the minimum amplitude to ensure it is visible
						eof_render_waveform_line(zeroamp, rightchannel, right.min, x, eof_color_waveform_trough);	//Render the minimum amplitude
						eof_render_waveform_line(zeroamp, rightchannel, right.rms, x, eof_color_waveform_rms);		//Render the root mean square
						eof_render_waveform_line(zeroamp, rightchannel, right.peak, x, eof_color_waveform_peak);	//Render the peak amplitude
					}
					else
					{	//Otherwise render it first
						eof_render_waveform_line(zeroamp, rightchannel, right.peak, x, eof_color_waveform_peak);	//Render the peak amplitude
						eof_render_waveform_line(zeroamp, rightchannel, right.rms, x, eof_color_waveform_rms);		//Render the root mean square
						eof_render_waveform_line(zeroamp, rightchannel, right.min, x, eof_color_waveform_trough);	//Render the minimum amplitude
					}
				}
			}
		}
	}

	return 0;
}

void eof_render_waveform_line(unsigned int zeroamp, struct waveformchanneldata *channel, unsigned amp, unsigned long x, int color)
{
	unsigned long yoffset;	//The offset from the y axis coordinate to render the line to

	if(amp > zeroamp)	//Render positive amplitude
	{	//Transform y to fit between 0 and zeroamp, then scale to fit the graph
		yoffset = (amp - zeroamp) * channel->halfheight / channel->maxampoffset;
		vline(eof_window_editor->screen, x, channel->yaxis, channel->yaxis - yoffset, color);
	}
	else
	{	//Correct the negative amplitude, then scale it to fit the graph
		yoffset = (zeroamp - amp) * channel->halfheight / channel->maxampoffset;
		vline(eof_window_editor->screen, x, channel->yaxis, channel->yaxis + yoffset, color);
	}
}

struct wavestruct *eof_create_waveform(char *oggfilename, unsigned long slicelength)
{
	ALOGG_OGG *oggstruct = NULL;
	SAMPLE *audio = NULL;
	void * oggbuffer = NULL;
	struct wavestruct *waveform = NULL;
	static struct wavestruct emptywaveform;	//all variables in this auto initialize to value 0
	char done = 0;	//-1 on unsuccessful completion, 1 on successful completion
	unsigned long slicenum = 0;

 	eof_log("\tGenerating waveform", 1);
 	eof_log("eof_create_waveform() entered", 1);

	set_window_title("Generating Waveform Graph...");

	if((oggfilename == NULL) || !slicelength)
	{
		eof_log("Waveform: Invalid parameters", 1);
		return NULL;
	}

//Load OGG file into memory
	oggbuffer = eof_buffer_file(oggfilename, 0);
	if(!oggbuffer)
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Waveform: Failed to open input audio file: %s",strerror(errno));
		eof_log(eof_log_string, 1);
		return NULL;
	}
	oggstruct = alogg_create_ogg_from_buffer(oggbuffer, (int)file_size_ex(oggfilename));
	if(oggstruct == NULL)
	{
		eof_log("Waveform: ALOGG failed to open input audio file", 1);
		free(oggbuffer);
		return NULL;
	}

//Decode OGG into memory
	audio = alogg_create_sample_from_ogg(oggstruct);
	if(audio == NULL)
	{
		eof_log("Waveform: ALOGG failed to decode input audio file", 1);
		done = -1;
	}
	else if((audio->bits != 8) && (audio->bits != 16))	//This logic currently only supports 8 and 16 bit audio
	{
		eof_log("Waveform: Invalid sample size", 1);
		done = -1;
	}
	else
	{
//Initialize waveform structure
		waveform = (struct wavestruct *)malloc(sizeof(struct wavestruct));
		if(waveform == NULL)
		{
			eof_log("Waveform: Unable to allocate memory for the waveform structure", 1);
			done = -1;
		}
		else
		{
			*waveform = emptywaveform;					//Set all variables to value zero
			waveform->slicelength = slicelength;
			if(alogg_get_wave_is_stereo_ogg(oggstruct))	//If this audio file has two audio channels
				waveform->is_stereo = 1;
			else
				waveform->is_stereo = 0;

			if(audio->bits == 8)
				waveform->zeroamp = 128;	//128 represents amplitude 0 for unsigned 8 bit audio samples
			else
				waveform->zeroamp = 32768;	//32768 represents amplitude 0 for unsigned 16 bit audio samples

			waveform->oggfilename = (char *)malloc(strlen(oggfilename)+1);
			if(waveform->oggfilename == NULL)
			{
				eof_log("Waveform: Unable to allocate memory for the audio filename string", 1);
				done = -1;
			}
			else
			{
				waveform->slicesize = audio->freq * slicelength / 1000;	//Find the number of samples in each slice
				if((audio->freq * slicelength) % 1000)					//If there was any remainder
					waveform->slicesize++;								//Increment the size of the slice

				waveform->numslices = (double)audio->len / ((double)audio->freq * (double)slicelength / 1000.0);	//Find the number of slices to process
				if(audio->len % waveform->numslices)		//If there's any remainder
					waveform->numslices++;					//Increment the number of slices

				strcpy(waveform->oggfilename,oggfilename);
				waveform->left.slices = (struct waveformslice *)malloc(sizeof(struct waveformslice) * waveform->numslices);
				if(waveform->left.slices == NULL)
				{
					eof_log("Waveform: Unable to allocate memory for the left channel waveform data", 1);
					done = -1;
				}
				else if(waveform->is_stereo)	//If this OGG is stereo
				{				//Allocate memory for the right channel waveform data
					waveform->right.slices = (struct waveformslice *)malloc(sizeof(struct waveformslice) * waveform->numslices);
					if(waveform->right.slices == NULL)
					{
						eof_log("Waveform: Unable to allocate memory for the right channel waveform data", 1);
						done = -1;
					}
				}
			}
		}
	}

	while(!done)
	{
		done = eof_process_next_waveform_slice(waveform, audio, slicenum++);
	}

//Cleanup
	if(oggstruct != NULL)
		alogg_destroy_ogg(oggstruct);
	if(audio != NULL)
		destroy_sample(audio);
	if(oggbuffer)
		free(oggbuffer);
	if(done == -1)	//Unsuccessful completion
	{
		if(waveform)
		{
			if(waveform->oggfilename)
				free(waveform->oggfilename);
			free(waveform);
		}
		allegro_message("Failed to generate waveform.  See log for details");
		return NULL;	//Return error
	}

	//Cache the difference between each channel's zero amplitude and its maximum amplitude for optimized rendering
	if(waveform->left.maxamp > waveform->zeroamp)
		waveform->left.maxampoffset = waveform->left.maxamp - waveform->zeroamp;
	else
		waveform->left.maxampoffset = waveform->zeroamp - waveform->left.maxamp;
	if(waveform->is_stereo)
	{	//If there is right channel waveform data
		if(waveform->right.maxamp > waveform->zeroamp)
			waveform->right.maxampoffset = waveform->right.maxamp - waveform->zeroamp;
		else
			waveform->right.maxampoffset = waveform->zeroamp - waveform->right.maxamp;
	}

	eof_log("\tWaveform generated", 1);
	return waveform;	//Return waveform data
}

int eof_process_next_waveform_slice(struct wavestruct *waveform, SAMPLE *audio, unsigned long slicenum)
{
	unsigned long sampleindex = 0;	//The byte index into audio->data
	unsigned long startsample = 0;	//The sample number of the first sample being processed
	unsigned long samplesize = 0;	//Number of bytes for each sample: 1 for 8 bit audio, 2 for 16 bit audio.  Doubled for stereo
	unsigned long ctr = 0;
	double sum;			//Stores the sums of each sample's square (for finding the root mean square)
	double rms;			//Stores the root square mean
	unsigned min;		//Stores the lowest amplitude for the slice
	unsigned peak;		//Stores the highest amplitude for the slice
	unsigned long sample = 0;
	char firstread;		//Set to nonzero after the first sample is read into the min/max variables
	char channel = 0;
	struct waveformslice *dest;	//The structure to write this slice's data to
	char outofsamples = 0;		//Will be set to 1 if all samples in the audio structure have been processed

//Validate parameters
	if((waveform == NULL) || (waveform->left.slices == NULL) || (audio == NULL))
		return -1;	//Return error

	if(waveform->is_stereo && (waveform->right.slices == NULL))
		return -1;	//Return error

	if((slicenum > waveform->numslices))	//If this is more than the number of slices that were supposed to be read
		return 1;	//Return out of samples

	samplesize = audio->bits / 8;
	if(waveform->is_stereo)		//Stereo data is interleaved as left channel, right channel, ...
		samplesize += samplesize;	//Double the sample size

	for(channel = 0; channel <= waveform->is_stereo; channel++)	//Process loop once for mono track, twice for stereo track
	{
//Initialize processing for this audio channel
		sum = rms = min = peak = firstread = 0;
		startsample = (double)slicenum * (double)audio->freq * (double)waveform->slicelength / 1000.0;	//This is the sample index for this slices starting sample
		sampleindex = startsample * samplesize;		//This is the byte index for this slice's starting sample number

		if(channel)							//If processing the sample for the right channel
			sampleindex += (audio->bits / 8);	//Seek past the left channel sample

//Process audio samples for this channel
		for(ctr = 0; ctr < waveform->slicesize; ctr++)
		{
			if(startsample + ctr >= audio->len)	//If there are no more samples to read
			{
				outofsamples = 1;
				break;
			}

			sample = ((unsigned char *)audio->data)[sampleindex];	//Store first sample byte (Allegro documentation states the sample data is stored in unsigned format)
			if(audio->bits > 8)	//If this sample is more than one byte long (16 bit)
				sample += ((unsigned char *)audio->data)[sampleindex+1]<<8;	//Assume little endian byte order, read the next (high byte) of data

			if(!firstread)			//If this is the first sample
			{
				min = peak = sample;	//Assume it is the highest and lowest amplitude until found otherwise
				firstread = 1;
			}
			else					//Track the highest and lowest amplitude
			{
				if(sample > peak)
					peak = sample;
				if(sample < min)
					min = sample;
			}

			sum += ((double)sample*sample)/waveform->slicesize;	//Add the square of this sample divided by the number of samples to read
			sampleindex += samplesize;		//Adjust index to point to next sample for this channel
		}
		rms = sqrt(sum);

//Store results to the appropriate channel's waveform data array
		if(channel == 0)
		{
			dest = &(waveform->left.slices[slicenum]);	//Store results to mono/left channel array
			if(peak > waveform->left.maxamp)
				waveform->left.maxamp = peak;	//Track absolute maximum amplitude of mono/left channel
		}
		else
		{
			dest = &(waveform->right.slices[slicenum]);	//Store results to right channel array
			if(peak > waveform->right.maxamp)
				waveform->right.maxamp = peak;	//Track absolute maximum amplitude of right channel
		}

		dest->min = min;
		dest->peak = peak;
		dest->rms = rms;
	}

	return outofsamples;	//Return success/completed status
}
