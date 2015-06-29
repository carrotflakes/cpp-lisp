#include <iostream>
#include <sstream>
#include <string>
#include <memory>
#include <vector>
#include <map>
#include <fstream>
#include <ctime>
#include "mem.hpp"
#include "fmap.hpp"

#define GC_DEBUG true
#define TCO true

struct Env;

struct Lobj : public MemObject {
	virtual ~Lobj() {}

	template<typename T> bool typep() const {
		return typeid(*this) == typeid(T);
	}
	template<typename T> T &getAs() {
		return *static_cast<T*>(this);
	}
	template<typename T> const T &getAs() const {
		return *static_cast<const T*>(this);
	}

	virtual void print(std::ostream &os) const = 0;
	virtual void readablePrint(std::ostream &os) const = 0;
	virtual bool eq(Lobj *obj) const;
	bool isNil() const;
	void markTree();
};

typedef Lobj* LobjSPtr;
typedef Lobj* LobjWPtr;
typedef Env* EnvSPtr;
typedef Env* EnvWPtr;

struct Cons : public Lobj {
	LobjSPtr car;
	LobjSPtr cdr;

	Cons(LobjSPtr a, LobjSPtr d)
	: car(a), cdr(d) {}

	void print(std::ostream &os) const;
	void readablePrint(std::ostream &os) const;
	void markTree();
	static void *operator new(size_t size);
};

struct Symbol : public Lobj {
	const std::string name;

	Symbol(const std::string n)
	: name(n) {}

	void print(std::ostream &os) const;
	void readablePrint(std::ostream &os) const;
	static void *operator new(size_t size);
};

struct Int : public Lobj {
	int value;

	Int (int v)
	: value(v) {}

	void print(std::ostream &os) const;
	void readablePrint(std::ostream &os) const;
	bool eq(Lobj *obj) const;
	static void *operator new(size_t size);
};

struct String : public Lobj {
	std::string value;

	String (const std::string &v)
	: value(v) {}

	void print(std::ostream &os) const;
	void readablePrint(std::ostream &os) const;
	bool eq(Lobj *obj) const;
	static void *operator new(size_t size);
};

struct SpecialForm : public Lobj {
	std::string name;
	std::function<LobjSPtr(Env *, LobjSPtr, bool)> function;

	SpecialForm (const std::string &n, std::function<LobjSPtr(Env *, LobjSPtr, bool)> f)
	: name(n), function(f) {}

	void print(std::ostream &os) const;
	void readablePrint(std::ostream &os) const;
};

struct Proc : public Lobj {
	LobjSPtr parameterList;
	LobjSPtr body;
	EnvSPtr env;

	Proc (LobjSPtr pl, LobjSPtr b, EnvSPtr e)
	: parameterList(pl), body(b), env(e) {}

	void print(std::ostream &os) const;
	void readablePrint(std::ostream &os) const;
	void markTree();
	static void *operator new(size_t size);
};

struct BuiltinProc : public Lobj {
	std::function<LobjSPtr(Env &, std::vector<LobjSPtr> &)> function;

	BuiltinProc (std::function<LobjSPtr(Env &, std::vector<LobjSPtr> &)> f)
	: function(f) {}

	void print(std::ostream &os) const;
	void readablePrint(std::ostream &os) const;
};

struct Macro : public Lobj {
	LobjSPtr parameterList;
	LobjSPtr body;
	EnvSPtr env;

	Macro (LobjSPtr pl, LobjSPtr b, EnvSPtr e)
	: parameterList(pl), body(b), env(e) {}

	void print(std::ostream &os) const;
	void readablePrint(std::ostream &os) const;
	void markTree();
	static void *operator new(size_t size);
};


void Cons::print(std::ostream &os) const {
	os << "(";
	car->print(os);
	Lobj *o = cdr;
	while (1) {
		if (o->typep<Cons>()) {
			os << " ";
			Cons *cons = &o->getAs<Cons>();
			cons->car->print(os);
			o = cons->cdr;
		} else if (o->isNil()) {
			break;
		} else {
			os << " . ";
			o->print(os);
			break;
		}
	}
	os << ")";
}
void Symbol::print(std::ostream &os) const {
	os << name;
}
void Int::print(std::ostream &os) const {
	os << value;
}
void String::print(std::ostream &os) const {
	os << value;
}
void SpecialForm::print(std::ostream &os) const {
	os << "#<SpecialForm " << name << ">";
}
void Proc::print(std::ostream &os) const {
	os << "#<Proc>";
}
void BuiltinProc::print(std::ostream &os) const {
	os << "#<BuiltinProc>";
}
void Macro::print(std::ostream &os) const {
	os << "#<Macro>";
}

void Cons::readablePrint(std::ostream &os) const {
	os << "(";
	car->readablePrint(os);
	Lobj *o = cdr;
	while (1) {
		if (o->typep<Cons>()) {
			os << " ";
			Cons *cons = &o->getAs<Cons>();
			cons->car->readablePrint(os);
			o = cons->cdr;
		} else if (o->isNil()) {
			break;
		} else {
			os << " . ";
			o->readablePrint(os);
			break;
		}
	}
	os << ")";
}
void Symbol::readablePrint(std::ostream &os) const {
	os << name;
}
void Int::readablePrint(std::ostream &os) const {
	os << value;
}
void String::readablePrint(std::ostream &os) const {
	os << value; // TODO
}
void SpecialForm::readablePrint(std::ostream &os) const {
	os << "#<SpecialForm " << name << ">";
}
void Proc::readablePrint(std::ostream &os) const {
	os << "#<Proc>"; // TODO
}
void BuiltinProc::readablePrint(std::ostream &os) const {
	os << "#<BuiltinProc>"; // TODO
}
void Macro::readablePrint(std::ostream &os) const {
	os << "#<Macro>"; // TODO
}

