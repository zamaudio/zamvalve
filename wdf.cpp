#include <stdio.h>
#include <cmath>

#define ONEPORT 0
#define PASSTHROUGH 1
#define THREEPORT 2
#define max(x,y) (( (x) > (y) ) ? x : y )

typedef double T;

typedef struct valve {

	T r0g; T ag; T bg; T vg;
	T r0k; T ak; T bk; T vk;
	T r0p; T ap; T bp; T vp;
	T D, K, voff, mumin, mu0, mu1, mu2, mu3, h0, Gmin, G0, G1, G2, G3;
} Valve;

T alpha(Valve v, T vk) {
	T vgk = v.vg - vk;
	T G = max(v.Gmin, v.G0*vgk*0.25 + v.G1*vgk*0.5 + v.G2*vgk*0.75 + v.G3*vgk);
	return (powf((vk-v.ak)/(v.r0k*G),2.0/3.0));
}

T h(Valve v, T vk) {
	T vgk = v.vg - vk;
	return (v.h0*vgk*1.0/2.0);
}

T mu(Valve v, T vk) {
	T vgk = v.vg - vk;
	return (max(v.mumin, v.mu0*vgk*0.25 + v.mu1*vgk*0.5 + v.mu2*vgk*0.75 + v.mu3*vgk));
}

T beta(Valve v, T vk) {
	return (powf(-1.0/v.D*(v.r0g/v.r0k*((v.ak-vk)/(v.ag-v.vg)+1.0)),1.0/v.K));
}

T f0(Valve v, T vkn) {
	return ((v.r0k*(v.ap+mu(v,vkn)*(v.vg+h(v,vkn)-alpha(v,vkn))+v.r0p*v.ak)/(v.r0p+(mu(v,vkn)+1.0)*v.r0k))-(v.r0k*v.r0p*(v.vg-v.ag))/(v.r0g*(v.r0p+(mu(v,vkn)+1.0)/v.r0k))-vkn);
}

T f1(Valve v, T vkn) {
	return ((v.r0k*(v.ap+mu(v,vkn)*(v.ag+h(v,vkn)-alpha(v,vkn))+v.r0p*v.ak)/(v.r0p+(mu(v,vkn)+1.0)*v.r0k))-vkn);
}

T secantf1(Valve v, T i1, T i2) {
	T tolerance = 1e-6;
	T vkn = 0.0;
	do {
		vkn = i1 - f1(v,i1)*(i1-i2)/(f1(v,i1)-f1(v,i2));
		i2 = i1;
		i1 = vkn;
	} while (fabs(f1(v,vkn)) > tolerance);
}

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
	void setWD(T waveparent);
};

class Adaptor : public OnePort {
public:
	WDF *left;
	WDF *right;
	Adaptor(int flag);
};

class inv : public Adaptor { 
public:
	template <class Port>inv(Port *left);
	T waveUp();
	void setWD(T waveparent);
};

class ser : public Adaptor {
public:
	template <class Port1, class Port2>ser(Port1 *left, Port2 *right);
	T waveUp();
	void setWD(T waveparent);	
};

