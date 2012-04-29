
/*
 *
 * FILE: tcp.c
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

#include "tcp.h"
#include "net.h"


#include <stdio.h>


// IPv4 specific functions ...

static Socket_tcp *tcp_init4 (const gchar *addr, const gchar *iface, guint16 rx_port, guint16 tx_port, gint ttl, GError **error)
{
    struct sockaddr_in s_in;
    struct in_addr iface_addr;
    Socket_tcp *s;

#ifdef TCP_SETBUFFERSIZE
    gint tcpbufsize = TCP_SETBUFFERSIZE;
#endif

#ifdef TCP_SOREUSEADDR
    gint reuse = 1;
#else
#ifdef TCP_SOREUSEPORT
    gint reuse = 1;
#endif
#endif


    s = (Socket_tcp *) g_try_malloc(sizeof(Socket_tcp));
    g_return_val_if_fail(s != NULL, NULL);
    s->mode = IPv4;
    s->addr = NULL;
    s->rx_port = rx_port;
    s->tx_port = tx_port;
    s->ttl = ttl;
    if (inet_pton(AF_INET, addr, &s->addr4) != 1)
    {
	struct hostent *h = gethostbyname(addr);
	if (h == NULL)
	{
	    g_set_error(error, TCP_ERROR, TCP_ERROR_UNRESOLVEIP, "can't resolve IP address for %s", addr);
	    g_printerr("[TCP] tcp_init4: can't resolve IP address for %s\n", addr);
	    g_free(s);
	    return NULL;
	}
	memcpy(&(s->addr4), h->h_addr_list[0], sizeof(s->addr4));
    }

    if (iface != NULL)
    {
	if (inet_pton(AF_INET, iface, &iface_addr) != 1)
	{
	    g_set_error(error, TCP_ERROR, TCP_ERROR_NOIFACE, "illegal interface specification");
	    g_printerr("[TCP] tcp_init4: illegal interface specification\n");
	    g_free(s);
	    return NULL;
	}
    } else iface_addr.s_addr = 0;

    s->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (s->fd < 0)
    {
	g_set_error(error, TCP_ERROR, TCP_ERROR_ERRNO, "%s", g_strerror(errno));
	g_printerr("[TCP] tcp_init4: %s\n", (*error)->message);
	g_free(s);
	return NULL;
    }

#ifdef TCP_SETBUFFERSIZE
    if (setsockopt(s->fd, SOL_SOCKET, SO_SNDBUF, (char *) &tcpbufsize, sizeof(tcpbufsize)) != 0)
    {
	g_set_error(error, TCP_ERROR, TCP_ERROR_ERRNO, "%s", g_strerror(errno));
	g_printerr("[TCP] tcp_init4: %s\n", (*error)->message);
	g_free(s);
	return NULL;
    }
    if (setsockopt(s->fd, SOL_SOCKET, SO_RCVBUF, (char *) &tcpbufsize, sizeof(tcpbufsize)) != 0)
    {
	g_set_error(error, TCP_ERROR, TCP_ERROR_ERRNO, "%s", g_strerror(errno));
	g_printerr("[TCP] tcp_init4: %s\n", (*error)->message);
	g_free(s);
	return NULL;
    }
#endif

#ifdef TCP_SOREUSEADDR
    if (setsockopt(s->fd, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse, sizeof(reuse)) != 0)
    {
	g_set_error(error, TCP_ERROR, TCP_ERROR_ERRNO, "%s", g_strerror(errno));
	g_printerr("[TCP] tcp_init4: %s\n", (*error)->message);
	g_free(s);
	return NULL;
    }
#endif

#ifdef TCP_SOREUSEPORT
    if (setsockopt(s->fd, SOL_SOCKET, SO_REUSEPORT, (char *) &reuse, sizeof(reuse)) != 0)
    {
	g_set_error(error, TCP_ERROR, TCP_ERROR_ERRNO, "%s", g_strerror(errno));
	g_printerr("[TCP] tcp_init4: %s\n", (*error)->message);
	g_free(s);
	return NULL;
    }
#endif

    s_in.sin_family = AF_INET;
    s_in.sin_addr.s_addr = INADDR_ANY;
    s_in.sin_port = g_htons(rx_port);

    if (bind(s->fd, (struct sockaddr *) &s_in, sizeof(s_in)) != 0)
    {
	g_set_error(error, TCP_ERROR, TCP_ERROR_ERRNO, "%s", g_strerror(errno));
	g_printerr("[TCP] tcp_init4: %s\n", (*error)->message);
	g_free(s);
	return NULL;
    }

    if (IN_MULTICAST(g_ntohl(s->addr4.s_addr)))
    {
	gchar loop = 1;
	struct ip_mreq  imr;
		
	imr.imr_multiaddr.s_addr = s->addr4.s_addr;
	imr.imr_interface.s_addr = iface_addr.s_addr;
		
	if (setsockopt(s->fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) &imr, sizeof(struct ip_mreq)) != 0)
	{
	    g_set_error(error, TCP_ERROR, TCP_ERROR_ERRNO, "%s", g_strerror(errno));
	    g_printerr("[TCP] tcp_init4: %s\n", (*error)->message);
	    g_free(s);
	    return NULL;
	}
	if (setsockopt(s->fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) != 0)
	{
	    g_set_error(error, TCP_ERROR, TCP_ERROR_ERRNO, "%s", g_strerror(errno));
	    g_printerr("[TCP] tcp_init4: %s\n", (*error)->message);
	    g_free(s);
	    return NULL;
	}
	if (setsockopt(s->fd, IPPROTO_IP, IP_MULTICAST_TTL, (char *) &s->ttl, sizeof(s->ttl)) != 0)
	{
	    g_set_error(error, TCP_ERROR, TCP_ERROR_ERRNO, "%s", g_strerror(errno));
	    g_printerr("[TCP] tcp_init4: %s\n", (*error)->message);
	    g_free(s);
	    return NULL;
	}

	if (iface_addr.s_addr != 0)
	{
	    if (setsockopt(s->fd, IPPROTO_IP, IP_MULTICAST_IF, (char *) &iface_addr, sizeof(iface_addr)) != 0)
	    {
		g_set_error(error, TCP_ERROR, TCP_ERROR_ERRNO, "%s", g_strerror(errno));
		g_printerr("[TCP] tcp_init4: %s\n", (*error)->message);
		g_free(s);
		return NULL;
	    }
	}
    }
    return s;
}



static void tcp_exit4 (Socket_tcp *s, GError **error)
{
    if (IN_MULTICAST(g_ntohl(s->addr4.s_addr)))
    {
	struct ip_mreq  imr;
	imr.imr_multiaddr.s_addr = s->addr4.s_addr;
	imr.imr_interface.s_addr = INADDR_ANY;

	if (setsockopt(s->fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *) &imr, sizeof(struct ip_mreq)) != 0)
	{
	    g_set_error(error, TCP_ERROR, TCP_ERROR_ERRNO, "%s", g_strerror(errno));
	    g_printerr("[TCP] tcp_exit4: %s\n", (*error)->message);
	}
    }
    close(s->fd);
    g_free(s->addr);
    g_free(s);
    return;
}



static gchar *tcp_host_addr4 (GError **error)
{
    gchar *hname;
    struct hostent *hent;
    struct in_addr iaddr;


    hname = (gchar *) g_try_malloc(sizeof (gchar) * TCP_MAXHOSTNAMELEN);
    g_return_val_if_fail(hname != NULL, NULL);
    if (gethostname(hname, TCP_MAXHOSTNAMELEN) != 0)
    {
	g_set_error(error, TCP_ERROR, TCP_ERROR_ERRNO, "%s", g_strerror(errno));
	g_printerr("[TCP] tcp_host_addr4: %s\n", (*error)->message);
	return NULL;
    }
    hent = gethostbyname(hname);
    if (hent == NULL)
    {
	g_set_error(error, TCP_ERROR, TCP_ERROR_UNRESOLVEIP, "can't resolve IP address for %s", hname);
	g_printerr("[TCP] tcp_host_addr4 : can't resolve IP address for %s\n", hname);
	return NULL;
    }

    g_return_val_if_fail(hent->h_addrtype == AF_INET, NULL);
    memcpy(&iaddr.s_addr, hent->h_addr, sizeof(iaddr.s_addr));
    strncpy(hname, inet_ntoa(iaddr), TCP_MAXHOSTNAMELEN);
    return (gchar *) hname;
}



#ifdef TCP_HAVE_IPv6

// IPv6 specific functions...

static Socket_tcp *tcp_init6 (const gchar *addr, const gchar *iface, guint16 rx_port, guint16 tx_port, gint ttl, GError **error)
{
    struct sockaddr_in6 s_in;
    Socket_tcp *s;

#ifdef TCP_SETBUFFERSIZE
    gint tcpbufsize = TCP_SETBUFFERSIZE;
#endif

#ifdef TCP_SOREUSEADDR
    gint reuse = 1;
#else
#ifdef TCP_SOREUSEPORT
    gint reuse = 1;
#endif
#endif


    if (iface != NULL)
    {
	g_printerr("[TCP] tcp_init6: not implemented!");
	g_set_error(error, TCP_ERROR, TCP_ERROR_NOTIMPLEMENTED, "not implemented!");
	return NULL;
    }

    s = (Socket_tcp *) g_try_malloc(sizeof(Socket_tcp));
    g_return_val_if_fail(s != NULL, NULL);
    s->mode = IPv6;
    s->addr = NULL;
    s->rx_port = rx_port;
    s->tx_port = tx_port;
    s->ttl = ttl;

    if (inet_pton(AF_INET6, addr, &s->addr6) != 1)
    {
	/* We should probably try to do a DNS lookup on the name */
	/* here, but I'm trying to get the basics going first... */
	g_set_error(error, TCP_ERROR, TCP_ERROR_ERRNO, "%s", g_strerror(errno));
	g_printerr("[TCP] tcp_init6: %s\n", (*error)->message);
	g_free(s);
	return NULL;
    }

    s->fd = socket(AF_INET6, SOCK_STREAM, 0);
    if (s->fd < 0)
    {
	g_set_error(error, TCP_ERROR, TCP_ERROR_ERRNO, "%s", g_strerror(errno));
	g_printerr("[TCP] tcp_init6: %s\n", (*error)->message);
	g_free(s);
	return NULL;
    }

