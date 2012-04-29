
/*
 *
 * FILE: addbook_xml.h
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

#ifndef _ADDBOOK_XML_H_
#define _ADDBOOK_XML_H_


#include <glib.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "common.h"




struct _ItemUser
{
    gchar *name;
    gchar *user;
    gchar *host;
    gboolean ignore;
    gchar *description;
    gchar *photo;
};

typedef struct _ItemUser ItemUser;




#define XML_OK                  9000
#define XML_ERROR               9010
#define XML_ERROR_UKFORMAT      9020
#define XML_ERROR_FILE          9030



#define XML_ROOT_NODE           "agenda"
#define XML_CONTACT             "contacto"
#define XML_CONTACT_DEFAULT     "default"
#define XML_CONTACT_IGNORE      "ignore"
#define XML_CONTACT_NAME        "nombre"
#define XML_CONTACT_USER        "usuario"
#define XML_CONTACT_HOST        "host"
#define XML_CONTACT_DESCRIPTION "descripcion"
#define XML_CONTACT_PHOTO       "foto"

#define XML_COMMENT             "Fichero de contactos, programa 'addressbook'"




xmlDocPtr xml_opendoc (gchar *docname, GError **error);
xmlDocPtr xml_newdoc ();
void xml_freedoc (xmlDocPtr doc);
gboolean xml_savedoc (xmlDocPtr doc, gchar *filename, GError **error);

gboolean xml_set_default_contact (ItemUser *user, xmlDocPtr doc);
gboolean xml_set_contact (ItemUser *user, xmlDocPtr doc);

GSList *xml_get_contact (xmlDocPtr doc, GError **error);
ItemUser *xml_get_default_contact (xmlDocPtr doc, GError **error);

ItemUser *xml_get_user (xmlNodePtr child);

gboolean is_user_ignored (xmlDocPtr doc, gchar *user, GError **error);
ItemUser *get_item_user (xmlDocPtr doc, gchar *user, GError **error);

ItemUser *get_user (gchar *username, gchar *db, GError **error);

void item_user_free(ItemUser *i);


#endif


