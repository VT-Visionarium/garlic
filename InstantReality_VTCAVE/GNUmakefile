# This is a GNU makefile for building an installing this stuff on
# GNU/Linux.  This is not portable but it's way less code than when this
# is done in CMake and I consider it more readable.  This took me less
# than one hour, at start this file as a replacement to the equivalent
# CMake file.  The corresponding CMakeLists.txt file took me two days.

# This GNU makefile uses wildcard suffix rules to make and install what it
# finds in the current directory.

# Install prefix directory.  Change the version number if you like, via
# common line 'make PREFIX=/My/Prefix' or edit this file.  This version
# number is only defined here.
# Try running: 'make PREFIX=/my/prefix' to override this
# default PREFIX value.
PREFIX := /usr/local/encap/instantReality_VTCAVE-0.0

# common compiler options used by all compiler commands
#CXXFLAGS = -Wall -Werror
CXXFLAGS = -Wall -Werror -g


MODELS_DIR = $(PREFIX)/share/models
NAVS_DIR = $(PREFIX)/share/instantReality_navs
ENGINES_DIR = $(PREFIX)/share/instantReality_engines
PLUGINS_DIR = $(PREFIX)/lib/instantReality_plugins
BIN_DIR = $(PREFIX)/bin


DTK_ROOT = /usr/local
IR_ROOT = /opt/instantReality

# C pre-processor flags used in the compile command to make objects
CPPFLAGS = -fpic -I$(DTK_ROOT)/include -I$(IR_ROOT)/include

# All the linking stuff in making the shared object libraries.
# No need to separate linker arguments into parts like LIBADD and shit.
LDFLAGS = -shared -L$(DTK_ROOT)/lib -ldtk -L$(IR_ROOT)/lib -lInstantIO\
 -Wl,-rpath=/usr/local/lib

# butt ugly UNIX shell hack to find bash shell scripts to install
scripts = $(sort $(patsubst %.in, %, $(shell\
 file *|\
 grep 'Bourne-Again shell script'|\
 awk '{print $$1}'|\
 sed -e 's/:$$//g'|\
 grep -Ev '(_test|\.bash$$)')))

# installed models, may not be for just InstantReality
models = $(sort $(filter-out %_test.%, $(wildcard *.wrl *.x3d)))

# shared library InstantIO plug-ins
plugins = $(sort $(patsubst %_plugin.cpp, %.iio, $(wildcard *_plugin.cpp)))
inst_plugins = $(sort $(filter-out %_test.iio, $(plugins)))

test_bins = $(sort $(patsubst %.c, %, $(wildcard *_test.c)))

multiScreenGTK_test_LDFLAGS = $(shell pkg-config pkg-config --libs gtk+-2.0)
multiScreenGTK_test.o_CPPFLAGS = $(shell pkg-config pkg-config --cflags gtk+-2.0)


# InstantReality navigations # this is 6 lines in CMake
navs = $(sort $(filter-out %_test-nav.%, $(wildcard *-nav.wrl *-nav.x3d)))

# installed InstantReality engines
built_engines = $(sort\
 $(patsubst %_engineer.bash, %-engine.x3d, $(wildcard *_engineer.bash)))
engines = $(sort\
 $(wildcard *-engine.x3d *-engine.wrl)\
 $(build_engines))
inst_engines = $(sort $(filter-out %_test-engine.x3d, $(engines)))

installed = $(sort $(models) $(inst_plugins) $(inst_engines) $(navs) $(scripts))

bash_from_in_files = $(sort\
 $(patsubst %.in, %,\
 $(wildcard $(patsubst %, %.in, $(scripts)))))

built := $(sort $(plugins) $(built_engines) $(test_bins) $(bash_from_in_files))

# default target is build
build: $(built)

debug:
	@echo " plugins=$(plugins) inst_plugins=$(inst_plugins)"

$(bash_from_in_files): %: %.in
	echo "#!/bin/bash" > $@
	echo "# This is a generated file" >> $@
	sed $^\
 -e 's!@MODELS_DIR@!$(MODELS_DIR)!g'\
 -e 's!@NAVS_DIR@!$(NAVS_DIR)!g'\
 -e 's!@ENGINES_DIR@!$(ENGINES_DIR)!g'\
 -e 's!@PLUGINS_DIR@!$(PLUGINS_DIR)!g' >> $@
	chmod 755 $@


# built engine rules
%-engine.x3d: %_engineer.bash
	./$< > $@ || rm -f $@
	@if [ ! -e "$@" ] ; then exit 1 ; fi

engineFeaturesList.txt: mkEngines.bash all_engines.x3d.txt
	./mkEngines.bash GetAllFeatures > $@ || rm -f $@
	@if [ ! -e "$@" ] ; then exit 1 ; fi


$(built_engines): engineFeaturesList.txt

# build shared library plug-in rules
%.lo: %_plugin.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $^ -o $@

%.iio: %.lo
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CXXFLAGS) $($@_CPPFLAGS) -c $^ -o $@

$(test_bins): %: %.o
	$(CC) $(CXXFLAGS) $< -o $@ $($@_LDFLAGS)


# to un-install just 'rm -rf PREFIX'
install: $(installed)
ifneq ($(models),)
	install -d $(MODELS_DIR)
	cp $(models) $(MODELS_DIR)
endif
ifneq ($(inst_plugins),)
	install -d $(PLUGINS_DIR)
	cp $(inst_plugins) $(PLUGINS_DIR)
endif
ifneq ($(inst_engines),)
	install -d $(ENGINES_DIR)
	cp $(inst_engines) $(ENGINES_DIR)
endif
ifneq ($(navs),)
	install -d $(NAVS_DIR)
	cp $(navs) $(NAVS_DIR)
endif
ifneq ($(scripts),)
	install -d $(BIN_DIR)
	cp $(scripts) $(BIN_DIR)
endif


# We clean up the CMake shit too, just in case.
clean distclean cleaner:
	rm -f *.o *.lo *.iio Makefile cmake_install.cmake $(built)\
 engineFeaturesList.txt install_manifest.txt
	rm -rf CMakeFiles/

# See: more comments and still shorter than the corresponding
# CMakeLists.txt file.  Like it or not GNU make is powerful.
# Oops, now we have more lines, due to more comments and additions,
# which the CMakeLists.txt does not have.
