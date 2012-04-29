
/*
 *
 * FILE: audio_convert.c
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
 * This file is based on OSS-xmms plugin
 * Copyright (C) 2001  Haavard Kvaalen
 *
 ***************************************************************************/

#include "audio_convert.h"




static void *get_convert_buffer (size_t size)
{
    static size_t length;
    static void *buffer;


    if (size > 0 && size <= length) return buffer;
    length = size;
    buffer = g_realloc(buffer, size);
    return buffer;
}




gint convert_swap_endian (void **data, gint length)
{
    guint16 *ptr = *data;
    gint i;

    for (i = 0; i < length; i += 2, ptr++)
	*ptr = GUINT16_SWAP_LE_BE(*ptr);

    return i;
}



gint convert_swap_sign_and_endian_to_native (void **data, gint length)
{
    guint16 *ptr = *data;
    gint i;

    for (i = 0; i < length; i += 2, ptr++)
	*ptr = GUINT16_SWAP_LE_BE(*ptr) ^ 1 << 15;

    return i;
}



gint convert_swap_sign_and_endian_to_alien (void **data, gint length)
{
    guint16 *ptr = *data;
    gint i;

    for (i = 0; i < length; i += 2, ptr++)
	*ptr = GUINT16_SWAP_LE_BE(*ptr ^ 1 << 15);

    return i;
}



gint convert_swap_sign16 (void **data, gint length)
{
    gint16 *ptr = *data;
    gint i;

    for (i = 0; i < length; i += 2, ptr++)
	*ptr ^= 1 << 15;

    return i;
}



gint convert_swap_sign8 (void **data, gint length)
{
    gint8 *ptr = *data;
    gint i;

    for (i = 0; i < length; i++)
	*ptr++ ^= 1 << 7;

    return i;
}



gint convert_to_8_native_endian (void **data, gint length)
{
    gint8 *output = *data;
    gint16 *input = *data;
    gint i;

    for (i = 0; i < length / 2; i++)
	*output++ = *input++ >> 8;

    return i;
}



gint convert_to_8_native_endian_swap_sign (void **data, gint length)
{
    gint8 *output = *data;
    gint16 *input = *data;
    gint i;

    for (i = 0; i < length / 2; i++)
	*output++ = (*input++ >> 8) ^ (1 << 7);

    return i;
}



gint convert_to_8_alien_endian (void **data, gint length)
{
    gint8 *output = *data;
    gint16 *input = *data;
    gint i;

    for (i = 0; i < length / 2; i++)
	*output++ = *input++ & 0xff;

    return i;
}



gint convert_to_8_alien_endian_swap_sign (void **data, gint length)
{
    gint8 *output = *data;
    gint16 *input = *data;
    gint i;

    for (i = 0; i < length / 2; i++)
	*output++ = (*input++ & 0xff) ^ (1 << 7);

    return i;
}



gint convert_to_16_native_endian (void **data, gint length)
{
    guint8 *input = *data;
    guint16 *output;
    gint i;

    *data = get_convert_buffer(length * 2);
    output = *data;
    for (i = 0; i < length; i++)
	*output++ = *input++ << 8;

    return i * 2;
}



gint convert_to_16_native_endian_swap_sign (void **data, gint length)
{
    guint8 *input = *data;
    guint16 *output;
    gint i;

    *data = get_convert_buffer(length * 2);
    output = *data;
    for (i = 0; i < length; i++)
	*output++ = (*input++ << 8) ^ (1 << 15);

    return i * 2;
}



gint convert_to_16_alien_endian (void **data, gint length)
{
    guint8 *input = *data;
    guint16 *output;
    gint i;

    *data = get_convert_buffer(length * 2);
    output = *data;
    for (i = 0; i < length; i++)
	*output++ = *input++;

    return i * 2;
}



gint convert_to_16_alien_endian_swap_sign (void **data, gint length)
{
    guint8 *input = *data;
    guint16 *output;
    gint i;

    *data = get_convert_buffer(length * 2);
    output = *data;
    for (i = 0; i < length; i++)
	*output++ = *input++ ^ (1 << 7);

    return i * 2;
}



