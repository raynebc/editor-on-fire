#include <math.h>	//For sqrt()
#include <allegro.h>
#include "waveform.h"
#include "song.h"
#include "main.h"

struct wavestruct *eof_waveform=NULL;	//Stores the waveform data
char eof_display_waveform=0;			//Specifies whether the waveform display is enabled
char eof_waveform_renderlocation=0;		//Specifies where and how high the graph will render (0 = fretboard area, 1 = editor window)
char eof_waveform_renderleftchannel=1;	//Specifies whether the left channel's graph should render
char eof_waveform_renderrightchannel=0;	//Specifies whether the right channel's graph should render

int eof_waveform_slice_mean(struct waveformslice *left,struct waveformslice *right,struct wavestruct *waveform,unsigned long slicestart, unsigned long num)
{
	unsigned long ctr,ctr2;
	struct waveformslice results;
	struct waveformslice *channel=NULL;

//Validate parameters
	if(waveform == NULL)
		return 1;

	if((left == NULL) && (right == NULL))
		return 0;	//Return without doing anything

//Calculate mean data
	for(ctr=0;ctr < 2;ctr++)
	{	//For each possible channel
		if((ctr == 0) && (left == NULL))	//If not processing for the left channel
			continue;	//Skip it
		if((ctr == 1) && ((right == NULL) || (waveform->is_stereo == 0)))	//If not processing for the right channel, or the right channel doesn't exist
			continue;	//Skip it
		results.min=results.peak=results.rms=0;

		if(ctr == 0)
			channel = waveform->left.slices;	//Point to left channel data
		else
			channel = waveform->right.slices;	//Point to right channel data

		for(ctr2=0;ctr2 < num;ctr2++)
		{	//For each requested waveform slice, add the data
			if(slicestart + ctr2 + 1 > waveform->numslices)	//If the next slice to process doesn't exist
			{
				if(ctr2 == 0)	//If the first amplitude is unavailable,
					results.min=results.peak=results.rms=waveform->zeroamp;	//Set it to a 0dB amplitude

				break;					//exit this loop
			}

			results.min += channel[slicestart + ctr2].min;
			results.peak += channel[slicestart + ctr2].peak;
			results.rms += channel[slicestart + ctr2].rms;
		}

		if(ctr2 > 1)
		{	//If at least two slices' data was added, calculate the mean, round up
			results.min = ((float)results.min / ctr2 + 0.5);
			results.peak = ((float)results.peak / ctr2 + 0.5);
			results.rms = ((double)results.rms / ctr2 );
		}

		if((ctr == 0) && (left != NULL))
			*left = results;
		else if((ctr == 1) && (right != NULL))
			*right = results;
	}

	return 0;	//Return success
}

void eof_destroy_waveform(struct wavestruct *ptr)
{
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
	unsigned long x,startslice,startpixel,ctr;
	struct waveformslice left,right;
	unsigned long ycoord1,ycoord2;	//Stores the Y coordinates of graph 1's and 2's Y axis
	unsigned long height;		//Stores the heigth of the fretboard area
	unsigned long top,bottom;	//Stores the top and bottom coordinates for the area the graph will render to
	char numgraphs;		//Stores the number of channels to render
	unsigned long pos = eof_music_pos / eof_zoom;

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
		numgraphs = eof_waveform_renderrightchannel;
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
			waveform->left.yaxis = ycoord1;
		}
		else
		{	//If only rendering the right channel
			waveform->right.height = height;
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
	}
	else
	{	//Do not render anything unless it's the graph for 1 or 2 channels
		return 1;
	}