bool Lobj::eq(Lobj *obj) const {
	return this == obj;
}
bool Int::eq(Lobj *obj) const {
	return obj->typep<Int>() && value == obj->getAs<Int>().value;
}
bool String::eq(Lobj *obj) const {
	return obj->typep<String>() && value == obj->getAs<String>().value;
}

bool Lobj::isNil() const {
	return this->typep<Symbol>() &&	this->getAs<Symbol>().name == "nil";
}

void unrootTree(LobjSPtr o) {
	o->unroot();
	if (o->typep<Cons>()) {
		if (o->getAs<Cons>().car != nullptr) unrootTree(o->getAs<Cons>().car);
		if (o->getAs<Cons>().cdr != nullptr) unrootTree(o->getAs<Cons>().cdr);
	}
}


LobjSPtr intern(std::string name);

bool isSymbolChar(const char c) {
	return c != '(' && c != ')' && c != ' ' &&
		c != '\t' && c != '\n' && c != '\r' && c != 0;
}

LobjSPtr readAux(std::istream &is);

LobjSPtr readList(std::istream &is) {
	is >> std::ws;
	if (is.eof()) throw "parse failed";
	char c = is.get();
	if (c == ')') {
		return intern("nil");
	} else if (c == '.') {
		RootPtr<Lobj> cdr(readAux(is));
		is >> std::ws;
		if (is.get() != ')') throw "parse failed";
		return cdr.get();
	} else {
		is.unget();
		RootPtr<Lobj> car(readAux(is));
		RootPtr<Lobj> cdr(readList(is));
		return new Cons(car.get(), cdr.get());
	}
}

LobjSPtr readString(std::istream &is) {
	char c = is.get();
	std::stringstream ss;
	while (c != '"') {
		if (c == '\\') {
			switch (c = is.get()) {
			case 'n': c = '\n'; break;
			case 'f': c = '\f'; break;
			case 'b': c = '\b'; break;
			case 'r': c = '\r'; break;
			case 't': c = '\t'; break;
			case '\'': c = '\''; break;
			case '\"': c = '\"'; break;
			case '\\': c = '\\'; break;
			case '\n': case '\r': c = 0; break;
			}
		}
		if (c != 0) ss << c;
		if (is.eof()) throw "parse failed";
		c = is.get();
	}
	return new String(ss.str());
}

void skipCommentOut(std::istream &is) {
	is >> std::ws;
	char c = is.peek();
	while (c == ';') {
		while (c != 0 && c != '\n' && c != '\r') c = is.get();
		is >> std::ws;
		c = is.peek();
	}
}

LobjSPtr readAux(std::istream &is) {
	skipCommentOut(is);
	if (is.eof()) throw "parse failed";
	char c = is.get();
	if (c == '(') {
		return readList(is);
	} else if (('0' <= c && c <= '9') ||
						 (c == '-' && ('0' <= is.peek() && is.peek() <= '9'))) {
		is.unget();
		int value;
		is >> value;
		return new Int(value);
	} else if (c == '"') {
		return readString(is);
	} else {
		char symbolName[512];
		int i = 0;
		while (isSymbolChar(c) && !is.eof()) {
			symbolName[i++] = c;
			c = is.get();
		}
		is.unget();
		if (i == 0) throw "parse fialed";
		symbolName[i] = 0;
		return intern(symbolName);
	}
}

LobjSPtr read(std::istream &is) {
	try {
		LobjSPtr o = readAux(is);
		//unrootTree(o);
		return o;
	} catch (char const *e) {
		return nullptr;
	}
}


int gensymId = 0;
std::map<std::string, LobjSPtr> symbolMap;
Env *rootEnv;

LobjSPtr intern(std::string name) {
	auto it = symbolMap.find(name);
	LobjSPtr objPtr;
	if (it == symbolMap.end()) {
		objPtr = new Symbol(name);
		objPtr->root();
		symbolMap[name] = objPtr;
	} else {
		objPtr = it->second;
	}
	return objPtr;
}

struct Env : public MemObject {
private:
	EnvSPtr outerEnv = nullptr;
	EnvSPtr lexEnv = nullptr;
	Fmap<Symbol*, LobjSPtr> symbolValueMap;
	bool closed = true;

public:
	Env();
	Env(EnvSPtr e, EnvSPtr l = nullptr)
		: outerEnv(e), lexEnv(l), closed(false) {}

	EnvSPtr resolveEnv(Symbol *symbol) {
		if (isSpecialVariable(symbol))
			return resolveEnvDyn(symbol);
		else
			return resolveEnvLex(symbol);
	}

	EnvSPtr resolveEnvDyn(Symbol *symbol) {
		if (symbolValueMap.count(symbol))
			return this;
		if (outerEnv != nullptr)
			return outerEnv->resolveEnvDyn(symbol);
		return nullptr;
	}

	EnvSPtr resolveEnvLex(Symbol *symbol) {
		if (symbolValueMap.count(symbol))
			return this;
		if (lexEnv != nullptr)
			return lexEnv->resolveEnvLex(symbol);
		if (outerEnv != nullptr)
			return outerEnv->resolveEnvLex(symbol);
		return nullptr;
	}

