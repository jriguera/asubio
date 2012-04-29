
/*
 *
 * FILE: notify.h
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

#ifndef _NOTIFY_H_
#define _NOTIFY_H_



#include <glib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

#include "net/net.h"
#include "net/tcp.h"
#include "common.h"
#include "ssip.h"



#define NOTIFY_ERROR        1000
#define NOTIFY_ERROR_RECV   1020
#define NOTIFY_ERROR_SEND   1010


#define NOTIFY_TIME_EXPIRED 5



gint notify_online_to (gchar *user, gchar *clave, guint16 ntf_port, gchar *to_user, gchar *to_host, guint16 to_port, GError **error);

gint notify_offline_to (gchar *user, gchar *clave, gchar *to_user, gchar *to_host, guint16 to_port, GError **error);


#endif


