
/*
 *
 * FILE: io.c
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

#include "io.h"




static void sdes_print (RtpSession *session, guint32 ssrc, rtcp_sdes_type stype, RTP_Info *info)
{
    gint i;


    const char *sdes_type_names[] =
    {
	"end", "cname", "name", "email", "telephone",
	"location", "tool", "note", "priv"
    };
    const guint8 n = sizeof(sdes_type_names) / sizeof(sdes_type_names[0]);

    if (stype > n) return;

    /*
    if (stype != RTCP_SDES_PRIV)
    {
	printf("SSRC 0x%08x reported SDES type %s - ", ssrc, sdes_type_names[stype]);
	printf("%s\n", rtp_get_sdes(session, ssrc, stype));
    }
    */

    for (i = 0; i < 10; i++)
    {
	if (info->ssrcs[i] == ssrc)
	{
	    switch (stype)
	    {
	    case RTCP_SDES_CNAME:
		g_strlcpy(info->sdes_info[i][0], rtp_get_sdes(session, ssrc, RTCP_SDES_CNAME), 256);
		break;

	    case RTCP_SDES_NAME:
		g_strlcpy(info->sdes_info[i][1], rtp_get_sdes(session, ssrc, RTCP_SDES_NAME), 256);
		break;

	    case RTCP_SDES_EMAIL:
		g_strlcpy(info->sdes_info[i][2], rtp_get_sdes(session, ssrc, RTCP_SDES_EMAIL), 256);
		break;

	    case RTCP_SDES_TOOL:
		g_strlcpy(info->sdes_info[i][4], rtp_get_sdes(session, ssrc, RTCP_SDES_TOOL), 256);
		break;

	    case RTCP_SDES_NOTE:
		g_strlcpy(info->sdes_info[i][3], rtp_get_sdes(session, ssrc, RTCP_SDES_NOTE), 256);
		break;

	    default:
		break;
	    }
            break;
	}
    }
    return;
}



static void handler_rtcp_event (RtpSession *rtpsession, RtpEvent *e)
{
    RTPUserdata *user_data;
    RtpEvent *event;


    switch (e->type)
    {
    case RX_SDES:
	event = (RtpEvent *) g_malloc(sizeof(RtpEvent));
	event->data = g_memdup(e->data, sizeof(rtcp_sdes_item));
	break;
    case RX_BYE:
	event = (RtpEvent *) g_malloc(sizeof(RtpEvent));
	event->data = NULL;
	break;
    case SOURCE_CREATED:
	event = (RtpEvent *) g_malloc(sizeof(RtpEvent));
	event->data = NULL;
	break;
    case SOURCE_DELETED:
	event = (RtpEvent *) g_malloc(sizeof(RtpEvent));
	event->data = NULL;
	break;
    case RX_SR:
	event = (RtpEvent *) g_malloc(sizeof(RtpEvent));
	event->data = g_memdup(e->data, sizeof(rtcp_sr));
	break;
    case RX_RR:
	event = (RtpEvent *) g_malloc(sizeof(RtpEvent));
	event->data = g_memdup(e->data, sizeof(rtcp_rr));
	break;
    case RR_TIMEOUT:
	event = (RtpEvent *) g_malloc(sizeof(RtpEvent));
	event->data = g_memdup(e->data, sizeof(rtcp_rr));
	break;
    case RX_APP:
	event = (RtpEvent *) g_malloc(sizeof(RtpEvent));
	event->data = e->data;
	break;
    case RX_RR_EMPTY:
    case RX_RTCP_START:
    case RX_RTCP_FINISH:
    default:
        return;
    }

    event->ssrc = e->ssrc;
    event->type = e->type;
    event->ts = g_memdup(e->ts, sizeof(struct timeval));

    user_data = rtp_get_userdata(rtpsession);
    g_async_queue_push(user_data->rtcp, event);
    return;
}



