
/*
 *
 * FILE: rtp.c
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
 * This file is based on:
 *
 * AUTHOR:   Colin Perkins   <csp@isi.edu>
 * MODIFIED: Orion Hodson    <o.hodson@cs.ucl.ac.uk>
 *           Markus Germeier <mager@tzi.de>
 *           Bill Fenner     <fenner@research.att.com>
 *           Timur Friedman  <timur@research.att.com>
 *           Jose Riguera    <jriguera@gmail.com>
 *
 * The routines in this file implement the Real-time Transport Protocol,
 * RTP, as specified in RFC1889 with current updates under discussion in
 * the IETF audio/video transport working group. Portions of the code are
 * derived from the algorithms published in that specification.
 *
 * Copyright (c) 1998-2001 University College London
 * All rights reserved.
 *
 ***************************************************************************/

#include "rtp_private.h"
#include <stdio.h>




/* Minimum average time between RTCP packets from this site (in   */
/* seconds).  This time prevents the reports from `clumping' when */
/* sessions are small and the law of large numbers isn't helping  */
/* to smooth out the traffic.  It also keeps the report interval  */
/* from becoming ridiculously small during transient outages like */
/* a network partition.                                           */
gdouble const RTCP_MIN_TIME = 5.0;

/* Fraction of the RTCP bandwidth to be shared among active       */
/* senders.  (This fraction was chosen so that in a typical       */
/* session with one or two active senders, the computed report    */
/* time would be roughly equal to the minimum report time so that */
/* we don't unnecessarily slow down receiver reports.) The        */
/* receiver fraction must be 1 - the sender fraction.             */
gdouble const RTCP_SENDER_BW_FRACTION = 0.25;

/* To compensate for "unconditional reconsideration" converging   */
/* to a value below the intended average.                         */
gdouble const COMPENSATION = 2.71828 - 1.5;




/* Funciones Auxiliares */



/* Filtra los paquetes que envia el cliente, es decir, los que coinciden con el ssrc */

static inline gint filter_event (RtpSession *session, guint32 ssrc)
{
	return session->opt->filter_my_packets && (ssrc == rtp_my_ssrc(session));
}


/* This returns each Source marked "should_advertise_sdes" in turn. */
/* Devuelve cada source marcado como "should_advertise_sdes" por turno */

static guint32 next_csrc (RtpSession *session)
{
    gint chain;
    Source *s;
    gint cc = 0;


    for (chain = 0; chain < RTP_DB_SIZE; chain++)
    {
	/* Check that the linked lists making up the chains in */
	/* the hash table are correctly linked together...     */
	for (s = session->db[chain]; s != NULL; s = s->next)
	{
	    if (s->should_advertise_sdes)
	    {
		if (cc == session->last_advertised_csrc)
		{
		    session->last_advertised_csrc++;
		    if (session->last_advertised_csrc == session->csrc_count) session->last_advertised_csrc = 0;
		    return s->ssrc;
		}
		else cc++;
	    }
	}
    }
    /* We should never get here... */
    g_print("[RTP] Critical error in function 'next_csrc', (should_advertise_sdes)\n");
    g_printerr("[RTP] Critical error in function 'next_csrc', (should_advertise_sdes)\n");
    fflush(NULL);
    return 0;
}


/* Hash from an ssrc to a position in the Source database.   */
/* Assumes that ssrc values are uniformly distributed, which */
/* should be true but probably isn't (Rosenberg has reported */
/* that many implementations generate ssrc values which are  */
/* not uniformly distributed over the space, and the H.323   */
/* spec requires that they are non-uniformly distributed).   */
/* This routine is written as a function rather than inline  */
/* code to allow it to be made smart in future: probably we  */
/* should run MD5 on the ssrc and derive a hash value from   */
/* that, to ensure it's more uniformly distributed?          */

/* Calcula la posicion de la BD "source" mediante una funcion*/
/* aplicada al "ssrc"                                        */

static gint ssrc_hash (guint32 ssrc)
{
    return (ssrc % RTP_DB_SIZE);
}


/* Insert the reception report into the receiver report      */
/* database. This database is a two dimensional table of     */
/* rr_wrappers indexed by hashes of reporter_ssrc and        */
/* reportee_src.  The rr_wrappers in the database are        */
/* sentinels to reduce conditions in list operations.        */
/* The ts is used to determine when to timeout this rr.      */

/* Inserta "reporter_ssrc" en la tabla (BD) de reception     */

static void insert_rr (RtpSession *session, guint32 reporter_ssrc, rtcp_rr *rr, struct timeval *ts)
{
    rtcp_rr_wrapper *cur, *start;


    start = &session->rr[ssrc_hash(reporter_ssrc)][ssrc_hash(rr->ssrc)];
    cur   = start->next;
    while (cur != start)
    {
	if (cur->reporter_ssrc == reporter_ssrc && cur->rr->ssrc == rr->ssrc)
	{
	    /* Replace existing entry in the database  */
	    g_free(cur->rr);
	    g_free(cur->ts);
	    cur->rr = rr;
	    cur->ts = (struct timeval *) g_malloc(sizeof(struct timeval));
	    memcpy(cur->ts, ts, sizeof(struct timeval));
	    return;
	}
	cur = cur->next;
    }
    /* No entry in the database so create one now. */
    cur = (rtcp_rr_wrapper*) g_malloc(sizeof(rtcp_rr_wrapper));
    cur->reporter_ssrc = reporter_ssrc;
    cur->rr = rr;
    cur->ts = (struct timeval *) g_malloc(sizeof(struct timeval));
    memcpy(cur->ts, ts, sizeof(struct timeval));
    cur->next       = start->next;
    cur->next->prev = cur;
    cur->prev       = start;
    cur->prev->next = cur;

    g_print("[RTP] Created new RR entry for 0x%08x from Source 0x%08x\n", rr->ssrc, reporter_ssrc);
    return;
}


/* Remove any RRs from "s" which refer to "ssrc" as either   */
/* reporter or reportee.                                     */

/* Borra de la BD todos "ssrc"                               */

static void remove_rr (RtpSession *session, guint32 ssrc)
{
    rtcp_rr_wrapper *start, *cur, *tmp;
    gint i;

    /* Remove rows, i.e. ssrc == reporter_ssrc               */
    for(i = 0; i < RTP_DB_SIZE; i++)
    {
	start = &session->rr[ssrc_hash(ssrc)][i];
	cur   = start->next;
	while (cur != start)
	{
	    if (cur->reporter_ssrc == ssrc)
	    {
		tmp = cur;
		cur = cur->prev;
		tmp->prev->next = tmp->next;
		tmp->next->prev = tmp->prev;
		g_free(tmp->ts);
		g_free(tmp->rr);
		g_free(tmp);
	    }
	    cur = cur->next;
	}
    }
    /* Remove columns, i.e.  ssrc == reporter_ssrc */
    for(i = 0; i < RTP_DB_SIZE; i++)
    {
	start = &session->rr[i][ssrc_hash(ssrc)];
	cur   = start->next;
	while (cur != start)
	{
	    if (cur->rr->ssrc == ssrc)
	    {
		tmp = cur;
		cur = cur->prev;
		tmp->prev->next = tmp->next;
		tmp->next->prev = tmp->prev;
		g_free(tmp->ts);
		g_free(tmp->rr);
		g_free(tmp);
	    }
	    cur = cur->next;
	}
    }
    return;
}


/* Timeout any reception reports which have been in the database for more than 3 */
/* times the RTCP reporting interval without refresh.                            */

/* Salta el "timeout" de todos los rtcp recibidos que estan el la BD mas de 3    */
/* sin refrescar (timeout).                                                      */

static void timeout_rr (RtpSession *session, struct timeval *curr_ts)
{
    rtcp_rr_wrapper *start, *cur, *tmp;
    RtpEvent event;
    gint i, j;


    for(i = 0; i < RTP_DB_SIZE; i++)
    {
	for(j = 0; j < RTP_DB_SIZE; j++)
	{
	    start = &session->rr[i][j];
	    cur = start->next;
	    while (cur != start)
	    {
		if (diff_timeval(*curr_ts, *(cur->ts)) > (session->rtcp_interval * 3))
		{
		    /* Signal the application... */
		    if (!filter_event(session, cur->reporter_ssrc))
		    {
			event.ssrc = cur->reporter_ssrc;
			event.type = RR_TIMEOUT;
			event.data = cur->rr;
			event.ts = curr_ts;
			session->callback_rtcp(session, &event);
		    }
		    /* Delete this reception report... */
		    tmp = cur;
		    cur = cur->prev;
		    tmp->prev->next = tmp->next;
		    tmp->next->prev = tmp->prev;
		    g_free(tmp->ts);
		    g_free(tmp->rr);
		    g_free(tmp);
		}
		cur = cur->next;
	    }
	}
    }
    return;
}


/* Devuelve un "rtcp_rr" del "reporter_ssrc" que coincide con "reporter_ssrc"  */

static const rtcp_rr* get_rr (RtpSession *session, guint32 reporter_ssrc, guint32 reportee_ssrc)
{
    rtcp_rr_wrapper *cur, *start;

    start = &session->rr[ssrc_hash(reporter_ssrc)][ssrc_hash(reportee_ssrc)];
    cur   = start->next;
    while (cur != start)
    {
	if (cur->reporter_ssrc == reporter_ssrc && cur->rr->ssrc == reportee_ssrc) return cur->rr;
	cur = cur->next;
    }
    return NULL;
}


/* Devuelve un "source" de la BD que coincide con ssrc     */

static inline Source *get_source (RtpSession *session, guint32 ssrc)
{
    Source *s;

    for (s = session->db[ssrc_hash(ssrc)]; s != NULL; s = s->next)
    {
	if (s->ssrc == ssrc) return s;
    }
    return NULL;
}


/* Create a new Source entry, and add it to the database.    */
/* The database is a hash table, using the separate chaining */
/* algorithm.                                                */

/* Crea una nueva entrada "source" y la añade a la BD,       */
/* tabla hash                                                */

static Source *create_source (RtpSession *session, guint32 ssrc, gboolean probation)
{
    RtpEvent event;
    struct timeval event_ts;
    Source *s;
    gint h;


    s = get_source(session, ssrc);
    if (s != NULL)
    {
	/* Source is already in the database... Mark it as */
	/* active and exit (this is the common case...)    */
	gettimeofday(&(s->last_active), NULL);
	return s;
    }
    /* This is a new Source, we have to create it... */
    h = ssrc_hash(ssrc);
    s = (Source *) g_malloc0(sizeof(Source));
    s->next = session->db[h];
    s->ssrc = ssrc;

    s->cname = NULL;
    s->name = NULL;
    s->email = NULL;
    s->phone = NULL;
    s->loc = NULL;
    s->tool = NULL;
    s->note = NULL;

    /* This is a probationary Source, which only counts as */
    /* valid once several consecutive packets are received */
    if (probation) s->probation = -1;
    else s->probation = 0;

    gettimeofday(&(s->last_active), NULL);
    /* Now, add it to the database... */
    if (session->db[h] != NULL) session->db[h]->prev = s;
    session->db[ssrc_hash(ssrc)] = s;
    session->ssrc_count++;

    g_print("[RTP] Created database entry for ssrc 0x%08x (%d valid Sources)\n", ssrc, session->ssrc_count);
    if (ssrc != session->my_ssrc)
    {
	/* Do not send during rtp_init since application cannot map the address */
	/* of the rtp session to anything since rtp_init has not returned yet.  */
	if (!filter_event(session, ssrc))
	{
	    gettimeofday(&event_ts, NULL);
	    event.ssrc = ssrc;
	    event.type = SOURCE_CREATED;
	    event.data = NULL;
	    event.ts = &event_ts;
	    session->callback_rtcp(session, &event);
	}
    }
    return s;
}


/* Remove a Source from the RTP database ...                         */

/* Borra un "source" de la BD, devuelve TRUE si se ha podido borrar  */
/* el "source" que coincide con 2ssrc" o FALSE en otro caso.         */

