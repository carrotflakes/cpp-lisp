#include <iostream>
#include <sstream>
#include <string>
#include <memory>
#include <vector>
#include <map>
#include <stdint.h>
#include <fstream>

class Env;

struct Lobj {
	virtual ~Lobj() {}

	virtual void print(std::ostream &os) const = 0;
	virtual bool eq(Lobj *obj) const;
	bool isNil() const;
};

typedef std::shared_ptr<Lobj> LobjSPtr;
typedef std::weak_ptr<Lobj>   LobjWPtr;
typedef std::shared_ptr<Env> EnvSPtr;
typedef std::weak_ptr<Env>   EnvWPtr;

struct Cons : public Lobj {
	LobjSPtr car;
	LobjSPtr cdr;

	Cons(LobjSPtr a, LobjSPtr d)
	: car(a), cdr(d) {}

	void print(std::ostream &os) const;
};

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
	bool eq(Lobj *obj) const;
};

struct String : public Lobj {
	std::string value = 0;

	String (const std::string &v)
	: value(v) {}

	void print(std::ostream &os) const;
	bool eq(Lobj *obj) const;
};


struct Proc : public Lobj {
	LobjSPtr parameterList;
	LobjSPtr body;
	EnvSPtr env;

	Proc (LobjSPtr pl, LobjSPtr b, EnvSPtr e)
	: parameterList(pl), body(b), env(e) {}

	void print(std::ostream &os) const;
};

struct BuiltinProc : public Lobj {
	std::function<LobjSPtr(Env &env, std::vector<LobjSPtr> &)> function;

	BuiltinProc (std::function<LobjSPtr(Env &env, std::vector<LobjSPtr> &)> f)
	: function(f) {}

	void print(std::ostream &os) const;
};

struct Macro : public Lobj {
	LobjSPtr parameterList;
	LobjSPtr body;
	EnvSPtr env;

	Macro (LobjSPtr pl, LobjSPtr b, EnvSPtr e)
	: parameterList(pl), body(b), env(e) {}

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

void String::print(std::ostream &os) const {
	os << value;
}

void Proc::print(std::ostream &os) const {
	os << "#Proc";
}

void BuiltinProc::print(std::ostream &os) const {
	os << "#BuiltinProc";
}

void Macro::print(std::ostream &os) const {
	os << "#Macro";
}

bool Lobj::eq(Lobj *obj) const {
	return this == obj;
}

bool Int::eq(Lobj *obj) const {
	return typeid(*obj) == typeid(Int) && value == static_cast<Int*>(obj)->value;
}

bool String::eq(Lobj *obj) const {
	return typeid(*obj) == typeid(String) && value == static_cast<String*>(obj)->value;
}

bool Lobj::isNil() const {
	return typeid(*this) == typeid(Symbol) &&
		dynamic_cast<const Symbol*>(this)->name == "nil";
}


int gensymId = 0;
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
	EnvWPtr self;
	EnvSPtr outerEnv;
	EnvSPtr lexEnv;
	std::map<Symbol*, LobjSPtr> symbolValueMap;

	Env();
	Env(EnvSPtr e, EnvSPtr l)
		: outerEnv(e), lexEnv(l) {}

public:
	static EnvSPtr makeEnv() {
		EnvSPtr env(new Env());
		env->self = env;
		return env;
	}

	static EnvSPtr makeInnerEnv(EnvSPtr p, EnvSPtr l = nullptr) {
		EnvSPtr env(new Env(p, l));
		env->self = env;
		return env;
	}

	EnvSPtr rootEnv() const {
		if (outerEnv != nullptr)
			return outerEnv->rootEnv();
		return EnvSPtr(self);
	}

	EnvSPtr resolveEnv(Symbol *symbol) const {
		if (symbolValueMap.count(symbol))
			return EnvSPtr(self);
		if (lexEnv != nullptr) {
			EnvSPtr le = lexEnv->resolveEnvLex(symbol);
			if (le != nullptr)
				return le;
			if (outerEnv != nullptr)
				return outerEnv->resolveEnvDyn(symbol);
			return EnvSPtr(nullptr);
		}
		if (outerEnv != nullptr)
			return outerEnv->resolveEnv(symbol);
		return EnvSPtr(nullptr);
	}