#ifdef TCP_SETBUFFERSIZE
    if (setsockopt(s->fd, SOL_SOCKET, SO_SNDBUF, (char *) &tcpbufsize, sizeof(tcpbufsize)) != 0)
    {
	g_set_error(error, TCP_ERROR, TCP_ERROR_ERRNO, "%s", g_strerror(errno));
	g_printerr("[TCP] tcp_init6: %s\n", (*error)->message);
	g_free(s);
	return NULL;
    }
    if (setsockopt(s->fd, SOL_SOCKET, SO_RCVBUF, (char *) &tcpbufsize, sizeof(tcpbufsize)) != 0)
    {
	g_set_error(error, TCP_ERROR, TCP_ERROR_ERRNO, "%s", g_strerror(errno));
	g_printerr("[TCP] tcp_init6: %s\n", (*error)->message);
	g_free(s);
	return NULL;
    }
#endif

#ifdef TCP_SOREUSEADDR
    if (setsockopt(s->fd, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse, sizeof(reuse)) != 0)
    {
	g_set_error(error, TCP_ERROR, TCP_ERROR_ERRNO, "%s", g_strerror(errno));
	g_printerr("[TCP] tcp_init6: %s\n", (*error)->message);
	g_free(s);
	return NULL;
    }
#endif