static gboolean delete_source (RtpSession *session, guint32 ssrc)
{
    Source *s;
    gint h;
    RtpEvent event;
    struct timeval event_ts;


    s = get_source(session, ssrc);
    h = ssrc_hash(ssrc);
    g_return_val_if_fail(s != NULL, FALSE);
    gettimeofday(&event_ts, NULL);

    if (session->db[h] == s)
    {
	/* It's the first entry in this chain... */
	session->db[h] = s->next;
	if (s->next != NULL) s->next->prev = NULL;
    }
    else
    {
	g_return_val_if_fail(s->prev != NULL, FALSE); 	/* Else it would be the first in the chain... */
	s->prev->next = s->next;
	if (s->next != NULL) s->next->prev = s->prev;
    }
    /* Free the memory allocated to a Source... */
    if (s->cname != NULL) g_free(s->cname);
    if (s->name != NULL) g_free(s->name);
    if (s->email != NULL) g_free(s->email);
    if (s->phone != NULL) g_free(s->phone);
    if (s->loc != NULL) g_free(s->loc);
    if (s->tool != NULL) g_free(s->tool);
    if (s->note != NULL) g_free(s->note);
    if (s->priv != NULL) g_free(s->priv);
    if (s->sr != NULL) g_free(s->sr);

    remove_rr(session, ssrc);
    /* Reduce our SSRC count, and perform reverse reconsideration on the RTCP */
    /* reporting interval (draft-ietf-avt-rtp-new-05.txt, section 6.3.4). To  */
    /* make the transmission rate of RTCP packets more adaptive to changes in */
    /* group membership, the following "reverse reconsideration" algorithm    */
    /* SHOULD be executed when a BYE packet is received that reduces members  */
    /* to a value less than pmembers:                                         */
    /* o  The value for tn is updated according to the following formula:     */
    /*       tn = tc + (members/pmembers)(tn - tc)                            */
    /* o  The value for tp is updated according the following formula:        */
    /*       tp = tc - (members/pmembers)(tc - tp).                           */
    /* o  The next RTCP packet is rescheduled for transmission at time tn,    */
    /*    which is now earlier.                                               */
    /* o  The value of pmembers is set equal to members.                      */
    session->ssrc_count--;
    if (session->ssrc_count < session->ssrc_count_prev)
    {
	gettimeofday(&(session->next_rtcp_send_time), NULL);
	gettimeofday(&(session->last_rtcp_send_time), NULL);
	add_timeval(&(session->next_rtcp_send_time), (session->ssrc_count / session->ssrc_count_prev) * diff_timeval(session->next_rtcp_send_time, event_ts));
	add_timeval(&(session->last_rtcp_send_time), - ((session->ssrc_count / session->ssrc_count_prev) * diff_timeval(event_ts, session->last_rtcp_send_time)));
	session->ssrc_count_prev = session->ssrc_count;
    }
    /* Reduce our csrc count... */
    if (s->should_advertise_sdes == TRUE) session->csrc_count--;
    if (session->last_advertised_csrc == session->csrc_count) session->last_advertised_csrc = 0;

    /* Signal to the application that this source is dead... */
    if (!filter_event(session, ssrc))
    {
	event.ssrc = ssrc;
	event.type = SOURCE_DELETED;
	event.data = NULL;
	event.ts = &event_ts;
	session->callback_rtcp(session, &event);
    }
    g_free(s);
    return TRUE;
}


/* Inicializa todos los valores necesarios para la transmision rtp   */
/* conforme al borrador draft-ietf-avt-rtp-new-01.txt.               */

static inline void init_seq (Source *s, guint16 seq)
{
    /* Taken from draft-ietf-avt-rtp-new-01.txt */
    s->base_seq = seq;
    s->max_seq = seq;
    s->bad_seq = RTP_SEQ_MOD + 1;
    s->cycles = 0;
    s->received = 0;
    s->received_prior = 0;
    s->expected_prior = 0;
}


/* Actualiza los parametros (paquetes enviados, numeros de secuencia, etc)  */
/* de "s" segun el borrador draft-ietf-avt-rtp-new-01.txt                   */

static gboolean update_seq (Source *s, guint16 seq)
{
    /* Taken from draft-ietf-avt-rtp-new-01.txt */
    guint16 udelta = seq - s->max_seq;


    /* Source is not valid until RTP_MIN_SEQUENTIAL packets with  */
    /* sequential sequence numbers have been received.        */
    if (s->probation)
    {
	/* packet is in sequence */
	if (seq == s->max_seq + 1)
	{
	    s->probation--;
	    s->max_seq = seq;
	    if (s->probation == 0)
	    {
		init_seq(s, seq);
		s->received++;
		return TRUE;
	    }
	}
	else
	{
	    s->probation = RTP_MIN_SEQUENTIAL - 1;
	    s->max_seq = seq;
	}
	return FALSE;
    }
    else
    {
	if (udelta < RTP_MAX_DROPOUT)
	{
	    /* in order, with permissible gap */
	    if (seq < s->max_seq)
	    {
		/* Sequence number wrapped - count another 64K cycle. */
		s->cycles += RTP_SEQ_MOD;
	    }
	    s->max_seq = seq;
	}
	else
	{
	    if (udelta <= RTP_SEQ_MOD - RTP_MAX_MISORDER)
	    {
		/* the sequence number made a very large jump */
		if (seq == s->bad_seq)
		{
		    /* Two sequential packets -- assume that the other side
		     restarted without telling us so just re-sync
		     (i.e., pretend this was the first packet). */
		    init_seq(s, seq);
		}
		else
		{
		    s->bad_seq = (seq + 1) & (RTP_SEQ_MOD-1);
		    return FALSE;
		}
	    }
	    else
	    {
		/* duplicate or reordered packet */
                //return FALSE;
	    }
	}
    }
    s->received++;
    return TRUE;
}


/* Calcula en intervalo (segun el numero de participantes) en el      */
/* que se deben enviar los paquetes rtcp, segun el estandar           */

static inline gdouble rtcp_interval (RtpSession *session)
{
    gdouble t; /* interval */
    gdouble rtcp_min_time = RTCP_MIN_TIME;
    gint n; /* no. of members for computation */
    gdouble rtcp_bw = session->rtcp_bw;

    gdouble const RTCP_RCVR_BW_FRACTION = (1.0 - RTCP_SENDER_BW_FRACTION);

    /* Very first call at application start-up uses half the min      */
    /* delay for quicker notification while still allowing some time  */
    /* before reporting for randomization and to learn about other    */
    /* sources so the report interval will converge to the correct    */
    /* interval more quickly.                                         */
    if (session->initial_rtcp)
    {
	rtcp_min_time /= 2;
    }
    /* If there were active senders, give them at least a minimum     */
    /* share of the RTCP bandwidth.  Otherwise all participants share */
    /* the RTCP bandwidth equally.                                    */
    if (session->sending_bye) n = session->bye_count;
    else n = session->ssrc_count;

    if (session->sender_count > 0 && session->sender_count < n * RTCP_SENDER_BW_FRACTION)
    {
	if (session->we_sent)
	{
	    rtcp_bw *= RTCP_SENDER_BW_FRACTION;
	    n = session->sender_count;
	}
	else
	{
	    rtcp_bw *= RTCP_RCVR_BW_FRACTION;
	    n -= session->sender_count;
	}
    }
    /* The effective number of sites times the average packet size is */
    /* the total number of octets sent when each site sends a report. */
    /* Dividing this by the effective bandwidth gives the time        */
    /* interval over which those packets must be sent in order to     */
    /* meet the bandwidth target, with a minimum enforced.  In that   */
    /* time interval we send one report so this time is also our      */
    /* average time between reports.                                  */
    t = session->avg_rtcp_size * n / rtcp_bw;
    if (t < rtcp_min_time)
    {
	t = rtcp_min_time;
    }
    session->rtcp_interval = t;

    /* To avoid traffic bursts from unintended synchronization with   */
    /* other sites, we then pick our actual next report interval as a */
    /* random number uniformly distributed between 0.5*t and 1.5*t.   */
    return (t * (drand48() + 0.5)) / COMPENSATION;
}


/* Llama a la funcion proporcionada en la inicializacion cada vez que se procesa un */
/* paquete rtp recibido.                                                            */

static void process_rtp (RtpSession *session, guint32 curr_rtp_ts, RtpPacket *packet, Source *s)
{
    gint i, d, transit;
    RtpEvent event;
    struct timeval event_ts;


    if (packet->cc > 0)
    {
	for (i = 0; i < packet->cc; i++) create_source(session, packet->csrc[i], FALSE);
    }
    /* Update the Source database... */
    if (s->sender == FALSE)
    {
	s->sender = TRUE;
	session->sender_count++;
    }
    transit = curr_rtp_ts - packet->ts;
    d = transit - s->transit;
    s->transit = transit;
    if (d < 0) d = -d;
    s->jitter += d - ((s->jitter + 8) / 16);

    /* Callback to the application to process the packet... */
    if (!filter_event(session, packet->ssrc))
    {
	gettimeofday(&event_ts, NULL);
	event.ssrc = packet->ssrc;
	event.type = RX_RTP;
	event.data = (gpointer) packet;	/* The callback function MUST free this! */
	event.ts = &event_ts;
	session->callback_rtp(session, &event);
    }
}


/* valida un paquete rtp recibido, devuelve TRUE si el paquete es correcto (esta  */
/* bien formado, FALSE en otro caso                                               */

static inline gboolean validate_rtp (RtpSession *session, RtpPacket *packet, gint len)
{
    /* This function checks the header info to make sure that the packet */
    /* is valid. We return TRUE if the packet is valid, FALSE otherwise. */
    /* See Appendix A.1 of the RTP specification.                        */

    /* We only accept RTPv2 packets... */
    if (packet->v != RTP_VERSION)
    {
	g_print("[RTP] rtp_header_validation: v != 2; v = %d (paket_seq: %d, timestamp: %d, ssrc: 0x%08x)\n", packet->v, packet->seq, packet->ts, packet->ssrc);
	return FALSE;
    }

    if (!session->opt->wait_for_rtcp)
    {
	/* We prefer speed over accuracy... */
	return TRUE;
    }

    /* Check for valid payload types..... 72-76 are RTCP payload type numbers, with */
    /* the high bit missing so we report that someone is running on the wrong port. */
    if ((packet->pt >= 72) && (packet->pt <= 76))
    {
	g_print("[RTP] rtp_header_validation: payload-type invalid (paket_seq: %d, timestamp: %d, ssrc: 0x%08x)\n", packet->seq, packet->ts, packet->ssrc);
	if (packet->m)
	{
	    g_print("[RTP] rtp_header_validation: RTCP packet on RTP port? (paket_seq: %d, timestamp: %d, ssrc: 0x%08x)\n", packet->seq, packet->ts, packet->ssrc);
	}
	return FALSE;
    }
    /* Check that the length of the packet is sensible... */
    if (len < (12 + (4 * packet->cc)))
    {
	g_print("[RTP] rtp_header_validation: packet length is smaller than the header (paket_seq: %d, timestamp: %d, ssrc: 0x%08x)\n", packet->seq, packet->ts, packet->ssrc);
	return FALSE;
    }
    /* Check that the amount of padding specified is sensible. */
    /* Note: have to include the size of any extension header! */
    if (packet->p)
    {
	gint  payload_len = len - 12 - (packet->cc * 4);
	if (packet->x)
	{
	    /* extension header and data */
	    payload_len -= 4 * (1 + packet->extn_len);
	}
	if (packet->data[packet->data_len - 1] > payload_len)
	{
	    g_print("[RTP] rtp_header_validation: padding greater than payload length (paket_seq: %d, timestamp: %d, ssrc: 0x%08x)\n", packet->seq, packet->ts, packet->ssrc);
	    return FALSE;
	}
	if (packet->data[packet->data_len - 1] < 1)
	{
	    g_print("[RTP] rtp_header_validation: padding zero (paket_seq: %d, timestamp: %d, ssrc: 0x%08x)\n", packet->seq, packet->ts, packet->ssrc);
	    return FALSE;
	}
    }
    return TRUE;
}


/* This routine preprocesses an incoming RTP packet, deciding whether to process it.     */

/* Esta funcion recibe tos paquetes rtp recibidos, los valida y los envia a la aplicacion */

