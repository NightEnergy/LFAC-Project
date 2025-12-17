
%code requires {
  #include <string>
  #include <vector>
  using namespace std;
}

%{
#include <iostream>
#include "SymTable.h"
extern FILE* yyin;
extern char* yytext;
extern int yylineno;
extern int yylex();
void yyerror(const char * s);
class SymTable* current;
int errorCount = 0;
%}

%union {
     std::string* Str;
     std::vector<std::string*>* StrVec;
}

//%destructor { delete $$; } <Str> 

%token BGIN END ASSIGN NR WHILE EQ NEQ GE LE AND OR PRINT IF ELSE RETURN CLASS NEW DOT

%left '+' '-'
%left '*' '/'

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE
%token<Str> ID TYPE
%type<Str> type_name var
%type<StrVec> list_elem
%start progr
%%
progr :  global_declarations main {if (errorCount == 0) cout<< "The program is correct!" << endl;}
      ;

global_declarations: decl
	              | global_declarations decl    
	              ;

decl : class_def
     | func_def
     | var_decl

type_name : TYPE {
               $$ = $1;
          }
          | ID
          ;

class_def : CLASS ID '{' class_block '}' ';'
          ;

class_block : /*clasa void*/
            | class_block class_member
            ;

class_member : var_decl
             | func_def
             ;

func_def : type_name ID '(' list_param ')' '{' list '}' {
               if(current->existsId($2)) {
                    errorCount++;
                    yyerror("ID already used");
               }
               else {
                    current->addVar($1, $2);
               }
         }
         ;

var_decl : type_name list_elem ';' {
               for(string* varName : *$2) {
                    if(current->existsId(varName)) {
                         errorCount++;
                         yyerror("Variable already exists");
                    }
                    else {
                         current->addVar($1, varName);
                    }
                    delete varName;
               }
               delete $2;
               delete $1;
         }
         ;

list_elem : ID
               {
                    $$ = new vector<string*>();
                    $$->push_back($1);
               }
            | list_elem ',' ID
               {
                    $$ = $1;
                    $$->push_back($3);
               }
            ;

list_param : param
           | list_param ','  param 
           ;

param : /*empty func*/
      | TYPE ID
      ; 
      
main : BGIN main_list END  
     ;

main_list : /*functia void*/
     | main_list main_item
     ;

main_item : statement ';'
     | if_stmt
     | while_loop
     | block
     | RETURN expr ';'
     ;

list : /*functia void*/
     | list item
     ;

item : statement ';'
     | var_decl
     | if_stmt
     | while_loop
     | block
     | RETURN expr ';'
     ;

statement : var ASSIGN expr	 
          | var '(' call_list ')'
          | PRINT '(' expr ')'
          ;

if_stmt : IF '(' condition ')' item %prec LOWER_THAN_ELSE
        | IF '(' condition ')' item ELSE item
        ;

while_loop : WHILE '(' condition ')' item 
     ;

condition : expr sign expr
          ;

sign : '<'
     | '>'
     | EQ
     | NEQ
     | GE
     | LE

block : '{' list '}' 
      ;

expr : var
     | NR
     | call
     | expr '+' expr
     | expr '*' expr
     | expr '-' expr
     | expr '/' expr
     | '(' expr ')'
     ;

call : var '(' call_list ')'
     ;

var : ID {
          if (current->existsId($1)) {
               $$ = $1;
          }
          else {
               errorCount++;
               yyerror("ID is not used");
          }
    }
    | var DOT ID
    ;

call_list : expr
          | call_list ',' expr
          ;
%%
void yyerror(const char * s){
     cout << "error:" << s << " at line: " << yylineno << endl;
}

int main(int argc, char** argv){
     yyin=fopen(argv[1],"r");
     current = new SymTable("global");
     yyparse();
     delete current;
} 