#ifdef TCP_SOREUSEPORT
    if (setsockopt(s->fd, SOL_SOCKET, SO_REUSEPORT, (char *) &reuse, sizeof(reuse)) != 0)
    {
	g_set_error(error, TCP_ERROR, TCP_ERROR_ERRNO, "%s", g_strerror(errno));
	g_printerr("[TCP] tcp_init6: %s\n", (*error)->message);
	g_free(s);
	return NULL;
    }
#endif

    memset((char *)&s_in, 0, sizeof(s_in));
    s_in.sin6_family = AF_INET6;
    s_in.sin6_port = g_htons(rx_port);

#ifdef HAVE_SIN6_LEN
    s_in.sin6_len = sizeof(s_in);
#endif

    s_in.sin6_addr = in6addr_any;
    if (bind(s->fd, (struct sockaddr *) &s_in, sizeof(s_in)) != 0)
    {
	g_set_error(error, TCP_ERROR, TCP_ERROR_ERRNO, "%s", g_strerror(errno));
	g_printerr("[TCP] tcp_init6: %s\n", (*error)->message);
	g_free(s);
	return NULL;
    }
	
    if (IN6_IS_ADDR_MULTICAST(&(s->addr6)))
    {
	//unsigned int loop = 1;
	guint loop = 1;
	struct ipv6_mreq  imr;

	imr.ipv6mr_multiaddr = s->addr6;
	imr.ipv6mr_interface = 0;
		
	if (setsockopt(s->fd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (char *) &imr, sizeof(struct ipv6_mreq)) != 0)
	{
	    g_set_error(error, TCP_ERROR, TCP_ERROR_ERRNO, "%s", g_strerror(errno));
	    g_printerr("[TCP] tcp_init6: %s\n", (*error)->message);
	    g_free(s);
	    return NULL;
	}
	if (setsockopt(s->fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, (char *) &loop, sizeof(loop)) != 0)
	{
	    g_set_error(error, TCP_ERROR, TCP_ERROR_ERRNO, "%s", g_strerror(errno));
	    g_printerr("[TCP] tcp_init6: %s\n", (*error)->message);
	    g_free(s);
	    return NULL;
	}
	if (setsockopt(s->fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, (char *) &ttl, sizeof(ttl)) != 0)
	{
	    g_set_error(error, TCP_ERROR, TCP_ERROR_ERRNO, "%s", g_strerror(errno));
	    g_printerr("[TCP] tcp_init6: %s\n", (*error)->message);
	    g_free(s);
	    return NULL;
	}
    }
    return s;
}