static void handler_rtp_event (RtpSession *rtpsession, RtpEvent *e)
{
    RtpPacket *p;
    RtpEvent *event;
    rtcp_app *app;
    RtcpAppData r;
    RTPUserdata *user_data;
    struct timeval *event_ts;
    static guint16 seq = 0;
    static gint control = 1;
    static guint32 timestamp;
    gint data_len;
    gint val;
    gint i;
    gint bytes;
    guint16 *j;
    guint16 *k;
    gboolean enter = TRUE;


    user_data = rtp_get_userdata(rtpsession);

    g_mutex_unlock(user_data->mutex);

    p = (RtpPacket*) e->data;
    //if ((p->pt != user_data->payload_rx) || (user_data->data_len_rx != p->data_len))
    if (p->pt != user_data->payload_rx)
    {
	/* Copy the entire packet, converting the header (only) into host byte order. */
	app = (rtcp_app *) g_malloc(RTP_MAX_PACKET_LEN);
	app->version = RTP_VERSION;
	app->p = p->p;
	app->subtype = LOCAL_RTCP_APP;
	app->pt = p->pt;
	app->length = sizeof(RtcpAppData);
	app->ssrc = rtp_my_ssrc(rtpsession);
	app->name[0] = 0;
	app->name[1] = 0;
	app->name[2] = 0;
	app->name[3] = 0;
	data_len = (app->length - 2) * 4;
        r = RTCP_APP_CHANGE_PAYLOAD;
	memcpy(app->data, &r, data_len);
	event = (RtpEvent *) g_malloc(sizeof(RtpEvent));
	event->ssrc = p->ssrc;
	event->type = RX_APP;
	event->data = app;
	event_ts = (struct timeval *) g_malloc(sizeof(struct timeval));
	gettimeofday(event_ts, NULL);
	event->ts = event_ts;

	g_async_queue_push(user_data->rtcp, event);
    }
    else
    {
        enter = TRUE;
	if (user_data->node_codec_rx != NULL)
	{
	    if ((bytes = codec_decode(user_data->node_codec_rx->codec, p->data, user_data->buffer_data_rx)) == PLUGIN_CODEC_NOTDECODE)
	    {
		g_printerr("[IO] handler_rtp_event: el plugin seleccionado es incapaz de descomprimir el audio.\n");
                enter = FALSE;
	    }
	}
	else
	{
	    k = (guint16 *) p->data;
	    j = (guint16 *) user_data->buffer_data_rx;

	    for (i=0; i<(user_data->data_len_rx/2); i++)
	    {
		*j = g_ntohs(*k);
		j++;
		k++;
	    }
	}
	if (enter)
	{
	    if (seq != 0)
	    {
		while (++seq < p->seq)
		{
		    if (control == 0)
		    {
			control = write_audio_buffer(user_data->ab, 0, user_data->mute_vol, user_data->buffer_data_rx);
		    }
		    timestamp += user_data->audio_ms_rx;
		}
	    }
	    else
	    {
		seq = p->seq;
		timestamp = p->ts;
	    }
	    while ((timestamp += user_data->audio_ms_rx) < p->ts)
	    {
		if (control == 0)
		{
		    control = write_audio_buffer(user_data->ab, 0, user_data->mute_vol, user_data->buffer_data_rx);
		}
	    }
	    control = write_audio_buffer(user_data->ab, user_data->audio_gaim_rx, user_data->mute_vol, user_data->buffer_data_rx);
	}
	else
	{
	    if (seq != 0)
	    {
		while (++seq < p->seq)
		{
		    if (control == 0)
		    {
			control = write_audio_buffer(user_data->ab, 0, user_data->mute_vol, user_data->buffer_data_rx);
		    }
		    timestamp += user_data->audio_ms_rx;
		}
	    }
	    else
	    {
		seq = p->seq;
		timestamp = p->ts;
	    }
	    while ((timestamp += user_data->audio_ms_rx) < p->ts)
	    {
		if (control == 0)
		{
		    control = write_audio_buffer(user_data->ab, 0, user_data->mute_vol, user_data->buffer_data_rx);
		}
	    }
	}
    }
    rtp_get_option(rtpsession, RTP_OPT_REUSE_PACKET_BUFS, &val);
    if (!val) g_free(p);

    g_mutex_lock(user_data->mutex);

    return;
}



