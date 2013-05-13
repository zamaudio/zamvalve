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
#define min(x,y) (( (x) < (y) ) ? x : y )
#define sign(x) ( (x) >= 0.0 ? 1.0 : -1.0 )
#define BIG 1e11
#define SMALL 1e-18
#define SWAP_PP(x,y) {T tmp=y; y=x; x=tmp;}
#define SWAP_PN(x,y) {T tmp=y; y=-x; x=tmp;}
#define SWAP_NP(x,y) {T tmp=y; y=x; x=-tmp;}
#define SWAP_NN(x,y) {T tmp=y; y=-x; x=-tmp;}


// Works on little-endian machines only
inline bool is_nan(T& value ) {
//	if (((*(uint32_t *) &value) & 0x7fffffff) > 0x7f800000) {
//		return true;
//	}
	if (value == 0.0) return false;
	if (fabs(value) > BIG) return true;
	if (fabs(value) < SMALL) return true;
	return false;
}

inline T sanitize_denormal(T value) {
	if (is_nan(value)) {
		fprintf(stderr,"Broken number ( %e )\n",value);
		if (fabs(value) > BIG) {return BIG;}
		if (fabs(value) < SMALL) {return SMALL;}
	}
	return value;
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
	if (v.ag == vg) {
		fprintf(stderr,"Broken beta\n");
		vg = v.r0g/v.r0k*(v.ak-v.vk)+v.ag;
	}
	T x = (powf(fabs(-1.0/v.D*(v.r0g/v.r0k*(((v.ak-v.vk)/(v.ag-vg))+1.0))),1.0/v.K));
	//if (v.ag == vg || v.ak == v.vk) 
	//	x = (powf(fabs(-1.0/v.D*(v.r0g/v.r0k)),1.0/v.K));
	x = sanitize_denormal(x);
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
	T tolerance = 1e-8;
	T vkn = 0.0;
	for (int i = 0; i < 50; ++i) {
		vkn = *i1 - f8(v,*i1)*(*i1-*i2)/(f8(v,*i1)-f8(v,*i2));
		*i1 = *i2;
		*i2 = vkn;
		if (sanitize_denormal(fabs(f8(v,vkn))) < tolerance) break;
		if (vkn < v.ak)
			break;
			//vkn = v.ak;
		if (vkn < (v.ag-v.vg)*v.r0k/v.r0g+v.ak) 
			break;
			//vkn = (v.ag-v.vg)*v.r0k/v.r0g+v.ak;
	} 
	//printf("%f\n",vkn);
	return vkn;
}

T secantf10(Valve v, T *i1, T *i2) {
	T tolerance = 1e-8;
	T vkn = 0.0;
 	for (int i = 0; i<50; ++i) {
		vkn = *i1 - f10(v,*i1)*(*i1-*i2)/(f10(v,*i1)-f10(v,*i2));
		*i1 = *i2;
		*i2 = vkn;
		if (sanitize_denormal(fabs(f10(v,vkn))) < tolerance) break;
		if (vkn < v.ak) 
			break;
			//vkn = v.ak;
		if (vkn < (v.ag-v.vg)*v.r0k/v.r0g+v.ak) 
			break;
			//vkn = (v.ag-v.vg)*v.r0k/v.r0g+v.ak;
	}
	return vkn;
}

T secantf12(Valve v, T *i1, T *i2) {
	T tolerance = 1e-8;
	T vgn = 0.0;
 	for (int i = 0; i<50; ++i) {
		vgn = *i1 - f12(v,*i1)*(*i1-*i2)/(f12(v,*i1)-f12(v,*i2));
		*i1 = *i2;
		*i2 = vgn;
		if (sanitize_denormal(fabs(f12(v,vgn))) < tolerance) break;
		if (vgn > v.ag) 
			break;
			//vgn = v.ag;
		if (vgn < (v.ak-v.vk)*v.r0g/v.r0k+v.ag) 
			break;
			//vgn = (v.ak-v.vk)*v.r0g/v.r0k+v.ag;
	}
	return vgn;
}


/////////////////////////////////// Second valve model

typedef struct valve2 {
	T r0g; T ag; T bg; T vg;
	T r0k; T ak; T bk; T vk;
	T r0p; T ap; T bp; T vp;
	T g, mu, gamma, c, gg, e, cg, ig0;
} Valve2;

T fg(Valve2 v, T vg) {
	return (v.ag-v.r0g*v.gg*powf(log(1.0+exp(v.cg*vg))/v.cg,v.e)+v.ig0-vg);
}

T fp(Valve2 v, T vp) {
	return (-v.r0p*(v.g*powf(log(1.0+exp(v.c*(vp/v.mu+v.vg)))/v.c,v.gamma))+v.ap-vp);
}

T fk(Valve2 v) {
	return (v.ak + v.r0k*((v.vp-v.ap)/v.r0p+(v.vg-v.ag)/v.r0g));
}

