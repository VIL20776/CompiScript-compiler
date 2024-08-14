#pragma once

#include <string>
#include <unordered_map>

enum SymbolType: int {
    VARIABLE,
    FUNCTION,
    CLASS,
    OBJECT
};

struct SymbolData {
    std::string name; 
    SymbolType type;
    std::string data_type;
    std::string value;
    int size;
    int method_table_index;
    bool global;
};

class SymbolTable
{
private:
    std::unordered_map<std::string, SymbolData> table;
public:
    SymbolTable(/* args */);
    ~SymbolTable();

    bool bind(SymbolData symbol);    
    bool lookup(std::string name);
    bool update(std::string name, SymbolData symbol);

};