static gpointer tx_loop (gpointer data)
{
    gint bytes;
    guchar *buffer;
    guchar *buffer_data;
    ThPOut *th;
    RTPUserdata *user_data;
    gint i;
    guint16 *j;
    guint16 *k;
    gboolean enter;


    th = (ThPOut *) data;
    th->rounds = 0;
    user_data = rtp_get_userdata(th->rtps);
    buffer = (guchar *) g_malloc0(th->audio_blocksize_tx);
    buffer_data = (guchar *) g_malloc0(th->data_len_tx);

    while (th->process)
    {
	user_data->rtp_ts = (gint32) th->rounds++ * th->audio_ms_tx;
	read_audio_buffer(th->ab, th->mute_mic, buffer);

	if (*(th->vad) == 1)
	{
	    if (th->vad_state != NULL)
	    {
		if (!audio_vad_calcule(th->vad_state, buffer, th->audio_blocksize_tx))
		{
                    printf("-------------------------------------------------DESCARTO AUDIO\n");
                    continue;
		}
	    }
	}
        enter = TRUE;
	if ((th->node_codec_tx != NULL) || (th->payload_tx != IO_PCM_PAYLOAD))
	{
	    if ((bytes = codec_encode(th->node_codec_tx->codec, buffer, buffer_data)) == PLUGIN_CODEC_NOTENCODE)
	    {
		g_printerr("[IO] tx_loop: el plugin seleccionado es incapaz de comprimir el audio.\n");
                enter = FALSE;
	    }
	}
	else
	{
	    k = (guint16 *) buffer;
	    j = (guint16 *) buffer_data;

	    for (i=0; i<(th->audio_blocksize_tx/2); i++)
	    {
		*j = g_htons(*k);
		j++;
		k++;
	    }
	}
	if (enter)
	{
	    bytes = rtp_send_data(th->rtps, user_data->rtp_ts, th->payload_tx, 0, 0, 0, buffer_data, th->data_len_tx, 0, 0, 0);
	    //bytes = rtp_send_data(th->rtps, user_data->rtp_ts, th->payload_tx, 0, 0, 0, buffer_data, bytes, 0, 0, 0);
	}
    }
    g_free(buffer);
    g_free(buffer_data);
    g_thread_exit(NULL);
    return NULL;
}



static gpointer rx_loop (gpointer data)
{
    ThPIn *th;
    struct timeval timeout;
    RTPUserdata *user_data;


    th = (ThPIn *) data;
    th->rounds = 0;
    user_data = rtp_get_userdata(th->rtps);

    while (th->process)
    {
	th->rounds++;
	timeout.tv_sec = 0;
	timeout.tv_usec = th->usec_recv_timeout_block;

	g_mutex_lock(user_data->mutex);

	rtp_recv(th->rtps, &timeout, user_data->rtp_ts);

        g_mutex_unlock(user_data->mutex);
    }
    g_thread_exit(NULL);
    return NULL;
}



