#include <math.h>	//For sqrt()
#include <allegro.h>
#include "utility.h"
#include "spectrogram.h"
#include "song.h"
#include "main.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

struct spectrogramstruct *eof_spectrogram = NULL;	//Stores the spectrogram data
struct spectrogramcolorscalestruct *eof_spectrogram_colorscale=NULL;	//Stores the current colorscale being used
char eof_display_spectrogram = 0;					//Specifies whether the spectrogram display is enabled
char eof_spectrogram_renderlocation = 0;			//Specifies where and how high the graph will render (0 = fretboard area, 1 = editor window)
char eof_spectrogram_renderleftchannel = 1;			//Specifies whether the left channel's graph should render
char eof_spectrogram_renderrightchannel = 0;		//Specifies whether the right channel's graph should render
char eof_spectrogram_colorscheme = 1;				//Specifies the index of the colorscale being used
int eof_spectrogram_windowsize = 1024;				//Specifies the window size for the spectrogram
double eof_half_spectrogram_windowsize = 512.0;		//Caches the value of eof_spectrogram_windowsize / 2
double eof_spectrogram_startfreq = DEFAULT_STARTFREQ;	//For displaying a subset of the frequency range, the lower frequency
double eof_spectrogram_endfreq = DEFAULT_ENDFREQ;		//For displaying a subset of the frequency range, the higher frequency
int eof_spectrogram_userange = 0;					//Specifies whether to display the selected subset
int eof_spectrogram_logplot = 1;					//Specifies whether to graph the y-axis on a log scale
int eof_spectrogram_avgbins = 0;					//Specifies whether to average the bins within a pixel

void eof_destroy_spectrogram(struct spectrogramstruct *ptr)
{
	unsigned long ctr;

 	eof_log("eof_destroy_spectrogram() entered", 1);

	if(ptr)
	{
		if(ptr->oggfilename)
			free(ptr->oggfilename);
		if(ptr->left.slices)
		{
			for(ctr = 0; ctr < ptr->numslices; ctr++)
			{	//For each slice of spectrogram data
				if(ptr->left.slices[ctr].amplist)
					free(ptr->left.slices[ctr].amplist);
			}
			free(ptr->left.slices);
		}
		if(ptr->right.slices)
		{
			for(ctr = 0; ctr < ptr->numslices; ctr++)
			{	//For each slice of spectrogram data
				if(ptr->right.slices[ctr].amplist)
					free(ptr->right.slices[ctr].amplist);
			}
			free(ptr->right.slices);
		}
		if(ptr->px_to_freq.map)
			free(ptr->px_to_freq.map);
		free(ptr);
	}

	//Destroy any colorscale data
	if(eof_spectrogram_colorscale != NULL)
	{
		if(eof_spectrogram_colorscale->colortable != NULL)
		{
			free(eof_spectrogram_colorscale->colortable);
		}
		free(eof_spectrogram_colorscale);
	}
	eof_spectrogram_colorscale = NULL;
}