static void rtp_recv_data (RtpSession *session, guint32 curr_rtp_ts)
{
    static RtpPacket *packet = NULL;
    static guint8 *buffer = NULL;
    static guint8 *buffer12 = NULL;
    gint buflen;
    Source *s;

    if (!session->opt->reuse_bufs || (packet == NULL))
    {
	packet = (RtpPacket *) g_malloc(RTP_MAX_PACKET_LEN);
	buffer = ((guint8 *) packet) + RTP_PACKET_HEADER_SIZE;
	buffer12 = buffer + 12;
    }
    buflen = udp_recv(session->rtp_socket, buffer, RTP_MAX_PACKET_LEN - RTP_PACKET_HEADER_SIZE);
    if (buflen > 0)
    {
#ifdef RTP_CRYPT_SUPPORT
	if (session->encryption_enabled)
	{
	    guint8  initVec[8] = {0,0,0,0,0,0,0,0};
	    (session->decrypt_func)(session, buffer, buflen, initVec);
	}
#endif
	/* Convert header fields to host byte order... */
	packet->seq = g_ntohs(packet->seq);
	packet->ts = g_ntohl(packet->ts);
	packet->ssrc = g_ntohl(packet->ssrc);
	/* Setup internal pointers, etc... */
	if (packet->cc)
	{
	    gint i;
	    packet->csrc = (guint32 *)(buffer12);
	    for (i = 0; i < packet->cc; i++) packet->csrc[i] = g_ntohl(packet->csrc[i]);
	}
	else
	{
	    packet->csrc = NULL;
	}
	if (packet->x)
	{
	    packet->extn = buffer12 + (packet->cc * 4);
	    packet->extn_len = (packet->extn[2] << 8) | packet->extn[3];
	    packet->extn_type = (packet->extn[0] << 8) | packet->extn[1];
	}
	else
	{
	    packet->extn = NULL;
	    packet->extn_len = 0;
	    packet->extn_type = 0;
	}
	packet->data = buffer12 + (packet->cc * 4);
	packet->data_len = buflen - (packet->cc * 4) - 12;
	if (packet->extn != NULL)
	{
	    packet->data += ((packet->extn_len + 1) * 4);
	    packet->data_len -= ((packet->extn_len + 1) * 4);
	}
	if (validate_rtp(session, packet, buflen))
	{
	    if (session->opt->wait_for_rtcp) s = create_source(session, packet->ssrc, TRUE);
	    else s = get_source(session, packet->ssrc);
	    if (session->opt->promiscuous_mode)
	    {
		if (s == NULL)
		{
		    s = create_source(session, packet->ssrc, FALSE);
		    s = get_source(session, packet->ssrc);
		}
		process_rtp(session, curr_rtp_ts, packet, s);
		/* We don't free "packet", that's done by the callback function... */
		return; 
	    }
	    if (s != NULL)
	    {
		if (s->probation == -1)
		{
		    s->probation = RTP_MIN_SEQUENTIAL;
		    s->max_seq   = packet->seq - 1;
		}
		if (update_seq(s, packet->seq))
		{
		    process_rtp(session, curr_rtp_ts, packet, s);
		    /* we don't free "packet", that's done by the callback function... */
		    return;
		}
		else
		{
		    /* This source is still on probation... */
		    g_print("[RTP] rtp_recv_data: RTP packet from probationary source ignored ... (paket_seq: %d, timestamp: %d, ssrc: 0x%08x)\n", packet->seq, packet->ts, packet->ssrc);
		}
	    }
	    else
	    {
		g_print("[RTP] rtp_recv_data: RTP packet from unknown source ignored (paket_seq: %d, timestamp: %d, ssrc: 0x%08x)\n", packet->seq, packet->ts, packet->ssrc);
	    }
	}
	else
	{
	    session->invalid_rtp_count++;
	    g_print("[RTP] rtp_recv_data: invalid RTP packet discarded\n");
	}
    }
    if (!session->opt->reuse_bufs) g_free(packet);
}


/* Validity check for a compound RTCP packet. This function returns        */
/* TRUE if the packet is okay, FALSE if the validity check fails.          */
/* The following checks can be applied to RTCP packets [RFC1889]:          */
/* o RTP version field must equal 2.                                       */
/* o The payload type field of the first RTCP packet in a compound         */
/*   packet must be equal to SR or RR.                                     */
/* o The padding bit (P) should be zero for the first packet of a          */
/*   compound RTCP packet because only the last should possibly            */
/*   need padding.                                                         */
/* o The length fields of the individual RTCP packets must total to        */
/*   the overall length of the compound RTCP packet as received.           */

/* Valida un paquete RTCP compuesto, devuelve TRUE si esta bien formado,   */
/* FALSE en caso contrario.                                                */

static gboolean validate_rtcp (guint8 *packet, gint len)
{
    rtcp_t *pkt = (rtcp_t *) packet;
    rtcp_t *end = (rtcp_t *) (((gchar *) pkt) + len);
    rtcp_t *r = pkt;
    gint l = 0;
    gint pc = 1;
    gint p = 0;


    /* All RTCP packets must be compound packets (RFC1889, section 6.1) */
    if (((g_ntohs(pkt->common.length) + 1) * 4) == len)
    {
	g_print("[RTP] validate_rtcp: bogus RTCP packet: not a compound packet\n");
	return FALSE;
    }

    /* Check the RTCP version, payload type and padding of the first in  */
    /* the compund RTCP packet...                                        */
    if (pkt->common.version != RTP_VERSION)
    {
	g_print("[RTP] validate_rtcp: version number(%d) != %d in the first sub-packet\n", pkt->common.version, RTP_VERSION);
	return FALSE;
    }
    if (pkt->common.p != 0)
    {
	g_print("[RTP] validate_rtcp: bogus RTCP packet: padding bit is set on first packet in compound\n");
	return FALSE;
    }
    if ((pkt->common.pt != RTCP_SR) && (pkt->common.pt != RTCP_RR))
    {
	g_print("[RTP] validate_rtcp: bogus RTCP packet: compund packet does not start with SR or RR\n");
	return FALSE;
    }
    /* Check all following parts of the compund RTCP packet. The RTP version */
    /* number must be 2, and the padding bit must be zero on all apart from  */
    /* the last packet.                                                      */
    do {
	if (p == 1)
	{
	    g_print("[RTP] validate_rtcp: bogus RTCP packet: padding bit set before last in compound (sub-packet %d)\n", pc);
	    return FALSE;
	}
	if (r->common.p) p = 1;
	if (r->common.version != RTP_VERSION)
	{
	    g_print("[RTP] validate_rtcp: bogus RTCP packet: version number(%d) != %d in sub-packet %d\n", r->common.version, RTP_VERSION, pc);
	    return FALSE;
	}
	l += (g_ntohs(r->common.length) + 1) * 4;
	r  = (rtcp_t *) (((guint32 *) r) + g_ntohs(r->common.length) + 1);
	pc++;	/* count of sub-packets, for debugging... */
    }
    while (r < end);

    /* Check that the length of the packets matches the length of the UDP */
    /* packet in which they were received...                              */
    if (l != len)
    {
	g_print("[RTP] validate_rtcp: bogus RTCP packet: RTCP packet length does not match UDP packet length (%d != %d)\n", l, len);
	return FALSE;
    }
    if (r != end)
    {
	g_print("[RTP] validate_rtcp: bogus RTCP packet: RTCP packet length does not match UDP packet length (%p != %p)\n", r, end);
	return FALSE;
    }
    return TRUE;
}


/* Procesa todos los componentes de un paquete RTCP compuesto             */

static void process_report_blocks (RtpSession *session, rtcp_t *packet, guint32 ssrc, rtcp_rr *rrp, struct timeval *event_ts)
{
    gint i;
    RtpEvent event;
    rtcp_rr *rr;


    /* ...process RRs... */
    if (packet->common.count == 0)
    {
	if (!filter_event(session, ssrc))
	{
	    event.ssrc = ssrc;
	    event.type = RX_RR_EMPTY;
	    event.data = NULL;
	    event.ts = event_ts;
	    session->callback_rtcp(session, &event);
	}
    }
    else
    {
	for (i = 0; i < packet->common.count; i++, rrp++)
	{
	    rr = (rtcp_rr *) g_malloc(sizeof(rtcp_rr));
	    rr->ssrc = g_ntohl(rrp->ssrc);
	    rr->fract_lost = rrp->fract_lost;	/* Endian conversion handled in the */
	    rr->total_lost = rrp->total_lost;	/* definition of the rtcp_rr type.  */
	    rr->last_seq = g_ntohl(rrp->last_seq);
	    rr->jitter = g_ntohl(rrp->jitter);
	    rr->lsr = g_ntohl(rrp->lsr);
	    rr->dlsr = g_ntohl(rrp->dlsr);

	    /* Create a database entry for this SSRC, if one doesn't already exist... */
	    create_source(session, rr->ssrc, FALSE);

	    /* Store the RR for later use... */
	    insert_rr(session, ssrc, rr, event_ts);

	    /* Call the event handler... */
	    if (!filter_event(session, ssrc))
	    {
		event.ssrc = ssrc;
		event.type = RX_RR;
		event.data = (gpointer) rr;
		event.ts = event_ts;
		session->callback_rtcp(session, &event);
	    }
	}
    }
}


static void process_rtcp_sr (RtpSession *session, rtcp_t *packet, struct timeval *event_ts)
{
    guint32 ssrc;
    RtpEvent event;
    rtcp_sr *sr;
    Source *s;


    ssrc = g_ntohl(packet->r.sr.sr.ssrc);
    s = create_source(session, ssrc, FALSE);
    if (s == NULL)
    {
	g_print("[RTP] process_rtcp_sr: source 0x%08x invalid, skipping ...\n", ssrc);
	return;
    }
    /* Mark as an active sender, if we get a sender report... */
    if (s->sender == FALSE)
    {
	s->sender = TRUE;
	session->sender_count++;
    }
    /* Process the SR... */
    sr = (rtcp_sr *) g_malloc(sizeof(rtcp_sr));
    sr->ssrc = ssrc;
    sr->ntp_sec = g_ntohl(packet->r.sr.sr.ntp_sec);
    sr->ntp_frac = g_ntohl(packet->r.sr.sr.ntp_frac);
    sr->rtp_ts = g_ntohl(packet->r.sr.sr.rtp_ts);
    sr->sender_pcount = g_ntohl(packet->r.sr.sr.sender_pcount);
    sr->sender_bcount = g_ntohl(packet->r.sr.sr.sender_bcount);

    /* Store the SR for later retrieval... */
    if (s->sr != NULL) g_free(s->sr);
    s->sr = sr;
    s->last_sr = *event_ts;

    /* Call the event handler... */
    if (!filter_event(session, ssrc))
    {
	event.ssrc = ssrc;
	event.type = RX_SR;
	event.data = (gpointer) sr;
	event.ts = event_ts;
	session->callback_rtcp(session, &event);
    }
    process_report_blocks(session, packet, ssrc, packet->r.sr.rr, event_ts);

    if (((packet->common.count * 6) + 1) < (g_ntohs(packet->common.length) - 5))
    {
	g_print("[RTP] process_rtcp_sr: source 0x%08x: profile specific SR extension ignored\n", ssrc);
    }
    return;
}


static void process_rtcp_rr (RtpSession *session, rtcp_t *packet, struct timeval *event_ts)
{
    guint32 ssrc;
    Source *s;

    ssrc = g_ntohl(packet->r.rr.ssrc);
    s = create_source(session, ssrc, FALSE);
    if (s == NULL)
    {
	g_print("[RTP] process_rtcp_rr: source 0x%08x invalid, skipping ...\n", ssrc);
	return;
    }
    process_report_blocks(session, packet, ssrc, packet->r.rr.rr, event_ts);

    if (((packet->common.count * 6) + 1) < g_ntohs(packet->common.length))
    {
	g_print("[RTP] process_rtcp_rr: source 0x%08x: profile specific RR extension ignored\n", ssrc);
    }
    return;
}


static void process_rtcp_sdes (RtpSession *session, rtcp_t *packet, struct timeval *event_ts)
{
    gint count;
    struct rtcp_sdes_t *sd;
    rtcp_sdes_item *rsp;
    rtcp_sdes_item *rspn;
    rtcp_sdes_item *end;
    Source *s;
    RtpEvent event;


    count = packet->common.count;
    sd = &packet->r.sdes;
    end = (rtcp_sdes_item *) ((guint32 *)packet + packet->common.length + 1);

    while (--count >= 0)
    {
	rsp = &sd->item[0];
	if (rsp >= end) break;
	sd->ssrc = g_ntohl(sd->ssrc);
	s = create_source(session, sd->ssrc, FALSE);
	if (s == NULL)
	{
	    g_print("[RTP] process_rtcp_sdes: can't get valid Source entry for 0x%08x, skipping...\n", sd->ssrc);
	}
	else
	{
	    for (; rsp->type; rsp = rspn )
	    {
		rspn = (rtcp_sdes_item *)((gchar *)rsp+rsp->length+2);
		if (rspn >= end)
		{
		    rsp = rspn;
		    break;
		}
		if (rtp_set_sdes(session, sd->ssrc, rsp->type, rsp->data, rsp->length))
		{
		    if (!filter_event(session, sd->ssrc))
		    {
			event.ssrc = sd->ssrc;
			event.type = RX_SDES;
			event.data = (gpointer) rsp;
			event.ts = event_ts;
			session->callback_rtcp(session, &event);
		    }
		}
		else
		{
		    g_print("[RTP] process_rtcp_sdes: invalid sdes item for source 0x%08x, skipping ...\n", sd->ssrc);
		}
	    }
	}
	sd = (struct rtcp_sdes_t *) ((guint32 *)sd + (((gchar *)rsp - (gchar *)sd) >> 2)+1);
    }
    if (count >= 0)
    {
	g_print("[RTP] process_rtcp_sdes: invalid RTCP SDES packet, some items ignored\n");
    }
    return;
}


