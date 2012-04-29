
/*
 *
 * FILE: echo.h
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

#ifndef _ECHO_H_
#define _ECHO_H_


#include <glib.h>
#include <gtk/gtk.h>
#include "../../plugin_effect.h"




#define ECHO_DEFAULT_DELAY    500
#define ECHO_DEFAULT_FEEDBACK 50
#define ECHO_DEFAULT_VOLUME   50
#define MAX_DELAY             1000



#define ECHO_CFG_SECTION  "echo_plugin"
#define ECHO_CFG_DELAY    "delay"
#define ECHO_CFG_FEEDBACK "feedback"
#define ECHO_CFG_VOLUME   "volume"
#define ECHO_CFG_WHEN     "when"



#define ECHO_NAME          "Audio Echo Effect Plugin"
#define ECHO_FILENAME      "libecho.so"
#define ECHO_AUTOR_NAME    "Jose Riguera"
#define ECHO_AUTOR_MAIL    "<jriguera@gmail.com>"
#define ECHO_AUTOR_DATE    "Mayo 2003"
#define ECHO_SHORT_LICENSE "GPL"
#define ECHO_LONG_LICENSE  "This program is free software; you can redistribute it and/or modify \
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




#define MAX_SRATE      50000
#define MAX_CHANNELS   2
#define BYTES_PS       2
#define BUFFER_SAMPLES (MAX_SRATE * MAX_DELAY / 1000)
#define BUFFER_SHORTS  (BUFFER_SAMPLES * MAX_CHANNELS)
#define BUFFER_BYTES   (BUFFER_SHORTS * BYTES_PS)




struct _Echo_Conf
{
    AudioProperties *ap;
    ConfigFile *configuration;
    gboolean echo_surround_enable;
    gint echo_delay;
    gint echo_volume;
    gint echo_feedback;
    TPCall when;
    gint16 *buffer_in;
    gint w_ofs_in;
    gint16 *buffer_out;
    gint w_ofs_out;
};

typedef struct _Echo_Conf Echo_Conf;


#endif


