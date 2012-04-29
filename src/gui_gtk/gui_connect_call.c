
/*
 *
 * FILE: gui_connect_call.c
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

#include "gui_connect_call.h"



static GtkWidget *frame_advanced;
static GtkWidget *entry_user;
static GtkWidget *entry_host;
static GtkWidget *entry_port;
static GtkWidget *entry_msg;
static GtkWidget *entry_rtp_rxport;
static GtkWidget *entry_rtp_txport;
static GtkWidget *checkbutton_des;
static GtkWidget *entry_des;
static GtkWidget *checkbutton_neg_session;
static GtkWidget *button_advanced;
static GtkWidget *dialog1;
static GtkListStore *model_codeclist;
static GtkWidget *treeview;
static ItemUser *sel_user = NULL;




static void seleted_user (ItemUser *user)
{
    gtk_entry_set_text(GTK_ENTRY(entry_user), user->user);
    gtk_entry_set_text(GTK_ENTRY(entry_host), user->host);

    if (sel_user != NULL)
    {
        // ojo aqui queda memoria pillada, hay un bug
/*
	g_free(sel_user->user);
	g_free(sel_user->host);
	g_free(sel_user->photo);
	g_free(sel_user->description);
	g_free(sel_user->name);
	g_free(sel_user);
*/
    }
    else
    {
        sel_user = user;
    }
    return;
}



static void cb_button_addbook (GtkButton *button, ConfigFile *fileconf)
{
    UNUSED(button);
    gchar *archive;


    if (cfgf_read_string(fileconf, PROGRAM_GLOBAL_SECTION, PROGRAM_DIR_DB, &archive))
    {
	create_addressbook_window(archive, (CB_gui_addbook) seleted_user);
        g_free(archive);
    }
}



static void cb_button_advanced (GtkButton *button, gchar *data)
{
    UNUSED(button);
    UNUSED(data);


    if (!GTK_TOGGLE_BUTTON(button_advanced)->active)
    {
	gtk_dialog_set_has_separator(GTK_DIALOG(dialog1), TRUE);
	gtk_widget_hide(frame_advanced);
	gtk_window_resize(GTK_WINDOW(dialog1), 450, 1);
    }
    else
    {
	gtk_dialog_set_has_separator(GTK_DIALOG(dialog1), FALSE);
	gtk_window_resize(GTK_WINDOW(dialog1), 450, 501);
	gtk_widget_show_all(dialog1);
	//gtk_widget_show_all(frame_advanced);
    }
}



static void cb_button_del (GtkButton *button, gchar *data)
{
    GtkTreeSelection *selection;


    UNUSED(button);
    UNUSED(data);

    gtk_editable_delete_text(GTK_EDITABLE(entry_des), 0, -1);
    gtk_editable_delete_text(GTK_EDITABLE(entry_rtp_rxport), 0, -1);
    gtk_editable_delete_text(GTK_EDITABLE(entry_rtp_txport), 0, -1);
    gtk_editable_delete_text(GTK_EDITABLE(entry_msg), 0, -1);
    gtk_editable_delete_text(GTK_EDITABLE(entry_user), 0, -1);
    gtk_editable_delete_text(GTK_EDITABLE(entry_host), 0, -1);
    gtk_editable_delete_text(GTK_EDITABLE(entry_port), 0, -1);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton_neg_session), FALSE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton_des), FALSE);

    gtk_widget_set_sensitive(entry_des, FALSE);
    gtk_widget_set_sensitive(entry_rtp_txport, FALSE);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
    gtk_tree_selection_unselect_all(GTK_TREE_SELECTION(selection));
    if (sel_user != NULL)
    {
        // ojo aqui queda memoria pillada, hay un bug
/*
	g_free(sel_user->user);
	g_free(sel_user->host);
	g_free(sel_user->photo);
	g_free(sel_user->description);
	g_free(sel_user->name);
	g_free(sel_user);
*/
    }
    sel_user = NULL;
}



static void toggle_neg_session (GtkWidget *checkbutton, gpointer data)
{
    UNUSED(checkbutton);
    UNUSED(data);


    if (GTK_TOGGLE_BUTTON(checkbutton_neg_session)->active)
    {
	gtk_widget_set_sensitive(entry_rtp_txport, TRUE);
    }
    else
    {
	gtk_editable_delete_text(GTK_EDITABLE(entry_rtp_txport), 0, -1);
	gtk_widget_set_sensitive(entry_rtp_txport, FALSE);
    }
}



