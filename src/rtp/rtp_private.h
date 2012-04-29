
/*
 *
 * FILE: rtp_private.h
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

#ifndef _RTP_PRIVATE_H_
#define _RTP_PRIVATE_H_


#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <glib.h>
#include <string.h>
#include <math.h>

#include "rtp.h"
#include "../net/udp.h"
#include "../net/net.h"

#ifdef RTP_CRYPT_SUPPORT
#include "../crypt/md5.h"
#include "rtp_des.h"
#endif



#define RTP_MAX_ENCRYPTION_PAD 16



/* function update_seq, taken from draft-ietf-avt-rtp-new-01.txt  */
#define RTP_MAX_DROPOUT    3000
#define RTP_MAX_MISORDER   100
#define RTP_MIN_SEQUENTIAL 1




/* Definitions for the RTP/RTCP packets on the wire ...           */

#define RTP_SEQ_MOD        0x10000
#define RTP_MAX_SDES_LEN   256

#define RTP_LOWER_LAYER_OVERHEAD 28                /* IPv4 + UDP  */




#define RTCP_SR   200
#define RTCP_RR   201
#define RTCP_SDES 202
#define RTCP_BYE  203
#define RTCP_APP  204



typedef struct
{
#if G_BYTE_ORDER == G_BIG_ENDIAN
    unsigned short  version:2;	        /* packet type            */
    unsigned short  p:1;		/* padding flag           */
    unsigned short  count:5;	        /* varies by payload type */
    unsigned short  pt:8;		/* payload type           */
#elif G_BYTE_ORDER == G_LITTLE_ENDIAN
    unsigned short  count:5;            /* varies by payload type */
    unsigned short  p:1;		/* padding flag           */
    unsigned short  version:2;	        /* packet type            */
    unsigned short  pt:8;		/* payload type           */
#else
#error "G_BYTE_ORDER should be big or little endian."
#endif
    guint16         length;		/* packet length          */
} RtcpCommon;




typedef struct
{
    RtcpCommon   common;
    union
    {
	struct
	{
	    rtcp_sr     sr;
	    rtcp_rr   	rr[1]; /* variable-length list */
	} sr;

	struct
	{
	    guint32     ssrc;  /* source this RTCP packet is coming from */
	    rtcp_rr   	rr[1]; /* variable-length list */
	} rr;

	struct rtcp_sdes_t
	{
	    guint32	        ssrc;
	    rtcp_sdes_item 	item[1]; /* list of SDES */
	} sdes;

	struct
	{
	    guint32     ssrc[1]; /* list of sources */
	/* can't express the trailing text... */
	} bye;

	struct
	{
	    guint32     ssrc;
	    guint8      name[4];
	    guint8      data[1];
	} app;
    } r;
} rtcp_t;




typedef struct _rtcp_rr_wrapper
{
    struct _rtcp_rr_wrapper	*next;
    struct _rtcp_rr_wrapper	*prev;
    guint32                     reporter_ssrc;
    rtcp_rr			*rr;
    struct timeval		*ts;      /* Arrival time of this RR */
} rtcp_rr_wrapper;




/* The RTP database contains source-specific information needed       */
/* to make it all work.                                               */

typedef struct _source
{
	struct _source	*next;
	struct _source	*prev;
	guint32	         ssrc;
	gchar		*cname;
	gchar		*name;
	gchar		*email;
	gchar		*phone;
	gchar		*loc;
	gchar		*tool;
	gchar		*note;
	gchar		*priv;
	rtcp_sr		*sr;
	struct timeval	 last_sr;
	struct timeval	 last_active;
	gint		 should_advertise_sdes;	/* TRUE if this source is a CSRC which we need to advertise SDES for */
	gint		 sender;
	gint		 got_bye;		/* TRUE if we've received an RTCP bye from this source */
	guint32	         base_seq;
	guint16 	 max_seq;
	guint32	         bad_seq;
	guint32	         cycles;
	gint		 received;
	gint		 received_prior;
	gint		 expected_prior;
	gint		 probation;
	guint32	         jitter;
	guint32	         transit;
} Source;

