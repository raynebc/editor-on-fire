/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Rubber Band Library
    An audio time-stretching and pitch-shifting library.
    Copyright 2007-2012 Particular Programs Ltd.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.

    Alternatively, if you have a valid commercial licence for the
    Rubber Band Library obtained by agreement with the copyright
    holders, you may redistribute and/or modify it under the terms
    described in that licence.

    If you wish to distribute code using the Rubber Band Library
    under terms other than those of the GNU General Public License,
    you must obtain a valid commercial licence before doing so.
*/

#include "minibpm-c.h"
#include "MiniBpm.h"

struct MiniBPMState_
{
    breakfastquay::MiniBPM *m_s;
};

MiniBPMState minibpm_new(double sampleRate)
{
    MiniBPMState_ *state = new MiniBPMState_();
    state->m_s = new breakfastquay::MiniBPM(sampleRate);
    return state;
}

void minibpm_delete(MiniBPMState state)
{
    delete state->m_s;
    delete state;
}

void minibpm_reset(MiniBPMState state)
{
    state->m_s->reset();
}

void minibpm_set_bpm_range(MiniBPMState state, double min, double max)
{
	state->m_s->setBPMRange(min, max);
}

void minibpm_get_bpm_range(MiniBPMState state, double * min, double * max)
{
	state->m_s->getBPMRange(min, max);
}

void minibpm_set_beats_per_bar(MiniBPMState state, int bpb)
{
	state->m_s->setBeatsPerBar(bpb);
}

int minibpm_get_beats_per_bar(MiniBPMState state)
{
	return state->m_s->getBeatsPerBar();
}

double minibpm_estimate_tempo_of_samples(MiniBPMState state, const float * samples, int nsamples)
{
	return state->m_s->estimateTempoOfSamples(samples, nsamples);
}

void minibpm_process(MiniBPMState state, const float * samples, int nsamples)
{
	state->m_s->process(samples, nsamples);
}

double minibpm_estimate_tempo(MiniBPMState state)
{
	return state->m_s->estimateTempo();
}
