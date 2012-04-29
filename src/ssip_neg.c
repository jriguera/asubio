
/*
 *
 * FILE: ssip_neg.c
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

#include "ssip_neg.h"



#define SSIP_ERROR_CODEC_MSG       "imposible establecer comunicacion: codec de audio no definido"
#define SSIP_ERROR_CONNECT_MSG     "Paquete de error recibido :"
#define SSIP_ERROR_TIME_RECV_MSG   "error recibiendo paquete de datos, time expired ..."
#define SSIP_ERROR_RECV_MSG        "error recibiendo datos: formato de paquete desconocido"
#define SSIP_ERROR_SEND_MSG        "error enviando paquete de datos"

#define SSIP_CONNECT_INTERRUPT_MSG "comunicacion de negociacion interrumpida ..."




// funtions


static void set_pkt (SsipPkt *pkt, gchar *from_user, gchar *from_host, guint16 from_port,
		     gchar *to_user, gchar *to_host, guint16 to_port)
{
    gint len;


    len = strlen(from_user) + 1;
    if (PKT_USERNAME_LEN <= len) len = PKT_USERNAME_LEN;
    g_strlcpy(pkt->from_user, from_user, len);
    len = strlen(from_host) + 1;
    if (PKT_HOSTNAME_LEN <= len) len = PKT_HOSTNAME_LEN;
    g_strlcpy(pkt->from_host, from_host, len);
    pkt->from_port = g_htons(from_port);

    len = strlen(to_user) + 1;
    if (PKT_USERNAME_LEN <= len) len = PKT_USERNAME_LEN;
    g_strlcpy(pkt->to_user, to_user, len);
    len = strlen(to_host) + 1;
    if (PKT_HOSTNAME_LEN <= len) len = PKT_HOSTNAME_LEN;
    g_strlcpy(pkt->to_host, to_host, len);
    pkt->to_port = g_htons(to_port);
}



static gboolean time_expired (Socket_tcp *skt)
{
    fd_set rfds;
    struct timeval tv;
    gint valret;
    gint fd;


    fd = tcp_fd(skt);
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    tv.tv_sec = SSIP_NEG_TCPTIME_EXPIRED;
    tv.tv_usec = 0;
    valret = select(fd+1, &rfds, NULL, NULL, &tv);

    if (valret == -1) return TRUE;
    if (FD_ISSET(fd, &rfds)) return FALSE;
    return TRUE;
}



static gboolean time_expired_mtime (Socket_tcp *skt, guint mt)
{
    fd_set rfds;
    struct timeval tv;
    gint valret;
    gint fd;


    fd = tcp_fd(skt);
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    tv.tv_sec = 0;
    tv.tv_usec = mt;
    valret = select(fd+1, &rfds, NULL, NULL, &tv);
    if (valret == -1) return FALSE;
    if (FD_ISSET(fd, &rfds)) return TRUE;
    return FALSE;
}



/*
 * ssip_accept:
 *
 * return
 * -2 : error, error = NULL;
 * -1 : error;
 *  0 : bye recv;
 *  1 : no accepted;
 *  2 : accepted;
 *
 */

