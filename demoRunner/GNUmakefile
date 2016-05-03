# This is a GNU makefile


# You can override PREFIX and WWW_ROOT by running like so:
# 
#    make PREFIX=/my/prefix WWW_ROOT=/my/www_root install
#
# Note: you must set these variables to the same values every
# time you run make or you will get bad results.
# Run 'make clean' will reset them.
#

####################################################
#  CONFIGURATION
####################################################
# This following can be overridden of changed here
PREFIX := /usr/local/encap/demoRunner-0.0
WWW_ROOT := /usr/local/src/demoRunner
USER_CONFIG_DIR := .hy_demoRunner
####################################################


# Stuff we install in BIN
bins = demoRunner-server php.ini

BIN = $(PREFIX)/bin

build: $(bins)

install: build
	mkdir -p $(BIN)
	cp $(bins) $(BIN)
	echo "php.ini" > $(BIN)/encap.exclude

# We make an intermediate sed file which we add a head to.
%.sed: %.in GNUmakefile
	sed $< \
 -e 's!@PREFIX@!$(PREFIX)!g'\
 -e 's!@WWW_ROOT@!$(WWW_ROOT)!g'\
 -e 's!@USER_CONFIG_DIR@!$(USER_CONFIG_DIR)!g'\
 > $@


# add the header
php.ini: %: %.sed
	echo "; This is a generated file" > $@
	cat $< >> $@

# add the header
demoRunner-server: %: %.sed
	echo "#!/bin/bash" > $@
	echo "# This is a generated file" >> $@
	cat $< >> $@
	chmod 755 $@


clean:
	rm -f demoRunner-server php.ini *.sed

