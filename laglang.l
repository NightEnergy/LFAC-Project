%{
#include <stdio.h>
#include "laglang.tab.h"
#include <string>
%}
%option noyywrap
%%
"int"|"float"|"char"|"string"|"bool"|"void" {yylval.Str = new std::string(yytext); return TYPE;}
"incepe_progr" {return BGIN;}
"incheie_progr" {return END;}
"cat_timp" {return WHILE;}
"clasa" {return CLASS;}
"daca" {return IF;}
"altfel" {return ELSE;}
"intoarce" {return RETURN;}
"Print" {return PRINT;}
"nou" {return NEW;}
"==" {return EQ;}
"!=" {return NEQ;}
">=" {return GE;}
"<=" {return LE;}
"&&"|"si" {return AND;}
"||"|"sau" {return OR;}
"." {return DOT;}
[a-zA-Z][a-zA-Z0-9]* {yylval.Str = new std::string(yytext); return ID;}
"<-" {return ASSIGN;}
[0-9]+ {return NR;}
[ \t] ;
\n {yylineno++;}
. {return yytext[0];}


