#include <math.h>
#include <stdlib.h>

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "wdf.h"
#include "tubestage.h"

#define ZAMVALVE_URI "http://zamaudio.com/lv2/zamvalve"

typedef enum {
	ZAMVALVE_INPUT = 0,
	ZAMVALVE_OUTPUT = 1,
	ZAMVALVE_TUBEDRIVE = 2
} PortIndex;

typedef struct {
	float* input;
	float* output;
	float* tubedrive;
	float samplerate;
	Circuit c;
} ZAMVALVE;

static LV2_Handle
instantiate(const LV2_Descriptor* descriptor,
            double rate,
            const char* bundle_path,
            const LV2_Feature* const* features)
{
	ZAMVALVE* zamvalve = (ZAMVALVE*)malloc(sizeof(ZAMVALVE));
	zamvalve->samplerate = rate;
	T Fs = rate;

	// Passive components
	T ci = 0.0000001;       //100nF
	T ck = 0.00001;         //10uF
	T co = 0.00000001;      //10nF
	T ro = 1000000.0;       //1Mohm
	T rp = 100000.0;        //100kohm
	T rg = 20000.0;         //20kohm
	T ri = 1000000.0;       //1Mohm
	T rk = 1000.0;          //1kohm
	zamvalve->c.e = 250.0;  //250V

	zamvalve->c.Vi = V(0.0,10000.0);  //10kohm internal resistance
	zamvalve->c.Ci = C(ci, Fs);
	zamvalve->c.Ck = C(ck, Fs);
	zamvalve->c.Co = C(co, Fs);
	zamvalve->c.Ro = R(ro);
	zamvalve->c.Rg = R(rg);
	zamvalve->c.Ri = R(ri);
	zamvalve->c.Rk = R(rk);
	zamvalve->c.E = V(zamvalve->c.e, rp);

	//Circuit description
	//->Gate
	zamvalve->c.S0 = ser(&zamvalve->c.Ci, &zamvalve->c.Vi);
	zamvalve->c.I0 = inv(&zamvalve->c.S0);
	zamvalve->c.P0 = par(&zamvalve->c.I0, &zamvalve->c.Ri);
	zamvalve->c.S1 = ser(&zamvalve->c.Rg, &zamvalve->c.P0);
	zamvalve->c.I1 = inv(&zamvalve->c.S1);

	//->Cathode
	zamvalve->c.I3 = par(&zamvalve->c.Ck, &zamvalve->c.Rk);

	//->Plate
	zamvalve->c.S2 = ser(&zamvalve->c.Co, &zamvalve->c.Ro);
	zamvalve->c.I4 = inv(&zamvalve->c.S2);
	zamvalve->c.P2 = par(&zamvalve->c.I4, &zamvalve->c.E);  
	
	// 12AX7 triode model RSD-1
	zamvalve->c.v.g = 2.242e-3;
	zamvalve->c.v.mu = 103.2;
	zamvalve->c.v.gamma = 1.26;
	zamvalve->c.v.c = 3.4;
	zamvalve->c.v.gg = 6.177e-4;
	zamvalve->c.v.e = 1.314;
	zamvalve->c.v.cg = 9.901;
	zamvalve->c.v.ig0 = 8.025e-8;

	return (LV2_Handle)zamvalve;
}

static void
connect_port(LV2_Handle instance,
             uint32_t port,
             void* data)
{
	ZAMVALVE* zamvalve = (ZAMVALVE*)instance;
  
	switch ((PortIndex)port) {
	case ZAMVALVE_INPUT:
		zamvalve->input = (float*)data;
  	break;
	case ZAMVALVE_OUTPUT:
		zamvalve->output = (float*)data;
  	break;
	case ZAMVALVE_TUBEDRIVE:
		zamvalve->tubedrive = (float*)data;
	break;
	}
  
}

static inline float
from_dB(float gdb) {
	return (exp(gdb/20.f*log(10.f)));
};

static inline float
to_dB(float g) {
	return (20.f*log10(g));
}


static void
activate(LV2_Handle instance)
{
}


static void
run(LV2_Handle instance, uint32_t n_samples)
{
	ZAMVALVE* zamvalve = (ZAMVALVE*)instance;
  
	const float* const input = zamvalve->input;
	float* const output = zamvalve->output;
  
	float tubedrive = *(zamvalve->tubedrive);
	Circuit* c = &zamvalve->c;
 
	for (uint32_t i = 0; i < n_samples; ++i) {
		output[i] = input[i];
		output[i] = tubestage(input[i], c, tubedrive);
	}
  
}


static void
deactivate(LV2_Handle instance)
{
}

static void
cleanup(LV2_Handle instance)
{
	free(instance);
}

const void*
extension_data(const char* uri)
{
	return NULL;
}

static const LV2_Descriptor descriptor = {
	ZAMVALVE_URI,
	instantiate,
	connect_port,
	activate,
	run,
	deactivate,
	cleanup,
	extension_data
};

LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	switch (index) {
	case 0:
		return &descriptor;
	default:
		return NULL;
	}
}
