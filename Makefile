all: ladspa/zamvalve.so

ladspa/zamvalve.so: ladspa/zamvalve.dsp.cpp
	./compileladspa zamvalve.dsp

ladspa/zamvalve.dsp.cpp:
	./genladspa zamvalve.dsp

install:
	mkdir -p /usr/local/lib/ladspa
	cp -a ladspa/zamvalve.so /usr/local/lib/ladspa
	
uninstall:
	rm -f /usr/local/lib/ladspa/zamvalve.so

clean:
	rm -f ladspa/zamvalve.so
