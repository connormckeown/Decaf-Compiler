
#include "default-defs.h"
#include <list>
#include <ostream>
#include <iostream>
#include <sstream>

#ifndef YYTOKENTYPE
#include "decafast.tab.h"
#endif

using namespace std;

/// decafAST - Base class for all abstract syntax tree nodes.
class decafAST {
public:
  virtual ~decafAST() {}
  virtual string str() { return string(""); }
};

string getString(decafAST *d) {
	if (d != NULL) {
		return d->str();
	} else {
		return string("None");
	}
}

// handles ascii escape chars
string strtoascii(string s) {
	int c = s[1];
	if (c == 92) {
		c = s[2];
		if (c == 't') {
			c = 9;
		} else if (c == 'n') {
			c = 10;
		} else if (c == 'v') {
			c = 11;
		} else if (c == 'a') {
			c = 7;
		} else if (c == 'f') {
			c = 12;
		} else if (c == 'r') {
			c = 13;
		} else if (c == 'b') {
			c = 8;
		} 
	}
	return to_string(c);
}

template <class T>
string commaList(list<T> vec) {
    string s("");
    for (typename list<T>::iterator i = vec.begin(); i != vec.end(); i++) { 
        s = s + (s.empty() ? string("") : string(",")) + (*i)->str(); 
    }   
    if (s.empty()) {
        s = string("None");
    }   
    return s;
}

/// decafStmtList - List of Decaf statements
class decafStmtList : public decafAST {
	list<decafAST *> stmts;
public:
	decafStmtList() {}
	~decafStmtList() {
		for (list<decafAST *>::iterator i = stmts.begin(); i != stmts.end(); i++) { 
			delete *i;
		}
	}
	int size() { return stmts.size(); }
	void push_front(decafAST *e) { stmts.push_front(e); }
	void push_back(decafAST *e) { stmts.push_back(e); }
	string str() { return commaList<class decafAST *>(stmts); }
};

class PackageAST : public decafAST {
	string Name;
	decafStmtList *FieldDeclList;
	decafStmtList *MethodDeclList;
public:
	PackageAST(string name, decafStmtList *fieldlist, decafStmtList *methodlist) 
		: Name(name), FieldDeclList(fieldlist), MethodDeclList(methodlist) {}
	~PackageAST() { 
		if (FieldDeclList != NULL) { delete FieldDeclList; }
		if (MethodDeclList != NULL) { delete MethodDeclList; }
	}
	string str() { 
		return string("Package") + "(" + Name + "," + getString(FieldDeclList) + "," + getString(MethodDeclList) + ")";
	}
};

/// ProgramAST - the decaf program
class ProgramAST : public decafAST {
	decafStmtList *ExternList;
	PackageAST *PackageDef;
public:
	ProgramAST(decafStmtList *externs, PackageAST *c) : ExternList(externs), PackageDef(c) {}
	~ProgramAST() { 
		if (ExternList != NULL) { delete ExternList; } 
		if (PackageDef != NULL) { delete PackageDef; }
	}
	string str() { return string("Program") + "(" + getString(ExternList) + "," + getString(PackageDef) + ")"; }
};

// BlockAST
class BlockAST : public decafAST {
	decafStmtList *var_decl_list;
	decafStmtList *statement_list;
public:
	BlockAST(decafStmtList *v, decafStmtList *s) : var_decl_list(v), statement_list(s) {}
	~BlockAST() {
		if (var_decl_list != NULL) { delete var_decl_list; }
		if (statement_list != NULL) { delete statement_list; }
	}
	string str() { return string("Block") + "(" + getString(var_decl_list) + "," + getString(statement_list) + ")"; }
};

class ConstantAST : public decafAST {
	string type;
	string val;
public:
	ConstantAST(string type, string val) : type(type), val(val) {}
	string str() { return type + "(" + val + ")"; }
};