static void process_rtcp_bye (RtpSession *session, rtcp_t *packet, struct timeval *event_ts)
{
    gint i;
    guint32 ssrc;
    RtpEvent event;
    Source *s;

    for (i = 0; i < packet->common.count; i++)
    {
	ssrc = g_ntohl(packet->r.bye.ssrc[i]);
	/* This is kind-of strange, since we create a Source we are about to delete. */
	/* This is done to ensure that the Source mentioned in the event which is    */
	/* passed to the user of the RTP library is valid, and simplify client code. */
	s = create_source(session, ssrc, FALSE);
	/* Call the event handler... */
	if (!filter_event(session, ssrc))
	{
	    event.ssrc = ssrc;
	    event.type = RX_BYE;
	    event.data = NULL;
	    event.ts = event_ts;
	    session->callback_rtcp(session, &event);
	}
	/* Mark the source as ready for deletion. Sources are not deleted immediately */
	/* since some packets may be delayed and arrive after the BYE...              */
	//s = get_source(session, ssrc);
	s->got_bye = TRUE;
	session->bye_count++;
    }
}


static void process_rtcp_app (RtpSession *session, rtcp_t *packet, struct timeval *event_ts)
{
    guint32 ssrc;
    RtpEvent event;
    rtcp_app *app;
    Source *s;
    gint data_len;


    /* Update the database for this Source. */
    ssrc = g_ntohl(packet->r.app.ssrc);
    s = create_source(session, ssrc, FALSE);
    /* Copy the entire packet, converting the header (only) into host byte order. */
    app = (rtcp_app *) g_malloc(RTP_MAX_PACKET_LEN);
    app->version = packet->common.version;
    app->p = packet->common.p;
    app->subtype = packet->common.count;
    app->pt = packet->common.pt;
    app->length = g_ntohs(packet->common.length);
    app->ssrc = ssrc;
    app->name[0] = packet->r.app.name[0];
    app->name[1] = packet->r.app.name[1];
    app->name[2] = packet->r.app.name[2];
    app->name[3] = packet->r.app.name[3];
    data_len = (app->length - 2) * 4;
    memcpy(app->data, packet->r.app.data, data_len);

    /* Callback to the application to process the app packet... */
    if (!filter_event(session, ssrc))
    {
	event.ssrc = ssrc;
	event.type = RX_APP;
	event.data = (gpointer) app; /* The callback function MUST free this! */
	event.ts = event_ts;
	session->callback_rtcp(session, &event);
    }
}


/* This routine processes incoming RTCP packets                             */

/* Llama a todas las funciones que procesan los tipos de paquetes RTCP      */

static void rtp_process_ctrl (RtpSession *session, guint8 *buffer, gint buflen)
{
    RtpEvent event;
    struct timeval event_ts;
    rtcp_t *packet;
#ifdef RTP_CRYPT_SUPPORT
    guint8 initVec[8] = {0,0,0,0,0,0,0,0};
#endif
    gboolean first;
    guint32 packet_ssrc = rtp_my_ssrc(session);


    gettimeofday(&event_ts, NULL);
    if (buflen > 0)
    {

#ifdef RTP_CRYPT_SUPPORT
	if (session->encryption_enabled)
	{
	    /* Decrypt the packet... */
	    (session->decrypt_func)(session, buffer, buflen, initVec);
	    buffer += 4;	/* Skip the random prefix... */
	    buflen -= 4;
	}
#endif

	if (validate_rtcp(buffer, buflen))
	{
	    first  = TRUE;
	    packet = (rtcp_t *) buffer;
	    while (packet < (rtcp_t *) (buffer + buflen))
	    {
		switch (packet->common.pt)
		{
		case RTCP_SR:
		    if (first && !filter_event(session, g_ntohl(packet->r.sr.sr.ssrc)))
		    {
			event.ssrc = g_ntohl(packet->r.sr.sr.ssrc);
			event.type = RX_RTCP_START;
			event.data = &buflen;
			event.ts = &event_ts;
			packet_ssrc = event.ssrc;
			session->callback_rtcp(session, &event);
		    }
		    process_rtcp_sr(session, packet, &event_ts);
		    break;
		case RTCP_RR:
		    if (first && !filter_event(session, g_ntohl(packet->r.rr.ssrc)))
		    {
			event.ssrc = g_ntohl(packet->r.rr.ssrc);
			event.type = RX_RTCP_START;
			event.data = &buflen;
			event.ts = &event_ts;
			packet_ssrc = event.ssrc;
			session->callback_rtcp(session, &event);
		    }
		    process_rtcp_rr(session, packet, &event_ts);
		    break;
		case RTCP_SDES:
		    if (first && !filter_event(session, g_ntohl(packet->r.sdes.ssrc)))
		    {
			event.ssrc = g_ntohl(packet->r.sdes.ssrc);
			event.type = RX_RTCP_START;
			event.data = &buflen;
			event.ts = &event_ts;
			packet_ssrc = event.ssrc;
			session->callback_rtcp(session, &event);
		    }
		    process_rtcp_sdes(session, packet, &event_ts);
		    break;
		case RTCP_BYE:
		    if (first && !filter_event(session, g_ntohl(packet->r.bye.ssrc[0])))
		    {
			event.ssrc = g_ntohl(packet->r.bye.ssrc[0]);
			event.type = RX_RTCP_START;
			event.data = &buflen;
			event.ts = &event_ts;
			packet_ssrc = event.ssrc;
			session->callback_rtcp(session, &event);
		    }
		    process_rtcp_bye(session, packet, &event_ts);
		    break;
		case RTCP_APP:
		    if (first && !filter_event(session, g_ntohl(packet->r.app.ssrc)))
		    {
			event.ssrc = g_ntohl(packet->r.app.ssrc);
			event.type = RX_RTCP_START;
			event.data = &buflen;
			event.ts = &event_ts;
			packet_ssrc = event.ssrc;
			session->callback_rtcp(session, &event);
		    }
		    process_rtcp_app(session, packet, &event_ts);
		    break;
		default:
		    g_print("[RTP] rtp_process_ctrl: RTCP packet with unknown type (%d) ignored.\n", packet->common.pt);
		    break;
		}
		packet = (rtcp_t *) ((gchar *) packet + (4 * (g_ntohs(packet->common.length) + 1)));
		first  = FALSE;
	    }
	    if (session->avg_rtcp_size < 0)
	    {
		/* This is the first RTCP packet we've received, set our initial estimate */
		/* of the average  packet size to be the size of this packet.             */
		session->avg_rtcp_size = buflen + RTP_LOWER_LAYER_OVERHEAD;
	    }
	    else
	    {
		/* Update our estimate of the average RTCP packet size. The constants are */
		/* 1/16 and 15/16 (section 6.3.3 of draft-ietf-avt-rtp-new-02.txt).       */
		session->avg_rtcp_size = (0.0625 * (buflen + RTP_LOWER_LAYER_OVERHEAD)) + (0.9375 * session->avg_rtcp_size);
	    }
	    /* Signal that we've finished processing this packet */
	    if (!filter_event(session, packet_ssrc))
	    {
		event.ssrc = packet_ssrc;
		event.type = RX_RTCP_FINISH;
		event.data = NULL;
		event.ts = &event_ts;
		session->callback_rtcp(session, &event);
	    }
	}
	else
	{
	    g_print("[RTP] rtp_process_ctrl: invalid RTCP packet discarded\n");
	    session->invalid_rtcp_count++;
	}
    }
}


/* Calcula el numero de bloques (componentes) para un paquete rtcp ... */

static gint format_report_blocks (rtcp_rr *rrp, gint remaining_length, RtpSession *session)
{
    gint h;
    gint tmp;
    Source *s;
    struct timeval now;
    gint nblocks = 0;


    gettimeofday(&now, NULL);
    for (h = 0; h < RTP_DB_SIZE; h++)
    {
	for (s = session->db[h]; s != NULL; s = s->next)
	{
	    if ((nblocks == 31) || (remaining_length < 24))
	    {
		break; /* Insufficient space for more report blocks... */
	    }
	    if (s->sender)
	    {
		/* Much of this is taken from A.3 of draft-ietf-avt-rtp-new-01.txt */
		gint extended_max = s->cycles + s->max_seq;
		gint expected = extended_max - s->base_seq + 1;
		gint lost = expected - s->received;
		gint expected_interval = expected - s->expected_prior;
		gint received_interval = s->received - s->received_prior;
		gint lost_interval = expected_interval - received_interval;
		gint fraction;
		guint32 lsr;
		guint32 dlsr;

		s->expected_prior = expected;
		s->received_prior = s->received;
		if (expected_interval == 0 || lost_interval <= 0) fraction = 0;
		else fraction = (lost_interval << 8) / expected_interval;

		if (s->sr == NULL)
		{
		    lsr = 0;
		    dlsr = 0;
		}
		else
		{
		    lsr = ntp64_to_ntp32(s->sr->ntp_sec, s->sr->ntp_frac);
		    dlsr = (guint32)(diff_timeval(now, s->last_sr) * 65536);
		}
		rrp->ssrc = g_htonl(s->ssrc);
		rrp->fract_lost = fraction;
		rrp->total_lost = lost & 0x00ffffff;
		rrp->last_seq = g_htonl(extended_max);
                tmp = s->jitter / 16;
		rrp->jitter = g_htonl(tmp);
		rrp->lsr = g_htonl(lsr);
		rrp->dlsr = g_htonl(dlsr);
		rrp++;
		remaining_length -= 24;
		nblocks++;
		s->sender = FALSE;
		session->sender_count--;
		if (session->sender_count == 0)
		{
		    break; /* No point continuing, since we've reported on all senders... */
		}
	    }
	}
    }
    return nblocks;
}


/* Write an RTCP SR into buffer, returning a pointer to */
/* the next byte after the header we have just written. */

static guint8 *format_rtcp_sr (guint8 *buffer, gint buflen, RtpSession *session, guint32 rtp_ts)
{
    rtcp_t *packet = (rtcp_t *) buffer;
    gint remaining_length;
    guint16 tmp;
    guint32 ntp_sec, ntp_frac;

    /* ...else there isn't space for the header and sender report */
    g_assert(buflen >= 28);

    packet->common.version = RTP_VERSION;
    packet->common.p = 0;
    packet->common.count = 0;
    packet->common.pt = RTCP_SR;
    packet->common.length = g_htons(1);

    ntp64_time(&ntp_sec, &ntp_frac);
    packet->r.sr.sr.ssrc = g_htonl(rtp_my_ssrc(session));
    packet->r.sr.sr.ntp_sec = g_htonl(ntp_sec);
    packet->r.sr.sr.ntp_frac = g_htonl(ntp_frac);
    packet->r.sr.sr.rtp_ts = g_htonl(rtp_ts);
    packet->r.sr.sr.sender_pcount = g_htonl(session->rtp_pcount);
    packet->r.sr.sr.sender_bcount = g_htonl(session->rtp_bcount);

    /* Add report blocks, until we either run out of senders */
    /* to report upon or we run out of space in the buffer.  */
    remaining_length = buflen - 28;
    packet->common.count = format_report_blocks(packet->r.sr.rr, remaining_length, session);
    tmp = (guint16) (6 + (packet->common.count * 6));
    packet->common.length = g_htons(tmp);
    return buffer + 28 + (24 * packet->common.count);
}


/* Write an RTCP RR into buffer, returning a pointer to */
/* the next byte after the header we have just written. */

static guint8 *format_rtcp_rr (guint8 *buffer, gint buflen, RtpSession *session)
{
    rtcp_t *packet = (rtcp_t *) buffer;
    gint remaining_length;

    g_assert(buflen >= 8);
    /* ...else there isn't space for the header */

    packet->common.version = RTP_VERSION;
    packet->common.p = 0;
    packet->common.count = 0;
    packet->common.pt = RTCP_RR;
    packet->common.length = g_htons(1);
    packet->r.rr.ssrc = g_htonl(session->my_ssrc);

    /* Add report blocks, until we either run out of senders */
    /* to report upon or we run out of space in the buffer.  */
    remaining_length = buflen - 8;
    packet->common.count = format_report_blocks(packet->r.rr.rr, remaining_length, session);
    packet->common.length = g_htons((guint16) (1 + (packet->common.count * 6)));
    return buffer + 8 + (24 * packet->common.count);
}


/* Fill out an SDES item. It is assumed that the item is a NULL    */
/* terminated string.                                              */

static inline  gint add_sdes_item (guint8 *buf, gint buflen, gint type, const gchar *val)
{
    rtcp_sdes_item *shdr = (rtcp_sdes_item *) buf;
    gint namelen;

    if (val == NULL) return 0;
    shdr->type = type;
    namelen = strlen(val);
    shdr->length = namelen;
    g_strlcpy(shdr->data, val, buflen - 2); /* The "-2" accounts for the other shdr fields */
    return namelen + 2;
}


