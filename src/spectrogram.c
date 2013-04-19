#include <math.h>	//For sqrt()
#include <allegro.h>
#include "utility.h"
#include "spectrogram.h"
#include "song.h"
#include "main.h"

#ifdef USEMEMWATCH
#include "memwatch.h"
#endif

struct spectrogramstruct *eof_spectrogram=NULL;	//Stores the spectrogram data
char eof_display_spectrogram=0;			//Specifies whether the spectrogram display is enabled
char eof_spectrogram_renderlocation=0;		//Specifies where and how high the graph will render (0 = fretboard area, 1 = editor window)
char eof_spectrogram_renderleftchannel=1;	//Specifies whether the left channel's graph should render
char eof_spectrogram_renderrightchannel=0;	//Specifies whether the right channel's graph should render
char eof_spectrogram_colorscheme = 1;		//Specifies the color scheme to use for the graph
int eof_spectrogram_windowsize = 1024;	//Specifies the window size to use

void eof_destroy_spectrogram(struct spectrogramstruct *ptr)
{
 	eof_log("eof_destroy_spectrogram() entered", 1);

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

int eof_render_spectrogram(struct spectrogramstruct *spectrogram)
{
// 	eof_log("eof_render_spectrogram() entered");

	unsigned long x,startpixel;
	unsigned long ycoord1,ycoord2;	//Stores the Y coordinates of graph 1's and 2's Y axis
	unsigned long height;		//Stores the heigth of the fretboard area
	unsigned long top,bottom;	//Stores the top and bottom coordinates for the area the graph will render to
	char numgraphs;		//Stores the number of channels to render
	unsigned long pos = eof_music_pos / eof_zoom;
    unsigned long curms;

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
		}
		else
		{	//If only rendering the right channel
			spectrogram->right.height = height;
			spectrogram->right.yaxis = ycoord1;
		}
	}
	else if(numgraphs == 2)
	{
		ycoord2 = bottom - (height / 2);	//This graph will take 1/2 the entire graphing area, oriented at the bottom

		spectrogram->left.height = height;
		spectrogram->left.yaxis = ycoord1;
		spectrogram->right.height = height;
		spectrogram->right.yaxis = ycoord2;
	}
	else
	{	//Do not render anything unless it's the graph for 1 or 2 channels
		return 1;
	}

//render graph from left to right, one pixel at a time (each pixel represents eof_zoom number of milliseconds of audio)
	//for(x=startpixel;x < eof_window_editor->w;x++)
	for(x=startpixel;x < eof_window_editor->w;x++,curms+=eof_zoom)
	{	//for each pixel in the piano roll's visible width
        if(eof_spectrogram_renderleftchannel)
        {	//If the left channel rendering is enabled
            eof_render_spectrogram_col(spectrogram,&spectrogram->left,spectrogram->left.slices,x,curms);	//Render the peak amplitude in green
        }

        if(eof_spectrogram_renderrightchannel)
        {	//If the right channel rendering is enabled
            eof_render_spectrogram_col(spectrogram,&spectrogram->right,spectrogram->right.slices,x,curms);	//Render the peak amplitude in green
        }
	}

	return 0;
}

void eof_render_spectrogram_col(struct spectrogramstruct *spectrogram,struct spectrogramchanneldata *channel,struct spectrogramslice *ampdata, unsigned long x, unsigned long curms)
{
	unsigned long yoffset;	//The offset from the y axis coordinate to render the line to
    double sampinterval;
    unsigned long curslice;
    unsigned long actualzero;
    unsigned long sampoffset;
    double avg;
    double curpoint;
    double nextpoint;
    unsigned long cursamp;
    unsigned long nextsamp;
    double logheight;
    double maxval;

    actualzero = channel->yaxis + channel->height/2;
    curslice = curms / spectrogram->windowlength;

    sampinterval=(double)eof_spectrogram_windowsize/2.0;
    maxval = eof_spectrogram_windowsize * spectrogram->zeroamp;

	if(spectrogram != NULL)
	{
        logheight = log(channel->height); 
        for(yoffset=0;yoffset < channel->height-1;yoffset++) 
        {
            if(1) {
                curpoint = 1.0 - log(channel->height-yoffset)/logheight;
                nextpoint = 1.0 - log(channel->height-yoffset-1)/logheight;
            } else {
                curpoint = (double)yoffset / (double)channel->height;
                nextpoint = (double)(yoffset+1) / (double)channel->height;
            }
            cursamp = (unsigned long)(sampinterval * curpoint); 
            nextsamp = (unsigned long)(sampinterval * nextpoint); 
            if(cursamp == nextsamp) { nextsamp = cursamp + 1; }

            //Average the samples to get a gray value
            avg = 0.0;
            for(sampoffset = cursamp; sampoffset < nextsamp; sampoffset++) {
                avg += ampdata[curslice].amplist[sampoffset];
            }
            avg = avg/(double)(nextsamp - cursamp);

            putpixel(eof_window_editor->screen, x, actualzero - yoffset, eof_color_scale(log(avg),log(maxval),eof_spectrogram_colorscheme));
            //To test a color scale
            //putpixel(eof_window_editor->screen, x, actualzero - yoffset, eof_color_scale(yoffset,channel->height,eof_spectrogram_colorscheme));
        }
	}
}

