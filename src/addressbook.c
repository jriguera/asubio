
/*
 *
 * FILE: addressbook.c
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

#include <gtk/gtk.h>
#include <glib.h>
#include <string.h>
#include <stdlib.h>

#include "gui_gtk/gui_addressbook.h"



#define AB_PROGRAM_NAME    "addressbook"
#define AB_PROGRAM_VERSION "0.10"
#define AB_PROGRAM_DATE    "Mayo 2003"
#define AB_PROGRAM_AUTHOR  "Jose Riguera"
#define AB_PROGRAM_AMAIL   "<jriguera@gmail.com>"




int main (int argc, char **argv)
{
    if (argc != 2)
    {
	g_print("\n\t%s v %s (c) %s, %s %s\n", AB_PROGRAM_NAME, AB_PROGRAM_VERSION, AB_PROGRAM_DATE, AB_PROGRAM_AUTHOR, AB_PROGRAM_AMAIL);
	g_print("\nNumero de parametros incorrecto.\n");
	g_print("Modo de empleo:\n\n");
	g_print("\t%s <fichero.xml>\n", AB_PROGRAM_NAME);
	g_print("\n\tdonde <fichero.xml> es el fichero agenda que se desea abrir.\n");
	g_print("\tSi no existe sera creado\n\n");
        exit(-1);

    }
    gtk_set_locale();
    if (gtk_init_check (&argc, &argv) == FALSE)
    {
	g_printerr("Imposible acceder al sistema grafico ... Finalizando\n");
        exit(-1);
    }

    create_addressbook_window(argv[1], NULL);

    gtk_main();

    return 1;
}


