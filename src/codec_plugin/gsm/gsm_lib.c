
#include "libgsm.h"



G_MODULE_EXPORT Plugin_Codec *plugin_gsm_codec;



#define GSM_NAME          "GSM VoCodec Plugin"
#define GSM_FILENAME      "libgsm.so"
#define GSM_AUTOR_NAME    "Jose Riguera"
#define GSM_AUTOR_MAIL    "<jriguera@gmail.com>"
#define GSM_AUTOR_DATE    "Mayo 2003"
#define GSM_SHORT_LICENSE "GPL"
#define GSM_LONG_LICENSE  "This program is free software; you can redistribute it and/or modify \
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









G_MODULE_EXPORT const gchar *g_module_check_init (GModule *module)
{
    UNUSED(module);

    plugin_gsm_codec = (Plugin_Codec *) g_malloc(sizeof(Plugin_Codec));

    plugin_gsm_codec->handle = NULL;
    plugin_gsm_codec->name = g_strdup(GSM_NAME);
    plugin_gsm_codec->filename = g_strdup(GSM_FILENAME);
    plugin_gsm_codec->description = g_strdup_printf("GSM Dynamic Load Library (c) %s %s %s", GSM_AUTOR_DATE, GSM_AUTOR_NAME, GSM_AUTOR_MAIL);
    plugin_gsm_codec->license = g_strdup(GSM_SHORT_LICENSE);

    plugin_gsm_codec->uncompressed_fr_size = 160*2;
    plugin_gsm_codec->compressed_fr_size = 33;
    plugin_gsm_codec->profile = 3;

    plugin_gsm_codec->is_usable = g_malloc(sizeof(gint));
    *plugin_gsm_codec->is_usable = 0;
    plugin_gsm_codec->is_selected = g_malloc(sizeof(gint));
    *plugin_gsm_codec->is_selected = 1;




    return NULL;
}



G_MODULE_EXPORT void g_module_unload (GModule *module)
{
    UNUSED(module);


    g_free(plugin_gsm_codec->name);
    g_free(plugin_gsm_codec->filename);
    g_free(plugin_gsm_codec->description);
    g_free(plugin_gsm_codec->license);
    g_free(plugin_gsm_codec->is_usable);
    g_free(plugin_gsm_codec->is_selected);
    g_free(plugin_gsm_codec);
    return;
}



Codec *gsm_codec_new (CodecMode mode)
{
    GSMCodec *obj;


    obj = (GSMCodec *) g_malloc(sizeof(GSMCodec));
    obj->codec_class._init = &gsm_codec_init;
    obj->codec_class._getinfo = &gsm_codec_getinfo;
    obj->codec_class._encode = NULL;
    obj->codec_class._decode = NULL;
    obj->codec_class._about = &gsm_codec_about;
    obj->codec_class._configure = &gsm_codec_configure;
    obj->codec_class._destroy = &gsm_destroy;
    obj->gsm_encode_status = NULL;
    obj->gsm_decode_status = NULL;
    obj->mode = mode;

    return ((Codec *) obj);
}



gboolean gsm_codec_init (Codec *codec, GError **error)
{
    GSMCodec *obj = (GSMCodec *) codec;


    if (obj->mode == CODEC_DECODE)
    {
	if (obj->mode == CODEC_ENCODE)
	{
	    obj->gsm_encode_status = gsm_create();
	    if (! (obj->gsm_encode_status))
	    {
		g_set_error(error, GSM_ERROR, GSM_ERROR_CREATE_ENCODE, "Can't create encode gsm object (error calling 'gsm_create()')");
		return FALSE;
	    }
            obj->codec_class._encode = &gsm_codec_encode;
	}
	obj->gsm_decode_status = gsm_create();
	if (! (obj->gsm_decode_status))
	{
	    g_set_error(error, GSM_ERROR, GSM_ERROR_CREATE_DECODE, "Can't create decode gsm object (error calling 'gsm_create()')");
	    if (obj->gsm_encode_status) gsm_destroy(obj->gsm_encode_status);
            return FALSE;
	}
        obj->codec_class._decode = &gsm_codec_decode;
    }
    else
    {
	if (obj->mode == CODEC_ENCODE)
	{
	    obj->gsm_encode_status = gsm_create();
	    if (! (obj->gsm_encode_status))
	    {
		g_set_error(error, GSM_ERROR, GSM_ERROR_CREATE_ENCODE, "Can't create encode gsm object (error calling 'gsm_create()')");
                return FALSE;
	    }
	    obj->codec_class._encode = &gsm_codec_encode;
	}
    }
    return TRUE;
}



gboolean gsm_codec_getinfo (Codec *codec, PluginCodec *info)
{
    if (info == NULL) return FALSE;
    memcpy (info, plugin_gsm_codec, sizeof(PluginCodec));
    return TRUE;
}



gint gsm_codec_encode (Codec *codec, gchar *frame, gchar *data)
{
    GSMCodec *obj = (GSMCodec *) codec;

    if (obj->mode == CODEC_ENCODE) return gsm_encode(obj->gsm_encode_status, (gsm_signal*) frame, data);
    else return GSM_ERROR_MODE_NOTENCODE;
}



gint gsm_codec_decode (Codec *codec, gchar *data, gchar *frame)
{
    GSMCodec *obj = (GSMCodec *) codec;

    if (obj->mode == CODEC_DECODE) return gsm_decode(obj->gsm_decode_status, data, (gsm_signal*) frame);
    else return GSM_ERROR_MODE_NOTDECODE;
}



void gsm_codec_about (Codec *codec)
{
    return;
}



gboolean gsm_codec_configure (Codec *codec)
{
    return TRUE;
}



gboolean gsm_codec_destroy (Codec *codec)
{
    GSMCodec *obj = (GSMCodec *) codec;


    if (obj->mode == CODEC_DECODE)
    {
	if (obj->mode == CODEC_ENCODE) gsm_destroy(obj->gsm_encode_status);
	gsm_destroy(obj->gsm_decode_status);
    }
    else
    {
	if (obj->mode == CODEC_ENCODE) gsm_destroy(obj->gsm_encode_status);
    }
    g_free(obj);
    return TRUE;
}


