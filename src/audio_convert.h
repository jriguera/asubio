
/*
 *
 * FILE: audio_convert.h
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

#ifndef _AUDIO_CONVERT_H_
#define _AUDIO_CONVERT_H_


#include <glib.h>
#include "audio.h"
#include "common.h"




#if G_BYTE_ORDER == G_BIG_ENDIAN
#define IS_BIG_ENDIAN TRUE
#elif G_BYTE_ORDER == G_LITTLE_ENDIAN
#define IS_BIG_ENDIAN FALSE
#else
#error "G_BYTE_ORDER should be big or little endian."
#endif




typedef gint (*Convert_Function) (void **data, gint length);



gint convert_swap_endian (void **data, gint length);
gint convert_swap_sign_and_endian_to_native (void **data, gint length);
gint convert_swap_sign_and_endian_to_alien (void **data, gint length);
gint convert_swap_sign16 (void **data, gint length);
gint convert_swap_sign8 (void **data, gint length);
gint convert_to_8_native_endian (void **data, gint length);
gint convert_to_8_native_endian_swap_sign (void **data, gint length);
gint convert_to_8_alien_endian (void **data, gint length);
gint convert_to_8_alien_endian_swap_sign (void **data, gint length);
gint convert_to_16_native_endian (void **data, gint length);
gint convert_to_16_native_endian_swap_sign (void **data, gint length);
gint convert_to_16_alien_endian (void **data, gint length);
gint convert_to_16_alien_endian_swap_sign (void **data, gint length);

gint (*get_convert_function(AudioProperties *output, AudioProperties *input))(void **, gint);


#endif


