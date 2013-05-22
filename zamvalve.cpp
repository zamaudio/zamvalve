/*
zamvalve.cpp
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

typedef double T;

class ZamValve : public Plugin<ZamValve> { 
public:
	ZamValve(double rate) : Plugin<ZamValve>(4) {

	}

	void run(uint32_t nframes) {
		
		T drive = *p(2);
		T tone = *p(3);
		
		for (uint32_t i = 0; i < nframes; ++i) {

			T tape   = (tone < 0.5) ? 0.0 : 0.25/0.5*tone - 0.25;
			T valve  = 0.25/0.5*fabs(tone - 0.5);

			T shape1 = tape/drive;
                        T shape2 = valve/drive;

                        T asig   = p(0)[i];
                        T ax1    = asig*drive*(1.0+shape1);
                        T ax2    = -asig*drive*(1.0+shape2);
                        T ax3    = asig*drive;
                        T aout   = (exp(ax1)-exp(ax2))/(exp(ax3)+exp(-ax3))/((1.0+shape2)*1.3);
	
			p(1)[i]  = p(0)[i];
			p(1)[i]  = aout;

//			p(1)[i]  = p(0)[i];
//			p(1)[i]  = 2.0*p(0)[i]*(1.0-fabs(p(0)[i])*0.5); //other saturation algorithm
		}
	}
};

static int _ = ZamValve::register_class("http://zamaudio.com/lv2/zamvalve");
