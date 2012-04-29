
/*
 *
 * FILE: audio.h
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

#ifndef _AUDIO_H_
#define _AUDIO_H_


#include <glib.h>
#include "common.h"




enum _AFormat
{
    FMT_U8,
    FMT_S8,
    FMT_U16_LE,
    FMT_U16_BE,
    FMT_S16_LE,
    FMT_S16_BE,
};

typedef enum _AFormat AFormat;



struct _AudioProperties
{
    gint channels;
    gint rate;
    AFormat format;
};

typedef struct _AudioProperties AudioProperties;



#endif


