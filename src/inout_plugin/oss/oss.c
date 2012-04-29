
/*
 *
 * FILE: oss.c
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

#include "oss.h"
#include <ctype.h>




ConfigFile *configuration = NULL;
Plugin_InOut *plugin_inout;
Oss_Conf *oss_cfg;
gboolean device_in_init = FALSE;
gboolean device_out_init = FALSE;
const char *source_audio_in_options[] =
{
    "Mic", "Line In", NULL
};
const char *volume_control_options[] =
{
    "PCM Wave", "Master", NULL
};



static gint oss_configure (void);
static gint oss_about (void);
static gint oss_init (ConfigFile *cfgfile, GError **error);
static gint oss_open_in (gint open_flags, AFormat format, gint channels, gint rate, gint fragmet, AInfo *info, GError **error);
static gint oss_open_out (gint open_flags, AFormat format, gint channels, gint rate, gint fragmet, AInfo *info, GError **error);
static gint oss_close_in (void);
static gint oss_close_out (void);
static gint oss_flush (gint time);
static ACaps *oss_get_caps (void);
static gint oss_read (gpointer buffer, gint l);
static gint oss_write (gpointer buffer, gint l);
static gint oss_get_out_volume (gint *l_vol, gint *r_vol);
static gint oss_set_out_volume (gint l_vol, gint r_vol);
static gint oss_get_in_volume (gint *l_vol, gint *r_vol);
static gint oss_set_in_volume (gint l_vol, gint r_vol);




G_MODULE_EXPORT const gchar *g_module_check_init (GModule *module)
{
    UNUSED(module);

    plugin_inout = (Plugin_InOut *) g_malloc(sizeof(Plugin_InOut));

    plugin_inout->handle = NULL;
    plugin_inout->name = g_strdup(OSS_NAME);
    plugin_inout->filename = g_strdup(OSS_FILENAME);
    plugin_inout->description = g_strdup_printf("OSS Dynamic Load Library (c) %s %s %s", OSS_AUTOR_DATE, OSS_AUTOR_NAME, OSS_AUTOR_MAIL);
    plugin_inout->license = g_strdup(OSS_SHORT_LICENSE);

    plugin_inout->is_usable = g_malloc(sizeof(gint));
    *plugin_inout->is_usable = 0;
    plugin_inout->is_selected = g_malloc(sizeof(gint));
    *plugin_inout->is_selected = 1;

    plugin_inout->configure = oss_configure;
    plugin_inout->about = oss_about;
    plugin_inout->init = oss_init;
    plugin_inout->cleanup = NULL;

    plugin_inout->open_in = oss_open_in;
    plugin_inout->open_out = oss_open_out;

    plugin_inout->close_out = oss_close_out;
    plugin_inout->close_in = oss_close_in;

    plugin_inout->read = oss_read;
    plugin_inout->write = oss_write;

    plugin_inout->get_in_volume = oss_get_in_volume;
    plugin_inout->set_in_volume = oss_set_in_volume;

    plugin_inout->get_out_volume = oss_get_out_volume;
    plugin_inout->set_out_volume = oss_set_out_volume;

    plugin_inout->flush = oss_flush;
    plugin_inout->get_caps = oss_get_caps;

    oss_cfg = (Oss_Conf *) g_malloc0(sizeof(Oss_Conf));

    return NULL;
}



G_MODULE_EXPORT void g_module_unload (GModule *module)
{
    UNUSED(module);

    g_free(plugin_inout->name);
    g_free(plugin_inout->filename);
    g_free(plugin_inout->description);
    g_free(plugin_inout->license);
    g_free(plugin_inout->is_usable);
    g_free(plugin_inout->is_selected);

    g_free(plugin_inout);
    g_free(oss_cfg);
    return;
}



G_MODULE_EXPORT Plugin_InOut *get_Plugin_InOut_struct (void)
{
    return plugin_inout;
}




// Private functions
// Funciones privadas



static inline gint AFormat_2_oss (gint format)
{
    switch (format)
    {
    case FMT_U8: return AFMT_U8;
    case FMT_S8: return AFMT_S8;
    case FMT_U16_LE: return AFMT_U16_LE;
    case FMT_U16_BE: return AFMT_U16_BE;
    case FMT_S16_LE: return AFMT_S16_LE;
    case FMT_S16_BE: return AFMT_S16_BE;
    default : return -1;
    }
}



static inline gint oss_2_AFormat (gint format)
{
    switch (format)
    {
    case AFMT_U8: return FMT_U8;
    case AFMT_S8: return FMT_S8;
    case AFMT_U16_LE: return FMT_U16_LE;
    case AFMT_U16_BE: return FMT_U16_BE;
    case AFMT_S16_LE: return FMT_S16_LE;
    case AFMT_S16_BE: return FMT_S16_BE;
    default : return -1;
    }
}



static void insert_text_handler (GtkEditable *editable, const gchar *text, gint length, gint *position, gpointer data)
{
    g_signal_handlers_block_by_func(GTK_OBJECT(editable), insert_text_handler, data);
    gtk_editable_insert_text(GTK_EDITABLE(editable), text, length, position);
    gtk_editable_insert_text(GTK_EDITABLE(data), text, length, position);
    g_signal_handlers_unblock_by_func(GTK_OBJECT(editable), insert_text_handler, data);
    g_signal_stop_emission_by_name(GTK_OBJECT (editable), "insert_text");
    return;
}



static void delete_text_handler (GtkEditable *editable, gint start_pos, gint end_pos, gpointer data)
{
    g_signal_handlers_block_by_func(GTK_OBJECT(editable), delete_text_handler, data);
    gtk_editable_delete_text(GTK_EDITABLE(editable), start_pos, end_pos);
    gtk_editable_delete_text(GTK_EDITABLE(data), start_pos, end_pos);
    g_signal_handlers_unblock_by_func(GTK_OBJECT(editable), delete_text_handler, data);
    g_signal_stop_emission_by_name(GTK_OBJECT(editable), "delete-text");
    return;
}



static gint oss_configure (void)
{
    static GtkWidget *dialog;
    GtkWidget *frame_in;
    GtkWidget *frame_out;
    GtkWidget *label;
    GtkWidget *entry_device_in;
    GtkWidget *entry_mixer_in;
    GtkWidget *table_in;
    GtkWidget *entry_device_out;
    GtkWidget *entry_mixer_out;
    GtkWidget *table_out;
    GtkWidget *option_menu;
    GtkWidget *option_menu_vol_control;
    GtkWidget *menu;
    GtkWidget *menu_vol_control;
    gchar *text;
    gint result;
    const char **str;
    GtkTooltips *tips_help;


    if (dialog != NULL)
    {
	gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_NONE);
	return GTK_RESPONSE_NONE;
    }
    if (!gtk_init_check (NULL, NULL)) return PLUGIN_INOUT_ERROR;

    dialog = gtk_dialog_new_with_buttons("Configuracion del Plugin de E/S audio OSS", NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_STOCK_APPLY, GTK_RESPONSE_APPLY,
					 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 270, 351);
    gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);
    tips_help = gtk_tooltips_new ();

    frame_in = gtk_frame_new("Audio In");
    gtk_container_set_border_width(GTK_CONTAINER(frame_in), 5);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), frame_in, FALSE, FALSE, 0);

    table_in = gtk_table_new(3, 2, FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(table_in), 5);
    gtk_container_add(GTK_CONTAINER(frame_in), table_in);

    label = gtk_label_new("DSP Device");
    gtk_table_attach(GTK_TABLE(table_in), label, 0, 1, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 5, 5);

    entry_device_in = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(entry_device_in), 256);
    gtk_entry_set_width_chars(GTK_ENTRY(entry_device_in), 20);
    gtk_table_attach(GTK_TABLE(table_in), entry_device_in, 1, 2, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 5, 5);
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), entry_device_in,
			  "Dispositivo DSP de entrada de audio, en la mayoria de los casos es '/dev/dsp'.", NULL );

    label = gtk_label_new("Mixer Device");
    gtk_table_attach(GTK_TABLE(table_in), label, 0, 1, 1, 2, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 5, 5);

    entry_mixer_in = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(entry_mixer_in), 256);
    gtk_entry_set_width_chars(GTK_ENTRY(entry_mixer_in), 20);
    gtk_table_attach(GTK_TABLE(table_in), entry_mixer_in, 1, 2, 1, 2, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 5, 5);
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), entry_mixer_in,
			  "Dispositivo mezclador del audio de entrada. Este dispositivo controla la ganancia de entrada y la fuente del audio, generalmente es '/dev/mixer'.", NULL );

    label = gtk_label_new("Audio Source  ");
    gtk_table_attach(GTK_TABLE(table_in), label, 0, 1, 2, 3, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 5, 5);
    menu = gtk_menu_new();
    for (str = source_audio_in_options; *str; str++)
    {
	GtkWidget *menu_item = gtk_menu_item_new_with_label(*str);
	gtk_widget_show(menu_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    }
    option_menu = gtk_option_menu_new();
    gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
    gtk_table_attach(GTK_TABLE(table_in), option_menu,  1, 2, 2, 3, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 5, 5);
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), option_menu,
			  "Fuente de entrada del audio, generalmente es el microfono (Mic).", NULL );


    frame_out = gtk_frame_new("Audio Out");
    gtk_container_set_border_width(GTK_CONTAINER(frame_out), 5);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), frame_out, FALSE, FALSE, 0);

    table_out = gtk_table_new(3, 2, FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(table_out), 5);
    gtk_container_add(GTK_CONTAINER(frame_out), table_out);

    label = gtk_label_new("DSP Device");
    gtk_table_attach(GTK_TABLE(table_out), label, 0, 1, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 5, 5);

    entry_device_out = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(entry_device_out), 256);
    gtk_entry_set_width_chars(GTK_ENTRY(entry_device_out), 20);
    gtk_table_attach(GTK_TABLE(table_out), entry_device_out, 1, 2, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 5, 5);
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), entry_device_out,
			  "Dispositivo DSP de salida de audio. Normalmente coincide con el dispositivo de entrada, de esa forma el modulo inicia el 'device driver' en modo 'full duplex' (si el hardware lo soporta), en caso contrario se abre el dispositivo en modo 'half duplex'. En la mayoria de los casos es '/dev/dsp' (modo 'full duplex' activado).", NULL );

    label = gtk_label_new("Mixer Device");
    gtk_table_attach(GTK_TABLE(table_out), label, 0, 1, 1, 2, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 5, 5);

    entry_mixer_out = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(entry_mixer_out), 256);
    gtk_entry_set_width_chars(GTK_ENTRY(entry_mixer_out), 20);
    gtk_table_attach(GTK_TABLE(table_out), entry_mixer_out, 1, 2, 1, 2, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 5, 5);
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), entry_mixer_out,
			  "Dispositivo mezclador del audio de salida. Este dispositivo controla el volumen, generalmente es '/dev/mixer' en modo 'full duplex' (el mismo que el mezclador de entrada).", NULL );

    label = gtk_label_new("Volume Control");
    gtk_table_attach(GTK_TABLE(table_out), label, 0, 1, 2, 3, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 5, 5);
    menu_vol_control = gtk_menu_new ();
    for (str = volume_control_options; *str; str++)
    {
	GtkWidget *menu_item = gtk_menu_item_new_with_label(*str);
	gtk_widget_show(menu_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_vol_control), menu_item);
    }
    option_menu_vol_control = gtk_option_menu_new();
    gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu_vol_control), menu_vol_control);
    gtk_table_attach(GTK_TABLE(table_out), option_menu_vol_control,  1, 2, 2, 3, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 5, 5);
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), option_menu_vol_control,
			  "Determina el canal del mezclador que controla el volumen del audio. Generalmente es 'PCM Wave'.", NULL );

    label = gtk_label_new("Los cambios no seran efectivos hasta que\nse reinicie el modulo.");
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label, FALSE, FALSE, 15);


    if (oss_cfg->audio_device_in != NULL) gtk_entry_set_text(GTK_ENTRY(entry_device_in), oss_cfg->audio_device_in);
    if (oss_cfg->mixer_device_in != NULL)gtk_entry_set_text(GTK_ENTRY(entry_mixer_in), oss_cfg->mixer_device_in);
    gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu), oss_cfg->audio_source_in);
    if (oss_cfg->audio_device_out != NULL) gtk_entry_set_text(GTK_ENTRY(entry_device_out), oss_cfg->audio_device_out);
    if (oss_cfg->mixer_device_out != NULL) gtk_entry_set_text(GTK_ENTRY(entry_mixer_out), oss_cfg->mixer_device_out);
    gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu_vol_control), oss_cfg->volume_control_pcm);

    g_signal_connect(G_OBJECT(entry_device_in), "insert_text", G_CALLBACK(insert_text_handler), entry_device_out);
    g_signal_connect(G_OBJECT(entry_mixer_in), "insert_text", G_CALLBACK(insert_text_handler), entry_mixer_out);
    g_signal_connect(G_OBJECT(entry_device_in), "delete_text", G_CALLBACK(delete_text_handler), entry_device_out);
    g_signal_connect(G_OBJECT(entry_mixer_in), "delete_text", G_CALLBACK(delete_text_handler), entry_mixer_out);

    gtk_widget_show_all(dialog);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_APPLY);
    result = gtk_dialog_run(GTK_DIALOG(dialog));

    if (result == GTK_RESPONSE_APPLY)
    {
	text = gtk_editable_get_chars(GTK_EDITABLE(entry_device_in), 0, -1);
	cfgf_write_string(configuration, OSS_CFG_SECTION, OSS_CFG_AUDIO_DEVICE_IN, text);
	g_free(text);
	text = gtk_editable_get_chars(GTK_EDITABLE(entry_mixer_in), 0, -1);
	cfgf_write_string(configuration, OSS_CFG_SECTION, OSS_CFG_MIXER_DEVICE_IN, text);
	g_free(text);

	cfgf_write_int(configuration, OSS_CFG_SECTION, OSS_CFG_SOURCE_IN, gtk_option_menu_get_history(GTK_OPTION_MENU(option_menu)));

	text = gtk_editable_get_chars(GTK_EDITABLE(entry_device_out), 0, -1);
	cfgf_write_string(configuration, OSS_CFG_SECTION, OSS_CFG_AUDIO_DEVICE_OUT, text);
	g_free(text);
	text = gtk_editable_get_chars(GTK_EDITABLE(entry_mixer_in), 0, -1);
	cfgf_write_string(configuration, OSS_CFG_SECTION, OSS_CFG_MIXER_DEVICE_OUT, text);
	g_free(text);

	cfgf_write_int(configuration, OSS_CFG_SECTION, OSS_CFG_VOL_CONTROL, gtk_option_menu_get_history(GTK_OPTION_MENU(option_menu_vol_control)));
    }
    gtk_widget_destroy(dialog);
    gtk_widget_destroyed(dialog, &dialog);

    while (gtk_events_pending()) gtk_main_iteration();

    return result;
}



static gint oss_about (void)
{
    static GtkWidget *dialog;
    gint result;


    if (dialog != NULL)
    {
	gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_NONE);
	return GTK_RESPONSE_NONE;
    }
    if (!gtk_init_check (NULL, NULL)) return PLUGIN_INOUT_ERROR;

    dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
				    "\nAbout OSS Plugin:\n\n\n(c) %s %s %s\nOSS Dynamic Load Library\n\n\n%s\n",
				    OSS_AUTOR_DATE, OSS_AUTOR_NAME, OSS_AUTOR_MAIL, OSS_LONG_LICENSE);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    gtk_widget_destroyed(dialog, &dialog);

    while (gtk_events_pending()) gtk_main_iteration();

    return result;
}



static gint oss_init (ConfigFile *cfgfile, GError **error)
{
    gboolean value;


    oss_cfg->fd_audio_device_in = -1;
    oss_cfg->fd_audio_device_out = -1;
    oss_cfg->audio_full_duplex = FALSE;

    if (cfgfile != NULL)
    {
	configuration = cfgfile;
	value = TRUE;
	value &= cfgf_read_string(cfgfile, OSS_CFG_SECTION, OSS_CFG_AUDIO_DEVICE_IN, &oss_cfg->audio_device_in);
	value &= cfgf_read_string(cfgfile, OSS_CFG_SECTION, OSS_CFG_MIXER_DEVICE_IN, &oss_cfg->mixer_device_in);
	value &= cfgf_read_int(cfgfile, OSS_CFG_SECTION, OSS_CFG_SOURCE_IN, &oss_cfg->audio_source_in);
	value &= cfgf_read_string(cfgfile, OSS_CFG_SECTION, OSS_CFG_AUDIO_DEVICE_OUT, &oss_cfg->audio_device_out);
	value &= cfgf_read_string(cfgfile, OSS_CFG_SECTION, OSS_CFG_MIXER_DEVICE_OUT, &oss_cfg->mixer_device_out);
	value &= cfgf_read_int(cfgfile, OSS_CFG_SECTION, OSS_CFG_VOL_CONTROL, &oss_cfg->volume_control_pcm);
	if (!value)
	{
	    g_set_error(error, PLUGIN_INOUT_AUDIO_ERROR, PLUGIN_INOUT_AUDIO_ERROR_CFGFILE_READ, "configuracion invalida o incorrecta");
	    return PLUGIN_INOUT_AUDIO_ERROR_CFGFILE_READ;
	}
    }
    else
    {
	g_set_error(error, PLUGIN_INOUT_AUDIO_ERROR, PLUGIN_INOUT_AUDIO_ERROR_CFGFILE_READ, "sin configuracion");
	return PLUGIN_INOUT_AUDIO_ERROR_NOCFGFILE;
    }

    if (oss_cfg->mixer_device_in != NULL) oss_cfg->mixer_in = TRUE;
    else oss_cfg->mixer_in = FALSE;

    if (oss_cfg->mixer_device_out != NULL) oss_cfg->mixer_out = TRUE;
    else oss_cfg->mixer_out = FALSE;

    if (g_ascii_strcasecmp(oss_cfg->audio_device_in, oss_cfg->audio_device_out) == 0)
    {
	oss_cfg->audio_full_duplex = TRUE;
	if (oss_cfg->mixer_out || oss_cfg->mixer_out)
	{
	    oss_cfg->mixer_out = TRUE;
	    oss_cfg->mixer_in = TRUE;
	}
    }
    return PLUGIN_INOUT_OK;
}



static gint oss_open_in (gint open_flags, AFormat format, gint channels, gint rate, gint fragmet, AInfo *info, GError **error)
{
    gboolean control;
    gint caps;
    gint fd;
    gint fd_mixer;
    gint fmt;
    gint frag;
    gint caps_set;


    info->channels = channels;
    info->rate = rate;
    info->blocksize = 0;
    info->format = format;
    info->mixer = FALSE;
    fmt = AFormat_2_oss(format);

    if (oss_cfg->audio_full_duplex)
    {
	if (!device_out_init)
	{
	    if ((fd = open(oss_cfg->audio_device_in, O_RDWR)) == -1)
	    {
		g_set_error(error, PLUGIN_INOUT_AUDIO_ERROR, PLUGIN_INOUT_AUDIO_ERROR_ERRNO, "%s : %s",oss_cfg->audio_device_in, g_strerror(errno));
		return PLUGIN_INOUT_AUDIO_ERROR_OPEN;
	    }
	    //ioctl(fd, SNDCTL_DSP_RESET, 0);
	    frag = (OSS_NUMBER_FRAGS)|(fragmet);
	    if (ioctl(fd, SNDCTL_DSP_SETFRAGMENT, &frag) == -1)
	    {
		info->blocksize = -1;
		close(fd);
		g_set_error(error, PLUGIN_INOUT_AUDIO_ERROR, PLUGIN_INOUT_AUDIO_ERROR_ERRNO, "imposible establecer tamaño minimo de bloque a %X (%s)", frag, g_strerror(errno));
		return PLUGIN_INOUT_AUDIO_ERROR_SETBLOCKSIZE;
	    }
	    if (ioctl(fd, SNDCTL_DSP_GETCAPS, &caps) == -1)
	    {
		close(fd);
		g_set_error(error, PLUGIN_INOUT_AUDIO_ERROR, PLUGIN_INOUT_AUDIO_ERROR_ERRNO, "imposible obtener capacidades del hardware (%s)", g_strerror(errno));
		return PLUGIN_INOUT_AUDIO_ERROR_GETCAPS;
	    }
	    if (!(caps & DSP_CAP_DUPLEX))
	    {
		close(fd);
		g_set_error(error, PLUGIN_INOUT_AUDIO_ERROR, PLUGIN_INOUT_AUDIO_ERROR_ERRNO, "el hardware no soporta modo full duplex");
		return PLUGIN_INOUT_AUDIO_ERROR_SETDUPLEX;
	    }
	    if (ioctl(fd, SNDCTL_DSP_SETDUPLEX, 0) == -1)
	    {
		close(fd);
		g_set_error(error, PLUGIN_INOUT_AUDIO_ERROR, PLUGIN_INOUT_AUDIO_ERROR_ERRNO, "imposible establecer el modo full duplex (%s)", g_strerror(errno));
		return PLUGIN_INOUT_AUDIO_ERROR_SETDUPLEX;
	    }
	    control = TRUE;
	}
	else
	{
	    info->channels = oss_cfg->audio_channels_out;
	    info->rate = oss_cfg->audio_rate_out;
	    info->format = oss_cfg->audio_format_out;
	    info->blocksize = oss_cfg->audio_blocksize_out;
            fd = oss_cfg->fd_audio_device_out;
            control = FALSE;
	}
    }
    else
    {
	if ((fd = open(oss_cfg->audio_device_in, open_flags)) == -1)
	{
	    g_set_error(error, PLUGIN_INOUT_AUDIO_ERROR, PLUGIN_INOUT_AUDIO_ERROR_ERRNO, "%s : %s", oss_cfg->audio_device_in, g_strerror(errno));
	    return PLUGIN_INOUT_AUDIO_ERROR_OPEN;
	}
	//ioctl(fd, SNDCTL_DSP_RESET, 0);
	frag = (OSS_NUMBER_FRAGS)|(fragmet);
	if (ioctl(fd, SNDCTL_DSP_SETFRAGMENT, &frag) == -1)
	{
	    info->blocksize = -1;
	    close(fd);
	    g_set_error(error, PLUGIN_INOUT_AUDIO_ERROR, PLUGIN_INOUT_AUDIO_ERROR_ERRNO, "imposible establecer tamaño minimo de bloque a %X (%s)", frag, g_strerror(errno));
	    return PLUGIN_INOUT_AUDIO_ERROR_SETBLOCKSIZE;
	}
	control = TRUE;
    }
    if (control)
    {
	if (ioctl(fd, SNDCTL_DSP_SETFMT, &fmt) == -1)
	{
	    close(fd);
	    g_set_error(error, PLUGIN_INOUT_AUDIO_ERROR, PLUGIN_INOUT_AUDIO_ERROR_ERRNO, "%s", g_strerror(errno));
	    return PLUGIN_INOUT_AUDIO_ERROR_SETFORMAT;
	}
	info->format = oss_2_AFormat(fmt);
	if (fmt != AFormat_2_oss(info->format))
	{
	    close(fd);
	    return PLUGIN_INOUT_AUDIO_ERROR_SETFORMAT;
	}
	if (ioctl(fd, SNDCTL_DSP_CHANNELS, &info->channels) == -1)
	{
	    close(fd);
	    g_set_error(error, PLUGIN_INOUT_AUDIO_ERROR, PLUGIN_INOUT_AUDIO_ERROR_ERRNO, "%s", g_strerror(errno));
	    return PLUGIN_INOUT_AUDIO_ERROR_CHANNELS;
	}
	if (info->channels != channels)
	{
	    close(fd);
	    return PLUGIN_INOUT_AUDIO_ERROR_CHANNELS;
	}
	if (ioctl(fd, SNDCTL_DSP_SPEED, &info->rate) == -1)
	{
	    close(fd);
	    g_set_error(error, PLUGIN_INOUT_AUDIO_ERROR, PLUGIN_INOUT_AUDIO_ERROR_ERRNO, "%s", g_strerror(errno));
	    return PLUGIN_INOUT_AUDIO_ERROR_RATE;
	}
	if (info->rate != rate)
	{
	    close(fd);
	    return PLUGIN_INOUT_AUDIO_ERROR_RATE;
	}
	if (ioctl(fd, SNDCTL_DSP_GETBLKSIZE, &info->blocksize) == -1)
	{
	    close(fd);
	    g_set_error(error, PLUGIN_INOUT_AUDIO_ERROR, PLUGIN_INOUT_AUDIO_ERROR_ERRNO, "%s", g_strerror(errno));
	    return PLUGIN_INOUT_AUDIO_ERROR_GETBLOCKSIZE;
	}
    }
    oss_cfg->audio_blocksize_in = info->blocksize;
    oss_cfg->audio_rate_in = info->rate;
    oss_cfg->audio_channels_in = info->channels;
    oss_cfg->audio_format_in = info->format;
    oss_cfg->fd_audio_device_in = fd;

    if (oss_cfg->mixer_in)
    {
	if ((fd_mixer = open (oss_cfg->mixer_device_in, O_RDWR)) == -1)
	{
	    close(fd);
	    g_set_error(error, PLUGIN_INOUT_AUDIO_MIXER_ERROR, PLUGIN_INOUT_AUDIO_MIXER_ERROR_ERRNO, "%s : %s", oss_cfg->mixer_device_in, g_strerror(errno));
	    return PLUGIN_INOUT_AUDIO_MIXER_ERROR_OPEN;
	}
	if (ioctl(fd_mixer, MIXER_READ(SOUND_MIXER_READ_RECMASK), &caps) == -1)
	{
	    close(fd);
	    close(fd_mixer);
	    g_set_error(error, PLUGIN_INOUT_AUDIO_MIXER_ERROR, PLUGIN_INOUT_AUDIO_MIXER_ERROR_ERRNO, "%s", g_strerror(errno));
	    return PLUGIN_INOUT_AUDIO_MIXER_ERROR_SOURCE;
	}
	switch (oss_cfg->audio_source_in)
	{
	case 0:
	    caps_set = 1 << SOUND_MIXER_MIC;
	    break;
	case 1:
	    caps_set = 1 << SOUND_MIXER_LINE;
	    break;
	case 2:
	    caps_set = 1 << SOUND_MIXER_CD;
	    break;
	}
	if (!(caps & caps_set))
	{
	    close(fd);
	    close(fd_mixer);
	    return PLUGIN_INOUT_AUDIO_MIXER_ERROR_SOURCE;
	}
	if (ioctl(fd_mixer, MIXER_WRITE(SOUND_MIXER_WRITE_RECSRC), &caps_set) == -1)
	{
	    close(fd);
	    close(fd_mixer);
	    g_set_error(error, PLUGIN_INOUT_AUDIO_MIXER_ERROR, PLUGIN_INOUT_AUDIO_MIXER_ERROR_ERRNO, "%s", g_strerror(errno));
	    return PLUGIN_INOUT_AUDIO_MIXER_ERROR_SOURCE;
	}
	close(fd_mixer);
	info->mixer = TRUE;
    }
    device_in_init = TRUE;
    return PLUGIN_INOUT_OK;
}



static gint oss_open_out (gint open_flags, AFormat format, gint channels, gint rate, gint fragmet, AInfo *info, GError **error)
{
    gboolean control;
    gint caps;
    gint fd;
    gint fd_mixer;
    gint fmt;
    gint frag;


    info->channels = channels;
    info->rate = rate;
    info->blocksize = 0;
    info->format = format;
    info->mixer = FALSE;
    fmt = AFormat_2_oss(format);

    if (oss_cfg->audio_full_duplex)
    {
	if (!device_in_init)
	{
	    if ((fd = open(oss_cfg->audio_device_out, O_RDWR)) == -1)
	    {
		g_set_error(error, PLUGIN_INOUT_AUDIO_ERROR, PLUGIN_INOUT_AUDIO_ERROR_ERRNO, "%s : %s",oss_cfg->audio_device_out, g_strerror(errno));
		return PLUGIN_INOUT_AUDIO_ERROR_OPEN;
	    }
	    //ioctl(fd, SNDCTL_DSP_RESET, 0);
	    frag = (OSS_NUMBER_FRAGS)|(fragmet);
	    if (ioctl(fd, SNDCTL_DSP_SETFRAGMENT, &frag) == -1)
	    {
		info->blocksize = -1;
		close(fd);
		g_set_error(error, PLUGIN_INOUT_AUDIO_ERROR, PLUGIN_INOUT_AUDIO_ERROR_ERRNO, "imposible establecer tamaño minimo de bloque a %X (%s)", frag, g_strerror(errno));
		return PLUGIN_INOUT_AUDIO_ERROR_SETBLOCKSIZE;
	    }
	    if (ioctl(fd, SNDCTL_DSP_GETCAPS, &caps) == -1)
	    {
		close(fd);
		g_set_error(error, PLUGIN_INOUT_AUDIO_ERROR, PLUGIN_INOUT_AUDIO_ERROR_ERRNO, "imposible obtener capacidades del hardware (%s)", g_strerror(errno));
		return PLUGIN_INOUT_AUDIO_ERROR_GETCAPS;
	    }
	    if (!(caps & DSP_CAP_DUPLEX))
	    {
		close(fd);
		g_set_error(error, PLUGIN_INOUT_AUDIO_ERROR, PLUGIN_INOUT_AUDIO_ERROR_ERRNO, "el hardware no soporta modo full duplex");
		return PLUGIN_INOUT_AUDIO_ERROR_SETDUPLEX;
	    }
	    if (ioctl(fd, SNDCTL_DSP_SETDUPLEX, 0) == -1)
	    {
		close(fd);
		g_set_error(error, PLUGIN_INOUT_AUDIO_ERROR, PLUGIN_INOUT_AUDIO_ERROR_ERRNO, "imposible establecer el modo full duplex (%s)", g_strerror(errno));
		return PLUGIN_INOUT_AUDIO_ERROR_SETDUPLEX;
	    }
	    control = TRUE;
	}
	else
	{
	    info->channels = oss_cfg->audio_channels_in;
	    info->rate = oss_cfg->audio_rate_in;
	    info->format = oss_cfg->audio_format_in;
	    info->blocksize = oss_cfg->audio_blocksize_in;
	    fd = oss_cfg->fd_audio_device_in;
            control = FALSE;
	}
    }
    else
    {
	if ((fd = open(oss_cfg->audio_device_out, open_flags)) == -1)
	{
	    g_set_error(error, PLUGIN_INOUT_AUDIO_ERROR, PLUGIN_INOUT_AUDIO_ERROR_ERRNO, "%s : %s",oss_cfg->audio_device_out, g_strerror(errno));
	    return PLUGIN_INOUT_AUDIO_ERROR_OPEN;
	}
	//ioctl(fd, SNDCTL_DSP_RESET, 0);
	frag = (OSS_NUMBER_FRAGS)|(fragmet);
	if (ioctl(fd, SNDCTL_DSP_SETFRAGMENT, &frag) == -1)
	{
	    info->blocksize = -1;
	    close(fd);
	    g_set_error(error, PLUGIN_INOUT_AUDIO_ERROR, PLUGIN_INOUT_AUDIO_ERROR_ERRNO, "imposible establecer tamaño minimo de bloque a %X (%s)", frag, g_strerror(errno));
	    return PLUGIN_INOUT_AUDIO_ERROR_SETBLOCKSIZE;
	}
	control = TRUE;
    }
    if (control)
    {
	if (ioctl(fd, SNDCTL_DSP_SETFMT, &fmt) == -1)
	{
	    close(fd);
	    g_set_error(error, PLUGIN_INOUT_AUDIO_ERROR, PLUGIN_INOUT_AUDIO_ERROR_ERRNO, "%s", g_strerror(errno));
	    return PLUGIN_INOUT_AUDIO_ERROR_SETFORMAT;
	}
	info->format = oss_2_AFormat(fmt);
	if (fmt != AFormat_2_oss(info->format))
	{
	    close(fd);
	    return PLUGIN_INOUT_AUDIO_ERROR_SETFORMAT;
	}
	if (ioctl(fd, SNDCTL_DSP_CHANNELS, &info->channels) == -1)
	{
	    close(fd);
	    g_set_error(error, PLUGIN_INOUT_AUDIO_ERROR, PLUGIN_INOUT_AUDIO_ERROR_ERRNO, "%s", g_strerror(errno));
	    return PLUGIN_INOUT_AUDIO_ERROR_CHANNELS;
	}
	if (info->channels != channels)
	{
	    close(fd);
	    return PLUGIN_INOUT_AUDIO_ERROR_CHANNELS;
	}
	if (ioctl(fd, SNDCTL_DSP_SPEED, &info->rate) == -1)
	{
	    close(fd);
	    g_set_error(error, PLUGIN_INOUT_AUDIO_ERROR, PLUGIN_INOUT_AUDIO_ERROR_ERRNO, "%s", g_strerror(errno));
	    return PLUGIN_INOUT_AUDIO_ERROR_RATE;
	}
	if (info->rate != rate)
	{
	    close(fd);
	    return PLUGIN_INOUT_AUDIO_ERROR_RATE;
	}
	if (ioctl(fd, SNDCTL_DSP_GETBLKSIZE, &info->blocksize) == -1)
	{
	    close(fd);
	    g_set_error(error, PLUGIN_INOUT_AUDIO_ERROR, PLUGIN_INOUT_AUDIO_ERROR_ERRNO, "%s", g_strerror(errno));
	    return PLUGIN_INOUT_AUDIO_ERROR_GETBLOCKSIZE;
	}
    }
    oss_cfg->audio_blocksize_out = info->blocksize;
    oss_cfg->audio_rate_out = info->rate;
    oss_cfg->audio_channels_out = info->channels;
    oss_cfg->audio_format_out = info->format;
    oss_cfg->fd_audio_device_out = fd;

    if (oss_cfg->mixer_out)
    {
	if ((fd_mixer = open (oss_cfg->mixer_device_out, O_RDWR)) == -1)
	{
	    close(fd);
	    g_set_error(error, PLUGIN_INOUT_AUDIO_MIXER_ERROR, PLUGIN_INOUT_AUDIO_MIXER_ERROR_ERRNO, "%s : %s", oss_cfg->mixer_device_out, g_strerror(errno));
	    return PLUGIN_INOUT_AUDIO_MIXER_ERROR_OPEN;
	}
	close(fd_mixer);
	info->mixer = TRUE;
    }
    device_out_init = TRUE;
    return PLUGIN_INOUT_OK;
}



static gint oss_close_in (void)
{
    if (oss_cfg->audio_full_duplex)
    {
	if (!(device_out_init)) close(oss_cfg->fd_audio_device_in);
    }
    else
    {
	close (oss_cfg->fd_audio_device_in);
    }
    device_in_init = FALSE;
    oss_cfg->mixer_in = FALSE;
    oss_cfg->fd_audio_device_in = -1;
    return PLUGIN_INOUT_OK;
}



static gint oss_close_out (void)
{
    if (oss_cfg->audio_full_duplex)
    {
	if (!(device_in_init)) close(oss_cfg->fd_audio_device_out);
    }
    else
    {
	close(oss_cfg->fd_audio_device_out);
    }
    device_out_init = FALSE;
    oss_cfg->mixer_out = FALSE;
    oss_cfg->fd_audio_device_out = -1;
    return PLUGIN_INOUT_OK;
}



static gint oss_flush (gint time)
{
    UNUSED(time);

    ioctl(oss_cfg->fd_audio_device_in, SNDCTL_DSP_SYNC, 0);
    if (!oss_cfg->audio_full_duplex) ioctl(oss_cfg->fd_audio_device_out, SNDCTL_DSP_SYNC, 0);

    // ¿?
    return 0;
}



static ACaps *oss_get_caps (void)
{
    ACaps *caps;
    audio_buf_info info;
    gint fmt;
    gint mask;


    if (!(device_in_init)) return NULL;
    if (!oss_cfg->audio_full_duplex)
    {
	if (!(device_out_init)) return NULL;
    }
    caps = (ACaps *) g_malloc(sizeof(ACaps));

    caps->device_in = g_strdup(oss_cfg->audio_device_in);
    caps->mixer_in = g_strdup(oss_cfg->mixer_device_in);
    caps->device_out = g_strdup(oss_cfg->audio_device_out);
    caps->mixer_out = g_strdup(oss_cfg->mixer_device_out);
    caps->rate_in = oss_cfg->audio_rate_in;
    caps->rate_out = oss_cfg->audio_rate_out;
    caps->op_blocksize_in = oss_cfg->audio_blocksize_in;
    caps->op_blocksize_out = oss_cfg->audio_blocksize_out;
    caps->channels_in = oss_cfg->audio_channels_in;
    caps->channels_out = oss_cfg->audio_channels_out;

    fmt = AFMT_QUERY;
    if (oss_cfg->audio_full_duplex)
    {
	ioctl(oss_cfg->fd_audio_device_in, SNDCTL_DSP_SETFMT, &fmt);
	caps->format_in_now = oss_2_AFormat(fmt);
	caps->format_out_now = caps->format_in_now;
    }
    else
    {
	ioctl(oss_cfg->fd_audio_device_in, SNDCTL_DSP_SETFMT, &fmt);
	caps->format_in_now = oss_2_AFormat(fmt);
	fmt = AFMT_QUERY;
	ioctl(oss_cfg->fd_audio_device_out, SNDCTL_DSP_SETFMT, &fmt);
	caps->format_out_now = oss_2_AFormat(fmt);
    }
    //caps->format_in_now = oss_cfg->audio_format_in;
    //caps->format_out_now = oss_cfg->audio_format_out;

    ioctl(oss_cfg->fd_audio_device_in, SNDCTL_DSP_GETFMTS, &mask);
    caps->u8_format_out = caps->u8_format_in = mask & AFMT_U8 ? TRUE : FALSE;
    caps->s8_format_out = caps->s8_format_in = mask & AFMT_S8 ? TRUE : FALSE;
    caps->u16le_format_out = caps->u16le_format_in = mask & AFMT_U16_LE ? TRUE : FALSE;
    caps->u16be_format_out = caps->u16be_format_in = mask & AFMT_U16_BE ? TRUE : FALSE;
    caps->s16le_format_out = caps->s16le_format_in = mask & AFMT_S16_LE ? TRUE : FALSE;
    caps->s16be_format_out = caps->s16be_format_in = mask & AFMT_S16_BE ? TRUE : FALSE;

    if (!oss_cfg->audio_full_duplex)
    {
	ioctl(oss_cfg->fd_audio_device_out, SNDCTL_DSP_GETFMTS, &mask);
	caps->u8_format_out = mask & AFMT_U8 ? TRUE : FALSE;
	caps->s8_format_out = mask & AFMT_S8 ? TRUE : FALSE;
	caps->u16le_format_out = mask & AFMT_U16_LE ? TRUE : FALSE;
	caps->u16be_format_out = mask & AFMT_U16_BE ? TRUE : FALSE;
	caps->s16le_format_out = mask & AFMT_S16_LE ? TRUE : FALSE;
	caps->s16be_format_out = mask & AFMT_S16_BE ? TRUE : FALSE;
    }
    caps->block_read = TRUE;
    caps->block_write = FALSE;
    oss_get_in_volume(&caps->volume_in_l, &caps->volume_in_r);
    oss_get_out_volume(&caps->volume_out_l, &caps->volume_out_r);

    ioctl(oss_cfg->fd_audio_device_in, SNDCTL_DSP_GETISPACE, &info);
    caps->fragments_in = info.fragments;
    caps->fragstotal_in = info.fragstotal;
    caps->fragsize_in = info.fragsize;
    caps->bytes_in = info.bytes;
    ioctl(oss_cfg->fd_audio_device_out, SNDCTL_DSP_GETOSPACE, &info);
    caps->fragments_out = info.fragments;
    caps->fragstotal_out = info.fragstotal;
    caps->fragsize_out = info.fragsize;
    caps->bytes_out = info.bytes;

    caps->fflush = TRUE;
    caps->trigger = FALSE;
    caps->mmap = FALSE;
    caps->full_duplex = oss_cfg->audio_full_duplex;

    return caps;
}



static gint oss_read (gpointer buffer, gint l)
{
    if (device_in_init)
    {
	if (l == 0)
	    return read(oss_cfg->fd_audio_device_in, buffer, oss_cfg->audio_blocksize_in);
	else
	    return read(oss_cfg->fd_audio_device_in, buffer, l);
    }
    return 0;
}



static gint oss_write (gpointer buffer, gint l)
{
    if (device_out_init)
    {
	if (l == 0)
	    return write(oss_cfg->fd_audio_device_out, buffer, oss_cfg->audio_blocksize_out);
	else
	    return write(oss_cfg->fd_audio_device_out, buffer, l);
    }
    return 0;
}



static gint oss_get_out_volume (gint *l_vol, gint *r_vol)
{
    gint fd;
    gint volumen;
    gint cmd;
    gint mask;


    if (oss_cfg->mixer_out)
    {
	if ((fd = open(oss_cfg->mixer_device_out, O_RDONLY)) == -1) return PLUGIN_INOUT_AUDIO_MIXER_ERROR_OPEN;

	if (ioctl(fd, SOUND_MIXER_READ_DEVMASK, &mask) == -1)
	{
	    close (fd);
	    return PLUGIN_INOUT_AUDIO_MIXER_ERROR_GET_CAPS;
	}
	if ((mask & SOUND_MASK_PCM) && (oss_cfg->volume_control_pcm == 0))
	{
	    cmd = SOUND_MIXER_READ_PCM;
	}
	else
	{
	    if ((mask & SOUND_MASK_VOLUME) && (oss_cfg->volume_control_pcm == 1))
	    {
		cmd = SOUND_MIXER_READ_VOLUME;
	    }
	    else
	    {
		close(fd);
		return PLUGIN_INOUT_AUDIO_MIXER_ERROR_GET_VOLUME;
	    }
	}
	if (ioctl(fd, MIXER_READ(cmd), &volumen) == -1)
	{
	    close(fd);
	    return PLUGIN_INOUT_AUDIO_MIXER_ERROR_GET_VOLUME;
	}
	*r_vol = (volumen & 0xFF00) >> 8;
	*l_vol = (volumen & 0x00FF);
	close(fd);
    }
    return PLUGIN_INOUT_OK;
}



static gint oss_set_out_volume (gint l_vol, gint r_vol)
{
    gint fd;
    gint volumen;
    gint cmd;
    gint mask;


    if (oss_cfg->mixer_out)
    {
	if ((fd = open(oss_cfg->mixer_device_out, O_WRONLY)) == -1) return PLUGIN_INOUT_AUDIO_MIXER_ERROR_OPEN;

	if (ioctl(fd, SOUND_MIXER_READ_DEVMASK, &mask) == -1)
	{
	    close(fd);
	    return PLUGIN_INOUT_AUDIO_MIXER_ERROR_GET_CAPS;
	}
	if ((mask & SOUND_MASK_PCM) && (oss_cfg->volume_control_pcm == 0))
	{
	    cmd = SOUND_MIXER_READ_PCM;
	}
	else
	{
	    if ((mask & SOUND_MASK_VOLUME) && (oss_cfg->volume_control_pcm == 1))
	    {
		cmd = SOUND_MIXER_READ_VOLUME;
	    }
	    else
	    {
		close(fd);
		return PLUGIN_INOUT_AUDIO_MIXER_ERROR_SET_VOLUME;
	    }
	}
	volumen = (r_vol << 8) | l_vol;
	if (ioctl(fd, MIXER_WRITE(cmd), &volumen) == -1)
	{
	    close(fd);
	    return PLUGIN_INOUT_AUDIO_MIXER_ERROR_SET_VOLUME;
	}
	close(fd);
    }
    return PLUGIN_INOUT_OK;
}



static gint oss_get_in_volume (gint *l_vol, gint *r_vol)
{
    gint fd;
    gint volumen;
    gint gaim;
    gint caps = 0;
    gint r1;
    gint r2;


    if (oss_cfg->mixer_in)
    {
	if ((fd = open(oss_cfg->mixer_device_in, O_RDONLY)) == -1) return PLUGIN_INOUT_AUDIO_MIXER_ERROR_OPEN;
	switch (oss_cfg->audio_source_in)
	{
	case 0:
	    caps = SOUND_MIXER_MIC;
	    break;
	case 1:
	    caps = SOUND_MIXER_LINE;
	    break;
	case 2:
	    caps = SOUND_MIXER_CD;
	    break;
	}
	//r0 = ioctl(fd, MIXER_READ(SOUND_MIXER_RECLEV), &volumen);
	r1 = ioctl(fd, MIXER_READ(SOUND_MIXER_IGAIN), &gaim);
	r2 = ioctl(fd, MIXER_READ(caps), &volumen);

	if ((r1 == -1) || (r2 == -1))
	{
	    close(fd);
	    return PLUGIN_INOUT_AUDIO_MIXER_ERROR_GET_VOLUME;
	}
	close(fd);
	// *r_vol = (volumen & 0xFF00) >> 8;
	// *l_vol = (volumen & 0x00FF);
	*r_vol = (gaim & 0xFF00) >> 8;
	*l_vol = (gaim & 0x00FF);
    }
    return PLUGIN_INOUT_OK;
}



static gint oss_set_in_volume (gint l_vol, gint r_vol)
{
    gint fd;
    gint volumen;
    gint gaim;
    gint caps = 0;
    gint r1;
    gint r2;


    if (oss_cfg->mixer_in)
    {
	if ((fd = open(oss_cfg->mixer_device_in, O_WRONLY)) == -1) return PLUGIN_INOUT_AUDIO_MIXER_ERROR_OPEN;
	volumen = (r_vol << 8) | l_vol;
        gaim = volumen;
	switch (oss_cfg->audio_source_in)
	{
	case 0:
	    caps = SOUND_MIXER_MIC;
	    break;
	case 1:
	    caps = SOUND_MIXER_LINE;
	    break;
	case 2:
	    caps = SOUND_MIXER_CD;
	    break;
	}
	//r0 = ioctl(fd, MIXER_WRITE(SOUND_MIXER_RECLEV), &volumen);
        r1 = ioctl(fd, MIXER_WRITE(SOUND_MIXER_IGAIN), &gaim);
	r2 = ioctl(fd, MIXER_WRITE(caps), &volumen);

	if ((r1 == -1) || (r2 == -1))
	{
	    close(fd);
	    return PLUGIN_INOUT_AUDIO_MIXER_ERROR_SET_VOLUME;
	}
	close(fd);
    }
    return PLUGIN_INOUT_OK;
}


