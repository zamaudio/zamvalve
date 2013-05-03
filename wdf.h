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

#define ONEPORT 0
#define PASSTHROUGH 1
#define THREEPORT 2

#if 1
#define DUMP(x) x
#else
#define DUMP(x)
#endif

typedef double T;

class WDF {
public:
	T WD;
	T WU;
	T PortRes;
	WDF();
	T Voltage();
	T Current();
	T state;
	char type;

	virtual T waveUp() = 0;
	virtual void setWD(T waveparent);
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

void WDF::setWD(T val) {
	WD = val;
	state = val;
	DUMP(printf("DOWN\tWDF\t%c\tWD=%f\tWU=%f\tV=%f\n",type,WD,WU,(WD+WU)/2.0));	
}

void OnePort::setWD(T val) {
	WD = val;
	state = val;
	DUMP(printf("DOWN\tOneport\t%c\tWD=%f\tWU=%f\tV=%f\n",type,WD,WU,(WD+WU)/2.0));	
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
	type = 'S';
}

template <class Port>inv::inv(Port *l) : Adaptor(PASSTHROUGH) {
	left = l;
	PortRes = l->PortRes;
	type = 'I';
}

T ser::waveUp() {
	//Adaptor::WU = -left->waveUp() - right->waveUp();
	WDF::WU = -left->waveUp() - right->waveUp();
	DUMP(printf("UP\tser\tWU=%f\tWD=%f\tV=%f\n",WU,WD,(WD+WU)/2.0));
	return WU;
}

template <class Port1, class Port2>par::par(Port1 *l, Port2 *r) : Adaptor(THREEPORT) {
	left = l;
	right = r;
	PortRes = 1.0 / (1.0 / l->PortRes + 1.0 / r->PortRes);
	type = 'P';
}

T par::waveUp() {
	T G23 = 1.0 / left->PortRes + 1.0 / right->PortRes;
	//Adaptor::WU = (1.0 / left->PortRes)/G23*left->waveUp() + (1.0 / right->PortRes)/G23*right->waveUp();
	WDF::WU = (1.0 / left->PortRes)/G23*left->waveUp() + (1.0 / right->PortRes)/G23*right->waveUp();
	DUMP(printf("UP\tpar\tWU=%f\tWD=%f\tV=%f\n",WU,WD,(WD+WU)/2.0));
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
	DUMP(printf("SER WP=%f\n",waveparent));
	left->setWD(left->WU-(2.0*left->PortRes/(PortRes+left->PortRes+right->PortRes))*(waveparent+left->WU+right->WU));
	right->setWD(right->WU-(2.0*right->PortRes/(PortRes+left->PortRes+right->PortRes))*(waveparent+left->WU+right->WU));
}

void par::setWD(T waveparent) {
	Adaptor::setWD(waveparent);
	DUMP(printf("PAR WP=%f\n",waveparent));
	T p = 2.0*(waveparent/PortRes + left->WU/left->PortRes + right->WU/right->PortRes)/(1.0/PortRes + 1.0/left->PortRes + 1.0/right->PortRes);

	left->setWD((p - left->WU));
	right->setWD((p - right->WU));
}

T inv::waveUp() {
	///////////WD = -left->WD;
	WU = -left->waveUp(); 	//-
	DUMP(printf("UP\tinv\tWU=%f\tWD=%f\tV=%f\n",WU,WD,(WD+WU)/2.0));
	return WU;
}

void inv::setWD(T waveparent) {
	WDF::setWD(waveparent);
	DUMP(printf("INV WP=%f\n",waveparent));
	//left->WD = -waveparent;		//-
	///////////left->WU = -WU;
	left->setWD(-waveparent);	//-
	
}

R::R(T res) : Adaptor(ONEPORT) {
	PortRes = res;
	type = 'R';
}

T R::waveUp() {
	WU = 0.0;
	DUMP(printf("UP\tR\tWU=%f\tWD=%f\tV=%f\n",WU, WD,(WD+WU)/2.0));
	return WU;
}

C::C(T c, T fs) : Adaptor(ONEPORT) {
	PortRes = 1.0/(2.0*c*fs);
	state = 0.0;
	type = 'C';
}

T C::waveUp() {
	WU = state;
	DUMP(printf("UP\tC\tWU=%f\tWD=%f\tV=%f\n",WU,WD,(WD+WU)/2.0));
	return WU;
}

V::V(T ee, T r) : Adaptor(ONEPORT) {
	e = ee;
	PortRes = r;
	WD = 0.0;  //always?
	type = 'V';
}

T V::waveUp() {
	WU = 2.0*e - WD;
	DUMP(printf("UP\tV\tWU=%f\tWD=%f\tV=%f\n",WU, WD,(WD+WU)/2.0));
	return WU;
}

