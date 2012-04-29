
/*
 *
 * FILE: codec.c
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

#include "codec.h"



inline gboolean codec_init (Codec *codec, AudioProperties *ap, ConfigFile *config, CodecMode mode, GError **error)
{
    GError *tmp_error = NULL;
    gboolean val;

    val = codec->_init(codec, ap, config, mode, &tmp_error);
    if (tmp_error != NULL)
    {
	g_propagate_error(error, tmp_error);
	return FALSE;
    }
    return val;
}



inline gboolean codec_getinfo (Codec *codec, Plugin_Codec *info)
{
    return codec->_getinfo(codec, info);
}



inline gint codec_encode (Codec *codec, gpointer frame, gpointer data)
{
    return codec->_encode(codec, frame, data);
}



inline gint codec_decode (Codec *codec, gpointer data, gpointer frame)
{
    return codec->_decode(codec, data, frame);
}



inline gint codec_about (Codec *codec)
{
    return codec->_about(codec);
}



inline gint codec_configure (Codec *codec)
{
    return codec->_configure(codec);
}



inline gboolean codec_destroy (Codec *codec)
{
    gboolean b;


    b = codec->_destroy(codec);
    return b;
}



inline gboolean codec_is_usable (Plugin_Codec *codec, gdouble bandwidth)
{
    gdouble codec_band;
    gdouble npacket;
    gdouble packet_size;

    /* calculate the total bandwdith needed by codec (including headers  */
    /* for rtp, udp, ip)                                                 */
    /* number of packet per second                                       */

    npacket = 2.0 * (gdouble)(codec->rate) / (gdouble)(codec->compressed_fr_size);
    packet_size = (gdouble)(codec->compressed_fr_size) + UDP_HEADER_SIZE + RTP_HEADER_SIZE + NET_IP_HEADER_SIZE;
    codec_band = packet_size * 8.0 * npacket;

    return (codec_band < bandwidth);
}


