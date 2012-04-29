
/*
 *
 * FILE: plugin_effect.h
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

#ifndef _PLUGIN_EFFECT_H_
#define _PLUGIN_EFFECT_H_


#include <glib.h>
#include <gmodule.h>
#include "audio.h"
#include "audio_convert.h"
#include "configfile.h"
#include "common.h"




#define PLUGIN_EFFECT_OK    1
#define PLUGIN_EFFECT_ERROR 0


#define PLUGIN_EFFECT_AUDIO_ERROR                  1001
#define PLUGIN_EFFECT_AUDIO_ERROR_ERRNO            1002
#define PLUGIN_EFFECT_AUDIO_ERROR_OPEN             1010
#define PLUGIN_EFFECT_AUDIO_ERROR_CLOSE            1011
#define PLUGIN_EFFECT_AUDIO_ERROR_CAPS             1020
#define PLUGIN_EFFECT_AUDIO_ERROR_FORMAT           1040
#define PLUGIN_EFFECT_AUDIO_ERROR_CHANNELS         1050
#define PLUGIN_EFFECT_AUDIO_ERROR_RATE             1060
#define PLUGIN_EFFECT_AUDIO_ERROR_BLOCKSIZE        1070
#define PLUGIN_EFFECT_AUDIO_ERROR_APROPERTIES      1080


#define PLUGIN_EFFECT_AUDIO_ERROR_CFGFILE_READ     1201
#define PLUGIN_EFFECT_AUDIO_ERROR_CFGFILE_WRITE    1202
#define PLUGIN_EFFECT_AUDIO_ERROR_NOCFGFILE        1203




enum _TPCall
{
    AUDIO_CALL_NONE = 0,
    AUDIO_CALL_IN = 1 << 0,
    AUDIO_CALL_OUT = 1 << 1,
};

typedef enum _TPCall TPCall;



#define PLUGIN_EFFECT_STRUCT "get_Plugin_Effect_struct"



struct _Plugin_Effect
{
    void *handle;
    gchar *name;
    gchar *filename;
    gchar *description;
    gchar *license;

    gint *is_usable;
    gint *is_selected;

    gpointer (*init) (ConfigFile *cfgfile, AudioProperties *aprts, GError **error);
    gint (*cleanup) (gpointer status);

    gint (*configure) (gpointer status);
    gint (*about) (void);

    gint (*pefunction) (gpointer status, gpointer d, gint *length, TPCall type);
};

typedef struct _Plugin_Effect Plugin_Effect;




typedef Plugin_Effect *(*LFunctionPlugin_Effect) (void);




#include "audio_buffer.h"


#endif


