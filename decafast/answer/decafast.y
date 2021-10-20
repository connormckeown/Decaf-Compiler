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

%left T_OR
%left T_AND
%left T_EQ T_NEQ T_GEQ T_LEQ T_LT T_GT
%left T_PLUS T_MINUS
%left T_MULT T_DIV T_MOD T_LEFTSHIFT T_RIGHTSHIFT
%left T_NOT
%left UMINUS

%type <ast> extern_list decafpackage typed_symbol statement constant bool_constant
%type <ast> rvalue method_arg method_arg_list method_call expr assign
%type <sval> decaf_type method_type extern_type

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

decaf_type: T_INTTYPE { $$ = new string("IntType"); }
    |       T_BOOLTYPE { $$ = new string("BoolType"); }
    ;

method_type: T_VOID { $$ = new string("MethodType"); }
    |        decaf_type { $$ = $1; }
    ;

extern_type: T_STRINGTYPE { $$ = new string("StringType"); }
    |        decaf_type { $$ = $1; }
    ;

typed_symbol: T_ID { $$ = new string("identifier name"); }
    |         decaf_type { $$ = $1; }
    ;

bool_constant: T_TRUE { $$ = new ConstantAST(string("BoolExpr"), string("True")); }
    |          T_FALSE { $$ = new ConstantAST(string("BoolExpr"), string("False")); }
    ;

constant: T_INTCONSTANT { $$ = new ConstantAST(string("NumberExpr"), *$1); delete $1; }
    |     T_CHARCONSTANT { $$ = new ConstantAST(string("NumberExpr"), strtoascii(*$1)); delete $1; }
    |     T_STRINGCONSTANT {$$ = new ConstantAST(string("StringConstant"), *$1); delete $1; }
    |     bool_constant { $$ = $1; }
    ;

rvalue: T_ID T_LSB expr T_RSB { $$ = new ArrayLocExprAST(*$1, (decafStmtList*) $3); delete $1; }
    |   T_ID { $$ = new VariableExprAST(*$1); delete $1; }
    ;

method_call: T_ID T_LPAREN T_RPAREN { $$ = new MethodCallAST(*$1, NULL); }
    |        T_ID T_LPAREN method_arg_list T_RPAREN { $$ = new MethodCallAST(*$1, $3); delete $1; }
    ;

method_arg: expr { $$ = $1; }
    |       T_STRINGCONSTANT { $$ = new ConstantAST(string("StringConstant"), *$1); delete $1; }
    ;

method_arg_list: method_arg T_COMMA method_arg_list { decafStmtList* list = (decafStmtList*) $3; 
                                                      list->push_front($1);
                                                      $$ = list; }
    |            method_arg { decafStmtList* list = new decafStmtList();
                              list->push_front($1);
                              $$ = list; }
    ;

/* delete $2 ? */
expr: rvalue { $$ = $1; }
    | method_call { $$ = $1; }
    | constant { $$ = $1; }
    | expr T_MULT expr { $$ = new BinaryExprAST(string("T_MULT"), $1, $3); }
    | expr T_DIV expr { $$ = new BinaryExprAST(string("T_DIV"), $1, $3); }
    | expr T_MOD expr { $$ = new BinaryExprAST(string("T_MOD"), $1, $3); }
    | expr T_PLUS expr { $$ = new BinaryExprAST(string("T_PLUS"), $1, $3); }
    | expr T_MINUS expr { $$ = new BinaryExprAST(string("T_MINUS"), $1, $3); }
    | expr T_LEFTSHIFT expr { $$ = new BinaryExprAST(string("T_LEFTSHIFT"), $1, $3); }
    | expr T_RIGHTSHIFT expr { $$ = new BinaryExprAST(string("T_RIGHTSHIFT"), $1, $3); }
    | expr T_EQ expr { $$ = new BinaryExprAST(string("T_EQ"), $1, $3); }
    | expr T_NEQ expr { $$ = new BinaryExprAST(string("T_NEQ"), $1, $3); }
    | expr T_GEQ expr { $$ = new BinaryExprAST(string("T_GEQ"), $1, $3); }
    | expr T_LEQ expr { $$ = new BinaryExprAST(string("T_LEQ"), $1, $3); }
    | expr T_GT expr { $$ = new BinaryExprAST(string("T_GT"), $1, $3); }
    | expr T_LT expr { $$ = new BinaryExprAST(string("T_LT"), $1, $3); }
    | expr T_AND expr { $$ = new BinaryExprAST(string("T_AND"), $1, $3); }
    | expr T_OR expr { $$ = new BinaryExprAST(string("T_OR"), $1, $3); }
    | T_NOT expr { $$ = new UnaryExprAST(string("T_NOT"), $2); }
    | T_MINUS expr %prec UMINUS { $$ = new UnaryExprAST(string("T_UMINUS"), $2); }
    | T_LPAREN expr T_RPAREN { $$ = $2; }
    ;

assign: T_ID T_ASSIGN expr { $$ = new AssignVarAST(*$1, $3); delete $1; }
    |   T_ID T_LSB expr T_RSB T_ASSIGN expr { $$ = new AssignArrayLocAST(*$1, $3, $6); delete $1; }
    ;




%%

int main() {
  // parse the input and create the abstract syntax tree
  int retval = yyparse();
  return(retval >= 1 ? EXIT_FAILURE : EXIT_SUCCESS);
}

