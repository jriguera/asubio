
/*
 *
 * FILE: des.h
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
 * (c) February 1993 Saleem N. Bhatti
 *
 ***************************************************************************/

#ifndef _DES_H_
#define _DES_H_

#include <unistd.h>
#include <string.h>
#include <time.h>

#include <glib.h>


/* what */
typedef enum {DES_encrypt, DES_decrypt} DES_What;

/* mode */
typedef enum {DES_ecb, DES_cbc, DES_cfb, DES_ofb} DES_Mode;

/* parity */
typedef enum {DES_even, DES_odd} DES_Parity;

/* key/IV generation */
typedef enum {DES_key, DES_iv} DES_Generate;


/* This does it all */
gint DES (guchar *key, guchar *data, guint size, const DES_What what, const DES_Mode mode, guchar *initVec);

/* Handy macros */
#define DES_ECB_e(_key, _data, _size) DES(_key, _data, _size, DES_encrypt, DES_ecb, (guchar *) 0)
#define DES_ECB_d(_key, _data, _size) DES(_key, _data, _size, DES_decrypt, DES_ecb, (guchar *) 0)

#define DES_CBC_e(_key, _data, _size, _initVec) DES(_key, _data, _size, DES_encrypt, DES_cbc, _initVec)
#define DES_CBC_d(_key, _data, _size, _initVec) DES(_key, _data, _size, DES_decrypt, DES_cbc, _initVec)

#define DES_CFB_e(_key, _data, _size, _initVec) DES(_key, _data, _size, DES_encrypt, DES_cfb, _initVec)
#define DES_CFB_d(_key, _data, _size, _initVec) DES(_key, _data, _size, DES_decrypt, DES_cfb, _initVec)

#define DES_OFB_e(_key, _data, _size, _initVec) DES(_key, _data, _size, DES_encrypt, DES_ofb, _initVec)
#define DES_OFB_d(_key, _data, _size, _initVec) DES(_key, _data, _size, DES_decrypt, DES_ofb, _initVec)

/* Padded [m|re]alloc() */
guchar DES_setPad (guchar pad);

#define DES_padSpace() DES_setPad((guchar) ' ')
#define DES_padZero() DES_setPad((guchar) '\0')

/* The size of text in a DES_malloc()ed block */
#define DES_plainTextSize(_ptr, _size) (guint) ((_size) - (guint) (_ptr)[(_size) - 1])

/* Keys */
inline void DES_setParity (guchar *ptr, guint size, const DES_Parity parity);
inline guint DES_checkParity (guchar *ptr, guint size, const DES_Parity parity);

guchar *DES_generate (const DES_Generate what); /* returns a pointer to static memory */

#define DES_generateKey() DES_generate(DES_key)
#define DES_generateIV() DES_generate(DES_iv)

inline gint DES_checkWeakKeys (guchar *key);


#endif

