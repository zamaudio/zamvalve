/*
tubestage.cpp  zamvalve dsp
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

#include <stdlib.h>
#include "wdf.h"
#include "tubestage.h"

#define TOLERANCE 1e-8

#ifdef __cplusplus
extern "C" {
#endif

float tubestage_run(void* circuit, float input, float tubedrive) {
	return (((Circuit *)circuit)->tubestage(input, tubedrive));
}

#ifdef __cplusplus
}
#endif

Circuit::Circuit() : 
		Vi(0.0,10000.0), Ci(0.0000001,48000.0), Ck(0.00001,48000.0), Co(0.00000001,48000.0), Ro(1000000.0), Rg(20000.0), 
		Ri(1000000.0), Rk(1000.0), E(250.0,100000.0), 
		S0(&Ci,&Vi), I0(&S0), P0(&I0,&Ri), S1(&Rg,&P0), 
		I1(&S1), I3(&Ck,&Rk), S2(&Co,&Ro), I4(&S2), P2(&I4,&E) {
	
	// 12AX7 triode model RSD-1
	v.g = 2.242e-3;
	v.mu = 103.2;
	v.gamma = 1.26;
	v.c = 3.4;
	v.gg = 6.177e-4;
	v.e = 1.314;
	v.cg = 9.901;
	v.ig0 = 8.025e-8;
}


float Circuit::tubestage(float input, float tubedrive) { 

	float output;

	//Step 1: read input sample as voltage for the source
	Vi.e = tubedrive*input;

	//Step 2: propagate waves up to the triode and push values into triode element
	//((Class*)v)->foo()
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

	//Step 4: output 
	output = (float) Ro.Voltage()/200.0; //Rescale output voltage to be within digital limits

	//Step 5: push new waves down from the triode element
	P2.setWD(v.P.WU); 
	I1.setWD(v.G.WU);
	I3.setWD(v.K.WU);
	
	return output;
}

