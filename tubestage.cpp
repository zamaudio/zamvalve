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

#include "wdf.h"
#include "tubestage.h"

#define TOLERANCE 1e-8

#ifdef __cplusplus
extern "C" {
#endif

float tubestage(float input, Circuit* c, float tubedrive) { 

	float output;
	
	Triode *t = &c->v;
	inv *gate = &c->I1;
	par *cathode = &c->I3;
	par *plate = &c->P2;
	R *out = &c->Ro;

	//Step 1: read input sample as voltage for the source
	c->Vi.e = tubedrive*input;

	//Step 2: propagate waves up to the triode and push values into triode element
	//((Class*)v)->foo()
	gate->waveUp();
	cathode->waveUp();
	plate->waveUp();
	c->v.G.WD = c->I1.WU;
	c->v.K.WD = c->I3.WU; 
	c->v.P.WD = c->P2.WU;
	c->v.vg = c->v.G.WD;
	c->v.vk = c->v.K.WD;
	c->v.vp = c->v.P.WD;
	c->v.G.PortRes = c->I1.PortRes;
	c->v.K.PortRes = c->I3.PortRes;
	c->v.P.PortRes = c->P2.PortRes;

	//Step 3: compute wave reflections inside the triode
	T vg0, vg1, vp0, vp1;

	vg0 = -10.0;
	vg1 = 10.0;
	c->v.vg = t->zeroffg(vg0,vg1,TOLERANCE);

	vp0 = c->e;
	vp1 = 0.0;
	c->v.vp = t->zeroffp(vp0,vp1,TOLERANCE);

	c->v.vk = t->ffk();

	c->v.G.WU = 2.0*c->v.vg-c->v.G.WD;
	c->v.K.WU = 2.0*c->v.vk-c->v.K.WD;
	c->v.P.WU = 2.0*c->v.vp-c->v.P.WD;

	//Step 4: output 
	output = (float) out->Voltage()/200.0; //Rescale output voltage to be within digital limits

	//Step 5: push new waves down from the triode element
	plate->setWD(c->v.P.WU); 
	gate->setWD(c->v.G.WU);
	cathode->setWD(c->v.K.WU);
	
	return output;
}

#ifdef __cplusplus
}
#endif

