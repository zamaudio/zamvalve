declare name "ZamValve";
declare author "Damien Zammit";
declare copyright "2013";
declare version "2.1";
declare license "GPLv2";

import("math.lib");
import("filter.lib");

//db2linear(x) = pow(10.0, x/20.0);

tone = hslider("Tube/Tape blend", 0.0, 0.0, 1.0, 0.1) : smooth(0.99);
drive = hslider("Drive (dB)", 0.0, 0.0, 35.0, 0.1) : smooth(0.99) : db2linear;

saturate(x, D, T) = (exp(x1)-exp(x2))/(exp(x3)+exp(-x3))/((1.0 + shape2)*1.3)
	with {
		tape = (T < 0.5)*(0.5*T - 0.25);
		valve = 0.5*abs(T - 0.5);
		shape1 = tape/D;
		shape2 = valve/D;
		x3 = x*D;
		x1 = x3*(1.0 + shape1);
		x2 = -x3*(1.0 + shape2);
	};

process(x) = saturate(x, drive, tone);

