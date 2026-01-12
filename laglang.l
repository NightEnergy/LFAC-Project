%{
#include <iostream>
#include <string>
#include "SymTable.h"
#include "AST.h"
#include "laglang.tab.h"
using namespace std;
%}

%option noyywrap
%option yylineno

%%

"int"|"float"|"string"|"bool"|"void" { yylval.strVal = new string(yytext); return TYPE; }
"begin_progr" { return BGIN; }
"end_progr" { return END; }
"class" { return CLASS; }
"if" { return IF; }
"else" { return ELSE; }
"while" { return WHILE; }
"return" { return RETURN; }
"Print" { return PRINT; }
"new" { return NEW; }
"<-" { return ASSIGN; }
"." { return DOT; }

[0-9]+"."[0-9]+ { 
    yylval.valVal = new Value();
    yylval.valVal->fVal = atof(yytext); 
    yylval.valVal->type = "float"; 
    return FLOAT_VAL; 
}

[0-9]+ { 
    yylval.valVal = new Value();
    yylval.valVal->iVal = atoi(yytext); 
    yylval.valVal->type = "int"; 
    return INT_VAL; 
}

\"[^\"]*\" { 
    string txt = yytext; 
    yylval.valVal = new Value();
    yylval.valVal->sVal = txt.substr(1, txt.length()-2); 
    yylval.valVal->type = "string"; 
    return STRING_VAL; 
}

"true" { 
    yylval.valVal = new Value();
    yylval.valVal->bVal = true; 
    yylval.valVal->type = "bool"; 
    return BOOL_VAL; 
}

"false" { 
    yylval.valVal = new Value();
    yylval.valVal->bVal = false; 
    yylval.valVal->type = "bool"; 
    return BOOL_VAL; 
}

"//".* { /* Comentariu pe o singura linie */ }

"/*"([^*]|\*+[^*/])*\*+"/" { /* Comentariu pe mai multe linii */ }

[a-zA-Z][a-zA-Z0-9]* { yylval.strVal = new string(yytext); return ID; }

"==" { return EQ; }
"!=" { return NEQ; }
"<=" { return LE; }
">=" { return GE; }
"&&" { return AND; }
"||" { return OR; }

[ \t] ;
\n { }
. { return yytext[0]; }

%%