int eof_color_scale(double value, double max, short int scalenum) {
    int cnt;
    int rgb[3] = {0,0,0};
    int scaledval;
    value = value/max;
    if(value < 0.0) { value = 0.0; }
    if(value > 1.0) { value = 1.0; }
    switch(scalenum) {
        case 0:
            for(cnt=0;cnt<3;cnt++) { rgb[cnt] = value * 255.0; }
            break;
        case 1:
            scaledval = value * 1280.0;
            rgb[0] = 384.0 - abs(scaledval-896.0);
            rgb[1] = 384.0 - abs(scaledval-640.0);
            rgb[2] = 384.0 - abs(scaledval-384.0);  
            for(cnt=0;cnt<3;cnt++) { 
                if(rgb[cnt] > 255) { rgb[cnt] = 255; } 
                if(rgb[cnt] < 0) { rgb[cnt] = 0; }
            }
            break;
    }
    return makecol(rgb[0],rgb[1],rgb[2]);
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
 	eof_log("\tGenerating spectrogram", 1);
 	eof_log("eof_create_spectrogram() entered", 1);

	ALOGG_OGG *oggstruct=NULL;
	SAMPLE *audio=NULL;
	void * oggbuffer = NULL;
	struct spectrogramstruct *spectrogram=NULL;
	static struct spectrogramstruct emptyspectrogram;	//all variables in this auto initialize to value 0
	char done=0;	//-1 on unsuccessful completion, 1 on successful completion
	unsigned long slicenum=0;

    fftw_plan fftplan;
    
	set_window_title("Generating Spectrogram...");

	if((oggfilename == NULL))
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
		#ifdef EOF_DEBUG_SPECTROGRAM
		allegro_message("Spectrogram: Failed to open input audio file: %s",strerror(errno));
		#endif
		return NULL;
	}
	oggstruct=alogg_create_ogg_from_buffer(oggbuffer, file_size_ex(oggfilename));
	if(oggstruct == NULL)
	{
		#ifdef EOF_DEBUG_SPECTROGRAM
		allegro_message("Spectrogram: ALOGG failed to open input audio file");
		#endif
		free(oggbuffer);
		return NULL;
	}