int eof_render_spectrogram(struct spectrogramstruct *spectrogram)
{
	unsigned long x,startpixel;
	unsigned long ycoord1,ycoord2;	//Stores the Y coordinates of graph 1's and 2's Y axis
	unsigned long height;		//Stores the heigth of the fretboard area
	unsigned long top,bottom;	//Stores the top and bottom coordinates for the area the graph will render to
	char numgraphs;				//Stores the number of channels to render
	unsigned long pos = eof_music_pos / eof_zoom;
	unsigned long curms;

//	eof_log("eof_render_spectrogram() entered", 1);

//validate input
	if(!eof_song_loaded || !spectrogram)
		return 1;	//Return error
	if(!spectrogram->left.slices)
		return 1;	//Return error if the spectrogram graph has no left/mono channel data
	if(spectrogram->is_stereo && !spectrogram->right.slices)
		return 1;	//Return error if the stereo spectrogram graph has no right channel data

//determine how many channels will be graphed
	if(spectrogram->is_stereo)
	{	//Take both channels into account
		numgraphs = eof_spectrogram_renderleftchannel + eof_spectrogram_renderrightchannel;
	}
	else
	{	//Take only the first channel into account
		numgraphs = eof_spectrogram_renderrightchannel;
	}
	if(numgraphs == 0)	//If user specified not to render either channel, or to render the right channel on a mono audio file
		return 0;

//determine timestamp of the left visible edge of the piano roll, which will be in ms, the same as the length of each spectrogram slice
	curms = eof_determine_piano_roll_left_edge();

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
	if(eof_spectrogram_renderlocation == 0)
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
		if(eof_spectrogram_renderleftchannel)
		{	//If only rendering the left channel
			spectrogram->left.height = height;
			spectrogram->left.yaxis = ycoord1;
			spectrogram->left.halfheight = height / 2;
			spectrogram->left.logheight = log(height);
		}
		else
		{	//If only rendering the right channel
			spectrogram->right.height = height;
			spectrogram->right.yaxis = ycoord1;
			spectrogram->right.halfheight = height / 2;
			spectrogram->right.logheight = log(height);
		}
	}
	else if(numgraphs == 2)
	{
		ycoord2 = bottom - (height / 2);	//This graph will take 1/2 the entire graphing area, oriented at the bottom

		spectrogram->left.height = height;
		spectrogram->left.yaxis = ycoord1;
		spectrogram->right.height = height;
		spectrogram->right.yaxis = ycoord2;
		spectrogram->left.halfheight = spectrogram->right.halfheight = height / 2;
		spectrogram->left.logheight = spectrogram->right.logheight = log(height / 2);
	}
	else
	{	//Do not render anything unless it's the graph for 1 or 2 channels
		return 1;
	}

	//Calculate the y axis bins
	eof_spectrogram_calculate_px_to_freq(spectrogram);

//render graph from left to right, one pixel at a time (each pixel represents eof_zoom number of milliseconds of audio)
	//for(x=startpixel;x < eof_window_editor->w;x++)
	for(x=startpixel;x < eof_window_editor->w;x++,curms+=eof_zoom)
	{	//for each pixel in the piano roll's visible width
		if(eof_spectrogram_renderleftchannel)
		{	//If the left channel rendering is enabled
			eof_render_spectrogram_col(spectrogram,&spectrogram->left,spectrogram->left.slices,x,curms);
		}

		if(eof_spectrogram_renderrightchannel)
		{	//If the right channel rendering is enabled
			eof_render_spectrogram_col(spectrogram,&spectrogram->right,spectrogram->right.slices,x,curms);
		}
	}

	return 0;
}

/**
 * Pre-calculate a table that maps pixels on the y-axis to frequency bins
 *
 * This function populates the px_to_freq table in the spectrogram that is
 * used to avoid converting pixels to frequencies repeatedly, which is costly
 * especially in the case of log plots.
 *
 * This function checks itself if the table needs to be recalculated and simply
 * returns if the old version is usable, so it can be called on each render
 *
 * @param spectrogram The spectrogramstruct of the given spectrogram
 */
void eof_spectrogram_calculate_px_to_freq(struct spectrogramstruct *spectrogram)
{
	int height = 0, y;
	double binsize, a, b, yfrac, freq;
	double startfreq = DEFAULT_STARTFREQ;
	double endfreq = DEFAULT_ENDFREQ;

	//Determine the height we should use from the channels
	if(spectrogram->left.height)
	{
		height = spectrogram->left.height;
	}
	if(spectrogram->right.height > height)
	{
		height = spectrogram->right.height;
	}
	if(!height)
		return;

	//Just in case it didn't get marked
	if(height != spectrogram->px_to_freq.height)
	{
		spectrogram->px_to_freq.dirty = 1;
	}

	spectrogram->px_to_freq.height = height;

	if(spectrogram->px_to_freq.dirty == 0)
	{
		//eof_log("Using existing px_to_freq table",1);
		return;
	}

	eof_log("Recreating px_to_freq table",1);
	spectrogram->px_to_freq.dirty = 0;

	if(spectrogram->px_to_freq.map)
		free(spectrogram->px_to_freq.map);

	spectrogram->px_to_freq.map = (unsigned long *)malloc((height+1) * sizeof(unsigned long));
	if(spectrogram->px_to_freq.map == NULL)
	{
		eof_log("Couldn't allocate memory for px_to_freq map!",1);
		return;
	}

	binsize=((double)spectrogram->rate/2.0)/(double)eof_spectrogram_windowsize;

	if(eof_spectrogram_userange)
	{ //Assign custom start/end if we're using it
		startfreq = eof_spectrogram_startfreq;
		endfreq = eof_spectrogram_endfreq;
	}

	a = eof_spectrogram_y_from_freq(spectrogram->rate, startfreq);
	b = eof_spectrogram_y_from_freq(spectrogram->rate, endfreq);
	yfrac = (b-a)/height;

	for(y=0;y <= height;y++)
	{
		freq = eof_spectrogram_freq_from_y(spectrogram->rate, a + (double)y*yfrac);
		spectrogram->px_to_freq.map[y] = (unsigned long)ceil(freq/binsize);
	}
}