class BinaryExprAST : public decafAST {
	string op;
	decafAST *LHS;
	decafAST *RHS;
public:
	BinaryExprAST(string op, decafAST *LHS, decafAST *RHS) : op(op), LHS(LHS), RHS(RHS) {}
	~BinaryExprAST() {
		if (LHS != NULL) { delete LHS; }
		if (RHS != NULL) { delete RHS; }
	}
	string str() {
		string res = "";
		if (op.compare("T_MULT") == 0) { res = "Mult"; }
		else if (op.compare("T_DIV") == 0) { res = "Div"; }
		else if (op.compare("T_MOD") == 0) { res = "Mod"; }
		else if (op.compare("T_PLUS") == 0) { res = "Plus"; }
		else if (op.compare("T_MINUS") == 0) { res = "Minus"; }
		else if (op.compare("T_LEFTSHIFT") == 0) { res = "Leftshift"; }
		else if (op.compare("T_RIGHTSHIFT") == 0) { res = "Rightshift"; }
		else if (op.compare("T_EQ") == 0) { res = "Eq"; }
		else if (op.compare("T_NEQ") == 0) { res = "Neq"; }
		else if (op.compare("T_GEQ") == 0) { res = "Geq"; }
		else if (op.compare("T_LEQ") == 0) { res = "Leq"; }
		else if (op.compare("T_GT") == 0) { res = "Gt"; }
		else if (op.compare("T_LT") == 0) { res = "Lt"; }
		else if (op.compare("T_AND") == 0) { res = "And"; }
		else if (op.compare("T_OR") == 0) { res = "Or"; }
		
		return string("BinaryExpr") + "(" + res + "," + LHS->str() + "," + RHS->str() + ")";
	}
};

class UnaryExprAST : public decafAST {
	string op;
	decafAST *LHS;
public:
	UnaryExprAST(string op, decafAST *LHS) : op(op), LHS(LHS) {}
	~UnaryExprAST() {
		if (LHS != NULL) { delete LHS; }
	}
	string str() {
		string res = "";
		if (op.compare("T_NOT") == 0) { res = "Not"; }
		else if (op.compare("T_UMINUS") == 0) { res = "UnaryMinus"; }
		
		return string("UnaryExpr") + "(" + res + "," + LHS->str() + ")";
	}
};

class VariableExprAST : public decafAST {
	string name;
public:
	VariableExprAST(string name) : name(name) {}
	string str() { return string("VariableExpr") + "(" + name + ")"; }
};

class ArrayLocExprAST : public decafAST {
	string name;
	decafStmtList* index;
public:
	ArrayLocExprAST(string name, decafStmtList* index) : name(name), index(index) {}
	string str() { return string("ArrayLocExpr") + "(" + name + "," + getString(index) + ")"; }
};

class MethodCallAST : public decafAST {
	string name;
	decafAST *method_arg_list = NULL;
public:
	MethodCallAST(string name, decafAST *method_arg_list) : name(name), method_arg_list(method_arg_list) {}
	~MethodCallAST() {
		if (method_arg_list != NULL) { delete method_arg_list; }
	}
	string str() {
		if (method_arg_list) {
			return string("MethodCall") + "(" + name + "," + method_arg_list->str() + ")";
		} else {
			return string("MethodCall") + "(" + name + "," + "None" + ")";
		}
	}
};

class AssignVarAST : public decafAST {
	string name;
	decafAST* val;
public:
	AssignVarAST(string name, decafAST* val) : name(name), val(val) {}
	~AssignVarAST() {
		if (val != NULL) { delete val; }
	}
	string str() { return string("AssignVar") + "(" + name + "," + getString(val) + ")"; }
};

class AssignArrayLocAST : public decafAST {
	string name;
	decafAST* index;
	decafAST* val;
public:
	AssignArrayLocAST(string name, decafAST* index, decafAST* val) : name(name), index(index), val(val) {}
	~AssignArrayLocAST() {
		if (index != NULL) { delete index; }
		if (val != NULL) { delete val; }
	}
	string str() { return string("AssignArrayLoc") + "(" + name + "," + getString(index) + "," + getString(val) + ")"; }
};