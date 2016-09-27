# This is a GNU makefile
#
# Butt-ugly make files are so fun!
# But only if you're a make expert.

PREFIX := /usr/local/encap/hypercube_vt-0.2

BIN = $(PREFIX)/bin
ETC = $(PREFIX)/etc

CFLAGS = -g

HOSTNAME = $(shell hostname -s)

hy_getRootWidth_LFLAGS = -lX11
hy_getRootHeight_LFLAGS = -lX11
hy_blackEdge.o_CFLAGS = $(shell pkg-config gtk+-3.0 --cflags)
hy_blackEdge_CFLAGS = $(shell pkg-config gtk+-3.0 --cflags)
hy_blackEdge_LFLAGS = $(shell pkg-config gtk+-3.0 --libs)

cbinaries = $(patsubst %.c,%,$(wildcard *.c))
built_files = $(patsubst %.in,%,$(wildcard hy_*.in)) $(cbinaries)

ifeq ($(HOSTNAME),cube)
inst_files = $(sort\
 $(filter-out %.in,$(wildcard hy_*) $(built_files))\
 nv-control-blend-visbox encap.exclude)
else
inst_files = hy_demoRunner hy_console_logout
endif

build: $(built_files)


$(cbinaries): %: %.o
	$(CC) $(CFLAGS) $($@_CFLAGS) $^ -o $@ $($@_LFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $($@_CFLAGS) $< -c -o $@

hy_projectors_on: hy_projectors_on.in
	echo "#!/usr/bin/ruby -w" > $@
	echo "# This is a generated file" >> $@
	sed $^ -e 's/@on@/on/g' -e 's/@CMD@/POWR1/g' >> $@
	chmod 755 $@

hy_projectors_off: hy_projectors_off.in
	echo "#!/usr/bin/ruby -w" > $@
	echo "# This is a generated file" >> $@
	sed $^ -e 's/@on@/off/g' -e 's/@CMD@/POWR0/g' >> $@
	chmod 755 $@

hy_edgeBlend_on hy_edgeBlend_off: %:%.in
	echo "#!/bin/bash" > $@
	echo "# This is a generated file" >> $@
	sed $^ -e 's!@BIN@!$(BIN)!g' >> $@
	chmod 755 $@

sudo_install:
	chown root:root $(BIN)/hy_stopx $(BIN)/hy_kill_dtk-server
	chmod u+s $(BIN)/hy_stopx $(BIN)/hy_kill_dtk-server
	cp xorg.conf.cube_1920x1200 xorg.conf.cube_2560x1600 /etc/X11/
	if [ ! -f dot_xinit.OLD ] ; then mv /etc/X11/xinit/xinitrc dot_xinit.OLD ; echo ; fi
	cp dot_xinitrc /etc/X11/xinit/xinitrc

install: build
ifeq ($(HOSTNAME),cube)
	sudo rm -f $(BIN)/hy_stopx $(BIN)/hy_kill_dtk-server
endif
	mkdir -p $(BIN) $(ETC)
	cp $(inst_files) $(BIN)
	cp cave_128x128.png $(ETC)
ifeq ($(HOSTNAME),cube)
	sudo $(MAKE) sudo_install
endif


clean:
	rm -f $(built_files) *.o