/**
 * A safe accessor for the px_to_freq table
 *
 * Ensures the given pixel value is in the range of available values
 * and that the px_to_freq map is populated properly
 *
 * @param spectrogram The spectrogram data structure
 * @param px The pixel value to be looked up
 * @return The frequency bin represented by that pixel
 */
unsigned long spectrogram_get_freq_from_px(struct spectrogramstruct *spectrogram, int px)
{
	if(spectrogram->px_to_freq.map == NULL)
	{
		return 0;
	}

	if(px > 0 && px <= spectrogram->px_to_freq.height)
	{
		return spectrogram->px_to_freq.map[px];
	}

	return 0;
}

/**
 * Helper function to calculate the frequency given a percentage of the y axis
 *
 * Checks for log scale or not and calculates accordingly
 *
 * @param rate The sampling rate of the audio, which determines the max
 * @param y A value from 0 to 1 representing the position on the y axis
 * @return The frequency at that position
 */
double eof_spectrogram_freq_from_y(long rate, double y)
{
	if(eof_spectrogram_logplot)
	{
		return MINFREQ * pow(((double)rate/2.0) / MINFREQ, y);
	}

	return MINFREQ + y * (((double)rate/2.0) - MINFREQ);
}

/**
 * Helper function to calculate the y position for a given frequency
 *
 * Checks for log scale or not and calculates accordingly
 *
 * @param rate The sampling rate of the audio, which determines the max
 * @param freq The frequency being plotted
 * @return The position on the y axis, from 0 to 1, of that frequency
 */
double eof_spectrogram_y_from_freq(long rate, double freq)
{
	double lograte;
	if(eof_spectrogram_logplot)
	{
		lograte = log(((double)rate/2.0) / MINFREQ);	//Cache this to avoid Splint's nag about multiple calls to log() in one line of code, obscuring which one may have returned an error through errno
		return log(freq / MINFREQ) / lograte;
	}

	return (freq - MINFREQ) / (((double)rate/2.0) - MINFREQ);
}

void eof_render_spectrogram_col(struct spectrogramstruct *spectrogram,struct spectrogramchanneldata *channel,struct spectrogramslice *ampdata, unsigned long x, unsigned long curms)
{
	unsigned long yoffset;	//The offset from the y axis coordinate to render the line to
	unsigned long curslice;
	unsigned long actualzero;
	double val;
	unsigned long cursamp;
	unsigned long nextsamp;
	unsigned long sampoffset;

	if(spectrogram != NULL)
	{
		actualzero = channel->yaxis + channel->halfheight;
		curslice = curms / spectrogram->windowlength;
		if(curslice >= spectrogram->numslices)
		{	//Avoid a buffer overread
			return;
		}
		for(yoffset=0;yoffset < channel->height-1;yoffset++)
		{
			//Find the bins for these frequencies
			cursamp = spectrogram_get_freq_from_px(spectrogram, yoffset);
			if(eof_spectrogram_avgbins)
			{
				nextsamp = spectrogram_get_freq_from_px(spectrogram, yoffset+1);
				if(cursamp == nextsamp)
				{
					nextsamp = cursamp + 1;
				}

				//Average the samples to get a gray value
				val = 0.0;
				for(sampoffset = cursamp; sampoffset < nextsamp; sampoffset++)
				{	//Break from loop before a buffer overread can occur
					val += ampdata[curslice].amplist[sampoffset];
				}
				val = val/(double)(nextsamp - cursamp);
			}
			else
			{
				val = ampdata[curslice].amplist[cursamp];
			}

			putpixel(eof_window_editor->screen, x, actualzero - yoffset, eof_color_scale(log(val),spectrogram->log_max,eof_spectrogram_colorscheme));
			//To test a color scale
			//putpixel(eof_window_editor->screen, x, actualzero - yoffset, eof_color_scale(yoffset,channel->height,eof_spectrogram_colorscheme));
		}
	}
}

