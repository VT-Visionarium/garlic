SHELL = /bin/bash

LINUX_GLX64_INCS = -I/usr/include
LINUX_GLX64_LIBS = -lGL -L/usr/X11R6/lib64 -lX11 -lXi
CFLAGS = -g -DWIN_GLX -DSHM_DUMMY -DSHM_SVR4MMAP -DSEM_SYSVIPC -DHOST='$(UNAME)' -DARCH='"linux"' $(LINUX_GLX64_INCS)

GLX_FREEVR_LIB = libfreevr_64.so
FREEVR_LIB = $(GLX_FREEVR_LIB)
APP_FLAGS = -DFREEVR
drawing.o_CFLAGS = $(APP_FLAGS)
APP_LIBS = -L. -lfreevr_64 $(LINUX_GLX64_LIBS) -ldl -lm

PREFIX = /usr/local/encap/freevr_0.6f_debug

VERSION_TAG = "freevr_0.6f"
VERSION_CVSTAG = "freevr_0-6f"
VERSION_MSG = "release 0.6f"

build: travel inputs configurator $(FREEVR_LIB)

travel inputs configure: $(FREEVR_LIB)


# =======================================================
# rules for making freevr library
# =======================================================
FREEVR_SRC = \
	vr_system.c \
	vr_config.c \
	vr_procs.c \
	vr_objects.c \
	vr_entity.c \
	vr_parse.c \
	vr_callback.c \
	vr_serial.c \
	vr_socket.c \
	vr_shmem.c \
	vr_math.c \
	vr_telnet.c \
	vr_visren.c \
	vr_visren.txt.c \
	vr_basicgfx.glx.c \
	vr_utils.c \
	vr_sem_tcp.c \
	vr_input.c \
	vr_input.static.c \
	vr_input.shmemd.c \
	vr_input.asc_fob.c \
	vr_input.asc_ms.c \
	vr_input.fastrak.c \
	vr_input.joydev.c \
	vr_input.evio.c \
	vr_input.magellan.c \
	vr_input.spacetec.c \
	vr_input.pinch.c \
	vr_input.cyberglove.c \
	vr_input.dtrack.c \
	vr_input.vruidd.c \
	vr_input.vrpn.c \
	vr_output.shmemd.c \
	vr_output.vruidd.c \
	vr_debug.c

# source files used in particular versions of the library
#   (OpenGL vs. Performer vs. Cygwin).
FREEVR_GLX_SRC = \
	vr_visren.glx.c \
	vr_input.xwindows.c


FREEVR_HEAD = freevr.h \
	vr_context.h \
	vr_system.h \
	vr_config.h \
	vr_procs.h \
	vr_objects.h \
	vr_entity.h \
	vr_enums.h \
	vr_parse.h \
	vr_callback.h \
	vr_serial.h \
	vr_socket.h \
	vr_shmem.h \
	vr_math.h \
	vr_visren.h \
	vr_visren.opts.h \
	vr_visren.txt.h \
	vr_utils.h \
	vr_sem_tcp.h \
	vr_basicgfx.glx.h \
	vr_input.h \
	vr_input.opts.h \
	vr_debug.h

FREEVR_GLX_HEAD = \
	vr_visren.glx.h


FREEVR_GLX_OBJ = $(FREEVR_SRC:.c=.o) $(FREEVR_GLX_SRC:.c=.o)

travel.o drawing.o $(FREEVR_GLX_OBJ): lance_debug.h

# We seem to need the extra -lm
$(GLX_FREEVR_LIB): $(FREEVR_GLX_OBJ)
	$(CC) -shared $(CFLAGS) -o $(GLX_FREEVR_LIB) $(FREEVR_GLX_OBJ) -lm


#
# Some object files have special dependencies
vr_shmem.o: vr_shmem.c vr_shmem.dummy.c
vr_visren.o: vr_visren.c vr_visren.h vr_visren.opts.h
vr_input.o: vr_input.c vr_input.h vr_input.opts.h


DUMMY_SRC = vr_shmem.dummy.c

EXAMPLE_SRC = static.c travel.c valtest.c simple.c configurator.c drawing.c \
	pfex3_dynamic.c++ pfTravel.c++ \
	fvconfig.c serialspy.c socketspy.c

OTHER_FILES = Makefile Make-config Make-arch configure \
	README \
	freevr.bnf indent.style \
	freevr_back.xbm freevr_icon.xbm \
	vr_input.vrpn.cxx

#
# The executable files for the distribution
FREEVR_BIN = cave2fvr ipcrms fvri.tcl

#
# The et cetera files for the distribution
FREEVR_ETC = \
	rc_sample_cave rc_sample_6dof rc_sample_2head \
	rc_sample_cygwin rc_sample_joydev rc_sample_evio rc_sample_vruiddserver \
	rc_sample_dtrack rc_sample_vrpn rc_sample_ks

#
# The documentation files for the distribution
FREEVR_DOC = README_dist contrib coverage todo \
	index.html libdev_guide.html vradmin_guide.html user_guide.html \
	appdev_guide.html functions.html telnet.html tutorials.html \
	downloads.html


# =======================================================
# rules for making example VR programs
# =======================================================

travel: $(FREEVR_LIB) travel.o drawing.o
	$(CC) $(CFLAGS) -o $@ travel.o drawing.o $(APP_LIBS)

static: $(FREEVR_LIB) static.o drawing.o
	$(CC) $(CFLAGS) -o $@ static.o drawing.o $(APP_LIBS)

inputs: $(FREEVR_LIB) valtest.o drawing.o
	$(CC) $(CFLAGS) -o $@ valtest.o drawing.o $(APP_LIBS)

configurator: $(FREEVR_LIB) configurator.o drawing.o
	$(CC) $(CFLAGS) -o $@ configurator.o drawing.o $(APP_LIBS)

## NOTE: -lGLU must NOT be used with Cygwin (odd, but that's the way it is)
simple: $(FREEVR_LIB) simple.o
	$(CC) $(CFLAGS) -o $@ simple.o $(APP_LIBS)
	#$(CC) $(CFLAGS) -o $@ simple.o $(APP_LIBS) -lGLU

## TODO: see if we need the following target (ie. "simple.o")
simple.o: simple.c
	$(CC) $(CFLAGS) -c $(APP_FLAGS) simple.c

fvconfig: $(FREEVR_LIB) fvconfig.o
	$(CC) $(CFLAGS) -o $@ fvconfig.o $(APP_LIBS)

mkprefix:
	mkdir -p $(PREFIX)/bin $(PREFIX)/include $(PREFIX)/lib $(PREFIX)/etc


install: build mkprefix
	cp travel $(PREFIX)/bin/freevr_test
	cp $(FREEVR_LIB) $(PREFIX)/lib
	cp vr_*.h freevr.h $(PREFIX)/include
	cp ../hy_VT_freevrrc $(PREFIX)/etc


clean:
	-/bin/rm -f *.o static travel inputs configurator ex7+ core $(INDEVTESTS) $(UTILITIES) $(FREEVR_LIB)
	rm -f cscope.out


.c.o:
	$(CC) -fPIC $(CFLAGS) $($@_CFLAGS) -c $< -o $@

