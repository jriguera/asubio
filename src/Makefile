
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

# -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include 
# -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include
# -pthread -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include
# -I/usr/include/gtk-2.0 -I/usr/lib/gtk-2.0/include -I/usr/include/atk-1.0 -I/usr/include/pango-1.0 -I/usr/include/freetype2 -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include

# xml
# -I/usr/include/libxml2

# -lglib-2.0
# -Wl,--export-dynamic -lgmodule-2.0 -ldl -lglib-2.0
# -pthread -lgthread-2.0 -lglib-2.0
# -Wl,--export-dynamic -lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lgdk_pixbuf-2.0 -lm -lpangoxft-1.0 -lpangox-1.0 -lpango-1.0 -lgobject-2.0 -lgmodule-2.0 -ldl -lglib-2.0

# xml
# -L/usr/lib -lxml2 -lz -lpthread -lm

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


PROGRAM = asubio addressbook


GUI_DIR = gui_gtk
RTP_DIR = rtp
CRYPT_DIR = crypt
NET_DIR = net
CODEC_DIR = codec_plugin
INOUT_DIR = inout_plugin
EFFECT_DIR = effect_plugin
SSIP_PROXY_DIR = ssip_proxy


all:
	make -C $(NET_DIR)
	make -C $(CRYPT_DIR)
	make -C $(RTP_DIR)
	make -C $(CODEC_DIR)
	make -C $(EFFECT_DIR)
	make -C $(INOUT_DIR)
	make -C $(GUI_DIR)
	make proxy
	make $(PROGRAM)


audio_util.o: audio_util.c audio_util.h audio.h


configfile.o: configfile.c configfile.h audio.h


audio_convert.o: audio_convert.c audio_convert.h


audio_buffer.o: audio_buffer.c audio_buffer.h


audio_buffer_in.o: audio_buffer_in.c audio_buffer.h


audio_buffer_out.o: audio_buffer_out.c audio_buffer.h


audio_processor.o: audio_processor.c audio_processor.h


io.o: io.c io.h


common.o: common.c common.h


notify.o: notify.c notify.h


ssip_neg.o: ssip_neg.c ssip_neg.h


addbook_xml.o: addbook_xml.c addbook_xml.h


#addbook_xml.o: addbook_xml.c addbook_xml.h
#	$(CC) -g -O2 -W -Wall -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -I/usr/include/libxml2/libxml -I/usr/include/libxml2 -c $<


.PHONY: proxy clean distclean asubio



asubio:   $(GUI_DIR)/gui_main.o $(GUI_DIR)/gui_addressbook.o $(GUI_DIR)/gui_configuration.o $(GUI_DIR)/gui_incoming_call.o $(GUI_DIR)/gui_session.o $(GUI_DIR)/gui_common.o $(GUI_DIR)/gui_connect_call.o $(GUI_DIR)/gui_main_functions.o $(GUI_DIR)/gui_ssip_neg.o \
	  $(NET_DIR)/net.o $(NET_DIR)/tcp.o $(NET_DIR)/udp.o \
	  $(CRYPT_DIR)/des.o $(CRYPT_DIR)/md5.o \
	  $(RTP_DIR)/rtp_des.o $(RTP_DIR)/rtp.o $(RTP_DIR)/rtp_private.o \
          $(CODEC_DIR)/codec.o \
          addbook_xml.o ssip_neg.o notify.o io.o common.o audio_processor.o audio_buffer_out.o audio_buffer_in.o audio_buffer.o audio_util.o configfile.o audio_convert.o

	$(CC) $(LDFLAGS) $^ -o $@


addressbook: addressbook.c $(GUI_DIR)/gui_addressbook.o $(GUI_DIR)/gui_common.o addbook_xml.o



proxy: 
	make -C $(SSIP_PROXY_DIR)


clean:
	make -C $(NET_DIR) clean
	make -C $(CRYPT_DIR) clean
	make -C $(RTP_DIR) clean
	make -C $(CODEC_DIR) clean
	make -C $(EFFECT_DIR) clean
	make -C $(INOUT_DIR) clean
	make -C $(GUI_DIR) clean
	make -C $(SSIP_PROXY_DIR) clean
	rm -f *~


distclean:
	make -C $(NET_DIR) distclean
	make -C $(CRYPT_DIR) distclean
	make -C $(RTP_DIR) distclean
	make -C $(CODEC_DIR) distclean
	make -C $(EFFECT_DIR) distclean
	make -C $(INOUT_DIR) distclean
	make -C $(GUI_DIR) distclean
	make -C $(SSIP_PROXY_DIR) distclean
	make clean
	rm -f $(PROGRAM)
	rm -f *.o