/**
 * Returns a color for a given value on a scale
 *
 * @param value The value (from 0 to @max) to be represented
 * @param max The maximum value on the scale
 * @param scalenum The index of the scale being used
 * @return An integer representing a RGB color
 */
int eof_color_scale(double value, double max, short int scalenum)
{
	eof_generate_colorscale(scalenum);

	if((eof_spectrogram_colorscale != NULL) && (eof_spectrogram_colorscale->colortable != NULL))
	{
		value = value/max;
		if(value < 0.0)
		{
			value = 0.0;
		}
		if(value > 1.0)
		{
			value = 1.0;
		}

		return eof_spectrogram_colorscale->colortable[(int)(value * eof_spectrogram_colorscale->maxval)];
	}

	return 0;
}

/**
 * Generates the given colorscale
 *
 * This function is where algorithms for the various colorscales are defined
 * It simply has a switch statement with a block for each scale
 * That creates a given colorscale in the colorscale datastructure
 *
 * @param scalenum The index number of the scale to generate
 */
void eof_generate_colorscale(char scalenum)
{
	int rgb[3] = {0,0,0};
	int cnt, scaledval;

	if((eof_spectrogram_colorscale != NULL) && (eof_spectrogram_colorscale->scalenum == scalenum))
		return;

	//Destroy any previous colorscale
	if(eof_spectrogram_colorscale != NULL)
	{
		if(eof_spectrogram_colorscale->colortable != NULL)
		{
			free(eof_spectrogram_colorscale->colortable);
		}
		free(eof_spectrogram_colorscale);
	}

	eof_spectrogram_colorscale = (struct spectrogramcolorscalestruct *)malloc(sizeof(struct spectrogramcolorscalestruct));

	if(eof_spectrogram_colorscale != NULL)
	{
		//Initialize the maximum value of the colorscale
		//Determined by the number of values available
		eof_spectrogram_colorscale->scalenum = scalenum;
		switch(scalenum)
		{
			case 0:
				eof_spectrogram_colorscale->maxval = 255.0;
				break;
			case 1:
				eof_spectrogram_colorscale->maxval = 1280.0;
				break;
			default:	//If scalenum is some unexpected value, use the default value of 1 for it
				eof_spectrogram_colorscale->maxval = 1280.0;
				break;
		}

		eof_spectrogram_colorscale->colortable = (int *)malloc((eof_spectrogram_colorscale->maxval+1) * sizeof(int));

		//Generate the color values
		for(scaledval = 0; scaledval <= eof_spectrogram_colorscale->maxval; scaledval++)
		{ //Loop through, calculating a color for each point on the scale
			switch(scalenum)
			{
				case 0:
					//Grayscale, just R=G=B=value
					for(cnt=0;cnt<3;cnt++)
					{
						rgb[cnt] = scaledval;
					}
				break;
				case 1:
					//More complex heatmap, rainbow (red->orange->green->blue)
					rgb[0] = 384.0 - fabs(scaledval-896.0);
					rgb[1] = 384.0 - fabs(scaledval-640.0);
					rgb[2] = 384.0 - fabs(scaledval-384.0);
					for(cnt=0;cnt<3;cnt++)
					{
						if(rgb[cnt] > 255)
						{
							rgb[cnt] = 255;
						}
						if(rgb[cnt] < 0)
						{
							rgb[cnt] = 0;
						}
					}
					break;
				default:
				break;
			}
			eof_spectrogram_colorscale->colortable[scaledval] = makecol(rgb[0],rgb[1],rgb[2]);
		}
	}
}

