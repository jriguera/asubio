
/*
 *
 * FILE: rtp.h
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

#ifndef _RTP_H_
#define _RTP_H_


#include <glib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include "../common.h"




#define RTP_VERSION 2

#define RTP_HEADER_SIZE 12

#define RTP_MAX_PACKET_LEN 1500




#define RTP_ERROR         10
#define RTP_ERROR_PORT    20
#define RTP_ERROR_ERRNO   30
#define RTP_OK            40




#define RTP_PACKET_HEADER_SIZE sizeof(struct _rtp_header_aux)

typedef struct
{
    /* The following are pointers to the data in the packet as    */
    /* it came off the wire. The packet it read in such that the  */
    /* header maps onto the latter part of this struct, and the   */
    /* fields in this first part of the struct point into it. The */
    /* entire packet can be freed by freeing this struct, without */
    /* having to free the csrc, data and extn blocks separately.  */

    struct _rtp_header_aux
    {
	guint32	  *csrc;
	gchar     *data;
	gint      data_len;
	guchar    *extn;

	/* Size of the extension in 32 bit words minus one        */
	guint16	  extn_len;

	/* Extension type field in the RTP packet header          */
	guint16	  extn_type;
    };

    /* The following map directly onto the RTP packet header...   */
#if G_BYTE_ORDER == G_BIG_ENDIAN
    unsigned short   v:2;      	/* packet type                    */
    unsigned short   p:1;      	/* padding flag                   */
    unsigned short   x:1;      	/* header extension flag          */
    unsigned short   cc:4;     	/* CSRC count                     */
    unsigned short   m:1;      	/* marker bit                     */
    unsigned short   pt:7;      /* payload type                   */
#elif G_BYTE_ORDER == G_LITTLE_ENDIAN
    unsigned short   cc:4;     	/* CSRC count                     */
    unsigned short   x:1;      	/* header extension flag          */
    unsigned short   p:1;      	/* padding flag                   */
    unsigned short   v:2;      	/* packet type                    */
    unsigned short   pt:7;     	/* payload type                   */
    unsigned short   m:1;      	/* marker bit                     */
#else
#error "G_BYTE_ORDER should be big or little endian."
#endif
    guint16          seq;     	/* sequence number                */
    guint32          ts;      	/* timestamp                      */
    guint32          ssrc;    	/* synchronization source         */

    /* The csrc list, header extension and data follow, but can't */
    /* be represented in the struct.                              */
} RtpPacket;




typedef struct
{
    guint32 ssrc;
    guint32 ntp_sec;
    guint32 ntp_frac;
    guint32 rtp_ts;
    guint32 sender_pcount;
    guint32 sender_bcount;
} rtcp_sr;



typedef struct
{
    guint32	ssrc;		/* The ssrc to which this RR pertains */
#if G_BYTE_ORDER == G_BIG_ENDIAN
    guint32	fract_lost:8;
    guint32	total_lost:24;
#elif G_BYTE_ORDER == G_LITTLE_ENDIAN
    guint32	total_lost:24;
    guint32	fract_lost:8;
#else
#error "G_BYTE_ORDER should be big or little endian."
#endif
    guint32	last_seq;
    guint32	jitter;
    guint32	lsr;
    guint32	dlsr;
} rtcp_rr;




typedef struct
{
#if G_BYTE_ORDER == G_BIG_ENDIAN
    unsigned short  version:2;	/* RTP version            */
    unsigned short  p:1;       	/* padding flag           */
    unsigned short  subtype:5;	/* application dependent  */
#elif G_BYTE_ORDER == G_LITTLE_ENDIAN
    unsigned short  subtype:5;	/* application dependent  */
    unsigned short  p:1;       	/* padding flag           */
    unsigned short  version:2;	/* RTP version            */
#else
#error "G_BYTE_ORDER should be big or little endian."
#endif
    unsigned short  pt:8;      	/* packet type            */
    guint16         length;    	/* packet length          */
    guint32         ssrc;
    gchar           name[4];    /* four ASCII characters  */
    gchar           data[1];    /* variable length field  */
} rtcp_app;




