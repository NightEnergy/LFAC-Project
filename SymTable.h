#pragma once
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <fstream>

using namespace std;

class ASTNode;

struct Value {
    string type; // "int", "float", "bool", "string", "void"
    int iVal = 0;
    float fVal = 0.0;
    bool bVal = false;
    string sVal = "";
};

class IdInfo {
public:
    string name;
    string type;     
    string category; // "var", "func", "param"
    Value value;     
    
    vector<string> paramTypes; 
    vector<string> paramNames; 
    
    ASTNode* funcBody = nullptr;

    IdInfo() {}
    IdInfo(string n, string t, string c) : name(n), type(t), category(c) {}
};

class SymTable {
    map<string, IdInfo> ids;
    SymTable* parent;
    string scopeName; 

public:
    SymTable(string name, SymTable* parent = nullptr);
    
    bool exists(string name);
    bool existsInCurrentScope(string name);
    
    IdInfo* lookup(string name);
    
    void addVar(string name, string type, Value val = {});
    void addFunc(string name, string returnType, vector<string> pTypes, vector<string> pNames, ASTNode* body = nullptr);
    
    SymTable* getParent();
    string getName();
    static SymTable* findGlobalScope(SymTable* start);
    SymTable* findChildScope(string childName);

    void printTable(ofstream& out);
    ~SymTable();
    
    vector<SymTable*> children;

    map<string, IdInfo> getIds() { return ids; }
    void setIds(const map<string, IdInfo>& newIds) { ids = newIds; }
};