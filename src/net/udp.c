
/*
 *
 * FILE: udp.c
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
 * This file is based on 'net_udp.c'
 *
 * AUTHOR:   Colin Perkins 
 * MODIFIED: Orion Hodson & Piers O'Hanlon
 * 
 * Copyright (c) 1998-2000 University College London
 * All rights reserved.
 *
 ***************************************************************************/

#include "udp.h"
#include "net.h"



static fd_set rfd;
static fd_t max_fd;



// IPv4 specific functions ...

static Socket_udp *udp_init4 (const gchar *addr, const gchar *iface, guint16 rx_port, guint16 tx_port, gint ttl, GError **error)
{
    struct sockaddr_in s_in;
    struct in_addr iface_addr;
    Socket_udp *s;

#ifdef UDP_SETBUFFERSIZE
    gint udpbufsize = UDP_SETBUFFERSIZE;
#endif

#ifdef UDP_SOREUSEADDR
    gint reuse = 1;
#else
#ifdef UDP_SOREUSEPORT
    gint reuse = 1;
#endif
#endif


    s = (Socket_udp *) g_try_malloc(sizeof (Socket_udp));
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
	    g_set_error(error, UDP_ERROR, UDP_ERROR_UNRESOLVEIP, "can't resolve IP address for %s", addr);
	    g_printerr("[UDP] udp_init4: can't resolve IP address for %s\n", addr);
	    g_free(s);
	    return NULL;
	}
	memcpy(&(s->addr4), h->h_addr_list[0], sizeof(s->addr4));
    }

    if (iface != NULL)
    {
	if (inet_pton(AF_INET, iface, &iface_addr) != 1)
	{
	    g_set_error(error, UDP_ERROR, UDP_ERROR_NOIFACE, "illegal interface specification");
	    g_printerr("[UDP] udp_init4: illegal interface specification\n");
	    g_free(s);
	    return NULL;
	}
    } else iface_addr.s_addr = 0;

    s->fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (s->fd < 0)
    {
	g_set_error(error, UDP_ERROR, UDP_ERROR_ERRNO, "%s", g_strerror(errno));
	g_printerr("[UDP] udp_init4: %s\n", (*error)->message);
	g_free(s);
	return NULL;
    }

#ifdef UDP_SETBUFFERSIZE
    if (setsockopt(s->fd, SOL_SOCKET, SO_SNDBUF, (char *) &udpbufsize, sizeof(udpbufsize)) != 0)
    {
	g_set_error(error, UDP_ERROR, UDP_ERROR_ERRNO, "%s", g_strerror(errno));
	g_printerr("[UDP] udp_init4: %s\n", (*error)->message);
	g_free(s);
	return NULL;
    }
    if (setsockopt(s->fd, SOL_SOCKET, SO_RCVBUF, (char *) &udpbufsize, sizeof(udpbufsize)) != 0)
    {
	g_set_error(error, UDP_ERROR, UDP_ERROR_ERRNO, "%s", g_strerror(errno));
	g_printerr("[UDP] udp_init4: %s\n", (*error)->message);
	g_free(s);
	return NULL;
    }
#endif

#ifdef UDP_SOREUSEADDR
    if (setsockopt(s->fd, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse, sizeof(reuse)) != 0)
    {
	g_set_error(error, UDP_ERROR, UDP_ERROR_ERRNO, "%s", g_strerror(errno));
	g_printerr("[UDP] udp_init4: %s\n", (*error)->message);
	g_free(s);
	return NULL;
    }
#endif

#ifdef UDP_SOREUSEPORT
    if (setsockopt(s->fd, SOL_SOCKET, SO_REUSEPORT, (char *) &reuse, sizeof(reuse)) != 0)
    {
	g_set_error(error, UDP_ERROR, UDP_ERROR_ERRNO, "%s", g_strerror(errno));
	g_printerr("[UDP] udp_init4: %s\n", (*error)->message);
	g_free(s);
	return NULL;
    }