/* From draft-ietf-avt-profile-new-00:                             */
/* "Applications may use any of the SDES items described in the    */
/* RTP specification. While CNAME information is sent every        */
/* reporting interval, other items should be sent only every third */
/* reporting interval, with NAME sent seven out of eight times     */
/* within that slot and the remaining SDES items cyclically taking */
/* up the eighth slot, as defined in Section 6.2.2 of the RTP      */
/* specification. In other words, NAME is sent in RTCP packets 1,  */
/* 4, 7, 10, 13, 16, 19, while, say, EMAIL is used in RTCP packet  */
/* 22".                                                            */

static guint8 *format_rtcp_sdes (guint8 *buffer, gint buflen, guint32 ssrc, RtpSession *session)
{
    guint8 *packet = buffer;
    RtcpCommon	*common = (RtcpCommon *) buffer;
    const gchar	*item;
    size_t remaining_len;
    guint16 tmp;
    gint pad;


    g_assert(buflen > (gint) sizeof(RtcpCommon));
    common->version = RTP_VERSION;
    common->p = 0;
    common->count = 1;
    common->pt = RTCP_SDES;
    common->length = 0;
    packet += sizeof(common);
    *((guint32 *) packet) = g_htonl(ssrc);
    packet += 4;
    remaining_len = buflen - (packet - buffer);
    item = rtp_get_sdes(session, ssrc, RTCP_SDES_CNAME);
    if ((item != NULL) && ((strlen(item) + (size_t) 2) <= remaining_len))
    {
	packet += add_sdes_item(packet, remaining_len, RTCP_SDES_CNAME, item);
    }
    remaining_len = buflen - (packet - buffer);
    item = rtp_get_sdes(session, ssrc, RTCP_SDES_NOTE);
    if ((item != NULL) && ((strlen(item) + (size_t) 2) <= remaining_len))
    {
	packet += add_sdes_item(packet, remaining_len, RTCP_SDES_NOTE, item);
    }
    remaining_len = buflen - (packet - buffer);
    if ((session->sdes_count_pri % 3) == 0)
    {
	session->sdes_count_sec++;
	if ((session->sdes_count_sec % 8) == 0)
	{
	    /* Note that the following is supposed to fall-through the cases */
	    /* until one is found to send... The lack of break statements in */
	    /* the switch is not a bug.                                      */
	    switch (session->sdes_count_ter % 5)
	    {
	    case 0:
		item = rtp_get_sdes(session, ssrc, RTCP_SDES_TOOL);
		if ((item != NULL) && ((strlen(item) + (size_t) 2) <= remaining_len))
		{
		    packet += add_sdes_item(packet, remaining_len, RTCP_SDES_TOOL, item);
		    break;
		}
	    case 1:
		item = rtp_get_sdes(session, ssrc, RTCP_SDES_EMAIL);
		if ((item != NULL) && ((strlen(item) + (size_t) 2) <= remaining_len))
		{
		    packet += add_sdes_item(packet, remaining_len, RTCP_SDES_EMAIL, item);
		    break;
		}
	    case 2:
		item = rtp_get_sdes(session, ssrc, RTCP_SDES_PHONE);
		if ((item != NULL) && ((strlen(item) + (size_t) 2) <= remaining_len))
		{
		    packet += add_sdes_item(packet, remaining_len, RTCP_SDES_PHONE, item);
		    break;
		}
	    case 3:
		item = rtp_get_sdes(session, ssrc, RTCP_SDES_LOC);
		if ((item != NULL) && ((strlen(item) + (size_t) 2) <= remaining_len))
		{
		    packet += add_sdes_item(packet, remaining_len, RTCP_SDES_LOC, item);
		    break;
		}
	    case 4:
		item = rtp_get_sdes(session, ssrc, RTCP_SDES_PRIV);
		if ((item != NULL) && ((strlen(item) + (size_t) 2) <= remaining_len))
		{
		    packet += add_sdes_item(packet, remaining_len, RTCP_SDES_PRIV, item);
		    break;
		}
	    }
	    session->sdes_count_ter++;
	}
	else
	{
	    item = rtp_get_sdes(session, ssrc, RTCP_SDES_NAME);
	    if (item != NULL) packet += add_sdes_item(packet, remaining_len, RTCP_SDES_NAME, item);

	}
    }
    session->sdes_count_pri++;
    /* Pad to a multiple of 4 bytes... */
    pad = 4 - ((packet - buffer) & 0x3);
    while (pad--) *packet++ = RTCP_SDES_END;
    tmp = (guint16) ((gint) (packet - buffer) / 4) - 1;
    common->length = g_htons(tmp);
    return packet;
}


/* Write an RTCP APP into the outgoing packet buffer. */

static guint8 *format_rtcp_app (guint8 *buffer, gint buflen, guint32 ssrc, rtcp_app *app)
{
    rtcp_app *packet = (rtcp_app *) buffer;
    gint pkt_octets = (app->length + 1) * 4;
    gint data_octets = pkt_octets - 12;


    g_assert(data_octets >= 0);
    /* ...else not a legal APP packet.               */
    g_assert(buflen >= pkt_octets);
    /* ...else there isn't space for the APP packet. */

    /* Copy one APP packet from "app" to "packet". */
    packet->version = RTP_VERSION;
    packet->p = app->p;
    packet->subtype = app->subtype;
    packet->pt = RTCP_APP;
    packet->length = g_htons(app->length);
    packet->ssrc = g_htonl(ssrc);
    memcpy(packet->name, app->name, 4);
    memcpy(packet->data, app->data, data_octets);
    /* Return a pointer to the byte that immediately follows the last byte written. */
    return buffer + pkt_octets;
}


/* Construct and send an RTCP packet. The order in which packets are packed into a */
/* compound packet is defined by section 6.1 of draft-ietf-avt-rtp-new-03.txt and  */
/* we follow the recommended order.                                                */

static void send_rtcp (RtpSession *session, guint32 rtp_ts, rtcp_app_callback appcallback)
{
    guint8 buffer[RTP_MAX_PACKET_LEN + RTP_MAX_ENCRYPTION_PAD];	/* The +8 is to allow for padding when encrypting */
    guint8 *ptr = buffer;
    guint8 *old_ptr;
    guint8 *lpt;  /* the last packet in the compound */
    rtcp_app *app;


#ifdef RTP_CRYPT_SUPPORT
    guint8 initVec[8] = {0,0,0,0,0,0,0,0};


    /* If encryption is enabled, add a 32 bit random prefix to the packet */
    if (session->encryption_enabled)
    {
	*((guint32 *) ptr) = g_random_int();
	ptr += 4;
    }
#endif

    /* The first RTCP packet in the compound packet MUST always be a report packet...  */
    if (session->we_sent)
	ptr = format_rtcp_sr(ptr, RTP_MAX_PACKET_LEN - (ptr - buffer), session, rtp_ts);
    else
	ptr = format_rtcp_rr(ptr, RTP_MAX_PACKET_LEN - (ptr - buffer), session);

    /* Add the appropriate SDES items to the packet... This should really be after the */
    /* insertion of the additional report blocks, but if we do that there are problems */
    /* with us being unable to fit the SDES packet in when we run out of buffer space  */
    /* adding RRs. The correct fix would be to calculate the length of the SDES items  */
    /* in advance and subtract this from the buffer length but this is non-trivial and */
    /* probably not worth it.                                                          */
    lpt = ptr;
    ptr = format_rtcp_sdes(ptr, RTP_MAX_PACKET_LEN - (ptr - buffer), rtp_my_ssrc(session), session);

    /* If we have any CSRCs, we include SDES items for each of them in turn...         */
    if (session->csrc_count > 0)
    {
	ptr = format_rtcp_sdes(ptr, RTP_MAX_PACKET_LEN - (ptr - buffer), next_csrc(session), session);
    }
    /* Following that, additional RR packets SHOULD follow if there are more than 31   */
    /* senders, such that the reports do not fit into the initial packet. We give up   */
    /* if there is insufficient space in the buffer: this is bad, since we always drop */
    /* the reports from the same sources (those at the end of the hash table).         */
    while ((session->sender_count > 0)  && ((RTP_MAX_PACKET_LEN - (ptr - buffer)) >= 8))
    {
	lpt = ptr;
	ptr = format_rtcp_rr(ptr, RTP_MAX_PACKET_LEN - (ptr - buffer), session);
    }
    /* Finish with as many APP packets as the application will provide. */
    old_ptr = ptr;
    if (appcallback)
    {
	while ((app = (*appcallback)(session, rtp_ts, RTP_MAX_PACKET_LEN - (ptr - buffer))))
	{
	    lpt = ptr;
	    ptr = format_rtcp_app(ptr, RTP_MAX_PACKET_LEN - (ptr - buffer), rtp_my_ssrc(session), app);
	    g_assert(ptr > old_ptr);
	    old_ptr = ptr;
	    g_assert(RTP_MAX_PACKET_LEN - (ptr - buffer) >= 0);
	}
    }

#ifdef RTP_CRYPT_SUPPORT
    /* And encrypt if desired... */
    if (session->encryption_enabled)
    {
	if (((ptr - buffer) % session->encryption_pad_length) != 0)
	{
	    /* Add padding to the last packet in the compound, if necessary. */
	    /* We don't have to worry about overflowing the buffer, since we */
	    /* intentionally allocated it 8 bytes longer to allow for this.  */
	    gint padlen = session->encryption_pad_length - ((ptr - buffer) % session->encryption_pad_length);
	    gint i;
            gint16 tmp;

	    for (i = 0; i < padlen-1; i++) *(ptr++) = '\0';
	    *(ptr++) = (guint8) padlen;
	    g_assert(((ptr - buffer) % session->encryption_pad_length) == 0);
	    ((rtcp_t *) lpt)->common.p = TRUE;
            tmp = (gint16) ((ptr - lpt) / 4) - 1;
	    ((rtcp_t *) lpt)->common.length = g_htons(tmp);
	}
	(session->encrypt_func)(session, buffer, ptr - buffer, initVec);
    }
#endif
    udp_send(session->rtcp_socket, buffer, ptr - buffer);

    /* Loop the data back to ourselves so local participant can */
    /* query own stats when using unicast or multicast with no  */
    /* loopback.                                                */
    rtp_process_ctrl(session, buffer, ptr - buffer);
}


/* Send a BYE packet immediately. This is an internal function,  */
/* hidden behind the rtp_send_bye() wrapper which implements BYE */
/* reconsideration for the application.                          */

static gboolean rtp_send_bye_now (RtpSession *session)
{
    guint8 buffer[RTP_MAX_PACKET_LEN + RTP_MAX_ENCRYPTION_PAD]; /* + 8 to allow for padding when encrypting */
    guint8 *ptr = buffer;
    RtcpCommon	*common;


#ifdef RTP_CRYPT_SUPPORT
    guint8 initVec[8] = {0,0,0,0,0,0,0,0};


    /* If encryption is enabled, add a 32 bit random prefix to the packet */
    if (session->encryption_enabled)
    {
	*((guint32 *) ptr) = g_random_int();
	ptr += 4;
    }
#endif

    ptr = format_rtcp_rr(ptr, RTP_MAX_PACKET_LEN - (ptr - buffer), session);
    common = (RtcpCommon *) ptr;
    common->version = RTP_VERSION;
    common->p = 0;
    common->count = 1;
    common->pt = RTCP_BYE;
    common->length = g_htons(1);
    ptr += sizeof(common);
    *((guint32 *) ptr) = g_htonl(session->my_ssrc);
    ptr += 4;

#ifdef RTP_CRYPT_SUPPORT
    if (session->encryption_enabled)
    {
	if (((ptr - buffer) % session->encryption_pad_length) != 0)
	{
	    /* Add padding to the last packet in the compound, if necessary. */
	    /* We don't have to worry about overflowing the buffer, since we */
	    /* intentionally allocated it 8 bytes longer to allow for this.  */
	    gint padlen = session->encryption_pad_length - ((ptr - buffer) % session->encryption_pad_length);
	    gint i;
            gint16 tmp;

	    for (i = 0; i < padlen-1; i++) *(ptr++) = '\0';
	    *(ptr++) = (guint8) padlen;
	    common->p = TRUE;
            tmp = (gint16) ((ptr - (guint8 *) common) / 4) - 1;
	    common->length = g_htons(tmp);
	}
        if (((ptr - buffer) % session->encryption_pad_length) != 0) return FALSE;
	(session->encrypt_func)(session, buffer, ptr - buffer, initVec);
    }
#endif

    udp_send(session->rtcp_socket, buffer, ptr - buffer);
    /* Loop the data back to ourselves so local participant can */
    /* query own stats when using unicast or multicast with no  */
    /* loopback.                                                */
    rtp_process_ctrl(session, buffer, ptr - buffer);
    return TRUE;
}




// Funciones publicas



