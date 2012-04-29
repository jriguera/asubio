
/*
 *
 * FILE: lib_adpcm.h
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

#ifndef _LIB_ADPCM_H_
#define _LIB_ADPCM_H_


#include <glib.h>
#include <gtk/gtk.h>
#include <gmodule.h>
#include "../codec.h"
#include "adpcm.h"




struct _ADPCMCodec
{
    Codec codec_class;
    CodecMode mode;
    Adpcm_State adpcm_encode_status;
    gint adpcm_encode_len;
    Adpcm_State adpcm_decode_status;
    gint adpcm_decode_len;
};

typedef struct _ADPCMCodec ADPCMCodec;




#define ADPCM_ENCODE_LENGTH  1024
#define ADPCM_DECODE_LENGTH  256
#define ADPCM_BW             32000



#define ADPCM_NAME          "ADPCM VoCodec Plugin"
#define ADPCM_FILENAME      "libadpcm.so"
#define ADPCM_AUTOR_NAME    "Jose Riguera"
#define ADPCM_AUTOR_MAIL    "<jriguera@gmail.com>"
#define ADPCM_AUTOR_DATE    "Mayo 2003"
#define ADPCM_SHORT_LICENSE "GPL"
#define ADPCM_LONG_LICENSE  "This program is free software; you can redistribute it and/or modify \
it under the terms of the GNU General Public License as published by \
the Free Software Foundation; either version 2 of the License, or \
(at your option) any later version.\n \
This program is distributed in the hope that it will be useful,\
but WITHOUT ANY WARRANTY; without even the implied warranty of \
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the \
GNU General Public License for more details.\n \
You should have received a copy of the GNU General Public License \
along with this program; if not, write to the Free Software \
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, \
USA."




Codec *adpcm_codec_new ();

gboolean adpcm_codec_init (Codec *codec, AudioProperties *ap, ConfigFile *config, CodecMode mode, GError **error);
//gboolean adpcm_codec_getinfo (Codec *codec, PluginCodec *info);

gint adpcm_codec_encode (Codec *codec, gpointer frame, gpointer data);
gint adpcm_codec_decode (Codec *codec, gpointer data, gpointer frame);

gint adpcm_codec_about (Codec *codec);
gint adpcm_codec_configure (Codec *codec);

gboolean adpcm_codec_destroy (Codec *codec);



#endif


