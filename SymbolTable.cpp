#include<iostream>
#include<string>
#include<fstream>
#include<sstream>
#include<vector>
using namespace std;

class SymbolInfo{ 
    public: 
    string name;
    string type;
    string varType;
    string retType;
    string* args;
    string* fields;
    string* fieldTypes;
    int numFields;
    int numArgs;
    bool isDefined;
    bool isFunction;
    bool isArray;
    SymbolInfo* next;
    SymbolInfo(){
        this->name = "";
        this->type = "";
        this->retType = "";
        this->varType = "";
        this->args = nullptr;
        this->fields = nullptr;
        this->fieldTypes = nullptr;
        this->numFields = 0;
        this->numArgs = 0;
        this->next = nullptr;
        this->isDefined = false;
        this->isFunction = false;
        this->isArray = false;
    }
    SymbolInfo(string name, string full, string varType, bool isFunction=false, bool isDefined=false, bool isArray=false){
        this->name = name;
        this->type = "ID";
        this->retType = "";
        this->varType = varType;
        this->args = nullptr;
        this->fields = nullptr;
        this->fieldTypes = nullptr;
        this->numFields = 0;
        this->numArgs = 0;
        this->isDefined = isDefined;
        this->next = nullptr;
        this->isFunction = isFunction;
        this->isArray = isArray;
        if (isFunction) {
            addType(full);
        }

    }
    SymbolInfo(const SymbolInfo &symbol){
        this->name = symbol.name;
        this->type = symbol.type;
        this->retType = symbol.retType;
        this->varType = symbol.varType;
        this->args = nullptr;
        this->fields = nullptr;
        this->fieldTypes = nullptr;
        this->numFields = 0;
        this->numArgs = 0;
        this->isDefined = symbol.isDefined;
        this->isFunction = symbol.isFunction;
        this->isArray = symbol.isArray;
        this->next = nullptr;

        for (int i = 0; i < symbol.numArgs; i++) {
            addArgs(symbol.args[i]);
        }
        for (int i = 0; i < symbol.numFields; i++) {
            addField(symbol.fieldTypes[i], symbol.fields[i]);
        }
    }
    ~SymbolInfo(){
        delete[] args;
        delete[] fields;
        delete[] fieldTypes;
    }
    string getName(){
        return name;
    }
    string getType(){
        return type;
    }
    string getRetType(){
        return retType;
    }
    vector<string> getArgs(){
        vector<string> argList;
        for (int i = 0; i < numArgs; i++) {
            argList.push_back(args[i]);
        }
        return argList;
    }
    void setName(string name){
        this->name = name;
    }
    void setType(string type){
        this->type = type;
    }
    void setRetType(string retType){
        this->retType = retType;
    }
    void addArgs(string arg){
        numArgs++;
        string* newArgs = new string[numArgs];
        for (int i = 0; i < numArgs - 1; i++) {
            newArgs[i] = args[i];
        }
        newArgs[numArgs - 1] = arg;
        delete[] args;
        args = newArgs;
    }
    void addType(string full) { 
        istringstream iss(full);
        string dummyID, rType;
    
        if (iss >> dummyID >> rType) {
            setRetType(rType);
        }

        string rawParams;
        getline(iss, rawParams);

        if (rawParams.empty()) return;

        stringstream ss(rawParams);
        string paramToken;
        while (getline(ss, paramToken, ',')) {
            stringstream paramStream(paramToken);
            string typeSpec, varName;
            if (paramStream >> typeSpec) {                
                addArgs(typeSpec);                 
            }
        }
    }
    void addField(string fieldType, string fieldName){
        numFields++;
        string* newFields = new string[numFields];
        string* newFieldTypes = new string[numFields];
        for (int i = 0; i < numFields - 1; i++) {
            newFields[i] = fields[i];
            newFieldTypes[i] = fieldTypes[i];
        }
        newFields[numFields - 1] = fieldName;
        newFieldTypes[numFields - 1] = fieldType;
        delete[] fields;
        delete[] fieldTypes;
        fields = newFields;
        fieldTypes = newFieldTypes;
    }
};

class ScopeTable{
    SymbolInfo** table;
    string scopeID;
    int childNum;
    int numBuckets;
    
