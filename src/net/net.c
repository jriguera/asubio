
/*
 *
 * FILE: net.c
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

#include "net.h"



static gboolean net_addr_valid4 (const gchar *dst, GError **error)
{
    struct in_addr addr4;
    struct hostent *h;


    if (inet_pton(AF_INET, dst, &addr4)) return TRUE;
    h = gethostbyname(dst);
    if (h != NULL) return TRUE;

    g_set_error(error, NET_ERROR, NET_ERROR_UNRESOLVEIP, "can't resolve IP address for %s", dst);
    g_printerr("[NET] net_addr_valid4: can't resolve IP address for %s\n", dst);
    return FALSE;
}



#ifdef NET_HAVE_IPv6

static gboolean net_addr_valid6 (const gchar *dst, GError **error)
{
    struct in6_addr addr6;


    switch (inet_pton(AF_INET6, dst, &addr6))
    {
        case 1: return TRUE;
        case 0: return FALSE;
        case -1: 
	    g_set_error(error, NET_ERROR, NET_ERROR_ERRNO, "%s", g_strerror(errno));
	    g_printerr("[NET] net_addr_valid6: can't resolve IP address for %s (%s)\n", dst, (*error)->message);
	    return FALSE;
    }
    return FALSE;
}

#endif



/**
 * udp_addr_valid:
 * @addr: cadena de caracteres que representa una direccion de red, tanto
 * en IPv4 como en IPv6.
 * @error: doble puntero a una estructura GError.
 *
 * Retorna TRUE si @addr es una direccion correcta, FALSE en otro caso y
 * rellena la estructura @error con el tipo/codigo de error generado.
 *
 **/

gboolean net_addr_valid(const char *addr, GError **error)
{
    gboolean result;
    GError *tmp_error = NULL;


    result = net_addr_valid4(addr, &tmp_error);
    if (tmp_error != NULL)
    {
	g_propagate_error(error, tmp_error);
	return FALSE;
    }

#ifdef NET_HAVE_IPv6
    if (! result)
    {
	result = net_addr_valid6(addr, &tmp_error);
	if (tmp_error != NULL)
	{
	    g_propagate_error(error, tmp_error);
	    return FALSE;
	}
    }
#endif

    return result;
}


