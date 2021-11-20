%{
#include "default-defs.h"
#include "decafcomp.tab.h"
#include <cstring>
#include <string>
#include <sstream>
#include <iostream>

using namespace std;

int lineno = 1;
int tokenpos = 1;

%}

comment             \/\/.*\n
whitespace          [\t\r\a\v\b ]+
int_lit             [0-9]+

char_lit_chars      [^\\]|\\.
string_lit_chars    [ 0-9a-zA-Z#$&%()~,\*\+-\./:;<>=!\?\[\]_\{\}|]
escaped_char        "\\"("n"|"r"|"t"|"v"|"f"|"a"|"b"|"\\"|"\'"|"\"")

char_lit            "\'"({char_lit_chars}|{escaped_char})"\'"
string_lit          "\""({string_lit_chars}|{escaped_char})*"\""


%%
  /*
    Pattern definitions for all tokens 
  */

{int_lit}                  { yylval.sval = new string(yytext); return T_INTCONSTANT; }
{char_lit}                 { yylval.sval = new string(yytext); return T_CHARCONSTANT; }
{string_lit}               { yylval.sval = new string(yytext); return T_STRINGCONSTANT;}

\{                         { return T_LCB; }
\}                         { return T_RCB; }
\(                         { return T_LPAREN; }
\)                         { return T_RPAREN; }
&&                         { return T_AND; }
==                         { return T_EQ; }
\>=                        { return T_GEQ; }
\>                         { return T_GT; }
\<=                        { return T_LEQ; }
\<                         { return T_LT; }
!=                         { return T_NEQ; }
!                          { return T_NOT; }
\|\|                       { return T_OR; }
\+                         { return T_PLUS; }
-                          { return T_MINUS; }
\*                         { return T_MULT; }
\/                         { return T_DIV; }
\%                         { return T_MOD; }
\=                         { return T_ASSIGN; }
,                          { return T_COMMA; }
\.                         { return T_DOT; }
\<\<                       { return T_LEFTSHIFT; }
\[                         { return T_LSB; }
\]                         { return T_RSB; }
>>                         { return T_RIGHTSHIFT; }
\;                         { return T_SEMICOLON; }

{comment}                  { }
{whitespace}               { }
func                       { return T_FUNC; }
int                        { return T_INTTYPE; }
package                    { return T_PACKAGE; }
bool                       { return T_BOOLTYPE; }
break                      { return T_BREAK; }
continue                   { return T_CONTINUE; }
else                       { return T_ELSE; }
extern                     { return T_EXTERN; }
false                      { return T_FALSE; }
for                        { return T_FOR; }
if                         { return T_IF; }
null                       { return T_NULL; }
return                     { return T_RETURN; }
string                     { return T_STRINGTYPE; }
true                       { return T_TRUE; }
var                        { return T_VAR; }
void                       { return T_VOID; }
while                      { return T_WHILE; }

[a-zA-Z\_][a-zA-Z\_0-9]*   { yylval.sval = new string(yytext); return T_ID; } /* note that identifier pattern must be after all keywords */
[\t\r\n\a\v\b ]+           { } /* ignore whitespace */
.                          { cerr << "Error: unexpected character in input" << endl; return -1; }

%%

int yyerror(const char *s) {
  cerr << lineno << ": " << s << " at char " << tokenpos << endl;
  return 1;
}