//render graph from left to right, one pixel at a time (each pixel represents eof_zoom number of milliseconds of audio)
	for(x=startpixel,ctr=0;x < eof_window_editor->w;x++,ctr+=eof_zoom)
	{	//for each pixel in the piano roll's visible width
		if(eof_waveform_slice_mean(&left,&right,waveform,startslice+ctr,eof_zoom) == 0)
		{	//processing was successful
			if(eof_waveform_renderleftchannel)
			{	//If the left channel rendering is enabled
				if(left.peak != waveform->zeroamp)	//If there was a nonzero left peak amplitude, scale it to the channel's maximum amplitude and scale again to half the fretboard's height and render it in green
				{
					if(left.peak < waveform->zeroamp)	//If the peak is a negative amplitude
					{	//Render it after the minimum amplitude to ensure it is visible
						eof_render_waveform_line(waveform,&waveform->left,left.min,x,makecol(0, 124, 0));	//Render the minimum amplitude in dark green
						eof_render_waveform_line(waveform,&waveform->left,left.rms,x,makecol(190, 0, 0));	//Render the root mean square amplitude in red
						eof_render_waveform_line(waveform,&waveform->left,left.peak,x,makecol(0, 190, 0));	//Render the peak amplitude in green
					}
					else
					{	//Otherwise render it first
						eof_render_waveform_line(waveform,&waveform->left,left.peak,x,makecol(0, 190, 0));	//Render the peak amplitude in green
						eof_render_waveform_line(waveform,&waveform->left,left.rms,x,makecol(190, 0, 0));	//Render the root mean square amplitude in red
						eof_render_waveform_line(waveform,&waveform->left,left.min,x,makecol(0, 124, 0));	//Render the minimum amplitude in dark green
					}
				}
			}

			if(eof_waveform_renderrightchannel)
			{	//If the right channel rendering is enabled
				if(waveform->is_stereo && (right.peak != waveform->zeroamp))	//If there was a nonzero right peak amplitude, scale it to the channel's maximum amplitude and scale again to half the fretboard's height and render it in green
				{
					if(right.peak < waveform->zeroamp)	//If the peak is a negative amplitude
					{	//Render it after the minimum amplitude to ensure it is visible
						eof_render_waveform_line(waveform,&waveform->right,right.min,x,makecol(0, 124, 0));	//Render the minimum amplitude in dark green
						eof_render_waveform_line(waveform,&waveform->right,right.rms,x,makecol(190, 0, 0));	//Render the root mean square amplitude in red
						eof_render_waveform_line(waveform,&waveform->right,right.peak,x,makecol(0, 190, 0));	//Render the peak amplitude in green
					}
					else
					{	//Otherwise render it first
						eof_render_waveform_line(waveform,&waveform->right,right.peak,x,makecol(0, 190, 0));	//Render the peak amplitude in green
						eof_render_waveform_line(waveform,&waveform->right,right.rms,x,makecol(190, 0, 0));	//Render the root mean square amplitude in red
						eof_render_waveform_line(waveform,&waveform->right,right.min,x,makecol(0, 124, 0));	//Render the minimum amplitude in dark green
					}
				}
			}
		}
	}

	return 0;
}

void eof_render_waveform_line(struct wavestruct *waveform,struct waveformchanneldata *channel,unsigned amp,unsigned long x,int color)
{
	unsigned long maxampoffset;	//The difference between the zero amplitude and the channel's maximum amplitude
	unsigned long yoffset;	//The offset from the y axis coordinate to render the line to

	if(waveform != NULL)
	{
		if(channel->maxamp > waveform->zeroamp)
			maxampoffset = channel->maxamp - waveform->zeroamp;
		else
			maxampoffset = waveform->zeroamp - channel->maxamp;

		if(amp > waveform->zeroamp)	//Render positive amplitude
		{	//Transform y to fit between 0 and zeroamp, then scale to fit the graph
			yoffset=(amp - waveform->zeroamp) * (channel->height / 2) / maxampoffset;
			vline(eof_window_editor->screen, x, channel->yaxis, channel->yaxis - yoffset, color);
		}
		else
		{	//Correct the negative amplitude, then scale it to fit the graph
			yoffset=(waveform->zeroamp - amp) * (channel->height / 2) / maxampoffset;
			vline(eof_window_editor->screen, x, channel->yaxis, channel->yaxis + yoffset, color);
		}
	}
}

#define EOF_DEBUG_WAVEFORM
struct wavestruct *eof_create_waveform(char *oggfilename,unsigned long slicelength)
{
	ALOGG_OGG *oggstruct=NULL;
	SAMPLE *audio=NULL;
	FILE *fp=NULL;
	struct wavestruct *waveform=NULL;
	static struct wavestruct emptywaveform;	//all variables in this auto initialize to value 0
	char done=0;	//-1 on unsuccessful completion, 1 on successful completion
	unsigned long slicenum=0;