#endif

    s_in.sin_family = AF_INET;
    s_in.sin_addr.s_addr = INADDR_ANY;
    s_in.sin_port = g_htons(rx_port);

    if (bind(s->fd, (struct sockaddr *) &s_in, sizeof(s_in)) != 0)
    {
	g_set_error(error, UDP_ERROR, UDP_ERROR_ERRNO, "%s", g_strerror(errno));
	g_printerr("[UDP] udp_init4: %s\n", (*error)->message);
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
	    g_set_error(error, UDP_ERROR, UDP_ERROR_ERRNO, "%s", g_strerror(errno));
	    g_printerr("[UDP] udp_init4: %s\n", (*error)->message);
	    g_free(s);
	    return NULL;
	}
	if (setsockopt(s->fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) != 0)
	{
	    g_set_error(error, UDP_ERROR, UDP_ERROR_ERRNO, "%s", g_strerror(errno));
	    g_printerr("[UDP] udp_init4: %s\n", (*error)->message);
	    g_free(s);
	    return NULL;
	}
	if (setsockopt(s->fd, IPPROTO_IP, IP_MULTICAST_TTL, (char *) &s->ttl, sizeof(s->ttl)) != 0)
	{
	    g_set_error(error, UDP_ERROR, UDP_ERROR_ERRNO, "%s", g_strerror(errno));
	    g_printerr("[UDP] udp_init4: %s\n", (*error)->message);
	    g_free(s);
	    return NULL;
	}

	if (iface_addr.s_addr != 0)
	{
	    if (setsockopt(s->fd, IPPROTO_IP, IP_MULTICAST_IF, (char *) &iface_addr, sizeof(iface_addr)) != 0)
	    {
		g_set_error(error, UDP_ERROR, UDP_ERROR_ERRNO, "%s", g_strerror(errno));
		g_printerr("[UDP] udp_init4: %s\n", (*error)->message);
		g_free(s);
		return NULL;
	    }
	}
    }
    s->addr = g_strdup(addr);
    return s;
}



static void udp_exit4 (Socket_udp *s, GError **error)
{
    if (IN_MULTICAST(g_ntohl(s->addr4.s_addr)))
    {
	struct ip_mreq  imr;
	imr.imr_multiaddr.s_addr = s->addr4.s_addr;
	imr.imr_interface.s_addr = INADDR_ANY;

	if (setsockopt(s->fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *) &imr, sizeof(struct ip_mreq)) != 0)
	{
	    g_set_error(error, UDP_ERROR, UDP_ERROR_ERRNO, "%s", g_strerror(errno));
	    g_printerr("[UDP] udp_exit4: %s\n", (*error)->message);
	}
    }
    close(s->fd);
    g_free(s->addr);
    g_free(s);
    return;
}



static inline gint udp_send4 (Socket_udp *s, char *buffer, gint buflen)
{
    struct sockaddr_in	s_in;


    g_return_val_if_fail(s != NULL, -5);
    g_return_val_if_fail(s->mode == IPv4, -4);
    g_return_val_if_fail(buffer != NULL, -3);
    g_return_val_if_fail(buflen > 0, -2);

    s_in.sin_family = AF_INET;
    s_in.sin_addr.s_addr = s->addr4.s_addr;
    s_in.sin_port = g_htons(s->tx_port);
    return sendto(s->fd, buffer, buflen, 0, (struct sockaddr *) &s_in, sizeof(s_in));
}



static const gchar *udp_host_addr4 (GError **error)
{
    gchar *hname;
    struct hostent *hent;
    struct in_addr iaddr;


    hname = (gchar *) g_try_malloc(sizeof (gchar) * UDP_MAXHOSTNAMELEN);
    g_return_val_if_fail(hname != NULL, NULL);
    if (gethostname(hname, UDP_MAXHOSTNAMELEN) != 0)
    {
	g_set_error(error, UDP_ERROR, UDP_ERROR_ERRNO, "%s", g_strerror(errno));
	g_printerr("[UDP] udp_host_addr4: %s\n", (*error)->message);
	return NULL;
    }
    hent = gethostbyname(hname);
    if (hent == NULL)
    {
	g_set_error(error, UDP_ERROR, UDP_ERROR_UNRESOLVEIP, "can't resolve IP address for %s", hname);
	g_printerr("[UDP] udp_host_addr4 : can't resolve IP address for %s\n", hname);
	return NULL;
    }

    g_return_val_if_fail(hent->h_addrtype == AF_INET, NULL);
    memcpy(&iaddr.s_addr, hent->h_addr, sizeof(iaddr.s_addr));
    strncpy(hname, inet_ntoa(iaddr), UDP_MAXHOSTNAMELEN);
    return (const gchar *) hname;
}



