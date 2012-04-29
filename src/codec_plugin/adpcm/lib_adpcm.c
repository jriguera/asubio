
/*
 *
 * FILE: lib_adpcm.c
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

#include "lib_adpcm.h"
#include <stdio.h>



static Plugin_Codec *plugin_adpcm_codec;




G_MODULE_EXPORT const gchar *g_module_check_init (GModule *module)
{
    UNUSED(module);


    plugin_adpcm_codec = (Plugin_Codec *) g_malloc(sizeof(Plugin_Codec));

    plugin_adpcm_codec->handle = NULL;
    plugin_adpcm_codec->name = g_strdup(ADPCM_NAME);
    plugin_adpcm_codec->filename = g_strdup(ADPCM_FILENAME);
    plugin_adpcm_codec->description = g_strdup_printf("ADPCM Dynamic Load Library (c) %s %s %s", ADPCM_AUTOR_DATE, ADPCM_AUTOR_NAME, ADPCM_AUTOR_MAIL);
    plugin_adpcm_codec->license = g_strdup(ADPCM_SHORT_LICENSE);

    plugin_adpcm_codec->uncompressed_fr_size = ADPCM_ENCODE_LENGTH;
    plugin_adpcm_codec->compressed_fr_size = ADPCM_DECODE_LENGTH;
    plugin_adpcm_codec->rate = 8000;
    plugin_adpcm_codec->bw = ADPCM_BW;
    plugin_adpcm_codec->payload = 5;
    plugin_adpcm_codec->audio_ms = 32;

    plugin_adpcm_codec->is_usable = g_malloc(sizeof(gint));
    *plugin_adpcm_codec->is_usable = 1;
    plugin_adpcm_codec->is_selected = g_malloc(sizeof(gint));
    *plugin_adpcm_codec->is_selected = 0;

    plugin_adpcm_codec->constructor = adpcm_codec_new;

    return NULL;
}



G_MODULE_EXPORT void g_module_unload (GModule *module)
{
    UNUSED(module);


    g_free(plugin_adpcm_codec->name);
    g_free(plugin_adpcm_codec->filename);
    g_free(plugin_adpcm_codec->description);
    g_free(plugin_adpcm_codec->license);
    g_free(plugin_adpcm_codec->is_usable);
    g_free(plugin_adpcm_codec->is_selected);
    g_free(plugin_adpcm_codec);
    return;
}



G_MODULE_EXPORT Plugin_Codec *get_Plugin_Codec_struct (void)
{
    return plugin_adpcm_codec;
}




Codec *adpcm_codec_new ()
{
    ADPCMCodec *obj;


    obj = (ADPCMCodec *) g_malloc(sizeof(ADPCMCodec));
    obj->codec_class._init = &adpcm_codec_init;
//    obj->codec_class._getinfo = &adpcm_codec_getinfo;
    obj->codec_class._getinfo = NULL;
    obj->codec_class._encode = NULL;
    obj->codec_class._decode = NULL;
    obj->codec_class._about = &adpcm_codec_about;
    obj->codec_class._configure = &adpcm_codec_configure;
    obj->codec_class._destroy = &adpcm_codec_destroy;
    obj->mode = 0;
    obj->adpcm_encode_len = 0;
    obj->adpcm_decode_len = 0;

    return ((Codec *) obj);
}



gboolean adpcm_codec_init (Codec *codec, AudioProperties *ap, ConfigFile *config, CodecMode mode, GError **error)
{
    ADPCMCodec *obj = (ADPCMCodec *) codec;


    UNUSED(config);
    if (ap->format != FMT_S16_LE)
    {
	g_set_error(error, PLUGIN_CODEC_AUDIO_ERROR, PLUGIN_CODEC_AUDIO_ERROR_APROPERTIES, "audio con formato no soportado");
        return FALSE;
    }
    if (mode & CODEC_DECODE)
    {
	if (mode & CODEC_ENCODE)
	{
	    obj->codec_class._encode = &adpcm_codec_encode;
	    obj->adpcm_encode_len = ADPCM_ENCODE_LENGTH;
	}
	obj->codec_class._decode = &adpcm_codec_decode;
	obj->adpcm_decode_len = ADPCM_DECODE_LENGTH;
	obj->mode |= mode ;
        return TRUE;
    }
    else
    {
	if (mode & CODEC_ENCODE)
	{
	    obj->codec_class._encode = &adpcm_codec_encode;
	    obj->adpcm_encode_len = ADPCM_ENCODE_LENGTH;
	}
	obj->mode |= mode;
        return TRUE;
    }
    return FALSE;
}


/*
gboolean adpcm_codec_getinfo (Codec *codec, PluginCodec *info)
{
    if (info == NULL) return FALSE;
    memcpy (info, plugin_adpcm_codec, sizeof(PluginCodec));
    return TRUE;
}
*/


gint adpcm_codec_encode (Codec *codec, gpointer frame, gpointer data)
{
    ADPCMCodec *obj = (ADPCMCodec *) codec;


    if (obj->mode & CODEC_ENCODE)
    {
	adpcm_coder(frame, data, (obj->adpcm_encode_len/2), &obj->adpcm_encode_status);
        return 1;
    }
    else return PLUGIN_CODEC_NOTENCODE;
}



gint adpcm_codec_decode (Codec *codec, gpointer data, gpointer frame)
{
    ADPCMCodec *obj = (ADPCMCodec *) codec;


    if (obj->mode & CODEC_DECODE)
    {
	adpcm_decoder(data, frame, (obj->adpcm_decode_len*2), &obj->adpcm_decode_status);
        return 1;
    }
    else return PLUGIN_CODEC_NOTDECODE;
}



gint adpcm_codec_about (Codec *codec)
{
    static GtkWidget *dialog;
    gint result;


    UNUSED(codec);

    if (dialog != NULL)
    {
	gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_NONE);
        return PLUGIN_CODEC_OK;
    }
    if (!gtk_init_check (NULL, NULL)) return PLUGIN_CODEC_ERROR;

    dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
				    "\nAbout Audio ADPCM VoCodec Plugin:\n\n\n(c) %s %s %s\nADPCM Dynamic Load Library\n\n\n%s\n",
				    ADPCM_AUTOR_DATE, ADPCM_AUTOR_NAME, ADPCM_AUTOR_MAIL, ADPCM_LONG_LICENSE);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    gtk_widget_destroyed(dialog, &dialog);

    while (gtk_events_pending()) gtk_main_iteration();

    return PLUGIN_CODEC_OK;
}



gint adpcm_codec_configure (Codec *codec)
{
    static GtkWidget *dialog;
    gint result;


    UNUSED(codec);

    if (dialog != NULL)
    {
	gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_NONE);
        return PLUGIN_CODEC_OK;
    }
    if (!gtk_init_check (NULL, NULL)) return PLUGIN_CODEC_ERROR;

    dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
				    "\nEl codec de audio ADPCM no tiene parametros de configuracion.\n");
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    gtk_widget_destroyed(dialog, &dialog);

    while (gtk_events_pending()) gtk_main_iteration();

    return PLUGIN_CODEC_OK;
}



gboolean adpcm_codec_destroy (Codec *codec)
{
    ADPCMCodec *obj = (ADPCMCodec *) codec;


    if (obj != NULL) g_free(obj);
    obj = NULL;
    return TRUE;
}


