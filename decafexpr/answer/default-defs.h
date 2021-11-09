
#ifndef _DECAF_DEFS
#define _DECAF_DEFS

#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/IRBuilder.h"
#include <cstdio> 
#include <cstdlib>
#include <cstring> 
#include <string>
#include <stdexcept>
#include <vector>
#include <list>
#include <map>

extern int lineno;
extern int tokenpos;

using namespace std;

extern "C"
{
	extern int yyerror(const char *);
	int yyparse(void);
	int yylex(void);  
	int yywrap(void);
}

struct struct descriptor {
	int lineno;
	string type;
	llvm::GlobalVariable* p_global;
	llvm::AllocaInst* p_alloc;
	llvm::Function* p_func;
	vector<string> arg_names;
	vector<llvm::Type*> arg_types;
} descriptor;

typedef map<string, descriptor* > symbol_table;
typedef list<symbol_table > symbol_table_list;
extern symbol_table_list symtbl;
extern descriptor* access_symtbl(string id);



#endif

