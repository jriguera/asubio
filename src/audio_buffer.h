
/*
 *
 * FILE: audio_buffer.h
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
 *
 ***************************************************************************/

#ifndef _AUDIO_BUFFER_H_
#define _AUDIO_BUFFER_H_

#include <glib.h>
#include "audio.h"
#include "plugin_effect.h"
#include "common.h"



#define AUDIO_BUFFER_ERROR         1
#define AUDIO_BUFFER_ERROR_ERRNO   2
#define AUDIO_BUFFER_BLKSIZE       3
#define AUDIO_BUFFER_OK            4




struct _AudioBuffer
{
    guchar *buffer;
    gint ptr_write_buffer;
    gint ptr_read_buffer;
    gint blocksize_write;
    gint blocksize_read;
    gint buffer_length;
    AudioProperties *pp;
};

typedef struct _AudioBuffer AudioBuffer;




struct _PluginEffectNode
{
    Plugin_Effect *plugin;
    GModule *module;
    gint id;
    gboolean active;
    gpointer state;
};

typedef struct _PluginEffectNode PluginEffectNode;



struct _ABuffer
{
    GSList *list_plugins;
    GAsyncQueue *aqueue;
    AudioBuffer *audio;
};

typedef struct _ABuffer ABuffer;




ABuffer *init_read_audio_buffer (gpointer audio_aqueue, gint audio_blksize_in, gint audio_blksize_out, AudioProperties *apts, GSList *plugins, GError **error);
gint free_read_audio_buffer (ABuffer *ab);
gint read_audio_buffer (ABuffer *ab, gint *mute, gpointer audio_data);

ABuffer *init_write_audio_buffer (gpointer audio_aqueue, gint audio_blksize_in, gint audio_blksize_out, AudioProperties *apts, GSList *plugins, GError **error);
gint free_write_audio_buffer (ABuffer *ab);
gint write_audio_buffer (ABuffer *ab, gfloat *f, gint *mute, gpointer audio_data);


inline gint mcm (gint a, gint b);


#endif