#ifdef UDP_HAVE_IPv6

// IPv6 specific functions...

static Socket_udp *udp_init6 (const gchar *addr, const gchar *iface, guint16 rx_port, guint16 tx_port, gint ttl, GError **error)
{
    struct sockaddr_in6 s_in;
    Socket_udp *s;

#ifdef UDP_SETBUFFERSIZE
    gint udpbufsize = UDP_SETBUFFERSIZE;
#endif

#ifdef UDP_SOREUSEADDR
    gint reuse = 1;
#else
#ifdef UDP_SOREUSEPORT
    gint reuse = 1;
#endif
#endif


    if (iface != NULL)
    {
	g_printerr("[UDP] udp_init6: not implemented!");
	g_set_error(error, UDP_ERROR, UDP_ERROR_NOTIMPLEMENTED, "not implemented!");
	return NULL;
    }

    s = (Socket_udp *) g_try_malloc(sizeof(Socket_udp));
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
	g_set_error(error, UDP_ERROR, UDP_ERROR_ERRNO, "%s", g_strerror(errno));
	g_printerr("[UDP] udp_init6: %s\n", (*error)->message);
	g_free(s);
	return NULL;
    }

    s->fd = socket(AF_INET6, SOCK_DGRAM, 0);
    if (s->fd < 0)
    {
	g_set_error(error, UDP_ERROR, UDP_ERROR_ERRNO, "%s", g_strerror(errno));
	g_printerr("[UDP] udp_init6: %s\n", (*error)->message);
	g_free(s);
	return NULL;
    }

#ifdef UDP_SETBUFFERSIZE
    if (setsockopt(s->fd, SOL_SOCKET, SO_SNDBUF, (char *) &udpbufsize, sizeof(udpbufsize)) != 0)
    {
	g_set_error(error, UDP_ERROR, UDP_ERROR_ERRNO, "%s", g_strerror(errno));
	g_printerr("[UDP] udp_init6: %s\n", (*error)->message);
	g_free(s);
	return NULL;
    }
    if (setsockopt(s->fd, SOL_SOCKET, SO_RCVBUF, (char *) &udpbufsize, sizeof(udpbufsize)) != 0)
    {
	g_set_error(error, UDP_ERROR, UDP_ERROR_ERRNO, "%s", g_strerror(errno));
	g_printerr("[UDP] udp_init6: %s\n", (*error)->message);
	g_free(s);
	return NULL;
    }
#endif

#ifdef UDP_SOREUSEADDR
    if (setsockopt(s->fd, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse, sizeof(reuse)) != 0)
    {
	g_set_error(error, UDP_ERROR, UDP_ERROR_ERRNO, "%s", g_strerror(errno));
	g_printerr("[UDP] udp_init6: %s\n", (*error)->message);
	g_free(s);
	return NULL;
    }
#endif

#ifdef UDP_SOREUSEPORT
    if (setsockopt(s->fd, SOL_SOCKET, SO_REUSEPORT, (char *) &reuse, sizeof(reuse)) != 0)
    {
	g_set_error(error, UDP_ERROR, UDP_ERROR_ERRNO, "%s", g_strerror(errno));
	g_printerr("[UDP] udp_init6: %s\n", (*error)->message);
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
	g_set_error(error, UDP_ERROR, UDP_ERROR_ERRNO, "%s", g_strerror(errno));
	g_printerr("[UDP] udp_init6: %s\n", (*error)->message);
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
	    g_set_error(error, UDP_ERROR, UDP_ERROR_ERRNO, "%s", g_strerror(errno));
	    g_printerr("[UDP] udp_init6: %s\n", (*error)->message);
	    g_free(s);
	    return NULL;
	}
	if (setsockopt(s->fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, (char *) &loop, sizeof(loop)) != 0)
	{
	    g_set_error(error, UDP_ERROR, UDP_ERROR_ERRNO, "%s", g_strerror(errno));
	    g_printerr("[UDP] udp_init6: %s\n", (*error)->message);
	    g_free(s);
	    return NULL;
	}
	if (setsockopt(s->fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, (char *) &ttl, sizeof(ttl)) != 0)
	{
	    g_set_error(error, UDP_ERROR, UDP_ERROR_ERRNO, "%s", g_strerror(errno));
	    g_printerr("[UDP] udp_init6: %s\n", (*error)->message);
	    g_free(s);
	    return NULL;
	}
    }
    s->addr = g_strdup(addr);
    return s;
}