static void tcp_exit6 (Socket_tcp *s, GError **error)
{
    if (IN6_IS_ADDR_MULTICAST(&(s->addr6)))
    {
	struct ipv6_mreq  imr;

	imr.ipv6mr_multiaddr = s->addr6;
	imr.ipv6mr_interface = 0;

	if (setsockopt(s->fd, IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP, (char *) &imr, sizeof(struct ipv6_mreq)) != 0)
	{
	    g_set_error(error, TCP_ERROR, TCP_ERROR_ERRNO, "%s", g_strerror(errno));
	    g_printerr("[TCP] tcp_exit6: %s\n", (*error)->message);
	}
    }
    close(s->fd);
    g_free(s->addr);
    g_free(s);
}



static gchar *tcp_host_addr6 (Socket_tcp *s, GError **error)
{
    gchar *hname;
    gint newsock;
    struct addrinfo hints, *ai;
    struct sockaddr_in6 local, addr6;
    gint result;
    gint len;


    hname = (gchar *) g_try_malloc(sizeof (gchar) * TCP_MAXHOSTNAMELEN);
    g_return_val_if_fail(hname != NULL, NULL);
    len = sizeof(struct sockaddr_in6);
    newsock = socket(AF_INET6, SOCK_DGRAM, 0);
    memset((char *)&addr6, 0, sizeof(struct sockaddr_in6));
    addr6.sin6_family = AF_INET6;

#ifdef HAVE_SIN6_LEN
    addr6.sin6_len = sizeof(struct sockaddr_in6);
#endif

    bind(newsock, (struct sockaddr *) &addr6, sizeof(struct sockaddr_in6));
    addr6.sin6_addr = s->addr6;
    addr6.sin6_port = g_htons(s->rx_port);
    connect(newsock, (struct sockaddr *) &addr6, sizeof(struct sockaddr_in6));
    memset((char *)&local, 0, sizeof(struct sockaddr_in6));
    if ((result = getsockname(newsock, (struct sockaddr *)&local, &len)) < 0)
    {
	local.sin6_addr = in6addr_any;
	local.sin6_port = 0;
	g_print("[TCP] tcp_host_addr6: getsockname failed (IPv6)\n");
    }
    close (newsock);

    if (IN6_IS_ADDR_UNSPECIFIED(&local.sin6_addr) || IN6_IS_ADDR_MULTICAST(&local.sin6_addr))
    {
	if (gethostname(hname, TCP_MAXHOSTNAMELEN) != 0)
	{
	    g_set_error(error, TCP_ERROR, TCP_ERROR_ERRNO, "%s", g_strerror(errno));
	    g_printerr("[TCP] tcp_host_addr6: %s\n", (*error)->message);
	    return NULL;
	}
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = 0;
	hints.ai_flags = 0;
	hints.ai_addrlen = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	if ((result = getaddrinfo(hname, NULL, &hints, &ai)) < 0)
	{
	    freeaddrinfo(ai);   // ¿!
	    g_set_error(error, TCP_ERROR, TCP_ERROR_ERRNO, "getaddrinfo: %s: %s", hname, gai_strerror(result));
	    g_printerr("[TCP] tcp_host_addr6: getaddrinfo: %s: %s\n", hname, gai_strerror(result));
	    return NULL;
	}
	if (inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)(ai->ai_addr))->sin6_addr), hname, TCP_MAXHOSTNAMELEN) == NULL)
	{
	    freeaddrinfo(ai);
	    g_set_error(error, TCP_ERROR, TCP_ERROR_ERRNO, "%s (inet_ntop: %s)", g_strerror(errno), hname);
	    g_printerr("[TCP] tcp_host_addr6: %s\n", (*error)->message);
	    return NULL;
	}
	freeaddrinfo(ai);
	return (gchar *) hname;
    }
    if (inet_ntop(AF_INET6, &local.sin6_addr, hname, TCP_MAXHOSTNAMELEN) == NULL)
    {
	g_set_error(error, TCP_ERROR, TCP_ERROR_ERRNO, "%s (inet_ntop: %s)", g_strerror(errno), hname);
	g_printerr("[TCP] tcp_host_addr6: %s\n", (*error)->message);
	return NULL;
    }
    return (gchar *) hname;
}


