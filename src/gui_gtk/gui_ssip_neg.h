
/*
 *
 * FILE: gui_ssip_neg.h
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

#ifndef _GUI_SSIP_NEG_H_
#define _GUI_SSIP_NEG_H_


#include <glib.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "../ssip.h"
#include "../ssip_neg.h"
#include "../configfile.h"
#include "gui_incoming_call.h"
#include "../addbook_xml.h"
#include "gui_connect_call.h"
#include "../common.h"
#include "../net/tcp.h"



#define SSIP_NEG_TIME_SOCKET_BLOCK   500000



gchar *user_photo (gchar *sel_user, gchar *db);

/*
void ssip_neg_init (ConfigFile *conf, gint *numusers, GtkWidget *this_window, GMutex *mutex_list_codecs);
gboolean ssip_process_loop_create (GSList *codecs);
void ssip_process_loop_destroy ();

void ssip_connect_to ();
*/

void user_filter ();


#endif


