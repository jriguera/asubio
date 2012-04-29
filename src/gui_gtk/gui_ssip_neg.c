
/*
 *
 * FILE: gui_ssip_neg.c
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
#include "gui_ssip_neg.h"



// Functions



gchar *user_photo (gchar *sel_user, gchar *db)
{
    ItemUser *user;
    GError *tmp_error = NULL;
    gchar *photo;
    GtkWidget *dialog;


    if (db == NULL) return g_strdup(PROGRAM_INCOMING_LOST_IMAGE_CALL);

    user = get_user(sel_user, db, &tmp_error);
    if ((tmp_error != NULL) || (user == NULL) || (user->photo == NULL))
    {
	if (tmp_error != NULL)
	{
	    g_printerr("[GUI_SSIP_NEG] user_photo: Error searching user: %s.\n", tmp_error->message);
	    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "No se ha encontrado el usuario por defecto, el fichero de agenda esta corrupto : %s", tmp_error->message);
	    g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	    gtk_widget_show_all(dialog);
	    g_clear_error(&tmp_error);
	}
	else
	{
	    g_printerr("[GUI_SSIP_NEG] ssip_process_loop: Error searching user on contact db.\n");
	    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "No se ha encontrado el usuario por defecto, el fichero de agenda esta corrupto");
	    g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	    gtk_widget_show_all(dialog);
	}
	photo = g_strdup(PROGRAM_INCOMING_LOST_IMAGE_CALL);
    }
    else
    {
	photo = g_strdup(user->photo);
	item_user_free(user);
    }
    if ((!g_path_is_absolute(photo)) && (!g_file_test(photo, G_FILE_TEST_EXISTS)))
    {
        g_free(photo);
	photo = g_strdup(PROGRAM_INCOMING_LOST_IMAGE_CALL);
    }
    return photo;
}



void user_filter ()
{
    gboolean user_accepted;
    gchar *file_addressbook;
    GError *tmp_error = NULL;
    gchar *command;
    ItemUser *user = NULL;
    gboolean has_command;
    gboolean value;
    gint exec_ignored = 0;
    GtkWidget *dialog;


    user_accepted = FALSE;
    if (cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_DIR_DB, &file_addressbook))
    {
	user = get_user(notify_call.from_user, file_addressbook, &tmp_error);
	if (tmp_error != NULL)
	{
	    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "Error leyendo %s '%s' (provocado por atender a una conexion)", file_addressbook, tmp_error->message);
	    g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	    gtk_widget_show_all(dialog);
	    g_error_free(tmp_error);

	    g_free(file_addressbook);

	    notify_call.accept = FALSE;
	    notify_call.ready = FALSE;
	    return;
	}
	else
	{
	    if (user != NULL)
	    {
		has_command = cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_EXEC_ONCALL, &command);
		value = cfgf_read_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_EXEC_ONCALL_IGNORED, &exec_ignored);
		if (value && (exec_ignored == 1) && has_command)
		{
		    execute_command(command);
                    value = TRUE;
		}
		else value = FALSE;

		user_accepted = TRUE;
		if (user->ignore)
		{
		    if (value && (exec_ignored == 1) && has_command)
		    {
			dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "Usuario '%s@%s' llamo pero fue ignorado", notify_call.from_user, notify_call.pkt_r->from_host);
			g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
			gtk_widget_show_all(dialog);
		    }
		    user_accepted = FALSE;
		}
		else
		{
		    if (has_command && (!value)) execute_command(command);
		    gui_incoming_call(user, notify_call.pkt_r, notify_call.skt, configuration, window, &notify_call, mutex_notify_call);
		}
		if (has_command) g_free(command);
		item_user_free(user);
		if (!user_accepted)
		{
		    notify_call.accept = FALSE;
		    notify_call.ready = FALSE;
		}
	    }
	    else
	    {
		dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "Usuario no encontrado (%s@%s), conexion rechazada", notify_call.from_user, notify_call.pkt_r->from_host);
		g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
		gtk_widget_show_all(dialog);
		g_printerr("[GUI_SSIP_NEG] user_filter: Error: user not found (%s@%s)\n", notify_call.pkt_r->from_user, notify_call.pkt_r->from_host);
		notify_call.accept = FALSE;
		notify_call.ready = FALSE;
	    }
	    g_free(file_addressbook);
	}
    }
    else
    {
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "Imposible acceder al fichero agenda de contactos (provocado por atender a una conexion)");
	g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_widget_show_all(dialog);

	notify_call.accept = FALSE;
	notify_call.ready = FALSE;
    }
}



gboolean exit_ssip ()
{
    gint gv;
    gboolean b;
    gboolean has_ssip;
    gchar *pas;
    gchar *user;
    gchar *ssiphost;
    GError *tmp_error = NULL;
    GtkWidget *dialog;


    ssip_process_loop_destroy();

    if (cfgf_read_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_PROXY, &gv) && (gv == 1))
    {
	b = cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_PASS, &pas);
	if (!b)
	{
	    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
					    GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
					    "El fichero de configuracion esta incompleto y es imposible notificar su baja al proxy (se ha perdido el password), debe contactar con el administrador del proxy");
	    g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	    gtk_widget_show_all(dialog);
            return FALSE;
	}
	else
	{
	    b = TRUE;
	    b &= cfgf_read_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_LOCAL_HOST_PORT, &gv);
	    b &= has_ssip  = cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_LOCAL_HOST, &ssiphost);
	    if (!b)
	    {
		dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
						"La notificacion de estado al SSIP-proxy no esta configurada correctamente");
		g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
		gtk_widget_show_all(dialog);
		g_free(pas);
		if (has_ssip) g_free(ssiphost);
		return FALSE;
	    }
	    user = g_strdup(g_get_user_name());
	    gv =  notify_offline_to(user, pas, "notify", ssiphost, gv, &tmp_error);
	    g_free(user);
	    g_free(pas);
	    switch (gv)
	    {
	    case 1:
		g_free(ssiphost);
		break;

	    case 0:
		dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_MESSAGE_WARNING, GTK_BUTTONS_CLOSE,
						"Imposible notificar su baja al proxy, tal vez no haya iniciado su session en el proxy %s", ssiphost);
		g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
		gtk_widget_show_all(dialog);
		g_free(ssiphost);
		return FALSE;
		break;

	    case 2:
		dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
						"Imposible notificar su baja al proxy, error estableciendo la comunicacion. Contacte con el administrador del SSIP-proxy %s", ssiphost);
		g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
		gtk_widget_show_all(dialog);
		g_free(ssiphost);
		return FALSE;
		break;

	    case -1:
		dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
						"Error contactando con el SSIP-proxy %s: %s", ssiphost, tmp_error->message);
		g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
		gtk_widget_show_all(dialog);
		g_error_free(tmp_error);
		g_free(ssiphost);
		return FALSE;
		break;
	    }
            return TRUE;
	}
    }
    return TRUE;
}



gboolean init_ssip_proxy_params (gchar *proxy_host, gint proxy_port, gint ssip_local_port)
{
    gint gv;
    gchar *pas;
    gchar *user;
    GError *tmp_error = NULL;
    GtkWidget *dialog;
    gboolean b;


    b = cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_PASS, &pas);
    if (!b)
    {
	pas = g_strdup_printf("%d", g_random_int_range(-65000, 65000));
	cfgf_write_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_PASS, pas);
    }
    user = g_strdup(g_get_user_name());
    gv = notify_online_to(user, pas, ssip_local_port, "notify", proxy_host, proxy_port, &tmp_error);
    g_free(user);
    g_free(pas);
    switch (gv)
    {
    case 1:
        return TRUE;
	break;

    case 0:
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_WARNING, GTK_BUTTONS_CLOSE,
					"Imposible notificar su alta al proxy, tal vez no haya notificado su salida. Vuelva a offline manualmente y luego a online o contacte con el administrador del SSIP-proxy %s", proxy_host);
	g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_widget_show_all(dialog);
	return FALSE;
	break;

    case 2:
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
					"Imposible notificar su alta al proxy, error estableciendo la comunicacion. Contacte con el administrador del SSIP-proxy %s", proxy_host);
	g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_widget_show_all(dialog);
	return FALSE;
	break;

    case -1:
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
					"Error contactando con el SSIP-proxy %s : %s", proxy_host, tmp_error->message);
	g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_widget_show_all(dialog);
	g_error_free(tmp_error);
	return FALSE;
	break;
    }
    return TRUE;
}



gboolean init_ssip ()
{
    gint gv;
    gboolean b;
    gboolean has_ssip;
    gchar *host;
    gint ssipport;
    GtkWidget *dialog;


    ssip_process_loop_destroy();
    usleep(200000);
    if (cfgf_read_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_PROXY, &gv) && (gv == 1))
    {
	b = TRUE;
	b &= has_ssip = cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_LOCAL_HOST, &host);
	b &= cfgf_read_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_LOCAL_HOST_PORT, &gv);
	b &= cfgf_read_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_LOCALHOST_PORT, &ssipport);
	if (!b)
	{
	    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
					    GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
					    "La notificacion de estado al SSIP-proxy no esta configurada correctamente");
	    g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	    gtk_widget_show_all(dialog);
            if (has_ssip) g_free(host);
            return FALSE;
	}
	b = init_ssip_proxy_params(host, gv, ssipport);
	g_free(host);
        return b;
    }
    ssip_process_loop_create();
    return TRUE;
}



static gpointer ssip_process_loop (gpointer data)
{
    Socket_tcp *skt;
    GError *tmp_error = NULL;
    gint fd_skt;
    gchar *db;
    InitRTPsession *ip;
    gchar *from_user;
    gint localssipport;
    GtkWidget *dialog;


    UNUSED(data);
    if (!cfgf_read_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_LOCALHOST_PORT, &localssipport))
    {
	g_printerr("[GUI_SSIP_NEG] ssip_process_loop_create: Error no SSIP port defined for this host.\n");
	gdk_threads_enter();
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
					"No se ha definido un puerto (SSIP) donde atender las negociaciones");
	g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), dialog);
	gtk_widget_show_all(dialog);
	gdk_threads_leave();
        return NULL;
    }
    skt = tcp_init(PROGRAM_LOCALHOST, NULL, localssipport, 0, PROGRAM_RTP_TCP_TTL, &tmp_error);
    if (tmp_error != NULL)
    {
	g_printerr("[GUI_SSIP_NEG] ssip_process_loop: Unable to init SSIP session: %s\n", tmp_error->message);
	gdk_threads_enter();
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
					"Error iniciando SSIP session %s", tmp_error->message);
	g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), dialog);
	gtk_widget_show_all(dialog);
	gdk_threads_leave();
	g_clear_error(&tmp_error);
        return NULL;
    }
    fd_skt = tcp_fd(skt);
    if (listen(fd_skt, 1) == -1)
    {
	g_printerr("[GUI_SSIP_NEG] ssip_process_loop: Unable to set listen tcp option.\n");
	tcp_exit(skt, NULL);
        return NULL;
    }
    from_user = g_strdup(g_get_user_name());

    while (go_ssip_process)
    {
	Socket_tcp *sacpt;
	gint ret;
        gchar *photo;
	gchar *rtp_sel_user;
	gchar *rtp_sel_host;
	guint16 rtp_sel_port;
        Node_listvocodecs *sel_codec;
	fd_set rfds;
	struct timeval tv;
	gint valret;
	gint ssip_proxy;
	gchar *ssip_remote_ip;
	gchar *from_real_host;
	GError *tmp_error2 = NULL;
	gint ssipport;
	gchar *username;
	gchar *usermail;
	gchar *usernote;


	FD_ZERO(&rfds);
	FD_SET(fd_skt, &rfds);
	tv.tv_sec = 0;
	tv.tv_usec = SSIP_NEG_TIME_SOCKET_BLOCK;

	valret = select(fd_skt + 1, &rfds, NULL, NULL, &tv);
	if (!valret) continue;

	sacpt = tcp_accept(skt, &tmp_error);
	if (tmp_error != NULL)
	{
	    g_printerr("[GUI_SSIP_NEG] ssip_process_loop: Error on accept: %s.\n", tmp_error->message);
	    g_clear_error(&tmp_error);
            sleep(2);
	    continue;
	}
	valret = cfgf_read_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_PROXY, &ssip_proxy);
	if (valret && ssip_proxy)
	{
	    if (!cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_REMOTE_HOST, &ssip_remote_ip))
	    {
		g_printerr("[GUI_SSIP_NEG] ssip_process_loop: SSIP Proxy defined, but no hostname or ip specified.\n");
		sleep(2);
                continue;
	    }
	    else
	    {
		if (!cfgf_read_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_REMOTE_HOST_PORT, &ssipport))
		{
		    g_printerr("[GUI_SSIP_NEG] ssip_process_loop: Proxy defined, but no port of SSIP proxy.\n");
                    g_free(ssip_remote_ip);
		    sleep(2);
		    continue;
		}
	    }
	}
	else
	{
	    if (!cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_HOSTNAME, &from_real_host))
	    {
		g_printerr("[GUI_SSIP_NEG] ssip_process_loop: No hostname or ip defined for this host.\n");
		sleep(2);
		continue;
	    }
	    else
	    {
		ssip_remote_ip = from_real_host;
                ssipport = localssipport;
	    }
	}

	rtp_base_port += (num_users_active_dialogs * 2);

	ret = ssip_accept(sacpt, from_user, ssip_remote_ip, ssipport,
			  ssip_remote_ip, rtp_base_port,
			  list_codec_plugins, list_codec_mutex, &notify_call, mutex_notify_call, &tmp_error2,
			  &rtp_sel_user, &rtp_sel_host, &rtp_sel_port, &sel_codec);
	tcp_exit(sacpt, &tmp_error);
	if (tmp_error != NULL)
	{
	    g_printerr("[GUI_SSIP_NEG] ssip_process_loop: Error close socket: %s.\n", tmp_error->message);
	    g_clear_error(&tmp_error);
	}
	switch (ret)
	{
	case -2:
	    sleep(1);
	    break;

	case -1:
            gdk_threads_enter();
	    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "Error procesando la llamada: %s", tmp_error2->message);
	    g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	    gtk_widget_show_all(dialog);
	    gdk_threads_leave();
	    g_clear_error(&tmp_error2);
            sleep(1);
	    break;

	case 0:
            sleep(1);
	    break;

	case 1:
	    break;

	case 2:

	    if (!cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_NAME_USER, &username))
	    {
		username = g_strdup(g_get_real_name());
	    }
	    if (!cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_MAIL_USER, &usermail))
	    {
		usermail = g_strdup_printf("%s@%s", g_get_real_name(), ssip_remote_ip);
	    }
	    if (!cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_DESCRIPTION_USER, &usernote))
	    {
		usernote = NULL;
	    }
	    if (!cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_DIR_DB , &db))
	    {
		g_printerr("[GUI_SSIP_NEG] ssip_process_loop: No contact file defined!\n");
		db = NULL;
	    }
	    gdk_threads_enter();
	    photo = user_photo(rtp_sel_user, db);
	    gdk_threads_leave();
	    ip = g_malloc0(sizeof(InitRTPsession));
	    ip->username = username;
	    ip->ttl = PROGRAM_RTP_TCP_TTL;
	    ip->rtcp_bw = PROGRAM_RTP_MAX_BW;
	    ip->toolname = g_strdup(PROGRAM_RTP_TOOLNAME);
	    ip->usermail = usermail;
	    ip->userlocate = NULL;
	    ip->usernote = usernote;
	    ip->addr = rtp_sel_host;
	    ip->rx_port = rtp_base_port;
	    ip->tx_port = rtp_sel_port;

	    g_mutex_lock(mutex_session_ready);
	    g_mutex_lock(session_ready.mutex);  //lo desbloquea el bucle gtk_main()
	    session_ready.user = rtp_sel_user;
	    session_ready.host = g_strdup(rtp_sel_host);
	    session_ready.image_user = photo;
	    session_ready.init_param = ip;
	    session_ready.selected_rx_payload = sel_codec->payload;
	    session_ready.selected_tx_payload = sel_codec->payload;
	    session_ready.des = FALSE;
	    session_ready.ready = TRUE;
	    g_mutex_unlock(mutex_session_ready);

	    g_free(db);
	    break;
	}
	g_free(ssip_remote_ip);
    }
    g_free(from_user);
    tcp_exit(skt, NULL);
    usleep(100000);
    return NULL;
}



static gpointer ssip_connect_loop (gpointer data)
{
    gboolean value;
    gint ssip_proxy;
    gint gi;
    InitRTPsession *ip;
    gint connect_exit = 1;
    gchar *ssip_remote_ip;
    GtkWidget *dialog;
    gint ssipport;
    gchar *photo;
    gboolean has_db;
    gchar *db;
    gchar *from_real_host;
    gchar *from_user;
    gchar *rtp_sel_host;
    guint16 rtp_sel_port;
    Node_listvocodecs *sel_codec;
    GError *tmp_error = NULL;
    GuiConnect *guiconnect = (GuiConnect *) data;
    ProgressData *pd;
    gchar *username = NULL;
    gchar *usermail = NULL;
    gchar *usernote = NULL;



    has_db = cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_DIR_DB , &db);
    value = cfgf_read_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_PROXY, &ssip_proxy);
    if (value && ssip_proxy)
    {
	if (!cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_REMOTE_HOST, &ssip_remote_ip))
	{
            gdk_threads_enter();
	    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "No ha sido definido una maquina que actue de proxy SSIP");
	    g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	    gtk_widget_show_all(dialog);
            gdk_threads_leave();
	    return NULL;
	}
	if (!cfgf_read_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_REMOTE_HOST_PORT, &ssipport))
	{
            gdk_threads_enter();
	    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "No ha sido definido un puerto de negociacion de conexiones del proxy SSIP");
	    g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	    gtk_widget_show_all(dialog);
            gdk_threads_leave();
	    return NULL;
	}
    }
    else
    {
	if (!cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_HOSTNAME, &from_real_host))
	{
            gdk_threads_enter();
	    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "No ha sido definido un nombre o una IP asociada a este host");
	    g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	    gtk_widget_show_all(dialog);
            gdk_threads_leave();
	    return NULL;
	}
	ssip_remote_ip = g_strdup(from_real_host);
	g_free(from_real_host);
	if (!cfgf_read_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_LOCALHOST_PORT, &ssipport))
	{
            gdk_threads_enter();
	    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "No ha sido definido un puerto SSIP local donde negociar las peticiones de conexion");
	    g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	    gtk_widget_show_all(dialog);
            gdk_threads_leave();
	    return NULL;
	}
    }
    from_user = g_strdup(g_get_user_name());

    gdk_threads_enter();
    pd = gui_connect_progresbar(window, &connect_exit, from_user, ssip_remote_ip, guiconnect->username, guiconnect->host);
    gtk_label_set_markup(GTK_LABEL(pd->label), "<span color=\"blue\"><big><b>Aguardando por aceptacion de la llamada ...</b></big></span>");
    gdk_threads_leave();

    if (guiconnect->ssip_port == 0) guiconnect->ssip_port = SSIP_MANAGE_DEFAULT_PORT;

    // WARNING, OJO -> NO mutex
    if (guiconnect->rx_port != 0)
    {
	if ((guiconnect->rx_port == rtp_base_port) || (guiconnect->rx_port == (rtp_base_port+1)))
	{
	    gdk_threads_enter();
	    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "El puerto RTP seleccionado esta ocupado por otra session");
	    g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	    gtk_widget_show_all(dialog);
	    gdk_threads_leave();
	    g_free(from_user);
	    return NULL;
	}
    }
    else guiconnect->rx_port = rtp_base_port += (num_users_active_dialogs * 2);

    gi = ssip_connect(from_user, ssip_remote_ip, ssipport,
		      guiconnect->username, guiconnect->host, guiconnect->ssip_port, guiconnect->msg,
		      guiconnect->cod_list, list_codec_mutex, guiconnect->rx_port, configuration, pd, &tmp_error,
		      &rtp_sel_host, &rtp_sel_port, &sel_codec);
    gdk_threads_enter();
    gtk_widget_destroy(pd->dialog);
    gdk_threads_leave();

    switch(gi)
    {
    case -3:
        // interrup conection
	break;

    case -2:
        g_printerr("[GUI_SSIP_NEG] Sorry, dangerous error ...\n");
	break;

    case -1:
	gdk_threads_enter();
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Error negociando la conexion: %s", tmp_error->message);
	g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_widget_show_all(dialog);
	gdk_threads_leave();
        g_error_free(tmp_error);
	break;

    case 0:
	gdk_threads_enter();
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "%s no existe en %s", guiconnect->username, guiconnect->host);
	g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_widget_show_all(dialog);
	gdk_threads_leave();
	break;

    case 1:
	gdk_threads_enter();
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "El usuario %s ignora la peticion de conexion ...", guiconnect->username);
	g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_widget_show_all(dialog);
	gdk_threads_leave();
	break;

    case 2:
	gdk_threads_enter();
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
					"Ha sido imposible establecer la comunicacion, ya que %s no tiene los codecs de audio seleccionados. Reintentelo seleccionando otros codecs ... ", guiconnect->username);
	g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_widget_show_all(dialog);
	gdk_threads_leave();
	break;

    case 3:
	gdk_threads_enter();
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "Error negociando los parametros RTP/RTCP, tal vez los puertos RTP esten ocupados");
	g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_widget_show_all(dialog);
	gdk_threads_leave();
	break;

    case 4:

	if (guiconnect->user == NULL)
	{
	    gdk_threads_enter();
	    photo = user_photo(guiconnect->username, db);
	    gdk_threads_leave();
	}
        else photo = g_strdup(guiconnect->user->photo);

	if (!cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_NAME_USER, &username))
	{
	    username = g_strdup(g_get_real_name());
	}
	if (!cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_MAIL_USER, &usermail))
	{
	    usermail = g_strdup_printf("%s@%s", g_get_real_name(), ssip_remote_ip);
	}
	if (!cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_DESCRIPTION_USER, &usernote))
	{
	    usernote = NULL;
	}
        ip = g_malloc0(sizeof(InitRTPsession));
	ip->username = username;
	ip->ttl = PROGRAM_RTP_TCP_TTL;
	ip->rtcp_bw = PROGRAM_RTP_MAX_BW;
	ip->toolname = g_strdup(PROGRAM_RTP_TOOLNAME);
	ip->usermail = usermail;
	ip->userlocate = NULL;
	ip->usernote = usernote;
	ip->addr = rtp_sel_host;
	ip->rx_port = rtp_base_port;
	ip->tx_port = rtp_sel_port;

	g_mutex_lock(mutex_session_ready);
        g_mutex_lock(session_ready.mutex); //lo desbloquea el bucle gtk_main()
	session_ready.user = g_strdup(guiconnect->username);
	session_ready.host = g_strdup(rtp_sel_host);
	session_ready.image_user = photo;
	session_ready.init_param = ip;
	session_ready.selected_rx_payload = sel_codec->payload;
	session_ready.selected_tx_payload = sel_codec->payload;
	session_ready.des = FALSE;
	session_ready.ready = TRUE;
	g_mutex_unlock(mutex_session_ready);
	break;
    }
    if (has_db) g_free(db);
    g_free(from_user);
    g_free(guiconnect->username);
    g_free(guiconnect->host);
    g_free(guiconnect->msg);
    g_free(guiconnect->despass);
    if (guiconnect->user != NULL)
    {
	g_free(guiconnect->user->name);
	g_free(guiconnect->user->description);
	g_free(guiconnect->user->photo);
	g_free(guiconnect->user->host);
	g_free(guiconnect->user->user);
    }
    g_free(guiconnect);
    return NULL;
}



// Public Functions




gboolean ssip_process_loop_create ()
{
    gint port;
    GtkWidget *dialog;
    gchar *charvalue;
    gchar *sip_proxy;
    gint intvalue;
    gint ssipport;
    gboolean value;
    gint ssip_proxy;
    GError *tmp_error = NULL;


    if (!cfgf_read_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_RTP_BASE_PORT, &port))
    {
	g_printerr("[GUI_SSIP_NEG] ssip_process_loop_create: Error getting base rtp port.\n");
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
					"El puerto RTP/RTCP base no esta definido, necesitara configurarlo para continuar");
	g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_widget_show_all(dialog);
        return FALSE;
    }
    if (!cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_HOSTNAME, &charvalue))
    {
	g_printerr("[GUI_SSIP_NEG] ssip_process_loop_create: Error no IP or hostname for this host.\n");
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
					"No se ha definido una IP para este host, imposible negociar conexiones con el protocolo SSIP");
	g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_widget_show_all(dialog);
        return FALSE;
    }
    g_free(charvalue);

    if (!cfgf_read_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_LOCALHOST_PORT, &ssipport))
    {
	g_printerr("[GUI_SSIP_NEG] ssip_process_loop_create: Error no SSIP port defined for this host.\n");
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
					"No se ha definido un puerto (SSIP) donde atender las negociaciones");
	g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_widget_show_all(dialog);
        return FALSE;
    }

    value = cfgf_read_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_PROXY, &ssip_proxy);
    if (value && ssip_proxy)
    {
	if (!cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_REMOTE_HOST, &sip_proxy))
	{
	    g_printerr("[GUI_SSIP_NEG] ssip_process_loop: SSIP Proxy defined, but no hostname or ip specified.\n");
	    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
					    GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
					    "No se ha definido una IP para conectar con el proxy SSIP");
	    g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	    gtk_widget_show_all(dialog);
	    return FALSE;
	}
	g_free(sip_proxy);
	if (!cfgf_read_int(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_SSIP_REMOTE_HOST_PORT, &intvalue))
	{
	    g_printerr("[GUI_SSIP_NEG] ssip_process_loop: Proxy defined, but no port of SSIP proxy.\n");
	    dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
					    GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
					    "No se ha definido un puerto para conectar con el proxy SSIP");
	    g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	    gtk_widget_show_all(dialog);
	    return FALSE;
	}
    }

    rtp_base_port = port;
    go_ssip_process = TRUE;

    thread_ssip_loop = g_thread_create((GThreadFunc) ssip_process_loop, NULL, TRUE, &tmp_error);
    if (tmp_error != NULL)
    {
	g_printerr("Error creando thread de negociacion SSIP: %s\n", tmp_error->message);
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
					"Imposible crear thread de negociacion SSIP: %s", tmp_error->message);
	g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_widget_show_all(dialog);
	g_error_free(tmp_error);
	return FALSE;
    }
    return TRUE;
}



void ssip_process_loop_destroy ()
{
    go_ssip_process = FALSE;
    if (thread_ssip_loop != NULL)
    {
	g_thread_join(thread_ssip_loop);
	thread_ssip_loop = NULL;
    }
    return;
}



void ssip_connect_to ()
{
    InitRTPsession *ip;
    GuiConnect *guiconnect = NULL;
    GError *tmp_error = NULL;
    GThread *thread_connect_loop;
    GtkWidget *dialog;
    Node_listvocodecs *selc;
    gint sel_payload;
    gchar *db;
    gchar *photo;
    gchar *username;
    gchar *usermail;
    gchar *usernote;
    gchar *from_real_host;


    if (!cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_DIR_DB , &db))
    {
	g_printerr("[GUI_SSIP_NEG] ssip_connect_to: No contact file defined!\n");
	dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Imposible iniciar la session con un fichero agenda de contactos corrupto");
	g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_widget_show_all(dialog);
        return;
    }
    if (create_connect_dialog(configuration, window, list_codec_plugins, &guiconnect))
    {
	if (guiconnect == NULL)
	{
	    g_free(db);
	    return;
	}
	if (guiconnect->ssip)
	{
	    thread_connect_loop = g_thread_create((GThreadFunc) ssip_connect_loop, guiconnect, FALSE, &tmp_error);
	    if (tmp_error != NULL)
	    {
		g_printerr("Error creando thread : %s\n", tmp_error->message);
		dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
						"Imposible crear thread de conexion: %s", tmp_error->message);
		g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
		gtk_widget_show_all(dialog);
		g_error_free(tmp_error);
	    }
	    g_free(db);
	}
	else
	{
	    if ((guiconnect->rx_port == rtp_base_port) ||
		(guiconnect->rx_port == (rtp_base_port+1)))
	    {
		dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "El puerto RX RTP seleccionado esta ocupado");
		g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
		gtk_widget_show_all(dialog);
		g_free(guiconnect->username);
		g_free(guiconnect->host);
		g_free(guiconnect->msg);
		g_free(guiconnect->despass);
		if (guiconnect->user != NULL)
		{
		    g_free(guiconnect->user->name);
		    g_free(guiconnect->user->description);
		    g_free(guiconnect->user->photo);
		    g_free(guiconnect->user->host);
		    g_free(guiconnect->user->user);
		}
		g_free(guiconnect);
		g_free(db);
		return;
	    }
	    selc = g_slist_nth_data(guiconnect->cod_list, 0);
	    if (selc == NULL) sel_payload = IO_PCM_PAYLOAD;
	    else sel_payload = selc->payload;

	    photo = user_photo(guiconnect->username, db);
	    g_free(db);

	    if (!cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_NAME_USER, &username))
	    {
		username = g_strdup(g_get_real_name());
	    }
	    if (!cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_MAIL_USER, &usermail))
	    {
		if (!cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_HOSTNAME, &from_real_host))
		{
		    usermail = g_strdup_printf("%s@%s", g_get_real_name(), from_real_host);
		}
                else usermail = g_strdup_printf("%s@hostname.net", g_get_real_name());
	    }
	    if (!cfgf_read_string(configuration, PROGRAM_GLOBAL_SECTION, PROGRAM_DESCRIPTION_USER, &usernote))
	    {
		usernote = NULL;
	    }
	    ip = g_malloc0(sizeof(InitRTPsession));
	    ip->username = username;
	    ip->ttl = PROGRAM_RTP_TCP_TTL;
	    ip->rtcp_bw = PROGRAM_RTP_MAX_BW;
	    ip->toolname = g_strdup(PROGRAM_RTP_TOOLNAME);
	    ip->usermail = usermail;
	    ip->userlocate = NULL;
	    ip->usernote = usernote;
	    ip->addr = g_strdup(guiconnect->host);
	    ip->rx_port = guiconnect->rx_port;
	    ip->tx_port = guiconnect->tx_port;

	    g_mutex_lock(mutex_session_ready);
	    g_mutex_lock(session_ready.mutex); //lo desbloquea el bucle gtk_main()
	    session_ready.user = g_strdup(guiconnect->username);
	    session_ready.host = g_strdup(guiconnect->host);
	    session_ready.image_user = photo;
	    session_ready.init_param = ip;
	    session_ready.selected_rx_payload = sel_payload;
	    session_ready.selected_tx_payload = sel_payload;
	    session_ready.des = guiconnect->des;
	    session_ready.despass = g_strdup(guiconnect->despass);
	    session_ready.ready = TRUE;
	    g_mutex_unlock(mutex_session_ready);

	    g_free(guiconnect->username);
	    g_free(guiconnect->host);
	    g_free(guiconnect->msg);
	    g_free(guiconnect->despass);
	    if (guiconnect->user != NULL)
	    {
		g_free(guiconnect->user->name);
		g_free(guiconnect->user->description);
		g_free(guiconnect->user->photo);
		g_free(guiconnect->user->host);
		g_free(guiconnect->user->user);
	    }
	    g_free(guiconnect);
	}
    }
}


