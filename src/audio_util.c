
/*
 *
 * FILE: audio_util.c
 *
 * Copyright: (c) Junio 2003, Jose Riguera <jriguera@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 ***************************************************************************/

#include "audio_util.h"
#include <math.h>




gint audio_bitrate_calcule (gint rate, gint channels, gint bits)
{
    gint bitrate;


    bitrate = rate * channels;
    switch (bits)
    {
        case 8:
	    break;
        case 16:
	    bitrate *= 2;
	    break;
        case 24:
        case 32:
	    bitrate *= 4;
    }
    return bitrate;
}



/*

 //Basic VOX algorithm adapted from SpeakFreely
 //Returns 1 if buffer passes VOX, and 0 if it does not


VAD_State *audio_vat_create (gint voxspeed, gint noisethreshold)
{
    VAD_State *vox;

    vox = g_malloc(sizeof(VAD_State));
    if(vox == NULL) return NULL;
    vox->rate = voxspeed;
    vox->noisethreshold = (gint) exp(log(32767.0) * ((1000 - noisethreshold) / 1000.0));
    vox->samplecount = 0;
    return vox;
}



gboolean audio_vad_calcule (VAD_State *vox, gpointer buffer, gint buflen)
{
    gint     i;
    long    level = 0;

    for (i=0; i<buflen; i++)
    {
	long sample = buffer[i];

	if(sample < 0) level -= sample;
        else level += sample;
    }
    level /= buflen;
    if(level < vox->noisethreshold)
    {
	if (vox->samplecount <= 0) return FALSE;
        vox->samplecount -= buflen;
    }
    else
    {
        vox->samplecount = vox->rate;
    }
    return TRUE;
}


gboolean audio_vat_destroy (VAD_State *state)
{
    state
    free(vox);
}

*/



VAD_State *audio_vat_create (gint down_hysteresis, gint adjust_cutoff_amplitude, gint noisethreshold)
{
    VAD_State *state;


    state = (VAD_State *) g_malloc(sizeof(VAD_State));
    if (state == NULL) return NULL;

    state->squelch = 0.0;
    state->average_amplitude = 0.0;
    state->cutoff_amplitude = 0.0;
    if (noisethreshold > 0)
    {
	state->noisethreshold = (gint) exp(log(32767.0) * ((1000 - noisethreshold) / 1000.0));
    }
    state->count = 0;

    if (down_hysteresis == -1) down_hysteresis = AUDIO_VAT_DOWN_HYSTERESIS;
    if (adjust_cutoff_amplitude == -1) adjust_cutoff_amplitude = AUDIO_VAT_ADJUST_CUTOFF_AMPLITUDE ;

    state->count_limit = down_hysteresis;
    state->adjust_cutoff_amplitude = adjust_cutoff_amplitude;

    return state;
}



gboolean audio_vat_destroy (VAD_State *state)
{
    g_free(state);
    return TRUE;
}



gint audio_vat_set_cutoff_amplitude (VAD_State *state, gpointer buf, gint buf_len, gint buf_steps)
{
    gint steps = 0;
    gint total_average_amplitude = 0;
    gint fragment;
    gint index;
    guchar *buffer = buf;


    /* Hold on..! Dont start talking.                                        */
    /* Calculating the threshold value for the default audio buffer...       */

    fragment = buf_len / buf_steps;
    if (fragment < 2) return -1;
    index = 0;

    while (steps < buf_steps)
    {
	if (audio_vad_calcule(state, (buffer + index), fragment))
	{
	    total_average_amplitude += state->average_amplitude;
	}
	steps++;
        index += fragment;
    }
    state->cutoff_amplitude = (gfloat) total_average_amplitude / steps;
    state->cutoff_amplitude += state->adjust_cutoff_amplitude;
    state->noisethreshold = (gint) exp(log(32767.0) * ((1000 - state->cutoff_amplitude) / 1000.0));

    return state->cutoff_amplitude;
}



gboolean audio_vad_calcule (VAD_State *state, gpointer buf, gint len)
{
    gint i;
    gint total_amplitude = 0;
    gint16 aux;
    gint16 *buffer = (gint16 *) buf;


    //adaptive squelch value.
    //state->squelch = 0.963 * state->squelch + 0.03 * state->average_amplitude + 1;
    //state->squelch = 0.963 * state->squelch + 0.03 * state->average_amplitude + 1;
    for (i=0; i<(len/2); i++)
    {
	aux = (gint16) *(buffer + i);
	total_amplitude += ABS(aux);
    }

    state->average_amplitude = (gfloat) total_amplitude/(len/2);
    //state->average_amplitude = (gfloat) total_amplitude/len;

    //if (state->average_amplitude < state->squelch || state->average_amplitude < state->cutoff_amplitude)
    if (state->average_amplitude < state->cutoff_amplitude)
    {
	state->count++;
    }
    //if (state->average_amplitude >= state->squelch && state->average_amplitude > state->cutoff_amplitude)
    if (state->average_amplitude > state->cutoff_amplitude)
    {
	state->count--;
	if (state->count < 0) state->count = 0;
    }

    //ditch the second consecutive silence buffer.
    if (state->count == state->count_limit)
    {
	state->count--;
	return FALSE;
    }
    return TRUE;
}