gpointer session_io_loop (gpointer data)
{
    gpointer p;
    guint32 my_ssrc;
    GError *tmp_error;
    ABuffer *ab_read;
    ABuffer *ab_write;
    GAsyncQueue *qrtcp;
    ASession *asession;
    RTPUserdata user_data;
    GThread *th_tx_loop;
    GThread *th_rx_loop;
    ThPIn *rx_data;
    ThPOut *tx_data;
    RtpEvent *e;
    rtcp_sdes_item *r;
    GTimeVal time;
    RTP_Info *rtpinfo;


    asession = (ASession *) data;
    rtpinfo = asession->rtpinfo;
    tmp_error = NULL;

    user_data.audio_blocksize_dsp = asession->blocksize_dsp;
    user_data.mute_vol = asession->mute_vol;
    user_data.ap = asession->ap_dsp;
    user_data.mutex = g_mutex_new();
    user_data.node_codec_rx = asession->node_codec_rx;
    if (asession->node_codec_rx == NULL)
    {
	user_data.audio_ms_rx = IO_PCM_MS;
	user_data.audio_blocksize_rx = IO_PCM_SIZE;
	user_data.payload_rx = IO_PCM_PAYLOAD;
	user_data.data_len_rx = IO_PCM_SIZE;
    }
    else
    {
	user_data.audio_ms_rx = asession->node_codec_rx->pc->audio_ms;
	user_data.audio_blocksize_rx = asession->node_codec_rx->pc->uncompressed_fr_size;
	user_data.payload_rx = asession->node_codec_rx->pc->payload;
	user_data.data_len_rx = asession->node_codec_rx->pc->compressed_fr_size;
    }
    user_data.buffer_data_rx = g_malloc0(user_data.audio_blocksize_rx);
    user_data.audio_gaim_rx = asession->gaim_rx;
    user_data.node_codec_tx = asession->node_codec_tx;
    if (asession->node_codec_tx == NULL)
    {
	user_data.audio_ms_tx = IO_PCM_MS;
	user_data.audio_blocksize_tx = IO_PCM_SIZE;
	user_data.payload_tx = IO_PCM_PAYLOAD;
	user_data.data_len_tx = IO_PCM_SIZE;
    }
    else
    {
	user_data.audio_ms_tx = asession->node_codec_tx->pc->audio_ms;
	user_data.audio_blocksize_tx = asession->node_codec_tx->pc->uncompressed_fr_size;
	user_data.payload_tx = asession->node_codec_tx->pc->payload;
	user_data.data_len_tx = asession->node_codec_tx->pc->compressed_fr_size;
    }

    if ((ab_read = init_read_audio_buffer(asession->queue_in, asession->blocksize_dsp, user_data.audio_blocksize_tx, asession->ap_dsp, asession->plugins_effect, &tmp_error)) == NULL)
    {
	g_printerr("[IO] Unable init read audio buffer: %s\n", tmp_error->message);
	return tmp_error;
    }
    if ((ab_write = init_write_audio_buffer(asession->queue_out, user_data.audio_blocksize_rx, asession->blocksize_dsp, asession->ap_dsp, asession->plugins_effect, &tmp_error)) == NULL)
    {
        free_read_audio_buffer(ab_read);
	g_printerr("[IO] Unable init write audio buffer: %s\n", tmp_error->message);
	return tmp_error;
    }
    if ((qrtcp = g_async_queue_new()) == NULL)
    {
        free_read_audio_buffer(ab_read);
	free_write_audio_buffer(ab_write);
	g_set_error(&tmp_error, IO_ERROR, IO_ERROR_CONTROL_QUEUE, "null reference to async queue RTCP");
	g_printerr("[IO] Unable to create GAsyncQueue: %s\n", tmp_error->message);
	return tmp_error;
    }

    user_data.rtcp = qrtcp;
    user_data.ab = ab_write;
    user_data.rtp_ts = 0;

    rtp_set_callback(asession->rtps, handler_rtp_event, handler_rtcp_event);
    rtp_set_userdata(asession->rtps, (gpointer) &user_data);
    my_ssrc = rtp_my_ssrc(asession->rtps);

    rtpinfo->num_sources = 1;
    if (rtp_get_sdes(asession->rtps, my_ssrc, RTCP_SDES_CNAME))
    {
	g_strlcpy(rtpinfo->sdes_info[0][0], rtp_get_sdes(asession->rtps, my_ssrc, RTCP_SDES_CNAME), 256);
    }
    if (rtp_get_sdes(asession->rtps, my_ssrc, RTCP_SDES_NAME))
    {
	g_strlcpy(rtpinfo->sdes_info[0][1], rtp_get_sdes(asession->rtps, my_ssrc, RTCP_SDES_NAME), 256);
    }
    if (rtp_get_sdes(asession->rtps, my_ssrc, RTCP_SDES_EMAIL))
    {
	g_strlcpy(rtpinfo->sdes_info[0][2], rtp_get_sdes(asession->rtps, my_ssrc, RTCP_SDES_EMAIL), 256);
    }
    if (rtp_get_sdes(asession->rtps, my_ssrc, RTCP_SDES_NOTE))
    {
	g_strlcpy(rtpinfo->sdes_info[0][3], rtp_get_sdes(asession->rtps, my_ssrc, RTCP_SDES_NOTE), 256);
    }
    if (rtp_get_sdes(asession->rtps, my_ssrc, RTCP_SDES_TOOL))
    {
	g_strlcpy(rtpinfo->sdes_info[0][4], rtp_get_sdes(asession->rtps, my_ssrc, RTCP_SDES_TOOL), 256);
    }
    rtpinfo->change_payload = FALSE;
    rtpinfo->payload = user_data.payload_rx;
    rtpinfo->ssrcs[0] = my_ssrc;

    rx_data = (ThPIn *) g_malloc(sizeof(ThPIn));
    rx_data->process = TRUE;
    rx_data->rounds = 0;
    rx_data->rtps = asession->rtps;
    rx_data->ap = asession->ap_dsp;
    rx_data->ab = ab_write;
    rx_data->audio_blocksize_rx = user_data.audio_blocksize_rx;
    rx_data->payload_rx = user_data.payload_rx;
    rx_data->data_len_rx = user_data.data_len_rx;
    rx_data->audio_ms_rx = user_data.audio_ms_rx;
    rx_data->usec_recv_timeout_block = 10000;

    th_rx_loop = g_thread_create((GThreadFunc) rx_loop, rx_data, TRUE, &tmp_error);
    if (tmp_error != NULL)
    {
	free_read_audio_buffer(ab_read);
	free_write_audio_buffer(ab_write);
	g_async_queue_unref(qrtcp);
	g_printerr("[IO] Unable to create Thread to process incoming RTP data: %s\n", tmp_error->message);
	return tmp_error;
    }

    tx_data = (ThPOut *) g_malloc(sizeof(ThPOut));
    tx_data->process = TRUE;
    tx_data->rounds = 0;
    tx_data->rtps = asession->rtps;
    tx_data->ap = asession->ap_dsp;
    tx_data->mute_mic = asession->mute_mic;
    tx_data->ab = ab_read;
    tx_data->audio_blocksize_tx = user_data.audio_blocksize_tx;
    tx_data->payload_tx = user_data.payload_tx;
    tx_data->data_len_tx = user_data.data_len_tx;
    tx_data->audio_ms_tx = user_data.audio_ms_tx;
    tx_data->vad = asession->vad;
    tx_data->vad_state = *asession->vad_state;
    tx_data->node_codec_tx = user_data.node_codec_tx;

    th_tx_loop = g_thread_create((GThreadFunc) tx_loop, tx_data, TRUE, &tmp_error);
    if (tmp_error != NULL)
    {
	rx_data->process = FALSE;
	free_read_audio_buffer(ab_read);
        free_write_audio_buffer(ab_write);
	g_async_queue_unref(qrtcp);
	g_thread_join(th_tx_loop);
	g_printerr("[IO] Unable to create Thread to process out RTP data: %s\n", tmp_error->message);
	return tmp_error;
    }

    asession->bye = FALSE;
    while (asession->process)
    {
	gint i;
        gint32 ssrc_timeout = -1;
        rtcp_app *app;


	g_get_current_time(&time);
        g_time_val_add(&time, 80000);

	if ((p = g_async_queue_timed_pop(qrtcp, &time)) == NULL)
	{
	    g_mutex_lock(user_data.mutex);
	    rtp_send_ctrl(asession->rtps, user_data.rtp_ts, NULL);
	    rtp_update(asession->rtps);
	    g_mutex_unlock(user_data.mutex);
	}
	else
	{
	    e = (RtpEvent *) p;
	    switch (e->type)
	    {
	    case RX_SDES:
                // Warning controled ;-) (mutex)
                g_mutex_lock(rtpinfo->mutex);
		r = (rtcp_sdes_item *)e->data;
		sdes_print(asession->rtps, e->ssrc, r->type, rtpinfo);
                g_mutex_unlock(rtpinfo->mutex);
		break;

	    case RX_BYE:
                g_mutex_lock(rtpinfo->mutex);
		if (rtpinfo->num_sources == 2)
		{
		    asession->bye = TRUE;
                    rtpinfo->bye = TRUE;
		    asession->process = FALSE;
		}
                g_mutex_unlock(rtpinfo->mutex);
		break;

	    case SOURCE_CREATED:
		g_mutex_lock(rtpinfo->mutex);
                rtpinfo->ssrcs[rtpinfo->num_sources] = e->ssrc;
		rtpinfo->num_sources = rtpinfo->num_sources + 1;
                g_mutex_unlock(rtpinfo->mutex);
		break;

	    case SOURCE_DELETED:
                g_mutex_lock(rtpinfo->mutex);
		rtpinfo->num_sources = rtpinfo->num_sources - 1;
		for (i = 0; i < 10; i++)
		{
		    if (rtpinfo->ssrcs[i] == e->ssrc)
		    {
			rtpinfo->ssrcs[i] = 0;
			memset(rtpinfo->sdes_info[i][0], 0, 256);
			memset(rtpinfo->sdes_info[i][1], 0, 256);
			memset(rtpinfo->sdes_info[i][2], 0, 256);
			memset(rtpinfo->sdes_info[i][3], 0, 256);
			memset(rtpinfo->sdes_info[i][4], 0, 256);
		    }
		}
		if (rtpinfo->num_sources == 1)
		{
                    rtpinfo->timeout = TRUE;
		}
                g_mutex_unlock(rtpinfo->mutex);
		break;

	    case RX_SR:
		break;

	    case RX_RR:
		break;

	    case RR_TIMEOUT:

                ssrc_timeout = e->ssrc;
		//g_mutex_unlock(rtpinfo->mutex);
		//if (rtpinfo->num_sources == 2)
		//{
                //    rtpinfo->timeout = TRUE;
		//}
                //g_mutex_unlock(rtpinfo->mutex);
		break;

	    case RX_APP:

		app = (rtcp_app *) e->data;
		if ((app->ssrc == my_ssrc) && (app->subtype == LOCAL_RTCP_APP))
		{
		    // change payload
                    // OK for the moment, becose ...
		    g_mutex_lock(rtpinfo->mutex);
		    rtpinfo->change_payload = TRUE;
                    rtpinfo->payload = app->pt;
		    g_mutex_unlock(rtpinfo->mutex);
		}
		break;

	    default:
                break;
	    }
	    g_free(e->data);
	    g_free(e->ts);
            g_free(e);
	    g_mutex_lock(user_data.mutex);
	    rtp_send_ctrl(asession->rtps, user_data.rtp_ts, NULL);
	    rtp_update(asession->rtps);
	    g_mutex_unlock(user_data.mutex);
	}
    }
    tx_data->process = FALSE;
    g_thread_join(th_tx_loop);

    rx_data->process = FALSE;
    g_thread_join(th_rx_loop);

    if (asession->bye)
    {
	rtp_send_bye(asession->rtps);
    }
    while ((p = g_async_queue_try_pop(qrtcp)) != NULL)
    {
	e = (RtpEvent *) p;
	g_free(e->data);
	g_free(e->ts);
	g_free(e);
    }
    //g_async_queue_unref(qrtcp);

    free_read_audio_buffer(ab_read);
    free_write_audio_buffer(ab_write);

    g_free(rx_data);
    g_free(tx_data);
    g_free(user_data.buffer_data_rx);

    g_mutex_free(user_data.mutex);

    g_thread_exit(NULL);
    return NULL;
}



