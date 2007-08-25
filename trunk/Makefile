CC = g++
AR = ar
RANLIB = ranlib
CCFLAGS = -Wall -Werror -D_FILE_OFFSET_BITS=64 -D__FreeBSD__=10 \
          -DFUSE_USE_VERSION=26 -DDEBUG
LDFLAGS = -lfuse -framework CoreFoundation -F/System/Library/PrivateFrameworks \
          -framework MobileDevice -Lythread -lythread -L. -liphonedisk

all: iphonediskd iphonedisk_mount

.cpp.o:
	$(CC) $(CCFLAGS) -c $<

libythread.a:
	cd ythread && make

libiphonedisk.a: iphonedisk.o connection.o
	$(AR) -r $@ iphonedisk.o connection.o
	$(RANLIB) $@

iphonedisk_mount: iphonedisk_mount.o libiphonedisk.a libythread.a
	$(CC) -o iphonedisk_mount iphonedisk_mount.o $(LDFLAGS)

iphonediskd: iphonediskd.o libiphonedisk.a libythread.a
	$(CC) -o iphonediskd iphonediskd.o $(LDFLAGS)

clean:
	cd ythread && make clean
	-rm -f *.o *.a *.core core iphonediskd iphonedisk_mount
