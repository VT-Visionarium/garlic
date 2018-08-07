# This is a GNU make file.

VERSION = 0.2
PREFIX ?= /usr/local/encap/PhotoScan-$(VERSION)

# These files are generated
built = PhotoScan_MovePointer

# files installed in PREFIX/bin
bin = $(built) photoscan

PhotoScan_MovePointer_LFLAGS = -lX11


build: $(built)

%: %.c
	$(CC) -Wall -Werror -g $($@_CFLAGS) $^ -o $@ $($@_LFLAGS)

install: $(built)
	mkdir -p $(PREFIX)/bin
	cp $(bin) $(PREFIX)/bin/

clean:
	rm -f $(built)

distclean: clean