static void udp_exit6 (Socket_udp *s, GError **error)
{
    if (IN6_IS_ADDR_MULTICAST(&(s->addr6)))
    {
	struct ipv6_mreq  imr;

	imr.ipv6mr_multiaddr = s->addr6;
	imr.ipv6mr_interface = 0;

	if (setsockopt(s->fd, IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP, (char *) &imr, sizeof(struct ipv6_mreq)) != 0)
	{
	    g_set_error(error, UDP_ERROR, UDP_ERROR_ERRNO, "%s", g_strerror(errno));
	    g_printerr("[UDP] udp_exit6: %s\n", (*error)->message);
	}
    }
    close(s->fd);
    g_free(s->addr);
    g_free(s);
}



static inline gint udp_send6 (Socket_udp *s, char *buffer, gint buflen)
{
    struct sockaddr_in6	s_in;

    g_return_val_if_fail(s != NULL, -5);
    g_return_val_if_fail(s->mode == IPv4, -4);
    g_return_val_if_fail(buffer != NULL, -3);
    g_return_val_if_fail(buflen > 0, -2);

    s_in.sin6_family = AF_INET6;
    s_in.sin6_addr = s->addr6;
    s_in.sin6_port = g_htons(s->tx_port);
    memset((char *)&s_in, 0, sizeof(s_in));

#ifdef HAVE_SIN6_LEN
    s_in.sin6_len = sizeof(s_in);
#endif

    return sendto(s->fd, buffer, buflen, 0, (struct sockaddr *) &s_in, sizeof(s_in));
}



static const gchar *udp_host_addr6 (Socket_udp *s, GError **error)
{
    gchar *hname;
    gint newsock;
    struct addrinfo hints, *ai;
    struct sockaddr_in6 local, addr6;
    gint result;
    gint len;


    hname = (gchar *) g_try_malloc(sizeof (gchar) * UDP_MAXHOSTNAMELEN);
    g_return_val_if_fail(hname != NULL, NULL);
    len = sizeof(struct sockaddr_in6);
    newsock = socket(AF_INET6, SOCK_DGRAM, 0);
    memset ((char *)&addr6, 0, sizeof(struct sockaddr_in6));
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
	g_print("[UDP] udp_host_addr6: getsockname failed (IPv6)\n");
    }
    close (newsock);

    if (IN6_IS_ADDR_UNSPECIFIED(&local.sin6_addr) || IN6_IS_ADDR_MULTICAST(&local.sin6_addr))
    {
	if (gethostname(hname, UDP_MAXHOSTNAMELEN) != 0)
	{
	    g_set_error(error, UDP_ERROR, UDP_ERROR_ERRNO, "%s", g_strerror(errno));
	    g_printerr("[UDP] udp_host_addr6: %s\n", (*error)->message);
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
	    g_set_error(error, UDP_ERROR, UDP_ERROR_ERRNO, "getaddrinfo: %s: %s", hname, gai_strerror(result));
	    g_printerr("[UDP] udp_host_addr6: %s", hname, (*error)->message);
	    return NULL;
	}
	if (inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)(ai->ai_addr))->sin6_addr), hname, UDP_MAXHOSTNAMELEN) == NULL)
	{
	    freeaddrinfo(ai);
	    g_set_error(error, UDP_ERROR, UDP_ERROR_ERRNO, "%s (inet_ntop: %s)", g_strerror(errno), hname);
	    g_printerr("[UDP] udp_host_addr6: %s", (*error)->message);
	    return NULL;
	}
	freeaddrinfo(ai);
	return (const gchar *) hname;
    }
    if (inet_ntop(AF_INET6, &local.sin6_addr, hname, UDP_MAXHOSTNAMELEN) == NULL)
    {
	g_set_error(error, UDP_ERROR, UDP_ERROR_ERRNO, "%s (inet_ntop: %s)", g_strerror(errno), hname);
	g_printerr("[UDP] udp_host_addr6: %s", (*error)->message);
	return NULL;
    }
    return (const gchar *) hname;
}


