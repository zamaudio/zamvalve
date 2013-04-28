typedef double T;

class WDF {
public:
	T WD;
	T WU;
	T PortRes;
	WDF();
	T Voltage();
	T state;
	T waveUp() {}
	void setWD(T waveparent);
};

class OnePort : public WDF {
public:
	T WD;
	T WU;
	void setWD(T val);
};

class Adaptor : public WDF {
public:
	WDF *left;
	WDF *right;
};

class ser : public Adaptor {
public:
	T WD;
	T WU;
	ser(Adaptor *left, Adaptor *right);
	T waveUp();
	void setWD(T waveparent);	
};

class R : public OnePort {
public:
	R(T res);
	T waveUp();
};


WDF::WDF() {}

T WDF::Voltage() {
	T Volts = (WU + WD) / 2.0;
	return Volts;
}

ser::ser(Adaptor *l, Adaptor *r) {
	left = l;
	right = r;
	PortRes = l->PortRes + r->PortRes;
}

T ser::waveUp() {
	WU = -left->waveUp() - right->waveUp();
	return WU;
}


void ser::setWD(T waveparent) {
	WD = waveparent;
	left->setWD(left->WU-(left->PortRes/PortRes)*(waveparent+left->WU+right->WU));
	right->setWD(right->WU-(right->PortRes/PortRes)*(waveparent+left->WU+right->WU));
}

void WDF::setWD(T val) {
	WD = val;
	state = val;
}

R::R(T res) {
	PortRes = res;
}

T R::waveUp() {
	WU = 0.0;
	WU = WU;
}





