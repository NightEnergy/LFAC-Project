#include <iostream>
#include <map>
#include <string>

using namespace std;

class IdInfo {
    public:
    string type;
    string name;
   
    IdInfo() {}
    IdInfo(string* type, string* name) : type(*type), name(*name) {}
};

class SymTable {
    SymTable* parent;
    map<string, IdInfo> ids;
    string name;
    public:
    SymTable(const char* , SymTable* parent = NULL) :  name(name){}
    bool existsId(string* s);
    void addVar(string* type, string* name );
    void printVars();
    ~SymTable();
};






