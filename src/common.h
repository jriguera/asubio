
/*
 *
 * FILE: common.h
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

#ifndef _COMMON_H_
#define _COMMON_H_


#include <glib.h>




#define PROGRAM_NAME    "asubio"
#define PROGRAM_VERSION "0.10"
#define PROGRAM_DATE    "Junio 2003"
#define PROGRAM_AUTHOR  "Jose Riguera"
#define PROGRAM_AMAIL   "jriguera@gmail.com"



//#define NET_HAVE_IPv6

//#define RTP_CRYPT_SUPPORT

#define HAVE_NANOSLEEP

#define SYSLOG_MESG

//#define A_PROCESSOR_DEGUG

#ifdef NET_HAVE_IPv6
#define PROGRAM_LOCALHOST "::1"
#else
#define PROGRAM_LOCALHOST "127.0.0.1"
#endif

#define UNUSED(x) (x=x)




#define PROGRAM_DEFAULT_CONFFILE       "asubio.conf"
#define PROGRAM_DEFAULT_DIR            ".asubio"


#define PROGRAM_GLOBAL_SECTION         "GLOBAL"
#define PROGRAM_DEFAULT_IOP            "default_inout_plugin"
#define PROGRAM_DEFAULT_AUDIOBLOCKSIZE "audio_blocksize"
#define PROGRAM_AUDIO_IN_GAIM          "audio_mic_vol"
#define PROGRAM_AUDIO_OUT_VOL          "audio_spk_vol"
#define PROGRAM_AUDIO_CNG              "audio_vad"
#define PROGRAM_AUDIO_VAD              "audio_cng"
#define PROGRAM_DIR_EFFECT             "plugins_effect_dir"
#define PROGRAM_DIR_CODECS             "plugins_codecs_dir"
#define PROGRAM_DIR_INOUT              "plugins_inout_dir"
#define PROGRAM_DIR_DB                 "contact_db"
#define PROGRAM_DEFAULT_CODEC          "default_codec_payload"
#define PROGRAM_EXEC_ONCALL_IGNORED    "exec_on_call_ignored"
#define PROGRAM_EXEC_ONCALL            "exec_on_call"
#define PROGRAM_RTP_BASE_PORT          "rtp_base_port"
#define PROGRAM_HOSTNAME               "hostname"
#define PROGRAM_SSIP_LOCALHOST_PORT    "ssip_localhost_port"
#define PROGRAM_SSIP_PROXY             "ssip_proxy"
#define PROGRAM_SSIP_REMOTE_HOST       "ssip_remote_host"
#define PROGRAM_SSIP_REMOTE_HOST_PORT  "ssip_remote_host_port"
#define PROGRAM_NAME_USER              "name_user"
#define PROGRAM_MAIL_USER              "mail_user"
#define PROGRAM_DESCRIPTION_USER       "description_user"
#define PROGRAM_SSIP_LOCAL_HOST_PORT   "ssip_notify_host_port"
#define PROGRAM_SSIP_LOCAL_HOST        "ssip_notify_host"
#define PROGRAM_SSIP_PASS              "ssip_notify_pass"


#define PROGRAM_ANILOGO_INCOMING_CALL    "pixmaps/officer.gif"
#define PROGRAM_INCOMING_LOST_IMAGE_CALL "pixmaps/elton.gif"



#define PROGRAM_ERROR                        1010
#define PROGRAM_ERROR_OPEN_DIR               1030
#define PROGRAM_ERROR_RTP_BASE_PORT          1050
#define PROGRAM_ERROR_HOSTNAME               1060
#define PROGRAM_ERROR_SSIP_LOCALHOST_PORT    1070
#define PROGRAM_ERROR_SSIP_REMOTE_HOST       1080
#define PROGRAM_ERROR_SSIP_REMOTE_HOST_PORT  1090



#define AUDIO_HZ             8000
#define AUDIO_FORMAT         FMT_S16_LE
#define AUDIO_FORMAT_BYTES   2
#define AUDIO_DEFAULT_BLOCK  1024



#define PROGRAM_RTP_TCP_TTL   16
#define PROGRAM_RTP_MAX_BW    64000
#define PROGRAM_RTP_TOOLNAME  PROGRAM_NAME


#define PROGRAM_SHELL         "/bin/sh"




void bury_child (gint signal);

inline gint square_2 (gint p);

void execute_command (gchar *cmd);



#endif


