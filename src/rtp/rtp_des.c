
/*
 *
 * FILE: rtp_des.c
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

#include "rtp_des.h"
#include "../common.h"


gboolean des_initialize (RtpSession *session, guchar *hash, GError **error)
{
    gchar *key;
    gint i, j, k;


    UNUSED(error);
    session->encryption_pad_length = 8;
    session->encrypt_func = des_encrypt;
    session->decrypt_func = des_decrypt;
	
    if (session->crypto_state.des.encryption_key != NULL)
    {
	g_free(session->crypto_state.des.encryption_key);
    }
    key = session->crypto_state.des.encryption_key = (gchar *) g_malloc(8);

    /* Step 3: take first 56 bits of the MD5 hash */
    key[0] = hash[0];
    key[1] = hash[0] << 7 | hash[1] >> 1;
    key[2] = hash[1] << 6 | hash[2] >> 2;
    key[3] = hash[2] << 5 | hash[3] >> 3;
    key[4] = hash[3] << 4 | hash[4] >> 4;
    key[5] = hash[4] << 3 | hash[5] >> 5;
    key[6] = hash[5] << 2 | hash[6] >> 6;
    key[7] = hash[6] << 1;

    /* Step 4: add parity bits */
    for (i = 0; i < 8; ++i)
    {
	k = key[i] & 0xfe;
	j = k;
	j ^= j >> 4;
	j ^= j >> 2;
	j ^= j >> 1;
	j = (j & 1) ^ 1;
	key[i] = k | j;
    }
    return TRUE;
}



gint des_exit (RtpSession *session)
{
    g_free(session->crypto_state.des.encryption_key);
    return 1;
}



gint des_encrypt (RtpSession *session, guchar *data, guint size, guchar *initVec)
{
    DES_CBC_e(session->crypto_state.des.encryption_key, data, size, initVec);
    return 1;
}



gint des_decrypt (RtpSession *session, guchar *data, guint size, guchar *initVec)
{
    DES_CBC_d(session->crypto_state.des.encryption_key, data, size, initVec);
    return 1;
}