/* The size of the hash table used to hold the source database. */
/* Should be large enough that we're unlikely to get collisions */
/* when sources are added, but not too large that we waste too  */
/* much memory. Sedgewick ("Algorithms", 2nd Ed, Addison-Wesley */
/* 1988) suggests that this should be around 1/10th the number  */
/* of entries that we expect to have in the database and should */
/* be a prime number. Everything continues to work if this is   */
/* too low, it just goes slower... for now we assume around 100 */
/* participants is a sensible limit so we set this to 11.       */   
#define RTP_DB_SIZE	11




#define RTP_INIT_SEED_STRING "ARANDOMSTRINGSOWEDONTCOREDUMP"




/* function rtp_send_bye */

#define RTP_SESSION_LIMIT_INMEDIATE_BYE 50




/* Options for an RTP session are stored in the "Options" struct*/

typedef struct
{
    gint promiscuous_mode;
    gint wait_for_rtcp;
    gint filter_my_packets;
    gint reuse_bufs;
} Options;




/* Encryption function types */

typedef gint (*RtpEncryptFunction)(RtpSession *, guchar *data, guint size, guchar *initvec);
typedef gint (*RtpDecryptFunction)(RtpSession *, guchar *data, guint size, guchar *initvec);




/* The "RtpSession" defines an RTP session. */
//typedef struct _rtp_session RtpSession;


struct _rtp_session
{
	Socket_udp	*rtp_socket;
	Socket_udp	*rtcp_socket;
	gchar		*addr;
	guint16	         rx_port;
	guint16	         tx_port;
	gint		 ttl;
	guint32	         my_ssrc;
	gint		 last_advertised_csrc;
	Source		*db[RTP_DB_SIZE];
        rtcp_rr_wrapper  rr[RTP_DB_SIZE][RTP_DB_SIZE]; 	/* Indexed by [hash(reporter)][hash(reportee)] */
	Options		*opt;
	gpointer       	 userdata;
	gint		 invalid_rtp_count;
	gint		 invalid_rtcp_count;
	gint		 bye_count;
	gint		 csrc_count;
	gint		 ssrc_count;
	gint		 ssrc_count_prev;		/* ssrc_count at the time we last recalculated our RTCP interval */
	gint		 sender_count;
	gint		 initial_rtcp;
	gint		 sending_bye;			/* TRUE if we're in the process of sending a BYE packet */
	gdouble		 avg_rtcp_size;
	gint		 we_sent;
	gdouble		 rtcp_bw;			/* RTCP bandwidth fraction, in octets per second. */
	struct timeval	 last_update;
	struct timeval	 last_rtp_send_time;
	struct timeval	 last_rtcp_send_time;
	struct timeval	 next_rtcp_send_time;
	gdouble		 rtcp_interval;
	gint		 sdes_count_pri;
	gint		 sdes_count_sec;
	gint		 sdes_count_ter;
	guint16	         rtp_seq;
	guint32	         rtp_pcount;
	guint32	         rtp_bcount;
	gchar 		*encryption_algorithm;
 	gint 		 encryption_enabled;
 	RtpEncryptFunction encrypt_func;
 	RtpDecryptFunction decrypt_func;
 	gint 		 encryption_pad_length;
	union
	{
	    struct
	    {
		gchar     *encryption_key;
	    } des;
	} crypto_state;

	rtp_callback	 callback_rtp;
	rtp_callback	 callback_rtcp;
};




#define RTP_MAXCNAMELEN	255



#define SECS_BETWEEN_1900_1970 2208988800u




#define ntp64_to_ntp32(ntp_sec, ntp_frac) ((((ntp_sec)  & 0x0000ffff) << 16) | (((ntp_frac) & 0xffff0000) >> 16))

inline void ntp64_time(guint32 *ntp_sec, guint32 *ntp_frac);
inline gdouble diff_timeval (struct timeval curr_time, struct timeval prev_time);
inline void add_timeval (struct timeval *ts, gdouble offset);
inline gint gt_timeval (struct timeval a, struct timeval b);

gchar *get_cname (Socket_udp *s);

void init_rng (const gchar *s);


#endif