	LobjSPtr resolve(Symbol *symbol) {
		EnvSPtr env = resolveEnv(symbol);
		if (env == nullptr) return nullptr;
		return env->symbolValueMap[symbol];
	}

	void bind(LobjSPtr objPtr, Symbol *symbol) {
		symbolValueMap[symbol] = objPtr;
	}

	static bool isSpecialVariable(Symbol *symbol) {
		return rootEnv->symbolValueMap.count(symbol);
	}

	void merge(EnvSPtr env) {
		for (auto &kv : env->symbolValueMap) {
			symbolValueMap[kv.first] = kv.second;
		}
		if (env->lexEnv != nullptr)
			lexEnv = env->lexEnv;
	}

	void setLexEnv(EnvSPtr l) {
		if (l != nullptr)
			lexEnv = l;
	}

	bool isClosed() const {
		return closed;
	}

	//LobjSPtr macroexpand1(LobjSPtr objPtr);
	//LobjSPtr macroexpand(LobjSPtr objPtr);
	LobjSPtr macroexpandAll(LobjSPtr objPtr);

	//LobjSPtr apply(LobjSPtr op, LobjSPtr args);
	LobjSPtr eval(LobjSPtr objPtr, bool tail = false);

	Lobj *evalTop(LobjSPtr objPtr) {
		return eval(macroexpandAll(objPtr));
	}

	void repl();

	void markTree();
	static void *operator new(size_t size);

	void print() const {
		std::cout << "{";
		for(auto &kv : symbolValueMap) {
			kv.first->print(std::cout);
			std::cout << ":";
			kv.second->print(std::cout);
			std::cout << ",";
		}
		std::cout << "}";
	}

	void printAll(bool exceptRoot = false) const {
		if (exceptRoot && outerEnv == nullptr) {
			std::cout << "{...}";
			return;
		}
		std::cout << "{";
		for(auto &kv : symbolValueMap) {
			kv.first->print(std::cout);
			std::cout << ":";
			kv.second->print(std::cout);
			std::cout << ",";
		}
		if (lexEnv != nullptr) {
			std::cout << "#lex:";
			lexEnv->printAll(exceptRoot);
		}
		if (outerEnv != nullptr) {
			std::cout << "#outer:";
			outerEnv->printAll(exceptRoot);
		}
		std::cout << "}";
	}
};

// memory //////////////////////////////////////////////////

void Lobj::markTree() {
	mark();
}
void Cons::markTree() {
	if (isMarked()) return;
	mark();
	if (car != nullptr) car->markTree();
	if (cdr != nullptr) cdr->markTree();
}
void Proc::markTree() {
	if (isMarked()) return;
	mark();
	if (parameterList != nullptr) parameterList->markTree();
	if (body != nullptr) body->markTree();
	if (env != nullptr) env->markTree();
}
void Macro::markTree() {
	if (isMarked()) return;
	mark();
	if (parameterList != nullptr) parameterList->markTree();
	if (body != nullptr) body->markTree();
	if (env != nullptr) env->markTree();
}
void Env::markTree() {
	if (isMarked()) return;
	mark();
	if (outerEnv != nullptr) outerEnv->markTree();
	if (lexEnv != nullptr) lexEnv->markTree();
	for (auto &kv : symbolValueMap) {
		kv.second->markTree();
	}
}

size_t heapSize = 0;
size_t heapSizeMax = 128 * 1024;

MemPool<sizeof(Cons),        32> consPool;
MemPool<sizeof(Symbol),      32> symbolPool;
MemPool<sizeof(Int),         32> intPool;
MemPool<sizeof(String),      32> stringPool;
MemPool<sizeof(Proc),        32> procPool;
MemPool<sizeof(Macro),       32> macroPool;
MemPool<sizeof(Env),         32> envPool;
//MemPool<sizeof(void*),       32> pointerPool;

void gc() {
	if (GC_DEBUG)
		std::cout << "GC running..." << std::endl;
	// mark
	rootEnv->markTree();
	consPool.markAll();
	symbolPool.markAll();

	// sweep
	size_t sweepSize = 0;
	sweepSize += consPool.sweep();
	sweepSize += symbolPool.sweep();
	sweepSize += intPool.sweep();
	sweepSize += stringPool.sweep();
	sweepSize += procPool.sweep();
	sweepSize += macroPool.sweep();
	sweepSize += envPool.sweep();

	if (GC_DEBUG) {
		std::cout << "GC end" << std::endl;
		std::cout << "sweep size: " << sweepSize << std::endl;
	}
}

template<size_t cellSize, size_t numofCell>
void *alloc(MemPool<cellSize, numofCell> &memPool) {
	void *o = memPool.alloc();
	if (o != nullptr) return o;
	int pageSize = memPool.pageSize();
	if (heapSizeMax < heapSize + pageSize)
		gc();
	else {
		memPool.extend();
		heapSize += pageSize;
	}
	o = memPool.alloc();
	if (o != nullptr) return o;
	throw "heap is full";
}

static void *Cons::operator new(size_t size) {
	return alloc(consPool);
}
static void *Symbol::operator new(size_t size) {
	return alloc(symbolPool);
}
static void *Int::operator new(size_t size) {
	return alloc(intPool);
}
static void *String::operator new(size_t size) {
	return alloc(stringPool);
}
static void *Proc::operator new(size_t size) {
	return alloc(procPool);
}
static void *Macro::operator new(size_t size) {
	return alloc(macroPool);
}
static void *Env::operator new(size_t size) {
	return alloc(envPool);
}

// utilities ///////////////////////////////////////////////