#endif  //IPv6



// Funciones genericas, que abstraen la version del protocolo usado:
// IPv4 ó IPv6



/**
 * udp_init:
 * @addr: cadena de caracteres que representa una direccion de red, tanto
 * en IPv4 como en IPv6.
 * @iface: cadena de caracteres que representa un nombre de interfaz o NULL
 * para seleccionar la interfaz por defecto.
 * @rx_port: puerto de recepcion.
 * @tx_port: puerto de transmision.
 * @ttl: valor time-to-live para los paquetes transmitidos.
 * @error: doble puntero a una estructura GError.
 *
 * Crea una sesion para enviar y recibir paquetes UDP sobre redes IP.
 * La sesion usa @iface como el interfaz para enviar y recibir los paquetes.
 * Si @iface es NULL, entonces se usara la interfaz por defecto
 * 
 * Retorna un puntero a una estructura Socket_udp si la funcion tuvo exito, en
 * otro caso devuelve NULL y rellena la estructura @error con el tipo/codigo
 * de error generado.
 *
 **/

Socket_udp *udp_init (const gchar *addr, const gchar *iface, guint16 rx_port, guint16 tx_port, gint ttl, GError **error)
{
    GError *tmp_error = NULL;
    Socket_udp *session_udp = NULL;

    if (strchr(addr, ':') == NULL)
    {
	session_udp = udp_init4(addr, iface, rx_port, tx_port, ttl, &tmp_error);
	if (session_udp == NULL)
	{
	    g_propagate_error (error, tmp_error);
	    return NULL;
	}
    }
    else
    {

#ifdef UDP_HAVE_IPv6
	session_udp = udp_init6(addr, iface, rx_port, tx_port, ttl, &tmp_error);
	if (session_udp == NULL)
	{
	    g_propagate_error (error, tmp_error);
	    return NULL;
	}
#else
	return NULL;
#endif

    }
    return session_udp;
}



/**
 * udp_exit:
 * @s: sesion a terminar.
 * @error: doble puntero a una estructura GError.
 *
 * Termina una sesion iniciada con udp_init.
 * 
 **/

void udp_exit (Socket_udp *s, GError **error)
{
    GError *tmp_error = NULL;


    if (s == NULL)
    {
	g_set_error(error, UDP_ERROR, UDP_ERROR_NULL, "NULL");
	g_printerr("[UDP] udp_exit: Socket_udp = NULL\n");
	return;
    }
    switch(s->mode)
    {
       case IPv4 :
	   udp_exit4(s, &tmp_error);
	   if (tmp_error != NULL)
	   {
	       g_propagate_error(error, tmp_error);
	       return;
	   }
	   break;

#ifdef UDP_HAVE_IPv6
       case IPv6 :
	   udp_exit6(s, &tmp_error);
	   if (tmp_error != NULL)
	   {
	       g_propagate_error(error, tmp_error);
	       return;
	   }
	   break;
#endif

       default :
	   g_set_error(error, UDP_ERROR, UDP_ERROR_NOTIMPLEMENTED, "NULL");
	   g_printerr("[UDP] udp_exit: Socket type must be IPv4 or IPv6\n");
	   return;
    }
}



/**
 * udp_send:
 * @s: sesion.
 * @buffer: puntero al buffer cuyo contenido va a ser transmitido.
 * @buflen: tamaño de @buffer.
 * 
 * Transmite un datagrama UDP con el contenido del @buffer.
 * 
 * Valor retornado: Numero de bytes enviados si todo va bien o un codigo de
 * error negativo en otro caso.
 *
 **/

gint udp_send (Socket_udp *s, char *buffer, gint buflen)
{
    if (s->mode == IPv4)
    {
	return udp_send4(s, buffer, buflen);
    }
    else
    {

#ifdef UDP_HAVE_IPv6
        return udp_send6(s, buffer, buflen);
#else
        return -1;
#endif

    }
}


