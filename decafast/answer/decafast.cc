
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
