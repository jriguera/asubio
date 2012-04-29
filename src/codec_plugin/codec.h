
/*
 *
 * FILE: codec.h
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

#ifndef _PLUGIN_CODEC_H_
#define _PLUGIN_CODEC_H_


#include <glib.h>

#include "../net/net.h"
#include "../net/udp.h"
#include "../rtp/rtp.h"
#include "../audio.h"
#include "../common.h"
#include "../configfile.h"




#ifndef UNUSED
#define UNUSED(x) (x=x)
#endif



#define PLUGIN_CODEC_OK         1
#define PLUGIN_CODEC_ERROR      -1
#define PLUGIN_CODEC_NOTENCODE  -3
#define PLUGIN_CODEC_NOTDECODE  -4


#define PLUGIN_CODEC_AUDIO_ERROR                  1001
#define PLUGIN_CODEC_AUDIO_ERROR_ERRNO            1002
#define PLUGIN_CODEC_AUDIO_ERROR_OPEN             1010
#define PLUGIN_CODEC_AUDIO_ERROR_CLOSE            1011
#define PLUGIN_CODEC_AUDIO_ERROR_CAPS             1020
#define PLUGIN_CODEC_AUDIO_ERROR_FORMAT           1040
#define PLUGIN_CODEC_AUDIO_ERROR_CHANNELS         1050
#define PLUGIN_CODEC_AUDIO_ERROR_RATE             1060
#define PLUGIN_CODEC_AUDIO_ERROR_BLOCKSIZE        1070
#define PLUGIN_CODEC_AUDIO_ERROR_APROPERTIES      1080


#define PLUGIN_CODEC_AUDIO_ERROR_CFGFILE_READ     1201
#define PLUGIN_CODEC_AUDIO_ERROR_CFGFILE_WRITE    1202
#define PLUGIN_CODEC_AUDIO_ERROR_NOCFGFILE        1203




enum _CodecMode
{
    CODEC_NONE = 0,
    CODEC_ENCODE = 1 << 0,
    CODEC_DECODE = 1 << 1,
};

typedef enum _CodecMode CodecMode;




#define PLUGIN_CODEC_STRUCT "get_Plugin_Codec_struct"




/* this is the base class for all codecs. Override the functions pointer  */
/* to implement new codecs, and create a constructor named                */
/* codecname_create() (ex: gsmcodec_create()).                            */

struct _Plugin_Codec
{
    void *handle;
    gchar *name;
    gchar *filename;
    gchar *description;
    gchar *license;
    gint uncompressed_fr_size;
    gint compressed_fr_size;
    gint rate;
    gint bw;
    gint payload;
    gint audio_ms;
    gint *is_usable;
    gint *is_selected;
    struct _Codec *(*constructor) (void);
};

typedef struct _Plugin_Codec Plugin_Codec;




typedef Plugin_Codec *(*LFunctionPlugin_Codec) (void);




struct _Codec
{
    gboolean (*_init) (struct _Codec *codec, AudioProperties *ap, ConfigFile *config, CodecMode mode, GError **error);
    gboolean (*_getinfo) (struct _Codec *codec, Plugin_Codec *info);

    gint (*_encode) (struct _Codec *codec, gpointer frame, gpointer data);
    gint (*_decode) (struct _Codec *codec, gpointer data, gpointer frame);

    gint (*_about) (struct _Codec *codec);
    gint (*_configure) (struct _Codec *codec);
    gboolean (*_destroy) (struct _Codec *codec);
};

typedef struct _Codec Codec;




inline gboolean codec_init (Codec *codec, AudioProperties *ap, ConfigFile *config, CodecMode mode, GError **error);
inline gboolean codec_getinfo (Codec *codec, Plugin_Codec *info);
inline gint codec_encode (Codec *codec, gpointer frame, gpointer data);
inline gint codec_decode (Codec *codec, gpointer data, gpointer frame);
inline gint codec_about (Codec *codec);
inline gint codec_configure (Codec *codec);
inline gboolean codec_destroy (Codec *codec);


inline gboolean codec_is_usable (Plugin_Codec *codec, double bandwidth);


#endif


