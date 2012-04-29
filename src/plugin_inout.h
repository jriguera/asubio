
/*
 *
 * FILE: plugin_inout.h
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

#ifndef _PLUGIN_INOUT_H_
#define _PLUGIN_INOUT_H_


#include <glib.h>
#include <gmodule.h>
#include "audio.h"
#include "audio_convert.h"
#include "configfile.h"


#define UNUSED(x) (x=x)




struct _AInfo
{
    gint rate;
    gint blocksize;
    gint channels;
    gboolean mixer;
    AFormat format;
};
typedef struct _AInfo AInfo;



struct _ACaps
{
    gchar *device_in;
    gchar *device_out;
    gchar *mixer_in;
    gchar *mixer_out;
    gint rate_in;
    gint rate_out;
    gint op_blocksize_in;
    gint op_blocksize_out;
    gint channels_in;
    gint channels_out;
    AFormat format_in_now;
    AFormat format_out_now;

    gboolean u8_format_in;
    gboolean s8_format_in;
    gboolean u16le_format_in;
    gboolean u16be_format_in;
    gboolean s16le_format_in;
    gboolean s16be_format_in;

    gboolean u8_format_out;
    gboolean s8_format_out;
    gboolean u16le_format_out;
    gboolean u16be_format_out;
    gboolean s16le_format_out;
    gboolean s16be_format_out;

    gboolean block_read;
    gboolean block_write;
    gint volume_in_l;
    gint volume_in_r;
    gint volume_out_l;
    gint volume_out_r;
    gint fragments_in;
    gint fragstotal_in;
    gint fragsize_in;
    gint bytes_in;
    gint fragments_out;
    gint fragstotal_out;
    gint fragsize_out;
    gint bytes_out;
    gboolean fflush;
    gboolean trigger;
    gboolean mmap;
    gboolean full_duplex;
};
typedef struct _ACaps ACaps;



#define PLUGIN_INOUT_OK    1
#define PLUGIN_INOUT_ERROR 0


#define PLUGIN_INOUT_AUDIO_ERROR                  1001
#define PLUGIN_INOUT_AUDIO_ERROR_ERRNO            1002
#define PLUGIN_INOUT_AUDIO_ERROR_OPEN             1010
#define PLUGIN_INOUT_AUDIO_ERROR_CLOSE            1011
#define PLUGIN_INOUT_AUDIO_ERROR_GETCAPS          1020
#define PLUGIN_INOUT_AUDIO_ERROR_SETDUPLEX        1030
#define PLUGIN_INOUT_AUDIO_ERROR_SETFORMAT        1040
#define PLUGIN_INOUT_AUDIO_ERROR_CHANNELS         1050
#define PLUGIN_INOUT_AUDIO_ERROR_RATE             1060
#define PLUGIN_INOUT_AUDIO_ERROR_GETBLOCKSIZE     1070
#define PLUGIN_INOUT_AUDIO_ERROR_SETBLOCKSIZE     1071


#define PLUGIN_INOUT_AUDIO_MIXER_ERROR            1101
#define PLUGIN_INOUT_AUDIO_MIXER_ERROR_ERRNO      1102
#define PLUGIN_INOUT_AUDIO_MIXER_ERROR_OPEN       1110
#define PLUGIN_INOUT_AUDIO_MIXER_ERROR_CLOSE      1111
#define PLUGIN_INOUT_AUDIO_MIXER_ERROR_GET_VOLUME 1120
#define PLUGIN_INOUT_AUDIO_MIXER_ERROR_SET_VOLUME 1121
#define PLUGIN_INOUT_AUDIO_MIXER_ERROR_GET_CAPS   1130
#define PLUGIN_INOUT_AUDIO_MIXER_ERROR_SOURCE     1140


#define PLUGIN_INOUT_AUDIO_ERROR_CFGFILE_READ     1201
#define PLUGIN_INOUT_AUDIO_ERROR_CFGFILE_WRITE    1202
#define PLUGIN_INOUT_AUDIO_ERROR_NOCFGFILE        1203



#define PLUGIN_INOUT_STRUCT "get_Plugin_InOut_struct"



struct _Plugin_InOut
{
    void *handle;
    gchar *name;
    gchar *filename;
    gchar *description;
    gchar *license;

    gint *is_usable;
    gint *is_selected;

    gint (*configure) (void);
    gint (*about) (void);
    gint (*init) (ConfigFile *cfgfile, GError **error);
    gint (*cleanup) (void);

    gint (*open_in) (gint open_flags, AFormat format, gint channels, gint rate, gint fragmet, AInfo *info, GError **error);
    gint (*open_out) (gint open_flags, AFormat format, gint channels, gint rate, gint fragmet, AInfo *info, GError **error);

    gint (*close_out) (void);
    gint (*close_in) (void);

    gint (*read) (gpointer buffer, gint l);
    gint (*write) (gpointer buffer, gint l);

    gint (*get_in_volume) (gint *l_vol, gint *r_vol);
    gint (*set_in_volume) (gint l_vol, gint r_vol);

    gint (*get_out_volume) (gint *l_vol, gint *r_vol);
    gint (*set_out_volume) (gint l_vol, gint r_vol);

    gint (*flush) (gint time);
    ACaps *(*get_caps) (void);
};

typedef struct _Plugin_InOut Plugin_InOut;



typedef Plugin_InOut *(*LFunctionPlugin_InOut) (void);



#endif


