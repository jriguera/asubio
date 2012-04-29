
/*
 *
 * FILE: gui_configuration.c
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

#include "gui_main.h"



static GtkWidget *entry_rtp_ports;
static GtkWidget *entry_rtp_username;
static GtkWidget *entry_rtp_mailname;
static GtkWidget *entry_rtp_description;
static GtkWidget *entry_manage_host;
static GtkWidget *entry_manage_host_port;
static GtkWidget *entry_notify_host;
static GtkWidget *entry_notify_host_port;
static GtkWidget *entry_ip_localhost;
static GtkWidget *entry_port_localhost;
static GtkWidget *check_button_proxy_ssip;
static GtkWidget *entry_command;
static GtkWidget *check_button_command_ig;
static GtkWidget *label_sel_codec;
static GtkWidget *label_sel_in_out;





void set_ssip_options (GtkButton *button, gpointer data)
{
    gchar *vchar;
    gint vint;
    gint ssip_local_port;
    gchar *localhost_ip;
    gchar *notify_host;
    gint notify_host_port;
    GtkWidget *dialog;


    UNUSED(button);
    UNUSED(data);
    localhost_ip = gtk_editable_get_chars(GTK_EDITABLE(entry_ip_localhost), 0, -1);
    if (strlen(localhost_ip) < 7)
    {
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "IP no valida en el campo 'IP localhost'");
	g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_widget_show_all(dialog);
	g_free(localhost_ip);
	return;
    }

    vchar = gtk_editable_get_chars(GTK_EDITABLE(entry_port_localhost), 0, -1);
    ssip_local_port = atoi(vchar);
    g_free(vchar);
    if ((ssip_local_port <= 0) || (ssip_local_port > 65535))
    {
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Numero de puerto no valido en el campo 'Puerto SSIP'");
	g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_widget_show_all(dialog);
	g_free(localhost_ip);
        return;
    }

    if (GTK_TOGGLE_BUTTON(check_button_proxy_ssip)->active)
    {
	vchar = gtk_editable_get_chars(GTK_EDITABLE(entry_manage_host), 0, -1);
	if (strlen(vchar) <= 0)
	{
	    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "IP no valida en el campo 'IP host notificacion SSIP'");
	    g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	    gtk_widget_show_all(dialog);
	    g_free(localhost_ip);
	    g_free(vchar);
	    return;
	}
        g_free(vchar);
	vchar = gtk_editable_get_chars(GTK_EDITABLE(entry_manage_host_port), 0, -1);
	vint = atoi(vchar);
        g_free(vchar);
	if ((vint <= 0) || (vint > 65535))
	{
	    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Numero de puerto no valido en el campo 'IP host notificacion SSIP'");
	    g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	    gtk_widget_show_all(dialog);
	    g_free(localhost_ip);
	    return;
	}
	notify_host = gtk_editable_get_chars(GTK_EDITABLE(entry_notify_host), 0, -1);
	if (strlen(notify_host) <= 0)
	{
	    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "IP no valida en el campo 'IP host gestion SSIP'");
	    g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	    gtk_widget_show_all(dialog);
	    g_free(localhost_ip);
	    g_free(notify_host);
	    return;
	}
	vchar = gtk_editable_get_chars(GTK_EDITABLE(entry_notify_host_port), 0, -1);
	notify_host_port = atoi(vchar);
	g_free(vchar);
	if ((notify_host_port <= 0) || (notify_host_port > 65535))
	{
	    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Numero de puerto no valido en el campo 'IP host gestion SSIP'");
	    g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	    gtk_widget_show_all(dialog);
	    g_free(localhost_ip);
	    g_free(notify_host);
	    return;
	}
    }

    if (GTK_TOGGLE_BUTTON(check_button_proxy_ssip)->active)
    {
	if (init_ssip_proxy_params(notify_host, notify_host_port, ssip_local_port))
	{
	    exit_ssip();

	    cfgf_write_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_HOSTNAME, localhost_ip);
	    cfgf_write_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_LOCALHOST_PORT, ssip_local_port);
	    cfgf_write_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_PROXY, 1);

	    vchar = gtk_editable_get_chars(GTK_EDITABLE(entry_manage_host), 0, -1);
	    cfgf_write_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_REMOTE_HOST, vchar);
	    g_free(vchar);

	    vchar = gtk_editable_get_chars(GTK_EDITABLE(entry_manage_host_port), 0, -1);
	    vint = atoi(vchar);
	    g_free(vchar);
	    cfgf_write_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_REMOTE_HOST_PORT, vint);

	    cfgf_write_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_LOCAL_HOST, notify_host);
	    cfgf_write_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_LOCAL_HOST_PORT, notify_host_port);
	    g_free(notify_host);
	}
	else
	{
	    g_free(localhost_ip);
	    g_free(notify_host);
            return;
	}
    }
    else
    {
	exit_ssip();
	cfgf_write_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_HOSTNAME, localhost_ip);
	cfgf_write_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_LOCALHOST_PORT, ssip_local_port);
	cfgf_write_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_PROXY, 0);
    }
    g_free(localhost_ip);


    ssip_process_loop_create();


    vchar = gtk_editable_get_chars(GTK_EDITABLE(entry_command), 0, -1);
    if (strlen(vchar) > 0)
    {
	cfgf_write_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_EXEC_ONCALL, vchar);
	g_free(vchar);
	if (GTK_TOGGLE_BUTTON(check_button_command_ig)->active)
	{
	    cfgf_write_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_EXEC_ONCALL_IGNORED, 1);
	}
        else cfgf_write_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_EXEC_ONCALL_IGNORED, 0);
    }
    else
    {
	cfgf_remove_key(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_EXEC_ONCALL);
	g_free(vchar);
    }
    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Parametros establecidos correctamente");
    g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
    gtk_widget_show_all(dialog);
    return;
}



void toggle_ssip (GtkWidget *checkbutton, gpointer data)
{
    UNUSED(checkbutton);
    UNUSED(data);


    if (GTK_TOGGLE_BUTTON(check_button_proxy_ssip)->active)
    {
	gtk_widget_set_sensitive(entry_notify_host, TRUE);
	gtk_widget_set_sensitive(entry_notify_host_port, TRUE);
	gtk_widget_set_sensitive(entry_manage_host, TRUE);
	gtk_widget_set_sensitive(entry_manage_host_port, TRUE);
    }
    else
    {
	gtk_widget_set_sensitive(entry_notify_host, FALSE);
	gtk_widget_set_sensitive(entry_notify_host_port, FALSE);
	gtk_widget_set_sensitive(entry_manage_host, FALSE);
	gtk_widget_set_sensitive(entry_manage_host_port, FALSE);
    }
}



void set_rtp_options (GtkButton *button, gpointer data)
{
    gchar *vchar;
    gint vint;
    GtkWidget *dialog;


    UNUSED(button);
    UNUSED(data);
    vchar = gtk_editable_get_chars(GTK_EDITABLE(entry_rtp_ports), 0, -1);
    vint = atoi(vchar);
    if (vint > 0)
    {
	cfgf_write_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_RTP_BASE_PORT, vint);
	g_free(vchar);
    }
    else
    {
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Numero de puerto base RTP/RTCP no valido");
	g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_widget_show_all(dialog);
	g_free(vchar);
	return;
    }

    vchar = gtk_editable_get_chars(GTK_EDITABLE(entry_rtp_username), 0, -1);
    if (strlen(vchar) > 0)
    {
        cfgf_write_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_NAME_USER, vchar);
    }
    g_free(vchar);

    vchar = gtk_editable_get_chars(GTK_EDITABLE(entry_rtp_mailname), 0, -1);
    if (strlen(vchar) > 0)
    {
        cfgf_write_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_MAIL_USER, vchar);
    }
    g_free(vchar);

    vchar = gtk_editable_get_chars(GTK_EDITABLE(entry_rtp_description), 0, -1);
    if (strlen(vchar) > 0)
    {
        cfgf_write_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_DESCRIPTION_USER, vchar);
    }
    g_free(vchar);
    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Parametros establecidos correctamente");
    g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
    gtk_widget_show_all(dialog);
    return;
}



void configure_about_effect (GtkButton *button, GtkTreeView *treeview)
{
    GtkTreeSelection *sel;
    GtkTreeIter iter;
    GtkTreeModel *model;
    gint tipo;


    tipo = (gint) g_object_get_data(G_OBJECT(button), "tipo");
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

    if (gtk_tree_selection_get_selected(sel, &model, &iter))
    {
	gchar *filename;
	gchar *dir;
        gpointer p;
	GModule *g;
	LFunctionPlugin_Effect fe;
	Plugin_Effect *pe;
        GError *tmp_error = NULL;


	gtk_tree_model_get(model, &iter, EP_COLUMN_FILENAME, &filename, -1);

        dir = g_module_build_path(plugins_effect_dir, filename);
        g_free(filename);

	g = g_module_open(dir, 0);
	if (!g) show_error_module_noload(dir);
	else
	{
	    if (!g_module_symbol(g, PLUGIN_EFFECT_STRUCT, (gpointer *) &fe))
	    {
		show_error_module_incorrect(dir, g_module_error());
	    }
	    else
	    {
		pe = fe();
		p = pe->init(configuration, audio_properties, &tmp_error);
		if (tmp_error != NULL) show_error_module_noinit(tmp_error);
		if (tipo == 6)
		{
		    if (pe->about != NULL) pe->about();
		}
		else
		{
		    if (pe->configure != NULL) pe->configure(p);
		    else show_error_module_noconfigure();
		}
		if (pe->cleanup != NULL) pe->cleanup(p);
	    }
            g_module_close(g);
	}
	g_free(dir);
    }
}



void configure_about_codec (GtkButton *button, GtkTreeView *treeview)
{
    GtkTreeSelection *sel;
    GtkTreeIter iter;
    GtkTreeModel *model;
    gint tipo;


    tipo = (gint) g_object_get_data(G_OBJECT(button), "tipo");
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
    if (gtk_tree_selection_get_selected(sel, &model, &iter))
    {
        gchar *filename;
        gchar *dir;
	gint id;
	GModule *g;
	GtkWidget *dialog;
	LFunctionPlugin_Codec co;
	Plugin_Codec *pc;
	Codec *codec;
        GError *tmp_error = NULL;


	gtk_tree_model_get(model, &iter, CP_COLUMN_ID, &id, CP_COLUMN_FILENAME, &filename, -1);

	if (id == 1)
	{
	    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, "El item seleccionado es un pseudo-modulo, ya que envia el audio en formato 'AFMT_S16 mono LITTLE_ENDIAN' sin comprimir.");
	    g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), dialog);
	    gtk_widget_show_all(dialog);
	    return;
	}

        dir = g_module_build_path(plugins_codecs_dir, filename);
        g_free(filename);

	g = g_module_open(dir, 0);
	if (!g) show_error_module_noload(dir);
	else
	{
	    if (!g_module_symbol(g, PLUGIN_CODEC_STRUCT, (gpointer *) &co))
	    {
		show_error_module_incorrect(dir, g_module_error());
	    }
	    else
	    {
		pc = co();
		if (pc->constructor != NULL)
		{
		    if ((codec = pc->constructor()) == NULL) show_error_module_noload(dir);
		    else
		    {
			codec_init(codec, audio_properties, configuration, CODEC_NONE, &tmp_error);
			if (tmp_error != NULL) show_error_module_noinit(tmp_error);
			if (tipo == 1)
			{
			    codec_about(codec);
			}
			else
			{
			    codec_configure(codec);
			}
			codec_destroy(codec);
		    }
		}
		else
		{
		    g_printerr("Error cargando simbolos del modulo '%s': '%s'.\n", dir, g_module_error());
		    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Error cargando simbolos del modulo '%s': '%s'.\n", dir, g_module_error());
		    g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
		    gtk_widget_show_all(dialog);
		}

	    }
            g_module_close(g);
	}
	g_free(dir);
    }
}



void about_configure_select_inout (GtkButton *button, GtkTreeView *treeview)
{
    GtkTreeSelection *sel;
    GtkTreeIter iter;
    GtkTreeModel *model;
    gint tipo;
    GtkWidget *dialog;


    tipo = (gint) g_object_get_data(G_OBJECT(button), "tipo");
    model = gtk_tree_view_get_model (treeview);
    sel = gtk_tree_view_get_selection(treeview);

    if (gtk_tree_selection_get_selected(sel, &model, &iter))
    {
	gchar *name;
	gchar *nname;
        gchar *ant_name;
        gchar *dir;
        gint r;
	GModule *g;
        gboolean b;
	Plugin_InOut *plio;
	LFunctionPlugin_InOut fio;
        GError *tmp_error = NULL;


	gtk_tree_model_get(model, &iter, IOP_COLUMN_FILENAME, &name, IOP_COLUMN_NAME, &nname, -1);

        dir = g_module_build_path(plugins_inout_dir, name);

	g = g_module_open(dir, 0);
	if (!g) show_error_module_noload(dir);
	else
	{
	    if (!g_module_symbol(g, PLUGIN_INOUT_STRUCT, (gpointer *) &fio))
	    {
		show_error_module_incorrect(dir, g_module_error());
	    }
	    else
	    {
		plio = (Plugin_InOut *) fio();
		if ((io_plugin == NULL) || (strcmp(io_plugin->filename, plio->filename) != 0))
		{
		    r = plio->init(configuration, &tmp_error);
		    switch (tipo)
		    {
		    case 3:
			if (plio->about != NULL) plio->about();
			if (tmp_error != NULL) g_error_free(tmp_error);
			break;
		    case 4:
			if (plio->configure != NULL) plio->configure();
			else show_error_module_noconfigure();
			if (tmp_error != NULL) g_error_free(tmp_error);
			break;
		    case 5:
			if (tmp_error != NULL) show_error_module_noinit(tmp_error);
			else
			{
			    b = cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_DEFAULT_IOP, &ant_name);
			    cfgf_write_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_DEFAULT_IOP, name);
			    if (!set_io_plugin()) cfgf_write_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_DEFAULT_IOP, ant_name);
			    else
			    {
				gtk_label_set_text(GTK_LABEL(label_sel_in_out), nname);
				create_model_inout();
			    }
			    if (b) g_free(ant_name);
			}
			break;
		    }
		    if (plio->cleanup != NULL) plio->cleanup();
		}
		else
		{
		    switch (tipo)
		    {
		    case 3:
			if (io_plugin->about != NULL) io_plugin->about();
			if (tmp_error != NULL) g_error_free(tmp_error);
			break;

		    case 4:
			if (io_plugin->configure != NULL) io_plugin->configure();
			else show_error_module_noconfigure();
			if (tmp_error != NULL) g_error_free(tmp_error);
			break;

		    default:
			dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, "El modulo seleccionado ya esta siendo usado");
			g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
			gtk_widget_show_all(dialog);
                        break;
		    }
		}
	    }
            g_module_close(g);
	}
	g_free(dir);
	g_free(name);
	g_free(nname);
    }
}



void toggled_efplugin (GtkCellRendererToggle *cell, gchar *path_str, GtkTreeView *treeview)
{
    GtkTreeSelection *sel;
    GtkTreeModel *model;
    GtkTreeIter  iter;
    gboolean active;
    gchar *name;


    UNUSED(cell);
    UNUSED(path_str);
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

    if (gtk_tree_selection_get_selected(sel, &model, &iter))
    {
	gtk_tree_model_get(model, &iter, EP_COLUMN_ACTIVE, &active, EP_COLUMN_FILENAME, &name, -1);
	active ^= 1;
	gtk_list_store_set(GTK_LIST_STORE(model), &iter, EP_COLUMN_ACTIVE, active, -1);
	if (active)
	{
	    cfgf_write_int(configuration, PROGRAM_GLOBAL_SECTION, name, 1);
	}
	else
	{
	    cfgf_write_int(configuration, PROGRAM_GLOBAL_SECTION, name, 0);
	}
    }
    return;
}



void cb_button_select_codec (GtkButton *button, GtkTreeView *treeview)
{
    GtkTreeSelection *sel;
    GtkTreeModel *model;
    GtkTreeIter  iter;
    gint payload;
    gchar *name;
    GSList *list_aux;


    UNUSED(button);
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
    if (gtk_tree_selection_get_selected(sel, &model, &iter))
    {
	gtk_tree_model_get(model, &iter, CP_COLUMN_PAYLOAD, &payload, CP_COLUMN_NAME, &name, -1);

	g_mutex_lock(list_codec_mutex);

	list_aux = g_slist_find_custom(list_codec_plugins, &payload, (GCompareFunc) compare_payload_node_plugin_vocodec);
	list_codec_plugins = g_slist_remove_link(list_codec_plugins, list_aux);
	list_codec_plugins = g_slist_concat(list_aux, list_codec_plugins);

        g_mutex_unlock(list_codec_mutex);

	if (payload == IO_PCM_PAYLOAD) default_sel_codec = NULL;
        else default_sel_codec = g_slist_nth_data(list_aux, 0);

	cfgf_write_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_DEFAULT_CODEC, payload);
	gtk_label_set_text(GTK_LABEL(label_sel_codec), name);
        g_free(name);
    }
    return;
}



void select_audio_blocksize (GtkButton *button, GtkWidget *entry)
{
    gint value;
    GtkWidget *dialog;


    UNUSED(button);
    value = (gint) g_ascii_strtod(gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1), NULL);
    if (num_users_active_dialogs == 0)
    {
        audio_blocksize = ABS(value);
    }
    else
    {
        show_error_active_dialogs();
    }
    cfgf_write_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_DEFAULT_AUDIOBLOCKSIZE, value);
    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Parametros establecidos correctamente");
    g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
    gtk_widget_show_all(dialog);
}



void response_configuration_settings (GtkWidget *dialog, gint arg1, gpointer user_data)
{
    GError *tmp_error = NULL;
    GtkWidget *d;


    UNUSED(user_data);
    if (arg1 == GTK_RESPONSE_ACCEPT)
    {
	if (!cfgf_write_file(configuration, configuration_filename, &tmp_error))
	{
	    d = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
				       GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
				       "Problema escribiendo el fichero de configuracion '%s': '%s'",
				       configuration_filename, tmp_error->message);
	    gtk_dialog_run(GTK_DIALOG(d));
	    gtk_widget_destroy(d);
	    g_error_free(tmp_error);
	}
    }
    gtk_widget_destroy(dialog);
}



void show_configuration_settings ()
{
    static GtkWidget *dialog = NULL;
    GtkTooltips *tips_help;
    GtkWidget *notebook;
    GtkWidget *hbox;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkWidget *vbox;
    GtkWidget *label;
    GtkWidget *sw_effect;
    GtkWidget *treeview_effect;
    GtkWidget *button_about_effect;
    GtkWidget *button_configure_effect;
    GtkWidget *sw_codec;
    GtkWidget *treeview_codec;
    GtkWidget *button_about_codec;
    GtkWidget *button_configure_codec;
    GtkWidget *button_select_codec;
    GtkWidget *sw_inout;
    GtkWidget *treeview_inout;
    GtkWidget *button_about_inout;
    GtkWidget *button_configure_inout;
    GtkWidget *button_select_inout;
    GtkWidget *button_select_block;
    GtkWidget *button_select_rtp;
    GtkWidget *combo;
    GtkWidget *alignment;
    GtkWidget *hseparator;
    GtkWidget *table;
    GtkWidget *vbox_aux;
    GtkWidget *frame_ssip;
    GtkWidget *button_select_ssip;
    gchar *a_block;
    gint read_int;
    gchar *read_char;
    GList *combo_items = NULL;


    if (dialog != NULL)
    {
	gtk_widget_destroy(dialog);
	return;
    }
    dialog = gtk_dialog_new_with_buttons("Configuracion general", GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					 GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
					 NULL);
    g_signal_connect(GTK_OBJECT(dialog), "response", G_CALLBACK(response_configuration_settings), NULL);
    g_signal_connect(GTK_OBJECT(dialog), "destroy", G_CALLBACK(gtk_widget_destroyed), &dialog);
    //gtk_window_set_default_size(GTK_WINDOW(dialog), 600, 300);
    tips_help = gtk_tooltips_new();

    notebook = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), notebook, TRUE, TRUE, 0);

    // Effect //

    label = gtk_label_new("Effect");
    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
    
    sw_effect = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw_effect), GTK_SHADOW_ETCHED_IN);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw_effect), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), sw_effect, TRUE, TRUE, 0);

    treeview_effect = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store_effect_plugin));
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(treeview_effect), TRUE);
    gtk_tree_view_set_search_column(GTK_TREE_VIEW(treeview_effect), EP_COLUMN_NAME);

    gtk_container_add(GTK_CONTAINER(sw_effect), treeview_effect);

    add_effect_plugins_columns(treeview_effect, GTK_TREE_MODEL(store_effect_plugin), toggled_efplugin);

    hbox = gtk_hbox_new (TRUE, 30);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 8);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    button_about_effect = button_image_from_stock("_Acerca de" ,GTK_STOCK_PROPERTIES, GTK_ICON_SIZE_BUTTON);
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), button_about_effect, "Informacion del plugin seleccionado", NULL );
    g_object_set_data(G_OBJECT(button_about_effect),"tipo", GINT_TO_POINTER(6));
    g_signal_connect(G_OBJECT(button_about_effect), "clicked", G_CALLBACK(configure_about_effect), treeview_effect);

    button_configure_effect = button_image_from_stock("_Configurar" ,GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_BUTTON);
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), button_configure_effect, "Configuracion del plugin seleccionado", NULL );
    g_object_set_data(G_OBJECT(button_configure_effect),"tipo", GINT_TO_POINTER(7));
    g_signal_connect(G_OBJECT(button_configure_effect), "clicked", G_CALLBACK(configure_about_effect), treeview_effect);

    gtk_box_pack_start(GTK_BOX(hbox), button_about_effect, TRUE, TRUE, 10);
    gtk_box_pack_start(GTK_BOX(hbox), button_configure_effect, TRUE, TRUE, 10);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);


    // Codecs //

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);

    sw_codec = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw_codec), GTK_SHADOW_ETCHED_IN);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw_codec), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), sw_codec, TRUE, TRUE, 0);

    // create tree view //
    if (store_codec_plugin != NULL)
    {
	treeview_codec = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store_codec_plugin));
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(treeview_codec), TRUE);
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(treeview_codec), CP_COLUMN_NAME);

	gtk_container_add(GTK_CONTAINER(sw_codec), treeview_codec);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Numero", renderer, "text", CP_COLUMN_ID, NULL);
	gtk_tree_view_column_set_sort_column_id(column, CP_COLUMN_ID);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview_codec), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Nombre", renderer, "text", CP_COLUMN_NAME, NULL);
	gtk_tree_view_column_set_sort_column_id(column, CP_COLUMN_NAME);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview_codec), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Payload", renderer, "text", CP_COLUMN_PAYLOAD, NULL);
	gtk_tree_view_column_set_sort_column_id(column, CP_COLUMN_PAYLOAD);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview_codec), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Band width", renderer, "text", CP_COLUMN_BW, NULL);
	gtk_tree_view_column_set_sort_column_id(column, CP_COLUMN_BW);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview_codec), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Fichero", renderer, "text", CP_COLUMN_FILENAME, NULL);
	gtk_tree_view_column_set_sort_column_id(column, CP_COLUMN_FILENAME);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview_codec), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Descripcion", renderer, "text", CP_COLUMN_DESCRIPTION, NULL);
	gtk_tree_view_column_set_sort_column_id (column, CP_COLUMN_DESCRIPTION);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview_codec), column);

	hbox = gtk_hbox_new(FALSE, 20);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 15);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	label = gtk_label_new("<span color=\"blue\"><big><b>Codec preferente en las sessiones RTP:</b></big></span>");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	label_sel_codec = gtk_label_new(NULL);
	if (list_codec_plugins != NULL)
	{
	    Node_listvocodecs *node = g_slist_nth_data(list_codec_plugins, 0);
            if (node != NULL) gtk_label_set_text(GTK_LABEL(label_sel_codec), node->name);
	}
	gtk_box_pack_start(GTK_BOX(hbox), label_sel_codec, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(TRUE, 30);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 8);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	button_about_codec = button_image_from_stock("_Acerca de" ,GTK_STOCK_PROPERTIES, GTK_ICON_SIZE_BUTTON);
	gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), button_about_codec, "Informacion del plugin seleccionado", NULL );
	g_object_set_data(G_OBJECT(button_about_codec),"tipo", GINT_TO_POINTER(1));
	g_signal_connect(G_OBJECT(button_about_codec), "clicked", G_CALLBACK(configure_about_codec), treeview_codec);

	button_configure_codec = button_image_from_stock("_Configurar" ,GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_BUTTON);
	gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), button_configure_codec, "Configuracion del plugin seleccionado", NULL );
	g_object_set_data(G_OBJECT(button_configure_codec),"tipo", GINT_TO_POINTER(2));
	g_signal_connect(G_OBJECT(button_configure_codec), "clicked", G_CALLBACK(configure_about_codec), treeview_codec);

	button_select_codec = button_image_from_stock("_Seleccionar" ,GTK_STOCK_JUMP_TO, GTK_ICON_SIZE_BUTTON);
	g_object_set_data(G_OBJECT(button_select_codec),"tipo", GINT_TO_POINTER(5));
	gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), button_select_codec, "Establecer el codec seleccionado como preferido", NULL);
	g_signal_connect(G_OBJECT(button_select_codec), "clicked", G_CALLBACK(cb_button_select_codec), treeview_codec);

	gtk_box_pack_start(GTK_BOX(hbox), button_about_codec, TRUE, TRUE, 10);
	gtk_box_pack_start(GTK_BOX(hbox), button_configure_codec, TRUE, TRUE, 10);
	gtk_box_pack_start(GTK_BOX(hbox), button_select_codec, TRUE, TRUE, 10);
    }
    label = gtk_label_new("Codecs");
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);


    // InOut //

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);

    sw_inout = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw_inout), GTK_SHADOW_ETCHED_IN);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw_inout), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), sw_inout, TRUE, TRUE, 0);

    treeview_inout = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store_inout_plugin));
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(treeview_inout), TRUE);
    gtk_tree_view_set_search_column(GTK_TREE_VIEW(treeview_inout), IOP_COLUMN_NAME);
    gtk_container_add(GTK_CONTAINER(sw_inout), treeview_inout);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Numero", renderer,
						      "text", IOP_COLUMN_ID,
						      "background", IOP_COLUMN_COLOR,
						      NULL);
    gtk_tree_view_column_set_sort_column_id(column, IOP_COLUMN_ID);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview_inout), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Nombre", renderer,
						      "text", IOP_COLUMN_NAME,
						      "background", IOP_COLUMN_COLOR,
						      NULL);
    gtk_tree_view_column_set_sort_column_id(column, IOP_COLUMN_NAME);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview_inout), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Fichero", renderer,
						      "text", IOP_COLUMN_FILENAME,
						      "background", IOP_COLUMN_COLOR,
						      NULL);
    gtk_tree_view_column_set_sort_column_id(column, IOP_COLUMN_FILENAME);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview_inout), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Fichero", renderer,
						      "text", IOP_COLUMN_DESCRIPTION,
						      "background", IOP_COLUMN_COLOR,
						      NULL);
    gtk_tree_view_column_set_sort_column_id(column, IOP_COLUMN_DESCRIPTION);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview_inout), column);

    hbox = gtk_hbox_new(FALSE, 20);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 15);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new("<span color=\"#11A52B\"><big><b>Plugin actual de E/S de audio:</b></big></span>");
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    label_sel_in_out = gtk_label_new(NULL);
    gtk_box_pack_start(GTK_BOX(hbox), label_sel_in_out, FALSE, FALSE, 0);
    if (io_plugin != NULL) gtk_label_set_text(GTK_LABEL(label_sel_in_out), io_plugin->name);

    hbox = gtk_hbox_new(TRUE, 30);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 8);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    button_about_inout = button_image_from_stock("_Acerca de" ,GTK_STOCK_PROPERTIES, GTK_ICON_SIZE_BUTTON);
    g_object_set_data(G_OBJECT(button_about_inout),"tipo", GINT_TO_POINTER(3));
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), button_about_inout, "Informacion del plugin seleccionado", NULL);
    g_signal_connect(G_OBJECT(button_about_inout), "clicked", G_CALLBACK(about_configure_select_inout), treeview_inout);

    button_configure_inout = button_image_from_stock("_Configurar" ,GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_BUTTON);
    g_object_set_data(G_OBJECT(button_configure_inout),"tipo", GINT_TO_POINTER(4));
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), button_configure_inout, "Configuracion del plugin seleccionado", NULL);
    g_signal_connect(G_OBJECT(button_configure_inout), "clicked", G_CALLBACK(about_configure_select_inout), treeview_inout);

    button_select_inout = button_image_from_stock("_Seleccionar" ,GTK_STOCK_JUMP_TO, GTK_ICON_SIZE_BUTTON);
    g_object_set_data(G_OBJECT(button_select_inout),"tipo", GINT_TO_POINTER(5));
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), button_select_inout, "Establecer el plugin 'In-Out Audio' seleccionado", NULL);
    g_signal_connect(G_OBJECT(button_select_inout), "clicked", G_CALLBACK(about_configure_select_inout), treeview_inout);

    gtk_box_pack_start(GTK_BOX(hbox), button_about_inout, TRUE, TRUE, 10);
    gtk_box_pack_start(GTK_BOX(hbox), button_configure_inout, TRUE, TRUE, 10);
    gtk_box_pack_start(GTK_BOX(hbox), button_select_inout, TRUE, TRUE, 10);

    label = gtk_label_new("In-Out Audio");
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);


    // Audio Block //

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 20);

    label = gtk_label_new("Tamano del bloque para escritura/lectura de audio");
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 20);

    alignment = gtk_alignment_new(0.5, 0.5, 0, 1);
    gtk_box_pack_start(GTK_BOX(vbox), alignment, FALSE, TRUE, 0);
    combo = gtk_combo_new();
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), GTK_WIDGET(GTK_COMBO(combo)->entry),
			 "Seleccione el tamano de bloque de audio (en bytes) con el que se leera y/o escribira en el dispositivo manejado por el 'In-Out Plugin'."
			 "Un tamano pequeno exige mas CPU pero se reduce la latencia, mientras que un tamano grande reduce el consumo de CPU, pero puede producir una latencia de varios segundos.", NULL);
    gtk_container_add(GTK_CONTAINER(alignment), combo);

    combo_items = g_list_append(combo_items, (gpointer) "512");
    combo_items = g_list_append(combo_items, (gpointer) "1024");
    combo_items = g_list_append(combo_items, (gpointer) "2048");
    combo_items = g_list_append(combo_items, (gpointer) "4096");
    combo_items = g_list_append(combo_items, (gpointer) "8192");
    gtk_combo_set_popdown_strings(GTK_COMBO(combo), combo_items);
    g_list_free(combo_items);

    gtk_entry_set_max_length(GTK_ENTRY(GTK_COMBO(combo)->entry), 6);
    gtk_entry_set_width_chars(GTK_ENTRY(GTK_COMBO(combo)->entry), 10);
    a_block = g_strdup_printf("%d", audio_blocksize);
    gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(combo)->entry), a_block);

    button_select_block = button_image_from_stock("_Establecer" ,GTK_STOCK_JUMP_TO, GTK_ICON_SIZE_BUTTON);
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), button_select_block, "Establecer el tamano de bloque de audio seleccionado", NULL);
    g_signal_connect(G_OBJECT(button_select_block), "clicked", G_CALLBACK(select_audio_blocksize), GTK_ENTRY(GTK_COMBO(combo)->entry));
    alignment = gtk_alignment_new(0.5, 0.5, 0, 1);
    gtk_box_pack_end(GTK_BOX(vbox), alignment, FALSE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(alignment), button_select_block);

    hseparator = gtk_hseparator_new();
    gtk_box_pack_end(GTK_BOX(vbox), hseparator, FALSE, TRUE, 10);

    label = gtk_label_new("Audio Blocksize");
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);


    // RTP //

    vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 20);

    hbox = gtk_hbox_new(FALSE, 10);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    label = gtk_label_new("Numero de puerto UDP a partir del cual se abriran las conexiones RTP/RTCP:");
    gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);

    entry_rtp_ports = gtk_entry_new();
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), GTK_WIDGET(entry_rtp_ports),
			 "Seleccione el numero de puerto base UDP a partir del cual se abriran las conexiones de las sessiones RTP/RTCP."
			 "Los puertos seran abiertos sucesivamente y de dos en dos, es decir, Si Vd selecciona '2000' como numero de puerto"
			 "base, la primera session que cree abrira los puertos '20000' para RTP y '20001' para RTCP. El puerto base debe ser"
			 "par (cuestiones del protocolo RTP/RTCP ;-) )", NULL);
    gtk_container_add(GTK_CONTAINER(hbox), entry_rtp_ports);
    gtk_entry_set_max_length(GTK_ENTRY(entry_rtp_ports), 5);
    gtk_entry_set_width_chars(GTK_ENTRY(entry_rtp_ports), 7);

    if (cfgf_read_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_RTP_BASE_PORT, &read_int))
    {
        read_char = g_strdup_printf("%d", read_int);
	gtk_entry_set_text(GTK_ENTRY(entry_rtp_ports), read_char);
        g_free(read_char);
    }

    hseparator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(vbox), hseparator, TRUE, TRUE, 0);

    table = gtk_table_new(2, 3, FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 10);
    gtk_table_set_col_spacings(GTK_TABLE(table), 20);
    gtk_table_set_row_spacings(GTK_TABLE(table), 5);

    label = gtk_label_new("Nombre  ");
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

    entry_rtp_username = gtk_entry_new();
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), GTK_WIDGET(entry_rtp_username),
			 "Nombre asociado a la session RTP/RTCP", NULL);
    gtk_entry_set_max_length(GTK_ENTRY(entry_rtp_username), 256);
    gtk_table_attach(GTK_TABLE(table), entry_rtp_username, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

    if (cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_NAME_USER, &read_char))
    {
	gtk_entry_set_text(GTK_ENTRY(entry_rtp_username), read_char);
        g_free(read_char);
    }

    label = gtk_label_new("E-Mail  ");
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

    entry_rtp_mailname = gtk_entry_new();
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), GTK_WIDGET(entry_rtp_mailname),"Mail asociado a la session RTP/RTCP", NULL);
    gtk_entry_set_max_length(GTK_ENTRY(entry_rtp_mailname), 256);
    gtk_table_attach(GTK_TABLE(table), entry_rtp_mailname, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
    if (cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_MAIL_USER, &read_char))
    {
	gtk_entry_set_text(GTK_ENTRY(entry_rtp_mailname ), read_char);
        g_free(read_char);
    }

    label = gtk_label_new("Descripcion");
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3, GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

    entry_rtp_description = gtk_entry_new();
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), GTK_WIDGET(entry_rtp_description),"Su descripcion, ;-)", NULL);
    gtk_entry_set_max_length(GTK_ENTRY(entry_rtp_description), 256);
    gtk_table_attach(GTK_TABLE(table), entry_rtp_description, 1, 2, 2, 3, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
    if (cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_DESCRIPTION_USER, &read_char))
    {
	gtk_entry_set_text(GTK_ENTRY(entry_rtp_description), read_char);
        g_free(read_char);
    }

    button_select_rtp = button_image_from_stock("_Establecer" ,GTK_STOCK_JUMP_TO, GTK_ICON_SIZE_BUTTON);
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), button_select_rtp, "Establecer los parametros seleccionados", NULL);
    g_signal_connect(G_OBJECT(button_select_rtp), "clicked", G_CALLBACK(set_rtp_options), NULL);
    alignment = gtk_alignment_new(0.5, 0.5, 0, 1);
    gtk_box_pack_end(GTK_BOX(vbox), alignment, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(alignment), button_select_rtp);

    hseparator = gtk_hseparator_new();
    gtk_box_pack_end(GTK_BOX(vbox), hseparator, TRUE, TRUE, 0);

    label = gtk_label_new("RTP/RTCP");
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);


    // SSIP //

    vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);

    table = gtk_table_new(6, 5, FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 10);
    gtk_table_set_col_spacings(GTK_TABLE(table), 5);
    gtk_table_set_row_spacings(GTK_TABLE(table), 5);

    label = gtk_label_new("IP Localhost");
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

    entry_ip_localhost = gtk_entry_new();
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), GTK_WIDGET(entry_ip_localhost), "IP de la red a la que esta conectado este host", NULL);
    gtk_entry_set_max_length(GTK_ENTRY(entry_ip_localhost), 256);
    gtk_entry_set_width_chars(GTK_ENTRY(entry_ip_localhost), 12);
    gtk_table_attach(GTK_TABLE(table), entry_ip_localhost, 1, 4, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
    if (cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_HOSTNAME, &read_char))
    {
	gtk_entry_set_text(GTK_ENTRY(entry_ip_localhost), read_char);
        g_free(read_char);
    }

    label = gtk_label_new("Puerto SSIP");
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

    entry_port_localhost = gtk_entry_new();
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), GTK_WIDGET(entry_port_localhost), "Puerto en donde se negociaran las conexiones SSIP", NULL);
    gtk_entry_set_max_length(GTK_ENTRY(entry_port_localhost), 5);
    gtk_entry_set_width_chars(GTK_ENTRY(entry_port_localhost), 5);
    gtk_table_attach(GTK_TABLE(table), entry_port_localhost, 1, 2, 1, 2, GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
    if (cfgf_read_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_LOCALHOST_PORT, &read_int))
    {
        read_char = g_strdup_printf("%d", read_int);
	gtk_entry_set_text(GTK_ENTRY(entry_port_localhost), read_char);
        g_free(read_char);
    }

    check_button_proxy_ssip = gtk_check_button_new_with_mnemonic("Habilitar SSIP con proxy");
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), GTK_WIDGET(check_button_proxy_ssip),
			 "Habilita las negociaciones de inicio de session (SSIP) usando un proxy-server."
			 "Para ello debera rellenar los demas datos referentes al proxy", NULL);
    gtk_table_attach(GTK_TABLE(table), check_button_proxy_ssip, 3, 5, 1, 2, GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
    g_signal_connect(G_OBJECT(check_button_proxy_ssip), "clicked", G_CALLBACK(toggle_ssip), NULL);

    frame_ssip = gtk_frame_new("Proxy SSIP");
    gtk_table_attach(GTK_TABLE(table), frame_ssip, 0, 5, 2, 3, GTK_FILL, GTK_FILL, 0, 0);

    vbox_aux = gtk_vbox_new(TRUE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox_aux), 10);
    gtk_container_add(GTK_CONTAINER(frame_ssip), vbox_aux);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(vbox_aux), hbox);

    entry_notify_host_port = gtk_entry_new();
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), GTK_WIDGET(entry_notify_host_port), "Puerto a donde dirigir la notificacion de estado", NULL);
    gtk_entry_set_max_length(GTK_ENTRY(entry_notify_host_port), 5);
    gtk_entry_set_width_chars(GTK_ENTRY(entry_notify_host_port), 8);
    gtk_box_pack_end(GTK_BOX(hbox), entry_notify_host_port, FALSE, TRUE, 20);
    label = gtk_label_new("Puerto");
    gtk_box_pack_end(GTK_BOX(hbox), label, FALSE, TRUE, 10);
    entry_notify_host = gtk_entry_new();
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), GTK_WIDGET(entry_notify_host),
			 "IP del host que se encarga de gestionar el estado (online - offline) de los usuarios."
			 "Normalmente esta en la misma red que este host", NULL);
    gtk_entry_set_max_length(GTK_ENTRY(entry_notify_host), 256);
    gtk_box_pack_end(GTK_BOX(hbox), entry_notify_host, FALSE, TRUE, 0);
    label = gtk_label_new("IP host notificacion SSIP");
    gtk_box_pack_end(GTK_BOX(hbox), label, FALSE, TRUE, 30);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(vbox_aux), hbox);

    entry_manage_host_port = gtk_entry_new();
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), GTK_WIDGET(entry_manage_host_port), "Puerto en donde se gestiona la negociacion de sesiones", NULL);
    gtk_entry_set_max_length(GTK_ENTRY(entry_manage_host_port), 5);
    gtk_entry_set_width_chars(GTK_ENTRY(entry_manage_host_port), 8);
    gtk_box_pack_end(GTK_BOX(hbox), entry_manage_host_port, FALSE, TRUE, 20);
    label = gtk_label_new("Puerto");
    gtk_box_pack_end(GTK_BOX(hbox), label, FALSE, TRUE, 10);
    entry_manage_host = gtk_entry_new();
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), GTK_WIDGET(entry_manage_host),
			 "IP en donde se van a recibir las peticiones de conexion SSIP (SSIP-proxy)", NULL);
    gtk_entry_set_max_length(GTK_ENTRY(entry_manage_host), 256);
    gtk_box_pack_end(GTK_BOX(hbox), entry_manage_host, FALSE, TRUE, 0);
    label = gtk_label_new("IP host gestion SSIP");
    gtk_box_pack_end(GTK_BOX(hbox), label, FALSE, TRUE, 30);


    label = gtk_label_new("Comando :");
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 3, 4, GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 10);

    entry_command = gtk_entry_new();
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), GTK_WIDGET(entry_command), "Comando a ejecutar cada vez que se reciba una conexion valida", NULL);
    gtk_table_attach(GTK_TABLE(table), entry_command, 1, 4, 3, 4, GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 10);
    gtk_entry_set_max_length(GTK_ENTRY(entry_command), 256);
    gtk_entry_set_width_chars(GTK_ENTRY(entry_command), 12);

    check_button_command_ig = gtk_check_button_new_with_mnemonic("Ejecutar para ignorados");
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), GTK_WIDGET(check_button_command_ig), "El comando sera ejecutado aunque el usuario este ignorado", NULL);
    gtk_table_attach(GTK_TABLE(table), check_button_command_ig, 4, 5, 3, 4, GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 10);

    hseparator = gtk_hseparator_new();
    gtk_table_attach(GTK_TABLE(table), hseparator, 0, 5, 4, 5, GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 10);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 0, 5, 5, 6, GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

    button_select_ssip = button_image_from_stock("_Establecer" ,GTK_STOCK_JUMP_TO, GTK_ICON_SIZE_BUTTON);
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), button_select_ssip, "Establecer los parametros seleccionados", NULL);
    g_signal_connect(G_OBJECT(button_select_ssip), "clicked", G_CALLBACK(set_ssip_options), NULL);
    alignment = gtk_alignment_new(0.5, 0.5, 0, 1);
    gtk_container_add(GTK_CONTAINER(alignment), button_select_ssip);
    gtk_container_add(GTK_CONTAINER(hbox), alignment);


    if (cfgf_read_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_REMOTE_HOST_PORT, &read_int))
    {
	read_char = g_strdup_printf("%d", read_int);
	gtk_entry_set_text(GTK_ENTRY(entry_manage_host_port), read_char);
	g_free(read_char);
    }
    if (cfgf_read_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_LOCAL_HOST_PORT, &read_int))
    {
	read_char = g_strdup_printf("%d", read_int);
	gtk_entry_set_text(GTK_ENTRY(entry_notify_host_port), read_char);
	g_free(read_char);
    }
    if (cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_LOCAL_HOST, &read_char))
    {
	gtk_entry_set_text(GTK_ENTRY(entry_notify_host), read_char);
        g_free(read_char);
    }

    if (cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_REMOTE_HOST, &read_char))
    {
	gtk_entry_set_text(GTK_ENTRY(entry_manage_host), read_char);
        g_free(read_char);
    }

    if (cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_EXEC_ONCALL, &read_char))
    {

	if ((cfgf_read_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_EXEC_ONCALL_IGNORED, &read_int)) && (read_int == 1))
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button_command_ig), TRUE);
	}
        else gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button_command_ig), FALSE);
	gtk_entry_set_text(GTK_ENTRY(entry_command), read_char);
        g_free(read_char);
    }

    if ((!cfgf_read_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_PROXY, &read_int)) || (read_int != 1))
    {
	gtk_widget_set_sensitive(entry_notify_host, FALSE);
	gtk_widget_set_sensitive(entry_notify_host_port, FALSE);
	gtk_widget_set_sensitive(entry_manage_host, FALSE);
	gtk_widget_set_sensitive(entry_manage_host_port, FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button_proxy_ssip), FALSE);
    }
    else gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button_proxy_ssip), TRUE);

    label = gtk_label_new("SSIP");
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);

    gtk_widget_show_all(dialog);
    return;
}


