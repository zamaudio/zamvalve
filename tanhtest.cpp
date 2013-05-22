/*
Test classes for zamvalve
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
#include <math.h>

/////////////////////////////////////////////

int main(){ 
	T Fs = 48000.0;
	T N = Fs/3;
	T f0 = 1001.0;
	T input[48000] = { 0.0 };
	int i;
	for (i = 0; i < N; ++i) {
		input[i] = sin(2.0*M_PI*f0/Fs*i);
	}
	
	for (int j = 0; j < N; ++j) {
		//Step 1: read input sample as voltage for the source

		//Step 2: propagate waves up to the triode and push values into triode element
                        T tubedrive = 300.0;
			T tubetone = 1.0;

			T tape = (tubetone < 0.5) ? 0.0 : 0.25/0.5*tubetone - 0.25;
                        T valve = 0.25/0.5*fabs(tubetone - 0.5);

                        T shape1 = tape/tubedrive;
                        T shape2 = valve/tubedrive;

                        T asig   =       input[j];
                        T ax1    =       asig*tubedrive*(1.0+shape1);
                        T ax2    =       -asig*tubedrive*(1.0+shape2);
                        T ax3    =       asig*tubedrive;
                        T aout   =       (exp(ax1)-exp(ax2))/(exp(ax3)+exp(-ax3))/((1.0+shape2)*1.3);
        
			printf("%f\n",aout);
	}
}

