
/*
 *
 * FILE: gui_session.c
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




void toggle_button_user_mute (GtkToggleButton *check_button, gpointer data)
{
    TSession *tse = (TSession *) data;


    if (gtk_toggle_button_get_active(check_button))
    {
        *(tse->asession->mute_vol) = TRUE;
    }
    else
    {
	*(tse->asession->mute_vol) = FALSE;
    }
    return;
}



void toggle_button_user_mic_mute (GtkToggleButton *check_button, gpointer data)
{
    TSession *tse = (TSession *) data;


    if (gtk_toggle_button_get_active(check_button))
    {
	*(tse->asession->mute_mic) = TRUE;
    }
    else
    {
        *(tse->asession->mute_mic) = FALSE;
    }
    return;
}



void toggle_button_user_vad (GtkToggleButton *check_button, gpointer data)
{
    TSession *tse = (TSession *) data;


    if (gtk_toggle_button_get_active(check_button))
    {
	if (audio_vad != NULL)
	{
            g_signal_handlers_disconnect_by_func(check_button, G_CALLBACK(toggle_button_user_vad), tse);
	    //gtk_toggle_button_set_active(check_button, FALSE);
	    g_signal_connect(check_button, "toggled", G_CALLBACK(toggle_button_user_vad), tse);

            tse->asession->vad_state = &audio_vad;
	    *(tse->asession->vad) = TRUE;
	}
        else gtk_toggle_button_set_active(check_button, FALSE);
    }
    else
    {
	*(tse->asession->vad) = FALSE;
    }
    return;
}



void change_user_vol (GtkAdjustment *get, gpointer data)
{
    TSession *tse = (TSession *) data;
    gdouble v = gtk_adjustment_get_value(get);


    UNUSED(data);
    //dB -> factor amplicacion
    // db = 20 * log10 (Vd/Vb)

    v = v / 20.0;
    v = pow(10, v);

    *(tse->asession->gaim_rx) = (gfloat) v;
    return;
}



void destroy_user_session (GtkWidget *widget, TSession *tse)
{
    GtkWidget *dialog;
    gint result;


    UNUSED(widget);
    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK_CANCEL, "Seguro que desea terminar esta conversacion ?");
    gtk_widget_show_all(dialog);
    result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    if (result == GTK_RESPONSE_OK) destroy_session(tse->id);

    return;
}



void destroy_InitRTPsession (InitRTPsession *irtpsession)
{
    if (irtpsession->username != NULL) g_free(irtpsession->username);
    if (irtpsession->toolname != NULL) g_free(irtpsession->toolname);
    if (irtpsession->usermail != NULL) g_free(irtpsession->usermail);
    if (irtpsession->userlocate != NULL) g_free(irtpsession->userlocate);
    if (irtpsession->usernote != NULL) g_free(irtpsession->usernote);
    g_free(irtpsession->addr);
    g_free(irtpsession);
}



void destroy_partial_session (TSession *tse)
{
    audio_processor_loop_free_mic(tse->id+1);
    audio_processor_loop_free_spk(tse->id+2);
    destroy_rtp_session(tse->asession->rtps, NULL);
    g_free(tse->userhost);
    gtk_list_store_clear(tse->store_codec);
    clear_list_plugins_codec(tse->plugins_codec);
    gtk_list_store_clear(tse->store_effect);
    clear_list_plugins_effect(tse->asession->plugins_effect);
    cfgf_free(tse->cfg);
    g_free(tse->asession->ap_dsp);
    g_free(tse->asession->vad);
    g_free(tse->asession->mute_mic);
    g_free(tse->asession->mute_vol);
    g_free(tse->asession->gaim_rx);
    g_mutex_free(tse->asession->rtpinfo->mutex);
    g_free(tse->asession->rtpinfo);
    g_free(tse->asession);
    destroy_InitRTPsession(tse->irtpsession);
    g_free(tse);
}



void configure_about_effect_user (GtkButton *button, TSession *tse)
{
    GtkTreeSelection *sel;
    GtkTreeIter iter;
    GtkTreeModel *model;
    GtkTreeView *tree;
    gint tipo;


    tipo = (gint) g_object_get_data(G_OBJECT(button), "tipo");
    tree = (GtkTreeView *) g_object_get_data(G_OBJECT(button), "treeview");
    model = (GtkTreeModel *) tse->store_effect;

    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));

    if (gtk_tree_selection_get_selected(sel, &model, &iter))
    {
        GError *tmp_error = NULL;
	gboolean active;
	gint number;
	GSList *list_aux = NULL;
        PluginEffectNode *pn;


	gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, EP_COLUMN_ID, &number, EP_COLUMN_ACTIVE, &active, -1);

	list_aux = g_slist_find_custom(tse->asession->plugins_effect, &number, (GCompareFunc) compare_node_plugin_effect);
	if (list_aux)
	{
	    pn = (PluginEffectNode *) g_slist_nth_data(list_aux, 0);
	    if (!active)
	    {
		pn->state = pn->plugin->init(tse->cfg, audio_properties, &tmp_error);
		if (tmp_error != NULL)
		{
		    show_error_module_noinit(tmp_error);
		    pn->active = FALSE;
		    pn->state = NULL;
		}
		if (tipo == 6)
		{
		    if (pn->plugin->about != NULL) pn->plugin->about();
		}
		else
		{
		    if (pn->plugin->configure != NULL)  pn->plugin->configure(pn->state);
		    else show_error_module_noconfigure();
		}
		if (pn->plugin->cleanup != NULL) pn->plugin->cleanup(pn->state);
	    }
	    else
	    {
		if (tipo == 6)
		{
		    if (pn->plugin->about != NULL) pn->plugin->about();
		}
		else
		{
		    if (pn->plugin->configure != NULL)  pn->plugin->configure(pn->state);
		    else show_error_module_noconfigure();
		}
	    }
	}
    }
}



void toggled_user_efplugin (GtkCellRendererToggle *cell, gchar *path_str, GtkTreeView *treeview)
{
    GtkTreeModel *model;
    GtkTreeIter  iter;
    TSession *tse;
    GtkTreePath *path;


    UNUSED(cell);
    tse = (TSession *) g_object_get_data(G_OBJECT(treeview), "TSession");
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
    path = gtk_tree_path_new_from_string(path_str);

    if (gtk_tree_model_get_iter(model, &iter, path))
    {
        GError *tmp_error = NULL;
	gboolean active;
	gint number;
	GSList *list_aux = NULL;
        PluginEffectNode *pn;


	gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, EP_COLUMN_ID, &number, EP_COLUMN_ACTIVE, &active, -1);
	gtk_list_store_set(GTK_LIST_STORE(model), &iter, EP_COLUMN_ACTIVE, (active) ? FALSE : TRUE, -1);
	while (gtk_events_pending()) gtk_main_iteration();

	list_aux = g_slist_find_custom(tse->asession->plugins_effect, &number, (GCompareFunc) compare_node_plugin_effect);
	if (list_aux)
	{
	    pn = (PluginEffectNode *) g_slist_nth_data(list_aux, 0);

	    if (pn->active)
	    {
		pn->active = FALSE;
                sleep(1);
		if (pn->plugin->cleanup != NULL) pn->plugin->cleanup(pn->state);
                pn->state = NULL;
	    }
	    else
	    {
		pn->state = pn->plugin->init(tse->cfg, audio_properties, &tmp_error);
		if (tmp_error != NULL)
		{
		    show_error_module_noinit(tmp_error);
		    pn->active = FALSE;
		    pn->state = NULL;
		}
		else pn->active = TRUE;
	    }
	    gtk_list_store_set(GTK_LIST_STORE(model), &iter, EP_COLUMN_ACTIVE, pn->active, -1);
	}
    }
    gtk_tree_path_free(path);
    return;
}



void configure_about_codec_user (GtkButton *button, TSession *tse)
{
    GtkTreeSelection *sel;
    GtkTreeIter iter;
    GtkTreeModel *model;
    gint tipo;
    GtkTreeView *tree;


    tipo = (gint) g_object_get_data(G_OBJECT(button), "tipo");
    tree = (GtkTreeView *) g_object_get_data(G_OBJECT(button), "treeview");
    model = (GtkTreeModel *) tse->store_effect;

    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));

    if (gtk_tree_selection_get_selected(sel, &model, &iter))
    {
	gint id;
	GtkWidget *dialog;
	GSList *list_aux;
        NodeVoCodec *node;


	gtk_tree_model_get(model, &iter, CP_COLUMN_ID, &id, -1);

	if (id == 1)
	{
	    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, "El item seleccionado es un pseudo-modulo, ya que envia el audio en formato 'AFMT_S16 mono LITTLE_ENDIAN' sin comprimir.");
	    g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), dialog);
	    gtk_widget_show_all(dialog);
	    return;
	}
	else
	{
	    list_aux = g_slist_find_custom(tse->plugins_codec, &id, (GCompareFunc) compare_id_node_plugin_vocodec);
	    if (list_aux)
	    {
		node = (NodeVoCodec *) g_slist_nth_data(list_aux, 0);
		if (tipo == 1) codec_about(node->codec);
		else codec_configure(node->codec);
	    }
	}
    }
}



void change_user_codec (GtkButton *button, TSession *tse)
{
    GtkTreeSelection *sel;
    GtkTreeIter iter;
    GtkTreeModel *model;
    gint result;
    GtkTreeView *tree;
    GtkWidget *dialog;
    GtkLabel *label;
    gint mute_mic;


    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING,
				    GTK_BUTTONS_OK_CANCEL,
				    "CUIDADO:\n No todos los programas de RTP soportan el cambio del compresor de audio (codec) de salida.\n"
				    "Si cambia esta opcion, asegurese que su interlocutor tiene un  programa RTP que soporta el cambio dinamico de codec de audio,"
				    "de lo contrario esta interesante conversacion terminara irremediablemente.\n\n"
                                    "Entiende lo que esto significa y quiere continuar ?\n");
    gtk_widget_show_all(dialog);
    result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    if (result != GTK_RESPONSE_OK) return;

    tree = (GtkTreeView *) g_object_get_data(G_OBJECT(button), "treeview");
    label = (GtkLabel *) g_object_get_data(G_OBJECT(button), "label_codec_tx");
    model = (GtkTreeModel *) tse->store_effect;

    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));

    if (gtk_tree_selection_get_selected(sel, &model, &iter))
    {
	gint payload;
        gchar *name;
	GSList *list_aux = NULL;
	NodeVoCodec *nodecodec;
	GError *tmp_error = NULL;
	gint page;
	gint counter = 0;
	TSession *tse_aux;
        GSList *list;


	gtk_tree_model_get(model, &iter, CP_COLUMN_NAME, &name, CP_COLUMN_PAYLOAD, &payload, -1);

	list_aux = g_slist_find_custom(tse->plugins_codec, &payload, (GCompareFunc) compare_payload_node_plugin_vocodec);
	if ((list_aux) || (payload == IO_PCM_PAYLOAD))
	{

	    tse->asession->process = FALSE;
	    mute_mic = *(tse->asession->mute_mic);
	    *(tse->asession->mute_mic) = FALSE;
	    g_thread_join(tse->loop);

	    nodecodec = tse->asession->node_codec_tx;

	    if ((nodecodec != NULL) && (tse->asession->node_codec_rx != NULL))
	    {
		if (tse->asession->node_codec_rx->pc->payload != tse->asession->node_codec_tx->pc->payload)
		{
		    codec_destroy(nodecodec->codec);
		}
		nodecodec->selected_tx = FALSE;
	    }
	    else
	    {
		if (nodecodec != NULL)
		{
		    codec_destroy(nodecodec->codec);
		    nodecodec->selected_tx = FALSE;
		}
	    }
	    if (payload != IO_PCM_PAYLOAD)
	    {
		nodecodec = (NodeVoCodec *) g_slist_nth_data(list_aux, 0);
		nodecodec->selected_tx = TRUE;

		codec_init(nodecodec->codec, tse->asession->ap_dsp, tse->cfg, CODEC_ENCODE, &tmp_error);
		if (tmp_error != NULL)
		{
		    show_error_module_noinit(tmp_error);

		    page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook_users));
		    gtk_notebook_remove_page(GTK_NOTEBOOK(notebook_users), page);
		    gtk_widget_queue_draw(GTK_WIDGET(notebook_users));
		    gtk_window_resize(GTK_WINDOW(window), 1, 1);

		    tse->asession->node_codec_tx = NULL;
		    tse->asession->process = TRUE;
		    counter = 0;
		    tse_aux = (TSession *) g_slist_nth_data(list_session_users, counter);
		    while (tse_aux != NULL)
		    {
			if (tse_aux->id == tse->id) break;
			tse_aux = (TSession *) g_slist_nth_data(list_session_users, ++counter);
		    }
		    if (tse_aux == NULL)
		    {
			g_printerr("[GUI_SESSION] Lost reference to this session, lost memory\n");
			return;
		    }
		    list = g_slist_nth(list_session_users, counter);
		    list_session_users = g_slist_remove_link(list_session_users, list);

		    *(tse->asession->mute_mic) = mute_mic;
		    destroy_partial_session(tse);
		    g_free(name);
		    g_slist_free_1(list);
		    destroy_audio_processor();

                    return;
		}
	    }
	    else nodecodec = NULL;

	    tse->asession->node_codec_tx = nodecodec;
	    tse->asession->process = TRUE;

	    tse->loop = g_thread_create((GThreadFunc) session_io_loop, tse->asession, TRUE, &tmp_error);
	    if (tmp_error != NULL)
	    {
		dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
						"ERROR GRAVE cambiando el codec de compresion de salida.\n\nImposible crear sesion thread de E/S de audio: %s.", tmp_error->message);
		page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook_users));
		gtk_notebook_remove_page(GTK_NOTEBOOK(notebook_users), page);
		gtk_widget_queue_draw(GTK_WIDGET(notebook_users));
		gtk_window_resize(GTK_WINDOW(window), 1, 1);
		g_error_free(tmp_error);

                counter = 0;
		tse_aux = (TSession *) g_slist_nth_data(list_session_users, counter);
		while (tse_aux != NULL)
		{
		    if (tse_aux->id == tse->id) break;
		    tse_aux = (TSession *) g_slist_nth_data(list_session_users, ++counter);
		}
		if (tse_aux == NULL)
		{
		    g_printerr("[GUI_SESSION] Lost reference to this session, lost memory\n");
		    return;
		}
		list = g_slist_nth(list_session_users, counter);
		list_session_users = g_slist_remove_link(list_session_users, list);

		*(tse->asession->mute_mic) = mute_mic;
		destroy_partial_session(tse);
		g_free(name);
		g_slist_free_1(list);
		destroy_audio_processor();

                return;
	    }
	    *(tse->asession->mute_mic) = mute_mic;
	}
	else
	{
	    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
					    GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
					    "Imposible cambiar el codec compresor de audio de salida a %s", name);
	    gtk_widget_show_all(dialog);
	    gtk_dialog_run(GTK_DIALOG(dialog));
	    gtk_widget_destroy(dialog);
	}
	g_free(name);
	if (tse->asession->node_codec_tx != NULL)
	{
	    gtk_label_set_label(GTK_LABEL(label), tse->asession->node_codec_tx->pc->name);
	}
	else gtk_label_set_label(GTK_LABEL(label), IO_PCM_DESCRIPTION);
    }
}



void show_session_info (GtkWidget *widget, TSession *tse)
{
    GtkWidget *view;
    GtkWidget *sw;
    GtkWidget *dialog;
    GtkWidget *frame;
    GtkTextBuffer *buffer;
    GtkTextIter iter;
    gchar *aux;
    gint i;


    UNUSED(widget);
    dialog = gtk_dialog_new_with_buttons("Informacion adicional", GTK_WINDOW(window),
					 GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_STOCK_CLOSE, GTK_RESPONSE_NONE,
					 NULL);
    frame = gtk_frame_new(NULL);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), frame, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(frame), 5);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
    g_signal_connect(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
    g_signal_connect(GTK_OBJECT(dialog), "destroy", G_CALLBACK(gtk_widget_destroyed), &dialog);

    view = gtk_text_view_new();
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(sw), view);
    gtk_container_add(GTK_CONTAINER(frame), sw);

    gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view), GTK_WRAP_WORD);
    gtk_text_buffer_create_tag(buffer, "blue_foreground", "foreground", "blue", NULL);
    gtk_text_buffer_create_tag(buffer, "heading", "weight", PANGO_WEIGHT_BOLD, "size", 15 * PANGO_SCALE, NULL);
    gtk_text_buffer_create_tag(buffer, "bold", "weight", PANGO_WEIGHT_BOLD, "size", 10 * PANGO_SCALE, NULL);


    gtk_text_buffer_get_iter_at_offset(buffer, &iter, 0);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "\nAll info of this session is ...\n\n", -1, "heading", NULL);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(view), 5);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(view), 5);

    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "RTP Conection info:\n", -1, "bold", NULL);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(view), 20);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Remote host :", -1, "blue_foreground", NULL);
    gtk_text_buffer_insert(buffer, &iter, rtp_get_addr(tse->asession->rtps), -1);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);

    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Remote port :", -1, "blue_foreground", NULL);
    aux = g_strdup_printf("%d", rtp_get_tx_port(tse->asession->rtps));
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Local port :", -1, "blue_foreground", NULL);
    aux = g_strdup_printf("%d", rtp_get_rx_port(tse->asession->rtps));
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Crypt session :", -1, "blue_foreground", NULL);
    aux = g_strdup_printf("%s", (tse->asession->crypt) ? "TRUE" : "FALSE");
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n\n", -1);


    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Local SSRC: ", -1, "blue_foreground", NULL);
    g_mutex_lock(tse->asession->rtpinfo->mutex);
    aux = g_strdup_printf("%x", tse->asession->rtpinfo->ssrcs[0]);
    g_mutex_unlock(tse->asession->rtpinfo->mutex);
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);

    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Local SDES CNAME: ", -1, "blue_foreground", NULL);
    g_mutex_lock(tse->asession->rtpinfo->mutex);
    aux = g_strdup_printf("%s", tse->asession->rtpinfo->sdes_info[0][0]);
    g_mutex_unlock(tse->asession->rtpinfo->mutex);
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);

    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Local SDES NAME: ", -1, "blue_foreground", NULL);
    g_mutex_lock(tse->asession->rtpinfo->mutex);
    aux = g_strdup_printf("%s", tse->asession->rtpinfo->sdes_info[0][1]);
    g_mutex_unlock(tse->asession->rtpinfo->mutex);
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);

    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Local SDES E-MAIL: ", -1, "blue_foreground", NULL);
    g_mutex_lock(tse->asession->rtpinfo->mutex);
    aux = g_strdup_printf("%s", tse->asession->rtpinfo->sdes_info[0][2]);
    g_mutex_unlock(tse->asession->rtpinfo->mutex);
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);

    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Local SDES NOTE: ", -1, "blue_foreground", NULL);
    g_mutex_lock(tse->asession->rtpinfo->mutex);
    aux = g_strdup_printf("%s", tse->asession->rtpinfo->sdes_info[0][3]);
    g_mutex_unlock(tse->asession->rtpinfo->mutex);
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);

    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Local SDES TOOL: ", -1, "blue_foreground", NULL);
    g_mutex_lock(tse->asession->rtpinfo->mutex);
    aux = g_strdup_printf("%s", tse->asession->rtpinfo->sdes_info[0][4]);
    g_mutex_unlock(tse->asession->rtpinfo->mutex);
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n\n", -1);

    //Warnig ...
    for (i = 1; ((i < tse->asession->rtpinfo->num_sources) && (i < 10)); i++)
    {
	aux = g_strdup_printf("Remote SOURCE %d\n", i);
	gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, aux, -1, "bold", NULL);
        g_free(aux);

	gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Remote SSRC: ", -1, "blue_foreground", NULL);
	g_mutex_lock(tse->asession->rtpinfo->mutex);
	aux = g_strdup_printf("%x", tse->asession->rtpinfo->ssrcs[i]);
	g_mutex_unlock(tse->asession->rtpinfo->mutex);
	gtk_text_buffer_insert(buffer, &iter, aux, -1);
	g_free(aux);
	gtk_text_buffer_insert(buffer, &iter, "\n", -1);

	gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Remote SDES CNAME: ", -1, "blue_foreground", NULL);
	g_mutex_lock(tse->asession->rtpinfo->mutex);
	aux = g_strdup_printf("%s", tse->asession->rtpinfo->sdes_info[i][0]);
	g_mutex_unlock(tse->asession->rtpinfo->mutex);
	gtk_text_buffer_insert(buffer, &iter, aux, -1);
	g_free(aux);
	gtk_text_buffer_insert(buffer, &iter, "\n", -1);

	gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Remote SDES NAME: ", -1, "blue_foreground", NULL);
	g_mutex_lock(tse->asession->rtpinfo->mutex);
	aux = g_strdup_printf("%s", tse->asession->rtpinfo->sdes_info[i][1]);
	g_mutex_unlock(tse->asession->rtpinfo->mutex);
	gtk_text_buffer_insert(buffer, &iter, aux, -1);
	g_free(aux);
	gtk_text_buffer_insert(buffer, &iter, "\n", -1);

	gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Remote SDES E-MAIL: ", -1, "blue_foreground", NULL);
	g_mutex_lock(tse->asession->rtpinfo->mutex);
	aux = g_strdup_printf("%s", tse->asession->rtpinfo->sdes_info[i][2]);
	g_mutex_unlock(tse->asession->rtpinfo->mutex);
	gtk_text_buffer_insert(buffer, &iter, aux, -1);
	g_free(aux);
	gtk_text_buffer_insert(buffer, &iter, "\n", -1);

	gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Remote SDES NOTE: ", -1, "blue_foreground", NULL);
	g_mutex_lock(tse->asession->rtpinfo->mutex);
	aux = g_strdup_printf("%s", tse->asession->rtpinfo->sdes_info[i][3]);
	g_mutex_unlock(tse->asession->rtpinfo->mutex);
	gtk_text_buffer_insert(buffer, &iter, aux, -1);
	g_free(aux);
	gtk_text_buffer_insert(buffer, &iter, "\n", -1);

	gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Remote SDES TOOL: ", -1, "blue_foreground", NULL);
	g_mutex_lock(tse->asession->rtpinfo->mutex);
	aux = g_strdup_printf("%s", tse->asession->rtpinfo->sdes_info[i][4]);
	g_mutex_unlock(tse->asession->rtpinfo->mutex);
	gtk_text_buffer_insert(buffer, &iter, aux, -1);
	g_free(aux);
	gtk_text_buffer_insert(buffer, &iter, "\n\n", -1);
    }
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);


    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "RX Codec:\n", -1, "bold", NULL);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(view), 20);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Name: ", -1, "blue_foreground", NULL);
    if (tse->asession->node_codec_rx != NULL)
    {
	gtk_text_buffer_insert(buffer, &iter, tse->asession->node_codec_rx->pc->name, -1);
    }
    else gtk_text_buffer_insert(buffer, &iter, IO_PCM_NAME, -1);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Payload: ", -1, "blue_foreground", NULL);
    if (tse->asession->node_codec_rx != NULL)
    {
	aux = g_strdup_printf("%d", tse->asession->node_codec_rx->pc->payload);
	gtk_text_buffer_insert(buffer, &iter, aux, -1);
	g_free(aux);
    }
    else
    {
	aux = g_strdup_printf("%d", IO_PCM_PAYLOAD);
	gtk_text_buffer_insert(buffer, &iter, aux, -1);
	g_free(aux);
    }
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Bytes compresed frame: ", -1, "blue_foreground", NULL);
    if (tse->asession->node_codec_rx != NULL)
    {
	aux = g_strdup_printf("%d", tse->asession->node_codec_rx->pc->compressed_fr_size);
	gtk_text_buffer_insert(buffer, &iter, aux, -1);
	g_free(aux);
    }
    else
    {
	aux = g_strdup_printf("%d", IO_PCM_SIZE);
	gtk_text_buffer_insert(buffer, &iter, aux, -1);
	g_free(aux);
    }
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);

    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Bytes uncompresed frame: ", -1, "blue_foreground", NULL);
    if (tse->asession->node_codec_rx != NULL)
    {
	aux = g_strdup_printf("%d", tse->asession->node_codec_rx->pc->uncompressed_fr_size);
	gtk_text_buffer_insert(buffer, &iter, aux, -1);
	g_free(aux);
    }
    else
    {
	aux = g_strdup_printf("%d", IO_PCM_SIZE);
	gtk_text_buffer_insert(buffer, &iter, aux, -1);
	g_free(aux);
    }
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);

    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Audio ms frame: ", -1, "blue_foreground", NULL);
    if (tse->asession->node_codec_rx != NULL)
    {
	aux = g_strdup_printf("%d", tse->asession->node_codec_rx->pc->audio_ms);
	gtk_text_buffer_insert(buffer, &iter, aux, -1);
	g_free(aux);
    }
    else
    {
	aux = g_strdup_printf("%d", IO_PCM_MS);
	gtk_text_buffer_insert(buffer, &iter, aux, -1);
	g_free(aux);
    }
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Band width: ", -1, "blue_foreground", NULL);
    if (tse->asession->node_codec_rx != NULL)
    {
	aux = g_strdup_printf("%d", tse->asession->node_codec_rx->pc->bw);
	gtk_text_buffer_insert(buffer, &iter, aux, -1);
	g_free(aux);
    }
    else
    {
	aux = g_strdup_printf("%d", IO_PCM_BW);
	gtk_text_buffer_insert(buffer, &iter, aux, -1);
	g_free(aux);
    }
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Description: ", -1, "blue_foreground", NULL);
    if (tse->asession->node_codec_rx != NULL)
    {
	gtk_text_buffer_insert(buffer, &iter, tse->asession->node_codec_rx->pc->description, -1);
    }
    else gtk_text_buffer_insert(buffer, &iter, IO_PCM_DESCRIPTION, -1);
    gtk_text_buffer_insert(buffer, &iter, "\n\n\n", -1);


    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "TX Codec:\n", -1, "bold", NULL);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(view), 20);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Name: ", -1, "blue_foreground", NULL);
    if (tse->asession->node_codec_tx != NULL)
    {
	gtk_text_buffer_insert(buffer, &iter, tse->asession->node_codec_tx->pc->name, -1);
    }
    else gtk_text_buffer_insert(buffer, &iter, IO_PCM_NAME, -1);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Payload: ", -1, "blue_foreground", NULL);
    if (tse->asession->node_codec_tx != NULL)
    {
	aux = g_strdup_printf("%d", tse->asession->node_codec_tx->pc->payload);
	gtk_text_buffer_insert(buffer, &iter, aux, -1);
	g_free(aux);
    }
    else
    {
	aux = g_strdup_printf("%d", IO_PCM_PAYLOAD);
	gtk_text_buffer_insert(buffer, &iter, aux, -1);
	g_free(aux);
    }
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Bytes compresed frame: ", -1, "blue_foreground", NULL);
    if (tse->asession->node_codec_tx != NULL)
    {
	aux = g_strdup_printf("%d", tse->asession->node_codec_tx->pc->compressed_fr_size);
	gtk_text_buffer_insert(buffer, &iter, aux, -1);
	g_free(aux);
    }
    else
    {
	aux = g_strdup_printf("%d", IO_PCM_SIZE);
	gtk_text_buffer_insert(buffer, &iter, aux, -1);
	g_free(aux);
    }
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Bytes uncompresed frame: ", -1, "blue_foreground", NULL);
    if (tse->asession->node_codec_tx != NULL)
    {
	aux = g_strdup_printf("%d", tse->asession->node_codec_tx->pc->uncompressed_fr_size);
	gtk_text_buffer_insert(buffer, &iter, aux, -1);
	g_free(aux);
    }
    else
    {
	aux = g_strdup_printf("%d", IO_PCM_SIZE);
	gtk_text_buffer_insert(buffer, &iter, aux, -1);
	g_free(aux);
    }
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);

    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Audio ms frame: ", -1, "blue_foreground", NULL);
    if (tse->asession->node_codec_tx != NULL)
    {
	aux = g_strdup_printf("%d", tse->asession->node_codec_tx->pc->audio_ms);
	gtk_text_buffer_insert(buffer, &iter, aux, -1);
	g_free(aux);
    }
    else
    {
	aux = g_strdup_printf("%d", IO_PCM_MS);
	gtk_text_buffer_insert(buffer, &iter, aux, -1);
	g_free(aux);
    }
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Band width: ", -1, "blue_foreground", NULL);
    if (tse->asession->node_codec_tx != NULL)
    {
	aux = g_strdup_printf("%d", tse->asession->node_codec_tx->pc->bw);
	gtk_text_buffer_insert(buffer, &iter, aux, -1);
	g_free(aux);
    }
    else
    {
	aux = g_strdup_printf("%d", IO_PCM_BW);
	gtk_text_buffer_insert(buffer, &iter, aux, -1);
	g_free(aux);
    }
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Description: ", -1, "blue_foreground", NULL);
    if (tse->asession->node_codec_tx != NULL)
    {
	gtk_text_buffer_insert(buffer, &iter, tse->asession->node_codec_tx->pc->description, -1);
    }
    else gtk_text_buffer_insert(buffer, &iter, IO_PCM_DESCRIPTION, -1);
    gtk_text_buffer_insert(buffer, &iter, "\n\n", -1);

    gtk_window_resize(GTK_WINDOW(dialog), 400, 300);
    gtk_widget_show_all(dialog);
}



void show_plugins_user (GtkWidget *widget, TSession *tse)
{
    static GtkWidget *dialog;
    GtkTooltips *tips_help;
    GtkWidget *notebook;
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *vbox;
    GtkWidget *sw_effect;
    GtkWidget *button_about_effect;
    GtkWidget *button_configure_effect;
    GtkWidget *sw_codec;
    GtkWidget *button_about_codec;
    GtkWidget *button_init_codec;
    GtkWidget *button_configure_codec;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkWidget *frame_aux;
    GtkWidget *vbox_aux;
    GtkWidget *hbox_aux;
    GtkWidget *label1;
    GtkWidget *label_codec_tx;
    GtkWidget *label_codec_rx;
    GtkWidget *treeview_effect;
    GtkWidget *treeview_codec;


    UNUSED(widget);
    if (dialog != NULL)
    {
	gtk_widget_destroy(dialog);
	return;
    }
    dialog = gtk_dialog_new_with_buttons("Plugins Disponibles", GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 600, 350);
    g_signal_connect(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
    g_signal_connect(GTK_OBJECT(dialog), "destroy", G_CALLBACK(gtk_widget_destroyed), &dialog);
    tips_help = gtk_tooltips_new();

    notebook = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK (notebook), GTK_POS_TOP);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), notebook, TRUE, TRUE, 0);

    // Effect //

    label = gtk_label_new("Effect");
    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
    
    sw_effect = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw_effect), GTK_SHADOW_ETCHED_IN);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw_effect), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), sw_effect, TRUE, TRUE, 0);

    treeview_effect = gtk_tree_view_new_with_model(GTK_TREE_MODEL(tse->store_effect));
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(treeview_effect), TRUE);
    gtk_tree_view_set_search_column(GTK_TREE_VIEW(treeview_effect), EP_COLUMN_NAME);

    gtk_container_add(GTK_CONTAINER(sw_effect), treeview_effect);
    g_object_set_data(G_OBJECT(treeview_effect),"TSession", tse);

    add_effect_plugins_columns(treeview_effect, GTK_TREE_MODEL(tse->store_effect), toggled_user_efplugin);

    hbox = gtk_hbox_new (TRUE, 30);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 8);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    button_about_effect = button_image_from_stock("_Acerca de" ,GTK_STOCK_PROPERTIES, GTK_ICON_SIZE_BUTTON);
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), button_about_effect, "Informacion del plugin seleccionado", NULL );
    g_object_set_data(G_OBJECT(button_about_effect),"tipo", GINT_TO_POINTER(6));
    g_object_set_data(G_OBJECT(button_about_effect),"treeview", treeview_effect);
    g_signal_connect(G_OBJECT(button_about_effect), "clicked", G_CALLBACK(configure_about_effect_user), tse);

    button_configure_effect = button_image_from_stock("_Configurar" ,GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_BUTTON);
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), button_configure_effect, "Configuracion del plugin seleccionado", NULL );
    g_object_set_data(G_OBJECT(button_configure_effect),"tipo", GINT_TO_POINTER(7));
    g_object_set_data(G_OBJECT(button_configure_effect),"treeview", treeview_effect);
    g_signal_connect(G_OBJECT(button_configure_effect), "clicked", G_CALLBACK(configure_about_effect_user), tse);

    gtk_box_pack_start(GTK_BOX(hbox), button_about_effect, TRUE, TRUE, 10);
    gtk_box_pack_start(GTK_BOX(hbox), button_configure_effect, TRUE, TRUE, 10);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);

    // Codecs //

    label = gtk_label_new("Codecs");
    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);

    sw_codec = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw_codec), GTK_SHADOW_ETCHED_IN);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw_codec), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), sw_codec, TRUE, TRUE, 0);

    // create tree view //
    treeview_codec = gtk_tree_view_new_with_model(GTK_TREE_MODEL(tse->store_codec));
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


    frame_aux = gtk_frame_new("Codecs usados actualmente");
    vbox_aux = gtk_vbox_new(TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(vbox_aux), 8);
    gtk_container_add(GTK_CONTAINER(frame_aux), vbox_aux);

    hbox_aux = gtk_hbox_new(FALSE, 10);
    label1 = gtk_label_new("<span color=\"blue\"><big><b>          TX Audio codec : </b></big></span>");
    gtk_label_set_use_markup(GTK_LABEL(label1), TRUE);
    label_codec_tx = gtk_label_new(NULL);
    gtk_label_set_selectable(GTK_LABEL(label_codec_tx), TRUE);
    if (tse->asession->node_codec_tx != NULL)
    {
	gtk_label_set_label(GTK_LABEL(label_codec_tx), tse->asession->node_codec_tx->pc->name);
    }
    else gtk_label_set_label(GTK_LABEL(label_codec_tx), IO_PCM_DESCRIPTION);
    gtk_box_pack_start(GTK_BOX(vbox_aux), hbox_aux, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_aux), label1, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_aux), label_codec_tx, FALSE, TRUE, 0);

    hbox_aux = gtk_hbox_new(FALSE, 10);
    label1 = gtk_label_new("<span color=\"red\"><big><b>          RX Audio codec : </b></big></span>");
    gtk_label_set_use_markup(GTK_LABEL(label1), TRUE);
    label_codec_rx = gtk_label_new(NULL);
    if (tse->asession->node_codec_rx != NULL)
    {
	gtk_label_set_label(GTK_LABEL(label_codec_rx), tse->asession->node_codec_rx->pc->name);
    }
    else gtk_label_set_label(GTK_LABEL(label_codec_rx), IO_PCM_DESCRIPTION);
    gtk_box_pack_start(GTK_BOX(vbox_aux), hbox_aux, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_aux), label1, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_aux), label_codec_rx, FALSE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), frame_aux, FALSE, FALSE, 5);

    hbox = gtk_hbox_new(TRUE, 30);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 8);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    button_about_codec = button_image_from_stock("_Acerca de" ,GTK_STOCK_PROPERTIES, GTK_ICON_SIZE_BUTTON);
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), button_about_codec, "Informacion del plugin seleccionado", NULL );
    g_object_set_data(G_OBJECT(button_about_codec),"treeview", treeview_codec);
    g_object_set_data(G_OBJECT(button_about_codec),"tipo", GINT_TO_POINTER(1));
    g_signal_connect(G_OBJECT(button_about_codec), "clicked", G_CALLBACK(configure_about_codec_user), tse);

    button_configure_codec = button_image_from_stock("_Configurar" ,GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_BUTTON);
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), button_configure_codec, "Configuracion del plugin seleccionado", NULL );
    g_object_set_data(G_OBJECT(button_configure_codec),"treeview", treeview_codec);
    g_object_set_data(G_OBJECT(button_configure_codec),"tipo", GINT_TO_POINTER(2));
    g_signal_connect(G_OBJECT(button_configure_codec), "clicked", G_CALLBACK(configure_about_codec_user), tse);

    button_init_codec = button_image_from_stock("_Seleccionar" ,GTK_STOCK_CONVERT, GTK_ICON_SIZE_BUTTON);
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), button_init_codec, "Inicializa y cambia la session al codec seleccionado. CUIDADO!: Si cambia esta opcion, asegurese que su interlocutor tiene un  programa RTP que soporta el cambio dinamico de codec de audio.", NULL );
    g_object_set_data(G_OBJECT(button_init_codec),"treeview", treeview_codec);
    g_object_set_data(G_OBJECT(button_init_codec),"label_codec_tx", label_codec_tx);

    g_signal_connect(G_OBJECT(button_init_codec), "clicked", G_CALLBACK(change_user_codec), tse);

    gtk_box_pack_start(GTK_BOX(hbox), button_about_codec, TRUE, TRUE, 10);
    gtk_box_pack_start(GTK_BOX(hbox), button_configure_codec, TRUE, TRUE, 10);
    gtk_box_pack_start(GTK_BOX(hbox), button_init_codec, TRUE, TRUE, 10);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);

    gtk_widget_show_all(dialog);
    return;
}



gboolean destroy_session (gint id)
{
    NodeVoCodec *nodecodec;
    TSession *tse;
    GSList *list;
    GError *tmp_error = NULL;
    gint counter;
    gboolean b = FALSE;
    gint page;


    counter = 0;
    g_mutex_lock(mutex_list_session_users);
    tse = (TSession *) g_slist_nth_data(list_session_users, counter);
    while (tse != NULL)
    {
	if (tse->id == id)
	{
            b = TRUE;
	    break;
	}
	tse = (TSession *) g_slist_nth_data(list_session_users, ++counter);
    }
    if (b == FALSE)
    {
	g_mutex_unlock(mutex_list_session_users);
	return FALSE;
    }
    list = g_slist_nth(list_session_users, counter);
    list_session_users = g_slist_remove_link(list_session_users, list);

    g_mutex_unlock(mutex_list_session_users);

    tse->asession->bye = TRUE;
    tse->asession->process = FALSE;
    *(tse->asession->mute_mic) = FALSE;

    page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook_users));
    gtk_notebook_remove_page(GTK_NOTEBOOK(notebook_users), page);
    gtk_widget_queue_draw(GTK_WIDGET(notebook_users));
    gtk_window_resize(GTK_WINDOW(window), 1, 1);
    gtk_list_store_clear(tse->store_codec);
    gtk_list_store_clear(tse->store_effect);

    g_thread_join(tse->loop);
    g_free(tse->asession->vad);
    g_free(tse->asession->mute_mic);
    g_free(tse->asession->mute_vol);
    g_free(tse->asession->gaim_rx);

    nodecodec = tse->asession->node_codec_rx;
    if (nodecodec != NULL) codec_destroy(nodecodec->codec);

    if ((tse->asession->node_codec_rx != NULL) && (tse->asession->node_codec_tx != NULL))
    {
	if (tse->asession->node_codec_rx->pc->payload != tse->asession->node_codec_tx->pc->payload)
	{
	    nodecodec = tse->asession->node_codec_tx;
	    if (nodecodec != NULL) codec_destroy(nodecodec->codec);
	}
    }
    else
    {
	nodecodec = tse->asession->node_codec_tx;
	if (nodecodec != NULL) codec_destroy(nodecodec->codec);
    }

    clear_list_plugins_codec(tse->plugins_codec);
    clear_list_plugins_effect(tse->asession->plugins_effect);
    tse->asession->plugins_auth = NULL;

    destroy_rtp_session(tse->asession->rtps, &tmp_error);
    if (tmp_error != NULL) g_error_free(tmp_error);

    g_free(tse->asession->ap_dsp);
    audio_processor_loop_free_mic(tse->asession->id_queue_in);
    audio_processor_loop_free_spk(tse->asession->id_queue_out);

    destroy_audio_processor();

    g_mutex_free(tse->asession->rtpinfo->mutex);
    g_free(tse->asession->rtpinfo);
    g_free(tse->asession);
    destroy_InitRTPsession(tse->irtpsession);
    g_free(tse->userhost);
    cfgf_free(tse->cfg);
    g_free(tse);
    g_slist_free_1(list);

    return TRUE;
}



TSession *create_session (gchar *user, gchar *host, gchar *image_user, InitRTPsession *init_param,
			  gint selected_rx_codec_payload, gint selected_tx_codec_payload, gint id)
{
    GError *tmp_error = NULL;
    LFunctionPlugin_Effect fe;
    LFunctionPlugin_Codec fc;
    GModule *mod;
    GtkListStore *store_effect;
    GSList *list_plugins_effect = NULL;
    GtkListStore *store_codec;
    GSList *list_plugins_codec = NULL;
    GtkTreeIter iter;
    GtkTreeIter iterp;
    GString* string = NULL;
    NodeVoCodec *pnc;
    NodeVoCodec *codec_rx;
    NodeVoCodec *codec_tx;
    PluginEffectNode *pn;
    gboolean selected_tx_codec = FALSE;
    gboolean selected_rx_codec = FALSE;
    GAsyncQueue *gap1;
    GAsyncQueue *gap2;
    TSession *tse;
    gboolean valid = TRUE;
    gint num_user;
    GtkWidget *frame_tab_user;
    GtkWidget *label_user;
    GtkWidget *hbox;
    GtkWidget *frame_image;
    GtkWidget *vbox;
    GtkWidget *hbox_aux;
    GtkWidget *frame_user;
    GtkWidget *image;
    GtkObject *adj_user_vol;
    GtkWidget *h_separator;
    GtkWidget *vscale_user_vol;
    GtkWidget *table_button;
    GtkWidget *check_button_mute;
    GtkWidget *check_button_mic_mute;
    GtkWidget *button_user;
    GtkWidget *button_exit;
    GtkWidget *check_button_vad;
    GtkWidget *button_plugins;
    GtkTooltips *tips_help;
    GtkWidget *dialog;


    if (id < 1)	return NULL;
    num_user = id;

    if (selected_rx_codec_payload == IO_PCM_PAYLOAD)
    {
	selected_rx_codec = TRUE;
	codec_rx = NULL;
    }
    if (selected_tx_codec_payload == IO_PCM_PAYLOAD)
    {
	selected_tx_codec = TRUE;
	codec_tx = NULL;
    }

    store_codec = gtk_list_store_new(CP_NUM_COLUMNS, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING);
    valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store_codec_plugin), &iterp);
    while (valid)
    {
        gchar *path;
	gchar *filename;
	gchar *name;
        gint payload;
        gchar *description;
	gint number;
        gint bw;

	gtk_tree_model_get(GTK_TREE_MODEL(store_codec_plugin), &iterp,
			   CP_COLUMN_ID, &number,
			   CP_COLUMN_NAME, &name,
			   CP_COLUMN_PAYLOAD, &payload,
			   CP_COLUMN_BW, &bw,
			   CP_COLUMN_FILENAME, &filename,
			   CP_COLUMN_DESCRIPTION, &description, -1);
	if (number == 1)
	{
	    gtk_list_store_append(store_codec, &iter);
	    gtk_list_store_set(store_codec, &iter,
			       CP_COLUMN_ID, number,
			       CP_COLUMN_NAME, name,
			       CP_COLUMN_PAYLOAD, payload,
			       CP_COLUMN_BW, bw,
			       CP_COLUMN_FILENAME, filename,
			       CP_COLUMN_DESCRIPTION, description,
			       -1);
	}
	else
	{
	    path = g_module_build_path(plugins_codecs_dir, filename);
	    mod = g_module_open(path, 0);
	    if (mod)
	    {
		if (!g_module_symbol(mod, PLUGIN_CODEC_STRUCT, (gpointer *) &fc))
		{
		    if (string == NULL) g_string_new("Error cargando simbolos de lo(s) plugin(s): ");
		    else g_string_append_printf(string, "(Codec) %s", filename);
		    g_printerr("[KERNEL] create_session: Error cargado simbolo 'get_Plugin_Codec_struct' de %s : %s", filename, g_module_error());
		}
		else
		{
		    pnc = (NodeVoCodec *) g_malloc(sizeof(NodeVoCodec));
		    pnc->module = mod;
		    pnc->pc = fc();
		    pnc->codec = pnc->pc->constructor();
		    pnc->id = number;
		    pnc->selected_rx = FALSE;
		    pnc->selected_tx = FALSE;
		    if ((selected_rx_codec_payload == pnc->pc->payload) && (!selected_rx_codec))
		    {
			selected_rx_codec = TRUE;
			pnc->selected_rx = TRUE;
			codec_rx = pnc;
		    }
		    if ((selected_tx_codec_payload == pnc->pc->payload) && (!selected_tx_codec))
		    {
			selected_tx_codec = TRUE;
			pnc->selected_tx = TRUE;
			codec_tx = pnc;
		    }
		    gtk_list_store_append(store_codec, &iter);
		    gtk_list_store_set(store_codec, &iter,
				       CP_COLUMN_ID, number,
				       CP_COLUMN_NAME, name,
				       CP_COLUMN_PAYLOAD, pnc->pc->payload,
				       CP_COLUMN_BW, pnc->pc->bw,
				       CP_COLUMN_FILENAME, filename,
				       CP_COLUMN_DESCRIPTION, pnc->pc->description,
				       -1);
		    list_plugins_codec = g_slist_append(list_plugins_codec, pnc);
		}
	    }
	    g_free(path);
	}
	g_free(filename);
	g_free(name);
	g_free(description);
	valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store_codec_plugin), &iterp);
    }
    if ((!selected_rx_codec) || (!selected_tx_codec))
    {
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
					"No existen codec disponibles para codificar/descodificar audio con payload '%d' para transmitir y payload '%d' para recibir.",
					selected_tx_codec_payload, selected_rx_codec_payload);
	g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_widget_show_all(dialog);
        gtk_list_store_clear(store_codec);
	clear_list_plugins_codec(list_plugins_codec);
	destroy_audio_processor();
        return NULL;
    }

    tse = (TSession *) g_malloc(sizeof(TSession));
    tse->id = (num_user * 10);
    tse->cfg = cfgf_open_file(configuration_filename, &tmp_error);
    if (tmp_error != NULL)
    {
	show_error_configfile(tmp_error);
	g_free(tse);
	gtk_list_store_clear(store_codec);
	clear_list_plugins_codec(list_plugins_codec);
	destroy_audio_processor();
        return NULL;
    }
    store_effect = gtk_list_store_new(EP_NUM_COLUMNS, G_TYPE_INT, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING);
    valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store_effect_plugin), &iterp);
    while (valid)
    {
        gchar *path;
	gchar *filename;
	gchar *name;
        gchar *description;
	gboolean active;
        gint number;


	gtk_tree_model_get(GTK_TREE_MODEL(store_effect_plugin), &iterp,
			   EP_COLUMN_ID, &number,
			   EP_COLUMN_NAME, &name,
			   EP_COLUMN_ACTIVE, &active,
			   EP_COLUMN_FILENAME, &filename,
			   EP_COLUMN_DESCRIPTION, &description, -1);

        path = g_module_build_path(plugins_effect_dir, filename);
	mod = g_module_open(path, 0);
	if (mod)
	{
	    if (!g_module_symbol(mod, PLUGIN_EFFECT_STRUCT, (gpointer *) &fe))
	    {
		if (string == NULL) g_string_new("Error cargando simbolos de lo(s) plugin(s): ");
		else g_string_append_printf(string, "(Effect) %s", filename);
		g_printerr("[KERNEL] create_session: Error cargado simbolo 'get_Plugin_Effect_struct' de %s : %s", filename, g_module_error());
	    }
	    else
	    {
		pn = (PluginEffectNode *) g_malloc(sizeof(PluginEffectNode));
		pn->module = mod;
		pn->plugin = fe();
		pn->id = number;
		if (active)
		{
		    pn->state = pn->plugin->init(tse->cfg, audio_properties, &tmp_error);
		    if (tmp_error != NULL)
		    {
                        show_error_module_noinit(tmp_error);
			active = FALSE;
                        pn->state = NULL;
		    }
		}
		else pn->state = NULL;

		pn->active = active;
		gtk_list_store_append(store_effect, &iter);
		gtk_list_store_set(store_effect, &iter,
				   EP_COLUMN_ID, number,
				   EP_COLUMN_NAME, name,
				   EP_COLUMN_ACTIVE, active,
				   EP_COLUMN_FILENAME, filename,
				   EP_COLUMN_DESCRIPTION, description,
				   -1);
		list_plugins_effect = g_slist_append(list_plugins_effect, pn);
	    }
	}
	g_free(filename);
	g_free(path);
	g_free(name);
	g_free(description);
	valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store_effect_plugin), &iterp);
    }

    if (string != NULL)
    {
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                        g_string_free(string, TRUE));
	g_signal_connect_swapped(GTK_OBJECT(dialog),"response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
        gtk_widget_show_all(dialog);
    }

    tse->store_codec = store_codec;
    tse->store_effect = store_effect;
    tse->userhost = g_strdup_printf("%s@%s", user, host);
    tse->asession = (ASession *) g_malloc(sizeof(ASession));
    tse->asession->crypt = FALSE;

    tse->irtpsession = (InitRTPsession *) g_malloc(sizeof(InitRTPsession));
    memcpy(tse->irtpsession, init_param, sizeof(InitRTPsession));

    tse->asession->rtps = create_rtp_session(tse->irtpsession, &tmp_error);
    if (tmp_error != NULL)
    {
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
					"Imposible iniciar session RTP/RTCP: %s", tmp_error->message);
	g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_widget_show_all(dialog);
	g_error_free(tmp_error);
	gtk_list_store_clear(store_codec);
	clear_list_plugins_codec(list_plugins_codec);
	gtk_list_store_clear(store_effect);
	clear_list_plugins_effect(list_plugins_effect);
	g_free(tse->asession);
	g_free(tse->userhost);
	destroy_InitRTPsession(tse->irtpsession);
	cfgf_free(tse->cfg);
	g_free(tse);
	destroy_audio_processor();
        return NULL;
    }
    if (!audio_processor_loop_create_mic(tse->id+1, &gap1))
    {
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
					"Imposible crear cola lectora de audio.");
	g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_widget_show_all(dialog);
	destroy_rtp_session(tse->asession->rtps, NULL);
	gtk_list_store_clear(store_codec);
	clear_list_plugins_codec(list_plugins_codec);
	gtk_list_store_clear(store_effect);
        clear_list_plugins_effect(list_plugins_effect);
	g_free(tse->asession);
        cfgf_free(tse->cfg);
	g_free(tse->userhost);
	destroy_InitRTPsession(tse->irtpsession);
	g_free(tse);
	destroy_audio_processor();
        return NULL;
    }
    if (!audio_processor_loop_create_spk(tse->id+2, &gap2))
    {
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
					"Imposible crear cola escritora de audio.");
	g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_widget_show_all(dialog);
	audio_processor_loop_free_mic(tse->id+1);
	destroy_rtp_session(tse->asession->rtps, NULL);
	gtk_list_store_clear(store_codec);
	clear_list_plugins_codec(list_plugins_codec);
	gtk_list_store_clear(store_effect);
        clear_list_plugins_effect(list_plugins_effect);
	g_free(tse->asession);
        cfgf_free(tse->cfg);
	g_free(tse->userhost);
	destroy_InitRTPsession(tse->irtpsession);
	g_free(tse);
	destroy_audio_processor();
        return NULL;
    }
    tse->asession->blocksize_dsp = audio_blocksize;
    tse->asession->ap_dsp = (AudioProperties *) g_malloc(sizeof(AudioProperties));
    memcpy(tse->asession->ap_dsp, audio_properties, sizeof(AudioProperties));
    tse->asession->id_queue_in = tse->id+1;
    tse->asession->queue_in = gap1;
    tse->asession->id_queue_out = tse->id+2;
    tse->asession->queue_out = gap2;
    tse->asession->vad_state = &audio_vad;
    tse->asession->process = TRUE;

    tse->asession->vad = (gint *) g_malloc(sizeof(gint));
    *(tse->asession->vad) = FALSE;
    tse->asession->mute_mic = (gint *) g_malloc(sizeof(gint));
    *(tse->asession->mute_mic) = FALSE;
    tse->asession->mute_vol = (gint *) g_malloc(sizeof(gint));
    *(tse->asession->mute_vol) = FALSE;
    tse->asession->gaim_rx = (gfloat *) g_malloc(sizeof(gfloat));
    *(tse->asession->gaim_rx) = 1.0;

    tse->asession->node_codec_rx = codec_rx;
    if (codec_rx != NULL)
    {
	codec_init(tse->asession->node_codec_rx->codec, tse->asession->ap_dsp, tse->cfg, CODEC_DECODE, &tmp_error);
	if (tmp_error != NULL)
	{
	    show_error_module_noinit(tmp_error);

	    audio_processor_loop_free_mic(tse->id+1);
	    audio_processor_loop_free_spk(tse->id+2);
	    destroy_rtp_session(tse->asession->rtps, NULL);
	    g_free(tse->userhost);
	    gtk_list_store_clear(tse->store_codec);
	    clear_list_plugins_codec(list_plugins_codec);
	    gtk_list_store_clear(tse->store_effect);
	    clear_list_plugins_effect(list_plugins_effect);
	    cfgf_free(tse->cfg);
	    g_free(tse->asession->ap_dsp);
	    g_free(tse->asession->vad);
	    g_free(tse->asession->mute_mic);
	    g_free(tse->asession->mute_vol);
	    g_free(tse->asession->gaim_rx);
	    g_free(tse->asession);
	    destroy_InitRTPsession(tse->irtpsession);
	    g_free(tse);
	    destroy_audio_processor();
	    return NULL;
	}
    }
    tse->asession->node_codec_tx = codec_tx;
    if (codec_tx != NULL)
    {
	codec_init(tse->asession->node_codec_tx->codec, tse->asession->ap_dsp, tse->cfg, CODEC_ENCODE, &tmp_error);
	if (tmp_error != NULL)
	{
	    show_error_module_noinit(tmp_error);

	    audio_processor_loop_free_mic(tse->id+1);
	    audio_processor_loop_free_spk(tse->id+2);
	    destroy_rtp_session(tse->asession->rtps, NULL);
	    g_free(tse->userhost);
	    gtk_list_store_clear(tse->store_codec);
	    clear_list_plugins_codec(list_plugins_codec);
	    gtk_list_store_clear(tse->store_effect);
	    clear_list_plugins_effect(list_plugins_effect);
	    cfgf_free(tse->cfg);
	    g_free(tse->asession->ap_dsp);
	    g_free(tse->asession->vad);
	    g_free(tse->asession->mute_mic);
	    g_free(tse->asession->mute_vol);
	    g_free(tse->asession->gaim_rx);
	    g_free(tse->asession);
	    destroy_InitRTPsession(tse->irtpsession);
	    g_free(tse);
	    destroy_audio_processor();
	    return NULL;
	}
    }
    tse->asession->plugins_effect = list_plugins_effect;
    tse->asession->plugins_auth = NULL;
    tse->plugins_codec = list_plugins_codec;

    tse->asession->rtpinfo = g_malloc0(sizeof(RTP_Info));
    tse->asession->rtpinfo->mutex = g_mutex_new();
    tse->asession->rtpinfo->show = FALSE;

    tse->loop = g_thread_create((GThreadFunc) session_io_loop, tse->asession, TRUE, &tmp_error);
    if (tmp_error != NULL)
    {
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
					"Imposible crear sesion thread de E/S de audio: %s.", tmp_error->message);
	g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_widget_show_all(dialog);
	destroy_partial_session(tse);
	destroy_audio_processor();
        return NULL;
    }

    // Notebook

    tips_help = gtk_tooltips_new();
    frame_tab_user = gtk_frame_new(NULL);
    gtk_container_set_border_width(GTK_CONTAINER(frame_tab_user), 2);

    label_user = gtk_label_new(user);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(frame_tab_user), hbox);

    frame_image = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame_image), GTK_SHADOW_IN);
    gtk_container_set_border_width (GTK_CONTAINER (frame_image), 5);
    gtk_box_pack_start(GTK_BOX(hbox), frame_image, TRUE, FALSE, 0);

    vbox= gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(frame_image), vbox);

    hbox_aux = gtk_hbox_new(FALSE, 0);
    frame_user = gtk_frame_new(NULL);

    gtk_frame_set_shadow_type(GTK_FRAME(frame_user), GTK_SHADOW_OUT);
    gtk_container_set_border_width(GTK_CONTAINER(frame_user), 10);

    image = load_scaled_image_from_file(image_user, GUI_IMAGE_USER_SIZEX, GUI_IMAGE_USER_SIZEY, GDK_INTERP_BILINEAR, NULL);

    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), image, tse->userhost, NULL);
    gtk_container_add(GTK_CONTAINER(frame_user), image);
    gtk_box_pack_start(GTK_BOX(hbox_aux), frame_user, TRUE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_aux, TRUE, TRUE, 0);

    button_user = gtk_button_new_with_label(user);
    gtk_button_set_relief(GTK_BUTTON(button_user), GTK_RELIEF_NONE);
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), button_user, "Informacion adiccional de esta session", NULL );
    g_signal_connect(button_user, "clicked", G_CALLBACK(show_session_info), tse);
    gtk_box_pack_start(GTK_BOX(vbox), button_user, TRUE, TRUE, 0);

    /* value, lower, upper, step_increment, page_increment, page_size        */
    /* Note that the page_size value only makes a difference for             */
    /* scrollbar widgets, and the highest value you'll get is actually       */
    /* (upper - page_size).                                                  */
    adj_user_vol = gtk_adjustment_new(0.0, -20.0, 21.0, 1.0, 6.0, 6.0);
    g_signal_connect(G_OBJECT(adj_user_vol), "value_changed", G_CALLBACK(change_user_vol), tse);

    vbox = gtk_vbox_new(TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
    vscale_user_vol = gtk_vscale_new(GTK_ADJUSTMENT(adj_user_vol));
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), vscale_user_vol, "Factor de amplificacion del audio para este usuario (en dB)", NULL );

    gtk_range_set_update_policy(GTK_RANGE(vscale_user_vol), GTK_UPDATE_DELAYED);
    gtk_range_set_inverted(GTK_RANGE(vscale_user_vol), TRUE);
    gtk_scale_set_digits(GTK_SCALE(vscale_user_vol), 0);
    gtk_scale_set_value_pos(GTK_SCALE(vscale_user_vol), GTK_POS_BOTTOM);
    gtk_scale_set_draw_value(GTK_SCALE(vscale_user_vol), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), vscale_user_vol, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);

    table_button = gtk_table_new(6, 1, FALSE);
    gtk_box_pack_start(GTK_BOX(hbox), table_button, TRUE, TRUE, 0);
    check_button_mute = gtk_check_button_new_with_mnemonic("_Enmudecer");
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), check_button_mute, "Hacer callar al usuario / sincronizar audio", NULL);
    g_signal_connect(check_button_mute, "toggled", G_CALLBACK(toggle_button_user_mute), tse);

    check_button_mic_mute = gtk_check_button_new_with_mnemonic("_Cerrar microfono");
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), check_button_mic_mute, "Cerrar el microfono a este usuario", NULL);
    g_signal_connect(check_button_mic_mute, "toggled", G_CALLBACK(toggle_button_user_mic_mute), tse);

    check_button_vad = gtk_check_button_new_with_mnemonic("_Activar VAD");
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), check_button_vad, "Activar el sistema de deteccion de voz (VAD) para esta session", NULL );
    g_signal_connect(check_button_vad, "toggled", G_CALLBACK(toggle_button_user_vad), tse);

    gtk_table_attach(GTK_TABLE(table_button), check_button_mute, 0, 1, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 10, 5);
    gtk_table_attach(GTK_TABLE(table_button), check_button_mic_mute, 0, 1, 1, 2, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 10, 5);
    gtk_table_attach(GTK_TABLE(table_button), check_button_vad, 0, 1, 2, 3, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 10, 5);

    h_separator = gtk_hseparator_new();
    gtk_table_attach(GTK_TABLE(table_button), h_separator, 0, 1, 3, 4, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 10, 5);

    button_plugins = button_image_from_stock("_Plugins" ,GTK_STOCK_INDEX, GTK_ICON_SIZE_BUTTON);
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), button_plugins, "Informacion de los plugins disponibles", NULL );
    g_signal_connect(G_OBJECT(button_plugins), "clicked", G_CALLBACK(show_plugins_user), tse);

    button_exit = button_image_from_stock("_Terminar" ,GTK_STOCK_QUIT, GTK_ICON_SIZE_BUTTON);
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), button_exit, "Terminar esta conversacion", NULL);
    g_signal_connect(G_OBJECT(button_exit), "clicked", G_CALLBACK(destroy_user_session), tse);

    gtk_table_attach(GTK_TABLE(table_button), button_plugins, 0, 1, 4, 5, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 10, 5);
    gtk_table_attach(GTK_TABLE(table_button), button_exit, 0, 1, 5, 6, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 10, 5);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook_users), frame_tab_user, label_user);

    gtk_window_present(GTK_WINDOW(window));
    gtk_widget_show_all(window);
    if (!vbox_vol_visible) gtk_widget_hide(vbox_volume);

    g_mutex_lock(mutex_list_session_users);
    list_session_users = g_slist_append(list_session_users, tse);
    g_mutex_unlock(mutex_list_session_users);

    return tse;
}


