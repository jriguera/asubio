
/*
 *
 * FILE: oss.h
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

#ifndef _OSS_H_
#define _OSS_H_


#include <sys/soundcard.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <gtk/gtk.h>

#include "../../plugin_inout.h"




#define OSS_NUMBER_FRAGS  0x7fff0000


#define OSS_NAME          "OSS Plugin"
#define OSS_FILENAME      "liboss.so"
#define OSS_AUTOR_NAME    "Jose Riguera"
#define OSS_AUTOR_MAIL    "<jriguera@gmail.com>"
#define OSS_AUTOR_DATE    "Mayo 2003"
#define OSS_SHORT_LICENSE "GPL"
#define OSS_LONG_LICENSE  "This program is free software; you can redistribute it and/or modify \
it under the terms of the GNU General Public License as published by \
the Free Software Foundation; either version 2 of the License, or \
(at your option) any later version.\n \
This program is distributed in the hope that it will be useful,\
but WITHOUT ANY WARRANTY; without even the implied warranty of \
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the \
GNU General Public License for more details.\n \
You should have received a copy of the GNU General Public License \
along with this program; if not, write to the Free Software \
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, \
USA."


#define OSS_CFG_SECTION          "OSS"
#define OSS_CFG_AUDIO_DEVICE_IN  "audio_device_in"
#define OSS_CFG_MIXER_DEVICE_IN  "mixer_device_in"
#define OSS_CFG_AUDIO_DEVICE_OUT "audio_device_out"
#define OSS_CFG_MIXER_DEVICE_OUT "mixer_device_out"
#define OSS_CFG_SOURCE_IN        "audio_source"
#define OSS_CFG_VOL_CONTROL      "volume_control"


struct _Oss_Conf
{
    gchar *audio_device_in;
    gchar *mixer_device_in;
    gboolean mixer_in;
    gint fd_audio_device_in;
    gint audio_source_in;
    gint audio_volume_in;
    gint audio_channels_in ;
    gint audio_bits_in;
    gint audio_rate_in;
    gint audio_format_in;
    gint audio_blocksize_in;

    gchar *audio_device_out;
    gchar *mixer_device_out;
    gboolean mixer_out;
    gint fd_audio_device_out;
    gint audio_source_out;
    gint audio_volume_out;
    gint audio_channels_out;
    gint audio_bits_out;
    gint audio_rate_out;
    gint audio_format_out;
    gint audio_blocksize_out;

    gint volume_control_pcm;
    gboolean audio_full_duplex;
};
typedef struct _Oss_Conf Oss_Conf;



#endif


