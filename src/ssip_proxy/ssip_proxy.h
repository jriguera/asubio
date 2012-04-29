/*
 *
 * FILE: ssip_proxy.h
 *
 * Copyright: (c) 2003, Jose Riguera <jriguera@gmail.com>
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
 */

#ifndef _SSIP_PROXY_H_
#define _SSIP_PROXY_H_


#include <glib.h>
#include <syslog.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "../net/net.h"
#include "../net/tcp.h"
#include "../common.h"
#include "../configfile.h"
#include "../ssip.h"



#define SSIP_PROXY_PROGRAM_NAME     "ssip_proxy"
#define SSIP_PROXY_PROGRAM_VERSION  "0.10"
#define SSIP_PROXY_PROGRAM_DATE     "Mayo 2003"
#define SSIP_PROXY_PROGRAM_AUTHOR   "Jose Riguera"
#define SSIP_PROXY_PROGRAM_AMAIL    "<jriguera@gmail.com>"




#define PROGRAM_NUM_LISTEN  5

#define SYSLOG_MESG

#define PROGRAM_DAEMONMODE

#define PROCESS_MANAGE "MANAGE_PROCESS"

#define PROCESS_NOTIFY "NOTIFY_PROCESS"

#define CFG_GLOBAL_KEY "SSIP_PROXY"


#endif