T secantfg(Valve2 v, T *i1, T *i2) {
        T tolerance = 1e-8;
        T vgn = 0.0;
        for (int i = 0; i<50; ++i) {
                vgn = *i1 - fg(v,*i1)*(*i1-*i2)/(fg(v,*i1)-fg(v,*i2));
                *i1 = *i2;
                *i2 = vgn;
                if (sanitize_denormal(fabs(fg(v,vgn))) < tolerance) break;
        }
        return vgn;
}


T secantfp(Valve2 v, T *i1, T *i2) {
        T tolerance = 1e-8;
        T vpn = 0.0;
        for (int i = 0; i<50; ++i) {
                vpn = *i1 - fp(v,*i1)*(*i1-*i2)/(fp(v,*i1)-fp(v,*i2));
                *i1 = *i2;
                *i2 = vpn;
                if (sanitize_denormal(fabs(fp(v,vpn))) < tolerance) break;
        }
        return vpn;
}

int main(){ 
	T Fs = 48000.0;
	T N = Fs/2.0;
	T gain = 2.0;
	T f0 = 1000.0;
	T input[38400] = { 0.0 };
	T output[38400] = { 0.0 };
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
	T wmax = 20.0;

	V Vi = V(0.0,1000.0);
	C Ci = C(ci, Fs);
	C Ck = C(ck, Fs);
	C Co = C(co, Fs);
	R Ro = R(ro);
	R Rg = R(rg);
	R Ri = R(ri);
	R Rk = R(rk);
	V E = V(e, rp);

#if 0
//Mod
	ser S0 = ser(&Ci, &Vi);
	inv I0 = inv(&S0);
	inv I5 = inv(&Ri);
	par P0 = par(&I0, &I5);
	ser S1 = ser(&Rg, &P0);
	inv I1 = inv(&S1);

	par I3 = par(&Ck, &Rk);

	ser S2 = ser(&Co, &Ro);
	inv I4 = inv(&S2);
	inv EE = inv(&E);
	par P2 = par(&I4, &E);
	//inv P2 = inv(&I2);
#else
//Official
	ser S0 = ser(&Ci, &Vi);
	inv I0 = inv(&S0);
	par P0 = par(&I0, &Ri);
	ser S1 = ser(&Rg, &P0);
	inv I1 = inv(&S1);

	par I3 = par(&Ck, &Rk);
//	inv I3 = inv(&S3);

	ser S2 = ser(&Co, &Ro);
	inv I4 = inv(&S2);
	par P2 = par(&I4, &E);
#endif	
/*
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
*/
	// 12AX7 triode model RSD-1
	Valve2 v;
	v.g = 2.242e-3;
	v.mu = 103.2;
	v.gamma = 1.26;
	v.c = 3.4;
	v.gg = 6.177e-4;
	v.e = 1.314;
	v.cg = 9.901;
	v.ig0 = 8.025e-8;

	I1.waveUp();
	I3.waveUp();
	I1.WD = 0.0;
	I3.WD = 0.0;
	P2.WD = 0.0;
	
	DUMP(printf("0j\t  Vi\t  Ro\t  Vg\t  Vk\t  Vp\t  Ri\t  Rk\t  Rg\t  E\t  Co\t  Ck\t  EA\t  RoA\t  Ig\t  Ik\t  Ip\n"));
	
	for (int j = 0; j < N; ++j) {
		Vi.e = input[j];
		I1.waveUp();
		I3.waveUp();
		P2.waveUp();
		v.ag = I1.WU;
		v.ak = I3.WU;
		v.ap = P2.WU;
		v.r0g = I1.PortRes;
		v.r0k = I3.PortRes;
		v.r0p = P2.PortRes;
		//Step 1: read input sample as voltage for the source

		//Step 2: propagate waves up to the 3 roots


		//Step 3: compute wave reflections at non-linearity

		T vg0, vg1, vp0, vp1;

		vg0 = v.ag;
		vg1 = v.ag + fg(v, v.ag);
		v.vg = secantfg(v, &vg0, &vg1);
		
		vp0 = v.ap;
		vp1 = v.ap + fp(v, v.ap);
		v.vp = secantfp(v, &vp0, &vp1);

		v.vk = fk(v);

		v.bg = 2.0*v.vg-v.ag;
		v.bp = 2.0*v.vp-v.ap;
		v.bk = 2.0*v.vk-v.ak;
		
		I1.setWD(v.bg);
		P2.setWD(v.bp);	
		I3.setWD(v.bk);
		
		printf("%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t: %.4f\t%.4f\t%.4f a:%.2f: %.2f b:%.2f: %.2f\n",j/Fs, input[j], Ro.Voltage(), I1.Voltage(),I3.Voltage(),P2.Voltage(),Ri.Voltage(),Rk.Voltage(),Rg.Voltage(),E.Voltage(),Co.Voltage(), Ck.Voltage(), E.Current(), Ro.Current(), I1.Current(),I3.Current(),P2.Current(),v.ak,I3.WU, v.bk,I3.WD);
		printf("1%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n", j/Fs, input[j], Ro.Voltage(), v.vg,v.vk,v.vp,Ri.Voltage(),Rk.Voltage(),Rg.Voltage(),E.Voltage(),Co.Voltage(), Ck.Voltage(), E.Current(), Ro.Current(), I1.Current(),I3.Current(),P2.Current());
	}
}