//Decode OGG into memory
	audio=alogg_create_sample_from_ogg(oggstruct);
	if(audio == NULL)
	{
		#ifdef EOF_DEBUG_SPECTROGRAM
		allegro_message("Spectrogram: ALOGG failed to decode input audio file");
		#endif
		done=-1;
	}
	else if((audio->bits != 8) && (audio->bits != 16))	//This logic currently only supports 8 and 16 bit audio
	{
		#ifdef EOF_DEBUG_SPECTROGRAM
		allegro_message("Spectrogram: Invalid sample size");
		#endif
		done=-1;
	}
	else
	{
//Initialize spectrogram structure
		spectrogram=(struct spectrogramstruct *)malloc(sizeof(struct spectrogramstruct));
		if(spectrogram == NULL)
		{
			#ifdef EOF_DEBUG_SPECTROGRAM
			allegro_message("Spectrogram: Unable to allocate memory for the spectrogram structure");
			#endif
			done=-1;
		}
		else
		{
			*spectrogram=emptyspectrogram;					//Set all variables to value zero
            spectrogram->numbuff = 1;
			if(alogg_get_wave_is_stereo_ogg(oggstruct))	//If this audio file has two audio channels
				spectrogram->is_stereo=1;
			else
				spectrogram->is_stereo=0;

			if(audio->bits == 8)
				spectrogram->zeroamp=128;	//128 represents amplitude 0 for unsigned 8 bit audio samples
			else
				spectrogram->zeroamp=32768;	//32768 represents amplitude 0 for unsigned 16 bit audio samples

			spectrogram->oggfilename=(char *)malloc(strlen(oggfilename)+1);
			if(spectrogram->oggfilename == NULL)
			{
				#ifdef EOF_DEBUG_SPECTROGRAM
				allegro_message("Spectrogram: Unable to allocate memory for the audio filename string");
				#endif
				done=-1;
			}
			else
			{
                spectrogram->windowlength = (float)eof_spectrogram_windowsize / (float)audio->freq * 1000.0;

				spectrogram->numslices=(float)audio->len / ((float)audio->freq * spectrogram->windowlength / 1000.0);	//Find the number of slices to process
				if(audio->len % spectrogram->numslices)		//If there's any remainder
					spectrogram->numslices++;					//Increment the number of slices

				strcpy(spectrogram->oggfilename,oggfilename);
				spectrogram->left.slices=(struct spectrogramslice *)malloc(sizeof(struct spectrogramslice) * spectrogram->numslices);
				if(spectrogram->left.slices == NULL)
				{
					#ifdef EOF_DEBUG_SPECTROGRAM
					allegro_message("Spectrogram: Unable to allocate memory for the left channel spectrogram data");
					#endif
					done=-1;
				}
				else if(spectrogram->is_stereo)	//If this OGG is stereo
				{				//Allocate memory for the right channel spectrogram data
					spectrogram->right.slices=(struct spectrogramslice *)malloc(sizeof(struct spectrogramslice) * spectrogram->numslices);
					if(spectrogram->right.slices == NULL)
					{
						#ifdef EOF_DEBUG_SPECTROGRAM
						allegro_message("Spectrogram: Unable to allocate memory for the right channel spectrogram data");
						#endif
						done=-1;
					}
				}
			}
		}
	}

    //Allocate memory for the buffer pointers
    spectrogram->buffin=(double *) fftw_malloc(sizeof(double) * eof_spectrogram_windowsize*spectrogram->numbuff);
    spectrogram->buffout=(double *) fftw_malloc(sizeof(double) * eof_spectrogram_windowsize*spectrogram->numbuff);
    fftplan=fftw_plan_r2r_1d(eof_spectrogram_windowsize,spectrogram->buffin,spectrogram->buffout,FFTW_R2HC,FFTW_DESTROY_INPUT);

    eof_log("\tPlan generated.", 1);

    eof_log("\tGenerating slices...", 1);
	while(!done)
	{
		done=eof_process_next_spectrogram_slice(spectrogram,audio,slicenum++,fftplan);
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
		#ifdef EOF_DEBUG_SPECTROGRAM
		allegro_message("Spectrogram: Failed to generate spectrogram");
		#endif
		return NULL;	//Return error
	}

	eof_log("\tSpectrogram generated", 1);

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
	if((spectrogram == NULL) || (spectrogram->left.slices == NULL) || (audio == NULL)) {
        eof_log("\tNo sound found", 1);
		return -1;	//Return error
    }

	if(spectrogram->is_stereo && (spectrogram->right.slices == NULL)) {
        eof_log("\tStereo channel not found", 1);
		return -1;	//Return error
    }

	if((slicenum > spectrogram->numslices)) {	//If this is more than the number of slices that were supposed to be read
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
        halfsize = eof_spectrogram_windowsize/2;
        dest->amplist=(double*)malloc(sizeof(double) * (halfsize+1));
        dest->amplist[0] = spectrogram->buffout[0];    //The first one is all real
        for(cursamp=halfsize-1;cursamp > 0;cursamp--) {
            dest->amplist[cursamp] = sqrt(
                spectrogram->buffout[cursamp] * spectrogram->buffout[cursamp] +
                spectrogram->buffout[eof_spectrogram_windowsize - cursamp] *
                spectrogram->buffout[eof_spectrogram_windowsize - cursamp]
            );
            if(dest->amplist[cursamp] > spectrogram->destmax) { spectrogram->destmax = dest->amplist[cursamp]; }
        }
        dest->amplist[halfsize] = spectrogram->buffout[halfsize];
	}

	return outofsamples;	//Return success/completed status
}
