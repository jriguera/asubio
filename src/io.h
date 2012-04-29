
/*
 *
 * FILE: io.h
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

#ifndef _IO_H_
#define _IO_H_


#include <glib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>

#include "audio.h"
#include "audio_buffer.h"
#include "audio_util.h"
#include "rtp/rtp.h"
#include "plugin_inout.h"
#include "plugin_effect.h"
#include "configfile.h"
#include "audio_processor.h"
#include "audio_convert.h"
#include "audio_util.h"
#include "codec_plugin/codec.h"




#define IO_ERROR                5010
#define IO_OK                   5000
#define IO_ERROR_CONTROL_QUEUE  5020



// NULL Codec -> RAW audio

#define IO_PCM_SIZE        512
#define IO_PCM_BW          128000
#define IO_PCM_MS          32
#define IO_PCM_PAYLOAD     23
#define IO_PCM_NAME        "Internal PCM 16 bits"
#define IO_PCM_DESCRIPTION "RAW audio PCM 16 bits (no compress) 'internal pseudocodec'"




struct _NodeVoCodec
{
    GModule *module;
    Codec *codec;
    Plugin_Codec *pc;
    gint selected_rx;
    gint selected_tx;
    gint id;
};

typedef struct _NodeVoCodec NodeVoCodec;




struct _RTPUserdata
{
    GAsyncQueue *rtcp;
    gint32 rtp_ts;
    AudioProperties *ap;
    ABuffer *ab;
    GMutex *mutex;
    gint *mute_vol;

    gint audio_blocksize_dsp;

    NodeVoCodec *node_codec_rx;
    gint audio_blocksize_rx;
    gint audio_ms_rx;
    gint data_len_rx;
    guchar *buffer_data_rx;
    gint payload_rx;
    gfloat *audio_gaim_rx;

    NodeVoCodec *node_codec_tx;
    gint audio_blocksize_tx;
    gint audio_ms_tx;
    gint payload_tx;
    gint data_len_tx;
};

typedef struct _RTPUserdata RTPUserdata;




struct _ThPIn
{
    AudioProperties *ap;
    RtpSession *rtps;
    ABuffer *ab;
    gboolean process;
    glong rounds;
    gint audio_blocksize_rx;
    gint payload_rx;
    gint data_len_rx;
    gint audio_ms_rx;
    gint usec_recv_timeout_block;
};

typedef struct _ThPIn ThPIn;




struct _ThPOut
{
    AudioProperties *ap;
    RtpSession *rtps;
    ABuffer *ab;
    VAD_State *vad_state;
    gboolean process;
    glong rounds;
    gint audio_blocksize_tx;
    gint payload_tx;
    gint data_len_tx;
    gint audio_ms_tx;
    NodeVoCodec *node_codec_tx;
    gint *mute_mic;
    gint *vad;
};

typedef struct _ThPOut ThPOut;




struct _RTP_Info
{
    gint next_notify;
    GMutex *mutex;
    gint num_sources;
    gboolean change_payload;
    gint payload;
    gboolean timeout;
    gboolean bye;
    gboolean show;
    guint32 ssrcs[10];
    gchar sdes_info[10][5][256];
};

typedef struct _RTP_Info RTP_Info;




struct _ASession
{
    AudioProperties *ap_dsp;
    gint blocksize_dsp;
    RtpSession *rtps;
    NodeVoCodec *node_codec_tx;
    NodeVoCodec *node_codec_rx;
    gint id_queue_in;
    GAsyncQueue *queue_in;
    gint id_queue_out;
    GAsyncQueue *queue_out;
    gboolean process;
    gboolean bye;
    VAD_State **vad_state;
    gint *vad;
    gint *mute_mic;
    gint *mute_vol;
    gfloat *gaim_rx;

    RTP_Info *rtpinfo;
    gboolean crypt;
    GSList *plugins_effect;
    GSList *plugins_auth;
};

typedef struct _ASession ASession;




struct _InitRTPsession
{
    gchar *addr;
    guint16 rx_port;
    guint16 tx_port;
    gint ttl;
    gdouble rtcp_bw;
    gchar *username;
    gchar *toolname;
    gchar *usermail;
    gchar *userlocate;
    gchar *usernote;
};

typedef struct _InitRTPsession InitRTPsession;




enum _RtcpAppSubtype
{
    LOCAL_RTCP_APP,
    REMOTE_RTCP_APP,
};

typedef enum _RtcpAppSubtype RtcpAppSubtype;




enum _RtcpAppData
{
    RTCP_APP_CHANGE_PAYLOAD,
    RTCP_APP,
};

typedef enum _RtcpAppData RtcpAppData;



// Functions


RtpSession *create_rtp_session (InitRTPsession *init, GError **error);
void destroy_rtp_session (RtpSession *session, GError **error);

gpointer session_io_loop (gpointer data);


#endif


