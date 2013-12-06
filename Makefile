PREFIX ?= /usr/local
LIBDIR ?= lib
OPTIMIZATIONS ?= -O3 -ffast-math

all: zamvalve-tanh.so

zamvalve-tanh.so: zamvalve-tanh.dsp.cpp
	$(CXX) $(OPTFLAGS) -fPIC -shared -Dmydsp=zamvalve_tanh -I/usr/lib zamvalve-tanh.dsp.cpp -o zamvalve-tanh.so

install:
	install -d $(DESTDIR)$(PREFIX)/$(LIBDIR)/ladspa
	install zamvalve-tanh.so -t $(DESTDIR)$(PREFIX)/$(LIBDIR)/ladspa
	
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/$(LIBDIR)/ladspa/zamvalve-tanh.so

clean:
	rm -f zamvalve-tanh.so
