#ifndef _MINIBPM_C_API_H_
#define _MINIBPM_C_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#define MINIBPM_VERSION "1.0"

/**
 * This is a C-linkage interface to MiniBPM.
 * 
 * This is a wrapper interface: the primary interface is in C++ and is
 * defined and documented in MiniBpm.h.  The library
 * itself is implemented in C++, and requires C++ standard library
 * support even when using the C-linkage API.
 *
 * Please see MiniBpm.h for documentation.
 *
 * If you are writing to the C++ API, do not include this header.
 */

struct MiniBPMState_;
typedef struct MiniBPMState_ *MiniBPMState;

extern MiniBPMState minibpm_new(float sampleRate);

extern void minibpm_delete(MiniBPMState);

extern void minibpm_reset(MiniBPMState);

extern void minibpm_set_bpm_range(MiniBPMState, double min, double max);
extern void minibpm_get_bpm_range(MiniBPMState, double * min, double * max);

extern void minibpm_set_beats_per_bar(MiniBPMState, int bpb);
extern int minibpm_get_beats_per_bar(MiniBPMState);

extern double minibpm_estimate_tempo_of_samples(MiniBPMState, const float * samples, int nsamples);

extern void minibpm_process(MiniBPMState, const float * samples, int nsamples);

extern double minibpm_estimate_tempo(MiniBPMState);

extern void minibpm_reset(MiniBPMState);

#ifdef __cplusplus
}
#endif

#endif
