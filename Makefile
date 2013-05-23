all: lv2/zamvalve.lv2/zamvalve.ttl ladspa/zamvalve.so

lv2/zamvalve.lv2/zamvalve.ttl: lv2/zamvalve.cpp
	./compilelv2 zamvalve.dsp

lv2/zamvalve.cpp:
	./genlv2 zamvalve.dsp	

ladspa/zamvalve.so: ladspa/zamvalve.dsp.cpp
	./compileladspa zamvalve.dsp

ladspa/zamvalve.dsp.cpp:
	./genladspa zamvalve.dsp

install:
	mkdir -p /usr/local/lib/lv2
	mkdir -p /usr/local/lib/ladspa
	cp -a lv2/zamvalve.lv2 /usr/local/lib/lv2
	cp -a ladspa/zamvalve.so /usr/local/lib/ladspa
	
uninstall:
	rm -fr /usr/local/lib/lv2/zamvalve.lv2
	rm -f /usr/local/lib/ladspa/zamvalve.so

clean:
	rm -fr lv2/zamvalve.lv2
	rm -f ladspa/zamvalve.so