	EnvSPtr resolveEnvDyn(Symbol *symbol) const {
		if (symbolValueMap.count(symbol))
			return EnvSPtr(self);
		if (outerEnv != nullptr)
			return outerEnv->resolveEnvDyn(symbol);
		return EnvSPtr(nullptr);
	}

	EnvSPtr resolveEnvLex(Symbol *symbol) const {
		if (symbolValueMap.count(symbol))
			return EnvSPtr(self);
		if (lexEnv != nullptr)
			return lexEnv->resolveEnvLex(symbol);
		if (outerEnv != nullptr)
			return outerEnv->resolveEnvLex(symbol);
		return EnvSPtr(nullptr);
	}


	LobjSPtr resolve(Symbol *symbol) {
		EnvSPtr env = resolveEnv(symbol);
		if (env == nullptr) return LobjSPtr(nullptr);
		return env->symbolValueMap[symbol];
	}

	void bind(LobjSPtr objPtr, Symbol *symbol) {
		symbolValueMap[symbol] = objPtr;
	}

	LobjSPtr read(std::istream &is);

	/*	LobjSPtr macroexpand1(LobjSPtr objPtr);
	LobjSPtr macroexpand(LobjSPtr objPtr);	*/
	LobjSPtr macroexpandAll(LobjSPtr objPtr);

	LobjSPtr procSpecialForm(LobjSPtr objPtr);
	LobjSPtr eval(LobjSPtr objPtr);

