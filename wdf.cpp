/*
wdf.cpp  Test classes for zamvalve
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
#include "wdf.h"

#define max(x,y) (( (x) > (y) ) ? x : y )

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
	T vgk = (v.vg - vk);
	T G = max(v.Gmin, (v.G0 + v.G1*vgk + v.G2*vgk*vgk + v.G3*vgk*vgk*vgk));
	//T G = v.G0 + v.G1*vgk + v.G2*vgk*vgk + v.G3*vgk*vgk*vgk;
	return (powf(((vk-v.ak)/(v.r0k*G)*(vk-v.ak)/(v.r0k*G)),1.0/3.0));
}

T h(Valve v, T vk) {
	T vgk = v.vg - vk;
	return (v.h0);
}

T mu(Valve v, T vk) {
	T vgk = (v.vg - vk);
	return (max(v.mumin, (v.mu0 + v.mu1*vgk + v.mu2*vgk*vgk + v.mu3*vgk*vgk*vgk)));
	//return (v.mu0 + v.mu1*vgk + v.mu2*vgk*vgk + v.mu3*vgk*vgk*vgk);
}

T beta(Valve v, T vg) {
	if (v.ag == vg)
		vg = v.ag + 0.01;
	T x = (powf(fabs(-1.0/v.D*(v.r0g/v.r0k*(((v.ak-v.vk)/(v.ag-vg))+1.0))),1.0/v.K));
	//if (v.ag == vg || v.ak == v.vk) 
	//	x = (powf(fabs(-1.0/v.D*(v.r0g/v.r0k)),1.0/v.K));
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

int main(){ 
	T Fs = 48000.0;
	int N = Fs;
	T gain = 4.0;
	T f0 = 1000.0;
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
	T ro = 1000e3;
	T rp = 100e3;
	T rg = 20e3;
	T ri = 1000e3;
	T rk = 1000; //from paper
	T e = 250.0;

	V Vi = V(0.0,1000.0);
	C Ci = C(ci, Fs);
	C Ck = C(ck, Fs);
	C Co = C(co, Fs);
	R Ro = R(ro);
	R Rg = R(rg);
	R Ri = R(ri);
	R Rk = R(rk);
	V E = V(e, rp);
#if 1
	ser S0 = ser(&Ci, &Vi);
	inv I0 = inv(&S0);
	par P0 = par(&I0, &Ri);
	inv I4 = inv(&P0);
	ser I1 = ser(&Rg, &I4);
	//inv I1 = inv(&S1);

	par I3 = par(&Ck, &Rk);
	//inv I3 = inv(&P1);
	
	ser S2 = ser(&Co, &Ro);
	inv I2 = inv(&S2);
	par P2 = par(&I2, &E);
#else

	ser S0 = ser(&Ci, &Vi);
	par P0 = par(&S0, &Ri);
	ser I1 = ser(&Rg, &P0);

	par I3 = par(&Ck, &Rk);

	ser S2 = ser(&Co, &Ro);
	par P2 = par(&S2, &E);
#endif	

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

	I1.waveUp();
	I3.waveUp();
	v.vk = I3.WU;
	v.vp = 0.0;
	I1.WD = 0.0;
	I3.WD = 0.0;
	P2.WD = 0.0;

	for (int j = 0; j < N; ++j) {
		//Step 1: read input sample as voltage for the source
		Vi.e = input[j];

		//Step 2: propagate waves up to the 3 roots
		I1.waveUp();
		I3.waveUp();
		P2.waveUp();
		
		v.vg = I1.WU + v.voff; //IMPORTANT!!

		//Step 3: compute wave reflections at non-linearity
		v.ag = I1.WU;		//-
		v.ak = I3.WU;		//-
		v.ap = P2.WU;		//+
		I1.WU = v.ag;		//-
		I3.WU = v.ak;		//-
		P2.WU = v.ap;		//+
		v.r0g = I1.PortRes;
		v.r0k = I3.PortRes;
		v.r0p = P2.PortRes;

		T vg0, vg1, vk0, vk1;

		T tol = 1e-4;
		int cnt = 0;
		//v.vg = I1.Voltage();	//-

		vk0 = v.vk;	//+
		vk1 = vk0 + f10(v, vk0);
		v.vk = secantf10(v, &vk0, &vk1);
#if 1	
		if (v.vg - v.vk <= v.voff+0.01) {
			goto Done;
 		} else {
			
			//initial guess for vg
			//vg0 = -I1.Voltage();
			vg0 = v.vg;
			vg1 = vg0 + f12(v,vg0);
			v.vg = secantf12(v, &vg0, &vg1) + v.voff;
			v.vk = secantf8(v, &vk0, &vk1);
Start:
			if (v.vg - v.vk <= v.voff+0.01) goto Done;
			
			v.vg = secantf12(v, &vg0, &vg1) + v.voff;
			v.vk = secantf8(v, &vk0, &vk1);

			if (++cnt > 3) goto Done;
			
			goto Start;
		}

Done:
#endif
		 
		/*
		if (v.vg < -e) v.vg = -e;
		if (v.vg > e) v.vg = e;
		if (v.vk < -e) v.vk = -e;
		if (v.vk > e) v.vk = e;
		*/
		v.bg = (2.0*v.vg - v.ag);
		v.bk = (2.0*v.vk - v.ak);
		I1.setWD(v.bg);
		DUMP(printf("\n"));
		I3.setWD(v.bk);
		DUMP(printf("\n"));
		
		
		v.vp = v.ap - v.r0p*((v.vg - v.ag)/v.r0g + (v.vk - v.ak)/v.r0k);
		
		//v.vp = v.vk + mu(v,v.vk)*(v.vk-v.vg-h(v,v.vk)+alpha(v,v.vk));
		//v.vp = v.vk + (v.vg - v.vk - v.voff)*beta(v,v.vg);

		//if (v.vp < -e) v.vp = -e;
		//if (v.vp > e) v.vp = e;
		v.bp = (2.0*v.vp - v.ap);

		//Step 4: propagate waves leaving non-linearity back to the leaves
		P2.setWD(-v.bp);		//-
		DUMP(printf("\n"));
		
		//v.vg = I1.Voltage(); 
		//v.vk = I3.Voltage();
		//v.vp = P2.Voltage(); 
		DUMP(printf("vg=%f\t\t\tvk=%f\t\t\tvp=%f\nag=%f\tbg=%f\tak=%f\tbk=%f\tap=%f\tbp=%f\n",v.vg,v.vk,v.vp,v.ag,v.bg,v.ak,v.bk,v.ap,v.bp));
		DUMP(printf("vg-ag=%.3f-\t\t\tvk-ak=%.3f+\t\t\tvp-ap=%.3f-\t\tIk=%f+ Ig=%f- Ip=%f\ng\n",v.vg-v.ag,v.vk-v.ak,v.vp-v.ap, -I3.Current(), -I1.Current(), -P2.Current()));

		//Step 5: measure the voltage across the output load resistance and set the sample
		output[j] = Ro.Voltage();
		//printf("%f %f %f %f %f %f %f %f\n", j/Fs, Vi.Voltage(), Ro.Voltage(), Rk.Voltage(), Rg.Voltage(),I1.Voltage(),Ri.Voltage(),P2.Current());
		printf("%f %f %f %f %f %f %f %f %f\n", j/Fs, Vi.Voltage(), Ro.Voltage(), I1.Voltage(),I3.Voltage(),P2.Voltage(),Ri.Voltage(),Rk.Voltage(),Rg.Voltage());
	}
}

