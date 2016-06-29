# This is a GNU makefile


# You can override PREFIX and DOC_ROOT by running like so:
# 
#    make PREFIX=/my/prefix DOC_ROOT=/my/www_root install
#
# Note: you must set these variables to the same values every
# time you run make or you will get bad results.
# Run 'make clean' will reset them.
#

####################################################
#  CONFIGURATION
####################################################
# This following can be overridden of changed here
# via running: make install PREFIX=/my/prefix
PREFIX := /usr/local/encap/demoRunner-0.0
####################################################


# Stuff we install in BIN
private_files = \
 $(wildcard etc/*.htm etc/*.js etc/*.css)\

BIN = $(PREFIX)/bin
ETC = $(PREFIX)/etc

# We npm install a copy of ws for development
build:
	npm install ws

mkdirs:
	rm -rf $(ETC) $(BIN) && mkdir -p $(ETC) $(BIN)


# The current version of socket.io seems to be broken when used
# with firefox and chromium-browser.  So we are trying nodejs 'ws'
# WebSockets.
install: build mkdirs
	cp  -r $(private_files) $(ETC)/
	cp demoRunner-server $(BIN)/
	cd $(BIN)/ && npm install ws
	echo "etc" > $(PREFIX)/encap.exclude
	echo "node_modules" > $(BIN)/encap.exclude


clean:
	rm -rf node_modules

