PREFIX=/usr/local

all: zamvalve-tanh.so

zamvalve-tanh.so: zamvalve-tanh.dsp.cpp
	g++ -O3 -ffast-math -fPIC -shared -Dmydsp=zamvalve_tanh -I/usr/lib zamvalve-tanh.dsp.cpp -o zamvalve-tanh.so

install:
	mkdir -p $(DESTDIR)$(PREFIX)/lib/ladspa
	cp zamvalve-tanh.so $(DESTDIR)$(PREFIX)/lib/ladspa
	
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/lib/ladspa/zamvalve-tanh.so

clean:
	rm -f zamvalve-tanh.so