    string id;
    unsigned int SDBMHash(string str, unsigned int num_buckets) {
        unsigned int sum = 0;
        for (unsigned char c : str) {
            sum += c;
        }
        return sum % num_buckets;
    }
    public:
    int entries;
    ScopeTable *parentScope;
    ScopeTable(int numBuckets, ScopeTable *parentScope){
        this->numBuckets = numBuckets;
        entries=0;
        if (parentScope==nullptr){
            scopeID = "1";
            childNum = 0;
        }
        else{
            parentScope->childNum++;            
            string num = to_string( parentScope->childNum);
            scopeID = parentScope->scopeID + "."+num ;
            childNum = 0;
        }
        this->parentScope = parentScope;
        table = new SymbolInfo*[numBuckets];
        for(int i=0; i<numBuckets; i++){
            table[i] = nullptr;
        }
    }
    ~ScopeTable (){
        for(int i=0; i<numBuckets; i++){
            SymbolInfo* entry = table[i];
            while(entry != nullptr){
                SymbolInfo* temp = entry;
                entry = entry->next;
                delete temp;
            }
        }
        delete[] table;
    }   
    string getScopeID(){
        return scopeID;
    }
    bool Insert(SymbolInfo symbol){
        unsigned int index = SDBMHash(symbol.getName(), numBuckets);
        int n = 1;
        SymbolInfo* ptr = table[index];
        if (ptr == nullptr){
            SymbolInfo* newSymbol = new SymbolInfo(symbol);
            table[index] = newSymbol;
            entries++;
            return true;
        }
        else {
            while(ptr->next != nullptr){
                if(ptr->getName() == symbol.getName()){
                    //cout << "\t'" << symbol.getName() << "' already exists in the current ScopeTable" << endl;
                    return false;
                }
                ptr = ptr->next;
                n++;
            }
            if(ptr->getName() == symbol.getName()){
                //cout << "\t'" << symbol.getName() << "' already exists in the current ScopeTable" << endl;
                return false;
            }
            ptr->next = new SymbolInfo(symbol);
            entries++;
        }

        return true;
    }

    SymbolInfo* Lookup(string name){
        unsigned int index = SDBMHash(name, numBuckets);
        int n = 1;
        SymbolInfo* ptr = table[index];
        while(ptr != nullptr){
            if(ptr->getName() == name){
                return ptr;
            }
            ptr = ptr->next;
            n++;
        }

        if (scopeID == "1") {
            return nullptr;
        } else{ 
            return parentScope->Lookup(name); 
        }
    }

    bool Delete(string name){
        unsigned int index = SDBMHash(name, numBuckets);
        SymbolInfo* ptr = table[index];
        int n = 1;
        SymbolInfo* prev = nullptr;
        while(ptr != nullptr){
            if(ptr->getName() == name){
                if(prev == nullptr){
                    table[index] = ptr->next;
                } else {
                    prev->next = ptr->next;
                }
                delete ptr;
                entries--;
                return true;
            }
            prev = ptr;
            ptr = ptr->next;
            n++;
        }
        return false;
    }

    void Print(ofstream &logFile){

        logFile <<endl<< "ScopeTable # " << scopeID << endl;
        for(int i = 0; i < numBuckets; i++){            
            SymbolInfo* ptr = table[i];
            if(ptr != nullptr){      
                logFile << " " << i << " -->";          
                while(ptr != nullptr){
                    logFile << " < " << ptr->getName() << " , " << ptr->getType() << " >";
                    ptr = ptr->next;
                }
                logFile << endl;                
            }
        }
        logFile << endl<<endl;
    }

};

class SymbolTable{    
    public:
    ScopeTable *currentScope;
    SymbolTable(int numBuckets=30){
        currentScope = new ScopeTable(numBuckets, nullptr);
    }
    ~SymbolTable(){
        while(currentScope != nullptr){
            ScopeTable* temp = currentScope;
            currentScope = currentScope->parentScope;
            delete temp;
        }
    }
    void EnterScope(int numBuckets){
        ScopeTable *newScope = new ScopeTable(numBuckets, currentScope);
        currentScope = newScope;
    }
    bool ExitScope(int& cmdcount){
        if(currentScope->parentScope != nullptr){
            ScopeTable *temp = currentScope;
            currentScope = currentScope->parentScope;
            delete temp;
            return true;
        }
        else{
            return false;
        }
    }

    bool ExitScope(){
        if(currentScope->parentScope != nullptr){
            ScopeTable *temp = currentScope;
            currentScope = currentScope->parentScope;
            delete temp;
            return true;
        }
        else{
            delete currentScope;
            currentScope = nullptr;
            return false;
        }
    }
    bool InsertParent(string name, string type, string varType, bool isFunction=false, bool isDefined=false, bool isArray=false){
        SymbolInfo symbol(name, type, varType, isFunction, isDefined, isArray);
        return currentScope->parentScope->Insert(symbol);
    }
    bool Insert(string name, string type, string varType, bool isFunction=false, bool isDefined=false, bool isArray=false){
        SymbolInfo symbol(name, type, varType, isFunction, isDefined, isArray);
        return currentScope->Insert(symbol);
    }
    bool Remove(string name){
        return currentScope->Delete(name);
    }
    SymbolInfo* Lookup(string name){
        return currentScope->Lookup(name);
    }
    void PrintCurrent(ofstream &logFile){
        currentScope->Print(logFile);
    }
    void PrintAll(ofstream &logFile){
        ScopeTable *temp = currentScope;
        logFile << endl<<endl;
        int indent = 4;
        while(temp != nullptr){
            temp->Print(logFile);
            indent = indent + 4;
            temp = temp->parentScope;
        }
    }
};