/**
 * rtp_init:
 * @addr: IP destination of this session (unicast or multicast),
 * as an ASCII string.  May be a host name, which will be looked up,
 * or may be an IPv4 dotted quad or IPv6 literal adddress.
 * @iface: If the destination of the session is multicast,
 * the optional interface to bind to.  May be NULL, in which case
 * the default multicast interface as determined by the system
 * will be used.
 * @rx_port: The port to which to bind the UDP socket
 * @tx_port: The port to which to send UDP packets
 * @ttl: The TTL with which to send multicasts
 * @rtcp_bw: The total bandwidth (in units of ___) that is
 * allocated to RTCP.
 * @callback: See section on #rtp_callback.
 * @userdata: Opaque data associated with the session.  See
 * rtp_get_userdata().
 *
 * Creates and initializes an RTP session.
 *
 * Returns: An opaque session identifier to be used in future calls to
 * the RTP library functions, or NULL on failure.
 */

RtpSession *rtp_init (const gchar *addr, gchar *iface, guint16 rx_port, guint16 tx_port, gint ttl, gdouble rtcp_bw,
		      rtp_callback callback_rtp, rtp_callback callback_rtcp, gpointer userdata, GError **error)
{
    RtpSession *session;
    gint i, j;
    gchar *cname;
    GError *tmp_error = NULL;


    if (ttl < 0)
    {
	g_set_error(error, RTP_ERROR, RTP_ERROR_PORT, "ttl must be greater than zero");
	g_printerr("[RTP] rtp_init: ttl(%d) must be greater than zero\n", ttl);
	return NULL;
    }
    if (rx_port % 2)
    {
	g_set_error(error, RTP_ERROR, RTP_ERROR_PORT, "rx_port must be even");
	g_printerr("[RTP] rtp_init: rx_port(%d) must be even\n", rx_port);
	return NULL;
    }
    if (tx_port % 2)
    {
	g_set_error(error, RTP_ERROR, RTP_ERROR_PORT, "tx_port must be even");
	g_printerr("[RTP] rtp_init: tx_port(%d) must be even\n", tx_port);
        return NULL;
    }
    session = (RtpSession *) g_malloc0(sizeof(RtpSession));
    session->opt = (Options *) g_malloc(sizeof(Options));
    session->userdata = userdata;
    session->addr = g_strdup(addr);
    session->rx_port = rx_port;
    session->tx_port = tx_port;
    session->ttl = MIN(ttl, 127);
    session->rtp_socket	= udp_init(addr, iface, rx_port, tx_port, ttl, &tmp_error);
    if (session->rtp_socket == NULL)
    {
        g_propagate_error (error, tmp_error);
	g_free(session);
	return NULL;
    }
    session->rtcp_socket= udp_init(addr, iface, (guint16) (rx_port+1), (guint16) (tx_port+1), ttl, &tmp_error);
    if (session->rtcp_socket == NULL)
    {
        g_propagate_error (error, tmp_error);
	g_free(session);
	return NULL;
    }
    /* Default option settings. */
    rtp_set_option(session, RTP_OPT_PROMISC,           FALSE);
    rtp_set_option(session, RTP_OPT_WEAK_VALIDATION,   FALSE);
    rtp_set_option(session, RTP_OPT_FILTER_MY_PACKETS, FALSE);
    rtp_set_option(session, RTP_OPT_REUSE_PACKET_BUFS, FALSE);

    //inicializa o rollo aleatorio
    init_rng(udp_host_addr(session->rtp_socket, NULL));

    session->my_ssrc = (guint32) lrand48();
    session->callback_rtp = callback_rtp;
    session->callback_rtcp = callback_rtcp;
    session->invalid_rtp_count = 0;
    session->invalid_rtcp_count = 0;
    session->bye_count = 0;
    session->csrc_count = 0;
    session->ssrc_count = 0;
    session->ssrc_count_prev = 0;
    session->sender_count = 0;
    session->initial_rtcp = TRUE;
    session->sending_bye = FALSE;
    session->avg_rtcp_size = -1;	/* Sentinal value: reception of first packet starts initial value... */
    session->we_sent = FALSE;
    session->rtcp_bw = rtcp_bw;
    session->sdes_count_pri = 0;
    session->sdes_count_sec = 0;
    session->sdes_count_ter = 0;
    session->rtp_seq = (guint16) lrand48();
    session->rtp_pcount = 0;
    session->rtp_bcount = 0;
    gettimeofday(&(session->last_update), NULL);
    gettimeofday(&(session->last_rtcp_send_time), NULL);
    gettimeofday(&(session->next_rtcp_send_time), NULL);

#ifdef RTP_CRYPT_SUPPORT
    session->encryption_enabled = 0;
    session->encryption_algorithm = NULL;
#endif

    /* Calculate when we're supposed to send our first RTCP packet... */
    add_timeval(&(session->next_rtcp_send_time), rtcp_interval(session));

    /* Initialise the Source database... */
    for (i = 0; i < RTP_DB_SIZE; i++) session->db[i] = NULL;
    session->last_advertised_csrc = 0;

    /* Initialize sentinels in rr table */
    for (i = 0; i < RTP_DB_SIZE; i++)
    {
	for (j = 0; j < RTP_DB_SIZE; j++)
	{
	    session->rr[i][j].next = &session->rr[i][j];
	    session->rr[i][j].prev = &session->rr[i][j];
	}
    }
    /* Create a database entry for ourselves... */
    create_source(session, session->my_ssrc, FALSE);
    cname = get_cname(session->rtp_socket);
    rtp_set_sdes(session, session->my_ssrc, RTCP_SDES_CNAME, cname, strlen(cname));
    g_free(cname);	/* cname is copied by rtp_set_sdes()... */
    return session;
}



gboolean rtp_set_callback (RtpSession *session, rtp_callback callback_rtp, rtp_callback callback_rtcp)
{
    session->callback_rtp = callback_rtp;
    session->callback_rtcp = callback_rtcp;
    return TRUE;
}


/**
 * rtp_set_my_ssrc:
 * @session: the RTP session 
 * @ssrc: the SSRC to be used by the RTP session
 * 
 * This function coerces the local SSRC identifer to be ssrc.  For
 * this function to succeed it must be called immediately after
 * rtp_init or rtp_init_if.  The intended purpose of this
 * function is to co-ordinate SSRC's between layered sessions, it
 * should not be used otherwise.
 *
 * Returns: TRUE on success, FALSE otherwise.  
 */

gboolean rtp_set_my_ssrc (RtpSession *session, guint32 ssrc)
{
    Source *s;
    guint32 h;

    if ((session->ssrc_count != 1) && (session->sender_count != 0)) return FALSE;
    /* Remove existing Source */
    h = ssrc_hash(session->my_ssrc);
    s = session->db[h];
    session->db[h] = NULL;
    /* Fill in new ssrc       */
    session->my_ssrc = ssrc;
    s->ssrc = ssrc;
    h = ssrc_hash(ssrc);
    /* Put Source back        */
    session->db[h] = s;
    return TRUE;
}



/**
 * rtp_set_option:
 * @session: The RTP session.
 * @optname: The option name, see #rtp_option.
 * @optval: The value to set.
 *
 * Sets the value of a session option.  See #rtp_option for
 * documentation on the Options and their legal values.
 *
 * Returns: TRUE on success, else FALSE.
 */

gboolean rtp_set_option (RtpSession *session, rtp_option optname, gboolean optval)
{
    switch (optname)
    {
    case RTP_OPT_WEAK_VALIDATION:
	session->opt->wait_for_rtcp = optval;
	break;
    case RTP_OPT_PROMISC:
	session->opt->promiscuous_mode = optval;
	break;
    case RTP_OPT_FILTER_MY_PACKETS:
	session->opt->filter_my_packets = optval;
	break;
    case RTP_OPT_REUSE_PACKET_BUFS:
	session->opt->reuse_bufs = optval;
	break;
    default:
	g_printerr("[RTP] rtp_set_option: unknown option\n");
	return FALSE;
    }
    return TRUE;
}



/**
 * rtp_get_option:
 * @session: The RTP session.
 * @optname: The option name, see #rtp_option.
 * @optval: The return value.
 *
 * Retrieves the value of a session option.  See #rtp_option for
 * documentation on the Options and their legal values.
 *
 * Returns: TRUE and the value of the option in optval on success, else FALSE.
 */
gboolean rtp_get_option (RtpSession *session, rtp_option optname, gint *optval)
{
    switch (optname)
    {
    case RTP_OPT_WEAK_VALIDATION:
	*optval = session->opt->wait_for_rtcp;
	break;
    case RTP_OPT_PROMISC:
	*optval = session->opt->promiscuous_mode;
	break;
    case RTP_OPT_FILTER_MY_PACKETS:
	*optval = session->opt->filter_my_packets;
	break;
    case RTP_OPT_REUSE_PACKET_BUFS:
	*optval = session->opt->reuse_bufs;
	break;
    default:
	*optval = -1;
	g_printerr("[RTP] rtp_get_option: unknown option\n");
	return FALSE;
    }
    return TRUE;
}



/**
 * rtp_get_userdata:
 * @session: The RTP session.
 *
 * This function returns the userdata pointer that was passed to the
 * rtp_init() or rtp_init_if() function when creating this session.
 *
 * Returns: pointer to userdata.
 */
gpointer rtp_get_userdata (RtpSession *session)
{
    return session->userdata;
}




void rtp_set_userdata (RtpSession *session, gpointer userdata)
{
    session->userdata = userdata;
}



/**
 * rtp_my_ssrc:
 * @session: The RTP Session.
 *
 * Returns: The SSRC we are currently using in this session. Note that our
 * SSRC can change at any time (due to collisions) so applications must not
 * store the value returned, but rather should call this function each time 
 * they need it.
 */
guint32 rtp_my_ssrc (RtpSession *session)
{
    return session->my_ssrc;
}



/**
 * rtp_recv:
 * @session: the session pointer (returned by rtp_init())
 * @timeout: the amount of time that rtcp_recv() is allowed to block
 * @curr_rtp_ts: the current time expressed in units of the media
 * timestamp.
 *
 * Receive RTP packets and dispatch them.
 *
 * Returns: TRUE if data received, FALSE if the timeout occurred.
 */

gboolean rtp_recv (RtpSession *session, struct timeval *timeout, guint32 curr_rtp_ts)
{
    udp_fd_zero();
    udp_fd_set(session->rtp_socket);
    udp_fd_set(session->rtcp_socket);
    if (udp_select(timeout) > 0)
    {
	if (udp_fd_isset(session->rtp_socket)) rtp_recv_data(session, curr_rtp_ts);
	if (udp_fd_isset(session->rtcp_socket))
	{
	    guint8 buffer[RTP_MAX_PACKET_LEN];
	    gint buflen;
	    buflen = udp_recv(session->rtcp_socket, buffer, RTP_MAX_PACKET_LEN);
	    rtp_process_ctrl(session, buffer, buflen);
	}
	return TRUE;
    }
    return FALSE;
}



/**
 * rtp_add_csrc:
 * @session: the session pointer (returned by rtp_init()) 
 * @csrc: Constributing SSRC identifier
 * 
 * Adds @csrc to list of contributing sources used in SDES items.
 * Used by mixers and transcoders.
 * 
 **/

gboolean rtp_add_csrc (RtpSession *session, guint32 csrc)
{
    /* Mark csrc as something for which we should advertise RTCP SDES items, */
    /* in addition to our own SDES.                                          */
    Source *s;

    s = create_source(session, csrc, FALSE);
    g_return_val_if_fail (s == NULL, FALSE);
    if (!s->should_advertise_sdes)
    {
	s->should_advertise_sdes = TRUE;
	session->csrc_count++;
    }
    return TRUE;
}



/**
 * rtp_del_csrc:
 * @session: the session pointer (returned by rtp_init()) 
 * @csrc: Constributing SSRC identifier
 * 
 * Removes @csrc from list of contributing sources used in SDES items.
 * Used by mixers and transcoders.
 * 
 * Return value: TRUE on success, FALSE if @csrc is not a valid source.
 **/

gboolean rtp_del_csrc (RtpSession *session, guint32 csrc)
{
    Source *s;

    s = get_source(session, csrc);
    if (s == NULL)
    {
	g_print("[RTP] rtp_del_csrc: invalid Source 0x%08x\n", csrc);
	return FALSE;
    }
    s->should_advertise_sdes = FALSE;
    session->csrc_count--;
    if (session->last_advertised_csrc >= session->csrc_count) session->last_advertised_csrc = 0;
    return TRUE;
}


/**
 * rtp_set_sdes:
 * @session: the session pointer (returned by rtp_init()) 
 * @ssrc: the SSRC identifier of a participant
 * @type: the SDES type represented by @value
 * @value: the SDES description
 * @length: the length of the description
 * 
 * Sets session description information associated with participant
 * @ssrc.  Under normal circumstances applications always use the
 * @ssrc of the local participant, this SDES information is
 * transmitted in receiver reports.  Setting SDES information for
 * other participants affects the local SDES entries, but are not
 * transmitted onto the network.
 * 
 * Return value: Returns TRUE if participant exists, FALSE otherwise.
 **/