LobjSPtr listLastCdrObj(LobjSPtr objPtr) {
	if (objPtr->typep<Cons>())
		return listLastCdrObj(objPtr->getAs<Cons>().cdr);
	return objPtr;
}

bool isProperList(Lobj *obj) {
	if (obj->typep<Cons>())
		return isProperList(obj->getAs<Cons>().cdr);
	return obj->isNil();
}

int listLength(Lobj *obj) {
	if (obj->typep<Cons>())
		return 1 + listLength(obj->getAs<Cons>().cdr);
	return 0;
}

LobjSPtr listNth(LobjSPtr &objptr, int i) {
	if (typeid(*objptr) != typeid(Cons))
		return LobjSPtr(nullptr);
	if (i == 0)
		return dynamic_cast<Cons*>(objptr)->car;
	return listNth(dynamic_cast<Cons*>(objptr)->cdr, i - 1);
}

LobjSPtr listNthCdr(LobjSPtr &objptr, int i) {
	if (i == 0)
		return objptr;
	if (typeid(*objptr) != typeid(Cons))
		return LobjSPtr(nullptr);
	return listNthCdr(dynamic_cast<Cons*>(objptr)->cdr, i - 1);
}

LobjSPtr map(LobjSPtr objPtr, std::function<LobjSPtr(LobjSPtr)> func) {
	if (!objPtr->typep<Cons>())
		return objPtr;
	Cons *cons = &objPtr->getAs<Cons>();
	return new Cons(func(cons->car), map(cons->cdr, func));
}

void nmap(LobjSPtr objPtr, std::function<LobjSPtr(LobjSPtr)> func) {
	if (!objPtr->typep<Cons>())
		return;
	Cons *cons = &objPtr->getAs<Cons>();
	cons->car = func(cons->car);
	nmap(cons->cdr, func);
}

LobjSPtr boolToLobj(bool b) {
	return intern(b ? "t" : "nil");
}

LobjSPtr evalListElements(EnvSPtr env, LobjSPtr objPtr) {
	if (typeid(*objPtr) != typeid(Cons)) return objPtr;
	Cons *cons = dynamic_cast<Cons*>(objPtr);
	return new Cons(env->eval(cons->car), evalListElements(env, cons->cdr));
}

LobjSPtr vectorToList(std::vector<LobjSPtr> &v) {
	LobjSPtr list = intern("nil");
	for (auto it = v.rbegin(); it != v.rend(); ++it) {
		list = new Cons(*it, list);
	}
	return list;
}

// cardinal ////////////////////////////////////////////////

EnvSPtr makeEnvForMacro(EnvSPtr outerEnv, EnvSPtr procEnv, LobjSPtr prms, LobjSPtr args, bool tail = false) {
	EnvSPtr env = new Env(outerEnv, procEnv);
	if (!isProperList(args))
		throw "bad macro apply";
	while (prms->typep<Cons>() && args->typep<Cons>()) {
		Symbol *symbol = &prms->getAs<Cons>().car->getAs<Symbol>();
		env->bind(args->getAs<Cons>().car, symbol);
		prms = prms->getAs<Cons>().cdr;
		args = args->getAs<Cons>().cdr;
	}
	if (typeid(*prms) == typeid(Symbol) && !prms->isNil()) {
		env->bind(args, &prms->getAs<Symbol>());
	}
	if (tail && !outerEnv->isClosed()) {
		outerEnv->merge(env);
		env = outerEnv;
	}
	return env;
}
/*
EnvSPtr makeEnvForApply(LobjSPtr outerEnv, LobjSPtr procEnv, LobjSPtr prms, LobjSPtr args, bool tail = false) {
	if (!isProperList(args))
		throw "bad apply";
	int prmsLength = listLength(prms);
	int argsLength = listLength(args);
	if (argsLength < prmsLength) throw "arguments is fewer";
	std::vector<LobjSPtr> evaledArgs(prmsLength);
	for (int i = 0; i < prmsLength; ++i) {
		evaledArgs[i] = outerEnv->eval(args->getAs<Cons>().car);
		args = args->getAs<Cons>().cdr;
	}
	LobjSPtr argsRest = evalListElements(outerEnv, args);
	LobjSPtr env;
	if (tail && !outerEnv->isClosed()) {
		env = outerEnv;
		env->setLexEnv(procEnv);
	} else {
		env = new Env(outerEnv, procEnv);
	}
	for (auto &evaledArg : evaledArgs) {
		Symbol *symbol = &prms->getAs<Cons>().car->getAs<Symbol>();
		env->bind(evaledArg, symbol);
		prms = prms->getAs<Cons>().cdr;
	}
	if (typeid(*prms) == typeid(Symbol) && !prms->isNil()) {
		env->bind(argsRest, &prms->getAs<Symbol>());
	}
	return env;
}//*/
//*
EnvSPtr makeEnvForApply(EnvSPtr outerEnv, EnvSPtr procEnv, LobjSPtr prms, LobjSPtr args, bool tail = false) {
	EnvSPtr env = new Env(outerEnv, procEnv);
	if (!isProperList(args))
		throw "bad apply";
	while (prms->typep<Cons>() && args->typep<Cons>()) {
		Symbol *symbol = &prms->getAs<Cons>().car->getAs<Symbol>();
		env->bind(outerEnv->eval(args->getAs<Cons>().car), symbol);
		prms = prms->getAs<Cons>().cdr;
		args = args->getAs<Cons>().cdr;
	}
	if (typeid(*prms) == typeid(Symbol) && !prms->isNil()) {
		LobjSPtr rest = evalListElements(outerEnv, args);
		env->bind(rest, &prms->getAs<Symbol>());
	}
	if (tail && !outerEnv->isClosed()) {
		outerEnv->merge(env);
		env = outerEnv;
	}
	return env;
}//*/


