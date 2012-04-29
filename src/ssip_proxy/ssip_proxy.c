
/*
 *
 * FILE: ssip_proxy.h
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

#include "ssip_proxy.h"



static gchar *db_file = NULL;


static gchar *username_notify = NULL;
static gchar *hostip_notify = NULL;
static gboolean go_notify_process = TRUE;
static guint16 port_notify = 0;


static gchar *username_manager = NULL;
static gchar *hostip_manager = NULL;
static guint16 port_manager = 0;
static gint num_forks_manager = 0;
static gboolean go_manage_process = TRUE;

static gint childcount = 0;



// Functions



void handler_signal_manage (int sig)
{
    g_log(PROCESS_NOTIFY, G_LOG_LEVEL_INFO, "Signal received (%d)", sig);


    switch (sig)
    {
    case SIGCHLD:
	wait(0);
	childcount--;
	break;

    case SIGUSR1:
	go_manage_process = FALSE;
        go_notify_process = FALSE;
	while (childcount > 0) wait(0);
	break;

    default:
        break;
    }
}



void file_block (gint fd, gint mode)
{
    struct flock lk;


    lk.l_type = mode;
    lk.l_whence = SEEK_SET;
    lk.l_start = 0;
    lk.l_len = 0;

    lseek(fd, 0L, SEEK_SET);

    if (fcntl(fd, F_SETLKW, &lk))
    {
	g_log(PROCESS_NOTIFY, G_LOG_LEVEL_CRITICAL, "Unable to set file block: %s", strerror(errno));
    }
}



void protocol_manager (Socket_tcp *sacpt, gchar *filename)
{
    SsipPkt *ss_rx;
    SsipPkt *ss_tx;
    SsipPkt *ss_ttx;
    gint pid = getpid();


    ss_rx = (SsipPkt *) g_malloc0(sizeof(SsipPkt));
    ss_tx = (SsipPkt *) g_malloc0(sizeof(SsipPkt));
    ss_ttx = (SsipPkt *) g_malloc0(sizeof(SsipPkt));

    if (tcp_recv(sacpt, ss_rx, sizeof(SsipPkt), 0) == sizeof(SsipPkt))
    {
	gchar *user;
	gchar *host;
	gint32 type;
        gboolean value_host = TRUE;
	gboolean value = TRUE;
	gchar *tohost;
	gchar *host_fromcall;
	gchar *user_fromcall;
        gint port_fromcall;
	gint toport;
	gint32 tipo;
	ConfigFile *file;
	gchar *fileblock;
	gint fd_block;
	GError *tmp_error = NULL;
	Socket_tcp *s_mag = NULL;
	gint len;
        gboolean next_bye;


	type = g_ntohl(ss_rx->type);
	switch (type)
	{
	case CONNECT_HELO:
	    break;

	case CONNECT_BYE:

	    g_log(PROCESS_MANAGE, G_LOG_LEVEL_INFO, "FORK -- (pid=%d) -- Recv BYE (user=%s, host=%s, port=%d), responding ...", pid, ss_rx->from_user, ss_rx->from_host, ss_rx->from_port);

	    ss_rx->type = g_htonl(CONNECT_BYE);
	    g_strlcpy(ss_rx->to_user, ss_rx->from_user, PKT_USERNAME_LEN);
	    g_strlcpy(ss_rx->to_host, ss_rx->from_host, PKT_HOSTNAME_LEN);
	    ss_rx->to_port = ss_rx->from_port;

            len = strlen(username_manager) + 1;
	    if (PKT_USERNAME_LEN <= len) len = PKT_USERNAME_LEN;
	    g_strlcpy(ss_rx->from_user, username_manager, len);
            len = strlen(hostip_manager) + 1;
	    if (PKT_HOSTNAME_LEN <= len) len = PKT_HOSTNAME_LEN;
	    g_strlcpy(ss_rx->from_host, hostip_manager, len);
	    ss_rx->from_port = g_htons(tcp_rxport(sacpt));

	    if (tcp_send(sacpt, ss_rx, sizeof(SsipPkt), 0) != sizeof(SsipPkt))
	    {
		g_log(PROCESS_MANAGE, G_LOG_LEVEL_CRITICAL, "FORK -- (pid=%d) -- Unable send packet to %s (user %s)", pid, ss_rx->to_host ,ss_rx->to_user);
	    }
	    goto __FINAL;
	    break;

	case CONNECT_ERROR:

	    len = g_ntohl(ss_rx->data_len);
	    if (PKT_USERNAME_LEN <= len) len = PKT_USERNAME_LEN - 1;
	    ss_rx->data[len] = 0;
	    g_log(PROCESS_MANAGE, G_LOG_LEVEL_WARNING, "FORK -- (pid=%d) -- Recv ERROR_PACKET: %s", pid, ss_rx->data);
	    goto __FINAL;
	    break;

	default:

	    g_log(PROCESS_MANAGE, G_LOG_LEVEL_WARNING, "FORK -- (pid=%d) -- Recv UNKNOWN PACKET", pid);

            goto __FINAL;
	}

	host_fromcall = g_strndup(ss_rx->from_host, PKT_HOSTNAME_LEN);
	user_fromcall = g_strndup(ss_rx->from_user, PKT_USERNAME_LEN);
        port_fromcall = ss_rx->from_port;
	user = g_strndup(ss_rx->to_user, PKT_USERNAME_LEN);
	host = g_strndup(ss_rx->to_host, PKT_HOSTNAME_LEN);

	fileblock = g_strdup_printf("%s.lck", filename);
	fd_block = open(fileblock, O_RDWR | O_CREAT, 00700);
	file_block(fd_block, F_RDLCK);

	file = cfgf_open_file(filename, &tmp_error);

	file_block(fd_block, F_UNLCK);
	g_free(fileblock);
	close(fd_block);

	if (tmp_error != NULL)
	{
	    g_log(PROCESS_MANAGE, G_LOG_LEVEL_CRITICAL, "FORK -- (pid=%d) -- %s", pid, tmp_error->message);
            g_clear_error(&tmp_error);
	    cfgf_free(file);
            goto __FIN;
	}
        value = TRUE;
	value &= value_host = cfgf_read_string(file, user, "host", &tohost);
	value &= cfgf_read_int(file, user, "port", &toport);

	cfgf_free(file);

	if (!value)
	{
	    g_log(PROCESS_MANAGE, G_LOG_LEVEL_INFO, "FORK -- (pid=%d) -- Unknown user %s (host %s)", pid, user, host);

            ss_rx->type = g_htonl(CONNECT_BYE);
	    g_strlcpy(ss_rx->to_user, ss_rx->from_user, PKT_USERNAME_LEN);
	    g_strlcpy(ss_rx->to_host, ss_rx->from_host, PKT_HOSTNAME_LEN);
	    ss_rx->to_port = ss_rx->from_port;

            len = strlen(username_manager) + 1;
	    if (PKT_USERNAME_LEN <= len) len = PKT_USERNAME_LEN;
	    g_strlcpy(ss_rx->from_user, username_manager, len);
            len = strlen(hostip_manager) + 1;
	    if (PKT_HOSTNAME_LEN <= len) len = PKT_HOSTNAME_LEN;
	    g_strlcpy(ss_rx->from_host, hostip_manager, len);
	    ss_rx->from_port = g_htons(tcp_rxport(sacpt));

	    if (tcp_send(sacpt, ss_rx, sizeof(SsipPkt), 0) != sizeof(SsipPkt))
	    {
		g_log(PROCESS_MANAGE, G_LOG_LEVEL_CRITICAL, "FORK -- (pid=%d) -- Unable send packet to %s (user %s)", pid, ss_rx->to_host ,ss_rx->to_user);
		g_log(PROCESS_MANAGE, G_LOG_LEVEL_INFO, "FORK -- (pid=%d) -- Connection FINISH to %s at port %d", pid, tohost, toport);
		g_free(host_fromcall);
		g_free(user_fromcall);
		g_free(user);
		g_free(host);
		if (value_host) g_free(tohost);
		goto __FINAL;
	    }

	    if (tcp_recv(sacpt, ss_ttx, sizeof(SsipPkt), 0) != sizeof(SsipPkt))
	    {
		g_log(PROCESS_MANAGE, G_LOG_LEVEL_CRITICAL, "FORK -- (pid=%d) -- Unable send packet to %s (user %s)", pid, ss_rx->to_host ,ss_rx->to_user);
	    }
	    g_log(PROCESS_MANAGE, G_LOG_LEVEL_INFO, "FORK -- (pid=%d) -- Connection FINISH to %s at port %d", pid, tohost, toport);
	    g_free(host_fromcall);
	    g_free(user_fromcall);
	    g_free(user);
	    g_free(host);
	    if (value_host) g_free(tohost);
	    goto __FINAL;
	}

	s_mag = tcp_init(PROGRAM_LOCALHOST, NULL, 0, toport, 15, &tmp_error);
	if (tmp_error != NULL)
	{
	    g_log(PROCESS_MANAGE, G_LOG_LEVEL_CRITICAL, "FORK -- (pid=%d) -- Unable create socket: %s", pid, tmp_error->message);

            len = strlen(tmp_error->message) + 1;
	    if (PKT_USERDATA_LEN <= len) len = PKT_USERDATA_LEN;
	    g_strlcpy(ss_rx->data, tmp_error->message, len);
	    ss_rx->data_len = len;
            g_clear_error(&tmp_error);
            goto __ERROR;
	}
        tcp_connect(s_mag, tohost, toport, &tmp_error);
	if (tmp_error != NULL)
	{
	    g_log(PROCESS_MANAGE, G_LOG_LEVEL_CRITICAL, "FORK -- (pid=%d) -- Unable connect to %s at port %d : %s", pid, tohost, toport, tmp_error->message);

            len = strlen(tmp_error->message) + 1;
	    if (PKT_USERDATA_LEN <= len) len = PKT_USERDATA_LEN;
	    g_strlcpy(ss_rx->data, tmp_error->message, len);
	    ss_rx->data_len = len;
            g_clear_error(&tmp_error);
            goto __ERROR;
	}

	g_log(PROCESS_MANAGE, G_LOG_LEVEL_INFO, "FORK -- (pid=%d) -- Connection INIT to %s at port %d", pid, tohost, toport);

	// send (same!) packet
	len = strlen(tohost) + 1;
	if (PKT_HOSTNAME_LEN <= len) len = PKT_HOSTNAME_LEN;
	g_strlcpy(ss_rx->to_host, tohost, len);
	ss_rx->to_port = g_htons(toport);

	if (tcp_send(s_mag, ss_rx, sizeof(SsipPkt), 0) != sizeof(SsipPkt))
	{
	    g_log(PROCESS_MANAGE, G_LOG_LEVEL_CRITICAL, "FORK -- (pid=%d) -- Unable send packet to %s (user %s)", pid, ss_rx->to_host ,ss_rx->to_user);

            len = strlen(tmp_error->message) + 1;
	    if (PKT_USERDATA_LEN <= len) len = PKT_USERDATA_LEN;
	    g_strlcpy(ss_rx->data, tmp_error->message, len);
	    ss_rx->data_len = len;
            g_clear_error(&tmp_error);
            goto __ERROR;
	}

        next_bye = FALSE;
	do
	{
	    if (tcp_recv(s_mag, ss_tx, sizeof(SsipPkt), 0) != sizeof(SsipPkt))
	    {
		g_log(PROCESS_MANAGE, G_LOG_LEVEL_CRITICAL, "FORK -- (pid=%d) -- Recv incomplete packet from %s (user %s)", pid, ss_tx->from_host ,ss_tx->from_user);
		len = strlen("(proxy) Recv incomplete packet from user host") + 1;
		if (PKT_USERDATA_LEN <= len) len = PKT_USERDATA_LEN;
		g_strlcpy(ss_rx->data, "(proxy) Recv incomplete packet from user host", len);
		ss_rx->data_len = len;
		goto __ERROR;
	    }

	    //len = strlen(hostip_manager) + 1;
	    //if (PKT_HOSTNAME_LEN <= len) len = PKT_HOSTNAME_LEN;
	    //g_strlcpy(ss_tx->from_host, hostip_manager, len);
	    //ss_tx->from_port = g_htons(tcp_rxport(sacpt));

	    tipo = g_ntohl(ss_tx->type);

	    if (tcp_send(sacpt, ss_tx, sizeof(SsipPkt), 0) != sizeof(SsipPkt))
	    {
		g_log(PROCESS_MANAGE, G_LOG_LEVEL_CRITICAL, "FORK -- (pid=%d) -- Unable send paket to %s (user %s)", pid, ss_tx->to_host ,ss_tx->to_user);
		ss_tx->type = g_htonl(CONNECT_ERROR);
		len = strlen("(proxy) Unable send packet to host") + 1;
		if (PKT_USERDATA_LEN <= len) len = PKT_USERDATA_LEN;
		g_strlcpy(ss_tx->data, "(proxy) Unable send packet to host", len);
		ss_tx->data_len = len;

		g_strlcpy(ss_tx->to_user, ss_tx->from_user, PKT_USERNAME_LEN);
		g_strlcpy(ss_tx->to_host, ss_tx->from_host, PKT_HOSTNAME_LEN);
		ss_tx->to_port = ss_tx->from_port;

		len = strlen(username_manager) + 1;
		if (PKT_USERNAME_LEN <= len) len = PKT_USERNAME_LEN;
		g_strlcpy(ss_tx->from_user, username_manager, len);
		len = strlen(hostip_manager) + 1;
		if (PKT_HOSTNAME_LEN <= len) len = PKT_HOSTNAME_LEN;
		g_strlcpy(ss_tx->from_host, hostip_manager, len);
		ss_tx->from_port = g_htons(tcp_rxport(s_mag));

		tcp_send(s_mag, ss_tx, sizeof(SsipPkt), 0);
                goto __FIN;
	    }

	    switch (tipo)
	    {
	    case CONNECT_BYE:
		if (next_bye) goto __FIN;
		next_bye = TRUE;
		break;

	    case CONNECT_ERROR:
                goto __FIN;
		break;

	    default:
                break;
	    }

	    if (tcp_recv(sacpt, ss_ttx, sizeof(SsipPkt), 0) != sizeof(SsipPkt))
	    {
		g_log(PROCESS_MANAGE, G_LOG_LEVEL_CRITICAL, "FORK -- (pid=%d) -- Recv incomplete paket from %s (user %s)", pid, ss_tx->from_host ,ss_tx->from_user);
		ss_ttx->type = g_htonl(CONNECT_ERROR);

		len = strlen("(proxy) Recv incomplete packet from user host") + 1;
		if (PKT_USERDATA_LEN <= len) len = PKT_USERDATA_LEN;
		g_strlcpy(ss_ttx->data, "(proxy) Recv incomplete packet from user host", len);
		ss_ttx->data_len = len;

		len = strlen(user) + 1;
		if (PKT_USERNAME_LEN <= len) len = PKT_USERNAME_LEN;
		g_strlcpy(ss_ttx->to_user, user, len);

		len = strlen(host) + 1;
		if (PKT_HOSTNAME_LEN <= len) len = PKT_HOSTNAME_LEN;
		g_strlcpy(ss_ttx->to_host, host, len);
		ss_ttx->to_port = g_htons(tcp_txport(s_mag));

		len = strlen(username_manager) + 1;
		if (PKT_USERNAME_LEN <= len) len = PKT_USERNAME_LEN;
		g_strlcpy(ss_ttx->from_user, username_manager, len);
		len = strlen(hostip_manager) + 1;
		if (PKT_HOSTNAME_LEN <= len) len = PKT_HOSTNAME_LEN;
		g_strlcpy(ss_ttx->from_host, hostip_manager, len);
		ss_ttx->from_port = g_htons(tcp_rxport(s_mag));

		tcp_send(s_mag, ss_ttx, sizeof(SsipPkt), 0);
                goto __FIN;
	    }

	    len = strlen(tohost) + 1;
	    if (PKT_HOSTNAME_LEN <= len) len = PKT_HOSTNAME_LEN;
	    g_strlcpy(ss_ttx->to_host, tohost, len);
	    ss_ttx->to_port = g_htons(toport);
	    tipo = g_ntohl(ss_ttx->type);

	    if (tcp_send(s_mag, ss_ttx, sizeof(SsipPkt), 0) != sizeof(SsipPkt))
	    {
		g_log(PROCESS_MANAGE, G_LOG_LEVEL_CRITICAL, "FORK -- (pid=%d) -- Unable send paket to %s (user %s)", pid, ss_ttx->to_host ,ss_ttx->to_user);
		len = strlen("(proxy) Unable send paket to host") + 1;
		if (PKT_USERDATA_LEN <= len) len = PKT_USERDATA_LEN;
		g_strlcpy(ss_rx->data, "(proxy) Unable send paket to host", len);
		ss_rx->data_len = len;
		goto __ERROR;
	    }

	    switch (tipo)
	    {
	    case CONNECT_BYE:
		if (next_bye) goto __FIN;
		next_bye = TRUE;
		break;

	    case CONNECT_ERROR:
                goto __FIN;
		break;

	    default:
                break;
	    }
	}
        while (TRUE);

	__ERROR:

	    ss_rx->type = g_htonl(CONNECT_ERROR);
	    len = strlen(user_fromcall) + 1;
	    if (PKT_USERNAME_LEN <= len) len = PKT_USERNAME_LEN;
	    g_strlcpy(ss_rx->to_user, user_fromcall, PKT_USERNAME_LEN);

	    len = strlen(host_fromcall) + 1;
	    if (PKT_HOSTNAME_LEN <= len) len = PKT_HOSTNAME_LEN;
	    g_strlcpy(ss_rx->to_host, host_fromcall, len);

	    ss_rx->to_port = port_fromcall;

	    len = strlen(username_manager) + 1;
	    if (PKT_USERNAME_LEN <= len) len = PKT_USERNAME_LEN;
	    g_strlcpy(ss_rx->from_user, username_manager, len);
	    len = strlen(hostip_manager) + 1;
	    if (PKT_HOSTNAME_LEN <= len) len = PKT_HOSTNAME_LEN;
	    g_strlcpy(ss_rx->from_host, hostip_manager, len);
	    ss_rx->from_port = g_htons(tcp_rxport(sacpt));

	    if (tcp_send(sacpt, ss_rx, sizeof(SsipPkt), 0) != sizeof(SsipPkt))
	    {
		g_log(PROCESS_MANAGE, G_LOG_LEVEL_CRITICAL, "FORK -- (pid=%d) -- Unable send paket to %s (user %s)", pid, ss_rx->to_host ,ss_rx->to_user);
	    }

	__FIN:

	    tcp_exit(s_mag, NULL);
	    g_log(PROCESS_MANAGE, G_LOG_LEVEL_INFO, "FORK -- (pid=%d) -- Connection FINISH to %s at port %d", pid, tohost, toport);

            g_free(host_fromcall);
	    g_free(user_fromcall);
	    g_free(user);
	    g_free(host);
            if (value_host) g_free(tohost);
    }
    else
    {
	g_log(PROCESS_MANAGE, G_LOG_LEVEL_WARNING, "FORK -- (pid=%d) -- Unknown packet from : %s", pid, sacpt->addr);
    }

    __FINAL:

    g_free(ss_rx);
    g_free(ss_tx);
    g_free(ss_ttx);

    return;
}



gboolean manage_user_process (gchar *filename, guint16 port, gint ch_pid)
{
    Socket_tcp *Sc;
    GError *tmp_error = NULL;
    gint sc;
    struct sigaction action;
    gint fpid = getpid();
    void handler_signal_manage(int);


    g_log(PROCESS_MANAGE, G_LOG_LEVEL_INFO, "PROCESS MANAGE -- INIT --, pid=%d at port %d", getpid(), port);

    action.sa_handler = handler_signal_manage;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_NOCLDSTOP;

    if ((sigaction(SIGUSR1, &action, NULL)) < 0)
    {
	g_log(PROCESS_MANAGE, G_LOG_LEVEL_CRITICAL, "%s", strerror(errno));
	kill(ch_pid, SIGUSR1);
	wait(0);
        return FALSE;
    }
    if ((sigaction(SIGUSR2, &action, NULL)) < 0)
    {
	g_log(PROCESS_MANAGE, G_LOG_LEVEL_CRITICAL, "%s", strerror(errno));
	kill(ch_pid, SIGUSR1);
	wait(0);
        return FALSE;
    }

    Sc = tcp_init(PROGRAM_LOCALHOST, NULL, port, 0, 15, &tmp_error);
    if (tmp_error != NULL)
    {
        g_log(PROCESS_MANAGE, G_LOG_LEVEL_CRITICAL, "%s", tmp_error->message);
	g_error_free(tmp_error);
	tmp_error = NULL;
	kill(ch_pid, SIGUSR1);
	wait(0);
        return FALSE;
    }
    sc = tcp_fd(Sc);
    if (listen(sc, PROGRAM_NUM_LISTEN) == -1)
    {
	g_log(PROCESS_MANAGE, G_LOG_LEVEL_CRITICAL, "%s", tmp_error->message);
	tcp_exit(Sc, NULL);
	kill(ch_pid, SIGUSR1);
	wait(0);
        return FALSE;
    }

    while (go_manage_process)
    {
	Socket_tcp *sacpt;


	while (childcount >= num_forks_manager)
	{
	    g_log(PROCESS_MANAGE, G_LOG_LEVEL_DEBUG, "Limits FORKS (%d)", childcount);
	    sleep(1);
	}


        sacpt = tcp_accept(Sc, &tmp_error);
	if (tmp_error != NULL)
	{
	    g_log(PROCESS_MANAGE, G_LOG_LEVEL_WARNING, "%s", tmp_error->message);
	    g_clear_error(&tmp_error);
            sleep(1);
            continue;
	}
	else
	{
	    gint pid;


	    g_log(PROCESS_MANAGE, G_LOG_LEVEL_DEBUG, "Connect from : %s", sacpt->addr);

	    if ((pid = fork()) == 0)
	    {
		pid = getpid();
		g_log(PROCESS_MANAGE, G_LOG_LEVEL_INFO, "FORK -- INIT -- MANAGE PROCESS, pid=%d -- remote host: %s", pid, sacpt->addr);
		tcp_exit(Sc, &tmp_error);
		if (tmp_error != NULL)
		{
		    g_log(PROCESS_MANAGE, G_LOG_LEVEL_WARNING, "FORK -- (pid=%d) -- %s", pid, tmp_error->message);
                    g_clear_error(&tmp_error);
		}
		protocol_manager(sacpt, filename);

		g_log(PROCESS_MANAGE, G_LOG_LEVEL_INFO, "FORK -- EXIT -- MANAGE PROCESS, pid=%d -- remote host: %s", pid, sacpt->addr);
		tcp_exit(sacpt, &tmp_error);
		if (tmp_error != NULL)
		{
		    g_log(PROCESS_MANAGE, G_LOG_LEVEL_WARNING, "FORK -- (pid=%d) -- Unable close socket: %s", pid, tmp_error->message);
                    g_clear_error(&tmp_error);
		}
		kill(fpid, SIGCHLD);
		exit(1);
	    }
	    else
	    {
		if (pid > 0)
		{
		    childcount++;
		}
		else
		{
		    g_log(PROCESS_MANAGE, G_LOG_LEVEL_CRITICAL, "Unable to fork: %s (remote host: %s)", strerror(errno), sacpt->addr);
		}
	    }
	    tcp_exit(sacpt, NULL);
	}
    }
    tcp_exit(Sc, &tmp_error);
    if (tmp_error != NULL)
    {
	g_log(PROCESS_MANAGE, G_LOG_LEVEL_WARNING, "Unable close socket: %s", tmp_error->message);
	g_error_free(tmp_error);
	kill(ch_pid, SIGUSR1);
        return FALSE;
    }
    kill(ch_pid, SIGUSR1);

    g_log(PROCESS_MANAGE, G_LOG_LEVEL_INFO, "PROCESS MANAGE -- FINISH --, pid=%d", getpid());

    return TRUE;
}



void handler_signal_notify (int sig)
{
    g_log(PROCESS_NOTIFY, G_LOG_LEVEL_INFO, "Signal received (%d)", sig);

    if (sig == SIGUSR1) go_notify_process = FALSE;
    else
    {
        // notificar al padre
	exit(0);
    }
}



gint add_user (gchar *filename, NtfPkt *pkt)
{
    gchar *from_user;
    gchar *from_host = NULL;
    guint16 fromport;
    gint ipmode;
    gint nada;
    gboolean value;
    gchar *des;
    ConfigFile *file;
    gchar *fileblock;
    gint fd_block;
    GError *tmp_error = NULL;


    file = cfgf_open_file(filename, &tmp_error);
    if (tmp_error != NULL)
    {
        g_log(PROCESS_NOTIFY, G_LOG_LEVEL_CRITICAL, "%s", tmp_error->message);
	g_error_free(tmp_error);
        return -1;
    }
    from_user = g_strndup(pkt->from_user, PKT_USERNAME_LEN);

    ipmode = g_ntohl(pkt->ipmode);
    if (ipmode != IPv6)
    {
        from_host = g_malloc0(INET_ADDRSTRLEN+1);
	inet_ntop(AF_INET, &(pkt->from_addr4), from_host, INET_ADDRSTRLEN);
        from_host[INET_ADDRSTRLEN] = '\0';
    }

#ifdef NET_HAVE_IPv6

    else
    {
        from_host = g_malloc0(INET6_ADDRSTRLEN+1);
	inet_ntop(AF_INET6, &(pkt->from_addr4), from_host, INET6_ADDRSTRLEN);
        from_host[INET6_ADDRSTRLEN] = '\0';
    }

#endif

    des = g_strndup(pkt->data, PKT_USERDES_LEN);
    fromport = g_ntohs(pkt->from_port);

    value = cfgf_read_int(file, from_user, "port", &nada);

    if (!value)
    {
	cfgf_write_string(file, from_user, "host", from_host);
	cfgf_write_int(file, from_user, "port", fromport);
	cfgf_write_string(file, from_user, "password", des);

	fileblock = g_strdup_printf("%s.lck", filename);
	fd_block = open(fileblock, O_RDWR | O_CREAT, 00700);
	file_block(fd_block, F_WRLCK);

	cfgf_write_file(file, filename, &tmp_error);

	cfgf_free(file);
	file_block(fd_block, F_UNLCK);
	g_free(fileblock);
	close(fd_block);
        value = TRUE;
    }
    else
    {
	cfgf_free(file);
        value = FALSE;
    }

    g_free(from_user);
    g_free(from_host);
    g_free(des);

    if (tmp_error != NULL)
    {
        g_log(PROCESS_NOTIFY, G_LOG_LEVEL_CRITICAL, "%s", tmp_error->message);
	g_error_free(tmp_error);
        return -1;
    }
    return value;
}



gint del_user (gchar *filename, NtfPkt *pkt)
{
    ConfigFile *file;
    GError *tmp_error = NULL;
    gchar *user;
    gchar *des;
    gchar *file_from_host;
    gchar *file_des;
    gboolean value;
    gboolean value_host;
    gboolean value_pass;
    gint ipmode;
    gchar *from_host = NULL;
    gchar *fileblock;
    gint fd_block;


    file = cfgf_open_file(filename, &tmp_error);
    if (tmp_error != NULL)
    {
        g_log(PROCESS_NOTIFY, G_LOG_LEVEL_CRITICAL, "%s", tmp_error->message);
	g_error_free(tmp_error);
        return -1;
    }
    user = g_strndup(pkt->from_user, PKT_USERNAME_LEN);
    des = g_strndup(pkt->data, PKT_USERDES_LEN);

    ipmode = g_ntohl(pkt->ipmode);
    if (pkt->ipmode != IPv6)
    {
        from_host = g_malloc0(INET_ADDRSTRLEN+1);
	inet_ntop(AF_INET, &(pkt->from_addr4), from_host, INET_ADDRSTRLEN);
        from_host[INET_ADDRSTRLEN] = '\0';
    }

#ifdef NET_HAVE_IPv6

    else
    {
        from_host = g_malloc0(INET6_ADDRSTRLEN+1);
	inet_ntop(AF_INET6, &(pkt->from_addr4), from_host, INET6_ADDRSTRLEN);
        from_host[INET6_ADDRSTRLEN] = '\0';
    }

#endif

    value = TRUE;
    value &= value_host = cfgf_read_string(file, user, "host", &file_from_host);
    value &= value_pass = cfgf_read_string(file, user, "password", &file_des);

    des[PKT_USERDES_LEN-1] = '\0';

    if ((value) && (!strcmp(file_des, des)) && (!strcmp(file_from_host, from_host)))
    {
	cfgf_remove_key(file, user, "host");
	cfgf_remove_key(file, user, "port");
	cfgf_remove_key(file, user, "password");

	fileblock = g_strdup_printf("%s.lck", filename);
	fd_block = open(fileblock, O_RDWR | O_CREAT, 00700);
	file_block(fd_block, F_WRLCK);

	cfgf_write_file(file, filename, &tmp_error);

	cfgf_free(file);
	file_block(fd_block, F_UNLCK);
	g_free(fileblock);
	close(fd_block);
	g_free(file_from_host);
	g_free(file_des);
        value = TRUE;
    }
    else
    {
        if (value_host) g_free(file_from_host);
        if (value_pass) g_free(file_des);
	cfgf_free(file);
        value = FALSE;
    }

    g_free(user);
    g_free(des);
    g_free(from_host);

    if (tmp_error != NULL)
    {
        g_log(PROCESS_NOTIFY, G_LOG_LEVEL_CRITICAL, "%s", tmp_error->message);
	g_error_free(tmp_error);
        return -1;
    }
    return value;
}



void notify_process (gint fpid, gchar *filename, guint16 port)
{
    Socket_tcp *Sc;
    GError *tmp_error = NULL;
    gint type;
    gint ret;
    gint sc;
    gint len;
    NtfPkt *ss_rx;
    NtfPkt *ss_tx;
    struct sigaction action;
    void handler_signal_notify(int);


    g_log(PROCESS_NOTIFY, G_LOG_LEVEL_INFO, "PROCESS NOTIFY -- INIT --, pid=%d, (father pid=%d) at port %d", getpid(), fpid, port);

    action.sa_handler = handler_signal_notify;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_NOCLDSTOP;

    if ((sigaction(SIGUSR1, &action, NULL)) < 0)
    {
	g_log(PROCESS_NOTIFY, G_LOG_LEVEL_CRITICAL, "%s", strerror(errno));
        exit(-1);
    }

    Sc = tcp_init(PROGRAM_LOCALHOST, NULL, port, 0, 15, &tmp_error);
    if (tmp_error != NULL)
    {
        g_log(PROCESS_NOTIFY, G_LOG_LEVEL_CRITICAL, "%s", tmp_error->message);
	g_error_free(tmp_error);
	tmp_error = NULL;
        exit(-1);
    }
    sc = tcp_fd(Sc);
    if (listen(sc, PROGRAM_NUM_LISTEN) == -1)
    {
	g_log(PROCESS_NOTIFY, G_LOG_LEVEL_CRITICAL, "%s", tmp_error->message);
	tcp_exit(Sc, NULL);
        exit(-1);
    }

    ss_tx = (NtfPkt *) g_malloc0(sizeof(NtfPkt));
    ss_rx = (NtfPkt *) g_malloc0(sizeof(NtfPkt));
    go_notify_process = TRUE;

    while (go_notify_process)
    {
	Socket_tcp *sacpt;


        sacpt = tcp_accept(Sc, &tmp_error);
	if (tmp_error != NULL)
	{
	    g_log(PROCESS_NOTIFY, G_LOG_LEVEL_WARNING, "%s", tmp_error->message);
	    g_error_free(tmp_error);
	    tmp_error = NULL;
            sleep(1);
            continue;
	}
	g_log(PROCESS_NOTIFY, G_LOG_LEVEL_DEBUG, "Connect from : %s", sacpt->addr);

	if (tcp_recv(sacpt, ss_rx, sizeof(NtfPkt), 0) == sizeof(NtfPkt))
	{
	    memset(ss_tx, 0, sizeof(NtfPkt));
	    ss_tx->ipmode = ss_rx->ipmode;
	    len = strlen(username_notify) + 1;
	    if (PKT_USERNAME_LEN <= len) len = PKT_USERNAME_LEN;
	    g_strlcpy(ss_tx->from_user, username_notify, len);
	    memcpy(&(ss_tx->from_addr4), &(ss_rx->to_addr4), sizeof(struct in_addr));
#ifdef NET_HAVE_IPv6
	    memcpy(&(ss_tx->from_addr6), &(ss_rx->to_addr6), sizeof(struct in6_addr));
#endif
	    ss_tx->from_port = g_htons(tcp_rxport(sacpt));

	    g_strlcpy(ss_tx->to_user, ss_rx->from_user, PKT_USERNAME_LEN);
	    memcpy(&(ss_tx->to_addr4), &(ss_rx->from_addr4), sizeof(struct in_addr));
#ifdef NET_HAVE_IPv6
	    memcpy(&(ss_tx->to_addr6), &(ss_rx->from_addr6), sizeof(struct in6_addr));
#endif
	    ss_tx->to_port = ss_rx->from_port;

            type = g_ntohl(ss_rx->type);
	    switch (type)
	    {
	    case USER_HELO_ONLINE:

		ret = add_user(filename, ss_rx);
		switch (ret)
		{
		case 1:
		    ss_tx->type = g_htonl(USER_OK_ONLINE);
		    g_log(PROCESS_NOTIFY, G_LOG_LEVEL_INFO, "Added user : '%s'", ss_rx->from_user);
		    break;
		case 0:
		    ss_tx->type = g_htonl(USER_INCORRECT);
		    g_log(PROCESS_NOTIFY, G_LOG_LEVEL_INFO, "Unable to add user : '%s', EXIST", ss_rx->from_user);
		    break;

		default:
		    ss_tx->type = g_htonl(ERROR_PKT);
		    g_log(PROCESS_NOTIFY, G_LOG_LEVEL_WARNING, "Unable to add user : '%s'", ss_rx->from_user);
                    break;
		}
		break;

	    case USER_BYE_ONLINE:

		ret = del_user(filename, ss_rx);
		switch (ret)
		{
		case 1:
		    ss_tx->type = g_htonl(USER_OK_OFFLINE);
		    g_log(PROCESS_NOTIFY, G_LOG_LEVEL_INFO, "Deleted user : '%s'", ss_rx->from_user);
		    break;
		case 0:
		    ss_tx->type = g_htonl(USER_INCORRECT);
		    g_log(PROCESS_NOTIFY, G_LOG_LEVEL_INFO, "Unable to del user : '%s', PASS INCORRECT", ss_rx->from_user);
		    break;
		default:
		    ss_tx->type = g_htonl(ERROR_PKT);
		    g_log(PROCESS_NOTIFY, G_LOG_LEVEL_WARNING, "Unable to del user : '%s'", ss_rx->from_user);
                    break;
		}
		break;

	    default:

		g_log(PROCESS_NOTIFY, G_LOG_LEVEL_WARNING, "Unknown init packet");
		ss_tx->type = g_htonl(ERROR_PKT);

                break;
	    }
	    if (tcp_send(sacpt, ss_tx, sizeof(NtfPkt), 0) != sizeof(NtfPkt))
	    {
		g_log(PROCESS_NOTIFY, G_LOG_LEVEL_WARNING, "Packet not send to '%s'", ss_tx->to_user);
	    }
	    tcp_exit(sacpt, &tmp_error);
	    if (tmp_error != NULL)
	    {
		g_log(PROCESS_NOTIFY, G_LOG_LEVEL_CRITICAL, "%s", tmp_error->message);
		g_error_free(tmp_error);
                tmp_error = NULL;
	    }
	}
	else
	{
	    g_log(PROCESS_NOTIFY, G_LOG_LEVEL_WARNING, "Unknown packet from : %s", sacpt->addr);
	}
    }
    tcp_exit(Sc, &tmp_error);
    if (tmp_error != NULL)
    {
	g_log(PROCESS_NOTIFY, G_LOG_LEVEL_CRITICAL, "%s", tmp_error->message);
	g_error_free(tmp_error);
	tmp_error = NULL;
    }
    g_free(ss_tx);
    g_free(ss_rx);

    g_log(PROCESS_NOTIFY, G_LOG_LEVEL_INFO, "PROCESS NOTIFY -- END --, pid=%d", getpid());
}



void log_handler (const gchar *log_domain, GLogLevelFlags log_level, const gchar *message,  gpointer user_data)
{
    time_t t;
    struct tm *t_m;


    UNUSED(user_data);
    time(&t);
    t_m=localtime(&t);

#ifndef SYSLOG_MESG

    switch (log_level)
    {
    case G_LOG_LEVEL_CRITICAL:

	printf("%24.24s %s: (CRITICAL) %s\n", asctime(t_m), log_domain, message);
        break;
    case G_LOG_LEVEL_WARNING:

	printf("%24.24s %s: (WARNING) %s\n", asctime(t_m), log_domain, message);
        break;
    case G_LOG_LEVEL_MESSAGE:

	printf("%24.24s %s: (MESSAGE) %s\n", asctime(t_m), log_domain, message);
        break;
    case G_LOG_LEVEL_INFO:

	printf("%24.24s %s: (INFO) %s\n", asctime(t_m), log_domain, message);
	break;
    case G_LOG_LEVEL_DEBUG:

	printf("%24.24s %s: (DEBUG) %s\n", asctime(t_m), log_domain, message);
	break;
    default:

	printf("%24.24s %s: (--) %s\n", asctime(t_m), log_domain, message);
	break;
    }
    fflush(NULL);

#else


    switch (log_level)
    {
    case G_LOG_LEVEL_CRITICAL:

	syslog(LOG_DAEMON | LOG_CRIT, "%s: (CRITICAL) %s\n", log_domain, message);
        break;
    case G_LOG_LEVEL_WARNING:

	syslog(LOG_DAEMON | LOG_WARNING, "%s: (WARNING) %s\n", log_domain, message);
        break;
    case G_LOG_LEVEL_MESSAGE:

	syslog(LOG_DAEMON | LOG_NOTICE, "%s: (MESSAGE) %s\n", log_domain, message);
        break;
    case G_LOG_LEVEL_INFO:

	syslog(LOG_DAEMON | LOG_INFO, "%s: (INFO) %s\n", log_domain, message);
	break;
    case G_LOG_LEVEL_DEBUG:

	syslog(LOG_DAEMON | LOG_WARNING, "%s: (DEBUG) %s\n", log_domain, message);
	break;
    default:

	syslog(LOG_DAEMON, "%s: (--) %s\n", log_domain, message);
	break;
    }

#endif

}



int main (int argc, char *argv[])
{
    ConfigFile *file;
    GError *error = NULL;
    gboolean value = TRUE;
    gint fpid;
    gint chpid;

    gchar *file_db;

    gchar *user_manage;
    gchar *host_manage;
    gint port_manage;
    gint chils_manage;

    gchar *user_notify;
    gchar *host_notify;
    gint port_not;



    if (argc != 2)
    {
	printf("\n\t%s v %s (c) %s, %s %s\n", SSIP_PROXY_PROGRAM_NAME, SSIP_PROXY_PROGRAM_VERSION, SSIP_PROXY_PROGRAM_DATE, SSIP_PROXY_PROGRAM_AUTHOR, SSIP_PROXY_PROGRAM_AMAIL);
	printf("\nNumero de parametros incorrecto.\n");
	printf("Modo de empleo:\n\n");
	printf("\t%s <fichero>\n", SSIP_PROXY_PROGRAM_NAME);
	printf("\n\tdonde fichero es el fichero de configuracion del programa.\n");
	printf("\tTodas las opciones de configuracion deben ser indicadas en ese\n");
	printf("\tfichero de configuracion.\n\n");
        exit(-1);
    }

    if (!g_thread_supported()) g_thread_init(NULL);

    file = cfgf_open_file(argv[1], &error);
    if (error != NULL)
    {
	printf("Error accediendo al fichero %s: %s.\n", argv[1], error->message);
        g_error_free(error);
        exit(-1);
    }

    value &= cfgf_read_string(file, CFG_GLOBAL_KEY, "db_file", &file_db);

    value &= cfgf_read_string(file, CFG_GLOBAL_KEY, "username_manage", &user_manage);
    value &= cfgf_read_string(file, CFG_GLOBAL_KEY, "hostip_manage", &host_manage);
    value &= cfgf_read_int(file, CFG_GLOBAL_KEY, "port_manage", &port_manage);
    value &= cfgf_read_int(file, CFG_GLOBAL_KEY, "num_forks_manage", &chils_manage);

    value &= cfgf_read_string(file, CFG_GLOBAL_KEY, "username_notify", &user_notify);
    value &= cfgf_read_string(file, CFG_GLOBAL_KEY, "hostip_notify", &host_notify);
    value &= cfgf_read_int(file, CFG_GLOBAL_KEY, "port_notify", &port_not);

    cfgf_free(file);

    if (!value)
    {
	printf("Fichero de configuracion %s incompleto.\n", argv[1]);
        exit(-1);
    }

    db_file = file_db;

    username_manager = user_manage;
    hostip_manager = host_manage;
    port_manager = port_manage;
    num_forks_manager = chils_manage;

    username_notify = user_notify;
    port_notify = port_not;
    hostip_notify = host_notify;


#ifdef SYSLOG_MESG

    openlog("SSIP_PROXY", 0, LOG_DAEMON);

#endif

    g_log_set_handler(PROCESS_MANAGE,
		      G_LOG_LEVEL_CRITICAL |
		      G_LOG_LEVEL_WARNING |
		      G_LOG_LEVEL_MESSAGE |
		      G_LOG_LEVEL_INFO |
		      G_LOG_LEVEL_DEBUG,
		      log_handler, NULL);

    g_log_set_handler(PROCESS_NOTIFY,
		      G_LOG_LEVEL_CRITICAL |
		      G_LOG_LEVEL_WARNING |
		      G_LOG_LEVEL_MESSAGE |
		      G_LOG_LEVEL_INFO |
		      G_LOG_LEVEL_DEBUG,
		      log_handler, NULL);


    open(file_db, O_RDWR | O_CREAT, 00700);


#ifdef PROGRAM_DAEMONMODE

    if (daemon(0, 0) == -1)
    {
	g_log("SSIP_PROXY", G_LOG_LEVEL_CRITICAL, "PROCESS Unable FORK %s", strerror(errno));
        exit(0);
    }

#endif

    fpid = getpid();

    if ((chpid = fork()) == 0)
    {
	notify_process(fpid, file_db, port_notify);
        exit(1);
    }
    else
    {
	if (chpid > 0)
	{
	    value = manage_user_process(file_db, port_manage, chpid);
	    wait(0);
	}
	else
	{
	    g_log("SSIP_PROXY", G_LOG_LEVEL_CRITICAL, "PROCESS Unable FORK (pid=%d)", chpid);
	}
    }

#ifdef SYSLOG_MESG

    closelog();

#endif

    g_free(file_db);
    g_free(user_manage);
    g_free(host_manage);
    g_free(user_notify);
    g_free(host_notify);

    exit(1);
}



