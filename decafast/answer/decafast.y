%{
#include <iostream>
#include <ostream>
#include <string>
#include <cstdlib>
#include "default-defs.h"

int yylex(void);
int yyerror(char *); 

// print AST?
bool printAST = true;

#include "decafast.cc"

using namespace std;

%}

%define parse.error verbose

%union{
    class decafAST *ast;
    std::string *sval;
 }


%token T_AND T_EQ T_GEQ T_GT T_LEQ T_LT T_NEQ T_NOT T_OR T_PLUS T_MINUS T_MULT T_DIV T_MOD T_ASSIGN
%token T_LCB T_RCB T_LPAREN T_RPAREN T_COMMA T_DOT T_LEFTSHIFT T_LSB T_RSB T_RIGHTSHIFT T_SEMICOLON
%token T_BOOLTYPE T_BREAK T_CONTINUE T_ELSE T_EXTERN T_FALSE T_FOR T_IF T_NULL T_RETURN T_STRINGTYPE
%token T_PACKAGE T_FUNC T_INTTYPE T_TRUE T_VAR T_VOID T_WHILE

%token <sval> T_ID
%token <sval> T_CHARCONSTANT
%token <sval> T_STRINGCONSTANT
%token <sval> T_INTCONSTANT

%type <ast> extern_list decafpackage

%%

start: program

program: extern_list decafpackage
    { 
        ProgramAST *prog = new ProgramAST((decafStmtList *)$1, (PackageAST *)$2); 
		if (printAST) {
			cout << getString(prog) << endl;
		}
        delete prog;
    }

extern_list: /* extern_list can be empty */
    { decafStmtList *slist = new decafStmtList(); $$ = slist; }
    ;

decafpackage: T_PACKAGE T_ID T_LCB T_RCB
    { $$ = new PackageAST(*$2, new decafStmtList(), new decafStmtList()); delete $2; }
    ;

%%

int main() {
  // parse the input and create the abstract syntax tree
  int retval = yyparse();
  return(retval >= 1 ? EXIT_FAILURE : EXIT_SUCCESS);
}

