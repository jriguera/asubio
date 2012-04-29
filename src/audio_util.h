
/*
 *
 * FILE: audio_util.h
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

#ifndef _AUDIO_UTIL_H_
#define _AUDIO_UTIL_H_


#include <glib.h>



#define AUDIO_VAT_DOWN_HYSTERESIS           15
#define AUDIO_VAT_ADJUST_CUTOFF_AMPLITUDE   200




struct _VAD_State
{
    gfloat squelch;
    gfloat average_amplitude;
    gfloat cutoff_amplitude;
    gint count;
    gint count_limit;
    gint adjust_cutoff_amplitude;
    gint noisethreshold;
};

typedef struct _VAD_State VAD_State;



/*

struct _VAD_State
{
    gint rate;           // HVDI_VOX_FAST, HVDI_VOX_MEDIUM, or HVDI_VOX_SLOW
    gint noisethreshold; // The actual threshold used by hvdiVOX
    gint samplecount;    // init to 0; used internally by hvdiVOX
};

typedef struct _VAD_State VAD_State;

*/




gint audio_bitrate_calcule (gint rate, gint channels, gint bits);


VAD_State *audio_vat_create (gint down_hysteresis, gint adjust_cutoff_amplitude, gint noisethreshold);
gboolean audio_vat_destroy (VAD_State *state);
gint audio_vat_set_cutoff_amplitude (VAD_State *state, gpointer buf, gint buf_len, gint buf_steps);
gboolean audio_vad_calcule (VAD_State *state, gpointer buf, gint len);


#endif


