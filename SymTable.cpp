#include "SymTable.h"
using namespace std;

void SymTable::addVar(string* type, string*name) {
    IdInfo var(type, name);
    ids[*name] = var; 
}


bool SymTable::existsId(string* var) {
    return ids.count(*var) > 0;  
}

SymTable::~SymTable() {
    ids.clear();
}