#endif  //IPv6



// Funciones genericas, que abstraen la version del protocolo usado:
// IPv4 ó IPv6



/**
 * tcp_init:
 * @addr: cadena de caracteres que representa una direccion de red, tanto
 * en IPv4 como en IPv6.
 * @iface: cadena de caracteres que representa un nombre de interfaz o NULL
 * para seleccionar la interfaz por defecto.
 * @rx_port: puerto de recepcion.
 * @tx_port: puerto de transmision.
 * @ttl: valor time-to-live para los paquetes transmitidos.
 * @error: doble puntero a una estructura GError.
 *
 * Crea una sesion para enviar y recibir paquetes TCP sobre redes IP.
 * La sesion usa @iface como el interfaz para enviar y recibir los paquetes.
 * Si @iface es NULL, entonces se usara la interfaz por defecto
 * 
 * Retorna un puntero a una estructura Socket_tcp si la funcion tuvo exito, en
 * otro caso devuelve NULL y rellena la estructura @error con el tipo/codigo
 * de error generado.
 *
 **/

Socket_tcp *tcp_init (const gchar *addr, const gchar *iface, guint16 rx_port, guint16 tx_port, gint ttl, GError **error)
{
    GError *tmp_error = NULL;
    Socket_tcp *session_tcp = NULL;


    if (strchr(addr, ':') == NULL)
    {
	session_tcp = tcp_init4(addr, iface, rx_port, tx_port, ttl, &tmp_error);
	if (session_tcp == NULL)
	{
	    g_propagate_error(error, tmp_error);
	    return NULL;
	}
    }
    else
    {

#ifdef TCP_HAVE_IPv6
	session_tcp = tcp_init6(addr, iface, rx_port, tx_port, ttl, &tmp_error);
	if (session_tcp == NULL)
	{
	    g_propagate_error(error, tmp_error);
	    return NULL;
	}
#else
	return NULL;
#endif

    }
    return session_tcp;
}



