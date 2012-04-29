
/*
 *
 * FILE: tcp.h
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

#ifndef _TCP_H_
#define _TCP_H_


#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <glib.h>
#include "net.h"



#ifdef NET_HAVE_IPv6
#define TCP_HAVE_IPv6
#endif




struct _Socket_tcp
{
    gint            mode;   /* IPv4 or IPv6 */
    gchar           *addr;
    guint16         rx_port;
    guint16         tx_port;
    ttl_t           ttl;
    fd_t            fd;
    struct in_addr  addr4;
#ifdef TCP_HAVE_IPv6
    struct in6_addr addr6;
#endif 
};

typedef struct _Socket_tcp Socket_tcp;



#define TCP_ERROR                200
#define TCP_ERROR_ERRNO          201
#define TCP_ERROR_UNRESOLVEIP    202
#define TCP_ERROR_NOIFACE        203
#define TCP_ERROR_NOTIMPLEMENTED 204
#define TCP_ERROR_NULL           205




#define TCP_MAXHOSTNAMELEN    255
//#define TCP_SETBUFFERSIZE   1048576
#define TCP_SOREUSEADDR
//#define TCP_SOREUSEPORT



Socket_tcp *tcp_init (const gchar *addr, const gchar *iface, guint16 rx_port, guint16 tx_port, gint ttl, GError **error);
void tcp_exit (Socket_tcp *s, GError **error);

gboolean tcp_connect (Socket_tcp *s, const gchar *addr, guint16 port, GError **error);
Socket_tcp *tcp_accept (Socket_tcp *s, GError **error);

gint tcp_send (Socket_tcp *s, gpointer buffer, gint buflen, gint flags);
inline gint tcp_recv (Socket_tcp *s, gpointer buffer, gint buflen, gint flags);

gchar *tcp_host_addr (Socket_tcp *s, GError **error);

gint tcp_fd (Socket_tcp *s);
guint16 tcp_txport (Socket_tcp *s);
guint16 tcp_rxport (Socket_tcp *s);
gchar *tcp_addr (Socket_tcp *s);


#endif


