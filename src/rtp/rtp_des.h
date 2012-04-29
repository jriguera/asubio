
/*
 *
 * FILE: rtp_des.h
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

#ifndef _RTP_DES_H_
#define _RTP_DES_H_


#include <glib.h>
#include "rtp_private.h"
#include "../crypt/des.h"



gboolean des_initialize (RtpSession *session, guchar *hash, GError **error);
gint des_exit (RtpSession *session);
gint des_encrypt (RtpSession *session, guchar *data, guint size, guchar *initVec);
gint des_decrypt (RtpSession *session, guchar *data, guint size, guchar *initVec);


#endif