static void toggle_des (GtkWidget *checkbutton, gpointer data)
{
    UNUSED(checkbutton);
    UNUSED(data);


    if (GTK_TOGGLE_BUTTON(checkbutton_des)->active)
    {
	gtk_widget_set_sensitive(entry_des, TRUE);
    }
    else
    {
	gtk_editable_delete_text(GTK_EDITABLE(entry_des), 0, -1);
	gtk_widget_set_sensitive(entry_des, FALSE);
    }
}



static void create_model (GSList *list_codec)
{
    Node_listvocodecs *pn;
    gint j;
    GtkTreeIter iter;


    j = 0;
    model_codeclist = gtk_list_store_new(CL_NUM_COLUMNS, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT);
    pn = (Node_listvocodecs *) g_slist_nth_data(list_codec, j);
    while (pn)
    {
	gtk_list_store_append(GTK_LIST_STORE(model_codeclist), &iter);
	gtk_list_store_set(GTK_LIST_STORE(model_codeclist), &iter,
			   CL_COLUMN_NUMBER, (j + 1),
			   CL_COLUMN_NAME, pn->name,
			   CL_COLUMN_PAYLOAD, pn->payload,
			   CL_COLUMN_BW, pn->bw,
			   -1);
	pn = (Node_listvocodecs *) g_slist_nth_data(list_codec, ++j);
    }
}



static gint progress_timeout (gpointer data)
{
    ProgressData *pdata = (ProgressData *) data;

    gtk_progress_bar_pulse(GTK_PROGRESS_BAR(pdata->pbar));
    return TRUE;
}



static void destroy_progress (GtkWidget *widget, ProgressData *pdata)
{
    UNUSED(widget);
    gtk_timeout_remove(pdata->timer);
    pdata->timer = 0;
    pdata->window = NULL;
    *pdata->exit = 0;
    g_free(pdata);
}



static gboolean delete_progress (GtkWidget *widget, GdkEvent *event, ProgressData *pdata)
{
    UNUSED(widget);
    UNUSED(event);


    *pdata->exit = 0;
    return TRUE;
}



void button_cancel (GtkDialog *dialog, gint arg1, ProgressData *pdata)
{
    UNUSED(dialog);
    UNUSED(arg1);

    *pdata->exit = 0;
}



// main functions



ProgressData *gui_connect_progresbar (GtkWidget *main_window, gint *_exit,
				      gchar *from_user, gchar *from_host,
				      gchar *to_user, gchar *to_host)
{
    ProgressData *pdata;
    GtkWidget *align;
    GtkWidget *separator;
    GtkWidget *table;
    GtkWidget *label;
    GtkWidget *hbox;


    pdata = g_malloc(sizeof(ProgressData));
    pdata->exit = _exit;

    pdata->window = main_window;
    pdata->dialog = gtk_dialog_new_with_buttons("Progreso de la negociacion", GTK_WINDOW(main_window),
						GTK_DIALOG_DESTROY_WITH_PARENT, NULL);
    //    					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
    gtk_window_set_resizable(GTK_WINDOW(pdata->dialog), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(pdata->dialog), 10);
    g_signal_connect(GTK_OBJECT(pdata->dialog), "destroy", G_CALLBACK(destroy_progress), pdata);
    g_signal_connect(GTK_OBJECT(pdata->dialog), "delete_event", G_CALLBACK(delete_progress), pdata);
    //g_signal_connect(GTK_OBJECT(pdata->dialog), "response", G_CALLBACK(button_cancel), pdata);
    //g_signal_connect(GTK_OBJECT(pdata->dialog), "response", G_CALLBACK(delete_progress), pdata);

    gtk_container_set_border_width(GTK_CONTAINER(GTK_DIALOG(pdata->dialog)->vbox), 30);
    gtk_box_set_spacing(GTK_BOX(GTK_DIALOG(pdata->dialog)->vbox), 5);

    table = gtk_table_new(3, 1, FALSE);
    gtk_table_set_col_spacings(GTK_TABLE(table), 5);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pdata->dialog)->vbox), table, FALSE, FALSE, 5);


    hbox = gtk_hbox_new(FALSE, 0);
    label = gtk_label_new(from_user);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<span color=\"blue\"><big><b>@</b></big></span>");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    label = gtk_label_new(from_host);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_table_attach_defaults(GTK_TABLE(table), hbox, 0, 1, 0, 1);

    align = gtk_alignment_new(0.5, 0.5, 0, 0);
    gtk_table_attach_defaults(GTK_TABLE(table), align, 1, 2, 0, 1);
    pdata->pbar = gtk_progress_bar_new();
    gtk_container_add (GTK_CONTAINER (align), pdata->pbar);
    pdata->timer = gtk_timeout_add (100, progress_timeout, pdata);

    hbox = gtk_hbox_new(FALSE, 0);
    label = gtk_label_new(to_host);
    gtk_box_pack_end(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<span color=\"blue\"><big><b>@</b></big></span>");
    gtk_box_pack_end(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    label = gtk_label_new(to_user);
    gtk_box_pack_end(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_table_attach_defaults(GTK_TABLE(table), hbox, 2, 3, 0, 1);

    separator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pdata->dialog)->vbox), separator, TRUE, TRUE, 0);

    pdata->label = gtk_label_new(NULL);
    gtk_label_set_line_wrap(GTK_LABEL(pdata->label), TRUE);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pdata->dialog)->vbox), pdata->label, TRUE, TRUE, 0);

    gtk_widget_show_all(pdata->dialog);
    return pdata;
}



