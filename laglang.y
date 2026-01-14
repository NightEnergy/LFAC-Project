%{
#include <iostream>
#include <vector>
#include <fstream>
#include "SymTable.h"
#include "AST.h"

extern int yylineno;
extern int yylex();
extern FILE* yyin;

void yyerror(const char * s);

SymTable* currentScope;
vector<SymTable*> allTables; 
BlockNode* mainBlock = nullptr;
int errorCount = 0;
ofstream tableOut("tables.txt");

vector<string> tempParamTypes;
vector<string> tempParamNames;
vector<ASTNode*> tempArgs; 
string currentFuncName = "";

void enterScope(string name) {
    SymTable* newScope = new SymTable(name, currentScope);
    if(currentScope) currentScope->children.push_back(newScope);
    allTables.push_back(newScope);
    currentScope = newScope;
}
void exitScope() { currentScope = currentScope->getParent(); }
%}

%union {
    std::string* strVal;
    Value* valVal;
    ASTNode* nodeVal;
    BlockNode* blockVal;
}

%token BGIN END ASSIGN WHILE IF ELSE RETURN CLASS PRINT NEW DOT
%token EQ NEQ GE LE AND OR
%token <strVal> ID TYPE
%token <valVal> INT_VAL FLOAT_VAL STRING_VAL BOOL_VAL

%type <nodeVal> expr variable assignment print_stmt statement if_stmt while_stmt call
%type <blockVal> list_statements block main_list func_body
/* Am eliminat type_name pentru a rezolva conflictul Shift/Reduce */

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE
%left OR AND EQ NEQ '<' '>' LE GE 
%left '+' '-'
%left '*' '/'
%right UMINUS

%%

progr : global_declarations main 
      { 
        if(errorCount == 0 && mainBlock) {
            cout << "Syntax OK. Executing...\n----------------\n";
            mainBlock->eval();
        } else {
            cout << "Errors found, skipping execution.\n";
        }
        for(auto t : allTables) { 
             if(t->getName() == "global") t->printTable(tableOut); 
        }
      }
      ;

global_declarations : /* e */ | global_declarations decl ;
decl : class_def | func_def | var_decl_global ;

/* DECLARATII GLOBALE */
var_decl_global 
    : TYPE ID ';' {
        Value v; v.type = *$1; 
        currentScope->addVar(*$2, *$1, v);
    }
    | ID ID ';' {
        Value v; v.type = *$1; 
        currentScope->addVar(*$2, *$1, v);
    }
    ;

/* CLASE */
class_def : CLASS ID { enterScope("class_" + *$2); } '{' class_block '}' ';' { exitScope(); };
class_block : /* e */ | class_block var_decl_global | class_block func_def;

/* FUNCTII */
func_def 
    : TYPE ID '(' { 
        currentFuncName = *$2;
        tempParamTypes.clear(); 
        tempParamNames.clear(); 
      } param_list ')' func_body {
        currentScope->addFunc(*$2, *$1, tempParamTypes, tempParamNames, $7);
      }
    | ID ID '(' { 
        currentFuncName = *$2;
        tempParamTypes.clear(); 
        tempParamNames.clear(); 
      } param_list ')' func_body {
        currentScope->addFunc(*$2, *$1, tempParamTypes, tempParamNames, $7);
      }
    ;

param_list : /* e */ | param_item | param_list ',' param_item;

param_item 
    : TYPE ID { tempParamTypes.push_back(*$1); tempParamNames.push_back(*$2); }
    | ID ID   { tempParamTypes.push_back(*$1); tempParamNames.push_back(*$2); }
    ;

func_body : '{' { 
     string funcName = "func_" + currentFuncName; 
     enterScope(funcName);
     for(size_t i=0; i<tempParamNames.size(); ++i) {
        Value v; v.type = tempParamTypes[i];
        currentScope->addVar(tempParamNames[i], tempParamTypes[i], v);
     }
   } list_statements '}' { 
     $$ = $3; 
     exitScope(); 
   };

/* MAIN */
main : BGIN { enterScope("main"); } main_list END { mainBlock = $3; exitScope(); };
main_list : /* e */ { $$ = new BlockNode(); }
          | main_list statement { if ($2) $1->addStatement($2); $$ = $1; };

/* STATEMENTS */
statement : assignment ';' { $$ = $1; }
          | print_stmt ';' { $$ = $1; }
          | var_decl_local { $$ = nullptr; }
          | if_stmt { $$ = $1; }
          | while_stmt { $$ = $1; }
          | block { $$ = $1; }
          | RETURN expr ';' { $$ = new ReturnNode($2, yylineno); }
          | call ';' { $$ = $1; }
          ;

