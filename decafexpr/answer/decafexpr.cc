
#include "default-defs.h"
#include <list>
#include <ostream>
#include <iostream>
#include <sstream>
#include <map>
#include <algorithm>


#ifndef YYTOKENTYPE
#include "decafexpr.tab.h"
#endif

using namespace std;

symbol_table_list symtbl;
llvm::Value* returnValue;

descriptor* access_symtbl(string ident) {
    for (auto i : symtbl) {
        auto find_ident = i.find(ident);
        if (find_ident != i.end()) {
            return find_ident->second;
        }
    }
    return NULL;
}


llvm::Type* getType(string type) {
	if (type == "StringType") { return Builder.getInt8PtrTy(); }
	else if (type == "IntType") { return Builder.getInt32Ty(); }
	else if (type == "VoidType") { return Builder.getVoidTy(); }
	else if (type == "BoolType") { return Builder.getInt1Ty(); }
	return NULL;
}

// https://releases.llvm.org/3.6.0/docs/tutorial/LangImpl8.html
static llvm::AllocaInst *CreateEntryBlockAlloca(llvm::Function *TheFunction, llvm::Type* VarType, const std::string &VarName) {
	llvm::IRBuilder<> TmpB(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());
  	return TmpB.CreateAlloca(VarType, NULL, VarName.c_str());
}


