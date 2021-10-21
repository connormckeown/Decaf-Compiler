%{
#include <iostream>
#include <ostream>
#include <string>
#include <cstdlib>
#include <vector>
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
%type <ast> rvalue method_arg method_arg_list method_call expr assign block
%type <ast> statement_list var_decl var_decl_list identifier_list assign_list
%type <ast> field_decl field_decl_list method method_block method_list method_type_list break_statement continue_statement
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

decafpackage: T_PACKAGE T_ID T_LCB field_decl_list method_list T_RCB
    { $$ = new PackageAST(*$2, (decafStmtList*)$4, (decafStmtList*)$5); delete $2; }
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

assign_list: assign assign_list { decafStmtList* list = (decafStmtList*)$2;
                                  list->push_front($1);
                                  $$ = list; }
    |        assign { decafStmtList* list = new decafStmtList();
                      list->push_front($1);
                      $$ = list; }
    ;

break_statement: T_BREAK T_SEMICOLON { $$ = new BreakStmtAST(); }
    ;

continue_statement: T_CONTINUE T_SEMICOLON { $$ = new ContinueStmtAST(); }
    ;

identifier_list: T_ID T_COMMA identifier_list { IdListAST* list = (IdListAST*)$3;
                                                list->vec.push_back(*$1);
                                                $$ = list;
                                                delete $1; }
    |            T_ID                         { $$ = new IdListAST(*$1);
                                                delete $1; }
    ;

var_decl: T_VAR identifier_list decaf_type T_SEMICOLON { IdListAST* list = (IdListAST*)$2;
                                                         decafStmtList* list2 = new decafStmtList();
                                                         for (vector<string>::iterator it = (*list).vec.begin(); it != (*list).vec.end(); ++it) {
                                                             VarDefAST* var = new VarDefAST(*it, *$3);
                                                             list2->push_front(var);
                                                         }
                                                         $$ = list2;
                                                         delete $2;
                                                         delete $3; }
    ;

var_decl_list: var_decl var_decl_list { decafStmtList* list = new decafStmtList();
                                        list->push_back($1);
                                        list->push_back($2);
                                        $$ = list; }
    |          var_decl { decafStmtList* list = new decafStmtList();
                          list->push_back($1);
                          $$ = list; }
    |          { $$ = new decafStmtList(); }
    ;

field_decl: T_VAR identifier_list decaf_type T_SEMICOLON { IdListAST* list = (IdListAST*)$2; 
                                                           decafStmtList* list2 = new decafStmtList();
                                                           for (vector<string>::iterator it = (*list).vec.begin(); it != (*list).vec.end(); ++it) {
                                                               FieldDeclAST* field = new FieldDeclAST(*it, *$3, "Scalar");
                                                               list2->push_front(field); 
                                                           }
                                                           $$ = list2;
                                                           delete $2;
                                                           delete $3; }
    ;

field_decl_list: field_decl field_decl_list { decafStmtList* list = new decafStmtList();
                                              list->push_back($1);
                                              list->push_back($2);
                                              $$ = list; }
    |            field_decl { decafStmtList* list = new decafStmtList();
                              list->push_back($1);
                              $$ = list; }
    |            { $$ = new decafStmtList(); }
    ;

method_block: T_LCB var_decl_list statement_list T_RCB { $$ = new MethodBlockAST((decafStmtList*)$2, (decafStmtList*)$3); }
    ;

method_type_list: method_type_list T_COMMA T_ID decaf_type { decafStmtList* list = new decafStmtList();
                                                             VarDefAST* var = new VarDefAST(*$3, *$4); 
                                                             list->push_back($1);
                                                             list->push_back(var);
                                                             $$ = list;
                                                             delete $3; }
    |             T_ID decaf_type { decafStmtList* list = new decafStmtList();
                                    VarDefAST* var = new VarDefAST(*$1, *$2);
                                    list->push_back(var);
                                    $$ = list;
                                    delete $1; }
    ;

method: T_FUNC T_ID T_LPAREN method_type_list T_RPAREN method_type method_block { decafStmtList* list = new decafStmtList();
                                                                                  MethodAST* method = new MethodAST(*$2, *$6, (decafStmtList*)$4, (MethodBlockAST*)$7);
                                                                                  list->push_back(method);
                                                                                  $$ = list;
                                                                                  delete $2;
                                                                                  delete $6; }
    ;

method_list: method method_list { decafStmtList* list = new decafStmtList();
                                  list->push_back($1);
                                  list->push_back($2);
                                  $$ = list; }
    |        method { decafStmtList* list = new decafStmtList(); 
                      list->push_back($1);
                      $$ = list; }
    |        { $$ = new decafStmtList(); }
    ;

/* same structure as var_decl_list */
statement_list: statement statement_list { decafStmtList* list = new decafStmtList();
                                        list->push_back($1);
                                        list->push_back($2);
                                        $$ = list; }
    |          statement { decafStmtList* list = new decafStmtList();
                          list->push_back($1);
                          $$ = list; }
    |          { $$ = new decafStmtList(); }
    ;

block: T_LCB var_decl_list statement_list T_RCB { $$ = new BlockAST((decafStmtList*)$2, (decafStmtList*)$3); }
    ;

statement: assign T_SEMICOLON { $$ = $1; }
    |      method_call T_SEMICOLON { $$ = $1; }
    |      T_IF T_LPAREN expr T_RPAREN block { $$ = new IfStmtAST($3, (BlockAST*)$5, NULL); }
    |      T_IF T_LPAREN expr T_RPAREN block T_ELSE block { $$ = new IfStmtAST($3, (BlockAST*)$5, (BlockAST*)$7); }
    |      T_WHILE T_LPAREN expr T_RPAREN block { $$ = new WhileStmtAST($3, (BlockAST*)$5); }
    |      T_FOR T_LPAREN assign_list T_SEMICOLON expr T_SEMICOLON assign_list T_RPAREN block {$$ = new ForStmtAST((AssignVarAST*)$3, $5, (AssignVarAST*)$7, (BlockAST*)$9); }
    |      T_RETURN T_SEMICOLON { $$ = new ReturnStmtAST(NULL); }
    |      T_RETURN T_LPAREN T_RPAREN T_SEMICOLON { $$ = new ReturnStmtAST(NULL); }
    |      T_RETURN T_LPAREN expr T_RPAREN T_SEMICOLON { $$ = new ReturnStmtAST($3); }
    |      break_statement { $$ = $1; }
    |      continue_statement { $$ = $1; }
    |      block { $$ = $1; }
    ;


%%

int main() {
  // parse the input and create the abstract syntax tree
  int retval = yyparse();
  return(retval >= 1 ? EXIT_FAILURE : EXIT_SUCCESS);
}

