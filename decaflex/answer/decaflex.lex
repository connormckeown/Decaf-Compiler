/*
    Definition Section
*/
%{

#include <iostream>
#include <cstdlib>

using namespace std;

%}


char_lit_chars  [.]
escaped_char    "\\"("n"|"r"|"t"|"v"|"f"|"a"|"b"|"\\"|"\'"|"\"")
char_lit        "\'"({char_lit_chars}|{escaped_char})"\'"



/*
    Rules Section
*/
%%

func                        { return 1; }
int                         { return 2; }
package                     { return 3; }
bool                        { return 33; }
break                       { return 34; }
continue                    { return 35; }
else                        { return 36; }
extern                      { return 37; }
false                       { return 38; }
for                         { return 39; }
if                          { return 40; }
null                        { return 41; }
return                      { return 42; }
string                      { return 43; }
true                        { return 44; }
var                         { return 45; }
void                        { return 46; }
while                       { return 47; }
{char_lit}                  { return 48; }
\{                          { return 4; }
\}                          { return 5; }
\(                          { return 6; }
\)                          { return 7; }
[a-zA-Z\_][a-zA-Z\_0-9]*    { return 8; }
[\t\r\a\v\b ]+              { return 9; }
\n+[\t\r\a\v\b ]*\n*        { return 10; }
&&                          { return 11; }
==                          { return 12; }
\>=                         { return 13; }
\>                          { return 14; }
\<=                         { return 15; }
\<                          { return 16; }
!=                          { return 17; }
!                           { return 18; }
\|\|                        { return 19; }
\+                          { return 20; }
-                           { return 21; }
\*                          { return 22; }
\/                          { return 23; }
\%                          { return 24; }
\=                          { return 25; }
,                           { return 26; }
\.                          { return 27; }
\<\<                        { return 28; }
\[                          { return 29; }
\]                          { return 30; }
>>                          { return 31; }
\;                          { return 32; }

.                           { cerr << "Error: unexpected character in input" << endl; return -1; }

%%

/*
    Function to concatenate chunks of whitespace into a single T_WHITESPACE token
*/
string concat_whitespace(string lexeme) {
    string whitespace = "";
    for (int i = 0; i < lexeme.size(); i++) {
        if (lexeme[i] == '\n') {
            whitespace += "\\n";
        } else if (lexeme[i] == '\t') {
            whitespace += "\t";
        } else if (lexeme[i] == '\r') {
            whitespace += "\r";
        } else if (lexeme[i] == '\v') {
            whitespace += "\v";
        } else if (lexeme[i] == '\b') {
            whitespace += "\b";
        } else if (lexeme[i] == ' ') {
            whitespace += " ";
        }
    }
    return whitespace;
}



int main () {
    int token;
    string lexeme;
    while ((token = yylex())) {
        if (token > 0) {
            lexeme.assign(yytext);
            switch(token) {
                case 1: cout << "T_FUNC " << lexeme << endl; break;
                case 2: cout << "T_INTTYPE " << lexeme << endl; break;
                case 3: cout << "T_PACKAGE " << lexeme << endl; break;
                case 4: cout << "T_LCB " << lexeme << endl; break;
                case 5: cout << "T_RCB " << lexeme << endl; break;
                case 6: cout << "T_LPAREN " << lexeme << endl; break;
                case 7: cout << "T_RPAREN " << lexeme << endl; break;
                case 8: cout << "T_ID " << lexeme << endl; break;
                case 9: cout << "T_WHITESPACE " << lexeme << endl; break;
                case 10: cout << "T_WHITESPACE " << concat_whitespace(lexeme) << endl; break;
                case 11: cout << "T_AND " << lexeme << endl; break;
                case 12: cout << "T_EQ " << lexeme << endl; break;
                case 13: cout << "T_GEQ " << lexeme << endl; break;
                case 14: cout << "T_GT " << lexeme << endl; break;
                case 15: cout << "T_LEQ " << lexeme << endl; break;
                case 16: cout << "T_LT " << lexeme << endl; break;
                case 17: cout << "T_NEQ " << lexeme << endl; break;
                case 18: cout << "T_NOT " << lexeme << endl; break;
                case 19: cout << "T_OR " << lexeme << endl; break;
                case 20: cout << "T_PLUS " << lexeme << endl; break;
                case 21: cout << "T_MINUS " << lexeme << endl; break;
                case 22: cout << "T_MULT " << lexeme << endl; break;
                case 23: cout << "T_DIV " << lexeme << endl; break;
                case 24: cout << "T_MOD " << lexeme << endl; break;
                case 25: cout << "T_ASSIGN " << lexeme << endl; break;
                case 26: cout << "T_COMMA " << lexeme << endl; break;
                case 27: cout << "T_DOT " << lexeme << endl; break;
                case 28: cout << "T_LEFTSHIFT " << lexeme << endl; break;
                case 29: cout << "T_LSB " << lexeme << endl; break;
                case 30: cout << "T_RSB " << lexeme << endl; break;
                case 31: cout << "T_RIGHTSHIFT " << lexeme << endl; break;
                case 32: cout << "T_SEMICOLON " << lexeme << endl; break;
                case 33: cout << "T_BOOLTYPE " << lexeme << endl; break;
                case 34: cout << "T_BREAK " << lexeme << endl; break;
                case 35: cout << "T_CONTINUE " << lexeme << endl; break;
                case 36: cout << "T_ELSE " << lexeme << endl; break;
                case 37: cout << "T_EXTERN " << lexeme << endl; break;
                case 38: cout << "T_FALSE " << lexeme << endl; break;
                case 39: cout << "T_FOR " << lexeme << endl; break;
                case 40: cout << "T_IF " << lexeme << endl; break;
                case 41: cout << "T_NULL " << lexeme << endl; break;
                case 42: cout << "T_RETURN " << lexeme << endl; break;
                case 43: cout << "T_STRINGTYPE " << lexeme << endl; break;
                case 44: cout << "T_TRUE " << lexeme << endl; break;
                case 45: cout << "T_VAR " << lexeme << endl; break;
                case 46: cout << "T_VOID " << lexeme << endl; break;
                case 47: cout << "T_WHILE " << lexeme << endl; break;
                case 48: cout << "T_CHARCONSTANT " << lexeme << endl; break;
                default: exit(EXIT_FAILURE);
            }
        } else {
            if (token < 0) {
                exit(EXIT_FAILURE);
            }
        }
    }
    exit(EXIT_SUCCESS);
}
