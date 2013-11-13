PREFIX ?= /usr/local

all: zamvalve-tanh.so

zamvalve-tanh.so: zamvalve-tanh.dsp.cpp
	g++ -O3 -ffast-math -fPIC -shared -Dmydsp=zamvalve_tanh -I/usr/lib zamvalve-tanh.dsp.cpp -o zamvalve-tanh.so

install:
	install -d $(DESTDIR)$(PREFIX)/lib/ladspa
	install zamvalve-tanh.so -t $(DESTDIR)$(PREFIX)/lib/ladspa
	
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/lib/ladspa/zamvalve-tanh.so

clean:
	rm -f zamvalve-tanh.so
