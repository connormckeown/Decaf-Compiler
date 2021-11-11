
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

int strtoint(string s){
	int result;
	stringstream stringStream;
	if (s.find("x") != string::npos) {
		stringStream << hex << s;
	} else {
		stringStream << s;
	}
	stringStream >> result;
	return result;
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
	vector<llvm::Value *> getArgs() {
		vector<llvm::Value *> args;
		for (list<class decafAST *>::iterator i = stmts.begin(); i != stmts.end(); i++){
			args.push_back((*i)->Codegen());
		}
		return args;
	}
	list<decafAST *>::iterator begin() { return stmts.begin(); }
	list<decafAST *>::iterator end() { return stmts.end(); }
	llvm::Value *Codegen() { 
		return listCodegen<decafAST *>(stmts); 
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
	llvm::Value *Codegen() {
		symtbl.push_front(symbol_table());

		if (var_decl_list != NULL) { var_decl_list->Codegen(); }
		if (statement_list != NULL) { statement_list->Codegen(); }

		symbol_table st = symtbl.front();
		for (symbol_table::iterator it = st.begin(); it != st.end(); it++) {
			delete(it->second);
		}
		symtbl.pop_front();

		return NULL;
	}
};

class ConstantAST : public decafAST {
	string type;
	string val;
public:
	ConstantAST(string type, string val) : type(type), val(val) {}
	string str() { return type + "(" + val + ")"; }
	llvm::Value *Codegen() { 
		llvm::Constant *Const = NULL;

		if (type == "NumberExpr") { 
			Const = Builder.getInt32(strtoint(val));

		} else if (type == "BoolExpr") {
			if (val == "True") { Const = Builder.getInt1(1); }
			if (val == "False") { Const = Builder.getInt1(0); }

			return (llvm::Value*)Const;

		} else if (type == "StringConstant") {
			string s = "";

			for (int i = 1; i < val.length()-1; i++) {
				if (val[i] != '\\') {
					s.push_back(val[i]);
				}
				else {
					switch(val[i+1]){
						case 'a':  s.push_back('\a'); break;
      					case 'b':  s.push_back('\b'); break;
      					case 't':  s.push_back('\t'); break;
						case 'n':  s.push_back('\n'); break;
						case 'v':  s.push_back('\v'); break;
						case 'f':  s.push_back('\f'); break;
						case 'r':  s.push_back('\r'); break;
						case '\\': s.push_back('\\'); break;
						case '\'': s.push_back('\''); break;
						case '\"': s.push_back('\"'); break;
					}
					i++;
				}
			}
			llvm::GlobalVariable *GV = Builder.CreateGlobalString(s.c_str(), "globalstring");
			return Builder.CreateConstGEP2_32(GV->getValueType(), GV, 0, 0, "cast");
		}

		return (llvm::Value*)Const;
	}
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
	llvm::Value *Codegen() {
		llvm::Value* lval = LHS->Codegen();
		llvm::Value* rval = RHS->Codegen();

		if(op.compare("T_MULT") == 0) { return Builder.CreateMul(lval, rval, "multmp"); }
		if(op.compare("T_DIV") == 0) { return Builder.CreateSDiv(lval, rval, "divtmp"); }
		if(op.compare("T_MOD") == 0) { return Builder.CreateSRem(lval, rval, "modtmp"); }
		if(op.compare("T_PLUS") == 0) { return Builder.CreateAdd(lval, rval, "addtmp"); }
		if(op.compare("T_MINUS") == 0) { return Builder.CreateSub(lval, rval, "subtmp"); }
		if(op.compare("T_LEFTSHIFT") == 0) { return Builder.CreateShl(lval, rval, "lstmp"); }
		if(op.compare("T_RIGHTSHIFT") == 0) { return Builder.CreateLShr(lval, rval, "rstmp"); }
		if(op.compare("T_EQ") == 0) { return Builder.CreateICmpEQ(lval, rval, "eqtmp"); }
		if(op.compare("T_NEQ") == 0) { return Builder.CreateICmpNE(lval, rval, "neqtmp");}
		if(op.compare("T_GEQ") == 0) { return Builder.CreateICmpSGE(lval, rval, "geqtmp"); }
		if(op.compare("T_LEQ") == 0) { return Builder.CreateICmpSLE(lval, rval, "leqtmp"); }
		if(op.compare("T_GT") == 0) { return Builder.CreateICmpSGT(lval, rval, "gttmp"); }
		if(op.compare("T_LT") == 0) { return Builder.CreateICmpSLT(lval, rval, "lttmp"); }
		if(op.compare("T_AND") == 0) { return Builder.CreateAnd(lval, rval, "andtmp"); }
		if(op.compare("T_OR") == 0) { return Builder.CreateOr(lval, rval, "ortmp"); }

		return NULL;
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

	llvm::Value *Codegen() {
	  	llvm::Value* lval = LHS->Codegen();

		if (op.compare("T_NOT") == 0) { return Builder.CreateNot(lval, "unottmp"); }
		if (op.compare("T_UMINUS") == 0) { return Builder.CreateNeg(lval, "unegtmp"); }

		return NULL;
  	}
};

class VariableExprAST : public decafAST {
	string name;
public:
	VariableExprAST(string name) : name(name) {}
	string str() { return string("VariableExpr") + "(" + name + ")"; }
	llvm::Value *Codegen() {
		descriptor* d = access_symtbl(name);
		return Builder.CreateLoad(d->p_alloc);
	}
};

class ArrayLocExprAST : public decafAST {
	string name;
	decafStmtList* index;
public:
	ArrayLocExprAST(string name, decafStmtList* index) : name(name), index(index) {}
	string str() { return string("ArrayLocExpr") + "(" + name + "," + getString(index) + ")"; }
	llvm::Value *Codegen() {
		descriptor* d = access_symtbl(name);
		return Builder.CreateLoad(d->p_alloc);
	}
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
	llvm::Value *Codegen() {
		llvm::Function *p_func = TheModule->getFunction(name);

		assert(p_func != NULL);

		std::vector<llvm::Value *> args;
        for (auto it = method_arg_list->begin(); it != method_arg_list->end(); it++) {
            args.push_back((*it)->Codegen());
            if (!args.back()) { return NULL; }
        }

		int idx = 0;
        for (auto it = p_func->arg_begin(); it != p_func->arg_end(); it++) {
            if (it->getType()->isIntegerTy(32) && args[idx]->getType()->isIntegerTy(1)) {
                args[idx] = Builder.CreateIntCast(args[idx], Builder.getInt32Ty(), false);
            }
            idx++;
        }

		if (p_func->getReturnType()->isVoidTy()){
            return Builder.CreateCall(p_func, args);
        } else {
			return Builder.CreateCall(p_func, args, "calltmp");
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
	llvm::Value *Codegen() {
		descriptor *d = access_symtbl(name);
		llvm::AllocaInst *p_alloc = d->p_alloc;
		llvm::Value *value = val->Codegen();

		if ((value->getType()->isIntegerTy(1) == true) && (p_alloc->getType()->isIntegerTy(32) == true)) {
			value = Builder.CreateZExt(value, Builder.getInt32Ty(), "zexttmp");
		}

		if (p_alloc->getType() == value->getType()->getPointerTo()) {
			return Builder.CreateStore(value, p_alloc);
		}

		return NULL;
	}
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
	llvm::Value *Codegen() { return NULL; } 
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
	llvm::Value *Codegen() { return NULL; }
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
	llvm::Value *Codegen() { return NULL; }
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
	llvm::Value *Codegen() { return NULL; }
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
	llvm::Value *Codegen() { return NULL; }
};

class VarDefAST : public decafAST {
	bool param;
	string name;
	string type;
public:
	VarDefAST(bool param, string name, string type) : param(param), name(name), type(type) {}
	string getName() { return name; }
	string getVarType() { return type; }
	string str() {
		if (name.compare("extern") != 0) {
			return string("VarDef") + "(" + name + "," + type + ")";
		} else {
			return string("VarDef") + "(" + type + ")"; 
		}
	}
	llvm::Value *Codegen() {
		if (name.empty()) { return NULL; }

		llvm::Type *llvm_type = getType(type);
		
		llvm::AllocaInst *p_alloc = NULL;

		if (param == false) {
			p_alloc = Builder.CreateAlloca(llvm_type, NULL, name);
		}

		descriptor* d = new descriptor;
		d->type = type;
		d->lineno = lineno;
		d->p_alloc = p_alloc;
		(symtbl.front())[name] = d;

		return (llvm::Value*)p_alloc;
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
	llvm::Value *Codegen() {
		llvm::Constant* Initializer;
		llvm::Type* llvm_type = getType(type);

		if (constant) {
			Initializer = (llvm::Constant*)constant->Codegen();
		} else {
			if (llvm_type->isIntegerTy(32)) {
				Initializer = Builder.getInt32(0);
			} else if (llvm_type->isVoidTy()) {
				Initializer = NULL;
			} else if (llvm_type->isIntegerTy(1)) {
				Initializer = Builder.getInt1(0);
			}
		}
		

		llvm::GlobalVariable *GV = new llvm::GlobalVariable(*TheModule, llvm_type, false, llvm::GlobalValue::InternalLinkage, Initializer, name);

		descriptor* d = new descriptor;
		d->lineno = lineno;
		d->type = type;
		d->p_global = GV;

		(symtbl.front())[name] = d;
		return GV;
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
	llvm::Value *Codegen() {
		symtbl.push_front(symbol_table());
		if(var_decl_list != NULL) { var_decl_list->Codegen(); }
		if(statement_list != NULL) { statement_list->Codegen(); }

		symbol_table st = symtbl.front();
		for (symbol_table::iterator it = st.begin(); it != st.end(); it++){
			delete(it->second);
		}

		symtbl.pop_front();
		return NULL;
	}
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
		llvm::Type *return_type; 
		return_type = getType(type);

		assert(return_type != NULL);

		if (param_list != NULL) {
			stmnts = param_list->getList();
			param_list->Codegen();
		}

		vector<string> arg_names;
		vector<llvm::Type*> arg_types;
		for(list<decafAST*>::iterator it = stmnts.begin(); it != stmnts.end(); it++) {
			VarDefAST* varDef = (VarDefAST*)(*it);
      		llvm::Type* vdtype = getType(varDef->getVarType());
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
			llvm::Type* vdtype = getType(varDef->getVarType());
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


class BreakStmtAST : public decafAST {
	string str() { return string("BreakStmt"); }
	llvm::Value *Codegen() { return NULL; }
};

class ContinueStmtAST : public decafAST {
	string str() { return string("ContinueStmt"); }
	llvm::Value *Codegen() { return NULL; }
};

class IdListAST : public decafAST {
public:
	vector<string> vec;
	IdListAST(string name) {
		vec.push_back(name);
	}
	~IdListAST() {}
	string str() { return *(vec.begin()); }
	llvm::Value *Codegen() { return NULL; }
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
	llvm::Value *Codegen() {
		llvm::Type *ret_type = getType(return_type);
		std::vector<llvm::Type*> args;

		if (type_list != NULL) {
			list<decafAST*> stmts = type_list->getList();
			llvm::Type* retType;

			for (list<decafAST*>::iterator it = stmts.begin(); it != stmts.end(); it++) {
				string type = ((VarDefAST*)(*it))->getVarType();
				if (type.empty()) { 
					args.clear(); 
					break; 
				} else { 
					retType = getType(type);
				}
				args.push_back(retType);
			}
		}

		llvm::Function *p_func = llvm::Function::Create(llvm::FunctionType::get(ret_type, args, false), llvm::Function::ExternalLinkage, name, TheModule);
		verifyFunction(*p_func);
		llvm::Value *val = (llvm::Value*)p_func;

		descriptor* d = new descriptor;
		d->lineno = lineno;
		d->p_func = p_func;
		d->type = return_type;
		d->arg_types = args;

		(symtbl.front())[name] = d;
		return val;
	}
};