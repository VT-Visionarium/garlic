# This is a GNU makefile
#
# Butt-ugly make files are so fun!
# But only if you're a make expert.

PREFIX := /usr/local/encap/hypercube_vt-0.5

BIN = $(PREFIX)/bin
ETC = $(PREFIX)/etc

CFLAGS = -g -Werror

HOSTNAME = $(shell hostname -s)

hy_getRootWidth_LFLAGS = -lX11
hy_getRootHeight_LFLAGS = -lX11
hy_checkXRunning.o_CFLAGS = $(shell pkg-config gtk+-3.0 --cflags)
hy_checkXRunning_CFLAGS = $(shell pkg-config gtk+-3.0 --cflags)
hy_checkXRunning_LFLAGS = $(shell pkg-config gtk+-3.0 --libs)

cbinaries = $(patsubst %.c,%,$(wildcard *.c))
built_files = $(patsubst %.in,%,$(wildcard hy_*.in)) $(cbinaries)

ifeq ($(HOSTNAME),cube)
inst_files = $(sort\
 $(filter-out %.in %.c %.o %_OLD %~,$(wildcard hy_*) $(built_files))\
 nv-control-blend-visbox encap.exclude)
else
inst_files =\
 hy_demoRunner\
 hy_console_logout\
 hy_projectors_on\
 hy_projectors_off\
 hy_projectors_common.rb
endif


build: $(built_files)

check:
	@echo "inst_files=$(inst_files)"
	@echo
	@echo "cbinaries=$(cbinaries)"
	@echo
	@echo "built_files=$(built_files)"


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
	cp xorg.conf.cube_1920x1200 xorg.conf.cube_2560x1600 /etc/X11/
	cp xorg.conf.cube_2560x1600 /etc/X11/xorg.conf
	if [ ! -f xinitrc.OLD ] ; then mv /etc/X11/xinit/xinitrc xinitrc.OLD ; echo ; fi
	cp xinitrc /etc/X11/xinit/xinitrc
	#cp _hy_checkX11Session.desktop /etc/xdg/autostart/hy_checkX11Session.desktop

install: build
	mkdir -p $(BIN) $(ETC)
	cp $(inst_files) $(BIN)
	cp cave_128x128.png $(ETC)
ifeq ($(HOSTNAME),cube)
	sudo $(MAKE) sudo_install
endif


clean:
	rm -f $(built_files) *.o