Env::Env() {
	LobjSPtr obj;
	LobjSPtr sform;
	LobjSPtr bfunc;

	obj = intern("if");
	sform = new SpecialForm("if", [](Env *env, LobjSPtr args, bool tail) {
			int length = listLength(args);
			if (length == 2 || length == 3) {
				LobjSPtr cond = listNth(args, 0);
				if(!env->eval(cond)->isNil()) {
					return env->eval(listNth(args, 1), tail);
				} else if (length == 3) {
					return env->eval(listNth(args, 2), tail);
				} else {
					return intern("nil");
				}
			}
			throw "bad 'if'";
		});
	bind(sform, &obj->getAs<Symbol>());

	obj = intern("quote");
	sform = new SpecialForm("quote", [](Env *env, LobjSPtr args, bool tail) {
			int length = listLength(args);
			if (length == 1)
				return listNth(args, 0);
			throw "bad 'quote'";
		});
	bind(sform, &obj->getAs<Symbol>());

	obj = intern("do");
	sform = new SpecialForm("quote", [](Env *env, LobjSPtr args, bool tail) {
			int length = listLength(args);
			if (length == 0)
				return intern("nil");
			Cons *cons = &args->getAs<Cons>();
			while (cons->cdr->typep<Cons>()) {
				env->eval(cons->car);
				cons = &cons->cdr->getAs<Cons>();
			}
			return env->eval(cons->car, tail);
		});
	bind(sform, &obj->getAs<Symbol>());

	obj = intern("def");
	sform = new SpecialForm("def", [](Env *env, LobjSPtr args, bool tail) {
			int length = listLength(args);
			if (length == 2) {
				LobjSPtr variable = listNth(args, 0);
				if (!variable->typep<Symbol>())
					throw "bad 'def'";
				Symbol *symbol = &variable->getAs<Symbol>();
				rootEnv->bind(env->eval(listNth(args, 1), tail), symbol); // tail?
				return variable;
			}
		});
	bind(sform, &obj->getAs<Symbol>());

	obj = intern("set!");
	sform = new SpecialForm("set!", [](Env *env, LobjSPtr args, bool tail) {
			int length = listLength(args);
			if (length == 2) {
				LobjSPtr variable = listNth(args, 0);
				if (!variable->typep<Symbol>())
					throw "bad 'set!'";
				Symbol *symbol = &variable->getAs<Symbol>();
				EnvSPtr bindEnv = env->resolveEnv(symbol);
				if (bindEnv == nullptr) bindEnv = rootEnv;
				LobjSPtr value = env->eval(listNth(args, 1), tail);
				bindEnv->bind(value, symbol);
				return value;
			}
		});
	bind(sform, &obj->getAs<Symbol>());

	obj = intern("let");
	sform = new SpecialForm("let", [](Env *env, LobjSPtr args, bool tail) {
			int length = listLength(args);
			if (length < 1) throw "bad let";

			LobjSPtr bindings = listNth(args, 0);
			if (!isProperList(bindings)) throw "bad let bindings";
			if (listLength(bindings) % 2 != 0) throw "number of bindings elements of let is odd.";
			EnvSPtr letEnv = new Env(env);
			while (!bindings->isNil()) {
				LobjSPtr objSymbol = bindings->getAs<Cons>().car;
				LobjSPtr objForm = bindings->getAs<Cons>().cdr->getAs<Cons>().car;
				// TODO type check
				letEnv->bind(env->eval(objForm), &objSymbol->getAs<Symbol>());
				bindings = listNthCdr(bindings, 2);
			}
			if (tail && !env->closed) {
				env->merge(letEnv);
				letEnv = env;
			}
			return letEnv->eval(new Cons(intern("do"), listNthCdr(args, 1)), TCO);
		});
	bind(sform, &obj->getAs<Symbol>());

	obj = intern("let*");
	sform = new SpecialForm("let*", [](Env *env, LobjSPtr args, bool tail) {
			int length = listLength(args);
			if (length < 1) throw "bad let*";

			LobjSPtr bindings = listNth(args, 0);
			if (!isProperList(bindings)) throw "bad let* bindings";
			if (listLength(bindings) % 2 != 0) throw "number of bindings elements of let* is odd.";
			EnvSPtr letEnv;
			if (tail && !env->closed) {
				letEnv = env;
			} else {
				letEnv = new Env(env);
			}
			while (!bindings->isNil()) {
				LobjSPtr objSymbol = bindings->getAs<Cons>().car;
				LobjSPtr objForm = bindings->getAs<Cons>().cdr->getAs<Cons>().car;
				// TODO type check
				Symbol *symbol = &objSymbol->getAs<Symbol>();
				letEnv->bind(letEnv->eval(objForm), symbol);
				bindings = listNthCdr(bindings, 2);
			}
			return letEnv->eval(new Cons(intern("do"), listNthCdr(args, 1)), TCO);
		});
	bind(sform, &obj->getAs<Symbol>());

	obj = intern("\\");
	sform = new SpecialForm("\\", [](Env *env, LobjSPtr args, bool tail) {
			int length = listLength(args);
			if (1 <= length) {
				LobjSPtr pl = listNth(args, 0);
				//if (!isProperList(pl)) throw "bad lambda form";
				env->closed = true;
				return new Proc(pl, new Cons(intern("do"), listNthCdr(args, 1)), env);
			}
		});
	bind(sform, &obj->getAs<Symbol>());

	obj = intern("macro");
	sform = new SpecialForm("macro", [](Env *env, LobjSPtr args, bool tail) {
			int length = listLength(args);
			if (1 <= length) {
				LobjSPtr pl = listNth(args, 0);
				//if (!isProperList(pl)) throw "bad lambda form";
				env->closed = true;
				return new Macro(pl, new Cons(intern("do"), listNthCdr(args, 1)), env);
			}
		});
	bind(sform, &obj->getAs<Symbol>());


	obj = intern("t");
	bind(obj, &obj->getAs<Symbol>());

	obj = intern("nil");
	bind(obj, &obj->getAs<Symbol>());

	obj = intern("eq?");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			if (args.size() == 0) throw "bad arguments for function 'eq?'";
			for (int i = 0; i < args.size() - 1; ++i) {
				if (!args[i]->eq(args[i+1]))
					return intern("nil");
			}
			return intern("t");
		});
	bind(LobjSPtr(bfunc), &obj->getAs<Symbol>());

	obj = intern("nil?");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			if (args.size() != 1) throw "bad arguments for function 'nil'";
			return boolToLobj(args[0]->isNil());
		});
	bind(LobjSPtr(bfunc), &obj->getAs<Symbol>());

	obj = intern("cons?");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			if (args.size() != 1) throw "bad arguments for function 'nil'";
			return boolToLobj(typeid(*args[0]) == typeid(Cons));
		});
	bind(LobjSPtr(bfunc), &obj->getAs<Symbol>());

	obj = intern("list?");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			if (args.size() != 1) throw "bad arguments for function 'nil'";
			return boolToLobj(typeid(*args[0]) == typeid(Cons) || args[0]->isNil());
		});
	bind(LobjSPtr(bfunc), &obj->getAs<Symbol>());

	obj = intern("symbol?");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			if (args.size() != 1) throw "bad arguments for function 'nil'";
			return boolToLobj(typeid(*args[0]) == typeid(Symbol));
		});
	bind(LobjSPtr(bfunc), &obj->getAs<Symbol>());

	obj = intern("int?");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			if (args.size() != 1) throw "bad arguments for function 'nil'";
			return boolToLobj(typeid(*args[0]) == typeid(Int));
		});
	bind(LobjSPtr(bfunc), &obj->getAs<Symbol>());

	obj = intern("string?");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			if (args.size() != 1) throw "bad arguments for function 'nil'";
			return boolToLobj(typeid(*args[0]) == typeid(String));
		});
	bind(LobjSPtr(bfunc), &obj->getAs<Symbol>());

	obj = intern("proc?");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			if (args.size() != 1) throw "bad arguments for function 'nil'";
			return boolToLobj(typeid(*args[0]) == typeid(Proc) ||
												typeid(*args[0]) == typeid(BuiltinProc));
		});
	bind(LobjSPtr(bfunc), &obj->getAs<Symbol>());

	obj = intern("+");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			int value = 0;
			for (LobjSPtr &objPtr : args) {
				if (typeid(*objPtr) != typeid(Int)) throw "bad arguments for function '+'";
				value += dynamic_cast<Int*>(objPtr)->value;
			}
			return new Int(value);
		});
	bind(LobjSPtr(bfunc), &obj->getAs<Symbol>());

	obj = intern("-");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			if (args.size() == 0 || typeid(*args[0]) != typeid(Int))
				throw "bad arguments for function '-'";
			int value = dynamic_cast<Int*>(args[0])->value;
			if (args.size() == 1)
				return new Int(-value);
			for (int i = 1; i < args.size(); ++i) {
				if (typeid(*args[i]) != typeid(Int)) throw "bad arguments for function '-'";
				value -= dynamic_cast<Int*>(args[i])->value;
			}
			return new Int(value);
		});
	bind(LobjSPtr(bfunc), &obj->getAs<Symbol>());

	obj = intern("*");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			int value = 1;
			for (LobjSPtr &objPtr : args) {
				if (typeid(*objPtr) != typeid(Int)) throw "bad arguments for function '*'";
				value *= dynamic_cast<Int*>(objPtr)->value;
			}
			return new Int(value);
		});
	bind(LobjSPtr(bfunc), &obj->getAs<Symbol>());

	obj = intern("/");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			if (args.size() == 0 || typeid(*args[0]) != typeid(Int))
				throw "bad arguments for function '/'";
			int value = dynamic_cast<Int*>(args[0])->value;
			for (int i = 1; i < args.size(); ++i) {
				if (typeid(*args[i]) != typeid(Int)) throw "bad arguments for function '/'";
				int divisor = dynamic_cast<Int*>(args[i])->value;
				if (divisor == 0) throw "dividing by zero";
				value /= divisor;
			}
			return new Int(value);
		});
	bind(LobjSPtr(bfunc), &obj->getAs<Symbol>());

	obj = intern("mod");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			if (args.size() != 2 ||
					typeid(*args[0]) != typeid(Int) || typeid(*args[1]) != typeid(Int))
				throw "bad arguments for function 'mod'";
			int value = dynamic_cast<Int*>(args[0])->value;
			int divisor = dynamic_cast<Int*>(args[1])->value;
			if (divisor == 0) throw "dividing by zero";
			return new Int(value % divisor);
		});
	bind(LobjSPtr(bfunc), &obj->getAs<Symbol>());

	obj = intern("=");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			if (args.size() == 0) throw "bad arguments for function '='";
			for (LobjSPtr &objPtr : args) {
				if (typeid(*objPtr) != typeid(Int)) throw "bad arguments for function '='";
		}
		for (int i = 0; i < args.size() - 1; ++i) {
			if (dynamic_cast<Int*>(args[i])->value !=
					dynamic_cast<Int*>(args[i+1])->value)
				return intern("nil");
		}
		return intern("t");
	});
	bind(LobjSPtr(bfunc), &obj->getAs<Symbol>());

	obj = intern("<");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			if (args.size() == 0) throw "bad arguments for function '<'";
			for (LobjSPtr &objPtr : args) {
				if (typeid(*objPtr) != typeid(Int)) throw "bad arguments for function '<'";
			}
			for (int i = 0; i < args.size() - 1; ++i) {
				if (dynamic_cast<Int*>(args[i])->value >=
						dynamic_cast<Int*>(args[i+1])->value)
					return intern("nil");
			}
			return intern("t");
		});
	bind(LobjSPtr(bfunc), &obj->getAs<Symbol>());

	obj = intern("print");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			for (LobjSPtr &objPtr : args) {
				objPtr->print(std::cout);
			}
			return intern("nil");
		});
	bind(LobjSPtr(bfunc), &obj->getAs<Symbol>());

	obj = intern("println");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			for (LobjSPtr &objPtr : args) {
				objPtr->print(std::cout);
				std::cout << std::endl;
			}
			return intern("nil");
		});
	bind(LobjSPtr(bfunc), &obj->getAs<Symbol>());

	obj = intern("print-to-string");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			std::stringstream ss;
			for (LobjSPtr &objPtr : args) {
				objPtr->print(ss);
			}
			return new String(ss.str());
		});
	bind(LobjSPtr(bfunc), &obj->getAs<Symbol>());

	obj = intern("car");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			if (args.size() != 1 || typeid(*args[0]) != typeid(Cons))
				throw "bad arguments for function 'car'";
			return dynamic_cast<Cons*>(args[0])->car;
		});
	bind(LobjSPtr(bfunc), &obj->getAs<Symbol>());

	obj = intern("cdr");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			if (args.size() != 1 || typeid(*args[0]) != typeid(Cons))
				throw "bad arguments for function 'cdr'";
			return dynamic_cast<Cons*>(args[0])->cdr;
		});
	bind(LobjSPtr(bfunc), &obj->getAs<Symbol>());

	obj = intern("cons");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
		if (args.size() != 2)
			throw "bad arguments for function 'cons'";
		return new Cons(args[0], args[1]);
	});
	bind(LobjSPtr(bfunc), &obj->getAs<Symbol>());

	obj = intern("gensym");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			std::stringstream ss;
			if (args.size() == 0) {
				ss << "#g" << (gensymId++);
			} else if (args.size() == 1 && typeid(*args[0]) == typeid(String)) {
				ss << "#" << (static_cast<String*>(args[0])->value) << (gensymId++);
			} else {
				throw "bad arguments for function 'gensym'";
			}
			return new Symbol(ss.str());
	});
	bind(LobjSPtr(bfunc), &obj->getAs<Symbol>());

	obj = intern("bound?");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			if (args.size() != 1 || !args[0]->typep<Symbol>())
				throw "bad arguments for function 'bound?'";
			return boolToLobj(env.resolve(&args[0]->getAs<Symbol>()) != nullptr);
	});
	bind(LobjSPtr(bfunc), &obj->getAs<Symbol>());

	obj = intern("get-time");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			if (args.size() != 0)
				throw "bad arguments for function 'get-time'";
			return new Int(static_cast<int>(std::clock() / (CLOCKS_PER_SEC / 1000)));
	});
	bind(LobjSPtr(bfunc), &obj->getAs<Symbol>());

	obj = intern("eval");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
		if (args.size() != 1)
			throw "bad arguments for function 'eval'";
		return env.evalTop(args[0]);
	});
	bind(LobjSPtr(bfunc), &obj->getAs<Symbol>());

	obj = intern("read");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
		if (args.size() != 0)
			throw "bad arguments for function 'read'";
		return read(std::cin);
	});
	bind(LobjSPtr(bfunc), &obj->getAs<Symbol>());

	obj = intern("load");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			if (args.size() != 1 || typeid(*args[0]) != typeid(String))
				throw "bad arguments for function 'load'";
			std::string filename = dynamic_cast<String*>(args[0])->value;
			std::ifstream ifs(filename);
			if (ifs.fail()) return intern("nil");
			try {
				while (!ifs.eof()) {
					LobjSPtr o = read(ifs);
					env.evalTop(o);
					skipCommentOut(ifs);
				}
			} catch (char const *e) {
				std::cout << std::endl << "Load failed." << std::endl;
				return intern("nil");
			}
			return intern("t");
	});
	bind(LobjSPtr(bfunc), &obj->getAs<Symbol>());

	obj = intern("macroexpand-all");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
		if (args.size() != 1)
			throw "bad arguments for function 'macroexpand-all'";
		return env.macroexpandAll(args[0]);
	});
	bind(LobjSPtr(bfunc), &obj->getAs<Symbol>());

	obj = intern("exit");
	bind(obj, &obj->getAs<Symbol>());



	// functions for debug
	obj = intern("proc-parameter-list");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			if (args.size() != 1 || !args[0]->typep<Proc>())
				throw "bad arguments for function 'proc-parameter-list'";
			return args[0]->getAs<Proc>().parameterList;
		});
	bind(LobjSPtr(bfunc), &obj->getAs<Symbol>());
	obj = intern("proc-body");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			if (args.size() != 1 || !args[0]->typep<Proc>())
				throw "bad arguments for function 'proc-body'";
			return args[0]->getAs<Proc>().body;
		});
	bind(LobjSPtr(bfunc), &obj->getAs<Symbol>());
	obj = intern("proc-env-print");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			if (args.size() != 1 || !args[0]->typep<Proc>())
				throw "bad arguments for function 'proc-env-print'";
			args[0]->getAs<Proc>().env->print();
			std::cout << std::endl;
			return intern("nil");
		});
	bind(LobjSPtr(bfunc), &obj->getAs<Symbol>());
	obj = intern("macro-env-print");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			if (args.size() != 1 || !args[0]->typep<Macro>())
				throw "bad arguments for function 'macro-env-print'";
			args[0]->getAs<Macro>().env->print();
			std::cout << std::endl;
			return intern("nil");
		});
	bind(LobjSPtr(bfunc), &obj->getAs<Symbol>());

	obj = intern("env-print");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
		if (args.size() != 0)
			throw "bad arguments for function 'env-print'";
		env.print();
		std::cout << std::endl;
		return intern("nil");
	});
	bind(LobjSPtr(bfunc), &obj->getAs<Symbol>());

	obj = intern("env-print-all");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
		if (args.size() != 0)
			throw "bad arguments for function 'env-print-all'";
		env.printAll(true);
		std::cout << std::endl;
		return intern("nil");
	});
	bind(LobjSPtr(bfunc), &obj->getAs<Symbol>());
}

