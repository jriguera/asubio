
CC      = gcc
CFLAGS  = -g -ggdb -W -Wall -O2
#CFLAGS  = -W -Wall -O2
CFLAGS  += `pkg-config --cflags glib-2.0`
CFLAGS  += `pkg-config --cflags gmodule-2.0`
CFLAGS  += `pkg-config --cflags gtk+-2.0`

LDFLAGS = `pkg-config --libs glib-2.0`
LDFLAGS += `pkg-config --libs gmodule-2.0`
LDFLAGS += `pkg-config --libs gtk+-2.0`

all: libgsm.so


libgsm.so: lib_gsm.o
	$(CC) $(LDFLAGS) -fPIC -shared $^ -o $@


lib_gsm.o: lib_gsm.c lib_gsm.h


adpcm.o: adpcm.c adpcm.h



clean:
	rm -f *~


distclean:
	make clean
	rm -f *.so *.o

