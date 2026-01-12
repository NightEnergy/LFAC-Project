#pragma once
#include "SymTable.h"
#include <iostream>
#include <vector>
#include <cstdlib>

using namespace std;

// Functie helper pentru erori fatale cu linie
inline void fatalError(string msg, int line = -1) {
    cerr << "[Runtime/Semantic Error";
    if(line != -1) cerr << " at line " << line;
    cerr << "]: " << msg << endl;
    exit(1);
}

// Structura pentru a propaga valoarea RETURN prin stiva de apeluri
struct ReturnException {
    Value val;
    ReturnException(Value v) : val(v) {}
};

class ASTNode {
protected:
    int line = -1;
public:
    virtual Value eval() = 0;
    virtual string getType() = 0;
    virtual ~ASTNode() {}
    void setLine(int l) { line = l; }
};

class BlockNode : public ASTNode {
    vector<ASTNode*> statements;
public:
    void addStatement(ASTNode* node) { statements.push_back(node); }
    string getType() override { return "void"; }
    Value eval() override {
        Value lastVal;
        for(auto* node : statements) if(node) lastVal = node->eval();
        return lastVal;
    }
};

// --- ReturnNode (NOU) ---
class ReturnNode : public ASTNode {
    ASTNode* expr;
public:
    ReturnNode(ASTNode* e, int l) : expr(e) { line = l; }
    string getType() override { return expr->getType(); }
    Value eval() override {
        Value v = expr->eval();
        throw ReturnException(v); // Intrerupe executia si trimite valoarea sus
    }
};

class ConstNode : public ASTNode {
    Value val;
public:
    ConstNode(Value v) : val(v) {}
    Value eval() override { return val; }
    string getType() override { return val.type; }
};

class IdNode : public ASTNode {
    string name;
    SymTable* scope;
public:
    IdNode(string n, SymTable* s, int l) : name(n), scope(s) { line = l; }
    
    string getType() override {
        IdInfo* info = scope->lookup(name);
        if(!info) fatalError("Variable '" + name + "' used but not declared.", line);
        return info->type;
    }

    Value eval() override {
        IdInfo* info = scope->lookup(name);
        if(!info) fatalError("Variable '" + name + "' not found in memory.", line);
        return info->value;
    }
};

class MemberAccessNode : public ASTNode {
    string objName;
    string memberName;
    SymTable* currentScope;
public:
    MemberAccessNode(string obj, string mem, SymTable* s, int l) 
        : objName(obj), memberName(mem), currentScope(s) { line = l; }

    IdInfo* getMemberInfo() {
        IdInfo* objInfo = currentScope->lookup(objName);
        if(!objInfo) fatalError("Object '" + objName + "' not found.", line);
        
        SymTable* global = SymTable::findGlobalScope(currentScope);
        SymTable* classScope = global->findChildScope("class_" + objInfo->type);
        if(!classScope) fatalError("Class definition for '" + objInfo->type + "' not found.", line);

        IdInfo* member = classScope->lookup(memberName);
        if(!member) fatalError("Member '" + memberName + "' not found in class '" + objInfo->type + "'.", line);
        
        return member;
    }

    string getType() override {
        return getMemberInfo()->type;
    }

    Value eval() override {
        return getMemberInfo()->value;
    }
};

class CallNode : public ASTNode {
    string objName;
    string funcName;
    vector<ASTNode*> args;
    SymTable* currentScope;
public:
    CallNode(string obj, string func, vector<ASTNode*> a, SymTable* s, int l) 
        : objName(obj), funcName(func), args(a), currentScope(s) { line = l; }

    IdInfo* getFuncInfo(SymTable*& outFuncParentScope) {
        SymTable* searchScope = currentScope;
        
        if(!objName.empty()) {
            IdInfo* objInfo = currentScope->lookup(objName);
            if(!objInfo) fatalError("Object '" + objName + "' not found for method call.", line);
            
            SymTable* global = SymTable::findGlobalScope(currentScope);
            searchScope = global->findChildScope("class_" + objInfo->type);
            if(!searchScope) fatalError("Class scope for object '" + objName + "' not found.", line);
        }

        while (searchScope != nullptr) {
            if (searchScope->existsInCurrentScope(funcName)) {
                IdInfo* info = searchScope->lookup(funcName);
                if (info->category == "func") {
                    outFuncParentScope = searchScope;
                    return info;
                }
            }
            searchScope = searchScope->getParent();
        }
        
        fatalError("Function '" + funcName + "' not found.", line);
        return nullptr;
    }

    string getType() override {
        SymTable* dummy;
        IdInfo* info = getFuncInfo(dummy);
        return info->type;
    }

