
/*
 *
 * FILE: audio_processor.c
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

#include "audio_processor.h"



static gboolean process = TRUE;
static AudioProperties *prpts_dsp = NULL;
static AudioProperties *prpts_internal = NULL;

static gint audio_blocksize_dsp = 0;
static gint audio_blocksize_buf = 0;

static glong sleep_time = 0;
static gint *audio_cng = 0;

static GMutex *mutex_aqueue_mic = NULL;
static GMutex *mutex_aqueue_spk = NULL;

static GSList *list_aqueues_mic = NULL;
static GSList *list_aqueues_spk = NULL;

static Plugin_InOut *plugin = NULL;

static GThread *gth_audio;

static Convert_Function audio_in_convert_function = NULL;
static Convert_Function audio_out_convert_function = NULL;




gpointer audio_processor_loop (gpointer data);
static gint compare_node_aqueue (gconstpointer a, gconstpointer b);
static inline void sleep_us (gint usec);




gboolean audio_processor_loop_create (AudioProperties *ap_dsp, AudioProperties *ap_internal, gint blocksize_dsp, gint blocksize_internal, glong block_time, gint *cng, Plugin_InOut *pluginio, GError **error)
{
    GError *tmp_error = NULL;


    process = TRUE;
    prpts_dsp = (AudioProperties *) g_malloc(sizeof(AudioProperties));
    memcpy(prpts_dsp, ap_dsp, sizeof(AudioProperties));
    prpts_internal = (AudioProperties *) g_malloc(sizeof(AudioProperties));
    memcpy(prpts_internal, ap_internal, sizeof(AudioProperties));

    sleep_time = block_time;

    if (pluginio == NULL)
    {
	g_set_error(error, AUDIO_PROCESSOR_ERROR, AUDIO_PROCESSOR_ERROR_NULL_IO, "plugin E/S de audio nulo");
	g_printerr("[AUDIO PROCESSOR] Plugin 'Plugin_InOut' nulo.\n");
	return FALSE;
    }
    plugin = pluginio;

    mutex_aqueue_mic = g_mutex_new();
    mutex_aqueue_spk = g_mutex_new();
    list_aqueues_mic = NULL;
    list_aqueues_spk = NULL;

    audio_in_convert_function = get_convert_function(ap_internal, ap_dsp);
    audio_out_convert_function = get_convert_function(ap_dsp, ap_internal);

    if (audio_in_convert_function == NULL)
    {
	if (blocksize_internal != blocksize_dsp)
	{
	    g_set_error(error, AUDIO_PROCESSOR_ERROR, AUDIO_PROCESSOR_ERROR_BLOCKSIZE, "tamaños de bloque de memoria distintos para 2 formatos de audio iguales");
	    g_printerr("[AUDIO PROCESSOR] Tamaños de bloque de memoria distintos para 2 formatos de audio iguales.\n");
	    return FALSE;
	}
    }
    audio_blocksize_dsp = blocksize_dsp;
    audio_blocksize_buf = blocksize_internal;
    audio_cng = cng;

    gth_audio = g_thread_create((GThreadFunc) audio_processor_loop, NULL, TRUE, &tmp_error);
    if (tmp_error != NULL)
    {
	g_propagate_error(error, tmp_error);
	return FALSE;
    }
    return TRUE;
}



gboolean audio_processor_loop_destroy (GError **error)
{
    if ((list_aqueues_mic != NULL) || (list_aqueues_spk != NULL))
    {
	g_set_error(error, AUDIO_PROCESSOR_ERROR, AUDIO_PROCESSOR_ERROR_AQUEUES, "todavia hay colas de audio");
	g_printerr("[AUDIO PROCESSOR] Las colas de audio (productor y/o consumidor) no estan vacias.\n");
	return FALSE;
    }
    process = FALSE;
    g_thread_join(gth_audio);
    g_mutex_free(mutex_aqueue_mic);
    g_mutex_free(mutex_aqueue_spk);
    g_free(prpts_dsp);
    g_free(prpts_internal);
    return TRUE;
}



gboolean audio_processor_loop_free_mic (gint id)
{
    GSList *list;
    NodeAqueue *node;
    gpointer p;


    list = g_slist_find_custom(list_aqueues_mic, &id, (GCompareFunc) compare_node_aqueue);
    if (list == NULL)
    {
	g_printerr("[AUDIO PROCESSOR] No se encuentra 'GAsyncQueue' consumidor de audio con id %d para liberar.\n", id);
	return FALSE;
    }
    g_mutex_lock(mutex_aqueue_mic);
    list_aqueues_mic = g_slist_remove_link(list_aqueues_mic, list);
    if (g_slist_length(list_aqueues_mic) == 0) g_slist_free(list_aqueues_mic);
    g_mutex_unlock(mutex_aqueue_mic);

    node = (NodeAqueue *) g_slist_nth_data(list, 0);
    while ((p = g_async_queue_try_pop(node->aqueue)) != NULL)
    {
	g_free(p);
    }
    g_async_queue_unref(node->aqueue);
    g_free(node);
    g_slist_free(list);

    g_print("[AUDIO PROCESSOR] Liberado consumidor audio con id  %d.\n", id);
    return TRUE;
}



gboolean audio_processor_loop_free_spk (gint id)
{
    GSList *list;
    NodeAqueue *node;
    gpointer p;


    list = g_slist_find_custom(list_aqueues_spk, &id, (GCompareFunc) compare_node_aqueue);
    if (list == NULL)
    {
	g_printerr("[AUDIO PROCESSOR] No se encuentra 'GAsyncQueue' productor de audio con id %d para liberar.\n", id);
	return FALSE;
    }
    g_mutex_lock(mutex_aqueue_spk);
    list_aqueues_spk = g_slist_remove_link(list_aqueues_spk, list);
    if (g_slist_length(list_aqueues_spk) == 0) g_slist_free(list_aqueues_spk);
    g_mutex_unlock(mutex_aqueue_spk);

    node = (NodeAqueue *) g_slist_nth_data(list, 0);
    while ((p = g_async_queue_try_pop(node->aqueue)) != NULL)
    {
	g_free(p);
    }
    g_async_queue_unref(node->aqueue);
    g_free(node);
    g_slist_free(list);

    g_print("[AUDIO PROCESSOR] Liberado productor audio con id  %d.\n", id);
    return TRUE;
}



gboolean audio_processor_loop_create_mic (gint id, GAsyncQueue **queue)
{
    NodeAqueue *node;
    GAsyncQueue *aqueue;


    node = (NodeAqueue *) g_malloc(sizeof(NodeAqueue));
    if (node == NULL) return FALSE;

    aqueue = g_async_queue_new();
    node->id_aqueue = id;
    node->aqueue = aqueue;
    *queue = aqueue;

    g_mutex_lock(mutex_aqueue_mic);
    list_aqueues_mic = g_slist_append(list_aqueues_mic, node);
    g_mutex_unlock(mutex_aqueue_mic);

    g_print("[AUDIO PROCESSOR] Creado consumidor audio con id  %d.\n", id);
    return TRUE;
}



gboolean audio_processor_loop_create_spk (gint id, GAsyncQueue **queue)
{
    NodeAqueue *node;
    GAsyncQueue *aqueue;


    node = (NodeAqueue *) g_malloc(sizeof(NodeAqueue));
    if (node == NULL) return FALSE;

    aqueue = g_async_queue_new();
    node->id_aqueue = id;
    node->aqueue = aqueue;
    *queue = aqueue;

    g_mutex_lock(mutex_aqueue_spk);
    list_aqueues_spk = g_slist_append(list_aqueues_spk, node);
    g_mutex_unlock(mutex_aqueue_spk);

    g_print("[AUDIO PROCESSOR] Creado productor de audio con id  %d.\n", id);
    return TRUE;
}



gpointer audio_processor_loop (gpointer data)
{
    guchar *areturn;
    guchar *buffer_read;
    guchar *buffer_read_aux;
    guchar *buffer_write;
    guchar *buffer_write_aux;
    guchar *buffer_write_add;
    NodeAqueue *node;
    gint number_spk;
    gint counter;
    gfloat factor;
    gint bytes_read;
    gint c;
    gint16 *j;
    gint16 *k;
    gfloat tmp;
    gpointer buf;
    gint counterpops;


    UNUSED(data);
    buffer_read = (guchar *) g_malloc0(audio_blocksize_dsp);
    buffer_read_aux = (guchar *) g_malloc0(audio_blocksize_buf);
    buffer_write_aux = (guchar *) g_malloc0(audio_blocksize_buf);
    buffer_write_add = (guchar *) g_malloc0(audio_blocksize_buf);
    buffer_write = (guchar *) g_malloc0(audio_blocksize_dsp);

    while (process)
    {
	if (sleep_time != 0)
	{
	    sleep_us(sleep_time - 1000);
	}
        g_mutex_lock(mutex_aqueue_mic);

	bytes_read = plugin->read(buffer_read, audio_blocksize_dsp);
	if (bytes_read != audio_blocksize_dsp)
	{
	    g_printerr("[AUDIO PROCESSOR] Bloque esperado != bytes leidos (%d != %d)\n", audio_blocksize_dsp, bytes_read);
	    if (bytes_read <= 0)
	    {
		g_printerr("[AUDIO PROCESSOR] bytes leidos : %d\n", bytes_read);
	    }
	    else
	    {
		gint cond = audio_blocksize_dsp - bytes_read;
		gint index = bytes_read;

		/* fix for some obscure drivers (in BSD) that don't return   */
		/* the number of bytes that they should                      */
		while (cond > 0)
		{
		    bytes_read = plugin->read(&buffer_read[index], cond);
		    index += bytes_read;
		    cond = cond - bytes_read;
		}
	    }
	}

        memcpy(buffer_read_aux, buffer_read, audio_blocksize_dsp);
	buf = buffer_read_aux;
	if (audio_in_convert_function != NULL) audio_blocksize_buf = audio_in_convert_function(buf, audio_blocksize_dsp);
        else audio_blocksize_buf = audio_blocksize_dsp;

	// Audio in (mic)
	counter = 0;
	node = (NodeAqueue *) g_slist_nth_data(list_aqueues_mic, counter);
	while (node != NULL)
	{
#ifdef A_PROCESSOR_DEGUG
	    g_print("[AUDIO PROCESSOR] LEE: %d; Numero de cola: %d; long cola mics: %d\n", bytes_read, counter, g_async_queue_length(node->aqueue));
            fflush(NULL);
#endif
	    g_async_queue_push(node->aqueue, g_memdup(buffer_read_aux, audio_blocksize_buf));

	    node = (NodeAqueue *) g_slist_nth_data(list_aqueues_mic, ++counter);
	}
	g_mutex_unlock(mutex_aqueue_mic);


	// Audio out (speaker)
	g_mutex_lock(mutex_aqueue_spk);

	counter = 0;
        counterpops = 0;
        number_spk = g_slist_length(list_aqueues_spk);
	factor = 1.0 / (gfloat) number_spk;
        memset(buffer_write_add, '\0', audio_blocksize_buf);
	node = (NodeAqueue *) g_slist_nth_data(list_aqueues_spk, counter);
	while (node != NULL)
	{
	    if ((areturn = g_async_queue_try_pop(node->aqueue)) != NULL)
	    {
                counterpops++;
		if (number_spk != 1)
		{
		    memcpy(buffer_write_aux, areturn, audio_blocksize_buf);
		    j = (gint16 *) buffer_write_aux;
		    k = (gint16 *) buffer_write_add;
		    for (counter=0; counter<(audio_blocksize_buf/2); counter++)
		    {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN

			tmp = *j * factor;

			if (tmp >= 0) *k = *k + (gint16) (tmp + 0.5);
			else *k = *k + (gint16) (tmp - 0.5);

#elif G_BYTE_ORDER == G_BIG_ENDIAN

			tmp = GINT16_TO_BE(*j);
			tmp = tmp * factor;

			if (tmp >= 0) *k = GINT16_TO_LE(GINT16_TO_BE(*k) + (gint16) (tmp + 0.5));
			else *k = GINT16_TO_LE(GINT16_TO_BE(*k) + (gint16) (tmp - 0.5));
#else
#error "G_BYTE_ORDER should be big or little endian."
#endif
			j++;
			k++;
		    }
		}
		else
		{
		    memcpy(buffer_write_add, areturn, audio_blocksize_buf);
		}
                g_free(areturn);
	    }
#ifdef A_PROCESSOR_DEGUG
	    g_print("[AUDIO PROCESSOR] ESCRIBE: %d; Numero de cola: %d; long cola speakers %d\n", bytes_read, counter, g_async_queue_length(node->aqueue));
            fflush(NULL);
#endif
	    node = (NodeAqueue *) g_slist_nth_data(list_aqueues_spk, ++counter);
	}

	buf = buffer_write_add;
	if (audio_out_convert_function != NULL) audio_blocksize_dsp = audio_out_convert_function(buf, audio_blocksize_buf);

	memcpy(buffer_write, buffer_write_add, audio_blocksize_dsp);

	if ((counterpops == 0) && (*audio_cng == 1))
	{
	    j = (gint16 *) buffer_write;
	    for (c=0; c<(audio_blocksize_dsp/2); c++)
	    {
		tmp = (*j * (1 - AP_GNG_FACTOR)) + (AP_GNG_FACTOR * (gint16) g_random_int_range(-AP_CNG_RANGE, AP_CNG_RANGE));
		if (tmp >= 0) *j = (gint16) (tmp + 0.5);
		else *j = (gint16) (tmp - 0.5);
		j++;
	    }
	}

	bytes_read = plugin->write(buffer_write, audio_blocksize_dsp);
	if (bytes_read != audio_blocksize_dsp)
	{
	    g_printerr("[AUDIO PROCESSOR] Bloque disponible != bytes escritos (%d != %d)\n", audio_blocksize_dsp, bytes_read);
	}
	g_mutex_unlock(mutex_aqueue_spk);

        sleep_us(1000);
    }
    plugin->flush(0);
    g_free(buffer_read);
    g_free(buffer_read_aux);
    g_free(buffer_write_aux);
    g_free(buffer_write_add);
    g_free(buffer_write);
    g_thread_exit(NULL);
    return NULL;
}



static gint compare_node_aqueue (gconstpointer a, gconstpointer b)
{
    NodeAqueue *na;
    gint *nb;


    na = (NodeAqueue *) a;
    nb = (gint *) b;

    if ((na != NULL) && (nb != NULL))
    {
	if (na->id_aqueue == *nb) return 0;
        return 1;
    }
    return -1;
}



static inline void sleep_us (gint usec)
{
#ifdef HAVE_NANOSLEEP
    struct timespec req;

    req.tv_sec = usec / 1000000;
    usec -= req.tv_sec * 1000000;
    req.tv_nsec = usec * 1000;

    nanosleep(&req, NULL);
#else
    struct timeval tv;

    tv.tv_sec = usec / 1000000;
    usec -= tv.tv_sec * 1000000;
    tv.tv_usec = usec;
    select(0, NULL, NULL, NULL, &tv);
#endif
    return;
}


