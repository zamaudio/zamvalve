#include <stdio.h>
#include <cmath>

typedef double T;

class WDF {
public:
	T WD;
	T WU;
	T PortRes;
	WDF();
	T Voltage();
	T state;
	virtual T waveUp() = 0;
	virtual void setWD(T waveparent) = 0;
};

class OnePort : public WDF {
public:
	T WD;
	T WU;
	void setWD(T waveparent);
};

class Adaptor : public OnePort {
public:
	Adaptor *left;
	Adaptor *right;
	Adaptor(bool flag);
};

class ser : public Adaptor {
public:
	template <class Port1, class Port2>ser(Port1 *left, Port2 *right);
	T waveUp();
	void setWD(T waveparent);	
};

class R : public Adaptor {
public:
	R(T res);
	T waveUp();
};

class C : public Adaptor {
public:
	C(T c, T fs);
	T waveUp();
};

class V : public Adaptor {
public:
	T e;
	V(T ee, T r);
	T waveUp();
};

WDF::WDF() {}

void OnePort::setWD(T val) {
	WD = val;
	WDF::WD = val;
	state = val;
}

T WDF::Voltage() {
	T Volts = (WU + WD) / 2.0;
	return Volts;
}

template <class Port1, class Port2>ser::ser(Port1 *l, Port2 *r) : Adaptor(true) {
	left = l;
	right = r;
	PortRes = l->PortRes + r->PortRes;
}

T ser::waveUp() {
	Adaptor::WU = -left->waveUp() - right->waveUp();
	WDF::WU = -left->waveUp() - right->waveUp();
	return WU;
}

Adaptor::Adaptor(bool flag) {
	if (!flag) {
		left = NULL;
		right = NULL;
	}
}

void ser::setWD(T waveparent) {
	Adaptor::setWD(waveparent);
	left->setWD(left->WU-(2.0*left->PortRes/(PortRes+left->PortRes+right->PortRes))*(waveparent+left->WU+right->WU));
	right->setWD(right->WU-(2.0*right->PortRes/(PortRes+left->PortRes+right->PortRes))*(waveparent+left->WU+right->WU));
}

R::R(T res) : Adaptor(false) {
	PortRes = res;
}

T R::waveUp() {
	WU = 0.0;
	return WU;
}

C::C(T c, T fs) : Adaptor(false) {
	PortRes = 1.0/(2.0*c*fs);
	state = 0.0;
}

T C::waveUp() {
	WU = state;
	return WU;
}

V::V(T ee, T r) : Adaptor(false) {
	e = ee;
	PortRes = r;
	WD = 0.0;  //always?
}

T V::waveUp() {
	WU = 2.0*e - WD;
	return WU;
}


int main(){ 
	T Fs = 48000.0;
	int N = Fs/10.0;
	T gain = 30.0;
	T f0 = 100.0;
	T input[5000] = { 0.0 };
	T output[5000] = { 0.0 };
	int i;
	for (i = 0; i < N; ++i) {
		input[i] = gain*sin(2.0*M_PI*f0/Fs*i);
	}

	//Model
	V V1 = V(30.0,1.0);
	R R1 = R(80.0);
	T capval = 0.000035;
	C C1 = C(capval,Fs);
	ser S0 = ser(&C1,&R1);
	ser S1 = ser(&V1, &S0);
	
	T vdiode = 0.0;
	T rdiode = 0.0;
	T r = 0.0;
	for (int j = 0; j < N; ++j) {
		V1.e = input[j];
		S1.waveUp();
		rdiode = 125.56*exp(-0.036*vdiode);
		r = (rdiode-S1.PortRes)/(rdiode+S1.PortRes);
		S1.setWD(r*S1.WU);
		vdiode = (S1.WD+S1.WU)/2.0;
		output[j] = R1.Voltage();//-S1.right->Voltage();
		printf("%f %f %f\n", j/Fs, input[j], output[j]);
	}
}

