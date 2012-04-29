
/*
 *
 * FILE: ssip.h
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

#ifndef _SSIP_H_
#define _SSIP_H_


#include <glib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>



#define SSIP_MANAGE_DEFAULT_PORT  10000
#define SSIP_NOTIFY_DEFAULT_PORT  20000




#define PKT_USERNAME_LEN    16
#define PKT_USERDES_LEN    256
#define PKT_USERDATA_LEN   512
#define PKT_HOSTNAME_LEN   256




// SSIP  (Simple SIP Protocol - protocolo simple de inicio de session)


#define CONNECT_HELO       10
#define CONNECT_BYE        20
#define CONNECT_IGNOREUSER 30
#define CONNECT_VALIDUSER  40
#define CONNECT_CODECTX    50
#define CONNECT_RTP        60
#define CONNECT_ERROR     100




struct _SsipPkt
{
    gchar to_user[PKT_USERNAME_LEN];
    gchar to_host[PKT_HOSTNAME_LEN];
    guint16 to_port;

    gchar from_user[PKT_USERNAME_LEN];
    gchar from_host[PKT_HOSTNAME_LEN];
    guint16 from_port;

    gint32 type;

    gint32 data_len;
    gchar data[PKT_USERDATA_LEN];
};

typedef struct _SsipPkt SsipPkt;




// SNP  (Simple Notify Protocol - protocolo de notificacion simple)


#define USER_HELO_ONLINE   10
#define USER_OK_ONLINE     20
#define USER_BYE_ONLINE    30
#define USER_OK_OFFLINE    40
#define USER_INCORRECT     50
#define ERROR_PKT         100


struct _NtfPkt
{
    gint ipmode;

    gchar from_user[PKT_USERNAME_LEN];
    struct in_addr from_addr4;
#ifdef NET_HAVE_IPv6
    struct in6_addr from_addr6;
#endif
    guint16 from_port;

    gchar to_user[PKT_USERNAME_LEN];
    struct in_addr to_addr4;
#ifdef NET_HAVE_IPv6
    struct in6_addr to_addr6;
#endif
    guint16 to_port;

    gchar data[PKT_USERDES_LEN];

    gint32 type;
};

typedef struct _NtfPkt NtfPkt;



#endif


