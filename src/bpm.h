#ifndef EOF_BPM_H
#define EOF_BPM_H

#include <alogg.h>

double eof_estimate_bpm(ALOGG_OGG * ogg, double startpoint, double endpoint);
	//Uses MiniBPM to estimate the tempo of some/all of the specified audio
	//If startpoint is less than endpoint (each is a timestamp measured in seconds), that time range the entire audio file is analyzed
	// otherwise the entire audio file is analyzed

extern unsigned long eof_bpm_estimator_sample_count;	//Used to track how many samples were processed by MiniBPM

#endif
