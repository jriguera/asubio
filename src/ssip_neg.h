
/*
 *
 * FILE: ssip_neg.h
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

#ifndef _SSIP_NEG_H_
#define _SSIP_NEG_H_


#include <glib.h>

#include "net/net.h"
#include "net/tcp.h"
#include "configfile.h"
#include "common.h"
#include "gui_gtk/gui_connect_call.h"




#define SSIP_ERROR              10000
#define SSIP_OK                 10100
#define SSIP_ERROR_SEND         10200
#define SSIP_ERROR_RECV         10300
#define SSIP_ERROR_TIME_RECV    10400
#define SSIP_ERROR_CODEC        10500
#define SSIP_ERROR_CONNECT      10600
#define SSIP_ERROR_UNRESOLVEIP  10700



#define SSIP_NEG_TCPTIME_EXPIRED    5



gint ssip_accept (Socket_tcp *skt, gchar *from_user, gchar *from_host, guint16 from_port,
		  gchar *from_real_host, guint16 rtp_port,
		  GSList *list_codec, GMutex *list_mutex, NotifyCall *_notify_call_, GMutex *_mutex_notify_call, GError **error,
		  gchar **rtp_sel_user, gchar **rtp_sel_host, guint16 *rtp_sel_port, Node_listvocodecs **sel_codec);

gint ssip_connect (gchar *from_user, gchar *from_host, guint16 from_port,
		   gchar *to_user, gchar *to_host, guint16 to_port, gchar *from_msg,
		   GSList *list_codec, GMutex *list_mutex, guint16 rtp_port, ConfigFile *conf, ProgressData *pd, GError **error,
		   gchar **rtp_sel_host, guint16 *rtp_sel_port, Node_listvocodecs **sel_codec);

#endif


