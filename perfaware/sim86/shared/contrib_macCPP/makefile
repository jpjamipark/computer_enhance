CXX=g++
CXXFLAGS=-std=c++17
SHAREDI=..
LIBI=.

all: sim86lib sharedtest memory.o

sim86lib: libsim86mac.so

sharedtest: sharedlibtest.o

memory.o: memory.cpp sim86lib
	$(CXX) $(CXXFLAGS) -I$(SHAREDI) -L$(LIBI) -o $(@D)/$@ -lsim86mac $<

sharedlibtest.o: ../shared_library_test.cpp sim86lib
	$(CXX) $(CXXFLAGS) -I$(SHAREDI) -L$(LIBI) -o $(@D)/$@ -lsim86mac $<

libsim86mac.so: ../../sim86_lib.cpp
	$(CXX) $(CXXFLAGS) -shared -o $(@D)/$@ -fPIC $<

clean:
	rm -rf *.o *.so
