CC = g++
CCFLAGS = -Wall -D_FILE_OFFSET_BITS=64 -D__FreeBSD__=10 -DFUSE_USE_VERSION=26
LDFLAGS = -lfuse -framework CoreFoundation -F/System/Library/PrivateFrameworks -framework MobileDevice -Lythread -lythread

all: libythread iphonedisk

.cpp.o:
	$(CC) $(CCFLAGS) -c $<

libythread:
	cd ythread && make

iphonedisk: iphonedisk.o connection.o
	$(CC) $(LDFLAGS) -o iphonedisk iphonedisk.o connection.o

clean:
	cd ythread && make clean
	-rm -f *.o *.core core iphonedisk