void eof_render_spectrogram_line(struct spectrogramstruct *spectrogram,struct spectrogramchanneldata *channel,unsigned amp,unsigned long x,int color)
{
	unsigned long maxampoffset;	//The difference between the zero amplitude and the channel's maximum amplitude
	unsigned long yoffset;	//The offset from the y axis coordinate to render the line to

	if(spectrogram != NULL)
	{
		if(channel->maxamp > spectrogram->zeroamp)
			maxampoffset = channel->maxamp - spectrogram->zeroamp;
		else
			maxampoffset = spectrogram->zeroamp - channel->maxamp;

		if(amp > spectrogram->zeroamp)	//Render positive amplitude
		{	//Transform y to fit between 0 and zeroamp, then scale to fit the graph
			yoffset=(amp - spectrogram->zeroamp) * (channel->height / 2) / maxampoffset;
			vline(eof_window_editor->screen, x, channel->yaxis, channel->yaxis - yoffset, color);
		}
		else
		{	//Correct the negative amplitude, then scale it to fit the graph
			yoffset=(spectrogram->zeroamp - amp) * (channel->height / 2) / maxampoffset;
			vline(eof_window_editor->screen, x, channel->yaxis, channel->yaxis + yoffset, color);
		}
	}
}

#define EOF_DEBUG_SPECTROGRAM
struct spectrogramstruct *eof_create_spectrogram(char *oggfilename)
{
	ALOGG_OGG *oggstruct=NULL;
	SAMPLE *audio=NULL;
	void * oggbuffer = NULL;
	struct spectrogramstruct *spectrogram=NULL;
	static struct spectrogramstruct emptyspectrogram;	//all variables in this auto initialize to value 0
	char done=0;	//-1 on unsuccessful completion, 1 on successful completion
	unsigned long slicenum=0;
	clock_t starttime = 0, endtime = 0;

	fftw_plan fftplan;

	eof_log("\tGenerating spectrogram", 1);
	eof_log("eof_create_spectrogram() entered", 1);
	set_window_title("Generating Spectrogram...");
	starttime = clock();	//Get the start time of the spectrogram creation

	if(oggfilename == NULL)
	{
		#ifdef EOF_DEBUG_SPECTROGRAM
		allegro_message("Spectrogram: Invalid parameters");
		#endif
		return NULL;
	}

//Load OGG file into memory
	oggbuffer = eof_buffer_file(oggfilename, 0);
	if(!oggbuffer)
	{
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "Spectrogram: Failed to open input audio file: %s",strerror(errno));
		eof_log(eof_log_string, 1);
		return NULL;
	}
	oggstruct=alogg_create_ogg_from_buffer(oggbuffer, (int) file_size_ex(oggfilename));
	if(oggstruct == NULL)
	{
		eof_log("Spectrogram: ALOGG failed to open input audio file", 1);
		free(oggbuffer);
		return NULL;
	}

