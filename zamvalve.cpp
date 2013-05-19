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

#include <lv2plugin.hpp>
#include <stdint.h>
#include <math.h>


using namespace LV2;

#define TOLERANCE 5e-6
//1e-8

#include "wdf.h"

class ZamValve : public Plugin<ZamValve> { 
public:
	Triode v;
	V Vi;
	C Ci;
	C Ck;
	C Co;
	R Ro;
	R Rg;
	R Ri;
	R Rk;
	V E;

	T e;

	//Official
	//->Gate
	ser S0;
	inv I0;
	par P0;
	ser S1;
	inv I1;

	//->Cathode
	par I3;

	//->Plate
	ser S2;
	inv I4;
	par P2;

	ZamValve(double rate) : Plugin<ZamValve>(4), 
		Vi(0.0,10000.0), Ci(0.0000001,48000.0), Ck(0.00001,48000.0), Co(0.00000001,48000.0), Ro(1000000.0), Rg(20000.0), 
		Ri(1000000.0), Rk(1000.0), E(250.0,100000.0), 
		S0(&Ci,&Vi), I0(&S0), P0(&I0,&Ri), S1(&Rg,&P0), 
		I1(&S1), I3(&Ck,&Rk), S2(&Co,&Ro), I4(&S2), P2(&I4,&E) {

		T Fs = rate;
		
		// Passive components
		T ci = 0.0000001;	//100nF
		T ck = 0.00001;		//10uF
		T co = 0.00000001;	//10nF
		T ro = 1000000.0;	//1Mohm
		T rp = 100000.0;	//100kohm
		T rg = 20000.0;		//20kohm
		T ri = 1000000.0;	//1Mohm
		T rk = 1000.0;		//1kohm
		e = 250.0;		//250V

		Vi = V(0.0,10000.0);	//1kohm internal resistance
		Ci = C(ci, Fs);
		Ck = C(ck, Fs);
		Co = C(co, Fs);
		Ro = R(ro);
		Rg = R(rg);
		Ri = R(ri);
		Rk = R(rk);
		E = V(e, rp);

		//Official
		//->Gate
		S0 = ser(&Ci, &Vi);
		I0 = inv(&S0);
		P0 = par(&I0, &Ri);
		S1 = ser(&Rg, &P0);
		I1 = inv(&S1);

		//->Cathode
		I3 = par(&Ck, &Rk);

		//->Plate
		S2 = ser(&Co, &Ro);
		I4 = inv(&S2);
		P2 = par(&I4, &E);

		// 12AX7 triode model RSD-1
		v.g1 = 2.242e-3;
		v.mu1 = 103.2;
		v.gamma1 = 1.26;
		v.c1 = 3.4;
		v.gg1 = 6.177e-4;
		v.e1 = 1.314;
		v.cg1 = 9.901;
		v.ig01 = 8.025e-8;

		// 12AX7 triode model EHX-1
		v.g2 = 1.371e-3;
		v.mu2 = 86.9;
		v.gamma2 = 1.349;
		v.c2 = 4.56;
		v.gg2 = 3.263e-4;
		v.e2 = 1.156;
		v.cg2 = 11.99;
		v.ig02 = 3.917e-8;
	}

	void run(uint32_t nframes) {
		
		T tubedrive = *p(2);
		T tubetone = *p(3);
		
		//12AX7 triode tube mod
		v.g = v.g1 + (v.g2-v.g1)*tubetone;
		v.mu = v.mu1 + (v.mu2-v.mu1)*tubetone;
		v.gamma = v.gamma1 + (v.gamma2-v.gamma1)*tubetone;
		v.c = v.c1 + (v.c2-v.c1)*tubetone;
		v.gg = v.gg1 + (v.gg2-v.gg1)*tubetone;
		v.e = v.e1 + (v.e2-v.e1)*tubetone;
		v.cg = v.cg1 + (v.cg2-v.cg1)*tubetone;
		v.ig0 = v.ig01 + (v.ig02-v.ig01)*tubetone;

		for (uint32_t i = 0; i < nframes; ++i) {

			//Step 1: read input sample as voltage for the source
			Vi.e = p(0)[i]*tubedrive;

			//Step 2: propagate waves up to the triode and push values into triode element
			I1.waveUp();
			I3.waveUp();
			P2.waveUp();
			v.G.WD = I1.WU;
			v.K.WD = I3.WU; 
			v.P.WD = P2.WU;
			v.vg = v.G.WD;
			v.vk = v.K.WD;
			v.vp = v.P.WD;
			v.G.PortRes = I1.PortRes;
			v.K.PortRes = I3.PortRes;
			v.P.PortRes = P2.PortRes;

			//Step 3: compute wave reflections inside the triode
			T vg0, vg1, vp0, vp1;

			vg0 = -10.0;
			vg1 = 10.0;
			v.vg = v.zeroffg(vg0,vg1,TOLERANCE);

			vp0 = e;
			vp1 = 0.0;
			v.vp = v.zeroffp(vp0,vp1,TOLERANCE);

			v.vk = v.ffk();

			v.G.WU = 2.0*v.vg-v.G.WD;
			v.K.WU = 2.0*v.vk-v.K.WD;
			v.P.WU = 2.0*v.vp-v.P.WD;
			
			p(1)[i] = p(0)[i];
			p(1)[i] = Ro.Voltage()/e; 

			P2.setWD(v.P.WU); 
			I1.setWD(v.G.WU);
			I3.setWD(v.K.WU);
		}
	}
};

static int _ = ZamValve::register_class("http://zamaudio.com/lv2/zamvalve");