RtpSession *create_rtp_session (InitRTPsession *init, GError **error)
{
    RtpSession *session;
    GError *tmp_error = NULL;
    guint32 my_ssrc;


    if ((session = rtp_init(init->addr, NULL, init->rx_port, init->tx_port, init->ttl, init->rtcp_bw, NULL, NULL, NULL, &tmp_error)) == NULL)
    {
	g_printerr("[IO] Unable to create RTP/RTCP session: %s\n", tmp_error->message);
        g_propagate_error (error, tmp_error);
	return NULL;
    }
    my_ssrc = rtp_my_ssrc(session);
    if (init->username != NULL) rtp_set_sdes(session, my_ssrc, RTCP_SDES_NAME, init->username, strlen(init->username));
    if (init->toolname != NULL) rtp_set_sdes(session, my_ssrc, RTCP_SDES_TOOL, init->toolname, strlen(init->toolname));
    if (init->usermail != NULL) rtp_set_sdes(session, my_ssrc, RTCP_SDES_EMAIL, init->usermail, strlen(init->usermail));
    if (init->userlocate != NULL)  rtp_set_sdes(session, my_ssrc, RTCP_SDES_LOC, init->userlocate, strlen(init->userlocate));
    if (init->usernote != NULL)  rtp_set_sdes(session, my_ssrc, RTCP_SDES_NOTE, init->usernote, strlen(init->usernote));

    rtp_set_option(session, RTP_OPT_FILTER_MY_PACKETS, TRUE);
    rtp_set_option(session, RTP_OPT_REUSE_PACKET_BUFS, TRUE);
    rtp_set_option(session, RTP_OPT_WEAK_VALIDATION, FALSE);

    return session;
}



void destroy_rtp_session (RtpSession *session, GError **error)
{
    GError *tmp_error = NULL;

    rtp_done(session, &tmp_error);
    if (tmp_error != NULL)
    {
	g_propagate_error(error, tmp_error);
	g_printerr("[IO] destroy_rtp_session: %s\n", tmp_error->message);
    }
    return;
}


