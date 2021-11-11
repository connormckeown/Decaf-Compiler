%{
#include <iostream>
#include <ostream>
#include <string>
#include <cstdlib>
#include <vector>
#include <list>
#include <sstream>
#include <map>
#include <algorithm>
#include "default-defs.h"

int yylex(void);
int yyerror(char *); 

// print AST?
bool printAST = false;

using namespace std;

// this global variable contains all the generated code
static llvm::Module *TheModule;

// this is the method used to construct the LLVM intermediate code (IR)
static llvm::LLVMContext TheContext;
static llvm::IRBuilder<> Builder(TheContext);
// the calls to TheContext in the init above and in the
// following code ensures that we are incrementally generating
// instructions in the right order

// dummy main function
// WARNING: this is not how you should implement code generation
// for the main function!
// You should write the codegen for the main method as 
// part of the codegen for method declarations (MethodDecl)
// static llvm::Function *TheFunction = 0;

// we have to create a main function 
// llvm::Function *gen_main_def() {
  // create the top-level definition for main
//  llvm::FunctionType *FT = llvm::FunctionType::get(llvm::IntegerType::get(TheContext, 32), false);
//  llvm::Function *TheFunction = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, "main", TheModule);
//  if (TheFunction == 0) {
//    throw runtime_error("empty function block"); 
//  }
  // Create a new basic block which contains a sequence of LLVM instructions
//  llvm::BasicBlock *BB = llvm::BasicBlock::Create(TheContext, "entry", TheFunction);
  // All subsequent calls to IRBuilder will place instructions in this location
//  Builder.SetInsertPoint(BB);
//  return TheFunction;
// }

#include "decafexpr.cc"

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

%type <ast> extern_list decafpackage statement constant bool_constant
%type <ast> rvalue method_arg method_arg_list method_call expr assign block
%type <ast> statement_list var_decl var_decl_list identifier_list assign_list
%type <ast> field_decl field_decl_list method method_block method_list method_type_list 
%type <ast> break_statement continue_statement extern_def extern_typelist method_arg_list_empty id_type_list
%type <sval> decaf_type method_type extern_type 

%%

start: program

program: extern_list decafpackage
    { 
        ProgramAST *prog = new ProgramAST((decafStmtList *)$1, (PackageAST *)$2); 
		if (printAST) {
			cout << getString(prog) << endl;
		}
        try {
            prog->Codegen();
        } 
        catch (std::runtime_error &e) {
            cout << "semantic error: " << e.what() << endl;
            //cout << prog->str() << endl; 
            exit(EXIT_FAILURE);
        }
        delete prog;
    }
    ;

extern_list: extern_def extern_list { decafStmtList* list; 
                               if ($2 == NULL) {
                                   list = new decafStmtList();
                               } else if ($2 != NULL) {
                                   list = (decafStmtList*)$2;
                               }
                               list->push_front($1);
                               $$ = list;
                               }
    |        { $$ = NULL; }
    ;

extern_typelist: extern_type T_COMMA extern_typelist { decafStmtList* list = (decafStmtList*)$3;
                                                       VarDefAST* var = new VarDefAST(true, string("extern"), *$1);
                                                       list->push_front(var);
                                                       $$ = list;
                                                       delete $1; }
    |            extern_type { decafStmtList* list = new decafStmtList();
                               VarDefAST* var = new VarDefAST(true, string("extern"), *$1);
                               list->push_front(var);
                               $$ = list;
                               delete $1; }
    |                        { decafStmtList* list = new decafStmtList();
                               VarDefAST* varDef = new VarDefAST(true, string("extern"), string("extern"));
                               list->push_front(varDef);
                               $$ = list;}
    ;

extern_def: T_EXTERN T_FUNC T_ID T_LPAREN T_RPAREN method_type T_SEMICOLON { $$ = new ExternFunctionAST(*$3, *$6, new decafStmtList()); 
                                                                             delete $3; }
    |       T_EXTERN T_FUNC T_ID T_LPAREN extern_typelist T_RPAREN method_type T_SEMICOLON { $$ = new ExternFunctionAST(*$3, *$7, (decafStmtList*)$5); 
                                                                                             delete $3;
                                                                                             delete $7; }
    ;

begin_block: T_LCB { symtbl.push_front(symbol_table()); }
    ;

end_block: T_RCB { symbol_table st = symtbl.front();
                   for(symbol_table::iterator it = st.begin(); it != st.end(); it++) {
                        delete(it->second);
                   }
                   symtbl.pop_front(); }
    ;

decafpackage: T_PACKAGE T_ID begin_block field_decl_list method_list end_block
    { $$ = new PackageAST(*$2, (decafStmtList*)$4, (decafStmtList*)$5); delete $2; }
    ;

decaf_type: T_INTTYPE { $$ = new string("IntType"); }
    |       T_BOOLTYPE { $$ = new string("BoolType"); }
    ;

method_type: T_VOID { $$ = new string("VoidType"); }
    |        decaf_type { $$ = $1; }
    ;

extern_type: T_STRINGTYPE { $$ = new string("StringType"); }
    |        decaf_type { $$ = $1; }
    ;

bool_constant: T_TRUE { $$ = new ConstantAST(string("BoolExpr"), string("True")); }
    |          T_FALSE { $$ = new ConstantAST(string("BoolExpr"), string("False")); }
    ;

method_arg: expr { $$ = $1; }
    |       T_STRINGCONSTANT { $$ = new ConstantAST(string("StringConstant"), *$1); 
                               descriptor *d = access_symtbl(*$1);
                               delete $1; }
    ;

constant: T_INTCONSTANT { $$ = new ConstantAST(string("NumberExpr"), *$1); delete $1; }
    |     T_CHARCONSTANT { descriptor* d = access_symtbl(*$1); $$ = new ConstantAST(string("NumberExpr"), strtoascii(*$1)); delete $1; }
    |     bool_constant { $$ = $1; }
    ;

rvalue: T_ID T_LSB expr T_RSB { descriptor *d = access_symtbl(*$1); $$ = new ArrayLocExprAST(*$1, (decafStmtList*) $3); delete $1; }
    |   T_ID { descriptor *d = access_symtbl(*$1); $$ = new VariableExprAST(*$1); delete $1; }
    ;

/* T_ID T_LPAREN T_RPAREN { $$ = new MethodCallAST(*$1, NULL); } */
method_call: T_ID T_LPAREN method_arg_list_empty T_RPAREN { $$ = new MethodCallAST(*$1, (decafStmtList*)$3); delete $1; }
    ;


// method_arg_list? or method_arg_list_empty? as last arg
method_arg_list: method_arg T_COMMA method_arg_list { decafStmtList* list = (decafStmtList*) $3; 
                                                      list->push_front($1);
                                                      $$ = list; }
    |            method_arg { decafStmtList* list = new decafStmtList();
                              list->push_front($1);
                              $$ = list; }
    ;

method_arg_list_empty: method_arg_list { $$ = $1; }
    |                  { $$ = NULL; }
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
    | T_MINUS expr %prec UMINUS { $$ = new UnaryExprAST(string("T_UMINUS"), $2); }
    | T_NOT expr { $$ = new UnaryExprAST(string("T_NOT"), $2); }
    | T_LPAREN expr T_RPAREN { $$ = $2; }
    ;

assign: T_ID T_ASSIGN expr { $$ = new AssignVarAST(*$1, $3); delete $1; }
    |   T_ID T_LSB expr T_RSB T_ASSIGN expr { $$ = new AssignArrayLocAST(*$1, $3, $6); delete $1; }
    ;

assign_list: assign T_COMMA assign_list { decafStmtList* list = (decafStmtList*)$3;
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

var_decl_list: var_decl var_decl_list { decafStmtList* list;
                                        if ($2) {
                                            list = (decafStmtList*)$2;
                                        } else {
                                            list = new decafStmtList();
                                        }
                                        list->push_front($1);
                                        $$ = list; }
    |          { $$ = NULL; }
    ;

var_decl: T_VAR identifier_list decaf_type T_SEMICOLON { IdListAST* list = (IdListAST*)$2;
                                                         decafStmtList* list2 = new decafStmtList();
                                                         for (vector<string>::iterator it = (*list).vec.begin(); it != (*list).vec.end(); ++it) {
                                                             VarDefAST* var = new VarDefAST(false, (*it), *$3);
                                                             list2->push_front(var);
                                                         }
                                                         $$ = list2; }
    ;


field_decl_list: field_decl field_decl_list { decafStmtList* list;
                                              if ($2) {
                                                  list = (decafStmtList*)$2;
                                              } else {
                                                  list = new decafStmtList();
                                              }
                                              list->push_front($1);
                                              $$ = list; }
    |            { $$ = NULL; }
    ;

field_decl: T_VAR identifier_list decaf_type T_SEMICOLON { IdListAST* list = (IdListAST*)$2; 
                                                           decafStmtList* list2 = new decafStmtList();
                                                           for (vector<string>::iterator it = (*list).vec.begin(); it != (*list).vec.end(); ++it) {
                                                               FieldDeclAST* field = new FieldDeclAST((*it), *$3, "Scalar", NULL);
                                                               list2->push_front(field); 
                                                           }
                                                           $$ = list2;
                                                           delete $2;
                                                           delete $3; }
    |       T_VAR identifier_list T_LSB T_INTCONSTANT T_RSB decaf_type T_SEMICOLON { IdListAST* list = (IdListAST*)$2; 
                                                                                     decafStmtList* list2 = new decafStmtList();
                                                                                     for (vector<string>::iterator it = (*list).vec.begin(); it != (*list).vec.end(); ++it) {
                                                                                         FieldDeclAST* field = new FieldDeclAST((*it), *$6, "Array(" + string(*$4) + ")", NULL);
                                                                                         list2->push_front(field); 
                                                                                     }
                                                                                     $$ = list2;
                                                                                     delete $2;
                                                                                     delete $6; }
    |       T_VAR identifier_list decaf_type T_ASSIGN constant T_SEMICOLON { IdListAST* list = (IdListAST*)$2; 
                                                                             decafStmtList* list2 = new decafStmtList();
                                                                             for (vector<string>::iterator it = (*list).vec.begin(); it != (*list).vec.end(); ++it) {
                                                                                 FieldDeclAST* field = new FieldDeclAST((*it), *$3, "", (ConstantAST*)$5);
                                                                                 list2->push_front(field); 
                                                                             }
                                                                             $$ = list2;
                                                                             delete $2;
                                                                             delete $3; }
    ;


method_block: T_LCB var_decl_list statement_list T_RCB { $$ = new MethodBlockAST((decafStmtList*)$2, (decafStmtList*)$3); }
    ;


method_type_list:   { $$ = NULL; }
    |               id_type_list { $$ = $1; }
    ;

id_type_list:   T_ID decaf_type T_COMMA id_type_list { VarDefAST* varDef = new VarDefAST(true, *$1, *$2);
                                                       ((decafStmtList*)$4)->push_front(varDef);
                                                       $$ = $4;
                                                       delete $1; }
    |           T_ID decaf_type { decafStmtList* list = new decafStmtList();
                                  VarDefAST* varDef = new VarDefAST(true, *$1, *$2);
                                  list->push_front(varDef);
                                  delete $1;
                                  $$ = list; }
    ;

method: T_FUNC T_ID T_LPAREN method_type_list T_RPAREN method_type method_block { decafStmtList* list = new decafStmtList();
                                                                                  MethodAST* method = new MethodAST(*$2, *$6, (decafStmtList*)$4, (MethodBlockAST*)$7);
                                                                                  list->push_front(method);
                                                                                  delete $2;
                                                                                  delete $6;
                                                                                  $$ = (decafAST*)method; }
                                                                                  // $$ = list;
                                                                                  // delete $2;
                                                                                  // delete $6; }
    ;

method_list: { $$ = NULL; }
    |        method method_list { decafStmtList* list;
                                  if ($2) {
                                      list = (decafStmtList*)$2;
                                  } else {
                                      list = new decafStmtList();
                                  }
                                  list->push_front($1);
                                  $$ = list; }      
    ;

/* same structure as var_decl_list */
statement_list: statement statement_list { decafStmtList* list;
                                           if ($2) {
                                               list = (decafStmtList*)$2;
                                           } else {
                                               list = new decafStmtList();
                                           }
                                           list->push_front($1);
                                           $$ = list; }
    |           { $$ = NULL; }
    ;

block: begin_block var_decl_list statement_list end_block { $$ = new BlockAST((decafStmtList*)$2, (decafStmtList*)$3); }
    ;

statement: assign T_SEMICOLON { $$ = $1; }
    |      method_call T_SEMICOLON { $$ = $1; }
    |      T_IF T_LPAREN expr T_RPAREN block T_ELSE block { $$ = new IfStmtAST($3, (BlockAST*)$5, (BlockAST*)$7); }
    |      T_IF T_LPAREN expr T_RPAREN block { $$ = new IfStmtAST($3, (BlockAST*)$5, NULL); }
    |      T_WHILE T_LPAREN expr T_RPAREN block { $$ = new WhileStmtAST($3, (BlockAST*)$5); }
    |      T_FOR T_LPAREN assign_list T_SEMICOLON expr T_SEMICOLON assign_list T_RPAREN block {$$ = new ForStmtAST((AssignVarAST*)$3, $5, (AssignVarAST*)$7, (BlockAST*)$9); }
    |      T_RETURN T_LPAREN expr T_RPAREN T_SEMICOLON { $$ = new ReturnStmtAST($3); }
    |      T_RETURN T_LPAREN T_RPAREN T_SEMICOLON { $$ = new ReturnStmtAST(NULL); }
    |      T_RETURN T_SEMICOLON { $$ = new ReturnStmtAST(NULL); }
    |      break_statement { $$ = $1; }
    |      continue_statement { $$ = $1; }
    |      block { $$ = $1; }
    ;


%%

int main() {
  // initialize LLVM
  llvm::LLVMContext &Context = TheContext;

  // Make the module, which holds all the code.
  TheModule = new llvm::Module("Test", Context);
  
  // set up symbol table
  symtbl.push_front(symbol_table());
  // set up dummy main function
  // TheFunction = gen_main_def();
  // parse the input and create the abstract syntax tree
  int retval = yyparse();
  // remove symbol table
  // Finish off the main function. (see the WARNING above)
  // return 0 from main, which is EXIT_SUCCESS
  // Builder.CreateRet(llvm::ConstantInt::get(TheContext, llvm::APInt(32, 0)));
  // Validate the generated code, checking for consistency.
  // verifyFunction(*TheFunction);

  symbol_table symtbl_front = symtbl.front();

  // free head
  for (symbol_table::iterator it = symtbl_front.begin(); it != symtbl_front.end(); it++) {
    delete(it->second);
  }

  symtbl.pop_front(); 
  
  // Print out all of the generated code to stderr
  TheModule->print(llvm::errs(), nullptr);
  return(retval >= 1 ? EXIT_FAILURE : EXIT_SUCCESS);
}

