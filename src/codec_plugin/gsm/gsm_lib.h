
#ifndef _GSM_LIB_H_
#define _GSM_LIB_H_


#include <glib.h>
#include <gmodule.h>
#include "../codec.h"
#include "gsm.h"



typedef struct _GSMCodec
{
    Codec codec_class;
    CodecMode mode;
    gsm gsm_encode_status;
    gsm gsm_decode_status;
} GSMCodec;



Codec *gsm_codec_new (CodecMode mode);

gboolean gsm_codec_init (Codec *codec, GError **error);
gboolean gsm_codec_getinfo (Codec *codec, PluginCodec *info);
gint gsm_codec_encode (Codec *codec, gchar *frame, gchar *data);
gint gsm_codec_decode (Codec *codec, gchar *data, gchar *frame);
void gsm_codec_about (Codec *codec);
gboolean gsm_codec_configure (Codec *codec);
gboolean gsm_codec_destroy (Codec *codec);



#endif