/**
 * tcp_exit:
 * @s: sesion a terminar.
 * @error: doble puntero a una estructura GError.
 *
 * Termina una sesion iniciada con tcp_init.
 * 
 **/

void tcp_exit (Socket_tcp *s, GError **error)
{
    GError *tmp_error = NULL;


    if (s == NULL)
    {
	g_set_error(error, TCP_ERROR, TCP_ERROR_NULL, "NULL");
	g_printerr("[TCP] tcp_exit: Socket_tcp = NULL\n");
	return;
    }
    switch(s->mode)
    {
       case IPv4 :
	   tcp_exit4(s, &tmp_error);
	   if (tmp_error != NULL)
	   {
	       g_propagate_error(error, tmp_error);
	       return;
	   }
	   break;

#ifdef TCP_HAVE_IPv6
       case IPv6 :
	   tcp_exit6(s, &tmp_error);
	   if (tmp_error != NULL)
	   {
	       g_propagate_error(error, tmp_error);
	       return;
	   }
	   break;
#endif

       default :
	   g_set_error(error, TCP_ERROR, TCP_ERROR_NOTIMPLEMENTED, "NULL");
	   g_printerr("[TCP] tcp_exit: Socket type must be IPv4 or IPv6\n");
	   return;
    }
}



/**
 * tcp_connect:
 * @s: sesion.
 * @addr: direccion a donde conectar el socket.
 * @port: puerto a donde conectar el socket.
 * @error: doble puntero a una estructura GError.
 *
 * Ver connect (Sockets API)
 * 
 **/

gboolean tcp_connect (Socket_tcp *s, const gchar *addr, guint16 port, GError **error)
{
    struct sockaddr_in con4;

#ifdef TCP_HAVE_IPv6
    struct sockaddr_in6 con6;
#endif


    if (s == NULL)
    {
	g_set_error(error, TCP_ERROR, TCP_ERROR_NULL, "NULL");
	g_printerr("[TCP] tcp_exit: Socket_tcp = NULL\n");
	return FALSE;
    }
    switch(s->mode)
    {
    case IPv4 :

	con4.sin_family = AF_INET;
	con4.sin_port = g_htons(port);

	if (inet_pton(AF_INET, addr, &con4.sin_addr) != 1)
	{
	    g_set_error(error, TCP_ERROR, TCP_ERROR_ERRNO, "%s", g_strerror(errno));
	    g_printerr("[TCP] tcp_connect: %s\n", (*error)->message);
	    return FALSE;
	}
	if (connect(s->fd, (struct sockaddr *) &con4, sizeof(con4)) == -1)
	{
	    g_set_error(error, TCP_ERROR, TCP_ERROR_ERRNO, "%s", g_strerror(errno));
	    g_printerr("[TCP] tcp_connect: %s\n", (*error)->message);
	    return FALSE;
	}
	s->tx_port = port;
        if (s->addr != NULL) g_free(s->addr);
        s->addr = g_strdup(addr);
        g_memmove(&(s->addr4), &con4.sin_addr, sizeof(struct in_addr));
	break;

#ifdef TCP_HAVE_IPv6
    case IPv6 :

	con6.sin6_family = AF_INET6;
	con6.sin6_port = g_htons(port);

	if (inet_pton(AF_INET6, addr, &con6.sin6_addr) != 1)
	{
	    g_set_error(error, TCP_ERROR, TCP_ERROR_ERRNO, "%s", g_strerror(errno));
	    g_printerr("[TCP] tcp_connect: %s\n", (*error)->message);
	    return FALSE;
	}
	if (connect(s->fd, (struct sockaddr *) &con6, sizeof(con6)) == -1)
	{
	    g_set_error(error, TCP_ERROR, TCP_ERROR_ERRNO, "%s", g_strerror(errno));
	    g_printerr("[TCP] tcp_connect: %s\n", (*error)->message);
	    return FALSE;
	}
	s->tx_port = port;
        if (s->addr != NULL) g_free(s->addr);
        s->addr = g_strdup(addr);
        g_memmove(&(s->addr6), &con6.sin6_addr, sizeof(struct in6_addr));
	break;
#endif

    default :
	g_set_error(error, TCP_ERROR, TCP_ERROR_NOTIMPLEMENTED, "NULL");
	g_printerr("[TCP] tcp_connect: Socket type must be IPv4 or IPv6\n");
	return FALSE;
    }
    return TRUE;
}



