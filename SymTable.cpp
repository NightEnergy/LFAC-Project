#include "SymTable.h"
#include "AST.h"

extern int yylineno;

SymTable::SymTable(string name, SymTable* p) : scopeName(name), parent(p) {}

bool SymTable::exists(string name) {
    if (ids.find(name) != ids.end()) return true;
    if (parent != nullptr) return parent->exists(name);
    return false;
}

bool SymTable::existsInCurrentScope(string name) {
    return ids.find(name) != ids.end();
}

IdInfo* SymTable::lookup(string name) {
    if (ids.find(name) != ids.end()) return &ids[name];
    if (parent != nullptr) return parent->lookup(name);
    return nullptr;
}

void SymTable::addVar(string name, string type, Value val) {
    if (ids.find(name) != ids.end()) {
        fatalError("Variable '" + name + "' already declared in scope '" + scopeName + "'.", yylineno);
    }
    
    IdInfo info(name, type, "var");
    info.value = val;
    info.value.type = type; 
    ids[name] = info;
}

void SymTable::addFunc(string name, string returnType, vector<string> pTypes, vector<string> pNames, ASTNode* body) {
    if (ids.find(name) != ids.end()) {
        fatalError("Function '" + name + "' already declared.", yylineno);
    }

    IdInfo info(name, returnType, "func");
    info.paramTypes = pTypes;
    info.paramNames = pNames;
    info.funcBody = body;
    ids[name] = info;
}

SymTable* SymTable::getParent() { return parent; }
string SymTable::getName() { return scopeName; }

SymTable* SymTable::findGlobalScope(SymTable* start) {
    if(start->getParent() == nullptr) return start;
    return findGlobalScope(start->getParent());
}

SymTable* SymTable::findChildScope(string childName) {
    for(auto* child : children) {
        if(child->getName() == childName) return child;
    }
    return nullptr;
}

void SymTable::printTable(ofstream& out) {
    out << "Scope: " << scopeName << endl;
    for (auto const& [key, val] : ids) {
        out << "  Name: " << val.name << " | Type: " << val.type 
            << " | Cat: " << val.category;
        
        if (val.category == "var") {
             if(val.type == "int") out << " | Val: " << val.value.iVal;
             else if(val.type == "float") out << " | Val: " << val.value.fVal;
             else if(val.type == "string") out << " | Val: " << val.value.sVal;
             else if(val.type == "bool") out << " | Val: " << (val.value.bVal ? "true" : "false");
        }
        else if (val.category == "func") {
            out << " | Params: (";
            for(size_t i=0; i<val.paramTypes.size(); ++i) {
                out << val.paramTypes[i] << " " << val.paramNames[i];
                if(i < val.paramTypes.size() - 1) out << ", ";
            }
            out << ")";
        }
        out << endl;
    }
    out << "-----------------------------------" << endl;
    
    for(auto* child : children) {
        child->printTable(out);
    }
}

SymTable::~SymTable() { ids.clear(); }
