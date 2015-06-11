#include <iostream>
#include <string>
#include <sstream>
#include <memory>
#include <vector>
#include <map>
#include <stdint.h>

/*
struct Symbol {
	const std::string name;

	Symbol(const std::string n)
	: name(n) {}
};*/

class Env;

struct Lobj {
	virtual ~Lobj() {}

	virtual void print(std::ostream &os) const = 0;

	bool isNil() const;
};

typedef std::shared_ptr<Lobj> LobjSPtr;
typedef std::weak_ptr<Lobj>   LobjWPtr;

struct Cons : public Lobj {
	LobjSPtr car;
	LobjSPtr cdr;
	
	Cons(LobjSPtr a, LobjSPtr d)
	: car(a), cdr(d) {}

	void print(std::ostream &os) const;
};
/*
struct SymbolRef : public Lobj {
	Symbol *symbol;

	SymbolRef(Symbol *s)
	: symbol(s) {}

	void print(std::ostream &os) const;
};*/

struct Symbol : public Lobj {
	const std::string name;

	Symbol(const std::string n)
	: name(n) {}

	void print(std::ostream &os) const;
};

struct Int : public Lobj {
	int value = 0;

	Int (int v)
	: value(v) {}

	void print(std::ostream &os) const;
};

struct Func : public Lobj {
	LobjSPtr parameterList;
	LobjSPtr body;

	Func (LobjSPtr pl, LobjSPtr b)
	: parameterList(pl), body(b) {}

	void print(std::ostream &os) const;
};

struct BuiltinFunc : public Lobj {
	std::function<LobjSPtr(Env &env, std::vector<LobjSPtr> &)> function;

	BuiltinFunc (std::function<LobjSPtr(Env &env, std::vector<LobjSPtr> &)> f)
	: function(f) {}

	void print(std::ostream &os) const;
};