/* RtpEvent type values. */
typedef enum
{
        RX_RTP,
        RX_SR,
        RX_RR,
        RX_SDES,
        RX_BYE,         /* Source is leaving the session, database entry is still valid                           */
        SOURCE_CREATED,
        SOURCE_DELETED, /* Source has been removed from the database                                              */
        RX_RR_EMPTY,    /* We've received an empty reception report block                                         */
        RX_RTCP_START,  /* Processing a compound RTCP packet about to start. The SSRC is not valid in this event. */
        RX_RTCP_FINISH,	/* Processing a compound RTCP packet finished. The SSRC is not valid in this event.       */
        RR_TIMEOUT,
        RX_APP
} rtp_event_type;




typedef struct
{
	guint32	        ssrc;
	rtp_event_type  type;
	gpointer        data;
	struct timeval 	*ts;
} RtpEvent;




/* SDES packet types... */

typedef enum
{
        RTCP_SDES_END   = 0,
        RTCP_SDES_CNAME = 1,
        RTCP_SDES_NAME  = 2,
        RTCP_SDES_EMAIL = 3,
        RTCP_SDES_PHONE = 4,
        RTCP_SDES_LOC   = 5,
        RTCP_SDES_TOOL  = 6,
        RTCP_SDES_NOTE  = 7,
        RTCP_SDES_PRIV  = 8
} rtcp_sdes_type;




typedef struct
{
	guint8		type;		/* type of SDES item              */
	guint8		length;		/* length of SDES item (in bytes) */
	gchar		data[1];	/* text, not zero-terminated      */
} rtcp_sdes_item;




/* RTP Options */

typedef enum
{
        RTP_OPT_PROMISC =	    1,
        RTP_OPT_WEAK_VALIDATION	=   2,
        RTP_OPT_FILTER_MY_PACKETS = 3,
	RTP_OPT_REUSE_PACKET_BUFS = 4  /* Each data packet is written into the same buffer, */
	                               /* rather than malloc()ing a new buffer each time.   */
} rtp_option;




typedef struct _rtp_session RtpSession;



/* Callback types */

typedef void (*rtp_callback)(RtpSession *session, RtpEvent *e);
typedef rtcp_app* (*rtcp_app_callback)(RtpSession *session, guint32 rtp_ts, gint max_size);




RtpSession *rtp_init (const gchar *addr, gchar *iface, guint16 rx_port, guint16 tx_port, gint ttl, gdouble rtcp_bw,
		      rtp_callback callback_rtp, rtp_callback callback_rtcp, gpointer userdata, GError **error);
gboolean rtp_set_callback (RtpSession *session, rtp_callback callback_rtp, rtp_callback callback_rtcp);
gboolean rtp_set_my_ssrc (RtpSession *session, guint32 ssrc);
gboolean rtp_set_option (RtpSession *session, rtp_option optname, gboolean optval);
gboolean rtp_get_option (RtpSession *session, rtp_option optname, gint *optval);
gpointer rtp_get_userdata (RtpSession *session);
void rtp_set_userdata (RtpSession *session, gpointer userdata);
guint32 rtp_my_ssrc (RtpSession *session);
gboolean rtp_recv (RtpSession *session, struct timeval *timeout, guint32 curr_rtp_ts);
gboolean rtp_add_csrc (RtpSession *session, guint32 csrc);
gboolean rtp_del_csrc (RtpSession *session, guint32 csrc);
gboolean rtp_set_sdes (RtpSession *session, guint32 ssrc, rtcp_sdes_type type, const gchar *value, gint length);
const gchar *rtp_get_sdes (RtpSession *session, guint32 ssrc, rtcp_sdes_type type);
const rtcp_sr *rtp_get_sr (RtpSession *session, guint32 ssrc);
const rtcp_rr *rtp_get_rr (RtpSession *session, guint32 reporter, guint32 reportee);
gint rtp_send_data (RtpSession *session, guint32 rtp_ts, gchar pt, gint m, gint cc, guint32* csrc,
		    gchar *data, gint data_len, gchar *extn, guint16 extn_len, guint16 extn_type);
void rtp_send_ctrl (RtpSession *session, guint32 rtp_ts, rtcp_app_callback appcallback);
void rtp_update (RtpSession *session);
gboolean rtp_send_bye (RtpSession *session);
void rtp_done (RtpSession *session, GError **error);

#ifdef RTP_CRYPT_SUPPORT
gboolean rtp_set_encryption_key (RtpSession* session, gchar *passphrase, GError **error);
#endif

gchar *rtp_get_addr (RtpSession *session);
guint16 rtp_get_rx_port (RtpSession *session);
guint16 rtp_get_tx_port (RtpSession *session);
gint rtp_get_ttl (RtpSession *session);


#endif