gboolean rtp_set_sdes (RtpSession *session, guint32 ssrc, rtcp_sdes_type type, const gchar *value, gint length)
{
    Source *s;
    gchar *v;


    if ((value == NULL) || (length == 0)) return TRUE;
    s = get_source(session, ssrc);
    if (s == NULL) return FALSE;
    v = (gchar *) g_malloc0(length + 1);
    memcpy(v, value, length);
    switch (type)
    {
    case RTCP_SDES_CNAME:
	if (s->cname) g_free(s->cname);
	s->cname = v;
	break;
    case RTCP_SDES_NAME:
	if (s->name) g_free(s->name);
	s->name = v;
	break;
    case RTCP_SDES_EMAIL:
	if (s->email) g_free(s->email);
	s->email = v;
	break;
    case RTCP_SDES_PHONE:
	if (s->phone) g_free(s->phone);
	s->phone = v;
	break;
    case RTCP_SDES_LOC:
	if (s->loc) g_free(s->loc);
	s->loc = v;
	break;
    case RTCP_SDES_TOOL:
	if (s->tool) g_free(s->tool);
	s->tool = v;
	break;
    case RTCP_SDES_NOTE:
	if (s->note) g_free(s->note);
	s->note = v;
	break;
    case RTCP_SDES_PRIV:
	if (s->priv) g_free(s->priv);
	s->priv = v;
	break;
    default :
	g_free(v);
	return FALSE;
    }
    return TRUE;
}



/**
 * rtp_get_sdes:
 * @session: the session pointer (returned by rtp_init()) 
 * @ssrc: the SSRC identifier of a participant
 * @type: the SDES information to retrieve
 * 
 * Recovers session description (SDES) information on participant
 * identified with @ssrc.  The SDES information associated with a
 * Source is updated when receiver reports are received.  There are
 * several different types of SDES information, e.g. username,
 * location, phone, email.  These are enumerated by #rtcp_sdes_type.
 * 
 * Return value: pointer to string containing SDES description if
 * received, NULL otherwise.  
 */

const gchar *rtp_get_sdes (RtpSession *session, guint32 ssrc, rtcp_sdes_type type)
{
    Source *s;


    s = get_source(session, ssrc);
    if (s == NULL) return NULL;
    switch (type)
    {
        case RTCP_SDES_CNAME: return s->cname;
        case RTCP_SDES_NAME: return s->name;
        case RTCP_SDES_EMAIL: return s->email;
        case RTCP_SDES_PHONE: return s->phone;
        case RTCP_SDES_LOC: return s->loc;
        case RTCP_SDES_TOOL: return s->tool;
        case RTCP_SDES_NOTE: return s->note;
        case RTCP_SDES_PRIV: return s->priv;
        default:
	    /* This includes RTCP_SDES_PRIV and RTCP_SDES_END */
	    g_printerr("[RTP] rtp_get_sdes: unknown SDES item (type=%d)\n", type);
            return NULL;
    }
    return NULL;
}



/**
 * rtp_get_sr:
 * @session: the session pointer (returned by rtp_init()) 
 * @ssrc: identifier of Source
 * 
 * Retrieve the latest sender report made by sender with @ssrc identifier.
 * 
 * Return value: A pointer to an rtcp_sr structure on success, NULL
 * otherwise.  The pointer must not be freed.
 **/

const rtcp_sr *rtp_get_sr (RtpSession *session, guint32 ssrc)
{
    /* Return the last SR received from this ssrc. The */
    /* caller MUST NOT free the memory returned to it. */
    Source *s;


    s = get_source(session, ssrc);
    if (s == NULL) return NULL;
    return s->sr;
}



/**
 * rtp_get_rr:
 * @session: the session pointer (returned by rtp_init())
 * @reporter: participant originating receiver report
 * @reportee: participant included in receiver report
 * 
 * Retrieve the latest receiver report on @reportee made by @reporter.
 * Provides an indication of other receivers reception service.
 * 
 * Return value: A pointer to a rtcp_rr structure on success, NULL
 * otherwise.  The pointer must not be freed.
 **/

const rtcp_rr *rtp_get_rr (RtpSession *session, guint32 reporter, guint32 reportee)
{
    return get_rr(session, reporter, reportee);
}



/**
 * rtp_send_data:
 * @session: the session pointer (returned by rtp_init())
 * @rtp_ts: The timestamp reflects the sampling instant of the first octet of the RTP data to be sent.  The timestamp is expressed in media units.
 * @pt: The payload type identifying the format of the data.
 * @m: Marker bit, interpretation defined by media profile of payload.
 * @cc: Number of contributing sources (excluding local participant)
 * @csrc: Array of SSRC identifiers for contributing sources.
 * @data: The RTP data to be sent.
 * @data_len: The size @data in bytes.
 * @extn: Extension data (if present).
 * @extn_len: size of @extn in bytes.
 * @extn_type: extension type indicator.
 * 
 * Send an RTP packet.  Most media applications will only set the
 * @session, @rtp_ts, @pt, @m, @data, @data_len arguments.
 *
 * Mixers and translators typically set additional contributing sources 
 * arguments (@cc, @csrc).
 *
 * Extensions fields (@extn, @extn_len, @extn_type) are for including
 * application specific information.  When the widest amount of
 * inter-operability is required these fields should be avoided as
 * some applications discard packets with extensions they do not
 * recognize.
 * 
 * Return value: Number of bytes transmitted.
 **/

gint rtp_send_data (RtpSession *session, guint32 rtp_ts, gchar pt, gint m, gint cc, guint32* csrc,
		    gchar *data, gint data_len, gchar *extn, guint16 extn_len, guint16 extn_type)
{
    gint buffer_len, i, rc, pad, pad_len;
    guint8 *buffer;
    RtpPacket *packet;

#ifdef RTP_CRYPT_SUPPORT
    guint8 initVec[8] = {0,0,0,0,0,0,0,0};
#endif


    g_return_val_if_fail(data_len > 0, -1);
    buffer_len = data_len + 12 + (4 * cc);
    if (extn != NULL) buffer_len += (extn_len + 1) * 4;

#ifdef RTP_CRYPT_SUPPORT
    /* Do we need to pad this packet to a multiple of 64 bits? */
    /* This is only needed if encryption is enabled, since DES */
    /* only works on multiples of 64 bits. We just calculate   */
    /* the amount of padding to add here, so we can reserve    */
    /* space - the actual padding is added later.              */
    if ((session->encryption_enabled) && ((buffer_len % session->encryption_pad_length) != 0))
    {
	pad = TRUE;
	pad_len = session->encryption_pad_length - (buffer_len % session->encryption_pad_length);
	buffer_len += pad_len;
	g_return_val_if_fail((buffer_len % session->encryption_pad_length) == 0, -1);
    }
    else
    {
	pad = FALSE;
	pad_len = 0;
    }
#else

    pad = FALSE;
    pad_len = 0;

#endif

    /* Allocate memory for the packet... */
    buffer = (guint8 *) g_malloc(buffer_len + RTP_PACKET_HEADER_SIZE);
    packet = (RtpPacket *) buffer;

    /* These are internal pointers into the buffer... */
    packet->csrc = (guint32 *) (buffer + RTP_PACKET_HEADER_SIZE + 12);
    packet->extn = (guint8  *) (buffer + RTP_PACKET_HEADER_SIZE + 12 + (4 * cc));
    packet->data = (guint8  *) (buffer + RTP_PACKET_HEADER_SIZE + 12 + (4 * cc));
    if (extn != NULL) packet->data += (extn_len + 1) * 4;

    /* ...and the actual packet header... */
    packet->v = RTP_VERSION;
    packet->p = pad;
    packet->x = (extn != NULL);
    packet->cc = cc;
    packet->m = m;
    packet->pt = pt;
    packet->seq = g_htons(session->rtp_seq);
    packet->ts = g_htonl(rtp_ts);
    packet->ssrc = g_htonl(rtp_my_ssrc(session));
    session->rtp_seq++;

    /* ...now the CSRC list... */
    for (i = 0; i < cc; i++) packet->csrc[i] = g_htonl(csrc[i]);

    /* ...a header extension? */
    if (extn != NULL)
    {
	/* We don't use the packet->extn_type field here, that's for receive only... */
	guint16 *base = (guint16 *) packet->extn;
	base[0] = g_htons(extn_type);
	base[1] = g_htons(extn_len);
	memcpy(packet->extn + 4, extn, extn_len * 4);
    }
    /* ...and the media data... */
    memcpy(packet->data, data, data_len);
    /* ...and any padding... */
    if (pad)
    {
	for (i = 0; i < pad_len; i++) buffer[buffer_len + RTP_PACKET_HEADER_SIZE - pad_len + i] = 0;
	buffer[buffer_len + RTP_PACKET_HEADER_SIZE - 1] = (gchar) pad_len;
    }

#ifdef RTP_CRYPT_SUPPORT
    /* Finally, encrypt if desired... */
    if (session->encryption_enabled)
    {
	if ((buffer_len % session->encryption_pad_length) != 0)
	{
	    g_free(buffer);
            return -1;
	}
	(session->encrypt_func)(session, buffer + RTP_PACKET_HEADER_SIZE, buffer_len, initVec);
    }
#endif

    rc = udp_send(session->rtp_socket, buffer + RTP_PACKET_HEADER_SIZE, buffer_len);
    g_free(buffer);

    /* Update the RTCP statistics... */
    session->we_sent = TRUE;
    session->rtp_pcount += 1;
    session->rtp_bcount += buffer_len;
    gettimeofday(&session->last_rtp_send_time, NULL);
    return rc;
}



/**
 * rtp_send_ctrl:
 * @session: the session pointer (returned by rtp_init())
 * @rtp_ts: the current time expressed in units of the media timestamp.
 * @appcallback: a callback to create an APP RTCP packet, if needed.
 *
 * Checks RTCP timer and sends RTCP data when nececessary.  The
 * interval between RTCP packets is randomized over an interval that
 * depends on the session bandwidth, the number of participants, and
 * whether the local participant is a sender.  This function should be
 * called at least once per second, and can be safely called more
 * frequently.  
 */

void rtp_send_ctrl (RtpSession *session, guint32 rtp_ts, rtcp_app_callback appcallback)
{
    /* Send an RTCP packet, if one is due... */
    struct timeval curr_time;


    gettimeofday(&curr_time, NULL);
    if (gt_timeval(curr_time, session->next_rtcp_send_time))
    {
	/* The RTCP transmission timer has expired. The following */
	/* implements draft-ietf-avt-rtp-new-02.txt section 6.3.6 */
	gint h;
	Source *s;
	struct timeval new_send_time;
	gdouble new_interval;
	new_interval  = rtcp_interval(session) / (session->csrc_count + 1);
	new_send_time = session->last_rtcp_send_time;
	add_timeval(&new_send_time, new_interval);
	if (gt_timeval(curr_time, new_send_time))
	{
	    send_rtcp(session, rtp_ts, appcallback);
	    session->initial_rtcp = FALSE;
	    session->last_rtcp_send_time = curr_time;
	    session->next_rtcp_send_time = curr_time;
	    add_timeval(&(session->next_rtcp_send_time), rtcp_interval(session) / (session->csrc_count + 1));
	    /* We're starting a new RTCP reporting interval, zero out */
	    /* the per-interval statistics.                           */
	    session->sender_count = 0;
	    for (h = 0; h < RTP_DB_SIZE; h++)
	    {
		for (s = session->db[h]; s != NULL; s = s->next) s->sender = FALSE;

	    }
	}
	else session->next_rtcp_send_time = new_send_time;
	session->ssrc_count_prev = session->ssrc_count;
    }
    return;
}


/**
 * rtp_update:
 * @session: the session pointer (returned by rtp_init())
 *
 * Trawls through the internal data structures and performs
 * housekeeping.  This function should be called at least once per
 * second.  It uses an internal timer to limit the number of passes
 * through the data structures to once per second, it can be safely
 * called more frequently.
 */