gint ssip_accept (Socket_tcp *skt, gchar *from_user, gchar *from_host, guint16 from_port,
		  gchar *from_real_host, guint16 rtp_port,
		  GSList *list_codec, GMutex *list_mutex, NotifyCall *_notify_call, GMutex *_mutex_notify_call, GError **error,
		  gchar **rtp_sel_user, gchar **rtp_sel_host, guint16 *rtp_sel_port, Node_listvocodecs **sel_codec)
{
    gboolean search;
    gchar *_rtp_sel_host;
    gint len;
    gint counter;
    gint32 type;
    gchar *to_user;
    gchar *to_host;
    guint16 to_port;
    gchar *call_from_user;
    gchar *call_from_host;
    guint16 call_from_port;
    SsipPkt *pkt_s;
    SsipPkt *pkt_r;
    gint16 *int16;
    Node_listvocodecs *pn = NULL;
    gchar *rtp_host;
    gboolean _break;


    pkt_s = (SsipPkt *) g_malloc0(sizeof(SsipPkt));
    pkt_r = (SsipPkt *) g_malloc0(sizeof(SsipPkt));

    len = strlen(from_user) + 1;
    if (PKT_USERNAME_LEN <= len) len = PKT_USERNAME_LEN;
    g_strlcpy(pkt_s->from_user, from_user, len);
    len = strlen(from_host) + 1;
    if (PKT_HOSTNAME_LEN <= len) len = PKT_HOSTNAME_LEN;
    g_strlcpy(pkt_s->from_host, from_host, len);
    pkt_s->from_port = g_htons(from_port);

    if (time_expired(skt))
    {
	g_printerr("[SSIP_NEG] ssip_accept: Error recv data packet, time expired ...\n");
	g_free(pkt_s);
	g_free(pkt_r);
        return -2;
    }

    if (tcp_recv(skt, pkt_r, sizeof(SsipPkt), 0) != sizeof(SsipPkt))
    {
	g_printerr("[SSIP_NEG] ssip_accept: Error sending data packet\n");
	g_free(pkt_s);
	g_free(pkt_r);
        return -2;
    }

    type = g_ntohl(pkt_r->type);
    switch (type)
    {
    case CONNECT_HELO:

	break;
    case CONNECT_BYE:

	g_strlcpy(pkt_s->to_user, pkt_r->from_user, PKT_USERNAME_LEN);
	g_strlcpy(pkt_s->to_host, pkt_r->from_host, PKT_HOSTNAME_LEN);
	pkt_s->to_port = pkt_r->from_port;

	pkt_s->type = g_htonl(CONNECT_BYE);
	pkt_s->data_len = g_htonl(0);
	if (tcp_send(skt, pkt_s, sizeof(SsipPkt), 0) != sizeof(SsipPkt))
	{
	    g_printerr("[SSIP_NEG] ssip_accept: Error sending data packet\n");
	    g_free(pkt_s);
	    g_free(pkt_r);
	    return -2;
	}
	g_free(pkt_s);
	g_free(pkt_r);
        return 0;
	break;

    case CONNECT_ERROR:
    default:
	g_free(pkt_s);
	len = g_ntohl(pkt_r->data_len);
	if (PKT_USERDATA_LEN <= len) len = PKT_USERDATA_LEN - 1;
        pkt_r->data[len] = 0;
	g_printerr("[SSIP_NEG] ssip_accept: RECV CONNECT_ERROR: %s\n", pkt_r->data);
	g_free(pkt_r);
	return -2;
	break;
    }

    len = strlen(pkt_r->from_user);
    if (PKT_USERNAME_LEN <= len) len = PKT_USERNAME_LEN;
    to_user = g_strndup(pkt_r->from_user, len);
    len = strlen(pkt_r->from_host);
    if (PKT_HOSTNAME_LEN <= len) len = PKT_HOSTNAME_LEN;
    to_host = g_strndup(pkt_r->from_host, len);
    to_port = g_ntohs(pkt_r->from_port);

    len = strlen(pkt_r->to_user);
    if (PKT_USERNAME_LEN <= len) len = PKT_USERNAME_LEN;
    call_from_user = g_strndup(pkt_r->to_user, len);
    len = strlen(pkt_r->from_host);
    if (PKT_HOSTNAME_LEN <= len) len = PKT_HOSTNAME_LEN;
    call_from_host = g_strndup(pkt_r->to_host, len);
    call_from_port = g_ntohs(pkt_r->to_port);

    if ((strcmp(call_from_user, from_user) || strcmp(call_from_host, from_host)))
    {
	g_printerr("[SSIP_NEG] ssip_accept: Recv packet to unknown user (who is %s@%s?)\n", call_from_user, call_from_host);
        g_free(call_from_host);
        g_free(call_from_user);
	set_pkt(pkt_s, from_user, from_host, from_port, to_user, to_host, to_port);
	pkt_s->type = g_htonl(CONNECT_BYE);
	pkt_s->data_len = g_htonl(0);

	if (tcp_send(skt, pkt_s, sizeof(SsipPkt), 0) != sizeof(SsipPkt))
	{
	    g_printerr("[SSIP_NEG] ssip_accept: Error sending data packet\n");
	    g_free(to_user);
	    g_free(to_host);
	    g_free(pkt_s);
	    g_free(pkt_r);
	    return -2;
	}
	if (time_expired(skt))
	{
	    g_printerr("[SSIP_NEG] ssip_accept: Error recv data packet, time expired ...\n");
	    g_free(to_user);
	    g_free(to_host);
	    g_free(pkt_s);
	    g_free(pkt_r);
	    return -2;
	}
	if (tcp_recv(skt, pkt_r, sizeof(SsipPkt), 0) != sizeof(SsipPkt))
	{
	    g_printerr("[SSIP_NEG] ssip_accept: Error recv data packet: unknown format\n");
	    g_free(to_user);
	    g_free(to_host);
	    g_free(pkt_s);
	    g_free(pkt_r);
	    return -2;
	}
	type = g_ntohl(pkt_r->type);
	switch (type)
	{
	case CONNECT_BYE:
	    g_free(to_user);
	    g_free(to_host);
	    g_free(pkt_s);
	    g_free(pkt_r);
            return 0;
	    break;
	case CONNECT_ERROR:
	default:
	    len = g_ntohl(pkt_r->data_len);
	    if (PKT_USERDATA_LEN <= len) len = PKT_USERDATA_LEN - 1;
	    pkt_r->data[len] = 0;
	    g_printerr("[SSIP_NEG] ssip_accept: RECV CONNECT_ERROR: %s\n", pkt_r->data);
	    g_free(pkt_r);
	    g_free(pkt_s);
	    g_free(to_user);
	    g_free(to_host);
	    return -2;
	    break;
	}
    }
    g_free(call_from_host);
    g_free(call_from_user);

    g_mutex_lock(_mutex_notify_call);
    _notify_call->from_user = to_user;
    _notify_call->pkt_r = pkt_r;
    _notify_call->skt = skt;
    _notify_call->ready = TRUE;
    _notify_call->process = FALSE;
    _notify_call->accept = FALSE;
    g_mutex_unlock(_mutex_notify_call);

    _break = FALSE;
    while (TRUE)
    {
	g_mutex_lock(_mutex_notify_call);
	if (!_notify_call->ready) _break = TRUE;
	g_mutex_unlock(_mutex_notify_call);

        if (_break) break;
        usleep(100000);
    }
    g_mutex_lock(_mutex_notify_call);
    _break = _notify_call->accept;
    g_mutex_unlock(_mutex_notify_call);

    if (!_break)
    {
	set_pkt(pkt_s, from_user, from_host, from_port, to_user, to_host, to_port);
	pkt_s->type = g_htonl(CONNECT_BYE);
	pkt_s->data_len = g_htonl(0);
	if (tcp_send(skt, pkt_s, sizeof(SsipPkt), 0) != sizeof(SsipPkt))
	{
	    g_printerr("[SSIP_NEG] ssip_accept: Error sending data packet\n");
	    g_free(to_user);
	    g_free(to_host);
	    g_free(pkt_s);
	    g_free(pkt_r);
	    return -2;
	}
	if (time_expired(skt))
	{
	    g_printerr("[SSIP_NEG] ssip_accept: Error recv data packet, time expired ...\n");
	    g_free(to_user);
	    g_free(to_host);
	    g_free(pkt_s);
	    g_free(pkt_r);
	    return -2;
	}
	tcp_recv(skt, pkt_r, sizeof(SsipPkt), 0);
	g_free(to_user);
	g_free(to_host);
	g_free(pkt_s);
	g_free(pkt_r);
	return 1;
    }

    // Accepted

    set_pkt(pkt_s, from_user, from_host, from_port, to_user, to_host, to_port);
    pkt_s->type = g_htonl(CONNECT_VALIDUSER);
    pkt_s->data_len = g_htonl(0);

    if (tcp_send(skt, pkt_s, sizeof(SsipPkt), 0) != sizeof(SsipPkt))
    {
	g_set_error(error, SSIP_ERROR, SSIP_ERROR_SEND, SSIP_ERROR_SEND_MSG);
	g_printerr("[SSIP_NEG] ssip_accept: Error sending data packet\n");
        g_free(to_user);
        g_free(to_host);
	g_free(pkt_s);
	g_free(pkt_r);
        return -1;
    }
    if (time_expired(skt))
    {
	g_set_error(error, SSIP_ERROR, SSIP_ERROR_TIME_RECV, SSIP_ERROR_TIME_RECV_MSG);
	g_printerr("[SSIP_NEG] ssip_accept: Error recv data packet, time expired ...\n");
        g_free(to_user);
        g_free(to_host);
	g_free(pkt_s);
	g_free(pkt_r);
	return -1;
    }
    if (tcp_recv(skt, pkt_r, sizeof(SsipPkt), 0) != sizeof(SsipPkt))
    {
	g_set_error(error, SSIP_ERROR, SSIP_ERROR_RECV, SSIP_ERROR_RECV_MSG);
	g_printerr("[SSIP_NEG] ssip_accept: Error recv data packet: unknown format\n");
        g_free(to_user);
        g_free(to_host);
	g_free(pkt_s);
	g_free(pkt_r);
	return -1;
    }
    type = g_ntohl(pkt_r->type);
    switch (type)
    {
    case CONNECT_CODECTX:

	memset(&(pkt_s->data), 0, PKT_USERDATA_LEN);
        len = g_ntohl(pkt_r->data_len);
	if (len < 1)
	{
	    set_pkt(pkt_s, from_user, from_host, from_port, to_user, to_host, to_port);
	    pkt_s->type = g_htonl(CONNECT_BYE);
	    pkt_s->data_len = g_htonl(0);
	    if (tcp_send(skt, pkt_s, sizeof(SsipPkt), 0) != sizeof(SsipPkt))
	    {
		g_set_error(error, SSIP_ERROR, SSIP_ERROR_SEND, SSIP_ERROR_SEND_MSG);
		g_printerr("[SSIP_NEG] ssip_accept: Error sending data packet\n");
		g_free(to_user);
		g_free(to_host);
		g_free(pkt_s);
		g_free(pkt_r);
		return -1;
	    }
	    if (time_expired(skt))
	    {
		g_set_error(error, SSIP_ERROR, SSIP_ERROR_TIME_RECV, SSIP_ERROR_TIME_RECV_MSG);
		g_printerr("[SSIP_NEG] ssip_accept: Error recv data packet, time expired ...\n");
		g_free(to_user);
		g_free(to_host);
		g_free(pkt_s);
		g_free(pkt_r);
		return -1;
	    }
	    tcp_recv(skt, pkt_r, sizeof(SsipPkt), 0);
	    g_set_error(error, SSIP_ERROR, SSIP_ERROR_CODEC, SSIP_ERROR_CODEC_MSG);
	    g_printerr("[SSIP_NEG] ssip_accept: Error: codec audio not defined");
	    g_free(to_user);
	    g_free(to_host);
	    g_free(pkt_s);
	    g_free(pkt_r);
	    return -1;
	}
	search = FALSE;
	int16 = (gint16 *) pkt_r->data;
	for (counter = 0; counter != len; counter++)
	{
            gint16 compare;
	    gint16 *pint16;
	    gint j;

	    g_mutex_lock(list_mutex);
            j = 0;
	    pn = (Node_listvocodecs *) g_slist_nth_data(list_codec, j);
	    while (pn)
	    {
                pint16 = int16;
		compare = g_ntohs(*pint16);
		if (pn->payload == compare)
		{
		    pint16++;
                    compare = g_ntohs(*pint16);
		    if (pn->ms == compare)
		    {
			search = TRUE;
			break;
		    }
		}
		pn = (Node_listvocodecs *) g_slist_nth_data(list_codec, ++j);
	    }
	    g_mutex_unlock(list_mutex);
	    if (search) break;
	    int16 += 4;
	}

	if (!search)
	{
	    set_pkt(pkt_s, from_user, from_host, from_port, to_user, to_host, to_port);
	    pkt_s->type = g_htonl(CONNECT_BYE);
	    pkt_s->data_len = g_htonl(0);
	    if (tcp_send(skt, pkt_s, sizeof(SsipPkt), 0) != sizeof(SsipPkt))
	    {
		g_set_error(error, SSIP_ERROR, SSIP_ERROR_SEND, SSIP_ERROR_SEND_MSG);
		g_printerr("[SSIP_NEG] ssip_accept: Error sending data packet\n");
		g_free(to_user);
		g_free(to_host);
		g_free(pkt_s);
		g_free(pkt_r);
		return -1;
	    }
	    if (time_expired(skt))
	    {
		g_set_error(error, SSIP_ERROR, SSIP_ERROR_TIME_RECV, SSIP_ERROR_TIME_RECV_MSG);
		g_printerr("[SSIP_NEG] ssip_accept: Error recv data packet, time expired ...\n");
		g_free(to_user);
		g_free(to_host);
		g_free(pkt_s);
		g_free(pkt_r);
		return -1;
	    }
	    tcp_recv(skt, pkt_r, sizeof(SsipPkt), 0);
	    g_set_error(error, SSIP_ERROR, SSIP_ERROR_CODEC, SSIP_ERROR_CODEC_MSG);
	    g_printerr("[SSIP_NEG] ssip_accept: Error: codec audio not defined");
	    g_free(to_user);
	    g_free(to_host);
	    g_free(pkt_s);
	    g_free(pkt_r);
	    return -1;
	}
	break;

    case CONNECT_BYE:
	set_pkt(pkt_s, from_user, from_host, from_port, to_user, to_host, to_port);
	pkt_s->type = g_htonl(CONNECT_BYE);
	pkt_s->data_len = g_htonl(0);
	if (tcp_send(skt, pkt_s, sizeof(SsipPkt), 0) != sizeof(SsipPkt))
	{
	    g_set_error(error, SSIP_ERROR, SSIP_ERROR_SEND, SSIP_ERROR_SEND_MSG);
	    g_printerr("[SSIP_NEG] ssip_accept: Error sending data packet\n");
	    g_free(to_user);
	    g_free(to_host);
	    g_free(pkt_s);
	    g_free(pkt_r);
	    return -1;
	}
	g_free(to_user);
	g_free(to_host);
	g_free(pkt_s);
	g_free(pkt_r);
        return 0;
	break;

    case CONNECT_ERROR:
    default:
	len = g_ntohl(pkt_r->data_len);
	if (PKT_USERDATA_LEN <= len) len = PKT_USERDATA_LEN - 1;
	pkt_r->data[len] = '\0';
	g_set_error(error, SSIP_ERROR, SSIP_ERROR_CONNECT, "%s %s", SSIP_ERROR_CONNECT_MSG, pkt_r->data);
	g_printerr("[SSIP_NEG] ssip_accept: RECV CONNECT_ERROR: %s\n", pkt_r->data);
	g_free(pkt_r);
	g_free(pkt_s);
	g_free(to_user);
	g_free(to_host);
	return -1;
	break;
    }

    set_pkt(pkt_s, from_user, from_host, from_port, to_user, to_host, to_port);
    pkt_s->type = g_htonl(CONNECT_CODECTX);

    memset(&(pkt_s->data), 0, PKT_USERDATA_LEN);

    g_mutex_lock(list_mutex);

    int16 = (gint16 *) pkt_s->data;
    *int16 = g_htons(pn->payload);
    int16++;
    *int16 = g_htons(pn->ms);
    int16++;
    *int16 = g_htons(pn->compresed_frame);
    int16++;
    *int16 = g_htons(pn->uncompresed_frame);

    g_mutex_unlock(list_mutex);

    pkt_s->data_len = g_htonl(1);

    if (tcp_send(skt, pkt_s, sizeof(SsipPkt), 0) != sizeof(SsipPkt))
    {
	g_set_error(error, SSIP_ERROR, SSIP_ERROR_SEND, SSIP_ERROR_SEND_MSG);
	g_printerr("[SSIP_NEG] ssip_accept: Error sending data packet\n");
	g_free(to_user);
	g_free(to_host);
	g_free(pkt_s);
	g_free(pkt_r);
        return -1;
    }

    if (time_expired(skt))
    {
	g_set_error(error, SSIP_ERROR, SSIP_ERROR_TIME_RECV, SSIP_ERROR_TIME_RECV_MSG);
	g_printerr("[SSIP_NEG] ssip_accept: Error recv data packet, time expired ...\n");
	g_free(to_user);
	g_free(to_host);
	g_free(pkt_s);
	g_free(pkt_r);
	return -1;
    }
    if (tcp_recv(skt, pkt_r, sizeof(SsipPkt), 0) != sizeof(SsipPkt))
    {
	g_set_error(error, SSIP_ERROR, SSIP_ERROR_RECV, SSIP_ERROR_RECV_MSG);
	g_printerr("[SSIP_NEG] ssip_accept: Error recv data packet: unknown format\n");
	g_free(pkt_s);
	g_free(pkt_r);
	g_free(to_user);
	g_free(to_host);
	return -1;
    }

    type = g_ntohl(pkt_r->type);
    switch (type)
    {
    case CONNECT_RTP:

	int16 = (gint16 *) pkt_r->data;
        *rtp_sel_port = *int16;
        *rtp_sel_port = g_ntohs(*rtp_sel_port);
	int16++;
	rtp_host = (gchar *) int16;
	len = pkt_s->data_len;
	if ((PKT_USERDATA_LEN - 2) <= len) len = (PKT_USERDATA_LEN - 2);
	_rtp_sel_host = g_strndup(rtp_host, len);

	break;

    case CONNECT_BYE:
	pkt_s->type = g_htonl(CONNECT_BYE);
	pkt_s->data_len = g_htonl(0);
	if (tcp_send(skt, pkt_s, sizeof(SsipPkt), 0) != sizeof(SsipPkt))
	{
	    g_set_error(error, SSIP_ERROR, SSIP_ERROR_SEND, SSIP_ERROR_SEND_MSG);
	    g_printerr("[SSIP_NEG] ssip_accept: Error sending data packet\n");
	    g_free(pkt_s);
	    g_free(pkt_r);
	    g_free(to_user);
	    g_free(to_host);
	    return -1;
	}
	g_free(pkt_s);
	g_free(pkt_r);
	g_free(to_user);
	g_free(to_host);
	return 0;
	break;

    case CONNECT_ERROR:
    default:
	len = g_ntohl(pkt_r->data_len);
	if (PKT_USERDATA_LEN <= len) len = PKT_USERDATA_LEN - 1;
	pkt_r->data[len] = 0;
	g_set_error(error, SSIP_ERROR, SSIP_ERROR_CONNECT, "%s %s", SSIP_ERROR_CONNECT_MSG, pkt_r->data);
	g_printerr("[SSIP_NEG] ssip_accept: RECV CONNECT_ERROR: %s\n", pkt_r->data);
	g_free(pkt_r);
	g_free(pkt_s);
	g_free(to_user);
	g_free(to_host);
	return -1;
	break;
    }

    set_pkt(pkt_s, from_user, from_host, from_port, to_user, to_host, to_port);
    pkt_s->type = g_htonl(CONNECT_RTP);

    int16 = (gint16 *) pkt_s->data;
    *int16 = g_htons(rtp_port);
    int16++;
    rtp_host = (gchar *) int16;
    len = strlen(from_real_host) + 1;
    if ((PKT_USERDATA_LEN - 2) <= len) len = (PKT_USERDATA_LEN - 2);
    g_strlcpy(rtp_host, from_real_host, len);
    len = len + 2;
    pkt_s->data_len = g_htonl(len);

    if (tcp_send(skt, pkt_s, sizeof(SsipPkt), 0) != sizeof(SsipPkt))
    {
	g_set_error(error, SSIP_ERROR, SSIP_ERROR_SEND, SSIP_ERROR_SEND_MSG);
	g_printerr("[SSIP_NEG] ssip_accept: Error sending data packet\n");
	g_free(pkt_s);
	g_free(pkt_r);
	g_free(to_user);
	g_free(to_host);
        return -1;
    }
    if (time_expired(skt))
    {
	g_set_error(error, SSIP_ERROR, SSIP_ERROR_TIME_RECV, SSIP_ERROR_TIME_RECV_MSG);
	g_printerr("[SSIP_NEG] ssip_accept: Error recv data packet, time expired ...\n");
	g_free(to_user);
	g_free(to_host);
	g_free(pkt_s);
	g_free(pkt_r);
	return -1;
    }
    if (tcp_recv(skt, pkt_r, sizeof(SsipPkt), 0) != sizeof(SsipPkt))
    {
	g_set_error(error, SSIP_ERROR, SSIP_ERROR_RECV, SSIP_ERROR_RECV_MSG);
	g_printerr("[SSIP_NEG] ssip_accept: Error recv data packet: unknown format\n");
	g_free(pkt_s);
	g_free(pkt_r);
	g_free(to_user);
	g_free(to_host);
	return -1;
    }

    type = g_ntohl(pkt_r->type);
    switch (type)
    {
    case CONNECT_BYE:

	break;
    case CONNECT_ERROR:
    default:
	len = g_ntohl(pkt_r->data_len);
	if (PKT_USERDATA_LEN <= len) len = PKT_USERDATA_LEN - 1;
	pkt_r->data[len] = 0;
	g_set_error(error, SSIP_ERROR, SSIP_ERROR_CONNECT, "%s %s", SSIP_ERROR_CONNECT_MSG, pkt_r->data);
	g_printerr("[SSIP_NEG] ssip_accept: RECV CONNECT_ERROR: %s\n", pkt_r->data);
	g_free(pkt_r);
	g_free(pkt_s);
	g_free(to_user);
	g_free(to_host);
        if (_rtp_sel_host != NULL) g_free(_rtp_sel_host);
	return -1;
	break;
    }

    set_pkt(pkt_s, from_user, from_host, from_port, to_user, to_host, to_port);
    pkt_s->type = g_htonl(CONNECT_BYE);
    pkt_s->data_len = g_htons(0);

    if (tcp_send(skt, pkt_s, sizeof(SsipPkt), 0) != sizeof(SsipPkt))
    {
	g_set_error(error, SSIP_ERROR, SSIP_ERROR_SEND, SSIP_ERROR_SEND_MSG);
	g_printerr("[SSIP_NEG] ssip_accept: Error sending data packet\n");
	g_free(pkt_s);
	g_free(pkt_r);
	g_free(to_user);
	g_free(to_host);
        if (_rtp_sel_host != NULL) g_free(_rtp_sel_host);
        return -1;
    }
    g_free(pkt_s);
    g_free(pkt_r);

    *rtp_sel_user = g_strdup(to_user);
    *rtp_sel_host = g_strdup(_rtp_sel_host);
    *sel_codec = pn;

    g_free(_rtp_sel_host);
    g_free(to_user);
    g_free(to_host);
    return 2;
}



