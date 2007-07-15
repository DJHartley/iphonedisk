CC = g++
CCFLAGS = -Wall -I../iPhoneInterface -D_FILE_OFFSET_BITS=64 -D__FreeBSD__=10 -DFUSE_USE_VERSION=26
LDFLAGS = -lfuse -framework CoreFoundation -F/System/Library/PrivateFrameworks -framework MobileDevice -lythread

all: iphonedisk

.cpp.o:
	$(CC) $(CCFLAGS) -c $<

iphonedisk: iphonedisk.o connection.o
	$(CC) $(LDFLAGS) -o iphonedisk iphonedisk.o connection.o

clean:
	-rm -f *.o *.core core iphonedisk
