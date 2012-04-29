
/*
 *
 * FILE: notify.c
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

#include "notify.h"




gint notify_online_to (gchar *user, gchar *clave, guint16 ntf_port, gchar *to_user, gchar *to_host, guint16 to_port, GError **error)
{
    Socket_tcp *skt;
    GError *tmp_error = NULL;
    NtfPkt *pkt;
    gint fd;
    gchar *this_host;
    gint type;
    gint len;
    fd_set rfds;
    struct timeval tv;
    gint valret;


    skt = tcp_init(PROGRAM_LOCALHOST, NULL, 0, to_port, 15, &tmp_error);
    if (tmp_error != NULL)
    {
        g_propagate_error(error, tmp_error);
        return -1;
    }
    tcp_connect(skt, to_host, to_port, &tmp_error);
    if (tmp_error != NULL)
    {
        g_propagate_error(error, tmp_error);
        tcp_exit(skt, NULL);
        return -1;
    }

    pkt = (NtfPkt *) g_malloc0(sizeof(NtfPkt));

#ifdef NET_HAVE_IPv6
    pkt->ipmode = (skt->mode == IPv4) ? IPv4 : IPv6;
#else
    pkt->ipmode = IPv4;
#endif

    len = strlen(user) + 1;
    if (PKT_USERNAME_LEN <= len) len = PKT_USERNAME_LEN;
    g_strlcpy(pkt->from_user, user, len);
    this_host = tcp_host_addr(skt, NULL);
    inet_pton(AF_INET, this_host, &pkt->from_addr4);
#ifdef NET_HAVE_IPv6
    inet_pton(AF_INET6, this_host, &pkt->from_addr6);
#endif
    pkt->from_port = g_htons(ntf_port);
    g_free(this_host);

    len = strlen(to_user) + 1;
    if (PKT_USERNAME_LEN <= len) len = PKT_USERNAME_LEN;
    g_strlcpy(pkt->to_user, to_user, len);
    g_memmove(&(pkt->to_addr4), &skt->addr4, sizeof(struct in_addr));
#ifdef NET_HAVE_IPv6
    g_memmove(&(pkt->to_addr6), &skt->addr6, sizeof(struct in6_addr));
#endif
    pkt->to_port = g_htons(tcp_txport(skt));

    len = strlen(clave) + 1;
    if (PKT_USERDES_LEN <= len) len = PKT_USERDES_LEN;
    g_strlcpy(pkt->data, clave, len);
    pkt->type = g_htonl(USER_HELO_ONLINE);

    if (tcp_send(skt, pkt, sizeof(NtfPkt), 0) != sizeof(NtfPkt))
    {
	g_set_error(error, NOTIFY_ERROR, NOTIFY_ERROR_SEND, "error sending data packet");
	g_printerr("[NOTIFY] notify_offline_to: Error sending data packet\n");
	tcp_exit(skt, NULL);
	g_free(pkt);
        return -1;
    }

    fd = tcp_fd(skt);
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    // wait 5 s, for response
    tv.tv_sec = NOTIFY_TIME_EXPIRED;
    tv.tv_usec = 0;
    valret = select(fd+1, &rfds, NULL, NULL, &tv);
    FD_CLR(fd, &rfds);
    FD_ZERO(&rfds);

    if (valret)
    {
	if (tcp_recv(skt, pkt, sizeof(NtfPkt), 0) != sizeof(NtfPkt))
	{
	    g_set_error(error, NOTIFY_ERROR, NOTIFY_ERROR_RECV, "error recv data packet: unknown format");
	    g_printerr("[NOTIFY] notify_offline_to: Error recv data packet: unknown format\n");
	    g_free(pkt);
	    tcp_exit(skt, NULL);
	    return -1;
	}
	type = g_ntohl(pkt->type);
	g_free(pkt);
	tcp_exit(skt, NULL);

	switch (type)
	{
	case USER_OK_ONLINE:
            return 1;
            break;
	case USER_INCORRECT:
            return 0;
            break;
	default:
            return 2;
	    break;
	}
    }
    else
    {
	g_set_error(error, NOTIFY_ERROR, NOTIFY_ERROR_RECV, "error recv data packet, time expired ...");
	g_printerr("[NOTIFY] notify_offline_to: Error recv data packet, time expired ...\n");
	tcp_exit(skt, NULL);
	g_free(pkt);
	return -1;
    }
}



gint notify_offline_to (gchar *user, gchar *clave, gchar *to_user, gchar *to_host, guint16 to_port, GError **error)
{
    Socket_tcp *skt;
    GError *tmp_error = NULL;
    NtfPkt *pkt;
    gint fd;
    gchar *this_host;
    gint type;
    gint len;
    fd_set rfds;
    struct timeval tv;
    gint valret;


    skt = tcp_init(PROGRAM_LOCALHOST, NULL, 0, to_port, 15, &tmp_error);
    if (tmp_error != NULL)
    {
        g_propagate_error(error, tmp_error);
        return -1;
    }
    tcp_connect(skt, to_host, to_port, &tmp_error);
    if (tmp_error != NULL)
    {
        g_propagate_error(error, tmp_error);
        return -1;
    }

    pkt = (NtfPkt *) g_malloc0(sizeof(NtfPkt));

#ifdef NET_HAVE_IPv6
    pkt->ipmode = (skt->mode == IPv4) ? IPv4 : IPv6;
#else
    pkt->ipmode = IPv4;
#endif

    len = strlen(user) + 1;
    if (PKT_USERNAME_LEN <= len) len = PKT_USERNAME_LEN;
    g_strlcpy(pkt->from_user, user, len);
    this_host = tcp_host_addr(skt, NULL);
    inet_pton(AF_INET, this_host, &pkt->from_addr4);
#ifdef NET_HAVE_IPv6
    inet_pton(AF_INET6, this_host, &pkt->from_addr6);
#endif
    pkt->from_port = g_htons(0);
    g_free(this_host);

    len = strlen(to_user) + 1;
    if (PKT_USERNAME_LEN <= len) len = PKT_USERNAME_LEN;
    g_strlcpy(pkt->to_user, to_user, len);
    g_memmove(&(pkt->to_addr4), &skt->addr4, sizeof(struct in_addr));
#ifdef NET_HAVE_IPv6
    g_memmove(&(pkt->to_addr6), &skt->addr6, sizeof(struct in6_addr));
#endif
    pkt->to_port = g_htons(tcp_txport(skt));

    len = strlen(clave) + 1;
    if (PKT_USERDES_LEN <= len) len = PKT_USERDES_LEN;
    g_strlcpy(pkt->data, clave, len);
    pkt->type = g_htonl(USER_BYE_ONLINE);

    if (tcp_send(skt, pkt, sizeof(NtfPkt), 0) != sizeof(NtfPkt))
    {
	g_set_error(error, NOTIFY_ERROR, NOTIFY_ERROR_SEND, "error sending data packet");
	g_printerr("[NOTIFY] notify_offline_to: Error sending data packet\n");
	tcp_exit(skt, NULL);
	g_free(pkt);
        return -1;
    }

    fd = tcp_fd(skt);
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    // wait 5 s, for response
    tv.tv_sec = NOTIFY_TIME_EXPIRED;
    tv.tv_usec = 0;
    valret = select(fd+1, &rfds, NULL, NULL, &tv);
    FD_CLR(fd, &rfds);
    FD_ZERO(&rfds);

    if (valret)
    {
	if (tcp_recv(skt, pkt, sizeof(NtfPkt), 0) != sizeof(NtfPkt))
	{
	    g_set_error(error, NOTIFY_ERROR, NOTIFY_ERROR_RECV, "error recv data packet: unknown format");
	    g_printerr("[NOTIFY] notify_offline_to: Error recv data packet: unknown format\n");
	    g_free(pkt);
	    tcp_exit(skt, NULL);
	    return -1;
	}
	type = g_ntohl(pkt->type);
	g_free(pkt);
	tcp_exit(skt, NULL);

	switch (type)
	{
	case USER_OK_OFFLINE:
            return 1;
            break;
	case USER_INCORRECT:
            return 0;
            break;
	default:
            return 2;
	    break;
	}
    }
    else
    {
	g_set_error(error, NOTIFY_ERROR, NOTIFY_ERROR_RECV, "error recv data packet, time expired ...");
	g_printerr("[NOTIFY] notify_offline_to: Error recv data packet, time expired ...\n");
	tcp_exit(skt, NULL);
	g_free(pkt);
	return -1;
    }
}


