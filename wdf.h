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
	Adaptor *l;
	Adaptor *r;
	Adaptor(Adaptor *a);
	template <class Port1, class Port2>Adaptor(Port1 *a, Port2 *b);
};

class Serial : public Adaptor {
public:
	template <class Port1, class Port2>Serial(Port1 a, Port2 b);
	T waveUp();
	void setWD(T waveparent);
};

class OnePort : public Adaptor {
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

