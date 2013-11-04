#include <alogg.h>
#include "minibpm-1.0/src/minibpm-c.h"

double eof_estimate_bpm(ALOGG_OGG * ogg)
{
	SAMPLE * sp;
	MiniBPMState state;
	float buf[1024] = {0.0};
	int iter = 1;
	int i, spos = 0;
	int samples_left;
	double bpm = 0.0;
	unsigned short * sp_pointer;
	
	sp = alogg_create_sample_from_ogg(ogg);
	if(sp)
	{
		sp_pointer = (unsigned short *)sp->data;
		if(sp->stereo)
		{
			iter = 2;
		}
		samples_left = sp->len / iter;
		state = minibpm_new((float)sp->freq);
		if(state)
		{
			while(samples_left > 0)
			{
				for(i = 0; i < (samples_left > 1024 ? 1024 : samples_left); i++)
				{
					buf[i] = (float)((long)sp_pointer[spos] - 0x8000) / (float)0x8000;
					spos += iter;
				}
				samples_left -= 1024;
				
				/* if we ran out of samples, fill rest of buffer with silence */
				if(samples_left < 0)
				{
					for(i = samples_left + 1024; i < 1024; i++)
					{
						buf[i] = 0.0;
					}
				}
				minibpm_process(state, buf, 1024);
			}
			bpm = minibpm_estimate_tempo(state);
			minibpm_delete(state);
		}
		destroy_sample(sp);
	}
	return bpm;
}

