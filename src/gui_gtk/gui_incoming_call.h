
/*
 *
 * FILE: gui_incoming_call.h
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

#ifndef _GUI_INCOMING_CALL_H_
#define _GUI_INCOMING_CALL_H_


#include <gtk/gtk.h>
#include <glib.h>
#include <stdio.h>

#include "../configfile.h"
#include "../addbook_xml.h"
#include "../ssip.h"
#include "../net/tcp.h"
#include "../common.h"
#include "gui_addressbook.h"
#include "../ssip.h"
#include "gui_main.h"




#define USER_PHOTO_X   100
#define USER_PHOTO_Y   120



void gui_incoming_call (ItemUser *user, SsipPkt *pkt, Socket_tcp *skt, ConfigFile *fileconf, GtkWidget *main_window, NotifyCall *notify, GMutex *mutex_notify);


#endif


