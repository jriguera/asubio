
/*
 *
 * FILE: configfile.h
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
 *  Copyright (C) 1998-2002  Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2002  Haavard Kvaalen
 *
 ***************************************************************************/

#ifndef _CONFIGFILE_H_
#define _CONFIGFILE_H_


#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <locale.h>
#include <stdlib.h>
#include <errno.h>
#include <glib.h>
#include "common.h"



#define CFGF_ERROR          5010
#define CFGF_ERROR_STAT     5020
#define CFGF_ERROR_OPEN     5030
#define CFGF_ERROR_CLOSE    5040
#define CFGF_ERROR_READ     5050
#define CFGF_ERROR_WRITE    5060



struct _ConfigLine
{
    gchar *key;
    gchar *value;
};

typedef struct _ConfigLine ConfigLine;



struct _ConfigSection
{
    gchar *name;
    GList *lines;
};

typedef struct _ConfigSection ConfigSection;



struct _ConfigFile
{
    GList *sections;
    //GMutex *read_mutex;
    GMutex *write_mutex;
};

typedef struct _ConfigFile ConfigFile;




ConfigFile *cfgf_new (void);

ConfigFile *cfgf_open_file (gchar *filename, GError **error);
gboolean cfgf_write_file (ConfigFile * cfg, gchar * filename, GError **error);

void cfgf_free (ConfigFile * cfg);

gboolean cfgf_read_int (ConfigFile *cfg, gchar *section, gchar *key, gint *value);
gboolean cfgf_read_string (ConfigFile *cfg, gchar *section, gchar *key, gchar **value);
gboolean cfgf_read_float (ConfigFile *cfg, gchar *section, gchar *key, gfloat *value);

void cfgf_write_int (ConfigFile *cfg, gchar *section, gchar *key, gint value);
void cfgf_write_string (ConfigFile *cfg, gchar *section, gchar *key, gchar *value);
void cfgf_write_float (ConfigFile *cfg, gchar *section, gchar *key, gfloat value);

gboolean cfgf_remove_key (ConfigFile *cfg, gchar *section, gchar *key);


#endif


