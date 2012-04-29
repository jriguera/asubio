
/*
 *
 * FILE: echo.c
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

#include "echo.h"




static Plugin_Effect *plugin_effect = NULL;



static gpointer echo_init (ConfigFile *cfgfile, AudioProperties *aprts, GError **error);
static gint echo_cleanup (gpointer status);
static gint echo_function (gpointer status, gpointer d, gint *length, TPCall type);
static gint echo_configure (gpointer status);
static gint echo_about ();




G_MODULE_EXPORT const gchar *g_module_check_init (GModule *module)
{
    UNUSED(module);


    plugin_effect = (Plugin_Effect *) g_malloc(sizeof(Plugin_Effect));

    plugin_effect->handle = NULL;
    plugin_effect->name = g_strdup(ECHO_NAME);
    plugin_effect->filename = g_strdup(ECHO_FILENAME);
    plugin_effect->description = g_strdup_printf("Audio Echo Dynamic Load Library (c) %s %s %s", ECHO_AUTOR_DATE, ECHO_AUTOR_NAME, ECHO_AUTOR_MAIL);
    plugin_effect->license = g_strdup(ECHO_SHORT_LICENSE);

    plugin_effect->is_usable = g_malloc(sizeof(gint));
    *plugin_effect->is_usable = 0;
    plugin_effect->is_selected = g_malloc(sizeof(gint));
    *plugin_effect->is_selected = 1;

    plugin_effect->init = echo_init;
    plugin_effect->cleanup = echo_cleanup;
    plugin_effect->configure = echo_configure;
    plugin_effect->about = echo_about;
    plugin_effect->pefunction = echo_function;

    return NULL;
}



G_MODULE_EXPORT void g_module_unload (GModule *module)
{
    UNUSED(module);


    g_free(plugin_effect->name);
    g_free(plugin_effect->filename);
    g_free(plugin_effect->description);
    g_free(plugin_effect->license);
    g_free(plugin_effect->is_usable);
    g_free(plugin_effect->is_selected);
    g_free(plugin_effect);
    return;
}



G_MODULE_EXPORT Plugin_Effect *get_Plugin_Effect_struct (void)
{
    return plugin_effect;
}




static gpointer echo_init (ConfigFile *cfgfile, AudioProperties *aprts, GError **error)
{
    Echo_Conf *echo_cfg;
    gint when;


    if ((aprts == NULL) ||  (aprts->format != FMT_S16_LE))
    {
	g_set_error(error, PLUGIN_EFFECT_AUDIO_ERROR, PLUGIN_EFFECT_AUDIO_ERROR_APROPERTIES, "audio con formato no soportado");
	return NULL;
    }
    echo_cfg = (Echo_Conf *) g_malloc(sizeof(Echo_Conf));
    echo_cfg->ap = (AudioProperties *) g_malloc(sizeof(AudioProperties));
    memcpy(echo_cfg->ap, aprts, sizeof(AudioProperties));
    if (cfgfile != NULL)
    {
	echo_cfg->configuration = cfgfile;
	if (!cfgf_read_int(cfgfile, ECHO_CFG_SECTION, ECHO_CFG_DELAY, &echo_cfg->echo_delay))
	{
            echo_cfg->echo_delay = ECHO_DEFAULT_DELAY;
	}
	if (!cfgf_read_int(cfgfile, ECHO_CFG_SECTION, ECHO_CFG_FEEDBACK, &echo_cfg->echo_feedback))
	{
	    echo_cfg->echo_feedback = ECHO_DEFAULT_FEEDBACK;
	}
	if (!cfgf_read_int(cfgfile, ECHO_CFG_SECTION, ECHO_CFG_VOLUME, &echo_cfg->echo_volume))
	{
            echo_cfg->echo_volume = ECHO_DEFAULT_VOLUME;
	}
	if (!cfgf_read_int(cfgfile, ECHO_CFG_SECTION, ECHO_CFG_WHEN, &when))
	{
	    echo_cfg->when = AUDIO_CALL_NONE;
	}
        else echo_cfg->when = when;
    }
    else
    {
	g_set_error(error, PLUGIN_EFFECT_AUDIO_ERROR, PLUGIN_EFFECT_AUDIO_ERROR_CFGFILE_READ, "no se encuentra configuracion");
	g_free(echo_cfg->ap);
	g_free(echo_cfg);
	return NULL;
    }
    echo_cfg->buffer_in = g_malloc0(BUFFER_BYTES + 2);
    echo_cfg->buffer_out = g_malloc0(BUFFER_BYTES + 2);
    echo_cfg->echo_surround_enable = FALSE;
    echo_cfg->w_ofs_out = 0;
    echo_cfg->w_ofs_in = 0;

    return echo_cfg;
}



static gint echo_cleanup (gpointer status)
{
    Echo_Conf *echo_conf = (Echo_Conf *) status;


    if (status != NULL) return 1;

    g_free(echo_conf->ap);
    g_free(echo_conf->buffer_in);
    g_free(echo_conf->buffer_out);
    g_free(echo_conf);
    return 1;
}



static gint echo_function (gpointer status, gpointer d, gint *length, TPCall type)
{
    gint16 *data = (gint16 *) d;
    Echo_Conf *echo_cfg = (Echo_Conf *) status;
    gint r_ofs;
    gint fb_div;
    gint i;
    gint in;
    gint out;
    gint buf;


    if (echo_cfg->echo_surround_enable && echo_cfg->ap->channels == 2) fb_div = 200;
    else fb_div = 100;

    if ((type & AUDIO_CALL_IN) && (echo_cfg->when & AUDIO_CALL_IN))
    {
	r_ofs = echo_cfg->w_ofs_in - (echo_cfg->ap->rate * echo_cfg->echo_delay / 1000) * echo_cfg->ap->channels;

	if (r_ofs < 0) r_ofs += BUFFER_SHORTS;

	for (i = 0; i < ((*length)/2); i++)
	{
	    in = data[i];
	    buf = echo_cfg->buffer_in[r_ofs];

	    if (echo_cfg->echo_surround_enable && echo_cfg->ap->channels == 2)
	    {
		if (i & 1) buf -= echo_cfg->buffer_in[r_ofs - 1];
		else buf -= echo_cfg->buffer_in[r_ofs + 1];
	    }
	    out = in + buf * echo_cfg->echo_volume / 100;
	    buf = in + buf * echo_cfg->echo_feedback / fb_div;
	    out = CLAMP(out, -32768, 32767);
	    buf = CLAMP(buf, -32768, 32767);
	    echo_cfg->buffer_in[echo_cfg->w_ofs_in] = buf;
	    data[i] = out;
	    if (++r_ofs >= BUFFER_SHORTS) r_ofs -= BUFFER_SHORTS;
	    echo_cfg->w_ofs_in++;
	    if (echo_cfg->w_ofs_in >= BUFFER_SHORTS) echo_cfg->w_ofs_in -= BUFFER_SHORTS;
	}
    }

    if ((type & AUDIO_CALL_OUT) && (echo_cfg->when & AUDIO_CALL_OUT))
    {
	r_ofs = echo_cfg->w_ofs_out - (echo_cfg->ap->rate * echo_cfg->echo_delay / 1000) * echo_cfg->ap->channels;

	if (r_ofs < 0) r_ofs += BUFFER_SHORTS;

	for (i = 0; i < ((*length)/2); i++)
	{
	    in = data[i];
	    buf = echo_cfg->buffer_out[r_ofs];

	    if (echo_cfg->echo_surround_enable && echo_cfg->ap->channels == 2)
	    {
		if (i & 1) buf -= echo_cfg->buffer_out[r_ofs - 1];
		else buf -= echo_cfg->buffer_out[r_ofs + 1];
	    }
	    out = in + buf * echo_cfg->echo_volume / 100;
	    buf = in + buf * echo_cfg->echo_feedback / fb_div;
	    out = CLAMP(out, -32768, 32767);
	    buf = CLAMP(buf, -32768, 32767);
	    echo_cfg->buffer_out[echo_cfg->w_ofs_out] = buf;
	    data[i] = out;
	    if (++r_ofs >= BUFFER_SHORTS) r_ofs -= BUFFER_SHORTS;
	    echo_cfg->w_ofs_out++;
	    if (echo_cfg->w_ofs_out >= BUFFER_SHORTS) echo_cfg->w_ofs_out -= BUFFER_SHORTS;
	}
    }
    return 1;
}



static gint echo_configure (gpointer status)
{
    Echo_Conf *echo_conf = (Echo_Conf *) status;
    static GtkWidget *conf_dialog = NULL;
    gint result;
    GtkWidget *table;
    GtkWidget *label;
    GtkWidget *hscale;
    GtkObject *echo_delay_adj;
    GtkObject *echo_feedback_adj;
    GtkObject *echo_volume_adj;
    GtkWidget *frame;
    GtkWidget *vbox;
    GtkWidget *check_button_in;
    GtkWidget *check_button_out;
    GtkTooltips *tips_help;
    gboolean in;
    gboolean out;


    if (conf_dialog != NULL)
    {
	gtk_dialog_response(GTK_DIALOG(conf_dialog), GTK_RESPONSE_NONE);
        return GTK_RESPONSE_NONE;
    }
    if (!gtk_init_check (NULL, NULL)) return PLUGIN_EFFECT_ERROR;

    conf_dialog = gtk_dialog_new_with_buttons("Configuracion del Plugin de efecto de echo", NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_STOCK_APPLY, GTK_RESPONSE_APPLY,
					 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
    gtk_window_set_resizable(GTK_WINDOW(conf_dialog), FALSE);
    tips_help = gtk_tooltips_new ();

    echo_delay_adj = gtk_adjustment_new(echo_conf->echo_delay, 0, MAX_DELAY + 100, 10, 100, 100);
    echo_feedback_adj = gtk_adjustment_new(echo_conf->echo_feedback, 0, 100 + 10, 2, 10, 10);
    echo_volume_adj = gtk_adjustment_new(echo_conf->echo_volume, 0, 100 + 10, 2, 10, 10);

    table = gtk_table_new(2, 4, FALSE);
    gtk_table_set_col_spacings(GTK_TABLE(table), 5);
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(conf_dialog)->vbox), table, TRUE, TRUE, 5);

    label = gtk_label_new("Delay (ms) :");
    gtk_misc_set_alignment(GTK_MISC(label), 1, 0);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL | GTK_EXPAND, 0, 15);

    label = gtk_label_new("Feedback (%) :");
    gtk_misc_set_alignment(GTK_MISC(label), 1, 0);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL | GTK_EXPAND, 0, 0);

    label = gtk_label_new("Volume (%) :");
    gtk_misc_set_alignment(GTK_MISC(label), 1, 0);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3, GTK_FILL, GTK_FILL | GTK_EXPAND, 0, 10);

    hscale = gtk_hscale_new(GTK_ADJUSTMENT(echo_delay_adj));
    gtk_widget_set_usize(hscale, 400, 35);
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), hscale, "Tiempo, en milisegundos, de retardo del eco.", NULL);
    gtk_scale_set_digits(GTK_SCALE(hscale), 0);
    gtk_scale_set_value_pos(GTK_SCALE(hscale), GTK_POS_BOTTOM);
    gtk_table_attach(GTK_TABLE(table), hscale, 1, 2, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 15);

    hscale = gtk_hscale_new(GTK_ADJUSTMENT(echo_feedback_adj));
    gtk_widget_set_usize(hscale, 400, 35);
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), hscale, "Cantidad, de audio que se destina a la retroalimentacion.", NULL);
    gtk_scale_set_digits(GTK_SCALE(hscale), 0);
    gtk_scale_set_value_pos(GTK_SCALE(hscale), GTK_POS_BOTTOM);
    gtk_table_attach(GTK_TABLE(table), hscale, 1, 2, 1, 2, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);

    hscale = gtk_hscale_new(GTK_ADJUSTMENT(echo_volume_adj));
    gtk_widget_set_usize(hscale, 400, 35);
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), hscale, "Volumen del eco provocado", NULL);
    gtk_scale_set_digits(GTK_SCALE(hscale), 0);
    gtk_scale_set_value_pos(GTK_SCALE(hscale), GTK_POS_BOTTOM);
    gtk_table_attach(GTK_TABLE(table), hscale, 1, 2, 2, 3, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 10);

    frame = gtk_frame_new("Plugin Activity");
    gtk_container_set_border_width(GTK_CONTAINER(frame), 5);
    gtk_table_attach(GTK_TABLE(table), frame, 0, 2, 3, 4, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);

    vbox = gtk_vbox_new(TRUE, 0);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);

    check_button_out = gtk_check_button_new_with_mnemonic("Audio _out, tratar el audio de salida (microfono)");
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), check_button_out, "Esta opcion activa el plugin para que trate el audio de salida del PC, normalmente del microfono.", NULL);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button_out), echo_conf->when & AUDIO_CALL_OUT ? TRUE : FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), check_button_out, TRUE, TRUE, 2);

    check_button_in = gtk_check_button_new_with_mnemonic("Audio _in, tratar el audio de entrada (altavoces)");
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), check_button_in, "Con esta opcion se activa el plugin para que trate el audio recibido, es decir, el sonido que fluctua por los altavoces.", NULL);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button_in), echo_conf->when & AUDIO_CALL_IN ? TRUE : FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), check_button_in, TRUE, TRUE, 2);


    gtk_dialog_set_default_response(GTK_DIALOG(conf_dialog), GTK_RESPONSE_APPLY);
    gtk_widget_show_all(conf_dialog);

    result = gtk_dialog_run(GTK_DIALOG(conf_dialog));

    if (result == GTK_RESPONSE_APPLY)
    {
	echo_conf->echo_delay = GTK_ADJUSTMENT(echo_delay_adj)->value;
	echo_conf->echo_feedback = GTK_ADJUSTMENT(echo_feedback_adj)->value;
	echo_conf->echo_volume = GTK_ADJUSTMENT(echo_volume_adj)->value;

	cfgf_write_int(echo_conf->configuration, ECHO_CFG_SECTION, ECHO_CFG_DELAY, GTK_ADJUSTMENT(echo_delay_adj)->value);
	cfgf_write_int(echo_conf->configuration, ECHO_CFG_SECTION, ECHO_CFG_FEEDBACK, GTK_ADJUSTMENT(echo_feedback_adj)->value);
	cfgf_write_int(echo_conf->configuration, ECHO_CFG_SECTION, ECHO_CFG_VOLUME, GTK_ADJUSTMENT(echo_volume_adj)->value);
	in = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_button_in));
	out = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_button_out));
	if (in)
	{
	    if (out)
	    {
		cfgf_write_int(echo_conf->configuration, ECHO_CFG_SECTION, ECHO_CFG_WHEN, AUDIO_CALL_IN | AUDIO_CALL_OUT);
		echo_conf->when = AUDIO_CALL_IN | AUDIO_CALL_OUT;
	    }
	    else
	    {
		cfgf_write_int(echo_conf->configuration, ECHO_CFG_SECTION, ECHO_CFG_WHEN, AUDIO_CALL_IN);
		echo_conf->when = AUDIO_CALL_IN;
	    }
	}
	else
	{
	    if (out)
	    {
		cfgf_write_int(echo_conf->configuration, ECHO_CFG_SECTION, ECHO_CFG_WHEN, AUDIO_CALL_OUT);
		echo_conf->when = AUDIO_CALL_OUT;
	    }
	    else
	    {
		cfgf_write_int(echo_conf->configuration, ECHO_CFG_SECTION, ECHO_CFG_WHEN, AUDIO_CALL_NONE);
		echo_conf->when = AUDIO_CALL_NONE;
	    }
	}
    }
    gtk_widget_destroy(conf_dialog);
    gtk_widget_destroyed(conf_dialog, &conf_dialog);

    while (gtk_events_pending()) gtk_main_iteration();

    return result;
}



static gint echo_about ()
{
    static GtkWidget *dialog;
    gint result;


    if (dialog != NULL)
    {
	gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_NONE);
        return GTK_RESPONSE_NONE;
    }
    if (!gtk_init_check (NULL, NULL)) return PLUGIN_EFFECT_ERROR;

    dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
				    "\nAbout Audio Echo Effect Plugin:\n\n\n(c) %s %s %s\nEcho Dynamic Load Library\n\n\n%s\n",
				    ECHO_AUTOR_DATE, ECHO_AUTOR_NAME, ECHO_AUTOR_MAIL, ECHO_LONG_LICENSE);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    gtk_widget_destroyed(dialog, &dialog);

    while (gtk_events_pending()) gtk_main_iteration();

    return result;
}



