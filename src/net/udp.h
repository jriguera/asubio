
/*
 *
 * FILE: udp.h
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
 * This file is based on 'net_udp.c'
 *
 * AUTHOR:   Colin Perkins 
 * MODIFIED: Orion Hodson & Piers O'Hanlon
 * 
 * Copyright (c) 1998-2000 University College London
 * All rights reserved.
 *
 ***************************************************************************/

#ifndef _UDP_H_
#define _UDP_H_


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
#define UDP_HAVE_IPv6
#endif



#define UDP_HEADER_SIZE 8




struct _Socket_udp
{
    gint            mode;   /* IPv4 or IPv6 */
    gchar           *addr;
    guint16         rx_port;
    guint16         tx_port;
    ttl_t           ttl;
    fd_t            fd;
    struct in_addr  addr4;
#ifdef UDP_HAVE_IPv6
    struct in6_addr addr6;
#endif 
};

typedef struct _Socket_udp Socket_udp;



#define UDP_ERROR                200
#define UDP_ERROR_ERRNO          201
#define UDP_ERROR_UNRESOLVEIP    202
#define UDP_ERROR_NOIFACE        203
#define UDP_ERROR_NOTIMPLEMENTED 204
#define UDP_ERROR_NULL           205




#define UDP_MAXHOSTNAMELEN    255
#define UDP_SETBUFFERSIZE     1048576
#define UDP_SOREUSEADDR
//#define UDP_SOREUSEPORT




Socket_udp *udp_init (const gchar *addr, const gchar *iface, guint16 rx_port, guint16 tx_port, gint ttl, GError **error);
void udp_exit (Socket_udp *s, GError **error);

gint udp_send (Socket_udp *s, char *buffer, gint buflen);
inline gint udp_recv (Socket_udp *s, char *buffer, gint buflen);

void udp_fd_zero (void);
void udp_fd_set (Socket_udp *s);
gint udp_fd_isset (Socket_udp *s);
gint udp_select (struct timeval *timeout);

const gchar *udp_host_addr (Socket_udp *s, GError **error);

gint udp_fd (Socket_udp *s);


#endif


