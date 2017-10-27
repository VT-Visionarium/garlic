IR_ROOT = /opt/instantReality
CXXFLAGS = -Wall -Werror -g
CPPFLAGS = -fpic -I$(IR_ROOT)/include
LDFLAGS = -shared -ldtk -L$(IR_ROOT)/lib -lInstantIO\
 -Wl,-rpath=/usr/local/lib

built := $(patsubst %.cpp,%.iio,$(wildcard *.cpp))

build: $(built)

%.iio: %.lo
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

%.lo: %.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $^ -o $@

clean cleaner:
	rm -f *.lo $(built)