	void repl() {
		while (1) {
			std::cout << "> ";
			LobjSPtr o = read(std::cin);
			if (o == nullptr) {
				std::cout << std::endl << "Parse failed." << std::endl;
				return;
			}
			o = macroexpandAll(o);
			o = eval(o);
			o->print(std::cout);
			std::cout << std::endl;
			if (o == intern("exit")) break;
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

LobjSPtr readString(Env &env, std::istream &is) {
	char c = is.get();
	std::stringstream ss;
	while (c != '"') {
		if (c == '\\') {
			switch (is.get()) {
			case 'n': c = '\n'; break;
			case 'f': c = '\f'; break;
			case 'b': c = '\b'; break;
			case 'r': c = '\r'; break;
			case 't': c = '\t'; break;
			case '\'': c = '\''; break;
			case '\"': c = '\"'; break;
			case '\\': c = '\\'; break;
			}
		}
		ss << c;
		if (is.eof()) throw "parse failed";
		c = is.get();
	}
	return LobjSPtr(new String(ss.str()));
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

LobjSPtr readAux(Env &env, std::istream &is) {
	skipCommentOut(is);
	if (is.eof()) throw "parse failed";
	char c = is.get();
	if (c == '(') {
		return readList(env, is);
	} else if (('0' <= c && c <= '9') ||
						 (c == '-' && ('0' <= is.peek() && is.peek() <= '9'))) {
		is.unget();
		int value;
		is >> value;
		return LobjSPtr(new Int(value));
	} else if (c == '"') {
		return readString(env, is);
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

// TODO LobjSPtr lastCdrObj(LobjSPtr objPtr) {

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
	if (i == 0)
		return objptr;
	if (typeid(*objptr) != typeid(Cons))
		return LobjSPtr(nullptr);
	return listNthCdr(dynamic_cast<Cons*>(objptr.get())->cdr, i - 1);
}

LobjSPtr map(LobjSPtr objPtr, std::function<LobjSPtr(LobjSPtr)> func) {
	if (typeid(*objPtr) != typeid(Cons))
		return objPtr;
	Cons *cons = dynamic_cast<Cons*>(objPtr.get());
	return LobjSPtr(new Cons(func(cons->car), map(cons->cdr, func)));
}

LobjSPtr boolToLobj(bool b) {
	return intern(b ? "t" : "nil");
}

LobjSPtr evalListElements(EnvSPtr env, LobjSPtr objPtr) {
	if (typeid(*objPtr) != typeid(Cons)) return objPtr;
	Cons *cons = dynamic_cast<Cons*>(objPtr.get());
	return LobjSPtr(new Cons(env->eval(cons->car), evalListElements(env, cons->cdr)));
}

EnvSPtr makeEnvForMacro(EnvSPtr outerEnv, EnvSPtr procEnv, LobjSPtr prms, LobjSPtr args) {
	EnvSPtr env = outerEnv->makeInnerEnv(outerEnv, procEnv);
	if (!isProperList(args.get()))
		throw "bad macro apply";
	while (typeid(*prms) == typeid(Cons) && typeid(*args) == typeid(Cons)) {
		Symbol *symbol = dynamic_cast<Symbol*>(dynamic_cast<Cons*>(prms.get())->car.get());
		env->bind(dynamic_cast<Cons*>(args.get())->car, symbol);
		prms = dynamic_cast<Cons*>(prms.get())->cdr;
		args = dynamic_cast<Cons*>(args.get())->cdr;
	}
	if (typeid(*prms) == typeid(Symbol) && !prms->isNil()) {
		env->bind(args, dynamic_cast<Symbol*>(prms.get()));
	}
	return env;
}

EnvSPtr makeEnvForApply(EnvSPtr outerEnv, EnvSPtr procEnv, LobjSPtr prms, LobjSPtr args) {
	EnvSPtr env = outerEnv->makeInnerEnv(outerEnv, procEnv);
	if (!isProperList(args.get()))
		throw "bad apply";
	while (typeid(*prms) == typeid(Cons) && typeid(*args) == typeid(Cons)) {
		Symbol *symbol = dynamic_cast<Symbol*>(dynamic_cast<Cons*>(prms.get())->car.get());
		env->bind(outerEnv->eval(dynamic_cast<Cons*>(args.get())->car), symbol);
		prms = dynamic_cast<Cons*>(prms.get())->cdr;
		args = dynamic_cast<Cons*>(args.get())->cdr;
	}
	if (typeid(*prms) == typeid(Symbol) && !prms->isNil()) {
		LobjSPtr rest = evalListElements(outerEnv, args);
		env->bind(rest, dynamic_cast<Symbol*>(prms.get()));
	}
	return env;
}


Env::Env() {
	LobjSPtr obj;
	BuiltinProc *bfunc;

	obj = intern("t");
	bind(obj, dynamic_cast<Symbol*>(obj.get()));

	obj = intern("nil");
	bind(obj, dynamic_cast<Symbol*>(obj.get()));

	obj = intern("eq?");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			if (args.size() == 0) throw "bad arguments for function 'eq?'";
			for (int i = 0; i < args.size() - 1; ++i) {
				if (!args[i]->eq(args[i+1].get()))
					return intern("nil");
			}
			return intern("t");
		});
	bind(LobjSPtr(bfunc), dynamic_cast<Symbol*>(obj.get()));

	obj = intern("nil?");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			if (args.size() != 1) throw "bad arguments for function 'nil'";
			return boolToLobj(args[0]->isNil());
		});
	bind(LobjSPtr(bfunc), dynamic_cast<Symbol*>(obj.get()));

	obj = intern("cons?");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			if (args.size() != 1) throw "bad arguments for function 'nil'";
			return boolToLobj(typeid(*args[0]) == typeid(Cons));
		});
	bind(LobjSPtr(bfunc), dynamic_cast<Symbol*>(obj.get()));

	obj = intern("list?");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			if (args.size() != 1) throw "bad arguments for function 'nil'";
			return boolToLobj(typeid(*args[0]) == typeid(Cons) || args[0]->isNil());
		});
	bind(LobjSPtr(bfunc), dynamic_cast<Symbol*>(obj.get()));

	obj = intern("symbol?");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			if (args.size() != 1) throw "bad arguments for function 'nil'";
			return boolToLobj(typeid(*args[0]) == typeid(Symbol));
		});
	bind(LobjSPtr(bfunc), dynamic_cast<Symbol*>(obj.get()));

	obj = intern("int?");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			if (args.size() != 1) throw "bad arguments for function 'nil'";
			return boolToLobj(typeid(*args[0]) == typeid(Int));
		});
	bind(LobjSPtr(bfunc), dynamic_cast<Symbol*>(obj.get()));

	obj = intern("string?");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			if (args.size() != 1) throw "bad arguments for function 'nil'";
			return boolToLobj(typeid(*args[0]) == typeid(String));
		});
	bind(LobjSPtr(bfunc), dynamic_cast<Symbol*>(obj.get()));

	obj = intern("proc?");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			if (args.size() != 1) throw "bad arguments for function 'nil'";
			return boolToLobj(typeid(*args[0]) == typeid(Proc) ||
												typeid(*args[0]) == typeid(BuiltinProc));
		});
	bind(LobjSPtr(bfunc), dynamic_cast<Symbol*>(obj.get()));

	obj = intern("do");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			if (args.size() == 0) return intern("nil");
			return args.back();
		});
	bind(LobjSPtr(bfunc), dynamic_cast<Symbol*>(obj.get()));

	obj = intern("+");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			int value = 0;
			for (LobjSPtr &objPtr : args) {
				if (typeid(*objPtr) != typeid(Int)) throw "bad arguments for function '+'";
				value += dynamic_cast<Int*>(objPtr.get())->value;
			}
			return LobjSPtr(new Int(value));
		});
	bind(LobjSPtr(bfunc), dynamic_cast<Symbol*>(obj.get()));

	obj = intern("-");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
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
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			int value = 1;
			for (LobjSPtr &objPtr : args) {
				if (typeid(*objPtr) != typeid(Int)) throw "bad arguments for function '*'";
				value *= dynamic_cast<Int*>(objPtr.get())->value;
			}
			return LobjSPtr(new Int(value));
		});
	bind(LobjSPtr(bfunc), dynamic_cast<Symbol*>(obj.get()));

	obj = intern("/");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			if (args.size() == 0 || typeid(*args[0]) != typeid(Int))
				throw "bad arguments for function '/'";
			int value = dynamic_cast<Int*>(args[0].get())->value;
			for (int i = 1; i < args.size(); ++i) {
				if (typeid(*args[i]) != typeid(Int)) throw "bad arguments for function '/'";
				int divisor = dynamic_cast<Int*>(args[i].get())->value;
				if (divisor == 0) throw "dividing by zero";
				value /= divisor;
			}
			return LobjSPtr(new Int(value));
		});
	bind(LobjSPtr(bfunc), dynamic_cast<Symbol*>(obj.get()));

	obj = intern("mod");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			if (args.size() != 2 ||
					typeid(*args[0]) != typeid(Int) || typeid(*args[1]) != typeid(Int))
				throw "bad arguments for function 'mod'";
			int value = dynamic_cast<Int*>(args[0].get())->value;
			int divisor = dynamic_cast<Int*>(args[1].get())->value;
			if (divisor == 0) throw "dividing by zero";
			return LobjSPtr(new Int(value % divisor));
		});
	bind(LobjSPtr(bfunc), dynamic_cast<Symbol*>(obj.get()));

	obj = intern("=");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
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
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
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
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			for (LobjSPtr &objPtr : args) {
				objPtr->print(std::cout);
			}
			return intern("nil");
		});
	bind(LobjSPtr(bfunc), dynamic_cast<Symbol*>(obj.get()));

	obj = intern("println");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			for (LobjSPtr &objPtr : args) {
				objPtr->print(std::cout);
				std::cout << std::endl;
			}
			return intern("nil");
		});
	bind(LobjSPtr(bfunc), dynamic_cast<Symbol*>(obj.get()));

	obj = intern("print-to-string");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			std::stringstream ss;
			for (LobjSPtr &objPtr : args) {
				objPtr->print(ss);
			}
			return LobjSPtr(new String(ss.str()));
		});
	bind(LobjSPtr(bfunc), dynamic_cast<Symbol*>(obj.get()));

	obj = intern("car");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			if (args.size() != 1 || typeid(*args[0]) != typeid(Cons))
				throw "bad arguments for function 'car'";
			return dynamic_cast<Cons*>(args[0].get())->car;
		});
	bind(LobjSPtr(bfunc), dynamic_cast<Symbol*>(obj.get()));

	obj = intern("cdr");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			if (args.size() != 1 || typeid(*args[0]) != typeid(Cons))
				throw "bad arguments for function 'cdr'";
			return dynamic_cast<Cons*>(args[0].get())->cdr;
		});
	bind(LobjSPtr(bfunc), dynamic_cast<Symbol*>(obj.get()));

	obj = intern("cons");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
		if (args.size() != 2)
			throw "bad arguments for function 'cons'";
		return LobjSPtr(new Cons(args[0], args[1]));
	});
	bind(LobjSPtr(bfunc), dynamic_cast<Symbol*>(obj.get()));

	obj = intern("gensym");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			std::stringstream ss;
			if (args.size() == 0) {
				ss << "#g" << (gensymId++);
			} else if (args.size() == 1 && typeid(*args[0]) == typeid(String)) {
				ss << "#" << (static_cast<String*>(args[0].get())->value) << (gensymId++);
			} else {
				throw "bad arguments for function 'gensym'";
			}
			return LobjSPtr(new Symbol(ss.str()));
	});
	bind(LobjSPtr(bfunc), dynamic_cast<Symbol*>(obj.get()));

	obj = intern("eval");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
		if (args.size() != 1)
			throw "bad arguments for function 'eval'";
		return env.eval(args[0]);
	});
	bind(LobjSPtr(bfunc), dynamic_cast<Symbol*>(obj.get()));

	obj = intern("read");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
		if (args.size() != 0)
			throw "bad arguments for function 'read'";
		return env.read(std::cin);
	});
	bind(LobjSPtr(bfunc), dynamic_cast<Symbol*>(obj.get()));

	obj = intern("load");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
			if (args.size() != 1 || typeid(*args[0]) != typeid(String))
				throw "bad arguments for function 'load'";
			std::string filename = dynamic_cast<String*>(args[0].get())->value;
			std::ifstream ifs(filename);
			if (ifs.fail()) return intern("nil");
			try {
				while (!ifs.eof()) {
					LobjSPtr o = env.read(ifs);
					o = env.macroexpandAll(o);
					env.eval(o);
					skipCommentOut(ifs);
				}
			} catch (char const *e) {
				std::cout << std::endl << "Parse failed." << std::endl;
				return intern("nil");
			}
			return intern("t");
	});
	bind(LobjSPtr(bfunc), dynamic_cast<Symbol*>(obj.get()));

	obj = intern("macroexpand-all");
	bfunc = new BuiltinProc([](Env &env, std::vector<LobjSPtr> &args) {
		if (args.size() != 1)
			throw "bad arguments for function 'macroexpand-all'";
		return env.macroexpandAll(args[0]);
	});
	bind(LobjSPtr(bfunc), dynamic_cast<Symbol*>(obj.get()));

	obj = intern("exit");
	bind(obj, dynamic_cast<Symbol*>(obj.get()));
}