	if((oggfilename == NULL) || !slicelength)
	{
		#ifdef EOF_DEBUG_WAVEFORM
		allegro_message("Waveform: Invalid parameters");
		#endif
		return NULL;
	}

//Load OGG file into memory
	fp=fopen(oggfilename,"rb");
	if(fp == NULL)
	{
		#ifdef EOF_DEBUG_WAVEFORM
		allegro_message("Waveform: Failed to open input audio file");
		#endif
		return NULL;
	}
	oggstruct=alogg_create_ogg_from_file(fp);
	if(oggstruct == NULL)
	{
		#ifdef EOF_DEBUG_WAVEFORM
		allegro_message("Waveform: ALOGG failed to open input audio file");
		#endif
		fclose(fp);
		return NULL;
	}

//Decode OGG into memory
	audio=alogg_create_sample_from_ogg(oggstruct);
	if(audio == NULL)
	{
		#ifdef EOF_DEBUG_WAVEFORM
		allegro_message("Waveform: ALOGG failed to decode input audio file");
		#endif
		done=-1;
	}
	else if((audio->bits != 8) && (audio->bits != 16))	//This logic currently only supports 8 and 16 bit audio
	{
		#ifdef EOF_DEBUG_WAVEFORM
		allegro_message("Waveform: Invalid sample size");
		#endif
		done=-1;
	}
	else
	{
//Initialize waveform structure
		waveform=(struct wavestruct *)malloc(sizeof(struct wavestruct));
		if(waveform == NULL)
		{
			#ifdef EOF_DEBUG_WAVEFORM
			allegro_message("Waveform: Unable to allocate memory for the waveform structure");
			#endif
			done=-1;
		}
		else
		{
			*waveform=emptywaveform;					//Set all variables to value zero
			waveform->slicelength = slicelength;
			if(alogg_get_wave_is_stereo_ogg(oggstruct))	//If this audio file has two audio channels
				waveform->is_stereo=1;
			else
				waveform->is_stereo=0;

			if(audio->bits == 8)
				waveform->zeroamp=128;	//128 represents amplitude 0 for unsigned 8 bit audio samples
			else
				waveform->zeroamp=32768;	//32768 represents amplitude 0 for unsigned 16 bit audio samples

			waveform->oggfilename=(char *)malloc(strlen(oggfilename)+1);
			if(waveform->oggfilename == NULL)
			{
				#ifdef EOF_DEBUG_WAVEFORM
				allegro_message("Waveform: Unable to allocate memory for the audio filename string");
				#endif
				done=-1;
			}
			else
			{
				waveform->slicesize=audio->freq * slicelength / 1000;	//Find the number of samples in each slice
				if((audio->freq * slicelength) % 1000)					//If there was any remainder
					waveform->slicesize++;								//Increment the size of the slice

				waveform->numslices=(float)audio->len / ((float)audio->freq * (float)slicelength / 1000.0);	//Find the number of slices to process
				if(audio->len % waveform->numslices)		//If there's any remainder
					waveform->numslices++;					//Increment the number of slices

				strcpy(waveform->oggfilename,oggfilename);
				waveform->left.slices=(struct waveformslice *)malloc(sizeof(struct waveformslice) * waveform->numslices);
				if(waveform->left.slices == NULL)
				{
					#ifdef EOF_DEBUG_WAVEFORM
					allegro_message("Waveform: Unable to allocate memory for the left channel waveform data");
					#endif
					done=-1;
				}
				else if(waveform->is_stereo)	//If this OGG is stereo
				{				//Allocate memory for the right channel waveform data
					waveform->right.slices=(struct waveformslice *)malloc(sizeof(struct waveformslice) * waveform->numslices);
					if(waveform->right.slices == NULL)
					{
						#ifdef EOF_DEBUG_WAVEFORM
						allegro_message("Waveform: Unable to allocate memory for the right channel waveform data");
						#endif
						done=-1;
					}
				}
			}
		}
	}

	while(!done)
	{
		done=eof_process_next_waveform_slice(waveform,audio,slicenum++);
	}

//Cleanup
	if(oggstruct != NULL)
		alogg_destroy_ogg(oggstruct);
	if(audio != NULL)
		destroy_sample(audio);
	if(done == -1)	//Unsuccessful completion
	{
		if(waveform)
		{
			if(waveform->oggfilename)
				free(waveform->oggfilename);
			free(waveform);
		}
		#ifdef EOF_DEBUG_WAVEFORM
		allegro_message("Waveform: Failed to generate waveform");
		#endif
		return NULL;	//Return error
	}

