
/*
 *
 * FILE: audio_buffer_in.c
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




ABuffer *init_read_audio_buffer (gpointer audio_aqueue, gint audio_blksize_in, gint audio_blksize_out, AudioProperties *apts, GSList *plugins, GError **error)
{
    ABuffer *ab;


    if (audio_aqueue == NULL) return NULL;
    if ((audio_blksize_in < audio_blksize_out) || (0 > audio_blksize_in) || (0 > audio_blksize_out) || ((audio_blksize_in%2) != 0))
    {
	g_set_error(error, AUDIO_BUFFER_ERROR, AUDIO_BUFFER_BLKSIZE,
                    "audio : blocksize out > blocksize in ( %d > %d > 0 y/o %d debe ser par)",
		    audio_blksize_in, audio_blksize_out, audio_blksize_in);
	g_printerr("[AUDIO BUFFER] init_read_audio_buffer: Audio Read Buffer: blocksize out > blocksize in ( %d > %d > 0 y/o %d debe ser par).\n", audio_blksize_in, audio_blksize_out, audio_blksize_in);
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
	g_printerr("[AUDIO BUFFER] init_read_audio_buffer: no hay memoria suficiente\n");
	return NULL;
    }
    return ab;
}



gint free_read_audio_buffer (ABuffer *ab)
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



gint read_audio_buffer (ABuffer *ab, gint *mute, gpointer audio_data)
{
    gpointer c;
    gint counter;
    gint b;
    PluginEffectNode *pn;


    if ((ab->audio->ptr_write_buffer - ab->audio->ptr_read_buffer) < ab->audio->blocksize_read)
    {
	if (ab->audio->ptr_write_buffer == ab->audio->ptr_read_buffer)
	{
	    // return at the beginning
	    ab->audio->ptr_write_buffer = 0;
	    ab->audio->ptr_read_buffer = 0;
	}

	do
	{
	    c = g_async_queue_pop(ab->aqueue);
	    memcpy((ab->audio->buffer + ab->audio->ptr_write_buffer), c, ab->audio->blocksize_write);
	    g_free(c);
	}
	while (*mute == 1);

	// plugins
	counter = 0;
	pn = (PluginEffectNode *) g_slist_nth_data(ab->list_plugins, counter);
	while (pn)
	{
	    if (pn->active)
	    {
                b = ab->audio->blocksize_write;
		pn->plugin->pefunction(pn->state, (ab->audio->buffer + ab->audio->ptr_write_buffer), &b, AUDIO_CALL_OUT);
	    }
	    counter++;
	    pn = (PluginEffectNode *) g_slist_nth_data(ab->list_plugins, counter);
	}
	ab->audio->ptr_write_buffer += ab->audio->blocksize_write;
	memcpy(audio_data, (ab->audio->buffer + ab->audio->ptr_read_buffer), ab->audio->blocksize_read);
	ab->audio->ptr_read_buffer += ab->audio->blocksize_read;
	return 1;
    }
    // and then returns the buffer
    memcpy(audio_data, (ab->audio->buffer + ab->audio->ptr_read_buffer), ab->audio->blocksize_read);
    ab->audio->ptr_read_buffer += ab->audio->blocksize_read;
    return 0;
}


