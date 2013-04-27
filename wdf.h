using namespace std;

typedef double T;

class WDF {
public:
	T wD;
	T wU;
	T rp;
	WDF();
	virtual T voltage() {}
	virtual T waveUp();
	virtual void setWD(T val);
	void setWU(T val);
	void setRp(T r);
	T getRp();
	void dump();
};

class Adaptor : public WDF {
public:
	WDF *l;
	WDF *r;
	Adaptor(WDF *a, WDF *b);
};

class Serial : public Adaptor {
public:
	Serial(WDF a, WDF b);
	T waveUp();
	void setWD(T waveparent);
};

class OnePort : public WDF {
public:
	OnePort(T r);
	T state;
	void setWD(T val);
};

class R : public OnePort {
public:
	R(T r);
	T waveUp();
	T voltage();
};

class C : public OnePort {
public:
	C(T r);
	T waveUp();
};

class V : public OnePort {
public:
	T e;
	V(T ee, T r);
	T waveUp();
};

