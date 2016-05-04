# GNU makefile

PREFIX := /usr/local/encap/hypercube_vt-0.1

BIN = $(PREFIX)/bin

CFLAGS = -g

cbinaries = $(patsubst %.c,%,$(wildcard *.c))
built_files = $(patsubst %.in,%,$(wildcard hy_*.in)) $(cbinaries)

inst_files = $(sort\
 $(filter-out %.in,$(wildcard hy_*) $(built_files))\
 nv-control-blend-visbox encap.exclude)

build: $(built_files)

$(cbinaries): %: %.o
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@

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


install: build
	mkdir -p $(BIN)
	cp $(inst_files) $(BIN)
	@echo
	@echo "You may want to run: sudo encap"
	@echo
	@echo "You also need to run:"
	@echo
	@echo "  sudo chown root:root $(BIN)/hy_stopx"
	@echo "  sudo chmod u+s $(BIN)/hy_stopx"
	@echo
	@echo "So that any user may stop an X session for other users."
	@echo

clean:
	rm -f $(built_files) *.o

