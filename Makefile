PREFIX=/usr

all: zamvalve-tanh.so

zamvalve-tanh.so: zamvalve-tanh.dsp.cpp
	g++ -O3 -mfpmath=sse -msse -msse2 -ffast-math -fPIC -shared -Dmydsp=zamvalve_tanh -I/usr/lib zamvalve-tanh.dsp.cpp -o zamvalve-tanh.so

install:
	mkdir -p $(PREFIX)/lib/ladspa
	cp zamvalve-tanh.so $(PREFIX)/lib/ladspa
	
uninstall:
	rm -f $(PREFIX)/lib/ladspa/zamvalve-tanh.so

clean:
	rm -f zamvalve-tanh.so