void rtp_update (RtpSession *session)
{
    /* Perform housekeeping on the Source database... */
    gint h;
    Source *s, *n;
    struct timeval curr_time;
    gdouble delay;


    gettimeofday(&curr_time, NULL);
    if (diff_timeval(curr_time, session->last_update) < 1.0)
    {
	/* We only perform housekeeping once per second... */
	return;
    }
    session->last_update = curr_time;

    /* Update we_sent (section 6.3.8 of RTP spec) */
    delay = diff_timeval(curr_time, session->last_rtp_send_time);
    if (delay >= 2 * rtcp_interval(session)) session->we_sent = FALSE;

    for (h = 0; h < RTP_DB_SIZE; h++)
    {
	for (s = session->db[h]; s != NULL; s = n)
	{
	    n = s->next;
	    /* Expire sources which haven't been heard from for a long time.   */
	    /* Section 6.2.1 of the RTP specification details the timers used. */

	    /* How long since we last heard from this source?  */
	    delay = diff_timeval(curr_time, s->last_active);
			
	    /* Check if we've received a BYE packet from this Source.    */
	    /* If we have, and it was received more than 2 seconds ago   */
	    /* then the Source is deleted. The arbitrary 2 second delay  */
	    /* is to ensure that all delayed packets are received before */
	    /* the Source is timed out.                                  */
	    if (s->got_bye && (delay > 2.0))
	    {
		g_print("[RTP] rtp_update: deleting source 0x%08x due to reception of BYE %f seconds ago ...\n", s->ssrc, delay);
		delete_source(session, s->ssrc);
	    }
	    /* Sources are marked as inactive if they haven't been heard */
	    /* from for more than 2 intervals (RTP section 6.3.5)        */
	    if ((s->ssrc != rtp_my_ssrc(session)) && (delay > (session->rtcp_interval * 2)))
	    {
		if (s->sender)
		{
		    s->sender = FALSE;
		    session->sender_count--;
		}
	    }
	    /* If a source hasn't been heard from for more than 5 RTCP   */
	    /* reporting intervals, we delete it from our database...    */
	    if ((s->ssrc != rtp_my_ssrc(session)) && (delay > (session->rtcp_interval * 5)))
	    {
		g_print("[RTP] rtp_update: deleting source 0x%08x due to timeout...\n", s->ssrc);
		delete_source(session, s->ssrc);
	    }
	}
    }
    /* Timeout those reception reports which haven't been refreshed for a long time */
    timeout_rr(session, &curr_time);
    return;
}



/**
 * rtp_send_bye:
 * @session: The RTP session
 *
 * Sends a BYE message on the RTP session, indicating that this
 * participant is leaving the session. The process of sending a
 * BYE may take some time, and this function will block until 
 * it is complete. During this time, RTCP events are reported 
 * to the application via the callback function (data packets 
 * are silently discarded).
 */

gboolean rtp_send_bye (RtpSession *session)
{
    struct timeval curr_time, timeout, new_send_time;
    guint8 buffer[RTP_MAX_PACKET_LEN];
    gint buflen;
    gdouble new_interval;


    /* "...a participant which never sent an RTP or RTCP packet MUST NOT send  */
    /* a BYE packet when they leave the group." (section 6.3.7 of RTP spec)    */
    if ((session->we_sent == FALSE) && (session->initial_rtcp == TRUE))
    {
	g_print("[RTP] rtp_send_bye: silent BYE\n");
	return TRUE;
    }

    /* If the session is small, send an immediate BYE. Otherwise, we delay and */
    /* perform BYE reconsideration as needed.                                  */
    if (session->ssrc_count < RTP_SESSION_LIMIT_INMEDIATE_BYE)
    {
	return rtp_send_bye_now(session);
    }
    else
    {
	gettimeofday(&curr_time, NULL);
	session->sending_bye = TRUE;
	session->last_rtcp_send_time = curr_time;
	session->next_rtcp_send_time = curr_time;
	session->bye_count = 1;
	session->initial_rtcp = TRUE;
	session->we_sent = FALSE;
	session->sender_count = 0;
	session->avg_rtcp_size = 70.0 + RTP_LOWER_LAYER_OVERHEAD;	/* FIXME */
	add_timeval(&session->next_rtcp_send_time, rtcp_interval(session) / (session->csrc_count + 1));
	while (1)
	{
	    /* Schedule us to block in udp_select() until the time we are due to send our */
	    /* BYE packet. If we receive an RTCP packet from another participant before   */
	    /* then, we are woken up to handle it...                                      */
	    timeout.tv_sec = 0;
	    timeout.tv_usec = 0;
	    add_timeval(&timeout, diff_timeval(session->next_rtcp_send_time, curr_time));
	    udp_fd_zero();
	    udp_fd_set(session->rtcp_socket);
	    if ((udp_select(&timeout) > 0) && udp_fd_isset(session->rtcp_socket))
	    {
		/* We woke up because an RTCP packet was received; process it... */
		buflen = udp_recv(session->rtcp_socket, buffer, RTP_MAX_PACKET_LEN);
		rtp_process_ctrl(session, buffer, buflen);
	    }
	    /* Is it time to send our BYE? */
	    gettimeofday(&curr_time, NULL);
	    new_interval  = rtcp_interval(session) / (session->csrc_count + 1);
	    new_send_time = session->last_rtcp_send_time;
	    add_timeval(&new_send_time, new_interval);
	    if (gt_timeval(curr_time, new_send_time))
	    {
		g_print("[RTP] rtp_send_bye: Sending BYE ...\n");
		return rtp_send_bye_now(session);
	    }
	    /* No, we reconsider... */
	    session->next_rtcp_send_time = new_send_time;
	    /* ...and perform housekeeping in the usual manner */
	    rtp_update(session);
	}
    }
}



/**
 * rtp_done:
 * @session: the RTP session to finish
 *
 * Free the state associated with the given RTP session. This function does 
 * not send any packets (e.g. an RTCP BYE) - an application which wishes to
 * exit in a clean manner should call rtp_send_bye() first.
 */

void rtp_done (RtpSession *session, GError **error)
{
    gint i;
    Source *s, *n;
    GError *tmp_error1 = NULL;
    GError *tmp_error2 = NULL;


    /* In delete_source, check database gets called and this assumes */
    /* first added and last removed is us.                           */
    for (i = 0; i < RTP_DB_SIZE; i++)
    {
	s = session->db[i];
	while (s != NULL)
	{
	    n = s->next;
	    if (s->ssrc != session->my_ssrc) delete_source(session,session->db[i]->ssrc);
	    s = n;
	}
    }
    delete_source(session, session->my_ssrc);

#ifdef RTP_CRYPT_SUPPORT
    if (session->encryption_enabled)
    {
	if (strcmp(session->encryption_algorithm, "DES") == 0)
	{
	    g_print("[RTP] rtp_done: exiting encryption ... , algorithm was '%s'\n", session->encryption_algorithm);
	    des_exit(session);
            g_free(session->encryption_algorithm);
	}
	else
	{
	    g_print("[RTP] rtp_done: encryption algorithm \"%s\" not found\n", session->encryption_algorithm);
            g_free(session->encryption_algorithm);
	}
    }
#endif

    udp_exit(session->rtp_socket, &tmp_error1);
    if (tmp_error1 != NULL)
    {
	g_propagate_error(error, tmp_error1);
	g_printerr("[RTP] rtp_done: %s\n", tmp_error1->message);
	udp_exit(session->rtcp_socket, NULL);
	g_free(session->addr);
	g_free(session->opt);
	g_free(session);
	return;
    }
    udp_exit(session->rtcp_socket, &tmp_error2);
    if (tmp_error2 != NULL)
    {
	g_propagate_error(error, tmp_error2);
	g_printerr("[RTP] rtp_done: %s\n", tmp_error2->message);
	g_free(session->addr);
	g_free(session->opt);
	g_free(session);
	return;
    }
    g_free(session->addr);
    g_free(session->opt);
    g_free(session);
}


#ifdef RTP_CRYPT_SUPPORT

/**
 * rtp_set_encryption_key:
 * @session: The RTP session.
 * @passphrase: The user-provided "pass phrase" to map to an encryption key.
 *
 * Converts the user supplied key into a form suitable for use with RTP
 * and install it as the active key. Passing in NULL as the passphrase
 * disables encryption. The passphrase is converted into a DES key as
 * specified in RFC1890, that is:
 * 
 *   - convert to canonical form
 * 
 *   - derive an MD5 hash of the canonical form
 * 
 *   - take the first 56 bits of the MD5 hash
 * 
 *   - add parity bits to form a 64 bit key
 * 
 * Note that versions of rat prior to 4.1.2 do not convert the passphrase
 * to canonical form before taking the MD5 hash, and so will
 * not be compatible for keys which are non-invarient under this step.
 *
 * Determine from the user's encryption key which encryption
 * mechanism we're using. Per the RTP RFC, if the key is of the form
 *
 *	string/key
 *
 * then "string" is the name of the encryption algorithm,  and
 * "key" is the key to be used. If no / is present, then the
 * algorithm is assumed to be (the appropriate variant of) DES.
 *
 * Returns: TRUE on success, FALSE on failure.
 */

gboolean rtp_set_encryption_key (RtpSession* session, gchar *passphrase, GError **error)
{
    gchar *canonical_passphrase;
    guchar hash[16];
    MD5_CTX context;
    //gchar *slash;
    GError *tmp_error = NULL;
    gsize *g_sizer = 0, *g_sizew = 0;
    gchar *aux_passphrase;


    if (session->encryption_algorithm != NULL)
    {
	g_free(session->encryption_algorithm);
	session->encryption_algorithm = NULL;
    }
    if (passphrase == NULL)
    {
	/* A NULL passphrase means disable encryption... */
	session->encryption_enabled = 0;
	return FALSE;
    }
    g_print("[RTP] rtp_set_encryption_key: enabling RTP/RTCP encryption\n");
    session->encryption_enabled = 1;

    /* Determine which algorithm we're using. */
    //slash = strchr(passphrase, '/');
    //if (slash == 0) session->encryption_algorithm = g_strdup("DES");
    //else
    //{
    //    gint l = slash - passphrase;
    //    session->encryption_algorithm = g_strndup(passphrase, l);
    //    session->encryption_algorithm[l] = '\0';
    //    passphrase = slash + 1;
    //}
    session->encryption_algorithm = g_strdup("DES");
    g_print("[RTP] rtp_set_encryption_key: initializing encryption, algorithm is '%s'\n", session->encryption_algorithm);

    /* Step 1: convert to canonical form, comprising the following steps:  */
    /*   a) convert the input string to the ISO 10646 character set, using */
    /*      the UTF-8 encoding as specified in Annex P to ISO/IEC          */
    /*      10646-1:1993 (ASCII characters require no mapping, but ISO     */
    /*      8859-1 characters do);                                         */
    /*   b) remove leading and trailing white space characters;            */
    /*   c) replace one or more contiguous white space characters by a     */
    /*      single space (ASCII or UTF-8 0x20);                            */
    /*   d) convert all letters to lower case and replace sequences of     */
    /*      characters and non-spacing accents with a single character,    */
    /*      where possible.                                                */

    aux_passphrase = (gchar *) g_ascii_strdown(passphrase, -1);
    g_strstrip(aux_passphrase);
    canonical_passphrase = g_locale_to_utf8(aux_passphrase, -1, g_sizer, g_sizew, &tmp_error);
    if (canonical_passphrase == NULL)
    {
	g_free(aux_passphrase);
	g_printerr("[RTP] rtp_set_encryption_key: canonical_passphrase, %s\n", tmp_error->message);
        g_propagate_error(error, tmp_error);
	return FALSE;
    }
    g_free(aux_passphrase);

    /* Step 2: derive an MD5 hash */
    MD5Init(&context);
    MD5Update(&context, (guchar *) canonical_passphrase, strlen(canonical_passphrase));
    MD5Final((guchar *) hash, &context);

    g_free(aux_passphrase);

    /* Initialize the encryption algorithm we've received */
    if (strcmp(session->encryption_algorithm, "DES") == 0)
    {
	des_initialize(session, hash, &tmp_error);
	if (tmp_error != NULL)
	{
	    g_propagate_error (error, tmp_error);
	    g_printerr("[RTP] rtp_set_encryption_key: canonical_passphrase, %s\n", tmp_error->message);
	    return FALSE;
	}
    }
    else
    {
	g_print("[RTP] rtp_set_encryption_key: encryption algorithm \"%s\" not found\n", session->encryption_algorithm);
	return FALSE;
    }
    return TRUE;
}

#endif

/**
 * rtp_get_addr:
 * @session: The RTP Session.
 *
 * Returns: The session's destination address, as set when creating the
 * session with rtp_init() or rtp_init_if().
 */

gchar *rtp_get_addr (RtpSession *session)
{
	return session->addr;
}



/**
 * rtp_get_rx_port:
 * @session: The RTP Session.
 *
 * Returns: The UDP port to which this session is bound, as set when
 * creating the session with rtp_init() or rtp_init_if().
 */

guint16 rtp_get_rx_port (RtpSession *session)
{
    return session->rx_port;
}



/**
 * rtp_get_tx_port:
 * @session: The RTP Session.
 *
 * Returns: The UDP port to which RTP packets are transmitted, as set
 * when creating the session with rtp_init() or rtp_init_if().
 */

guint16 rtp_get_tx_port (RtpSession *session)
{
	return session->tx_port;
}



/**
 * rtp_get_ttl:
 * @session: The RTP Session.
 *
 * Returns: The session's TTL, as set when creating the session with
 * rtp_init() or rtp_init_if().
 */

gint rtp_get_ttl (RtpSession *session)
{
    return session->ttl;
}

