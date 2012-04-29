
/*
 *
 * FILE: gui_connect_call.h
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

#ifndef _GUI_CONNECT_CALL_H_
#define _GUI_CONNECT_CALL_H_


#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <gtk/gtk.h>

#include "gui_addressbook.h"
#include "../ssip.h"
#include "../configfile.h"
#include "../common.h"




struct _GuiConnect
{
    gchar *username;
    gchar *host;
    gint ssip_port;
    gchar *msg;
    gboolean ssip;
    GSList *cod_list;
    gint rx_port;
    gint tx_port;
    gboolean des;
    gchar *despass;
    ItemUser *user;
    gpointer data;
};

typedef struct _GuiConnect GuiConnect;




enum
{
  CL_COLUMN_NUMBER,
  CL_COLUMN_NAME,
  CL_COLUMN_PAYLOAD,
  CL_COLUMN_BW,
  CL_NUM_COLUMNS
};




struct _ProgressData
{
    GtkWidget *window;
    GtkWidget *pbar;
    GtkWidget *dialog;
    GtkWidget *label;
    gint timer;
    gint *exit;
};

typedef struct _ProgressData ProgressData;



#include "gui_main.h"



gboolean create_connect_dialog (ConfigFile *conffile, GtkWidget *main_window, GSList *list_codec, GuiConnect **gui_connect);

ProgressData *gui_connect_progresbar (GtkWidget *main_window, gint *exit,
				      gchar *from_user, gchar *from_host,
				      gchar *to_user, gchar *to_host);

#endif


