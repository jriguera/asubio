
/*
 *
 * FILE: audio_buffer_out.c
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

#include "audio_buffer.h"




ABuffer *init_write_audio_buffer (gpointer audio_aqueue, gint audio_blksize_in, gint audio_blksize_out, AudioProperties *apts, GSList *plugins, GError **error)
{
    ABuffer *ab;


    if (audio_aqueue == NULL) return NULL;
    if ((audio_blksize_in > audio_blksize_out) || (0 > audio_blksize_in) || (0 > audio_blksize_out) || ((audio_blksize_out%2) != 0))
    {
	g_set_error(error, AUDIO_BUFFER_ERROR, AUDIO_BUFFER_BLKSIZE,
		    "audio : blocksize in > blocksize out ( %d > %d > 0 y %d debe ser par)",
		    audio_blksize_out, audio_blksize_in, audio_blksize_out);
	g_printerr("[AUDIO BUFFER] init_write_audio_buffer: %s.\n", (*error)->message);
	return NULL;
    }
    ab = (ABuffer *) g_malloc(sizeof(ABuffer));
    ab->audio = (AudioBuffer *) g_malloc(sizeof(AudioBuffer));
    ab->aqueue = (GAsyncQueue *) audio_aqueue;
    ab->audio->buffer_length = mcm(audio_blksize_in, audio_blksize_out);
    ab->audio->buffer = (guchar *) g_malloc(ab->audio->buffer_length * sizeof(guchar));
    ab->audio->ptr_write_buffer = 0;
    ab->audio->ptr_read_buffer = 0;
    ab->audio->blocksize_write = audio_blksize_in;
    ab->audio->blocksize_read = audio_blksize_out;
    ab->audio->pp = (AudioProperties *) g_malloc(sizeof(AudioProperties));
    ab->audio->pp->channels = apts->channels;
    ab->audio->pp->rate = apts->rate;
    ab->audio->pp->format = apts->format;
    ab->list_plugins = plugins;

    if ((ab == NULL) || (ab->audio == NULL) || (ab->audio->buffer == NULL) || (ab->audio->pp == NULL))
    {
	g_set_error(error, AUDIO_BUFFER_ERROR, AUDIO_BUFFER_ERROR_ERRNO, g_strerror(ENOMEM));
	g_printerr("[AUDIO BUFFER] init_write_audio_buffer: no hay memoria suficiente\n");
	return NULL;
    }
    return ab;
}



gint free_write_audio_buffer (ABuffer *ab)
{
    gpointer p;


    while ((p = g_async_queue_try_pop(ab->aqueue)) != NULL)
    {
	g_free(p);
    }

    g_free(ab->audio->pp);
    g_free(ab->audio->buffer);
    g_free(ab->audio);
    g_free(ab);
    return AUDIO_BUFFER_OK;
}



gint write_audio_buffer (ABuffer *ab, gfloat *f, gint *mute, gpointer audio_data)
{
    gpointer c;
    gint16 *j;
    gfloat tmp;
    gint b;
    gint counter;
    gfloat factor;
    PluginEffectNode *pn;


    //if (f == NULL) factor = 1.0;
    //else factor = *f;
    factor = *f;

    memcpy((ab->audio->buffer + ab->audio->ptr_write_buffer), audio_data, ab->audio->blocksize_write);
    ab->audio->ptr_write_buffer += ab->audio->blocksize_write;

    if ((ab->audio->ptr_write_buffer - ab->audio->ptr_read_buffer) >= ab->audio->blocksize_read)
    {
	// Volume
	if (factor > 0)
	{
	    j = (gint16 *) (ab->audio->buffer + ab->audio->ptr_read_buffer);
	    for (counter=0; counter<(ab->audio->blocksize_read/2); counter++)
	    {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN

		tmp = *j * factor;
		if (tmp > 32767) tmp = 32767;
		if (tmp < -32767) tmp = -32767;

		if (tmp >= 0) *j = (gint16) (tmp + 0.5);
		else *j = (gint16) (tmp - 0.5);

#elif G_BYTE_ORDER == G_BIG_ENDIAN

                tmp = GINT16_TO_BE(*j);
		tmp = tmp * factor;
		if (tmp > 32767) tmp = 32767;
		if (tmp < -32767) tmp = -32767;

		if (tmp >= 0) *j = GINT16_TO_LE((gint16) (tmp + 0.5));
		else *j = GINT16_TO_LE((gint16) (tmp - 0.5));
#else
#error "G_BYTE_ORDER should be big or little endian."
#endif
		j++;
	    }
	}
	// plugins
	counter = 0;
	pn = (PluginEffectNode *) g_slist_nth_data(ab->list_plugins, counter);
	while (pn)
	{
	    if (pn->active)
	    {
                b = ab->audio->blocksize_read;
		pn->plugin->pefunction(pn->state, (ab->audio->buffer + ab->audio->ptr_read_buffer), &b, AUDIO_CALL_IN);
	    }
	    counter++;
	    pn = (PluginEffectNode *) g_slist_nth_data(ab->list_plugins, counter);
	}

	if (*mute != 1)
	{
	    c = g_malloc(ab->audio->blocksize_read);
	    memcpy(c, (ab->audio->buffer + ab->audio->ptr_read_buffer), ab->audio->blocksize_read);
	    g_async_queue_push(ab->aqueue, c);
	}

        ab->audio->ptr_read_buffer += ab->audio->blocksize_read;
	if (ab->audio->ptr_write_buffer == ab->audio->ptr_read_buffer)
	{
	    // return at the beginning
	    ab->audio->ptr_write_buffer = 0;
	    ab->audio->ptr_read_buffer = 0;
	}
        return 1;
    }
    return 0;
}


