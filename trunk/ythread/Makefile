# Author: Allen Porter <allen@thebends.org>

AR = ar
CC = g++ 
RANLIB = ranlib
INSTALL = install
CFLAGS = -O -Wall
INSTALLDIR = /usr/local
INSTALL_LIBDIR = $(INSTALLDIR)/lib
INSTALL_INCLUDEDIR = $(INSTALLDIR)/include/ythread

all: libythread.a

thread.o: thread.cpp
	$(CC) $(CFLAGS) -c thread.cpp

mutex.o: mutex.cpp
	$(CC) $(CFLAGS) -c mutex.cpp

condvar.o: condvar.cpp
	$(CC) $(CFLAGS) -c condvar.cpp

threadpool.o: threadpool.cpp
	$(CC) $(CFLAGS) -c threadpool.cpp

libythread.a: thread.o mutex.o condvar.o threadpool.o
	$(AR) rc $@ thread.o mutex.o condvar.o threadpool.o
	$(RANLIB) $@

test: test.cpp libythread.a
	$(CC) $(CFLAGS) -o test test.cpp libythread.a -lpthread
	./test

install: libythread.a
	$(INSTALL) -p -m 755 libythread.a $(INSTALL_LIBDIR)
	mkdir -p $(INSTALL_INCLUDEDIR)
	$(INSTALL) -p -m 755 callback*.h thread.h mutex.h condvar.h threadpool.h $(INSTALL_INCLUDEDIR)

clean:
	-rm -f core *.o test libythread.a
