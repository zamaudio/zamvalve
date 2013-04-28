typedef double T;

class WDF {
public:
	T WD;
	T WU;
	T PortRes;
	WDF();
	T Voltage(WDF *a);
	T state;
	T waveUp();
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
	T waveUp(Adaptor *a);
	void setWD(ser *a, T waveparent);	
};

class OnePort : public WDF {
public:
	T WD;
	T WU;
	void setWD(WDF *a, T val);
};

class R : public OnePort {
public:
	R(T res);
	T waveUp(OnePort *a);
};



T Voltage(WDF *a) {
	T Volts = (a->WU + a->WD) / 2.0;
	return Volts;
}

ser::ser(Adaptor *l, Adaptor *r) {
	left = l;
	right = r;
	PortRes = l->PortRes + r->PortRes;
}

T ser::waveUp(Adaptor *a) {
	WU = -waveUp(a->left) - waveUp(a->right);
	a->WU = WU;
	return WU;
}

void ser::setWD(ser *a, T waveparent) {
	a->WD = waveparent;
	a->left->setWD(a->left, a->left->WU-(a->left->PortRes/a->PortRes)*(waveparent+a->left->WU+a->right->WU));
	a->right->setWD(a->right, a->right->WU-(a->right->PortRes/a->PortRes)*(waveparent+a->left->WU+a->right->WU));
}

void OnePort::setWD(WDF *a, T val) {
	a->WD = val;
	a->state = val;
}

R::R(T res) {
	PortRes = res;
}

T R::waveUp(OnePort *a) {
	WU = 0.0;
	a->WU = WU;
}