class par : public Adaptor {
public:
	template <class Port1, class Port2>par(Port1 *left, Port2 *right);
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

template <class Port1, class Port2>ser::ser(Port1 *l, Port2 *r) : Adaptor(THREEPORT) {
	left = l;
	right = r;
	PortRes = l->PortRes + r->PortRes;
}

template <class Port>inv::inv(Port *l) : Adaptor(PASSTHROUGH) {
	left = l;
	PortRes = l->PortRes;
}

T ser::waveUp() {
	Adaptor::WU = -left->waveUp() - right->waveUp();
	WDF::WU = -left->waveUp() - right->waveUp();
	return WU;
}

template <class Port1, class Port2>par::par(Port1 *l, Port2 *r) : Adaptor(THREEPORT) {
	left = l;
	right = r;
	PortRes = 1.0 / (1.0 / l->PortRes + 1.0 / r->PortRes);
}

T par::waveUp() {
	T G23 = 1.0 / left->PortRes + 1.0 / right->PortRes;
	Adaptor::WU = (1.0 / left->PortRes)/G23*left->waveUp() + (1.0 / right->PortRes)/G23*right->waveUp();
	WDF::WU = (1.0 / left->PortRes)/G23*left->waveUp() + (1.0 / right->PortRes)/G23*right->waveUp();
	return WU;
}

Adaptor::Adaptor(int flag) {
	switch (flag) {
		case ONEPORT:
			left = NULL;
			right = NULL;
			break;
		case PASSTHROUGH:
			right = NULL;
			break;
		default:
		case THREEPORT:
			break;
	}
}

void ser::setWD(T waveparent) {
	Adaptor::setWD(waveparent);
	left->setWD(left->WU-(2.0*left->PortRes/(PortRes+left->PortRes+right->PortRes))*(waveparent+left->WU+right->WU));
	right->setWD(right->WU-(2.0*right->PortRes/(PortRes+left->PortRes+right->PortRes))*(waveparent+left->WU+right->WU));
}

void par::setWD(T waveparent) {
	Adaptor::setWD(waveparent);
	T p = 2.0*(waveparent/PortRes + left->WU/left->PortRes + right->WU/right->PortRes)/(1.0/PortRes + 1.0/left->PortRes + 1.0/right->PortRes);

	left->setWD(p - left->WU);
	right->setWD(p - right->WU);
}

T inv::waveUp() {
	Adaptor::WU = -left->WD; // polarity? WD
	WDF::WU = -left->WD;
	return WU;
}

void inv::setWD(T waveparent) {
	//Adaptor::setWD(-waveparent);
	WD = waveparent;
	left->setWD(waveparent);
}

R::R(T res) : Adaptor(ONEPORT) {
	PortRes = res;
}

T R::waveUp() {
	WU = 0.0;
	return WU;
}

C::C(T c, T fs) : Adaptor(ONEPORT) {
	PortRes = 1.0/(2.0*c*fs);
	state = 0.0;
}

T C::waveUp() {
	WU = state;
	return WU;
}

V::V(T ee, T r) : Adaptor(ONEPORT) {
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
	T gain = 4.0;
	T f0 = 100.0;
	T input[5000] = { 0.0 };
	T output[5000] = { 0.0 };
	int i;
	for (i = 0; i < N; ++i) {
		input[i] = gain*sin(2.0*M_PI*f0/Fs*i);
	}

	//Model
	T ci = 100e-9;
	T ck = 10e-6;
	T co = 10e-9;
	T ro = 1e6;
	T rp = 100e3;
	T rg = 20e3;
	T ri = 1e6;
	T rk = 100; //Made up value
	T e = 250;

	V Vi = V(0.0,100.0);
	C Ci = C(ci, Fs);
	C Ck = C(ck, Fs);
	C Co = C(co, Fs);
	R Ro = R(ro);
	R Rg = R(rg);
	R Ri = R(ri);
	R Rk = R(rk);
	V E = V(e, rp);
	

	ser S0 = ser(&Ci, &Vi);
	inv I0 = inv(&S0);
	par P0 = par(&I0, &Ri);
	ser S1 = ser(&Rg, &P0);
	inv I1 = inv(&S1);

	par P1 = par(&Ck, &Rk);

	ser S2 = ser(&Co, &Ro);
	inv I2 = inv(&S2);
	par P2 = par(&I2, &E);

/*
	ser S0 = ser(&Ci, &Vi);
	par P0 = par(&S0, &Ri);
	ser S1 = ser(&Rg, &P0);

	par P1 = par(&Ck, &Rk);

	ser S2 = ser(&Co, &Ro);
	par P2 = par(&S2, &E);
*/	
	Valve v;
	v.D = 0.12;
	v.K = 1.1;
	v.voff = -0.2;
	v.mumin = 1e-9;
	v.mu0 = 99.705;
	v.mu1 = -22.98e-3;
	v.mu2 = -0.4489;
	v.mu3 = -22.27e-3;
	v.h0 = 0.6;
	v.Gmin = 1e-9;
	v.G0 = 1.102e-3;
	v.G1 = 15.12e-6;
	v.G2 = -31.56e-6;
	v.G3 = -3.286e-6;

	for (int j = 0; j < N; ++j) {
		//Step 1: read input sample as voltage for the source
		Vi.e = input[j];

		//Step 2: propagate waves up to the root
		I1.waveUp();
		P1.waveUp();
		P2.waveUp();

		//Step 3: compute wave reflections at non-linearity
		//rdiode = 125.56*exp(-0.036*vdiode);
		//r = (rdiode-S1.PortRes)/(rdiode+S1.PortRes);
		v.ag = S1.WU;
		v.ak = P1.WU;
		v.ap = P2.WU;
		v.r0g = S1.PortRes;
		v.r0k = P1.PortRes;
		v.r0p = P2.PortRes;

		v.vg = v.ag;
		
		T vk0 = v.ak;
		T vk1 = v.ak + f1(v, v.ak);
		v.vk = secantf1(v, vk0, vk1);
		
		v.vp = v.ap - v.r0p*((v.vg-v.ag)/v.r0g + (v.vk - v.ak)/v.r0k);
		
		//printf("g:%f k:%f p:%f\n",v.vg,v.vk,v.vp);

		v.bg = 2.0*v.vg - v.ag;
		v.bk = 2.0*v.vk - v.ak;
		v.bp = 2.0*v.vp - v.ap;

		//Step 4: propagate waves leaving non-linearity back to the leaves
		I1.setWD(v.bg);
		P1.setWD(v.bk);
		P2.setWD(v.bp);

		//Step 5: remember the old voltages for next time
		v.vg = (I1.WD+I1.WU)/2.0;
		v.vk = (P1.WD+P1.WU)/2.0;
		v.vp = (P2.WD+P2.WU)/2.0;

		//Step 6: measure the voltage across the output load resistance and set the sample
		output[j] = Ro.Voltage();
		printf("%f %f %f\n", j/Fs, input[j], output[j]);
	}
}