gint (*get_convert_function (AudioProperties *output, AudioProperties *input))(void **, gint)
{
    if (output->format == input->format) return NULL;

    if ((output->format == FMT_U16_BE && input->format == FMT_U16_LE) ||
	(output->format == FMT_U16_LE && input->format == FMT_U16_BE) ||
	(output->format == FMT_S16_BE && input->format == FMT_S16_LE) ||
	(output->format == FMT_S16_LE && input->format == FMT_S16_BE))
	return convert_swap_endian;

    if ((output->format == FMT_U16_BE && input->format == FMT_S16_BE) ||
	(output->format == FMT_U16_LE && input->format == FMT_S16_LE) ||
	(output->format == FMT_S16_BE && input->format == FMT_U16_BE) ||
	(output->format == FMT_S16_LE && input->format == FMT_U16_LE))
	return convert_swap_sign16;

    if ((IS_BIG_ENDIAN &&
	 ((output->format == FMT_U16_BE && input->format == FMT_S16_LE) ||
	  (output->format == FMT_S16_BE && input->format == FMT_U16_LE))) ||
	(!IS_BIG_ENDIAN &&
	 ((output->format == FMT_U16_LE && input->format == FMT_S16_BE) ||
	  (output->format == FMT_S16_LE && input->format == FMT_U16_BE))))
	return convert_swap_sign_and_endian_to_native;

    if ((!IS_BIG_ENDIAN &&
	 ((output->format == FMT_U16_BE && input->format == FMT_S16_LE) ||
	  (output->format == FMT_S16_BE && input->format == FMT_U16_LE))) ||
	(IS_BIG_ENDIAN &&
	 ((output->format == FMT_U16_LE && input->format == FMT_S16_BE) ||
	  (output->format == FMT_S16_LE && input->format == FMT_U16_BE))))
	return convert_swap_sign_and_endian_to_alien;

    if ((IS_BIG_ENDIAN &&
	 ((output->format == FMT_U8 && input->format == FMT_U16_BE) ||
	  (output->format == FMT_S8 && input->format == FMT_S16_BE))) ||
	(!IS_BIG_ENDIAN &&
	 ((output->format == FMT_U8 && input->format == FMT_U16_LE) ||
	  (output->format == FMT_S8 && input->format == FMT_S16_LE))))
	return convert_to_8_native_endian;

    if ((IS_BIG_ENDIAN &&
	 ((output->format == FMT_U8 && input->format == FMT_S16_BE) ||
	  (output->format == FMT_S8 && input->format == FMT_U16_BE))) ||
	(!IS_BIG_ENDIAN &&
	 ((output->format == FMT_U8 && input->format == FMT_S16_LE) ||
	  (output->format == FMT_S8 && input->format == FMT_U16_LE))))
	return convert_to_8_native_endian_swap_sign;

    if ((!IS_BIG_ENDIAN &&
	 ((output->format == FMT_U8 && input->format == FMT_U16_BE) ||
	  (output->format == FMT_S8 && input->format == FMT_S16_BE))) ||
	(IS_BIG_ENDIAN &&
	 ((output->format == FMT_U8 && input->format == FMT_U16_LE) ||
	  (output->format == FMT_S8 && input->format == FMT_S16_LE))))
	return convert_to_8_alien_endian;

    if ((!IS_BIG_ENDIAN &&
	 ((output->format == FMT_U8 && input->format == FMT_S16_BE) ||
	  (output->format == FMT_S8 && input->format == FMT_U16_BE))) ||
	(IS_BIG_ENDIAN &&
	 ((output->format == FMT_U8 && input->format == FMT_S16_LE) ||
	  (output->format == FMT_S8 && input->format == FMT_U16_LE))))
	return convert_to_8_alien_endian_swap_sign;

    if ((output->format == FMT_U8 && input->format == FMT_S8) ||
	(output->format == FMT_S8 && input->format == FMT_U8))
	return convert_swap_sign8;

    if ((IS_BIG_ENDIAN &&
	 ((output->format == FMT_U16_BE && input->format == FMT_U8) ||
	  (output->format == FMT_S16_BE && input->format == FMT_S8))) ||
	(!IS_BIG_ENDIAN &&
	 ((output->format == FMT_U16_LE && input->format == FMT_U8) ||
	  (output->format == FMT_S16_LE && input->format == FMT_S8))))
	return convert_to_16_native_endian;

    if ((IS_BIG_ENDIAN &&
	 ((output->format == FMT_U16_BE && input->format == FMT_S8) ||
	  (output->format == FMT_S16_BE && input->format == FMT_U8))) ||
	(!IS_BIG_ENDIAN &&
	 ((output->format == FMT_U16_LE && input->format == FMT_S8) ||
	  (output->format == FMT_S16_LE && input->format == FMT_U8))))
	return convert_to_16_native_endian_swap_sign;

    if ((!IS_BIG_ENDIAN &&
	 ((output->format == FMT_U16_BE && input->format == FMT_U8) ||
	  (output->format == FMT_S16_BE && input->format == FMT_S8))) ||
	(IS_BIG_ENDIAN &&
	 ((output->format == FMT_U16_LE && input->format == FMT_U8) ||
	  (output->format == FMT_S16_LE && input->format == FMT_S8))))
	return convert_to_16_alien_endian;

    if ((!IS_BIG_ENDIAN &&
	 ((output->format == FMT_U16_BE && input->format == FMT_S8) ||
	  (output->format == FMT_S16_BE && input->format == FMT_U8))) ||
	(IS_BIG_ENDIAN &&
	 ((output->format == FMT_U16_LE && input->format == FMT_S8) ||
	  (output->format == FMT_S16_LE && input->format == FMT_U8))))
	return convert_to_16_alien_endian_swap_sign;

    return NULL;
}


