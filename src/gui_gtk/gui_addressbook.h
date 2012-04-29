
/*
 *
 * FILE: gui_addressbook.h
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

#ifndef _GUI_ADDRESSBOOK_H_
#define _GUI_ADDRESSBOOK_H_


#include <gtk/gtk.h>
#include <glib.h>
#include <string.h>

#include "../addbook_xml.h"
#include "gui_common.h"
#include "../common.h"




enum
{
  ADB_COLUMN_NUMBER,
  ADB_COLUMN_NAME,
  ADB_COLUMN_USER,
  ADB_COLUMN_CHECK,
  ADB_COLUMN_DESCRIPTION,
  ADB_COLUMN_PHOTO,
  ADB_COLUMN_EDITABLE,
  ADB_NUM_COLUMNS
};



#define AB_DEFAULT_PHOTO_SIZEX     100
#define AB_DEFAULT_PHOTO_SIZEY     120


#define AB_DEFAULT_NAMEUSER        "desconocido"
#define AB_DEFAULT_USERNAME        "user"
#define AB_DEFAULT_HOST            "host"
#define AB_DEFAULT_USERDESCRIPTION "Usuario por defecto"




struct _Item_user_display
{
  GtkWidget *button_user_image;
  GtkWidget *image_user;
  GtkWidget *label_name;
};

typedef struct _Item_user_display Item_user_display;




typedef void (*CB_gui_addbook) (ItemUser *user);




void create_addressbook_window (gchar *file_name, CB_gui_addbook cb);



#endif