/*
LobjSPtr Env::macroexpand1 (LobjSPtr objPtr) {
}

LobjSPtr Env::macroexpand (LobjSPtr objPtr) {
}*/

LobjSPtr Env::macroexpandAll(LobjSPtr objPtr) {
	if (typeid(*objPtr) != typeid(Cons))
		return objPtr;
	Cons *cons = dynamic_cast<Cons*>(objPtr.get());
	if (typeid(*cons->car) == typeid(Symbol)) {
		Symbol *opSymbol = dynamic_cast<Symbol*>(cons->car.get());
		// special form
		// TODO quote set! let \ macro
		if (opSymbol->name == "quote") {
			return objPtr;
		}
		//if (opSymbol->name == "set!") {
		//}
		LobjSPtr op = resolve(opSymbol);
		//macro
		if (op != nullptr && typeid(*op) == typeid(Macro)) {
			Macro *macro = dynamic_cast<Macro*>(op.get());
			EnvSPtr env = makeEnvForMacro(EnvSPtr(self), macro->env,
																		macro->parameterList, cons->cdr);
			return macroexpandAll(env->eval(macro->body));
		}
	}
	return map(objPtr, [this](LobjSPtr objPtr) {
			return this->macroexpandAll(objPtr);
		});
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
	} else if (opName == "def") {
		if (length == 3) {
			LobjSPtr variable = listNth(objPtr, 1);
			if (typeid(*variable) != typeid(Symbol))
				throw "bad 'def'";
			Symbol *symbol = dynamic_cast<Symbol*>(variable.get());
			EnvSPtr env = rootEnv();
			env->bind(eval(listNth(objPtr, 2)), symbol);
			return variable;
		}
	} else if (opName == "set!") {
		if (length == 3) {
			LobjSPtr variable = listNth(objPtr, 1);
			if (typeid(*variable) != typeid(Symbol))
				throw "bad 'set!'";
			Symbol *symbol = dynamic_cast<Symbol*>(variable.get());
			EnvSPtr env = resolveEnv(symbol);
			if (env == nullptr) env = rootEnv();
			env->bind(eval(listNth(objPtr, 2)), symbol);
			return variable;
		}
	} else if (opName == "let") {
		if (length < 2) throw "bad let";

		LobjSPtr bindings = listNth(objPtr, 1);
		if (!isProperList(bindings.get())) throw "bad let bindings";
		if (listLength(bindings.get()) % 2 != 0) throw "number of bindings elements of let is odd.";
		EnvSPtr env = makeInnerEnv(EnvSPtr(self));
		while (!bindings->isNil()) {
			LobjSPtr objSymbol = dynamic_cast<Cons*>(bindings.get())->car;
			LobjSPtr objForm = dynamic_cast<Cons*>(dynamic_cast<Cons*>(bindings.get())->cdr.get())->car;
			// TODO type check
			Symbol *symbol = dynamic_cast<Symbol*>(objSymbol.get());
			env->bind(eval(objForm), symbol);
			bindings = listNthCdr(bindings, 2);
		}
		return env->eval(LobjSPtr(new Cons(intern("do"), listNthCdr(objPtr, 2))));

	} else if (opName == "\\") {
		if (2 <= length) {
			LobjSPtr pl = listNth(objPtr, 1);
			//if (!isProperList(pl.get())) throw "bad lambda form";
			return LobjSPtr(new Proc(pl, LobjSPtr(new Cons(intern("do"), listNthCdr(objPtr, 2))), EnvSPtr(self)));
		}
	} else if (opName == "macro") {
		if (2 <= length) {
			LobjSPtr pl = listNth(objPtr, 1);
			//if (!isProperList(pl.get())) throw "bad lambda form";
			return LobjSPtr(new Macro(pl, LobjSPtr(new Cons(intern("do"), listNthCdr(objPtr, 2))), EnvSPtr(self)));
		}
	}
	return LobjSPtr(nullptr);
}