/// decafAST - Base class for all abstract syntax tree nodes.
class decafAST {
public:
  virtual ~decafAST() {}
  virtual string str() { return string(""); }
  virtual llvm::Value *Codegen() = 0;
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

template <class T>
llvm::Value *listCodegen(list<T> vec) {
	llvm::Value *val = NULL;
	for (typename list<T>::iterator i = vec.begin(); i != vec.end(); i++) { 
		llvm::Value *j = (*i)->Codegen();
		if (j != NULL) { val = j; }
	}	
	return val;
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
	list<decafAST *> getList() { return stmts; }
	string str() { return commaList<class decafAST *>(stmts); }
	vector<llvm::Value *> getArgs(){
		vector<llvm::Value *> args;
		for (list<class decafAST *>::iterator i = stmts.begin(); i != stmts.end(); i++){
			args.push_back((*i)->Codegen());
		}
		return args;
	}
	list<decafAST *>::iterator head() { return stmts.begin(); }
	list<decafAST *>::iterator tail() { return stmts.end(); }
	llvm::Value *Codegen() { 
		return listCodegen<decafAST *>(stmts); 
	}
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
	llvm::Value *Codegen() { 
		llvm::Value *val = NULL;
		TheModule->setModuleIdentifier(llvm::StringRef(Name)); 
		if (NULL != FieldDeclList) {
			val = FieldDeclList->Codegen();
		}
		if (NULL != MethodDeclList) {
			list<decafAST *> stmts = MethodDeclList->getList();
			for(list<decafAST*>::iterator it = stmts.begin(); it != stmts.end(); it++){
				MethodAST* method = (MethodAST*)(*it);
				method->func();
			}
			val = MethodDeclList->Codegen();
		} 
		// Q: should we enter the class name into the symbol table?
		return val; 
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
	llvm::Value *Codegen() { 
		llvm::Value *val = NULL;
		if (NULL != ExternList) {
			val = ExternList->Codegen();
		}
		if (NULL != PackageDef) {
			val = PackageDef->Codegen();
		} else {
			throw runtime_error("no package definition in decaf program");
		}
		return val; 
	}
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
	decafStmtList* method_arg_list = NULL;
public:
	MethodCallAST(string name, decafStmtList* method_arg_list) : name(name), method_arg_list(method_arg_list) {}
	~MethodCallAST() {
		if (method_arg_list != NULL) { delete method_arg_list; }
	}
	string str() {
		if (method_arg_list) {
			return string("MethodCall") + "(" + name + "," + getString(method_arg_list) + ")";
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

class IfStmtAST : public decafAST {
	decafAST* condition;
	decafAST* if_block;
	decafAST* else_block;
public:
	IfStmtAST(decafAST* condition, decafAST* if_block, decafAST* else_block) : condition(condition), if_block(if_block), else_block(else_block) {}
	~IfStmtAST() {
		if (condition != NULL) { delete condition; }
		if (if_block != NULL) { delete if_block; }
		if (else_block != NULL) { delete else_block; }
	}
	string str() { 
		if (else_block) {
			return string("IfStmt") + "(" + condition->str() + "," + if_block->str() + "," + else_block->str() + ")";
		} else {
			return string("IfStmt") + "(" + condition->str() + "," + if_block->str() + "," + "None" + ")";
		}
	}
};

class WhileStmtAST : public decafAST {
	decafAST* condition;
	decafAST* while_block;
public:
	WhileStmtAST(decafAST* condition, decafAST* while_block) : condition(condition), while_block(while_block) {}
	~WhileStmtAST() {
		if (condition != NULL) { delete condition; }
		if (while_block != NULL) { delete while_block; }
	}
	string str() { return string("WhileStmt") + "(" + condition->str() + "," + while_block->str() + ")"; }
};

class ForStmtAST : public decafAST {
	decafAST* pre_assign_list;
	decafAST* condition;
	decafAST* loop_assign_list;
	decafAST* for_block;
public:
	ForStmtAST(decafAST* pre_assign_list, decafAST* condition, decafAST* loop_assign_list, decafAST* for_block) 
		: pre_assign_list(pre_assign_list), condition(condition), loop_assign_list(loop_assign_list), for_block(for_block) {}
	~ForStmtAST() {
		if (pre_assign_list != NULL) { delete pre_assign_list; }
		if (condition != NULL) { delete condition; }
		if (loop_assign_list != NULL) { delete loop_assign_list; }
		if (for_block != NULL) { delete for_block; }
	}
	string str() { return string("ForStmt") + "(" + pre_assign_list->str() + "," + condition->str() + "," + loop_assign_list->str() + "," + for_block->str() + ")"; }
};

class ReturnStmtAST : public decafAST {
	decafAST* return_value;
public:
	ReturnStmtAST(decafAST* return_value) : return_value(return_value) {}
	~ReturnStmtAST() {
		if (return_value != NULL) { delete return_value; }
	}
	string str() { 
		if (return_value) {
			return string("ReturnStmt") + "(" + return_value->str() + ")"; 
		} else {
			return string("ReturnStmt") + "(" + "None" + ")";
		}
		
	}
};

class VarDefAST : public decafAST {
	string name;
	string type;
public:
	VarDefAST(string name, string type) : name(name), type(type) {}
	string getName() { return name; }
	string getType() { return type; }
	string str() {
		if (name.compare("extern") != 0) {
			return string("VarDef") + "(" + name + "," + type + ")";
		} else {
			return string("VarDef") + "(" + type + ")"; 
		}
	}
};

class FieldDeclAST : public decafAST {
	string name;
	string type;
	string size;
	ConstantAST* constant;
	
public:
	FieldDeclAST(string name, string type, string size, ConstantAST* constant) : name(name), type(type), size(size), constant(constant) {}
	string str() { 
		if (constant) {
			return string("AssignGlobalVar") + "(" + name + "," + type + "," + getString(constant) + ")";
		} else {
			return string("FieldDecl") + "(" + name + "," + type + "," + size + ")";
		}
	}
};

class MethodBlockAST : public decafAST {
	decafStmtList* var_decl_list;
	decafStmtList* statement_list;
public:
	MethodBlockAST(decafStmtList* var_decl_list, decafStmtList* statement_list) : var_decl_list(var_decl_list), statement_list(statement_list) {}
	~MethodBlockAST() {
		if (var_decl_list != NULL) { delete var_decl_list; }
		if (statement_list != NULL) { delete statement_list; }
	}
	string str() { return string("MethodBlock") + "(" + getString(var_decl_list) + "," + getString(statement_list) + ")"; }
};

class MethodAST : public decafAST {
	string name;
	string type;
	decafStmtList* param_list;
	MethodBlockAST* block;
public:
	MethodAST(string name, string type, decafStmtList* param_list, MethodBlockAST* block)
		: name(name), type(type), param_list(param_list), block(block) {}
	~MethodAST() {
		if (param_list != NULL) { delete param_list; }
		if (block != NULL) { delete block; }
	}
	string str() { return string("Method") + "(" + name + "," + type + "," + getString(param_list) + "," + getString(block) + ")"; }	// param list printed the wrong way
	llvm::Function *func() {
		llvm::Function *p_func;
		list<decafAST*> stmnts;
		llvm::Type *return_type = getType(type);

		if(param_list != NULL){
			stmnts = param_list->getList();
			param_list->Codegen();
		}

		vector<string> arg_names;
		vector<llvm::Type*> arg_types;
		for(list<decafAST*>::iterator it = stmnts.begin(); it != stmnts.end(); it++) {
			VarDefAST* varDef = (VarDefAST*)(*it);
      		llvm::Type* vdtype = getType(varDef->getType());
      		string vdname = varDef->getName(); 
			arg_types.push_back(vdtype);  
      		arg_names.push_back(vdname);
		}

		p_func = llvm::Function::Create(llvm::FunctionType::get(return_type, arg_types, false), llvm::Function::ExternalLinkage, name, TheModule);

		// build descriptor
		descriptor* d = new descriptor;
		d->p_func = p_func;
		d->lineno = lineno;
		d->type = type;
		d->arg_names = arg_names;
		d->arg_types = arg_types;
		(symtbl.front())[name] = d;

		return p_func;
	}
	llvm::Value *Codegen() {
		llvm::Function *p_func;
		list<decafAST*> stmnts;
		llvm::Type *return_type = getType(type);
		descriptor* d = access_symtbl(name);

		if (return_type->isIntegerTy(32)) {
			returnValue = Builder.getInt32(0);
		} else {
			returnValue = Builder.getInt1(1) ;
		}

		if (param_list != NULL) {
			stmnts = param_list->getList();
			param_list->Codegen();
		}

		vector<llvm::Type*> arg_types;
		vector<string> arg_names;
		for(list<decafAST*>:: iterator it = stmnts.begin(); it != stmnts.end(); it++){
			VarDefAST* varDef = (VarDefAST*)(*it);
			llvm::Type* vdtype = getType(varDef->getType());
			string vdname = varDef->getName();
			arg_names.push_back(vdname);
			arg_types.push_back(vdtype);
		}

		// if there isn't a descriptor, create one
		if (d == NULL) {
			p_func = llvm::Function::Create(llvm::FunctionType::get(return_type, arg_types, false), llvm::Function::ExternalLinkage, name, TheModule);

			descriptor* d = new descriptor;
			d->p_func = p_func;
			d->lineno = lineno;
			d->type = type;
			d->arg_names = arg_names;
			d->arg_types = arg_types;
			(symtbl.front())[name] = d;

		} else {
			p_func = d->p_func;
		}

		llvm::BasicBlock *BB = llvm::BasicBlock::Create(TheContext, "entry", p_func);
		Builder.SetInsertPoint(BB);

		unsigned int idx = 0;
		for(llvm::Function::arg_iterator it = p_func->arg_begin(); it != p_func->arg_end(); it++, idx++) {
			descriptor* d = access_symtbl(arg_names[idx]);
			llvm::AllocaInst* p_alloc = d->p_alloc;

			p_alloc = CreateEntryBlockAlloca(p_func, arg_types[idx], arg_names[idx]);
			Builder.CreateStore(&(*it), p_alloc);

			d->p_alloc = p_alloc;
		}

		if (block != NULL) {
			block->Codegen();
		}

		if (return_type->isVoidTy()) {
			Builder.CreateRet(NULL);
		} else {
			Builder.CreateRet(returnValue);
		}

		verifyFunction(*p_func);
		return (llvm::Value*)p_func;
	}
};

class BreakStmtAST : public decafAST {
	string str() { return string("BreakStmt"); }
};

class ContinueStmtAST : public decafAST {
	string str() { return string("ContinueStmt"); }
};

class IdListAST : public decafAST {
public:
	vector<string> vec;
	IdListAST(string name) {
		vec.push_back(name);
	}
	~IdListAST() {}
	string str() { return *(vec.begin()); }
};

class ExternFunctionAST : public decafAST {
	string name;
	string return_type;
	decafStmtList* type_list;
public:
	ExternFunctionAST(string name, string return_type, decafStmtList* type_list) 
		: name(name), return_type(return_type), type_list(type_list) {}
	~ExternFunctionAST() {
		if (type_list != NULL) { delete type_list; }
	}
	string str() { return string("ExternFunction") + "(" + name + "," + return_type + "," + getString(type_list) + ")"; }
};