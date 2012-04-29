
/*
 *
 * FILE: rtp_private.c
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

#include "rtp_private.h"



inline void ntp64_time (guint32 *ntp_sec, guint32 *ntp_frac)
{
    struct timeval now;

    gettimeofday(&now, NULL);

    /* NB ntp_frac is in units of 1 / (2^32 - 1) secs. */
    *ntp_sec  = now.tv_sec + SECS_BETWEEN_1900_1970;
    *ntp_frac = (now.tv_usec << 12) + (now.tv_usec << 8) - ((now.tv_usec * 3650) >> 6);
    return;
}



/* Return curr_time - prev_time */

inline gdouble diff_timeval (struct timeval curr_time, struct timeval prev_time)
{
    gdouble ct, pt;

    ct = (gdouble) curr_time.tv_sec + (((gdouble) curr_time.tv_usec) / 1000000.0);
    pt = (gdouble) prev_time.tv_sec + (((gdouble) prev_time.tv_usec) / 1000000.0);
    return (ct - pt);
}



/* Add offset seconds to ts */

inline void add_timeval (struct timeval *ts, gdouble offset)
{
    gdouble offset_sec, offset_usec;

    offset_usec = modf(offset, &offset_sec) * 1000000;
    ts->tv_sec  += (long) offset_sec;
    ts->tv_usec += (long) offset_usec;
    if (ts->tv_usec > 1000000)
    {
	ts->tv_sec++;
	ts->tv_usec -= 1000000;
    }
    return;
}



/* Returns (a>b) */

inline gint gt_timeval (struct timeval a, struct timeval b)
{
    if (a.tv_sec > b.tv_sec) return TRUE;
    if (a.tv_sec <= b.tv_sec) return FALSE;

    return (a.tv_usec > b.tv_usec);
}



/* Set the CNAME. This is "user@hostname" or just "hostname" if the username cannot be found. */

gchar *get_cname (Socket_udp *s)
{
    const gchar *hname;
    G_CONST_RETURN gchar *auxname;
    gchar *cnameaux;
    gchar *cname;

    auxname = g_get_user_name();
    cnameaux = g_strconcat(auxname, "@", NULL);
    /* Now the hostname. Must be dotted-quad IP address. */
    hname = udp_host_addr(s, NULL);
    if (hname == NULL)
    {
	/* If we can't get our IP address we use the loopback address... */
	/* This is horrible, but it stops the code from failing.         */
        hname = g_strdup("127.0.0.1");
    }
    cname = g_strconcat(cnameaux, hname, NULL);
    g_free(cnameaux);
    return cname;
}



void init_rng (const gchar *s)
{
    static guint32 seed;


    if (s == NULL)
    {
	/* This should never happen, but just in case */
        s = RTP_INIT_SEED_STRING;
    }
    if (seed == 0)
    {
	pid_t p = getpid();
	while (*s)
	{
	    seed += (guint32)*s++;
	    seed = seed * 31 + 1;
	}
	seed = 1 + seed * 31 + (guint32)p;
	srand48(seed);
	/* At time of writing we use srand48 -> srand on Win32
	 which is only 16 bit. lrand48 -> rand which is only
	 15 bits, step a long way through table seq */
    }
    return;
}


