/*
    Definition Section
*/
%{

#include <iostream>
#include <cstdlib>

using namespace std;

int lines = 0;
int pos = 0;

%}


char_lit_chars      [^\\]|\\.
string_lit_chars    [ 0-9a-zA-Z#$&%()~,\*\+-\./:;<>=!\?\[\]_\{\}|]
escaped_char        "\\"("n"|"r"|"t"|"v"|"f"|"a"|"b"|"\\"|"\'"|"\"")

char_lit            "\'"({char_lit_chars}|{escaped_char})"\'"
char_lit_lenerr     \'[a-zA-Z]+\'
char_lit_termerr    \'.
char_lit_zerowerr   \'\'

string_lit          "\""({string_lit_chars}|{escaped_char})*"\""
string_lit_nerr     "\""({string_lit_chars}|{escaped_char}|"\n")*"\""
string_lit_delerr   "\""({string_lit_chars}|{escaped_char})*
string_lit_escerr   \"[\\]+.*\"

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
{char_lit_zerowerr}         { return 57; }
{char_lit_lenerr}           { return 55; }
{char_lit_termerr}          { return 56; }
{string_lit}                { return 49; }
{string_lit_nerr}           { return 52; }
{string_lit_escerr}         { return 54; }
{string_lit_delerr}         { return 53; }
\{                          { return 4; }
\}                          { return 5; }
\(                          { return 6; }
\)                          { return 7; }
[a-zA-Z\_][a-zA-Z\_0-9]*    { return 8; }
[\t\r\a\v\b ]+              { return 9; }
\n+[\t\r\a\v\b ]*\n*        { return 10; }
[0-9]+                      { return 50; }
\/\/.*\n                    { return 51; }
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

.                           { lines++; pos++; cout << "Error: unexpected character in input" << endl << "Lexical error: line " << lines << ", position " << pos << endl; exit(EXIT_FAILURE); }

%%

/*
    Function to concatenate chunks of whitespace into a single T_WHITESPACE token
*/
string concat_whitespace(string lexeme) {
    string whitespace = "";
    for (int i = 0; i < lexeme.size(); i++) {
        if (lexeme[i] == '\n') {
            whitespace += "\\n";
            pos = 0;
            lines++;
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
        pos++;
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
                case 1: cout << "T_FUNC " << lexeme << endl; pos += lexeme.size(); break;
                case 2: cout << "T_INTTYPE " << lexeme << endl; pos += lexeme.size(); break;
                case 3: cout << "T_PACKAGE " << lexeme << endl; pos += lexeme.size(); break;
                case 4: cout << "T_LCB " << lexeme << endl; pos += lexeme.size(); break;
                case 5: cout << "T_RCB " << lexeme << endl; pos += lexeme.size(); break;
                case 6: cout << "T_LPAREN " << lexeme << endl; pos += lexeme.size(); break;
                case 7: cout << "T_RPAREN " << lexeme << endl; pos += lexeme.size(); break;
                case 8: cout << "T_ID " << lexeme << endl; pos += lexeme.size(); break;
                case 9: cout << "T_WHITESPACE " << lexeme << endl; pos += lexeme.size(); break;
                case 10: cout << "T_WHITESPACE " << concat_whitespace(lexeme) << endl; break;
                case 11: cout << "T_AND " << lexeme << endl; pos += lexeme.size(); break;
                case 12: cout << "T_EQ " << lexeme << endl; pos += lexeme.size(); break;
                case 13: cout << "T_GEQ " << lexeme << endl; pos += lexeme.size(); break;
                case 14: cout << "T_GT " << lexeme << endl; pos += lexeme.size(); break;
                case 15: cout << "T_LEQ " << lexeme << endl; pos += lexeme.size(); break;
                case 16: cout << "T_LT " << lexeme << endl; pos += lexeme.size(); break;
                case 17: cout << "T_NEQ " << lexeme << endl; pos += lexeme.size(); break;
                case 18: cout << "T_NOT " << lexeme << endl; pos += lexeme.size(); break;
                case 19: cout << "T_OR " << lexeme << endl; pos += lexeme.size(); break;
                case 20: cout << "T_PLUS " << lexeme << endl; pos += lexeme.size(); break;
                case 21: cout << "T_MINUS " << lexeme << endl; pos += lexeme.size(); break;
                case 22: cout << "T_MULT " << lexeme << endl; pos += lexeme.size(); break;
                case 23: cout << "T_DIV " << lexeme << endl; pos += lexeme.size(); break;
                case 24: cout << "T_MOD " << lexeme << endl; pos += lexeme.size(); break;
                case 25: cout << "T_ASSIGN " << lexeme << endl; pos += lexeme.size(); break;
                case 26: cout << "T_COMMA " << lexeme << endl; pos += lexeme.size(); break;
                case 27: cout << "T_DOT " << lexeme << endl; pos += lexeme.size(); break;
                case 28: cout << "T_LEFTSHIFT " << lexeme << endl; pos += lexeme.size(); break;
                case 29: cout << "T_LSB " << lexeme << endl; pos += lexeme.size(); break;
                case 30: cout << "T_RSB " << lexeme << endl; pos += lexeme.size(); break;
                case 31: cout << "T_RIGHTSHIFT " << lexeme << endl; pos += lexeme.size(); break;
                case 32: cout << "T_SEMICOLON " << lexeme << endl; pos += lexeme.size(); break;
                case 33: cout << "T_BOOLTYPE " << lexeme << endl; pos += lexeme.size(); break;
                case 34: cout << "T_BREAK " << lexeme << endl; pos += lexeme.size(); break;
                case 35: cout << "T_CONTINUE " << lexeme << endl; pos += lexeme.size(); break;
                case 36: cout << "T_ELSE " << lexeme << endl; pos += lexeme.size(); break;
                case 37: cout << "T_EXTERN " << lexeme << endl; pos += lexeme.size(); break;
                case 38: cout << "T_FALSE " << lexeme << endl; pos += lexeme.size(); break;
                case 39: cout << "T_FOR " << lexeme << endl; pos += lexeme.size(); break;
                case 40: cout << "T_IF " << lexeme << endl; pos += lexeme.size(); break;
                case 41: cout << "T_NULL " << lexeme << endl; pos += lexeme.size(); break;
                case 42: cout << "T_RETURN " << lexeme << endl; pos += lexeme.size(); break;
                case 43: cout << "T_STRINGTYPE " << lexeme << endl; pos += lexeme.size(); break;
                case 44: cout << "T_TRUE " << lexeme << endl; pos += lexeme.size(); break;
                case 45: cout << "T_VAR " << lexeme << endl; pos += lexeme.size(); break;
                case 46: cout << "T_VOID " << lexeme << endl; pos += lexeme.size(); break;
                case 47: cout << "T_WHILE " << lexeme << endl; pos += lexeme.size(); break;
                case 48: cout << "T_CHARCONSTANT " << lexeme << endl; pos += lexeme.size(); break;
                case 49: cout << "T_STRINGCONSTANT " << lexeme << endl; pos += lexeme.size(); break;
                case 50: cout << "T_INTCONSTANT " << lexeme << endl; pos += lexeme.size(); break;
                case 51: cout << "T_COMMENT " << lexeme.substr(0, lexeme.size()-1) << "\\n" << endl; pos = 0; lines++; break;
                case 52: lines++; pos++; cout << "Error: newline in string constant" << endl << "Lexical error: line " << lines << ", position " << pos << endl; exit(EXIT_FAILURE);
                case 53: lines++; pos++; cout << "Error: string constant is missing closing delimiter" << endl << "Lexical error: line " << lines << ", position " << pos << endl; exit(EXIT_FAILURE);
                case 54: lines++; pos++; cout << "Error: unknown escape sequence in string constant" << endl << "Lexical error: line " << lines << ", position " << pos << endl; exit(EXIT_FAILURE);
                case 55: lines++; pos++; cout << "Error: char constant length is greater than one" << endl << "Lexical error: line " << lines << ", position " << pos << endl; exit(EXIT_FAILURE);
                case 56: lines++; pos++; cout << "Error: unterminated char constant" << endl << "Lexical error: line " << lines << ", position " << pos << endl; exit(EXIT_FAILURE);
                case 57: lines++; pos++; cout << "Error: char constant has zero width" << endl << "Lexical error: line " << lines << ", position " << pos << endl; exit(EXIT_FAILURE);
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
