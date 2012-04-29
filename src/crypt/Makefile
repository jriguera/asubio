
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
CFLAGS  = -g -ggdb -W -Wall
#CFLAGS  = -W -Wall -O2

CFLAGS  += `pkg-config --cflags glib-2.0`

LDFLAGS = `pkg-config --libs glib-2.0`


all: des.o md5.o



des.o: des.c des.h


md5.o: md5.c md5.h



clean:
	rm -f *~


distclean:
	make clean
	rm -f *.o


