CC = g++
AR = ar
RANLIB = ranlib
CCFLAGS = -Wall -D_FILE_OFFSET_BITS=64 -D__FreeBSD__=10 -DFUSE_USE_VERSION=26
LDFLAGS = -lfuse -framework CoreFoundation -F/System/Library/PrivateFrameworks -framework MobileDevice -Lythread -lythread

all: iphonedisk_mount

.cpp.o:
	$(CC) $(CCFLAGS) -c $<

libythread.a:
	cd ythread && make

libiphonedisk.a: iphonedisk.o connection.o
	$(AR) rc $@ iphonedisk.o connection.o
	$(RANLIB) $@

iphonedisk_mount: iphonedisk_mount.o libiphonedisk.a libythread.a
	rm -f iphonedisk_mount
	$(CC) -o iphonedisk_mount iphonedisk_mount.o $(LDFLAGS) -L. -liphonedisk

clean:
	cd ythread && make clean
	-rm -f *.o *.a *.core core iphonedisk
