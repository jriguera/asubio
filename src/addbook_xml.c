
/*
 *
 * FILE: addbook_xml.c
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

#include "addbook_xml.h"



#define XML_ERROR_UKFORMAT_MSG "documento xml mal formado, nodo raiz !="
#define XML_ERROR_EMPTY_MSG    "documento xml vacio"



xmlDocPtr xml_opendoc (gchar *docname, GError **error)
{
    xmlDocPtr doc;


    doc = xmlParseFile(docname);
    if (doc == NULL)
    {
	g_set_error(error, XML_ERROR, XML_ERROR_FILE, "documento xml no leido satisfactoriamente (%s)", docname);
	g_printerr("[ADDBOOK_XML] xml_initdoc: Document %s not parsed successfully.\n", docname);
	return NULL;
    }
    return doc;
}



xmlDocPtr xml_newdoc ()
{
    xmlNodePtr root;
    xmlNodePtr com;
    xmlDocPtr doc;


    doc = xmlNewDoc("1.0");

    root = xmlNewDocNode(doc, NULL, XML_ROOT_NODE, NULL);
    com = xmlNewText("\n");
    xmlAddChild(root, com);
    com = xmlNewDocComment(doc, XML_COMMENT);
    xmlAddChild(root, com);
    com = xmlNewText("\n");
    xmlAddChild(root, com);
    xmlDocSetRootElement(doc, root);
    return doc;
}



void xml_freedoc (xmlDocPtr doc)
{
    xmlFreeDoc(doc);
}



gboolean xml_savedoc (xmlDocPtr doc, gchar *filename, GError **error)
{
    FILE *file;


    if (!(file = fopen(filename, "w")))
    {
	g_set_error(error, XML_ERROR, XML_ERROR_FILE, "%s (%s)", g_strerror(errno), filename);
	return FALSE;
    }
    xmlDocDump(file, doc);
    fclose(file);
    return TRUE;
}



gboolean xml_set_default_contact (ItemUser *user, xmlDocPtr doc)
{
    xmlNodePtr root;
    xmlNodePtr nodeuser;
    xmlNodePtr com;


    root = xmlDocGetRootElement(doc);
    if (root == NULL) return FALSE;

    if (xmlStrcmp(root->name, (const xmlChar *) XML_ROOT_NODE)) return FALSE;

    com = xmlNewText("\n");
    xmlAddChild(root, com);

    nodeuser = xmlNewChild(root, NULL, XML_CONTACT, NULL);
    com = xmlNewText("\n\t");
    xmlAddChild(nodeuser, com);
    xmlSetProp(nodeuser, XML_CONTACT_DEFAULT, "TRUE");
    xmlSetProp(nodeuser, XML_CONTACT_IGNORE, (user->ignore) ? "TRUE" : "FALSE");
    xmlNewChild(nodeuser, NULL, XML_CONTACT_NAME, user->name);
    com = xmlNewText("\n\t");
    xmlAddChild(nodeuser, com);
    xmlNewChild(nodeuser, NULL, XML_CONTACT_USER, user->user);
    com = xmlNewText("\n\t");
    xmlAddChild(nodeuser, com);
    xmlNewChild(nodeuser, NULL, XML_CONTACT_HOST, user->host);
    com = xmlNewText("\n\t");
    xmlAddChild(nodeuser, com);
    xmlNewChild(nodeuser, NULL, XML_CONTACT_DESCRIPTION, user->description);
    com = xmlNewText("\n\t");
    xmlAddChild(nodeuser, com);
    xmlNewChild(nodeuser, NULL, XML_CONTACT_PHOTO, user->photo);
    com = xmlNewText("\n");
    xmlAddChild(nodeuser, com);
    com = xmlNewText("\n");
    xmlAddChild(root, com);

    return TRUE;
}



gboolean xml_set_contact (ItemUser *user, xmlDocPtr doc)
{
    xmlNodePtr root;
    xmlNodePtr nodeuser;
    xmlNodePtr com;


    root = xmlDocGetRootElement(doc);
    if (root == NULL) return FALSE;

    if (xmlStrcmp(root->name, (const xmlChar *) XML_ROOT_NODE)) return FALSE;

    com = xmlNewText("\n");
    xmlAddChild(root, com);

    nodeuser = xmlNewChild(root, NULL, XML_CONTACT, NULL);
    com = xmlNewText("\n\t");
    xmlAddChild(nodeuser, com);
    xmlSetProp(nodeuser, XML_CONTACT_IGNORE, (user->ignore) ? "TRUE" : "FALSE");
    xmlNewChild(nodeuser, NULL, XML_CONTACT_NAME, user->name);
    com = xmlNewText("\n\t");
    xmlAddChild(nodeuser, com);
    xmlNewChild(nodeuser, NULL, XML_CONTACT_USER, user->user);
    com = xmlNewText("\n\t");
    xmlAddChild(nodeuser, com);
    xmlNewChild(nodeuser, NULL, XML_CONTACT_HOST, user->host);
    com = xmlNewText("\n\t");
    xmlAddChild(nodeuser, com);
    xmlNewChild(nodeuser, NULL, XML_CONTACT_DESCRIPTION, user->description);
    com = xmlNewText("\n\t");
    xmlAddChild(nodeuser, com);
    xmlNewChild(nodeuser, NULL, XML_CONTACT_PHOTO, user->photo);
    com = xmlNewText("\n");
    xmlAddChild(nodeuser, com);

    com = xmlNewText("\n");
    xmlAddChild(root, com);

    return TRUE;
}



GSList *xml_get_contact (xmlDocPtr doc, GError **error)
{
    ItemUser *i;
    xmlChar *d = NULL;
    xmlNodePtr cur;
    GSList *array = NULL;


    cur = xmlDocGetRootElement(doc);
    if (cur == NULL)
    {
	g_set_error(error, XML_ERROR, XML_ERROR_UKFORMAT, XML_ERROR_EMPTY_MSG);
	g_printerr("[ADDBOOK_XML] xml_initparse: Empty document.\n");
	return NULL;
    }

    if (xmlStrcmp(cur->name, (const xmlChar *) XML_ROOT_NODE) != 0)
    {
	g_set_error(error, XML_ERROR, XML_ERROR_UKFORMAT, "%s %s", XML_ERROR_UKFORMAT_MSG, XML_ROOT_NODE);
	g_printerr("[ADDBOOK_XML] xml_initparse: Document of the wrong type, root node != %s", XML_ROOT_NODE);
	return NULL;
    }
    cur = cur->xmlChildrenNode;

    while (cur != NULL)
    {
	if ((!xmlStrcmp(cur->name, (const xmlChar *)XML_CONTACT)))
	{
	    d = xmlGetProp(cur, (const xmlChar *) XML_CONTACT_DEFAULT);
	    if ((d == NULL) || (!xmlStrcmp(d, (const xmlChar *)"FALSE")))
	    {
		i = xml_get_user(cur);
                array = g_slist_append(array, i);
	    }
	    xmlFree(d);
	}
	cur = cur->next;
    }
    return array;
}



ItemUser *xml_get_default_contact (xmlDocPtr doc, GError **error)
{
    ItemUser *i;
    xmlChar *d = NULL;
    xmlNodePtr cur;


    cur = xmlDocGetRootElement(doc);
    if (cur == NULL)
    {
	g_set_error(error, XML_ERROR, XML_ERROR_UKFORMAT, XML_ERROR_EMPTY_MSG);
	g_printerr("[ADDBOOK_XML] xml_initparse: Empty document.\n");
	return NULL;
    }

    if (xmlStrcmp(cur->name, (const xmlChar *) XML_ROOT_NODE) != 0)
    {
	g_set_error(error, XML_ERROR, XML_ERROR_UKFORMAT, "%s %s", XML_ERROR_UKFORMAT_MSG, XML_ROOT_NODE);
	g_printerr("[ADDBOOK_XML] xml_initparse: Document of the wrong type, root node != %s", XML_ROOT_NODE);
	return NULL;
    }
    cur = cur->xmlChildrenNode;

    while (cur != NULL)
    {
	if ((!xmlStrcmp(cur->name, (const xmlChar *)XML_CONTACT)))
	{
	    d = xmlGetProp(cur, (const xmlChar *) XML_CONTACT_DEFAULT);
	    if ((d != NULL) && (!xmlStrcmp(d, (const xmlChar *)"TRUE")))
	    {
		i = xml_get_user(cur);
		xmlFree(d);
		return i;
	    }
	    xmlFree(d);
	}
	cur = cur->next;
    }
    return NULL;
}



ItemUser *xml_get_user (xmlNodePtr child)
{
    ItemUser *item;
    xmlNodePtr node;
    xmlChar *ig;


    if (child == NULL) return NULL;

    item = (ItemUser *) g_malloc0(sizeof(ItemUser));

    ig = xmlGetProp(child, (const xmlChar *) XML_CONTACT_IGNORE);
    if (ig == NULL) item->ignore = FALSE;
    else
    {
	if (!xmlStrcmp(ig, (const xmlChar *)"TRUE")) item->ignore = TRUE;
	else item->ignore = FALSE;
	xmlFree(ig);
    }

    node = child->xmlChildrenNode;

    while (node != NULL)
    {
	if ((!xmlStrcmp(node->name, (const xmlChar *)XML_CONTACT_NAME)))
	{
	    item->name = xmlNodeGetContent(node);
	}
	if ((!xmlStrcmp(node->name, (const xmlChar *)XML_CONTACT_USER)))
	{
	    item->user = xmlNodeGetContent(node);
	}
	if ((!xmlStrcmp(node->name, (const xmlChar *)XML_CONTACT_HOST)))
	{
	    item->host = xmlNodeGetContent(node);
	}
	if ((!xmlStrcmp(node->name, (const xmlChar *)XML_CONTACT_DESCRIPTION)))
	{
	    item->description = xmlNodeGetContent(node);
	}
	if ((!xmlStrcmp(node->name, (const xmlChar *)XML_CONTACT_PHOTO)))
	{
	    item->photo = xmlNodeGetContent(node);
	}
	node = node->next;
    }
    return item;
}



void item_user_free(ItemUser *item)
{
    if (item != NULL)
    {
	if (item->name) g_free(item->name);
	if (item->user) g_free(item->user);
	if (item->host) g_free(item->host);
	if (item->description) g_free(item->description);
	if (item->photo) g_free(item->photo);

	g_free(item);
    }
}



gboolean is_user_ignored (xmlDocPtr doc, gchar *user, GError **error)
{
    ItemUser *i;
    xmlChar *d = NULL;
    GError *tmp_error = NULL;
    xmlNodePtr cur;
    gboolean var;


    cur = xmlDocGetRootElement(doc);
    if (cur == NULL)
    {
	g_set_error(error, XML_ERROR, XML_ERROR_UKFORMAT, XML_ERROR_EMPTY_MSG);
	g_printerr("[ADDBOOK_XML] is_user_ignored: Empty document.\n");
	return FALSE;
    }

    if (xmlStrcmp(cur->name, (const xmlChar *) XML_ROOT_NODE) != 0)
    {
	g_set_error(error, XML_ERROR, XML_ERROR_UKFORMAT, "%s %s", XML_ERROR_UKFORMAT_MSG, XML_ROOT_NODE);
	g_printerr("[ADDBOOK_XML] is_user_ignored: Document of the wrong type, root node != %s", XML_ROOT_NODE);
	return FALSE;
    }
    cur = cur->xmlChildrenNode;

    while (cur != NULL)
    {
	if ((!xmlStrcmp(cur->name, (const xmlChar *)XML_CONTACT)))
	{
	    d = xmlGetProp(cur, (const xmlChar *) XML_CONTACT_DEFAULT);
	    if ((d == NULL) || (!xmlStrcmp(d, (const xmlChar *)"FALSE")))
	    {
		i = xml_get_user(cur);
		if (strcmp(i->user, user) == 0)
		{
		    var = i->ignore;
		    g_free(i);
		    xmlFree(d);
                    return var;
		}
	    }
	    xmlFree(d);
	}
	cur = cur->next;
    }

    i = xml_get_default_contact(doc, &tmp_error);
    if (tmp_error != NULL)
    {
	g_propagate_error(error, tmp_error);
        return FALSE;
    }
    var = i->ignore;
    g_free(i);
    return var;
}



ItemUser *get_item_user (xmlDocPtr doc, gchar *user, GError **error)
{
    ItemUser *i;
    xmlChar *d = NULL;
    xmlNodePtr cur;


    cur = xmlDocGetRootElement(doc);
    if (cur == NULL)
    {
	g_set_error(error, XML_ERROR, XML_ERROR_UKFORMAT, XML_ERROR_EMPTY_MSG);
	g_printerr("[ADDBOOK_XML] get_item_user: Empty document.\n");
	return FALSE;
    }

    if (xmlStrcmp(cur->name, (const xmlChar *) XML_ROOT_NODE) != 0)
    {
	g_set_error(error, XML_ERROR, XML_ERROR_UKFORMAT, "%s %s", XML_ERROR_UKFORMAT_MSG, XML_ROOT_NODE);
	g_printerr("[ADDBOOK_XML] get_item_user: Document of the wrong type, root node != %s", XML_ROOT_NODE);
	return FALSE;
    }
    cur = cur->xmlChildrenNode;

    while (cur != NULL)
    {
	if ((!xmlStrcmp(cur->name, (const xmlChar *)XML_CONTACT)))
	{
	    d = xmlGetProp(cur, (const xmlChar *) XML_CONTACT_DEFAULT);
	    if ((d == NULL) || (!xmlStrcmp(d, (const xmlChar *)"FALSE")))
	    {
		i = xml_get_user(cur);
		if (strcmp(i->user, user) == 0)
		{
                    xmlFree(d);
		    return i;
		}
	    }
	    xmlFree(d);
	}
	cur = cur->next;
    }
    return NULL;
}



ItemUser *get_user (gchar *username, gchar *db, GError **error)
{
    xmlDocPtr doc;
    GError *tmp_error = NULL;
    ItemUser *user;


    doc = xml_opendoc(db, &tmp_error);
    if (tmp_error != NULL)
    {
        g_propagate_error(error, tmp_error);
        return NULL;
    }
    user = get_item_user(doc, username, &tmp_error);
    if (tmp_error != NULL)
    {
        g_propagate_error(error, tmp_error);
        return NULL;
    }
    if (user == NULL)
    {
	user = xml_get_default_contact(doc, &tmp_error);
	if (tmp_error != NULL)
	{
	    g_propagate_error(error, tmp_error);
	    return NULL;
	}
    }
    xml_freedoc(doc);
    return user;
}


