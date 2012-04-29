
/*
 *
 * FILE: audio_processor.h
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

#ifndef _AUDIO_PROCESSOR_H_
#define _AUDIO_PROCESSOR_H_


#include <glib.h>
#include <string.h>
#include <time.h>
#include "audio.h"
#include "audio_convert.h"
#include "plugin_inout.h"
#include "common.h"




#define AP_CNG_RANGE   1000
#define AP_GNG_FACTOR  0.05


#define AUDIO_PROCESSOR_ERROR           6001
#define AUDIO_PROCESSOR_OK              6000
#define AUDIO_PROCESSOR_ERROR_NULL_IO   6010
#define AUDIO_PROCESSOR_ERROR_BLOCKSIZE 6020
#define AUDIO_PROCESSOR_ERROR_AQUEUES   6030




struct _NodeAqueue
{
    gint id_aqueue;
    GAsyncQueue *aqueue;
};

typedef struct _NodeAqueue NodeAqueue;




gboolean audio_processor_loop_create (AudioProperties *ap_dsp, AudioProperties *ap_internal, gint blocksize_dsp, gint blocksize_internal, glong block_time, gint *cng, Plugin_InOut *pluginio, GError **error);
gboolean audio_processor_loop_destroy (GError **error);

gboolean audio_processor_loop_create_mic (gint id, GAsyncQueue **queue);
gboolean audio_processor_loop_free_mic (gint id);

gboolean audio_processor_loop_create_spk (gint id, GAsyncQueue **queue);
gboolean audio_processor_loop_free_spk (gint id);

gpointer audio_processor_loop (gpointer data);


#endif