gboolean create_connect_dialog (ConfigFile *conffile, GtkWidget *main_window, GSList *list_codec, GuiConnect **gui_connect)
{
    GtkWidget *dialog_vbox1;
    GtkWidget *vbox;
    GtkWidget *button_addbook;
    GtkWidget *hbox;
    GtkWidget *button_del;
    GtkWidget *frame;
    GtkWidget *scrolledwindow1;
    GtkWidget *label;
    GtkWidget *hbox_rtp_ports;
    GtkWidget *alignment;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeSelection *select;
    GtkTooltips *tips_help;
    gint result;


    dialog1 = gtk_dialog_new_with_buttons("Conectar", GTK_WINDOW(main_window),
					  GTK_DIALOG_DESTROY_WITH_PARENT,
					  GTK_STOCK_OK, GTK_RESPONSE_OK,
					  GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
					  NULL);
    gtk_dialog_set_has_separator(GTK_DIALOG(dialog1), TRUE);
    tips_help = gtk_tooltips_new();
    dialog_vbox1 = GTK_DIALOG(dialog1)->vbox;
    gtk_widget_show(dialog_vbox1);

    vbox = gtk_vbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(dialog_vbox1), vbox, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_widget_show(vbox);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
    gtk_widget_show(hbox);

    alignment = gtk_alignment_new(0.5, 0.5, 0, 1);
    gtk_box_pack_start(GTK_BOX(hbox), alignment, FALSE, FALSE, 0);
    gtk_widget_show(alignment);
    entry_user = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(entry_user), 10);
    gtk_entry_set_width_chars(GTK_ENTRY(entry_user), 12);
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), entry_user, "Usuario con quien conectar", NULL);
    gtk_container_add(GTK_CONTAINER(alignment), entry_user);
    gtk_entry_set_max_length(GTK_ENTRY(entry_user), PKT_USERNAME_LEN);
    gtk_widget_show(entry_user);

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<span color=\"blue\"><big><b> @ </b></big></span>");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    gtk_widget_show(label);

    alignment = gtk_alignment_new(0.5, 0.5, 0, 1);
    gtk_box_pack_start(GTK_BOX(hbox), alignment, FALSE, FALSE, 0);
    gtk_widget_show(alignment);
    entry_host = gtk_entry_new();
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), entry_host, "Host o IP de la maquina en donde esta el usuario", NULL);
    gtk_container_add(GTK_CONTAINER(alignment), entry_host);
    gtk_entry_set_max_length(GTK_ENTRY(entry_host), PKT_HOSTNAME_LEN);
    gtk_widget_show(entry_host);

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<span color=\"blue\"><big><b> : </b></big></span>");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    gtk_widget_show(label);

    alignment = gtk_alignment_new(0.5, 0.5, 0, 1);
    gtk_box_pack_start(GTK_BOX(hbox), alignment, FALSE, FALSE, 0);
    gtk_widget_show(alignment);
    entry_port = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(entry_port), 5);
    gtk_entry_set_width_chars(GTK_ENTRY(entry_port), 8);
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), entry_port, "Puerto a donde realizar la conexion", NULL);
    gtk_container_add(GTK_CONTAINER(alignment), entry_port);
    gtk_entry_set_max_length(GTK_ENTRY(entry_port), PKT_HOSTNAME_LEN);
    gtk_widget_show(entry_port);

    button_addbook = gtk_button_new_from_stock("gtk-open");
    gtk_container_set_border_width(GTK_CONTAINER(button_addbook), 5);
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), button_addbook, "Acceder a la agenda de contactos", NULL);
    g_signal_connect(G_OBJECT(button_addbook), "clicked", G_CALLBACK(cb_button_addbook), conffile);
    gtk_box_pack_end(GTK_BOX(hbox), button_addbook, FALSE, FALSE, 0);
    gtk_button_set_relief(GTK_BUTTON(button_addbook), GTK_RELIEF_NONE);
    gtk_widget_show(button_addbook);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
    gtk_widget_show(hbox);

    label = gtk_label_new("Motivo ");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    gtk_widget_show(label);

    entry_msg = gtk_entry_new ();
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), entry_msg, "Motivo por el que desea establecer la comunicacion (opcional)", NULL);
    gtk_box_pack_start(GTK_BOX (hbox), entry_msg, TRUE, TRUE, 0);
    gtk_entry_set_max_length(GTK_ENTRY(entry_msg), PKT_USERDES_LEN);
    gtk_widget_show(entry_msg);

    hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 10);
    gtk_widget_show(hbox);

    button_advanced = gtk_toggle_button_new_with_mnemonic("A_vanzado");
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), button_advanced, "Abrir/Ocultar propiedades avanzadas", NULL);
    g_signal_connect(G_OBJECT(button_advanced), "toggled", G_CALLBACK(cb_button_advanced), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), button_advanced, TRUE, TRUE, 0);
    gtk_widget_show(button_advanced);

    button_del = gtk_button_new_from_stock("gtk-delete");
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), button_del, "Borrar todos los datos establecidos", NULL);
    g_signal_connect(G_OBJECT(button_del), "clicked", G_CALLBACK(cb_button_del), NULL);
    gtk_box_pack_end(GTK_BOX(hbox), button_del, FALSE, FALSE, 0);
    gtk_widget_show(button_del);

    frame_advanced = gtk_frame_new("RTP/RTCP Options");
    gtk_box_pack_start(GTK_BOX(vbox), frame_advanced, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER (frame_advanced), 5);
    gtk_widget_show_all(frame_advanced);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(frame_advanced), vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
    gtk_widget_show_all(vbox);

    frame = gtk_frame_new("Codecs");
    gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), frame, "Selecione al menos un codec con el que iniciar la comunicacion, si no hay ningun codec seleccionado se usaran todos en el orden mostrado", NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
    gtk_widget_show_all(frame);
    gtk_widget_hide(frame_advanced);

    scrolledwindow1 = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow1), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
    gtk_container_add(GTK_CONTAINER(frame), scrolledwindow1);
    gtk_container_set_border_width(GTK_CONTAINER(scrolledwindow1), 5);

    // model
    create_model(list_codec);

    treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(model_codeclist));
    gtk_container_add(GTK_CONTAINER(scrolledwindow1), treeview);

    // columns
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Nombre", renderer,
						      "text", CL_COLUMN_NAME,
						      NULL);
    gtk_tree_view_column_set_min_width(column, 200);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Payload", renderer,
						      "text", CL_COLUMN_PAYLOAD,
						      NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
    gtk_tree_view_column_set_min_width(column, 70);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Band width", renderer,
						      "text", CL_COLUMN_BW,
						      NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    select = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
    gtk_tree_selection_set_mode(select, GTK_SELECTION_MULTIPLE);


    checkbutton_neg_session = gtk_check_button_new_with_mnemonic("Desabilitar negociado de inicio de sesion (SSIP)");
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), checkbutton_neg_session, "Desabilita el SSIP (Protocolo de inicio de sesion simple), con esta opcion no se realiza ningun negociado de sesion, es decir, no se comprueba si ambos usuarios tienen los codecs, etc. ,"
			 "simplemente comienza la transmion RTP/RTCP desde el puerto RX al puerto TX del host del usuario remoto. Esta opcion es util para interactuar con otros programas"
			 "RTP/RTCP que no soportan SSIP", NULL);
    gtk_box_pack_start(GTK_BOX(vbox), checkbutton_neg_session, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(checkbutton_neg_session), "toggled", G_CALLBACK(toggle_neg_session), NULL);
    gtk_container_set_border_width(GTK_CONTAINER(checkbutton_neg_session), 5);

    hbox_rtp_ports = gtk_hbox_new(FALSE, 10);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_rtp_ports, FALSE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(hbox_rtp_ports), 4);

    label = gtk_label_new("RTP RX port :");
    gtk_box_pack_start(GTK_BOX(hbox_rtp_ports), label, FALSE, FALSE, 0);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);

    entry_rtp_rxport = gtk_entry_new();
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), entry_rtp_rxport, "Puerto local para la session RTP/RTCP ", NULL);
    gtk_box_pack_start(GTK_BOX(hbox_rtp_ports), entry_rtp_rxport, TRUE, TRUE, 0);
    gtk_entry_set_max_length(GTK_ENTRY(entry_rtp_rxport), 5);
    gtk_entry_set_width_chars(GTK_ENTRY(entry_rtp_rxport), 0);

    label = gtk_label_new("RTP TX port:");
    gtk_box_pack_start(GTK_BOX(hbox_rtp_ports), label, FALSE, FALSE, 0);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);

    entry_rtp_txport = gtk_entry_new();
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), entry_rtp_txport, "Puerto de conexion al host remoto para la sesion RTP/RTCP ", NULL);
    gtk_box_pack_end(GTK_BOX(hbox_rtp_ports), entry_rtp_txport, TRUE, TRUE, 0);
    gtk_entry_set_max_length(GTK_ENTRY(entry_rtp_txport), 5);
    gtk_entry_set_width_chars(GTK_ENTRY(entry_rtp_txport), 0);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);

    checkbutton_des = gtk_check_button_new_with_mnemonic("Habilitar cifrado RTP/RTCP DES : ");
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), checkbutton_des, "Habilita el cifrado DES en la conexion RTP/RTCP, la contrasena debe ser la misma para ambos usuarios", NULL);
    g_signal_connect(G_OBJECT(checkbutton_des), "toggled", G_CALLBACK(toggle_des), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), checkbutton_des, FALSE, FALSE, 0);

    entry_des = gtk_entry_new();
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), entry_des, "Contrasena sesion RTP/RTCP", NULL);
    gtk_box_pack_start(GTK_BOX(hbox), entry_des, TRUE, TRUE, 0);

    gtk_widget_set_sensitive(entry_rtp_txport, FALSE);
    gtk_widget_set_sensitive(entry_des, FALSE);

    //gtk_widget_show_all(dialog1);
    //gtk_widget_hide(frame_advanced);
    gtk_window_resize(GTK_WINDOW(dialog1), 450, 1);
    result = gtk_dialog_run(GTK_DIALOG(dialog1));

    if (result == GTK_RESPONSE_OK)
    {
	GtkTreeSelection *selection;
	GSList *codec_sel_list = NULL;
	gint j;
	gint sels;
        Node_listvocodecs *pn;
        gchar *test1;
	gchar *test2;
        GtkWidget *dialog;


	*gui_connect = (GuiConnect *) g_malloc0(sizeof(GuiConnect));
        test1 = gtk_editable_get_chars(GTK_EDITABLE(entry_user), 0, PKT_USERNAME_LEN);
        test2 = gtk_editable_get_chars(GTK_EDITABLE(entry_host), 0, PKT_HOSTNAME_LEN);

	if ((strlen(test1) > 0) && (strlen(test2) > 3))
	{
	    (*gui_connect)->host = test2;
	    (*gui_connect)->username = test1;

	    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	    if ((sels  = gtk_tree_selection_count_selected_rows(selection)) > 0)
	    {
		gint control = 0;
		gint payload;
		GtkTreeIter iter;


		gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model_codeclist), &iter);
		do
		{
		    if (gtk_tree_selection_iter_is_selected(GTK_TREE_SELECTION(selection), &iter))
		    {
			gtk_tree_model_get(GTK_TREE_MODEL(model_codeclist), &iter, CL_COLUMN_PAYLOAD, &payload, -1);
			j = 0;
			pn = (Node_listvocodecs *) g_slist_nth_data(list_codec, j);
			while (pn)
			{
			    if (pn->payload == payload)
			    {
				codec_sel_list = g_slist_append(codec_sel_list, pn);
				control++;
				break;
			    }
			    pn = (Node_listvocodecs *) g_slist_nth_data(list_codec, ++j);
			}
		    }
		    gtk_tree_model_iter_next(GTK_TREE_MODEL(model_codeclist), &iter);
		}
		while (control != sels);
	    }
	    else
	    {
		j = 0;
		pn = (Node_listvocodecs *) g_slist_nth_data(list_codec, j);
		while (pn)
		{
		    codec_sel_list = g_slist_append(codec_sel_list, pn);
		    pn = (Node_listvocodecs *) g_slist_nth_data(list_codec, ++j);
		}
	    }
	    (*gui_connect)->cod_list = codec_sel_list;
	    (*gui_connect)->msg = gtk_editable_get_chars(GTK_EDITABLE(entry_msg), 0, PKT_USERDES_LEN);
	    (*gui_connect)->ssip_port = atoi(gtk_entry_get_text(GTK_ENTRY(entry_port)));

	    (*gui_connect)->rx_port = atoi(gtk_entry_get_text(GTK_ENTRY(entry_rtp_rxport)));
	    (*gui_connect)->ssip = !GTK_TOGGLE_BUTTON(checkbutton_neg_session)->active;

	    if (!(*gui_connect)->ssip)
	    {
		dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_MESSAGE_WARNING, GTK_BUTTONS_OK_CANCEL,
						"Atencion, va a crear una conexion sin negociacion de sesion. Si no ha introducido bien todos los campos, la conexion puede fracasar.");
		gtk_widget_show_all(dialog);
		result = gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		if (result != GTK_RESPONSE_OK)
		{
		    g_free(test1);
		    g_free(test2);
		    gtk_widget_destroy(dialog1);
		    g_free(*gui_connect);
		    *gui_connect = NULL;
		    return FALSE;
		}
		(*gui_connect)->tx_port = atoi(gtk_entry_get_text(GTK_ENTRY(entry_rtp_txport)));
		if ((((*gui_connect)->tx_port % 2) != 0) || ((*gui_connect)->tx_port == 0))
		{
		    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "El puerto TX RTP debe ser un numero par y distinto de 0 (RFC 1889)");
		    gtk_widget_show_all(dialog);
		    gtk_dialog_run(GTK_DIALOG(dialog));
		    gtk_widget_destroy(dialog);
		    g_free(test1);
		    g_free(test2);
		    gtk_widget_destroy(dialog1);
		    g_free(*gui_connect);
		    *gui_connect = NULL;
		    return FALSE;
		}
	    }
            else (*gui_connect)->tx_port = 0;

	    (*gui_connect)->des = GTK_TOGGLE_BUTTON(checkbutton_des)->active;
	    if ((*gui_connect)->des)
	    {
		(*gui_connect)->despass = gtk_editable_get_chars(GTK_EDITABLE(entry_des), 0, 256);
	    }
            else (*gui_connect)->despass = NULL;

	    (*gui_connect)->user = sel_user;
	    gtk_widget_destroy(dialog1);
	    return TRUE;
	}
	else
	{
            g_free(test1);
            g_free(test2);
	    gtk_widget_destroy(dialog1);

	    dialog1 = gtk_message_dialog_new(GTK_WINDOW(main_window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_CLOSE,
					     "Imposible conectar, no se ha indicado el usuario y/o el host");
	    g_signal_connect_swapped(GTK_OBJECT(dialog1), "response", G_CALLBACK(gtk_widget_destroy), dialog1);
	    gtk_widget_show_all(dialog1);
            g_free(*gui_connect);
            *gui_connect = NULL;
	    return FALSE;
	}
    }
    else
    {
	gtk_widget_destroy(dialog1);
	*gui_connect = NULL;
        return FALSE;
    }
}


