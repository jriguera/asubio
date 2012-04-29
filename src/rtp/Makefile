
#
#
# FILE: Makefile
#
# Copyright: (c) Junio 2003, Jose Riguera <jriguera@gmail.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
#
# ***************************************************************************

CC      = gcc
#CFLAGS  = -g -ggdb -W -Wall
CFLAGS  = -W -Wall -O2 -g -ggdb

CFLAGS  += `pkg-config --cflags glib-2.0`

LDFLAGS = `pkg-config --libs glib-2.0`


all: rtp.o rtp_des.o rtp_private.o


rtp.o: rtp.c rtp.h  rtp_private.h rtp_des.o rtp_private.o


rtp_des.o: rtp_des.c rtp_des.h


rtp_private.o: rtp_private.c rtp_private.h


clean:
	rm -f *~


distclean:
	make clean
	rm -f *.so *.o


