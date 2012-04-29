
/*
 *
 * FILE: common.c
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

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "common.h"



void bury_child (gint signal)
{
    UNUSED(signal);
    waitpid(-1, NULL, WNOHANG);
}



inline gint square_2 (gint p)
{
    gint c = 0;

    if (((p % 2) != 0) || (p < 2)) return -1;

    do
    {
	p /= 2;
	c++;
    }
    while (p > 1);

    return c;
}



void execute_command (gchar *cmd)
{
    gchar *argv[4] = {PROGRAM_SHELL, "-c", NULL, NULL};
    gint pid;
    argv[2] = cmd;


    signal(SIGCHLD, bury_child);

    if ((pid = fork()) == 0)
    {
	execv(PROGRAM_SHELL, argv);
    }
    else
    {
	if (pid <= 0) g_printerr("Execute_command: Unable fork\n");
    }
}