    Value eval() override {
        SymTable* funcScopeParent = nullptr;
        IdInfo* funcInfo = getFuncInfo(funcScopeParent);

        if(args.size() != funcInfo->paramNames.size()) {
            fatalError("Function '" + funcName + "' expects " + to_string(funcInfo->paramNames.size()) + 
                       " arguments, but got " + to_string(args.size()) + ".", line);
        }

        vector<Value> argValues;
        for(auto* node : args) argValues.push_back(node->eval());

        SymTable* funcScope = funcScopeParent->findChildScope("func_" + funcName);
        if(!funcScope) fatalError("Internal scope for function '" + funcName + "' lost.", line);

        auto stateBackup = funcScope->getIds();

        for(size_t i=0; i<funcInfo->paramNames.size(); ++i) {
             if(argValues[i].type != funcInfo->paramTypes[i]) {
                 fatalError("Argument " + to_string(i+1) + " of function '" + funcName + 
                            "' mismatch. Expected " + funcInfo->paramTypes[i] + ", got " + argValues[i].type, line);
             }
             funcScope->addVar(funcInfo->paramNames[i], funcInfo->paramTypes[i], argValues[i]);
        }

        Value res;
        res.type = funcInfo->type; 

        if(funcInfo->funcBody) {
             try {
                 // Incercam sa executam. Daca intalneste RETURN, va arunca exceptie.
                 funcInfo->funcBody->eval();
             } catch (ReturnException& e) {
                 res = e.val; // Prindem valoarea returnata
             }
             
             if(funcInfo->type == "void") res.type = "void"; 
             else if (res.type != funcInfo->type) {
                 if(res.type == "") fatalError("Function '" + funcName + "' did not return a value.", line);
                 fatalError("Function '" + funcName + "' returned '" + res.type + "' but expected '" + funcInfo->type + "'.", line);
             }
        }

        funcScope->setIds(stateBackup);
        return res;
    }
};

class BinaryNode : public ASTNode {
    ASTNode *left, *right;
    string op;
public:
    BinaryNode(ASTNode* l, string o, ASTNode* r, int ln) : left(l), op(o), right(r) { line = ln; }
    
    string getType() override {
        string t1 = left->getType();
        string t2 = right->getType();
        if(t1 != t2) fatalError("Type mismatch: " + t1 + " " + op + " " + t2, line);

        if (op == "<" || op == ">" || op == "<=" || op == ">=" || 
            op == "==" || op == "!=" || op == "&&" || op == "||") return "bool";
        if (op == "+" || op == "-" || op == "*" || op == "/") return t1;
        return "unknown";
    }

    Value eval() override {
        Value v1 = left->eval();
        Value v2 = right->eval();
        Value res; 
        
        bool typesMatch = false;

        if (v1.type == "bool" && v2.type == "bool") {
            typesMatch = true;
            res.type = "bool";
            if (op == "==") res.bVal = (v1.bVal == v2.bVal);
            else if (op == "!=") res.bVal = (v1.bVal != v2.bVal);
            else if (op == "&&") res.bVal = (v1.bVal && v2.bVal);
            else if (op == "||") res.bVal = (v1.bVal || v2.bVal);
            else fatalError("Invalid operator '" + op + "' for boolean.", line);
        }
        else if (v1.type == "int" && v2.type == "int") {
            typesMatch = true;
            if (op == "+") { res.type="int"; res.iVal = v1.iVal + v2.iVal; }
            else if (op == "-") { res.type="int"; res.iVal = v1.iVal - v2.iVal; }
            else if (op == "*") { res.type="int"; res.iVal = v1.iVal * v2.iVal; }
            else if (op == "/") { 
                if(v2.iVal == 0) fatalError("Division by zero.", line);
                res.type="int"; res.iVal = v1.iVal / v2.iVal; 
            }
            else if (op == "==") { res.type="bool"; res.bVal = (v1.iVal == v2.iVal); }
            else if (op == "!=") { res.type="bool"; res.bVal = (v1.iVal != v2.iVal); }
            else if (op == "<") { res.type="bool"; res.bVal = (v1.iVal < v2.iVal); }
            else if (op == ">") { res.type="bool"; res.bVal = (v1.iVal > v2.iVal); }
            else if (op == "<=") { res.type="bool"; res.bVal = (v1.iVal <= v2.iVal); }
            else if (op == ">=") { res.type="bool"; res.bVal = (v1.iVal >= v2.iVal); }
            else fatalError("Invalid operator '" + op + "' for int.", line);
        }
        else if (v1.type == "float" && v2.type == "float") {
             typesMatch = true;
             if (op == "+") { res.type="float"; res.fVal = v1.fVal + v2.fVal; }
             else if (op == "-") { res.type="float"; res.fVal = v1.fVal - v2.fVal; }
             else if (op == "*") { res.type="float"; res.fVal = v1.fVal * v2.fVal; }
             else if (op == "/") { 
                 if(v2.fVal == 0.0) fatalError("Division by zero.", line);
                 res.type="float"; res.fVal = v1.fVal / v2.fVal; 
             }
             else if (op == "==") { res.type="bool"; res.bVal = (v1.fVal == v2.fVal); }
             else if (op == "<") { res.type="bool"; res.bVal = (v1.fVal < v2.fVal); }
             else if (op == ">") { res.type="bool"; res.bVal = (v1.fVal > v2.fVal); }
             else if (op == "<=") { res.type="bool"; res.bVal = (v1.fVal <= v2.fVal); }
             else if (op == ">=") { res.type="bool"; res.bVal = (v1.fVal >= v2.fVal); }
             else fatalError("Invalid operator '" + op + "' for float.", line);
        }
        else if(v1.type == "string" && v2.type == "string") {
            typesMatch = true;
            if(op == "+") { res.type="string"; res.sVal = v1.sVal + v2.sVal; }
            else if(op == "==") { res.type="bool"; res.bVal = (v1.sVal == v2.sVal); }
            else fatalError("Invalid operator '" + op + "' for string.", line);
        }

        if (!typesMatch) fatalError("Type mismatch: " + v1.type + " " + op + " " + v2.type, line);
        return res;
    }
};