/**
 * tcp_accept:
 * @s: sesion.
 * @addr: direccion a donde conectar el socket.
 * @port: puerto a donde conectar el socket.
 * @error: doble puntero a una estructura GError.
 *
 * Ver accept (Sockets API)
 * 
 **/

Socket_tcp *tcp_accept (Socket_tcp *s, GError **error)
{
    struct sockaddr_in con4;
    gint fd;
    gint addrlen;
    gchar buf[100];
    Socket_tcp *st;

#ifdef TCP_HAVE_IPv6
    struct sockaddr_in6 con6;
#endif


    if (s == NULL)
    {
	g_set_error(error, TCP_ERROR, TCP_ERROR_NULL, "NULL");
	g_printerr("[TCP] tcp_accept: Socket_tcp = NULL\n");
	return NULL;
    }
    switch(s->mode)
    {
    case IPv4 :

        addrlen = sizeof(struct sockaddr_in);
	if (-1 == (fd = accept(s->fd, (struct sockaddr *) &con4, &addrlen)))
	{
	    g_set_error(error, TCP_ERROR, TCP_ERROR_ERRNO, "%s", g_strerror(errno));
	    g_printerr("[TCP] tcp_accept: %s\n", (*error)->message);
	    return NULL;
	}
	st = (Socket_tcp *) g_try_malloc(sizeof(Socket_tcp));
	g_return_val_if_fail(st != NULL, NULL);
	st->mode = IPv4;
	memset(buf, 0, sizeof(gchar) * 100);
        inet_ntop(AF_INET, &con4.sin_addr, buf, sizeof(gchar) * 100);
	st->addr = g_strdup(buf);
	st->rx_port = s->rx_port;
	st->tx_port = con4.sin_port;
	st->ttl = s->ttl;
	st->fd = fd;

	break;

#ifdef TCP_HAVE_IPv6
    case IPv6 :

        addrlen = sizeof(struct sockaddr_in6);
	if (-1 == (fd = accept(s->fd, (struct sockaddr *) &con6, &addrlen)))
	{
	    g_set_error(error, TCP_ERROR, TCP_ERROR_ERRNO, "%s", g_strerror(errno));
	    g_printerr("[TCP] tcp_accept: %s\n", (*error)->message);
	    return NULL;
	}
	st = (Socket_tcp *) g_try_malloc(sizeof(Socket_tcp));
	g_return_val_if_fail(st != NULL, NULL);
	st->mode = IPv6;
	memset(buf, 0, sizeof(gchar) * 100);
        inet_ntop(AF_INET6, &con6.sin6_addr, buf, sizeof(gchar) * 100);
	st->addr = g_strdup(buf);
	st->rx_port = con6.sin6_port;
	st->tx_port = s->tx_port;
	st->ttl = s->ttl;
	st->fd = fd;
	break;
#endif

    default :
	g_set_error(error, TCP_ERROR, TCP_ERROR_NOTIMPLEMENTED, "NULL");
	g_printerr("[TCP] tcp_accept: Socket type must be IPv4 or IPv6\n");
	return NULL;
    }
    return st;
}



