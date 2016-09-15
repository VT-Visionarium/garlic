# GNU makefile

CXXFLAGS = -g -Wall

VRPN_PREFIX = /usr/local/encap/vrpn-v07.33
IR_ROOT = /opt/instantReality


VRPN_CPPFLAGS = -I$(VRPN_PREFIX)/include
VRPN_LDFLAGS = -lvrpn -L$(VRPN_PREFIX)/lib 

libs = ZSpaceNode.iio

ZSpaceNode.lo_CPPFLAGS = -fpic $(VRPN_CPPFLAGS) -I$(IR_ROOT)/include
ZSpaceNode.iio_LDFLAGS = $(VRPN_LDFLAGS) -L$(IR_ROOT)/lib -lInstantIO -Wl,-rpath=$(IR_ROOT)/lib


build: $(libs)


# build shared library plug-in rules
%.lo: %.cpp
	$(CXX) $(CXXFLAGS) $($@_CPPFLAGS) -c $^ -o $@

%.iio: %.lo
	$(CXX) $(CXXFLAGS) $^ -o $@ -shared $($@_LDFLAGS)

clean:
	rm -f *.lo *.o *.iio

