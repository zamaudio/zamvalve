/*
wdf.cpp
Copyright (C) 2013  Damien Zammit

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <stdio.h>
#include <inttypes.h>
#include <cmath>

#define ONEPORT 0
#define PASSTHROUGH 1
#define THREEPORT 2
#define max(x,y) (( (x) > (y) ) ? x : y )

typedef double T;

// Works on little-endian machines only
inline bool is_nan(T& value ) {
	if (((*(uint32_t *) &value) & 0x7fffffff) > 0x7f800000) {
		return true;
	}
	return false;
}

inline void sanitize_denormal(T& value) {
	if (is_nan(value)) {
		value = 0.0;
	}
}

typedef struct valve {
	T r0g; T ag; T bg; T vg;
	T r0k; T ak; T bk; T vk;
	T r0p; T ap; T bp; T vp;
	T D, K, voff, mumin, mu0, mu1, mu2, mu3, h0, Gmin, G0, G1, G2, G3;
} Valve;

T alpha(Valve v, T vk) {
	T vgk = v.vg - vk;
	T G = max(v.Gmin, v.G0 + v.G1*vgk + v.G2*vgk*vgk + v.G3*vgk*vgk*vgk);
	return (powf(fabs((vk-v.ak)/(v.r0k*G)*(vk-v.ak)/(v.r0k*G)),1.0/3.0));
}

T h(Valve v, T vk) {
	T vgk = v.vg - vk;
	return (v.h0);
}

T mu(Valve v, T vk) {
	T vgk = v.vg - vk;
	return (max(v.mumin, v.mu0 + v.mu1*vgk + v.mu2*vgk*vgk + v.mu3*vgk*vgk*vgk));
}

T beta(Valve v, T vg) {
	if (v.ag == vg) return 0.0;
	T x = (powf(fabs(-1.0/v.D*(v.r0g/v.r0k*(((v.ak-v.vk)/(v.ag-vg))+1.0))),1.0/v.K));
	return x;
}

T f8(Valve v, T vkn) {
	return ((v.r0k*(v.ap+mu(v,vkn)*(v.vg+h(v,vkn)-alpha(v,vkn))+v.r0p*v.ak)/(v.r0p+(mu(v,vkn)+1.0)*v.r0k))-(v.r0k*v.r0p*(v.vg-v.ag+v.voff))/(v.r0g*(v.r0p+(mu(v,vkn)+1.0)*v.r0k))-vkn);
}

T f10(Valve v, T vkn) {
	return ((v.r0k*(v.ap+mu(v,vkn)*(v.ag+h(v,vkn)-alpha(v,vkn))+v.r0p*v.ak)/(v.r0p+(mu(v,vkn)+1.0)*v.r0k))-vkn);
}

T f12(Valve v, T vgn) {
	return ((v.r0g*((v.voff+v.vk)*beta(v,vgn)-v.vk+v.ap)+v.r0p*v.ag)/(v.r0g*beta(v, vgn)+v.r0p)+(v.r0g*v.r0p*(v.ak-v.vk))/(v.r0k*(v.r0g*beta(v,vgn)+v.r0p))-vgn);
}

T secantf8(Valve v, T *i1, T *i2) {
	T tolerance = 1e-9;
	T vkn = 0.0;
	for (int i = 0; i < 9; ++i) {
		vkn = *i1 - f8(v,*i1)*(*i1-*i2)/(f8(v,*i1)-f8(v,*i2));
		*i1 = *i2;
		*i2 = vkn;
		if (fabs(f8(v,vkn)) < tolerance) break;
	} 
	//printf("%f\n",vkn);
	return vkn;
}

T secantf10(Valve v, T *i1, T *i2) {
	T tolerance = 1e-9;
	T vkn = 0.0;
 	for (int i = 0; i<9; ++i) {
		vkn = *i1 - f10(v,*i1)*(*i1-*i2)/(f10(v,*i1)-f10(v,*i2));
		*i1 = *i2;
		*i2 = vkn;
		if (fabs(f10(v,vkn)) < tolerance) break;
	}
	return vkn;
}

T secantf12(Valve v, T *i1, T *i2) {
	T tolerance = 1e-9;
	T vgn = 0.0;
 	for (int i = 0; i<9; ++i) {
		vgn = *i1 - f12(v,*i1)*(*i1-*i2)/(f12(v,*i1)-f12(v,*i2));
		*i1 = *i2;
		*i2 = vgn;
		if (fabs(f12(v,vgn)) < tolerance) break;
	}
	return vgn;
}


class WDF {
public:
	T WD;
	T WU;
	T PortRes;
	WDF();
	T Voltage();
	T Current();
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
//	WDF::WD = val;
	state = val;
}

T WDF::Voltage() {
	T Volts = (WU + WD) / 2.0;
	return Volts;
}

T WDF::Current() {
	T Amps = (WU - WD) / (2.0*PortRes);
	return Amps;
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
	WU = 0.0;
	WD = 0.0;
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
	WDF::WU = -left->waveUp(); 		//-
	return WU;
}

void inv::setWD(T waveparent) {
	Adaptor::setWD(waveparent);
	//WDF::WD = waveparent;		//-
	left->WD = -waveparent;		//-
	left->setWD(-waveparent);	//-
	
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
	int N = Fs*2;
	T gain = 2.5;
	T f0 =1200.0;
	T input[384000] = { 0.0 };
	T output[384000] = { 0.0 };
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
	T rk = 1e3; //from paper
	T e = 250.0;

	V Vi = V(0.0,1e4);
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
	ser I1 = ser(&Rg, &P0);

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

		//Step 2: propagate waves up to the 3 roots
		I1.waveUp();
		P1.waveUp();
		P2.waveUp();

		//Step 3: compute wave reflections at non-linearity
		v.vk = 0.0;
		v.vg = 0.0;
		v.vp = 0.0;
		v.ag = I1.WU;
		v.ak = P1.WU;
		v.ap = P2.WU;
		v.r0g = I1.PortRes;
		v.r0k = P1.PortRes;
		v.r0p = P2.PortRes;

		T vg0, vg1, vk0, vk1;

		T tol = 1e-4;
		int cnt = 0;
		v.vg = v.ag;

		vk0 = v.ak;
		vk1 = v.ak + f10(v, v.ak);
		v.vk = secantf10(v, &vk0, &vk1);
		
		if (v.vg - v.vk <= v.voff) {
			goto Done;
 		} else {
			
			//initial guess for vg
			vg0 = v.ag;
			vg1 = v.ag + f12(v,v.ag+0.0001);
			v.vg = secantf12(v, &vg0, &vg1);
			v.vk = secantf8(v, &vk0, &vk1);
Start:
			if (v.vg - v.vk <= v.voff) goto Done;
			
			v.vg = secantf12(v, &vg0, &vg1);
			v.vk = secantf8(v, &vk0, &vk1);

			if (++cnt > 2) goto Done;
			
			goto Start;
		}

Done:

		v.vp = v.ap - v.r0p*((v.vg-v.ag)/v.r0g + (v.vk - v.ak)/v.r0k);
		//sanitize_denormal(v.vg);
		//sanitize_denormal(v.vk);
		//sanitize_denormal(v.vp);
		
//		printf("g:%f k:%f p:%f\n",v.vg,v.vk,v.vp);

		v.bg = 2.0*v.vg - v.ag;
		v.bk = 2.0*v.vk - v.ak;
		v.bp = 2.0*v.vp - v.ap;

		//Step 4: propagate waves leaving non-linearity back to the leaves
		I1.setWD(v.bg);
		P1.setWD(v.bk);
		P2.setWD(v.bp);

		//Step 5: measure the voltage across the output load resistance and set the sample
		output[j] = Ro.Voltage();
		printf("%f %f %f\n", j/Fs, input[j], output[j]);
	}
}