//Decode OGG into memory
	audio=alogg_create_sample_from_ogg(oggstruct);
	if(audio == NULL)
	{
		eof_log("Spectrogram: ALOGG failed to decode input audio file", 1);
		done=-1;
	}
	else if((audio->bits != 8) && (audio->bits != 16))	//This logic currently only supports 8 and 16 bit audio
	{
		eof_log("Spectrogram: Invalid sample size", 1);
		done=-1;
	}
	else
	{
//Initialize spectrogram structure
		spectrogram=(struct spectrogramstruct *)malloc(sizeof(struct spectrogramstruct));
		if(spectrogram == NULL)
		{
			eof_log("Spectrogram: Unable to allocate memory for the spectrogram structure", 1);
			done=-1;
		}
		else
		{
			*spectrogram=emptyspectrogram;					//Set all variables to value zero
			spectrogram->numbuff = 1;
			if(alogg_get_wave_is_stereo_ogg(oggstruct))	//If this audio file has two audio channels
				spectrogram->is_stereo = 1;
			else
				spectrogram->is_stereo = 0;

			if(audio->bits == 8)
				spectrogram->zeroamp = 128;	//128 represents amplitude 0 for unsigned 8 bit audio samples
			else
				spectrogram->zeroamp = 32768;	//32768 represents amplitude 0 for unsigned 16 bit audio samples
			spectrogram->log_max = log(eof_spectrogram_windowsize * spectrogram->zeroamp);	//Cache this value, since it is needed to render each pixel of the spectrogram

			spectrogram->oggfilename = (char *)malloc(strlen(oggfilename)+1);
			if(spectrogram->oggfilename == NULL)
			{
				eof_log("Spectrogram: Unable to allocate memory for the audio filename string", 1);
				done=-1;
			}
			else
			{
				spectrogram->rate = audio->freq;
				spectrogram->windowlength = (double)eof_spectrogram_windowsize / (double)audio->freq * 1000.0;

				spectrogram->numslices=(double)audio->len / ((double)audio->freq * spectrogram->windowlength / 1000.0);	//Find the number of slices to process
				if(audio->len % spectrogram->numslices)		//If there's any remainder
					spectrogram->numslices++;					//Increment the number of slices

				strcpy(spectrogram->oggfilename,oggfilename);
				spectrogram->left.slices=(struct spectrogramslice *)malloc(sizeof(struct spectrogramslice) * spectrogram->numslices);
				if(spectrogram->left.slices == NULL)
				{
					eof_log("Spectrogram: Unable to allocate memory for the left channel spectrogram data", 1);
					done=-1;
				}
				else if(spectrogram->is_stereo)	//If this OGG is stereo
				{				//Allocate memory for the right channel spectrogram data
					spectrogram->right.slices=(struct spectrogramslice *)malloc(sizeof(struct spectrogramslice) * spectrogram->numslices);
					if(spectrogram->right.slices == NULL)
					{
						eof_log("Spectrogram: Unable to allocate memory for the right channel spectrogram data", 1);
						done=-1;
					}
				}
			}
		}
	}

	//Allocate memory for the buffer pointers
	if((done != -1) && spectrogram)
	{	//If there wasn't an error yet
		clock_t starttime2 = 0, endtime2 = 0;

		spectrogram->buffin=(double *) fftw_malloc(sizeof(double) * eof_spectrogram_windowsize*spectrogram->numbuff);
		spectrogram->buffout=(double *) fftw_malloc(sizeof(double) * eof_spectrogram_windowsize*spectrogram->numbuff);

		starttime2 = clock();
		fftplan=fftw_plan_r2r_1d(eof_spectrogram_windowsize,spectrogram->buffin,spectrogram->buffout,FFTW_R2HC,FFTW_DESTROY_INPUT | FFTW_EXHAUSTIVE);
		endtime2 = clock();
		(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tFFTW plan generated in %f seconds.", (double)(endtime2 - starttime2) / (double)CLOCKS_PER_SEC);
		eof_log(eof_log_string, 1);

		eof_log("\tGenerating slices...", 1);
		while(!done)
		{
			done=eof_process_next_spectrogram_slice(spectrogram,audio,slicenum++,fftplan);
		}
	}

//Cleanup
	if(oggstruct != NULL)
		alogg_destroy_ogg(oggstruct);
	if(audio != NULL)
	if(oggbuffer)
		free(oggbuffer);
	if(done == -1)	//Unsuccessful completion
	{
		if(spectrogram)
		{
			if(spectrogram->oggfilename)
				free(spectrogram->oggfilename);
			free(spectrogram);
		}
		eof_log("Spectrogram: Failed to generate spectrogram", 1);
		return NULL;	//Return error
	}

	endtime = clock();	//Get the start time of the spectrogram creation
	(void) snprintf(eof_log_string, sizeof(eof_log_string) - 1, "\tSpectrogram generated in %f seconds", (double)(endtime - starttime) / (double)CLOCKS_PER_SEC);
	eof_log(eof_log_string, 1);

	return spectrogram;	//Return spectrogram data
}

