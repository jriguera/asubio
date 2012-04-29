
/*
 *
 * FILE: gui_main_functions.c
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
#include <string.h>



// Wrapper for errors in GLib

gboolean str_has_suffix (const gchar *cad, const gchar *suff)
{
    gchar *c;


    if ((c = strrchr(cad, '.')) != NULL)
    {
	c++;
	if (g_strrstr(c, suff) != NULL) return TRUE;
        else return FALSE;
    }
    return FALSE;
}



void init_configfile (void)
{
    GError *tmp_error = NULL;
    gboolean value;
    GtkWidget *dialog;


    configuration = cfgf_open_file(configuration_filename, &tmp_error);
    if (tmp_error != NULL)
    {
	//show_error_configfile(tmp_error);
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
					"Problema accediendo al fichero de configuracion: %s. Se va a crear un fichero de configuracion nuevo", tmp_error->message);
	gtk_widget_show_all(dialog);
	gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

	g_print("[GUI_MAIN] Creando fichero de configuracion: '%s'.\n", configuration_filename);
	configuration = cfgf_new();

	cfgf_write_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_EXEC_ONCALL_IGNORED, 0);
	cfgf_write_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_PROXY, 0);

	cfgf_write_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_LOCALHOST_PORT, SSIP_MANAGE_DEFAULT_PORT);
	// :-)
	cfgf_write_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_RTP_BASE_PORT, SSIP_MANAGE_DEFAULT_PORT);

	config_dirs();
    }
    value = TRUE;
    value &= cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_DIR_EFFECT, &plugins_effect_dir);
    value &= cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_DIR_CODECS, &plugins_codecs_dir);
    value &= cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_DIR_INOUT, &plugins_inout_dir);

    if (value)
    {
	set_io_plugin();
	create_models();
    }
    else config_dirs();
}



gboolean unset_io_plugin (void)
{
    if (num_users_active_dialogs != 0) 
    {
	show_error_active_dialogs();
        return FALSE;
    }
    if (module_io_plugin != NULL)
    {
	if (io_plugin->cleanup != NULL) io_plugin->cleanup();
        io_plugin = NULL;
        g_module_close(module_io_plugin);
        module_io_plugin = NULL;
    }
    return TRUE;
}



gboolean set_io_plugin (void)
{
    Plugin_InOut *plio;
    LFunctionPlugin_InOut fio;
    GError *tmp_error = NULL;
    GModule *g;
    gint block;
    gchar *value;
    gchar *dir;


    if (num_users_active_dialogs != 0)
    {
	show_error_active_dialogs();
        return FALSE;
    }

    if (!cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_DEFAULT_IOP, &value)) return FALSE;

    if (!cfgf_read_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_DEFAULT_AUDIOBLOCKSIZE, &block))
    {
	block = AUDIO_DEFAULT_BLOCK;
    }
    audio_blocksize = block;

    unset_io_plugin();

    dir = g_module_build_path(plugins_inout_dir, value);
    g_free(value);

    g = g_module_open(dir, 0);
    if (!g)
    {
	show_error_module_noload(dir);
    }
    else
    {
	if (!g_module_symbol(g, PLUGIN_INOUT_STRUCT, (gpointer *) &fio))
	{
	    show_error_module_incorrect(dir, g_module_error());
	}
	else
	{
	    plio = (Plugin_InOut *) fio();
	    plio->init(configuration, &tmp_error);
	    if (tmp_error != NULL)
	    {
		show_error_module_noinit(tmp_error);
		if (plio->cleanup != NULL) plio->cleanup();
	    }
	    else
	    {
		io_plugin = plio;
                module_io_plugin = g;
	    }
	}
    }
    g_free(dir);
    return TRUE;
}



void cb_audio_processor_loop_error (GtkDialog *dialog, gint arg1, gpointer user_data)
{
    UNUSED(arg1);
    UNUSED(user_data);


    conection_reject = TRUE;
    gtk_widget_destroy(GTK_WIDGET(dialog));
}



void cb_audio_processor_loop_used (GtkDialog *dialog, gint arg1, gpointer user_data)
{
    UNUSED(user_data);


    if (arg1 == GTK_RESPONSE_OK)
    {
        control_audio_processor = FALSE;
    }
    else conection_reject = TRUE;
    gtk_widget_destroy(GTK_WIDGET(dialog));
}



gint init_audio_processor (gboolean with_control)
{
    GError *tmp_error = NULL;
    GtkWidget *dialog;
    AInfo info;
    AudioProperties ap_dsp;
    glong timeout = 0;


    g_mutex_lock(audio_processor_mutex);

    if (num_users_active_dialogs != 0)
    {
	num_users_active_dialogs++;
	g_mutex_unlock(audio_processor_mutex);
        return num_users_active_dialogs;
    }

    if (io_plugin == NULL)
    {
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_CLOSE,
					"No hay ningun InOut Plugin definido");
	g_signal_connect(GTK_OBJECT(dialog), "response", G_CALLBACK(cb_audio_processor_loop_error), GTK_OBJECT(dialog));
	gtk_widget_show_all(dialog);

	g_mutex_unlock(audio_processor_mutex);
	return -1;
    }

    if (io_plugin->open_in(O_RDONLY, AUDIO_FORMAT, 1, AUDIO_HZ, square_2(audio_blocksize), &info, &tmp_error) != PLUGIN_INOUT_OK)
    {
	g_printerr("Error abriendo dispositivo de lectura de audio proporcionado por el InOut Plugin '%s': '%s'",io_plugin->filename, tmp_error->message);
	g_printerr("\n\nEstructura AInfo:\n");
	g_printerr("rate: %d\n", info.rate);
	g_printerr("blocksize: %d\n", info.blocksize);
	g_printerr("channels: %d\n", info.channels);
	g_printerr("mixer: %d\n", info.mixer);
	g_printerr("AFormat: %d\n\n", info.format);
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK_CANCEL,
					"Error abriendo dispositivo de lectura de audio proporcionado por el InOut Plugin '%s': '%s'", io_plugin->filename, tmp_error->message);
	if (with_control)
	{
	    g_signal_connect(GTK_OBJECT(dialog), "response", G_CALLBACK(cb_audio_processor_loop_used), GTK_OBJECT(dialog));
	}
	else g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_widget_show_all(dialog);
	g_clear_error(&tmp_error);
	g_mutex_unlock(audio_processor_mutex);
	return -1;
    }
    if (io_plugin->open_out(O_WRONLY, AUDIO_FORMAT, 1, AUDIO_HZ, square_2(audio_blocksize), &info, &tmp_error) != PLUGIN_INOUT_OK)
    {
	g_printerr("Error abriendo dispositivo de escritura de audio proporcionado por el InOut Plugin '%s': '%s'",io_plugin->filename, tmp_error->message);
	g_printerr("\n\nEstructura AInfo:\n");
	g_printerr("rate: %d\n", info.rate);
	g_printerr("blocksize: %d\n", info.blocksize);
	g_printerr("channels: %d\n", info.channels);
	g_printerr("mixer: %d\n", info.mixer);
	g_printerr("AFormat: %d\n\n", info.format);
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK_CANCEL,
					"Error abriendo dispositivo de escritura de audio proporcionado por el InOut Plugin '%s': '%s'", io_plugin->filename, tmp_error->message);
	if (with_control)
	{
	    g_signal_connect(GTK_OBJECT(dialog), "response", G_CALLBACK(cb_audio_processor_loop_used), GTK_OBJECT(dialog));
	}
	else g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_widget_show_all(dialog);
	g_clear_error(&tmp_error);
	io_plugin->close_in();
	g_mutex_unlock(audio_processor_mutex);
	return -1;
    }

    if (audio_caps != NULL)
    {
	g_free(audio_caps->device_in);
	g_free(audio_caps->mixer_in);
	g_free(audio_caps->device_out);
	g_free(audio_caps->mixer_out);
        g_free(audio_caps);
    }
    audio_caps = io_plugin->get_caps();
    if (!audio_caps->block_read)
    {
        gdouble tmp;

	tmp = (AUDIO_HZ*1*AUDIO_FORMAT_BYTES)/audio_blocksize;
        tmp *= 0.000001;
	if (audio_caps->block_write) tmp /= 2;
        timeout = (glong) tmp;
    }

    ap_dsp.channels = 1;
    ap_dsp.rate = AUDIO_HZ;
    ap_dsp.format = AUDIO_FORMAT;
    tmp_error = NULL;

    audio_processor_loop_create(&ap_dsp, audio_properties, audio_blocksize, audio_blocksize, timeout, audio_cng, io_plugin, &tmp_error);
    if (tmp_error != NULL)
    {
	g_printerr("Error creando hilo (thread) procesador de audio: '%s'", tmp_error->message);
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
					"Error creando hilo (thread) procesador de audio: '%s'", tmp_error->message);
	if (with_control)
	{
	    g_signal_connect(GTK_OBJECT(dialog), "response", G_CALLBACK(cb_audio_processor_loop_error), GTK_OBJECT(dialog));
	}
        else g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_widget_show_all(dialog);
	io_plugin->close_in();
	io_plugin->close_out();
	g_clear_error(&tmp_error);

	g_mutex_unlock(audio_processor_mutex);
        return -1;
    }
    num_users_active_dialogs++;
    g_mutex_unlock(audio_processor_mutex);
    return num_users_active_dialogs;
}



gint destroy_audio_processor (void)
{
    GtkWidget *dialog;
    GError *tmp_error = NULL;


    g_mutex_lock(audio_processor_mutex);

    if (num_users_active_dialogs <= 0)
    {
	g_mutex_unlock(audio_processor_mutex);
	return num_users_active_dialogs;
    }
    num_users_active_dialogs--;
    if (num_users_active_dialogs != 0)
    {
	g_mutex_unlock(audio_processor_mutex);
	return num_users_active_dialogs;
    }

    if (audio_processor_loop_destroy(&tmp_error))
    {
	io_plugin->close_in();
	io_plugin->close_out();

	g_mutex_unlock(audio_processor_mutex);
	return num_users_active_dialogs;
    }
    else
    {
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
					"Error cerrando dispositivos de E/S de audio (no cerrados) %s", (tmp_error != NULL) ? tmp_error->message : " ");
	g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_widget_show_all(dialog);
	if (tmp_error != NULL) g_error_free(tmp_error);

	g_mutex_unlock(audio_processor_mutex);
	return num_users_active_dialogs;
    }
}



void add_effect_plugins_columns (GtkWidget *treeview, GtkTreeModel *model, gpointer func)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;


    UNUSED(model);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Numero", renderer, "text", EP_COLUMN_ID, NULL);
    gtk_tree_view_column_set_sort_column_id(column, EP_COLUMN_ID);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes ("Nombre", renderer, "text", EP_COLUMN_NAME, NULL);
    gtk_tree_view_column_set_sort_column_id(column, EP_COLUMN_NAME);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    renderer = gtk_cell_renderer_toggle_new();
    g_signal_connect(renderer, "toggled", G_CALLBACK(func), treeview);
    column = gtk_tree_view_column_new_with_attributes("Activo", renderer, "active", EP_COLUMN_ACTIVE, NULL);
    gtk_tree_view_column_set_sizing(GTK_TREE_VIEW_COLUMN(column), GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_fixed_width(GTK_TREE_VIEW_COLUMN(column), 50);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes ("Fichero", renderer, "text", EP_COLUMN_FILENAME, NULL);
    gtk_tree_view_column_set_sort_column_id(column, EP_COLUMN_FILENAME);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Descripcion", renderer, "text", EP_COLUMN_DESCRIPTION, NULL);
    gtk_tree_view_column_set_sort_column_id(column, EP_COLUMN_DESCRIPTION);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    return;
}



gint compare_node_plugin_effect (gconstpointer a, gconstpointer b)
{
    PluginEffectNode *na;
    gint *nb;


    na = (PluginEffectNode *) a;
    nb = (gint *) b;

    if ((na != NULL) && (nb != NULL))
    {
	if (na->id == *nb) return 0;
	return 1;
    }
    return -1;
}



gint compare_payload_node_plugin_vocodec (gconstpointer a, gconstpointer b)
{
    NodeVoCodec *na;
    gint *nb;


    na = (NodeVoCodec *) a;
    nb = (gint *) b;

    if ((na != NULL) && (nb != NULL))
    {
	if (na->pc == NULL) return -1;
	if (na->pc->payload == *nb) return 0;
	return 1;
    }
    return -1;
}



gint compare_id_node_plugin_vocodec (gconstpointer a, gconstpointer b)
{
    NodeVoCodec *na;
    gint *nb;


    na = (NodeVoCodec *) a;
    nb = (gint *) b;

    if ((na != NULL) && (nb != NULL))
    {
	if (na->id == *nb) return 0;
	return 1;
    }
    return -1;
}



void clear_list_plugins_effect (GSList *list_plugins_effect)
{
    gint counter = 0;
    PluginEffectNode *pn;


    counter = 0;
    pn = (PluginEffectNode *) g_slist_nth_data(list_plugins_effect, counter);
    while (pn != NULL)
    {
	if (pn->active == TRUE) pn->plugin->cleanup(pn->state);
	g_module_close(pn->module);
	g_free(pn);
	pn = (PluginEffectNode *) g_slist_nth_data(list_plugins_effect, ++counter);
    }
    g_slist_free(list_plugins_effect);
}



void clear_list_plugins_codec (GSList *list_plugins_codec)
{
    gint counter = 0;
    NodeVoCodec *pnc;


    pnc = (NodeVoCodec *) g_slist_nth_data(list_plugins_codec, counter);
    while (pnc != NULL)
    {
	g_module_close(pnc->module);
	g_free(pnc);
	pnc = (NodeVoCodec *) g_slist_nth_data(list_plugins_codec, ++counter);
    }
    g_slist_free(list_plugins_codec);
}



gboolean create_model_inout (void)
{
    GDir *dir_io;
    GError *tmp_error = NULL;
    gchar *path;
    gchar *plu;
    const gchar *name;
    gint counter = 0;
    GModule *g;
    gboolean b;
    Plugin_InOut *plio;
    LFunctionPlugin_InOut fio;
    GtkTreeIter iter;
    GtkWidget *dialog;


    if (plugins_inout_dir == NULL)
    {
	g_printerr("Error accediendo al directorio de plugins E/S audio\n");
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "No se ha definido el directorio de los plugins E/S de audio");
	g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_widget_show_all(dialog);
	return FALSE;
    }
    else dir_io = g_dir_open(plugins_inout_dir, 0, &tmp_error);
    if (tmp_error != NULL)
    {
	show_error_open_dir(plugins_inout_dir, tmp_error);
	g_dir_close(dir_io);
        return FALSE;
    }

    b = cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_DEFAULT_IOP, &plu);

    store_inout_plugin = gtk_list_store_new(IOP_NUM_COLUMNS, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    counter = 1;
    while ((name = g_dir_read_name(dir_io)) != NULL)
    {
	if (str_has_suffix(name, G_MODULE_SUFFIX))
	{
	    path = g_module_build_path(plugins_inout_dir, name);
	    g = g_module_open(path, 0);
	    if (g)
	    {
		if (g_module_symbol(g, PLUGIN_INOUT_STRUCT, (gpointer *) &fio))
		{
		    plio = (Plugin_InOut *) fio();
		    gtk_list_store_append(store_inout_plugin, &iter);
		    gtk_list_store_set(store_inout_plugin, &iter,
				       IOP_COLUMN_ID, counter,
				       IOP_COLUMN_NAME, plio->name,
				       IOP_COLUMN_FILENAME, name,
				       IOP_COLUMN_DESCRIPTION, plio->description,
				       IOP_COLUMN_COLOR, (b && (0 == g_ascii_strcasecmp(plu, name))) ? PROGRAM_IOP_SELECT_COLOR : NULL,
				       -1);
                    counter++;
		}
	    }
            g_module_close(g);
	    g_free(path);
	}
        else continue;
    }
    g_dir_close(dir_io);
    return TRUE;
}



gboolean create_models (void)
{
    GDir *dir_cd;
    GDir *dir_ef;
    GError *tmp_error = NULL;
    GtkWidget *dialog;
    const gchar *name;
    gchar *path;
    gint counter = 0;
    GModule *g;
    gint i;
    gboolean b;
    Plugin_Codec *pc;
    Plugin_Effect *pe;
    LFunctionPlugin_Effect fe;
    LFunctionPlugin_Codec fc;
    GtkTreeIter iter;
    Node_listvocodecs *no;


    // In-Out

    create_model_inout();

    if (plugins_codecs_dir == NULL)
    {
	g_printerr("El directorio de Codecs de compresion de audio no esta definido\n");
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "El directorio de Codecs de compresion de audio no esta definido");
	g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_widget_show_all(dialog);
	return FALSE;
    }
    else dir_cd = g_dir_open(plugins_codecs_dir, 0, &tmp_error);
    if (tmp_error != NULL)
    {
	show_error_open_dir(plugins_codecs_dir, tmp_error);
        return FALSE;
    }
    if (plugins_effect_dir == NULL)
    {
	g_printerr("El directorio de Effect plugins no esta definido\n");
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "El directorio de Effect plugins no esta definido");
	g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_widget_show_all(dialog);
	return FALSE;
    }
    else dir_ef = g_dir_open(plugins_effect_dir, 0, &tmp_error);
    if (tmp_error != NULL)
    {
	show_error_open_dir(plugins_effect_dir, tmp_error);
	g_dir_close(dir_cd);
        return FALSE;
    }

    // codecs

    store_codec_plugin = gtk_list_store_new(CP_NUM_COLUMNS, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING);
    counter = 1;
    gtk_list_store_append(store_codec_plugin, &iter);
    gtk_list_store_set(store_codec_plugin, &iter,
		       CP_COLUMN_ID, counter,
		       CP_COLUMN_NAME, IO_PCM_NAME,
		       CP_COLUMN_PAYLOAD, IO_PCM_PAYLOAD,
		       CP_COLUMN_BW, IO_PCM_BW,
		       CP_COLUMN_FILENAME, "(interno)",
		       CP_COLUMN_DESCRIPTION, IO_PCM_DESCRIPTION,
		       -1);
    counter++;
    while ((name = g_dir_read_name(dir_cd)) != NULL)
    {
	if (str_has_suffix(name, G_MODULE_SUFFIX))
	{
	    path = g_module_build_path(plugins_codecs_dir, name);
	    g = g_module_open(path, 0);
	    if (g)
	    {
		if (g_module_symbol(g, PLUGIN_CODEC_STRUCT, (gpointer *) &fc))
		{
		    pc = fc();
		    gtk_list_store_append(store_codec_plugin, &iter);
		    gtk_list_store_set(store_codec_plugin, &iter,
				       CP_COLUMN_ID, counter,
				       CP_COLUMN_NAME, pc->name,
				       CP_COLUMN_PAYLOAD, pc->payload,
				       CP_COLUMN_BW, pc->bw,
				       CP_COLUMN_FILENAME, name,
				       CP_COLUMN_DESCRIPTION, pc->description,
				       -1);
		    no = (Node_listvocodecs *) g_malloc(sizeof(Node_listvocodecs));
		    no->filename = g_strdup(name);
		    no->payload = pc->payload;
                    no->bw = pc->bw;
                    no->name = g_strdup(pc->name);
                    no->id = counter;
		    list_codec_plugins = g_slist_append(list_codec_plugins, no);
                    counter++;
		}
	    }
            g_module_close(g);
	    g_free(path);
	}
        else continue;
    }
    g_dir_close(dir_cd);

    // Effect

    store_effect_plugin = gtk_list_store_new(EP_NUM_COLUMNS, G_TYPE_INT, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING);
    counter = 1;
    while ((name = g_dir_read_name(dir_ef)) != NULL)
    {
	if (str_has_suffix(name, G_MODULE_SUFFIX))
	{
	    path = g_module_build_path(plugins_effect_dir, name);
	    g = g_module_open(path, 0);
	    if (g)
	    {
		if (g_module_symbol(g, PLUGIN_EFFECT_STRUCT, (gpointer *) &fe))
		{
		    pe = (Plugin_Effect *) fe();

		    b = cfgf_read_int(configuration, PROGRAM_GLOBAL_SECTION, (gchar *) name, &i);
		    if (b && (i == 0))  b = FALSE;

		    gtk_list_store_append(store_effect_plugin, &iter);
		    gtk_list_store_set(store_effect_plugin, &iter,
				       EP_COLUMN_ID, counter,
				       EP_COLUMN_NAME, pe->name,
                                       EP_COLUMN_ACTIVE, b,
				       EP_COLUMN_FILENAME, name,
				       EP_COLUMN_DESCRIPTION, pe->description,
				       -1);
                    counter++;
		}
	    }
            g_module_close(g);
	    g_free(path);
	}
        else continue;
    }
    g_dir_close(dir_ef);

    return TRUE;
}



void destroy_models (void)
{
    gint counter;
    Node_listvocodecs *node;


    counter = 0;
    node = (Node_listvocodecs *) g_slist_nth_data(list_codec_plugins, counter);
    while (node != NULL)
    {
	g_free(node->filename);
        g_free(node);
	node = (Node_listvocodecs *) g_slist_nth_data(list_codec_plugins, ++counter);
    }
    g_slist_free(list_codec_plugins);
    list_codec_plugins = NULL;

    gtk_list_store_clear(store_inout_plugin);
    gtk_list_store_clear(store_codec_plugin);
    gtk_list_store_clear(store_effect_plugin);
    store_inout_plugin = NULL;
    store_codec_plugin = NULL;
    store_effect_plugin = NULL;
}