/*
 * ssip_connect:
 *
 * return:
 * -1 : error;
 *  1 : ignore_user;
 *  0 : no user, bye;
 *  2 : no codec avaliable;
 *  3 : no RTP protocol;
 *  4 : OK
 *
 */

gint ssip_connect (gchar *from_user, gchar *from_host, guint16 from_port,
		   gchar *to_user, gchar *to_host, guint16 to_port, gchar *from_msg,
		   GSList *list_codec, GMutex *list_mutex, guint16 rtp_port, ConfigFile *conf, ProgressData *pd, GError **error,
		   gchar **rtp_sel_host, guint16 *rtp_sel_port, Node_listvocodecs **sel_codec)
{
    gint16 *int16;
    Node_listvocodecs *pn;
    Socket_tcp *skt;
    GError *tmp_error = NULL;
    gint type;
    gint len;
    SsipPkt *pkt_s;
    SsipPkt *pkt_r;
    gint counter;
    gchar *rtp_host;
    gchar *_rtp_sel_host;
    guint16 rtp_sel_codec;
    gboolean search;
    gint j;
    struct hostent *h;
    struct in_addr iaddr4;
#ifdef NET_HAVE_IPv6
    struct in6_addr iaddr6;
#endif
    gchar *from_real_host;


    //gtk_label_set_markup(GTK_LABEL(pd->label), "<span color=\"blue\"><big><b>Iniciando conexion de negociacion SSIP ...</b></big></span>");

    skt = tcp_init(PROGRAM_LOCALHOST, NULL, 0, to_port, 15, &tmp_error);
    if (tmp_error != NULL)
    {
        g_propagate_error(error, tmp_error);
        return -1;
    }

    h = gethostbyname(to_host);
    if (h == NULL)
    {
	g_set_error(error, SSIP_ERROR, SSIP_ERROR_UNRESOLVEIP, "imposible determinar direccion IP de %s", to_host);
	g_printerr("[SSIP_NEG] ssip_connect: can't resolve IP address for %s\n", to_host);
        return -1;
    }

    if (strchr(h->h_addr, ':') != NULL)
    {
#ifdef NET_HAVE_IPv6

	memcpy(&iaddr6.s_addr, h->h_addr, sizeof(iaddr6.s_addr));
	to_host = g_malloc0(INET6_ADDRSTRLEN + 1);
	inet_ntop(AF_INET6, &iaddr6, to_host, INET6_ADDRSTRLEN);
	to_host[INET6_ADDRSTRLEN] = 0;
#endif
    }
    else
    {
	memcpy(&iaddr4.s_addr, h->h_addr, sizeof(iaddr4.s_addr));
	to_host = g_malloc0(INET_ADDRSTRLEN + 1);
	inet_ntop(AF_INET, &iaddr4, to_host, INET_ADDRSTRLEN);
	to_host[INET_ADDRSTRLEN] = 0;
    }

    tcp_connect(skt, to_host, to_port, &tmp_error);
    if (tmp_error != NULL)
    {
        g_propagate_error(error, tmp_error);
	tcp_exit(skt, NULL);
        g_free(to_host);
        return -1;
    }
    //gtk_label_set_markup(GTK_LABEL(pd->label), "<span color=\"blue\"><big><b>Conexion iniciada, transfiriendo datos ...</b></big></span>");

    pkt_s = (SsipPkt *) g_malloc0(sizeof(SsipPkt));
    pkt_r = (SsipPkt *) g_malloc0(sizeof(SsipPkt));

    if (!cfgf_read_string(conf, PROGRAM_GLOBAL_SECTION, PROGRAM_HOSTNAME, &from_real_host))
    {
	g_printerr("[SSIP_NEG] ssip_connect: Error getting local hostname.\n");
	from_real_host = tcp_host_addr(skt, &tmp_error);
	if (tmp_error != NULL)
	{
	    g_propagate_error(error, tmp_error);
	    tcp_exit(skt, NULL);
	    g_free(to_host);
	    g_free(pkt_s);
	    g_free(pkt_r);
	    return -1;
	}
    }

    set_pkt(pkt_s, from_user, from_host, from_port, to_user, to_host, to_port);
    pkt_s->type = g_htonl(CONNECT_HELO);
    len = strlen(from_msg) + 1;
    if (PKT_USERDATA_LEN <= len) len = PKT_USERDATA_LEN;
    g_strlcpy(pkt_s->data, from_msg, len);
    pkt_s->data_len = g_htonl(len);

    if (tcp_send(skt, pkt_s, sizeof(SsipPkt), 0) != sizeof(SsipPkt))
    {
	g_set_error(error, SSIP_ERROR, SSIP_ERROR_SEND, SSIP_ERROR_SEND_MSG);
	g_printerr("[SSIP_NEG] ssip_connect: Error sending data packet\n");
	tcp_exit(skt, NULL);
	g_free(from_real_host);
        g_free(to_host);
	g_free(pkt_s);
	g_free(pkt_r);
        return -1;
    }

    //gtk_label_set_markup(GTK_LABEL(pd->label), "<span color=\"blue\"><big><b>Esperando aceptacion del usuario ...</b></big></span>");
    do
    {
	if (*(pd->exit) == 0)
	{
	    set_pkt(pkt_s, from_user, from_host, from_port, to_user, to_host, to_port);
	    pkt_s->type = g_htonl(CONNECT_ERROR);
	    pkt_s->data_len = strlen(SSIP_CONNECT_INTERRUPT_MSG) + 1;
	    g_strlcpy(pkt_s->data, SSIP_CONNECT_INTERRUPT_MSG, pkt_s->data_len);
	    pkt_s->data[pkt_s->data_len - 1] = 0;
            pkt_s->data_len = g_htonl(pkt_s->data_len);
	    tcp_send(skt, pkt_s, sizeof(SsipPkt), 0);
	    tcp_exit(skt, NULL);
	    g_free(from_real_host);
	    g_free(to_host);
	    g_free(pkt_s);
	    g_free(pkt_r);
            return -3;
	}
    }
    while (!time_expired_mtime(skt, 500000));

    if (tcp_recv(skt, pkt_r, sizeof(SsipPkt), 0) != sizeof(SsipPkt))
    {
	g_set_error(error, SSIP_ERROR, SSIP_ERROR_RECV, SSIP_ERROR_RECV_MSG);
	g_printerr("[SSIP_NEG] ssip_connect: Error recv data packet: unknown format\n");
	tcp_exit(skt, NULL);
	g_free(from_real_host);
        g_free(to_host);
	g_free(pkt_s);
	g_free(pkt_r);
	return -1;
    }

    //gtk_label_set_markup(GTK_LABEL(pd->label), "<span color=\"blue\"><big><b>Recibiendo contestacion del usuario ...</b></big></span>");
    type = g_ntohl(pkt_r->type);
    switch (type)
    {
    case CONNECT_IGNOREUSER:

	set_pkt(pkt_s, from_user, from_host, from_port, to_user, to_host, to_port);
	pkt_s->type = g_htonl(CONNECT_BYE);
	pkt_s->data_len = g_htonl(0);
	if (tcp_send(skt, pkt_s, sizeof(SsipPkt), 0) != sizeof(SsipPkt))
	{
	    g_set_error(error, SSIP_ERROR, SSIP_ERROR_SEND, SSIP_ERROR_SEND_MSG);
	    g_printerr("[SSIP_NEG] ssip_connect: Error sending data packet\n");
	    tcp_exit(skt, NULL);
	    g_free(from_real_host);
	    g_free(to_host);
	    g_free(pkt_s);
	    g_free(pkt_r);
	    return -1;
	}

	if (!time_expired(skt))
	{
	    if (tcp_recv(skt, pkt_r, sizeof(SsipPkt), 0) != sizeof(SsipPkt))
	    {
		g_set_error(error, SSIP_ERROR, SSIP_ERROR_RECV, SSIP_ERROR_RECV_MSG);
		g_printerr("[SSIP_NEG] ssip_connect: Error recv data packet: unknown format\n");
		tcp_exit(skt, NULL);
		g_free(from_real_host);
		g_free(to_host);
		g_free(pkt_s);
		g_free(pkt_r);
		return -1;
	    }
	}
	tcp_exit(skt, NULL);
	g_free(from_real_host);
        g_free(to_host);
	g_free(pkt_s);
	g_free(pkt_r);
	return 1;
	break;

    case CONNECT_VALIDUSER:

	//gtk_label_set_markup(GTK_LABEL(pd->label), "<span color=\"blue\"><big><b>El usuario acepta la conexion ...</b></big></span>");
        // ok, continue
        break;
    case CONNECT_BYE:

	set_pkt(pkt_s, from_user, from_host, from_port, to_user, to_host, to_port);
	pkt_s->type = g_htonl(CONNECT_BYE);
	pkt_s->data_len = g_htonl(0);
	if (tcp_send(skt, pkt_s, sizeof(SsipPkt), 0) != sizeof(SsipPkt))
	{
	    g_set_error(error, SSIP_ERROR, SSIP_ERROR_SEND, SSIP_ERROR_SEND_MSG);
	    g_printerr("[SSIP_NEG] ssip_connect: Error sending data packet\n");
	    tcp_exit(skt, NULL);
	    g_free(from_real_host);
	    g_free(to_host);
	    g_free(pkt_s);
	    g_free(pkt_r);
	    return -1;
	}
	tcp_exit(skt, NULL);
	g_free(from_real_host);
        g_free(to_host);
	g_free(pkt_s);
	g_free(pkt_r);
	return 0;
	break;

    case CONNECT_ERROR:
    default:
	len = g_ntohl(pkt_r->data_len);
	if (PKT_USERDATA_LEN <= len) len = PKT_USERDATA_LEN - 1;
	pkt_r->data[len] = 0;
	g_set_error(error, SSIP_ERROR, SSIP_ERROR_CONNECT, "%s %s", SSIP_ERROR_CONNECT_MSG, pkt_r->data);
	g_printerr("[SSIP_NEG] ssip_connect: RECV CONNECT_ERROR: %s\n", pkt_r->data);
	tcp_exit(skt, NULL);
	g_free(from_real_host);
        g_free(to_host);
	g_free(pkt_s);
	g_free(pkt_r);
	return -1;
	break;
    }

    set_pkt(pkt_s, from_user, from_host, from_port, to_user, to_host, to_port);
    pkt_s->type = g_htonl(CONNECT_CODECTX);
    memset(&(pkt_s->data), 0, PKT_USERDATA_LEN);
    int16 = (gint16 *) pkt_s->data;
    counter = 0;
    // ojo con la logitud (SI ES MAYOR QUE 64 :-)

    g_mutex_lock(list_mutex);

    pn = (Node_listvocodecs *) g_slist_nth_data(list_codec, counter);
    while (pn)
    {
	*int16 = g_htons(pn->payload);
        int16++;
	*int16 = g_htons(pn->ms);
        int16++;
	*int16 = g_htons(pn->compresed_frame);
        int16++;
	*int16 = g_htons(pn->uncompresed_frame);
        int16++;
	counter++;
	pn = (Node_listvocodecs *) g_slist_nth_data(list_codec, counter);
    }
    g_mutex_unlock(list_mutex);

    pkt_s->data_len = g_htonl(counter);
    if (counter == 0)
    {
	set_pkt(pkt_s, from_user, from_host, from_port, to_user, to_host, to_port);
	pkt_s->type = g_htonl(CONNECT_BYE);
	pkt_s->data_len = g_htonl(0);
	if (tcp_send(skt, pkt_s, sizeof(SsipPkt), 0) != sizeof(SsipPkt))
	{
	    g_set_error(error, SSIP_ERROR, SSIP_ERROR_SEND, SSIP_ERROR_SEND_MSG);
	    g_printerr("[SSIP_NEG] ssip_connect: Error sending data packet\n");
	    tcp_exit(skt, NULL);
	    g_free(from_real_host);
	    g_free(to_host);
	    g_free(pkt_s);
	    g_free(pkt_r);
	    return -1;
	}
        if (!time_expired(skt)) tcp_recv(skt, pkt_r, sizeof(SsipPkt), 0);
	g_set_error(error, SSIP_ERROR, SSIP_ERROR_CODEC, SSIP_ERROR_CODEC_MSG);
	g_printerr("[SSIP_NEG] ssip_connect: Error: codec audio not defined");
	tcp_exit(skt, NULL);
	g_free(from_real_host);
	g_free(to_host);
	g_free(pkt_s);
	g_free(pkt_r);
	return -1;
    }
    if (tcp_send(skt, pkt_s, sizeof(SsipPkt), 0) != sizeof(SsipPkt))
    {
	g_set_error(error, SSIP_ERROR, SSIP_ERROR_SEND, SSIP_ERROR_SEND_MSG);
	g_printerr("[SSIP_NEG] ssip_connect: Error sending data packet\n");
	tcp_exit(skt, NULL);
	g_free(from_real_host);
	g_free(to_host);
	g_free(pkt_s);
	g_free(pkt_r);
        return -1;
    }

    //gtk_label_set_markup(GTK_LABEL(pd->label), "<span color=\"blue\"><big><b>Enviando lista de codecs disponibles ... </b></big></span>");
    do
    {
	if (*(pd->exit) == 0)
	{
	    set_pkt(pkt_s, from_user, from_host, from_port, to_user, to_host, to_port);
	    pkt_s->type = g_htonl(CONNECT_ERROR);
	    pkt_s->data_len = strlen(SSIP_CONNECT_INTERRUPT_MSG) + 1;
	    g_strlcpy(pkt_s->data, SSIP_CONNECT_INTERRUPT_MSG, pkt_s->data_len);
	    pkt_s->data[pkt_s->data_len - 1] = 0;
            pkt_s->data_len = g_htonl(pkt_s->data_len);
	    tcp_send(skt, pkt_s, sizeof(SsipPkt), 0);
	    tcp_exit(skt, NULL);
	    g_free(from_real_host);
	    g_free(to_host);
	    g_free(pkt_s);
	    g_free(pkt_r);
            return -3;
	}
    }
    while (!time_expired_mtime(skt, 500000));

    if (tcp_recv(skt, pkt_r, sizeof(SsipPkt), 0) != sizeof(SsipPkt))
    {
	g_set_error(error, SSIP_ERROR, SSIP_ERROR_RECV, SSIP_ERROR_RECV_MSG);
	g_printerr("[SSIP_NEG] ssip_connect: Error recv data packet: unknown format\n");
	tcp_exit(skt, NULL);
	g_free(from_real_host);
	g_free(to_host);
	g_free(pkt_s);
	g_free(pkt_r);
	return -1;
    }

    //gtk_label_set_markup(GTK_LABEL(pd->label), "<span color=\"blue\"><big><b>Validando lista de codecs recibidos ... </b></big></span>");
    type = g_ntohl(pkt_r->type);
    switch (type)
    {
    case CONNECT_CODECTX:

	len = pkt_r->data_len;
	len = g_ntohl(len);
	search = FALSE;
	if (len > 0)
	{
	    int16 = (gint16 *) pkt_r->data;
	    rtp_sel_codec = *int16;
	    rtp_sel_codec = g_ntohs(rtp_sel_codec);

	    g_mutex_lock(list_mutex);

	    j = 0;
	    pn = (Node_listvocodecs *) g_slist_nth_data(list_codec, j);
	    while (pn)
	    {
		if (pn->payload == rtp_sel_codec)
		{
		    search = TRUE;
		    break;
		}
		pn = (Node_listvocodecs *) g_slist_nth_data(list_codec, ++j);
	    }
	    g_mutex_unlock(list_mutex);
	}
	if ((len < 1) || (!search))
	{
	    set_pkt(pkt_s, from_user, from_host, from_port, to_user, to_host, to_port);
	    pkt_s->type = g_htonl(CONNECT_BYE);
	    pkt_s->data_len = g_htonl(0);
	    if (tcp_send(skt, pkt_s, sizeof(SsipPkt), 0) != sizeof(SsipPkt))
	    {
		g_set_error(error, SSIP_ERROR, SSIP_ERROR_SEND, SSIP_ERROR_SEND_MSG);
		g_printerr("[SSIP_NEG] ssip_connect: Error sending data packet\n");
		tcp_exit(skt, NULL);
		g_free(from_real_host);
		g_free(to_host);
		g_free(pkt_s);
		g_free(pkt_r);
		return -1;
	    }
	    if (!time_expired(skt)) tcp_recv(skt, pkt_r, sizeof(SsipPkt), 0);
	    g_set_error(error, SSIP_ERROR, SSIP_ERROR_CODEC, SSIP_ERROR_CODEC_MSG);
	    g_printerr("[SSIP_NEG] ssip_connect: Error: codec audio not defined");
	    tcp_exit(skt, NULL);
	    g_free(from_real_host);
	    g_free(to_host);
	    g_free(pkt_s);
	    g_free(pkt_r);
	    return -1;
	}
	break;

    case CONNECT_BYE:

	set_pkt(pkt_s, from_user, from_host, from_port, to_user, to_host, to_port);
	pkt_s->type = g_htonl(CONNECT_BYE);
	pkt_s->data_len = g_htonl(0);
	if (tcp_send(skt, pkt_s, sizeof(SsipPkt), 0) != sizeof(SsipPkt))
	{
	    g_set_error(error, SSIP_ERROR, SSIP_ERROR_SEND, SSIP_ERROR_SEND_MSG);
	    g_printerr("[SSIP_NEG] ssip_connect: Error sending data packet\n");
	    tcp_exit(skt, NULL);
	    g_free(from_real_host);
	    g_free(to_host);
	    g_free(pkt_s);
	    g_free(pkt_r);
	    return -1;
	}
	tcp_exit(skt, NULL);
	g_free(from_real_host);
	g_free(to_host);
	g_free(pkt_s);
	g_free(pkt_r);
        return 2;
	break;

    case CONNECT_ERROR:
    default:
	len = g_ntohl(pkt_r->data_len);
	if (PKT_USERDATA_LEN <= len) len = PKT_USERDATA_LEN -1;
	pkt_r->data[len] = 0;
	g_set_error(error, SSIP_ERROR, SSIP_ERROR_CONNECT, "%s %s", SSIP_ERROR_CONNECT_MSG, pkt_r->data);
	g_printerr("[SSIP_NEG] ssip_connect: RECV CONNECT_ERROR: %s\n", pkt_r->data);
	tcp_exit(skt, NULL);
	g_free(from_real_host);
	g_free(to_host);
	g_free(pkt_s);
	g_free(pkt_r);
	return -1;
	break;
    }

    set_pkt(pkt_s, from_user, from_host, from_port, to_user, to_host, to_port);
    pkt_s->type = g_htonl(CONNECT_RTP);

    int16 = (gint16 *) pkt_s->data;
    *int16 = g_htons(rtp_port);

    int16++;
    rtp_host = (gchar *) int16;
    len = strlen(from_real_host) + 1;
    if ((PKT_USERDATA_LEN - 2) <= len) len = (PKT_USERDATA_LEN - 2);
    g_strlcpy(rtp_host, from_real_host, len);
    len = len + 2;
    pkt_s->data_len = g_htonl(len);

    //gtk_label_set_markup(GTK_LABEL(pd->label), "<span color=\"blue\"><big><b>Enviando parametros RTP/RTCP ... </b></big></span>");

    if (tcp_send(skt, pkt_s, sizeof(SsipPkt), 0) != sizeof(SsipPkt))
    {
	g_set_error(error, SSIP_ERROR, SSIP_ERROR_SEND, SSIP_ERROR_SEND_MSG);
	g_printerr("[SSIP_NEG] ssip_connect: Error sending data packet\n");
	tcp_exit(skt, NULL);
	g_free(to_host);
	g_free(pkt_s);
        g_free(from_real_host);
	g_free(pkt_r);
        return -1;
    }

    do
    {
	if (*(pd->exit) == 0)
	{
	    set_pkt(pkt_s, from_user, from_host, from_port, to_user, to_host, to_port);
	    pkt_s->type = g_htonl(CONNECT_ERROR);
	    pkt_s->data_len = strlen(SSIP_CONNECT_INTERRUPT_MSG) + 1;
	    g_strlcpy(pkt_s->data, SSIP_CONNECT_INTERRUPT_MSG, pkt_s->data_len);
	    pkt_s->data[pkt_s->data_len - 1] = 0;
            pkt_s->data_len = g_htonl(pkt_s->data_len);
	    tcp_send(skt, pkt_s, sizeof(SsipPkt), 0);
	    tcp_exit(skt, NULL);
	    g_free(from_real_host);
	    g_free(to_host);
	    g_free(pkt_s);
	    g_free(pkt_r);
            return -3;
	}
    }
    while (!time_expired_mtime(skt, 500000));

    if (tcp_recv(skt, pkt_r, sizeof(SsipPkt), 0) != sizeof(SsipPkt))
    {
	g_set_error(error, SSIP_ERROR, SSIP_ERROR_RECV, SSIP_ERROR_RECV_MSG);
	g_printerr("[SSIP_NEG] ssip_connect: Error recv data packet: unknown format\n");
	tcp_exit(skt, NULL);
	g_free(from_real_host);
	g_free(to_host);
	g_free(pkt_s);
	g_free(pkt_r);
	return -1;
    }

    type = g_ntohl(pkt_r->type);
    switch (type)
    {
    case CONNECT_RTP:

	int16 = (gint16 *) pkt_r->data;
        *rtp_sel_port = *int16;
        *rtp_sel_port = g_ntohs(*rtp_sel_port);
	int16++;
	rtp_host = (gchar *) int16;
	len = pkt_s->data_len;
	if ((PKT_USERDATA_LEN - 2) <= len) len = (PKT_USERDATA_LEN - 2);
	_rtp_sel_host = g_strndup(rtp_host, len);
	break;

    case CONNECT_BYE:

	set_pkt(pkt_s, from_user, from_host, from_port, to_user, to_host, to_port);
	pkt_s->type = g_htonl(CONNECT_BYE);
	pkt_s->data_len = g_htonl(0);
	if (tcp_send(skt, pkt_s, sizeof(SsipPkt), 0) != sizeof(SsipPkt))
	{
	    g_set_error(error, SSIP_ERROR, SSIP_ERROR_SEND, SSIP_ERROR_SEND_MSG);
	    g_printerr("[SSIP_NEG] ssip_connect: Error sending data packet\n");
	    tcp_exit(skt, NULL);
	    g_free(to_host);
	    g_free(from_real_host);
	    g_free(pkt_s);
	    g_free(pkt_r);
	    return -1;
	}
	tcp_exit(skt, NULL);
	g_free(from_real_host);
	g_free(to_host);
	g_free(pkt_s);
	g_free(pkt_r);
        return 3;
        break;

    case CONNECT_ERROR:
    default:

	len = g_ntohl(pkt_r->data_len);
	if (PKT_USERDATA_LEN <= len) len = PKT_USERDATA_LEN - 1;
	pkt_r->data[len] = 0;
	g_set_error(error, SSIP_ERROR, SSIP_ERROR_CONNECT, "%s %s", SSIP_ERROR_CONNECT_MSG, pkt_r->data);
	g_printerr("[SSIP_NEG] ssip_connect: RECV CONNECT_ERROR: %s\n", pkt_r->data);
	tcp_exit(skt, NULL);
	g_free(to_host);
        g_free(from_real_host);
	g_free(pkt_s);
	g_free(pkt_r);
	return -1;
	break;
    }
    //gtk_label_set_markup(GTK_LABEL(pd->label), "<span color=\"blue\"><big><b>Parametros RTP/RTCP remotos recibidos ... </b></big></span>");

    set_pkt(pkt_s, from_user, from_host, from_port, to_user, to_host, to_port);
    pkt_s->type = g_htonl(CONNECT_BYE);
    pkt_s->data_len = g_htons(0);

    if (tcp_send(skt, pkt_s, sizeof(SsipPkt), 0) != sizeof(SsipPkt))
    {
	g_set_error(error, SSIP_ERROR, SSIP_ERROR_SEND, SSIP_ERROR_SEND_MSG);
	g_printerr("[SSIP_NEG] ssip_connect: Error sending data packet\n");
	tcp_exit(skt, NULL);
	g_free(to_host);
        g_free(from_real_host);
	g_free(pkt_s);
	g_free(pkt_r);
        if (_rtp_sel_host != NULL) g_free(_rtp_sel_host);
        return -1;
    }

    do
    {
	if (*(pd->exit) == 0)
	{
	    set_pkt(pkt_s, from_user, from_host, from_port, to_user, to_host, to_port);
	    pkt_s->type = g_htonl(CONNECT_ERROR);
	    pkt_s->data_len = strlen(SSIP_CONNECT_INTERRUPT_MSG) + 1;
	    g_strlcpy(pkt_s->data, SSIP_CONNECT_INTERRUPT_MSG, pkt_s->data_len);
	    pkt_s->data[pkt_s->data_len - 1] = 0;
            pkt_s->data_len = g_htonl(pkt_s->data_len);
	    tcp_send(skt, pkt_s, sizeof(SsipPkt), 0);
	    tcp_exit(skt, NULL);
	    g_free(from_real_host);
	    if (_rtp_sel_host != NULL) g_free(_rtp_sel_host);
	    g_free(to_host);
	    g_free(pkt_s);
	    g_free(pkt_r);
            return -3;
	}
    }
    while (!time_expired_mtime(skt, 500000));

    if (tcp_recv(skt, pkt_r, sizeof(SsipPkt), 0) != sizeof(SsipPkt))
    {
	g_set_error(error, SSIP_ERROR, SSIP_ERROR_RECV, SSIP_ERROR_RECV_MSG);
	g_printerr("[SSIP_NEG] ssip_connect: Error recv data packet: unknown format\n");
	tcp_exit(skt, NULL);
	g_free(from_real_host);
	g_free(to_host);
	g_free(pkt_s);
	g_free(pkt_r);
	if (_rtp_sel_host != NULL) g_free(_rtp_sel_host);
	sel_codec = NULL;
	return -1;
    }
    //gtk_label_set_markup(GTK_LABEL(pd->label), "<span color=\"blue\"><big><b>OK, negociacion SSIP finalizada </b></big></span>");

    *rtp_sel_host = g_strdup(_rtp_sel_host);
    *sel_codec = pn;

    tcp_exit(skt, NULL);
    g_free(from_real_host);
    g_free(to_host);
    g_free(pkt_s);
    g_free(pkt_r);
    g_free(_rtp_sel_host);

    return 4;
}


