
/*
 *
 * FILE: configfile.c
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
 * This file is based on XMMS  - Cross-platform multimedia player
 *  Copyright (C) 1998-2002  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2002  Haavard Kvaalen
 *
 ***************************************************************************/

#include "configfile.h"



static ConfigLine *_create_string (ConfigSection *section, gchar *key, gchar *value);
static ConfigLine *_find_string (ConfigSection *section, gchar *key);
static ConfigSection *_create_section (ConfigFile *cfg, gchar *name);
static ConfigSection *_find_section (ConfigFile *cfg, gchar *name);



ConfigFile *cfgf_new (void)
{
    ConfigFile *cfg;


    cfg = g_malloc0(sizeof(ConfigFile));
    //cfg->read_mutex = g_mutex_new();
    cfg->write_mutex = g_mutex_new();
    return cfg;
}



ConfigFile *cfgf_open_file (gchar *filename, GError **error)
{
    ConfigFile *cfg;
    gchar *buffer;
    gchar **lines;
    gchar *tmp;
    struct stat stats;
    ConfigSection *section = NULL;
    gint i;
    FILE *file;


    if (stat(filename, &stats) == -1)
    {
	g_set_error(error, CFGF_ERROR, CFGF_ERROR_STAT, "%s (%s)", g_strerror(errno), filename);
	return NULL;
    }
    if (!(file = fopen(filename, "r")))
    {
	g_set_error(error, CFGF_ERROR, CFGF_ERROR_OPEN, "%s (%s)", g_strerror(errno), filename);
	return NULL;
    }
    buffer = g_malloc(stats.st_size + 1);
    if ((signed) fread(buffer, 1, stats.st_size, file) != stats.st_size)
    {
	g_free(buffer);
	fclose(file);
	g_set_error(error, CFGF_ERROR, CFGF_ERROR_READ, "problema leyendo el fichero %s", g_strerror(errno));
	return NULL;
    }
    fclose(file);
    buffer[stats.st_size] = '\0';

    cfg = g_malloc0(sizeof (ConfigFile));
    lines = g_strsplit(buffer, "\n", 0);
    g_free(buffer);
    i = 0;
    while (lines[i])
    {
	if (lines[i][0] == '[')
	{
	    if ((tmp = strchr(lines[i], ']')))
	    {
		*tmp = '\0';
		section = _create_section(cfg, &lines[i][1]);
	    }
	}
	else
	{
	    if (lines[i][0] != '#' && section)
	    {
		if ((tmp = strchr(lines[i], '=')))
		{
		    *tmp = '\0';
		    _create_string(section, lines[i], ++tmp);
		}
	    }
	}
	i++;
    }
    g_strfreev(lines);
    //cfg->read_mutex = g_mutex_new();
    cfg->write_mutex = g_mutex_new();

    return cfg;
}






gboolean cfgf_write_file (ConfigFile *cfg, gchar *filename, GError **error)
{
    FILE *file;
    GList *section_list;
    GList *line_list;
    ConfigSection *section;
    ConfigLine *line;


    g_mutex_lock(cfg->write_mutex);

    if (!(file = fopen(filename, "w")))
    {
	g_set_error(error, CFGF_ERROR, CFGF_ERROR_OPEN, "%s (%s)", g_strerror(errno), filename);
	g_mutex_unlock(cfg->write_mutex);
	return FALSE;
    }
    section_list = cfg->sections;
    while (section_list)
    {
	section = (ConfigSection *) section_list->data;
	if (section->lines)
	{
	    fprintf(file, "[%s]\n", section->name);
	    line_list = section->lines;
	    while (line_list)
	    {
		line = (ConfigLine *) line_list->data;
		fprintf(file, "%s=%s\n", line->key, line->value);
		line_list = g_list_next(line_list);
	    }
	    fprintf(file, "\n");
	}
	section_list = g_list_next(section_list);
    }
    fclose(file);

    g_mutex_unlock(cfg->write_mutex);

    return TRUE;
}



gboolean cfgf_read_string (ConfigFile *cfg, gchar *section, gchar *key, gchar **value)
{
    ConfigSection *sect;
    ConfigLine *line;


    g_mutex_lock(cfg->write_mutex);

    if (!(sect = _find_section(cfg, section)))
    {
	g_mutex_unlock(cfg->write_mutex);
	return FALSE;
    }
    if (!(line = _find_string(sect, key)))
    {
	g_mutex_unlock(cfg->write_mutex);
	return FALSE;
    }
    *value = g_strdup(line->value);

    g_mutex_unlock(cfg->write_mutex);

    return TRUE;
}