block : '{' list_statements '}' { $$ = $2; };
list_statements : /* e */ { $$ = new BlockNode(); }
                | list_statements statement { if($2) $1->addStatement($2); $$ = $1; };

/* DECLARATII LOCALE */
var_decl_local 
    : TYPE ID ';' { 
        Value v; v.type = *$1; currentScope->addVar(*$2, *$1, v); 
      }
    | ID ID ';' { 
        Value v; v.type = *$1; currentScope->addVar(*$2, *$1, v); 
      }
    ;

/* ATRIBUIRE */
assignment : ID ASSIGN expr { 
               $$ = new AssignNode("", *$1, $3, currentScope, yylineno); 
           }
           | ID DOT ID ASSIGN expr {
               $$ = new AssignNode(*$1, *$3, $5, currentScope, yylineno); 
           }
           ;

print_stmt : PRINT '(' expr ')' { $$ = new PrintNode($3); };

if_stmt : IF '(' expr ')' statement %prec LOWER_THAN_ELSE { $$ = new IfNode($3, $5, nullptr, yylineno); }
        | IF '(' expr ')' statement ELSE statement { $$ = new IfNode($3, $5, $7, yylineno); }
        ;

while_stmt : WHILE '(' expr ')' statement { $$ = new WhileNode($3, $5, yylineno); };

/* EXPRESII */
expr : expr '+' expr { $$ = new BinaryNode($1, "+", $3, yylineno); }
     | expr '-' expr { $$ = new BinaryNode($1, "-", $3, yylineno); }
     | expr '*' expr { $$ = new BinaryNode($1, "*", $3, yylineno); }
     | expr '/' expr { $$ = new BinaryNode($1, "/", $3, yylineno); }
     | expr '<' expr { $$ = new BinaryNode($1, "<", $3, yylineno); }
     | expr '>' expr { $$ = new BinaryNode($1, ">", $3, yylineno); }
     | expr GE expr  { $$ = new BinaryNode($1, ">=", $3, yylineno); }
     | expr LE expr  { $$ = new BinaryNode($1, "<=", $3, yylineno); }
     | expr EQ expr  { $$ = new BinaryNode($1, "==", $3, yylineno); }
     | expr NEQ expr { $$ = new BinaryNode($1, "!=", $3, yylineno); }
     | expr AND expr { $$ = new BinaryNode($1, "&&", $3, yylineno); }
     | expr OR expr  { $$ = new BinaryNode($1, "||", $3, yylineno); }
     | '(' expr ')'  { $$ = $2; }
     | variable { $$ = $1; }
     | call { $$ = $1; }
     | '-' expr %prec UMINUS { 
         Value zeroVal; zeroVal.type="int"; zeroVal.iVal=0; zeroVal.fVal=0.0;
         $$ = new BinaryNode(new ConstNode(zeroVal), "-", $2, yylineno); 
       }
     | INT_VAL { $$ = new ConstNode(*$1); delete $1; }
     | FLOAT_VAL { $$ = new ConstNode(*$1); delete $1; }
     | STRING_VAL { $$ = new ConstNode(*$1); delete $1; }
     | BOOL_VAL { $$ = new ConstNode(*$1); delete $1; }
     ;

variable : ID { $$ = new IdNode(*$1, currentScope, yylineno); }
         | ID DOT ID { $$ = new MemberAccessNode(*$1, *$3, currentScope, yylineno); }
         ;

/* APEL FUNCTII */
call : ID '(' { tempArgs.clear(); } args_list ')' {
         $$ = new CallNode("", *$1, tempArgs, currentScope, yylineno);
     }
     | ID DOT ID '(' { tempArgs.clear(); } args_list ')' {
         $$ = new CallNode(*$1, *$3, tempArgs, currentScope, yylineno);
     }
     ;

args_list : /* empty */
          | real_args_list
          ;

real_args_list : arg_item
               | real_args_list ',' arg_item
               ;

arg_item : expr { tempArgs.push_back($1); };

%%
void yyerror(const char * s){ cout << "Error: " << s << " at line: " << yylineno << endl; }
int main(int argc, char** argv){
     if(argc > 1) yyin = fopen(argv[1],"r");
     currentScope = new SymTable("global");
     allTables.push_back(currentScope);
     yyparse();
     tableOut.close();
}