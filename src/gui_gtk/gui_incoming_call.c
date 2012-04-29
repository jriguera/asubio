
/*
 *
 * FILE: gui_incoming_call.c
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

#include "gui_incoming_call.h"



static GtkWidget *window;
static SsipPkt *local_pkt;
static Socket_tcp *local_skt;
static ItemUser *local_user;
static NotifyCall *local_notify;
static GMutex *local_mutex_notify;




static void button_nacp_click (GtkWidget *button, gpointer data)
{
    UNUSED(button);
    UNUSED(data);


    g_mutex_lock(local_mutex_notify);
    local_notify->accept = FALSE;
    local_notify->ready = FALSE;
    g_mutex_unlock(local_mutex_notify);

    item_user_free(local_user);

    gtk_widget_destroy(window);
}



static gboolean delete_event (GtkWidget *widget, GdkEvent *event, gpointer data)
{
    UNUSED(widget);
    UNUSED(event);
    UNUSED(data);


    g_mutex_lock(local_mutex_notify);
    local_notify->accept = FALSE;
    local_notify->ready = FALSE;
    g_mutex_unlock(local_mutex_notify);

    item_user_free(local_user);

    return FALSE;
}



static void button_acp_click (GtkWidget *button, gpointer data)
{
    UNUSED(button);
    UNUSED(data);


    g_mutex_lock(local_mutex_notify);
    local_notify->accept = TRUE;
    local_notify->ready = FALSE;
    g_mutex_unlock(local_mutex_notify);

    item_user_free(local_user);

    gtk_widget_destroy(window);
}



static void call_addressbook (GtkWidget *button, ConfigFile *fileconf)
{
    UNUSED(button);
    gchar *archive;


    if (cfgf_read_string(fileconf, PROGRAM_GLOBAL_SECTION, PROGRAM_DIR_DB, &archive))
    {
	create_addressbook_window(archive, NULL);
        g_free(archive);
    }
}



static void show_info (GtkWidget *button, GtkWidget *window)
{
    GtkWidget *view;
    GtkWidget *sw;
    GtkWidget *dialog;
    GtkWidget *frame;
    GtkTextBuffer *buffer;
    GtkTextIter iter;
    gchar *aux;


    UNUSED(button);
    dialog = gtk_dialog_new_with_buttons("Informacion adicional", GTK_WINDOW(window),
					 GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_STOCK_CLOSE, GTK_RESPONSE_NONE,
					 NULL);
    frame = gtk_frame_new(NULL);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), frame, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(frame), 5);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);

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
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "\nAll info of this conection is ...\n\n", -1, "heading", NULL);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(view), 5);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(view), 5);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Local info of user:\n", -1, "bold", NULL);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(view), 20);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Name: ", -1, "blue_foreground", NULL);

    gtk_text_buffer_insert(buffer, &iter, local_user->name, -1);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "User: ", -1, "blue_foreground", NULL);
    gtk_text_buffer_insert(buffer, &iter, local_user->user, -1);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Host: ", -1, "blue_foreground", NULL);
    gtk_text_buffer_insert(buffer, &iter, local_user->host, -1);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Description: ", -1, "blue_foreground", NULL);
    gtk_text_buffer_insert(buffer, &iter, local_user->description, -1);
    gtk_text_buffer_insert(buffer, &iter, "\n\n\n", -1);

    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(view), 5);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "SIP Negotiation info:\n", -1, "bold", NULL);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(view), 20);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Remote user: ", -1, "blue_foreground", NULL);
    gtk_text_buffer_insert(buffer, &iter, local_pkt->from_user, -1);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Remote host: ", -1, "blue_foreground", NULL);
    gtk_text_buffer_insert(buffer, &iter, local_pkt->from_host, -1);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Remote port: ", -1, "blue_foreground", NULL);
    aux = g_strdup_printf("%d", g_ntohs(local_pkt->from_port));
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n\n\n", -1);

    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(view), 5);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Conection info:\n", -1, "bold", NULL);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(view), 20);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Remote host :", -1, "blue_foreground", NULL);
    gtk_text_buffer_insert(buffer, &iter, local_skt->addr, -1);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Remote port :", -1, "blue_foreground", NULL);
    aux = g_strdup_printf("%d", g_ntohs(tcp_txport(local_skt)));
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Local port :", -1, "blue_foreground", NULL);
    aux = g_strdup_printf("%d", tcp_rxport(local_skt));
    gtk_text_buffer_insert(buffer, &iter, aux, -1);
    g_free(aux);
    gtk_text_buffer_insert(buffer, &iter, "\n\n", -1);

    gtk_window_resize(GTK_WINDOW(dialog), 400, 300);
    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}



void gui_incoming_call (ItemUser *user, SsipPkt *pkt, Socket_tcp *skt, ConfigFile *fileconf, GtkWidget *main_window, NotifyCall *notify, GMutex *mutex_notify)
{
    GtkWidget *button_acp;
    GtkWidget *button_noacp;
    GtkWidget *table;
    GtkWidget *frame;
    GtkWidget *hbox_acp_noacp;
    GtkWidget *image_animation;
    GtkWidget *user_image;
    GtkWidget *hbox_adb;
    GtkWidget *button_adb;
    GtkWidget *entry_user_men;
    GtkWidget *button_user_info;
    GtkWidget *label_user_ip;
    GtkWidget *label_user_name;
    GtkWidget *frame_user_image;
    GtkWidget *table_user;
    GtkWidget *entry_user_name;
    GtkWidget *entry_user_ip;
    GtkWidget *label_user_port;
    GtkWidget *entry_user_port;
    gchar *aux;
    GtkWidget *frame_animation;
    GtkWidget *hseparator;
    GtkWidget *dialog_vbox1;
    GtkTooltips *tips_help;
    gint len;
    GError *tmp_error = NULL;


    window = gtk_dialog_new_with_buttons("Llamada ...", GTK_WINDOW(main_window),
					 GTK_DIALOG_DESTROY_WITH_PARENT, NULL);
    gtk_window_set_modal(GTK_WINDOW(window), FALSE);
    gtk_dialog_set_has_separator(GTK_DIALOG(window), FALSE);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_present(GTK_WINDOW(window));
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    gtk_window_set_decorated(GTK_WINDOW(window), TRUE);
    tips_help = gtk_tooltips_new();
    dialog_vbox1 = GTK_DIALOG(window)->vbox;

    table = gtk_table_new(1, 5, FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table), 10);
    gtk_container_add(GTK_CONTAINER(dialog_vbox1), table);

    frame_animation = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame_animation), GTK_SHADOW_IN);
    image_animation = load_image_from_file(PROGRAM_ANILOGO_INCOMING_CALL, &tmp_error);
    if (tmp_error != NULL)
    {
	g_printerr("[GUI_INCOMING_CALL] Error loading program logo: %s\n", tmp_error->message);
	g_clear_error(&tmp_error);
    }
    gtk_container_add(GTK_CONTAINER(frame_animation), image_animation);
    gtk_table_attach_defaults(GTK_TABLE(table), frame_animation, 0, 1, 0, 1);

    frame = gtk_frame_new(NULL);
    gtk_table_attach_defaults(GTK_TABLE(table), frame, 0, 1, 1, 2);

    table_user = gtk_table_new(5, 7, FALSE);
    gtk_container_add(GTK_CONTAINER(frame), table_user);

    frame_user_image = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME (frame_user_image), GTK_SHADOW_OUT);
    gtk_table_attach(GTK_TABLE(table_user), frame_user_image, 0, 1, 0, 4, GTK_FILL, GTK_FILL, 5, 5);

    label_user_name = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label_user_name), "<span foreground='blue' weight='bold'>Usuario :</span>");
    gtk_misc_set_alignment(GTK_MISC(label_user_name), 1, 0.5);
    gtk_table_attach(GTK_TABLE(table_user), label_user_name, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 3);

    entry_user_name = gtk_entry_new();
    gtk_editable_set_editable(GTK_EDITABLE(entry_user_name), FALSE);
    pkt->from_user[PKT_USERNAME_LEN - 1] = 0;
    gtk_entry_set_text(GTK_ENTRY(entry_user_name), pkt->from_user);
    gtk_table_attach(GTK_TABLE(table_user), entry_user_name, 2, 3, 0, 1, GTK_FILL, GTK_FILL, 5, 6);

    label_user_ip = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label_user_ip), "<span foreground='blue' weight='bold'>Host/IP :</span>");
    gtk_misc_set_alignment(GTK_MISC(label_user_ip), 1, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table_user), label_user_ip, 1, 2, 1, 2);

    entry_user_ip = gtk_entry_new();
    gtk_editable_set_editable(GTK_EDITABLE(entry_user_ip), FALSE);
    pkt->from_host[PKT_HOSTNAME_LEN - 1] = 0;
    gtk_entry_set_text(GTK_ENTRY(entry_user_ip), pkt->from_host);
    gtk_table_attach(GTK_TABLE(table_user), entry_user_ip, 2, 3, 1, 2, GTK_FILL, GTK_FILL, 5, 0);

    label_user_port = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label_user_port), "<span foreground='blue' weight='bold'>Port :</span>");
    gtk_misc_set_alignment(GTK_MISC(label_user_port), 1, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table_user), label_user_port, 1, 2, 2, 3);

    entry_user_port = gtk_entry_new();
    gtk_editable_set_editable(GTK_EDITABLE(entry_user_port), FALSE);
    aux = g_strdup_printf("%d", g_ntohs(pkt->from_port));
    gtk_entry_set_text(GTK_ENTRY(entry_user_port), aux);
    gtk_table_attach(GTK_TABLE(table_user), entry_user_port, 2, 3, 2, 3, GTK_FILL, GTK_FILL, 5, 0);
    g_free(aux);

    button_user_info = gtk_button_new_with_mnemonic("Mas informacion");
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), button_user_info, "Informacion adicional acerca de la conexion", NULL);
    g_signal_connect(G_OBJECT(button_user_info), "clicked", G_CALLBACK(show_info), window);
    gtk_table_attach(GTK_TABLE(table_user), button_user_info, 1, 3, 3, 4, GTK_FILL, GTK_FILL, 5, 5);

    if (user->photo != NULL)
    {
	user_image = load_scaled_image_from_file(user->photo, USER_PHOTO_X, USER_PHOTO_Y, GDK_INTERP_BILINEAR, &tmp_error);
	if ((tmp_error != NULL) || (user_image == NULL))
	{
	    if (tmp_error != NULL)
	    {
		g_printerr("[GUI_INCOMING_CALL] Error loading user photo: %s\n", tmp_error->message);
                g_clear_error(&tmp_error);
	    }
            else g_printerr("[GUI_INCOMING_CALL] Error loading user photo.\n");
	    user_image = gtk_image_new_from_stock(GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG);
	}
    }
    else user_image = gtk_image_new_from_stock(GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG);
    gtk_container_add(GTK_CONTAINER(frame_user_image), user_image);

    entry_user_men = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(entry_user_men), PKT_USERDES_LEN);
    gtk_editable_set_editable(GTK_EDITABLE(entry_user_men), FALSE);
    len = pkt->data_len;
    if (len >= PKT_USERDES_LEN) len = PKT_USERDES_LEN - 1;
    pkt->data[len] = '\0';
    gtk_entry_set_text(GTK_ENTRY(entry_user_men), pkt->data);
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), entry_user_men, "Motivo de la conexion", NULL);
    gtk_table_attach(GTK_TABLE(table_user), entry_user_men, 0, 3, 4, 5, GTK_FILL, GTK_FILL, 5, 5);

    hbox_acp_noacp = gtk_hbox_new(TRUE, 0);
    button_acp = button_image_from_stock("_Aceptar", GTK_STOCK_APPLY, GTK_ICON_SIZE_BUTTON);
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), button_acp, "Acepta la conexion", NULL);
    g_signal_connect(G_OBJECT(button_acp), "clicked", G_CALLBACK(button_acp_click), NULL);

    button_noacp = button_image_from_stock("_Rechazar", GTK_STOCK_CANCEL, GTK_ICON_SIZE_BUTTON);
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), button_noacp, "Rechaza la peticion de conexion", NULL);
    g_signal_connect(G_OBJECT(button_noacp), "clicked", G_CALLBACK(button_nacp_click), NULL);

    gtk_box_pack_start(GTK_BOX(hbox_acp_noacp), button_acp, TRUE, TRUE, 7);
    gtk_box_pack_start(GTK_BOX(hbox_acp_noacp), button_noacp, TRUE, TRUE, 7);
    gtk_table_attach_defaults(GTK_TABLE(table), hbox_acp_noacp, 0, 1, 2, 3);

    hseparator = gtk_hseparator_new();
    gtk_table_attach_defaults(GTK_TABLE(table), hseparator, 0, 1, 3, 4);

    hbox_adb = gtk_hbox_new(TRUE, 0);
    gtk_table_attach_defaults(GTK_TABLE(table), hbox_adb, 0, 1, 4, 5);
    button_adb = gtk_button_new_with_mnemonic("A_genda");
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips_help), button_adb, "Gestionar agenda", NULL);
    gtk_box_pack_start(GTK_BOX(hbox_adb), button_adb, FALSE, TRUE, 0);
    g_signal_connect(G_OBJECT(button_adb), "clicked", G_CALLBACK(call_addressbook), fileconf);

    local_pkt = pkt;
    local_skt = skt;

    local_user = (ItemUser *) g_malloc0(sizeof(ItemUser));
    local_user->name = g_strdup(user->name);
    local_user->user = g_strdup(user->user);
    local_user->host = g_strdup(user->host);
    local_user->ignore = user->ignore;
    local_user->description = g_strdup(user->description);
    local_user->photo = g_strdup(user->photo);

    local_notify = notify;
    local_mutex_notify = mutex_notify;

    g_signal_connect(G_OBJECT(window), "delete_event", G_CALLBACK(delete_event), NULL);
    gtk_widget_show_all(window);

    return;
}