gboolean cfgf_read_int (ConfigFile *cfg, gchar *section, gchar *key, gint *value)
{
    gchar *str;


    if (!cfgf_read_string(cfg, section, key, &str)) return FALSE;
    *value = atoi(str);
    g_free(str);
    return TRUE;
}



gboolean cfgf_read_float (ConfigFile *cfg, gchar *section, gchar *key, gfloat *value)
{
    gchar *str;
    gchar *locale;


    if (!cfgf_read_string(cfg, section, key, &str)) return FALSE;
    locale = setlocale(LC_NUMERIC, "C");
    *value = strtod(str, NULL);
    setlocale(LC_NUMERIC, locale);
    g_free(str);
    return TRUE;
}



void cfgf_write_string (ConfigFile *cfg, gchar *section, gchar *key, gchar *value)
{
    ConfigSection *sect;
    ConfigLine *line;


    g_mutex_lock(cfg->write_mutex);

    if (!(sect = _find_section(cfg, section))) sect = _create_section(cfg, section);
    if ((line = _find_string(sect, key)))
    {
	g_free(line->value);
	line->value = g_strchug(g_strchomp(g_strdup(value)));
    }
    else _create_string(sect, key, value);

    g_mutex_unlock(cfg->write_mutex);
}



void cfgf_write_int (ConfigFile *cfg, gchar *section, gchar *key, gint value)
{
    gchar *strvalue;


    strvalue = g_strdup_printf("%d", value);
    cfgf_write_string(cfg, section, key, strvalue);
    g_free(strvalue);
}



void cfgf_write_float (ConfigFile *cfg, gchar *section, gchar *key, gfloat value)
{
    gchar *strvalue;
    gchar *locale;


    locale = setlocale(LC_NUMERIC, "C");
    strvalue = g_strdup_printf("%g", value);
    setlocale(LC_NUMERIC, locale);
    cfgf_write_string(cfg, section, key, strvalue);
    g_free(strvalue);
}



gboolean cfgf_remove_key (ConfigFile *cfg, gchar *section, gchar *key)
{
    ConfigSection *sect;
    ConfigLine *line;


    g_mutex_lock(cfg->write_mutex);

    if ((sect = _find_section(cfg, section)) != NULL)
    {
	if ((line = _find_string(sect, key)) != NULL)
	{
	    g_free(line->key);
	    g_free(line->value);
	    g_free(line);
	    sect->lines = g_list_remove(sect->lines, line);
	}
	g_mutex_unlock(cfg->write_mutex);
        return TRUE;
    }
    g_mutex_unlock(cfg->write_mutex);

    return FALSE;
}



void cfgf_free (ConfigFile * cfg)
{
    ConfigSection *section;
    ConfigLine *line;
    GList *section_list;
    GList *line_list;


    section_list = cfg->sections;
    while (section_list)
    {
	section = (ConfigSection *) section_list->data;
	g_free(section->name);

	line_list = section->lines;
	while (line_list)
	{
	    line = (ConfigLine *) line_list->data;
	    g_free(line->key);
	    g_free(line->value);
	    g_free(line);
	    line_list = g_list_next(line_list);
	}
	g_list_free(section->lines);
	g_free(section);

	section_list = g_list_next(section_list);
    }
    g_list_free(cfg->sections);
    g_mutex_free(cfg->write_mutex);
    g_free(cfg);
}




/* Private functions                                                        */
/* Funciones privadas (solo accesibles desde este fichero)                  */




static ConfigSection *_create_section (ConfigFile *cfg, gchar* name)
{
    ConfigSection *section;


    section = g_malloc0(sizeof (ConfigSection));
    section->name = g_strdup(name);
    cfg->sections = g_list_append(cfg->sections, section);
    return section;
}



static ConfigLine *_create_string (ConfigSection *section, gchar *key, gchar *value)
{
    ConfigLine *line;


    line = g_malloc0(sizeof (ConfigLine));
    line->key = g_strchug(g_strchomp(g_strdup(key)));
    line->value = g_strchug(g_strchomp(g_strdup(value)));
    section->lines = g_list_append(section->lines, line);
    return line;
}



static ConfigSection *_find_section (ConfigFile *cfg, gchar *name)
{
    ConfigSection *section;
    GList *list;


    list = cfg->sections;
    while (list)
    {
	section = (ConfigSection *) list->data;
	if (!strcasecmp(section->name, name)) return section;
	list = g_list_next(list);
    }
    return NULL;
}



static ConfigLine *_find_string (ConfigSection *section, gchar *key)
{
    ConfigLine *line;
    GList *list;


    list = section->lines;
    while (list)
    {
	line = (ConfigLine *) list->data;
	if (!strcasecmp(line->key, key)) return line;
	list = g_list_next(list);
    }
    return NULL;
}


