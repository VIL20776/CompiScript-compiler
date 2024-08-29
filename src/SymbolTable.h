#pragma once

#include <map>
#include <string>
#include <vector>
#include <unordered_map>

enum class SymbolType: int {
    VARIABLE,
    FUNCTION,
    PARAMETER,
    CLASS,
    CLOSURE,
    ARRAY
};

enum class SymbolDataType: int {
    UNDEFINED,
    NUMBER,
    STRING,
    BOOLEAN,
    NIL,
    ANY // Placeholder para otros tipos
};

/*
name - Identificacion
scope - Ambito nombrado. Default = "local"
    Si es variable, indica la clase de la que es instancia.
    Si es funcion, indica a cual clase pertenece.
    Si es parametro, indica a que funcion pertenece.
    Si es clase, indica de que clase hereda
type - Tipo de simbolo
data_type - Tipo de dato. 
    Si es función o cerradura, es nil o any.
    Si es clase u objeto, es any.
value - Valor contenido en la variable.    
size - Tamaño del símbolo
offset - ubicacion en memoria

*/
struct SymbolData {
    std::string name;
    std::string scope;
    SymbolType type;
    SymbolDataType data_type; 
    std::string value;
    int size;
    int offset;
};

class SymbolTable
{
private:

    struct pair_hash {
        template <class T1, class T2>
        int operator() (const std::pair<T1, T2>& p) const {
            auto hash1 = std::hash<T1>{}(p.first);
            auto hash2 = std::hash<T2>{}(p.second);
            return hash1 ^ hash2;
        }
    };

    SymbolTable* parent;
    std::unordered_map<
        std::pair<std::string, std::string>, 
        SymbolData, 
        pair_hash
    > table;
    
public:
    SymbolTable(SymbolTable* parent = nullptr);
    ~SymbolTable();

    void insert(const SymbolData &symbol);

    /*Find symbols matching parent symbol*/
    std::vector<SymbolData> find_range(const std::string &scope);

    std::pair<SymbolData, bool> find(const std::string &symbol_name, const std::string &scope = "local");

    bool update(const std::string &symbol_name, const SymbolData &symbol, const std::string &scope = "local");

};