	#ifdef EOF_DEBUG_WAVEFORM
	allegro_message("Waveform: Waveform generation successful");
	#endif

	return waveform;	//Return waveform data
}

int eof_process_next_waveform_slice(struct wavestruct *waveform,SAMPLE *audio,unsigned long slicenum)
{
	unsigned long sampleindex=0;	//The byte index into audio->data
	unsigned long startsample=0;	//The sample number of the first sample being processed
	unsigned long samplesize=0;	//Number of bytes for each sample: 1 for 8 bit audio, 2 for 16 bit audio.  Doubled for stereo
	unsigned long ctr=0;
	double sum;			//Stores the sums of each sample's square (for finding the root mean square)
	double rms;			//Stores the root square mean
	unsigned min;		//Stores the lowest amplitude for the slice
	unsigned peak;		//Stores the highest amplitude for the slice
	unsigned long sample=0;
	char firstread;		//Set to nonzero after the first sample is read into the min/max variables
	char channel=0;
	struct waveformslice *dest;	//The structure to write this slice's data to
	char outofsamples=0;		//Will be set to 1 if all samples in the audio structure have been processed

//Validate parameters
	if((waveform == NULL) || (waveform->left.slices == NULL) || (audio == NULL))
		return -1;	//Return error

	if(waveform->is_stereo && (waveform->right.slices == NULL))
		return -1;	//Return error

	if((slicenum > waveform->numslices))	//If this is more than the number of slices that were supposed to be read
		return 1;	//Return out of samples

	samplesize=audio->bits / 8;
	if(waveform->is_stereo)		//Stereo data is interleaved as left channel, right channel, ...
		samplesize+=samplesize;	//Double the sample size

	for(channel=0;channel<=waveform->is_stereo;channel++)	//Process loop once for mono track, twice for stereo track
	{
//Initialize processing for this audio channel
		sum=rms=min=peak=firstread=0;
		startsample=(float)slicenum * (float)audio->freq * (float)waveform->slicelength / 1000.0;	//This is the sample index for this slices starting sample
		sampleindex=startsample * samplesize;		//This is the byte index for this slice's starting sample number

		if(channel)							//If processing the sample for the right channel
			sampleindex+=(audio->bits / 8);	//Seek past the left channel sample

//Process audio samples for this channel
		for(ctr=0;ctr < waveform->slicesize;ctr++)
		{
			if(startsample + ctr >= audio->len)	//If there are no more samples to read
			{
				outofsamples=1;
				break;
			}

			sample=((unsigned char *)audio->data)[sampleindex];	//Store first sample byte (Allegro documentation states the sample data is stored in unsigned format)
			if(audio->bits > 8)	//If this sample is more than one byte long (16 bit)
				sample+=((unsigned char *)audio->data)[sampleindex+1]<<8;	//Assume little endian byte order, read the next (high byte) of data

			if(!firstread)			//If this is the first sample
			{
				min=peak=sample;	//Assume it is the highest and lowest amplitude until found otherwise
				firstread=1;
			}
			else					//Track the highest and lowest amplitude
			{
				if(sample > peak)
					peak=sample;
				if(sample < min)
					min=sample;
			}

			sum+=((double)sample*sample)/waveform->slicesize;	//Add the square of this sample divided by the number of samples to read
			sampleindex+=samplesize;		//Adjust index to point to next sample for this channel
		}
		rms=sqrt(sum);

//Store results to the appropriate channel's waveform data array
		if(channel == 0)
		{
			dest=&(waveform->left.slices[slicenum]);	//Store results to mono/left channel array
			if(peak > waveform->left.maxamp)
				waveform->left.maxamp=peak;	//Track absolute maximum amplitude of mono/left channel
		}
		else
		{
			dest=&(waveform->right.slices[slicenum]);	//Store results to right channel array
			if(peak > waveform->right.maxamp)
				waveform->right.maxamp=peak;	//Track absolute maximum amplitude of right channel
		}

		dest->min=min;
		dest->peak=peak;
		dest->rms=rms;
	}

	return outofsamples;	//Return success/completed status
}