/*
LobjSPtr Env::macroexpand1 (LobjSPtr objPtr) {
}

LobjSPtr Env::macroexpand (LobjSPtr objPtr) {
}*/

LobjSPtr Env::macroexpandAll(LobjSPtr objPtr) {
	if (!objPtr->typep<Cons>())
		return objPtr;
	RootPtr<Lobj> rp(objPtr);
	Cons *cons = &objPtr->getAs<Cons>();
	if (cons->car->typep<Symbol>()) {
		Symbol *opSymbol = &cons->car->getAs<Symbol>();
		// special form
		// TODO set! let \ macro
		if (opSymbol->name == "quote") {
			return objPtr;
		}
		RootPtr<Lobj> op(resolve(opSymbol));
		//macro
		if (op != nullptr && op->typep<Macro>()) {
			Macro *macro = &op->getAs<Macro>();
			EnvSPtr env = makeEnvForMacro(this, macro->env,
																		macro->parameterList, cons->cdr);
			return macroexpandAll(env->eval(macro->body));
		}
	}
	return map(objPtr, [this](LobjSPtr objPtr) {
			return this->macroexpandAll(objPtr);
		});
}

LobjSPtr Env::eval(LobjSPtr o, bool tail) {
	RootPtr<Env> envPtr(this);
	RootPtr<Lobj> rp(o);
	if (o->typep<Symbol>()) {
		LobjSPtr rr = resolve(&o->getAs<Symbol>());
		if (rr == nullptr) {
			std::cout << "unbound symbol: " << o->getAs<Symbol>().name << std::endl;
			throw "evaluated unbound symbol";
		}
		return rr;
	}
	if (o->typep<Int>() || o->typep<String>()) {
		return o;
	}
	if (o->typep<Cons>()) {
		Cons *cons = &o->getAs<Cons>();
		LobjSPtr opPtr = eval(cons->car);
		if (opPtr->typep<SpecialForm>()) {
			return opPtr->getAs<SpecialForm>().function(this, cons->cdr, tail);
		}
		if (opPtr->typep<Proc>()) {
			Proc *func = &opPtr->getAs<Proc>();
			EnvSPtr env = makeEnvForApply(this, func->env,
																		func->parameterList, cons->cdr, tail);
			return env->eval(func->body, TCO);
		}
		if (opPtr->typep<BuiltinProc>()) {
			BuiltinProc *bfunc = &opPtr->getAs<BuiltinProc>();
			Lobj *argCons = cons->cdr;
			if (!isProperList(argCons))
				throw "bad built-in-function call";
			std::vector<LobjSPtr> args;
			while (!argCons->isNil()) {
				args.push_back(eval(argCons->getAs<Cons>().car));
				argCons = argCons->getAs<Cons>().cdr;
			}
			return bfunc->function(*this, args);
		}
		throw "bad apply";
	}
	return o;
}

void Env::repl() {
	while (1) {
		std::cout << "> ";
		LobjSPtr o = read(std::cin);
		if (o == nullptr) {
			std::cout << std::endl << "Parse failed." << std::endl;
			return;
		}
		o = evalTop(o);
		o->print(std::cout);
		std::cout << std::endl;
		if (o == intern("exit")) break;
	}
}

std::string initializeCode = "(do (println \"Loding core file...\") (println (load \"core.lisp\")))";

int main(int argc, char* argv[]) {
	bool initializeFlg = true;
	for (int i = 0; i < argc; ++i) {
		if (std::string("no-initialize") == argv[i])
			initializeFlg = false;
	}

	rootEnv = new Env();

	if (initializeFlg) {
		std::istringstream ss(initializeCode);
		LobjSPtr objPtr = read(ss);
		rootEnv->evalTop(objPtr);
	}

	try {
		rootEnv->repl();
	} catch (char const *e) {
		std::cout << "Fatal error: " << e << std::endl;
	}
	return 0;
}