/**
 * tcp_send:
 * @s: sesion.
 * @buffer: puntero al buffer cuyo contenido va a ser transmitido.
 * @buflen: tamaño de @buffer.
 * @flags: flags, ver funcion send (Sockets API)
 *
 * Transmite un datagrama TCP con el contenido del @buffer.
 * 
 * Valor retornado: Numero de bytes enviados si todo va bien o un codigo de
 * error negativo en otro caso.
 *
 **/

gint tcp_send (Socket_tcp *s, gpointer buffer, gint buflen, gint flags)
{
    g_return_val_if_fail(s != NULL, -5);
    g_return_val_if_fail(s->mode == IPv4, -4);
    g_return_val_if_fail(buffer != NULL, -3);
    g_return_val_if_fail(buflen > 0, -2);

    return send(s->fd, buffer, buflen, flags);
}



/**
 * tcp_recv:
 * @s: sesion.
 * @buffer: buffer donde recoger los datos recibidos.
 * @buflen: tamaño de @buffer.
 * @flags: flags, ver funcion recv (Sockets API)
 * 
 * Lee los datos de la cola de entrada (socket) y los coloca en @buffer.
 *
 * Retorna el numero de bytes recibidos o un numero negativo que indica
 * el codigo de error.
 *
 **/

inline gint tcp_recv (Socket_tcp *s, gpointer buffer, gint buflen, gint flags)
{
    /* Reads data into the buffer, returning the number of bytes read.   */
    /* If no data is available, this returns the value zero immediately. */
    /* Note: since we don't care about the source address of the packet  */
    /* we receive, this function becomes protocol independent.           */
    gint len;

    g_return_val_if_fail(s != NULL, -5);
    g_return_val_if_fail(buffer != NULL, -3);
    g_return_val_if_fail(buflen > 0, -2);

    len = recv(s->fd, buffer, buflen, flags);
    g_return_val_if_fail(buflen > 0, len);
    return len;
}



/**
 * tcp_host_addr:
 * @s: sesion.
 * @error: doble puntero a una estructura GError.
 *
 * Retorna un puntero a un string que contiene la direccion asociada a
 * la sesion @s; si ocurrio un error, NULL y rellena la estructura @error.
 *
 **/

gchar *tcp_host_addr (Socket_tcp *s, GError **error)
{
    GError *tmp_error = NULL;
    gchar* c;


    if (s == NULL)
    {
	g_set_error(error, TCP_ERROR, TCP_ERROR_NULL, "NULL");
	g_printerr("[TCP] tcp_host_addr: Socket_tcp = NULL\n");
	return NULL;
    }
    switch(s->mode)
    {
       case IPv4 :
           c = tcp_host_addr4(&tmp_error);
	   if (tmp_error != NULL)
	   {
	       g_propagate_error(error, tmp_error);
	       return NULL;
	   }
	   return c;
	   break;

#ifdef TCP_HAVE_IPv6
       case IPv6 :
	   c = tcp_host_addr6(s, &tmp_error);
	   if (tmp_error != NULL)
	   {
	       g_propagate_error(error, tmp_error);
	       return NULL;
	   }
           return c;
	   break;
#endif

       default :
	   g_set_error(error, TCP_ERROR, TCP_ERROR_NOTIMPLEMENTED, "NULL");
	   g_printerr("[TCP] tcp_host_addr: Socket type must be IPv4 or IPv6\n");
	   return NULL;
    }
}



/**
 * tcp_fd:
 * @s: sesion.
 * 
 * Esta funcion permite que el programa que usa esta libreria, pueda acceder
 * a los descriptores de fichero usados por los sockets para establecer los
 * parametros deseados (socketopt() y ioctl()).
 * 
 * Devuelve el descriptor de fichero del socket asociado a la sesion @s o -1
 * en otro caso.
 *
 **/

gint tcp_fd (Socket_tcp *s)
{
    if (s && s->fd > 0)
    {
	return s->fd;
    }
    return -1;
}



guint16 tcp_txport (Socket_tcp *s)
{
    return s->tx_port;
}



guint16 tcp_rxport (Socket_tcp *s)
{
    return s->rx_port;
}



gchar *tcp_addr (Socket_tcp *s)
{
    return g_strdup(s->addr);
}