/**
 * udp_recv:
 * @s: sesion.
 * @buffer: buffer donde recoger los datos recibidos.
 * @buflen: tamaño de @buffer.
 * 
 * Lee los datos de la cola de entrada (socket) y los coloca en @buffer.
 *
 * Retorna el numero de bytes recibidos o un numero negativo que indica
 * el codigo de error.
 *
 **/

inline gint udp_recv (Socket_udp *s, char *buffer, gint buflen)
{
    /* Reads data into the buffer, returning the number of bytes read.   */
    /* If no data is available, this returns the value zero immediately. */
    /* Note: since we don't care about the source address of the packet  */
    /* we receive, this function becomes protocol independent.           */
    gint len;

    g_return_val_if_fail(s != NULL, -5);
    g_return_val_if_fail(buffer != NULL, -3);
    g_return_val_if_fail(buflen > 0, -2);

    len = recvfrom(s->fd, buffer, buflen, 0, 0, 0);
    g_return_val_if_fail(buflen > 0, len);
    return len;
}



/**
 * udp_fd_zero:
 * 
 * Clears file descriptor from set associated with UDP sessions (see select(2)).
 * Borra el descriptor de fichero asociado con la session
 * 
 **/

void udp_fd_zero (void)
{
    FD_ZERO(&rfd);
    max_fd = 0;
}



/**
 * udp_fd_set:
 * @s: sesion.
 * 
 * Adds file descriptor associated of @s to set associated with UDP sessions.
 * Añade el descriptor de fichero asociado a @s ...
 *
 **/

void udp_fd_set (Socket_udp *s)
{
    FD_SET(s->fd, &rfd);
    if (s->fd > (fd_t)max_fd)
    {
	max_fd = s->fd;
    }
}



/**
 * udp_fd_isset:
 * @s: sesion.
 * 
 * Chequea si el descriptor de fichero asociado a la sesion esta listo
 * para leer. Esta funcion debe ser llamada despues de udp_select().
 *
 * Retorna un valor distinto de cero is esta "set" o cero en otro caso.
 *
 **/

gint udp_fd_isset (Socket_udp *s)
{
    return FD_ISSET(s->fd, &rfd);
}



/**
 * udp_select:
 * @timeout: tiempo maximo de espera para la llegada de los datos.
 *
 * Espera a la llegada de los datos a la sesion.
 * 
 * Devuelve el numero de descriptores de la sesion listos para ser leidos.
 *
 **/

gint udp_select (struct timeval *timeout)
{
    return select(max_fd + 1, &rfd, NULL, NULL, timeout);
}



/**
 * udp_host_addr:
 * @s: sesion.
 * @error: doble puntero a una estructura GError.
 *
 * Retorna un puntero a un string que contiene la direccion asociada a
 * la sesion @s; si ocurrio un error, NULL y rellena la estructura @error.
 *
 **/

const gchar *udp_host_addr (Socket_udp *s, GError **error)
{
    GError *tmp_error = NULL;
    const gchar* c;


    if (s == NULL)
    {
	g_set_error(error, UDP_ERROR, UDP_ERROR_NULL, "NULL");
	g_printerr("[UDP] udp_host_addr: Socket_udp = NULL\n");
	return NULL;
    }
    switch(s->mode)
    {
       case IPv4 :
           c = udp_host_addr4(&tmp_error);
	   if (tmp_error != NULL)
	   {
	       g_propagate_error(error, tmp_error);
	       return NULL;
	   }
	   return c;
	   break;

#ifdef UDP_HAVE_IPv6
       case IPv6 :
	   c = udp_host_addr6(s, &tmp_error);
	   if (tmp_error != NULL)
	   {
	       g_propagate_error(error, tmp_error);
	       return NULL;
	   }
           return c;
	   break;
#endif

       default :
	   g_set_error(error, UDP_ERROR, UDP_ERROR_NOTIMPLEMENTED, "NULL");
	   g_printerr("[UDP] udp_host_addr: Socket type must be IPv4 or IPv6\n");
	   return NULL;
    }
}



/**
 * udp_fd:
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

gint udp_fd (Socket_udp *s)
{
    if (s && s->fd > 0)
    {
	return s->fd;
    }
    return -1;
}


