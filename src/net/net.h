
/*
 *
 * FILE: net.h
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

#ifndef _NET_H_
#define _NET_H_


#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <glib.h>

#include "../common.h"




#define IPv4	4
#define IPv6	6




#ifdef NET_HAVE_IPv6

#ifdef NET_HAVE_NETINET6_IN6_H
#include <netinet6/in6.h>
#else
#include <netinet/in.h>
#endif

#ifndef IPV6_ADD_MEMBERSHIP
#ifdef  IPV6_JOIN_GROUP
#define IPV6_ADD_MEMBERSHIP IPV6_JOIN_GROUP
#else
#error No definition of IPV6_ADD_MEMBERSHIP, please report to mm-tools@cs.ucl.ac.uk.
#endif // IPV6_JOIN_GROUP
#endif // IPV6_ADD_MEMBERSHIP


#ifndef IPV6_DROP_MEMBERSHIP
#ifdef  IPV6_LEAVE_GROUP
#define IPV6_DROP_MEMBERSHIP IPV6_LEAVE_GROUP
#else
#error No definition of IPV6_LEAVE_GROUP, please report to mm-tools@cs.ucl.ac.uk.
#endif // IPV6_LEAVE_GROUP
#endif // IPV6_DROP_MEMBERSHIP

#endif // NET_HAVE_IPv6




#define NET_INADDR_NONE 0xffffffff



#define NET_IP4_HEADER_SIZE 20  //20 is the minimum, but there may be some options
#define NET_IP6_HEADER_SIZE 40



#ifdef NET_HAVE_IPv6
#define NET_IP_HEADER_SIZE 40
#else
#define NET_IP_HEADER_SIZE 20
#endif



#define NET_ERROR              100
#define NET_ERROR_ERRNO        101
#define NET_ERROR_UNRESOLVEIP  102




typedef guchar ttl_t;
typedef gint fd_t;




gboolean net_addr_valid (const gchar *addr, GError **error);


#endif


