
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
#CFLAGS  = -g -ggdb -W -Wall `xml2-config --cflags`
CFLAGS  = -W -Wall -g -ggdb `xml2-config --cflags`

CFLAGS  += `pkg-config --cflags glib-2.0`
CFLAGS  += `pkg-config --cflags gmodule-2.0`
CFLAGS  += `pkg-config --cflags gthread-2.0`
CFLAGS  += `pkg-config --cflags gtk+-2.0`

#LDFLAGS = -lefence `pkg-config --libs glib-2.0`
LDFLAGS =  `pkg-config --libs glib-2.0` 
LDFLAGS += `xml2-config --libs`
LDFLAGS += `pkg-config --libs gmodule-2.0`
LDFLAGS += `pkg-config --libs gthread-2.0`
LDFLAGS += `pkg-config --libs gtk+-2.0`


MAIN_DIR = ..
RTP_DIR = $(MAIN_DIR)/rtp
CRYPT_DIR = $(MAIN_DIR)/crypt
NET_DIR = $(MAIN_DIR)/net
CODEC_DIR = $(MAIN_DIR)/codec_plugin
INOUT_DIR = $(MAIN_DIR)/inout_plugin
EFFECT_DIR = $(MAIN_DIR)/effect_plugin



%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@


OBJ = gui_ssip_neg.o \
      gui_session.o \
      gui_main_functions.o \
      gui_incoming_call.o \
      gui_connect_call.o \
      gui_configuration.o \
      gui_common.o \
      gui_addressbook.o \
      gui_main.o

all: $(OBJ)



clean:
	rm -f *~


distclean:
	make clean
	rm -f $(OBJ)