class AssignNode : public ASTNode {
    string objName;
    string varName;
    ASTNode* expr;
    SymTable* currentScope;
public:
    AssignNode(string obj, string v, ASTNode* e, SymTable* s, int l) 
        : objName(obj), varName(v), expr(e), currentScope(s) { line = l; }
    
    string getType() override { return expr->getType(); }

    Value eval() override {
        Value v = expr->eval();
        IdInfo* info = nullptr;

        if(objName.empty()) {
            info = currentScope->lookup(varName);
        } else {
            IdInfo* objInfo = currentScope->lookup(objName);
            if(objInfo) {
                SymTable* global = SymTable::findGlobalScope(currentScope);
                SymTable* classScope = global->findChildScope("class_" + objInfo->type);
                if(classScope) info = classScope->lookup(varName);
            }
        }

        if (info) {
            if(info->value.type != "" && info->value.type != v.type) {
                fatalError("Cannot assign " + v.type + " to variable '" + info->name + "' of type " + info->value.type, line);
            }
            info->value = v;
            if(info->value.type == "") info->value.type = v.type;
        } else {
            fatalError("Variable '" + varName + "' not found during assignment.", line);
        }
        return v;
    }
};

class PrintNode : public ASTNode {
    ASTNode* expr;
public:
    PrintNode(ASTNode* e) : expr(e) {}
    string getType() override { return "void"; }
    Value eval() override {
        Value v = expr->eval();
        if (v.type == "int") cout << "[PRINT]: " << v.iVal << endl;
        else if (v.type == "float") cout << "[PRINT]: " << v.fVal << endl;
        else if (v.type == "string") cout << "[PRINT]: " << v.sVal << endl;
        else if (v.type == "bool") cout << "[PRINT]: " << (v.bVal ? "true" : "false") << endl;
        else fatalError("Unknown type in Print.");
        return v;
    }
};

class IfNode : public ASTNode {
    ASTNode *cond, *thenB, *elseB;
public:
    IfNode(ASTNode* c, ASTNode* t, ASTNode* e, int l) : cond(c), thenB(t), elseB(e) { line = l; }
    string getType() override { return "void"; }
    
    Value eval() override {
        Value c = cond->eval();
        bool isTrue = false;
        
        if(c.type == "bool") isTrue = c.bVal;
        else if(c.type == "int") isTrue = (c.iVal != 0);
        else fatalError("Condition in IF must be bool or int.", line);
        
        if(isTrue) {
            if(thenB) thenB->eval();
        } else {
            if(elseB) elseB->eval();
        }
        return {};
    }
};

class WhileNode : public ASTNode {
    ASTNode* condition;
    ASTNode* body;
public:
    WhileNode(ASTNode* c, ASTNode* b, int l) : condition(c), body(b) { line = l; }
    string getType() override { return "void"; }
    
    Value eval() override {
        while(true) {
            Value c = condition->eval();
            bool isTrue = false;
            
            if(c.type == "bool") isTrue = c.bVal;
            else if(c.type == "int") isTrue = (c.iVal != 0);
            else fatalError("Condition in WHILE must be bool or int.", line);
            
            if(!isTrue) break;
            
            if(body) body->eval();
        }
        return {};
    }
};