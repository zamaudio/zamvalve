#include "wdf.h"
#include <cmath>
#include <stdio.h>

WDF::WDF() {

}

void WDF::setWD(T val) {
	wD = val;
}

void WDF::setRp(T r) {
	rp = r;
}

void WDF::setWU(T val) {
	wU = val;
}

T WDF::waveUp() {
	return wU;
}

T WDF::getRp() {
	return rp;
}

void WDF::dump() {	
	printf("wD=%.4f wU=%.4f Rp=%.4f\n",wD,wU,rp);
}

Serial::Serial(WDF a, WDF b) : Adaptor(&a, &b) {
	Serial::setRp(Adaptor::l->getRp() + Adaptor::r->getRp());
	dump();
}

T Serial::waveUp() {
	WDF::setWU(-(Adaptor::l->waveUp() + Adaptor::r->waveUp()));
	l->dump();
	r->dump();
	return (wU);
}

void Serial::setWD(T waveparent) {
	Serial::wD = waveparent;
	Adaptor::l->setWD((Adaptor::l->wU - (Adaptor::l->getRp()/rp)*(waveparent + Adaptor::l->wU + Adaptor::r->wU)));
	Adaptor::r->setWD((Adaptor::r->wU - (Adaptor::r->getRp()/rp)*(waveparent + Adaptor::l->wU + Adaptor::r->wU)));
	
	dump();
	Adaptor::l->dump();
	Adaptor::r->dump();
}

OnePort::OnePort(T r) : WDF() {
	setRp(r);
	dump();
}

Adaptor::Adaptor(WDF *a, WDF *b) : WDF() {
	Adaptor::l = a;
	Adaptor::r = b;
}

void OnePort::setWD(T val) {
	wD = val;
	state = val;
}

R::R(T r) : OnePort(r) {
	printf("R\n");
}

T R::waveUp() {
	wU = 0.0;
	return wU;
}

T R::voltage() {
	T volts = ((wU + wD) / 2.0);
	return volts;
}

C::C(T r) : OnePort(r) {
	printf("C\n");
	state = 0.0;
}

T C::waveUp() {
	wU = state;
	return (wU);
}

V::V(T ee, T r) : OnePort(r) {
	printf("V\n");
	e = ee;
	wD = 0.0;
}

T V::waveUp() {
	V::wU = 2.0*e - wD;
	return (V::wU);
}


int main(){ 
	T Fs = 48000.0;
	int N = Fs/100.0;
	T gain = 30.0;
	T f0 = 100.0;
	T input[5000] = { 0.0 };
	T output[5000] = { 0.0 };
	int i;
	for (i = 0; i < N; ++i) {
		input[i] = gain*sin(2.0*M_PI*f0/Fs*i);
	}

	//Model
	V V1 = V(0.0,1.0);
	R R1 = R(80.0);
	T capval = 0.000035;
	C C1 = C(1.0/(2.0*capval*Fs));
	Serial S1 = Serial(V1, R1);
	T vdiode = 0.0;
	T rdiode = 0.0;
	T r = 0.0;
	for (int j = 0; j < N; ++j) {
		V1.e = input[j];
		S1.waveUp();
		rdiode = 125.56*exp(-0.036*vdiode);
		r = (rdiode-S1.rp)/(rdiode+S1.rp);
		S1.setWD(r*S1.wU);
		R1.dump();
		vdiode = (S1.wD+S1.wU)/2.0;
		output[j] = R1.voltage();
		printf("%f %f %f\n", j/Fs, input[j], output[j]);
	}
}