LobjSPtr Env::eval(LobjSPtr objPtr) {
	//objPtr->print(std::cout); std::cout << " | ";
	Lobj *o = objPtr.get();
	if (typeid(*o) == typeid(Symbol)) {
		LobjSPtr rr = resolve(dynamic_cast<Symbol*>(o));
		if (rr == nullptr) {
			std::cout << "unbound symbol: " << dynamic_cast<Symbol*>(o)->name << std::endl;
			throw "evaluated unbound symbol";
		}
		return rr;
	}
	if (typeid(*o) == typeid(Int)) {
		return objPtr;
	}
	if (typeid(*o) == typeid(String)) {
		return objPtr;
	}
	if (typeid(*o) == typeid(Cons)) {
		LobjSPtr psfr = procSpecialForm(objPtr);
		if (psfr != nullptr) {
			return psfr;
		}

		Cons *cons = dynamic_cast<Cons*>(o);
		LobjSPtr opPtr = eval(cons->car);
		if (typeid(*opPtr) == typeid(Proc)) {
			Proc *func = dynamic_cast<Proc*>(opPtr.get());
			EnvSPtr env = makeEnvForApply(EnvSPtr(self), func->env,
																		func->parameterList, cons->cdr);
			return env->eval(func->body);
		}
		if (typeid(*opPtr) == typeid(BuiltinProc)) {
			BuiltinProc *bfunc = dynamic_cast<BuiltinProc*>(opPtr.get());
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

std::string initializeCode = "(println \"Loding core file...\" (load \"core.lisp\"))";

int main() {
	EnvSPtr env = Env::makeEnv();

	std::istringstream ss(initializeCode);
	LobjSPtr objPtr = env->read(ss);
	env->eval(env->macroexpandAll(objPtr));

	try {
		env->repl();
	} catch (char const *e) {
		std::cout << "Fatal error: " << e << std::endl;
	}
	return 0;
}