void Cons::print(std::ostream &os) const {
	os << "(";
	car->print(os);
	Lobj *o = this->cdr.get();
	while (1) {
		if (typeid(*o) == typeid(Cons)) {
			os << " ";
			Cons *cons = dynamic_cast<Cons*>(o);
			cons->car->print(os);
			o = cons->cdr.get();
		} else if (typeid(*o) == typeid(Symbol) &&
				dynamic_cast<Symbol*>(o)->name == "nil") {
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

void Func::print(std::ostream &os) const {
	os << "#Func";
}

void BuiltinFunc::print(std::ostream &os) const {
	os << "#BuiltinFunc";
}

bool Lobj::isNil() const {
	return typeid(*this) == typeid(Symbol) &&
		dynamic_cast<const Symbol*>(this)->name == "nil";
}


std::map<std::string, LobjSPtr> symbolMap;

LobjSPtr intern(std::string name) {
	auto it = symbolMap.find(name);
	LobjSPtr objPtr;
	if (it == symbolMap.end()) {
		objPtr = LobjSPtr(new Symbol(name));
		symbolMap[name] = objPtr;
	} else {
		objPtr = it->second;
	}
	return objPtr;
}

class Env {
	Env *parent = nullptr;
	std::map<Symbol*, LobjSPtr> symbolValueMap;

public:
	Env();

	Env(Env *p)
	: parent(p) {
	}

	LobjSPtr resolve(Symbol *symbol) const {
		auto it = symbolValueMap.find(symbol);
		if (it != symbolValueMap.end())
			return it->second;
		if (parent != nullptr)
			return parent->resolve(symbol);
		return LobjSPtr(nullptr);
	}

	void bind(LobjSPtr objPtr, Symbol *symbol) {
		symbolValueMap[symbol] = objPtr;
	}

	LobjSPtr read(std::istream &is);

	LobjSPtr procSpecialForm(LobjSPtr objPtr);
	LobjSPtr eval(LobjSPtr objPtr);

	void repl() {
		while (1) {
			//print(eval(read(std::cin)));
			LobjSPtr o = read(std::cin);
			/*if (o != nullptr) {
				o->print(std::cout);
				std::cout << std::endl;
			} else {
				std::cout << "nil" << std::endl;
				break;
			}*/
			if (eval(o) == intern("exit"))
				break;
		}
	}
};

bool isSymbolChar(const char c) {
	return c != '(' && c != ')' && c != ' ' &&
		c != '\t' && c != '\n' && c != '\r' && c != 0;
}

LobjSPtr readAux(Env &env, std::istream &is);

LobjSPtr readList(Env &env, std::istream &is) {
	is >> std::ws;
	if (is.eof()) throw "parse failed";
	char c = is.get();
	if (c == ')') {
		return intern("nil");
	} else if (c == '.') {
		LobjSPtr cdr = readAux(env, is);
		is >> std::ws;
		if (is.get() != ')') throw "parse failed";
		return cdr;
	} else {
		is.unget();
		LobjSPtr car = readAux(env, is);
		LobjSPtr cdr = readList(env, is);
		return LobjSPtr(new Cons(car, cdr));
	}
}

LobjSPtr readAux(Env &env, std::istream &is) {
	is >> std::ws;
	if (is.eof()) throw "parse failed";
	char c = is.get();
	if (c == '(') {
		return readList(env, is);
	} else if (('0' <= c && c <= '9') || (c == '-' && ('0' <= is.peek() && is.peek() <= '9'))) {
		is.unget();
		int value;
		is >> value;
		return LobjSPtr(new Int(value));
	} else if (c == ';') {
		while (c != 0 && c != '\n' && c != '\r') c = is.get();
		return readAux(env, is);
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

LobjSPtr Env::read(std::istream &is) {
	try {
		return readAux(*this, is);
	} catch (char const *e) {
		return LobjSPtr(nullptr);
	}
}

bool isProperList(Lobj *obj) {
	if (typeid(*obj) == typeid(Cons))
		return isProperList(dynamic_cast<Cons*>(obj)->cdr.get());
	if (typeid(*obj) == typeid(Symbol))
		return dynamic_cast<Symbol*>(obj)->name == "nil";
	return false;
}

int listLength(Lobj *obj) {
	if (typeid(*obj) == typeid(Cons))
		return 1 + listLength(dynamic_cast<Cons*>(obj)->cdr.get());
	return 0;
}

LobjSPtr listNth(LobjSPtr &objptr, int i) {
	if (typeid(*objptr) != typeid(Cons))
		return LobjSPtr(nullptr);
	if (i == 0)
		return dynamic_cast<Cons*>(objptr.get())->car;
	return listNth(dynamic_cast<Cons*>(objptr.get())->cdr, i - 1);
}

LobjSPtr listNthCdr(LobjSPtr &objptr, int i) {
	if (typeid(*objptr) != typeid(Cons))
		return LobjSPtr(nullptr);
	if (i == 0)
		return objptr;
	return listNthCdr(dynamic_cast<Cons*>(objptr.get())->cdr, i - 1);
}


Env::Env() {
	LobjSPtr obj;
	BuiltinFunc *bfunc;

	obj = intern("t");
	bind(obj, dynamic_cast<Symbol*>(obj.get()));

	obj = intern("nil");
	bind(obj, dynamic_cast<Symbol*>(obj.get()));

	obj = intern("do");
	bfunc = new BuiltinFunc([](Env &env, std::vector<LobjSPtr> &args) {
		return args.back();
	});
	bind(LobjSPtr(bfunc), dynamic_cast<Symbol*>(obj.get()));

	obj = intern("+");
	bfunc = new BuiltinFunc([](Env &env, std::vector<LobjSPtr> &args) {
		int value = 0;
		for (LobjSPtr &objPtr : args) {
			if (typeid(*objPtr) != typeid(Int)) throw "bad arguments for function '+'";
			value += dynamic_cast<Int*>(objPtr.get())->value;
		}
		return LobjSPtr(new Int(value));
	});
	bind(LobjSPtr(bfunc), dynamic_cast<Symbol*>(obj.get()));

	obj = intern("-");
	bfunc = new BuiltinFunc([](Env &env, std::vector<LobjSPtr> &args) {
		if (args.size() == 0 || typeid(*args[0]) != typeid(Int))
			throw "bad arguments for function '-'";
		int value = dynamic_cast<Int*>(args[0].get())->value;
		if (args.size() == 1)
			return LobjSPtr(new Int(-value));
		for (int i = 1; i < args.size(); ++i) {
			if (typeid(*args[i]) != typeid(Int)) throw "bad arguments for function '-'";
			value -= dynamic_cast<Int*>(args[i].get())->value;
		}
		return LobjSPtr(new Int(value));
	});
	bind(LobjSPtr(bfunc), dynamic_cast<Symbol*>(obj.get()));

	obj = intern("*");
	bfunc = new BuiltinFunc([](Env &env, std::vector<LobjSPtr> &args) {
		int value = 0;
		for (LobjSPtr &objPtr : args) {
			if (typeid(*objPtr) != typeid(Int)) throw "bad arguments for function '*'";
			value *= dynamic_cast<Int*>(objPtr.get())->value;
		}
		return LobjSPtr(new Int(value));
	});
	bind(LobjSPtr(bfunc), dynamic_cast<Symbol*>(obj.get()));

	obj = intern("=");
	bfunc = new BuiltinFunc([](Env &env, std::vector<LobjSPtr> &args) {
		if (args.size() == 0) throw "bad arguments for function '='";
		for (LobjSPtr &objPtr : args) {
			if (typeid(*objPtr) != typeid(Int)) throw "bad arguments for function '='";
		}
		for (int i = 0; i < args.size() - 1; ++i) {
			if (dynamic_cast<Int*>(args[i].get())->value !=
					dynamic_cast<Int*>(args[i+1].get())->value)
				return intern("nil");
		}
		return intern("t");
	});
	bind(LobjSPtr(bfunc), dynamic_cast<Symbol*>(obj.get()));

	obj = intern("<");
	bfunc = new BuiltinFunc([](Env &env, std::vector<LobjSPtr> &args) {
		if (args.size() == 0) throw "bad arguments for function '<'";
		for (LobjSPtr &objPtr : args) {
			if (typeid(*objPtr) != typeid(Int)) throw "bad arguments for function '<'";
		}
		for (int i = 0; i < args.size() - 1; ++i) {
			if (dynamic_cast<Int*>(args[i].get())->value >=
					dynamic_cast<Int*>(args[i+1].get())->value)
				return intern("nil");
		}
		return intern("t");
	});
	bind(LobjSPtr(bfunc), dynamic_cast<Symbol*>(obj.get()));

	obj = intern("print");
	bfunc = new BuiltinFunc([](Env &env, std::vector<LobjSPtr> &args) {
		for (LobjSPtr &objPtr : args) {
			objPtr->print(std::cout);
			std::cout << std::endl;
		}
		return intern("nil");
	});
	bind(LobjSPtr(bfunc), dynamic_cast<Symbol*>(obj.get()));

	obj = intern("car");
	bfunc = new BuiltinFunc([](Env &env, std::vector<LobjSPtr> &args) {
		if (args.size() != 1 || typeid(*args[0]) != typeid(Cons))
			throw "bad arguments for function 'car'";
		return dynamic_cast<Cons*>(args[0].get())->car;
	});
	bind(LobjSPtr(bfunc), dynamic_cast<Symbol*>(obj.get()));

	obj = intern("cdr");
	bfunc = new BuiltinFunc([](Env &env, std::vector<LobjSPtr> &args) {
		if (args.size() != 1 || typeid(*args[0]) != typeid(Cons))
			throw "bad arguments for function 'cdr'";
		return dynamic_cast<Cons*>(args[0].get())->cdr;
	});
	bind(LobjSPtr(bfunc), dynamic_cast<Symbol*>(obj.get()));

	obj = intern("cons");
	bfunc = new BuiltinFunc([](Env &env, std::vector<LobjSPtr> &args) {
		if (args.size() != 2)
			throw "bad arguments for function 'cons'";
		return LobjSPtr(new Cons(args[0], args[1]));
	});
	bind(LobjSPtr(bfunc), dynamic_cast<Symbol*>(obj.get()));

	obj = intern("eval");
	bfunc = new BuiltinFunc([](Env &env, std::vector<LobjSPtr> &args) {
		if (args.size() != 1)
			throw "bad arguments for function 'eval'";
		return env.eval(args[0]);
	});
	bind(LobjSPtr(bfunc), dynamic_cast<Symbol*>(obj.get()));

	obj = intern("exit");
	bind(obj, dynamic_cast<Symbol*>(obj.get()));
}

LobjSPtr Env::procSpecialForm(LobjSPtr objPtr) {
	Cons *cons = dynamic_cast<Cons*>(objPtr.get());
	Lobj *op = cons->car.get();
	if (typeid(*op) != typeid(Symbol))
		return LobjSPtr(nullptr);
	int length = listLength(cons);
	std::string opName = dynamic_cast<Symbol*>(op)->name;
	if (opName == "if") {
		if (length == 3 || length == 4) {
			LobjSPtr cond = listNth(objPtr, 1);
			if(!eval(cond)->isNil()) {
				return eval(listNth(objPtr, 2));
			} else if (length == 4) {
				return eval(listNth(objPtr, 3));
			} else {
				return intern("nil");
			}
		}
	} else if (opName == "quote") {
		if (length == 2)
			return listNth(objPtr, 1);
	} else if (opName == "setq") {
		if (length == 3) {
			LobjSPtr variable = listNth(objPtr, 1);
			if (typeid(*variable) != typeid(Symbol))
				throw "bad setq";
			bind(eval(listNth(objPtr, 2)), dynamic_cast<Symbol*>(variable.get()));
			return variable;
		}
	} else if (opName == "\\") {
		if (2 <= length) {
			LobjSPtr pl = listNth(objPtr, 1);
			if (!isProperList(pl.get())) throw "bad lambda form";
			return LobjSPtr(new Func(pl, LobjSPtr(new Cons(intern("do"), listNthCdr(objPtr, 2)))));
		}
	}
	return LobjSPtr(nullptr);
}

LobjSPtr Env::eval(LobjSPtr objPtr) {
	//objPtr->print(std::cout); std::cout << " | ";
	Lobj *o = objPtr.get();
	if (typeid(*o) == typeid(Symbol)) {
		LobjSPtr rr = resolve(dynamic_cast<Symbol*>(o));
		if (rr == nullptr) throw "evaluated unbound symbol";
		return rr;
	}
	if (typeid(*o) == typeid(Int)) {
		return objPtr;
	}
	if (typeid(*o) == typeid(Cons)) {
		LobjSPtr psfr = procSpecialForm(objPtr);
		if (psfr != nullptr) {
			return psfr;
		}

		Cons *cons = dynamic_cast<Cons*>(o);
		LobjSPtr opPtr = eval(cons->car);
		if (typeid(*opPtr) == typeid(Func)) {
			Func *func = dynamic_cast<Func*>(opPtr.get());
			Env env(this);
			Lobj *prmCons = func->parameterList.get();
			Lobj *argCons = cons->cdr.get();
			if (!isProperList(argCons))
				throw "bad function call";
			while (!prmCons->isNil() && !argCons->isNil()) {
				Symbol *symbol = dynamic_cast<Symbol*>(dynamic_cast<Cons*>(prmCons)->car.get());
				env.bind(eval(dynamic_cast<Cons*>(argCons)->car), symbol);
				prmCons = dynamic_cast<Cons*>(prmCons)->cdr.get();
				argCons = dynamic_cast<Cons*>(argCons)->cdr.get();
			}
			return env.eval(func->body);
		}
		if (typeid(*opPtr) == typeid(BuiltinFunc)) {
			BuiltinFunc *bfunc = dynamic_cast<BuiltinFunc*>(opPtr.get());
			Lobj *argCons = cons->cdr.get();
			if (!isProperList(argCons))
				throw "bad built-in-function call";
			std::vector<LobjSPtr> args;
			while (!argCons->isNil()) {
				args.push_back(eval(dynamic_cast<Cons*>(argCons)->car));
				argCons = dynamic_cast<Cons*>(argCons)->cdr.get();
			}
			return bfunc->function(*this, args);
		}
		throw "bad evaluation";
	}
	return objPtr;
}

int main() {
	Env env;
	try {
		env.repl();
	} catch (char const *e) {
		std::cout << "Fatal error: " << e << std::endl;
	}
	return 0;
}