int eof_process_next_spectrogram_slice(struct spectrogramstruct *spectrogram,SAMPLE *audio,unsigned long slicenum,fftw_plan fftplan)
{
	unsigned long sampleindex=0;	//The byte index into audio->data
	unsigned long startsample=0;	//The sample number of the first sample being processed
	unsigned long samplesize=0;	//Number of bytes for each sample: 1 for 8 bit audio, 2 for 16 bit audio.  Doubled for stereo
	unsigned long cursamp=0;
	unsigned long halfsize;
	long sample=0;
	char channel=0;
	struct spectrogramslice *dest;	//The structure to write this slice's data to
	char outofsamples=0;		//Will be set to 1 if all samples in the audio structure have been processed

//Validate parameters
	if((spectrogram == NULL) || (spectrogram->left.slices == NULL) || (audio == NULL))
	{
		eof_log("\tNo sound found", 1);
		return -1;	//Return error
	}

	if(spectrogram->is_stereo && (spectrogram->right.slices == NULL))
	{
		eof_log("\tStereo channel not found", 1);
		return -1;	//Return error
	}

	if((slicenum > spectrogram->numslices))
	{	//If this is more than the number of slices that were supposed to be read
		eof_log("\tSlicenum is too far", 1);
		return 1;	//Return out of samples
	}

	samplesize=audio->bits / 8;
	if(spectrogram->is_stereo)		//Stereo data is interleaved as left channel, right channel, ...
		samplesize+=samplesize;	//Double the sample size

	for(channel=0;channel<=spectrogram->is_stereo;channel++)	//Process loop once for mono track, twice for stereo track
	{
//Initialize processing for this audio channel
		startsample=slicenum * eof_spectrogram_windowsize; //This is the sample index for this slices starting sample
		sampleindex=startsample * samplesize;		//This is the byte index for this slice's starting sample number

		if(channel)							//If processing the sample for the right channel
			sampleindex+=(audio->bits / 8);	//Seek past the left channel sample

//Process audio samples for this channel
		for(cursamp=0;cursamp < eof_spectrogram_windowsize;cursamp++)
		{
			if(startsample + cursamp >= audio->len)	//If there are no more samples to read
			{
				outofsamples=1;
				//Zero-pad the remaining buffer...
				memset(spectrogram->buffin+cursamp,0,sizeof(double) * (eof_spectrogram_windowsize - cursamp));
				break;
			}

			sample=((unsigned char *)audio->data)[sampleindex];	//Store first sample byte (Allegro documentation states the sample data is stored in unsigned format)
			if(audio->bits > 8)	//If this sample is more than one byte long (16 bit)
				sample+=((unsigned char *)audio->data)[sampleindex+1]<<8;	//Assume little endian byte order, read the next (high byte) of data

			sample -= spectrogram->zeroamp;
			spectrogram->buffin[cursamp] = (double)sample;

			sampleindex+=samplesize;		//Adjust index to point to next sample for this channel
		}
		fftw_execute(fftplan);
		if(channel == 0)
		{
			dest=&spectrogram->left.slices[slicenum];
		}
		else
		{
			dest=&(spectrogram->right.slices[slicenum]);	//Store results to right channel array
		}
		halfsize = eof_spectrogram_windowsize / 2;
		dest->amplist=(double*)malloc(sizeof(double) * (halfsize + 1));
		dest->amplist[0] = spectrogram->buffout[0];			//The first one is all real
		for(cursamp=halfsize - 1; cursamp > 0; cursamp--)
		{
			dest->amplist[cursamp] = sqrt(
				spectrogram->buffout[cursamp] * spectrogram->buffout[cursamp] +
				spectrogram->buffout[eof_spectrogram_windowsize - cursamp] *
				spectrogram->buffout[eof_spectrogram_windowsize - cursamp]
			);
			if(dest->amplist[cursamp] > spectrogram->destmax)
			{
				spectrogram->destmax = dest->amplist[cursamp];
			}
		}
		dest->amplist[halfsize] = spectrogram->buffout[halfsize];
	}

	return outofsamples;	//Return success/completed status
}
